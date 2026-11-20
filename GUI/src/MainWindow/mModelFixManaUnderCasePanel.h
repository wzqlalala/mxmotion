#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QListWidgetItem>

#include <QStringListModel>

#include "mPanelBase.h"

#include "mPhysicalFieldEnum.h"

namespace MechMainWindow
{
    //挂载在分析工况下的 模型修改节点面板
    class MMAINWINDOW_EXPORT mModelFixManaUnderCasePanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mModelFixManaUnderCasePanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mModelFixManaUnderCasePanel();


        /*
        * 编辑工况下模型修改节点（从树上点击过来的）
        */
        void editModelFixManagerUnderCasePanel(int parentCaseId);

    private slots:
        void slot_allModelFixBtn();
        void slot_addModelFixBtn();
        void slot_reduceBtn();
        void slot_cleanBtn();


    private:

        int _parentCaseId;

        QList<int> _modelFixIdList;

        QStringListModel* _modelFixModel;

        QVBoxLayout* _mainLayout;

        QLabel* _signLabel;
        QListWidget* _listModelFix;
        QToolButton* _allBtn;
        QToolButton* _addBtn;
        QToolButton* _reduceBtn;
        QToolButton* _cleanBtn;


    };

}
