#include "mTimeSeriesCaseStepWidget.h"
#pragma execution_character_set("utf-8")

#include "mCaseTimeSeriesHeatWizard.h"
#include "mTimeSeriesFileParser.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

using MxFunctions::mCalcLineEdit;

namespace MechMainWindow
{
    namespace
    {
        // 表格样式对齐 NVH 弹窗（NTF/VTF）：斑马纹 + 选中行浅蓝 + 表头 #E8EBF2
        const char* kTableStyle =
            "QTableWidget{alternate-background-color:#F6F8FA;background-color:#FFFFFF;"
            "  selection-background-color:#D0E7FD;gridline-color:#DCDFE6;"
            "  border:1px solid #DCDFE6;border-radius:2px;}"
            "QTableWidget::item{color:#475467;border:0;border-bottom:1px solid #ccc;padding-left:8px;font-size:12px;}"
            "QHeaderView::section:horizontal{background-color:#E8EBF2;color:#2C3E50;"
            "  border:0;border-right:1px solid #DCDFE6;border-bottom:1px solid #DCDFE6;"
            "  height:29px;font-size:12px;padding-left:8px;}";

        const char* kPrimaryBtnStyle =
            "QPushButton{background:#168ff0;border:1px solid #168ff0;border-radius:3px;color:#fff;}"
            "QPushButton:hover{background:#3ba2f3;border-color:#3ba2f3;}"
            "QPushButton:disabled{color:#b6bec9;border-color:#e7ebf0;background:#f3f5f7;}";

        /*
        * mCalcLineEdit 数值读取：文本为空/非法表达式未求值时回退。
        * 回退值取原 SpinBox 的下限，保持"越界钳到下限"的旧语义。
        */
        double calcEditValue(const mCalcLineEdit* edit, double fallback)
        {
            bool ok = false;
            const double v = edit->text().trimmed().toDouble(&ok);
            return ok ? v : fallback;
        }
    }

    mTimeSeriesCaseStepWidget::mTimeSeriesCaseStepWidget(mCaseTimeSeriesHeatWizard* wizard)
        : QWidget(wizard), m_wizard(wizard)
    {
        buildUi();
        // 恢复向导已保存的数据（关闭再打开时页面内容不清除）
        reloadFromData();
    }

