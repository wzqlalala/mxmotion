#include "mCurveDataSourceStore.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFileInfo>
#include <QThread>

#include <cmath>
#include <filesystem>
#include <exception>
#include <utility>

//跨线程队列信号携带解析结果（sig_parseFinished → onParseFinished）需要元类型。
//H1 性能修复：载体为 shared_ptr——排队连接跨线程只拷贝智能指针本身（原子
//计数增减），原按值 PchFile 载体会在 emit 侧与派发侧各深拷贝一次整个解析结果。
//ascii 版（R22）同款：sig_asciiParseFinished → onAsciiParseFinished
Q_DECLARE_METATYPE(std::shared_ptr<PchReader::PchFile>)
Q_DECLARE_METATYPE(std::shared_ptr<PostCurve::AsciiColumnFile>)

namespace PostCurve
{
	// =============================================================================
	// 后台纯解析 worker（仅本文件可见；只持有入参与出参，不触 store 状态与任何
	// GUI 对象）。worker QObject + moveToThread 为项目主导范式（mIOFileManager
	// 的 mxdb 导入线程 / mBatchMeshWorker 同款）；worker 事件循环天然串行化，
	// 与 store 侧闸门共同保证同一时刻至多一个文件在解析
	// =============================================================================
	class mCurveDataSourceStore::ImportWorker : public QObject
	{
		Q_OBJECT
	public:
		//cancelFlag 为 store 持有的退出快速中止标志（R3）——非拥有指针，生命
		//周期覆盖线程全程（store 单例永不析构）；构造期传入，跨线程只读
		explicit ImportWorker(const std::atomic<bool>* cancelFlag, QObject* parent = nullptr)
			: QObject(parent)
			, _cancelFlag(cancelFlag)
		{
		}

	public slots:
		//解析一个文件：关原文口径与同步 addFile 完全一致（曲线导入只读不写回）；
		//进度经 ReaderOptions::progressCallback 发队列信号（解析器内已 50ms 节流
		//+ 区间边界强制上报，≤20 次/秒·线程，无事件洪泛）；err 文案与同步路径
		//同源（异常 what()/「未知异常」/「文件无有效数据块」）
		void parse(const QString& key, const QString& displayPath)
		{
			Q_UNUSED(displayPath);
			_currentKey = key;
			PchReader::ReaderOptions options;
			options.keepOriginalLines = false;
			options.keepBlockRawLines = false;
			options.cancelFlag = _cancelFlag;
			options.progressCallback = [this](std::size_t bytesDone, std::size_t bytesTotal) {
				emit sig_parseProgress(_currentKey,
					static_cast<qint64>(bytesDone), static_cast<qint64>(bytesTotal));
			};

			PchReader::PchFile file;
			bool ok = true;
			QString err;
			try
			{
				file = PchReader::PchReader(options).readFile(std::filesystem::u8path(key.toStdString()));
			}
			catch (const std::exception& e)
			{
				ok = false;
				err = QString::fromUtf8(e.what());
			}
			catch (...)
			{
				ok = false;
				err = QStringLiteral("未知异常");
			}
			if (ok && file.blocks.empty())
			{
				ok = false;
				err = QStringLiteral("文件无有效数据块");
			}
			//H1：解析结果仅此一处构造 shared_ptr，信号跨队列只传递指针——
			//emit 侧/派发侧均不再深拷贝 PchFile（原按值载体实测主线程
			//Release ~143-180ms、Debug ~0.9-1.2s 双深拷贝）
			emit sig_parseFinished(key, ok, err,
				std::make_shared<PchReader::PchFile>(std::move(file)));
		}

		//解析一个 csv/txt 列数据文件（R22）：mCurveAsciiReader 纯解析，err 文案
		//与同步 addFile ascii 分支同源（readFile 直出）。无逐步进度机制（小文件
		//毫秒级瞬时完成），进度信号照常上报——完成前单次 100%，保持
		//sig_importStarted/Progress/Finished 生命周期对调用方透明
		void parseAscii(const QString& key, const QString& displayPath)
		{
			Q_UNUSED(displayPath);
			_currentKey = key;
			AsciiColumnFile file;
			QString err;
			const bool ok = mCurveAsciiReader::readFile(key, &file, &err);
			qint64 totalBytes = 0;
			const QFileInfo info(key);
			if (info.exists())
				totalBytes = info.size();
			emit sig_parseProgress(key, totalBytes, totalBytes);
			emit sig_asciiParseFinished(key, ok, err,
				std::make_shared<AsciiColumnFile>(std::move(file)));
		}

	signals:
		//解析进度（解析线程触发，经队列连接回 store 主线程再以
		//sig_importProgress 对外转发）
		void sig_parseProgress(const QString& key, qint64 bytesDone, qint64 bytesTotal);
		//解析完成（结果所有权随信号移交主线程提交；H1：shared_ptr 载体，
		//排队连接零深拷贝，主线程槽内 move 进 _files）
		void sig_parseFinished(const QString& key, bool ok, const QString& err,
			std::shared_ptr<PchReader::PchFile> file);
		//ascii 解析完成（R22；同构，主线程槽内 move 进 _asciiFiles）
		void sig_asciiParseFinished(const QString& key, bool ok, const QString& err,
			std::shared_ptr<AsciiColumnFile> file);

	private:
		const std::atomic<bool>* _cancelFlag = nullptr;   //非拥有（store 成员）
		QString _currentKey;                              //在跑文件键（进度回调用）
	};

	mCurveDataSourceStore* mCurveDataSourceStore::_instance = nullptr;

	mCurveDataSourceStore* mCurveDataSourceStore::getInstance()
	{
		if (_instance == nullptr)
			_instance = new mCurveDataSourceStore;
		return _instance;
	}

