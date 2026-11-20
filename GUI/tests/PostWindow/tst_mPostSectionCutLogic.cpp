#pragma execution_character_set("utf-8")

// mPostSectionCut 纯逻辑回归测试
//
// 覆盖面：坐标文本解析（detail::parsePoint 分隔符/有效性）、单值文本校验、
// 法向 L13 校验问题码、几何校验问题码（未应用点/基点/方向对象/三点共线族）、
// 草稿签名（dirty 检测口径）、渲染装配折算（法向解算/基准点/平面落点/保留侧
// 三态翻转/截面带双平面/最近节点位移近似）。
// 全部用例只依赖 Qt 容器与 QVector3D（QCoreApplication 即可，无头运行），
// 不触碰视口/渲染/结果库（applySectionCuts 族依赖渲染端对象，不在无头范围）。

#include <QtTest/QtTest>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector3D>

#include "mPostSectionCut.h"

using namespace PostWindow;

class TestPostSectionCutLogic : public QObject
{
    Q_OBJECT

private slots:
    void parsePointFormats();
    void parsePointInvalid();
    void finiteTextCases();
    void normalIssueCodes();
    void geometryIssueCodes();
    void signatureStability();
    void normalAxisAndVector();
    void normalStartEndAndThreePoints();
    void basePointByDefinition();
    void vertexWithOffsetAndShift();
    void effectiveKeepSideFolding();
    void effectiveClipModelFolding();
    void bandVerticesSymmetry();
    void nearestNodeDisplacementCases();
};

//坐标解析：逗号/中文逗号/空白分隔均可，三有限数
void TestPostSectionCutLogic::parsePointFormats()
{
    QList<double> v = detail::parsePoint(QString("1, 2, 3"));
    QCOMPARE(v.size(), 3);
    QCOMPARE(v.at(0), 1.0);
    QCOMPARE(v.at(1), 2.0);
    QCOMPARE(v.at(2), 3.0);

    v = detail::parsePoint(QString("1，2，3"));          //中文逗号
    QCOMPARE(v.size(), 3);
    v = detail::parsePoint(QString("1 2 3"));            //空白分隔
    QCOMPARE(v.size(), 3);
    v = detail::parsePoint(QString("  -4.5 , 0 , 1e2 ")); //首尾空白+科学计数
    QCOMPARE(v.size(), 3);
    QCOMPARE(v.at(0), -4.5);
    QCOMPARE(v.at(2), 100.0);
}

//坐标解析失败态：分量数不对/非数值/非有限
void TestPostSectionCutLogic::parsePointInvalid()
{
    QVERIFY(detail::parsePoint(QString("1, 2")).isEmpty());
    QVERIFY(detail::parsePoint(QString("1, 2, 3, 4")).isEmpty());
    QVERIFY(detail::parsePoint(QString("a, b, c")).isEmpty());
    QVERIFY(detail::parsePoint(QString()).isEmpty());
    QVERIFY(detail::parsePoint(QString("inf, 0, 0")).isEmpty());
    QVERIFY(detail::parsePoint(QString("nan, 0, 0")).isEmpty());
}

//单值文本：空/非数值/inf 拒绝，正负数与科学计数通过
void TestPostSectionCutLogic::finiteTextCases()
{
    QVERIFY(detail::finiteText(QString("0")));
    QVERIFY(detail::finiteText(QString("-12.5")));
    QVERIFY(detail::finiteText(QString(" 1e3 ")));
    QVERIFY(!detail::finiteText(QString()));
    QVERIFY(!detail::finiteText(QString("  ")));
    QVERIFY(!detail::finiteText(QString("abc")));
    QVERIFY(!detail::finiteText(QString("inf")));
}

//法向校验问题码：仅向量/起点-终点模式产生；轴模式恒 None
void TestPostSectionCutLogic::normalIssueCodes()
{
    SectionCutDraft d;
    d.normalMode = 0;
    QCOMPARE(detail::normalIssue(d), detail::NormalIssue::None);

    d.normalMode = 5;
    d.vectorText = QStringLiteral("1, 0");
    QCOMPARE(detail::normalIssue(d), detail::NormalIssue::InvalidComponents);
    d.vectorText = QStringLiteral("0, 0, 0");
    QCOMPARE(detail::normalIssue(d), detail::NormalIssue::ZeroDirection);
    d.vectorText = QStringLiteral("0.5, 0, 0");
    QCOMPARE(detail::normalIssue(d), detail::NormalIssue::None);

    d.normalMode = 3;
    d.startText = QStringLiteral("0, 0, 0");
    d.endText = QStringLiteral("0, 0, 0");
    QCOMPARE(detail::normalIssue(d), detail::NormalIssue::ZeroDirection);
    d.endText = QStringLiteral("x, 0, 0");
    QCOMPARE(detail::normalIssue(d), detail::NormalIssue::InvalidComponents);
    d.endText = QStringLiteral("1, 0, 0");
    QCOMPARE(detail::normalIssue(d), detail::NormalIssue::None);
}

