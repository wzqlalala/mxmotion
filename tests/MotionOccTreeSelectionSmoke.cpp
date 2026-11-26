#include "MotionWindow.h"
#include "NativeMotionExchange.h"
#include "OccViewport.h"
#include "OriginalSelection.h"
#include "compat/mViewportModelManager.h"
#include "compat/model/LegacyMotionContext.h"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <DockManager.h>
#include <DockWidget.h>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QLabel>
#include <QMouseEvent>
#include <QTextEdit>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <algorithm>
#include <stdexcept>

namespace {
using namespace MotionOcc;

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

template<class Predicate>
bool waitFor(Predicate ready, int timeoutMs)
{
    if (ready()) return true;
    QEventLoop wait;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &wait, [&] { if (ready()) wait.quit(); });
    QTimer::singleShot(timeoutMs, &wait, &QEventLoop::quit);
    poll.start(20);
    wait.exec();
    return ready();
}

QString makeFixture(const QTemporaryDir& temporary)
{
    CadDocument cad;
    QString error;
    int box, cylinder;
    require(cad.addPrimitive(Primitive::Box, 30, 20, 16, box, error), error);
    require(cad.addPrimitive(Primitive::Cylinder, 9, 28, 0, cylinder, error), error);
    const QJsonObject body{{"id", 0}, {"name", QStringLiteral("选中测试运动体")}, {"geoEntityIds", QJsonArray{17}},
        {"mass", 2}, {"isCustomMassProps", true}, {"inertia", QJsonArray{100, 100, 100, 0, 0, 0}},
        {"centerOfMass", QJsonObject{{"x", 0}, {"y", 0}, {"z", 0}}}};
    NativeMotionDocument motion;
    motion.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{body}}}}};
    motion.bodyPartBindings.insert(0, {box});
    const auto path = temporary.filePath("selection.motion.json");
    require(writeNativeMotionExchange(path, cad, motion, error), error);
    return path;
}

double click(QWidget& widget, const QPoint& local)
{
    QElapsedTimer timer;
    timer.start();
    QMouseEvent press(QEvent::MouseButtonPress, local, widget.mapToGlobal(local), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, local, widget.mapToGlobal(local), Qt::LeftButton,
                        Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &release);
    return timer.nsecsElapsed() / 1e6;
}

bool isNode(const QTreeWidgetItem* item, const ModelNode& node)
{
    return item && item->data(0, Qt::UserRole + 1).toInt() == int(node.kind)
        && item->data(0, Qt::UserRole).toInt() == node.id
        && item->data(0, Qt::UserRole + 2).toString() == node.collection;
}

QTreeWidgetItem* findNode(QTreeWidget& tree, const ModelNode& node)
{
    for (QTreeWidgetItemIterator it(&tree); *it; ++it)
        if (isNode(*it, node)) return *it;
    return nullptr;
}

double selectRow(QTreeWidget& tree, const ModelNode& node, const QString& stage)
{
    auto* item = findNode(tree, node);
    require(item, QStringLiteral("%1时找不到模型树测试行").arg(stage));
    const QString label = item->text(0);
    tree.scrollToItem(item, QAbstractItemView::PositionAtCenter);
    const auto rect = tree.visualItemRect(item).intersected(tree.viewport()->rect());
    require(!rect.isEmpty(), QStringLiteral("%1时模型树测试行不可见：%2").arg(stage, label));
    const double elapsed = click(*tree.viewport(), rect.center());
    // 原运动面板进入编辑时重建树，校验逻辑节点而非失效的行指针。
    const auto* current = tree.currentItem();
    if (!isNode(current, node) || !current->isSelected()) {
        const auto actual = current ? QStringLiteral("%1 (kind=%2, id=%3, collection=%4, selected=%5)")
            .arg(current->text(0)).arg(current->data(0, Qt::UserRole + 1).toInt())
            .arg(current->data(0, Qt::UserRole).toInt()).arg(current->data(0, Qt::UserRole + 2).toString())
            .arg(current->isSelected()) : QStringLiteral("无");
        throw std::runtime_error(QStringLiteral("%1时未选中 %2 (kind=%3, id=%4, collection=%5)，实际当前行：%6，耗时 %7 ms")
            .arg(stage, label).arg(int(node.kind)).arg(node.id).arg(node.collection, actual).arg(elapsed, 0, 'f', 3).toStdString());
    }
    return elapsed;
}

