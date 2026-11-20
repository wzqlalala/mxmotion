#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "MPreData_global.h"

//std
#include <memory>

//Qt
#include <QWidget>
#include <QString>
#include <set>
#include <QTreeWidgetItem>
#include <QPointer>

#include "mxr/basic/PublicStruct.h"

#include "mBasicEnum.h"
#include "mPostGUIData.h"
#include "mModelStruct.h"
#include "mModelData.h"


namespace MechModelData
{
	class mModelData;
}

namespace mxr
{
	class mBaseRend;
	class CCSMRendWidget;
	class REBUILD_PostRend;
    class SelectService;
}

namespace PlotView
{
    class mPlotView;
}

using namespace MxFunctions;
namespace MechPreData
{
    class PreViewportWidget;
    class PostViewportWidget;
	class IViewportWidget;

	//截面
	struct mSection
	{
		int _id;//ID
		QString _name;//名称
		bool _isEnable;//是否启用

		mSection(int id, QString name, bool isEnable) {
			_id = id;
			_name = name;
			_isEnable = isEnable;
		};

        static bool isSectionNameExists(const QString& name, QList<mSection*> sections, const MechPreData::mSection* excludeSection)
        {
            const QString targetName = name.trimmed();

            for (const auto section : sections)
            {
                if (!section || section == excludeSection)
                {
                    continue;
                }

                if (section->_name.trimmed().compare(
                    targetName, Qt::CaseSensitive) == 0)
                {
                    return true;
                }
            }

            return false;
        }
	};
	//前处理视口
	class MPREDATA_EXPORT mViewportPrivate
	{
	public:
		mViewportPrivate(QString viewportname = "", ViewportType type = Viewport3D);

		~mViewportPrivate();

		QString getViewportName();//获取视口名称

		void setViewportName(QString name) { _viewportName = name; };//设置视口名称

		//用户是否手动改过视口名称（改名后不再跟随模型重命名联动前缀）
		bool isUserRenamed() const { return _userRenamed; }
		void setUserRenamed(bool renamed) { _userRenamed = renamed; }

		ViewportType getViewportType();//获取视口类型

		//获取视口全局唯一ID（用于视图树节点与视口的映射）
		int getViewportID() const { return _viewportID; }

        void setRendWidget(mxr::mBaseRend* rend);//设置视口
        void setWidget(IViewportWidget* widget);//设置视口

		mxr::mBaseRend* getRendWidget();
		IViewportWidget* getWidget();

		QList<mItemKey> getTreeSelectItems();

		void setTreeSelectItems(QList<mItemKey> items);

		void removeSelectItem(mItemKey item);

		virtual void function() = 0;

		template<typename T>
		T* toType()
		{
			//_rendWidget 为 QPointer：渲染控件随视口控件（Qt 父子链）先行析构时自动
			//置空——退出序列中面板 hideEvent 等仍会经此访问，悬空裸指针在 dynamic_cast
			//的 RTTI 查找中抛异常直接 abort（2026-11-05 等值面面板退出崩溃实锤）
			return dynamic_cast<T*>(_rendWidget.data());
		};

        template<typename T>
        T* getTypeWidget()
        {
            return dynamic_cast<T*>(_viewport_widget);
        };
	public:

		/**显示菜单栏**/
		QHash<int, QVector3D> _nodeIdVertexs;//节点IdLabel
		QHash<int, QVector3D> _eleIdVertexs;//单元IdLabel
		std::set<int> _showFreeNode;//显示的自由节点
		//后处理节点集合点显示状态（_showPostNodeSetPoints）已随节点集显隐功能取消（用户定案 2026-11-12）移除
		std::set<int> _entityRodBeam;//显示的杆梁实体形状部件id
		std::set<int> _entityShell;//显示的壳实体形状部件id
		std::set<int> _entitySpring;//显示的壳实体形状部件id
		std::set<int> _entityDamping;//显示的壳实体形状部件id

		//左键功能菜单组选中状态
		int _leftViewCheckedStatus;
		int _showViewCheckedStatus;
        int _multiplyPickStatus;

		//当前拾取模式(int)
		int _pickFilter;

	private:
		QString _viewportName;//视口名称

		//用户是否手动改过视口名称（改名后不再跟随模型重命名联动前缀）
		bool _userRenamed = false;

		ViewportType _viewportType;//视口类型

		//视口全局唯一ID（构造时分配）
		int _viewportID = 0;

