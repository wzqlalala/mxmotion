#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mTableDataWindow.h"
#include "mxr/data/SelectDataStruct.h"
#include "mPhysicalFieldEnum.h"
#include "mDockLayoutManager.h"
#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialog>
#include <QProgressBar>
#include "mPanelBase.h"

#include "mCalcLineEdit.h"
#include "mPropertyManager.h"
#include "mPropertyBase.h"
#include "mCaseManager.h"
#include "mCaseBase.h"

namespace MechPreData
{
    class mViewportModel;
}

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mStructual : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mStructual(QWidget* parent = nullptr);

        // 新建面板
        void newMyPanel();

        // 关闭/取消编辑
        void uneditMyPanel();
    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
    private:
        void initTabButtons();
        void switchTab(int index);
        void initDesignSpaceTab();
        void initOptimizationProblemTab();
        void initOptimizationSolverTab();
        void updateDesignSpaceTableForType(int type);

        void populateShellSectionCombo(QComboBox* combo);
        void populateStaticCaseCombos();
        QStringList getStaticCaseNames();
        QStringList getModalCaseNames();

        QVBoxLayout* _mainLayout = nullptr;
        QWidget* _tabButtonContainer = nullptr;
        QPushButton* _tabDesignSpaceBtn = nullptr;
        QPushButton* _tabProblemBtn = nullptr;
        QPushButton* _tabSolverBtn = nullptr;
        QStackedWidget* _stackedWidget = nullptr;

        // 设计空间
        QComboBox* _componentTypeCombo = nullptr;
        QComboBox* _designVariableTypeCombo = nullptr;
        QPushButton* _discreteValueBtn = nullptr;
        QTableWidget* _designSpaceTable = nullptr;

        // 优化算法
        QComboBox* _solverTypeCombo = nullptr;
        QLineEdit* _maxSampleCountEdit = nullptr;

        // 优化问题
        QComboBox* _objectiveResponseCombo = nullptr;
        QComboBox* _objectiveCaseCombo = nullptr;
        QCheckBox* _massConstraintCheck = nullptr;
        QLineEdit* _massConstraintEdit = nullptr;
        QCheckBox* _stiffnessConstraintCheck = nullptr;
        QLineEdit* _stiffnessConstraintEdit = nullptr;
        QComboBox* _stiffnessCaseCombo = nullptr;
        QCheckBox* _stressConstraintCheck = nullptr;
        QLineEdit* _stressLimitEdit = nullptr;
        QComboBox* _stressCaseCombo = nullptr;
        QCheckBox* _frequencyConstraintCheck = nullptr;
        QLineEdit* _frequencyLimitEdit = nullptr;
        QLineEdit* _frequencyOrderEdit = nullptr;
        QComboBox* _frequencyCaseCombo = nullptr;

        // 优化求解
        QLineEdit* _currentIterEdit = nullptr;
        QLineEdit* _sampledCountEdit = nullptr;
        QProgressBar* _progressEdit = nullptr;
        QTableWidget* _monitorTable = nullptr;
        QPushButton* _viewDetailBtn = nullptr;
        QMap<QLabel*, QCheckBox*> _labelToCheckMap;
    private slots:
        void onShowDiscreteValueTable();
        void onDesignVariableTypeChanged(int index);
        void onRunOptimization();
        void onResetWizard();
        void onMassConstraintToggled(bool checked);
        void onStiffnessConstraintToggled(bool checked);
        void onStressConstraintToggled(bool checked);
        void onFrequencyConstraintToggled(bool checked);
    };
}
