#pragma once
#include "mGeoDevPreviewGate.h" //【GUI 门控】实体化 V2 依赖未推送 API
#include "mGeoDirectModelingTypes.h" //【直模引擎化·S11】缺失面分组契约

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
#include <QLineEdit>
#include <QPushButton>
#include <qvector3d.h>

#include <cstdint>
#include <vector>

#include "mGeoPanelBase.h"
//#include <mGeoCommandFactory.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGeoSolidifyPanel : public mGeoPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mGeoSolidifyPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoSolidifyPanel();

		/*
		* 显示特征界面
		*/
		void showFold(GeoOperationType type);

		/*
		* 进入面板时连接拾取、后台任务等全局信号（mInfoWindow::createGeoSolidify 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;


		//void slot_contextMenuTableData(const QPoint& pos);

	private:
        void clearRepairSymbols();
        bool autoStitchSearchAction();
        bool autoGapSearchAction();
        bool autoMissingFaceSearchAction();
		
		/*
		* 初始化拼接界面
		*/
		void initStitchFold();

		/*
		* 初始化间距界面
		*/
		void initGapFold();

		/*
		* 初始化缺失面界面
		*/
		void initMissingFaceFold();

		/*
		* 初始化逆向建模界面
		*/
		void initReverseModelFold();

		/*
		* 提取当前模型全部网格部件中的三角形单元
		* xyz逐点存放，indices为零基三角形索引
		*/
		bool extractCurrentMeshTriangles(std::vector<double>& xyz, std::vector<std::uint32_t>& indices);

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

		//拼接控件
        MxFunctions::mCalcLineEdit* stitchTolerance;
        QLabel* stitchToleranceUnit;
        QCheckBox* reserveASM;
        QCheckBox* checkOverlap;
        
        //间距控件
        MxFunctions::mCalcLineEdit* maxGapSize;
        MxFunctions::mCalcLineEdit* minGapSize;
        
        //缺失面控件
        MxFunctions::mCalcLineEdit* faceTolerance;
        MxFunctions::mCalcLineEdit* faceAngleMin;
        QLabel* faceToleranceUnit;
        QLabel* faceAngleMinUnit;
        QComboBox* repairMethod;
        QCheckBox* excludeChkBoxMissingFace;
        //【直模引擎化·S11】引擎缺失面搜索结果（替代退役 PinHole 组）
        QVector<EventDispatcher::mGeoMissingFaceGroup> m_engineMissingFaceGroups;

        //MGeoCommand::mGeoCommandSurfaceEditOfPinHole* mGeoMissingFaces = nullptr; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
#if MXGUI_HAS_SOLIDIFY_V2
        //std::vector<MGeoCommand::mGeoCommandSurfaceEditOfPinHole::PinHoleGroup> missingFacesGroups; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
#endif // missingFacesGroups

        //逆向建模控件
        QRadioButton* reverseSourceFileRadio = nullptr;
        QRadioButton* reverseSourceMeshRadio = nullptr;
        QLineEdit* reverseStlFileEdit = nullptr;
        QPushButton* reverseStlFileBtn = nullptr;
        QLineEdit* reverseStepPathEdit = nullptr;
        QPushButton* reverseStepPathBtn = nullptr;
        QCheckBox* reverseGenStepChk = nullptr;
        QCheckBox* reverseCreatePartChk = nullptr;
        QPushButton* reverseConvertBtn = nullptr;
#if MXGUI_HAS_SOLIDIFY_V2
        //MGeoCommand::mGeoCommandSolidOfStlToStep* mGeoReverseCmd = nullptr; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
#endif // mGeoReverseCmd
        int reverseTaskId = -1;


		//表格右键菜单
		QAction* subDelAction = nullptr;

    protected:
        void hideEvent(QHideEvent* event) override;

	private slots:

		void slot_onCancel();

		/*
		* 拼接操作
		*/
		void slot_stitchAction();

		/*
		* 间距修复操作
		*/
		void slot_gapAction();

		/*
		* 缺失面修复操作
		*/
        void slot_missingFacesActionAll();
        void slot_missingFacesActionSingle(int groupID);

        /*
        * 逆向建模转换操作
        */
        void slot_reverseModelAction();

        /*
        * 逆向建模后台任务结束处理
        */
        void slot_reverseTaskEnded(int taskId);

	};
}