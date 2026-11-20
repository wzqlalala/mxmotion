#include "mParamPanelBase.h"

#include <QPalette>

namespace MechMainWindow
{
    mPanelBase::mPanelBase(QWidget* parent)
        : QWidget(parent)
    {
        // 默认大小
        //setMinimumSize(200, 100);

        //this->setAttribute(Qt::WA_StyledBackground, true);
        //参数栏控件状态样式（单一来源）：三方参数面板继承本类即自动套用，
        //宿主祖先样式表盖掉应用级 QSS 的 :disabled 时本层兜底
        applyParamBarStyle(this);

        //ThemeManager 定位标记：切主题/切档时 applyGlobalRound 按属性找到本面板，
        //经 applyTheme 槽重发样式（Common 不反向链接 CommonWidgets）
        setProperty("mxParamPanel", true);

        //视口改名跟随注册表：构造自登记（析构注销，见 ~mPanelBase）
        registeredPanels().append(this);
    }

    mPanelBase::~mPanelBase()
    {
        registeredPanels().removeOne(this);
        qDeleteAll(m_shortcuts);

        for (auto foldWidget : m_foldWidgets)
        {
            delete foldWidget;
            foldWidget = nullptr;
        }
        m_foldWidgets.clear();
    }

    void mPanelBase::setTitle(const QString& title)
    {
        setWindowTitle(title);
    }

    void mPanelBase::applyTheme()
    {
        //主题/档位重发：按当前 UiPalette/UiMetrics 重新生成样式串，
        //内容未变时 applyParamBarStyle 内部跳过，不触发重抛光
        applyParamBarStyle(this);
    }

    void mPanelBase::onViewportRenamed(const QString& oldName, const QString& newName)
    {
        //空 oldName 无意义（不劫持尚未关联视口的面板），同名分发为无操作
        if (!oldName.isEmpty() && oldName != newName && _curViewName == oldName)
            _curViewName = newName;
    }

    void mPanelBase::setBackgroundColor(const QColor& color)
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, color);
        setAutoFillBackground(true);
        setPalette(pal);
    }

    void mPanelBase::bindShortcut(const QKeySequence& key, const std::function<void()>& callback)
    {
        QShortcut* shortcut = new QShortcut(key, this);
        connect(shortcut, &QShortcut::activated, this, callback);
        m_shortcuts.append(shortcut);
    }

    FoldSection* mPanelBase::getFoldSection(const QString& title)
    {
        for (auto foldWidget : m_foldWidgets)
        {
            if (foldWidget->getTitle() == title)
            {
                return foldWidget;
            }
        }
        return nullptr;
    }

    void mPanelBase::clearFoldWidgets()
    {
        for (auto foldWidget : m_foldWidgets)
        {
            if (foldWidget == nullptr)
            {
                continue;
            }
            //先清空行再删除控件（QWidget 析构会自动从父布局移除自身）
            foldWidget->removeAll();
            delete foldWidget;
        }
        m_foldWidgets.clear();
    }

    void mPanelBase::showWidget()
    {
        show();
    }

    void mPanelBase::hideWidget()
    {
        hide();
    }

    bool mPanelBase::setRowVisible(QWidget* w1, QWidget* w2, bool visible)
    {
        bool changed = false;
        if (w1 && w1->isVisible() != visible) {
            w1->setVisible(visible);
            changed = true;
        }
        if (w2 && w2->isVisible() != visible) {
            w2->setVisible(visible);
            changed = true;
        }
        return changed;
    }

    void mPanelBase::closeEvent(QCloseEvent* event)
    {
        QWidget::closeEvent(event); // 默认行为
    }
    void mPanelBase::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
    }

    void mPanelBase::hideEvent(QHideEvent* event)
    {
        QWidget::hideEvent(event);
    }

    QStringList mPanelBase::getSelectedList(QListWidget* listWidget)
    {
        QStringList strList;
        QList<QListWidgetItem*> items = listWidget->selectedItems();
        for (QListWidgetItem* item : items)
        {
            QString text = item->text();
            strList.append(text);
        }
        return strList;
    }

    void mPanelBase::clearListWidget(QListWidget* listwidget)
    {
        if (listwidget == nullptr)
        {
            return;
        }

        const int nrow = listwidget->count();
        for (int i = 0; i < nrow; ++i)
        {
            QListWidgetItem* tepItem = listwidget->item(i);
            if (tepItem != nullptr)
            {
                delete tepItem;
                tepItem = nullptr;
            }
        }
        listwidget->clear();
    }
}

namespace MechMainWindow
{
    //视口改名跟随注册表（分发见 mParamPanelBase.h 的 dispatchViewportRenamed）
    QList<mPanelBase*>& mPanelBase::registeredPanels()
    {
        static QList<mPanelBase*> s_panels;
        return s_panels;
    }
}
