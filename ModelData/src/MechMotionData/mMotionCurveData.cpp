#include "mMotionCurveData.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionCurveData::mMotionCurveData(int id,  const QString& name)
        : m_id(id),
        m_name(name)
    {
        if (m_name.isEmpty()) {
            m_name = QString("Curve_%1").arg(m_id);
        }
    }

    mMotionCurveData::~mMotionCurveData() {}

    int mMotionCurveData::getId() const { return m_id; }

    void mMotionCurveData::setName(const QString& name) { m_name = name; }

    QString mMotionCurveData::getName() const { return m_name; }

}
