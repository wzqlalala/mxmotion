//-----------------------------------------------------------------------------
// tst_mPostResultRegistry —— 结果跳转注册表单测
//
// 覆盖（key=QString::number(创建实例 modelDataID)，由漏斗铸造、注册表不管铸造）：
// key语义与同key覆盖登记；登记/查询往返；同路径多实例共存互不干扰；
// invalidateKey/removeByPostModelID 触发失效回调；takeByKey静默取出+原key重登记
// （mtime重建后key保留、postModelID换绑到新模型）不触发回调；未命中作废不触发回调；
// currentMonitoredMtime（登记与漂移检测共用的 mtime 单一口径）。
// 无GUI依赖（QCoreApplication 即可），经 add_postwindow_test 注册 ctest。
//-----------------------------------------------------------------------------
#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDateTime>
#include "mPostResultRegistry.h"

using PostWindow::mPostResultRegistry;

namespace
{
	mPostResultRegistry::PostJumpRecord makeRecord(const QStringList& paths, int modelID,
		int viewportID, qint64 mtime)
	{
		mPostResultRegistry::PostJumpRecord rec;
		rec.filePaths = paths;
		rec.postModelID = modelID;
		rec.mainViewportID = viewportID;
		rec.fileMtime = mtime;
		return rec;
	}
}

class TestPostResultRegistry : public QObject
{
	Q_OBJECT

private slots:
	//key语义：key=创建实例modelDataID十进制串；同文件两次导入=两个模型两个key必然互异
	//（模型数据ID静态计数器保证唯一不复用）；同key重复登记覆盖旧记录、不触发回调
	void keySemanticsAndOverwriteInsert()
	{
		mPostResultRegistry reg;
		const QString k1 = QString::number(5);   //创建实例1（模型5）
		const QString k2 = QString::number(6);   //创建实例2（模型6，同文件再导入）
		QVERIFY(k1 != k2);

		int fired = 0;
		reg.onKeyInvalidated = [&fired](const QString&) { ++fired; };

		reg.insert(k1, makeRecord({"a.mxdb"}, 5, 101, 1000));
		reg.insert(k2, makeRecord({"a.mxdb"}, 6, 102, 1000));
		QCOMPARE(reg.size(), 2);

		//同key覆盖：记录被替换、不触发回调
		reg.insert(k1, makeRecord({"a.mxdb"}, 5, 105, 3000));
		QCOMPARE(reg.size(), 2);
		QCOMPARE(fired, 0);
		const auto* r1 = reg.find(k1);
		QVERIFY(r1 != nullptr);
		QCOMPARE(r1->mainViewportID, 105);
		QCOMPARE(r1->fileMtime, qint64(3000));
	}

	//登记/查询往返：按key取到自己的记录；未知key返回nullptr
	void insertFindRoundTrip()
	{
		mPostResultRegistry reg;
		reg.insert(QString::number(5), makeRecord({"a.mxdb"}, 5, 101, 1000));
		reg.insert(QString::number(6), makeRecord({"b.op2", "b.bdf"}, 6, 102, 2000));

		QCOMPARE(reg.size(), 2);
		const auto* r1 = reg.find(QString::number(5));
		QVERIFY(r1 != nullptr);
		QCOMPARE(r1->postModelID, 5);
		QCOMPARE(r1->mainViewportID, 101);
		QCOMPARE(r1->filePaths, QStringList({"a.mxdb"}));
		QCOMPARE(r1->fileMtime, qint64(1000));

		const auto* r2 = reg.find(QString::number(6));
		QVERIFY(r2 != nullptr);
		QCOMPARE(r2->postModelID, 6);
		QCOMPARE(r2->filePaths.size(), 2);

		QVERIFY(reg.find(QString::number(999)) == nullptr);
	}

	//同一路径多次导入：两条记录各自独立，作废其一不影响另一实例
	void sameFileMultipleInstances()
	{
		mPostResultRegistry reg;
		const QStringList samePaths({"a.mxdb"});
		reg.insert(QString::number(5), makeRecord(samePaths, 5, 101, 1000));
		reg.insert(QString::number(6), makeRecord(samePaths, 6, 102, 1000));

		QVERIFY(reg.invalidateKey(QString::number(5)));
		QVERIFY(reg.find(QString::number(5)) == nullptr);

		const auto* r2 = reg.find(QString::number(6));
		QVERIFY(r2 != nullptr);
		QCOMPARE(r2->postModelID, 6);
		QCOMPARE(reg.size(), 1);
	}

	//作废已有key：返回true并触发一次失效回调（携带该key）
	void invalidateKeyFiresCallback()
	{
		mPostResultRegistry reg;
		QStringList notified;
		reg.onKeyInvalidated = [&notified](const QString& k) { notified << k; };

		const QString key = QString::number(5);
		reg.insert(key, makeRecord({"a.mxdb"}, 5, 101, 1000));

		QVERIFY(reg.invalidateKey(key));
		QCOMPARE(notified, QStringList({ key }));
		QVERIFY(reg.find(key) == nullptr);

		//重复作废：返回false，不重复触发回调
		QVERIFY(!reg.invalidateKey(key));
		QCOMPARE(notified.size(), 1);
	}

