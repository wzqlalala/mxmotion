#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXStructOptionData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QVector3D>
#include <QVector>
#include <QList>
#include <unordered_map>
#include "mModelStruct.h"
#include "mBasicEnum.h"
#include "mModelEnum.h"

namespace MechStructOptionData
{	
	class MXSTRUCTOPTIONDATA_EXPORT mOptimizationConstraintData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mOptimizationConstraintData(int id, QString name = "");

		/*
		* 析构函数
		*/
		~mOptimizationConstraintData();

		/*
		* 设置ID
		*/
		void setId(int id);

		/*
		* 获取ID
		*/
		int getId();

		/*
		* 设置名
		*/
		void setName(const QString& name);

		/*
		* 获取名
		*/
		QString getName();

		/*
		 * 设置描述
		 */
		void setDescribe(const QString& describe);

		/*
		 * 获取描述
		 */
		QString getDescribe();

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		/*
		* 拷贝
		*/
		mOptimizationConstraintData* copyData(const int& id, const QString& name);

		friend QDataStream& operator>>(QDataStream& in, mOptimizationConstraintData& mgr);
		friend QDataStream& InStreammOptimizationConstraintDataV3(QDataStream& in, mOptimizationConstraintData& obj);
		friend QDataStream& operator<<(QDataStream& out, mOptimizationConstraintData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

		//描述
		QString _describe;


	public:

		/******************************************优化约束**************************************************/

		// 约束上限
		bool _upperBoundStatus;
		QString _upperBoundValue;

		// 约束下限
		bool _lowerBoundStatus;
		QString _lowerBoundValue;

		// 约束类型(质量/质量分数/体积/体积分数/柔度/位移/频率/应力/综合柔度/综合频率/柔度指数/热柔度/节点温度/PSD位移/频响位移)
		QString _constraintType;

		/******************************************指定区域**************************************************/
		//是否指定区域
		bool _isArea;

		/******************************************响应类型参数(与优化目标一致，约束类型为响应类型时使用)**************************************************/
		//频率响应-目标阶数
		QString _frequencyOrder;

		//柔度指数
		bool _isCompliance;
		bool _isFrequency;

		//综合柔度响应
		std::unordered_map<int, double> _caseToweight;

		//综合频率响应
		QList<QPair<double, double>> _order;

		//位移响应
		//节点编号
		std::pair<MxFunctions::PickDataType, RenderPickResultData> _node;

		//自由度(1/2/3/4/5/6/disp/rot)
		QString _nodeDofs;

		//PSD位移-频响位移响应
		//目标频率类型：指定频率点/最大响应点/最小响应点/平均/求和/RMS
		QString _frequencyTargetType;

		//指定频率点（仅在目标频率类型为“指定频率点”时有效）
		QString _specifiedFrequency;

		//频响位移优化类型：幅值/实部/虚部
		QString _frfValueType;
	};
}