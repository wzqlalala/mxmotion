// PostCurve 曲线允许重名（R23-T3）无头自测（纯控制台断言，退出码=失败数）
//
// 验证 R23 提交链 378fa7a2（T1 模型去查重）+ a9b7a48b（T2 引擎键↔节点映射）的
// 模型层行为与视图层键规则规格。分五节：
//   A 模型层·同名创建（普通/复数/柱状）成功且节点独立（数据/样式互不覆盖；
//     文档层断言 series.name 相同但为两个节点）
//   B kName 重命名为既有名成功 + 空名守卫仍在 + 撤销/重做/增删后 id/名稳定
//   C 粘贴 " (N)" 后缀回归（同名加后缀/剪切原位粘贴=原名回归/跨图表原名）
//   D 视图层键规则镜像（ensureEngineCurveKeys 的独立复算规格锁定：无碰撞键=名/
//     碰撞 名#2/#3/字面 "名#2" 共存不串键/改名后重算/双射不变式）+ 图例文本
//     优先级镜像（标签文本 > 模型名 > 键名）+ 模板叠加 " (N)" 后缀镜像
//   E pch/复数/柱状无碰撞抽样回归（复数 a/b 截齐、XY→柱状提取转换名保留、
//     同名曲线整批转柱状名各自保留）
//
// 与 tests/PostCurveAscii 同款设定：直接编译 src/PostCurve 模型层源文件
// （BUILD_STATIC 剥导出宏），不链 PostCurve DLL；mCurveDocController.cpp 的
// report*（信息上报）引用 MxFunctions 两个符号，测试内给空实现（不链
// MxFunctions DLL，getInstance 返回空且测试路径不触 report*）。
// 视图成员函数 ensureEngineCurveKeys/makeCurveSettings 位于 mCurveChartView.cpp
// （QWidget/mPlotView 链，无法无头链接），D 节以独立复算锁定其文档口径
// （CurveReadMe §1.2 / R23-T2 评审推演），真实视图接线列入手动验收清单。
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"
#include "CurveCore/CurveDefine.h"

//MxFunctions 空实现桩：BUILD_STATIC 下 MXFUNCTIONS_EXPORT 为空（见
//mxfunctions_global.h），成员函数可在本测试内定义；mCurveDocController.cpp 的
//reportInfo/reportWarning/reportError 引用 getInstance 与 outputMessageSig
//(int 重载)，测试路径不触 report*，getInstance 返回空指针安全
#include "mGlobalSignals.h"
namespace MxFunctions
{
	mGlobalSignals* mGlobalSignals::getInstance() { return nullptr; }
	void mGlobalSignals::outputMessageSig(const QString&, bool, bool) {}
	void mGlobalSignals::outputMessageSig(int, const QString&, bool, bool) {}
}

//QT
#include <QCoreApplication>
#include <QColor>
#include <QHash>
#include <QSet>

//STL
#include <cstdio>
#include <string>

using namespace PostCurve;

namespace
{
	int g_cases = 0;
	int g_failures = 0;

	void check(bool ok, const char* what)
	{
		++g_cases;
		if (!ok)
		{
			++g_failures;
			std::printf("[FAIL] %s\n", what);
		}
	}

	void checkEq(const QString& got, const QString& want, const char* what)
	{
		++g_cases;
		if (got != want)
		{
			++g_failures;
			std::printf("[FAIL] %s (got [%s] want [%s])\n", what,
				got.toUtf8().constData(), want.toUtf8().constData());
		}
	}

	void report(const char* section)
	{
		std::printf("-- %s: cases=%d failures=%d\n", section, g_cases, g_failures);
	}

	//图表内 Curve 类型子节点数（图表直属子含 X1/Y1/Legend/Header/Footer 固定子）
	int curveCount(CurveDocument* doc, int chartId)
	{
		int n = 0;
		for (int id : doc->childNodeIds(chartId))
		{
			if (doc->nodeType(id) == CurveNodeType::Curve)
				++n;
		}
		return n;
	}

