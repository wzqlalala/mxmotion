#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

#include <functional>

class QEventLoop;
class QWidget;

namespace StrengthReport
{
	class mStrengthReportProjectConfig;

	//——三期批次 C（FR-331/332/333/334，13-技术路线-三期 §3 批次 C/§4.4 批量截图链调用序）——
	//全自动批量截图链控制器：一键导出流程中（数据集装配之前、autoCapture 开关开启时）
	//按 slotMappings 映射序列逐槽执行四步链：
	//  ①映射应用：四元组→guidata 公共字段写+cloudResultPanel()->reloadCloudFromGUIData(viewName)
	//    （既有 public 回放入口，mPost3DResult.h:183-190；生产先例 mPostDeformation.cpp:710-721
	//    同构——渲染库/他人并行编辑文件零修改，只读调用）；
	//    SolidColor 伪变量槽/无变量四元组=仅帧维应用（MechCommon changedFrame 既有切帧链，
	//    与用户手动操作完全同链——二期试点 grabWithAlign 同源）
	//  ②回显校验（T3-2/A-2 防自证修订）：不依赖 guidata 收口字段（reloadCloudFromGUIData 五条
	//    早退守卫 mPost3DResult.cpp:1953-1977 先于收口写回，"收口==四元组"必过=自证）——
	//    判据=面板链侧独立证据：frameChanged(frameId, UserSeek)（slot_showCloud 实际进入的
	//    重绘证明，链尾 seekFrame emit=true 绕过同帧去重 mPost3DResult.cpp:4858）+postDB 单槽
	//    结果缓存旁证 getResultVarNameByFrameID==复合格式"变量名 分量名"（渲染链分发写面，
	//    独立于调用方 guidata 写入；复合格式=AutoAssembler::compositeResultName 镜像 T3-3①；
	//    mPost3DResult.cpp:4713 读数缓存门同款比对先例）。早退/失配→直接分类 E3-8"映射应用
	//    失配"跳槽留空（不落 5s 超时误报——失败原因分类正确性=FR-334 验收项）
	//  ③就绪等待+抓取（二期件全套复用）：connect frameChanged 先于切换指令（C-1 硬约束②，
	//    同线程 direct connection 在切换调用栈内同步发射，先切后连必漏收）+frameId==目标帧 且
	//    source==UserSeek 双过滤（C-1 硬约束①）+update/processEvents+同步 grab()+200ms 防抖
	//    （D3-4 定值）+单槽 5s 预算（NFR-3.1）+中止通道（视口关闭/对话框关闭→abortBatch）
	//    →mStrengthReportCollector::saveViewportScreenshot（LogoFlipGuard RAII 十步序零改动
	//    复用，按页签④水印开关）
	//  ④错图检测：落盘 PNG 回读非空+单色检测（O3-5 双判据：64×64 降采样亮度极差≤ε1 且
	//    标准差≤ε2→可疑；可疑=标"可疑"继续不阻断——误报无害偏保守，漏报=逐槽核对人工闸兜底）
	//失败降级铁律（12 §2.1/E3-6）：逐槽失败/超时/单色可疑/视口关闭/动画播放中→该槽留空走
	//缺项策略"-"+原因分类聚合（E3-6），继续下一槽绝不阻塞导出、绝无假图（C7/D3-4 全继承）。
	//纯逻辑判定面（classifySlotFailure/isSuspectMonochrome/mappingsFromConfig）为静态纯函数
	//=镜像自测锚（批次 B/C 惯例）。

	//槽位映射四元组（FR-332；frameOnly=true=仅帧维应用——SolidColor 伪变量槽/旧
	//slotFrameMap int 形态迁移特例，13 §4.2；frameId<=0=不切帧直接抓当前画面）
	struct CaptureMapping
	{
		QString caseName;     //工况（空=仅帧维）
		int frameId = -1;     //帧号（<=0=不切帧）
		QString var;          //变量名（"SolidColor"=网格图伪变量→frameOnly）
		QString compo;        //分量
		bool frameOnly = true;
	};

	//逐槽截图结果（E3-6 原因分类聚合数据面；导出汇总披露行=第三行拼接消费）
	struct CaptureSlotResult
	{
		QString slotKey;
		bool ok = false;          //true=已落图（suspect 可真——可疑不失败）
		bool suspect = false;     //单色可疑（提示性不阻断，O3-5）
		bool skipped = false;     //未映射跳过（非失败——保持槽现状）
		QString path;             //成功=落盘 PNG 路径
		QString reasonCode;       //""|ViewportClosed|RenderUnreachable|Playing|
		                          //MappingMismatch|Timeout|Aborted|GrabFailed|ImageUnreadable
		QString reason;           //人读原因（分类汇总/披露行原文）
	};

	class MPOSTWINDOW_EXPORT mStrengthReportAutoCapture : public QObject
	{
		Q_OBJECT
	public:
		explicit mStrengthReportAutoCapture(QObject* parent = nullptr);

		//映射解析（R3-9 兼容读取）：槽键→四元组——slotMappings 命中→解析（frame>0 校验，
		//非法丢弃）；未命中→旧 slotFrameMap int 形态=仅帧维特例（var="SolidColor" 语义）；
		//两处皆无=该槽不入映射表（调用方=跳过槽保持现状）。返回表键序=slotKeys 序
		static QMap<QString, CaptureMapping> mappingsFromConfig(
			const mStrengthReportProjectConfig* cfg, const QStringList& slotKeys);

		//失败分类纯判定（镜像自测锚；T3-2 口径=三道闸独立、回显校验闸先于等待闸触发、
		//早退/失配分类 E3-8 不落超时误报）。优先级：中止＞视口/会话不可达＞播放中＞
		//映射失配（前置守卫/等待后缓存旁证失配）＞超时＞抓取失败。返回 reasonCode（""=可抓取）
		static QString classifySlotFailure(bool viewportOk, bool renderOk, bool playing,
			bool mappingValid, bool preGuardOk, bool received, bool aborted,
			bool deadlineHit, bool cacheMismatch);

		//单色检测（O3-5 双判据，ε 初值保守=宁误报不漏报，真机矩阵定版）：图空=false
		//(调用方按 GrabFailed/ImageUnreadable 分类)；亮度极差≤6 且 标准差≤2.0→true（可疑）
		static bool isSuspectMonochrome(const QImage& img);

		//批量执行（GUI 线同步逐槽；槽内等待走嵌套 QEventLoop——UI 步间保持响应，
		//与二期试点 grabWithAlign 同构；progress=逐槽进度回调可空）
		QList<CaptureSlotResult> runBatch(const QMap<QString, CaptureMapping>& mappings,
			const QStringList& slotKeys, bool watermarkOff, QWidget* hideDuringGrab,
			const std::function<void(int, int, const QString&)>& progress);

		//中止通道（R3-7：视口抢操作/视口关闭/对话框关闭→中止剩余槽留空——既有
		//abortAlignWait B-6① 同口径扩展到批量级；Dlg 侧四个既有中止点经 abortAlignWait 漏斗接入）
		void abortBatch();
		bool batchActive() const { return _batchActive; }

	private:
		CaptureSlotResult captureOne(const CaptureMapping& mapping, const QString& slotKey,
			bool watermarkOff, QWidget* hideDuringGrab);

		//等待态（与 Dlg 试点 _alignLoop/_alignAborted 同构——批量级互斥与中止）
		bool _batchActive = false;
		bool _batchAborted = false;
		QPointer<QEventLoop> _batchLoop;   //活跃等待循环（非空=等待中，abortBatch→quit）
	};
}
