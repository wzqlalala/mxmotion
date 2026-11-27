#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidPlot.h"
#include "RigidProject.h"

#include <DockManager.h>
#include <DockAreaWidget.h>
#include <DockWidget.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_map.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QLineEdit>
#include <QMenu>
#include <QSlider>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPushButton>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTabBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextLayout>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWheelEvent>
#include <functional>
#include <cmath>
#include <stdexcept>

namespace {
using namespace MotionOcc;

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

bool waitFor(const std::function<bool()>& predicate, int timeoutMs)
{
    if (predicate()) return true;
    QEventLoop loop;
    QTimer check;
    QObject::connect(&check, &QTimer::timeout, &loop, [&] { if (predicate()) loop.quit(); });
    check.start(20);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
    return predicate();
}

void mouse(QWidget& widget, QEvent::Type type, const QPoint& pos, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, pos, widget.mapToGlobal(pos), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(&widget, &event);
}

QImage frame(OccViewport& viewport)
{
    const auto image = viewport.grabFramebuffer();
    require(!image.isNull() && image.width() > 50 && image.height() > 50, QStringLiteral("Empty OpenGL framebuffer"));
    return image;
}

QTreeWidgetItem* findNode(QTreeWidget& tree, ModelNode::Kind kind)
{
    for (QTreeWidgetItemIterator it(&tree); *it; ++it)
        if ((*it)->data(0, Qt::UserRole + 1).toInt() == int(kind)) return *it;
    return nullptr;
}

int ribbonTabIndex(const QTabBar& tabs, const QString& title)
{
    for (int index = 0; index < tabs.count(); ++index)
        if (tabs.tabText(index) == title) return index;
    return -1;
}

QToolButton* resultRibbonButton(MotionWindow& window, Command id)
{
    const auto* action = window.command(id);
    const auto* ribbon = window.findChild<QWidget*>(QStringLiteral("mMenuWidget"));
    if (!action || !ribbon) return nullptr;
    for (auto* button : ribbon->findChildren<QToolButton*>())
        if (button->isVisible() && button->accessibleName() == action->text()) return button;
    return nullptr;
}

void requireLegend(QwtPlot& plot)
{
    auto* legend = qobject_cast<QwtLegend*>(plot.legend());
    require(legend, QStringLiteral("Curve legend object is missing"));
    bool visible = legend->isVisible() && !legend->visibleRegion().isEmpty();
    for (auto* curve : plot.itemList(QwtPlotItem::Rtti_PlotCurve)) {
        const auto entries = legend->legendWidgets(plot.itemToInfo(curve));
        require(entries.size() == 1, QStringLiteral("A plotted curve has no unique legend entry"));
        auto* label = qobject_cast<QwtLegendLabel*>(entries.front());
        require(label && label->text().text() == curve->title().text(), QStringLiteral("Legend has lost a curve quantity/unit"));
        require(label->updatesEnabled(), QStringLiteral("Legend label remains explicitly paint-disabled after reopening its dock"));
        const auto pixels = label->grab().toImage();
        const QColor expected = static_cast<QwtPlotCurve*>(curve)->pen().color();
        int colored = 0;
        for (int y = 0; y < pixels.height(); ++y)
            for (int x = 0; x < pixels.width(); ++x) {
                const QColor actual = pixels.pixelColor(x, y);
                if (actual.alpha() > 128 && std::abs(actual.red() - expected.red()) < 20
                    && std::abs(actual.green() - expected.green()) < 20 && std::abs(actual.blue() - expected.blue()) < 20) ++colored;
            }
        require(colored >= 3, QStringLiteral("Legend label has no rendered series color"));
        visible = visible && label->isVisible() && !label->visibleRegion().isEmpty();
    }
    require(visible, QStringLiteral("A saved curve legend entry is hidden or clipped on first display"));
}

void requireCurveRows(QListWidget& list)
{
    bool fit = true;
    for (int index = 0; index < list.count(); ++index) {
        auto* row = list.itemWidget(list.item(index));
        require(row, QStringLiteral("A curve row is missing"));
        QPushButton* remove = nullptr;
        for (auto* button : row->findChildren<QPushButton*>())
            if (button->text() == QStringLiteral("删除")) remove = button;
        require(remove, QStringLiteral("Curve removal control is missing"));
        const auto left = remove->mapTo(list.viewport(), remove->rect().topLeft()).x();
        const auto right = remove->mapTo(list.viewport(), remove->rect().topRight()).x();
        fit = fit && left >= 0 && right < list.viewport()->width();
    }
    require(fit, QStringLiteral("Curve removal buttons are clipped by the narrow parameter column"));
}
}

