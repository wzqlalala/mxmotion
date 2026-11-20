#include "mTimeSeriesBoundaryStepWidget.h"
#pragma execution_character_set("utf-8")

#include "mCaseTimeSeriesHeatWizard.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mSetManager.h"

#include <QComboBox>
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
            "QTableWidget::item:selected{background:#D0E7FD;color:#2C3E50;}"
            "QHeaderView::section:horizontal{background-color:#E8EBF2;color:#2C3E50;"
            "  border:0;border-right:1px solid #DCDFE6;border-bottom:1px solid #DCDFE6;"
            "  height:29px;font-size:12px;padding-left:8px;}";

        const char* kPrimaryBtnStyle =
            "QPushButton{background:#168ff0;border:1px solid #168ff0;border-radius:3px;color:#fff;}"
            "QPushButton:hover{background:#3ba2f3;border-color:#3ba2f3;}"
            "QPushButton:disabled{color:#b6bec9;border-color:#e7ebf0;background:#f3f5f7;}";

        const char* kDangerBtnStyle =
            "QPushButton{border:1px solid #e3a4a4;color:#d64545;background:#fff;border-radius:3px;}"
            "QPushButton:hover{border-color:#d64545;}";

        const char* kPaneTitleStyle = "font-size:13px;color:#4b5058;";

        /*
        * mCalcLineEdit 数值读取：空/非法表达式时回退默认温度 20℃
        */
        double tempValue(const mCalcLineEdit* edit)
        {
            bool ok = false;
            const double v = edit->text().trimmed().toDouble(&ok);
            return ok ? v : 20.0;
        }
    }

    mTimeSeriesBoundaryStepWidget::mTimeSeriesBoundaryStepWidget(mCaseTimeSeriesHeatWizard* wizard)
        : QWidget(wizard), m_wizard(wizard)
    {
        buildUi();
        // 恢复向导已保存的数据（关闭再打开时页面内容不清除）
        reloadFromData();
    }

    void mTimeSeriesBoundaryStepWidget::buildUi()
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(10);

        // ===== 上半：边界条件列表 + 边界条件信息 =====
        auto* topRow = new QHBoxLayout();
        topRow->setSpacing(10);

        auto* listPane = new QWidget(this);
        auto* listLayout = new QVBoxLayout(listPane);
        listLayout->setContentsMargins(0, 0, 0, 0);
        listLayout->setSpacing(6);
        auto* listTitle = new QLabel(QStringLiteral("边界条件列表"), listPane);
        listTitle->setStyleSheet(kPaneTitleStyle);

        m_boundaryList = new QTableWidget(0, 3, listPane);
        m_boundaryList->setHorizontalHeaderLabels({ QStringLiteral("名称"), QStringLiteral("类型"),
            QStringLiteral("关联工况") });
        m_boundaryList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_boundaryList->verticalHeader()->setVisible(false);
        m_boundaryList->verticalHeader()->setDefaultSectionSize(33);
        m_boundaryList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_boundaryList->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_boundaryList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_boundaryList->setAlternatingRowColors(true);
        m_boundaryList->setShowGrid(false);
        m_boundaryList->setStyleSheet(kTableStyle);

        auto* listActions = new QHBoxLayout();
        listActions->setSpacing(6);
        m_newButton = new QPushButton(QStringLiteral("新建"), listPane);
        m_editButton = new QPushButton(QStringLiteral("编辑"), listPane);
        m_deleteButton = new QPushButton(QStringLiteral("删除"), listPane);
        m_deleteButton->setStyleSheet(kDangerBtnStyle);
        listActions->addStretch();
        listActions->addWidget(m_newButton);
        listActions->addWidget(m_editButton);
        listActions->addWidget(m_deleteButton);
        listLayout->addWidget(listTitle);
        listLayout->addWidget(m_boundaryList, 1);
        listLayout->addLayout(listActions);

        auto* editorPane = new QWidget(this);
        auto* editorLayout = new QVBoxLayout(editorPane);
        editorLayout->setContentsMargins(0, 0, 0, 0);
        editorLayout->setSpacing(10);
        auto* editorTitle = new QLabel(QStringLiteral("边界条件信息"), editorPane);
        editorTitle->setStyleSheet(kPaneTitleStyle);
        editorLayout->addWidget(editorTitle);

        auto makeEditorRow = [this, editorLayout](const QString& label, QWidget* widget) {
            auto* row = new QHBoxLayout();
            row->setSpacing(10);
            auto* lbl = new QLabel(label, this);
            lbl->setFixedWidth(92);
            lbl->setStyleSheet(QStringLiteral("font-size:12px;color:#666e77;"));
            row->addWidget(lbl);
            row->addWidget(widget, 1);
            editorLayout->addLayout(row);
        };

        m_nameEdit = new QLineEdit(editorPane);
        m_nameEdit->setText(QStringLiteral("TemperatureBoundary"));
        m_typeCombo = new QComboBox(editorPane);
        m_typeCombo->addItem(QStringLiteral("温度边界"));
        m_typeCombo->setEnabled(false);
        m_setCombo = new QComboBox(editorPane);

        // 温度：数值输入框（不引用文件、不关联分析场）；mCalcLineEdit 支持科学计数法/表达式，
        // 允许负温（NoEmpty），范围与原 SpinBox 一致 [-1e6, 1e6]
        m_tempEdit = new mCalcLineEdit(QStringLiteral("20"), mCalcLineEdit::CalcMode::NoEmpty, editorPane);
        m_tempEdit->setRange(-1e6, 1e6);
        m_tempEdit->setUnit(QStringLiteral("℃"));

        m_saveButton = new QPushButton(QStringLiteral("保存"), editorPane);
        m_saveButton->setStyleSheet(kPrimaryBtnStyle);
        auto* saveRow = new QHBoxLayout();
        saveRow->addStretch();
        saveRow->addWidget(m_saveButton);

        makeEditorRow(QStringLiteral("名称"), m_nameEdit);
        makeEditorRow(QStringLiteral("类型"), m_typeCombo);
        makeEditorRow(QStringLiteral("集合"), m_setCombo);
        makeEditorRow(QStringLiteral("温度"), m_tempEdit);
        editorLayout->addStretch();
        editorLayout->addLayout(saveRow);

        topRow->addWidget(listPane, 1);
        topRow->addWidget(editorPane, 1);
        mainLayout->addLayout(topRow);

        // ===== 下半：工况边界配置表 =====
        auto* tableTitle = new QLabel(QStringLiteral("工况边界配置"), this);
        tableTitle->setStyleSheet(kPaneTitleStyle);
        mainLayout->addWidget(tableTitle);

        m_configTable = new QTableWidget(this);
        m_configTable->verticalHeader()->setVisible(false);
        m_configTable->verticalHeader()->setDefaultSectionSize(33);
        m_configTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_configTable->setSelectionMode(QAbstractItemView::NoSelection);
        m_configTable->setAlternatingRowColors(true);
        m_configTable->setShowGrid(false);
        m_configTable->setStyleSheet(kTableStyle);
        mainLayout->addWidget(m_configTable, 1);

        connect(m_newButton, &QPushButton::clicked, this, &mTimeSeriesBoundaryStepWidget::slot_newBoundary);
        connect(m_editButton, &QPushButton::clicked, this, &mTimeSeriesBoundaryStepWidget::slot_editBoundary);
        connect(m_deleteButton, &QPushButton::clicked, this, &mTimeSeriesBoundaryStepWidget::slot_deleteBoundary);
        connect(m_saveButton, &QPushButton::clicked, this, &mTimeSeriesBoundaryStepWidget::slot_saveBoundary);
        connect(m_boundaryList, &QTableWidget::cellClicked, this, &mTimeSeriesBoundaryStepWidget::slot_selectBoundary);
        connect(m_setCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mTimeSeriesBoundaryStepWidget::slot_setChanged);
        connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString&) { refreshConfigTable(); });
        connect(m_tempEdit, &QLineEdit::textChanged,
            this, [this](const QString&) { refreshConfigTable(); });

        refreshSetCombo();
        refreshBoundaryList();
        refreshConfigTable();
    }

    void mTimeSeriesBoundaryStepWidget::reloadFromData()
    {
        // 关闭再打开时恢复页面：集合下拉刷新（模型集合可能已变化），列表与编辑区按当前数据回显
        refreshSetCombo();
        refreshBoundaryList();
        if (m_selectedIndex >= 0 && m_selectedIndex < m_wizard->m_boundaryItems.size())
            enterEditMode(m_selectedIndex);
        else
            enterEditMode(-1);
    }

    void mTimeSeriesBoundaryStepWidget::refreshSetCombo()
    {
        m_setCombo->blockSignals(true);
        m_setCombo->clear();
        m_setCombo->addItem(QStringLiteral("未定义"));

        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData)
        {
            auto& setManager = modelData->getManager<MechSetData::mSetManager>(MxFunctions::Physical_Set);
            m_setCombo->addItems(setManager.getAllSetNamesByTypes(MxFunctions::SetType::Node));
        }
        m_setCombo->blockSignals(false);
    }

    void mTimeSeriesBoundaryStepWidget::refreshBoundaryList()
    {
        m_boundaryList->blockSignals(true);
        m_boundaryList->setRowCount(m_wizard->m_boundaryItems.size());
        for (int i = 0; i < m_wizard->m_boundaryItems.size(); ++i)
        {
            const TimeSeriesBoundaryItem& item = m_wizard->m_boundaryItems.at(i);
            auto makeItem = [](const QString& text) { return new QTableWidgetItem(text); };
            auto* nameItem = makeItem(item.name);
            QFont font = nameItem->font();
            font.setBold(true);
            nameItem->setFont(font);
            m_boundaryList->setItem(i, 0, nameItem);
            m_boundaryList->setItem(i, 1, makeItem(QStringLiteral("温度边界")));
            m_boundaryList->setItem(i, 2, makeItem(QString::number(m_wizard->m_cases.size())));
        }
        if (m_selectedIndex >= 0 && m_selectedIndex < m_boundaryList->rowCount())
            m_boundaryList->selectRow(m_selectedIndex);
        m_boundaryList->blockSignals(false);
    }

    void mTimeSeriesBoundaryStepWidget::refreshConfigTable()
    {
        QStringList headers = { QStringLiteral("序号"), QStringLiteral("工况名称"),
            QStringLiteral("边界名称"), QStringLiteral("作用位置"), QStringLiteral("温度") };
        m_configTable->setColumnCount(headers.size());
        m_configTable->setHorizontalHeaderLabels(headers);
        m_configTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        const QString baseName = m_nameEdit->text().trimmed().isEmpty()
            ? QStringLiteral("TemperatureBoundary") : m_nameEdit->text().trimmed();
        const QString setText = (m_setCombo->currentText() == QStringLiteral("未定义")
            || m_setCombo->currentText().isEmpty())
            ? QStringLiteral("全模型") : m_setCombo->currentText();

        // 边界不与工况一一对应创建：配置表仅作预览，展示当前编辑项应用到各工况的效果
        m_configTable->setRowCount(m_wizard->m_cases.size());

        const QString tempText = QStringLiteral("%1 ℃").arg(tempValue(m_tempEdit));
        for (int i = 0; i < m_wizard->m_cases.size(); ++i)
        {
            const TimeSeriesCaseSpec& spec = m_wizard->m_cases.at(i);
            auto makeItem = [](const QString& text) { return new QTableWidgetItem(text); };
            m_configTable->setItem(i, 0, makeItem(QStringLiteral("%1").arg(spec.no, 3, 10, QLatin1Char('0'))));
            auto* nameItem = makeItem(spec.name);
            QFont font = nameItem->font();
            font.setBold(true);
            nameItem->setFont(font);
            m_configTable->setItem(i, 1, nameItem);
            m_configTable->setItem(i, 2, makeItem(baseName));
            m_configTable->setItem(i, 3, makeItem(setText));
            m_configTable->setItem(i, 4, makeItem(tempText));
        }
    }

    void mTimeSeriesBoundaryStepWidget::enterEditMode(int index)
    {
        m_editingIndex = index;
        if (index >= 0 && index < m_wizard->m_boundaryItems.size())
        {
            const TimeSeriesBoundaryItem& item = m_wizard->m_boundaryItems.at(index);
            m_nameEdit->setText(item.name);
            m_setCombo->setCurrentText(item.setName.isEmpty() ? QStringLiteral("未定义") : item.setName);
            m_tempEdit->setExpression(QString::number(item.temperature));
        }
        else
        {
            m_nameEdit->setText(QStringLiteral("TemperatureBoundary"));
            m_setCombo->setCurrentIndex(0);
            m_tempEdit->setExpression(QStringLiteral("20"));
        }
        refreshConfigTable();
    }

    void mTimeSeriesBoundaryStepWidget::clearPage()
    {
        m_wizard->m_boundaryItems.clear();
        m_editingIndex = -1;
        m_selectedIndex = -1;
        refreshBoundaryList();
        enterEditMode(-1);
        emit dataModified();
    }

    void mTimeSeriesBoundaryStepWidget::slot_newBoundary()
    {
        m_selectedIndex = -1;
        m_boundaryList->clearSelection();
        enterEditMode(-1);
    }

    void mTimeSeriesBoundaryStepWidget::slot_editBoundary()
    {
        if (m_selectedIndex >= 0 && m_selectedIndex < m_wizard->m_boundaryItems.size())
            enterEditMode(m_selectedIndex);
    }

    void mTimeSeriesBoundaryStepWidget::slot_deleteBoundary()
    {
        if (m_selectedIndex < 0 || m_selectedIndex >= m_wizard->m_boundaryItems.size())
            return;
        const QString name = m_wizard->m_boundaryItems.at(m_selectedIndex).name;
        m_wizard->m_boundaryItems.removeAt(m_selectedIndex);
        if (m_editingIndex == m_selectedIndex)
            m_editingIndex = -1;
        else if (m_editingIndex > m_selectedIndex)
            --m_editingIndex;
        m_selectedIndex = qMin(m_selectedIndex, m_wizard->m_boundaryItems.size() - 1);
        refreshBoundaryList();
        refreshConfigTable();
        emit dataModified();
        QMessageBox::information(this, QStringLiteral("删除边界"), QStringLiteral("已删除“%1”").arg(name));
    }

    void mTimeSeriesBoundaryStepWidget::slot_selectBoundary(int row, int /*column*/)
    {
        m_selectedIndex = row;
    }

    void mTimeSeriesBoundaryStepWidget::slot_setChanged(int /*index*/)
    {
        refreshConfigTable();
    }

    void mTimeSeriesBoundaryStepWidget::slot_saveBoundary()
    {
        TimeSeriesBoundaryItem item;
        item.name = m_nameEdit->text().trimmed();
        if (item.name.isEmpty())
            item.name = QStringLiteral("TemperatureBoundary");
        const QString setText = m_setCombo->currentText();
        item.setName = (setText == QStringLiteral("未定义")) ? QString() : setText;
        item.temperature = tempValue(m_tempEdit);

        if (m_editingIndex >= 0 && m_editingIndex < m_wizard->m_boundaryItems.size())
        {
            m_wizard->m_boundaryItems[m_editingIndex] = item;
        }
        else
        {
            m_wizard->m_boundaryItems.append(item);
            m_editingIndex = m_wizard->m_boundaryItems.size() - 1;
            m_selectedIndex = m_editingIndex;
        }
        refreshBoundaryList();
        refreshConfigTable();
        emit dataModified();
        QMessageBox::information(this, QStringLiteral("保存边界"), QStringLiteral("已保存“%1”").arg(item.name));
    }

    bool mTimeSeriesBoundaryStepWidget::validatePage(QString* /*error*/)
    {
        // 边界非必选：可整页跳过，保存即生效（温度输入框始终有值）
        return true;
    }
}
