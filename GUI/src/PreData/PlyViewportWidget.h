//#include "PlyViewportWidget.h"
#pragma once
#include "MPreData_global.h"

#include <QWidget>
#include <QString>
#include "mxr/view/mBaseRend.h"
#include "InteractionCore/Event/InputEvent.h"
#include "InteractionCore/Event/InputHost.h"
#include "InteractionCore/InteractionSystem.h"
#include "IViewportWidget.h"

namespace mxr
{
    class mPlyRend;
}

namespace MechPreData {

//铺层显示窗口的视口包装：照抄PreViewportWidget的模式，
//把mxr::mPlyRend包进带InteractionSystem的QWidget，获得与主视口一致的相机交互
class MPREDATA_EXPORT PlyViewportWidget : public QWidget , public IViewportWidget, public mxr::IInputHost
{
public:
    PlyViewportWidget(QWidget* parent = nullptr);

    QWidget* inputWidget() override { return _inputSurface; }
    void requestFocus() override { _inputSurface->setFocus(); }
    void grabMouse() override { _inputSurface->grabMouse(); }
    void releaseMouse() override { _inputSurface->releaseMouse(); }

    ViewportId viewportId() const override;
    QWidget* asWidget() override;
    mxr::mBaseRend* renderWidget() override;
    QWidget* inputSurface() override;
    mxr::InteractionSystem* interaction() override;
    mxr::CameraController* cameraController() override;  // 通用的viewport service

    void requestRedraw() override;
    void setInputEnabled(bool enabled) override;

    //获取内部的铺层渲染窗口
    mxr::mPlyRend* plyRend();

protected:
    bool eventFilter(QObject* watched, QEvent* e) override;

private:
    std::shared_ptr<mxr::mPlyRend> _rend = nullptr;
    std::shared_ptr<mxr::InteractionSystem> _isys = nullptr;

    QWidget* _inputSurface = nullptr;
};

}
