
#include "mViewportModel.h"
#include <QRegularExpression>
#include <QFile>
//【关闭挂起探针·临时】追加写关闭链路日志（定位完成后整体删除）
static void shutdownTrace(const QString& tag) {
    QFile file(QStringLiteral("E:/Mech_GUI/shutdown_trace.log"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        file.write(tag.toUtf8());
        file.write(QByteArray(1, 0x0A));
        file.close();
    }
}
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMMeshModelRender.h"
#include "mxr/view/mPreGeoModelRender.h"
#include "mxr/view/SelectionCaculator.h"
#include "InteractionCore/Service/SelectService.h"


//BasicData
#include "mMeshData.h"
#include "mMapRelationManager.h"
#include "mMaterialBase.h"
#include "mPropertyBase.h"
#include "mCoordinateBase.h"
#include "mCoordinateManager.h"

//Qt
#include <mGeoDataBase.h>
//【渲染断线修复】ZsModelData 直存部件管理器后，构造点必须注入（原穿透查询已移除）
#include <mGeoManager.h>
#include <QMdiArea>
#include <QTimer>
#include <QApplication>
#include <QEvent>
#include <QDir>
#include <QPainter>
#include <QStyle>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <QDebug>

#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "PickCommonFunction.h"
#include "PreViewportWidget.h"
#include "PostViewportWidget.h"
#include "mViewportModelManager.h"

using namespace MechModelData;
using namespace mxr;
using namespace MechCommon;
using namespace MechMapRelation;
using namespace MechMaterialData;
using namespace MechPropertyData;
namespace MechPreData
{
	std::map<QString, ModelOperateEnum> mapModelOpreate = {
		{"ImportPart", ImportPart},
		{"AddOrDeleteTempNode", AddOrDeleteTempNode},
		{"ShowOnePart", ShowOnePart},
		{"HideOnePart",HideOnePart},
		{"AppendOnePart",AppendOnePart},
		{"DeleteOnePart",DeleteOnePart},
		{"ReplaceOnePart",ReplaceOnePart}
	};

	//视口全局唯一ID分配
	static int s_nextViewportID = 1;

	void mViewportModel::resetViewportGlobalID()
	{
		s_nextViewportID = 1;
	}

	mViewportModel::mViewportModel(bool isPost):_isPost(isPost)
	{
        _colorType = PartColorType::partColor;
		_dataStatusManager = new mDataStatusManager;
		initModelData();
	}

	//mViewportModel::mViewportModel(std::shared_ptr<MechModelData::mModelData> modelData)
	//{
	//	_modelData = modelData;
	//}

	mViewportModel::~mViewportModel()
	{
		shutdownTrace("[viewport] dtor begin");
		if (_dataStatusManager != nullptr)
		{
			delete _dataStatusManager;
			_dataStatusManager = nullptr;
		}

		shutdownTrace("[viewport] dataStatus deleted");
		if (_modelData != nullptr)
		{
			delete _modelData;
			_modelData = nullptr;
		shutdownTrace("[viewport] modelData deleted");
		}

  //      if (_preRenderWidget != nullptr)
  //      {
  //          delete _preRenderWidget;
		//	_preRenderWidget = nullptr;
  //      }
		//if (_subWindow != nullptr)
		//{
		//	delete _subWindow;
		//	_subWindow = nullptr;
		//}
	}

	MechModelData::mModelData* mViewportModel::getModelData()
	{
		return _modelData;
	}

	void mViewportModel::deleteAllViewportModel()
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (_viewports.value(iter.key()))
			{
				auto subWindow = iter.key();
				delete _viewports[subWindow];
				_viewports[subWindow] = nullptr;
				delete subWindow;
			}
		}
        _currentSubWindow = nullptr;
		// _modelData 可能已在析构或其他清理路径中被提前释放（剖分线程持有可能延长其生命周期的操作），
		// 此处需判空避免 delete 悬空指针
		if (_modelData != nullptr)
		{
			delete _modelData;
			_modelData = nullptr;
		}
		_viewports.clear();
	}

	QWidget* mViewportModel::addViewport()
	{
		//未指定类型时默认前处理3D视口。
		//注意：不能用 mViewportModelManager::getInstance()->getCurrentViewportPrivate()，
		//该调用在管理器构造期间会触发 getInstance() 递归构造（单例尚未返回），导致重复初始化。
		//且历史语义本就是"新建前处理视口"（类型参数版才会区分前/后处理），此处固定 Viewport3D。
		return addViewport(Viewport3D);
	}

	QWidget* mViewportModel::addViewport(ViewportType type, QString name)
	{
		//模型内视口类型唯一：前处理模型（_isPost=false）只接受 Viewport3D，
		//后处理模型只接受后处理类型（ViewportPost3D 等），混加属于调用方逻辑错误
		const bool typeMatched = _isPost ? (type != Viewport3D) : (type == Viewport3D);
		if (!typeMatched)
		{
			Q_ASSERT_X(false, "mViewportModel::addViewport", "viewport type does not match model type (_isPost)");
			return nullptr;
		}

		if (type == Viewport3D)
		{
			if (name.isEmpty())
			{
				//前处理视口默认命名：视口-N。序号取已有名称中的最大后缀+1
				//（与 AddModelTree 的"模型-N"规则一致），避免删除中间视口后计数回退产生重名
				static const QRegularExpression suffixRe(QString::fromUtf8("视口-(\\d+)$"));
				int maxIndex = 0;
				QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
				while (iter.hasNext())
				{
					iter.next();
					if (!iter.value()) continue;
					auto match = suffixRe.match(iter.value()->getViewportName());
					if (match.hasMatch() && match.captured(1).toInt() > maxIndex)
					{
						maxIndex = match.captured(1).toInt();
					}
				}
				//命名规则：模型数据名称后接 "· 视口-N"（模型名称在模型创建时已写入 modelData，
				//模型树与视口命名均以它为主导）
				const QString prefix = _modelData->getModelDataName() + QString::fromUtf8("· ");
				name = prefix + QString("视口-%1").arg(maxIndex + 1);
			}
			auto viewportPrivate = new mPreViewportPrivate(name);
            auto viewport_pre = new PreViewportWidget();
            viewportPrivate->setWidget(viewport_pre);
			auto rendWidget = static_cast<mxr::CCSMRendWidget*>(viewport_pre->renderWidget());
            //【渲染断线修复】第 4 参注入中性部件管理器：渲染代理经 getGeoPartMgr() 读取，
            // 缺失时 CCSMVisualDataProxy::updateGeoModel 会静默早退（视口无显示）
            rendWidget->initModelData({_modelData->getMeshData(),_modelData->getGeoData(),_modelData->getGeoManager(),_modelData->getGeoManager()->getGeoPartMgr()});
			auto subWindow = new mViewportWindow;
			subWindow->setViewportTitle(name);
			setAxisIntersectPlaneFunc(viewport_pre->interaction()->getSelectService(), rendWidget, _modelData);
			_viewports.insert(subWindow, viewportPrivate);
			updateModelOperate(rendWidget);
			return subWindow;
		}
		else
		{
			if (getViewport(name) != nullptr) return getViewport(name);

			auto viewportPrivate = new mPostViewportPrivate(name, type);
            auto viewport_post = new PostViewportWidget();
            viewportPrivate->setWidget(viewport_post);

			auto rendWidget = static_cast<mxr::REBUILD_PostRend*>(viewport_post->renderWidget());
			rendWidget->setViewportName(name);
            rendWidget->setPickFilter(PickFilter::PickGeoSmartPick);
            auto postGuiData = new mPostGUIData();
			viewportPrivate->setPostGuiData(postGuiData);
			auto subWindow = new mViewportWindow;
			subWindow->setViewportTitle(name);
			_viewports.insert(subWindow, viewportPrivate);
			return subWindow;
		}
		return nullptr;
	}

	bool mViewportModel::deleteViewport(QString name)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportName() == name)
			{
				//转发到带"最后一个视口"守卫的重载，透传真实删除结果
				return deleteViewport(iter.key());
			}
		}
		return false;
	}

	bool mViewportModel::deleteViewport(QWidget* subWindow)
	{
        if (!_viewports.contains(subWindow))
            return false;

		//模型内视口类型唯一（addViewport 有校验），不允许删除最后一个视口：
		//前处理/后处理模型均须至少保留一个视口保证显示
		if (_viewports.size() == 1)
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString(QObject::tr("当前仅存在一个视口，为保证正常显示，不允许删除最后一个视口。")));
			return false;
		}

		return removeViewportRaw(subWindow);
	}

	bool mViewportModel::removeViewportRaw(QWidget* subWindow)
	{
		if (!_viewports.contains(subWindow))
			return false;

		auto temp = _viewports[subWindow];
		_viewports.remove(subWindow);
		//删除的是当前视口时同步清空当前句柄，后续激活由管理器恢复
		if (_currentSubWindow == subWindow)
		{
			_currentSubWindow = nullptr;
		}
		deleteSubWindowAndRend(subWindow, temp);
		//返回真实删除结果：调用方（视口管理器）依据该返回值发 sig_viewportRemoved
		return true;
	}

	QWidget* mViewportModel::activateViewport()
	{
		//仅返回目标视口句柄，激活动作由 ViewportContainer 完成
		auto* currentRend = getPreRenderWidget();
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportType() == Viewport3D)
			{
				if (currentRend)
				{
					if (currentRend == iter.value()->getRendWidget())
					{
						return iter.key();
					}
				}
				else
				{
					return iter.key();
				}
			}
		}
		return nullptr;
	}

	QWidget* mViewportModel::activatePostViewport()
	{
		//仅返回目标视口句柄，激活动作由 ViewportContainer 完成
		auto* currentPostRend = getPostRenderWidget();
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportType() == ViewportPost3D)
			{
				if (currentPostRend)
				{
					if (currentPostRend == iter.value()->getRendWidget())
					{
						return iter.key();
					}
				}
				else
				{
					return iter.key();
				}
			}
		}
		return nullptr;
	}

	QWidget* mViewportModel::getViewport(QString name)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportName() == name)
			{
				return iter.key();
			}
		}
		return nullptr;
	}

	QWidget* mViewportModel::getSubWindowFromRend(mBaseRend* rend)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getRendWidget() == rend)
			{
				return iter.key();
			}
		}
		return nullptr;
	}

	void mViewportModel::setPreRenderWidget(QWidget* subWindow)
	{
		//仅记录模型内当前视口句柄；前/后处理类型化指针由 getter 从该句柄派生
		_currentSubWindow = _viewports.contains(subWindow) ? subWindow : nullptr;
	}

	mxr::CCSMRendWidget* mViewportModel::getPreRenderWidget()
	{
		auto* priv = _viewports.value(_currentSubWindow);
		return priv ? priv->toType<mxr::CCSMRendWidget>() : nullptr;
	}

	PreViewportWidget* mViewportModel::getPreViewportWidget()
	{
		auto* priv = _viewports.value(_currentSubWindow);
		return priv ? priv->getTypeWidget<PreViewportWidget>() : nullptr;
	}

	mxr::REBUILD_PostRend* mViewportModel::getPostRenderWidget()
	{
		auto* priv = _viewports.value(_currentSubWindow);
		return priv ? priv->toType<mxr::REBUILD_PostRend>() : nullptr;
	}

	PostViewportWidget* mViewportModel::getPostWidget()
	{
		auto* priv = _viewports.value(_currentSubWindow);
		return priv ? priv->getTypeWidget<PostViewportWidget>() : nullptr;
	}

	bool mViewportModel::isContainSubWindow(QWidget* subWindow)
	{
		return _viewports.contains(subWindow);
	}

	QList<mViewportPrivate*> mViewportModel::getRendViewportPrivates(bool containHide, ViewportType type)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		QList<mViewportPrivate*> rends;
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportType() == type)
			{
				if (!containHide)
				{
					//新架构下显隐由 ViewportContainer 管理，句柄本身不可见，
					//以容器内对应 ViewportWidget 的可见性为准
					if (!mViewportModelManager::getInstance()->isViewportVisibleInContainer(iter.value()->getViewportID()))
					{
						continue;
					}
				}
				rends.append(iter.value());
			}
		}
		return rends;
	}

	mxr::REBUILD_PostRend* mViewportModel::getPostRenderByViewName(const QString& viewName)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			auto val = iter.value();
			if (iter.value())
			{
				auto name = iter.value()->getViewportName();
				std::string view_name = viewName.toStdString();
			}
			if (iter.value() && iter.value()->getViewportName() == viewName)
			{
				return dynamic_cast<mxr::REBUILD_PostRend*>(iter.value()->getRendWidget());
			}
		}
		return nullptr;
	}

	mViewportPrivate* mViewportModel::getViewportPrivateByName(const QString& viewName)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportName() == viewName)
			{
				return iter.value();
			}
		}
		return nullptr;
	}

	PostViewportWidget* mViewportModel::getPostWidgetByName(const QString& viewName)
	{
        QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
        while (iter.hasNext())
        {
            iter.next();
            if (iter.value() && iter.value()->getViewportName() == viewName)
            {
                return dynamic_cast<PostViewportWidget*>(iter.value()->getWidget());
            }
        }
		return nullptr;
	}

	mPostGUIData* mViewportModel::getGUIDataByPostRender(REBUILD_PostRend* render)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getRendWidget() == render)
			{
				mPostViewportPrivate* postPrivate= dynamic_cast<mPostViewportPrivate*>(iter.value());
				return postPrivate->getPostGuiData();
			}
		}
		return nullptr;
	}

	mPostGUIData* mViewportModel::getGUIDataByViewName(QString viewName)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportName() == viewName)
			{
				mPostViewportPrivate* postPrivate = dynamic_cast<mPostViewportPrivate*>(iter.value());
				return postPrivate->getPostGuiData();
			}
		}
		return nullptr;
	}

	int mViewportModel::getViewportNum()
	{
		return _viewports.size();
	}

	int mViewportModel::getViewportIDByName(const QString& viewName)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportName() == viewName)
			{
				return iter.value()->getViewportID();
			}
		}
		return -1;
	}

	int mViewportModel::getPreViewportNum()
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		int num = 0;
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportType() == Viewport3D)
			{
				num++;
			}
		}
		return num;
	}

	QWidget* mViewportModel::getSubWindowByViewportID(int viewportID)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportID() == viewportID)
			{
				return iter.key();
			}
		}
		return nullptr;
	}

	QList<int> mViewportModel::getAllViewportIDs()
	{
		QList<int> ids;
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value())
			{
				ids.push_back(iter.value()->getViewportID());
			}
		}
		return ids;
	}

	bool mViewportModel::renameViewport(int viewportID, const QString& newName, bool byUser)
	{
		auto subWindow = getSubWindowByViewportID(viewportID);
		if (!subWindow || newName.isEmpty()) return false;
		auto priv = getViewportPrivate(subWindow);
		if (!priv) return false;

		//模型内视口名称唯一（getViewport 按名称查找依赖唯一性）
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value() != priv && iter.value()->getViewportName() == newName)
			{
				return false;
			}
		}

		const QString oldViewportName = priv->getViewportName();
		//视口名全局唯一（跨模型重名同样禁止）：会话文件(.mxses)按视口名单键
		//引用页结构，依赖该不变量
		{
			auto* vmManager = mViewportModelManager::getInstance();
			for (int mid : vmManager->getAllModelDataID())
			{
				auto* other = vmManager->getViewportModel(mid);
				if (!other) continue;
				const QList<QWidget*> otherSubs = other->getSubWindows();
				for (QWidget* w : otherSubs)
				{
					auto* p = other->getViewportPrivate(w);
					if (p && p != priv && p->getViewportName() == newName)
					{
						return false;
					}
				}
			}
		}

		priv->setViewportName(newName);
		//用户手动改名的视口打上标记，之后模型重命名不再联动替换其前缀
		if (byUser)
		{
			priv->setUserRenamed(true);
		}

		//自定义标题栏同步显示名
		auto viewportSub = dynamic_cast<mViewportWindow*>(subWindow);
		if (viewportSub)
		{
			viewportSub->setViewportTitle(newName);
			viewportSub->update();
		}

		//后处理渲染窗口按名称查找（getPostRenderByViewName 等），键需同步更新
		if (priv->getViewportType() == ViewportPost3D)
		{
			auto postRend = dynamic_cast<mxr::REBUILD_PostRend*>(priv->getRendWidget());
			if (postRend)
			{
				postRend->setViewportName(newName);
			}
		}

		emit sig_viewportRenamed(viewportID, oldViewportName, newName);
		//同步容器侧标题栏文本
		mViewportModelManager::getInstance()->syncViewportTitleToContainer(viewportID, newName);
		return true;
	}

	mViewportWindow::mViewportWindow(QWidget* parent)
		: QWidget(parent)
	{
		//轻量句柄：不承载内容，内容部件由 ViewportContainer 管理
		setVisible(false);
	}

	void mViewportWindow::setViewportTitle(const QString& name)
	{
		_viewportTitle = name;
	}

	mxr::REBUILD_PostRend* mViewportModel::clearViewport3D(mxr::REBUILD_PostRend* pview)
	{
		if (pview == nullptr)
			return nullptr;

		QString viewName = pview->getViewportName();
		QWidget* tepSub = this->getViewport(viewName);
		if (tepSub == nullptr)
		{
			return nullptr;
		}

		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getRendWidget() == pview)
			{
				mViewportPrivate*  pridata = _viewports.take(iter.key());
				if (pridata != nullptr)
				{
					delete pridata;
					pridata = nullptr;
				}
				break;
			}
		}

		REBUILD_PostRend* newRender = new REBUILD_PostRend(viewName);
		auto viewportPrivate = new mPostViewportPrivate(viewName);
		auto postGuiData = new mPostGUIData();
		viewportPrivate->setRendWidget(newRender);
		viewportPrivate->setPostGuiData(postGuiData);
		_viewports.insert(tepSub, viewportPrivate);
		// 旧渲染器已被销毁并以同名重建，若被替换的正是当前激活的后处理视口，必须同步指向新渲染器，
		// 否则 getPostRenderWidget() 返回悬空指针
		//if (_currentPostViewport == pview)
		//	_currentPostViewport = newRender;
		return newRender;
	}

	void mViewportModel::setPostAllPartVisual(const QString& viewName, bool isShow)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		if (isShow)
		{
			while (iter.hasNext())
			{
				iter.next();
				if (iter.value() && iter.value()->getViewportName() == viewName)
				{
					REBUILD_PostRend* mpostrend =  dynamic_cast<mxr::REBUILD_PostRend*>(iter.value()->getRendWidget());
					auto mpostrender = mpostrend->getPostRender();
					mpostrender->updateAllModelOperate(ShowAllPartOperate);
					return;
				}
			}
		}
		else
		{
			while (iter.hasNext())
			{
				iter.next();
				if (iter.value() && iter.value()->getViewportName() == viewName)
				{
					REBUILD_PostRend* mpostrend = dynamic_cast<mxr::REBUILD_PostRend*>(iter.value()->getRendWidget());
					auto mpostrender = mpostrend->getPostRender();
					mpostrender->updateAllModelOperate(HideAllPartOperate);
					return;
				}
			}
		}
	}

	void mViewportModel::setPostSomePartVisual(const QString& viewName, std::set<int> partIds, bool isShow)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		if (isShow)
		{
			while (iter.hasNext())
			{
				iter.next();
				if (iter.value() && iter.value()->getViewportName() == viewName && iter.value()->getViewportType() == ViewportPost3D)
				{
					REBUILD_PostRend* mpostrend = dynamic_cast<mxr::REBUILD_PostRend*>(iter.value()->getRendWidget());
					auto mpostrender = mpostrend->getPostRender();
					QPair<mxr::PostModelOperateEnum, std::set<int>> postModelOperates(ShowOnePartOperate, partIds);
					mpostrender->updateOneModelOperate(postModelOperates);
					return;
				}
			}
		}
		else
		{
			while (iter.hasNext())
			{
				iter.next();
				if (iter.value() && iter.value()->getViewportName() == viewName && iter.value()->getViewportType() == ViewportPost3D)
				{
					REBUILD_PostRend* mpostrend = dynamic_cast<mxr::REBUILD_PostRend*>(iter.value()->getRendWidget());
					auto mpostrender = mpostrend->getPostRender();
					QPair<mxr::PostModelOperateEnum, std::set<int>> postModelOperates(HideOnePartOperate, partIds);
					mpostrender->updateOneModelOperate(postModelOperates);
					return;
				}
			}
		}
	}

	void mViewportModel::updateAllPostView()
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportType() == ViewportPost3D)
			{
				//REBUILD_PostRend* mpostrend = dynamic_cast<mxr::REBUILD_PostRend*>(iter.value()->getRendWidget());
				//mpostrend->update();
				iter.value()->getRendWidget()->update();
			}
		}
	}

	std::set<int> mViewportModel::getModelPartVisualData(const QString& viewName)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportName() == viewName)
			{
				mPostViewportPrivate* postPrivate = dynamic_cast<mPostViewportPrivate*>(iter.value());
				return postPrivate->getModelPartVisualData();
			}
		}
		return {};
	}

	void mViewportModel::setModelPartVisualData(const QString& viewName, int partId, bool visual)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportName() == viewName)
			{
				mPostViewportPrivate* postPrivate = dynamic_cast<mPostViewportPrivate*>(iter.value());
				return postPrivate->setModelPartVisualData(partId, visual);
			}
		}
	}

	void mViewportModel::deleteModelPartVisualData(const QString& viewName)
	{
		QHashIterator<QWidget*, mViewportPrivate*> iter(_viewports);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() && iter.value()->getViewportName() == viewName)
			{
				mPostViewportPrivate* postPrivate = dynamic_cast<mPostViewportPrivate*>(iter.value());
				return postPrivate->deleteModelPartVisualData();
			}
		}
	}

	bool mViewportModel::isPostModel()
	{
		return _isPost;
	}

	PickDataType mViewportModel::getPostPickDataType(int& objectNum)
	{
		PickDataType pickDataType = PickDataType::None;
		objectNum = 0;

		auto* postViewport = getPostRenderWidget();
		if (postViewport == nullptr)
		{
			return pickDataType;
		}

		auto pickFilter = postViewport->getPickFilter();
		auto mesh = postViewport->getSelectionContainer()->getMeshSelectData();

		// ------------ Mesh Object ------------
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshNode)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshNode).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::Node;
			}
		}
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshLine)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshLine).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::ElementEdge;
			}
		}
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshFace)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshFace).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::ElementFace;
			}
		}
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshElement)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshElement).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::Element;
			}
		}
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshPart)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshPart).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::Part;
			}
		}

		return PickDataType::None;
	}

	mViewportPrivate* mViewportModel::getViewportPrivate(QWidget* subWindow)
	{
		if (isContainSubWindow(subWindow))
		{
			return _viewports[subWindow];
		}
		return nullptr;
	}

	PickDataType mViewportModel::getCurrentPickDataType(int& objectNum)
	{
		PickDataType pickDataType = PickDataType::None;
		objectNum = 0;

		auto* preViewport = getPreRenderWidget();
		if (preViewport == nullptr)
		{
			return pickDataType;
		}

		auto pickFilter = preViewport->getPickFilter();
		auto geo = preViewport->getSelectionContainer()->getGeoSelectData();
		auto mesh = preViewport->getSelectionContainer()->getMeshSelectData();

		if (((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickGeoPart) || ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshPart))
		{
			auto n1 = preViewport->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshPart).size();
			auto n2 = preViewport->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoPart).size();
			if (n1 > 0 || n2 > 0)
			{
				objectNum = n1+ n2;
				return PickDataType::Part;
			}
		}

		// ------------ Geo Object ------------
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickGeoSolid)
		{
			auto n = geo->getSelectDataByType(SelectionInfoType::GeoSolid).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::GeoSolid;
			}
		}

		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickGeoFace)
		{
			auto n = geo->getSelectDataByType(SelectionInfoType::GeoFace).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::GeoFace;
			}
		}

		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickGeoLine)
		{
			auto n = geo->getSelectDataByType(SelectionInfoType::GeoEdge).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::GeoEdge;
			}
		}

		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickGeoPoint)
		{
			auto n = geo->getSelectDataByType(SelectionInfoType::GeoPoint).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::GeoPoint;
			}
		}
		// ------------ Mesh Object ------------
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshNode)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshNode).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::Node;
			}
		}
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshLine)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshLine).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::ElementEdge;
			}
		}
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshFace)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshFace).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::ElementFace;
			}
		}
		if ((uint64_t)pickFilter & (uint64_t)mxr::PickFilter::PickMeshElement)
		{
			auto n = mesh->getSelectDataByType(SelectionInfoType::MeshElement).size();
			if (n > 0)
			{
				objectNum = n;
				return PickDataType::Element;
			}
		}

		return PickDataType::None;
	}

	void mViewportModel::clearPick()
	{
		auto* preViewport = getPreRenderWidget();
		if (preViewport)
		{
			ClearPreWidgetGeoPick(preViewport);
			ClearPreWidgetMeshPick(preViewport);
            UpdatePreWidgetHighlightWithoutRTRC(preViewport);
			updateRender();
		}

		// 后处理清除拾取
		auto* postViewport = getPostRenderWidget();
		if (postViewport)
		{
			auto postRender = postViewport->getPostRender();   // S5 门面直接替换：shared_ptr 门面恒持有会话
			if (!postRender || !postRender->frameProxy(postRender->currentFrame())) return;   // F8：换帧/重装载后重取
			ClearPostWidgetMeshPick(postViewport);
			UpdatePostWidgetHighlight(postViewport);
			emit postViewport->getSelectionContainer()->ReadyToReadContainer();
			updateRender();
		}
	}

	void mViewportModel::updateRender()
	{
		if (auto* preViewport = getPreRenderWidget())
		{
			preViewport->updatePaint();
		}

		if (auto* postViewport = getPostRenderWidget())
		{
			postViewport->updatePaint();
		}
	}

	void mViewportModel::removeSelectItem(mItemKey item)
	{
		for (auto& it : _viewports)
		{
			it->removeSelectItem(item);
		}
	}

	void mViewportModel::updateModelOperate(mxr::CCSMRendWidget* view, mxr::ModelOperateEnum type, std::set<int> ids)
	{
		if (view)
		{
			view->getPreRender()->updateGeoAndMeshOperate({ type,ids });
			view->updatePaint();
			if (type == ImportPart)
			{
				view->SetRotateCenterToModelCenter();
				view->FitView();
				view->getCameraController()->immediateToTargetState();
			}
		}
	}

    void mViewportModel::updateGeoModelOperate(mxr::CCSMRendWidget* view, mxr::ModelOperateEnum type, std::set<int> ids, bool updateView)
    {
        if (view)
        {
            QElapsedTimer timer;
            timer.start();
            view->getPreRender()->updateGeoOperate({ type,ids });
            timer.restart();
            view->updatePaint();
            if (updateView)
            {
                view->SetRotateCenterToModelCenter();
                view->FitView();
            }
        }
    }

	void mViewportModel::updateMeshModelOperate(mxr::CCSMRendWidget* view, mxr::ModelOperateEnum type)
	{
		if (view)
		{
			view->getPreRender()->updateMeshOperate({ type,{} });
			view->updatePaint();
			if (type == ImportPart)
			{
				view->SetRotateCenterToModelCenter();
				view->FitView();
			}
		}
	}

	void mViewportModel::updateAllModelOperate(mxr::ModelOperateEnum type, std::set<int> ids)
	{
		for (auto view : _viewports)
		{
			if (view == nullptr)
				continue;
			if (view->getViewportType() == Viewport3D)
			{
				updateModelOperate(view->toType<mxr::CCSMRendWidget>(), type, ids);
			}
		}
	}

	void mViewportModel::updateAllGeoModelOperate(mxr::ModelOperateEnum type, std::set<int> ids)
	{
		for (auto view : _viewports)
		{
			if (view == nullptr)
				continue;
			if (view->getViewportType() == Viewport3D)
			{
				updateGeoModelOperate(view->toType<mxr::CCSMRendWidget>(), type, ids);
			}
		}
	}

	void mViewportModel::updateAllMeshModelOperate(mxr::ModelOperateEnum type)
	{
		for (auto view : _viewports)
		{
			if (view == nullptr)
				continue;
			if (view->getViewportType() == Viewport3D)
			{
				updateMeshModelOperate(view->toType<mxr::CCSMRendWidget>(), type);
			}
		}
	}

	int mViewportModel::createFreeNode(QVector3D nodeVertex)
	{
		int id = _modelData->getMeshData()->createNode(nodeVertex);
		updateAllMeshModelOperate(AddOrDeleteTempNode);
		return id;
	}

    void mViewportModel::modelFastCapture(const QString& path, const QString& id)
    {
        auto* preViewport = getPreRenderWidget();
        if (preViewport == nullptr)
            return;
        QDir dir(QCoreApplication::applicationDirPath());
        dir.cdUp();
        QString targetDir = dir.absolutePath() + "/Setting/Recent";
        QDir().mkpath(targetDir);
        QString name = targetDir + "/" + QFileInfo(path).baseName() + id + ".png";
        QVector3D color;
        preViewport->getBackGroundRender()->getPureColor(color);
        float w = preViewport->width();
        float h = preViewport->height();
        preViewport->FastCapture(name, { 0,0 }, { w, h }, color, 1);
    }

    void mViewportModel::setPartType(MxFunctions::PartColorType type)
    {
        if (_colorType == type) return;

        _colorType = type;

        set<int> colorPartID;
        vector<int> partIDs = _modelData->getAllPhyID(Physical_Part);
        if (type == PartColorType::MatColor)
        {
            mMapRelationManager& mapRelationManager = _modelData->getMapRelationManager();
            for (int partID : partIDs)
            {
                mPartData* part = _modelData->getMeshData()->getPartData(partID);
                if (!part) continue;

                int sectionID = mapRelationManager.getMapRelation<int>({ Physical_Part, partID }, Physical_Property);
                if (sectionID == -1) continue;

                int matID = mapRelationManager.getMapRelation<int>({ Physical_Property, sectionID }, Physical_Material);
                if (matID == -1) continue;

                shared_ptr<MechMaterialData::mMaterialBase> mat = _modelData->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(matID);
                if (!mat) continue;

                part->setPartColor(mat->getMatColor());
                colorPartID.insert(partID);
            }
        }
        else if (type == PartColorType::partColor)
        {
            for (int partID : partIDs)
            {
                mPartData* part = _modelData->getMeshData()->getPartData(partID);
                if (!part) continue;

                part->setPartColor(part->getPartInnerColor());
                colorPartID.insert(partID);
            }
        }

        if (colorPartID.empty()) return;
        updateAllModelOperate(ColorOnePart, colorPartID);
        updateRender();
	}

    void mViewportModel::updatePartShowColor(MxFunctions::PartColorType type, vector<int> partIDs)
    {
        // 改色/透明度统一刷新口：内部色同步到显示色后通知渲染重新取色
        //（渲染 ColorOnePart 分支按部件显示色重着色——没有这一步，
        // 改色要等到下一次显隐/重建触发取色才生效）
        if (partIDs.empty())
        {
            partIDs = _modelData->getAllPhyID(Physical_Part);
        }

        set<int> colorPartID;
        if (type == PartColorType::MatColor)
        {
            // 材料色分支保持注释态：注释代码引用的 MechMaterialData
            // 材料管理器 API 形态与当前 BasicData 头不一致（编译不过），
            // 且无用户场景验证——待材料色需求牵引时再对齐现 API 恢复
            //（历史原样保留在版本库注释中）
        }
        else if (type == PartColorType::partColor)
        {
            for (int partID : partIDs)
            {
                mPartData* part = _modelData->getMeshData()->getPartData(partID);
                if (!part) continue;

                part->setPartColor(part->getPartInnerColor());
                colorPartID.insert(partID);
            }
        }

        if (colorPartID.empty()) return;
        updateAllModelOperate(ColorOnePart, colorPartID);
        updateRender();
    }

    void mViewportModel::updatePartShowColorByMat(int matID)
    {
        if (_colorType != PartColorType::MatColor) return;

        shared_ptr<MechMaterialData::mMaterialBase> mat = _modelData->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(matID);
        if (!mat) return;

        set<int> colorPartID;
        mMapRelationManager& mapRelationManager = _modelData->getMapRelationManager();

        unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> matMap = mapRelationManager.getPhysicalFieldMap(Physical_Material, matID);
        for (auto proID : matMap[Physical_Property])
        {
            auto proBase = _modelData->getManager<mPropertyManager>(Physical_Property).getPropertyData(proID);
            if (!proBase || proBase->getPropertyType() == PlyLaminateProperty) continue;

            unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> proMap = mapRelationManager.getPhysicalFieldMap(Physical_Property, proID);
            for (auto partID : proMap[Physical_Part])
            {
                mPartData* part = _modelData->getMeshData()->getPartData(partID);
                if (!part) continue;
                part->setPartColor(mat->getMatColor());
                colorPartID.insert(partID);
            }
        }

        if (colorPartID.empty()) return;
        updateAllModelOperate(ColorOnePart, colorPartID);
        updateRender();
    }

    void mViewportModel::initModelData()
	{
		_modelData = new MechModelData::mModelData();
		QObject::connect(_modelData->getMeshData(), &mMeshData::updateModelOperate, this, [=](modelOperateData data) {
			if (data._partType == 1)
			{
				updateAllGeoModelOperate(mapModelOpreate[data._modelEnum], data._geoIds);
			}
			else if (data._partType == 2)
			{
				updateAllMeshModelOperate(mapModelOpreate[data._modelEnum]);
			}
			else if (data._partType == 3)
			{
				updateAllModelOperate(mapModelOpreate[data._modelEnum], data._geoIds);
			}
		});
        QObject::connect(_modelData->getGeoData(), &MGeoData::GeoDataBase::updateGeoDataSig, this, [=](QString modelEnum, std::set<int> ids, bool updateView) {
            if (modelEnum == "DeleteOnePart")
            {
                //updateGeoModelOperate(_currentPreViewport, DeleteOnePart, ids, updateView);
                updateAllGeoModelOperate(DeleteOnePart, ids);
            }
            else if (modelEnum == "AppendOnePart")
            {
                //updateGeoModelOperate(_currentPreViewport, AppendOnePart, ids, updateView);
                updateAllGeoModelOperate(AppendOnePart, ids);
            }
            else if (modelEnum == "ImportPart")
            {
                //updateAllModelOperate(ImportPart);
                updateAllModelOperate(ImportPart);
            }
            else if (modelEnum == "ReplaceOnePart")
            {
                //updateGeoModelOperate(_currentPreViewport, ReplaceOnePart, ids, updateView);
                updateAllGeoModelOperate(ReplaceOnePart, ids);
            }
            });
		QObject::connect(_modelData->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, [=](UnitType type, QVector<int> idx) {
			if (type & UnitSystem)
			{
				emit mMechGlobalSignals::getInstance()->unitSystemChanged(idx[0]);
			}
			if (type & AngleUnit)
			{
				emit mMechGlobalSignals::getInstance()->angleUnitChanged(idx[1]);
			}
			if (type & SpeedUnit)
			{
				emit mMechGlobalSignals::getInstance()->speedUnitChanged(idx[2]);
			}
			if (type & TempUnit)
			{
				emit mMechGlobalSignals::getInstance()->tempUnitChanged(idx[3]);
			}
			});
	}

	void mViewportModel::deleteSubWindowAndRend(QWidget* subWindow, mViewportPrivate* view)
	{
		//句柄从容器侧 ViewportWidget 脱离，避免容器延迟销毁时连带 delete 本句柄
		if (subWindow)
		{
			subWindow->setParent(nullptr);
		}

		// ---- 关键：彻底关闭 OSG ----
		if (view)
		{
			delete view;
		}

		if (subWindow)
			delete subWindow;
	}

    void mViewportModel::setAxisIntersectPlaneFunc(mxr::SelectService* select_service, mxr::mBaseRend* base_rend, MechModelData::mModelData* model_data)
    {
        select_service->setgetAxisIntersectPlaneFunc([=](float screen_x, float screen_y)->mxr::InteractionPickResult
            {
                InteractionPickResult result;
                auto coord_ids = model_data->getAllPhyID(Physical_Coordinate);
                auto ray = base_rend->getCameraController()->makeRay(screen_x, screen_y);
                float scale = base_rend->getCameraController()->getCameraState().ortho_zoom * 0.1;
                auto scale_vis = (base_rend->getAABBOfView().maxEdge - base_rend->getAABBOfView().minEdge).length();
                HitInfo candidate_hit;
                glm::vec3 normal;
                for (int i = 0; i < coord_ids.size(); ++i)
                {
                    int coord_id = coord_ids[i];
                    if (coord_id == -1) continue;
                    MechCoordinateData::mCoordinateBase* coord = model_data->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(coord_id);
                    bool flag;
                    QString info;
                    QVector3D qorigin;
                    auto qcoords = coord->getXYZAxis(flag, info, qorigin);
                    auto qorigin_test = coord->getOrigin();
                    //std::printf("scale: %f\n", scale);
                    glm::vec3 nx = { qcoords[0].x() * scale, qcoords[0].y() * scale, qcoords[0].z() * scale };
                    glm::vec3 ny = { qcoords[1].x() * scale, qcoords[1].y() * scale, qcoords[1].z() * scale };
                    glm::vec3 nz = { qcoords[2].x() * scale, qcoords[2].y() * scale, qcoords[2].z() * scale };
                    glm::vec3 nx_vis = { qcoords[0].x() * scale_vis, qcoords[0].y() * scale_vis, qcoords[0].z() * scale_vis };
                    glm::vec3 ny_vis = { qcoords[1].x() * scale_vis, qcoords[1].y() * scale_vis, qcoords[1].z() * scale_vis };
                    glm::vec3 nz_vis = { qcoords[2].x() * scale_vis, qcoords[2].y() * scale_vis, qcoords[2].z() * scale_vis };
                    glm::vec3 origin = { qorigin.x(), qorigin.y(), qorigin.z() };

                    auto hit_xy = intersectRayRectangle(ray, origin, nx, ny);
                    auto hit_yz = intersectRayRectangle(ray, origin, ny, nz);
                    auto hit_zx = intersectRayRectangle(ray, origin, nz, nx);
                    // 拿到最近相交
                    if (hit_xy.hit && hit_xy.t < candidate_hit.t)
                    {
                        candidate_hit = hit_xy;
                        candidate_hit.pos = origin;
                        normal = nz;
                        result.u = nx_vis;
                        result.v = ny_vis;
                    }
                    if (hit_yz.hit && hit_yz.t < candidate_hit.t)
                    {
                        candidate_hit = hit_yz;
                        candidate_hit.pos = origin;
                        normal = nx;
                        result.u = ny_vis;
                        result.v = nz_vis;
                    }
                    if (hit_zx.hit && hit_zx.t < candidate_hit.t)
                    {
                        candidate_hit = hit_zx;
                        candidate_hit.pos = origin;
                        normal = ny;
                        result.u = nz_vis;
                        result.v = nx_vis;
                    }
                }
                if (candidate_hit.hit)
                {
                    result.position = candidate_hit.pos;
                    result.normal = normal;
                }
                return result;
            });
    }


    mViewportPrivate::mViewportPrivate(QString viewportName, ViewportType type)
	{
		_viewportName = viewportName;
		_viewportType = type;
		_viewportID = s_nextViewportID++;

		_leftViewCheckedStatus = 0;
		_showViewCheckedStatus = 3;
        _multiplyPickStatus = 0;
	}

    mViewportPrivate::~mViewportPrivate()
	{
		//渲染控件 _rendWidget 为 QPointer：若已随视口控件的 Qt 父子链先行析构则自动置空，
		//所有权随视口控件统一释放（原"先置空再 if(_rendWidget) delete"为恒不可达死代码，
		//勿恢复按 _rendWidget 二次 delete——Qt 父子链已拥有时会双重释放）
		if (_viewport_widget)
		{
			delete _viewport_widget;
		}
		_viewport_widget = nullptr;
	}

	QString mViewportPrivate::getViewportName()
	{
		return _viewportName;
	}

	ViewportType mViewportPrivate::getViewportType()
	{
		return _viewportType;
	}

	void mViewportPrivate::setRendWidget(mxr::mBaseRend* rend)
	{
		_rendWidget = rend;
	}

	void mViewportPrivate::setWidget(IViewportWidget* widget)
	{
		_viewport_widget = widget;
		_rendWidget = _viewport_widget->renderWidget();
	}

	mxr::mBaseRend* mViewportPrivate::getRendWidget()
	{
		return _rendWidget;
	}

	IViewportWidget* mViewportPrivate::getWidget()
	{
		return _viewport_widget;
	}

	QList<mItemKey> mViewportPrivate::getTreeSelectItems()
	{
		return m_SelectedKey;
	}

	void mViewportPrivate::setTreeSelectItems(QList<mItemKey> items)
	{
        m_SelectedKey = items;
	}

	void mViewportPrivate::removeSelectItem(mItemKey item)
	{
		if (m_SelectedKey.contains(item))
		{
            m_SelectedKey.removeOne(item);
		}
	}

	mPreViewportPrivate::mPreViewportPrivate(QString viewportname):mViewportPrivate(viewportname, Viewport3D)
	{
		_sectionMaxID = 0;
		_isShowSectionLine = true;
		_isSectionColor = true;
		_isSectionTransparency = true;
		_isActiveShowMesh = true;

		_pickFilter = 0;//默认智能拾取
	}

	mPreViewportPrivate::~mPreViewportPrivate()
	{

		for (auto section : _sections)
		{
			delete section;
		}
		_sections.clear();
	}

	mPostViewportPrivate::mPostViewportPrivate(QString viewportname, ViewportType type) :mViewportPrivate(viewportname, type)
	{
		_pickFilter = 5;
	}

	mPostViewportPrivate::~mPostViewportPrivate()
	{
		if (_guiData)
		{
			delete _guiData;
			_guiData = nullptr;
		}
	}

	void mPostViewportPrivate::setPostGuiData(mPostGUIData* data)
	{
		_guiData = data;
	}

	mPostGUIData* mPostViewportPrivate::getPostGuiData()
	{
		return _guiData;
	}

	std::set<int> mPostViewportPrivate::getModelPartVisualData()
	{
		return _modelPartHides;
	}

	void mPostViewportPrivate::setModelPartVisualData(int partID, bool visual)
	{
		if (visual)
		{
			_modelPartHides.erase(partID);
		}
		else
		{
			_modelPartHides.insert(partID);
		}
	}

	void mPostViewportPrivate::deleteModelPartVisualData()
	{
		_modelPartHides.clear();
	}

}