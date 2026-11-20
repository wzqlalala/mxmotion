#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "mPostDerivedCaseTypes.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>

#include <functional>

namespace PostWindow
{
	/*
	* 派生工况定义存储（进程级单例；纯数据类，无 QObject/UI 依赖——仿
	* mPostResultRegistry 先例，无头单测覆盖 tst_mPostDerivedCaseStore）。
	*
	* 分桶：按"后处理模型实例"隔离，键 = mModelData::getModelDataID()。
	* 同一结果文件多次导入为各自独立实例、互不串定义；模型删除由
	* deletePostModelBrowserTree 链调 removeModel 回收残桶。
	*
	* 职责收敛（替代步序列/包络/线性叠加三面板各自的重复实现，2026-11-12
	* 派生工况开发计划批次A0）：
	*  - 定义 CRUD 与 id 发号（id 进程内全局单调、永不复用）；
	*  - 名称空间：同桶全 kind 共用一个名域（树"派生工况"分支同名节点无法
	*    区分）——默认命名查重递增（"步序列-N"）与重名校验；
	*  - 引用校验（原型 validateDefinition）：环检测 + 两层派生引用上限 +
	*    引用目标缺失；支持编辑草稿模拟（editingId 的启用派生引用集以
	*    draftRefs 覆盖，candidate 追加模拟"再勾选一行"）；
	*  - 定义展开（引擎生成用，批次A1接入）：递归展开派生来源至原生来源，
	*    同帧比例相加；
	*  - 序列化往返（.mxses postItemsChunk 派生块载荷，帧头由会话层拼装）。
	*
	* 变更通知：definitionsChanged 回调列表（树"派生工况"分支重建订阅；
	* store 无 QObject 不能发信号）。回调在 UI 线程同步触发，回调内禁止再
	* 改 store（防重入）。仅 UI 线程调用。
	*/
	class MPOSTWINDOW_EXPORT mPostDerivedCaseStore
	{
	public:
		//引用校验结果（错误文案归调用方面板——中英随界面语言）
		enum class ReferenceError
		{
			None = 0,
			MissingTarget,	//引用的派生工况不存在
			Cycle,			//自身/循环引用
			DepthExceeded,	//超过两层派生引用
			InvalidScale	//来源行缩放系数非有限数值（展开时校验）
		};

		//删除定义结果
		enum class RemoveResult
		{
			Removed = 0,
			NotFound,
			Referenced	//被其它定义引用，须先删引用方（referencedBy 回带引用方 id 列表）
		};

		//展开产物：定义递归展开后的原生来源
		struct ExpandedSource
		{
			QString caseName;	//原生工况名
			QString frameName;	//原生帧名
			QString rowId;		//来源行标识（原生 "工况|帧"）
			int frameId = 0;	//线性叠加定义展开出的帧号（其余 kind 为 0，生成时解析）
			double scale = 1.0;	//同帧累计比例
		};

		static mPostDerivedCaseStore& instance();

		//——定义集——
		QList<DerivedCaseDefinition> definitions(int modelKey) const;					//全 kind（树分支用）
		QList<DerivedCaseDefinition> definitions(int modelKey, DerivedCaseKind kind) const;	//单 kind（面板用）
		bool definition(int modelKey, int definitionId, DerivedCaseDefinition* out = nullptr) const;

		//新增定义：id 由 store 发号（覆盖传入 def.id）；名称空/重名拒绝返回 -1
		int appendDefinition(int modelKey, const DerivedCaseDefinition& def);
		//按 id 整体替换（面板 commit 写回；id 不存在或名称与其它定义重复返回 false）
		bool updateDefinition(int modelKey, const DerivedCaseDefinition& def);
		//删除定义；被引用时拒绝（先删引用方）
		RemoveResult removeDefinition(int modelKey, int definitionId, QList<int>* referencedBy = nullptr);
		//模型删除清理（回收残桶；不触发变更通知——桶已随模型消亡）
		void removeModel(int modelKey);

		//——名称空间（同桶全 kind 共用一名域）——
		bool nameExists(int modelKey, const QString& name, int exceptId = -1) const;
		//默认命名查重递增：base + "-N"（N 自 1 递增至不重名）
		QString uniqueName(int modelKey, const QString& base) const;

		/*
		* 引用校验（纯函数，无状态）：defs 为该桶全 kind 定义集；editingId 的
		* 启用派生引用集以 draftRefs 覆盖（面板编辑草稿模拟；editingId<0 即
		* 校验存量集）；candidateDerivedId>0 模拟"再勾选一个派生来源行"
		* （行级禁用预判/整体兜底共用）。
		*/
		static ReferenceError checkReferences(const QList<DerivedCaseDefinition>& defs,
			int editingId, const QList<int>& draftRefs, int candidateDerivedId = -1);

		/*
		* 定义展开（纯函数）：definitionId 递归展开至原生来源列表（按定义行序，
		* 同帧比例相加）；失败（引用缺失/环/超两层/系数非法）返回 false 并经
		* errorKind 回带原因。checkReferences 通过的存量集不会失败，防御性
		* 保留环/深度检查（引擎消费未校验数据时的兜底）。
		*/
		static bool expandDefinition(const QList<DerivedCaseDefinition>& defs, int definitionId,
			QList<ExpandedSource>* out, ReferenceError* errorKind = nullptr);

		//——序列化（.mxses postItemsChunk 派生块载荷；QDataStream 版本与会话层同口径）——
		QByteArray serialize(int modelKey) const;
		//恢复（整桶替换——会话恢复时模型为新建空桶；恢复后 id 计数器抬升防新发号撞已恢复 id；
		//返回恢复定义数，载荷不合法返回 -1）
		int deserialize(int modelKey, const QByteArray& blob);

		//——变更通知（参数=modelKey；订阅方自行过滤与本模型相关的变更）——
		void addChangeListener(const std::function<void(int)>& listener);

		//测试隔离：清空全部桶/计数器/监听器（仅无头单测调用）
		void clearAll_forTest();

	private:
		mPostDerivedCaseStore() = default;

		void notifyChanged(int modelKey);
		bool bucketNameOk(int modelKey, const QString& name, int exceptId) const;

		QHash<int, QList<DerivedCaseDefinition>> _buckets;	//modelDataID → 定义集
		int _nextId = 1;									//定义 id 发号（进程内全局单调）
		QList<std::function<void(int)>> _listeners;
	};
}
