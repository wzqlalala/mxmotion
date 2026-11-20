#include "mApplyStyleDlg.h"
#include "CurveCore/CurveDocument.h"
#include "CurveCore/mCurveDocController.h"
#include "mDlgGeometry.h"
#include "mTreeDelegate.h"   //曲线树同款虚线分支绘制（PaintDashBranches）

#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizeGrip>
#include <QToolButton>
#include <QTreeWidget>
#include <QUndoStack>
#include <QVBoxLayout>

namespace PostCurve
{
    mApplyStyleDlg* mApplyStyleDlg::_instance = nullptr;

    namespace
    {
        //--- 选项叶子键（构建与查询两处共用，防拼写漂移）。
        //页眉/页脚侧别独立成键，允许只勾一侧 ---
        const char* const kKeyPlotBg = "plot.background";
        const char* const kKeyPlotGrid = "plot.grid";
        const char* const kKeyLegendFont = "legend.font";
        const char* const kKeyLegendBorder = "legend.border";
        const char* const kKeyLegendColor = "legend.color";
        const char* const kKeyLegendPos = "legend.position";
        const char* const kKeyLegendVis = "legend.visible";
        const char* const kKeyHeaderFont = "header.font";
        const char* const kKeyHeaderColor = "header.color";
        const char* const kKeyHeaderAlign = "header.align";
        const char* const kKeyHeaderVis = "header.visible";
        const char* const kKeyFooterFont = "footer.font";
        const char* const kKeyFooterColor = "footer.color";
        const char* const kKeyFooterAlign = "footer.align";
        const char* const kKeyFooterVis = "footer.visible";
        const char* const kKeyAxisX = "axis.applyX";
        const char* const kKeyAxisY = "axis.applyY";
        const char* const kKeyAxisY2 = "axis.applyY2";
        const char* const kKeyAxisTitleFont = "axis.titleFont";
        const char* const kKeyAxisLabelFont = "axis.labelFont";
        const char* const kKeyAxisColor = "axis.color";
        const char* const kKeyMarkScale = "mark.scale";
        const char* const kKeyMarkMin = "mark.min";
        const char* const kKeyMarkMax = "mark.max";
        const char* const kKeyMarkStep = "mark.step";
        const char* const kKeyMarkGrid = "mark.grid";
        const char* const kKeyMarkAuto = "mark.auto";
        const char* const kKeyCurveLineStyle = "curve.lineStyle";
        const char* const kKeyCurveColor = "curve.color";
        const char* const kKeyCurveWidth = "curve.lineWidth";
        const char* const kKeyCurveSymStyle = "curve.symbolStyle";
        const char* const kKeyCurveSymSize = "curve.symbolSize";
        const char* const kKeyCurveSymColor = "curve.symbolColor";
        const char* const kKeyCurveAreaFill = "curve.areaFill";

        //--- 复选树通用操作（QTreeWidget 无内置父子联动；勾选仅向下级联、
        //不向上回算，点任一复选框只影响自身与后代） ---
        Qt::ItemFlags checkableItemFlags()
        {
            return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        }

        //子节点跟随本节点勾选状态（递归下发）
        void cascadeCheckToChildren(QTreeWidgetItem* item)
        {
            const Qt::CheckState state = item->checkState(0);
            for (int i = 0; i < item->childCount(); ++i)
            {
                QTreeWidgetItem* child = item->child(i);
                child->setCheckState(0, state);
                cascadeCheckToChildren(child);
            }
        }

        //按子节点回算自身三态（全勾=勾，部分=半选，全空=空）
        void recomputeOwnCheckState(QTreeWidgetItem* item)
        {
            if (item->childCount() == 0)
                return;
            bool any = false;
            bool all = true;
            for (int i = 0; i < item->childCount(); ++i)
            {
                const Qt::CheckState s = item->child(i)->checkState(0);
                if (s != Qt::Unchecked) any = true;
                if (s != Qt::Checked) all = false;
            }
            item->setCheckState(0, all ? Qt::Checked : (any ? Qt::PartiallyChecked : Qt::Unchecked));
        }

        //整棵（自 item 起）自底向上重算组节点状态（叶子不动）
        void refreshGroupCheckStates(QTreeWidgetItem* item)
        {
            for (int i = 0; i < item->childCount(); ++i)
                refreshGroupCheckStates(item->child(i));
            recomputeOwnCheckState(item);
        }