		//QPointer 防悬空：渲染控件（mBaseRend=QWidget）在退出序列中随视口控件 Qt 父子链
		//先行析构，而 priv 由模型管理器持有存活更久——期间任何 toType/dynamic_cast 访问
		//悬空指针都会在 RTTI 查找抛异常 abort；QPointer 随析构自动置空，访问方判空即安全
		QPointer<mxr::mBaseRend> _rendWidget;//渲染
		IViewportWidget* _viewport_widget = nullptr; // 窗口

		//模型树选中
		QList<mItemKey> m_SelectedKey;
	};
	class MPREDATA_EXPORT mPreViewportPrivate :public mViewportPrivate
	{
	public:
		mPreViewportPrivate(QString viewportname = "");

		~mPreViewportPrivate();

		void function() override {  };

		//设置是否显示网格
		void setActiveShowMesh(bool active) {
			_isActiveShowMesh = active;
		};

		//是否显示网格
		bool isActiveShowMesh() {
			return _isActiveShowMesh;
		};
		
		//void setRendWidget();

	public:

		/**截面**/
		int _sectionMaxID; //截面最大ID
		QList<mSection*> _sections;//截面
		bool _isShowSectionLine;//剖面线显隐
		bool _isSectionColor;//剖面颜色
		bool _isSectionTransparency;//透明

	private:
		//是否显示网格
		bool _isActiveShowMesh;

	};

	class MPREDATA_EXPORT mPostViewportPrivate :public mViewportPrivate
	{
	public:
		mPostViewportPrivate(QString viewportname = "", ViewportType type = ViewportPost3D);

		~mPostViewportPrivate();

		void setPostGuiData(mPostGUIData* data);

		mPostGUIData* getPostGuiData();

		std::set<int> getModelPartVisualData();

		void setModelPartVisualData(int partID, bool visual);

		void deleteModelPartVisualData();

		void function() override {};

	public:
		/**后处理渲染**/
		//mxr::REBUILD_PostRender* _rendWidget;

		//界面数据
		mPostGUIData*	_guiData{};

		//部件隐藏数据
		std::set<int>	_modelPartHides{};

		//显示的杆实体形状部件id
		std::set<int> _entityRod;

		//显示的杆实体形状部件id
		std::set<int> _entityBeam;
	};

	class mDataStatusManager;

	/*
	* 视口包装部件：替代原 QMdiSubWindow 的键值载体。
	* 新架构下实际窗口由 MainWindow::ViewportWidget 承担，
	* 本类仅作为 PreData 侧"视口句柄"保存状态（标题/最小化标记），
	* 内容部件（PreViewportWidget/PostViewportWidget）不再挂为本类子部件，
	* 由 ViewportContainer 直接承载；本类为无内容的轻量句柄。
	*/
	class MPREDATA_EXPORT mViewportWindow : public QWidget
	{
		Q_OBJECT
	public:
		explicit mViewportWindow(QWidget* parent = nullptr);

		//设置/获取视口标题（同步到容器侧 ViewportWidget 标题栏）
		void setViewportTitle(const QString& name);
		QString getViewportTitle() const { return _viewportTitle; }

	private:
		QString _viewportTitle;
	};

	class MPREDATA_EXPORT mViewportModel : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 默认构造函数
		*/
		mViewportModel(bool isPost = false);

		/*
		* 析构函数
		*/
		~mViewportModel();


		//获取模型数据指针
		MechModelData::mModelData* getModelData();

	signals:
		/*
		* 视口重命名完成（含自动回填前缀），参数为视口ID与新名称。
		* 视图树节点与显示层据此同步文本
		*/
		void sig_viewportRenamed(int viewportID, const QString& oldName, const QString& newName);

	public:

		//删除所有视口和模型
		void deleteAllViewportModel();

		//根据当前选中类型添加视口（未选中默认为前处理视口）
		//返回新视口的包装部件（键值载体），内容部件为其子部件
		QWidget* addViewport();

		//添加视口
		QWidget* addViewport(ViewportType type, QString name = "");

		//删除视口根据名称（返回真实删除结果：调用方据此发移除信号/清容器；
		//原 QWidget* 版恒返 nullptr，被管理器误用作 bool，导致按名删除成功后
		//容器包裹部件与视图树节点残留——内容已删、包装仍在）
		bool deleteViewport(QString name);

		//删除视口（保证模型至少保留一个前处理视口）
		bool deleteViewport(QWidget* subWindow);

		//删除视口（无"最后一个视口"保护：删除模型流程清空全部视口时使用）
		bool removeViewportRaw(QWidget* subWindow);