//几何校验问题码：判定顺序与面板文案链一致
void TestPostSectionCutLogic::geometryIssueCodes()
{
    SectionCutDraft d;
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::None);    //默认草稿=Z轴+有效基点

    d.basePoint.dirty = true;                                       //未应用点优先
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::PendingPoints);
    d.basePoint.dirty = false;

    d.basePoint.text = QStringLiteral("1, 2");
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::InvalidBase);
    d.basePoint.text = QStringLiteral("0, 0, 0");

    d.normalMode = 4;                                               //选择对象暂无数据源
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::NoDirectionObject);
    d.normalMode = 5;
    d.vectorText = QStringLiteral("0, 0, 0");                       //法向无效经 L13 问题码
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::InvalidNormal);
    d.vectorText = QStringLiteral("1, 0, 0");

    d.definition = 1;                                               //三点平面族
    d.point1.text = QStringLiteral("0, 0, 0");
    d.point2.text = QStringLiteral("a, 0, 0");
    d.point3.text = QStringLiteral("0, 1, 0");
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::InvalidThreePoints);
    d.point2.text = QStringLiteral("0, 0, 0");                      //与点1重合
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::CollinearPoints);
    d.point2.text = QStringLiteral("2, 0, 0");                      //三点共线
    d.point3.text = QStringLiteral("3, 0, 0");
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::CollinearPoints);
    d.point3.text = QStringLiteral("0, 1, 0");
    QCOMPARE(detail::geometryIssue(d), detail::GeomIssue::None);
}

//草稿签名：同草稿稳定、任一字段变化（含点 dirty 标记）即变化
void TestPostSectionCutLogic::signatureStability()
{
    SectionCutDraft a;
    SectionCutDraft b;
    QCOMPARE(detail::sectionCutSignature(a), detail::sectionCutSignature(b));

    b.offset = QStringLiteral("5");
    QVERIFY(detail::sectionCutSignature(a) != detail::sectionCutSignature(b));
    b = a;
    b.color = QColor(1, 2, 3);
    QVERIFY(detail::sectionCutSignature(a) != detail::sectionCutSignature(b));
    b = a;
    b.basePoint.dirty = true;                                       //未应用点编辑入签名
    QVERIFY(detail::sectionCutSignature(a) != detail::sectionCutSignature(b));
    b = a;
    b.name = QStringLiteral("Other");
    QVERIFY(detail::sectionCutSignature(a) != detail::sectionCutSignature(b));
}

//法向解算：轴三模式 + 向量模式（归一）
void TestPostSectionCutLogic::normalAxisAndVector()
{
    SectionCutDraft d;
    QVector3D n;

    d.normalMode = 0;
    QVERIFY(detail::sectionCutNormal(d, n));
    QCOMPARE(n, QVector3D(1, 0, 0));
    d.normalMode = 1;
    QVERIFY(detail::sectionCutNormal(d, n));
    QCOMPARE(n, QVector3D(0, 1, 0));
    d.normalMode = 2;
    QVERIFY(detail::sectionCutNormal(d, n));
    QCOMPARE(n, QVector3D(0, 0, 1));

    d.normalMode = 5;
    d.vectorText = QStringLiteral("2, 0, 0");                       //非单位向量归一
    QVERIFY(detail::sectionCutNormal(d, n));
    QCOMPARE(n, QVector3D(1, 0, 0));
    d.vectorText = QStringLiteral("0, 0, 0");
    QVERIFY(!detail::sectionCutNormal(d, n));

    d.normalMode = 4;                                               //选择对象无数据源
    QVERIFY(!detail::sectionCutNormal(d, n));
}

//法向解算：起点-终点 + 三点平面（(P2−P1)×(P3−P1) 方向）
void TestPostSectionCutLogic::normalStartEndAndThreePoints()
{
    SectionCutDraft d;
    QVector3D n;

    d.normalMode = 3;
    d.startText = QStringLiteral("0, 0, 0");
    d.endText = QStringLiteral("0, 0, 5");
    QVERIFY(detail::sectionCutNormal(d, n));
    QCOMPARE(n, QVector3D(0, 0, 1));

    d.definition = 1;
    d.point1.text = QStringLiteral("0, 0, 0");
    d.point2.text = QStringLiteral("1, 0, 0");
    d.point3.text = QStringLiteral("0, 1, 0");
    QVERIFY(detail::sectionCutNormal(d, n));
    QCOMPARE(n, QVector3D(0, 0, 1));                                //x×y=z

    d.point2.text = QStringLiteral("0, 1, 0");                      //共线失败
    d.point3.text = QStringLiteral("0, 2, 0");
    QVERIFY(!detail::sectionCutNormal(d, n));
}

//基准点：法向轴分支取基点，三点平面取点1
void TestPostSectionCutLogic::basePointByDefinition()
{
    SectionCutDraft d;
    d.basePoint.text = QStringLiteral("45, 10, 8");
    d.point1.text = QStringLiteral("-1, -2, -3");
    QVector3D base;

    QVERIFY(detail::sectionCutBasePoint(d, base));
    QCOMPARE(base, QVector3D(45, 10, 8));

    d.definition = 1;
    QVERIFY(detail::sectionCutBasePoint(d, base));
    QCOMPARE(base, QVector3D(-1, -2, -3));

    d.point1.text = QStringLiteral("bad");
    QVERIFY(!detail::sectionCutBasePoint(d, base));
}

