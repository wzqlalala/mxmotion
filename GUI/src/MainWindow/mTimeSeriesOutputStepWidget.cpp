#include "mTimeSeriesOutputStepWidget.h"
#pragma execution_character_set("utf-8")

#include "mCaseTimeSeriesHeatWizard.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mSetManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace MechMainWindow
{
    mTimeSeriesOutputStepWidget::mTimeSeriesOutputStepWidget(mCaseTimeSeriesHeatWizard* wizard)
        : QWidget(wizard), m_wizard(wizard)
    {
        buildUi();
    }

    void mTimeSeriesOutputStepWidget::buildUi()
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 20, 0, 0);
        mainLayout->setSpacing(15);

        auto* title = new QLabel(QStringLiteral("结果输出"), this);
        title->setStyleSheet(QStringLiteral("font-size:16px;font-weight:600;color:#344054;"));
        mainLayout->addWidget(title);

        // ===== 输出格式 =====
        auto* formatRow = new QHBoxLayout();
        formatRow->setSpacing(12);
        auto* formatLabel = new QLabel(QStringLiteral("输出格式"), this);
        formatLabel->setFixedWidth(92);
        formatLabel->setStyleSheet(QStringLiteral("color:#526070;font-size:13px;"));
        m_formatCombo = new QComboBox(this);
        m_formatCombo->addItem(QStringLiteral("mxdb"));
        m_formatCombo->addItem(QStringLiteral("op2"));
        m_formatCombo->setFixedWidth(220);
        m_formatCombo->setFixedHeight(30);
        m_formatCombo->setCurrentText(m_wizard->m_outputSpec.format);
        formatRow->addWidget(formatLabel);
        formatRow->addWidget(m_formatCombo);
        formatRow->addStretch();
        mainLayout->addLayout(formatRow);
        mainLayout->addSpacing(7);

        // ===== 物理量设置 =====
        auto* caption = new QLabel(QStringLiteral("物理量设置："), this);
        caption->setStyleSheet(QStringLiteral("color:#526070;font-size:13px;"));
        mainLayout->addWidget(caption);

        // 与工况“热学”组的四个输出变量一一对应（见 mCaseTransientHeatPanel / _bhStringOutput）
        const QList<QPair<QString, QString>> variables = {
            { QStringLiteral("NT"),      QStringLiteral("温度") },
            { QStringLiteral("GRADT"),   QStringLiteral("节点温度梯度") },
            { QStringLiteral("EGRADT"),  QStringLiteral("单元温度梯度") },
            { QStringLiteral("HFL"),     QStringLiteral("热流密度") }
        };
        // 默认勾选：温度、热流密度
        const QSet<QString> defaultChecked = { QStringLiteral("NT"), QStringLiteral("HFL") };
        if (m_wizard->m_outputSpec.variables.isEmpty())
            m_wizard->m_outputSpec.variables = defaultChecked;

        // 节点类变量集合下拉只列 NodeSet，单元类列 ElementSet/ElementSurface
        const QSet<QString> elementVariables = { QStringLiteral("EGRADT") };
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;

        for (const auto& pair : variables)
        {
            VariableRow row;
            row.solverName = pair.first;
            row.displayName = pair.second;

            auto* rowLayout = new QHBoxLayout();
            rowLayout->setSpacing(12);
            row.check = new QCheckBox(QStringLiteral("%1  %2").arg(pair.second, pair.first), this);
            row.check->setChecked(m_wizard->m_outputSpec.variables.contains(pair.first));
            row.check->setStyleSheet(QStringLiteral("QCheckBox{color:#344054;font-size:13px;spacing:8px;}"
                "QCheckBox::indicator{width:15px;height:15px;}"));
            row.scope = new QComboBox(this);
            row.scope->addItem(QStringLiteral("全局"));
            // 集合选项来自当前模型中的真实集合（无集合时仅“全局”可选）
            if (modelData)
            {
                auto& setManager = modelData->getManager<MechSetData::mSetManager>(MxFunctions::Physical_Set);
                if (elementVariables.contains(pair.first))
                {
                    for (MxFunctions::SetType type : { MxFunctions::SetType::Element,
                        MxFunctions::SetType::ElementSurface })
                    {
                        row.scope->addItems(setManager.getAllSetNamesByTypes(type));
                    }
                }
                else
                {
                    row.scope->addItems(setManager.getAllSetNamesByTypes(MxFunctions::SetType::Node));
                }
            }
            // 恢复已保存的集合选择
            const QString savedSet = m_wizard->m_outputSpec.variableSets.value(pair.first).value(0);
            if (!savedSet.isEmpty())
            {
                const int idx = row.scope->findText(savedSet);
                if (idx >= 0)
                    row.scope->setCurrentIndex(idx);
            }
            row.scope->setFixedWidth(118);
            row.scope->setFixedHeight(30);
            row.scope->setEnabled(row.check->isChecked());

            connect(row.check, &QCheckBox::toggled, this, [this, row](bool checked) mutable {
                row.scope->setEnabled(checked);
                syncToSpec();
            });
            connect(row.scope, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int) { syncToSpec(); });

            rowLayout->addWidget(row.check, 1);
            rowLayout->addWidget(row.scope);
            mainLayout->addLayout(rowLayout);
            m_variables.append(row);
        }

        mainLayout->addStretch();

        connect(m_formatCombo, &QComboBox::currentTextChanged, this, [this](const QString&) { syncToSpec(); });
    }

    void mTimeSeriesOutputStepWidget::clearPage()
    {
        for (const VariableRow& row : m_variables)
            row.check->setChecked(false);
        m_formatCombo->setCurrentText(QStringLiteral("mxdb"));
        syncToSpec();
    }

    void mTimeSeriesOutputStepWidget::syncToSpec()
    {
        m_wizard->m_outputSpec.format = m_formatCombo->currentText();
        m_wizard->m_outputSpec.variables.clear();
        m_wizard->m_outputSpec.variableSets.clear();
        for (const VariableRow& row : m_variables)
        {
            if (!row.check->isChecked())
                continue;
            m_wizard->m_outputSpec.variables.insert(row.solverName);
            // “全局”记为空集合列表（全模型），否则记录集合名称
            const QString setName = row.scope->currentText();
            if (!setName.isEmpty() && setName != QStringLiteral("全局"))
                m_wizard->m_outputSpec.variableSets.insert(row.solverName, { setName });
        }
    }

    bool mTimeSeriesOutputStepWidget::validatePage(QString* error)
    {
        syncToSpec();
        // 四个输出变量至少勾选一项
        if (m_wizard->m_outputSpec.variables.isEmpty())
        {
            if (error) *error = QStringLiteral("请至少勾选一个输出变量");
            return false;
        }
        return true;
    }
}