	//收集图表内 Curve 子节点 id（树序）
	QList<int> curveIds(CurveDocument* doc, int chartId)
	{
		QList<int> out;
		for (int id : doc->childNodeIds(chartId))
		{
			if (doc->nodeType(id) == CurveNodeType::Curve)
				out.append(id);
		}
		return out;
	}

	bool dvEq(const QVector<double>& got, const QVector<double>& want)
	{
		return got == want;
	}

	//--- A 节：同名创建与节点独立 ---

	void sectionA()
	{
		mCurveDocController* ctl = mCurveDocController::getInstance();
		CurveDocument* doc = ctl->document();
		const int pageId = ctl->addPage(QStringLiteral("A"));
		const int chartId = ctl->addChart(pageId);
		check(chartId > 0, "A0 addChart succeeds");

		//A1 同图表同名普通曲线创建成功：两个节点、同名（文档层断言 series.name
		//相同但为两个节点）、树名与渲染名同步
		const int idA = ctl->addCurve(chartId, QStringLiteral("STRAIN"));
		const int idB = ctl->addCurve(chartId, QStringLiteral("STRAIN"));
		check(idA > 0 && idB > 0, "A1 same-name addCurve returns valid ids twice");
		check(idA != idB, "A1 same-name curves are two distinct nodes");
		checkEq(doc->node(idA) ? doc->node(idA)->series.name : QString(),
			QStringLiteral("STRAIN"), "A1 node A series.name == STRAIN");
		checkEq(doc->node(idB) ? doc->node(idB)->series.name : QString(),
			QStringLiteral("STRAIN"), "A1 node B series.name == STRAIN (same name, second node)");
		checkEq(doc->nodeName(idA), QStringLiteral("STRAIN"), "A1 node A tree name == STRAIN");
		checkEq(doc->nodeName(idB), QStringLiteral("STRAIN"), "A1 node B tree name == STRAIN");
		check(curveCount(doc, chartId) == 2, "A1 chart holds 2 curve nodes");

		//A2 数据独立：互不覆盖
		ctl->setSeriesData(idA, QVector<double>{ 0, 1, 2 }, QVector<double>{ 1, 2, 3 });
		ctl->setSeriesData(idB, QVector<double>{ 0, 1, 2 }, QVector<double>{ 10, 20, 30 });
		check(dvEq(doc->node(idA)->series.yData, QVector<double>{ 1, 2, 3 }),
			"A2 node A keeps its own yData");
		check(dvEq(doc->node(idB)->series.yData, QVector<double>{ 10, 20, 30 }),
			"A2 node B keeps its own yData (not overwritten by A)");

		//A3 样式独立：kColor 各自保留
		ctl->setNodeProperty(idA, CurveProp::kColor, QColor(255, 0, 0));
		ctl->setNodeProperty(idB, CurveProp::kColor, QColor(0, 128, 0));
		check(doc->nodeProperty(idA, CurveProp::kColor).value<QColor>() == QColor(255, 0, 0),
			"A3 node A keeps its own color");
		check(doc->nodeProperty(idB, CurveProp::kColor).value<QColor>() == QColor(0, 128, 0),
			"A3 node B keeps its own color (styles independent)");

		//A4 复数曲线同名创建成功且两路数据独立
		const int cA = ctl->addComplexCurve(chartId, QStringLiteral("CPLX"));
		const int cB = ctl->addComplexCurve(chartId, QStringLiteral("CPLX"));
		check(cA > 0 && cB > 0 && cA != cB, "A4 same-name addComplexCurve succeeds twice");
		ctl->setComplexSeriesData(cA, QVector<double>{ 1, 2, 3 },
			QVector<double>{ 1, 2, 3 }, QVector<double>{ 0, 90, 180 });
		ctl->setComplexSeriesData(cB, QVector<double>{ 1, 2, 3 },
			QVector<double>{ 7, 8, 9 }, QVector<double>{ 10, 20, 30 });
		check(doc->node(cA)->series.isComplex() && doc->node(cB)->series.isComplex(),
			"A4 both complex curves marked complex");
		check(dvEq(doc->node(cA)->series.yData, QVector<double>{ 1, 2, 3 }),
			"A4 complex node A keeps own magnitude");
		check(dvEq(doc->node(cB)->series.yData, QVector<double>{ 7, 8, 9 }),
			"A4 complex node B keeps own magnitude (not overwritten)");

		//A5 柱状曲线同名创建成功且类目签独立
		const int bA = ctl->addBarCurve(chartId, QStringLiteral("BAR"));
		const int bB = ctl->addBarCurve(chartId, QStringLiteral("BAR"));
		check(bA > 0 && bB > 0 && bA != bB, "A5 same-name addBarCurve succeeds twice");
		ctl->setSeriesTickLabels(bA, QStringList{ "f1", "f2" });
		ctl->setSeriesTickLabels(bB, QStringList{ "g1", "g2", "g3" });
		check(doc->node(bA)->series.xTickLabels == QStringList{ "f1", "f2" },
			"A5 bar node A keeps own tick labels");
		check(doc->node(bB)->series.xTickLabels == QStringList{ "g1", "g2", "g3" },
			"A5 bar node B keeps own tick labels");

		//A6 同名曲线可分别选中（选择中枢按节点 id 不按名）
		ctl->setCurrentNodeId(idA);
		check(ctl->currentNodeId() == idA, "A6 select node A by id");
		ctl->setCurrentNodeId(idB);
		check(ctl->currentNodeId() == idB, "A6 select node B by id (same name, correct node)");

		report("A same-name creation/independence");
	}

