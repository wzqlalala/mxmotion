#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QStringList>

#include "mpostwindow_global.h"

namespace PostWindow
{
	/*
	* UI 截图控制器（界面对照自动化，配套 Auto_Postpage skill §1.2 截图对照流程）
	*
	* 用法（GUI 正常启动后自动执行，非 headless——面板与 Info dock 依赖完整主窗）：
	*   MxMech.exe --ui-shot measure [--ui-shot-case 结果文件] [--ui-shot-dialog 弹窗名]
	*              [--ui-shot-arm] [--ui-shot-collapse] [--ui-shot-dump 控件树.json]
	*              [--ui-shot-out out.png] [--ui-shot-settle 毫秒] [--ui-shot-wait 毫秒]
	*
	* 流程：主窗就绪 →（可选）openPostResultFiles 载入结果并等导入完成信号
	*       → 经 mPostInfoHub 激活目标面板 → panelHostRef()->mainWidget()->grab()
	*       → 保存 PNG → 自动退出（退出码见 UiShotExitCode，stdout 打印 UI_SHOT_OK/ERR）。
	*
	* 截图的是 Info dock 当前面板部件（dock 标题栏/树/视口不入图），与原型元素截图
	* （proto_shot.py 按 CSS 选择器截元素）同一颗粒度，供 ui_compare.py 像素对比。
	*/
	class MPOSTWINDOW_EXPORT mUiShotController
	{
	public:
		/*
		* 命令行含 --ui-shot 时安排截图任务（QTimer 延迟到事件循环启动后执行，
		* 完成后自动 QCoreApplication::exit），返回实际退出码（参数非法时同步退出码，
		* 由 app.exec() 返回）；无 --ui-shot 返回 -1，调用方照常进入主循环
		*/
		static int scheduleIfRequested(const QStringList& arguments);

		/*
		* 面板注册名清单（供参数错误提示；与 cpp 内创建表保持一致）
		*/
		static QStringList panelNames();

		struct Config
		{
			QString panel;          // --ui-shot 面板注册名
			QString caseFile;       // --ui-shot-case 可选：先载入的结果文件（.mxdb 等；"|"分隔多文件＝对话框多选组合）
			QString menuPage;       // --ui-shot-menu 可选：面板激活后切换 Ribbon 页（如"云图"；验证工具栏布局用）
			QStringList dialogs;    // --ui-shot-dialog 可多次：面板弹窗对象名（逐个打开并单独截图）
			QStringList invokes;    // --ui-shot-invoke 可多次：面板槽名（状态驱动，如空态提示）
			QStringList treeClicks; // --ui-shot-treeclick 可多次：后处理树节点显示名（选中并重放单击派发，驱动树交互链；presetN=云图根第N子节点、cloudroot=云图根自身，ASCII 别名防中文实参链路损坏）
			QStringList rePanels;  // --ui-shot-repanel 可多次：树交互后依次重新激活的面板注册名（驱动"面板切走再切回"多段序列，如 等值面生成→点云图→开动画→回等值面）
			QStringList setTexts;   // --ui-shot-settext 可多次：objectName=text，直接注入文本（复现长数值串等文本驱动的布局状态）
		QStringList tlShots;    // --ui-shot-tlshot 可多次：截非宿主顶层窗口（reparent 出的独立弹窗，如信息标签配置窗）；取值 active=当前活动窗，否则按窗口标题精确匹配
			QString dumpPath;       // --ui-shot-dump 可选：导出控件树 JSON（逐控件对齐对比用）
			bool arm = false;       // --ui-shot-arm：截图前进入拾取武装态（按钮勾选+"正在拾取节点…"）
			bool collapse = false;  // --ui-shot-collapse：截图前收起全部折叠组（仅标题条）
			bool wholeWindow = false; // --ui-shot-window：另存主窗口整窗截图（含 3D 视口，面板外渲染验证用）
			QString outPath;        // --ui-shot-out 输出 PNG（默认 ./ui_shot_<面板>.png；弹窗截图自动加 _<弹窗名> 后缀）
			int settleMs = 1500;    // --ui-shot-settle 面板创建后稳定等待（布局/样式收敛）
		int waitMs = 60000;     // --ui-shot-wait 结果导入完成信号等待上限
		int panelWidth = 0;     // --ui-shot-panelwidth 可选：钉住面板宽（复现 Info dock 被拖窄的窄参数栏，做行控件裁切回归）
		int soakMs = 0;         //【临时诊断R14】--ui-shot-soak-ms：收尾前保活事件循环 N 毫秒——
		                        //动画设置弹窗 UI 注入循环档→animRealtimeContent.setPlayForShot(true) 真实链播放→
		                        //心跳保活→停播退出（播放卡顿与内存泄漏专项活体观察窗；收尾拆除）
		};
	};
}
