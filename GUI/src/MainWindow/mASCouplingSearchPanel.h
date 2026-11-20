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
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>

namespace MechGlobalParamData
{
    class mGenericAssignData;
    class mASCouplingSearchData;
}

namespace MechMainWindow
{
    //声固耦合对搜索面板
    class MMAINWINDOW_EXPORT mASCouplingSearchPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mASCouplingSearchPanel(QWidget* parent = Q_NULLPTR);
        ~mASCouplingSearchPanel();

        /*
        * 创建声固耦合对搜索
        */
        void createASCouplingSearch();

        /*
        * 编辑声固耦合对搜索(从树上点击过来的)
        */
        void editASCouplingSearchPanel();

    private:
        QVBoxLayout* _mainLayout = nullptr;

        // 声固耦合搜索参数
        QComboBox* _autoSearchCombo = nullptr;  // 自动搜索开关
        QComboBox* _mainSurfaceCombo = nullptr; // 主面

        QCheckBox* _searchAngelCheck = nullptr;   // 耦合面搜索夹角复选框
        QLineEdit* _searchAngleLineEidt = nullptr;  //值输入框

        QCheckBox* _outerNormalCheck = nullptr;
        QLineEdit* _outerNormalLineEdit = nullptr;   // 面单元外法线方向延伸比例值

        QCheckBox* _innerNormalCheck = nullptr;
        QLineEdit* _innerNormalLineEdit = nullptr;   // 面单元内法线方向延伸比例值

        QCheckBox* _transverseCheck = nullptr;
        QLineEdit* _transverseLineEdit = nullptr;    // 面单元横向延伸比例值

        QCheckBox* _boundingBoxCheck = nullptr;
        QLineEdit* _boundingBoxLineEdit = nullptr;   // 包围盒延伸比例值

        MechGlobalParamData::mASCouplingSearchData* _couplingSearchData = nullptr;
    };
}
