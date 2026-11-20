#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QDialog>
#include <QJsonDocument>
#include <QPointer>
#include <QSet>
#include <QStringList>

#include "mStrengthReportConfig.h"
#include "mStrengthReportContainerProbe.h"
#include "mStrengthReportAutoPreview.h"   //三期批次 A：Tab0 主视图部件+装配结果类型
#include "mStrengthReportAutoCapture.h"   //三期批次 C：全自动批量截图链控制器+逐槽结果

class QCheckBox;
class QComboBox;
class QEventLoop;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class QTabWidget;
class QProgressBar;

namespace StrengthReport
{
	class mStrengthReportContainerProbe;
	class mStrengthReportEngineRunner;
	class mStrengthReportProjectConfig;

	/*
	* "强度计算报告导出"主对话框（R10 独立对话框；02 §3.2~3.4；03 §4.5）：
	* 顶部规则模板加载态栏（浏览/重新校验/状态行五态）+ 四页签（①基础信息与文本槽
	* ②工况与表格 ③截图槽管理 ④导出）+ 底部进度/保存为项目配置/重新校验/一键导出。
	* 非模态打开（T-23/B15/U-01=A：页签③"切变量↔回抓取"交替流在应用级模态下不可达）；
	* 同输出路径并发面=引擎启动前占用探测一处闭合（C-10）。
	* 槽位清单由已加载容器 rules 声明驱动（T-08，UI 不写死槽数）。
	* 状态机（02 §3.4）：未加载/容器损坏/校验失败 N 条[查看全部]/版本不符/
	* 已加载·有缺项（黄字预警，不阻断）/导出中（仅[取消]可用）/成败回显。
	* 视口与项目状态（T-23/D-13）：sig_viewportRemoved/切换模型→[抓取当前视口]禁用、
	* 页签①自动采集区标"数据待刷新"、已抓截图保留（按 <项目标识>/<槽键> 目录组织——V-05）。
	*/
	class MPOSTWINDOW_EXPORT mStrengthReportDlg : public QDialog
	{
		Q_OBJECT
	public:
		//单例弹出（非模态：不设 WindowModality——既有 mExportReportDlg 应用级模态理由
		//=同步渲染独占，TDR 异步导出已消除；重复调用置前）
		static void showInstance(QWidget* parent = nullptr);

	private:
		explicit mStrengthReportDlg(QWidget* parent = nullptr);

	public:
		//CB-03：Esc 走 QDialog::reject() 不经 closeEvent——导出中同样二次确认（02 §3.6）
		void reject() override;

	private:
		void buildUi();
		QWidget* buildTabOneClick();  //三期批次 A：Tab0 一键导出主视图（前插默认页，13 §3）
		QWidget* buildTabBasic();     //页签①基础信息与文本槽
		QWidget* buildTabCases();     //页签②工况与表格
		QWidget* buildTabImages();    //页签③截图槽管理
		QWidget* buildTabExport();    //页签④导出

		//——三期批次 A（FR-341/342/343 一键导出主视图+元信息全自动，13 §3 批次 A）——
		void startAutoAssembly();      //S1 装配触发（probe 完成 Loaded/视口变化；QTimer 单拍异步）
		void runAutoAssembly();        //S1 装配执行（AutoAssembler 只读通道）→S2 预览刷新
		void runAutoAssemblyStep();    //三期批次 B：分步装配步进（R3-5/O3-6——读文件通道
		                               //逐帧 IO，QTimer 逐帧推进防界面冻结；完成→S2 收口）
		void applyAutoPrefill();       //装配结果→页签①对象名预填（UI 内存态不落配置，E3-5 显示净化）
		void updateAutoPreviewUi();    //Tab0 主按钮使能（与 LoadState/probe/exporting 正交刷新）
		//三期批次 C（FR-331/332/333，13 §3 批次 C）：全自动批量截图段+Tab0 建议行刷新
	bool runAutoCaptureBatch();    //S3 截图段：开关关=完全不进入（行为与批次 B 逐字节一致，
	                               //Q3-3 默认关）；开=AutoCapture 批量链→成功槽入 imagePaths。
	                               //LG-02：批量段并入导出互斥态（setExporting(true)——页签/底部
	                               //导出/行内抓取全部禁用）；返回 true=批量段已实际进入
		void refreshCaptureRows();     //Tab0 截图槽列表（映射四元组列+建议列）刷新
		int adoptAllCaptureSuggestions();   //D-07：逐槽采纳 Tab0 截图建议（与手动[采纳建议]
		                                    //逐行同语义，复用 slot_adoptCaptureSuggestion）；
		                                    //返回实际采纳条数（无建议槽跳过）

