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
class IncentiveNodeTableWidget;
QT_END_NAMESPACE

class NavIconDelegate;

namespace MechModelData
{
	class mModelData;
}

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mCaseImplicitStiffnessWindow : public QDialog
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
        mCaseImplicitStiffnessWindow(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mCaseImplicitStiffnessWindow();

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
		QWidget* createPageDamping();

        /*
        * 构建第四个页面：约束设置
        */
        QWidget* createPageConstraint();

		/*
		* 构建第五个页面：激励点设置
		*/
		QWidget* createPageIncentivePoint();

        /*
        * 构建第六个页面：输出设置
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
        bool isConstraintValid() const;
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
        void connectTableCheckBoxes(IncentiveNodeTableWidget* table, int pageIndex);

        /*
        * 删除最后一行模态阻尼
        */
        void RemoveModalDampingRow();

        /*
        * 删除最后一行激励点
        */
        void RemoveIncentiveRow();

        /*
        * 更新删除按钮状态
        */
        void updateConstraintDeleteButtonState();
        void updateIncentiveDeleteButtonState();
        void updateModalDampingDeleteButtonState();

        /*
        * 创建频响分析
        */
        void createFrequencyResponseAnalysis(const QString& name, int forceLoadID, int i);
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
        * 添加一行激励点
        */
        void slot_onAddIncentiveRow();

        /*
        * 添加一行全局模态阻尼
        */
        void slot_onAddModalDampingRow();

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
        * 根据节点个数输入框调整激励点行数
        */
        void slot_onIncentivePointCountChanged();

        /*
        * 根据节点个数输入框调整模态阻尼行数
        */
        void slot_onModalDampingPointCountChanged();

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
        QWidget* m_analysisSettingsContainer = nullptr;
        QRadioButton* m_direct = nullptr;
        QRadioButton* m_modal = nullptr;
        QComboBox* m_comboBox_Solver = nullptr;
        QCheckBox* m_checkBox_ModeScale = nullptr;
        QLineEdit* m_lineEdit_FreqMin = nullptr;
        QLineEdit* m_lineEdit_FreqMax = nullptr;
        QLineEdit* m_lineEdit_ModeCount = nullptr;
        QLineEdit* m_lineEdit_ModeScale = nullptr;
        QFormLayout* m_rangeFormLayout = nullptr;

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

        // 第四页：约束设置
        QLineEdit* m_constraint_PointCount = nullptr;
        QPushButton* m_constraint_AddRow = nullptr;
        QPushButton* m_constraint_RemoveRow = nullptr;
        QPushButton* m_constraint_ImportFile = nullptr;
        QPushButton* m_constraint_ExportFile = nullptr;
        IncentiveNodeTableWidget* m_constraint_NodeTable = nullptr;

		// 第五页：激励点设置
        QLineEdit* m_Incentive_PointCount = nullptr;
        QPushButton* m_Incentive_AddRow = nullptr;
        QPushButton* m_Incentive_RemoveRow = nullptr;
        QPushButton* m_Incentive_ImportFile = nullptr;
        QPushButton* m_Incentive_ExportFile = nullptr;
        IncentiveNodeTableWidget* m_Incentive_NodeTable = nullptr;

		// 第六页：输出设置
		QCheckBox* m_checkBox_OutputDisplacement = nullptr;
        QCheckBox* m_checkBox_OutputSpeed = nullptr;
        QCheckBox* m_checkBox_Outputaccelerated = nullptr;
        QPushButton* m_outputCurveButton = nullptr;
        QWidget* m_outputCurveContentWidget = nullptr;

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
