#include "mMotionJointPointOnCurve.h"

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointPointOnCurve::mMotionJointPointOnCurve(int id, JointType type, const QString& name)
        : mMotionJointBasic(id, type, name)
    {
    }

    mMotionJointPointOnCurve::~mMotionJointPointOnCurve() {}

    void mMotionJointPointOnCurve::setCurveEdgeIds(const QVector<int>& edgeIds) { m_curveEdgeIds = edgeIds; }

    QVector<int> mMotionJointPointOnCurve::getCurveEdgeIds() const { return m_curveEdgeIds; }

    mMotionJointBase* mMotionJointPointOnCurve::copyJoint(int newId, const QString& newName)
    {
        mMotionJointPointOnCurve* newJoint = new mMotionJointPointOnCurve(newId, m_type, newName);
        this->copyCommonDataTo(*newJoint);
        newJoint->setOrigin(this->getOrigin());
        newJoint->setAxis(this->getAxis());
        newJoint->setCurveEdgeIds(this->getCurveEdgeIds());

        return newJoint;
    }
}
