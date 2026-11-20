#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QStringList>
#include <QVector>
#include <QDataStream>
#include <memory>
#include "mFrpData_global.h"
#include "mPhysicalFieldEnum.h"

namespace MechFrpData
{
	class mFrpBaseData;
	class MXFRPDATA_EXPORT mFrpManager
	{
	public:
		mFrpManager();
		~mFrpManager();

		//通过id获取材料数据
		mFrpBaseData* getFrpBaseData(const QString& id);

		void clear();

		// 追加（而非替换）：把 other 的模型根并入本管理器，根级重名自动避让
		void appendFrom(mFrpManager& other);

		// 新建 AnalysisN 根，只挂 5 个空分组（RVE 带数据）
		mFrpBaseData* addAnalysisRoot();
		// 新建 Analysis 根时：只挂 5 个分组（材料/微观结构/RVE/失效/长细比），不建任何子节点
		void buildDefaultGroups(mFrpBaseData* a);

		// 唯一“加子节点”原语：微观结构分组下加微观结构N、各分组下加对应实例、微观结构N 下加四相（基体唯一）
		mFrpBaseData* addChildOfType(mFrpBaseData* parent, MxFunctions::FrpNodeType nodeType);
		static bool canAttachPhase(const mFrpBaseData* micro, MxFunctions::FrpNodeType nodeType, const mFrpBaseData* ignoredPhase = nullptr);

		// 相类型 -> 默认相名（界面相变形命名复用，避免两处维护相名表）
		static QString phaseName(MxFunctions::FrpNodeType phaseKind);

		// 深拷贝(含子树)到同级，_copy 命名
		mFrpBaseData* duplicateNode(mFrpBaseData* src);

		const QVector<mFrpBaseData*>& roots() const;
		const QStringList getAllRootsName() const;

		// 删除节点（含子树）
		bool removeNode(mFrpBaseData* data);

		// 删除某节点的全部直接子节点（连带子树），保留节点本身
		void clearChildren(mFrpBaseData* data);

		// 取向张量 6 分量排列转换（界面层 .mf 文本导入导出复用）：
		// 窗口存储序 a11;a12;a13;a22;a23;a33  <->  求解器序 a11,a22,a33,a23,a13,a12
		static QStringList reorderTensor6ToAbaqus(const QStringList& store);     // 存储序 -> 求解器序 [0,3,5,4,2,1]
		static QStringList reorderTensor6FromAbaqus(const QStringList& abaqus);  // 求解器序 -> 存储序（逆映射 [0,5,4,1,3,2]）

		// 改名并级联更新按名引用；空名/同级重名返回 false 拒绝
		bool renameNode(mFrpBaseData* node, const QString& newName);

	private:
		friend QDataStream& operator>>(QDataStream& in, mFrpManager& mgr);
		friend QDataStream& operator<<(QDataStream& out, const mFrpManager& mgr);
		friend QDataStream& InStreammFrpManagerV3(QDataStream& in, mFrpManager& mgr);

		//数据命名
		QString suggestName(const mFrpBaseData* parentData, MxFunctions::FrpNodeType newType);

		//新建索引id
		QString genId();
		void reassignIds(mFrpBaseData* n);

		// 把模型内引用 oldName 的基体材料字段改为 newName
		void remapMaterialRefs(mFrpBaseData* root, const QString& oldName, const QString& newName);

		void remapMicrostructureRefs(mFrpBaseData* root, const QString& oldName, const QString& newName);

		// 微观结构分组整棵子树（微观结构N + 相节点）内是否已存在 name（ignore 节点自身跳过）
		bool microSubtreeNameExists(const mFrpBaseData* microGroup,const QString& name, const mFrpBaseData* ignore) const;

	private:
		int m_idSeq = 0;
		int m_analysisSeq = 0;
		int m_materialSeq = 0;
		int m_microSeq = 0;
		int m_failureSeq = 0;
		int m_funSeq = 0;

		QVector<mFrpBaseData*> m_roots;

	};
}
