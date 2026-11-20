#include "mCustomTooltip.h"
#include "ui_mCustomTooltip.h"
#include "UiPalette.h"

using namespace MxFunctions;

namespace MechMainWindow
{
	mCustomTooltip* mCustomTooltip::_instance = nullptr;
    
    mCustomTooltip* mCustomTooltip::getInstance()
    {
        if (!_instance)
            _instance = new mCustomTooltip;
        return _instance;
    }

    mCustomTooltip::mCustomTooltip(QWidget* parent)
        : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
        , ui(new Ui::mCustomTooltipClass)
    {
        ui->setupUi(this);
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
            Qt::WindowStaysOnTopHint |   // 置顶
            Qt::BypassWindowManagerHint); // 绕过窗口管理器裁剪
        qApp->installEventFilter(this);   // 监听全局事件
        m_fade = new QPropertyAnimation(this, "opacity");
        m_fade->setDuration(180);
        m_fade->setStartValue(1.0f);
        m_fade->setEndValue(0.0f);
        connect(m_fade, &QPropertyAnimation::finished, this, &QWidget::hide);
    }



    float mCustomTooltip::opacity() const 
    {
        return windowOpacity(); 
    }
    
    void  mCustomTooltip::setOpacity(float op) 
    { 
        setWindowOpacity(op); 
    }

    void mCustomTooltip::showTip(const QString& title,const QPixmap& icon,
        const QString& body, const QPoint& pos,QWidget* anchor)
    {
        refreshTheme();   // 弹出前按当前主题刷新底色/边框/文字色
        ui->titleLabel->setText(title);
        if (!icon.isNull()) {
            ui->iconLabel->setPixmap(icon);
            ui->iconLabel->setScaledContents(true);   // 让图片自动填满 label
            ui->iconLabel->show(); 
        }
        else {
            ui->iconLabel->hide();
        }
        ui->bodyLabel->setText(body);
        ui->bodyLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

        adjustSize();   // 让提示框按内容重新计算大小

        QRect desk = QApplication::desktop()->availableGeometry(pos);
        QPoint target = pos + QPoint(16, 16);
        if (target.x() + width() > desk.right())  target.rx() -= width() + 32;
        if (target.y() + height() > desk.bottom()) target.ry() -= height() + 32;

        move(target);
        setOpacity(1.0f);
        show();
        raise();
        m_anchor = anchor;
        if (m_anchor) m_anchor->installEventFilter(this);
 
        ui->bodyLabel->setSizePolicy(QSizePolicy::Expanding,
            QSizePolicy::Preferred);
        ui->bodyLabel->setText(body);
        ui->bodyLabel->adjustSize();     
        adjustSize();                 
    }

    void mCustomTooltip::hideTip()
    {
        startFadeOut();
    }

    void mCustomTooltip::startFadeOut()
    {
        if (m_mouseInside) return;
        m_fade->stop();
        m_fade->start();
    }

    void mCustomTooltip::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        // 底色/边框随主题（UiPalette surface/border）
        p.setBrush(UiPalette::surface());
        p.setPen(QPen(UiPalette::border(), 1));
        p.drawRoundedRect(rect(), 6, 6);
    }

    void mCustomTooltip::refreshTheme()
    {
        // 标题/正文文字色随主题；气泡本身是 Qt::Tool 顶层窗，样式表就地写
        const QString labelQss = QStringLiteral("QLabel { color:%1; background:transparent; border:none; }");
        ui->titleLabel->setStyleSheet(labelQss.arg(UiPalette::textHex()));
        ui->bodyLabel->setStyleSheet(labelQss.arg(UiPalette::labelHex()));
        update();   // 立即按新底色/边框重绘
    }

    bool mCustomTooltip::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::FocusOut) {
            hideTip();
        }
        return QWidget::eventFilter(watched, event);
    }

    void mCustomTooltip::leaveEvent(QEvent*)
    {
        m_mouseInside = false;
    }


    TooltipInstaller::TooltipInstaller(const QString& title,const QPixmap& icon,
        const QString& body, int delayMs, QObject* parent)
        : QObject(parent), m_delay(delayMs)
    {
        _title = title;
        _icon = icon;
        _body = body;
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &TooltipInstaller::slotShow);
    }

    bool TooltipInstaller::eventFilter(QObject* watched, QEvent* event)
    {
        QWidget* w = qobject_cast<QWidget*>(watched);
        QAction* a = qobject_cast<QAction*>(watched);
        QString tip;
        if (w)      tip = w->toolTip();
        else if (a) tip = a->toolTip();
        else        return false;

        switch (event->type()) {
        case QEvent::HoverEnter:
            m_target = watched;
            m_timer.start(m_delay);
            _curpos = QCursor::pos();
            break;
        case QEvent::HoverLeave:
            m_timer.stop();
            mCustomTooltip::getInstance()->hideTip();
            break;
        default: break;
        }
        return false;
    }

    void TooltipInstaller::slotShow()
    {
        if (!m_target) return;
        QString tip;
        QWidget* w = qobject_cast<QWidget*>(m_target);
        QAction* a = qobject_cast<QAction*>(m_target);
        if (w) 
        {
            mCustomTooltip::getInstance()->showTip(_title, _icon, _body, _curpos, w);
        }
        else if (a) 
        {
            if (QWidget* pw = a->associatedWidgets().value(0))
            {
                mCustomTooltip::getInstance()->showTip(_title, _icon, _body, _curpos, pw);
            }
        }
    }

    ToolbarTipInstaller::ToolbarTipInstaller(QToolBar* toolbar, int delayMs, QObject* parent)
        : QObject(parent), m_toolBar(toolbar), m_timer(this)
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &ToolbarTipInstaller::slotShow);
    }

    bool ToolbarTipInstaller::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched != m_toolBar)
            return false;

        switch (event->type()) {
        case QEvent::HoverEnter:
        case QEvent::HoverMove: {
            QHoverEvent* he = static_cast<QHoverEvent*>(event);
            QAction* act = m_toolBar->actionAt(he->pos());
            if (act) {
                if (m_pendingAction != act) {
                    m_pendingAction = act;
                    m_lastPos = he->pos();
                    restartTimer();
                }
            }
            else {
                m_timer.stop();
                mCustomTooltip::getInstance()->hideTip();
                m_pendingAction = nullptr;
            }
            break;
        }
        case QEvent::HoverLeave:
            m_timer.stop();
            mCustomTooltip::getInstance()->hideTip();
            m_pendingAction = nullptr;
            break;

        default:
            break;
        }
        return false;
    }

    void ToolbarTipInstaller::slotShow()
    {
        if (!m_pendingAction) return;

        QString title = m_pendingAction->property("ttTitle").toString();
        QString body = m_pendingAction->property("ttBody").toString();
        if (body.isEmpty() || title.isEmpty())
        {
            return;
        }
        QPixmap icon = m_pendingAction->icon().pixmap(32, 32);

        if (QWidget* btn = m_toolBar->widgetForAction(m_pendingAction)) {
            QPoint pos = btn->mapToGlobal(btn->rect().center());
            mCustomTooltip::getInstance()->showTip(title, icon, body, pos, btn);
        }
    }

    void ToolbarTipInstaller::restartTimer()
    {
        m_timer.start(400);   
    }

    MenuTipFilter::MenuTipFilter(QMenu* menu, int delayMs, QObject* parent)
        : QObject(parent), m_menu(menu), m_timer(this)
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &MenuTipFilter::slotShowTip);
    }

    bool MenuTipFilter::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched != m_menu)
            return false;

        switch (event->type()) {
        case QEvent::HoverEnter:
        case QEvent::HoverMove: {
            QHoverEvent* he = static_cast<QHoverEvent*>(event);
            QAction* act = m_menu->actionAt(he->pos());
            if (act && act->isEnabled() && !act->isSeparator()) {
                if (m_pendingAction != act) {
                    m_pendingAction = act;
                    restartTimer();
                }
            }
            else {
                m_timer.stop();
                mCustomTooltip::getInstance()->hideTip();
                m_pendingAction = nullptr;
            }
            break;
        }
        case QEvent::HoverLeave:
            m_timer.stop();
            mCustomTooltip::getInstance()->hideTip();
            m_pendingAction = nullptr;
            break;
        default:
            break;
        }
        return false;
    }

    void MenuTipFilter::slotShowTip()
    {
        if (!m_pendingAction) return;

        QString title = m_pendingAction->property("ttTitle").toString();
        if (title.isEmpty()) title = m_pendingAction->text();
        QString body = m_pendingAction->property("ttBody").toString();
        if (body.isEmpty()) body = m_pendingAction->toolTip();
        QPixmap icon = m_pendingAction->icon().pixmap(32, 32);

        // 对于 QWidgetAction（如自定义复选框菜单项），action 本身没有 text/toolTip/icon，
        // 若三者均为空则无需显示空提示框，直接返回
        if (title.isEmpty() && body.isEmpty() && icon.isNull())
        {
            return;
        }

        // 固定锚点：当前行矩形中心
        QRect rc = m_menu->actionGeometry(m_pendingAction);
        QPoint pos = m_menu->mapToGlobal(rc.center());
        mCustomTooltip::getInstance()->showTip(title, icon, body, pos, m_menu);
    }

    void MenuTipFilter::restartTimer()
    {
        m_timer.start(400);
    }

    InputIDLineEditTipInstaller::InputIDLineEditTipInstaller(QLineEdit* lineEdit, int delayMs, QObject* parent)
        : QObject(parent), m_lineEdit(lineEdit), m_delay(delayMs), m_timer(this)
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &InputIDLineEditTipInstaller::slotShow);
        // 确保控件启用了 hover 事件
        m_lineEdit->setMouseTracking(true);
        m_lineEdit->setAttribute(Qt::WA_Hover, true);
        // 安装事件过滤器
        m_lineEdit->installEventFilter(this);
    }

    bool InputIDLineEditTipInstaller::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched != m_lineEdit)
            return false;

        switch (event->type()) {
        case QEvent::HoverEnter:
        case QEvent::HoverMove: {
            QHoverEvent* he = static_cast<QHoverEvent*>(event);
            if (m_pendingLineEdit != m_lineEdit) {
                m_pendingLineEdit = m_lineEdit;
                m_lastPos = he->pos();
                restartTimer();
            }
            break;
        }
        case QEvent::HoverLeave:
            m_timer.stop();
            mCustomTooltip::getInstance()->hideTip();
            m_pendingLineEdit = nullptr;
            break;
        default:
            break;
        }

        return false;
    }

    void InputIDLineEditTipInstaller::slotShow()
    {
        if (!m_pendingLineEdit) return;

        QString title = m_pendingLineEdit->property("ttTitle").toString();
        QString body = m_pendingLineEdit->property("ttBody").toString();
        if (body.isEmpty() || title.isEmpty()) {
            return;
        }

        QPoint pos = m_pendingLineEdit->mapToGlobal(m_pendingLineEdit->rect().center());
        mCustomTooltip::getInstance()->showTip(title, QPixmap(""), body, pos, m_pendingLineEdit);
    }

    void InputIDLineEditTipInstaller::restartTimer()
    {
        m_timer.start(400);
    }
}