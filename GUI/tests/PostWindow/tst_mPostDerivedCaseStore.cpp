#include <QtTest/QtTest>

#include "DerivedCase/mPostDerivedCaseStore.h"

using PostWindow::DerivedCaseDefinition;
using PostWindow::DerivedCaseKind;
using PostWindow::DerivedSourceRow;
using PostWindow::mPostDerivedCaseStore;

/*
* mPostDerivedCaseStore 无头回归（批次A0 新增；纯数据类无 QObject/UI 依赖）：
* 覆盖 定义CRUD与id发号 / 名称空间（查重递增、跨kind共用） / 引用校验
* （环/两层上限/引用缺失/草稿模拟） / 展开（顺序保留、比例乘积、不合并同帧、
* 深度与系数防御） / 序列化往返（id计数器抬升） / 删除引用拦截 / 模型桶回收 /
* 变更通知。
*/
class tst_mPostDerivedCaseStore : public QObject
{
	Q_OBJECT

private slots:
	void init() { mPostDerivedCaseStore::instance().clearAll_forTest(); }

	//——定义 CRUD 与 id 发号——
	void crudAndIdIssuing();
	//——名称空间：默认命名查重递增 / 跨 kind 共用一名域——
	void nameSpace();
	//——引用校验：环 / 两层上限 / 引用缺失 / 草稿模拟——
	void referenceChecks();
	//——展开：顺序与比例 / 同帧不合并 / 防御路径——
	void expansion();
	//——序列化往返 + 恢复后发号不撞——
	void serializeRoundTrip();
	//——删除引用拦截 / 模型桶回收 / 变更通知——
	void removeGuardsAndNotify();

private:
	static DerivedSourceRow nativeRow(const QString& name, const QString& frame,
		const QString& scale = QStringLiteral("1"));
	static DerivedCaseDefinition makeDef(DerivedCaseKind kind, const QString& name);
};

DerivedSourceRow tst_mPostDerivedCaseStore::nativeRow(const QString& name, const QString& frame,
	const QString& scale)
{
	DerivedSourceRow row;
	row.id = name + QLatin1Char('|') + frame;
	row.name = name;
	row.frame = frame;
	row.scale = scale;
	return row;
}

DerivedCaseDefinition tst_mPostDerivedCaseStore::makeDef(DerivedCaseKind kind, const QString& name)
{
	DerivedCaseDefinition def;
	def.kind = kind;
	def.name = name;
	return def;
}

void tst_mPostDerivedCaseStore::crudAndIdIssuing()
{
	mPostDerivedCaseStore& store = mPostDerivedCaseStore::instance();

	const int id1 = store.appendDefinition(7, makeDef(DerivedCaseKind::Steps, QStringLiteral("步序列-1")));
	QVERIFY(id1 > 0);
	const int id2 = store.appendDefinition(7, makeDef(DerivedCaseKind::Envelope, QStringLiteral("包络-1")));
	QVERIFY(id2 > id1);   //id 进程内单调

	//名称空/重名拒绝（不插入）
	QVERIFY(store.appendDefinition(7, makeDef(DerivedCaseKind::Steps, QString())) < 0);
	QVERIFY(store.appendDefinition(7, makeDef(DerivedCaseKind::Steps, QStringLiteral("包络-1"))) < 0);   //跨 kind 重名也拒绝

	//kind 过滤与整体枚举
	QCOMPARE(store.definitions(7).size(), 2);
	QCOMPARE(store.definitions(7, DerivedCaseKind::Steps).size(), 1);
	QCOMPARE(store.definitions(7, DerivedCaseKind::Envelope).size(), 1);
	QCOMPARE(store.definitions(8).size(), 0);   //桶隔离

	//读取
	DerivedCaseDefinition fetched;
	QVERIFY(store.definition(7, id2, &fetched));
	QCOMPARE(fetched.kind, DerivedCaseKind::Envelope);
	QCOMPARE(fetched.envelopeType, 1);   //默认 Max
	QVERIFY(!store.definition(7, 999));

	//更新：改名 + 行回填 + configured
	fetched.name = QStringLiteral("包络-改名");
	fetched.envelopeType = 3;   //Range
	fetched.rows << nativeRow(QStringLiteral("CaseA"), QStringLiteral("0: t=1"));
	fetched.configured = true;
	QVERIFY(store.updateDefinition(7, fetched));
	DerivedCaseDefinition updated;
	QVERIFY(store.definition(7, id2, &updated));
	QCOMPARE(updated.name, QStringLiteral("包络-改名"));
	QCOMPARE(updated.envelopeType, 3);
	QCOMPARE(updated.rows.size(), 1);
	QVERIFY(updated.configured);

	//更新到重名拒绝（保持原值）
	DerivedCaseDefinition dup = updated;
	dup.name = QStringLiteral("步序列-1");   //与 id1 重名
	QVERIFY(!store.updateDefinition(7, dup));
	DerivedCaseDefinition untouched;
	QVERIFY(store.definition(7, id2, &untouched));
	QCOMPARE(untouched.name, QStringLiteral("包络-改名"));
}