        //反选：只翻叶子（作用对象树=图表项，选项树=属性叶子），组节点随后重算
        void invertLeafChecks(QTreeWidgetItem* item)
        {
            if (item->childCount() == 0)
                item->setCheckState(0, item->checkState(0) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
            else
                for (int i = 0; i < item->childCount(); ++i)
                    invertLeafChecks(item->child(i));
        }

        //递归收集勾选的图表项（UserRole+1 存节点类型，图表项恒为叶子）
        void collectCheckedChartItems(QTreeWidgetItem* item, QSet<int>& out)
        {
            if (item->data(0, Qt::UserRole + 1).toInt() == static_cast<int>(CurveNodeType::Chart)
                && item->checkState(0) == Qt::Checked)
                out.insert(item->data(0, Qt::UserRole).toInt());
            for (int i = 0; i < item->childCount(); ++i)
                collectCheckedChartItems(item->child(i), out);
        }

        //恢复图表项勾选（组节点随后重算）
        void setChartCheckStates(QTreeWidgetItem* item, const QSet<int>& checked)
        {
            if (item->data(0, Qt::UserRole + 1).toInt() == static_cast<int>(CurveNodeType::Chart))
                item->setCheckState(0, checked.contains(item->data(0, Qt::UserRole).toInt())
                    ? Qt::Checked : Qt::Unchecked);
            for (int i = 0; i < item->childCount(); ++i)
                setChartCheckStates(item->child(i), checked);
        }

        //递归查找选项叶子（键唯一映射一个叶子）是否勾选
        bool findLeafChecked(QTreeWidgetItem* item, const QString& key)
        {
            if (item->childCount() == 0 && item->data(0, Qt::UserRole).toString() == key)
                return item->checkState(0) == Qt::Checked;
            for (int i = 0; i < item->childCount(); ++i)
                if (findLeafChecked(item->child(i), key))
                    return true;
            return false;
        }

        //全选/全不选/反选小图标按钮（UI设计图：Icon/Curve/操作/ 16px 线性 SVG）
        QToolButton* makeTreeToolButton(const QIcon& icon, const QString& tip)
        {
            QToolButton* btn = new QToolButton();
            btn->setIcon(icon);
            btn->setToolTip(tip);
            btn->setAutoRaise(true);
            btn->setFixedSize(24, 24);
            return btn;
        }

        //两棵复选树共用样式：面板去边框 + 行背景恒透明（无选中/悬停高亮，
        //设计稿无行选中态）+ 复选指示器对齐全局 qss 的 QCheckBox
        //图片规格（蓝底白勾；全局表只覆盖 QCheckBox::indicator，树内指示器无
        //规则会回落 Fusion 默认白框黑勾）。半选用 partcheck3x（蓝底白横线）
        const char* const kTreeQss = R"(
	        QTreeWidget{border:none;background:transparent;}
		    QTreeWidget::item{
		        background:transparent;
		        height:24px;  
		    }
			QTreeWidget::item:hover, QTreeWidget::item:selected{background:transparent;color:#333333;}
			QTreeWidget::indicator{width:14px;height:14px;margin-left:6px;margin-right:6px;}
			QTreeWidget::indicator:unchecked{image:url(:/mMainWidgets/Icon/uncheck3x.png);}
			QTreeWidget::indicator:checked{image:url(:/mMainWidgets/Icon/check3x.png);}
			QTreeWidget::indicator:indeterminate{image:url(:/mMainWidgets/Icon/partcheck3x.png);}
			QTreeWidget::indicator:unchecked:hover{image:url(:/mMainWidgets/Icon/hoverCheck.png);}
			QTreeWidget::indicator:checked:hover{image:url(:/mMainWidgets/Icon/check3x.png);}
			QTreeWidget::indicator:indeterminate:hover{image:url(:/mMainWidgets/Icon/partcheck3x.png);}
			QTreeWidget::indicator:unchecked:disabled{image:url(:/mMainWidgets/Icon/disableUnCheck.png);}
			QTreeWidget::indicator:checked:disabled{image:url(:/mMainWidgets/Icon/disableCheck.png);}
			QTreeWidget::indicator:indeterminate:disabled{image:url(:/mMainWidgets/Icon/partcheck3x.png);}
		)";

        //曲线树同款分支样式：drawBranches 画父子虚线连接 + 展开箭头（PaintDashBranches，
        //原生箭头经离屏渲染后贴近节点）——分组可收缩、层级有虚线导引，与曲线结构树一致
        class DashBranchTreeWidget : public QTreeWidget
        {
        public:
            using QTreeWidget::QTreeWidget;
            //缩进与 PaintDashBranches 箭头口径配套（箭头对齐父行符号列且贴近自身内容）
            explicit DashBranchTreeWidget(QWidget* parent = nullptr) : QTreeWidget(parent) { setIndentation(14); }
        protected:
            void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override
            {
                MechMainWindow::PaintDashBranches(this, painter, rect, index,
                    [this](QPainter* p, const QRect& r, const QModelIndex& i) { QTreeWidget::drawBranches(p, r, i); });
            }
        };
    }

    mApplyStyleDlg::mApplyStyleDlg(QWidget* parent)
        : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        buildUi();
        resize(720, 668);   //UI设计图09：中等体量双栏对话框
        setMinimumSize(560, 380);
        DlgGeom::bind(this, "ApplyStyleDlg", DlgGeom::Mode::Geometry, QSize(560, 380));

        //本次运行无已存几何时打开居中于父窗口（有几何则 bind 已恢复位置，不再
        //居中；几何只记当次运行，重启软件后回到默认尺寸+首开居中）
        _needCenter = !DlgGeom::hasSessionGeometry("ApplyStyleDlg");

        //对话框存活期间文档结构变化：删的是源图表（或其祖先页）-> 关闭；
        //其他删除 -> 重建左栏树剔除失效行（勾选集保留仍存在部分）
        mCurveDocController* controller = mCurveDocController::getInstance();
        connect(controller, &mCurveDocController::sig_nodeAboutToBeRemoved,
            this, &mApplyStyleDlg::onNodeAboutToBeRemoved);
        connect(controller, &mCurveDocController::sig_nodeRemoved,
            this, &mApplyStyleDlg::onNodeRemoved);
    }

    mApplyStyleDlg::~mApplyStyleDlg()
    {
        if (_instance == this)
            _instance = nullptr;
    }

    void mApplyStyleDlg::showForChart(int sourceChartId, QWidget* parent)
    {
        //父窗口须为顶层主窗口：QMdiSubWindow 也带 Window 标志，直接作父会把
        //对话框卷进 MDI 管理——沿 parentWidget 链走到真正的顶层
        QWidget* host = parent;
        while (host && host->parentWidget())
            host = host->parentWidget();
        if (!host)
            host = parent;

        if (!_instance)
            _instance = new mApplyStyleDlg(host);
        _instance->_sourceChartId = sourceChartId;
        _instance->rebuildTargetTree();

        if (_instance->_needCenter)
        {
            _instance->_needCenter = false;
            if (QWidget* pw = _instance->parentWidget())
                _instance->move(pw->frameGeometry().center()
                    - QPoint(_instance->width() / 2, _instance->height() / 2));
        }
        _instance->show();
        _instance->raise();
        _instance->activateWindow();
    }

    void mApplyStyleDlg::buildUi()
    {
        setAutoFillBackground(true);
        //窗口细边框：白底标题栏与内容区同为浅色，加描边保证弹窗轮廓可辨识。
        //须用 objectName 选择器：本类在 PostCurve 命名空间内，QSS 裸类名选择器
        //按 metaObject className（PostCurve::mApplyStyleDlg）匹配不上，规则整体
        //失效，未被子控件覆盖的区域（底部 QSizeGrip 行、外边距）露出调色板灰底
        setObjectName("applyStyleDlg");
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget#applyStyleDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");

        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮（照 mPropertyEditorPopup）
        _titleBar = new QWidget(this);
        _titleBar->setObjectName("applyStyleTitleBar");
        _titleBar->setFixedHeight(52);
        _titleBar->setStyleSheet(R"(
            QWidget#applyStyleTitleBar{
                background:#FFFFFF;
            }
        )");
        QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(3);
        QLabel* logoLabel = new QLabel(_titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("应用样式"), _titleBar);
        titleLabel->setStyleSheet(
            "QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");
        QToolButton* closeBtn = new QToolButton(_titleBar);
        closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
        closeBtn->setIconSize(QSize(24, 24));
        closeBtn->setToolTip(QStringLiteral("关闭"));
        closeBtn->setStyleSheet(R"(
            QToolButton{
                background:transparent; border:none;
            }
            QToolButton:hover{ background:#EFF1F2; border-radius:2px; }
        )");
        //close() 而非 hide()：对齐 mPropertyEditorPopup 的关闭语义（进程退出联动）
        connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn);

