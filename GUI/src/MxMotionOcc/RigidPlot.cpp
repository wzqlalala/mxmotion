#include "RigidPlot.h"
#include "UiPalette.h"
#include "mFoldSection.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QPalette>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPointer>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include <qwt_legend.h>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_zoomer.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace MotionOcc {
namespace {

class CurveLegend final : public QwtLegend {
protected:
    void updateWidget(QWidget* widget, const QwtLegendData& data) override
    {
        const bool explicitlyDisabled = widget->testAttribute(Qt::WA_ForceUpdatesDisabled);
        QwtLegend::updateWidget(widget, data);
        // Qwt 6.1's label setData() mistakes an ancestor's temporary update
        // suspension (as used by ADS for closed docks) for an explicit label
        // disable. Preserve the latter only, so reopening the dock paints its
        // saved curves' legends without needing to recreate the curves.
        if (!explicitlyDisabled) widget->setUpdatesEnabled(true);
    }
};

// Keep the original list's full name available without forcing the shared
// parameter column to widen for long object/quantity names.
class CurveNameLabel final : public QLabel {
public:
    explicit CurveNameLabel(const QString& text, QWidget* parent) : QLabel(text, parent)
    {
        setObjectName(QStringLiteral("motionCurveRowTitle"));
        setToolTip(text);
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        setMinimumWidth(0);
    }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setPen(palette().color(foregroundRole()));
        const QString displayed = fontMetrics().elidedText(text(), Qt::ElideRight, contentsRect().width());
        painter.drawText(contentsRect(), alignment() | Qt::TextSingleLine, displayed);
    }
};

QColor curveColor(std::size_t index)
{
    static const char* colors[] = {"#2176c1", "#d35f39", "#24946b", "#9864ba", "#b18a13", "#cf547a", "#298a9a", "#6e7790"};
    return QColor(QString::fromLatin1(colors[index % (sizeof(colors) / sizeof(colors[0]))]));
}

QByteArray csvLabel(QString text)
{
    const auto trimmed = text.trimmed();
    if (!trimmed.isEmpty() && QStringLiteral("=+-@＝＋－＠").contains(trimmed.front())) text.prepend(QLatin1Char('\''));
    auto bytes = text.toUtf8();
    return '"' + bytes.replace("\"", "\"\"") + '"';
}

} // namespace

RigidPlot::RigidPlot(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("motionCurvePlot"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    m_plot = new QwtPlot(this);
    m_plot->setObjectName(QStringLiteral("motionQwtPlot"));
    // QwtPlot owns its attached items and legend; the canvas owns the pickers.
    auto* canvas = static_cast<QwtPlotCanvas*>(m_plot->canvas());
    canvas->setFrameShape(QFrame::NoFrame);
    m_plot->setAxisTitle(QwtPlot::xBottom, tr("时间 (s)"));
    m_plot->insertLegend(new CurveLegend, QwtPlot::BottomLegend);
    layout->addWidget(m_plot, 1);
    m_readout = new QLabel(this);
    m_readout->setObjectName(QStringLiteral("motionCurveReadout"));
    m_readout->setWordWrap(true);
    layout->addWidget(m_readout);
    auto* grid = new QwtPlotGrid;
    grid->attach(m_plot);
    m_cursor = new QwtPlotMarker(tr("播放时间"));
    m_cursor->setLineStyle(QwtPlotMarker::VLine);
    m_cursor->setItemAttribute(QwtPlotItem::AutoScale, false);
    m_cursor->attach(m_plot);
    m_zoomer = new QwtPlotZoomer(canvas, false);
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::ShiftModifier);
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton);
    m_zoomer->setKeyPattern(QwtEventPattern::KeySelect1, Qt::Key_Return, Qt::ShiftModifier);
    m_zoomer->setTrackerMode(QwtPicker::AlwaysOff);
    auto* picker = new QwtPlotPicker(canvas);
    picker->setStateMachine(new QwtPickerClickPointMachine);
    picker->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    connect(picker, qOverload<const QPointF&>(&QwtPlotPicker::selected), this, [this](const QPointF& point) {
        if (m_result && hasVisibleCurves() && std::isfinite(point.x()))
            emit timeRequested(std::clamp(point.x(), m_start, m_end));
    });
    QTimer::singleShot(0, this, [this, grid] {
        using MxFunctions::UiPalette;
        QPalette colors = palette();
        colors.setColor(QPalette::Window, UiPalette::surface());
        colors.setColor(QPalette::WindowText, UiPalette::text());
        colors.setColor(QPalette::Text, UiPalette::text());
        setPalette(colors);
        setAutoFillBackground(true);
        m_plot->setCanvasBackground(UiPalette::surface());
        grid->setMajorPen(UiPalette::border());
        m_cursor->setLinePen(UiPalette::text(), 1.5, Qt::DashLine);
        m_zoomer->setRubberBandPen(QPen(UiPalette::primary(), 1, Qt::DashLine));
        m_plot->replot();
    });
    replotCurves();
}

