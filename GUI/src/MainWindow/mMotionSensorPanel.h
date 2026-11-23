#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionSensorBase.h"

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
#include <QStackedLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>

#include "mPanelBase.h"
#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"

#include <string>

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
	class mMotionSensorBase;
}
namespace MechMainWindow
{

	//传感器面板
	class MMAINWINDOW_EXPORT mMotionSensorPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionSensorPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionSensorPanel();

		/*
		* 新建传感器
		*/
		void newMotionSensorPanel(MechMotionData::SensorType type);

		/*
		* 编辑传感器(从树上点击过来的)
		*/
		void editMotionSensorPanel(int id = 0);

		/*
		* 取消编辑传感器
		*/
		void uneditMotionSensorPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionSensorDataAboutToBeDestroyed(int id = -1);


	private:

		void refreshBodyList();
		void refreshJointList();
		void refreshDriveList();

		void updateUIFromData();

		//处理类型切换并自动更新名称
		void handleTypeChange(MechMotionData::SensorType newType);

		//体传感器变量下拉按类型填充（位移类/速度类/加速度类各含线量与角量）
		void populateBodyVariableCombo(MechMotionData::SensorType type);

		//按当前传感器定义刷新运动体高亮（红/蓝槽位，参照运动副面板）
		void refreshBodyHighlight();

		//把指定运动体渲染到高亮槽位（key 为槽位名，如 "red"/"blue"；体无效时清除槽位）
		void highlightBodyById(int bodyId, const std::string& key, int r, int g, int b);

		//按当前传感器定义刷新视口标记点（参考点/距离点A/点B）
		void refreshPointRender();

		//世界坐标 <-> 体局部坐标（局部系原点=质心、与世界系平行，大地bodyId<0原样返回）
		QVector3D worldToBodyLocal(int bodyId, const QVector3D& world) const;
		QVector3D bodyLocalToWorld(int bodyId, const QVector3D& local) const;

		//清理本面板的视口渲染（标记点+高亮槽位），不动编辑状态
		void clearViewportRender();

		//传感器类型对应的中文名称
		QString getTypeName(MechMotionData::SensorType type) const;

		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

		//根据传感器类型刷新界面可用性
		void refreshUIBySensorType();

	private slots:
		//切换视口
		void slot_subWindowActivated(QMdiSubWindow* subWindow);

		//拾取信号
		void slot_finishedPick();

	protected:

		//真正的传感器数据
		MechMotionData::mMotionSensorBase* m_sensorData;

		//总布局
		QVBoxLayout* m_mainLayout;

		// ==================== 界面控件 ====================
		// 1. 基础信息层
		QComboBox* m_cmbSensorType;     ///< 传感器类型

		// 2. 通用参数层
		QComboBox* m_cmbCoordSystem;    ///< 坐标系
		QComboBox* m_cmbComponentMode;  ///< 分量模式

		// 3. 类型特定参数层（使用堆叠控件切换）
		QStackedLayout* m_stkSpecific;

		// --- 体状态传感器页面 (Index 0) ---
		QWidget* m_pageBody;
		QComboBox* m_cmbTargetBody;     ///< 目标体
		mPointPickerWidget* m_refPointPicker = nullptr; ///< 参考点通用拾取控件（世界坐标显示，入库转体局部）
		QComboBox* m_cmbBodyVariable;   ///< 变量

		// --- 关节反力传感器页面 (Index 1) ---
		QWidget* m_pageJoint;
		QComboBox* m_cmbTargetJoint;    ///< 目标关节
		QComboBox* m_cmbJointVariable;  ///< 变量

		// --- 驱动传感器页面 (Index 2) ---
		QWidget* m_pageDrive;
		QComboBox* m_cmbTargetDrive;    ///< 目标驱动
		QComboBox* m_cmbDriveVariable;  ///< 变量

		// --- 距离传感器页面 (Index 3) ---
		QWidget* m_pageDistance;
		QComboBox* m_cmbPointABody;     ///< 点A 所属体
		mPointPickerWidget* m_pointAPicker = nullptr;   ///< 点A 通用拾取控件（世界坐标显示，入库转体局部）
		QComboBox* m_cmbPointBBody;     ///< 点B 所属体
		mPointPickerWidget* m_pointBPicker = nullptr;   ///< 点B 通用拾取控件（世界坐标显示，入库转体局部）
		QComboBox* m_cmbDistanceVariable; ///< 变量

		// --- 角度传感器页面 (Index 4) ---
		QWidget* m_pageAngle;
		QComboBox* m_cmbAxisBody;       ///< 轴参考体
		mDirectionSelectorWidget* m_axisDirSelector = nullptr; ///< 轴方向选择控件
		QComboBox* m_cmbBodyA;          ///< 体A
		QComboBox* m_cmbBodyB;          ///< 体B

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;
	};

}
