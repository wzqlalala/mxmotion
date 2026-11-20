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
#include <QCheckBox>
#include <QLineEdit>

namespace MechGlobalParamData
{
    class mGenericAssignData;
}

namespace MechMainWindow
{
    //超单元调用面板
    class MMAINWINDOW_EXPORT mGenericAssignPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mGenericAssignPanel(QWidget* parent = Q_NULLPTR);
        ~mGenericAssignPanel();

        /*
        * 创建超单元调用
        */
        void createGenericAssign();

        /*
        * 编辑超单元调用(从树上点击过来的)
        */
        void editGenericAssignPanel();

    private slots:
        /*
        * 确定
        */
        void slot_ok();

        /*
        * 矩阵复选框状态改变
        */
        void slot_matrixCheckBoxToggled(bool checked);

    private:
        /*
        * 添加超单元导入行
        */
        void addImportRow(int index, QString path = "");

        /*
        * 仅调整导入行数量
        */
        void adjustImportRows(int number);

        /*
        * 同步数据到UI
        */
        void syncDataToUI();

        /*
        * 同步UI到数据
        */
        void syncUIToData();

        /*
        * 验证矩阵名字格式和数量
        */
        bool validateMatrixNames(const QString& text, int expectedCount, QString& errorMsg);
    private:
        // 数据同步标志位：true表示正在从数据同步到UI，此时忽略所有UI信号
        bool _bSyncingData = false;

        // 显示错误提示（强制在输入框右上角显示）
        void showErrorTip(QLineEdit* lineEdit, const QString& errorMsg);

        // 清除错误提示
        void clearErrorTip(QLineEdit* lineEdit);

    private:
        QVBoxLayout* _mainLayout = nullptr;

        mCalcLineEdit* _assignNum;

        MechGlobalParamData::mGenericAssignData* _genericAssign = nullptr;

        // 矩阵UI控件（固定4个）
        QCheckBox* _stiffnessCheck;
        QLineEdit* _stiffnessLine;

        QCheckBox* _massCheck;
        QLineEdit* _massLine;

        QCheckBox* _viscousDampingCheck;
        QLineEdit* _viscousDampingLine;

        QCheckBox* _materialCheck;
        QLineEdit* _materialLine;
    };
}