        //主体：左右两栏 + 底部按钮行
        QWidget* body = new QWidget(this);
        body->setStyleSheet("QWidget{background:#FFFFFF;}");
        QVBoxLayout* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(20, 20, 20, 20);
        bodyLayout->setSpacing(30);

        //两栏按设计稿宽度比例分配，间隔20px
        QHBoxLayout* columns = new QHBoxLayout();
        columns->setSpacing(20);
        columns->addLayout(buildTargetPanel(), 8);
        columns->addLayout(buildOptionPanel(), 5);
        bodyLayout->addLayout(columns, 1);

        //底部按钮行（应用/取消/确定，确定为蓝底主按钮）
        QHBoxLayout* buttonRow = new QHBoxLayout();
        buttonRow->setSpacing(12);
        buttonRow->setContentsMargins(0, 0, 0, 0);
        _applyBtn = new QPushButton(QStringLiteral("应用"), body);
        _cancelBtn = new QPushButton(QStringLiteral("取消"), body);
        _okBtn = new QPushButton(QStringLiteral("确定"), body);
        _applyBtn->setFixedSize(54, 26);
        _cancelBtn->setFixedSize(54, 26);
        _okBtn->setFixedSize(54, 26);
        const QString normalBtnStyle = R"(
            QPushButton{
                background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
                color:#333333; padding:2px 14px;
            }
            QPushButton:hover{ background:#EFF1F2; }
        )";
        _applyBtn->setStyleSheet(normalBtnStyle);
        _cancelBtn->setStyleSheet(normalBtnStyle);
        _okBtn->setStyleSheet(R"(
            QPushButton{
                background:#0066CC; border:1px solid #0066CC; border-radius:4px;
                color:#FFFFFF; padding:2px 14px;
            }
            QPushButton:hover{ background:#0073E0; }
        )");
        connect(_applyBtn, &QPushButton::clicked, this, &mApplyStyleDlg::applyStyles);
        connect(_cancelBtn, &QPushButton::clicked, this, &QWidget::close);
        connect(_okBtn, &QPushButton::clicked, this, [this]()
            {
                if (applyStyles())
                    close();
            });
        buttonRow->addStretch(1);
        buttonRow->addWidget(_applyBtn);
        buttonRow->addWidget(_cancelBtn);
        buttonRow->addWidget(_okBtn);
        bodyLayout->addLayout(buttonRow);

        outer->addWidget(_titleBar);
        outer->addWidget(body, 1);

        //右下角缩放手柄：不占布局行——单独成行会把按钮行抬高，改为覆盖子件贴角，resizeEvent 跟随
        _grip = new QSizeGrip(this);
        _grip->setFixedSize(_grip->sizeHint());
    }

    QVBoxLayout* mApplyStyleDlg::buildTargetPanel()
    {
        QVBoxLayout* panel = new QVBoxLayout();
        panel->setSpacing(12);

        QLabel* caption = new QLabel(QStringLiteral("作用对象筛选："), this);
        caption->setStyleSheet(
            "QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");

        QFrame* card = new QFrame(this);
        card->setObjectName("applyStyleTargetCard");
        card->setStyleSheet(
            "QFrame#applyStyleTargetCard{background:#FFFFFF;border:1px solid #D9DCE0;border-radius:6px;}");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        cardLayout->setSpacing(6);

        //行1：仅可见曲线（曲线样式按序配对时，源/目标两侧都滤掉隐藏曲线）
        _visibleOnlyCheck = new QCheckBox(QStringLiteral("仅可见曲线"), card);
        _visibleOnlyCheck->setChecked(true);
        _visibleOnlyCheck->setToolTip(
            QStringLiteral("应用曲线样式时仅作用于可见曲线（源与目标两侧均过滤）"));

        //行2：作用范围 + 全选/全不选/反选（作用于作用对象树）
        _scopeCombo = new QComboBox(card);
        _scopeCombo->addItems({
            QStringLiteral("全部页面"),
            QStringLiteral("当前页面"),
            QStringLiteral("最后一次选择"),
            });
        _scopeCombo->setToolTip(QStringLiteral("作用对象的页面范围"));
        _scopeCombo->setMinimumWidth(150);
        connect(_scopeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mApplyStyleDlg::onScopeChanged);

        QToolButton* selectAllBtn = makeTreeToolButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/全选.svg")), QStringLiteral("全选"));
        QToolButton* selectNoneBtn = makeTreeToolButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/全不选.svg")), QStringLiteral("全不选"));
        QToolButton* invertBtn = makeTreeToolButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/反选.svg")), QStringLiteral("反选"));
        connect(selectAllBtn, &QToolButton::clicked, this, [this]() { setTreeChecked(_targetTree, Qt::Checked); });
        connect(selectNoneBtn, &QToolButton::clicked, this, [this]() { setTreeChecked(_targetTree, Qt::Unchecked); });
        connect(invertBtn, &QToolButton::clicked, this, [this]() { invertTreeChecked(_targetTree); });

        QHBoxLayout* scopeRow = new QHBoxLayout();
        scopeRow->setSpacing(8);
        scopeRow->addWidget(_scopeCombo);
        scopeRow->addWidget(selectAllBtn);
        scopeRow->addWidget(selectNoneBtn);
        scopeRow->addWidget(invertBtn);
        scopeRow->addStretch(1);

        //树：工程->页->图表（两列：名称/类型），组勾选向下级联；
        //分支绘制用曲线树同款（虚线导引+可收缩）
        _targetTree = new DashBranchTreeWidget(card);
        _targetTree->setColumnCount(2);
        _targetTree->setHeaderHidden(true);
        _targetTree->setSelectionMode(QAbstractItemView::NoSelection);
        _targetTree->setFocusPolicy(Qt::NoFocus);
        _targetTree->setUniformRowHeights(true);
        _targetTree->setColumnWidth(0, 220);
        _targetTree->setColumnWidth(1, 64);
        _targetTree->setStyleSheet(kTreeQss);
        connect(_targetTree, &QTreeWidget::itemChanged, this, &mApplyStyleDlg::onTargetItemChanged);

        //设计稿09：勾选框/范围行在边框卡片外，卡片只包住下方的作用对象树；
        //勾选框与范围行之间上下间隔3.5px（QLayout 间距为整型，取4）
        QVBoxLayout* filterRows = new QVBoxLayout();
        filterRows->setContentsMargins(0, 0, 0, 0);
        filterRows->setSpacing(12);
        filterRows->addWidget(_visibleOnlyCheck);
        filterRows->addLayout(scopeRow);

        panel->addWidget(caption);
        panel->addLayout(filterRows);
        panel->addWidget(card, 1);

        cardLayout->addWidget(_targetTree, 1);
        return panel;
    }