bool hasPartDetails(const QTableWidget& table, const QString& name)
{
    if (table.rowCount() < 3 || table.columnCount() < 2) return false;
    const auto* part = table.item(table.rowCount() - 1, 0);
    const auto* volume = table.item(table.rowCount() - 1, 1);
    return part && part->text() == name && volume && !volume->text().isEmpty();
}
}

int runMotionOccTreeSelectionSmoke(const QString& outputDirectory, const QString& motionPath)
{
    using namespace MotionOcc;
    const QDir output(outputDirectory);
    if (!QDir().mkpath(output.absolutePath())) return 2;
    QFile report(output.filePath("tree-selection-smoke.txt"));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text)) return 2;
    QTextStream log(&report);
    log.setCodec("UTF-8");
    try {
        QTemporaryDir temporary;
        require(temporary.isValid(), QStringLiteral("无法创建测试目录"));
        const QString input = motionPath.isEmpty() ? makeFixture(temporary) : motionPath;
        MotionWindow window;
        window.resize(1440, 960);
        window.show();
        if (!window.importMotionJson(input, false)) {
            const auto* console = window.findChild<QTextEdit*>(QStringLiteral("motionOutput"));
            throw std::runtime_error(QStringLiteral("运动文件导入失败\n%1")
                .arg(console ? console->toPlainText().right(3000) : QString{}).toStdString());
        }
        auto* tree = window.findChild<QTreeWidget*>("mTreeView");
        auto* viewport = window.findChild<OccViewport*>("occViewport");
        auto* status = window.findChild<QLabel*>("selectionStatus");
        auto* docks = window.findChild<ads::CDockManager*>();
        auto* detailsDock = docks ? docks->dockWidgetsMap().value(QStringLiteral("Select")) : nullptr;
        auto* table = detailsDock ? detailsDock->findChild<QTableWidget*>("selectionInfoTable") : nullptr;
        require(tree && viewport && status && table, QStringLiteral("模型树、视口或选择信息控件缺失"));
        require(waitFor([&] { return viewport->glReady(); }, 10000), QStringLiteral("视口未初始化"));

        ModelNode body;
        QVector<int> parts;
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            auto* item = *it;
            const int kind = item->data(0, Qt::UserRole + 1).toInt();
            const int id = item->data(0, Qt::UserRole).toInt();
            if (kind == ModelNode::Part) parts.append(id);
            if (body.kind == ModelNode::None && kind == ModelNode::Native && item->data(0, Qt::UserRole + 2).toString() == "bodies")
                body = {ModelNode::Native, id, QStringLiteral("bodies")};
        }
        require(parts.size() >= 2 && body.kind == ModelNode::Native, QStringLiteral("测试需要两个部件和一个原始运动体"));
        tree->expandToDepth(2);
        QApplication::processEvents();
        const double firstBodyMs = selectRow(*tree, body, QStringLiteral("首次点击运动体"));
        require(getCVM && getCVM->getModelData()->context, QStringLiteral("原始运动体面板上下文缺失"));
        const auto& cad = getCVM->getModelData()->context->cadDocument();
        std::stable_sort(parts.begin(), parts.end(), [&](int a, int b) {
            return cad.part(a)->display.triangles.size() > cad.part(b)->display.triangles.size();
        });
        const ModelNode first{ModelNode::Part, parts.at(0)};
        const ModelNode second{ModelNode::Part, parts.at(1)};
        window.command(Command::ClearSelection)->trigger();
        const auto neutral = viewport->grabFramebuffer();
        const double firstPartMs = selectRow(*tree, first, QStringLiteral("首次点击部件"));
        require(status->text().contains(QStringLiteral("1 部件")), QStringLiteral("状态栏未即时显示部件选中"));
        require(status->toolTip().contains(cad.part(first.id)->name) && !status->toolTip().contains(QStringLiteral("体积 =")),
            QStringLiteral("单击返回前已等待精确体积，而非先显示部件名称"));
        const auto highlighted = viewport->grabFramebuffer();
        require(!highlighted.isNull() && highlighted != neutral, QStringLiteral("部件选中没有改变视口高亮"));
        click(*viewport, QPoint(viewport->width() - 6, 6));
        require(isNode(tree->currentItem(), first) && tree->currentItem()->isSelected(),
            QStringLiteral("点击视口空白处清除了树选中"));
        const double secondPartMs = selectRow(*tree, second, QStringLiteral("点击第二个部件"));
        const double repeatPartMs = selectRow(*tree, first, QStringLiteral("返回第一个部件"));
        const double repeatBodyMs = selectRow(*tree, body, QStringLiteral("再次点击运动体"));
        const double backToPartMs = selectRow(*tree, first, QStringLiteral("从运动体切回部件"));

        // 比较旧单击路径执行的整装配精确包围盒和同一部件的详细测量。
        QElapsedTimer reference;
        reference.start();
        Bnd_Box bounds;
        for (const auto& part : cad.parts()) BRepBndLib::AddOptimal(part.shape, bounds, false, false);
        const double assemblyBoundsMs = reference.nsecsElapsed() / 1e6;
        require(!bounds.IsVoid(), QStringLiteral("未取得装配包围盒"));
        reference.restart();
        PickResult partPick;
        partPick.partId = first.id;
        const auto exact = describeSelection(cad, {}, {partPick});
        const double exactMeasureMs = reference.nsecsElapsed() / 1e6;
        require(!exact.rows.isEmpty() && exact.rows.last().size() > 1, QStringLiteral("没有基准体积数据"));

        const auto partName = cad.part(first.id)->name;
        QMetaObject::invokeMethod(status, "linkActivated", Qt::DirectConnection, Q_ARG(QString, QStringLiteral("selection")));
        require(!detailsDock->isClosed(), QStringLiteral("状态栏链接没有打开选择信息"));
        require(waitFor([&] { return hasPartDetails(*table, partName); }, 10000), QStringLiteral("选择信息没有更新到当前部件"));
        const auto* displayedVolume = table->item(table->rowCount() - 1, 1);
        require(displayedVolume->text() == exact.rows.last().at(1), QStringLiteral("选择信息的体积与精确测量不一致"));
        require(status->toolTip().contains(QStringLiteral("体积 = %1").arg(displayedVolume->text())),
            QStringLiteral("精确测量完成后状态栏未更新体积"));
        log << "parts=" << parts.size() << '\n'
            << "first_native_node_ms=" << firstBodyMs << '\n' << "repeat_native_node_ms=" << repeatBodyMs << '\n'
            << "first_part_ms=" << firstPartMs << '\n' << "second_part_ms=" << secondPartMs << '\n'
            << "repeat_part_ms=" << repeatPartMs << '\n' << "return_from_native_to_part_ms=" << backToPartMs << '\n'
            << "uncached_assembly_bounds_ms=" << assemblyBoundsMs << '\n' << "exact_part_measurement_ms=" << exactMeasureMs << '\n'
            << "PASS selection, viewport highlight, empty-space retention and exact selection details\n";
        log.flush();
        return 0;
    } catch (const std::exception& error) {
        log << "FAIL " << QString::fromUtf8(error.what()) << '\n';
        log.flush();
        return 1;
    }
}
