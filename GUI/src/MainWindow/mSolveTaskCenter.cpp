#include "mSolveTaskCenter.h"

#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QApplication>
#include <QTextCodec>
#include <QThread>
#include <QFile>
#include <QStringList>

#include "mAppConfiguration.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mPostResultServiceProvider.h"
#include "mCaseBase.h"
#include "mIOFileManager.h"
#include "mOThirdFile.h"
#include "mSolveTaskData.h"
#include "mSolveTaskManager.h"
#include "mGlobalParam.h"
#include "mSolveControlData.h"

using namespace MechPreData;
using namespace MechCommon;
using namespace MechIOFile;
using namespace MechModelData;
using namespace MechCaseData;
using namespace MechSolveTaskData;
using namespace MAppConfiguration;
using namespace MxFunctions;

namespace MechMainWindow
{
	/*
	* 求解器输出转Unicode（先UTF-8，无效字符多则转GBK）
	* 与mStartSolve中byteArrayToUnicode逻辑一致，static避免符号冲突
	*/
	static QString toUnicodeText(const QByteArray& array)
	{
		QTextCodec::ConverterState state;
		QString text = QTextCodec::codecForName("UTF-8")->toUnicode(array.constData(), array.size(), &state);
		if (state.invalidChars > 0)
		{
			text = QTextCodec::codecForName("GBK")->toUnicode(array);
		}
		return text;
	}

	/*
	* 目录段字符清理：替换路径非法字符（Windows），避免任务名含特殊字符导致目录创建失败
	* 只影响目录命名，不改动任务名本身；清理后为空时回退为Task
	*/
	static QString sanitizePathSegment(const QString& name)
	{
		static const QString illegalChars = "\\/:*?\"<>|";
		QString result = name;
		for (const QChar& c : illegalChars)
		{
			result.replace(c, QChar('_'));
		}
		result = result.trimmed();
		return result.isEmpty() ? QString("Task") : result;
	}

	/*
	* 日志时间戳前缀 [HH:mm:ss]
	*/
	static QString logStamp()
	{
		return "[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] ";
	}

	/*
	* 内存策略名称
	*/
	static QString memoryStrategyName(MemoryStrategy strategy)
	{
		switch (strategy)
		{
		case Memory_InCore: return "核内";
		case Memory_OutCore: return "核外";
		default: return "自适应";
		}
	}

	/*
	* 耗时格式化 hh:mm:ss
	*/
	static QString durationText(qint64 msecs)
	{
		qint64 seconds = msecs > 0 ? msecs / 1000 : 0;
		return QString("%1:%2:%3")
			.arg(seconds / 3600, 2, 10, QChar('0'))
			.arg((seconds / 60) % 60, 2, 10, QChar('0'))
			.arg(seconds % 60, 2, 10, QChar('0'));
	}

	/*
	* run结束状态文本
	*/
	static QString runStatusText(mSolveRunRecord::RunStatus status)
	{
		switch (status)
		{
		case mSolveRunRecord::Status_Done: return "所有工况计算完成";
		case mSolveRunRecord::Status_Failed: return "求解失败";
		case mSolveRunRecord::Status_Interrupted: return "用户中断求解";
		case mSolveRunRecord::Status_Cancelled: return "已取消（未开始求解）";
		default: return "结束";
		}
	}
	mSolveTaskCenter* mSolveTaskCenter::_instance = nullptr;

	mSolveTaskCenter::mSolveTaskCenter()
	{
		//后处理实例被用户删除的被动失效通知（PostCommon契约）：清除对应run的key，
		//使"结果"入口回到"未打开"状态（再次点击=重新创建，属用户显式动作）
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::postResultRemovedSig,
			this, [this](const QString& removedKey)
			{
				QVector<quint64> staleRuns;
				for (auto iter = _runPostKeys.begin(); iter != _runPostKeys.end(); ++iter)
				{
					if (iter.value() == removedKey)
					{
						staleRuns.push_back(iter.key());
					}
				}
				for (quint64 runID : staleRuns)
				{
					_runPostKeys.remove(runID);
				}
			});