	mCurveDataSourceStore::mCurveDataSourceStore(QObject* parent)
		: QObject(parent)
	{
		//队列信号携带解析结果需注册元类型（queued connection 实参拷贝用；
		//H1：载体为 shared_ptr，注册智能指针本身）。R22：ascii 结果同款
		qRegisterMetaType<std::shared_ptr<PchReader::PchFile>>();
		qRegisterMetaType<std::shared_ptr<AsciiColumnFile>>();

		//后台导入线程：启动一次、随进程存活（store 为永不析构单例，退出经
		//aboutToQuit 收尾，无静态析构顺序风险）
		_thread = new QThread(this);
		_thread->setObjectName(QStringLiteral("CurvePchImportThread"));
		_worker = new ImportWorker(&_cancelCurrent);   //无父：随线程 finished deleteLater
		_worker->moveToThread(_thread);
		//进度：解析器 50ms 节流后回主线程，经钳制槽转发（观测乱序防护）
		connect(_worker, &ImportWorker::sig_parseProgress,
			this, &mCurveDataSourceStore::onParseProgress, Qt::QueuedConnection);
		//完成：主线程一次性提交（01 NFR-3：解析结果仅允许主线程提交）。
		//pch/ascii 两条完成信号分槽提交（pch 路径保持逐位不动）
		connect(_worker, &ImportWorker::sig_parseFinished,
			this, &mCurveDataSourceStore::onParseFinished, Qt::QueuedConnection);
		connect(_worker, &ImportWorker::sig_asciiParseFinished,
			this, &mCurveDataSourceStore::onAsciiParseFinished, Qt::QueuedConnection);
		connect(_thread, &QThread::finished, _worker, &QObject::deleteLater);
		_thread->start();

		//退出收尾（R3/D6）：置中止标志——解析器区间级检查，毫秒~区间级返回，
		//在跑结果丢弃；排空队列并停线程。等待期间不再接受用户输入由退出流程
		//自身保证（aboutToQuit 后主事件循环已退出）
		QCoreApplication* app = QCoreApplication::instance();
		if (app)
		{
			connect(app, &QCoreApplication::aboutToQuit, this, [this]() {
				_pending.clear();
				_cancelCurrent.store(true);
				if (_thread)
				{
					_thread->quit();
					_thread->wait();
				}
			});
		}
	}

	mCurveDataSourceStore::~mCurveDataSourceStore()
	{
		if (_instance == this)
			_instance = nullptr;
	}

	QString mCurveDataSourceStore::normalizePath(const QString& filePath)
	{
		if (filePath.isEmpty())
			return filePath;
		//路径规范化缓存（optimize-channel-map-perf）：canonicalFilePath 是 stat
		//系统调用，通道映射路径下每次候选查询/文本反解都会触发。GUI 线程序列化
		//调用前提沿袭单例现状；clear() 一并清（语义重置点）。文件被移走后缓存
		//仍返回旧 canonical——此时查询本就不命中登记键，无行为影响
		mCurveDataSourceStore* store = getInstance();
		const auto it = store->_canonCache.constFind(filePath);
		if (it != store->_canonCache.constEnd())
			return it.value();
		const QFileInfo info(filePath);
		//canonicalFilePath 仅对存在的文件非空（解析符号链接/规范大小写）；
		//文件已被移走时退回 absoluteFilePath（统一分隔符/去 "./"，大小写保留）
		const QString canonical = info.canonicalFilePath();
		const QString out = canonical.isEmpty() ? info.absoluteFilePath() : canonical;
		store->_canonCache.insert(filePath, out);
		return out;
	}

	mCurveDataSourceStore::DataSourceKind mCurveDataSourceStore::fileKind(const QString& filePath)
	{
		//扩展名判定（不分大小写）：.csv/.txt -> AsciiColumn，其余 -> Pch。
		//后缀取末个 '.' 后段（"a.b.csv" -> "csv"）；无后缀按 Pch（与既有唯一
		//入口兼容）。仅凭扩展名判定，不读文件内容——登记前的轻量分发
		const QString ext = QFileInfo(filePath).suffix();
		return (ext.compare(QLatin1String("csv"), Qt::CaseInsensitive) == 0
			|| ext.compare(QLatin1String("txt"), Qt::CaseInsensitive) == 0)
			? DataSourceKind::AsciiColumn
			: DataSourceKind::Pch;
	}

	bool mCurveDataSourceStore::addFile(const QString& filePath, QString* err)
	{
		//闸门等待（FR-03，罕见重叠）：同步导入（会话恢复/批量管线）与后台异步
		//导入重叠时，主线程嵌套事件循环等待在跑队列排空；ExcludeUserInputEvents
		//保证等待期间不处理用户输入（防再入导入入口/弹窗形成嵌套循环叠加，
		//先例 mMenuWidget.cpp:5316），换取 store 单线程写不变式与解析内存峰值
		//不叠加；该重叠场景下最长冻结一个队列，现状接受（02 §FR-03）
		if (_running)
		{
			QEventLoop gate;
			connect(this, &mCurveDataSourceStore::sig_parseIdle, &gate, &QEventLoop::quit);
			while (_running)
			{
				gate.exec(QEventLoop::ExcludeUserInputEvents);
			}
		}

		const QString key = normalizePath(filePath);
		//幂等：同一文件（pch 或 ascii，键同规范化）不重复解析
		if (_files.contains(key) || _asciiFiles.contains(key))
			return true;

		//ascii 分支（R22）：mCurveAsciiReader 纯解析，err 文案与 pch"无有效数据块"
		//同风格（"文件无有效数据列/行"），成功判定（>=1 列且 >=1 行）在解析器内
		if (fileKind(filePath) == DataSourceKind::AsciiColumn)
		{
			AsciiColumnFile ascii;
			QString asciiErr;
			if (!mCurveAsciiReader::readFile(key, &ascii, &asciiErr))
			{
				if (err) *err = asciiErr;
				return false;
			}
			_asciiFiles.insert(key, std::move(ascii));
			buildIndex(key);
			_order.append(key);
			emit sig_fileAdded(key);
			return true;
		}

		//pch 分支（原路径零改动）
		PchReader::PchFile file;
		try
		{
			//曲线导入只读不写回：关闭原始文本保留（大文件省 2-3 份全文件文本
			//拷贝与百万级小字符串分配），解析内部按 $TITLE 块边界并行
			PchReader::ReaderOptions readerOptions;
			readerOptions.keepOriginalLines = false;
			readerOptions.keepBlockRawLines = false;
			file = PchReader::PchReader(readerOptions).readFile(std::filesystem::u8path(key.toStdString()));
		}
		catch (const std::exception& e)
		{
			if (err) *err = QString::fromUtf8(e.what());
			return false;
		}
		catch (...)
		{
			if (err) *err = QStringLiteral("未知异常");
			return false;
		}

		if (file.blocks.empty())
		{
			if (err) *err = QStringLiteral("文件无有效数据块");
			return false;
		}

		_files.insert(key, std::move(file));
		buildIndex(key);
		_order.append(key);
		emit sig_fileAdded(key);
		return true;
	}

