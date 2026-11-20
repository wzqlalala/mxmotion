//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mUiShotController.h"

//PostWindow
#include "mPostInfoHub.h"
#include "mPostBrowserWindow.h"
#include "mPost3DResult.h"
#include "mMechGlobalSignals.h"
//MechCommon
#include "mPanelHostProvider.h"
//CommonWidgets
#include "mFoldSection.h"

//Qt
#include <QCoreApplication>
#include <QApplication>
#include <QElapsedTimer>	//【临时诊断R14】soak 计时（收尾拆除）
#include <QMainWindow>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFontInfo>
#include <QHash>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QTimer>
#include <functional>
#include <iostream>

namespace PostWindow
{
	namespace
	{
		/*
		* 退出码（与 MGUIMesh::BatchCLIExitCode 编号风格对齐，便于脚本统一处理）
		*/
		enum class UiShotExitCode
		{
			Success = 0,
			InvalidArguments = 1,
			CaseFileMissing = 3,
			PanelWidgetUnavailable = 4,
			SaveFailed = 5,
		};

		/*
		* 面板注册表：注册名 → 创建动作（面板部件经 panelHostRef()->mainWidget()
		* 通用获取，不按面板单列访问器）。收录 createXXX 带 caseid 默认值的族；
		* 必须显式工况的面板后续按需补录。
		*/
			QHash<QString, std::function<void()>> panelRegistry()
			{
				QHash<QString, std::function<void()>> reg;
				auto* hub = mPostInfoHub::getInstance();
				reg.insert("measure", [hub] { hub->createMeasureTool(); });
				reg.insert("tag", [hub] { hub->createTagPanel(); });
				reg.insert("sectionforce", [hub] { hub->createSectionForce(); });
				reg.insert("explode", [hub] { hub->createExplode(); });
				reg.insert("note", [hub] { hub->createNote(); });
				//派生族（2026-11-04 源视口收敛走查补录）：走完整 hub createXxx 链——
				//含面板打开经 createXxxSig→树槽→editXxx 绑定源视口的整条新链路
				reg.insert("linear", [hub] { hub->createLinearSuperposition(-1); });
				reg.insert("scalar", [hub] { hub->createScalarOperation(-1); });
				//animation 走真实点击链（clickAnimationSig→树槽→createAnimation）：
				//含 resolveActiveCloudViewName/contextCaseId 解析，一键生成实际可跑通
				//（直调 createAnimation(1,-1) 时 caseid=-1，newAniPanel 查库为空静默返回）
				reg.insert("animation", [bus = MechCommon::mMechGlobalSignals::getInstance()] { emit bus->clickAnimationSig(1); });
				//自定义动画同走真实点击链（用户 2026-11-05 报障：点"自定义"参数栏空骨架）
				reg.insert("customanimation", [bus = MechCommon::mMechGlobalSignals::getInstance()] { emit bus->clickAnimationSig(2); });
				reg.insert("section", [hub] { hub->createSection(-1); });
				reg.insert("isocreate", [hub] { hub->createIsosurface(-1); });
				reg.insert("vector", [hub] { hub->createVector(-1); });
				reg.insert("streamline", [hub] { hub->createStreamLine(-1); });
				reg.insert("set", [hub] { hub->createSet(-1); });
			//云图参数面板：带 -Case 时导入收尾已把它挂为 Info 主部件
			//（import3DResult→setMainWidget），再走 create3DResult 会重建复位——
			//已是当前主部件时直接复用；无 case 才新建（空态）
			reg.insert("cloud", [hub] {
				if (MechCommon::panelHostRef()
					&& MechCommon::panelHostRef()->mainWidget() == hub->cloudResultPanel().get())
					return;
				hub->create3DResult(false);
			});
			//无参创建的参数面板（窄栏裁切普查用；挂载均经 mPostInfoHub::mountToHost）
			reg.insert("deformation", [hub] { hub->createDeformation(); });
			reg.insert("isosurface", [hub] { hub->openIsosurfacePanel(); });
				reg.insert("advancedquery", [hub] { hub->createAdvancedQuery(); });
				reg.insert("curveextract", [hub] { hub->createCurveExtract(); });
				reg.insert("query", [hub] { hub->createQuery(); });
				//对比视频面板（2026-11-16 动画同步重构补录，createCompVideo 无默认参显式传 -1）
				reg.insert("compvideo", [hub] { hub->createCompVideo(-1); });
			reg.insert("sectioncut", [hub] { hub->createSectionCut(); });
			//坐标系面板（2026-11-18 界面搭建补录；直角档，三类型经面板"类型"下拉切换）
			reg.insert("coordinate", [hub] { hub->createCoordinate(0); });
			reg.insert("parray", [hub] { hub->createParrayMimage(-1); });
			reg.insert("stepsequence", [hub] { hub->createStepSequence(); });
			reg.insert("envelope", [hub] { hub->createEnvelope(); });
			reg.insert("vectorplot", [hub] { hub->createVectorPlot(); });
			reg.insert("customvideo", [hub] { hub->createCustomVideo(); });
			reg.insert("customscreenshot", [hub] { hub->createCustomScreenshot(); });
				return reg;
			}