		//该模型run记录委托的task指针随模型析构失效，收到通知先清记录并终止进程
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::viewportModelAboutToBeDeleted,
			this, [this](int modelID)
			{
				removeRunsOfModel(modelID);
			});
	}

	mSolveTaskCenter::~mSolveTaskCenter()
	{
		//终止所有仍在运行的求解器进程
		for (QProcess* process : _processes)
		{
			process->terminate();
			process->waitForFinished(3000);
			delete process;
		}
		_processes.clear();
		//关闭所有日志文件
		for (QFile* logFile : _logFiles)
		{
			if (logFile != nullptr)
			{
				logFile->close();
				delete logFile;
			}
		}
		_logFiles.clear();
		qDeleteAll(_runs);
		_runs.clear();
	}

	mSolveTaskCenter* mSolveTaskCenter::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mSolveTaskCenter;
		}
		return _instance;
	}

	void mSolveTaskCenter::interruptAll()
	{
		//中断所有运行中的run（程序关闭等场景；排队中的不动，随会话结束）
		const QVector<mSolveRunRecord*> runs = getAllActiveRuns();
		for (mSolveRunRecord* run : runs)
		{
			if (run->getStatus() == mSolveRunRecord::Status_Running)
			{
				interruptRun(run->getRunID());
			}
		}
	}

	void mSolveTaskCenter::viewRunResult(quint64 runID)
	{
		//已打开过：跳转已有后处理模型；未打开过：新开一个后处理模型导入结果
		if (!jumpToRunPostModel(runID))
		{
			openRunResultInNewPost(runID);
		}
	}

	bool mSolveTaskCenter::openRunResultInNewPost(quint64 runID)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || run->getStatus() != mSolveRunRecord::Status_Done)
		{
			mGlobalSignals::getInstance()->outputMessageSig("该任务没有可查看的结果！");
			return false;
		}
		if (run->getCalDir().isEmpty() || !QDir(run->getCalDir()).exists())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("任务\"%1\"的结果目录不存在！").arg(run->getTaskName()));
			return false;
		}

		//收集结果文件（与"打开结果文件"菜单同过滤规则）
		QDir resultDir(run->getCalDir());
		resultDir.setFilter(QDir::Files | QDir::NoSymLinks);
		resultDir.setNameFilters(QStringList() << "*.mxdb" << "*.mxdb0" << "*.op2");
		QStringList resultFiles;
		for (const QString& fileName : resultDir.entryList())
		{
			resultFiles << resultDir.absoluteFilePath(fileName);
		}
		if (resultFiles.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("任务\"%1\"的结果目录中没有结果文件！").arg(run->getTaskName()));
			return false;
		}

		//后处理结果服务未注册（主窗口未建成等）：不可查看
		auto* service = PostCommon::postResultServiceRef();
		if (service == nullptr)
		{
			mGlobalSignals::getInstance()->outputMessageSig("后处理不可用，无法查看结果！");
			return false;
		}

		//经结果服务创建后处理实例（新建模型+树+导入一条龙，失败服务内部回滚）；
		//key按不透明句柄只保存不解析（PostCommon契约）
		QString key = service->createPostResult(resultFiles);
		if (key.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("任务\"%1\"的结果导入失败！").arg(run->getTaskName()));
			return false;
		}
		_runPostKeys.insert(runID, key);
		return true;
	}

	bool mSolveTaskCenter::jumpToRunPostModel(quint64 runID)
	{
		QString key = _runPostKeys.value(runID);
		if (key.isEmpty())
		{
			return false;
		}
		auto* service = PostCommon::postResultServiceRef();
		if (service == nullptr)
		{
			return false;
		}
		if (service->showPostResult(key) == PostCommon::PostResultError::InvalidKey)
		{
			//key已失效（后处理模型被删除等）：作废入口，不自动重建（契约：重新打开是用户显式动作）
			_runPostKeys.remove(runID);
			mGlobalSignals::getInstance()->outputMessageSig("该后处理结果已失效或被删除，请重新打开！");
			return false;
		}
		return true;
	}

	bool mSolveTaskCenter::hasRunPostModel(quint64 runID) const
	{
		//key存在即视为已打开；失效由postResultRemovedSig被动清除，showPostResult兜底
		return !_runPostKeys.value(runID).isEmpty();
	}

	void mSolveTaskCenter::quickSolve(int modelID, int caseID)
	{
		Q_UNUSED(caseID);   //快速求解始终包含全部工况，caseID参数保留兼容旧调用方

		//同一时刻只允许一个自动写出流程
		if (_quickWriteThread != nullptr)
		{
			return;
		}
		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(modelID);
		if (viewportModel == nullptr || viewportModel->getModelData() == nullptr)
		{
			mGlobalSignals::getInstance()->outputMessageSig("模型不存在，无法求解！");
			return;
		}
		mModelData* modelData = viewportModel->getModelData();
		mSolveTaskManager& taskMgr = modelData->getManager<mSolveTaskManager>(Physical_SolveTask);

		//创建/复用快速任务：Solve_Model<模型ID>，包含全部工况，参数用弹窗默认值
		QString taskName = QString("Solve_Model%1").arg(modelID);
		mSolveTaskData* task = taskMgr.getTaskByName(taskName);
		if (task == nullptr)
		{
			task = taskMgr.createTask(taskName);
		}
		if (task == nullptr)
		{
			mGlobalSignals::getInstance()->outputMessageSig("快速求解：任务创建失败！");
			return;
		}

		//包含模型全部工况
		QVector<int> allCaseIDs;
		for (int id : modelData->getAllPhyID(Physical_Case))
		{
			allCaseIDs.push_back(id);
		}
		if (allCaseIDs.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig("无分析工况可求解！");
			return;
		}
		task->setCaseIDs(allCaseIDs);
		task->setCpuCores(8);                                     //弹窗默认
		task->setMemoryStrategy(Memory_Adaptive);                  //弹窗默认
		task->setOutputDir("");                                    //空=默认目录（与手动建任务一致）
		task->setSolverFile("");                                   //清空→走通用写文件流程

		//走通用"写出文件→提交"流程（路径与弹窗的写出文件/提交求解完全一致）
		_quickModelID = modelID;
		_quickTaskID = task->getTaskID();
		QString solverFile = taskSolverFile(task, modelID);

		//确保输出目录存在
		QDir outputDir = QFileInfo(solverFile).absoluteDir();
		if (!outputDir.exists())
		{
			outputDir.mkpath(".");
		}

		_quickWriteThread = new mOThirdFile(solverFile);
		connect(_quickWriteThread, &QThread::finished, this, [this]()
			{
				submitQuickTask();
			});
		_quickWriteThread->start();
	}

	void mSolveTaskCenter::submitQuickTask()
	{
		if (_quickWriteThread == nullptr)
		{
			return;
		}
		bool writeSuccess = _quickWriteThread->isWriteSuccess();
		delete _quickWriteThread;
		_quickWriteThread = nullptr;

		if (!writeSuccess)
		{
			mGlobalSignals::getInstance()->outputMessageSig("计算文件生成失败！");
			emit mMechGlobalSignals::getInstance()
				->mpcommSolverStatusChanged(QStringLiteral("failed"), QVariantMap{});
			return;
		}

		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(_quickModelID);
		if (viewportModel == nullptr || viewportModel->getModelData() == nullptr)
		{
			mGlobalSignals::getInstance()->outputMessageSig("提交求解失败：模型不存在！");
			return;
		}
		mSolveTaskManager& taskMgr = viewportModel->getModelData()->getManager<mSolveTaskManager>(Physical_SolveTask);
		mSolveTaskData* task = taskMgr.getTask(_quickTaskID);
		if (task == nullptr)
		{
			mGlobalSignals::getInstance()->outputMessageSig("提交求解失败：任务不存在！");
			return;
		}

		// mpcomm：求解自动写出的计算文件回传
		task->setSolverFile(taskSolverFile(task, _quickModelID));
		emit mMechGlobalSignals::getInstance()
			->mpcommExportCalcFileFinished(true, task->getSolverFile());

		QString errMsg;
		if (submitTask(_quickModelID, task->getTaskID(), &errMsg) == 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig(QString("快速求解：%1").arg(errMsg));
		}
	}

	QString mSolveTaskCenter::modelDirToken(int modelID)
	{
		//模型目录段：modelDataID每次启动软件都从1重排，跨会话会撞目录；
		//首次使用时生成带时间戳的会话级唯一标识并缓存（同会话同模型稳定复用）
		if (!_modelDirTokens.contains(modelID))
		{
			QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
			_modelDirTokens.insert(modelID, QString("Model_%1_%2").arg(modelID).arg(stamp));
		}
		return _modelDirTokens.value(modelID);
	}

	QString mSolveTaskCenter::taskOutputDir(const mSolveTaskData* task, int modelID)
	{
		if (task == nullptr)
		{
			return QString();
		}
		//taskOutputDir是静态方法，经单例取模型目录段（哈希缓存挂在单例上）
		QString modelName = mSolveTaskCenter::getInstance()->modelDirToken(modelID);
		QString dirTaskName = sanitizePathSegment(task->getTaskName());
		QString outputDir = task->getOutputDir();
		if (outputDir.isEmpty())
		{
			outputDir = mAppConfiguration::getInstance()->_workingDir
				+ QString("/%1/%2").arg(modelName, dirTaskName);
		}
		return QDir(outputDir).absolutePath();
	}

	QString mSolveTaskCenter::taskSolverFile(const mSolveTaskData* task, int modelID)
	{
		return taskOutputDir(task, modelID) + QString("/%1.mx").arg(sanitizePathSegment(task->getTaskName()));
	}

	quint64 mSolveTaskCenter::submitTask(int modelID, int taskID, QString* errMsg)
	{
		auto fail = [errMsg](const QString& msg) -> quint64
		{
			if (errMsg != nullptr)
			{
				*errMsg = msg;
			}
			return 0;
		};

		//按modelID定位模型（不依赖当前模型）
		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(modelID);
		if (viewportModel == nullptr || viewportModel->getModelData() == nullptr)
		{
			return fail(QString("模型（ID:%1）不存在！").arg(modelID));
		}
		mModelData* modelData = viewportModel->getModelData();

		//定位任务
		mSolveTaskData* task = modelData->getManager<mSolveTaskManager>(MxFunctions::Physical_SolveTask).getTask(taskID);
		if (task == nullptr)
		{
			return fail("任务不存在！");
		}

		//同一任务不允许重复排队
		const QVector<mSolveRunRecord*> actives = getActiveRunsOfModel(modelID);
		for (const mSolveRunRecord* run : actives)
		{
			if (run->getTaskID() == taskID)
			{
				return fail(QString("任务\"%1\"已在队列中或正在求解！").arg(task->getTaskName()));
			}
		}

		//V1约束：计算文件必须已由用户"写出文件"动作生成，轮到队列执行时绝不自动补写
		QString solverFile = task->getSolverFile();
		if (solverFile.isEmpty() || !QFileInfo::exists(solverFile))
		{
			return fail(QString("任务\"%1\"尚未写出计算文件，请先点击\"写出文件\"！").arg(task->getTaskName()));
		}

		//计算目录：与"写出文件"共用同一计算（含目录段非法字符清理与查重）
		QString calDir = taskOutputDir(task, modelID);

		//计算目录查重：目录已被排队/运行中的任务占用时拒绝提交（默认路径靠模型段天然隔离，
		//此检查主要拦截自定义路径撞车——两个任务互写互删同一目录会破坏对方的计算文件与结果）
		for (auto iter = _runs.begin(); iter != _runs.end(); ++iter)
		{
			const mSolveRunRecord* other = iter.value();
			if (other->isFinished())
			{
				continue;
			}
			if (QString::compare(QDir(other->getCalDir()).absolutePath(), calDir, Qt::CaseInsensitive) == 0)
			{
				return fail(QString("计算目录\"%1\"已被任务\"%2\"占用！").arg(calDir, other->getTaskName()));
			}
		}

		//同任务重复提交时覆盖旧记录（一个任务只保留最近一次运行）
		removeRunsOfTask(modelID, taskID);

		//生成运行记录：只设identity/路径/状态，配置委托给task读取（不拷贝）
		mSolveRunRecord* run = new mSolveRunRecord(task);
		run->setRunID(_nextRunID++);
		run->setModelID(modelID);
		run->setModelName(modelDirToken(modelID));
		run->setCalDir(calDir);
		run->setLogFile(calDir + QString("/%1_solve.log").arg(sanitizePathSegment(task->getTaskName())));
		run->setStatus(mSolveRunRecord::Status_Queued);
		run->setSubmittedAt(QDateTime::currentMSecsSinceEpoch());

		_runs.insert(run->getRunID(), run);
		_modelActives[modelID].push_back(run->getRunID());
		emit runStatusChanged(run->getRunID());

		//核数软提醒：本任务与运行中任务核数合计超过机器逻辑核数时提示（不阻断提交）
		int idealCores = QThread::idealThreadCount();
		if (idealCores > 0)
		{
			int usedCores = run->getCpuCores();
			for (auto iter = _runs.begin(); iter != _runs.end(); ++iter)
			{
				const mSolveRunRecord* other = iter.value();
				if (other->getStatus() == mSolveRunRecord::Status_Running)
				{
					usedCores += other->getCpuCores();
				}
			}
			if (usedCores > idealCores)
			{
				mGlobalSignals::getInstance()->outputMessageSig(QString(
					"提示：任务\"%1\"提交后各任务核数合计 %3 将超过机器逻辑核数 %2，多任务会分时竞争CPU！")
					.arg(task->getTaskName()).arg(idealCores).arg(usedCores));
			}
		}

		//立即尝试调度（并行额度空闲时直接开始）
		scheduleNext(modelID);

		return run->getRunID();
	}

	bool mSolveTaskCenter::cancelRun(quint64 runID)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || run->getStatus() != mSolveRunRecord::Status_Queued)
		{
			return false;
		}
		finishRun(runID, mSolveRunRecord::Status_Cancelled);
		return true;
	}

	bool mSolveTaskCenter::interruptRun(quint64 runID)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || run->getStatus() != mSolveRunRecord::Status_Running)
		{
			return false;
		}
		QProcess* process = _processes.value(runID, nullptr);
		if (process != nullptr)
		{
			//标记中断中：进程结束回调据此判定为"已中断"而非"失败"
			_interrupting.insert(runID);
			process->kill();
			return true;
		}
		//无进程（异常场景）直接按中断收尾
		finishRun(runID, mSolveRunRecord::Status_Interrupted);
		return true;
	}

	bool mSolveTaskCenter::moveRunOrder(quint64 runID, int delta)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || run->getStatus() != mSolveRunRecord::Status_Queued)
		{
			return false;
		}
		QVector<quint64>& actives = _modelActives[run->getModelID()];
		int index = actives.indexOf(runID);
		int target = index + delta;
		if (index < 0 || target < 0 || target >= actives.size())
		{
			return false;
		}
		//仅与同为排队的run交换（不打断运行中的）
		if (_runs.value(actives[target])->getStatus() != mSolveRunRecord::Status_Queued)
		{
			return false;
		}
		std::swap(actives[index], actives[target]);
		emit runStatusChanged(runID);
		return true;
	}

	bool mSolveTaskCenter::removeRun(quint64 runID)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || !run->isFinished())
		{
			return false;
		}
		_logTails.remove(runID);
		_runPostKeys.remove(runID);
		_runs.remove(runID);
		delete run;
		//通知其它界面同步（记录已删，接收方按run==nullptr全量刷新）
		emit runStatusChanged(runID);
		return true;
	}

	void mSolveTaskCenter::removeRunsOfTask(int modelID, int taskID)
	{
		//收集后删除（遍历中不能直接改容器）
		QVector<quint64> runIDs;
		for (auto iter = _runs.begin(); iter != _runs.end(); ++iter)
		{
			if (iter.value()->getModelID() == modelID && iter.value()->getTaskID() == taskID)
			{
				runIDs.push_back(iter.key());
			}
		}
		for (quint64 runID : runIDs)
		{
			removeRun(runID);
		}
	}

	void mSolveTaskCenter::removeRunsOfModel(int modelID)
	{
		//模型即将销毁：其全部run记录作废（任意状态）——run配置委托的task指针
		//将随模型析构失效，不能等到进程结束再收尾；先收集再删除（遍历中不能改容器）
		QVector<quint64> runIDs;
		for (auto iter = _runs.begin(); iter != _runs.end(); ++iter)
		{
			if (iter.value()->getModelID() == modelID)
			{
				runIDs.push_back(iter.key());
			}
		}
		for (quint64 runID : runIDs)
		{
			mSolveRunRecord* run = _runs.value(runID, nullptr);
			if (run == nullptr)
			{
				continue;
			}
			//运行中的进程就地终止：对象延迟销毁，其后续回调按run已删除直接返回
			if (QProcess* process = _processes.take(runID))
			{
				_interrupting.remove(runID);
				process->kill();
				process->deleteLater();
			}
			if (QFile* logFile = _logFiles.take(runID))
			{
				logFile->close();
				delete logFile;
			}
			_logTails.remove(runID);
			_runPostKeys.remove(runID);
			removeFromActive(runID);
			_runs.remove(runID);
			delete run;
		}
		if (!runIDs.isEmpty())
		{
			//通知各界面全量刷新（记录已删，接收方按run==nullptr处理）
			emit runStatusChanged(runIDs.first());
		}
	}

	mSolveRunRecord* mSolveTaskCenter::getRun(quint64 runID) const
	{
		return _runs.value(runID, nullptr);
	}

	QVector<mSolveRunRecord*> mSolveTaskCenter::getRunsOfModel(int modelID) const
	{
		QVector<mSolveRunRecord*> runs;
		for (auto iter = _runs.begin(); iter != _runs.end(); ++iter)
		{
			if (iter.value()->getModelID() == modelID)
			{
				runs.push_back(iter.value());
			}
		}
		//按runID升序即提交顺序
		std::sort(runs.begin(), runs.end(),
			[](const mSolveRunRecord* a, const mSolveRunRecord* b) { return a->getRunID() < b->getRunID(); });
		return runs;
	}

	QVector<mSolveRunRecord*> mSolveTaskCenter::getActiveRunsOfModel(int modelID) const
	{
		QVector<mSolveRunRecord*> runs;
		for (quint64 runID : _modelActives.value(modelID))
		{
			mSolveRunRecord* run = _runs.value(runID, nullptr);
			if (run != nullptr)
			{
				runs.push_back(run);
			}
		}
		return runs;
	}

	QVector<mSolveRunRecord*> mSolveTaskCenter::getAllActiveRuns() const
	{
		QVector<mSolveRunRecord*> runs;
		for (auto iter = _modelActives.begin(); iter != _modelActives.end(); ++iter)
		{
			for (quint64 runID : iter.value())
			{
				mSolveRunRecord* run = _runs.value(runID, nullptr);
				if (run != nullptr)
				{
					runs.push_back(run);
				}
			}
		}
		return runs;
	}

	bool mSolveTaskCenter::isModelBusy(int modelID) const
	{
		return !_modelActives.value(modelID).isEmpty();
	}

	bool mSolveTaskCenter::isBusy() const
	{
		if (_quickWriteThread != nullptr)
		{
			return true;
		}
		for (auto iter = _modelActives.begin(); iter != _modelActives.end(); ++iter)
		{
			if (!iter.value().isEmpty())
			{
				return true;
			}
		}
		return false;
	}

	void mSolveTaskCenter::setMaxParallelPerModel(int n)
	{
		_maxParallelPerModel = n < 1 ? 1 : n;
		//并行度提高后尝试补启各模型排队任务
		for (auto iter = _modelActives.begin(); iter != _modelActives.end(); ++iter)
		{
			scheduleNext(iter.key());
		}
	}

	void mSolveTaskCenter::scheduleNext(int modelID)
	{
		QVector<quint64>& actives = _modelActives[modelID];
		int runningCount = 0;
		for (quint64 runID : actives)
		{
			if (_runs.value(runID)->getStatus() == mSolveRunRecord::Status_Running)
			{
				runningCount++;
			}
		}
		while (runningCount < _maxParallelPerModel)
		{
			//队首排队任务（活动队列中第一个Queued）
			quint64 next = 0;
			for (quint64 runID : actives)
			{
				if (_runs.value(runID)->getStatus() == mSolveRunRecord::Status_Queued)
				{
					next = runID;
					break;
				}
			}
			if (next == 0)
			{
				break;
			}
			startRun(next);
			runningCount++;
		}
	}

	void mSolveTaskCenter::startRun(quint64 runID)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || run->getStatus() != mSolveRunRecord::Status_Queued)
		{
			return;
		}
		run->setStatus(mSolveRunRecord::Status_Running);
		run->setStartedAt(QDateTime::currentMSecsSinceEpoch());
		emit runStatusChanged(runID);
		emit runStarted(runID);

		//本次运行重建日志文件（追加模式下旧运行内容会残留）
		QFile::remove(run->getLogFile());

		//日志头
		const QString stamp = logStamp();
		QStringList header;
		header << stamp + "任务：" + run->getTaskName();
		header << stamp + "读取计算文件：" + run->getSolverFile();
		header << stamp + QString("求解资源：%1核 / %2").arg(run->getCpuCores()).arg(memoryStrategyName(run->getMemoryStrategy()));
		header << stamp + QString("工况数量：%1").arg(run->getCaseIDs().size());
		header << stamp + "启动求解";
		appendLog(runID, header.join('\n') + '\n');

		launchProcess(runID);
	}

	void mSolveTaskCenter::launchProcess(quint64 runID)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr)
		{
			return;
		}

		//确定求解器路径
		QString solverpath;
