#include "mViewBrowserWindow.h"
#include "mTreeDelegate.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"
#include "mViewportModelManager.h"
#include "ViewportContainer.h"
#include "mViewportModel.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mBrowserWindow.h"
#include "mTreeView.h"
#include "mMechMainWindow.h"   //双击图表卡片节点的溢出页替换呈现
#include "mFramelessMessageBox.h"	//删除模型确认框（自绘标题栏，与模型树一致）
#include "CurveView/mCurveViewportFactory.h"   //布局应用后物化本页图表卡片（懒加载，对齐 HyperViewer"切到才渲染"）
#include "CurveCore/mCurveDocController.h"   //视图树删除/改名曲线图表卡片：走曲线文档命令（撤销栈管理）
#include "mUiMenu.h"
#include "ThemeManager.h"

//Qt
#include <QMenu>
#include <QApplication>
#include <QVBoxLayout>
#include <QCursor>
#include <QFont>

using namespace MxFunctions;
using namespace MechPreData;
namespace MechMainWindow
{
    //////////////////////////////////////////////////////////////////////////
    // 视口布局表与条带几何
    //////////////////////////////////////////////////////////////////////////
    const QVector<mViewLayoutDef>& viewLayoutTable()
    {
        //verticalBands: true=条带纵向堆叠(行)；false=条带横向排列(列)
        static const QVector<mViewLayoutDef> table = {
            { "1.svg",           "单视口",          true,  {1}, 1 },
            { "2@横分.svg",      "2视口-横分",      true,  {1,1}, 2 },
            { "2@纵分.svg",      "2视口-纵分",      false, {1,1}, 2 },
            { "3@上1下2.svg",    "3视口-上1下2",    true,  {1,2}, 2 },
            { "3@上2下1.svg",    "3视口-上2下1",    true,  {2,1}, 2 },
            { "3@左1右2.svg",    "3视口-左1右2",    false, {1,2}, 2 },
            { "3@左2右1.svg",    "3视口-左2右1",    false, {2,1}, 2 },
            { "3@横分.svg",      "3视口-横分",      true,  {1,1,1}, 3 },
            { "3@纵分.svg",      "3视口-纵分",      false, {1,1,1}, 3 },
            { "4@均分.svg",      "4视口-均分",      true,  {2,2}, 2 },
            { "4@横分.svg",      "4视口-横分",      true,  {1,1,1,1}, 4 },
            { "4@纵分.svg",      "4视口-纵分",      false, {1,1,1,1}, 4 },
            { "6@上3下3.svg",    "6视口-上3下3",    true,  {3,3}, 2 },
            { "6@左3右3.svg",    "6视口-左3右3",    false, {3,3}, 2 },
            { "8@上4下4.svg",    "8视口-上4下4",    true,  {4,4}, 2 },
            { "8@左4右4.svg",    "8视口-左4右4",    false, {4,4}, 2 },
            { "9@均分.svg",      "9视口-均分",      true,  {3,3,3}, 3 },
            { "12@上4中4下4.svg","12视口-上4中4下4",true,  {4,4,4}, 3 },
            { "12@左4中4右4.svg","12视口-左4中4右4",false, {4,4,4}, 3 },
            { "16@均分.svg",     "16视口-均分",     true,  {4,4,4,4}, 4 },
        };
        return table;
    }

    int viewLayoutSlotCount(const mViewLayoutDef& def)
    {
        int n = 0;
        for (int b = 0; b < def.bandCount; ++b) n += def.bands[b];
        return n;
    }

    QRect viewLayoutCellRect(const mViewLayoutDef& def, int cellIndex, const QSize& area)
    {
        //定位条带与带内序号
        int before = 0, band = 0, inBand = 0;
        for (int b = 0; b < def.bandCount; ++b)
        {
            if (cellIndex < before + def.bands[b])
            {
                band = b;
                inBand = cellIndex - before;
                break;
            }
            before += def.bands[b];
        }
        if (def.verticalBands)
        {
            //行条带：等分高度；带内格子等分宽度横向排列
            const int bh = area.height() / def.bandCount;
            const int cw = area.width() / def.bands[band];
            return QRect(inBand * cw, band * bh, cw, bh);
        }
        //列条带：等分宽度；带内格子等分高度纵向堆叠
        const int bw = area.width() / def.bandCount;
        const int ch = area.height() / def.bands[band];
        return QRect(band * bw, inBand * ch, bw, ch);
    }

    //////////////////////////////////////////////////////////////////////////
    // mViewTreeModel
    //////////////////////////////////////////////////////////////////////////
    mViewTreeModel::mViewTreeModel(QObject* parent) : QAbstractItemModel(parent)
    {
        m_root = new Item;
        //默认页：页-1
        addPage();
    }

    mViewTreeModel::~mViewTreeModel()
    {
        delete m_root;
    }

    void mViewTreeModel::clearAllPages()
    {
        beginResetModel();
        for (Item* c : m_root->children) delete c;
        m_root->children.clear();
        endResetModel();
        emit sig_pagesChanged();
    }

    mViewTreeModel::Item* mViewTreeModel::itemFromIndex(const QModelIndex& idx) const
    {
        return idx.isValid() ? static_cast<Item*>(idx.internalPointer()) : m_root;
    }

    QModelIndex mViewTreeModel::indexFromItem(Item* item) const
    {
        if (!item || item == m_root) return {};
        Item* p = item->parent;
        if (!p) return {};
        int row = p->children.indexOf(item);
        if (row < 0) return {};
        return createIndex(row, 0, item);
    }

    QModelIndex mViewTreeModel::index(int row, int col, const QModelIndex& parent) const
    {
        if (col != 0 || row < 0) return {};
        Item* p = itemFromIndex(parent);
        if (!p || row >= p->children.size()) return {};
        return createIndex(row, 0, p->children[row]);
    }

    QModelIndex mViewTreeModel::parent(const QModelIndex& child) const
    {
        Item* c = itemFromIndex(child);
        Item* p = c ? c->parent : nullptr;
        if (!p || p == m_root) return {};
        return indexFromItem(p);
    }

    int mViewTreeModel::rowCount(const QModelIndex& parent) const
    {
        Item* p = itemFromIndex(parent);
        return p ? p->children.size() : 0;
    }

