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
#include <QMdiSubWindow>
#include <QComboBox>
#include <QRadioButton>
#include <qvector3d.h>

#include "mGeoPanelBase.h"
//#include "mGeoCommandFactory.h" //【OCC摘除·S10/P0-1】退役头（零使用）
//#include <BOPAlgo_MakeConnected.hxx> //【OCC摘除·S10/P0-1】SharedTopologyEngine 为零使用死代码（执行体已走 geoEdit().sharedTopologyOCC 引擎路径），整块下线

namespace MechMainWindow
{
#if 0 //【OCC摘除·S10/P0-1】下方三组类型（shareTopo/SharedEdgeInfo/SharedTopologyEngine）
    // 为 BOPAlgo 直调残留：全仓零使用（面板执行体已走引擎 sharedTopologyOCC），
    // 随"仅 RebuildGeometry 依赖 OCC"约束整块下线；引擎化重写共享边提取时在此重建。
    struct MMAINWINDOW_EXPORT shareTopo {
        int mainPartId;
        TopoDS_Shape mainShape;
        int mainShareFaceId;//投影后拿到
        int secondPartId;
        TopoDS_Shape secondShape;
        int secondShareFaceId;
    };

    /**
 * @brief 存储一条共享边及其对应的两个原始面的信息
 */
    struct MMAINWINDOW_EXPORT SharedEdgeInfo : public mGeoPanelBase {
        TopoDS_Edge edge;      // 共享边（结果中的边）
        TopoDS_Face face1;     // 边正侧的原始面
        TopoDS_Face face2;     // 边负侧的原始面
    };

    /**
     * @brief 共享拓扑引擎：仿照 SpaceClaim 的共享功能，允许设定距离容差，
     *        自动完成分割、共享，并记录每条共享边对应的原始面。
     */
    class SharedTopologyEngine {
    public:
        /**
         * @brief 构造函数
         * @param shapes 需要建立共享拓扑的形状列表（通常为面）
         * @param fuzzy  模糊容差，允许面之间的最大间隙，默认为 0.0
         */
        SharedTopologyEngine(const TopTools_ListOfShape& shapes, Standard_Real fuzzy = 0.0);

        /**
         * @brief 执行共享拓扑操作
         * @return true 表示成功，false 表示失败
         */
        bool Perform();

        /**
         * @brief 获取共享边信息列表
         * @return 所有共享边的信息（edge 及其对应的两个原始面）
         */
        const std::vector<SharedEdgeInfo>& GetSharedEdges() const;

        /**
         * @brief 获取共享操作后的结果形状（复合体）
         * @return 结果形状
         */
        const TopoDS_Shape& GetResultShape() const;

    private:
        TopTools_ListOfShape      myShapes;          // 输入形状
        Standard_Real             myFuzzyValue;      // 模糊容差
        TopoDS_Shape              myResultShape;     // 执行结果
        std::vector<SharedEdgeInfo> mySharedEdges;   // 提取的共享边信息

        /**
         * @brief 从算法结果中提取共享边及其对应的原始面
         */
        void ExtractSharedEdges(const BOPAlgo_MakeConnected& maker);
    };
#endif

	class MMAINWINDOW_EXPORT mGeoSharedTopologyPanel : public mGeoPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mGeoSharedTopologyPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoSharedTopologyPanel();

		/*
		* 显示特征界面
		*/
		void showFold(GeoOperationType type);

		/*
		* 进入面板时连接拾取、视口切换等全局信号（mInfoWindow::createGeoSharedTopology 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;


		void slot_contextMenuTableData(const QPoint& pos);

	private:
		/*
		* 初始化共享拓扑界面
		*/
		void initShareFold();

		/*
		* 初始化取消共享拓扑界面
		*/
		void initUnshareFold();

		/*
		* 初始化强制共享拓扑界面
		*/
		void initForceShareFold();

		//共享拓扑重合公差
		MxFunctions::mCalcLineEdit* shareTolerance;
		QLabel* shareToleranceUnit;

		//取消共享拓扑重合公差
		MxFunctions::mCalcLineEdit* unshareTolerance;
		QLabel* unshareToleranceUnit;

		//强制共享拓扑重合公差
		MxFunctions::mCalcLineEdit* forceShareTolerance;
		QLabel* forceShareToleranceUnit;

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

        //QList<shareTopo>m_shareTopoList; //【OCC摘除·S10/P0-1】死成员（类型已下线）
	private slots:

		void slot_onCancel();

        /*
		* 共享拓扑操作
		*/
		void slot_shareAction();

		/*
		* 取消共享拓扑操作
		*/
		void slot_unshareAction();

		/*
		* 强制共享拓扑操作
		*/
		void slot_forceShareAction();


	};
}