	//作废未登记的key：返回false且不触发回调
	void invalidateUnknownKeyNoCallback()
	{
		mPostResultRegistry reg;
		int fired = 0;
		reg.onKeyInvalidated = [&fired](const QString&) { ++fired; };

		QVERIFY(!reg.invalidateKey(QString::number(42)));
		QCOMPARE(fired, 0);
	}

	//按后处理模型ID反查作废：只移除记录postModelID匹配的条目，每条触发一次回调；无匹配返回空。
	//多记录同挂一模型为注册表能力语义（防御性；正常流程一模型至多一条登记——mtime重建后
	//key为冻结旧ID、postModelID换绑新模型，与该新模型上的新建实例各占一条）
	void removeByPostModelID()
	{
		mPostResultRegistry reg;
		QStringList notified;
		reg.onKeyInvalidated = [&notified](const QString& k) { notified << k; };

		const QString kRebuilt = QString::number(5);   //mtime重建实例：key=原模型5的ID（冻结）
		const QString kFresh = QString::number(6);     //另一实例：key=模型6的ID
		reg.insert(kRebuilt, makeRecord({"a.mxdb"}, 7, 101, 1000));   //重建换绑到新模型7
		reg.insert(kFresh, makeRecord({"b.mxdb"}, 7, 102, 2000));     //同挂模型7（能力语义模拟）

		const QStringList removed = reg.removeByPostModelID(7);
		QCOMPARE(removed.size(), 2);
		QVERIFY(removed.contains(kRebuilt) && removed.contains(kFresh));
		QCOMPARE(notified.size(), 2);
		QVERIFY(reg.find(kRebuilt) == nullptr);
		QVERIFY(reg.find(kFresh) == nullptr);

		//无匹配：返回空、不触发回调
		const QStringList none = reg.removeByPostModelID(999);
		QVERIFY(none.isEmpty());
		QCOMPARE(notified.size(), 2);
	}

	//异步导入失败回滚槽（slot_postImportFailedRollback）依赖的契约：
	//①回调已设置时 removeByPostModelID 的失效回调在函数内部触发（先于返回）——
	//   槽内要保证"失败语义信号先于失效信号"必须临时断开回调静默取出；
	//②断开回调后仍返回全部被移除的key、不触发任何回调，恢复回调再逐key手动补发
	//   即得 F→R 顺序（F=postResultCreateFailedSig、R=postResultRemovedSig）；
	//③静默取出后注册表对该模型已空，后续删除链路（树根移除接收端）再查无副作用，
	//   双信号各恰好一次、无关实例不受影响
	void removeByPostModelIDRollbackContract()
	{
		mPostResultRegistry reg;
		QStringList events;   //事件顺序：R:key=失效回调、F:key=失败信号（槽内发射）
		reg.onKeyInvalidated = [&events](const QString& k) { events << QStringLiteral("R:") + k; };

		const QString keyTarget = QString::number(5);     //回滚目标的失败实例
		const QString keyOther = QString::number(9);      //无关实例（另一模型）
		reg.insert(keyTarget, makeRecord({"a.op2"}, 5, 101, 1000));
		reg.insert(keyOther, makeRecord({"c.mxdb"}, 9, 103, 3000));

		//①先证契约前提：回调在函数内部触发（未断开时先于返回发生）
		QStringList preNotified;
		reg.insert(QString::number(6), makeRecord({"b.op2"}, 6, 102, 2000));
		reg.onKeyInvalidated = [&preNotified](const QString& k) { preNotified << k; };
		QCOMPARE(reg.removeByPostModelID(6).size(), 1);
		QCOMPARE(preNotified.size(), 1);   //返回前已触发
		reg.onKeyInvalidated = [&events](const QString& k) { events << QStringLiteral("R:") + k; };

		//②回滚槽同款序列：断开回调静默取出
		const auto savedInvalidated = reg.onKeyInvalidated;
		reg.onKeyInvalidated = nullptr;
		const QStringList keys = reg.removeByPostModelID(5);
		reg.onKeyInvalidated = savedInvalidated;
		QCOMPARE(keys, QStringList({ keyTarget }));
		QVERIFY(events.isEmpty());   //静默期间无失效通知

		//槽内对每个key先发"失败"再手动补发失效回调 → F严格先于R
		for (const QString& k : keys)
		{
			events << QStringLiteral("F:") + k;
			if (savedInvalidated)
				savedInvalidated(k);
		}
		QCOMPARE(events, QStringList({ QStringLiteral("F:") + keyTarget,
			QStringLiteral("R:") + keyTarget }));
		QCOMPARE(reg.size(), 1);   //无关实例保留

		//③后续删除链路再查同模型：无条目、无回调（不会二次作废）
		int firedAfter = 0;
		reg.onKeyInvalidated = [&firedAfter](const QString&) { ++firedAfter; };
		QVERIFY(reg.removeByPostModelID(5).isEmpty());
		QCOMPARE(firedAfter, 0);
		QVERIFY(reg.find(keyOther) != nullptr);
	}

