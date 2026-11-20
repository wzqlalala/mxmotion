#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QHash>
#include <QObject>
#include <memory>
#include <functional>
#include <set>

#include "MPreData_global.h"
#include "mGeoEngineServices.h" //【OCC摘除·S10/P0-1】getGEOServer 同语法视图（随宏发放）

#include "mBasicEnum.h"

class QWidget;

namespace MechMainWindow
{
	class ViewportContainer;
	class ViewportWidget;
}

namespace MechPreData
{

	class mViewportModel;
	class mViewportPrivate;
    class mViewportManagerOperatePrivate;

	/*
	* 视口模型管理器（新架构，QObject 基类，无 QMdiArea 依赖）。
	* 窗口/页/布局由 MainWindow::ViewportContainer 承载，
	* 本类仅维护"模型-视口数据"及当前激活状态。
	*/
	class MPREDATA_EXPORT mViewportModelManager : public QObject
	{
		Q_OBJECT
	public:

		/*
		* 获取单例指针
		*/
		static mViewportModelManager* getInstance();

		/*
		* 设置/获取 headless 模式。headless 模式下不创建渲染视口，
		* 仅保留模型数据，用于批量 CLI 等无窗口场景。
		*/
		static void setHeadlessMode(bool headless);
		static bool isHeadlessMode();

	    /*
	    * 定时执行(ms, 次数)
	    */
	    void setCameraSyncTimer(int time, int n, std::function<void()> task);

		/*******************************************************************不可使用！！！！！！！***************************************************************************/
		/*
		*  删除所有模型及其视口
		*/
		void deleteAllViewportModel();

		/*
		* 基于当前的模型新建视口
		*/
		void newViewport(bool fullScreen = false);

		/*
		* 新建视口根据类型(Viewport3D/ViewportPost3D)
		*/
		void newViewport(MxFunctions::ViewportType type, QString name = "");

		/*
		* 新建模型和视口
		*/
		int newViewportModel(bool fullScreen = false);

		/*
		* 新建后处理模型和视口
		*/
		int newPostViewportModel();

		/*
		* 删除指定模型的所有视口（不保留任何视口和该模型）
		*/
		void deleteViewportModel(int viewportModelId);

		/*
		* 清空模型数据：先按原名称就地重建新模型，再删除旧模型（含全部视口）
		* 返回新模型数据ID，失败返回-1
		*/
		int clearViewportModel(int viewportModelId);

		/*
		* 删除某个视口
		*/
		void deleteViewport(int removedViewportID);

		/*
		* 删除指定模型下的指定名称的后处理视口(viewportModelId=-1表示当前模型，其余为指定模型)
		*/
		void deletePostViewport(QString name, int viewportModelId = -1);

		/*
		* 删除指定模型下的所有后处理视口
		*/
		void deletePostViewport(int viewportModelId);

		/*
		* 激活前处理视口(根据当前模型),若要指定模型可先使用setCurrentViewportModel
		*/
		void activatePreViewport();

		/*
		* 激活后处理视口根据名称(根据当前模型),若要指定模型可先使用setCurrentViewportModel
		*/
		void activatePostViewport(QString name = "");

		/*************************************************************不可使用！！！！！！！*********************************************************************************/


		/*
		* 设置当前模型
		*/
		void setCurrentViewportModel(int index);

		/*
		* 强制设置当前模型（同模型也执行赋值，不发切换通知）。
		* 会话恢复逐模型补齐视口时使用：恢复流程在模型刷新阶段已把当前模型切回该模型，
		* 普通 setCurrentViewportModel 会因"同模型重复设置"直接返回，导致后续
		* newViewport 落到了错误的模型上
		*/
		void forceSetCurrentViewportModel(int index);

		/*
		* 获取当前模型
		*/
		mViewportModel* getCurrentViewportModel();

		/*
		* 获取指定模型（按模型ID），不存在返回nullptr
		*/
		mViewportModel* getViewportModel(int modelDataID);

		/*
		* 获取当前模型视口数据
		*/
		mViewportPrivate* getCurrentViewportPrivate();

		/*
		* 按视口全局ID获取视口数据（可带出所属模型ID），不存在返回nullptr
		*/
		mViewportPrivate* getViewportPrivateByID(int viewportID, int* modelID = nullptr);

		/*
		* 按视口全局ID获取视口句柄，不存在返回nullptr
		*/
		QWidget* getSubWindowByViewportID(int viewportID);

