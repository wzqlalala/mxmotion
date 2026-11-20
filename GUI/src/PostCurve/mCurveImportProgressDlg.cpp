#include "mCurveImportProgressDlg.h"

#include "UiPalette.h"   //BUG-05：进度条配色取主题语义色（与 mStatusBar 同源）

#include <QCloseEvent>
#include <QFontMetrics>
#include <QHideEvent>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>

namespace PostCurve
{
	namespace
	{
		//秒 -> "mm:ss"（02 §3.2：已用时 1s 粒度；剩余为估算值）
		QString formatDuration(qint64 seconds)
		{
			if (seconds < 0)
				seconds = 0;
			return QStringLiteral("%1:%2")
				.arg(seconds / 60, 2, 10, QLatin1Char('0'))
				.arg(seconds % 60, 2, 10, QLatin1Char('0'));
		}
	}

	mCurveImportProgressDlg::mCurveImportProgressDlg(QWidget* parent)
		: QDialog(parent)
	{
		//非模态；去掉标题栏关闭钮与"?"（N8：无任何用户取消入口）；保留标准
		//标题栏以便拖动（02 §3.2：用户可拖开看曲线区）
		setModal(false);
		setWindowFlags(windowFlags()
			& ~Qt::WindowCloseButtonHint
			& ~Qt::WindowContextHelpButtonHint);
		setFixedSize(380, 140);
		setWindowTitle(QStringLiteral("正在导入 PCH 数据"));

		_fileLabel = new QLabel(this);
		_fileLabel->setToolTipDuration(0);

		//BUG-04：百分比文本条内居中（02 §3.2 内容②"进度条+百分比文本（条内
		//居中）"），删除原右侧独立百分比列；条高 14->18 给条内文本留行高
		_bar = new QProgressBar(this);
		_bar->setRange(0, 100);
		_bar->setValue(0);
		_bar->setTextVisible(true);
		_bar->setAlignment(Qt::AlignCenter);
		_bar->setFormat(QStringLiteral("%p%"));
		_bar->setFixedHeight(18);

		//BUG-05：配色对齐 mStatusBar 进度条（mStatusBar.cpp applyTheme 的
		//QProgressBar QSS：panel 次表面底 + primary 主强调 chunk，UiPalette 随
		//主题动态取值）。依赖核验：PostCurve 已链接 MxFunctions，且 UiPalette.h
		//与其既有 include 的 mGlobalSignals.h 同目录（RebuildFunctions/src，头
		//路径编译可达），零新增链接/包含依赖，故采 UiPalette 方案而非字面值
		//复制（字面值会在深色/护眼主题下脱色，且无主题切换同步时机）。仅
		//height:8px 不随抄——本条含条内文本，高度由上方 setFixedHeight 定
		_bar->setStyleSheet(QStringLiteral(
			"QProgressBar { background:%1; border:none; text-align:center; }"
			"QProgressBar::chunk { background-color:%2; }"
		).arg(MxFunctions::UiPalette::panel().name(), MxFunctions::UiPalette::primary().name()));

		_queueLabel = new QLabel(this);
		_queueLabel->hide();

		_timeLabel = new QLabel(this);

		auto* layout = new QVBoxLayout(this);
		layout->addSpacing(10);
		layout->addWidget(_fileLabel);
		layout->addWidget(_bar);
		layout->addWidget(_queueLabel);
		layout->addWidget(_timeLabel);
		layout->addStretch(1);

		//已用时 1s 粒度自驱动
		_tickTimer = new QTimer(this);
		_tickTimer->setInterval(1000);
		connect(_tickTimer, &QTimer::timeout, this, [this]() { updateTimeText(); });
	}

