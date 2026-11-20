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
#include <QMap>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"
#include "mConnector.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mBoundaryPanelBase.h"
#include "mPickWidgetController.h"
#include "mBoundaryDisplacement.h"
#include "mOtherFunction.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mBoundaryDisplacementPanel : public mBoundaryPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        mBoundaryDisplacementPanel(QWidget* parent = nullptr);
        ~mBoundaryDisplacementPanel();

        // 初始化输入控件
        void initInputs() override;

        // 清除输入控件
        void clearInputs() override;

        // 控件设值
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData) override;

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;

        // 作用对象输入控件
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;

        // 值输入控件
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        // 获取当前控件上的作用对象
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData() override;

        // 获取函数
        int getFunc();

        // 失活并提交拾取数据：供统一退出出口调用
        void deactivatePickAndCommit();

        // 实时渲染边界符号（启动防抖定时器）
        void updateBoundarySymbolRealtime();
        // 实时渲染边界符号执行体
        void doUpdateBoundarySymbolRealtime();

    private slots:
        // 定义方式切换
        void slot_definitionModeChanged(int idx);

        // 离散数据控件（独立赋值表格）数据变化
        void slot_discreteDataChanged();

        // 离散数据控件单元格编辑
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);

        // 函数
        void slot_funcChanged(int idx);

    private:
        void updateModeWidgets(int mode);
        void updateOpenedTable();
        void filterPicksToNodeOnly();

        // 应用拾取类型白名单
        void applyAllowedPickTypes(int mode);

        // 单独保存各数据项，避免每次修改都 emit sg_editFinished 触发全量 storeBoundaryValue
        void saveDefinitionMode();      // 定义方式
        void saveComponents();          // U1..UR3 分量
        void saveScale();               // 缩放因子
        void saveFunc();                // 幅值曲线映射
        void notifyBoundaryChanged();   // 仅通知外部刷新树/渲染

        // 将 QMap<int, QMap<int, QVariant>> 独立赋值缓存转换为通用 std::map 结构
        static std::map<RenderPickResultData, QVector<QVariant>> convertIndependentTable(
            const QMap<int, QMap<int, QVariant>>& cache);
        // 将通用 std::map 结构转换回 QMap<int, QMap<int, QVariant>> 缓存
        static QMap<int, QMap<int, QVariant>> convertIndependentTable(
            const std::map<RenderPickResultData, QVector<QVariant>>& data);

        // 汇总拾取+集合数据并计算渲染坐标
        void collectRenderPositions(QVector<QVector3D>& outPositions);

        // 当前边界数据
        std::shared_ptr<MechCaseData::mBoundaryDisplacement> _currentBoundary;

        QHash<QString, int> _functable;
        QVector<InputWithCheckBox> InputWidgets;
        comboBoxWithPushBtn combofunc;

        // 定义方式：0=各向分量，1=独立赋值
        singleComboBox _comboDefinitionMode;

        // 表格行容器控件（独立赋值模式：通用离散数据控件入口 + 锚定弹窗）
        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;
        // 通用离散数据控件（独立赋值模式的数据表格入口）
        mDiscreteDataControl* _discreteControl = nullptr;

        // 缩放因子
        InputWidgetSet _scale;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 实时符号渲染防抖定时器
        QTimer* _realtimeUpdateTimer = nullptr;

        // 上次实时渲染的最终数据签名（对象ID+位置/方向/分量值），未变化时跳过符号赋值与全量重建
        size_t _lastRenderSig = 0;
    };
}