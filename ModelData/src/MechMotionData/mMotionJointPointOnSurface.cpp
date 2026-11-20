#include "mMotionJointPointOnSurface.h"

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointPointOnSurface::mMotionJointPointOnSurface(int id, JointType type, const QString& name)
        : mMotionJointBasic(id, type, name)
    {
    }

    mMotionJointPointOnSurface::~mMotionJointPointOnSurface() {}

    void mMotionJointPointOnSurface::setSurfaceFaceIds(const QVector<int>& faceIds) { m_surfaceFaceIds = faceIds; }

    QVector<int> mMotionJointPointOnSurface::getSurfaceFaceIds() const { return m_surfaceFaceIds; }

    mMotionJointBase* mMotionJointPointOnSurface::copyJoint(int newId, const QString& newName)
    {
        mMotionJointPointOnSurface* newJoint = new mMotionJointPointOnSurface(newId, m_type, newName);
        this->copyCommonDataTo(*newJoint);
        newJoint->setOrigin(this->getOrigin());
        newJoint->setAxis(this->getAxis());
        newJoint->setSurfaceFaceIds(this->getSurfaceFaceIds());

        return newJoint;
    }
}
