//解决中文乱码
#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mCalcLineEdit.h"

//QT
#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>

namespace MechGlobalParamData
{
    class mGenericMatrixData;
}

namespace MechMainWindow
{
    //超单元矩阵面板
    class MMAINWINDOW_EXPORT mGenericMatrixPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mGenericMatrixPanel(QWidget* parent = Q_NULLPTR);
        ~mGenericMatrixPanel();

        /*
        * 创建超单元参数
        */
        void createGenericMatrix();

        /*
        * 编辑超单元参数(从树上点击过来的)
        */
        void editGenericMatrixPanel();

    private:
        QVBoxLayout* _mainLayout = nullptr;

        QLineEdit* _matrixName;
    };
}