void RigidPlot::setResult(const RigidInput* input, const RigidResult* result)
{
    if (input == m_input && result == m_result) return;
    ++m_resultGeneration;
    m_input = input;
    m_result = result;
    m_markerVisible = false;
    m_requests = input && result ? input->scenario.curveRequests : std::vector<RigidCurveRequest>{};
    m_start = result && !result->frames.empty() ? result->frames.front().seconds : 0.0;
    m_end = result && !result->frames.empty() ? result->frames.back().seconds : 0.0;
    m_time = m_start;
    replotCurves();
    emit resultChanged();
    emit configurationChanged();
}

std::vector<RigidCurveRequest> RigidPlot::curveRequests(int scenarioId) const
{
    return m_input && m_input->scenario.id == scenarioId ? m_requests : std::vector<RigidCurveRequest>{};
}

bool RigidPlot::hasVisibleCurves() const { return !m_visible.empty(); }

QString RigidPlot::displayTitle(const RigidCurveRequest& request) const
{
    if (!m_input) return {};
    QString title = rigidCurveTitle(*m_input, request);
    JointKind kind = JointKind::Revolute;
    if (!isBodyCurve(request.metric)) {
        const auto it = std::find_if(m_input->joints.begin(), m_input->joints.end(),
            [&](const RigidJoint& joint) { return joint.id == request.sourceId; });
        if (it != m_input->joints.end()) kind = it->kind;
    }
    const auto unit = rigidCurveUnit(request.metric, kind);
    if (!unit.isEmpty()) title += QStringLiteral(" (%1)").arg(unit);
    return title;
}

QString RigidPlot::uniqueCurveTitle(const QString& base) const
{
    auto used = [&](const QString& title) {
        return std::any_of(m_requests.begin(), m_requests.end(), [&](const RigidCurveRequest& request) {
            return rigidCurveTitle(*m_input, request) == title;
        });
    };
    QString title = base;
    for (int suffix = 2; used(title); ++suffix) title = QStringLiteral("%1 (%2)").arg(base).arg(suffix);
    return title;
}

void RigidPlot::addCurve(RigidCurveMetric metric, int sourceId, int component, const QString& name)
{
    if (!m_input || !m_result) return;
    if (m_requests.size() >= 256) { showMessage(tr("每个方案最多配置 256 条曲线。")); return; }
    RigidCurveRequest request;
    request.metric = metric;
    request.sourceId = sourceId;
    request.component = component;
    const auto series = rigidCurveSeries(*m_input, *m_result, request);
    if (!series.isValid()) { showMessage(series.error); return; }
    request.name = uniqueCurveTitle(name.trimmed().isEmpty() ? rigidCurveTitle(*m_input, request) : name.trimmed());
    request.color = curveColor(m_requests.size()).name();
    // Reuse a free scenario-local ID instead of overflowing after edits/imports.
    request.id = 1;
    while (std::any_of(m_requests.begin(), m_requests.end(), [&](const RigidCurveRequest& other) { return other.id == request.id; }))
        ++request.id;
    m_requests.push_back(std::move(request));
    requestsEdited();
}

