#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"

//QT
#include <QTableWidget>

class QTimer;
class QLabel;
class QPushButton;
class QMdiSubWindow;

namespace MechMainWindow
{
	/*
	* 求解任务参数面板（树上"求解任务"节点点击后显示）
	* 对齐交互原型的参数页：一键求解/管理任务按钮 + 任务队列/已完成任务紧凑表格
	* - 一键求解：调用"提交计算"接口（与菜单提交计算一致）
	* - 管理任务：打开求解任务配置弹窗
	* - 查看结果：打开选中完成记录的结果目录
	* 面板展示当前模型的数据，随模型切换与执行中心状态实时刷新
	*/
	class MMAINWINDOW_EXPORT mSolveTaskPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		explicit mSolveTaskPanel(QWidget* parent = Q_NULLPTR);
		~mSolveTaskPanel();

		/*
		* 刷新面板（树节点点击/模型切换/运行状态变化时调用）
		*/
		void refreshPanel();

	private slots:
		void slot_quickSolve();
		void slot_manageTasks();
		void slot_queueMove(int delta);
		void slot_queueAction();
		void slot_viewResult();
		void slot_runStatusChanged(quint64 runID);
		void slot_modelChanged(int viewportID);
		void slot_updateElapsed();

	private:
		/*
		* 当前模型ID（无效返回-1）
		*/
		int currentModelID() const;

		/*
		* 刷新任务队列/已完成表格
		*/
		void refreshQueueTable();
		void refreshCompletedTable();

	private:
		//任务队列
		QTableWidget* _queueTable = nullptr;
		QLabel* _queueCountLabel = nullptr;
		QPushButton* _queueActionBtn = nullptr;
		quint64 _selectedQueueRunID = 0;

		//查看结果按钮（未选中完成记录时置灰）
		QPushButton* _viewResultBtn = nullptr;

		//已完成任务
		QTableWidget* _completedTable = nullptr;
		QLabel* _completedCountLabel = nullptr;
		quint64 _selectedCompletedRunID = 0;

		//运行中任务计时刷新
		QTimer* _elapsedTimer = nullptr;
	};
}
