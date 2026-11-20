#include "mSolveTaskPanel.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QDateTime>
#include <QMdiSubWindow>
#include <QDialog>
#include <QCursor>

#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mSolveTaskCenter.h"
#include "mSolveTaskDialog.h"
#include "mSolveRunRecord.h"

using namespace MechPreData;
using namespace MechModelData;
using namespace MechSolveTaskData;
using namespace MxFunctions;
using namespace MAppConfiguration;

namespace MechMainWindow
{
	/*
	* 耗时格式化 hh:mm:ss
	*/
	static QString panelFormatElapsed(qint64 msecs)
	{
		qint64 seconds = msecs > 0 ? msecs / 1000 : 0;
		return QString("%1:%2:%3")
			.arg(seconds / 3600, 2, 10, QChar('0'))
			.arg((seconds / 60) % 60, 2, 10, QChar('0'))
			.arg(seconds % 60, 2, 10, QChar('0'));
	}

	/*
	* 时间戳格式化 HH:mm:ss
	*/
	static QString panelFormatClock(qint64 epochMSecs)
	{
		return epochMSecs > 0 ? QDateTime::fromMSecsSinceEpoch(epochMSecs).toString("HH:mm:ss") : QString("—");
	}

	mSolveTaskPanel::mSolveTaskPanel(QWidget* parent)
		: mPanelBase(parent)
	{
		//整体白底风格（与界面上其他参数面板保持一致）
		this->setStyleSheet(
			"mSolveTaskPanel { background-color: #ffffff; }"
			"QWidget { background-color: #ffffff; }"
			"QComboBox:enabled { background-color: white; }"
			"QComboBox:disabled { background-color: #F5F7FA; color: #808080; }"
			"QLineEdit { background-color: white; }"
			"QLineEdit:disabled { background-color: #F5F7FA; color: #808080; }"
			"QCheckBox { background-color: transparent; }"
			"QLabel { background-color: transparent; }"
			"QPushButton { background-color: white; }"
			"QPushButton:disabled { background-color: #E4E7EE; color: #A8ABB2; }"
			"QTableWidget { background-color: white; }"
			"QHeaderView::section { background-color: #F5F7FA; border-bottom: 1px solid #E4E7EE; }");

		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(8, 8, 8, 8);
		mainLayout->setSpacing(8);

		//顶部操作：一键求解 / 管理任务
		QHBoxLayout* actionLayout = new QHBoxLayout();
		QPushButton* quickBtn = new QPushButton("一键求解", this);
		quickBtn->setMinimumHeight(30);
		connect(quickBtn, &QPushButton::clicked, this, &mSolveTaskPanel::slot_quickSolve);
		QPushButton* manageBtn = new QPushButton("管理任务", this);
		manageBtn->setMinimumHeight(30);
		connect(manageBtn, &QPushButton::clicked, this, &mSolveTaskPanel::slot_manageTasks);
		actionLayout->addWidget(quickBtn, 1);
		actionLayout->addWidget(manageBtn, 1);
		mainLayout->addLayout(actionLayout);

		//任务队列
		QHBoxLayout* queueHeadLayout = new QHBoxLayout();
		QLabel* queueTitle = new QLabel("任务队列", this);
		_queueCountLabel = new QLabel(this);
		queueHeadLayout->addWidget(queueTitle);
		queueHeadLayout->addStretch(1);
		queueHeadLayout->addWidget(_queueCountLabel);
		mainLayout->addLayout(queueHeadLayout);

		_queueTable = new QTableWidget(0, 3, this);
		_queueTable->setHorizontalHeaderLabels(QStringList() << "名称" << "状态" << "时间");
		_queueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		_queueTable->verticalHeader()->hide();
		_queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		_queueTable->setSelectionMode(QAbstractItemView::SingleSelection);
		_queueTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_queueTable->setMinimumHeight(360);
		connect(_queueTable, &QTableWidget::itemSelectionChanged, this, [this]()
			{
				int row = _queueTable->currentRow();
				QTableWidgetItem* item = row >= 0 ? _queueTable->item(row, 0) : nullptr;
				_selectedQueueRunID = item != nullptr ? item->data(Qt::UserRole).toULongLong() : 0;
				//按钮状态随选择变化
				mSolveRunRecord* run = mSolveTaskCenter::getInstance()->getRun(_selectedQueueRunID);
				_queueActionBtn->setText(run != nullptr && run->getStatus() == mSolveRunRecord::Status_Running ? "中断求解" : "取消求解");
			});
		mainLayout->addWidget(_queueTable);

		QHBoxLayout* queueBtnLayout = new QHBoxLayout();
		QPushButton* queueUpBtn = new QPushButton("上移", this);
		QPushButton* queueDownBtn = new QPushButton("下移", this);
		connect(queueUpBtn, &QPushButton::clicked, this, [this]() { slot_queueMove(-1); });
		connect(queueDownBtn, &QPushButton::clicked, this, [this]() { slot_queueMove(1); });
		_queueActionBtn = new QPushButton("取消求解", this);
		connect(_queueActionBtn, &QPushButton::clicked, this, &mSolveTaskPanel::slot_queueAction);
		queueBtnLayout->addWidget(queueUpBtn);
		queueBtnLayout->addWidget(queueDownBtn);
		queueBtnLayout->addStretch(1);
		queueBtnLayout->addWidget(_queueActionBtn);
		mainLayout->addLayout(queueBtnLayout);

		//已完成任务
		QHBoxLayout* completedHeadLayout = new QHBoxLayout();
		QLabel* completedTitle = new QLabel("已完成任务", this);
		_completedCountLabel = new QLabel(this);
		completedHeadLayout->addWidget(completedTitle);
		completedHeadLayout->addStretch(1);
		completedHeadLayout->addWidget(_completedCountLabel);
		mainLayout->addLayout(completedHeadLayout);

		_completedTable = new QTableWidget(0, 3, this);
		_completedTable->setHorizontalHeaderLabels(QStringList() << "名称" << "开始时间" << "耗时");
		_completedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		_completedTable->verticalHeader()->hide();
		_completedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		_completedTable->setSelectionMode(QAbstractItemView::SingleSelection);
		_completedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_completedTable->setMinimumHeight(360);
		connect(_completedTable, &QTableWidget::itemSelectionChanged, this, [this]()
			{
				int row = _completedTable->currentRow();
				QTableWidgetItem* item = row >= 0 ? _completedTable->item(row, 0) : nullptr;
				_selectedCompletedRunID = item != nullptr ? item->data(Qt::UserRole).toULongLong() : 0;
				//查看结果按钮随选中状态启停
				if (_viewResultBtn != nullptr)
				{
					_viewResultBtn->setEnabled(_selectedCompletedRunID != 0);
				}
			});
		mainLayout->addWidget(_completedTable, 1);

		QHBoxLayout* completedBtnLayout = new QHBoxLayout();
		completedBtnLayout->addStretch(1);
		QPushButton* viewResultBtn = new QPushButton("查看结果", this);
		viewResultBtn->setEnabled(false);
		_viewResultBtn = viewResultBtn;
		connect(viewResultBtn, &QPushButton::clicked, this, &mSolveTaskPanel::slot_viewResult);
		completedBtnLayout->addWidget(viewResultBtn);
		mainLayout->addLayout(completedBtnLayout);
		mainLayout->addStretch(1);

		//执行中心状态回推（槽内过滤当前模型）
		connect(mSolveTaskCenter::getInstance(), &mSolveTaskCenter::runStatusChanged,
			this, &mSolveTaskPanel::slot_runStatusChanged);
		//模型切换刷新
		connect(mViewportModelManager::getInstance(), &mViewportModelManager::currentViewportChanged,
			this, &mSolveTaskPanel::slot_modelChanged);
		//运行中任务计时
		_elapsedTimer = new QTimer(this);
		connect(_elapsedTimer, &QTimer::timeout, this, &mSolveTaskPanel::slot_updateElapsed);
		_elapsedTimer->start(1000);

		refreshPanel();
	}

