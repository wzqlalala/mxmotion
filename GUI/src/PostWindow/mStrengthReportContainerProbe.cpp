#include "mStrengthReportContainerProbe.h"

#include "mStrengthReportEngineRunner.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>

#include <cstring>

namespace StrengthReport
{
	namespace
	{
		//EOCD/中央目录签名（zip 规范，little-endian）
		const char ZIP_EOCD_SIG[] = { '\x50', '\x4B', '\x05', '\x06' };
		const char ZIP_CENHDR_SIG[] = { '\x50', '\x4B', '\x01', '\x02' };

		quint16 readU16(const uchar* p) { return quint16(p[0]) | (quint16(p[1]) << 8); }
		quint32 readU32(const uchar* p)
		{
			return quint32(p[0]) | (quint32(p[1]) << 8) | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
		}

		//CB-11：校验子进程看门狗超时（与引擎 Runner 同 120s 口径——python 挂死时
		//旧实现 _running 恒 true，重校验/状态机永久阻塞，仅重启恢复）
		const int PROBE_WATCHDOG_MS = 120000;
	}

	mStrengthReportContainerProbe::mStrengthReportContainerProbe(QObject* parent)
		: QObject(parent)
	{
	}

	mStrengthReportContainerProbe::~mStrengthReportContainerProbe()
	{
		//CB-11：析构先停看门狗并断开子进程→本对象全部信号，再 kill 运行中子进程——
		//Qt5 QProcess 析构不终止子进程（孤儿 python）；且 waitForFinished 期间 finished
		//同步派发会回调 Dlg 触碰 UI（与 Runner 析构 CB-06 同型脆弱依赖，先断开消除）
		if (_watchdog != nullptr)
			_watchdog->stop();
		const QList<QProcess*> procs = findChildren<QProcess*>();
		for (QProcess* proc : procs)
		{
			proc->disconnect(this);
			if (proc->state() != QProcess::NotRunning)
			{
				proc->kill();
				proc->waitForFinished(2000);
			}
		}
	}

	void mStrengthReportContainerProbe::startWatchdog()
	{
		if (_watchdog == nullptr)
		{
			_watchdog = new QTimer(this);
			_watchdog->setSingleShot(true);
			connect(_watchdog, &QTimer::timeout, this, [this]()
				{
					_timedOut = true;
					if (_activeProcess != nullptr && _activeProcess->state() != QProcess::NotRunning)
						_activeProcess->kill();   //kill 后由对应 finished 回调收尾汇报
				});
		}
		_watchdog->start(PROBE_WATCHDOG_MS);
	}

	QString mStrengthReportContainerProbe::resolveToolsDir(QString& err)
	{
		err.clear();
		//两级确定性定位（与 Runner 的 python 探测同构，无"回溯几层"模糊语义）：
		//①发布形态 appDir=Application → Application/tdr_tools（T-02-a 布局修订=用户指示
		//2026-11-13：随包落盘位由包根/tools/ 迁至 Application/ 下，与 exe 同级）；
		//②开发树 appDir=bin/Debug → 模块根 resource/scripts/tdr_tools（批次 1 源树落位）
		const QStringList candidates
		{
			QCoreApplication::applicationDirPath() + QStringLiteral("/tdr_tools"),
			QCoreApplication::applicationDirPath() + QStringLiteral("/../../resource/scripts/tdr_tools")
		};
		for (const QString& dir : candidates)
		{
			if (QFile::exists(dir + QStringLiteral("/tdr_validate.py")))
				return QDir::cleanPath(dir);
		}
		err = QStringLiteral("未找到容器检查工具目录（tdr_tools，已探测：%1）").arg(candidates.join("; "));
		return QString();
	}

