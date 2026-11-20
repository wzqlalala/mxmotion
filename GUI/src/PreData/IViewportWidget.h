//#include "IViewportWidget.h"
#pragma once
#include "MPreData_global.h"

#include <QWidget>

namespace mxr
{
    class CCSMRendWidget;
    class mBaseRend;
    class InteractionSystem;
    class IRenderViewBridge;
    class CameraController;
}

namespace MechPreData {

    struct ViewportId 
    {
        QString name;
    };

    class MPREDATA_EXPORT IViewportWidget
    {
    public:
        virtual ~IViewportWidget() = default;

        // identity
        virtual ViewportId viewportId() const = 0;

        // widget access
        virtual QWidget* asWidget() = 0;               // 方便上层统一 addWidget
        virtual mxr::mBaseRend* renderWidget() = 0;          // 如果上层需要直接访问渲染控件
        virtual QWidget* inputSurface() = 0;           // 真正接收鼠标的表面（OSG/QOpenGLWidget）

        // interaction access
        virtual mxr::InteractionSystem* interaction() = 0;  // 允许上层更换preset、禁用输入等
        virtual mxr::CameraController* cameraController() = 0;  // 通用的viewport service

        // view/control
        virtual void requestRedraw() = 0;
        virtual void setInputEnabled(bool enabled) = 0;

        // 可选：命令入口（如果你们命令要按当前视口执行）
        //virtual bool canHandleCommand(const QString& cmdId) const = 0;
        //virtual void handleCommand(const QString& cmdId) = 0;
    };

}
