#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QDialog>
#include <QHash>

class QListWidget;
class QTableWidget;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QTabWidget;
class QPlainTextEdit;
class QTimer;
class QSplitter;

#include <QStyledItemDelegate>

namespace MechMainWindow
{
	/*
	* 去悬停delegate：绘制前剥离State_MouseOver，原生样式的悬停高亮不再出现
	*/
	class NoHoverItemDelegate : public QStyledItemDelegate
	{
		Q_OBJECT

	public:
		using QStyledItemDelegate::QStyledItemDelegate;

	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	};
	/*
	* 按钮按下态跟踪mixin（delegate自绘按钮的点击反馈）
	* 用法：delegate私有继承或组合本类，paint时调isPressed、editorEvent时调handlePress/handleRelease
	*/
	class ButtonPressTracker
	{
	public:
		//按下的按钮编号（0=左按钮，1=右按钮），-1=无按下
		int pressedButton() const { return _pressedBtn; }
		int pressedRow() const { return _pressedRow; }
		bool isPressed(int row, int btn) const { return _pressedRow == row && _pressedBtn == btn; }

		void handlePress(int row, int btn) { _pressedRow = row; _pressedBtn = btn; }
		void handleRelease() { _pressedRow = -1; _pressedBtn = -1; }

	private:
		int _pressedRow = -1;
		int _pressedBtn = -1;
	};

    /*
    * 任务列表"操作"列（设置/结果）自绘delegate（替代cellWidget）
    * 数据约定：列item的 UserRole+1 存可查看的结果runID（0=无结果，结果按钮置灰）
    */
	class TaskActionDelegate : public QStyledItemDelegate
	{
		Q_OBJECT

	public:
		using QStyledItemDelegate::QStyledItemDelegate;

		static QRect setButtonRect(const QRect& cellRect)
		{
			const int top = (cellRect.height() - 20) / 2;
			return QRect(cellRect.left() + 2, cellRect.top() + top, 44, 20);
		}
		static QRect resultButtonRect(const QRect& cellRect)
		{
			return setButtonRect(cellRect).translated(48, 0);
		}

		//按下态查询（paint用）
		bool isSetPressed(int row) const { return _press.isPressed(row, 0); }
		bool isResultPressed(int row) const { return _press.isPressed(row, 1); }

	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

	signals:
		void setClicked(int row);
		void resultClicked(int row);

	private:
		ButtonPressTracker _press;
	};
	/*
	* 工况列表"添加"按钮的自绘delegate（替代cellWidget，绘制几何完全确定）
	* 数据约定：列item的 UserRole+1 存"是否已添加"；按钮铺满单元格（上下留7px）
	*/
	class CaseAddButtonDelegate : public QStyledItemDelegate
	{
		Q_OBJECT

	public:
		using QStyledItemDelegate::QStyledItemDelegate;

	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

	signals:
		void addClicked(int row);
	};

	/*
	* 任务工况表"工况关系"列的自绘delegate（替代cellWidget中的QComboBox，
	*/
	class RelationComboDelegate : public QStyledItemDelegate
	{
		Q_OBJECT

	public:
		explicit RelationComboDelegate(QTableWidget* table, QObject* parent = nullptr);

	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

	private:
		//宿主表格（用于点击时计算弹出菜单位置）
		QTableWidget* _table = nullptr;
	};
    /*
    * 已完成任务列表"操作"列（设置/结果）自绘delegate（替代cellWidget）
    * 数据约定：列item的 UserRole+1 存可查看的结果runID（0=无结果，结果按钮置灰）
    */
    class TaskCompletedDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        using QStyledItemDelegate::QStyledItemDelegate;

        static QRect setButtonRect(const QRect& cellRect)
        {
            const int top = (cellRect.height() - 20) / 2;
            return QRect(cellRect.left() + 2, cellRect.top() + top, 44, 20);
        }
        static QRect resultButtonRect(const QRect& cellRect)
        {
            return setButtonRect(cellRect).translated(48, 0);
        }

        //按下态查询（paint用）
        bool isLogPressed(int row) const { return _press.isPressed(row, 0); }
        bool isResultPressed(int row) const { return _press.isPressed(row, 1); }

    protected:
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    signals:
        void logClicked(int row);
        void resultClicked(int row);

    private:
        ButtonPressTracker _press;
    };
}


namespace MechIOFile
{
	class mOThirdFile;
}
namespace MechSolveTaskData
{
	class mSolveTaskData;
	class mSolveRunRecord;
	class mSolveTaskManager;
}

namespace MechMainWindow
{
	/*
	* 求解任务配置弹窗（模态，绑定单个模型）
	* 三栏布局（对齐交互原型）：工况列表 / 任务编辑 / 任务列表与提交
	* 所有数据访问显式携带modelID，不依赖"当前模型"
	*/
	class MMAINWINDOW_EXPORT mSolveTaskDialog : public QDialog
	{
		Q_OBJECT

	public:
		/*
		* 打开指定模型的求解任务弹窗
		*/
		static void openDialog(int modelID, QWidget* parent = nullptr);

	private slots:
		//左栏：工况列表
		void slot_filterChanged();
		void slot_toggleCaseSelectAll();
		void slot_toggleSelectAll();
		void slot_addSelectedCases();
		void slot_quickSolve();

		//中栏：任务编辑
		void slot_moveUp();
		void slot_moveDown();
		void slot_removeCases();
		void slot_applyContinuation();
		void slot_chooseOutputDir();
		void slot_resetOutputDir();
		void slot_newTask();
		void slot_saveTask();