		//规则层槽位声明解析（probe 结果 rulesDocument → 页签①文本槽预览+页签③图片槽行）
		struct ImageSlot
		{
			QString key;          //槽键（bind=image.<key>.(path|caption) 剥中段；锚点形态展示用）
			QString anchorDesc;   //锚点形态描述（占位文本/空段坐标/表格坐标——02 §2.2 ④三形态）
			QString bind;         //原始绑定键（path 形态优先作主绑定展示）
			QString caption;      //子题注格文本（表格坐标形态）
			bool captionBind = false;  //CB-05：该槽存在 image.<key>.caption 绑定（题注列可录入）
		};
		void parseRulesDocument(const QJsonDocument& doc);
		void refreshTextSlotPreview();
		QString textSlotCurrentValue(const QString& bind) const;   //T1：绑定键当前值（复用既有采集/装配取值链）
		void rebuildImageTable();              //页签③行重建（槽位清单=规则层图片槽声明驱动）
		//二期批次 B：页签②两组录入面行重建（行=采集自动+按名匹配回显配置；C-4 default 行
		//黄字标注；08 §2.3 失配条目黄字提示）
		struct MaterialRow
		{
			QString name;      //材料名（采集值，行只读）
			QString source;    //来源通道标注（"结果文件"/"模型数据"）
			bool isDefault = false;   //D3-1："default"占位名（C-4① 显著标注）
		};
		void rebuildCaseEntryTables();
		void onCaseResultItemChanged(QTableWidgetItem* item);   //页签②工况结果表逐字段校验+落配置
		void onMaterialValueItemChanged(QTableWidgetItem* item); //页签②材料表逐字段校验+落配置
		void updateVerdictLabel();             //verdict 推导行（材料级 min 三态实时刷新）
		void updateEntryMsgLabel();            //录入消息行（校验错误/历史失配/清洗计数三源）
		void setImageActionsEnabled(bool grabEnabled);  //无激活视口→抓取禁用、导入/清除可用
		void switchProjectConfigIfNeeded();    //CB-01：项目标识变化→存旧项目配置+载入新项目配置（D-12）
		void echoConfigToUi();                 //配置回显全部页签（构造与 CB-01 项目切换共用）

		//——二期批次 C（FR-231 去水印 + FR-221 半自动试点，09 §0.2/§0.3/§4.5）——
		//试点"对齐抓取"（08 §2.5 四步流程）：目标对齐（映射表仅帧标识 D3-3，空映射槽跳过）
		//→等就绪（C-1 双硬约束：frameId+UserSeek 过滤/connect 先于切换）→update+processEvents
		//→同步 grab() 上屏同步→200ms 防抖→复用抓取链（含水印开关）；超时 5s/中止→不抓取
		//回退手动（无假图）
		bool grabWithAlign(int row, bool watermarkOff, QString& outPath, QString& err,
			QString* warn);
		void abortAlignWait();                 //视口关闭/移除/对话框关闭→立即中止试点等待（B-6①）
		void updateGrabButtonsForPilot();      //试点开关→逐槽抓取钮文案（"抓取"/"对齐抓取"）

		//加载态栏状态机（02 §3.4 五态+缺项）：刷新状态行文案/颜色与[一键导出]可用性
		void applyLoadState(const QString& statusHtml);
		void refreshCollectedData();   //页签①自动采集+页签②工况列表（只读通道刷新）
		void refreshManualCandidates();   //人工补录区 6 下拉候选重填（通用词表+模型数据；保留当前录入值）
		int countMissingItems() const; //缺项预检（E1 前置预警）：单元类型/对象名/图片槽缺图
		void setExporting(bool exporting);   //导出中：全页签与按钮禁用，仅[取消]（02 §3.4）
		void collectUiToConfig();      //页签①③录入值回写 config（保存/导出前调用）
		bool ensureOutputPath();       //输出路径空→弹保存对话框补齐+补 .docx 后缀（渲染器先例）

