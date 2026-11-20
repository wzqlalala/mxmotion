#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mDirectionSelectorWidget.h"

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

#include "mPanelBase.h"
#include "mCalcLineEdit.h"

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
    class mMotionDriveBase;
    class mMotionScenario;
}
namespace MechMainWindow
{

	//方案面板
	class MMAINWINDOW_EXPORT mMotionScenarioPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionScenarioPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionScenarioPanel();

		/*
		* 新建方案
		*/
		void newMotionScenarioPanel();

		/*
		* 编辑方案(从树上点击过来的)
		*/
		void editMotionScenarioPanel(int id = 0);

		/*
		* 取消编辑方案
		*/
		void uneditMotionScenarioPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionScenarioDataAboutToBeDestroyed(int id = -1);


        /*
        * 求解
        */
        void onStartSolve(int scenarioId);

	private:

        void updateUIFromData();

		// 求解完成后把结果落盘成文件（平台上报 resultFiles 用）。
		// 返回结果文件绝对路径，失败返回空串。
		QString exportMotionResultToFile(int scenarioId);

		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

	private slots:
		void slot_unitSystemResponse(int);
		//切换视口	
		void slot_subWindowActivated(QMdiSubWindow* subWindow);

		//拾取信号
		void slot_finishedPick();
	protected:

		//真正的方案数据
        MechMotionData::mMotionScenario* m_motionScenarioData;

		//总布局
		QVBoxLayout* m_mainLayout;

        // ==================== 界面控件 ====================

        // 2. 时间控制
        MxFunctions::mCalcLineEdit* m_spbEndTime; ///< 仿真总时间
        MxFunctions::mCalcLineEdit* m_spbStepSize; ///< 积分步长
        QLabel* m_lblTotalSteps;         ///< 动态显示总步数 (纯UI展示，不存入数据)

        // 3. 物理环境 (重力)
        mDirectionSelectorWidget* m_gravityDirSelector;
		QLabel* m_lblGravityUnit = nullptr;      ///< 当前重力单位提示（随全局单位制刷新）

		void refreshGravityUnitLabel();
		/// 将数据中的重力矢量拆为方向+大小反显到方向控件（会话模式保留）
		void refreshGravityWidget();

		
       // ==================== 求解模式设置 ====================
        QCheckBox* m_chkUsePreset;       ///< 使用预设开关
        QLabel* m_lblPresetMode;         ///< 预设模式标签
        QComboBox* m_cmbPresetMode;      ///< 预设模式下拉框 (FAST/CONTACT/ROBUST/HIGH_PRECISION)

        // ==================== 积分器设置 ====================
        QComboBox* m_cmbIntegratorAdv;   ///< 积分方法下拉框 (高级)
        QLabel* m_lblAlpha;
        QLabel* m_lblBeta;
        MxFunctions::mCalcLineEdit* m_spbAlpha; ///< HHT Alpha
        MxFunctions::mCalcLineEdit* m_spbBeta; ///< HHT Beta
        QSpinBox* m_spbIntegMaxIters;    ///< 最大迭代次数
        MxFunctions::mCalcLineEdit* m_spbIntegTol; ///< 收敛容差

        // ==================== 求解器设置 ====================
        QComboBox* m_cmbSolverType;      ///< 求解器类型下拉框
        QSpinBox* m_spbSolverMaxIters;   ///< 最大迭代次数
        MxFunctions::mCalcLineEdit* m_spbSolverTol; ///< 收敛容差
        QCheckBox* m_chkSolverVerbose;   ///< 输出日志

        QPushButton* m_btnSolve;

        //拾取高亮数据改变的连接
        QMetaObject::Connection conn_geo;
    };
}
