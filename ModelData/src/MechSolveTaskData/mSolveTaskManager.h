#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXSolveTaskData_global.h"
#include "mSolveTaskData.h"

#include <QString>
#include <QVector>
#include <map>

namespace MechSolveTaskData
{
	/*
	* 求解任务管理器：每个模型持有一个（由mModelData聚合）
	* 负责任务的增删改查、ID分配与名称唯一性
	* 注意：只管理任务配置，不涉及求解执行（执行由GUI层求解中心负责）
	*/
	class MXSOLVETASKDATA_EXPORT mSolveTaskManager
	{
	public:
		mSolveTaskManager();
		~mSolveTaskManager();

		/*
		* 添加任务（接管指针所有权），返回分配的任务ID；名称重复返回-1（不接管指针）
		*/
		int addTask(mSolveTaskData* task);

		/*
		* 创建并添加任务（自动分配ID；名称为空或重复时自动生成唯一名称）
		*/
		mSolveTaskData* createTask(const QString& name = "");

		/*
		* 删除指定任务
		*/
		bool removeTask(int taskID);

		/*
		* 删除所有任务
		*/
		void removeAllTask();

		/*
		* 通过ID获取任务，不存在返回nullptr
		*/
		mSolveTaskData* getTask(int taskID) const;

		/*
		* 通过名称获取任务，不存在返回nullptr
		*/
		mSolveTaskData* getTaskByName(const QString& name) const;

		/*
		* 获取所有任务（按创建顺序）
		*/
		QVector<mSolveTaskData*> getAllTask() const;

		/*
		* 任务数量
		*/
		int getTaskCount() const { return (int)_tasks.size(); }

		/*
		* 判断任务名是否存在
		*/
		bool isExistName(const QString& name) const;

		/*
		* 重命名任务（校验唯一性），成功返回true
		*/
		bool renameTask(int taskID, const QString& newName);

		/*
		* 生成不重复的任务名（前缀 + 三位序号，如Task_001）
		*/
		QString createUniqueName(const QString& prefix = "Task") const;

	private:
		//下一个任务ID
		int _nextTaskID = 1;

		//任务表（taskID -> 任务，创建顺序）
		std::map<int, mSolveTaskData*> _tasks;
	};
}
