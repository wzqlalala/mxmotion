#pragma once
#include <QString>
#include <functional>
#include <utility>

namespace MechMainWindow {
// Compatibility boundary for the original panels. The existing ADS host
// supplies activation; original callers retain their named dock interface.
class mDockLayoutManager {
public:
    static mDockLayoutManager* getInstance() {
        static mDockLayoutManager manager;
        return &manager;
    }
    void setActivationHandler(std::function<void(const QString&)> handler) {
        activate_ = std::move(handler);
    }
    void activateDock(const QString& key) {
        if (activate_) activate_(key);
    }
private:
    std::function<void(const QString&)> activate_;
};
}