    QVBoxLayout* mApplyStyleDlg::buildOptionPanel()
    {
        QVBoxLayout* panel = new QVBoxLayout();
        panel->setSpacing(12);

        QLabel* caption = new QLabel(QStringLiteral("选项："), this);
        caption->setStyleSheet(
            "QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");

        //标签行右侧：全选/全不选/反选（作用于选项树）
        QToolButton* selectAllBtn = makeTreeToolButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/全选.svg")), QStringLiteral("全选"));
        QToolButton* selectNoneBtn = makeTreeToolButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/全不选.svg")), QStringLiteral("全不选"));
        QToolButton* invertBtn = makeTreeToolButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/反选.svg")), QStringLiteral("反选"));
        connect(selectAllBtn, &QToolButton::clicked, this, [this]() { setTreeChecked(_optionTree, Qt::Checked); });
        connect(selectNoneBtn, &QToolButton::clicked, this, [this]() { setTreeChecked(_optionTree, Qt::Unchecked); });
        connect(invertBtn, &QToolButton::clicked, this, [this]() { invertTreeChecked(_optionTree); });

        QHBoxLayout* captionRow = new QHBoxLayout();
        captionRow->setSpacing(8);
        captionRow->addWidget(selectAllBtn);
        captionRow->addWidget(selectNoneBtn);
        captionRow->addWidget(invertBtn);
        captionRow->addStretch(1);

        QFrame* card = new QFrame(this);
        card->setObjectName("applyStyleOptionCard");
        card->setStyleSheet(
            "QFrame#applyStyleOptionCard{background:#FFFFFF;border:1px solid #D9DCE0;border-radius:6px;}");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(4, 10, 4, 10);
        cardLayout->setSpacing(0);

        _optionTree = new DashBranchTreeWidget(card);
        _optionTree->setHeaderHidden(true);
        _optionTree->setSelectionMode(QAbstractItemView::NoSelection);
        _optionTree->setFocusPolicy(Qt::NoFocus);
        _optionTree->setUniformRowHeights(true);
        _optionTree->setIndentation(16);
        //选项树行间距 3px
        //（kTreeQss 两棵树共用，行距只加在选项树上，左侧作用对象树不变）；
        _optionTree->setStyleSheet(QString(kTreeQss)
            + QStringLiteral("QTreeWidget::item{padding-bottom:3px;}"));
        connect(_optionTree, &QTreeWidget::itemChanged, this, &mApplyStyleDlg::onOptionItemChanged);
        buildOptionTree();
        cardLayout->addWidget(_optionTree);

        panel->addWidget(caption);
        panel->addLayout(captionRow);
        panel->addWidget(card, 1);
        return panel;
    }

