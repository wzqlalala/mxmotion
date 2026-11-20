#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QPointer>
#include <QString>
#include <QSize>
#include <functional>

class QVBoxLayout;
class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;

#include "mmainwindow_global.h"

namespace MechMainWindow
{
    /**
     * @brief 吸附式弹窗通用基类（对应《通用结构软件界面UI设计规则》2.2.6 吸附式弹窗）
     *
     * 只承载吸附式弹窗的公共部分，与具体业务内容无关：
     *  - 入口按钮（标题 + 摘要 + ›），点击展开/收起弹窗，入口剖分移交宿主布局；
     *  - 弹窗外壳：以宿主顶层窗口为父的 Tool 无边框窗口、窗口头（‹ 返回/标题/元信息/✕ 关闭）、
     *    吸附定位（优先入口右侧，空间不足换侧并约束于视口）、点击外部关闭、宿主窗口移动跟随、
     *    宿主销毁监视（关闭期只做纯界面收尾）；
     *  - 底部工具栏骨架 + 导入/导出文件拓展区（互斥向下展开，D5），通过 ImportParams/回调
     *    预留真实读写接口；可选右侧工具按钮（绘制曲线/渲染云图等业务按钮）；
     *  - 底部内联提示消息条。
     *
     * 数据表格、绘图区、拾取会话等业务内容全部由派生类实现：
     *  - buildContent() 在窗口头之后、工具栏之前插入业务内容；
     *  - contentSizeHint() 报告内容宽高，基类完成视口约束与吸附定位；
     *  - 点击外部关闭、宿主移动跟随、析构/宿主销毁收尾等生命周期由基类统一处理。
     *
     * 时序约定：
     *  - 构造期只创建入口按钮一行，不建弹窗、不装全局事件过滤、不调任何虚函数；
     *  - 弹窗内容在 ensurePopup()（首次打开或 preparePopup() 预建）时构建，
     *    此刻派生类已构造完成，buildContent() 内可安全调用虚函数；
     *  - 弹窗惰性构建路径经 0ms 延迟首开，消除同步栈里 show() 的首开闪现。
     */
    class MMAINWINDOW_EXPORT mAttachedPopupBase : public QWidget
    {
        Q_OBJECT

    public:
        /** 导入参数（界面当前值，随「导入」执行回调传给宿主） */
        struct ImportParams
        {
            QString filePath;      // 选择的文件完整路径
            int startRow = 1;      // 起始行（1 起）
            int endRow = -1;       // 结束行，-1 表示末行
            int startColumn = 1;   // 起始列（1 起）
            bool append = false;   // false=覆盖 true=追加
        };

        explicit mAttachedPopupBase(const QString& title, QWidget* parent = nullptr);
        ~mAttachedPopupBase() override;

        /** 展开/收起弹窗（与入口按钮同一行为，供宿主按钮转发） */
        void togglePopup();
        /** 关闭弹窗（面板重建/退出编辑时调用；默认收起全部拓展区与提示） */
        virtual void closePopup();
        bool isPopupOpen() const;
        /** 首次点击前预建弹窗并完成隐藏布局（幂等），消除首开闪现 */
        void preparePopup();

        /** 设置弹窗标题（与入口标题保持对应，2.2.6 层级规则） */
        void setPopupTitle(const QString& title);
        /** 窗口头右侧元信息文本（如「N 行」），派生类按内容刷新 */
        void setMetaText(const QString& text);

        /** 弹窗锚定部件（默认入口按钮；入口嵌入宿主行时锚定整行，
            保证弹窗吸附在整行右侧而非半截按钮上） */
        void setAnchorWidget(QWidget* w) { if (w) _anchor = w; }
        /** 入口按钮访问（宿主内嵌布局时调整高度/最小宽度） */
        QPushButton* entryButton() const { return _entryBtn; }
        /** 入口摘要标签访问（宿主刷新入口显示文本） */
        QLabel* entrySummaryLabel() const { return _entrySummary; }
        /** 入口按钮从控件中剖分：控件放弃所有权交给 newParent，按钮改为「点击
            转发 togglePopup 的宿主按钮」（勾选行联动置灰由宿主自行 setEnabled，
            不再与控件本体 enable 状态耦合）。popupOpened/Closed 仍同步其选中态。
            剖分时移除按钮内嵌样式表，由宿主行布局统一着色（离散数据面板形态）。 */
        void detachEntryWidget(QWidget* newParent);

        /** 导入/导出文件区可选（默认提供）。弹窗创建前调用；隐藏后入口与面板均不出现 */
        void setFilePanelsVisible(bool visible);
        /** 导入/导出回调：设置后点击面板中的执行按钮回调，由宿主实现真实读写 */
        void setImportHandler(const std::function<void(const ImportParams&)>& handler);
        void setExportHandler(const std::function<void(const QString& filePath)>& handler);

        /** 底部工具栏右侧可选业务按钮（绘制曲线/渲染云图等，可 checkable 作为拓展区开关）。
            弹窗创建前后均可调用；点击回调由 setActionHandler 设置 */
        void setActionButtonVisible(bool visible, const QString& text = QString(),
            bool checkable = true);
        void setActionButtonChecked(bool checked);
        void setActionButtonEnabled(bool enabled);
        void setActionHandler(const std::function<void()>& handler);

    signals:
        /** 弹窗开关 */
        void popupOpened();
        void popupClosed();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

