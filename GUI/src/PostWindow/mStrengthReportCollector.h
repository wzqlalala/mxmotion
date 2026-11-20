#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QJsonObject>
#include <QStringList>

class QJsonDocument;
class QWidget;

namespace StrengthReport
{
	class mStrengthReportProjectConfig;
	struct AssemblyResult;   //三期批次 B：装配合并口可选形参（前向声明——AutoPreview.h 定型）

	/*
	* 数据采集器（03 §4.5 mStrengthReportCollector）：
	* 只读复用既有通道——模型统计（mPostDataBase 取数样板=mPostExportWord.cpp:1351-1413
	* getResultInfo：getCVM→getManager<mPostDataManager>(Physical_Post)→getPostDatabase），
	* 工况列表（getAllCaseNames）、材料名清单（二期 FR-201/202，09 §0.1 三级链）、
	* 视口屏拷截图（activatePostViewport+grabWindow；二期批次 C 水印档参数化——09 §0.2）。
	* 全部静态只读，不修改任何既有收集器（FR-51 冻结纪律）。
	* ★二期批次 B（FR-241 录入版，Q2-3=A）：cases[].results 四键从"恒 null"转"页签②
	*   录入值或 null"（配置 caseResults 按工况名键控）——自动采集列三期，本批不做
	*   读后处理结果通道（03 §4.5 占位接口 collectCaseResults() 的"读结果通道"路径
	*   维持未启用，无需复审）；verdict 推导=材料级 min 口径（D2-2/C-2，09 §0.4）
	*/
	class MPOSTWINDOW_EXPORT mStrengthReportCollector
	{
	public:
		//当前项目标识（模型级，D-12/B17）：当前后处理数据名净化做目录名；无激活视口=空串
		//（调用方落"default"兜底，保证配置/截图目录仍可定位）
		static QString currentProjectId();

		//模型统计只读（FR-42）：{software_name, node_count, element_count}；
		//通道任一环缺失→对应键 null 并 err 汇总（A-08：缺失键按 null 填，勿复制恒 return true 形态）。
		//software_name 单键单值（D-24①），取 config 录入值，未录入用产品默认常量；
		//★不含 element_type——单元类型按 U-06/T-04-b 唯一来源=manual_fields（V-01/C3-05）
		static QJsonObject collectModelStats(const QString& softwareName, QString& err);

		//工况列表（mPostDataBase::getAllCaseNames，mPostDataBase.h:257）；无 postDB=空表
		static QStringList collectCaseNames();

		//载荷列集（T-09 实测列集：position/end_load/pressure/temperature）。
		//首期工况载荷收集通道（PartInfo 类比样板）未核实，按 03 §4.5"通道缺失键=null"
		//全部填 null（E1 标缺，不静默填错）——通道核实后在此填真值，契约零改动
		static QJsonObject collectCaseLoads();

		//二期 FR-201 材料名清单（09 §0.1 三级链）：①postDB 材料映射 getMaterialParts
		//（结果文件读取链回填，纯后处理态可达）键集非空→D3-2 定序（各材料映射零件
		//min(partId) 升序、同序名称字典序——QHash 无稳定遍历序的确定性发射口径），
		//来源="结果文件"；②键集空→回退模型侧 getAllNames(Physical_Material)（仅建模
		//模型在役时有值），保持返回序，来源="模型数据"；③双空=ok=false+err 汇总
		//（FR-202：materials=[]+聚合告警 1 条，导出继续）。D3-1：材料名"default"
		//（v06+ 无材料部件占位名）不过滤逐字进清单（C-4 裁定；页签②显著标注）
		struct MaterialList
		{
			QStringList names;
			QString source;    //"结果文件"|"模型数据"（页签②来源列展示用）
			bool ok = false;
		};
		static MaterialList collectMaterialNames(QString& err);

