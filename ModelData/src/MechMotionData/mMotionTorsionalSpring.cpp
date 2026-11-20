#include "mMotionTorsionalSpring.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionTorsionalSpring::mMotionTorsionalSpring(int id, const QString& name)
        : mMotionSpringBase(id, SpringType::Torsional, name)
    {
    }

    mMotionTorsionalSpring::~mMotionTorsionalSpring() {}

    mMotionSpringBase* mMotionTorsionalSpring::copySpring(int newId, const QString& newName)
    {
        mMotionTorsionalSpring* newSpring = new mMotionTorsionalSpring(newId, newName);

        newSpring->setStiffness(this->getStiffness());
        newSpring->setDamping(this->getDamping());
        newSpring->setPreload(this->getPreload());
        newSpring->setSpringDirectionMode(this->getSpringDirectionMode());
        newSpring->setDampingDirectionMode(this->getDampingDirectionMode());
        newSpring->setFreeValue(this->getFreeValue());
        newSpring->setInstallValue(this->getInstallValue());
        newSpring->setTable(this->getTableX(), this->getTableY());
        newSpring->setForceFunction(this->getForceFunction());
        newSpring->setDampingFunction(this->getDampingFunction());
        newSpring->setDampingEnabled(this->isDampingEnabled());

        return newSpring;
    }

}
