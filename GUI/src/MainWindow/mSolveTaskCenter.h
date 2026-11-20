#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QObject>
#include <QHash>
#include <QVector>
#include <QSet>
#include <QProcess>

//BasicData
#include "mSolveTaskData.h"
#include "mSolveRunRecord.h"

class QFile;

namespace MechIOFile
{
	class mOThirdFile;
}

namespace MechMainWindow
{
	/*
	* 求解任务执行中心（全局单例）
	* - 提交：校验 -> 生成任务不可变快照（run）-> 入队
	* - 调度：每模型独立队列，同模型同时最多 maxParallelPerModel 个运行，
	*   模型之间互不阻塞（各自队列各自调度）
	* - 进程启动/结果落地在后续步骤接入（startRun 内挂接QProcess）
	* 所有接口只在主线程调用
	*/
	class MMAINWINDOW_EXPORT mSolveTaskCenter : public QObject
	{
		Q_OBJECT

	public:
		/*
		* 获取单例指针
		*/
		static mSolveTaskCenter* getInstance();

		/*
		* 快速求解：创建/复用 Solve_Model<模型ID> 任务（含全部工况、默认参数、默认输出目录），
		* 然后走通用"写出文件→提交"流程。不再使用单独的目录规则和参数快照，
		* 与手动建任务提交完全同路径。
		*/
		void quickSolve(int modelID, int caseID);

		/*
		* 中断所有运行中的run（程序关闭等场景）
		*/
		void interruptAll();

		/*
		* 查看run结果：已打开过则跳转到已有后处理模型，否则新开一个后处理模型导入结果
		*/
		void viewRunResult(quint64 runID);

		/*
		* 新开后处理：为该run创建一个后处理模型并导入其结果文件，
		* 成功后记录runID->后处理模型ID映射（可多次新开，映射更新为最新）
		*/
		bool openRunResultInNewPost(quint64 runID);

		/*
		* 跳转已有：切到该run已打开的后处理模型并激活其后处理视口（无有效模型返回false）
		*/
		bool jumpToRunPostModel(quint64 runID);

		/*
		* 该run的结果是否已打开过（存在仍存活的后处理模型）
		*/
		bool hasRunPostModel(quint64 runID) const;

		/*
		* 计算任务输出目录（绝对路径）：自定义目录优先，否则默认 工作目录/模型目录段/任务名
		* 目录段做非法字符清理；提交run与"写出文件"共用此计算，保证两处路径一致
		*/
		static QString taskOutputDir(const MechSolveTaskData::mSolveTaskData* task, int modelID);

		/*
		* 模型目录段（Model_<id>_<首用时间戳>，会话内稳定）：modelDataID每次启动从1重排，
		* 跨会话同ID会撞目录，带时间戳保证不同次软件处理互不覆盖
		*/
		QString modelDirToken(int modelID);

		/*
		* 任务计算文件默认路径（输出目录/任务名.mx），用于"写出文件"
		*/
		static QString taskSolverFile(const MechSolveTaskData::mSolveTaskData* task, int modelID);

		/*
		* 提交任务求解：对任务配置做快照并入队
		* 返回runID（全局唯一，跨模型）；失败返回0，errMsg带回原因
		*/
		quint64 submitTask(int modelID, int taskID, QString* errMsg = nullptr);

		/*
		* 取消排队中的run（移出队列，不产生求解）
		*/
		bool cancelRun(quint64 runID);

		/*
		* 中断运行中的run（终止求解器进程）
		*/
		bool interruptRun(quint64 runID);

		/*
		* 调整排队run在本模型队列中的顺序（delta=±1，仅与同为排队的run交换）
		*/
		bool moveRunOrder(quint64 runID, int delta);

		/*
		* 删除已结束的run记录（供记录页清理历史，运行中/排队中不可删）
		*/
		bool removeRun(quint64 runID);

		/*
		* 删除指定任务的全部已结束run记录（删除任务时同步清其完成记录）
		*/
		void removeRunsOfTask(int modelID, int taskID);

		/*
		* 删除指定模型的全部run记录（任意状态）：模型即将销毁时调用，
		* 运行中的求解进程就地终止，后续进程回调按run已删除直接返回
		*/
		void removeRunsOfModel(int modelID);

		/*
		* 获取run记录（含已完成的，本次会话内）
		*/
		MechSolveTaskData::mSolveRunRecord* getRun(quint64 runID) const;

		/*
		* 获取指定模型的全部run（含已完成，按提交顺序）
		*/
		QVector<MechSolveTaskData::mSolveRunRecord*> getRunsOfModel(int modelID) const;

		/*
		* 获取指定模型的活动run（排队+运行，队列顺序）
		*/
		QVector<MechSolveTaskData::mSolveRunRecord*> getActiveRunsOfModel(int modelID) const;

