#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPhysicalFieldEnum.h"

//QT
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
	class mMotionControllerBase;
	class mMotionPIDController;
}
namespace MechMainWindow
{

	//控制器面板（Phase 1: PID）
	class MMAINWINDOW_EXPORT mMotionControllerPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionControllerPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionControllerPanel();

		/*
		* 新建控制器
		*/
		void newMotionControllerPanel();

		/*
		* 编辑控制器(从树上点击过来的)
		*/
		void editMotionControllerPanel(int id = 0);

		/*
		* 取消编辑控制器
		*/
		void uneditMotionControllerPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionControllerDataAboutToBeDestroyed(int id = -1);


	private:

		//刷新目标驱动下拉（仅速度控制模式的关节驱动）
		void refreshDriveList();

		//按当前数据刷新界面
		void updateUIFromData();

	private slots:
		void slot_targetDriveChanged(int index);
		void slot_feedbackTypeChanged(int index);
		void slot_targetSourceChanged(int index);

	private:
		//刷新目标控制输入下拉
		void refreshInputList();

		//刷新反馈传感器下拉（启用的运动学类传感器）
		void refreshSensorList();

	private:

		//真正的控制器数据
		MechMotionData::mMotionPIDController* m_controllerData;

		//总布局
		QVBoxLayout* m_mainLayout;

		// ==================== 界面控件 ====================
		QComboBox* m_cmbTargetDrive;     ///< 目标驱动（速度模式）
		QComboBox* m_cmbFeedbackType;    ///< 反馈量（位移/速度）
		QComboBox* m_cmbTargetSource;    ///< 目标来源（常数/控制输入）
		QComboBox* m_cmbTargetInput;     ///< 目标控制输入

		QComboBox* m_cmbFeedbackSource;  ///< 反馈源（驱动自由度/传感器）
		QComboBox* m_cmbFeedbackSensor;  ///< 反馈传感器
		QCheckBox* m_chkFeedbackSign;    ///< 反馈取反

		MxFunctions::mCalcLineEdit* m_edtTargetValue; ///< 目标值

		MxFunctions::mCalcLineEdit* m_edtKp;          ///< 比例系数
		MxFunctions::mCalcLineEdit* m_edtKi;          ///< 积分系数
		MxFunctions::mCalcLineEdit* m_edtKd;          ///< 微分系数
		MxFunctions::mCalcLineEdit* m_edtOutputLimit; ///< 输出限幅
	};

}
