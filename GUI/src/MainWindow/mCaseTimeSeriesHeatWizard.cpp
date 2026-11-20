#include "mCaseTimeSeriesHeatWizard.h"
#pragma execution_character_set("utf-8")

#include "mTimeSeriesCaseStepWidget.h"
#include "mTimeSeriesLoadStepWidget.h"
#include "mTimeSeriesBoundaryStepWidget.h"
#include "mTimeSeriesOutputStepWidget.h"
#include "mTimeSeriesHeatWizardCommit.h"
#include "mNVHWindowComponent.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace MechMainWindow
{
    namespace
    {
        const QStringList kStepNames = {
            QStringLiteral("工况设置"), QStringLiteral("载荷设置"),
            QStringLiteral("边界设置"), QStringLiteral("输出设置")
        };
    }

    // ===== 向导主窗口 =====

    mCaseTimeSeriesHeatWizard::mCaseTimeSeriesHeatWizard(QWidget* parent)
        : QDialog(parent)
    {
        initUI();
        initConnect();
        updateStepUi();
    }

    mCaseTimeSeriesHeatWizard::~mCaseTimeSeriesHeatWizard() = default;

    void mCaseTimeSeriesHeatWizard::initUI()
    {
        // 外层框架对齐 NTF/VTF 等 NVH 分析弹窗
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint
            | Qt::WindowMinMaxButtonsHint);
        setWindowTitle(QStringLiteral("时序热分析工况"));
        setMinimumSize(960, 640);

        // 居中弹窗：min(1120, 屏宽-56) × min(820, 屏高-56)
        const QRect screen = QApplication::desktop()->availableGeometry(this);
        resize(qMin(1120, screen.width() - 56), qMin(820, screen.height() - 56));

        setStyleSheet(QStringLiteral(
            "QDialog{background:#fff;}"
            "QWidget{background:#fff;}"
            "QLabel{color:#4b5058;font-size:13px;background:transparent;}"
            "QLineEdit,QComboBox,QDoubleSpinBox,QSpinBox{height:28px;min-height:28px;border:1px solid #d6dde3;border-radius:4px;"
            "  padding:0 8px;background:#fff;color:#344054;font-size:12px;}"
            "QComboBox{padding-right:24px;}"   // 给全局 30px 下拉箭头留出空间，避免遮挡文字
            "QComboBox:disabled{background:#F5F7FA;color:#808080;}"
            "QLineEdit:focus,QComboBox:focus,QDoubleSpinBox:focus,QSpinBox:focus{border-color:#168ff0;}"
            "QLineEdit:read-only{color:#98a2b3;}"
            "QLineEdit:disabled,QDoubleSpinBox:disabled,QSpinBox:disabled{background:#F5F7FA;color:#808080;}"
            "QCheckBox{background:transparent;}"
            "QPushButton{height:28px;min-height:28px;padding:0 14px;border:1px solid #dcdfe6;"
            "  border-radius:4px;background:#fff;color:#606266;font-size:12px;}"
            "QPushButton:hover{border-color:#c0c4cc;}"
            "QPushButton:disabled{background:#E4E7EE;color:#A8ABB2;border-color:#E4E7EE;}"));

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 20, 20, 20);

        // ===== 中部：左侧步骤导航 + 分隔线 + 右侧内容 =====
        auto* contentLayout = new QHBoxLayout();
        contentLayout->setSpacing(0);
        contentLayout->setContentsMargins(0, 0, 0, 20);
        contentLayout->setAlignment(Qt::AlignTop);

        initLeftNavigation();
        contentLayout->addWidget(m_navList, 0, Qt::AlignTop);

        auto* line = new QFrame(this);
        line->setFrameShape(QFrame::StyledPanel);
        line->setStyleSheet(QStringLiteral("QFrame{background-color:#DCDFE6;border:none;}"));
        line->setFixedWidth(1);
        line->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        contentLayout->addWidget(line);

        // ===== 右侧面板 =====
        auto* rightLayout = new QVBoxLayout();
        rightLayout->setSpacing(0);
        rightLayout->setContentsMargins(20, 10, 0, 0);

        m_stackWidget = new QStackedWidget(this);
        m_caseStep = new mTimeSeriesCaseStepWidget(this);
        m_loadStep = new mTimeSeriesLoadStepWidget(this);
        m_boundaryStep = new mTimeSeriesBoundaryStepWidget(this);
        m_outputStep = new mTimeSeriesOutputStepWidget(this);
        m_stackWidget->addWidget(m_caseStep);
        m_stackWidget->addWidget(m_loadStep);
        m_stackWidget->addWidget(m_boundaryStep);
        m_stackWidget->addWidget(m_outputStep);
        m_stackWidget->setCurrentIndex(0);
        rightLayout->addWidget(m_stackWidget, 1);

        // 页面内容修改信号（在 initConnect 之前创建页面对象，统一于 initConnect 中连接）

        contentLayout->addLayout(rightLayout, 1);
        mainLayout->addLayout(contentLayout, 1);

        // ===== 底部按钮（与 NVH 弹窗一致的主/次按钮样式） =====
        auto* bottomLayout = new QHBoxLayout();
        bottomLayout->setSpacing(10);
        bottomLayout->setContentsMargins(0, 0, 0, 0);
        bottomLayout->addStretch(1);

        // 清除按钮：清空当前页内容（上一步左侧）
        m_clearButton = new QPushButton(QStringLiteral("清除"), this);
        m_clearButton->setFixedSize(110, 30);
        m_clearButton->setAutoDefault(false);
        m_clearButton->setStyleSheet(QStringLiteral(
            "QPushButton{background-color:#ffffff;color:#d64545;border:1px solid #e3a4a4;border-radius:4px;padding:6px 18px;}"
            "QPushButton:disabled{background-color:#E4E7EE;color:#A8ABB2;border-color:#E4E7EE;}"
            "QPushButton:hover{border-color:#d64545;}"));

        m_previousButton = new QPushButton(QStringLiteral("上一步"), this);
        m_previousButton->setFixedSize(110, 30);
        m_nextButton = new QPushButton(QStringLiteral("下一步"), this);
        m_nextButton->setFixedSize(110, 30);
        m_previousButton->setAutoDefault(false);
        m_nextButton->setAutoDefault(false);

        const QString primaryButtonStyle = QStringLiteral(
            "QPushButton{background-color:#1385F6;color:white;border:none;border-radius:4px;padding:6px 18px;}"
            "QPushButton:disabled{background-color:#E4E7EE;color:#A8ABB2;}"
            "QPushButton:hover:!disabled{background-color:#409eff;}");
        const QString secondaryButtonStyle = QStringLiteral(
            "QPushButton{background-color:#ffffff;color:#606266;border:1px solid #dcdfe6;border-radius:4px;padding:6px 18px;}"
            "QPushButton:disabled{background-color:#E4E7EE;color:#A8ABB2;}"
            "QPushButton:hover{border-color:#c0c4cc;}");
        m_previousButton->setStyleSheet(secondaryButtonStyle);
        m_nextButton->setStyleSheet(primaryButtonStyle);

        bottomLayout->addWidget(m_clearButton);
        bottomLayout->addWidget(m_previousButton);
        bottomLayout->addWidget(m_nextButton);
        mainLayout->addLayout(bottomLayout);
    }

    void mCaseTimeSeriesHeatWizard::initLeftNavigation()
    {
        m_navList = new QListWidget(this);
        QFont font;
        font.setPointSize(10);
        m_navList->setFont(font);
        m_navList->setFixedWidth(220);
        m_navList->setFocusPolicy(Qt::NoFocus);
        m_navList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_navList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_navList->setSpacing(0);
        m_navList->setFrameShape(QFrame::NoFrame);
        m_navList->setViewMode(QListView::ListMode);
        m_navList->setUniformItemSizes(true);
        m_navList->setItemDelegate(m_navIconDelegate = new NavIconDelegate(this));
        m_navIconDelegate->setOutputPageIndex(kStepNames.size() - 1);   // 输出页专用图标
        m_navList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_navList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_navList->setFixedHeight(48 * kStepNames.size());

        for (int i = 0; i < kStepNames.size(); ++i)
        {
            auto* item = new QListWidgetItem(kStepNames.at(i), m_navList);
            item->setData(Qt::UserRole, i);
            item->setSizeHint(QSize(120, 48));
        }

        m_navList->setStyleSheet(QStringLiteral(
            "QListWidget::item{background-color:transparent;color:#606266;border:none;"
            "  padding:0px;padding-left:16px;min-height:48px;max-height:48px;}"
            "QListWidget{background-color:#ffffff;border:none;outline:none;}"
            "QListWidget::item:selected{background-color:transparent;color:#1385F6;}"
            "QListWidget::item:hover:!selected{background-color:transparent;}"));

        m_navList->setCurrentRow(0);
    }

    void mCaseTimeSeriesHeatWizard::initConnect()
    {
        connect(m_previousButton, &QPushButton::clicked, this, &mCaseTimeSeriesHeatWizard::slot_previous);
        connect(m_nextButton, &QPushButton::clicked, this, &mCaseTimeSeriesHeatWizard::slot_next);
        connect(m_clearButton, &QPushButton::clicked, this, &mCaseTimeSeriesHeatWizard::slot_clearCurrentPage);

        connect(m_navList, &QListWidget::currentRowChanged, this, [this](int row) {
            const int currentRow = m_stackWidget->currentIndex();
            if (row >= 0 && row < m_stackWidget->count() && isNavigationIndexEnabled(row))
            {
                if (QWidget* fw = QApplication::focusWidget())
                    fw->clearFocus();
                m_activeStep = row;
                updateStepUi();
            }
            else if (m_navList->currentRow() != currentRow)
            {
                // 未到达的步骤不可跳转，恢复选中当前页
                QSignalBlocker blocker(m_navList);
                m_navList->setCurrentRow(currentRow);
            }
        });

        // 页面内容修改后，重置其后续步骤的完成状态（对齐 NVH 弹窗交互）：
        // 工况设置：生成工况/切换创建方式；载荷设置：新建/保存/删除载荷；边界设置同理
        connect(m_caseStep, &mTimeSeriesCaseStepWidget::dataModified, this, [this]() { onPageModified(0); });
        connect(m_loadStep, &mTimeSeriesLoadStepWidget::dataModified, this, [this]() { onPageModified(1); });
        connect(m_boundaryStep, &mTimeSeriesBoundaryStepWidget::dataModified, this, [this]() { onPageModified(2); });
    }

    bool mCaseTimeSeriesHeatWizard::isNavigationIndexEnabled(int index) const
    {
        return index >= 0 && index <= m_farthestStep;
    }

    void mCaseTimeSeriesHeatWizard::onPageModified(int pageIndex)
    {
        // 修改已到达过的页：其后的步骤标记为未完成（最远到达位置不回收，
        // 用户仍可点击导航回去查看，但需重新经过“下一步”校验才会重新打勾）
        if (pageIndex < 0 || pageIndex >= m_stackWidget->count())
            return;
        for (int i = pageIndex + 1; i < m_stackWidget->count(); ++i)
            m_navIconDelegate->setPageCompleted(i, false);
        m_navList->viewport()->update();
    }

    void mCaseTimeSeriesHeatWizard::slot_previous()
    {
        if (m_activeStep > 0)
        {
            --m_activeStep;
            updateStepUi();
        }
    }

    void mCaseTimeSeriesHeatWizard::slot_clearCurrentPage()
    {
        // 清除当前页内容（各页实现 clearPage，内部会发 dataModified 重置后续步骤完成状态）
        switch (m_activeStep)
        {
        case 0: m_caseStep->clearPage(); break;
        case 1: m_loadStep->clearPage(); break;
        case 2: m_boundaryStep->clearPage(); break;
        case 3: m_outputStep->clearPage(); break;
        }
    }

    void mCaseTimeSeriesHeatWizard::slot_next()
    {
        if (!validateCurrentStep())
            return;

        // 经过校验的当前页打勾，并解锁下一页
        m_farthestStep = qMax(m_farthestStep, m_activeStep + 1);
        m_navIconDelegate->setPageCompleted(m_activeStep, true);
        if (m_activeStep < m_stackWidget->count() - 1)
        {
            ++m_activeStep;
            updateStepUi();
        }
        else
        {
            finishWizard();
        }
    }

    void mCaseTimeSeriesHeatWizard::updateStepUi()
    {
        m_stackWidget->setCurrentIndex(m_activeStep);

        {
            QSignalBlocker blocker(m_navList);
            m_navList->setCurrentRow(m_activeStep);
        }
        // 仅刷新可点击状态；完成打勾由“下一步”(slot_next)驱动，内容修改由 onPageModified 重置
        for (int i = 0; i < m_stackWidget->count(); ++i)
            m_navIconDelegate->setPageEnabled(i, isNavigationIndexEnabled(i));
        m_navIconDelegate->setCurrentPage(m_activeStep);
        m_navList->viewport()->update();

        m_previousButton->setEnabled(m_activeStep > 0);
        m_nextButton->setText(m_activeStep == m_stackWidget->count() - 1
            ? QStringLiteral("完成") : QStringLiteral("下一步"));
    }

    bool mCaseTimeSeriesHeatWizard::validateCurrentStep()
    {
        QString error;
        bool ok = true;
        switch (m_activeStep)
        {
        case 0: ok = m_caseStep->validatePage(&error); break;
        case 1: ok = m_loadStep->validatePage(&error); break;
        case 2: ok = m_boundaryStep->validatePage(&error); break;
        case 3: ok = m_outputStep->validatePage(&error); break;
        }
        if (!ok)
            QMessageBox::warning(this, kStepNames.at(m_activeStep), error);
        return ok;
    }

    void mCaseTimeSeriesHeatWizard::finishWizard()
    {
        QString summary;
        QString error;
        if (!TimeSeriesHeatWizardCommit::commit(m_cases, m_stepSpec, m_loadItems,
            m_boundaryItems, m_outputSpec, &summary, &error))
        {
            QMessageBox::critical(this, QStringLiteral("时序热分析工况"), error);
            return;
        }
        QMessageBox::information(this, QStringLiteral("时序热分析工况"), summary);
        accept();
    }

    void mCaseTimeSeriesHeatWizard::resetToDefault()
    {
        // 内容持久化：关闭再打开不再清空。仅重建步骤页并按当前向导数据回显。
        while (m_stackWidget->count() > 0)
        {
            QWidget* widget = m_stackWidget->widget(0);
            m_stackWidget->removeWidget(widget);
            widget->deleteLater();
        }
        m_caseStep = new mTimeSeriesCaseStepWidget(this);
        m_loadStep = new mTimeSeriesLoadStepWidget(this);
        m_boundaryStep = new mTimeSeriesBoundaryStepWidget(this);
        m_outputStep = new mTimeSeriesOutputStepWidget(this);
        m_stackWidget->addWidget(m_caseStep);
        m_stackWidget->addWidget(m_loadStep);
        m_stackWidget->addWidget(m_boundaryStep);
        m_stackWidget->addWidget(m_outputStep);

        // 重建后重新连接页面修改信号
        connect(m_caseStep, &mTimeSeriesCaseStepWidget::dataModified, this, [this]() { onPageModified(0); });
        connect(m_loadStep, &mTimeSeriesLoadStepWidget::dataModified, this, [this]() { onPageModified(1); });
        connect(m_boundaryStep, &mTimeSeriesBoundaryStepWidget::dataModified, this, [this]() { onPageModified(2); });

        updateStepUi();
    }
}
