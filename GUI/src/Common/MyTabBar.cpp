#include "MyTabBar.h"

#include "UiMetrics.h"
#include "UiPalette.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>

using namespace MxFunctions;

namespace MechMainWindow
{
    MyTabBar::MyTabBar(QWidget* parent) : QTabBar(parent)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::TabFocus);
        setExpanding(false);
        connect(this, &QTabBar::currentChanged,
            this, [this](int) { refreshGeometry(); });
    }

    QFont MyTabBar::fontForTab(int index) const
    {
        //字号/字族钉定 UiMetrics 令牌（规则 1.2 切换文字=menuFont，标准档 14px）：
        //页签裸继承 qApp 字体会取 QFont 缺省 12pt(≈16px)，比规范大 2px（NVH 面板
        //页签病灶实证；mMenuWidget 调用点先例同款钉定，收进通用件后各消费方单源）
        QFont f = font();
        f.setFamily(UiMetrics::uiFontFamily());
        f.setPixelSize(UiMetrics::tabFontSize());
        f.setWeight(index == currentIndex() ? QFont::DemiBold : QFont::Normal);
        return f;
    }

    QSize MyTabBar::tabSizeHint(int index) const
    {
        const QFontMetrics fm(fontForTab(index));
        return QSize(fm.horizontalAdvance(tabText(index)) + 2 * UiMetrics::tabPadding(),
            UiMetrics::tabBarHeight());
    }

    void MyTabBar::refreshGeometry()
    {
        // Qt 5 的文字尺寸缓存以页签文本为键，不能只靠 updateGeometry 清除。
        // 同步恢复原文本即可使缓存失效；不会重建页签或刷新期间绘制空标签。
        QStringList texts;
        for (int i = 0; i < count(); ++i)
            texts.append(tabText(i));
        for (int i = 0; i < count(); ++i)
            setTabText(i, QString());
        for (int i = 0; i < count(); ++i)
            setTabText(i, texts.at(i));
        updateGeometry();
        update();
    }

    void MyTabBar::setSeparatorGroup(const QStringList& tabTexts)
    {
        m_separatorGroup = tabTexts;
        update();
    }

    void MyTabBar::setResidentTabNames(const QStringList& tabNames)
    {
        m_residentTabNames = tabNames;
        update();
    }

    void MyTabBar::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);
        const int underlineY = height() - 2;
        for (int i = 0; i < count(); ++i)
        {
            const QRect tabR = tabRect(i);
            const QString text = tabText(i);
            if (text.isEmpty())
                continue;
            const bool selected = i == currentIndex();
            const bool enabled = isTabEnabled(i);
            const bool hover = enabled && i == m_lastHoverIndex;
            painter.fillRect(tabR, hover ? UiPalette::hover() : UiPalette::surface());

            const QFont f = fontForTab(i);
            const QFontMetrics fm(f);
            const int textWidth = fm.horizontalAdvance(text);
            const int textX = tabR.x() + UiMetrics::tabPadding();
            const int textY = tabR.y() + (tabR.height() - fm.height()) / 2 + fm.ascent();
            painter.setFont(f);
            painter.setPen(!enabled ? UiPalette::disabled()
                : selected ? UiPalette::selectedText() : UiPalette::text());
            painter.drawText(textX, textY, text);
            if (selected && enabled)
                painter.fillRect(textX, underlineY, textWidth, 2, UiPalette::primary());
        }

        //整行基线（opt-in，面板页签用）：与选中蓝底线同一 2px 带、铺到行右缘，
        //起点与首个页签文字左缘对齐（原型实测灰线不伸进文字左缘之外的前导空隙）；
        //选中段蓝线重画置顶，规则 1.2 选中标识不变。默认关，功能区形态零影响
        if (m_baselineEnabled && count() > 0)
        {
            const int firstTextX = tabRect(0).x() + UiMetrics::tabPadding();
            painter.fillRect(firstTextX, underlineY, width() - firstTextX, 2,
                UiPalette::border());
            const int cur = currentIndex();
            if (cur >= 0 && isTabEnabled(cur) && !tabText(cur).isEmpty())
            {
                const QFontMetrics fm(fontForTab(cur));
                painter.fillRect(tabRect(cur).x() + UiMetrics::tabPadding(), underlineY,
                    fm.horizontalAdvance(tabText(cur)), 2, UiPalette::primary());
            }
        }

        int groupIndex = -1;
        int dynamicIndex = -1;
        for (int i = 0; i < count(); ++i)
        {
            if (groupIndex < 0 && m_separatorGroup.contains(tabText(i)))
                groupIndex = i;
            if (dynamicIndex < 0 && !m_residentTabNames.isEmpty()
                && !m_residentTabNames.contains(tabText(i)))
                dynamicIndex = i;
        }
        painter.setPen(QPen(UiPalette::border(), 1));
        const int separatorHeight = UiMetrics::separatorLineHeight();
        const int y0 = (height() - separatorHeight) / 2;
        for (int index : { groupIndex, dynamicIndex })
        {
            if (index > 0)
            {
                const int x = (tabRect(index - 1).right() + tabRect(index).left()) / 2;
                painter.drawLine(x, y0, x, y0 + separatorHeight);
            }
        }

        if (m_keyboardFocus && hasFocus() && currentIndex() >= 0
            && isTabEnabled(currentIndex()))
        {
            painter.setPen(QPen(UiPalette::focus(), 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(tabRect(currentIndex()).adjusted(1, 1, -2, -2));
        }
    }

    void MyTabBar::mouseMoveEvent(QMouseEvent* event)
    {
        QTabBar::mouseMoveEvent(event);
        const int index = tabAt(event->pos());
        if (index != m_lastHoverIndex)
        {
            m_lastHoverIndex = index;
            update();
        }
    }

    void MyTabBar::leaveEvent(QEvent* event)
    {
        QTabBar::leaveEvent(event);
        m_lastHoverIndex = -1;
        update();
    }

    void MyTabBar::focusInEvent(QFocusEvent* event)
    {
        m_keyboardFocus = event->reason() == Qt::TabFocusReason
            || event->reason() == Qt::BacktabFocusReason;
        QTabBar::focusInEvent(event);
        update();
    }

    void MyTabBar::focusOutEvent(QFocusEvent* event)
    {
        m_keyboardFocus = false;
        QTabBar::focusOutEvent(event);
        update();
    }

    void MyTabBar::keyPressEvent(QKeyEvent* event)
    {
        m_keyboardFocus = true;
        if (count() > 0 && (event->key() == Qt::Key_Home || event->key() == Qt::Key_End))
        {
            const int direction = event->key() == Qt::Key_Home ? 1 : -1;
            int index = direction > 0 ? 0 : count() - 1;
            while (index >= 0 && index < count() && !isTabEnabled(index))
                index += direction;
            if (index >= 0 && index < count())
                setCurrentIndex(index);
            event->accept();
            update();
            return;
        }
        QTabBar::keyPressEvent(event);
        update();
    }
}
