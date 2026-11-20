#include "mMxsesRestore.h"

//Qt
#include <QDataStream>
#include <QFile>
#include <QTimer>
#include <QCoreApplication>
#include <algorithm>

//全局信号
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"

//IOFile
#include "mIOFileManager.h"

//Common
#include "mPostSessionProvider.h"

//PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//GUIFoundation
#include "mViewTreeFunction.h"

//MainWindow
#include "mBrowserWindow.h"
#include "mTreeView.h"
#include "mViewBrowserWindow.h"
#include "mPickWidgetController.h"
#include "mMechMainWindow.h"
#include "ViewportContainer.h"

//PostCurve
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"
#include "CurveCore/CurveDocSerialize.h"
#include "CurveView/mCurveViewportFactory.h"
#include "mCurveDataSourceStore.h"

//BasicData
#include "mModelData.h"
#include "mModelInOutSteam.h"

using namespace MxFunctions;
using namespace MechPreData;
using namespace MechCommon;
using namespace MechIOFile;
using namespace PostCurve;

namespace MechMainWindow
{
	namespace
	{
		//延迟收尾上下文（restoreFrom 解析，finishSessionRestore 消费；
		//界面线程串行访问，无并发）
		int s_pendingActiveViewportID = -1;
		int s_pendingFallbackModelID = -1;
		int s_pendingActiveChartNodeId = -1;
		int s_pendingCurveNodeId = -1;
		QString s_pendingFileName;

		//采集单个视口数据（名称、类型与用户重命名标记；视口名全局唯一，
		//是页结构引用的键）
		MxsesViewportState collectViewportState(mViewportPrivate* priv)
		{
			MxsesViewportState state;
			state.name = priv->getViewportName();
			state.viewportType = static_cast<int>(priv->getViewportType());
			state.userRenamed = priv->isUserRenamed();
			return state;
		}
	}

	bool mMxsesRestore::saveSession(const QString& filePath)
	{
		if (filePath.isEmpty()) return false;

		//后处理结果异步重导入未完成时拒绝（采集会读到半恢复状态）
		if (auto* postParticipant = postSessionParticipantRef();
			postParticipant != nullptr && postParticipant->pendingPostRestoreCount() > 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("后处理结果数据仍在恢复中，请稍候再保存会话"));
			return false;
		}

		//提交当前面板未保存的拾取数据（与mxcae保存一致）。
		//注意：不能 emit requestPrepareWriteFile——它与拾取提交以
		//BlockingQueuedConnection 连接，从界面线程直接发射会同步等待界面线程处理，立即死锁。
		mPickWidgetController::commitActivePickWidget();

