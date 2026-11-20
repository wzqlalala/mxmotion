#pragma execution_character_set("utf-8")

// mPostDeformation 纯逻辑回归测试
//
// 覆盖面：变形变量列表过滤（节点变量且前三分量为空间矢量 X/Y/Z 或 T1/T2/T3，
// 张量/标量/伪变量剔除）、默认（文件位移）变量判定、三分量标量场组装节点矢量场
//（并集键集/缺分量补0/空输入）。
// 全部用例只依赖 Qt 容器与 mPostGUIData 纯数据结构（QCoreApplication 即可，无头运行），
// 不触碰视口/渲染/读取器（applyDeformVariableField 依赖渲染端对象，不在无头用例范围）。

#include <QtTest/QtTest>
#include <QHash>
#include <QStringList>
#include <QVector3D>

#include "mPostDeformation.h"
#include "mPostGUIData.h"

using namespace PostWindow;

class TestPostDeformation : public QObject
{
    Q_OBJECT

private slots:
    void filterVariables();
    void filterVariablesKeepsOrderAndDropsUnknown();
    void defaultVariableDetection();
    void assembleField();
    void assembleFieldTolerance();
};

// 变形变量列表过滤：只留"前三分量为空间矢量分量（X/Y/Z 或 T1/T2/T3）的节点变量"——
// 张量（Stress 的 XX/YY/ZZ）、标量、单元变量、未入表名字（伪变量）均剔除
void TestPostDeformation::filterVariables()
{
    QStringList varNames{ "Displacement", "Stress(N)", "Velocity", "Temperature", "SolidColor" };
    QHash<QString, QStringList> composByName;
    composByName.insert("Displacement", QStringList{ "X", "Y", "Z" });
    composByName.insert("Stress(N)", QStringList{ "XX", "YY", "ZZ", "XY", "YZ", "ZX" });
    composByName.insert("Velocity", QStringList{ "X", "Y", "Z", "Mag" });
    composByName.insert("Temperature", QStringList{ "Value" });
    QHash<QString, int> varTypes;
    varTypes.insert("Displacement", 0);
    varTypes.insert("Stress(N)", 0);         // 节点应力张量：分量非空间矢量，剔除
    varTypes.insert("Velocity", 0);
    varTypes.insert("Temperature", 0);       // 标量：分量数<3 剔除

    const QStringList outs = detail::filterDeformVariables(varNames, composByName, varTypes);
    const QStringList expected{ "Displacement", "Velocity" };
    QCOMPARE(outs, expected);
}

// OP2 命名（T1/T2/T3 前缀）放行；单元变量剔除；未入表名字剔除；保持原顺序
void TestPostDeformation::filterVariablesKeepsOrderAndDropsUnknown()
{
    QStringList varNames{ "SolidColor", "Rotation", "U2", "Alpha" };
    QHash<QString, QStringList> composByName;
    composByName.insert("Rotation", QStringList{ "X", "Y", "Z" });
    composByName.insert("U2", QStringList{ "T1", "T2", "T3", "R1", "R2", "R3", "Magnitude" });
    composByName.insert("Alpha", QStringList{ "X", "Y", "Z" });
    QHash<QString, int> varTypes;
    varTypes.insert("Rotation", 0);
    varTypes.insert("U2", 0);
    varTypes.insert("Alpha", 1);      // 单元变量剔除

    const QStringList outs = detail::filterDeformVariables(varNames, composByName, varTypes);
    const QStringList expected{ "Rotation", "U2" };
    QCOMPARE(outs, expected);
}

// 默认判定：空变量/空列表/等于首项 → 默认（文件位移变量）
void TestPostDeformation::defaultVariableDetection()
{
    MechPreData::mPostGUIData guidata;

    // 空变量名 → 默认
    QVERIFY(detail::isDefaultDeformVariable(&guidata));
    // 空指针视作默认（保守：不覆盖位移哈希）
    QVERIFY(detail::isDefaultDeformVariable(nullptr));

    guidata._deformVarNames = QStringList{ "Displacement", "Velocity" };
    // 等于首项 → 默认
    guidata._deformVariable = "Displacement";
    QVERIFY(detail::isDefaultDeformVariable(&guidata));
    // 非首项 → 非默认
    guidata._deformVariable = "Velocity";
    QVERIFY(!detail::isDefaultDeformVariable(&guidata));

    // 列表为空 → 一律视作默认（无候选变量可切）
    guidata._deformVarNames.clear();
    guidata._deformVariable = "Velocity";
    QVERIFY(detail::isDefaultDeformVariable(&guidata));
}

// 组装：取三分量键并集，缺分量补 0；键域与输入一致
void TestPostDeformation::assembleField()
{
    QHash<int, double> x, y, z;
    x.insert(1, 1.5);
    x.insert(2, 2.5);
    y.insert(1, 0.5);
    y.insert(3, 3.5);   // 节点3仅在 Y 有值：X/Z 补 0
    z.insert(1, -1.0);

    const QHash<int, QVector3D> field = detail::assembleNodeVectorField(x, y, z);
    QCOMPARE(field.size(), 3);
    QCOMPARE(field.value(1), QVector3D(1.5f, 0.5f, -1.0f));
    QCOMPARE(field.value(2), QVector3D(2.5f, 0.0f, 0.0f));
    QCOMPARE(field.value(3), QVector3D(0.0f, 3.5f, 0.0f));

    // 全空输入 → 空场
    QVERIFY(detail::assembleNodeVectorField({}, {}, {}).isEmpty());
}

// 组装容错：同一节点在某分量重复键只取一次（QHash 语义），负值/零值不丢弃
void TestPostDeformation::assembleFieldTolerance()
{
    QHash<int, double> x, y, z;
    x.insert(10, -2.0);
    y.insert(10, 0.0);
    z.insert(10, 0.0);

    const QHash<int, QVector3D> field = detail::assembleNodeVectorField(x, y, z);
    QCOMPARE(field.size(), 1);
    QCOMPARE(field.value(10), QVector3D(-2.0f, 0.0f, 0.0f));
}

// 经 mPostDeformation.h 传递包含 QWidget 族会定义 QT_WIDGETS_LIB，QTEST_MAIN 会因此
// 构造 QApplication——无头环境无平台插件会挂死在启动报错上；本用例只测纯逻辑，
// 用 QTEST_GUILESS_MAIN 强制 QCoreApplication（与 tst_mPostTreeModel 同为无头可跑）
QTEST_GUILESS_MAIN(TestPostDeformation)
#include "tst_mPostDeformation.moc"
