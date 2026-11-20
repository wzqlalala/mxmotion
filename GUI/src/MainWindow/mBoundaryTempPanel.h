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
#include <QTimer>
#include <memory>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"
#include "mConnector.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mBoundaryPanelBase.h"
#include "mPickWidgetController.h"
#include "mBoundaryTemp.h"
#include "mFieldManager.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mBoundaryTempPanel : public mBoundaryPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        mBoundaryTempPanel(QWidget* parent = nullptr);
        ~mBoundaryTempPanel();

        // 初始化输入控件
        void initInputs()override;

        // 清除输入控件
        void clearInputs()override;

        // 控件设值
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>boundaryData)override;

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;

        // 作用对象输入控件
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;

        // 值输入控件
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        // 获取当前控件上的作用对象
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        // 获取函数
        int getFunc();

        // 失活并提交拾取数据：供统一退出出口调用
        void deactivatePickAndCommit();

        // 实时渲染温度边界符号
        void updateBoundarySymbolRealtime();

    private slots:
        // 函数
        void slot_funcChanged(int idx);
        // 定义方式切换
        void slot_defineTypeChanged(int idx);
        // 分析场
        void slot_analysisFieldChanged(int idx);
    private:
        // 应用拾取类型白名单：温度边界支持几何点/边/面/体 + 节点
        void applyAllowedPickTypes();

        // 汇总拾取+集合数据并计算渲染坐标
        void collectRenderPositions(QVector<QVector3D>& outPositions);

        // 单独保存各字段，不触发 sg_editFinished
        void saveTemp();
        void saveFunc();
        void saveAnalysisField();
        void saveScale();

        // 仅通知外部刷新树/渲染
        void notifyBoundaryChanged();

        // 实时渲染执行体
        void doUpdateBoundarySymbolRealtime();

        QHash<QString, int> _functable;
        InputWidgetSet _inp;
        comboBoxWithPushBtn combofunc;
        InputWidgetSet _scale;

        // 定义方式下拉框：0=统一赋值，1=分析场
        singleComboBox _defineTypeCombo;

        // 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        singleComboBox _analysisFieldCombo;
        QHash<QString, int> _analysisFieldTable;

        // 当前边界数据
        std::shared_ptr<MechCaseData::mBoundaryTemp> _currentBoundary;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 实时符号渲染防抖定时器
        QTimer* _realtimeUpdateTimer = nullptr;

        // 上次实时渲染的最终数据签名（对象ID+位置/方向/分量值），未变化时跳过符号赋值与全量重建
        size_t _lastRenderSig = 0;
    };
}