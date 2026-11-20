#include "mMotionControlInput.h"

namespace MechMotionData {

    mMotionControlInput::mMotionControlInput()
        : m_id(-1),
        m_name(),
        m_inputType(ControlInputType::Constant),
        m_timeStart(0.0),
        m_timeEnd(1.0),
        m_polyCoeffs({ 0.0 }),
        m_curvePoints()
    {
    }

    mMotionControlInput::mMotionControlInput(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_inputType(ControlInputType::Constant),
        m_timeStart(0.0),
        m_timeEnd(1.0),
        m_polyCoeffs({ 0.0 }),
        m_curvePoints()
    {
        if (m_name.isEmpty()) {
            m_name = QString("ControlInput_%1").arg(m_id);
        }
    }

    mMotionControlInput::~mMotionControlInput() {}

    void mMotionControlInput::setId(int id) { m_id = id; }
    int mMotionControlInput::getId() const { return m_id; }

    void mMotionControlInput::setName(const QString& name) { m_name = name; }
    QString mMotionControlInput::getName() const { return m_name; }

    void mMotionControlInput::setInputType(ControlInputType type) { m_inputType = type; }
    ControlInputType mMotionControlInput::getInputType() const { return m_inputType; }

    void mMotionControlInput::setTimeStart(double t) { m_timeStart = t; }
    double mMotionControlInput::getTimeStart() const { return m_timeStart; }

    void mMotionControlInput::setTimeEnd(double t) { m_timeEnd = t; }
    double mMotionControlInput::getTimeEnd() const { return m_timeEnd; }

    void mMotionControlInput::setPolyCoeffs(const QVector<double>& coeffs) { m_polyCoeffs = coeffs; }
    QVector<double> mMotionControlInput::getPolyCoeffs() const { return m_polyCoeffs; }

    void mMotionControlInput::setCurvePoints(const QVector<QPair<double, double>>& points) { m_curvePoints = points; }
    QVector<QPair<double, double>> mMotionControlInput::getCurvePoints() const { return m_curvePoints; }

}
