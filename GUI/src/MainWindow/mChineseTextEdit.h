#ifndef MCHINESETEXTEDIT_H
#define MCHINESETEXTEDIT_H

#include <QTextEdit>
#include <QContextMenuEvent>
#include <QMenu>

#include "mUiMenu.h"
#include "ThemeManager.h"

namespace MechMainWindow
{
    class mChineseTextEdit : public QTextEdit {
        Q_OBJECT
    public:
        explicit mChineseTextEdit(QWidget* parent = nullptr) : QTextEdit(parent) {}

    protected:
        void contextMenuEvent(QContextMenuEvent* event) override {
            // 裸 QMenu 不在自绘菜单体系内时回退原生样式，悬浮行底/文字色由系统主题
            // 决定（本机浅色下高亮底近白、文字变纯白 → 白底白字不可读）。
            // 按模型树/曲线图右键同口径换 mUiMenu 自绘：悬浮/按下铺 UiPalette::hover
            // 底 + text 字、禁用项 disabled 字，三主题下对比度均达标；
            // 菜单项与触发逻辑（复制/全选）保持不变，仅接管绘制样式
            mUiMenu menu(this);
            ThemeManager::applyMenuStyle(&menu);

            // 根据状态创建菜单项
            /*QAction* undo = menu.addAction("撤销");
            undo->setEnabled(isUndoRedoEnabled() && document()->isUndoAvailable());
            connect(undo, &QAction::triggered, this, &QTextEdit::undo);

            QAction* redo = menu.addAction("恢复");
            redo->setEnabled(isUndoRedoEnabled() && document()->isRedoAvailable());
            connect(redo, &QAction::triggered, this, &QTextEdit::redo);

            menu.addSeparator();

            QAction* cut = menu.addAction("剪切");
            cut->setEnabled(textCursor().hasSelection() && !isReadOnly());
            connect(cut, &QAction::triggered, this, &QTextEdit::cut);*/

            QAction* copy = menu.addAction("复制");
            copy->setEnabled(textCursor().hasSelection());
            connect(copy, &QAction::triggered, this, &QTextEdit::copy);

            /*QAction* paste = menu.addAction("粘贴");
            paste->setEnabled(!isReadOnly() && canPaste());
            connect(paste, &QAction::triggered, this, &QTextEdit::paste);

            QAction* del = menu.addAction("删除");
            del->setEnabled(textCursor().hasSelection() && !isReadOnly());
            connect(del, &QAction::triggered, this, [this]() {
                textCursor().removeSelectedText();
            });*/

            menu.addSeparator();

            QAction* selectAll = menu.addAction("全选");
            selectAll->setEnabled(!document()->isEmpty());
            connect(selectAll, &QAction::triggered, this, &QTextEdit::selectAll);

            menu.exec(event->globalPos());
        }
    };
}

#endif // MCHINESETEXTEDIT_H