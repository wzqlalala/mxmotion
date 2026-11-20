#include "mStrengthReportEngineRunner.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>           //resolvePythonExe 第③级 Linux 系统 python3 兜底

#include "mCommonFunctions.h"        //resolveBundledPythonExe：python 第一级探测（冻结函数只调用）
#include "mPostReportRenderer.h"     //PostData::releaseQrcResource：qrc 内容校验释放（先例 mPostWordReportRenderer.cpp:193-211）

namespace StrengthReport
{
	namespace
	{
		//E10：独立临时目录（与既有 mx_word_export_tmp 隔离）
		QString strengthReportTempDir()
		{
			return QDir::temp().absoluteFilePath(QStringLiteral("mx_strength_report_tmp"));
		}
	}

	mStrengthReportEngineRunner::mStrengthReportEngineRunner(QObject* parent)
		: QObject(parent)
	{
	}

	mStrengthReportEngineRunner::~mStrengthReportEngineRunner()
	{
		//T-15/A-04：Qt5 QProcess 析构不终止子进程——宿主退出/对话框析构路径上 kill+等待收尾，
		//防止导出中关闭对话框后遗留 python 子进程继续写输出文件。
		//CB-06：先停看门狗并断开子进程→本对象的全部信号，再 kill+waitForFinished——
		//waitForFinished 期间 finished 为同线程同步派发，旧写法回调链
		//onProcessFinished→emit finished→Dlg::slot_onExportFinished→setExporting 触碰
		//兄弟 UI 子控件，存活依赖"子对象先于 UI 创建"的顺序侥幸；先断开消除该脆弱依赖
		if (_watchdog != nullptr)
			_watchdog->stop();
		if (_process != nullptr)
		{
			_process->disconnect(this);
			if (_process->state() != QProcess::NotRunning)
			{
				_process->kill();
				_process->waitForFinished(5000);
			}
		}
		//kill/崩溃残留 tmp 兜底清理（与失败/取消路径同口径；_outputPath 未启动过=空串跳过）
		if (!_outputPath.isEmpty())
			cleanupTmpFiles(_outputPath);
	}

	QString mStrengthReportEngineRunner::resolvePythonExe(QStringList* searchedPaths)
	{
		//两级确定性探测序（B11/R13）：①既有捆绑探测（appDir/python/python.exe|python_d.exe）；
		//②appDir/../tools/python/python.exe 命中即用——Windows 发布包 python 实际在 tools/python
		//（Auto_Build packager.py:978），Application/ 下无 python 目录（03 §5 R13 断点兜底）；
		//③T-02-a 布局修订（用户指示 2026-11-13）：前两级未命中且非 Windows（Linux 发布包
		//无捆绑 python）→系统 python3 兜底（脚本为 3.7 兼容语法可在 python3.x 运行）。
		//只在新代码实现，不改既有函数（冻结纪律）
		QStringList probed;
		const QString first = PostCommon::mCommonFunctions::resolveBundledPythonExe(&probed);
		if (!first.isEmpty())
			return first;
		const QString second = QCoreApplication::applicationDirPath()
			+ QStringLiteral("/../tools/python/python.exe");
		probed.append(second);
		if (QFile::exists(second))
			return second;
#ifndef Q_OS_WIN
		//Linux 兜底：系统 python3（命中即用并记录探测路径）
		const QString systemPython3
			= QStandardPaths::findExecutable(QStringLiteral("python3"));
		if (!systemPython3.isEmpty())
		{
			probed.append(systemPython3);
			return systemPython3;
		}
		//PK-03：未命中不 append 空串（防失败文案"已探测：%1"尾部出现 "; " 空项）
#endif
		if (searchedPaths != nullptr)
			*searchedPaths = probed;
		return QString();
	}

