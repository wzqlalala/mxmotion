#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

namespace StrengthReport
{
	/*
	* 引擎子进程封装（FR-53/M6，03 §4.5 mStrengthReportEngineRunner）：
	* 捆绑 python 两级确定性探测（B11/R13）+ qrc 引擎脚本释放（tempDir/tdr_engine/ 子目录，
	* 先 mkpath——B8）+ QProcess 异步（刻意不同于既有渲染器 waitForFinished 同步模式：
	* NFR-2 进度反馈+不卡 UI）+ stdout JSON-lines 阶段消息解析（T-14）。
	* 成败唯一判据=exitCode（T-16/A-05）：stderr 仅作诊断文本，引擎打印良性告警仍判成功。
	* 生命周期（T-15/A-04）：Qt5 QProcess 析构不终止子进程——析构/closeEvent 路径
	* kill()+waitForFinished() 收尾；超时看门狗 120s（A-09）kill+清理。
	* python 协议（批次 1 实测）：CLI=<python> [-u] tdr_engine.py <container> <dataset> <output>；
	* stdout 行={"stage":"verify|load|locate|transform|pack|done|failed","pct":N,"msg":...}；
	* 退出码 0=成功/1=引擎失败/2=用法错误；stderr=[warn] 告警行与异常栈。
	*/
	class MPOSTWINDOW_EXPORT mStrengthReportEngineRunner : public QObject
	{
		Q_OBJECT
	public:
		explicit mStrengthReportEngineRunner(QObject* parent = nullptr);
		~mStrengthReportEngineRunner() override;   //T-15：析构 kill+waitForFinished 收尾

		//两级确定性探测序（B11/R13）：①resolveBundledPythonExe（appDir/python，不改既有函数）；
		//②appDir/../tools/python/python.exe 命中即用（覆盖发布形态 appDir=Application→包根）
		static QString resolvePythonExe(QStringList* searchedPaths = nullptr);

		//引擎脚本解析（T-02-a）：磁盘候选 appDir/../tools/tdr_engine/tdr_engine.py 与 qrc 内容
		//字节比对（mPostWordReportRenderer.cpp:193-211 先例）——一致用磁盘，陈旧/缺失→qrc
		//释放 tempDir/tdr_engine/（先 QDir::mkpath 子目录——B8；三引擎脚本同目录释放，
		//引擎以自身所在目录 sys.path 导入同目录 tdroox/tdrrules）
		static QString resolveEngineScript(QString& err);

		//启动引擎（异步）。启动前三预检（03 §4.5，顺序执行任一失败阻止启动）：
		//①输出目录 mkpath+试写预检（C-06/T-19）；②输出目标占用探测（T-13/A-01/C-10：
		//对 <out> 试读写打开——不截断不删除，被占用即报"文件可能正被打开"）；
		//③glob 清理残留 <out>.tdr_tmp*（A-09/C-08：前缀匹配与 pid 解耦，禁止宽模式删除）
		bool start(const QString& containerPath, const QString& datasetJsonPath,
			const QString& outputPath, QString& err);

		//取消：kill 子进程+按输出名清理本引擎临时件（03"取消=kill+清理 tmp"）
		void cancel();

		bool isRunning() const;

	signals:
		void stageProgress(const QString& stage, int pct, const QString& msg);   //JSON-lines → 进度条
		void finished(bool ok, const QString& summary);                          //成败汇总（含缺项告警清单 FR-35）

	private slots:
		void onReadyStdout();
		void onReadyStderr();
		void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
		void onWatchdogTimeout();

	private:
		bool precheckOutputTarget(const QString& outputPath, QString& err);
		void cleanupTmpFiles(const QString& outputPath);

		QProcess* _process = nullptr;
		QTimer* _watchdog = nullptr;
		QString _outputPath;
		QString _stdoutRemainder;   //JSON-lines 半行缓冲（readyRead 按行切分）
		QString _failedMsg;         //failed 阶段消息（引擎结构化错误行）
		QStringList _failedErrors;  //failed 附带条目级错误
		QString _stderrTail;        //stderr 尾部诊断文本（崩溃时取尾部，限长防内存膨胀）
		bool _cancelRequested = false;
		bool _timedOut = false;     //CB-07：看门狗超时标志（汇总区分"超时"与用户"取消"）
	};
}
