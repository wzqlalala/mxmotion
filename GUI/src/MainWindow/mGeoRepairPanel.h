#pragma once
#include "mGeoDevPreviewGate.h" //【GUI 门控】质量检查/修复 V2 依赖未推送 API

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
#include <QPushButton>
#include <QRadioButton>
#include <set>
#include <vector>
#include <qvector3d.h>

#include "mGeoPanelBase.h"
//#include <mGeoCommandFactory.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGeoRepairPanel : public mGeoPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mGeoRepairPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoRepairPanel();

		/*
		* 显示特征界面
		*/
		void showFold(GeoOperationType type);

		/*
		* 进入面板时连接拾取等全局信号（mInfoWindow::createGeoRepair 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;


		void slot_contextMenuTableData(const QPoint& pos);

	private:

        bool autoSplitEdgeSearchAction();
        bool autoAdditionalEdgeSearchAction();
        bool autoRepeatFaceSearchAction();
        bool autoDuplicateLinesSearchAction();
        bool autoFitCurvesSearchAction();
        bool autoHardPointSearchAction();
		/*
		* 初始化分割边界面
		*/
		void initRepairSplitEdgeFold();

		/*
		* 初始化额外边界面
		*/
		void initAdditionalEdgeFold();

		/*
		* 初始化重复面界面
		*/
		void initRepeatFaceFold();

        /*
        * 初始化硬点删除操作界面
        */
        void initHardPointFold();

        /*
        * 初始化重复线操作界面
        */
        void initDuplicateLinesFold();

        /*
       * 初始化拟合线操作界面
       */
        void initFitCurvesFold();

		/*
		* 捕获窗口激活
		*/
		void slot_subWindowActivated();

		/*
		* 高亮几何数据变化
		*/
		void slot_highlightGeoData();

    protected:
        void hideEvent(QHideEvent* event) override;

	private:
        //清理重复面/重复线等临时符号
        void clearRepairSymbols();

        //清理分割点识别符号
        void clearSplitEdgeSymbols(bool updateRender);

        //统一执行分割边修复：调用 GeoServer 的 OCC repairSplitEdge 算法
        bool executeSplitEdgeRepair(const std::vector<int>& pointIds);

        //清理额外边识别符号
        void clearAdditionalEdgeSymbols(bool updateRender);

        //清理重复面识别符号
        void clearDuplicateFaceSymbols(bool updateRender);

        //清理重复曲线识别符号
        void clearDuplicateLineSymbols(bool updateRender);

        //清理拟合曲线识别符号
        void clearFitCurveSymbols(bool updateRender);

        //清理硬点识别符号
        void clearHardPointSymbols(bool updateRender);

        //额外边修复：红色边参与合并，绿色边作为保留边传给 OCC
        bool executeAdditionalEdgeRepair(const std::vector<int>& edgeIds,
            const std::set<int>& keepEdgeIds);

        //重复面修复：只提交仍为红色的重复面
        bool executeDuplicateFaceRepair(const std::vector<int>& faceIds);

        //拟合曲线修复：只提交仍为红色的识别组
        bool executeFitCurveRepair(const std::vector<int>& groupIds);


		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;

		//视口切换信号的连接（connectGeoSignals 建立）
		QMetaObject::Connection conn_viewportChanged;

        //分割边控件
        MxFunctions::mCalcLineEdit* maxLength;
        MxFunctions::mCalcLineEdit* minAngle;
        QLabel* maxLengthUnit;
        QLabel* minAngleUnit;
        QCheckBox* excludeChkBoxSplitEdge;

        //修复分割边识别结果：每个分割点一个符号，符号名携带几何点ID
        struct SplitEdgeGroupInfo {
            int pointId = 0;
            QString symbolName;
        };
        std::vector<SplitEdgeGroupInfo> splitEdgeGroups;

        //自动修复识别结果通用记录：itemId 为几何线/面 ID
        struct RepairSymbolInfo {
            int itemId = 0;
            QString symbolName;
        };
        std::vector<RepairSymbolInfo> additionalEdgeGroups;
        std::vector<RepairSymbolInfo> duplicateFaceGroups;
        std::vector<RepairSymbolInfo> fitCurveGroups;
        std::vector<RepairSymbolInfo> hardPointGroups;

		//额外边控件
        QCheckBox* excludeChkBoxExtraEdge;
        //重复面控件
        QCheckBox* deleteSolidFace;
        QCheckBox* excludeChkBoxDuplicateFaces;
        //硬点删除
        QComboBox* hardPointDelete;

        //重复线界面控件
        struct OverlapGroupInfo {
            int ID;                                  // 组唯一标识
            QString DuplicateLinesGroupName;         // 高亮符号名
        };
        std::vector<OverlapGroupInfo> overlapGroups;
        QCheckBox* excludeChkBox;

        //拟合线界面控件
        MxFunctions::mCalcLineEdit* maxDisFitCurves;
        QCheckBox* excludeChkBoxFitCurves;
        QCheckBox* linesFitCurvesChkBox;
        QCheckBox* arcsFitCurvesChkBox;
        QCheckBox* splinesFitCurvesChkBox;
        QCheckBox* tangencyFitCurvesChkBox;
        QLabel* maxDisFitCurvesUnit;

		//表格右键菜单
		QAction* subDelAction = nullptr;

	private slots:

		void slot_onCancel();
		//修复分割边：确认按钮修复所有仍为红色的分割点
		void slot_repairSplitEdgeAction();
		//符号点选时修复单个分割点
		void slot_repairSplitEdgeSingle(int pointId);
		void slot_additionalEdgeAction();
		void slot_repeatFaceAction();

		/*
		* 修复分割边操作
		*/
        //void slot_splitEdgesActionAll();
        //void slot_splitEdgesActionSingle(int groupID);

		/*
		* 修复额外边操作
		*/
        void slot_extraEdgesActionAll();
        void slot_extraEdgesActionSingle(int groupID);

        /*
        * 修复重复面操作
        */
        void slot_duplicateFacesActionAll();
        void slot_duplicateFacesActionSingle(int groupID);

        /*
        * 硬点删除操作
        */
        void slot_hardPointAction();

        /*
        * 修复重复线操作
        */
        void slot_duplicateLinesActionAll();
        void slot_duplicateLinesActionSingle(int groupID, int actionType);

        /*
        * 修复拟合曲线操作
        */
        void slot_fitCurvesActionAll();
        void slot_fitCurvesActionSingle(int groupID);
	};
}