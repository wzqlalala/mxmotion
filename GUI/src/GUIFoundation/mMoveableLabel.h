#pragma once
#include "guifoundation_global.h"
#include <QApplication>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QLabel>
#include <QWidget>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>

#include "mAppConfiguration.h"
#include "mUiMenu.h"


class GUIFOUNDATION_EXPORT MovableLabel : public QLabel {
    Q_OBJECT
public:
    explicit MovableLabel(QWidget *parent = nullptr) : QLabel(parent), _dragging(false) {}

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            _dragging = true;
            _dragPosition = event->globalPos() - this->frameGeometry().topLeft();
            event->accept();
        }
        else if (event->button() == Qt::RightButton) {
            showContextMenu(event->globalPos());
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (_dragging) {
            move(event->globalPos() - _dragPosition);
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        _dragging = false;
        event->accept();
    }

private:
    bool _dragging;
    QPoint _dragPosition;

    void showContextMenu(const QPoint &pos) {
        // 统一走 mUiMenu 自绘菜单（悬浮铺 UiPalette::hover 底 + text 字，三主题对比达标）。
        // 旧实现只设 QMenu::item:selected 底色 rgb(98,133,166)、未设文字色：
        // 一旦命中自绘体系，该项既被选中介质又触发 hover 底，字色浅导致"选中行文字丢失"。
        MechMainWindow::mUiMenu contextMenu(this);

        QString act;
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            act = "删除";
        }
        else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
        {
            act = "delete";
        }
        QAction actionDelete(act, this);
        connect(&actionDelete, &QAction::triggered, this, &MovableLabel::deleteLabel);
        contextMenu.addAction(&actionDelete);
        contextMenu.exec(pos);
    }

    void deleteLabel() {
        deleteLater();
    }
};
