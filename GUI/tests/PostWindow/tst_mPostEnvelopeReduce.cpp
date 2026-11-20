#include <QtTest/QtTest>

#include "DerivedCase/mPostDerivedCaseEngine.h"

using PostWindow::mPostDerivedCaseEngine;

/*
* 包络归约纯函数无头回归（批次B 新增；流式两数组归约的四类型语义）：
* 覆盖 Min/Max/Extreme（负值主导取绝对值最大者**原值**、|max|==|min| 并列取
* max 侧）/Range（极差）/ 键域交集（不一致键剔除+标记、交集空拒绝）/ 空输入 /
* 极值来源帧索引随更新跟踪。generateEnvelope 的 IO/进度/回滚链与 generateSteps
* 同构，归一走查（批次A1 已验证范式），不在此无头覆盖。
*/
class tst_mPostEnvelopeReduce : public QObject
{
	Q_OBJECT

private slots:
	//——四类型基础语义——
	void fourTypes();
	//——Extreme：负值主导取原值 / |max|==|min| 并列取 max 侧——
	void extremeNegativeDominant();
	//——键域交集：不一致键剔除+标记 / 交集空拒绝——
	void keyIntersection();
	//——空输入拒绝 / 单帧退化（min==max、Range=0）——
	void emptyAndSingleFrame();
	//——极值来源帧索引随更新跟踪——
	void sourceIndices();

private:
	static QHash<int, double> hash(std::initializer_list<QPair<int, double>> pairs);
};

QHash<int, double> tst_mPostEnvelopeReduce::hash(std::initializer_list<QPair<int, double>> pairs)
{
	QHash<int, double> out;
	for (const auto& pair : pairs)
		out.insert(pair.first, pair.second);
	return out;
}

void tst_mPostEnvelopeReduce::fourTypes()
{
	mPostDerivedCaseEngine::EnvelopeAccumulator acc;
	QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 10.0}, {2, -5.0}, {3, 0.0} })));
	QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 20.0}, {2, -10.0}, {3, 1.0} })));
	QCOMPARE(acc.fedFrames, 2);
	QVERIFY(!acc.keysIntersected);

	QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 0), hash({ {1, 10.0}, {2, -10.0}, {3, 0.0} }));	//Min
	QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 1), hash({ {1, 20.0}, {2, -5.0}, {3, 1.0} }));	//Max（默认）
	QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 2), hash({ {1, 20.0}, {2, -10.0}, {3, 1.0} }));	//Extreme：逐键 |max|>|min| 全取 max 侧
	QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 3), hash({ {1, 10.0}, {2, 5.0}, {3, 1.0} }));	//Range=max-min
	//未知类型回退 Max（防御）
	QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 99), hash({ {1, 20.0}, {2, -5.0}, {3, 1.0} }));
}

void tst_mPostEnvelopeReduce::extremeNegativeDominant()
{
	{
		//负值主导：|−100|>|10| → Extreme 取 −100（原值，非绝对值）
		mPostDerivedCaseEngine::EnvelopeAccumulator acc;
		QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 10.0} })));
		QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, -100.0} })));
		QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 2), hash({ {1, -100.0} }));
	}
	{
		//|max|==|min| 并列：取 max 侧（+10）
		mPostDerivedCaseEngine::EnvelopeAccumulator acc;
		QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, -10.0} })));
		QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 10.0} })));
		QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 2), hash({ {1, 10.0} }));
	}
}

void tst_mPostEnvelopeReduce::keyIntersection()
{
	mPostDerivedCaseEngine::EnvelopeAccumulator acc;
	QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 1.0}, {2, 2.0} })));
	//键 2 不在第二帧 → 剔除并置标记；键 3 为第二帧独有 → 不进交集域
	QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 5.0}, {3, 30.0} })));
	QCOMPARE(acc.minByKey.size(), 1);
	QVERIFY(acc.minByKey.contains(1));
	QVERIFY(acc.keysIntersected);
	QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 1), hash({ {1, 5.0} }));

	//交集已空（剩余键 1 不在本帧）→ 拒绝
	QVERIFY(!mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {2, 9.0} })));
}

void tst_mPostEnvelopeReduce::emptyAndSingleFrame()
{
	mPostDerivedCaseEngine::EnvelopeAccumulator acc;
	QVERIFY(!mPostDerivedCaseEngine::envelopeFeed(acc, QHash<int, double>()));	//首帧空数据拒绝
	QCOMPARE(acc.fedFrames, 0);

	QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {7, 3.5} })));		//单帧退化
	QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 0), hash({ {7, 3.5} }));	//min==max==值
	QCOMPARE(mPostDerivedCaseEngine::envelopeReduce(acc, 3), hash({ {7, 0.0} }));	//Range=0
}

void tst_mPostEnvelopeReduce::sourceIndices()
{
	mPostDerivedCaseEngine::EnvelopeAccumulator acc;
	//喂入序 0/1/2：键 1 序列 5→8→1，最小来自第 3 帧、最大来自第 2 帧；
	//键 2 序列 1→2→0.5，最小来自第 3 帧、最大来自第 2 帧
	QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 5.0}, {2, 1.0} })));
	QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 8.0}, {2, 2.0} })));
	QVERIFY(mPostDerivedCaseEngine::envelopeFeed(acc, hash({ {1, 1.0}, {2, 0.5} })));
	QCOMPARE(acc.minSourceByKey.value(1), 2);
	QCOMPARE(acc.maxSourceByKey.value(1), 1);
	QCOMPARE(acc.minSourceByKey.value(2), 2);
	QCOMPARE(acc.maxSourceByKey.value(2), 1);
}

QTEST_MAIN(tst_mPostEnvelopeReduce)
#include "tst_mPostEnvelopeReduce.moc"