    void mApplyStyleDlg::buildOptionTree()
    {
        //通用建项：parent 为空=顶层组；key 非空=属性叶子（UserRole 存键）。
        //默认全勾（曲线组除外，见下）；选项树只建一次，勾选状态随实例保留
        auto makeItem = [](QTreeWidgetItem* parent, const QString& text, const char* key) -> QTreeWidgetItem*
            {
                QTreeWidgetItem* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem();
                item->setText(0, text);
                item->setFlags(checkableItemFlags());
                item->setCheckState(0, Qt::Checked);
                if (key)
                    item->setData(0, Qt::UserRole, QString::fromLatin1(key));
                return item;
            };
        auto makeGroup = [&makeItem](const QString& text) -> QTreeWidgetItem*
            {
                QTreeWidgetItem* g = makeItem(nullptr, text, nullptr);
                QFont f = g->font(0);
                f.setBold(true);
                g->setFont(0, f);
                return g;
            };

        //Plot属性（挂图表节点）
        QTreeWidgetItem* plot = makeGroup(QStringLiteral("Plot属性"));
        makeItem(plot, QStringLiteral("背景色"), kKeyPlotBg);
        makeItem(plot, QStringLiteral("网格线颜色"), kKeyPlotGrid);

        //图例（挂图表 Legend 固定子节点）
        QTreeWidgetItem* legend = makeGroup(QStringLiteral("图例"));
        makeItem(legend, QStringLiteral("字体"), kKeyLegendFont);
        makeItem(legend, QStringLiteral("边框图颜色"), kKeyLegendBorder);
        makeItem(legend, QStringLiteral("颜色"), kKeyLegendColor);
        makeItem(legend, QStringLiteral("位置"), kKeyLegendPos);
        makeItem(legend, QStringLiteral("可见性"), kKeyLegendVis);

        //页眉（数据挂图表节点；文本内容不属于样式，不复制）
        QTreeWidgetItem* header = makeGroup(QStringLiteral("页眉"));
        makeItem(header, QStringLiteral("一级字体"), kKeyHeaderFont);
        makeItem(header, QStringLiteral("颜色"), kKeyHeaderColor);
        makeItem(header, QStringLiteral("对齐方式"), kKeyHeaderAlign);
        makeItem(header, QStringLiteral("可见性"), kKeyHeaderVis);

        //页脚（与页眉对称）
        QTreeWidgetItem* footer = makeGroup(QStringLiteral("页脚"));
        makeItem(footer, QStringLiteral("一级字体"), kKeyFooterFont);
        makeItem(footer, QStringLiteral("颜色"), kKeyFooterColor);
        makeItem(footer, QStringLiteral("对齐方式"), kKeyFooterAlign);
        makeItem(footer, QStringLiteral("可见性"), kKeyFooterVis);

        //坐标轴：应用到（范围开关）+ 外观属性
        QTreeWidgetItem* axis = makeGroup(QStringLiteral("坐标轴"));
        QTreeWidgetItem* axisApply = makeItem(axis, QStringLiteral("应用到"), nullptr);
        makeItem(axisApply, QStringLiteral("水平主轴"), kKeyAxisX);
        makeItem(axisApply, QStringLiteral("主垂直轴"), kKeyAxisY);
        makeItem(axisApply, QStringLiteral("全部副轴"), kKeyAxisY2);
        QTreeWidgetItem* axisLook = makeItem(axis, QStringLiteral("外观属性"), nullptr);
        makeItem(axisLook, QStringLiteral("标签字体"), kKeyAxisTitleFont);
        makeItem(axisLook, QStringLiteral("刻度字体"), kKeyAxisLabelFont);
        makeItem(axisLook, QStringLiteral("颜色"), kKeyAxisColor);

        //标记设置（轴的刻度/范围；作用轴随坐标轴"应用到"范围，全不勾时默认
        //水平主轴+主垂直轴）
        QTreeWidgetItem* mark = makeGroup(QStringLiteral("标记设置"));
        makeItem(mark, QStringLiteral("坐标轴缩放"), kKeyMarkScale);
        makeItem(mark, QStringLiteral("最小值"), kKeyMarkMin);
        makeItem(mark, QStringLiteral("最大值"), kKeyMarkMax);
        makeItem(mark, QStringLiteral("增量"), kKeyMarkStep);
        makeItem(mark, QStringLiteral("网格数量"), kKeyMarkGrid);
        makeItem(mark, QStringLiteral("自动适配"), kKeyMarkAuto);

        //曲线（按序配对复制到目标图表曲线）
        QTreeWidgetItem* curve = makeGroup(QStringLiteral("曲线"));
        makeItem(curve, QStringLiteral("线型"), kKeyCurveLineStyle);
        makeItem(curve, QStringLiteral("线条颜色"), kKeyCurveColor);
        makeItem(curve, QStringLiteral("线宽"), kKeyCurveWidth);
        makeItem(curve, QStringLiteral("符号样式"), kKeyCurveSymStyle);
        makeItem(curve, QStringLiteral("标记大小"), kKeyCurveSymSize);
        makeItem(curve, QStringLiteral("标记颜色"), kKeyCurveSymColor);
        makeItem(curve, QStringLiteral("区域填充"), kKeyCurveAreaFill);
        //曲线组默认不勾选（按序配对有错位风险，按需手动勾选），子项随组级联为空
        curve->setCheckState(0, Qt::Unchecked);
        cascadeCheckToChildren(curve);

        _optionTree->addTopLevelItems({ plot, legend, header, footer, axis, mark, curve });
        _optionTree->expandAll();
    }

    void mApplyStyleDlg::rebuildTargetTree()
    {
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        const int scope = _scopeCombo->currentIndex();   //0全部页面 1当前页面 2最后一次选择

        //"最后一次选择"：回放上次应用时的作用对象（会话内记忆）
        if (scope == 2)
            _checkedChartIds = _lastTargets;

        //首次打开：默认全勾（源图表除外）
        if (!_targetInited)
        {
            _targetInited = true;
            _checkedChartIds.clear();
            for (int pageId : doc->childNodeIds(doc->rootNodeId()))
                for (int chartId : doc->childNodeIds(pageId))
                    if (doc->nodeType(chartId) == CurveNodeType::Chart && chartId != _sourceChartId)
                        _checkedChartIds.insert(chartId);
        }

        _targetTree->blockSignals(true);
        _targetTree->clear();

        //建行：名称+类型图标+类型列（灰字），UserRole=id / UserRole+1=类型
        auto makeNodeItem = [doc](int nodeId, CurveNodeType type) -> QTreeWidgetItem*
            {
                QTreeWidgetItem* item = new QTreeWidgetItem();
                item->setText(0, doc->nodeName(nodeId));
                QString iconRes;
                switch (type)
                {
                case CurveNodeType::Project: iconRes = ":/mMainWidgets/Icon/Curve/结构树/工程.svg"; break;
                case CurveNodeType::Page:    iconRes = ":/mMainWidgets/Icon/Curve/结构树/页面.svg"; break;
                default:                     iconRes = ":/mMainWidgets/Icon/Curve/结构树/图表.svg"; break;
                }
                item->setIcon(0, QIcon(iconRes));
                item->setText(1, curveNodeTypeDisplayName(type));
                item->setForeground(1, QBrush(QColor(150, 155, 160)));
                item->setFlags(checkableItemFlags());
                item->setCheckState(0, Qt::Unchecked);
                item->setData(0, Qt::UserRole, nodeId);
                item->setData(0, Qt::UserRole + 1, static_cast<int>(type));
                return item;
            };
        auto fillPage = [&doc, &makeNodeItem](QTreeWidgetItem* pageItem, int pageId)
            {
                for (int chartId : doc->childNodeIds(pageId))
                    if (doc->nodeType(chartId) == CurveNodeType::Chart)
                        pageItem->addChild(makeNodeItem(chartId, CurveNodeType::Chart));
                pageItem->setExpanded(true);
            };

        QList<QTreeWidgetItem*> tops;
        if (scope == 1)
        {
            //当前页面：仅源图表所在页（页为顶层，不显示工程行）
            const int pageId = doc->parentNodeId(_sourceChartId);
            if (pageId > 0 && doc->nodeType(pageId) == CurveNodeType::Page)
            {
                QTreeWidgetItem* pageItem = makeNodeItem(pageId, CurveNodeType::Page);
                fillPage(pageItem, pageId);
                tops.append(pageItem);
            }
        }
        else
        {
            //全部页面/最后一次选择：工程 -> 页 -> 图表
            QTreeWidgetItem* rootItem = makeNodeItem(doc->rootNodeId(), CurveNodeType::Project);
            for (int pageId : doc->childNodeIds(doc->rootNodeId()))
            {
                if (doc->nodeType(pageId) != CurveNodeType::Page)
                    continue;
                QTreeWidgetItem* pageItem = makeNodeItem(pageId, CurveNodeType::Page);
                fillPage(pageItem, pageId);
                rootItem->addChild(pageItem);
            }
            rootItem->setExpanded(true);
            tops.append(rootItem);
        }
        _targetTree->addTopLevelItems(tops);

        //恢复勾选（仅树中存在的图表；已删节点自然剔除），组节点自底向上回算
        for (QTreeWidgetItem* top : tops)
        {
            setChartCheckStates(top, _checkedChartIds);
            refreshGroupCheckStates(top);
        }
        _targetTree->blockSignals(false);
    }

