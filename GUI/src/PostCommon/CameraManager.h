#include "mcommon_global.h"

#include <string>
#include <vector>
#include <list>

namespace mxr {
    class mBaseRend;
}

namespace PostCommon {
    class POSTCOMMON_EXPORT CameraManager
    {
    public:
        // 获取单例
        static CameraManager& getInstance()
        {
            static CameraManager instance; // 只会初始化一次
            return instance;
        }
        // 清除某个窗口的同步
        void removeObserver(mxr::mBaseRend* observer);
        // 清除所有窗口的同步
        void removeAllObserver();
        // 添加某个窗口的同步
        void addObserver(mxr::mBaseRend* observer);
        // 以某个窗口为基础更新其他窗口
        void updateObserver(mxr::mBaseRend* observer);
        void setUpdate(bool need_update) { _is_update = need_update; }
    private:
        // 私有化默认构造函数并且禁止其他拷贝构造
        CameraManager() = default;
        CameraManager(const CameraManager&) = delete;
        CameraManager& operator=(const CameraManager&) = delete;

        bool _is_update = true; // 是否需要同步视图更新

        std::list<mxr::mBaseRend*> observers; // 所有同步的窗口
    };
}