#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
class QToolButton;

namespace StrengthReport
{
	class mStrengthReportProjectConfig;

	//——三期批次 A（FR-341/342/343+FR-301/302/303+FR-325 骨架，13-技术路线-三期 §3/§4.1）——
	//Tab0 一键导出主视图：装配引擎（AutoAssembler）与主视图部件（mStrengthReportAutoPreview）
	//同文件对分立成类——UI 与取值分离可镜像自测（批次 B/C 惯例延续）。
	//——三期批次 B（FR-311/312/313/321/322/323/324/325/326，13 §3 批次 B）扩展——
	//collectElementType/scanCaseResults 通道族（T3-1 只读读文件通道实证可达）+automation 节
	//读取契约（D3-05 改判=规则层承载，T3-4③ 逐键错型守卫）+默认句机制+剩余系数自动计算
	//+分步装配会话（R3-5/O3-6：读文件通道含 IO 耗时，QTimer 逐帧分步防界面冻结）。

	//逐自动键取值结果（12-设计方案-三期 §2.2 来源/置信度标注落地；13 §4.1 定型）
	struct AutoValue
	{
		//PA-06/PA-04 消根：默认值显式 Null（QJsonValue 默认构造=Undefined，而
		//QJsonObject::insert(Undefined)=删键、isNull() 对 Undefined 为 false——
		//缺省 AutoValue 一律按"无值=null 走缺项策略"语义）
		QJsonValue value = QJsonValue(QJsonValue::Null);
		QString source;          //"录入"|"自动·计算文件"|"自动·登录名"|"自动·单元计数"|
		                         //"自动·结果扫描"|"自动·自动计算"|"规则层默认句"|"规则层未配置"|""
		QString confidence;      //"high"|"warn"（⚠ 需注意）|"fail"（失败回退，汇总红字）|"info"（中性）
		QString warn;            //失败/需注意原因（E3 聚合告警原文）
	};

	//automation 节读取契约（13 §4.3 承载结构=GUI 侧读取面；规则文档措辞权威在 PM/D-18）。
	//T3-4③ 逐键类型守卫：错型条目剔除→该键 null+typeWarns 逐键定位（R3-11——不崩溃
	//不静默失实），合法条目照常消费（D3-06 裁定=tdrrules 不加轻校验，GUI 侧守卫兜底）
	struct AutomationConfig
	{
		bool present = false;                     //规则层有 automation 节（缺席=E3-11 全回退缺项）
		QMap<QString, QString> defaultSentences;  //默认句（键=数据集键位；仅字符串条目）
		struct VarSpec
		{
			bool valid = false;                   //键型合法（names=字符串数组 且 compo=字符串）
			QStringList names;                    //候选变量名清单（按序试探）
			QString compo;                        //分量
		};
		VarSpec stressPeak;                       //stress_peak 映射（FR-321）
		VarSpec displacement;                     //displacement 映射（FR-322/T3-5）
		bool displacementMagnitude = false;       //metric=="magnitude"（Q3-5 冻结=模长口径）
	QMap<QString, int> bucklingMap;           //工况→失稳阶次（FR-323；正整数条目）
	QMap<QString, double> materialLimits;     //材料名→极限强度（FR-324/Q3-6=规则层配置表）
	QStringList invalidKeys;                  //错型键位清单（告警定位）
	QStringList typeWarns;                    //逐键告警原文（聚合进 AssemblyResult.warns）
	bool varMapShapeFailure = false;          //BUG-303：形态识别失败（嵌套/平铺键在位而均
	                                          //非法）——预览 ⚠ 行+导出汇总披露，不留静默
	};

	//装配请求（13 §4.1；automation=规则层 automation 节对象（无节=nullptr——E3-11 回退缺项），
	//rulesDocument=槽位清单文档（批次 C 槽位建议消费））
	struct AssemblyInput
	{
		const mStrengthReportProjectConfig* config = nullptr;   //录入值优先级最高
		const QJsonObject* automation = nullptr;                //规则层 automation 节（无节=nullptr）
		const QJsonDocument* rulesDocument = nullptr;           //槽位清单/automation 读取文档
	};

