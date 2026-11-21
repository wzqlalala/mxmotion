#pragma once

#include "RigidMotion.h"
#include "RigidResultSeries.h"

#include <QColor>
#include <QWidget>
#include <vector>

class QLabel;
class QwtPlot;
class QwtPlotMarker;
class QwtPlotZoomer;

namespace MotionOcc {

class RigidPlot final : public QWidget {
    Q_OBJECT
public:
    explicit RigidPlot(QWidget* parent = nullptr);
    // The caller owns the data. Clear the plot before replacing it. Requests
    // belong to the scenario, not to a selected channel in the plotting widget.
    void setResult(const RigidInput* input, const RigidResult* result);
    QWidget* createConfigurationPanel(QWidget* parent = nullptr);
    std::vector<RigidCurveRequest> curveRequests(int scenarioId) const;
    bool hasVisibleCurves() const;
    void setTime(double seconds);
    void setTimeMarkerVisible(bool visible);
    bool exportCurvesCsv(const QString& path, QString& error, int interval = 0) const;
    bool exportCurvesTxt(const QString& path, QString& error, int interval = 0) const;

signals:
    void timeRequested(double seconds);
    void curvesApplied();
    void curveRequestsChanged(int scenarioId);
    void configurationChanged();
    void resultChanged();
    void messageChanged(const QString& message);

private:
    bool exportCurvesDelimited(const QString& path, QString& error, int interval, char delimiter) const;
    struct VisibleCurve {
        QString title;
        QColor color;
        RigidCurveSeries series;
    };
    void replotCurves();
    void requestsEdited();
    void addCurve(RigidCurveRequest request, const QString& name);
    void removeCurve(int requestId);
    void setCurveVisible(int requestId, bool visible);
    void setCurveColor(int requestId, const QColor& color);
    void setTimeWindow(double start, double end);
    QString uniqueCurveTitle(const QString& base) const;
    QString displayTitle(const RigidCurveRequest& request) const;
    void showMessage(const QString& message);

    std::vector<RigidCurveRequest> m_requests;
    std::vector<VisibleCurve> m_visible;
    const RigidInput* m_input = nullptr;
    const RigidResult* m_result = nullptr;
    QLabel* m_readout = nullptr;
    QwtPlot* m_plot = nullptr;
    QwtPlotMarker* m_cursor = nullptr;
    QwtPlotZoomer* m_zoomer = nullptr;
    double m_time = 0.0;
    double m_start = 0.0;
    double m_end = 0.0;
    bool m_markerVisible = false;
    std::size_t m_resultGeneration = 0;
};

} // namespace MotionOcc