	//mtime"删旧重建该key实例"前奏与收尾：takeByKey静默取出（不触发失效回调），
	//随后按原key重登记且postModelID换绑到新模型——key冻结语义，查模型走记录字段
	void takeByKeySilentThenReinsert()
	{
		mPostResultRegistry reg;
		int fired = 0;
		reg.onKeyInvalidated = [&fired](const QString&) { ++fired; };

		const QString key = QString::number(5);
		reg.insert(key, makeRecord({"a.mxdb"}, 5, 101, 1000));

		//静默取出：拿到记录、条目移除、不触发回调
		mPostResultRegistry::PostJumpRecord taken;
		QVERIFY(reg.takeByKey(key, &taken));
		QCOMPARE(taken.postModelID, 5);
		QCOMPARE(taken.filePaths, QStringList({"a.mxdb"}));
		QCOMPARE(fired, 0);
		QVERIFY(reg.find(key) == nullptr);

		//二次取失败；未登记key取失败
		QVERIFY(!reg.takeByKey(key));
		QVERIFY(!reg.takeByKey(QString::number(42)));

		//重建完成：按原key重新登记（key保留），postModelID换绑到重建的新模型8，仍不触发回调
		reg.insert(key, makeRecord({"a.mxdb"}, 8, 104, 5000));
		const auto* rec = reg.find(key);
		QVERIFY(rec != nullptr);
		QCOMPARE(rec->postModelID, 8);
		QCOMPARE(rec->fileMtime, qint64(5000));
		QCOMPARE(fired, 0);
	}

	//currentMonitoredMtime：登记侧（fileMtime 铸造）与 showPostResult 漂移检测侧必须
	//同用的 mtime 单一口径——filePaths 与非空 modelFilePath 一并取最大；缺失文件计 0；
	//空记录为 0。缺陷回归：曾因检测侧漏算 modelFilePath，bdf 比 op2 新时每次跳转都
	//误触发"删旧重建"，bdf 较旧重写时重放漏触发
	void currentMonitoredMtimeSemantics()
	{
		QTemporaryFile resultFile;
		QTemporaryFile meshFile;
		QVERIFY(resultFile.open());
		QVERIFY(meshFile.open());
		//显式造出可分辨的 mtime（resultFile 旧、meshFile 新，模拟 op2 与伴随 bdf）
		const QDateTime tResult(QDate(2024, 1, 1), QTime(10, 0, 0));
		const QDateTime tMesh(QDate(2024, 1, 2), QTime(10, 0, 0));
		QVERIFY(resultFile.setFileTime(tResult, QFileDevice::FileModificationTime));
		QVERIFY(meshFile.setFileTime(tMesh, QFileDevice::FileModificationTime));
		const QString resultPath = resultFile.fileName();
		const QString meshPath = meshFile.fileName();
		const qint64 tR = QFileInfo(resultPath).lastModified().toSecsSinceEpoch();
		const qint64 tM = QFileInfo(meshPath).lastModified().toSecsSinceEpoch();
		QVERIFY(tR > 0);
		QVERIFY(tM > tR);

		//空记录：0
		mPostResultRegistry::PostJumpRecord empty;
		QCOMPARE(empty.currentMonitoredMtime(), qint64(0));

		//仅结果文件（QStringList 初始化列表赋值在 Qt5/MSVC 下 operator= 歧义，显式构造）
		mPostResultRegistry::PostJumpRecord rec;
		rec.filePaths = QStringList{ resultPath };
		QCOMPARE(rec.currentMonitoredMtime(), tR);

		//filePaths 内缺失文件计 0：不拉低也不抬高最大值
		rec.filePaths = QStringList{ resultPath, QStringLiteral("Z:/no/such/file.mxdb") };
		QCOMPARE(rec.currentMonitoredMtime(), tR);

		//显式伴随网格一并计入，且能抬高最大值（bdf 比 op2 新的真实场景）
		rec.modelFilePath = meshPath;
		QCOMPARE(rec.currentMonitoredMtime(), tM);

		//伴随网格缺失：计 0，回落到结果文件的 mtime
		rec.modelFilePath = "Z:/no/such/mesh.bdf";
		QCOMPARE(rec.currentMonitoredMtime(), tR);
	}
};

QTEST_GUILESS_MAIN(TestPostResultRegistry)
#include "tst_mPostResultRegistry.moc"
