#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXStructOptionData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QVector>
#include <QMap>  
#include <QHash> 
#include "mModelStruct.h"
#include "mBasicEnum.h"
#include "mModelEnum.h"

namespace MechStructOptionData
{	
	class MXSTRUCTOPTIONDATA_EXPORT mOptimizationGoalData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mOptimizationGoalData();

		/*
		* 析构函数
		*/
		~mOptimizationGoalData();

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);


		friend QDataStream& operator>>(QDataStream& in, mOptimizationGoalData& mgr);
		friend QDataStream& InStreammOptimizationGoalDataV3(QDataStream& in, mOptimizationGoalData& obj);
		friend QDataStream& operator<<(QDataStream& out, mOptimizationGoalData& mgr);


	public:

		//优化方式(最小化/最大化)，下拉框
		QString _objectiveType;

		//优化响应（柔度/位移/频率/应力/综合柔度/综合频率/柔度指数/热柔度/节点温度），下拉框
		QString _responseType;
		/******************************************柔度响应对应的参数**************************************************/
		//无

		/******************************************频率响应对应的参数**************************************************/
		QString _frequencyOrder;// 目标阶数




		/************************************************柔度指数参数开始包含**************************************************/
		bool _isCompliance;  
		bool _isFrequency;

		/******************************************综合柔度响应对应的参数**************************************************/
		std::unordered_map<int, double> _caseToweight;

		/******************************************综合频率响应对应的参数**************************************************/
		QList<QPair<double,double>> _order;

		/************************************************柔度指数参数结束包含**************************************************/




		/******************************************位移响应对应的参数**************************************************/
		//节点编号
		std::pair<MxFunctions::PickDataType, RenderPickResultData> _node;

		//自由度(1/2/3/4/5/6/disp/rot)
		QString _nodeDofs;

		/******************************************PSD位移-频响位移响应对应的参数**************************************************/
		//目标频率类型：指定频率点/最大响应点/最小响应点/平均/求和/RMS
		QString _frequencyTargetType;

		//指定频率点（仅在目标频率类型为“指定频率点”时有效）
		QString _specifiedFrequency;

		//频响位移优化类型：幅值/实部/虚部
		QString _frfValueType;
		
	};
}