		/*
		* 弹窗注册表：弹窗对象名 → 打开它的面板槽（懒创建路径）。
		* 新弹窗在此登记一行即可被 --ui-shot-dialog 截图。
		* anim-tab-v13（T-3）：动画设置弹窗挂在主窗口工具栏内容件下（非面板子树），
		* 打开槽走云图面板转发（slot_openAnimSettingsForShot→控件条 openSettingsPopup），
		* 部件查找由 --ui-shot-dialog 循环内的 allWidgets 兜底承接（见弹窗段注释）
		*/
		QHash<QString, const char*> dialogSlots()
		{
			return { { "mLabelPopup", "slot_labelButtonClicked" },
				     { "mReviewPopup", "slot_showReviewPopup" },

				     { "mAnimSettingsPopup", "slot_openAnimSettingsForShot" },
				     { "animTypeMenu", "slot_openAnimTypeMenuForShot" },
				     { "queryValuesPopup", "slot_valuesOpenClicked" },
				     { "vectorDisplayPopup", "slot_displayOpenClicked" } };
		}

		/*
		* anim-tab-v13（T-3）：按 objectName 全 widgets 扫描兜底——动画设置弹窗
		* （Qt::Popup 顶层）与动画工具栏均挂在主窗口工具栏区，不在 Info dock 面板子树内，
		* panel->findChild 找不到；通用兜底不影响既有面板子树查找路径
		*/
		QWidget* findWidgetByNameGlobally(const QString& name)
		{
			for (QWidget* w : QApplication::allWidgets())
			{
				if (w != nullptr && w->objectName() == name)
					return w;
			}
			return nullptr;
		}

		/*
		* 点分调用形式：<objectName>.<槽名>(<bool|int>[,<bool|int>])。
		* 弹窗懒创建，折叠组/下拉等目标子控件仅在弹窗打开后才存在——该形式
		* 不在面板阶段执行，留待弹窗序列打开后对其子树 findChild 解析
		* （如 mLabelShowSec.onFoldClicked(false,false)、_rvYCombo.setCurrentIndex(3)）。
		* 返回值只回答"是否为点分形式"，执行见 invokeChildSlot。
		*/
		bool isChildSlotSpec(const QString& spec)
		{
			const int open = spec.indexOf('(');
			return open > 0 && spec.endsWith(QLatin1Char(')'))
				&& spec.lastIndexOf(QLatin1Char('.'), open) > 0;
		}

		/*
		* 后处理树按显示名递归定位（--ui-shot-treeclick）：在（代理）模型上深度优先
		* 匹配 Qt::DisplayRole，命中即返回该层首个匹配索引；沿途展开父节点，保证
		* 程序化选中在可见行上（委托绘制/选中态与真实点击一致）
		*/
		QModelIndex matchTreeIndex(QTreeView* view, const QModelIndex& parent, const QString& name)
		{
			QAbstractItemModel* m = view->model();
			for (int r = 0; r < m->rowCount(parent); ++r)
			{
				const QModelIndex idx = m->index(r, 0, parent);
				if (idx.data(Qt::DisplayRole).toString() == name)
					return idx;
				if (m->hasChildren(idx))
				{
					view->setExpanded(idx, true);
					const QModelIndex hit = matchTreeIndex(view, idx, name);
					if (hit.isValid())
						return hit;
				}
			}
			return QModelIndex();
		}

		void invokeChildSlot(QWidget* root, const QString& spec)
		{
			const int open = spec.indexOf('(');
			const int dot = spec.lastIndexOf(QLatin1Char('.'), open);
			const QString objName = spec.left(dot);
			const QString slot = spec.mid(dot + 1, open - dot - 1);
			const QString args = spec.mid(open + 1, spec.size() - open - 2);

			//参数支持 bool 与整数（整数用于 QComboBox::setCurrentIndex(int) 等
			//轴切换驱动）；同一次调用内必须同类型
			QStringList toks;
			const QString trimmed = args.trimmed();
			if (!trimmed.isEmpty())
				toks = trimmed.split(QLatin1Char(','));
			bool allBool = true, allInt = true;
			for (const QString& a : toks)
			{
				const QString t = a.trimmed();
				if (t.compare(QLatin1String("true"), Qt::CaseInsensitive) != 0
					&& t.compare(QLatin1String("false"), Qt::CaseInsensitive) != 0)
					allBool = false;
				bool okInt = false;
				t.toInt(&okInt);
				if (!okInt)
					allInt = false;
			}
			if (!toks.isEmpty() && !allBool && !allInt)
			{
				qWarning().noquote() << QString::fromUtf8(
					"UI_SHOT_WARN 点分调用仅支持 bool/int 参数，已跳过: %1").arg(spec);
				return;
			}


			QObject* target = root->findChild<QObject*>(objName);
			//TC-28 测试槽补齐：目标=弹窗自身（如 mAnimSettingsPopup.setDisplayStyleForShot(0)
			//——findChild 只搜子树不含 self，弹窗级 Q_INVOKABLE 注入槽按 objectName 兜底匹配根）
			if (target == nullptr && root->objectName() == objName)
				target = root;
			//TC 状态矩阵：工具栏子树目标（animRealtimeContent/mAnimSettingsPopup 等，
			//挂在主窗口工具栏区，不在面板/dlg 子树）——allWidgets 全局兜底
			if (target == nullptr)
				target = findWidgetByNameGlobally(objName);
			if (target == nullptr)
			{
				qWarning().noquote() << QString::fromUtf8(
					"UI_SHOT_WARN 点分调用目标未找到: %1").arg(objName);
				return;
			}

			const QByteArray slotName = slot.toLatin1();
			if (allInt && !toks.isEmpty())
			{
				const int v0 = toks.at(0).trimmed().toInt();
				if (toks.size() >= 2)
					QMetaObject::invokeMethod(target, slotName.constData(),
						Q_ARG(int, v0), Q_ARG(int, toks.at(1).trimmed().toInt()));
				else
					QMetaObject::invokeMethod(target, slotName.constData(), Q_ARG(int, v0));
				return;
			}

			QStringList flags;
			for (const QString& a : toks)
			{
				const QString t = a.trimmed();
				if (t.isEmpty())
					continue;
				flags << ((t.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0)
					? QLatin1String("1") : QLatin1String("0"));
			}
			if (flags.size() >= 2)
				QMetaObject::invokeMethod(target, slotName.constData(),
					Q_ARG(bool, flags.at(0) == QLatin1String("1")),
					Q_ARG(bool, flags.at(1) == QLatin1String("1")));
			else if (flags.size() == 1)
				QMetaObject::invokeMethod(target, slotName.constData(),
					Q_ARG(bool, flags.at(0) == QLatin1String("1")));
			else
				QMetaObject::invokeMethod(target, slotName.constData());
		}

