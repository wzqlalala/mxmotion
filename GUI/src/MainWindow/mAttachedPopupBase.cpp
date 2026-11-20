#include "mAttachedPopupBase.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

namespace MechMainWindow
{
    namespace
    {
        // 2.2.6 吸附式弹窗（标准档）公共尺寸
        constexpr int kHeaderHeight = 26;     // 表头/单行控件高（2.2.6：26px）
        constexpr int kPopupMinWidth = 480;   // 普通态最小窗宽（D8：W = max(480, T+F+S)）
        constexpr int kPopupMargin = 12;      // 数据区左右边距（D8：示例左右各 12px）
        constexpr int kViewportMargin = 12;   // 视口两侧安全留白（D8：左右各 12px）
        constexpr int kFooterHeight = 38;     // 底部工具栏高

        // 吸附式弹窗公共样式：窗口引用 2 窗口统一（边框/圆角/头部），
        // 业务内容（表格/绘图等）样式由派生类样式表补充
        const QString kPopupStyle = QStringLiteral(R"(
            QWidget#mAttachedPopup {
                background: #ffffff;
                border: 1px solid #91caff;
                border-radius: 3px;
            }
            QLabel { color: #424b57; font-size: 12px; background: transparent; }
            QLabel#dialogTitle { font-weight: bold; }
            QLabel#dialogMeta { color: #788391; font-size: 11px; }
            QWidget#dialogHeader {
                background: #e9edf3;
                border: 1px solid #91caff;
                border-bottom: 1px solid #e4e8ee;
                border-top-left-radius: 3px;
                border-top-right-radius: 3px;
            }
            QPushButton#iconBtn {
                background: transparent; border: none; color: #788391;
                font-size: 15px; min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px;
            }
            QPushButton#iconBtn:hover { background: #f5f7fa; color: #424b57; }
            QPushButton#backBtn {
                background: transparent; border: none; color: #1890ff;
                font-size: 20px; min-width: 18px; max-width: 18px; padding: 0px;
            }
            QPushButton#backBtn:hover { color: #40a9ff; }
            QPushButton#btn {
                background: #ffffff; border: 1px solid #dce2e9; border-radius: 2px;
                color: #424b57; font-size: 12px;
                min-height: 24px; max-height: 24px; padding: 0 9px;
            }
            QPushButton#btn:hover { border-color: #91caff; color: #1890ff; background: #f5faff; }
            QPushButton#btn:checked {
                background: #e8f3ff; border-color: #91caff; color: #176ba6;
            }
            QPushButton#btn:disabled { color: #a6afbb; background: #f5f7fa; border-color: #e4e8ee; }
            QLineEdit#message {
                border: none; background: #f5f7fa; color: #5c6674; font-size: 11px;
                min-height: 22px; padding: 0 7px;
            }
        )");

        QPushButton* makeButton(const QString& text, const QString& tip, const QString& objName)
        {
            QPushButton* btn = new QPushButton(text);
            btn->setObjectName(objName);
            btn->setToolTip(tip);
            btn->setCursor(Qt::PointingHandCursor);
            return btn;
        }
    }

    mAttachedPopupBase::mAttachedPopupBase(const QString& title, QWidget* parent)
        : QWidget(parent)
        , _title(title)
    {
        // 构造期只创建入口按钮一行：不建弹窗、不装事件过滤、不调任何虚函数，
        // 宿主面板在主窗口构造链中创建本控件也完全无副作用
        buildEntry();
        watchHost(window());
    }

    mAttachedPopupBase::~mAttachedPopupBase()
    {
        // 弹窗存在说明事件过滤已安装（两者同在 ensurePopup 中建立）
        if (_popup)
        {
            qApp->removeEventFilter(this);
            _popup->removeEventFilter(this);
            delete _popup;   // _popup 为 QPointer，销毁后自动置空
        }
    }

    // ------------------------------------------------------------------
    // 入口按钮
    // ------------------------------------------------------------------
    void mAttachedPopupBase::buildEntry()
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // 入口为一行整体：摘要 + 末尾 › 箭头（L9 入口形态，2.2.6：入口与弹窗标题对应，
        // 打开时入口显示选中状态，重复点击可收起）
        _entryBtn = new QPushButton(this);
        _entryBtn->setFixedHeight(26);
        _entryBtn->setCursor(Qt::PointingHandCursor);
        _entryBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _entryBtn->setCheckable(true);
        _entryBtn->setProperty("attachedEntry", true);
        _entryBtn->setStyleSheet(QStringLiteral(R"(
            QPushButton {
                background: #ffffff; border: 1px solid #dce2e9; border-radius: 2px;
                color: #424b57; font-size: 12px; text-align: left; padding: 0 10px;
            }
            QPushButton:hover, QPushButton:checked {
                border-color: #1890ff; background: #e8f3ff;
            }
            QPushButton:disabled {
                background: #f5f5f5; border-color: #d9d9d9; color: rgba(0, 0, 0, 0.25);
            }
        )"));

        QHBoxLayout* entryLayout = new QHBoxLayout(_entryBtn);
        // 左 8px（在按钮 padding 基础上再加），右 10px，与下拉框的文字/箭头边距对齐
        entryLayout->setContentsMargins(8, 0, 10, 0);
        entryLayout->setSpacing(6);
        _entrySummary = new QLabel(_title, _entryBtn);
        _entrySummary->setAttribute(Qt::WA_TransparentForMouseEvents);
        // 颜色跟随按钮 palette：禁用时自动变灰（与 Qt 原生 setEnabled 置灰观感一致）
        _entrySummary->setStyleSheet(QStringLiteral("font-size: 12px; background: transparent;"));
        _entrySummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QLabel* arrowLabel = new QLabel(QStringLiteral("›"), _entryBtn);
        arrowLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        arrowLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        arrowLabel->setStyleSheet(QStringLiteral("font-size: 14px; background: transparent;"));
        entryLayout->addWidget(_entrySummary, 1);
        entryLayout->addWidget(arrowLabel);

        connect(_entryBtn, &QPushButton::clicked, this, &mAttachedPopupBase::togglePopup);
        layout->addWidget(_entryBtn);
    }

    void mAttachedPopupBase::detachEntryWidget(QWidget* newParent)
    {
        if (!_entryBtn || !newParent) return;
        // 断开的只是「按钮作为控件子对象」的关系：控件本体（含隐藏/禁用状态）
        // 不再处于按钮的父链上，宿主对按钮的 setEnabled 与控件互不干扰
        disconnect(_entryBtn, &QPushButton::clicked, this, &mAttachedPopupBase::togglePopup);
        this->layout()->removeWidget(_entryBtn);
        _entryBtn->setParent(newParent);
        // 剖分后由宿主行布局统一着色：移除内嵌样式表，避免与宿主面板的全局
        // 控件样式叠加出双重描边/底色（恢复离散数据入口的宿主行观感）
        _entryBtn->setStyleSheet(QString());
        if (_entrySummary)
            _entrySummary->setStyleSheet(QStringLiteral("color: #424b57; font-size: 12px; background: transparent;"));
        connect(_entryBtn, &QPushButton::clicked, this, [this]() { togglePopup(); });
        // 弹窗开关同步宿主按钮的选中态（入口已是宿主按钮，checked 即「弹窗展开」）
        connect(this, &mAttachedPopupBase::popupOpened, _entryBtn, [this]() { _entryBtn->setChecked(true); });
        connect(this, &mAttachedPopupBase::popupClosed, _entryBtn, [this]() { _entryBtn->setChecked(false); });
    }

    // ------------------------------------------------------------------
    // 弹窗：首次打开时才创建（幂等）
    // ------------------------------------------------------------------
    void mAttachedPopupBase::preparePopup()
    {
        ensurePopup();
    }

    void mAttachedPopupBase::ensurePopup()
    {
        if (_popup) return;
        QWidget* hostWindow = this->window();
        if (!hostWindow) return;

        _popup = new QWidget(hostWindow, Qt::Tool | Qt::FramelessWindowHint);
        _popup->setObjectName(QStringLiteral("mAttachedPopup"));
        _popup->setStyleSheet(kPopupStyle);
        // WA_StyledBackground：Qt 样式表 border 只在启用样式背景时绘制，
        // 无边框 Tool 窗口默认不启用，顶部 1px 蓝色边框会被裁掉
        _popup->setAttribute(Qt::WA_StyledBackground, true);
        _popup->setAttribute(Qt::WA_DeleteOnClose, false);
        _popup->setAttribute(Qt::WA_QuitOnClose, false);
        _popup->setAttribute(Qt::WA_ShowWithoutActivating);
        // 点击外部关闭/宿主窗口移动跟随所需的全局过滤，与弹窗同生共死
        qApp->installEventFilter(this);
        buildPopup();
        // 提前按当前内容完成隐藏布局：首开直接落位，不再闪现默认尺寸一帧。
        // 不做 resize/move——位置留待首开时按已就位的锚点计算；
        // 面板创建期预建若此刻定位，会因锚点未排版而闪错位置
        _popup->setMinimumSize(QSize(qMin(popupMinimumWidth(), 240), 140));
        if (QLayout* lay = _popup->layout())
            lay->activate();
        _geomReady = false; // 首开时按实际锚点完成定位
    }

    void mAttachedPopupBase::buildPopup()
    {
        QVBoxLayout* main = new QVBoxLayout(_popup);
        main->setContentsMargins(0, 0, 0, 0);
        main->setSpacing(0);

        // ---- 头部：‹ 返回 | 标题 | 元信息 | × 关闭（2.2.6：标准档窗口头 28px，panel 底色）----
        QWidget* header = new QWidget(_popup);
        header->setObjectName(QStringLiteral("dialogHeader"));
        header->setFixedHeight(kHeaderHeight);
        QHBoxLayout* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(6, 0, 6, 0);
        headerLayout->setSpacing(4);
        QPushButton* backBtn = makeButton(QStringLiteral("‹"), QStringLiteral("返回"), QStringLiteral("backBtn"));
        _popupTitle = new QLabel(_title, header);
        _popupTitle->setObjectName(QStringLiteral("dialogTitle"));
        _rowCountLabel = new QLabel(header);
        _rowCountLabel->setObjectName(QStringLiteral("dialogMeta"));
        QPushButton* closeBtn = makeButton(QStringLiteral("✕"), QStringLiteral("关闭"), QStringLiteral("iconBtn"));
        headerLayout->addWidget(backBtn);
        headerLayout->addWidget(_popupTitle);
        headerLayout->addStretch();
        headerLayout->addWidget(_rowCountLabel);
        headerLayout->addWidget(closeBtn);
        main->addWidget(header);
        connect(backBtn, &QPushButton::clicked, this, [this]() { closePopup(); });
        connect(closeBtn, &QPushButton::clicked, this, [this]() { closePopup(); });

        // ---- 业务内容区（派生类填充；此时派生类已构造完成，可安全走虚函数）----
        buildContent(_popup, main);

        // ---- 底部工具栏：导入 导出 … [业务按钮] ----
        _footer = new QWidget(_popup);
        _footer->setFixedHeight(kFooterHeight);
        QHBoxLayout* footerLayout = new QHBoxLayout(_footer);
        footerLayout->setContentsMargins(kPopupMargin, 6, kPopupMargin, 6);
        footerLayout->setSpacing(6);
        _importBtn = makeButton(QStringLiteral("导入"), QStringLiteral("从文件导入数据"), QStringLiteral("btn"));
        _exportBtn = makeButton(QStringLiteral("导出"), QStringLiteral("导出为 CSV 文件"), QStringLiteral("btn"));
        // 展开文件设置面板时按钮呈选中态（checkable + :checked 浅蓝底）
        _importBtn->setCheckable(true);
        _exportBtn->setCheckable(true);
        _importBtn->setVisible(_filePanelsVisible);
        _exportBtn->setVisible(_filePanelsVisible);
        _actionBtn = makeButton(_actionText, QString(), QStringLiteral("btn"));
        _actionBtn->setCheckable(_actionCheckable);
        _actionBtn->setVisible(_actionVisible);
        footerLayout->addWidget(_importBtn);
        footerLayout->addWidget(_exportBtn);
        footerLayout->addStretch();
        footerLayout->addWidget(_actionBtn);
        // 工具栏整体可见性：文件区与业务按钮全隐藏时不占位
        _footer->setVisible(_filePanelsVisible || _actionVisible);
        main->addWidget(_footer);

        // ---- 文件设置面板（导入/导出，互斥向下展开；功能接口预留）----
        // 2.2.6 文件区：左右留白与数据区一致（12px）；导入设置单行（起始行/结束行/起始列/方式），
        // 标签贴近字段 4px、项间 6px；数字框最小 72px、等宽字体、容纳 8 位有效数字；
        // 视口不足整行横向滚动，不换行
        const QString fieldStyle = QStringLiteral(
            "QLineEdit#field, QComboBox#field { border: 1px solid #dce2e9; border-radius: 2px;"
            " background: #ffffff; color: #424b57; min-height: 24px; max-height: 24px; padding: 0 4px; }"
            "QLineEdit#field[readOnly=\"true\"] { background: #fafbfd; color: #788391; }"
            "QLineEdit#importNum { font-family: Consolas, monospace; min-width: 72px; }"
            "QComboBox#importModeField { min-width: 64px; }"
            "QLabel#fieldLabel { color: #5c6674; font-size: 11px; }"
            "QLabel#fileNote { color: #788391; font-size: 11px; }");

        auto makeField = [](const QString& text = QString()) {
            QLineEdit* edit = new QLineEdit(text);
            edit->setObjectName(QStringLiteral("field"));
            return edit;
        };

        // 导入面板：文件路径 → 文件夹图标 → 蓝色导入；下方单行范围选项
        _importPanel = new QWidget(_popup);
        _importPanel->setObjectName(QStringLiteral("filePanel"));
        _importPanel->setStyleSheet(fieldStyle + QStringLiteral(
            "QWidget#filePanel { background: #ffffff; border: 1px solid #91caff; border-top: 1px solid #e4e8ee; }"));
        QVBoxLayout* importLayout = new QVBoxLayout(_importPanel);
        importLayout->setContentsMargins(kPopupMargin, 8, kPopupMargin, 8);
        importLayout->setSpacing(8);
        {
            // 路径行：文件路径弹性宽度 + 文件夹图标 + 执行按钮（路径选择及执行操作保持可见）
            QHBoxLayout* row1 = new QHBoxLayout();
            row1->setSpacing(6);
            QLabel* nameLabel = new QLabel(QStringLiteral("文件路径"), _importPanel);
            nameLabel->setObjectName(QStringLiteral("fieldLabel"));
            nameLabel->setFixedWidth(50);
            _importNameEdit = makeField();
            _importNameEdit->setReadOnly(true);
            _importNameEdit->setPlaceholderText(QStringLiteral("请选择 CSV / TXT / XYZ 文件"));
            QPushButton* browseBtn = makeButton(QStringLiteral("📂"), QStringLiteral("选择导入文件"), QStringLiteral("btn"));
            browseBtn->setFixedWidth(30);
            _execImportBtn = makeButton(QStringLiteral("导入"), QStringLiteral("执行导入"), QStringLiteral("btn"));
            _execImportBtn->setStyleSheet(QStringLiteral(
                "QPushButton { background: #1890ff; border: 1px solid #1890ff; border-radius: 2px;"
                " color: #ffffff; min-height: 24px; max-height: 24px; padding: 0 12px; }"
                "QPushButton:hover { background: #40a9ff; }"));
            row1->addWidget(nameLabel);
            row1->addWidget(_importNameEdit, 1);
            row1->addWidget(browseBtn);
            row1->addWidget(_execImportBtn);
            importLayout->addLayout(row1);

            // 导入设置固定一行：起始行、结束行、起始列、方式（2.2.6 文件区；标签 4px、项间 6px）
            QHBoxLayout* row2 = new QHBoxLayout();
            row2->setSpacing(6);
            auto optLabel = [this](const QString& t) {
                QLabel* l = new QLabel(t, _importPanel);
                l->setObjectName(QStringLiteral("fieldLabel"));
                return l;
            };
            auto optPair = [&row2](QLabel* label, QWidget* field, int stretch) {
                label->setContentsMargins(0, 0, 0, 0);
                QHBoxLayout* pair = new QHBoxLayout();
                pair->setContentsMargins(0, 0, 0, 0);
                pair->setSpacing(4); // 标签贴近字段 4px
                pair->addWidget(label);
                pair->addWidget(field, stretch);
                row2->addLayout(pair);
            };
            _importStartRow = makeField(QStringLiteral("1"));
            _importStartRow->setObjectName(QStringLiteral("importNum"));
            _importEndRow = makeField();
            _importEndRow->setObjectName(QStringLiteral("importNum"));
            _importEndRow->setPlaceholderText(QStringLiteral("末行"));
            _importStartCol = makeField(QStringLiteral("1"));
            _importStartCol->setObjectName(QStringLiteral("importNum"));
            _importMode = new QComboBox(_importPanel);
            _importMode->setObjectName(QStringLiteral("importModeField"));
            _importMode->addItems({ QStringLiteral("覆盖"), QStringLiteral("追加") });
            optPair(optLabel(QStringLiteral("起始行")), _importStartRow, 1);
            optPair(optLabel(QStringLiteral("结束行")), _importEndRow, 1);
            optPair(optLabel(QStringLiteral("起始列")), _importStartCol, 1);
            optPair(optLabel(QStringLiteral("方式")), _importMode, 1);
            importLayout->addLayout(row2);

            QLabel* note = new QLabel(
                QStringLiteral("结束行留空读至末行；从起始列按表格列数读取。支持逗号、制表符或空格分隔。"), _importPanel);
            note->setObjectName(QStringLiteral("fileNote"));
            importLayout->addWidget(note);

            connect(browseBtn, &QPushButton::clicked, this, &mAttachedPopupBase::onBrowseImport);
            connect(_execImportBtn, &QPushButton::clicked, this, &mAttachedPopupBase::onExecuteImport);
        }
        _importPanel->setVisible(false);
        main->addWidget(_importPanel);

        // 导出面板：文件路径 → 文件夹图标 → 蓝色导出（2.2.6：路径行弹性宽度）
        _exportPanel = new QWidget(_popup);
        _exportPanel->setObjectName(QStringLiteral("filePanel"));
        _exportPanel->setStyleSheet(fieldStyle + QStringLiteral(
            "QWidget#filePanel { background: #ffffff; border: 1px solid #91caff; border-top: 1px solid #e4e8ee; }"));
        QVBoxLayout* exportLayout = new QVBoxLayout(_exportPanel);
        exportLayout->setContentsMargins(kPopupMargin, 8, kPopupMargin, 8);
        exportLayout->setSpacing(8);
        {
            QHBoxLayout* row1 = new QHBoxLayout();
            row1->setSpacing(6);
            QLabel* nameLabel = new QLabel(QStringLiteral("文件路径"), _exportPanel);
            nameLabel->setObjectName(QStringLiteral("fieldLabel"));
            nameLabel->setFixedWidth(50);
            _exportNameEdit = makeField(_title + QStringLiteral(".csv"));
            QPushButton* browseBtn = makeButton(QStringLiteral("📂"), QStringLiteral("选择导出位置"), QStringLiteral("btn"));
            browseBtn->setFixedWidth(30);
            QPushButton* execBtn = makeButton(QStringLiteral("导出"), QStringLiteral("执行导出"), QStringLiteral("btn"));
            execBtn->setStyleSheet(QStringLiteral(
                "QPushButton { background: #1890ff; border: 1px solid #1890ff; border-radius: 2px;"
                " color: #ffffff; min-height: 24px; max-height: 24px; padding: 0 12px; }"
                "QPushButton:hover { background: #40a9ff; }"));
            row1->addWidget(nameLabel);
            row1->addWidget(_exportNameEdit, 1);
            row1->addWidget(browseBtn);
            row1->addWidget(execBtn);
            exportLayout->addLayout(row1);

            QLabel* note = new QLabel(
                QStringLiteral("导出为带表头的 CSV，可通过文件夹图标选择保存位置。"), _exportPanel);
            note->setObjectName(QStringLiteral("fileNote"));
            exportLayout->addWidget(note);

            connect(browseBtn, &QPushButton::clicked, this, &mAttachedPopupBase::onBrowseExport);
            connect(execBtn, &QPushButton::clicked, this, &mAttachedPopupBase::onExecuteExport);
        }
        _exportPanel->setVisible(false);
        main->addWidget(_exportPanel);

        connect(_importBtn, &QPushButton::clicked, this, [this]() { toggleFilePanel(true); });
        connect(_exportBtn, &QPushButton::clicked, this, [this]() { toggleFilePanel(false); });
        connect(_actionBtn, &QPushButton::clicked, this, [this]() {
            if (_actionHandler) _actionHandler();
            });

        // ---- 底部内联提示消息条 ----
        _msgWrap = new QWidget(_popup);
        QHBoxLayout* msgLayout = new QHBoxLayout(_msgWrap);
        msgLayout->setContentsMargins(kPopupMargin, 0, kPopupMargin, 8);
        _messageEdit = new QLineEdit(_msgWrap);
        _messageEdit->setObjectName(QStringLiteral("message"));
        _messageEdit->setReadOnly(true);
        msgLayout->addWidget(_messageEdit);
        _msgWrap->setVisible(false);
        main->addWidget(_msgWrap);
    }

    // ------------------------------------------------------------------
    // 弹窗开关与定位
    // ------------------------------------------------------------------
    void mAttachedPopupBase::togglePopup()
    {
        if (_popup && _popup->isVisible())
        {
            closePopup();
            return;
        }
        if (!_popup && !_geomReady && !_pendingOpen)
        {
            // 首次点击：先创建弹窗，推迟到事件循环空闲再真正打开。
            // 避免在按钮 clicked 的同步栈里 show() 时尺寸/位置尚未最终稳定（首开闪烁来源）。
            // 关键时序：首开必须在 show() 之后再做内容布局——隐藏期 Stretch 列不分配
            // 实际宽度，提前按错误宽度定位/灌数据会导致空态占位行 span、列宽、行高
            // 全部错位（首开「尚未添加…」不居中、操作行被挤压即源于此）。
            ensurePopup();
            if (!_popup) return;
            _pendingOpen = true;
            QTimer::singleShot(0, this, [this]() {
                _pendingOpen = false;
                if (_popup && !_popup->isVisible())
                    openPopup();
                });
            return;
        }
        if (_popup) openPopup();
    }

    void mAttachedPopupBase::openPopup()
    {
        // 弹窗可能已随宿主主窗口销毁（QPointer 置空），此时入口按钮点击不得崩溃
        if (!_popup) return;
        const bool geometryReady = _geomReady && _popup->parentWidget() == window();
        // 重新绑定宿主主窗口（面板可能被挂到不同顶层窗口），保证弹窗始终在主窗口之上
        QWidget* host = window();
        if (host && _popup->parentWidget() != host)
            _popup->setParent(host, Qt::Tool | Qt::FramelessWindowHint);
        // 控件迁到新顶层窗口后，宿主销毁监视同步迁移
        watchHost(host);
        // 上次关闭前的提示消息/文件面板残留收起，重开为干净状态
        resetExtensions();
        // 首开必须先 show 再做内容布局：隐藏期 Stretch 列不分配实际宽度，
        // 提前定位/灌数据会让空态占位行 span、列宽、行高按错误尺寸计算
        _popup->show();
        _popup->raise();
        positionPopup();
        // 视图布局完成后让派生类重建依赖可见性的结构（空态 span、列宽、行高）
        onPopupShown();
        positionPopup(); // 内容尺寸可能因重建变化，再吸附一次
        if (!geometryReady)
            positionPopup(); // 内容就绪后补一次定位，保证首开即最终位置
        _geomReady = true;
        emit popupOpened();
    }

    void mAttachedPopupBase::prepareClosePopup()
    {
        // 基类无额外收尾；派生类（如拾取会话）重写
    }

    void mAttachedPopupBase::closePopup()
    {
        if (!_popup) return;
        prepareClosePopup();
        if (_popup && _popup->isVisible())
        {
            _popup->hide();
            emit popupClosed();
        }
        if (_entryBtn) _entryBtn->setChecked(false);
        resetExtensions();
    }

    bool mAttachedPopupBase::isPopupOpen() const
    {
        return _popup && _popup->isVisible();
    }

    void mAttachedPopupBase::resetExtensions()
    {
        // 关闭弹窗/重开时收起文件设置面板与消息条并退出入口选中态，保持干净状态
        showInlineMessage(QString(), false);
        if (_importPanel) _importPanel->setVisible(false);
        if (_exportPanel) _exportPanel->setVisible(false);
        if (_importBtn) _importBtn->setChecked(false);
        if (_exportBtn) _exportBtn->setChecked(false);
    }

    void mAttachedPopupBase::closeFilePanels()
    {
        if (_importPanel) _importPanel->setVisible(false);
        if (_exportPanel) _exportPanel->setVisible(false);
        if (_importBtn) _importBtn->setChecked(false);
        if (_exportBtn) _exportBtn->setChecked(false);
    }

    void mAttachedPopupBase::positionPopup()
    {
        // 锚点：默认入口按钮；入口剖分嵌入宿主行时锚定整行
        QWidget* anchor = (_anchor ? _anchor.data() : _entryBtn);
        if (!_popup || !anchor) return;
        adjustPopupGeometry();
        const QSize size = _popup->size();
        const QPoint anchorRight = anchor->mapToGlobal(QPoint(anchor->width(), 0));
        QScreen* screen = QApplication::screenAt(anchorRight);
        if (!screen) screen = QApplication::primaryScreen();
        const QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

        // 2.2.6 位置与边界：优先靠近入口侧向展开；空间不足时调整到可用侧并约束于视口
        int x = anchorRight.x() + 10;
        if (x + size.width() > avail.right() - 12)
        {
            const int leftX = anchor->mapToGlobal(QPoint(0, 0)).x() - size.width() - 10;
            x = (leftX >= avail.left() + 12) ? leftX : (avail.right() - 12 - size.width());
        }
        int y = anchorRight.y();
        if (y + size.height() > avail.bottom() - 12)
            y = qMax(avail.top() + 12, avail.bottom() - 12 - size.height());
        _popup->move(x, y);

        // 兜底拉回可视区域。注意不能用 intersected：交集语义会把弹出屏幕的弹窗
        // 裁剪变小（甚至清零），此处只平移位置、保留弹窗自身尺寸
        const QRect g = _popup->geometry();
        int gx = qBound(avail.left() + 12, g.x(), qMax(avail.left() + 12, avail.right() - 12 - g.width() + 1));
        int gy = qBound(avail.top() + 12, g.y(), qMax(avail.top() + 12, avail.bottom() - 12 - g.height() + 1));
        if (gx != g.x() || gy != g.y())
            _popup->move(gx, gy);
    }

    void mAttachedPopupBase::adjustPopupGeometry()
    {
        if (!_popup) return;
        const QSize content = contentSizeHint();

        // 视口约束（D8：实际窗宽 ≤ 可用视口 − 左右各 12px），受限时内容区内部滚动；
        // 宽高共用一次屏幕查询，视口找不到时退化为不约束
        int maxPopupWidth = 1 << 20;
        int maxPopupHeight = 1 << 20;
        if (QScreen* screen = QApplication::screenAt(_popup->isVisible()
                ? _popup->frameGeometry().center()
                : (_anchor ? _anchor.data() : _entryBtn)->mapToGlobal(QPoint())))
        {
            const QRect avail = screen->availableGeometry();
            // 可用视口极小（极端多屏/虚拟分辨率）时保底可交互的最小宽高
            maxPopupWidth = qMax(avail.width() - 2 * kViewportMargin, 240);
            maxPopupHeight = qMax(avail.height() - 2 * kViewportMargin, 160);
        }

        int popupWidth = content.width() + 2; // 弹窗 1px 边框
        popupWidth = qMax(popupWidth, popupMinimumWidth());
        popupWidth = qMin(popupWidth, maxPopupWidth);

        const int msgHeight = _msgWrap && _msgWrap->isVisible() ? 32 : 0;
        const int footerHeight = _footer && _footer->isVisible() ? kFooterHeight : 0;
        int popupHeight = kHeaderHeight + content.height() + footerHeight
            + currentExtensionHeight() + msgHeight;
        // 高度随内容向下增长，触及视口边界后内部滚动（D8：不预留空滚动槽）
        popupHeight = qMin(popupHeight, maxPopupHeight);

        // 弹窗尺寸完全由内容决定（不支持用户拖拽调整）：数据增长时按内容重新自适应
        const QSize minimum(qMin(popupMinimumWidth(), maxPopupWidth), 140);
        _popup->setMinimumSize(minimum);
        _popup->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        _popup->resize(popupWidth, qMax(popupHeight, minimum.height()));
    }

    int mAttachedPopupBase::popupMinimumWidth() const
    {
        return kPopupMinWidth;
    }

    int mAttachedPopupBase::currentExtensionHeight() const
    {
        return (_importPanel && _importPanel->isVisible()) ? _importPanel->sizeHint().height()
            : (_exportPanel && _exportPanel->isVisible()) ? _exportPanel->sizeHint().height()
            : 0;
    }

    bool mAttachedPopupBase::isMsgVisible() const
    {
        return _msgWrap && _msgWrap->isVisible();
    }

    void mAttachedPopupBase::setEntryChecked(bool checked)
    {
        if (_entryBtn) _entryBtn->setChecked(checked);
    }

    void mAttachedPopupBase::onPopupResized()
    {
        // 基类无需额外处理；派生类叠加操作格对齐等逻辑时重写
    }

    void mAttachedPopupBase::repositionPopup()
    {
        if (_popup && _popup->isVisible())
            positionPopup();
    }

    // ------------------------------------------------------------------
    // 宿主销毁期判定（实例级：只盯本控件监视的顶层窗口，不用进程级全局标志）
    // ------------------------------------------------------------------
    void mAttachedPopupBase::watchHost(QWidget* host)
    {
        if (!host || host == _watchedHost) return;
        _watchedHost = host;
        _hostDestroyed = false;
        // 带 context 的连接：旧 host 销毁或本控件析构时任一侧断开，不会误触发
        connect(host, &QObject::destroyed, this, [this]() { _hostDestroyed = true; });
    }

    bool mAttachedPopupBase::isShuttingDown() const
    {
        // 关闭流程中只允许纯界面收尾，不得触碰视口/渲染/全局单例。
        // 全部为 O(1) 判定，任何事件路径调用都无性能负担。
        if (!qApp || qApp->closingDown())
            return true;
        return _hostDestroyed;
    }

    // ------------------------------------------------------------------
    // 事件过滤：点击外部关闭；宿主窗口移动时跟随
    // ------------------------------------------------------------------
    bool mAttachedPopupBase::eventFilter(QObject* watched, QEvent* event)
    {
        // 宿主主窗口销毁期防御：面板单例随 qApp 在主窗口之后析构，其 eventFilter
        // 已摘除，但全局仍有其它存活实例的过滤器，会继续收到宿主子控件（含本控件
        // 已半销毁的弹窗子树）的事件。此时本控件的成员指针可能已野化，任何成员
        // 解引用都属 UB——宿主已销毁时直接放行，不做任何成员访问。
        if (isShuttingDown())
            return QWidget::eventFilter(watched, event);

        if (watched == _popup && event->type() == QEvent::Close)
        {
            // closePopup 内的同步信号槽可能 delete 本控件；WA_DeleteOnClose=false
            // 时 hide 即可，无论是否存活都吞掉 Close，不走尾部基类虚调用
            closePopup();
            return true;
        }

        if (watched == _popup && event->type() == QEvent::Resize)
            onPopupResized();

        if (!_popup || !_popup->isVisible())
            return QWidget::eventFilter(watched, event);

        if (event->type() == QEvent::MouseButtonPress)
        {
            QWidget* w = qobject_cast<QWidget*>(watched);
            if (w)
            {
                const bool onPopup = (w == _popup || _popup->isAncestorOf(w));
                const bool onEntry = _entryBtn && (w == _entryBtn || _entryBtn->isAncestorOf(w));
                // 内容区 QComboBox 的下拉列表是独立顶级窗口（既不在弹窗子树内、
                // 也不是 entry 子控件），点击其中条目不得按「点击外部」关弹窗
                bool onComboPopup = false;
                for (QComboBox* combo : _popup->findChildren<QComboBox*>())
                {
                    if (!combo || !combo->view()) continue;
                    QWidget* popup = combo->view()->window();
                    if (popup && popup != this->window() && (w == popup || popup->isAncestorOf(w)))
                    {
                        onComboPopup = true;
                        break;
                    }
                }
                if (!onPopup && !onEntry && !onComboPopup)
                {
                    QPointer<mAttachedPopupBase> guard(this);
                    closePopup();
                    // closePopup 内同步信号槽可能 delete 本控件
                    if (!guard) return true;
                }
            }
        }
        else if (event->type() == QEvent::Move)
        {
            // 宿主顶层窗口移动时弹窗跟随锚点（入口被嵌入宿主行时入口恒不可见，
            // 不能以入口可见性判断，改用锚点）
            QWidget* w = qobject_cast<QWidget*>(watched);
            if (w && w->isWindow() && w == this->window())
            {
                QWidget* anchor = (_anchor ? _anchor.data() : _entryBtn);
                if (anchor && anchor->isVisible())
                {
                    positionPopup();
                }
                else
                {
                    QPointer<mAttachedPopupBase> guard(this);
                    closePopup();
                    if (!guard) return true;
                }
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    // ------------------------------------------------------------------
    // 导入/导出文件设置面板（界面完成，功能接口预留）
    // ------------------------------------------------------------------
    void mAttachedPopupBase::toggleFilePanel(bool importPanel)
    {
        QWidget* target = importPanel ? _importPanel.data() : _exportPanel.data();
        QWidget* other = importPanel ? _exportPanel.data() : _importPanel.data();
        if (!target) return;
        const bool opening = !target->isVisible();
        if (other) other->setVisible(false);
        target->setVisible(opening);
        // 选中态同步：展开的入口按钮呈淡蓝选中，收起的恢复常态（互斥，只一个选中）
        if (_importBtn) _importBtn->setChecked(importPanel && opening);
        if (_exportBtn) _exportBtn->setChecked(!importPanel && opening);
        showInlineMessage(QString(), false);
        if (opening && importPanel && _importStartRow && _importEndRow && _importStartCol && _importMode)
        {
            // 每次展开恢复默认范围参数，已选文件保留
            _importStartRow->setText(QStringLiteral("1"));
            _importEndRow->clear();
            _importStartCol->setText(QStringLiteral("1"));
            _importMode->setCurrentIndex(0);
        }
        if (opening && !importPanel && _exportNameEdit && _exportNameEdit->text().trimmed().isEmpty())
        {
            _exportNameEdit->setText(_title + QStringLiteral(".csv"));
        }
        // 拓展区互斥：派生类收起自身拓展（如绘图预览）
        onFilePanelToggled(importPanel, opening);
        if (_popup && _popup->isVisible())
            positionPopup();
    }

    void mAttachedPopupBase::onBrowseImport()
    {
        const QString path = QFileDialog::getOpenFileName(
            _popup, QStringLiteral("选择导入文件"), QString(),
            QStringLiteral("数据文件 (*.csv *.txt *.xyz);;所有文件 (*)"));
        if (path.isEmpty()) return;
        _importFilePath = path;
        _importNameEdit->setText(QFileInfo(path).fileName());
        _importNameEdit->setToolTip(path);
        showInlineMessage(QString(), false);
    }

    void mAttachedPopupBase::onExecuteImport()
    {
        if (_importFilePath.isEmpty())
        {
            showInlineMessage(QStringLiteral("请先选择导入文件。"), true);
            return;
        }
        ImportParams params;
        params.filePath = _importFilePath;
        params.startRow = qMax(1, _importStartRow->text().trimmed().toInt());
        params.endRow = _importEndRow->text().trimmed().isEmpty() ? -1 : _importEndRow->text().trimmed().toInt();
        params.startColumn = qMax(1, _importStartCol->text().trimmed().toInt());
        params.append = (_importMode->currentIndex() == 1);
        if (_importHandler)
        {
            _importHandler(params);
        }
        else
        {
            showInlineMessage(QStringLiteral("导入功能接口已预留，待宿主接入。"), false);
        }
    }

    void mAttachedPopupBase::onBrowseExport()
    {
        QString suggested = _exportNameEdit->text().trimmed();
        if (suggested.isEmpty()) suggested = _title + QStringLiteral(".csv");
        const QString path = QFileDialog::getSaveFileName(
            _popup, QStringLiteral("选择导出位置"), suggested,
            QStringLiteral("CSV 文件 (*.csv)"));
        if (path.isEmpty()) return;
        _exportFilePath = path;
        _exportNameEdit->setText(QFileInfo(path).fileName());
        _exportNameEdit->setToolTip(path);
        showInlineMessage(QString(), false);
    }

    void mAttachedPopupBase::onExecuteExport()
    {
        QString path = _exportFilePath;
        if (path.isEmpty())
        {
            // 未选择保存位置时按浏览器式回退：弹出保存对话框
            QString suggested = _exportNameEdit->text().trimmed();
            if (suggested.isEmpty()) suggested = _title + QStringLiteral(".csv");
            path = QFileDialog::getSaveFileName(
                _popup, QStringLiteral("导出数据"), suggested, QStringLiteral("CSV 文件 (*.csv)"));
            if (path.isEmpty()) return;
            _exportFilePath = path;
        }
        if (_exportHandler)
        {
            _exportHandler(path);
        }
        else
        {
            showInlineMessage(QStringLiteral("导出功能接口已预留，待宿主接入。"), false);
        }
    }

    void mAttachedPopupBase::showInlineMessage(const QString& text, bool error)
    {
        if (!_messageEdit || !_msgWrap) return;
        _messageEdit->setText(text);
        _messageEdit->setStyleSheet(error
            ? QStringLiteral("QLineEdit#message { color: #d95151; background: #fff5f5; }")
            : QString());
        const bool show = !text.isEmpty();
        if (_msgWrap->isVisible() != show)
        {
            _msgWrap->setVisible(show);
            if (_popup && _popup->isVisible())
                positionPopup();
        }
    }

    // ------------------------------------------------------------------
    // 对外接口
    // ------------------------------------------------------------------
    void mAttachedPopupBase::setPopupTitle(const QString& title)
    {
        _title = title;
        if (_popupTitle) _popupTitle->setText(title);
    }

    void mAttachedPopupBase::setMetaText(const QString& text)
    {
        if (_rowCountLabel) _rowCountLabel->setText(text);
    }

    void mAttachedPopupBase::setFilePanelsVisible(bool visible)
    {
        _filePanelsVisible = visible;
        if (_importBtn) _importBtn->setVisible(visible);
        if (_exportBtn) _exportBtn->setVisible(visible);
        if (!visible) closeFilePanels();
        if (_footer) _footer->setVisible(visible || _actionVisible);
    }

    void mAttachedPopupBase::setActionButtonVisible(bool visible, const QString& text, bool checkable)
    {
        _actionVisible = visible;
        if (!text.isEmpty()) _actionText = text;
        _actionCheckable = checkable;
        if (_actionBtn)
        {
            _actionBtn->setVisible(visible);
            if (!text.isEmpty()) _actionBtn->setText(text);
            _actionBtn->setCheckable(checkable);
        }
        if (_footer) _footer->setVisible(_filePanelsVisible || visible);
    }

    void mAttachedPopupBase::setActionButtonChecked(bool checked)
    {
        if (_actionBtn) _actionBtn->setChecked(checked);
    }

    void mAttachedPopupBase::setActionButtonEnabled(bool enabled)
    {
        if (_actionBtn) _actionBtn->setEnabled(enabled);
    }

    void mAttachedPopupBase::setImportHandler(const std::function<void(const ImportParams&)>& handler) { _importHandler = handler; }
    void mAttachedPopupBase::setExportHandler(const std::function<void(const QString&)>& handler) { _exportHandler = handler; }
    void mAttachedPopupBase::setActionHandler(const std::function<void()>& handler) { _actionHandler = handler; }
}
