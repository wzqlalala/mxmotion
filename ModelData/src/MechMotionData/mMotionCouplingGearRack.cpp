#include "mMotionCouplingGearRack.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionCouplingGearRack::mMotionCouplingGearRack(int id, CouplingType type, const QString& name)
        : mMotionCouplingBase(id, type, name),
        m_radius(1.0)
    {
    }

    mMotionCouplingGearRack::~mMotionCouplingGearRack() {}

    void mMotionCouplingGearRack::setRadius(double radius) { m_radius = radius; }

    double mMotionCouplingGearRack::getRadius() const { return m_radius; }

    mMotionCouplingBase* mMotionCouplingGearRack::copyCoupling(int newId, const QString& newName)
    {
        mMotionCouplingGearRack* newCoupling = new mMotionCouplingGearRack(newId, m_type, newName);

        newCoupling->setRadius(this->getRadius());

        return newCoupling;
    }
}
