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
#include <QHeaderView>
#include <QTableWidget>
#include <QVector>
#include <memory>
#include <map>
#include <algorithm>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include "mLoadPanelBase.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"
#include "mDockLayoutManager.h"
#include "mLoadSurface.h"
#include "mPickWidgetController.h"
#include "mDirectionSelectorWidget.h"
#include "mOtherFunction.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadSurfacePanel : public mLoadPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        mLoadSurfacePanel(QWidget* parent = nullptr);
        ~mLoadSurfacePanel();

        void initInputs() override;
        void clearInputs() override;
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData) override;

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        int getFunc();

        void connectUnitSystemResponse() override;
        void disconnectUnitSystemResponse() override;

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData() override;
        void clearData();

        // 失活并提交拾取数据：供统一退出出口调用
        void deactivatePickAndCommit();

        // 实时渲染表面载荷符号
        void updateLoadSymbolRealtime();

    private slots:
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);
        void slot_distributeTypeChanged(int idx);
        void slot_defineTypeChanged(int idx);
        void slot_dirModeChanged(int idx);
        void slot_funcChanged(int idx);

        void slot_discreteDataChanged();
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);

    private:
        void updateOpenedTable();
        void filterPicksToElementSurfaceOnly();
        void applyAllowedPickTypes(int defineType);
        void rebuildValueLayout();
        void ensureDirectionWidget();

        // 局部保存辅助函数（不再 emit sg_editFinished）
        void saveLoadValue();       // 统一赋值：载荷值 + 参考方向
        void saveDirMode();         // 方向模式切换
        void saveFunc();            // 幅值曲线映射
        void saveScale();           // 缩放因子
        void notifyLoadChanged();   // 仅通知外部刷新树/渲染

        void applyDirectionWidgetToLoad(std::shared_ptr<MechCaseData::mLoadSurface> load);
        void restoreDirectionWidgetFromLoad(std::shared_ptr<MechCaseData::mLoadSurface> load);

        QHash<QString, int> _functable;
        QVector<singleComboBox> comboBoxs;    // 0=分布类型,1=定义方式,2=方向模式
        comboBoxWithPushBtn combofunc;

        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;
        // 通用离散数据控件（独立赋值模式的数据表格入口）
        mDiscreteDataControl* _discreteControl = nullptr;

        InputWidgetSet _scale;
        std::shared_ptr<mLoadSurface> _currentLoad;

        QStringList _pressureUnits;

        // 统一赋值：载荷值
        InputWidgetSet _loadValueInput;

        // 统一赋值：方向选择控件（替代原参考方向 QLineEdit）
        mDirectionSelectorWidget* _directionWidget = nullptr;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;
    };
}
