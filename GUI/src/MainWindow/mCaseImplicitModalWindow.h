#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QDialog>
#include <QMap>
#include <vector>

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
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;
class QLabel;
QT_END_NAMESPACE

class IncentiveNodeTableWidget;
class NavIconDelegate;

namespace MechModelData
{
	class mModelData;
}

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mCaseImplicitModalWindow : public QDialog
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mCaseImplicitModalWindow(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mCaseImplicitModalWindow();

		/*
		* 恢复窗口默认状态（复用已关闭的窗口实例时调用）
		*/
		void resetToDefault();

		/*
		* 获取工况名称
		*/
		QString getCaseName() const;
        
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
		* 构建第一个页面：分析设置
		*/
		QWidget* createPageAnalysisSetting();

		/*
		* 构建第二个页面：约束设置
		*/
		QWidget* createPageConstraint();

		/*
		* 构建第三个页面：输出设置
		*/
		QWidget* createPageOutput();

		/*
		* 初始化左侧竖向导航栏
		*/
		void initLeftNavigation();

		/*
		* 更新底部按钮状态
		*/
		void updateBottomButtons(int currentIndex);

		/*
		* 更新下一步按钮文本
		*/
		void updateNextButtonText(int currentIndex);

		void refreshNavigationIcons();

		// 检查分析设置是否满足完成条件
		bool isAnalysisSettingValid() const;

		// 检查约束设置是否满足完成条件
		bool isConstraintValid() const;

		// 检查输出设置是否满足完成条件
		bool isOutputValid() const;

		// 更新所有导航项完成状态与输出页按钮状态
		void updateValidationState();

		// 根据特征值求解器类型更新关联控件状态
		void applySolverDependentState();

		// 记录用户已到达的最远步骤索引
		void updateFarthestReachedIndex(int index);

		// 用户修改当前页后，将后续步骤重置为未完成
		void resetSubsequentSteps(int fromIndex);

		// 检查当前页相对于已到达位置是否发生回退修改，若是则重置后续状态
		void onPageModified(int pageIndex);

		// 判断指定导航页是否可交互（已到达/当前页/已完成）
		bool isNavigationIndexEnabled(int index) const;

		// 判断输入框是否为有效浮点数
		bool isValidDouble(QLineEdit* lineEdit) const;

		// 判断输入框是否为有效正整数
		bool isValidPositiveInt(QLineEdit* lineEdit) const;
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
		* 导入约束文件（*.txt / *.csv）
		*/
		void slot_onImportConstraintFile();

		/*
		* 导出约束文件（*.txt / *.csv）
		*/
		void slot_onExportConstraintFile();

		/*
		* 根据节点个数输入框调整约束行数
		*/
		void slot_onPointCountChanged();

		/*
		* 更新删除按钮状态
		*/
		void updateDeleteButtonState();

		// 输出设置页校验并刷新导出/创建按钮状态
		void updateOutputActionButtons();

		// 特征值求解器切换
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

		// 第一页：分析设置（含工况名称、预应力、分析设置）
		QLineEdit* m_lineEdit_CaseName = nullptr;
		QCheckBox* m_checkBox_EnablePreload = nullptr;
		QComboBox* m_comboBox_PreloadCase = nullptr;
		QComboBox* m_comboBox_Solver = nullptr;
		QCheckBox* m_checkBox_ModeScale = nullptr;
		QLineEdit* m_lineEdit_FreqMin = nullptr;
		QLineEdit* m_lineEdit_FreqMax = nullptr;
		QLineEdit* m_lineEdit_ModeCount = nullptr;
		QLineEdit* m_lineEdit_ModeScale = nullptr;
        QFormLayout* m_rangeFormLayout = nullptr;

		// 第二页：约束设置
		QLineEdit* m_lineEdit_PointCount = nullptr;
		QPushButton* m_pushButton_AddRow = nullptr;
		QPushButton* m_pushButton_RemoveRow = nullptr;
		QPushButton* m_pushButton_ImportFile = nullptr;
		QPushButton* m_pushButton_ExportFile = nullptr;
        QWidget* m_constraintListWidget = nullptr;
        IncentiveNodeTableWidget* m_nodetable = nullptr;

		// 第三页：输出设置
		QCheckBox* m_checkBox_OutputStress = nullptr;
		QCheckBox* m_checkBox_OutputStrain = nullptr;
		QCheckBox* m_checkBox_OutputStrainEnergy = nullptr;
		QCheckBox* m_checkBox_OutputDisplacement = nullptr;

		// 响应点云图结果输出折叠面板
		QPushButton* m_outputOp2FoldButton = nullptr;
		QWidget* m_outputOp2ContentWidget = nullptr;
		QComboBox* m_comboBox_OutputFormat = nullptr;

		// 底部按钮
		QPushButton* m_pushButton_Previous = nullptr;
		QPushButton* m_pushButton_Next = nullptr;
		QPushButton* m_pushButton_CreateMxCase = nullptr;
		QPushButton* m_pushButton_ExportFem = nullptr;
		QHBoxLayout* m_bottomLayout = nullptr;

		// 导航是否已解锁（到达输出页后变为 true）
		bool m_navUnlocked = false;

		// 用户通过“下一步”实际到达过的最远步骤索引（解锁前只有 0/1/2 单调递增）
		int m_farthestReachedIndex = 0;

		// 工况是否已创建（创建后关闭不再提示）
		bool m_mxCaseCreated = false;

// 控件默认状态快照（关闭后复用窗口实例时用于恢复）
		QMap<QLineEdit*, QString> m_defaultEditTexts;
		QMap<QCheckBox*, bool> m_defaultChecks;
		QMap<QComboBox*, int> m_defaultComboIndex;
		QMap<QTableWidget*, int> m_defaultRowCount;
	};
}
