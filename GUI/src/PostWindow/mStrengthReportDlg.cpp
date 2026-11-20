#include "mStrengthReportDlg.h"

#include "mStrengthReportAutoPreview.h"
#include "mStrengthReportCollector.h"
#include "mStrengthReportContainerProbe.h"
#include "mStrengthReportEngineRunner.h"
#include "mStrengthReportImportWizard.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QSet>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "mViewportModelManager.h"       //getCVM/VMManager（只读调用+视口状态信号）
#include "mViewportModel.h"              //getCVM 完整类型（getPostRenderWidget 调用）
#include "mxr/view/REBUILD_PostRend.h"   //REBUILD_PostRend：getPostRender/setLogoVisible（只调用）
#include "mGlobalSignals.h"              //MxFunctions::mGlobalSignals::outputMessageSig（E10 "[强度报告]"前缀）
#include "mMechGlobalSignals.h"          //MechCommon::mMechGlobalSignals::changedFrame（批次 C 切帧链
                                         //——与手动同链：云图面板 slot_changedFrame 连接先例
                                         //mPost3DResult.cpp:631、调用先例 mPostAdvancedQuery.cpp:413）
#include "mPostDataBase.h"               //三期批次 C（FR-333 建议帧号）：getCaseFrameNamesAndIDs
                                         //只读枚举（Collector::currentPostDb 同源链副本）
#include "mPostDataManager.h"            //Physical_Post 管理器→getPostDatabase（只读）
#include "mModelData.h"                  //MechModelData::mModelData::getManager
#include "mElementFunction.h"            //本轮：elementMetaCount/getElementString（单元类型候选，FR-311 同源）
#include "mPostGUIData.h"                //本轮：getGUIDataByPostRender→_cloudFrame（帧号取法=collectModelStats 同源）

using namespace MechPreData;
using namespace PostData;