	mSolveTaskPanel::~mSolveTaskPanel()
	{
	}

	int mSolveTaskPanel::currentModelID() const
	{
		if (getCVM == nullptr || getCVM->getModelData() == nullptr || getCVM->isPostModel())
		{
			return -1;
		}
		return getCVM->getModelData()->getModelDataID();
	}

	void mSolveTaskPanel::refreshPanel()
	{
		refreshQueueTable();
		refreshCompletedTable();
	}

	void mSolveTaskPanel::refreshQueueTable()
	{
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		const int modelID = currentModelID();
		const qint64 now = QDateTime::currentMSecsSinceEpoch();

		_queueTable->blockSignals(true);
		_queueTable->setRowCount(0);
		int runningCount = 0;
		int queuedCount = 0;
		if (modelID >= 0)
		{
			int queuedSeen = 0;
			for (mSolveRunRecord* run : center->getActiveRunsOfModel(modelID))
			{
				const bool running = run->getStatus() == mSolveRunRecord::Status_Running;
				if (running)
				{
					runningCount++;
				}
				else
				{
					queuedSeen++;
					queuedCount++;
				}
				int position = running ? 0 : queuedSeen;
				int row = _queueTable->rowCount();
				_queueTable->insertRow(row);

				QTableWidgetItem* nameItem = new QTableWidgetItem(run->getTaskName());
				nameItem->setData(Qt::UserRole, run->getRunID());
				_queueTable->setItem(row, 0, nameItem);
				_queueTable->setItem(row, 1, new QTableWidgetItem(running ? "求解中" : QString("排队中·第%1位").arg(position)));
				QTableWidgetItem* timeItem = new QTableWidgetItem(running ? panelFormatElapsed(now - run->getStartedAt()) : QString("—"));
				timeItem->setTextAlignment(Qt::AlignCenter);
				_queueTable->setItem(row, 2, timeItem);

				if (run->getRunID() == _selectedQueueRunID)
				{
					_queueTable->selectRow(row);
				}
			}
		}
		_queueTable->blockSignals(false);
		_queueCountLabel->setText(QString("%1 个求解中 · %2 个等待").arg(runningCount).arg(queuedCount));
	}

