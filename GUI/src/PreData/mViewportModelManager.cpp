#include <Windows.h> //【OCC迁移·头链卫生配套】前置避免 byte 歧义（mGlobalSignals 全局 using namespace std 与 rpcndr 的 byte；原经 OCC 头链提前解析，MechModelData 门面瘦身后暴露）
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
#include "mViewportModelManager.h"

#include <algorithm>
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mGlobalSignals.h"
#include "mDataStatusManager.h"
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/SelectionCaculator.h"
#include "mxr/view/SymbolSelectionCalculator.h"
#include "mxr/view/PostSelectionCalculator.h"

#include "PickCommonFunction.h"

//#include "GeoWidget.h" //【OCC摘除·S10】截面形状子窗口下线（见 showPropShapeViewport 注释）
#include "PreViewportWidget.h"
#include "PostViewportWidget.h"
#include "mModelData.h"

#include <QTimer>
#include <QDebug>

using namespace MechCommon;
using namespace MechModelData;
using namespace mxr;

namespace MechPreData
{

class mViewportManagerOperatePrivate : public QObject
{
public:

	mViewportManagerOperatePrivate(mViewportModelManager* manager): _manager(manager)
	{
	}

    void slot_finishPick(FinishPickInfo info)
    {
		if (!getCVM)
		{
			return;
		}
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		//需判断前处理还是后处理
		if (viewportPrivate->getViewportType() == Viewport3D)
		{
			auto widget_ptr = _manager->getCurrentViewportModel()->getPreRenderWidget();
			ProcessPickCaculateResult(info,widget_ptr, viewportPrivate->getViewportType());

		}
		else if (viewportPrivate->getViewportType() == ViewportPost3D)
		{
			mxr::mBaseRend* widget_ptr = dynamic_cast<mxr::mBaseRend*>(_manager->getCurrentViewportModel()->getPostRenderWidget());
			ProcessPickCaculateResult(info, widget_ptr, viewportPrivate->getViewportType());
		}
    }

private:
	mViewportModelManager* _manager;
};
}

namespace MechPreData
{
	mViewportModelManager* mViewportModelManager::_instance = nullptr;
	bool mViewportModelManager::_headlessMode = false;

	void mViewportModelManager::setHeadlessMode(bool headless)
	{
		_headlessMode = headless;
	}

	bool mViewportModelManager::isHeadlessMode()
	{
		return _headlessMode;
	}

    mViewportModelManager::mViewportModelManager()
    {
        connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::MeshSutureSig, this, [this](bool b, set<int> c)->void{
            this->slot_MeshSuture(b, c);
        });
		_currentViewportModel = new mViewportModel;
		_viewportModel.insert(_currentViewportModel->getModelData()->getModelDataID(), _currentViewportModel);
		//模型数据创建后立即命名（模型树与视口命名的唯一来源）
		_currentViewportModel->getModelData()->setModelDataName(makeModelName(false));
		QObject::connect(_currentViewportModel, &mViewportModel::sig_viewportRenamed, this, [this](int id, const QString& oldName, const QString& n) {
					emit sig_viewportRenamed(id, oldName, n);
			});
		if (!_headlessMode)
		{
			newViewport();
		}

		//新建截面形状视口
		_propShapeWidget.first = nullptr;
		_propShapeWidget.second = nullptr;
		this->getPropShapeWidget();
		hidePropShapeViewport();//默认隐藏

