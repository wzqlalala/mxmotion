#pragma once

#include "RigidMotion.h"

#include <QWidget>
#include <vector>

class QComboBox;
class QLabel;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotMarker;
class QwtPlotZoomer;

namespace MotionOcc {

class RigidPlot final : public QWidget {
    Q_OBJECT
public:
    explicit RigidPlot(QWidget* parent = nullptr);
    // The caller owns the data. Clear the plot before replacing it.
    void setResult(const RigidInput* input, const RigidResult* result);
    void setTime(double seconds);

signals:
    void timeRequested(double seconds);

private:
    void resetChannels();
    void replotCurve();

    enum class Metric { BodyDisplacement, BodyVelocity, BodyAngularVelocity, JointPosition, JointSpeed };
    struct Channel { Metric metric; std::size_t index; };
    std::vector<Channel> m_channels;
    const RigidInput* m_input = nullptr;
    const RigidResult* m_result = nullptr;
    QComboBox* m_channel = nullptr;
    QComboBox* m_axis = nullptr;
    QLabel* m_readout = nullptr;
    QwtPlot* m_plot = nullptr;
    QwtPlotCurve* m_curve = nullptr;
    QwtPlotMarker* m_cursor = nullptr;
    QwtPlotZoomer* m_zoomer = nullptr;
    double m_time = 0.0;
};

} // namespace MotionOcc