	void mSolveTaskPanel::refreshCompletedTable()
	{
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		const int modelID = currentModelID();

		_completedTable->blockSignals(true);
		_completedTable->setRowCount(0);
		if (modelID >= 0)
		{
			QVector<mSolveRunRecord*> finished;
			for (mSolveRunRecord* run : center->getRunsOfModel(modelID))
			{
				//只记录成功完成的任务（中断/取消/失败不进完成表）
				if (run->getStatus() == mSolveRunRecord::Status_Done)
				{
					finished.push_back(run);
				}
			}
			std::sort(finished.begin(), finished.end(),
				[](const mSolveRunRecord* a, const mSolveRunRecord* b) { return a->getFinishedAt() > b->getFinishedAt(); });
			for (mSolveRunRecord* run : finished)
			{
				int row = _completedTable->rowCount();
				_completedTable->insertRow(row);
				QTableWidgetItem* nameItem = new QTableWidgetItem(run->getTaskName());
				nameItem->setData(Qt::UserRole, run->getRunID());
				_completedTable->setItem(row, 0, nameItem);
				QTableWidgetItem* startItem = new QTableWidgetItem(panelFormatClock(run->getStartedAt()));
				startItem->setTextAlignment(Qt::AlignCenter);
				_completedTable->setItem(row, 1, startItem);
				QTableWidgetItem* elapsedItem = new QTableWidgetItem(
					run->getStartedAt() > 0 ? panelFormatElapsed(run->getFinishedAt() - run->getStartedAt()) : QString("—"));
				elapsedItem->setTextAlignment(Qt::AlignCenter);
				_completedTable->setItem(row, 2, elapsedItem);
				if (run->getRunID() == _selectedCompletedRunID)
				{
					_completedTable->selectRow(row);
				}
			}
			_completedCountLabel->setText(QString("%1 条记录").arg(finished.size()));
		}
			else
			{
				_completedCountLabel->setText("0 条记录");
			}
		_completedTable->blockSignals(false);
		//整表重建期间信号被屏蔽：同步一次按钮状态
		//（选中行存在则保持有效，选中记录已被删/无记录则置灰）
		bool selectionAlive = false;
		for (int row = 0; row < _completedTable->rowCount(); ++row)
		{
			QTableWidgetItem* item = _completedTable->item(row, 0);
			if (item != nullptr && item->data(Qt::UserRole).toULongLong() == _selectedCompletedRunID)
			{
				selectionAlive = true;
				break;
			}
		}
		if (!selectionAlive)
		{
			_selectedCompletedRunID = 0;
		}
		if (_viewResultBtn != nullptr)
		{
			_viewResultBtn->setEnabled(_selectedCompletedRunID != 0);
		}
	}

