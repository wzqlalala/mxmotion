#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QVBoxLayout>
#include <QHideEvent>
#include <QWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QComboBox>
#include <qvector3d.h>

#include "mGeoPanelBase.h"

#include "mGeoFeatureTypes.h"
#include "mGeoPreviewTypes.h"


namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGeoFeaturePanel : public mGeoPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mGeoFeaturePanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoFeaturePanel();

		/*
		* 显示特征界面
		*/
		void showFeatureFold(GeoOperationType type);

		/*
		* 进入面板时连接拾取、单位制等全局信号（mInfoWindow::createGeoFeature 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;

	protected:
		/*
		* 面板隐藏时复位拾取状态并清理点击点标记
		*/
		void hideEvent(QHideEvent* event) override;



	private slots:
		/*
		* 取消按钮
		*/
		void slot_cancelBtn();

		/*
		* 创建拉伸特征
		*/
		void slot_createStretchBtn();

		/*
		* 创建旋转特征
		*/
		void slot_createRotationBtn();

		/*
		* 创建扫掠特征
		*/
		void slot_createSweepBtn();

		/*
		* 创建放样特征
		*/
		void slot_createLoftBtn();
		/*
		* 创建抽壳特征
		*/
		void slot_createHollowingBtn();
		/*
		* 创建拔模特征
		*/
		void slot_createDraftBtn();

		/*
		* 创建倒角特征
		*/
		void slot_createChamferBtn();

		/*
		* 创建孔特征
		*/
		void slot_createHoleBtn();

		/*
		* 拉伸预览：任何拉伸参数编辑后触发
		*/
		void slot_stretchPreview();

		/*
		* 旋转预览
		*/
		void slot_rotationPreview();

		/*
		* 扫掠预览
		*/
		void slot_sweepPreview();

		/*
		* 放样预览
		*/
		void slot_loftPreview();

		/*
		* 抽壳预览
		*/
		void slot_hollowingPreview();

		/*
		* 拔模预览
		*/
		void slot_draftPreview();

		/*
		* 倒角/圆角预览
		*/
		void slot_chamferPreview();

	private:

		/*
		* 初始化拉伸操作界面
		*/
		void initStretchFold();

		/*
		* 初始化旋转操作界面
		*/
		void initRotationFold();

		/*
		* 初始化扫掠操作界面
		*/
		void initSweepFold();

		/*
		* 初始化放样操作界面
		*/
		void initLoftFold();

		/*
		* 初始化抽壳操作界面
		*/
		void initShellRmFold();

		/*
		* 初始化拔模操作界面
		*/
		void initWDPRmFold();

		/*
		* 初始化倒角操作界面
		*/
		void initChamferFold();

		/*
		* 初始化孔操作界面
		*/
		void initHoleFold();

		/*
		* 连接/断开拉伸预览信号；conn 为 true 时所有拉伸参数控件编辑都会触发拉伸预览
		*/
		void connectPreView(bool conn);

		/*
		* 连接/断开指定特征的预览信号；conn 为 true 时该特征参数控件编辑都会触发对应预览
		*/
		void connectPreView(bool conn, GeoOperationType type);

		/*
		* 激活/取消激活直接建模渲染工具（以几何面板编辑/取消编辑为界）
		*/
		void activateDirectGeoModelingTool(bool active);

		/*
		* 清除特征预览渲染
		*/
		void clearStretchPreviewRender();

		/*
		* 当前特征的预览勾选框是否勾选
		*/
		bool isPreviewChecked(GeoOperationType type) const;

		/*
		* 把预览离散化结果渲染到视口（颜色取被操作部件内色，线/点同色系加深；
		* 增量操作面 30% 透明，减量操作面不透明）
		*/
		void renderPreviewResult(const EventDispatcher::mGeoPreviewResult& res, bool reducePreview = false);

		/*
		* 减量操作（倒角/圆角/抽壳等）预览时把原部件透明，预览结束时恢复
		*/
		void setSourcePartTransparent(bool transparent, int partID);

		/*
		* 捕获窗口激活
		*/
		void slot_subWindowActivated();

		/*
		* 高亮几何数据变化
		*/
		void slot_highlightGeoData();

		/*
		* 拾取表格右键菜单
		*/
		void slot_contextMenuTableData(const QPoint& pos);

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;

		//视口切换信号的连接（connectGeoSignals 建立）
		QMetaObject::Connection conn_viewportChanged;

		//减量预览时被透明化的部件及其原透明度（65535 刻度）
		int m_transparentPartID = -1;
		int m_transparentPartOriginAlpha = -1;

		//拉伸界面控件
		int stretchDirId;
		QLabel* stretchUnitsLabel;
		QCheckBox* stretchPreviewChkBox;
		QCheckBox* stretchMergeChkBox;
		QPushButton* stretchCancelBtn;
		QPushButton* stretchOkBtn;
		MxFunctions::mCalcLineEdit* stretchHeightEdit;
		MxFunctions::mCalcLineEdit* stretchVx, * stretchVy, * stretchVz;

		//旋转界面控件
		int rotationAxisId;
		int rotationPntId;
		QLabel* rotationUnitsLabel;
		QCheckBox* rotationPreviewChkBox;
		QCheckBox* rotationMergeChkBox;
		QPushButton* rotationCancelBtn;
		QPushButton* rotationOkBtn;
		MxFunctions::mCalcLineEdit* rotationAngleEdit;
		MxFunctions::mCalcLineEdit* rotationPntX, * rotationPntY, * rotationPntZ;
		MxFunctions::mCalcLineEdit* rotationVx, * rotationVy, * rotationVz;

		//扫掠界面控件
		QTableWidget* sweepPickLineTable;
		QCheckBox* sweepPreviewChkBox;
		QPushButton* sweepCancelBtn;
		QPushButton* sweepOkBtn;
		QCheckBox* sweepMergeChkBox;
		//表格右键菜单
		QAction* sweepDelAction = nullptr;

		//放样界面控件
		QCheckBox* loftPreviewChkBox;
		QPushButton* loftCancelBtn;
		QPushButton* loftOkBtn;
		QCheckBox* loftTypeChkBox;

		//抽壳界面控件
		QCheckBox* shellRmPreviewChkBox;
		QLabel* shellRmUnitsLabel;
		QPushButton* shellRmCancelBtn;
		QPushButton* shellRmOkBtn;
		MxFunctions::mCalcLineEdit* shellRmThicknessEdit;

		//拔模界面控件
		int wdpFaceId;
		QVector3D wdpDir;
		QLineEdit* wdpPickMidEdit;
		QToolButton* wdpPickMidBtn;
		QToolButton* wdpPickMidReverseBtn;
		QCheckBox* wdpPreviewChkBox;
		QLabel* wdpUnitsLabel;
		QPushButton* wdpCancelBtn;
		QPushButton* wdpOkBtn;
		MxFunctions::mCalcLineEdit* wdpAngleEdit;

		//倒角界面控件
		QComboBox* chamferComboBox;
		QLabel* chamferUnitsLabel;
		QCheckBox* chamferPreviewChkBox;
		QPushButton* chamferCancelBtn;
		QPushButton* chamferOkBtn;
		MxFunctions::mCalcLineEdit* chamferDimEdit;
		MxFunctions::mCalcLineEdit* chamferAngEdit;
        QLabel* chamferAngleUnit;

		//孔界面控件
		QComboBox* holeTypeComboBox;
		QLabel* holeRadiusUnitsLabel;
		QLabel* holeDepthUnitsLabel;
		QPushButton* holeCancelBtn;
		QPushButton* holeOkBtn;
		MxFunctions::mCalcLineEdit* holeRadiusEdit;
		MxFunctions::mCalcLineEdit* holeDepthEdit;


	};

}