int runMotionOccDesktopSmoke(QApplication&, const QString& outputDirectory)
{
    using namespace MotionOcc;
    const QDir output(outputDirectory);
    QFile report(output.filePath(QStringLiteral("desktop-smoke.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 2;
    const auto line = [&report](const QString& value) {
        report.write(value.toUtf8());
        report.write("\n");
        report.flush();
    };

    try {
        QTemporaryDir temp;
        require(temp.isValid(), QStringLiteral("No fixture directory"));
        CadDocument fixture;
        int id = -1;
        QString error;
        require(fixture.addPrimitive(Primitive::Box, 30, 20, 16, id, error), QStringLiteral("Could not create CAD fixture"));
        const auto step = temp.filePath(QStringLiteral("桌面测试.step"));
        require(fixture.exportFile(step, id, error), QStringLiteral("Could not write STEP fixture: %1").arg(error));

        // A project opened from the command line has its requests before the
        // initially closed ADS curve dock has ever been shown. Exercise that
        // lifecycle independently of adding fresh curves to an already shown plot.
        RigidModel savedModel;
        savedModel.bodies.push_back({41, QStringLiteral("转子 é"), id, false, 2.5, 2500});
        savedModel.joints.push_back({73, QStringLiteral("斜轴铰链"), JointKind::Revolute, 41, 0, {17, -13, 11}, {2, -3, 6}});
        savedModel.drives.push_back({109, QStringLiteral("恒速驱动"), 73, DriveKind::Speed, 105});
        savedModel.scenarios.push_back({151, QStringLiteral("偏置旋转"), 2.003, 0.005, {0.4, 0.1, -9.8}});
        savedModel.scenarios.front().curveRequests = {
            {1, RigidCurveMetric::JointCoordinate, 73, 0, QStringLiteral("旋转副相对转角"), QStringLiteral("#2176c1"), true},
            {2, RigidCurveMetric::BodyPosition, 41, 0, QStringLiteral("质心世界位置 X"), QStringLiteral("#d35f39"), true},
            {3, RigidCurveMetric::BodyDisplacement, 41, 0, QStringLiteral("质心位移增量 X"), QStringLiteral("#24946b"), false}
        };
        const auto savedProject = temp.filePath(QStringLiteral("saved-curves.mxmotion"));
        require(saveRigidProject(savedProject, fixture, savedModel, error), QStringLiteral("Could not save curve project fixture: %1").arg(error));
        {
            MotionWindow storedWindow;
            storedWindow.resize(1300, 760);
            require(storedWindow.openRigidProject(savedProject), QStringLiteral("Could not open saved curve project"));
            storedWindow.showNormal();
            auto* storedViewport = storedWindow.findChild<OccViewport*>(QStringLiteral("occViewport"));
            require(storedViewport && waitFor([&] { return storedViewport->glReady(); }, 12000),
                    QStringLiteral("Saved project viewport did not initialize"));
            auto* storedTree = storedWindow.findChild<QTreeWidget*>(QStringLiteral("mTreeView"));
            require(storedTree && findNode(*storedTree, ModelNode::Scenario), QStringLiteral("Saved scenario is missing"));
            storedTree->setCurrentItem(findNode(*storedTree, ModelNode::Scenario));
            auto* resultTabs = storedWindow.findChild<QTabBar*>(QStringLiteral("MenuTabBar"));
            require(resultTabs && ribbonTabIndex(*resultTabs, QStringLiteral("结果")) >= 0,
                QStringLiteral("Result ribbon tab is missing"));
            resultTabs->setCurrentIndex(ribbonTabIndex(*resultTabs, QStringLiteral("结果")));
            QApplication::processEvents();
            for (const auto command : {Command::ShowAnimation, Command::ShowResultCurves, Command::ExportResult}) {
                const auto* button = resultRibbonButton(storedWindow, command);
                require(button && !button->isEnabled() && !storedWindow.command(command)->isEnabled(),
                    QStringLiteral("Unsolved project has an enabled result ribbon command"));
            }
            storedWindow.command(Command::Solve)->trigger();
            require(waitFor([&] {
                const auto* node = findNode(*storedTree, ModelNode::Curves);
                return node && !node->isDisabled() && !storedWindow.command(Command::StopSolve)->isEnabled();
            }, 20000), QStringLiteral("Saved project did not solve"));
            auto* storedDocks = storedWindow.findChild<ads::CDockManager*>();
            auto* storedCurveDock = storedDocks ? storedDocks->dockWidgetsMap().value(QStringLiteral("Curve")) : nullptr;
            auto* storedPlot = storedCurveDock ? storedCurveDock->widget()->findChild<QwtPlot*>() : nullptr;
            require(storedPlot, QStringLiteral("Saved project curve dock has no plot"));
            require(storedCurveDock->isClosed() && !storedPlot->isVisible(), QStringLiteral("Saved requests opened the plot before curve selection"));
            resultTabs->setCurrentIndex(ribbonTabIndex(*resultTabs, QStringLiteral("结果")));
            QApplication::processEvents();
            for (const auto command : {Command::ShowAnimation, Command::ShowResultCurves, Command::ExportResult}) {
                const auto* button = resultRibbonButton(storedWindow, command);
                require(button && button->isEnabled() && storedWindow.command(command)->isEnabled(),
                    QStringLiteral("Solved project is missing an enabled result ribbon command"));
            }
            resultRibbonButton(storedWindow, Command::ShowResultCurves)->click();
            QApplication::processEvents();
            require(storedPlot->isVisible() && storedPlot->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 2,
                    QStringLiteral("Ribbon did not open the original saved-project curve display and requests"));
            require(storedWindow.grab().save(output.filePath(QStringLiteral("persisted-curve-configuration.png"))),
                    QStringLiteral("Could not capture saved-project curve configuration"));
            requireLegend(*storedPlot);
            auto* storedList = storedWindow.findChild<QListWidget*>(QStringLiteral("motionCurveList"));
            require(storedList && storedList->count() == 3, QStringLiteral("Saved curve list lost a hidden request"));
            requireCurveRows(*storedList);
            line(QStringLiteral("Result ribbon stayed disabled before solve and opened saved curves with their original requests after solve"));
        }

        MotionWindow window;
        window.resize(1300, 760);
        window.showNormal();
        auto* viewport = window.findChild<OccViewport*>(QStringLiteral("occViewport"));
        require(viewport != nullptr, QStringLiteral("Viewport missing"));
        QString graphicsError;
        QObject::connect(viewport, &OccViewport::graphicsError, &window, [&graphicsError](const QString& reason) {
            graphicsError = reason;
        });
        require(waitFor([&] { return viewport->glReady() || !graphicsError.isEmpty(); }, 12000)
                && viewport->glReady(), QStringLiteral("OpenGL initialization failed: %1").arg(graphicsError));
        const auto gl = viewport->context()->format();
        line(QStringLiteral("Desktop OpenGL %1.%2 core initialized").arg(gl.majorVersion()).arg(gl.minorVersion()));

        auto* tabs = window.findChild<QTabBar*>(QStringLiteral("MenuTabBar"));
        const QStringList expectedTabs{"主页", "几何", "体定义", "运动副与约束", "驱动", "连接与力元", "传感与控制", "机电液",
            "求解", "显示", "选择", "柔性体", "航空", "车辆", "传动", "机器人", "结果", "振动耐久", "设计分析", "自动化"};
        require(tabs && tabs->count() == expectedTabs.size(), QStringLiteral("Motion ribbon tab count changed"));
        for (const auto& title : expectedTabs)
            require(ribbonTabIndex(*tabs, title) >= 0, QStringLiteral("Motion ribbon tab is missing: %1").arg(title));
        const int startingTab = tabs->currentIndex();
        auto* ribbon = window.findChild<QWidget*>(QStringLiteral("mMenuWidget"));
        require(ribbon, QStringLiteral("Motion ribbon is missing"));
        const auto ribbonButtons = ribbon->findChildren<QToolButton*>();
        for (const auto* button : ribbonButtons) {
            if (!button->property("mxPlannedFeature").toBool()) continue;
            require(!button->isEnabled() && button->property("mxOriginalAction").toString().isEmpty(),
                QStringLiteral("Planned command has a live action: %1").arg(button->text()));
        }
        const QStringList expectedMultiphysics{};
        const struct {
            QString tab;
            QString image;
            QStringList commands;
        } previews[]{
            {"柔性体", "planned-flexible-bodies-ribbon.png", {"MNF 生成向导", "ANCF 梁"}},
            {"航空", "planned-aircraft-ribbon.png", {"舱门模板", "应急放下"}},
            {"车辆", "planned-vehicle-ribbon.png", {"轮胎力学", "路面不平度"}},
            {"传动", "planned-transmission-ribbon.png", {"带传动模板"}},
            {"机器人", "planned-robotics-ribbon.png", {"末端定位"}},
            {"结果", "planned-results-ribbon.png", {"频谱分析"}},
            {"振动耐久", "planned-vibration-ribbon.png", {"疲劳分析"}},
            {"设计分析", "planned-design-analysis-ribbon.png", {"试验设计", "灵敏度分析"}},
            {"自动化", "planned-automation-ribbon.png", {"宏录制", "任务队列"}},
            {"机电液", "planned-multiphysics-ribbon.png", expectedMultiphysics}
        };
        for (const auto* menu : window.findChildren<QMenu*>()) {
            require(menu->title() != QStringLiteral("后处理常驻菜单栏")
                && menu->title() != QStringLiteral("曲线常驻菜单栏"),
                QStringLiteral("Unused post-processing ribbon settings leaked into Motion"));
        }
        const auto originalSize = window.size();
        window.resize(1600, originalSize.height());
        for (const auto& preview : previews) {
            tabs->setCurrentIndex(ribbonTabIndex(*tabs, preview.tab));
            QApplication::processEvents();
            QStringList plannedCommands;
            int plannedClicks = 0;
            for (auto* button : ribbonButtons) {
                if (!button->isVisible() || !button->property("mxPlannedFeature").toBool()) continue;
                plannedCommands.append(button->text());
                const auto connection = QObject::connect(button, &QToolButton::clicked, button,
                    [&plannedClicks] { ++plannedClicks; });
                button->click();
                QObject::disconnect(connection);
            }
            require(plannedClicks == 0, QStringLiteral("Planned ribbon command responded to a click in %1").arg(preview.tab));
            for (const auto& command : preview.commands)
                require(plannedCommands.count(command) == 1,
                    QStringLiteral("Planned ribbon command missing or duplicated in %1: %2").arg(preview.tab, command));
            if (preview.tab == QStringLiteral("机电液")) {
                require(plannedCommands.size() == expectedMultiphysics.size(), QStringLiteral("Unexpected multiphysics ribbon command"));
                bool hasCylinderBtn = false, hasMotorBtn = false, hasPumpBtn = false,
                     hasValveBtn = false, hasCheckBtn = false, hasFluidBtn = false,
                     hasElecMotorBtn = false, hasSupplyBtn = false, hasInverterBtn = false,
                     hasEhaBtn = false, hasEmaBtn = false, hasSchemBtn = false;
                for (auto* button : ribbonButtons) {
                    if (!button->isVisible() || button->property("mxPlannedFeature").toBool()) continue;
                    if (button->text() == QStringLiteral("液压缸")) hasCylinderBtn = true;
                    if (button->text() == QStringLiteral("液压马达")) hasMotorBtn = true;
                    if (button->text() == QStringLiteral("液压泵")) {
                        hasPumpBtn = true;
                        require(button->isEnabled(), QStringLiteral("Hydraulic pump button should be enabled"));
                    }
                    if (button->text() == QStringLiteral("换向阀")) hasValveBtn = true;
                    if (button->text() == QStringLiteral("电机") && button->isEnabled()) hasElecMotorBtn = true;
                    if (button->text() == QStringLiteral("电源") && button->isEnabled()) hasSupplyBtn = true;
                    if (button->text() == QStringLiteral("逆变器") && button->isEnabled()) hasInverterBtn = true;
                    if (button->text() == QStringLiteral("EHA") && button->isEnabled()) hasEhaBtn = true;
                    if (button->text() == QStringLiteral("EMA") && button->isEnabled()) hasEmaBtn = true;
                    if (button->text() == QStringLiteral("回路图") && button->isEnabled()) hasSchemBtn = true;
                    if (button->text() == QStringLiteral("回路检查")) {
                        hasCheckBtn = true;
                        require(button->isEnabled(), QStringLiteral("Hydraulic circuit check button should be enabled"));
                    }
                    if (button->text() == QStringLiteral("油液设置")) {
                        hasFluidBtn = true;
                        require(button->isEnabled(), QStringLiteral("Hydraulic fluid settings button should be enabled"));
                    }
                }
                require(hasCylinderBtn && hasMotorBtn && hasPumpBtn && hasValveBtn && hasCheckBtn && hasFluidBtn
                        && hasElecMotorBtn && hasSupplyBtn && hasInverterBtn && hasEhaBtn && hasEmaBtn && hasSchemBtn,
                        QStringLiteral("Active multiphysics ribbon commands missing"));
            }
            require(ribbon->grab().save(output.filePath(preview.image)),
                QStringLiteral("Could not capture ribbon tab: %1").arg(preview.tab));
        }
        window.resize(originalSize);
        tabs->setCurrentIndex(startingTab);
        QApplication::processEvents();
        const auto* docks = window.findChild<ads::CDockManager*>();
        require(docks != nullptr, QStringLiteral("ADS manager missing"));
        const auto panes = docks->dockWidgetsMap();
        for (const auto& key : { QStringLiteral("Center"), QStringLiteral("Tree"), QStringLiteral("Info") }) {
            auto* pane = panes.value(key);
            require(pane && pane->isVisible() && !pane->isClosed(), QStringLiteral("ADS pane not visible: %1").arg(key));
        }
        line(QStringLiteral("ADS model, parameters and viewport visible; 20 verified tabs and 10 module previews"));

        auto* outputDock = panes.value(QStringLiteral("Output"));
        auto* messages = window.findChild<QTextEdit*>(QStringLiteral("motionOutput"));
        const auto* status = window.statusBar()->findChild<QLabel*>(QStringLiteral("motionStatusNotice"));
        require(status && status->text().isEmpty() && outputDock && messages && messages->isReadOnly()
            && messages->document()->isEmpty(), QStringLiteral("The idle status or information output shows startup details"));

        const auto empty = frame(*viewport);
        require(window.importCad(step), QStringLiteral("STEP import failed through the window"));
        require(waitFor([&] { return frame(*viewport) != empty; }, 4000), QStringLiteral("No CAD frame after import"));
        const auto importNotice = status->text();
        const QRegularExpression dated(QStringLiteral(R"(^>>  .+  \[  [0-9]{2}:[0-9]{2}:[0-9]{2}  \]$)"));
        const auto importLine = messages->document()->lastBlock().text();
        require(importNotice.startsWith(QStringLiteral("已导入")) && importLine.contains(importNotice)
            && dated.match(importLine).hasMatch(), QStringLiteral("The console lost its dated message formatting"));
        require(!window.importCad(temp.filePath(QStringLiteral("不存在.step"))),
            QStringLiteral("Missing CAD fixture did not generate an error"));
        require(messages->document()->lastBlock().text().contains(status->text()),
            QStringLiteral("The failed import was not reported in the status bar and information output"));
        outputDock->toggleView(true);
        outputDock->dockAreaWidget()->setCurrentDockWidget(outputDock);
        QApplication::processEvents();
        const auto selection = messages->document()->lastBlock().layout()->formats();
        require(!selection.isEmpty() && selection.front().format.foreground().color().red() == 255
            && messages->font().pixelSize() == 14, QStringLiteral("Information output lost the original font size or error styling"));
        require(outputDock->grab().save(output.filePath(QStringLiteral("information-output.png")))
            && window.statusBar()->grab().save(output.filePath(QStringLiteral("motion-status.png"))),
            QStringLiteral("Could not capture the information output and status bar"));
        outputDock->toggleView(false);
        line(QStringLiteral("Status bar and information output verified: empty at startup, dated notices and colored errors"));

        auto* tree = window.findChild<QTreeWidget*>(QStringLiteral("mTreeView"));
        require(tree && tree->currentItem() && tree->currentItem()->parent()
            && tree->currentItem()->parent()->text(0) == QStringLiteral("部件"), QStringLiteral("No imported part selected in tree"));

        auto* transparency = window.findChild<QSlider*>(QStringLiteral("partTransparencySlider"));
        auto* transparencySpin = window.findChild<QDoubleSpinBox*>(QStringLiteral("partTransparencySpin"));
        require(transparency && transparency->isVisible() && transparencySpin && transparencySpin->isVisible()
            && transparency->value() == 0, QStringLiteral("CAD part transparency controls are missing or not opaque by default"));
        const int transparentPart = tree->currentItem()->data(0, Qt::UserRole).toInt();
        const auto centerColor = [](const QImage& image) { return image.pixelColor(image.rect().center()); };
        const auto colorDistance = [](const QColor& a, const QColor& b) {
            return std::abs(a.red() - b.red()) + std::abs(a.green() - b.green()) + std::abs(a.blue() - b.blue());
        };
        const auto opaqueColor = centerColor(frame(*viewport));
        transparency->setValue(65);
        require(std::abs(transparencySpin->value() - 0.65) < 1e-9, QStringLiteral("Slider and numeric transparency values differ"));
        const auto mixed = frame(*viewport);
        transparencySpin->setValue(1.0);
        const auto clearColor = centerColor(frame(*viewport));
        require(transparency->value() == 100 && colorDistance(clearColor, centerColor(empty)) < 8
            && colorDistance(centerColor(mixed), opaqueColor) > 10 && colorDistance(centerColor(mixed), clearColor) > 10,
            QStringLiteral("The CAD face did not blend between opaque and fully transparent"));
        require(viewport->pickAt(viewport->rect().center()).partId == transparentPart && tree->currentItem()->isSelected(),
            QStringLiteral("Transparency changed CAD picking or cleared the model tree selection"));
        require(mixed.save(output.filePath(QStringLiteral("cad-transparency.png"))), QStringLiteral("Could not save the transparent CAD frame"));

        transparencySpin->setValue(0.65);
        const auto presentationProject = temp.filePath(QStringLiteral("cad-transparency.mxmotion"));
        QTimer selectPresentationPath;
        QObject::connect(&selectPresentationPath, &QTimer::timeout, &window, [&] {
            if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) {
                dialog->selectFile(presentationProject);
                if (auto* name = dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))) name->setText(presentationProject);
                QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
            }
        });
        QTimer stopPresentationDialog;
        stopPresentationDialog.setSingleShot(true);
        QObject::connect(&stopPresentationDialog, &QTimer::timeout, &window, [] {
            if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) dialog->reject();
        });
        stopPresentationDialog.start(8000);
        selectPresentationPath.start(20);
        window.command(Command::SaveAs)->trigger();
        selectPresentationPath.stop();
        stopPresentationDialog.stop();
        require(window.openRigidProject(presentationProject) && findNode(*tree, ModelNode::Part),
            QStringLiteral("Could not reopen the project saved through CAD part properties"));
        tree->setCurrentItem(findNode(*tree, ModelNode::Part));
        require(transparency->value() == 65 && std::abs(transparencySpin->value() - 0.65) < 1e-9,
            QStringLiteral("Saved CAD transparency was not restored to the property controls"));
        transparencySpin->setValue(0.0);
        line(QStringLiteral("CAD part transparency slider, actual framebuffer, picking and project save/reopen passed"));

        auto* toolbar = window.findChild<QToolBar*>(QStringLiteral("viewToolBar"));
        auto* pickMode = window.findChild<QToolButton*>(QStringLiteral("viewportPickMode"));
        auto* statusSelection = window.findChild<QLabel*>(QStringLiteral("selectionStatus"));
        auto* selectionDock = panes.value(QStringLiteral("Select"));
        require(toolbar && pickMode && pickMode->menu() && statusSelection && selectionDock,
            QStringLiteral("Viewport picking controls or selection status missing"));
        auto* pickFace = pickMode->findChild<QAction*>(QStringLiteral("pickFace"));
        auto* pickPart = pickMode->findChild<QAction*>(QStringLiteral("pickPart"));
        require(pickFace && pickPart && toolbar->actions().contains(pickFace) && pickPart->isChecked(),
            QStringLiteral("Picking mode actions are not shared by the toolbar and menu"));
        pickFace->trigger();
        window.command(Command::Top)->trigger();
        const auto facePoint = viewport->rect().center();
        mouse(*viewport, QEvent::MouseButtonPress, facePoint, Qt::LeftButton, Qt::LeftButton);
        mouse(*viewport, QEvent::MouseButtonRelease, facePoint, Qt::LeftButton, Qt::NoButton);
        require(!viewport->isPicking() && viewport->viewportSelection().size() == 1
            && viewport->viewportSelection().front().kind == PickKind::Face
            && statusSelection->text().contains(QStringLiteral("1 几何面")),
            QStringLiteral("Face mode did not select geometry and update the status bar"));
        QApplication::processEvents();
        const auto statusPoint = statusSelection->rect().center();
        mouse(*statusSelection, QEvent::MouseButtonPress, statusPoint, Qt::LeftButton, Qt::LeftButton);
        mouse(*statusSelection, QEvent::MouseButtonRelease, statusPoint, Qt::LeftButton, Qt::NoButton);
        require(!selectionDock->isClosed(), QStringLiteral("Selection status did not open the details dock"));
        auto* selectionTable = selectionDock->findChild<QTableWidget*>(QStringLiteral("selectionInfoTable"));
        require(selectionTable && selectionTable->cellWidget(0, 0),
            QStringLiteral("Selection dock did not show the original structured information table"));
        selectionDock->toggleView(false);
        viewport->beginPick(PickKind::Vertex);
        require(!pickFace->isEnabled(), QStringLiteral("Toolbar mode can override an editor's pick filter"));
        viewport->endPick();
        require(pickFace->isEnabled() && pickFace->isChecked() && viewport->viewportSelection().size() == 1,
            QStringLiteral("Editor session did not restore viewport pick state"));
        pickPart->trigger();
        window.command(Command::Isometric)->trigger();
        window.command(Command::Fit)->trigger();
        line(QStringLiteral("Viewport face filter, selection status dock and editor pick isolation passed"));

        window.command(Command::ClearSelection)->trigger();
        require(tree->selectedItems().isEmpty(), QStringLiteral("Clear Selection did not clear the model tree"));
        const auto center = viewport->rect().center();
        mouse(*viewport, QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
        mouse(*viewport, QEvent::MouseButtonRelease, center, Qt::LeftButton, Qt::NoButton);
        require(tree->currentItem() && tree->currentItem()->parent()
            && tree->currentItem()->parent()->text(0) == QStringLiteral("部件"), QStringLiteral("Viewport picking did not update the model tree"));
        const auto* selectedPart = tree->currentItem();
        const QPoint blank(viewport->width() - 6, 6);
        mouse(*viewport, QEvent::MouseButtonPress, blank, Qt::LeftButton, Qt::LeftButton);
        mouse(*viewport, QEvent::MouseButtonRelease, blank, Qt::LeftButton, Qt::NoButton);
        require(tree->currentItem() == selectedPart && selectedPart->isSelected(),
            QStringLiteral("Clicking empty viewport space cleared the selected model tree row"));
        line(QStringLiteral("Part picking, explicit clearing and empty-space tree selection passed"));
        const auto original = frame(*viewport);
        require(original.save(output.filePath(QStringLiteral("cad-viewport.png"))), QStringLiteral("CAD frame could not be saved"));

        const auto end = center + QPoint(54, 24);
        mouse(*viewport, QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
        mouse(*viewport, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        mouse(*viewport, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
        const auto rotated = frame(*viewport);
        require(rotated != original, QStringLiteral("CAD frame did not change after orbit"));
        QWheelEvent wheel(center, viewport->mapToGlobal(center), {}, { 0, 120 }, Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(viewport, &wheel);
        require(frame(*viewport) != rotated, QStringLiteral("CAD frame did not change after zoom"));
        window.command(Command::Isometric)->trigger();
        window.command(Command::Fit)->trigger();
        line(QStringLiteral("STEP import, part picking, orbit, zoom and fit passed"));

        window.command(Command::GenerateMesh)->trigger();
        auto* generate = window.findChild<QPushButton*>(QStringLiteral("generateMeshButton"));
        require(generate && generate->isVisible() && generate->isEnabled(), QStringLiteral("Mesh parameters not available"));
        generate->click();
        require(waitFor([&] { return window.command(Command::ExportMesh)->isEnabled(); }, 30000),
            QStringLiteral("No validated mesh from the window's Netgen worker"));
        auto* summary = window.findChild<QLabel*>(QStringLiteral("partSummary"));
        require(summary && summary->text().contains(QStringLiteral("四面体："))
            && !summary->text().contains(QStringLiteral("四面体：0")), QStringLiteral("No tetrahedra shown in parameters"));
        auto* grid = window.command(Command::MeshDisplay);
        require(grid->isEnabled() && grid->isChecked(), QStringLiteral("Real mesh display not selected"));
        const auto mesh = frame(*viewport);
        transparencySpin->setValue(0.75);
        require(frame(*viewport) == mesh, QStringLiteral("CAD transparency modified the finite element mesh display"));
        transparencySpin->setValue(0.0);
        window.command(Command::ShadedEdges)->trigger();
        require(frame(*viewport) != mesh, QStringLiteral("Finite-element grid did not change the OpenGL frame"));
        grid->trigger();
        require(mesh.save(output.filePath(QStringLiteral("mesh-viewport.png"))), QStringLiteral("Mesh frame could not be saved"));
        require(window.grab().save(output.filePath(QStringLiteral("mxmotion.png"))), QStringLiteral("Window image could not be saved"));
        line(summary->text());

        const auto* meshedPart = tree->currentItem();
        require(meshedPart, QStringLiteral("No meshed part selected for measurement"));
        const int meshPart = meshedPart->data(0, Qt::UserRole).toInt();
        PickResult firstNode, secondNode;
        firstNode.kind = secondNode.kind = PickKind::Node;
        firstNode.partId = secondNode.partId = meshPart;
        firstNode.index = 0;
        secondNode.index = 1;
        viewport->beginPick(PickKind::Node, true, {meshPart}, {firstNode, secondNode});
        const auto nodes = viewport->pickSelection();
        require(nodes.size() == 2, QStringLiteral("Measurement fixture has no two mesh nodes"));
        const auto& a = nodes[0].position;
        const auto& b = nodes[1].position;
        const double distance = std::hypot(b[0]-a[0], b[1]-a[1], b[2]-a[2]);
        selectionDock->toggleView(true);
        QApplication::processEvents();
        require(selectionTable->rowCount() == 5 && selectionTable->columnCount() == 6
            && selectionTable->item(1, 0) && selectionTable->item(1, 0)->text() == QStringLiteral("距离(mm)"),
            QStringLiteral("Two selected nodes did not open the original measurement rows"));
        const auto* distanceCell = selectionTable->item(1, 1);
        const auto* firstId = selectionTable->item(3, 5);
        const auto* secondId = selectionTable->item(4, 5);
        require(distanceCell && distance > 0 && std::abs(distanceCell->text().toDouble() - distance) < 1e-5 * distance
            && firstId && firstId->text().toInt() == nodes[0].nodeId
            && secondId && secondId->text().toInt() == nodes[1].nodeId,
            QStringLiteral("Selection measurement did not report actual node IDs and distance"));
        require(window.grab().save(output.filePath(QStringLiteral("selection-info.png"))),
            QStringLiteral("Selection information panel could not be captured"));
        selectionDock->toggleView(false);
        viewport->endPick();
        line(QStringLiteral("Original selection table shows real node IDs and point-to-point distance"));
#ifdef MX_MOTION_OCC_COMPAT
        // Generated motion forms were removed. Their replacement is exercised
        // through the actual uploaded panels in MotionOcc.NativeDesktop.
        line(QStringLiteral("Original desktop chrome / CAD / real Netgen mesh passed; original motion editor workflow is covered by NativeDesktop."));
        return 0;
#endif
        auto applyForm = [&] {
            const auto buttons = window.findChildren<QPushButton*>(QStringLiteral("motionApplyButton"));
            for (auto* button : buttons) if (button->isVisible()) { button->click(); return; }
            require(false, QStringLiteral("Motion parameter editor not visible"));
        };
        window.command(Command::AddBody)->trigger();
        applyForm();
        require(window.command(Command::AddRevolute)->isEnabled(), QStringLiteral("Motion body not created"));
        window.command(Command::AddRevolute)->trigger();
        applyForm();
        require(window.command(Command::AddSpeedDrive)->isEnabled(), QStringLiteral("Revolute joint not created"));
        window.command(Command::AddSpeedDrive)->trigger();
        if (auto* value=window.findChild<QLineEdit*>("native_expression")) value->setText("90");
        for (auto* value : window.findChildren<QDoubleSpinBox*>())
            if (value->isVisible() && value->suffix().contains(QStringLiteral("°/s"))) value->setValue(90);
        applyForm();
        window.command(Command::NewScenario)->trigger();
        applyForm();
        auto resultNode = [tree](ModelNode::Kind kind) -> QTreeWidgetItem* {
            for (QTreeWidgetItemIterator it(tree); *it; ++it)
                if ((*it)->data(0, Qt::UserRole + 1).toInt() == int(kind)) return *it;
            return nullptr;
        };
        auto partGroup = [tree]() -> QTreeWidgetItem* {
            for (QTreeWidgetItemIterator it(tree); *it; ++it)
                if ((*it)->text(0) == QStringLiteral("部件")) return *it;
            return nullptr;
        };
        require(partGroup(), QStringLiteral("Missing parts group"));
        partGroup()->setExpanded(false);
        window.command(Command::Solve)->trigger();
        auto* curveDock = panes.value(QStringLiteral("Curve"));
        auto* plot = curveDock ? curveDock->widget()->findChild<QwtPlot*>() : nullptr;
        require(plot, QStringLiteral("QwtPlot object not created"));
        require(waitFor([&] {
            const auto* node = resultNode(ModelNode::Animation);
            return node && !node->isDisabled() && !window.command(Command::StopSolve)->isEnabled();
        }, 20000), QStringLiteral("MotionEngine did not enable the result nodes"));
        require(curveDock->isClosed() && plot->itemList(QwtPlotItem::Rtti_PlotCurve).isEmpty(),
            QStringLiteral("Solving must not automatically create a curve or enter animation"));
        require(!partGroup()->isExpanded(), QStringLiteral("Solving reset the model tree's collapsed group"));

        tree->setCurrentItem(resultNode(ModelNode::Curves));
        QApplication::processEvents();
        auto* configuration = window.findChild<QWidget*>(QStringLiteral("motionCurveConfiguration"));
        require(configuration && configuration->isVisible(), QStringLiteral("Curve configuration is missing from the parameter column"));
        auto* source = configuration->findChild<QComboBox*>(QStringLiteral("motionCurveSource"));
        auto* variable = configuration->findChild<QComboBox*>(QStringLiteral("motionCurveVariable"));
        auto* addCurve = configuration->findChild<QPushButton*>(QStringLiteral("motionAddCurveButton"));
        require(source && variable && addCurve, QStringLiteral("Original curve request controls missing"));
        const bool native=source->findData("joints")>=0;
        source->setCurrentIndex(source->findData(native ? QVariant("joints") : QVariant(1))); // Relative joint angle.
        variable->setCurrentIndex(variable->findData(native ? QVariant("Coordinate:0") : QVariant(int(RigidCurveMetric::JointCoordinate)*4)));
        addCurve->click();
        require(waitFor([&] { return plot->isVisible() && plot->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 1; }, 4000),
            QStringLiteral("Explicit Add Curve did not create the requested series"));
        auto* curve = static_cast<QwtPlotCurve*>(plot->itemList(QwtPlotItem::Rtti_PlotCurve).front());
        require(curve->dataSize() > 10 && curve->sample(curve->dataSize() - 1).x() >= 0.99,
            QStringLiteral("Qwt is missing the physical time series"));
        require(std::abs(curve->sample(curve->dataSize() - 1).y() - 90.0) < 0.5,
            QStringLiteral("Joint angle curve disagrees with the solved 90 degree motion"));
        const auto physicalFrames = curve->dataSize();
        source->setCurrentIndex(source->findData(native ? QVariant("bodies") : QVariant(0)));
        variable->setCurrentIndex(variable->findData(native ? QVariant("Position:0") : QVariant(int(RigidCurveMetric::BodyPosition)*4)));
        addCurve->click();
        require(plot->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 2,
            QStringLiteral("Adding another channel replaced the existing curve"));
        auto* rigidPlot = window.findChild<RigidPlot*>();
        require(rigidPlot && rigidPlot->exportCurvesCsv(temp.filePath(QStringLiteral("selected-curves.csv")), error),
            QStringLiteral("Could not export selected result curves: %1").arg(error));
        double clickedTime = -1;
        QObject::connect(rigidPlot, &RigidPlot::timeRequested, &window,
            [&clickedTime](double time) { clickedTime = time; });
        const auto markers = plot->itemList(QwtPlotItem::Rtti_PlotMarker);
        require(markers.size() == 1 && !markers.front()->isVisible(), QStringLiteral("Animation marker is visible outside animation"));
        // ADS schedules splitter/layout changes when a previously closed dock
        // opens. Compare rendered poses only after the canvas has its real size.
        QApplication::processEvents();
        require(window.grab().save(output.filePath(QStringLiteral("curve-configuration.png"))), QStringLiteral("Could not capture curve configuration"));
        requireLegend(*plot);

        const auto initialCad = frame(*viewport);
        initialCad.save(output.filePath(QStringLiteral("before-animation.png")));
        tree->setCurrentItem(resultNode(ModelNode::Animation));
        QApplication::processEvents();
        auto* timeline = window.findChild<QSlider*>(QStringLiteral("motionTimeline"));
        require(timeline && timeline->isEnabled() && timeline->maximum() == int(physicalFrames) - 1,
            QStringLiteral("Playback slider does not index real result frames"));
        require(markers.front()->isVisible(), QStringLiteral("Animation did not activate the curve time marker"));
        const auto rest = frame(*viewport);
        timeline->setValue(timeline->maximum());
        require(frame(*viewport) != rest, QStringLiteral("Playback did not animate the real CAD/mesh frame"));
        const auto* cursor = static_cast<QwtPlotMarker*>(markers.front());
        require(std::abs(cursor->xValue() - 1.0) < 1e-9, QStringLiteral("Curve marker is not at the physical playback time"));
        require(window.grab().save(output.filePath(QStringLiteral("motion-qwt.png"))), QStringLiteral("Could not save Qwt screenshot"));

        auto* follow = window.findChild<QCheckBox*>(QStringLiteral("motionFollowCurveMarker"));
        auto* jump = window.findChild<QCheckBox*>(QStringLiteral("motionClickCurveJump"));
        auto* play = window.findChild<QPushButton*>(QStringLiteral("motionPlayButton"));
        auto* pause = window.findChild<QPushButton*>(QStringLiteral("motionPauseButton"));
        auto* stop = window.findChild<QPushButton*>(QStringLiteral("motionStopButton"));
        require(follow && jump && play && pause && stop, QStringLiteral("Original animation controls missing"));
        follow->setChecked(false);
        require(!cursor->isVisible(), QStringLiteral("Disabling marker follow did not hide the marker"));
        follow->setChecked(true);
        const auto clickTime = [plot](double time) {
            auto* canvas = plot->canvas();
            const QPoint point(int(plot->canvasMap(QwtPlot::xBottom).transform(time)), canvas->height() / 2);
            mouse(*canvas, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
            mouse(*canvas, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
        };
        jump->setChecked(false);
        clickTime(0.5);
        require(timeline->value() == timeline->maximum(), QStringLiteral("Disabled curve jump still moved the animation"));
        jump->setChecked(true);
        clickTime(0.5);
        require(std::abs(timeline->value() - timeline->maximum() / 2) <= 2,
            QStringLiteral("Clicking the curve did not seek the matching animation frame: frame %1/%2, requested %3 s, slider enabled %4")
                .arg(timeline->value()).arg(timeline->maximum()).arg(clickedTime).arg(timeline->isEnabled()));
        play->click();
        require(waitFor([&] { return timeline->value() > timeline->maximum() / 2 + 2; }, 3000),
            QStringLiteral("Playback did not advance"));
        pause->click();
        require(play->isEnabled() && !pause->isEnabled(), QStringLiteral("Pause did not preserve a resumable playback state"));
        stop->click();
        require(timeline->value() == 0, QStringLiteral("Stop did not restore the first physical frame"));
        require(frame(*viewport) == rest, QStringLiteral("Playback did not restore frame zero"));
        timeline->setValue(timeline->maximum());
        auto* animationDock = panes.value(QStringLiteral("MotionAnimation"));
        require(animationDock, QStringLiteral("Animation parameter dock missing"));
        animationDock->closeDockWidget();
        const auto afterAnimation = frame(*viewport);
        afterAnimation.save(output.filePath(QStringLiteral("after-animation.png")));
        require(afterAnimation == initialCad && !timeline->isEnabled() && !cursor->isVisible(),
            QStringLiteral("Exiting animation did not restore initial CAD and deactivate curve linkage: before %1x%2, after %3x%4, same pixels %5, slider enabled %6, marker visible %7")
                .arg(initialCad.width()).arg(initialCad.height()).arg(afterAnimation.width()).arg(afterAnimation.height())
                .arg(afterAnimation == initialCad).arg(timeline->isEnabled()).arg(cursor->isVisible()));
        require(plot->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 2,
            QStringLiteral("Exiting animation discarded configured curves"));
        tree->setCurrentItem(resultNode(ModelNode::Curves));
        tree->setCurrentItem(resultNode(ModelNode::Animation));
        require(timeline->isEnabled(), QStringLiteral("Animation could not be reopened"));
        timeline->setValue(timeline->maximum());
        auto* parameters = panes.value(QStringLiteral("Info"));
        require(parameters && parameters->dockAreaWidget(), QStringLiteral("Parameter host missing"));
        parameters->dockAreaWidget()->setCurrentDockWidget(parameters);
        require(frame(*viewport) == initialCad && !timeline->isEnabled() && !cursor->isVisible(),
            QStringLiteral("Switching back to parameter tab left a stale animated model"));
        // Switching ADS tabs leaves the tree's current item on Animation.
        // Navigate away first, as a fresh selection must emit currentItemChanged.
        tree->setCurrentItem(resultNode(ModelNode::Curves));
        tree->setCurrentItem(resultNode(ModelNode::Animation));
        require(timeline->isEnabled(), QStringLiteral("Animation could not be reopened before export selection"));
        timeline->setValue(timeline->maximum());
        tree->setCurrentItem(resultNode(ModelNode::Export));
        require(frame(*viewport) == initialCad && !timeline->isEnabled() && !cursor->isVisible(),
            QStringLiteral("Selecting the export node left a stale animated model"));
        for (auto* widget : QApplication::topLevelWidgets())
            require(!qobject_cast<QFileDialog*>(widget) || !widget->isVisible(),
                QStringLiteral("Selecting the export node unexpectedly opened a save dialog"));
        line(QStringLiteral("MotionEngine solve, explicit multi-curve configuration/CSV, frame indexing, pause/stop, independent curve linkage and CAD restoration passed"));
        line(QStringLiteral("Desktop smoke test passed"));
        return 0;
    } catch (const std::exception& e) {
        line(QStringLiteral("FAIL: %1").arg(QString::fromUtf8(e.what())));
        return 1;
    }
}
