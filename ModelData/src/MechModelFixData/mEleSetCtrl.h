#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXModelFixData_global.h"
#include "mModelFixDataBase.h"
#include "mPhysicalFieldEnum.h"
#include "mModelEnum.h"

#include <QStringList>
#include <QList>
#include <memory>

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mElementFunction.h"
#include "mModelStruct.h"
namespace MechModelFixData
{
	class MXMODELFIXDATA_EXPORT mEleSetCtrl : public mModelFixDataBase
	{
	public:
		/*
		* 含参构造函数
		*/
		mEleSetCtrl(int id, const QString& name = "");

		/*
		* 默认构造函数，仅用于输入输出流
		*/
		mEleSetCtrl() {}

		/*
		* 析构函数
		*/
		~mEleSetCtrl();

		/*
		* 拷贝模型修复数据
		*/
		std::shared_ptr<mModelFixDataBase> copyModelFixData(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		/*
		* 获取模型修复类型
		*/
		QString getModelFixType() const override;

		/*
		* 激活或移除单元
		*/
		void setActiveOrRemove(bool activeOrRemove);
		bool isActiveOrRemove() const;

		/*
		* 选择集类型
		*/
		void setSelectSetType(const QString& type);
		QString getSelectSetType() const;

		friend QDataStream& operator>>(QDataStream& in, mEleSetCtrl& obj);
		friend QDataStream& InStreammEleSetCtrlV3(QDataStream& in, mEleSetCtrl& obj);
		friend QDataStream& operator<<(QDataStream& out, mEleSetCtrl& obj);

	public:
		//false-移除单元；true-激活单元
		bool _activeOrRemove = false;

		//选择集类型（作用对象由 Physical_ModelFix -> Physical_Set 映射关系维护）
		QString _selectSetType;
	};
}

