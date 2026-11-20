#pragma once
// 双 ModelData 树同源共享头：MxAI 工作区同时安装 RebuildModelData(dev_Mech) 与
// RebuildModelData_Dyna(dev_Dyna)，本文件两树逐字节同源；#pragma once 按路径去重
// 无法跨树去重（双份均被包含即 C2011 重定义），补同名 include guard 先含者胜。
#ifndef MASSEMBLYMANAGER_H
#define MASSEMBLYMANAGER_H
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXAssemblyData_global.h"
#include "mAssemblyData.h"
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDataStream>
#include <algorithm>

namespace AssemblyData
{
	class mAssemblyManager;

	//流操作符需显式导出供跨 DLL 链接（如 ModelData 持久化段）；先于此处声明，
	//类内 friend 引用同一声明，避免链接属性不一致（C2375）
	MXASSEMBLYDATA_EXPORT QDataStream& operator<<(QDataStream& out, const mAssemblyManager& mgr);
	MXASSEMBLYDATA_EXPORT QDataStream& operator>>(QDataStream& in, mAssemblyManager& mgr);

	/*
	* 装配关系管理器：装配层级的唯一可信源（自持双向索引，与几何层解耦）。
	*
	* 关系语义：
	*  - 装配→子装配、装配→部件均为共享引用，同一子项可从属多个装配；
	*  - 所有装配挂载入口统一做 DAG 校验（wouldCreateCycle），禁止循环从属；
	*  - 删除装配只解除从属关系，不触碰部件/子装配实体；
	*  - 部件被删除时由外部调用 removePartReferences 清理全部引用。
	*/
	class MXASSEMBLYDATA_EXPORT mAssemblyManager
	{
	public:
		mAssemblyManager();
		~mAssemblyManager();

	public:
		//------------- 实体 CRUD -------------

		/*
		* 创建装配，返回新装配ID（ID 递增不复用，避免删除重建后树 key 冲突）
		*/
		int createAssembly(const QString& name);

		/*
		* 重命名装配
		*/
		bool renameAssembly(int assemblyId, const QString& name);

		/*
		* 删除装配：仅解除其全部从属关系（父装配对其的引用 + 其对子项的引用），
		* 不影响任何部件/子装配实体；失去全部父装配的子装配自动转为顶层装配。
		*/
		bool removeAssembly(int assemblyId);

		/*
		* 复制装配层级：新建同名（由调用方编号）装配，其子装配与子部件均为
		* 对原装配子项的共享引用（含嵌套子装配，不递归克隆实体）。
		*/
		int cloneAssembly(int sourceAssemblyId, const QString& newName);

		bool containsAssembly(int assemblyId) const;
		QString getAssemblyName(int assemblyId) const;
		bool existAssemblyName(const QString& name) const;
		int getIDByAssemblyName(const QString& name) const;
		QVector<int> getAllAssemblyIDs() const;
		QStringList getAllAssemblyNames() const;

		//------------- 从属关系 -------------

		/*
		* 挂载子装配（共享引用）。内部 DAG 校验：child 为 parent 自身或其祖先时
		* 拒绝并返回 false；已存在该引用时幂等返回 true。
		*/
		bool addChildAssembly(int parentAssemblyId, int childAssemblyId);

		/*
		* 挂载部件引用（部件为叶子，无循环风险）
		*/
		bool addChildPart(int parentAssemblyId, int partId);

		/*
		* 解除装配对子装配/部件的单条引用（多父子项保留其他从属）
		*/
		bool removeChildAssembly(int parentAssemblyId, int childAssemblyId);
		bool removeChildPart(int parentAssemblyId, int partId);

		QVector<int> childAssemblies(int assemblyId) const;
		QVector<int> childParts(int assemblyId) const;

		//某装配的所有父装配
		QVector<int> assemblyParents(int assemblyId) const;
		//某部件的所有父装配
		QVector<int> partParents(int partId) const;

		//部件是否被任一装配引用（自由区 = 全部部件 - 被引用部件）
		bool isPartReferenced(int partId) const;
		QSet<int> referencedPartIds() const;

		/*
		* 若把 childAssemblyId 挂到 parentAssemblyId 下是否形成循环：
		* parent 为 child 自身，或 parent 是 child 的后代时为真
		*/
		bool wouldCreateCycle(int parentAssemblyId, int childAssemblyId) const;

		//------------- 视图求值 -------------

		//顶层装配（无父装配），按名称排序
		QVector<int> topAssemblies() const;

		//装配的全部后代装配（不含自身）
		QVector<int> descendantAssemblies(int assemblyId) const;

		//装配下递归收集的全部部件ID（含嵌套子装配的部件，供显隐级联等使用）
		QVector<int> descendantPartIds(int assemblyId) const;

		//------------- 级联清理 -------------

		/*
		* 部件被删除时清理其在所有装配下的引用
		*/
		void removePartReferences(int partId);

		/*
		* 清空全部装配实体与关系（新建/关闭工程用）
		*/
		void clear();

		//------------- 持久化 -------------

		friend QDataStream& operator<<(QDataStream& out, const mAssemblyManager& mgr);
		friend QDataStream& operator>>(QDataStream& in, mAssemblyManager& mgr);

	private:
		/*
		* DFS 判断 target 是否在 from 的子装配闭包内（含 from 自身）
		*/
		bool isReachable(int from, int target) const;

	private:
		//装配实体表：assemblyId → 实体
		QHash<int, mAssemblyData*> _assemblys;

		//正向索引：装配 → 子装配ID集（共享引用）
		QHash<int, QSet<int>> _asmChildrenAsm;
		//正向索引：装配 → 子部件ID集（共享引用）
		QHash<int, QSet<int>> _asmChildrenPart;

		//反向索引：子装配 → 父装配ID集
		QHash<int, QSet<int>> _asmParents;
		//反向索引：部件 → 父装配ID集
		QHash<int, QSet<int>> _partParents;

		//装配ID分配器（递增不复用）
		int _nextAssemblyID = 1;
	};
}
#endif // MASSEMBLYMANAGER_H