	void mCurveImportProgressDlg::setJob(const QString& fileName, int index, int total)
	{
		_fileName = fileName;
		//长名中段省略（380 宽减去边距/百分比列），tooltip 保全名
		const QFontMetrics metrics(_fileLabel->fontMetrics());
		_fileLabel->setText(metrics.elidedText(fileName, Qt::ElideMiddle, 300));
		_fileLabel->setToolTip(fileName);

		if (total > 1)
		{
			_queueLabel->setText(QStringLiteral("第 %1/%2 个").arg(index).arg(total));
			_queueLabel->show();
		}
		else
		{
			_queueLabel->hide();
		}

		//翻页后进度从本文件重新起算（总进度不回跳由协调器钳制保证）；条内
		//文本格式复位（上一文件可能停留在不定进度"…"）。已用时不在此复位：
		//翻页属同批次，计时连续（跨批次复位由 beginBatch 负责——BUG-02）
		_doneBytes = 0;
		_totalBytes = 0;
		_bar->setRange(0, 100);
		_bar->setValue(0);
		_bar->setFormat(QStringLiteral("%p%"));
		updateTimeText();
	}

	void mCurveImportProgressDlg::setProgress(qint64 bytesDone, qint64 bytesTotal)
	{
		_doneBytes = bytesDone;
		_totalBytes = bytesTotal;
		if (bytesTotal > 0)
		{
			_bar->setRange(0, 100);
			const qint64 percent = bytesDone * 100 / bytesTotal;
			_bar->setValue(static_cast<int>(percent));
			_bar->setFormat(QStringLiteral("%p%"));   //不定转确定后恢复百分比
		}
		else
		{
			//流尺寸不可得：忙碌条 + 条内"…"（02 §3.2 不定进度分支）
			_bar->setRange(0, 0);
			_bar->setFormat(QStringLiteral("…"));
		}
		updateTimeText();
	}

	void mCurveImportProgressDlg::beginBatch()
	{
		//新批次计时复位（BUG-02：跨批次"已用时"从零起算）；同批次翻页不经
		//此处。隐藏态（常态：新批次通常在对话框关闭后开启）只复位计时、不启
		//定时器，显示时由 showEvent 兜底；可见态（BUG-06 窗口期翻新批次）须
		//确保 1s 自驱动在跑
		_elapsed.restart();
		if (isVisible() && !_tickTimer->isActive())
			_tickTimer->start();
	}

	void mCurveImportProgressDlg::closeEvent(QCloseEvent* event)
	{
		//N8：导入不可取消——忽略一切关闭请求（队列清空/作废由协调器 hide）
		event->ignore();
	}

	void mCurveImportProgressDlg::reject()
	{
		//N8：ESC 一律忽略
	}

	void mCurveImportProgressDlg::showEvent(QShowEvent* event)
	{
		//计时兜底：首个批次协调器 beginBatch 时 _dlg 尚未创建（懒创建于
		//maybeShowDialog），以首次显示为批次起点；1s 自驱动确保运行
		//（BUG-02：隐藏时已停，再显示必须重启）
		if (!_elapsed.isValid())
			_elapsed.start();
		if (!_tickTimer->isActive())
			_tickTimer->start();
		QDialog::showEvent(event);
	}

	void mCurveImportProgressDlg::hideEvent(QHideEvent* event)
	{
		//BUG-02：隐藏即停 1s 自驱动（消除空转）；计时起点不动，跨批次复位
		//由下一批次 beginBatch 负责
		_tickTimer->stop();
		QDialog::hideEvent(event);
	}

	void mCurveImportProgressDlg::updateTimeText()
	{
		const qint64 elapsedSec = _elapsed.isValid() ? _elapsed.elapsed() / 1000 : 0;
		//预计剩余 = 已用时 / 百分比 × 余量；进度 <5% 或不定进度时显示"计算中…"
		const qint64 percent = _totalBytes > 0 ? _doneBytes * 100 / _totalBytes : 0;
		QString text;
		if (percent >= 5 && _doneBytes > 0)
		{
			const qint64 remainSec = elapsedSec * (100 - percent) / percent;
			text = QStringLiteral("已用时 %1 · 预计剩余 约 %2")
				.arg(formatDuration(elapsedSec), formatDuration(remainSec));
		}
		else
		{
			text = QStringLiteral("已用时 %1 · 预计剩余 计算中…")
				.arg(formatDuration(elapsedSec));
		}
		_timeLabel->setText(text);
	}
}