		//右栏：任务列表与提交
		void slot_taskListSelectionChanged();
		void slot_editTask();
		void slot_deleteTask();
		void slot_writeFile();
		void slot_submitTask();
		void slot_autoOpenPostToggled(bool checked);

		//执行中心回推
		void slot_runStatusChanged(quint64 runID);
		void slot_writeThreadFinished();

		//求解记录页
		void slot_queueMove(int delta);
		void slot_queueLog();
		void slot_queueAction();
		void slot_deleteRecord();
		void slot_recordsTabChanged(int index);
		void slot_updateElapsed();

		//日志查看器
		void slot_logAppended(quint64 runID, const QString& text);
		void slot_logTabCloseRequested(int index);
		void slot_toggleLogViewer();

	private:
		mSolveTaskDialog(int modelID, QWidget* parent = nullptr);

	protected:
		/*
		* 写文件进行中阻止关闭（写线程回调依赖本弹窗存活）
		*/
		void reject() override;

		/*
		* 构建界面
		*/
		QWidget* buildConfigPage();
		QWidget* buildRecordsPage();

		/*
		* 刷新各区域
		*/
		void refreshCaseList();
		void updateSelectAllBtnText();	//按两张表的勾选态刷新两个"全选/取消全选"按钮文字
		void refreshCaseTypeFilter();
		void refreshCaseTable();
		void refreshTaskList();
		void refreshSubmitPanel();
		void refreshOutputDirLabel();
		void refreshQueueTable();
		void refreshCompletedTable();

		/*
		* 日志查看器
		*/
		void openLogTab(quint64 runID);
		void setLogViewerExpanded(bool expanded);	//展开/收起（收起仅留标题行）
		void showResultActions(quint64 runID);		//"结果"按钮：首次直接新开后处理，已打开过弹选择（新开/跳转）

		/*
		* 编辑器与任务数据互转
		*/
		void loadTaskToEditor(int taskID);
		void clearEditor();
		bool collectEditor(QString& errMsg);			//编辑器 -> _editingTaskID对应任务（含校验）
		void appendCaseRow(int caseID, bool continued);	//编辑表追加一行工况
		void swapCaseRows(int rowA, int rowB);			//交换两行（含勾选/名称/关系/类型）
		QVector<int> checkedCaseRows() const;			//编辑表中勾选的行号
		void updateCaseRowAddState(int caseID, bool added);	//只更新工况列表指定行的添加按钮状态（不重建整表）

		/*
		* 查询辅助
		*/
		MechSolveTaskData::mSolveTaskManager* taskManager() const;
		MechSolveTaskData::mSolveTaskData* editingTask() const;
		MechSolveTaskData::mSolveTaskData* selectedTask() const;
		MechSolveTaskData::mSolveRunRecord* activeRunOfTask(int taskID) const;	//任务的排队/运行中run
		QString taskDisplayStatus(int taskID, QColor* color = nullptr) const;	//状态文本（可带出状态颜色）

		/*
		* 为指定任务写出计算文件（autoSubmit为true时写出成功后直接提交，供一键求解）
		*/
		void writeTaskFile(int taskID, bool autoSubmit);

	private:
		//绑定的模型ID
		int _modelID;

		//页签
		QTabWidget* _tabs;

		//左栏：工况列表（复选框/名称/类型/添加按钮四列）
		QLineEdit* _caseSearch;
		QComboBox* _caseTypeFilter;
		QTableWidget* _caseList;
		QPushButton* _caseListSelectAllBtn = nullptr;	//工况列表全选按钮（文字随全选态切换）

		//中栏：任务编辑
		QPushButton* _taskCaseSelectAllBtn = nullptr;	//任务工况表全选按钮（文字随全选态切换）
		QLineEdit* _taskNameEdit;
		QTableWidget* _caseTable;
		QLineEdit* _outputDirEdit;
		QSpinBox* _cpuSpin;
		QComboBox* _memoryCombo;

		//右栏：任务列表与提交
		QTableWidget* _taskList;
		QLabel* _curTaskLabel;
		QLabel* _solverFileLabel;
		QCheckBox* _autoOpenPostCheck;
		QPushButton* _writeFileBtn;
		QPushButton* _submitBtn;

		//编辑中的任务ID（-1为新建）
		int _editingTaskID = -1;

		//右栏选中的任务ID
		int _selectedTaskID = -1;

		//自定义输出目录（空=默认目录）
		QString _customOutputDir;

		//自动提交跳过目录覆盖检查一次（写文件流程自己建的目录不算"已存在"）
		bool _skipDirCheckOnce = false;

		//写文件线程（写出文件/一键求解共用）
		MechIOFile::mOThirdFile* _writeThread = nullptr;
		int _writeTaskID = -1;
		bool _autoSubmitAfterWrite = false;
		QString _writeFilePath;

		//求解记录页：任务队列
		QTableWidget* _queueTable;
		quint64 _selectedQueueRunID = 0;

		//求解记录页：已完成任务
		QTableWidget* _completedTable;

		//运行中任务计时刷新
		QTimer* _elapsedTimer;

		//日志查看器（仅"求解记录"页显示，常驻可展开/收起）
		QSplitter* _mainSplitter;
		QWidget* _logViewer;
		QWidget* _logBody = nullptr;			//正文容器（收起时隐藏）
		QTabWidget* _logTabs;
		QPushButton* _logToggleBtn;
		bool _logsCollapsed = false;
		QHash<quint64, QPlainTextEdit*> _logPages;
	};
}