		bool parseArguments(const QStringList& arguments, mUiShotController::Config& config, QString& err)
		{
			for (int i = 0; i < arguments.size(); ++i)
			{
				const QString& a = arguments[i];
				const bool hasValue = (i + 1 < arguments.size());
				if (a == QLatin1String("--ui-shot") && hasValue)
					config.panel = arguments[++i];
				else if (a == QLatin1String("--ui-shot-case") && hasValue)
					config.caseFile = arguments[++i];
				else if (a == QLatin1String("--ui-shot-menu") && hasValue)
					config.menuPage = arguments[++i];
				else if (a == QLatin1String("--ui-shot-dialog") && hasValue)
					config.dialogs.append(arguments[++i]);
				else if (a == QLatin1String("--ui-shot-invoke") && hasValue)
					config.invokes.append(arguments[++i]);
				else if (a == QLatin1String("--ui-shot-treeclick") && hasValue)
					config.treeClicks.append(arguments[++i]);
				else if (a == QLatin1String("--ui-shot-repanel") && hasValue)
					config.rePanels.append(arguments[++i]);
				else if (a == QLatin1String("--ui-shot-settext") && hasValue)
					config.setTexts.append(arguments[++i]);
				else if (a == QLatin1String("--ui-shot-tlshot") && hasValue)
					config.tlShots.append(arguments[++i]);
				else if (a == QLatin1String("--ui-shot-arm"))
					config.arm = true;
				else if (a == QLatin1String("--ui-shot-collapse"))
					config.collapse = true;
				else if (a == QLatin1String("--ui-shot-window"))
					config.wholeWindow = true;
				else if (a == QLatin1String("--ui-shot-dump") && hasValue)
					config.dumpPath = arguments[++i];
				else if (a == QLatin1String("--ui-shot-out") && hasValue)
					config.outPath = arguments[++i];
				else if (a == QLatin1String("--ui-shot-settle") && hasValue)
					config.settleMs = arguments[++i].toInt();
				else if (a == QLatin1String("--ui-shot-wait") && hasValue)
					config.waitMs = arguments[++i].toInt();
				else if (a == QLatin1String("--ui-shot-panelwidth") && hasValue)
					config.panelWidth = arguments[++i].toInt();
				//【临时诊断R14】soak 观察窗时长（收尾拆除）
				else if (a == QLatin1String("--ui-shot-soak-ms") && hasValue)
					config.soakMs = arguments[++i].toInt();
				//兜底须放最后：放在 out/settle/wait 之前会让它们永远不可达（已踩坑）
				else if (a.startsWith(QLatin1String("--ui-shot")))
					qWarning().noquote() << QString::fromUtf8("UI_SHOT_WARN 未识别的参数（已忽略）: %1").arg(a);
			}
			if (config.panel.isEmpty())
				return false; // 未请求截图模式
			if (!panelRegistry().contains(config.panel))
			{
				err = QString::fromUtf8("未知面板注册名: %1（可选: %2）")
					.arg(config.panel, mUiShotController::panelNames().join(QChar(',')));
				return false;
			}
			if (config.outPath.isEmpty())
				config.outPath = QString::fromUtf8("ui_shot_%1.png").arg(config.panel);
			for (const QString& rp : config.rePanels)
			{
				if (!panelRegistry().contains(rp))
				{
					err = QString::fromUtf8("未知 repanel 注册名: %1（可选: %2）")
						.arg(rp, mUiShotController::panelNames().join(QChar(',')));
					return false;
				}
			}
			return true;
		}

		void finish(int code, const QString& message)
		{
			if (!message.isEmpty())
			{
				if (code == 0)
					qInfo().noquote() << message;
				else
					qWarning().noquote() << message;
			}
			// 交给事件循环退出，保证日志 flush 与收尾槽先执行
			QTimer::singleShot(0, qApp, [code] { QCoreApplication::exit(code); });
		}

		/*
		* 阻塞等待 delayMs（事件循环不被打断，面板布局/样式正常收敛）
		*/
		void sleepInLoop(int delayMs)
		{
			QEventLoop loop;
			QTimer::singleShot(delayMs, &loop, &QEventLoop::quit);
			loop.exec();
		}

