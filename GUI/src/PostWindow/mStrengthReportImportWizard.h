#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStringList>
#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTimer;

namespace StrengthReport
{
	/*
	* GUI 模板导入向导（二期批次 D，FR-251/252；08-设计方案-二期 §2.6 五步页序；
	* 09-技术路线-二期 §4.5 向导五步+C-6 步③三要素+B-8 两级定位+B-5 启动清扫+D2-4 纯磁盘 CLI）：
	* 步①选源 docx（E6 前置校验=tdr_digest.verify_input 同套口径/文案，纯 C++ 同步快检；
	*   digest 步仍为权威闸）→步②一键消化（tdr_digest.py，报告六项摘要呈现）→
	* 步③规则层编辑引导（C-6 三要素：a) D-18 类人工确认项顶部显式列出；b) XXX 等非高亮
	*   字面（keep_highlight 草稿默认）逐条确认；c) 循环章/表格槽行组声明只读预览；
	*   编辑对象=digest 产 rules.json 草稿（CB-16 剥整行 # 注释口径），字段级编辑+轻校验，
	*   硬校验归 put）→步④回填+校验（tdr_rules.py put→tdr_validate.py，条目级 bullets）→
	* 步⑤试导出（tdr_engine.py 空数据集+样例数据集两态，路径呈现+F9 提示）→
	* 步⑥定版（临时容器复制到用户选定路径——E2.7 中途取消零半成品；完成后主对话框
	*   写加载栏并自动重校验）。
	* 工具定位（B-8 与一期运行器/探针同源复用）：python=mStrengthReportEngineRunner::
	*   resolvePythonExe；tdr_tools=mStrengthReportContainerProbe::resolveToolsDir；
	*   引擎脚本=mStrengthReportEngineRunner::resolveEngineScript（tools 磁盘候选+qrc 释放
	*   两级，D2-4=向导不新增 qrc 条目）。子进程编排=probe 已验证模式（QProcess 异步+
	*   120s 看门狗+文本协议解析）。中间产物全部落 tempDir/mx_strength_report_tmp/
	*   wizard_<pid>/（全程临时目录，定版步才落用户路径）；任一步失败/取消=整目录删除；
	*   B-5：构造期按 mtime 阈值清扫陈旧 wizard_* 目录（防进程崩溃残液与 pid 复用冲突）。
	*/
	class MPOSTWINDOW_EXPORT mStrengthReportImportWizard : public QDialog
	{
		Q_OBJECT
	public:
		explicit mStrengthReportImportWizard(QWidget* parent = nullptr);
		~mStrengthReportImportWizard() override;   //析构 kill 运行中子进程+清理临时目录（零半成品，E2.7）

		//CB-03 同口径：Esc/closeEvent 走 reject()——任务进行中二次确认+kill；已定版后
		//按"完成"语义关闭（定版产物已落用户路径，主对话框照常交付）
		void reject() override;

			//步⑥定版成功后的容器路径（主对话框 exec() 返回后取用→写加载栏+自动重校验；
			//已定版后即使按取消/关闭，定版产物已在用户路径，主对话框照常应用）
			QString finalizedContainerPath() const { return _finalizedPath; }

	private:
			void buildUi();
			QWidget* buildStepSourcePage();      //步①选择源文档
			QWidget* buildStepDigestPage();      //步②一键消化
			QWidget* buildStepRulesPage();       //步③规则层编辑引导
			QWidget* buildStepValidatePage();    //步④回填与校验
			QWidget* buildStepTrialPage();       //步⑤试导出
			QWidget* buildStepFinalPage();       //步⑥定版

			//——B-5：临时目录与陈旧清扫——
			static QString wizardTempRoot();       //tempDir/mx_strength_report_tmp/wizard_<pid>
			static void sweepStaleWizardDirs();    //mtime 超 24h 的 wizard_* 目录递归删除（活跃目录不触碰）

			//——步① E6 前置校验（与 tdr_digest.verify_input 同序同文案：扩展名→读模式占用
			//探测→试解包快检；zip 快检=EOCD 签名扫描，权威试解包仍由步② digest 承担；
			//REF-01/C15：读试开口径——可读但有写锁/只读属性不阻断，经 hint 返回编辑中提示）——
			static bool precheckSourceDocx(const QString& src, QString& err, QString* hint = nullptr);

			//——子进程链（probe 同模式）：命令逐个异步执行，全部成功后回调 purpose 对应收尾——
			struct CmdSpec
			{
				QString tag;            //命令标识（digest/rules_get/rules_put/validate/trial_empty/trial_sample）
				QString program;        //可执行（python 全路径）
				QStringList args;       //参数（含脚本路径）
			};
			enum ChainPurpose { PurposeNone, PurposeDigest, PurposeRulesGet, PurposePutValidate, PurposeTrial };
			void runChain(const QVector<CmdSpec>& chain, ChainPurpose purpose);
			void startNextCmd();
			void onCmdFinished(int exitCode, QProcess::ExitStatus exitStatus);
			void onWatchdogTimeout();
			void abortChain(const QString& failText);   //链失败/超时收尾：状态呈现+按钮恢复
			void finishChainOk();                       //链全部成功→按 purpose 分发收尾