    QSet<int> mApplyStyleDlg::collectCheckedCharts() const
    {
        QSet<int> result;
        for (int i = 0; i < _targetTree->topLevelItemCount(); ++i)
            collectCheckedChartItems(_targetTree->topLevelItem(i), result);
        return result;
    }

    void mApplyStyleDlg::onTargetItemChanged(QTreeWidgetItem* item, int column)
    {
        if (column != 0)
            return;
        _targetTree->blockSignals(true);
        cascadeCheckToChildren(item);   //仅向下级联，父节点不回算
        _targetTree->blockSignals(false);
        _checkedChartIds = collectCheckedCharts();
    }

    void mApplyStyleDlg::onOptionItemChanged(QTreeWidgetItem* item, int column)
    {
        if (column != 0)
            return;
        _optionTree->blockSignals(true);
        cascadeCheckToChildren(item);   //仅向下级联，父节点不回算
        _optionTree->blockSignals(false);
    }

    void mApplyStyleDlg::setTreeChecked(QTreeWidget* tree, Qt::CheckState state)
    {
        tree->blockSignals(true);
        for (int i = 0; i < tree->topLevelItemCount(); ++i)
        {
            tree->topLevelItem(i)->setCheckState(0, state);
            cascadeCheckToChildren(tree->topLevelItem(i));
        }
        tree->blockSignals(false);
        if (tree == _targetTree)
            _checkedChartIds = collectCheckedCharts();
    }

    void mApplyStyleDlg::invertTreeChecked(QTreeWidget* tree)
    {
        tree->blockSignals(true);
        for (int i = 0; i < tree->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* top = tree->topLevelItem(i);
            invertLeafChecks(top);
            refreshGroupCheckStates(top);
        }
        tree->blockSignals(false);
        if (tree == _targetTree)
            _checkedChartIds = collectCheckedCharts();
    }

    void mApplyStyleDlg::onScopeChanged(int index)
    {
        Q_UNUSED(index);
        rebuildTargetTree();   //范围语义（含"最后一次选择"回放）在重建内统一处理
    }

    void mApplyStyleDlg::onNodeAboutToBeRemoved(int nodeId)
    {
        if (_sourceChartId <= 0)
            return;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        //删的是源图表自身或其祖先（如所在页）-> 关闭（此时节点仍可查父子链）
        if (nodeId == _sourceChartId)
        {
            close();
            return;
        }
        for (int p = doc->parentNodeId(_sourceChartId); p > 0; p = doc->parentNodeId(p))
        {
            if (p == nodeId)
            {
                close();
                return;
            }
        }
    }

    void mApplyStyleDlg::onNodeRemoved(int nodeId)
    {
        _checkedChartIds.remove(nodeId);
        _lastTargets.remove(nodeId);
        if (isVisible())
            rebuildTargetTree();
    }

    bool mApplyStyleDlg::optionChecked(const char* key) const
    {
        const QString k = QString::fromLatin1(key);
        for (int i = 0; i < _optionTree->topLevelItemCount(); ++i)
            if (findLeafChecked(_optionTree->topLevelItem(i), k))
                return true;
        return false;
    }

    bool mApplyStyleDlg::applyStyles()
    {
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller->document();

        QSet<int> targets = collectCheckedCharts();
        targets.remove(_sourceChartId);   //源自身强制跳过（样式来源）

        //剔除对话框存活期间已失效的节点（勾选集允许为超集）
        QSet<int> valid;
        for (int chartId : targets)
            if (doc->node(chartId) && doc->nodeType(chartId) == CurveNodeType::Chart)
                valid.insert(chartId);
        if (valid.isEmpty())
        {
            controller->reportWarning(QStringLiteral("[应用样式] 未勾选任何作用对象"));
            return false;
        }

        _lastTargets = valid;   //记住本次作用对象（"最后一次选择"回放）

        controller->undoStack()->beginMacro(QStringLiteral("应用样式"));
        for (int chartId : valid)
        {
            copyChartProps(_sourceChartId, chartId);
            copyLegendProps(_sourceChartId, chartId);
            copyTextProps(_sourceChartId, chartId, true);
            copyTextProps(_sourceChartId, chartId, false);
            copyAxisProps(_sourceChartId, chartId);
            copyCurveProps(_sourceChartId, chartId);
        }
        controller->undoStack()->endMacro();

        controller->reportInfo(QStringLiteral("[应用样式] 已应用到 %1 个图表").arg(valid.size()));
        return true;
    }

    void mApplyStyleDlg::copyProp(int fromId, int toId, const char* prop)
    {
        mCurveDocController* controller = mCurveDocController::getInstance();
        QVariant value = controller->document()->nodeProperty(fromId, prop);
        if (value.isValid())
            controller->setNodeProperty(toId, prop, value);   //值相同自动跳过命令
    }

    void mApplyStyleDlg::copyChartProps(int fromChart, int toChart)
    {
        if (optionChecked(kKeyPlotBg))
            copyProp(fromChart, toChart, CurveProp::kBackgroundColor);
        if (optionChecked(kKeyPlotGrid))
            copyProp(fromChart, toChart, CurveProp::kGridColor);
    }

