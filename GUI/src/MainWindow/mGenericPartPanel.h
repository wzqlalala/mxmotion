//解决中文乱码
#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mCalcLineEdit.h"

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QToolButton>

namespace MechGlobalParamData 
{
    class mGenericPartData;
}


namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mGenericPartPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mGenericPartPanel(QWidget* parent = Q_NULLPTR);
        ~mGenericPartPanel();


        /*
        * 超单元部件创建
        */
        void createGenericPart();

        /*
        *  编辑超单元部件
        */
        void editGenericPartPanel();

    private slots:
        //添加槽函数
        void slot_allPartBtn();

        //添加超单元参数槽函数
        void slot_addPartBtn();

        //减少超单元参数槽函数
        void slot_reducePartBtn();

        //清空超单元参数槽函数
        void slot_clearPartBtn();


    private:
        QVBoxLayout* _mainLayout = nullptr;

        MechGlobalParamData::mGenericPartData* _genericPartData;

        QListWidget* _listGenericPart;

        QStringListModel* _tempModel;


    };
}