		//获取前处理视口（激活行为由 ViewportContainer 完成，这里仅返回句柄）
		QWidget* activateViewport();

		//获取后处理视口
		QWidget* activatePostViewport();

		//根据名称获取视口
		QWidget* getViewport(QString name);

		//根据渲染部件获取视口句柄
		QWidget* getSubWindowFromRend(mxr::mBaseRend* rend);

		//设置前处理当前视口
		void setPreRenderWidget(QWidget* subWindow);

		//判断当前模型是否包含视口
		bool isContainSubWindow(QWidget* subWindow);

		//获取当前模型显示的视口
		QList<mViewportPrivate*> getRendViewportPrivates(bool containHide = false, ViewportType type = Viewport3D);

		//获取当前模型的所有视口
		const QList<QWidget*> getSubWindows() { return _viewports.keys(); };

		//获取当前前处理视口（按 _currentSubWindow 派生，非前处理视口时为空）
		mxr::CCSMRendWidget* getPreRenderWidget();

		// 获取当前前处理的新视口，包含了CCSMRendWidget
        PreViewportWidget* getPreViewportWidget();

		//获取当前后处理视口（按 _currentSubWindow 派生，非后处理视口时为空）
        mxr::REBUILD_PostRend* getPostRenderWidget();

		// 获取后处理窗口的新窗口，包含了PostRend
        PostViewportWidget* getPostWidget();

		//通过视口名称获取后处理视口
		mxr::REBUILD_PostRend* getPostRenderByViewName(const QString& viewName);

		//通过视口名称获取视口数据（等值面等按绑定视口寻址的面板用；未命中返回 nullptr）
		mViewportPrivate* getViewportPrivateByName(const QString& viewName);

		PostViewportWidget* getPostWidgetByName(const QString& viewName);


		//根据postrend获取mPostGUIData
		mPostGUIData* getGUIDataByPostRender(mxr::REBUILD_PostRend* render);

		//根据视口名称获取mPostGUIData
		mPostGUIData* getGUIDataByViewName(QString viewName);

		//清空某个后处理视口但是不删除该视口
		mxr::REBUILD_PostRend* clearViewport3D(mxr::REBUILD_PostRend* pview);

		/** 设置后处理全部部件的显隐 */
		void setPostAllPartVisual(const QString& viewName, bool isShow);

		/** 设置后处理部分部件的显隐 */
		void setPostSomePartVisual(const QString& viewName, std::set<int> partIds, bool isShow);

		/** 刷新所有后处理视图 */
		void updateAllPostView();

		//获取模型部件隐藏数据
		std::set<int> getModelPartVisualData(const QString& viewName);

		//设置模型部件隐藏数据
		void setModelPartVisualData(const QString& viewName, int partId, bool visual);

		//删除模型部件隐藏数据
		void deleteModelPartVisualData(const QString& viewName);

		//获取当前模型下的视口个数
		int getViewportNum();

		/*
		* 重置视口全局ID计数（会话恢复等"清空整个软件数据"场景使用：
		* 全部模型/视口删除后调用，使新建视口ID从初始值重新分配。
		* 仅在没有存活视口时调用才安全）
		*/
		static void resetViewportGlobalID();

		//获取当前模型下的前处理视口视口个数
		int getPreViewportNum();

		//按视口全局ID获取其子窗口（不存在返回nullptr）
		QWidget* getSubWindowByViewportID(int viewportID);

		//获取该模型全部视口的全局ID
		QList<int> getAllViewportIDs();

		/*
		* 重命名视口：同步视口数据名称、子窗口标题栏显示名，
		* 后处理渲染窗口按名称查找的键也一并更新；模型内名称唯一，重名返回false
		*/
		bool renameViewport(int viewportID, const QString& newName, bool byUser = false);

		/*
		* 按视口名称反查其全局ID（模型内名称唯一；不存在返回-1）：
		* 供 renameViewport 的调用方（后处理云图显示名接管等）从名称定位视口
		*/
		int getViewportIDByName(const QString& viewName);

		//是否为纯后处理模型
		bool isPostModel();

		//获取后处理拾取数据类型
		PickDataType getPostPickDataType(int& objectNum);

		//获取当前前处理视口数据
		mViewportPrivate* getViewportPrivate(QWidget* subWindow);

		//获取当前的拾取数据类型
		PickDataType getCurrentPickDataType(int &objectNum);