void RigidPlot::removeCurve(int requestId)
{
    m_requests.erase(std::remove_if(m_requests.begin(), m_requests.end(),
        [requestId](const RigidCurveRequest& request) { return request.id == requestId; }), m_requests.end());
    requestsEdited();
}

void RigidPlot::setCurveVisible(int requestId, bool visible)
{
    for (auto& request : m_requests) if (request.id == requestId) request.visible = visible;
    requestsEdited();
}

void RigidPlot::setCurveColor(int requestId, const QColor& color)
{
    if (!color.isValid()) return;
    for (auto& request : m_requests) if (request.id == requestId) request.color = color.name();
    requestsEdited();
}

void RigidPlot::requestsEdited()
{
    replotCurves();
    if (m_input) emit curveRequestsChanged(m_input->scenario.id);
    emit configurationChanged();
    if (hasVisibleCurves()) emit curvesApplied();
}

void RigidPlot::setTimeWindow(double start, double end)
{
    if (!m_result || m_result->frames.empty()) return;
    if (!std::isfinite(start) || !std::isfinite(end) || end < start) {
        emit configurationChanged();
        showMessage(tr("结束时间不能早于起始时间。"));
        return;
    }
    m_start = std::clamp(start, m_result->frames.front().seconds, m_result->frames.back().seconds);
    m_end = std::clamp(end, m_start, m_result->frames.back().seconds);
    replotCurves();
    emit configurationChanged();
    if (hasVisibleCurves()) emit curvesApplied();
}

