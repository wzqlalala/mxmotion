#include "mSolveTaskManager.h"

#include <QDateTime>

namespace MechSolveTaskData
{
	mSolveTaskManager::mSolveTaskManager()
	{
	}

	mSolveTaskManager::~mSolveTaskManager()
	{
		removeAllTask();
	}

	int mSolveTaskManager::addTask(mSolveTaskData* task)
	{
		if (task == nullptr)
		{
			return -1;
		}
		//名称唯一性校验，重复时不接管指针
		if (isExistName(task->getTaskName()))
		{
			return -1;
		}
		task->setTaskID(_nextTaskID);
		if (task->getCreateTime() == 0)
		{
			task->setCreateTime(QDateTime::currentMSecsSinceEpoch());
		}
		_tasks[_nextTaskID] = task;
		return _nextTaskID++;
	}

	mSolveTaskData* mSolveTaskManager::createTask(const QString& name)
	{
		mSolveTaskData* task = new mSolveTaskData;
		//名称为空或重复时自动生成唯一名称
		if (name.isEmpty() || isExistName(name))
		{
			task->setTaskName(createUniqueName("Task"));
		}
		else
		{
			task->setTaskName(name);
		}
		if (addTask(task) == -1)
		{
			delete task;
			return nullptr;
		}
		return task;
	}

	bool mSolveTaskManager::removeTask(int taskID)
	{
		auto iter = _tasks.find(taskID);
		if (iter == _tasks.end())
		{
			return false;
		}
		delete iter->second;
		_tasks.erase(iter);
		return true;
	}

	void mSolveTaskManager::removeAllTask()
	{
		for (auto& pair : _tasks)
		{
			delete pair.second;
		}
		_tasks.clear();
	}

	mSolveTaskData* mSolveTaskManager::getTask(int taskID) const
	{
		auto iter = _tasks.find(taskID);
		return iter != _tasks.end() ? iter->second : nullptr;
	}

	mSolveTaskData* mSolveTaskManager::getTaskByName(const QString& name) const
	{
		for (auto& pair : _tasks)
		{
			if (pair.second->getTaskName() == name)
			{
				return pair.second;
			}
		}
		return nullptr;
	}

	QVector<mSolveTaskData*> mSolveTaskManager::getAllTask() const
	{
		QVector<mSolveTaskData*> tasks;
		for (auto& pair : _tasks)
		{
			tasks.push_back(pair.second);
		}
		return tasks;
	}

	bool mSolveTaskManager::isExistName(const QString& name) const
	{
		return getTaskByName(name) != nullptr;
	}

	bool mSolveTaskManager::renameTask(int taskID, const QString& newName)
	{
		mSolveTaskData* task = getTask(taskID);
		if (task == nullptr || newName.isEmpty())
		{
			return false;
		}
		//与其他任务重名时拒绝
		mSolveTaskData* exist = getTaskByName(newName);
		if (exist != nullptr && exist != task)
		{
			return false;
		}
		task->setTaskName(newName);
		return true;
	}

	QString mSolveTaskManager::createUniqueName(const QString& prefix) const
	{
		QString name;
		int index = 1;
		do
		{
			name = QString("%1_%2").arg(prefix).arg(index, 3, 10, QChar('0'));
			index++;
		} while (isExistName(name));
		return name;
	}
}
