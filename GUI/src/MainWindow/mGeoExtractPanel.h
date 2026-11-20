#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mGeoPanelBase.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
//#include <mGeoCommandFactory.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
//#include "mGeoCommandSurfaceOfMidSurface.h" //【OCC摘除·S10/P0-1】退役头（mMidFace 死成员，零使用）

using namespace mxr;
namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGeoExtractPanel : public mGeoPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mGeoExtractPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoExtractPanel();

		/*
		* 显示特征界面
		*/
		void showFold(GeoOperationType type);

		/*
		* 进入面板时连接拾取、单位制等全局信号（mInfoWindow::createGeoExtract 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;

	private:

        /*
        * 撤消接入：异步中面提取的前态快照令牌（提交时 begin，
        * slotMidSurfaceThreadFinished 终态时按成败收口；null = 无进行中任务）。
        */
        void* m_midFaceHistoryToken = nullptr;

		/*
		* 初始化抽中面界面
		*/
		void initMidFacePanel();

        /*
        * 初始化抽中面界面
        */
        void initMidLinePanel();

        /*
        * 初始化抽中面界面
        */
        void initBoundaryFacePanel();

        /*
        * 初始化流体域界面
        */
        void initFluidPanel();

		/*
		* 捕获窗口激活
		*/
		void slot_subWindowActivated();

		/*
		* 高亮几何数据变化
		*/
		void slot_highlightGeoData();

        
		
		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;

		//视口切换信号的连接（connectGeoSignals 建立）
		QMetaObject::Connection conn_viewportChanged;

		//流体域界面控件
        QTableWidget* pickObjTable_2;
        //表格右键菜单
        QAction* subDelAction = nullptr;



		//中面界面控件
        QPushButton* extractMidFaceeOkBtn;
		QComboBox* midFaceFuncComb;
		QComboBox* offsetDisComb;
		MxFunctions::mCalcLineEdit* midFaceMeshSize;
		MxFunctions::mCalcLineEdit* midFaceOffsetDis;
		MxFunctions::mCalcLineEdit* midFaceAngle;
		MxFunctions::mCalcLineEdit* midFaceFillMax;
        QLabel* midDisUnit;
        QLabel* midAngleUnit;
        QLabel* midLimitValUnit;


		QCheckBox* midFaceSecBar;
		//MGeoCommand::mGeoCommandSurfaceOfMidSurface* mMidFace; //【OCC摘除·S10/P0-1】死成员（零使用，类型随退役头下线）
private slots:
	void slot_onCancel();
	void slot_onMidFaceActive();
	/*************************************************
		函数名称：slotMidSurfaceThreadFinished
		函数功能：中面线程完成之后
		输入参数：无
		输出参数：无
		*************************************************/
	void slotMidSurfaceThreadFinished();

    void slot_onFluidActive();
    void slot_onMidLineActive();
    void slot_onBoundaryFaceActive();
    void slot_contextMenuTableData(const QPoint& pos);

	};
}