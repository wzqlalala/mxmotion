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
#include <QTimer>
#include <memory>
#include <map>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"
#include "algorithm"
//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"

#include "mLoadForce.h"
#include "mFieldManager.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"
#include "mDockLayoutManager.h"
#include "mOtherFunction.h"

#include "mPickWidgetController.h"
#include "mDirectionSelectorWidget.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadForcePanel : public mLoadPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        /*
        * 构造函数
        */
        mLoadForcePanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadForcePanel();

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
        * 失活并提交拾取数据：供统一退出出口调用
        */
        void deactivatePickAndCommit();

        /*
        * 实时渲染载荷箭头符号
        */
        void updateLoadSymbolRealtime();

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

        /*
        * 连接单位制响应信号
        */
        void connectUnitSystemResponse()override;

        /*
        * 断开单位制响应信号
        */
        void disconnectUnitSystemResponse()override;

        /*
        * 清空数据
        */
        void clearData();

        /*
        * 按当前定义方式设置拾取类型白名单
        */
        void applyAllowedPickTypes(int way);
    private slots:
        /*
        * 单位制切换响应
        */
        void slot_unitSystemResponse(UnitType, QVector<int>);

        /*
        * 定义方式改变
        */
        void slot_defineTypeChanged(int idx);

        /*
        * 离散数据控件（独立赋值表格）数据变化
        */
        void slot_discreteDataChanged();

        /*
        * 离散数据控件单元格编辑
        */
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);

        /*
        * 幅值曲线
        */
        void slot_funcChanged(int idx);

        /*
        * 坐标系
        */
        void slot_coorChanged(int idx);

        /*
        * 分析场
        */
        void slot_analysisFieldChanged(int idx);

        /*
         * 浏览文件按钮
        */
        void slot_browseFileClicked();

        // 防抖定时器触发：合并短时间内的多次实时渲染请求
        void onRealtimeUpdateTimeout();

    private:
        void updateOpenedTable();
        void filterPicksToNodeOnly();
        void ensureDirectionWidget();

        // 单独保存各数据项
        void saveForceComponents();             // 各向分量 X/Y/Z
        void saveWayAndClearModeFields();       // 保存定义方式并清空模式字段
        void saveDirectionAndMagnitude();       // 方向与大小模式
        void saveScale();                       // 缩放因子
        void saveFilePath();                    // 文件路径
        void saveFunc();                        // 幅值曲线映射
        void saveCoor();                        // 坐标系映射
        void saveAnalysisField();               // 分析场映射
        void notifyLoadChanged();               // 仅通知外部刷新树/渲染

        // 将方向控件当前状态写入 load 对象（大小、方向矢量、dirPickState_）
        void applyDirectionWidgetToLoad(std::shared_ptr<MechCaseData::mLoadForce> load);

        // 实时渲染执行体：由 updateLoadSymbolRealtime 启动/重启的防抖定时器统一调用。
        void doUpdateLoadSymbolRealtime();

        mPickWidgetController* pickWidgetController() const { return _pickWidgetController; }

        QHash<QString, int>_functable;
        QList<QTreeWidgetItem*>_selectedItems;
        treeWidgetWithPushBtns _treewidgetSet;
        QVector<singleComboBox> comboBoxs;
        comboBoxWithPushBtn combofunc;
        comboBoxWithPushBtn combocoor;

        // 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        singleComboBox _analysisFieldCombo;
        QHash<QString, int> _analysisFieldTable;

        // 新增：各向分量模式输入框（X/Y/Z分量）
        QVector<InputWidgetSet> inputWidgets;

        // 文件选择控件
        QWidget* _fileRowWidget = nullptr;
        QLabel* _fileLabel = nullptr;
        QLineEdit* _filePathEdit = nullptr;
        QPushButton* _fileBrowseBtn = nullptr;

        // 独立赋值控件
        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;

        // 通用离散数据控件（独立赋值模式的数据表格入口）
        mDiscreteDataControl* _discreteControl = nullptr;

        InputWidgetSet _scale;

        std::shared_ptr<mLoadForce> _currentLoad;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 方向选择控件（方向与大小模式使用）
        mDirectionSelectorWidget* _directionWidget = nullptr;

        // 实时符号渲染防抖定时器：合并方向/大小/拾取连续变化触发的多次刷新。
        QTimer* _realtimeUpdateTimer = nullptr;

        // 上次实时渲染的最终数据签名（对象ID+位置/方向/分量值），未变化时跳过符号赋值与全量重建
        size_t _lastRenderSig = 0;
    };
}
