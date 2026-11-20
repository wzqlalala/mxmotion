#include "mPostSessionCoordinator.h"
#include "mPostSessionProvider.h"

//Qt
#include <QTimer>
#include <algorithm>

//PostWindow
#include "mPostBrowserWindow.h"
#include "DerivedCase/mPostDerivedCaseStore.h"
#include "mPostInfoHub.h"

//Common
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"

//PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mViewTreeFunction.h"

//BasicData
#include "mModelData.h"
#include "mPostDataManager.h"

using namespace MxFunctions;
using namespace MechPreData;

namespace PostWindow
{
	mPostSessionCoordinator* mPostSessionCoordinator::_instance = nullptr;

	mPostSessionCoordinator* mPostSessionCoordinator::getInstance()
	{
		if (_instance == nullptr)
			_instance = new mPostSessionCoordinator;
		return _instance;
	}

	mPostSessionCoordinator::mPostSessionCoordinator(QObject* parent)
		: QObject(parent)
	{
		//自注册（mPostBrowserWindow 构造最先处实例化本单例，保证本连接先于其
		//对 updateResultTreeSig 的连接：导入完成时回放先运行，见头注释）
		MechCommon::postSessionParticipantRef() = this;
		connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::updateResultTreeSig,
			this, &mPostSessionCoordinator::onImportFinished);
		//导入失败通知（读线程失败/结果库无效）：与完成信号对称，失败时跳过
		//该条继续推进，否则 _importing 永不复位、恢复收尾与保存/打开会话全部卡死
		connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::postResultImportFailedSig,
			this, &mPostSessionCoordinator::onImportFailed);
	}

	void mPostSessionCoordinator::collectPostSessionEntry(int modelID,
		QStringList& resultFiles, QByteArray& postItemsChunk)
	{
		auto* vm = mViewportModelManager::getInstance()->getViewportModel(modelID);
		if (vm == nullptr || !vm->isPostModel() || vm->getModelData() == nullptr)
			return;
		//原始导入清单（优先注册表记录，见 mPostBrowserWindow::importedResultFilesOfModel；
		//旧格式 mxdb 导入须 mxdb0+帧文件完整组合，单独回放初始构型文件会因缺帧被拒）
		resultFiles = mPostBrowserWindow::getInstance()->importedResultFilesOfModel(modelID);

		//云图条目块：[quint32 条目数] + 逐条 [QString itemName][QString viewName]
		const QList<QPair<QString, QString>> items =
			mPostBrowserWindow::getInstance()->getPostTree()->collectCloudItems(modelID);
		QByteArray buffer;
		QDataStream out(&buffer, QIODevice::WriteOnly);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		out.setVersion(QDataStream::Qt_5_13);
#else
		out.setVersion(QDataStream::Qt_5_12);
#endif
		out << quint32(items.size());
		for (const auto& pair : items)
			out << pair.first << pair.second;

		//派生工况定义块（批次A0）：[魔数][版本][store 载荷]；旧版本会话无此块，
		//恢复端按魔数缺失静默跳过
		constexpr quint32 kDerivedChunkMagic = 0x44534331u;   //"DSC1"
		out << kDerivedChunkMagic << quint32(1)
			<< mPostDerivedCaseStore::instance().serialize(
				vm->getModelData()->getModelDataID());
		postItemsChunk = buffer;
	}

	void mPostSessionCoordinator::attachPostTree(int modelID)
	{
		//建树入口按当前模型工作（无显式ID版），先显式切换（会话恢复期编排方
		//本已切到该模型，此处兜底用户中途切换的漂移）
		mViewportModelManager::getInstance()->setCurrentViewportModel(modelID);
		mPostBrowserWindow::getInstance()->getPostTree()->addPostModelBrowserTree();
	}

	void mPostSessionCoordinator::syncPostPanelToViewport(int modelID, int viewportID)
	{
		//恢复收尾把参数面板同步到激活视口：结果应用链按视口逐个级联刷新，
		//结束后面板停留在最后一个应用的视口（非激活视口）——不经此同步，
		//用户随后的变量/工况切换会打到末位应用视口。经 edit3DResult 走与
		//用户点树节点完全相同的全量级联（含 FitView 与面板下拉重建）
		auto* vm = mViewportModelManager::getInstance();
		int ownerModel = -1;
		auto* priv = vm->getViewportPrivateByID(viewportID, &ownerModel);
		if (priv == nullptr || ownerModel != modelID)
			return;
		auto* model = vm->getViewportModel(modelID);
		if (model == nullptr || !model->isPostModel())
			return;
		mPostInfoHub::getInstance()->edit3DResult(priv->getViewportName());
	}

	void mPostSessionCoordinator::queuePostResultImport(int modelID, const QStringList& resultFiles,
		const QByteArray& postItemsChunk)
	{
		//仅排队不启动（见头注释：编排方同步重建期间当前模型漂移，过早启动
		//会使导入收尾链按 getCVM 读到错误模型）
		PendingEntry entry;
		entry.modelID = modelID;
		entry.resultFiles = resultFiles;
		entry.postItemsChunk = postItemsChunk;
		_queue.append(entry);
	}

	void mPostSessionCoordinator::kickPostRestoreQueue()
	{
		if (!_importing && !_queue.isEmpty())
			startHeadImport();
	}

	void mPostSessionCoordinator::setPostRestoreDoneCallback(std::function<void()> onDone)
	{
		_onAllDone = std::move(onDone);
	}

	int mPostSessionCoordinator::pendingPostRestoreCount() const
	{
		return _queue.size() + (_importing ? 1 : 0);
	}

	void mPostSessionCoordinator::startHeadImport()
	{
		if (_queue.isEmpty())
		{
			//全部完成：触发一次性回调并清空（队列可复用于下一次会话恢复）。
			//回调延迟一拍：本函数运行在 updateResultTreeSig 的信号分发链内（协调器
			//槽最先执行），同步触发回调（=finishSessionRestore，会按保存的激活视口
			//切换当前模型）会使同链后续槽 mPostTreeView::updateResultTree 以错误
			//模型建结果根——结果根挂到树顶层，与模型根并列成"双模型树根"
			if (_onAllDone)
			{
				std::function<void()> done = std::move(_onAllDone);
				_onAllDone = nullptr;
				QTimer::singleShot(0, this, [this, done]() { done(); });
			}
			return;
		}

		const PendingEntry entry = _queue.first();
		auto* vm = mViewportModelManager::getInstance()->getViewportModel(entry.modelID);
		if (vm == nullptr || entry.resultFiles.isEmpty())
		{
			_queue.removeFirst();
			startHeadImport();
			return;
		}

		//导入漏斗按当前模型落库（new mPostDataBase 挂 getCVM 的 _postDataManager）
		mViewportModelManager::getInstance()->setCurrentViewportModel(entry.modelID);
		_importing = true;
		//notifyScanHint=false：会话恢复的自动重导入静默（新版格式大文件扫描提示
		//只服务于用户手动导入的等待预期，启动期恢复不刷消息）
		const bool started = mPostInfoHub::getInstance()->import3DResult(entry.resultFiles, -1, QString(), false);
		if (!started)
		{
			//启动失败（结果文件缺失/无可导入格式等）：提示并跳过，驱动下一个
			_importing = false;
			_queue.removeFirst();
			mGlobalSignals::getInstance()->outputMessageSig(QString(
				"会话恢复：后处理结果文件导入失败，已跳过（模型ID %1）").arg(entry.modelID));
			startHeadImport();
		}
	}

	void mPostSessionCoordinator::onImportFinished()
	{
		if (!_importing || _queue.isEmpty())
			return;   //非会话恢复期间的导入完成（用户手动导入等），忽略

		_importing = false;
		const PendingEntry entry = _queue.takeFirst();

		//回放云图条目（视口已按保存名重建；导入收尾链的剩余步骤在本次信号
		//分发返回后继续执行，回放须在其用 _curPostRend 前完成绑定，见
		//replayCloudItems 头注释）
		auto* vm = mViewportModelManager::getInstance()->getViewportModel(entry.modelID);
		//补登记重导入的完整文件清单：恢复链直导不经创建漏斗，缺登记会使
		//"恢复态再保存"采集到不完整清单（二代会话打开被导入过滤器拒绝）
		mPostBrowserWindow::getInstance()->registerSessionRestoredResult(entry.modelID, entry.resultFiles);
		if (vm != nullptr && !entry.postItemsChunk.isEmpty())
		{
			QList<QPair<QString, QString>> items;
			QDataStream in(entry.postItemsChunk);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
			in.setVersion(QDataStream::Qt_5_13);
#else
			in.setVersion(QDataStream::Qt_5_12);
#endif
			quint32 count = 0;
			in >> count;
			for (quint32 i = 0; i < count && in.status() == QDataStream::Ok; ++i)
			{
				QString itemName, viewName;
				in >> itemName >> viewName;
				if (!itemName.isEmpty() && !viewName.isEmpty())
					items.append({ itemName, viewName });
			}
			//派生工况定义块（批次A0）：云图条目后的 [魔数][版本][载荷]；旧会话
			//无此块（atEnd/魔数不符）静默跳过。恢复目标为新导入模型的 modelDataID
			//（与面板/树的 store 桶键同源）；树分支由随后的导入收尾链
			//（slot_updatePostModelTree → rebuildDerivedCaseTree）落节点
			constexpr quint32 kDerivedChunkMagic = 0x44534331u;   //"DSC1"
			if (vm != nullptr && vm->getModelData() != nullptr
				&& !in.atEnd() && in.status() == QDataStream::Ok)
			{
				quint32 magic = 0, version = 0;
				in >> magic >> version;
				if (magic == kDerivedChunkMagic && version == 1 && in.status() == QDataStream::Ok)
				{
					QByteArray derivedBlob;
					in >> derivedBlob;
					mPostDerivedCaseStore::instance().deserialize(
						vm->getModelData()->getModelDataID(), derivedBlob);
				}
			}

			//回放经树视图入口（选中/显示名接管等私有链路可达）；当前模型兜底
			//切到目标模型（renameItemViewportDisplay 等按 getCVM 工作）
			mViewportModelManager::getInstance()->setCurrentViewportModel(entry.modelID);
			mPostBrowserWindow::getInstance()->getPostTree()->replayCloudItems(entry.modelID, items);
		}

		//结果数据应用到该模型的全部云图视口：导入收尾链只绑定/应用其单视口
		//（_curPostRend），会话恢复重建的其余视口无人触发应用会呈空白。延迟到
		//本次信号分发（收尾链剩余步骤）结束后执行，避免与收尾链互踩
		if (vm != nullptr && vm->getModelData() != nullptr)
		{
			const QStringList postNames = vm->getModelData()
				->getManager<PostData::mPostDataManager>(Physical_Post).getAllDatabaseName();
			if (!postNames.isEmpty())
			{
				//按视口ID升序（=创建序，与保存序对齐）收集后应用：getAllViewportIDs
				//为 QHash 哈希序（Qt 指针哈希每进程随机种子），顺序漂移会使应用顺序
				//随机——后处理面板单例的"当前视口"随最后应用者变化，与导入收尾链
				//（按 _curPostRend 工作）互踩，表现为恢复间歇性空白/崩溃
				QList<int> vpIDs = vm->getAllViewportIDs();
				std::sort(vpIDs.begin(), vpIDs.end());
				QStringList viewNames;
				for (int vpID : vpIDs)
				{
					QWidget* sub = vm->getSubWindowByViewportID(vpID);
					auto* priv = sub ? vm->getViewportPrivate(sub) : nullptr;
					if (priv != nullptr && priv->getViewportType() == MxFunctions::ViewportPost3D)
						viewNames.append(priv->getViewportName());
				}
				const QString postName = postNames.first();
					const int modelID = entry.modelID;
				QTimer::singleShot(0, this, [modelID, viewNames, postName]()
				{
					//应用链内部按 getCVM（当前模型）按名查渲染器，多模型排队恢复时
					//须先切回目标模型（期间激活/收尾均按视口ID工作，不受当前模型影响）
					mViewportModelManager::getInstance()->setCurrentViewportModel(modelID);
					mPostInfoHub::getInstance()->applyResultToViewports(viewNames, postName);
				});
			}
		}

		//驱动下一个（队列空则触发完成回调）
		startHeadImport();
	}

	void mPostSessionCoordinator::onImportFailed(int postModelID)
	{
		Q_UNUSED(postModelID)   //失败链捕获的宿主模型ID：恢复回滚按队列条目 modelID（见下）
		if (!_importing || _queue.isEmpty())
			return;   //非会话恢复期间的导入失败（用户手动导入等，回滚在浏览器窗口回滚槽）

		//与 startHeadImport 的同步启动失败同款处理：跳过该条、驱动下一个；
		//不回放条目、不登记清单（无有效数据）。队列排空后由 startHeadImport
		//触发完成回调 → finishSessionRestore，恢复保存/打开会话与界面状态
		_importing = false;
		const int modelID = _queue.first().modelID;
		_queue.removeFirst();
		//回滚该条新建的空模型：树根移除+删模型与 mtime 重建同款删除序列（协调器
		//连接在浏览器窗口回滚槽之前，本回滚完成后该槽按"无注册表登记"自行跳过，
		//不会重复删除）；结果库已由失败分支删除，模型已不存在的兜底跳过
		if (mViewportModelManager::getInstance()->getViewportModel(modelID) != nullptr)
		{
			emit MechCommon::mMechGlobalSignals::getInstance()->removePostModelTreeSig(modelID);
			mViewTreeFunction::deleteViewportModel(modelID);
		}
		mGlobalSignals::getInstance()->outputMessageSig(QString(
			"会话恢复：后处理结果文件导入失败，已跳过（模型ID %1）").arg(modelID));
		startHeadImport();
	}
}
