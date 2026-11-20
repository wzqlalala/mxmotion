#pragma execution_character_set("utf-8")

// mPostVectorPlot 参数折算纯逻辑回归测试
//
// 覆盖面：向量图渲染通道表构建（面板通道勾选+色盘 → 引擎 VectorGraphParams.labels，
// 顺序固定 X/Y/Z/XY/YZ/XZ/XYZ、仅收勾选项、色值 0-255 域）、符号尺寸三选折算倍率、
// 着色方式 → 引擎 arrowType 折算、引擎通道名 → 面板索引反查。
// 全部用例只依赖 Qt 容器与 QColor（QCoreApplication 即可，无头运行），不触碰
// 视口/渲染（applyVectorPlot 依赖渲染端对象，不在无头用例范围）。

#include <QtTest/QtTest>
#include <QColor>
#include <QHash>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>

#include "mPostVectorPlot.h"

using namespace PostWindow;

class TestPostVectorPlotLogic : public QObject
{
    Q_OBJECT

private slots:
    void buildLabelsOrderAndColors();
    void buildLabelsEmptyWhenNoneChecked();
    void buildLabelsPartialSelection();
    void scaleFolding();
    void arrowTypeFolding();
    void channelIndexLookup();
    void fieldRangeExtremes();
    void fieldRangeEmpty();
    void legendRangeThreeModes();
};

// 通道勾选+色盘 → 引擎通道表：全勾时顺序固定 X/Y/Z/XY/YZ/XZ/XYZ（引擎投影表名域），
// 色值 0-255 原值透传（渲染端 /255 归一）
void TestPostVectorPlotLogic::buildLabelsOrderAndColors()
{
    bool checked[7] = { true, true, true, true, true, true, true };
    QColor colors[7];
    colors[0] = QColor(255, 0, 0);     //X
    colors[1] = QColor(0, 255, 0);     //Y
    colors[2] = QColor(0, 0, 255);     //Z
    colors[3] = QColor(10, 20, 30);    //X+Y
    colors[4] = QColor(40, 50, 60);    //Y+Z
    colors[5] = QColor(70, 80, 90);    //Z+X
    colors[6] = QColor(100, 110, 120); //X+Y+Z

    const QVector<QPair<QString, QVector3D>> labels = detail::buildVectorPlotLabels(checked, colors);
    QCOMPARE(labels.size(), 7);
    QCOMPARE(labels[0].first, QString("X"));
    QCOMPARE(labels[1].first, QString("Y"));
    QCOMPARE(labels[2].first, QString("Z"));
    QCOMPARE(labels[3].first, QString("XY"));
    QCOMPARE(labels[4].first, QString("YZ"));
    QCOMPARE(labels[5].first, QString("XZ"));
    QCOMPARE(labels[6].first, QString("XYZ"));
    QCOMPARE(labels[0].second, QVector3D(255, 0, 0));
    QCOMPARE(labels[3].second, QVector3D(10, 20, 30));
    QCOMPARE(labels[6].second, QVector3D(100, 110, 120));
}

// 全不勾 → 空表（面板校验①已拦，函数自身对空选择返回空）
void TestPostVectorPlotLogic::buildLabelsEmptyWhenNoneChecked()
{
    bool checked[7] = { false, false, false, false, false, false, false };
    QColor colors[7] = { QColor(Qt::black), QColor(Qt::black), QColor(Qt::black), QColor(Qt::black),
        QColor(Qt::black), QColor(Qt::black), QColor(Qt::black) };
    QVERIFY(detail::buildVectorPlotLabels(checked, colors).isEmpty());
}

// 部分勾选：仅收勾选项、保持固定相对顺序（面板默认态=仅 X+Y+Z 勾）
void TestPostVectorPlotLogic::buildLabelsPartialSelection()
{
    bool checked[7] = { false, true, false, false, false, true, true };   //Y/Z+X/X+Y+Z
    QColor colors[7];
    for (int i = 0; i < 7; ++i)
        colors[i] = QColor(1, 2, 3);
    colors[1] = QColor(200, 100, 50);

    const QVector<QPair<QString, QVector3D>> labels = detail::buildVectorPlotLabels(checked, colors);
    QCOMPARE(labels.size(), 3);
    QCOMPARE(labels[0].first, QString("Y"));
    QCOMPARE(labels[0].second, QVector3D(200, 100, 50));
    QCOMPARE(labels[1].first, QString("XZ"));
    QCOMPARE(labels[2].first, QString("XYZ"));
}

// 符号尺寸三选折算：0=自动→1.0；1=统一→统一尺寸值；2=缩放→倍率值；非法档回退 1.0
void TestPostVectorPlotLogic::scaleFolding()
{
    QCOMPARE(detail::vectorPlotScale(0, 5.0, 3.0), 1.0);
    QCOMPARE(detail::vectorPlotScale(1, 5.0, 3.0), 5.0);
    QCOMPARE(detail::vectorPlotScale(2, 5.0, 3.0), 3.0);
    QCOMPARE(detail::vectorPlotScale(9, 5.0, 3.0), 1.0);
}

