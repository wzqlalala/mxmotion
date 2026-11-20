#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector>
#include <QDataStream>

#include "mFrpData_global.h"
#include "mPhysicalFieldEnum.h"

using namespace MxFunctions;
namespace MechFrpData
{
	class MXFRPDATA_EXPORT mFrpBaseData
	{
	friend QDataStream& InStreammFrpBaseDataV3(QDataStream& in, mFrpBaseData& obj);
	public:
		mFrpBaseData(FrpNodeType t = FrpNode_None, const QString& n = QString(), mFrpBaseData* p = nullptr);
		mFrpBaseData(const mFrpBaseData& other);
		mFrpBaseData& operator=(const mFrpBaseData&) = delete;
		virtual ~mFrpBaseData();

		// 按节点类型 new 出对应数据类（相节点会设好 phaseKind）；分组类型(Micro 等)返回基类实例
		static mFrpBaseData* createByType(FrpNodeType nodeType);

		//查找该类型父节点
		mFrpBaseData* ancestorOfType(FrpNodeType nodeType);

		void addChild(mFrpBaseData* child);

		int childCountOfType(FrpNodeType nodeType) const;

		mFrpBaseData* findById(const QString& nodeId);

		// 强类型转调（用法：if(auto* d=node->as<mFrpMaterialData>()) ...）
		// 模板定义必须留在头文件：对任意 T 实例化，定义需在调用点可见，无法移到 .cpp
		template <class T> T* as() { return dynamic_cast<T*>(this); }
		template <class T> const T* as() const { return dynamic_cast<const T*>(this); }

		// 是否“无界面无数据”的分组节点（材料/微观/失效/长细比分组）
		bool isGroupData() const;

		virtual mFrpBaseData* clone(mFrpBaseData* parentNode = nullptr) const;

	protected:

		void cloneChildrenTo(mFrpBaseData* copy) const;

	public:
		// —— 树节点身份（所有节点共有）——
		QString id;                       // 全局唯一 id（视图据此反查模型节点）

		//数据类型
		FrpNodeType type = FrpNode_None;

		// 树显示名
		QString name;

		// 用户是否手动重命名过
		bool renamed = false;

		mFrpBaseData* m_pParent = nullptr;

		QVector<mFrpBaseData*> children;
	};
}