namespace StrengthReport
{
	namespace
	{
		//三期批次 C：postDB 只读取数链文件内副本（Collector::currentPostDb 同源——FR-333
		//建议帧号枚举用；匿名命名空间副本惯例同 AutoPreview.cpp 批次 B）
		QPointer<PostData::mPostDataBase> currentPostDbDlg()
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr)
				return nullptr;
			const QString postname = postRend->getPostDataName();
			return getCVM->getModelData()
				->getManager<mPostDataManager>(MxFunctions::Physical_Post)
				.getPostDatabase(postname);
		}

		//S10：映射对话框四列下拉候选（云图面板同源只读通道，复用 AutoCapture.cpp:312
		//postDB 注册面取数先例；postDB 空=空候选，editable 手输兜底）
		QStringList slotCaseCandidatesDlg(PostData::mPostDataBase* postDB)
		{
			if (postDB == nullptr)
				return QStringList();
			QStringList names = postDB->getAllCaseNames();
			names.removeAll(QString());   //空名不进工况下拉（mPost3DResult.cpp:1061 同口径）
			return names;
		}
		QStringList slotVarCandidatesDlg(PostData::mPostDataBase* postDB)
		{
			if (postDB == nullptr)
				return QStringList();
			QStringList names = postDB->getAllVariableNames();
			names.prepend(QStringLiteral("SolidColor"));   //伪变量首项=云图面板同口径
			return names;
		}
		QStringList slotCompoCandidatesDlg(PostData::mPostDataBase* postDB, const QString& var)
		{
			if (postDB == nullptr || var.isEmpty())
				return QStringList();
			return postDB->getVariableCompos(var);
		}
		QStringList slotFrameCandidatesDlg(PostData::mPostDataBase* postDB, const QString& caseName)
		{
			if (postDB == nullptr || caseName.isEmpty())
				return QStringList();
			//帧候选=该工况可用帧序号序列（getCaseFrameNamesAndIDs 只读枚举，先例同文件
			// buckling 建议帧号枚举）
			std::vector<QPair<QString, int>> frames;
			postDB->getCaseFrameNamesAndIDs(caseName, frames);
			QStringList ids;
			for (const auto& f : frames)
			{
				if (f.second > 0)
					ids << QString::number(f.second);
			}
			return ids;
		}
		//——本轮：人工补录区下拉候选（内置通用词表）——
		//兜底候选用词沿用 UI-05 界面用词表基线（.dev-workflow/template-docx-report/
		//checks/s5-phase3/fix11/UI-05-界面用词表.md）与 htsy 模板 §6.1 括号占位措辞，
		//不新造术语；词表内容集中于此便于后续调整（约束/加载形式各末位含"其它"）
		const QStringList& genericConstraintDescs()
		{
			//约束形式：CAE 常见支承/连接措辞（"固支/简支/悬臂"为 UI-05 A 类术语）
			static const QStringList list
			{ QStringLiteral("两端固支"), QStringLiteral("一端固支"), QStringLiteral("简支"),
			  QStringLiteral("悬臂"), QStringLiteral("弹性支撑"), QStringLiteral("螺栓连接"),
			  QStringLiteral("焊接"), QStringLiteral("其它") };
			return list;
		}
		const QStringList& genericLoadCaseDescs()
		{
			//加载形式：静/分布/集中/压力/温度/惯性/随机振动（模板 §5 载荷工况域措辞）
			static const QStringList list
			{ QStringLiteral("静载"), QStringLiteral("均布载荷"), QStringLiteral("集中载荷"),
			  QStringLiteral("压力载荷"), QStringLiteral("温度载荷"), QStringLiteral("惯性载荷"),
			  QStringLiteral("随机振动"), QStringLiteral("其它") };
			return list;
		}
		const QStringList& genericLoadTypes()
		{
			//载荷类型：分析类别（模板 §6.1 占位+UI-05 A 类术语：静力/模态/屈曲族）
			static const QStringList list
			{ QStringLiteral("静力"), QStringLiteral("瞬态动力"), QStringLiteral("模态"),
			  QStringLiteral("谐响应"), QStringLiteral("疲劳"), QStringLiteral("热-结构耦合"),
			  QStringLiteral("其它") };
			return list;
		}
		const QStringList& genericCalcMethods()
		{
			//计算方法：求解类型措辞（"模态分析"UI-05 A 类；静力学/动力学族）
			static const QStringList list
			{ QStringLiteral("线性静力学分析"), QStringLiteral("非线性静力学分析"),
			  QStringLiteral("模态分析"), QStringLiteral("瞬态动力学分析"),
			  QStringLiteral("谐响应分析"), QStringLiteral("疲劳分析"), QStringLiteral("其它") };
			return list;
		}
		const QStringList& genericElementTypes()
		{
			//单元类型：按网格拓扑措辞（UI-05 A 类"单元类型"域；"壳单元/梁单元"同 A 类）
			static const QStringList list
			{ QStringLiteral("四面体"), QStringLiteral("六面体"), QStringLiteral("三棱柱"),
			  QStringLiteral("壳单元"), QStringLiteral("梁单元"), QStringLiteral("混合"),
			  QStringLiteral("其它") };
			return list;
		}

		//模型候选：当前后处理数据当前帧内实际出现的单元类型名（FR-311 同源链——
		//framei=guidata->_cloudFrame fallback 1，单元计数与帧无关；逐型计数>0 即入选，
		//名=getElementString 字符串（如 D3T4/QPH），与自动装配 element_type 发射值同字面）
		QStringList collectModelElementTypes()
		{
			QStringList out;
			QPointer<PostData::mPostDataBase> postDB = currentPostDbDlg();
			if (postDB == nullptr)
				return out;
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			int framei = 1;
			if (postRend != nullptr)
			{
				if (mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend))
					framei = guidata->_cloudFrame;
			}
			if (framei <= 0)
				framei = 1;
			mPostModelData* modeldata = postDB->getModelData(framei);
			if (modeldata == nullptr)
				return out;
			const int metaCount = MxFunctions::elementMetaCount();
			for (int t = 0; t < metaCount; ++t)
			{
				const MxFunctions::ElementType type = static_cast<MxFunctions::ElementType>(t);
				if (type == MxFunctions::NoneElement)
					continue;
				if (modeldata->getNumByEleType(type) > 0)
				{
					const QString name = MxFunctions::mElementFunction::getElementString(type);
					if (!name.isEmpty() && !out.contains(name))
						out.append(name);   //保序去重（并列计数全收入，报告措辞由用户选）
				}
			}
			return out;
		}

		//模型候选：对象名（报告标题 "XXX"位）——后处理数据名（=项目标识同源，D-12）+
		//模型侧部件名清单（getAllNames(Physical_Part)，材料名兜底通道同款取法）
		QStringList collectModelObjectNames()
		{
			QStringList out;
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend != nullptr)
			{
				const QString postName = postRend->getPostDataName();
				if (!postName.isEmpty())
					out.append(postName);
			}
			if (MechModelData::mModelData* modelData = getCVM->getModelData())
			{
				const QStringList partNames = modelData->getAllNames(MxFunctions::Physical_Part);
				for (const QString& name : partNames)
				{
					if (!name.isEmpty() && !out.contains(name))
						out.append(name);   //保序去重
				}
			}
			return out;
		}

		//E10：独立临时目录（数据集 JSON/规则层取出副本落此，与既有导出临时区隔离）
		QString strengthReportTempDir()
		{
			return QDir::temp().absoluteFilePath(QStringLiteral("mx_strength_report_tmp"));
		}

		//UI-01：全局 QSS（mApplicationStart.qss）将 QPushButton 宽度钉为 80px（width:80px），
		//QSS width 会钳制 sizeHint——长文案按钮文本被截断。按字体实际宽度抬高最小宽度
		//下限（+左右内边距 24px），布局遵守显式 minimumWidth，完整文本可见
		void fitBtnMinWidth(QPushButton* btn)
		{
			if (btn == nullptr)
				return;
			const int textW = btn->fontMetrics().horizontalAdvance(btn->text());
			btn->setMinimumWidth(textW + 24);
		}

		//页签③操作按钮查找（行内操作区=抓取/导入/清除三钮，行重建由表清空重建承载）
		QPushButton* imageActionBtn(QTableWidget* table, int row, int index)
		{
			QWidget* cell = table->cellWidget(row, 2);
			if (cell == nullptr)
				return nullptr;
			const QList<QPushButton*> btns = cell->findChildren<QPushButton*>();
			return index >= 0 && index < btns.size() ? btns.at(index) : nullptr;
		}

		//只读单元格（CB-05：页签③整表开了编辑触发器，仅"题注"列可编辑单元格
		//保留 ItemIsEditable，其余列一律剥该标志防误编辑）
		QTableWidgetItem* readOnlyItem(const QString& text)
		{
			QTableWidgetItem* item = new QTableWidgetItem(text);
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			return item;
		}

		//UI-04：批量截图失败原因码→用户短标签（导出汇总"（原因：槽=标签）"呈现——
		//内部英文分类码不得直出用户界面；完整中文原因见逐槽 tooltip 与输出窗口消息）
		QString captureFailLabel(const QString& code)
		{
			if (code == QStringLiteral("ViewportClosed"))
				return QStringLiteral("视口已关闭");
			if (code == QStringLiteral("RenderUnreachable"))
				return QStringLiteral("渲染不可达");
			if (code == QStringLiteral("Playing"))
				return QStringLiteral("动画播放中");
			if (code == QStringLiteral("MappingMismatch"))
				return QStringLiteral("对应不符");
			if (code == QStringLiteral("Timeout"))
				return QStringLiteral("等待超时");
			if (code == QStringLiteral("Aborted"))
				return QStringLiteral("已中止");
			if (code == QStringLiteral("GrabFailed"))
				return QStringLiteral("保存失败");
			if (code == QStringLiteral("ImageUnreadable"))
				return QStringLiteral("文件不可读");
			return code;
		}

		//——二期批次 B：页签②录入逐字段校验（08 §2.3/09 §4.5）——
		//录入值类型：非负数值（应力峰值/位移最大值/最大应力）、正数（剩余系数）、
		//正整数字符串（失稳模态阶次——数据字典 string，存储/发射均为字符串形态）
		enum EntryKind { NonNegativeNumber, PositiveNumber, PositiveIntText };

		//空文本=未录（显式 null）；非法=ok=false+reason；E2.2 不落配置由调用方承担
		bool parseEntryValue(const QString& text, EntryKind kind, QJsonValue& out, QString& reason)
		{
			out = QJsonValue(QJsonValue::Null);
			reason.clear();
			if (text.isEmpty())
				return true;
			switch (kind)
			{
			case NonNegativeNumber:
			case PositiveNumber:
			{
				bool ok = false;
				const double v = QLocale::c().toDouble(text, &ok);
				if (!ok)
				{
					reason = QStringLiteral("应为数值");
					return false;
				}
				if (kind == NonNegativeNumber && v < 0.0)
				{
					reason = QStringLiteral("应为非负数值");
					return false;
				}
				if (kind == PositiveNumber && v <= 0.0)
				{
					reason = QStringLiteral("应为正数");
					return false;
				}
				out = QJsonValue(v);
				return true;
			}
			case PositiveIntText:
			{
				static const QRegularExpression re(QStringLiteral("^[0-9]+$"));
				if (!re.match(text).hasMatch() || text.toLongLong() <= 0)
				{
					reason = QStringLiteral("应为正整数（失稳模态阶次）");
					return false;
				}
				out = QJsonValue(text);   //数据字典 string——发射字符串形态
				return true;
			}
			}
			return true;
		}

		//配置值对象→回显文本（数值去多余零、失稳阶次原文、null/缺=空）
		QString entryDisplayText(const QJsonValue& v)
		{
			if (v.isDouble())
				return QLocale::c().toString(v.toDouble(), 'g', 15);
			if (v.isString())
				return v.toString();
			return QString();
		}
	}

	void mStrengthReportDlg::showInstance(QWidget* parent)
	{
		static QPointer<mStrengthReportDlg> _dlg;
		if (_dlg.isNull())
			_dlg = new mStrengthReportDlg(parent);
		//非模态（T-23/B15/U-01=A）：不设 WindowModality——页签③"切变量↔回抓取"
		//交替流在应用级模态下物理不可达
		_dlg->show();
		_dlg->raise();
		_dlg->activateWindow();
	}

	mStrengthReportDlg::mStrengthReportDlg(QWidget* parent)
		: QDialog(parent)
	{
		setWindowTitle(QStringLiteral("强度计算报告导出"));
		setObjectName(QStringLiteral("strengthReportDlg"));
		//非模态独立对话框：不占新 dock 区、不改既有面板布局（02 §6）
		setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
		resize(760, 640);

		_probe = new mStrengthReportContainerProbe(this);
		_runner = new mStrengthReportEngineRunner(this);

		buildUi();

		connect(_probe, &mStrengthReportContainerProbe::finished,
			this, &mStrengthReportDlg::slot_onProbeFinished);
		connect(_runner, &mStrengthReportEngineRunner::stageProgress,
			this, &mStrengthReportDlg::slot_onStageProgress);
		connect(_runner, &mStrengthReportEngineRunner::finished,
			this, &mStrengthReportDlg::slot_onExportFinished);
		//T-23/D-13：视口创建/删除/激活切换→页签③抓取可用性与页签①采集区联动
		connect(mViewportModelManager::getInstance(), &mViewportModelManager::currentViewportChanged,
			this, &mStrengthReportDlg::slot_viewportChanged);
		connect(mViewportModelManager::getInstance(), &mViewportModelManager::sig_viewportRemoved,
			this, &mStrengthReportDlg::slot_viewportRemoved);

		//CB-02：单视口架构下"当前视口在位"可能永不发信号（mViewportModelManager 仅在
		//viewportID 变化时 emit currentViewportChanged）——对话框打开时直接查询视口
		//在位初始化按钮态，消除"已有视口首开[抓取]永久禁用"
		_viewportAvailable = (getCVM->getPostRenderWidget() != nullptr);

		//项目配置回显（FR-45/D-12）：按当前项目标识载入各自配置；损坏→空配置+告警（H-15）
		QString projectId = mStrengthReportCollector::currentProjectId();
		if (projectId.isEmpty())
			projectId = QStringLiteral("default");
		_config.setProjectId(projectId);
		_probeProjectId = projectId;   //CB-01/R2-04：探测发起代币初始化（此后仅 probe 发起时更新）
		QString cfgErr;
		if (!_config.load(cfgErr) && !cfgErr.isEmpty())
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(cfgErr), true);

		echoConfigToUi();

		applyLoadState(QStringLiteral("<span style='color:#C00000;'>未加载——请选择规则模板文档(.rtdoc)</span>"));
		refreshCollectedData();

		//02 §3.3 一键体验：打开对话框即自动加载上次规则模板包并回显
		if (!_config.containerPath.isEmpty())
			slot_revalidate();
	}

	void mStrengthReportDlg::echoConfigToUi()
	{
		_softwareNameEdit->setText(_config.softwareName);
		_constraintDescEdit->setText(_config.constraintDesc);
		_loadCaseDescEdit->setText(_config.loadCaseDesc);
		_loadTypeEdit->setText(_config.loadType);
		_calcMethodEdit->setText(_config.calcMethod);
		_elementTypeEdit->setText(_config.elementType);
		_projectSubjectEdit->setText(_config.projectSubject);
		if (!_config.missingPolicy.isEmpty())
		{
			const int idx = _missingPolicyCombo->findData(_config.missingPolicy);
			if (idx >= 0)
				_missingPolicyCombo->setCurrentIndex(idx);
		}
		_outputPathEdit->setText(_config.outputPath);
		_containerPathEdit->setText(_config.containerPath);
		//二期批次 C：水印/试点开关回显（blockSignals 抑制 toggled——试点开启的一次性提示
		//只在用户手动开启时弹出，配置回显/项目切换不弹）
		_watermarkOffChk->blockSignals(true);
		_watermarkOffChk->setChecked(_config.watermarkOff);
		_watermarkOffChk->blockSignals(false);
		_semiAutoChk->blockSignals(true);
		_semiAutoChk->setChecked(_config.autoCapture);   //O3-2 开关合一：autoCapture=权威
		                                                 //（load 迁移旧 semiAutoGrab=true）
		_semiAutoChk->blockSignals(false);
		_mappingBtn->setEnabled(_config.autoCapture);
		//三期批次 C：Tab0 截图槽列表随配置回显刷新（映射列随项目配置切换）
		refreshCaptureRows();
		//页签③槽图/题注随配置回显（CB-01：项目切换后旧项目截图不得显示为"已就绪"）
		rebuildImageTable();
	}

	void mStrengthReportDlg::switchProjectConfigIfNeeded()
	{
		//CB-01（D-12 违约修复）：对话框为单例且关闭=隐藏，存活整个会话；旧实现仅构造时
		//按当次项目标识载入一次，切换模型/分析文件后 A 项目的人工字段/截图路径静默串入
		//B 项目导出，且 B 期保存会把 B 数据写进 A 的 project_config.json（双向污染）。
		//此处检测项目标识变化：保存旧项目当前录入→载入新项目配置（无文件=空配置默认态）→
		//回显全部页签；加载态/槽位清单属"容器会话"，随项目切换重置（旧容器结果不跨项目显示）
		QString projectId = mStrengthReportCollector::currentProjectId();
		if (projectId.isEmpty() || projectId == _config.projectId())
			return;   //无激活视口=项目悬空：保留原配置待回切（不落 default，防误清）；未切换=无操作
	collectUiToConfig();
	QString saveErr;
	if (!_config.save(saveErr) && !saveErr.isEmpty())
		MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
			QStringLiteral("[强度报告]%1").arg(saveErr), true);
	//LG-06（UB-04）：首开对话框（无视口）期录入/容器选择落"default"暂存目录，打开结果
	//文件切到真实项目=载入空配置整体回退（容器路径清空、状态回未加载——用户前几分钟
	//操作"消失"）。小改闭合：目标项目无配置文件（首次使用）时迁移 containerPath/
	//outputPath 两个非数据性记忆——录入值仍不迁（防 A 值串 B，D-12 口径不变）；
	//迁移后下方"记忆容器路径→自动重校验"链照常接续
	const bool freshTarget = !QFileInfo(_config.configFilePath()).exists();
	const QString stagedContainer = _config.containerPath;
	const QString stagedOutput = _config.outputPath;
	_config.setProjectId(projectId);
		//R2-04：此处刻意不覆写 _probeProjectId——它是"探测发起代币"（发起时快照的项目
		//标识），切项目覆写会令 slot_onProbeFinished 的过期判据恒假（死码洞），A 项目
		//进行中的探测结果会写入 B 会话；保持发起值即可在 finished 时与当前项目比对
	QString loadErr;
	if (!_config.load(loadErr) && !loadErr.isEmpty())
		MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
			QStringLiteral("[强度报告]%1").arg(loadErr), true);
	//LG-06：首次使用目标项目回填暂存记忆（仅非数据性两项；显式空载=迁移条件成立）
	if (freshTarget)
	{
		if (_config.containerPath.isEmpty())
			_config.containerPath = stagedContainer;
		if (_config.outputPath.isEmpty())
			_config.outputPath = stagedOutput;
	}
		//容器会话重置：旧项目探测/校验结果一律不带入新项目——探测发起代币 _probeProjectId
		//保持发起时快照（R2-04：不随切换覆写），进行中的旧探测结果在 slot_onProbeFinished
		//按"代币≠当前项目"判过期丢弃并重探；本函数已先行重置加载态为未加载兜底
		_loadState = NotLoaded;
		_templateName.clear();
		_validationErrors.clear();
		_rulesDocument = QJsonDocument();
		_pendingConfirmKeys.clear();   //B-204：待确认槽键集随项目会话重置（不跨项目携带）
		//三期批次 A：Tab0 装配会话随项目重置（13 §3"CB-01 项目切换挂 Tab0 会话重置"——
		//装配缓存清空+预览复位+预填标记清除；自动值不入配置故无跨项目串值面）
		_autoAssembly = AssemblyResult();
		_assemblyReady = false;
		_autoPrefillSubject.clear();
		//三期批次 B：分步会话销毁+automation 提取清空+新预填标记清除（同会话重置纪律）
		delete _assemblySession;
		_assemblySession = nullptr;
		_automationNode = QJsonObject();
		_autoPrefillElementType.clear();
		_autoPrefillConstraint.clear();
		_autoPrefillLoadCase.clear();
		_autoPrefillLoadType.clear();
		_autoPrefillCalcMethod.clear();
		if (_autoPreview != nullptr)
			_autoPreview->resetSession();
		parseRulesDocument(_rulesDocument);
		refreshTextSlotPreview();
		echoConfigToUi();
		applyLoadState(QStringLiteral("<span style='color:#C00000;'>未加载——请选择规则模板文档(.rtdoc)</span>"));
		refreshCollectedData();
		//02 §3.3 一键体验：新项目记忆了容器路径→自动校验
		if (!_config.containerPath.isEmpty())
			slot_revalidate();
	}

	void mStrengthReportDlg::buildUi()
	{
		auto* mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(10, 8, 10, 8);
		mainLayout->setSpacing(6);

		//——顶部规则模板加载态栏（02 §3.2）——
		auto* topRow = new QHBoxLayout();
		topRow->addWidget(new QLabel(QStringLiteral("规则模板:"), this));
		_containerPathEdit = new QLineEdit(this);
		_containerPathEdit->setPlaceholderText(QStringLiteral("规则模板文档路径(.rtdoc)"));
		topRow->addWidget(_containerPathEdit, 1);
		_browseContainerBtn = new QPushButton(QStringLiteral("浏览…"), this);
		topRow->addWidget(_browseContainerBtn);
		//二期批次 D（FR-251/08 §2.6/§3.4）：向导入口——任意加载态可用（导出中禁用），
		//从 docx 模板提炼生成 .rtdoc 并回填加载栏
		_importTemplateBtn = new QPushButton(QStringLiteral("导入模板…"), this);
		topRow->addWidget(_importTemplateBtn);
		_revalidateBtnTop = new QPushButton(QStringLiteral("重新检查"), this);
		//UI-01：顶栏按钮宽度按完整文本自适应（全局 QSS 钉宽 80px 复核——"导入模板…"等
		//长文案防截断）
		fitBtnMinWidth(_browseContainerBtn);
		fitBtnMinWidth(_importTemplateBtn);
		fitBtnMinWidth(_revalidateBtnTop);
		topRow->addWidget(_revalidateBtnTop);
		mainLayout->addLayout(topRow);

		_statusLabel = new QLabel(this);
		_statusLabel->setTextFormat(Qt::RichText);
		_statusLabel->setWordWrap(true);
		mainLayout->addWidget(_statusLabel);

		//——四页签——
		_tabs = new QTabWidget(this);
		_tabs->addTab(buildTabBasic(), QStringLiteral("基础信息与文本槽"));
		_tabs->addTab(buildTabCases(), QStringLiteral("工况与表格"));
		_tabs->addTab(buildTabImages(), QStringLiteral("截图槽管理"));
		_tabs->addTab(buildTabExport(), QStringLiteral("导出"));
		//三期批次 A（FR-341/12 §3.1）：Tab0 一键导出主视图前插为默认选中页——既有四页签
		//原样保留为高级覆盖区（仅索引+1，页签控件树/状态机/CB 系列修复面零迁移——
		//13 §2.1 方案 A"insertTab 前插"字面；预览只读消双真源）
		_tabs->insertTab(0, buildTabOneClick(), QStringLiteral("一键导出"));
		_tabs->setCurrentIndex(0);
		mainLayout->addWidget(_tabs, 1);

		//——底部进度/保存/重新校验/一键导出（按任务批次口径；导出中仅[取消]可用）——
		auto* progressRow = new QHBoxLayout();
		_stageLabel = new QLabel(this);
		progressRow->addWidget(_stageLabel, 1);
		_progressBar = new QProgressBar(this);
		_progressBar->setRange(0, 100);
		_progressBar->setValue(0);
		_progressBar->setFixedWidth(220);
		progressRow->addWidget(_progressBar);
		mainLayout->addLayout(progressRow);

		auto* bottomRow = new QHBoxLayout();
		_saveConfigBtn = new QPushButton(QStringLiteral("保存为项目配置"), this);
		bottomRow->addWidget(_saveConfigBtn);
		_revalidateBtnBottom = new QPushButton(QStringLiteral("重新检查"), this);
		bottomRow->addWidget(_revalidateBtnBottom);
		bottomRow->addStretch(1);
		_cancelExportBtn = new QPushButton(QStringLiteral("取消"), this);
		_cancelExportBtn->setEnabled(false);
		bottomRow->addWidget(_cancelExportBtn);
	_exportBtn = new QPushButton(QStringLiteral("一键导出"), this);
	_exportBtn->setEnabled(false);
	//LG-04：两个同名"一键导出"按钮语义分叉披露（B-6 回归锚=底部按钮直达 slot_export
	//跳过 S3 截图段，刻意设计保留）——tooltip 注明差异，消"视觉同权按钮不同产物"陷阱
	_exportBtn->setToolTip(QStringLiteral(
		"直接执行导出（不含自动截图批量段，图片槽保持现状）；\n"
		"含自动截图的完整链请用「一键导出」页签主按钮。"));
		//UI-01：底部按钮宽度按完整文本自适应（"保存为项目配置"7 字最长，全局 QSS 钉宽
		//80px 时会截断——复核同修）
		fitBtnMinWidth(_saveConfigBtn);
		fitBtnMinWidth(_revalidateBtnBottom);
		fitBtnMinWidth(_cancelExportBtn);
		fitBtnMinWidth(_exportBtn);
		bottomRow->addWidget(_exportBtn);
		mainLayout->addLayout(bottomRow);

		connect(_browseContainerBtn, &QPushButton::clicked, this, &mStrengthReportDlg::slot_browseContainer);
		connect(_importTemplateBtn, &QPushButton::clicked, this, &mStrengthReportDlg::slot_openImportWizard);
		connect(_revalidateBtnTop, &QPushButton::clicked, this, &mStrengthReportDlg::slot_revalidate);
		connect(_revalidateBtnBottom, &QPushButton::clicked, this, &mStrengthReportDlg::slot_revalidate);
		connect(_saveConfigBtn, &QPushButton::clicked, this, &mStrengthReportDlg::slot_saveConfig);
		connect(_exportBtn, &QPushButton::clicked, this, &mStrengthReportDlg::slot_export);
		connect(_cancelExportBtn, &QPushButton::clicked, this, &mStrengthReportDlg::slot_cancelExport);
		//"查看全部"链接→条目级错误列表（FR-13 条目级定位回显）
		connect(_statusLabel, &QLabel::linkActivated, this, [this](const QString&) { slot_openErrorList(); });
		//三期批次 A：Tab0 主按钮/覆盖跳转（主按钮=装配前置→既有 slot_export 同链包装入口——
		//B-6 回归锚：对话框底部既有[一键导出]按钮与 slot_export 状态机保留不动）
		connect(_autoPreview, &mStrengthReportAutoPreview::oneClickRequested,
			this, &mStrengthReportDlg::slot_oneClickExport);
		connect(_autoPreview, &mStrengthReportAutoPreview::jumpRequested,
			this, &mStrengthReportDlg::slot_jumpToControl);
		//三期批次 C：Tab0 截图槽建议行动作（FR-333——采纳才落 slotMappings/清除回退手动/
		//编辑→页签③映射设置）+批量链控制器（abortBatch 经 abortAlignWait 漏斗接入四个
		//既有中止点——视口切换/移除/对话框关闭/Esc）
		connect(_autoPreview, &mStrengthReportAutoPreview::adoptSuggestion,
			this, &mStrengthReportDlg::slot_adoptCaptureSuggestion);
		connect(_autoPreview, &mStrengthReportAutoPreview::clearSlotMapping,
			this, &mStrengthReportDlg::slot_clearCaptureMapping);
		connect(_autoPreview, &mStrengthReportAutoPreview::editMappingsRequested,
			this, &mStrengthReportDlg::slot_editSlotFrameMap);
		_autoCapture = new mStrengthReportAutoCapture(this);
		//E3-7（清除录入→自动回填）：对象名被用户清空→重新装配回填自动值（textEdited 仅
		//用户编辑触发——预填 setText/配置回显不触发，无递归；改写为非空=录入语义不触发）。
		//LG-09：补 probe 运行闸（PA-07 同款——校验窗口内清空不得以空/旧 _rulesDocument
		//裸装配，probe 完成链 startAutoAssembly 统一触发）
		//LG-05：非空改写=录入语义→Tab0 预览对应行即时同步（值+来源"录入"）——消
		//"预览快照过时第三真源"（UB-05），预览与实际发射一致；不动双真源原则（轻量
		//单行更新，非整链重装配，预览仍只读）
		connect(_projectSubjectEdit, &QLineEdit::textEdited, this, [this](const QString& text)
			{
				if (!text.isEmpty())
					_autoPreview->updateManualEntryRow(
						QStringLiteral("manual_fields.project_subject"), text);
				if (text.isEmpty() && _assemblyReady && !_probe->isRunning())
					startAutoAssembly();
			});
		//三期批次 B：单元类型+默认句四键同款（E3-7 清除录入→自动回填/默认句重预填；
		//sentinel 摘除经 collectUiToConfig/applyAutoPrefill 守卫承担；LG-09/LG-05 同上）
		const QList<QPair<QLineEdit*, const char*>> autoRefillEdits
		{
			{ _elementTypeEdit,    "manual_fields.element_type" },
			{ _constraintDescEdit, "manual_fields.constraint_desc" },
			{ _loadCaseDescEdit,   "manual_fields.load_case_desc" },
			{ _loadTypeEdit,       "manual_fields.load_type" },
			{ _calcMethodEdit,     "manual_fields.calc_method" },
		};
		for (const auto& refillEdit : autoRefillEdits)
		{
			const QString itemKey = QLatin1String(refillEdit.second);
			connect(refillEdit.first, &QLineEdit::textEdited, this,
				[this, itemKey](const QString& text)
				{
					if (!text.isEmpty())
						_autoPreview->updateManualEntryRow(itemKey, text);
					if (text.isEmpty() && _assemblyReady && !_probe->isRunning())
						startAutoAssembly();
				});
		}
		//LG-05：软件名改写同步预览"软件名"行（model_stats.software_name——采集自动值
		//可覆盖通道，无 E3-7 重装配面，仅预览同步）。
		//R2L-04：清空≠缺项——发射走首期默认常量（Collector::defaultSoftwareName"迈曦CAE"
		//镜像锚），预览标"（默认）"来源，与 buildDataset 发射口径一致
		connect(_softwareNameEdit, &QLineEdit::textEdited, this, [this](const QString& text)
			{
				if (!text.isEmpty())
					_autoPreview->updateManualEntryRow(
						QStringLiteral("model_stats.software_name"), text);
				else
					_autoPreview->updateManualEntryRow(
						QStringLiteral("model_stats.software_name"),
						QStringLiteral("迈曦CAE"), QStringLiteral("（默认）"));
			});
	}

	QWidget* mStrengthReportDlg::buildTabBasic()
	{
		auto* page = new QWidget(this);
		auto* layout = new QVBoxLayout(page);
		layout->setContentsMargins(6, 6, 6, 6);

		//自动采集只读区（02 §3.2 页签①：软件名称可录入覆盖——D-24①；节点/单元数只读）
		auto* autoGroup = new QWidget(page);
		auto* autoForm = new QGridLayout(autoGroup);
		autoForm->setContentsMargins(0, 0, 0, 0);
		autoForm->addWidget(new QLabel(QStringLiteral("软件名称:"), autoGroup), 0, 0);
		_softwareNameEdit = new QLineEdit(autoGroup);
		_softwareNameEdit->setPlaceholderText(QStringLiteral("自动填充，可在此修改"));
		autoForm->addWidget(_softwareNameEdit, 0, 1);
		autoForm->addWidget(new QLabel(QStringLiteral("节点数:"), autoGroup), 1, 0);
		_nodeCountEdit = new QLineEdit(autoGroup);
		_nodeCountEdit->setReadOnly(true);
		autoForm->addWidget(_nodeCountEdit, 1, 1);
		autoForm->addWidget(new QLabel(QStringLiteral("单元数:"), autoGroup), 2, 0);
		_elementCountEdit = new QLineEdit(autoGroup);
		_elementCountEdit->setReadOnly(true);
		autoForm->addWidget(_elementCountEdit, 2, 1);
		autoForm->setColumnStretch(1, 1);
		layout->addWidget(autoGroup);

		_modelSwitchHint = new QLabel(
			QStringLiteral("<span style='color:#B8860B;'>模型已切换，请重新核对自动采集区</span>"), page);
		_modelSwitchHint->hide();
		layout->addWidget(_modelSwitchHint);

		//人工补录区（FR-45；单元类型 U-06 补录不得标缺；对象名 D-24②）
		//本轮改造：6 字段 QLineEdit→可编辑 QComboBox——候选=内置通用词表+模型数据
		//（取不到模型候选的字段仅通用词表），手输兜底保留（editable 天然支持"当前已存
		//值不在候选中也正常显示"，存量项目配置字符串零迁移）。上方 *_Edit 成员改持各
		//下拉内嵌 lineEdit()：collectUiToConfig/echoConfigToUi/校验/textEdited 信号链
		//全部按 QLineEdit 语义零改动（textEdited 仅用户编辑触发，程序回填不触发=E3-7 口径不变）
		auto* manualGroup = new QWidget(page);
		auto* manualForm = new QGridLayout(manualGroup);
		manualForm->setContentsMargins(0, 0, 0, 0);
		const QList<QString> manualLabels
		{
			QStringLiteral("约束形式描述:"),
			QStringLiteral("加载形式描述:"),
			QStringLiteral("载荷类型:"),
			QStringLiteral("计算方法:"),
			QStringLiteral("单元类型(必填):"),
			QStringLiteral("对象名(报告标题):")
		};
		QLineEdit** targets[] =
		{ &_constraintDescEdit, &_loadCaseDescEdit, &_loadTypeEdit,
		  &_calcMethodEdit, &_elementTypeEdit, &_projectSubjectEdit };
		_manualCombos.clear();
		for (int i = 0; i < manualLabels.size(); ++i)
		{
			manualForm->addWidget(new QLabel(manualLabels.at(i), manualGroup), i, 0);
			auto* combo = new QComboBox(manualGroup);
			combo->setEditable(true);   //手输兜底保留（默认 insertPolicy 不限制，按需）
			*targets[i] = combo->lineEdit();
			_manualCombos.append(combo);
			manualForm->addWidget(combo, i, 1);
		}
		//候选首次注入（通用词表置底；模型候选留待 refreshManualCandidates 数据触点重填——
		//构造末尾 refreshCollectedData 即首调）
		refreshManualCandidates();
		manualForm->setColumnStretch(1, 1);
		layout->addWidget(manualGroup);

		//文本槽绑定值预览（槽位清单来自规则层声明，T-08）
		//T1：预览由纯文本改表格——说明列=字典人话+当前值（绑定键移行级 tooltip）；
		//QTableWidget 自身可滚，两列均 Stretch 兜底窄窗（最小改动面，勿大改布局）
		layout->addWidget(new QLabel(QStringLiteral("文本槽绑定预览（槽位由规则模板声明驱动）:"), page));
		_textSlotPreview = new QTableWidget(0, 2, page);
		_textSlotPreview->setHorizontalHeaderLabels(QStringList
		{ QStringLiteral("槽位（锚点文字）"), QStringLiteral("绑定内容（说明与当前值）") });
		_textSlotPreview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		_textSlotPreview->verticalHeader()->setVisible(false);
		_textSlotPreview->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_textSlotPreview->setSelectionBehavior(QAbstractItemView::SelectRows);
		layout->addWidget(_textSlotPreview, 1);
		return page;
	}

	QWidget* mStrengthReportDlg::buildTabCases()
	{
		auto* page = new QWidget(this);
		auto* layout = new QVBoxLayout(page);
		layout->setContentsMargins(6, 6, 6, 6);

		//工况结果表（08 §3.1 二期录入面 FR-241，Q2-3=A）：行=自动采集工况（只读），
		//列=results 四键录入；空置=导出按缺项策略标缺并告警（C7/D-4 口径不变）
		layout->addWidget(new QLabel(QStringLiteral("工况结果表（逐工况录入；空置=导出时按缺项策略留空）:"), page));
		_caseResultTable = new QTableWidget(0, 5, page);
		_caseResultTable->setHorizontalHeaderLabels(QStringList
		{ QStringLiteral("工况"), QStringLiteral("应力峰值"), QStringLiteral("位移最大值"),
		  QStringLiteral("失稳模态阶次"), QStringLiteral("剩余系数") });
		_caseResultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		_caseResultTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
			| QAbstractItemView::AnyKeyPressed);
		layout->addWidget(_caseResultTable, 2);

		//固定录入提示（08 §2.3 C7/D-4：录入值=用户显式负责的数据）
		_entryNoticeLabel = new QLabel(
			QStringLiteral("<span style='color:#B8860B;'>⚠ 录入值将直接写入报告，请核对数值与单位；"
				"空置键按缺项策略留空并告警（应力峰值/位移最大值=非负数值，失稳模态阶次=正整数，"
				"剩余系数=正数）</span>"), page);
		_entryNoticeLabel->setWordWrap(true);
		layout->addWidget(_entryNoticeLabel);
		//条件消息行（校验错误红字/历史录入失配黄字/E2.2 清洗提示——updateEntryMsgLabel 统一刷新）
		_entryMsgLabel = new QLabel(page);
		_entryMsgLabel->setWordWrap(true);
		_entryMsgLabel->hide();
		layout->addWidget(_entryMsgLabel);

		//材料表（08 §3.1/FR-201）：行=材料名自动采集（只读+来源列标注），列=最大应力/
		//剩余系数录入，与表8 行组一一对应；"default"占位行黄字标注（C-4①）
		layout->addWidget(new QLabel(QStringLiteral("材料表（材料名自动采集，与表8 行组一一对应）:"), page));
		_materialTable = new QTableWidget(0, 4, page);
		_materialTable->setHorizontalHeaderLabels(QStringList
		{ QStringLiteral("材料名(自动)"), QStringLiteral("来源"),
		  QStringLiteral("最大应力(录入)"), QStringLiteral("剩余系数(录入)") });
		_materialTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		_materialTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
			| QAbstractItemView::AnyKeyPressed);
		layout->addWidget(_materialTable, 2);

		//verdict 推导行（08 §3.1/D2-2 材料级 min 口径）：实时随材料表 reserve_factor 录入刷新
		_verdictLabel = new QLabel(page);
		_verdictLabel->setWordWrap(true);
		layout->addWidget(_verdictLabel);

		//载荷表预览（一期保留，T-09 列集）
		layout->addWidget(new QLabel(QStringLiteral("载荷表预览（列: 工况/端面载荷/外压(MPa)/温度）:"), page));
		_loadTable = new QTableWidget(0, 4, page);
		_loadTable->setHorizontalHeaderLabels(QStringList
		{ QStringLiteral("工况"), QStringLiteral("端面载荷"), QStringLiteral("外压(MPa)"), QStringLiteral("温度") });
		_loadTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		_loadTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		layout->addWidget(_loadTable, 1);

		//录入回写配置（E2.2：逐字段校验——非法红边+文案不落配置；重建期间由
		//_buildingCaseTables 抑制，与页签③ CB-05 同模式）
		connect(_caseResultTable, &QTableWidget::itemChanged, this,
			[this](QTableWidgetItem* item) { onCaseResultItemChanged(item); });
		connect(_materialTable, &QTableWidget::itemChanged, this,
			[this](QTableWidgetItem* item) { onMaterialValueItemChanged(item); });
		return page;
	}

	QWidget* mStrengthReportDlg::buildTabImages()
	{
		auto* page = new QWidget(this);
		auto* layout = new QVBoxLayout(page);
		layout->setContentsMargins(6, 6, 6, 6);
		//——三期批次 C：自动截图总开关行（12 §2.5/O3-2 开关合一：原"半自动抓取（试点）"
		//文案升级"自动截图"——行为语义变化点登记；开关语义合一到 config.autoCapture，
		//Q3-3 默认关=一键导出不含截图批量且页签③[抓取]=一期手动链）——
		auto* pilotRow = new QHBoxLayout();
		_semiAutoChk = new QCheckBox(QStringLiteral("自动截图"), page);
		_semiAutoChk->setToolTip(QStringLiteral(
			"开启后一键导出将按「对应设置」逐槽自动截图（对应项：工况/帧/变量/分量），"
				"单槽失败留空走缺项策略不阻塞导出；批量完成后请逐槽核对缩略图。\n"
				"页签③[抓取]同步升级为\"对齐抓取\"（按对应设置的帧号切帧后抓取）。\n"
				"默认关闭：一键导出不含截图批量，[抓取]为手动抓取。"));
		pilotRow->addWidget(_semiAutoChk);
		_mappingBtn = new QPushButton(QStringLiteral("对应设置…"), page);
		_mappingBtn->setToolTip(QStringLiteral(
			"槽位对应表（槽→工况/帧/变量/分量；留空项=仅按帧应用；"
				"SolidColor=网格图伪变量槽）。旧版仅帧号配置自动兼容读取。"));
		_mappingBtn->setEnabled(false);   //试点关闭=按钮禁用（08 §3.2 开关关闭回归锚点）
		fitBtnMinWidth(_mappingBtn);   //D-03/P-06：L5 零余量 80/80（qss 钉宽 80 恰等文案 5 全角），
		                               //与孪生钮 _captureEditBtn（AutoPreview fit=165）口径拉齐
		pilotRow->addWidget(_mappingBtn);
		pilotRow->addStretch(1);
		layout->addLayout(pilotRow);
		//槽位清单由规则层图片槽声明驱动、UI 不写死槽数（T-08/D-10）
		//CB-05：补题注录入列（A-10 题注录入面）——槽存在 image.<key>.caption 绑定时该列可编辑，
		//编辑即写入配置并随项目配置持久化；无 caption 绑定的槽该列只读"-"
		//补丁4（方案 a）：撤"定位方式/绑定键"两列（规则层技术标识对终端用户低价值），
		//信息移行级 tooltip（rebuildImageTable 行构建处拼装）；4 列=槽位/截图/操作/题注
		_imageTable = new QTableWidget(0, 4, page);
		_imageTable->setHorizontalHeaderLabels(QStringList
		{ QStringLiteral("槽位"), QStringLiteral("截图"), QStringLiteral("操作"), QStringLiteral("题注") });
		_imageTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		//补丁4：撤列后截图列(Stretch)自然获得宽裕空间（≥200px@默认窗宽），不再钉宽；
		//保留列沿用原压缩值（WO-4/P-05 压缩口径不变）
		_imageTable->setColumnWidth(0, 96);    //槽位
		_imageTable->setColumnWidth(2, 190);   //操作（行内三钮，不动）
		_imageTable->setColumnWidth(3, 96);    //题注
		_imageTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
			| QAbstractItemView::AnyKeyPressed);
		_imageTable->setIconSize(QSize(56, 56));
		layout->addWidget(_imageTable, 1);
		//CB-05：题注录入回写配置（仅第 3 列；表重建期间由 _buildingImageTable 抑制）
		connect(_imageTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item)
			{
				if (item == nullptr || _buildingImageTable || item->column() != 3)
					return;
				const int row = item->row();
				if (row < 0 || row >= _imageSlots.size())
					return;
				//PB-05 同口径：题注纯空白串归空（发射经 CB-05 空值→null 走缺项策略，
				//防空白串作为绑定值清空模板既有题注文字）
				_config.imageCaptions.insert(_imageSlots.at(row).key, item->text().trimmed());
			});
		_imageHint = new QLabel(
			QStringLiteral("提示：抓取=当前视口屏拷（默认带产品 logo 水印，与显示一致）；"
				"页签④勾选\"无水印截图\"后新抓取截图在截图瞬间隐藏水印、抓取完成即恢复（瞬间视口水印短暂消失）；"
				"逐槽水印档随槽记录（缩略图 tooltip 可查）；变量对齐请先在主窗口手动切换（对应设置仅支持帧号）；"
				"截图存报告槽位目录 <项目标识>/<槽名>/（按项目隔离）；"
				"无激活视口时[抓取]禁用、[导入]可用；允许缺项的槽（如失稳图）可走缺项策略或[导入]外部图片。"), page);
		_imageHint->setWordWrap(true);
		layout->addWidget(_imageHint);
		//试点开关/映射入口（批次 C）：toggled 在配置回显期被 blockSignals 抑制
		connect(_semiAutoChk, &QCheckBox::toggled, this, &mStrengthReportDlg::slot_semiAutoToggled);
		connect(_mappingBtn, &QPushButton::clicked, this, &mStrengthReportDlg::slot_editSlotFrameMap);
		return page;
	}

	QWidget* mStrengthReportDlg::buildTabExport()
	{
		auto* page = new QWidget(this);
		auto* layout = new QVBoxLayout(page);
		layout->setContentsMargins(6, 6, 6, 6);

		auto* outRow = new QHBoxLayout();
		outRow->addWidget(new QLabel(QStringLiteral("输出路径:"), page));
		_outputPathEdit = new QLineEdit(page);
		_outputPathEdit->setPlaceholderText(QStringLiteral("留空=导出时选择（记忆上次输出路径）"));
		outRow->addWidget(_outputPathEdit, 1);
		_browseOutputBtn = new QPushButton(QStringLiteral("浏览…"), page);
		outRow->addWidget(_browseOutputBtn);
		layout->addLayout(outRow);
		connect(_browseOutputBtn, &QPushButton::clicked, this, [this]()
			{
				//输出路径选择（docx 过滤；渲染器先例：后缀补齐由导出前置统一兜底）
				const QString dir = QCoreApplication::applicationDirPath()
					+ QStringLiteral("/Report/强度报告导出");
				QString file = QFileDialog::getSaveFileName(this,
					QStringLiteral("选择输出路径"), dir,
					QStringLiteral("Word 文档 (*.docx)"));
				if (!file.isEmpty())
					_outputPathEdit->setText(file);
			});

		auto* policyRow = new QHBoxLayout();
		policyRow->addWidget(new QLabel(QStringLiteral("缺项策略:"), page));
		_missingPolicyCombo = new QComboBox(page);
		//E1 三选一（02 §3.2 页签④；默认留"-"并告警）。二期 FR-211/Q2-7=A：三选一升级为
		//真覆盖——值随数据集顶层 missing_policy_override 发射（批次 A 引擎统一解析序：
		//覆盖键＞槽级 missing_policy＞L5 全局默认）；阻断导出=预检拒绝在 GUI 侧、
		//发射 block_export 为导出中新缺项的引擎兜底（08 §2.2 映射表）
		_missingPolicyCombo->addItem(QStringLiteral("留\"-\"并告警"), QStringLiteral("dash_and_warn"));
		_missingPolicyCombo->addItem(QStringLiteral("保留模板高亮原文"), QStringLiteral("keep_highlight"));
		_missingPolicyCombo->addItem(QStringLiteral("阻断导出"), QStringLiteral("block_export"));
		_missingPolicyCombo->setToolTip(QStringLiteral(
			"此设置优先于模板自带的缺项策略（数据集 missing_policy_override，导出告警注明）：\n"
			"留\"-\"并告警=全部缺值槽标\"-\"并汇总告警（含模板声明保留原文的槽）；\n"
			"保留模板原文=缺值槽保留模板原文并告警；\n"
			"阻断导出=预检存在缺项时拒绝启动导出，导出中新出现缺项时引擎拒绝产出"));
		policyRow->addWidget(_missingPolicyCombo);
		_precheckBtn = new QPushButton(QStringLiteral("缺项预检"), page);
		policyRow->addWidget(_precheckBtn);
		_precheckLabel = new QLabel(page);
		policyRow->addWidget(_precheckLabel, 1);
		layout->addLayout(policyRow);
		connect(_precheckBtn, &QPushButton::clicked, this, &mStrengthReportDlg::slot_missingPrecheck);

		//——二期批次 C：截图水印全局开关（FR-231/Q2-4=A，08 §2.4/§3.3）——
		//默认带水印（与一期行为逐字节一致）；勾选=新功能抓取链去水印（屏拷通道+logo 翻转，
		//09 §0.2）；[导入]外部图与已抓截图不受影响；开关即时生效、全局档不逐槽
		_watermarkOffChk = new QCheckBox(QStringLiteral(
			"无水印截图（勾选后新抓取的截图在截图瞬间隐藏产品水印，抓取完成即恢复）"), page);
		_watermarkOffChk->setToolTip(QStringLiteral(
			"仅作用于本功能页签③的[抓取]/[对齐抓取]动作；[导入]外部图与已抓截图不受影响；\n"
			"抓取瞬间主视口水印短暂消失（毫秒级）；逐槽水印档随槽记录（页签③缩略图 tooltip 可查）"));
		layout->addWidget(_watermarkOffChk);
		//即时生效：抓取流程直读控件当前态，此处同步供配置持久化（toggled 在配置回显期被抑制）
		connect(_watermarkOffChk, &QCheckBox::toggled, this, [this](bool on)
			{
				_config.watermarkOff = on;
			});

		layout->addWidget(new QLabel(QStringLiteral("结果消息:"), page));
		_resultMessages = new QPlainTextEdit(page);
		_resultMessages->setReadOnly(true);
		layout->addWidget(_resultMessages, 1);
		return page;
	}

	QWidget* mStrengthReportDlg::buildTabOneClick()
	{
		//三期批次 A（FR-341/342/343，13 §3 批次 A）：主视图部件——预览一览（值/来源/置信
		//三列只读+汇总行）+签栏说明行（FR-303）+高级设置折叠区入口+自动截图占位（批次 C）
		//+主按钮；装配引擎独立成类（AutoAssembler——UI 与取值分离可镜像自测）。
		//页签①七项预填子集=对象名（约束/载荷/计算方法默认句=批次 B；单元类型=批次 B；
		//软件名既有默认通道零变化），见 applyAutoPrefill
		_autoPreview = new mStrengthReportAutoPreview(this);
		return _autoPreview;
	}

	//——三期批次 A：元信息全自动装配（FR-301/302/303+325 骨架；13 §3 批次 A）——
	//三期批次 B 改造（13 §3 批次 B/R3-5/O3-6）：装配主体转 AutoAssemblySession 分步形态——
	//读文件通道（T3-1 只读出参重载）单帧读取含秒级 IO，QTimer 逐帧推进保持 UI 响应
	//（不引工作线程——postDB/mMxdbThread 调用契约均在 GUI 线先例内）
	void mStrengthReportDlg::startAutoAssembly()
	{
		if (_autoPreview == nullptr || _exporting)
			return;
		//R3-13：装配触发挂 probe 完成（Loaded）之后/导出中禁装配——probe 运行中不装配由
		//slot_onProbeFinished 时序保证；装配会话复位后进入 S1
		_assemblyReady = false;
		//批次 B：旧分步会话作废（重入/重触发安全——步进回调以 _assemblySession 非空为门）
		delete _assemblySession;
		_assemblySession = nullptr;
		updateAutoPreviewUi();
		_autoPreview->beginAssembly();
		//13 O3-6/R3-5：装配=只读数据通道——QTimer 单拍脱开触发栈（S1 异步骨架）
		QTimer::singleShot(0, this, [this]() { runAutoAssembly(); });
	}

	void mStrengthReportDlg::runAutoAssembly()
	{
		if (_autoPreview == nullptr || _exporting)
			return;
		//批次 B：automation 节提取（D3-05 改判=规则层承载；随 probe 的 _rulesDocument
		//一并到手，零新子进程——13 §0.2）。CB-01 项目切换已清 _rulesDocument（空=无节）
		_automationNode = _rulesDocument.isObject()
			? _rulesDocument.object().value(QStringLiteral("automation")).toObject()
			: QJsonObject();
		StrengthReport::AssemblyInput input;
		input.config = &_config;
		input.automation = !_automationNode.isEmpty() ? &_automationNode : nullptr;
		input.rulesDocument = &_rulesDocument;
		delete _assemblySession;
		_assemblySession = new AutoAssemblySession(input);
		QTimer::singleShot(0, this, [this]() { runAutoAssemblyStep(); });
	}

	void mStrengthReportDlg::runAutoAssemblyStep()
	{
		if (_assemblySession == nullptr || _autoPreview == nullptr || _exporting)
			return;   //导出中/会话已失效：步进链终止（重触发由 startAutoAssembly 重建）
		_assemblySession->step();
		//逐项刷新（13 §3 批次 A 骨架"预览逐项刷新"——步间回 S1 文案，完成态由收口置 S2）
		_autoPreview->applyResult(_assemblySession->result());
		if (!_assemblySession->finished())
		{
			_autoPreview->beginAssembly();
			QTimer::singleShot(0, this, [this]() { runAutoAssemblyStep(); });
			return;
		}
		_autoAssembly = _assemblySession->result();
		delete _assemblySession;
		_assemblySession = nullptr;
		_assemblyReady = true;
		applyAutoPrefill();                //页签①预填（对象名+单元类型+默认句四键，仅 UI 内存态）
		_autoPreview->applyResult(_autoAssembly);   //S2：预览三列+汇总行刷新
		rebuildCaseEntryTables();          //批次 B：页签②自动值预填+来源标注（_buildingCaseTables 抑制）
		updateVerdictLabel();              //批次 B：verdict 推导行随自动值刷新（FR-326）
		refreshCaptureRows();              //批次 C：Tab0 截图槽建议列随装配副产品刷新（FR-333）
		refreshTextSlotPreview();          //T1：文本槽预览当前值随装配刷新（case.results.* 触点）
		updateAutoPreviewUi();
	}

	void mStrengthReportDlg::applyAutoPrefill()
	{
		//13 §3 批次 A：装配完成→默认值写入页签①（仅 UI 内存态+预览表，不落配置——覆盖后
		//collectUiToConfig 才落盘）。批次 A 预填子集=对象名（其余页签①项：软件名既有默认
		//通道零变化/默认句三键+单元类型=批次 B）。
		//E3-5：显示侧 sanitizeProjectId 净化，数据集发射原始名（引擎 E8 转义兜底）
		const AutoValue item = _autoAssembly.items.value(
			QStringLiteral("manual_fields.project_subject"));
		const QString display = item.value.isString()
			? mStrengthReportProjectConfig::sanitizeProjectId(item.value.toString())
			: QString();
		const QString now = _projectSubjectEdit->text();
		if (item.source == QStringLiteral("录入"))
			return;   //录入来源不预填不触碰（页签①已按配置回显——优先级录入＞自动，E3-7）
		if (display.isEmpty())
		{
			//退化（视口关闭/计算文件名不可得）：自动值撤回——清预填回缺项预警口径；录入值不触碰
			if (!_autoPrefillSubject.isEmpty())
			{
				_projectSubjectEdit->setText(QString());
				_autoPrefillSubject.clear();
			}
			return;
		}
		if (now.isEmpty() || now == _autoPrefillSubject)
		{
			//空置或仍是上次自动值→预填/刷新自动值（用户无感刷新——模型切换重装配链）
			_projectSubjectEdit->setText(display);
			_autoPrefillSubject = display;
		}
		else if (!_autoPrefillSubject.isEmpty())
		{
			//用户已改写预填值=录入语义（优先级录入＞自动，E3-7）——不触碰并摘除自动标记
			_autoPrefillSubject.clear();
		}

		//三期批次 B（FR-311/312/313）：单元类型+默认句四键预填（同批次 A 纪律：仅 UI 内存态
		//不落配置；录入/默认句来源判定经 item.source——录入来源不触碰；退化=预填撤回回
		//缺项预警口径；sentinel 守卫与对象名同构）
		struct PrefillSpec
		{
			QLineEdit* edit;
			QString* sentinel;
			const char* itemKey;
		};
		const QVector<PrefillSpec> specs
		{
			{ _elementTypeEdit,    &_autoPrefillElementType, "manual_fields.element_type" },
			{ _constraintDescEdit, &_autoPrefillConstraint,  "manual_fields.constraint_desc" },
			{ _loadCaseDescEdit,   &_autoPrefillLoadCase,    "manual_fields.load_case_desc" },
			{ _loadTypeEdit,       &_autoPrefillLoadType,    "manual_fields.load_type" },
			{ _calcMethodEdit,     &_autoPrefillCalcMethod,  "manual_fields.calc_method" },
		};
		for (const PrefillSpec& spec : specs)
		{
			const AutoValue item = _autoAssembly.items.value(QLatin1String(spec.itemKey));
			if (item.source == QStringLiteral("录入"))
			{
				spec.sentinel->clear();   //录入来源不预填不触碰（页签①已按配置回显——E3-7）
				continue;
			}
			const QString val = item.value.isString() ? item.value.toString() : QString();
			if (val.isEmpty())
			{
				//退化（无自动值/无默认句配置）：自动值撤回——清预填回缺项预警口径
				if (!spec.sentinel->isEmpty() && spec.edit->text() == *spec.sentinel)
					spec.edit->setText(QString());
				spec.sentinel->clear();
				continue;
			}
			const QString now = spec.edit->text();
			if (now.isEmpty() || now == *spec.sentinel)
			{
				//空置或仍是上次自动值→预填/刷新自动值（用户无感刷新——模型切换重装配链）
				spec.edit->setText(val);
				*spec.sentinel = val;
			}
			else if (!spec.sentinel->isEmpty())
			{
				//用户已改写预填值=录入语义（优先级录入＞自动，E3-7）——摘除自动标记
				spec.sentinel->clear();
			}
		}
	}

	void mStrengthReportDlg::updateAutoPreviewUi()
	{
		//R3P-01：底部[一键导出]使能与 Tab0 主按钮共用同一判据且本函数为唯一刷新点
		//（applyLoadState/setExporting/装配收口/校验发起与排队全部汇聚至此）——装配
		//完成瞬间（runAutoAssemblyStep 收口）底部钮随 ready=true 复使能，消除 PA-02
		//三闸互斥引入的"就绪态灰死"回归（B-6 保留的既有导出入口）。判据=Loaded+
		//非导出中+非校验中（CB-04）+装配就绪（PA-02）
		_exportBtn->setEnabled(_loadState == Loaded && !_exporting
			&& !_probe->isRunning() && _assemblyReady);
		//Tab0 主按钮使能=既有加载态/probe/导出旗标（正交）+装配就绪（S2）。
		//视口无数据不禁用（12 §3.1：全部键 null 可导出=缺项策略语义）
		if (_autoPreview == nullptr)
			return;
		_autoPreview->setExportEnabled(!_exporting && _loadState == Loaded
			&& !_probe->isRunning() && _assemblyReady);
	}

	void mStrengthReportDlg::parseRulesDocument(const QJsonDocument& doc)
	{
		_textSlotRows.clear();
		_imageSlots.clear();
		if (!doc.isObject())
			return;
		const QJsonObject rules = doc.object();
		//注意：局部变量不可名 slots——Qt 的 slots 宏（Q_SLOTS）会替换标识符致编译错误
		const QJsonArray slotArray = rules.value(QStringLiteral("slots")).toArray();
		for (const QJsonValue& v : slotArray)
		{
			const QJsonObject slot = v.toObject();
			const QString kind = slot.value(QStringLiteral("kind")).toString();
			const QJsonObject anchor = slot.value(QStringLiteral("anchor")).toObject();
			const QString bind = slot.value(QStringLiteral("bind")).toString();
			if (kind == QStringLiteral("text"))
			{
				//T1：解析期只存锚点+绑定键（展示说明/当前值随 refreshTextSlotPreview
				//刷新现算——编辑区回显与 _autoAssembly 装配值随触点变化，解析期不定格）。
				//CB-15：manual_fields 键空值时行首标 ⚠ 语义保留（02 §3.4 缺项槽位在①③页标 ⚠）
				TextSlotRow rowInfo;
				rowInfo.anchor = anchor.value(QStringLiteral("match")).toString();
				rowInfo.bind = bind;
				_textSlotRows.append(rowInfo);
			}
			else if (kind == QStringLiteral("image"))
			{
				//CB-05：bind 形态与引擎/规则校验器一致——image.<key>.(path|caption)，
				//key 字符集 [A-Za-z0-9_]+（tdrrules.py 形态校验）。caption 形态是同一 key
				//图片槽的题注绑定（引擎 parts[1]=key 按 key 查 images[] 取字段），不另立槽行；
				//非该两形态退化为整 bind 展示（引擎按缺项处理，仅展示层兼容）
				QString key = bind;
				bool captionBind = false;
				static const QRegularExpression s_imgBind(
					QStringLiteral("^image\\.([A-Za-z0-9_]+)\\.(path|caption)$"));
				const QRegularExpressionMatch m = s_imgBind.match(bind);
				if (m.hasMatch())
				{
					key = m.captured(1);
					captionBind = (m.captured(2) == QStringLiteral("caption"));
				}
				QString anchorDesc;
				if (anchor.contains(QStringLiteral("match")))
					anchorDesc = QStringLiteral("按段落文本定位");
				else if (anchor.contains(QStringLiteral("cell")))
					anchorDesc = QStringLiteral("表格坐标 ") + anchor.value(QStringLiteral("cell")).toString();
				else if (anchor.contains(QStringLiteral("block_paragraph")) || anchor.contains(QStringLiteral("ref")))
					anchorDesc = QStringLiteral("空段落坐标");
				else
					anchorDesc = QStringLiteral("模板声明位置");
				//同 key 合并（先 caption 后 path 声明/先 path 后 caption 声明均归并为一行；
				//path 形态为主绑定展示，caption 形态置可录入题注标记）
				ImageSlot* existing = nullptr;
				for (ImageSlot& s : _imageSlots)
				{
					if (s.key == key)
					{
						existing = &s;
						break;
					}
				}
				if (existing == nullptr)
				{
					ImageSlot info;
					info.key = key;
					info.anchorDesc = anchorDesc;
					info.bind = bind;
					info.caption = anchor.value(QStringLiteral("caption_cell")).toString();
					info.captionBind = captionBind;
					_imageSlots.append(info);
				}
				else if (captionBind)
				{
					existing->captionBind = true;
				}
				else
				{
					existing->anchorDesc = anchorDesc;
					existing->bind = bind;
				}
			}
		}
	}

	//T1：绑定键当前值（复用既有采集/装配取值链，只读不改数据面）：
	//manual_fields.*=页签①编辑区当前值；model_stats.*=自动采集区回显值；
	//case.results.*=_autoAssembly 逐工况装配副产品（取不到→空串，展示层标缺）；
	//其余键（case.load_type 等）当前值取数成本高→空串（展示层标"见导出"）
	QString mStrengthReportDlg::textSlotCurrentValue(const QString& bind) const
	{
		const QString leaf = bind.section(QLatin1Char('.'), -1);
		if (bind.startsWith(QStringLiteral("manual_fields.")))
		{
			if (leaf == QStringLiteral("element_type"))
				return _elementTypeEdit->text();
			if (leaf == QStringLiteral("project_subject"))
				return _projectSubjectEdit->text();
			if (leaf == QStringLiteral("constraint_desc"))
				return _constraintDescEdit->text();
			if (leaf == QStringLiteral("load_case_desc"))
				return _loadCaseDescEdit->text();
			if (leaf == QStringLiteral("load_type"))
				return _loadTypeEdit->text();
			if (leaf == QStringLiteral("calc_method"))
				return _calcMethodEdit->text();
			return QString();
		}
		if (bind.startsWith(QStringLiteral("model_stats.")))
		{
			//自动采集区回显（"-"=不可用→按无值处理，A-08 不静默填错）
			QString v;
			if (leaf == QStringLiteral("software_name"))
				v = _softwareNameEdit->text();
			else if (leaf == QStringLiteral("node_count"))
				v = _nodeCountEdit->text();
			else if (leaf == QStringLiteral("element_count"))
				v = _elementCountEdit->text();
			return (v == QStringLiteral("-")) ? QString() : v;
		}
		if (bind.startsWith(QStringLiteral("case.results.")))
		{
			//逐工况装配值聚合（多工况拼接展示，超 3 条截断并注明"见导出"）
			QStringList vals;
			for (const AssemblyResult::CaseScan& scan : _autoAssembly.cases)
			{
				AutoValue item;
				if (leaf == QStringLiteral("stress_peak"))
					item = scan.stressPeak;
				else if (leaf == QStringLiteral("displacement_max"))
					item = scan.dispMax;
				else if (leaf == QStringLiteral("buckling_mode"))
					item = scan.buckling;
				else if (leaf == QStringLiteral("reserve_factor"))
					item = scan.reserve;
				else
					continue;
				if (!item.value.isNull())
				{
					vals.append(item.value.isDouble()
						? QString::number(item.value.toDouble())
						: item.value.toString());
				}
			}
			if (vals.isEmpty())
				return QString();
			if (vals.size() > 3)
				return vals.mid(0, 3).join(QStringLiteral("、"))
					+ QStringLiteral("…（共%1 工况，见导出）").arg(vals.size());
			return vals.join(QStringLiteral("、"));
		}
		return QString();   //case.load_type 等其余键：展示层标"见导出"
	}

	void mStrengthReportDlg::refreshTextSlotPreview()
	{
		if (_textSlotPreview == nullptr)
			return;
		//T1：说明列=字典人话+当前值（未知键显示键名原文，不静默失实）；
		//绑定键移行级 tooltip；CB-15 ⚠ 语义保留（manual_fields 空值行首标）
		_textSlotPreview->setRowCount(0);
		for (const TextSlotRow& rowInfo : _textSlotRows)
		{
			const bool manualBind = rowInfo.bind.startsWith(QStringLiteral("manual_fields."));
			const QString desc = bindingKeyDescription(rowInfo.bind);
			const QString val = textSlotCurrentValue(rowInfo.bind);
			//无当前值：人工键标"未填写"（⚠ 同语义），自动键标"-"（与自动采集区 A-08 同口径）
			const QString showVal = val.isEmpty()
				? (manualBind ? QStringLiteral("未填写") : QStringLiteral("-"))
				: val;
			const QString text = (manualBind && val.isEmpty() ? QStringLiteral("⚠ ") : QString())
				+ (desc.isEmpty() ? rowInfo.bind : desc)
				+ QStringLiteral("＝") + showVal;
			const int row = _textSlotPreview->rowCount();
			_textSlotPreview->insertRow(row);
			QTableWidgetItem* anchorItem = readOnlyItem(
				QStringLiteral("「%1」").arg(rowInfo.anchor));
			anchorItem->setToolTip(QStringLiteral("绑定键：%1").arg(rowInfo.bind));
			_textSlotPreview->setItem(row, 0, anchorItem);
			QTableWidgetItem* bindItem = readOnlyItem(text);
			bindItem->setToolTip(QStringLiteral("绑定键：%1").arg(rowInfo.bind));
			_textSlotPreview->setItem(row, 1, bindItem);
		}
	}

	void mStrengthReportDlg::applyLoadState(const QString& statusHtml)
	{
		_statusLabel->setText(statusHtml);
		//CB-04：校验进行中一律禁用导出（Loaded 态可能是上一容器的陈旧态）；
		//PA-02：装配未就绪（S1 进行中/会话未建）同样禁用（R3-13 反向互斥补全）。
		//R3P-01：使能刷新单点化——统一经 updateAutoPreviewUi（本函数不再直置 _exportBtn）
		updateAutoPreviewUi();   //三期批次 A：Tab0 主按钮/底部导出钮随加载态/probe 正交刷新
	}

	void mStrengthReportDlg::refreshCollectedData()
	{
		if (_nodeCountEdit == nullptr)
			return;
		//页签①自动采集（FR-42 只读通道；A-08：不可用→"-"+消息区记录，不静默填错）
		QString statsErr;
		const QJsonObject stats = mStrengthReportCollector::collectModelStats(
			_softwareNameEdit->text(), statsErr);
		const int nodeCount = stats.value(QStringLiteral("node_count")).toInt(-1);
		const int elementCount = stats.value(QStringLiteral("element_count")).toInt(-1);
		_nodeCountEdit->setText(nodeCount >= 0 ? QString::number(nodeCount) : QStringLiteral("-"));
		_elementCountEdit->setText(elementCount >= 0 ? QString::number(elementCount) : QStringLiteral("-"));

		//页签②两组录入面行重建（二期批次 B：工况结果表+材料表，行=采集自动、
		//回显按名匹配的历史录入+失配黄字提示）+载荷表预览
		const QStringList caseNames = mStrengthReportCollector::collectCaseNames();
		rebuildCaseEntryTables();
		_loadTable->setRowCount(0);
		for (const QString& name : caseNames)
		{
			const int row = _loadTable->rowCount();
			_loadTable->insertRow(row);
			_loadTable->setItem(row, 0, new QTableWidgetItem(name));
			for (int col = 1; col < 4; ++col)
				_loadTable->setItem(row, col, new QTableWidgetItem(QStringLiteral("-")));
		}
		_modelSwitchHint->hide();
		refreshManualCandidates();   //本轮：模型候选随数据装载/刷新触点重填（与 autoForm 采集同触点）
		refreshTextSlotPreview();    //T1：文本槽预览当前值随采集刷新（model_stats 回显值触点）
	}

	//人工补录区 6 下拉候选重填（本轮）：items=模型精确候选（能取到的字段）+内置通用词表
	//（兜底），editable 手输值天然不受限。对话框单例复用（showInstance 不重建）+模型切换
	//（refreshCollectedData 触点）均经此重入重填，防旧模型候选残留；重建前保存当前录入值
	//（含配置回填值/用户手输值），重填后 setCurrentText 恢复——不在候选中也正常显示
	void mStrengthReportDlg::refreshManualCandidates()
	{
		if (_manualCombos.isEmpty())
			return;   //buildTabBasic 未跑（防御；正常构造序不可达）
		//字段序=约束形式/加载形式/载荷类型/计算方法/单元类型/对象名（与 _manualCombos 一致）
		QStringList modelElementTypes;   //单元类型：模型当前帧实际出现的类型字面
		QStringList modelObjectNames;    //对象名：后处理数据名+部件名清单
		if (currentPostDbDlg() != nullptr)
			modelElementTypes = collectModelElementTypes();
		modelObjectNames = collectModelObjectNames();
		const QList<QStringList> modelCandidates
		{
			QStringList(),          //约束形式：模型边界实体名为技术 ID 非报告措辞——仅通用词表
			QStringList(),          //加载形式：同上——仅通用词表
			QStringList(),          //载荷类型：无模型通道——仅通用词表
			QStringList(),          //计算方法：无模型通道——仅通用词表
			modelElementTypes,      //单元类型：模型精确候选在前（FR-311 同源字面）
			modelObjectNames        //对象名：不给通用表（纯手输+模型名候选，D-24②）
		};
		const QList<const QStringList*> genericCandidates
		{
			&genericConstraintDescs(), &genericLoadCaseDescs(), &genericLoadTypes(),
			&genericCalcMethods(), &genericElementTypes(), nullptr   //对象名：通用表=空
		};
		for (int i = 0; i < _manualCombos.size(); ++i)
		{
			QComboBox* combo = _manualCombos.at(i);
			if (combo == nullptr)
				continue;
			const QString current = combo->currentText();   //保存当前录入值（重入防丢）
			combo->clear();
			QStringList items;
			for (const QString& name : modelCandidates.at(i))
			{
				if (!items.contains(name))
					items.append(name);
			}
			if (genericCandidates.at(i) != nullptr)
			{
				for (const QString& name : *genericCandidates.at(i))
				{
					if (!items.contains(name))
						items.append(name);
				}
			}
			combo->addItems(items);
			combo->setCurrentText(current);   //回填值不在候选中也正常显示（editable 语义）
		}
	}

	void mStrengthReportDlg::rebuildCaseEntryTables()
	{
		if (_caseResultTable == nullptr || _materialTable == nullptr)
			return;
		_buildingCaseTables = true;

		//工况结果表：行=collectCaseNames 自动（行只读），按工况名匹配回显历史录入
		//（08 §2.3：重名匹配、当前模型不存在的历史条目保留在配置但不发射）
		//三期批次 B（FR-325/12 §3.2）：自动值预填+逐格来源标注——录入值优先；未录格=装配
		//扫描值预填（浅黄底+tooltip"自动"标注），用户修改即经 itemChanged 转"录入"落配置
		const QStringList caseNames = mStrengthReportCollector::collectCaseNames();
		QMap<QString, const AssemblyResult::CaseScan*> scanByCase;
		if (_assemblyReady)
		{
			for (const AssemblyResult::CaseScan& scan : _autoAssembly.cases)
				scanByCase.insert(scan.caseName, &scan);
		}
		_caseResultTable->setRowCount(0);
		for (const QString& name : caseNames)
		{
			const int row = _caseResultTable->rowCount();
			_caseResultTable->insertRow(row);
			_caseResultTable->setItem(row, 0, readOnlyItem(name));
			const QJsonObject entered = _config.caseResults.value(name);
			const AssemblyResult::CaseScan* scan = scanByCase.value(name, nullptr);
			static const AutoValue emptyAuto;
			const AutoValue* autoVals[4] =
			{
				scan != nullptr ? &scan->stressPeak : &emptyAuto,
				scan != nullptr ? &scan->dispMax : &emptyAuto,
				scan != nullptr ? &scan->buckling : &emptyAuto,
				&emptyAuto   //reserve_factor 工况位本批无扫描值（发射口径=材料级保守侧）
			};
			for (int col = 1; col < 5; ++col)
			{
				static const QString keys[] =
				{ QStringLiteral("stress_peak"), QStringLiteral("displacement_max"),
				  QStringLiteral("buckling_mode"), QStringLiteral("reserve_factor") };
				const QJsonValue enteredVal = entered.value(keys[col - 1]);
				const bool hasEntered = !enteredVal.isNull() && !enteredVal.isUndefined();
				//PA-06：自动值空值判定 isNull()+isUndefined() 双判（Undefined 非空但为
				//"无值"——误饰黄底+"自动·"tooltip 即出处标注失真；NSDMI 消根后此为防御）
				const bool hasAuto = !autoVals[col - 1]->value.isNull()
					&& !autoVals[col - 1]->value.isUndefined();
				QTableWidgetItem* item = new QTableWidgetItem(hasEntered
					? entryDisplayText(enteredVal)
					: (hasAuto ? entryDisplayText(autoVals[col - 1]->value) : QString()));
				item->setToolTip(col == 3
					? QStringLiteral("失稳模态阶次（正整数；留空=按缺项策略处理）")
					: QStringLiteral("留空=按缺项策略处理并告警"));
				if (!hasEntered && hasAuto)
				{
					//自动值预填标注（12 §3.2 逐格来源标注；浅黄底=自动，改格即转录入落配置）
					item->setBackground(QColor(0xFF, 0xF8, 0xE1));
					item->setToolTip(QStringLiteral("自动·%1（修改后转为录入值并保存）")
						.arg(autoVals[col - 1]->source)
						+ (autoVals[col - 1]->warn.isEmpty()
							? QString() : QStringLiteral("；%1").arg(autoVals[col - 1]->warn)));
				}
				_caseResultTable->setItem(row, col, item);
			}
		}

		//材料表：行=collectMaterialNames（D3-2 定序发射同一实现）；"default"行黄字
		//标注（C-4①：来源列+黄字提示，不允许用户把占位行当真实材料录入而无所提示）
		QString matErr;
		const mStrengthReportCollector::MaterialList mats =
			mStrengthReportCollector::collectMaterialNames(matErr);
		_materialRows.clear();
		_materialTable->setRowCount(0);
		for (const QString& name : mats.names)
		{
			MaterialRow rowInfo;
			rowInfo.name = name;
			rowInfo.source = mats.source;
			rowInfo.isDefault = (name == QStringLiteral("default"));
			_materialRows.append(rowInfo);
			const int row = _materialTable->rowCount();
			_materialTable->insertRow(row);
			QTableWidgetItem* nameItem = readOnlyItem(name);
			QTableWidgetItem* sourceItem = readOnlyItem(mats.source);
			if (rowInfo.isDefault)
			{
				//C-4①：显著标注（黄底+tooltip）
				nameItem->setBackground(QColor(0xFF, 0xF2, 0xCC));
				sourceItem->setBackground(QColor(0xFF, 0xF2, 0xCC));
				const QString tip = QStringLiteral("结果文件的默认材料名（无材料信息部件统一使用该默认名，非真实材料）");
				nameItem->setToolTip(tip);
				sourceItem->setToolTip(tip);
			}
			_materialTable->setItem(row, 0, nameItem);
			_materialTable->setItem(row, 1, sourceItem);
			const QJsonObject vals = _config.materialValues.value(name);
			QTableWidgetItem* maxItem = new QTableWidgetItem(
				entryDisplayText(vals.value(QStringLiteral("max_stress"))));
			maxItem->setToolTip(QStringLiteral("最大应力（非负数值；留空=按缺项策略处理）"));
			_materialTable->setItem(row, 2, maxItem);
			//三期批次 B（FR-324）：剩余系数自动值预填（录入优先；保守侧=limit÷全工况最大峰值）
			const QJsonValue enteredRf = vals.value(QStringLiteral("reserve_factor"));
			const bool hasEnteredRf = !enteredRf.isNull() && !enteredRf.isUndefined();
			const AutoValue autoRf = _assemblyReady
				? _autoAssembly.materialReserves.value(name) : AutoValue();
			//PA-06：自动值空值判定 isNull()+isUndefined() 双判（同工况表格子同口径）
			const bool hasAutoRf = !autoRf.value.isNull() && !autoRf.value.isUndefined();
			QTableWidgetItem* rfItem = new QTableWidgetItem(hasEnteredRf
				? entryDisplayText(enteredRf)
				: (hasAutoRf ? entryDisplayText(autoRf.value) : QString()));
			rfItem->setToolTip(QStringLiteral("剩余系数（正数；留空=按缺项策略处理）"));
			if (!hasEnteredRf && hasAutoRf)
			{
				rfItem->setBackground(QColor(0xFF, 0xF8, 0xE1));
				rfItem->setToolTip(QStringLiteral("自动·自动计算（材料库极限÷全工况最大应力峰值，"
					"偏保守；修改后转为录入值并保存）")
					+ (autoRf.warn.isEmpty() ? QString()
						: QStringLiteral("；%1").arg(autoRf.warn)));
			}
			_materialTable->setItem(row, 3, rfItem);
		}

		_buildingCaseTables = false;
		_lastEntryError.clear();
		updateEntryMsgLabel();
		updateVerdictLabel();
	}

	void mStrengthReportDlg::onCaseResultItemChanged(QTableWidgetItem* item)
	{
		if (item == nullptr || _buildingCaseTables || _caseResultTable == nullptr)
			return;
		if (item->column() == 0)   //工况名只读列（剥过 ItemIsEditable，防御）
			return;
		QTableWidgetItem* nameItem = _caseResultTable->item(item->row(), 0);
		if (nameItem == nullptr)
			return;
		const QString caseName = nameItem->text();
		QString key;
		QString colName;
		EntryKind kind = NonNegativeNumber;
		switch (item->column())
		{
		case 1: key = QStringLiteral("stress_peak");      colName = QStringLiteral("应力峰值");     kind = NonNegativeNumber; break;
		case 2: key = QStringLiteral("displacement_max"); colName = QStringLiteral("位移最大值");   kind = NonNegativeNumber; break;
		case 3: key = QStringLiteral("buckling_mode");    colName = QStringLiteral("失稳模态阶次"); kind = PositiveIntText;   break;
		case 4: key = QStringLiteral("reserve_factor");   colName = QStringLiteral("剩余系数");     kind = PositiveNumber;    break;
		default: return;
		}
		QJsonValue value;
		QString reason;
		if (parseEntryValue(item->text().trimmed(), kind, value, reason))
		{
			item->setData(Qt::BackgroundRole, QVariant());   //恢复默认底色
			QJsonObject vals = _config.caseResults.value(caseName);
			vals.insert(key, value);   //空文本=显式 null（未录→导出按缺项策略）
			_config.caseResults.insert(caseName, vals);
			if (_lastEntryError.startsWith(QStringLiteral("工况「%1」").arg(caseName)))
				_lastEntryError.clear();   //同工况修正后清除错误提示
		}
		else
		{
			//E2.2：非法输入红边+文案、不落配置（导出按 null=缺项策略标缺，不静默带错值）
			item->setBackground(QColor(0xFF, 0xC8, 0xC8));
			_lastEntryError = QStringLiteral("工况「%1」%2：%3（未写入，请修正或清空）")
				.arg(caseName, colName, reason);
		}
		updateEntryMsgLabel();
	}

	void mStrengthReportDlg::onMaterialValueItemChanged(QTableWidgetItem* item)
	{
		if (item == nullptr || _buildingCaseTables || _materialTable == nullptr)
			return;
		if (item->column() != 2 && item->column() != 3)   //材料名/来源列只读（防御）
			return;
		QTableWidgetItem* nameItem = _materialTable->item(item->row(), 0);
		if (item->row() < 0 || item->row() >= _materialRows.size() || nameItem == nullptr)
			return;
		const QString matName = _materialRows.at(item->row()).name;
		const QString key = item->column() == 2
			? QStringLiteral("max_stress") : QStringLiteral("reserve_factor");
		const QString colName = item->column() == 2
			? QStringLiteral("最大应力") : QStringLiteral("剩余系数");
		QJsonValue value;
		QString reason;
		if (parseEntryValue(item->text().trimmed(),
			item->column() == 2 ? NonNegativeNumber : PositiveNumber, value, reason))
		{
			item->setData(Qt::BackgroundRole, QVariant());
			QJsonObject vals = _config.materialValues.value(matName);
			vals.insert(key, value);
			_config.materialValues.insert(matName, vals);
			if (_lastEntryError.startsWith(QStringLiteral("材料「%1」").arg(matName)))
				_lastEntryError.clear();
			if (key == QStringLiteral("reserve_factor"))
				updateVerdictLabel();   //推导行实时刷新（08 §3.1）
		}
		else
		{
			item->setBackground(QColor(0xFF, 0xC8, 0xC8));
			_lastEntryError = QStringLiteral("材料「%1」%2：%3（未写入，请修正或清空）")
				.arg(matName, colName, reason);
		}
		updateEntryMsgLabel();
	}

	void mStrengthReportDlg::updateVerdictLabel()
	{
		if (_verdictLabel == nullptr)
			return;
		//D2-2/C-2 材料级 min 口径（08 §2.3）：全部已录材料级 reserve_factor 最小值
		//≥1.0→满足；任一<1.0→不满足；无录入→结论句按缺项文案。三态实时刷新。
		//三期批次 B（FR-326）：自动剩余系数随录入缺席处并入推导（材料级 min 口径不变，
		//verdict 链零改动——12 §2.2 #7"数据源随 §2.3 扩展"）
		bool has = false;
		double minRf = 0.0;
		for (const MaterialRow& rowInfo : _materialRows)
		{
			QJsonValue rf = _config.materialValues.value(rowInfo.name)
				.value(QStringLiteral("reserve_factor"));
			if (rf.isNull() || rf.isUndefined())
			{
				const AutoValue autoRf = _assemblyReady
					? _autoAssembly.materialReserves.value(rowInfo.name) : AutoValue();
				if (!autoRf.value.isNull())
					rf = autoRf.value;
			}
			if (rf.isDouble())
			{
				const double d = rf.toDouble();
				if (!has || d < minRf)
				{
					minRf = d;
					has = true;
				}
			}
		}
		if (!has)
		{
			_verdictLabel->setText(QStringLiteral(
				"<span style='color:#B8860B;'>结论判定: 自动（最小剩余系数≥1.0→满足）——"
				"当前无剩余系数录入/自动值，结论句按缺项文案输出</span>"));
		}
		else if (minRf >= 1.0)
		{
			_verdictLabel->setText(QStringLiteral(
				"<span style='color:#008000;'>结论判定: 自动（最小剩余系数≥1.0→满足）"
				"　当前推导=满足 (min=%1)</span>").arg(entryDisplayText(minRf)));
		}
		else
		{
			_verdictLabel->setText(QStringLiteral(
				"<span style='color:#C00000;'>结论判定: 自动（最小剩余系数≥1.0→满足）"
				"　当前推导=不满足 (min=%1)</span>").arg(entryDisplayText(minRf)));
		}
	}

	void mStrengthReportDlg::updateEntryMsgLabel()
	{
		if (_entryMsgLabel == nullptr)
			return;
		//三源优先级：录入校验错误（红）>历史录入失配（黄，08 §2.3）>E2.2 清洗计数（黄）
		if (!_lastEntryError.isEmpty())
		{
			_entryMsgLabel->setText(QStringLiteral("<span style='color:#C00000;'>录入校验：%1</span>")
				.arg(_lastEntryError));
			_entryMsgLabel->show();
			return;
		}
		int stale = 0;
		QSet<QString> activeCases;
		for (int row = 0; row < _caseResultTable->rowCount(); ++row)
		{
			if (QTableWidgetItem* nameItem = _caseResultTable->item(row, 0))
				activeCases.insert(nameItem->text());
		}
		for (auto it = _config.caseResults.constBegin(); it != _config.caseResults.constEnd(); ++it)
		{
			if (!activeCases.contains(it.key()))
				++stale;
		}
		QSet<QString> activeMats;
		for (const MaterialRow& rowInfo : _materialRows)
			activeMats.insert(rowInfo.name);
		for (auto it = _config.materialValues.constBegin(); it != _config.materialValues.constEnd(); ++it)
		{
			if (!activeMats.contains(it.key()))
				++stale;
		}
		if (stale > 0)
		{
			_entryMsgLabel->setText(QStringLiteral(
				"<span style='color:#B8860B;'>%1 条历史录入与当前模型不匹配（已保留在项目配置中，不参与导出）</span>")
				.arg(stale));
			_entryMsgLabel->show();
			return;
		}
		if (_config.lastSanitizedCount > 0)
		{
			_entryMsgLabel->setText(QStringLiteral(
				"<span style='color:#B8860B;'>已清洗 %1 条非法历史录入值（保存后生效）</span>")
				.arg(_config.lastSanitizedCount));
			_entryMsgLabel->show();
			return;
		}
		_entryMsgLabel->hide();
	}

	int mStrengthReportDlg::countMissingItems() const
	{
		//缺项预检（E1 前置预警口径）：单元类型（U-06 不得标缺）/对象名（D-24②）/
		//图片槽缺图（逐槽计数）；工况列表为空不计（引擎支持 0 实例空导出）
		int missing = 0;
		if (_elementTypeEdit->text().trimmed().isEmpty())
			++missing;
		if (_projectSubjectEdit->text().trimmed().isEmpty())
			++missing;
		for (const ImageSlot& slot : _imageSlots)
		{
			const QString path = _config.imagePaths.value(slot.key);
			if (path.isEmpty() || !QFile::exists(path))
				++missing;
		}
		return missing;
	}

	void mStrengthReportDlg::collectUiToConfig()
	{
		//PB-05：字符串录入统一 trimmed——纯空白串原样落配置会按"有效值"发射（空白串
		//被引擎当作绑定值清槽且无缺项告警）；空白归空=未录（null 走缺项策略，
		//CB-08/E2.2 清洗同口径）
		_config.softwareName = _softwareNameEdit->text().trimmed();
		_config.constraintDesc = _constraintDescEdit->text().trimmed();
		_config.loadCaseDesc = _loadCaseDescEdit->text().trimmed();
		_config.loadType = _loadTypeEdit->text().trimmed();
		_config.calcMethod = _calcMethodEdit->text().trimmed();
		_config.elementType = _elementTypeEdit->text().trimmed();
		//三期批次 A（FR-325 自动值不落配置，12 §2.4）：对象名自动预填守卫——当前文本仍等于
		//预填自动值（用户未触碰）→配置写空（发射经 slot_export 装配缓存注入）；用户改写=
		//录入落配置（持久化）；用户清空=E3-7 清除录入（textEdited 已挂重装配→自动回填）
		const QString subjectText = _projectSubjectEdit->text().trimmed();   //PB-05 同上
		if (!_autoPrefillSubject.isEmpty() && _autoPrefillSubject == subjectText)
			_config.projectSubject = QString();
		else
			_config.projectSubject = subjectText;
		if (!_autoPrefillSubject.isEmpty() && _autoPrefillSubject != subjectText)
			_autoPrefillSubject.clear();   //预填已偏离（改写/清空）→摘除自动标记（转录入语义）
		//三期批次 B：单元类型+默认句四键同款守卫（FR-325——自动值/默认句不落配置，
		//发射经 buildDataset 装配合并口；用户改写=录入落配置持久化）
		struct GuardSpec
		{
			QString* config;
			const QString text;
			QString* sentinel;
		};
		const QVector<GuardSpec> guards
		{
			{ &_config.elementType,    _elementTypeEdit->text(),    &_autoPrefillElementType },
			{ &_config.constraintDesc, _constraintDescEdit->text(), &_autoPrefillConstraint },
			{ &_config.loadCaseDesc,   _loadCaseDescEdit->text(),   &_autoPrefillLoadCase },
			{ &_config.loadType,       _loadTypeEdit->text(),       &_autoPrefillLoadType },
			{ &_config.calcMethod,     _calcMethodEdit->text(),     &_autoPrefillCalcMethod },
		};
		for (const GuardSpec& g : guards)
		{
			if (!g.sentinel->isEmpty() && *g.sentinel == g.text)
				*g.config = QString();          //未触碰=自动值/默认句不落配置
			else
				*g.config = g.text;             //改写/清空=用户显式值（空=清除录入）
			if (!g.sentinel->isEmpty() && *g.sentinel != g.text)
				g.sentinel->clear();            //预填已偏离→摘除自动标记（转录入语义）
		}
		_config.missingPolicy = _missingPolicyCombo->currentData().toString();
		//CB-12：路径记忆统一 trim——记忆值与引擎实际执行值（start 用 trimmed）保持一致
		_config.outputPath = _outputPathEdit->text().trimmed();
		_config.containerPath = _containerPathEdit->text().trimmed();
		//二期批次 C：开关态随配置持久化（页签④水印/页签③试点——抓取流程直读控件即时生效，
		//此处同步供 save/项目切换落盘；须置于 _imageSlots 空清单早退之前）
		_config.watermarkOff = _watermarkOffChk->isChecked();
		_config.semiAutoGrab = _semiAutoChk->isChecked();
		_config.autoCapture = _semiAutoChk->isChecked();   //三期批次 C/O3-2：开关合一镜像
		                                                   //（autoCapture=权威键，semiAutoGrab
		                                                   //仅作旧版读兼容镜像写入）
		//页签③槽图路径/题注：仅保留当前规则层声明的槽键（槽位清单随容器变更重建——T-08）。
		//CB-01：无槽位清单（容器未加载/校验失败）时不清空既有槽图映射——项目切换保存路径
		//依赖此处保留旧项目截图记忆（清单为空≠用户清图，不得误清持久化数据）
		if (_imageSlots.isEmpty())
			return;
		const QMap<QString, QString> prevPaths = _config.imagePaths;
		const QMap<QString, QString> prevCaptions = _config.imageCaptions;
		const QMap<QString, bool> prevWatermarks = _config.imageWatermarks;
		_config.imagePaths.clear();
		_config.imageCaptions.clear();
		_config.imageWatermarks.clear();
		for (const ImageSlot& slot : _imageSlots)
		{
			_config.imagePaths.insert(slot.key, prevPaths.value(slot.key));
			_config.imageCaptions.insert(slot.key, prevCaptions.value(slot.key, QString()));
			_config.imageWatermarks.insert(slot.key, prevWatermarks.value(slot.key, false));
		}
	}

	bool mStrengthReportDlg::ensureOutputPath()
	{
		QString path = _outputPathEdit->text().trimmed();
		if (path.isEmpty())
		{
			const QString dir = QCoreApplication::applicationDirPath()
				+ QStringLiteral("/Report/强度报告导出");
			QString projectId = mStrengthReportCollector::currentProjectId();
			if (projectId.isEmpty())
				projectId = QStringLiteral("default");
			path = QFileDialog::getSaveFileName(this, QStringLiteral("选择输出路径"),
				dir + QStringLiteral("/%1_%2.docx")
					.arg(projectId, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmm"))),
				QStringLiteral("Word 文档 (*.docx)"));
			if (path.isEmpty())
				return false;
			_outputPathEdit->setText(path);
		}
		//CB-12：后缀兜底按 .docx 结尾判断（"报告.v2" 这类多段名也要补，不只空后缀）
		if (!path.endsWith(QStringLiteral(".docx"), Qt::CaseInsensitive))
		{
			path += QStringLiteral(".docx");
			_outputPathEdit->setText(path);
		}
		return true;
	}

	QString mStrengthReportDlg::datasetJsonPath() const
	{
		return strengthReportTempDir() + QStringLiteral("/report_dataset.json");
	}

	QString mStrengthReportDlg::probeDatasetJsonPath() const
	{
		//CB-04：校验与导出分文件——校验器读数据集期间用户点导出不再竞写同一文件
		return strengthReportTempDir() + QStringLiteral("/probe_dataset.json");
	}

	void mStrengthReportDlg::slot_browseContainer()
	{
		if (_exporting)
			return;
		const QString start = _containerPathEdit->text().trimmed();
		const QString file = QFileDialog::getOpenFileName(this,
			QStringLiteral("选择规则模板文档"),
			start.isEmpty() ? QCoreApplication::applicationDirPath() : start,
			QStringLiteral("规则模板文档 (*.rtdoc)"));
		if (file.isEmpty())
			return;
		_containerPathEdit->setText(file);
		slot_revalidate();
	}

	void mStrengthReportDlg::slot_openImportWizard()
	{
		//二期批次 D（FR-251/FR-252，08 §2.6）：模板导入向导——对主对话框模态（主对话框
		//对主窗口仍非模态，U-01 不变）；全部重活=子进程调用既有 CLI（digest/rules/
		//validate/引擎试导出），中间产物全在临时目录（E2.7 零半成品）。定版成功→
		//回填加载栏+自动重新校验（08 步⑤"写入主对话框加载栏并自动重新检查"）
		if (_exporting)
			return;
		switchProjectConfigIfNeeded();   //CB-01：向导定版产物归属当前项目
		mStrengthReportImportWizard wizard(this);
		wizard.exec();
		const QString finalizedPath = wizard.finalizedContainerPath();
		if (!finalizedPath.isEmpty())
		{
			_containerPathEdit->setText(finalizedPath);
			slot_revalidate();
		}
	}

	void mStrengthReportDlg::slot_revalidate()
	{
		if (_exporting)
			return;
		switchProjectConfigIfNeeded();   //CB-01：校验结果归属当前项目
		if (_probe->isRunning())
		{
			//CB-04：校验进行中不再静默忽略新校验请求（改选容器后状态行仍显示旧容器
			//"已加载"的陈旧态）——排队等待当前校验完成后自动补跑
			_revalidatePending = true;
			_exportBtn->setEnabled(false);
			updateAutoPreviewUi();   //PA-15：Tab0 主按钮使能随 probe 运行刷新（消"可点但静默 no-op"陈旧外观）
			_statusLabel->setText(QStringLiteral(
				"<span style='color:#606060;'>检查中…（新选择的容器将在当前校验完成后自动重新检查）</span>"));
			return;
		}
		const QString path = _containerPathEdit->text().trimmed();
		if (path.isEmpty())
		{
			_loadState = NotLoaded;
			applyLoadState(QStringLiteral("<span style='color:#C00000;'>未加载——请选择规则模板文档(.rtdoc)</span>"));
			return;
		}
		//校验数据集（tdr_validate 第 2 参数）：装载/重校验时以当前采集+录入态构建一次——
		//S5-2 instance_key 探测需要数据集 cases 键在位（缺数据集实测误报"校验失败 1 条"）。
		//CB-04：落盘用校验专用文件名（probe_dataset.json），与导出数据集（report_dataset.json）
		//分文件——大容器校验窗口内启动导出不再竞写同一 JSON
		collectUiToConfig();
		QString datasetErr;
		const QJsonDocument dataset = mStrengthReportCollector::buildDataset(_config, datasetErr);
		const QString jsonPath = probeDatasetJsonPath();
		QDir().mkpath(QFileInfo(jsonPath).absolutePath());
		QString datasetForProbe;
		QFile jsonFile(jsonPath);
		if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			jsonFile.write(dataset.toJson(QJsonDocument::Indented));
			jsonFile.close();
			datasetForProbe = jsonPath;
		}
		else
		{
			//数据集落盘失败不阻断校验：降级为无数据集参数（instance_key 探测降级兼容路径）
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]检查数据写入失败（%1），将按无数据继续检查").arg(jsonPath), true);
		}
		_probeProjectId = _config.projectId();
		_probeContainerPath = path;   //L6-02：容器路径随探测发起快照（会话指纹一半，与项目代币同生命周期）
		//CB-04：校验窗口内[一键导出]禁用（旧状态是上一容器的陈旧 Loaded 态，不得据此启动导出）；
		//状态行先置"校验中"——probe 同步完成（未选路径/结构层失败）时随后由 finished 覆盖
		_exportBtn->setEnabled(false);
		_statusLabel->setText(QStringLiteral("<span style='color:#606060;'>检查中…</span>"));
		QString err;
		_probe->probe(path, datasetForProbe, err);
		updateAutoPreviewUi();   //PA-15：probe 发起后 Tab0 主按钮随运行态刷新（含同步完成快路径，届时 finished 已覆盖）
		if (!err.isEmpty())
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
				QStringLiteral("[强度报告]%1").arg(err), true);
	}

	void mStrengthReportDlg::slot_onProbeFinished(
		const mStrengthReportContainerProbe::Result& result)
	{
		//CB-01/R2-04：过期结果丢弃——探测发起代币（发起时快照的项目标识，切项目不覆写）
		//与当前项目不一致＝旧项目探测结果不得写入新项目状态；或校验期间有新校验请求
		//排队。两种情况都以当前输入重跑一次校验收敛
		if (_probeProjectId != _config.projectId() || _revalidatePending)
		{
			_revalidatePending = false;
			slot_revalidate();
			return;
		}
		//状态映射：两枚举数值序已对齐（见头文件注），probe Status→LoadState 直接映射
		_loadState = static_cast<LoadState>(result.status);
		_templateName = result.templateName;
		_validationErrors = result.errors;
		_rulesDocument = result.rulesDocument;
		//L6-02：待确认槽键集仅在容器会话实际变更时清零（容器路径或模板名变化）——同容器
		//"重新检查"、排队重校验、切走再切回同容器不得静默丢弃试点"待确认"标记（B-204
		//人工核对最后闸）。首次指纹为空 vs 非空必不等→首探照旧清零（集合本为空，无副作用）
		const QString sessionKey = _probeContainerPath + QChar('|') + _templateName;
		if (sessionKey != _pendingConfirmSessionKey)
		{
			_pendingConfirmKeys.clear();   //L5-02：容器会话重建收口（新容器未试点过的槽不得继承旧容器试点标记，防 B-204 计数失真）
			_pendingConfirmSessionKey = sessionKey;
		}
		parseRulesDocument(_rulesDocument);
		refreshTextSlotPreview();
		rebuildImageTable();
		//三期批次 A（13 §3）：probe 完成+Loaded 态→Tab0 自动装配触发（12 §2.1 S0→S1；
		//probe 运行中/导出中不装配——R3-13 并发交叠闸）
		if (result.status == mStrengthReportContainerProbe::Loaded && !_exporting)
			startAutoAssembly();

		_missingCount = countMissingItems();
		const QString templateSuffix = _templateName.isEmpty()
			? QString() : QStringLiteral(" · 模板\"%1\"").arg(_templateName);
		switch (result.status)
		{
		case mStrengthReportContainerProbe::Loaded:
			//已加载（有缺项=黄字预警，不阻断——02 §3.4）
			if (_missingCount > 0)
				applyLoadState(QStringLiteral("<span style='color:#B8860B;'>已加载·有缺项 %1 项%2</span>")
					.arg(_missingCount).arg(templateSuffix));
			else
				applyLoadState(QStringLiteral("<span style='color:#008000;'>已加载·检查通过%1</span>")
					.arg(templateSuffix));
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
				QStringLiteral("[强度报告]规则模板已加载并检查通过：%1").arg(_probe->containerPath()), true);
			break;
		case mStrengthReportContainerProbe::IntegrityFailed:
			applyLoadState(QStringLiteral("<span style='color:#C00000;'>%1</span>")
				.arg(result.message));
			break;
		case mStrengthReportContainerProbe::VersionMismatch:
			applyLoadState(QStringLiteral("<span style='color:#C00000;'>%1%2</span>")
				.arg(result.message, templateSuffix));
			break;
		case mStrengthReportContainerProbe::ValidationFailed:
			applyLoadState(QStringLiteral("<span style='color:#C00000;'>%1 <a href=\"#errors\">查看全部</a>%2</span>")
				.arg(result.message, templateSuffix));
			break;
		default:
			applyLoadState(QStringLiteral("<span style='color:#C00000;'>未加载——请选择规则模板文档(.rtdoc)</span>"));
			break;
		}
	}

	void mStrengthReportDlg::slot_openErrorList()
	{
		if (_validationErrors.isEmpty())
			return;
		QMessageBox::information(this, QStringLiteral("检查失败明细（%1 条）")
			.arg(_validationErrors.size()), _validationErrors.join(QStringLiteral("\n")));
	}

	void mStrengthReportDlg::slot_viewportChanged()
	{
		abortAlignWait();   //批次 C：等待期间激活视口变化→立即中止试点等待（B-6① 同口径）
		switchProjectConfigIfNeeded();   //CB-01：项目/模型切换→重载对应配置（D-12）
		_viewportAvailable = (getCVM->getPostRenderWidget() != nullptr);
		refreshCollectedData();   //A3：工况列表随当前分析结果自动刷新
		setImageActionsEnabled(_viewportAvailable && !_exporting);   //D-13：视口恢复→抓取恢复可用
		//三期批次 A：模型切换→重装配+预览刷新（12 §3.1；CB-01 切换链后按新项目通道重取）。
		//PA-07：probe 运行中不装配（R3-13"装配触发挂 slot_onProbeFinished 之后"）——
		//项目切换链 switchProjectConfigIfNeeded 已因记忆容器路径启动异步 probe（_rulesDocument
		//刚清空），此处无条件装配会先行"裸装配"全量扫描（双倍 IO+无 automation 瞬态误导），
		//probe 完成链（slot_onProbeFinished→startAutoAssembly，带 automation）统一触发
		if (!_exporting && !_probe->isRunning())
			startAutoAssembly();
	}

	void mStrengthReportDlg::slot_viewportRemoved(int /*id*/)
	{
		//T-23/D-13：视口关闭→[抓取当前视口]禁用；已抓截图文件保留（按目录组织，V-05）；
		//页签①自动采集区标"数据待刷新/模型已切换"。
		//CB-01：视口关闭时项目标识为空，switchProjectConfigIfNeeded 内部保留原配置待回切
		abortAlignWait();   //批次 C/B-6①：等待期间视口关闭→立即中止试点等待（无落图）
		switchProjectConfigIfNeeded();
		_viewportAvailable = (getCVM->getPostRenderWidget() != nullptr);
		if (!_viewportAvailable)
			_modelSwitchHint->show();
		setImageActionsEnabled(_viewportAvailable && !_exporting);
		//三期批次 A：视口关闭→退化重装配（自动值撤回+预览失败标注；导出仍可用=缺项策略语义）。
		//PA-07：probe 运行中不装配（R3-13，与 slot_viewportChanged 同闸）
		if (!_exporting && !_probe->isRunning())
			startAutoAssembly();
	}

	void mStrengthReportDlg::rebuildImageTable()
	{
		if (_imageTable == nullptr)
			return;
		//CB-02：重建时（校验完成/项目切换回显）重新查询视口在位——不依赖"先有对话框
		//后有视口信号"的时序，单视口首开场景按钮态与实际一致
		_viewportAvailable = (getCVM->getPostRenderWidget() != nullptr);
		_buildingImageTable = true;   //CB-05：抑制 itemChanged 回写（回显写入会触发信号）
		_imageTable->setRowCount(0);
		for (int i = 0; i < _imageSlots.size(); ++i)
		{
			const ImageSlot& slot = _imageSlots.at(i);
			const int row = _imageTable->rowCount();
			_imageTable->insertRow(row);

			//回显已抓/导入图（配置记忆，含缩略图）；CB-15：缺图槽在槽位/截图格标 ⚠（02 §3.4）
			const QString savedPath = _config.imagePaths.value(slot.key);
			const bool missing = savedPath.isEmpty() || !QFile::exists(savedPath);

			//补丁4（方案 a）：撤"定位方式/绑定键"两列，原始值移入行级 tooltip
			//（沿用 UI-05 既有词"定位方式/绑定键"，规则层技术标识仅悬停可见）
			const QString techTip = QStringLiteral("定位方式：%1\n绑定键：%2")
				.arg(slot.anchorDesc, slot.bind);

			QTableWidgetItem* keyItem = readOnlyItem(missing
				? QStringLiteral("⚠ %1").arg(slot.key) : slot.key);
			keyItem->setToolTip(missing
				? QStringLiteral("该槽缺图：导出按缺项策略处理，或抓取/导入补图\n\n") + techTip
				: techTip);
			_imageTable->setItem(row, 0, keyItem);

			if (!savedPath.isEmpty())
			{
				//B-204：试点抓取且未经替换的槽在重建回显时保留"待确认"标记（标记数据面=
				//_pendingConfirmKeys，不随表重建丢失）；其余=一期"已就绪"文案不变
				QTableWidgetItem* item = readOnlyItem(missing
					? QStringLiteral("⚠ 缺图（文件已不存在）")
					: (_pendingConfirmKeys.contains(slot.key)
						? QStringLiteral("已抓取·待确认") : QStringLiteral("已就绪")));
				item->setIcon(QIcon(savedPath));
				//Q2-4 逐槽追溯：截图落槽时的水印档随槽展示（批次 C 开关即时生效的追溯面）
				QString tip = savedPath;
				if (_config.imageWatermarks.contains(slot.key))
				{
					tip += QStringLiteral("\n截图档：") + (_config.imageWatermarks.value(slot.key)
						? QStringLiteral("无水印（交付版）") : QStringLiteral("带水印"));
				}
				item->setToolTip(tip + QStringLiteral("\n\n") + techTip);
				_imageTable->setItem(row, 1, item);
			}
			else
			{
				QTableWidgetItem* missingItem = readOnlyItem(QStringLiteral("⚠ 缺图"));
				missingItem->setToolTip(techTip);
				_imageTable->setItem(row, 1, missingItem);
			}

			//操作区：抓取当前视口/从文件导入/清除（逐槽三钮）
			auto* actions = new QWidget(_imageTable);
			auto* btnRow = new QHBoxLayout(actions);
			btnRow->setContentsMargins(0, 0, 0, 0);
			auto* grabBtn = new QPushButton(QStringLiteral("抓取"), actions);
			auto* importBtn = new QPushButton(QStringLiteral("导入"), actions);
			auto* clearBtn = new QPushButton(QStringLiteral("清除"), actions);
			btnRow->addWidget(grabBtn);
			btnRow->addWidget(importBtn);
			btnRow->addWidget(clearBtn);
			_imageTable->setCellWidget(row, 2, actions);
			connect(grabBtn, &QPushButton::clicked, this, [this, row]() { slot_grabImage(row); });
			connect(importBtn, &QPushButton::clicked, this, [this, row]() { slot_importImage(row); });
			connect(clearBtn, &QPushButton::clicked, this, [this, row]() { slot_clearImage(row); });

			//CB-05：题注列——槽带 image.<key>.caption 绑定时可编辑录入（持久化到项目配置），
			//否则只读"-"
			if (slot.captionBind)
			{
				QTableWidgetItem* captionItem =
					new QTableWidgetItem(_config.imageCaptions.value(slot.key));
				captionItem->setToolTip(QStringLiteral("题注（image.%1.caption 绑定；留空=该题注槽按缺项策略处理）").arg(slot.key)
					+ QStringLiteral("\n\n") + techTip);
				_imageTable->setItem(row, 3, captionItem);
			}
			else
			{
				QTableWidgetItem* dashItem = readOnlyItem(QStringLiteral("-"));
				dashItem->setToolTip(techTip);
				_imageTable->setItem(row, 3, dashItem);
			}
		}
		_buildingImageTable = false;
		setImageActionsEnabled(_viewportAvailable && !_exporting);
		updateGrabButtonsForPilot();   //批次 C：重建后按试点开关态恢复抓取钮文案
	}

	void mStrengthReportDlg::setImageActionsEnabled(bool grabEnabled)
	{
		//T-23/D-13：无激活视口→[抓取]禁用、[导入]/[清除]仍可用；导出中全部禁用
		for (int row = 0; row < _imageTable->rowCount(); ++row)
		{
			if (QPushButton* grabBtn = imageActionBtn(_imageTable, row, 0))
				grabBtn->setEnabled(grabEnabled);
			if (QPushButton* importBtn = imageActionBtn(_imageTable, row, 1))
				importBtn->setEnabled(!_exporting);
			if (QPushButton* clearBtn = imageActionBtn(_imageTable, row, 2))
				clearBtn->setEnabled(!_exporting);
		}
	}

	void mStrengthReportDlg::slot_grabImage(int row)
	{
		if (_exporting || row < 0 || row >= _imageSlots.size())
			return;
		//批次 C：对齐抓取进行中→拒绝并发重入（等待循环期间用户事件仍在派发）
		if (_alignBusy)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]对齐抓取进行中，请等待当前抓取完成"), true);
			return;
		}
		switchProjectConfigIfNeeded();   //CB-01：防信号缺失时截图落错项目配置/目录
		if (_exporting || row < 0 || row >= _imageSlots.size())
			return;
		const QString key = _imageSlots.at(row).key;
		//页签④水印开关即时生效（Q2-4=A 全局档）：抓取时直读控件当前态（QMap 追溯供查询）
		const bool watermarkOff = _watermarkOffChk->isChecked();
		//试点开关（默认关）：关闭=一期手动抓取链（T-21 时序，与批次 C 前交互逐字节一致）；
		//开启=对齐抓取四步流程（08 §2.5，映射/D3-3+等就绪/C-1+同步上屏/防抖+复用抓取链）
		const bool pilot = _semiAutoChk->isChecked();
		QString outPath;
		QString err;
		QString warn;
		bool ok = false;
		if (pilot)
		{
			_alignBusy = true;
			//LG-10（UB-06）：对齐抓取 5s 等待循环期间同步禁用全局重入入口（嵌套事件循环
			//仍派发用户输入——导入模板/映射设置等模态子对话框可嵌套在等待循环之上"套娃"，
			//校验/保存/导出与视口抓取互踩）。页签整体+顶部入口+底部按钮禁用，等待结束恢复
			//（底部导出钮使能经 R3P-01 单点刷新恢复）
			_tabs->setEnabled(false);
			_browseContainerBtn->setEnabled(false);
			_importTemplateBtn->setEnabled(false);
			_revalidateBtnTop->setEnabled(false);
			_revalidateBtnBottom->setEnabled(false);
			_saveConfigBtn->setEnabled(false);
			_exportBtn->setEnabled(false);
			ok = grabWithAlign(row, watermarkOff, outPath, err, &warn);
			_tabs->setEnabled(true);
			_browseContainerBtn->setEnabled(true);
			_importTemplateBtn->setEnabled(true);
			_revalidateBtnTop->setEnabled(true);
			_revalidateBtnBottom->setEnabled(true);
			_saveConfigBtn->setEnabled(true);
			updateAutoPreviewUi();
			_alignBusy = false;
		}
		else
		{
			ok = mStrengthReportCollector::saveViewportScreenshot(key, this, watermarkOff,
				outPath, err, &warn);
		}
		if (!warn.isEmpty())
		{
			//09 §0.2：rend 为空等开关失效告警（带水印抓取不阻断）
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(warn), true);
		}
		if (!ok)
		{
			//无假图原则（FR-221）：超时/中止/失败→不落图，槽保持原状（回退手动提示已含于 err）
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
				QStringLiteral("[强度报告]%1").arg(err), true);
			_resultMessages->appendPlainText(err);
			return;
		}
		_config.imagePaths.insert(key, outPath);
		_config.imageWatermarks.insert(key, watermarkOff);   //Q2-4 逐槽追溯：截图时水印档
		//B-204（C13 裁定④）：试点抓取=入待确认键集（导出成功汇总计数提示行数据面）；
		//手动抓取=替换确认语义，移除键集
		if (pilot)
			_pendingConfirmKeys.insert(key);
		else
			_pendingConfirmKeys.remove(key);
		//试点抓取=缩略图回显+"待确认"标记（08 §2.5 步4：人工核对=防假图静默入包最后闸）；
		//手动抓取=一期"已就绪"文案不变（试点关闭回归锚点）
		QTableWidgetItem* item = readOnlyItem(pilot
			? QStringLiteral("已抓取·待确认") : QStringLiteral("已就绪"));
		item->setIcon(QIcon(outPath));
		item->setToolTip(outPath + QStringLiteral("\n截图档：") + (watermarkOff
			? QStringLiteral("无水印（交付版）") : QStringLiteral("带水印")));
		_imageTable->setItem(row, 1, item);
		MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
			QStringLiteral("[强度报告]槽「%1」截图已保存：%2%3").arg(key, outPath,
				watermarkOff ? QStringLiteral("（无水印档）") : QString()), true);
	}

	//——二期批次 C：半自动试点"对齐抓取"（FR-221，08 §2.5 四步；09 §0.3/§4.5）——
	bool mStrengthReportDlg::grabWithAlign(int row, bool watermarkOff,
		QString& outPath, QString& err, QString* warn)
	{
		const QString key = _imageSlots.at(row).key;
		//渲染会话可达性前置（信号不可达=与超时回退同口径中止，R2-7 降级面）
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr)
		{
			err = QStringLiteral("无激活后处理视口，对齐抓取已中止（请改用手动抓取）");
			return false;
		}
		//只调用既有门面（getPostRender 先例 mPost3DResult.cpp:5774；不触碰他人并行编辑文件）
		std::shared_ptr<mxr::REBUILD_PostRender> postRender = rend->getPostRender();
		if (postRender == nullptr)
		{
			err = QStringLiteral("3D 渲染不可用，对齐抓取已中止（请改用手动抓取）");
			return false;
		}
		//动画播放中中止试点抓取（C-1：动画步进信号 PlayAdvance 不作就绪判据的入口闸）
		if (postRender->playbackState() == mxr::PlaybackState::Playing)
		{
			err = QStringLiteral("动画播放中，对齐抓取已中止（请先停止动画或改用手动抓取）");
			return false;
		}
		//步1 目标对齐（08 §2.5/D3-3）：映射表首版仅帧标识；空映射槽=跳过切换与等待
		//（用户先在主窗口手动切好目标画面再点抓取）——直接走下方同步抓取段
		//步1 目标对齐（08 §2.5/D3-3；三期批次 C 映射升级）：帧标识取法=四元组映射帧维优先
		//（slotMappings，13 §4.2）→旧 slotFrameMap int 形态兼容回退（R3-9）；空映射槽=跳过
		//切换与等待（用户先在主窗口手动切好目标画面再点抓取）——直接走下方同步抓取段。
		//试点"对齐抓取"仍仅帧维应用（12 §2.5①：试点四步语义不变——变量对齐=三期批量链）
		int targetFrame = -1;
		{
			const QJsonObject quad = _config.slotMappings.value(key);
			if (!quad.isEmpty())
				targetFrame = quad.value(QStringLiteral("frame")).toInt(-1);
		}
		if (targetFrame <= 0)
			targetFrame = _config.slotFrameMap.value(key, -1);
		const bool haveMapping = targetFrame > 0;
		if (haveMapping)
		{
			bool received = false;
			bool aborted = false;
			QEventLoop waitLoop;
			//C-1 硬约束②：订阅 connect 必须先于切换指令——同线程 direct connection 在切换
			//调用栈内同步发射（slot_changedFrame→seekFrame(fi,UserSeek,emit=true)），先切后连
			//必漏收→恒 5s 假超时（09 §0.3）
			const QMetaObject::Connection readyConn = connect(postRender.get(),
				&mxr::REBUILD_PostRender::frameChanged, this,
				[&received, targetFrame](int frameId, mxr::FrameChangeSource source)
				{
					//C-1 硬约束①：frameId==目标帧 且 source==UserSeek 双过滤——动画步进
					//（step 恒发含同帧）/他人切帧不提前放行等待，防抓到错帧（09 §0.3）
					if (frameId == targetFrame && source == mxr::FrameChangeSource::UserSeek)
						received = true;
				});
			//切换指令=与用户手动操作完全相同的既有切帧链：MechCommon 全局信号 changedFrame→
			//云图面板 slot_changedFrame（面板构造期连接，mPost3DResult.cpp:631）→
			//seekFrame(fi, UserSeek, emit=true)——零触碰面板文件（调用先例 mPostAdvancedQuery.cpp:413）
			MechCommon::mMechGlobalSignals::getInstance()->changedFrame(targetFrame);
			//direct 同步发射时 received 可能已置位；异步收敛（目标帧未装载经 frameReady 重入
			//seek）或链路不可达场景走等待循环
			if (!received)
			{
				//步2 等待画面就绪（FR-221）：等待态仅置灰不切按钮文字（FR-05/WO-5 改法A：
				//原"等待画面…"文案 L4 裁~8px/L5 裁~17px 根因消除），提示移 _statusLabel；
				//上限 5s（09 定值）→超时不抓取回退手动（无假图）。QPointer 守护：等待期间
				//视口关闭可触发 rebuildImageTable 删除重建行控件，恢复使能前必须判空
				QPointer<QPushButton> grabBtn = imageActionBtn(_imageTable, row, 0);
				const QString prevStatus = _statusLabel->text();   //FR-05：状态行快照（配对清除用）
				if (grabBtn != nullptr)
				{
					grabBtn->setEnabled(false);
				}
				//FR-05：等待提示走既有 RichText 状态行（灰 span 沿用 :1598/:1636 同款）；
				//末段"等待画面…"为 E10 清除守卫判据片段，改词须两处同步
				_statusLabel->setText(QStringLiteral(
					"<span style='color:#606060;'>槽「%1」等待画面…</span>").arg(key));
				_alignAborted = false;
				_alignLoop = &waitLoop;   //视口关闭/对话框关闭→abortAlignWait→立即中止（B-6①）
				const QMetaObject::Connection loopConn = connect(postRender.get(),
					&mxr::REBUILD_PostRender::frameChanged, this,
					[&waitLoop, targetFrame](int frameId, mxr::FrameChangeSource source)
					{
						if (frameId == targetFrame && source == mxr::FrameChangeSource::UserSeek)
							waitLoop.quit();
					});
				QTimer waitTimer;
				waitTimer.setSingleShot(true);
				connect(&waitTimer, &QTimer::timeout, &waitLoop, &QEventLoop::quit);
				waitTimer.start(5000);
				waitLoop.exec();
				_alignLoop = nullptr;
				disconnect(loopConn);
				if (grabBtn != nullptr)
				{
					grabBtn->setEnabled(_viewportAvailable && !_exporting);
				}
				//FR-05：提示清除与置灰严格配对（三条退出路径：就绪/5s超时/中止全部线性收敛
				//至此，aborted 的 return false 在其后）。守卫：等待期若被在飞 probe 的
				//applyLoadState 覆盖，不回写陈旧快照；提示片段仍在才清除——判据片段须与
				//上方 E9 提示尾段同步（S-1）
				if (_statusLabel->text().contains(QStringLiteral("等待画面…")))
					_statusLabel->setText(prevStatus);
				aborted = _alignAborted;
			}
			disconnect(readyConn);
			if (aborted)
			{
				err = QStringLiteral("视口或对话框已关闭，对齐抓取已中止（未生成截图）");
				return false;
			}
			if (!received)
			{
				err = QStringLiteral("视口画面未就绪（等待超时），已回退手动抓取；"
					"本槽未生成截图，请确认对应设置的帧号或改用手动抓取");
				return false;
			}
		}
		//步3 就绪后上屏同步（09 §0.3/D3-4 定版序）：frameChanged 发于场景图 bind 后、
		//paintGL 前存在上屏时差——update+processEvents 一轮→同步 grab()（QWidget::grab 对
		//QOpenGLWidget 是同步 GL 渲染=上屏同步原语，兼一期预热）→200ms 防抖（DWM 合成滞后，
		//D3-4 初值，S6 真机实测回填）
		QWidget* subwin = getCVM->activatePostViewport();
		if (subwin == nullptr)
		{
			err = QStringLiteral("无激活后处理视口，对齐抓取已中止（请改用手动抓取）");
			return false;
		}
		subwin->update();
		QApplication::processEvents();
		subwin->grab();
		QEventLoop debounceLoop;
		QTimer::singleShot(200, &debounceLoop, &QEventLoop::quit);
		debounceLoop.exec();
		//步4 抓取：复用 §0.2 同一抓取链（含水印开关语义，logo 守卫同 RAII 收口——C-5）
		return mStrengthReportCollector::saveViewportScreenshot(key, this, watermarkOff,
			outPath, err, warn);
	}

	void mStrengthReportDlg::abortAlignWait()
	{
		//B-6①：等待期间视口关闭/移除/对话框关闭→立即中止试点等待（无落图、无假超时）。
		//仅在有活跃等待循环时生效（_alignLoop 非空）；旗标由 grabWithAlign 于循环前复位
		if (_alignLoop != nullptr)
		{
			_alignAborted = true;
			_alignLoop->quit();
		}
		//三期批次 C：批量截图等待循环同漏斗中止（R3-7——abort 既有通道扩展到批量级，
		//中止剩余槽留空+原因分类，绝不阻塞导出）
		if (_autoCapture != nullptr)
			_autoCapture->abortBatch();
	}

	void mStrengthReportDlg::updateGrabButtonsForPilot()
	{
		if (_imageTable == nullptr)
			return;
		const QString text = _semiAutoChk != nullptr && _semiAutoChk->isChecked()
			? QStringLiteral("对齐抓取") : QStringLiteral("抓取");
		for (int row = 0; row < _imageTable->rowCount(); ++row)
		{
			if (QPushButton* grabBtn = imageActionBtn(_imageTable, row, 0))
				grabBtn->setText(text);
		}
	}

	void mStrengthReportDlg::slot_semiAutoToggled(bool on)
	{
		_mappingBtn->setEnabled(on);      //试点关闭=映射入口禁用（08 §3.2 开关关闭回归锚点）
		updateGrabButtonsForPilot();
		_config.semiAutoGrab = on;        //即时生效（与页签④水印开关同口径，随配置持久化）
		_config.autoCapture = on;         //三期批次 C/O3-2：开关合一镜像（autoCapture=权威键）
		refreshCaptureRows();             //Tab0 截图槽列表/提示行随开关刷新
		if (on)
		{
			//12 §2.5③：试点一次性提示升级为"批量完成后请逐槽核对缩略图"
			QMessageBox::information(this, QStringLiteral("自动截图"),
				QStringLiteral("自动截图已开启：一键导出将按「对应设置」逐槽批量截图"
					"（单槽失败留空不阻塞导出）。\n"
					"批量抓取完成后请逐槽核对缩略图；期间请勿操作 3D 视口。"));
		}
	}

	void mStrengthReportDlg::slot_editSlotFrameMap()
	{
		if (_imageSlots.isEmpty())
		{
			QMessageBox::information(this, QStringLiteral("槽位对应设置"),
				QStringLiteral("当前无已加载规则模板的图片槽声明，请先加载规则模板"
					"（页签③槽位清单由规则模板声明驱动）。"));
			return;
		}
		QDialog dlg(this);
		//三期批次 C（13 §3 批次 C 页签③行）：既有映射编辑对话框扩为五列（槽键/工况/帧/
		//变量/分量）——四元组写 slotMappings（批次 B 配置面消费）；旧 slotFrameMap int 形态
		//作为帧维预填来源读取（R3-9 兼容），确定后迁移写入四元组
		dlg.setWindowTitle(QStringLiteral("槽位对应设置（槽→工况/帧/变量/分量）"));
		auto* layout = new QVBoxLayout(&dlg);
		layout->addWidget(new QLabel(QStringLiteral(
			"逐槽声明自动截图目标画面：帧=正整数（与主窗口帧号一致）；工况/变量/分量留空"
			"=仅按帧应用；网格图槽变量填 SolidColor 时帧可留空=抓当前帧。\n"
			"全留空=该槽不参与自动截图（一键导出时保持现状）。"), &dlg));
		auto* table = new QTableWidget(_imageSlots.size(), 5, &dlg);
		table->setHorizontalHeaderLabels(QStringList
		{ QStringLiteral("槽位"), QStringLiteral("工况"), QStringLiteral("帧（正整数）"),
		  QStringLiteral("变量"), QStringLiteral("分量") });
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		//S10：数据源快照（postDB 注册面 + guidata 云图当前选中）——新行预填=云图面板
		//当前选中（工况/变量/分量/帧），取不到=留空；候选清单取不到=空下拉（可手输）
		QPointer<PostData::mPostDataBase> mapPostDB = currentPostDbDlg();
		QString cloudCase, cloudVar, cloudCompo;
		int cloudFrame = -1;
		{
			REBUILD_PostRend* mapRend = getCVM->getPostRenderWidget();
			mPostGUIData* mapGui = mapRend != nullptr
				? getCVM->getGUIDataByPostRender(mapRend) : nullptr;
			if (mapGui != nullptr)
			{
				cloudCase = mapGui->_cloudCase;
				cloudVar = mapGui->_cloudVarName;
				cloudCompo = mapGui->_cloudCompo;
				cloudFrame = mapGui->_cloudFrame;
			}
		}
		//可编辑下拉工厂：候选+当前文本；存量值不在候选中也插入显示（兼容回填）
		auto makeSlotCombo = [](const QStringList& items, const QString& cur,
			const QString& placeholder)
		{
			QComboBox* cb = new QComboBox();
			cb->setEditable(true);
			cb->addItems(items);
			if (!cur.isEmpty() && cb->findText(cur) < 0)
				cb->addItem(cur);
			cb->setCurrentText(cur);
			if (cb->lineEdit() != nullptr && !placeholder.isEmpty())
				cb->lineEdit()->setPlaceholderText(placeholder);
			return cb;
		};
		//候选刷新（级联用）：重建 items；keep 非法=清空（placeholder 提示按需/可手输）
		auto refreshCombo = [](QComboBox* cb, const QStringList& items, const QString& keep)
		{
			if (cb == nullptr)
				return;
			cb->blockSignals(true);
			cb->clear();
			cb->addItems(items);
			cb->setCurrentText(keep);
			cb->blockSignals(false);
		};
		for (int i = 0; i < _imageSlots.size(); ++i)
		{
			QTableWidgetItem* keyItem = new QTableWidgetItem(_imageSlots.at(i).key);
			keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
			table->setItem(i, 0, keyItem);
			const QString key = _imageSlots.at(i).key;
			//预填=四元组优先→旧 int 形态回退（仅帧维）
			const QJsonObject quad = _config.slotMappings.value(key);
			QString caseText, frameText, varText, compoText;
			if (!quad.isEmpty())
			{
				caseText = quad.value(QStringLiteral("case")).toString();
				varText = quad.value(QStringLiteral("var")).toString();
				compoText = quad.value(QStringLiteral("compo")).toString();
				const int f = quad.value(QStringLiteral("frame")).toInt(-1);
				frameText = f > 0 ? QString::number(f) : QString();
			}
			else
			{
				const int mapped = _config.slotFrameMap.value(key, -1);
				frameText = mapped > 0 ? QString::number(mapped) : QString();
			}
			//S10：无既有映射（四元组/旧 int 形态均无）的新行预填=云图面板当前选中；
			//仍有任何存量值则原样回填不动
			if (caseText.isEmpty() && frameText.isEmpty()
				&& varText.isEmpty() && compoText.isEmpty())
			{
				caseText = cloudCase;
				varText = cloudVar;
				compoText = cloudCompo;
				if (cloudFrame > 0)
					frameText = QString::number(cloudFrame);
			}
			//S10：四格改单元格可编辑下拉（手输兜底保留；帧候选=该工况帧序号序列，
			//非正整数仍走确定时既有校验口径）
			QComboBox* caseCb = makeSlotCombo(slotCaseCandidatesDlg(mapPostDB), caseText,
				QString());
			QComboBox* frameCb = makeSlotCombo(slotFrameCandidatesDlg(mapPostDB, caseText),
				frameText, QStringLiteral("正整数"));
			QComboBox* varCb = makeSlotCombo(slotVarCandidatesDlg(mapPostDB), varText,
				QStringLiteral("留空=仅按帧"));
			QComboBox* compoCb = makeSlotCombo(slotCompoCandidatesDlg(mapPostDB, varText),
				compoText, QStringLiteral("留空=按需"));
			table->setCellWidget(i, 1, caseCb);
			table->setCellWidget(i, 2, frameCb);
			table->setCellWidget(i, 3, varCb);
			table->setCellWidget(i, 4, compoCb);
			//S10 级联（行内，行间互不影响）：工况变化→帧/变量候选刷新（旧值仍合法保留，
			//否则清空交 placeholder 提示）→分量候选随变量刷新；触发=用户弹层选中
			//（activated）或手输提交（editingFinished）——编程回填期不触发，免逐字符重建
			auto refreshRowFromCase = [caseCb, frameCb, varCb, compoCb, mapPostDB, refreshCombo]()
			{
				const QString caseNow = caseCb->currentText().trimmed();
				//帧候选随工况刷新：当前帧值仍在候选则保留，否则清空
				const QStringList frameIds = slotFrameCandidatesDlg(mapPostDB, caseNow);
				refreshCombo(frameCb, frameIds,
					frameIds.contains(frameCb->currentText()) ? frameCb->currentText() : QString());
				//变量候选刷新：旧值仍合法保留，否则清空
				const QStringList vars = slotVarCandidatesDlg(mapPostDB);
				refreshCombo(varCb, vars,
					vars.contains(varCb->currentText()) ? varCb->currentText() : QString());
				//分量候选随（新）变量刷新：旧值仍合法保留，否则清空
				const QStringList compos = slotCompoCandidatesDlg(mapPostDB,
					varCb->currentText().trimmed());
				refreshCombo(compoCb, compos,
					compos.contains(compoCb->currentText()) ? compoCb->currentText() : QString());
			};
			auto refreshRowCompo = [varCb, compoCb, mapPostDB, refreshCombo]()
			{
				const QStringList compos = slotCompoCandidatesDlg(mapPostDB,
					varCb->currentText().trimmed());
				refreshCombo(compoCb, compos,
					compos.contains(compoCb->currentText()) ? compoCb->currentText() : QString());
			};
			connect(caseCb, QOverload<int>::of(&QComboBox::activated), caseCb,
				[refreshRowFromCase](int) { refreshRowFromCase(); });
			if (caseCb->lineEdit() != nullptr)
				connect(caseCb->lineEdit(), &QLineEdit::editingFinished, caseCb, refreshRowFromCase);
			connect(varCb, QOverload<int>::of(&QComboBox::activated), varCb,
				[refreshRowCompo](int) { refreshRowCompo(); });
			if (varCb->lineEdit() != nullptr)
				connect(varCb->lineEdit(), &QLineEdit::editingFinished, varCb, refreshRowCompo);
		}
		layout->addWidget(table);
		auto* btnRow = new QHBoxLayout();
		btnRow->addStretch(1);
		auto* okBtn = new QPushButton(QStringLiteral("确定"), &dlg);
		auto* cancelBtn = new QPushButton(QStringLiteral("取消"), &dlg);
		btnRow->addWidget(okBtn);
		btnRow->addWidget(cancelBtn);
		layout->addLayout(btnRow);
		connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
		connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
		if (dlg.exec() != QDialog::Accepted)
			return;
		//写入配置（四元组）：帧空=移除映射（该槽不参与自动截图）；帧非正整数=提示且该槽
		//不落（E2.2 同精神）；case/var/compo 空=键省略（仅帧维应用语义）。旧 slotFrameMap
		//条目随四元组写入一并移除（消费面=slotMappings 单一权威，grabWithAlign 帧维取法
		//已升级为四元组优先）
		for (int i = 0; i < _imageSlots.size(); ++i)
		{
			const QString key = _imageSlots.at(i).key;
			//S10：四列改 setCellWidget 下拉后取文本统一走 cellWidget（item 兜底=旧路径形状）
			auto mapCellText = [table](int row, int col) -> QString {
				if (QComboBox* cb = qobject_cast<QComboBox*>(table->cellWidget(row, col)))
					return cb->currentText().trimmed();
				return table->item(row, col) != nullptr
					? table->item(row, col)->text().trimmed() : QString();
			};
			const QString frameText = mapCellText(i, 2);
			if (frameText.isEmpty())
			{
				//R3X-03：新建面兑现说明文案承诺（"变量填 SolidColor 时帧可留空=抓当前帧"）——
				//帧空且变量=SolidColor（大小写不敏感）直接落无帧 quad {var:"SolidColor"}，
				//与 Config::load 放行/消费面 frameOnly 抓当前帧同口径（此前按文案新建的条目
				//被静默删除）；R2L-01 保留语义（既有 solidNoFrame quad 往返稳定）并入下方
				//else 分支原样保留；其余情形（无映射/带帧映射被清帧）仍=移除语义不变
				const QString varTextEmpty = mapCellText(i, 3);
				if (varTextEmpty.compare(QStringLiteral("SolidColor"), Qt::CaseInsensitive) == 0)
				{
					//L6-03(a)：非空 case/compo 一并入 solidQuad（此前静默丢弃用户输入）
					QJsonObject solidQuad;
					const QString caseTextEmpty = mapCellText(i, 1);
					const QString compoTextEmpty = mapCellText(i, 4);
					if (!caseTextEmpty.isEmpty())
						solidQuad.insert(QStringLiteral("case"), caseTextEmpty);
					solidQuad.insert(QStringLiteral("var"), varTextEmpty);
					if (!compoTextEmpty.isEmpty())
						solidQuad.insert(QStringLiteral("compo"), compoTextEmpty);
					_config.slotMappings.insert(key, solidQuad);
					_config.slotFrameMap.remove(key);
				}
				else
				{
					//R2L-01：SolidColor 网格图无帧 quad（采纳建议写入形态，与 Config::load 放行
					//同口径）不被"帧空=移除"清掉——打开→确定往返形态稳定（消"采纳映射被编辑器
					//静默删除且不可重建"缺口，LG-01"采纳才落、落了就在"兑现到编辑面）
					const QJsonObject prevQuad = _config.slotMappings.value(key);
					const bool solidNoFrame = !prevQuad.isEmpty()
						&& prevQuad.value(QStringLiteral("frame")).toInt(-1) <= 0
						&& prevQuad.value(QStringLiteral("var")).toString()
							.compare(QStringLiteral("SolidColor"), Qt::CaseInsensitive) == 0;
					if (solidNoFrame && !varTextEmpty.isEmpty())
					{
						//L6-03(b)：用户改了 var 且与存量无帧 quad 冲突→警告提示（不再静默保留
						//旧 quad 使 UI 显示与配置实际不一致；既有 quad 按往返稳定语义保留）
						QMessageBox::warning(this, QStringLiteral("槽位对应设置"),
							QStringLiteral("槽「%1」帧留空：既有 SolidColor 无帧映射保留，本次输入的变量 \"%2\" 未应用（如需按新变量映射请填写正整数帧号）")
								.arg(key, varTextEmpty));
					}
					if (!solidNoFrame)
					{
						_config.slotMappings.remove(key);
						_config.slotFrameMap.remove(key);
					}
				}
				continue;
			}
			bool numOk = false;
			const int frameId = frameText.toInt(&numOk);
			if (!numOk || frameId <= 0)
			{
				QMessageBox::warning(this, QStringLiteral("槽位对应设置"),
					QStringLiteral("槽「%1」帧应为正整数（当前输入：\"%2\"），该槽对应未保存")
						.arg(key, frameText));
				continue;
			}
			QJsonObject quad;
			const QString caseText = mapCellText(i, 1);
			const QString varText = mapCellText(i, 3);
			const QString compoText = mapCellText(i, 4);
			if (!caseText.isEmpty())
				quad.insert(QStringLiteral("case"), caseText);
			quad.insert(QStringLiteral("frame"), frameId);
			if (!varText.isEmpty())
				quad.insert(QStringLiteral("var"), varText);
			if (!compoText.isEmpty())
				quad.insert(QStringLiteral("compo"), compoText);
			_config.slotMappings.insert(key, quad);
			_config.slotFrameMap.remove(key);
		}
		refreshCaptureRows();   //Tab0 截图槽列表映射列随编辑刷新
		//映射随项目配置立即落盘（D-12：防丢）
		QString saveErr;
		if (!_config.save(saveErr) && !saveErr.isEmpty())
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(saveErr), true);
	}

	//——三期批次 A：Tab0 一键导出主视图动作面（FR-341/342/343，13 §2.1 六态合并）——
	void mStrengthReportDlg::slot_oneClickExport()
	{
		//Tab0 主按钮=既有 slot_export 的包装入口：S1 装配前置（S2 就绪后）→S3 自动截图
		//（三期批次 C 交付，开关默认关 Q3-3=本批直跳）→既有导出全链（缺项预检/覆盖键/
		//数据集装配/引擎子进程零改动复用）。B-6 回归锚：对话框底部既有[一键导出]按钮与
		//slot_export 状态机保留不动=同一入口；E3-9：本链零依赖 Tab0（按钮不可达时四页签
		//手动模式照常导出）
		if (_exporting)
			return;
		switchProjectConfigIfNeeded();   //CB-01：与既有按钮同首步（导出归属当前项目）
		if (_loadState != Loaded)
		{
			const QString msg = QStringLiteral("规则模板未加载——请先在顶部加载规则模板文档(.rtdoc)");
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return;
		}
		if (_probe->isRunning())
		{
			//CB-04：校验中禁用语义同既有按钮（主按钮此时本已禁用，防御）。
			//PA-15：静默 return 补一条消息，不留"可点无反馈"死点
			const QString msg = QStringLiteral("规则模板检查进行中，请等待检查完成后再导出");
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return;
		}
		if (!_assemblyReady)
		{
			startAutoAssembly();   //防御：S1 未完（主按钮装配期禁用，理论不可达）——装配完成后再导出
			return;
		}
		//三期批次 C（13 §3 批次 C slot_oneClickExport 行）：S3 截图段——autoCapture 开且映射
		//非空→AutoCapture 批量链（成功槽入 imagePaths→数据集装配消费）；开关关=完全不进入
		//截图环节（行为与三期批次 B 逐字节一致——Q3-3 回归锚）。插入点=数据集装配之前
		//（slot_export 内 buildDataset 消费 imagePaths）；12 §2.1 S3 态：逐槽进度刷新+
		//"请勿操作 3D 视口"提示；逐槽失败留空绝不阻塞导出（E3-6）
		//LG-02/LG-03（UB-01/UB-02）：批量段已并入导出互斥态（runAutoCaptureBatch 内
		//setExporting(true)）——批量期间页签/底部导出/行内抓取全部禁用，重入面闭合；
		//批量段经二次确认取消→整链终止（不进导出段、不在隐藏对话框下续跑引擎）；
		//批量正常收尾→瞬时解除互斥（期间无事件派发，对用户原子）后由 slot_export
		//自行重新进入导出态（slot_export 既有闸/收尾语义零改动）
		const bool batchEntered = runAutoCaptureBatch();
		if (batchEntered && _oneClickCancelled)
		{
			_oneClickCancelled = false;
			_captureBatchRan = false;
			//R2L-03：取消终态覆盖批量收尾提示行（中止槽≠失败——批量链收尾恒写"完成：
			//成功 X·失败 Y"，中止槽被计为失败且"原因见导出汇总"指引在整链终止后失效）
			//R4X-02：中止（Aborted）槽按"取消=保留现状"语义保留既有旧图（FR-221/R2L-03
			//同源——取消场景无新图产生，不存在旧图顶替新图风险面），文案同步披露
			int keptCount = 0;
			int abortKeepCount = 0;
			for (const CaptureSlotResult& r : _lastCaptureResults)
			{
				if (r.ok)
					++keptCount;
				else if (!r.skipped && r.reasonCode == QStringLiteral("Aborted"))
					++abortKeepCount;
			}
			//R4X-03：丢弃（R3X-02 项目切换）与取消组合时，提示行让位主因（跨项目丢弃），
			//中止留用文案不得以"成功 0 槽已保留"掩盖丢弃事实（结果面板两条消息齐备）
			const bool discarded = _captureBatchDiscarded;
			_captureBatchDiscarded = false;
			if (_autoPreview != nullptr)
			{
				const QString hint = discarded
					? QStringLiteral("<span style='color:#B8860B;'>自动截图已取消：批量截图期间项目已切换，"
						"本次截图结果已丢弃，本次导出未执行。</span>")
					: QStringLiteral("<span style='color:#B8860B;'>自动截图已取消：成功 %1 槽已保留，"
						"中止 %2 槽保留原状——本次导出未执行。</span>").arg(keptCount).arg(abortKeepCount);
				_autoPreview->setCaptureHint(hint);
			}
			setExporting(false);
			const QString msg = QStringLiteral("自动截图段已取消，本次导出未执行");
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return;
		}
		if (batchEntered)
			setExporting(false);
		slot_export();   //S4 数据集装配→S5 引擎→S6 完成（既有全链零改动复用）
	}

	//——三期批次 C：全自动批量截图段（FR-331/332/334，13 §3 批次 C/§4.4 调用序）——
	//LG-02：返回 true=批量段已实际进入（调用方据此处置互斥态/取消链）
	bool mStrengthReportDlg::runAutoCaptureBatch()
	{
		_captureBatchRan = false;
		_captureBatchDiscarded = false;   //R4X-03：组合窗标记逐轮复位
		_lastCaptureResults.clear();
		//开关关=完全不进入截图环节（12 §2.1 S3 门；行为与三期批次 B 逐字节一致——Q3-3）
		if (!_config.autoCapture || _imageSlots.isEmpty())
			return false;
		collectUiToConfig();   //水印开关/映射当前态（与 slot_export 同口径）
		QStringList keys;
		for (const ImageSlot& slot : _imageSlots)
			keys.append(slot.key);
		QMap<QString, CaptureMapping> mappings =
			mStrengthReportAutoCapture::mappingsFromConfig(&_config, keys);
		if (mappings.isEmpty())
		{
			//D-07：开关开而映射空=先尝试逐槽自动采纳 Tab0 截图建议（采纳语义与手动
			//[采纳建议]逐行一致；插入点在 collectUiToConfig 之后——其不触碰
			//slotMappings，采纳写入无被回读覆盖风险）；采纳成功=按新映射继续原批量流程
			const int adopted = adoptAllCaptureSuggestions();
			if (adopted > 0)
			{
				mappings = mStrengthReportAutoCapture::mappingsFromConfig(&_config, keys);
			}
			if (mappings.isEmpty())
			{
				//建议也为空（或无建议可采纳）=直跳 S4（12 §2.1 S3 门"映射非空"）：
				//D-07 升级为弹窗提示（文案保留原意+指引），弹窗后照常继续导出
				//（不含截图批量，图片槽保持现状——既有语义不变）
				const QString msg = QStringLiteral("自动截图已开启但未设置槽位对应，且无可用截图建议"
					"（装配未就绪或槽键无匹配建议）。\n"
					"本次导出不含截图批量，图片槽保持现状；"
					"请到页签③[对应设置…]逐槽配置后重试。");
				QMessageBox::information(this, QStringLiteral("自动截图"), msg);
				_resultMessages->appendPlainText(msg);
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
					QStringLiteral("[强度报告]%1").arg(msg).replace(
						QStringLiteral("\n"), QStringLiteral(" ")), true);
				return false;
			}
			//状态披露：Tab0 灰字提示行+结果面板+主窗口输出（三面同文案口径）
			const QString msg = QStringLiteral("已自动采纳 %1 条截图建议（映射为空自动补齐）").arg(adopted);
			if (_autoPreview != nullptr)
				_autoPreview->setCaptureHint(QStringLiteral(
					"<span style='color:#606060;'>%1</span>").arg(msg));
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
		}
		const bool watermarkOff = _watermarkOffChk->isChecked();
		//R3X-02：批量段嵌套事件循环（等待/旁证轮询/防抖）可派发视口切换信号→
		//slot_viewportChanged→abortBatch+switchProjectConfigIfNeeded 已把 B 项目配置
		//载入 _config——收尾回填前快照比对项目代币，不一致=丢弃全部截图结果（A 项目
		//抓的图不得写入 B 项目内存配置再经 save/导出落盘固化——CB-01/CB-17 跨项目
		//隔离口径与截图子目录 <项目标识>/<槽键> 隔离同源）；返回 true（批量段确已
		//进入，调用方解除互斥后 slot_export 按 NotLoaded 态拒绝并提示）
		const QString batchProjectId = _config.projectId();
		_captureBatchRan = true;
		//LG-02/LG-03（UB-01/UB-02/UB-06）：S3 段并入导出互斥态——批量链在嵌套事件循环中
		//派发用户输入，此前 _exporting=false 期间底部[一键导出]/页签③[抓取]/[导入模板]/
		//映射编辑等全部可重入（同屏两链抢视口、截图结果与产物脱节、关闭无确认续跑）。
		//提前置导出态：页签/顶部入口/底部按钮禁用（slot_grabImage/slot_adoptCaptureSuggestion
		//等既有 _exporting 闸随之生效），[取消]按钮转可用（S3 段取消通道，LG-03）
		setExporting(true);
		if (_autoPreview != nullptr)
			_autoPreview->enterCapturePhase();   //LG-08：S3 段落 phase 呈现（12 §2.1 六态图对齐）
		//S3 态提示（12 §2.1/O3-6："自动截图进行中，请勿操作 3D 视口"）+批量逐槽进度
		if (_autoPreview != nullptr)
			_autoPreview->setCaptureHint(QStringLiteral(
				"<span style='color:#606060;'>自动截图进行中，请勿操作 3D 视口…（逐槽失败留空，不阻塞导出）</span>"));
		QApplication::processEvents();   //让 S3 提示先呈现（批量链首槽前）
		_lastCaptureResults = _autoCapture->runBatch(mappings, keys, watermarkOff, this,
			[this](int done, int total, const QString& key)
			{
				if (_autoPreview != nullptr)
					_autoPreview->setCaptureHint(QStringLiteral(
						"<span style='color:#606060;'>自动截图进行中（%1/%2）：槽「%3」…"
						"请勿操作 3D 视口</span>").arg(done).arg(total).arg(key));
			});
		//R3X-02：项目代币比对（_config.projectId 已被切项目链改为新项目=批量结果属旧项目）
		if (_config.projectId() != batchProjectId)
		{
			_lastCaptureResults.clear();
			_captureBatchRan = false;
			_captureBatchDiscarded = true;   //R4X-03：供取消终态提示行让位主因措辞
			if (_autoPreview != nullptr)
				_autoPreview->setCaptureHint(QStringLiteral(
					"<span style='color:#B8860B;'>批量截图期间项目已切换，本次截图结果已丢弃"
					"（防跨项目污染），本次导出未执行。</span>"));
			const QString msg = QStringLiteral("批量截图期间项目已切换，本次截图结果已丢弃（防跨项目污染），本次导出未执行");
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return true;
		}
		//成功槽回填：路径入配置（slot_export 内 buildDataset 消费 imagePaths——与手动抓取
		//同一数据面）+页签③缩略图回显（可疑=⚠ 标记——O3-5 提示性不阻断）
		int okCount = 0;
		int suspectCount = 0;
		for (const CaptureSlotResult& r : _lastCaptureResults)
		{
			if (!r.ok)
				continue;
			++okCount;
			if (r.suspect)
				++suspectCount;
			_config.imagePaths.insert(r.slotKey, r.path);
			_config.imageWatermarks.insert(r.slotKey, watermarkOff);   //Q2-4 逐槽追溯
			//R3X-06：批量 C 成功槽以 Tab0 提示行+⚠ 可疑标记承担核对提示，移出试点
			//"待确认"计数面——批量新图已替换试点旧图，"待确认槽 N 处"不再指向旧截图
			_pendingConfirmKeys.remove(r.slotKey);
			for (int row = 0; row < _imageSlots.size(); ++row)
			{
				if (_imageSlots.at(row).key != r.slotKey)
					continue;
				QTableWidgetItem* item = readOnlyItem(r.suspect
					? QStringLiteral("已抓取·⚠ 可疑（自动）") : QStringLiteral("已抓取（自动）"));
				item->setIcon(QIcon(r.path));
				item->setToolTip(r.path + QStringLiteral("\n截图档：") + (watermarkOff
					? QStringLiteral("无水印（交付版）") : QStringLiteral("带水印"))
					+ (r.suspect ? QStringLiteral("\n⚠ 近单色画面，请人工核对") : QString()));
				_imageTable->setItem(row, 1, item);
				break;
			}
		}
		//R3X-01：本轮截图失败=该槽本次无效——旧图（上次导出/试点抓取/手动导入残留）不得
		//顶替缺项策略（逐槽 reason 承诺"该槽留空（缺项策略）"）：有映射且失败的槽覆盖
		//内存配置 imagePaths 条目为空（R4X-01 措辞对齐：_config 即持久化对象，该覆盖
		//随 slot_export 收尾/切项目 save 链落盘，非纯会话内）→buildDataset 不发射该槽→
		//按槽级 missing_policy 缺项（缺项预检/block_export 阻断同步生效，过期帧/变量态
		//旧画面不入交付件）；未映射跳过槽="保持现状"语义不变（12 §2.5 映射驱动口径）
		//R4X-02：取消通道（Aborted=用户[取消]/关闭/Esc 中止后未尝试槽）除外——按
		//R2L-03/FR-221"取消=保留现状"语义保留既有旧图原状（不进本轮"按缺项策略"清空
		//口径，不产生新图，不存在旧图顶替风险面）；真实失败码（超时/单色/映射失配/
		//视口关闭等）仍清空走缺项策略
		for (const CaptureSlotResult& r : _lastCaptureResults)
		{
			if (r.ok || r.skipped || !mappings.contains(r.slotKey))
				continue;
			if (r.reasonCode == QStringLiteral("Aborted"))
				continue;   //R4X-02：取消留用（槽保持原状，页签③呈现不改动）
			_config.imagePaths.insert(r.slotKey, QString());
			//R4X-05：失败清空=该槽已无图无可确认——同步移出试点"待确认"计数面（与
			//R3X-06 成功槽移出对称、B-204 清除分支同款），防"待确认槽 N 处"提示指向无图槽
			_pendingConfirmKeys.remove(r.slotKey);
			for (int row = 0; row < _imageSlots.size(); ++row)
			{
				if (_imageSlots.at(row).key != r.slotKey)
					continue;
				QTableWidgetItem* item = readOnlyItem(QStringLiteral("⚠ 截图失败（按缺项策略）"));
				item->setToolTip(r.reason);
				_imageTable->setItem(row, 1, item);
				break;
			}
		}
		//Tab0 提示行终态（⚠ 分级：失败/可疑黄字提示性不阻断——FR-343 同口径）
		//R4X-02：中止（Aborted）槽不入失败桶（旧图已保留原状），单列"中止保留原状"
		int failCount = 0;
		int skipCount = 0;
		int abortCount = 0;
		for (const CaptureSlotResult& r : _lastCaptureResults)
		{
			if (r.skipped) ++skipCount;
			else if (!r.ok && r.reasonCode == QStringLiteral("Aborted")) ++abortCount;
			else if (!r.ok) ++failCount;
		}
		if (_autoPreview != nullptr)
		{
			const QString tone = (failCount > 0 || suspectCount > 0)
				? QStringLiteral("#B8860B") : QStringLiteral("#008000");
			QString hint = QStringLiteral(
				"<span style='color:%1;'>自动截图完成：成功 %2 槽（可疑 %3）·失败 %4 槽·"
				"未设置对应跳过 %5 槽").arg(tone).arg(okCount).arg(suspectCount).arg(failCount).arg(skipCount);
			if (abortCount > 0)
				hint += QStringLiteral("·中止保留原状 %1 槽").arg(abortCount);
			hint += QStringLiteral("——失败原因与可疑提示见导出完成后的汇总信息。</span>");
			_autoPreview->setCaptureHint(hint);
		}
		return true;
	}

	//——三期批次 C：Tab0 截图槽列表（FR-333 建议列，12 §2.5 五槽建议口径）——
	void mStrengthReportDlg::refreshCaptureRows()
	{
		if (_autoPreview == nullptr)
			return;
		const bool switchOn = _semiAutoChk != nullptr && _semiAutoChk->isChecked();
		//LG-07：load 丢弃的非法映射计数黄字呈现（消"映射消失零线索"静默——E2.2 同精神；
		//计数随下次 load 重置）
		QString hintHtml;
		if (_config.lastMappingDroppedCount > 0)
			hintHtml += QStringLiteral(
				"<br><span style='color:#B8860B;'>⚠ 项目配置中 %1 条非法槽位对应已丢弃"
				"（帧号非法；可在[对应设置…]重新配置）</span>")
				.arg(_config.lastMappingDroppedCount);
		if (_autoPreview != nullptr)
		{
			_autoPreview->setCaptureHint(hintHtml + (switchOn ? QStringLiteral(
				"自动截图：已开启——一键导出将按下方对应设置逐槽批量截图"
				"（单槽失败留空走缺项策略，不阻塞导出）；逐槽对应编辑请用[对应设置…→页签③]。")
				: QStringLiteral("自动截图：开关关闭。开启与逐槽对应请在「截图槽管理」页签设置"
						"（页签③\"自动截图\"开关+[对应设置…]）；开启后一键导出将按对应设置批量截图。")));
		}
		QList<mStrengthReportAutoPreview::CaptureSlotRow> rows;
		if (_imageSlots.isEmpty())
		{
			_autoPreview->setCaptureSlots(rows);
			return;
		}
		//建议源（FR-333"建议·来自自动扫描"）：批次 B 峰值帧号副产品同源（_autoAssembly.cases
		//的 stressPeakFrameId/dispMaxFrameId）+automation 节变量映射（T3-1 通道产物）；
		//装配未就绪=无建议（防陈旧装配缓存串建议——CB-01 隔离口径）
		const AutomationConfig automation = AutoAssembler::parseAutomation(_automationNode);
		QString stressCase;
		int stressFrame = -1;
		QString dispCase;
		int dispFrame = -1;
		double stressBest = 0.0;
		double dispBest = 0.0;
		bool haveStress = false;
		bool haveDisp = false;
		if (_assemblyReady)
		{
			for (const AssemblyResult::CaseScan& scan : _autoAssembly.cases)
			{
				if (scan.stressPeak.value.isDouble()
					&& (!haveStress || scan.stressPeak.value.toDouble() > stressBest))
				{
					stressBest = scan.stressPeak.value.toDouble();
					stressCase = scan.caseName;
					stressFrame = scan.stressPeakFrameId;
					haveStress = true;
				}
				if (scan.dispMax.value.isDouble()
					&& (!haveDisp || scan.dispMax.value.toDouble() > dispBest))
				{
					dispBest = scan.dispMax.value.toDouble();
					dispCase = scan.caseName;
					dispFrame = scan.dispMaxFrameId;
					haveDisp = true;
				}
			}
		}
		for (const ImageSlot& slot : _imageSlots)
		{
			mStrengthReportAutoPreview::CaptureSlotRow row;
			row.key = slot.key;
			//映射列：四元组文本（旧 int 形态=仅帧维标注，R3-9）
			const QJsonObject quad = _config.slotMappings.value(slot.key);
			if (!quad.isEmpty())
			{
				QStringList parts;
				const QString caseText = quad.value(QStringLiteral("case")).toString();
				const int f = quad.value(QStringLiteral("frame")).toInt(-1);
				const QString varText = quad.value(QStringLiteral("var")).toString();
				const QString compoText = quad.value(QStringLiteral("compo")).toString();
				if (!caseText.isEmpty())
					parts.append(caseText);
				if (f > 0)
					parts.append(QStringLiteral("帧%1").arg(f));
				if (!varText.isEmpty())
					parts.append(compoText.isEmpty() ? varText
						: QStringLiteral("%1/%2").arg(varText, compoText));
				row.mappingText = parts.isEmpty() ? QStringLiteral("-")
					: parts.join(QStringLiteral("·"));
			}
			else
			{
				const int legacy = _config.slotFrameMap.value(slot.key, -1);
				row.mappingText = legacy > 0
					? QStringLiteral("帧%1（旧版仅设帧号）").arg(legacy) : QStringLiteral("-");
			}
			//建议列（12 §2.5：网格图=SolidColor+当前帧；失稳=映射工况+指定阶次；
			//位移=位移最大值帧；应力=应力峰值帧——槽键关键词匹配，不匹配=无建议）
			QString sug;
			const QString lk = slot.key.toLower();
			if (lk.contains(QStringLiteral("mesh")) || lk.contains(QStringLiteral("boundary"))
				|| slot.key.contains(QStringLiteral("网格")))
			{
				sug = QStringLiteral("SolidColor·当前帧（网格图）");
			}
			else if ((lk.contains(QStringLiteral("buckle")) || slot.key.contains(QStringLiteral("失稳")))
				&& !automation.bucklingMap.isEmpty())
			{
				//首条失稳映射（工况→阶次）→阶次对应帧号（getCaseFrameNamesAndIDs 只读枚举）
				auto buckIt = automation.bucklingMap.constBegin();
				if (buckIt != automation.bucklingMap.constEnd())
				{
					QPointer<mPostDataBase> postDB = currentPostDbDlg();
					if (postDB != nullptr)
					{
						std::vector<QPair<QString, int>> frames;
						postDB->getCaseFrameNamesAndIDs(buckIt.key(), frames);
						const int idx = buckIt.value() - 1;
						if (idx >= 0 && idx < frames.size())
							sug = QStringLiteral("%1·%2阶（帧%3）")
								.arg(buckIt.key()).arg(buckIt.value()).arg(frames.at(idx).second);
					}
				}
			}
			else if ((lk.contains(QStringLiteral("disp")) || slot.key.contains(QStringLiteral("位移")))
				&& haveDisp && automation.displacement.valid
				&& !automation.displacement.names.isEmpty())
			{
				sug = QStringLiteral("%1·帧%2·%3(%4)").arg(dispCase).arg(dispFrame)
					.arg(automation.displacement.names.first(),
						automation.displacement.compo);
			}
			else if ((lk.contains(QStringLiteral("stress")) || slot.key.contains(QStringLiteral("应力")))
				&& haveStress && automation.stressPeak.valid
				&& !automation.stressPeak.names.isEmpty())
			{
				sug = QStringLiteral("%1·帧%2·%3(%4)").arg(stressCase).arg(stressFrame)
					.arg(automation.stressPeak.names.first(), automation.stressPeak.compo);
			}
			row.suggestionText = sug;
			rows.append(row);
		}
		_autoPreview->setCaptureSlots(rows);
	}

	int mStrengthReportDlg::adoptAllCaptureSuggestions()
	{
		//D-07：映射为空时逐槽自动采纳截图建议。采纳路径复用 slot_adoptCaptureSuggestion
		//（写 _config.slotMappings+移除旧 slotFrameMap 条目+save+refreshCaptureRows——与
		//手动点[采纳建议]逐行完全一致语义，不复制建议构建/写入逻辑）；无建议可采纳的槽
		//该函数内部直接 return（不落映射），以采纳后映射计数为准
		int adopted = 0;
		if (_autoPreview == nullptr)
			return 0;
		for (const ImageSlot& slot : _imageSlots)
		{
			if (!_config.slotMappings.value(slot.key).isEmpty())
				continue;   //已有映射的槽不覆盖（仅补空）
			slot_adoptCaptureSuggestion(slot.key);
			if (!_config.slotMappings.value(slot.key).isEmpty())
				++adopted;
		}
		return adopted;
	}

	void mStrengthReportDlg::slot_adoptCaptureSuggestion(const QString& slotKey)
	{
		//采纳才落 slotMappings（建议≠配置——FR-333/12 §2.5；采纳后随项目配置落盘 D-12）
		if (_exporting)
			return;
		const AutomationConfig automation = AutoAssembler::parseAutomation(_automationNode);
		QJsonObject quad;
		const QString lk = slotKey.toLower();
		if (lk.contains(QStringLiteral("mesh")) || lk.contains(QStringLiteral("boundary"))
			|| slotKey.contains(QStringLiteral("网格")))
		{
			quad.insert(QStringLiteral("var"), QStringLiteral("SolidColor"));   //帧维=当前帧不切
		}
		else if ((lk.contains(QStringLiteral("buckle")) || slotKey.contains(QStringLiteral("失稳")))
			&& !automation.bucklingMap.isEmpty())
		{
			auto buckIt = automation.bucklingMap.constBegin();
			if (buckIt == automation.bucklingMap.constEnd())
				return;
			QPointer<mPostDataBase> postDB = currentPostDbDlg();
			if (postDB == nullptr)
				return;
			std::vector<QPair<QString, int>> frames;
			postDB->getCaseFrameNamesAndIDs(buckIt.key(), frames);
			const int idx = buckIt.value() - 1;
			if (idx < 0 || idx >= static_cast<int>(frames.size()))
				return;
			quad.insert(QStringLiteral("case"), buckIt.key());
			quad.insert(QStringLiteral("frame"), frames.at(idx).second);
		}
		else if ((lk.contains(QStringLiteral("disp")) || slotKey.contains(QStringLiteral("位移")))
			&& automation.displacement.valid && !automation.displacement.names.isEmpty())
		{
			//位移建议=位移最大值帧（_autoAssembly 副产品同源）
			for (const AssemblyResult::CaseScan& scan : _autoAssembly.cases)
			{
				if (scan.dispMaxFrameId > 0)
				{
					quad.insert(QStringLiteral("case"), scan.caseName);
					quad.insert(QStringLiteral("frame"), scan.dispMaxFrameId);
					quad.insert(QStringLiteral("var"), automation.displacement.names.first());
					if (!automation.displacement.compo.isEmpty())
						quad.insert(QStringLiteral("compo"), automation.displacement.compo);
					break;
				}
			}
			if (quad.isEmpty())
				return;
		}
		else if ((lk.contains(QStringLiteral("stress")) || slotKey.contains(QStringLiteral("应力")))
			&& automation.stressPeak.valid && !automation.stressPeak.names.isEmpty())
		{
			//应力建议=应力峰值帧（FR-333：最大应力槽=应力峰值帧，副产品结构注释锁死）
			for (const AssemblyResult::CaseScan& scan : _autoAssembly.cases)
			{
				if (scan.stressPeakFrameId > 0)
				{
					quad.insert(QStringLiteral("case"), scan.caseName);
					quad.insert(QStringLiteral("frame"), scan.stressPeakFrameId);
					quad.insert(QStringLiteral("var"), automation.stressPeak.names.first());
					if (!automation.stressPeak.compo.isEmpty())
						quad.insert(QStringLiteral("compo"), automation.stressPeak.compo);
					break;
				}
			}
			if (quad.isEmpty())
				return;
		}
		else
		{
			return;   //无建议可采纳（按钮态已禁用，防御）
		}
		_config.slotMappings.insert(slotKey, quad);
		_config.slotFrameMap.remove(slotKey);
		QString saveErr;
		if (!_config.save(saveErr) && !saveErr.isEmpty())
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(saveErr), true);
		refreshCaptureRows();
	}

	void mStrengthReportDlg::slot_clearCaptureMapping(const QString& slotKey)
	{
		//清除=回退手动（该槽不参与自动截图；保留既有截图路径不动）
		if (_exporting)
			return;
		_config.slotMappings.remove(slotKey);
		_config.slotFrameMap.remove(slotKey);
		QString saveErr;
		if (!_config.save(saveErr) && !saveErr.isEmpty())
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(saveErr), true);
		refreshCaptureRows();
	}

	void mStrengthReportDlg::slot_jumpToControl(int role)
	{
		//12 §3.1"○可覆盖 点击→跳对应页签定位控件"：预览只读（消双真源），覆盖仍在页签
		//原控件进行（页签索引：0=Tab0，1~4=页签①~④——Tab0 前插仅索引位移）
		switch (role)
		{
		case mStrengthReportAutoPreview::JumpSoftwareName:   //软件名→页签①
			_tabs->setCurrentIndex(1);
			_softwareNameEdit->setFocus();
			_softwareNameEdit->selectAll();
			break;
		case mStrengthReportAutoPreview::JumpProjectSubject: //标题/对象名→页签①对象名
			_tabs->setCurrentIndex(1);
			_projectSubjectEdit->setFocus();
			_projectSubjectEdit->selectAll();
			break;
		case mStrengthReportAutoPreview::JumpExportTab:      //缺项策略/输出/水印→页签④
			_tabs->setCurrentIndex(4);
			break;
		//三期批次 B（FR-311/312/313）：单元类型+默认句四键→页签①对应录入框
		case mStrengthReportAutoPreview::JumpElementType:
			_tabs->setCurrentIndex(1);
			_elementTypeEdit->setFocus();
			_elementTypeEdit->selectAll();
			break;
		case mStrengthReportAutoPreview::JumpConstraintDesc:
			_tabs->setCurrentIndex(1);
			_constraintDescEdit->setFocus();
			_constraintDescEdit->selectAll();
			break;
		case mStrengthReportAutoPreview::JumpLoadCaseDesc:
			_tabs->setCurrentIndex(1);
			_loadCaseDescEdit->setFocus();
			_loadCaseDescEdit->selectAll();
			break;
		case mStrengthReportAutoPreview::JumpLoadType:
			_tabs->setCurrentIndex(1);
			_loadTypeEdit->setFocus();
			_loadTypeEdit->selectAll();
			break;
		case mStrengthReportAutoPreview::JumpCalcMethod:
			_tabs->setCurrentIndex(1);
			_calcMethodEdit->setFocus();
			_calcMethodEdit->selectAll();
			break;
		default:
			break;
		}
	}

	void mStrengthReportDlg::slot_importImage(int row)
	{
		if (_exporting || row < 0 || row >= _imageSlots.size())
			return;
		switchProjectConfigIfNeeded();   //CB-01：导入路径随当前项目配置
		if (_exporting || row < 0 || row >= _imageSlots.size())
			return;
		const QString file = QFileDialog::getOpenFileName(this,
				QStringLiteral("导入外部图片"), QString(),
			QStringLiteral("图片 (*.png *.jpg *.jpeg *.bmp)"));
		if (file.isEmpty())
			return;
		const QString key = _imageSlots.at(row).key;
		_config.imagePaths.insert(key, file);
		_pendingConfirmKeys.remove(key);   //B-204：导入替换=确认语义，移出待确认计数
		QTableWidgetItem* item = readOnlyItem(QStringLiteral("已导入"));
		item->setIcon(QIcon(file));
		item->setToolTip(file);
		_imageTable->setItem(row, 1, item);
	}

	void mStrengthReportDlg::slot_clearImage(int row)
	{
		if (_exporting || row < 0 || row >= _imageSlots.size())
			return;
		const QString key = _imageSlots.at(row).key;
		_config.imagePaths.insert(key, QString());
		_pendingConfirmKeys.remove(key);   //B-204：清除=移出待确认计数（槽已无图）
		_imageTable->setItem(row, 1, readOnlyItem(QStringLiteral("⚠ 缺图")));
	}

	void mStrengthReportDlg::slot_saveConfig()
	{
		switchProjectConfigIfNeeded();   //CB-01：保存永远落"当前项目"的配置文件（D-12）
		//FR-45：保存内容=规则模板包路径+元信息+全部人工补录值+输出路径记忆（+页签③槽图路径）
		collectUiToConfig();
		QString err;
		if (_config.save(err))
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
				QStringLiteral("[强度报告]项目配置已保存：%1").arg(_config.configFilePath()), true);
		}
		else
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
				QStringLiteral("[强度报告]%1").arg(err), true);
		}
	}

	void mStrengthReportDlg::slot_missingPrecheck()
	{
		switchProjectConfigIfNeeded();   //CB-01：预检随当前项目态
		collectUiToConfig();
		_missingCount = countMissingItems();
		QStringList detail;
		if (_elementTypeEdit->text().trimmed().isEmpty())
			detail.append(QStringLiteral("单元类型未录入（必填项，请补录）"));
		if (_projectSubjectEdit->text().trimmed().isEmpty())
			detail.append(QStringLiteral("对象名未录入（导出件保留 XXX 原文并告警）"));
		for (const ImageSlot& slot : _imageSlots)
		{
			const QString path = _config.imagePaths.value(slot.key);
			if (path.isEmpty() || !QFile::exists(path))
				detail.append(QStringLiteral("图片槽缺图：%1（按缺项策略处理；云图A 允许缺项）").arg(slot.key));
		}
		if (detail.isEmpty())
		{
			_precheckLabel->setText(QStringLiteral("<span style='color:#008000;'>无缺项</span>"));
		}
		else
		{
			_precheckLabel->setText(QStringLiteral("<span style='color:#B8860B;'>缺项 %1 项：%2</span>")
				.arg(detail.size()).arg(detail.join(QStringLiteral("；"))));
		}
		if (_loadState == Loaded)
			applyLoadState(_missingCount > 0
				? QStringLiteral("<span style='color:#B8860B;'>已加载·有缺项 %1 项 · 模板\"%2\"</span>")
					.arg(_missingCount).arg(_templateName)
				: QStringLiteral("<span style='color:#008000;'>已加载·检查通过 · 模板\"%1\"</span>")
					.arg(_templateName));
	}

	void mStrengthReportDlg::setExporting(bool exporting)
	{
		_exporting = exporting;
		_tabs->setEnabled(!exporting);                 //导出中：全页签禁用（02 §3.4）
		_containerPathEdit->setEnabled(!exporting);
		_browseContainerBtn->setEnabled(!exporting);
		_importTemplateBtn->setEnabled(!exporting);   //二期批次 D：导出中向导入口同禁用
		_revalidateBtnTop->setEnabled(!exporting);
		_revalidateBtnBottom->setEnabled(!exporting);
	_saveConfigBtn->setEnabled(!exporting);
	_cancelExportBtn->setEnabled(exporting);       //仅[取消]可用
	//R3P-01：导出收尾/启动的底部钮使能随单点刷新（判据=Loaded+非导出+非校验+装配就绪；
	//导出中本行恒禁用——_exporting 已先行置位）
	updateAutoPreviewUi();
		if (_autoPreview != nullptr)
			_autoPreview->setExporting(exporting);     //三期批次 A：Tab0 子状态机与导出链正交同步
		if (!exporting)
		{
			_progressBar->setValue(0);
			_stageLabel->clear();
		}
	}

	void mStrengthReportDlg::slot_export()
	{
		switchProjectConfigIfNeeded();   //CB-01：导出数据集必须装配"当前项目"的配置（防 A 字段串 B）
		if (_exporting)
			return;
		if (_loadState != Loaded)
		{
			//R3X-02 附带：批量段项目切换后一键导出链走到此分支此前为静默 return——
			//用户只见"自动截图完成"提示而无导出，无线索面；补消息收敛
			const QString msg = QStringLiteral("规则模板未加载——导出未执行，请先在顶部加载当前项目的规则模板文档(.rtdoc)");
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return;
		}
		//CB-04：校验进行中拒绝启动导出——Loaded 态可能是上一容器的陈旧态，且
		//导出重写数据集文件会与校验器读文件竞态（现两路径已分文件，此处为状态机互斥）
		if (_probe->isRunning())
		{
			const QString msg = QStringLiteral("规则模板检查进行中，请等待检查完成后再导出");
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return;
		}
		//PA-02：装配就绪门（S1 装配期/会话未建拒绝导出）——装配期放行会走 2 参合并口
		//=发射形状静默退回二期（meta/自动值全 null 无提示），且 runAutoAssemblyStep 被
		//_exporting 短路后 _assemblySession 残留、Tab0 永滞"正在自动装配…"死滞态。
		//按钮使能已按 _assemblyReady 收口（applyLoadState/setExporting），此处为入口
		//防御+口径落字；装配会话由 probe 完成/视口链自动触发，无需在此重排
		if (!_assemblyReady)
		{
			const QString msg = QStringLiteral(
				"自动取值尚未完成，请等待取值完成后导出（可在「一键导出」页签查看取值进度）");
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return;
		}
		if (!ensureOutputPath())
			return;
		collectUiToConfig();

		//缺项预检（02 §3.3：存在缺项按默认策略照常导出；策略=阻断导出时 GUI 拒绝启动）
		_missingCount = countMissingItems();
		const QString policy = _missingPolicyCombo->currentData().toString();
		if (_missingCount > 0 && policy == QStringLiteral("block_export"))
		{
			const QString msg = QStringLiteral("缺项预检发现 %1 项缺项，当前缺项策略=阻断导出，"
				"请补齐后在页签④重试").arg(_missingCount);
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return;
		}

		//数据集装配（FR-41+二期 FR-241：results=页签②录入值或 null，材料通道/verdict/
		//覆盖键随数据集发射；A-08 ok/err 语义——err 仅作告警汇总不阻断）。
		//三期批次 B 装配合并口（13 §3 Collector 行）：装配就绪时传入装配结果——buildDataset
		//内部按"录入＞自动＞默认句＞null"优先级逐键合并（meta 三键+对象名/单元类型+默认句
		//四键+cases.load_type+cases[].results 四键+materials[].reserve_factor+conclusion.
		//pass/fail 门随自动对象名激活；批次 A 注入口语义整体吸收于此）。未装配（退化）=
		//nullptr=二期发射形状逐字节兼容锚（R3-2 退化一致性，部件级 SHA1 逐值一致）
		QString datasetErr;
		const QJsonDocument dataset = _assemblyReady
			? mStrengthReportCollector::buildDataset(_config, datasetErr, &_autoAssembly)
			: mStrengthReportCollector::buildDataset(_config, datasetErr);
		//三期批次 A 元信息发射注入口语义已并入上方装配合并口（批次 B 收口——"自动"来源
		//meta.title/author/calc_file+project_subject 注入规则逐字保留，见 Collector::buildDataset
		//合并段注释；全部退化=零插入=二期发射形状逐字节兼容锚不变）
		//三期批次 A 注入块已并入 buildDataset 装配合并口（批次 B 收口——meta.title/author/
		//calc_file+project_subject"自动"来源注入规则逐字保留，且扩展至单元类型/默认句/
		//results 四键/材料剩余系数/conclusion.pass/fail 门）；此处置零重复注入
		const QJsonDocument& emission = dataset;
		const QString jsonPath = datasetJsonPath();
		QDir().mkpath(QFileInfo(jsonPath).absolutePath());
		QFile jsonFile(jsonPath);
		if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			const QString msg = QStringLiteral("数据集 JSON 写入失败：%1（%2）")
				.arg(jsonPath, jsonFile.errorString());
			_resultMessages->appendPlainText(msg);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
				QStringLiteral("[强度报告]%1").arg(msg), true);
			return;
		}
		jsonFile.write(emission.toJson(QJsonDocument::Indented));
		jsonFile.close();
		if (!datasetErr.isEmpty())
			_resultMessages->appendPlainText(QStringLiteral("数据集告警：\n%1").arg(datasetErr));

		//输出路径记忆随项目配置落盘（02 §3.3：打开对话框即回显）
		QString cfgErr;
		if (!_config.save(cfgErr) && !cfgErr.isEmpty())
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QStringLiteral("[强度报告]%1").arg(cfgErr), true);

		MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
			QStringLiteral("[强度报告]开始导出：%1 → %2")
				.arg(_containerPathEdit->text(), _outputPathEdit->text()), true);
		_resultMessages->appendPlainText(QStringLiteral("——开始导出（缺项 %1 项，策略：%2）——")
			.arg(_missingCount).arg(_missingPolicyCombo->currentText()));
		setExporting(true);

		QString err;
		if (!_runner->start(_containerPathEdit->text().trimmed(), jsonPath,
			_outputPathEdit->text().trimmed(), err))
		{
			setExporting(false);
			_resultMessages->appendPlainText(err);
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
				QStringLiteral("[强度报告]%1").arg(err), true);
		}
	}

	void mStrengthReportDlg::slot_cancelExport()
	{
		if (!_exporting)
			return;
		//LG-03：S3 批量截图段取消（此刻引擎尚未启动）——置整链取消旗标+中止剩余槽，
		//批量收尾后一键导出链跳过导出段（与导出中取消同一按钮，两段各自有定义）
		if (_autoCapture != nullptr && _autoCapture->batchActive())
		{
			_oneClickCancelled = true;
			_autoCapture->abortBatch();
			return;
		}
		_runner->cancel();   //03：取消=kill+清理 tmp；成败收尾统一在 finished 汇总
	}

	void mStrengthReportDlg::slot_onStageProgress(const QString& stage, int pct, const QString& msg)
	{
		_progressBar->setValue(pct);
		_stageLabel->setText(QStringLiteral("%1 %2").arg(stage, msg));
		_resultMessages->appendPlainText(QStringLiteral("[%1 %2%%] %3").arg(stage).arg(pct).arg(msg));
		if (_autoPreview != nullptr)
			_autoPreview->setStageText(QStringLiteral("%1 %2").arg(stage, msg));   //三期批次 A：S5 透传
	}

	void mStrengthReportDlg::slot_onExportFinished(bool ok, const QString& summary)
	{
		setExporting(false);
		_resultMessages->appendPlainText(ok
			? QStringLiteral("✔ %1").arg(summary)
			: QStringLiteral("✘ %1").arg(summary));
		//B-204（C13 裁定④）：试点"待确认"标记维持提示性、不阻断导出——补偿=导出成功
		//汇总增列"试点待确认槽 N 处"计数提示（N=试点抓取且未经手动抓取/导入/清除替换的
		//声明槽位数；N=0 不显示）。计数键面=_pendingConfirmKeys∩当前声明槽（容器会话内）
		if (ok)
		{
			int pendingConfirm = 0;
			for (const ImageSlot& slot : _imageSlots)
			{
				if (_pendingConfirmKeys.contains(slot.key))
					++pendingConfirm;
			}
			if (pendingConfirm > 0)
			{
				const QString tipLine = QStringLiteral("⚠ 自动截图待确认槽 %1 处（请核对缩略图后再交付）")
					.arg(pendingConfirm);
				_resultMessages->appendPlainText(tipLine);
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
					QStringLiteral("[强度报告]%1").arg(tipLine), true);
			}
			//BUG-302（C14②/B-303，11 §8 C14③）：导出成功汇总披露行（"确认不打扰机制"
			//审计对价——12 §2.1；批次 C 截图行=第三行拼接留位）。行 1=采用规则层默认句
			//N 处（逐键列出：键位名+句首截断）；行 2=自动采集结果数值 N 项（逐工况×键+
			//逐材料键列出：名/键/值，上限 10 项超出折叠"…等 N 项（详见页签②）"）；
			//N=0 该行不显示（三态）
			{
				//行 1：默认句披露（N=defaultSentenceCount 既有计数；来源=规则层默认句键）
				if (_autoAssembly.defaultSentenceCount > 0)
				{
					QStringList sentenceItems;
					for (auto it = _autoAssembly.items.constBegin();
						it != _autoAssembly.items.constEnd(); ++it)
					{
						const AutoValue& item = it.value();
						if (item.source != QStringLiteral("规则层默认句")
							|| item.value.isNull() || !item.value.isString())
							continue;
						const QString sentence = item.value.toString();
						sentenceItems.append(QStringLiteral("%1=%2%3")
							.arg(it.key(), sentence.left(12),
								sentence.size() > 12 ? QStringLiteral("…") : QString()));
					}
					if (!sentenceItems.isEmpty())
					{
						const QString line = QStringLiteral("采用规则层默认句 %1 处：%2")
							.arg(_autoAssembly.defaultSentenceCount)
							.arg(sentenceItems.join(QStringLiteral("；")));
						_resultMessages->appendPlainText(line);
					}
				}
				//行 2：自动采集结果数值披露（逐工况×键+逐材料 reserve；仅非 null 扫描/
				//计算值——上限 10 项折叠）。V-301（验收遗留微修复，与 C14① 排除清单对齐）：
				//录入覆盖键排除（E3-7 并存场景）——用户已录入键的未采纳自动值不以"自动采集"
				//名义披露（判定=config.caseResults/materialValues 与 C14① 计数同判定）
				{
					struct ScanItem { QString where; QString key; QString value; };
					QVector<ScanItem> scanItems;
					const QStringList resultKeys
					{
						QStringLiteral("stress_peak"), QStringLiteral("displacement_max"),
						QStringLiteral("buckling_mode"), QStringLiteral("reserve_factor")
					};
					for (const AssemblyResult::CaseScan& scan : _autoAssembly.cases)
					{
						const QJsonObject enteredCase =
							_config.caseResults.value(scan.caseName);
						const AutoValue* vals[4] =
						{ &scan.stressPeak, &scan.dispMax, &scan.buckling, &scan.reserve };
						for (int i = 0; i < 4; ++i)
						{
							if (vals[i]->value.isNull())
								continue;   //null=走缺项=失败计数披露面（failCount），不列数值项
							//V-301：录入键排除（与 C14① 计数同判定——未采纳自动值不披露）
							const QJsonValue enteredVal =
								enteredCase.value(resultKeys.at(i));
							if (!enteredVal.isNull() && !enteredVal.isUndefined())
								continue;
							QString text;
							if (vals[i]->value.isDouble())
								text = QLocale::c().toString(vals[i]->value.toDouble(), 'g', 15);
							else if (vals[i]->value.isString())
								text = vals[i]->value.toString();
							if (!text.isEmpty())
								scanItems.append({ scan.caseName, resultKeys.at(i), text });
						}
					}
					for (auto it = _autoAssembly.materialReserves.constBegin();
						it != _autoAssembly.materialReserves.constEnd(); ++it)
					{
						const AutoValue& rf = it.value();
						if (rf.value.isNull() || !rf.value.isDouble())
							continue;
						//V-301：材料录入 reserve 排除（与 C14① 计数同判定）
						const QJsonValue enteredRf = _config.materialValues
							.value(it.key()).value(QStringLiteral("reserve_factor"));
						if (!enteredRf.isNull() && !enteredRf.isUndefined())
							continue;
						scanItems.append({ QStringLiteral("材料"), it.key(),
							QLocale::c().toString(rf.value.toDouble(), 'g', 15) });
					}
					const int total = scanItems.size();
					if (total > 0)
					{
						QStringList rendered;
						const int showMax = qMin(total, 10);
						for (int i = 0; i < showMax; ++i)
						{
							const ScanItem& item = scanItems.at(i);
							rendered.append(QStringLiteral("%1/%2=%3")
								.arg(item.where, item.key, item.value));
						}
						QString line = QStringLiteral("自动采集数值 %1 项：%2")
							.arg(total).arg(rendered.join(QStringLiteral("；")));
						if (total > 10)
							line += QStringLiteral("；…等 %1 项（详见页签②）").arg(total);
						_resultMessages->appendPlainText(line);
					}
				}
				//行 3（三期批次 C 截图行=BUG-302 第三行拼接留位兑现，12 §2.1）：自动截图
				//成功 N 槽·失败 M 槽（原因分类逐槽列出，上限 10 折叠）+可疑 K 槽提示
				//（O3-5 单色检测提示性不阻断）；_captureBatchRan=false（开关关/映射空）
				//该行不显示=批次 B 行为零变化（Q3-3 回归锚）
				if (_captureBatchRan)
				{
					int okN = 0;
					int failN = 0;
					int suspectN = 0;
					int abortN = 0;
					QStringList failItems;
					for (const CaptureSlotResult& r : _lastCaptureResults)
					{
						if (r.ok)
						{
							++okN;
							if (r.suspect)
								++suspectN;
						}
						else if (!r.skipped && r.reasonCode == QStringLiteral("Aborted"))
							++abortN;   //R4X-02：取消通道槽保留原状，不入失败桶
						else if (!r.skipped)
						{
							++failN;
							failItems.append(QStringLiteral("%1=%2")
								.arg(r.slotKey, captureFailLabel(r.reasonCode)));
						}
					}
					if (okN + failN + abortN > 0)
					{
						QString capLine = QStringLiteral("自动截图成功 %1 槽·失败 %2 槽")
							.arg(okN).arg(failN);
						if (!failItems.isEmpty())
						{
							const int showMax = qMin(failItems.size(), 10);
							QStringList shown = failItems.mid(0, showMax);
							capLine += QStringLiteral("（原因：%1%2）")
								.arg(shown.join(QStringLiteral("；")),
									failItems.size() > 10
									? QStringLiteral("；…等 %1 槽").arg(failItems.size())
									: QString());
						}
						//R4X-02：中止槽披露口径="取消保留原状 N 槽"（与页签③/Tab0 提示行一致）
						if (abortN > 0)
							capLine += QStringLiteral("；中止保留原状 %1 槽").arg(abortN);
						if (suspectN > 0)
							capLine += QStringLiteral("；⚠ 可疑（近单色）%1 槽，请逐槽核对缩略图")
								.arg(suspectN);
						_resultMessages->appendPlainText(capLine);
					}
				}
				//BUG-303 静默降级治理：形态识别失败→导出汇总披露（预览 ⚠ 行已在装配侧
				//__automation_shape__ 项——两处呈现，不留静默）
				const AutoValue shapeItem = _autoAssembly.items.value(
					QStringLiteral("__automation_shape__"));
				if (!shapeItem.warn.isEmpty())
					_resultMessages->appendPlainText(
						QStringLiteral("⚠ %1").arg(shapeItem.warn));
				//R3P-03：装配聚合告警披露（warns 全链零消费治理——PB-01 空白句逐键/
				//PA-10 names 错型/PB-03 同名多极限/E3-2 映射缺失/E3-3 格式回退/材料双空
				//降级的定位文本经此呈现；逐条列出、上限 10 折叠同 BUG-302 惯例；
				//N=0 不显示）。与 Tab0 汇总行 ⚠ 计数（悬停明细）同源双呈现；
				//形态失败 ⚠ 已有专属行（上方）——去重防同文双行
				QStringList warnItems = _autoAssembly.warns;
				if (!shapeItem.warn.isEmpty())
					warnItems.removeAll(shapeItem.warn);
				if (!warnItems.isEmpty())
				{
					const int total = warnItems.size();
					const int showMax = qMin(total, 10);
					QStringList rendered;
					for (int i = 0; i < showMax; ++i)
						rendered.append(warnItems.at(i));
					QString warnLine = QStringLiteral("自动化配置/取值告警 %1 条：%2")
						.arg(total).arg(rendered.join(QStringLiteral("；")));
					if (total > 10)
						warnLine += QStringLiteral("；…等 %1 条").arg(total);
					_resultMessages->appendPlainText(warnLine);
				}
			}
		}
		_resultMessages->appendPlainText(QString());
		if (_autoPreview != nullptr)
		{
			//三期批次 A：S6 完成态（12 §2.1 六态合并——S6 Done 后主按钮使能=支持重复一键导出
			//幂等 NFR-3.5）；主按钮使能随导出旗标解除正交刷新
			_autoPreview->finishExport(ok, summary.split(QLatin1Char('\n')).value(0));
			updateAutoPreviewUi();
		}
		//FR-35：输出窗口"[强度报告]"成败消息；失败文案已含引擎映射（占用/磁盘满/目录不可写）
		MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(ok ? 0 : 2,
			QStringLiteral("[强度报告]%1")
				.arg(summary.split(QLatin1Char('\n')).value(0)), true);
		//CB-01 收敛：导出期间发生项目切换时，switchProjectConfigIfNeeded 的自动重校验
		//被 slot_revalidate 的 _exporting 短路——导出收尾后容器路径在位而状态=未加载，补跑
		if (_loadState == NotLoaded && !_containerPathEdit->text().trimmed().isEmpty()
			&& !_probe->isRunning())
			slot_revalidate();
		//C-5 兜底复位：导出完成时强制恢复产品水印一次（防异常残留把水印长期丢失——E2.5；
		//正常抓取路径已由 LogoFlipGuard 析构收口，此为09 §0.2 要求的兜底点）
		mStrengthReportCollector::restoreViewportLogo();
	}

	void mStrengthReportDlg::reject()
	{
		//CB-03：Esc 触发 QDialog::reject()，不经 closeEvent——导出中复用同一二次确认
		//（02 §3.6"导出中 Esc 二次确认"；确认后 cancel() 再关闭，与 closeEvent 路径一致）
		if (_exporting)
		{
			//R2L-02：批量截图段（引擎未启动）与导出中分文案——措辞与实际进行中环节一致
			const bool batchNow = _autoCapture != nullptr && _autoCapture->batchActive();
			const QMessageBox::StandardButton choice = QMessageBox::question(this,
				QStringLiteral("强度计算报告导出"),
				batchNow
				? QStringLiteral("自动截图批量进行中，确认取消自动截图与本次导出并关闭对话框？")
				: QStringLiteral("导出正在进行中，确认取消导出并关闭对话框？"));
			if (choice != QMessageBox::Yes)
				return;
			_runner->cancel();
			//LG-03：S3 批量截图段（已并入导出互斥态）——确认取消=整链终止旗标，
			//批量收尾后导出段不再续跑（下方 abortAlignWait→abortBatch 即时中止）
			if (_autoCapture != nullptr && _autoCapture->batchActive())
				_oneClickCancelled = true;
		}
		abortAlignWait();   //批次 C：等待中的试点抓取随对话框关闭立即中止（B-6①）
		//C-5 兜底复位（Esc 关闭与 closeEvent 同口径）
		mStrengthReportCollector::restoreViewportLogo();
		QDialog::reject();
	}

	void mStrengthReportDlg::closeEvent(QCloseEvent* event)
	{
		//03 §4.5/T-15：导出中关闭→二次确认+cancel()（Esc 同路径）
		if (_exporting)
		{
			//R2L-02：批量截图段（引擎未启动）与导出中分文案——措辞与实际进行中环节一致
			const bool batchNow = _autoCapture != nullptr && _autoCapture->batchActive();
			const QMessageBox::StandardButton choice = QMessageBox::question(this,
				QStringLiteral("强度计算报告导出"),
				batchNow
				? QStringLiteral("自动截图批量进行中，确认取消自动截图与本次导出并关闭对话框？")
				: QStringLiteral("导出正在进行中，确认取消导出并关闭对话框？"));
			if (choice != QMessageBox::Yes)
			{
				event->ignore();
				return;
			}
			_runner->cancel();
			//LG-03：S3 批量截图段（已并入导出互斥态）——确认取消=整链终止旗标，
			//批量收尾后导出段不再续跑（下方 abortAlignWait→abortBatch 即时中止）
			if (_autoCapture != nullptr && _autoCapture->batchActive())
				_oneClickCancelled = true;
		}
		abortAlignWait();   //批次 C：等待中的试点抓取随对话框关闭立即中止（B-6①）
		//C-5 兜底复位：主对话框关闭时强制恢复产品水印一次（防异常残留长期丢水印——09 §0.2）
		mStrengthReportCollector::restoreViewportLogo();
		event->accept();
	}
}