QWidget* RigidPlot::createConfigurationPanel(QWidget* parent)
{
    using MechMainWindow::FoldSection;
    auto* panel = new QWidget(parent);
    panel->setObjectName(QStringLiteral("motionCurveConfiguration"));
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    auto* config = new FoldSection(tr("曲线配置"), panel);
    auto* source = new QComboBox(config);
    source->setObjectName(QStringLiteral("motionCurveSource"));
    source->addItem(tr("运动体"), 0);
    source->addItem(tr("运动副"), 1);
    auto* object = new QComboBox(config);
    object->setObjectName(QStringLiteral("motionCurveObject"));
    auto* variable = new QComboBox(config);
    variable->setObjectName(QStringLiteral("motionCurveVariable"));
    variable->setWhatsThis(tr("位置为世界坐标；位移增量相对初始帧。欧拉角沿用原界面约定并展开 ±180° 跳变；运动副相对转角为连续关节坐标。"));
    for (auto* combo : {source, object, variable}) {
        combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        combo->setMinimumContentsLength(6);
        combo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        combo->setMinimumWidth(0);
        combo->setToolTip(combo->currentText());
        connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), combo, [combo](int) {
            combo->setToolTip(combo->currentText() + (combo->whatsThis().isEmpty()
                ? QString() : QLatin1Char('\n') + combo->whatsThis()));
        });
    }
    auto* name = new QLineEdit(config);
    name->setObjectName(QStringLiteral("motionCurveName"));
    name->setMaxLength(200);
    name->setPlaceholderText(tr("留空使用默认名称"));
    config->addRow(tr("源类型："), source);
    config->addRow(tr("对象："), object);
    config->addRow(tr("变量－分量："), variable);
    config->addRow(tr("曲线名称："), name);
    auto* add = new QPushButton(tr("添加曲线"), config);
    add->setObjectName(QStringLiteral("motionAddCurveButton"));
    auto* clear = new QPushButton(tr("清空"), config);
    clear->setObjectName(QStringLiteral("motionClearCurvesButton"));
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(clear);
    buttons->addWidget(add, 1);
    config->addRow(buttons);
    layout->addWidget(config);
    auto* listGroup = new FoldSection(tr("曲线列表"), panel);
    auto* list = new QListWidget(listGroup);
    list->setObjectName(QStringLiteral("motionCurveList"));
    list->setSelectionMode(QAbstractItemView::NoSelection);
    list->setResizeMode(QListView::Adjust);
    list->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setMinimumWidth(0);
    list->setMinimumHeight(100);
    list->setMaximumHeight(220);
    listGroup->addRow(list);
    layout->addWidget(listGroup);
    auto* display = new FoldSection(tr("显示设置"), panel);
    auto* start = new QDoubleSpinBox(display);
    auto* end = new QDoubleSpinBox(display);
    start->setObjectName(QStringLiteral("motionCurveStart"));
    end->setObjectName(QStringLiteral("motionCurveEnd"));
    for (auto* field : {start, end}) {
        field->setDecimals(9);
        field->setSuffix(QStringLiteral(" s"));
        field->setRange(-1e12, 1e12);
        field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        field->setMinimumWidth(0);
    }
    display->addRow(tr("起始时间："), start);
    display->addRow(tr("结束时间："), end);
    layout->addWidget(display);
    auto* exportGroup = new FoldSection(tr("导出"), panel);
    auto* interval = new QSpinBox(exportGroup);
    interval->setObjectName(QStringLiteral("motionCurveExportInterval"));
    interval->setRange(0, 999999);
    interval->setToolTip(tr("0 表示全部导出；N 表示每隔 N 个点取一个，保留末点。"));
    auto* exportButton = new QPushButton(tr("导出数据"), exportGroup);
    exportButton->setObjectName(QStringLiteral("motionExportCurvesButton"));
    exportGroup->addRow(tr("间隔点："), interval);
    exportGroup->addRow(exportButton);
    layout->addWidget(exportGroup);
    auto* feedback = new QLabel(panel);
    feedback->setObjectName(QStringLiteral("motionCurveFeedback"));
    feedback->setWordWrap(true);
    feedback->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    layout->addWidget(feedback);
    layout->addStretch();

    auto populateVariables = [this, source, object, variable, add] {
        const QSignalBlocker block(variable);
        const int old = variable->currentData().toInt();
        variable->clear();
        if (m_input && source->currentData().toInt() == 0) {
            for (const auto metric : {RigidCurveMetric::BodyPosition, RigidCurveMetric::BodyVelocity,
                 RigidCurveMetric::BodyRotation, RigidCurveMetric::BodyAngularVelocity, RigidCurveMetric::BodyDisplacement}) {
                const int components = metric == RigidCurveMetric::BodyRotation ? 3 : 4;
                for (int component = 0; component < components; ++component) {
                    const QString axis = component < 3 ? QString(QChar('X' + component)) : tr("幅值");
                    variable->addItem(QStringLiteral("%1 %2 (%3)").arg(rigidCurveQuantity(metric), axis, rigidCurveUnit(metric)),
                                      int(metric) * 4 + component);
                }
            }
        } else if (m_input && object->currentIndex() >= 0) {
            const auto id = object->currentData().toInt();
            const auto it = std::find_if(m_input->joints.begin(), m_input->joints.end(), [id](const RigidJoint& joint) { return joint.id == id; });
            if (it != m_input->joints.end()) {
                for (const auto metric : {RigidCurveMetric::JointCoordinate, RigidCurveMetric::JointRate})
                    variable->addItem(QStringLiteral("%1 (%2)").arg(rigidCurveQuantity(metric, it->kind), rigidCurveUnit(metric, it->kind)), int(metric) * 4);
            }
        }
        const int restored = variable->findData(old);
        if (restored >= 0) variable->setCurrentIndex(restored);
        variable->setToolTip(variable->currentText() + QLatin1Char('\n')
            + tr("位置为世界坐标；位移增量相对初始帧。欧拉角沿用原界面约定并展开 ±180° 跳变；运动副相对转角为连续关节坐标。"));
        add->setEnabled(object->currentIndex() >= 0 && variable->count() > 0 && m_result && !m_result->frames.empty());
    };
    auto populateObjects = [this, source, object, populateVariables] {
        const QSignalBlocker block(object);
        const int old = object->currentData().toInt();
        object->clear();
        if (m_input && source->currentData().toInt() == 0) {
            for (const auto& body : m_input->bodies)
                object->addItem(QStringLiteral("%1 [B%2]").arg(body.body.name).arg(body.body.id), body.body.id);
        } else if (m_input) {
            for (const auto& joint : m_input->joints)
                if (joint.kind != JointKind::Fixed)
                    object->addItem(QStringLiteral("%1 [J%2]").arg(joint.name).arg(joint.id), joint.id);
        }
        const int restored = object->findData(old);
        if (restored >= 0) object->setCurrentIndex(restored);
        object->setToolTip(object->currentText());
        populateVariables();
    };
    auto refresh = [this, panel, list, start, end, clear, exportButton, feedback] {
        list->clear();
        for (std::size_t index = 0; index < m_requests.size(); ++index) {
            const auto& request = m_requests[index];
            auto* item = new QListWidgetItem(list);
            item->setData(Qt::UserRole, request.id);
            auto* row = new QWidget(list);
            auto* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(2, 2, 2, 2);
            auto* color = new QPushButton(row);
            const QColor chosen = QColor(request.color).isValid() ? QColor(request.color) : curveColor(index);
            color->setFixedSize(18, 18);
            color->setStyleSheet(QStringLiteral("background-color: %1; border: 1px solid #888;").arg(chosen.name()));
            color->setToolTip(tr("选择曲线颜色"));
            auto* title = new CurveNameLabel(displayTitle(request), row);
            auto* visible = new QCheckBox(tr("显示"), row);
            visible->setChecked(request.visible);
            auto* remove = new QPushButton(tr("删除"), row);
            remove->setFlat(true);
            remove->setFixedWidth(remove->fontMetrics().horizontalAdvance(remove->text()) + 12);
            rowLayout->addWidget(color);
            rowLayout->addWidget(title, 1);
            rowLayout->addWidget(visible);
            rowLayout->addWidget(remove);
            // QListView keeps the widest item size even with its horizontal
            // scrollbar disabled. Only prescribe height: a long curve title
            // must elide inside the viewport, without pushing actions offscreen.
            item->setSizeHint(QSize(0, row->sizeHint().height()));
            list->setItemWidget(item, row);
            const int requestId = request.id;
            const auto generation = m_resultGeneration;
            // Queued updates avoid deleting a row widget in its own click handler.
            connect(visible, &QCheckBox::toggled, panel, [this, requestId, generation](bool checked) {
                if (m_resultGeneration == generation) setCurveVisible(requestId, checked);
            }, Qt::QueuedConnection);
            connect(remove, &QPushButton::clicked, panel, [this, requestId, generation] {
                if (m_resultGeneration == generation) removeCurve(requestId);
            }, Qt::QueuedConnection);
            connect(color, &QPushButton::clicked, panel, [this, panel, requestId, chosen, generation] {
                if (m_resultGeneration != generation) return;
                const auto color = QColorDialog::getColor(chosen, panel, tr("选择曲线颜色"));
                if (m_resultGeneration == generation && color.isValid()) setCurveColor(requestId, color);
            }, Qt::QueuedConnection);
        }
        const QSignalBlocker bs(start), be(end);
        start->setValue(m_start);
        end->setValue(m_end);
        clear->setEnabled(!m_requests.empty());
        exportButton->setEnabled(hasVisibleCurves());
        feedback->setText(m_input && m_result ? tr("方案：%1。选择变量后点击“添加曲线”，可叠加比较；只有列表中的可见曲线参与导出。").arg(m_input->scenario.name)
                                             : tr("当前方案没有可用结果，请先求解。"));
    };
    connect(source, qOverload<int>(&QComboBox::currentIndexChanged), panel, [populateObjects](int) { populateObjects(); });
    connect(object, qOverload<int>(&QComboBox::currentIndexChanged), panel, [populateVariables](int) { populateVariables(); });
    connect(this, &RigidPlot::resultChanged, panel, populateObjects);
    connect(this, &RigidPlot::configurationChanged, panel, refresh);
    connect(this, &RigidPlot::messageChanged, feedback, &QLabel::setText);
    connect(add, &QPushButton::clicked, panel, [this, object, variable, name] {
        if (object->currentIndex() < 0 || variable->currentIndex() < 0) return;
        const int value = variable->currentData().toInt();
        addCurve(static_cast<RigidCurveMetric>(value / 4), object->currentData().toInt(), value % 4, name->text());
    });
    connect(clear, &QPushButton::clicked, panel, [this] { m_requests.clear(); requestsEdited(); });
    auto changeWindow = [this, start, end] { setTimeWindow(start->value(), end->value()); };
    connect(start, &QDoubleSpinBox::editingFinished, panel, changeWindow);
    connect(end, &QDoubleSpinBox::editingFinished, panel, changeWindow);
    connect(exportButton, &QPushButton::clicked, panel, [this, panel, interval, feedback] {
        QString path = QFileDialog::getSaveFileName(panel, tr("导出可见曲线"), QStringLiteral("motion-curves.csv"), tr("CSV 表格 (*.csv)"));
        if (path.isEmpty()) return;
        if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".csv");
        QString error;
        feedback->setText(exportCurvesCsv(path, error, interval->value()) ? tr("已导出：%1").arg(QFileInfo(path).fileName()) : error);
    });
    populateObjects();
    refresh();
    return panel;
}

