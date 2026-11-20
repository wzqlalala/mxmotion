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
#include <QLineEdit>
#include <QFileDialog>
#include <QTimer>
#include <memory>
#include <map>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"
#include "mConnector.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mPredefinedFieldPanelBase.h"
#include "mPredefinedFieldTempLoad.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"
#include "mDockLayoutManager.h"
#include "mPickWidgetController.h"
#include "mOtherFunction.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mPredefinedFieldTempLoadPanel : public mPredefinedFieldPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        mPredefinedFieldTempLoadPanel(QWidget* parent = nullptr);
        ~mPredefinedFieldTempLoadPanel();

        void initInputs()override;
        void clearInputs()override;
        void setInputsValue(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData)override;
        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;
        void storeFieldValue(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData);
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;
        int getFunc();

        /**
         * @brief 失活并提交拾取数据：供统一退出出口调用
         */
        void deactivatePickAndCommit();

        /**
         * @brief 实时渲染温度载荷符号
         */
        void updateLoadSymbolRealtime();

        /**
         * @brief 按当前定义方式设置拾取类型白名单
         */
        void applyAllowedPickTypes(int way);

    private slots:
        void slot_defineTypeChanged(int idx);
        void slot_funcChanged(int idx);
        void slot_interpolateTypeChanged(int idx);
        void slot_browseFile();
        void slot_midNodeInterpolateChanged(int state);
        // 离散数据控件（独立赋值表格）数据变化
        void slot_discreteDataChanged();
        // 离散数据控件单元格编辑
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);

        // 防抖定时器触发：合并短时间内的多次实时渲染请求
        void onRealtimeUpdateTimeout();

    private:
        void updateOpenedTable();
        void filterPicksToNodeOnly();
        void initValueInputs();
        void initFileInputs();

        // 单独保存各数据项
        void saveTemp();                        // 统一赋值温度
        void saveWayAndClearModeFields();       // 保存定义方式并清空模式字段
        void saveFilePath();                    // 文件路径
        void saveInterpolateInfo();             // 插值类型+中间节点插值
        void saveScale();                       // 缩放因子
        void saveFunc();                        // 幅值曲线映射
        void notifyLoadChanged();               // 仅通知外部刷新树/渲染

        // 实时渲染执行体；assignOnly=true 时仅把数据写入符号对象，不触发
        // UpdateAllSymbol/updatePaint（供切出面板时同步收尾，重建交给随后的树选中刷新）
        void doUpdateLoadSymbolRealtime(bool assignOnly = false);

        // 节点温度云图显示/关闭（独立赋值模式下按 _nodeTempMap 逐节点着色）
        void setNodeTempCloudChart(bool flag);

        QHash<QString, int>_functable;

        // 定义方式下拉框（已移入温度载荷定义折叠框）
        singleComboBox _defineTypeCombo;

        // 统一赋值内容
        InputWidgetSet _inp;
        comboBoxWithPushBtn combofunc;

        // 独立赋值内容
        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;
        // 通用离散数据控件（独立赋值模式的数据表格入口）
        mDiscreteDataControl* _discreteControl = nullptr;

        // 云图开关状态（独立赋值：开关在离散数据弹窗内，不再单独占行）
        bool _cloudOn = false;

        // 文件定义内容
        QWidget* _fileRowWidget = nullptr;
        QLabel* _fileLabel = nullptr;
        QLineEdit* _filePathEdit = nullptr;
        QPushButton* _fileBrowseBtn = nullptr;

        InputWidgetSet _scale;

        // 文件定义相关（保留旧控件用于非文件模式显隐，但实际文件模式使用 _fileRowWidget）
        InputWidgetSet _filePathInput;
        singleComboBox _interpolateTypeCombo;
        QCheckBox* _midNodeCheck = nullptr;
        comboBoxWithPushBtn combofuncFile;

        std::shared_ptr<mPredefinedFieldTempLoad> _currentField;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 实时符号渲染防抖定时器
        QTimer* _realtimeUpdateTimer = nullptr;

        // 标记当前是否处于设置值阶段，避免 updateOpenedTable 循环触发
        bool _isSettingValue = false;

        // 上次实时渲染的输入签名（fieldId+定义方式+汇总后作用对象）：
        // 未变化时跳过坐标重算与符号赋值（几何面坐标采样代价很高）
        int _lastRenderFieldId = -1;
        size_t _lastRenderSignature = 0;
    };
}
