#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
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
#include <QFileDialog>
#include <memory>
#include <unordered_map>
#include <QSet>
#include <map>
#include <algorithm>
#include <QVariant>
#include <QVector3D>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"
#include "mLoadPressure.h"
#include "mFieldManager.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"
#include "mPickWidgetController.h"
#include "mDirectionSelectorWidget.h"
#include "mOtherFunction.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadPressurePanel : public mLoadPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        /*
        * 构造函数
        */
        mLoadPressurePanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadPressurePanel();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 退出编辑界面时清理（关闭云图显示）
        */
        void unEditInputs()override;

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;

        /*
        * 控件设值
        */
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)override;

        /*
        * 获取当前树上的作用对象
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

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
        * 连接单位制响应信号
        */
        void connectUnitSystemResponse()override;

        /*
        * 断开单位制响应信号
        */
        void disconnectUnitSystemResponse()override;

        void deactivatePickAndCommit();

    private slots:
        // 单位制切换响应
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);
        // 函数
        void slot_funcChanged(int idx);
        // 压强类型切换
        void slot_pressureTypeChanged(int idx);
        // 定义方式切换
        void slot_defineTypeChanged(int idx);
        // 分析场
        void slot_analysisFieldChanged(int idx);
        // 离散数据控件（独立赋值表格）数据变化
        void slot_discreteDataChanged();
        // 离散数据控件单元格编辑
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);
        // 浏览文件按钮
        void slot_browseFileClicked();

    private:
        // 确保方向选择控件已创建
        void ensureDirectionWidget();
        // 将方向控件当前状态写入 load 对象（方向矢量 + UI 状态）
        void applyDirectionWidgetToLoad(std::shared_ptr<MechCaseData::mLoadPressure> load);
        // 从 load 对象反显方向控件状态
        void restoreDirectionWidgetFromLoad(std::shared_ptr<MechCaseData::mLoadPressure> load);

        // 仅保留单元面拾取（独立赋值使用）
        void filterPicksToCellFaceOnly();
        // 根据当前模式限制可拾取类型
        void applyAllowedPickTypes(int pressureType, int defineType);
        // 以离散数据控件当前行为准重建 cellFaceMap_（独立赋值模式）
        void updateOpenedTable();
        // 根据压强类型和定义方式刷新值折叠框布局
        void rebuildValueLayout();
        // 实时刷新压力载荷符号
        void updateLoadSymbolRealtime();

        // 局部保存辅助函数（不再 emit sg_editFinished）
        void savePressure();
        void saveDepthInfo();
        void saveFunc();
        void saveAnalysisField();
        void saveScale();
        void saveModeState();
        void notifyLoadChanged();

        // 开启/关闭单元面载荷云图（数据取自当前载荷的cellFaceMap_）
        void setEleLoadCloudChart(bool flag);

        std::unordered_map<MxFunctions::SetType, std::vector<RenderPickResultData>>_picks;

        QStringList _pressureUnits;
        QHash<QString, int>_functable;
        QVector<InputWidgetSet> inputWidgets;
        comboBoxWithPushBtn combofunc;
        InputWidgetSet _scale;

        // 压强类型下拉框
        singleComboBox _pressureTypeCombo;

        // 定义方式下拉框
        singleComboBox _defineTypeCombo;

        // 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        singleComboBox _analysisFieldCombo;
        QHash<QString, int> _analysisFieldTable;
        // 表格行容器控件
        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;
        // 通用离散数据控件（独立赋值模式的数据表格入口）
        mDiscreteDataControl* _discreteControl = nullptr;

        // 云图开关状态（独立赋值：开关在离散数据弹窗内，不再单独占行）
        bool _cloudOn = false;

        // 文件选择控件
        QWidget* _fileRowWidget = nullptr;
        QLabel* _fileLabel = nullptr;
        QLineEdit* _filePathEdit = nullptr;
        QPushButton* _fileBrowseBtn = nullptr;

        // 当前压强载荷数据
        std::shared_ptr<mLoadPressure> _currentLoad;

        // 通用拾取控件
        mPickWidgetController* _pickWidgetController = nullptr;

        // 净水压力输入
        mDirectionSelectorWidget* _directionWidget = nullptr;

        InputWidgetSet _zeroDepthInput;
        InputWidgetSet _refDepthInput;
        InputWidgetSet _refPressureInput;
    };
}
