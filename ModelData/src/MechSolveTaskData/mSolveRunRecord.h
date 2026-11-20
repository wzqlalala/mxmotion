#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSolveTaskData_global.h"
#include "mSolveTaskData.h"

#include <QString>
#include <QVector>
#include <QMap>

namespace MechSolveTaskData
{
	/*
	* 求解运行记录：只持有执行状态和运行时数据；
	* 配置信息（工况/核数/内存策略等）通过_task指针委托读取，
	* 生命周期：task先于run存在，task删除时其run一并删除。
	*/
	class MXSOLVETASKDATA_EXPORT mSolveRunRecord
	{
	public:
		/*
		* 执行状态
		*/
		enum RunStatus
		{
			Status_Queued = 0,		//排队中
			Status_Running = 1,		//求解中
			Status_Done = 2,		//已完成
			Status_Failed = 3,		//失败（进程异常退出或结果导入失败）
			Status_Interrupted = 4,	//已中断（求解中被用户终止）
			Status_Cancelled = 5	//已取消（排队中被用户移出）
		};

		explicit mSolveRunRecord(mSolveTaskData* task = nullptr);
		~mSolveRunRecord();

		/*
		* 关联的任务（配置委托源，task先于run存在）
		*/
		mSolveTaskData* getTask() const { return _task; }
		void setTask(mSolveTaskData* task) { _task = task; }

		/*
		* 配置委托读取（内部直接查task，不拷贝）
		*/
		int getTaskID() const { return _task ? _task->getTaskID() : -1; }
		QString getTaskName() const { return _task ? _task->getTaskName() : QString(); }
		QVector<int> getCaseIDs() const { return _task ? _task->getCaseIDs() : QVector<int>{}; }
		CaseRelation getCaseRelation(int caseID) const { return _task ? _task->getCaseRelation(caseID) : Relation_Independent; }
		int getCpuCores() const { return _task ? _task->getCpuCores() : 0; }
		MemoryStrategy getMemoryStrategy() const { return _task ? _task->getMemoryStrategy() : Memory_Adaptive; }
		QString getSolverFile() const { return _task ? _task->getSolverFile() : QString(); }
		bool isAutoOpenPost() const { return _task ? _task->isAutoOpenPost() : false; }

		/*
		* runID（全局唯一，跨模型，由执行中心分配）
		*/
		quint64 getRunID() const { return _runID; }
		void setRunID(quint64 id) { _runID = id; }

		/*
		* 所属模型ID
		*/
		int getModelID() const { return _modelID; }
		void setModelID(int id) { _modelID = id; }

		/*
		* 模型目录段（会话内标识，用于日志前缀和消息）
		*/
		QString getModelName() const { return _modelName; }
		void setModelName(const QString& name) { _modelName = name; }

		/*
		* 计算目录与日志文件路径（提交时由任务配置算出，定死）
		*/
		QString getCalDir() const { return _calDir; }
		void setCalDir(const QString& dir) { _calDir = dir; }
		QString getLogFile() const { return _logFile; }
		void setLogFile(const QString& file) { _logFile = file; }

		/*
		* 状态与时间（epoch毫秒）
		*/
		RunStatus getStatus() const { return _status; }
		void setStatus(RunStatus status) { _status = status; }
		qint64 getSubmittedAt() const { return _submittedAt; }
		void setSubmittedAt(qint64 time) { _submittedAt = time; }
		qint64 getStartedAt() const { return _startedAt; }
		void setStartedAt(qint64 time) { _startedAt = time; }
		qint64 getFinishedAt() const { return _finishedAt; }
		void setFinishedAt(qint64 time) { _finishedAt = time; }

		/*
		* 求解器进程退出码（未结束为-1）
		*/
		int getExitCode() const { return _exitCode; }
		void setExitCode(int code) { _exitCode = code; }

		/*
		* 是否已结束（Done/Failed/Interrupted/Cancelled）
		*/
		bool isFinished() const;

	private:
		//关联的任务（配置委托源）
		mSolveTaskData* _task = nullptr;

		//runID
		quint64 _runID = 0;

		//模型ID
		int _modelID = -1;

		//模型目录段
		QString _modelName;

		//计算目录/日志文件
		QString _calDir;
		QString _logFile;

		//状态与时间
		RunStatus _status = Status_Queued;
		qint64 _submittedAt = 0;
		qint64 _startedAt = 0;
		qint64 _finishedAt = 0;

		//进程退出码
		int _exitCode = -1;
	};
}
