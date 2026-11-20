//#include "PreViewportWidget.h"
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
    class CCSMRendWidget;
}

namespace MechPreData {

class MPREDATA_EXPORT PreViewportWidget : public QWidget , public IViewportWidget, public mxr::IInputHost
{
public:
    PreViewportWidget(QWidget* parent = nullptr);

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

protected:
    bool eventFilter(QObject* watched, QEvent* e) override;
    bool focusNextPrevChild(bool next) override;

private:
    std::shared_ptr<mxr::CCSMRendWidget> _rend = nullptr;
    std::shared_ptr<mxr::InteractionSystem> _isys = nullptr;

    QWidget* _inputSurface = nullptr;
};

}