    QVariant mViewTreeModel::data(const QModelIndex& idx, int role) const
    {
        Item* it = itemFromIndex(idx);
        if (it == m_root) return {};
        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return it->text;
        case Qt::FontRole:
        {
            //当前激活视口节点文本加粗
            if (it->type == ViewTreeTypes::ViewTreeViewportItem && it->active)
            {
                QFont f;
                f.setBold(true);
                return f;
            }
            return {};
        }
        case Roles::TypeRole:
            return it->type;
        case Roles::ModelRole:
            return it->modelID;
        case Roles::PhyidRole:
            return it->id;
        case Roles::CaseidRole:
            return 0;
        case Roles::HiddenRole:
            return false;
        case Roles::StatusRole:
            return (int)Tree_Yes;
        case Roles::Icon2Role:
            return it->type == ViewTreeTypes::ViewTreePageItem
                ? ":/mMainWidgets/Icon/BrowserTree/jihe.png"
                : ":/mMainWidgets/Icon/BrowserTree/moxing3x.png";
        default:
            return {};
        }
    }

    Qt::ItemFlags mViewTreeModel::flags(const QModelIndex& idx) const
    {
        if (!idx.isValid()) return Qt::NoItemFlags;
        //页节点与视口子节点均支持原地重命名（与前处理树操作方式一致）
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    bool mViewTreeModel::setData(const QModelIndex& idx, const QVariant& value, int role)
    {
        if (role != Qt::EditRole || !idx.isValid()) return false;
        Item* it = itemFromIndex(idx);
        if (!it || it == m_root) return false;

        QString newName = value.toString();
        if (newName.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, "名称不能为空！");
            return false;
        }

        if (it->type == ViewTreeTypes::ViewTreePageItem)
        {
            for (const auto& page : m_root->children)
            {
                if (page != it && page->text == newName)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, "该名称已存在！");
                    return false;
                }
            }
            it->text = newName;
            emit sig_pagesChanged();
        }
        else if (it->type == ViewTreeTypes::ViewTreeViewportItem)
        {
            //曲线图表卡片（懒加载登记，管理器无此登记表）：改名走曲线文档 kName
            //命令（撤销栈管理，卡片标题经工厂 retitle 链联动刷新）——原实现对
            //卡片项 getViewportModel(-1) 落空，误报"该名称已存在！"且改名失败
            const int chartId = PostCurve::mCurveViewportFactory::getInstance()->chartIdForCard(it->id);
            if (chartId >= 0)
            {
                PostCurve::mCurveDocController::getInstance()->setNodeProperty(
                    chartId, PostCurve::CurveProp::kName, newName);
                it->text = newName;
            }
            else
            {
                //同步视口数据名、标题栏显示名与后处理渲染窗口查找键（模型内重名返回false）
                //byUser=true：标记为用户手动改名，之后模型重命名不再联动替换其前缀
                auto viewportModel = VMManager->getViewportModel(it->modelID);
                if (!viewportModel || !viewportModel->renameViewport(it->id, newName, true))
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, "该名称已存在！");
                    return false;
                }
                it->text = newName;
            }
        }
        else
        {
            return false;
        }

        emit dataChanged(idx, idx, { Qt::DisplayRole });
        return true;
    }

    QModelIndex mViewTreeModel::addPage(const QString& name)
    {
        int pageID = m_nextPageID++;
        QString pageName = name;
        if (pageName.isEmpty())
        {
            int maxIndex = 0;
            for (const auto& it : m_root->children)
            {
                if (it->text.contains("-"))
                {
                    QStringList list = it->text.split("-");
                    if (list.size() > 1 && list[1].toInt() > maxIndex)
                    {
                        maxIndex = list[1].toInt();
                    }
                }
            }
            pageName = QString("视图页-%1").arg(maxIndex + 1);
        }

        const int first = m_root->children.size();
        beginInsertRows({}, first, first);
        Item* page = new Item;
        page->type = ViewTreeTypes::ViewTreePageItem;
        page->id = pageID;
        page->text = pageName;
        page->parent = m_root;
        m_root->children.push_back(page);
        endInsertRows();
        emit sig_pagesChanged();
        return createIndex(first, 0, page);
    }

    bool mViewTreeModel::removePage(const QModelIndex& pageIdx)
    {
        Item* page = itemFromIndex(pageIdx);
        if (!page || page == m_root || page->type != ViewTreeTypes::ViewTreePageItem) return false;
        if (!page->children.isEmpty()) return false;

        QModelIndex parentIdx = parent(pageIdx);
        int row = pageIdx.row();
        beginRemoveRows(parentIdx, row, row);
        page->parent->children.removeAt(row);
        delete page;
        endRemoveRows();
        emit sig_pagesChanged();
        return true;
    }

    QModelIndex mViewTreeModel::findPageIndex(int pageID) const
    {
        for (int i = 0; i < m_root->children.size(); ++i)
        {
            if (m_root->children[i]->type == ViewTreeTypes::ViewTreePageItem && m_root->children[i]->id == pageID)
                return createIndex(i, 0, m_root->children[i]);
        }
        return {};
    }

    QModelIndex mViewTreeModel::findViewportIndex(int viewportID) const
    {
        for (auto* page : m_root->children)
        {
            for (int i = 0; i < page->children.size(); ++i)
            {
                if (page->children[i]->id == viewportID)
                    return createIndex(i, 0, page->children[i]);
            }
        }
        return {};
    }

    bool mViewTreeModel::addViewportToPage(int pageID, int viewportID, int modelID, const QString& name)
    {
        if (findViewportIndex(viewportID).isValid()) return false; //同一视口只登记一次

        QModelIndex pageIdx = findPageIndex(pageID);
        Item* page = itemFromIndex(pageIdx);
        if (!page || page->type != ViewTreeTypes::ViewTreePageItem) return false;

        const int first = page->children.size();
        beginInsertRows(pageIdx, first, first);
        Item* node = new Item;
        node->type = ViewTreeTypes::ViewTreeViewportItem;
        node->id = viewportID;
        node->modelID = modelID;
        node->text = name.isEmpty() ? QString("视口-%1").arg(viewportID) : name;
        node->parent = page;
        page->children.push_back(node);
        endInsertRows();
        return true;
    }

    bool mViewTreeModel::insertViewportToPage(int pageID, int row, int viewportID, int modelID, const QString& name)
    {
        if (findViewportIndex(viewportID).isValid()) return false; //同一视口只登记一次

        QModelIndex pageIdx = findPageIndex(pageID);
        Item* page = itemFromIndex(pageIdx);
        if (!page || page->type != ViewTreeTypes::ViewTreePageItem) return false;

        const int pos = qBound(0, row, page->children.size());
        beginInsertRows(pageIdx, pos, pos);
        Item* node = new Item;
        node->type = ViewTreeTypes::ViewTreeViewportItem;
        node->id = viewportID;
        node->modelID = modelID;
        node->text = name.isEmpty() ? QString("视口-%1").arg(viewportID) : name;
        node->parent = page;
        page->children.insert(pos, node);
        endInsertRows();
        return true;
    }

    bool mViewTreeModel::renameViewportNode(int viewportID, const QString& name)
    {
        QModelIndex idx = findViewportIndex(viewportID);
        if (!idx.isValid() || name.isEmpty()) return false;
        //直接更新文本：不走 setData(EditRole)，避免再次触发视口重命名校验/同步的递归
        Item* it = itemFromIndex(idx);
        if (!it || it->type != ViewTreeTypes::ViewTreeViewportItem) return false;
        it->text = name;
        emit dataChanged(idx, idx, { Qt::DisplayRole, Qt::EditRole });
        return true;
    }

    bool mViewTreeModel::viewportNodeInfo(int viewportID, int& modelID, QString& name) const
    {
        QModelIndex idx = findViewportIndex(viewportID);
        if (!idx.isValid()) return false;
        Item* it = itemFromIndex(idx);
        if (!it || it->type != ViewTreeTypes::ViewTreeViewportItem) return false;
        modelID = it->modelID;
        name = it->text;
        return true;
    }

    bool mViewTreeModel::removeViewport(int viewportID)
    {
        QModelIndex idx = findViewportIndex(viewportID);
        if (!idx.isValid()) return false;

        Item* node = itemFromIndex(idx);
        QModelIndex parentIdx = parent(idx);
        int row = idx.row();
        beginRemoveRows(parentIdx, row, row);
        node->parent->children.removeAt(row);
        delete node;
        endRemoveRows();
        return true;
    }

    QList<int> mViewTreeModel::pageIDs() const
    {
        QList<int> ids;
        for (const auto& it : m_root->children)
        {
            if (it->type == ViewTreeTypes::ViewTreePageItem)
                ids.push_back(it->id);
        }
        return ids;
    }

    QList<QPair<QString, int>> mViewTreeModel::pageList() const
    {
        QList<QPair<QString, int>> pages;
        for (const auto& it : m_root->children)
        {
            if (it->type == ViewTreeTypes::ViewTreePageItem)
                pages.push_back(qMakePair(it->text, it->id));
        }
        return pages;
    }

    int mViewTreeModel::pageLayout(int pageID) const
    {
        for (const auto& it : m_root->children)
        {
            if (it->type == ViewTreeTypes::ViewTreePageItem && it->id == pageID)
                return it->layout;
        }
        return 0;
    }

    bool mViewTreeModel::setPageLayout(int pageID, int slotCount)
    {
        for (auto& it : m_root->children)
        {
            if (it->type == ViewTreeTypes::ViewTreePageItem && it->id == pageID)
            {
                if (it->layout == slotCount) return false;
                it->layout = slotCount;
                //布局下标变更=用户显式更改布局：手动布局可见组失效，回到按节点序填格
                //（"除非更改布局否则保持该布局设置"）
                it->visibleViewportIDs.clear();
                emit sig_pagesChanged();
                return true;
            }
        }
        return false;
    }

    bool mViewTreeModel::swapViewportNodes(int pageID, int viewportA, int viewportB)
    {
        QModelIndex pageIdx = findPageIndex(pageID);
        if (!pageIdx.isValid()) return false;
        Item* page = itemFromIndex(pageIdx);
        if (!page || page == m_root) return false;
        //定位两视口节点当前行（同页内交换）
        int rowA = -1, rowB = -1;
        for (int i = 0; i < page->children.size(); ++i)
        {
            Item* c = page->children[i];
            if (c->type != ViewTreeTypes::ViewTreeViewportItem) continue;
            if (c->id == viewportA) rowA = i;
            if (c->id == viewportB) rowB = i;
        }
        if (rowA < 0 || rowB < 0 || rowA == rowB) return false;
        if (rowA > rowB)
        {
            const int tmp = rowA; rowA = rowB; rowB = tmp;  //统一为 rowA 在上
        }
        const QModelIndex pIdx = indexFromItem(page);
        //两次行移动完成交换（保留视图选中/展开状态）：下方节点先上移到上方位置，
        //上方节点顺移到 rowA+1；相邻时一步即完成交换，否则再把它下移回原下方位置。
        //beginMoveRows 同父约束：目的行不得落在 [first, last+1]，故上移目的行=目标行、
        //下移目的行=目标行+1
        beginMoveRows(pIdx, rowB, rowB, pIdx, rowA);
        Item* lower = page->children.takeAt(rowB);
        page->children.insert(rowA, lower);
        endMoveRows();
        if (rowB - rowA > 1)
        {
            beginMoveRows(pIdx, rowA + 1, rowA + 1, pIdx, rowB + 1);
            Item* upper = page->children.takeAt(rowA + 1);
            page->children.insert(rowB, upper);
            endMoveRows();
        }
        //节点序变化影响布局填格顺序，通知显示栏刷新
        emit sig_pagesChanged();
        return true;
    }

    QList<int> mViewTreeModel::pageVisibleViewportIDs(int pageID) const
    {
        for (const auto& it : m_root->children)
        {
            if (it->type == ViewTreeTypes::ViewTreePageItem && it->id == pageID)
                return it->visibleViewportIDs;
        }
        return {};
    }

    bool mViewTreeModel::setPageVisibleViewportIDs(int pageID, const QList<int>& ids)
    {
        for (const auto& it : m_root->children)
        {
            if (it->type == ViewTreeTypes::ViewTreePageItem && it->id == pageID)
            {
                it->visibleViewportIDs = ids;
                //组变化影响呈现（视图显示栏/容器重排均由树驱动）
                emit sig_pagesChanged();
                return true;
            }
        }
        return false;
    }

    QList<int> mViewTreeModel::pageViewportIDs(int pageID) const
    {
        QList<int> ids;
        for (const auto& page : m_root->children)
        {
            if (page->type == ViewTreeTypes::ViewTreePageItem && page->id == pageID)
            {
                for (const auto& node : page->children)
                    ids.push_back(node->id);
                break;
            }
        }
        return ids;
    }

    void mViewTreeModel::setActiveViewportNode(int viewportID)
    {
        //遍历全部视口节点：目标加粗，其余取消
        std::function<void(Item*)> walk = [&](Item* item) {
            if (!item) return;
            if (item->type == ViewTreeTypes::ViewTreeViewportItem)
            {
                bool shouldActive = (item->id == viewportID);
                if (item->active != shouldActive)
                {
                    item->active = shouldActive;
                    QModelIndex idx = indexFromItem(item);
                    if (idx.isValid())
                    {
                        emit dataChanged(idx, idx, { Qt::FontRole });
                    }
                }
            }
            for (Item* c : item->children) walk(c);
        };
        walk(m_root);
    }

    //////////////////////////////////////////////////////////////////////////
    // mViewTreeView
    //////////////////////////////////////////////////////////////////////////
    mViewTreeView::mViewTreeView(QWidget* parent) : QTreeView(parent)
    {
        //视图级样式与前处理树保持一致
        setIndentation(14);      // 与 PaintDashBranches 箭头口径配套（箭头对齐父行状态符号列且贴近自身内容）
        setMouseTracking(true);  // 启用悬浮事件
        viewport()->setMouseTracking(true);
        viewport()->setAttribute(Qt::WA_Hover);  // 启用 hover 属性
        setStyleSheet(R"(
QTreeView{
border:none;
}
)");
        QPalette pal = this->palette();
        pal.setColor(QPalette::Highlight, Qt::white);        // 选中背景变白（选中样式由委托绘制）
        this->setPalette(pal);

        _model = new mViewTreeModel(this);
        setModel(_model);
        auto* delegate = new mTreeDelegate(this);
        delegate->setShowStatusIcon(false);//视图树无状态语义，不绘制状态勾选图标
        setItemDelegate(delegate);
        setUniformRowHeights(true);
        setAnimated(false);
        setHeaderHidden(true);
        setContextMenuPolicy(Qt::CustomContextMenu);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        //支持 Ctrl/Shift 多选（用于"布局"等批量操作）；
        //单击选中/双击激活的语义不受多选影响（slot_selectionChanged 只响应单选场景）
        setSelectionMode(QAbstractItemView::ExtendedSelection);

        expandAll();

        connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mViewTreeView::slot_selectionChanged);
        //双击视口子节点才切换模型与视口（单击仅选中）
        connect(this, &QTreeView::doubleClicked, this, &mViewTreeView::slot_doubleClicked);
        connect(this, &QTreeView::customContextMenuRequested, this, [this]() { slot_rightClick(); });

        //页增删/重命名/布局变化 -> 中继给视图显示栏刷新
        connect(_model, &mViewTreeModel::sig_pagesChanged, this, [this]() {
            emit sig_pageStateChanged();
            });

        //视口增删与激活联动
        connect(VMManager, &MechPreData::mViewportModelManager::sig_viewportAdded, this, &mViewTreeView::slot_viewportAdded, Qt::UniqueConnection);
        connect(VMManager, &MechPreData::mViewportModelManager::sig_viewportRemoved, this, &mViewTreeView::slot_viewportRemoved, Qt::UniqueConnection);
        connect(VMManager, &MechPreData::mViewportModelManager::sig_viewportRenamed, this, &mViewTreeView::slot_viewportRenamed, Qt::UniqueConnection);
        connect(VMManager, &MechPreData::mViewportModelManager::currentViewportChanged, this, &mViewTreeView::slot_currentViewportChanged);

        //登记窗口构造前已存在的视口
        syncFromViewports();
    }

    void mViewTreeView::suspendViewportSync()
    {
        if (_viewportSyncSuspended) return;
        _viewportSyncSuspended = true;
        disconnect(VMManager, &MechPreData::mViewportModelManager::sig_viewportAdded, this, &mViewTreeView::slot_viewportAdded);
        disconnect(VMManager, &MechPreData::mViewportModelManager::sig_viewportRemoved, this, &mViewTreeView::slot_viewportRemoved);
        disconnect(VMManager, &MechPreData::mViewportModelManager::sig_viewportRenamed, this, &mViewTreeView::slot_viewportRenamed);
        disconnect(VMManager, &MechPreData::mViewportModelManager::currentViewportChanged, this, &mViewTreeView::slot_currentViewportChanged);
    }

    void mViewTreeView::resumeViewportSync()
    {
        if (!_viewportSyncSuspended) return;
        _viewportSyncSuspended = false;
        connect(VMManager, &MechPreData::mViewportModelManager::sig_viewportAdded, this, &mViewTreeView::slot_viewportAdded, Qt::UniqueConnection);
        connect(VMManager, &MechPreData::mViewportModelManager::sig_viewportRemoved, this, &mViewTreeView::slot_viewportRemoved, Qt::UniqueConnection);
        connect(VMManager, &MechPreData::mViewportModelManager::sig_viewportRenamed, this, &mViewTreeView::slot_viewportRenamed, Qt::UniqueConnection);
        connect(VMManager, &MechPreData::mViewportModelManager::currentViewportChanged, this, &mViewTreeView::slot_currentViewportChanged, Qt::UniqueConnection);
    }

    void mViewTreeView::syncFromViewports()
    {
        for (int modelID : VMManager->getAllModelDataID())
        {
            auto viewportModel = VMManager->getViewportModel(modelID);
            if (!viewportModel) continue;
            const QList<QWidget*> subWindows = viewportModel->getSubWindows();
            for (auto sub : subWindows)
            {
                auto priv = viewportModel->getViewportPrivate(sub);
                if (!priv) continue;
                if (_model->findViewportIndex(priv->getViewportID()).isValid()) continue;
                int pageID = _lastPageID;
                if (!_model->findPageIndex(pageID).isValid())
                {
                    _model->addPage();
                    pageID = _model->pageIDs().last();
                }
                _model->addViewportToPage(pageID, priv->getViewportID(), modelID, priv->getViewportName());
            }
        }
        expandAll();
        //登记完成后按当前页布局排列（构造期首个视口已在 mCenterMainWindow 登记到容器，
        //此处补一次布局确保几何正确）
        applyPageLayout(_lastPageID);
        //激活首个视口：管理器构造期 newViewport 未走容器激活路径，
        //此处补一次确保启动后 getCVM/当前视口有效，并标记为当前激活（加粗）
        QList<int> ids = _model->pageViewportIDs(_lastPageID);
        if (!ids.isEmpty())
        {
            VMManager->activateViewportByID(ids.first());
            _model->setActiveViewportNode(ids.first());
        }
    }

    void mViewTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
    {
        //父子节点虚线连接 + 展开箭头（与前处理树一致），原生箭头经离屏渲染后贴近节点
        PaintDashBranches(this, painter, rect, index,
            [this](QPainter* p, const QRect& r, const QModelIndex& i) { QTreeView::drawBranches(p, r, i); });
    }

    QItemSelectionModel::SelectionFlags mViewTreeView::selectionCommand(const QModelIndex& index, const QEvent* event) const
    {
        //右键不选中、不激活；目标节点由 slot_rightClick 程序化选中（抑制激活）后弹菜单
        if (event && event->type() == QEvent::MouseButtonPress)
        {
            const auto* me = static_cast<const QMouseEvent*>(event);
            if (me->button() == Qt::RightButton)
            {
                return QItemSelectionModel::NoUpdate;
            }
        }
        return QTreeView::selectionCommand(index, event);
    }

    void mViewTreeView::slot_selectionChanged()
    {
        //右键菜单触发的程序化选中不激活视口（激活只在左键点击/联动选中时发生）
        if (_suppressActivate)
        {
            return;
        }
        QModelIndexList idxs = selectionModel()->selectedRows();
        if (idxs.size() != 1) return;
        const int nodeType = _model->data(idxs[0], Roles::TypeRole).toInt();

        if (nodeType == MechPreData::ViewTreeTypes::ViewTreePageItem && !_switchingPage)
        {
            //点击页节点 = 切换到该页
            switchToPage(_model->data(idxs[0], Roles::PhyidRole).toInt());
            return;
        }
        if (nodeType != MechPreData::ViewTreeTypes::ViewTreeViewportItem) return;

        //单击视口节点仅选中，不切换模型与视口（双击才切换，见 slot_doubleClicked）
    }

    void mViewTreeView::slot_doubleClicked(const QModelIndex& idx)
    {
        if (!idx.isValid()) return;
        const int nodeType = _model->data(idx, Roles::TypeRole).toInt();
        if (nodeType != MechPreData::ViewTreeTypes::ViewTreeViewportItem) return;

        int viewportID = _model->data(idx, Roles::PhyidRole).toInt();
        int modelID = _model->data(idx, Roles::ModelRole).toInt();

        //双击其他页的视口节点：先切到该页，再激活所点视口
        if (idx.parent().isValid())
        {
            const int pageID = _model->data(idx.parent(), Roles::PhyidRole).toInt();
            if (pageID != _lastPageID)
            {
                _lastPageID = pageID;
                applyPageLayout(pageID);
                emit sig_pageStateChanged();
            }
        }

        auto priv = VMManager->getViewportPrivateByID(viewportID);
        if (priv == nullptr)
        {
            //图表卡片节点（独立ID段挂进容器，不进渲染视口注册表）：激活卡片并显式进入
            //曲线上下文——切曲线树+菜单切曲线模式（“曲线”专属曲线模式，经菜单栏的
            //跨模式切换语义任何模式下都会切入；卡片激活不触发currentViewportChanged，
            //渲染视口驱动的自动联动对它不生效，须在此补齐）
            if (auto* container = VMManager->getViewportContainer())
            {
                //切换前选中的视口=溢出页替换呈现目标（激活后容器内已是新卡片，先取）
                const int prevViewportId = container->activeViewportID();
                container->setActiveViewport(viewportID);
                //双击图表卡片节点同样发起溢出页"选中即替换呈现"（与曲线树双击同语义）
                mMechMainWindow::presentChartOnOverflowPage(viewportID, prevViewportId);
            }
            mDockLayoutManager::getInstance()->showTreeDockForType(MxFunctions::ViewportCurve);
            emit MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("曲线");
            return;
        }

        activateViewportNode(viewportID, modelID);

        //双击视口节点：跳转到该视口类型对应的树窗口标签（模型/云图/曲线）
        mDockLayoutManager::getInstance()->showTreeDockForType(priv->getViewportType());
    }

    void mViewTreeView::activateViewportNode(int viewportID, int modelDataID)
    {
        auto priv = VMManager->getViewportPrivateByID(viewportID);
        if (!priv) return;
        if (!getCVM) return;
        if (modelDataID != getCVM->getModelData()->getModelDataID())
        {
            VMManager->setCurrentViewportModel(modelDataID);
        }
        //激活交由容器完成（sig_activeViewportChanged 驱动管理器恢复当前模型/视口）
        VMManager->activateViewportByID(viewportID);
    }

    int mViewTreeView::currentPageID()
    {
        QModelIndexList idxs = selectionModel()->selectedRows();
        for (const auto& idx : idxs)
        {
            int type = _model->data(idx, Roles::TypeRole).toInt();
            if (type == MechPreData::ViewTreeTypes::ViewTreePageItem)
            {
                return _model->data(idx, Roles::PhyidRole).toInt();
            }
            if (type == MechPreData::ViewTreeTypes::ViewTreeViewportItem && idx.parent().isValid())
            {
                return _model->data(idx.parent(), Roles::PhyidRole).toInt();
            }
        }
        return _lastPageID;
    }

    void mViewTreeView::slot_viewportAdded(int viewportID)
    {
        int modelID = -1;
        auto priv = VMManager->getViewportPrivateByID(viewportID, &modelID);
        if (!priv || modelID == -1) return;

        //新视口放入：当前选中页（或视口节点所在页），否则最近使用的页
        int pageID = currentPageID();
        if (!_model->findPageIndex(pageID).isValid())
        {
            _model->addPage();
            pageID = _model->pageIDs().last();
        }
        _lastPageID = pageID;
        _model->addViewportToPage(pageID, viewportID, modelID, priv->getViewportName());

        QModelIndex pageIdx = _model->findPageIndex(pageID);
        if (pageIdx.isValid()) expand(pageIdx);

        //总是重新应用当前页布局：落入当前页则按布局排列（覆盖默认居中几何），
        //落入其他页则把新视口隐藏（非本页视口不显示）
        applyPageLayout(_lastPageID);

        //落入当前页时高亮新节点（激活已完成，选中不会再触发跳转）
        if (pageID == _lastPageID)
        {
            selectViewportNode(viewportID);
        }
    }

    void mViewTreeView::slot_viewportRemoved(int viewportID)
    {
        //删除前记录该视口所属页与是否为激活视口
        QModelIndex idx = _model->findViewportIndex(viewportID);
        const int pageID = (idx.isValid() && idx.parent().isValid())
            ? _model->data(idx.parent(), Roles::PhyidRole).toInt() : -1;
        const bool wasActive = (VMManager->currentViewportID() == viewportID);
        _model->removeViewport(viewportID);

        if (pageID == _lastPageID)
        {
            applyPageLayout(_lastPageID);
        }

        if (!wasActive)
        {
            //删除非激活视口：仅移除节点并重排补位，不切换激活
            return;
        }

        //批量删除（删除模型）期间不做激活转移：全部视口删除完成后，
        //管理器统一发 currentViewportChanged(-1) 由树决定新激活目标
        if (VMManager->isBatchRemovingViewports())
        {
            return;
        }

        activateFirstAvailableViewport(pageID);
    }

    void mViewTreeView::activateFirstAvailableViewport(int preferPageID)
    {
        //按树结构决定新激活目标——
        //优先页还有其他子节点则激活其第一个子节点；
        //已空则从第一页起找到第一个有视口的页，切换到该页并激活其第一个子节点
        int newActiveID = -1;
        const QList<int> curPageViewports = (preferPageID != -1) ? _model->pageViewportIDs(preferPageID) : QList<int>();
        if (!curPageViewports.isEmpty())
        {
            newActiveID = curPageViewports.first();
        }
        else
        {
            for (int pid : _model->pageIDs())
            {
                const QList<int> ids = _model->pageViewportIDs(pid);
                if (!ids.isEmpty())
                {
                    newActiveID = ids.first();
                    if (pid != _lastPageID)
                    {
                        switchToPage(pid);
                    }
                    break;
                }
            }
        }

        if (newActiveID != -1)
        {
            VMManager->activateViewportByID(newActiveID);
        }
        else
        {
            //树内已无视口可激活：由"模型最少存在一个视口"可知系统已无任何模型，
            //复位管理器当前视口状态，避免面板持有已删除视口的悬空 ID
            VMManager->slot_viewportActivated(-1);
            _model->setActiveViewportNode(-1);

            //软件至少保留一个前处理模型：全部模型已删除时自动补建。
            //补建与"新建模型"入口同构：数据层建模型（含首个视口，经信号链
            //登记视口树并激活）+ 模型树登记节点
            mViewTreeFunction::newViewportModel();
            mBrowserWindow::getInstance()->getBrowserTree()->addModelBrowserTree();
        }
    }

    void mViewTreeView::slot_viewportRenamed(int viewportID, const QString& oldName, const QString& name)
    {
        Q_UNUSED(oldName);   //旧名供数据同步订阅方使用，视图树文本只关心新名
        //重命名联动（含模型树命名回填/模型重命名前缀同步）：仅更新节点文本
        _model->renameViewportNode(viewportID, name);
    }

    void mViewTreeView::slot_currentViewportChanged(int viewportID)
    {
        //无效视口（-1）：管理器当前视口已复位（如删除当前模型后），
        //树从现存节点中决定新的激活目标
        if (viewportID == -1)
        {
            _model->setActiveViewportNode(-1);
            activateFirstAvailableViewport(_lastPageID);
            return;
        }
        if (!getCVM) return;
        auto priv = VMManager->getCurrentViewportPrivate();
        if (!priv) return;
        const int activeID = priv->getViewportID();
        //当前激活视口节点文本加粗（其余取消）
        _model->setActiveViewportNode(activeID);
        selectViewportNode(activeID);
    }

    void mViewTreeView::selectViewportNode(int viewportID)
    {
        QModelIndex idx = _model->findViewportIndex(viewportID);
        if (!idx.isValid()) return;
        QModelIndexList cur = selectionModel()->selectedRows();
        if (cur.size() == 1 && cur[0] == idx) return;
        selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    void mViewTreeView::syncActiveViewportNode(int viewportID)
    {
        //与 slot_currentViewportChanged 的联动两步一致：目标节点加粗 + 程序化选中。
        //selectViewportNode 选中视口节点不触发激活（激活仅双击），无联动回环；
        //节点未登记（卡片已关闭等）时静默返回
        if (!_model->findViewportIndex(viewportID).isValid())
            return;
        _model->setActiveViewportNode(viewportID);
        selectViewportNode(viewportID);
    }

    int mViewTreeView::curPageLayout() const
    {
        return _model->pageLayout(_lastPageID);
    }

    int mViewTreeView::createPageAndGetId(const QString& name)
    {
        _model->addPage(name);
        const QList<int> ids = _model->pageIDs();
        //addPage 追加到根节点子列表尾部，新页ID即末位
        return ids.isEmpty() ? -1 : ids.last();
    }

    bool mViewTreeView::attachViewportToPage(int pageID, int viewportID, const QString& name, int modelID)
    {
        return _model->addViewportToPage(pageID, viewportID, modelID, name);
    }

    bool mViewTreeView::detachViewportNode(int viewportID)
    {
        return _model->removeViewport(viewportID);
    }

    int mViewTreeView::pageOfViewport(int viewportID) const
    {
        //经各页的视口列表反查所属页（页/视口数量小，遍历开销可忽略）
        const QList<int> pages = _model->pageIDs();
        for (int pageID : pages)
        {
            if (_model->pageViewportIDs(pageID).contains(viewportID))
                return pageID;
        }
        return -1;
    }

    int mViewTreeView::rowOfViewport(int viewportID) const
    {
        QModelIndex idx = _model->findViewportIndex(viewportID);
        return idx.isValid() ? idx.row() : -1;
    }

    bool mViewTreeView::moveViewportToPageRow(int viewportID, int pageID, int row)
    {
        QModelIndex idx = _model->findViewportIndex(viewportID);
        if (!idx.isValid()) return false;

        //目标位置与现状一致时不动（同页同行）
        const int curPageID = pageOfViewport(viewportID);
        if (curPageID == pageID && idx.row() == row) return true;

        //先移除再按行号插入到目标页（modelID/名称沿用节点原值）
        int modelID = -1;
        QString name;
        if (!_model->viewportNodeInfo(viewportID, modelID, name))
        {
            return false;
        }
        _model->removeViewport(viewportID);
        if (!_model->insertViewportToPage(pageID, row, viewportID, modelID, name))
        {
            return false;
        }

        //目标页即当前页时重排布局，使容器内几何与节点顺序一致；
        //目标页非当前页时新视口保持隐藏（非本页视口不显示）
        if (pageID == _lastPageID)
        {
            applyPageLayout(pageID);
        }
        return true;
    }

    QList<QPair<QString, int>> mViewTreeView::pageList() const
    {
        return _model->pageList();
    }

    void mViewTreeView::switchToPage(int pageID)
    {
        if (!_model->findPageIndex(pageID).isValid()) return;
        if (pageID == _lastPageID)
        {
            applyPageLayout(pageID);//重复切换视作刷新布局
            return;
        }
        _lastPageID = pageID;

        //选中页节点（抑制选中联动重入）
        QModelIndex pageIdx = _model->findPageIndex(pageID);
        if (pageIdx.isValid())
        {
            _switchingPage = true;
            selectionModel()->select(pageIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            _switchingPage = false;
        }
        applyPageLayout(pageID);
        emit sig_pageStateChanged();
    }

    void mViewTreeView::setCurrentPageLayout(int layoutIndex)
    {
        //同值布局也强制重新应用（用户显式选择布局即期望立即生效，
        //例如默认几何覆盖了布局后需要恢复）
        _model->setPageLayout(_lastPageID, layoutIndex);
        applyPageLayout(_lastPageID);
        emit sig_pageStateChanged();
    }

    void mViewTreeView::applyPageLayout(int pageID)
    {
        auto* container = VMManager->getViewportContainer();
        if (!container) return;

        const auto& table = viewLayoutTable();
        int layoutIdx = _model->pageLayout(pageID);
        if (layoutIdx < 0 || layoutIdx >= table.size()) layoutIdx = 0;

        //手动布局可见组优先（视图树选视口+"布局"设置）：未更改布局下标前保持该组——
        //切换图表/切页等重排不回退到按节点序填格；组内节点已删除的过滤，
        //组全部失效时清除手动布局回到节点序
        QList<int> visible = _model->pageVisibleViewportIDs(pageID);
        if (!visible.isEmpty())
        {
            const QList<int> all = _model->pageViewportIDs(pageID);
            QList<int> alive;
            for (int id : visible)
            {
                if (all.contains(id))
                    alive.append(id);
            }
			if (!alive.isEmpty())
			{
				//激活视口保格（与下方节点序填格/setActiveViewport 的换入规则一致）：
				//手动组多于格数时，激活视口若排在溢出位会随布局重放被挤出隐藏——
				//曲线提取静默登记触发的排队同值重放即此症状：单格页把正在看的云图
				//视口挤掉、换上节点序靠前的前处理视口（2026-11-17 真机复现钉因）。
				//同款"换入尾格"保住用户当前视口
				const int slotCount = viewLayoutSlotCount(table[layoutIdx]);
				int activeID = VMManager->currentViewportID();
				const int containerActiveID = container->activeViewportID();
				if (containerActiveID != -1)
					activeID = containerActiveID;
				if (activeID != -1 && alive.contains(activeID))
				{
					const int activePos = alive.indexOf(activeID);
					if (activePos >= slotCount && slotCount > 0)
					{
						alive.removeAll(activeID);
						alive.insert(slotCount - 1, activeID);
					}
				}
				//布局由树唯一驱动：布局类型与可见视口列表直接传给容器，列表外全部隐藏
				container->applyLayout(layoutIdx, alive);
				//懒加载：本页可见图表卡片就地物化（幂等；已物化跳过）——
				//对齐 HyperViewer"切到该视口才创建"。只传实际呈现集（applyLayout
				//同列表前 slotCount 个）：溢出隐藏卡片不物化（免整页建 GL 画布），
				//也不进工厂回收保护集（隐藏中可被 LRU 卸载，进呈现集时按需重物化）；
				//此前整列表传入会经补施循环把溢出卡翻可见（2026-11-17 挤压钉因）
				PostCurve::mCurveViewportFactory::getInstance()->materializeForCardIds(
					alive.mid(0, qMin(slotCount, alive.size())));
				return;
			}
            _model->setPageVisibleViewportIDs(pageID, {});
        }

        //布局由树唯一驱动：布局类型与可见视口列表（页内节点顺序）直接传给容器，
        //容器不持有页概念，列表外视口全部隐藏（显隐不影响激活）
        QList<int> pageViewports = _model->pageViewportIDs(pageID);

        //当前页有激活视口时，激活视口若不在前 slotCount 个内，换入尾格（其余格保持）；
        //无激活视口则按节点顺序取前 slotCount 个。
        //与 setActiveViewport 的换入规则一致：激活换尾格，不做"激活置顶"。
        const int slotCount = viewLayoutSlotCount(table[layoutIdx]);
        int activeID = VMManager->currentViewportID();
        const int containerActiveID = container->activeViewportID();
        if (containerActiveID != -1)
            activeID = containerActiveID;
        if (activeID != -1 && pageViewports.contains(activeID))
        {
            const int activePos = pageViewports.indexOf(activeID);
            if (activePos >= slotCount && slotCount > 0)
            {
                //激活视口超出当前格数：换入尾格
                pageViewports.removeAll(activeID);
                pageViewports.insert(slotCount - 1, activeID);
            }
        }
        container->applyLayout(layoutIdx, pageViewports);
        //懒加载：本页可见图表卡片就地物化（幂等；已物化跳过）。只传实际呈现集
        //（applyLayout 同列表前 slotCount 个）：溢出隐藏卡片不物化不保护，
        //防补施循环越格翻可见（同手动组分支注释，2026-11-17 挤压钉因）
        PostCurve::mCurveViewportFactory::getInstance()->materializeForCardIds(
            pageViewports.mid(0, qMin(slotCount, pageViewports.size())));
    }

    void mViewTreeView::layoutSelectedViewports()
    {
        auto* container = VMManager->getViewportContainer();
        if (!container) return;

        //收集选中的视口节点（保持选中顺序，即用户点选顺序）
        QList<int> selectedIDs;
        int pageID = -1;
        for (const auto& sel : selectionModel()->selectedRows())
        {
            if (_model->data(sel, Roles::TypeRole).toInt() != MechPreData::ViewTreeTypes::ViewTreeViewportItem)
                continue;
            selectedIDs.append(_model->data(sel, Roles::PhyidRole).toInt());
            if (sel.parent().isValid())
            {
                const int pid = _model->data(sel.parent(), Roles::PhyidRole).toInt();
                if (pageID == -1) pageID = pid;
                else if (pageID != pid)
                {
                    //所有选中的视口必须属于同一页
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString(QObject::tr("选中的视口不属于同一页，无法布局。")));
                    return;
                }
            }
        }

        if (selectedIDs.isEmpty() || pageID == -1) return;

        //按该页布局类型排列：格数与选中数不一致时警告提示但不退出，
        //仍按选中顺序填入格子（超出的视口被隐藏，空格留空）
        const auto& table = viewLayoutTable();
        int layoutIdx = _model->pageLayout(pageID);
        if (layoutIdx < 0 || layoutIdx >= table.size()) layoutIdx = 0;
        const int slotCount = viewLayoutSlotCount(table[layoutIdx]);
        if (slotCount != selectedIDs.size())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString(QObject::tr("选中视口数（%1）与当前布局格数（%2）不一致，已按选中顺序布局。")
                .arg(selectedIDs.size()).arg(slotCount)));
        }

        //以选中视口为可见组应用布局：未选中的同页视口隐藏。
        //该分组持久到页（除非更改布局下标，否则切换图表/切页等重排均保持此组，
        //不会回退到按节点序填格——如页内4个图表选3个做布局后，切换仍是3个视口）
        container->applyLayout(layoutIdx, selectedIDs);
        //懒加载：手动布局改变了可见组，同步工厂可见集并物化新可见卡片。
        //只传实际呈现集（前 slotCount 个），同上方两处填格分支的收敛口径
        PostCurve::mCurveViewportFactory::getInstance()->materializeForCardIds(
            selectedIDs.mid(0, qMin(slotCount, selectedIDs.size())));
        _model->setPageVisibleViewportIDs(pageID, selectedIDs);
    }

    void mViewTreeView::slot_rightClick()
    {
        //右键直接作用于光标下的节点
        QModelIndex idx = indexAt(viewport()->mapFromGlobal(QCursor::pos()));
        if (!idx.isValid())
        {
            return;
        }
        //多选状态下右键落在已选中节点上时保留多选（供"布局"等批量操作）；
        //否则按该节点单选（程序化选中抑制激活）
        const QModelIndexList selected = selectionModel()->selectedRows();
        if (!selected.contains(idx))
        {
            _suppressActivate = true;
            selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            _suppressActivate = false;
        }

        int type = _model->data(idx, Roles::TypeRole).toInt();

        //与模型树右键同口径的自绘菜单：原生 QMenu 会继承本树被改白的
        //QPalette::Highlight 作悬浮底色（白底上悬浮行发白），悬浮色统一走 UiPalette::hover
        mUiMenu menu(this);
        ThemeManager::applyMenuStyle(&menu);

        if (type == MechPreData::ViewTreeTypes::ViewTreePageItem)
        {
            QAction* action = menu.addAction(QIcon(":/mMainWidgets/Icon/homepage/xinjian-23x.png"), QString("新建视口"));
            connect(action, &QAction::triggered, this, [this]() {
                //新视口由管理器创建后经 sig_viewportAdded 登记到当前选中页
                mViewTreeFunction::newViewport();
                });

            action = menu.addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), QString("新建页"));
            connect(action, &QAction::triggered, this, [this]() {
                QModelIndex pageIdx = _model->addPage();
                _lastPageID = _model->data(pageIdx, Roles::PhyidRole).toInt();
                expandAll();
                selectionModel()->select(pageIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                });

            //删除页：遍历页内视口——
            //前处理视口按模型分组判断：该模型在其他页还有视口则页内这些全删；
            //该模型所有视口都在这一页时保留最后一个（避免模型变空）。
            //非前处理视口（后处理/曲线等）连同所属模型一并删除。
            //最后判断页内是否还有视口：有则保留该页，无则删除该页
            action = menu.addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QString("删除页"));
            connect(action, &QAction::triggered, this, [this, idx]() {
                const int pageID = _model->data(idx, Roles::PhyidRole).toInt();
                const QList<int> pageViewports = _model->pageViewportIDs(pageID);

                //第一遍：按模型分组页内前处理视口；非前处理视口所属模型直接进级联删除集合
                QHash<int, QList<int>> preModelToViewports;  //前处理 modelID -> 页内该模型的视口列表
                QSet<int> toDeleteModels;                     //非前处理视口所属模型（级联删除）
                for (int viewportID : pageViewports)
                {
                    int modelID = -1;
                    auto* priv = VMManager->getViewportPrivateByID(viewportID, &modelID);
                    if (!priv || modelID == -1) continue;

                    const auto type = priv->getViewportType();
                    const bool isPre = (type == MxFunctions::Viewport3D || type == MxFunctions::Viewport2D);
                    if (!isPre)
                    {
                        toDeleteModels.insert(modelID);
                        continue;
                    }
                    preModelToViewports[modelID].append(viewportID);
                }

                //第二遍：按模型判断前处理视口的删除/保留
                QList<int> toDeleteViewports;
                for (auto it = preModelToViewports.begin(); it != preModelToViewports.end(); ++it)
                {
                    const int modelID = it.key();
                    const QList<int>& vps = it.value();
                    auto* viewportModel = VMManager->getViewportModel(modelID);
                    if (!viewportModel) continue;
                    const int totalViewports = viewportModel->getSubWindows().size();

                    if (totalViewports > vps.size())
                    {
                        //该模型在其他页还有视口：页内这些都可以删
                        toDeleteViewports.append(vps);
                    }
                    else
                    {
                        //该模型所有视口都在这一页：保留最后一个，其余删除（避免模型变空）
                        for (int i = 0; i < vps.size() - 1; ++i)
                        {
                            toDeleteViewports.append(vps[i]);
                        }
                        //vps.last() 保留
                    }
                }

                //执行删除：先删前处理视口（走 sig_viewportRemoved 流程），再级联删模型
                for (int viewportID : toDeleteViewports)
                {
                    mViewTreeFunction::deleteViewport(viewportID);
                }
                for (int modelID : toDeleteModels)
                {
                    //模型树（mTreeModel）节点同步移除
                    mBrowserWindow::getInstance()->getBrowserTree()->removeModelRootItem(modelID);
                    mViewTreeFunction::deleteViewportModel(modelID);
                }

                //最后判断页内是否还有视口（有保留的"最后一个前处理视口"时页保留）
                if (_model->rowCount(idx) == 0)
                {
                    //页内无视口了：删除该页（至少保留一页的约束仍生效）
                    if (_model->pageIDs().size() > 1)
                    {
                        if (_lastPageID == pageID)
                        {
                            //被删页是最近使用页时，回退到其他页
                            QList<int> ids = _model->pageIDs();
                            for (int id : ids)
                            {
                                if (id != pageID)
                                {
                                    _lastPageID = id;
                                    break;
                                }
                            }
                        }
                        _model->removePage(idx);
                    }
                }
                //页内还有保留的视口：该页保留，不执行 removePage
                });

            action = menu.addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), QString("重命名"));
            connect(action, &QAction::triggered, this, [this, idx]() {
                //原地编辑重命名（与前处理树操作方式一致），校验逻辑在模型 setData 中
                edit(idx);
                });
        }
        else if (type == MechPreData::ViewTreeTypes::ViewTreeViewportItem)
        {
            //右键点击视口节点：仅选中（程序化选中已在函数开头完成），不激活视口。
            //激活只在左键双击时发生（slot_doubleClicked）
            //删除分流：曲线图表卡片（懒加载登记，经工厂 cardId->chartId 反查）在
            //视口管理器无登记，原路径 deleteViewport 查不到即静默无效——改走曲线
            //文档删除命令（同曲线树"删除图表"：Ctrl+Z 可整体恢复，卡片经
            //nodeAboutToBeRemoved 联动自动关闭）。三维图卡片（R28 入树）同为
            //曲线文档图表节点，走同一 removeNode 链；其余为 3D 渲染视口，走
            //管理器原路径
            const int nodeId = _model->data(idx, Roles::PhyidRole).toInt();
            const int chartIdOfNode = PostCurve::mCurveViewportFactory::getInstance()
                ->chartIdForCard(nodeId);
            QAction* action = menu.addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"),
                chartIdOfNode >= 0 ? QString("删除图表") : QString("删除视口"));
            connect(action, &QAction::triggered, this, [this, idx, chartIdOfNode]() {
                if (chartIdOfNode >= 0)
                {
                    PostCurve::mCurveDocController::getInstance()->removeNode(chartIdOfNode);
                    return;
                }
                const int delId = _model->data(idx, Roles::PhyidRole).toInt();
                mViewTreeFunction::deleteViewport(delId);
                });

            action = menu.addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), QString("重命名"));
            connect(action, &QAction::triggered, this, [this, idx]() {
                //原地编辑重命名（与前处理树操作方式一致），校验与同步逻辑在模型 setData 中
                edit(idx);
                });

            //布局：多选同页视口按该页布局类型排列（选中数须与布局格数一致）
            action = menu.addAction(QString("布局"));
            connect(action, &QAction::triggered, this, [this]() {
                layoutSelectedViewports();
                });

            //删除模型：该模型的所有视口一并删除（每个视口走 sig_viewportRemoved 流程，
            //激活视口在其中时由 slot_viewportRemoved 按既有逻辑转移激活）；
            //模型树（mTreeModel）节点同步移除。删除前弹确认框（与模型树删除模型一致）。
            //曲线图表卡片项无所属模型（modelID=-1），不提供本项（原实现点击空操作）
            if (_model->data(idx, Roles::ModelRole).toInt() >= 0)
            {
                action = menu.addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QString("删除模型"));
                connect(action, &QAction::triggered, this, [this, idx]() {
                    const int modelID = _model->data(idx, Roles::ModelRole).toInt();
                    if (modelID == -1) return;
                    const int r = MechMainWindow::mFramelessMessageBox::warning(this, "警告",
                        "确定要删除当前模型吗？", "确定", "取消", QString(), 1, 0);
                    if (r != 0) return;
                    mBrowserWindow::getInstance()->getBrowserTree()->removeModelRootItem(modelID);
                    mViewTreeFunction::deleteViewportModel(modelID);
                    });
            }
        }
        else
        {
            return;
        }

        menu.exec(QCursor::pos());
    }

    //////////////////////////////////////////////////////////////////////////
    // mViewBrowserWindow
    //////////////////////////////////////////////////////////////////////////
    mViewBrowserWindow* mViewBrowserWindow::_instance = nullptr;

    mViewBrowserWindow* mViewBrowserWindow::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mViewBrowserWindow;
            _instance->setObjectName("ViewTreeDock");
            _instance->setMinimumWidth(220);
        }
        return _instance;
    }

    mViewBrowserWindow::mViewBrowserWindow(QWidget* parent) : QWidget(parent)
    {
        QPalette pal;
        pal.setColor(QPalette::Background, QColor(255, 255, 255));
        setAutoFillBackground(true);
        setPalette(pal);

        _treeView = new mViewTreeView(this);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->addWidget(_treeView);
    }

    mViewBrowserWindow::~mViewBrowserWindow()
    {
        if (_treeView)
        {
            delete _treeView;
            _treeView = nullptr;
        }
    }
}