    void mTimeSeriesCaseStepWidget::buildUi()
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 6, 0, 0);
        mainLayout->setSpacing(0);

        // ===== 分析序列名称（底部带分隔线） =====
        auto* sequenceWidget = new QWidget(this);
        auto* sequenceRow = new QHBoxLayout(sequenceWidget);
        sequenceRow->setContentsMargins(0, 4, 0, 13);
        sequenceRow->setSpacing(14);
        auto* sequenceLabel = new QLabel(QStringLiteral("分析序列名称"), sequenceWidget);
        sequenceLabel->setFixedWidth(92);
        sequenceLabel->setStyleSheet(QStringLiteral("font-size:13px;color:#344054;"));
        m_sequenceNameEdit = new QLineEdit(m_wizard->m_sequenceName, sequenceWidget);
        m_sequenceNameEdit->setFixedWidth(268);
        sequenceRow->addWidget(sequenceLabel);
        sequenceRow->addWidget(m_sequenceNameEdit);
        sequenceRow->addStretch();
        mainLayout->addWidget(sequenceWidget);

        auto* separator = new QFrame(this);
        separator->setFrameShape(QFrame::HLine);
        separator->setStyleSheet(QStringLiteral("QFrame{color:#d6dce1;background:#d6dce1;max-height:1px;border:0;}"));
        mainLayout->addWidget(separator);
        mainLayout->addSpacing(14);

        // ===== 工况创建 =====
        auto* createTitle = new QLabel(QStringLiteral("工况创建"), this);
        createTitle->setStyleSheet(QStringLiteral("font-size:13px;color:#4b5058;"));
        mainLayout->addWidget(createTitle);
        mainLayout->addSpacing(9);

        m_methodCombo = new QComboBox(this);
        m_methodCombo->addItem(QStringLiteral("时序参数"));
        m_methodCombo->addItem(QStringLiteral("时序文件"));
        m_methodCombo->setFixedWidth(150);

        // --- 时序参数行：数值框随窗口拉宽（最小宽度，多余空间均分） ---
        // 使用 mCalcLineEdit：支持科学计数法与表达式输入（如 1e-5、1/2），
        // 且限制字符类型；原 QDoubleSpinBox 限制小数位数与最大值、无法输入科学计数法
        m_paramRow = new QWidget(this);
        auto* paramLayout = new QHBoxLayout(m_paramRow);
        paramLayout->setContentsMargins(0, 0, 0, 0);
        paramLayout->setSpacing(9);

        auto makeNumberField = [this, paramLayout](const QString& label, mCalcLineEdit*& edit,
            const QString& initialText, const QString& unit, bool integer) {
            auto* lbl = new QLabel(label, this);
            lbl->setStyleSheet(QStringLiteral("font-size:12px;color:#666e77;"));
            if (integer)
            {
                edit = new mCalcLineEdit(initialText, mCalcLineEdit::CalcMode::Strict, this);
                edit->setRange(1, 100000);
            }
            else
            {
                edit = new mCalcLineEdit(initialText, mCalcLineEdit::CalcMode::NoEmptyAndNonNegative, this);
                edit->setRange(0.0, 1e12);
            }
            edit->setMinimumWidth(80);
            auto* unitLbl = new QLabel(unit, this);
            unitLbl->setStyleSheet(QStringLiteral("color:#9aa3ad;font-size:11px;"));
            paramLayout->addWidget(lbl);
            paramLayout->addWidget(edit, 1);
            paramLayout->addWidget(unitLbl);
            paramLayout->addSpacing(7);
        };

        mCalcLineEdit* startEdit = nullptr;
        mCalcLineEdit* totalEdit = nullptr;
        mCalcLineEdit* countEdit = nullptr;
        makeNumberField(QStringLiteral("开始时间"), startEdit, QStringLiteral("0"), QStringLiteral("s"), false);
        makeNumberField(QStringLiteral("总持续时间"), totalEdit, QStringLiteral("1200"), QStringLiteral("s"), false);
        makeNumberField(QStringLiteral("工况数量"), countEdit, QStringLiteral("120"), QStringLiteral("个"), true);
        m_startTimeEdit = startEdit;
        m_totalTimeEdit = totalEdit;
        m_caseCountEdit = countEdit;

        // --- 时序文件行 ---
        m_fileRow = new QWidget(this);
        auto* fileLayout = new QHBoxLayout(m_fileRow);
        fileLayout->setContentsMargins(0, 0, 0, 0);
        fileLayout->setSpacing(8);
        auto* fileLabel = new QLabel(QStringLiteral("文件路径"), m_fileRow);
        fileLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#666e77;"));
        m_filePathEdit = new QLineEdit(m_fileRow);
        m_filePathEdit->setReadOnly(true);
        m_filePathEdit->setPlaceholderText(QStringLiteral("请选择时序文件"));
        auto* browseButton = new QPushButton(QStringLiteral("…"), m_fileRow);
        browseButton->setFixedSize(28, 28);
        browseButton->setToolTip(QStringLiteral("选择文件"));
        fileLayout->addWidget(fileLabel);
        fileLayout->addWidget(m_filePathEdit, 1);
        fileLayout->addWidget(browseButton);
        m_fileRow->setVisible(false);

        m_generateButton = new QPushButton(QStringLiteral("生成工况"), this);
        m_generateButton->setFixedSize(90, 28);
        m_generateButton->setStyleSheet(kPrimaryBtnStyle);

        auto* methodRow = new QHBoxLayout();
        methodRow->setSpacing(9);
        auto* methodLabel = new QLabel(QStringLiteral("创建方式"), this);
        methodLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#666e77;"));
        methodRow->addWidget(methodLabel);
        methodRow->addWidget(m_methodCombo);
        methodRow->addWidget(m_paramRow, 1);
        methodRow->addWidget(m_fileRow, 1);
        methodRow->addWidget(m_generateButton);
        mainLayout->addLayout(methodRow);
        mainLayout->addSpacing(16);

        // ===== 时间步长 =====
        auto* stepTitle = new QLabel(QStringLiteral("时间步长"), this);
        stepTitle->setStyleSheet(QStringLiteral("font-size:13px;color:#4b5058;"));
        mainLayout->addWidget(stepTitle);
        mainLayout->addSpacing(9);

        auto* stepRow = new QHBoxLayout();
        stepRow->setSpacing(9);
        auto* stepLabel = new QLabel(QStringLiteral("时间步"), this);
        stepLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#666e77;"));
        m_stepModeCombo = new QComboBox(this);
        m_stepModeCombo->addItem(QStringLiteral("自动"));
        m_stepModeCombo->addItem(QStringLiteral("固定"));
        m_stepModeCombo->setCurrentIndex(1);
        m_stepModeCombo->setFixedWidth(100);
        m_stepUnitCombo = new QComboBox(this);
        m_stepUnitCombo->addItem(QStringLiteral("时间"));
        m_stepUnitCombo->addItem(QStringLiteral("百分比"));
        m_stepUnitCombo->setFixedWidth(100);

        auto makeStepField = [this](const QString& label, mCalcLineEdit*& edit,
            const QString& initialText) -> QWidget* {
            auto* widget = new QWidget(this);
            auto* layout = new QHBoxLayout(widget);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(7);
            auto* lbl = new QLabel(label, widget);
            lbl->setStyleSheet(QStringLiteral("font-size:12px;color:#666e77;"));
            edit = new mCalcLineEdit(initialText, mCalcLineEdit::CalcMode::NoEmptyAndNonNegative, widget);
            edit->setRange(0.0, 1e12);
            edit->setMinimumWidth(70);
            m_stepUnitLabels.append(lbl);
            layout->addWidget(lbl);
            layout->addWidget(edit, 1);
            return widget;
        };

        m_fixedStepWidget = makeStepField(QStringLiteral("时间步长"), m_fixedStepEdit, QStringLiteral("1"));
        m_autoStepWidget = new QWidget(this);
        auto* autoLayout = new QHBoxLayout(m_autoStepWidget);
        autoLayout->setContentsMargins(0, 0, 0, 0);
        autoLayout->setSpacing(7);
        auto makeAutoField = [this, autoLayout](const QString& label, mCalcLineEdit*& edit,
            const QString& initialText) {
            auto* lbl = new QLabel(label, this);
            lbl->setStyleSheet(QStringLiteral("font-size:12px;color:#666e77;"));
            edit = new mCalcLineEdit(initialText, mCalcLineEdit::CalcMode::NoEmptyAndNonNegative, this);
            edit->setRange(0.0, 1e12);
            edit->setMinimumWidth(60);
            autoLayout->addWidget(lbl);
            autoLayout->addWidget(edit, 1);
        };
        makeAutoField(QStringLiteral("初始步长"), m_initialStepEdit, QStringLiteral("1"));
        makeAutoField(QStringLiteral("最小步长"), m_minStepEdit, QStringLiteral("0.1"));
        makeAutoField(QStringLiteral("最大步长"), m_maxStepEdit, QStringLiteral("10"));
        m_autoStepWidget->setVisible(false);

        m_stepUpdateButton = new QPushButton(QStringLiteral("更新步长"), this);
        m_stepUpdateButton->setFixedSize(90, 28);
        m_stepUpdateButton->setStyleSheet(kPrimaryBtnStyle);

        // 两个下拉固定宽度，输入区随窗口拉宽，按钮始终靠右
        stepRow->addWidget(stepLabel);
        stepRow->addWidget(m_stepModeCombo);
        stepRow->addWidget(m_stepUnitCombo);
        stepRow->addWidget(m_fixedStepWidget, 1);
        stepRow->addWidget(m_autoStepWidget, 1);
        stepRow->addStretch();
        stepRow->addWidget(m_stepUpdateButton);
        mainLayout->addLayout(stepRow);
        mainLayout->addSpacing(16);

        // ===== 时序工况表 =====
        auto* tableTitle = new QLabel(QStringLiteral("时序工况表"), this);
        tableTitle->setStyleSheet(QStringLiteral("font-size:13px;color:#4b5058;"));
        mainLayout->addWidget(tableTitle);
        mainLayout->addSpacing(9);

        m_caseTable = new QTableWidget(this);
        m_caseTable->setColumnCount(6);
        m_caseTable->setHorizontalHeaderLabels({ QStringLiteral("序号"), QStringLiteral("工况名称"),
            QStringLiteral("开始时间 T0"), QStringLiteral("结束时间 T0 + t"),
            QStringLiteral("持续时间 t"), QStringLiteral("步长（固定）") });
        m_caseTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_caseTable->verticalHeader()->setVisible(false);
        m_caseTable->verticalHeader()->setDefaultSectionSize(33);
        m_caseTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_caseTable->setSelectionMode(QAbstractItemView::NoSelection);
        m_caseTable->setAlternatingRowColors(true);
        m_caseTable->setShowGrid(false);
        m_caseTable->setStyleSheet(kTableStyle);
        mainLayout->addWidget(m_caseTable, 1);

        // ===== 信号 =====
        connect(browseButton, &QPushButton::clicked, this, &mTimeSeriesCaseStepWidget::slot_browseFile);
        connect(m_methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mTimeSeriesCaseStepWidget::slot_methodChanged);
        connect(m_generateButton, &QPushButton::clicked, this, &mTimeSeriesCaseStepWidget::slot_generate);
        connect(m_stepModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mTimeSeriesCaseStepWidget::slot_stepModeChanged);
        connect(m_sequenceNameEdit, &QLineEdit::textChanged,
            this, &mTimeSeriesCaseStepWidget::slot_sequenceNameChanged);
        connect(m_stepUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { refreshTable(); });
        connect(m_stepUpdateButton, &QPushButton::clicked, this, &mTimeSeriesCaseStepWidget::slot_updateStep);

        // mCalcLineEdit 继承 QLineEdit：焦点离开/回车完成求值后再刷新工况表
        const QList<QLineEdit*> refreshEdits = { m_startTimeEdit, m_totalTimeEdit, m_caseCountEdit,
            m_fixedStepEdit, m_initialStepEdit, m_minStepEdit, m_maxStepEdit };
        for (QLineEdit* edit : refreshEdits)
            connect(edit, &QLineEdit::editingFinished, this, [this]() { refreshTable(); });
    }

    void mTimeSeriesCaseStepWidget::clearPage()
    {
        m_wizard->m_sequenceName = QStringLiteral("HeatStep");
        m_wizard->m_cases.clear();
        m_wizard->m_stepSpec = TimeSeriesStepSpec();
        m_wizard->m_caseMethodIsFile = false;
        m_wizard->m_timeSeriesFilePath.clear();
        m_sequenceNameEdit->setText(m_wizard->m_sequenceName);
        m_filePathEdit->clear();
        m_methodCombo->setCurrentIndex(0);
        m_startTimeEdit->setExpression(QStringLiteral("0"));
        m_totalTimeEdit->setExpression(QStringLiteral("1200"));
        m_caseCountEdit->setExpression(QStringLiteral("120"));
        m_stepModeCombo->setCurrentIndex(1);
        m_stepUnitCombo->setCurrentIndex(0);
        m_fixedStepEdit->setExpression(QStringLiteral("1"));
        m_initialStepEdit->setExpression(QStringLiteral("1"));
        m_minStepEdit->setExpression(QStringLiteral("0.1"));
        m_maxStepEdit->setExpression(QStringLiteral("10"));
        refreshTable();
        emit dataModified();
    }

    void mTimeSeriesCaseStepWidget::slot_methodChanged(int index)
    {
        const bool isFile = (index == 1);
        m_paramRow->setVisible(!isFile);
        m_fileRow->setVisible(isFile);
        m_wizard->m_caseMethodIsFile = isFile;
        emit dataModified();
    }

    void mTimeSeriesCaseStepWidget::reloadFromData()
    {
        // 恢复向导数据：创建方式、序列名称、时序参数、步长参数与工况表
        m_sequenceNameEdit->setText(m_wizard->m_sequenceName);
        m_filePathEdit->setText(m_wizard->m_timeSeriesFilePath);
        m_methodCombo->setCurrentIndex(m_wizard->m_caseMethodIsFile ? 1 : 0);
        const bool isFile = m_wizard->m_caseMethodIsFile;
        m_paramRow->setVisible(!isFile);
        m_fileRow->setVisible(isFile);
        m_stepModeCombo->setCurrentIndex(m_wizard->m_stepSpec.isAuto ? 0 : 1);
        m_fixedStepWidget->setVisible(!m_wizard->m_stepSpec.isAuto);
        m_autoStepWidget->setVisible(m_wizard->m_stepSpec.isAuto);
        m_fixedStepEdit->setExpression(QString::number(m_wizard->m_stepSpec.fixed));
        m_initialStepEdit->setExpression(QString::number(m_wizard->m_stepSpec.initial));
        m_minStepEdit->setExpression(QString::number(m_wizard->m_stepSpec.minStep));
        m_maxStepEdit->setExpression(QString::number(m_wizard->m_stepSpec.maxStep));
        refreshTable();
    }

    void mTimeSeriesCaseStepWidget::slot_browseFile()
    {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择时序文件"),
            QString(), QStringLiteral("时序文件 (*.txt)"));
        if (path.isEmpty())
            return;
        m_filePathEdit->setText(path);
        m_wizard->m_timeSeriesFilePath = path;
    }

    void mTimeSeriesCaseStepWidget::slot_generate()
    {
        // 仅“生成工况”按钮触发生成，编辑输入框不自动生成
        regenerateCases();
        if (!m_wizard->m_cases.isEmpty())
        {
            QMessageBox::information(this, QStringLiteral("生成工况"),
                QStringLiteral("已生成 %1 个时序工况").arg(m_wizard->m_cases.size()));
        }
        emit dataModified();
    }

    void mTimeSeriesCaseStepWidget::slot_stepModeChanged(int index)
    {
        m_wizard->m_stepSpec.isAuto = (index == 0);
        m_fixedStepWidget->setVisible(!m_wizard->m_stepSpec.isAuto);
        m_autoStepWidget->setVisible(m_wizard->m_stepSpec.isAuto);
        m_caseTable->setHorizontalHeaderLabels({ QStringLiteral("序号"), QStringLiteral("工况名称"),
            QStringLiteral("开始时间 T0"), QStringLiteral("结束时间 T0 + t"), QStringLiteral("持续时间 t"),
            m_wizard->m_stepSpec.isAuto ? QStringLiteral("步长（初始/最小/最大）") : QStringLiteral("步长（固定）") });
        refreshTable();
    }

    void mTimeSeriesCaseStepWidget::slot_sequenceNameChanged(const QString& text)
    {
        m_wizard->m_sequenceName = text.trimmed().isEmpty() ? QStringLiteral("HeatStep") : text.trimmed();
        refreshTable();
    }

    void mTimeSeriesCaseStepWidget::slot_updateStep()
    {
        // “更新步长”：确认当前步长输入并刷新工况表步长列
        refreshTable();
        QMessageBox::information(this, QStringLiteral("更新步长"), QStringLiteral("已更新步长"));
    }

    void mTimeSeriesCaseStepWidget::regenerateCases()
    {
        m_wizard->m_cases.clear();

        if (m_methodCombo->currentIndex() == 0)
        {
            // 时序参数：均分总持续时间（数量钳回 [1,100000]，与原 SpinBox 范围一致）
            const double start = qMax(0.0, calcEditValue(m_startTimeEdit, 0.0));
            const double total = qMax(0.0, calcEditValue(m_totalTimeEdit, 0.0));
            const int count = qBound(1, qRound(calcEditValue(m_caseCountEdit, 1.0)), 100000);
            const double duration = total / count;
            for (int i = 0; i < count; ++i)
            {
                TimeSeriesCaseSpec spec;
                spec.no = i + 1;
                spec.start = start + i * duration;
                spec.end = start + (i + 1) * duration;
                spec.duration = duration;
                spec.name = QStringLiteral("%1_%2_%3").arg(m_wizard->m_sequenceName,
                    timeSeriesTimeToken(spec.start), timeSeriesTimeToken(spec.end));
                m_wizard->m_cases.append(spec);
            }
        }
        else
        {
            // 时序文件：读取前两列
            const QString path = m_filePathEdit->text().trimmed();
            if (path.isEmpty())
            {
                refreshTable();
                return;
            }
            const TimeSeriesParseResult result = TimeSeriesFileParser::parseTimeSequenceFile(path);
            if (!result.ok)
            {
                QMessageBox::warning(this, QStringLiteral("时序文件解析失败"), result.error);
                refreshTable();
                return;
            }
            for (int i = 0; i < result.timeRanges.size(); ++i)
            {
                TimeSeriesCaseSpec spec;
                spec.no = i + 1;
                spec.start = result.timeRanges.at(i).first;
                spec.end = result.timeRanges.at(i).second;
                spec.duration = spec.end - spec.start;
                spec.name = QStringLiteral("%1_%2_%3").arg(m_wizard->m_sequenceName,
                    timeSeriesTimeToken(spec.start), timeSeriesTimeToken(spec.end));
                m_wizard->m_cases.append(spec);
            }
        }

        refreshTable();
    }

    QString mTimeSeriesCaseStepWidget::stepDescription(double duration) const
    {
        // 单位折算：百分比 → 乘以当前工况持续时间
        const bool percent = (m_stepUnitCombo->currentIndex() == 1);
        const auto actual = [percent, duration](double value) {
            return percent ? duration * value / 100.0 : value;
        };

        if (m_wizard->m_stepSpec.isAuto)
        {
            return QStringLiteral("自动：%1 / %2 / %3 s")
                .arg(actual(calcEditValue(m_initialStepEdit, 0.0)))
                .arg(actual(calcEditValue(m_minStepEdit, 0.0)))
                .arg(actual(calcEditValue(m_maxStepEdit, 0.0)));
        }
        return QStringLiteral("固定 %1 s").arg(actual(calcEditValue(m_fixedStepEdit, 0.0)));
    }

    void mTimeSeriesCaseStepWidget::refreshTable()
    {
        // 工况名称跟随序列名称实时刷新
        for (TimeSeriesCaseSpec& spec : m_wizard->m_cases)
        {
            spec.name = QStringLiteral("%1_%2_%3").arg(m_wizard->m_sequenceName,
                timeSeriesTimeToken(spec.start), timeSeriesTimeToken(spec.end));
        }

        m_caseTable->setRowCount(m_wizard->m_cases.size());
        for (int i = 0; i < m_wizard->m_cases.size(); ++i)
        {
            const TimeSeriesCaseSpec& spec = m_wizard->m_cases.at(i);
            auto makeItem = [](const QString& text) {
                return new QTableWidgetItem(text);
            };
            m_caseTable->setItem(i, 0, makeItem(QStringLiteral("%1").arg(spec.no, 3, 10, QLatin1Char('0'))));
            auto* nameItem = makeItem(spec.name);
            QFont nameFont = nameItem->font();
            nameFont.setBold(true);
            nameItem->setFont(nameFont);
            nameItem->setForeground(QColor("#344054"));
            m_caseTable->setItem(i, 1, nameItem);
            m_caseTable->setItem(i, 2, makeItem(QStringLiteral("%1 s").arg(spec.start)));
            m_caseTable->setItem(i, 3, makeItem(QStringLiteral("%1 s").arg(spec.end)));
            m_caseTable->setItem(i, 4, makeItem(QStringLiteral("%1 s").arg(spec.duration)));
            m_caseTable->setItem(i, 5, makeItem(stepDescription(spec.duration)));
        }
    }

    bool mTimeSeriesCaseStepWidget::validatePage(QString* error)
    {
        // 采集时间步参数并折算为秒（按最大工况时长折算百分比）
        double maxDuration = 0.0;
        for (const TimeSeriesCaseSpec& spec : m_wizard->m_cases)
            maxDuration = qMax(maxDuration, spec.duration);

        const bool percent = (m_stepUnitCombo->currentIndex() == 1);
        const auto actual = [percent, maxDuration](double value) {
            return percent ? maxDuration * value / 100.0 : value;
        };
        m_wizard->m_stepSpec.fixed = actual(calcEditValue(m_fixedStepEdit, 0.0));
        m_wizard->m_stepSpec.initial = actual(calcEditValue(m_initialStepEdit, 0.0));
        m_wizard->m_stepSpec.minStep = actual(calcEditValue(m_minStepEdit, 0.0));
        m_wizard->m_stepSpec.maxStep = actual(calcEditValue(m_maxStepEdit, 0.0));

        if (m_wizard->m_cases.isEmpty())
        {
            if (error) *error = QStringLiteral("请先生成时序工况");
            return false;
        }
        if (m_wizard->m_stepSpec.isAuto)
        {
            if (m_wizard->m_stepSpec.minStep <= 0.0 || m_wizard->m_stepSpec.maxStep <= 0.0
                || m_wizard->m_stepSpec.minStep > m_wizard->m_stepSpec.maxStep)
            {
                if (error) *error = QStringLiteral("时间步长不合法：最小步长必须大于0且不大于最大步长");
                return false;
            }
        }
        else if (m_wizard->m_stepSpec.fixed <= 0.0)
        {
            if (error) *error = QStringLiteral("固定时间步长必须大于0");
            return false;
        }
        return true;
    }
}
