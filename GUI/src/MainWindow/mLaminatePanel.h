#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <memory>
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"

#include<QComboBox>
#include<QTableWidget>
#include<QPushButton>
#include<QStringList>
#include<QListWidget>

#include "mLaminateBase.h"

namespace MechMainWindow
{
    //截面属性面板
    class MMAINWINDOW_EXPORT mLaminatePanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mLaminatePanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mLaminatePanel();

        template<typename T>
        T* toType(MechPropertyData::mLaminateBase* base)
        {
            return dynamic_cast<T*>(base);
        };

        /*
        * 新建
        */
        void newLaminatePanel(LaminateType type);

        /*
        * 编辑(从树上点击过来的)
        */
        void editLaminatePanel(int laminateID = 0);

    public slots:
        /*
        *单层区域（作用对象）下拉框切换
        */
        void slot_targetObjectChanged();

        //跳转到新建截面形状槽函数(并且获取最新创建的材料然后存到属性数据中)
        void slot_newMaterial();

        //材料下拉框变化
        void slot_matComboBoxChanged(const QString& text);

        //离散转角下拉框变化
        void slot_disRotComboBoxChanged(int index);

        void onLaminateTypeChanged(int index);

        // 表格槽函数
        void slot_addPartBtn();
        void slot_reducePartBtn();
        void slot_clearPartBtn();
    protected:
        /*
        * 更新界面
        */
        void updatePanel();

        /*
        * 更新参数页面
        */
        void updateParamPanel();

        /*
        * 更新材料
        */
        void updateMatCombobox(QComboBox* combobox);

        /*
        * 更新材料页面
        */
        void updateMatPanel();

        void loadListFromMapRelation(); // 从映射加载数据
    private:
        // 各类型参数面板创建函数（核心模块化）
        void createSglLayerPanel(FoldSection* fold);
        void createGlobalSglLamPanel(FoldSection* fold);
        void createLamPlatePanel(FoldSection* fold);

    private:
        //总布局
        QVBoxLayout* _mainLayout;

        //材料下拉框
        QComboBox* _comboMat;

        //单层板控件
        QComboBox* _regionCombo = nullptr; //区域下拉框
        QComboBox* _disRotCombo = nullptr; //离散转角下拉框

        QComboBox* _lamTypeCombo = nullptr;
        QStringList m_lamComboItems;

        bool m_isLoadingData = false; //加这个，仅加载数据时用

        //数据
        MechPropertyData::mLaminateBase* _LaminateData = nullptr;

        //层合板列表
        QListWidget* _listAssignPart;

    };
}