    void mApplyStyleDlg::copyLegendProps(int fromChart, int toChart)
    {
        const int fromLegend = childNodeOfType(fromChart, static_cast<int>(CurveNodeType::Legend));
        const int toLegend = childNodeOfType(toChart, static_cast<int>(CurveNodeType::Legend));
        if (fromLegend < 0 || toLegend < 0)
            return;   //图例为固定子节点正常都在；防御
        if (optionChecked(kKeyLegendFont))
            copyProp(fromLegend, toLegend, CurveProp::kLegendFont);
        if (optionChecked(kKeyLegendBorder))
            copyProp(fromLegend, toLegend, CurveProp::kBorderColor);
        if (optionChecked(kKeyLegendColor))
        {
            //颜色=背景填充色+曲线文本色（模式与自定义色一并同步）
            copyProp(fromLegend, toLegend, CurveProp::kBgColor);
            copyProp(fromLegend, toLegend, CurveProp::kTextColorMode);
            copyProp(fromLegend, toLegend, CurveProp::kTextColor);
        }
        if (optionChecked(kKeyLegendPos))
        {
            copyProp(fromLegend, toLegend, CurveProp::kAutoPosition);
            copyProp(fromLegend, toLegend, CurveProp::kPosition);
            copyProp(fromLegend, toLegend, CurveProp::kPosX);
            copyProp(fromLegend, toLegend, CurveProp::kPosY);
        }
        if (optionChecked(kKeyLegendVis))
            copyProp(fromLegend, toLegend, CurveProp::kVisible);
    }

    void mApplyStyleDlg::copyTextProps(int fromChart, int toChart, bool headerSide)
    {
        //页眉/页脚数据挂图表节点；文本内容(kHeaderText/kFooterText)不属于样式，不复制
        if (optionChecked(headerSide ? kKeyHeaderFont : kKeyFooterFont))
            copyProp(fromChart, toChart, headerSide ? CurveProp::kHeaderFont : CurveProp::kFooterFont);
        if (optionChecked(headerSide ? kKeyHeaderColor : kKeyFooterColor))
            copyProp(fromChart, toChart, headerSide ? CurveProp::kHeaderColor : CurveProp::kFooterColor);
        if (optionChecked(headerSide ? kKeyHeaderAlign : kKeyFooterAlign))
            copyProp(fromChart, toChart, headerSide ? CurveProp::kHeaderAlign : CurveProp::kFooterAlign);
        if (optionChecked(headerSide ? kKeyHeaderVis : kKeyFooterVis))
            copyProp(fromChart, toChart, headerSide ? CurveProp::kHeaderVisible : CurveProp::kFooterVisible);
    }

    void mApplyStyleDlg::copyAxisProps(int fromChart, int toChart)
    {
        mCurveDocController* controller = mCurveDocController::getInstance();
        //单对轴的同步（外观+标记设置），由下方各范围调用
        auto copyAxisPair = [this, controller](int fromAxis, int toAxis)
            {
                //外观属性
                if (optionChecked(kKeyAxisTitleFont))
                    copyProp(fromAxis, toAxis, CurveProp::kTitleFont);
                if (optionChecked(kKeyAxisLabelFont))
                    copyProp(fromAxis, toAxis, CurveProp::kLabelFont);
                if (optionChecked(kKeyAxisColor))
                    copyProp(fromAxis, toAxis, CurveProp::kTitleColor);
                //标记设置（刻度/范围）
                if (optionChecked(kKeyMarkScale))
                {
                    copyProp(fromAxis, toAxis, CurveProp::kScaleMode);
                    copyProp(fromAxis, toAxis, CurveProp::kWeightMode);   //计权随缩放设置一并应用
                }
                if (optionChecked(kKeyMarkMin))
                {
                    copyProp(fromAxis, toAxis, CurveProp::kMin);
                    //改范围退出自动（对齐轴属性编辑页语义）
                    controller->setNodeProperty(toAxis, CurveProp::kAutoScale, false);
                }
                if (optionChecked(kKeyMarkMax))
                {
                    copyProp(fromAxis, toAxis, CurveProp::kMax);
                    controller->setNodeProperty(toAxis, CurveProp::kAutoScale, false);
                }
                if (optionChecked(kKeyMarkStep))
                {
                    //固定步长仅在"固定步长法"下参与刻度生成（轴属性页语义：
                    //该行仅此模式显示），随增量一并同步刻度生成方式——否则
                    //目标停留在均匀分步法时，复制步长值无任何效果
                    copyProp(fromAxis, toAxis, CurveProp::kTickGenMode);
                    copyProp(fromAxis, toAxis, CurveProp::kMajorStep);
                }
                if (optionChecked(kKeyMarkGrid))
                {
                    copyProp(fromAxis, toAxis, CurveProp::kMinorCount);
                    //非 linear 目标轴"每主刻度网格数"不参与渲染，一并同步
                    //每十倍网格线数（缩放方式若同批复制，此处读到的是新值）
                    if (controller->document()->nodeProperty(toAxis, CurveProp::kScaleMode).toInt() != 0)
                        copyProp(fromAxis, toAxis, CurveProp::kDecadeMinorCount);
                }
                if (optionChecked(kKeyMarkAuto))
                    controller->setNodeProperty(toAxis, CurveProp::kAutoScale, true);
            };
        //"应用到"范围：水平主轴/主垂直轴各一枚；"全部副轴"覆盖两种副轴全部节点
        //（多坐标轴：X2..Xn/Y2..Yn 按序配对，目标不足跳过）。三个范围全不勾时
        //回退到水平主轴+主垂直轴：标记设置/外观属性没有自己的范围开关，仅勾
        //这些项（不勾"应用到"）也须生效，不能整体静默
        const bool scopeX = optionChecked(kKeyAxisX);
        const bool scopeY = optionChecked(kKeyAxisY);
        const bool scopeY2 = optionChecked(kKeyAxisY2);
        const bool fallbackPrimary = !scopeX && !scopeY && !scopeY2;
        const struct { const char* optKey; CurveNodeType type; bool fallback; } singleScopes[] = {
            { kKeyAxisX,  CurveNodeType::XAxis, fallbackPrimary },
            { kKeyAxisY,  CurveNodeType::YAxis, fallbackPrimary },
        };
        for (const auto& scope : singleScopes)
        {
            if (!optionChecked(scope.optKey) && !scope.fallback)
                continue;
            const int fromAxis = childNodeOfType(fromChart, static_cast<int>(scope.type));
            const int toAxis = childNodeOfType(toChart, static_cast<int>(scope.type));
            if (fromAxis < 0 || toAxis < 0)
                continue;   //目标无该轴则跳过
            copyAxisPair(fromAxis, toAxis);
        }
        if (scopeY2)
        {
            const CurveNodeType secondTypes[] = { CurveNodeType::XAxis2, CurveNodeType::YAxis2 };
            for (CurveNodeType type : secondTypes)
            {
                QList<int> fromAxes = childNodesOfType(fromChart, static_cast<int>(type));
                QList<int> toAxes = childNodesOfType(toChart, static_cast<int>(type));
                const int n = qMin(fromAxes.size(), toAxes.size());
                for (int i = 0; i < n; ++i)
                    copyAxisPair(fromAxes.at(i), toAxes.at(i));
            }
        }
    }

