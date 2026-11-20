#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mNVHWindowComponent.h"

#include <QDialog>
#include <QStringList>
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
    class MMAINWINDOW_EXPORT mCaseImplicitRandomVibrationWindow : public QDialog
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mCaseImplicitRandomVibrationWindow(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mCaseImplicitRandomVibrationWindow();

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
        * 构建第六个页面：工况设置
        */
        QWidget* createPageCaseSetting();

        /*
        * 构建第七个页面：响应设置
        */
        QWidget* createPageResponse();

        /*
        * 构建第八个页面：输出设置
        */
        QWidget* createPageOutput();

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
        bool isCaseSettingValid() const;
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

        /*
        * 将工况组合名称列表同步到工况设置表第4列的所有下拉框
        */
        void syncCaseCombinationCombos();

        /*
        * 根据频响工况设置表更新工况标签列表：
        * 选“独立工况”的行显示“节点ID+自由度”，其余行显示所选工况组合名
        */
        void updateCaseLabelTable();

        /*
        * 根据工况标签列表行数刷新随机功率谱矩阵：n行n列，列宽固定48px，
        * 下三角置灰，对角线及上三角不可编辑但可选择
        */
        void updateSpectrumMatrixTable();

        /*
        * 功率谱矩阵 item 数据存取：
        * 判断单元格是否可设置（对角线或上三角）、保存/加载实部与虚部设置
        */
        bool isValidSpectrumCell(int row, int col) const;
        void saveSpectrumCellData();
        void loadSpectrumCellData(int row, int col);

        // 在 PSD 曲线下拉框中注册并选中曲线名
        void registerPsdCurveName(QComboBox* combo, const QString& name);

        // 删除下拉框中第 row 行的 PSD 曲线：从曲线库与两个下拉框中移除，并复位矩阵中的引用
        void removePsdCurve(QComboBox* combo, int row);

        // 打开 PSD 曲线创建/编辑对话框（combo 为发起侧的曲线下拉框）
        void openPsdCurveCreateDialog(QComboBox* combo);
        void openPsdCurveEditDialog(QComboBox* combo);

        /*
        * “应用”按钮状态：设置完整且与已应用值不同时可点击，否则置灰
        */
        void updateCaseApplyButtonState();

        /*
        * 将当前设置应用到当前矩阵 item：快照已应用值，格子变绿，按钮置灰
        */
        void applySpectrumCellSettings();

        /*
        * 刷新功率谱矩阵格子底色：曲线设置完整的格子（含上三角）为绿色，否则白色
        */
        void refreshSpectrumCellColors();

        // 连接表格中所有复选框状态变化到指定修改回调
        void connectTableCheckBoxes(IncentiveNodeTableWidget* table, int pageIndex);
        /*
        * 更新删除按钮状态
        */
        void updateConstraintDeleteButtonState();
        void updateIncentiveDeleteButtonState();
        void updateModalDampingDeleteButtonState();
        void updateFluidModalDampingDeleteButtonState();

        /*
        * 创建声固耦合分析
        */
        void createASCouplingAnalysis(const QString& name, const QList<int>& forceLoadIDs, int i, int loadCount);
        /*
        * 创建频响分析
        */
        void createFrequencyResponseAnalysis(const QString& name, const QList<int>& forceLoadIDs, int i, int loadCount);
        /*
        * 创建频响分析
        */
        void createRandomVibrationAnalysis();
        /*
        * 创建采样频率点
        */
        int createFrequencyPoint(const QString& name);

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
        * 添加一行激励设置
        */
        void slot_onAddIncentiveRow();

        /*
        * 添加一行全局模态阻尼
        */
        void slot_onAddModalDampingRow();

        /*
        * 添加一行流体全局模态阻尼
        */
        void slot_onAddFluidModalDampingRow();

        /*
        * 从文件导入激励点
        */
        void slot_ImportIncentiveFromFile();

        /*
        * 导出激励点文件（*.txt / *.csv）
        */
        void slot_onExportIncentiveFile();

        /*
        * 根据节点个数输入框调整约束行数
        */
        void slot_onConstraintPointCountChanged();

        /*
        * 根据节点个数输入框调整激励设置行数
        */
        void slot_onIncentivePointCountChanged();

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

        void slot_onSolverChanged();

        //工况设置中的频响工况设置表
        void slot_onIncentiveSettingTableChanged();

        //工况组合编辑按钮
        void slot_editCaseCombination();

        /*
        * 清除按钮：当前格子的曲线恢复为“无”，底色变白
        */
        void slot_onCaseClear();
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
        QComboBox* m_comboBox_Solver = nullptr;
        QCheckBox* m_checkBox_ModeScale = nullptr;
        QLineEdit* m_lineEdit_ModeScale = nullptr;
        QLineEdit* m_lineEdit_FreqMin = nullptr;
        QLineEdit* m_lineEdit_FreqMax = nullptr;
        QLineEdit* m_lineEdit_ModeCount = nullptr;
        QLineEdit* m_lineEdit_FluidFreqMin = nullptr;
        QLineEdit* m_lineEdit_FluidFreqMax = nullptr;
        QLineEdit* m_lineEdit_FluidModeCount = nullptr;
        QWidget* ratioWidget = nullptr;
        QWidget* m_ASCoupling_Widget = nullptr;
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
        QLineEdit* m_Incentive_PointCount = nullptr;
        QComboBox* m_LoadType = nullptr;
        QPushButton* m_Incentive_AddRow = nullptr;
        QPushButton* m_Incentive_RemoveRow = nullptr;
        QPushButton* m_Incentive_ImportFile = nullptr;
        QPushButton* m_Incentive_ExportFile = nullptr;
        IncentiveNodeTableWidget* m_Incentive_NodeTable = nullptr;

        // 第六页：工况设置
        QTableWidget* m_FrequencyResponse_Table = nullptr;
        QTableWidget* m_CaseLabel_Table = nullptr;
        QTableWidget* m_SpectrumMatrix_Table = nullptr;
        // PSD 曲线库：曲线名 -> 曲线数据
        QMap<QString, IncentiveCurve> m_psdCurveMap;
        // 工况组合名称列表（首项“独立工况”为固定项，不可删除）
        QStringList m_caseCombinationList = { QStringLiteral("独立工况") };
        QWidget* m_RealWidgetContent = nullptr;
        QLineEdit* m_RealPartFactor = nullptr;
        QComboBox* m_RealPSDCurve = nullptr;
        QPushButton* m_RealCurveEdit = nullptr;
        QPushButton* m_RealCurveCreate = nullptr;
        QWidget* m_ImaginaryWidgetContent = nullptr;
        QLineEdit* m_ImaginaryPartFactor = nullptr;
        QComboBox* m_ImaginaryPSDCurve = nullptr;
        QPushButton* m_ImaginaryCurveEdit = nullptr;
        QPushButton* m_ImaginaryCurveCreate = nullptr;
        QPushButton* m_CaseClear = nullptr;
        QPushButton* m_CaseApply = nullptr;

        // 第七页：响应设置
        ResponseSettingFoldablePanel* m_SoundPressureResponse = nullptr;
        ResponseSettingFoldablePanel* m_VibrationResponse = nullptr;
        
        // 第八页：输出设置
         // 响应点曲线结果输出
        QPushButton* m_outputCurveButton = nullptr;
        QWidget* m_outputCurveContentWidget = nullptr;
        QCheckBox* m_OutputDisplacement = nullptr;
        QCheckBox* m_OutputSpeed = nullptr;
        QCheckBox* m_Outputaccelerated = nullptr;
        QCheckBox* m_OutputSoundPressure = nullptr;
        // 全局云图结果输出
        QPushButton* m_outputCloudButton = nullptr;
        QWidget* m_outputCloudContentWidget = nullptr;
        QComboBox* m_comboBox_outputForm = nullptr;
        QCheckBox* m_PhysicalDisplacement = nullptr;
        QCheckBox* m_PhysicalSpeed = nullptr;
        QCheckBox* m_Physicalaccelerated = nullptr;
        QCheckBox* m_PhysicalStress = nullptr;

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
