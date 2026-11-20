#include "PlyViewportWidget.h"
#include "mxr/base/log.h"

#include <qboxlayout.h>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include "mxr/view/mPlyRend.h"

#include "LowCPlusDefine.h"
#include "InteractionCore/InteractionSystem.h"
#include "InteractionCore/Controller/CameraController.h"

namespace MechPreData
{
    PlyViewportWidget::PlyViewportWidget(QWidget* parent) : QWidget(parent)
    {
        MXR_LOG_DEBUG("PlyViewportWidget init");

        _rend = std::make_shared<mxr::mPlyRend>("Ply Stack Plot");
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(_rend.get());

        _inputSurface = _rend->getOsgWidget();
        Q_ASSERT(_inputSurface);

        // 让输入表面能收到 move （hover 高亮用）
        _inputSurface->setMouseTracking(true);
        _inputSurface->setFocusPolicy(Qt::StrongFocus);

        // 在 UI 层监听输入表面的事件
        _inputSurface->installEventFilter(this);

        _rend->installEventFilter(this);

        // 组装 InteractionSystem + Services
        _isys = std::make_unique<mxr::InteractionSystem>(this, _rend.get());
    }
    ViewportId PlyViewportWidget::viewportId() const
    {
        return ViewportId();
    }
    QWidget* PlyViewportWidget::asWidget()
    {
        return this;
    }
    mxr::mBaseRend* PlyViewportWidget::renderWidget()
    {
        return _rend.get();
    }
    QWidget* PlyViewportWidget::inputSurface()
    {
        return _inputSurface;
    }
    mxr::InteractionSystem* PlyViewportWidget::interaction()
    {
        return _isys.get();
    }
    mxr::CameraController* PlyViewportWidget::cameraController()
    {
        return _isys->getCameraController();
    }

    void PlyViewportWidget::requestRedraw()
    {
    }
    void PlyViewportWidget::setInputEnabled(bool enabled)
    {
    }
    mxr::mPlyRend* PlyViewportWidget::plyRend()
    {
        return _rend.get();
    }
    bool PlyViewportWidget::eventFilter(QObject* watched, QEvent* event)
    {
        // 只处理来自输入表面的事件，避免重复
        if (watched != _inputSurface)
        {

            return QWidget::eventFilter(watched, event);
        }

        // 让点击视口后能接键盘
        if (event->type() == QEvent::MouseButtonPress)
        {
            _inputSurface->setFocus();
        }

        // 转发给 InteractionSystem（它决定是否消费）
        if (_isys && _isys->handleQtEvent(watched, event))
        {
            return true;
        }

        return QWidget::eventFilter(watched, event);
    }
}
