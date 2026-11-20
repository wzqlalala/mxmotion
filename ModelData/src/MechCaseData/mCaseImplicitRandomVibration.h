#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QList>
#include <QDataStream>

#include "mCaseBase.h"

namespace MechCaseData
{
	struct RandomCase_RandPs {
		QString _x;
		QString _y;

	friend QDataStream& operator>>(QDataStream& in, RandomCase_RandPs& obj);
	friend QDataStream& operator<<(QDataStream& out, const RandomCase_RandPs& obj);
	};
	
	struct RandomCase_OutPutType {
		QString _outputType;
		QString _randomType;
		QString _nodeNum;
		QString _dofNum;

	friend QDataStream& operator>>(QDataStream& in, RandomCase_OutPutType& obj);
	friend QDataStream& operator<<(QDataStream& out, const RandomCase_OutPutType& obj);
	};

	inline QDataStream& operator>>(QDataStream& in, RandomCase_RandPs& obj)
	{
		in >> obj._x >> obj._y;
		return in;
	}

	inline QDataStream& operator<<(QDataStream& out, const RandomCase_RandPs& obj)
	{
		out << obj._x << obj._y;
		return out;
	}

	inline QDataStream& operator>>(QDataStream& in, RandomCase_OutPutType& obj)
	{
		in >> obj._dofNum >> obj._nodeNum >> obj._outputType >> obj._randomType;
		return in;
	}

	inline QDataStream& operator<<(QDataStream& out, const RandomCase_OutPutType& obj)
	{
		out << obj._dofNum << obj._nodeNum << obj._outputType << obj._randomType;
		return out;
	}

	//随机振动工况
	class MXCASEDATA_EXPORT mCaseImplicitRandomVibration : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitRandomVibration(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitRandomVibration() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitRandomVibration();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitRandomVibration* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitRandomVibration& obj);
		friend QDataStream& InStreammCaseImplicitRandomVibrationV3(QDataStream& in, mCaseImplicitRandomVibration& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitRandomVibration& obj);

	public:
		/* 自相关函数参数 */
		//起始时间
		QString _time0;

		//最大时间
		QString _timeMax;

		//时间间隔数
		QString _nTime;

		//表格存储容器
		QList<RandomCase_RandPs> _tableXYData;

		QList<RandomCase_OutPutType> m_outputData;

	};
}
