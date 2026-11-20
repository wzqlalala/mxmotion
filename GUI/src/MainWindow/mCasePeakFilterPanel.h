#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QToolButton>

namespace MechMainWindow
{
    class mCasePeakFilterPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        mCasePeakFilterPanel(QWidget* parent = Q_NULLPTR);
        ~mCasePeakFilterPanel();

        /*
        * 创建/编辑工况下峰值筛选
        */
        void createCasePeakFilter(int caseId);
        void editCasePeakFilter(int caseId);

        /*
        * 刷新列表（供外部调用，如模型树勾选"应用到全局"后）
        */
        void refreshList();

    private slots:
        // 列表按钮槽
        void slot_addPeakFilterBtn();
        void slot_reducePeakFilterBtn();
        void slot_clearPeakFilterBtn();

        // 创建新的全局峰值筛选并关联到当前工况
        void slot_createPeakFilterClicked();

    private:
        void loadListFromMapRelation();

        //总布局
        QVBoxLayout* _mainLayout;

        // 工况ID
        int _caseId;

        // 列表 + 按钮（参考层合板）
        QListWidget* _listPeakFilter;
        QToolButton* _btnCreatePeakFilter;
    };
}
