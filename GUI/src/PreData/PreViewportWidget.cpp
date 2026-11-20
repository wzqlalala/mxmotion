#include "PreViewportWidget.h"
#include "mxr/base/log.h"

#include <qboxlayout.h>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include "mxr/view/CCSMRendWidget.h"
//#include "mxr/view/DirectModelingArrowManipulator.h"

#include "LowCPlusDefine.h"
#include "InteractionCore/InteractionSystem.h"
#include "InteractionCore/Controller/CameraController.h"

namespace MechPreData
{
    PreViewportWidget::PreViewportWidget(QWidget* parent) : QWidget(parent)
    {
        MXR_LOG_DEBUG("PreViewportWidget init");

        _rend = std::make_shared<mxr::CCSMRendWidget>();
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
        //_bridge = std::make_unique<MyRenderViewBridge>(/*wrap _rend render view*/);
        //_pick = std::make_unique<PrePickService>(/*wrap SelectionCalculator/...*/);
        //_overlay = std::make_unique<PreOverlayService>(/*wrap HighlightController/...*/);

        _isys = std::make_unique<mxr::InteractionSystem>(this, _rend.get());
        //_isys->setServices(_pick.get(), _overlay.get());

        // 加载默认鼠标预设
        //_isys->mapping().loadPreset_CAD(); // 例：RMB orbit / MMB pan / LMB select
    }
    ViewportId PreViewportWidget::viewportId() const
    {
        return ViewportId();
    }
    QWidget* PreViewportWidget::asWidget()
    {
        return this;
    }
    mxr::mBaseRend* PreViewportWidget::renderWidget()
    {
        return _rend.get();
    }
    QWidget* PreViewportWidget::inputSurface()
    {
        return _inputSurface;
    }
    mxr::InteractionSystem* PreViewportWidget::interaction()
    {
        return _isys.get();
    }
    mxr::CameraController* PreViewportWidget::cameraController()
    {
        return _isys->getCameraController();
    }

    void PreViewportWidget::requestRedraw()
    {
    }
    void PreViewportWidget::setInputEnabled(bool enabled)
    {
    }
    bool PreViewportWidget::eventFilter(QObject* watched, QEvent* event)
    {
        // 只处理来自输入表面的事件，避免重复
        if (watched != _inputSurface)
        {

            return QWidget::eventFilter(watched, event);
        }

        // 让点击视口后能接键盘
        if (event->type() == QEvent::MouseButtonPress/* || event->type() == QEvent::MouseMove*/)
        {
            _inputSurface->setFocus();
        }

        // 直接建模箭头操作器优先于拾取：命中箭头/拖拽中时直接消费，
        // 不再交给 InteractionSystem（否则 press 会被拾取 Sel_Solo_Add 吃掉，
        // 导致箭头无法被按下拖动）。
        if (_rend)
        {
           /* if (auto* manip = _rend->getDirectModelingArrowManipulator())
            {
                bool consumed = false;
                switch (event->type())
                {
                case QEvent::MouseButtonPress:
                    consumed = manip->handleMousePress(static_cast<QMouseEvent*>(event));
                    break;
                case QEvent::MouseMove:
                    consumed = manip->handleMouseMove(static_cast<QMouseEvent*>(event));
                    break;
                case QEvent::MouseButtonRelease:
                    consumed = manip->handleMouseRelease(static_cast<QMouseEvent*>(event));
                    break;
                case QEvent::KeyPress:
                    consumed = manip->handleKeyPress(static_cast<QKeyEvent*>(event)->key());
                    break;
                default:
                    break;
                }
                if (consumed)
                {
                    return true;
                }
            }*/
        }

        // 转发给 InteractionSystem（它决定是否消费）
        if (_isys && _isys->handleQtEvent(watched, event))
        {

            //std::printf("PreViewportWidget success!\n");
            return true;
        }
        //return true;

        return QWidget::eventFilter(watched, event);
    }

    //Tab/Shift+Tab 的 Qt 焦点遍历止于视口（等效旧 QMdiSubWindow 架构下子窗口自带的焦点域）：
    //渲染输入链依赖输入表面持有键盘焦点，焦点一旦被遍历带离视口（跳到界面按钮），
    //渲染将收不到任何按键，直到重新点击视口。自研视口容器后视口与界面控件同处一条
    //焦点链，需在此恢复焦点域边界：返回 true 表示本次焦点移动已由本视口处理，
    //Qt 不再向父级（视口容器/主窗口焦点链）继续遍历
    bool PreViewportWidget::focusNextPrevChild(bool next)
    {
        Q_UNUSED(next);
        return true;
    }
}

