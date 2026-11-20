#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXStructOptionData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QVector3D>

namespace MechStructOptionData
{	
	class MXSTRUCTOPTIONDATA_EXPORT mRunnerOptimizationConstraintData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mRunnerOptimizationConstraintData(int id, QString name = "");

		/*
		* 析构函数
		*/
		~mRunnerOptimizationConstraintData();

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
		* 拷贝
		*/
		mRunnerOptimizationConstraintData* copyData(const int& id, const QString& name);

		friend QDataStream& operator>>(QDataStream& in, mRunnerOptimizationConstraintData& mgr);
		friend QDataStream& InStreammRunnerOptimizationConstraintDataV3(QDataStream& in, mRunnerOptimizationConstraintData& obj);
		friend QDataStream& operator<<(QDataStream& out, mRunnerOptimizationConstraintData& mgr);

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

		// 约束类型(质量/质量分数/体积/体积分数)
		QString _constraintType;

		// 约束方式(固体/流体)
		QString _constraintMode;

		/******************************************指定区域**************************************************/
		//是否指定区域
		bool _isArea;
	};
}
