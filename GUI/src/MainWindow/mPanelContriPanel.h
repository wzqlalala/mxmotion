#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <memory>
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mPanelContriData.h"

#include<QComboBox>
#include<QTableWidget>
#include<QPushButton>
#include<QStringList>
#include<QListWidget>

using namespace MechGlobalParamData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mPanelContriPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mPanelContriPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mPanelContriPanel();


        /*
        * 新建
        */
        void createPanelContri(int id);

        /*
        * 编辑(从树上点击过来的)
        */
        void editPanelContri(int contriID = 0);

    private:
        //总布局
        QVBoxLayout* _mainLayout;
        /**************************************面板贡献量**************************************************/
        // 输出频率点集合
        QComboBox* _combPnlFreqSet;
        // 输出类型
        QComboBox* _combPnlOutType;
        // 响应点集合
        QComboBox* _combPnlRespSet;
        // 峰值筛选
        QCheckBox* _checkBoxPnlPkScr;

        mPanelContriData* data;
    };
}
