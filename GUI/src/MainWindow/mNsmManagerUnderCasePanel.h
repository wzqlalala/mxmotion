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

#include "mNsmUnderCaseData.h"

namespace MechMainWindow
{
    //挂载在分析工况下的 非结构质量节点面板
    class MMAINWINDOW_EXPORT mNsmManagerUnderCasePanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mNsmManagerUnderCasePanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mNsmManagerUnderCasePanel();


        /*
        * 编辑工况下非结构体质量节点（从树上点击过来的）
        */
        void editNsmManagerUnderCasePanel(int parentCaseId);

    private slots:
        void slot_allNsmBtn();
        void slot_addNsmBtn();
        void slot_reduceBtn();
        void slot_cleanBtn();


    private:

        MechCaseData::mNsmUnderCaseData* _nsmManagerData;


        int _parentCaseId;

        QList<int> _nsmIdList;

        QStringListModel* _nsmModel;

        QVBoxLayout* _mainLayout;

        QLabel* _signLabel;
        QListWidget* _listNsm;
        QToolButton* _allBtn;
        QToolButton* _addBtn;
        QToolButton* _reduceBtn;
        QToolButton* _cleanBtn;


    };

}