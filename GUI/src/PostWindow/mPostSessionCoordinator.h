#pragma once
#pragma execution_character_set("utf-8")

//QT
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <functional>

//Common
#include "mPostSessionProvider.h"

/*
* 后处理会话协调器（PostWindow 侧，单例）
*
* 实现 MechCommon::IPostSessionParticipant，供 MainWindow 侧会话编排
* （mMxsesRestore）经提供者间接调用，完成 .mxses 会话的后处理接入：
*   - 采集：后处理模型的结果文件路径（mPostDataManager::getAllDatabaseFile）
*     + 后处理树云图条目（mPostTreeView::collectCloudItems）序列化块；
*   - 恢复：模型/视口由编排方同步重建后，按队列串行异步重导入结果文件
*     （mPostInfoHub::import3DResult 漏斗，落库到当前模型），每个导入完成
*     （updateResultTreeSig 唯一发射点）后回放该模型的云图条目并启动下一个；
*     全部完成后触发一次性回调（编排方延迟收尾：布局重放/激活/解除置灰）。
*
* 时序约定：本单例在 mPostBrowserWindow 构造最先处实例化并自注册，保证对
* updateResultTreeSig 的连接先于 mPostBrowserWindow 的同名连接——导入完成时
* 回放先运行（云图根填非空，使后续 updateResultTree 跳过初始视口创建分支，
* 多视口模型不产生游离协议名视口）。
*/
namespace PostWindow
{
	class mPostSessionCoordinator : public QObject, public MechCommon::IPostSessionParticipant
	{
		Q_OBJECT
	public:
		static mPostSessionCoordinator* getInstance();

		//--- IPostSessionParticipant ---
		void collectPostSessionEntry(int modelID,
			QStringList& resultFiles, QByteArray& postItemsChunk) override;
		void attachPostTree(int modelID) override;
		void syncPostPanelToViewport(int modelID, int viewportID) override;
		void queuePostResultImport(int modelID, const QStringList& resultFiles,
			const QByteArray& postItemsChunk) override;
		void kickPostRestoreQueue() override;
		void setPostRestoreDoneCallback(std::function<void()> onDone) override;
		int pendingPostRestoreCount() const override;

	private:
		explicit mPostSessionCoordinator(QObject* parent = nullptr);

		struct PendingEntry
		{
			int modelID = -1;
			QStringList resultFiles;
			QByteArray postItemsChunk;
		};

		//启动队首导入（空闲时）；队列空则触发完成回调
		void startHeadImport();
		//导入完成（updateResultTreeSig 且恢复中）：回放队首条目并驱动队列
		void onImportFinished();
		//导入失败（postResultImportFailedSig 且恢复中）：回滚该条新建的空模型后
		//跳过队首条目并驱动队列（读线程失败/结果库无效时无 updateResultTreeSig，
		//缺此会永久卡在 _importing）。postModelID 为失败链捕获的宿主模型ID，
		//恢复条目模型由编排方新建（无注册表登记），回滚按队列条目的 modelID 自理
		void onImportFailed(int postModelID);

		static mPostSessionCoordinator* _instance;
		QList<PendingEntry> _queue;
		bool _importing = false;
		std::function<void()> _onAllDone;
	};
}
