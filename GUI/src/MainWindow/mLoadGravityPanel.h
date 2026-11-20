#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QLabel>
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
#include "mPickWidgetController.h"
#include "mDirectionSelectorWidget.h"

#include "mLoadGravity.h"


using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadGravityPanel : public mLoadPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoadGravityPanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadGravityPanel();

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

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;

        /*
        * 值输入控件
        */
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        /*
        * 作用对象
        */
        QVector<QWidget*> getTargetInputs(FoldSection* fatherval) override;

        /*
        * 连接单位制响应信号
        */
        void connectUnitSystemResponse()override;

        /*
        * 断开单位制响应信号
        */
        void disconnectUnitSystemResponse()override;

        /*
        * 获取当前拾取数据
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        /*
        * 失活并提交拾取数据：供统一退出出口调用
        */
        void deactivatePickAndCommit();

        /*
        * 根据局部施加折叠框勾选状态启用/禁用拾取控件
        */
        void setPickEnabled(bool enabled);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private slots:
        /*
        * 单位制切换响应
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

        /*
        * 定义方式切换响应
        */
        void slot_defineTypeChanged(int);

    private:
        // 延迟创建方向控件
        void ensureDirectionWidget();

        // 设置可拾取类型
        void applyAllowedPickTypes();

        // 各向分量局部保存
        void saveGravityComponents();
        // 方向与大小保存
        void saveDirectionAndMagnitude();
        // 将方向控件状态写入载荷
        void applyDirectionWidgetToLoad(std::shared_ptr<MechCaseData::mLoadGravity> load);
        // 保存定义方式并清空模式字段
        void saveWayAndClearModeFields();
        // 缩放因子局部保存
        void saveScale();
        // 仅通知外部刷新树/渲染
        void notifyLoadChanged();

        QStringList _units;
        QVector<InputWidgetSet> inputWidgets;
        QVector<singleComboBox> comboBoxs;
        InputWidgetSet _scale;

        // 方向与大小控件
        mDirectionSelectorWidget* _directionWidget = nullptr;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 当前拾取控件是否处于可交互状态（由局部施加折叠框控制）
        bool _pickWidgetEnabled = false;

        std::shared_ptr<mLoadGravity> _currentLoad;
    };
}