		/*
		* 平铺所有模型的所有视口（新架构由页布局接管，保留接口兼容）
		*/
		void tileAllViewportModel(bool isAll);

		/*
		* 层叠所有模型的所有视口（新架构由页布局接管，保留接口兼容）
		*/
		void cascadeAllViewportModel(bool isAll);

		/*
		* 平铺当前模型的所有视口（新架构由页布局接管，保留接口兼容）
		*/
		void tileViewportModel(bool isAll);

		/*
		* 层叠当前模型的所有视口（新架构由页布局接管，保留接口兼容）
		*/
		void cascadeViewportModel(bool isAll);

		/*
		* 设置视口联动（同状态重复调用直接返回，避免相机同步连接叠加）
		*/
		void setViewportSync(bool isSync);

		/*
		* 获取视口联动开关状态
		*/
		bool isViewportSynced() const { return _viewportSynced; }

		/*
		* 获取模型个数
		*/
		int getViewportModelSize();

		/*
		* 模型名称是否已存在（模型树/视口重命名时的唯一性校验）
		*/
		bool isExistModelDataName(const QString& name, int excludeModelDataID = -1);

		/*
		* 获取第一个模型id
		*/
		int getBeginModelID();

		/*
		* 显示截面形状视口
		*/
		void showPropShapeViewport();

		/*
		* 隐藏截面形状视口
		*/
		void hidePropShapeViewport();

		/*
		* 获取截面形状视口
		*/
		QWidget* getPropShapeWidget();

		/*
		* 获取所有模型id
		*/
		std::vector<int> getAllModelDataID();

		/*
		* 获取当前视口句柄（按当前视口ID从当前模型派生，无当前视口时为空）
		*/
		QWidget* getPreCurrentWindow();

		/*
		* 获取当前激活视口ID（-1 表示无）
		*/
		int currentViewportID() const { return _currentViewportID; }

        /*
        * 判断模型是否为纯后处理模型
        */
        bool isPostModel(int modelDataID);

		/*************************** 新视口容器架构 ***************************/
		/*
		* 初始化视口容器（幂等）。headless 模式下不创建。
		* 容器定义在 MainWindow 模块，通过 std::function 回调反向打通，
		* 避免 PreData -> MainWindow 的反向链接依赖。
		*/
		void initViewportContainer();

		/*
		* 获取视口容器，未初始化或 headless 返回 nullptr
		*/
		MechMainWindow::ViewportContainer* getViewportContainer() const { return _viewportContainer; }

		/*
		* 按视口ID查询容器侧视口可见性（容器未初始化时返回 true）
		*/
		bool isViewportVisibleInContainer(int viewportID) const;

		/*
		* 同步视口标题到容器侧标题栏（容器未初始化时为空操作）
		*/
		void syncViewportTitleToContainer(int viewportID, const QString& name);

		/*
		* 注册容器操作回调（add/remove/setActive），由 MainWindow 注入
		*/
		void setViewportContainerCallbacks(
			std::function<void(int, QWidget*)> addFunc,
			std::function<void(int)> removeFunc,
			std::function<void(int)> setActiveFunc);

		/*
		* 注册容器查询回调（可见性/标题设置/当前激活视口ID），由 MainWindow 注入
		*/
		void setViewportContainerQueryCallbacks(
			std::function<bool(int)> visibleFunc,
			std::function<void(int, const QString&)> setTitleFunc,
			std::function<int()> activeViewportIdFunc = nullptr);

		/*
		* 注册容器创建回调（由 MainWindow 注入，initViewportContainer 时调用）
		*/
		void setViewportContainerCreateFunc(std::function<MechMainWindow::ViewportContainer*()> createFunc);

		/*
		* 按视口ID激活视口（新架构入口）
		*/
		void activateViewportByID(int viewportID);

		/*
		* 视口从隐藏转为可见后补一次渲染：隐藏期间对渲染部件的重绘请求
		* （QWidget::update）全部无效，重新显示时呈现的是旧帧，需显式出帧
		*/
		void refreshViewportRender(int viewportID);

		/*
		* 是否处于批量删除视口流程（deleteViewportModel 期间为 true）。
		* 批量期间视图树收到 sig_viewportRemoved 只移除节点/重排，不做激活转移，
		* 激活在批量结束后统一处理
		*/
		bool isBatchRemovingViewports() const { return _batchRemovingViewports; }