void RigidPlot::showMessage(const QString& message)
{
    m_readout->setText(message);
    emit messageChanged(message);
}

void RigidPlot::replotCurves()
{
    const auto oldCurves = m_plot->itemList(QwtPlotItem::Rtti_PlotCurve);
    for (auto* curve : oldCurves) { curve->detach(); delete curve; }
    m_visible.clear();
    QString error;
    if (m_input && m_result) {
        for (std::size_t index = 0; index < m_requests.size(); ++index) {
            const auto& request = m_requests[index];
            if (!request.visible) continue;
            auto series = rigidCurveSeries(*m_input, *m_result, request);
            if (!series.isValid()) { error = series.error; continue; }
            const auto lo = std::lower_bound(series.times.begin(), series.times.end(), m_start) - series.times.begin();
            const auto hi = std::upper_bound(series.times.begin(), series.times.end(), m_end) - series.times.begin();
            if (lo >= hi) continue;
            series.times = series.times.mid(int(lo), int(hi - lo));
            series.values = series.values.mid(int(lo), int(hi - lo));
            VisibleCurve item{rigidCurveTitle(*m_input, request), QColor(request.color), std::move(series)};
            if (!item.color.isValid()) item.color = curveColor(index);
            auto* curve = new QwtPlotCurve(QStringLiteral("%1 (%2)").arg(item.title, item.series.unit));
            curve->setRenderHint(QwtPlotItem::RenderAntialiased);
            curve->setPen(item.color, 1.75);
            curve->setSamples(item.series.times, item.series.values);
            curve->setStyle(item.series.times.size() == 1 ? QwtPlotCurve::Dots : QwtPlotCurve::Lines);
            curve->attach(m_plot);
            m_visible.push_back(std::move(item));
        }
    }
    const bool hasSamples = hasVisibleCurves();
    m_cursor->setVisible(hasSamples && m_markerVisible);
    m_zoomer->setEnabled(hasSamples);
    m_plot->setVisible(hasSamples);
    m_readout->setAlignment(hasSamples ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignCenter);
    m_readout->setSizePolicy(QSizePolicy::Preferred, hasSamples ? QSizePolicy::Preferred : QSizePolicy::Expanding);
    if (!hasSamples) {
        showMessage(!error.isEmpty() ? error : (!m_result ? tr("求解后，在“运动曲线”参数面板配置曲线。")
            : m_requests.empty() ? tr("曲线列表为空。选择源类型、对象和变量，然后点击“添加曲线”。")
                                 : tr("当前时间范围没有可见曲线采样。")));
        m_plot->replot();
        return;
    }
    QString yTitle;
    if (m_visible.size() == 1) yTitle = QStringLiteral("%1 (%2)").arg(m_visible.front().title, m_visible.front().series.unit);
    else {
        const QString unit = m_visible.front().series.unit;
        const bool sameUnit = std::all_of(m_visible.begin(), m_visible.end(), [&](const VisibleCurve& item) { return item.series.unit == unit; });
        yTitle = sameUnit ? tr("数值 (%1)").arg(unit) : tr("数值（单位见图例）");
    }
    m_plot->setAxisTitle(QwtPlot::yLeft, yTitle);
    const double pad = m_end == m_start ? 0.5 : 0.0;
    m_plot->setAxisScale(QwtPlot::xBottom, m_start - pad, m_end + pad);
    m_plot->setAxisAutoScale(QwtPlot::yLeft);
    m_cursor->setXValue(m_time);
    m_plot->replot();
    m_zoomer->setZoomBase();
    setTime(m_time);
    if (!error.isEmpty()) showMessage(tr("部分曲线没有有效数据：%1").arg(error));
}