	bool mCurveDataSourceStore::contains(const QString& filePath) const
	{
		//pch 与 ascii 数据集一并覆盖（R22）：登记与否的判定不分文件类型
		const QString key = normalizePath(filePath);
		return _files.contains(key) || _asciiFiles.contains(key);
	}

	mCurveDataSourceStore::AddFileAsyncResult mCurveDataSourceStore::addFileAsync(const QString& filePath)
	{
		const QString key = normalizePath(filePath);
		//幂等现状语义：不发任何导入信号（pch/ascii 两仓一并查重）
		if (_files.contains(key) || _asciiFiles.contains(key))
			return AddFileAsyncResult::AlreadyRegistered;
		if (_running && _runningKey == key)
			return AddFileAsyncResult::DuplicateInFlight;   //解析中重复触发（D2）
		for (const ImportJob& job : _pending)
		{
			if (job.key == key)
				return AddFileAsyncResult::DuplicateInFlight;   //排队中重复触发（D2）
		}

		ImportJob job;
		job.key = key;
		job.displayPath = filePath;
		job.kind = fileKind(filePath);   //解析分发（worker 槽二选一，队列共享）
		_pending.append(job);
		startNextJobIfIdle();
		return AddFileAsyncResult::Accepted;
	}

	int mCurveDataSourceStore::importQueueDepth() const
	{
		return _pending.size() + (_running ? 1 : 0);
	}

	void mCurveDataSourceStore::onParseProgress(const QString& key, qint64 bytesDone, qint64 bytesTotal)
	{
		//仅当前在跑作业且会话未作废：clear() 作废后迟到的进度直接丢弃（进度
		//UI 已由 sig_importAborted 复位）
		if (!_running || key != _runningKey || _runningGeneration != _sessionGeneration)
			return;
		//多 chunk 并发下各线程对共享原子计数的观测顺序可能轻微乱序（解析器内
		//为 relaxed 读）——按已上报最大值钳制，保证对外单调不减（FR-02）
		if (bytesDone < _lastProgressDone)
			return;
		_lastProgressDone = bytesDone;
		emit sig_importProgress(key, bytesDone, bytesTotal);
	}

	void mCurveDataSourceStore::startNextJobIfIdle()
	{
		if (_running || _pending.isEmpty())
			return;

		const ImportJob job = _pending.takeFirst();
		_running = true;
		_runningKey = job.key;
		_runningGeneration = _sessionGeneration;
		_lastProgressDone = -1;
		_cancelCurrent.store(false);   //复位退出中止标志（上一作业可能置位过）

		//总字节（进度加权用）：键为规范化绝对路径，文件存在时必可取；不可得
		//时发 0（协调器按不定进度处理）
		qint64 totalBytes = 0;
		const QFileInfo info(job.key);
		if (info.exists())
			totalBytes = info.size();
		emit sig_importStarted(job.key, totalBytes);

		//跨线程派发（队列调用；worker 事件循环串行化与 _running 闸门双保险）。
		//pch/ascii 按作业类型走各自 worker 槽（解析器二选一，队列/闸门共享）
		if (job.kind == DataSourceKind::AsciiColumn)
		{
			QMetaObject::invokeMethod(_worker, "parseAscii", Qt::QueuedConnection,
				Q_ARG(QString, job.key), Q_ARG(QString, job.displayPath));
		}
		else
		{
			QMetaObject::invokeMethod(_worker, "parse", Qt::QueuedConnection,
				Q_ARG(QString, job.key), Q_ARG(QString, job.displayPath));
		}
	}

	void mCurveDataSourceStore::onParseFinished(const QString& key, bool ok, const QString& err,
		std::shared_ptr<PchReader::PchFile> file)
	{
		//代次校验（D3 导入作废）：clear() 已使会话代次前移——在跑结果丢弃，
		//不登记不发 sig_fileAdded/sig_importFinished（进度 UI 已由
		//sig_importAborted 收尾），仅放行闸门
		const bool generationMatches = (_runningGeneration == _sessionGeneration);
		_running = false;
		_runningKey.clear();
		_lastProgressDone = -1;
		if (generationMatches)
		{
			if (ok)
			{
				//H1：move 解引用进 _files（零深拷贝）——worker 侧 shared_ptr 仍
				//指 moved-from 空壳，随队列事件实参析构；file 必非空（唯一发射
				//点 worker::parse 恒 make_shared 构造）
				_files.insert(key, std::move(*file));
				buildIndex(key);
				_order.append(key);
				emit sig_fileAdded(key);
			}
			emit sig_importFinished(key, ok, err);
		}
		//放行下一个排队作业（FIFO）；闸门放行信号最后发（同步 addFile 的嵌套
		//等待按放行后的队列状态决定是否继续等）
		startNextJobIfIdle();
		emit sig_parseIdle();
	}

