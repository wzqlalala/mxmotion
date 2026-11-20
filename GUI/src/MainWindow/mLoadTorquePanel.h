#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QListView>
#include <QCheckBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QTableWidget>
#include <memory>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"

#include "mLoadForce.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"
#include "mDockLayoutManager.h"
#include "mOtherFunction.h"

#include "mPickWidgetController.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    // 力矩面板（完全参照集中力面板结构）
    class MMAINWINDOW_EXPORT mLoadTorquePanel : public mLoadPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        /*
        * 构造函数
        */
        mLoadTorquePanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadTorquePanel();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值
        */
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)override;

        /*
        * 连接单位制响应信号
        */
        void connectUnitSystemResponse()override;

        /*
        * 断开单位制响应信号
        */
        void disconnectUnitSystemResponse()override;

        /*
        * 失活并提交拾取数据：供统一退出出口调用
        */
        void deactivatePickAndCommit();

        /*
        * 实时渲染力矩符号
        */
        void updateLoadSymbolRealtime();

        /*
        * 按当前定义方式设置拾取类型白名单
        */
        void applyAllowedPickTypes(int way);

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;

        /*
        * 作用对象输入控件
        */
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;

        /*
        * 值输入控件
        */
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;


        /*
        * 获取函数
        */
        int getFunc();

        /*
        * 获取坐标系
        */
        int getCoor();

    private slots:
        /*
        * 定义方式改变
        */
        void slot_defineTypeChanged(int idx);

        /*
        * 函数
        */
        void slot_funcChanged(int idx);

        /*
        * 坐标系
        */
        void slot_coorChanged(int idx);

        /*
        * 离散数据控件（独立赋值表格）数据变化
        */
        void slot_discreteDataChanged();

        /*
        * 离散数据控件单元格编辑
        */
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);

        /*
        * 浏览文件
        */
        void slot_browseFileClicked();

        /*
        * 单位制切换响应
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

    private:
        void updateOpenedTable();
        void filterPicksToNodeOnly();

        // 单独保存各数据项
        void saveTorqueComponents();        // 各向分量 X/Y/Z
        void saveWayAndClearModeFields();   // 保存定义方式并清空模式字段
        void saveFilePath();                // 文件路径
        void saveScale();                   // 缩放因子
        void saveFunc();                    // 幅值曲线映射
        void saveCoor();                    // 坐标系映射
        void notifyLoadChanged();           // 仅通知外部刷新树/渲染

        QHash<QString, int>_functable;
        QVector<InputWidgetSet> inputWidgets;
        QVector<singleComboBox> comboBoxs;
        comboBoxWithPushBtn combofunc;
        comboBoxWithPushBtn combocoor;

        // 定义方式
        comboBoxWithPushBtn _defineTypeCombo;

        // 独立赋值控件
        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;

        // 通用离散数据控件（独立赋值模式的数据表格入口）
        mDiscreteDataControl* _discreteControl = nullptr;

        InputWidgetSet _scale;

        // 文件选择控件
        QWidget* _fileRowWidget = nullptr;
        QLabel* _fileLabel = nullptr;
        QLineEdit* _filePathEdit = nullptr;
        QPushButton* _fileBrowseBtn = nullptr;

        std::shared_ptr<mLoadForce> _currentLoad;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;
    };

}