		MxsesFileData data = collectSessionData();
		if (data.models.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("当前无可保存的模型，会话文件未生成"));
			return false;
		}

		mIOFileManager::getInstance()->writeMxsesFile(filePath, data);
		return true;
	}

	bool mMxsesRestore::openSession(const QString& filePath)
	{
		if (filePath.isEmpty()) return false;
		//后处理结果异步重导入未完成时拒绝（恢复中再开新会话会与延迟收尾互踩）
		if (auto* postParticipant = postSessionParticipantRef();
			postParticipant != nullptr && postParticipant->pendingPostRestoreCount() > 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("后处理结果数据仍在恢复中，请稍候再打开会话"));
			return false;
		}
		mIOFileManager::getInstance()->readMxsesFile(filePath);
		return true;
	}

	void mMxsesRestore::slot_readMxsesFinished()
	{
		auto* ioManager = mIOFileManager::getInstance();
		const bool isSuccess = ioManager->isSuccessReadMxses();
		const QString fileName = ioManager->getReadMxsesFileName();

		if (isSuccess)
		{
			//先拷贝数据再释放线程，恢复期间不再依赖读线程
			const MxsesFileData data = ioManager->getMxsesFileData();
			ioManager->releaseReadMxsesThread();

			bool hasPendingPostImports = false;
			if (restoreFrom(data, hasPendingPostImports))
			{
				s_pendingFileName = fileName;
				auto* postParticipant = postSessionParticipantRef();
				if (hasPendingPostImports && postParticipant != nullptr)
				{
					//后处理结果文件异步重导入中：收尾延迟到队列排空（回调一次）
					mGlobalSignals::getInstance()->outputMessageSig(
						QString("正在恢复后处理结果数据，请稍候……"));
					postParticipant->setPostRestoreDoneCallback([]()
					{
						finishSessionRestore();
					});
					postParticipant->kickPostRestoreQueue();
					//队列全部同步失败时 kick 内已触发回调（finish 已执行），此处不再收尾
					return;
				}
				finishSessionRestore();
				return;
			}
			mGlobalSignals::getInstance()->outputMessageSig(QString("会话文件恢复失败： %1").arg(fileName));
		}
		else
		{
			ioManager->releaseReadMxsesThread();
		}
		mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
	}

	void mMxsesRestore::slot_writeMxsesFinished()
	{
		auto* ioManager = mIOFileManager::getInstance();
		const QString fileName = ioManager->getWriteMxsesFileName();
		ioManager->releaseWriteMxsesThread();
		mGlobalSignals::getInstance()->outputMessageSig(QString("会话文件 %1 保存成功!").arg(fileName));
	}

	void mMxsesRestore::finishSessionRestore()
	{
		auto* vmManager = mViewportModelManager::getInstance();
		auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();

		//上下文收尾按保存态三分支：图表卡片（曲线上下文）> 激活渲染视口 > 首个模型兜底
		bool chartContextDone = false;
		if (s_pendingActiveChartNodeId != -1)
		{
			//保存时激活的是图表卡片（曲线上下文）：合成一次卡片激活，走与用户
			//点击卡片完全相同的补齐链（菜单/树窗/视图树选中/参数栏），随后按
			//保存值重放曲线树选中。卡片查无（曲线文档或页结构恢复缺失）时落空，
			//落回下方渲染视口/兜底分支
			auto* mainWindow = mMechMainWindow::getInstance();
			auto* container = vmManager->getViewportContainer();
			const int cardId = mainWindow != nullptr
				? mainWindow->cardIdForChartId(s_pendingActiveChartNodeId) : -1;
			if (mainWindow != nullptr && container != nullptr && cardId != -1)
			{
				//先切到卡片所在页（卡片随页显隐；同页早退，仿双击激活路径）
				if (viewTree)
				{
					const int pageId = viewTree->pageOfViewport(cardId);
					if (pageId >= 0)
						viewTree->switchToPage(pageId);
				}
				//守卫仍挂起时合成激活：补齐 lambda 的直接段（setWorkMode(Curve)、
				//树窗切换、视图树选中、参数栏判重补齐）不受守卫影响；其内
				//syncTreeSelectionForChart 可能改走曲线当前节点，故随后按保存值重放
				container->setActiveViewport(cardId);
				//提前解除守卫再重放选中：sig_currentNodeChanged 全链生效（含
				//editCurveNode 参数栏切换）；同 ID 早退无副作用
				mainWindow->setRestoringSession(false);
				mCurveDocController::getInstance()->setCurrentNodeId(s_pendingCurveNodeId);
				chartContextDone = true;
			}
		}
		if (!chartContextDone && s_pendingActiveViewportID != -1)
		{
			vmManager->activateViewportByID(s_pendingActiveViewportID);

			//激活视口为后处理视口时同步参数面板到它：结果应用链按视口逐个
			//级联刷新，结束后面板停留在最后一个应用的视口（非激活视口），
			//不同步则用户随后的变量/工况切换会打到末位应用视口
			int actModelID = -1;
			vmManager->getViewportPrivateByID(s_pendingActiveViewportID, &actModelID);
			auto* actModel = actModelID == -1 ? nullptr : vmManager->getViewportModel(actModelID);
			if (actModel != nullptr && actModel->isPostModel()
				&& postSessionParticipantRef() != nullptr)
			{
				postSessionParticipantRef()->syncPostPanelToViewport(actModelID, s_pendingActiveViewportID);
			}
		}
		else if (!chartContextDone && s_pendingFallbackModelID != -1)
		{
			vmManager->setCurrentViewportModel(s_pendingFallbackModelID);
		}

		//激活切换后再次重放当前页布局（单视口布局按"激活视口提首位"排列，
		//须以最终激活状态为准；后处理导入收尾激活过其视口，此处统一纠正回保存态）
		if (viewTree)
		{
			viewTree->refreshCurrentPageLayout();
		}

		if (auto* mainWindow = mMechMainWindow::getInstance())
		{
			mainWindow->setRestoringSession(false);
			//上下文终态对齐：恢复激活视口与恢复期既得激活同ID时容器去重早退
			//（无sig_activeViewportChanged/currentViewportChanged），菜单/参数栏
			//联动缺席；恢复期曲线树选中回放也被主窗口守卫拦下全局切换，此处按
			//激活视口显式对齐。先同步一次，再挂一拍延迟复核——恢复期排队的
			//延迟轮（图表scheduleRebuild等singleShot）先入队先执行，任何延迟
			//副作用之后终态仍收敛到激活视口
			mainWindow->alignContextToActiveViewport();
			QTimer::singleShot(0, []()
			{
				if (QCoreApplication::closingDown())
					return;
				if (auto* mw = mMechMainWindow::getInstance())
					mw->alignContextToActiveViewport();
			});
		}

		mGlobalSignals::getInstance()->outputMessageSig(
			QString("会话文件加载完成： %1").arg(s_pendingFileName));
		mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
		s_pendingActiveViewportID = -1;
		s_pendingFallbackModelID = -1;
		s_pendingActiveChartNodeId = -1;
		s_pendingCurveNodeId = -1;
		s_pendingFileName.clear();
	}

	MxsesFileData mMxsesRestore::collectSessionData()
	{
		MxsesFileData data;
		auto* vmManager = mViewportModelManager::getInstance();
		auto* mainWindow = mMechMainWindow::getInstance();
		auto* postParticipant = postSessionParticipantRef();

		//采集全部模型与视口。
		//顺序关键：getAllModelDataID/getSubWindows 均为 QHash 哈希序（非创建序），
		//恢复时"新视口ID与文件ID一致"依赖重建顺序与文件顺序严格对齐，
		//故此处必须按ID升序（=创建序，ID计数单调递增）排序后再写入文件
		std::vector<int> modelIDs = vmManager->getAllModelDataID();
		std::sort(modelIDs.begin(), modelIDs.end());
		for (int modelID : modelIDs)
		{
			auto* viewportModel = vmManager->getViewportModel(modelID);
			if (!viewportModel || !viewportModel->getModelData()) continue;

			MxsesModelEntry entry;
			entry.modelName = viewportModel->getModelData()->getModelDataName();
			entry.isPost = viewportModel->isPostModel();
			entry.mxcaePath = viewportModel->getMxcaePath();

			QList<QPair<int, mViewportPrivate*>> viewportPrivs;
			const QList<QWidget*> subWindows = viewportModel->getSubWindows();
			for (QWidget* subWindow : subWindows)
			{
				auto* priv = viewportModel->getViewportPrivate(subWindow);
				if (!priv) continue;
				viewportPrivs.append({ priv->getViewportID(), priv });
			}
			std::sort(viewportPrivs.begin(), viewportPrivs.end(),
				[](const QPair<int, mViewportPrivate*>& a, const QPair<int, mViewportPrivate*>& b)
				{ return a.first < b.first; });
			for (const auto& vp : viewportPrivs)
			{
				entry.viewports.append(collectViewportState(vp.second));
			}

			//模型数据chunk（与 mxcae 相同的序列化方式）
			QDataStream chunkStream(&entry.modelDataChunk, QIODevice::WriteOnly);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
			chunkStream.setVersion(QDataStream::Qt_5_13);
#else
			chunkStream.setVersion(QDataStream::Qt_5_12);
#endif
			chunkStream << *viewportModel->getModelData();

			//后处理模型：结果文件路径 + 后处理树云图条目（经参与者采集）
			if (entry.isPost && postParticipant != nullptr)
			{
				postParticipant->collectPostSessionEntry(modelID, entry.resultFiles, entry.postItemsChunk);
			}

			data.models.append(entry);
		}

		//采集视图树页结构（渲染视口按名、图表卡片按曲线文档图表节点id记录，
		//保持页内交错顺序；非渲染卡片查无 priv，经主窗口卡片登记反查识别）
		auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
		if (viewTree && viewTree->getModel())
		{
			auto* treeModel = viewTree->getModel();
			const QList<QPair<QString, int>> pages = treeModel->pageList();
			for (const auto& page : pages)
			{
				MxsesPageEntry pageEntry;
				pageEntry.name = page.first;
				pageEntry.layoutIndex = treeModel->pageLayout(page.second);

				const QList<int> viewportIDs = treeModel->pageViewportIDs(page.second);
				for (int viewportID : viewportIDs)
				{
					int nodeModelID = -1;
					QString nodeName;
					treeModel->viewportNodeInfo(viewportID, nodeModelID, nodeName);
					auto* priv = vmManager->getViewportPrivateByID(viewportID);
					if (priv != nullptr)
					{
						MxsesPageItem item;
						item.kind = 0;
						item.name = priv->getViewportName();
						pageEntry.items.append(item);
					}
					else if (mainWindow != nullptr && mainWindow->chartIdForCardId(viewportID) != -1)
					{
						MxsesPageItem item;
						item.kind = 1;
						item.refId = mainWindow->chartIdForCardId(viewportID);
						item.name = nodeName;
						pageEntry.items.append(item);
					}
					//其余异常登记（查无视口也非卡片）跳过
				}

				const QList<int> visibleIDs = treeModel->pageVisibleViewportIDs(page.second);
				for (int viewportID : visibleIDs)
				{
					int nodeModelID = -1;
					QString nodeName;
					treeModel->viewportNodeInfo(viewportID, nodeModelID, nodeName);
					auto* priv = vmManager->getViewportPrivateByID(viewportID);
					if (priv != nullptr)
					{
						MxsesPageItem item;
						item.kind = 0;
						item.name = priv->getViewportName();
						pageEntry.visibleItems.append(item);
					}
						else if (mainWindow != nullptr && mainWindow->chartIdForCardId(viewportID) != -1)
						{
							MxsesPageItem item;
							item.kind = 1;
							item.refId = mainWindow->chartIdForCardId(viewportID);
							item.name = nodeName;
							pageEntry.visibleItems.append(item);
						}
					}

				data.pages.append(pageEntry);
			}

			//当前页
			QModelIndex curPageIdx = treeModel->findPageIndex(viewTree->curPageID());
			if (curPageIdx.isValid())
			{
				data.currentPageName = treeModel->data(curPageIdx, Qt::DisplayRole).toString();
			}
		}

		//当前激活视口与联动开关（激活视口按名称记录）
		if (auto* activePriv = vmManager->getCurrentViewportPrivate())
		{
			data.activeViewportName = activePriv->getViewportName();
		}
		//激活视口为图表卡片时按图表节点id记录：卡片不在渲染视口注册表，
		//卡片激活时上方 activeViewportName 采集为空，保存时的曲线上下文
		//由此字段承载（恢复收尾据此合成卡片激活）
		if (auto* container = vmManager->getViewportContainer())
		{
			const int activeID = container->activeViewportID();
			if (activeID >= mMechMainWindow::ChartViewportIdBase)
			{
				data.activeChartNodeId = mainWindow != nullptr
					? mainWindow->chartIdForCardId(activeID) : -1;
			}
		}
		data.viewportSynced = vmManager->isViewportSynced();

		//曲线模块整体快照：文档树（结构+数据+样式+来源，内嵌 xData/yData 原始值）、
		//数据源路径（恢复时重解析，供"更改曲线"等溯源功能）、树选中节点
		auto* curveController = mCurveDocController::getInstance();
		data.curveDocChunk = saveCurveDocument(curveController->document());
		data.curveSourceFiles = mCurveDataSourceStore::getInstance()->filePaths();
		data.curveCurrentNodeId = curveController->currentNodeId();

		return data;
	}

	bool mMxsesRestore::restoreFrom(const MxsesFileData& data, bool& hasPendingPostImports)
	{
		hasPendingPostImports = false;
		if (data.models.isEmpty()) return false;

		auto* vmManager = mViewportModelManager::getInstance();
		auto* browserTree = mBrowserWindow::getInstance()->getBrowserTree();
		auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
		auto* mainWindow = mMechMainWindow::getInstance();
		auto* postParticipant = postSessionParticipantRef();
		auto* curveController = mCurveDocController::getInstance();

		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		//批量重建期间挂起视图树与视口管理器的信号联动：
		//否则视口增删信号会自动增删树节点（与下方手动重建页结构冲突，出现重复页/残留节点），
		//且清空阶段 active 视口被删会触发视图树兜底逻辑自动补建一个空模型
		if (viewTree)
		{
			viewTree->suspendViewportSync();
		}

		//会话恢复模式：抑制图表卡片创建联动的激活dock/切页/激活卡片副作用
		//（页归属按保存结构由下方页重建统一登记，收尾在 finishSessionRestore）
		if (mainWindow)
		{
			mainWindow->setRestoringSession(true);
		}

		//0. 清空曲线侧：文档逐页删除联动工厂自动关闭全部图表卡片（主窗口移除
		//卡片与树节点），数据源缓存一并清空。V3 文件无曲线数据，此处即整体重置
		curveController->clearDocument();
		mCurveDataSourceStore::getInstance()->clear();

		//1. 清空全部旧模型与视图树页（仿 restoreAutoSaveFile 的全删模式）
		const std::vector<int> oldModelIDs = vmManager->getAllModelDataID();
		for (int oldID : oldModelIDs)
		{
			browserTree->removeModelRootItem(oldID);
			mViewTreeFunction::deleteViewportModel(oldID);
		}
		if (viewTree && viewTree->getModel())
		{
			viewTree->getModel()->clearAllPages();
		}

		//全部模型/视口已删除，重置全局ID计数，避免ID无限累加
		MechModelData::mModelData::resetModelGlobalID();
		mViewportModel::resetViewportGlobalID();

		//视口名 -> 恢复后实际新视口ID 的映射。
		//视口名全局唯一（renameViewport 强制），页结构/激活视口按名称经此映射
		//换算到恢复后的视口；不依赖ID计数与创建顺序
		QHash<QString, int> nameToNewViewportID;
		//记录每个文件模型对应的新模型ID
		QList<int> newModelIDs;
		//后处理模型排队重导入的条目数
		int queuedPostImports = 0;

		//2. 逐模型重建
		for (const MxsesModelEntry& modelEntry : data.models)
		{
			const int newModelID = modelEntry.isPost
				? vmManager->newPostViewportModel()
				: vmManager->newViewportModel();
			newModelIDs.append(newModelID);

			auto* viewportModel = vmManager->getViewportModel(newModelID);
			if (!viewportModel || !viewportModel->getModelData()) continue;

			//灌入文件中的模型数据（保留文件内名称，与打开mxcae时保留当前名的策略不同——
			//会话恢复以文件内容为准）
			QDataStream chunkStream(modelEntry.modelDataChunk);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
			chunkStream.setVersion(QDataStream::Qt_5_13);
#else
			chunkStream.setVersion(QDataStream::Qt_5_12);
#endif
			try {
				chunkStream >> *viewportModel->getModelData();
			}
			catch (...) {
				mGlobalSignals::getInstance()->outputMessageSig(QString("模型数据反序列化失败： %1").arg(modelEntry.modelName));
			}
			viewportModel->getModelData()->setModelDataName(modelEntry.modelName);
			viewportModel->setMxcaePath(modelEntry.mxcaePath);

			//补齐视口：首个视口复用新建的，其余按类型+名称补齐。
			//注意 getSubWindows() 返回 QHash 键集（哈希序，非创建序），
			//不能按 first()/last() 取"第一个视口"，须按视口ID定位。
			//模型新建时自动创建的视口只有一个，直接取唯一视口复用（i==0）。
			for (int i = 0; i < modelEntry.viewports.size(); ++i)
			{
				const MxsesViewportState& viewportState = modelEntry.viewports.at(i);
				QWidget* subWindow = nullptr;

				if (i == 0)
				{
					//复用模型新建时自动创建的唯一视口，按文件内容改名
					const QList<QWidget*> subWindows = viewportModel->getSubWindows();
					if (!subWindows.isEmpty())
					{
						subWindow = subWindows.first();
						auto* priv = viewportModel->getViewportPrivate(subWindow);
						if (priv && priv->getViewportName() != viewportState.name)
						{
							viewportModel->renameViewport(priv->getViewportID(), viewportState.name);
						}
					}
				}
				else
				{
					//走管理器层 newViewport：内部完成 addViewport、容器登记、
					//标题同步与 sig_viewportAdded 发射（视图树联动已挂起，不会自动加节点）。
					//用 forceSetCurrentViewportModel 强制切换：上方刷新阶段已把当前模型
					//切回本模型，普通 set 会因"同模型重复设置"直接返回，导致 newViewport
					//实际落到了上一个模型（视口丢失到别的模型）
					vmManager->forceSetCurrentViewportModel(newModelID);
					vmManager->newViewport(static_cast<ViewportType>(viewportState.viewportType), viewportState.name);
					//newViewport 后按最新分配的视口ID定位（QHash 键序不可靠）：
					//新视口ID = 当前最大ID（ID计数单调递增）
					int maxID = -1;
					const QList<QWidget*> curWindows = viewportModel->getSubWindows();
					for (QWidget* w : curWindows)
					{
						auto* p = viewportModel->getViewportPrivate(w);
						if (p && p->getViewportID() > maxID)
						{
							maxID = p->getViewportID();
							subWindow = w;
						}
					}
				}

				if (!subWindow) continue;
				auto* priv = viewportModel->getViewportPrivate(subWindow);
				if (!priv) continue;

				//回写用户重命名标记（影响模型重命名时的视口名称联动）
				priv->setUserRenamed(viewportState.userRenamed);
				//登记 视口名 -> 实际新视口ID 映射（页结构注册经映射换算）
				nameToNewViewportID.insert(viewportState.name, priv->getViewportID());
			}

			//3. 刷新模型树与渲染
			if (modelEntry.isPost)
			{
				//后处理模型：树根建在后处理树（经参与者；不再走前处理树——
				//原实现误建前处理树根属缺陷）。updateAllModelOperate 仅遍历
				//前处理 3D 视口，对后处理模型为空操作
				if (postParticipant != nullptr)
				{
					postParticipant->attachPostTree(newModelID);
					//结果文件排队异步重导入（视口/页已同步重建完毕后由调用方 kick）
					if (!modelEntry.resultFiles.isEmpty())
					{
						postParticipant->queuePostResultImport(newModelID,
							modelEntry.resultFiles, modelEntry.postItemsChunk);
						++queuedPostImports;
					}
				}
			}
			else
			{
				browserTree->addModelBrowserTree();
			}
			mGlobalSignals::getInstance()->updateModelTree();
			vmManager->setCurrentViewportModel(newModelID);
			viewportModel->updateAllModelOperate();
		}

		//4. 曲线侧恢复：数据源重登记（缺文件警告跳过——数据本体已随文档快照
		//内嵌，仅"更改曲线"等溯源功能不可用）→ 文档整树恢复（保留节点id；
		//insertNodeTree 仅对页根发 nodeAdded，图表不自动开卡，卡片由页重建统一开）
		if (!data.curveSourceFiles.isEmpty())
		{
			auto* sourceStore = mCurveDataSourceStore::getInstance();
			for (const QString& path : data.curveSourceFiles)
			{
				QString err;
				if (!sourceStore->addFile(path, &err))
				{
					mGlobalSignals::getInstance()->outputMessageSig(QString(
						"曲线数据源文件加载失败（%1）：%2").arg(path, err));
				}
			}
		}
		if (!data.curveDocChunk.isEmpty())
		{
			if (!restoreCurveDocument(curveController->document(), data.curveDocChunk))
			{
				mGlobalSignals::getInstance()->outputMessageSig(QString("曲线文档恢复失败，会话不包含曲线"));
			}
		}
		else if (MXSES_FILE_VERSION < 0x4000)
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString(
				"旧版会话文件（V3）不包含曲线与后处理结果数据，已按空曲线恢复"));
		}

		//5. 重建视图树页结构（视口与图表卡片按保存的交错顺序统一登记）
		if (viewTree && viewTree->getModel())
		{
			auto* treeModel = viewTree->getModel();
			CurveDocument* curveDoc = curveController->document();
			//已开卡片的图表节点集合（兜底补登未入页图表用）
			QList<int> openedChartIds;

			for (const MxsesPageEntry& pageEntry : data.pages)
			{
				treeModel->addPage(pageEntry.name);
				//addPage 内部生成页ID，从 pageList 回查
				int realPageID = -1;
				const QList<QPair<QString, int>> pages = treeModel->pageList();
				for (const auto& p : pages)
				{
					if (p.first == pageEntry.name) { realPageID = p.second; }
				}
				if (realPageID == -1) continue;

				treeModel->setPageLayout(realPageID, pageEntry.layoutIndex);

				QList<int> visibleIDs;
				for (const MxsesPageItem& item : pageEntry.items)
				{
					if (item.kind == 0)
					{
						//渲染视口：按名经映射注册（与原实现一致）
						const int newID = nameToNewViewportID.value(item.name, -1);
						if (newID == -1) continue;
						int modelID = -1;
						auto* priv = vmManager->getViewportPrivateByID(newID, &modelID);
						treeModel->addViewportToPage(realPageID, newID, modelID,
							priv ? priv->getViewportName() : QString());
					}
					else if (item.kind == 1)
					{
						//图表卡片：先重绑曲线页↔视图页映射（openChart 按
						//pageViewPageId 取目标页），再开卡（恢复模式：仅建卡不挂树
						//不切页），最后按保存的页归属与顺序登记树节点
						const int chartId = item.refId;
						if (curveDoc->node(chartId) == nullptr) continue;
						const int curvePageId = curveDoc->parentNodeId(chartId);
						curveController->setPageViewPageId(curvePageId, realPageID);
						mCurveViewportFactory::getInstance()->openChart(chartId);
						const int cardId = mainWindow ? mainWindow->cardIdForChartId(chartId) : -1;
						if (cardId != -1)
						{
							treeModel->addViewportToPage(realPageID, cardId, -1,
								curveDoc->nodeFullDisplayName(chartId));
							openedChartIds.append(chartId);
						}
					}
				}

				for (const MxsesPageItem& item : pageEntry.visibleItems)
				{
					if (item.kind == 0)
					{
						const int newID = nameToNewViewportID.value(item.name, -1);
						if (newID != -1) visibleIDs.append(newID);
					}
					else if (item.kind == 1 && mainWindow != nullptr)
					{
						const int cardId = mainWindow->cardIdForChartId(item.refId);
						if (cardId != -1) visibleIDs.append(cardId);
					}
				}
				if (!visibleIDs.isEmpty())
				{
					treeModel->setPageVisibleViewportIDs(realPageID, visibleIDs);
				}
			}

			//兜底一：曲线页↔视图页按名重对齐（页对齐机制：两树页同名；
			//未入页重建循环的曲线页在此补映射，-1=保持未绑定）
			for (int curvePageId : curveDoc->childNodeIds(curveDoc->rootNodeId()))
			{
				const QString curvePageName = curveDoc->nodeName(curvePageId);
				for (const auto& p : treeModel->pageList())
				{
					if (p.first == curvePageName)
					{
						curveController->setPageViewPageId(curvePageId, p.second);
						break;
					}
				}
			}

			//兜底二：任何未列入页结构的视口（页数据缺失的）补登到第一页，
			//保证视图树节点与恢复出的视口一一对应；未入页的图表卡片同样补开
			QList<int> registeredIDs;
			for (int pageID : treeModel->pageIDs())
			{
				registeredIDs.append(treeModel->pageViewportIDs(pageID));
			}
			const QList<int> pageIDs = treeModel->pageIDs();
			if (!pageIDs.isEmpty())
			{
				const int fallbackPageID = pageIDs.first();
				QHashIterator<QString, int> mapIter(nameToNewViewportID);
				while (mapIter.hasNext())
				{
					mapIter.next();
					if (registeredIDs.contains(mapIter.value())) continue;
					int modelID = -1;
					auto* priv = vmManager->getViewportPrivateByID(mapIter.value(), &modelID);
					treeModel->addViewportToPage(fallbackPageID, mapIter.value(), modelID,
						priv ? priv->getViewportName() : QString());
				}
				//图表兜底：文档中存在但未入任何保存页的图表，补开卡片登到第一页
				if (mainWindow != nullptr)
				{
					for (int curvePageId : curveDoc->childNodeIds(curveDoc->rootNodeId()))
					{
						for (int chartId : curveDoc->childNodeIds(curvePageId))
						{
							if (curveDoc->nodeType(chartId) != CurveNodeType::Chart
								|| openedChartIds.contains(chartId))
								continue;
							curveController->setPageViewPageId(curvePageId, fallbackPageID);
							mCurveViewportFactory::getInstance()->openChart(chartId);
							const int cardId = mainWindow->cardIdForChartId(chartId);
							if (cardId != -1)
							{
								treeModel->addViewportToPage(fallbackPageID, cardId, -1,
									curveDoc->nodeFullDisplayName(chartId));
								openedChartIds.append(chartId);
							}
						}
					}
				}
			}

			//展开全部页节点：正常路径下节点随 sig_viewportAdded 自动 expand，
			//恢复期间信号联动挂起被跳过，这里统一补展开
			viewTree->expandAll();
		}

		//6. 恢复视图树联动、联动开关、切页（激活视口与最终布局重放延迟到
		//finishSessionRestore：后处理导入收尾会激活其视口，须在全部完成后统一纠正）
		if (viewTree)
		{
			viewTree->resumeViewportSync();
		}
		vmManager->setViewportSync(data.viewportSynced);
		if (viewTree)
		{
			const QList<QPair<QString, int>> pages = viewTree->pageList();
			for (const auto& p : pages)
			{
				if (p.first == data.currentPageName)
				{
					viewTree->switchToPage(p.second);
					break;
				}
			}
			//无论是否发生切页，强制重放当前页布局（挂起期间信号驱动的布局应用被跳过）
			viewTree->refreshCurrentPageLayout();
		}

		//7. 视口显示状态/相机不恢复，使用新建视口的默认状态；
		//曲线轴缩放（用户视图状态）同理不恢复，图表自动量程

		//记录延迟收尾上下文
		s_pendingActiveViewportID = data.activeViewportName.isEmpty()
			? -1 : nameToNewViewportID.value(data.activeViewportName, -1);
		s_pendingFallbackModelID = newModelIDs.isEmpty() ? -1 : newModelIDs.first();
		s_pendingActiveChartNodeId = data.activeChartNodeId;
		s_pendingCurveNodeId = data.curveCurrentNodeId;
		s_pendingFileName.clear();   //由 slot_readMxsesFinished 设置（读线程文件名）
		hasPendingPostImports = queuedPostImports > 0;

		//曲线树选中恢复（放在卡片全部重建后，避免联动激活副作用抢占）
		curveController->setCurrentNodeId(data.curveCurrentNodeId);

		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
		//兜底：异常路径下若联动仍挂起则恢复，避免视图树从此失去信号同步
		if (viewTree)
		{
			viewTree->resumeViewportSync();
		}
		return true;
	}
}
