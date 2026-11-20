#include "RigidPlot.h"
#include "UiPalette.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPalette>
#include <QPen>
#include <QPointF>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>

#include <qwt_interval.h>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_zoomer.h>

#include <algorithm>
#include <cmath>

namespace MotionOcc {
namespace {

QwtInterval timeRange(const RigidResult& result)
{
    QwtInterval range;
    for (const auto& frame : result.frames) {
        if (std::isfinite(frame.seconds))
            range |= QwtInterval(frame.seconds, frame.seconds);
    }
    return range;
}

template<class State>
const State* findState(const std::vector<State>& states, int State::* member, int id, std::size_t index)
{
    if (index < states.size() && states[index].*member == id)
        return &states[index];
    const auto found = std::find_if(states.begin(), states.end(), [=](const State& state) {
        return state.*member == id;
    });
    return found == states.end() ? nullptr : &*found;
}

} // namespace

RigidPlot::RigidPlot(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    auto* controls = new QHBoxLayout;
    layout->addLayout(controls);

    m_channel = new QComboBox(this);
    m_channel->setPlaceholderText(tr("选择结果通道"));
    m_channel->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_channel->setMinimumContentsLength(12);
    m_axis = new QComboBox(this);
    m_axis->addItem(QStringLiteral("X"));
    m_axis->addItem(QStringLiteral("Y"));
    m_axis->addItem(QStringLiteral("Z"));
    m_axis->addItem(tr("模长"));
    m_axis->setToolTip(tr("仅适用于刚体矢量；关节结果为标量。"));

    auto* channelLabel = new QLabel(tr("通道"), this);
    channelLabel->setBuddy(m_channel);
    controls->addWidget(channelLabel);
    controls->addWidget(m_channel, 1);
    auto* axisLabel = new QLabel(tr("分量"), this);
    axisLabel->setBuddy(m_axis);
    controls->addWidget(axisLabel);
    controls->addWidget(m_axis);

    m_plot = new QwtPlot(this);
    // QwtPlot owns its attached items; the canvas owns both pickers.
    auto* canvas = static_cast<QwtPlotCanvas*>(m_plot->canvas());
    canvas->setFrameShape(QFrame::NoFrame);
    m_plot->setAxisTitle(QwtAxis::XBottom, tr("时间 (s)"));
    layout->addWidget(m_plot, 1);
    m_readout = new QLabel(this);
    m_readout->setWordWrap(true);
    layout->addWidget(m_readout);

    auto* grid = new QwtPlotGrid;
    grid->attach(m_plot);
    m_curve = new QwtPlotCurve;
    m_curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    m_curve->attach(m_plot);
    m_cursor = new QwtPlotMarker(tr("播放时间"));
    m_cursor->setLineStyle(QwtPlotMarker::VLine);
    m_cursor->setItemAttribute(QwtPlotItem::AutoScale, false);
    m_cursor->attach(m_plot);

    m_zoomer = new QwtPlotZoomer(canvas, false);
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::ShiftModifier);
    m_zoomer->setKeyPattern(QwtEventPattern::KeySelect1, Qt::Key_Return, Qt::ShiftModifier);
    m_zoomer->setTrackerMode(QwtPicker::AlwaysOff);
    auto* picker = new QwtPlotPicker(canvas);
    picker->setStateMachine(new QwtPickerClickPointMachine);
    picker->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    connect(picker, qOverload<const QPointF&>(&QwtPlotPicker::selected), this, [this](const QPointF& point) {
        if (!m_result || !m_curve->dataSize() || !std::isfinite(point.x()))
            return;
        const auto range = timeRange(*m_result);
        if (range.isValid())
            emit timeRequested(std::clamp(point.x(), range.minValue(), range.maxValue()));
    });

    // The workspace is created before the main window applies the saved theme.
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
        m_curve->setPen(UiPalette::primary(), 1.75);
        m_cursor->setLinePen(UiPalette::text(), 1.5, Qt::DashLine);
        m_zoomer->setRubberBandPen(QPen(UiPalette::primary(), 1, Qt::DashLine));
        m_plot->replot();
    });

    connect(m_channel, qOverload<int>(&QComboBox::currentIndexChanged), this, &RigidPlot::replotCurve);
    connect(m_axis, qOverload<int>(&QComboBox::currentIndexChanged), this, &RigidPlot::replotCurve);
    setResult(nullptr, nullptr);
}

void RigidPlot::setResult(const RigidInput* input, const RigidResult* result)
{
    m_input = input;
    m_result = result;
    const auto range = result ? timeRange(*result) : QwtInterval();
    m_time = range.isValid() ? range.minValue() : 0.0;
    resetChannels();
    replotCurve();
}

void RigidPlot::resetChannels()
{
    const QSignalBlocker channelBlocker(m_channel);
    const QSignalBlocker axisBlocker(m_axis);
    m_channels.clear();
    m_channel->clear();
    m_axis->setCurrentIndex(0);
    if (m_input && m_result && !m_result->frames.empty()) {
        const auto add = [this](Metric metric, std::size_t index, const QString& name, const QString& quantity) {
            m_channels.push_back({metric, index});
            m_channel->addItem(tr("%1 · %2").arg(name, quantity));
        };
        for (std::size_t i = 0; i < m_input->bodies.size(); ++i) {
            const auto& body = m_input->bodies[i].body;
            const auto name = body.name.isEmpty() ? tr("刚体 %1").arg(body.id) : body.name;
            add(Metric::BodyDisplacement, i, name, tr("质心位移"));
            add(Metric::BodyVelocity, i, name, tr("线速度"));
            add(Metric::BodyAngularVelocity, i, name, tr("角速度"));
        }
        for (std::size_t i = 0; i < m_input->joints.size(); ++i) {
            const auto& joint = m_input->joints[i];
            if (joint.kind == JointKind::Fixed)
                continue; // A fixed joint has no position or speed coordinate.
            const auto name = joint.name.isEmpty() ? tr("关节 %1").arg(joint.id) : joint.name;
            add(Metric::JointPosition, i, name, tr("关节位置"));
            add(Metric::JointSpeed, i, name, tr("关节速度"));
        }
    }
    m_channel->setCurrentIndex(m_channels.empty() ? -1 : 0);
    m_channel->setEnabled(!m_channels.empty());
}

