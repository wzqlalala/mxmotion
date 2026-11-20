#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>

#include "mCaseBase.h"

namespace MechCaseData
{
	struct ResponseSpectrum_Data
	{
		//激励的放大系数(可以为空)
		QString _scales;

		//加载方向X(可以为空)
		QString _loadx;

		//加载方向Y(可以为空)
		QString _loady;

		//加载方向Z(可以为空)
		QString _loadz;
	};

	inline QDataStream& operator>>(QDataStream& in, ResponseSpectrum_Data& obj)
	{
		in >> obj._scales >> obj._loadx >> obj._loady >> obj._loadz;
		return in;
	}

	inline QDataStream& operator<<(QDataStream& out, const ResponseSpectrum_Data& obj)
	{
		out << obj._scales << obj._loadx << obj._loady << obj._loadz;
		return out;
	}
	//响应谱工况
	class MXCASEDATA_EXPORT mCaseImplicitResponseSpectrum : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitResponseSpectrum(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitResponseSpectrum() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitResponseSpectrum();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitResponseSpectrum* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitResponseSpectrum& obj);
		friend QDataStream& InStreammCaseImplicitResponseSpectrumV3(QDataStream& in, mCaseImplicitResponseSpectrum& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitResponseSpectrum& obj);

	public:

		/*模态参数(模态法)*/
		//模态求解器，存储的字符串为自动多重子结构/兰索斯（只有模态分析可用）,下拉框，写文件时要按照关键字文档写,下拉框
		QString _modalSolver;

		//最小频率（可以为空）
		QString _minFrequency;

		//最大频率（可以为空）
		QString _maxFrequency;

		//阶数（可以为空）
		QString _order;

		//容差（可以为空）
		QString _tolerance;

		//放大系数（可以为空）,兰索斯不可用放大系数
		QString _ampffact;

		//特征向量,下拉框(无/广义质量/最大位移)，自动多重子结构不可用特征向量
		QString _normalization;

		/*阻尼(可共用)*/
		//整体结构阻尼
		QString _dampingG;

		//瑞利阻尼1
		QString _dampingAlpha1;

		//瑞丽阻尼2
		QString _dampingAlpha2;

		/*响应谱分析参数*/

		//方向组合方法，下拉框（无/ALG/SRSS）
		QString _dirction;

		//模态组合方法，下拉框（无/ABS/SRSS/CQC/NRL/DSUM/ROSE）
		QString _modal;

		//模态频率接近系数(不可以为空)
		QString _modalFrequencyClose;

		//载荷谱名称(不可以为空)
		//表格存储容器
		QList<ResponseSpectrum_Data> _tableData;


	};
}