		QString datasetJsonPath() const;   //tempDir/mx_strength_report_tmp/report_dataset.json（E10）
		QString probeDatasetJsonPath() const;  //CB-04：校验专用数据集（与导出数据集分文件，防校验读/导出写竞写）

	private slots:
		void slot_browseContainer();
		void slot_openImportWizard();          //二期批次 D：模板导入向导（FR-251，08 §2.6）——
		                                       //定版成功回填加载栏+自动重校验
		void slot_revalidate();
		void slot_onProbeFinished(const StrengthReport::mStrengthReportContainerProbe::Result& result);
		void slot_viewportChanged();           //currentViewportChanged：采集刷新+抓取可用性
		void slot_viewportRemoved(int id);     //sig_viewportRemoved：抓取禁用+采集区待刷新标记
		void slot_grabImage(int row);          //页签③逐槽抓取当前视口（T-21 时序）
		void slot_importImage(int row);        //页签③从文件导入（U-04 外部图通道）
		void slot_clearImage(int row);         //页签③清除槽图
		void slot_saveConfig();                //底部[保存为项目配置]（FR-45 回显）
		void slot_missingPrecheck();           //页签④缺项预检（黄字预警清单）
		void slot_export();                    //一键导出（02 §3.3 三步收敛）
		void slot_cancelExport();
		void slot_onStageProgress(const QString& stage, int pct, const QString& msg);
		void slot_onExportFinished(bool ok, const QString& summary);
		void slot_openErrorList();             //状态行"查看全部"链接→条目级错误列表弹窗
		void slot_semiAutoToggled(bool on);    //二期批次 C：页签③试点开关（一次性提示+按钮文案切换）
		void slot_editSlotFrameMap();          //二期批次 C：槽位映射表编辑（仅帧标识——D3-3）
		void slot_oneClickExport();            //三期批次 A：Tab0 主按钮=装配前置→既有 slot_export 全链
		void slot_jumpToControl(int role);     //三期批次 A：预览"可覆盖"跳转定位页签控件（消双真源）
		//三期批次 C（FR-333）：Tab0 截图槽行动作（采纳建议→写 slotMappings=采纳才落；
		//清除→回退手动；编辑→页签③映射设置）
		void slot_adoptCaptureSuggestion(const QString& slotKey);
		void slot_clearCaptureMapping(const QString& slotKey);

	protected:
		//导出中关闭→二次确认+cancel()（03 §4.5 closeEvent/T-15；Esc 同路径）
		void closeEvent(QCloseEvent* event) override;

	private:
		//顶部加载态栏
		QLineEdit* _containerPathEdit = nullptr;
		QPushButton* _browseContainerBtn = nullptr;
		QPushButton* _importTemplateBtn = nullptr;  //二期批次 D：向导入口（08 §2.6/§3.4，
		                                            //FR-251：任意加载态可用）
		QPushButton* _revalidateBtnTop = nullptr;
		QLabel* _statusLabel = nullptr;

