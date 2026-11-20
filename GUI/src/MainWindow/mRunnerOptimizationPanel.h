//解决中文乱码
#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mBasicEnum.h"
#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mRunnerStructOption.h"
#include "mMapRelationEnum.h"

#include <memory>
#include <functional>

//QT
#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QComboBox>

namespace MechMainWindow
{
    //流道优化面板
    class MMAINWINDOW_EXPORT mRunnerOptimizationPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        // 默认构造函数
        mRunnerOptimizationPanel(QWidget* parent = Q_NULLPTR);

        // 析构函数
        ~mRunnerOptimizationPanel();

        //新建流道优化
        void newRunnerOptimization();

        //编辑
        void editRunnerOptimization();

    private:
        // 一个参数项的UI-数据绑定描述
        struct ParamBinding
        {
            QCheckBox*     checkBox = nullptr;   // 状态复选框，nullptr 表示无状态
            mCalcLineEdit* lineEdit = nullptr;   // 输入框
            bool*          status   = nullptr;   // 数据中的状态指针
            QString*       value    = nullptr;   // 数据中的值指针
        };

        // 创建带状态（复选框+输入框）的参数行
        ParamBinding addCheckParam(FoldSection* fold, const QString& label);

        // 创建纯输入（仅输入框）的参数行
        ParamBinding addPlainParam(FoldSection* fold, const QString& label);

        // 创建集合选择下拉框行
        void addSetComboRow(FoldSection* fold, const QString& label, QComboBox*& combo, MxFunctions::MapRelationType type);

        // 更新下拉框可选集合列表
        void updateComboSetList(QComboBox* combo);

        // 将UI数据同步到内存
        void syncDataToUI();

    private:
        MechStructOptionData::mRunnerStructOption* _runnerOptionBase = nullptr;

        QVBoxLayout* _mainLayout = nullptr;
        QVector<ParamBinding> _paramBindings;

        // 流体边界 - 集合选择
        QComboBox* _comboInlet  = nullptr;   // 入口
        QComboBox* _comboOutlet = nullptr;   // 出口
        QComboBox* _comboWall   = nullptr;   // 壁面
    };
}
