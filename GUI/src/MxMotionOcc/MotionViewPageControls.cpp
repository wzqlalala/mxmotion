#include "MotionWindow.h"
#include "MotionViewPages.h"
#include "MotionWorkspace.h"
#include "ThemeManager.h"
#include "ViewportLayouts.h"
#include "mUiMenu.h"

#include <DockAreaWidget.h>
#include <DockWidget.h>
#include <QAction>
#include <QComboBox>
#include <QGridLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMouseEvent>
#include <QPersistentModelIndex>
#include <QSignalBlocker>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QWidgetAction>
#include <algorithm>
#include <functional>

namespace MotionOcc {
namespace {

constexpr int pageRole = Qt::UserRole;
constexpr int viewportRole = Qt::UserRole + 1;

class ViewPageTree final : public QTreeWidget {
public:
    using QTreeWidget::indexFromItem;

protected:
    QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& index, const QEvent* event) const override
    {
        if (event && event->type() == QEvent::MouseButtonPress
            && static_cast<const QMouseEvent*>(event)->button() == Qt::RightButton)
            return QItemSelectionModel::NoUpdate;
        return QTreeWidget::selectionCommand(index, event);
    }
};

QIcon viewIcon(const QString& file)
{
    return QIcon(QStringLiteral(":/mMainWidgets/Icon/CenterMainWindow/") + file);
}

QMenu* layoutPalette(QWidget* parent, const std::function<void(int)>& choose)
{
    const auto& layouts = MechMainWindow::viewLayoutTable();
    auto* menu = new MechMainWindow::mUiMenu(parent);
    auto* grid = new QWidget(menu);
    auto* layout = new QGridLayout(grid);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    for (int index = 0; index < layouts.size(); ++index) {
        auto* cell = new QToolButton(grid);
        const auto label = QString::fromUtf8(layouts[index].name);
        cell->setObjectName(QStringLiteral("viewportLayout_%1").arg(index));
        cell->setIcon(viewIcon(QString::fromUtf8(layouts[index].icon)));
        cell->setIconSize(QSize(36, 28));
        cell->setToolTip(label);
        cell->setAccessibleName(label);
        cell->setFixedSize(46, 34);
        cell->setAutoRaise(true);
        QObject::connect(cell, &QToolButton::clicked, menu, [menu, index, choose] {
            menu->close();
            choose(index);
        });
        layout->addWidget(cell, index / 4, index % 4);
    }
    auto* palette = new QWidgetAction(menu);
    palette->setDefaultWidget(grid);
    menu->addAction(palette);
    return menu;
}

} // namespace

QWidget* MotionWindow::createViewPages(QToolBar* toolbar, QWidget* parent)
{
    m_viewPageCombo = new QComboBox(toolbar);
    m_viewPageCombo->setObjectName(QStringLiteral("viewportPageCombo"));
    m_viewPageCombo->setMinimumWidth(105);
    connect(m_viewPageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this](int page) { m_viewManager->selectPage(page); });
    toolbar->addWidget(m_viewPageCombo);

    m_viewLayoutButton = new QToolButton(toolbar);
    m_viewLayoutButton->setObjectName(QStringLiteral("layoutDropButton"));
    m_viewLayoutButton->setToolTip(tr("视口布局"));
    m_viewLayoutButton->setAccessibleName(tr("视口布局"));
    m_viewLayoutButton->setAutoRaise(true);
    m_viewLayoutButton->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image:none; width:0px; }"));
    m_viewLayoutButton->setPopupMode(QToolButton::InstantPopup);
    m_viewLayoutButton->setMenu(layoutPalette(m_viewLayoutButton,
        [this](int index) { m_viewManager->setLayout(index); }));
    toolbar->addWidget(m_viewLayoutButton);

    m_viewSyncButton = new QToolButton(toolbar);
    m_viewSyncButton->setObjectName(QStringLiteral("viewportCameraSync"));
    m_viewSyncButton->setIcon(viewIcon(QStringLiteral("视口联动.svg")));
    m_viewSyncButton->setToolTip(tr("视口联动"));
    m_viewSyncButton->setAccessibleName(tr("视口联动"));
    m_viewSyncButton->setCheckable(true);
    m_viewSyncButton->setAutoRaise(true);
    connect(m_viewSyncButton, &QToolButton::toggled, this,
        [this](bool enabled) { m_viewManager->setLinked(enabled); });
    toolbar->addWidget(m_viewSyncButton);

    m_previousViewPage = toolbar->addAction(viewIcon(QStringLiteral("左箭头.svg")), tr("上一页"));
    m_previousViewPage->setObjectName(QStringLiteral("previousViewportPage"));
    m_viewPageCount = new QLabel(toolbar);
    m_viewPageCount->setObjectName(QStringLiteral("viewportPageCount"));
    m_viewPageCount->setAlignment(Qt::AlignCenter);
    m_viewPageCount->setMinimumWidth(28);
    toolbar->addWidget(m_viewPageCount);
    m_nextViewPage = toolbar->addAction(viewIcon(QStringLiteral("右箭头.svg")), tr("下一页"));
    m_nextViewPage->setObjectName(QStringLiteral("nextViewportPage"));
    connect(m_previousViewPage, &QAction::triggered, this, [this] {
        const int count = m_viewManager->pageCount();
        if (count > 1) m_viewManager->selectPage((m_viewManager->currentPage() + count - 1) % count);
    });
    connect(m_nextViewPage, &QAction::triggered, this, [this] {
        const int count = m_viewManager->pageCount();
        if (count > 1) m_viewManager->selectPage((m_viewManager->currentPage() + 1) % count);
    });

    return m_viewManager->createHost(*this, parent);
}