	QString mStrengthReportEngineRunner::resolveEngineScript(QString& err)
	{
		err.clear();
		const QString qrcPath = QStringLiteral(":/mMainWidgets/scripts/tdr_engine/tdr_engine.py");
		//T-02-a 布局修订（用户指示 2026-11-13）：磁盘候选（发布形态 Application/tdr_engine
		//随包落盘，与 exe 同级）与 qrc 字节比对——一致用磁盘；
		//开发树无 Application/tdr_engine（候选缺失）→走 qrc 释放临时目录，逻辑同先例分支
		const QString diskCandidate = QCoreApplication::applicationDirPath()
			+ QStringLiteral("/tdr_engine/tdr_engine.py");
		{
			QFile diskScript(diskCandidate);
			QFile qrcScript(qrcPath);
			if (diskScript.open(QIODevice::ReadOnly) && qrcScript.open(QIODevice::ReadOnly)
				&& diskScript.readAll() == qrcScript.readAll())
			{
				return diskCandidate;
			}
		}
		//释放目标=tempDir 子目录 tdr_engine/（B8：QFile 不自动建父目录，必须先 mkpath——
		//既有 releaseQrcResource 先例均为平铺文件释放，子目录释放易踩）
		const QString targetDir = strengthReportTempDir() + QStringLiteral("/tdr_engine");
		if (!QDir().mkpath(targetDir))
		{
			err = QStringLiteral("引擎脚本临时目录创建失败：%1").arg(targetDir);
			return QString();
		}
		//三引擎脚本同目录释放（引擎以自身所在目录 sys.path 导入同目录 tdroox/tdrrules）
		const QStringList engineScripts
		{
			QStringLiteral("tdr_engine.py"),
			QStringLiteral("tdroox.py"),
			QStringLiteral("tdrrules.py")
		};
		for (const QString& script : engineScripts)
		{
			const QString qrc = QStringLiteral(":/mMainWidgets/scripts/tdr_engine/%1").arg(script);
			QString releaseErr;
			//releaseQrcResource 内部按内容比对：目标已存在且与 qrc 一致时复用，陈旧则删除重释放
			if (!PostData::releaseQrcResource(qrc, targetDir + QStringLiteral("/%1").arg(script),
				releaseErr))
			{
				err = QStringLiteral("引擎脚本释放失败（%1）：%2").arg(script, releaseErr);
				return QString();
			}
		}
		return targetDir + QStringLiteral("/tdr_engine.py");
	}

	bool mStrengthReportEngineRunner::precheckOutputTarget(const QString& outputPath, QString& err)
	{
		const QFileInfo outInfo(outputPath);
		//预检①：输出目录 mkpath+试写预检（C-06/T-19；>260 字符长路径失败同文案兜底）
		const QString dir = outInfo.absolutePath();
		if (!QDir().mkpath(dir))
		{
			err = QStringLiteral("输出目录不可写或路径无效：%1").arg(dir);
			return false;
		}
		const QString probeFile = dir + QStringLiteral("/tdr_write_probe_%1.tmp")
			.arg(QCoreApplication::applicationPid());
		{
			QFile probe(probeFile);
			if (!probe.open(QIODevice::WriteOnly | QIODevice::Truncate))
			{
				err = QStringLiteral("输出目录不可写或路径无效（含 >260 字符长路径）：%1").arg(dir);
				return false;
			}
			probe.close();
		}
		QFile::remove(probeFile);

		//预检②：输出目标占用探测（T-13/A-01/C-10：取代 pre-delete——被占用即阻止，
		//不删旧文件，引擎 os.replace 保旧文件语义成立）。ReadWrite 打开不截断既有内容。
		//CB-10：先查只读/无写入权限属性分流文案，避免只读文件被误报为"被其他程序占用"
		if (QFile::exists(outputPath))
		{
			const QFileInfo targetInfo(outputPath);
			if (!targetInfo.isWritable())
			{
				err = QStringLiteral("输出文件为只读或当前用户无写入权限，请去除只读属性或更换路径：%1")
						  .arg(outputPath);
				return false;
			}
			QFile target(outputPath);
			if (!target.open(QIODevice::ReadWrite | QIODevice::ExistingOnly))
			{
				err = QStringLiteral("输出文件可能正被打开（Word 或其他程序占用），请关闭后重试：%1")
						  .arg(outputPath);
				return false;
			}
			target.close();
		}
		return true;
	}

	void mStrengthReportEngineRunner::cleanupTmpFiles(const QString& outputPath)
	{
		//A-09/C-08：仅按"输出文件名.tdr_tmp*"前缀精确匹配清理（与 pid 解耦），禁止宽模式删除
		const QFileInfo outInfo(outputPath);
		const QString dir = outInfo.absolutePath();
		const QString filter = outInfo.fileName() + QStringLiteral(".tdr_tmp*");
		const QStringList leftovers = QDir(dir).entryList(QStringList{ filter }, QDir::Files);
		for (const QString& name : leftovers)
			QFile::remove(dir + QStringLiteral("/") + name);
	}

