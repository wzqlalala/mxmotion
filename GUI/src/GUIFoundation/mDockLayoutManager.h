#pragma once

#include <QWidget>
#include <QList>
#include <QDockWidget>
#include <QToolButton>
#include <QSettings>
#include <QPair>
#include <QColor>
#include <QObject>

#include "mBasicEnum.h"
#include "guifoundation_global.h"

class QMainWindow;
class QMenu;
namespace ads
{
    class CDockManager;
    class CDockWidget;
    class CDockAreaWidget;
    class CDockWidgetTab;
}
//详细信息面板的基类
namespace MechMainWindow
{
    class GUIFOUNDATION_EXPORT mDockLayoutManager : public QObject
    {
        Q_OBJECT
    public:

        /*
        * 获取单例指针
        */
        static mDockLayoutManager* getInstance();

        /*
        * 析构函数
        */
        ~mDockLayoutManager();

        void setCDockManager(ads::CDockManager* dockManager, QMainWindow* mainwin);

        /*
        * 停靠窗口公共头部按规则 2 重发（不套功能区页签规则）：
        * 高 H+2、header 底 + 1px border 下边线；标题 body/400/text 左对齐；
        * 动作按钮 H−2、16px 图形、相邻 4px；激活页签不再使用 selectedText 蓝字。
        * setCDockManager 时首次调用，切主题/切档时由 ThemeManager 链重复调用。
        */
        void applyTheme();

        void setCenterWidget(QWidget *w);

        void getCenterWidget() {};

        /*
        * 激活窗口
        */
        void activateDock(const QString& key);

        ads::CDockWidget* GetDock(const QString& key);

        // 注册需要管理的 dock
        ads::CDockWidget* registerDock(QWidget* w, QString titleName, QString keyName, const QColor& titleBarColor = QColor(255, 255, 255));

        // 将窗口添加到底部区域（若不存在则自动创建），并激活显示
        void addDockToBottom(QWidget* w, QString titleName, QString keyName);

        // 创建“管理布局”按钮（带菜单，供 MainWindow 加到工具栏）
        QMenu* createLayoutMenuButton();

        // 保存/恢复布局
        void saveLayout();
        bool restoreLayout();

        // 应用两种固定预设布局
        void applyPreset1() ;
        void applyPreset2() ;

        /*
        * 按当前激活视口类型切换树窗口显隐：
        * 前处理视口->前处理树；后处理视口->后处理树；曲线视口->曲线树；视图树常显
        * 目标树放入当前树所在的标签位（与前处理树同位置），再隐藏其余树
        */
        void updateTreeWindows(MxFunctions::ViewportType type);

        /*
        * 设置指定 key 的 dock 显示/隐藏（不存在时忽略）
        */
        void setDockVisible(const QString& key, bool visible);

    private:
        /*
        * 视口类型对应的树窗口key（前处理树/后处理树/NVH树）
        */
        QString treeKeyForType(MxFunctions::ViewportType type) const;

        /*
        * 显示树窗口并放入"树标签位"：当前打开的树dock所在区域（即前处理树的标签位置），
        * 无可用区域时退回左侧区域，避免ADS重开时落到其他位置
        */
        void showTreeDock(const QString& key);

    public:
        /*
        * 按视口类型跳转（置前）对应树窗口：前处理->模型、后处理->云图、曲线->曲线；
        * 关闭态会先归位打开。用于视图树双击视口节点后的显式跳转。
        * 与 updateTreeWindows 同源：跳转同样执行三棵树互斥隐藏，
        * 保证任何展示路径下"三棵树同一时刻只存在一棵"
        */
        void showTreeDockForType(MxFunctions::ViewportType type);

    private:
        mDockLayoutManager();

        // 根据激活状态更新页签底色/文字（色值全部取自 UiPalette）
        void updateTabBackground(ads::CDockWidgetTab* tab);

        // 规则 2 公共窗口头：单个 dock 标题/关闭按钮、区域头部及动态新建区域统一重发
        void applyDockHeaderRules(ads::CDockWidget* dock);
        void applyAreaHeaderRules(ads::CDockAreaWidget* area);
        void applyHeaderRulesToChildren();

        /*
        * 信息输出窗可折叠分割条（CollapsibleSplitterHandle）主题重发：
        * 自绘 handle 不读全局 QSS，经元方法调其 applyTheme()
        */
        void applyCollapsibleHandlesTheme();

    private:
        void resetDockNetwork();


    private:
        //单例指针
        static mDockLayoutManager* _instance;
        ads::CDockManager* _dockManager;
        QMainWindow* m_mainWin;
        //重置布局期间置位：移除 dock 会触发视口/树选中信号的同步重入，此间不响应面板激活
        bool m_resettingLayout = false;
      
        ads::CDockWidget*  m_center;
        QList<QString> m_dockNameList;
        QMap<QString, QAction*> m_actions;
        // 每个 dock 激活时的标题栏颜色
        QMap<QString, QColor> m_dockTitleBarColors;
        //QList<QDockWidget*> m_docks;
        //QString m_org;
        //QString m_app;
        //int m_version;
        //QString m_keyGeo;
        //QString m_keyState;
    public:
        QMap<QString, ads::CDockWidget*> m_docks;
    };
}