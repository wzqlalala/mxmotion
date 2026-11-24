#pragma once
#pragma execution_character_set("utf-8")

//Qt
#include <QWidget>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QVector>

#include "ViewportLayouts.h"
#include "mTreeModel.h" //Roles / ViewTreeTypes

namespace MechPreData
{
    class mViewportPrivate;
}

namespace MechMainWindow
{
    //////////////////////////////////////////////////////////////////////////
    // 视图树数据模型：页（顶级节点）-> 视口子节点
    // 视口子节点 phyID 存视口全局ID，可跨模型混排（一页内允许不同模型的不同视口）
    //////////////////////////////////////////////////////////////////////////
    class mViewTreeModel : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        explicit mViewTreeModel(QObject* parent = nullptr);
        ~mViewTreeModel() override;

        QModelIndex index(int row, int col, const QModelIndex& parent) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent) const override;
        int columnCount(const QModelIndex&) const override { return 1; }
        QVariant data(const QModelIndex& idx, int role) const override;
        bool setData(const QModelIndex& idx, const QVariant& value, int role) override;
        Qt::ItemFlags flags(const QModelIndex& idx) const override;

        //新增一页（name 为空时自动命名 页-N）
        QModelIndex addPage(const QString& name = QString());
        //删除空页（有子节点时不允许）
        bool removePage(const QModelIndex& pageIdx);
        //向指定页添加视口子节点
        bool addViewportToPage(int pageID, int viewportID, int modelID, const QString& name);
        //向指定页的指定位置插入视口子节点（row 越界时追加到末尾）
        bool insertViewportToPage(int pageID, int row, int viewportID, int modelID, const QString& name);
        //按视口ID移除视口子节点
        bool removeViewport(int viewportID);
        //按视口ID取出节点归属信息（modelID 与显示名），供迁移/重建节点用
        bool viewportNodeInfo(int viewportID, int& modelID, QString& name) const;
        //按视口ID更新视口节点显示名（重命名联动，不做校验）
        bool renameViewportNode(int viewportID, const QString& name);
        //按视口ID查找（返回视口节点索引）
        QModelIndex findViewportIndex(int viewportID) const;
        //按页ID查找页索引
        QModelIndex findPageIndex(int pageID) const;
        //获取全部页ID
        QList<int> pageIDs() const;
        //获取页列表（页名, 页ID），用于下拉框
        QList<QPair<QString, int>> pageList() const;
        //页的视口布局索引（viewLayoutTable 下标）
        int pageLayout(int pageID) const;
        //设置页的视口布局索引
        bool setPageLayout(int pageID, int layoutIndex);
        //交换页内两个视口节点的位置（布局按节点序填格：前 cells 个呈现，其余隐藏）。
        //用于溢出页"选中即替换呈现"——新选中图表与其顶替的视口交换节点位：顶替者
        //入格呈现，被顶替者退到隐藏位，页内其余节点位置不变。
        //任一视口不在页上或为同一节点返回 false
        bool swapViewportNodes(int pageID, int viewportA, int viewportB);
        //页的手动布局可见组（视图树选视口+"布局"设置；空=未手动布局，
        //按节点序填格呈现）。未更改布局下标前保持不变
        QList<int> pageVisibleViewportIDs(int pageID) const;
        //设置页的手动布局可见组（整组替换；空组=清除手动布局回到节点序填格）
        bool setPageVisibleViewportIDs(int pageID, const QList<int>& ids);
        //页内视口ID列表（按节点顺序）
        QList<int> pageViewportIDs(int pageID) const;
        //设置当前激活视口（该节点文本加粗，其余取消加粗）
        void setActiveViewportNode(int viewportID);

        //清空全部页（会话恢复前清空视图树用；不自动补默认页，由调用方逐页重建）
        void clearAllPages();

    signals:
        //页增删/重命名/布局变化时发出（视图显示栏据此刷新）
        void sig_pagesChanged();

    private:
        struct Item
        {
            int type = 0;       //ViewTreeTypes::ViewTreePageItem / ViewTreeViewportItem
            int id = 0;         //页ID 或 视口ID
            int modelID = 0;    //仅视口节点使用
            int layout = 0;     //仅页节点使用：视口布局索引（viewLayoutTable 下标）
            bool active = false;//仅视口节点使用：是否为当前激活视口（文本加粗）
            //仅页节点使用：手动布局可见组（视图树选视口+"布局"设置；空=未手动布局，
            //按节点序填格）。布局下标变更时清除，其余重排保持该组
            QList<int> visibleViewportIDs;
            QString text;
            Item* parent = nullptr;
            QVector<Item*> children;

            ~Item() { for (Item* c : children) delete c; }
        };

        Item* itemFromIndex(const QModelIndex& idx) const;
        QModelIndex indexFromItem(Item* item) const;
        void deleteItem(Item* item);

    private:
        Item* m_root = nullptr;
        int m_nextPageID = 1;
    };

    //////////////////////////////////////////////////////////////////////////
    // 视图树视图：点击视口节点激活对应视口；右键维护页/视口；
    // 随当前激活视口高亮对应节点；新视口放入当前选中页（或最近使用页）
    //////////////////////////////////////////////////////////////////////////
    class mViewTreeView : public QTreeView
    {
        Q_OBJECT
    public:
        explicit mViewTreeView(QWidget* parent = nullptr);

        //挂起/恢复与视口管理器的信号同步（会话恢复等批量重建期间挂起，
        //避免视口增删信号驱动树节点自动增删与手动重建冲突）
        void suspendViewportSync();
        void resumeViewportSync();

        //从视口管理器同步已有视口（窗口初始化时调用）
        void syncFromViewports();

        //数据模型（视图显示栏查询页列表/布局）
        mViewTreeModel* getModel() const { return _model; }

    protected:
        //父子节点虚线连接（与前处理树一致）
        void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;

        /*
        * 右键按下不改变选中：防止Qt默认的右键选中触发"选中视口节点即激活"跳转，
        * 右键目标节点由 slot_rightClick 程序化选中（带抑制激活标志）后弹出菜单
        */
        QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& index, const QEvent* event) const override;

    private slots:
        void slot_selectionChanged();
        void slot_doubleClicked(const QModelIndex& idx);
        void slot_viewportAdded(int viewportID);
        void slot_viewportRemoved(int viewportID);
        void slot_viewportRenamed(int viewportID, const QString& oldName, const QString& name);
        void slot_currentViewportChanged(int viewportID);
        void slot_rightClick();

    private:
        //激活视口节点对应的视口（必要时切换模型）
        void activateViewportNode(int viewportID, int modelID);
        //新视口应放入的页ID：当前选中页（或视口节点所在页），否则最近使用页
        int currentPageID();
        //选中某个视口节点（用于激活联动高亮）
        void selectViewportNode(int viewportID);
        void applyPageLayout(int pageID);
        //激活首选页的第一个视口；该页为空则从第一页起找有视口的页，
        //切换到该页并激活其第一个子节点；整棵树无视口时复位管理器状态
        void activateFirstAvailableViewport(int preferPageID);
        //按选中视口布局：校验同页、选中数与该页布局格数一致后，
        //以选中视口（按选中顺序）作为可见组应用该页布局类型
        void layoutSelectedViewports();

    public:
        //当前页ID（工具栏查询）
        int curPageID() const { return _lastPageID; }
        //新建一页并返回页ID（曲线模块等经此为图表卡片开新页）
        int createPageAndGetId(const QString& name = QString());
        //把业务视口卡片登记为页子节点（modelID<0 表示非渲染视口，如曲线图表卡片）
        bool attachViewportToPage(int pageID, int viewportID, const QString& name, int modelID = -1);
        //按视口ID移除页上的登记节点（卡片关闭时）
        bool detachViewportNode(int viewportID);
        //视口ID -> 所属页ID（未登记返回 -1；激活卡片前先切到其所属页用）
        int pageOfViewport(int viewportID) const;
        //视口ID -> 所属页内行号（未登记返回 -1）
        int rowOfViewport(int viewportID) const;
        //把视口节点迁移到指定页的指定行（清空模型后新视口迁回原"视图页"位置用）
        bool moveViewportToPageRow(int viewportID, int pageID, int row);
        //页列表（页名, 页ID）转发（组合根注入曲线模块的视图页查询用）
        QList<QPair<QString, int>> pageList() const;
        //当前页布局格数
        int curPageLayout() const;
        //切换到指定页（选中页节点并应用布局）
        void switchToPage(int pageID);
        //设置当前页布局格数并立即应用
        void setCurrentPageLayout(int slotCount);
        //刷新当前页布局（供容器登记完存量视口后补一次布局，
        //视图树懒汉构造可能早于容器登记导致 syncFromViewports 的布局空跑）
        void refreshCurrentPageLayout() { applyPageLayout(_lastPageID); }

        //同步"激活视口"的树表现（激活节点加粗+程序化选中），视口ID可传图表卡片ID：
        //渲染视口激活经 currentViewportChanged -> slot_currentViewportChanged 走同两步；
        //图表卡片激活不触发该信号，由主窗口的卡片激活补齐路径显式调用
        void syncActiveViewportNode(int viewportID);

    signals:
        //当前页/页数/布局变化（视图显示栏刷新）
        void sig_pageStateChanged();

    private:
        mViewTreeModel* _model = nullptr;
        //最近使用的页ID
        int _lastPageID = 1;
        //右键菜单触发的程序化选中时，抑制"选中视口节点即激活"的联动
        bool _suppressActivate = false;
        //切页过程中抑制选中联动的重入
        bool _switchingPage = false;
        //会话恢复等批量重建期间挂起视口信号同步（见 suspendViewportSync）
        bool _viewportSyncSuspended = false;
    };

    //////////////////////////////////////////////////////////////////////////
    // 视图树窗口（左侧独立 Dock）
    //////////////////////////////////////////////////////////////////////////
    class mViewBrowserWindow : public QWidget
    {
        Q_OBJECT
    public:
        static mViewBrowserWindow* getInstance();

        ~mViewBrowserWindow();

        mViewTreeView* getViewTree() const { return _treeView; }

    private:
        explicit mViewBrowserWindow(QWidget* parent = nullptr);

    private:
        static mViewBrowserWindow* _instance;
        mViewTreeView* _treeView = nullptr;
    };
}