		//按视口句柄取视口数据（getViewportPrivate 别名，供会话采集等遍历使用）
		mViewportPrivate* getViewportPrivateByWindow(QWidget* subWindow) { return getViewportPrivate(subWindow); }

		//清空当前窗口拾取
		void clearPick();

		//刷新视口
		void updateRender();

		//获取MDI子窗口
		//QMdiSubWindow* getMidSubWindow() { return _subWindow; };
		
		//删除视口中的item
		void removeSelectItem(mItemKey item);

		//设置mxcae文件路径
		void setMxcaePath(QString path) { _mxcaePath = path; }

		//获取mxace文件路径
		QString getMxcaePath() { return _mxcaePath; }

		//更新部件操作
		void updateModelOperate(mxr::CCSMRendWidget* view, mxr::ModelOperateEnum type = mxr::ModelOperateEnum::ImportPart,std::set<int> = {});

		//更新几何部件操作
        void updateGeoModelOperate(mxr::CCSMRendWidget* view, mxr::ModelOperateEnum type = mxr::ModelOperateEnum::ImportPart, std::set<int> = {}, bool updateView = false);

		//更新网格部件操作
		void updateMeshModelOperate(mxr::CCSMRendWidget* view, mxr::ModelOperateEnum type = mxr::ModelOperateEnum::ImportPart);

		//更新所有部件操作
		void updateAllModelOperate(mxr::ModelOperateEnum type = mxr::ModelOperateEnum::ImportPart, std::set<int> ids = {});

		//更新所有视图的几何部件操作
		void updateAllGeoModelOperate(mxr::ModelOperateEnum type = mxr::ModelOperateEnum::ImportPart, std::set<int> ids = {});
		
		//更新所有视图的网格部件操作
		void updateAllMeshModelOperate(mxr::ModelOperateEnum type = mxr::ModelOperateEnum::ImportPart);

		//创建临时节点
		int createFreeNode(QVector3D nodeVertex);

		//获取数据状态管理类
		mDataStatusManager* getDataStatusManager() { return _dataStatusManager; }

        //模型快照
        void modelFastCapture(const QString& path, const QString& id);

        //////////////////////////////////////////////////////////////////颜色///////////////////////////////////////////////////////////

        //设置颜色类型
        void setPartType(MxFunctions::PartColorType type);

        //获取颜色类型
        MxFunctions::PartColorType getPartType() { return _colorType; }

        //更新部件显示颜色
        void updatePartShowColor(MxFunctions::PartColorType type, vector<int> partIDs);

        //更新材料显示颜色
        void updatePartShowColorByMat(int matID);

        //////////////////////////////////////////////////////////////////颜色///////////////////////////////////////////////////////////



        ////////////////////////////////////////////////////////////////////////2D视口
        ///** 获取激活曲线窗口 */
        //PlotView::mPlotView* getActivePlotXYPlot();

        //void removeAllPlotSub();

        ///** 打开2D曲线视口 */
        //QMdiSubWindow* openViewporPlot(QMdiArea* parent, PlotView::mPlotView* plotview, QString name);

        ///** 根据名字打开2d窗口 */
        //QMdiSubWindow* openViewPortPlot2DbyName(const QString& name);

        ///** 根据名称获取曲线视图 */
        //PlotView::mPlotView* getTimeCurveView(const QString& name);

        ///** 根据名称获取曲线subWindow */
        //QMdiSubWindow* getTimeCurvesubWindow(const QString& name);
        //////////////////////////////////////////////////////////////////////////2D视口

	private:
		void initModelData();

		void deleteSubWindowAndRend(QWidget* subWindow, mViewportPrivate* view);

        void setAxisIntersectPlaneFunc(mxr::SelectService* select_service, mxr::mBaseRend* base_rend, MechModelData::mModelData*);


	private:
		//模型数据
		MechModelData::mModelData *_modelData;

		//模型内当前视口句柄（由管理器激活时经 setPreRenderWidget 设置；
		//前/后处理类型化指针均由该句柄从 _viewports 派生，不再缓存）
		QWidget* _currentSubWindow = nullptr;

		//该模型的所有视口（键为视口包装部件 mViewportWindow）
		QHash<QWidget*, mViewportPrivate*> _viewports;

		//打开的mxcae文件路径
		QString _mxcaePath;

		//false:前处理	true:后处理
		bool _isPost = false; 

		//数据状态管理类
		mDataStatusManager* _dataStatusManager;

        //颜色 
        MxFunctions::PartColorType _colorType;
	};
}