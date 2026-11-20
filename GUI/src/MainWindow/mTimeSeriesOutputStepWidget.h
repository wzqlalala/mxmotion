#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mTimeSeriesHeatWizardTypes.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
QT_END_NAMESPACE

namespace MechMainWindow
{
    class mCaseTimeSeriesHeatWizard;

    /*
    * 向导第4步：输出设置（结果输出）
    *
    * 输出格式（mxdb/op2）+ 4 个可请求变量（NT/GRADT/EGRADT/HFL，与工况“热学”组
    * 温度/节点温度梯度/单元温度梯度/热流密度一一对应）独立勾选，
    * 每个变量带集合下拉（默认全局；节点类仅列 NodeSet，单元类列 ElementSet/ElementSurface）。
    * 勾选与集合选择保存在向导数据中，关闭再打开不丢失。
    */
    class MMAINWINDOW_EXPORT mTimeSeriesOutputStepWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit mTimeSeriesOutputStepWidget(mCaseTimeSeriesHeatWizard* wizard);

        bool validatePage(QString* error);

    public slots:
        /*
        * 清空本页内容（由向导底部“清除”按钮触发）
        */
        void clearPage();

    private:
        void buildUi();
        void syncToSpec();

        mCaseTimeSeriesHeatWizard* m_wizard = nullptr;

        QComboBox* m_formatCombo = nullptr;

        struct VariableRow
        {
            QString solverName;     // NT / GRADT / HFL / RFL / HGEN
            QString displayName;    // 温度 / 温度梯度 / ...
            QCheckBox* check = nullptr;
            QComboBox* scope = nullptr;
        };
        QList<VariableRow> m_variables;
    };
}
