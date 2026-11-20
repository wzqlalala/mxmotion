#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mCaseBase.h"

#include "mModalContriData.h"
#include "mNodeContriData.h"
#include "mPanelContriData.h"

using namespace MechGlobalParamData;
namespace MechCaseData
{
	class MXCASEDATA_EXPORT mCaseImplASCoupling : public mCaseBase
	{

	public:
		/*
		* 重载构造函数
		*/
		mCaseImplASCoupling(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplASCoupling() {}

		/*
		* 析构函数
		*/
		~mCaseImplASCoupling();

		/*
		* 拷贝分析工况
		*/
		mCaseImplASCoupling* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplASCoupling& obj);
		friend QDataStream& InStreammCaseImplASCouplingV3(QDataStream& in, mCaseImplASCoupling& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplASCoupling& obj);
	public:
		/************************模态贡献量相关接口*******************************/
		void deleteModalContriData();
		mModalContriData* createModalContriData();
		mModalContriData* getModalContriData();

		/************************节点贡献量相关接口*******************************/
		void deleteNodeContriData();
		mNodeContriData* createNodeContriData();
		mNodeContriData* getNodeContriData();

		/************************面板贡献量相关接口*******************************/
		void deletePanelContriData();
		mPanelContriData* createPanelContriData();
		mPanelContriData* getPanelContriData();

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

		//结构模态参数
		// 最小频率等6个字段
		QString _structMinFrequency;
		QString _structMaxFrequency;
		QString _structOrder;
		QString _structTolerance;
		QString _structAmpffact;
		QString _structNormalization;

		//声学模态参数
		//最小频率等6个字段
		QString _acousticsMinFrequency;
		QString _acousticsMaxFrequency;
		QString _acousticsOrder;
		QString _acousticsTolerance;
		QString _acousticsAmpffact;
		QString _acousticsNormalization;

		/*阻尼(可共用)*/
		//整体结构阻尼
		QString _dampingG;

		//瑞利阻尼1
		QString _dampingAlpha1;

		//瑞丽阻尼2
		QString _dampingAlpha2;

		//阻尼参数
		//结构阻尼参数
		//整体结构阻尼等3个参数
		QString _structDampingG;
		QString _structDampingAlpha1;
		QString _structDampingAlpha2;

		//声学阻尼参数
		QString _acousticsDampingG;
		QString _acousticsDampingAlpha1;
		QString _acousticsDampingAlpha2;

		//采样频率点勾选情况
		bool checked; //true 勾选 false 未勾选

		//模态贡献量
		mModalContriData* _modalContriData = nullptr;
		//节点贡献量
		mNodeContriData* _mNodeContriData = nullptr;

		//面板贡献量
		mPanelContriData* _mPanelContriData = nullptr;
	};
}