	bool mStrengthReportEngineRunner::start(const QString& containerPath,
		const QString& datasetJsonPath, const QString& outputPath, QString& err)
	{
		err.clear();
		if (_process != nullptr && _process->state() != QProcess::NotRunning)
		{
			err = QStringLiteral("导出正在进行中，请先取消或等待完成");
			return false;
		}
		//启动前三预检（顺序执行任一失败给可读文案并阻止启动）
		if (!precheckOutputTarget(outputPath, err))
			return false;

		QStringList probed;
		const QString pythonExe = resolvePythonExe(&probed);
		if (pythonExe.isEmpty())
		{
			err = QStringLiteral("未找到可用的 python 解释器（已探测：%1）").arg(probed.join("; "));
			return false;
		}
		const QString engineScript = resolveEngineScript(err);
		if (engineScript.isEmpty())
			return false;   //err 已带可读文案

		//预检③：glob 清理上次失败/取消残留的 <out>.tdr_tmp*（A-09/C-08 双保险——
		//引擎自身失败路径已清理，此处兜底进程崩溃/kill 场景）
		cleanupTmpFiles(outputPath);

		_outputPath = outputPath;
		_stdoutRemainder.clear();
		_failedMsg.clear();
		_failedErrors.clear();
		_stderrTail.clear();
		_cancelRequested = false;
		_timedOut = false;   //CB-07：超时标志随新一次启动复位

		if (_process == nullptr)
		{
			_process = new QProcess(this);
			connect(_process, &QProcess::readyReadStandardOutput,
				this, &mStrengthReportEngineRunner::onReadyStdout);
			connect(_process, &QProcess::readyReadStandardError,
				this, &mStrengthReportEngineRunner::onReadyStderr);
			connect(_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
				this, &mStrengthReportEngineRunner::onProcessFinished);
		}
		//进程模型（03 §2.1）：工作目录=输出文件所在目录（相对引用最少）；
		//PYTHONIOENCODING=utf-8（中文消息防 3.7 默认编码坑）；-u 双保险（T-14/A-02，
		//引擎已强制 flush，此处按 03"亦可加 -u"兜底管道块缓冲）
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
		_process->setProcessEnvironment(env);
		_process->setWorkingDirectory(QFileInfo(outputPath).absolutePath());
		QStringList args;
		args << QStringLiteral("-u")
			 << engineScript << containerPath << datasetJsonPath << outputPath;
		_process->start(pythonExe, args);
		if (!_process->waitForStarted(5000))
		{
			err = QStringLiteral("导出进程启动失败：%1（python=%2）")
				.arg(_process->errorString(), pythonExe);
			return false;
		}
		//超时看门狗 120s（A-09：防挂死；超时=失败场景矩阵第 5 场景 kill+清理）
		if (_watchdog == nullptr)
		{
			_watchdog = new QTimer(this);
			_watchdog->setSingleShot(true);
			connect(_watchdog, &QTimer::timeout, this, &mStrengthReportEngineRunner::onWatchdogTimeout);
		}
		_watchdog->start(120000);
		return true;
	}

	void mStrengthReportEngineRunner::cancel()
	{
		if (_process == nullptr || _process->state() == QProcess::NotRunning)
			return;
		_cancelRequested = true;
		_process->kill();   //03：取消=kill+清理 tmp（清理在 onProcessFinished 收尾统一做）
	}

	bool mStrengthReportEngineRunner::isRunning() const
	{
		return _process != nullptr && _process->state() != QProcess::NotRunning;
	}

	void mStrengthReportEngineRunner::onReadyStdout()
	{
		if (_process == nullptr)
			return;
		//T-18 GUI 侧：stdout 显式 fromUtf8（PYTHONIOENCODING=utf-8 下引擎按 utf-8 输出）
		_stdoutRemainder += QString::fromUtf8(_process->readAllStandardOutput());
		//JSON-lines 按行切分（半行留缓冲，防 readyRead 分片撕裂 JSON）
		for (;;)
		{
			const int nl = _stdoutRemainder.indexOf(QLatin1Char('\n'));
			if (nl < 0)
				break;
			const QString line = _stdoutRemainder.left(nl).trimmed();
			_stdoutRemainder.remove(0, nl + 1);
			if (line.isEmpty())
				continue;
			const QJsonObject obj = QJsonDocument::fromJson(line.toUtf8()).object();
			if (obj.isEmpty())
				continue;   //非 JSON 行（如 python 启动告警）忽略
			const QString stage = obj.value(QStringLiteral("stage")).toString();
			const int pct = obj.value(QStringLiteral("pct")).toInt();
			const QString msg = obj.value(QStringLiteral("msg")).toString();
			if (stage == QStringLiteral("failed"))
			{
				_failedMsg = msg;
				const QJsonArray errors = obj.value(QStringLiteral("errors")).toArray();
				for (const QJsonValue& e : errors)
					_failedErrors.append(e.toString());
			}
			emit stageProgress(stage, pct, msg);
		}
	}

