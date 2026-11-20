#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <memory>
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mModalContriData.h"
#include "mMaterialPropBase.h"

#include<QComboBox>
#include<QTableWidget>
#include<QPushButton>
#include<QStringList>
#include<QListWidget>

using namespace MechGlobalParamData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mModalContriPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mModalContriPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mModalContriPanel();


        /*
        * 新建
        */
        void createModalContri(int id);

        /*
        * 编辑(从树上点击过来的)
        */
        void editModalContri(int contriID = 0);

    private:
        //总布局
        QVBoxLayout* _mainLayout;
        /**************************************模态贡献量**************************************************/
        //输出类型下拉框 structure，fluid
        QComboBox* _combOutPutType;
        //排序方式下拉框 mag，percentage
        QComboBox* _combSortType;
        //输出集合
        QComboBox* _combOutPutSet;
        //输出频率点集合
        QComboBox* _combOutPutFreqSet;
        //贡献量结果类型
        QComboBox* _combResultType;
        //过滤分数（带勾选框）
        InputWithCheckBox _fltScoreInput;
        //输出阶数（带勾选框）
        InputWithCheckBox _OutOrdInput;
        //峰值筛选
        QCheckBox* _checkBoxPkScr;

        mModalContriData* data;
    };
}