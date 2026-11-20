#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mTimeSeriesHeatWizardTypes.h"
#include "mCalcLineEdit.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QComboBox;
class QPushButton;
class QTableWidget;
class QLabel;
QT_END_NAMESPACE

namespace MechMainWindow
{
    class mCaseTimeSeriesHeatWizard;

    /*
    * 向导第1步：工况设置
    *
    * 分析序列名称 + 工况创建（时序参数/时序文件）+ 时间步长（自动/固定 × 时间/百分比）
    * + 时序工况表。创建方式与文件路径持久保存在向导数据中（关闭再打开不丢失）。
    */
    class MMAINWINDOW_EXPORT mTimeSeriesCaseStepWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit mTimeSeriesCaseStepWidget(mCaseTimeSeriesHeatWizard* wizard);

        /*
        * 生成工况表（时序参数：按均分规则；时序文件：按解析结果）
        */
        void regenerateCases();

        /*
        * 校验当前页数据（供“下一步”门控）
        */
        bool validatePage(QString* error);

        /*
        * 恢复控件以显示当前向导数据（构造时与关闭再打开时调用）
        */
        void reloadFromData();

    public slots:
        /*
        * 清空本页内容（由向导底部“清除”按钮触发）
        */
        void clearPage();

    signals:
        /*
        * 页面数据被修改（生成工况/切换创建方式等），供向导重置后续步骤完成状态
        */
        void dataModified();

    private slots:
        void slot_methodChanged(int index);
        void slot_browseFile();
        void slot_generate();
        void slot_stepModeChanged(int index);
        void slot_updateStep();
        void slot_sequenceNameChanged(const QString& text);

    private:
        void buildUi();
        void refreshTable();
        QString stepDescription(double duration) const;

        mCaseTimeSeriesHeatWizard* m_wizard = nullptr;

        QLineEdit* m_sequenceNameEdit = nullptr;
        QComboBox* m_methodCombo = nullptr;

        // 时序参数（mCalcLineEdit：限制字符类型、支持科学计数法/表达式，焦点离开求值）
        QWidget* m_paramRow = nullptr;
        MxFunctions::mCalcLineEdit* m_startTimeEdit = nullptr;
        MxFunctions::mCalcLineEdit* m_totalTimeEdit = nullptr;
        MxFunctions::mCalcLineEdit* m_caseCountEdit = nullptr;

        // 时序文件
        QWidget* m_fileRow = nullptr;
        QLineEdit* m_filePathEdit = nullptr;

        QPushButton* m_generateButton = nullptr;

        // 时间步长
        QComboBox* m_stepModeCombo = nullptr;   // 自动/固定
        QComboBox* m_stepUnitCombo = nullptr;   // 时间/百分比
        QWidget* m_fixedStepWidget = nullptr;
        QWidget* m_autoStepWidget = nullptr;
        MxFunctions::mCalcLineEdit* m_fixedStepEdit = nullptr;
        MxFunctions::mCalcLineEdit* m_initialStepEdit = nullptr;
        MxFunctions::mCalcLineEdit* m_minStepEdit = nullptr;
        MxFunctions::mCalcLineEdit* m_maxStepEdit = nullptr;
        QPushButton* m_stepUpdateButton = nullptr;

        QTableWidget* m_caseTable = nullptr;
        QList<QLabel*> m_stepUnitLabels;
    };
}
