//解决中文乱码
#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mCaseSettingPanel.h"

//QT
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>

namespace MechMainWindow
{
    //全局输出控制控制面板
    class MMAINWINDOW_EXPORT mGlobalOutputControlPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mGlobalOutputControlPanel(QWidget* parent = Q_NULLPTR);
        ~mGlobalOutputControlPanel();

        /*
        * 创建全局输出控制（从工具栏/菜单点击过来）
        */
        void createGlobalOutputControlPanel();

        /*
        * 编辑全局输出控制面板（从树上点击过来）
        */
        void editGlobalOutputControlPanel();

    private:
        void initUI();
        QHash<QString, QStringList> buildAllOutputVariables() const;

        QVBoxLayout* _mainLayout = nullptr;
        FoldSection* _outputFoldWidget = nullptr;
        OutputControlWidget* _outputControlWidget = nullptr;
    };
}