		_op_private = std::make_shared<mViewportManagerOperatePrivate>(this);
	}

	mViewportModelManager::~mViewportModelManager()
	{
		shutdownTrace("[vmm] dtor begin");
		for (mViewportModel* it : _viewportModel)
		{
			if (it != nullptr)
			{
				delete it;
				it = nullptr;
			}
		}

		shutdownTrace("[vmm] all viewport models deleted");
		_currentViewportModel = nullptr;
		_viewportModel.clear();
	}

    void mViewportModelManager::setCameraSyncTimer(int time, int n, std::function<void()> task)
    {
        if (n <= 0) return;

        // 计算间隔 (注意：使用传入的 time，而不是硬编码的 500)
        int interval = time / n;

        // 创建定时器，挂载在 this 上防止内存泄漏
        // 不建议在这里用 shared_ptr 局部变量，因为它出了函数作用域就会销毁定时器
        QTimer* timer = new QTimer(this);

        // 初始化计数器
        int currentCount = 0;

        // 连接信号
        // [=] : 按值捕获所有变量 (包括 timer 指针, task, n, currentCount)
        // mutable : 允许修改捕获的按值拷贝的变量 (修改 currentCount)
        connect(timer, &QTimer::timeout, this, [=]() mutable {
            if (currentCount < n) {
                // --- 执行传入的逻辑 ---
                if (task) {
                    task();
                }
                // --------------------
                currentCount++;
            }

            // 检查是否完成
            if (currentCount >= n) {
                timer->stop();
                timer->deleteLater(); // 任务完成后，让定时器自杀，释放内存
            }
            });

        timer->start(interval);
    }

	mViewportModelManager* mViewportModelManager::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mViewportModelManager;
		}

		return _instance;
	}

	void mViewportModelManager::setCurrentViewportModel(int index)
	{
		if (!_viewportModel.contains(index))
		{
			return;
		}
		if (_viewportModel.value(index) == _currentViewportModel)
		{
			return;//同模型重复设置无实际切换，不通知
		}
		//切换当前模型前通知（目标视口未知传-1）：激活中的拾取控件按当前（旧）模型
		//先行提交。本函数不经过 slot_viewportActivated，若不在此通知，
		//后续激活流程到达时当前模型已切换，提交会写入新模型
		emit mMechGlobalSignals::getInstance()->viewportModelAboutToChange(-1);
		_currentViewportModel = _viewportModel.value(index);
	}

	void mViewportModelManager::forceSetCurrentViewportModel(int index)
	{
		if (!_viewportModel.contains(index))
		{
			return;
		}
		//同模型也强制赋值：调用方需要保证 _currentViewportModel 明确指向该模型
		_currentViewportModel = _viewportModel.value(index);
	}

	mViewportModel* mViewportModelManager::getCurrentViewportModel()
	{
		return _currentViewportModel;
	}

	mViewportModel* mViewportModelManager::getViewportModel(int modelDataID)
	{
		return _viewportModel.value(modelDataID, nullptr);
	}

	mViewportPrivate* mViewportModelManager::getCurrentViewportPrivate()
	{
		if (!_currentViewportModel || _currentViewportID == -1)
		{
			return nullptr;
		}
		auto* sub = _currentViewportModel->getSubWindowByViewportID(_currentViewportID);
		return sub ? _currentViewportModel->getViewportPrivate(sub) : nullptr;
	}

	QWidget* mViewportModelManager::getPreCurrentWindow()
	{
		if (!_currentViewportModel || _currentViewportID == -1)
		{
			return nullptr;
		}
		return _currentViewportModel->getSubWindowByViewportID(_currentViewportID);
	}

	mViewportPrivate* mViewportModelManager::getViewportPrivateByID(int viewportID, int* modelID)
	{
		if (modelID) *modelID = -1;
		QHashIterator<int, mViewportModel*> iter(_viewportModel);
		while (iter.hasNext())
		{
			iter.next();
			auto model = iter.value();
			if (!model)
			{
				continue;
			}
			auto sub = model->getSubWindowByViewportID(viewportID);
			if (!sub)
			{
				continue;
			}
			auto priv = model->getViewportPrivate(sub);
			if (priv)
			{
				if (modelID) *modelID = iter.key();
				return priv;
			}
		}
		return nullptr;
	}

	QWidget* mViewportModelManager::getSubWindowByViewportID(int viewportID)
	{
		for (auto model : _viewportModel)
		{
			if (!model)
			{
				continue;
			}
			auto sub = model->getSubWindowByViewportID(viewportID);
			if (sub)
			{
				return sub;
			}
		}
		return nullptr;
	}

	void mViewportModelManager::slot_MeshSuture(bool isSuppress, std::set<int> setEdgeIds)
	{
		// TODO: 根据 isSuppress 与 geoIds 更新抑制几何边的视口渲染（虚线/实线）
	}

	void mViewportModelManager::deleteAllViewportModel()
	{
        //删除全部模型前通知：需按当前模型落盘的接收者（如激活中的通用拾取控件）
        //先完成提交，随模型一并销毁，避免数据带入后续重建的模型
        emit mMechGlobalSignals::getInstance()->viewportModelAboutToChange(-1);

        QList<int> removedIDs;
        QHashIterator<int, mViewportModel*> iter(_viewportModel);
        while (iter.hasNext())
        {
            iter.next();
            auto viewportModel = _viewportModel[iter.key()];
            removedIDs.append(viewportModel->getAllViewportIDs());
            viewportModel->deleteAllViewportModel();
            delete viewportModel;
            viewportModel = nullptr;
        }

        _viewportModel.clear();
        // 所有模型已销毁，当前模型指针必须同步清空，避免 getCVM 返回悬空指针
        _currentViewportModel = nullptr;

        for (int id : removedIDs)
        {
			if (_containerRemoveFunc)
			{
				_containerRemoveFunc(id);
			}
            emit sig_viewportRemoved(id);
        }
	}
	void mViewportModelManager::newViewport(bool fullScreen)
	{
		if (_currentViewportModel == nullptr)
		{
			return;
		}
		auto subWindow = _currentViewportModel->addViewport();
		_currentViewportModel->setPreRenderWidget(subWindow);
		//登记到容器（内容部件加入容器并显示）
		auto addPriv = _currentViewportModel->getViewportPrivate(subWindow);
		if (addPriv)
		{
			if (_containerAddFunc)
			{
				_containerAddFunc(addPriv->getViewportID(), addPriv->getWidget() ? addPriv->getWidget()->asWidget() : nullptr);
			}
			//容器标题栏初始为占位名（视口-ID），同步真实名称（视口-N / 树名前缀版）
			syncViewportTitleToContainer(addPriv->getViewportID(), addPriv->getViewportName());
			emit sig_viewportAdded(addPriv->getViewportID());
			//新建视口后激活它：确保当前视口指向新视口，面板刷新到新视口。
			//构造期（容器未初始化）跳过激活：此时 _containerSetActiveFunc 为空，
			//activateViewportByID 会走 slot_viewportActivated 兜底，访问未就绪的容器状态
			if (_containerSetActiveFunc)
			{
				activateViewportByID(addPriv->getViewportID());
			}
		}
	}

	void mViewportModelManager::newViewport(MxFunctions::ViewportType type, QString name)
	{
		if (_currentViewportModel == nullptr)
		{
			return;
		}
		auto subWindow = _currentViewportModel->addViewport(type, name);
		_currentViewportModel->setPreRenderWidget(subWindow);
		if (type == ViewportPost3D)
		{
			QObject::connect(_currentViewportModel->getPostRenderWidget()->getSelectionCaculator(), &PostSelectionCalculator::signals_FinishPick, _op_private.get(), &mViewportManagerOperatePrivate::slot_finishPick);
		}
		auto addPriv = _currentViewportModel->getViewportPrivate(subWindow);
		if (addPriv)
		{
			if (_containerAddFunc)
			{
				_containerAddFunc(addPriv->getViewportID(), addPriv->getWidget() ? addPriv->getWidget()->asWidget() : nullptr);
			}
			//容器标题栏初始为占位名（视口-ID），同步真实名称
			syncViewportTitleToContainer(addPriv->getViewportID(), addPriv->getViewportName());
			emit sig_viewportAdded(addPriv->getViewportID());
			if (_containerSetActiveFunc)
			{
				activateViewportByID(addPriv->getViewportID());
			}
		}
	}

	int mViewportModelManager::newViewportModel(bool fullScreen)
	{
		//切换前通知：目标视口未知传-1，激活中的拾取控件按当前（旧）模型先行提交
		emit mMechGlobalSignals::getInstance()->viewportModelAboutToChange(-1);
		_currentViewportModel = new mViewportModel;
		_viewportModel.insert(_currentViewportModel->getModelData()->getModelDataID(), _currentViewportModel);
		//模型数据创建后立即命名（模型树与视口命名的唯一来源）
		_currentViewportModel->getModelData()->setModelDataName(makeModelName(false));
		QObject::connect(_currentViewportModel, &mViewportModel::sig_viewportRenamed, this, [this](int id, const QString& oldName, const QString& n) {
					emit sig_viewportRenamed(id, oldName, n);
			});

		newViewport(fullScreen);
		return _currentViewportModel->getModelData()->getModelDataID();
	}

	int mViewportModelManager::newPostViewportModel()
	{
		//切换前通知：目标视口未知传-1，激活中的拾取控件按当前（旧）模型先行提交
		emit mMechGlobalSignals::getInstance()->viewportModelAboutToChange(-1);
		_currentViewportModel = new mViewportModel(true);
		_viewportModel.insert(_currentViewportModel->getModelData()->getModelDataID(), _currentViewportModel);
		//模型数据创建后立即命名（模型树与视口命名的唯一来源）
		_currentViewportModel->getModelData()->setModelDataName(makeModelName(true));
		QObject::connect(_currentViewportModel, &mViewportModel::sig_viewportRenamed, this, [this](int id, const QString& oldName, const QString& n) {
					emit sig_viewportRenamed(id, oldName, n);
			});

		QString viewName = QString("Post_Cloud%1").arg(_currentViewportModel->getModelData()->getModelDataID());
		newViewport(ViewportPost3D, viewName);
		return _currentViewportModel->getModelData()->getModelDataID();
	}

	int mViewportModelManager::clearViewportModel(int viewportModelId)
	{
		auto viewportModel = _viewportModel.value(viewportModelId, nullptr);
		if (!viewportModel || !viewportModel->getModelData())
		{
			return -1;
		}

		//清空即"按原名称就地重建"，采用先建后删：
		//全程保证系统至少存在一个模型和视口，"无视口时兜底补建模型"的
		//逻辑不会被触发；删除旧模型时当前模型已指向新模型
		//（wasCurrentModel=false），也不会发出 currentViewportChanged(-1)
		const QString modelName = viewportModel->getModelData()->getModelDataName();

		//不走 newViewportModel()：需在建视口前恢复名称，使视口命名直接取到
		//原模型名前缀（"模型-N· 视口-1"），跳过占位名回填环节
		//切换前通知：目标视口未知传-1，激活中的拾取控件按当前（旧）模型先行提交，
		//避免残留拾取写入重建后的新模型
		emit mMechGlobalSignals::getInstance()->viewportModelAboutToChange(-1);
		_currentViewportModel = new mViewportModel;
		_viewportModel.insert(_currentViewportModel->getModelData()->getModelDataID(), _currentViewportModel);
		_currentViewportModel->getModelData()->setModelDataName(modelName);
		QObject::connect(_currentViewportModel, &mViewportModel::sig_viewportRenamed, this, [this](int id, const QString& oldName, const QString& n) {
					emit sig_viewportRenamed(id, oldName, n);
			});
		if (!_headlessMode)
		{
			newViewport();
		}
		const int newModelID = _currentViewportModel->getModelData()->getModelDataID();

		deleteViewportModel(viewportModelId);
		return newModelID;
	}

	QString mViewportModelManager::makeModelName(bool isPost)
	{
		//序号取同类模型名称中的最大后缀+1，避免删除中间模型后计数回退产生重名
		const QString tag = isPost ? QString::fromUtf8("后处理模型-") : QString::fromUtf8("模型-");
		int maxIndex = 0;
		QHashIterator<int, mViewportModel*> iter(_viewportModel);
		while (iter.hasNext())
		{
			iter.next();
			if (!iter.value() || iter.value()->isPostModel() != isPost) continue;
			auto modelData = iter.value()->getModelData();
			if (!modelData) continue;
			const QString name = modelData->getModelDataName();
			if (name.startsWith(tag))
			{
				const int index = name.mid(tag.size()).toInt();
				if (index > maxIndex)
				{
					maxIndex = index;
				}
			}
		}
		return tag + QString::number(maxIndex + 1);
	}

	bool mViewportModelManager::isExistModelDataName(const QString& name, int excludeModelDataID)
	{
		QHashIterator<int, mViewportModel*> iter(_viewportModel);
		while (iter.hasNext())
		{
			iter.next();
			if (!iter.value() || iter.key() == excludeModelDataID) continue;
			auto modelData = iter.value()->getModelData();
			if (modelData && modelData->getModelDataName() == name)
			{
				return true;
			}
		}
		return false;
	}

	void mViewportModelManager::deleteViewportModel(int viewportModelId)
	{
        if (!_viewportModel.contains(viewportModelId))
        {
            return;
        }

        //持有该模型内数据指针的模块（如求解任务中心）先行清理
        emit mMechGlobalSignals::getInstance()->viewportModelAboutToBeDeleted(viewportModelId);

        //是否删除当前模型：决定删除完成后是否需要重新激活
        const bool wasCurrentModel = _currentViewportModel
            && _currentViewportModel->getModelData()->getModelDataID() == viewportModelId;

        //删除当前模型前通知：激活中的拾取控件按当前（待删除）模型先行提交，
        //数据随模型一并销毁；避免删除后的激活转移把数据带入其他模型
        if (wasCurrentModel)
        {
            emit mMechGlobalSignals::getInstance()->viewportModelAboutToChange(-1);
        }

        auto viewportModel = _viewportModel[viewportModelId];
        const QList<int> removedIDs = viewportModel->getAllViewportIDs();

        //批量删除流程：期间视图树收到 sig_viewportRemoved 只移除节点/重排，
        //不做激活转移（避免每个视口删除各触发一次激活切换，且中途状态无意义）
        _batchRemovingViewports = true;

        //逐个复用删除视口的清理流程：断开联动连接（_connects 键为 mViewportPrivate*，
        //直接物理删除会留悬空指针）、容器移除、发移除信号。
        //注意绕过 mViewportModel::deleteViewport 的"最后一个视口"保护——
        //删除模型语义就是清空全部视口
        for (int id : removedIDs)
        {
            auto* sub = viewportModel->getSubWindowByViewportID(id);
            auto* priv = sub ? viewportModel->getViewportPrivate(sub) : nullptr;

            //断开视口联动连接
            if (priv)
            {
                auto iter = _connects.find(priv);
                if (iter != _connects.end())
                {
                    for (auto conn : iter.value())
                    {
                        disconnect(conn);
                    }
                    _connects.erase(iter);
                }
            }

            //模型内物理删除（不走"最后一个视口"保护）
            if (sub && priv)
            {
                viewportModel->removeViewportRaw(sub);
            }

            if (_containerRemoveFunc)
            {
                _containerRemoveFunc(id);
            }
            emit sig_viewportRemoved(id);
        }

        _batchRemovingViewports = false;

        //删除模型对象
        viewportModel->deleteAllViewportModel();
        delete viewportModel;
        viewportModel = nullptr;
        _viewportModel.remove(viewportModelId);

        if (wasCurrentModel)
        {
            //复位当前状态：激活目标的选择（页首节点→跨页查找）是视图树的职责，
            //通知树从删除后的干净状态统一决定新的激活视口
            _currentViewportModel = nullptr;
            _currentViewportID = -1;
            emit currentViewportChanged(-1);
        }

        //删除期间容器的激活转移被批量标志屏蔽，补一次轴标定位模式同步
        slot_syncViewportAxisActive();
	}

	void mViewportModelManager::deleteViewport(int removedViewportID)
	{
		//先按视口ID定位所属模型，再从该模型删除——不能用当前模型：
		//调用方（如视口树右键删除非激活视口）传入的视口可能属于其他模型
		int modelID = -1;
		auto viewportPrivate = getViewportPrivateByID(removedViewportID, &modelID);
		if (!viewportPrivate || modelID == -1 || !_viewportModel.contains(modelID))
		{
			return;
		}
		auto* targetModel = _viewportModel.value(modelID);
		auto* temp = targetModel->getSubWindowByViewportID(removedViewportID);
		const bool isCurrentViewport = (removedViewportID == _currentViewportID);

        //断开视口联动连接（仅删除当前视口时需要：删除转移后由新激活视口重建连接）
        if (isCurrentViewport)
        {
            QHashIterator<mViewportPrivate*, QList<QMetaObject::Connection>> iter(_connects);
            while (iter.hasNext())
            {
                iter.next();
                if (iter.key() == viewportPrivate)
                {
                    for (auto conn : iter.value())
                    {
                        disconnect(conn);
                    }
                    _connects.remove(iter.key());
                    break;
                }
            }
        }

		bool isSuccess = targetModel->deleteViewport(temp);
		if (isSuccess)
		{
			if (removedViewportID > 0)
			{
				if (_containerRemoveFunc)
				{
					_containerRemoveFunc(removedViewportID);
				}
				emit sig_viewportRemoved(removedViewportID);
			}
		}

		//删除激活视口时容器静默转移激活（不发 sig_activeViewportChanged），
		//补一次轴标定位模式同步
		slot_syncViewportAxisActive();
	}

	void mViewportModelManager::deletePostViewport(QString name, int viewportModelId)
	{
		mViewportModel* viewportModel = nullptr;
		if (viewportModelId == -1)
		{
			viewportModel = _currentViewportModel;
		}
		else if (_viewportModel.contains(viewportModelId))
		{
			viewportModel = _viewportModel[viewportModelId];
		}
		if (!viewportModel)
		{
			return;
		}

		auto subWindow = viewportModel->getViewport(name);
		auto priv = subWindow ? viewportModel->getViewportPrivate(subWindow) : nullptr;
		int removedID = priv ? priv->getViewportID() : -1;
		bool deleted = viewportModel->deleteViewport(name);
		if (deleted && removedID > 0)
		{
			if (_containerRemoveFunc)
			{
				_containerRemoveFunc(removedID);
			}
			emit sig_viewportRemoved(removedID);
		}

		//删除后补一次轴标定位模式同步（激活转移不发信号）
		slot_syncViewportAxisActive();
	}

	void mViewportModelManager::deletePostViewport(int viewportModelId)
	{
		if (!_viewportModel.contains(viewportModelId))
		{
			return;
		}
		auto viewportModel = _viewportModel[viewportModelId];
		auto subWindows = viewportModel->getSubWindows();
		for (auto subWindow : subWindows)
		{
			auto priv = viewportModel->getViewportPrivate(subWindow);
			int removedID = priv ? priv->getViewportID() : -1;
			//删除成功才发移除信号（如最后一个前处理视口会被守卫拦下，不误删视图树节点）
			if (viewportModel->deleteViewport(subWindow) && removedID > 0)
			{
				if (_containerRemoveFunc)
				{
					_containerRemoveFunc(removedID);
				}
				emit sig_viewportRemoved(removedID);
			}
		}

		//删除后补一次轴标定位模式同步（激活转移不发信号）
		slot_syncViewportAxisActive();
	}

	void mViewportModelManager::activatePreViewport()
	{
		if (!_currentViewportModel)
		{
			return;
		}
		auto subWindow = _currentViewportModel->activateViewport();
		if (subWindow)
		{
			auto priv = _currentViewportModel->getViewportPrivate(subWindow);
			if (priv)
			{
				activateViewportByID(priv->getViewportID());
			}
		}
	}

	void mViewportModelManager::activatePostViewport(QString name)
	{
		if (!_currentViewportModel)
		{
			return;
		}

		QWidget* subWindow = nullptr;
		if (name == "")
		{
			subWindow = _currentViewportModel->activatePostViewport();
		}
		else
		{
			subWindow = _currentViewportModel->getViewport(name);
		}

		if (!subWindow)
		{
			return;
		}
		auto priv = _currentViewportModel->getViewportPrivate(subWindow);
		if (priv)
		{
			activateViewportByID(priv->getViewportID());
		}
	}

	void mViewportModelManager::tileAllViewportModel(bool isAll)
	{
		//新架构由页布局接管，平铺/层叠不再手动操作子窗口几何
		Q_UNUSED(isAll);
	}

	void mViewportModelManager::cascadeAllViewportModel(bool isAll)
	{
		Q_UNUSED(isAll);
	}

	void mViewportModelManager::tileViewportModel(bool isAll)
	{
		Q_UNUSED(isAll);
	}

	void mViewportModelManager::cascadeViewportModel(bool isAll)
	{
		Q_UNUSED(isAll);
	}

	void mViewportModelManager::setViewportSync(bool isSync)
	{
		if (_viewportSynced == isSync)
		{
			return;//同状态重复调用：避免相机同步连接叠加
		}
		_viewportSynced = isSync;

		QList<mViewportPrivate*> viewportPrivates;
		QHashIterator<int, mViewportModel*> iter(_viewportModel);
		while (iter.hasNext())
		{
			iter.next();
			viewportPrivates.append(iter.value()->getRendViewportPrivates());
            //添加后处理视口
            viewportPrivates.append(iter.value()->getRendViewportPrivates(false, MxFunctions::ViewportPost3D));
		}
		if (isSync)
		{
			for (auto viewportPrivate : viewportPrivates)
			{
				auto rend = viewportPrivate->getRendWidget();
				for (auto viewportPrivate1 : viewportPrivates)
				{
					auto rend1 = viewportPrivate1->getRendWidget();
					if (viewportPrivate == viewportPrivate1)
					{
						continue;
					}
					auto conn = connect(rend, &mBaseRend::signal_CameraSync, this, [rend, rend1]() {
						rend1->getCameraController()->setCameraState(rend->getCameraController()->getCameraState());
                        rend1->getCameraController()->resize(rend1->width(), rend1->height());
						rend1->getCameraController()->applyStateToCamera();
						rend1->getCameraController()->update();
						rend1->updatePaint();
						});
					_connects[viewportPrivate1].append(conn);
				}
			}
		}
		else
		{
			for (auto connects : _connects)
			{
				for (auto conn : connects)
				{
					disconnect(conn);
				}
			}
			_connects.clear();
		}
	}

	int mViewportModelManager::getViewportModelSize()
	{
		int count = 0;
		QHashIterator<int, mViewportModel*> iter(_viewportModel);
		while (iter.hasNext())
		{
			iter.next();
			if (!iter.value()->isPostModel())
				count = count + 1;
		}
		return count;
	}

	int mViewportModelManager::getBeginModelID()
	{
		if (_viewportModel.size() == 0)
			return -1;

		int minIndex = std::numeric_limits<int>::max();
		QHashIterator<int, mViewportModel*> iter(_viewportModel);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value()->isPostModel())
			{
				continue;
			}
			if (minIndex > iter.key())
				minIndex = iter.key();
		}

		return minIndex;
	}

	void mViewportModelManager::showPropShapeViewport()
	{
		//【OCC摘除·S10】截面形状子窗口整体下线：其载体 GeoWidget 为 OCC 可视化栈
		//（V3d/AIS/OpenGl viewer），且自创建后无任何数据喂入（空窗口，源注释
		// "李思辰需要修改为 beam 窗口"系未完成 TODO）。严格执行"仅 RebuildGeometry
		// 依赖 OCC"约束，随 GeoWidget 出列构建（见 PreData/CMakeLists.txt）。
		// 后续截面形状显示需求应在 Render 库 OSG 管线上实现，届时本函数重写。
		/*
		if (_propShapeWidget.first == nullptr)
		{
			auto propShapeW = new MxBeam::GeoWidget();
			auto subWindow = this->addSubWindow(propShapeW);
			_propShapeWidget.first = propShapeW;
			_propShapeWidget.second = subWindow;
			subWindow->setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::SubWindow);
		}
		_propShapeWidget.second->showMaximized();
		*/
	}

	void mViewportModelManager::hidePropShapeViewport()
	{
		//【OCC摘除·S10】保壳：截面形状子窗口下线后 _propShapeWidget 不再创建
		if (_propShapeWidget.first != nullptr)
		{
			_propShapeWidget.second->hide();
		}
	}

	QWidget* mViewportModelManager::getPropShapeWidget()
	{
		//【OCC摘除·S10】随截面形状子窗口下线（GeoWidget=OCC viewer，零调用者），
		// 保壳返回空；详见 showPropShapeViewport 注释。
		return nullptr;
		/*
		if (_propShapeWidget.first == nullptr)
		{
			auto propShapeW = new MxBeam::GeoWidget();
			_propShapeWidget.first = propShapeW;
		}
		return _propShapeWidget.first;
		*/
	}

	void mViewportModelManager::slot_viewportActivated(int viewportID)
	{
		//切换前通知：本函数稍后会先更新当前模型/视口指针、再发 currentViewportChanged，
		//届时（树选中→unEdit 链路）触发的拾取提交将以新模型 getCVM 落盘。
		//激活中的通用拾取控件必须在此信号中（旧模型仍为当前模型时）完成提交
		emit mMechGlobalSignals::getInstance()->viewportModelAboutToChange(viewportID);

		//关闭创建平面模式
		//注意：删除视口/视图页后，getCVM 可能非空但其渲染组件已销毁，必须逐级判空。
		//getPreRenderWidget/getPreViewportWidget 按当前视口ID从模型派生，
		//当前视口无效时返回空，判空可挡住悬空访问
		if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreViewportWidget())
		{
			auto* preViewport = getCVM->getPreViewportWidget();
			if (preViewport->interaction() && preViewport->interaction()->getToolManager())
			{
				auto* toolManager = preViewport->interaction()->getToolManager();
				if (toolManager->active() != nullptr &&
					toolManager->active()->type() == mxr::InteractionToolType::CreateSketchPlane)
				{
					toolManager->setActive(mxr::InteractionToolType::None);
				}
			}
		}

		disconnectSignal();
		const int prevViewportID = _currentViewportID;
		_currentViewportID = viewportID;

		//按 viewportID 找到所属模型，恢复当前模型/视口
		int modelID = -1;
		auto priv = getViewportPrivateByID(viewportID, &modelID);
		if (priv && modelID != -1 && _viewportModel.contains(modelID))
		{
			_currentViewportModel = _viewportModel.value(modelID);
			_currentViewportModel->setPreRenderWidget(
				_currentViewportModel->getSubWindowByViewportID(viewportID));
			connectSignal();
		}
		//视口ID无效（已删除/取消激活 -1）：保留当前模型指针供调用方判空后优雅降级，
		//派生查询会因 ID 无效自然返回空，面板已有判空兜底

		// 内部状态更新完成后再通知外部，确保外部接收者拿到的是最新的当前模型/视口。
		// 目标视口与当前视口一致时不发信号（删除视口的转移激活场景：
		// 先取消激活(-1)再激活回原视口时，中间态不应触发面板刷新）
		if (viewportID != prevViewportID)
		{
			emit currentViewportChanged(viewportID);
        }
	}

	void mViewportModelManager::slot_syncViewportAxisActive()
	{
		// 按容器当前激活视口做幂等全量同步：点击切换/新建视口由
		// sig_activeViewportChanged 驱动，删除视口后的静默激活转移由各
		// 删除流程末尾补调——否则新激活视口的轴标会残留"贴边"态。
		// 激活ID经查询回调取（PreData 不依赖容器具体类型）；回调未注册
		//（容器未初始化）时退用管理器自身的当前视口ID
		const int activeID = _containerActiveViewportIdFunc
			? _containerActiveViewportIdFunc() : _currentViewportID;

		QHashIterator<int, mViewportModel*> iter(_viewportModel);
		while (iter.hasNext())
		{
			iter.next();
			auto* model = iter.value();
			if (!model)
			{
				continue;
			}
			const QList<int> viewids = model->getAllViewportIDs();
			for (int id : viewids)
			{
				auto* priv = getViewportPrivateByID(id);
				auto* rend = priv ? priv->getRendWidget() : nullptr;
				const auto& axis = rend ? rend->GetAxisWindow() : nullptr;
				if (axis)
				{
					axis->SetViewportActive(id == activeID);
				}
			}
		}
	}

    void mViewportModelManager::connectSignal()
    {
		if (_currentViewportModel == nullptr)
			return;
		if (!_currentViewportModel->getPreRenderWidget())
			return;
        QObject::connect(_currentViewportModel->getPreRenderWidget(), &CCSMRendWidget::signal_osgWidgetEvent, mMechGlobalSignals::getInstance(), &mMechGlobalSignals::signal_osgWidgetEvent);

		QObject::connect(_currentViewportModel->getPreRenderWidget()->getSelectionCaculator(), &SelectionCaculator::signals_FinishPick, _op_private.get(), &mViewportManagerOperatePrivate::slot_finishPick);
		QObject::connect(_currentViewportModel->getPreRenderWidget()->GetSymbolCalculator(), &SymbolSelectionCalculator::signals_FinishPick, _op_private.get(), &mViewportManagerOperatePrivate::slot_finishPick);
    }

    void mViewportModelManager::disconnectSignal()
    {
		if (_currentViewportModel == nullptr)
			return;
		if (!_currentViewportModel->getPreRenderWidget())
			return;

        QObject::disconnect(_currentViewportModel->getPreRenderWidget(), &CCSMRendWidget::signal_osgWidgetEvent, mMechGlobalSignals::getInstance(), &mMechGlobalSignals::signal_osgWidgetEvent);

		QObject::disconnect(_currentViewportModel->getPreRenderWidget()->getSelectionCaculator(), &SelectionCaculator::signals_FinishPick, _op_private.get(), &mViewportManagerOperatePrivate::slot_finishPick);
        QObject::disconnect(_currentViewportModel->getPreRenderWidget()->GetSymbolCalculator(), &SymbolSelectionCalculator::signals_FinishPick, _op_private.get(), &mViewportManagerOperatePrivate::slot_finishPick);
    }
	std::vector<int> mViewportModelManager::getAllModelDataID()
	{
		vector<int> ids;
		for (auto it = _viewportModel.begin(); it != _viewportModel.end(); ++it)
			ids.push_back(it.key());

		return ids;
	}
	bool mViewportModelManager::isPostModel(int modelDataID)
	{
        if (_viewportModel.constFind(modelDataID) != _viewportModel.cend())
            return _viewportModel[modelDataID]->isPostModel();

        return false;
	}

	/*************************** 新视口容器架构 ***************************/

	void mViewportModelManager::initViewportContainer()
	{
		if (_headlessMode)
		{
			return;
		}
		if (_viewportContainer != nullptr)
		{
			return; //幂等
		}
		//容器本体由 MainWindow 侧通过注册函数创建并注入
		if (_containerCreateFunc)
		{
			_viewportContainer = _containerCreateFunc();
		}
	}

	void mViewportModelManager::setViewportContainerCallbacks(
		std::function<void(int, QWidget*)> addFunc,
		std::function<void(int)> removeFunc,
		std::function<void(int)> setActiveFunc)
	{
		_containerAddFunc = std::move(addFunc);
		_containerRemoveFunc = std::move(removeFunc);
		_containerSetActiveFunc = std::move(setActiveFunc);
	}

	void mViewportModelManager::setViewportContainerCreateFunc(std::function<MechMainWindow::ViewportContainer*()> createFunc)
	{
		_containerCreateFunc = std::move(createFunc);
	}

	void mViewportModelManager::setViewportContainerQueryCallbacks(
		std::function<bool(int)> visibleFunc,
		std::function<void(int, const QString&)> setTitleFunc,
		std::function<int()> activeViewportIdFunc)
	{
		_containerVisibleFunc = std::move(visibleFunc);
		_containerSetTitleFunc = std::move(setTitleFunc);
		_containerActiveViewportIdFunc = std::move(activeViewportIdFunc);
	}

	bool mViewportModelManager::isViewportVisibleInContainer(int viewportID) const
	{
		if (_containerVisibleFunc)
		{
			return _containerVisibleFunc(viewportID);
		}
		return true;
	}

	void mViewportModelManager::syncViewportTitleToContainer(int viewportID, const QString& name)
	{
		if (_containerSetTitleFunc)
		{
			_containerSetTitleFunc(viewportID, name);
		}
	}

	void mViewportModelManager::activateViewportByID(int viewportID)
	{
		if (_containerSetActiveFunc)
		{
			_containerSetActiveFunc(viewportID);
		}
		else
		{
			//容器未初始化时直接走激活逻辑（headless 兜底）
			slot_viewportActivated(viewportID);
		}
	}

	void mViewportModelManager::refreshViewportRender(int viewportID)
	{
		auto* priv = getViewportPrivateByID(viewportID);
		if (priv && priv->getRendWidget())
		{
			//updatePaint = 同步场景状态 + 请求出帧：部件已可见，update 不再被丢弃
			priv->getRendWidget()->updatePaint();
		}
	}

}