	void mCurveDataSourceStore::onAsciiParseFinished(const QString& key, bool ok, const QString& err,
		std::shared_ptr<AsciiColumnFile> file)
	{
		//ascii 解析完成提交（R22）：与 onParseFinished 同构（代次校验/提交/
		//闸门放行逐行对齐），仅登记目标为 _asciiFiles——pch 完成路径保持
		//逐位不动，故独立成槽而非抽共用实现。file 必非空（worker::parseAscii
		//恒 make_shared 构造），move 解引用进 _asciiFiles 零深拷贝
		const bool generationMatches = (_runningGeneration == _sessionGeneration);
		_running = false;
		_runningKey.clear();
		_lastProgressDone = -1;
		if (generationMatches)
		{
			if (ok)
			{
				_asciiFiles.insert(key, std::move(*file));
				buildIndex(key);
				_order.append(key);
				emit sig_fileAdded(key);
			}
			emit sig_importFinished(key, ok, err);
		}
		startNextJobIfIdle();
		emit sig_parseIdle();
	}

	QStringList mCurveDataSourceStore::filePaths() const
	{
		return _order;   //登记顺序（QHash 键序未指定）
	}

	void mCurveDataSourceStore::clear()
	{
		//整体清空缓存（会话恢复/打开新会话前的重置用）。不发 sig_fileAdded：
		//数据集界面（打开曲线/定义曲线弹窗）在每次弹出时按仓库现状重建，无需
		//订阅失效。
		//异步导入作废（D3）：清空队列 + 会话代次前移——在跑结果提交前校验
		//代次不匹配即丢弃（不登记不发 sig_fileAdded/sig_importFinished）；有
		//活动时发 sig_importAborted 供进度对话框/状态栏立即收尾（02 §3.3）
		const bool hadActivity = _running || !_pending.isEmpty();
		_pending.clear();
		++_sessionGeneration;
		_files.clear();
		_asciiFiles.clear();
		_indexes.clear();
		_blockIndexes.clear();
		_canonCache.clear();
		_order.clear();
		if (hadActivity)
			emit sig_importAborted();
	}

	const PchReader::PchFile* mCurveDataSourceStore::file(const QString& filePath) const
	{
		const auto it = _files.constFind(normalizePath(filePath));
		return it == _files.constEnd() ? nullptr : &it.value();
	}

	const ListWgtStruct* mCurveDataSourceStore::index(const QString& filePath) const
	{
		const auto it = _indexes.constFind(normalizePath(filePath));
		return it == _indexes.constEnd() ? nullptr : &it.value();
	}

	bool mCurveDataSourceStore::isVectorSchema(const QString& schema)
	{
		//与 mOpenCurveDlg 分量列表联动一致：仅节点矢量结果展开分量
		return schema == QStringLiteral("VELOCITY")
			|| schema == QStringLiteral("DISPLACEMENTS")
			|| schema == QStringLiteral("ACCELERATION")
			|| schema == QStringLiteral("FORCE");
	}

	bool mCurveDataSourceStore::isAbscissaSchema(const QString& schema)
	{
		//buildIndex 生成进 xresulttype 的两种横坐标类型（文本恒定，无需查索引）
		return schema == QStringLiteral("Time[Sec]")
			|| schema == QStringLiteral("Frequency[Hz]");
	}

	bool mCurveDataSourceStore::isChannelSchema(const QString& schema)
	{
		//实际结果类型（通道分量值形态）：非空且非横坐标类型；空=未记录（随
		//锚通道走），不算通道形态
		return !schema.isEmpty() && !isAbscissaSchema(schema);
	}

	QString mCurveDataSourceStore::subcaseDisplayText(const PchReader::PchFile& file, int subcaseId)
	{
		//工况显示文本："Subcase {id} (label)"，label 取该工况首个数据块的
		//$LABEL 行内容（自 mOpenCurveDlg::setListWig 迁移为共享，"定义曲线"
		//工况列同源调用，保证两处工况列表内容一致）
		QString text = QStringLiteral("Subcase %1").arg(subcaseId);
		for (const auto& blk : file.blocks)
			if (blk.subcaseId == subcaseId && !blk.label.empty())
				return text + QStringLiteral(" (%1)").arg(QString::fromStdString(blk.label));
		return text;
	}

	QString mCurveDataSourceStore::subcaseDisplayText(const QString& filePath, int subcaseId) const
	{
		//回显统一入口（R22）：按文件类型分发。ascii 无工况级语义——回显
		//空串（UI 隐藏工况列，标签模板 {y.MXSubcase} 解析为空，符合"占位符
		//来源缺失替换为空"既有规则）；pch 走静态版语义；未登记返回空串
		if (fileKind(filePath) == DataSourceKind::AsciiColumn)
			return QString();
		const PchReader::PchFile* f = file(filePath);
		return f ? subcaseDisplayText(*f, subcaseId) : QString();
	}

