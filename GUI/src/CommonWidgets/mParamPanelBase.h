#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 参数面板基座（CommonWidgets 库）
//
// 前处理（MainWindow 各面板）、后处理（PostWindow 云图/动画/注释等面板）、
// 曲线（PostCurve 属性页基座 mCurvePropPageBase）三方参数面板的公共基类：
// 构造即应用参数栏控件状态样式（applyParamBarStyle，单一来源），继承即得
// 统一样式，不依赖各面板自觉调用；另提供折叠组管理、快捷键绑定、列表
// 工具等通用能力。
//
// 自 GUIFoundation/mPanelBase 下沉（类名/命名空间/文件引用零改动，
// GUIFoundation/mPanelBase.h 转发包含本头）。留在 GUIFoundation 侧的是
// DataBinder/UiController（依赖 MxFunctions 的 mCalcLineEdit）与
// getPickVecData/getPickPointData（依赖 Geo/Render 拾取栈）——这些是
// PostCurve 不该背的重依赖，基类本体是纯 Qt 的，可以也应当三方共用。
// =============================================================================

#include "CommonWidgets_global.h"
#include "mFoldSection.h"
#include "mParamBarStyle.h"

//QT
#include <QWidget>
#include <QColor>
#include <QShortcut>
#include <QListWidget>

#include <functional>

namespace MechMainWindow
{
    class COMMONWIDGETS_EXPORT mPanelBase : public QWidget
    {
        Q_OBJECT

    public:
        explicit mPanelBase(QWidget* parent = nullptr);
        virtual ~mPanelBase();

        // 设置窗口标题
        void setTitle(const QString& title);

        // 设置背景颜色
        void setBackgroundColor(const QColor& color);

        // 快捷键绑定
        void bindShortcut(const QKeySequence& key, const std::function<void()>& callback);

        //通过名称获取折叠组件
        FoldSection* getFoldSection(const QString& title);

        // 显示/隐藏封装
        void showWidget();
        void hideWidget();

        // 获取列表中选中的元素
        static QStringList getSelectedList(QListWidget* listWidget);

        // 清空列表
        static void clearListWidget(QListWidget* listwidget);

        // 面板是否处于编辑状态，用于避免模型树切换时对所有面板执行无意义的 unedit 清理
        bool isEditing() const { return _isEditing; }

        // 设置编辑状态，进入编辑时置 true，退出编辑时置 false
        void setEditing(bool editing) { _isEditing = editing; }

        /*
        * 卸载并销毁全部折叠框（从父布局移除、清空行、删除控件）。
        * unEdit 收尾的 UI 清理必须走本函数而非各分支自行删除：
        * 数据被删除或当前模型已切换（按 ID 复核落空）时同样需要清理 UI，
        * 否则折叠框残留布局，后续其它对象进入编辑会与新折叠框叠加显示
        */
        void clearFoldWidgets();

        static bool setRowVisible(QWidget* w1, QWidget* w2, bool visible);

    public:
        // 视口改名跟随：默认仅字符串跟随（_curViewName==oldName 时更新）；子类改名后
        // 需刷新 UI 可 override。由桥接方（mPostInfoHub）订阅视口改名信号后经
        // dispatchViewportRenamed 分发——基类不依赖任何上层模块，分发方向由桥决定
        // 函数体必须在 mParamPanelBase.cpp（out-of-line）：本类为 dllexport 基类，
        // 头文件内联定义虚函数体时，导入方（PostWindowd 等）的派生类虚表该槽位
        // 会被链接器落 0，虚调用直接跳 0x0（导入后处理触发视口改名即崩，实证 2026-10-15）
        virtual void onViewportRenamed(const QString& oldName, const QString& newName);

        // 面板注册表分发：桥接方调用（UI 线程同步）。注册表仅持裸指针用于同步分发，
        // 面板构造/析构自登记自注销
        static void dispatchViewportRenamed(const QString& oldName, const QString& newName)
        {
            for (mPanelBase* p : registeredPanels())
                p->onViewportRenamed(oldName, newName);
        }

    public slots:
        // 主题/档位重发入口：ThemeManager::applyGlobalRound 经 mxParamPanel 标记
        // 定位后按名调用（Common 不反向链接 CommonWidgets，与状态栏 applyTheme
        // 同一解耦手法）；重新生成参数栏控件状态样式（幂等，内容未变不重设）
        void applyTheme();

    protected:
        // 当前关联视口名（原各后处理面板私有 _curViewName 提升至基类，统一所有权，
        // 改名跟随一处生效；按名查找的渲染器/guidata 键随视口当前名匹配）
        QString _curViewName;

        // 可重载的关闭事件
        void closeEvent(QCloseEvent* event) override;

        void showEvent(QShowEvent* event) override;

        void hideEvent(QHideEvent* event) override;

        QList<FoldSection*> m_foldWidgets;

    private:
        QList<QShortcut*> m_shortcuts;
        bool _isEditing = false;

        static QList<mPanelBase*>& registeredPanels();
    };
}
