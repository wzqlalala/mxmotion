#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QDialog>
#include <QMap>

QT_BEGIN_NAMESPACE
class QListWidget;
class QStackedWidget;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QDialogButtonBox;
class QTableWidget;
class QTableWidgetItem;
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;
class QLabel;
class QRadioButton;
class QGridLayout;
class QScrollArea;
class IncentiveNodeTableWidget;
class NavIconDelegate;
class ResponseSettingFoldablePanel;
class CreateCurveDialog;
class IncentiveSettingTableWidget;
QT_END_NAMESPACE

namespace MechModelData
{
    class mModelData;
}

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mCaseImplicitFrequencyResponseWindow : public QDialog
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mCaseImplicitFrequencyResponseWindow(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mCaseImplicitFrequencyResponseWindow();

        /*
        * 恢复窗口默认状态（复用已关闭的窗口实例时调用）
        */
        void resetToDefault();

    private:
        /*
        * 初始化界面布局
        */
        void initUI();

        /*
        * 初始化信号连接
        */
        void initConnect();
        /*
        * 输入框输入限制：数值/整数输入框非法输入直接置空
        */
        void installInputValidators();

        /*
        * 构造完成后快照控件默认状态
        */
        void captureDefaultState();

        /*
        * 初始化左侧竖向导航栏
        */
        void initLeftNavigation();

        /*
        * 构建第一个页面：分析设置
        */
        QWidget* createPageAnalysisMethod();

        /*
        * 构建第二个页面：频率范围设置
        */
        QWidget* createPageFrequencyRange();

        /*
        * 构建第三个页面：阻尼设置
        */
        QScrollArea* createPageDamping();

        /*
        * 构建第四个页面：约束设置
        */
        QWidget* createPageConstraint();

        /*
        * 构建第五个页面：激励点设置
        */
        QWidget* createPageIncentivePoint();

        /*
        * 构建第六个页面：响应设置
        */
        QScrollArea* createPageResponse();

        /*
        * 构建第七个页面：输出设置
        */
        QScrollArea* createPageOutput();

        /*
        * 更新底部按钮状态
        */
        void updateBottomButtons(int currentIndex);

        /*
        * 更新下一步按钮文本
        */
        void updateNextButtonText(int currentIndex);

        /*
        * 刷新导航图标
        */
        void refreshNavigationIcons();

        /*
        * 更新所有导航项完成状态与输出页按钮状态
        */
        void updateValidationState();

        // 根据求解器类型应用可用/可见状态
        void applySolverDependentState();

        // 各页面有效性校验
        bool isAnalysisSettingValid() const;
        bool isFrequencyRangeValid() const;
        bool isFrequencyGroupValid(int groupIndex) const;
        bool isIncentiveValid() const;
        bool isDampingValid() const;
        bool isFluidDampingValid() const;
        bool isConstraintValid() const;
        bool isResponseValid() const;
        bool isOutputValid() const;

        // 更新输出页操作按钮可用状态
        void updateOutputActionButtons();

        // 判断指定导航页是否可交互（已到达/当前页/已完成）
        bool isNavigationIndexEnabled(int index) const;

        // 记录用户已到达的最远步骤索引
        void updateFarthestReachedIndex(int index);

        // 用户修改当前页后，将后续步骤重置为未完成
        void resetSubsequentSteps(int fromIndex);

        // 检查当前页相对于已到达位置是否发生回退修改，若是则重置后续状态
        void onPageModified(int pageIndex);

        // 判断输入框是否为有效浮点数
        bool isValidDouble(QLineEdit* lineEdit) const;

        // 判断输入框是否为有效正整数
        bool isValidPositiveInt(QLineEdit* lineEdit) const;

        // 连接表格中所有复选框状态变化到指定修改回调
        void connectSettingTableCheckBoxes(IncentiveSettingTableWidget* table, int pageIndex);
        void connectConstraintTableCheckBoxes(IncentiveNodeTableWidget* table, int pageIndex);
        /*
        * 更新删除按钮状态
        */
        void updateConstraintDeleteButtonState();
        void updateForceSettingDeleteButtonState();
        void updateIncentiveSettingDeleteButtonState();
        void updateModalDampingDeleteButtonState();
        void updateFluidModalDampingDeleteButtonState();
        void updatePeakSelectionDeleteButtonState();

        /*
        * 创建声固耦合分析
        */
        void createASCouplingAnalysis(bool isForce, const QString& name, int forceLoadID, int i);
        /*
        * 创建频响分析
        */
        void createFrequencyResponseAnalysis(bool isForce, const QString& name, int forceLoadID, int i);
        /*
        * 创建采样频率点
        */
        int createFrequencyPoint(const QString& name);
        /*
        * 创建力矩或力载荷
        */
        void createForce(bool isForce, const int& row, const int& col, const int& i);
        /*
        * 创建等效辐射功率
        */
        void readERPData();

        /*
        * 从 txt/csv 导入设置表（节点ID + 6 自由度勾选，不含曲线数据）
        */
        void importSettingTableFromFile(IncentiveSettingTableWidget* table, QLineEdit* countEdit, const QString& title);

        /*
        * 导出设置表到 txt/csv（节点ID + 6 自由度勾选，不含曲线数据）
        */
        void exportSettingTableToFile(IncentiveSettingTableWidget* table, const QString& title);

    private slots:
        /*
        * 关闭按钮
        */
        void slot_onClose();

        /*
        * 创建 Mx 工况按钮
        */
        void slot_onCreateMxCase();

        /*
        * 导出 fem 文件按钮
        */
        void slot_onExportFem();

        /*
        * 上一步按钮
        */
        void slot_onPreviousStep();

        /*
        * 下一步按钮
        */
        void slot_onNextStep();

        /*
        * 添加一行约束
        */
        void slot_onAddConstraintRow();

        /*
        * 删除选中的约束行
        */
        void slot_onRemoveConstraintRow();

        /*
        * 删除选中的模态阻尼行
        */
        void slot_onRemoveModalDampingRow();

        /*
        * 删除选中的流体模态阻尼行
        */
        void slot_onRemoveFluidModalDampingRow();

        /*
        * 删除选中的峰值筛选行
        */
        void slot_onRemovePeakSelectionRow();

        /*
        * 添加一行力载荷设置
        */
        void slot_onAddForceSettingRow();

        /*
        * 添加一行激励设置
        */
        void slot_onAddIncentiveSettingRow();

        /*
        * 添加一行全局模态阻尼
        */
        void slot_onAddModalDampingRow();

        /*
        * 添加一行流体全局模态阻尼
        */
        void slot_onAddFluidModalDampingRow();

        /*
       * 添加一行峰值筛选
       */
        void slot_onAddPeakSelectionRow();

        /*
        * 根据节点个数输入框调整约束行数
        */
        void slot_onConstraintPointCountChanged();

        /*
        * 根据节点个数输入框调整力载荷设置行数
        */
        void slot_onForceSettingCountChanged();

        /*
        * 根据节点个数输入框调整激励设置行数
        */
        void slot_onIncentiveSettingCountChanged();

        /*
        * 根据节点个数输入框调整模态阻尼行数
        */
        void slot_onModalDampingPointCountChanged();

        /*
        * 根据节点个数输入框调整流体模态阻尼行数
        */
        void slot_onFluidModalDampingPointCountChanged();

        /*
        * 导入约束文件（*.txt / *.csv）
        */
        void slot_onImportConstraintFile();

        /*
        * 导出约束文件（*.txt / *.csv）
        */
        void slot_onExportConstraintFile();
      
        /*
        * 导入阻尼文件（*.txt / *.csv）
        */
        void slot_onImportDampingFile();
      
        /*
        * 导出阻尼文件（*.txt / *.csv）
        */
        void slot_onExportDampingFile();

        /*
        * 导入流体阻尼文件（*.txt / *.csv）
        */
        void slot_onImportFluidDampingFile();

        /*
        * 导出流体阻尼文件（*.txt / *.csv）
        */
        void slot_onExportFluidDampingFile();

        /*
        * 导入峰值筛选文件（*.txt / *.csv）
        */
        void slot_onImportPeakSelectionFile();

        /*
        * 导出峰值筛选文件（*.txt / *.csv）
        */
        void slot_onExportPeakSelectionFile();

        /*
        * 导入力载荷设置文件（*.txt / *.csv）
        */
        void slot_ImportIncentiveFromFile();

        /*
        * 导出力载荷设置文件（*.txt / *.csv）
        */
        void slot_onExportIncentiveFile();

        /*
        * 导入激励设置文件（*.txt / *.csv）
        */
        void slot_ImportJiliFromFile();

        /*
        * 导出激励设置文件（*.txt / *.csv）
        */
        void slot_onExportJiliFile();

        void slot_onSolverChanged();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

        /*
        * 关闭事件：工况未创建时二次确认
        */
        void closeEvent(QCloseEvent* event) override;

    private:
        // 左侧竖向导航栏
        QListWidget* m_listWidget_Nav = nullptr;
        NavIconDelegate* m_navIconDelegate = nullptr;
        // 右侧内容堆叠容器
        QStackedWidget* m_stackWidget = nullptr;

        // 第一页：分析设置（含分析方法、模态提取设置）
        QLineEdit* m_lineEdit_CaseName;
        QWidget* m_analysisSettingsContainer = nullptr;
        QRadioButton* m_direct = nullptr;
        QRadioButton* m_modal = nullptr;
        QComboBox* m_comboBox_Solver = nullptr;
        QWidget* m_rangeWidget = nullptr;
        QCheckBox* m_checkBox_ModeScale = nullptr;
        QLineEdit* m_lineEdit_ModeScale = nullptr;
        QLineEdit* m_lineEdit_FreqMin = nullptr;
        QLineEdit* m_lineEdit_FreqMax = nullptr;
        QLineEdit* m_lineEdit_ModeCount = nullptr;
        QLineEdit* m_lineEdit_FluidFreqMin = nullptr;
        QLineEdit* m_lineEdit_FluidFreqMax = nullptr;
        QLineEdit* m_lineEdit_FluidModeCount = nullptr;
        QWidget* m_ASCoupling_Widget = nullptr;
        QWidget* ratioWidget = nullptr;
        QCheckBox* m_checkBox_AcousticSolidCoupling = nullptr;
        QLineEdit* m_lineEdit_NormalExtRatio = nullptr;
        QLineEdit* m_lineEdit_SknepsExtRatio = nullptr;
        QCheckBox* m_checkbox_FluidRangeSetting;
        QWidget* m_FluidRangeSettingContainer;

        // 第二页：频率范围设置
        QWidget* m_widget_FREQ = nullptr;
        QWidget* m_widget_FREQ1 = nullptr;
        QWidget* m_widget_FREQ2 = nullptr;
        QWidget* m_widget_FREQ3 = nullptr;
        QWidget* m_widget_FREQ4 = nullptr;
        QCheckBox* m_FREQ = nullptr;
        QLineEdit* m_FrepPoint1 = nullptr;
        QCheckBox* m_FREQ1 = nullptr;//默认勾选
        QLineEdit* m_FREQ1_F1 = nullptr;
        QLineEdit* m_FREQ1_DF = nullptr;
        QLineEdit* m_FREQ1_NDF = nullptr;
        QCheckBox* m_FREQ2 = nullptr;
        QLineEdit* m_FREQ2_F1 = nullptr;
        QLineEdit* m_FREQ2_DF = nullptr;
        QLineEdit* m_FREQ2_NF = nullptr;
        QCheckBox* m_FREQ3 = nullptr;
        QLineEdit* m_FREQ3_F1 = nullptr;
        QLineEdit* m_FREQ3_F2 = nullptr;
        QComboBox* m_FREQ3_Form = nullptr;
        QLineEdit* m_FREQ3_NEF = nullptr;
        QLineEdit* m_FREQ3_CLUSTER = nullptr;
        QCheckBox* m_FREQ4 = nullptr;
        QLineEdit* m_FREQ4_F1 = nullptr;
        QLineEdit* m_FREQ4_F2 = nullptr;
        QLineEdit* m_FREQ4_FSPD = nullptr;
        QLineEdit* m_FREQ4_NFM = nullptr;

        // 第三页：阻尼设置
         //结构阻尼
        QCheckBox* m_checkBox_EnableDamping = nullptr;
        QWidget* m_dampingSettingsContainer = nullptr;
        QRadioButton* m_StructuralDamping = nullptr;
        QLineEdit* m_editStructuralDamping = nullptr;
        QRadioButton* m_ModalDamping = nullptr;
        QLineEdit* m_ModalDamping_PointCount = nullptr;
        QComboBox* m_ModalDamping_Type = nullptr;
        QPushButton* m_ModalDamping_AddRow = nullptr;
        QPushButton* m_ModalDamping_RemoveRow = nullptr;
        QPushButton* m_ModalDamping_ImportFile = nullptr;
        QPushButton* m_ModalDamping_ExportFile = nullptr;
        QTableWidget* m_ModalDamping_Table = nullptr;
        QWidget* m_modelDampingWidget = nullptr;
         //流体阻尼
        QCheckBox* m_checkBox_FluidEnableDamping = nullptr;
        QWidget* m_FluiddampingSettingsContainer = nullptr;
        QRadioButton* m_FluidStructuralDamping = nullptr;
        QLineEdit* m_FluideditStructuralDamping = nullptr;
        QRadioButton* m_FluidModalDamping = nullptr;
        QLineEdit* m_FluidModalDamping_PointCount = nullptr;
        QComboBox* m_FluidModalDamping_Type = nullptr;
        QPushButton* m_FluidModalDamping_AddRow = nullptr;
        QPushButton* m_FluidModalDamping_RemoveRow = nullptr;
        QPushButton* m_FluidModalDamping_ImportFile = nullptr;
        QPushButton* m_FluidModalDamping_ExportFile = nullptr;
        QTableWidget* m_FluidModalDamping_Table = nullptr;
        QWidget* m_FluidmodelDampingWidget = nullptr;

        // 第四页：约束设置
        QLineEdit* m_constraint_PointCount = nullptr;
        QPushButton* m_constraint_AddRow = nullptr;
        QPushButton* m_constraint_RemoveRow = nullptr;
        QPushButton* m_constraint_ImportFile = nullptr;
        QPushButton* m_constraint_ExportFile = nullptr;
        IncentiveNodeTableWidget* m_constraint_NodeTable = nullptr;

        // 第五页：激励点设置
        QPushButton* m_Force_toggleButton = nullptr;;
        QWidget* m_Force_contentWidget = nullptr;
        QLineEdit* m_Force_PointCount = nullptr;
        QComboBox* m_Force_LoadForm = nullptr;
        QPushButton* m_Force_AddRow = nullptr;
        QPushButton* m_Force_RemoveRow = nullptr;
        QPushButton* m_Force_ImportFile = nullptr;
        QPushButton* m_Force_ExportFile = nullptr;
        IncentiveSettingTableWidget* m_Force_Table = nullptr;

        QPushButton* m_jili_toggleButton = nullptr;;
        QWidget* m_jili_contentWidget = nullptr;
        QLineEdit* m_jili_PointCount = nullptr;
        QComboBox* m_jili_LoadForm = nullptr;
        QComboBox* m_jili_Type = nullptr;
        QPushButton* m_jili_AddRow = nullptr;
        QPushButton* m_jili_RemoveRow = nullptr;
        QPushButton* m_jili_ImportFile = nullptr;
        QPushButton* m_jili_ExportFile = nullptr;
        IncentiveSettingTableWidget* m_jili_Table = nullptr;

        QPushButton* m_zuhe_toggleButton = nullptr;;
        QWidget* m_zuhe_contentWidget = nullptr;
        QLineEdit* m_zuhe_Number = nullptr;
        // 第六页：响应设置
        ResponseSettingFoldablePanel* m_SoundPressureResponse = nullptr;
        ResponseSettingFoldablePanel* m_VibrationResponse = nullptr;
        ResponseSettingFoldablePanel* m_NodalForces = nullptr;
        ResponseSettingFoldablePanel* m_ElementForces = nullptr;
        ResponseSettingFoldablePanel* m_ConstraintReaction = nullptr;
        
        // 第七页：输出设置
         // 响应点曲线结果输出
        QPushButton* m_outputCurveButton = nullptr;
        QWidget* m_outputCurveContentWidget = nullptr;
        QCheckBox* m_OutputDisplacement = nullptr;
        QCheckBox* m_OutputSpeed = nullptr;
        QCheckBox* m_Outputaccelerated = nullptr;
        QCheckBox* m_OutputNodesForce = nullptr;
        QCheckBox* m_OutputElementForce = nullptr;
        QCheckBox* m_OutputConstraintReaction = nullptr;
        QCheckBox* m_OutputSoundPressure = nullptr;
         // 全局云图结果输出
        QPushButton* m_outputCloudButton = nullptr;
        QWidget* m_outputCloudContentWidget = nullptr;
        QComboBox* m_comboBox_outputForm = nullptr;
        QCheckBox* m_ODSDisplacement = nullptr;
        QCheckBox* m_ODSStrain = nullptr;
        QCheckBox* m_ODSPeckout = nullptr;
        QCheckBox* m_PhysicalDisplacement = nullptr;
        QCheckBox* m_PhysicalSpeed = nullptr;
        QCheckBox* m_Physicalaccelerated = nullptr;
        QCheckBox* m_PhysicalStress = nullptr;
        QCheckBox* m_PhysicalStrain = nullptr;
        QCheckBox* m_PhysicalStrainEnergy = nullptr;
         //峰值筛选
        QPushButton* m_PeakSelectionButton = nullptr;
        QWidget* m_PeakSelectionContentWidget = nullptr;
        QCheckBox* m_PeakoutSetting = nullptr;
        QWidget* m_PeakoutSettingContentWidget = nullptr;
        QLineEdit* m_edit_SID = nullptr;
        QLineEdit* m_edit_NPEAK = nullptr;
        QLineEdit* m_edit_NEAR = nullptr;
        QLineEdit* m_edit_FAR = nullptr;
        QLineEdit* m_edit_LFREQ = nullptr;
        QLineEdit* m_edit_HFREQ = nullptr;
        QComboBox* m_comboBox_TYPE = nullptr;
        QComboBox* m_comboBox_SCALE = nullptr;
        QPushButton* m_PeakSelection_AddRow = nullptr;
        QPushButton* m_PeakSelection_RemoveRow = nullptr;
        QPushButton* m_PeakSelection_ImportFile = nullptr;
        QPushButton* m_PeakSelection_ExportFile = nullptr;
        QTableWidget* m_PeakSelection_Table = nullptr;
         // 等效辐射功率（ERP）结果输出
        QPushButton* m_erpOutputButton = nullptr;
        QWidget* m_erpOutputContentWidget = nullptr;
        QCheckBox* m_OutputERP = nullptr;
        QComboBox* m_comboBox_ERPCurveType = nullptr;
        QLineEdit* m_edit_ERPC = nullptr;
        QLineEdit* m_edit_ERPRLF = nullptr;
        QLineEdit* m_edit_ERPRHO = nullptr;
        QLineEdit* m_edit_RHOCP = nullptr;
        QLineEdit* m_edit_ERPREFDB = nullptr;
         //贡献量结果输出
        QPushButton* m_ContributionButton = nullptr;
        QWidget* m_ContributionContentWidget = nullptr;
        QCheckBox* m_ModalContribution1 = nullptr;
        QCheckBox* m_ModalContribution2 = nullptr;
        QCheckBox* m_ModalContribution3 = nullptr;
        QCheckBox* m_NodeContribution1 = nullptr;
        QCheckBox* m_NodeContribution2 = nullptr;
        QCheckBox* m_NodeContribution3 = nullptr;
        QCheckBox* m_PanelContribution1 = nullptr;
        QCheckBox* m_PanelContribution2 = nullptr;

        // 底部按钮
        QPushButton* m_pushButton_Previous = nullptr;
        QPushButton* m_pushButton_Next = nullptr;
        QPushButton* m_pushButton_CreateMxCase = nullptr;
        QPushButton* m_pushButton_ExportFem = nullptr;
        QHBoxLayout* m_bottomLayout = nullptr;

        // 导航是否已解锁（到达输出页后变为 true）
        bool m_navUnlocked = false;

        // 用户通过“下一步”实际到达过的最远步骤索引（解锁前单调递增）
        int m_farthestReachedIndex = 0;

        // 工况是否已创建（创建后关闭不再提示）
        bool m_mxCaseCreated = false;

// 控件默认状态快照（关闭后复用窗口实例时用于恢复）
        QMap<QLineEdit*, QString> m_defaultEditTexts;
        QMap<QCheckBox*, bool> m_defaultChecks;
        QMap<QRadioButton*, bool> m_defaultRadioChecks;
        QMap<QComboBox*, int> m_defaultComboIndex;
        QMap<QTableWidget*, int> m_defaultRowCount;



    };
}