	//--- B 节：重命名/空名守卫/撤销重做稳定 ---

	void sectionB()
	{
		mCurveDocController* ctl = mCurveDocController::getInstance();
		CurveDocument* doc = ctl->document();
		const int pageId = ctl->addPage(QStringLiteral("B"));
		const int chartId = ctl->addChart(pageId);
		const int idA = ctl->addCurve(chartId, QStringLiteral("STRAIN"));
		const int idB = ctl->addCurve(chartId, QStringLiteral("VELOCITY"));
		check(idA > 0 && idB > 0, "B0 setup two curves");
		ctl->setSeriesData(idA, QVector<double>{ 0, 1, 2 }, QVector<double>{ 1, 2, 3 });

		//B1 kName 重命名为既有名成功（R23 取消四处拒绝之一）：series.name 联动同步
		ctl->setNodeProperty(idB, CurveProp::kName, QStringLiteral("STRAIN"));
		checkEq(doc->nodeName(idB), QStringLiteral("STRAIN"), "B1 rename to existing name succeeds");
		checkEq(doc->node(idB)->series.name, QStringLiteral("STRAIN"),
			"B1 series.name synced after rename");
		checkEq(doc->nodeName(idA), QStringLiteral("STRAIN"), "B1 node A untouched by rename");
		check(curveCount(doc, chartId) == 2, "B1 still two distinct nodes after rename");

		//B2 重命名可撤销/重做（undo 恢复原名，redo 再撞名）
		ctl->undo();
		checkEq(doc->nodeName(idB), QStringLiteral("VELOCITY"), "B2 undo restores old name");
		checkEq(doc->node(idB)->series.name, QStringLiteral("VELOCITY"), "B2 undo restores series.name");
		ctl->redo();
		checkEq(doc->nodeName(idB), QStringLiteral("STRAIN"), "B2 redo re-applies collision name");

		//B3 空名守卫仍在：空白名创建回退默认名（Curve N），不落空名
		const int idEmpty = ctl->addCurve(chartId, QStringLiteral("   "));
		check(idEmpty > 0, "B3 whitespace-only name still creates a curve");
		const QString emptyName = doc->nodeName(idEmpty);
		check(!emptyName.trimmed().isEmpty(), "B3 whitespace name falls back to default name");
		check(emptyName.startsWith(QStringLiteral("Curve ")), "B3 default name is Curve N form");
		//trimmed 语义保留："  STRAIN  " 创建后 trim 为 STRAIN（与既有同名共存）
		const int idTrim = ctl->addCurve(chartId, QStringLiteral("  STRAIN  "));
		checkEq(doc->nodeName(idTrim), QStringLiteral("STRAIN"), "B3 name trimmed on create");
		check(curveCount(doc, chartId) == 4, "B3 chart now holds 4 curves (2x STRAIN + default + trim)");

		//B4 页空名守卫仍在（页/视口本就不查重，但空白名拒绝不变）
		const int page2 = ctl->addPage(QStringLiteral("B-page2"));
		ctl->setPageName(page2, QStringLiteral("   "));
		checkEq(doc->nodeName(page2), QStringLiteral("B-page2"), "B4 setPageName whitespace rejected");

		//B5 删除重名曲线之一再撤销：id 不变恢复、数据完整（增删后键映射稳定的前提）
		ctl->removeNode(idA);
		check(curveCount(doc, chartId) == 3, "B5 remove one STRAIN curve");
		check(doc->node(idA) == nullptr, "B5 removed node gone");
		ctl->undo();
		check(doc->node(idA) != nullptr && doc->nodeType(idA) == CurveNodeType::Curve,
			"B5 undo restores removed node with same id");
		checkEq(doc->nodeName(idA), QStringLiteral("STRAIN"), "B5 restored node keeps name");
		check(dvEq(doc->node(idA)->series.yData, QVector<double>{ 1, 2, 3 }),
			"B5 restored node keeps its own data");
		ctl->redo();
		ctl->undo();
		check(doc->node(idA) != nullptr, "B5 redo/undo cycle keeps node stable");

		report("B rename/guard/undo-redo");
	}

