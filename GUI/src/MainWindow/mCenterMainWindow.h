#pragma once
#pragma execution_character_set("utf-8")


#include <QMainWindow>
#include <QToolBar>

class QComboBox;
class QLabel;
class QToolButton;
class QPushButton;
#include <QToolButton>
#include <QSpinBox>
#include <mCheckComboBox.h>

namespace MechMainWindow
{
    class mCenterMainWindow : public QMainWindow
    {
        Q_OBJECT
    public:
        /*
        *构造函数
        */
        mCenterMainWindow(QWidget* parent = nullptr);

        /*
        *析构函数
        */
        ~mCenterMainWindow();

        /*
        * 初始化
        */
        void init();

        /*
        * 设置视图工具栏显隐
        */
        void setViewToolBarVisibility(bool isShow, QAction* action);

        /*
        * 设置视图工具栏的位置
        */
        void setViewToolBarArea(Qt::ToolBarArea toolBarArea);

        // 重置为默认工具栏布局：菜单工具栏在左，视图显示栏在右。
        Q_INVOKABLE void resetViewToolBarLayout();

        /*
        * 获取对象捕捉当前勾选的吸附类型列表
        */
        std::vector<int> getCheckedSnapTypes() const;

        /*
        * 获取当前主窗口中的中心视图实例
        */
        static mCenterMainWindow* instance();

    private slots:

        /*
        * 切换视口
        */
        void slot_subWindowActivated(int viewportID);

        /*
        * 切换第二个菜单信号
        */
        void slot_switchMenuBarWindow(const QString& name);

        /*
        * 拾取模式变化
        */
        void slot_onPickModeSelected(int mode);

        /*
        * 拾取完成
        */
        void slot_finishedPick();

        /*
        * 映射拾取
        */
        void slot_onPickMapping(int mode);

        /*
        * 设置左键功能
        */
        void slot_setLeftButtonViewMode(int mode);

        /*
        * 设置缩放功能
        */
        void slot_setScaleViewMode(int mode);

        /*
        * 设置旋转功能
        */
        void slot_setRotateViewMode(int mode);

        /*
        * 设置视图方向功能
        */
        void slot_setViewMode(int mode);


        /*
        设置框选功能
        */
        void slot_setselectMode(int mode);

        /*
        * 设置显示功能
        */
        void slot_setShowMode(int mode);

        //窗口同步
        void slot_winSync();

        /*
        * 对象捕捉选项变化
        */
        void slot_snapOptionChanged(int index, bool checked);

    protected:
        //注册快捷键
        void initShortcuts();
    public:
        //视图工具栏/视图显示栏（布局重排时由 mDockLayoutManager 重插排序）
        QToolBar* getViewToolBar() const { return _toolBar; }
        QToolBar* getViewDisplayBar() const { return _viewBar; }
        //布局恢复后调用：跳过 showEvent 的钉正逻辑（restoreState 已恢复用户位置）
        void setSkipToolbarPinning(bool skip) { _skipToolbarPinning = skip; }

        /*
        * 视口工具栏主题重发（附 A：工具栏随主题/档位）：ThemeManager::applyAll
        * 遍历顶层窗时经元方法调用（Common 不反向链接 MainWindow 类型）。幂等。
        */
        Q_INVOKABLE void applyToolbarTheme();

    private:
        // 新增：统一更新映射菜单启用状态
         // allDisable: 是否全部禁用（无拾取对象时用）
         // extraDisableIndex: 额外禁用的映射项索引（当前模式/当前拾取类型对应项），-1表示无
        void updateMappingMenuState(bool allDisable = false, int extraDisableIndex = -1);
    private:
        //视图显示栏：页选择/翻页/视口布局/视口联动
        void initViewDisplayBar();

        //动画工具栏壳（anim-tab-v13）：主窗口预创建空壳（restoreLayout 前已存在→
        //saveState/restoreState 位置持久化成立），内容件由 mPost3DResultPanel 经
        //MechCommon::animToolBarProvider 回挂；前处理隐藏/后处理常显（D-08），显隐由
        //面板 slot_viewportContextChanged 驱动
        void initAnimToolBar();

        // 布局恢复结束后钉正 _viewBar 顺序，保留用户保存的工具栏区域
        void showEvent(QShowEvent* event) override;


        //按视图树当前状态刷新视图显示栏各控件
        void refreshViewBar();

    private:
        QToolBar* _toolBar = nullptr;
        QToolBar* _viewBar = nullptr;
        QToolBar* _animBar = nullptr;   //动画工具栏壳（anim-tab-v13，空壳预创建+初始隐藏）
        MxFunctions::mCheckComboBox* _pageCombo = nullptr;
        QToolButton* _layoutBtn = nullptr;
        QToolButton* _syncBtn = nullptr;
        QLabel* _pageLabel = nullptr;
        //刷新下拉框时的同步保护（避免 currentIndexChanged 回环切页）
        bool _pageComboSyncing = false;
        //_viewBar 顺序是否已在首次显示时钉正
        bool _viewBarOrderPinned = false;
        //布局恢复后跳过 showEvent 的钉正逻辑（restoreState 已恢复用户保存的位置，
        //不能再 removeToolBar/insertToolBar 重置区域）
        bool _skipToolbarPinning = false;

        //当前第二个菜单栏的名称（是否为网格）
        QString _currentSecondMenuName;

        //视图的左键功能
        QActionGroup* _leftViewActionGroup;

        //视图的缩放功能
        QActionGroup* _scaleViewActionGroup;

        //视图的旋转功能
        QActionGroup* _rotateViewActionGroup;

        //旋转角度
        QDoubleSpinBox* _rotateSpinBox;

        //视图的方向功能
        QActionGroup* _viewActionGroup;

        //视图的显示功能
        QActionGroup* _showViewActionGroup;

        //视图的框选功能
        QActionGroup* _selectViewActionGroup;

        //显示网格action
        QAction* _actionShowMesh;

        //拾取工具栏
        QList<QAction*> _actionPick;

        //转换下拉菜单
        QMenu* _mappingMenu;

        //对象捕捉下拉菜单
        QMenu* _snapMenu = nullptr;
        QToolButton* _snapButton = nullptr;
        QList<QAction*> _snapActions;

        //拾取完成的连接
        QMetaObject::Connection _connPick1;

        //保存拾取类型全局启用状态
        QVector<bool> _pickTypesEnabled;
    };
}