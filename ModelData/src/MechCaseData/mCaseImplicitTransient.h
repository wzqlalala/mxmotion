#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mCaseBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mCaseImplicitTransient : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitTransient(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitTransient() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitTransient();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitTransient* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitTransient& obj);
		friend QDataStream& InStreammCaseImplicitTransientV3(QDataStream& in, mCaseImplicitTransient& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitTransient& obj);

	public:
		//方法（模态法/直接法）下拉框
		QString _method;

		/*求解控制(直接法)*/
		//求解方法 (瞬态newmark法/瞬态hht法/瞬态wilson法)下拉框
		QString _solveMethod;

		/*求解控制(模态法)*/
		//残余矢量分析
		bool _isRemnantOpen;

		/*时间步(可共用)*/
		//时间步数
		QString _timestepCount;
		//时间步长
		QString _timestepVal;

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

		//全局结构阻尼
		QString _dampingOmg3;

		//瑞利阻尼1
		QString _dampingAlpha1;

		//瑞丽阻尼2
		QString _dampingAlpha2;

	};
}
