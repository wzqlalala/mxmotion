#include "mDefine3DCurveDlg.h"
#include "mDlgGeometry.h"
#include "m3DPlotDataParser.h"
#include "mCurveExpressionEvaluator.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"
#include "CurveView/mCurveViewportFactory.h"

//QT
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QListWidget>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSet>
#include <QSizeGrip>
#include <QStackedWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>

//STL
#include <algorithm>

namespace PostCurve
{
    mDefine3DCurveDlg* mDefine3DCurveDlg::_instance = nullptr;

    namespace
    {
        //类型下拉序 -> plot3dType（对齐 set3DSeriesData 的序列类型：0 瀑布/1 曲线/
        //2 曲面/3 柱状；下拉默认"曲线"）
        const int kTypePlot3d[] = { 1, 2, 0, 3 };

        //数值表默认行数（照截图初始 8 行零值）
        constexpr int kValueRowSeed = 8;

        //数学来源 X 侧的参数采样点数（u ∈ [0,1] 均匀含端点）
        constexpr int kMathSampleCount = 101;

        //下拉弹出列表可见行数上限。B04 归因（测试岗实测证据）：setMaxVisibleItems
        //在本运行环境（Qt 5.13.2 msvc2017）对弹出实际高度不生效——含无 QSS 的
        //裸路径（maxVisibleItems=3 亦 27 行全展开），属性仅语义层保留（真机若
        //生效与之取窄，行为无差）；确定性手段=eventFilter 在弹出视图 Show 事件
        //时按实测行高钳 maxHeight（行数≤上限仅复位不钳，短列表自然全展）
        constexpr int kComboMaxVisibleItems = 10;

        //右栏最小宽度防线：低于该宽右栏页面会被裁到只剩"文件/类型/请求/分量"
        //光杆标签（输入控件不可见）。文件页行内容=标签 40 + 间距 12 + 浏览按钮/
        //下拉 + 间距 12 + 路径框最小可用宽，实算下限取整保证最小宽下文件页
        //全部控件可见可用
        constexpr int kRightPanelMinWidth = 360;

        //路径过长时的省略回显（中段省略保留文件名可辨识，tooltip 带完整路径）
        QString elidedPath(const QString& path, int maxWidth)
        {
            const QFontMetrics fm = QApplication::fontMetrics();
            return fm.elidedText(path, Qt::ElideMiddle, maxWidth);
        }

        //数学来源 X 侧的自变量采样：u ∈ [0,1] 均匀 kMathSampleCount 点（含端点）
        QVector<double> makeUSamples()
        {
            QVector<double> samples;
            samples.reserve(kMathSampleCount);
            for (int i = 0; i < kMathSampleCount; ++i)
                samples.push_back(static_cast<double>(i) / (kMathSampleCount - 1));
            return samples;
        }

        //网格类条目 Z 侧数学来源=网格装配模式判定（网格求值入口）：曲面
        //（plot3dType==2）/瀑布图（0）/柱状图（3）+ Z 侧数学来源（source==1）
        //即按 X×Y 笛卡尔积全网格装配——表达式是否真含 y 不作判定条件（常量
        //表达式得到均匀网格，自然成立）；曲线（plot3dType==1）=点列空间线走
        //逐点配对（见 assembleEntry），其余一切组合走条带装配既有口径
        bool entryInMathGridMode(const Define3DEntry& e)
        {
            return (e.plot3dType == 0 || e.plot3dType == 2 || e.plot3dType == 3)
                && e.sides[kVecZ].source == 1;
        }

        //数值网格态判定（网格类条目持网格维度且 Z 侧来源=数值）：数值表 X/Y/Z
        //三列即整网格摊平数据——应用按落定维度直读装配、表格三列全部可编辑
        //（无只读回显）。曲线类不进网格态（逐点配对口径零变化）；Z 侧切离
        //"数值"即退态（applySourceChange 清 valueGrid），本判定自带类型条件
        //兜底（条目类型创建后固定，无"切类型"入口）
        bool entryInValueGridMode(const Define3DEntry& e)
        {
            return e.valueGrid && e.sides[kVecZ].source == 2
                && (e.plot3dType == 0 || e.plot3dType == 2 || e.plot3dType == 3);
        }

        //数值表数据列下标（0=U 1=X 2=Y 3=Z）-> 向量侧下标（点列只读不挂侧；
        //U 列恒可编辑恒为数值列身份，X/Y/Z 列身份随各自 source 动态）
        constexpr int kDataColVector[4] = { kVecU, kVecX, kVecY, kVecZ };
    }

    // =============================================================================
    // 条目数据访问
    // =============================================================================

    QString Define3DEntry::valueAt(int row, int dataCol) const
    {
        if (row < 0 || row >= valueRows.size())
            return QString();
        const QStringList& r = valueRows.at(row);
        if (dataCol < 0 || dataCol >= r.size())
            return QString();
        return r.at(dataCol);
    }

    void Define3DEntry::setValueAt(int row, int dataCol, const QString& text)
    {
        if (row < 0 || row >= valueRows.size())
            return;
        QStringList& r = valueRows[row];
        if (dataCol < 0 || dataCol >= r.size())
            return;
        r[dataCol] = text;
    }

    // =============================================================================
    // 生命周期
    // =============================================================================

    mDefine3DCurveDlg::mDefine3DCurveDlg(QWidget* parent)
        : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        buildUi();
        resize(1000, 560);
        //几何记忆（大小+位置，仅当次软件运行内）：同 mCreate3DPlotDlg
        DlgGeom::bind(this, "Define3DCurveDlg");
        _needCenter = !DlgGeom::hasSessionGeometry("Define3DCurveDlg");

        //窗口细边框：白底弹窗加描边保证轮廓可辨识。须用 objectName 选择器：
        //本类在 PostCurve 命名空间内，QSS 裸类名选择器按 metaObject className
        //（PostCurve::mDefine3DCurveDlg）匹配不上（同 mCreate3DPlotDlg）
        setObjectName("define3DCurveDlg");
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget#define3DCurveDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");