		/*
		* 控件树导出（--ui-shot-dump）：面板子树内全部带文本控件 →
		* [{class, objectName, text, rect(相对面板左上), visible}]，
		* QTableWidget 的表头逐列补录（kind=th，列宽/位置是对齐关键项）。
		* 供 Auto_Postpage tools/ui_align.py 与原型控件清单按文本匹配算 Δx/Δy/Δw/Δh。
		*/
		void dumpControlTree(QWidget* panel, QWidget* target, const QString& path)
		{
			QJsonArray arr;
			std::function<void(QWidget*)> walk = [&](QWidget* w) {
				for (QObject* c : w->children())
				{
					auto* cw = qobject_cast<QWidget*>(c);
					if (cw == nullptr)
						continue;
					QJsonObject o;
					o.insert("class", QString::fromLatin1(cw->metaObject()->className()));
					o.insert("objectName", cw->objectName());
					QString text;
					if (auto* lb = qobject_cast<QLabel*>(cw))
						text = lb->text();
					else if (auto* pb = qobject_cast<QPushButton*>(cw))
						text = pb->text();
					else if (auto* le = qobject_cast<QLineEdit*>(cw))
						text = le->placeholderText().isEmpty() ? le->text() : le->placeholderText();
					else if (auto* cb = qobject_cast<QComboBox*>(cw))
					{
						text = cb->currentText();
						//下拉框全部选项（ui-shot 点分驱动 setCurrentIndex 选帧/切变量的索引依据）
						QJsonArray items;
						for (int i = 0; i < cb->count(); ++i)
							items.append(cb->itemText(i));
						if (!items.isEmpty())
							o.insert("items", items);
					}
					else if (auto* chk = qobject_cast<QCheckBox*>(cw))
						text = chk->text();
					else if (auto* rb = qobject_cast<QRadioButton*>(cw))
						text = rb->text();
					const QPoint tl = cw->mapTo(target, QPoint(0, 0));	//相对截取基准（面板或弹窗）
					o.insert("rect", QJsonArray{ tl.x(), tl.y(), cw->width(), cw->height() });
					o.insert("visible", cw->isVisible());
					//R3 盲区闭合：enabled 入 dump（禁用矩阵机检依据——dump 原无该字段，
					//禁用态只能靠截图灰度目测）
					o.insert("enabled", cw->isEnabled());
					//R4 字号证据（anim-tab-v13 用户否决项①取证）：QFontInfo 解析后的实际
					//渲染字体（像素字号/字重/字族首段）——与原型 getComputedStyle 的
					//font-size/font-weight/font-family 对表；QFontInfo 走字体匹配后的真值，
					//不受 QSS 声明与实际解析差异干扰
					const QFontInfo fit = cw->fontInfo();
					o.insert("fontPx", fit.pixelSize());
					o.insert("fontWt", fit.weight());
					o.insert("fontFam", QString(fit.family()).section(',', 0, 0));
					if (!text.isEmpty())
						o.insert("text", text);
					arr.append(o);
					walk(cw);
				}
			};
			walk(target);
			for (auto* tw : target->findChildren<QTableWidget*>())
			{
				//表格自身也入清单（isVisible 是排查"grab 未绘制"类问题的关键证据）
				const QPoint tl = tw->mapTo(target, QPoint(0, 0));
				QJsonObject o;
				o.insert("class", QString::fromLatin1("QTableWidget"));
				o.insert("objectName", tw->objectName());
				o.insert("text", QString());
				o.insert("rect", QJsonArray{ tl.x(), tl.y(), tw->width(), tw->height() });
				o.insert("visible", tw->isVisible());
				arr.append(o);
				auto* hd = tw->horizontalHeader();
				for (int i = 0; i < hd->count(); ++i)
				{
					//sectionRect 为 protected：用公开的 viewport 位置 + 尺寸合成
					const int pos = hd->sectionViewportPosition(i);
					const int size = hd->sectionSize(i);
					if (size <= 0)
						continue;
					const QRect r(pos, 0, size, hd->height());
					QTableWidgetItem* it = tw->horizontalHeaderItem(i);
					QJsonObject o;
					o.insert("class", QString::fromLatin1("th"));
					o.insert("objectName", QString::fromLatin1("%1.col%2").arg(tw->objectName()).arg(i));
					o.insert("text", it ? it->text() : QString());
					const QPoint tl = hd->mapTo(target, r.topLeft());
					o.insert("rect", QJsonArray{ tl.x(), tl.y(), r.width(), r.height() });
					o.insert("visible", true);
					arr.append(o);
				}
			}
			QJsonObject root;
			root.insert("panel", QJsonArray{ target->width(), target->height() });
			root.insert("controls", arr);
			QFile f(path);
			if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
				return;
			f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
			f.close();
		}

		/*
		* 弹窗外壳带透明阴影边距时（轻投影方案，uiShotContent 属性标记内容宿主），
		* 截图/控件导出锚定内容宿主而非外壳——否则透明边距混入对比基线
		*/
		QWidget* shotTargetOf(QWidget* dlg)
		{
			const auto kids = dlg->findChildren<QWidget*>();
			for (QWidget* c : kids)
			{
				if (c->property("uiShotContent").toBool())
					return c;
			}
			return dlg;
		}

