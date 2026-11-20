#pragma once
#pragma execution_character_set("utf-8")

#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QShowEvent>

#include "CommonWidgets_global.h"

namespace MechMainWindow
{
    /**
     * @brief 圆角 QListWidget
     * 解决 Windows 上 Qt::Popup 窗口 border-radius 样式表不生效的问题：
     * 1. 重写 paintEvent 绘制圆角背景
     * 2. resizeEvent/showEvent 中 setMask 裁剪窗口形状
     */
    class COMMONWIDGETS_EXPORT mRoundListWidget : public QListWidget
    {
        Q_OBJECT

    public:
        explicit mRoundListWidget(QWidget* parent = nullptr);
        ~mRoundListWidget() override = default;

        void setBorderRadius(int radius);

        /**
         * @brief 判断全局坐标是否落在本控件矩形内（含被 mask 裁掉的四角）
         * 弹窗可见时其矩形覆盖的区域视为"弹窗区域"，供上层据此拦截下层主控件气泡。
         */
        bool coversGlobalPos(const QPoint& globalPos) const;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void showEvent(QShowEvent* event) override;

    private:
        void updateMask();

        int _borderRadius = 6;
    };
}
