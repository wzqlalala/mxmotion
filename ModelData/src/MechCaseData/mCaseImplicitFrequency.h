#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "MXGlobalParamData_global.h"

#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mCaseBase.h"

#include "mModalContriData.h"

using namespace MechGlobalParamData;
namespace MechCaseData
{
	class MXCASEDATA_EXPORT mCaseImplicitFrequency : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitFrequency(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitFrequency() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitFrequency();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitFrequency* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitFrequency& obj);
		friend QDataStream& InStreammCaseImplicitFrequencyV3(QDataStream& in, mCaseImplicitFrequency& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitFrequency& obj);

	public:
		/************************模态贡献量相关接口开始*******************************/
		void deleteModalContriData();
		mModalContriData* createModalContriData();
		mModalContriData* getModalContriData();

	public:
		//方法（模态法/直接法）下拉框
		QString _method;

		/*求解控制(模态法)*/
		//残余矢量分析
		bool _isRemnantOpen;

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

		//采样频率点勾选情况
		bool checked; //true 勾选 false 未勾选

		//模态贡献量
		mModalContriData* _modalContriData = nullptr;
	};
}