	//装配结果（13 §4.1；批次 B 扩展：cases 逐工况扫描+materialReserves 材料级剩余系数）
	struct AssemblyResult
	{
		QMap<QString, AutoValue> items;   //键=数据集键位（meta.title/manual_fields.project_subject/…）
		struct CaseScan                   //FR-321/322/323 逐工况结果+峰值帧号副产品（FR-333 供批次 C）
		{
			QString caseName;
			AutoValue stressPeak;         //"自动·结果扫描"
			AutoValue dispMax;
			AutoValue buckling;
			AutoValue reserve;            //该工况关联峰值口径的剩余系数（自动·自动计算）
			//PA-03：峰值帧副产品按链拆分（原单一 peakFrameId/Name 被应力/位移两条
			//mergeFramePeak 链共写，终值=任务序上最后自我改进的合并——多帧样例常态
			//下"峰值帧"失真）。FR-333 五槽建议源=应力口径（stressPeakFrameId，结构
			//注释锁死；13 §6"最大应力槽=应力峰值帧；位移槽=位移最大值帧"）
			int stressPeakFrameId = -1;   //应力峰值帧（FR-333 建议源，批次 C 消费）
			QString stressPeakFrameName;
			int dispMaxFrameId = -1;      //位移最大值帧
			QString dispMaxFrameName;
		};
		QList<CaseScan> cases;
		QMap<QString, AutoValue> materialReserves;   //材料名→剩余系数（保守侧=全工况最大峰值口径）
		QStringList warns;                //聚合告警（逐键原文；E3-1~E3-4/E3-11/R3-11 分类）
		int autoCount = 0;
		int defaultSentenceCount = 0;     //默认句 N 处（FR-312/313 汇总披露）
		int warnCount = 0;
		int failCount = 0;                //汇总行"自动值 N·默认句 N·需注意 N·失败 N"（FR-343 分级）
		QStringList failKeys;             //BUG-304（fix6）：失败键逐键明细（键位+原因短文案）——
		                                  //与 failCount 同源自增（长度恒等 failCount），Tab0 汇总
		                                  //tooltip 逐键呈现（消"失败 N 无明细可见"UX 缺口）
	};

	//装配引擎（只读数据通道，零视口操作零抓图——12 §2.1 S1 口径）。
	//优先级合并=录入＞自动＞默认句＞null，失败回退逐键硬编码 null（C7 铁律）
	class MPOSTWINDOW_EXPORT AutoAssembler
	{
	public:
		//automation 节解析（T3-4③ 逐键错型守卫；无节→present=false，E3-11 全回退缺项）
		static AutomationConfig parseAutomation(const QJsonObject& automation);

		//同源注册名复合格式（T3-3①/B-4：mPostDataBase.cpp:1939-1955 拼接语义镜像——
		//"变量名 分量名"；compoName 含 varName 时（大小写不敏感）不拼接=单分量裸名特例）
		static QString compositeResultName(const QString& var, const QString& compo);

		//单元类型自动采集（FR-311/E3-1）：framei 取法同 collectModelStats（guidata->
		//_cloudFrame fallback 1）→getModelData(framei)→for type∈[0,elementMetaCount)
		//getNumByEleType→占比最大型主串（并列取枚举序首个；NoneElement 跳过）；
		//全零计数/帧未装载/null+聚合告警（录入优先级在 assemble/会话内合并）
		static AutoValue collectElementType(const mStrengthReportProjectConfig* cfg);

		//剩余系数纯计算核（FR-324/E3-4）：limit÷peak；除零/任一缺失/非有限值=null+告警
		//（绝不静默计算）。分子=automation.material_limits 配置表（Q3-6=规则层承载；
		//12 §2.3"枚举不进首版代码路径"——getAllProps 增强列候选池不入本期）
		static AutoValue computeReserveFactor(double limit, double peak);

		//同步全量装配（批次 A 形态保留：镜像自测/退化断言用；Dlg 走 AutoAssemblySession
		//分步形态——读文件通道逐帧 IO，同步全量仅适合单帧小模型）
		static AssemblyResult assemble(const AssemblyInput& input);
	};

	//分步装配会话（三期批次 B，13 §3 批次 A 骨架"异步逐项装配+预览逐项刷新"的落地形态；
	//R3-5/O3-7/T3-1③：readVarDatas 读文件通道单帧读取含秒级 IO——QTimer 逐帧分步在 GUI 线
	//推进（不引工作线程——postDB/mMxdbThread 调用契约均在 GUI 线先例内），步进单元=
	//一个（工况,帧）的映射变量读取，UI 在步间保持响应）
	class MPOSTWINDOW_EXPORT AutoAssemblySession
	{
	public:
		explicit AutoAssemblySession(const AssemblyInput& input);

		//推进一步（一个装配单元）；返回 false=全部完成（result 可用）
		bool step();

		const AssemblyResult& result() const { return _result; }
		int doneSteps() const { return _done; }
		int totalSteps() const { return _total; }
		bool finished() const { return _done >= _total; }

	private:
		void initMeta();          //步 0：元信息 5 键+automation 解析+默认句+单元类型
		void initScanPlan();      //扫描计划（逐工况逐帧；无 postDB=零扫描步，全 null 退化）
		void stepScanFrame();     //扫描步：一个（工况,帧）的应力峰值/位移读取
		void finishReserve();     //收尾步：剩余系数+汇总计数

		AssemblyInput _input;
		AutomationConfig _automation;
		AssemblyResult _result;
		struct FrameTask          //一个扫描单元
		{
			int caseIndex = -1;
			QString caseName;
			int frameId = -1;
			QString frameName;
		};
		QVector<FrameTask> _tasks;
		int _taskIdx = 0;
		int _done = 0;
		int _total = 0;
		bool _reserveDone = false;
	};

