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
#include <memory>
#include <unordered_map>
#include <map>
#include <QVariant>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"
#include "mLoadRadiation.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"
#include "mPickWidgetController.h"
#include "mOtherFunction.h"
#include "mFieldManager.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadRadiationPanel : public mLoadPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        mLoadRadiationPanel(QWidget* parent = nullptr);
        ~mLoadRadiationPanel();

        void initInputs()override;
        void clearInputs()override;
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)override;

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        void deactivatePickAndCommit();

        // 实时渲染热辐射符号
        void updateLoadSymbolRealtime();

        int getFunc();

    private slots:
        void slot_funcChanged(int idx);
        void slot_defineTypeChanged(int idx);
        void slot_analysisFieldChanged(int idx);
        // 离散数据控件（独立赋值表格）数据变化
        void slot_discreteDataChanged();
        // 离散数据控件单元格编辑
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);

    private:
        void filterPicksValid();
        void updateOpenedTable();
        void applyAllowedPickTypes(int defineType);

        // 局部保存辅助函数（不再 emit sg_editFinished）
        void saveV1V2();
        void saveFunc();
        void saveAnalysisField();
        void saveScale();
        void notifyLoadChanged();

        QVector<InputWidgetSet> inputWidgets;
        InputWidgetSet _scale;

        singleComboBox _defineTypeCombo;
        comboBoxWithPushBtn combofunc;
        QHash<QString, int>_functable;

        // 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        singleComboBox _analysisFieldCombo;
        QHash<QString, int> _analysisFieldTable;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 独立赋值表格行容器
        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;
        // 通用离散数据控件（独立赋值模式的数据表格入口）
        mDiscreteDataControl* _discreteControl = nullptr;

        // 当前载荷数据
        std::shared_ptr<MechCaseData::mLoadRadiation> _currentLoad;
    };
}