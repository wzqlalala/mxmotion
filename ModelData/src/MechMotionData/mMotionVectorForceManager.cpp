#include "mMotionVectorForce.h"
#include "mMotionBushing.h"
#include "mMotionVectorForceManager.h"

namespace MechMotionData
{
    mMotionVectorForce* mMotionVectorForceManager::getData(int id) const {
        return dynamic_cast<mMotionVectorForce*>(mMotionForceElementManager::getData(id));
    }

    mMotionVectorForce* mMotionVectorForceManager::getData(const QString& name) const {
        return dynamic_cast<mMotionVectorForce*>(mMotionForceElementManager::getData(name));
    }

    void mMotionVectorForceManager::addData(mMotionVectorForce* force) {
        mMotionForceElementManager::addData(force);
    }
}