		/*
		* 获取全部活动run（排队+运行，所有模型）
		*/
		QVector<MechSolveTaskData::mSolveRunRecord*> getAllActiveRuns() const;

		/*
		* 指定模型是否有排队/运行中的任务
		*/
		bool isModelBusy(int modelID) const;

		/*
		* 是否有任意模型存在排队/运行中的任务（供脚本执行器轮询）
		*/
		bool isBusy() const;

		/*
		* 每模型并行度（同模型同时运行的任务数上限）
		*/
		int getMaxParallelPerModel() const { return _maxParallelPerModel; }
		void setMaxParallelPerModel(int n);

		/*
		* 获取run日志的内存尾部缓冲（实时查看用，超限截断；完整历史在日志文件）
		*/
		QString getRunLogTail(quint64 runID) const;

		/*
		* 读取run完整日志文件内容（完成后查看历史用），成功返回true
		*/
		bool readRunLogFile(quint64 runID, QString& content) const;

	signals:
		/*
		* run真正开始运行（进程启动）时发出
		*/
		void runStarted(quint64 runID);

		/*
		* run状态变化（入队/开始/结束/中断/取消），UI接收后刷新展示
		*/
		void runStatusChanged(quint64 runID);

		/*
		* run日志实时追加（日志链路接入后使用）
		*/
		void logAppended(quint64 runID, const QString& chunk);

	private:
		mSolveTaskCenter();

		~mSolveTaskCenter();

		/*
		* 调度：若该模型并行额度未满，将队首排队run置为运行
		*/
		void scheduleNext(int modelID);

		/*
		* 启动run（置运行态并记录开始时间，创建run专属求解器进程）
		*/
		void startRun(quint64 runID);

		/*
		* 创建并启动该run专属的求解器进程（lambda捕获runID绑定回调）
		* 求解器不存在/计算文件不存在时直接按失败收尾
		*/
		void launchProcess(quint64 runID);

		/*
		* 求解器进程结束回调：按run上下文落结果（按modelID定位模型，不依赖当前模型），
		* 之后统一走finishRun收尾
		*/
		void onProcessFinished(quint64 runID, int exitCode, QProcess::ExitStatus status);

		/*
		* 结束run（统一收尾：置结束状态/时间，移出活动队列，继续调度该模型）
		*/
		void finishRun(quint64 runID, MechSolveTaskData::mSolveRunRecord::RunStatus status, int exitCode = -1);

		/*
		* 将run从所属模型的活动队列移除
		*/
		void removeFromActive(quint64 runID);

		/*
		* 追加run日志：内存尾部缓冲 + 落盘 + 发送logAppended，
		* 并以"[模型/任务]"前缀回显到信息窗口
		*/
		void appendLog(quint64 runID, const QString& text);

		/*
		* 获取run日志文件（惰性打开，追加模式；打开失败缓存nullptr不再重试）
		*/
		QFile* logFileOf(quint64 runID);

		/*
		* 快速求解写文件线程结束：写成功则创建/复用快速任务并提交
		*/
		void submitQuickTask();

	private:
		//单例指针
		static mSolveTaskCenter* _instance;

		//下一个runID
		quint64 _nextRunID = 1;

		//全部run（含已完成，会话内；runID -> 记录）
		QHash<quint64, MechSolveTaskData::mSolveRunRecord*> _runs;

		//每模型活动队列（runID有序：排队在前，运行在后）
		QHash<int, QVector<quint64>> _modelActives;

		//运行中run的求解器进程（runID -> 进程）
		QHash<quint64, QProcess*> _processes;

		//已发出中断（kill）等待进程结束回调的run
		QSet<quint64> _interrupting;

		//runID -> 已创建的后处理结果key（PostCommon结果服务的不透明句柄，只保存/透传）
		QHash<quint64, QString> _runPostKeys;

		//模型ID -> 模型目录段缓存（会话内稳定，见modelDirToken说明）
		QHash<int, QString> _modelDirTokens;

		//每run日志内存尾部缓冲（限制大小，完整历史在日志文件）
		QHash<quint64, QString> _logTails;

		//每run日志文件（run运行期间持有，结束关闭）
		QHash<quint64, QFile*> _logFiles;

		//日志尾部缓冲上限（字符数）
		enum { LogTailLimit = 64 * 1024 };

		//每模型并行度
		int _maxParallelPerModel = 1;

		//快速求解写文件线程（同一时刻至多一个）
		MechIOFile::mOThirdFile* _quickWriteThread = nullptr;

		//快速求解提交上下文（任务创建后走通用路径写文件+提交）
		int _quickModelID = -1;
		int _quickTaskID = -1;
	};
}