	bool mStrengthReportContainerProbe::structuralCheck(const QString& path,
		QStringList& entryNames, QString& err)
	{
		entryNames.clear();
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly))
		{
			err = QStringLiteral("规则模板文档无法读取：%1（%2）").arg(path, file.errorString());
			return false;
		}
		const QByteArray tail = file.readAll();
		file.close();
		const uchar* base = reinterpret_cast<const uchar*>(tail.constData());
		const qsizetype size = tail.size();

		//EOCD 定位：从尾部反向扫描签名（zip 注释区最长 65535，窗口取 min(22+65535, size)）
		qsizetype eocd = -1;
		const qsizetype window = qMin<qsizetype>(size, 22 + 65535);
		for (qsizetype i = size - 22; i >= size - window && i >= 0; --i)
		{
			if (std::memcmp(base + i, ZIP_EOCD_SIG, 4) == 0)
			{
				eocd = i;
				break;
			}
		}
		if (eocd < 0)
		{
			err = QStringLiteral("完整性检查失败：文件损坏或非规则模板容器（zip 结构缺失），请重新提炼");
			return false;
		}
		const quint16 entryCount = readU16(base + eocd + 10);
		const quint32 centralOffset = readU32(base + eocd + 16);
		if (centralOffset == 0 || centralOffset >= quint32(size))
		{
			err = QStringLiteral("完整性检查失败：容器 zip 目录损坏，请重新提炼");
			return false;
		}
		//中央目录条目名枚举（条目名区内嵌于容器，无需解压）
		qsizetype pos = qsizetype(centralOffset);
		for (quint16 n = 0; n < entryCount && pos + 46 <= size; ++n)
		{
			if (std::memcmp(base + pos, ZIP_CENHDR_SIG, 4) != 0)
			{
				err = QStringLiteral("完整性检查失败：容器目录条目损坏，请重新提炼");
				return false;
			}
			const quint16 nameLen = readU16(base + pos + 28);
			if (pos + 46 + nameLen > size)
			{
				err = QStringLiteral("完整性检查失败：容器目录越界，请重新提炼");
				return false;
			}
			entryNames.append(QString::fromUtf8(reinterpret_cast<const char*>(base + pos + 46), nameLen));
			//定长头 46+名称+额外区+注释区（额外/注释长度在此不参与后续解析——只按名称链推进需读取它们）
			const quint16 extraLen = readU16(base + pos + 30);
			const quint16 commentLen = readU16(base + pos + 32);
			pos += 46 + nameLen + extraLen + commentLen;
		}
		//关键部件在位核对（02 §2.1.2 容器结构：manifest+规则层）
		if (!entryNames.contains(QStringLiteral("manifest.json"))
			|| !entryNames.contains(QStringLiteral("rules/rules.json")))
		{
			err = QStringLiteral("完整性检查失败：容器缺少 manifest.json 或规则层，请重新提炼");
			return false;
		}
		return true;
	}

	void mStrengthReportContainerProbe::probe(const QString& containerPath,
		const QString& datasetJsonPath, QString& err)
	{
		if (_running)
		{
			err = QStringLiteral("容器检查正在进行中");
			return;
		}
		err.clear();
		_pending = Result();
		_timedOut = false;   //CB-11：超时标志随新探测复位
		_containerPath = containerPath;
		_datasetJsonPath = datasetJsonPath;
		if (containerPath.isEmpty() || !QFile::exists(containerPath))
		{
			_pending.status = NotLoaded;
			_pending.message = QStringLiteral("未加载——请选择规则模板文档(.rtdoc)");
			emitResult(_pending);
			return;
		}
		//①结构层（同步快检）：失败=容器损坏立即出结果，不起子进程
		QStringList entryNames;
		QString structErr;
		if (!structuralCheck(containerPath, entryNames, structErr))
		{
			_pending.status = IntegrityFailed;
			_pending.message = structErr;
			emitResult(_pending);
			return;
		}
		runValidateStep();
	}

	void mStrengthReportContainerProbe::runValidateStep()
	{
		//②内容层：tdr_validate.py（全量校验仍由引擎 M1 同套逻辑负责，此处为状态栏快检）
		QString toolsErr;
		const QString toolsDir = resolveToolsDir(toolsErr);
		if (toolsDir.isEmpty())
		{
			_pending.status = IntegrityFailed;
			_pending.message = toolsErr;
			emitResult(_pending);
			return;
		}
		QStringList searched;
		const QString pythonExe = mStrengthReportEngineRunner::resolvePythonExe(&searched);
		if (pythonExe.isEmpty())
		{
			_pending.status = IntegrityFailed;
			_pending.message = QStringLiteral("未找到可用的 python 解释器（已探测：%1）").arg(searched.join("; "));
			emitResult(_pending);
			return;
		}
		if (_process == nullptr)
		{
			_process = new QProcess(this);
			_process->setProcessChannelMode(QProcess::SeparateChannels);
			connect(_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
				this, [this](int exitCode, QProcess::ExitStatus exitStatus)
				{
					_running = false;
					_activeProcess = nullptr;
					if (_watchdog != nullptr)
						_watchdog->stop();
					const QByteArray out = _process->readAllStandardOutput();
					const QString text = QString::fromUtf8(out);
					//CB-11：看门狗超时如实汇报（kill→CrashExit，与异常退出区分文案）
					if (_timedOut)
					{
						_timedOut = false;
						_pending.status = IntegrityFailed;
						_pending.message = QStringLiteral("容器检查超时（超过 120 秒未完成），已终止检查进程");
						emitResult(_pending);
						return;
					}
					if (exitStatus != QProcess::NormalExit)
					{
						_pending.status = IntegrityFailed;
						_pending.message = QStringLiteral("容器检查进程异常退出");
						emitResult(_pending);
						return;
					}
					if (exitCode == 0)
					{
						_pending.status = Loaded;
						//"[validate] 模板=<template_id> | 部件=N | 块=N | 槽=N" 取模板名做状态行回显
						const QStringList lines = text.split(QLatin1Char('\n'));
						for (const QString& line : lines)
						{
							const int idx = line.indexOf(QStringLiteral("[validate] 模板="));
							if (idx >= 0)
							{
								const QString seg = line.mid(idx + QString::fromUtf8("[validate] 模板=").size());
								const int end = seg.indexOf(QLatin1Char('|'));
								_pending.templateName = (end >= 0 ? seg.left(end) : seg).trimmed();
								break;
							}
						}
						_pending.message = QStringLiteral("已加载·检查通过");
						runRulesStep();
						return;
					}
					//exitCode!=0："[validate] 校验失败：<msg>"+"  - <条目>" bullets（FR-13 条目级定位）
					QString failMsg;
					QStringList bullets;
					const QStringList lines = text.split(QLatin1Char('\n'));
					for (const QString& line : lines)
					{
						const int idx = line.indexOf(QStringLiteral("[validate] 校验失败："));
						if (idx >= 0)
							failMsg = line.mid(idx + QString::fromUtf8("[validate] 校验失败：").size()).trimmed();
						const int bullet = line.indexOf(QStringLiteral("  - "));
						if (bullet >= 0)
							bullets.append(line.mid(bullet + 4).trimmed());
					}
					if (failMsg.isEmpty())
						failMsg = QStringLiteral("完整性检查失败，请重新提炼");
					if (failMsg.contains(QStringLiteral("版本不符")))
					{
						//NFR-6：format_version 大版本不符（FR-12④"版本不符"提示口径）
						_pending.status = VersionMismatch;
						_pending.message = QStringLiteral("格式版本不符，请升级程序或按新格式重新提炼");
					}
					else if (!bullets.isEmpty())
					{
						_pending.status = ValidationFailed;
						_pending.message = QStringLiteral("检查失败 %1 条").arg(bullets.size());
						_pending.errors = bullets;
					}
					else
					{
						_pending.status = IntegrityFailed;
						_pending.message = failMsg;
					}
					emitResult(_pending);
				});
		}
		_running = true;
		QStringList args{ toolsDir + QStringLiteral("/tdr_validate.py"), _containerPath };
		if (!_datasetJsonPath.isEmpty())
			args.append(_datasetJsonPath);   //S5-2 instance_key 数组存在性探测需要数据集
		_process->start(pythonExe, args);
		if (!_process->waitForStarted(5000))
		{
			_running = false;
			_activeProcess = nullptr;
			_pending.status = IntegrityFailed;
			_pending.message = QStringLiteral("容器检查进程启动失败：%1").arg(_process->errorString());
			emitResult(_pending);
			return;
		}
		//CB-11：启动成功后挂 120s 看门狗（挂死 python 不再永久阻塞状态机）
		_activeProcess = _process;
		startWatchdog();
	}

	void mStrengthReportContainerProbe::runRulesStep()
	{
		//③槽位清单层：tdr_rules.py get（批次 1 工具的规则层取出子命令）→临时 JSON→QJsonDocument。
		//失败降级为"已加载但槽位清单为空"（校验已过，仅 UI 槽位清单缺数据，不阻断导出）
		QString toolsErr;
		const QString toolsDir = resolveToolsDir(toolsErr);
		const QString pythonExe = mStrengthReportEngineRunner::resolvePythonExe();
		const QString outJson = QDir::temp().absoluteFilePath(
			QStringLiteral("mx_strength_report_tmp/rules_extracted.json"));
		QDir().mkpath(QFileInfo(outJson).absolutePath());
		QFile::remove(outJson);
		if (toolsDir.isEmpty() || pythonExe.isEmpty())
		{
			_pending.message = QStringLiteral("已加载·检查通过（规则层取出失败，槽位清单为空）");
			emitResult(_pending);
			return;
		}
		_running = true;
		const QString containerPath = _containerPath;
		QProcess* getProc = new QProcess(this);
		getProc->setProcessChannelMode(QProcess::SeparateChannels);
		connect(getProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			this, [this, getProc, outJson, containerPath](int exitCode, QProcess::ExitStatus exitStatus)
			{
				_running = false;
				_activeProcess = nullptr;
				if (_watchdog != nullptr)
					_watchdog->stop();
				getProc->deleteLater();
				if (_containerPath != containerPath)
					return;   //探测期间用户已换容器：丢弃过期结果（_pending 已被新探测重置）
				//CB-11：规则层取出超时降级文案（校验已过，仅槽位清单缺数据，不阻断导出）
				if (_timedOut)
				{
					_timedOut = false;
					_pending.message = QStringLiteral("已加载·检查通过（规则层取出超时，槽位清单为空）");
					emitResult(_pending);
					return;
				}
				if (exitCode != 0 || exitStatus != QProcess::NormalExit)
				{
					_pending.message = QStringLiteral("已加载·检查通过（规则层取出失败，槽位清单为空）");
					emitResult(_pending);
					return;
				}
				QFile file(outJson);
				if (file.open(QIODevice::ReadOnly))
				{
					//CB-16：tdr_rules get 输出为 rules 原文（可含整行 '#' 注释头——
					//tdr_rules.py:37-46 _load_rules_text 原样落盘；引擎 tdrrules.load_rules
					//同口径剥整行注释后消费）。QJsonDocument 不认注释，先剥整行注释再解析，
					//草稿容器（digest 草稿态 rules）也能正常出槽位清单（JSON 字符串值内 '#' 
					//不可能出现在行首——字符串内换行恒为 \n 转义，整行判据安全）
					const QStringList rawLines =
						QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
					QStringList jsonLines;
					jsonLines.reserve(rawLines.size());
					for (const QString& line : rawLines)
					{
						if (!line.trimmed().startsWith(QLatin1Char('#')))
							jsonLines.append(line);
					}
					file.close();
					_pending.rulesDocument =
						QJsonDocument::fromJson(jsonLines.join(QLatin1Char('\n')).toUtf8());
				}
				emitResult(_pending);
			});
		getProc->start(pythonExe,
			QStringList{ QStringLiteral("-u"), toolsDir + QStringLiteral("/tdr_rules.py"),
				QStringLiteral("get"), _containerPath, outJson });
		if (!getProc->waitForStarted(5000))
		{
			_running = false;
			_activeProcess = nullptr;
			getProc->deleteLater();
			_pending.message = QStringLiteral("已加载·检查通过（规则层取出失败，槽位清单为空）");
			emitResult(_pending);
			return;
		}
		//CB-11：第二段（tdr_rules get）同样挂 120s 看门狗
		_activeProcess = getProc;
		startWatchdog();
	}

	void mStrengthReportContainerProbe::emitResult(const Result& result)
	{
		emit finished(result);
	}

	bool mStrengthReportContainerProbe::isRunning() const
	{
		return _running;
	}
}
