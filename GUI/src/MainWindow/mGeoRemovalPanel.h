#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mGeoPanelBase.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"

using namespace mxr;
namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGeoRemovalPanel : public mGeoPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mGeoRemovalPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoRemovalPanel();

		/*
		* 显示特征界面
		*/
		void showFold(GeoOperationType type);

		/*
		* 进入面板时连接拾取、单位制等全局信号（mInfoWindow::createGeoRemoval 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;

	private:

		/*
		* 捕获窗口激活
		*/
		void initRemovePanel();

		bool autoRemoveSearchAction();

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

		//自动去除界面控件
		QCheckBox* autoRemoveHole1;
		QCheckBox* autoRemoveHole2;
		QCheckBox* autoRemoveSolidHole;
		QCheckBox* autoRemoveFillet;
		QCheckBox* autoRemoveChamfer;
		//QCheckBox* autoRemoveThread;
		QCheckBox* autoRemoveLogo;   //【集成回补】dev 已完整实现的 logo 识别功能（src 侧"未开发"退役判定过时）
		MxFunctions::mCalcLineEdit* maxHoleRadius;
		MxFunctions::mCalcLineEdit* maxHoleArea;
		MxFunctions::mCalcLineEdit* maxSolidHoleRadius;
		MxFunctions::mCalcLineEdit* maxFilletRadius;
		MxFunctions::mCalcLineEdit* maxChamferDis;
		//MxFunctions::mCalcLineEdit* maxMinorDiameter;
		MxFunctions::mCalcLineEdit* maxLogoHeight;

        QLabel* maxHoleRadiusUnit;
        QLabel* maxSolidHoleRadiusUnit;
        QLabel* maxFilletRadiusUnit;
        QLabel* maxChamferDisUnit;
        //QLabel* maxMinorDiameterUnit;



		//自动识别的边或面的id
		mxr::PickFilter autoRemovalPickFilter;
		std::set<int> autoCollectIds;
		//仅存实体孔面
		std::set<int> solidHoleFaceIds;

private slots:
	void slot_onCancel();

	void slot_autoRemoveAction();

	void slot_manualRemoveAction();
	};
}