//平面落点：基点+法向×偏移+变形平移（默认草稿偏移 0=基点本身）
void TestPostSectionCutLogic::vertexWithOffsetAndShift()
{
    SectionCutDraft d;
    d.basePoint.text = QStringLiteral("10, 0, 0");
    d.offset = QStringLiteral("0");
    const QVector3D zAxis(0, 0, 1);
    QCOMPARE(detail::sectionCutVertex(d, zAxis), QVector3D(10, 0, 0));

    d.offset = QStringLiteral("2.5");
    QCOMPARE(detail::sectionCutVertex(d, zAxis), QVector3D(10, 0, 2.5));

    QCOMPARE(detail::sectionCutVertex(d, zAxis, QVector3D(1, 1, 1)), QVector3D(11, 1, 3.5));
}

//保留侧三态折算：reversed 翻转 0↔2，两侧不受影响，越界钳回正向侧
void TestPostSectionCutLogic::effectiveKeepSideFolding()
{
    QCOMPARE(detail::effectiveKeepSide(2, false), 2);
    QCOMPARE(detail::effectiveKeepSide(0, false), 0);
    QCOMPARE(detail::effectiveKeepSide(1, false), 1);
    QCOMPARE(detail::effectiveKeepSide(2, true), 0);
    QCOMPARE(detail::effectiveKeepSide(0, true), 2);
    QCOMPARE(detail::effectiveKeepSide(1, true), 1);
    QCOMPARE(detail::effectiveKeepSide(7, false), 2);
    QCOMPARE(detail::effectiveKeepSide(-1, true), 0);   //越界先钳回正向侧，再受反向翻转
}

//裁剪单元折算：裁剪开且非两侧才参与模型裁剪；裁剪关/保留两侧一律零方程占位
void TestPostSectionCutLogic::effectiveClipModelFolding()
{
    QCOMPARE(detail::effectiveClipModel(true, 2), true);    //裁剪开+正向侧→裁
    QCOMPARE(detail::effectiveClipModel(true, 0), true);    //裁剪开+负向侧→裁（经翻转方程）
    QCOMPARE(detail::effectiveClipModel(true, 1), false);   //裁剪开+两侧→不裁
    QCOMPARE(detail::effectiveClipModel(false, 2), false);  //裁剪关→不裁（保留侧不救）
    QCOMPARE(detail::effectiveClipModel(false, 0), false);
    QCOMPARE(detail::effectiveClipModel(false, 1), false);
}

//截面带双平面：关于"基点+法向×偏移"对称分布，间距=带宽
void TestPostSectionCutLogic::bandVerticesSymmetry()
{
    SectionCutDraft d;
    d.basePoint.text = QStringLiteral("0, 0, 0");
    d.offset = QStringLiteral("10");
    d.bandwidth = QStringLiteral("2");
    const QVector3D xAxis(1, 0, 0);
    const QPair<QVector3D, QVector3D> band = detail::bandVertices(d, xAxis);

    QCOMPARE(band.first, QVector3D(9, 0, 0));       //10−2/2
    QCOMPARE(band.second, QVector3D(11, 0, 0));     //10+2/2

    d.offset = QStringLiteral("0");
    const QPair<QVector3D, QVector3D> centered = detail::bandVertices(d, xAxis, QVector3D(0, 0, 5));
    QCOMPARE(centered.first, QVector3D(-1, 0, 5));
    QCOMPARE(centered.second, QVector3D(1, 0, 5));
}

//最近节点位移近似：取基点就近节点的位移；空表/缺位移项回零
void TestPostSectionCutLogic::nearestNodeDisplacementCases()
{
    QHash<int, QVector3D> positions;
    positions.insert(1, QVector3D(0, 0, 0));
    positions.insert(2, QVector3D(10, 0, 0));
    QHash<int, QVector3D> displacements;
    displacements.insert(1, QVector3D(0.5, 0, 0));
    displacements.insert(2, QVector3D(9, 9, 9));

    QCOMPARE(detail::nearestNodeDisplacement(QVector3D(1, 0, 0), positions, displacements),
        QVector3D(0.5, 0, 0));                      //距节点1更近
    QCOMPARE(detail::nearestNodeDisplacement(QVector3D(8, 0, 0), positions, displacements),
        QVector3D(9, 9, 9));                        //距节点2更近

    QVERIFY(detail::nearestNodeDisplacement(QVector3D(0, 0, 0), QHash<int, QVector3D>(), displacements)
        .isNull());                                 //无节点表→零
    QVERIFY(detail::nearestNodeDisplacement(QVector3D(0, 0, 0), positions, QHash<int, QVector3D>())
        .isNull());                                 //无位移项→零
}

QTEST_MAIN(TestPostSectionCutLogic)
#include "tst_mPostSectionCutLogic.moc"