        //首开空列表（条目仅经"新增"/"粘贴"创建；条目会话内留存，应用后不清空）：
        //初始无选中——对象行/右栏/应用/粘贴禁用，类型视图下拉与"新增"可用
        refreshActionStates();
    }

    mDefine3DCurveDlg::~mDefine3DCurveDlg()
    {
        if (_instance == this)
            _instance = nullptr;
    }

    void mDefine3DCurveDlg::showInstance(QWidget* parent)
    {
        //父窗口须为真正的顶层主窗口：沿父链走到顶层；顶层不是 QMainWindow 时
        //兜底取应用主窗口（同 mCreate3DPlotDlg 的取父策略）
        QWidget* host = parent;
        while (host && host->parentWidget())
            host = host->parentWidget();
        if (!qobject_cast<QMainWindow*>(host))
        {
            const QWidgetList tops = QApplication::topLevelWidgets();
            for (QWidget* w : tops)
            {
                if (qobject_cast<QMainWindow*>(w))
                {
                    host = w;
                    break;
                }
            }
        }
        if (!host)
            host = parent;

        if (!_instance)
            _instance = new mDefine3DCurveDlg(host);
        //打开即载入目标三维图内既有曲线（对齐 2D mDefineCurveDlg 编辑模式：
        //每次打开先复位，未应用的定义直接丢弃；无三维图/无曲线=初始空态）
        _instance->loadExistingCurves();
        //本次运行首开居中于父窗口（主窗口）：无边框窗口无系统定位
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

    // =============================================================================
    // UI 构建
    // =============================================================================

    void mDefine3DCurveDlg::buildUi()
    {
        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮（照 mCreate3DPlotDlg）
        _titleBar = new QWidget(this);
        _titleBar->setObjectName("define3DCurveTitleBar");
        _titleBar->setFixedHeight(52);
        _titleBar->setStyleSheet("QWidget#define3DCurveTitleBar{background:#FFFFFF;}");
        QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(3);
        QLabel* logoLabel = new QLabel(_titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("定义曲线"), _titleBar);
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
        //close() 而非 hide()：对齐既有功能弹窗的关闭语义（进程退出联动）
        connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn);

        //主体：左栏（对象）+ 矢量列 + 来源列 + 右栏来源页 + 底部按钮行（照截图三栏布局）
        QWidget* body = new QWidget(this);
        //统一控件样式（含禁用态——宿主级样式表优先级高于应用级 :disabled 规则，
        //此处必须自带禁用配色，否则初始态的置灰会退化为白底黑字，2D 定义曲线同款）
        body->setStyleSheet(R"(
            QWidget{background:#FFFFFF;color:#333333;font-size:12px;}
            QLabel{background:transparent;}
            QLineEdit,QComboBox,QListWidget,QTableWidget{
                background:#FFFFFF;border:1px solid #D9DCE0;border-radius:4px;padding:2px 0px;
                selection-background-color:#0066CC;selection-color:#FFFFFF;
            }
            QListWidget,QTableWidget{padding:1px;}
            QLineEdit:disabled,QComboBox:disabled,QListWidget:disabled,QTableWidget:disabled{
                background:#F5F7FA;color:#A8ABB2;border-color:#E4E7ED;
            }
            QComboBox QAbstractItemView{
                background:#FFFFFF;color:#333333;selection-background-color:#0066CC;
            }
            QCheckBox,QRadioButton{background:transparent;color:#333333;}
            QCheckBox:disabled,QRadioButton:disabled{color:#A8ABB2;}
            QPushButton{
                background:#FFFFFF;border:1px solid #C9CDD4;border-radius:4px;
                color:#333333;padding:3px 10px;
            }
            QPushButton:hover{background:#EFF1F2;}
            QPushButton:disabled{background:#F5F7FA;border-color:#E4E7ED;color:#A8ABB2;}
            QHeaderView::section{
                background:#F5F7FA;color:#333333;border:none;
                border-bottom:1px solid #E4E7EB;padding:3px 6px;
            }
            QTableWidget{gridline-color:#EFEFEF;selection-background-color:#E7F3FE;
                selection-color:#333333;}
            QFrame#vecSrcSeparator{background:#E4E7ED;border:none;}
        )");
        QVBoxLayout* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(20, 16, 20, 16);
        bodyLayout->setSpacing(12);

        QHBoxLayout* columns = new QHBoxLayout();
        columns->setSpacing(12);
        columns->addWidget(buildListPanel());
        columns->addWidget(makeVecSrcSeparator());
        columns->addWidget(buildVectorPanel());
        columns->addWidget(makeVecSrcSeparator());
        columns->addWidget(buildSourcePanel());
        columns->addWidget(makeVecSrcSeparator());
        _rightPanel = buildRightPages();
        columns->addWidget(_rightPanel, 1);
        bodyLayout->addLayout(columns, 1);

        //底部按钮行：关闭 / 应用（应用为蓝底主按钮，照 2D 定义曲线）
        QHBoxLayout* buttonRow = new QHBoxLayout();
        buttonRow->setContentsMargins(0, 6, 0, 0);
        buttonRow->setSpacing(12);
        _closeBtn = new QPushButton(QStringLiteral("关闭"), body);
        _applyBtn = new QPushButton(QStringLiteral("应用"), body);
        _closeBtn->setFixedSize(44, 26);
        _applyBtn->setFixedSize(44, 26);
        //应用=模块统一主蓝 #1385F6（同"从文件创建曲线"绘图钮）
        _applyBtn->setStyleSheet(R"(
            QPushButton{
                background:#1385F6; border:none; border-radius:4px;
                color:#FFFFFF; padding:0px;
            }
            QPushButton:hover{ background:#3D9EF8; }
            QPushButton:pressed{ background:#0F6ACB; }
            QPushButton:disabled{ background:#9CC3E5; color:#FFFFFF; }
        )");
        connect(_closeBtn, &QPushButton::clicked, this, &QWidget::close);
        connect(_applyBtn, &QPushButton::clicked, this, &mDefine3DCurveDlg::onApply);
        buttonRow->addStretch(1);
        buttonRow->addWidget(_closeBtn);
        buttonRow->addWidget(_applyBtn);
        bodyLayout->addLayout(buttonRow);

        outer->addWidget(_titleBar);
        outer->addWidget(body, 1);

        //右下角缩放手柄：不占布局行（覆盖子件贴角，resizeEvent 跟随，2D 同款）
        _grip = new QSizeGrip(this);
        _grip->setFixedSize(_grip->sizeHint());
    }

    QWidget* mDefine3DCurveDlg::makeVecSrcSeparator()
    {
        //列间细分隔线（照截图的竖线）
        QFrame* line = new QFrame(this);
        line->setObjectName("vecSrcSeparator");
        line->setFrameShape(QFrame::VLine);
        line->setFixedWidth(1);
        return line;
    }

    QWidget* mDefine3DCurveDlg::buildListPanel()
    {
        QWidget* panel = new QWidget(this);
        panel->setFixedWidth(280);
        QVBoxLayout* lay = new QVBoxLayout(panel);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(12);

        //对象行：复选框（勾选=应用时写入文档）+ 名称编辑框 + 类型视图下拉（照截图）
        _includeCheck = new QCheckBox(QStringLiteral("对象"), panel);
        _includeCheck->setChecked(true);
        _nameEdit = new QLineEdit(panel);
        _nameEdit->setFixedHeight(24);
        _typeCombo = new QComboBox(panel);
        _typeCombo->setFixedHeight(24);
        _typeCombo->setMaxVisibleItems(kComboMaxVisibleItems);
        bindComboPopupHeight(_typeCombo);   //弹出高度确定性钳制（B04，属性层不生效）
        //序对齐 kTypePlot3d：曲线/曲面/瀑布图/柱状图
        _typeCombo->addItems({ QStringLiteral("曲线"), QStringLiteral("曲面"),
            QStringLiteral("瀑布图"), QStringLiteral("柱状图") });
        _typeCombo->setMinimumContentsLength(3);
        _typeCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        //类型视图下拉恒可用（refreshActionStates 不参与其启停），connect 须在
        //addItems 之后——首项插入的 -1→0 信号不得早于 _curveList 创建进入过滤链
        connect(_includeCheck, &QCheckBox::toggled, this, &mDefine3DCurveDlg::onIncludeToggled);
        connect(_nameEdit, &QLineEdit::textEdited, this, &mDefine3DCurveDlg::onNameEdited);
        connect(_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mDefine3DCurveDlg::onTypeChanged);
        QHBoxLayout* nameRow = new QHBoxLayout();
        nameRow->setContentsMargins(0, 0, 0, 0);
        nameRow->setSpacing(8);
        nameRow->addWidget(_includeCheck);
        nameRow->addWidget(_nameEdit, 1);
        nameRow->addWidget(_typeCombo);
        lay->addLayout(nameRow);

        //待新增对象列表（类型过滤视图，单列名称；项 UserRole=条目下标，
        //选中条目 -> 右栏编辑其定义）
        _curveList = new QListWidget(panel);
        connect(_curveList, &QListWidget::currentRowChanged,
            this, &mDefine3DCurveDlg::onListSelectionChanged);
        lay->addWidget(_curveList, 1);

        //剪切 / 复制 / 粘贴 / 新增（照截图按钮行）
        _cutBtn = new QPushButton(QStringLiteral("剪切"), panel);
        _copyBtn = new QPushButton(QStringLiteral("复制"), panel);
        _pasteBtn = new QPushButton(QStringLiteral("粘贴"), panel);
        _addBtn = new QPushButton(QStringLiteral("新增"), panel);
        const QString btnStyle = QStringLiteral("QPushButton{padding:1px;}");
        for (QPushButton* b : { _cutBtn, _copyBtn, _pasteBtn, _addBtn })
        {
            b->setFixedHeight(24);
            b->setMinimumWidth(38);
            b->setStyleSheet(btnStyle);
        }
        connect(_cutBtn, &QPushButton::clicked, this, &mDefine3DCurveDlg::onCutClicked);
        connect(_copyBtn, &QPushButton::clicked, this, &mDefine3DCurveDlg::onCopyClicked);
        connect(_pasteBtn, &QPushButton::clicked, this, &mDefine3DCurveDlg::onPasteClicked);
        connect(_addBtn, &QPushButton::clicked, this, &mDefine3DCurveDlg::onAddClicked);
        QHBoxLayout* btnRow = new QHBoxLayout();
        btnRow->setContentsMargins(0, 0, 0, 0);
        btnRow->setSpacing(12);
        btnRow->addWidget(_cutBtn, 1);
        btnRow->addWidget(_copyBtn, 1);
        btnRow->addWidget(_pasteBtn, 1);
        btnRow->addWidget(_addBtn, 1);
        lay->addLayout(btnRow);
        return panel;
    }

    QWidget* mDefine3DCurveDlg::buildVectorPanel()
    {
        //矢量列：标题 + X/Y/Z/U/V 垂直单选（照截图；单选钮同父互斥天然成立）
        QWidget* panel = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(panel);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(14);
        QLabel* caption = new QLabel(QStringLiteral("矢量"), panel);
        caption->setStyleSheet(
            "QLabel{color:#333333;font-size:12px;font-weight:600;background:transparent;}");
        lay->addWidget(caption);
        const char* names[kVecCount] = { "X", "Y", "Z", "U", "V" };
        for (int i = 0; i < kVecCount; ++i)
        {
            _vectorRadios[i] = new QRadioButton(QLatin1String(names[i]), panel);
            //默认 X 侧：须在 connect 前置位——否则 toggled 回调过早进入
            //loadSideToPanel，右栏各页控件（_valueTable 等）尚未创建即解引用
            if (i == kVecX)
                _vectorRadios[i]->setChecked(true);
        }
        for (int i = 0; i < kVecCount; ++i)
        {
            connect(_vectorRadios[i], &QRadioButton::toggled, this,
                [this, i](bool on) { if (on) onVectorChanged(i); });
            lay->addWidget(_vectorRadios[i]);
        }
        lay->addStretch(1);
        return panel;
    }

    QWidget* mDefine3DCurveDlg::buildSourcePanel()
    {
        //来源列：标题 + 文件/数学运算/数值 垂直单选（照截图；点击=切换活动向量侧来源）
        QWidget* panel = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(panel);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(14);
        QLabel* caption = new QLabel(QStringLiteral("来源"), panel);
        caption->setStyleSheet(
            "QLabel{color:#333333;font-size:12px;font-weight:600;background:transparent;}");
        lay->addWidget(caption);
        _fileRadio = new QRadioButton(QStringLiteral("文件"), panel);
        _mathRadio = new QRadioButton(QStringLiteral("数学运算"), panel);
        _valueRadio = new QRadioButton(QStringLiteral("数值"), panel);
        _fileRadio->setChecked(true);
        connect(_fileRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onSourceChanged(0); });
        connect(_mathRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onSourceChanged(1); });
        connect(_valueRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onSourceChanged(2); });
        lay->addWidget(_fileRadio);
        lay->addWidget(_mathRadio);
        lay->addWidget(_valueRadio);
        lay->addStretch(1);
        return panel;
    }

    QWidget* mDefine3DCurveDlg::buildRightPages()
    {
        QWidget* panel = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(panel);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);
        _sourceStack = new QStackedWidget(panel);
        _sourceStack->addWidget(buildFilePage());
        _sourceStack->addWidget(buildMathPage());
        _sourceStack->addWidget(buildValuePage());
        lay->addWidget(_sourceStack, 1);
        //最小宽度防线：右栏页宽下限推高弹窗整体最小宽（布局自然推导=右栏
        //最小宽+左栏 280+矢量/来源列+边距），QSizeGrip 缩放在最小宽以上照常
        panel->setMinimumWidth(kRightPanelMinWidth);
        return panel;
    }

    QWidget* mDefine3DCurveDlg::buildFilePage()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(12);
        const QString labelStyle =
            "QLabel{color:#333333; font-size:12px; background:transparent;}";

        //文件行：浏览... + 路径回显（照截图顺序：按钮在前，输入框在后）
        QHBoxLayout* fileRow = new QHBoxLayout();
        fileRow->setSpacing(12);
        QLabel* fileLabel = new QLabel(QStringLiteral("文件"), page);
        fileLabel->setStyleSheet(labelStyle);
        fileLabel->setFixedWidth(40);
        fileRow->addWidget(fileLabel);
        _browseBtn = new QPushButton(QStringLiteral("浏览..."), page);
        _browseBtn->setAutoDefault(false);
        _browseBtn->setFixedHeight(24);
        fileRow->addWidget(_browseBtn);
        _fileEdit = new QLineEdit(page);
        _fileEdit->setReadOnly(true);
        _fileEdit->setFixedHeight(24);
        _fileEdit->setPlaceholderText(QStringLiteral("选择 CSV 或文本数据文件"));
        fileRow->addWidget(_fileEdit, 1);
        connect(_browseBtn, &QPushButton::clicked, this, &mDefine3DCurveDlg::onBrowseClicked);
        lay->addLayout(fileRow);

        //类型行/请求行共用构造：标签 + 单项下拉（txt/csv 数据口径固定；
        //pch 结果表来源后续版本接入）
        auto makeFixedComboRow = [this, &page, &labelStyle, &lay](const QString& text,
            QComboBox** combo)
            {
                QHBoxLayout* row = new QHBoxLayout();
                row->setSpacing(12);
                QLabel* label = new QLabel(text, page);
                label->setStyleSheet(labelStyle);
                label->setFixedWidth(40);
                row->addWidget(label);
                QComboBox* box = new QComboBox(page);
                box->setFixedHeight(24);
                box->setMaxVisibleItems(kComboMaxVisibleItems);   //单项下拉行为无差
                bindComboPopupHeight(box);   //弹出高度确定性钳制（B04，行为无差）
                row->addWidget(box, 1);
                lay->addLayout(row);
                *combo = box;
            };
        makeFixedComboRow(QStringLiteral("类型"), &_typeFixedCombo);
        _typeFixedCombo->addItem(QStringLiteral("数值表"));
        makeFixedComboRow(QStringLiteral("请求"), &_requestCombo);
        _requestCombo->addItem(QStringLiteral("当前数据表"));

        //分量行：按活动向量侧选择文件列（首项"未选择"；列名经
        //componentDisplayName——矩阵表恒 Column N，其余=解析缓存现行列名）
        QHBoxLayout* compRow = new QHBoxLayout();
        compRow->setSpacing(12);
        QLabel* compLabel = new QLabel(QStringLiteral("分量"), page);
        compLabel->setStyleSheet(labelStyle);
        compLabel->setFixedWidth(40);
        compRow->addWidget(compLabel);
        _componentCombo = new QComboBox(page);
        _componentCombo->setFixedHeight(24);
        //弹出高度上限：列数多的文件（矩阵表）弹出滚动展开，不一次撑满整屏
        //（B04：属性层在本运行环境不生效，确定性钳制见 bindComboPopupHeight）
        _componentCombo->setMaxVisibleItems(kComboMaxVisibleItems);
        bindComboPopupHeight(_componentCombo);
        compRow->addWidget(_componentCombo, 1);
        connect(_componentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mDefine3DCurveDlg::onComponentChanged);
        lay->addLayout(compRow);

        lay->addStretch(1);
        return page;
    }

    QWidget* mDefine3DCurveDlg::buildMathPage()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(12);

        //表达式编辑框（活动向量侧各自持有，textEdited 即时写回条目）
        _exprEdit = new QLineEdit(page);
        _exprEdit->setFixedHeight(26);
        //占位说明按来源×类型区分求值口径：X 侧 x=u 采样不变；Z 侧=本对象 Y
        //数据为 y（网格类曲面/瀑布/柱状按 X×Y 全网格求值；曲线逐点配对求值）
        _exprEdit->setPlaceholderText(QStringLiteral(
            "表达式（自变量 x；X 侧 x=参数 u∈[0,1]，Y/Z 侧 x=本对象 X 数据；"
            "Z 侧 x=对象 X 数据、y=对象 Y 数据（曲面/瀑布图/柱状图按 X×Y 全网格"
            "求值，曲线按行位逐点配对求值））"));
        connect(_exprEdit, &QLineEdit::textEdited, this, &mDefine3DCurveDlg::onExprEdited);
        lay->addWidget(_exprEdit);

        //计算器键盘（照截图 4 行 12 列；按键光标处插入，←=退格；
        //向量级函数与子对话框类按键本期未支持，置灰带 tooltip）
        QGridLayout* grid = new QGridLayout();
        grid->setSpacing(6);
        const QString disabledTip = QStringLiteral("本期未支持");
        //行0
        addKeypadButton(grid, 0, 0, 1, 1, QStringLiteral("7"), QStringLiteral("7"), true, QString());
        addKeypadButton(grid, 0, 1, 1, 1, QStringLiteral("8"), QStringLiteral("8"), true, QString());
        addKeypadButton(grid, 0, 2, 1, 1, QStringLiteral("9"), QStringLiteral("9"), true, QString());
        addKeypadButton(grid, 0, 3, 1, 1, QStringLiteral("+"), QStringLiteral("+"), true, QString());
        addKeypadButton(grid, 0, 4, 2, 1, QStringLiteral("范围"), QString(), false, disabledTip);
        addKeypadButton(grid, 0, 5, 1, 1, QStringLiteral("x"), QStringLiteral("x"), true,
            QStringLiteral("插入自变量 x"));
        addKeypadButton(grid, 0, 6, 1, 1, QStringLiteral("sin"), QStringLiteral("sin("), true, QString());
        addKeypadButton(grid, 0, 7, 1, 1, QStringLiteral("ln"), QStringLiteral("ln("), true, QString());
        addKeypadButton(grid, 0, 8, 1, 1, QStringLiteral("积分"), QString(), false, disabledTip);
        addKeypadButton(grid, 0, 9, 1, 1, QStringLiteral("freq"), QString(), false, disabledTip);
        addKeypadButton(grid, 0, 10, 1, 1, QStringLiteral("PSD"), QString(), false, disabledTip);
        addKeypadButton(grid, 0, 11, 1, 1, QStringLiteral("滤波..."), QString(), false, disabledTip);
        //行1（y 键插入自变量 y：四类型 Z 侧开放（网格类网格求值/曲线逐点配对），
        //动态启停见 refreshYKeyState——启用态随活动向量侧刷新，其余侧置灰带
        //tooltip）
        addKeypadButton(grid, 1, 0, 1, 1, QStringLiteral("4"), QStringLiteral("4"), true, QString());
        addKeypadButton(grid, 1, 1, 1, 1, QStringLiteral("5"), QStringLiteral("5"), true, QString());
        addKeypadButton(grid, 1, 2, 1, 1, QStringLiteral("6"), QStringLiteral("6"), true, QString());
        addKeypadButton(grid, 1, 3, 1, 1, QStringLiteral("-"), QStringLiteral("-"), true, QString());
        _yKeyBtn = addKeypadButton(grid, 1, 5, 1, 1, QStringLiteral("y"), QStringLiteral("y"), false,
            QStringLiteral("自变量 y 仅 Z 侧数学来源支持"));
        addKeypadButton(grid, 1, 6, 1, 1, QStringLiteral("cos"), QStringLiteral("cos("), true, QString());
        addKeypadButton(grid, 1, 7, 1, 1, QStringLiteral("exp"), QStringLiteral("exp("), true, QString());
        addKeypadButton(grid, 1, 8, 1, 1, QStringLiteral("微分"), QString(), false, disabledTip);
        addKeypadButton(grid, 1, 9, 1, 1, QStringLiteral("[FFT]"), QString(), false, disabledTip);
        addKeypadButton(grid, 1, 10, 1, 1, QStringLiteral("[RFI]"), QString(), false, disabledTip);
        addKeypadButton(grid, 1, 11, 1, 1, QStringLiteral("拟合..."), QString(), false, disabledTip);
        //行2
        addKeypadButton(grid, 2, 0, 1, 1, QStringLiteral("1"), QStringLiteral("1"), true, QString());
        addKeypadButton(grid, 2, 1, 1, 1, QStringLiteral("2"), QStringLiteral("2"), true, QString());
        addKeypadButton(grid, 2, 2, 1, 1, QStringLiteral("3"), QStringLiteral("3"), true, QString());
        addKeypadButton(grid, 2, 3, 1, 1, QStringLiteral("*"), QStringLiteral("*"), true, QString());
        addKeypadButton(grid, 2, 4, 1, 1, QStringLiteral("("), QStringLiteral("("), true, QString());
        addKeypadButton(grid, 2, 5, 1, 1, QStringLiteral(")"), QStringLiteral(")"), true, QString());
        addKeypadButton(grid, 2, 6, 1, 1, QStringLiteral("tan"), QStringLiteral("tan("), true, QString());
        addKeypadButton(grid, 2, 7, 1, 1, QStringLiteral("log"), QStringLiteral("log("), true, QString());
        addKeypadButton(grid, 2, 8, 1, 1, QStringLiteral("FFTφ"), QString(), false, disabledTip);
        addKeypadButton(grid, 2, 9, 1, 1, QStringLiteral("RFPφ"), QString(), false, disabledTip);
        addKeypadButton(grid, 2, 10, 1, 1, QStringLiteral("曲线..."), QString(), false, disabledTip);
        //行3
        addKeypadButton(grid, 3, 0, 1, 1, QStringLiteral("←"), QStringLiteral("\b"), true,
            QStringLiteral("退格"));
        addKeypadButton(grid, 3, 1, 1, 1, QStringLiteral("0"), QStringLiteral("0"), true, QString());
        addKeypadButton(grid, 3, 2, 1, 1, QStringLiteral("."), QStringLiteral("."), true, QString());
        addKeypadButton(grid, 3, 3, 1, 1, QStringLiteral("/"), QStringLiteral("/"), true, QString());
        addKeypadButton(grid, 3, 4, 1, 1, QStringLiteral("√"), QStringLiteral("sqrt("), true, QString());
        addKeypadButton(grid, 3, 5, 1, 1, QStringLiteral("^"), QStringLiteral("^"), true, QString());
        addKeypadButton(grid, 3, 6, 1, 1, QStringLiteral("π"), QStringLiteral("pi"), true,
            QStringLiteral("插入常量 pi"));
        addKeypadButton(grid, 3, 7, 1, 1, QStringLiteral("alog"), QStringLiteral("10^("), true,
            QStringLiteral("反常用对数（10^x）"));
        addKeypadButton(grid, 3, 8, 1, 1, QStringLiteral("加密..."), QString(), false, disabledTip);
        addKeypadButton(grid, 3, 9, 1, 2, QStringLiteral("希尔伯特"), QString(), false, disabledTip);
        addKeypadButton(grid, 3, 11, 1, 1, QStringLiteral("函数..."), QString(), false, disabledTip);

        //按键等宽拉伸（列宽一致，照截图等宽网格）
        for (int c = 0; c < 12; ++c)
            grid->setColumnStretch(c, 1);
        lay->addLayout(grid);
        lay->addStretch(1);
        return page;
    }

    QPushButton* mDefine3DCurveDlg::addKeypadButton(QGridLayout* grid, int row, int col,
        int spanRows, int spanCols, const QString& label, const QString& insertText,
        bool enabled, const QString& tooltip)
    {
        QPushButton* btn = new QPushButton(label, this);
        btn->setFixedHeight(26);
        btn->setMinimumWidth(44);
        btn->setStyleSheet(QStringLiteral("QPushButton{padding:1px 2px;}"));
        btn->setEnabled(enabled);
        if (!tooltip.isEmpty())
            btn->setToolTip(tooltip);
        //insertText 为空=本期未支持的占位键（不接行为）；"\b"=退格（沿用
        //QLineEdit::backspace 的选区/光标语义），其余文本经 insert 在光标处插入
        //（均发 textEdited，条目表达式即时同步）
        if (!insertText.isEmpty())
        {
            connect(btn, &QPushButton::clicked, this, [this, insertText]()
                {
                    if (insertText == QLatin1String("\b"))
                        _exprEdit->backspace();
                    else
                        _exprEdit->insert(insertText);
                });
        }
        grid->addWidget(btn, row, col, spanRows, spanCols);
        return btn;
    }

    QWidget* mDefine3DCurveDlg::buildValuePage()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(8);

        //行操作工具条（对齐二维 mDefineCurveDlg buildValuePage：[+][-] 图标钮
        //悬在表格左上 + [清空] 文字钮）
        QHBoxLayout* toolBar = new QHBoxLayout();
        toolBar->setSpacing(6);
        _vaddRowBtn = new QToolButton(page);
        _vaddRowBtn->setIcon(
            QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/添加.svg")));
        _vaddRowBtn->setFixedSize(24, 24);   //图标按钮统一规格（同二维"导入"钮）
        _vaddRowBtn->setToolTip(QStringLiteral("新增一行数值"));
        connect(_vaddRowBtn, &QToolButton::clicked,
            this, &mDefine3DCurveDlg::onValueAddClicked);
        _vdelRowBtn = new QToolButton(page);
        _vdelRowBtn->setIcon(
            QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/删除.svg")));
        _vdelRowBtn->setFixedSize(24, 24);
        _vdelRowBtn->setToolTip(QStringLiteral("删除选中行"));
        _vdelRowBtn->setEnabled(false);   //无选中行时不可删（随选择联动）
        connect(_vdelRowBtn, &QToolButton::clicked,
            this, &mDefine3DCurveDlg::onValueDeleteClicked);
        _vclearBtn = new QToolButton(page);
        //资源图标库无清空类图标，按二维同款先以文字占位（ToolButtonTextBesideIcon
        //已预留，图标后续提供后补一行 setIcon 即成"图标+文字"形态）
        _vclearBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        _vclearBtn->setText(QStringLiteral("清空"));
        _vclearBtn->setFixedHeight(24);   //高度同既有图标钮，宽度随文字自适应
        _vclearBtn->setToolTip(QStringLiteral("清空全部行"));
        _vclearBtn->setEnabled(false);   //表无行不可清（随行数联动，与选中无关）
        connect(_vclearBtn, &QToolButton::clicked,
            this, &mDefine3DCurveDlg::onValueClearClicked);
        toolBar->addWidget(_vaddRowBtn);
        toolBar->addWidget(_vdelRowBtn);
        toolBar->addWidget(_vclearBtn);
        toolBar->addStretch(1);
        lay->addLayout(toolBar);

        //数值表：点（只读行号）| U | X | Y | Z（照截图列序；列结构/宽度策略
        //既有口径零变化）
        _valueTable = new QTableWidget(page);
        _valueTable->setColumnCount(5);
        _valueTable->setHorizontalHeaderLabels({ QStringLiteral("点"), QStringLiteral("U"),
            QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z") });
        _valueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        _valueTable->setColumnWidth(0, 56);
        for (int c = 1; c < 5; ++c)
            _valueTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        _valueTable->verticalHeader()->setVisible(false);
        _valueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        _valueTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        //编辑触发对齐二维：键入即入编辑态（AnyKeyPressed）
        _valueTable->setEditTriggers(QAbstractItemView::DoubleClicked
            | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed
            | QAbstractItemView::AnyKeyPressed);
        //右键菜单：在上方/下方插入一行、剪切、复制、粘贴、删除（二维同款）
        _valueTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(_valueTable, &QWidget::customContextMenuRequested,
            this, &mDefine3DCurveDlg::onValueContextMenu);
        //选中行淡蓝（#D0E7FD，与二维数值表同配色），文字保持深色；行头选中格
        //同步淡蓝。表级 QSS 规则收口：全局 QSS 未定义表格 ::item:selected 时
        //Windows 主题按系统高亮色绘制，调色板压不过主题，须以表级样式规则
        //（优先级高于应用样式表/主题）显式指定（二维 1399-1405 同款）
        _valueTable->setStyleSheet(QStringLiteral(
            "QTableWidget::item:selected{background:#D0E7FD;color:#303133;}"
            "QHeaderView::section:vertical:selected{background:#D0E7FD;}"));
        connect(_valueTable, &QTableWidget::cellChanged,
            this, &mDefine3DCurveDlg::onValueCellChanged);
        connect(_valueTable, &QTableWidget::itemSelectionChanged, this, [this]()
        {
            refreshValueToolBarStates();   //[-]随选中行、[清空]随表行数一并刷新
        });
        lay->addWidget(_valueTable, 1);
        return page;
    }

    // =============================================================================
    // 条目与面板同步
    // =============================================================================

    QString mDefine3DCurveDlg::nextEntryName(int plot3dType) const
    {
        //类型 -> 默认名前缀（与类型视图下拉文案逐字一致；下标=plot3dType 本序
        //0 瀑布图/1 曲线/2 曲面/3 柱状图，勿与 kTypePlot3d 的下拉序映射混淆）
        static const QString kPrefix[4] = { QStringLiteral("瀑布图"),
            QStringLiteral("曲线"), QStringLiteral("曲面"), QStringLiteral("柱状图") };
        if (plot3dType < 0 || plot3dType > 3)
            return QString();
        //N=该类型计数 +1（各类型独立自增互不挤占；剪切/删除不回收编号）
        return kPrefix[plot3dType] + QStringLiteral(" %1").arg(_typeCounters[plot3dType] + 1);
    }

    int mDefine3DCurveDlg::currentEntryIndex() const
    {
        return _current;
    }

    int mDefine3DCurveDlg::viewPlot3dType() const
    {
        //类型视图下拉序 -> plot3dType（kTypePlot3d 固定映射；越界 -1=无视图）
        const int idx = _typeCombo->currentIndex();
        if (idx < 0 || idx > 3)
            return -1;
        return kTypePlot3d[idx];
    }

    int mDefine3DCurveDlg::entryAtRow(int row) const
    {
        //过滤列表行 -> 条目下标（行不在列表内/项未携带下标 -1）
        if (row < 0 || row >= _curveList->count())
            return -1;
        const QVariant v = _curveList->item(row)->data(Qt::UserRole);
        return v.isValid() ? v.toInt() : -1;
    }

    int mDefine3DCurveDlg::rowForEntry(int entryIdx) const
    {
        //条目下标 -> 过滤列表行（该条目不在当前类型视图 -1）
        for (int r = 0; r < _curveList->count(); ++r)
        {
            if (entryAtRow(r) == entryIdx)
                return r;
        }
        return -1;
    }

    void mDefine3DCurveDlg::rebuildFilteredList()
    {
        //按当前类型视图整表重建（项文本=条目名，UserRole=条目下标；全程 syncing
        //压 currentRowChanged——clear/逐项加入的中间态不得驱动选中链路）
        _syncing = true;
        _curveList->clear();
        const int view = viewPlot3dType();
        for (int i = 0; i < _entries.size(); ++i)
        {
            if (_entries.at(i).plot3dType != view)
                continue;   //其他类型条目隐藏不删除，切回完整恢复
            _curveList->addItem(_entries.at(i).name);
            _curveList->item(_curveList->count() - 1)
                ->setData(Qt::UserRole, i);
        }
        _syncing = false;
    }

    void mDefine3DCurveDlg::refreshActionStates()
    {
        //构造期护栏（同 refreshValueTable；_applyBtn 建于 buildUi 末尾）
        if (!_applyBtn)
            return;
        const bool hasSelection = _current >= 0 && _current < _entries.size();
        //对象行随选中态启停（名称再随"对象"复选框联动）；类型视图下拉恒可用
        //（驱动列表过滤与新增条目类型，不随选中态/复选框启停）
        _includeCheck->setEnabled(hasSelection);
        _nameEdit->setEnabled(hasSelection && _includeCheck->isChecked());
        _cutBtn->setEnabled(hasSelection);
        _copyBtn->setEnabled(hasSelection);
        _pasteBtn->setEnabled(_hasEntryClip);
        _rightPanel->setEnabled(hasSelection);

        //应用：存在勾选参与应用的条目才可用
        bool anyIncluded = false;
        for (const Define3DEntry& e : _entries)
        {
            if (e.include)
            {
                anyIncluded = true;
                break;
            }
        }
        _applyBtn->setEnabled(anyIncluded);
    }

    void mDefine3DCurveDlg::refreshYKeyState()
    {
        //构造期护栏：数学页键盘在 buildRightPages 中段才建（buildMathPage），
        //早到的初始化回调无键可刷
        if (!_yKeyBtn)
            return;
        //自变量 y 四类型 Z 侧均开放（活动向量侧=Z 即启用，不限条目类型——网格
        //类曲面/瀑布/柱状按 X×Y 全网格求值、曲线逐点配对求值，两类口径在
        //assembleEntry 分流；来源是否数学不作启停条件——非数学来源时数学页本就
        //不可见）。启停属 UI 态不写条目，_syncing 语义下同样刷新（loadSideToPanel
        //回填链路统一调用）
        const bool hasEntry = _current >= 0 && _current < _entries.size();
        const bool yAllowed = hasEntry && _vectorSide == kVecZ;
        _yKeyBtn->setEnabled(yAllowed);
        _yKeyBtn->setToolTip(yAllowed
            ? QStringLiteral("插入自变量 y（Z 侧=本对象 Y 数据；曲面/瀑布图/柱状图"
                "按 X×Y 网格求值，曲线按行位逐点配对求值）")
            : QStringLiteral("自变量 y 仅 Z 侧数学来源支持"));
    }

    void mDefine3DCurveDlg::ensureValueRows(Define3DEntry& e)
    {
        //首访补 8 行零值（照截图初始表格全零）；种子只补一次——[清空]后的空表
        //是合法持久态（二维同口径），切条目/切侧回显不得重新播种
        if (!e.valueRows.isEmpty() || e.valueSeeded)
            return;
        for (int i = 0; i < kValueRowSeed; ++i)
            e.valueRows.append({ QStringLiteral("0"), QStringLiteral("0"),
                QStringLiteral("0"), QStringLiteral("0") });
        e.valueSeeded = true;
    }

    void mDefine3DCurveDlg::refreshValueCellStates(int row)
    {
        //行内格状态收口（2D applySideCellStates 同构）：U 列恒可编辑；X/Y/Z
        //列仅当该侧来源=数值可编辑[网格态（entryInValueGridMode）例外：三列
        //全部可编辑数据列、跳过只读回显——表即整网格数据，所见即所得]；非
        //数值侧列只读并按行位回显该侧当前来源装配数据（'f' 6 token 缓存，
        //超出留空，装配失败/无内容=空串，2D 同口径）；非法数值格红字提示
        if (_current < 0 || _current >= _entries.size() || row < 0
            || row >= _valueTable->rowCount())
            return;
        const Define3DEntry& e = _entries.at(_current);
        const bool gridActive = entryInValueGridMode(e);
        for (int dataCol = 0; dataCol < 4; ++dataCol)
        {
            //数据列下标 -> 向量侧：0=U（不挂侧恒可编辑）1=X 2=Y 3=Z
            const int vec = kDataColVector[dataCol];
            QTableWidgetItem* it = _valueTable->item(row, dataCol + 1);
            if (!it)
                continue;
            const bool editable = dataCol == 0 || gridActive
                || e.sides[vec].source == 2;
            Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            if (editable)
                flags |= Qt::ItemIsEditable;
            //B03 主修复（缺陷单方向 1）：本仓 Qt 5.13.2 实测 setFlags 同值重复
            //调用仍发 dataChanged（setForeground 同值则正确早退），用户编辑路径
            //（无 _syncing 护栏）经 cellChanged→本函数→setFlags→dataChanged→
            //cellChanged 无限递归栈溢出。同值守卫与 setForeground 隐式早退对齐，
            //断掉递归链（状态确有变化时照常发射）
            if (it->flags() != flags)
                it->setFlags(flags);
            if (!editable)
            {
                //非数值侧列只读回显（缓存 token 第 row 位；同值不重写——
                //不依赖 setData 同值早退的 Qt 内部行为，零多余 dataChanged）
                const QString echo = _sideEchoTokens.value(vec).value(row, QString());
                if (it->text() != echo)
                    it->setText(echo);
            }
            const QString text = it->text().trimmed();
            bool ok = false;
            if (!text.isEmpty())
                text.toDouble(&ok);
            it->setForeground(ok || text.isEmpty() ? QColor(51, 51, 51)
                : QColor(0xF5, 0x6C, 0x6C));
        }
    }

    void mDefine3DCurveDlg::refreshValueTable()
    {
        //构造期护栏：buildUi 后半段才建（buildVectorPanel 等早建控件的初始化
        //回调可能先到）；未建时无事可做
        if (!_valueTable)
            return;
        //回显 token 现算：切条目/切侧/切来源后各非数值侧的来源数据可能已变
        //（2D fillValueTableSides→computeSideEchoTokens 同序）；行插删路径走
        //rebuildValueTableView 复用缓存，免重复装配
        if (_current >= 0 && _current < _entries.size())
        {
            Define3DEntry& e = _entries[_current];
            ensureValueRows(e);
            computeSideEchoTokens(e);
        }
        rebuildValueTableView();
    }

    //按既有回显 token 缓存整表重建（行插删后免重复装配）：点列=只读行号；
    //数值侧列（U 恒数值列身份、X/Y/Z 列该侧来源=数值；网格态=三列全数值列）
    //=条目已存值；非数值侧列=该侧当前来源装配数据 'f' 6 回显 token，行位对齐、
    //超出表行数留空（2D fillValueTableSides 同口径——3D 表行数恒=valueRows
    //行数不按回显数据扩行：valueRows 是四列共享行结构的唯一存储，扩显示行会
    //使所见行与条目行位错位；2D 按侧独立串存储才按各列最大值扩行）。整表重建
    //时 U 列空白归一写 "0"（写回 valueRows，下方回填自然带出——表格不再出现
    //U 空白；仅 U 补 0，X/Y/Z 空格不补：空=装配跳行，等长截齐语义依赖）
    void mDefine3DCurveDlg::rebuildValueTableView()
    {
        _syncing = true;
        _valueTable->setRowCount(0);
        if (_current >= 0 && _current < _entries.size())
        {
            Define3DEntry& e = _entries[_current];
            //网格态：X/Y/Z 三列全部按数值数据列回填（跳过只读回显 token）
            const bool gridActive = entryInValueGridMode(e);
            for (int r = 0; r < e.valueRows.size(); ++r)
            {
                if (e.valueAt(r, 0).trimmed().isEmpty())
                    e.setValueAt(r, 0, QStringLiteral("0"));   //U 空白归一 "0"
            }
            const int n = e.valueRows.size();
            _valueTable->setRowCount(n);
            for (int r = 0; r < n; ++r)
            {
                QTableWidgetItem* pt = new QTableWidgetItem(QString::number(r));
                pt->setTextAlignment(Qt::AlignCenter);
                pt->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);   //点列只读
                _valueTable->setItem(r, 0, pt);
                for (int dataCol = 0; dataCol < 4; ++dataCol)
                {
                    const int vec = kDataColVector[dataCol];
                    const bool valueCol = dataCol == 0 || gridActive
                        || e.sides[vec].source == 2;
                    QTableWidgetItem* it = new QTableWidgetItem(valueCol
                        ? e.valueAt(r, dataCol)
                        : _sideEchoTokens.value(vec).value(r, QString()));
                    it->setTextAlignment(Qt::AlignCenter);
                    _valueTable->setItem(r, dataCol + 1, it);
                }
                refreshValueCellStates(r);
            }
        }
        _syncing = false;
        //工具条启停收口（二维 fillValueTableSides 同款）：整表重建后行数变化
        //（回填不触发选择信号），[-]/[清空]重申启停
        refreshValueToolBarStates();
    }

    //非数值侧列的只读回显 token 现算缓存（'f' 6，行位对齐，2D
    //computeSideEchoTokens 同名同口径）：X/Y/Z 侧来源≠数值时装配该侧当前
    //来源数据——文件=已选分量列整列；数学=条带采样口径求值（X 侧=u∈[0,1]
    //101 点，Y 侧=以装配的 X 数据为采样，与条带应用装配同源）；Z 侧数学
    //=逐点配对二元按行求值（zMathPointwiseEval，与曲线逐点应用口径同源，
    //含 y 表达式可求值；R39 网格求值仅在应用装配路径，网格类条目此处同为
    //逐点口径不摊平网格）；装配失败/无内容=空列（回显留空），求值非有限
    //行 token 写空串（不写 "nan" 毒化文本）。数值侧列缓存空表（内容
    //=条目数值列）；U 恒数值列身份不回显、V 侧无数值列不缓存
    void mDefine3DCurveDlg::computeSideEchoTokens(const Define3DEntry& e)
    {
        _sideEchoTokens.clear();
        //网格态：X/Y/Z 三列全部为可编辑数值列（无只读回显），缓存留空即够
        //（rebuildValueTableView/refreshValueCellStates 均不再走回显分支；
        //下方装配亦不必为休眠的 X/Y 侧来源白算）
        if (entryInValueGridMode(e))
            return;
        for (int i = 0; i < kVecCount; ++i)
            _sideEchoTokens.append(QStringList());
        //Y 数学回显的采样横坐标=装配的 X 数据（X 装配不出=该侧数学回显留空，
        //2D 同口径——文件侧回显不依赖采样）；Z 侧数学不进此采样路径（分支内
        //逐点配对求值自带 X/Y 装配）
        QVector<double> xs;
        QString xsErr;
        const bool xsOk = assembleSideVector(e, kVecX, makeUSamples(), xs, nullptr, xsErr);
        for (int vec = kVecX; vec <= kVecZ; ++vec)
        {
            if (e.sides[vec].source == 2)
                continue;   //数值侧列：缓存空表
            if (vec == kVecZ && e.sides[vec].source == 1)
            {
                //Z 侧数学回显：逐点配对二元按行求值（与转换/曲线逐点应用
                //口径同源；数值表是点表，网格类条目不摊平网格——切回"数学"
                //来源应用仍走 assembleEntry 网格链路）。求值层已清洗 ±inf
                //→NaN（R40），非有限行 token 写空串
                QVector<double> v;
                if (zMathPointwiseEval(e, v))
                {
                    for (const double d : v)
                        _sideEchoTokens[vec] << (qIsFinite(d)
                            ? QString::number(d, 'f', 6) : QString());
                }
                continue;
            }
            QVector<double> samples;
            if (e.sides[vec].source == 1)
            {
                if (vec == kVecX)
                    samples = makeUSamples();
                else
                {
                    if (!xsOk)
                        continue;   //采样横坐标不可用，回显留空
                    samples = xs;
                }
            }
            QVector<double> v;
            QString err;
            if (assembleSideVector(e, vec, samples, v, nullptr, err))
            {
                for (const double d : v)
                    _sideEchoTokens[vec] << QString::number(d, 'f', 6);
            }
        }
    }

    void mDefine3DCurveDlg::loadSideToPanel()
    {
        //活动向量侧 -> 来源单选/页/文件分量/表达式/数值表（程序回填全程 syncing）
        _syncing = true;
        if (_current >= 0 && _current < _entries.size())
        {
            const Define3DEntry& e = _entries.at(_current);
            const Define3DSide& side = e.sides[_vectorSide];

            for (int i = 0; i < kVecCount; ++i)
                _vectorRadios[i]->setChecked(i == _vectorSide);
            _fileRadio->setChecked(side.source == 0);
            _mathRadio->setChecked(side.source == 1);
            _valueRadio->setChecked(side.source == 2);
            _sourceStack->setCurrentIndex(side.source);

            //文件页：路径回显 + 分量列（解析缓存列名 + 首项"未选择"）
            if (e.filePath.isEmpty())
            {
                _fileEdit->clear();
                _fileEdit->setToolTip(QString());
            }
            else
            {
                _fileEdit->setText(elidedPath(e.filePath, 420));
                _fileEdit->setToolTip(e.filePath);
            }
            _componentCombo->blockSignals(true);
            _componentCombo->clear();
            _componentCombo->addItem(QStringLiteral("未选择"));
            if (e.fileParsed)
            {
                //展示名经 componentDisplayName（矩阵表=Column N）；分量下标
                //仍=数据列下标（addItem 序=解析缓存列序），装配映射零变化
                for (int c = 0; c < e.parsed.columnNames.size(); ++c)
                    _componentCombo->addItem(componentDisplayName(e, c));
            }
            const int compIdx = side.component + 1;
            _componentCombo->setCurrentIndex(
                compIdx >= 0 && compIdx < _componentCombo->count() ? compIdx : 0);
            _componentCombo->blockSignals(false);

            //数学页：表达式回显
            _exprEdit->setText(side.expression);
        }
        _syncing = false;

        //数值表可编辑列随侧来源（refreshValueTable 自带 syncing 护栏）
        refreshValueTable();
        refreshActionStates();
        //y 键启停随条目类型×活动向量侧（选中载入/向量侧切换/来源切换三链路
        //均经本函数收口）
        refreshYKeyState();
    }

    void mDefine3DCurveDlg::loadEntryToPanel()
    {
        //条目 -> 左栏对象行 + 右栏当前侧；类型下拉不随条目回显——它是类型视图
        //过滤器（可见条目类型恒等于当前过滤类型），按条目回写既无必要，反向
        //驱动视图切换还会造成选中漂移
        _syncing = true;
        if (_current >= 0 && _current < _entries.size())
        {
            const Define3DEntry& e = _entries.at(_current);
            _includeCheck->setChecked(e.include);
            _nameEdit->setText(e.name);
        }
        _syncing = false;
        loadSideToPanel();
    }

    QString mDefine3DCurveDlg::componentDisplayName(const Define3DEntry& e, int col) const
    {
        //矩阵表（表头行为数值 token）列名展示恒回退 Column N——不把数值 token
        //当列名，与曲线部分（2D/无表头 csv 口径）一致；仅展示层改名，分量下标
        //仍=数据列下标（装配路径 assembleSideVector 不动，解析器零改动）。
        //其余文件维持现行列名（表头名或 Column N 回退名）
        if (!e.filePath.isEmpty() && e.fileParsed
            && m3DPlotDataParser::isMatrixTable(e.parsed))
            return QStringLiteral("Column %1").arg(col + 1);
        return e.parsed.columnNames.value(col);
    }

    // =============================================================================
    // 来源切换转换链/按侧脏标记（对齐 2D mDefineCurveDlg 同名链路）
    // =============================================================================

    //按侧数值脏读（2D sideValueDirty 同构）：脏=该侧数值列内容被用户改过
    //（改格/行级操作置脏；程序装配/回填/回显不算改动），离开"数值"脏确认
    //按侧判定；V 侧无数值列恒非脏
    bool mDefine3DCurveDlg::sideValueDirty(const Define3DEntry& e, int vector) const
    {
        switch (vector)
        {
        case kVecX: return e.xValueDirty;
        case kVecY: return e.yValueDirty;
        case kVecZ: return e.zValueDirty;
        case kVecU: return e.uValueDirty;
        default:    return false;
        }
    }

    void mDefine3DCurveDlg::setSideValueDirty(Define3DEntry& e, int vector, bool dirty) const
    {
        switch (vector)
        {
        case kVecX: e.xValueDirty = dirty; break;
        case kVecY: e.yValueDirty = dirty; break;
        case kVecZ: e.zValueDirty = dirty; break;
        case kVecU: e.uValueDirty = dirty; break;
        default: break;   //V 侧无数值列，不持脏
        }
    }

    //该侧当前来源有内容（2D sideHasContent 同构）：文件=已选文件且已解析且
    //分量列在界内；数学=表达式非空；数值分支走不到（仅切出侧 source∈{0,1}
    //时被询问）
    bool mDefine3DCurveDlg::sideHasContent(const Define3DEntry& e, int vector) const
    {
        const Define3DSide& sd = e.sides[vector];
        switch (sd.source)
        {
        case 0:
            return !e.filePath.isEmpty() && e.fileParsed
                && sd.component >= 0 && sd.component < e.parsed.columns.size();
        case 1:
            return !sd.expression.trimmed().isEmpty();
        default:
            return false;
        }
    }

    //"离开数值"脏确认框（2D leaveValueConfirmed 同文案/按钮/默认钮）：切回时
    //该侧数值列会按切出侧来源重新装配，用户改过的数据将会丢失。自动装配/回填
    //不算改动（脏标记只由用户编辑置位），未动过的侧由调用方跳过本框。返回
    //false=用户取消
    bool mDefine3DCurveDlg::leaveValueConfirmed()
    {
        return QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("确认继续操作，刚才编辑的所有数据将会丢失。是否继续？"),
            QStringLiteral("取消"), QStringLiteral("继续"), QString(), 1, 0) == 1;
    }

    //Z 侧数学来源逐点配对二元求值（曲线类转换倒表/各类条目只读回显共用，与
    //assembleEntry 曲线逐点模式同源序列）：装配 X（数学来源=u∈[0,1] 均匀
    //101 点）→装配 Y（采样横坐标=装配出的 X）→按行位配对截齐到较少者（静默，
    //转换/回显无 note 通道）→evaluatePointwiseXY 逐点 z_i=f(x_i,y_i)（调用方
    //保证 xs/ys 等长；单变量表达式在成对序列上为等价形态，统一走此路径）。
    //网格类（瀑布/曲面/柱状）条目转换已改整网格摊平（dumpSideToValueText
    //网格分支，数值网格态），不进本函数；回显维持逐点口径（R39 网格求值仅在
    //应用装配路径，切回"数学"来源应用仍走 assembleEntry 网格链路
    //evaluateGrid，零影响）。成功=true 且 out=逐点值（求值层已清洗 ±inf→NaN，
    //非有限行由调用方写空串）；X/Y 装配不出/表达式空/求值失败=false 且 out
    //清空
    bool mDefine3DCurveDlg::zMathPointwiseEval(const Define3DEntry& e,
        QVector<double>& out) const
    {
        out.clear();
        if (e.sides[kVecZ].source != 1)
            return false;   //仅 Z 侧数学来源适用
        QString err;
        //X 侧先装配（数学来源=参数 u∈[0,1] 均匀采样，同应用装配口径）
        QVector<double> xs;
        if (!assembleSideVector(e, kVecX, makeUSamples(), xs, nullptr, err))
            return false;
        //Y 侧装配（数学来源采样横坐标=装配出的 X，同应用装配口径）
        QVector<double> ys;
        if (!assembleSideVector(e, kVecY, xs, ys, nullptr, err))
            return false;
        //按行位配对截齐到较少者（静默——转换/回显无 note 通道；逐点求值
        //必须成对）
        const int m = qMin(xs.size(), ys.size());
        xs.resize(m);
        ys.resize(m);
        //逐点二元求值（含 y 与否均可）；空序列由求值器报错=false，与"有内容
        //装配不出"口径合流
        return mCurveExpressionEvaluator::evaluatePointwiseXY(
            e.sides[kVecZ].expression, xs, ys, out, err);
    }

    //切入"数值"（按侧，2D dumpSideToValueText 同名同语义）：把该侧切出前来源
    //内容装配进该侧数值列，供微调。调用时机约束（2D R20-B02 同款）：须在该侧
    //source 落新值之前调用——此时 sides[vector].source 仍读旧来源，下方卫语句
    //与装配分派才指向切出侧。该侧旧来源无内容（未解析/未选分量/表达式空）时
    //保留已存数值列——数值可能是该侧唯一内容，切走再切回不丢；有内容但装配
    //不出（表达式写错/求值失败/X 或 Y 装配不出等）才清空该侧列（残留上次倒表
    //内容会误导"数值"来源读数，2D 同口径）。装配成功=该侧列逐行写 'f' 6
    //（求值非有限行写空串=装配跳行，不写 "nan"/"inf" 毒化文本）、表行数扩到
    //max(现值Rows,数据长度)、扩行仅 U 列补 "0"（其余扩行列空串——X/Y/Z 空格
    //=装配跳行语义，仅 U 补 0）。返回 false=有内容装配不出（已清该侧列）。
    //网格类条目 Z 侧数学来源例外=整网格摊平（数值网格态）：清空现有 valueRows
    //后 gridRows×gridCols 行逐单元写入（X/Y/Z 逐单元 + U 补 0），维度落条目
    //valueGridRows/Cols——曲面从数值照常绘制的前提（表=整网格数据而非逐行
    //配对点列）
    bool mDefine3DCurveDlg::dumpSideToValueText(Define3DEntry& e, int vector) const
    {
        const int old = e.sides[vector].source;
        if (old == 2)
            return true;   //数值→数值无装配语义
        if (vector == kVecV)
            return true;   //V 侧无数值列，转换链不适用（现状边界）
        if (!sideHasContent(e, vector))
            return true;   //无内容：保留已存数值列（"切出去看了眼再切回不丢"）
        const int dataCol = vector == kVecU ? 0 : vector == kVecX ? 1
            : vector == kVecY ? 2 : 3;
        QVector<double> v;
        bool dumped = false;
        if (vector == kVecZ && old == 1 && entryInMathGridMode(e))
        {
            //网格类 Z 数学→数值：整网格摊平（数值网格态）——镜像 assembleEntry
            //网格模式装配序列（X 轴=u 采样/文件列、Y 轴=采样横坐标 X、
            //evaluateGrid 全网格求值），成功=清空现有 valueRows 后逐单元写入
            //gridRows×gridCols 行（行主序：X=横轴第 c 值、Y=纵轴第 r 值、
            //Z=单元值，'f' 6；NaN 单元写空串=断口；扩行即"新行"，U 列补 "0"
            //照旧），维度落条目 valueGridRows/Cols（应用按此直读装配，不回
            //猜）。装配不出（X/Y 装配失败/表达式非法/全部网格点无法计算）=
            //不落网格态，走下方既有"清该侧列+false"失败口径（表原样保留）
            e.valueGrid = false;   //先退态：装配不出不得残留网格维度（成功重落）
            QString sideErr;
            QVector<double> xsAxis;
            QVector<double> ysAxis;
            QVector<double> zsGrid;
            int gridRows = 0;
            int gridCols = 0;
            int nonFinite = 0;
            if (assembleSideVector(e, kVecX, makeUSamples(), xsAxis, nullptr, sideErr)
                && !xsAxis.isEmpty()
                && assembleSideVector(e, kVecY, xsAxis, ysAxis, nullptr, sideErr)
                && !ysAxis.isEmpty()
                && mCurveExpressionEvaluator::evaluateGrid(e.sides[kVecZ].expression,
                    xsAxis, ysAxis, zsGrid, gridRows, gridCols, sideErr, &nonFinite)
                && gridRows > 0 && gridCols > 0
                && nonFinite < gridRows * gridCols)
            {
                e.valueRows.clear();
                for (int r = 0; r < gridRows; ++r)
                {
                    for (int c = 0; c < gridCols; ++c)
                    {
                        const double zv = zsGrid.at(r * gridCols + c);
                        e.valueRows.append({ QStringLiteral("0"),
                            QString::number(xsAxis.at(c), 'f', 6),
                            QString::number(ysAxis.at(r), 'f', 6),
                            qIsFinite(zv) ? QString::number(zv, 'f', 6)
                                          : QString() });
                    }
                }
                e.valueGrid = true;
                e.valueGridRows = gridRows;
                e.valueGridCols = gridCols;
                return true;
            }
            dumped = false;   //装配不出：既有失败口径收口（清 Z 列+false）
        }
        else if (vector == kVecZ && old == 1)
        {
            //Z 侧数学来源：逐点配对二元按行求值（zMathPointwiseEval，与
            //曲线逐点应用口径同源；曲线类（plot3dType==1）转换口径零变化
            //——数值表是点表逐行配对；切回"数学"来源应用仍走 assembleEntry
            //网格/逐点链路，零影响）
            dumped = zMathPointwiseEval(e, v) && !v.isEmpty();
        }
        else
        {
            //采样横坐标（条带口径，与回显/应用装配同源）：X/U 侧数学=u∈[0,1]
            //101 点；Y 侧数学=以装配的 X 数据为采样（X 装配不出=该侧装配不出
            //→清列）；文件来源不依赖采样。Z 侧数学不进此路径（上方逐点配对
            //分支）
            QVector<double> samples;
            if (old == 1 && vector == kVecY)
            {
                QVector<double> xs;
                QString xsErr;
                if (!assembleSideVector(e, kVecX, makeUSamples(), xs, nullptr, xsErr)
                    || xs.isEmpty())
                {
                    for (int r = 0; r < e.valueRows.size(); ++r)
                        e.setValueAt(r, dataCol, QString());   //清该侧数值列
                    return false;
                }
                samples = xs;
            }
            else if (old == 1)
            {
                samples = makeUSamples();
            }
            QString err;
            dumped = assembleSideVector(e, vector, samples, v, nullptr, err)
                && !v.isEmpty();
        }
        if (dumped)
        {
            const int n = qMax(e.valueRows.size(), v.size());
            while (e.valueRows.size() < n)
                e.valueRows.append({ QStringLiteral("0"), QString(), QString(),
                    QString() });   //扩行：仅 U 列补 "0"，其余扩行列空串
            for (int r = 0; r < v.size(); ++r)
            {
                const double d = v.at(r);
                e.setValueAt(r, dataCol, qIsFinite(d)
                    ? QString::number(d, 'f', 6) : QString());
            }
            return true;
        }
        for (int r = 0; r < e.valueRows.size(); ++r)
            e.setValueAt(r, dataCol, QString());   //有内容装配不出：清该侧数值列
        return false;
    }

    //来源切换链路（活动向量侧，2D applySourceChange R21 链路的 3D 适配）：
    //离开"数值"脏确认→切入"数值"倒表（该侧切出前来源内容装配进数值列）→
    //落新来源→切页/整表重建（回显 token 现算）。返回 false=用户取消（来源
    //单选已回退，状态零变化）。向量侧切换不走本链路（3D 按侧模型与 2D R21
    //激活列模型不同，onVectorChanged 仅回显该侧现状）
    bool mDefine3DCurveDlg::applySourceChange(int id)
    {
        if (_current < 0 || _current >= _entries.size())
            return false;
        Define3DEntry& e = _entries[_current];
        const int vector = _vectorSide;
        const int old = e.sides[vector].source;
        //离开"数值"：该侧被用户改过（改格/行级操作置脏）→脏确认框；取消=
        //来源单选回退数值（QSignalBlocker 屏蔽 toggled 重入）+状态零变化
        //（来源/页面/表/脏标记全不动，2D 取消口径）
        if (old == 2 && id != 2 && sideValueDirty(e, vector)
            && !leaveValueConfirmed())
        {
            const QSignalBlocker rollback(_valueRadio);
            _valueRadio->setChecked(true);
            return false;
        }
        //切入"数值"：先把该侧切出前来源内容装配进该侧数值列（倒表须在 source
        //落新值之前——此时 sides[vector].source 仍读旧来源，2D R20-B02 同约束）；
        //程序装配非用户改动，装配/清列后复位该侧脏标记（2D 同口径——原脏标记
        //随本次倒表作废，无内容保留路径亦复位）
        if (id == 2 && old != 2)
        {
            dumpSideToValueText(e, vector);
            setSideValueDirty(e, vector, false);
        }
        e.sides[vector].source = id;
        //Z 侧切离"数值"：退网格态（表内容保留原样不回滚——应用按各侧来源
        //口径装配；切回 Z 数值且数学来源有内容=重新倒表覆盖，维度按新转换
        //落定。条目类型创建后固定无"切类型"入口，entryInValueGridMode 自带
        //类型条件兜底）
        if (vector == kVecZ && id != 2)
            e.valueGrid = false;
        if (id == 2)
            ensureValueRows(e);   //倒表后仍空表（旧来源无内容）时补首访种子
        //切页+数值表整表重建（回显 token 现算）+右栏刷新，全程 _syncing
        //（程序回填不置脏、不触发编辑入口）
        loadSideToPanel();
        return true;
    }

    //行级操作（增/删/清空/剪切/粘贴）置脏（2D markValueRowOpDirty 口径的 3D
    //适配）：行数变化实际改写整表行位——3D 数值列按侧并存且全部数值侧列
    //（source==2）同为可编辑列，行位内容随行数变化重排，故置脏全部数值侧
    //（2D 单激活列模型下等价于"置脏激活列"；头注释"全部数值侧"同文）；
    //复制不改动内容不置脏（2D 调用点口径：复制提前返回不经置脏点）
    void mDefine3DCurveDlg::markValueRowOpDirty(Define3DEntry& e)
    {
        for (int dataCol = 0; dataCol < 4; ++dataCol)
        {
            const int vec = kDataColVector[dataCol];
            if (e.sides[vec].source == 2)
                setSideValueDirty(e, vec, true);
        }
    }

    // =============================================================================
    // 左栏槽
    // =============================================================================

    void mDefine3DCurveDlg::onListSelectionChanged()
    {
        if (_syncing)
            return;
        //行 -> 条目下标经 UserRole 映射（过滤视图下行号≠条目下标）
        _current = entryAtRow(_curveList->currentRow());
        loadEntryToPanel();
    }

    void mDefine3DCurveDlg::onAddClicked()
    {
        Define3DEntry e;
        //新增条目类型跟随当前类型视图（曲线→1 曲面→2 瀑布图→0 柱状图→3）；
        //越界（下拉无合法选中）直接返回——守卫纯防御，防默认名计数数组越界
        e.plot3dType = viewPlot3dType();
        if (e.plot3dType < 0)
            return;
        //默认名按条目类型生成（"曲线 N/曲面 N/瀑布图 N/柱状图 N"各类型独立计数）
        e.name = nextEntryName(e.plot3dType);
        ++_typeCounters[e.plot3dType];
        _entries.append(e);
        //新条目类型==当前视图类型，必然落在过滤列表末行
        _syncing = true;
        _curveList->addItem(e.name);
        _curveList->item(_curveList->count() - 1)
            ->setData(Qt::UserRole, _entries.size() - 1);
        _curveList->setCurrentRow(_curveList->count() - 1);
        _syncing = false;
        //setCurrentRow 的信号已被 _syncing 压掉，_current 须显式跟上——否则
        //右栏按"无选中"整体禁用，来源/矢量切换全部无效
        _current = _entries.size() - 1;
        loadEntryToPanel();
        refreshActionStates();
    }

    void mDefine3DCurveDlg::onCutClicked()
    {
        if (_current < 0 || _current >= _entries.size())
            return;
        _entryClip = _entries.at(_current);
        _hasEntryClip = true;
        const int removedIdx = _current;
        const int prevRow = rowForEntry(removedIdx);
        _entries.removeAt(removedIdx);
        //只按当前过滤视图重建（被剪条目移除，其他类型条目不进本视图不受影响）
        rebuildFilteredList();
        //选中补位：取被剪条目原行位（越界收敛末行；无同类条目=无选中态）
        _syncing = true;
        int row = -1;
        if (_curveList->count() > 0)
            row = qBound(0, prevRow, _curveList->count() - 1);
        _curveList->setCurrentRow(row);
        _syncing = false;
        _current = entryAtRow(row);
        loadEntryToPanel();
        refreshActionStates();
    }

    void mDefine3DCurveDlg::onCopyClicked()
    {
        if (_current < 0 || _current >= _entries.size())
            return;
        _entryClip = _entries.at(_current);
        _hasEntryClip = true;
        refreshActionStates();
    }

    void mDefine3DCurveDlg::onPasteClicked()
    {
        if (!_hasEntryClip)
            return;
        Define3DEntry e = _entryClip;
        e.editCurveId = -1;              //粘贴条目是新建副本，解除原曲线编辑绑定
        //粘贴按剪贴板条目自身类型重命名（续该类型计数，避免同名歧义）
        e.name = nextEntryName(e.plot3dType);
        ++_typeCounters[e.plot3dType];
        //剪贴板条目按完整定义（含原类型）追加；类型 ≠ 当前视图类型时先把下拉
        //切到该类型（onTypeChanged 随之重建过滤视图），保证粘贴条目可见可选中
        for (int i = 0; i < 4; ++i)
        {
            if (kTypePlot3d[i] == e.plot3dType)
            {
                _typeCombo->setCurrentIndex(i);   //同视图无信号，不重复重建
                break;
            }
        }
        _entries.append(e);
        //粘贴条目类型==切换后的当前视图类型，必然落在过滤列表末行
        _syncing = true;
        _curveList->addItem(e.name);
        _curveList->item(_curveList->count() - 1)
            ->setData(Qt::UserRole, _entries.size() - 1);
        _curveList->setCurrentRow(_curveList->count() - 1);
        _syncing = false;
        _current = _entries.size() - 1;   //同 onAddClicked：信号已压，显式跟上
        loadEntryToPanel();
        refreshActionStates();
    }

    void mDefine3DCurveDlg::onIncludeToggled(bool checked)
    {
        if (_syncing || _current < 0 || _current >= _entries.size())
            return;
        _entries[_current].include = checked;
        refreshActionStates();
    }

    void mDefine3DCurveDlg::onNameEdited(const QString& text)
    {
        if (_syncing || _current < 0 || _current >= _entries.size())
            return;
        _entries[_current].name = text;
        const int row = rowForEntry(_current);
        if (row >= 0)
            _curveList->item(row)->setText(text);
    }

    void mDefine3DCurveDlg::onTypeChanged(int index)
    {
        if (_syncing)
            return;
        if (index < 0 || index > 3)
            return;
        //类型下拉=类型视图过滤器：切换只重建过滤列表并维护选中态，不回写条目
        //plot3dType（条目类型创建后固定，应用按条目自身类型装配）
        rebuildFilteredList();
        //目标类型有条目→自动选中第一条并载入；无条目→无选中态（对象行/右栏
        //禁用，下拉自身保持可用）
        _syncing = true;
        _curveList->setCurrentRow(_curveList->count() > 0 ? 0 : -1);
        _syncing = false;
        _current = entryAtRow(_curveList->currentRow());
        loadEntryToPanel();
        refreshActionStates();
    }

    // =============================================================================
    // 矢量/来源槽
    // =============================================================================

    void mDefine3DCurveDlg::onVectorChanged(int id)
    {
        if (_syncing)
            return;
        if (id < 0 || id >= kVecCount)
            return;
        _vectorSide = id;
        loadSideToPanel();
    }

    void mDefine3DCurveDlg::onSourceChanged(int id)
    {
        if (_syncing)
            return;
        //来源切换走共用链路（离开数值脏确认/切入数值倒表/切页回显收口），
        //落新来源等在链路内完成；取消=链路内已回退单选并零状态返回
        applySourceChange(id);
    }

    // =============================================================================
    // 文件页槽
    // =============================================================================

    void mDefine3DCurveDlg::onBrowseClicked()
    {
        if (_current < 0 || _current >= _entries.size())
            return;
        //过滤器对齐"从文件新建三维图"："数据文件 (*.txt *.csv);;所有文件 (*.*)"
        const QString path = QFileDialog::getOpenFileName(this,
            QStringLiteral("选择数据文件"), QString(),
            QStringLiteral("数据文件 (*.txt *.csv);;所有文件 (*.*)"));
        if (path.isEmpty())
            return;   //取消选择不清空既有回显
        Define3DEntry& e = _entries[_current];
        e.filePath = path;
        e.fileParsed = false;
        e.fileInvalid = false;
        e.parsed = AsciiColumnFile();
        //即时解析（列名供分量下拉）；失败红字警告经信息输出带原因
        QString err;
        if (m3DPlotDataParser::readFile(e.filePath, &e.parsed, &err))
            e.fileParsed = true;
        else
            mCurveDocController::getInstance()->reportWarning(
                QStringLiteral("定义曲线：数据文件解析失败——%1").arg(err));
        //新文件列数变少时失效的分量选择复位为未选
        const int colCount = e.fileParsed ? e.parsed.columnNames.size() : 0;
        for (int i = 0; i < kVecCount; ++i)
        {
            if (e.sides[i].component >= colCount)
                e.sides[i].component = -1;
        }
        loadSideToPanel();
    }

    void mDefine3DCurveDlg::onComponentChanged(int index)
    {
        if (_syncing || _current < 0 || _current >= _entries.size())
            return;
        //首项"未选择"= -1，其余=文件列下标
        _entries[_current].sides[_vectorSide].component = index - 1;
    }

    // =============================================================================
    // 数学页槽
    // =============================================================================

    void mDefine3DCurveDlg::onExprEdited(const QString& text)
    {
        if (_syncing || _current < 0 || _current >= _entries.size())
            return;
        _entries[_current].sides[_vectorSide].expression = text;
    }

    // =============================================================================
    // 数值页槽
    // =============================================================================

    void mDefine3DCurveDlg::onValueCellChanged(int row, int col)
    {
        if (_syncing || _current < 0 || _current >= _entries.size())
            return;
        if (col <= 0 || col > 4)
            return;   //点列只读；越界忽略
        QTableWidgetItem* it = _valueTable->item(row, col);
        if (!it)
            return;
        Define3DEntry& e = _entries[_current];
        ensureValueRows(e);
        //U 列收到空文本归一写 "0"（仅 U 补 0——X/Y/Z 空格不补：空=装配跳行，
        //等长截齐语义依赖）：写回条目+表格同步显示（程序回写以局部 _syncing
        //包裹防 cellChanged 重入，B03 双保险同款思路）
        if (col == 1 && it->text().trimmed().isEmpty())
        {
            e.setValueAt(row, 0, QStringLiteral("0"));
            _syncing = true;
            it->setText(QStringLiteral("0"));
            _syncing = false;
        }
        else
            e.setValueAt(row, col - 1, it->text());
        //改格置该侧脏（仅数值侧列：非数值侧列只读编辑不到；U 列恒可编辑但
        //U 侧来源≠数值时不计脏——该列内容随后续切"数值"按旧来源倒表装配，
        //2D 数值页仅数值列可编辑的同构约束）
        const int vec = kDataColVector[col - 1];
        if (e.sides[vec].source == 2)
            setSideValueDirty(e, vec, true);
        //B03 双保险（缺陷单方向 2）：行内状态收口内的程序回写（红字翻转等派生
        //dataChanged）以局部 _syncing 包裹，不回流编辑入口；护栏只覆盖本函数
        //末尾一次收口调用，不泄漏到入口卫语句之外
        _syncing = true;
        refreshValueCellStates(row);
        _syncing = false;
    }

    QList<int> mDefine3DCurveDlg::selectedValueRowsImpl() const
    {
        QSet<int> rows;
        for (const QModelIndex& idx : _valueTable->selectionModel()->selectedIndexes())
            rows.insert(idx.row());
        QList<int> out = rows.values();
        std::sort(out.begin(), out.end());
        return out;
    }

    //数值页行操作工具条启停（对齐二维 [+][-][清空]）：[-]随选中行（无选中
    //不可删）、[清空]随表行数（无行不可清，与选中无关）；[+]恒可用（表尾
    //追加无前置条件）
    void mDefine3DCurveDlg::refreshValueToolBarStates()
    {
        //构造期护栏：工具条建于 buildValuePage，早到的初始化回调无钮可刷
        if (!_vdelRowBtn || !_vclearBtn)
            return;
        _vdelRowBtn->setEnabled(_valueTable->currentRow() >= 0);
        _vclearBtn->setEnabled(_valueTable->rowCount() > 0);
    }

    //在 at 位插一空白行（全 0，同既有插入行初始值口径；越界钳制=表尾）。
    //工具条[+]与右键"在上方/下方插入一行"共用——行操作只改条目 valueRows 后
    //按既有回显缓存整表回填（免重复装配），再落新行 U 格（恒可编辑）直接可输；
    //置脏由调用方按各自口径收口（工具条[+]=数值侧内容改动置脏，右键插入
    //不置脏，2D 调用点同口径）
    void mDefine3DCurveDlg::insertValueRowAt(int at)
    {
        if (_current < 0 || _current >= _entries.size())
            return;
        Define3DEntry& e = _entries[_current];
        ensureValueRows(e);
        e.valueRows.insert(qBound(0, at, e.valueRows.size()),
            { QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("0") });
        rebuildValueTableView();
        const int r = qMin(at, _valueTable->rowCount() - 1);
        if (r >= 0 && _valueTable->item(r, 1))
        {
            _valueTable->setCurrentCell(r, 1);
            _valueTable->editItem(_valueTable->item(r, 1));   //新行落格直接可输
        }
        refreshValueToolBarStates();   //增行后重申启停（清空钮随行数启用）
    }

    void mDefine3DCurveDlg::onValueAddClicked()
    {
        //工具条[+]：表尾追加一空白行（无前置条件，二维 onAddValueRow 同款）；
        //增行=数值侧内容改动（置脏全部数值侧，二维调用点同口径）
        if (_current < 0 || _current >= _entries.size())
            return;
        insertValueRowAt(_entries[_current].valueRows.size());
        markValueRowOpDirty(_entries[_current]);
    }

    void mDefine3DCurveDlg::onValueClearClicked()
    {
        //工具条[清空]（对齐二维 onClearValueRows，R21-B01 修复后口径）：一键
        //清空数值表全部行。破坏性操作执行前弹确认框（二维同款文案：取消/继续，
        //默认继续）；取消=不清空。清空后空表为持久态（ensureValueRows 种子只
        //补一次，不自动回填零值行）
        if (_current < 0 || _current >= _entries.size())
            return;
        Define3DEntry& e = _entries[_current];
        ensureValueRows(e);
        if (e.valueRows.isEmpty())
            return;   //无行可清（按钮置灰兜底：直接返回）
        if (QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("确认清空全部行数据？是否继续？"),
            QStringLiteral("取消"), QStringLiteral("继续"), QString(), 1, 0) != 1)
            return;
        e.valueRows.clear();
        rebuildValueTableView();   //整表回填（尾部重申工具条启停，[清空]随即置灰）
        markValueRowOpDirty(e);   //清空=数值侧内容改动（二维 onClearValueRows 同口径）
    }

    void mDefine3DCurveDlg::onValueContextMenu(const QPoint& pos)
    {
        if (_current < 0 || _current >= _entries.size())
            return;
        const int row = _valueTable->rowAt(pos.y());
        //右键落在某行且该行不在选中集中时，先选中该行（右键即选中，照常见
        //表格交互，二维同款）
        if (row >= 0 && _valueTable->item(row, 1)
            && !_valueTable->item(row, 1)->isSelected())
            _valueTable->setCurrentCell(row, 1);
        const bool hasRow = row >= 0;
        const QList<int> rows = selectedValueRowsImpl();
        const bool hasSel = !rows.isEmpty();

        QMenu menu(this);
        const QIcon addIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/添加.svg"));
        QAction* insAbove = menu.addAction(addIcon, QStringLiteral("在上方插入一行"));
        QAction* insBelow = menu.addAction(addIcon, QStringLiteral("在下方插入一行"));
        menu.addSeparator();
        QAction* cutAct = menu.addAction(
            QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/剪切.svg")), QStringLiteral("剪切"));
        QAction* copyAct = menu.addAction(
            QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/复制.svg")), QStringLiteral("复制"));
        QAction* pasteAct = menu.addAction(
            QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/粘贴.svg")), QStringLiteral("粘贴"));
        menu.addSeparator();
        QAction* delAct = menu.addAction(
            QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/删除.svg")), QStringLiteral("删除"));

        //启停（二维同款）：无右键行不可插、无选中不可剪切/复制/删除、行剪贴板
        //空不可粘贴（三维行剪贴板语义，不写系统剪贴板）
        insAbove->setEnabled(hasRow);
        insBelow->setEnabled(hasRow);
        cutAct->setEnabled(hasSel);
        copyAct->setEnabled(hasSel);
        pasteAct->setEnabled(!_valueClip.isEmpty());
        delAct->setEnabled(hasSel);

        QAction* act = menu.exec(_valueTable->viewport()->mapToGlobal(pos));
        if (!act)
            return;
        if (act == insAbove || act == insBelow)
        {
            //在右键行上/下方插空白行（新行落 U 格直接可输），收口后即返
            //（复制/剪切等共用尾部刷新）
            insertValueRowAt(act == insAbove ? row : row + 1);
            return;
        }
        if (act == cutAct || act == copyAct)
        {
            //选中行按序放入三维行剪贴板（每行=[U,X,Y,Z] 四值，弹窗会话内，
            //与左栏条目剪贴板互不相通）
            Define3DEntry& e = _entries[_current];
            ensureValueRows(e);
            _valueClip.clear();
            for (int r : rows)
                _valueClip.append(e.valueRows.at(r));
            if (act == copyAct)
                return;   //复制到此为止；剪切继续走删除
        }
        if (act == cutAct || act == delAct)
        {
            //（剪）删选中行；删行=数值侧内容改动（置脏全部数值侧，二维同口径）
            for (int r = rows.size() - 1; r >= 0; --r)
                _entries[_current].valueRows.removeAt(rows.at(r));
            rebuildValueTableView();
            markValueRowOpDirty(_entries[_current]);
        }
        else if (act == pasteAct)
        {
            //粘贴到最后一选中行之后（无选中/空白处=表尾，二维同款）；
            //粘贴=数值侧内容改动（置脏全部数值侧，二维同口径）
            Define3DEntry& e = _entries[_current];
            ensureValueRows(e);
            const int at = rows.isEmpty()
                ? e.valueRows.size() : qMin(rows.last() + 1, e.valueRows.size());
            for (int i = 0; i < _valueClip.size(); ++i)
                e.valueRows.insert(qBound(0, at + i, e.valueRows.size()), _valueClip.at(i));
            rebuildValueTableView();
            markValueRowOpDirty(e);
        }
        refreshValueToolBarStates();   //行数变化不触发选择信号，重申启停
    }

    void mDefine3DCurveDlg::onValueCutClicked()
    {
        //剪切选中行进三维行剪贴板并删行（语义与右键菜单"剪切"一致，供后续
        //行操作入口复用）
        const QList<int> rows = selectedValueRowsImpl();
        if (rows.isEmpty() || _current < 0 || _current >= _entries.size())
            return;
        Define3DEntry& e = _entries[_current];
        ensureValueRows(e);
        _valueClip.clear();
        for (int r : rows)
            _valueClip.append(e.valueRows.at(r));
        for (int r = rows.size() - 1; r >= 0; --r)
            e.valueRows.removeAt(rows.at(r));
        rebuildValueTableView();
        markValueRowOpDirty(e);   //剪切删行=数值侧内容改动（二维同口径）
    }

    void mDefine3DCurveDlg::onValueCopyClicked()
    {
        const QList<int> rows = selectedValueRowsImpl();
        if (rows.isEmpty() || _current < 0 || _current >= _entries.size())
            return;
        const Define3DEntry& e = _entries.at(_current);
        _valueClip.clear();
        for (int r : rows)
            _valueClip.append(e.valueRows.at(r));
    }

    void mDefine3DCurveDlg::onValuePasteClicked()
    {
        if (_valueClip.isEmpty() || _current < 0 || _current >= _entries.size())
            return;
        Define3DEntry& e = _entries[_current];
        ensureValueRows(e);
        //粘贴到最后一选中行之后（无选中/空白处=表尾，二维同款；原 currentRow+1
        //口径与之仅在 Shift 反向多选等边角不一致，按二维对齐改判）
        const QList<int> rows = selectedValueRowsImpl();
        const int at = rows.isEmpty()
            ? e.valueRows.size() : qMin(rows.last() + 1, e.valueRows.size());
        for (int i = 0; i < _valueClip.size(); ++i)
            e.valueRows.insert(qBound(0, at + i, e.valueRows.size()), _valueClip.at(i));
        rebuildValueTableView();
        markValueRowOpDirty(e);   //粘贴=数值侧内容改动（二维同口径）
    }

    void mDefine3DCurveDlg::onValueDeleteClicked()
    {
        //删除选中行（工具条[-]与右键菜单"删除"共用；支持多选，二维同款）；
        //删行=数值侧内容改动（置脏全部数值侧，二维同口径）
        const QList<int> rows = selectedValueRowsImpl();
        if (rows.isEmpty() || _current < 0 || _current >= _entries.size())
            return;
        Define3DEntry& e = _entries[_current];
        for (int r = rows.size() - 1; r >= 0; --r)
            e.valueRows.removeAt(rows.at(r));
        rebuildValueTableView();
        markValueRowOpDirty(e);
    }

    // =============================================================================
    // 装配与应用
    // =============================================================================

    bool mDefine3DCurveDlg::assembleSideVector(const Define3DEntry& e, int vector,
        const QVector<double>& xSamples, QVector<double>& out,
        QString* zeroNote, QString& err) const
    {
        out.clear();
        if (vector < 0 || vector >= kVecCount)
        {
            err = QStringLiteral("向量侧越界");
            return false;
        }
        const Define3DSide& sd = e.sides[vector];

        if (sd.source == 0)
        {
            //文件来源：所选分量列整列取值（非法单元按 0 兜底，照矩阵装配口径）
            if (e.filePath.isEmpty())
            {
                err = QStringLiteral("未选择数据文件");
                return false;
            }
            if (!e.fileParsed)
            {
                err = e.fileInvalid ? QStringLiteral("数据文件解析失败，请重新选择")
                    : QStringLiteral("数据文件尚未解析");
                return false;
            }
            if (sd.component < 0 || sd.component >= e.parsed.columns.size())
            {
                err = QStringLiteral("未选择分量列");
                return false;
            }
            out = e.parsed.columns.at(sd.component);
            if (out.isEmpty())
            {
                err = QStringLiteral("所选分量列无数据");
                return false;
            }
            int zeroFilled = 0;
            for (double& v : out)
            {
                if (!qIsFinite(v))
                {
                    v = 0.0;
                    ++zeroFilled;
                }
            }
            if (zeroFilled > 0 && zeroNote)
                *zeroNote += QStringLiteral("%1 侧 %2 个无效单元按 0 处理；")
                    .arg(QLatin1Char("XYZUV"[vector])).arg(zeroFilled);
            return true;
        }

        if (sd.source == 1)
        {
            //数学来源：表达式按 xSamples 采样求值（X 侧=u 均匀采样，Y/Z 侧=X 数据）
            if (sd.expression.trimmed().isEmpty())
            {
                err = QStringLiteral("表达式为空");
                return false;
            }
            //自变量 y 仅 Z 侧数学来源（四类型 Z 侧均放行——网格类走 evaluateGrid、
            //曲线走逐点配对，实际求值在 assembleEntry 分流；条带 Z 侧不含 y 的
            //仍走单变量 evaluate）。其余任何侧表达式含独立标识符 y 直接报错不
            //静默（引擎"未知变量"文案不点名语义）；曲线引用 p1w1c1.y 同样命中
            //（本弹窗引用来源本就不支持，原路径为语法/缺数据报错）
            static const QRegularExpression yToken(QStringLiteral("\\b[yY]\\b"));
            if (vector != kVecZ && yToken.match(sd.expression).hasMatch())
            {
                err = QStringLiteral("自变量 y 仅 Z 侧数学来源支持");
                return false;
            }
            if (xSamples.isEmpty())
            {
                err = QStringLiteral("自变量 x 不可用（X 侧未装配出数据）");
                return false;
            }
            return mCurveExpressionEvaluator::evaluate(sd.expression, {}, xSamples, out, err);
        }

        //数值来源：数值表对应列逐行解析（空单元格跳行，模块点列装配同口径）
        //数值表列存序 [U, X, Y, Z]：X->1 Y->2 Z->3 U->0；V 侧无列
        const int dataCol = vector == kVecX ? 1 : vector == kVecY ? 2
            : vector == kVecZ ? 3 : vector == kVecU ? 0 : -1;
        if (dataCol < 0)
        {
            err = QStringLiteral("数值来源暂无 V 列");
            return false;
        }
        for (int r = 0; r < e.valueRows.size(); ++r)
        {
            const QString text = e.valueAt(r, dataCol).trimmed();
            if (text.isEmpty())
                continue;
            bool ok = false;
            const double v = text.toDouble(&ok);
            if (!ok)
            {
                err = QStringLiteral("数值表第 %1 行“%2”不是有效数值").arg(r).arg(text);
                return false;
            }
            out.push_back(v);
        }
        if (out.isEmpty())
        {
            err = QStringLiteral("数值列为空");
            return false;
        }
        return true;
    }

    bool mDefine3DCurveDlg::assembleEntry(Define3DEntry& e, QVector<double>& xs,
        QVector<double>& ys, QVector<double>& zs, int& rows, int& cols,
        QString* note, QString& err) const
    {
        rows = 0;
        cols = 0;

        //网格态前置分支（数值网格态）：网格类条目持网格维度且 Z 侧来源=数值
        //——数值表 X/Y/Z 三列即整网格摊平数据，按转换时落定的维度直读装配
        //rows/cols（表即数据：不读 X/Y 各侧自身来源、不做等长截齐；注意
        //entryInMathGridMode 含 Z==数学条件，网格态 Z==2 不满足，类型判定独立
        //进行）。表行数≠维度（增删行后）=清网格态走既有条带路径：维度只在
        //转换时落定不回猜、降级不复原（切回 Z 数值重新倒表可再落），诚实
        //降级+note 提示
        if (e.valueGrid && e.sides[kVecZ].source == 2
            && (e.plot3dType == 0 || e.plot3dType == 2 || e.plot3dType == 3))
        {
            const int tableRows = e.valueRows.size();
            if (tableRows != e.valueGridRows * e.valueGridCols)
            {
                e.valueGrid = false;
                if (note)
                    *note += QStringLiteral("网格行列与表行数不符，已按点列装配；");
            }
            else
            {
                //三列逐行直读（行=网格单元 row-major）：X/Y 单元必填（空格/
                //非法=错误，轴不完整网格无意义）；Z 单元空=NaN 断口（与转换
                //"NaN 单元写空串"对偶）、非法文本=错误
                int holes = 0;
                QString cellErr;
                auto parseGridCol = [&e, tableRows](int dataCol, bool blankAsHole,
                    QVector<double>& out, int& nonFinite, QString& cellErr) -> bool
                {
                    out.resize(tableRows);
                    for (int r = 0; r < tableRows; ++r)
                    {
                        const QString text = e.valueAt(r, dataCol).trimmed();
                        if (text.isEmpty())
                        {
                            if (!blankAsHole)
                            {
                                cellErr = QStringLiteral(
                                    "数值表第 %1 行 X/Y 单元为空，网格装配需完整行列").arg(r);
                                return false;
                            }
                            out[r] = qQNaN();
                            ++nonFinite;
                            continue;
                        }
                        bool ok = false;
                        const double v = text.toDouble(&ok);
                        if (!ok)
                        {
                            cellErr = QStringLiteral("数值表第 %1 行“%2”不是有效数值")
                                .arg(r).arg(text);
                            return false;
                        }
                        out[r] = v;
                    }
                    return true;
                };
                if (!parseGridCol(1, false, xs, holes, cellErr)
                    || !parseGridCol(2, false, ys, holes, cellErr)
                    || !parseGridCol(3, true, zs, holes, cellErr))
                {
                    err = cellErr;
                    return false;
                }
                //非有限统计（R40 口径对齐网格模式）：全部单元无法计算=报错不
                //建图；部分=断口入档+明确告知 N/M（复用既有断口 note 文案，
                //应用链模态提示同触发）；零非法=静默
                if (holes >= tableRows)
                {
                    err = QStringLiteral("Z：全部 %1 个网格点无法计算，未创建曲线。")
                        .arg(tableRows);
                    return false;
                }
                if (holes > 0 && note)
                {
                    *note += QStringLiteral("%1/%2 个网格点无法计算，已按断口显示；")
                        .arg(holes).arg(tableRows);
                }
                rows = e.valueGridRows;
                cols = e.valueGridCols;
                return true;
            }
        }

        //网格模式（网格类 Z 侧数学来源）：X/Y 两轴独立装配后按 X×Y 笛卡尔积全
        //网格求值 Z，rows/cols 实值带出——不做等长截齐（网格两轴独立）
        if (entryInMathGridMode(e))
        {
            QString sideErr;
            //X 侧=横轴列（数学来源=参数 u∈[0,1] 均匀采样不变）
            QVector<double> xsAxis;
            if (!assembleSideVector(e, kVecX, makeUSamples(), xsAxis, note, sideErr))
            {
                err = QStringLiteral("X：") + sideErr;
                return false;
            }
            //Y 侧=纵轴行（数学来源采样横坐标仍取装配出的 X，同条带口径）
            QVector<double> ysAxis;
            if (!assembleSideVector(e, kVecY, xsAxis, ysAxis, note, sideErr))
            {
                err = QStringLiteral("Y：") + sideErr;
                return false;
            }
            //Z 全网格求值：Z(r,c)=f(xs[c], ys[r])，行主序（行=Y 列=X）
            int gridRows = 0;
            int gridCols = 0;
            int nonFinite = 0;
            if (!mCurveExpressionEvaluator::evaluateGrid(e.sides[kVecZ].expression,
                xsAxis, ysAxis, zs, gridRows, gridCols, sideErr, &nonFinite))
            {
                err = QStringLiteral("Z：") + sideErr;
                return false;
            }
            //非有限统计口径（R40）：±inf/NaN 已被求值层统一清洗为 NaN（断口
            //语义，引擎破洞渲染）——部分非法=正常建图+明确告知 N/M；全部非法
            //（恒 NaN/inf 表达式）=报错不建图；零非法=静默
            const int gridTotal = gridRows * gridCols;
            if (nonFinite >= gridTotal)
            {
                err = QStringLiteral("Z：全部 %1 个网格点无法计算（表达式在采样域内无定义），未创建曲线。")
                    .arg(gridTotal);
                return false;
            }
            if (nonFinite > 0 && note)
            {
                *note += QStringLiteral("%1/%2 个网格点无法计算，已按断口显示；")
                    .arg(nonFinite).arg(gridTotal);
            }
            //三轴逐单元扁平展开喂 set3DSeriesData（与文件矩阵装配同口径：
            //xs[r*cols+c]=X 轴第 c 值、ys[r*cols+c]=Y 轴第 r 值，zs 行主序；
            //rows*cols==点数才被控制器认作规则网格）
            const int n = gridRows * gridCols;
            xs.resize(n);
            ys.resize(n);
            for (int r = 0; r < gridRows; ++r)
            {
                for (int c = 0; c < gridCols; ++c)
                {
                    xs[r * gridCols + c] = xsAxis.at(c);
                    ys[r * gridCols + c] = ysAxis.at(r);
                }
            }
            rows = gridRows;
            cols = gridCols;
            return true;
        }

        //曲线逐点配对模式（plot3dType==1 且 Z 侧数学来源）：曲线=点列空间线
        //（引擎 Line3DSeries 折线），网格形态渲染无意义——Z 按点逐点配对求值
        //z_i=f(x_i,y_i)（x=装配 X 数据第 i 点、y=装配 Y 数据第 i 点）。装配序
        //X→Y→Z：X/Y 两轴先按各自来源装配，逐点求值前按行位配对截齐到较少者
        //（对齐既有"各侧行数不一致,已截齐"note 口径），求值后三侧天然等长
        if (e.plot3dType == 1 && e.sides[kVecZ].source == 1)
        {
            QString sideErr;
            //X 侧先装配（数学来源=参数 u∈[0,1] 均匀采样，同条带口径）
            if (!assembleSideVector(e, kVecX, makeUSamples(), xs, note, sideErr))
            {
                err = QStringLiteral("X：") + sideErr;
                return false;
            }
            //Y 侧装配（数学来源采样横坐标仍取装配出的 X，同条带口径）
            if (!assembleSideVector(e, kVecY, xs, ys, note, sideErr))
            {
                err = QStringLiteral("Y：") + sideErr;
                return false;
            }
            //按行位配对截齐到较少者（两轴可异长——文件/数值/数学来源各自独立
            //装配；网格类不截齐，曲线逐点必须成对）
            const int m = qMin(xs.size(), ys.size());
            if (xs.size() != ys.size())
            {
                if (note)
                    *note += QStringLiteral("各侧行数不一致，已截齐至 %1 点；").arg(m);
                xs.resize(m);
                ys.resize(m);
            }
            //逐点二元求值 z_i=f(x_i,y_i)（含 y 与否均可——不含 y 即 z=f(x) 既有
            //单变量口径在成对序列上的等价形态）
            int nonFinite = 0;
            if (!mCurveExpressionEvaluator::evaluatePointwiseXY(
                e.sides[kVecZ].expression, xs, ys, zs, sideErr, &nonFinite))
            {
                err = QStringLiteral("Z：") + sideErr;
                return false;
            }
            //非有限统计口径（R40，与网格/条带同口径）：部分非法=正常建图+明确
            //告知 N/M；全部非法（恒 NaN/inf 表达式）=报错不建图；零非法=静默
            if (nonFinite >= m)
            {
                err = QStringLiteral("Z：全部 %1 个点无法计算（表达式在采样域内无定义），未创建曲线。")
                    .arg(m);
                return false;
            }
            if (nonFinite > 0 && note)
            {
                *note += QStringLiteral("%1/%2 个点无法计算，已按断口显示；")
                    .arg(nonFinite).arg(m);
            }
            //条带入档：rows/cols 恒 0,0（曲线无网格形状；xs/ys/zs 已等长，无需
            //再截齐）
            return true;
        }

        //条带模式（其余一切组合）：X 侧先装配（数学来源=参数 u∈[0,1] 均匀采样）；
        //Y/Z 数学来源以 X 为采样横坐标
        QString sideErr;
        if (!assembleSideVector(e, kVecX, makeUSamples(), xs, note, sideErr))
        {
            err = QStringLiteral("X：") + sideErr;
            return false;
        }
        if (!assembleSideVector(e, kVecY, xs, ys, note, sideErr))
        {
            err = QStringLiteral("Y：") + sideErr;
            return false;
        }
        if (!assembleSideVector(e, kVecZ, xs, zs, note, sideErr))
        {
            err = QStringLiteral("Z：") + sideErr;
            return false;
        }

        //等长截齐（按行位配对取较少者，模块混合来源既有口径）；≥1 点才有效
        int n = qMin(qMin(xs.size(), ys.size()), zs.size());
        if (n < 1)
        {
            err = QStringLiteral("无有效数据点");
            return false;
        }
        if (xs.size() > n || ys.size() > n || zs.size() > n)
        {
            if (note)
                *note += QStringLiteral("各侧行数不一致，已截齐至 %1 点；").arg(n);
        }
        xs.resize(n);
        ys.resize(n);
        zs.resize(n);

        //Z 侧数学来源非有限统计（R40，与网格模式同口径）：求值层已把 ±inf
        //清洗为 NaN（断口语义，引擎破洞渲染）——部分非法=正常建图+明确告知
        //N/M；全部非法（恒 NaN/inf 表达式）=报错不建图；零非法=静默。
        //文件来源本就按 0 兜底不产 NaN、数值来源逐行解析，天然不触发
        if (e.sides[kVecZ].source == 1)
        {
            int nonFinite = 0;
            for (double v : zs)
            {
                if (!qIsFinite(v))
                    ++nonFinite;
            }
            if (nonFinite >= n)
            {
                err = QStringLiteral("Z：全部 %1 个点无法计算（表达式在采样域内无定义），未创建曲线。").arg(n);
                return false;
            }
            if (nonFinite > 0 && note)
            {
                *note += QStringLiteral("%1/%2 个点无法计算，已按断口显示；")
                    .arg(nonFinite).arg(n);
            }
        }
        //条带模式无网格形状：rows/cols=0,0（曲面/瀑布按所选列装配为条带）
        return true;
    }

    int mDefine3DCurveDlg::findExisting3DChart() const
    {
        //查全文档是否已有三维图表（chartType=5）：遍历 工程->页->图表，全文档
        //唯一（R30 口径同"从文件新建三维图"），取首个命中
        mCurveDocController* controller = mCurveDocController::getInstance();
        const CurveDocument* doc = controller->document();
        const int rootId = doc->rootNodeId();
        for (int pageId : doc->childNodeIds(rootId))
        {
            for (int chartId : doc->childNodeIds(pageId))
            {
                if (doc->nodeType(chartId) != CurveNodeType::Chart)
                    continue;
                if (doc->nodeProperty(chartId, CurveProp::kChartType).toInt() != 5)
                    continue;
                return chartId;
            }
        }
        return -1;
    }

    int mDefine3DCurveDlg::target3DChartId() const
    {
        //载入目标三维图：当前激活图表为三维图时优先（存量多三维图会话跟随
        //用户当前视口），否则全文档首个（与 onApply 落点同口径）
        const int active = mCurveViewportFactory::getInstance()->currentChartId();
        const CurveDocument* doc = mCurveDocController::getInstance()->document();
        if (active > 0 && doc->nodeType(active) == CurveNodeType::Chart
            && doc->nodeProperty(active, CurveProp::kChartType).toInt() == 5)
            return active;
        return findExisting3DChart();
    }

    void mDefine3DCurveDlg::loadExistingCurves()
    {
        //打开载入（对齐 2D mDefineCurveDlg onDefineStarted 编辑模式）：每次打开
        //先复位——未应用的定义不落模型直接丢弃（条目/剪贴板/类型计数清零），
        //再载入目标三维图内全部曲线；无三维图/无曲线=初始空态（仅"新增/关闭"
        //可用，同旧首开口径）
        mCurveDocController* controller = mCurveDocController::getInstance();
        const CurveDocument* doc = controller->document();
        _entries.clear();
        _current = -1;
        _hasEntryClip = false;
        _entryClip = Define3DEntry();
        _valueClip.clear();
        _sideEchoTokens.clear();
        for (int i = 0; i < 4; ++i)
            _typeCounters[i] = 0;
        _curveList->blockSignals(true);
        _curveList->clear();
        _curveList->blockSignals(false);

        const int chartId = target3DChartId();
        if (chartId > 0)
        {
            for (int childId : doc->childNodeIds(chartId))
            {
                if (doc->nodeType(childId) != CurveNodeType::Curve)
                    continue;
                addEntryFromCurve(childId);
            }
        }

        //初始选中：打开前选中的曲线（或其 x/y/z 轴壳——轴壳为曲线固定子节点，
        //回挂父曲线）对应条目，否则树序第一条。类型视图下拉跟随初始条目类型
        //（过滤列表只显当前类型，不切视图初始条目不可见）；setCurrentRow/切换
        //全程 _syncing 压信号，_current/回填显式收口（onAddClicked 同款）
        int sel = controller->currentNodeId();
        const CurveNodeType selType = doc->nodeType(sel);
        if (selType == CurveNodeType::AxisX3DData || selType == CurveNodeType::AxisY3DData
            || selType == CurveNodeType::AxisZ3DData)
            sel = doc->parentNodeId(sel);
        if (!_entries.isEmpty())
        {
            int idx = 0;
            for (int i = 0; i < _entries.size(); ++i)
            {
                if (_entries.at(i).editCurveId == sel)
                {
                    idx = i;
                    break;
                }
            }
            const int t = _entries.at(idx).plot3dType;
            for (int di = 0; di < 4; ++di)
            {
                if (kTypePlot3d[di] == t)
                {
                    _syncing = true;
                    _typeCombo->setCurrentIndex(di);
                    _syncing = false;
                    break;
                }
            }
            rebuildFilteredList();
            _syncing = true;
            _curveList->setCurrentRow(rowForEntry(idx));
            _syncing = false;
            _current = idx;
            loadEntryToPanel();
        }
        refreshActionStates();
    }

    void mDefine3DCurveDlg::addEntryFromCurve(int curveId)
    {
        //既有曲线载入为单条目（对齐 2D addEntryFromCurve 同名语义）：名称/
        //plot3dType/编辑绑定随节点，逐侧按溯源还原定义——数学表达式在=数学侧
        //（X/Y=xRef/yRef.expression，Z=z3DExpression，本弹窗 onApply 落模），
        //文件分量列下标在且解析界内=文件侧；都缺席（旧数据/无溯源曲线）=数值
        //兜底（当前数据倒数值列，仍可编辑并原位更新）。任一侧数值兜底时整条
        //倒表一次（数值列共享行结构；非兜底侧列留空走只读回显）
        mCurveDocController* controller = mCurveDocController::getInstance();
        const CurveDocument* doc = controller->document();
        CurveNode* n = doc->node(curveId);
        if (!n || n->type != CurveNodeType::Curve)
            return;
        const CurveSeriesData& s = n->series;

        Define3DEntry e;
        e.editCurveId = curveId;
        e.include = true;
        e.name = doc->nodeName(curveId);
        e.plot3dType = (s.plot3dType >= 0 && s.plot3dType <= 3) ? s.plot3dType : 1;

        //溯源标记=真实文件路径（非保留字）时按文件来源解析（分量列界内判定
        //按侧进行）；解析失败红字提示并按数值兜底（文件路径留存供重新选择）
        if (!s.sourceFile.isEmpty() && s.sourceFile != kMathSourceTag
            && s.sourceFile != kNumericSourceTag)
        {
            e.filePath = s.sourceFile;
            QString err;
            if (m3DPlotDataParser::readFile(e.filePath, &e.parsed, &err))
                e.fileParsed = true;
            else
            {
                e.fileInvalid = true;
                controller->reportWarning(QStringLiteral("定义曲线：曲线「%1」来源文件"
                    "解析失败（%2），已按当前数据倒表载入。").arg(e.name).arg(err));
            }
        }

        const QString sideExpr[3] = { s.xRef.expression, s.yRef.expression,
            s.z3DExpression };
        const int sideComp[3] = { s.x3DComponent, s.y3DComponent, s.z3DComponent };
        bool anyNumeric = false;
        for (int v = kVecX; v <= kVecZ; ++v)
        {
            if (!sideExpr[v - kVecX].trimmed().isEmpty())
            {
                e.sides[v].source = 1;
                e.sides[v].expression = sideExpr[v - kVecX];
            }
            else if (e.fileParsed && sideComp[v - kVecX] >= 0
                && sideComp[v - kVecX] < e.parsed.columns.size())
            {
                e.sides[v].source = 0;
                e.sides[v].component = sideComp[v - kVecX];
            }
            else
            {
                e.sides[v].source = 2;
                anyNumeric = true;
            }
        }
        if (anyNumeric)
            dumpCurveValuesToEntry(curveId, e);

        _entries.append(e);
    }

    void mDefine3DCurveDlg::dumpCurveValuesToEntry(int curveId, Define3DEntry& e) const
    {
        //曲线当前数据倒条目数值列（2D 同名同思路）：三轴逐行 [U, X, Y, Z]，
        //'f' 6 与切源倒表/回显同口径（非有限写空串=装配跳行）；U 列恒数值列
        //补 "0"。仅数值兜底侧列写值（调用方保证至少一侧 source==2；其余列留空，
        //走只读回显现算）；行数=三轴截齐点数。网格类条目持有效网格维度
        //（rows*cols==点数）时落数值网格态——维度随模型还原，应用按维度直读
        //装配渲染真网格（与切源摊平同构）；维度无效退条带（诚实呈现）
        const CurveDocument* doc = mCurveDocController::getInstance()->document();
        CurveNode* n = doc->node(curveId);
        if (!n || n->type != CurveNodeType::Curve)
            return;
        const CurveSeriesData& s = n->series;
        const int nPts = qMin(s.xData.size(), qMin(s.yData.size(), s.zData.size()));

        e.valueGrid = false;
        e.valueGridRows = 0;
        e.valueGridCols = 0;
        const bool gridClass = e.plot3dType == 0 || e.plot3dType == 2 || e.plot3dType == 3;
        if (gridClass && s.plot3dRows > 0 && s.plot3dCols > 0
            && s.plot3dRows * s.plot3dCols == nPts)
        {
            e.valueGrid = true;
            e.valueGridRows = s.plot3dRows;
            e.valueGridCols = s.plot3dCols;
        }

        const bool dumpX = e.sides[kVecX].source == 2;
        const bool dumpY = e.sides[kVecY].source == 2;
        const bool dumpZ = e.sides[kVecZ].source == 2;
        e.valueRows.clear();
        for (int i = 0; i < nPts; ++i)
        {
            QStringList row;
            row << QStringLiteral("0");
            row << (dumpX && qIsFinite(s.xData.at(i))
                ? QString::number(s.xData.at(i), 'f', 6) : QString());
            row << (dumpY && qIsFinite(s.yData.at(i))
                ? QString::number(s.yData.at(i), 'f', 6) : QString());
            row << (dumpZ && qIsFinite(s.zData.at(i))
                ? QString::number(s.zData.at(i), 'f', 6) : QString());
            e.valueRows.append(row);
        }
        //倒表是合法持久态：不再种子补行（空数据保持空表）；程序倒表不算改动，
        //按侧脏标记复位（离开"数值"脏确认按侧判定）
        e.valueSeeded = true;
        e.xValueDirty = false;
        e.yValueDirty = false;
        e.zValueDirty = false;
        e.uValueDirty = false;
    }

    void mDefine3DCurveDlg::onApply()
    {
        mCurveDocController* controller = mCurveDocController::getInstance();

        //逐条目装配（失败红字带条目名与侧原因，不写文档）。非 const 遍历：
        //网格态降级（assembleEntry 清条目网格态）与新建绑定回写（editCurveId）
        //须落条目
        struct AssembledRec
        {
            QString name;
            int plot3dType = 1;
            QVector<double> xs;
            QVector<double> ys;
            QVector<double> zs;
            int rows = 0;   //网格行数（网格模式=len(Y)；条带模式 0）
            int cols = 0;   //网格列数（网格模式=len(X)；条带模式 0）
            int entryIndex = -1;   //来源条目下标（绑定回写定位）
            int curveId = -1;      //应用落点曲线节点 id（复用或新建）
        };
        QList<AssembledRec> recs;
        QString notes;
        QString holeNotes;   //含非有限断口点条目（R40 应用后弹窗明确告知 N/M）
        for (int ei = 0; ei < _entries.size(); ++ei)
        {
            Define3DEntry& e = _entries[ei];
            if (!e.include)
                continue;
            AssembledRec rec;
            rec.name = e.name;
            rec.plot3dType = e.plot3dType;
            rec.entryIndex = ei;
            QString note;
            QString err;
            if (!assembleEntry(e, rec.xs, rec.ys, rec.zs, rec.rows, rec.cols,
                &note, err))
            {
                controller->reportError(QStringLiteral("定义曲线：对象「%1」装配失败——%2。")
                    .arg(e.name).arg(err));
                return;
            }
            recs.append(rec);
            if (!note.isEmpty())
                notes += QStringLiteral("「%1」：%2").arg(e.name).arg(note);
            if (note.contains(QStringLiteral("已按断口显示")))
                holeNotes += QStringLiteral("「%1」：%2\n").arg(e.name).arg(note);
        }
        if (recs.isEmpty())
        {
            controller->reportWarning(QStringLiteral("定义曲线：没有勾选参与应用的曲线对象。"));
            return;
        }

        //单视口定位（R30）：无三维图→新建宏（总是新建视图页）；有→复用
        int chartId = findExisting3DChart();
        QUndoStack* stack = controller->undoStack();
        if (chartId <= 0)
        {
            stack->beginMacro(QStringLiteral("新建三维图"));
            const int pageId = controller->addPage();
            chartId = controller->add3DChart(pageId);
            stack->endMacro();
            if (chartId <= 0)
            {
                controller->reportError(QStringLiteral("定义曲线：三维图创建失败，请重试。"));
                return;
            }
        }

        //绑定/新建收口（对齐 2D editCurveId 口径）：条目绑定节点存活=复用该
        //id 原位更新（不 add3DCurve；条目名≠节点名时名称命令随宏可撤销）；
        //无绑定/绑定已失效（曲线被删，nodeType 查询非 Curve）=宏内新建并回写
        //绑定（重建重绑）。新建部分+名称命令包"定义曲线"宏一次撤销整批还原；
        //数据 set3DSeriesData 直写宏外（既有先例：数据变更不进撤销栈，撤宏
        //还原新建节点与改名、已直写数据留存——撤销语义边界文档明示）
        const CurveDocument* doc = controller->document();
        stack->beginMacro(QStringLiteral("定义曲线"));
        QList<int> curveIds;
        int newCount = 0;
        int updateCount = 0;
        for (int i = 0; i < recs.size(); ++i)
        {
            AssembledRec& rec = recs[i];
            Define3DEntry& e = _entries[rec.entryIndex];
            int curveId = -1;
            if (e.editCurveId > 0
                && doc->nodeType(e.editCurveId) == CurveNodeType::Curve)
            {
                //绑定存活：复用原位更新（必要时间名同步，走可撤销命令）
                curveId = e.editCurveId;
                if (rec.name != doc->nodeName(curveId))
                    controller->setNodeProperty(curveId, CurveProp::kName, rec.name);
                ++updateCount;
            }
            else
            {
                curveId = controller->add3DCurve(chartId, rec.name);
                if (curveId > 0)
                {
                    //新建成功回写绑定：再次应用原位更新，不重复新建
                    e.editCurveId = curveId;
                    ++newCount;
                }
            }
            rec.curveId = curveId;
            curveIds.append(curveId);
        }
        stack->endMacro();

        int created = 0;
        for (int i = 0; i < curveIds.size(); ++i)
        {
            const int curveId = curveIds.at(i);
            if (curveId <= 0)
                continue;
            const AssembledRec& rec = recs.at(i);
            //网格模式（网格类 Z 数学来源）/网格态直读装配 rows/cols 实值入档
            //渲染真网格（曲面/瀑布/柱状）；条带模式传 0,0——按侧装配无网格
            //形状，曲面/瀑布退化为条带（诚实呈现）；曲线逐点配对同为条带入档
            //（rows/cols=0,0）
            controller->set3DSeriesData(curveId, rec.xs, rec.ys, rec.zs,
                rec.plot3dType, rec.rows, rec.cols);
            //溯源落模（重开弹窗载入还原，对齐 2D 定义曲线来源落模思路）：主
            //标记=数学>文件>数值；三侧表达式（X/Y 落 xRef/yRef.expression、
            //Z 落 z3DExpression）与文件分量列下标按侧独立写入，非对应来源侧
            //写空串/-1。直写不进命令栈（同 set3DSeriesData 数据直写口径）
            Define3DEntry& se = _entries[rec.entryIndex];
            const bool anyMath = se.sides[kVecX].source == 1
                || se.sides[kVecY].source == 1 || se.sides[kVecZ].source == 1;
            bool anyFile = false;
            for (int v = kVecX; v <= kVecZ; ++v)
                anyFile = anyFile || (se.sides[v].source == 0 && se.fileParsed);
            const QString srcTag = anyMath ? kMathSourceTag
                : (anyFile ? se.filePath : kNumericSourceTag);
            auto sideExpr = [&se](int v)
            {
                return se.sides[v].source == 1 ? se.sides[v].expression : QString();
            };
            auto sideComp = [&se](int v)
            {
                return (se.sides[v].source == 0 && se.fileParsed
                    && se.sides[v].component >= 0
                    && se.sides[v].component < se.parsed.columns.size())
                    ? se.sides[v].component : -1;
            };
            controller->set3DCurveProvenance(curveId, srcTag,
                sideExpr(kVecX), sideExpr(kVecY), sideExpr(kVecZ),
                sideComp(kVecX), sideComp(kVecY), sideComp(kVecZ));
            ++created;
        }
        if (created < recs.size())
        {
            controller->reportError(QStringLiteral("定义曲线：%1/%2 条曲线创建失败，请重试。")
                .arg(recs.size() - created).arg(recs.size()));
            return;
        }

        //成功提示区分新建/更新（纯新建维持既有文案；编辑既有三维曲线=本弹窗
        //绑定原位更新口径，打开弹窗即载入图内曲线为绑定条目）
        QString msg;
        if (updateCount == 0)
            msg = QStringLiteral("已定义 %1 条三维曲线。").arg(created);
        else if (newCount == 0)
            msg = QStringLiteral("已更新 %1 条三维曲线。").arg(updateCount);
        else
            msg = QStringLiteral("已定义 %1 条、更新 %2 条三维曲线。")
                .arg(newCount).arg(updateCount);
        if (!notes.isEmpty())
            msg += QStringLiteral("（%1）").arg(notes);
        controller->reportInfo(msg);
        //部分采样点无法计算=断口建图成功（R40 验收口径：用户明确得知 N/M；
        //全部非法在装配阶段已报错不建图，不会走到这里）。形态拍板允许
        //QMessageBox information 或状态行——取模态弹窗保证不被忽略，
        //详情同步走上方 reportInfo 输出窗
        if (!holeNotes.isEmpty())
        {
            QMessageBox::information(this, QStringLiteral("定义曲线"),
                QStringLiteral("以下对象含无法计算的采样点，已按断口显示：\n%1")
                    .arg(holeNotes.trimmed()));
        }
        //成功后条目留存不清空（可继续修改再次应用：绑定原位更新/新建并存）
    }

    // =============================================================================
    // 下拉弹出高度钳制（B04 确定性手段：不依赖 maxVisibleItems 的弹出尺寸语义）
    // =============================================================================

    void mDefine3DCurveDlg::bindComboPopupHeight(QComboBox* box)
    {
        //过滤器挂在弹出视图上（view() 惰性创建，首次调用即创建并归 combo 所有：
        //combo 析构随删 view；本弹窗析构时 Qt 自动解除本对象安装的全部过滤器，
        //无悬挂）
        box->view()->installEventFilter(this);
    }

    bool mDefine3DCurveDlg::eventFilter(QObject* watched, QEvent* event)
    {
        //弹出视图 Show 时按实测行高钳 maxHeight。机理：Qt 在弹出容器 show 前
        //已按内容自然高完成容器 resize 与布局（show_helper 先激活布局再派发子件
        //Show），此刻 view/viewport/容器高度均为自然值——行高、余量皆可实测；
        //随后对容器/视图的任何 resize 都被 maximumSize 钳住（QWidget::resize
        //遵守 min/max 约束），弹出不会按未钳 sizeHint 撑回全展开。行数≤上限
        //仅复位钳制，短列表保持自然全展不被误钳
        if (event->type() == QEvent::Show)
        {
            QAbstractItemView* view = qobject_cast<QAbstractItemView*>(watched);
            QWidget* popup = view ? view->window() : nullptr;
            if (view && popup && popup != this)
            {
                //护栏（容器布局由 show 前激活，此处幂等）：确保实测值有效
                if (popup->layout())
                    popup->layout()->activate();
                //先复位上次弹出的旧钳制，再按本次内容决定是否钳
                view->setMaximumHeight(QWIDGETSIZE_MAX);
                popup->setMaximumHeight(QWIDGETSIZE_MAX);
                const int items = view->model() ? view->model()->rowCount() : 0;
                if (items > kComboMaxVisibleItems)
                {
                    //行高=首行 sizeHint 实测（含 QSS 内边距），取不到按字高+上
                    //下 2px 内边距兜底；余量=视图边框/边距 + 容器除视图外的
                    //边距（实测合计 6px）——钳制值含余量才不裁半行
                    int rowH = view->sizeHintForRow(0);
                    if (rowH <= 0)
                        rowH = view->fontMetrics().height() + 4;
                    const int chrome = (view->height() - view->viewport()->height())
                        + qMax(0, popup->height() - view->height());
                    const int maxH = rowH * kComboMaxVisibleItems + chrome;
                    view->setMaximumHeight(maxH);
                    popup->setMaximumHeight(maxH);
                }
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    // =============================================================================
    // 拖动/缩放（照 mCreate3DPlotDlg）
    // =============================================================================

    void mDefine3DCurveDlg::mousePressEvent(QMouseEvent* event)
    {
        //仅记录按下点：拖动延迟到 mouseMoveEvent 中移动超过系统拖动阈值后才
        //真正开始（双击微动不再挪动弹窗）
        if (event->button() == Qt::LeftButton && _titleBar->underMouse())
            _pressPos = event->globalPos();
        QWidget::mousePressEvent(event);
    }

    void mDefine3DCurveDlg::mouseMoveEvent(QMouseEvent* event)
    {
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

    void mDefine3DCurveDlg::mouseReleaseEvent(QMouseEvent* event)
    {
        _dragging = false;
        QWidget::mouseReleaseEvent(event);
    }

    void mDefine3DCurveDlg::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //双击不进入拖动链路
        if (event->button() == Qt::LeftButton)
            _pressPos = event->globalPos();
    }

    void mDefine3DCurveDlg::resizeEvent(QResizeEvent* event)
    {
        //右下角缩放手柄贴角跟随（手柄不占布局行，对齐 2D 定义曲线弹窗）
        if (_grip)
            _grip->move(width() - _grip->width(), height() - _grip->height());
        QWidget::resizeEvent(event);
    }
}
