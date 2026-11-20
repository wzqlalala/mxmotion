#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mTimeSeriesHeatWizardTypes.h"

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
    * 向导第2步：载荷设置
    *
    * 三段式：上半 工况载荷配置表（表头随载荷类型动态变化，未导入仅表头）；
    * 下半左 载荷列表（表格样式：名称/类型/关联工况）；下半右 载荷信息编辑区。
    * 集合下拉显示当前模型中可选集合（按载荷类型过滤 NodeSet/ElementSet/ElementSurface）。
    * 布局对齐原型 .load-page。
    */
    class MMAINWINDOW_EXPORT mTimeSeriesLoadStepWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit mTimeSeriesLoadStepWidget(mCaseTimeSeriesHeatWizard* wizard);

        /*
        * 校验当前页数据
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
        * 页面数据被修改（新建/保存/删除载荷），供向导重置后续步骤完成状态
        */
        void dataModified();

    private slots:
        void slot_newLoad();
        void slot_editLoad();
        void slot_deleteLoad();
        void slot_saveLoad();
        void slot_selectLoad(int row, int column);
        void slot_browseFile();
        void slot_importFile();
        void slot_typeChanged(int index);
        void slot_setChanged(int index);

    private:
        void buildUi();
        void refreshLoadList();
        void refreshConfigTable();
        void refreshSetCombo();
        void enterEditMode(int index);      // -1 表示新建
        TimeSeriesLoadKind currentKind() const;

        mCaseTimeSeriesHeatWizard* m_wizard = nullptr;

        // 上半：工况载荷配置表
        QTableWidget* m_configTable = nullptr;

        // 下半左：载荷列表（名称/类型/关联工况）
        QTableWidget* m_loadList = nullptr;
        QPushButton* m_newButton = nullptr;
        QPushButton* m_editButton = nullptr;
        QPushButton* m_deleteButton = nullptr;

        // 下半右：载荷信息
        QLineEdit* m_nameEdit = nullptr;
        QComboBox* m_typeCombo = nullptr;
        QComboBox* m_setCombo = nullptr;
        QLineEdit* m_filePathEdit = nullptr;
        QPushButton* m_importButton = nullptr;
        QPushButton* m_saveButton = nullptr;

        int m_editingIndex = -1;    // 正在编辑的载荷索引，-1=新建
        int m_selectedIndex = -1;   // 列表中选中项
    };
}
