#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QVBoxLayout>
#include <QWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QComboBox>
#include <QRadioButton>
#include <qvector3d.h>

#include "mGeoPanelBase.h"

#include "mGeoPreviewTypes.h"


namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGeoTransformPanel : public mGeoPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mGeoTransformPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoTransformPanel();

		/*
		* 显示特征界面
		*/
		void showFold(GeoOperationType type);

		/*
		* 进入面板时连接拾取、单位制等全局信号（mInfoWindow::createGeoTransform 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;

	protected:
		/*
		* 面板隐藏时取消预览
		*/
		void hideEvent(QHideEvent* event) override;

	private slots:
		void slot_onCancel();

		void slot_offsetAction();

		void slot_locationAction();

		void slot_rotateAction();

		void slot_mirrorimageAction();

		void slot_zoomAction();

		void slot_lineArrayAction();

		void slot_ringArrayAction();

		/*
		* 平移预览
		*/
		void slot_offsetPreview();

		/*
		* 旋转预览
		*/
		void slot_rotatePreview();

		/*
		* 镜像预览
		*/
		void slot_mirrorimagePreview();

		/*
		* 缩放预览
		*/
		void slot_zoomPreview();

		/*
		* 线性阵列预览
		*/
		void slot_lineArrayPreview();

		/*
		* 环形阵列预览
		*/
		void slot_ringArrayPreview();

	private:
		/*
		* 初始化平移操作界面
		*/
		void initOffsetFold();

		/*
		* 初始化定位操作界面
		*/
		void initLocationFold();

		/*
		* 初始化旋转操作界面
		*/
		void initRotateFold();

		/*
		* 初始化旋转操作界面
		*/
		void initMirrorimageFold();

		/*
		* 初始化缩放操作界面
		*/
		void initZoomFold();

		/*
		* 初始化线性阵列操作界面
		*/
		void initLineArrayFold();

		/*
		* 初始化环形阵列操作界面
		*/
		void initRingArrayFold();

		/*
		* 连接/断开指定操作的预览信号；conn 为 true 时该操作参数控件编辑都会触发对应预览
		*/
		void connectPreView(bool conn, GeoOperationType type);

		/*
		* 当前操作的预览勾选框是否勾选
		*/
		bool isPreviewChecked(GeoOperationType type) const;

		/*
		* 激活/取消激活直接建模渲染工具
		*/
		void activateDirectGeoModelingTool(bool active);

		/*
		* 清除预览渲染
		*/
		void clearPreviewRender();

		/*
		* 把预览离散化结果渲染到视口（颜色取被操作部件内色，面/线/点一致）
		*/
		void renderPreviewResult(const EventDispatcher::mGeoPreviewResult& res);

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
		//平移界面控件
		int offsetLastDirID;
		int offsetLastStartID;
		int offsetLastEndID;
		QRadioButton* offsetMoveRadio;
		QRadioButton* offsetCopyRadio;
		QLabel* offsetUnitsLabel;
		QCheckBox* offsetMergeChkBox;
		QPushButton* offsetCancelBtn;
		QPushButton* offsetOkBtn;
		MxFunctions::mCalcLineEdit* offsetDisEdit;
		MxFunctions::mCalcLineEdit* offsetVx, * offsetVy, * offsetVz;
		MxFunctions::mCalcLineEdit* offsetEVx, * offsetEVy, * offsetEVz;
		QLabel* offsetSXLabel, * offsetSYLabel, * offsetSZLabel;
		QLabel* offsetEXLabel, * offsetEYLabel, * offsetEZLabel;
		QLabel* offsetExLabel, * offsetDisLabel;
		MxFunctions::mCalcLineEdit* offsetExEdit;
		QCheckBox* offsetCustomDis;
		QComboBox* offsetVec;

		//定位界面控件
		QRadioButton* locationMoveRadio;
		QRadioButton* locationCopyRadio;
		QPushButton* locationCancelBtn;
		QPushButton* locationOkBtn;
		QComboBox* locationRefTypeCombo;
        QLineEdit* coordSystemA;
        QLineEdit* coordSystemB;
		QLineEdit* locationSourcePntA;
		QLineEdit* locationSourcePntB;
		QLineEdit* locationSourcePntC;
		QLineEdit* locationTargetPntA;
		QLineEdit* locationTargetPntB;
		QLineEdit* locationTargetPntC;
        QToolButton* coordSystemPickA;
        QToolButton* coordSystemPickB;
		QToolButton* locationPickSourcePntA;
		QToolButton* locationPickSourcePntB;
		QToolButton* locationPickSourcePntC;
		QToolButton* locationPickTargetPntA;
		QToolButton* locationPickTargetPntB;
		QToolButton* locationPickTargetPntC;
        QLabel* locationSourcePntALabel, * locationSourcePntBLabel, * locationSourcePntCLabel;
        QLabel* locationTargetPntALabel, * locationTargetPntBLabel, * locationTargetPntCLabel;
        QLabel* coordSystemALabel, * coordSystemBLabel;
        int coordSystemAId;
        int coordSystemBId;
		int locationSourcePntAId;
		int locationSourcePntBId;
		int locationSourcePntCId;
		int locationTargetPntAId;
		int locationTargetPntBId;
		int locationTargetPntCId;

		//旋转界面控件
		int rotateLastAxisID;
		int rotateLastPntID;
		QRadioButton* rotateMoveRadio;
		QRadioButton* rotateCopyRadio;
		QLabel* rotateUnitsLabel;
		QPushButton* rotateCancelBtn;
		QPushButton* rotateOkBtn;
		MxFunctions::mCalcLineEdit* rotateAngleEdit;
		MxFunctions::mCalcLineEdit* rotatePntX, * rotatePntY, * rotatePntZ;
		MxFunctions::mCalcLineEdit* rotateVx, * rotateVy, * rotateVz;

		//镜像界面控件
		int mirrorLastAxisID;
		int mirrorLastPntID;
		QRadioButton* mirrorimageMoveRadio;
		QRadioButton* mirrorimageCopyRadio;
		QPushButton* mirrorimageCancelBtn;
		QPushButton* mirrorimageOkBtn;
		MxFunctions::mCalcLineEdit* mirrorimagePntX, * mirrorimagePntY, * mirrorimagePntZ;
		MxFunctions::mCalcLineEdit* mirrorVx, * mirrorVy, * mirrorVz;

		//缩放界面控件
		int zoomLastPntID;
		QRadioButton* zoomMoveRadio;
		QRadioButton* zoomCopyRadio;
		QPushButton* zoomCancelBtn;
		QPushButton* zoomOkBtn;
		QCheckBox* zoomEqualChkBox;
		MxFunctions::mCalcLineEdit* zoomPntX, * zoomPntY, * zoomPntZ;
		MxFunctions::mCalcLineEdit* zoomSx, * zoomSy, * zoomSz;

		//线性阵列界面控件
		int lineArrayLastDirID;
		QComboBox* lineArrayTypeCombo;
		QLabel* lineArrayUnitsLabel;
		QPushButton* lineArrayCancelBtn;
		QPushButton* lineArrayOkBtn;
		MxFunctions::mCalcLineEdit* lineArrayDisEdit;
		MxFunctions::mCalcLineEdit* lineArrayCountEdit;
		MxFunctions::mCalcLineEdit* lineArrayVx, * lineArrayVy, * lineArrayVz;

		//环形阵列界面控件
		int ringArrayLastAxisID;
		int ringArrayLastPntID;
		QComboBox* ringArrayTypeCombo;
		QLabel* ringArrayUnitsLabel;
		QPushButton* ringArrayCancelBtn;
		QPushButton* ringArrayOkBtn;
		MxFunctions::mCalcLineEdit* ringArrayCountEdit;
		MxFunctions::mCalcLineEdit* ringArrayAngleEdit;
		MxFunctions::mCalcLineEdit* ringArrayPntX, * ringArrayPntY, * ringArrayPntZ;
		MxFunctions::mCalcLineEdit* ringArrayVx, * ringArrayVy, * ringArrayVz;

		//预览勾选框
		QCheckBox* offsetPreviewChkBox = nullptr;
		QCheckBox* rotatePreviewChkBox = nullptr;
		QCheckBox* mirrorimagePreviewChkBox = nullptr;
		QCheckBox* zoomPreviewChkBox = nullptr;
		QCheckBox* lineArrayPreviewChkBox = nullptr;
		QCheckBox* ringArrayPreviewChkBox = nullptr;
	};
}