	void mSolveTaskPanel::slot_quickSolve()
	{
		//一键求解：调用"提交计算"接口（与菜单提交计算一致）
		const int modelID = currentModelID();
		if (modelID < 0)
		{
			return;
		}
		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(modelID);
		if (viewportModel == nullptr || viewportModel->getModelData() == nullptr)
		{
			return;
		}
		std::vector<int> caseIDs = viewportModel->getModelData()->getAllPhyID(Physical_Case);
		if (caseIDs.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig("无分析工况可求解！");
			return;
		}
		mSolveTaskCenter::getInstance()->quickSolve(modelID, caseIDs[0]);
	}

	void mSolveTaskPanel::slot_manageTasks()
	{
		const int modelID = currentModelID();
		if (modelID < 0)
		{
			return;
		}
		mSolveTaskDialog::openDialog(modelID, this);
	}

	void mSolveTaskPanel::slot_queueMove(int delta)
	{
		if (_selectedQueueRunID == 0)
		{
			return;
		}
		mSolveTaskCenter::getInstance()->moveRunOrder(_selectedQueueRunID, delta);
		refreshQueueTable();
	}

	void mSolveTaskPanel::slot_queueAction()
	{
		mSolveRunRecord* run = mSolveTaskCenter::getInstance()->getRun(_selectedQueueRunID);
		if (run == nullptr)
		{
			return;
		}
		if (run->getStatus() == mSolveRunRecord::Status_Running)
		{
			mSolveTaskCenter::getInstance()->interruptRun(run->getRunID());
		}
		else
		{
			mSolveTaskCenter::getInstance()->cancelRun(run->getRunID());
		}
	}

