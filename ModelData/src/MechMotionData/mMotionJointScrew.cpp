#include "mMotionJointScrew.h"

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointScrew::mMotionJointScrew(int id, JointType type, const QString& name)
        : mMotionJointBase(id, type, name),
        m_origin(0.0f, 0.0f, 0.0f),
        m_axis(0.0f, 0.0f, 1.0f),
        m_lead(0.01)
    {
    }

    mMotionJointScrew::~mMotionJointScrew() {}

    void mMotionJointScrew::setOrigin(const QVector3D& origin) { m_origin = origin; }

    QVector3D mMotionJointScrew::getOrigin() const { return m_origin; }

    void mMotionJointScrew::setAxis(const QVector3D& axis) { m_axis = axis; }

    QVector3D mMotionJointScrew::getAxis() const { return m_axis; }

    void mMotionJointScrew::setLead(double lead) { m_lead = lead; }

    double mMotionJointScrew::getLead() const { return m_lead; }

    mMotionJointBase* mMotionJointScrew::copyJoint(int newId, const QString& newName)
    {
        mMotionJointScrew* newJoint = new mMotionJointScrew(newId, m_type, newName);
        this->copyCommonDataTo(*newJoint);
        newJoint->setOrigin(this->getOrigin());
        newJoint->setAxis(this->getAxis());
        newJoint->setLead(this->getLead());

        return newJoint;
    }
}
