#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mPredefinedFieldBase.h"
#include "mModelStruct.h"
#include <map>
#include <QVector>
#include <QVariant>

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mPredefinedFieldTempLoad :public mPredefinedFieldBase
	{
	public:
		/*
		* 构造函数
		*/
		mPredefinedFieldTempLoad(int id, MxFunctions::PredefinedFieldType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mPredefinedFieldTempLoad() {}

		/*
		* 析构函数
		*/
		~mPredefinedFieldTempLoad();

		/*
		* 拷贝
		*/
		std::shared_ptr<mPredefinedFieldBase> copyPredefinedField(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mPredefinedFieldTempLoad& obj);
		friend QDataStream& InStreammPredefinedFieldTempLoadV3(QDataStream& in, mPredefinedFieldTempLoad& obj);
		friend QDataStream& operator<<(QDataStream& out,mPredefinedFieldTempLoad& obj);

	public:
		//定义方式：0=统一赋值, 1=独立赋值, 2=文件定义
		int _defineType = 0;

		// 统一赋值温度值
		QString _temp;

		// 独立赋值温度表：key=节点拾取结果，value=[温度值字符串](QVariant 存储)
		std::map<RenderPickResultData, QVector<QVariant>> _nodeTempMap;

		//文件定义相关
		QString _filePath;
		int _interpolateType = 0;
		bool _midNodeInterpolate = false;
	};
}