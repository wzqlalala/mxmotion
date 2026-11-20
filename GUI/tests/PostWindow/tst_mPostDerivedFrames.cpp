#include <QtTest/QtTest>

#include "mPostDataBase.h"
#include "mPostModelData.h"

using PostData::mPostDataBase;
using PostData::mPostModelData;

/*
* mPostDataBase 派生工况接口族无头回归（批次A1 新增，宿主在 GUI 测试工程——
* BasicData 无独立测试基建）：
* 覆盖 appendDerivedFrame（帧号单调/枚举口径/变量名成对登记/极值现算）/
* 墓碑删除（各枚举过滤、结果数据释放、帧号不复用）/
* f-1 位漂根治（getFrameCase/getFrameVectors 帧号直查，墓碑空洞后仍正确）/
* renameDerivedCase / 无效几何源拒绝。
*
* 不触视口/渲染/读取器（QCoreApplication 即可运行）。
*/
class tst_mPostDerivedFrames : public QObject
{
	Q_OBJECT

private slots:
	//——appendDerivedFrame：帧号单调、枚举口径、变量登记、极值——
	void appendAndEnumerate();
	//——墓碑删除：枚举过滤、数据释放、帧号不复用——
	void removeTombstone();
	//——f-1 位漂根治：墓碑空洞帧号下 getFrameCase/getFrameVectors 直查正确——
	void directLookupAfterTombstone();
	//——改名联动 / 无效几何源——
	void renameAndInvalidSource();

private:
	//三帧分析工况 CaseA（帧 1..3）的最小结果库（帧模型数据不建几何，枚举不依赖）
	static mPostDataBase* makeDb();
};

mPostDataBase* tst_mPostDerivedFrames::makeDb()
{
	mPostDataBase* db = new mPostDataBase(QStringLiteral("tstDerivedFrames.mxdb"));
	for (int f = 1; f <= 3; ++f)
	{
		mPostModelData* md = new mPostModelData();
		md->_frameInfo._frameCase = QStringLiteral("CaseA");
		md->_frameInfo._frameName = QString::number(f);
		md->_frameInfo._frameNum = f;
		md->_frameInfo._frameTime = 0.1 * f;
		db->appendModelData(f, md);
	}
	return db;
}

void tst_mPostDerivedFrames::appendAndEnumerate()
{
	mPostDataBase* db = makeDb();

	QHash<int, double> res;
	res.insert(1, 10.0);
	res.insert(2, -5.0);

	//物化：几何克隆自帧 1；变量身份串与 GUI expectedResultVarName 同格式
	const int f4 = db->appendDerivedFrame(QStringLiteral("Steps-1"),
		QStringLiteral("1: CaseA/1"), 0.1, 1, res,
		QStringLiteral("Stress Mises"), 0);
	QVERIFY(f4 == 4);	//帧号 = 当前最大 3 + 1，全局单调

	const int f5 = db->appendDerivedFrame(QStringLiteral("Steps-1"),
		QStringLiteral("2: CaseA/2"), 0.2, 2, res,
		QStringLiteral("Stress Mises"), 0);
	QVERIFY(f5 == 5);

	//枚举口径：派生工况不进分析工况列表；getDerivedCaseNames 单独枚举
	QCOMPARE(db->getAllCaseNames(), QStringList{ QStringLiteral("CaseA") });
	QCOMPARE(db->getDerivedCaseNames(), QStringList{ QStringLiteral("Steps-1") });

	QVERIFY(db->isDerivedFrame(f4));
	QVERIFY(!db->isDerivedFrame(1));
	QVERIFY(!db->isDerivedFrame(999));

	//帧枚举：派生帧进全帧表；工况帧名（原始 _frameName 匹配）可解析回帧号
	QVERIFY(db->getFrameIds().contains(f4));
	QVERIFY(db->getFrameIds().contains(f5));
	QCOMPARE(db->getAllFrameInfo().size(), 5);
	QCOMPARE(db->getFrameNumByCaseAndFrameName(QStringLiteral("Steps-1"),
		QStringLiteral("1: CaseA/1")), f4);
	QCOMPARE(db->getCaseFrameNames(QStringLiteral("Steps-1")).size(), 2);

	//结果数据与变量名成对登记；极值从物化值现算（first=min，负缩放翻转场景由现算保证）
	QCOMPARE(db->getResultVarNameByFrameID(f4), QStringLiteral("Stress Mises"));
	QCOMPARE(db->getResultDataByFrameID(f4), res);
	const QPair<double, double> maxMin = db->getMaxMinByFrameID(f4);
	QCOMPARE(maxMin.first, -5.0);
	QCOMPARE(maxMin.second, 10.0);
}

