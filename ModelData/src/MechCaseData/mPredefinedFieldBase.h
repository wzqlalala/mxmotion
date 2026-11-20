#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"
#include <memory>
#include <QString>
#include <QVariant>
#include <QVector>
#include "mUnitSystemManager.h"

using namespace CommonData;
namespace MechCaseData
{
	class MXCASEDATA_EXPORT mPredefinedFieldBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mPredefinedFieldBase(int id, MxFunctions::PredefinedFieldType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mPredefinedFieldBase() {}

		/*
		* 析构函数
		*/
		~mPredefinedFieldBase();


		/*
		* 设置预定义场ID
		*/
		void setPredefinedFieldId(int id);

		/*
		* 获取预定义场ID
		*/
		int getPredefinedFieldId();

		/*
		* 设置预定义场名
		*/
		void setPredefinedFieldName(const QString& name);

		/*
		* 获取预定义场名
		*/
		QString getPredefinedFieldName();

		/*
		* 设置预定义场类型
		*/
		void setPredefinedFieldType(MxFunctions::PredefinedFieldType type);

		/*
		* 获取预定义场类型
		*/
		MxFunctions::PredefinedFieldType getPredefinedFieldType();

		/*
		* 设置拾取状态（运行时状态，不参与序列化：0-未编辑 1-面板编辑中）
		*/
		void setPickState(int pickstate);

		/*
		* 获取拾取状态
		*/
		int getPickState();

		/*
		* 拷贝预定义场
		*/
		virtual std::shared_ptr<mPredefinedFieldBase> copyPredefinedField(const int& id, const QString& name) = 0;

		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) { return MxFunctions::BrowserTreeStatus::Tree_Yes; };

		/*
		* 响应单位制变化
		*/
		virtual void responseUnitChanged(const int& unitidx, mUnitSystemManager* unitsystemmanager) {};

		friend QDataStream& operator>>(QDataStream& in, mPredefinedFieldBase& obj);
		friend QDataStream& InStreammPredefinedFieldBaseV3(QDataStream& in, mPredefinedFieldBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mPredefinedFieldBase& obj);

	private:
		//预定义场ID
		int _id;

		//预定义场名字
		QString _predefinedFieldName;

		//预定义场类型
		MxFunctions::PredefinedFieldType _predefinedFieldType;

		//拾取状态（运行时状态：0-未编辑 1-面板编辑中）
		int _pickState = 0;
	};
}