QWidget* MotionWindow::createViewPageTree()
{
    m_viewPageTree = new ViewPageTree;
    m_viewPageTree->setObjectName(QStringLiteral("viewPageTree"));
    m_viewPageTree->setAccessibleName(tr("布局树"));
    m_viewPageTree->setHeaderHidden(true);
    m_viewPageTree->setIndentation(14);
    m_viewPageTree->setUniformRowHeights(true);
    m_viewPageTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_viewPageTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_viewPageTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_viewPageTree, &QTreeWidget::customContextMenuRequested, this, &MotionWindow::showViewPageMenu);
    connect(m_viewPageTree, &QTreeWidget::itemSelectionChanged, this, [this] {
        const auto selected = m_viewPageTree->selectedItems();
        if (selected.size() == 1 && !selected.front()->parent())
            m_viewManager->selectPage(selected.front()->data(0, pageRole).toInt());
    });
    connect(m_viewPageTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        if (!item->parent()) return;
        const int viewport = item->data(0, viewportRole).toInt();
        m_viewManager->selectViewport(viewport);
        if (m_viewManager->activeViewport() != viewport) return;
        auto* model = m_workspace ? m_workspace->activeProject() : this;
        if (!model || !model->m_modelDock) return;
        model->m_modelDock->toggleView(true);
        if (auto* area = model->m_modelDock->dockAreaWidget()) area->setCurrentDockWidget(model->m_modelDock);
    });
    connect(m_viewPageTree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* item, int column) {
        if (column != 0) return;
        const int page = item->data(0, pageRole).toInt();
        const int viewport = item->data(0, viewportRole).toInt();
        const auto previous = item->parent() ? m_viewManager->viewportDisplayTitle(viewport) : m_viewManager->page(page).name;
        const auto name = item->text(0).trimmed();
        if (item->text(0) == previous) return;
        const auto matches = m_viewPageTree->findItems(name, Qt::MatchExactly | Qt::MatchRecursive, 0);
        const bool repeated = std::any_of(matches.begin(), matches.end(), [item](const QTreeWidgetItem* candidate) {
            return candidate != item && bool(candidate->parent()) == bool(item->parent());
        });
        if (name.isEmpty() || repeated) {
            const QSignalBlocker blocker(m_viewPageTree);
            item->setText(0, previous);
            setNotice(name.isEmpty() ? tr("名称不能为空。") : tr("该名称已存在。"), true);
            return;
        }
        if (item->parent()) m_viewManager->renameViewport(viewport, name);
        else m_viewManager->renamePage(page, name);
    });
    refreshViewPageControls(true);
    return m_viewPageTree;
}

void MotionWindow::refreshViewPageControls(bool rebuildTree)
{
    using namespace MechMainWindow;
    const int count = m_viewManager->pageCount();
    if (!count) return;
    const int current = m_viewManager->currentPage();
    const auto& page = m_viewManager->page(current);
    if (m_viewPageCombo) {
        const QSignalBlocker comboBlocker(m_viewPageCombo);
        if (rebuildTree || m_viewPageCombo->count() != count) {
            m_viewPageCombo->clear();
            for (int i = 0; i < count; ++i) m_viewPageCombo->addItem(m_viewManager->page(i).name);
        } else {
            for (int i = 0; i < count; ++i) m_viewPageCombo->setItemText(i, m_viewManager->page(i).name);
        }
        m_viewPageCombo->setCurrentIndex(current);
        m_viewPageCount->setText(QStringLiteral("%1/%2").arg(current + 1).arg(count));
        m_viewLayoutButton->setIcon(viewIcon(QString::fromUtf8(viewLayoutTable()[page.layout].icon)));
        m_previousViewPage->setEnabled(count > 1);
        m_nextViewPage->setEnabled(count > 1);
        const QSignalBlocker linkBlocker(m_viewSyncButton);
        m_viewSyncButton->setChecked(m_viewManager->linked());
    }
    if (!m_viewPageTree) return;
    const QSignalBlocker blocker(m_viewPageTree);
    if (rebuildTree || m_viewPageTree->topLevelItemCount() != count) {
        m_viewPageTree->clear();
        for (int i = 0; i < count; ++i) {
            const auto& treePage = m_viewManager->page(i);
            auto* item = new QTreeWidgetItem(m_viewPageTree, {treePage.name});
            item->setData(0, pageRole, i);
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            for (int id : treePage.viewports) {
                auto* child = new QTreeWidgetItem(item, {m_viewManager->viewportDisplayTitle(id)});
                child->setData(0, pageRole, i);
                child->setData(0, viewportRole, id);
                child->setIcon(0, viewIcon(QStringLiteral("1.svg")));
                child->setFlags(child->flags() | Qt::ItemIsEditable);
            }
        }
        m_viewPageTree->expandAll();
    }
    const int activeViewport = m_viewManager->activeViewport();
    QTreeWidgetItem* selection = nullptr;
    for (int i = 0; i < m_viewPageTree->topLevelItemCount(); ++i) {
        auto* item = m_viewPageTree->topLevelItem(i);
        const auto& page = m_viewManager->page(i);
        const auto& preset = viewLayoutTable()[page.layout];
        item->setText(0, page.name);
        item->setIcon(0, viewIcon(QString::fromUtf8(preset.icon)));
        item->setToolTip(0, QString::fromUtf8(preset.name));
        if (i == current && activeViewport < 0) selection = item;
        for (int index = 0; index < item->childCount(); ++index) {
            auto* child = item->child(index);
            const int id = child->data(0, viewportRole).toInt();
            const bool active = id == activeViewport;
            child->setText(0, m_viewManager->viewportDisplayTitle(id));
            auto font = child->font(0);
            font.setBold(active);
            child->setFont(0, font);
            if (active) selection = child;
        }
    }
    if (selection && (activeViewport < 0 || m_viewPageTree->selectedItems().size() <= 1))
        m_viewPageTree->setCurrentItem(selection);
}

