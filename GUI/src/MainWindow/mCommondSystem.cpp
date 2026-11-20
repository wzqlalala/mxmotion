#include "mCommondSystem.h"
#include "mCommonds.h"

namespace MechMainWindow
{
	mCommondSystem* mCommondSystem::_instance = nullptr;

	mCommondSystem* mCommondSystem::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mCommondSystem;
		}
		return _instance;
	}

	mCommondSystem::mCommondSystem():
		QObject(nullptr)
	{


	}

	mCommondSystem::~mCommondSystem()
	{
		resetStack();
	}

	void mCommondSystem::resetStack()
	{
		auto clearStack = [](std::deque<mCommondBase*>& stack) {
			while (!stack.empty()) {
				delete stack.front();
				stack.pop_front();
			}
			};

		clearStack(_undoStack);
		clearStack(_redoStack);
	}

	void mCommondSystem::addCommond(mCommondBase* commond)
	{
		_redoStack.push_front(commond);

		//压栈大小控制
		if (_redoStack.size() > MAX_STACK_SIZE)
		{
			delete _redoStack.back();
			_redoStack.pop_back();
		}
	}

	void mCommondSystem::undo()
	{
		if (_redoStack.empty())
		{
			return;
		}
		m_commondBase = _redoStack.front();
		_redoStack.pop_front();
		_undoStack.push_front(m_commondBase);
		m_commondBase->switchState();

		//压栈大小控制
		if (_undoStack.size() > MAX_STACK_SIZE)
		{
			delete _undoStack.back();
			_undoStack.pop_back();
		}
	}

	void mCommondSystem::redo()
	{
		if (_undoStack.empty())
		{
			return;
		}
		m_commondBase = _undoStack.front();
		_undoStack.pop_front();
		_redoStack.push_front(m_commondBase);
		m_commondBase->switchState();

		//压栈大小控制
		if (_redoStack.size() > MAX_STACK_SIZE)
		{
			delete _redoStack.back();
			_redoStack.pop_back();
		}
	}

	mCommondBase* mCommondSystem::getCommondBase()
	{
		return m_commondBase;
	}
}