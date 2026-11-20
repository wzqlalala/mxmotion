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
#include "mLoadConvection.h"
#include "mDiscreteDataControl.h"
#include "mPickWidgetController.h"
#include "mOtherFunction.h"
#include "mFieldManager.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadConvectionPanel : public mLoadPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        mLoadConvectionPanel(QWidget* parent = nullptr);
        ~mLoadConvectionPanel();

        void initInputs()override;
        void clearInputs()override;
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)override;

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar)override;

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        void deactivatePickAndCommit();

        void updateLoadSymbolRealtime();
        void onRealtimeUpdateTimeout();
        int getFunc();
        int getFuncConv();
    private slots:
        void slot_funcChanged(int idx);
        void slot_defineTypeChanged(int idx);
        void slot_analysisFieldChanged(int idx);
        void slot_convAnalysisFieldChanged(int idx);
        // 离散数据控件（独立赋值表格）数据变化
        void slot_discreteDataChanged();
        // 离散数据控件单元格编辑
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);
        void slot_browseFileClicked();

    private:
        void filterPicksToCellFaceOnly();
        void updateOpenedTable();
        void syncPicksToController();
        void applyAllowedPickTypes();
        void rebuildValueLayout();

        void saveConvectionValue();
        void saveFunc();
        void saveFuncConv();
        void saveAnalysisField();
        void saveConvAnalysisField();
        void saveScale();
        void notifyLoadChanged();

        void doUpdateLoadSymbolRealtime();

        QHash<QString, int> _functable;
        QHash<QString, int> _functableConv;
        QVector<singleComboBox> comboBoxs;
        comboBoxWithPushBtn combofunc;
        comboBoxWithPushBtn combofuncConv;

        singleComboBox _defineTypeCombo;

        // 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        singleComboBox _analysisFieldCombo;
        QHash<QString, int> _analysisFieldTable;
        // 对流换热系数分析场下拉框（convfield）
        singleComboBox _convAnalysisFieldCombo;
        QHash<QString, int> _convAnalysisFieldTable;

        QVector<InputWidgetSet> inputWidgets;
        InputWidgetSet _scale;

        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;
        // 通用离散数据控件（独立赋值模式的数据表格入口）
        mDiscreteDataControl* _discreteControl = nullptr;

        QWidget* _fileRowWidget = nullptr;
        QLabel* _fileLabel = nullptr;
        QLineEdit* _filePathEdit = nullptr;
        QPushButton* _fileBrowseBtn = nullptr;

        mPickWidgetController* _pickWidgetController = nullptr;
        std::shared_ptr<MechCaseData::mLoadConvection> _currentLoad;

        QTimer* _realtimeUpdateTimer = nullptr;

        // 上次实时渲染的最终数据签名（对象ID+位置/方向/分量值），未变化时跳过符号赋值与全量重建
        size_t _lastRenderSig = 0;
    };
}
