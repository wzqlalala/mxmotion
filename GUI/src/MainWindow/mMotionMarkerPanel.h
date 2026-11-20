#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mPointPickerWidget.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QMdiSubWindow>

#include "mPanelBase.h"

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
	class mMotionMarker;
}
namespace MechMainWindow
{
	//标记点面板
	class MMAINWINDOW_EXPORT mMotionMarkerPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionMarkerPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionMarkerPanel();

		/*
		* 新建标记点
		*/
		void newMotionMarkerPanel();

		/*
		* 编辑标记点(从树上点击过来的)
		*/
		void editMotionMarkerPanel(int motionMarkerID = 0);

		/*
		* 取消编辑标记点
		*/
		void uneditMotionMarkerPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionMarkerDataAboutToBeDestroyed(int id = -1);


	private:

		void refreshBodyComboBox();

		void updateUIFromData();

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

		//真正的标记点数据
		MechMotionData::mMotionMarker* m_motionMarkerData;

		//总布局
		QVBoxLayout* m_mainLayout;

		// ==================== 界面控件 ====================
		// 1. 附着运动体
		QComboBox* m_cmbBody;

		// 2. 局部坐标（相对于运动体）
		mPointPickerWidget* m_posPicker;

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;
	};

}
