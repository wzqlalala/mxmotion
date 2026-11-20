#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mGeoPanelBase.h"
#include "mxr/view/HighlightController.h"
//#include "mGeoCommandFactory.h" //【OCC摘除·S10/P0-1】退役头死 include（零引用）
#include "mGeoCheckTypes.h"

#include <QPushButton>
#include <vector>


namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGeoEditPanel : public mGeoPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mGeoEditPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoEditPanel();

		/*
		* 显示特征界面
		*/
		void showFold(GeoOperationType type);

		/*
		* 显示分割、合并的子界面
		*/
		void showChildFold(QString type);

		/*
		* 获取当前拾取类型
		*/
		FeaturePick getPickType(QString type);

		/*
		* 关闭屏幕绘制曲线功能
		*/
		void closeDrawCurve();

		/*
		* 进入面板时连接拾取、单位制等全局信号（mInfoWindow::createGeoEdit 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;

	private:
		/*
		* 初始化分割面操作界面
		*/
		void initSplitFaceFold();

		/*
		* 初始化分割体操作界面
		*/
		void initSplitSolidFold();

		/*
		* 初始化分割线操作界面
		*/
		void initSplitLineFold();

		/*
		* 初始化合并操作界面
		*/
		void initMergeFold();

		/*
		* 初始化取消裁剪面操作界面
		*/
		void initCancelTrimFaceFold();

        /*
        * 初始化面延伸操作界面
        */
        void initFaceExtendFold();

        /*
        * 初始化线连接操作界面
        */
        void initLineCombineFold();

		/*
		* 初始化修补缝隙操作界面
		*/
		void initRepairCracksFold();

        /*
        * 初始化填充曲面操作界面
        */
        void initFillSurfaceFold();

		/*
		* 初始化删除操作界面
		*/
		void iniDeleteFold();

		/*
		* 初始化删除线操作界面（仅独立线）
		*/
		void initDeleteLineFold();

        /*
        * 初始化线延长操作界面
        */
        void initLineLengthenFold();

        /*
        * 初始化面偏置操作界面
        */
        void initFaceOffsetFold();

        /*
        * 初始化特征投影操作界面
        */
        void initFeatureProjectionFold();

        /*
        * 初始化检查操作界面
        */
        void initCheckEdgeAndFaceFold();

        /*
        * 初始化微小实体检测界面
        */
        void initTinySolidFold();

        /*
        * 初始化几何干涉检查操作界面
        */
        void initInterferenceCheckFold();

		/*
		* 捕获窗口激活
		*/
		void slot_subWindowActivated();

		/*
		* 高亮几何数据变化
		*/
		void slot_highlightGeoData();

		/*
		* 收集所有等面积的面
		*/
		void CollectFaceIsEqualArea();

		/*
		* 收集所有等体积的实体
		*/
		void CollectSolidIsEqualVolume();


		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;

		//视口切换信号的连接（connectGeoSignals 建立）
		QMetaObject::Connection conn_viewportChanged;

		//第二拾取列表
		QTableWidget* spPickTable;
		QLabel* spliTypeLabel;
		QComboBox* splitTypeCombo;

		//点选分割交点信息
        bool isPntMode = false;
		std::vector<mxr::GeoSelectInfoData> vecIntersectionPnt;


		//分割面界面控件
		int spFaceLastLineID;
		int spFaceDtmLastLineID;
		
		//分割体界面控件
		int spSolidSwpLastLineID;
		int spSolidDtmLastLineID;

		//--通过点
		int spFacePntAId;
		int spFacePntBId;
		QLineEdit* spFacePntA;
		QLineEdit* spFacePntB;

		//--通过剪切线
		QCheckBox* spFaceLSmooth;
		QCheckBox* spFaceLClose;

		//--通过几何线
		QComboBox* spFaceDir;
		QComboBox* spFaceMode;
		QLineEdit* spFaceCustomLineVec;
		MxFunctions::mCalcLineEdit* spFaceDistance;
        QLabel* spFaceDisUnit;

		//--通过基准面
		QComboBox* spFaceVector;
		MxFunctions::mCalcLineEdit* spFaceSize;
		QLineEdit* spFaceStdPnt;
		QLineEdit* spFaceCustomFaceVec;
        QLabel* spFaceSizeUnit;

		//分割体
		//--通过曲线
		QCheckBox* spSolidLSmooth;
		QCheckBox* spSolidLClose;


		//--通过边界线
		QComboBox* spSolidFaceExpand;
		MxFunctions::mCalcLineEdit* spSolidFaceExNum;
		QCheckBox* spSolidExpFace;

		//--通过扫掠线
		QComboBox* spSolidDragDir;
		QLineEdit* spSolidCustomVec;
		QComboBox* spSolidDragMode;
		MxFunctions::mCalcLineEdit* spSolidDragDis;
        QLabel* spSolidDragDisUnit;

		//--通过基准面
		QComboBox* spSolidVector;
		MxFunctions::mCalcLineEdit* spSolidSize;
		QLineEdit* spSolidStdPnt;
		QLineEdit* spSolidCustomFaceVec;
        QLabel* spSolidSizeUnit;

		//--通过几何面分割
		MxFunctions::mCalcLineEdit* spSolidFaceExSize;
        QLabel* spSolidFaceExSizeUnit;

		//合并界面控件
		QCheckBox* mergeRemoveGap;

		//删除面界面控件
		QCheckBox* delFaceEqualArea;
		QCheckBox* delFaceAssociatedSolid;

		//删除体界面控件
		QCheckBox* delSolidEqualVolume;
		QCheckBox* delSolidKeepFace;

		//取消裁剪面控件
		QCheckBox* keepVertexChkBox;
		QCheckBox* extendChkBox;

		//修补缝隙
		MxFunctions::mCalcLineEdit* maxRepairFaceDis;
		MxFunctions::mCalcLineEdit* minRepairFaceDis;
        QLabel* maxRepairFaceDisUnit;
        QLabel* minRepairFaceDisUnit;

        //线延长界面控件
        MxFunctions::mCalcLineEdit* lengthSize;
        QLabel* lengthSizeUnit;
        QCheckBox* reverseLengthen;

        //面偏置界面控件
        MxFunctions::mCalcLineEdit* faceOffsetDis;
        QLabel* faceOffsetDisUnit;

        //特征投影界面控件
        int projectionAxisId;
        QLineEdit* projectionVec;
        QCheckBox* isSplitProjection;

        //线连接界面控件
        QCheckBox* isSmooth;
        int spLineAId;
        int spLineBId;
        QLineEdit* spLineA;
        QLineEdit* spLineB;
		//表格右键菜单
		QAction* subDelAction = nullptr;

		//绘制曲线信号返回的点
		QVector<QVector3D> point3dCoords;

        //短边界面控件
        MxFunctions::mCalcLineEdit* shortEdgeMaxLen;
        QLabel* shortEdgeMaxLenUnit;

        //锐边界面控件
        MxFunctions::mCalcLineEdit* sharpEdgeMaxAngle;
        QLabel* sharpEdgeMaxAngleUnit;
        QCheckBox* concave;//凹陷
        QCheckBox* convex;//凸起

        //小型面界面控件
        QCheckBox* useAreaCheckFace;//按面积查找
        QCheckBox* useWidthCheckFace;//按短边宽度查找
        QCheckBox* useRatioCheckFace;//按长宽比查找
        MxFunctions::mCalcLineEdit* smallFaceMaxArea;
        QLabel* smallFaceMaxAreaUnit;
        MxFunctions::mCalcLineEdit* smallFaceMaxWidth;
        QLabel* smallFaceMaxWidthUnit;
        //MxFunctions::mCalcLineEdit* smallFaceMinRatio;

        //微小实体界面控件
        MxFunctions::mCalcLineEdit* tinySolidMaxVolume;
        QLabel* tinySolidMaxVolumeUnit;

        //几何干涉检查界面控件
        //最近一次干涉检查返回的干涉对；供移除干涉使用
        std::vector<EventDispatcher::mGeoInterferenceEntry> m_interferencePairs;
        QString InterferName;
        QPushButton* interferenceRemoveBtn = nullptr;