	void mSolveTaskPanel::slot_viewResult()
	{
		//查看结果：与求解任务配置弹窗同逻辑——
		//首次直接新开后处理模型；已打开过弹出选择（新开后处理/跳转已有模型）
		if (_selectedCompletedRunID == 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig("请先在已完成任务中选择要查看的记录！");
			return;
		}
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		quint64 runID = _selectedCompletedRunID;

		if (!center->hasRunPostModel(runID))
		{
			center->openRunResultInNewPost(runID);
			return;
		}

		//已打开过：在按钮位置弹出选择弹窗（标题栏 + 两个带边框按钮，与配置弹窗同款）
		QDialog popup(this);
		popup.setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
		popup.setStyleSheet("QDialog { background-color: white; border: 1px solid #DCDFE6; border-radius: 2px; }");
		QVBoxLayout* popupLayout = new QVBoxLayout(&popup);
		popupLayout->setContentsMargins(0, 0, 0, 8);
		popupLayout->setSpacing(8);

		QWidget* titleBar = new QWidget(&popup);
		titleBar->setStyleSheet("background-color: #E9EDF3;");
		titleBar->setFixedHeight(28);
		QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
		titleLayout->setContentsMargins(10, 0, 4, 0);
		QLabel* titleLabel = new QLabel("结果模型已打开", titleBar);
		titleLabel->setStyleSheet("background-color: transparent; color: #424b57; font-weight: bold;");
		QPushButton* closeBtn = new QPushButton("×", titleBar);
		closeBtn->setFixedSize(24, 24);
		closeBtn->setStyleSheet(
			"QPushButton { border: none; background: transparent; color: #788391; font-size: 14px; }"
			"QPushButton:hover { color: #424b57; background: #DCE3EC; }");
		connect(closeBtn, &QPushButton::clicked, &popup, &QDialog::reject);
		titleLayout->addWidget(titleLabel);
		titleLayout->addStretch(1);
		titleLayout->addWidget(closeBtn);
		popupLayout->addWidget(titleBar);

		QPushButton* newPostBtn = new QPushButton("新开后处理", &popup);
		newPostBtn->setFixedSize(180, 28);
		newPostBtn->setStyleSheet(
			"QPushButton { background: white; color: #424b57; border: 1px solid #DCDFE6; border-radius: 2px; }"
			"QPushButton:hover { border-color: #91CAFF; color: #1890FF; }");
		QPushButton* jumpPostBtn = new QPushButton("跳转已有模型", &popup);
		jumpPostBtn->setFixedSize(180, 28);
		jumpPostBtn->setStyleSheet(
            "QPushButton { background-color: #1890FF; color: white; border: 1px solid #1890FF; border-radius: 2px; padding: 0px 14px; min-height: 0px; max-height: 20px; }"
            "QPushButton:hover { background-color: #40A9FF; border-color: #40A9FF; }"
            "QPushButton:disabled { background-color: #EDF0F5; border-color: #E4E8EE; color: #A6AFBB; }");
		popupLayout->addWidget(newPostBtn, 0, Qt::AlignCenter);
		popupLayout->addWidget(jumpPostBtn, 0, Qt::AlignCenter);

		int choice = -1;
		connect(newPostBtn, &QPushButton::clicked, &popup, [&popup, &choice]() { choice = 0; popup.accept(); });
		connect(jumpPostBtn, &QPushButton::clicked, &popup, [&popup, &choice]() { choice = 1; popup.accept(); });

		popup.adjustSize();
		popup.move(QCursor::pos());
		popup.exec();

		if (choice == 0)
		{
			center->openRunResultInNewPost(runID);
		}
		else if (choice == 1)
		{
			center->jumpToRunPostModel(runID);
		}
		//choice==-1（点"×"或点外部关闭）：不执行任何动作
	}

	void mSolveTaskPanel::slot_runStatusChanged(quint64 runID)
	{
		mSolveRunRecord* run = mSolveTaskCenter::getInstance()->getRun(runID);
		if (run == nullptr || run->getModelID() == currentModelID())
		{
			refreshPanel();
		}
	}

	void mSolveTaskPanel::slot_modelChanged(int viewportID)
	{
		Q_UNUSED(viewportID);
		//模型切换后清空选择并按新模型刷新
		_selectedQueueRunID = 0;
		_selectedCompletedRunID = 0;
		if (_viewResultBtn != nullptr)
		{
			_viewResultBtn->setEnabled(false);
		}
		refreshPanel();
	}

	void mSolveTaskPanel::slot_updateElapsed()
	{
		if (!isVisible())
		{
			return;
		}
		const qint64 now = QDateTime::currentMSecsSinceEpoch();
		for (int row = 0; row < _queueTable->rowCount(); ++row)
		{
			QTableWidgetItem* nameItem = _queueTable->item(row, 0);
			if (nameItem == nullptr)
			{
				continue;
			}
			mSolveRunRecord* run = mSolveTaskCenter::getInstance()->getRun(nameItem->data(Qt::UserRole).toULongLong());
			if (run != nullptr && run->getStatus() == mSolveRunRecord::Status_Running)
			{
				_queueTable->item(row, 2)->setText(panelFormatElapsed(now - run->getStartedAt()));
			}
		}
	}
}