	QStringList mCurveDataSourceStore::expandedComponents(const QString& numericFormat, bool isTime)
	{
		//分量展开规则自 mOpenCurveDlg 分量列表联动逻辑迁移（频响 12 个 / 瞬态 6 个）。
		//列表为进程级常量（optimize-channel-map-perf）：返回即 COW 共享，调用方
		//高频枚举（匹配全枚举/候选现算）不再每次重建
		static const QStringList kMagPhase = {
			QStringLiteral("MAG|X-Trans"), QStringLiteral("MAG|Y-Trans"), QStringLiteral("MAG|Z-Trans"),
			QStringLiteral("MAG|X-Rot"), QStringLiteral("MAG|Y-Rot"), QStringLiteral("MAG|Z-Rot"),
			QStringLiteral("PHA|X-Trans"), QStringLiteral("PHA|Y-Trans"), QStringLiteral("PHA|Z-Trans"),
			QStringLiteral("PHA|X-Rot"), QStringLiteral("PHA|Y-Rot"), QStringLiteral("PHA|Z-Rot"),
		};
		static const QStringList kRealImag = {
			QStringLiteral("REAL|X-Trans"), QStringLiteral("REAL|Y-Trans"), QStringLiteral("REAL|Z-Trans"),
			QStringLiteral("REAL|X-Rot"), QStringLiteral("REAL|Y-Rot"), QStringLiteral("REAL|Z-Rot"),
			QStringLiteral("IMAG|X-Trans"), QStringLiteral("IMAG|Y-Trans"), QStringLiteral("IMAG|Z-Trans"),
			QStringLiteral("IMAG|X-Rot"), QStringLiteral("IMAG|Y-Rot"), QStringLiteral("IMAG|Z-Rot"),
		};
		static const QStringList kRealFreq = {
			QStringLiteral("REAL|X-Trans"), QStringLiteral("REAL|Y-Trans"), QStringLiteral("REAL|Z-Trans"),
			QStringLiteral("REAL|X-Rot"), QStringLiteral("REAL|Y-Rot"), QStringLiteral("REAL|Z-Rot"),
			QStringLiteral("IMAG|X-Trans"), QStringLiteral("IMAG|Y-Trans"), QStringLiteral("IMAG|Z-Trans"),
			QStringLiteral("IMAG|X-Rot"), QStringLiteral("IMAG|Y-Rot"), QStringLiteral("IMAG|Z-Rot"),
		};
		static const QStringList kRealTime = {
			QStringLiteral("REAL|X-Trans"), QStringLiteral("REAL|Y-Trans"), QStringLiteral("REAL|Z-Trans"),
			QStringLiteral("REAL|X-Rot"), QStringLiteral("REAL|Y-Rot"), QStringLiteral("REAL|Z-Rot"),
		};
		if (numericFormat.contains("MAGNITUDE-PHASE", Qt::CaseInsensitive))
			return kMagPhase;   //频响：幅值 + 相位 -> 12 个分量
		if (numericFormat.contains("REAL-IMAGINARY", Qt::CaseInsensitive))
			return kRealImag;   //频响：实部 + 虚部 -> 12 个分量
		if (numericFormat.contains("REAL-ONLY", Qt::CaseInsensitive))
		{
			if (isTime)
				return kRealTime;   //瞬态：只有 6 个实数分量
			return kRealFreq;   //频响中的 REAL-ONLY 按实部/虚部处理（12 个分量）
		}
		return {};
	}

	int mCurveDataSourceStore::componentIndex(const QString& component)
	{
		//索引 0-5  -> primary[0..5]  (MAG / REAL / 瞬态值)
		//索引 6-11 -> secondary[0..5] (PHA / IMAG)
		//返回 -1 表示未识别

		//前半部分：MAG / REAL
		if (component == "MAG|X-Trans" || component == "REAL|X-Trans") return 0;
		if (component == "MAG|Y-Trans" || component == "REAL|Y-Trans") return 1;
		if (component == "MAG|Z-Trans" || component == "REAL|Z-Trans") return 2;
		if (component == "MAG|X-Rot" || component == "REAL|X-Rot")   return 3;
		if (component == "MAG|Y-Rot" || component == "REAL|Y-Rot")   return 4;
		if (component == "MAG|Z-Rot" || component == "REAL|Z-Rot")   return 5;

		//声压结果（NTF，设计稿令牌）：REL|FX = REAL 声压 × FX 激励方向。
		//声压为标量结果，块记录 primary[0] 即声压值——映射到下标 0，
		//assembleChannelData 即可走通用路径装配
		if (component == "REL|FX" || component == "REAL|FX" || component == "MAG|FX") return 0;

		//后半部分：PHA / IMAG
		if (component == "PHA|X-Trans" || component == "IMAG|X-Trans") return 6;
		if (component == "PHA|Y-Trans" || component == "IMAG|Y-Trans") return 7;
		if (component == "PHA|Z-Trans" || component == "IMAG|Z-Trans") return 8;
		if (component == "PHA|X-Rot" || component == "IMAG|X-Rot")   return 9;
		if (component == "PHA|Y-Rot" || component == "IMAG|Y-Rot")   return 10;
		if (component == "PHA|Z-Rot" || component == "IMAG|Z-Rot")   return 11;

		return -1;
	}

	QString mCurveDataSourceStore::siblingComplexComponent(const QString& component)
	{
		//复数配对分量：同方向令牌互换（MAG↔PHA、REAL↔IMAG）；非复数分量返回空
		const int bar = component.indexOf('|');
		if (bar <= 0)
			return QString();
		const QString tok = component.left(bar);
		const QString dir = component.mid(bar + 1);
		static const struct { const char* a; const char* b; } kPairs[] = {
			{ "MAG", "PHA" }, { "PHA", "MAG" }, { "REAL", "IMAG" }, { "IMAG", "REAL" },
		};
		for (const auto& p : kPairs)
		{
			if (tok.compare(QLatin1String(p.a), Qt::CaseInsensitive) == 0)
				return QStringLiteral("%1|%2").arg(QLatin1String(p.b), dir);
		}
		return QString();
	}

	//--- csv/txt 通用列数据（R22）语义常量 ---
	//ascii 数据集为固定骨架：单"工况"（id=1，UI 不显示）× 单"结果类型"
	//（"Unknown"）× 单"对象"（显示 "Block 1"，id=1），分量=全部列名。四常量
	//即该骨架的对外词汇表（下游 UI 与装配五元组共用，勿散落字面量）
	QString mCurveDataSourceStore::asciiSchemaDisplayText()
	{
		//Types 列唯一项（用户拍板 20261109：不要表头派生项、不要 Index）。
		//"Unknown" 按实际结果类型对待（非横坐标）：isChannelSchema 认
		//（可装配通道）、isVectorSchema 不认（不走矢量分量展开，分量列表
		//由 asciiColumnNames 下发）
		return QStringLiteral("Unknown");
	}

	int mCurveDataSourceStore::asciiSubcaseId()
	{
		//工况内部标识（装配五元组/层级索引键；显示级无工况列）
		return 1;
	}

	int mCurveDataSourceStore::asciiPointId()
	{
		//对象内部标识（装配五元组/层级索引键，对应显示文本 "Block 1"）
		return 1;
	}

	QString mCurveDataSourceStore::asciiObjectDisplayText()
	{
		//Requests 列唯一项
		return QStringLiteral("Block 1");
	}