		void run(const mUiShotController::Config& config)
		{
			// 1) 可选：载入结果文件（与菜单"云图结果"同一入口，导入收尾自动激活云图视口；
			//    测量等面板不收 caseid，激活视口即其数据上下文——见 newMeasureToolPanel）
			//    caseFile 以"|"分隔多文件：复现文件对话框多选组合（旧格式 mxdb0+帧文件序列）
			if (!config.caseFile.isEmpty())
			{
				QStringList caseFiles;
				for (const QString& f : config.caseFile.split(QLatin1Char('|')))
				{
					const QString t = f.trimmed();
					if (!t.isEmpty())
						caseFiles.append(t);
				}
				QStringList missingFiles;
				for (const QString& f : caseFiles)
				{
					if (!QFileInfo::exists(f))
						missingFiles.append(f);
				}
				if (!missingFiles.isEmpty())
				{
					finish(static_cast<int>(UiShotExitCode::CaseFileMissing),
						QString::fromUtf8("UI_SHOT_ERR 结果文件不存在: %1").arg(missingFiles.join(QLatin1String(", "))));
					return;
				}

				bool importFinished = false;
				QEventLoop loop;
				QTimer timeout;
				timeout.setSingleShot(true);
				// 先接线再触发导入，避免快速完成时漏信号
				QObject::connect(mPostInfoHub::getInstance()->cloudResultPanel().get(),
					&mPost3DResultPanel::finishedReadPostFileSig, &loop,
					[&importFinished, &loop](const QString&) { importFinished = true; loop.quit(); });
				QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
				timeout.start(config.waitMs);

				mPostBrowserWindow::getInstance()->openPostResultFiles(caseFiles);
				loop.exec();

				if (importFinished)
					qInfo().noquote() << QString::fromUtf8("UI_SHOT 结果导入完成: %1").arg(caseFiles.join(QLatin1String("|")));
				else
					// 信号超时不判死：导入仍在途时面板为空态，截图仍有布局对照价值
					qWarning().noquote() << QString::fromUtf8(
						"UI_SHOT_WARN 等待导入完成超时(%1ms)，按当前状态继续").arg(config.waitMs);
			}

			// 2) 激活目标面板
			panelRegistry()[config.panel]();
			sleepInLoop(config.settleMs);

			// 2.3) 可选：切换 Ribbon 页（--ui-shot-menu）。面板激活链常带自己的页签
			//      切换（如 create3DResult 落在"结果"页），整窗截图要验其它页的工具栏
			//      布局时在此改签（如"云图"页的切换/过滤组）。中文页名经 PowerShell
			//      →cmd 引号链会拆坏参数行（实测 0xC0000005），这里收 ASCII 别名转译
			if (!config.menuPage.isEmpty())
			{
				QString page = config.menuPage;
				if (page == QLatin1String("cloud"))
					page = QString::fromUtf8("云图");
				else if (page == QLatin1String("anim"))
					page = QString::fromUtf8("动画");
				else if (page == QLatin1String("result"))
					page = QString::fromUtf8("结果");
				else if (page == QLatin1String("model"))
					page = QString::fromUtf8("模型");
				MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow(page);
				sleepInLoop(400);
			}

			// 3) 截当前面板部件（setMainWidget 挂载的 Info dock 主部件）；
			//    --ui-shot-dialog 指定弹窗对象名时改为截弹窗（懒创建的经对应槽打开）
			QWidget* panel = MechCommon::panelHostRef() ? MechCommon::panelHostRef()->mainWidget() : nullptr;
			if (panel == nullptr)
			{
				finish(static_cast<int>(UiShotExitCode::PanelWidgetUnavailable),
					QString::fromUtf8("UI_SHOT_ERR 未取得面板部件（宿主未注册或面板未挂载）"));
				return;
			}

			// 2.4) 可选：钉住面板宽（--ui-shot-panelwidth，窄参数栏回归）。Info dock 可被
			//      用户拖窄（最小 300），字段列随之收窄——固定宽后表单按该宽重排，复现
			//      "行控件被折叠块右缘裁掉"类缺陷；grab 按控件自身尺寸渲染，不受宿主裁剪影响
			if (config.panelWidth > 0)
			{
				panel->setFixedWidth(config.panelWidth);
				sleepInLoop(300);
			}

			// 2.5) 可选：拾取武装态（按钮勾选 + "正在拾取节点…"）
			if (config.arm)
			{
				QMetaObject::invokeMethod(panel, "slot_pickNodeClicked", Q_ARG(bool, true));
				sleepInLoop(400);
			}

			// 2.6) 可选：收起次级折叠组（首组保持展开，对齐原型"仅收工具区"的形态）
			if (config.collapse)
			{
				const auto folds = panel->findChildren<MechMainWindow::FoldSection*>();
				for (int i = 1; i < folds.size(); ++i)
					folds[i]->setExpanded(false);
				sleepInLoop(300);
			}

			// 2.7) 可选：调用面板槽驱动状态（--ui-shot-invoke，如空态提示；
			//      通用入口不绑定具体面板，槽名由 states 配方给出）。
			//      点分形式对面板子控件同样执行（面板阶段子控件已存在，如
			//      comboVariable.setCurrentIndex(1) 切变量驱动真实渲染态）；
			//      懒创建弹窗内的目标仍由弹窗序列打开后处理
			for (const QString& slot : config.invokes)
			{
				//"post:" 前缀＝推迟到 repanel 之后执行（重开面板后的按钮驱动），
				//面板激活阶段跳过
				if (slot.startsWith(QLatin1String("post:")))
					continue;
				if (isChildSlotSpec(slot))
				{
					invokeChildSlot(panel, slot);
					sleepInLoop(300);
					continue;
				}
				//invokeMethod 失败静默会浪费一轮排查（Q_INVOKABLE 缺失等），必须告警
				if (!QMetaObject::invokeMethod(panel, slot.toLatin1().constData()))
					qWarning().noquote() << QString::fromUtf8(
						"UI_SHOT_WARN 槽调用失败（未找到/不可见）: %1").arg(slot);
				sleepInLoop(300);
			}

			// 2.8) 可选：后处理树节点单击（--ui-shot-treeclick 节点名）：按显示名定位
			//      代理索引，选中后显式重放 slot_singleClickItem（程序化选区不触发
			//      clicked，与 slot_viewportActivated 的重放派发同款惯例）——驱动
			//      "单击云图/缓存节点"等树交互链（L2 走查）
			if (!config.treeClicks.isEmpty())
			{
				if (mPostBrowserWindow* browser = mPostBrowserWindow::getInstance())
				{
					QTreeView* tree = browser->getPostTree();
					for (const QString& spec : config.treeClicks)
					{
						//preset<N>＝"云图"根下第 N 个子节点（1 起）：中文节点名作命令行
						//实参经 PowerShell/控制台代码页链路会引发启动期 0xC0000005
						//（实测 7/7），ASCII 别名绕开；cloudroot＝"云图"根自身；
						//其余取值按显示名全树匹配
						QModelIndex idx;
						if (spec.startsWith(QLatin1String("preset")))
						{
							bool ok = false;
							const int nth = spec.mid(6).toInt(&ok);
							const QModelIndex cloudRoot = matchTreeIndex(tree, QModelIndex(), QString::fromUtf8("云图"));
							if (ok && nth > 0 && cloudRoot.isValid())
								idx = tree->model()->index(nth - 1, 0, cloudRoot);
						}
						else if (spec == QLatin1String("cloudroot"))
							idx = matchTreeIndex(tree, QModelIndex(), QString::fromUtf8("云图"));
						//tag<N>＝"标签"根下第 N 个子节点（1 起）：标签树节点单击链的
						//ASCII 驱动别名（同 preset<N> 的代码页规避理由）
						else if (spec.startsWith(QLatin1String("tag")) && spec.size() > 3)
						{
							bool ok = false;
							const int nth = spec.mid(3).toInt(&ok);
							const QModelIndex tagRoot = matchTreeIndex(tree, QModelIndex(), QString::fromUtf8("标签"));
							if (ok && nth > 0 && tagRoot.isValid())
								idx = tree->model()->index(nth - 1, 0, tagRoot);
						}
						else
						{
							idx = matchTreeIndex(tree, QModelIndex(), spec);
						}
						if (!idx.isValid())
						{
							qWarning().noquote() << QString::fromUtf8(
								"UI_SHOT_WARN 树节点未找到（检查显示名/展开态）: %1").arg(spec);
							continue;
						}
						tree->setCurrentIndex(idx);
						if (tree->selectionModel() != nullptr)
						{
							tree->selectionModel()->select(idx,
								QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
						}
						sleepInLoop(200);
						QMetaObject::invokeMethod(tree, "slot_singleClickItem");
						sleepInLoop(800);	//单击派发→面板绑定/重绘收敛
					}
				}
			}

				// 2.85) 可选：重新激活面板（--ui-shot-repanel 可多次）：驱动
				//       "面板被树单击切走后再打开"/多段"面板切走再切回"序列。
				//       2026-11-13 解耦：本块原嵌在 treeclick 分支内——无 treeclick 时
				//       repanel 与 post:invoke 静默不执行（走查踩坑，曾以"必须配
				//       treeclick"留档绕开）；现 repanel 独立于 treeclick 生效，
				//       post:invoke 仍在全部 repanel 完成后执行（语义不变）。
				for (const QString& rp : config.rePanels)
				{
					panelRegistry()[rp]();
					sleepInLoop(800);
					//重激活可能挂载了另一面板实例：截图/驱动基准随宿主当前主部件更新
					if (MechCommon::panelHostRef() && MechCommon::panelHostRef()->mainWidget() != nullptr)
						panel = MechCommon::panelHostRef()->mainWidget();
				}
				if (!config.rePanels.isEmpty())
				{
					for (const QString& slot : config.invokes)
					{
						if (!slot.startsWith(QLatin1String("post:")))
							continue;
						const QString spec = slot.mid(5);
						if (isChildSlotSpec(spec))
						{
							invokeChildSlot(panel, spec);
							sleepInLoop(300);
							continue;
						}
						if (!QMetaObject::invokeMethod(panel, spec.toLatin1().constData()))
							qWarning().noquote() << QString::fromUtf8(
								"UI_SHOT_WARN 槽调用失败（未找到/不可见）: %1").arg(spec);
						sleepInLoop(300);
					}
				}

			// 2.9) 可选：注入文本（--ui-shot-settext objectName=text）——复现长数值串
			//      等文本驱动的布局状态（窄栏行裁切回归：QLineEdit 最小宽随文本增长）
			for (const QString& spec : config.setTexts)
			{
				const int eq = spec.indexOf(QLatin1Char('='));
				QLineEdit* target = eq > 0
					? qobject_cast<QLineEdit*>(panel->findChild<QWidget*>(spec.left(eq)))
					: nullptr;
				if (target == nullptr)
				{
					qWarning().noquote() << QString::fromUtf8(
						"UI_SHOT_WARN settext 未找到输入框（objectName 必须先在面板代码里设置）: %1").arg(spec);
					continue;
				}
				target->setText(spec.mid(eq + 1));
			}
			if (!config.setTexts.isEmpty())
				sleepInLoop(200);

			// 3) 弹窗序列：逐个打开并单独截图（Qt 弹窗互斥，同开只留最后一个）
			QWidget* lastDlg = nullptr;
			const QString base = config.outPath;
			const QString stem = base.left(base.lastIndexOf(QLatin1Char('.')) > 0
				? base.lastIndexOf(QLatin1Char('.')) : base.length());
			const auto saveOne = [](QWidget* w, const QString& file) -> bool {
				const QString f = QDir::toNativeSeparators(file);
				QDir().mkpath(QFileInfo(f).absolutePath());
				if (!w->grab().save(f, "PNG"))
					return false;
				qInfo().noquote() << QString::fromUtf8("UI_SHOT_OK %1 (%2x%3)")
					.arg(f).arg(w->grab().width()).arg(w->grab().height());
				return true;
			};

			for (const QString& name : config.dialogs)
			{
				QWidget* dlg = panel->findChild<QWidget*>(name);
				if (dlg == nullptr)
					dlg = findWidgetByNameGlobally(name);	//anim-tab-v13：工具栏子树弹窗兜底
				if (dlg == nullptr)
				{
					const QByteArray slot = dialogSlots().value(name, nullptr);
					if (slot == nullptr)
					{
						finish(static_cast<int>(UiShotExitCode::PanelWidgetUnavailable),
							QString::fromUtf8("UI_SHOT_ERR 未知弹窗: %1（可登记至 dialogSlots）").arg(name));
						return;
					}
					//D-19：面板宿主 mainWidget 随 dock 重构可能不再持有转发槽（实测解析到
					//mPostMeasureTool，槽实际在 mPost3DResultPanel）——panel 元对象无该方法时
					//按元对象全 widgets 扫描持有者兜底；panel 持有槽时零行为变化
					QObject* recv = panel;
					const QByteArray sig = QByteArray(slot) + "()";
					if (recv->metaObject()->indexOfMethod(sig.constData()) < 0)
					{
						for (QWidget* w : QApplication::allWidgets())
						{
							if (w != nullptr && w->metaObject()->indexOfMethod(sig.constData()) >= 0)
							{
								recv = w;
								break;
							}
						}
					}
					QMetaObject::invokeMethod(recv, slot);
					sleepInLoop(400);
					dlg = panel->findChild<QWidget*>(name);
					if (dlg == nullptr)
						dlg = findWidgetByNameGlobally(name);
				}
				if (dlg == nullptr)
				{
					finish(static_cast<int>(UiShotExitCode::PanelWidgetUnavailable),
						QString::fromUtf8("UI_SHOT_ERR 未取得弹窗部件: %1").arg(name));
					return;
				}
				//弹窗子控件驱动：点分调用（如折叠组收起/展开）。逐条等待让
				//0 时长折叠动画与布局链走完再截图
				for (const QString& spec : config.invokes)
				{
					if (!isChildSlotSpec(spec))
						continue;
					invokeChildSlot(dlg, spec);
					sleepInLoop(300);
				}
				if (!saveOne(shotTargetOf(dlg), stem + "_" + name + ".png"))
				{
					finish(static_cast<int>(UiShotExitCode::SaveFailed),
						QString::fromUtf8("UI_SHOT_ERR 弹窗保存失败: %1").arg(name));
					return;
				}
				//临时验证自绘阴影：另存窗口整幅（含阴影边距），确认后删除
				saveOne(dlg, stem + "_" + name + "_win.png");
				lastDlg = dlg;
				dlg->deleteLater();
				//弹窗用后即焚（延迟销毁）：Qt::Popup 存活到应用退出序列会
				//触发 0xC0000005（实测），截图完成后立即安排销毁
			}

			// 3.5) 可选：截非宿主顶层窗口（--ui-shot-tlshot，reparent 出的独立弹窗，
			//      如信息标签配置窗 openNotesAsDialog——不属于面板子树，-Dialog 寻址不到）。
			//      取值 active=当前活动窗（弹窗刚打开即为活动窗，规避中文标题命令行
			//      实参链路问题）；否则按 windowTitle 精确匹配首个可见顶层窗
			for (const QString& spec : config.tlShots)
			{
				QWidget* tl = nullptr;
				if (spec.compare(QLatin1String("active"), Qt::CaseInsensitive) == 0)
					tl = QApplication::activeWindow();
				else
				{
					for (QWidget* w : QApplication::topLevelWidgets())
					{
						if (w->isVisible() && w->windowTitle() == spec)
						{
							tl = w;
							break;
						}
					}
				}
				if (tl == nullptr)
				{
					qWarning().noquote() << QString::fromUtf8(
						"UI_SHOT_WARN 顶层窗口未找到（active=活动窗，否则按标题精确匹配）: %1").arg(spec);
					continue;
				}
				const QString tag = (spec.compare(QLatin1String("active"), Qt::CaseInsensitive) == 0)
					? QString::fromUtf8("active") : spec;
				saveOne(tl, stem + "_tl_" + tag + ".png");
			}

			// 4) 面板截图
			// 4.1) 可选：导出控件树 JSON（--ui-shot-dump），供逐控件对齐对比
			if (!config.dumpPath.isEmpty())
			{
				dumpControlTree(panel, lastDlg ? shotTargetOf(lastDlg) : panel, config.dumpPath);
				//anim-tab-v13（T-3）：动画工具栏在主窗口顶部工具栏区（非面板子树）——
				//存在时另存控件树（<dump>_animbar.json），FR-01/02 逐控件对齐用
				QWidget* animBar = findWidgetByNameGlobally(QStringLiteral("animDisplayBar"));
				if (animBar != nullptr)
					dumpControlTree(panel, animBar, config.dumpPath + QStringLiteral("_animbar.json"));
			}

			// 4.2) 收纳到内容高度：Info dock 会把面板拉高到可用高度，底部 stretch 产生
			// 大片空白；对照基线（原型元素截图）是内容高度，这里按 sizeHint 收敛后截图。
			// 弹窗自身固定尺寸，跳过收纳
			panel->setFixedHeight(panel->sizeHint().height());
			sleepInLoop(150);

			const QString outPath = QDir::toNativeSeparators(config.outPath);
			QDir().mkpath(QFileInfo(outPath).absolutePath());
			const QPixmap shot = panel->grab();
			if (!shot.save(outPath, "PNG"))
			{
				finish(static_cast<int>(UiShotExitCode::SaveFailed),
					QString::fromUtf8("UI_SHOT_ERR 保存失败: %1").arg(outPath));
				return;
			}
			qInfo().noquote() << QString::fromUtf8("UI_SHOT_OK %1 (%2x%3) 面板=%4")
				.arg(outPath).arg(shot.width()).arg(shot.height()).arg(config.panel);

			// 5) 可选：整窗截图（--ui-shot-window）：含 3D 视口——测量标注等
			//    面板外渲染只能整窗验证（面板 grab 不含视口）。
			//    面板可能已被树单击/repanel 切换卸载（setParent(nullptr)），
			//    此时 window() 是面板自身——回退取顶层主窗口，保证截到完整主窗
			if (config.wholeWindow)
			{
				const QString winPath = QDir::toNativeSeparators(stem + "_viewport.png");
				QWidget* win = panel->window();
				if (panel->parentWidget() == nullptr)
				{
					win = nullptr;
					for (QWidget* w : QApplication::topLevelWidgets())
					{
						if (qobject_cast<QMainWindow*>(w) != nullptr && w->isVisible())
						{
							win = w;
							break;
						}
					}
				}
				if (win != nullptr && win->grab().save(winPath, "PNG"))
					qInfo().noquote() << QString::fromUtf8("UI_SHOT_OK %1 (%2x%3) 整窗")
					.arg(winPath).arg(win->grab().width()).arg(win->grab().height());
			}
			//【临时诊断R14】soak 观察窗（播放卡顿与内存泄漏专项，收尾拆除）：
			//动画设置弹窗真实链注入循环档 → animRealtimeContent.setPlayForShot(true)
			//真实播放链启动 → 事件循环保活 N ms（心跳）→ 停播返回
			if (config.soakMs > 0)
			{
				qInfo().noquote() << QString::fromUtf8("UI_SHOT_SOAK 开始: %1 ms").arg(config.soakMs);
				//① 动画设置弹窗（懒创建，走 dialogSlots 登记的真实打开槽，与 --ui-shot-dialog 同路）
				QByteArray openSlot(dialogSlots().value(QStringLiteral("mAnimSettingsPopup"),
					"slot_openAnimSettingsForShot"));
				QObject* recv = nullptr;
				const QByteArray probe = openSlot + "()";
				for (QWidget* w : QApplication::allWidgets())
				{
					if (w != nullptr && w->metaObject()->indexOfMethod(probe.constData()) >= 0)
					{
						recv = w;
						break;
					}
				}
				if (recv != nullptr)
					QMetaObject::invokeMethod(recv, openSlot.constData());
				else
					qWarning().noquote() << QString::fromUtf8(
						"UI_SHOT_SOAK_WARN 弹窗打开槽持有者未找到: %1").arg(QString(openSlot));
				sleepInLoop(400);
				//② 循环档注入（radio checked→toggled→控件条应用，与用户点击同路径；1=循环）
				if (QWidget* popup = findWidgetByNameGlobally(QStringLiteral("mAnimSettingsPopup")))
				{
					invokeChildSlot(popup, QStringLiteral("mAnimSettingsPopup.setPlayStyleForShot(1)"));
					sleepInLoop(300);
					popup->close();
					popup->deleteLater();	//弹窗用后即焚（dialog 段同款纪律，Qt::Popup 存活至退出会崩）
				}
				else
					qWarning().noquote() << QString::fromUtf8(
						"UI_SHOT_SOAK_WARN 未取得设置弹窗（循环档未注入，按会话默认档播放）");
				sleepInLoop(200);
				//③ 播放（真实链：setPlayForShot→toggled→slot_playToggled→面板闸）
				if (QWidget* bar = findWidgetByNameGlobally(QStringLiteral("animRealtimeContent")))
					QMetaObject::invokeMethod(bar, "setPlayForShot", Q_ARG(bool, true));
				else
					qWarning().noquote() << QString::fromUtf8(
						"UI_SHOT_SOAK_WARN 未取得动画工具栏（animRealtimeContent），播放未启动");
				sleepInLoop(1000);
				//④ 保活观察窗（10s 分片让出 + 60s 心跳；diagTick 堆账本按秒输出到调试输出）
				QElapsedTimer soak;
				soak.start();
				while (soak.elapsed() < config.soakMs)
				{
					const int chunk = qMin(10000, static_cast<int>(config.soakMs - soak.elapsed()));
					sleepInLoop(chunk);
					if (soak.elapsed() % 60000 < 10000)
						qInfo().noquote() << QString::fromUtf8("UI_SHOT_SOAK 心跳 t=%1s")
							.arg(soak.elapsed() / 1000);
				}
				//⑤ 停播收尾
				if (QWidget* bar = findWidgetByNameGlobally(QStringLiteral("animRealtimeContent")))
					QMetaObject::invokeMethod(bar, "setPlayForShot", Q_ARG(bool, false));
				sleepInLoop(500);
				qInfo().noquote() << QString::fromUtf8("UI_SHOT_SOAK 完成");
			}
			finish(static_cast<int>(UiShotExitCode::Success), QString());
		}
	}

	QStringList mUiShotController::panelNames()
	{
		return panelRegistry().keys();
	}

	int mUiShotController::scheduleIfRequested(const QStringList& arguments)
	{
		Config config;
		QString err;
		if (!parseArguments(arguments, config, err))
		{
			if (!err.isEmpty())
			{
				qWarning().noquote() << QString::fromUtf8("UI_SHOT_ERR %1").arg(err);
				QCoreApplication::exit(static_cast<int>(UiShotExitCode::InvalidArguments));
				return static_cast<int>(UiShotExitCode::InvalidArguments);
			}
			return -1;
		}

		qInfo().noquote() << QString::fromUtf8("UI_SHOT 计划任务: 面板=%1 结果=%2 输出=%3")
			.arg(config.panel, config.caseFile.isEmpty() ? QString::fromUtf8("(无)") : config.caseFile, config.outPath);
		// 主窗 show/布局/ribbon 装配完成后执行；时长不敏感（一次性工具路径）
		QTimer::singleShot(1500, qApp, [config] { run(config); });
		return 0;
	}
}