	//--- C 节：粘贴 " (N)" 后缀回归（兼容保留路径零变化） ---

	void sectionC()
	{
		mCurveDocController* ctl = mCurveDocController::getInstance();
		CurveDocument* doc = ctl->document();
		const int pageId = ctl->addPage(QStringLiteral("C"));
		const int chartId = ctl->addChart(pageId);
		const int solo = ctl->addCurve(chartId, QStringLiteral("SOLO"));
		ctl->setSeriesData(solo, QVector<double>{ 0, 1 }, QVector<double>{ 5, 6 });

		//C1 同名粘贴自动加 " (N)" 后缀（既有语义保留）
		check(ctl->copyNode(solo), "C1 copy succeeds");
		const int p1 = ctl->pasteNode(chartId);
		check(p1 > 0, "C1 paste returns new id");
		checkEq(doc->nodeName(p1), QStringLiteral("SOLO (2)"), "C1 paste appends (2) suffix");
		checkEq(doc->node(p1)->series.name, QStringLiteral("SOLO (2)"), "C1 series.name synced on paste");
		const int p2 = ctl->pasteNode(chartId);
		checkEq(doc->nodeName(p2), QStringLiteral("SOLO (3)"), "C2 second paste appends (3)");

		//C3 剪切后原位粘贴=原名回归（保留原名优先语义）
		check(ctl->cutNode(solo), "C3 cut original SOLO");
		check(doc->node(solo) == nullptr, "C3 cut removes original");
		const int p3 = ctl->pasteNode(chartId);
		check(p3 > 0, "C3 paste after cut succeeds");
		checkEq(doc->nodeName(p3), QStringLiteral("SOLO"), "C3 in-place paste restores original name");

		//C4 跨图表粘贴无同名不改名
		const int chart2 = ctl->addChart(pageId);
		check(ctl->copyNode(p3), "C4 copy again");
		const int p4 = ctl->pasteNode(chart2);
		checkEq(doc->nodeName(p4), QStringLiteral("SOLO"), "C4 paste into empty chart keeps name");

		//C5 粘贴可撤销/重做（undo 仅移除粘贴节点，redo 恢复同名）
		ctl->undo();
		check(doc->node(p4) == nullptr, "C5 undo removes pasted node");
		ctl->redo();
		check(doc->node(p4) != nullptr
			&& doc->nodeName(p4) == QStringLiteral("SOLO"), "C5 redo restores pasted node");

		//C6 粘贴曲线的数据随节点独立（快照深拷贝）
		ctl->setSeriesData(p3, QVector<double>{ 0, 1 }, QVector<double>{ 50, 60 });
		check(dvEq(doc->node(p3)->series.yData, QVector<double>{ 50, 60 }),
			"C6 pasted node data editable independently");
		check(dvEq(doc->node(p1)->series.yData, QVector<double>{ 5, 6 }),
			"C6 other pasted node data untouched");

		report("C paste suffix regression");
	}

