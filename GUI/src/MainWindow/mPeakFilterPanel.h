#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <memory>
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mPeakFilterData.h"

#include <QVBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include "mCalcLineEdit.h"

using namespace MechGlobalParamData;
using namespace MxFunctions;

namespace MechMainWindow
{
    class mPeakFilterPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mPeakFilterPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mPeakFilterPanel();

        /*
        * 新建
        */
        void createPeakFilter();

        /*
        * 编辑(从树上点击过来的)
        */
        void editPeakFilter(int peakFilterID);

    private slots:
        void slot_tableDataChanged(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        void initTable();
        void applyGlobalToAllFreqCases();
        void removeGlobalFromAllFreqCases();

        //总布局
        QVBoxLayout* _mainLayout;

        /**************************************峰值筛选**************************************************/
        // 响应类型
        QComboBox* _comboRespType;
        // 识别类型
        QComboBox* _comboIdentType;
        // 提取个数
        mCalcLineEdit* _editExtractCount;
        // 最小间距
        mCalcLineEdit* _editMinSpacing;
        // 最大间距
        mCalcLineEdit* _editMaxSpacing;
        // 起始频率
        mCalcLineEdit* _editStartFreq;
        // 终止频率
        mCalcLineEdit* _editEndFreq;
        // 应用到全局
        QCheckBox* _checkApplyGlobal;

        mPeakFilterData* data;
    };
}
