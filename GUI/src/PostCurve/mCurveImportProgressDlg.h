#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - PCH 导入进度对话框（纯展示，无业务依赖）
//
// 02 §3.2 规格（D7）：
//   - 非模态、父=主窗口，首次弹出居中于主窗口，可拖动、不跟随鼠标，固定
//     380×140 不可拉伸、不记忆位置；
//   - 无按钮区，标题栏无关闭钮、无"?"，ESC（reject）与 closeEvent 一律忽略
//     ——N8"导入不可取消"在 UI 全部关闭路径上的封堵；
//   - 内容：文件名（长名中段省略 + tooltip 全名）、进度条 + 百分比（条内文本）、
//     队列位次"第 i/N 个"（单文件隐藏）、已用时（1s 粒度）+ 预计剩余
//     （进度 <5% 时显示"计算中…"，加"约"前缀防精确承诺）。
//
// 刷新由 mCurveImportCoordinator 驱动（setJob/setProgress）；已用时由本对话框
// 内部 1s 定时器自驱动，计时生命周期=批次生命周期（BUG-02 回归轮口径）：
// 协调器在新批次开始时调 beginBatch() 复位（跨批次从零起算），同批次内翻页
// 连续；隐藏时停 1s 定时器消除空转，再显示由 showEvent 兜底重启。
// =============================================================================

#include "PostCurveGlobal.h"

#include <QDialog>
#include <QElapsedTimer>

class QLabel;
class QProgressBar;
class QTimer;

namespace PostCurve
{
	class POSTCURVE_EXPORT mCurveImportProgressDlg : public QDialog
	{
		Q_OBJECT
	public:
		explicit mCurveImportProgressDlg(QWidget* parent = nullptr);

		//翻到下一个文件（队列场景）：fileName 为显示名，index/total 为 1 基位次
		//（total<=1 时隐藏位次行）
		void setJob(const QString& fileName, int index, int total);
		//进度（字节口径）：total<=0 按不定进度处理（忙碌条 + 百分比"…"）
		void setProgress(qint64 bytesDone, qint64 bytesTotal);
		//新批次计时复位（BUG-02：跨批次"已用时"从零起算）。由协调器在新批次
		//首个 sig_importStarted 时调用；同批次翻页（setJob）不调用，批次内
		//已用时保持连续
		void beginBatch();

	protected:
		//N8：不可取消——关闭路径全部忽略
		void closeEvent(QCloseEvent* event) override;
		void reject() override;
		//计时兜底（未复位过则以首次显示为起点）+ 确保 1s 自驱动运行
		void showEvent(QShowEvent* event) override;
		//隐藏即停 1s 自驱动（消除空转；计时复位由下一批次 beginBatch 负责）
		void hideEvent(QHideEvent* event) override;

	private:
		void updateTimeText();

		QLabel* _fileLabel = nullptr;     //文件名（中段省略 + tooltip 全名）
		QProgressBar* _bar = nullptr;     //进度条（百分比条内居中——BUG-04）
		QLabel* _queueLabel = nullptr;    //"第 i/N 个"
		QLabel* _timeLabel = nullptr;     //已用时 + 预计剩余

		QString _fileName;                //全名（tooltip）
		qint64 _doneBytes = 0;
		qint64 _totalBytes = 0;
		QElapsedTimer _elapsed;           //批次已用时（1s 粒度显示）
		QTimer* _tickTimer = nullptr;     //1s 自驱动刷新已用时/预计剩余
	};
}
