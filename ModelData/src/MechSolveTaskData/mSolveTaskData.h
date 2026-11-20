#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSolveTaskData_global.h"

#include <QString>
#include <QVector>
#include <QMap>

namespace MechSolveTaskData
{
	/*
	* 内存策略
	*/
	enum MemoryStrategy
	{
		Memory_InCore = 0,		//核内
		Memory_OutCore = 1,		//核外
		Memory_Adaptive = 2		//自适应
	};

	/*
	* 工况间接续关系
	*/
	enum CaseRelation
	{
		Relation_Independent = 0,	//独立计算
		Relation_Continue = 1		//接续上一步
	};

	/*
	* 求解任务配置数据（前处理数据，随模型保存）
	* 一个任务 = 一组有序工况 + 接续关系 + 求解资源参数 + 输出位置
	*/
	class MXSOLVETASKDATA_EXPORT mSolveTaskData
	{
	public:
		mSolveTaskData();
		~mSolveTaskData();

		/*
		* 任务ID（模型内唯一，由mSolveTaskManager分配）
		*/
		int getTaskID() const { return _taskID; }
		void setTaskID(int id) { _taskID = id; }

		/*
		* 任务名称（模型内唯一）
		*/
		QString getTaskName() const { return _taskName; }
		void setTaskName(const QString& name) { _taskName = name; }

		/*
		* 有序工况ID列表（即任务内的求解顺序）
		*/
		QVector<int> getCaseIDs() const { return _caseIDs; }
		void setCaseIDs(const QVector<int>& ids) { _caseIDs = ids; }

		/*
		* 工况接续关系（未设置的工况默认独立计算，首个工况固定独立计算）
		*/
		CaseRelation getCaseRelation(int caseID) const;
		void setCaseRelation(int caseID, CaseRelation relation);

		/*
		* 求解核数
		*/
		int getCpuCores() const { return _cpuCores; }
		void setCpuCores(int cores) { _cpuCores = cores; }

		/*
		* 内存策略
		*/
		MemoryStrategy getMemoryStrategy() const { return _memoryStrategy; }
		void setMemoryStrategy(MemoryStrategy strategy) { _memoryStrategy = strategy; }

		/*
		* 输出目录（空表示使用默认：工作目录/模型名/任务名）
		*/
		QString getOutputDir() const { return _outputDir; }
		void setOutputDir(const QString& dir) { _outputDir = dir; }

		/*
		* 实际写出的计算文件路径（由"写出文件"动作更新，提交求解前必须已写出）
		*/
		QString getSolverFile() const { return _solverFile; }
		void setSolverFile(const QString& file) { _solverFile = file; }

		/*
		* 求解完成后自动打开后处理
		*/
		bool isAutoOpenPost() const { return _autoOpenPost; }
		void setAutoOpenPost(bool autoOpen) { _autoOpenPost = autoOpen; }

		/*
		* 任务创建时间（epoch毫秒）
		*/
		qint64 getCreateTime() const { return _createTime; }
		void setCreateTime(qint64 time) { _createTime = time; }

	private:
		//任务ID
		int _taskID = -1;

		//任务名称
		QString _taskName;

		//有序工况ID列表
		QVector<int> _caseIDs;

		//工况接续关系（caseID -> CaseRelation）
		QMap<int, int> _caseRelations;

		//求解核数
		int _cpuCores = 8;

		//内存策略
		MemoryStrategy _memoryStrategy = Memory_Adaptive;

		//输出目录
		QString _outputDir;

		//计算文件路径
		QString _solverFile;

		//自动打开后处理
		bool _autoOpenPost = false;

		//创建时间
		qint64 _createTime = 0;
	};
}