#if defined(_WIN32)
		solverpath = QDir(qApp->applicationDirPath() + "/mxlibi/MxSim.exe").absolutePath();
#else
		solverpath = QDir(qApp->applicationDirPath() + "/mxlibi/MxSim").absolutePath();
#endif
		solverpath = QDir::toNativeSeparators(solverpath);
		QString solvefile = QDir::toNativeSeparators(run->getSolverFile());

		//判断求解器是否存在
		QFile exefile(solverpath);
		if (!exefile.exists())
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QString("[%1/%2] Solver does not exist").arg(run->getModelName(), run->getTaskName()));
			finishRun(runID, mSolveRunRecord::Status_Failed);
			// mpcomm：求解器缺失属于启动前失败，同样按失败回报（EXECUTE_ACTION
			// 返回 pending，终态只能由此信号带回，否则平台会一直等待）
			emit mMechGlobalSignals::getInstance()
				->mpcommSolverStatusChanged(QStringLiteral("failed"), QVariantMap{});
			return;
		}
		//判断求解文件是否存在
		QFile mxfile(solvefile);
		if (!mxfile.exists())
		{
			mGlobalSignals::getInstance()->outputMessageSig(
				QString("[%1/%2] Input file does not exist").arg(run->getModelName(), run->getTaskName()));
			finishRun(runID, mSolveRunRecord::Status_Failed);
			// mpcomm：同上，启动前失败按失败回报
			emit mMechGlobalSignals::getInstance()
				->mpcommSolverStatusChanged(QStringLiteral("failed"), QVariantMap{});
			return;
		}