	//--- D 节：视图层键规则/图例优先级/模板后缀——独立复算镜像 ---
	//
	//产品实现在 mCurveChartView（QWidget/mPlotView/Qwt 链）无法无头链接，此处按
	//CurveReadMe §1.2 与 R23-T2 评审口径在测试侧独立复算同一规则，锁定规格：
	//键生成（ensureEngineCurveKeys 的字面移植）：按树序逐节点，模型名首次出现
	//键=名；第 N 次重复依次尝试 名#2/名#3… 取首个不与任何模型名及已生成键冲突者。

	QHash<int, QString> keyRule(const QStringList& names)
	{
		const QSet<QString> allNames = names.toSet();   //Qt 5.13 无迭代器对构造
		QHash<QString, int> seen;
		QSet<QString> used;
		QHash<int, QString> out;
		for (int i = 0; i < names.size(); ++i)
		{
			const int nth = ++seen[names.at(i)];
			QString key = names.at(i);
			if (nth > 1)
			{
				int suffix = 2;
				do
				{
					key = names.at(i) + QStringLiteral("#") + QString::number(suffix);
					++suffix;
				} while (allNames.contains(key) || used.contains(key));
			}
			used.insert(key);
			out.insert(i, key);
		}
		return out;
	}

	//键映射不变式：键数=节点数、键两两互异、键互异于"非自身节点"的模型名
	//（防串键）、首次出现键=名。等价于 key↔node 双射可反查（engineNodeByKey）
	void checkKeyInvariants(const QStringList& names, const char* tag)
	{
		const QHash<int, QString> m = keyRule(names);
		check(m.size() == names.size(), (std::string(tag) + ": one key per node").c_str());
		const QSet<QString> allNames = names.toSet();   //Qt 5.13 无迭代器对构造
		QSet<QString> keys;
		for (auto it = m.cbegin(); it != m.cend(); ++it)
		{
			keys.insert(it.value());
			const int nth = names.count(names.at(it.key()));
			if (nth == 1)
				check(it.value() == names.at(it.key()),
					(std::string(tag) + ": unique name keeps key=name").c_str());
			else if (names.at(it.key()) != it.value())
				check(!allNames.contains(it.value()),
					(std::string(tag) + ": generated key never equals any model name").c_str());
		}
		check(keys.size() == names.size(), (std::string(tag) + ": keys pairwise distinct").c_str());
	}

