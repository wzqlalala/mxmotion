#include "mMotionJointCurveOnCurve.h"

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointCurveOnCurve::mMotionJointCurveOnCurve(int id, JointType type, const QString& name)
        : mMotionJointBasic(id, type, name)
    {
    }

    mMotionJointCurveOnCurve::~mMotionJointCurveOnCurve() {}

    void mMotionJointCurveOnCurve::setCurveIEdgeIds(const QVector<int>& edgeIds) { m_curveIEdgeIds = edgeIds; }

    QVector<int> mMotionJointCurveOnCurve::getCurveIEdgeIds() const { return m_curveIEdgeIds; }

    void mMotionJointCurveOnCurve::setCurveJEdgeIds(const QVector<int>& edgeIds) { m_curveJEdgeIds = edgeIds; }

    QVector<int> mMotionJointCurveOnCurve::getCurveJEdgeIds() const { return m_curveJEdgeIds; }

    void mMotionJointCurveOnCurve::setAllowSliding(bool allow) { m_allowSliding = allow; }

    bool mMotionJointCurveOnCurve::getAllowSliding() const { return m_allowSliding; }

    mMotionJointBase* mMotionJointCurveOnCurve::copyJoint(int newId, const QString& newName)
    {
        mMotionJointCurveOnCurve* newJoint = new mMotionJointCurveOnCurve(newId, m_type, newName);
        this->copyCommonDataTo(*newJoint);
        newJoint->setOrigin(this->getOrigin());
        newJoint->setAxis(this->getAxis());
        newJoint->setCurveIEdgeIds(this->getCurveIEdgeIds());
        newJoint->setCurveJEdgeIds(this->getCurveJEdgeIds());
        newJoint->setAllowSliding(this->getAllowSliding());

        return newJoint;
    }
}