void MotionWindow::showViewPageMenu(const QPoint& position)
{
    using namespace MechMainWindow;
    auto* item = m_viewPageTree->itemAt(position);
    if (!item) return;
    if (!item->isSelected()) {
        const QSignalBlocker blocker(m_viewPageTree);
        m_viewPageTree->setCurrentItem(item, 0, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    const QPersistentModelIndex target(static_cast<ViewPageTree*>(m_viewPageTree)->indexFromItem(item, 0));
    const auto menuPosition = m_viewPageTree->viewport()->mapToGlobal(position);
    const QIcon deleteIcon(QStringLiteral(":/mMainWidgets/Icon/BrowserTree/shanchu.png"));
    const QIcon renameIcon(QStringLiteral(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"));
    mUiMenu menu(m_viewPageTree);
    ThemeManager::applyMenuStyle(&menu);

    if (!item->parent()) {
        auto* addView = menu.addAction(QIcon(QStringLiteral(":/mMainWidgets/Icon/homepage/xinjian-23x.png")), tr("新建视口"));
        auto* addPage = menu.addAction(QIcon(QStringLiteral(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png")), tr("新建页"));
        auto* remove = menu.addAction(deleteIcon, tr("删除页"));
        auto* rename = menu.addAction(renameIcon, tr("重命名"));
        remove->setEnabled(m_viewManager->pageCount() > 1);
        auto* action = menu.exec(menuPosition);
        if (!action || !target.isValid()) return;
        const int pageIndex = target.data(pageRole).toInt();
        if (action == addView) m_viewManager->addViewport(pageIndex, *this);
        else if (action == addPage) m_viewManager->addLayout(0);
        else if (action == remove) m_viewManager->removePage(pageIndex);
        else if (action == rename) m_viewPageTree->edit(target);
        return;
    }

    const int id = item->data(0, viewportRole).toInt();
    auto* remove = menu.addAction(deleteIcon, tr("删除视口"));
    auto* rename = menu.addAction(renameIcon, tr("重命名"));
    auto* tile = menu.addAction(tr("布局"));
    auto* deleteModel = m_workspace ? menu.addAction(deleteIcon, tr("删除模型")) : nullptr;
    remove->setEnabled(m_viewManager->canRemoveViewport(id));
    auto* action = menu.exec(menuPosition);
    if (!action || !target.isValid()) return;
    if (action == remove) m_viewManager->removeViewport(id);
    else if (action == rename) m_viewPageTree->edit(target);
    else if (action == tile) {
        const int pageIndex = target.data(pageRole).toInt();
        QList<int> chosen;
        for (const auto* selected : m_viewPageTree->selectedItems()) {
            if (!selected->parent()) continue;
            if (selected->data(0, pageRole).toInt() != pageIndex) {
                setNotice(tr("选中的视口不属于同一页，无法布局。"), true);
                return;
            }
            chosen.append(selected->data(0, viewportRole).toInt());
        }
        const int slotCount = viewLayoutSlotCount(viewLayoutTable()[m_viewManager->page(pageIndex).layout]);
        if (chosen.size() != slotCount)
            setNotice(tr("选中视口数（%1）与当前布局格数（%2）不一致，按选中顺序布局。").arg(chosen.size()).arg(slotCount));
        m_viewManager->tile(pageIndex, chosen);
    } else if (action == deleteModel) {
        m_viewManager->removeViewportModel(id);
    }
}

} // namespace MotionOcc