		QTabWidget* _tabs = nullptr;
		//页签①
		QLineEdit* _softwareNameEdit = nullptr;
		QLineEdit* _nodeCountEdit = nullptr;
		QLineEdit* _elementCountEdit = nullptr;
		QLabel* _modelSwitchHint = nullptr;       //D-13：模型已切换提示
		QLineEdit* _constraintDescEdit = nullptr;
		QLineEdit* _loadCaseDescEdit = nullptr;
		QLineEdit* _loadTypeEdit = nullptr;
		QLineEdit* _calcMethodEdit = nullptr;
		QLineEdit* _elementTypeEdit = nullptr;    //U-06 单元类型补录（不得标缺）
		QLineEdit* _projectSubjectEdit = nullptr; //D-24② "XXX"对象名
		//人工补录区改可编辑下拉框（本轮）：6 字段 QLineEdit→QComboBox(editable)，上方
		//*_Edit 成员=各下拉内嵌 lineEdit()——既有 setText/text()/textEdited 读写链与
		//"仅用户编辑触发"信号语义零改动；候选=内置通用词表+模型数据（refreshManualCandidates 重填）
		QList<QComboBox*> _manualCombos;          //顺序=约束形式/加载形式/载荷类型/计算方法/单元类型/对象名
		QTableWidget* _textSlotPreview = nullptr;
		//页签②（二期批次 B，08 §3.1）：工况结果表+材料表两组录入面
		QTableWidget* _caseResultTable = nullptr; //行=工况（只读），列=应力峰值/位移最大值/失稳模态阶次/剩余系数（录入）
		QTableWidget* _materialTable = nullptr;   //行=材料（只读+来源列），列=最大应力/剩余系数（录入）
		QLabel* _entryNoticeLabel = nullptr;      //固定录入提示（核对数值与单位/空置按缺项策略）
		QLabel* _entryMsgLabel = nullptr;         //条件消息行（校验错误红字/历史失配黄字/E2.2 清洗提示）
		QLabel* _verdictLabel = nullptr;          //verdict 推导行（材料级 min≥1.0 三态）
		QTableWidget* _loadTable = nullptr;
		//页签③
		QTableWidget* _imageTable = nullptr;
		QLabel* _imageHint = nullptr;             //水印/缺项提示（U-02/U-04；批次 C 增试点/水印档提示）
		//页签③（二期批次 C）：半自动试点开关+槽位映射入口（08 §3.2；试点默认关闭）
		QCheckBox* _semiAutoChk = nullptr;        //半自动抓取（试点）——关闭=页签③与一期同交互
		QPushButton* _mappingBtn = nullptr;       //[映射设置…]：槽键→帧标识（D3-3 仅帧标识）
		//页签④（二期批次 C）：截图水印全局开关（FR-231/Q2-4=A；默认带水印=一期行为不变）
		QCheckBox* _watermarkOffChk = nullptr;
		//页签④
		QLineEdit* _outputPathEdit = nullptr;
		QPushButton* _browseOutputBtn = nullptr;
		QComboBox* _missingPolicyCombo = nullptr; //缺项策略三选一（E1：dash/keep/block）
		QPushButton* _precheckBtn = nullptr;
		QLabel* _precheckLabel = nullptr;
		QPlainTextEdit* _resultMessages = nullptr;
		//底部
		QProgressBar* _progressBar = nullptr;
		QLabel* _stageLabel = nullptr;
		QPushButton* _saveConfigBtn = nullptr;
		QPushButton* _revalidateBtnBottom = nullptr;
		QPushButton* _exportBtn = nullptr;
		QPushButton* _cancelExportBtn = nullptr;

		//加载态状态机（数值序与 mStrengthReportContainerProbe::Status 对齐，probe 结果可直接
		//static_cast 映射；02 §3.4 状态表）
		enum LoadState { NotLoaded, Loaded, IntegrityFailed, VersionMismatch, ValidationFailed };
		LoadState _loadState = NotLoaded;
		QString _templateName;
		QStringList _validationErrors;
		QJsonDocument _rulesDocument;
		//T1：文本槽预览行（锚点+绑定键；说明/当前值在 refreshTextSlotPreview 刷新时现算——
		//编辑区回显与 _autoAssembly 装配值随触点变化，不在解析期定格）
		struct TextSlotRow
		{
			QString anchor;   //锚点字面（anchor.match）
			QString bind;     //绑定键原文
		};
		QList<TextSlotRow> _textSlotRows;   //页签①文本槽预览行
		QList<ImageSlot> _imageSlots;    //页签③槽位清单（规则层图片槽声明驱动）
		QList<MaterialRow> _materialRows; //页签②材料表行数据（行↔配置键映射+default 标注）
		int _missingCount = 0;
		QString _lastEntryError;         //页签②最近一次录入校验错误文案（修正后清空）