#ifndef _WIN32
#include <sys/stat.h>
		//判断Linux下文件是否为可执行程序
		stat st;
		if (stat(solverpath.toStdString().c_str(), &st) != 0)
		{
			finishRun(runID, mSolveRunRecord::Status_Failed);
			return;
		}
		if (!S_ISREG(st.st_mode))
		{
			finishRun(runID, mSolveRunRecord::Status_Failed);
			return;
		}
		if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH) == 0)
		{
			finishRun(runID, mSolveRunRecord::Status_Failed);
			return;
		}
#endif

		//启动前确保计算目录存在（日志与结果文件写入其中）
		QDir(run->getCalDir()).mkpath(".");

		//命令行参数（核数取自任务快照）
		QStringList cmdparas{ "-i", solvefile, "-t", QString::number(run->getCpuCores()), "-nosplash" };

		QProcess* process = new QProcess;
		_processes.insert(runID, process);

		//求解器输出（stdout/stderr）进入该run的日志链路：
		//内存缓冲+落盘+logAppended信号，并由appendLog带[模型/任务]前缀回显到信息窗口
		//（信息窗口回显后续将移除，届时只改appendLog末尾一处）
		connect(process, &QProcess::readyReadStandardOutput, this, [this, runID]()
			{
				QProcess* proc = _processes.value(runID, nullptr);
				if (proc != nullptr)
				{
					appendLog(runID, toUnicodeText(proc->readAll()));
				}
			});
		connect(process, &QProcess::readyReadStandardError, this, [this, runID]()
			{
				QProcess* proc = _processes.value(runID, nullptr);
				if (proc != nullptr)
				{
					appendLog(runID, toUnicodeText(proc->readAllStandardError()));
				}
			});

		//进程结束回调携带runID，天然知道是哪个模型哪个任务结束
		connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
			[this, runID](int exitCode, QProcess::ExitStatus status)
			{
				onProcessFinished(runID, exitCode, status);
			});

		//启动失败兜底：FailedToStart不会触发finished（start前的存在性检查存在竞态窗口，
		//如检查后被杀毒软件隔离），不处理run会永久卡在"求解中"且进程对象泄漏
		connect(process, &QProcess::errorOccurred, this, [this, runID](QProcess::ProcessError error)
			{
				if (error == QProcess::FailedToStart)
				{
					onProcessFinished(runID, -1, QProcess::CrashExit);
				}
			});

		process->start(solverpath, cmdparas);

		// mpcomm：求解器进程真正启动，回报平台（changed(solution)）
		emit mMechGlobalSignals::getInstance()
			->mpcommSolverStatusChanged(QStringLiteral("started"), QVariantMap{});
	}

	void mSolveTaskCenter::onProcessFinished(quint64 runID, int exitCode, QProcess::ExitStatus status)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || run->isFinished())
		{
			return;
		}

		//进程对象清理
		QProcess* process = _processes.take(runID);
		if (process != nullptr)
		{
			process->deleteLater();
		}
		const bool interrupted = _interrupting.remove(runID);

		//按modelID定位模型（不依赖当前模型）
		mViewportModel* viewportModel = mViewportModelManager::getInstance()->getViewportModel(run->getModelID());
		mModelData* modelData = (viewportModel != nullptr ? viewportModel->getModelData() : nullptr);

		//任务内工况逐个挂结果目录
		auto setCaseSolveDir = [modelData, &run](const QString& dir)
		{
			if (modelData == nullptr)
			{
				return;
			}
			for (int caseID : run->getCaseIDs())
			{
				mCaseBase* caseBase = modelData->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
				if (caseBase != nullptr)
				{
					caseBase->setSolveDir(dir);
				}
			}
		};

		if (status == QProcess::NormalExit && !interrupted)
		{
			setCaseSolveDir(run->getCalDir());
			mGlobalSignals::getInstance()->outputMessageSig(
				QString("[%1/%2] Calculation complete!").arg(run->getModelName(), run->getTaskName()));
			finishRun(runID, mSolveRunRecord::Status_Done, exitCode);

			// mpcomm：求解完成回报（resultFiles=求解目录下的具体结果文件，
			// 与后处理的发现规则一致：*.mxdb/*.mxdb0/*.op2）
			{
				QStringList resultFiles;
				QDir resultDir(run->getCalDir());
				resultDir.setFilter(QDir::Files | QDir::NoSymLinks);
				resultDir.setNameFilters({ "*.mxdb", "*.mxdb0", "*.op2" });
				for (const QString& fileName : resultDir.entryList())
				{
					resultFiles.append(resultDir.absoluteFilePath(fileName));
				}

				QVariantMap outputs;
				outputs.insert(QStringLiteral("resultFiles"), resultFiles);
				emit mMechGlobalSignals::getInstance()
					->mpcommSolverStatusChanged(QStringLiteral("finished"), outputs);
			}
			//勾选了"自动打开后处理"的任务：完成后直接新开后处理模型展示结果
			//（必须在finishRun置Done之后调——openRunResultInNewPost入口校验Status_Done）
			if (run->isAutoOpenPost())
			{
				openRunResultInNewPost(runID);
			}
		}
		else
		{
			setCaseSolveDir("");
			for (int caseID : run->getCaseIDs())
			{
				mMechGlobalSignals::getInstance()->updateStatusPostResult(run->getModelID(), caseID);
			}
			mGlobalSignals::getInstance()->outputMessageSig(interrupted
				? QString("[%1/%2] Solve interrupted!").arg(run->getModelName(), run->getTaskName())
				: QString("[%1/%2] Calculation failed!").arg(run->getModelName(), run->getTaskName()));
			finishRun(runID, interrupted ? mSolveRunRecord::Status_Interrupted : mSolveRunRecord::Status_Failed, exitCode);

			// mpcomm：求解失败/被中断回报（平台侧只有 failed 终态，中断同按失败处理）
			emit mMechGlobalSignals::getInstance()
				->mpcommSolverStatusChanged(QStringLiteral("failed"), QVariantMap{});
		}
	}

	void mSolveTaskCenter::finishRun(quint64 runID, mSolveRunRecord::RunStatus status, int exitCode)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || run->isFinished())
		{
			return;
		}
		run->setStatus(status);
		run->setFinishedAt(QDateTime::currentMSecsSinceEpoch());
		if (exitCode != -1)
		{
			run->setExitCode(exitCode);
		}

		//日志尾并关闭日志文件
		const QString duration = run->getStartedAt() > 0
			? durationText(run->getFinishedAt() - run->getStartedAt()) : QString("-");
		appendLog(runID, QString("%1%2 · 总耗时 %3\n").arg(logStamp(), runStatusText(status), duration));
		if (QFile* logFile = _logFiles.take(runID))
		{
			logFile->flush();
			logFile->close();
			delete logFile;
		}

		int modelID = run->getModelID();
		removeFromActive(runID);
		emit runStatusChanged(runID);
		//该模型腾出并行额度，继续调度
		scheduleNext(modelID);
	}

	void mSolveTaskCenter::removeFromActive(quint64 runID)
	{
		mSolveRunRecord* run = getRun(runID);
		if (run == nullptr)
		{
			return;
		}
		QVector<quint64>& actives = _modelActives[run->getModelID()];
		actives.removeOne(runID);
		if (actives.isEmpty())
		{
			_modelActives.remove(run->getModelID());
		}
	}

	QString mSolveTaskCenter::getRunLogTail(quint64 runID) const
	{
		return _logTails.value(runID);
	}

	bool mSolveTaskCenter::readRunLogFile(quint64 runID, QString& content) const
	{
		const mSolveRunRecord* run = getRun(runID);
		if (run == nullptr || run->getLogFile().isEmpty())
		{
			return false;
		}
		QFile file(run->getLogFile());
		if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return false;
		}
		content = QString::fromUtf8(file.readAll());
		return true;
	}

	void mSolveTaskCenter::appendLog(quint64 runID, const QString& text)
	{
		if (text.isEmpty())
		{
			return;
		}
		const mSolveRunRecord* run = getRun(runID);
		if (run == nullptr)
		{
			return;
		}

		//内存尾部缓冲（完整历史在日志文件，缓冲超限丢弃头部）
		QString& tail = _logTails[runID];
		tail += text;
		if (tail.size() > LogTailLimit)
		{
			tail = tail.right(LogTailLimit - LogTailLimit / 8);
		}

		//落盘（追加，惰性打开）
		QFile* logFile = logFileOf(runID);
		if (logFile != nullptr && logFile->isOpen())
		{
			logFile->write(text.toUtf8());
			logFile->flush();
		}

		//界面订阅者（日志查看器等）实时接收
		emit logAppended(runID, text);

		//回显到信息窗口（带模型/任务前缀，多任务输出可区分）
		mGlobalSignals::getInstance()->outputMessageSig(
			QString("[%1/%2] %3").arg(run->getModelName(), run->getTaskName(), text), false, false);
	}

	QFile* mSolveTaskCenter::logFileOf(quint64 runID)
	{
		if (_logFiles.contains(runID))
		{
			return _logFiles.value(runID);
		}
		const mSolveRunRecord* run = getRun(runID);
		if (run == nullptr)
		{
			return nullptr;
		}
		//确保目录存在后打开追加模式日志文件；失败缓存nullptr避免每条输出重试
		QDir(run->getCalDir()).mkpath(".");
		QFile* logFile = new QFile(run->getLogFile());
		if (logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
		{
			_logFiles.insert(runID, logFile);
		}
		else
		{
			delete logFile;
			_logFiles.insert(runID, nullptr);
		}
		return _logFiles.value(runID);
	}
}