	//Tab0 一键导出主视图部件（12-设计方案-三期 §3.1 蓝图）：装配预览一览表（输入项/预览值/
	//来源/置信/操作 五列，QTableWidget 只读——消双真源，覆盖仍在页签原控件）+汇总行+
	//签栏说明行（FR-303）+高级设置折叠区入口（控件本体仍在页签①④，折叠区=入口不改布局）
	//+自动截图占位行（批次 C 填充）+主按钮。
	//六态局部子状态机（13 §2.1：与既有 LoadState 五态/probe 异步/_exporting 旗标正交组合
	//而非替换）：S0 Idle→S1 Assembling（装配）→S2 Ready（一键导出使能）→S3 AutoCapture
	//（批次 C，本批占位不进入）→S4 Assemble/S5 Engine（既有 slot_export 链内，正交透传）
	//→S6 Done（完成汇总）；模型/容器变化→回 S1 重装配。
	class MPOSTWINDOW_EXPORT mStrengthReportAutoPreview : public QWidget
	{
		Q_OBJECT
	public:
		//覆盖跳转目标（12 §3.1"○可覆盖 点击→跳对应页签定位控件"；预览只读消双真源）
		enum JumpRole
		{
			JumpSoftwareName = 0, JumpProjectSubject = 1, JumpExportTab = 2,
			//批次 B（FR-311/312/313）：单元类型+默认句四键→页签①对应录入框
			JumpElementType = 3, JumpConstraintDesc = 4, JumpLoadCaseDesc = 5,
			JumpLoadType = 6, JumpCalcMethod = 7
		};

		explicit mStrengthReportAutoPreview(QWidget* parent = nullptr);

		void beginAssembly();                          //→S1：装配中（主按钮禁用）
		void applyResult(const AssemblyResult& result); //→S2：预览三列+汇总行刷新
		void setExporting(bool on);                    //S2↔S4：与既有导出链正交同步
		void setStageText(const QString& text);        //S5：引擎阶段消息透传（既有进度链）
		void finishExport(bool ok, const QString& headline); //→S6：完成态
		void resetSession();                           //CB-01：项目切换会话重置（回 S0 空态）
		void setExportEnabled(bool on);                //主按钮使能（与 LoadState/probe/exporting 正交）

		//三期批次 C（FR-333，12 §2.5）：截图槽建议行——建议值黄字"建议·来自自动扫描"，
		//采纳才落 slotMappings（建议≠配置，save/load 往返不落建议）；清空=回退手动。
		//Tab0 只读消双真源纪律延伸：建议/映射编辑本体在页签③[映射设置…]（五列四元组），
		//Tab0 行内仅[采纳建议]/[清除]两动作钮
		struct CaptureSlotRow
		{
			QString key;
			QString mappingText;     //当前映射（四元组文本；"-"=未映射）
			QString suggestionText;  //建议（空=无建议——关键词不匹配/automation 缺映射）
		};
		void setCaptureSlots(const QList<CaptureSlotRow>& rows); //截图槽列表刷新（无行=隐藏表）
		void setCaptureHint(const QString& html);   //截图提示行填充（批次 A 占位行批次 C 交付）
		void enterCapturePhase();                   //LG-08：S3 批量截图段落 phase 呈现（12 §2.1
		                                            //六态图对齐——phaseLabel 进 S3 文案）
		//LG-05（UB-05）：页签①录入改写→Tab0 预览对应行即时同步（值+来源，默认"录入"）——
		//消"预览快照过时第三真源"（预览与实际发射脱节）；轻量单行更新，不整链重装配。
		//R2L-04：来源可显式指定（软件名清空=发射走首期默认常量非缺项——镜像标"（默认）"）
		void updateManualEntryRow(const QString& itemKey, const QString& value,
			const QString& source = QStringLiteral("录入"));

	signals:
		void oneClickRequested();     //主按钮→Dlg::slot_oneClickExport（装配前置→既有 slot_export 同链）
		void jumpRequested(int role); //覆盖跳转→Dlg 定位页签控件
		void adoptSuggestion(const QString& slotKey);   //采纳建议→Dlg 写 slotMappings（采纳才落）
		void clearSlotMapping(const QString& slotKey);  //清除映射→Dlg 回退手动
		void editMappingsRequested();                   //→Dlg 打开页签③映射设置（五列四元组）

	private:
		void buildUi();
		void setPhase(int phase);      //六态推进（单点改状态行文案）
		void rebuildTable();           //预览表逐项刷新（装配完成后/会话重置共用）
		void updateSummary();          //汇总行（FR-343 三态告警分级：黄字需注意/红字失败）

		QTableWidget* _table = nullptr;
		QLabel* _phaseLabel = nullptr;
		QLabel* _summaryLabel = nullptr;
		QLabel* _captureHint = nullptr;
		QToolButton* _advancedToggle = nullptr;
		QWidget* _advancedPanel = nullptr;
		QPushButton* _mainBtn = nullptr;
		QTableWidget* _captureTable = nullptr;  //三期批次 C：截图槽建议列表（无行=隐藏）
		QPushButton* _captureEditBtn = nullptr; //[映射设置…]入口（页签③五列四元组编辑）
		AssemblyResult _result;
		int _phase = 0;                //Phase 枚举值（Idle=0）
		bool _exportEnabled = false;
		bool _exporting = false;
		QString _lastHeadline;         //S6 完成态首行文案
	};
}
