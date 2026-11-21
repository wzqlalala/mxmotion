#include "RigidPlot.h"
#include "NativeMotionSolver.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEventLoop>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QDebug>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_map.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>

#include <cmath>
#include <memory>
#include <stdexcept>

namespace {
using namespace MotionOcc;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
template<class T>
T* widget(QWidget& parent, const char* name)
{
    auto* found = parent.findChild<T*>(QString::fromLatin1(name));
    require(found != nullptr, name);
    return found;
}

void requireLegend(QwtPlot& plot)
{
    QApplication::processEvents();
    auto* legend = qobject_cast<QwtLegend*>(plot.legend());
    require(legend && legend->isVisible() && !legend->visibleRegion().isEmpty(), "curve legend is missing or clipped");
    for (auto* curve : plot.itemList(QwtPlotItem::Rtti_PlotCurve)) {
        const auto entries = legend->legendWidgets(plot.itemToInfo(curve));
        require(entries.size() == 1, "a plotted curve has no legend entry");
        auto* label = qobject_cast<QwtLegendLabel*>(entries.front());
        require(label && label->isVisible() && !label->visibleRegion().isEmpty()
                && label->text().text() == curve->title().text(), "curve legend entry is hidden or lost its quantity/unit");
        require(label->updatesEnabled(), "curve legend paint remains disabled after its host resumes updates");
        const auto pixels = label->grab().toImage();
        const QColor expected = static_cast<QwtPlotCurve*>(curve)->pen().color();
        int colored = 0;
        for (int y = 0; y < pixels.height(); ++y)
            for (int x = 0; x < pixels.width(); ++x) {
                const QColor actual = pixels.pixelColor(x, y);
                if (actual.alpha() > 128 && std::abs(actual.red() - expected.red()) < 20
                    && std::abs(actual.green() - expected.green()) < 20 && std::abs(actual.blue() - expected.blue()) < 20) ++colored;
            }
        require(colored >= 3, "curve legend does not paint its colored series key");
    }
}
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    using namespace MotionOcc;
    try {
        RigidInput input;
        input.scenario.id = 7;
        input.scenario.name = QStringLiteral("Curve fixture");
        PreparedBody body;
        body.body.id = 42;
        body.body.name = QStringLiteral("Offset body");
        input.bodies.push_back(body);
        RigidResult result;
        result.scenarioId = 7;
        for (const double time : {0.0, 0.1, 0.4, 0.9, 1.0}) {
            RigidFrame frame;
            frame.seconds = time;
            BodyState state;
            state.bodyId = 42;
            state.positionMm = {50.0 + 10 * time, 100.0 - 20 * time, 0};
            frame.bodies.push_back(state);
            result.frames.push_back(frame);
        }
        {
            // Loading an existing project's requests into a closed result dock
            // must populate its legend before the first time that dock opens.
            auto savedInput = input;
            savedInput.scenario.curveRequests = {
                {1, RigidCurveMetric::BodyPosition, 42, 0, QStringLiteral("已有质心位置"), QStringLiteral("#2176c1"), true},
                {2, RigidCurveMetric::BodyDisplacement, 42, 1, QStringLiteral("已有位移增量"), QStringLiteral("#d35f39"), true},
                {3, RigidCurveMetric::BodyVelocity, 42, 2, QStringLiteral("隐藏速度"), {}, false}
            };
            QWidget host;
            RigidPlot storedPlot(&host);
            storedPlot.resize(1100, 350);
            host.resize(1100, 350);
            QApplication::processEvents();
            // ADS temporarily suspends updates while detaching a closed dock.
            // Qwt must not turn inherited suspension into an explicit per-label
            // disable that survives the dock being reopened.
            host.setUpdatesEnabled(false);
            storedPlot.setResult(&savedInput, &result);
            std::unique_ptr<QWidget> storedPanel(storedPlot.createConfigurationPanel());
            host.setUpdatesEnabled(true);
            host.show();
            storedPlot.show();
            auto* storedChart = storedPlot.findChild<QwtPlot*>();
            require(storedChart && storedChart->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 2,
                    "saved scenario requests did not create the two visible curves");
            requireLegend(*storedChart);
        }
        RigidPlot plot;
        plot.setResult(&input, &result);
        QObject::connect(&plot, &RigidPlot::curveRequestsChanged, &plot, [&](int scenarioId) {
            require(scenarioId == input.scenario.id, "editing a different scenario's request list");
            input.scenario.curveRequests = plot.curveRequests(scenarioId);
        });
        std::unique_ptr<QWidget> panel(plot.createConfigurationPanel());
        auto* chart = plot.findChild<QwtPlot*>();
        require(chart != nullptr, "Qwt plot missing");
        auto curves = [&] { return chart->itemList(QwtPlotItem::Rtti_PlotCurve); };
        require(curves().isEmpty() && !plot.hasVisibleCurves(), "result load manufactured an unrequested curve");
        auto* add = widget<QPushButton>(*panel, "motionAddCurveButton");
        auto* variable = widget<QComboBox>(*panel, "motionCurveVariable");
        auto* name = widget<QLineEdit>(*panel, "motionCurveName");
        name->setText(QStringLiteral("Offset, \"position\""));
        add->click();
        require(curves().size() == 1 && plot.hasVisibleCurves(), "Add Curve did not display requested series");
        auto* position = static_cast<QwtPlotCurve*>(curves().front());
        require(position->sample(0).y() == 50 && position->sample(2).x() == 0.4, "plot changed absolute position or physical time");
        variable->setCurrentIndex(variable->findData(int(RigidCurveMetric::BodyDisplacement) * 4 + 1));
        require(curves().size() == 1, "selecting a variable modified the existing request without Add");
        name->setText(QStringLiteral("Y displacement"));
        add->click();
        require(curves().size() == 2 && input.scenario.curveRequests.size() == 2, "adding a second curve replaced the first or failed persistence signal");
        auto* displacement = static_cast<QwtPlotCurve*>(curves().back());
        require(displacement->sample(0).y() == 0 && displacement->sample(4).y() == -20, "displacement curve used absolute position");
        const auto saved = input.scenario.curveRequests;
        plot.resize(900, 500);
        plot.show();
        QApplication::processEvents();
        const auto checkLegend = [&] { requireLegend(*chart); };
        checkLegend();
        double clickedTime = -1;
        int clickCount = 0;
        QObject::connect(&plot, &RigidPlot::timeRequested, &plot, [&](double seconds) { clickedTime = seconds; ++clickCount; });
        auto* canvas = chart->canvas();
        const QPoint clickPoint(qRound(chart->canvasMap(QwtPlot::xBottom).transform(0.4)), canvas->height() / 2);
        QMouseEvent press(QEvent::MouseButtonPress, clickPoint, canvas->mapToGlobal(clickPoint), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(canvas, &press);
        QMouseEvent release(QEvent::MouseButtonRelease, clickPoint, canvas->mapToGlobal(clickPoint), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(canvas, &release);
        require(clickCount == 1 && std::abs(clickedTime - 0.4) < 0.005, "canvas click did not request the physical curve time");
        {
            // The original shared parameter column is about 250 px wide. Long
            // variables and per-curve buttons must fit without sideways scroll.
            QScrollArea sidebar;
            sidebar.setFrameShape(QFrame::NoFrame);
            sidebar.setWidgetResizable(true);
            sidebar.resize(250, 600);
            auto* narrow = plot.createConfigurationPanel();
            sidebar.setWidget(narrow);
            sidebar.show();
            QEventLoop settle;
            QTimer::singleShot(20, &settle, &QEventLoop::quit);
            settle.exec();
            require(sidebar.horizontalScrollBar()->maximum() == 0 && narrow->width() <= sidebar.viewport()->width(),
                    "curve configuration forces horizontal scrolling in the original parameter column");
            for (const auto& field : {QStringLiteral("motionCurveSource"), QStringLiteral("motionCurveObject"), QStringLiteral("motionCurveVariable")}) {
                auto* combo = narrow->findChild<QComboBox*>(field);
                require(combo && combo->mapTo(narrow, combo->rect().topRight()).x() < narrow->width(), "curve selector is clipped in a narrow parameter column");
                require(combo->toolTip().contains(combo->currentText()), "full curve selector text is unavailable in its tooltip");
            }
            for (auto* title : narrow->findChildren<QLabel*>(QStringLiteral("motionCurveRowTitle")))
                require(!title->text().isEmpty() && title->toolTip() == title->text(), "elided curve name lost its full accessible text");
            auto* rows = widget<QListWidget>(*narrow, "motionCurveList");
            for (int index = 0; index < rows->count(); ++index) {
                auto* row = rows->itemWidget(rows->item(index));
                require(row && row->width() <= rows->viewport()->width(), "curve row is wider than its viewport");
                for (auto* button : row->findChildren<QPushButton*>())
                    require(button->mapTo(rows->viewport(), button->rect().topRight()).x() < rows->viewport()->width(),
                            "curve row action is clipped in the narrow parameter column");
            }
        }
        plot.setTimeMarkerVisible(true);
        auto* marker = static_cast<QwtPlotMarker*>(chart->itemList(QwtPlotItem::Rtti_PlotMarker).front());
        plot.setTime(0.4);
        require(marker->isVisible() && marker->xValue() == 0.4, "animation cursor is not synchronized");
        plot.setTimeMarkerVisible(false);
        require(!marker->isVisible(), "animation exit left an active curve cursor");
        checkLegend();
        auto* start = widget<QDoubleSpinBox>(*panel, "motionCurveStart");
        auto* end = widget<QDoubleSpinBox>(*panel, "motionCurveEnd");
        start->setValue(0.1);
        end->setValue(0.9);
        QMetaObject::invokeMethod(end, "editingFinished", Qt::DirectConnection);
        auto* filtered = static_cast<QwtPlotCurve*>(curves().front());
        require(filtered->dataSize() == 3 && filtered->sample(0).x() == 0.1 && filtered->sample(2).x() == 0.9, "time window did not use real sample times");
        QTemporaryDir temp;
        require(temp.isValid(), "CSV temp directory unavailable");
        QString error;
        const auto path = temp.filePath(QStringLiteral("curves.csv"));
        require(plot.exportCurvesCsv(path, error, 1), error.toUtf8().constData());
        QFile csv(path);
        require(csv.open(QIODevice::ReadOnly), "CSV not written");
        const auto text = csv.readAll();
        require(text.count('\n') == 3 && text.contains("\"Offset, \"\"position\"\" (mm)\""), "CSV sampling or quoted headers are wrong");
        require(text.contains("0.10000000000000001,51,-2") && text.contains("0.90000000000000002,59,-18"), "CSV data differs from displayed curves");
        auto* list = widget<QListWidget>(*panel, "motionCurveList");
        auto* visible = list->itemWidget(list->item(0))->findChild<QCheckBox*>();
        require(visible, "curve visibility control missing");
        visible->setChecked(false);
        QApplication::processEvents();
        require(curves().size() == 1 && !input.scenario.curveRequests.front().visible, "visibility change was not stored per scenario");
        checkLegend();
        RigidInput second = input;
        second.scenario.id = 8;
        second.scenario.curveRequests.clear();
        RigidResult secondResult = result;
        secondResult.scenarioId = 8;
        // A queued row action must stay with the scenario that created it even
        // if the active result changes before Qt dispatches that action.
        visible = list->itemWidget(list->item(0))->findChild<QCheckBox*>();
        visible->setChecked(true);
        plot.setResult(&second, &secondResult);
        QApplication::processEvents();
        require(curves().isEmpty(), "switching scenarios retained stale curves");
        input.scenario.curveRequests = saved;
        plot.setResult(&input, &result);
        require(curves().size() == 2, "returning to scenario lost its saved requests");
        checkLegend();
        widget<QPushButton>(*panel, "motionClearCurvesButton")->click();
        require(curves().isEmpty() && !plot.hasVisibleCurves() && input.scenario.curveRequests.empty(), "Clear retained stale curves or persisted requests");
        require(!plot.exportCurvesCsv(temp.filePath(QStringLiteral("empty.csv")), error), "empty plot exported stale curves");
        {
            RigidInput nativeInput;
            nativeInput.scenario.id = 0;
            nativeInput.scenario.name = QStringLiteral("完整模型结果");
            nativeInput.nativeInput = std::make_shared<NativeMotionInput>();
            nativeInput.nativeInput->document.root = {{"motionData", QJsonObject{
                {"drives", QJsonArray{QJsonObject{{"id",0},{"name",QStringLiteral("原始驱动零")}}}}}}};
            RigidResult native;
            native.scenarioId = 0;
            for (const double seconds : {0.0, 0.05, 0.12}) { RigidFrame frame; frame.seconds = seconds; native.frames.push_back(frame); }
            native.channels = {
                {"drives", 0, "ReactionForce", "N", 6, {1,2,3,4,5,6, 7,8,9,10,11,12, 13,14,15,16,17,18}},
                {"sensors", 0, "SensorValue", "mm", 3, {3,4,0, 0,0,7, 1,2,2}},
                {"controllers", 0, "PidError", {}, 1, {-2,0,2}}
            };
            RigidPlot nativePlot;
            nativePlot.setResult(&nativeInput, &native);
            QObject::connect(&nativePlot, &RigidPlot::curveRequestsChanged, &nativePlot, [&](int id) {
                require(id == 0, "native scenario ID zero did not round trip through requests");
                nativeInput.scenario.curveRequests = nativePlot.curveRequests(id);
            });
            std::unique_ptr<QWidget> nativePanel(nativePlot.createConfigurationPanel());
            auto* nativeSource = widget<QComboBox>(*nativePanel, "motionCurveSource");
            auto* nativeObject = widget<QComboBox>(*nativePanel, "motionCurveObject");
            auto* nativeVariable = widget<QComboBox>(*nativePanel, "motionCurveVariable");
            auto* nativeAdd = widget<QPushButton>(*nativePanel, "motionAddCurveButton");
            auto* nativeChart = nativePlot.findChild<QwtPlot*>();
            require(nativeSource->count() == 3 && nativeObject->currentData().toInt() == 0,
                    "native result sources were dropped or object ID zero was rejected");
            nativeSource->setCurrentIndex(nativeSource->findData(QStringLiteral("drives")));
            require(nativeObject->currentText().contains(QStringLiteral("原始驱动零")), "native object name was lost in the curve selector");
            require(nativeVariable->findData(QStringLiteral("ReactionForce:5")) >= 0,
                    "sixth result component is not available in the original curve panel");
            nativeVariable->setCurrentIndex(nativeVariable->findData(QStringLiteral("ReactionForce:5")));
            require(nativeChart->itemList(QwtPlotItem::Rtti_PlotCurve).isEmpty(), "native selection manufactured an unrequested curve");
            nativeAdd->click();
            require(nativeInput.scenario.curveRequests.size() == 1, "native Add did not persist request");
            const auto& request = nativeInput.scenario.curveRequests.front();
            require(request.metric == RigidCurveMetric::NativeChannel && request.sourceType == QStringLiteral("drives")
                && request.sourceId == 0 && request.variable == QStringLiteral("ReactionForce") && request.component == 5,
                "native request was saved with menu positions instead of stable keys");
            auto* sixth = static_cast<QwtPlotCurve*>(nativeChart->itemList(QwtPlotItem::Rtti_PlotCurve).front());
            require(sixth->sample(0).y() == 6 && sixth->sample(2).y() == 18 && sixth->sample(2).x() == 0.12,
                    "six-component native curve changed solver samples");
            nativeSource->setCurrentIndex(nativeSource->findData(QStringLiteral("sensors")));
            nativeVariable->setCurrentIndex(nativeVariable->findData(QStringLiteral("SensorValue:-1")));
            nativeAdd->click();
            require(nativeChart->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 2, "native sensor curve replaced the drive curve");
            auto* magnitude = static_cast<QwtPlotCurve*>(nativeChart->itemList(QwtPlotItem::Rtti_PlotCurve).back());
            require(magnitude->sample(0).y() == 5 && magnitude->sample(2).y() == 3, "native sensor magnitude is incorrect");
            nativeSource->setCurrentIndex(nativeSource->findData(QStringLiteral("controllers")));
            nativeAdd->click();
            require(nativeChart->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 3, "dimensionless PID scalar was hidden");
            auto* pid = static_cast<QwtPlotCurve*>(nativeChart->itemList(QwtPlotItem::Rtti_PlotCurve).back());
            require(pid->sample(0).y() == -2 && !pid->title().text().contains(QStringLiteral("()")), "PID value or unit label is incorrect");
            const auto nativePath = temp.filePath(QStringLiteral("native-curves.csv"));
            require(nativePlot.exportCurvesCsv(nativePath, error), error.toUtf8().constData());
            QFile nativeCsv(nativePath);
            require(nativeCsv.open(QIODevice::ReadOnly), "native curve CSV missing");
            const auto nativeText = nativeCsv.readAll();
            require(nativeText.contains("0,6,5,-2") && nativeText.contains("0.12,18,3,2"),
                    "native CSV differs from visible physical samples");
            nativePlot.setResult(nullptr, nullptr);
            nativePlot.setResult(&nativeInput, &native);
            require(nativeChart->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 3,
                    "native persisted requests were lost on result reopening");
        }
        qInfo() << "Qwt curve workflow: explicit add, native channels/components, request persistence, visibility, real-time filtering and CSV passed";
        return 0;
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return 1;
    }
}
