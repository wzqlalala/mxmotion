#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <memory>
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mNodeContriData.h"

#include<QComboBox>
#include<QTableWidget>
#include<QPushButton>
#include<QStringList>
#include<QListWidget>

using namespace MechGlobalParamData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mNodeContriPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mNodeContriPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mNodeContriPanel();


        /*
        * 新建
        */
        void createNodeContri(int id);

        /*
        * 编辑(从树上点击过来的)
        */
        void editNodeContri(int contriID = 0);

    private:
        //总布局
        QVBoxLayout* _mainLayout;
        /**************************************节点贡献量**************************************************/
        // 响应点集合
        QComboBox* _combNodeRespSet;
        // 结构节点输出集合
        QComboBox* _combStructNodeSet;
        // 声腔节点输出集合
        QComboBox* _combCavityNodeSet;
        // 输出频率点集合
        QComboBox* _combNodeFreqSet;
        // 响应截断值类型（帕斯卡/分贝计权）
        QComboBox* _truncTypeCombo;
        // 响应截断值输入框
        std::shared_ptr<mCalcLineEdit> _truncValueLineEdit;
        // 声腔网格投影值输出
        QCheckBox* _checkBoxCavityProj;
        // 峰值筛选
        QCheckBox* _checkBoxNodePkScr;

        mNodeContriData* data;
    };
}