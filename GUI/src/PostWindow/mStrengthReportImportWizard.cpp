#include "mStrengthReportImportWizard.h"

#include "mStrengthReportContainerProbe.h"
#include "mStrengthReportConfig.h"   //T2：绑定键人话字典访问器（与 Tab1 槽表共用）
#include "mStrengthReportEngineRunner.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <cstring>

namespace StrengthReport
{
	namespace
	{
		const char ZIP_EOCD_SIG[] = { '\x50', '\x4B', '\x05', '\x06' };

		//子进程看门狗（与引擎 Runner/probe 同 120s 口径——python 挂死不永久阻塞向导）
		const int WIZARD_WATCHDOG_MS = 120000;
		//B-5：陈旧 wizard_* 目录清扫阈值（进程崩溃残液与 pid 复用冲突防护；活跃目录
		//按"24h 内有写入"判定不触碰——一次向导全程远短于该阈值）
		const qint64 STALE_WIZARD_SECS = 24 * 3600;

		QString policyDisplay(const QString& policy)
		{
			if (policy == QStringLiteral("keep_highlight"))
				return QStringLiteral("保留模板原文");
			if (policy == QStringLiteral("block_export"))
				return QStringLiteral("阻断导出");
			return QStringLiteral("留\"-\"并告警");
		}

		QString policyValue(int index)
		{
			switch (index)
			{
			case 1: return QStringLiteral("keep_highlight");
			case 2: return QStringLiteral("block_export");
			default: return QStringLiteral("dash_and_warn");
			}
		}