    void mApplyStyleDlg::copyCurveProps(int fromChart, int toChart)
    {
        //按序配对：目标第 i 条 <- 源第 i 条；源不足时目标剩余不动
        const QList<int> fromCurves = curveNodeIds(fromChart);
        const QList<int> toCurves = curveNodeIds(toChart);
        const int n = qMin(fromCurves.size(), toCurves.size());
        for (int i = 0; i < n; ++i)
        {
            const int fromId = fromCurves.at(i);
            const int toId = toCurves.at(i);
            if (optionChecked(kKeyCurveLineStyle))
                copyProp(fromId, toId, CurveProp::kLineStyle);
            if (optionChecked(kKeyCurveColor))
                copyProp(fromId, toId, CurveProp::kColor);
            if (optionChecked(kKeyCurveWidth))
                copyProp(fromId, toId, CurveProp::kLineWidth);
            if (optionChecked(kKeyCurveSymStyle))
                copyProp(fromId, toId, CurveProp::kSymbolStyle);
            if (optionChecked(kKeyCurveSymSize))
            {
                //标记大小改为"基准大小×缩放尺寸"推导后，按各曲线自身的基准折算
                //缩放尺寸下发（来源/目标样式不同也保证符号像素大小与来源一致）；
                //基准规则同 mCurveChartView 的 baseSymbolSize（大圆点(1)12，其余8）
                auto baseSymbolSizeOf = [](int symbolStyle) -> double
                {
                    return symbolStyle == 1 ? 12.0 : 8.0;
                };
                CurveDocument* doc = mCurveDocController::getInstance()->document();
                mCurveDocController* controller = mCurveDocController::getInstance();
                const CurveNode* from = doc->node(fromId);
                const CurveNode* to = doc->node(toId);
                if (from && to)
                {
                    const double srcSize =
                        baseSymbolSizeOf(from->series.symbolStyle) * from->series.symbolScale;
                    controller->setNodeProperty(toId, CurveProp::kSymbolScale,
                        srcSize / baseSymbolSizeOf(to->series.symbolStyle));
                }
            }
            if (optionChecked(kKeyCurveSymColor))
                copyProp(fromId, toId, CurveProp::kSymbolColor);
            if (optionChecked(kKeyCurveAreaFill))
                copyProp(fromId, toId, CurveProp::kAreaFill);
        }
    }

    int mApplyStyleDlg::childNodeOfType(int chartId, int type) const
    {
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        for (int childId : doc->childNodeIds(chartId))
            if (doc->nodeType(childId) == static_cast<CurveNodeType>(type))
                return childId;
        return -1;
    }

    QList<int> mApplyStyleDlg::childNodesOfType(int chartId, int type) const
    {
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        QList<int> ids;
        for (int childId : doc->childNodeIds(chartId))
            if (doc->nodeType(childId) == static_cast<CurveNodeType>(type))
                ids.append(childId);
        return ids;
    }

    QList<int> mApplyStyleDlg::curveNodeIds(int chartId) const
    {
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        QList<int> ids;
        for (int childId : doc->childNodeIds(chartId))
        {
            CurveNode* n = doc->node(childId);
            if (n && n->type == CurveNodeType::Curve)
            {
                //"仅可见曲线"勾选时滤掉隐藏曲线（源/目标两侧同一规则）
                if (_visibleOnlyCheck->isChecked() && !n->series.visible)
                    continue;
                ids.append(childId);
            }
        }
        return ids;
    }

    void mApplyStyleDlg::mousePressEvent(QMouseEvent* event)
    {
        //仅记录按下点：拖动延迟到 mouseMoveEvent 中移动超过系统拖动阈值后才
        //真正开始（双击微动不再挪动弹窗，见 mouseDoubleClickEvent）
        if (event->button() == Qt::LeftButton && _titleBar->underMouse())
            _pressPos = event->globalPos();
        QWidget::mousePressEvent(event);
    }

    void mApplyStyleDlg::mouseMoveEvent(QMouseEvent* event)
    {
        //只有用户拖动才改变位置：左键按住且自按下点（双击则自双击点）移动超过
        //系统拖动阈值才进入拖动并固定偏移，此后跟随手部移动
        if ((event->buttons() & Qt::LeftButton) && !_dragging
            && _titleBar->underMouse()
            && (event->globalPos() - _pressPos).manhattanLength()
                >= QApplication::startDragDistance())
        {
            _dragging = true;
            _dragOffset = event->globalPos() - frameGeometry().topLeft();
        }
        if (_dragging && (event->buttons() & Qt::LeftButton))
            move(event->globalPos() - _dragOffset);
        QWidget::mouseMoveEvent(event);
    }

    void mApplyStyleDlg::mouseReleaseEvent(QMouseEvent* event)
    {
        _dragging = false;   //松手结束拖动（双击的第二击不延续上一次拖动态）
        QWidget::mouseReleaseEvent(event);
    }

    void mApplyStyleDlg::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //双击不进入拖动链路：默认实现会转发 mousePressEvent 重新置位拖动状态，
        //配合手部微动会挪动弹窗；此处仅刷新阈值起算点，应用样式弹窗双击无其他语义
        if (event->button() == Qt::LeftButton)
            _pressPos = event->globalPos();
    }

    void mApplyStyleDlg::resizeEvent(QResizeEvent* event)
    {
        if (_grip)
            _grip->move(width() - _grip->width(), height() - _grip->height());
        QWidget::resizeEvent(event);
    }
}
