#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QWidget>
#include <QComboBox>
#include <QTimer>
#include <QHash>
#include <QVector>
#include <memory>

#include "mLoadPanelBase.h"
#include "mLoadEdge.h"
#include "mPickWidgetController.h"
#include "mDirectionSelectorWidget.h"
#include "mCalcLineEdit.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadEdgePanel : public mLoadPanelBase
    {
        Q_OBJECT
    public:
        mLoadEdgePanel(QWidget* parent = nullptr);
        ~mLoadEdgePanel();

        void initInputs() override;
        void clearInputs() override;
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData) override;
        void deactivatePickAndCommit();
        void updateLoadSymbolRealtime();

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        int getFunc();
        void connectUnitSystemResponse() override;
        void disconnectUnitSystemResponse() override;
        void clearData();
        void applyAllowedPickTypes();

    private slots:
        void slot_unitSystemResponse(UnitType, QVector<int>);
        void slot_distributionChanged(int idx);
        void slot_defineWayChanged(int idx);
        void slot_directionModeChanged(int idx);
        void slot_sizeChanged();
        void slot_funcChanged(int idx);
        void onRealtimeUpdateTimeout();

    private:
        void ensureDirectionWidget();

        // 单项数据保存
        void saveDistribution();
        void saveDefineWay();
        void saveDirectionMode();
        void saveSize();
        void saveFunc();
        void saveScale();
        void notifyEdgeChanged();

        // 方向控件状态写入数据对象
        void applyDirectionWidgetToEdge(std::shared_ptr<MechCaseData::mLoadEdge> edge);

        // 实时符号渲染执行体
        void doUpdateLoadSymbolRealtime();

        // 控件成员
        singleComboBox _comboDistribution;   // 分布类型下拉框
        singleComboBox _comboDefineWay;      // 定义方式下拉框
        singleComboBox _comboDirectionMode;  // 方向模式下拉框
        comboBoxWithPushBtn combofunc;       // 幅值曲线（带创建按钮）
        InputWidgetSet _sizeInput;           // 载荷值输入框
        InputWidgetSet _scale;               // 缩放因子（载荷引用状态）

        QHash<QString, int> _functable;      // 函数名->ID映射
        std::shared_ptr<mLoadEdge> _currentEdge;

        mPickWidgetController* _pickWidgetController = nullptr;  // 通用拾取控件
        mDirectionSelectorWidget* _directionWidget = nullptr;    // 方向选择控件（通用模式）
        QTimer* _realtimeUpdateTimer = nullptr;                  // 渲染防抖定时器

        // 上次实时渲染的最终数据签名（对象ID+位置/方向/分量值），未变化时跳过符号赋值与全量重建
        size_t _lastRenderSig = 0;

        FoldSection* _faval = nullptr;  // 缓存值折叠框指针，用于动态调整布局
    };
}