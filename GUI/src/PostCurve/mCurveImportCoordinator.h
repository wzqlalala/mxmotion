#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - PCH 导入进度协调器（单例，懒创建）
//
// 订阅 mCurveDataSourceStore 的导入生命周期信号（sig_importStarted/Progress/
// Finished + sig_importAborted），编排（02 §3 / 03 §4.3）：
//   - 主窗口状态栏消息区：开始"正在导入 …"（≤500ms 首反馈）/ 完成 3s 自动
//     消失（对齐状态栏进度条 3s 隐藏惯例）/ 失败清除；不复用状态栏 200px
//     进度条（由既有全局信号驱动，混用会互相覆盖）；
//   - 进度对话框：800ms 延迟弹出（单发 QTimer，提前完成取消）；一旦弹出
//     ≥400ms 最短显示防闪烁；渲染节流 ≤20fps + 百分比 ≥1% 变化双保险；
//   - 多文件队列：按各文件字节加权合成总进度，跨文件/跨批次单调不回跳
//     （钳制取 max）；对话框随队列存活（"第 i/N 个"翻页），队列清空才关闭；
//   - clear() 作废（sig_importAborted）：对话框立即关闭、清除导入消息。
//
// 批次口径（BUG-02/06 回归轮）：sig_importStarted 在 _active==false 后首次到
// 达即"新批次开始"——递增 _batchId 并复位对话框已用时计时（beginBatch）；
// 队列清空/作废即批次收尾（resetBatchState，对话框计时复位由下一批次的
// beginBatch 承担）。
//
// 仅订阅 store 信号，store 保持无 GUI 依赖；主窗口经入口上下文解析
// （getInstance(window())——所有曲线弹窗 window() 顶端即主窗口），不链接
// MainWindow 模块（对齐 PostWindow"不再链接 MainWindow"的模块方向）。
// =============================================================================

#include "PostCurveGlobal.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTimer>

class QMainWindow;

namespace PostCurve
{
	class mCurveImportProgressDlg;

	class POSTCURVE_EXPORT mCurveImportCoordinator : public QObject
	{
		Q_OBJECT
	public:
		//懒创建单例。mainWindowContext：调用入口的任一 widget（取其 window()
		//作主窗口上下文，持续刷新；用于状态栏消息与进度对话框父窗口）。
		//入口在 addFileAsync 受理后调用一次即完成订阅
		static mCurveImportCoordinator* getInstance(QWidget* mainWindowContext = nullptr);

		//DuplicateInFlight 幂等提示（02 §3.4-2：状态栏一句话，不弹窗）
		void notifyDuplicateInFlight(const QString& filePath);

	private:
		explicit mCurveImportCoordinator();

	private slots:
		void onStarted(const QString& path, qint64 totalBytes);
		void onProgress(const QString& path, qint64 bytesDone, qint64 bytesTotal);
		void onFinished(const QString& path, bool ok, const QString& err);
		void onAborted();

	private:
		QMainWindow* mainWindow() const;
		void maybeShowDialog();
		void renderProgress();
		void closeDialog();
		void resetBatchState();

		QPointer<QWidget> _mainWindowContext;   //入口上下文（每次 getInstance 刷新）
		mCurveImportProgressDlg* _dlg = nullptr;

		QTimer _showTimer;              //800ms 延迟弹出（单发）
		QElapsedTimer _visibleSince;    //弹出时刻（≥400ms 最短显示）
		QElapsedTimer _renderClock;     //≤20fps 渲染节流
		qint64 _batchId = 0;            //批次令牌（新批次开始递增；旧批次收尾定时器闭包据此失效——BUG-06）
		bool _active = false;           //批次进行中（首个 started 至队列清空/作废）
		QString _activeKey;             //当前在跑文件（忽略其它键的迟到进度）
		qint64 _totalBytes = 0;         //当前文件总字节
		qint64 _doneBytes = 0;          //当前文件已完成字节
		qint64 _accumDone = 0;          //批次内已完成文件的字节合计（加权总进度分子）
		qint64 _accumTotal = 0;         //批次内已派发文件的字节合计（加权总进度分母）
		qint64 _lastShownPercent = -1;  //上次渲染百分比（≥1% 变化 + 不回跳钳制）
		int _finishedCount = 0;         //批次内已完成文件数（"第 i/N 个"）
		bool _dialogVisible = false;
	};
}
