#include "mSolveTaskDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QGroupBox>
#include <QSplitter>
#include <QFrame>
#include <QTabBar>
#include <QDebug>
#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTime>
#include <QPlainTextEdit>
#include <QTimer>
#include <QScrollBar>
#include <QTextCursor>

#include "mOThirdFile.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mCaseBase.h"
#include "mSolveTaskCenter.h"
#include "mSolveTaskData.h"
#include "mSolveTaskManager.h"

using namespace MechPreData;
using namespace MechCommon;
using namespace MechIOFile;
using namespace MxFunctions;
using namespace MechCaseData;
using namespace MechModelData;
using namespace MechSolveTaskData;
using namespace MAppConfiguration;

namespace MechMainWindow
{
	/******************** 去悬停delegate ********************/

	void NoHoverItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		//剥离悬停态后按默认绘制（选中/交替行等其余状态不受影响）
		QStyleOptionViewItem opt(option);
		opt.state &= ~QStyle::State_MouseOver;
		QStyledItemDelegate::paint(painter, opt, index);
	}

	/******************** 任务列表"操作"列自绘delegate ********************/

	void TaskActionDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		//背景：交替行FAFBFD/白，选中/悬停E8F3FF（底部补画行分隔线，与其它列一致）
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		QColor bgColor = (index.row() % 2 == 1) ? QColor(0xFA, 0xFB, 0xFD) : QColor(Qt::white);
		if ((option.state & QStyle::State_Selected) || (option.state & QStyle::State_MouseOver))
		{
			bgColor = QColor(0xE8, 0xF3, 0xFF);
		}
		painter->fillRect(option.rect, bgColor);

		const bool hasResult = index.data(Qt::UserRole + 1).toULongLong() != 0;
		//仅"待求解"状态可设置（UserRole+2存可设置标志，其余状态置灰）
		const bool settable = index.data(Qt::UserRole + 2).toBool();
		const QRect setRect = setButtonRect(option.rect);
		const QRect resultRect = resultButtonRect(option.rect);

		//设置按钮：白底灰边墨字；结果按钮：有结果同款、无结果置灰；按下时加深背景（点击反馈）
		auto drawButton = [painter](const QRect& rect, const QString& text, bool enabled, bool pressed)
		{
			painter->setPen(QPen(enabled ? QColor(0xDC, 0xDF, 0xE6) : QColor(0xE4, 0xE8, 0xEE), 1));
			painter->setBrush(enabled ? (pressed ? QColor(0xE8, 0xF3, 0xFF) : QColor(Qt::white)) : QColor(0xED, 0xF0, 0xF5));
			painter->drawRoundedRect(rect, 2, 2);
			painter->setPen(enabled ? QColor(0x42, 0x4b, 0x57) : QColor(0xA6, 0xAF, 0xBB));
			painter->drawText(rect, Qt::AlignCenter, text);
		};
		drawButton(setRect, "设置", settable, isSetPressed(index.row()));
		drawButton(resultRect, "结果", hasResult, isResultPressed(index.row()));

		//补画行底分隔线
		painter->setPen(QPen(QColor(0xED, 0xF0, 0xF4), 1));
		painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
		painter->restore();
	}

	bool TaskActionDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
		const QStyleOptionViewItem& option, const QModelIndex& index)
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			//按下时记录哪个按钮被按（paint据此画按下态）
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			const QPoint pos = mouseEvent->pos();
			if (setButtonRect(option.rect).contains(pos))
			{
				_press.handlePress(index.row(), 0);
			}
			else if (resultButtonRect(option.rect).contains(pos))
			{
				_press.handlePress(index.row(), 1);
			}
		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			const QPoint pos = mouseEvent->pos();
			const int pressedBtn = _press.pressedButton();
			_press.handleRelease();

			//仅"待求解"状态可点击设置
			if (setButtonRect(option.rect).contains(pos) && index.data(Qt::UserRole + 2).toBool()
				&& pressedBtn == 0)
			{
				emit setClicked(index.row());
			}
			else if (resultButtonRect(option.rect).contains(pos) && index.data(Qt::UserRole + 1).toULongLong() != 0
				&& pressedBtn == 1)
			{
				emit resultClicked(index.row());
			}
		}
		else if (event->type() == QEvent::Leave)
		{
			_press.handleRelease();
		}
		return QStyledItemDelegate::editorEvent(event, model, option, index);
	}

	/******************** 工况列表"添加"按钮自绘delegate ********************/

	void CaseAddButtonDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		//背景：交替行FAFBFD/白，选中E8F3FF（底部补画行分隔线，与其它列一致）
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		QColor bgColor = (index.row() % 2 == 1) ? QColor(0xFA, 0xFB, 0xFD) : QColor(Qt::white);
		if (option.state & QStyle::State_Selected)
		{
			bgColor = QColor(0xE8, 0xF3, 0xFF);
		}
		painter->fillRect(option.rect, bgColor);

		const bool added = index.data(Qt::UserRole + 1).toBool();
		QRect btnRect = option.rect.adjusted(2, (option.rect.height() - 20) / 2, -2, -(option.rect.height() - 20) / 2);

		//蓝边白底小按钮（已添加=灰字灰边灰底）
		painter->setPen(QPen(added ? QColor(0xE4, 0xE8, 0xEE) : QColor(0x91, 0xCA, 0xFF), 1));
		painter->setBrush(added ? QColor(0xED, 0xF0, 0xF5) : QColor(Qt::white));
		painter->drawRoundedRect(btnRect, 2, 2);
		painter->setPen(added ? QColor(0xA6, 0xAF, 0xBB) : QColor(0x18, 0x90, 0xFF));
		painter->drawText(btnRect, Qt::AlignCenter, added ? "已添加" : "添加");

		//补画行底分隔线
		painter->setPen(QPen(QColor(0xED, 0xF0, 0xF4), 1));
		painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
		painter->restore();
	}

	bool CaseAddButtonDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
		const QStyleOptionViewItem& option, const QModelIndex& index)
	{
		if (event->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			QRect btnRect = option.rect.adjusted(2, (option.rect.height() - 20) / 2, -2, -(option.rect.height() - 20) / 2);
			if (btnRect.contains(mouseEvent->pos()) && !index.data(Qt::UserRole + 1).toBool())
			{
				emit addClicked(index.row());
			}
		}
		return QStyledItemDelegate::editorEvent(event, model, option, index);
	}

	/******************** 工况关系列自绘delegate ********************/

	RelationComboDelegate::RelationComboDelegate(QTableWidget* table, QObject* parent)
		: QStyledItemDelegate(parent), _table(table)
	{
	}

	void RelationComboDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		//背景：交替行FAFBFD/白，选中行E8F3FF（自绘delegate不走QSS的item边框，行分隔线自己补画）
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		QColor bgColor = (index.row() % 2 == 1) ? QColor(0xFA, 0xFB, 0xFD) : QColor(Qt::white);
		if (option.state & QStyle::State_Selected)
		{
			bgColor = QColor(0xE8, 0xF3, 0xFF);
		}
		painter->fillRect(option.rect, bgColor);

		//普通下拉框外观（对齐弹窗QSS样式）：高20垂直居中，首行置灰
		const bool firstRow = index.row() == 0;
		const QString text = index.data(Qt::UserRole).toInt() == 1 ? "接续上一步" : "独立计算";
		QRect comboRect = option.rect.adjusted(2, (option.rect.height() - 20) / 2, -2, -(option.rect.height() - 20) / 2);

		//边框+底色
		painter->setPen(QPen(firstRow ? QColor(0xE4, 0xE8, 0xEE) : QColor(0xDC, 0xDF, 0xE6), 1));
		painter->setBrush(firstRow ? QColor(0xF5, 0xF7, 0xFA) : QColor(Qt::white));
		painter->drawRoundedRect(comboRect, 2, 2);

		//文字（左侧留7px内边距，右侧让出箭头区）
		painter->setPen(firstRow ? QColor(0x80, 0x80, 0x80) : QColor(0x42, 0x4b, 0x57));
		painter->drawText(comboRect.adjusted(7, 0, -16, 0), Qt::AlignLeft | Qt::AlignVCenter, text);

		//右侧下拉箭头
		QPoint center(comboRect.right() - 8, comboRect.center().y());
		QPolygon arrow;
		arrow << QPoint(center.x() - 3, center.y() - 1) << QPoint(center.x() + 3, center.y() - 1) << QPoint(center.x(), center.y() + 2);
		painter->setPen(Qt::NoPen);
		painter->setBrush(firstRow ? QColor(0xA6, 0xAF, 0xBB) : QColor(0x60, 0x62, 0x66));
		painter->drawPolygon(arrow);

		//补画行底分隔线（与其它列的QSS边框一致）
		painter->setPen(QPen(QColor(0xED, 0xF0, 0xF4), 1));
		painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
		painter->restore();
	}

	bool RelationComboDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
		const QStyleOptionViewItem& option, const QModelIndex& index)
	{
		//点击关系单元格（首行除外）：在单元格正下方弹出选项菜单（同步执行，位置自算无时序问题）
		if (event->type() == QEvent::MouseButtonRelease && index.row() > 0 && _table != nullptr)
		{
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			if (option.rect.contains(mouseEvent->pos()))
			{
				QMenu menu(_table);
				menu.setStyleSheet(
					"QMenu { background-color: white; border: 1px solid #DCDFE6; border-radius: 2px; padding: 4px 0; }"
					"QMenu::item { padding: 4px 40px 4px 8px; color: #424b57; }"
					"QMenu::item:selected { background-color: #F0F7FF; color: #1890FF; }");
				QAction* independent = menu.addAction("独立计算");
				QAction* continued = menu.addAction("接续上一步");
				QPoint popupPos = _table->viewport()->mapToGlobal(option.rect.bottomLeft());
				QAction* chosen = menu.exec(popupPos);
				if (chosen != nullptr)
				{
					model->setData(index, chosen == continued ? 1 : 0, Qt::UserRole);
				}
			}
		}
		return QStyledItemDelegate::editorEvent(event, model, option, index);
	}
    /******************** 已完成任务"操作"列自绘delegate ********************/

    void TaskCompletedDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        //背景：交替行FAFBFD/白，选中/悬停E8F3FF（底部补画行分隔线，与其它列一致）
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QColor bgColor = (index.row() % 2 == 1) ? QColor(0xFA, 0xFB, 0xFD) : QColor(Qt::white);
        if ((option.state & QStyle::State_Selected) || (option.state & QStyle::State_MouseOver))
        {
            bgColor = QColor(0xE8, 0xF3, 0xFF);
        }
        painter->fillRect(option.rect, bgColor);

        const bool hasResult = index.data(Qt::UserRole + 1).toULongLong() != 0;
        const QRect setRect = setButtonRect(option.rect);
        const QRect resultRect = resultButtonRect(option.rect);

        //设置按钮：白底灰边墨字；结果按钮：有结果同款、无结果置灰；按下时加深背景（点击反馈）
        auto drawButton = [painter](const QRect& rect, const QString& text, bool enabled, bool pressed)
            {
                painter->setPen(QPen(enabled ? QColor(0xDC, 0xDF, 0xE6) : QColor(0xE4, 0xE8, 0xEE), 1));
                painter->setBrush(enabled ? (pressed ? QColor(0xE8, 0xF3, 0xFF) : QColor(Qt::white)) : QColor(0xED, 0xF0, 0xF5));
                painter->drawRoundedRect(rect, 2, 2);
                painter->setPen(enabled ? QColor(0x42, 0x4b, 0x57) : QColor(0xA6, 0xAF, 0xBB));
                painter->drawText(rect, Qt::AlignCenter, text);
            };
        drawButton(setRect, "日志", true, isLogPressed(index.row()));
        drawButton(resultRect, "结果", hasResult, isResultPressed(index.row()));

        //补画行底分隔线
        painter->setPen(QPen(QColor(0xED, 0xF0, 0xF4), 1));
        painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
        painter->restore();
    }

    bool TaskCompletedDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
        const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            //按下时记录（paint画按下态）
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint pos = mouseEvent->pos();
            if (setButtonRect(option.rect).contains(pos))
            {
                _press.handlePress(index.row(), 0);
            }
            else if (resultButtonRect(option.rect).contains(pos))
            {
                _press.handlePress(index.row(), 1);
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint pos = mouseEvent->pos();
            const int pressedBtn = _press.pressedButton();
            _press.handleRelease();

            if (setButtonRect(option.rect).contains(pos) && pressedBtn == 0)
            {
                emit logClicked(index.row());
            }
            else if (resultButtonRect(option.rect).contains(pos) && index.data(Qt::UserRole + 1).toULongLong() != 0
                && pressedBtn == 1)
            {
                emit resultClicked(index.row());
            }
        }
        else if (event->type() == QEvent::Leave)
        {
            _press.handleRelease();
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
	/*
	* 耗时格式化 hh:mm:ss
	*/	static QString formatElapsed(qint64 msecs)
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
	static QString formatClock(qint64 epochMSecs)
	{
		return epochMSecs > 0 ? QDateTime::fromMSecsSinceEpoch(epochMSecs).toString("HH:mm:ss") : QString("—");
	}

	/*
	* 主操作按钮（蓝底白字，如保存任务/提交求解/管理任务）
	*/
	static const char* kPrimaryButtonStyle =
		"QPushButton { background-color: #1890FF; color: white; border: 1px solid #1890FF; border-radius: 2px; padding: 0px 14px; min-height: 0px; max-height: 20px; }"
		"QPushButton:hover { background-color: #40A9FF; border-color: #40A9FF; }"
		"QPushButton:disabled { background-color: #EDF0F5; border-color: #E4E8EE; color: #A6AFBB; }";

	/*
	* 次操作按钮（白底蓝字蓝边，如一键求解/加入任务）
	*/
	static const char* kOutlineButtonStyle =
		"QPushButton { background-color: white; color: #1890FF; border: 1px solid #91CAFF; border-radius: 2px; padding: 0px 12px; min-height: 0px; max-height: 20px; }"
		"QPushButton:hover { background-color: #E8F3FF; border-color: #1890FF; }"
		"QPushButton:disabled { background-color: #EDF0F5; border-color: #E4E8EE; color: #A6AFBB; }";

	/*
	* 节标题（统一加粗墨色，替代QGroupBox自带标题避免与内容重叠）
	*/
	static QLabel* sectionLabel(const QString& text, QWidget* parent)
	{
		QLabel* label = new QLabel(text, parent);
		label->setStyleSheet("QLabel { font-weight: bold; font-size: 9pt; color: #424b57; }");
		label->setFixedHeight(20);
		return label;
	}

	static QFrame* hSeparator(QWidget* parent)
	{
		QFrame* line = new QFrame(parent);
		line->setFrameShape(QFrame::HLine);
		line->setFixedHeight(1);
		line->setStyleSheet("QFrame { border: none; border-top: 1px solid #E4E8EE; background-color: transparent; }");
		return line;
	}

	/*
	* 工况类型所属分类（用于类型列显示与筛选，不带"分析"后缀）
	*/
	static QString caseTypeCategory(MxFunctions::CaseType type)
	{
		switch (type)
		{
		case MxFunctions::ImplicitLinearStatic:
		case MxFunctions::ImplicitNonLinearStatic:
			return "静态";
		case MxFunctions::ImplicitNonLinearDynamic:
			return "隐式动力学";
		case MxFunctions::ImplicitLinearBuckling:
		case MxFunctions::ImplicitPostBuckling:
			return "稳定性";
		case MxFunctions::ImplicitSteadyHeat:
		case MxFunctions::ImplicitTransientHeat:
		case MxFunctions::ImplicitStructuralCouplingHeat:
			return "传热";
		case MxFunctions::ImplicitModal:
		case MxFunctions::ImplicitResponseSpectrum:
		case MxFunctions::ImplicitRandomVibration:
		case MxFunctions::ImplicitFrequency:
		case MxFunctions::ImplicitTransient:
		case MxFunctions::ASCoupling:
		case MxFunctions::ImplicitGeneric:
			return "线性动态";
		default: return QString();
		}
	}

	/*
	* 求解核数序列（2的幂：1~256），步进与手动输入均吸附到序列值
	*/
	class CpuSpinBox : public QSpinBox
	{
	public:
		using QSpinBox::QSpinBox;

	protected:
		static int snapIndex(int value)
		{
			static const int seq[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };
			int index = 0;
			int best = INT_MAX;
			for (int i = 0; i < 9; ++i)
			{
				int diff = qAbs(seq[i] - value);
				if (diff < best)
				{
					best = diff;
					index = i;
				}
			}
			return index;
		}

		void stepBy(int steps) override
		{
			static const int seq[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };
			int index = qBound(0, snapIndex(value()) + steps, 8);
			setValue(seq[index]);
		}

		void fixup(QString& input) const override
		{
			static const int seq[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };
			bool ok = false;
			int value = input.toInt(&ok);
			input = QString::number(seq[ok ? snapIndex(value) : 0]);
		}
	};

	void mSolveTaskDialog::openDialog(int modelID, QWidget* parent)
	{
		mSolveTaskDialog dialog(modelID, parent);
		dialog.exec();
	}

	mSolveTaskDialog::mSolveTaskDialog(int modelID, QWidget* parent)
		: QDialog(parent), _modelID(modelID)
	{
		setWindowTitle(QString("求解任务配置"));
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
		setModal(true);
		resize(1100, 660);
		//整体白底扁平风格（对齐交互原型：无分组框边框，细分隔线分区）
		setStyleSheet(
			"QDialog { background-color: #ffffff; }"
			"QWidget { background-color: #ffffff; }"
			"QLabel { background-color: transparent; color: #424b57; }"
			"QLineEdit { background-color: white; border: 1px solid #DCDFE6; border-radius: 2px; padding: 0px 7px; min-height: 0px; max-height: 20px; color: #424b57; }"
			"QLineEdit:disabled { background-color: #F5F7FA; color: #808080; }"
			"QComboBox { background-color: white; border: 1px solid #DCDFE6; border-radius: 2px; padding: 0px 7px; min-height: 0px; max-height: 20px; color: #424b57; }"
			"QComboBox:disabled { background-color: #F5F7FA; color: #808080; }"
			"QSpinBox { background-color: white; border: 1px solid #DCDFE6; border-radius: 2px; padding: 0px 4px; min-height: 0px; max-height: 20px; color: #424b57; }"
			"QCheckBox { background-color: transparent; color: #424b57; }"
			"QPushButton { background-color: white; border: 1px solid #DCDFE6; border-radius: 2px; padding: 0px 10px; color: #424b57; }"
			"QPushButton:hover { border-color: #91CAFF; color: #1890FF; background-color: #F5FAFF; }"
			"QPushButton:disabled { background-color: #EDF0F5; border-color: #E4E8EE; color: #A6AFBB; }"
			"QGroupBox { background-color: transparent; border: none; margin-top: 0px; padding: 4px 4px 4px 4px; }"
			"QTabWidget::pane { border: none; }"
			"QTabBar::tab { background: transparent; color: #788391; padding: 6px 6px; border: none; }"
			"QTabBar::tab:selected { color: #1890FF; border-bottom: 2px solid #1890FF; }"
			"QTableWidget { background-color: white; border: 1px solid #E4E8EE; alternate-background-color: #FAFBFD; outline: none; }"
			"QTableWidget::item { padding: 4px 6px; border-bottom: 1px solid #EDF0F4; }"
			"QTableWidget::item:selected { background: #E8F3FF; color: #424b57; }"
			"QHeaderView::section { background-color: #E9EDF3; color: #687383; border: none; border-bottom: 1px solid #E4E8EE; padding: 4px 6px; }"
			"QListWidget { background-color: white; border: 1px solid #E4E8EE; }"
			"QPlainTextEdit { background-color: white; border: 1px solid #E4E8EE; }");

		QVBoxLayout* rootLayout = new QVBoxLayout(this);
		_tabs = new QTabWidget(this);
		//页签字体设在tabBar控件上：QSS子控件字体只影响绘制不影响尺寸计算，
		//调字号只改这一处setPointSize。
		QFont tabFont("Microsoft YaHei");
		tabFont.setPointSize(9);
		_tabs->tabBar()->setFont(tabFont);
		_tabs->addTab(buildConfigPage(), "任务配置");
		_tabs->addTab(buildRecordsPage(), "求解记录");
		connect(_tabs, &QTabWidget::currentChanged, this, &mSolveTaskDialog::slot_recordsTabChanged);

		//底部任务日志查看器（仅在"求解记录"页显示，常驻可展开/收起）
		_logViewer = new QWidget(this);
		QVBoxLayout* logLayout = new QVBoxLayout(_logViewer);
		logLayout->setContentsMargins(13, 0, 13, 0);
        logLayout->addWidget(hSeparator(_logViewer));
		QHBoxLayout* logHeadLayout = new QHBoxLayout();
		QLabel* logTitle = sectionLabel("日志查看", _logViewer);
		_logToggleBtn = new QPushButton("收起", _logViewer);
		connect(_logToggleBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_toggleLogViewer);
		logHeadLayout->addWidget(logTitle);
		logHeadLayout->addStretch(1);
		logHeadLayout->addWidget(_logToggleBtn);
		logLayout->addLayout(logHeadLayout);
		//正文容器（收起时整体隐藏，仅留标题行）
		_logBody = new QWidget(_logViewer);
		QVBoxLayout* logBodyLayout = new QVBoxLayout(_logBody);
		logBodyLayout->setContentsMargins(0, 0, 0, 0);
		_logTabs = new QTabWidget(_logBody);
        _logTabs->setStyleSheet(
            "QTabBar { background: #F5F7FA; border: 1px solid #E4E8EE; }"
            "QTabBar::close-button {image: url(:/mMainWidgets/Icon/close3x.png)}"
            "QTabBar::tab {background: #F5F7FA ; border: 1px solid #E4E8EE; font-size: 12px; padding: 6px 6px;}"
            "QTabBar::tab:selected {background: #FFFFFF ; color: #1890FF; border-bottom: 2px solid #1890FF; }"
        );
		_logTabs->setTabsClosable(true);
		_logTabs->setDocumentMode(true);
		connect(_logTabs, &QTabWidget::tabCloseRequested, this, &mSolveTaskDialog::slot_logTabCloseRequested);
		logBodyLayout->addWidget(_logTabs);
		logLayout->addWidget(_logBody);

		_mainSplitter = new QSplitter(Qt::Vertical, this);
		_mainSplitter->setChildrenCollapsible(false);
		_mainSplitter->addWidget(_tabs);
		_mainSplitter->addWidget(_logViewer);
		//上方页签区设最小高度，日志栏可拖至窗口一半
		_tabs->setMinimumHeight(350);
		_mainSplitter->setStretchFactor(0, 1);
		_mainSplitter->setStretchFactor(1, 1);
		_mainSplitter->setSizes(QList<int>() << 640 << 240);
		rootLayout->addWidget(_mainSplitter);
		//初始在"任务配置"页，日志栏隐藏；切到求解记录页时显示（默认收起）
		_logViewer->hide();
		_logsCollapsed = true;
		_logBody->hide();
		_logToggleBtn->setText("展开");

		//执行中心状态回推（槽内过滤本模型）
		connect(mSolveTaskCenter::getInstance(), &mSolveTaskCenter::runStatusChanged,
			this, &mSolveTaskDialog::slot_runStatusChanged);
		//日志实时追加
		connect(mSolveTaskCenter::getInstance(), &mSolveTaskCenter::logAppended,
			this, &mSolveTaskDialog::slot_logAppended);

		//运行中任务已运行时长每秒刷新
		_elapsedTimer = new QTimer(this);
		connect(_elapsedTimer, &QTimer::timeout, this, &mSolveTaskDialog::slot_updateElapsed);
		_elapsedTimer->start(1000);

		refreshCaseTypeFilter();
		refreshCaseList();
		slot_newTask();
		refreshTaskList();
	}

	void mSolveTaskDialog::reject()
	{
		//写文件进行中不允许关闭（线程回调依赖本弹窗存活）
		if (_writeThread != nullptr)
		{
			QMessageBox::information(this, "求解任务", "计算文件正在写出，请稍候...");
			return;
		}
		QDialog::reject();
	}

	QWidget* mSolveTaskDialog::buildConfigPage()
	{
		QWidget* page = new QWidget(this);
		QHBoxLayout* columnsLayout = new QHBoxLayout(page);
		columnsLayout->setContentsMargins(4, 0, 4, 0);
		columnsLayout->setSpacing(8);

		/******************** 左栏：工况列表 ********************/
		QWidget* leftWidget = new QWidget(page);
		QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
		leftLayout->setContentsMargins(0, 0, 0, 0);
		leftLayout->setSpacing(4);
		//标题行置于栏包装层顶部（三栏结构一致，保证标题平齐）
		QHBoxLayout* caseTitleLayout = new QHBoxLayout();
		caseTitleLayout->setContentsMargins(4, 0, 0, 0);
		caseTitleLayout->addWidget(sectionLabel("工况列表", leftWidget));
		caseTitleLayout->addStretch(1);
		leftLayout->addLayout(caseTitleLayout);

		QGroupBox* caseGroup = new QGroupBox(leftWidget);
		QVBoxLayout* caseLayout = new QVBoxLayout(caseGroup);
		caseLayout->setContentsMargins(0, 0, 0, 0);
		_caseSearch = new QLineEdit(caseGroup);
		_caseSearch->setPlaceholderText("搜索工况名称");
		_caseSearch->setClearButtonEnabled(true);
		connect(_caseSearch, &QLineEdit::textChanged, this, &mSolveTaskDialog::slot_filterChanged);
		_caseTypeFilter = new QComboBox(caseGroup);
		connect(_caseTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { refreshCaseList(); });
		//搜索与类型过滤同一行
		QHBoxLayout* caseFilterLayout = new QHBoxLayout();
		caseFilterLayout->addWidget(_caseSearch, 1);
		caseFilterLayout->addWidget(_caseTypeFilter);
		caseLayout->addLayout(caseFilterLayout);
		//类型下拉框下方：全选（作用于工况列表勾选，靠右）
		QHBoxLayout* caseToolbarLayout = new QHBoxLayout();
		caseToolbarLayout->addStretch(1);
		QPushButton* caseSelectAllBtn = new QPushButton("全选", caseGroup);
		connect(caseSelectAllBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_toggleCaseSelectAll);
		caseSelectAllBtn->setFixedWidth(caseSelectAllBtn->sizeHint().width() - 20);
		caseToolbarLayout->addWidget(caseSelectAllBtn);
		_caseListSelectAllBtn = caseSelectAllBtn;
		caseLayout->addLayout(caseToolbarLayout);
		//工况列表：复选框/名称居左，类型/添加按钮居右（无表头，原型风格：行分隔线无网格）
		_caseList = new QTableWidget(0, 4, caseGroup);
		_caseList->horizontalHeader()->hide();
		_caseList->verticalHeader()->hide();
		_caseList->verticalHeader()->setDefaultSectionSize(34);
		_caseList->setShowGrid(false);
		_caseList->setAlternatingRowColors(true);
		_caseList->setColumnWidth(0, 26);
		_caseList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		_caseList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
		_caseList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
		_caseList->setColumnWidth(3, 72);
		_caseList->setSelectionBehavior(QAbstractItemView::SelectRows);
		_caseList->setSelectionMode(QAbstractItemView::SingleSelection);
		_caseList->setEditTriggers(QAbstractItemView::NoEditTriggers);
		//第4列"添加"按钮delegate自绘（不用cellWidget）
		CaseAddButtonDelegate* caseAddDelegate = new CaseAddButtonDelegate(_caseList);
		_caseList->setItemDelegateForColumn(3, caseAddDelegate);
		connect(caseAddDelegate, &CaseAddButtonDelegate::addClicked, this, [this](int row)
			{
				QTableWidgetItem* checkItem = _caseList->item(row, 0);
				if (checkItem == nullptr)
				{
					return;
				}
				int caseID = checkItem->data(Qt::UserRole).toInt();
				appendCaseRow(caseID, false);
				updateCaseRowAddState(caseID, true);
			});
		//点击行内任意位置（复选框列/添加按钮列除外，其自身处理交互）联动勾选/取消该行复选框；
		//已添加行的复选框已禁用，点击不联动
		connect(_caseList, &QTableWidget::cellClicked, this, [this](int row, int column)
			{
				if (column == 0 || column == 3)
				{
					return;
				}
				QTableWidgetItem* checkItem = _caseList->item(row, 0);
				if (checkItem != nullptr && (checkItem->flags() & Qt::ItemIsUserCheckable))
				{
					checkItem->setCheckState(checkItem->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
				}
			});
		caseLayout->addWidget(_caseList, 1);
		QHBoxLayout* caseBtnLayout = new QHBoxLayout();
		QPushButton* quickBtn = new QPushButton("一键求解", caseGroup);
		quickBtn->setStyleSheet(kOutlineButtonStyle);
		connect(quickBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_quickSolve);
		QPushButton* addBtn = new QPushButton("加入任务", caseGroup);
		addBtn->setStyleSheet(kOutlineButtonStyle);
		connect(addBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_addSelectedCases);
		caseBtnLayout->addWidget(quickBtn);
		caseBtnLayout->addStretch(1);
		caseBtnLayout->addWidget(addBtn);
		caseLayout->addLayout(caseBtnLayout);
		leftLayout->addWidget(caseGroup, 1);

		/******************** 中栏：任务编辑 ********************/
		QWidget* editorWidget = new QWidget(page);
		QVBoxLayout* editorLayout = new QVBoxLayout(editorWidget);
		editorLayout->setContentsMargins(0, 0, 0, 0);
		editorLayout->setSpacing(4);
		//标题行："新建任务"标题 + 右侧"新建任务"按钮（置于栏包装层顶部，与其他栏标题平齐）
		//载入已有任务编辑时标题改为"修改任务"
		QHBoxLayout* editorTitleLayout = new QHBoxLayout();
		editorTitleLayout->setContentsMargins(4, 0, 4, 0);
		QLabel* editorTitle = sectionLabel("新建任务", editorWidget);
		editorTitle->setObjectName("editorTitleLabel");
		QPushButton* newBtn = new QPushButton("新建任务", editorWidget);
		connect(newBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_newTask);
		editorTitleLayout->addWidget(editorTitle);
		editorTitleLayout->addStretch(1);
		editorTitleLayout->addWidget(newBtn);
		editorLayout->addLayout(editorTitleLayout);

		QGroupBox* editorGroup = new QGroupBox(editorWidget);
		QVBoxLayout* editLayout = new QVBoxLayout(editorGroup);
		editLayout->setContentsMargins(0, 0, 0, 0);
		QFormLayout* nameLayout = new QFormLayout();
		_taskNameEdit = new QLineEdit(editorGroup);
		nameLayout->addRow("任务名称：", _taskNameEdit);
		editLayout->addLayout(nameLayout);
		editLayout->addWidget(hSeparator(editorGroup));

		QLabel* tableLabel = new QLabel("任务工况：", editorGroup);
		editLayout->addWidget(tableLabel);
		_caseTable = new QTableWidget(0, 4, editorGroup);
		_caseTable->setHorizontalHeaderLabels(QStringList() << "选择" << "工况名称" << "工况关系" << "类型");
		_caseTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
		_caseTable->setColumnWidth(0, 36);
		_caseTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		_caseTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
		_caseTable->setColumnWidth(2, 112);
		_caseTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
		_caseTable->setColumnWidth(3, 78);
		//表头与列内容对齐方式一致：选择/关系/类型居中，名称居左
		_caseTable->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignCenter);
		_caseTable->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_caseTable->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignCenter);
		_caseTable->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignCenter);
		_caseTable->verticalHeader()->setDefaultSectionSize(34);
		_caseTable->setShowGrid(false);
		_caseTable->setAlternatingRowColors(true);
		_caseTable->verticalHeader()->hide();
		//选中整行高亮E8F3FF（同任务列表）；点击名称/类型列仍联动勾选该行
		_caseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		_caseTable->setSelectionMode(QAbstractItemView::SingleSelection);
		//工况关系列用delegate自绘（不用cellWidget）
		_caseTable->setItemDelegateForColumn(2, new RelationComboDelegate(_caseTable, _caseTable));
		connect(_caseTable, &QTableWidget::cellClicked, this, [this](int row, int column)
			{
				if (column == 0 || column == 2)
				{
					return;
				}
				QTableWidgetItem* checkItem = _caseTable->item(row, 0);
				if (checkItem != nullptr)
				{
					checkItem->setCheckState(checkItem->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
				}
			});
		_caseTable->setMinimumHeight(160);
		editLayout->addWidget(_caseTable, 1);
		QHBoxLayout* rowBtnLayout = new QHBoxLayout();
		QPushButton* selectAllBtn = new QPushButton("全选", editorGroup);
        selectAllBtn->setStyleSheet("QPushButton { padding: 0px 2px; }");
        selectAllBtn->setFixedWidth(40);
		_taskCaseSelectAllBtn = selectAllBtn;
		QPushButton* upBtn = new QPushButton("上移", editorGroup);
		QPushButton* downBtn = new QPushButton("下移", editorGroup);
		QPushButton* removeBtn = new QPushButton("删除", editorGroup);
		QPushButton* continueBtn = new QPushButton("一键接续", editorGroup);
		//紧凑样式：全选/上移/下移/删除固定30px，一键接续固定60px
		auto compactBtn = [](QPushButton* btn)
		{
			btn->setStyleSheet("QPushButton { padding: 0px 2px; }");
			btn->setFixedWidth(40);
		};
		compactBtn(upBtn);
		compactBtn(downBtn);
		removeBtn->setStyleSheet("QPushButton { padding: 0px 2px; color: #D95151; } QPushButton:hover { color: #D95151; border-color: #D95151; }");
		removeBtn->setFixedWidth(40);
		continueBtn->setStyleSheet("QPushButton { padding: 0px 4px; }");
		continueBtn->setFixedWidth(70);
		connect(selectAllBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_toggleSelectAll);
		connect(upBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_moveUp);
		connect(downBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_moveDown);
		connect(removeBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_removeCases);
		connect(continueBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_applyContinuation);
		rowBtnLayout->addWidget(selectAllBtn);
		rowBtnLayout->addStretch(1);
		rowBtnLayout->addWidget(continueBtn);
		rowBtnLayout->addWidget(upBtn);
		rowBtnLayout->addWidget(downBtn);
		rowBtnLayout->addWidget(removeBtn);
		editLayout->addLayout(rowBtnLayout);

		editLayout->addWidget(hSeparator(editorGroup));

		QGroupBox* controlGroup = new QGroupBox(editorWidget);
		QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);
		controlLayout->setContentsMargins(0, 0, 0, 0);
		//标题行
		QHBoxLayout* controlTitleLayout = new QHBoxLayout();
		controlTitleLayout->addWidget(sectionLabel("任务控制", controlGroup));
		controlTitleLayout->addStretch(1);
		controlLayout->addLayout(controlTitleLayout);
		//输出位置行
		QHBoxLayout* outputRowLayout = new QHBoxLayout();
		outputRowLayout->addWidget(new QLabel("输出位置：", controlGroup));
		_outputDirEdit = new QLineEdit(controlGroup);
		_outputDirEdit->setReadOnly(true);
		QPushButton* chooseDirBtn = new QPushButton(controlGroup);
        chooseDirBtn->setIcon(QIcon(":/mMainWidgets/Icon/Case/openFile@3x.png"));
        chooseDirBtn->setFixedSize(30, 30);
        chooseDirBtn->setIconSize(QSize(20, 20));
		connect(chooseDirBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_chooseOutputDir);
		outputRowLayout->addWidget(_outputDirEdit, 1);
		outputRowLayout->addWidget(chooseDirBtn);
		controlLayout->addLayout(outputRowLayout);
		//求解核数与内存策略同行
		QHBoxLayout* resourceRowLayout = new QHBoxLayout();
		resourceRowLayout->addWidget(new QLabel("求解核数：", controlGroup));
		_cpuSpin = new CpuSpinBox(controlGroup);
        _cpuSpin->setFixedWidth(96);
		_cpuSpin->setRange(1, 256);
		_cpuSpin->setValue(8);
		resourceRowLayout->addWidget(_cpuSpin);
		resourceRowLayout->addWidget(new QLabel("内存策略：", controlGroup));
		_memoryCombo = new QComboBox(controlGroup);
        _memoryCombo->setFixedWidth(95);
		_memoryCombo->addItem("核内");
		_memoryCombo->addItem("核外");
		_memoryCombo->addItem("自适应");
		_memoryCombo->setCurrentIndex(2);
		resourceRowLayout->addWidget(_memoryCombo);
		controlLayout->addLayout(resourceRowLayout);
		editLayout->addWidget(controlGroup);

		QHBoxLayout* saveBtnLayout = new QHBoxLayout();
		saveBtnLayout->addStretch(1);
		QPushButton* saveBtn = new QPushButton("保存任务", editorGroup);
		saveBtn->setStyleSheet(kPrimaryButtonStyle);
		connect(saveBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_saveTask);
		saveBtnLayout->addWidget(saveBtn);
		editLayout->addLayout(saveBtnLayout);

		editorLayout->addWidget(editorGroup, 1);

		/******************** 右栏：任务列表与提交 ********************/
		QWidget* rightWidget = new QWidget(page);
		QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
		rightLayout->setContentsMargins(0, 0, 0, 0);
		rightLayout->setSpacing(4);
		//标题行："任务列表"标题 + 右侧"删除任务"按钮（置于栏包装层顶部，与其他栏标题平齐）
		QHBoxLayout* listTitleLayout = new QHBoxLayout();
		listTitleLayout->setContentsMargins(4, 0, 4, 0);
		QLabel* listTitle = sectionLabel("任务列表", rightWidget);
		QPushButton* deleteBtn = new QPushButton("删除任务", rightWidget);
		connect(deleteBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_deleteTask);
		listTitleLayout->addWidget(listTitle);
		listTitleLayout->addStretch(1);
		listTitleLayout->addWidget(deleteBtn);
		rightLayout->addLayout(listTitleLayout);

		QGroupBox* listGroup = new QGroupBox(rightWidget);
		QVBoxLayout* listLayout = new QVBoxLayout(listGroup);
		listLayout->setContentsMargins(0, 0, 0, 0);
		_taskList = new QTableWidget(0, 3, listGroup);
		_taskList->setHorizontalHeaderLabels(QStringList() << "任务 / 工况数" << "状态 / 队位" << "操作");
		_taskList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		_taskList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
		_taskList->setColumnWidth(1, 96);
		_taskList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
		_taskList->setColumnWidth(2, 100);
		//表头与内容统一左对齐
		_taskList->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_taskList->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_taskList->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_taskList->verticalHeader()->setDefaultSectionSize(34);
		_taskList->setShowGrid(false);
		_taskList->setAlternatingRowColors(true);
		_taskList->verticalHeader()->hide();
		//只保留选中高亮E8F3FF，无悬停态，交替行FAFBFD
		_taskList->setSelectionBehavior(QAbstractItemView::SelectRows);
		_taskList->setSelectionMode(QAbstractItemView::SingleSelection);
		_taskList->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_taskList->setStyleSheet(
			"QTableWidget { background: white; alternate-background-color: #FAFBFD; }"
			"QTableWidget::item:selected { background: #E8F3FF; color: #424b57; }");
		//整表去悬停（原生Fusion的悬停深蓝由delegate剥离State_MouseOver消除）
		_taskList->setItemDelegate(new NoHoverItemDelegate(_taskList));
		//操作列（设置/结果）delegate自绘（不用cellWidget）
		TaskActionDelegate* taskActionDelegate = new TaskActionDelegate(_taskList);
		_taskList->setItemDelegateForColumn(2, taskActionDelegate);
		connect(taskActionDelegate, &TaskActionDelegate::setClicked, this, [this](int row)
			{
				QTableWidgetItem* item = _taskList->item(row, 0);
				if (item == nullptr)
				{
					return;
				}
				int taskID = item->data(Qt::UserRole).toInt();
				_selectedTaskID = taskID;
				loadTaskToEditor(taskID);
				refreshSubmitPanel();
			});
		connect(taskActionDelegate, &TaskActionDelegate::resultClicked, this, [this](int row)
			{
				QTableWidgetItem* item = _taskList->item(row, 2);
				quint64 runID = item != nullptr ? item->data(Qt::UserRole + 1).toULongLong() : 0;
				showResultActions(runID);
			});
		//NoSelection下选中信号不再触发，点击行（操作列除外）更新"当前任务"
		connect(_taskList, &QTableWidget::cellClicked, this, [this](int row, int column)
			{
				Q_UNUSED(column);
				QTableWidgetItem* item = _taskList->item(row, 0);
				if (item == nullptr)
				{
					return;
				}
				_selectedTaskID = item->data(Qt::UserRole).toInt();
				refreshSubmitPanel();
			});
		listLayout->addWidget(_taskList, 1);

		QGroupBox* submitGroup = new QGroupBox(rightWidget);
		QVBoxLayout* submitLayout = new QVBoxLayout(submitGroup);
		submitLayout->setContentsMargins(0, 0, 0, 0);
		QHBoxLayout* submitTitleLayout = new QHBoxLayout();
		submitTitleLayout->addWidget(sectionLabel("提交求解", submitGroup));
		submitTitleLayout->addStretch(1);
		submitLayout->addLayout(submitTitleLayout);
		QFormLayout* submitInfoLayout = new QFormLayout();
		QComboBox* solverCombo = new QComboBox(submitGroup);
		solverCombo->addItem("本地求解器");
		submitInfoLayout->addRow("求解器：", solverCombo);
		_curTaskLabel = new QLabel("未选择任务", submitGroup);
		_solverFileLabel = new QLabel("--", submitGroup);
		_solverFileLabel->setWordWrap(true);
		submitInfoLayout->addRow("当前任务：", _curTaskLabel);
		submitInfoLayout->addRow("计算文件：", _solverFileLabel);
		submitLayout->addLayout(submitInfoLayout);
		_autoOpenPostCheck = new QCheckBox("求解完成后自动打开后处理", submitGroup);
		connect(_autoOpenPostCheck, &QCheckBox::toggled, this, &mSolveTaskDialog::slot_autoOpenPostToggled);
		submitLayout->addWidget(_autoOpenPostCheck);
		QHBoxLayout* submitBtnLayout = new QHBoxLayout();
		_writeFileBtn = new QPushButton("写出文件", submitGroup);
		_submitBtn = new QPushButton("提交求解", submitGroup);
		_submitBtn->setStyleSheet(kPrimaryButtonStyle);
		_submitBtn->setEnabled(false);
		connect(_writeFileBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_writeFile);
		connect(_submitBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_submitTask);
        submitBtnLayout->addStretch(1);
		submitBtnLayout->addWidget(_writeFileBtn);
		submitBtnLayout->addWidget(_submitBtn);
		submitLayout->addLayout(submitBtnLayout);

		rightLayout->addWidget(listGroup, 1);
		rightLayout->addWidget(hSeparator(rightWidget));
		rightLayout->addWidget(submitGroup);

		//三栏强制等宽：Ignored尺寸策略使各栏不受内容尺寸影响，按stretch均分
		leftWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		editorWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		rightWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

		//栏间竖分割线
		QFrame* vLine1 = new QFrame(page);
		vLine1->setFrameShape(QFrame::VLine);
		vLine1->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
		vLine1->setFixedWidth(1);
		vLine1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		QFrame* vLine2 = new QFrame(page);
		vLine2->setFrameShape(QFrame::VLine);
		vLine2->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
		vLine2->setFixedWidth(1);
		vLine2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

		columnsLayout->addWidget(leftWidget, 1);
		columnsLayout->addWidget(vLine1);
		columnsLayout->addWidget(editorWidget, 1);
		columnsLayout->addWidget(vLine2);
		columnsLayout->addWidget(rightWidget, 1);
		return page;
	}

	QWidget* mSolveTaskDialog::buildRecordsPage()
	{
		QWidget* page = new QWidget(this);
		QHBoxLayout* columnsLayout = new QHBoxLayout(page);
		columnsLayout->setContentsMargins(0, 0, 0, 0);
		columnsLayout->setSpacing(8);

		/******************** 任务队列 ********************/
		QGroupBox* queueGroup = new QGroupBox(page);
		QVBoxLayout* queueLayout = new QVBoxLayout(queueGroup);
		QHBoxLayout* queueTitleLayout = new QHBoxLayout();
		queueTitleLayout->addWidget(sectionLabel("任务队列", queueGroup));
		queueTitleLayout->addStretch(1);
		queueLayout->addLayout(queueTitleLayout);
		_queueTable = new QTableWidget(0, 5, queueGroup);
		_queueTable->setHorizontalHeaderLabels(QStringList() << "队位" << "任务名称" << "核数" << "状态" << "已运行");
		_queueTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		//表头统一左对齐（与内容一致）
		for (int col = 0; col < 5; ++col)
		{
			_queueTable->horizontalHeaderItem(col)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		}
		_queueTable->verticalHeader()->setDefaultSectionSize(34);
		_queueTable->setShowGrid(false);
		_queueTable->setAlternatingRowColors(true);
		_queueTable->verticalHeader()->hide();
		_queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		_queueTable->setSelectionMode(QAbstractItemView::SingleSelection);
		_queueTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		connect(_queueTable, &QTableWidget::itemSelectionChanged, this, [this]()
			{
				int row = _queueTable->currentRow();
				QTableWidgetItem* item = row >= 0 ? _queueTable->item(row, 0) : nullptr;
				_selectedQueueRunID = item != nullptr ? item->data(Qt::UserRole).toULongLong() : 0;
			});
		queueLayout->addWidget(_queueTable, 1);
		QHBoxLayout* queueBtnLayout = new QHBoxLayout();
		QPushButton* queueUpBtn = new QPushButton("上移", queueGroup);
        queueUpBtn->setFixedWidth(45);
		QPushButton* queueDownBtn = new QPushButton("下移", queueGroup);
        queueDownBtn->setFixedWidth(45);
		QPushButton* queueLogBtn = new QPushButton("查看日志", queueGroup);
        queueLogBtn->setFixedWidth(75);
		QPushButton* queueActionBtn = new QPushButton("取消求解", queueGroup);
        queueActionBtn->setFixedWidth(75);
		connect(queueUpBtn, &QPushButton::clicked, this, [this]() { slot_queueMove(-1); });
		connect(queueDownBtn, &QPushButton::clicked, this, [this]() { slot_queueMove(1); });
		connect(queueLogBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_queueLog);
		connect(queueActionBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_queueAction);
		queueBtnLayout->addWidget(queueUpBtn);
		queueBtnLayout->addWidget(queueDownBtn);
        queueBtnLayout->addStretch(1);
		queueBtnLayout->addWidget(queueLogBtn);
		queueBtnLayout->addWidget(queueActionBtn);
		queueLayout->addLayout(queueBtnLayout);

		/******************** 已完成任务 ********************/
		QGroupBox* doneGroup = new QGroupBox(page);
		QVBoxLayout* doneLayout = new QVBoxLayout(doneGroup);
		QHBoxLayout* doneTitleLayout = new QHBoxLayout();
		doneTitleLayout->addWidget(sectionLabel("已完成任务", doneGroup));
		doneTitleLayout->addStretch(1);
		doneLayout->addLayout(doneTitleLayout);
		_completedTable = new QTableWidget(0, 7, doneGroup);
		_completedTable->setHorizontalHeaderLabels(QStringList() << "任务名称" << "工况" << "核数" << "开始时间" << "结束时间" << "耗时" << "操作");
		//任务名称占剩余宽度；工况/核数窄列；时间/耗时/操作平分固定宽；禁用横向滚动
		_completedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		_completedTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
		_completedTable->setColumnWidth(1, 40);
		_completedTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
		_completedTable->setColumnWidth(2, 40);
		_completedTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
		_completedTable->setColumnWidth(3, 72);
		_completedTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
		_completedTable->setColumnWidth(4, 72);
		_completedTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
		_completedTable->setColumnWidth(5, 76);
		_completedTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
		_completedTable->setColumnWidth(6, 100);
		_completedTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		//表头统一左对齐（与内容一致）
		for (int col = 0; col < 7; ++col)
		{
			_completedTable->horizontalHeaderItem(col)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		}
		//操作列（日志/结果）delegate自绘，一次性挂载
		TaskCompletedDelegate* completedDelegate = new TaskCompletedDelegate(_completedTable);
		_completedTable->setItemDelegateForColumn(6, completedDelegate);
		connect(completedDelegate, &TaskCompletedDelegate::logClicked, this, [this](int row)
			{
				QTableWidgetItem* item = _completedTable->item(row, 6);
				quint64 runID = item != nullptr ? item->data(Qt::UserRole + 1).toULongLong() : 0;
				if (runID == 0)
				{
					return;
				}
				openLogTab(runID);
			});
		connect(completedDelegate, &TaskCompletedDelegate::resultClicked, this, [this](int row)
			{
				QTableWidgetItem* item = _completedTable->item(row, 6);
				quint64 runID = item != nullptr ? item->data(Qt::UserRole + 1).toULongLong() : 0;
				showResultActions(runID);
			});
		_completedTable->verticalHeader()->setDefaultSectionSize(34);
		_completedTable->setShowGrid(false);
		_completedTable->setAlternatingRowColors(true);
		_completedTable->verticalHeader()->hide();
		_completedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		_completedTable->setSelectionMode(QAbstractItemView::SingleSelection);
		_completedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		doneLayout->addWidget(_completedTable, 1);
		QHBoxLayout* doneBtnLayout = new QHBoxLayout();
		doneBtnLayout->addStretch(1);
		QPushButton* deleteRecordBtn = new QPushButton("删除记录", doneGroup);
		connect(deleteRecordBtn, &QPushButton::clicked, this, &mSolveTaskDialog::slot_deleteRecord);
		doneBtnLayout->addWidget(deleteRecordBtn);
		doneLayout->addLayout(doneBtnLayout);

		//两栏等宽 + 中间竖分隔线（与任务配置页同风格）
		queueGroup->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		doneGroup->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		QFrame* recordsVLine = new QFrame(page);
		recordsVLine->setFrameShape(QFrame::VLine);
		recordsVLine->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
		recordsVLine->setFixedWidth(1);
		recordsVLine->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

		columnsLayout->addWidget(queueGroup, 1);
		columnsLayout->addWidget(recordsVLine);
		columnsLayout->addWidget(doneGroup, 1);
		return page;
	}

	/******************** 左栏：工况列表 ********************/

	void mSolveTaskDialog::refreshCaseTypeFilter()
	{
		//类型筛选按分类：静态/隐式动力学/稳定性/传热/线性动态（与类型列显示一致）
		QString keep = _caseTypeFilter->currentText();
		_caseTypeFilter->blockSignals(true);
		_caseTypeFilter->clear();
		_caseTypeFilter->addItem("全部类型");
		_caseTypeFilter->addItem("静态");
		_caseTypeFilter->addItem("隐式动力学");
		_caseTypeFilter->addItem("稳定性");
		_caseTypeFilter->addItem("传热");
		_caseTypeFilter->addItem("线性动态");
		int index = _caseTypeFilter->findText(keep);
		_caseTypeFilter->setCurrentIndex(index >= 0 ? index : 0);
		_caseTypeFilter->blockSignals(false);
	}

	void mSolveTaskDialog::refreshCaseList()
	{
		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(_modelID);
		if (viewportModel == nullptr || viewportModel->getModelData() == nullptr)
		{
			return;
		}
		mModelData* modelData = viewportModel->getModelData();

		const QString key = _caseSearch->text().trimmed();
		const bool filterType = _caseTypeFilter->currentIndex() > 0;
		const QString typeText = _caseTypeFilter->currentText();

		//已在任务编辑表中的工况，行内按钮显示"已添加"并禁用
		QSet<int> editingIDs;
		for (int row = 0; row < _caseTable->rowCount(); ++row)
		{
			editingIDs.insert(_caseTable->item(row, 0)->data(Qt::UserRole).toInt());
		}

		_caseList->setRowCount(0);
		for (int caseID : modelData->getAllPhyID(Physical_Case))
		{
			QString caseName = modelData->getNameByPhyID(Physical_Case, caseID);
			mCaseBase* caseBase = modelData->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
			if (caseBase == nullptr)
			{
				continue;
			}
			MxFunctions::CaseType caseType = caseBase->getCaseType();
			//类型列显示所属分类（如超单元->线性动态分析、线性静力->静态分析）
			QString typeName = caseTypeCategory(caseType);
			if (!key.isEmpty() && !caseName.contains(key, Qt::CaseInsensitive))
			{
				continue;
			}
			if (filterType && caseTypeCategory(caseType) != typeText)
			{
				continue;
			}
			int row = _caseList->rowCount();
			_caseList->insertRow(row);
			//复选框（已在任务中的工况禁用：不可勾选）
			bool alreadyAdded = editingIDs.contains(caseID);
			QTableWidgetItem* checkItem = new QTableWidgetItem();
			checkItem->setFlags(alreadyAdded ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable)
				: (Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
			checkItem->setCheckState(Qt::Unchecked);
			checkItem->setData(Qt::UserRole, caseID);
			_caseList->setItem(row, 0, checkItem);
			//工况名称（左）
			QTableWidgetItem* nameItem = new QTableWidgetItem(caseName);
			nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			_caseList->setItem(row, 1, nameItem);
			//工况类型（右，弱化灰字）
			QTableWidgetItem* typeItem = new QTableWidgetItem(typeName);
			typeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			typeItem->setTextAlignment(Qt::AlignCenter);
			typeItem->setForeground(QColor(0x78, 0x83, 0x91));
			_caseList->setItem(row, 2, typeItem);
			//添加按钮列：数据项（UserRole+1存"是否已添加"，由CaseAddButtonDelegate自绘）
			QTableWidgetItem* btnItem = new QTableWidgetItem();
			btnItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			btnItem->setData(Qt::UserRole + 1, editingIDs.contains(caseID));
			_caseList->setItem(row, 3, btnItem);
	}
	updateSelectAllBtnText();
	}

	void mSolveTaskDialog::slot_filterChanged()
	{
		refreshCaseList();
	}

	void mSolveTaskDialog::slot_toggleCaseSelectAll()
	{
		//左栏全选：作用于工况列表（全选/取消全选当前可见可勾选的工况，已添加行跳过）
		bool allChecked = true;
		for (int row = 0; row < _caseList->rowCount(); ++row)
		{
			QTableWidgetItem* item = _caseList->item(row, 0);
			if (!(item->flags() & Qt::ItemIsUserCheckable))
			{
				continue;
			}
			if (item->checkState() == Qt::Unchecked)
			{
				allChecked = false;
				break;
			}
		}
		for (int row = 0; row < _caseList->rowCount(); ++row)
		{
			QTableWidgetItem* item = _caseList->item(row, 0);
			if (item->flags() & Qt::ItemIsUserCheckable)
			{
				item->setCheckState(allChecked ? Qt::Unchecked : Qt::Checked);
			}
		}
		updateSelectAllBtnText();
	}

	void mSolveTaskDialog::updateSelectAllBtnText()
	{
		//按两张表的勾选态切换按钮文字（全部勾选->取消全选，否则->全选）
		auto allChecked = [](QTableWidget* table) 
		{
			if (table->rowCount() == 0)
			{
				return false;
			}
			for (int row = 0; row < table->rowCount(); ++row)
			{
				if (table->item(row, 0) == nullptr || table->item(row, 0)->checkState() == Qt::Unchecked)
				{
					return false;
				}
			}
			return true;
		};
		if (_caseListSelectAllBtn != nullptr)
		{
			_caseListSelectAllBtn->setText(allChecked(_caseList) ? "取消全选" : "全选");
		}
		if (_taskCaseSelectAllBtn != nullptr)
		{
			_taskCaseSelectAllBtn->setText(allChecked(_caseTable) ? "取消全选" : "全选");
		}
	}

	void mSolveTaskDialog::slot_toggleSelectAll()
	{
		//作用于任务工况表格：全选/取消全选行复选框
		bool allChecked = true;
		for (int row = 0; row < _caseTable->rowCount(); ++row)
		{
			if (_caseTable->item(row, 0)->checkState() == Qt::Unchecked)
			{
				allChecked = false;
				break;
			}
		}
		for (int row = 0; row < _caseTable->rowCount(); ++row)
		{
			_caseTable->item(row, 0)->setCheckState(allChecked ? Qt::Unchecked : Qt::Checked);
		}
		updateSelectAllBtnText();
	}

	void mSolveTaskDialog::slot_addSelectedCases()
	{
		//已在编辑表中的工况不重复加入
		QSet<int> existIDs;
		for (int row = 0; row < _caseTable->rowCount(); ++row)
		{
			existIDs.insert(_caseTable->item(row, 0)->data(Qt::UserRole).toInt());
		}
		int added = 0;
		for (int row = 0; row < _caseList->rowCount(); ++row)
		{
			QTableWidgetItem* item = _caseList->item(row, 0);
			if (item->checkState() != Qt::Checked)
			{
				continue;
			}
			int caseID = item->data(Qt::UserRole).toInt();
			if (existIDs.contains(caseID))
			{
				continue;
			}
			appendCaseRow(caseID, false);
			item->setCheckState(Qt::Unchecked);
			updateCaseRowAddState(caseID, true);
			added++;
		}
		if (added == 0)
		{
			QMessageBox::information(this, "加入任务", "请先勾选工况（已在任务中的不再重复加入）。");
		}
	}

	void mSolveTaskDialog::slot_quickSolve()
	{
		//一键求解调用"提交计算"接口（与菜单/信息窗口的提交计算一致：
		//写文件（旧目录规则）-> 自动创建Solve_工况名快速任务 -> 提交执行中心）
		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(_modelID);
		if (viewportModel == nullptr || viewportModel->getModelData() == nullptr)
		{
			return;
		}
		std::vector<int> caseIDs = viewportModel->getModelData()->getAllPhyID(Physical_Case);
		if (caseIDs.empty())
		{
			QMessageBox::information(this, "一键求解", "当前模型没有分析工况！");
			return;
		}
		mSolveTaskCenter::getInstance()->quickSolve(_modelID, caseIDs[0]);
	}

	/******************** 中栏：任务编辑 ********************/

	void mSolveTaskDialog::appendCaseRow(int caseID, bool continued)
	{
		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(_modelID);
		QString caseName = QString("工况（ID:%1）").arg(caseID);
		QString typeName;
		if (viewportModel != nullptr && viewportModel->getModelData() != nullptr)
		{
			caseName = viewportModel->getModelData()->getNameByPhyID(Physical_Case, caseID);
			mCaseBase* caseBase = viewportModel->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
			if (caseBase != nullptr)
			{
				typeName = caseTypeCategory(caseBase->getCaseType());
			}
		}
		int row = _caseTable->rowCount();
		_caseTable->insertRow(row);
		//选择（复选框，批量操作依据；UserRole存工况ID）
		QTableWidgetItem* checkItem = new QTableWidgetItem();
		checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		checkItem->setCheckState(Qt::Unchecked);
		checkItem->setData(Qt::UserRole, caseID);
		_caseTable->setItem(row, 0, checkItem);
		//工况名称
		QTableWidgetItem* nameItem = new QTableWidgetItem(caseName);
		nameItem->setFlags((nameItem->flags() & ~Qt::ItemIsEditable) | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		_caseTable->setItem(row, 1, nameItem);
		//工况关系：数据项（UserRole存关系值，由RelationComboDelegate自绘；可编辑以支持弹出下拉）
		QTableWidgetItem* relationItem = new QTableWidgetItem();
		relationItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		relationItem->setData(Qt::UserRole, row == 0 ? 0 : (continued ? 1 : 0));
		_caseTable->setItem(row, 2, relationItem);
		//类型（所属分类，弱化灰字）
		QTableWidgetItem* typeItem = new QTableWidgetItem(typeName);
		typeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		typeItem->setTextAlignment(Qt::AlignCenter);
		typeItem->setForeground(QColor(0x78, 0x83, 0x91));
		_caseTable->setItem(row, 3, typeItem);
	}

	void mSolveTaskDialog::updateCaseRowAddState(int caseID, bool added)
	{
		for (int row = 0; row < _caseList->rowCount(); ++row)
		{
			QTableWidgetItem* checkItem = _caseList->item(row, 0);
			if (checkItem != nullptr && checkItem->data(Qt::UserRole).toInt() == caseID)
			{
				//只改数据，delegate自绘随之刷新（不动表格结构）；
				//已添加行的checkbox禁用（取消勾选并去掉可勾选标志，保留可选中以支持行高亮）
				_caseList->item(row, 3)->setData(Qt::UserRole + 1, added);
				checkItem->setCheckState(Qt::Unchecked);
				checkItem->setFlags(added ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable)
					: (Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
				return;
			}
		}
	}

	void mSolveTaskDialog::refreshCaseTable()
	{
		//整表重建（载入任务/清空编辑器时使用）
		while (_caseTable->rowCount() > 0)
		{
			_caseTable->removeRow(0);
		}
		updateSelectAllBtnText();
	}

	void mSolveTaskDialog::swapCaseRows(int rowA, int rowB)
	{
		//交换两行全部内容：勾选态/工况ID（列0）、名称（列1）、关系（列2）、类型（列3）
		QTableWidgetItem* checkA = _caseTable->item(rowA, 0);
		QTableWidgetItem* checkB = _caseTable->item(rowB, 0);
		Qt::CheckState stateA = checkA->checkState();
		int idA = checkA->data(Qt::UserRole).toInt();
		checkA->setCheckState(checkB->checkState());
		checkA->setData(Qt::UserRole, checkB->data(Qt::UserRole).toInt());
		checkB->setCheckState(stateA);
		checkB->setData(Qt::UserRole, idA);

		QString nameA = _caseTable->item(rowA, 1)->text();
		_caseTable->item(rowA, 1)->setText(_caseTable->item(rowB, 1)->text());
		_caseTable->item(rowB, 1)->setText(nameA);

		int relA = _caseTable->item(rowA, 2)->data(Qt::UserRole).toInt();
		_caseTable->item(rowA, 2)->setData(Qt::UserRole, _caseTable->item(rowB, 2)->data(Qt::UserRole).toInt());
		_caseTable->item(rowB, 2)->setData(Qt::UserRole, relA);

		QString typeA = _caseTable->item(rowA, 3)->text();
		_caseTable->item(rowA, 3)->setText(_caseTable->item(rowB, 3)->text());
		_caseTable->item(rowB, 3)->setText(typeA);

		//交换后首行固定独立计算
		_caseTable->item(0, 2)->setData(Qt::UserRole, 0);
	}

	QVector<int> mSolveTaskDialog::checkedCaseRows() const
	{
		QVector<int> rows;
		for (int row = 0; row < _caseTable->rowCount(); ++row)
		{
			if (_caseTable->item(row, 0) != nullptr && _caseTable->item(row, 0)->checkState() == Qt::Checked)
			{
				rows.push_back(row);
			}
		}
		return rows;
	}

	void mSolveTaskDialog::slot_moveUp()
	{
		QVector<int> rows = checkedCaseRows();
		if (rows.isEmpty())
		{
			QMessageBox::information(this, "上移", "请先勾选要移动的工况。");
			return;
		}
		std::sort(rows.begin(), rows.end());
		for (int row : rows)
		{
			if (row > 0)
			{
				swapCaseRows(row, row - 1);
			}
		}
	}

	void mSolveTaskDialog::slot_moveDown()
	{
		QVector<int> rows = checkedCaseRows();
		if (rows.isEmpty())
		{
			QMessageBox::information(this, "下移", "请先勾选要移动的工况。");
			return;
		}
		std::sort(rows.begin(), rows.end(), std::greater<int>());
		for (int row : rows)
		{
			if (row < _caseTable->rowCount() - 1)
			{
				swapCaseRows(row, row + 1);
			}
		}
	}

	void mSolveTaskDialog::slot_removeCases()
	{
		QVector<int> rows = checkedCaseRows();
		if (rows.isEmpty())
		{
			QMessageBox::information(this, "删除", "请先勾选要删除的工况。");
			return;
		}
		std::sort(rows.begin(), rows.end(), std::greater<int>());
		for (int row : rows)
		{
			//同步恢复工况列表对应行的"添加"按钮状态
			int caseID = _caseTable->item(row, 0)->data(Qt::UserRole).toInt();
			updateCaseRowAddState(caseID, false);
			_caseTable->removeRow(row);
		}
		//删除后首行固定独立计算（原第二行的关系值可能带着接续态上来）
		if (_caseTable->rowCount() > 0)
		{
			_caseTable->item(0, 2)->setData(Qt::UserRole, 0);
		}
        else 
        {
            _taskCaseSelectAllBtn->setText("全选");
        }
	}

	void mSolveTaskDialog::slot_applyContinuation()
	{
		if (_caseTable->rowCount() < 2)
		{
			return;
		}
		//作用范围：勾选行（无勾选则为全部行），首行除外
		QVector<int> rows = checkedCaseRows();
		if (rows.isEmpty())
		{
			for (int row = 1; row < _caseTable->rowCount(); ++row)
			{
				rows.push_back(row);
			}
		}
		else
		{
			rows.erase(std::remove_if(rows.begin(), rows.end(), [](int row) { return row == 0; }), rows.end());
		}
		if (rows.isEmpty())
		{
			return;
		}
		//判断目标是否已全部接续（是则取消接续，否则设置接续）
		bool allContinued = true;
		for (int row : rows)
		{
			if (_caseTable->item(row, 2)->data(Qt::UserRole).toInt() == 0)
			{
				allContinued = false;
				break;
			}
		}
		for (int row : rows)
		{
			_caseTable->item(row, 2)->setData(Qt::UserRole, 1);
		}
	}

	void mSolveTaskDialog::slot_chooseOutputDir()
	{
		QString current = _customOutputDir.isEmpty()
			? mAppConfiguration::getInstance()->_workingDir : _customOutputDir;
		QString dir = QFileDialog::getExistingDirectory(this, "选择输出目录", current);
		if (!dir.isEmpty())
		{
			_customOutputDir = QDir(dir).absolutePath();
			refreshOutputDirLabel();
		}
	}

	void mSolveTaskDialog::slot_resetOutputDir()
	{
		_customOutputDir.clear();
		refreshOutputDirLabel();
	}

	void mSolveTaskDialog::refreshOutputDirLabel()
	{
		if (!_customOutputDir.isEmpty())
		{
			_outputDirEdit->setText(_customOutputDir);
		}
		else
		{
			_outputDirEdit->setText(QString("%1/%2/%3").arg(
				mAppConfiguration::getInstance()->_workingDir,
				mSolveTaskCenter::getInstance()->modelDirToken(_modelID),
				_taskNameEdit->text().trimmed()));
		}
	}

	void mSolveTaskDialog::slot_newTask()
	{
		clearEditor();
	}

	void mSolveTaskDialog::clearEditor()
	{
		_editingTaskID = -1;
		_taskNameEdit->setText(taskManager() != nullptr ? taskManager()->createUniqueName("Task") : "Task_001");
		refreshCaseTable();
		_cpuSpin->setValue(8);
		_memoryCombo->setCurrentIndex(2);
		_customOutputDir.clear();
		refreshOutputDirLabel();
		//标题回到"新建任务"
		QLabel* titleLabel = findChild<QLabel*>("editorTitleLabel");
		if (titleLabel != nullptr)
		{
			titleLabel->setText("新建任务");
		}
		//编辑器清空后复位工况列表"已添加"状态；
		//延迟到信号栈外执行——在按钮信号栈内整表重建会引起行内按钮几何异常
		QTimer::singleShot(0, this, [this]() { refreshCaseList(); });
	}

	void mSolveTaskDialog::loadTaskToEditor(int taskID)
	{
		mSolveTaskData* task = taskManager() != nullptr ? taskManager()->getTask(taskID) : nullptr;
		if (task == nullptr)
		{
			return;
		}
		_editingTaskID = taskID;
		_taskNameEdit->setText(task->getTaskName());
		refreshCaseTable();
		for (int caseID : task->getCaseIDs())
		{
			appendCaseRow(caseID, task->getCaseRelation(caseID) == Relation_Continue);
		}
		_cpuSpin->setValue(task->getCpuCores() > 0 ? task->getCpuCores() : 8);
		_memoryCombo->setCurrentIndex((int)task->getMemoryStrategy());
		_customOutputDir = task->getOutputDir();
		refreshOutputDirLabel();
		//标题变为"修改任务"
		QLabel* titleLabel = findChild<QLabel*>("editorTitleLabel");
		if (titleLabel != nullptr)
		{
			titleLabel->setText("修改任务");
		}
		//按载入任务的工况刷新"已添加"状态；延迟到信号栈外执行（同上）
		QTimer::singleShot(0, this, [this]() { refreshCaseList(); });
	}

	bool mSolveTaskDialog::collectEditor(QString& errMsg)
	{
		QString name = _taskNameEdit->text().trimmed();
		if (name.isEmpty())
		{
			errMsg = "任务名称不能为空！";
			return false;
		}
		if (_caseTable->rowCount() == 0)
		{
			errMsg = "任务中至少需要一个工况！";
			return false;
		}
		if (taskManager() == nullptr)
		{
			errMsg = "任务管理器不可用！";
			return false;
		}
		//名称唯一性（排除自身）
		mSolveTaskData* exist = taskManager()->getTaskByName(name);
		if (exist != nullptr && exist->getTaskID() != _editingTaskID)
		{
			errMsg = QString("任务名\"%1\"已存在！").arg(name);
			return false;
		}

		mSolveTaskData* task = nullptr;
		if (_editingTaskID == -1)
		{
			task = taskManager()->createTask(name);
			if (task == nullptr)
			{
				errMsg = "任务创建失败！";
				return false;
			}
			_editingTaskID = task->getTaskID();
		}
		else
		{
			task = taskManager()->getTask(_editingTaskID);
			if (task == nullptr)
			{
				errMsg = "任务不存在（可能已被删除）！";
				return false;
			}
			if (task->getTaskName() != name)
			{
				taskManager()->renameTask(_editingTaskID, name);
			}
		}

		QVector<int> caseIDs;
		for (int row = 0; row < _caseTable->rowCount(); ++row)
		{
			int caseID = _caseTable->item(row, 0)->data(Qt::UserRole).toInt();
			caseIDs.push_back(caseID);
			//关系值存于列2的UserRole（首行固定独立计算）
			if (row == 0)
			{
				task->setCaseRelation(caseID, Relation_Independent);
			}
			else
			{
				task->setCaseRelation(caseID, (CaseRelation)(_caseTable->item(row, 2)->data(Qt::UserRole).toInt()));
			}
		}
		task->setCaseIDs(caseIDs);
		task->setCpuCores(_cpuSpin->value());
		task->setMemoryStrategy((MemoryStrategy)_memoryCombo->currentIndex());
		task->setOutputDir(_customOutputDir);
		return true;
	}

	void mSolveTaskDialog::slot_saveTask()
	{
		QString errMsg;
		if (!collectEditor(errMsg))
		{
			QMessageBox::warning(this, "保存任务", errMsg);
			return;
		}
		_selectedTaskID = _editingTaskID;
		//保存后进入"修改任务"模式
		QLabel* titleLabel = findChild<QLabel*>("editorTitleLabel");
		if (titleLabel != nullptr)
		{
			titleLabel->setText("修改任务");
		}
		refreshTaskList();
	}

	/******************** 右栏：任务列表与提交 ********************/

	void mSolveTaskDialog::slot_taskListSelectionChanged()
	{
		int row = _taskList->currentRow();
		QTableWidgetItem* item = row >= 0 ? _taskList->item(row, 0) : nullptr;
		_selectedTaskID = item != nullptr ? item->data(Qt::UserRole).toInt() : -1;
		refreshSubmitPanel();
	}

	void mSolveTaskDialog::slot_editTask()
	{
		//由任务列表每行的"设置"按钮触发（携带taskID），保留给旧入口
		if (_selectedTaskID == -1)
		{
			QMessageBox::information(this, "设置", "请先在任务列表中选择任务。");
			return;
		}
		loadTaskToEditor(_selectedTaskID);
	}

	void mSolveTaskDialog::slot_deleteTask()
	{
		if (_selectedTaskID == -1)
		{
			QMessageBox::information(this, "删除任务", "请先在任务列表中选择任务。");
			return;
		}
		if (activeRunOfTask(_selectedTaskID) != nullptr)
		{
			QMessageBox::warning(this, "删除任务", "任务正在排队或求解，请先取消/中断后再删除。");
			return;
		}
		//先清求解记录再删任务对象：run的配置委托task指针读取，task先删会让清记录过程悬空访问
		mSolveTaskCenter::getInstance()->removeRunsOfTask(_modelID, _selectedTaskID);
		taskManager()->removeTask(_selectedTaskID);
		if (_editingTaskID == _selectedTaskID)
		{
			clearEditor();
		}
		_selectedTaskID = -1;
		refreshTaskList();
		refreshCompletedTable();
	}

	void mSolveTaskDialog::slot_writeFile()
	{
		if (_selectedTaskID == -1)
		{
			QMessageBox::information(this, "写出文件", "请先在任务列表中选择任务。");
			return;
		}
		writeTaskFile(_selectedTaskID, false);
	}

	void mSolveTaskDialog::writeTaskFile(int taskID, bool autoSubmit)
	{
		mSolveTaskData* task = taskManager() != nullptr ? taskManager()->getTask(taskID) : nullptr;
		if (task == nullptr || _writeThread != nullptr)
		{
			return;
		}
		if (activeRunOfTask(taskID) != nullptr)
		{
			QMessageBox::information(this, "写出文件", "任务正在排队或求解中。");
			return;
		}
		//输出目录与执行中心共用同一计算（提交校验的文件路径即此处写出的路径）
		QString solverFile = mSolveTaskCenter::taskSolverFile(task, _modelID);
		QDir outputDir = QFileInfo(solverFile).absoluteDir();
		if (!outputDir.exists())
		{
			outputDir.mkpath(".");
		}
		_writeTaskID = taskID;
		_autoSubmitAfterWrite = autoSubmit;
		_writeFilePath = solverFile;
		_writeThread = new mOThirdFile(solverFile);
		//任务里有哪些工况就写哪些（不设置过滤则写全部工况）
		//注意：getCaseIDs按值返回，必须先存局部变量再取迭代器（两次调用返回两个不同临时对象）
		QVector<int> taskCaseIDs = task->getCaseIDs();
		std::vector<int> caseFilter(taskCaseIDs.begin(), taskCaseIDs.end());
		_writeThread->setCaseFilter(caseFilter);
		connect(_writeThread, SIGNAL(finished()), this, SLOT(slot_writeThreadFinished()));
		_writeThread->start();
		refreshSubmitPanel();
	}

	void mSolveTaskDialog::slot_writeThreadFinished()
	{
		if (_writeThread == nullptr)
		{
			return;
		}
		bool writeSuccess = _writeThread->isWriteSuccess();
		delete _writeThread;
		_writeThread = nullptr;

		int taskID = _writeTaskID;
		bool autoSubmit = _autoSubmitAfterWrite;
		_writeTaskID = -1;
		_autoSubmitAfterWrite = false;

		mSolveTaskData* task = taskManager() != nullptr ? taskManager()->getTask(taskID) : nullptr;
		if (task == nullptr)
		{
			refreshSubmitPanel();
			return;
		}
		if (!writeSuccess)
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("任务\"%1\"计算文件生成失败！").arg(task->getTaskName()));
			refreshSubmitPanel();
			return;
		}
		task->setSolverFile(_writeFilePath);
		_selectedTaskID = taskID;
		refreshTaskList();
		refreshSubmitPanel();
		if (autoSubmit)
		{
			//目录是本次写出流程刚创建的，自动提交不再弹覆盖确认
			_skipDirCheckOnce = true;
			slot_submitTask();
		}
	}

	void mSolveTaskDialog::slot_submitTask()
	{
		mSolveTaskData* task = selectedTask();
		if (task == nullptr)
		{
			return;
		}
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		//排队中→取消；求解中→中断
		if (mSolveRunRecord* run = activeRunOfTask(task->getTaskID()))
		{
			if (run->getStatus() == mSolveRunRecord::Status_Running)
			{
				center->interruptRun(run->getRunID());
			}
			else
			{
				center->cancelRun(run->getRunID());
			}
			refreshTaskList();
			return;
		}
		bool skipDirCheck = _skipDirCheckOnce;
		_skipDirCheckOnce = false;

		if (!skipDirCheck)
		{
			QString solverFilePath = mSolveTaskCenter::taskSolverFile(task, _modelID);
			QDir targetDir = QFileInfo(solverFilePath).absoluteDir();
			if (targetDir.exists())
			{
				QMessageBox confirmBox(this);
				confirmBox.setWindowTitle("提交求解");
				confirmBox.setText(QString("目录\"%1\"已存在，是否覆盖当前目录？").arg(targetDir.absolutePath()));
				confirmBox.setIcon(QMessageBox::Question);
				QPushButton* okBtn = confirmBox.addButton("确定", QMessageBox::YesRole);
				confirmBox.addButton("取消", QMessageBox::NoRole);
				confirmBox.exec();
				if (confirmBox.clickedButton() != okBtn)
				{
					return;
				}
			}
		}

		//计算文件未写出时自动写出并在成功后自动提交（不再提示用户手动写出）
		if (task->getSolverFile().isEmpty() || !QFileInfo::exists(task->getSolverFile()))
		{
			writeTaskFile(task->getTaskID(), true);
			refreshSubmitPanel();
			return;
		}
		QString errMsg;
		if (center->submitTask(_modelID, task->getTaskID(), &errMsg) == 0)
		{
			QMessageBox::warning(this, "提交求解", errMsg);
		}
		refreshTaskList();
	}

	void mSolveTaskDialog::slot_autoOpenPostToggled(bool checked)
	{
		mSolveTaskData* task = selectedTask();
		if (task != nullptr)
		{
			task->setAutoOpenPost(checked);
		}
	}

	void mSolveTaskDialog::slot_runStatusChanged(quint64 runID)
	{
		mSolveRunRecord* run = mSolveTaskCenter::getInstance()->getRun(runID);
		if (run != nullptr && run->getModelID() == _modelID)
		{
			refreshTaskList();
			refreshQueueTable();
			refreshCompletedTable();
		}
	}

	/******************** 刷新与查询辅助 ********************/

	void mSolveTaskDialog::refreshTaskList()
	{
		//延迟到信号栈外重建：本函数常在按钮/进程信号栈内被调用，
		//栈内整表重建会引起行内控件（设置/结果按钮）几何异常
		QTimer::singleShot(0, this, [this]()
			{
		_taskList->blockSignals(true);
		_taskList->setRowCount(0);
		if (taskManager() != nullptr)
		{
			for (mSolveTaskData* task : taskManager()->getAllTask())
			{
				int row = _taskList->rowCount();
				_taskList->insertRow(row);
				int taskID = task->getTaskID();
				//任务 / 工况数
				QTableWidgetItem* nameItem = new QTableWidgetItem(QString("%1 · %2")
					.arg(task->getTaskName()).arg(task->getCaseIDs().size()));
				nameItem->setData(Qt::UserRole, taskID);
				nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
				_taskList->setItem(row, 0, nameItem);
				//状态 / 队位（按状态着色）
				QColor statusColor;
				QString statusText = taskDisplayStatus(taskID, &statusColor);
				QTableWidgetItem* statusItem = new QTableWidgetItem(statusText);
				statusItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				statusItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
				statusItem->setForeground(statusColor);
				_taskList->setItem(row, 1, statusItem);
				//操作列：设置（载入编辑器）+ 结果（查看最近一次成功结果）
				quint64 resultRunID = 0;
				for (mSolveRunRecord* run : mSolveTaskCenter::getInstance()->getRunsOfModel(_modelID))
				{
					if (run->getTaskID() == taskID && run->getStatus() == mSolveRunRecord::Status_Done
						&& (resultRunID == 0 || run->getRunID() > resultRunID))
					{
						resultRunID = run->getRunID();
					}
				}
				//操作列：数据项（UserRole+1存可查看的结果runID；UserRole+2存"待求解"可设置标志）
				QTableWidgetItem* actionItem = new QTableWidgetItem();
				actionItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				actionItem->setData(Qt::UserRole + 1, resultRunID);
				actionItem->setData(Qt::UserRole + 2, statusText == "待求解" || statusText == "已完成");
				_taskList->setItem(row, 2, actionItem);
				if (task->getTaskID() == _selectedTaskID)
				{
					_taskList->selectRow(row);
				}
			}
		}
		_taskList->blockSignals(false);
		refreshSubmitPanel();
			});
	}

	void mSolveTaskDialog::refreshSubmitPanel()
	{
		mSolveTaskData* task = selectedTask();
		_curTaskLabel->setText(task != nullptr ? task->getTaskName() : "未选择任务");

		bool fileReady = task != nullptr && !task->getSolverFile().isEmpty() && QFileInfo::exists(task->getSolverFile());
		_solverFileLabel->setText(task == nullptr ? QString("--")
			: (fileReady ? QFileInfo(task->getSolverFile()).fileName() : "未写出"));

		mSolveRunRecord* run = task != nullptr ? activeRunOfTask(task->getTaskID()) : nullptr;
		if (run != nullptr)
		{
			_submitBtn->setText(run->getStatus() == mSolveRunRecord::Status_Running ? "中断求解" : "取消求解");
			_writeFileBtn->setEnabled(false);
		}
		else
		{
			_submitBtn->setText("提交求解");
			_writeFileBtn->setEnabled(task != nullptr && _writeThread == nullptr);
		}
		_writeFileBtn->setText(_writeThread != nullptr ? "写出中..." : "写出文件");
		_submitBtn->setEnabled(task != nullptr);
		_autoOpenPostCheck->setEnabled(task != nullptr);
		if (task != nullptr)
		{
			_autoOpenPostCheck->blockSignals(true);
			_autoOpenPostCheck->setChecked(task->isAutoOpenPost());
			_autoOpenPostCheck->blockSignals(false);
		}
	}

	mSolveTaskManager* mSolveTaskDialog::taskManager() const
	{
		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(_modelID);
		if (viewportModel == nullptr || viewportModel->getModelData() == nullptr)
		{
			return nullptr;
		}
		return &viewportModel->getModelData()->getManager<mSolveTaskManager>(Physical_SolveTask);
	}

	mSolveTaskData* mSolveTaskDialog::editingTask() const
	{
		return (_editingTaskID != -1 && taskManager() != nullptr) ? taskManager()->getTask(_editingTaskID) : nullptr;
	}

	mSolveTaskData* mSolveTaskDialog::selectedTask() const
	{
		return (_selectedTaskID != -1 && taskManager() != nullptr) ? taskManager()->getTask(_selectedTaskID) : nullptr;
	}

	mSolveRunRecord* mSolveTaskDialog::activeRunOfTask(int taskID) const
	{
		for (mSolveRunRecord* run : mSolveTaskCenter::getInstance()->getActiveRunsOfModel(_modelID))
		{
			if (run->getTaskID() == taskID)
			{
				return run;
			}
		}
		return nullptr;
	}

	QString mSolveTaskDialog::taskDisplayStatus(int taskID, QColor* color) const
	{
		//状态配色（对齐原型）：求解中蓝/排队黄/完成绿/失败红/其余灰
		auto fill = [color](const char* hex)
		{
			if (color != nullptr)
			{
				*color = QColor(hex);
			}
		};
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		mSolveRunRecord* active = activeRunOfTask(taskID);
		if (active != nullptr)
		{
			if (active->getStatus() == mSolveRunRecord::Status_Running)
			{
				fill("#1890FF");
				return "求解中";
			}
			//排队位置（本模型队列内）
			int position = 0;
			int queuedSeen = 0;
			for (mSolveRunRecord* run : center->getActiveRunsOfModel(_modelID))
			{
				if (run->getStatus() == mSolveRunRecord::Status_Queued)
				{
					queuedSeen++;
					if (run->getRunID() == active->getRunID())
					{
						position = queuedSeen;
					}
				}
			}
			fill("#B7791F");
			return QString("排队中·第%1位").arg(position);
		}
		//最近一次结束记录
		mSolveRunRecord* last = nullptr;
		for (mSolveRunRecord* run : center->getRunsOfModel(_modelID))
		{
			if (run->getTaskID() == taskID && run->isFinished()
				&& (last == nullptr || run->getRunID() > last->getRunID()))
			{
				last = run;
			}
		}
		if (last != nullptr)
		{
			switch (last->getStatus())
			{
			case mSolveRunRecord::Status_Done:
				fill("#259B7A");
				return "已完成";
			case mSolveRunRecord::Status_Failed:
				fill("#D95151");
				return "失败";
			default: break;
			}
		}
		fill("#788391");
		return "待求解";
	}

	/******************** 求解记录页 ********************/

	void mSolveTaskDialog::refreshQueueTable()
	{
		if (_queueTable == nullptr)
		{
			return;
		}
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		const qint64 now = QDateTime::currentMSecsSinceEpoch();
		_queueTable->blockSignals(true);
		_queueTable->setRowCount(0);
		int queuedSeen = 0;
		for (mSolveRunRecord* run : center->getActiveRunsOfModel(_modelID))
		{
			const bool running = run->getStatus() == mSolveRunRecord::Status_Running;
			int position = running ? 0 : ++queuedSeen;
			int row = _queueTable->rowCount();
			_queueTable->insertRow(row);

			QTableWidgetItem* posItem = new QTableWidgetItem(running ? QString("—") : QString::number(position));
			posItem->setData(Qt::UserRole, run->getRunID());
			_queueTable->setItem(row, 0, posItem);

			_queueTable->setItem(row, 1, new QTableWidgetItem(run->getTaskName()));
			_queueTable->setItem(row, 2, new QTableWidgetItem(QString::number(run->getCpuCores())));
			_queueTable->setItem(row, 3, new QTableWidgetItem(running ? "求解中" : QString("排队中·第%1位").arg(position)));
			_queueTable->setItem(row, 4, new QTableWidgetItem(running ? formatElapsed(now - run->getStartedAt()) : QString("—")));

			if (run->getRunID() == _selectedQueueRunID)
			{
				_queueTable->selectRow(row);
			}
		}
		_queueTable->blockSignals(false);
	}

	void mSolveTaskDialog::refreshCompletedTable()
	{
		if (_completedTable == nullptr)
		{
			return;
		}
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		_completedTable->setRowCount(0);
		//按结束时间倒序（最近在前）；只记录成功完成的任务（中断/取消/失败不进完成表）
		QVector<mSolveRunRecord*> finished;
		for (mSolveRunRecord* run : center->getRunsOfModel(_modelID))
		{
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
			QTableWidgetItem* caseItem = new QTableWidgetItem(QString::number(run->getCaseIDs().size()));
			_completedTable->setItem(row, 1, caseItem);
			QTableWidgetItem* coreItem = new QTableWidgetItem(QString::number(run->getCpuCores()));
			_completedTable->setItem(row, 2, coreItem);
			_completedTable->setItem(row, 3, new QTableWidgetItem(formatClock(run->getStartedAt())));
			_completedTable->setItem(row, 4, new QTableWidgetItem(formatClock(run->getFinishedAt())));
			_completedTable->setItem(row, 5, new QTableWidgetItem(
				run->getStartedAt() > 0 ? formatElapsed(run->getFinishedAt() - run->getStartedAt()) : QString("—")));
			//操作列：数据项（UserRole+1存runID，由TaskCompletedDelegate自绘日志/结果按钮）
			QTableWidgetItem* actionItem = new QTableWidgetItem();
			actionItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			actionItem->setData(Qt::UserRole + 1, run->getRunID());
			_completedTable->setItem(row, 6, actionItem);
		}
	}

	void mSolveTaskDialog::slot_queueMove(int delta)
	{
		if (_selectedQueueRunID == 0)
		{
			QMessageBox::information(this, "调整顺序", "请先在任务队列中选择任务。");
			return;
		}
		mSolveTaskCenter::getInstance()->moveRunOrder(_selectedQueueRunID, delta);
		refreshQueueTable();
	}

	void mSolveTaskDialog::slot_queueLog()
	{
		if (_selectedQueueRunID == 0)
		{
			QMessageBox::information(this, "查看日志", "请先在任务队列中选择任务。");
			return;
		}
		openLogTab(_selectedQueueRunID);
	}

	void mSolveTaskDialog::slot_queueAction()
	{
		mSolveRunRecord* run = mSolveTaskCenter::getInstance()->getRun(_selectedQueueRunID);
		if (run == nullptr)
		{
			QMessageBox::information(this, "求解控制", "请先在任务队列中选择任务。");
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

	void mSolveTaskDialog::slot_deleteRecord()
	{
		int row = _completedTable->currentRow();
		QTableWidgetItem* item = row >= 0 ? _completedTable->item(row, 0) : nullptr;
		if (item == nullptr)
		{
			QMessageBox::information(this, "删除记录", "请先选择要删除的完成记录。");
			return;
		}
		quint64 runID = item->data(Qt::UserRole).toULongLong();
		if (mSolveTaskCenter::getInstance()->removeRun(runID))
		{
			refreshCompletedTable();
		}
	}

	void mSolveTaskDialog::slot_recordsTabChanged(int index)
	{
		//日志栏仅在"求解记录"页显示（常驻），其它页隐藏
		_logViewer->setVisible(index == 1);
		if (index == 1)
		{
			//显示时同步收起态的分割条尺寸（避免残留展开时的占比）
			if (_logsCollapsed)
			{
				_mainSplitter->setSizes(QList<int>() << 700 << 40);
			}
			refreshQueueTable();
			refreshCompletedTable();
		}
	}

	void mSolveTaskDialog::slot_updateElapsed()
	{
		//每秒刷新队列中运行任务的"已运行"列
		if (!isVisible() || _queueTable == nullptr)
		{
			return;
		}
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		const qint64 now = QDateTime::currentMSecsSinceEpoch();
		for (int row = 0; row < _queueTable->rowCount(); ++row)
		{
			QTableWidgetItem* posItem = _queueTable->item(row, 0);
			if (posItem == nullptr)
			{
				continue;
			}
			mSolveRunRecord* run = center->getRun(posItem->data(Qt::UserRole).toULongLong());
			if (run != nullptr && run->getStatus() == mSolveRunRecord::Status_Running)
			{
				_queueTable->item(row, 4)->setText(formatElapsed(now - run->getStartedAt()));
			}
		}
	}

	/******************** 日志查看器 ********************/

	void mSolveTaskDialog::openLogTab(quint64 runID)
	{
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		mSolveRunRecord* run = center->getRun(runID);
		if (run == nullptr)
		{
			return;
		}
		QPlainTextEdit* page = _logPages.value(runID, nullptr);
		if (page == nullptr)
		{
			page = new QPlainTextEdit(_logTabs);
			page->setReadOnly(true);
			page->setLineWrapMode(QPlainTextEdit::NoWrap);
			QFont monoFont("Consolas");
			monoFont.setStyleHint(QFont::Monospace);
			page->setFont(monoFont);
			if (run->isFinished())
			{
				//已结束：读完整日志文件
				QString content;
				if (!center->readRunLogFile(runID, content))
				{
					content = center->getRunLogTail(runID);
				}
				page->setPlainText(content);
			}
			else
			{
				//运行中：先回放内存缓冲，后续由logAppended实时追加
				page->setPlainText(center->getRunLogTail(runID));
			}
			QString label = QString("%1 · #%2").arg(run->getTaskName()).arg(run->getRunID());
			_logTabs->addTab(page, label);
			_logPages.insert(runID, page);
		}
		_logTabs->setCurrentWidget(page);
		//切到求解记录页并展开日志栏
		_tabs->setCurrentIndex(1);
		setLogViewerExpanded(true);
		page->verticalScrollBar()->setValue(page->verticalScrollBar()->maximum());
	}

	void mSolveTaskDialog::setLogViewerExpanded(bool expanded)
	{
		_logsCollapsed = !expanded;
		_logBody->setVisible(expanded);
		_logToggleBtn->setText(expanded ? "收起" : "展开");
		//同步压缩分割条尺寸，收起时日志栏只剩标题行高度
		_mainSplitter->setSizes(expanded ? QList<int>() << 640 << 240 : QList<int>() << 700 << 40);
	}

	void mSolveTaskDialog::showResultActions(quint64 runID)
	{
		if (runID == 0)
		{
			return;
		}
		mSolveTaskCenter* center = mSolveTaskCenter::getInstance();
		//后处理模型需要交互：动作完成后关闭弹窗（写文件进行中则保持）
		auto closeDialog = [this]()
			{
				if (_writeThread == nullptr)
				{
					accept();
				}
			};

		//首次查看：直接新开后处理模型
		if (!center->hasRunPostModel(runID))
		{
			center->openRunResultInNewPost(runID);
			closeDialog();
			return;
		}

		//已打开过：在按钮位置弹出选择弹窗（标题栏 + 两个带边框按钮）
		QDialog popup(this);
		popup.setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
		popup.setAttribute(Qt::WA_TranslucentBackground, false);
		popup.setStyleSheet("QDialog { background-color: white; border: 1px solid #DCDFE6; border-radius: 2px; }");
		QVBoxLayout* popupLayout = new QVBoxLayout(&popup);
		popupLayout->setContentsMargins(0, 0, 0, 8);
		popupLayout->setSpacing(8);

		//标题行：底色E9EDF3，标题+右侧关闭"x"
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

		//两个带边框按钮
        QHBoxLayout* hlayout1 = new QHBoxLayout(&popup);
		QPushButton* newPostBtn = new QPushButton("新开后处理");
		newPostBtn->setFixedSize(180,28);
		newPostBtn->setStyleSheet(
			"QPushButton { background: white; color: #424b57; border: 1px solid #DCDFE6; border-radius: 2px; }"
			"QPushButton:hover { border-color: #91CAFF; color: #1890FF; }");
        hlayout1->addStretch();
        hlayout1->addWidget(newPostBtn);
        hlayout1->addStretch();
        QHBoxLayout* hlayout2 = new QHBoxLayout(&popup);
		QPushButton* jumpPostBtn = new QPushButton("跳转已有模型", &popup);
        jumpPostBtn->setStyleSheet(kPrimaryButtonStyle);
		jumpPostBtn->setFixedSize(180, 28);
        hlayout2->addStretch();
        hlayout2->addWidget(jumpPostBtn);
        hlayout2->addStretch();
		popupLayout->addLayout(hlayout1);
		popupLayout->addLayout(hlayout2);

		int choice = -1;
		connect(newPostBtn, &QPushButton::clicked, &popup, [&popup, &choice]() { choice = 0; popup.accept(); });
		connect(jumpPostBtn, &QPushButton::clicked, &popup, [&popup, &choice]() { choice = 1; popup.accept(); });

		popup.adjustSize();
		popup.move(QCursor::pos());
		popup.exec();

		if (choice == 0)
		{
			center->openRunResultInNewPost(runID);
			closeDialog();
		}
		else if (choice == 1)
		{
			center->jumpToRunPostModel(runID);
			closeDialog();
		}
		//choice==-1（点"×"关闭或点外部关闭）：不执行动作，任务管理弹窗保持打开
	}

	void mSolveTaskDialog::slot_logAppended(quint64 runID, const QString& text)
	{
		//只追加已打开页签的run；未打开的不渲染（数据仍在中心，随时可打开回放）
		QPlainTextEdit* page = _logPages.value(runID, nullptr);
		if (page == nullptr || !_logViewer->isVisible())
		{
			return;
		}
		//跟随滚动：原本就在底部才自动滚（用户手动上翻则不打扰）
		QScrollBar* bar = page->verticalScrollBar();
		const bool atBottom = bar->value() >= bar->maximum() - 4;
		QTextCursor cursor = page->textCursor();
		cursor.movePosition(QTextCursor::End);
		cursor.insertText(text);
		if (atBottom)
		{
			bar->setValue(bar->maximum());
		}
	}

	void mSolveTaskDialog::slot_logTabCloseRequested(int index)
	{
		QWidget* widget = _logTabs->widget(index);
		quint64 runID = 0;
		for (auto iter = _logPages.begin(); iter != _logPages.end(); ++iter)
		{
			if (iter.value() == widget)
			{
				runID = iter.key();
				break;
			}
		}
		if (runID != 0)
		{
			_logPages.remove(runID);
		}
		_logTabs->removeTab(index);
		delete widget;
		//日志栏常驻：页签全部关闭后不隐藏
	}

	void mSolveTaskDialog::slot_toggleLogViewer()
	{
		//展开/收起切换（收起时仅保留标题行）
		setLogViewerExpanded(_logsCollapsed);
	}
}
