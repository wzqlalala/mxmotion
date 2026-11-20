#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mTimeSeriesHeatWizardTypes.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class QStackedWidget;
class QListWidget;
QT_END_NAMESPACE

class NavIconDelegate;

namespace MechMainWindow
{
    class mTimeSeriesCaseStepWidget;
    class mTimeSeriesLoadStepWidget;
    class mTimeSeriesBoundaryStepWidget;
    class mTimeSeriesOutputStepWidget;

    /*
    * 时序热分析工况创建向导
    *
    * 四步：工况设置 → 载荷设置 → 边界设置 → 输出设置。
    * 外层框架对齐 NTF/VTF 等 NVH 分析弹窗：系统标题栏、
    * 左侧 QListWidget + NavIconDelegate 步骤导航、分隔线、
    * 右侧 QStackedWidget 内容区、底部 上一步/下一步(完成) 按钮。
    * 向导只负责收集数据，所有模型对象在点击“完成”时由 mTimeSeriesHeatWizardCommit 统一创建。
    */
    class MMAINWINDOW_EXPORT mCaseTimeSeriesHeatWizard : public QDialog
    {
        Q_OBJECT

    public:
        mCaseTimeSeriesHeatWizard(QWidget* parent = nullptr);
        ~mCaseTimeSeriesHeatWizard() override;

        /*
        * 恢复向导默认状态（复用已关闭的窗口实例时调用）
        * 注意：仅重建步骤页并按当前向导数据回显，**不清空已收集的数据**——
        * 内容持久保存，关闭再打开不丢失；如需清空请使用各页的“清除”按钮。
        */
        void resetToDefault();

    private slots:
        void slot_previous();
        void slot_next();

        /*
        * 清除当前页内容
        */
        void slot_clearCurrentPage();

    private:
        void initUI();
        void initConnect();
        void initLeftNavigation();
        void updateStepUi();

        /*
        * 判断指定导航页是否可交互（仅允许回到已到达的步骤）
        */
        bool isNavigationIndexEnabled(int index) const;

        /*
        * 页面内容被修改后，将其后续步骤标记为未完成（对齐 NVH 弹窗）
        */
        void onPageModified(int pageIndex);

        bool validateCurrentStep();

        /*
        * 完成向导：校验通过后执行数据落库
        */
        void finishWizard();

    private:
        // 左侧竖向步骤导航
        QListWidget* m_navList = nullptr;
        NavIconDelegate* m_navIconDelegate = nullptr;

        QStackedWidget* m_stackWidget = nullptr;
        mTimeSeriesCaseStepWidget* m_caseStep = nullptr;
        mTimeSeriesLoadStepWidget* m_loadStep = nullptr;
        mTimeSeriesBoundaryStepWidget* m_boundaryStep = nullptr;
        mTimeSeriesOutputStepWidget* m_outputStep = nullptr;

        QPushButton* m_previousButton = nullptr;
        QPushButton* m_nextButton = nullptr;
        QPushButton* m_clearButton = nullptr;   // 清除当前页（底部，上一步左侧）

        int m_activeStep = 0;
        int m_farthestStep = 0;     // 最远到达步骤（已完成标记依据）

        // 向导收集的数据
        QString m_sequenceName = QStringLiteral("HeatStep");
        bool m_caseMethodIsFile = false;    // 工况创建方式：false=时序参数 true=时序文件
        QString m_timeSeriesFilePath;       // 时序文件路径（创建方式=时序文件时使用）
        QList<TimeSeriesCaseSpec> m_cases;
        TimeSeriesStepSpec m_stepSpec;
        QList<TimeSeriesLoadItem> m_loadItems;
        QList<TimeSeriesBoundaryItem> m_boundaryItems;
        TimeSeriesOutputSpec m_outputSpec;

        friend class mTimeSeriesCaseStepWidget;
        friend class mTimeSeriesLoadStepWidget;
        friend class mTimeSeriesBoundaryStepWidget;
        friend class mTimeSeriesOutputStepWidget;
    };
}