	void sectionD()
	{
		//D1 无碰撞：键=名（存量单曲线行为逐位不变）
		{
			const QStringList names{ "A", "B", "C" };
			const QHash<int, QString> m = keyRule(names);
			checkEq(m.value(0), QStringLiteral("A"), "D1 no-collision key=name (0)");
			checkEq(m.value(1), QStringLiteral("B"), "D1 no-collision key=name (1)");
			checkEq(m.value(2), QStringLiteral("C"), "D1 no-collision key=name (2)");
			checkKeyInvariants(names, "D1");
		}

		//D2 碰撞：名#2/#3 顺序取首个不冲突键
		{
			const QStringList names{ "STRAIN", "STRAIN", "STRAIN" };
			const QHash<int, QString> m = keyRule(names);
			checkEq(m.value(0), QStringLiteral("STRAIN"), "D2 first occurrence keeps name");
			checkEq(m.value(1), QStringLiteral("STRAIN#2"), "D2 second occurrence key=STRAIN#2");
			checkEq(m.value(2), QStringLiteral("STRAIN#3"), "D2 third occurrence key=STRAIN#3");
			checkKeyInvariants(names, "D2");
		}

		//D3 与字面 "STRAIN#2" 曲线共存不串键：重复 STRAIN 的生成键避开字面名
		{
			const QStringList names{ "STRAIN", "STRAIN#2", "STRAIN" };
			const QHash<int, QString> m = keyRule(names);
			checkEq(m.value(0), QStringLiteral("STRAIN"), "D3 literal-first STRAIN key=name");
			checkEq(m.value(1), QStringLiteral("STRAIN#2"), "D3 literal STRAIN#2 key stays its name");
			checkEq(m.value(2), QStringLiteral("STRAIN#3"), "D3 duplicate STRAIN skips taken #2 -> #3");
			//反查不串键：键 "STRAIN#2" 回查到字面节点（index 1），非重复节点（index 2）
			checkKeyInvariants(names, "D3");
		}

		//D4 交错变体：字面 #2 在前、重复名在后仍互异
		{
			const QStringList names{ "STRAIN#2", "STRAIN", "STRAIN" };
			const QHash<int, QString> m = keyRule(names);
			checkEq(m.value(0), QStringLiteral("STRAIN#2"), "D4 literal #2 first keeps name");
			checkEq(m.value(1), QStringLiteral("STRAIN"), "D4 first STRAIN key=name");
			checkEq(m.value(2), QStringLiteral("STRAIN#3"), "D4 duplicate STRAIN -> #3");
			checkKeyInvariants(names, "D4");
		}

		//D5 #3 已被占用时顺延 #4
		{
			const QStringList names{ "S", "S#2", "S", "S" };
			const QHash<int, QString> m = keyRule(names);
			checkEq(m.value(0), QStringLiteral("S"), "D5 base name");
			checkEq(m.value(1), QStringLiteral("S#2"), "D5 literal #2");
			checkEq(m.value(2), QStringLiteral("S#3"), "D5 first dup -> #3");
			checkEq(m.value(3), QStringLiteral("S#4"), "D5 second dup -> #4");
			checkKeyInvariants(names, "D5");
		}

		//D6 改名后重算：重名消除则键回落=名（视图 rebuild 全量重算的口径）
		{
			QStringList names{ "A", "A" };
			QHash<int, QString> m = keyRule(names);
			checkEq(m.value(1), QStringLiteral("A#2"), "D6 before rename key=A#2");
			names[1] = QStringLiteral("B");
			m = keyRule(names);
			checkEq(m.value(0), QStringLiteral("A"), "D6 after rename first key back to name");
			checkEq(m.value(1), QStringLiteral("B"), "D6 renamed node key=new name");
		}

		//D7 图例文本优先级镜像（makeCurveSettings 图例分支，mCurveChartView.cpp
		//:714-743 口径）：标签文本 > 模型名 > 键名（键名兜底由引擎
		//legendFallbackToName 承担）
		{
			struct Out { QString text; bool fallback; };
			auto legendRule = [](const QString& composedLabel, bool labelVisible,
					const QString& modelName, const QString& engineKey) -> Out
			{
				Out o;
				o.text = composedLabel;
				if (o.text.isEmpty() && labelVisible && !engineKey.isEmpty() && engineKey != modelName)
					o.text = modelName;   //重名碰撞且标签未提供文本：图例显示模型名
				o.fallback = labelVisible && !o.text.isEmpty();
				return o;
			};
			//标签文本优先：有标签文本时图例=标签（即使键≠名）
			{
				const Out o = legendRule(QStringLiteral("REQ-COMP"), true,
					QStringLiteral("STRAIN"), QStringLiteral("STRAIN#2"));
				checkEq(o.text, QStringLiteral("REQ-COMP"), "D7 label text wins legend");
				check(o.fallback, "D7 label text enables fallback");
			}
			//无标签的重名曲线：图例显示模型名（"#2" 后缀不示人）
			{
				const Out o = legendRule(QString(), true,
					QStringLiteral("STRAIN"), QStringLiteral("STRAIN#2"));
				checkEq(o.text, QStringLiteral("STRAIN"), "D7 no-label collision legend=model name");
				check(o.fallback, "D7 model-name legend enables fallback");
			}
			//关闭"显示标签"：图例文字消失，不回退（既有勾选语义不变）
			{
				const Out o = legendRule(QString(), false,
					QStringLiteral("STRAIN"), QStringLiteral("STRAIN#2"));
				check(o.text.isEmpty(), "D7 label off legend empty");
				check(!o.fallback, "D7 label off no fallback");
			}
			//无碰撞（键=名）：不写 legendText，既有留空回退语义不变
			{
				const Out o = legendRule(QString(), true,
					QStringLiteral("STRAIN"), QStringLiteral("STRAIN"));
				check(o.text.isEmpty(), "D7 no-collision keeps legacy empty legendText");
				check(!o.fallback, "D7 no-collision legacy no fallback");
			}
		}

		//D8 模板叠加模式 " (N)" 后缀镜像（mTemplateApplyPipeline 匿名命名空间
		//uniqueCurveName 口径，管线 cpp 含 GUI 头无法无头链接）：目标图表内曲线
		//唯一名=同名追加 " (N)" 序号
		{
			mCurveDocController* ctl = mCurveDocController::getInstance();
			CurveDocument* doc = ctl->document();
			const int pageId = ctl->addPage(QStringLiteral("D"));
			const int chartId = ctl->addChart(pageId);
			ctl->addCurve(chartId, QStringLiteral("STRAIN"));
			ctl->addCurve(chartId, QStringLiteral("STRAIN (2)"));
			auto uniqueCurveName = [doc, chartId](const QString& name) -> QString
			{
				QSet<QString> taken;
				for (int cid : doc->childNodeIds(chartId))
				{
					if (doc->nodeType(cid) == CurveNodeType::Curve)
						taken.insert(doc->nodeName(cid));
				}
				if (!taken.contains(name))
					return name;
				for (int n = 2;; ++n)
				{
					const QString cand = QStringLiteral("%1 (%2)").arg(name).arg(n);
					if (!taken.contains(cand))
						return cand;
				}
			};
			checkEq(uniqueCurveName(QStringLiteral("NEW")), QStringLiteral("NEW"),
				"D8 no-collision template name unchanged");
			checkEq(uniqueCurveName(QStringLiteral("STRAIN")), QStringLiteral("STRAIN (3)"),
				"D8 template overlay skips taken (2) -> (3)");
			checkEq(uniqueCurveName(QStringLiteral("STRAIN (2)")), QStringLiteral("STRAIN (2) (2)"),
				"D8 literal (2) name gets own suffix");
		}

		report("D view key-rule/legend/template mirrors");
	}

