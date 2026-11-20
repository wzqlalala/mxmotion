#include "mCurveImportCoordinator.h"

#include "mCurveDataSourceStore.h"
#include "mCurveImportProgressDlg.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QMainWindow>
#include <QStatusBar>
#include <QTimer>

namespace PostCurve
{
	mCurveImportCoordinator* mCurveImportCoordinator::getInstance(QWidget* mainWindowContext)
	{
		static mCurveImportCoordinator* s_instance = nullptr;
		if (s_instance == nullptr)
			s_instance = new mCurveImportCoordinator;
		//主窗口上下文随入口刷新（QPointer 防悬挂；入口 window() 顶端即主窗口）
		if (mainWindowContext != nullptr)
			s_instance->_mainWindowContext = mainWindowContext;
		return s_instance;
	}

	mCurveImportCoordinator::mCurveImportCoordinator()
		: QObject(QCoreApplication::instance())   //随应用存活（进程生命周期单例）
	{
		auto* store = mCurveDataSourceStore::getInstance();
		connect(store, &mCurveDataSourceStore::sig_importStarted,
			this, &mCurveImportCoordinator::onStarted);
		connect(store, &mCurveDataSourceStore::sig_importProgress,
			this, &mCurveImportCoordinator::onProgress);
		connect(store, &mCurveDataSourceStore::sig_importFinished,
			this, &mCurveImportCoordinator::onFinished);
		connect(store, &mCurveDataSourceStore::sig_importAborted,
			this, &mCurveImportCoordinator::onAborted);

		_showTimer.setSingleShot(true);
		_showTimer.setInterval(800);
		//800ms 未完成才弹对话框（D1 延迟弹出：小文件全路径只见状态栏消息）
		connect(&_showTimer, &QTimer::timeout, this, [this]() { maybeShowDialog(); });
	}

	void mCurveImportCoordinator::notifyDuplicateInFlight(const QString& filePath)
	{
		//02 §3.4-2：解析中/排队中的重复触发——no-op 状态栏一句话提示，不弹窗
		QMainWindow* mw = mainWindow();
		if (mw && mw->statusBar())
		{
			mw->statusBar()->showMessage(
				QStringLiteral("文件已在导入队列中：%1").arg(QFileInfo(filePath).fileName()), 2000);
		}
	}

	QMainWindow* mCurveImportCoordinator::mainWindow() const
	{
		return qobject_cast<QMainWindow*>(static_cast<QWidget*>(_mainWindowContext));
	}

	void mCurveImportCoordinator::onStarted(const QString& path, qint64 totalBytes)
	{
		//批次判定：上一批次已收尾（onFinished 队列清空 / onAborted 置
		//_active=false）后的首个 started 即新批次开始
		const bool batchStart = !_active;
		_active = true;
		_activeKey = path;
		_totalBytes = totalBytes;
		_doneBytes = 0;
		_accumTotal += totalBytes;

		if (batchStart)
		{
			//BUG-06：新批次令牌递增——旧批次 ≥400ms 收尾定时器闭包据此失效，
			//窗口期内开启的新批次对话框不被旧收尾提前隐藏
			++_batchId;
			//BUG-02：新批次复位对话框已用时计时（跨批次从零起算）；同批次内
			//翻页（_active 已为 true 的 setJob）不复位，保持"已用时=本批次耗时"
			if (_dlg != nullptr)
				_dlg->beginBatch();
		}

		QMainWindow* mw = mainWindow();
		if (mw && mw->statusBar())
		{
			//首反馈（入口受理的同一事件循环内，实际毫秒级；02 FR-01②）
			mw->statusBar()->showMessage(
				QStringLiteral("正在导入 %1 …").arg(QFileInfo(path).fileName()));
		}

		//对话框翻页（可见时）；延迟弹出的单发计时器对首个文件启动
		if (_dialogVisible)
		{
			_dlg->setJob(QFileInfo(path).fileName(), _finishedCount + 1,
				_finishedCount + mCurveDataSourceStore::getInstance()->importQueueDepth());
			_dlg->setProgress(0, totalBytes > 0 ? totalBytes : 0);
		}
		else if (!_showTimer.isActive())
		{
			_showTimer.start();
		}
		_renderClock.start();
	}

	void mCurveImportCoordinator::onProgress(const QString& path, qint64 bytesDone, qint64 bytesTotal)
	{
		if (!_active || path != _activeKey)
			return;
		//解析器已 50ms 节流 + store 单调钳制；此处再按字节真值吸收残余乱序
		if (bytesDone > _doneBytes)
			_doneBytes = bytesDone;
		if (bytesTotal > 0 && _totalBytes <= 0)
			_totalBytes = bytesTotal;
		renderProgress();
	}

