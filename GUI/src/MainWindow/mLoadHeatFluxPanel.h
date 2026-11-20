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
#include <QHeaderView>
#include <QTableWidget>
#include <QTimer>
#include <memory>
#include <unordered_map>
#include <map>
#include <QVariant>
#include <algorithm>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"
#include "mLoadHeatFlux.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"
#include "mPickWidgetController.h"
#include "mOtherFunction.h"
#include "mFieldManager.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadHeatFluxPanel : public mLoadPanelBase
    {
        Q_OBJECT

    public:
        mLoadHeatFluxPanel(QWidget* parent = nullptr);
        ~mLoadHeatFluxPanel();

        void initInputs()override;
        void clearInputs()override;
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData) override;

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        int getFunc();

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData() override;

        // 失活并提交拾取数据：供统一退出出口调用
        void deactivatePickAndCommit();

        // 实时渲染热流密度符号
        void updateLoadSymbolRealtime();
        // 防抖定时器触发：合并短时间内的多次实时渲染请求
        void onRealtimeUpdateTimeout();

    private slots:
        void slot_funcChanged(int idx);
        void slot_valueInpWayChanged(int idx);
        void slot_defineTypeChanged(int idx);
        void slot_analysisFieldChanged(int idx);
        void slot_discreteDataChanged();
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);

    signals:
        // 定义方式切换导致作用对象折叠框显隐变化时通知外层重建面板
        void sg_rebuildPanel();

    private:
        // 根据当前热流类型过滤/保留拾取数据
        void filterPicksByHeatFluxType();
        void updateOpenedTable();
        void applyAllowedPickTypes(int way);
        void rebuildValueLayout();
        // 按当前热流类型重建独立赋值离散数据控件（拾取对象/标识列随点/面/体热流切换）
        void rebuildDiscreteControl();

        // 局部保存（不再 emit sg_editFinished）
        void saveHeatFluxValue();   // 统一赋值：热流密度大小
        void saveFunc();            // 幅值曲线映射
        void saveScale();           // 缩放因子
        void saveAnalysisField();   // 分析场映射
        void notifyLoadChanged();   // 仅通知外部刷新树/渲染

        // 实时渲染执行体：由 updateLoadSymbolRealtime 启动/重启的防抖定时器统一调用。
        void doUpdateLoadSymbolRealtime();

        // 从内部 _picks 生成供通用拾取控件显示的完整数据（含集合）
        void syncPicksToController();

        QHash<QString, int>_functable;
        QVector<singleComboBox> comboBoxs;    // 0=热流类型, 1=定义方式
        comboBoxWithPushBtn combofunc;
        InputWidgetSet _scale;
        InputWidgetSet _heatFluxInput;        // 统一赋值：热流密度大小

        // 定义方式下拉框
        singleComboBox _defineTypeCombo;

        // 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        singleComboBox _analysisFieldCombo;
        QHash<QString, int> _analysisFieldTable;

        // 表格行容器
        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;
        mDiscreteDataControl* _discreteControl = nullptr;

        // 通用拾取控件（替代原树形选择+按钮）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 当前载荷数据
        std::shared_ptr<MechCaseData::mLoadHeatFlux> _currentLoad;

        // 当前热流类型：0=点热流, 1=面热流, 2=体热流
        int index = 0;

        // 实时符号渲染防抖定时器：合并方向/大小/拾取连续变化触发的多次刷新。
        QTimer* _realtimeUpdateTimer = nullptr;

        // 上次实时渲染的最终数据签名（对象ID+位置/方向/分量值），未变化时跳过符号赋值与全量重建
        size_t _lastRenderSig = 0;
    };
}