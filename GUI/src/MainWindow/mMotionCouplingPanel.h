#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPhysicalFieldEnum.h"
#include "mPanelBase.h"

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
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
	class mMotionCouplingBase;
	class mMotionCouplingGearRack;
	class mMotionCouplingGear;
	class mMotionCouplingCable;
}
namespace MechMainWindow
{

	//耦合副面板
	class MMAINWINDOW_EXPORT mMotionCouplingPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionCouplingPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionCouplingPanel();

		/*
		* 新建耦合副
		*/
		void newMotionCouplingPanel(MxFunctions::CouplingType type);

		/*
		* 编辑耦合副(从树上点击过来的)
		*/
		void editMotionCouplingPanel(int id = 0);

		/*
		* 取消编辑耦合副
		*/
		void uneditMotionCouplingPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionCouplingDataAboutToBeDestroyed(int id = -1);


	private:

		void refreshJointList();

		void updateUIFromData();

		//处理耦合类型切换并自动更新名称
		void handleTypeChange(MxFunctions::CouplingType newType);

		//耦合副类型对应的中文名称
		QString getTypeName(MxFunctions::CouplingType type) const;

		//根据耦合类型刷新参数折叠框内容（通过 setParent(nullptr) / addRow 动态增删）
		void refreshParamsByCouplingType();

		//在三维视口中高亮显示当前选中的两个运动副
		void triggerHighlight();

		//清除三维视口中的高亮符号
		void clearHighlight();

		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

	private slots:
		//切换视口
		void slot_subWindowActivated(QMdiSubWindow* subWindow);

		//拾取信号
		void slot_finishedPick();

	protected:

		//真正的耦合副数据
		MechMotionData::mMotionCouplingBase* m_motionCouplingData;

		// 上次高亮的两个运动副 ID，用于清除旧符号
		int m_lastFirstJointId = -1;
		int m_lastSecondJointId = -1;

		//总布局
		QVBoxLayout* m_mainLayout;

		// ==================== 界面控件 ====================
		// 1. 基础信息层
		QLabel* m_lblCouplingType;      ///< 耦合副类型标签

		// 2. 运动副 1 折叠框
		FoldSection* m_foldFirstJoint;
		QComboBox* m_cmbFirstJoint;     ///< 第一个运动副（齿轮齿条副中为齿轮旋转副）

		// 3. 运动副 2 折叠框
		FoldSection* m_foldSecondJoint;
		QComboBox* m_cmbSecondJoint;    ///< 第二个运动副（齿轮齿条副中为齿条移动副）

		// 4. 参数折叠框（通过动态 addRow / setParent(nullptr) 切换不同耦合类型的参数）
		FoldSection* m_foldParams;

		// --- 齿轮齿条副参数 ---
		QLabel* m_lblGearRadius;
		QDoubleSpinBox* m_spbGearRadius;  ///< 齿轮半径（销半径）

		// --- 齿轮副参数 ---
		QLabel* m_lblGearParamMode;       ///< 齿轮副参数模式标签
		QComboBox* m_cmbGearParamMode;    ///< 传动比 / 齿数 / 分度圆半径

		QLabel* m_lblGearRatio;
		QDoubleSpinBox* m_spbGearRatio;   ///< 传动比

		QLabel* m_lblToothCount1;
		QSpinBox* m_spbToothCount1;       ///< 第一个齿轮齿数

		QLabel* m_lblToothCount2;
		QSpinBox* m_spbToothCount2;       ///< 第二个齿轮齿数

		QLabel* m_lblPitchRadius1;
		QDoubleSpinBox* m_spbPitchRadius1; ///< 第一个齿轮分度圆半径

		QLabel* m_lblPitchRadius2;
		QDoubleSpinBox* m_spbPitchRadius2; ///< 第二个齿轮分度圆半径

		// --- 线缆副参数 ---
		QLabel* m_lblCableRatio;
		QDoubleSpinBox* m_spbCableRatio;   ///< 位移比（第二侧滑动副行程 / 第一侧行程）
		QLabel* m_lblCableReverse;
		QCheckBox* m_chkCableReverse;      ///< 反向（约束方程中额外乘以 -1）

		// 后续扩展：皮带轮副等参数控件在此追加，
		// 统一通过 refreshParamsByCouplingType() 进行 addRow / setParent(nullptr) 管理。

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;
	};

}