        /***************************plotview*****************/
        // 打开plotView窗口
        //void openViewporPlot(PlotView::mPlotView* plotview, QString name);

signals:
	/*
	* 当前视口/模型切换完成后再发出，保证外部接收者拿到最新的当前模型与视口数据
	* 参数 viewportID 为当前激活视口全局ID（可能为 -1）
	*/
	void currentViewportChanged(int viewportID);

	/*
	* 新视口创建完成后发出（视口数据已就绪），参数为视口全局ID
	*/
	void sig_viewportAdded(int viewportID);

	/*
	* 视口删除完成后发出，参数为视口全局ID
	*/
	void sig_viewportRemoved(int viewportID);

	/*
	* 视口重命名完成后发出（含树名回填的自动改名），视图树节点据此同步文本
	*/
	void sig_viewportRenamed(int viewportID, const QString& oldName, const QString& newName);

	public slots:
		/*
		* 接收网格同步虚拓扑信号，更新抑制几何边渲染
		*/
		void slot_MeshSuture(bool isSuppress, std::set<int> geoIds);

		/*
		* 视口激活切换处理（由容器 sig_activeViewportChanged 驱动）
		*/
		void slot_viewportActivated(int viewportID);

		/*
		* 按容器当前激活视口全量同步轴标定位模式（幂等）：
		* 激活视口=默认位置；其余视口=按可见内容包围盒贴左下角
		*（渲染库 REBUILD_mAxisWindow::SetViewportActive，只改位置不改朝向）。
		* 由容器 sig_activeViewportChanged 与各删除流程末尾驱动
		*（删除导致的激活转移不发 sig_activeViewportChanged，需手动补调）
		*/
		void slot_syncViewportAxisActive();

	private:
		void connectSignal();
		void disconnectSignal();

		/*
		* 生成模型名称（模型树/视口命名的唯一来源）：
		* 扫描所有模型数据的名称，前处理按"模型-N"、后处理按"后处理模型-N"取最大后缀+1
		*/
		QString makeModelName(bool isPost);

	private:
		/*
		* 默认构造函数
		*/
		mViewportModelManager();

		/*
		* 析构函数
		*/
		~mViewportModelManager();

	private:
		//单例指针
		static mViewportModelManager* _instance;

		//headless 模式下不创建渲染视口
		static bool _headlessMode;

		//当前模型
		mViewportModel* _currentViewportModel;

		//当前激活视口ID（-1 表示无；视口句柄不再缓存，按 ID 从模型派生）
		int _currentViewportID = -1;

		//批量删除视口流程标志（deleteViewportModel 期间为 true）
		bool _batchRemovingViewports = false;

		//模型管理
		QHash<int, mViewportModel*> _viewportModel;

		//截面形状视口
		QPair<QWidget*, QWidget*> _propShapeWidget;

		//视口联动的连接
		QHash<mViewportPrivate*, QList<QMetaObject::Connection>> _connects;

		//内部槽函数的操作实现防止暴露头文件
		std::shared_ptr<mViewportManagerOperatePrivate> _op_private;

		//视口联动开关状态（防重复连接）
		bool _viewportSynced = false;

		//新架构视口容器（MainWindow 模块定义，指针透传避免反向链接）
		MechMainWindow::ViewportContainer* _viewportContainer = nullptr;

		//容器操作回调（由 MainWindow 注入）
		std::function<void(int, QWidget*)> _containerAddFunc;
		std::function<void(int)> _containerRemoveFunc;
		std::function<void(int)> _containerSetActiveFunc;
		std::function<MechMainWindow::ViewportContainer*()> _containerCreateFunc;
		//容器可见性/标题查询回调（避免 PreData 依赖容器具体类型）
		std::function<bool(int)> _containerVisibleFunc;
		std::function<void(int, const QString&)> _containerSetTitleFunc;
		//容器当前激活视口ID查询回调（同上；删除视口后的静默激活转移只有容器知道）
		std::function<int()> _containerActiveViewportIdFunc;
	};
}

#define VMManager MechPreData::mViewportModelManager::getInstance()
#define getCVM MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel()
//【OCC摘除·S10/P0-1】getGEOServer 切引擎句柄：engine() 返回零 OCC 的 mGeoEngineHandle，
// 配合同语法服务视图（mGeoEngineServices.h）——318 处调用点语法零改动；
// server() 模板类型已随 GeoEngineData pimpl 化退出导出头（MechModelData 仓零 OCC）。
#define getGEOServer MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel()->getModelData()->getGeoEngineDataMgr()->engine()