		//数据集 JSON 装配（02 §4.2 契约 FR-41+二期 §4.1 增量）：返回 ok/err 语义（A-08）。
		//二期批次 B 发射：missing_policy_override（Q2-7=A 映射，空/未知值不发射=一期语义）、
		//cases[].results 四键录入值或 null、materials[] 四键（name=采集值/allowable_stress=
		//schema 预留 null/max_stress+reserve_factor=录入值或 null）、conclusion.verdict=
		//本次发射材料级 reserve_factor 已录值最小值（数值）或 null（D2-2/C-2；全空→
		//结论句整句走槽级缺省文案 D-4）；conclusion.pass/fail=分支文案字面（对象名已录时
		//发射，htsy 条件措辞槽 use 键取文案——FR-26 措辞激活配套，见 .cpp 注记）
		static QJsonDocument buildDataset(const mStrengthReportProjectConfig& cfg, QString& err);

		//三期批次 B 装配合并口（13 §3 Collector 行，FR-325）：autoValues 非空时逐键按
		//"录入＞自动＞默认句＞null"优先级合并装配结果——meta.title/author/calc_file+
		//manual_fields.project_subject/element_type（"自动"来源）+默认句四键+cases.load_type
		//（"规则层默认句"来源）+cases[].results 四键（扫描值）+materials[].reserve_factor
		//（保守侧=全工况最大峰值分母）+conclusion.pass/fail 门随自动对象名激活；
		//数据集契约零新增键（全部落既有键位填充）；nullptr=二期形状逐字节兼容锚
		//（2831840b 退化一致性，R3-2）
		static QJsonDocument buildDataset(const mStrengthReportProjectConfig& cfg, QString& err,
			const AssemblyResult* autoValues);

		//视口屏拷截图（FR-43/T-21）：activatePostViewport()+QScreen::grabWindow（winId，与
		//运行时显示一致）。T-21 三项全落：
		//③首图预热=先 subwin->grab() 离屏抓一次（mPostExportWord.cpp:366-375 先例，防首图发黑）；
		//②防遮挡=hideDuringGrab（对话框）先 hide→processEvents 一轮→grab→restore（B14）；
		//①画面新鲜度=一期手动（以用户所见为准）；二期批次 C 试点"对齐抓取"的就绪等待
		//（frameChanged 过滤+同步 grab+防抖）在 Dlg 侧先行完成后才进入本链（09 §4.5）。
		//QScreen 取目标窗口所在 screen()（B20，勿照抄 primaryScreen 硬编码）；输出格式固定 PNG
		//（A-07）；落 appDir/Report/strength-report-images/<项目标识>/<slotKey>/pic_N.png
		//（B17 层级+跨重名计数，样板 mPostExportWord.cpp:506-513）
		//★二期批次 C（FR-231/232 去水印，Q2-4=A；09 §0.2 屏拷 10 步调用序）：
		//watermarkOff=true 时抓取经渲染门面 setLogoVisible 临时翻转（只调用既有接口，
		//渲染库零修改），全链以 RAII 栈守卫（LogoFlipGuard）包络——构造=按开关期望态强制
		//复位（E2.5 防上次异常残留），析构=恢复可见+update()（C-5：覆盖抓空/落盘失败/异常
		//全部早退路径，C++ 无 finally）；rend 为空→按带水印抓并经 *warn 告警（开关失效，
		//不阻断）。FastCapture FBO 备选通道本批不实现（08 §2.4 备选留接口注释：切换=
		//Collector 内图像来源函数替换，槽位管理/落盘/绑定链零变化——FR-232 同构）
		static bool saveViewportScreenshot(const QString& slotKey, QWidget* hideDuringGrab,
			bool watermarkOff, QString& outPath, QString& err, QString* warn = nullptr);

		//C-5 兜底复位（主对话框关闭/导出完成时调用一次）：渲染门面在位→强制 logo 可见
		//+请求重绘，防异常残留把产品水印长期丢失（E2.5；正常路径由 LogoFlipGuard 析构收口）
		static void restoreViewportLogo();
	};
}
