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
    * 向导第3步：边界设置
    *
    * 三段式（与载荷设置同构）：工况边界配置表 + 边界条件列表 + 边界条件信息。
    * 当前仅支持“温度边界”，直接指定温度值（不引用文件、不关联分析场）；
    * 落库时每个边界只创建一个本体并映射到全部时序工况。
    */
    class MMAINWINDOW_EXPORT mTimeSeriesBoundaryStepWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit mTimeSeriesBoundaryStepWidget(mCaseTimeSeriesHeatWizard* wizard);

        bool validatePage(QString* error);

        /*
        * 恢复控件以显示当前向导数据（关闭再打开时调用）
        */
        void reloadFromData();

    public slots:
        /*
        * 清空本页内容（由向导底部“清除”按钮触发）
        */
        void clearPage();

    signals:
        /*
        * 页面数据被修改（新建/保存/删除边界），供向导重置后续步骤完成状态
        */
        void dataModified();

    private slots:
        void slot_newBoundary();
        void slot_editBoundary();
        void slot_deleteBoundary();
        void slot_saveBoundary();
        void slot_selectBoundary(int row, int column);
        void slot_setChanged(int index);

    private:
        void buildUi();
        void refreshBoundaryList();
        void refreshConfigTable();
        void refreshSetCombo();
        void enterEditMode(int index);

        mCaseTimeSeriesHeatWizard* m_wizard = nullptr;

        QTableWidget* m_configTable = nullptr;

        QTableWidget* m_boundaryList = nullptr;
        QPushButton* m_newButton = nullptr;
        QPushButton* m_editButton = nullptr;
        QPushButton* m_deleteButton = nullptr;

        QLineEdit* m_nameEdit = nullptr;
        QComboBox* m_typeCombo = nullptr;   // 固定“温度边界”
        QComboBox* m_setCombo = nullptr;
        MxFunctions::mCalcLineEdit* m_tempEdit = nullptr;   // 温度输入（℃），支持科学计数法/表达式
        QPushButton* m_saveButton = nullptr;

        int m_editingIndex = -1;
        int m_selectedIndex = -1;
    };
}