void RigidPlot::replotCurve()
{
    QVector<QPointF> samples;
    QString axisTitle;
    QString emptyText = tr("运行刚体仿真后，此处将显示结果曲线。");
    bool vectorChannel = false;
    const int selected = m_channel->currentIndex();
    if (selected >= 0 && static_cast<std::size_t>(selected) < m_channels.size()) {
        const auto [metric, index] = m_channels[selected];
        const auto& frames = m_result->frames;
        const int axis = m_axis->currentIndex();
        vectorChannel = metric != Metric::JointPosition && metric != Metric::JointSpeed;
        if (vectorChannel) {
            const auto id = m_input->bodies[index].body.id;
            const auto* initial = metric == Metric::BodyDisplacement
                ? findState(frames.front().bodies, &BodyState::bodyId, id, index) : nullptr;
            QString quantity;
            QString unit;
            switch (metric) {
            case Metric::BodyDisplacement:
                quantity = tr("质心位移（相对初始帧）");
                unit = QStringLiteral("mm");
                break;
            case Metric::BodyVelocity:
                quantity = tr("线速度");
                unit = QStringLiteral("mm/s");
                break;
            case Metric::BodyAngularVelocity:
                quantity = tr("角速度");
                unit = QStringLiteral("°/s");
                break;
            default:
                break;
            }
            axisTitle = tr("%1 %2 (%3)").arg(quantity, m_axis->currentText(), unit);
            for (const auto& frame : frames) {
                if (!std::isfinite(frame.seconds))
                    continue;
                const auto* state = findState(frame.bodies, &BodyState::bodyId, id, index);
                if (!state)
                    continue;
                Vector3d value;
                if (metric == Metric::BodyDisplacement) {
                    if (!initial)
                        break;
                    for (std::size_t i = 0; i < value.size(); ++i)
                        value[i] = state->positionMm[i] - initial->positionMm[i];
                } else {
                    value = metric == Metric::BodyVelocity ? state->velocityMmS : state->angularVelocityDegS;
                }
                const double y = axis >= 0 && axis < 3 ? value[axis] : std::hypot(value[0], value[1], value[2]);
                if (std::isfinite(y))
                    samples.append(QPointF(frame.seconds, y));
            }
        } else {
            const auto& joint = m_input->joints[index];
            const bool position = metric == Metric::JointPosition;
            const bool linear = joint.kind == JointKind::Prismatic;
            const QString unit = position ? (linear ? QStringLiteral("mm") : QStringLiteral("°"))
                                          : (linear ? QStringLiteral("mm/s") : QStringLiteral("°/s"));
            axisTitle = tr("%1 (%2)").arg(position ? tr("关节位置") : tr("关节速度"), unit);
            for (const auto& frame : frames) {
                if (!std::isfinite(frame.seconds))
                    continue;
                const auto* state = findState(frame.joints, &JointState::jointId, joint.id, index);
                if (!state)
                    continue;
                const double y = position ? state->position : state->speed;
                if (std::isfinite(y))
                    samples.append(QPointF(frame.seconds, y));
            }
        }
        emptyText = tr("所选通道没有有效的仿真采样。");
    } else if (m_result && !m_result->frames.empty()) {
        emptyText = tr("当前仿真结果没有可绘制的刚体或关节通道。");
    }

    const bool hasSamples = !samples.isEmpty();
    m_axis->setEnabled(vectorChannel);
    m_curve->setSamples(samples);
    m_curve->setStyle(samples.size() == 1 ? QwtPlotCurve::Dots : QwtPlotCurve::Lines);
    m_plot->setAxisTitle(QwtAxis::YLeft, axisTitle);
    m_cursor->setVisible(hasSamples);
    m_zoomer->setEnabled(hasSamples);
    m_plot->setVisible(hasSamples);
    m_readout->setAlignment(hasSamples ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignCenter);
    m_readout->setSizePolicy(QSizePolicy::Preferred, hasSamples ? QSizePolicy::Preferred : QSizePolicy::Expanding);
    if (!hasSamples) {
        m_readout->setText(emptyText);
        return;
    }

    const auto range = timeRange(*m_result);
    const double pad = range.width() == 0.0 ? 0.5 : 0.0;
    m_plot->setAxisScale(QwtAxis::XBottom, range.minValue() - pad, range.maxValue() + pad);
    m_plot->setAxisAutoScale(QwtAxis::YLeft);
    setTime(m_time);
    m_zoomer->setZoomBase(); // Apply the new scales before resetting the zoom stack.
}

void RigidPlot::setTime(double seconds)
{
    if (!std::isfinite(seconds))
        return;
    m_time = seconds;
    if (!m_curve->dataSize())
        return;
    m_cursor->setXValue(seconds);
    m_readout->setText(tr("时间：%1 s   ·   单击定位，Shift + 拖动缩放，右键还原")
        .arg(locale().toString(seconds, 'g', 8)));
    // Playback only moves the marker; leave the axes and zoom stack untouched.
    static_cast<QwtPlotCanvas*>(m_plot->canvas())->replot();
}

} // namespace MotionOcc
