//解决中文乱码
#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"

//QT
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

namespace MechGlobalParamData
{
    class mSolveControlData;
}

namespace MechMainWindow
{
    //求解控制面板
    class MMAINWINDOW_EXPORT mSolveControlPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mSolveControlPanel(QWidget* parent = Q_NULLPTR);
        ~mSolveControlPanel();

        /*
        * 创建求解控制
        */
        void createSolveControl();

        /*
        * 编辑求解控制(从树上点击过来的)
        */
        void editSolveControlPanel();

    private:
        QVBoxLayout* _mainLayout = nullptr;

        MechGlobalParamData::mSolveControlData* _solveControlData = nullptr;

        // CPU并行核数输入框
        QLineEdit* _cpuCoreCountEdit = nullptr;
    };
}