	void mStrengthReportEngineRunner::onReadyStderr()
	{
		if (_process == nullptr)
			return;
		//stderr 仅作诊断文本（T-16：不作为成败判据）；限长保留尾部防大栈撑内存
		_stderrTail += QString::fromUtf8(_process->readAllStandardError());
		if (_stderrTail.size() > 16384)
			_stderrTail.remove(0, _stderrTail.size() - 8192);
	}

	void mStrengthReportEngineRunner::onWatchdogTimeout()
	{
		if (_process == nullptr || _process->state() == QProcess::NotRunning)
			return;
		//CB-07：置独立超时标志——汇总区分"看门狗超时"（FR-53 场景⑤）与用户主动取消
		_cancelRequested = true;
		_timedOut = true;
		_process->kill();
	}

	void mStrengthReportEngineRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
	{
		if (_watchdog != nullptr)
			_watchdog->stop();
		//成败唯一判据=exitCode（T-16/A-05）：既有渲染器"stderr 非空即失败、退出码不查"判据不可沿用——
		//引擎打印良性告警（[warn] 行）仍判成功；崩溃（CrashExit/无 JSON 行）时错误汇总取 stderr 尾部
		QString summary;
		const bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0 && !_cancelRequested);
		if (ok)
		{
			summary = QStringLiteral("导出完成：%1\n请在 Word 中刷新目录（F9）").arg(_outputPath);
			const QString tail = _stderrTail.trimmed();
			if (!tail.isEmpty())
			{
				//良性告警清单随成功汇总回显（FR-35 缺项告警清单）。
				//CB-09：计数用全量行数，展示仅取尾部 20 行并注明截断——告警 >20 条时
				//旧写法把窗口内行数当总数（"引擎告警 20 条"失真）。
				//三参 .arg 一次性替换，防告警文本内含 %N 被再次展开
				QStringList warnLines = tail.split(QLatin1Char('\n'));
				const int totalWarn = warnLines.size();
				if (totalWarn > 20)
					warnLines = warnLines.mid(totalWarn - 20);
				summary += QStringLiteral("\n引擎告警 %1 条%2：\n%3")
							   .arg(QString::number(totalWarn),
								   totalWarn > 20 ? QStringLiteral("（仅显示尾部 20 条）") : QString(),
								   warnLines.join(QLatin1Char('\n')));
			}
		}
		else if (_timedOut)
		{
			//CB-07：看门狗超时（FR-53 场景⑤）如实汇报，不再误报"导出已取消"
			summary = QStringLiteral("导出超时（超过 120 秒未完成），已终止引擎进程并清理临时文件");
		}
		else if (_cancelRequested)
		{
			summary = QStringLiteral("导出已取消");
		}
		else
		{
			summary = _failedMsg.isEmpty()
				? QStringLiteral("导出失败（退出码 %1）").arg(exitCode)
				: _failedMsg;
			if (!_failedErrors.isEmpty())
				summary += QStringLiteral("\n") + _failedErrors.join(QStringLiteral("\n"));
			//崩溃（无 JSON 行）时错误汇总取 stderr 尾部；exitCode==0 但被取消/异常也走失败收尾
			if (_failedMsg.isEmpty() && !_stderrTail.trimmed().isEmpty())
			{
				const QString tail = _stderrTail.trimmed();
				summary += QStringLiteral("\n") + tail.split(QLatin1Char('\n'))
					.mid(qMax(0, tail.count(QLatin1Char('\n')) + 1 - 20))
					.join(QLatin1Char('\n'));
			}
		}
		//失败不留半成品：引擎失败路径已清理 tmp；kill/崩溃场景 GUI 按 pid 解耦前缀兜底再清一次
		if (!ok)
			cleanupTmpFiles(_outputPath);
		emit finished(ok, summary);
	}
}