	QStringList mCurveDataSourceStore::asciiColumnNames(const QString& filePath) const
	{
		//分量列名（显示序，Components 列表内容）。未登记/非 ascii 返回空——
		//调用方据空串判型（与 fileKind 双保险）。同名列不去重，装配按首个
		//匹配下标取列
		if (fileKind(filePath) != DataSourceKind::AsciiColumn)
			return QStringList();
		const auto it = _asciiFiles.constFind(normalizePath(filePath));
		return it == _asciiFiles.constEnd() ? QStringList() : it->columnNames;
	}

	namespace
	{
	//装配核心（assembleChannelData 共用体，optimize-channel-map-perf 抽取）：
	//blocks 为已按 (subcaseId, schema, pointId) 匹配好的块序列（块序即拼接序），
	//防线与取值规则与原实现逐行一致
	bool assembleBlocks(const QVector<const PchReader::PchResultBlock*>& blocks,
		const QString& component, mCurveDataSourceStore::ChannelDataResult* out,
		QString* err)
	{
		if (!out)
		{
			if (err) *err = QStringLiteral("输出参数为空");
			return false;
		}
		out->xData.clear();
		out->yData.clear();
		out->skippedNoSecondary = false;

		const int idx = mCurveDataSourceStore::componentIndex(component);
		if (idx < 0)
		{
			if (err) *err = QStringLiteral("未识别的分量名称：%1").arg(component);
			return false;
		}

		for (const PchReader::PchResultBlock* blk : blocks)
		{
			//防线：PHA/IMAG 分量（idx>=6）需要复数 12 值数据（secondary）；块内
			//全无 secondary 说明是瞬态实数块（6 值），该分量没有真实数据——
			//整块跳过并记标志，由调用方决定如何告警
			if (idx >= 6)
			{
				bool hasSecondary = false;
				for (const auto& rec : blk->records)
				{
					if (rec.secondary.has_value())
					{
						hasSecondary = true;
						break;
					}
				}
				if (!hasSecondary)
				{
					out->skippedNoSecondary = true;
					continue;
				}
			}
			for (const auto& record : blk->records)
			{
				out->xData.push_back(record.abscissa);
				if (idx <= 5)
					out->yData.push_back(record.primary[idx]);       //前半：MAG/REAL/瞬态值
				else if (record.secondary.has_value())
					out->yData.push_back(record.secondary.value()[idx - 6]);   //后半：PHA/IMAG
				else
					out->yData.push_back(0.0);   //瞬态无 secondary 防御，保持 X/Y 点数一致
			}
		}

		if (out->xData.isEmpty() || out->yData.isEmpty())
		{
			if (err) *err = QStringLiteral("通道无有效数据点");
			return false;
		}
		return true;
	}

	//ascii 分支装配核心（R22，assembleChannelData/hasChannelData 共用）：
	//component 按列名首个匹配取列下标（同名列取首个），yData=列值，
	//xData=行号 1..N（默认横坐标；UI 侧 X 通道另经同一条路径以列名装配）。
	//skippedNoSecondary 恒 false（ascii 无 secondary 防线语义）。
	//ascii 解析成功判定保证 >=1 列且 >=1 行，空列仅在越界防御时出现
	bool asciiColumnExists(const AsciiColumnFile& ascii, const QString& component)
	{
		return ascii.columnNames.indexOf(component) >= 0;   //首个匹配下标语义
	}

	bool assembleAsciiColumn(const AsciiColumnFile& ascii, const QString& component,
		mCurveDataSourceStore::ChannelDataResult* out, QString* err)
	{
		if (!out)
		{
			if (err) *err = QStringLiteral("输出参数为空");
			return false;
		}
		out->xData.clear();
		out->yData.clear();
		out->skippedNoSecondary = false;

		const int col = ascii.columnNames.indexOf(component);
		if (col < 0)
		{
			if (err) *err = QStringLiteral("列名不存在：%1").arg(component);
			return false;
		}
		const QVector<double>& values = ascii.columns.at(col);
		out->xData.reserve(values.size());
		out->yData.reserve(values.size());
		for (int i = 0; i < values.size(); ++i)
		{
			out->xData.append(i + 1);          //默认横坐标 = 行号（1 基）
			out->yData.append(values.at(i));   //NaN 照搬（非有限值由消费链路跳过）
		}
		if (out->xData.isEmpty() || out->yData.isEmpty())
		{
			if (err) *err = QStringLiteral("通道无有效数据点");
			return false;
		}
		return true;
	}
	}   //匿名命名空间（文件局部装配核心）