	void mCurveImportCoordinator::onFinished(const QString& path, bool ok, const QString& err)
	{
		Q_UNUSED(err);
		if (path != _activeKey)
			return;

		//加权总进度：并入本文件实际完成字节（失败按最后进度并入，不回跳）
		_accumDone += _doneBytes;
		++_finishedCount;

		QMainWindow* mw = mainWindow();
		const bool queueRemains = mCurveDataSourceStore::getInstance()->importQueueDepth() > 0;
		if (queueRemains)
		{
			//对话框随队列存活，翻页由下一个 onStarted 驱动；状态栏消息保持
			_renderClock.start();
			return;
		}

		//队列清空：批次收尾
		_showTimer.stop();
		_active = false;
		_activeKey.clear();
		if (mw && mw->statusBar())
		{
				if (ok)
				{
					//对齐状态栏进度条 3s 隐藏惯例；文案逐字=02 §3.1/§3.3（半角冒号，BUG-03）
					mw->statusBar()->showMessage(
						QStringLiteral("导入完成: %1").arg(QFileInfo(path).fileName()), 3000);
				}
			else
			{
				//错误呈现权在各入口（现有通道弹原文案），状态栏仅清除导入消息
				mw->statusBar()->clearMessage();
			}
		}
		if (_dialogVisible)
		{
			//≥400ms 最短显示（02 §3.2：防闪烁）；尚未显示则不再弹出
			const qint64 shownMs = _visibleSince.isValid() ? _visibleSince.elapsed() : 0;
			const int remainMs = static_cast<int>(qMax<qint64>(0, 400 - shownMs));
			//BUG-06：闭包携带触发时的批次令牌——窗口期内若已开启新批次
			//（_batchId 已递增），旧收尾不得隐藏新批次对话框
			const qint64 batchId = _batchId;
			QTimer::singleShot(remainMs, this, [this, batchId]() {
				if (batchId == _batchId)
					closeDialog();
			});
		}
		resetBatchState();
	}

	void mCurveImportCoordinator::onAborted()
	{
		//clear() 作废（D3）：对话框立即关闭、清除导入消息、批次状态复位
		_showTimer.stop();
		closeDialog();
		QMainWindow* mw = mainWindow();
		if (mw && mw->statusBar())
			mw->statusBar()->clearMessage();
		_active = false;
		_activeKey.clear();
		resetBatchState();
	}

	void mCurveImportCoordinator::maybeShowDialog()
	{
		if (!_active || _dialogVisible)
			return;
		if (_dlg == nullptr)
			_dlg = new mCurveImportProgressDlg(mainWindow());
		_dlg->setJob(QFileInfo(_activeKey).fileName(), _finishedCount + 1,
			_finishedCount + mCurveDataSourceStore::getInstance()->importQueueDepth());
		_dlg->setProgress(_doneBytes, _totalBytes);
		_dlg->show();
		_dlg->raise();
		_dialogVisible = true;
		_visibleSince.start();
		renderProgress();
	}

	void mCurveImportCoordinator::renderProgress()
	{
		if (!_dialogVisible || !_active)
			return;
		//渲染节流 ≤20fps（50ms）；最终进度由完成路径强制收尾
		if (_renderClock.isValid() && _renderClock.elapsed() < 50)
			return;
		_renderClock.start();

		//多文件加权总进度：Σ已完成 + 当前文件进度，分母含已派发文件字节；
		//钳制取 max（跨文件/跨批次单调不回跳，FR-02）
		const qint64 numerator = _accumDone + _doneBytes;
		const qint64 denominator = _accumTotal > 0 ? _accumTotal : (_accumDone + _totalBytes);
		qint64 percent = denominator > 0 ? numerator * 100 / denominator : 0;
		if (percent > 100)
			percent = 100;
		if (percent < _lastShownPercent)
			percent = _lastShownPercent;
		if (_lastShownPercent >= 0 && percent == _lastShownPercent)
			return;   //1% 变化阈值：与上次渲染相同不刷
		_lastShownPercent = percent;

		_dlg->setProgress(numerator, denominator > 0 ? denominator : 0);
	}

	void mCurveImportCoordinator::closeDialog()
	{
		if (_dlg != nullptr && _dialogVisible)
		{
			//hide 而非 close：对话框的关闭事件已被 N8 语义忽略
			_dlg->hide();
			_dialogVisible = false;
		}
	}

	void mCurveImportCoordinator::resetBatchState()
	{
		_finishedCount = 0;
		_accumDone = 0;
		_accumTotal = 0;
		_doneBytes = 0;
		_totalBytes = 0;
		_lastShownPercent = -1;
	}
}