	//--- E 节：pch/复数/柱状无碰撞抽样回归 ---

	void sectionE()
	{
		mCurveDocController* ctl = mCurveDocController::getInstance();
		CurveDocument* doc = ctl->document();

		//E1 复数单曲线（无碰撞）：a/b 截齐兜底行为不变（x/mag/phase 等长截到最短）
		{
			const int pageId = ctl->addPage(QStringLiteral("E1"));
			const int chartId = ctl->addChart(pageId);
			const int id = ctl->addComplexCurve(chartId, QStringLiteral("8003-MAG|X-Trans"));
			check(id > 0, "E1 complex curve created");
			ctl->setComplexSeriesData(id, QVector<double>{ 1, 2, 3 },
				QVector<double>{ 1, 2, 3 }, QVector<double>{ 0, 90 });
			check(doc->node(id)->series.xData.size() == 2, "E1 truncated to min length");
			check(dvEq(doc->node(id)->series.yData, QVector<double>{ 1, 2 }),
				"E1 magnitude truncated");
			check(dvEq(doc->node(id)->series.complexYData, QVector<double>{ 0, 90 }),
				"E1 phase truncated");
			check(doc->node(id)->series.isComplex(), "E1 isComplex true");
		}

		//E2 XY→柱状提取转换（无碰撞唯一名）：名保留、数据=类目序号+提取值、可撤销
		{
			const int pageId = ctl->addPage(QStringLiteral("E2"));
			const int chartId = ctl->addChart(pageId);
			const int idA = ctl->addCurve(chartId, QStringLiteral("S1"));
			ctl->setSeriesData(idA, QVector<double>{ 0, 1, 2, 3 },
				QVector<double>{ 1, 2, 3, 4 });
			const int idB = ctl->addCurve(chartId, QStringLiteral("S2"));
			ctl->setSeriesData(idB, QVector<double>{ 0, 1, 2, 3 },
				QVector<double>{ 10, 20, 30, 40 });
			check(ctl->convertCurvesToBar(chartId, 4), "E2 convert succeeds");
			check(doc->nodeProperty(chartId, CurveProp::kChartType).toInt() == 1,
				"E2 chart type switched to bar");
			check(curveCount(doc, chartId) == 2, "E2 two bar nodes replace two curves");
			const QList<int> bars = curveIds(doc, chartId);
			checkEq(doc->nodeName(bars.at(0)), QStringLiteral("S1"), "E2 bar A keeps name");
			checkEq(doc->nodeName(bars.at(1)), QStringLiteral("S2"), "E2 bar B keeps name");
			check(doc->node(bars.at(0))->series.xData.size() == 4, "E2 bar A category space 0..3");
			check(dvEq(doc->node(bars.at(0))->series.yData, QVector<double>{ 1, 2, 3, 4 }),
				"E2 bar A extracted values");
			check(dvEq(doc->node(bars.at(1))->series.yData, QVector<double>{ 10, 20, 30, 40 }),
				"E2 bar B extracted values");
			check(doc->node(bars.at(0))->series.xTickLabels.size() == 4, "E2 category labels present");
			ctl->undo();
			check(doc->nodeProperty(chartId, CurveProp::kChartType).toInt() == 0,
				"E2 undo restores chart type");
			check(doc->node(idA) != nullptr
				&& dvEq(doc->node(idA)->series.yData, QVector<double>{ 1, 2, 3, 4 }),
				"E2 undo restores original curve node with data");
		}

		//E3 同名曲线整批转柱状：两条同名曲线各自转换、名都保留（柱状路径重名兼容）
		{
			const int pageId = ctl->addPage(QStringLiteral("E3"));
			const int chartId = ctl->addChart(pageId);
			const int idA = ctl->addCurve(chartId, QStringLiteral("STRAIN"));
			ctl->setSeriesData(idA, QVector<double>{ 0, 1, 2 }, QVector<double>{ 1, 2, 3 });
			const int idB = ctl->addCurve(chartId, QStringLiteral("STRAIN"));
			ctl->setSeriesData(idB, QVector<double>{ 0, 1, 2 }, QVector<double>{ 7, 8, 9 });
			check(ctl->convertCurvesToBar(chartId, 3), "E3 convert same-name pair succeeds");
			check(curveCount(doc, chartId) == 2, "E3 both curves converted");
			const QList<int> bars = curveIds(doc, chartId);
			checkEq(doc->nodeName(bars.at(0)), QStringLiteral("STRAIN"), "E3 bar A keeps name");
			checkEq(doc->nodeName(bars.at(1)), QStringLiteral("STRAIN"), "E3 bar B keeps same name");
			check(dvEq(doc->node(bars.at(0))->series.yData, QVector<double>{ 1, 2, 3 }),
				"E3 bar A data from curve A");
			check(dvEq(doc->node(bars.at(1))->series.yData, QVector<double>{ 7, 8, 9 }),
				"E3 bar B data from curve B (independent)");
		}

		//E4 pch 曲线名形态（对象-MAG|X-Trans）无碰撞路径：与普通曲线共存键=名
		//（pch 装配静态路径回归由 tst_mCurveAscii D 节覆盖，此处抽样模型层）
		{
			const int pageId = ctl->addPage(QStringLiteral("E4"));
			const int chartId = ctl->addChart(pageId);
			const int id = ctl->addCurve(chartId, QStringLiteral("8003-MAG|X-Trans"));
			ctl->setSeriesSource(id, QStringLiteral("sample.pch"), QStringLiteral("Subcase 1"),
				QStringLiteral("Frequency[Hz]"), QStringLiteral("Point id 8003"),
				QStringLiteral("X-Trans"));
			checkEq(doc->node(id)->series.component, QStringLiteral("X-Trans"),
				"E4 source component recorded");
			checkEq(doc->node(id)->series.subcase, QStringLiteral("Subcase 1"),
				"E4 source subcase recorded");
		}

		report("E no-collision sampling regression");
	}
}

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);   //QAbstractItemModel/QUndoStack 运行环境

	sectionA();
	sectionB();
	sectionC();
	sectionD();
	sectionE();

	std::printf("==== total: cases=%d failures=%d ====\n", g_cases, g_failures);
	return g_failures == 0 ? 0 : g_failures;
}