	bool mCurveDataSourceStore::assembleChannelData(const PchReader::PchFile& file,
	int subcaseId, const QString& schema, int pointId, const QString& component,
	ChannelDataResult* out, QString* err)
{
	//线性扫收集匹配块（块序 = 遍历序，与原实现逐位一致）后走共用装配核心
	QVector<const PchReader::PchResultBlock*> blocks;
	for (const auto& blk : file.blocks)
	{
		if (blk.subcaseId == subcaseId
			&& QString::fromStdString(blk.schemaName) == schema
			&& blk.pointId == pointId)
		{
			blocks.append(&blk);
		}
	}
	return assembleBlocks(blocks, component, out, err);
}

bool mCurveDataSourceStore::assembleChannelData(const QString& filePath,
	int subcaseId, const QString& schema, int pointId, const QString& component,
	ChannelDataResult* out, QString* err)
{
	//ascii 分支（R22）：component=列名，yData=列值、xData=行号 1..N。subcaseId/
	//schema/pointId 不参与匹配（ascii 骨架恒为常量 1/"Unknown"/1，调用方传
	//asciiSubcaseId()/asciiSchemaDisplayText()/asciiPointId()）；仅按登记键与
	//列名定位，未登记按既有 err 风格失败
	if (fileKind(filePath) == DataSourceKind::AsciiColumn)
	{
		const auto aIt = _asciiFiles.constFind(normalizePath(filePath));
		if (aIt == _asciiFiles.constEnd())
		{
			if (err) *err = QStringLiteral("数据文件不在仓库");
			return false;
		}
		Q_UNUSED(subcaseId);
		Q_UNUSED(schema);
		Q_UNUSED(pointId);
		return assembleAsciiColumn(*aIt, component, out, err);
	}

	//索引化重载（optimize-channel-map-perf）：按登记键直取匹配块下标，
	//不再全块线性扫；块序 = buildIndex 遍历序 = 静态版命中序（pch 原路径零改动）
	const QString key = normalizePath(filePath);
	const auto fileIt = _files.constFind(key);
	const auto idxIt = _blockIndexes.constFind(key);
	if (fileIt == _files.constEnd() || idxIt == _blockIndexes.constEnd())
	{
		if (err) *err = QStringLiteral("数据文件不在仓库");
		return false;
	}
	QVector<const PchReader::PchResultBlock*> blocks;
	const auto bIt = idxIt->constFind({ subcaseId, schema, pointId });
	if (bIt != idxIt->constEnd())
	{
		for (int bi : bIt.value())
			blocks.append(&fileIt->blocks[bi]);
	}
	return assembleBlocks(blocks, component, out, err);
}

bool mCurveDataSourceStore::hasChannelData(const QString& filePath,
	int subcaseId, const QString& schema, int pointId, const QString& component,
	QString* err) const
{
	//ascii 分支（R22）：列名在已登记 ascii 数据集中即存在（与装配成败判定
	//互充；列存在则数据必非空——解析成功判定保证 >=1 行）
	if (fileKind(filePath) == DataSourceKind::AsciiColumn)
	{
		const auto aIt = _asciiFiles.constFind(normalizePath(filePath));
		if (aIt == _asciiFiles.constEnd())
		{
			if (err) *err = QStringLiteral("数据文件不在仓库");
			return false;
		}
		Q_UNUSED(subcaseId);
		Q_UNUSED(schema);
		Q_UNUSED(pointId);
		if (!asciiColumnExists(*aIt, component))
		{
			if (err) *err = QStringLiteral("列名不存在：%1").arg(component);
			return false;
		}
		return true;
	}

	//与静态 assembleChannelData 成败判定严格互充（optimize-channel-map-perf）：
	//componentIndex 有效 ∧ ∃匹配块（records 非空 ∧ (idx<6 ∨ 块内存在 secondary)）
	//（pch 原路径零改动）
	if (componentIndex(component) < 0)
	{
		if (err) *err = QStringLiteral("未识别的分量名称：%1").arg(component);
		return false;
	}
	const QString key = normalizePath(filePath);
	const auto fileIt = _files.constFind(key);
	const auto idxIt = _blockIndexes.constFind(key);
	if (fileIt == _files.constEnd() || idxIt == _blockIndexes.constEnd())
	{
		if (err) *err = QStringLiteral("数据文件不在仓库");
		return false;
	}
	const auto bIt = idxIt->constFind({ subcaseId, schema, pointId });
	if (bIt != idxIt->constEnd())
	{
		const int idx = componentIndex(component);
		for (int bi : bIt.value())
		{
			const PchReader::PchResultBlock& blk = fileIt->blocks[bi];
			if (blk.records.empty())
				continue;   //无记录块产不出数据点，装配亦不成
			if (idx >= 6)
			{
				bool hasSecondary = false;
				for (const auto& rec : blk.records)
				{
					if (rec.secondary.has_value())
					{
						hasSecondary = true;
						break;
					}
				}
				if (!hasSecondary)
					continue;   //瞬态实数块：PHA/IMAG 无真实数据，装配整块跳过
			}
			return true;
		}
	}
	if (err) *err = QStringLiteral("通道无有效数据点");
	return false;
}

	bool mCurveDataSourceStore::assembleChannelMagnitude(const PchReader::PchFile& file,
		int subcaseId, const QString& schema, int pointId, const QString& direction,
		ChannelDataResult* out, QString* err)
	{
		if (!out)
		{
			if (err) *err = QStringLiteral("输出参数为空");
			return false;
		}
		out->xData.clear();
		out->yData.clear();
		out->skippedNoSecondary = false;

		//首个匹配块的数值格式决定装配方式（同键多块拼接场景各块格式一致）
		const PchReader::PchResultBlock* blk = nullptr;
		for (const auto& b : file.blocks)
		{
			if (b.subcaseId == subcaseId
				&& QString::fromStdString(b.schemaName) == schema
				&& b.pointId == pointId)
			{
				blk = &b;
				break;
			}
		}
		if (!blk)
		{
			if (err) *err = QStringLiteral("无匹配数据块");
			return false;
		}

		auto assembleOne = [&](const QString& component, ChannelDataResult* o, QString* e) -> bool
		{
			o->xData.clear();
			o->yData.clear();
			o->skippedNoSecondary = false;
			return assembleChannelData(file, subcaseId, schema, pointId, component, o, e);
		};

		//声压块（NTF，OptiStruct "$PRESSURE + $REAL OUTPUT" 复数标量）：typed 记录
		//6 值行的 primary 前 2 值为实部/虚部，幅值 = √(re²+im²)；12 值行实/虚部按
		//typed 约定分居 primary/secondary 首位。REL|FX 等令牌的 FX 是激励方向，
		//声压本身标量，方向不参与取值；同键多块按顺序拼接，与 assembleChannelData 一致
		if (blk->resultType == PchReader::ResultType::Pressure)
		{
			out->xData.clear();
			out->yData.clear();
			for (const auto& b : file.blocks)
			{
				if (b.subcaseId != subcaseId
					|| QString::fromStdString(b.schemaName) != schema
					|| b.pointId != pointId
					|| b.resultType != PchReader::ResultType::Pressure)
				{
					continue;
				}
				for (const auto& rec : b.records)
				{
					const double re = rec.primary[0];
					const double im = rec.secondary.has_value() ? (*rec.secondary)[0] : rec.primary[1];
					out->xData.append(rec.abscissa);
					out->yData.append(std::sqrt(re * re + im * im));
				}
			}
			if (out->xData.isEmpty() || out->yData.isEmpty())
			{
				if (err) *err = QStringLiteral("通道无有效数据点");
				return false;
			}
			return true;
		}

		switch (blk->numericFormat)
		{
		case PchReader::NumericFormat::MagnitudePhase:
			//幅值相位格式：MAG 分量即幅值
			return assembleOne(QStringLiteral("MAG|") + direction, out, err);
		case PchReader::NumericFormat::RealImaginary:
		{
			//实部虚部格式：REAL/IMAG 两路合成幅值（缺虚部数据时退化为实部原值，
			//并保留防线标志交由调用方告警）
			ChannelDataResult re, im;
			QString reErr, imErr;
			if (!assembleOne(QStringLiteral("REAL|") + direction, &re, &reErr))
			{
				if (err) *err = reErr;
				return false;
			}
			const bool okIm = assembleOne(QStringLiteral("IMAG|") + direction, &im, &imErr);
			const bool haveIm = okIm && im.yData.size() == re.yData.size();
			out->xData = re.xData;
			out->yData.resize(re.yData.size());
			for (int i = 0; i < re.yData.size(); ++i)
			{
				const double r = re.yData.at(i);
				const double m = haveIm ? im.yData.at(i) : 0.0;
				out->yData[i] = std::sqrt(r * r + m * m);
			}
			out->skippedNoSecondary = re.skippedNoSecondary || (okIm && im.skippedNoSecondary);
			return !out->yData.isEmpty();
		}
		case PchReader::NumericFormat::RealOnly:
		default:
			//REAL-ONLY（瞬态/标量声压等）取 REAL 分量原值；未识别格式先试 REAL
			//再回退 MAG
			if (assembleOne(QStringLiteral("REAL|") + direction, out, nullptr))
				return true;
			return assembleOne(QStringLiteral("MAG|") + direction, out, err);
		}
	}

