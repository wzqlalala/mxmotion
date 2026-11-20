#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <memory>
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mTPContriData.h"

#include<QComboBox>
#include<QTableWidget>
#include<QPushButton>
#include<QStringList>
#include<QListWidget>

using namespace MechGlobalParamData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mTPContriPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mTPContriPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mTPContriPanel();


        /*
        * 新建
        */
        void createTPContri(int id);

        /*
        * 编辑(从树上点击过来的)
        */
        void editTPContri(int contriID = 0);

    private:
        //总布局
        QVBoxLayout* _mainLayout;
        /**************************************传递路径贡献量**************************************************/
        // 响应类型
        QComboBox* _combTPRespType;
        // 连接点集合
        QComboBox* _combTPCnnPtSet;
        // 响应点集合
        QComboBox* _combTPRespPtSet;
        // 连接单元集合
        QComboBox* _combTPCnnElemSet;
        // 硬链接单元集合
        QComboBox* _combTPHardLinkElemSet;
        // 被动侧节点集合
        QComboBox* _combTPPassiveNodeSet;

        mTPContriData* data;
    };
}