void RigidPlot::setTime(double seconds)
{
    if (!std::isfinite(seconds)) return;
    m_time = seconds;
    if (!hasVisibleCurves()) return;
    m_cursor->setXValue(seconds);
    m_readout->setText(m_markerVisible
        ? tr("时间：%1 s · %2 条曲线 · Shift + 拖动缩放，右键还原").arg(locale().toString(seconds, 'g', 8)).arg(m_visible.size())
        : tr("%1 条曲线 · Shift + 拖动缩放，右键还原").arg(m_visible.size()));
    // Animation only moves the cursor, preserving plot zoom and scales.
    static_cast<QwtPlotCanvas*>(m_plot->canvas())->replot();
}

void RigidPlot::setTimeMarkerVisible(bool visible)
{
    m_markerVisible = visible;
    m_cursor->setVisible(visible && hasVisibleCurves());
    setTime(m_time);
}

bool RigidPlot::exportCurvesCsv(const QString& path, QString& error, int interval) const
{
    error.clear();
    if (m_visible.empty()) { error = tr("没有可导出的可见曲线。"); return false; }
    if (interval < 0) { error = tr("导出间隔不能为负数。"); return false; }
    const auto& times = m_visible.front().series.times;
    // Every accepted curve has all physical frames. Filtering is common to all
    // curves, so never interpolate or reuse another frame's value on export.
    for (const auto& curve : m_visible) {
        if (curve.series.times != times || curve.series.values.size() != times.size()) {
            error = tr("曲线时间轴不一致，无法导出。"); return false;
        }
    }
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) { error = file.errorString(); return false; }
    QByteArray header = csvLabel(QStringLiteral("time_s"));
    for (const auto& curve : m_visible) header += ',' + csvLabel(QStringLiteral("%1 (%2)").arg(curve.title, curve.series.unit));
    header += "\r\n";
    bool ok = file.write(header) == header.size();
    const qint64 step = qint64(interval) + 1;
    int last = -1;
    const auto writeRow = [&](int index) {
        QByteArray row = QByteArray::number(times[index], 'g', std::numeric_limits<double>::max_digits10);
        for (const auto& curve : m_visible)
            row += ',' + QByteArray::number(curve.series.values[index], 'g', std::numeric_limits<double>::max_digits10);
        row += "\r\n";
        return file.write(row) == row.size();
    };
    for (qint64 index = 0; ok && index < times.size(); index += step) {
        last = int(index);
        ok = writeRow(last);
    }
    if (ok && last != times.size() - 1) ok = writeRow(times.size() - 1);
    if (!ok || !file.commit()) { error = file.errorString(); return false; }
    return true;
}

} // namespace MotionOcc
