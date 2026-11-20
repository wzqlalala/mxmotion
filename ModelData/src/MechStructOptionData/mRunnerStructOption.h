#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXStructOptionData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>

#include "mRunnerOptimizationGoalData.h"

namespace MechStructOptionData
{	
	struct mRunnerOptionData
	{
		bool	 _isMaxiter = false;
		QString  _maxiter;        // 最大迭代步

		bool	 _isObjectivetolrance = false;
		QString  _objectivetolrance;      // 目标函数容差

		bool	 _isDiscretetarget = false;
		QString  _discretetarget;       //收敛离散指数

		bool	 _isTopomovelimit = false;
		QString  _topomovelimit;         // 优化步长

		bool	 _isRmin = false;
		QString  _rmin;    // 过滤半径

		bool	 _isPenal = false;
		QString  _penal;       // 惩罚参数

		/******************************************流体属性**************************************************/
		QString  _density;          // 密度
		QString  _dynamicViscosity; // 动力粘度

		friend QDataStream& operator>>(QDataStream& in, mRunnerOptionData& mgr);
		friend QDataStream& InStreammRunnerOptionDataV3(QDataStream& in, mRunnerOptionData& obj);
		friend QDataStream& operator<<(QDataStream& out, mRunnerOptionData& mgr);
	};


	class mRunnerOptimizationDesignData;
	class mRunnerOptimizationGoalData;
	class mRunnerOptimizationConstraintData;
	class MXSTRUCTOPTIONDATA_EXPORT mRunnerStructOption
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mRunnerStructOption();

		/*
		* 析构函数
		*/
		~mRunnerStructOption();

		/************************设计域相关接口开始******************************/
		mRunnerOptimizationDesignData* getOptimizationDesignData(int id);
		mRunnerOptimizationDesignData* getOptimizationDesignData(const QString& name) const;
		QString getOptimizationDesignNameByID(int id) const;
		int getOptimizationDesignIDByName(const QString& name) const;
		void addOptimizationDesignData(mRunnerOptimizationDesignData* body);
		bool existOptimizationDesignName(const QString& name) const;
		void resetOptimizationDesignName(const QString& oldName, const QString& newName);
		int copyOptimizationDesignData(const int& id);
		void deleteOptimizationDesignData(int id);
		void deleteAllOptimizationDesignData();
		std::vector<int> getAllOptimizationDesignIDs() const;
		QStringList getAllOptimizationDesignNames() const;
		QStringList getAllOptimizationMasterDesignNames() const;
		int createOptimizationDesignID();
		/************************设计域相关接口结束******************************/

		/************************优化约束相关接口开始******************************/
		mRunnerOptimizationConstraintData* getOptimizationConstraintData(int id);
		mRunnerOptimizationConstraintData* getOptimizationConstraintData(const QString& name) const;
		QString getOptimizationConstraintNameByID(int id) const;
		int getOptimizationConstraintIDByName(const QString& name) const;
		void addOptimizationConstraintData(mRunnerOptimizationConstraintData* body);
		bool existOptimizationConstraintName(const QString& name) const;
		void resetOptimizationConstraintName(const QString& oldName, const QString& newName);
		int copyOptimizationConstraintData(const int& id);
		void deleteOptimizationConstraintData(int id);
		void deleteAllOptimizationConstraintData();
		std::vector<int> getAllOptimizationConstraintIDs() const;
		QStringList getAllOptimizationConstraintNames() const;
		int createOptimizationConstraintID();
		/************************优化约束相关接口结束******************************/


		friend QDataStream& operator>>(QDataStream& in, mRunnerStructOption& mgr);
		friend QDataStream& InStreammRunnerStructOptionV3(QDataStream& in, mRunnerStructOption& mgr);
		friend QDataStream& operator<<(QDataStream& out, mRunnerStructOption& mgr);

	public:
		mRunnerOptionData _mRunnerOptionData;

		int _maxOptimizationDesignID;
		std::map<int, mRunnerOptimizationDesignData*> _designData;

		int _maxOptimizationConstraintID;
		std::map<int, mRunnerOptimizationConstraintData*> _constraintData;

		mRunnerOptimizationGoalData *_goalData;
	};
}
