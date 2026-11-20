#pragma once
#pragma execution_character_set("utf-8")

#include "mCommon_global.h"

#include <QTabBar>
#include <QStringList>

namespace MechMainWindow
{
    class COMMON_EXPORT MyTabBar : public QTabBar
    {
        Q_OBJECT
    public:
        explicit MyTabBar(QWidget* parent = nullptr);
        void refreshGeometry();

        QSize tabSizeHint(int index) const override;
        QSize minimumTabSizeHint(int index) const override { return tabSizeHint(index); }

        void setSeparatorGroup(const QStringList& tabTexts);
        void setResidentTabNames(const QStringList& tabNames);

        //整行基线开关（面板页签用，默认关）：tab 行底部 2px 通栏浅灰线，起点与
        //首个页签文字左缘对齐、铺到行右缘；选中段 2px 蓝底线最后重画置顶，规则
        //1.2 选中标识不变。功能区页签不开基线，形态零影响
        void setBaselineEnabled(bool on) { m_baselineEnabled = on; }

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        QFont fontForTab(int index) const;

        QStringList m_separatorGroup;
        QStringList m_residentTabNames;
        int m_lastHoverIndex = -1;
        bool m_keyboardFocus = false;
        bool m_baselineEnabled = false;   //整行基线（面板页签 opt-in，默认关）
    };
}
