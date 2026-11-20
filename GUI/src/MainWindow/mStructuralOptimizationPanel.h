 //解决中文乱码
#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include "mStructOption.h"
#include "mPanelBase.h"
#include "mCalcLineEdit.h"

namespace MechMainWindow
{
    //结构优化
    class MMAINWINDOW_EXPORT mStructuralOptimizationPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        
        // 默认构造函数
        mStructuralOptimizationPanel(QWidget* parent = Q_NULLPTR);

        
       // 析构函数
        ~mStructuralOptimizationPanel();

        
        //新建结构优化
        void newStructuralOptimization();

        //编辑
        void editStructuralOptimization();

    private slots:
        /*
        * 复选框改变
        */
        void slot_checkChanged(bool isChecked);

        //
        void slot_textChanged();

    protected:
        MechStructOptionData::mStructOption* _structOptionBase = nullptr;

        //总布局
        QVBoxLayout* _mainLayout;

        std::vector<QCheckBox*> _cBoxs;
        std::vector<mCalcLineEdit*> _cLineEdits;

        //优化算法
        QCheckBox* _matinterCb;
        QComboBox* _matinterCombox;

        //模型重构
        QCheckBox* _rebuildCb;
        mCalcLineEdit* _rebuildCLine;

        //重启动
        QCheckBox* _resCb;
        mCalcLineEdit* _resCLine;
        QLabel* _resLabel;
        QLineEdit* _resLine;
        QToolButton* _resTButton;
    };






}

