#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <memory>
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mERPData.h"

#include <QVBoxLayout>
#include "mCalcLineEdit.h"

using namespace MechGlobalParamData;
using namespace MxFunctions;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mERPPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mERPPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mERPPanel();

        /*
        * 新建
        */
        void createERP(int id);

        /*
        * 编辑(从树上点击过来的)
        */
        void editERP(int erpID = 0);

    private:
        //总布局
        QVBoxLayout* _mainLayout;

        /**************************************等效辐射功率**************************************************/
        // 介质密度
        mCalcLineEdit* _editRho;
        // 声速
        mCalcLineEdit* _editC;
        // 缩放系数
        mCalcLineEdit* _editRhocp;
        // 辐射损耗密度
        mCalcLineEdit* _editRlf;
        // 参考值
        mCalcLineEdit* _editRefDb;

        mERPData* data;
    };
}
