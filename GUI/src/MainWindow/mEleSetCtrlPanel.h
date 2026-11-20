#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mPhysicalFieldEnum.h"

#include "mPanelBase.h"

//BasicData
#include "mEleSetCtrl.h"

//通用拾取控件
#include "mPickWidgetController.h"

#include <QVBoxLayout>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>

#include <memory>

using namespace MechModelFixData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mEleSetCtrlPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mEleSetCtrlPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mEleSetCtrlPanel();

        /*
        * 新建单元生死设置面板
        */
        int newEleSetCtrlPanel();

        /*
        * 编辑已有单元生死设置
        */
        void editEleSetCtrlPanel(int modelFixId = -1);

        /*
        * 退出编辑
        */
        void uneditEleSetCtrlPanel();

    private:
        /*
        * 初始化界面控件
        */
        void initUI();

        /*
        * 配置通用拾取控件：持久化策略/集合解析器/单元集合列表
        */
        void setupPickController();

        /*
        * 从模型刷新界面
        */
        void updateFromData();

        /*
        * 失活拾取控件并提交数据（切出面板时调用）
        */
        void deactivatePickAndCommit();

    private slots:
        /*
        * 激活/移除切换
        */
        void slot_activeOrRemoveChanged(int id);

    protected:
        //数据
        std::shared_ptr<MechModelFixData::mEleSetCtrl> _eleSetCtrl;

        //界面
        QVBoxLayout* _mainLayout;

        // 模型修改类型
        QComboBox* _comboModelFixType;

        // 激活/移除
        QButtonGroup* _btnGroupActiveOrRemove;
        QRadioButton* _radioActive;
        QRadioButton* _radioRemove;

        // 作用对象：通用拾取控件（拾取单元 / 选择单元集合）
        mPickWidgetController* _pickWidgetController = nullptr;
    };
}
