#include "CameraManager.h"
#include "mxr/view/mBaseRend.h"
namespace PostCommon {

    void CameraManager::updateObserver(mxr::mBaseRend* observer)
    {
        if (!_is_update) return;

        for (mxr::mBaseRend* cur : observers)
        {
            //if (cur == observer) continue;
            //cur->getModelCamera()->syncData(observer->getModelCamera().get());
            //cur->getCommonCamera()->syncData(observer->getCommonCamera().get());
            //cur->update();
        }
    }


    void CameraManager::removeObserver(mxr::mBaseRend* observer)
    {
        for (auto cur : observers)
        {
            if (cur == observer)
            {
                observers.remove(observer);
                return;
            }
        }
    }

    void CameraManager::removeAllObserver()
    {
        observers.clear();
    }

    void CameraManager::addObserver(mxr::mBaseRend* observer)
    {
        for (auto cur : observers) 
			if (cur == observer) 
				return;

        observers.emplace_back(observer);
    }
}