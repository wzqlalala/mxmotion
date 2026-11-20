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
#include <QDataStream>
#include "mUnitSystemManager.h"

using namespace CommonData;
namespace MechCaseData
{
	class MXCASEDATA_EXPORT mRoatateAnalyseBase
	{
	friend QDataStream& InStreammRoatateAnalyseBaseV3(QDataStream& in, mRoatateAnalyseBase& obj);
	public:
		/*
		* 重载构造函数
		*/
		mRoatateAnalyseBase(int id, MxFunctions::RotateAnalysyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mRoatateAnalyseBase() {}

		/*
		* 析构函数
		*/
		~mRoatateAnalyseBase();


		/*
		* 设置旋转分析ID
		*/
		void setRotateAnalyseId(int id);

		/*
		* 获取旋转分析ID
		*/
		int getRotateAnalyseId();

		/*
		* 设置旋转分析名
		*/
		void setRotateAnalyseName(const QString& name);

		/*
		* 获取旋转分析名
		*/
		QString getRotateAnalyseName();

		/*
		* 设置旋转分析类型
		*/
		void setRotateAnalyseType(MxFunctions::RotateAnalysyType type);

		/*
		* 获取旋转分析类型
		*/
		MxFunctions::RotateAnalysyType getRotateAnalyseType();

		/*
		* 拷贝旋转分析
		*/
		virtual std::shared_ptr<mRoatateAnalyseBase> copyRotateAnalyse(const int& id, const QString& name) = 0;

		/*
		* 判断数据完整性
		*/
		virtual MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) { return MxFunctions::BrowserTreeStatus::Tree_Yes; };

		/*
		* 响应单位制变化
		*/
		virtual void responseUnitChanged(const int& unitidx, mUnitSystemManager* unitsystemmanager) {};

		friend MXCASEDATA_EXPORT QDataStream& operator>>(QDataStream& in, mRoatateAnalyseBase& obj);
		friend MXCASEDATA_EXPORT QDataStream& operator<<(QDataStream& out, mRoatateAnalyseBase& obj);

	private:
		//旋转分析ID
		int _id;

		//旋转分析名字
		QString _rotateAnalyseName;

		//旋转分析类型
		MxFunctions::RotateAnalysyType _rotateAnalyseType;
	};
}
