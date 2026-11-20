#include "mMotionJointBasic.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointBasic::mMotionJointBasic(int id, JointType type, const QString& name)
        : mMotionJointBase(id, type, name),
        m_origin(0.0f, 0.0f, 0.0f),
        m_axis(0.0f, 0.0f, 1.0f) // Ä¬ÈÏZÖá
    {
    }

    mMotionJointBasic::~mMotionJointBasic() {}

    void mMotionJointBasic::setOrigin(const QVector3D& origin) { m_origin = origin; }

    QVector3D mMotionJointBasic::getOrigin() const { return m_origin; }

    void mMotionJointBasic::setAxis(const QVector3D& axis) { m_axis = axis; }

    QVector3D mMotionJointBasic::getAxis() const { return m_axis; }

    mMotionJointBase* mMotionJointBasic::copyJoint(int newId, const QString& newName)
    {
        mMotionJointBasic* newJoint = new mMotionJointBasic(newId, m_type, newName);
        this->copyCommonDataTo(*newJoint);
        newJoint->setOrigin(this->getOrigin());
        newJoint->setAxis(this->getAxis());

        return newJoint;
    }
}