			//——步②消化报告摘要（08 §2.6 六项）——
			void presentDigestReport();
			//——步③规则层编辑引导——
			void presentRulesDraft();              //rules_edit.json → 确认表+D-18 区+只读预览
			void rebuildD18Area();                 //C-6(a)：digest 报告字面报告驱动的顶部确认区
			void rebuildLoopPreview();             //C-6(c)：循环章/表格槽行组声明只读预览
			bool applyRulesEdits(QString& err);    //确认表编辑写回 rules_edit.json（轻校验）
			//——步④/⑤结果呈现——
			void presentValidateOutcome(bool ok);
			void presentTrialOutcome(bool ok);
			//——步⑥定版——
			bool finalizeContainer(QString& err);  //占用探测+覆盖确认+复制临时容器到用户路径

			void gotoStep(int step);               //页切换+页入口动作+按钮态刷新
			//WZ-01/02：源变更/步③编辑变更→下游成功旗标级联失效（强制重跑 digest/put 起
			//后续步骤，页面绿灯与实际产物一致）
			void resetDownstreamFrom(int fromStep);
			//WZ-06：源指纹快照（步⑥入口比对 mtime/size，外部修改黄字提示）
			void captureSourceFingerprint(const QString& src);
			void setChainBusy(bool busy);          //子进程链运行中：翻页/重入禁用（步内可回退口径见各页）
			void setStepStatus(QLabel* label, bool ok, const QString& text);   //页内状态行（红/绿）
			void updateNavButtons();
			void cleanupTempDir();                 //wizard_<pid> 整目录递归删除（零半成品，E2.7）

			//——页面控件——
			QStackedWidget* _pages = nullptr;
			//步①
			QLineEdit* _sourcePathEdit = nullptr;
			QPushButton* _browseSourceBtn = nullptr;
			QLabel* _sourceStatus = nullptr;
			//步②
			QPlainTextEdit* _digestSummary = nullptr;
			QLabel* _digestStatus = nullptr;
			//步③
			QLabel* _d18Area = nullptr;            //C-6(a) D-18 类人工确认项顶部高亮区
			QTableWidget* _rulesTable = nullptr;   //候选占位确认表（行=草稿槽声明）
			QPlainTextEdit* _loopPreview = nullptr; //C-6(c) 循环章/表格槽行组只读预览
			QLabel* _rulesStatus = nullptr;
			//步④
			QPlainTextEdit* _validateReport = nullptr;
			QLabel* _validateStatus = nullptr;
			//步⑤
			QPlainTextEdit* _trialReport = nullptr;
			QLabel* _trialStatus = nullptr;
			//步⑥
			QLineEdit* _finalPathEdit = nullptr;
			QPushButton* _browseFinalBtn = nullptr;
			QPushButton* _finalizeBtn = nullptr;
			QLabel* _finalStatus = nullptr;
			//底部导航
			QLabel* _stepLabel = nullptr;
			QPushButton* _backBtn = nullptr;
			QPushButton* _nextBtn = nullptr;
			QPushButton* _cancelBtn = nullptr;

			//——状态机与子进程链——
			int _step = 0;                          //当前页 0..5
			bool _chainBusy = false;                //子进程链运行中（翻页/重入闸）
			bool _sourceOk = false;                 //步① E6 前置通过
			bool _digestOk = false;                 //步②消化成功
			bool _rulesLoaded = false;              //步③草稿取出+解析成功
			bool _validated = false;                //步④回填+校验通过
			bool _trialOk = false;                  //步⑤两态试导出成功
			bool _finalized = false;                //步⑥定版完成
			QString _finalizedPath;                 //定版产物路径（主对话框回填用）
			QString _sourceDocx;                    //步①选定源（全程只读，FR-12）
			QString _tmpRoot;                       //本向导临时目录 wizard_<pid>
			QString _containerPath;                 //临时容器（步②产出→步⑥定版源）
			QString _rulesEditPath;                 //步③编辑对象 rules_edit.json
			QJsonDocument _rulesDoc;                //步③解析后的草稿规则层（CB-16 剥注释后）
			QString _templateId;                    //digest 报告 template_id（定版默认文件名）
			QStringList _digestWarns;               //步② warn 清单（步③ D-18 区复用）
			QJsonArray _literalReport;              //digest 报告 literal_report（步③ D-18 区复用）
			QVector<CmdSpec> _chain;                //当前子进程链
			int _chainIndex = -1;
			ChainPurpose _purpose = PurposeNone;
			QString _chainStdout;                   //当前命令 stdout（协议解析）
			QString _chainStderr;                   //当前命令 stderr（拒绝原因/诊断）
			QString _failedTag;                     //链内失败命令标识（呈现定位）
			QProcess* _process = nullptr;
			QTimer* _watchdog = nullptr;
			bool _timedOut = false;
			//步③逐行编辑态（写回与轻校验用；行序=草稿 slots 数组序）
			QStringList _initialBinds;              //装载时 bind 原值
			QStringList _initialPolicies;           //装载时 missing_policy 原值（空=未显式声明）
			//WZ-06：步①precheck 时的源指纹（mtime/size）——步⑥入口复检
			qint64 _sourceMtimeMs = 0;
			qint64 _sourceSize = -1;
			bool _sourceExternallyChanged = false;  //步⑥入口检出源已变化（黄字提示一次）
	};
}
