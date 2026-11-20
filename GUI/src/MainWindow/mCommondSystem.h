#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <unordered_map>
#include <iostream>
#include <functional>
#include <string>
#include <stack>
#include <deque>
#include <QObject>

namespace MechMainWindow
{
	class mCommondBase;
	class MMAINWINDOW_EXPORT mCommondSystem:public QObject
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针
		*/
		static mCommondSystem* getInstance();

		/*
		* 析构函数
		*/
		~mCommondSystem();

		/*
		* 重置栈
		*/
		void resetStack();

		/*
		* 添加命令对象
		*/
		void addCommond(mCommondBase* commond);

		/*
		* 撤销
		*/
		void undo();

		/*
		* 重做
		*/
		void redo();

		/*
		* 获取命令基类
		*/
		mCommondBase* getCommondBase();

	private:
		/*
		* 构造函数
		*/
		mCommondSystem();

	signals:
		void sg_materialCreateUndo();
		void sg_materialCreateRedo();

	private:
		//单例指针
		static mCommondSystem* _instance;

		//命令基类
		mCommondBase* m_commondBase = nullptr;

		//undo栈
		std::deque<mCommondBase*>_undoStack;

		//redo栈
		std::deque<mCommondBase*>_redoStack;

		//撤销重做层数
		static const int MAX_STACK_SIZE = 20;
	};
}