void tst_mPostDerivedCaseStore::nameSpace()
{
	mPostDerivedCaseStore& store = mPostDerivedCaseStore::instance();
	store.appendDefinition(1, makeDef(DerivedCaseKind::Steps, QStringLiteral("步序列-1")));

	//查重递增：已有 -1，下一个默认名应为 -2
	QCOMPARE(store.uniqueName(1, QStringLiteral("步序列")), QStringLiteral("步序列-2"));
	//跨 kind 已占用同样避开
	store.appendDefinition(1, makeDef(DerivedCaseKind::Envelope, QStringLiteral("步序列-2")));
	QCOMPARE(store.uniqueName(1, QStringLiteral("步序列")), QStringLiteral("步序列-3"));
	//不同桶互不影响
	QCOMPARE(store.uniqueName(2, QStringLiteral("步序列")), QStringLiteral("步序列-1"));

	QVERIFY(store.nameExists(1, QStringLiteral("步序列-1")));
	QVERIFY(!store.nameExists(1, QStringLiteral("步序列-1"), 1));   //exceptId 自身
}

void tst_mPostDerivedCaseStore::referenceChecks()
{
	mPostDerivedCaseStore& store = mPostDerivedCaseStore::instance();
	typedef mPostDerivedCaseStore::ReferenceError RefErr;

	//A：纯原生（depth=1）
	DerivedCaseDefinition defA = makeDef(DerivedCaseKind::Steps, QStringLiteral("A"));
	defA.rows << nativeRow(QStringLiteral("CaseA"), QStringLiteral("f1"));
	const int idA = store.appendDefinition(2, defA);

	//B：引用 A（depth=2，合法）
	DerivedCaseDefinition defB = makeDef(DerivedCaseKind::Steps, QStringLiteral("B"));
	DerivedSourceRow refA;
	refA.isDerived = true;
	refA.derivedId = idA;
	refA.id = QStringLiteral("derived:%1|0").arg(idA);
	defB.rows << refA;
	const int idB = store.appendDefinition(2, defB);

	//C：引用 B（depth=3 → 超两层）
	DerivedCaseDefinition defC = makeDef(DerivedCaseKind::Steps, QStringLiteral("C"));
	DerivedSourceRow refB;
	refB.isDerived = true;
	refB.derivedId = idB;
	defC.rows << refB;
	const int idC = store.appendDefinition(2, defC);

	QList<DerivedCaseDefinition> defs = store.definitions(2);

	//草稿模拟：编辑 C 草稿引用 B → 超两层
	QCOMPARE(mPostDerivedCaseStore::checkReferences(defs, idC, { idB }), RefErr::DepthExceeded);
	//草稿模拟：编辑 B 草稿引用 A——C 仍引用 B（1+2=3 层），整体图校验按原型语义拒绝
	QCOMPARE(mPostDerivedCaseStore::checkReferences(defs, idB, { idA }), RefErr::DepthExceeded);
	//草稿模拟：自引用 → 环
	QCOMPARE(mPostDerivedCaseStore::checkReferences(defs, idA, { idA }), RefErr::Cycle);
	//草稿模拟：引用不存在的定义 → 引用缺失
	QCOMPARE(mPostDerivedCaseStore::checkReferences(defs, idA, { 999 }), RefErr::MissingTarget);
	//candidate 模拟：A 再勾选 C——A→C→B→A 成环（DFS 先于深度命中环）
	QCOMPARE(mPostDerivedCaseStore::checkReferences(defs, idA, {}, idC), RefErr::Cycle);
	//存量集整体校验（editingId=-1）：C→B→A 链存在 → 超两层
	QCOMPARE(mPostDerivedCaseStore::checkReferences(defs, -1, {}), RefErr::DepthExceeded);
	//存量集去掉 C 后合法
	for (int i = 0; i < defs.size(); ++i)
	{
		if (defs.at(i).name == QStringLiteral("C")) { defs.removeAt(i); break; }
	}
	QCOMPARE(mPostDerivedCaseStore::checkReferences(defs, -1, {}), RefErr::None);
	//去掉 C 后：编辑 B 草稿引用 A 合法（无超层链）
	QCOMPARE(mPostDerivedCaseStore::checkReferences(defs, idB, { idA }), RefErr::None);

	//两定义互相引用成环（存量）
	DerivedCaseDefinition defX = makeDef(DerivedCaseKind::Steps, QStringLiteral("X"));
	DerivedCaseDefinition defY = makeDef(DerivedCaseKind::Steps, QStringLiteral("Y"));
	const int idX = store.appendDefinition(3, defX);
	const int idY = store.appendDefinition(3, defY);
	DerivedSourceRow xRefY;  xRefY.isDerived = true;  xRefY.derivedId = idY;
	DerivedSourceRow yRefX;  yRefX.isDerived = true;  yRefX.derivedId = idX;
	defX = store.definitions(3).at(0);  defX.rows << xRefY;
	defY = store.definitions(3).at(1);  defY.rows << yRefX;
	QList<DerivedCaseDefinition> cycleDefs = { defX, defY };
	QCOMPARE(mPostDerivedCaseStore::checkReferences(cycleDefs, -1, {}), RefErr::Cycle);
}