private slots:
	/*
	* 连接表格右键菜单
	*/
	void slot_contextMenuTableData(const QPoint& pos);

	void slot_onCancel();

	void slot_splitFaceByPnt();

	void slot_splitFaceByCutLine();

	void slot_splitFaceByGeoLine();

	void slot_splitFaceByStdFace();

	void slot_splitSolidByCurve();

	void slot_splitSolidByBndLine();

	void slot_splitSolidBySwLine();

	void slot_splitSolidByStdFace();

	void slot_splitSolidByGeoFace();

	void slot_mergeAction();

	void slot_GeoDividerLineData(QVector<QVector3D> vec3Ddata);

	void slot_cancelCutFaceAction();

	void slot_repairFaceAction();

    void slot_fillSurfaceAction();

	void slot_deleteFaceAction();

	void slot_deleteSolidAction();

	void slot_deleteLineAction();

	void slot_splitLineAction();

    void slot_lineLengthenAction();

    void slot_faceOffsetAction();

    void slot_featureProjectionAction();

    void slot_lineCombineAction();

    void slot_faceExtendAction();

    void slot_checkShortEdge();

    void slot_checkSharpEdge();

    /*
    * 开放边检查操作（无参数，检查全部部件）
    */
    void slot_checkOpenEdge();

    void slot_checkSmallFace();

    /*
    * 微小实体检测操作
    */
    void slot_checkTinySolid();

    /*
    * 几何干涉检查操作
    */
    void slot_interferenceCheckAction();

    /*
    * 几何干涉移除
    */
    void slot_RemovalInterfer();

 };
}