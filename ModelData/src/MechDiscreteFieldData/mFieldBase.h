#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXDiscreteFieldData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include "mBasicEnum.h"

//Qt
#include <QString>
#include <QVector>
#include <QPair>

namespace MechDiscreteFieldData
{
	/*
	* 场数据基类：离散场（mDiscreteField）与分析场（mAnalysisField）均归于场大类下，
	* 统一持有ID与名称，通过 getFieldType() 区分子类型。
	*/
	class MXDISCRETEFIELDDATA_EXPORT mFieldBase
	{
	public:
		mFieldBase(int id, const QString& name = "") : _id(id), _name(name) {}
		mFieldBase() = default;
		virtual ~mFieldBase() = default;

		/*
		* 场子类型
		*/
		virtual MxFunctions::FieldType getFieldType() const = 0;

		/*
		* 拷贝场数据（子类实现深拷贝）
		*/
		virtual mFieldBase* copyField() = 0;

		/*
		* 设置/获取ID
		*/
		void setId(int id) { _id = id; }
		int getId() const { return _id; }

		/*
		* 场名称
		*/
		void setFieldName(const QString& name) { _name = name; }
		QString getFieldName() const { return _name; }

		/*
		* 判断节点状态
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) = 0;

	protected:
		//场ID
		int _id = -1;

		//场名称
		QString _name;
	};
}