void tst_mPostDerivedCaseStore::expansion()
{
	mPostDerivedCaseStore& store = mPostDerivedCaseStore::instance();
	typedef mPostDerivedCaseStore::ReferenceError RefErr;

	//A：两原生行（同帧重复出现=两个独立步骤，禁止合并）×系数
	DerivedCaseDefinition defA = makeDef(DerivedCaseKind::Steps, QStringLiteral("A"));
	defA.rows << nativeRow(QStringLiteral("CaseA"), QStringLiteral("f1"), QStringLiteral("2"));
	defA.rows << nativeRow(QStringLiteral("CaseA"), QStringLiteral("f1"), QStringLiteral("0.5"));
	defA.rows << nativeRow(QStringLiteral("CaseB"), QStringLiteral("f0"));
	const int idA = store.appendDefinition(4, defA);

	//B：引用 A ×3 + 自有原生行 ×1
	DerivedCaseDefinition defB = makeDef(DerivedCaseKind::Steps, QStringLiteral("B"));
	DerivedSourceRow refA;
	refA.isDerived = true;
	refA.derivedId = idA;
	refA.scale = QStringLiteral("3");
	DerivedSourceRow own = nativeRow(QStringLiteral("CaseC"), QStringLiteral("f9"), QStringLiteral("-1"));
	defB.rows << refA << own;
	const int idB = store.appendDefinition(4, defB);

	QList<mPostDerivedCaseStore::ExpandedSource> out;
	RefErr err = RefErr::None;
	QVERIFY(mPostDerivedCaseStore::expandDefinition(store.definitions(4), idB, &out, &err));
	QCOMPARE(err, RefErr::None);
	//顺序保留 + 比例乘积 + 同帧不合并（A 的两行 f1 分别为 2×3 与 0.5×3）
	QCOMPARE(out.size(), 4);
	QCOMPARE(out.at(0).caseName, QStringLiteral("CaseA"));
	QCOMPARE(out.at(0).frameName, QStringLiteral("f1"));
	QCOMPARE(out.at(0).scale, 6.0);
	QCOMPARE(out.at(1).frameName, QStringLiteral("f1"));
	QCOMPARE(out.at(1).scale, 1.5);
	QCOMPARE(out.at(2).caseName, QStringLiteral("CaseB"));
	QCOMPARE(out.at(2).scale, 3.0);
	QCOMPARE(out.at(3).caseName, QStringLiteral("CaseC"));
	QCOMPARE(out.at(3).scale, -1.0);

	//引用缺失
	QVERIFY(!mPostDerivedCaseStore::expandDefinition(store.definitions(4), 999, &out, &err));
	QCOMPARE(err, RefErr::MissingTarget);
	QVERIFY(out.isEmpty());   //失败清空

	//系数非法（非有限数值文本）
	DerivedCaseDefinition defBad = makeDef(DerivedCaseKind::Steps, QStringLiteral("Bad"));
	defBad.rows << nativeRow(QStringLiteral("CaseA"), QStringLiteral("f1"), QStringLiteral("abc"));
	const int idBad = store.appendDefinition(4, defBad);
	QVERIFY(!mPostDerivedCaseStore::expandDefinition(store.definitions(4), idBad, &out, &err));
	QCOMPARE(err, RefErr::InvalidScale);
}