	void mCurveDataSourceStore::buildIndex(const QString& filePath)
	{
		ListWgtStruct& idx = _indexes[filePath];
		idx = ListWgtStruct();   //防御重复构建时残留旧索引

		//ascii 数据集（R22）固定骨架索引：xresulttype 空集（无 Time/Frequency
		//横坐标类型语义）；单工况 1 / 单结果类型 "Unknown" / 单对象 1；分量叶子
		//NumericFmtInfo{"ASCII", false} 仅作索引占位——UI 分量列表对 ascii 走
		//asciiColumnNames，不经 expandedComponents 展开（"ASCII" 亦非
		//PchReader::toString 词汇，expandedComponents 恒返回空，双保险）。
		//不进 _blockIndexes（块索引为 pch blocks 专属，ascii 装配直取列）
		if (_asciiFiles.contains(filePath))
		{
			const int sub = asciiSubcaseId();
			const QString schema = asciiSchemaDisplayText();
			const int point = asciiPointId();
			idx.operatingcondition.insert(sub);
			idx.yresulttype[sub].insert(schema);
			idx.object[sub][schema].insert(point);
			NumericFmtInfo info;
			info.fmt = QStringLiteral("ASCII");
			info.transient = false;
			idx.ycomponent[sub][schema][point].insert(info);
			return;
		}

		const PchReader::PchFile& f = _files[filePath];

		//块索引同遍历填充（optimize-channel-map-perf）：块下标指向 f.blocks 的
		//vector 位置；追加序 = 遍历序 = 线性扫命中序（装配拼接序不变式）
		QHash<ChannelBlockKey, QVector<int>>& fileBlocks = _blockIndexes[filePath];
		fileBlocks.clear();

		int blockIdx = 0;
		for (const auto& blk : f.blocks)
		{
			const QString schema = QString::fromStdString(blk.schemaName);
			const QString fmt = QString::fromStdString(PchReader::toString(blk.numericFormat));

			idx.operatingcondition.insert(blk.subcaseId);

			idx.yresulttype[blk.subcaseId].insert(QString::fromStdString(PchReader::toString(blk.resultType)));

			idx.object[blk.subcaseId][schema].insert(blk.pointId);

			fileBlocks[{ blk.subcaseId, schema, blk.pointId }].append(blockIdx++);

			//瞬态判定（块级，数据形态优先）：频响复数每点 12 值必有 secondary，
			//瞬态实数每点 6 值 secondary 必空；label 显式声明时间时直接认定。
			//旧实现仅认 "IDENTIFIED BY TIME" 标签文本且为文件级粒度，OptiStruct 等
			//不带该标签的瞬态 deck（"$REAL OUTPUT"）会被误判为频响
			bool hasSecondary = false;
			for (const auto& rec : blk.records)
			{
				if (rec.secondary.has_value())
				{
					hasSecondary = true;
					break;
				}
			}
			bool labelTime = false;
			for (const auto& label : blk.labels)
			{
				if (!label.hasValue)
					continue;

				if (QString::fromStdString(label.value).contains("IDENTIFIED BY TIME", Qt::CaseInsensitive))
				{
					labelTime = true;
					break;
				}
			}
			//无结构化记录的块（纯 generic/标量结果）无法从数据形态判定，按非瞬态
			//处理——X 轴维持旧频响判定语义，避免无关结果类型外溢出 Time[Sec]
			const bool isTransient = labelTime || (!blk.records.empty() && !hasSecondary);

			NumericFmtInfo info;
			info.fmt = fmt;
			info.transient = isTransient;
			idx.ycomponent[blk.subcaseId][schema][blk.pointId].insert(info);

			//X 轴类型：瞬态 -> 时间轴；非瞬态按数值格式认频响（复数格式）
			if (isTransient)
			{
				idx.xresulttype.insert(QStringLiteral("Time[Sec]"));
			}
			else if (fmt.contains("MAGNITUDE-PHASE", Qt::CaseInsensitive)
				|| fmt.contains("REAL-ONLY", Qt::CaseInsensitive)
				|| fmt.contains("REAL-IMAGINARY", Qt::CaseInsensitive))
			{
				idx.xresulttype.insert(QStringLiteral("Frequency[Hz]"));
			}
		}
	}
}

//本文件内定义的 QObject 子类（ImportWorker）的 moc 产物（AUTOMOC 约定）
#include "mCurveDataSourceStore.moc"
