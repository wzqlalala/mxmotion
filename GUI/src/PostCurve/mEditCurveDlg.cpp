#include "mEditCurveDlg.h"
#include "CurveView/mCurveViewportFactory.h"
#include "CurveView/mCurveChartView.h"
#include "CurveCore/CurveDocument.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveWidgets/mCurveFoldSection.h"
#include "CurveWidgets/mCurveEditorWidgets.h"
#include "mDlgGeometry.h"
#include "mTreeDelegate.h"   //选取器层级树虚线导引线（与曲线树同源）
#include "mCurveTreeSelectDelegate.h"   //选取器树选中条代理（与曲线树同款浅蓝圆角条）

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPolygon>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QSizeGrip>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace PostCurve
{
    mEditCurveDlg* mEditCurveDlg::_instance = nullptr;

    //窗口默认/最小尺寸（设计图16：280×457）
    static const QSize kEditDlgDefaultSize(280, 491);
    static const QSize kEditDlgMinSize(280, 510);

    //曲线选择按钮图标：波浪曲线（选取目标曲线入口，无下拉框语义，直接用资源切图）
    static QIcon makeCurvePickIcon()
    {
        return QIcon(QStringLiteral(":/mMainWidgets/Icon/NVH/曲线.svg"));
    }

    //窄行标签：addRow 默认 90px 定宽（对齐参数栏），280 宽弹窗收窄到 40px，
    //把左右间隔让给编辑器（各行走同一窄宽，字段左缘保持对齐）
    static QLabel* makeNarrowLabel(const QString& text, QWidget* parent)
    {
        QLabel* lb = new QLabel(text, parent);
        lb->setFixedWidth(40);
        lb->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        return lb;
    }

    //选取器工具条图标（资源切图，与模板面板操作按钮同用 Curve/操作 一组）：
    //kind 0=全选 1=全取消 2=反选
    static QIcon makePickerToolIcon(int kind)
    {
        static const QStringList paths = {
            QStringLiteral(":/mMainWidgets/Icon/Curve/操作/全选.svg"),
            QStringLiteral(":/mMainWidgets/Icon/Curve/操作/全不选.svg"),
            QStringLiteral(":/mMainWidgets/Icon/Curve/操作/反选.svg") };
        return QIcon(paths.value(kind));
    }

    //选取器层级树：与曲线树同款父子虚线连接 + 展开箭头（PaintDashBranches，原生箭头离屏渲染后贴近节点）
    class PickerTreeWidget : public QTreeWidget
    {
    public:
        using QTreeWidget::QTreeWidget;
        //缩进与 PaintDashBranches 箭头口径配套（箭头对齐父行符号列且贴近自身内容）
        explicit PickerTreeWidget(QWidget* parent = nullptr) : QTreeWidget(parent) { setIndentation(14); }
    protected:
        void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override
        {
            MechMainWindow::PaintDashBranches(this, painter, rect, index,
                [this](QPainter* p, const QRect& r, const QModelIndex& i) { QTreeWidget::drawBranches(p, r, i); });
        }
    };

    mEditCurveDlg* mEditCurveDlg::getInstance()
    {
        if (!_instance)
            _instance = new mEditCurveDlg(QApplication::activeWindow());
        return _instance;
    }

    mEditCurveDlg::mEditCurveDlg(QWidget* parent)
        //编辑曲线与曲线页面并行操作：置顶避免点击页面后被原生视口覆盖
        : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        buildUi();
        resize(kEditDlgDefaultSize);
        setMinimumSize(kEditDlgMinSize);
        //只记宽高：位置每次会话落到目标视窗旁（同 mPropertyEditorPopup 策略）。
        //键名带 v3：定稿 280×457 时弃用旧键的历史几何，按设计尺寸起
        DlgGeom::bind(this, "EditCurveDlgV3", DlgGeom::Mode::SizeOnly, kEditDlgMinSize);

        //工厂会话信号 -> 显隐/回填（本窗口不直接接触视窗，经工厂解耦）
        mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
        connect(f, &mCurveViewportFactory::sig_editCurveStarted,
            this, &mEditCurveDlg::onEditStarted);
        connect(f, &mCurveViewportFactory::sig_editCurveTargetsChanged,
            this, &mEditCurveDlg::onEditTargetChanged);
        connect(f, &mCurveViewportFactory::sig_editCurveRangeChanged,
            this, &mEditCurveDlg::onEditRangeChanged);
        connect(f, &mCurveViewportFactory::sig_editCurveDirectionChanged,
            this, &mEditCurveDlg::onEditDirectionChanged);
        connect(f, &mCurveViewportFactory::sig_editCurveStopped,
            this, &mEditCurveDlg::onEditStopped);
    }

    mEditCurveDlg::~mEditCurveDlg()
    {
        if (_instance == this)
            _instance = nullptr;
    }

    void mEditCurveDlg::buildUi()
    {
        setAutoFillBackground(true);
        //窗口细边框：白底标题栏与内容区同为浅色，加描边保证弹窗轮廓可辨识。
        //须用 objectName 选择器：本类在 PostCurve 命名空间内，QSS 裸类名选择器
        //按 metaObject className（PostCurve::mEditCurveDlg）匹配不上，规则整体
        //失效，底部 QSizeGrip 行等未覆盖区域露出调色板灰底（mApplyStyleDlg 同修）
        setObjectName("editCurveDlg");
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget#editCurveDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");
        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮（设计图16）
        _titleBar = new QWidget(this);
        _titleBar->setObjectName("editCurveTitleBar");
        _titleBar->setFixedHeight(40);
        _titleBar->setStyleSheet(R"(
            QWidget#editCurveTitleBar{
                background:#FFFFFF;
            }
        )");
        QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
        titleLayout->setContentsMargins(12, 0, 8, 0);
        titleLayout->setSpacing(3);
        QLabel* logoLabel = new QLabel(_titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("编辑曲线"), _titleBar);
        titleLabel->setStyleSheet(
            "QLabel{color:#333333; font-size:15px; font-weight:600; background:transparent;}");
        QToolButton* closeBtn = new QToolButton(_titleBar);
        closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
        closeBtn->setIconSize(QSize(24, 24));
        closeBtn->setToolTip(QStringLiteral("关闭（退出编辑曲线）"));
        closeBtn->setFixedSize(32, 28);
        closeBtn->setStyleSheet(R"(
            QToolButton{
                background:transparent; border:none;
            }
            QToolButton:hover{ background:#EFF1F2; border-radius:4px; }
        )");
        //关闭即退出编辑模式（会话结束信号反过来 hide 本窗口），再走 close
        //关闭语义（lastWindowClosed，同 mPropertyEditorPopup 的说明）
        connect(closeBtn, &QToolButton::clicked, this, [this]()
            {
                mCurveViewportFactory::getInstance()->setEditCurveMode(false);
                close();
            });
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn);

        //内容区：两个折叠分组（mCurveFoldSection，样式与参数栏分组统一；
        //外边距+组间距按设计图放大，宽体弹窗留出呼吸感）
        QWidget* body = new QWidget(this);
        body->setStyleSheet("QWidget{background:white;}");
        QVBoxLayout* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(8, 6, 8, 6);
        bodyLayout->setSpacing(6);
        bodyLayout->addWidget(buildTargetSection());
        bodyLayout->addWidget(buildControlSection());

		//底部按钮行三者执行前压入当前曲线撤回历史栈并点亮撤回，撤回初始/退完后不可用（:disabled 置灰，对齐应用禁用配色）
		const QString normalBtnStyle = R"(
            QPushButton{
                background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
                color:#333333; padding:4px 10px;
            }
            QPushButton:hover{ background:#EFF1F2; }
            QPushButton:disabled{
                background:#F5F7FA; border:1px solid #E4E7ED; color:#A8ABB2;
            }
        )";
        QHBoxLayout* buttonRow = new QHBoxLayout();
        buttonRow->setContentsMargins(8, 0, 8, 0);
        buttonRow->setSpacing(8);
        QPushButton* truncateBtn = new QPushButton(QStringLiteral("裁剪"), body);
        QPushButton* deleteBtn = new QPushButton(QStringLiteral("删除"), body);
        QPushButton* hideBtn = new QPushButton(QStringLiteral("隐藏"), body);
        _undoBtn = new QPushButton(QStringLiteral("撤回"), body);
        truncateBtn->setFixedSize(53, 24);
        deleteBtn->setFixedSize(53, 24);
        hideBtn->setFixedSize(53, 24);
        _undoBtn->setFixedSize(53, 24);
        for (QPushButton* b : { truncateBtn, deleteBtn, hideBtn, _undoBtn })
        {
            b->setStyleSheet(normalBtnStyle);
        }
        truncateBtn->setToolTip(QStringLiteral("仅保留起始~终止区间内的数据点"));
        deleteBtn->setToolTip(QStringLiteral("删除起始~终止区间内的数据点"));
        hideBtn->setToolTip(QStringLiteral("隐藏起始~终止区间内的数据段（撤回可恢复）"));
        connect(truncateBtn, &QPushButton::clicked, this, &mEditCurveDlg::applyTruncate);
        connect(deleteBtn, &QPushButton::clicked, this, &mEditCurveDlg::applyDelete);
        connect(hideBtn, &QPushButton::clicked, this, &mEditCurveDlg::applyHide);
        connect(_undoBtn, &QPushButton::clicked, this, &mEditCurveDlg::applyUndo);
        updateUndoButton();
        buttonRow->addWidget(truncateBtn, 1);
        buttonRow->addWidget(deleteBtn, 1);
        buttonRow->addWidget(hideBtn, 1);
        buttonRow->addWidget(_undoBtn, 1);
        bodyLayout->addLayout(buttonRow);
        bodyLayout->addStretch(1);   //多余竖向空间留在分组与按钮行之间（按钮沉底）

        outer->addWidget(_titleBar);
        outer->addWidget(body, 1);

        //右下角缩放手柄：不占布局行（单独成行会把按钮行抬高、底部留白超出
        //设计间距，对齐 mApplyStyleDlg），改为覆盖子件贴角，resizeEvent 跟随
        _grip = new QSizeGrip(this);
        _grip->setFixedSize(_grip->sizeHint());
    }

    QWidget* mEditCurveDlg::buildTargetSection()
    {
        mCurveFoldSection* section = new mCurveFoldSection(QStringLiteral("目标曲线"), this);
        section->setHeaderHeight(28);   // 设计图16：分组头加高（参数栏保持默认25px不受影响）

        // 回显框：上方白底多行只读文本（长曲线名自动换行），
        // 下方整行灰色条带（设计图16：仅按钮所在行变灰），选择按钮停靠条带右端
        QFrame* echoBox = new QFrame(section);
        echoBox->setObjectName("curveEchoBox");
        echoBox->setStyleSheet(
            "QFrame#curveEchoBox{background:#FFFFFF; border:1px solid #DCDFE6;"
            "border-radius:4px;}");
        // 不再固定高度：总高 = 上下边框2 + 文本区56 + 条带30 = 88，交给布局计算

        QVBoxLayout* boxLayout = new QVBoxLayout(echoBox);
        boxLayout->setContentsMargins(0, 0, 0, 0);  // 灰条必须贴边铺满，文字缩进改由QTextEdit的padding承担
        boxLayout->setSpacing(0);

        _curveEdit = new QTextEdit(echoBox);
        _curveEdit->setObjectName("curveEchoEdit");
        _curveEdit->setReadOnly(true);
        _curveEdit->setFrameShape(QFrame::NoFrame);
        _curveEdit->setFocusPolicy(Qt::NoFocus);                 // 只读回显，不参与焦点链
        _curveEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _curveEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _curveEdit->setPlaceholderText(QStringLiteral("无目标曲线"));
        _curveEdit->setFixedHeight(56);
        _curveEdit->setStyleSheet(
            "QTextEdit{background:#FFFFFF; border:none; color:#333333;"
            "font-size:12px; padding:1px;}");                   // padding让文字不贴边框

        // 条带高度30 = 1(顶部分隔线border) + 2(上间距) + 24(按钮) + 3(下间距)
        QWidget* strip = new QWidget(echoBox);
        strip->setObjectName("curveEchoStrip");
        strip->setFixedHeight(24);
        strip->setStyleSheet(
            "QWidget#curveEchoStrip{background:#E8EBF2; border-top:1px solid #DCDFE6;"
            "border-bottom-left-radius:3px; border-bottom-right-radius:3px;}");

        QHBoxLayout* stripLayout = new QHBoxLayout(strip);
        stripLayout->setContentsMargins(0, 0, 0, 0);
        stripLayout->setSpacing(0);
        stripLayout->addStretch(1);

        _curvePickBtn = new QToolButton(strip);   // parent直接给strip，语义更清晰
        _curvePickBtn->setObjectName("curvePickBtn");
        _curvePickBtn->setIcon(makeCurvePickIcon());
        _curvePickBtn->setIconSize(QSize(18, 18));
        _curvePickBtn->setToolTip(QStringLiteral("选择目标曲线（也可在视窗中 Ctrl+左键点击曲线）"));
        _curvePickBtn->setFixedSize(24, 24);
        _curvePickBtn->setCursor(Qt::PointingHandCursor);
        _curvePickBtn->setFocusPolicy(Qt::NoFocus);   // 消除点击后残留的虚线焦点框
        _curvePickBtn->setStyleSheet(R"(
        QToolButton{
            background:#E8EBF2; border:0px;
        }
        QToolButton:hover{ background:#EFF1F2; }
        QToolButton:pressed{ background:#E2E5E9; }
    )");
        stripLayout->addWidget(_curvePickBtn);
        connect(_curvePickBtn, &QToolButton::clicked, this, &mEditCurveDlg::showCurvePicker);

        boxLayout->addWidget(_curveEdit);
        boxLayout->addWidget(strip);

        section->addRow(makeNarrowLabel(QStringLiteral("曲线"), section), echoBox);
        return section;
    }


    QWidget* mEditCurveDlg::buildControlSection()
    {
        mCurveFoldSection* section = new mCurveFoldSection(QStringLiteral("控制"), this);
        section->setHeaderHeight(28);

		//方向：水平=竖直裁剪线沿X截取 / 垂直=水平裁剪线沿Y截取（设计图默认水平）
		_directionCombo = new QComboBox(section);
		_directionCombo->setFixedHeight(25);
		_directionCombo->addItem(QStringLiteral("水平"));
		_directionCombo->addItem(QStringLiteral("垂直"));
		connect(_directionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index)
		{
			if (_syncing)
				return;
			mCurveViewportFactory::getInstance()->setEditDirection(index);
		});
		section->addRow(makeNarrowLabel(QStringLiteral("方向"), section), _directionCombo);

        section->addRow(makeNarrowLabel(QStringLiteral("起始"), section), buildBoundEditor(0));
        section->addRow(makeNarrowLabel(QStringLiteral("终止"), section), buildBoundEditor(1));

        //步长（步进按钮的移动量；默认1，设计图）
        _stepSpin = createCurveDoubleSpin(3, 1e-6, 1e9, 1.0, section);
        _stepSpin->setFixedHeight(25);
        _stepSpin->setValue(1.0);
        connect(_stepSpin, &QDoubleSpinBox::editingFinished, this, [this]()
            {
                if (_syncing)
                    return;
                mCurveViewportFactory::getInstance()->setEditStep(_stepSpin->value());
            });
        section->addRow(makeNarrowLabel(QStringLiteral("步长"), section), _stepSpin);

        return section;
    }

    QWidget* mEditCurveDlg::buildBoundEditor(int bound)
    {
        QWidget* w = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(w);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(12);

		//数字框
		QDoubleSpinBox*& spin = (bound == 0) ? _startSpin : _endSpin;
		spin = createCurveDoubleSpin(4, -1e9, 1e9, 1.0, w);
		spin->setFixedHeight(25);
		connect(spin, &QDoubleSpinBox::editingFinished, this, [this]()
		{
			if (_syncing)
				return;
			mCurveViewportFactory::getInstance()->setEditRange(_startSpin->value(), _endSpin->value());
		});
		lay->addWidget(spin);

        //四枚步进小按钮（等宽铺满，虚线描边对齐设计图）：
        //|< 跳到数据起点 / < 按步长左移 / > 按步长右移 / >| 跳到数据终点
        struct Nudge
        {
            const char* text;
            const char* tip;
            int dir;
            bool jump;
        };
        const Nudge nudges[] = {
            { "|<", "跳到数据起点", -1, true },
            { "<",  "按步长左移",   -1, false },
            { ">",  "按步长右移",   1,  false },
            { ">|", "跳到数据终点", 1,  true },
        };
        QHBoxLayout* btns = new QHBoxLayout();
        btns->setSpacing(4);
        for (const Nudge& nd : nudges)
        {
            QToolButton* b = new QToolButton(w);
            b->setText(QString::fromUtf8(nd.text));
            b->setToolTip(QString::fromUtf8(nd.tip));
            b->setFixedHeight(24);
            b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            b->setStyleSheet(R"(
                QToolButton{
                    background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
                    color:#333333; font-size:12px;
                }
                QToolButton:hover{ background:#EFF1F2; border-style:solid; }
                QToolButton:pressed{ background:#E5E7EB; border-style:solid; }
            )");
            connect(b, &QToolButton::clicked, this, [this, bound, nd]()
                {
                    mCurveViewportFactory::getInstance()->stepEditBound(bound, nd.dir, nd.jump);
                });
            btns->addWidget(b, 1);
        }
        lay->addLayout(btns);
        return w;
    }

    void mEditCurveDlg::refillFromSession()
    {
        mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
        refillTargetNames();
        _syncing = true;
        _directionCombo->setCurrentIndex(f->editDirection());
        //数字框范围随数据界收紧（用户键入越界值由 QDoubleSpinBox 先行钳制，
        //与工厂 setEditRange 的钳制语义一致）
        if (f->editBoundMax() > f->editBoundMin())
        {
            _startSpin->setRange(f->editBoundMin(), f->editBoundMax());
            _endSpin->setRange(f->editBoundMin(), f->editBoundMax());
        }
        _startSpin->setValue(f->editStart());
        _endSpin->setValue(f->editEnd());
        _stepSpin->setValue(f->editStep());
        _syncing = false;
        //撤回按钮按操作历史刷新（历史跨会话保留）
        updateUndoButton();
    }

	void mEditCurveDlg::onEditStarted(int chartId)
	{
		Q_UNUSED(chartId)
		//撤回历史不随会话清空（跨会话保留，重开面板可继续撤回该曲线操作）；
		//按钮态由 refillFromSession 按操作历史刷新
		refillFromSession();
		//落位：目标视窗右上角内侧，钳制到主屏可用区域内
		if (QWidget* view = mCurveViewportFactory::getInstance()->editCurveView())
		{
			QPoint pos = view->mapToGlobal(QPoint(view->width(), 0))
				+ QPoint(-width() - 12, 12);
			const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
			pos.setX(qBound(screen.left(), pos.x(), screen.right() - width()));
			pos.setY(qBound(screen.top(), pos.y(), screen.bottom() - height()));
			move(pos);
		}
		show();
		raise();
		activateWindow();
	}

    void mEditCurveDlg::refillTargetNames()
    {
        mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        QStringList names;
        for (int id : f->editCurveIds())
        {
            CurveNode* n = doc->node(id);
            if (n && n->type == CurveNodeType::Curve)
                names << n->series.name;
        }
        _syncing = true;
        _curveEdit->setPlainText(names.join(QStringLiteral("，")));
        _syncing = false;
    }

    void mEditCurveDlg::onEditTargetChanged()
    {
        //回填全部目标曲线名；范围由随后的 sig_editCurveRangeChanged
        //（refreshEditBounds）与 onEditRangeChanged 同步
        refillTargetNames();
    }

    void mEditCurveDlg::onEditRangeChanged(double start, double end)
    {
        _syncing = true;
        _startSpin->setValue(start);
        _endSpin->setValue(end);
        _syncing = false;
    }

    void mEditCurveDlg::onEditDirectionChanged(int direction)
    {
        _syncing = true;
        _directionCombo->setCurrentIndex(direction);
        _syncing = false;
    }

    void mEditCurveDlg::onEditStopped()
    {
        hide();   //撤回历史跨会话保留（应用级单例），不随会话结束清空
    }

    void mEditCurveDlg::showCurvePicker()
    {
        mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
        if (f->editChartId() < 0)
            return;
        //存活期内复用同一选取器（重复点按钮前置，不叠开）
        if (_pickerDlg)
        {
            _pickerDlg->raise();
            _pickerDlg->activateWindow();
            return;
        }
        CurveDocument* doc = mCurveDocController::getInstance()->document();

        //无框白卡片选取器（设计图：M徽标标题栏 + 工具条 + 工程->页->图表->曲线
        //层级树 + 取消/确定）；树图标与曲线树同源（绿√+类型图标）
        QWidget* dlg = new QWidget(this, Qt::Window | Qt::FramelessWindowHint);
        _pickerDlg = dlg;
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setAutoFillBackground(true);
        dlg->setObjectName("curvePickerDlg");
        dlg->setStyleSheet(
            "QWidget#curvePickerDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");
        dlg->setFixedSize(480, 468);   //含页筛选行（同曲线树窗口页下拉样式）的补偿高度

        QVBoxLayout* outer = new QVBoxLayout(dlg);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);

        //标题栏（可拖动，事件过滤在 eventFilter）
        QWidget* titleBar = new QWidget(dlg);
        _pickerTitleBar = titleBar;
        titleBar->setObjectName("pickerTitleBar");
        titleBar->setFixedHeight(52);
        titleBar->setStyleSheet(
            "QWidget#pickerTitleBar{background:#FFFFFF;border:1px solid #D9DCE0;border-bottom: none;}");
        QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(8);
        QLabel* logoLabel = new QLabel(titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("曲线选取器"), titleBar);
        titleLabel->setStyleSheet(
            "QLabel{color:#333333; font-size:14px; font-weight:600; background:transparent;}");
        QToolButton* closeBtn = new QToolButton(titleBar);
        closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
        closeBtn->setIconSize(QSize(24, 24));
        closeBtn->setToolTip(QStringLiteral("关闭"));
        closeBtn->setFixedSize(20, 20);
        closeBtn->setStyleSheet(R"(
            QToolButton{
                background:transparent; border:none;
            }
            QToolButton:hover{ background:#EFF1F2; border-radius:4px; }
        )");
        connect(closeBtn, &QToolButton::clicked, dlg, &QWidget::close);
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn);
        titleBar->installEventFilter(this);
        outer->addWidget(titleBar);

        //工具条：全选 / 全取消 / 反选（作用于可选的曲线条目）
        QWidget* toolbar = new QWidget(dlg);
        QHBoxLayout* toolLayout = new QHBoxLayout(toolbar);
        toolLayout->setContentsMargins(20, 20, 20, 12);
        toolLayout->setSpacing(6);
        struct ToolBtn
        {
            const char* tip;
            int icon;
        };
        const ToolBtn tools[] = {
            { "全选", 0 },
            { "全取消", 1 },
            { "反选", 2 },
        };
        for (const ToolBtn& tb : tools)
        {
            QToolButton* b = new QToolButton(toolbar);
            b->setIcon(makePickerToolIcon(tb.icon));
            b->setToolTip(QString::fromUtf8(tb.tip));
            b->setFixedSize(26, 22);
            b->setStyleSheet(R"(
                QToolButton{
                    background:#FFFFFF; border:1px solid #C9CDD4; border-radius:3px;
                }
                QToolButton:hover{ background:#EFF1F2; }
                QToolButton:pressed{ background:#E5E7EB; }
            )");
            if (tb.icon == 0)
            {
                //全选：全部可选条目（即曲线层；页筛选隐藏的条目跳过）
                connect(b, &QToolButton::clicked, this, [this]()
                    {
                        if (!_pickerDlg)
                            return;
                        if (QTreeWidget* t = _pickerDlg->findChild<QTreeWidget*>())
                            for (QTreeWidgetItemIterator it(t, QTreeWidgetItemIterator::Selectable);
                                *it; ++it)
                            {
                                if ((*it)->isHidden())
                                    continue;
                                (*it)->setSelected(true);
                            }
                    });
            }
            else if (tb.icon == 1)
            {
                //全取消：清空选中
                connect(b, &QToolButton::clicked, this, [this]()
                    {
                        if (!_pickerDlg)
                            return;
                        if (QTreeWidget* t = _pickerDlg->findChild<QTreeWidget*>())
                            t->clearSelection();
                    });
            }
            else
            {
                //反选：可选条目选中态取反（页筛选隐藏的条目跳过）
                connect(b, &QToolButton::clicked, this, [this]()
                    {
                        if (!_pickerDlg)
                            return;
                        if (QTreeWidget* t = _pickerDlg->findChild<QTreeWidget*>())
                            for (QTreeWidgetItemIterator it(t, QTreeWidgetItemIterator::Selectable);
                                *it; ++it)
                            {
                                if ((*it)->isHidden())
                                    continue;
                                (*it)->setSelected(!(*it)->isSelected());
                            }
                    });
            }
            toolLayout->addWidget(b);
        }
        toolLayout->addStretch(1);
        outer->addWidget(toolbar);

        //层级树：工程 -> 页 -> 图表 -> 曲线（仅曲线可选，多选；容器层只作导航）。
        //树为 PickerTreeWidget：父子虚线连接与曲线树同款
        PickerTreeWidget* tree = new PickerTreeWidget(dlg);
        tree->setHeaderHidden(true);
        tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
        tree->setIconSize(QSize(28, 16));   //与绿√合成图标的逻辑尺寸一致
        //行选中条与曲线树同款：文字区浅蓝圆角条（失焦转灰），无原生深蓝选中底
        tree->setItemDelegate(new CurveTreeItemDelegate(tree));
        //原生分支区选中底置白屏蔽（选中行左侧深蓝块，与曲线树同款处理）
        {
            QPalette pal = tree->palette();
            pal.setColor(QPalette::Highlight, Qt::white);
            tree->setPalette(pal);
        }
        tree->setStyleSheet(R"(
            QTreeWidget{
                background:#FFFFFF; border:1px solid #DCDFE6; border-radius:4px;
                font-size:12px;
            }
            QTreeWidget::item{ height:26px; }
        )");
        auto buildItem = [&](CurveNodeType type, const QString& name,
            int nodeId, bool selectable) -> QTreeWidgetItem*
            {
                auto* item = new QTreeWidgetItem(QStringList(name));
                item->setIcon(0, curveNodeStatusIcon(type));
                if (nodeId >= 0)
                    item->setData(0, Qt::UserRole, nodeId);
                if (!selectable)
                    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                return item;
            };
        CurveNode* rootNode = doc->node(doc->rootNodeId());
        QTreeWidgetItem* rootItem = buildItem(CurveNodeType::Project,
            rootNode ? rootNode->name : QStringLiteral("工程"), -1, false);
        tree->addTopLevelItem(rootItem);
        QTreeWidgetItem* targetItem = nullptr;
        for (int pageId : doc->childNodeIds(doc->rootNodeId()))
        {
            CurveNode* pn = doc->node(pageId);
            if (!pn || pn->type != CurveNodeType::Page)
                continue;
            QTreeWidgetItem* pageItem = buildItem(CurveNodeType::Page, pn->name, -1, false);
            rootItem->addChild(pageItem);
            for (int chartId : doc->childNodeIds(pageId))
            {
                CurveNode* cn = doc->node(chartId);
                if (!cn || cn->type != CurveNodeType::Chart)
                    continue;
                QTreeWidgetItem* chartItem = buildItem(CurveNodeType::Chart, cn->name, -1, false);
                pageItem->addChild(chartItem);
                for (int curveId : doc->childNodeIds(chartId))
                {
                    CurveNode* un = doc->node(curveId);
                    if (!un || un->type != CurveNodeType::Curve)
                        continue;
                    QTreeWidgetItem* curveItem = buildItem(CurveNodeType::Curve, un->name, curveId, true);
                    //本期限定本图表内换目标（工厂同样拦截跨图表）：其他视窗
                    //曲线置灰不可选，展示层级但不放行
                    if (doc->chartNodeId(curveId) != f->editChartId())
                    {
                        curveItem->setDisabled(true);
                        curveItem->setToolTip(0,
                            QStringLiteral("仅限当前编辑视窗内的曲线（跨视窗编辑待扩展）"));
                    }
                    chartItem->addChild(curveItem);
                    if (f->editCurveIds().contains(curveId))
                    {
                        curveItem->setSelected(true);
                        if (!targetItem)
                            targetItem = curveItem;
                    }
                }
            }
        }
        rootItem->setExpanded(true);
        //预选当前目标并展开其所在路径
        if (targetItem)
        {
            for (QTreeWidgetItem* a = targetItem->parent(); a; a = a->parent())
                a->setExpanded(true);
            tree->setCurrentItem(targetItem);
        }
        //多选：把当前目标集全部恢复为选中（setCurrentItem 只保证主目标）
        for (QTreeWidgetItemIterator it(tree, QTreeWidgetItemIterator::Selectable);
            *it; ++it)
            (*it)->setSelected(f->editCurveIds().contains(
                (*it)->data(0, Qt::UserRole).toInt()));

        QWidget* treeHost = new QWidget(dlg);
        QVBoxLayout* treeLayout = new QVBoxLayout(treeHost);
        treeLayout->setContentsMargins(20, 0, 20, 0);
        treeLayout->addWidget(tree);
        outer->addWidget(treeHost, 1);

        //底部：取消 / 确定（选中曲线后确定可用）
        QWidget* footer = new QWidget(dlg);
        QHBoxLayout* footLayout = new QHBoxLayout(footer);
        footLayout->setContentsMargins(20, 30, 20, 20);
        footLayout->setSpacing(8);
        const QString normalBtnStyle = R"(
            QPushButton{
                background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
                color:#333333; padding:0px;
            }
            QPushButton:hover{ background:#EFF1F2; }
            QPushButton:disabled{ background:#F5F7FA; border:1px solid #E4E7ED; color:#A8ABB2; }
        )";
        const QString okBtnStyle = R"(
            QPushButton{
                background:#1385F6; border:none; border-radius:4px;
                color:#FFFFFF; padding:0px;
            }
            QPushButton:hover{ background:#3D9EF8; }
            QPushButton:pressed{ background:#0F6ACB; }
            QPushButton:disabled{ background:#9CC3E5; color:#FFFFFF; }
        )";
		QPushButton* cancelBtn = new QPushButton(QStringLiteral("取消"), footer);
		QPushButton* okBtn = new QPushButton(QStringLiteral("确定"), footer);
		cancelBtn->setStyleSheet(normalBtnStyle);
		okBtn->setStyleSheet(okBtnStyle);
		okBtn->setFixedSize(44, 26);
		cancelBtn->setFixedSize(44, 26);
		okBtn->setEnabled(targetItem != nullptr);
		//收集当前选中的曲线 id（可选条目即曲线层；树序）
		auto selectedCurveIds = [tree]() -> QList<int>
		{
			QList<int> ids;
			QTreeWidgetItemIterator it(tree,
				QTreeWidgetItemIterator::Selectable | QTreeWidgetItemIterator::Selected);
			for (; *it; ++it)
				ids << (*it)->data(0, Qt::UserRole).toInt();
			return ids;
		};
		connect(tree, &QTreeWidget::itemSelectionChanged, dlg,
			[selectedCurveIds, okBtn]() { okBtn->setEnabled(!selectedCurveIds().isEmpty()); });
		connect(cancelBtn, &QPushButton::clicked, dlg, &QWidget::close);
		connect(okBtn, &QPushButton::clicked, dlg, [this, dlg, selectedCurveIds]()
		{
			const QList<int> ids = selectedCurveIds();
			if (ids.isEmpty())
				return;
			//整组设为编辑目标（本图表内；跨视窗编辑待扩展）
			mCurveViewportFactory::getInstance()->setEditTargetCurves(ids);
			dlg->close();
		});
		footLayout->addStretch(1);
		footLayout->addWidget(cancelBtn);
		footLayout->addWidget(okBtn);
		outer->addWidget(footer);

        //落位：选取按钮下方，钳制到屏幕可用区域
        QPoint pos = _curvePickBtn->mapToGlobal(QPoint(0, _curvePickBtn->height() + 6));
        const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
        pos.setX(qBound(screen.left(), pos.x(), screen.right() - dlg->width()));
        pos.setY(qBound(screen.top(), pos.y(), screen.bottom() - dlg->height()));
        dlg->move(pos);
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    }

	bool mEditCurveDlg::currentRawRange(double& lo, double& hi) const
	{
		mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
		if (!f->editCurveMode())
			return false;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(f->editCurveId());
		if (!n || n->type != CurveNodeType::Curve)
			return false;
		//渲染空间范围反变换回原始数据空间（刻度模式/参考值读轴节点，与视窗渲染/工厂界值同一来源；对数/dB 轴下必须逆变换）。
		//边界：标量界值无频率上下文，逆变换仅撤销刻度模式、不撤销计权
		const CurveNodeType axisType = (f->editDirection() == 0)
			? CurveNodeType::XAxis : CurveNodeType::YAxis;
		int scaleMode = 0;
		double dbRef = 1.0;
		for (int childId : doc->childNodeIds(f->editChartId()))
		{
			CurveNode* ax = doc->node(childId);
			if (ax && ax->type == axisType)
			{
				scaleMode = ax->axis.scaleMode;
				dbRef = ax->axis.dbRef;
				break;
			}
		}
		const double rawStart = mCurveChartView::inverseTransformValue(f->editStart(), scaleMode, dbRef);
		const double rawEnd = mCurveChartView::inverseTransformValue(f->editEnd(), scaleMode, dbRef);
		lo = qMin(rawStart, rawEnd);   //变换单调，防御性排序
		hi = qMax(rawStart, rawEnd);
		return true;
	}

    void mEditCurveDlg::applyTruncate()
    {
        double lo = 0.0, hi = 0.0;
        if (!currentRawRange(lo, hi))
            return;
        mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller->document();
        const int direction = f->editDirection();
        const QList<int> targets = f->editCurveIds();

        //操作前整组快照（数据体量大不走命令栈，与 pch 导入一致）；
        //全部目标都无可裁剪时弹出空组并提示未执行
        takeSnapshot(targets);
        int applied = 0;
        for (int curveId : targets)
        {
            CurveNode* n = doc->node(curveId);
            if (!n || n->type != CurveNodeType::Curve)
                continue;
            const CurveSeriesData& s = n->series;
            //保留区间内数据点（区间外落盘删除）
            const int count = qMin(s.xData.size(), s.yData.size());
            QVector<double> xs, ys;
            QVector<double> cs;   //复数下面板路同索引同步（非复数曲线保持空）
            xs.reserve(count);
            ys.reserve(count);
            for (int i = 0; i < count; ++i)
            {
                const double v = (direction == 0) ? s.xData.at(i) : s.yData.at(i);
                if (v < lo || v > hi)
                    continue;
                xs.append(s.xData.at(i));
                ys.append(s.yData.at(i));
                if (s.isComplex() && i < s.complexYData.size())
                    cs.append(s.complexYData.at(i));
            }
            //空结果不落盘；范围已覆盖全量时写回同值亦无意义，均跳过
            if (xs.isEmpty() || xs.size() == count)
                continue;
            if (!cs.isEmpty())
                controller->setComplexSeriesData(curveId, xs, ys, cs);
            else
                controller->setSeriesData(curveId, xs, ys);
            ++applied;
        }
        if (applied == 0)
        {
            _opHistory.removeLast();   //未发生任何修改：弹出刚压入的空组
            updateUndoButton();
            controller->reportWarning(QStringLiteral("编辑曲线：裁剪范围内无数据点，未执行"));
            return;
        }
        //写入后 "data" 信号触发视窗防抖重建，随后复位会话范围到新数据全量
        f->refreshEditBounds();
    }

    void mEditCurveDlg::applyDelete()
    {
        double lo = 0.0, hi = 0.0;
        if (!currentRawRange(lo, hi))
            return;
        mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller->document();
        const int direction = f->editDirection();
        const QList<int> targets = f->editCurveIds();

        //删除区间内数据点（与裁剪互逆：裁剪留内删外，删除留外删内）
        takeSnapshot(targets);
        int applied = 0;
        for (int curveId : targets)
        {
            CurveNode* n = doc->node(curveId);
            if (!n || n->type != CurveNodeType::Curve)
                continue;
            const CurveSeriesData& s = n->series;
            const int count = qMin(s.xData.size(), s.yData.size());
            QVector<double> xs, ys;
            QVector<double> cs;   //复数下面板路同索引同步（非复数曲线保持空）
            xs.reserve(count);
            ys.reserve(count);
            int removed = 0;
            for (int i = 0; i < count; ++i)
            {
                const double v = (direction == 0) ? s.xData.at(i) : s.yData.at(i);
                if (v >= lo && v <= hi)
                {
                    ++removed;
                    continue;
                }
                xs.append(s.xData.at(i));
                ys.append(s.yData.at(i));
                if (s.isComplex() && i < s.complexYData.size())
                    cs.append(s.complexYData.at(i));
            }
            //该曲线范围内无点 / 全删将清空曲线：均跳过（逐曲线独立判断）
            if (removed == 0 || xs.isEmpty())
                continue;
            if (!cs.isEmpty())
                controller->setComplexSeriesData(curveId, xs, ys, cs);
            else
                controller->setSeriesData(curveId, xs, ys);
            ++applied;
        }
        if (applied == 0)
        {
            _opHistory.removeLast();
            updateUndoButton();
            controller->reportWarning(QStringLiteral("编辑曲线：删除范围内无数据点，未执行"));
            return;
        }
        f->refreshEditBounds();   //数据界随删除收缩：复位会话范围到新数据全量
    }

    void mEditCurveDlg::applyHide()
    {
        double lo = 0.0, hi = 0.0;
        if (!currentRawRange(lo, hi))
            return;
        mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller->document();
        const int direction = f->editDirection();
        const QList<int> targets = f->editCurveIds();

        //隐藏区间三元组 [轴, lo, hi] 追加（点保留在模型，渲染为断口；
        //撤回按快照整组恢复）
        takeSnapshot(targets);
        int applied = 0;
        for (int curveId : targets)
        {
            CurveNode* n = doc->node(curveId);
            if (!n || n->type != CurveNodeType::Curve)
                continue;
            const CurveSeriesData& s = n->series;
            //范围内须至少一个尚未隐藏的数据点，否则该曲线无可隐藏
            int hideCount = 0;
            const int count = qMin(s.xData.size(), s.yData.size());
            for (int i = 0; i < count; ++i)
            {
                if (s.isPointHidden(i))
                    continue;
                const double v = (direction == 0) ? s.xData.at(i) : s.yData.at(i);
                if (v >= lo && v <= hi)
                    ++hideCount;
            }
            if (hideCount == 0)
                continue;
            QVector<double> ranges = s.hiddenRanges;
            ranges.append(direction == 0 ? 0.0 : 1.0);
            ranges.append(lo);
            ranges.append(hi);
            controller->setSeriesHiddenRanges(curveId, ranges);
            ++applied;
        }
        if (applied == 0)
        {
            _opHistory.removeLast();
            updateUndoButton();
            controller->reportWarning(QStringLiteral("编辑曲线：隐藏范围内无可隐藏数据点，未执行"));
        }
    }

    void mEditCurveDlg::applyUndo()
    {
        mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
        if (!f->editCurveMode() || _opHistory.isEmpty())
            return;
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller->document();
        //弹出最近一次操作组，整组恢复各目标曲线操作前状态（组内曲线若已
        //被删除则跳过，其余照常恢复）；可连续点击逐级退回未编辑状态
        const UndoGroup group = _opHistory.takeLast();
        bool restored = false;
        for (auto it = group.curves.constBegin(); it != group.curves.constEnd(); ++it)
        {
            CurveNode* n = doc->node(it.key());
            if (!n || n->type != CurveNodeType::Curve)
                continue;
            controller->setSeriesData(it.key(), it.value().xData, it.value().yData);
            controller->setSeriesHiddenRanges(it.key(), it.value().hiddenRanges);
            //来源五元组一并还原（"更改…"换源的撤回；裁剪/删除/隐藏快照里存的
            //即当前值，等值写回无害）。setSeriesSource 静默直写不发信号，页面
            //回显随 setSeriesData 的 "data" 信号链路一并刷新；
            //setSeriesSource 会归一化侧独立来源（Y=通道/X=横坐标），侧来源的
            //真实快照值须在其后显式写回
            controller->setSeriesSource(it.key(), it.value().sourceFile, it.value().subcase,
                it.value().schema, it.value().object, it.value().component);
            controller->setCurveSideSource(it.key(), true, it.value().xSideSubcase,
                it.value().xSideSchema, it.value().xSideObject, it.value().xSideComponent);
            controller->setCurveSideSource(it.key(), false, it.value().ySideSubcase,
                it.value().ySideSchema, it.value().ySideObject, it.value().ySideComponent);
            //复数曲线：下面板路数据与来源一并还原（快照有复数数据或节点当前为
            //复数时走复数写口；complexYData 空即退化为普通单曲线）
            if (!it.value().complexYData.isEmpty() || n->series.isComplex())
            {
                controller->setComplexSeriesData(it.key(), it.value().xData,
                    it.value().yData, it.value().complexYData);
                //b 侧四元组按快照显式写回（尾参缺省镜像 a 侧会抹掉 yp 独立块选择）
                controller->setComplexSeriesSource(it.key(), it.value().sourceFile,
                    it.value().subcase, it.value().schema, it.value().object,
                    it.value().component, it.value().complexSideComponent,
                    it.value().complexFormat,
                    it.value().complexSideSubcase, it.value().complexSideSchema,
                    it.value().complexSideObject);
            }
            restored = true;
        }
        if (restored)
            f->refreshEditBounds();   //恢复后数据界变化：复位会话范围到数据全量
        updateUndoButton();
    }

    void mEditCurveDlg::takeSnapshot(const QList<int>& curveIds)
    {
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        UndoGroup group;
        for (int curveId : curveIds)
        {
            CurveNode* n = doc->node(curveId);
            if (!n || n->type != CurveNodeType::Curve)
                continue;
            CurveState st;
            st.xData = n->series.xData;
            st.yData = n->series.yData;
            st.hiddenRanges = n->series.hiddenRanges;
            st.sourceFile = n->series.sourceFile;
            st.subcase = n->series.subcase;
            st.schema = n->series.schema;
            st.object = n->series.object;
            st.component = n->series.component;
            st.xSideSubcase = n->series.xSideSubcase;
            st.xSideSchema = n->series.xSideSchema;
            st.xSideObject = n->series.xSideObject;
            st.xSideComponent = n->series.xSideComponent;
            st.ySideSubcase = n->series.ySideSubcase;
            st.ySideSchema = n->series.ySideSchema;
            st.ySideObject = n->series.ySideObject;
            st.ySideComponent = n->series.ySideComponent;
            st.complexYData = n->series.complexYData;
            st.complexFormat = n->series.complexFormat;
            st.complexSideSubcase = n->series.complexSideSubcase;
            st.complexSideSchema = n->series.complexSideSchema;
            st.complexSideObject = n->series.complexSideObject;
            st.complexSideComponent = n->series.complexSideComponent;
            group.curves.insert(curveId, st);
        }
        if (group.curves.isEmpty())
            return;
        _opHistory.append(group);
        if (_opHistory.size() > kMaxUndoDepth)
            _opHistory.removeFirst();   //深度上限：防大体量数据多级快照内存失控
        updateUndoButton();
    }

    void mEditCurveDlg::snapshotCurve(int curveId)
    {
        takeSnapshot(QList<int>{ curveId });
    }

    void mEditCurveDlg::updateUndoButton()
    {
        const bool hasUndo = mCurveViewportFactory::getInstance()->editCurveMode()
            && !_opHistory.isEmpty();
        _undoBtn->setEnabled(hasUndo);
        _undoBtn->setToolTip(hasUndo
            ? QStringLiteral("撤回上一次裁剪/删除/隐藏/更改数据源操作（可连续点击逐级退回）")
            : QStringLiteral("暂无可撤回的操作"));
    }



    void mEditCurveDlg::mousePressEvent(QMouseEvent* event)
    {
        //仅记录按下点：拖动延迟到 mouseMoveEvent 中移动超过系统拖动阈值后才
        //真正开始（双击微动不再挪动弹窗，见 mouseDoubleClickEvent）
        if (event->button() == Qt::LeftButton && _titleBar->underMouse())
            _pressPos = event->globalPos();
        QWidget::mousePressEvent(event);
    }

    void mEditCurveDlg::mouseMoveEvent(QMouseEvent* event)
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
        {
            move(event->globalPos() - _dragOffset);
        }
        QWidget::mouseMoveEvent(event);
    }

    void mEditCurveDlg::mouseReleaseEvent(QMouseEvent* event)
    {
        _dragging = false;   //松手结束拖动（双击的第二击不延续上一次拖动态）
        QWidget::mouseReleaseEvent(event);
    }

    void mEditCurveDlg::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //双击不进入拖动链路：默认实现会转发 mousePressEvent 重新置位拖动状态，
        //配合手部微动会挪动弹窗；此处仅刷新阈值起算点，编辑曲线弹窗双击无其他语义
        if (event->button() == Qt::LeftButton)
            _pressPos = event->globalPos();
    }

    void mEditCurveDlg::resizeEvent(QResizeEvent* event)
    {
        if (_grip)
            _grip->move(width() - _grip->width(), height() - _grip->height());
        QWidget::resizeEvent(event);
    }

    bool mEditCurveDlg::eventFilter(QObject* watched, QEvent* event)
    {
        //曲线选取器标题栏拖动（独立无框窗口）：标题栏子控件忽略的鼠标事件
        //会冒泡回标题栏，统一在此过滤。与主弹窗同款阈值方案——按下仅记录
        //按下点，移动超过系统拖动阈值才进入拖动（双击/手部微动不挪窗）
        if (_pickerDlg && watched == _pickerTitleBar)
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton)
                    _pickerPressPos = me->globalPos();
            }
            else if (event->type() == QEvent::MouseMove)
            {
                auto* me = static_cast<QMouseEvent*>(event);
                if ((me->buttons() & Qt::LeftButton) && !_pickerDragging
                    && (me->globalPos() - _pickerPressPos).manhattanLength()
                        >= QApplication::startDragDistance())
                {
                    _pickerDragging = true;
                    _pickerDragOffset = me->globalPos() - _pickerDlg->frameGeometry().topLeft();
                }
                if (_pickerDragging && (me->buttons() & Qt::LeftButton))
                    _pickerDlg->move(me->globalPos() - _pickerDragOffset);
            }
            else if (event->type() == QEvent::MouseButtonRelease)
            {
                _pickerDragging = false;
            }
        }
        return QWidget::eventFilter(watched, event);
    }
}