		mStrengthReportProjectConfig _config;   //项目级配置（值成员：D-12 按项目隔离存取）
		mStrengthReportContainerProbe* _probe = nullptr;
		mStrengthReportEngineRunner* _runner = nullptr;
		bool _exporting = false;
		bool _viewportAvailable = false; //T-23/D-13：无激活视口→[抓取当前视口]禁用、[导入]可用
		                                 //（CB-02：构造/rebuildImageTable 时查询视口在位初始化，不依赖首信号）
		bool _revalidatePending = false; //CB-04：校验进行中收到重校验请求→完成后自动补跑
		QString _probeProjectId;         //CB-01：探测发起代币=发起时快照的项目标识（R2-04：切项目不覆写，
		                                 //供 slot_onProbeFinished 与当前项目比对丢弃陈旧结果）
		QString _probeContainerPath;     //L6-02：探测发起时快照的容器路径（待确认集会话指纹一半）
		QString _pendingConfirmSessionKey; //L6-02：待确认集容器会话指纹（容器路径+模板名）——
		                                 //仅指纹实际变更时才清零 _pendingConfirmKeys（同容器重校验不清）
	bool _buildingImageTable = false;//CB-05：页签③表重建期间抑制 itemChanged 回写
	bool _buildingCaseTables = false;//二期批次 B：页签②两表重建期间抑制 itemChanged 回写
	//二期批次 C：试点"对齐抓取"等待态（互斥重入+视口/对话框关闭中止通道——B-6①）
	bool _alignBusy = false;         //对齐抓取进行中（含就绪等待循环期间）——拒绝并发重入
	bool _alignAborted = false;      //等待循环被视口/对话框关闭中止的旗标
	QPointer<QEventLoop> _alignLoop; //活跃等待循环（非空=等待中，abortAlignWait→quit）
	//B-204（C13 裁定④）：试点抓取且未经替换确认的槽键集——缩略图"待确认"标记的数据面
	//（试点抓取成功=加入；手动抓取/导入/清除=移除；重建表按此集回显标记；导出成功汇总
	//按此集∩声明槽计数提示"试点待确认槽 N 处"，N=0 不显示）
	QSet<QString> _pendingConfirmKeys;
	//三期批次 A：Tab0 装配会话（自动值不入项目配置——12 §2.4/FR-325，仅会话态）
	mStrengthReportAutoPreview* _autoPreview = nullptr; //Tab0 主视图部件（buildUi 创建）
	AssemblyResult _autoAssembly;      //最近一次装配结果（slot_export 元信息发射注入数据源）
	bool _assemblyReady = false;       //S2 就绪旗标（装配中/项目切换会话重置=false）
	QString _autoPrefillSubject;       //页签①对象名当前预填的自动显示值（空=无预填；
	                                   //collectUiToConfig 据此守卫"自动值不落配置"）
	//三期批次 B：分步装配会话（读文件通道逐帧 IO——R3-5/O3-6 QTimer 分步防界面冻结；
	//会话随项目切换/重装配失效销毁）+automation 节提取（D3-05 规则层承载）
	AutoAssemblySession* _assemblySession = nullptr;
	QJsonObject _automationNode;       //_rulesDocument.object()["automation"]（probe 完成后提取）
	QString _autoPrefillElementType;   //页签①单元类型预填标记（自动值不落配置守卫，同 subject）
	QString _autoPrefillConstraint;    //默认句预填标记×4（constraint/load_case/load_type/calc）
	QString _autoPrefillLoadCase;
	QString _autoPrefillLoadType;
	QString _autoPrefillCalcMethod;
	//三期批次 C：全自动批量截图（开关=页签③"自动截图" config.autoCapture，Q3-3 默认关；
	//批量链控制器+逐槽结果——导出汇总披露行（第三行拼接留位）数据面）
	mStrengthReportAutoCapture* _autoCapture = nullptr;
	QList<CaptureSlotResult> _lastCaptureResults;  //最近一次批量逐槽结果（E3-6 分类聚合）
	bool _captureBatchRan = false;                 //本次一键导出是否实际进入截图段（披露行门）
	//LG-02/LG-03（UB-01/UB-02）：S3 批量截图段并入导出互斥态后的整链取消通道——
	//用户在批量段点[取消]/关闭/Esc 二次确认→置取消旗标+abortBatch，批量收尾后一键
	//导出链跳过导出段（杜绝"对话框已关闭、引擎在隐藏态续跑产出 docx"）
	bool _oneClickCancelled = false;
	//R4X-03：批量段项目代币不一致丢弃（R3X-02）与用户取消旗标组合窗标记——取消终态
	//提示行据此让位主因（跨项目丢弃）措辞，防"成功 0 槽已留用"中止文案掩盖丢弃事实
	bool _captureBatchDiscarded = false;
	};
}
