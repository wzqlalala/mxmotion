#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXStructOptionData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>

#include "mOptimizationGoalData.h"

namespace MechStructOptionData
{	
	struct mStructOptionData
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
		QString  _rmin;    // 最小尺寸

		bool	 _isPenal = false;
		QString  _penal;       // 惩罚参数

		bool	 _isInitdensity = false;
		QString  _initdensity;       // 初始材料分数

		bool	 _isMinDesity = false;
		QString  _minDesity;       // 最小密度值

		bool	 _isChecker = false;
		QString  _checker;       // 过滤

		bool	 _isMatintertype = false;
		QString  _matintertype = "SIMP";       // 优化算法

		bool	 _isRebuild = false;
		QString  _rebuild;       // 模型重构

		bool _isRestart = false;   //重启动
		QString _iteration;  
		QString _resStartPath;

		friend QDataStream& operator>>(QDataStream& in, mStructOptionData& mgr);
		friend QDataStream& InStreammStructOptionDataV3(QDataStream& in, mStructOptionData& obj);
		friend QDataStream& operator<<(QDataStream& out, mStructOptionData& mgr);
	};


	class mOptimizationDesignData;
	class mOptimizationGoalData;
	class mOptimizationConstraintData;
	class MXSTRUCTOPTIONDATA_EXPORT mStructOption
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mStructOption();

		/*
		* 析构函数
		*/
		~mStructOption();

		/************************设计域相关接口开始******************************/
		mOptimizationDesignData* getOptimizationDesignData(int id);
		mOptimizationDesignData* getOptimizationDesignData(const QString& name) const;
		QString getOptimizationDesignNameByID(int id) const;
		int getOptimizationDesignIDByName(const QString& name) const;
		void addOptimizationDesignData(mOptimizationDesignData* body);
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
		mOptimizationConstraintData* getOptimizationConstraintData(int id);
		mOptimizationConstraintData* getOptimizationConstraintData(const QString& name) const;
		QString getOptimizationConstraintNameByID(int id) const;
		int getOptimizationConstraintIDByName(const QString& name) const;
		void addOptimizationConstraintData(mOptimizationConstraintData* body);
		bool existOptimizationConstraintName(const QString& name) const;
		void resetOptimizationConstraintName(const QString& oldName, const QString& newName);
		int copyOptimizationConstraintData(const int& id);
		void deleteOptimizationConstraintData(int id);
		void deleteAllOptimizationConstraintData();
		std::vector<int> getAllOptimizationConstraintIDs() const;
		QStringList getAllOptimizationConstraintNames() const;
		int createOptimizationConstraintID();
		/************************优化约束相关接口结束******************************/


		friend QDataStream& operator>>(QDataStream& in, mStructOption& mgr);
		friend QDataStream& InStreammStructOptionV3(QDataStream& in, mStructOption& mgr);
		friend QDataStream& operator<<(QDataStream& out, mStructOption& mgr);

	public:
		mStructOptionData _mStructOptionData;

		int _maxOptimizationDesignID;
		std::map<int, mOptimizationDesignData*> _designData;

		int _maxOptimizationConstraintID;
		std::map<int, mOptimizationConstraintData*> _constraintData;

		mOptimizationGoalData *_goalData;
	};
}