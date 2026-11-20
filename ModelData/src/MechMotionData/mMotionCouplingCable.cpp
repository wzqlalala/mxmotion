#include "mMotionCouplingCable.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionCouplingCable::mMotionCouplingCable(int id, CouplingType type, const QString& name)
        : mMotionCouplingBase(id, type, name),
        m_ratio(1.0),
        m_reverse(false)
    {
    }

    mMotionCouplingCable::~mMotionCouplingCable() {}

    void mMotionCouplingCable::setRatio(double ratio) { m_ratio = ratio; }

    double mMotionCouplingCable::getRatio() const { return m_ratio; }

    void mMotionCouplingCable::setReverse(bool reverse) { m_reverse = reverse; }

    bool mMotionCouplingCable::getReverse() const { return m_reverse; }

    mMotionCouplingBase* mMotionCouplingCable::copyCoupling(int newId, const QString& newName)
    {
        mMotionCouplingCable* newCoupling = new mMotionCouplingCable(newId, m_type, newName);

        newCoupling->setRatio(this->getRatio());
        newCoupling->setReverse(this->getReverse());

        return newCoupling;
    }
}
