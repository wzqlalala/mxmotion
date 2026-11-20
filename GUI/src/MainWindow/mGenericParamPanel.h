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
    class mGenericParamData;
}

namespace MechMainWindow
{
    //超单元参数面板
    class MMAINWINDOW_EXPORT mGenericParamPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mGenericParamPanel(QWidget* parent = Q_NULLPTR);
        ~mGenericParamPanel();

        /*
        * 创建超单元参数
        */
        void createGenericParam();

        /*
        * 编辑超单元参数(从树上点击过来的)
        */
        void editGenericParamPanel(int id);

    private:
        QVBoxLayout* _mainLayout = nullptr;

        MechGlobalParamData::mGenericParamData* _genericParam = nullptr;

        // 缩聚方法
        QComboBox* _comboMethod = nullptr;
        // 频率上限
        mCalcLineEdit* _editFreqLimit = nullptr;
        // 模态阶数
        mCalcLineEdit* _editModalOrder = nullptr;
        // 坐标点起始编号
        mCalcLineEdit* _editPointStart = nullptr;
    };
}