void tst_mPostDerivedFrames::removeTombstone()
{
	mPostDataBase* db = makeDb();
	QHash<int, double> res;
	res.insert(1, 1.0);

	const int f4 = db->appendDerivedFrame(QStringLiteral("Steps-1"),
		QStringLiteral("1: CaseA/1"), 0.1, 1, res, QStringLiteral("V"), 0);
	const int f5 = db->appendDerivedFrame(QStringLiteral("Env-1"),
		QStringLiteral("0: 包络"), 0.0, 1, res, QStringLiteral("V"), 0);
	QVERIFY(f4 == 4 && f5 == 5);

	//删除 Steps-1：墓碑标记而非结构性移除（帧号 4 永不复用）
	QVERIFY(db->removeDerivedCase(QStringLiteral("Steps-1")));
	QVERIFY(!db->removeDerivedCase(QStringLiteral("Steps-1")));	//二次删除不命中
	QVERIFY(!db->removeDerivedCase(QStringLiteral("CaseA")));		//分析工况不可经此删

	//各枚举过滤墓碑帧；结果数据释放
	QCOMPARE(db->getDerivedCaseNames(), QStringList{ QStringLiteral("Env-1") });
	QVERIFY(!db->getFrameIds().contains(f4));
	QVERIFY(db->getFrameIds().contains(f5));
	QCOMPARE(db->getAllFrameInfo().size(), 4);	//3 分析帧 + Env-1
	QVERIFY(!db->isDerivedFrame(f4));
	QVERIFY(db->getResultDataByFrameID(f4).isEmpty());
	QVERIFY(db->getResultVarNameByFrameID(f4).isEmpty());

	//帧号不复用：再追加从最大键（含墓碑的 5）+1 起
	const int f6 = db->appendDerivedFrame(QStringLiteral("Env-2"),
		QStringLiteral("0: 包络"), 0.0, 1, res, QStringLiteral("V"), 0);
	QVERIFY(f6 == 6);
}

void tst_mPostDerivedFrames::directLookupAfterTombstone()
{
	//f-1 根治回归：原 getFrameVectors/getFrameCase 按 getAllFrameInfo 列表 f-1 下标
	//取帧信息，墓碑帧造成帧号空洞后位漂（帧 5 落在下标 4 越界/错帧）；改直查后
	//"帧号=哈希键"恒正确
	mPostDataBase* db = makeDb();
	QHash<int, double> res;
	res.insert(1, 1.0);
	db->appendDerivedFrame(QStringLiteral("Steps-1"),
		QStringLiteral("1: CaseA/1"), 0.1, 1, res, QStringLiteral("V"), 0);	//帧 4，将删
	const int f5 = db->appendDerivedFrame(QStringLiteral("Env-1"),
		QStringLiteral("0: 包络"), 0.0, 1, res, QStringLiteral("V"), 0);
	QVERIFY(db->removeDerivedCase(QStringLiteral("Steps-1")));

	//空洞（缺 4）后：帧 5 的工况/矢量名直查正确（原实现越界返回空）
	QCOMPARE(db->getFrameCase(f5), QStringLiteral("Env-1"));
	QCOMPARE(db->getFrameCase(3), QStringLiteral("CaseA"));
	QVERIFY(db->getFrameCase(f5 - 1).isEmpty());	//墓碑帧号按已删语义返回空（原实现会错取相邻帧）
	QVERIFY(db->getFrameVectors(f5).isEmpty());		//无矢量变量帧返回空表不崩
	QVERIFY(db->getFrameVectors(999).isEmpty());	//不存在帧号同理
}

void tst_mPostDerivedFrames::renameAndInvalidSource()
{
	mPostDataBase* db = makeDb();
	QHash<int, double> res;
	res.insert(1, 1.0);
	db->appendDerivedFrame(QStringLiteral("Steps-1"),
		QStringLiteral("1: CaseA/1"), 0.1, 1, res, QStringLiteral("V"), 0);

	//改名：枚举跟随，旧名不再命中
	QVERIFY(db->renameDerivedCase(QStringLiteral("Steps-1"), QStringLiteral("Steps-2")));
	QCOMPARE(db->getDerivedCaseNames(), QStringList{ QStringLiteral("Steps-2") });
	QCOMPARE(db->getCaseFrameNames(QStringLiteral("Steps-2")).size(), 1);
	QVERIFY(db->getCaseFrameNames(QStringLiteral("Steps-1")).isEmpty());
	QVERIFY(!db->renameDerivedCase(QStringLiteral("Steps-1"), QStringLiteral("X")));	//旧名已不存在
	QVERIFY(!db->renameDerivedCase(QStringLiteral("Steps-2"), QStringLiteral("Steps-2")));	//同名零动作

	//无效几何源：拒绝（返回 -1），不产生半登记帧
	QCOMPARE(db->appendDerivedFrame(QStringLiteral("Bad"),
		QStringLiteral("x"), 0.0, 99, res, QStringLiteral("V"), 0), -1);
	QVERIFY(!db->getDerivedCaseNames().contains(QStringLiteral("Bad")));
}

QTEST_MAIN(tst_mPostDerivedFrames)
#include "tst_mPostDerivedFrames.moc"