// 着色方式 → 引擎 arrowType：0=数值(MinMax 色表)→1；1=自定义(通道色 Common)→0
void TestPostVectorPlotLogic::arrowTypeFolding()
{
    QCOMPARE(detail::vectorPlotArrowType(0), 1);
    QCOMPARE(detail::vectorPlotArrowType(1), 0);
}

// 引擎通道名 → 面板通道索引：七个引擎名逐一命中；面板显示名/未知名不命中
void TestPostVectorPlotLogic::channelIndexLookup()
{
    QCOMPARE(detail::vectorPlotChannelIndex(QString("X")), 0);
    QCOMPARE(detail::vectorPlotChannelIndex(QString("Y")), 1);
    QCOMPARE(detail::vectorPlotChannelIndex(QString("Z")), 2);
    QCOMPARE(detail::vectorPlotChannelIndex(QString("XY")), 3);
    QCOMPARE(detail::vectorPlotChannelIndex(QString("YZ")), 4);
    QCOMPARE(detail::vectorPlotChannelIndex(QString("XZ")), 5);
    QCOMPARE(detail::vectorPlotChannelIndex(QString("XYZ")), 6);
    QCOMPARE(detail::vectorPlotChannelIndex(QString("X+Y")), -1);   //面板显示名不在引擎名域
    QCOMPARE(detail::vectorPlotChannelIndex(QString("Mag")), -1);
    QCOMPARE(detail::vectorPlotChannelIndex(QString()), -1);
}

// 矢量场幅值极值：正常场取各向量模长 min/max；全等值场 min==max 合法（非 (0,0) 即可用）
void TestPostVectorPlotLogic::fieldRangeExtremes()
{
    QHash<int, QVector3D> field;
    field.insert(1, QVector3D(3, 4, 0));     //模长 5（最大）
    field.insert(2, QVector3D(0, 0, 0));     //模长 0（最小）
    field.insert(3, QVector3D(1, 0, 0));     //模长 1
    const QPair<double, double> r = detail::vectorFieldRange(field);
    QCOMPARE(r.first, 0.0);
    QCOMPARE(r.second, 5.0);

    QHash<int, QVector3D> constant;
    constant.insert(7, QVector3D(2, 0, 0));
    constant.insert(8, QVector3D(0, 2, 0));
    const QPair<double, double> c = detail::vectorFieldRange(constant);
    QCOMPARE(c.first, 2.0);
    QCOMPARE(c.second, 2.0);
}

// 空场 → (0,0) 不可用标记（云图 getMaxMinByFrameID 空帧同判）
void TestPostVectorPlotLogic::fieldRangeEmpty()
{
    const QPair<double, double> r = detail::vectorFieldRange(QHash<int, QVector3D>());
    QCOMPARE(r.first, 0.0);
    QCOMPARE(r.second, 0.0);
}

// 图例范围三档解析：手动=锁定值；全程=全程域；逐帧=当前帧域；
// 手动不可用/全程不可用（(0,0) 倒挂）均回退逐帧
void TestPostVectorPlotLogic::legendRangeThreeModes()
{
    const QPair<double, double> frame(0.5, 2.5);
    const QPair<double, double> all(0.1, 9.9);

    //逐帧
    QCOMPARE(detail::resolveVectorLegendRange(QString::fromUtf8("逐帧"), 8.0, 1.0, frame, all), frame);
    //全程
    QCOMPARE(detail::resolveVectorLegendRange(QString::fromUtf8("全程"), 8.0, 1.0, frame, all), all);
    //手动=锁定值（返回 <min,max>）
    const QPair<double, double> manual = detail::resolveVectorLegendRange(
        QString::fromUtf8("手动"), 8.0, 1.0, frame, all);
    QCOMPARE(manual.first, 1.0);
    QCOMPARE(manual.second, 8.0);
    //全程不可用（(0,0)）→ 回退逐帧
    QCOMPARE(detail::resolveVectorLegendRange(QString::fromUtf8("全程"), 8.0, 1.0, frame,
        QPair<double, double>(0.0, 0.0)), frame);
    //手动不可用（倒挂 max<min）→ 回退逐帧
    QCOMPARE(detail::resolveVectorLegendRange(QString::fromUtf8("手动"), 0.5, 3.0, frame, all), frame);
    //未知档文本 → 逐帧兜底
    QCOMPARE(detail::resolveVectorLegendRange(QString("frame"), 8.0, 1.0, frame, all), frame);
}

QTEST_MAIN(TestPostVectorPlotLogic)
#include "tst_mPostVectorPlotLogic.moc"
