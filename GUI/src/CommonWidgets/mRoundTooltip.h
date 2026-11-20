#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>

#include "CommonWidgets_global.h"

namespace MechMainWindow
{
    /**
     * @brief 圆角 Tooltip 控件
     * 使用 Qt::ToolTip 窗口标志 + WA_TranslucentBackground + 自定义 paintEvent 实现真正圆角，
     * 解决 Windows 原生 QToolTip 不支持 border-radius 的问题。
     */
    class COMMONWIDGETS_EXPORT mRoundTooltip : public QWidget
    {
        Q_OBJECT

    public:
        explicit mRoundTooltip(QWidget* parent = nullptr);
        ~mRoundTooltip() override = default;

        // 设置提示文本
        void setText(const QString& text);
        QString text() const;

        // 在指定全局位置显示（自动计算位置避免超出屏幕）
        void showAt(const QPoint& globalPos);

        // 隐藏并停止自动隐藏定时器
        void hideTooltip();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;

    private:
        void initUI();
        void adjustPosition(const QPoint& globalPos);

        QLabel* _label = nullptr;
        QTimer* _hideTimer = nullptr;
        int _borderRadius = 8;
        int _paddingH = 10;
        int _paddingV = 6;
    };
}