		//zip 中央目录试解包（R3X-04：与 tdr_digest.verify_input 同深度——EOCD 定位+
		//中央目录枚举+[Content_Types].xml/word/document.xml 双部件在位核对，消"步①
		//EOCD 快检通过、步② digest 推翻"的深度不一致；文案与 CLI 逐字一致）
		//detail=失败明细（对应 CLI 异常 %s/缺失部件清单）；missingParts=true 时 detail
		//为缺失部件清单（调用方按"缺少部件"文案组装）
		bool zipTrialUnpack(const QString& path, QString& detail, bool* missingParts)
		{
			detail.clear();
			if (missingParts != nullptr)
				*missingParts = false;
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly))
			{
				detail = file.errorString();
				return false;
			}
			const QByteArray tail = file.readAll();
			file.close();
			const char* base = tail.constData();
			const qsizetype size = tail.size();
			//① 定位 EOCD（尾部 22+65535 字节窗口内倒扫签名）
			qsizetype eocd = -1;
			const qsizetype window = qMin<qsizetype>(size, 22 + 65535);
			for (qsizetype i = size - 22; i >= size - window && i >= 0; --i)
			{
				if (std::memcmp(base + i, ZIP_EOCD_SIG, 4) == 0)
				{
					eocd = i;
					break;
				}
			}
			if (eocd < 0)
			{
				detail = QStringLiteral("zip 结尾标记缺失（伪 zip/非 zip 文件）");
				return false;
			}
			auto u16 = [base](qsizetype off) -> quint32
			{
				return quint32(uchar(base[off])) | (quint32(uchar(base[off + 1])) << 8);
			};
			auto u32 = [base](qsizetype off) -> quint32
			{
				return quint32(uchar(base[off])) | (quint32(uchar(base[off + 1])) << 8)
					| (quint32(uchar(base[off + 2])) << 16) | (quint32(uchar(base[off + 3])) << 24);
			};
			const quint32 cdSize = u32(eocd + 12);
			const quint32 cdOffset = u32(eocd + 16);
			if (cdOffset + quint64(cdSize) > quint64(size))
			{
				detail = QStringLiteral("zip 目录越界（位置信息与文件不符）");
				return false;
			}
			//② 中央目录枚举（每条目固定 46 字节头+变长名/extra/comment）
			QSet<QString> names;
			quint64 pos = cdOffset;
			for (quint32 n = 0; n < u16(eocd + 10); ++n)
			{
				if (pos + 46 > quint64(size)
					|| std::memcmp(base + pos, "\x50\x4B\x01\x02", 4) != 0)
				{
					detail = QStringLiteral("zip 目录条目损坏");
					return false;
				}
				const quint32 nameLen = u16(pos + 28);
				const quint32 extraLen = u16(pos + 30);
				const quint32 commentLen = u16(pos + 32);
				if (pos + 46 + quint64(nameLen) + extraLen + commentLen > quint64(size))
				{
					detail = QStringLiteral("zip 目录条目越界");
					return false;
				}
				names.insert(QString::fromUtf8(base + pos + 46, int(nameLen)));
				pos += 46 + quint64(nameLen) + extraLen + commentLen;
			}
			//③ OOXML 双部件在位核对（与 CLI missing 清单同口径）
			QStringList missing;
			for (const QString& part : QStringList{ QStringLiteral("[Content_Types].xml"),
				QStringLiteral("word/document.xml") })
			{
				if (!names.contains(part))
					missing.append(part);
			}
			if (!missing.isEmpty())
			{
				if (missingParts != nullptr)
					*missingParts = true;
				detail = missing.join(QStringLiteral(", "));
				return false;
			}
			return true;
		}

		//CB-16 同口径：剥整行 '#' 注释后解析（digest 产草稿 rules/rules.json 带注释头，
		//QJsonDocument 不认注释；字符串值内换行恒为 \n 转义，整行判据安全）
		QJsonDocument parseRulesText(const QByteArray& raw, bool* ok)
		{
			const QStringList rawLines = QString::fromUtf8(raw).split(QLatin1Char('\n'));
			QStringList jsonLines;
			jsonLines.reserve(rawLines.size());
			for (const QString& line : rawLines)
			{
				if (!line.trimmed().startsWith(QLatin1Char('#')))
					jsonLines.append(line);
			}
			QJsonDocument doc = QJsonDocument::fromJson(jsonLines.join(QLatin1Char('\n')).toUtf8());
			if (ok != nullptr)
				*ok = doc.isObject();
			return doc;
		}
	}

	mStrengthReportImportWizard::mStrengthReportImportWizard(QWidget* parent)
		: QDialog(parent)
	{
		setWindowTitle(QStringLiteral("强度报告——从 docx 模板导入（提炼向导）"));
		setObjectName(QStringLiteral("strengthReportImportWizard"));
		//08 §2.6：向导为独立对话框、对主对话框模态（主对话框对主窗口仍非模态，U-01 不变）
		//——调用方 exec()（应用级模态，覆盖主对话框）
		setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
		resize(720, 560);

		//B-5：启动清扫陈旧 wizard_* 目录（防进程崩溃残液与 pid 复用冲突），
		//先清扫再建本进程目录（NFR-2.4 清扫不误删活跃目录=mtime 阈值边界）
		sweepStaleWizardDirs();
		_tmpRoot = wizardTempRoot();
		QDir().mkpath(_tmpRoot);
		_rulesEditPath = QDir(_tmpRoot).absoluteFilePath(QStringLiteral("rules_edit.json"));

		buildUi();
		gotoStep(0);
	}

	mStrengthReportImportWizard::~mStrengthReportImportWizard()
	{
		//CB-11/CB-06 同口径：先停看门狗+断开子进程信号，再 kill+wait（Qt5 QProcess
		//析构不终止子进程，防孤儿 python）；随后清理本向导临时目录（零半成品，E2.7）
		if (_watchdog != nullptr)
			_watchdog->stop();
		const QList<QProcess*> procs = findChildren<QProcess*>();
		for (QProcess* proc : procs)
		{
			proc->disconnect(this);
			if (proc->state() != QProcess::NotRunning)
			{
				proc->kill();
				proc->waitForFinished(2000);
			}
		}
		cleanupTempDir();
	}

	QString mStrengthReportImportWizard::wizardTempRoot()
	{
		//E10 同源：mx_strength_report_tmp 独立临时区下按 wizard_<pid> 隔离（并发向导/
		//崩溃残液互不覆盖；B-5 清扫按目录级处理）
		return QDir(QDir::temp().absoluteFilePath(QStringLiteral("mx_strength_report_tmp")))
			.absoluteFilePath(QStringLiteral("wizard_%1").arg(QCoreApplication::applicationPid()));
	}

	void mStrengthReportImportWizard::sweepStaleWizardDirs()
	{
		const QDir base(QDir::temp().absoluteFilePath(QStringLiteral("mx_strength_report_tmp")));
		if (!base.exists())
			return;
		const QDateTime now = QDateTime::currentDateTime();
		const QStringList stale = base.entryList(QStringList{ QStringLiteral("wizard_*") },
			QDir::Dirs | QDir::NoDotAndDotDot);
		for (const QString& name : stale)
		{
			const QFileInfo info(base.absoluteFilePath(name));
			if (info.lastModified().secsTo(now) > STALE_WIZARD_SECS)
				QDir(info.absoluteFilePath()).removeRecursively();
		}
	}

	//——步① E6 前置校验（与 tdr_digest.verify_input 同序同文案：扩展名→读模式
	//占用探测→试解包快检；FR-251 验收口径"E5/E6 负例文案与 CLI 一致"；
	//REF-01/C15 占用探测放宽：读试开（ReadOnly）为口径——共享可读即放行，
	//可读但写不开（Word 写锁/只读属性）→非阻断，经 hint 返回编辑中提示；
	//ReadOnly 仍失败=真不可读（独占锁或权限限制）→阻断）——
	bool mStrengthReportImportWizard::precheckSourceDocx(const QString& src, QString& err, QString* hint)
	{
		err.clear();
		if (hint != nullptr)
			hint->clear();
		const QFileInfo info(src);
		if (src.isEmpty() || !info.exists() || !info.isFile())
		{
			err = QStringLiteral("源文件不存在：%1").arg(src);
			return false;
		}
		const QString suffix = info.suffix();
		if (suffix.compare(QStringLiteral("doc"), Qt::CaseInsensitive) == 0)
		{
			err = QStringLiteral("源文件为 .doc 老格式：请先在 Word 中另存为 .docx 后重试（%1）").arg(src);
			return false;
		}
		if (suffix.compare(QStringLiteral("docx"), Qt::CaseInsensitive) != 0)
		{
			//R3X-05①：扩展名展示带前导点+直引号——与 CLI `当前 ".txt"` 形态逐字一致
			//（tdr_digest.py 同步改，FR-251"E5/E6 负例文案与 CLI 一致"口径）
			err = QStringLiteral("扩展名必须为 .docx（当前 \".%1\"）：请确认文件格式，"
				".doc 请先另存为 .docx").arg(suffix);
			return false;
		}
		//占用探测（REF-01/C15：读模式试开——Word 打开 docx 只加写锁、允许共享
		//读，此前写试开把"可安全读"场景误判阻断，与 FR-12 只读不写回相悖；
		//只读属性文件现在可正常消化）
		QFile probe(src);
		if (!probe.open(QIODevice::ReadOnly))
		{
			//R3X-05②：按错误类型分流（与 CLI PermissionError/其他 OSError 两文案同构）
			//——非权限类失败（网络路径不可达等）不再误归因"独占锁或权限限制"
			if (probe.error() == QFileDevice::PermissionsError)
				err = QStringLiteral("源文件不可读（独占锁或权限限制），请检查后重试");
			else
				err = QStringLiteral("源文件不可读：%1").arg(probe.errorString());
			return false;
		}
		probe.close();
		if (!probe.open(QIODevice::ReadWrite))
		{
			//可读但写不开=被 Word 等以写锁占用（共享读）或只读属性——不阻断，
			//按磁盘最近保存版本消化并在向导对应步骤显示同款提示
			if (hint != nullptr)
			{
				*hint = QStringLiteral("源文件可能正在 Word 中编辑，本次按最近一次保存到磁盘的"
					"版本消化（不含未保存修改），建议先保存并关闭以确保基线正确");
			}
		}
		//R3X-04：试解包深度对齐 CLI（中央目录枚举+双部件核对，zipTrialUnpack）；
		//失败文案与 CLI 逐字一致（无法按 zip 解包：%1 / 缺少部件 %1）
		QString zipDetail;
		bool missingParts = false;
		if (!zipTrialUnpack(src, zipDetail, &missingParts))
		{
			err = missingParts
				? QStringLiteral("文件损坏或非 docx 格式（缺少部件 %1）").arg(zipDetail)
				: QStringLiteral("文件损坏或非 docx 格式（无法按 zip 解包）：%1").arg(zipDetail);
			return false;
		}
		return true;
	}

	//——界面构建——

	void mStrengthReportImportWizard::buildUi()
	{
		auto* mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(10, 8, 10, 8);
		mainLayout->setSpacing(6);

		_pages = new QStackedWidget(this);
		_pages->addWidget(buildStepSourcePage());
		_pages->addWidget(buildStepDigestPage());
		_pages->addWidget(buildStepRulesPage());
		_pages->addWidget(buildStepValidatePage());
		_pages->addWidget(buildStepTrialPage());
		_pages->addWidget(buildStepFinalPage());
		mainLayout->addWidget(_pages, 1);

		//——底部导航——
		auto* navRow = new QHBoxLayout();
		_stepLabel = new QLabel(this);
		navRow->addWidget(_stepLabel, 1);
		_backBtn = new QPushButton(QStringLiteral("上一步"), this);
		navRow->addWidget(_backBtn);
		_nextBtn = new QPushButton(QStringLiteral("下一步"), this);
		//D-03/P-08：文案随步切换 6 态（updateNavButtons），最长态"回填并检查"5 全角=80 恰等
		//qss 钉宽 80（L5 零余量）——按最长态显式配最小宽（16n+24=104），与孪生钮口径拉齐
		//（创建处文案为"下一步"，fit 语义无效，须按最长态直写；不逐态 fit——6 态文字恒定）
		_nextBtn->setMinimumWidth(_nextBtn->fontMetrics().horizontalAdvance(
			QStringLiteral("回填并检查")) + 24);
		navRow->addWidget(_nextBtn);
		_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
		navRow->addWidget(_cancelBtn);
		mainLayout->addLayout(navRow);

		connect(_backBtn, &QPushButton::clicked, this, [this]() { gotoStep(_step - 1); });
		connect(_nextBtn, &QPushButton::clicked, this, [this]()
			{
				if (_step == 5)
				{
					cleanupTempDir();   //完成：定版产物已在用户路径，临时件清理（零残留）
					accept();
					return;
				}
				if (_step == 0 && !_sourceOk)
				{
					//手输路径场景：下一步前补做步①校验
					const QString src = _sourcePathEdit->text().trimmed();
					QString err;
					QString hint;
					if (!precheckSourceDocx(src, err, &hint))
					{
						setStepStatus(_sourceStatus, false, err);
						return;
					}
					if (src != _sourceDocx)
					{
						//WZ-01：手输换源同浏览回调口径——下游旗标级联失效+旧产物清理
						resetDownstreamFrom(1);
						QDir tmp(_tmpRoot);
						const QStringList olds = tmp.entryList(QStringList{
							QStringLiteral("*.rtdoc"), QStringLiteral("*_digest_report.json"),
							QStringLiteral("*_digest_report.txt") }, QDir::Files);
						for (const QString& old : olds)
							QFile::remove(tmp.absoluteFilePath(old));
					}
					_sourceDocx = src;
					captureSourceFingerprint(src);
					_sourceOk = true;
					setStepStatus(_sourceStatus, true,
						hint.isEmpty()
							? QStringLiteral("检查通过（源文件全程只读，提炼不写回）")
							: QStringLiteral("检查通过（源文件全程只读，提炼不写回）——%1").arg(hint));
					updateNavButtons();
				}
				if (_step == 2)
				{
					//步③离开闸：轻校验+未确认行提示（C-6b"逐条确认"），硬校验归步④ put
					QString err;
					if (!applyRulesEdits(err))
					{
						setStepStatus(_rulesStatus, false, err);
						return;
					}
					int unchecked = 0;
					int uncheckedKeep = 0;
					for (int r = 0; r < _rulesTable->rowCount(); ++r)
					{
						QTableWidgetItem* chk = _rulesTable->item(r, 0);
						if (chk == nullptr || chk->checkState() != Qt::Checked)
						{
							++unchecked;
							const QString note = _rulesTable->item(r, 5) != nullptr
								? _rulesTable->item(r, 5)->text() : QString();
							if (note.contains(QStringLiteral("非高亮文字")))
								++uncheckedKeep;
						}
					}
					if (unchecked > 0)
					{
						const QMessageBox::StandardButton pick = QMessageBox::question(this,
							QStringLiteral("待填内容确认"),
							QStringLiteral("还有 %1 条待填内容未确认（其中非高亮文字槽 %2 条）。\n"
								"未确认项将按草稿默认回填（非高亮文字槽=保留模板原文）。确认继续？")
								.arg(unchecked).arg(uncheckedKeep));
						if (pick != QMessageBox::Yes)
							return;
					}
				}
				gotoStep(_step + 1);
			});
		connect(_cancelBtn, &QPushButton::clicked, this, [this]() { reject(); });
	}

	QWidget* mStrengthReportImportWizard::buildStepSourcePage()
	{
		auto* page = new QWidget(this);
		auto* lay = new QVBoxLayout(page);
		lay->setContentsMargins(4, 4, 4, 4);
		lay->setSpacing(6);

		lay->addWidget(new QLabel(QStringLiteral(
			"第 1 步：选择源 docx 模板（模板内黄色高亮文字=待填写占位，可在 Word 中查看）"), this));
		auto* row = new QHBoxLayout();
		_sourcePathEdit = new QLineEdit(page);
		_sourcePathEdit->setPlaceholderText(QStringLiteral("源 Word 模板路径（.docx）"));
		row->addWidget(_sourcePathEdit, 1);
		_browseSourceBtn = new QPushButton(QStringLiteral("浏览…"), page);
		row->addWidget(_browseSourceBtn);
		lay->addLayout(row);

		_sourceStatus = new QLabel(page);
		_sourceStatus->setTextFormat(Qt::RichText);
		_sourceStatus->setWordWrap(true);
		lay->addWidget(_sourceStatus);

		lay->addWidget(new QLabel(QStringLiteral(
			"说明：选定即执行前置检查（扩展名/占用/格式三步，与命令行提炼同口径）；"
			"源文件全程只读，提炼不写回。"), page));
		lay->addStretch(1);

			connect(_browseSourceBtn, &QPushButton::clicked, this, [this]()
				{
					const QString start = _sourcePathEdit->text().trimmed();
					const QString file = QFileDialog::getOpenFileName(this,
						QStringLiteral("选择源 docx 模板"),
						start.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
						: start,
						QStringLiteral("Word 文档 (*.docx)"));
					if (file.isEmpty())
						return;
					_sourcePathEdit->setText(file);
					if (file != _sourceDocx)
					{
						//WZ-01：换源=下游全部产物失效（digest 起强制重跑；同源重选不失效），
						//并清理上一源临时容器/报告（防多容器残留与"旧容器当新源产物"）
						resetDownstreamFrom(1);
						QDir tmp(_tmpRoot);
						const QStringList olds = tmp.entryList(QStringList{
							QStringLiteral("*.rtdoc"), QStringLiteral("*_digest_report.json"),
							QStringLiteral("*_digest_report.txt") }, QDir::Files);
						for (const QString& old : olds)
							QFile::remove(tmp.absoluteFilePath(old));
					}
					_sourceDocx = file;
					captureSourceFingerprint(file);
					QString err;
					QString hint;
					_sourceOk = precheckSourceDocx(file, err, &hint);
				setStepStatus(_sourceStatus, _sourceOk,
					_sourceOk
						? (hint.isEmpty()
							? QStringLiteral("检查通过（源文件全程只读，提炼不写回）")
							: QStringLiteral("检查通过（源文件全程只读，提炼不写回）——%1").arg(hint))
						: err);
				updateNavButtons();
			});
		return page;
	}

	QWidget* mStrengthReportImportWizard::buildStepDigestPage()
	{
		auto* page = new QWidget(this);
		auto* lay = new QVBoxLayout(page);
		lay->setContentsMargins(4, 4, 4, 4);
		lay->setSpacing(6);

		lay->addWidget(new QLabel(QStringLiteral("第 2 步：一键消化（docx → 参数化规则模板容器）"), this));
		_digestStatus = new QLabel(page);
		_digestStatus->setTextFormat(Qt::RichText);
		_digestStatus->setWordWrap(true);
		lay->addWidget(_digestStatus);
		_digestSummary = new QPlainTextEdit(page);
		_digestSummary->setReadOnly(true);
		lay->addWidget(_digestSummary, 1);
		lay->addWidget(new QLabel(QStringLiteral(
			"说明：消化=资产提取+切块+规则层候选草稿生成（与命令行提炼工具 tdr_digest "
			"同一代码路径，产物等价）；失败零产出，可返回上一步换文件。"), page));
		return page;
	}

	QWidget* mStrengthReportImportWizard::buildStepRulesPage()
	{
		auto* page = new QWidget(this);
		auto* lay = new QVBoxLayout(page);
		lay->setContentsMargins(4, 4, 4, 4);
		lay->setSpacing(6);

		lay->addWidget(new QLabel(QStringLiteral("第 3 步：规则层编辑引导（待填内容确认与键名/缺项策略编辑）"), this));
		//C-6(a)：D-18 类人工确认项顶部显式列出（digest 字面报告驱动+固定待办）
		_d18Area = new QLabel(page);
		_d18Area->setTextFormat(Qt::RichText);
		_d18Area->setWordWrap(true);
		lay->addWidget(_d18Area);

		_rulesTable = new QTableWidget(page);
		_rulesTable->setColumnCount(6);
		_rulesTable->setHorizontalHeaderLabels(QStringList{
			QStringLiteral("确认"), QStringLiteral("类型"), QStringLiteral("匹配文字"),
			QStringLiteral("建议键名"), QStringLiteral("缺项策略"), QStringLiteral("备注") });   //WO-6/P-10：缩短防首尾裁字（96~100 < 需118/136/154）；"（可编辑）"语义移列头 tooltip
		_rulesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
		_rulesTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
		_rulesTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
		//FR-06：缩短掉的"（可编辑）"语义移列头 tooltip（信息不丢；悬停展示，零布局影响）
		_rulesTable->horizontalHeaderItem(3)->setToolTip(QStringLiteral(
			"建议键名（可编辑）：双击或选中后可直接修改该列单元格"));
		_rulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		lay->addWidget(_rulesTable, 2);

		lay->addWidget(new QLabel(QStringLiteral(
			"循环章/表格槽行组声明（只读预览；高级编辑可用命令行规则层文本工具 "
			"tdr_rules get/put 取出/回填）；图片/表格槽锚点以坐标简写呈现"
			"（表格[字面] 行列、空段 块号）："), page));
		_loopPreview = new QPlainTextEdit(page);
		_loopPreview->setReadOnly(true);
		lay->addWidget(_loopPreview, 1);

		_rulesStatus = new QLabel(page);
		_rulesStatus->setTextFormat(Qt::RichText);
		_rulesStatus->setWordWrap(true);
		lay->addWidget(_rulesStatus);
		return page;
	}

	QWidget* mStrengthReportImportWizard::buildStepValidatePage()
	{
		auto* page = new QWidget(this);
		auto* lay = new QVBoxLayout(page);
		lay->setContentsMargins(4, 4, 4, 4);
		lay->setSpacing(6);

		lay->addWidget(new QLabel(QStringLiteral("第 4 步：回填与检查（写回规则层 → 全量检查）"), this));
		_validateStatus = new QLabel(page);
		_validateStatus->setTextFormat(Qt::RichText);
		_validateStatus->setWordWrap(true);
		lay->addWidget(_validateStatus);
		_validateReport = new QPlainTextEdit(page);
		_validateReport->setReadOnly(true);
		lay->addWidget(_validateReport, 1);
		lay->addWidget(new QLabel(QStringLiteral(
			"说明：回填前会检查键名与匹配文字，非法条目将被阻断；"
			"检查失败可回上一步修改重试。"), page));
		return page;
	}

	QWidget* mStrengthReportImportWizard::buildStepTrialPage()
	{
		auto* page = new QWidget(this);
		auto* lay = new QVBoxLayout(page);
		lay->setContentsMargins(4, 4, 4, 4);
		lay->setSpacing(6);

		lay->addWidget(new QLabel(QStringLiteral("第 5 步：试导出（空数据与样例数据两种情况）"), this));
		_trialStatus = new QLabel(page);
		_trialStatus->setTextFormat(Qt::RichText);
		_trialStatus->setWordWrap(true);
		lay->addWidget(_trialStatus);
		_trialReport = new QPlainTextEdit(page);
		_trialReport->setReadOnly(true);
		lay->addWidget(_trialReport, 1);
		lay->addWidget(new QLabel(QStringLiteral(
			"说明：试导出产物为临时文件（向导关闭即清理），请在 Word 中打开核对样式与 "
			"F9 刷新目录后再定版。"), page));
		return page;
	}

	QWidget* mStrengthReportImportWizard::buildStepFinalPage()
	{
		auto* page = new QWidget(this);
		auto* lay = new QVBoxLayout(page);
		lay->setContentsMargins(4, 4, 4, 4);
		lay->setSpacing(6);

		lay->addWidget(new QLabel(QStringLiteral("第 6 步：定版（保存容器到最终路径）"), this));
		auto* row = new QHBoxLayout();
		row->addWidget(new QLabel(QStringLiteral("规则模板文档(.rtdoc)："), page));
		_finalPathEdit = new QLineEdit(page);
		row->addWidget(_finalPathEdit, 1);
		_browseFinalBtn = new QPushButton(QStringLiteral("浏览…"), page);
		row->addWidget(_browseFinalBtn);
		_finalizeBtn = new QPushButton(QStringLiteral("定版"), page);
		row->addWidget(_finalizeBtn);
		lay->addLayout(row);

		_finalStatus = new QLabel(page);
		_finalStatus->setTextFormat(Qt::RichText);
		_finalStatus->setWordWrap(true);
		lay->addWidget(_finalStatus);

		lay->addWidget(new QLabel(QStringLiteral(
			"说明：中间产物全程在临时目录，此步才写入最终路径（中途取消不会留下半成品）；"
			"完成向导后写入主对话框加载栏并自动重新检查。"
			"\n试导出产物为临时文件，完成向导时即删除；定版前请在 Word 核对后自行复制留存核对件。"
			"\n老容器升级请使用命令行半自动迁移工具（tdr_digest upgrade_container："
			"重消化+新旧差异报告）。"), page));
		lay->addStretch(1);

		connect(_browseFinalBtn, &QPushButton::clicked, this, [this]()
			{
				QString start = _finalPathEdit->text().trimmed();
				if (start.isEmpty())
				{
					QDir dir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
					start = dir.absoluteFilePath(
						(_templateId.isEmpty() ? QStringLiteral("template") : _templateId)
						+ QStringLiteral(".rtdoc"));
				}
				const QString file = QFileDialog::getSaveFileName(this,
					QStringLiteral("定版规则模板文档"), start,
					QStringLiteral("规则模板文档 (*.rtdoc)"));
				if (!file.isEmpty())
					_finalPathEdit->setText(file);
			});
		connect(_finalizeBtn, &QPushButton::clicked, this, [this]()
			{
				QString err;
				if (!finalizeContainer(err))
				{
					setStepStatus(_finalStatus, false, err);
					return;
				}
				setStepStatus(_finalStatus, true,
					QStringLiteral("已定版：%1（完成向导后将写入主对话框加载栏并自动重新检查）")
					.arg(_finalizedPath));
				updateNavButtons();
			});
		return page;
	}

	//——子进程链（probe 已验证模式：QProcess 异步+120s 看门狗+文本协议解析）——

	void mStrengthReportImportWizard::runChain(const QVector<CmdSpec>& chain, ChainPurpose purpose)
	{
		if (_chainBusy || chain.isEmpty())
			return;
		_chain = chain;
		_chainIndex = -1;
		_purpose = purpose;
		_failedTag.clear();
		setChainBusy(true);
		startNextCmd();
	}

	void mStrengthReportImportWizard::startNextCmd()
	{
		++_chainIndex;
		if (_chainIndex >= _chain.size())
		{
			setChainBusy(false);
			finishChainOk();
			return;
		}
		_chainStdout.clear();
		_chainStderr.clear();
		if (_process == nullptr)
		{
			_process = new QProcess(this);
			_process->setProcessChannelMode(QProcess::SeparateChannels);
			connect(_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
				this, &mStrengthReportImportWizard::onCmdFinished);
		}
		_timedOut = false;
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
		_process->setProcessEnvironment(env);
		_process->setWorkingDirectory(_tmpRoot);
		_process->start(_chain[_chainIndex].program, QStringList{ QStringLiteral("-u") }
			+ _chain[_chainIndex].args);
		if (!_process->waitForStarted(5000))
		{
			abortChain(QStringLiteral("命令进程启动失败：%1（%2）")
				.arg(_chain[_chainIndex].tag, _process->errorString()));
			return;
		}
		if (_watchdog == nullptr)
		{
			_watchdog = new QTimer(this);
			_watchdog->setSingleShot(true);
			connect(_watchdog, &QTimer::timeout, this, &mStrengthReportImportWizard::onWatchdogTimeout);
		}
		_watchdog->start(WIZARD_WATCHDOG_MS);
	}

	void mStrengthReportImportWizard::onWatchdogTimeout()
	{
		_timedOut = true;
		if (_process != nullptr && _process->state() != QProcess::NotRunning)
			_process->kill();   //kill 后由 finished 回调收尾汇报
	}

	void mStrengthReportImportWizard::onCmdFinished(int exitCode, QProcess::ExitStatus exitStatus)
	{
		if (_watchdog != nullptr)
			_watchdog->stop();
		_chainStdout = QString::fromUtf8(_process->readAllStandardOutput());
		_chainStderr = QString::fromUtf8(_process->readAllStandardError());
		const QString tag = _chainIndex >= 0 && _chainIndex < _chain.size()
			? _chain[_chainIndex].tag : QString();
		if (_timedOut)
		{
			_timedOut = false;
			abortChain(QStringLiteral("命令超时（超过 120 秒），已终止进程：%1").arg(tag));
			return;
		}
		if (exitStatus != QProcess::NormalExit)
		{
			abortChain(QStringLiteral("命令进程异常退出：%1").arg(tag));
			return;
		}
		if (exitCode != 0)
		{
			//按命令标识解析可读失败原因（FR-13 条目级形态——与 CLI 输出协议一致）
			QString fail = QStringLiteral("命令失败（%1，退出码 %2）").arg(tag).arg(exitCode);
			if (tag == QStringLiteral("digest"))
			{
				const int idx = _chainStdout.indexOf(QStringLiteral("[digest] 前置校验阻断："));
				if (idx >= 0)
				{
					fail = _chainStdout.mid(idx + QString::fromUtf8("[digest] 前置校验阻断：").size())
						.section(QLatin1Char('\n'), 0, 0).trimmed();
				}
			}
			else if (tag == QStringLiteral("rules_put"))
			{
				const int idx = _chainStderr.indexOf(QStringLiteral("[rules] 回填拒绝（规则层非法）："));
				if (idx >= 0)
				{
					fail = QStringLiteral("规则层回填被拒绝（内容检查未通过）：")
						+ _chainStderr.mid(idx + QString::fromUtf8("[rules] 回填拒绝（规则层非法）：").size())
						.section(QLatin1Char('\n'), 0, 0).trimmed();
				}
			}
			else if (tag == QStringLiteral("validate"))
			{
				//probe 同款解析："[validate] 校验失败：<msg>"+"  - <条目>" bullets
				QString msg;
				QStringList bullets;
				const QStringList lines = _chainStdout.split(QLatin1Char('\n'));
				for (const QString& line : lines)
				{
					const int head = line.indexOf(QStringLiteral("[validate] 校验失败："));
					if (head >= 0)
						msg = line.mid(head + QString::fromUtf8("[validate] 校验失败：").size()).trimmed();
					const int bullet = line.indexOf(QStringLiteral("  - "));
					if (bullet >= 0)
						bullets.append(line.mid(bullet + 4).trimmed());
				}
				fail = msg.isEmpty() ? fail : msg;
				if (msg.contains(QStringLiteral("版本不符")))
					fail = QStringLiteral("格式版本不符，请升级程序或按新格式重新提炼");
				if (!bullets.isEmpty())
					fail += QStringLiteral("\n") + bullets.join(QStringLiteral("\n"));
			}
			else if (tag.startsWith(QStringLiteral("trial_")))
			{
				//引擎 JSON-lines：failed 行 msg+errors（结构化错误）
				QString msg;
				QStringList errors;
				const QStringList lines = _chainStdout.split(QLatin1Char('\n'));
				for (const QString& line : lines)
				{
					const QJsonDocument row = QJsonDocument::fromJson(line.trimmed().toUtf8());
					if (!row.isObject())
						continue;
					const QJsonObject obj = row.object();
					if (obj.value(QStringLiteral("stage")).toString() == QStringLiteral("failed"))
					{
						msg = obj.value(QStringLiteral("msg")).toString();
						const QJsonArray arr = obj.value(QStringLiteral("errors")).toArray();
						for (const QJsonValue& v : arr)
							errors.append(v.toString());
					}
				}
				if (!msg.isEmpty())
					fail = msg + (errors.isEmpty() ? QString()
						: QStringLiteral("\n") + errors.join(QStringLiteral("\n")));
			}
			abortChain(fail);
			return;
		}
		startNextCmd();
	}

	void mStrengthReportImportWizard::abortChain(const QString& failText)
	{
		setChainBusy(false);
		switch (_purpose)
		{
		case PurposeDigest:
			setStepStatus(_digestStatus, false, failText);
			break;
		case PurposeRulesGet:
			setStepStatus(_rulesStatus, false, QStringLiteral("规则层草稿取出失败：%1").arg(failText));
			break;
		case PurposePutValidate:
			_validated = false;
			setStepStatus(_validateStatus, false, failText);
			_validateReport->setPlainText(failText);
			break;
		case PurposeTrial:
			_trialOk = false;
			setStepStatus(_trialStatus, false, failText);
			_trialReport->setPlainText(failText);
			break;
		default:
			break;
		}
		updateNavButtons();
	}

	void mStrengthReportImportWizard::finishChainOk()
	{
		switch (_purpose)
		{
		case PurposeDigest:
			presentDigestReport();
			break;
		case PurposeRulesGet:
			presentRulesDraft();
			break;
		case PurposePutValidate:
			presentValidateOutcome(true);
			break;
		case PurposeTrial:
			presentTrialOutcome(true);
			break;
		default:
			break;
		}
		updateNavButtons();
	}

	//——步②消化报告摘要（08 §2.6 六项）——

	void mStrengthReportImportWizard::presentDigestReport()
	{
		//容器与报告定位：digest 落 <out_dir>/<template_id>.rtdoc 与 <template_id>_digest_report.json
		//WZ-12：多容器（同目录双容器异常序列）取 lastModified 最新，不再字典序 first()
		const QDir outDir(_tmpRoot);
		const QStringList containers = outDir.entryList(QStringList{ QStringLiteral("*.rtdoc") },
			QDir::Files, QDir::Name);
		if (containers.isEmpty())
		{
			_digestOk = false;
			setStepStatus(_digestStatus, false, QStringLiteral("消化报告缺失（容器未产出），请返回上一步重试"));
			return;
		}
		QString newest = containers.first();
		QDateTime newestTime;
		for (const QString& c : containers)
		{
			const QFileInfo fi(outDir.absoluteFilePath(c));
			if (!newestTime.isValid() || fi.lastModified() > newestTime)
			{
				newestTime = fi.lastModified();
				newest = c;
			}
		}
		_containerPath = outDir.absoluteFilePath(newest);
		const QString reportPath = outDir.absoluteFilePath(
			QFileInfo(_containerPath).completeBaseName() + QStringLiteral("_digest_report.json"));
		QJsonDocument report;
		{
			QFile reportFile(reportPath);
			if (reportFile.open(QIODevice::ReadOnly))
				report = QJsonDocument::fromJson(reportFile.readAll());
		}
		if (!report.isObject())
		{
			_digestOk = false;
			setStepStatus(_digestStatus, false, QStringLiteral("消化报告解析失败，请返回上一步重试"));
			return;
		}
		const QJsonObject rep = report.object();
		_templateId = rep.value(QStringLiteral("template_id")).toString();
		_digestWarns.clear();
		const QJsonArray warns = rep.value(QStringLiteral("warnings")).toArray();
		for (const QJsonValue& v : warns)
			_digestWarns.append(v.toString());
		_literalReport = rep.value(QStringLiteral("literal_report")).toArray();
		const QJsonObject stats = rep.value(QStringLiteral("stats")).toObject();
		const QJsonObject hlAcc = stats.value(QStringLiteral("highlight_accounting")).toObject();

		QStringList lines;
		lines.append(QStringLiteral("模板标识：%1").arg(_templateId));
		lines.append(QStringLiteral("部件去向对账：%1/%2")
			.arg(stats.value(QStringLiteral("parts_dispositioned")).toInt())
			.arg(stats.value(QStringLiteral("source_parts")).toInt()));
		lines.append(QStringLiteral("内容块：%1").arg(stats.value(QStringLiteral("blocks")).toInt()));
		lines.append(QStringLiteral("高亮统计：%1（%2）")
			.arg(hlAcc.value(QStringLiteral("accounted")).toInt())
			.arg(hlAcc.value(QStringLiteral("formula")).toString()));
		lines.append(QStringLiteral("高亮跨度：%1 / 唯一短语：%2 / 候选待确认：%3")
			.arg(stats.value(QStringLiteral("highlight_spans")).toInt())
			.arg(stats.value(QStringLiteral("unique_phrases")).toInt())
			.arg(stats.value(QStringLiteral("candidates_for_confirm")).toInt()));
		lines.append(QStringLiteral("---- 特性告警清单（%1 条）----").arg(_digestWarns.size()));
		for (const QString& w : _digestWarns)
			lines.append(QStringLiteral("  [warn] %1").arg(w));
		//非高亮需处置字面报告（按 item 分组计数）
		QMap<QString, int> litGroups;
		for (const QJsonValue& v : _literalReport)
		{
			const QString item = v.toObject().value(QStringLiteral("item")).toString();
			if (!item.isEmpty())
				litGroups[item] += 1;
		}
		lines.append(QStringLiteral("---- 非高亮需处理文字报告（%1 组）----").arg(litGroups.size()));
		for (auto it = litGroups.constBegin(); it != litGroups.constEnd(); ++it)
			lines.append(QStringLiteral("  %1：%2 处").arg(it.key()).arg(it.value()));
		_digestSummary->setPlainText(lines.join(QLatin1Char('\n')));
		_digestOk = true;
		setStepStatus(_digestStatus, true,
			QStringLiteral("消化完成：容器已生成（%1）").arg(_containerPath));
	}

	//——步③规则层编辑引导——

	void mStrengthReportImportWizard::presentRulesDraft()
	{
		QFile file(_rulesEditPath);
		if (!file.open(QIODevice::ReadOnly))
		{
			_rulesLoaded = false;
			setStepStatus(_rulesStatus, false,
				QStringLiteral("规则层草稿读取失败：%1").arg(file.errorString()));
			updateNavButtons();
			return;
		}
		bool parseOk = false;
		_rulesDoc = parseRulesText(file.readAll(), &parseOk);
		file.close();
		if (!parseOk)
		{
			_rulesLoaded = false;
			setStepStatus(_rulesStatus, false, QStringLiteral("规则层草稿 JSON 解析失败"));
			updateNavButtons();
			return;
		}
		const QJsonObject root = _rulesDoc.object();
		const QJsonArray slotsArr = root.value(QStringLiteral("slots")).toArray();
		_initialBinds.clear();
		_initialPolicies.clear();

		_rulesTable->setRowCount(slotsArr.size());
		for (int i = 0; i < slotsArr.size(); ++i)
		{
			const QJsonObject slot = slotsArr.at(i).toObject();
			const QString kind = slot.value(QStringLiteral("kind")).toString();
			const QJsonObject anchor = slot.value(QStringLiteral("anchor")).toObject();
			QString anchorText = anchor.value(QStringLiteral("match")).toString();
			if (anchorText.isEmpty())
				anchorText = anchor.value(QStringLiteral("match_prefix")).toString();
			if (anchorText.isEmpty() && !anchor.isEmpty())
			{
				//图片槽三形态锚点（字面/表格坐标/空段坐标）紧凑呈现
				if (!anchor.value(QStringLiteral("table_match")).toString().isEmpty())
					anchorText = QStringLiteral("表格[%1] %2").arg(
						anchor.value(QStringLiteral("table_match")).toString(),
						anchor.value(QStringLiteral("cell")).toString());
				else if (!anchor.value(QStringLiteral("block")).toString().isEmpty())
					anchorText = QStringLiteral("空段 %1").arg(
						anchor.value(QStringLiteral("block")).toString());
			}
			QString bind = slot.value(QStringLiteral("bind")).toString();
			QString note = slot.value(QStringLiteral("_note")).toString();
			const bool isText = (kind == QStringLiteral("text"));

			//列 0 确认（C-6b"逐条确认"默认勾选）
			QTableWidgetItem* chkItem = new QTableWidgetItem();
			chkItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
			chkItem->setCheckState(Qt::Checked);
			_rulesTable->setItem(i, 0, chkItem);
			//列 1 类型
			_rulesTable->setItem(i, 1, new QTableWidgetItem(kind));
			//列 2 锚点字面
			QTableWidgetItem* anchorItem = new QTableWidgetItem(anchorText);
			anchorItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			_rulesTable->setItem(i, 2, anchorItem);
			//列 3 建议键名（仅 text 槽可编辑——编辑对象=digest 产草稿 JSON 字段级编辑）
			//T2+补丁：text 槽格改可编辑 QComboBox——候选=绑定键字典全键清单（作者从
			//带说明的清单里选），editable=true 手输兜底（未知键允许，定版/校验逻辑
			//零改动）；格 tooltip=键名说明，随选择变化同步；非 text 槽保持只读 item
			const QString keyDesc = bindingKeyDescription(bind);
			const QString bindTip = QStringLiteral("键名说明：%1")
				.arg(keyDesc.isEmpty() ? bind : keyDesc);
			if (isText)
			{
				auto* bindCombo = new QComboBox(_rulesTable);
				bindCombo->setEditable(true);
				bindCombo->addItems(bindingKeyKeys());
				bindCombo->setCurrentText(bind);   //值不在候选中也正常显示（手输兜底）
				bindCombo->setToolTip(bindTip);
				QObject::connect(bindCombo, &QComboBox::currentTextChanged, bindCombo,
					[bindCombo](const QString& text)
					{
						const QString desc = bindingKeyDescription(text);
						bindCombo->setToolTip(QStringLiteral("键名说明：%1")
							.arg(desc.isEmpty() ? text : desc));
					});
				_rulesTable->setCellWidget(i, 3, bindCombo);
			}
			else
			{
				QTableWidgetItem* bindItem = new QTableWidgetItem(bind);
				bindItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				//T2（最小形态）：建议键名格挂字典 tooltip——作者在编辑点即见键语义；
				//未知键 tooltip 显示键名原文（不静默失实）
				bindItem->setToolTip(bindTip);
				_rulesTable->setItem(i, 3, bindItem);
			}
			_initialBinds.append(bind);
			//列 4 缺项策略（仅 text 槽可改；默认=草稿值或"留-"）
			QString policy = slot.value(QStringLiteral("missing_policy")).toString();
			if (isText)
			{
				auto* combo = new QComboBox(_rulesTable);
				combo->addItem(QStringLiteral("留\"-\"并告警"));
				combo->addItem(QStringLiteral("保留模板原文"));
				combo->addItem(QStringLiteral("阻断导出"));
				int cur = 0;
				if (policy == QStringLiteral("keep_highlight"))
					cur = 1;
				else if (policy == QStringLiteral("block_export"))
					cur = 2;
				combo->setCurrentIndex(cur);
				_rulesTable->setCellWidget(i, 4, combo);
			}
			else
			{
				_rulesTable->setItem(i, 4, new QTableWidgetItem(
					policy.isEmpty() ? QStringLiteral("—") : policyDisplay(policy)));
			}
			_initialPolicies.append(policy);
			//列 5 备注（C-6b：keep_highlight 草稿默认槽显著标注——WZ-10 以策略为判定，
			//不再依赖锚点字面全等 "XXX"（变体字面槽同样进入未确认计数））
			if (isText && policy == QStringLiteral("keep_highlight"))
				note = QStringLiteral("非高亮文字（草稿默认保留模板原文），需逐条确认")
				+ (note.isEmpty() ? QString() : QStringLiteral("；") + note);
			QTableWidgetItem* noteItem = new QTableWidgetItem(note);
			noteItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			_rulesTable->setItem(i, 5, noteItem);
		}

		rebuildD18Area();
		rebuildLoopPreview();
		_rulesLoaded = true;
		setStepStatus(_rulesStatus, true,
			QStringLiteral("草稿已取出（%1 槽）；键名/缺项策略可编辑，其余字段经第 4 步最终检查")
			.arg(slotsArr.size()));
	}

	void mStrengthReportImportWizard::rebuildD18Area()
	{
		//C-6(a)：D-18 类人工确认项顶部显式列出——digest 字面报告条目（人工确认候选）
		//+ 表8 行组值落组首行固定待办
		//WZ-04：上限 10 条但溢出不再静默——追加"…等 N 项（完整清单见消化报告）"
		QStringList items;
		int totalItems = 0;
		int shown = 0;
		for (const QJsonValue& v : _literalReport)
		{
			const QJsonObject rec = v.toObject();
			const QString item = rec.value(QStringLiteral("item")).toString();
			if (item.isEmpty())
				continue;
			++totalItems;
			if (shown >= 10)
				continue;
			items.append(QStringLiteral("· %1：%2（%3）").arg(item,
				rec.value(QStringLiteral("text")).toString(),
				rec.value(QStringLiteral("suggest")).toString()));
			++shown;
		}
		QString html = QStringLiteral(
			"<span style='color:#B8860B;'>人工确认项（定版前请逐条确认）：</span><br/>");
		if (items.isEmpty())
			html += QStringLiteral("<span style='color:#B8860B;'>· 文字报告无待确认条目</span><br/>");
		for (const QString& it : items)
			html += QStringLiteral("<span style='color:#B8860B;'>%1</span><br/>").arg(it.toHtmlEscaped());
		if (totalItems > shown)
			html += QStringLiteral("<span style='color:#B8860B;'>· …等 %1 项（此处仅列前 %2 项，"
				"完整清单见步②消化报告的文字报告分组）</span><br/>").arg(totalItems).arg(shown);
		html += QStringLiteral("<span style='color:#B8860B;'>· 表8 行组绑定值落组首行"
			"——定版前请人工确认</span>");
		_d18Area->setText(html);
	}

	void mStrengthReportImportWizard::rebuildLoopPreview()
	{
		//C-6(c)：循环章/表格槽行组声明只读预览（内容口径=08 §2.6 步3；不在此编辑）
		const QJsonObject root = _rulesDoc.object();
		QStringList lines;
		const QJsonArray loops = root.value(QStringLiteral("loop_chapters")).toArray();
		for (const QJsonValue& v : loops)
		{
			const QJsonObject loop = v.toObject();
			const QJsonObject range = loop.value(QStringLiteral("template_range")).toObject();
			lines.append(QStringLiteral("[循环章 %1] 模板范围：%2 → %3 之前")
				.arg(loop.value(QStringLiteral("id")).toString(),
					range.value(QStringLiteral("from_heading")).toString(),
					range.value(QStringLiteral("to_before_heading")).toString()));
			const QJsonArray dead = loop.value(QStringLiteral("dead_chapters")).toArray();
			if (!dead.isEmpty())
			{
				QStringList deadNames;
				for (const QJsonValue& d : dead)
					deadNames.append(d.toString());
				lines.append(QStringLiteral("  废弃章：%1").arg(deadNames.join(QStringLiteral("、"))));
			}
			const QJsonArray rewrites = loop.value(QStringLiteral("literal_rewrite")).toArray();
			for (const QJsonValue& r : rewrites)
			{
				const QJsonObject rw = r.toObject();
				lines.append(QStringLiteral("  文字改写：\"%1\" → %2")
					.arg(rw.value(QStringLiteral("match")).toString(),
						rw.value(QStringLiteral("rewrite_to")).toString()));
			}
			lines.append(QStringLiteral("  实例键：%1  实例绑定：%2")
				.arg(loop.value(QStringLiteral("instance_key")).toString(),
					loop.value(QStringLiteral("instance_binding")).toString()));
		}
		const QJsonArray slotsArr = root.value(QStringLiteral("slots")).toArray();
		for (const QJsonValue& v : slotsArr)
		{
			const QJsonObject slot = v.toObject();
			if (slot.value(QStringLiteral("kind")).toString() != QStringLiteral("table"))
				continue;
			const QJsonObject anchor = slot.value(QStringLiteral("anchor")).toObject();
			const QJsonArray rowTpl = slot.value(QStringLiteral("row_templates")).toArray();
			QStringList tpl;
			for (const QJsonValue& t : rowTpl)
				tpl.append(QString::number(t.toInt()));
			const QString rowLine = QStringLiteral("[表格槽 %1] 行模板=[%2]  grow=%3  shrink=%4")
				.arg(anchor.value(QStringLiteral("match")).toString())
				.arg(tpl.join(QStringLiteral(",")),
					slot.value(QStringLiteral("grow")).toString(),
					slot.value(QStringLiteral("shrink")).toString());
			lines.append(rowLine);
			const QJsonObject group = slot.value(QStringLiteral("row_group")).toObject();
			if (!group.isEmpty())
			{
				lines.append(QStringLiteral("  行组：merge_col=%1 restart_row=%2")
					.arg(group.value(QStringLiteral("merge_col")).toInt())
					.arg(group.value(QStringLiteral("restart_row")).toInt()));
			}
			const QJsonArray cols = slot.value(QStringLiteral("columns")).toArray();
			QStringList colBinds;
			for (const QJsonValue& c : cols)
			{
				const QJsonObject col = c.toObject();
				colBinds.append(QStringLiteral("c%1=%2").arg(col.value(QStringLiteral("col")).toInt())
					.arg(col.value(QStringLiteral("bind")).toString()));
			}
			lines.append(QStringLiteral("  列绑定：%1").arg(colBinds.join(QStringLiteral("  "))));
		}
		if (lines.isEmpty())
			lines.append(QStringLiteral("（本模板无循环章/表格行组声明）"));
		_loopPreview->setPlainText(lines.join(QLatin1Char('\n')));
	}

	bool mStrengthReportImportWizard::applyRulesEdits(QString& err)
	{
		//确认表编辑写回 rules_edit.json（编辑对象=digest 产草稿 JSON，仅改 bind/缺项策略
		//字段；轻校验=非空+全限定字符集提示，硬校验归步④ put）
		if (!_rulesDoc.isObject())
		{
			err = QStringLiteral("规则层草稿尚未取出");
			return false;
		}
		QJsonObject root = _rulesDoc.object();
		QJsonArray slotsArr = root.value(QStringLiteral("slots")).toArray();
		//轻校验键名字符集（O-8：前端只做格式提示——非空/全限定形态字符集，硬校验归
		//步④ put schema 预检，不复制 schema 逻辑——与 C12/Q-01"校验器不复制引擎
		//回退序"同精神）
		const QRegularExpression bindRe(
			QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z0-9_]*)*$"));
		QStringList problems;
		//WZ-02：编辑检出与装载基线有差异→下游旗标失效（put/validate 强制重跑，
		//编辑不再被旧绿灯静默丢弃）
		bool editsChanged = false;
		for (int i = 0; i < slotsArr.size() && i < _rulesTable->rowCount(); ++i)
		{
			QJsonObject slot = slotsArr.at(i).toObject();
			if (slot.value(QStringLiteral("kind")).toString() != QStringLiteral("text"))
				continue;
			//列 3 建议键名：text 槽格=可编辑下拉（取 currentText），兼容回退 item text
			const QString bind = [this, i]()
			{
				if (auto* bindCombo = qobject_cast<QComboBox*>(_rulesTable->cellWidget(i, 3)))
					return bindCombo->currentText().trimmed();
				QTableWidgetItem* bindItem = _rulesTable->item(i, 3);
				return bindItem != nullptr ? bindItem->text().trimmed() : QString();
			}();
			if (bind.isEmpty() || !bindRe.match(bind).hasMatch())
			{
				//WZ-05：示例改为正则可接受的形态（custom.* 不匹配全限定正则，不再举例）
				problems.append(QStringLiteral("第 %1 行键名非法：\"%2\"（非空，全限定形态 "
					"如 conclusion.verdict / custom.备注名；同字符集：字母/数字/下划线，点分层）")
					.arg(i + 1).arg(bind));
				continue;
			}
			if (bind != _initialBinds.value(i))
				editsChanged = true;
			slot.insert(QStringLiteral("bind"), bind);
			//缺项策略：仅在用户改动时写键（草稿未显式声明的槽保持缺省语义，最小 diff）
			QComboBox* combo = qobject_cast<QComboBox*>(_rulesTable->cellWidget(i, 4));
			if (combo != nullptr)
			{
				const QString cur = policyValue(combo->currentIndex());
				if (cur != _initialPolicies.value(i))
				{
					editsChanged = true;
					slot.insert(QStringLiteral("missing_policy"), cur);
				}
			}
			slotsArr.replace(i, slot);
		}
		if (!problems.isEmpty())
		{
			err = QStringLiteral("键名初步检查未过（%1 条，修正后再试；最终检查在下一步「回填并检查」）：\n%2")
				.arg(problems.size()).arg(problems.join(QStringLiteral("\n")));
			return false;
		}
		root.insert(QStringLiteral("slots"), slotsArr);
		_rulesDoc = QJsonDocument(root);
		QFile file(_rulesEditPath);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			err = QStringLiteral("规则层草稿写回失败：%1（%2）").arg(_rulesEditPath, file.errorString());
			return false;
		}
		file.write(_rulesDoc.toJson(QJsonDocument::Indented));
		file.close();
		if (editsChanged)
		{
			//WZ-02：编辑落盘即标脏——步④/⑤旧绿灯失效（步④入口 _validated=false
			//自动重跑 put+validate，编辑必达容器规则层）
			_validated = false;
			_trialOk = false;
		}
		return true;
	}

	//——步④回填+校验 / 步⑤试导出结果呈现——

	void mStrengthReportImportWizard::presentValidateOutcome(bool ok)
	{
		_validated = ok;
		if (!ok)
			return;
		QString msg;
		const QStringList lines = _chainStdout.split(QLatin1Char('\n'));
		for (const QString& line : lines)
		{
			const int idx = line.indexOf(QStringLiteral("[validate]"));
			if (idx >= 0)
				msg += line.mid(idx).trimmed() + QStringLiteral("\n");
		}
		_validateReport->setPlainText(msg.trimmed());
		setStepStatus(_validateStatus, true,
			QStringLiteral("回填与全量检查通过，可进入试导出"));
	}

	void mStrengthReportImportWizard::presentTrialOutcome(bool ok)
	{
		_trialOk = ok;
		if (!ok)
			return;
		QStringList lines;
		lines.append(QStringLiteral("试导出产物（临时文件，向导关闭即清理）："));
		lines.append(QStringLiteral("  空数据集：  %1").arg(
			QDir(_tmpRoot).absoluteFilePath(QStringLiteral("trial_empty.docx"))));
		lines.append(QStringLiteral("  样例数据集：%1").arg(
			QDir(_tmpRoot).absoluteFilePath(QStringLiteral("trial_sample.docx"))));
		lines.append(QStringLiteral("请在 Word 中打开核对样式与 F9 刷新目录后再定版；"
			"导出告警（缺项留空）属缺项策略预期行为。"));
		_trialReport->setPlainText(lines.join(QLatin1Char('\n')));
		setStepStatus(_trialStatus, true, QStringLiteral("试导出完成（两种数据均通过）"));
	}

	//——步⑥定版——

	bool mStrengthReportImportWizard::finalizeContainer(QString& err)
	{
		err.clear();
		QString target = _finalPathEdit->text().trimmed();
		if (target.isEmpty())
		{
			err = QStringLiteral("请先选择定版保存路径");
			return false;
		}
		if (!target.endsWith(QStringLiteral(".rtdoc"), Qt::CaseInsensitive))
			target += QStringLiteral(".rtdoc");
		_finalPathEdit->setText(target);
		const QFileInfo info(target);
		if (!QDir().mkpath(info.absolutePath()))
		{
			err = QStringLiteral("目标目录创建失败：%1").arg(info.absolutePath());
			return false;
		}
		if (info.exists())
		{
			//同路径占用语义（CA-03/T-13 同款：先占用探测再问覆盖——不截断不静默删除）
			QFile probe(target);
			if (!probe.open(QIODevice::ReadWrite))
			{
				err = QStringLiteral("目标文件正被占用或为只读（可能已在 Word 中打开，"
					"或带有只读属性），请解除占用/只读后重试");
				return false;
			}
			probe.close();
			const QMessageBox::StandardButton pick = QMessageBox::question(this,
				QStringLiteral("覆盖确认"),
				QStringLiteral("目标文件已存在，确定覆盖？\n%1").arg(target));
			if (pick != QMessageBox::Yes)
			{
				err = QStringLiteral("已取消定版（原文件保留）");
				return false;
			}
			//WZ-03：先拷后删改为"改备份名→拷贝→清备份"（任何失败原容器完好可回滚）
			const QString bak = target + QStringLiteral(".bak%1")
				.arg(QCoreApplication::applicationPid());
			QFile::remove(bak);
			if (!QFile::rename(target, bak))
			{
				err = QStringLiteral("定版失败：原文件改备份名失败（可能被占用），原文件保留：%1")
					.arg(target);
				return false;
			}
			if (!QFile::copy(_containerPath, target))
			{
				//拷贝失败→回滚备份（WZ-14：不再取新建 QFile 的 errorString，按阶段组装诊断）
				QFile::remove(target);   //清理可能产生的残缺拷贝
				if (!QFile::rename(bak, target))
				{
					err = QStringLiteral("定版复制失败且原文件自动恢复失败，"
						"原文件备份保留于：%1").arg(bak);
					return false;
				}
				err = QStringLiteral("定版复制失败（目标目录不可写或磁盘空间不足？），"
					"原文件已保留：%1").arg(target);
				return false;
			}
			QFile::remove(bak);
		}
		else if (!QFile::copy(_containerPath, target))
		{
			//WZ-14：诊断按目标目录可写性组装（新建 QFile 的 errorString 恒无效信息）
			err = QStringLiteral("定版复制失败（目标目录不可写或磁盘空间不足？）：%1 → %2")
				.arg(_containerPath, target);
			return false;
		}
		_finalized = true;
		_finalizedPath = target;
		return true;
	}

	//——页状态机——

	//WZ-01/02：下游旗标级联失效（源变更/步③编辑变更后，页面绿灯与实际产物一致——
	//后续步骤强制重跑；fromStep 起的全部成功旗标清零）
	void mStrengthReportImportWizard::resetDownstreamFrom(int fromStep)
	{
		if (fromStep <= 1)
		{
			_digestOk = false;
			_containerPath.clear();
			_templateId.clear();
			_digestWarns.clear();
			_literalReport = QJsonArray();
		}
		if (fromStep <= 2)
			_rulesLoaded = false;
		if (fromStep <= 3)
			_validated = false;
		if (fromStep <= 4)
			_trialOk = false;
		//定版产物归属旧源/旧规则层，一并失效
		_finalized = false;
		_finalizedPath.clear();
		_sourceExternallyChanged = false;
	}

	//WZ-06：源指纹快照（步①校验通过时取 mtime/size，步⑥入口复检）
	void mStrengthReportImportWizard::captureSourceFingerprint(const QString& src)
	{
		const QFileInfo info(src);
		_sourceMtimeMs = info.lastModified().toMSecsSinceEpoch();
		_sourceSize = info.size();
		_sourceExternallyChanged = false;
	}

	void mStrengthReportImportWizard::gotoStep(int step)
	{
		if (step < 0 || step > 5 || _chainBusy)
			return;
		_step = step;
		_pages->setCurrentIndex(_step);

		//页入口动作（自动启动对应子进程链；工具/python/引擎脚本定位=B-8 与一期同源复用）
		if (_step == 1 && !_digestOk)
		{
			QString toolsErr;
			const QString toolsDir = mStrengthReportContainerProbe::resolveToolsDir(toolsErr);
			QStringList searched;
			const QString pythonExe = mStrengthReportEngineRunner::resolvePythonExe(&searched);
			_purpose = PurposeDigest;   //工具定位失败也按本页用途收尾呈现
			if (toolsDir.isEmpty() || pythonExe.isEmpty())
			{
				const QString why = toolsDir.isEmpty() ? toolsErr
					: QStringLiteral("未找到可用的 python 解释器（已探测：%1）").arg(searched.join("; "));
				abortChain(why);
				setStepStatus(_digestStatus, false, QStringLiteral("工具定位失败：%1").arg(why));
				_digestSummary->setPlainText(QStringLiteral("工具定位失败（未开始消化）：\n%1").arg(why));
				updateNavButtons();
				return;
			}
			setStepStatus(_digestStatus, true, QStringLiteral("消化中…"));
			QVector<CmdSpec> chain;
			chain.append(CmdSpec{ QStringLiteral("digest"), pythonExe,
				QStringList{ toolsDir + QStringLiteral("/tdr_digest.py"), _sourceDocx, _tmpRoot } });
			runChain(chain, PurposeDigest);
		}
		else if (_step == 2 && !_rulesLoaded)
		{
			QString toolsErr;
			const QString toolsDir = mStrengthReportContainerProbe::resolveToolsDir(toolsErr);
			const QString pythonExe = mStrengthReportEngineRunner::resolvePythonExe();
			if (toolsDir.isEmpty() || pythonExe.isEmpty())
			{
				setStepStatus(_rulesStatus, false, QStringLiteral("工具定位失败：%1")
					.arg(toolsDir.isEmpty() ? toolsErr : QStringLiteral("未找到可用的 python 解释器")));
				updateNavButtons();
				return;
			}
			QFile::remove(_rulesEditPath);
			setStepStatus(_rulesStatus, true, QStringLiteral("规则层草稿取出中…"));
			QVector<CmdSpec> chain;
			chain.append(CmdSpec{ QStringLiteral("rules_get"), pythonExe,
				QStringList{ toolsDir + QStringLiteral("/tdr_rules.py"), QStringLiteral("get"),
					_containerPath, _rulesEditPath } });
			runChain(chain, PurposeRulesGet);
		}
		else if (_step == 3 && !_validated)
		{
			QString toolsErr;
			const QString toolsDir = mStrengthReportContainerProbe::resolveToolsDir(toolsErr);
			const QString pythonExe = mStrengthReportEngineRunner::resolvePythonExe();
			const QString emptyDataset = toolsDir + QStringLiteral("/tdr_empty_dataset.json");
			if (toolsDir.isEmpty() || pythonExe.isEmpty() || !QFile::exists(emptyDataset))
			{
				setStepStatus(_validateStatus, false, QStringLiteral("工具定位失败：%1")
					.arg(toolsDir.isEmpty() ? toolsErr : QStringLiteral("空数据集缺失（%1）").arg(emptyDataset)));
				updateNavButtons();
				return;
			}
			setStepStatus(_validateStatus, true, QStringLiteral("回填与检查中…"));
			QVector<CmdSpec> chain;
			chain.append(CmdSpec{ QStringLiteral("rules_put"), pythonExe,
				QStringList{ toolsDir + QStringLiteral("/tdr_rules.py"), QStringLiteral("put"),
					_containerPath, _rulesEditPath } });
			//校验必须传数据集（S5-2 instance_key 探测：缺数据集误报"校验失败 1 条"，
			//一期批次 2 实测口径）——向导用工具目录内置空数据集
			chain.append(CmdSpec{ QStringLiteral("validate"), pythonExe,
				QStringList{ toolsDir + QStringLiteral("/tdr_validate.py"),
					_containerPath, emptyDataset } });
			runChain(chain, PurposePutValidate);
		}
		else if (_step == 4 && !_trialOk)
		{
			QString toolsErr;
			const QString toolsDir = mStrengthReportContainerProbe::resolveToolsDir(toolsErr);
			const QString pythonExe = mStrengthReportEngineRunner::resolvePythonExe();
			//B-8：引擎脚本定位与一期运行器同源（resolveEngineScript 内=tools 磁盘候选
			//与 qrc 字节比对、陈旧/缺失走 qrc 释放两级——D2-4 向导不新增 qrc 条目）
			QString engineErr;
			const QString engineScript = mStrengthReportEngineRunner::resolveEngineScript(engineErr);
			if (toolsDir.isEmpty() || pythonExe.isEmpty() || engineScript.isEmpty())
			{
				QString why;
				if (toolsDir.isEmpty())
					why = toolsErr;
				else if (pythonExe.isEmpty())
					why = QStringLiteral("未找到可用的 python 解释器");
				else
					why = engineErr;
				setStepStatus(_trialStatus, false, QStringLiteral("工具定位失败：%1").arg(why));
				updateNavButtons();
				return;
			}
			setStepStatus(_trialStatus, true, QStringLiteral("试导出中…（两种数据依次执行）"));
			QVector<CmdSpec> chain;
			chain.append(CmdSpec{ QStringLiteral("trial_empty"), pythonExe,
				QStringList{ engineScript, _containerPath,
					toolsDir + QStringLiteral("/tdr_empty_dataset.json"),
					QDir(_tmpRoot).absoluteFilePath(QStringLiteral("trial_empty.docx")) } });
			chain.append(CmdSpec{ QStringLiteral("trial_sample"), pythonExe,
				QStringList{ engineScript, _containerPath,
					toolsDir + QStringLiteral("/tdr_sample_dataset.json"),
					QDir(_tmpRoot).absoluteFilePath(QStringLiteral("trial_sample.docx")) } });
			runChain(chain, PurposeTrial);
		}
		else if (_step == 5)
		{
			//WZ-06：步⑥入口对源做 mtime/size 快检——外部修改黄字提示（定版基于开始时快照）
			const QFileInfo srcInfo(_sourceDocx);
			const bool srcChanged = srcInfo.exists()
				&& (srcInfo.lastModified().toMSecsSinceEpoch() != _sourceMtimeMs
					|| srcInfo.size() != _sourceSize);
			if (srcChanged && !_sourceExternallyChanged)
			{
				_sourceExternallyChanged = true;
				_finalStatus->setText(QStringLiteral(
					"<span style='color:#B8860B;'>%1</span>")
					.arg(QStringLiteral("提示：源文件在提炼后已被修改，本次定版基于开始向导时"
						"的版本（提炼取自当时磁盘快照）；如需按新版本提炼，请返回第 1 步换源重跑")));
			}
			if (_finalPathEdit->text().trimmed().isEmpty())
			{
				//默认落位=文档目录/<template_id>.rtdoc（不指向主对话框当前容器，防误覆盖）
				QDir dir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
				_finalPathEdit->setText(dir.absoluteFilePath(
					(_templateId.isEmpty() ? QStringLiteral("template") : _templateId)
					+ QStringLiteral(".rtdoc")));
			}
		}
		updateNavButtons();
	}

	void mStrengthReportImportWizard::setChainBusy(bool busy)
	{
		_chainBusy = busy;
		updateNavButtons();
	}

	void mStrengthReportImportWizard::setStepStatus(QLabel* label, bool ok, const QString& text)
	{
		if (label == nullptr)
			return;
		label->setText(QStringLiteral("<span style='color:%1;'>%2</span>")
			.arg(ok ? QStringLiteral("#008000") : QStringLiteral("#C00000"), text.toHtmlEscaped()));
	}

	void mStrengthReportImportWizard::updateNavButtons()
	{
		static const char* kStepNames[] =
		{ "选择源文档", "一键消化", "规则层编辑引导", "回填与检查", "试导出", "定版" };
		_stepLabel->setText(QStringLiteral("第 %1/6 步 · %2%3")
			.arg(_step + 1).arg(kStepNames[_step],
				_chainBusy ? QStringLiteral("（任务进行中…）") : QString()));

		_backBtn->setEnabled(_step > 0 && !_chainBusy);
		bool nextOk = !_chainBusy;
		switch (_step)
		{
		case 0: nextOk = nextOk && _sourceOk; break;
		case 1: nextOk = nextOk && _digestOk; break;
		case 2: nextOk = nextOk && _rulesLoaded; break;
		case 3: nextOk = nextOk && _validated; break;
		case 4: nextOk = nextOk && _trialOk; break;
		case 5: nextOk = nextOk && _finalized; break;
		default: break;
		}
		_nextBtn->setEnabled(nextOk);
		_nextBtn->setText(_step == 5 ? QStringLiteral("完成")
			: (_step == 0 ? QStringLiteral("开始消化")
				: (_step == 2 ? QStringLiteral("回填并检查")
					: (_step == 3 ? QStringLiteral("试导出")
						: (_step == 4 ? QStringLiteral("定版") : QStringLiteral("下一步"))))));
		_cancelBtn->setText(_finalized ? QStringLiteral("关闭") : QStringLiteral("取消"));
	}

	void mStrengthReportImportWizard::cleanupTempDir()
	{
		if (!_tmpRoot.isEmpty())
			QDir(_tmpRoot).removeRecursively();
	}

	//——关闭语义（E2.7：任一步取消/失败=临时件清理，零容器落用户路径；已定版后
	//关闭仍按"完成"语义交付定版路径）——

	void mStrengthReportImportWizard::reject()
	{
		if (_chainBusy)
		{
			const QMessageBox::StandardButton pick = QMessageBox::question(this,
				QStringLiteral("任务进行中"),
				QStringLiteral("向导任务正在进行中，确认取消并退出？（临时产物将被清理）"));
			if (pick != QMessageBox::Yes)
				return;
			if (_watchdog != nullptr)
				_watchdog->stop();
			if (_process != nullptr && _process->state() != QProcess::NotRunning)
			{
				_process->disconnect(this);
				_process->kill();
				_process->waitForFinished(2000);
			}
			_chainBusy = false;
		}
		if (_finalized)
		{
			cleanupTempDir();   //定版产物已在用户路径，临时件清理后按完成语义关闭
			accept();
			return;
		}
		cleanupTempDir();
		QDialog::reject();
	}
}