void tst_mPostDerivedCaseStore::serializeRoundTrip()
{
	mPostDerivedCaseStore& store = mPostDerivedCaseStore::instance();

	//三 kind 各一条（含行/包络类型/派生引用）
	DerivedCaseDefinition steps = makeDef(DerivedCaseKind::Steps, QStringLiteral("步序列-1"));
	steps.rows << nativeRow(QStringLiteral("CaseA"), QStringLiteral("f1"), QStringLiteral("1.5"));
	steps.configured = true;
	const int idSteps = store.appendDefinition(5, steps);

	DerivedCaseDefinition envelope = makeDef(DerivedCaseKind::Envelope, QStringLiteral("包络-1"));
	envelope.envelopeType = 2;
	DerivedSourceRow refSteps;
	refSteps.isDerived = true;
	refSteps.derivedId = idSteps;
	envelope.rows << refSteps;
	envelope.configured = true;
	store.appendDefinition(5, envelope);

	const QByteArray blob = store.serialize(5);
	QVERIFY(!blob.isEmpty());

	//恢复到新桶（模拟重开会话：新模型实例）
	QCOMPARE(store.deserialize(6, blob), 2);
	QList<DerivedCaseDefinition> restored = store.definitions(6);
	QCOMPARE(restored.size(), 2);
	QCOMPARE(restored.at(0).name, QStringLiteral("步序列-1"));
	QCOMPARE(restored.at(0).kind, DerivedCaseKind::Steps);
	QCOMPARE(restored.at(0).rows.at(0).scale, QStringLiteral("1.5"));
	QCOMPARE(restored.at(1).kind, DerivedCaseKind::Envelope);
	QCOMPARE(restored.at(1).envelopeType, 2);
	QCOMPARE(restored.at(1).rows.at(0).derivedId, idSteps);   //引用 id 保序

	//恢复后 id 计数器抬升：新发号不与已恢复 id 相撞
	const int fresh = store.appendDefinition(6, makeDef(DerivedCaseKind::Superposition, QStringLiteral("线性叠加-1")));
	QVERIFY(fresh > idSteps);
	QCOMPARE(store.definitions(6).size(), 3);

	//坏载荷拒绝
	QByteArray garbage("not-a-derived-blob");
	QCOMPARE(store.deserialize(9, garbage), -1);
	QCOMPARE(store.definitions(9).size(), 0);
}

void tst_mPostDerivedCaseStore::removeGuardsAndNotify()
{
	mPostDerivedCaseStore& store = mPostDerivedCaseStore::instance();

	DerivedCaseDefinition defA = makeDef(DerivedCaseKind::Steps, QStringLiteral("A"));
	defA.rows << nativeRow(QStringLiteral("CaseA"), QStringLiteral("f1"));
	const int idA = store.appendDefinition(10, defA);

	DerivedCaseDefinition defB = makeDef(DerivedCaseKind::Steps, QStringLiteral("B"));
	DerivedSourceRow refA;
	refA.isDerived = true;
	refA.derivedId = idA;
	defB.rows << refA;
	const int idB = store.appendDefinition(10, defB);

	//变更通知：append 已触发 2 次（clearAll 清空后累计）
	int notified = 0;
	int lastKey = -1;
	store.addChangeListener([&notified, &lastKey](int key) { ++notified; lastKey = key; });

	//被引用删除拦截
	QList<int> referrers;
	QCOMPARE(store.removeDefinition(10, idA, &referrers), mPostDerivedCaseStore::RemoveResult::Referenced);
	QCOMPARE(referrers, QList<int>() << idB);
	QCOMPARE(store.definitions(10).size(), 2);   //未删
	QCOMPARE(notified, 0);   //拦截路径不发通知

	//先删引用方再删被引用：成功且各发一次通知
	QCOMPARE(store.removeDefinition(10, idB), mPostDerivedCaseStore::RemoveResult::Removed);
	QCOMPARE(notified, 1);
	QCOMPARE(store.removeDefinition(10, idA), mPostDerivedCaseStore::RemoveResult::Removed);
	QCOMPARE(notified, 2);
	QCOMPARE(lastKey, 10);
	QCOMPARE(store.definitions(10).size(), 0);

	//不存在
	QCOMPARE(store.removeDefinition(10, idA), mPostDerivedCaseStore::RemoveResult::NotFound);

	//模型桶回收
	store.appendDefinition(11, makeDef(DerivedCaseKind::Steps, QStringLiteral("X")));
	store.removeModel(11);
	QCOMPARE(store.definitions(11).size(), 0);
}

QTEST_MAIN(tst_mPostDerivedCaseStore)
#include "tst_mPostDerivedCaseStore.moc"
