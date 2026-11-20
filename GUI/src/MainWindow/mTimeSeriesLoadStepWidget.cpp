#include "mTimeSeriesLoadStepWidget.h"
#pragma execution_character_set("utf-8")

#include "mCaseTimeSeriesHeatWizard.h"
#include "mTimeSeriesFileParser.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mSetManager.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

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
        * 载荷类型对应的集合类型（用于过滤集合下拉）
        */
        MxFunctions::SetType setTypeForKind(TimeSeriesLoadKind kind)
        {
            switch (kind)
            {
            case TimeSeriesLoadKind::PointHeatFlux:   return MxFunctions::SetType::Node;
            case TimeSeriesLoadKind::VolumeHeatFlux:  return MxFunctions::SetType::Element;
            default:                                  return MxFunctions::SetType::ElementSurface;
            }
        }

        /*
        * 载荷类型对应的配置表附加列
        */
        QStringList extraColumnsForKind(TimeSeriesLoadKind kind)
        {
            switch (kind)
            {
            case TimeSeriesLoadKind::Convection:
                return { QStringLiteral("环境温度"), QStringLiteral("对流换热系数文件") };
            case TimeSeriesLoadKind::Radiation:
                return { QStringLiteral("环境温度"), QStringLiteral("热辐射系数") };
            default:
                return { QStringLiteral("载荷文件") };
            }
        }

        /*
        * 载荷集文件要求的最小列数
        */
        int requiredColumnsForKind(TimeSeriesLoadKind kind)
        {
            return (kind == TimeSeriesLoadKind::Convection || kind == TimeSeriesLoadKind::Radiation) ? 4 : 3;
        }

        /*
        * 载荷集文件附加列的期望表头（第3列起，用于判断文件内容与载荷类型是否匹配）
        */
        QStringList expectedHeadersForKind(TimeSeriesLoadKind kind)
        {
            switch (kind)
            {
            case TimeSeriesLoadKind::Convection:
                return { QStringLiteral("环境温度_℃"), QStringLiteral("对流换热系数文件") };
            case TimeSeriesLoadKind::Radiation:
                return { QStringLiteral("环境温度_℃"), QStringLiteral("热辐射系数") };
            default:
                return { QStringLiteral("场文件") };
            }
        }

        /*
        * 引用场文件的附加列（0基列号）：对流换热的系数文件、热流类载荷的场文件；
        * 热辐射系数为数值，不引用文件
        */
        QList<int> fileColumnsForKind(TimeSeriesLoadKind kind)
        {
            if (kind == TimeSeriesLoadKind::Convection)
                return { 3 };
            if (kind == TimeSeriesLoadKind::Radiation)
                return {};
            return { 2 };
        }
    }

    mTimeSeriesLoadStepWidget::mTimeSeriesLoadStepWidget(mCaseTimeSeriesHeatWizard* wizard)
        : QWidget(wizard), m_wizard(wizard)
    {
        buildUi();
        // 恢复向导已保存的数据（关闭再打开时页面内容不清除）
        reloadFromData();
    }

    void mTimeSeriesLoadStepWidget::buildUi()
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(10);

        // ===== 上半：载荷列表 + 载荷信息（等分） =====
        auto* topRow = new QHBoxLayout();
        topRow->setSpacing(10);

        // --- 载荷列表 ---
        auto* listPane = new QWidget(this);
        auto* listLayout = new QVBoxLayout(listPane);
        listLayout->setContentsMargins(0, 0, 0, 0);
        listLayout->setSpacing(6);
        auto* listTitle = new QLabel(QStringLiteral("载荷列表"), listPane);
        listTitle->setStyleSheet(kPaneTitleStyle);

        m_loadList = new QTableWidget(0, 3, listPane);
        m_loadList->setHorizontalHeaderLabels({ QStringLiteral("名称"), QStringLiteral("类型"),
            QStringLiteral("关联工况") });
        m_loadList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_loadList->verticalHeader()->setVisible(false);
        m_loadList->verticalHeader()->setDefaultSectionSize(33);
        m_loadList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_loadList->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_loadList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_loadList->setAlternatingRowColors(true);
        m_loadList->setShowGrid(false);
        m_loadList->setStyleSheet(kTableStyle);

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
        listLayout->addWidget(m_loadList, 1);
        listLayout->addLayout(listActions);

        // --- 载荷信息 ---
        auto* editorPane = new QWidget(this);
        auto* editorLayout = new QVBoxLayout(editorPane);
        editorLayout->setContentsMargins(0, 0, 0, 0);
        editorLayout->setSpacing(10);
        auto* editorTitle = new QLabel(QStringLiteral("载荷信息"), editorPane);
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
        m_typeCombo = new QComboBox(editorPane);
        m_typeCombo->addItem(QStringLiteral("点热流"), static_cast<int>(TimeSeriesLoadKind::PointHeatFlux));
        m_typeCombo->addItem(QStringLiteral("面热流"), static_cast<int>(TimeSeriesLoadKind::SurfaceHeatFlux));
        m_typeCombo->addItem(QStringLiteral("体热流"), static_cast<int>(TimeSeriesLoadKind::VolumeHeatFlux));
        m_typeCombo->addItem(QStringLiteral("对流换热"), static_cast<int>(TimeSeriesLoadKind::Convection));
        m_typeCombo->addItem(QStringLiteral("热辐射"), static_cast<int>(TimeSeriesLoadKind::Radiation));
        m_typeCombo->setCurrentIndex(1);
        m_setCombo = new QComboBox(editorPane);

        auto* fileWidget = new QWidget(editorPane);
        auto* fileLayout = new QHBoxLayout(fileWidget);
        fileLayout->setContentsMargins(0, 0, 0, 0);
        fileLayout->setSpacing(6);
        m_filePathEdit = new QLineEdit(fileWidget);
        m_filePathEdit->setReadOnly(true);
        m_filePathEdit->setPlaceholderText(QStringLiteral("请选择载荷集文件"));
        auto* browseButton = new QPushButton(QStringLiteral("…"), fileWidget);
        browseButton->setFixedSize(28, 28);
        browseButton->setToolTip(QStringLiteral("选择文件"));
        m_importButton = new QPushButton(QStringLiteral("导入"), fileWidget);
        fileLayout->addWidget(m_filePathEdit, 1);
        fileLayout->addWidget(browseButton);
        fileLayout->addWidget(m_importButton);

        m_saveButton = new QPushButton(QStringLiteral("保存"), editorPane);
        m_saveButton->setStyleSheet(kPrimaryBtnStyle);
        auto* saveRow = new QHBoxLayout();
        saveRow->addStretch();
        saveRow->addWidget(m_saveButton);

        makeEditorRow(QStringLiteral("名称"), m_nameEdit);
        makeEditorRow(QStringLiteral("类型"), m_typeCombo);
        makeEditorRow(QStringLiteral("集合"), m_setCombo);
        makeEditorRow(QStringLiteral("文件"), fileWidget);
        editorLayout->addStretch();
        editorLayout->addLayout(saveRow);

        topRow->addWidget(listPane, 1);
        topRow->addWidget(editorPane, 1);
        mainLayout->addLayout(topRow);

        // ===== 下半：工况载荷配置表（标题位于边框外） =====
        auto* tableTitle = new QLabel(QStringLiteral("工况载荷配置"), this);
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

        // ===== 信号 =====
        connect(m_newButton, &QPushButton::clicked, this, &mTimeSeriesLoadStepWidget::slot_newLoad);
        connect(m_editButton, &QPushButton::clicked, this, &mTimeSeriesLoadStepWidget::slot_editLoad);
        connect(m_deleteButton, &QPushButton::clicked, this, &mTimeSeriesLoadStepWidget::slot_deleteLoad);
        connect(m_saveButton, &QPushButton::clicked, this, &mTimeSeriesLoadStepWidget::slot_saveLoad);
        connect(m_importButton, &QPushButton::clicked, this, &mTimeSeriesLoadStepWidget::slot_importFile);
        connect(browseButton, &QPushButton::clicked, this, &mTimeSeriesLoadStepWidget::slot_browseFile);
        connect(m_loadList, &QTableWidget::cellClicked, this, &mTimeSeriesLoadStepWidget::slot_selectLoad);
        connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mTimeSeriesLoadStepWidget::slot_typeChanged);
        connect(m_setCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mTimeSeriesLoadStepWidget::slot_setChanged);
        connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString&) { refreshConfigTable(); });

        refreshSetCombo();
        refreshLoadList();
        refreshConfigTable();
    }

    TimeSeriesLoadKind mTimeSeriesLoadStepWidget::currentKind() const
    {
        return static_cast<TimeSeriesLoadKind>(m_typeCombo->currentData().toInt());
    }

    void mTimeSeriesLoadStepWidget::refreshSetCombo()
    {
        m_setCombo->blockSignals(true);
        m_setCombo->clear();
        m_setCombo->addItem(QStringLiteral("未定义"));

        // 集合下拉：显示当前模型中可选集合（按载荷类型过滤）
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData)
        {
            auto& setManager = modelData->getManager<MechSetData::mSetManager>(MxFunctions::Physical_Set);
            m_setCombo->addItems(setManager.getAllSetNamesByTypes(setTypeForKind(currentKind())));
        }
        m_setCombo->blockSignals(false);
    }

    void mTimeSeriesLoadStepWidget::refreshLoadList()
    {
        m_loadList->blockSignals(true);
        m_loadList->setRowCount(m_wizard->m_loadItems.size());
        for (int i = 0; i < m_wizard->m_loadItems.size(); ++i)
        {
            const TimeSeriesLoadItem& item = m_wizard->m_loadItems.at(i);
            auto makeItem = [](const QString& text) { return new QTableWidgetItem(text); };
            auto* nameItem = makeItem(item.name);
            QFont font = nameItem->font();
            font.setBold(true);
            nameItem->setFont(font);
            m_loadList->setItem(i, 0, nameItem);
            m_loadList->setItem(i, 1, makeItem(timeSeriesLoadKindName(item.kind)));
            m_loadList->setItem(i, 2, makeItem(QString::number(m_wizard->m_cases.size())));
        }
        if (m_selectedIndex >= 0 && m_selectedIndex < m_loadList->rowCount())
            m_loadList->selectRow(m_selectedIndex);
        m_loadList->blockSignals(false);
    }

    void mTimeSeriesLoadStepWidget::refreshConfigTable()
    {
        const TimeSeriesLoadKind kind = currentKind();
        const QStringList extras = extraColumnsForKind(kind);

        QStringList headers = { QStringLiteral("序号"), QStringLiteral("工况名称"),
            QStringLiteral("载荷名称"), QStringLiteral("作用位置") };
        headers.append(extras);
        m_configTable->setColumnCount(headers.size());
        m_configTable->setHorizontalHeaderLabels(headers);
        m_configTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        // 当前编辑项：仅导入后才显示工况行与数据（未导入时仅表头）
        QList<QStringList> rows;
        bool imported = false;
        if (m_editingIndex >= 0 && m_editingIndex < m_wizard->m_loadItems.size())
        {
            rows = m_wizard->m_loadItems.at(m_editingIndex).rows;
            imported = m_wizard->m_loadItems.at(m_editingIndex).imported;
        }
        m_configTable->setRowCount(imported ? m_wizard->m_cases.size() : 0);

        const QString baseName = m_nameEdit->text().trimmed().isEmpty()
            ? timeSeriesLoadDefaultName(kind) : m_nameEdit->text().trimmed();
        const QString setText = (m_setCombo->currentText() == QStringLiteral("未定义")
            || m_setCombo->currentText().isEmpty())
            ? QStringLiteral("全模型") : m_setCombo->currentText();

        for (int i = 0; imported && i < m_wizard->m_cases.size(); ++i)
        {
            const TimeSeriesCaseSpec& spec = m_wizard->m_cases.at(i);
            auto makeItem = [](const QString& text) { return new QTableWidgetItem(text); };
            m_configTable->setItem(i, 0, makeItem(QStringLiteral("%1").arg(spec.no, 3, 10, QLatin1Char('0'))));
            auto* nameItem = makeItem(spec.name);
            QFont font = nameItem->font();
            font.setBold(true);
            nameItem->setFont(font);
            m_configTable->setItem(i, 1, nameItem);
            m_configTable->setItem(i, 2, makeItem(QStringLiteral("%1_%2_%3").arg(baseName,
                timeSeriesTimeToken(spec.start), timeSeriesTimeToken(spec.end))));
            m_configTable->setItem(i, 3, makeItem(setText));

            // 附加列：按导入的数据行刷新
            for (int c = 0; c < extras.size(); ++c)
            {
                QString value;
                if (i < rows.size())
                {
                    // 数据行：第1/2列时间，第3列起为附加字段
                    const QStringList& fields = rows.at(i);
                    const int fieldIndex = 2 + c;
                    if (fieldIndex < fields.size())
                        value = fields.at(fieldIndex).trimmed();
                }
                m_configTable->setItem(i, 4 + c, makeItem(value));
            }
        }
    }

    void mTimeSeriesLoadStepWidget::enterEditMode(int index)
    {
        m_editingIndex = index;
        if (index >= 0 && index < m_wizard->m_loadItems.size())
        {
            const TimeSeriesLoadItem& item = m_wizard->m_loadItems.at(index);
            m_nameEdit->setText(item.name);
            m_typeCombo->setCurrentIndex(m_typeCombo->findData(static_cast<int>(item.kind)));
            refreshSetCombo();
            m_setCombo->setCurrentText(item.setName.isEmpty() ? QStringLiteral("未定义") : item.setName);
            m_filePathEdit->setText(item.filePath);
        }
        else
        {
            m_nameEdit->setText(timeSeriesLoadDefaultName(currentKind()));
            refreshSetCombo();
            m_setCombo->setCurrentIndex(0);
            m_filePathEdit->clear();
        }
        refreshConfigTable();
    }

    void mTimeSeriesLoadStepWidget::clearPage()
    {
        m_wizard->m_loadItems.clear();
        m_editingIndex = -1;
        m_selectedIndex = -1;
        refreshLoadList();
        enterEditMode(-1);
        emit dataModified();
    }

    void mTimeSeriesLoadStepWidget::slot_newLoad()
    {
        m_selectedIndex = -1;
        m_loadList->clearSelection();
        enterEditMode(-1);
    }

    void mTimeSeriesLoadStepWidget::slot_editLoad()
    {
        if (m_selectedIndex >= 0 && m_selectedIndex < m_wizard->m_loadItems.size())
            enterEditMode(m_selectedIndex);
    }

    void mTimeSeriesLoadStepWidget::slot_deleteLoad()
    {
        if (m_selectedIndex < 0 || m_selectedIndex >= m_wizard->m_loadItems.size())
            return;
        const QString name = m_wizard->m_loadItems.at(m_selectedIndex).name;
        m_wizard->m_loadItems.removeAt(m_selectedIndex);
        if (m_editingIndex == m_selectedIndex)
            m_editingIndex = -1;
        else if (m_editingIndex > m_selectedIndex)
            --m_editingIndex;
        m_selectedIndex = qMin(m_selectedIndex, m_wizard->m_loadItems.size() - 1);
        refreshLoadList();
        refreshConfigTable();
        emit dataModified();
        QMessageBox::information(this, QStringLiteral("删除载荷"), QStringLiteral("已删除“%1”").arg(name));
    }

    void mTimeSeriesLoadStepWidget::slot_selectLoad(int row, int /*column*/)
    {
        // 仅更新选中状态，不带入编辑区（需点击“编辑”）
        m_selectedIndex = row;
    }

    void mTimeSeriesLoadStepWidget::slot_typeChanged(int /*index*/)
    {
        refreshSetCombo();
        if (m_editingIndex < 0)
            m_nameEdit->setText(timeSeriesLoadDefaultName(currentKind()));
        refreshConfigTable();
    }

    void mTimeSeriesLoadStepWidget::slot_setChanged(int /*index*/)
    {
        refreshConfigTable();
    }

    void mTimeSeriesLoadStepWidget::reloadFromData()
    {
        // 恢复向导数据：集合下拉刷新（模型集合可能已变化），列表与编辑区按当前数据回显
        refreshSetCombo();
        refreshLoadList();
        if (m_selectedIndex >= 0 && m_selectedIndex < m_wizard->m_loadItems.size())
            enterEditMode(m_selectedIndex);
        else
            enterEditMode(-1);
    }

    void mTimeSeriesLoadStepWidget::slot_browseFile()
    {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择载荷集文件"),
            QString(), QStringLiteral("载荷集文件 (*.txt)"));
        if (!path.isEmpty())
            m_filePathEdit->setText(path);
    }

    void mTimeSeriesLoadStepWidget::slot_importFile()
    {
        const QString path = m_filePathEdit->text().trimmed();
        if (path.isEmpty())
        {
            QMessageBox::warning(this, QStringLiteral("导入载荷集文件"), QStringLiteral("请先选择载荷集文件"));
            return;
        }

        const TimeSeriesLoadKind kind = currentKind();
        QList<int> numericColumns;
        if (kind == TimeSeriesLoadKind::Convection)
            numericColumns = { 2 };         // 环境温度为数值，系数文件为路径
        else if (kind == TimeSeriesLoadKind::Radiation)
            numericColumns = { 2, 3 };      // 环境温度、热辐射系数均为数值

        const TimeSeriesParseResult result = TimeSeriesFileParser::parseLoadSetFile(
            path, requiredColumnsForKind(kind), expectedHeadersForKind(kind),
            numericColumns, fileColumnsForKind(kind));
        if (!result.ok)
        {
            QMessageBox::warning(this, QStringLiteral("载荷集文件解析失败"), result.error);
            return;
        }

        // 校验文件工况数与向导工况数一致
        if (result.rows.size() != m_wizard->m_cases.size())
        {
            QMessageBox::warning(this, QStringLiteral("导入载荷集文件"),
                QStringLiteral("文件包含 %1 个工况，与当前 %2 个时序工况不一致")
                .arg(result.rows.size()).arg(m_wizard->m_cases.size()));
            return;
        }

        // 暂存到正在编辑的项（保存时写入列表）
        if (m_editingIndex >= 0 && m_editingIndex < m_wizard->m_loadItems.size())
        {
            m_wizard->m_loadItems[m_editingIndex].rows = result.rows;
            m_wizard->m_loadItems[m_editingIndex].imported = true;
            m_wizard->m_loadItems[m_editingIndex].filePath = path;
        }
        else
        {
            // 新建模式：先保存再标记导入
            slot_saveLoad();
            if (m_editingIndex >= 0 && m_editingIndex < m_wizard->m_loadItems.size())
            {
                m_wizard->m_loadItems[m_editingIndex].rows = result.rows;
                m_wizard->m_loadItems[m_editingIndex].imported = true;
                m_wizard->m_loadItems[m_editingIndex].filePath = path;
            }
        }
        refreshLoadList();
        refreshConfigTable();
        QMessageBox::information(this, QStringLiteral("导入载荷集文件"), QStringLiteral("已导入载荷集文件"));
    }

    void mTimeSeriesLoadStepWidget::slot_saveLoad()
    {
        TimeSeriesLoadItem item;
        item.name = m_nameEdit->text().trimmed();
        if (item.name.isEmpty())
            item.name = timeSeriesLoadDefaultName(currentKind());
        item.kind = currentKind();
        const QString setText = m_setCombo->currentText();
        item.setName = (setText == QStringLiteral("未定义")) ? QString() : setText;
        item.filePath = m_filePathEdit->text().trimmed();

        if (m_editingIndex >= 0 && m_editingIndex < m_wizard->m_loadItems.size())
        {
            // 类型或文件变化后，旧的导入数据不再适用，需重新导入
            const TimeSeriesLoadItem& old = m_wizard->m_loadItems.at(m_editingIndex);
            const bool reusable = (old.kind == item.kind && old.filePath == item.filePath);
            item.rows = reusable ? old.rows : QList<QStringList>();
            item.imported = reusable && old.imported;
            m_wizard->m_loadItems[m_editingIndex] = item;
        }
        else
        {
            m_wizard->m_loadItems.append(item);
            m_editingIndex = m_wizard->m_loadItems.size() - 1;
            m_selectedIndex = m_editingIndex;
        }
        refreshLoadList();
        refreshConfigTable();
        emit dataModified();
        QMessageBox::information(this, QStringLiteral("保存载荷"), QStringLiteral("已保存“%1”").arg(item.name));
    }

    bool mTimeSeriesLoadStepWidget::validatePage(QString* error)
    {
        // 载荷必选：至少有一个载荷且全部已导入载荷集文件
        if (m_wizard->m_loadItems.isEmpty())
        {
            if (error) *error = QStringLiteral("请至少创建一个载荷并导入载荷集文件");
            return false;
        }
        for (const TimeSeriesLoadItem& item : m_wizard->m_loadItems)
        {
            if (!item.imported)
            {
                if (error) *error = QStringLiteral("载荷“%1”尚未导入载荷集文件").arg(item.name);
                return false;
            }
        }
        return true;
    }
}