        /** 业务内容构建：在窗口头之后插入弹窗内容（表格/绘图/表单等）。
            基类随后追加底部工具栏、文件拓展区与消息条。仅在弹窗创建时调用一次，
            此刻派生类已构造完成，可安全调用虚函数。 */
        virtual void buildContent(QWidget* body, QVBoxLayout* layout) = 0;
        /** 内容宽高（不含窗口头/工具栏/消息条/弹窗边框）。基类完成视口约束与吸附定位。
            实现可读取 currentExtensionHeight()/isMsgVisible() 并更新内容区内部控件显隐。 */
        virtual QSize contentSizeHint() const = 0;
        /** 弹窗最小宽度（普通态 480px 下限，D8）；拾取窄列表等形态由派生类放宽 */
        virtual int popupMinimumWidth() const;
        /** 关闭前收尾（派生类先结束拾取等会话，再调基类完成隐藏） */
        virtual void prepareClosePopup();
        /** 拓展区互斥联动：文件区展开/收起时调用，派生类据此收起自身拓展（如绘图预览） */
        virtual void onFilePanelToggled(bool /*importPanel*/, bool /*open*/) {}
        /** 基类默认在弹窗 Resize 后重新吸附定位；派生类叠加操作格对齐等逻辑时重写并调基类 */
        virtual void onPopupResized();
        /** 弹窗 show 完成后的通知（首开/重开均触发）：派生类在此做依赖可见布局的重建
            （如空态占位行 span、Stretch 列宽），避免隐藏期按错误尺寸计算 */
        virtual void onPopupShown() {}

        // ---- 派生类可复用的操作 ----
        /** 显示底部内联提示（空串收起）；错误态红色 */
        void showInlineMessage(const QString& text, bool error);
        /** 展开/收起导入或导出文件设置面板（互斥），并弹性调整弹窗高度 */
        void toggleFilePanel(bool importPanel);
        /** 收起导入/导出面板及入口选中态（打开弹窗干净态、关闭弹窗、派生类互斥时复用） */
        void closeFilePanels();
        /** 当前可见文件拓展区高度（contentSizeHint 内读取） */
        int currentExtensionHeight() const;
        bool isMsgVisible() const;
        /** 宿主销毁期判定：本控件监视的顶层窗口已销毁/应用退出即 true，只做纯界面收尾 */
        bool isShuttingDown() const;
        /** 监视宿主顶层窗口销毁（构造与 openPopup reparent 时调用） */
        void watchHost(QWidget* host);
        /** 入口选中态同步（派生类自定义开关路径时保持入口态一致） */
        void setEntryChecked(bool checked);

        QString _title;
        // 弹窗子树（最终所有者为宿主顶层窗口，宿主先销毁时裸指针全部野化，
        // 统一 QPointer 托管：随宿主销毁自动置空，判空才真正可靠）
        QPointer<QWidget> _popup;
        QPointer<QLabel> _rowCountLabel;   // 窗口头右侧元信息（N 行等）
        QPointer<QWidget> _msgWrap;        // 底部内联提示消息条

        // 工具栏：导入 导出 … [业务按钮]；派生类开关拾取等形态时整体隐藏
        QPointer<QWidget> _footer;
        QPointer<QPushButton> _importBtn;
        QPointer<QPushButton> _exportBtn;
        QPointer<QPushButton> _actionBtn;

        /** 派生类显式触发吸附定位（内容尺寸变化后） */
        void repositionPopup();
        /** 首开几何就绪标记读写（兼容旧派生类的 markPopupReady 语义） */
        bool geomReady() const { return _geomReady; }
        void markGeomReady() { _geomReady = true; }

        // 文件设置面板（导入/导出，互斥展开；派生类拾取等形态下按需隐藏）
        QPointer<QWidget> _importPanel;
        QPointer<QWidget> _exportPanel;

    private:
        /** 首次打开时创建弹窗（宿主此时已在顶层窗口中），幂等 */
        void ensurePopup();
        void buildEntry();
        void buildPopup();
        void openPopup();
        void positionPopup();
        /** 按 contentSizeHint 完成视口约束与弹窗 resize（高度随内容增长，触及视口内部滚动） */
        void adjustPopupGeometry();
        /** 重开/关闭时收起全部拓展区（文件区、消息条、入口选中态），保证干净状态 */
        void resetExtensions();

        void onBrowseImport();
        void onExecuteImport();
        void onBrowseExport();
        void onExecuteExport();

        // 入口（this 的子对象，与控件同生共死；detachEntryWidget 后所有权移交宿主）
        QPushButton* _entryBtn = nullptr;
        QLabel* _entrySummary = nullptr;
        // 弹窗锚定部件（外部设置，不持有所有权），为空时回退入口按钮
        QPointer<QWidget> _anchor;

        QPointer<QLabel> _popupTitle;

        QPointer<QLineEdit> _importNameEdit;
        QPointer<QLineEdit> _importStartRow;
        QPointer<QLineEdit> _importEndRow;
        QPointer<QLineEdit> _importStartCol;
        QPointer<QComboBox> _importMode;
        QPointer<QPushButton> _execImportBtn;
        QString _importFilePath;
        QPointer<QLineEdit> _exportNameEdit;
        QString _exportFilePath;
        QPointer<QLineEdit> _messageEdit;

        // 文件区/业务按钮配置：弹窗创建前后均可设置，创建时应用
        bool _filePanelsVisible = true;
        bool _actionVisible = false;
        bool _actionCheckable = true;
        QString _actionText;
        std::function<void()> _actionHandler;
        std::function<void(const ImportParams&)> _importHandler;
        std::function<void(const QString&)> _exportHandler;

        // 宿主顶层窗口销毁监视（实例级，替代进程级全局标志）
        QPointer<QWidget> _watchedHost;
        bool _hostDestroyed = false;

        /** 首开经 0ms 延迟打开的去重标记（toggle 重入保护） */
        bool _pendingOpen = false;
        /** 弹窗是否已完成首开前的几何/布局就绪 */
        bool _geomReady = false;
    };
}
