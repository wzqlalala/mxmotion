#include "mPostTagPanel.h"
#include "mFbdColorPicker.h"
#include "mViewportModelManager.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/PostSelectionCalculator.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mViewportModel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mGroupBox.h"
#include "mChoiceGroup.h"
#include "mPanelTable.h"

//注释标签接入：样式结构与工具接口（InteractionCore install 树）
#include "InteractionCore/Controller/InteractionToolManager.h"
#include "InteractionCore/Tool/AnnotationLabelTool.h"
//实体依附锚点：节点世界坐标查询（后处理数据库/前处理几何引擎）
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostGUIData.h"
#include <mGeoManager.h>
#include "PostViewportWidget.h"
#include "PreViewportWidget.h"

#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPlainTextEdit>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyle>
#include <QFontMetricsF>
#include <QShortcut>
#include <QTimer>
#include <QCursor>
#include <QScreen>
#include <QGuiApplication>
#include <QToolTip>
#include <algorithm>

using namespace MechCommon;
namespace PostWindow
{
    namespace
    {
        //“通用结构软件界面UI设计规则”标准档令牌（--std-*，与测量面板同源）
        const char* kText       = "#424B57";	//正文 --std-text
        const char* kMuted      = "#788391";	//辅助文字 --std-muted
        const char* kDisabled   = "#A6AFBB";	//禁用文字 --std-disabled
        const char* kCtrlBorder = "#DCE2E9";	//控件描边 --std-border
        const char* kLineBorder = "#E4E8EE";	//表格分隔线
        const char* kPrimary    = "#1890FF";	//主蓝 --std-primary
        const char* kSelBg      = "#E8F3FF";	//选中底色 --std-selected
        const char* kDanger     = "#BF3434";	//删除红 --std-error

        //名称表头排序箭头（图灰；SortArrowHeader 专用，2026-11-16 表格收编后唯一保留的表头私值）
        const char* kTagSort     = "#999999";	//名称表头排序箭头（图灰）

        //标准档尺寸令牌：控件 26 / 管理表格行 26 / 筛选 88 / 行距 6
        const int kControl = 26;			//--std-control
        const int kFilterW = 88;			//管理筛选宽 --std-manager-filter-width
        const int kSwatch  = 26;			//名称行色块（原型 .label-color 26×26）

        //主操作蓝按钮（创建）：蓝底白字，悬停深蓝，禁用灰（--std-action/-hover）
        const char* kPrimaryBtnQss =
            "QPushButton{background:#1677C8;border:1px solid #1677C8;color:#FFFFFF;border-radius:2px;}"
            "QPushButton:hover{background:#1267B0;border:1px solid #1267B0;}"
            "QPushButton:disabled{background:#F5F7FA;border:1px solid #DCE2E9;color:#A6AFBB;}";

        //表尾文字钮（全选/显示/隐藏/删除）：透明底、悬停浅蓝、禁用灰字
        const char* kFooterBtnQss =
            "QPushButton{border:0;background:transparent;color:#424B57;border-radius:2px;padding:0 6px;}"
            "QPushButton:hover{background:#F0F7FF;}"
            "QPushButton:disabled{color:#A6AFBB;background:transparent;}";

        //次操作描边钮（取消编辑）：白底常规描边，悬停浅蓝描边
        const QByteArray kSecondaryBtnQssBa = QString(
            "QPushButton{background:#FFFFFF;border:1px solid %1;color:#424B57;border-radius:2px;}"
            "QPushButton:hover{border-color:#1677C8;color:#1677C8;}"
            "QPushButton:disabled{background:#F5F7FA;border-color:#DCE2E9;color:#A6AFBB;}")
            .arg(kLineBorder).toUtf8();
        const char* kSecondaryBtnQss = kSecondaryBtnQssBa.constData();

        //数据字段选项（原型 note-field option 序）
        struct FieldDef { const char* cn; const char* en; };
        const FieldDef kFieldDefs[] = {
            { "当前帧",   "Current Frame" },
            { "仿真步",   "Sim Step" },
            { "模型名称", "Model Name" },
            { "工况名称", "Case Name" },
            { "对象编号", "Object ID" },
            { "对象名称", "Object Name" },
            { "对象坐标", "Object Coord" },
            { "结果数值", "Result Value" },
        };
        const int kFieldCount = sizeof(kFieldDefs) / sizeof(kFieldDefs[0]);

        //附着对象类型（表格"类型"列 + 拾取域）
        const char* kTypeCn[] = { "节点", "单元", "部件" };
        const char* kTypeEn[] = { "Node", "Element", "Part" };

        //名称表头排序箭头（2026-11-12 参考图2）：基类绘制后于"名称"标题文字右侧紧贴
        //叠画灰色 ⇅ 双向小箭头，纯装饰不接排序（图仅名称列有箭头，类型/显隐/颜色无）；
        //QHeaderView 绘制不走 itemDelegate，只能子类化 paintSection
        class SortArrowHeader : public QHeaderView
        {
        public:
            explicit SortArrowHeader(QWidget* parent = nullptr)
                : QHeaderView(Qt::Horizontal, parent) {}

        protected:
            void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override
            {
                QHeaderView::paintSection(painter, rect, logicalIndex);
                if (logicalIndex != 1 || !model())
                    return;
                const QString text = model()->headerData(1, Qt::Horizontal, Qt::DisplayRole).toString();
                if (text.isEmpty())
                    return;
                //按 QSS 段字号 11px 量文字宽（字体族继承表头部件），箭头紧贴其后
                QFont f = font();
                f.setPixelSize(11);
                const qreal cx = rect.left() + 3	//QSS padding-left:3px
                    + QFontMetricsF(f).horizontalAdvance(text) + 4 + 4;
                const qreal cy = rect.center().y();
                const QPointF up[3] = {
                    QPointF(cx - 4, cy - 1), QPointF(cx + 4, cy - 1), QPointF(cx, cy - 5) };
                const QPointF down[3] = {
                    QPointF(cx - 4, cy + 1), QPointF(cx + 4, cy + 1), QPointF(cx, cy + 5) };
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor(kTagSort));
                painter->drawPolygon(up, 3);
                painter->drawPolygon(down, 3);
                painter->restore();
            }
        };

    }

    mPostTagPanel::mPostTagPanel(QWidget* parent)
        : mPanelBase(parent)
        , _preRend(nullptr)
        , _postRend(nullptr)
        , _isCurrPostViewport(true)
        , _isCurrStateValid(false)
    {
        setupUI();

        //折叠区注册到基类统一管理
        m_foldWidgets.append(_manageFold);
        m_foldWidgets.append(_toolFold);
        m_foldWidgets.append(_advFold);

        connect(_pickEntry, &QPushButton::clicked, this, &mPostTagPanel::slot_pickEntryClicked);
        connect(_btnCancelEdit, &QPushButton::clicked, this, &mPostTagPanel::slot_cancelEditClicked);
        connect(_btnApplyEdit, &QPushButton::clicked, this, &mPostTagPanel::slot_applyEditClicked);
        connect(_btnInsertField, &QPushButton::clicked, this, &mPostTagPanel::slot_insertFieldClicked);

        //即时保存（勾选态）：面板编辑（内容/颜色/字号/精度/对齐/边框）变化即时应用到
        //列表选中标签——未勾选或无选中时 instantApplyToSelection 内部直接返回
        connect(_contentEdit, &QPlainTextEdit::textChanged,
            this, &mPostTagPanel::instantApplyToSelection);
        connect(_fontSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mPostTagPanel::instantApplyToSelection);
        connect(_precisionSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mPostTagPanel::instantApplyToSelection);
        connect(_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mPostTagPanel::instantApplyToSelection);
        connect(_alignChoice, &MxFunctions::mChoiceGroup::currentIndexChanged,
            this, &mPostTagPanel::instantApplyToSelection);
        connect(_checkBorder, &QCheckBox::toggled, this, &mPostTagPanel::instantApplyToSelection);
        connect(_borderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mPostTagPanel::instantApplyToSelection);

        connect(_methodChoice, &MxFunctions::mChoiceGroup::currentIndexChanged,
            this, &mPostTagPanel::slot_attachMethodChanged);
        connect(_objectChoice, &MxFunctions::mChoiceGroup::currentIndexChanged,
            this, &mPostTagPanel::slot_attachObjectChanged);

        //管理列表：复选联动/显隐切换/颜色取色
        connect(_table, &QTableWidget::itemChanged, this, &mPostTagPanel::slot_itemChanged);
        connect(_table, &QTableWidget::cellClicked, this, [this](int row, int col) {
            if (col == 3)
            {
                QTableWidgetItem* nameIt = _table->item(row, 1);
                const int di = nameIt ? dataIndexById(nameIt->data(Qt::UserRole).toInt()) : -1;
                if (di < 0) return;
                _records[di].visible = !_records[di].visible;
                _table->setSortingEnabled(false);
                renderRow(row);
                _table->setSortingEnabled(true);
                _table->horizontalHeader()->setSortIndicatorShown(false);
                //同步渲染层单标签显隐
                if (auto* tool = annotationLabelTool())
                    tool->showOrHideAnnotationLabel({ _records.at(di).name }, _records.at(di).visible);
            }
            else if (col == 4)
            {
                slot_showColorPopup(row);
            }
            });
        connect(_btnSelectAll, &QPushButton::clicked, this, &mPostTagPanel::slot_selectAllClicked);
        connect(_btnShow, &QPushButton::clicked, this, &mPostTagPanel::slot_showSelectedClicked);
        connect(_btnHide, &QPushButton::clicked, this, &mPostTagPanel::slot_hideSelectedClicked);
        connect(_btnDelete, &QPushButton::clicked, this, &mPostTagPanel::slot_deleteButton);
        connect(_btnNew, &QPushButton::clicked, this, &mPostTagPanel::slot_newButton);
        connect(_searchEdit, &QLineEdit::textChanged, this, &mPostTagPanel::slot_searchChanged);
        connect(_typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mPostTagPanel::slot_typeFilterChanged);

        //名称行色块按压取色（eventFilter 消费按压防误触）
        _nameColorSwatch->installEventFilter(this);

        //高级设置联动
        connect(_checkBorder, &QCheckBox::stateChanged, this, &mPostTagPanel::slot_borderEnabledChanged);

        //锚定到屏幕勾选即时切换列表选中标签的锚定空间（引擎专用接口，切换瞬间
        //冻结屏幕位置无跳变）；无选中/占位条目不动作——创建与"保存修改"经样式
        //整包（currentLabelStyle().screenAnchored）同字段生效
        connect(_checkAnchor, &QCheckBox::toggled, this, [this](bool checked) {
            const int di = selectedRecordIndex();
            if (di < 0 || _records.at(di).pending)
                return;
            if (auto* tool = annotationLabelTool())
                tool->setAnnotationLabelScreenAnchored(_records.at(di).name, checked);
            });

        //底色勾选实时作用于全部已有标签（全局显示设置；自动隐藏/显示名称为记录态，后者在创建时拼入文字）
        connect(_checkFill, &QCheckBox::stateChanged, this, [this](int state) {
            if (auto* tool = annotationLabelTool())
            {
                const bool hasBack = state == Qt::Checked;
                for (const QString& n : tool->getAnnotationLabelNames())
                {
                    mxr::AnnotationLabelStyle style;
                    if (tool->getAnnotationLabelStyle(n, style))
                    {
                        style.hasBack = hasBack;
                        tool->updateAnnotationLabelStyle(n, style);
                    }
                }
            }
            });

        //拾取行清空已并入拾取入口交互（再次点击退出拾取即清态）

        //Esc 取消拾取（面板隐藏时快捷键自动失效）
        QShortcut* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
        escShortcut->setContext(Qt::WindowShortcut);
        connect(escShortcut, &QShortcut::activated, this, &mPostTagPanel::exitPickMode);

        //定义折叠组收起即退出拾取（对齐原型互斥模式）
        connect(_toolFold, &FoldSection::foldExpandChanged, this, [this](bool expand) {
            if (!expand) exitPickMode();
            });

        //切换界面语言（随全局语言信号刷新）
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
            this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

        //换帧跟随（标注面板同款信号）：动画播放/云图切帧刷新含 {{字段}} 标签的文字
        connect(mMechGlobalSignals::getInstance(), SIGNAL(postCloudFrameChanged(int)),
            this, SLOT(slot_frameChanged(int)));

        refreshEmptyState();
        syncManagerState();

        this->setMinimumWidth(319);	//原型侧栏宽 319（annotationParameters 面板宽，与测量面板同族）
    }

    mPostTagPanel::~mPostTagPanel()
    {
        if (_pickConn)
        {
            disconnect(_pickConn);
        }
    }

    void mPostTagPanel::setupUI()
    {
        _mainLayout = new QVBoxLayout(this);
        //原型 parameter-panel：内容左右 4、顶部 6（与测量面板同款）
        _mainLayout->setContentsMargins(4, 6, 4, 0);

        /********************标签数据列表********************/
        _manageFold = new FoldSection(QString::fromUtf8("标签数据列表"), this);
        //跨列行几何=表单边距（同测量面板管理组口径）：边距必须保持 0，底边距 4 补组间距
        if (auto* form = _manageFold->layout())
        {
            form->setContentsMargins(0, 10, 0, 4);
        }

        //筛选行：类型筛选 88 + 名称搜索占满（间距 10 同测量面板实测）
        QWidget* filterRow = new QWidget(this);
        QHBoxLayout* filterLayout = new QHBoxLayout(filterRow);
        filterLayout->setContentsMargins(0, 0, 0, 0);
        filterLayout->setSpacing(10);
        _typeFilterCombo = new QComboBox(filterRow);
        _typeFilterCombo->setObjectName("mTagTypeFilter");
        _typeFilterCombo->setFixedHeight(kControl);
        _typeFilterCombo->setFixedWidth(kFilterW);
        filterLayout->addWidget(_typeFilterCombo);
        _searchEdit = new QLineEdit(filterRow);
        _searchEdit->setObjectName("mTagSearchEdit");
        _searchEdit->setClearButtonEnabled(true);
        _searchEdit->setFixedHeight(kControl);
        filterLayout->addWidget(_searchEdit, 1);
        _manageFold->addRow(filterRow);

        //记录列表（复选/名称/类型/显隐/颜色；共享管理表格件 mPanelTable：样式/行高/
        //表头/网格线与颜色格、眼睛委托统一（UiPalette/UiMetrics 令牌归一），空态覆盖层内置；
        //2026-11-16 收编说明：原 #E9E9E9/#1A1A1A/#D0D0D0 表头专值与深灰眼睛、直角无描边
        //色块随之归一到共享口径——差异见 doc/管理表格共享控件替换差异表-20261116.md）
        _table = new MxFunctions::mPanelTable(0, 5, this);
        _table->setObjectName("mTagTable");
        //表头换 SortArrowHeader（名称列叠画 ⇅ 箭头）：须在下方表头配置前替换，
        //后续 setSectionResizeMode/列宽等作用于新实例
        _table->setHorizontalHeader(new SortArrowHeader(_table));
        _table->setGridMode(MxFunctions::mPanelTable::GridMode::GridLines);
        _table->setEyeColumn(3);
        _table->setColorColumn(4);
        _table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        _table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        _table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        _table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        _table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
        //列宽（2026-11-12 参考图2比例 名称42/类型25/显隐16/颜色16）：复选 26 /
        //类型 72 / 显隐 45 / 颜色 45 固定，名称 Stretch 吃剩余（319 面板下约 119≈42%）
        _table->setColumnWidth(0, 26);
        _table->setColumnWidth(2, 72);
        _table->setColumnWidth(3, 45);
        _table->setColumnWidth(4, 45);
        _table->setSelectionBehavior(QAbstractItemView::SelectRows);
        _table->setSelectionMode(QAbstractItemView::SingleSelection);
        _table->horizontalHeader()->setSortIndicatorShown(false);

        QWidget* tableHost = new QWidget(this);
        QVBoxLayout* tableLay = new QVBoxLayout(tableHost);
        tableLay->setContentsMargins(0, 0, 0, 0);
        tableLay->addWidget(_table);
        tableHost->setFixedHeight(184);	//M2：表头 26 + 最小六行 26×6 + 边框 2
        _manageFold->addRow(tableHost);

        //表尾：[全选][已选 N 项]…[显示][隐藏][删除（红字）]
        QWidget* footerRow = new QWidget(this);
        QHBoxLayout* footerLayout = new QHBoxLayout(footerRow);
        footerLayout->setContentsMargins(0, 0, 0, 0);
        footerLayout->setSpacing(6);
        _btnSelectAll = new QPushButton(footerRow);
        _btnSelectAll->setStyleSheet(kFooterBtnQss);
        _btnSelectAll->setFixedSize(40, kControl);	//全局 QSS width:80 会撑宽，钉死 40
        _btnSelectAll->setEnabled(false);
        footerLayout->addWidget(_btnSelectAll);
        //已选计数（原型 note-selected-count：紧随全选钮自然宽，flex 由其后弹簧承担）
        _managerStatusLabel = new QLabel(footerRow);
        _managerStatusLabel->setStyleSheet(
            QString("QLabel{color:%1;font-size:11px;border:0;background:transparent;}").arg(kMuted));
        footerLayout->addWidget(_managerStatusLabel);
        footerLayout->addSpacing(2);
        footerLayout->addStretch();
        _btnShow = new QPushButton(footerRow);
        _btnShow->setStyleSheet("QPushButton{padding:0 6px;}");
        _btnShow->setFixedSize(40, kControl);
        _btnShow->setEnabled(false);
        footerLayout->addWidget(_btnShow);
        _btnHide = new QPushButton(footerRow);
        _btnHide->setStyleSheet("QPushButton{padding:0 6px;}");
        _btnHide->setFixedSize(40, kControl);
        _btnHide->setEnabled(false);
        footerLayout->addWidget(_btnHide);
        _btnDelete = new QPushButton(footerRow);
        _btnDelete->setStyleSheet(
            QString("QPushButton{padding:0 6px;}QPushButton:enabled{color:%1;}").arg(kDanger));
        _btnDelete->setFixedSize(40, kControl);
        _btnDelete->setEnabled(false);
        footerLayout->addWidget(_btnDelete);
        //新建（2026-11-13 用户参考图：最右侧蓝实心主操作钮）——点击新增
        //标签_N 占位条目并回填标签定义区名称，编辑后经"保存修改"落地视口
        _btnNew = new QPushButton(footerRow);
        _btnNew->setObjectName("mTagNewBtn");
        _btnNew->setStyleSheet(kPrimaryBtnQss);
        _btnNew->setFixedSize(40, kControl);
        footerLayout->addWidget(_btnNew);
        _manageFold->addRow(footerRow);

        //全部标签显示设置：三等分复选（原型 measurement-list-options 三列网格）
        QWidget* optionsRow = new QWidget(this);
        QHBoxLayout* optionsLayout = new QHBoxLayout(optionsRow);
        optionsLayout->setContentsMargins(0, 0, 0, 2);	//末行控件底边框余量（高分屏缩放裁边防御）
        optionsLayout->setSpacing(6);
        _checkAutoHide = new QCheckBox(optionsRow);
        _checkFill = new QCheckBox(optionsRow);
        _checkFill->setChecked(true);      //默认白底（参考图形态：白底+细边框）
        _checkShowName = new QCheckBox(optionsRow);
        for (QCheckBox* chk : { _checkAutoHide, _checkFill, _checkShowName })
        {
            optionsLayout->addWidget(chk, 1);
        }
        _manageFold->addRow(optionsRow);

        _mainLayout->addWidget(_manageFold);

        /********************标签定义********************/
        _toolFold = new FoldSection(QString::fromUtf8("标签定义"), this);
        if (auto* form = _toolFold->layout())
        {
            form->setContentsMargins(8, 6, 8, 0);
        }

        //标签名称 + 颜色（measurement-name-color：输入框占满 + 26×26 色块，间距 6）
        _nameEdit = new QLineEdit(this);
        _nameEdit->setObjectName("mTagNameEdit");
        _nameEdit->setFixedHeight(kControl);
        _nameEdit->setText(QString::fromUtf8("Note 1"));	//原型 value="Note 1"
        _nameColorSwatch = new QPushButton(this);
        _nameColorSwatch->setObjectName("mTagNameColor");
        _nameColorSwatch->setFixedSize(kSwatch, kSwatch);
        _nameColorSwatch->setCursor(Qt::PointingHandCursor);
        _nameColorSwatch->setToolTip(QString::fromUtf8("标签文字颜色"));
        applySwatchColor(_nameColorSwatch, QColor(0, 0, 0));	//原型默认 #000000
        _nameColorSwatch->setProperty("color", QColor(0, 0, 0));
        QWidget* nameRow = new QWidget(this);
        QHBoxLayout* nameLay = new QHBoxLayout(nameRow);
        nameLay->setContentsMargins(0, 0, 0, 0);
        nameLay->setSpacing(6);
        nameLay->addWidget(_nameEdit, 1);
        nameLay->addWidget(_nameColorSwatch);
        _nameRowLabel = new PanelLabel(QString::fromUtf8("标签名称"), this);
        _toolFold->addRow(_nameRowLabel, nameRow);

        //数据字段 + 插入（note-field-insert-pair：下拉占满 + ＋26×26）
        _fieldCombo = new QComboBox(this);
        _fieldCombo->setObjectName("mTagFieldCombo");
        _fieldCombo->setFixedHeight(kControl);
        for (int i = 0; i < kFieldCount; ++i)
            _fieldCombo->addItem(QString::fromUtf8(kFieldDefs[i].cn));
        _btnInsertField = new QPushButton(this);
        _btnInsertField->setObjectName("mTagInsertField");
        _btnInsertField->setFixedSize(kControl, kControl);
        _btnInsertField->setCursor(Qt::PointingHandCursor);
        _btnInsertField->setText(QString::fromUtf8("＋"));
        _btnInsertField->setToolTip(QString::fromUtf8("插入数据字段"));
        _btnInsertField->setStyleSheet(
            "QPushButton{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:2px;"
            "color:#424B57;padding:0;}"
            "QPushButton:hover{background:#F0F7FF;}");
        QWidget* fieldRow = new QWidget(this);
        QHBoxLayout* fieldLay = new QHBoxLayout(fieldRow);
        fieldLay->setContentsMargins(0, 0, 0, 0);
        fieldLay->setSpacing(6);
        fieldLay->addWidget(_fieldCombo, 1);
        fieldLay->addWidget(_btnInsertField);
        _fieldRowLabel = new PanelLabel(QString::fromUtf8("数据字段"), this);
        _toolFold->addRow(_fieldRowLabel, fieldRow);

        //标签内容（多行 4 行 ≈ 88px 高，原型 textarea rows=4）
        _contentEdit = new QPlainTextEdit(this);
        _contentEdit->setObjectName("mTagContentEdit");
        _contentEdit->setFixedHeight(88);
        _contentEdit->setStyleSheet(
            "QPlainTextEdit{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:2px;"
            "color:#424B57;padding:2px 4px;}");
        _contentRowLabel = new PanelLabel(QString::fromUtf8("标签内容"), this);
        _toolFold->addRow(_contentRowLabel, _contentEdit);

        //附着方式 / 附着对象（三选共框组：RebuildFunctions mChoiceGroup，
        //原型 mirror-planes 圆点+文字等分连体）
        _methodChoice = new MxFunctions::mChoiceGroup(this);
        _methodChoice->setObjectName("mTagMethodChoice");
        _methodRowLabel = new PanelLabel(QString::fromUtf8("附着方式"), this);
        _toolFold->addRow(_methodRowLabel, _methodChoice);

        _objectChoice = new MxFunctions::mChoiceGroup(this);
        _objectChoice->setObjectName("mTagObjectChoice");
        _objectRowLabel = new PanelLabel(QString::fromUtf8("附着对象"), this);
        _toolFold->addRow(_objectRowLabel, _objectChoice);

        //拾取对象（图2样式：常规描边"请选择"钮占满整行，可勾选进入/退出拾取态，
        //无侧边竖条与叠放小钮）
        _pickEntry = new QPushButton(this);
        _pickEntry->setObjectName("mTagPickEntry");
        _pickEntry->setCheckable(true);
        _pickEntry->setFixedHeight(kControl);
        _pickEntry->setCursor(Qt::PointingHandCursor);
        _pickEntry->setStyleSheet(
            QString("QPushButton{background:#FFFFFF;border:1px solid %1;border-radius:2px;"
                "color:%2;text-align:left;padding-left:12px;padding-right:12px;}"
                "QPushButton:checked{background:%3;border-color:%4;}"
                "QPushButton:disabled{background:#F5F7FA;border-color:%1;color:%5;}")
            .arg(kLineBorder, kText, kSelBg, kPrimary, kDisabled));
        _pickRowHost = _pickEntry;	//行级显隐（附着方式=窗口/坐标时整行摘除）定位用
        _pickRowLabel = new PanelLabel(QString::fromUtf8("拾取对象"), this);
        _toolFold->addRow(_pickRowLabel, _pickEntry);

        //窗口定位条件行（附着方式=窗口时可见）
        _windowAnchorCombo = new QComboBox(this);
        _windowAnchorCombo->setObjectName("mTagWindowAnchor");
        _windowAnchorCombo->setFixedHeight(kControl);
        for (const char* a : { "左上", "上中", "右上", "左中", "居中", "右中", "左下", "下中", "右下" })
            _windowAnchorCombo->addItem(QString::fromUtf8(a));
        _windowAnchorLabel = new PanelLabel(QString::fromUtf8("窗口定位"), this);
        _toolFold->addRow(_windowAnchorLabel, _windowAnchorCombo);
        _windowXSpin = new QSpinBox(this);
        _windowXSpin->setObjectName("mTagWindowX");
        _windowXSpin->setFixedHeight(kControl);
        _windowXSpin->setRange(0, 100);
        _windowXSpin->setValue(10);	//默认 10%（2026-11-13 用户指定）
        _windowXSpin->setSuffix(QString("%"));
        _windowXLabel = new PanelLabel(QString::fromUtf8("水平位置"), this);
        _toolFold->addRow(_windowXLabel, _windowXSpin);
        _windowYSpin = new QSpinBox(this);
        _windowYSpin->setObjectName("mTagWindowY");
        _windowYSpin->setFixedHeight(kControl);
        _windowYSpin->setRange(0, 100);
        _windowYSpin->setValue(10);	//默认 10%（2026-11-13 用户指定）
        _windowYSpin->setSuffix(QString("%"));
        _windowYLabel = new PanelLabel(QString::fromUtf8("垂直位置"), this);
        _toolFold->addRow(_windowYLabel, _windowYSpin);

        //坐标条件行（附着方式=坐标时可见）
        _systemCombo = new QComboBox(this);
        _systemCombo->setObjectName("mTagSystem");
        _systemCombo->setFixedHeight(kControl);
        _systemCombo->addItem(QString::fromUtf8("全局坐标系"));
        _systemLabel = new PanelLabel(QString::fromUtf8("坐标系"), this);
        _toolFold->addRow(_systemLabel, _systemCombo);
        _coordXSpin = new QDoubleSpinBox(this);
        _coordXSpin->setObjectName("mTagCoordX");
        _coordXSpin->setFixedHeight(kControl);
        _coordXSpin->setDecimals(6);
        _coordXSpin->setRange(-1e12, 1e12);
        _coordXLabel = new PanelLabel(QString("X"), this);
        _toolFold->addRow(_coordXLabel, _coordXSpin);
        _coordYSpin = new QDoubleSpinBox(this);
        _coordYSpin->setObjectName("mTagCoordY");
        _coordYSpin->setFixedHeight(kControl);
        _coordYSpin->setDecimals(6);
        _coordYSpin->setRange(-1e12, 1e12);
        _coordYLabel = new PanelLabel(QString("Y"), this);
        _toolFold->addRow(_coordYLabel, _coordYSpin);
        _coordZSpin = new QDoubleSpinBox(this);
        _coordZSpin->setObjectName("mTagCoordZ");
        _coordZSpin->setFixedHeight(kControl);
        _coordZSpin->setDecimals(6);
        _coordZSpin->setRange(-1e12, 1e12);
        _coordZLabel = new PanelLabel(QString("Z"), this);
        _toolFold->addRow(_coordZLabel, _coordZSpin);

        //即时保存勾选行（2026-11-13 用户参考图2：复选框位于按钮行上方，左对齐，
        //默认不勾选）——勾选=面板编辑即时应用到列表选中标签的文字+文字框+背景；
        //创建标签统一走"保存修改"（无选中记录=按当前输入创建）
        QWidget* instantRow = new QWidget(this);
        QHBoxLayout* instantLayout = new QHBoxLayout(instantRow);
        instantLayout->setContentsMargins(0, 0, 0, 0);
        _checkInstantSave = new QCheckBox(instantRow);
        _checkInstantSave->setObjectName("mTagInstantSaveCheck");
        instantLayout->addWidget(_checkInstantSave);
        instantLayout->addStretch();
        _toolFold->addRow(instantRow);

        //操作行（图2：取消编辑+保存修改两钮等宽排布）——取消编辑=表单回退到
        //列表选中标签的已存值；保存修改=应用到选中标签的文字+文字框+背景，
        //无选中=创建新标签（转 slot_createButton）
        QWidget* createRow = new QWidget(this);
        QHBoxLayout* createLayout = new QHBoxLayout(createRow);
        createLayout->setContentsMargins(0, 0, 0, 0);
        createLayout->setSpacing(6);
        _btnCancelEdit = new QPushButton(createRow);
        _btnCancelEdit->setObjectName("mTagCancelEditBtn");
        _btnCancelEdit->setStyleSheet(kSecondaryBtnQss);
        _btnCancelEdit->setFixedHeight(kControl);
        createLayout->addWidget(_btnCancelEdit, 1);
        _btnApplyEdit = new QPushButton(createRow);
        _btnApplyEdit->setObjectName("mTagApplyEditBtn");
        _btnApplyEdit->setStyleSheet(kPrimaryBtnQss);	//主操作蓝底
        _btnApplyEdit->setFixedHeight(kControl);
        createLayout->addWidget(_btnApplyEdit, 1);
        _toolFold->addRow(createRow);

        //状态行（note-status；空文案整行收起，行级摘/插同步折叠高度帽）
        _statusLabel = new QLabel(this);
        _statusLabel->setStyleSheet(
            QString("QLabel{color:%1;font-size:11px;border:0;background:transparent;}").arg(kMuted));
        _statusLabel->setVisible(false);
        _toolFold->addRow(_statusLabel);

        //显示样式细线框（annotation-measurementDisplayStyleWire：骑线标题点击折叠，
        //RebuildFunctions mGroupBox 空图标=无勾选指示；标题色经 API 设，不走 QSS）
        _styleBox = new MxFunctions::mGroupBox(QString::fromUtf8("显示样式"), QPixmap(), QPixmap(), this);
        _styleBox->setObjectName("mTagStyleBox");
        _styleBox->setTitleColor(QColor(kText));
        QGridLayout* styleGrid = _styleBox->grid();
        styleGrid->setHorizontalSpacing(6);
        styleGrid->setVerticalSpacing(6);
        _fontStyleLabel = new PanelLabel(QString::fromUtf8("字号字体"), _styleBox->contentWidget());
        _fontSizeCombo = new QComboBox(_styleBox->contentWidget());
        _fontSizeCombo->setFixedHeight(kControl);
        for (const char* s : { "12", "14", "16", "18", "20", "24", "32" })
            _fontSizeCombo->addItem(QString::fromLatin1(s));
        _fontFamilyCombo = new QComboBox(_styleBox->contentWidget());
        _fontFamilyCombo->setFixedHeight(kControl);
        for (const char* f : { "宋体", "微软雅黑", "Arial" })
            _fontFamilyCombo->addItem(QString::fromUtf8(f));
        styleGrid->addWidget(_fontStyleLabel, 0, 0);
        styleGrid->addWidget(makeJoinedPair(_fontSizeCombo, _fontFamilyCombo), 0, 1);
        _precisionStyleLabel = new PanelLabel(QString::fromUtf8("数值精度"), _styleBox->contentWidget());
        _precisionSpin = new QSpinBox(_styleBox->contentWidget());
        _precisionSpin->setFixedHeight(kControl);
        _precisionSpin->setRange(0, 15);
        _precisionSpin->setValue(3);	//原型默认 3
        _formatCombo = new QComboBox(_styleBox->contentWidget());
        _formatCombo->setFixedHeight(kControl);
        for (const char* fmt : { "科学计数", "小数表示", "通用" })
            _formatCombo->addItem(QString::fromUtf8(fmt));
        styleGrid->addWidget(_precisionStyleLabel, 1, 0);
        styleGrid->addWidget(makeJoinedPair(_precisionSpin, _formatCombo), 1, 1);
        _alignStyleLabel = new PanelLabel(QString::fromUtf8("文字对齐"), _styleBox->contentWidget());
        _alignChoice = new MxFunctions::mChoiceGroup(_styleBox->contentWidget());
        _alignChoice->setObjectName("mTagAlignChoice");
        styleGrid->addWidget(_alignStyleLabel, 2, 0);
        styleGrid->addWidget(_alignChoice, 2, 1);
        //显示样式细线框跨整行（原型 fieldset 占 grid-column 1/-1，内部标签列与
        //外层表单标签列同线）：addRow(widget, nullptr) 走 QFormLayout 单部件跨列分支
        _styleBox->grid()->setContentsMargins(0, 2, 2, 2);
        _toolFold->addRow(static_cast<QWidget*>(_styleBox), static_cast<QWidget*>(nullptr));
        _toolFold->refreshLayout();

        //窗口/坐标条件行初始隐藏（原型默认附着方式=实体）
        const QList<QWidget*> conditionalRows = { _windowAnchorCombo, _windowXSpin, _windowYSpin,
            _systemCombo, _coordXSpin, _coordYSpin, _coordZSpin };
        for (QWidget* w : conditionalRows)
        {
            _toolFold->setRowVisible(w, false);
        }

        _mainLayout->addWidget(_toolFold);

        /********************高级设置（默认收起）********************/
        _advFold = new FoldSection(QString::fromUtf8("高级设置"), this);
        _advFold->setExpanded(false);
        if (auto* form = _advFold->layout())
        {
            form->setContentsMargins(8, 6, 8, 4);
        }

        //显示边框 + 边框样式（勾选联动可用性）
        QWidget* borderRow = new QWidget(this);
        QHBoxLayout* borderLay = new QHBoxLayout(borderRow);
        borderLay->setContentsMargins(0, 0, 0, 0);
        borderLay->setSpacing(6);
        _checkBorder = new QCheckBox(borderRow);
        _checkBorder->setObjectName("mTagBorderEnabled");
        _checkBorder->setChecked(true);    //默认显示外框（参考图形态：白底+细边框）
        _borderCombo = new QComboBox(borderRow);
        _borderCombo->setObjectName("mTagBorderStyle");
        _borderCombo->setFixedHeight(kControl);
        for (const char* b : { "无", "实线", "虚线" })
            _borderCombo->addItem(QString::fromUtf8(b));
        _borderCombo->setCurrentIndex(1);  //默认实线
        _borderCombo->setEnabled(true);
        borderLay->addWidget(_checkBorder);
        borderLay->addWidget(_borderCombo, 1);
        _advFold->addRow(borderRow);

        //跟随对象移动 / 锚定到屏幕（整行复选）
        _checkFollow = new QCheckBox(this);
        _checkFollow->setObjectName("mTagFollow");
        _advFold->addRow(_checkFollow);
        _checkAnchor = new QCheckBox(this);
        _checkAnchor->setObjectName("mTagAnchor");
        //默认不勾选=锚定世界（跟随视图旋转/平移/缩放）；勾选=文字+框+背景相对
        //屏幕不动（2026-11-13 用户需求：不设为默认勾选）
        _advFold->addRow(_checkAnchor);

        //排列方式（原型恒禁用——待接入）
        _arrangeCombo = new QComboBox(this);
        _arrangeCombo->setObjectName("mTagArrange");
        _arrangeCombo->setFixedHeight(kControl);
        _arrangeCombo->setEnabled(false);
        for (const char* a : { "手动定位", "实体位置", "模型外侧排列" })
            _arrangeCombo->addItem(QString::fromUtf8(a));
        _arrangeLabel = new PanelLabel(QString::fromUtf8("排列方式"), this);
        _advFold->addRow(_arrangeLabel, _arrangeCombo);

        _mainLayout->addWidget(_advFold);

        _mainLayout->addStretch();

        //原型行距（控件 26 + 间距 6）：实例级覆写表单行距，不改共享 FoldSection 默认值；
        //标签垂直居中对齐原型（标签 y 比控件行顶 +4：84 标签列在 26 行内居中）
        for (FoldSection* fold : { _manageFold, _toolFold, _advFold })
        {
            if (auto* form = fold->findChild<QFormLayout*>())
            {
                form->setVerticalSpacing(6);
                form->setLabelAlignment(Qt::AlignVCenter);
            }
        }
    }

    QWidget* mPostTagPanel::makeJoinedPair(QWidget* first, QWidget* second)
    {
        //连体对（measurement-pair）：容器描边圆角、内部两控件零间距、格间竖线、
        //子控件去掉自身描边（QComboBox/QSpinBox 全局 QSS 有边框，此处必须覆盖）
        QWidget* pair = new QWidget(this);
        pair->setAttribute(Qt::WA_StyledBackground, true);
        QHBoxLayout* lay = new QHBoxLayout(pair);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);
        first->setParent(pair);
        second->setParent(pair);
        lay->addWidget(first, 1);
        lay->addWidget(second, 1);
        first->setStyleSheet(
            "QComboBox,QSpinBox{border:0;border-radius:0;background:#FFFFFF;}"
            "QAbstractSpinBox::up-button,QAbstractSpinBox::down-button{border:0;}");
        second->setStyleSheet(
            "QComboBox,QSpinBox{border:0;border-radius:0;background:#FFFFFF;border-left:1px solid #DCE2E9;}"
            "QAbstractSpinBox::up-button,QAbstractSpinBox::down-button{border:0;}");
        pair->setStyleSheet(
            "QWidget{border:1px solid #DCE2E9;border-radius:2px;background:#FFFFFF;}");
        return pair;
    }

    void mPostTagPanel::newTagPanel(int caseId)
    {
        _isCurrStateValid = false;
        _caseId = caseId;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
            return;

        _isCurrPostViewport = (viewportPrivate->getViewportType() == ViewportPost3D);

        if (_isCurrPostViewport)
        {
            mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
            if (postRend == nullptr)
                return;
            _postRend = postRend;
            //节点坐标查询用的后处理数据库（与量测工具同款取法）
            _postDB = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post)
                .getPostDatabase(postRend->getPostDataName());
        }
        else
        {
            _preRend = getCVM->getPreRenderWidget();
            if (!_preRend)
                return;
        }

        //上下文切换后旧拾取对象失效，清空并退出拾取态
        _pickedIds.clear();
        refreshPickEntryText();

        _isCurrStateValid = true;

        //进入/重建面板上下文即激活注释标签工具（创建/拖拽/双击编辑的事件路由依赖活动工具）
        activateAnnotationLabelTool();

        exitPickMode();
    }

    QVector<QPair<int, QString>> mPostTagPanel::tagTreeRecords() const
    {
        //树"标签"分支重建源：phyID=记录 id，节点名=记录名（占位条目同原态上树，
        //无"（待定义）"后缀——后缀仅管理表名称列展示，与原型树口径一致）
        QVector<QPair<int, QString>> records;
        records.reserve(_records.size());
        for (const TagRecord& rec : _records)
            records.push_back({ rec.id, rec.name });
        return records;
    }

    void mPostTagPanel::selectRecord(int recordId)
    {
        //树"标签"子节点点击路由（原型 #annotationParameters：树节点点击=管理表
        //行点击同一语义）——清搜索/类型筛选防目标行被过滤隐藏，选中行滚动可见，
        //表单回填该记录已存值（名称/内容/颜色，与"取消编辑"同款回填）
        const int di = dataIndexById(recordId);
        if (di < 0) return;

        _searchEdit->blockSignals(true);
        _searchEdit->clear();
        _searchEdit->blockSignals(false);
        _typeFilterCombo->blockSignals(true);
        _typeFilterCombo->setCurrentIndex(0);
        _typeFilterCombo->blockSignals(false);
        applyManagerFilter();

        for (int r = 0; r < _table->rowCount(); ++r)
        {
            QTableWidgetItem* nameIt = _table->item(r, 1);
            if (nameIt == nullptr || nameIt->data(Qt::UserRole).toInt() != recordId)
                continue;
            const int visualRow = _table->row(nameIt);
            if (visualRow >= 0 && !_table->isRowHidden(visualRow))
            {
                _table->selectRow(visualRow);
                _table->scrollToItem(_table->item(visualRow, 1));
            }
            break;
        }
        formFromRecord(_records.at(di));
    }

    void mPostTagPanel::notifyTreeRecordsChanged()
    {
        //记录结构变化（新建/转正/删除）→ 后处理树重建"标签"分支子节点
        MechCommon::mMechGlobalSignals::getInstance()->postTagRecordsChangedSig();
    }

    void mPostTagPanel::setPanelPickAndSignal(bool isshow)
    {
        if (!_isCurrStateValid) return;

        if (_isCurrPostViewport)
        {
            if (_postRend == nullptr) return;
            auto sc = _postRend->getSelectionCaculator();
            if (sc == nullptr) return;

            if (isshow)
            {
                if (_pickConn)
                    disconnect(_pickConn);
                _pickConn = connect(sc, &mxr::PostSelectionCalculator::signals_FinishPick,
                    this, &mPostTagPanel::slot_finishPick);
            }
            else
            {
                if (_pickConn)
                {
                    disconnect(_pickConn);
                    _pickConn = QMetaObject::Connection{};
                }
                _postRend->setPickFilter(mxr::PickFilter::PickNothing);
            }
        }
        else
        {
            if (_preRend == nullptr) return;
            auto sc = _preRend->getSelectionCaculator();
            if (sc == nullptr) return;

            if (isshow)
            {
                if (_pickConn)
                    disconnect(_pickConn);
                _pickConn = connect(sc, &mxr::SelectionCaculator::signals_FinishPick,
                    this, &mPostTagPanel::slot_finishPick);
            }
            else
            {
                if (_pickConn)
                {
                    disconnect(_pickConn);
                    _pickConn = QMetaObject::Connection{};
                }
                _preRend->setPickFilter(mxr::PickFilter::PickNothing);
            }
        }
    }

    void mPostTagPanel::hideEvent(QHideEvent* event)
    {
        //面板切走（共享 dock 切页/总复位）时退出拾取并断开拾取信号
        if (_isCurrStateValid)
        {
            exitPickMode();
            setPanelPickAndSignal(false);
        }
        mPanelBase::hideEvent(event);
    }

    void mPostTagPanel::showEvent(QShowEvent* event)
    {
        //进入标签面板即激活注释标签工具（面板常驻期间视口点击可落标签/拖拽/双击编辑）
        if (_isCurrStateValid)
            activateAnnotationLabelTool();
        mPanelBase::showEvent(event);
    }

    bool mPostTagPanel::eventFilter(QObject* watched, QEvent* event)
    {
        //名称行色块按压 → 打开取色器（消费按压防误触面板其他区域）
        if (watched == _nameColorSwatch && event->type() == QEvent::MouseButtonPress)
        {
            slot_nameColorClicked();
            return true;
        }
        return mPanelBase::eventFilter(watched, event);
    }

    void mPostTagPanel::exitPickMode()
    {
        //必须在取消勾选前捕获拾取态：循环之后 isChecked 恒为 false，
        //下面的状态行清空判断会永远走不到（测量面板实测 bug 同款防御）
        const bool wasPicking = _pickEntry && _pickEntry->isChecked();
        if (_pickEntry)
            _pickEntry->setChecked(false);
        mxr::mBaseRend* pickRend = nullptr;
        if (_isCurrPostViewport)
            pickRend = _postRend;
        else
            pickRend = _preRend;
        if (pickRend)
            pickRend->setPickFilter(mxr::PickFilter::PickNothing);
        refreshPickEntryText();
        //仅在按钮确实处于拾取态（被取消勾选）时清状态行：创建成功等路径先写结果
        //再调本函数，不能把刚写入的结果擦掉
        if (wasPicking)
            setStatusLabel(QString());
    }

    void mPostTagPanel::setStatusLabel(const QString& text)
    {
        if (!_statusLabel)
            return;
        _statusLabel->setText(text);
        //行级摘/插并同步 FoldSection 高度帽（直接 setVisible 会留旧帽不更新）
        _toolFold->setRowVisible(_statusLabel, !text.isEmpty());
    }

    void mPostTagPanel::updatePickStatus()
    {
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        if (_pickedIds.isEmpty())
        {
            setStatusLabel(QString());	//armed 空拾取时状态行为空（文案在按钮上）
            return;
        }
        QStringList ids;
        for (int id : _pickedIds)
            ids.append(QString::number(id));
        const QString objName = typeName(_objectChoice ? _objectChoice->currentIndex() : 0, lang);
        setStatusLabel(lang == 0
            ? QString::fromUtf8("已选%1：%2（可点击“保存修改”）").arg(objName).arg(ids.join(","))
            : QString("Picked %1: %2 (ready to create)").arg(objName).arg(ids.join(",")));
    }

    QString mPostTagPanel::pickEntryLabel(bool armed, int lang) const
    {
        const QString objName = typeName(_objectChoice ? _objectChoice->currentIndex() : 0, lang);
        if (armed)
            return lang == 0
                ? QString::fromUtf8("正在拾取%1…").arg(objName)
                : QString("Picking %1...").arg(objName);
        //常态文案对齐原型 r-pick-caption："请选择"
        return lang == 0 ? QString::fromUtf8("请选择") : QString("Select");
    }

    void mPostTagPanel::refreshPickEntryText()
    {
        if (!_pickEntry)
            return;
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        _pickEntry->setText(pickEntryLabel(_pickEntry->isChecked(), lang));
    }

    int mPostTagPanel::selectedRecordIndex() const
    {
        const int row = _table->currentRow();
        QTableWidgetItem* nameIt = row >= 0 ? _table->item(row, 1) : nullptr;
        return nameIt ? dataIndexById(nameIt->data(Qt::UserRole).toInt()) : -1;
    }

    mxr::AnnotationLabelStyle mPostTagPanel::currentLabelStyle() const
    {
        mxr::AnnotationLabelStyle style;
        const QColor nameColor = _nameColorSwatch->property("color").value<QColor>();
        //字号换算（2026-11-12 用户反馈 12 过小）：引擎 Common 通道字形的屏幕像素高
        //≈ size/2，面板字号按"屏幕像素"语义提交须 ×2；下拉 12~32 → 24~64，
        //落在引擎 6~200 钳制内（详见 slot_createButton 原注释）
        style.size = _fontSizeCombo->currentText().toInt() * 2;
        style.textColor = { float(nameColor.red()), float(nameColor.green()), float(nameColor.blue()) };
        style.hasFrame = _checkBorder->isChecked() && _borderCombo->currentIndex() > 0;  //"无"=不加框
        //外框线形（2026-11-13 接入引擎 AnnotationLabelFrameStyle）：下拉 无/实线/虚线
        //映射 Solid=0/Dashed=1（虚线段/空隔 6px/4px 固定屏幕像素，缩放下段长恒定）；
        //仅 hasFrame 时被绘制消费
        style.frameStyle = static_cast<mxr::AnnotationLabelFrameStyle>(
            _borderCombo ? qBound(0, _borderCombo->currentIndex() - 1, 1) : 0);
        style.frameColor = { 0, 0, 0 };
        style.frameWidth = 1.f;
        style.hasBack = _checkFill->isChecked();
        style.backColor = { 255, 255, 255 };
        style.leaderColor = { 0, 0, 0 };
        style.leaderWidth = 1.f;
        style.align = static_cast<mxr::AnnotationLabelTextAlign>(_alignChoice ? _alignChoice->currentIndex() : 0);
        //锚定空间：勾选"锚定到屏幕"=文字+框+背景相对屏幕不动（引擎切换瞬间冻结
        //屏幕位置无跳变）；不勾选=锚定世界跟随视图。创建时引擎按世界点投影冻结
        //屏幕位，样式整包更新时引擎自动做 capture/restore 换算（2026-11-13 接入）
        style.screenAnchored = _checkAnchor ? _checkAnchor->isChecked() : false;
        return style;
    }

    void mPostTagPanel::formFromRecord(const TagRecord& rec)
    {
        _nameEdit->setText(rec.name);
        _contentEdit->setPlainText(rec.content);
        if (rec.color.isValid())
        {
            _nameColorSwatch->setProperty("color", rec.color);
            applySwatchColor(_nameColorSwatch, rec.color);
        }
    }

    void mPostTagPanel::instantApplyToSelection()
    {
        //即时保存（勾选态）：面板编辑变化即时应用到列表选中标签——文字重解析
        //（{{字段}}/显示名称与"保存修改"同口径）+ 样式整包刷新，并同步记录
        //（编辑即落库，"取消编辑"自然无从回退，即时保存语义使然）。
        //未勾选/无选中不动作；内容清空途中不动作（防把视口文字即时清掉）
        if (!_checkInstantSave || !_checkInstantSave->isChecked())
            return;
        const int di = selectedRecordIndex();
        if (di < 0)
            return;
        if (_records.at(di).pending)
            return;		//占位条目渲染层尚无标签，即时应用无对象（经"保存修改"创建转正）
        const QString text = _contentEdit->toPlainText();
        if (text.trimmed().isEmpty())
            return;

        TagRecord& rec = _records[di];
        QString full = text;
        if (_checkShowName && _checkShowName->isChecked())
            full = rec.name + QStringLiteral("\n") + full;

        if (mxr::InteractionAnnotationLabelTool* tool = annotationLabelTool())
        {
            tool->setAnnotationLabelText(rec.name,
                resolveFields(full, rec.typeIndex, rec.anchorId));
            tool->updateAnnotationLabelStyle(rec.name, currentLabelStyle());
        }

        //面板记录同步（内容/颜色）并刷新列表行
        rec.content = text;
        rec.color = _nameColorSwatch->property("color").value<QColor>();
        if (!rec.color.isValid())
            rec.color = QColor(0, 0, 0);
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            QTableWidgetItem* nameIt = _table->item(r, 1);
            if (nameIt && nameIt->data(Qt::UserRole).toInt() == rec.id)
            {
                _table->setSortingEnabled(false);
                renderRow(r);
                _table->setSortingEnabled(true);
                _table->horizontalHeader()->setSortIndicatorShown(false);
                break;
            }
        }
    }

    int mPostTagPanel::appendRecordRow(const TagRecord& rec)
    {
        //按记录插入列表行（调用前记录须已入 _records——renderRow 按 id 反查）；
        //排序期间插入会错位，先关排序；新行滚动可见并选中
        QTableWidgetItem* nameIt = nullptr;
        {
            MxFunctions::mPanelTable::RowUpdateGuard guard(_table);
            const int r = _table->rowCount();
            _table->insertRow(r);
            _table->setItem(r, 0, MxFunctions::mPanelTable::makeCheckItem());
            nameIt = MxFunctions::mPanelTable::makeNameItem(QString(), rec.id);
            _table->setItem(r, 1, nameIt);
            _table->setItem(r, 2, MxFunctions::mPanelTable::makeTextItem(QString()));
            _table->setItem(r, 3, MxFunctions::mPanelTable::makeTextItem(QString()));
            _table->setItem(r, 4, MxFunctions::mPanelTable::makeTextItem(QString()));
            renderRow(r);
        }
        _table->horizontalHeader()->setSortIndicatorShown(false);

        //新行滚动可见并选中
        const int visualRow = _table->row(nameIt);
        if (visualRow >= 0 && !_table->isRowHidden(visualRow))
        {
            _table->selectRow(visualRow);
            _table->scrollToItem(_table->item(visualRow, 1));
        }
        return visualRow;
    }

    void mPostTagPanel::upsertRecordRow(const QString& labelName, int anchorId)
    {
        //创建路径记录写入（upsert）：同名记录（"新建"占位条目）转正——更新附着/
        //内容/颜色并清 pending（名称列"（待定义）"后缀随之去除），行重渲并保持
        //选中；无同名才按当前面板输入新增记录与列表行
        for (int i = 0; i < _records.count(); ++i)
        {
            if (_records.at(i).name != labelName)
                continue;
            TagRecord& rec = _records[i];
            rec.typeIndex = _objectChoice ? _objectChoice->currentIndex() : 0;
            rec.anchorId = anchorId;
            rec.attachMethod = _methodChoice ? _methodChoice->currentIndex() : 0;
            rec.content = _contentEdit->toPlainText();
            rec.color = _nameColorSwatch->property("color").value<QColor>();
            if (!rec.color.isValid())
                rec.color = QColor(0, 0, 0);
            rec.pending = false;
            for (int r = 0; r < _table->rowCount(); ++r)
            {
                QTableWidgetItem* nameIt = _table->item(r, 1);
                if (!nameIt || nameIt->data(Qt::UserRole).toInt() != rec.id)
                    continue;
                _table->setSortingEnabled(false);
                renderRow(r);
                _table->setSortingEnabled(true);
                _table->horizontalHeader()->setSortIndicatorShown(false);
                const int visualRow = _table->row(nameIt);
                if (visualRow >= 0 && !_table->isRowHidden(visualRow))
                {
                    _table->selectRow(visualRow);
                    _table->scrollToItem(_table->item(visualRow, 1));
                }
                break;
            }
            notifyTreeRecordsChanged();
            return;
        }

        TagRecord rec;
        rec.id = _nextId++;
        rec.name = labelName;
        rec.typeIndex = _objectChoice ? _objectChoice->currentIndex() : 0;
        rec.anchorId = anchorId;
        rec.attachMethod = _methodChoice ? _methodChoice->currentIndex() : 0;
        rec.color = _nameColorSwatch->property("color").value<QColor>();
        if (!rec.color.isValid())
            rec.color = QColor(0, 0, 0);
        rec.visible = true;
        rec.content = _contentEdit->toPlainText();
        _records.append(rec);
        appendRecordRow(rec);
        notifyTreeRecordsChanged();
    }

    void mPostTagPanel::currentPickDomain(mxr::PickFilter& filter, int& pickKind,
        mxr::SelectionInfoType& selType) const
    {
        //拾取域随附着对象联动（后处理：网格点/网格单元/网格部件；前处理：几何点）
        const int obj = _objectChoice ? _objectChoice->currentIndex() : 0;
        if (_isCurrPostViewport)
        {
            switch (obj)
            {
            case 1: filter = mxr::PickFilter::PickMeshElement; pickKind = 8;
                selType = mxr::SelectionInfoType::MeshElement; break;
            case 2: filter = mxr::PickFilter::PickMeshPart; pickKind = 9;
                selType = mxr::SelectionInfoType::MeshPart; break;
            default: filter = mxr::PickFilter::PickMeshNode; pickKind = 5;
                selType = mxr::SelectionInfoType::MeshNode; break;
            }
        }
        else
        {
            filter = mxr::PickFilter::PickGeoPoint;
            pickKind = 1;
            selType = mxr::SelectionInfoType::GeoPoint;
        }
    }

    bool mPostTagPanel::getPickResultIDs(mxr::SelectionInfoType selType, std::vector<int>& outAllIds)
    {
        outAllIds.clear();
        if (!_isCurrStateValid) return false;

        if (_isCurrPostViewport)
        {
            if (_postRend == nullptr) return false;
            mxr::SelectionDataContainer* selectionContainer = _postRend->getSelectionContainer();
            if (selectionContainer)
            {
                mxr::MeshSelectMemoryData* meshSelData = selectionContainer->getMeshSelectData();
                if (meshSelData)
                {
                    const std::vector<mxr::MeshSelectInfoData>& selectDatas =
                        meshSelData->getSelectDataByType(selType);
                    for (const auto& selInfo : selectDatas)
                    {
                        if (selType == mxr::SelectionInfoType::MeshPart)
                            outAllIds.push_back(selInfo.part_id);
                        else if (selType == mxr::SelectionInfoType::MeshElement)
                            outAllIds.push_back(selInfo.data_id.element_id);
                        else
                            outAllIds.push_back(selInfo.data_id.node_id);
                    }
                }
            }
        }
        else
        {
            if (_preRend == nullptr) return false;
            mxr::SelectionDataContainer* selectionContainer = _preRend->getSelectionContainer();
            if (selectionContainer)
            {
                mxr::mGeoSelectData* geoSelData = selectionContainer->getGeoSelectData();
                if (geoSelData)
                    outAllIds = geoSelData->getSelectDataByType(selType);
            }
        }
        return true;
    }

    mxr::InteractionAnnotationLabelTool* mPostTagPanel::annotationLabelTool()
    {
        //上下文悬空（视口切换/结果重载后 QPointer 失效）时先重取一次再判
        if (!_isCurrStateValid)
            newTagPanel(_caseId);
        if (!_isCurrStateValid)
            return nullptr;

        //与标注族面板（mPostArrow 等）同款路径：视口 widget -> InteractionSystem -> 工具管理器。
        //只取工具不激活——工具激活统一在进入面板时做（activateAnnotationLabelTool），
        //避免每次创建都切换活动工具
        if (_isCurrPostViewport)
        {
            auto* w = getCVM->getPostWidget();
            return (w && w->interaction())
                ? w->interaction()->getToolManager()
                    ->getTool<mxr::InteractionAnnotationLabelTool>(mxr::InteractionToolType::AnnotationLabel)
                : nullptr;
        }
        auto* w = getCVM->getPreViewportWidget();
        return (w && w->interaction())
            ? w->interaction()->getToolManager()
                ->getTool<mxr::InteractionAnnotationLabelTool>(mxr::InteractionToolType::AnnotationLabel)
            : nullptr;
    }

    void mPostTagPanel::activateAnnotationLabelTool()
    {
        //进入标签面板时激活注释标签工具：一击放置的落地点击与拖拽/双击编辑
        //的事件路由都依赖活动工具（激活同时装载工具的按键映射层）
        if (!_isCurrStateValid)
            newTagPanel(_caseId);
        if (!_isCurrStateValid)
            return;

        if (_isCurrPostViewport)
        {
            if (auto* w = getCVM->getPostWidget())
                if (w->interaction())
                    w->interaction()->getToolManager()->setActive(mxr::InteractionToolType::AnnotationLabel);
        }
        else
        {
            if (auto* w = getCVM->getPreViewportWidget())
                if (w->interaction())
                    w->interaction()->getToolManager()->setActive(mxr::InteractionToolType::AnnotationLabel);
        }
    }

    bool mPostTagPanel::nodeWorldPos(int nodeId, QVector3D& out)
    {
        out = QVector3D();
        if (!_isCurrStateValid || nodeId < 0)
            return false;

        if (_isCurrPostViewport)
        {
            if (_postRend == nullptr || _postDB == nullptr) return false;

            MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_postRend);
            if (guidata == nullptr) return false;

            PostData::mPostModelData* modeldata = _postDB->getModelData(guidata->_cloudFrame);
            if (modeldata == nullptr) return false;

            //当前帧坐标 + 位移（与量测工具一致，变形后锚点跟随当前帧）
            QHash<int, QVector3D> dis = _postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
            const int cId = modeldata->_nodedata.nodeidmap.value(nodeId, -1);
            out = modeldata->getVectorById(nodeId) + dis.value(cId);
            return true;
        }

        //前处理：几何引擎顶点属性
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData || !modelData->getGeoManager()) return false;

        auto points = modelData->getGeoEngineDataMgr()->engine().geoCalculate().CalculateVertexAttr({ nodeId });
        if (points.size() <= 0) return false;
        out = points[0].m_coor;
        return true;
    }

    bool mPostTagPanel::pickAnchorWorld(int domain, int id, QVector3D& out)
    {
        //节点域沿用既有链路
        if (domain == 0)
            return nodeWorldPos(id, out);
        if (!_isCurrStateValid || id < 0)
            return false;

        if (_isCurrPostViewport)
        {
            if (_postRend == nullptr || _postDB == nullptr) return false;
            MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_postRend);
            if (guidata == nullptr) return false;
            PostData::mPostModelData* modeldata = _postDB->getModelData(guidata->_cloudFrame);
            if (modeldata == nullptr) return false;
            //当前帧位移（与 nodeWorldPos/renderPosCid 同口径：连续 id 键）
            const QHash<int, QVector3D> dis = _postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);

            if (domain == 1)
            {
                //单元形心：全节点均值（含高阶中节点各计一次，与 mPost3DResult
                //单元结果标注同配方；getElementNodeId 返回连续 id）
                const int partId = modeldata->getPartIDByEleID(id);
                if (partId < 0) return false;
                const QList<int> nodeIds = modeldata->getElementNodeId(partId, id);
                if (nodeIds.isEmpty()) return false;
                QVector3D sum;
                for (int nid : nodeIds)
                    sum += modeldata->getVector(nid) + dis.value(nid);
                out = sum / float(nodeIds.size());
                return true;
            }

            //部件中心：部件全部不重复节点均值
            QPointer<PostData::mPostPartData> part = modeldata->getPartData(id);
            if (part == nullptr) return false;
            const tsl::robin_set<int>* nodes = part->getNodeIdSet();
            if (nodes == nullptr || nodes->empty()) return false;
            QVector3D sum;
            for (int nid : *nodes)
                sum += modeldata->getVector(nid) + dis.value(nid);
            out = sum / float(nodes->size());
            return true;
        }

        //前处理：部件=几何引擎形心；单元域几何引擎无对应接口（失败回退状态行提示）
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData || !modelData->getGeoManager() || domain != 2) return false;
        const QVector3D c = modelData->getGeoEngineDataMgr()->engine().geoCalculate().getCentroid(id);
        if (c.isNull()) return false;
        out = c;
        return true;
    }

    QString mPostTagPanel::resolveFields(const QString& raw, int domain, int anchorId)
    {
        //全字段解析（2026-11-13 用户需求：数据字段全部要解析，不再显示 {{...}} 占位）。
        //全局字段与标注面板 getNoteContents 同源：模型=postDB 名 / 工况=getFrameCase /
        //当前帧=getFrameNameByFrameNum / 仿真步=getFrameTime；对象三字段需实体锚：
        //编号=锚对象 id、坐标=锚点世界坐标（当前帧含位移）、名称=部件名（节点/单元
        //无命名回退 N/A）；结果数值=当前变量当前帧值（实体+节点锚，链路同云图
        //"显示值"）；取不到的字段写 N/A
        QString out = raw;
        if (!out.contains(QStringLiteral("{{")))
            return out;

        MechPreData::mPostGUIData* guidata = nullptr;
        if (_isCurrPostViewport && _isCurrStateValid && _postRend && _postDB)
            guidata = getCVM->getGUIDataByPostRender(_postRend);
        PostData::mPostDataBase* db = _postDB.data();
        const int frame = guidata ? guidata->_cloudFrame : -1;

        auto replaceField = [&out](const char* cn, const char* en, const QString& value)
        {
            out.replace(QStringLiteral("{{%1}}").arg(QString::fromUtf8(cn)), value);
            out.replace(QStringLiteral("{{%1}}").arg(QString::fromLatin1(en)), value);
        };
        const QString na = QStringLiteral("N/A");

        if (frame >= 0 && db)
        {
            replaceField("当前帧", "Current Frame", db->getFrameNameByFrameNum(frame));
            replaceField("仿真步", "Sim Step", QString::number(db->getFrameTime(frame), 'f', 6));
            replaceField("工况名称", "Case Name", db->getFrameCase(frame));
        }
        replaceField("模型名称", "Model Name", db ? db->getName() : na);

        if (domain >= 0 && anchorId >= 0)
        {
            replaceField("对象编号", "Object ID", QString::number(anchorId));
            QVector3D anchor;
            if (pickAnchorWorld(domain, anchorId, anchor))
            {
                replaceField("对象坐标", "Object Coord", QStringLiteral("%1, %2, %3")
                    .arg(QString::number(anchor.x(), 'f', 3),
                         QString::number(anchor.y(), 'f', 3),
                         QString::number(anchor.z(), 'f', 3)));
            }
            else
            {
                replaceField("对象坐标", "Object Coord", na);
            }
            QString objName = na;
            if (domain == 2 && frame >= 0 && db)
            {
                if (PostData::mPostModelData* md = db->getModelData(frame))
                    if (auto part = md->getPartData(anchorId))
                        objName = part->getPartName();
            }
            replaceField("对象名称", "Object Name", objName);
        }
        else
        {
            replaceField("对象编号", "Object ID", na);
            replaceField("对象名称", "Object Name", na);
            replaceField("对象坐标", "Object Coord", na);
        }

        //结果数值：实体+节点锚当前变量当前帧（getResultDataByFrameID+nodeidmap）
        static const char* kValueCn[] = { "结果数值", "Result Value" };
        bool wantsValue = false;
        for (const char* f : kValueCn)
        {
            if (out.contains(QStringLiteral("{{%1}}").arg(QString::fromUtf8(f))))
            {
                wantsValue = true;
                break;
            }
        }
        if (wantsValue)
        {
            QString valueText = na;
            if (domain == 0 && anchorId >= 0 && frame >= 0 && db)
            {
                if (PostData::mPostModelData* modeldata = db->getModelData(frame))
                {
                    int flag = 0;
                    const QHash<int, double> values = db->getResultDataByFrameID(frame, flag);
                    const int cId = modeldata->_nodedata.nodeidmap.value(anchorId, -1);
                    if (cId >= 0 && values.contains(cId))
                    {
                        //格式随显示样式"数值精度+计数方式"（2026-11-13 用户需求：
                        //屏幕文字如 1.71e+03 的显示格式）：科学计数='e'（精度=有效位）、
                        //小数表示='f'（精度=小数位）、通用='g'（原固定 'g' 仅按精度）
                        const int prec = _precisionSpin ? _precisionSpin->value() : 6;
                        const int fmt = _formatCombo ? _formatCombo->currentIndex() : 2;
                        const char fmtChar = fmt == 0 ? 'e' : (fmt == 1 ? 'f' : 'g');
                        valueText = QString::number(values.value(cId), fmtChar, qMax(1, prec));
                    }
                }
            }
            for (const char* f : kValueCn)
                out.replace(QStringLiteral("{{%1}}").arg(QString::fromUtf8(f)), valueText);
        }
        return out;
    }

    QVector3D mPostTagPanel::sceneLabelOffset() const
    {
        //引线要有可见长度：取视口包围盒半径的一定比例做偏移；取不到时回退固定值
        mxr::AABB box = _isCurrPostViewport
            ? (_postRend ? _postRend->getAABBOfView() : mxr::AABB())
            : (_preRend ? _preRend->getAABBOfView() : mxr::AABB());
        if (!box.valid())
            return QVector3D(1.f, 1.f, 0.f);
        const float r = box.getDiagonal() * 0.5f;
        return QVector3D(r * 0.25f, r * 0.35f, 0.f);
    }

    bool mPostTagPanel::windowAnchorWorld(QVector3D& out) const
    {
        //窗口依附（图3）：九宫格基点 + 水平/垂直% 向窗内偏移（左/上/中=+、右/下=-，
        //0~50% 不出窗），像素 y 向下与引擎 screenToWorld（e.pos 同源）约定一致。
        //出屏钳制不在此做：创建流程改用引擎精确包围盒（getAnnotationLabelScreenBox）
        mxr::mBaseRend* rend = _isCurrPostViewport
            ? static_cast<mxr::mBaseRend*>(_postRend.data())
            : static_cast<mxr::mBaseRend*>(_preRend.data());
        if (!rend || rend->width() <= 0 || rend->height() <= 0)
            return false;
        auto ctrl = rend->getCameraController();
        if (!ctrl)
            return false;

        static const float kGrid[9][2] = {
            { 0.f, 0.f }, { 0.5f, 0.f }, { 1.f, 0.f },    //左上/上中/右上
            { 0.f, 0.5f }, { 0.5f, 0.5f }, { 1.f, 0.5f }, //左中/居中/右中
            { 0.f, 1.f }, { 0.5f, 1.f }, { 1.f, 1.f } };  //左下/下中/右下
        const int grid = _windowAnchorCombo ? _windowAnchorCombo->currentIndex() : 0;
        const float fx = kGrid[grid][0]
            + (grid % 3 == 2 ? -1.f : 1.f) * _windowXSpin->value() / 100.f;
        const float fy = kGrid[grid][1]
            + (grid / 3 == 2 ? -1.f : 1.f) * _windowYSpin->value() / 100.f;
        const float px = fx * rend->width();
        const float py = fy * rend->height();

        //深度取旋转中心视深（过相机目标点平行屏幕的平面，与引擎拾取空背景
        //回退同语义）：标签与模型群同深近似，屏幕分数即创建时的视口落点
        const QVector3D center = ctrl->getRotateCenter();
        const float depth = ctrl->worldToScreen({ center.x(), center.y(), center.z() }).z;
        const auto world = ctrl->screenToWorld(px, py, depth);
        out = { world.x, world.y, world.z };
        return true;
    }

    void mPostTagPanel::clampWindowLabelIntoView(mxr::InteractionAnnotationLabelTool* tool,
        const QString& name)
    {
        //窗口标签出屏钳制（2026-11-12 用户反馈 0% 只露一角）：锚点=首行基线左端点、
        //标签向右/向上展开，贴边百分比必越界——取引擎精确屏幕包围盒（文字/外框/
        //底框/命中同源度量，透视深度缩放已含）越界即原视深平移回屏；创建与
        //"保存修改"窗口重定位共用
        double bl = 0.0, bt = 0.0, br = 0.0, bb = 0.0;
        mxr::mBaseRend* rend = _isCurrPostViewport
            ? static_cast<mxr::mBaseRend*>(_postRend.data())
            : static_cast<mxr::mBaseRend*>(_preRend.data());
        auto ctrl = rend ? rend->getCameraController() : nullptr;
        if (!ctrl || !tool->getAnnotationLabelScreenBox(name, bl, bt, br, bb))
            return;
        double dx = 0.0, dy = 0.0;
        if (bl < 0.0) dx = -bl;
        else if (br > rend->width()) dx = rend->width() - br;
        if (bt < 0.0) dy = -bt;
        else if (bb > rend->height()) dy = rend->height() - bb;
        if (dx == 0.0 && dy == 0.0)
            return;
        QVector3D wPos;
        if (!tool->getAnnotationLabelPosition(name, wPos))
            return;
        //原视深平移：屏幕偏移反投影回世界，标签不随钳制改变远近
        const auto scr = ctrl->worldToScreen({ wPos.x(), wPos.y(), wPos.z() });
        const auto wNew = ctrl->screenToWorld(
            scr.x + static_cast<float>(dx), scr.y + static_cast<float>(dy), scr.z);
        tool->setAnnotationLabelPosition(name, QVector3D(wNew.x, wNew.y, wNew.z));
    }

    int mPostTagPanel::dataIndexById(int id) const
    {
        for (int i = 0; i < _records.count(); ++i)
        {
            if (_records.at(i).id == id)
                return i;
        }
        return -1;
    }

    QString mPostTagPanel::typeName(int typeIndex, int lang) const
    {
        if (typeIndex < 0 || typeIndex > 2)
            return QString();
        return lang == 0 ? QString::fromUtf8(kTypeCn[typeIndex])
            : QString::fromLatin1(kTypeEn[typeIndex]);
    }

    void mPostTagPanel::renderRow(int row)
    {
        QTableWidgetItem* nameIt = _table->item(row, 1);
        if (!nameIt) return;
        int di = dataIndexById(nameIt->data(Qt::UserRole).toInt());
        if (di < 0) return;
        const TagRecord& rec = _records.at(di);

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

        //名称列：占位条目带"（待定义）"后缀（图：标签_2（待定义）），转正后去除
        const QString displayName = rec.pending
            ? rec.name + (lang == 0 ? QString::fromUtf8("（待定义）") : QString(" (Undefined)"))
            : rec.name;
        nameIt->setText(displayName);
        nameIt->setToolTip(rec.content.isEmpty() ? displayName : rec.content);

        QTableWidgetItem* typeIt = _table->item(row, 2);
        typeIt->setText(typeName(rec.typeIndex, lang));
        typeIt->setForeground(QBrush(QColor(kMuted)));
        typeIt->setToolTip(typeName(rec.typeIndex, lang));

        QTableWidgetItem* visIt = _table->item(row, 3);
        //文本仅作排序键，显示由 EyeIconDelegate 覆盖绘制
        visIt->setText(lang == 0 ? (rec.visible ? QString::fromUtf8("显") : QString::fromUtf8("隐"))
            : (rec.visible ? QString("On") : QString("Off")));
        visIt->setData(Qt::UserRole + 1, rec.visible);
        visIt->setTextAlignment(Qt::AlignCenter);
        visIt->setToolTip(lang == 0 ? QString::fromUtf8("点击切换显隐") : QString("Click to toggle visibility"));

        //颜色格写入颜色值文本供表头排序（委托只画色块不画文本）
        QTableWidgetItem* colorIt = _table->item(row, 4);
        colorIt->setText(rec.color.name());
        colorIt->setBackground(QBrush(rec.color));
        colorIt->setToolTip(lang == 0 ? QString::fromUtf8("设置颜色") : QString("Set color"));

        applyCheckedRowBackground(row);
    }

    void mPostTagPanel::applyCheckedRowBackground(int row)
    {
        //对齐原型 tr:has(input:checked)：勾选行整行套选中底色（共享件口径；
        //颜色格 BackgroundRole 是色源数据，排除不参与高亮）
        if (!_table->item(row, 0))
            return;
        const bool checked = _table->item(row, 0)->checkState() == Qt::Checked;
        _table->setRowChecked(row, checked, 4);
    }

    void mPostTagPanel::refreshEmptyState()
    {
        //空态覆盖层由共享表格件内置（两文案区分"暂无标签数据"与"没有匹配"）
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        _table->setEmptyTexts(lang == 0 ? QString::fromUtf8("暂无标签数据") : QString("No labels"),
            lang == 0 ? QString::fromUtf8("没有匹配的标签数据") : QString("No matching labels"));
    }

    void mPostTagPanel::syncManagerState()
    {
        //已选计数（原型"已选 N 项"常显于全选钮之后）；显示/隐藏/删除随之启停
        int checkedAll = 0;
        int visibleCount = 0;
        int visibleChecked = 0;
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            const bool hidden = _table->isRowHidden(r);
            if (!hidden) ++visibleCount;
            if (_table->item(r, 0) && _table->item(r, 0)->checkState() == Qt::Checked)
            {
                ++checkedAll;
                if (!hidden) ++visibleChecked;
            }
        }

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        _managerStatusLabel->setText(lang == 0
            ? QString::fromUtf8("已选 %1 项").arg(checkedAll)
            : QString("%1 selected").arg(checkedAll));
        _btnDelete->setEnabled(checkedAll > 0);
        _btnShow->setEnabled(checkedAll > 0);
        _btnHide->setEnabled(checkedAll > 0);
        _btnSelectAll->setEnabled(visibleCount > 0);

        //对齐原型：可见行全部勾选时全选按钮切换为"取消"
        const bool allSelected = visibleCount > 0 && visibleChecked == visibleCount;
        _btnSelectAll->setText(lang == 0
            ? (allSelected ? QString::fromUtf8("取消") : QString::fromUtf8("全选"))
            : (allSelected ? QString("None") : QString("All")));
    }

    void mPostTagPanel::applyManagerFilter()
    {
        const QString key = _searchEdit->text().trimmed();
        const int typeFilter = _typeFilterCombo->currentData().toInt();

        for (int r = 0; r < _table->rowCount(); ++r)
        {
            QTableWidgetItem* nameIt = _table->item(r, 1);
            const int di = nameIt ? dataIndexById(nameIt->data(Qt::UserRole).toInt()) : -1;
            bool hit = di >= 0;
            if (hit && typeFilter >= 0 && _records.at(di).typeIndex != typeFilter)
                hit = false;
            if (hit && !key.isEmpty() && !_records.at(di).name.contains(key, Qt::CaseInsensitive))
                hit = false;
            _table->setRowHidden(r, !hit);
        }
    }

    void mPostTagPanel::setCheckedRecordsVisible(bool visible)
    {
        bool changed = false;
        std::vector<QString> names;
        {
            MxFunctions::mPanelTable::RowUpdateGuard guard(_table);
            for (int r = 0; r < _table->rowCount(); ++r)
            {
                if (!_table->item(r, 0) || _table->item(r, 0)->checkState() != Qt::Checked) continue;
                QTableWidgetItem* nameIt = _table->item(r, 1);
                const int di = nameIt ? dataIndexById(nameIt->data(Qt::UserRole).toInt()) : -1;
                if (di < 0) continue;
                _records[di].visible = visible;
                renderRow(r);
                changed = true;
                names.push_back(_records.at(di).name);
            }
        }
        _table->horizontalHeader()->setSortIndicatorShown(false);
        if (changed)
            syncManagerState();

        //同步渲染层（视口标签一并显隐）
        if (!names.empty())
            if (auto* tool = annotationLabelTool())
                tool->showOrHideAnnotationLabel(names, visible);
    }

    void mPostTagPanel::applySwatchColor(QPushButton* swatch, const QColor& color)
    {
        //色块按钮换色（内联样式刷新底色）
        swatch->setStyleSheet(QString(
            "QPushButton{background:%1;border:1px solid #DCE2E9;border-radius:2px;}")
            .arg(color.name()));
    }

    void mPostTagPanel::popupUnder(QWidget* popup, QWidget* anchor)
    {
        //弹窗贴锚点控件下方显示（越界收回屏内）
        popup->adjustSize();
        QPoint pos = anchor->mapToGlobal(QPoint(0, anchor->height() + 6));
        if (QScreen* screen = QGuiApplication::screenAt(anchor->mapToGlobal(QPoint(0, 0))))
        {
            const QRect avail = screen->availableGeometry();
            if (pos.x() + popup->width() > avail.right())
                pos.setX(avail.right() - popup->width());
            if (pos.y() + popup->height() > avail.bottom())
                pos.setY(anchor->mapToGlobal(QPoint(0, 0)).y() - popup->height() - 6);
        }
        popup->move(pos);
        popup->show();
        popup->raise();
    }

    /******************** slots ********************/

    void mPostTagPanel::slot_changeLanguage(int language)
    {
        int lang = language;

        //折叠组标题
        _manageFold->setTitle(lang == 0 ? QString::fromUtf8("标签数据列表") : QString("Label List"));
        _toolFold->setTitle(lang == 0 ? QString::fromUtf8("标签定义") : QString("Label Definition"));
        _advFold->setTitle(lang == 0 ? QString::fromUtf8("高级设置") : QString("Advanced"));

        //筛选行
        {
            const int cur = _typeFilterCombo->currentIndex();
            _typeFilterCombo->blockSignals(true);
            _typeFilterCombo->clear();
            _typeFilterCombo->addItem(language == 0 ? QString::fromUtf8("全部类型") : QString("All Types"), -1);
            for (int t = 0; t < 3; ++t)
                _typeFilterCombo->addItem(typeName(t, language), t);
            _typeFilterCombo->setCurrentIndex(cur < 0 ? 0 : cur);
            _typeFilterCombo->blockSignals(false);
        }
        _searchEdit->setPlaceholderText(language == 0
            ? QString::fromUtf8("搜索标签名称") : QString("Search label name"));

        //表头（5 列）
        QStringList heads;
        if (language == 0)
            heads << QString() << QString::fromUtf8("名称") << QString::fromUtf8("类型")
                << QString::fromUtf8("显隐") << QString::fromUtf8("颜色");
        else
            heads << QString() << QString("Name") << QString("Type")
                << QString("Show") << QString("Color");
        _table->setHorizontalHeaderLabels(heads);

        //表尾按钮
        _btnSelectAll->setText(language == 0 ? QString::fromUtf8("全选") : QString("All"));
        _btnShow->setText(language == 0 ? QString::fromUtf8("显示") : QString("Show"));
        _btnHide->setText(language == 0 ? QString::fromUtf8("隐藏") : QString("Hide"));
        _btnDelete->setText(language == 0 ? QString::fromUtf8("删除") : QString("Delete"));
        _btnNew->setText(language == 0 ? QString::fromUtf8("新建") : QString("New"));

        //全部标签显示设置三项（tooltip 原文照录）
        _checkAutoHide->setText(language == 0 ? QString::fromUtf8("自动隐藏") : QString("Auto Hide"));
        _checkAutoHide->setToolTip(language == 0
            ? QString::fromUtf8("附着对象隐藏或不在视图内时隐藏标签，窗口标签不受影响")
            : QString("Hide label when its object is hidden or out of view; window labels unaffected"));
        _checkFill->setText(language == 0 ? QString::fromUtf8("底色") : QString("Fill"));
        _checkFill->setToolTip(language == 0
            ? QString::fromUtf8("当前模型全部标签统一使用公共底色")
            : QString("Use a common background color for all labels of the model"));
        _checkShowName->setText(language == 0 ? QString::fromUtf8("显示名称") : QString("Show Name"));
        _checkShowName->setToolTip(language == 0
            ? QString::fromUtf8("在标签内容前显示管理名称")
            : QString("Show manager name before label content"));

        //标签定义区
        _nameRowLabel->setText(lang == 0 ? QString::fromUtf8("标签名称") : QString("Name"));
        _fieldRowLabel->setText(lang == 0 ? QString::fromUtf8("数据字段") : QString("Data Field"));
        _contentRowLabel->setText(lang == 0 ? QString::fromUtf8("标签内容") : QString("Content"));
        _methodRowLabel->setText(lang == 0 ? QString::fromUtf8("附着方式") : QString("Attach To"));
        _objectRowLabel->setText(lang == 0 ? QString::fromUtf8("附着对象") : QString("Object"));
        _pickRowLabel->setText(lang == 0 ? QString::fromUtf8("拾取对象") : QString("Pick"));
        _windowAnchorLabel->setText(lang == 0 ? QString::fromUtf8("窗口定位") : QString("Anchor"));
        _windowXLabel->setText(lang == 0 ? QString::fromUtf8("水平位置") : QString("Horizontal"));
        _windowYLabel->setText(lang == 0 ? QString::fromUtf8("垂直位置") : QString("Vertical"));
        _systemLabel->setText(lang == 0 ? QString::fromUtf8("坐标系") : QString("System"));
        {
            const int cur = _fieldCombo->currentIndex();
            _fieldCombo->blockSignals(true);
            _fieldCombo->clear();
            for (int i = 0; i < kFieldCount; ++i)
                _fieldCombo->addItem(language == 0 ? QString::fromUtf8(kFieldDefs[i].cn)
                    : QString::fromLatin1(kFieldDefs[i].en));
            _fieldCombo->setCurrentIndex(cur < 0 ? 0 : cur);
            _fieldCombo->blockSignals(false);
        }
        _btnInsertField->setToolTip(language == 0
            ? QString::fromUtf8("插入数据字段") : QString("Insert data field"));
        _nameColorSwatch->setToolTip(language == 0
            ? QString::fromUtf8("标签文字颜色") : QString("Label text color"));

        //三选组：setOptions 按文字匹配保持选中，中英切换必失配——按索引手动保持
        {
            const int keepMethod = _methodChoice->currentIndex();
            _methodChoice->setOptions(language == 0
                ? QStringList{ QString::fromUtf8("实体"), QString::fromUtf8("窗口"), QString::fromUtf8("坐标") }
                : QStringList{ QString("Solid"), QString("Window"), QString("Coords") });
            if (keepMethod >= 0) _methodChoice->setCurrentIndex(keepMethod);
        }
        {
            const int keepObject = _objectChoice->currentIndex();
            _objectChoice->setOptions(language == 0
                ? QStringList{ QString::fromUtf8("节点"), QString::fromUtf8("单元"), QString::fromUtf8("部件") }
                : QStringList{ QString("Node"), QString("Element"), QString("Part") });
            if (keepObject >= 0) _objectChoice->setCurrentIndex(keepObject);
        }
        {
            const int keepAlign = _alignChoice->currentIndex();
            _alignChoice->setOptions(language == 0
                ? QStringList{ QString::fromUtf8("左"), QString::fromUtf8("中"), QString::fromUtf8("右") }
                : QStringList{ QString("Left"), QString("Center"), QString("Right") });
            if (keepAlign >= 0) _alignChoice->setCurrentIndex(keepAlign);
        }
        _styleBox->setTitle(lang == 0 ? QString::fromUtf8("显示样式") : QString("Display Style"));
        _fontStyleLabel->setText(lang == 0 ? QString::fromUtf8("字号字体") : QString("Font"));
        _precisionStyleLabel->setText(lang == 0 ? QString::fromUtf8("数值精度") : QString("Precision"));
        _alignStyleLabel->setText(lang == 0 ? QString::fromUtf8("文字对齐") : QString("Align"));

        _checkInstantSave->setText(language == 0 ? QString::fromUtf8("即时保存") : QString("Instant Save"));
        _btnCancelEdit->setText(language == 0 ? QString::fromUtf8("取消编辑") : QString("Cancel Edit"));
        _btnApplyEdit->setText(language == 0 ? QString::fromUtf8("保存修改") : QString("Apply Changes"));

        //窗口/坐标条件行
        {
            const int cur = _windowAnchorCombo->currentIndex();
            _windowAnchorCombo->blockSignals(true);
            _windowAnchorCombo->clear();
            if (language == 0)
                for (const char* a : { "左上", "上中", "右上", "左中", "居中", "右中", "左下", "下中", "右下" })
                    _windowAnchorCombo->addItem(QString::fromUtf8(a));
            else
                for (const char* a : { "Top Left", "Top Center", "Top Right", "Mid Left", "Center", "Mid Right", "Bottom Left", "Bottom Center", "Bottom Right" })
                    _windowAnchorCombo->addItem(QString::fromLatin1(a));
            _windowAnchorCombo->setCurrentIndex(cur < 0 ? 0 : cur);
            _windowAnchorCombo->blockSignals(false);
        }
        _windowXSpin->setSuffix(QString("%"));
        _windowYSpin->setSuffix(QString("%"));

        //高级设置
        _checkBorder->setText(language == 0 ? QString::fromUtf8("显示边框") : QString("Border"));
        {
            const int cur = _borderCombo->currentIndex();
            _borderCombo->blockSignals(true);
            _borderCombo->clear();
            if (language == 0)
                for (const char* b : { "无", "实线", "虚线" })
                    _borderCombo->addItem(QString::fromUtf8(b));
            else
                for (const char* b : { "None", "Solid", "Dashed" })
                    _borderCombo->addItem(QString::fromLatin1(b));
            _borderCombo->setCurrentIndex(cur < 0 ? 0 : cur);
            _borderCombo->blockSignals(false);
        }
        _checkFollow->setText(language == 0 ? QString::fromUtf8("跟随对象移动") : QString("Follow Object"));
        _checkAnchor->setText(language == 0 ? QString::fromUtf8("锚定到屏幕") : QString("Anchor to Screen"));
        {
            const int cur = _arrangeCombo->currentIndex();
            _arrangeCombo->blockSignals(true);
            _arrangeCombo->clear();
            if (language == 0)
                for (const char* a : { "手动定位", "实体位置", "模型外侧排列" })
                    _arrangeCombo->addItem(QString::fromUtf8(a));
            else
                for (const char* a : { "Manual", "Solid Position", "Beside Model" })
                    _arrangeCombo->addItem(QString::fromLatin1(a));
            _arrangeCombo->setCurrentIndex(cur < 0 ? 0 : cur);
            _arrangeCombo->blockSignals(false);
        }

        //整表重渲（类型/显隐列文案）
        _table->setSortingEnabled(false);
        for (int r = 0; r < _table->rowCount(); ++r)
            renderRow(r);
        _table->setSortingEnabled(true);

        refreshEmptyState();
        syncManagerState();
        refreshPickEntryText();
    }

    void mPostTagPanel::slot_pickEntryClicked(bool checked)
    {
        //以参数为准设置勾选态：真实点击由 clicked(bool) 传入，程序化调用可显式传 true
        _pickEntry->setChecked(checked);
        if (!_isCurrStateValid)
        {
            if (!checked) _pickEntry->setChecked(false);
            return;
        }

        mxr::mBaseRend* pickRend = nullptr;
        if (_isCurrPostViewport)
            pickRend = _postRend;
        else
            pickRend = _preRend;
        if (!pickRend)
        {
            //无可用渲染器（视口已关闭等）：回弹勾选，避免按钮卡在拾取态
            _pickEntry->setChecked(false);
            return;
        }

        if (checked)
        {
            mxr::PickFilter filter = mxr::PickFilter::PickNothing;
            int pickKind = 0;
            mxr::SelectionInfoType selType = mxr::SelectionInfoType::MeshNode;
            currentPickDomain(filter, pickKind, selType);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(pickKind);
            pickRend->setPickFilter(filter);
            getCVM->clearPick();

            //armed 文案在按钮上，状态行清空（对齐原型）
            refreshPickEntryText();
            setStatusLabel(QString());
        }
        else
        {
            pickRend->setPickFilter(mxr::PickFilter::PickNothing);
            refreshPickEntryText();
            setStatusLabel(QString());
        }
    }

    void mPostTagPanel::slot_finishPick(mxr::FinishPickInfo info)
    {
        if (info.pick_operate == mxr::PickOperate::PreView)
            return;

        if (!_isCurrStateValid) return;

        QTimer::singleShot(0, this, [this]() {
            //在途信号防护：singleShot 期间面板可能已被隐藏/切换上下文（hideEvent 已断信号，
            //但仍可能收到断开前最后一次拾取），复检有效性避免写入失效上下文
            if (!_isCurrStateValid) return;
            mxr::PickFilter filter = mxr::PickFilter::PickNothing;
            int pickKind = 0;
            mxr::SelectionInfoType selType = mxr::SelectionInfoType::MeshNode;
            currentPickDomain(filter, pickKind, selType);
            std::vector<int> ids;
            if (getPickResultIDs(selType, ids))
            {
                _pickedIds.clear();
                for (int id : ids)
                    _pickedIds.append(id);
                updatePickStatus();
            }
            });
    }

    //字体下拉暂不接线：渲染库 mFontRender 当前只加载默认思源黑体（无按标签
    //选字体的既有入口），字体设置待渲染侧提供机制后接入——下拉选项保留占位

    void mPostTagPanel::slot_createButton()
    {
        //上下文悬空（视口切换/结果重载后失效）时先重取一次，仍无效才放弃
        if (!_isCurrStateValid)
            newTagPanel(_caseId);
        if (!_isCurrStateValid) return;

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

        //名称校验（原型：请输入标注名称——按面板语境对齐为"标签名称"）
        const QString name = _nameEdit->text().trimmed();
        if (name.isEmpty())
        {
            setStatusLabel(lang == 0 ? QString::fromUtf8("请输入标签名称")
                : QString("Enter a label name"));
            return;
        }

        //标签内容（显示名称勾选时首行拼标签名；{{字段}}占位暂按原文显示——字段值解析属后续轮次）
        QString text = _contentEdit->toPlainText();
        if (_checkShowName && _checkShowName->isChecked())
            text = name + QStringLiteral("\n") + text;
        if (text.trimmed().isEmpty())
        {
            setStatusLabel(lang == 0 ? QString::fromUtf8("请输入标签内容")
                : QString("Enter label content"));
            return;
        }

        //渲染层工具与样式（工具不可用时尚可记录参数，视口不渲染）；
        //样式整包改走 currentLabelStyle()——与"保存修改"同口径单源维护
        //（字号×2 换算注释见其实现；含 screenAnchored：勾选"锚定到屏幕"时
        //创建即屏幕锚定，引擎按世界点投影冻结屏幕位置）
        mxr::InteractionAnnotationLabelTool* tool = annotationLabelTool();
        mxr::AnnotationLabelStyle style = currentLabelStyle();

        //创建计划：窗口=屏幕分数锚点反投影直建（无锚点无引线）；坐标=面板XYZ
        //锚点（引线）；实体=拾取节点代表点锚点（引线）。leader=有锚点（引线+锚点圆）；
        //anchorId=实体锚对象 id（{{字段}}解析与换帧刷新依据），窗口/坐标=-1
        struct Plan { QString name; QVector3D anchor; bool leader; int anchorId; };
        QList<Plan> plans;
        const int method = _methodChoice ? _methodChoice->currentIndex() : 0;   //0实体/1窗口/2坐标
        const int objType = _objectChoice ? _objectChoice->currentIndex() : 0;  //0节点/1单元/2部件

        if (method == 1)
        {
            //窗口依附（2026-11-12 用户反馈改直建）：原一击放置走 createAnnotationLabelV4
            //等待态（下次视口单击才落地），表现为"点了创建屏幕无文字"；现屏幕锚点
            //反投影立即落地——文字+底色+边框即完整形态，无锚点无引线（图3）
            QVector3D winPt;
            if (!windowAnchorWorld(winPt))
            {
                setStatusLabel(lang == 0
                    ? QString::fromUtf8("视口不可用，窗口标签创建失败")
                    : QString("Viewport unavailable, failed to create window label"));
                return;
            }
            plans.append({ name, winPt, false, -1 });
        }
        else if (method == 2)
        {
            plans.append({ name, QVector3D(float(_coordXSpin->value()),
                float(_coordYSpin->value()), float(_coordZSpin->value())), true, -1 });
        }
        else
        {
            if (_pickedIds.isEmpty())
            {
                //缺拾取时自动进入拾取模式引导操作
                //（顺序不能反——进入拾取模式会清状态行，提示要写在后面）
                slot_pickEntryClicked(true);
                setStatusLabel(lang == 0
                    ? QString::fromUtf8("实体依附需先拾取：请在视口点选对象，完成后回来再点“保存修改”")
                    : QString("Entity attach needs picking: select objects in viewport, then click Apply Changes"));
                return;
            }
            int planned = 0;
            for (int i = 0; i < _pickedIds.size(); ++i)
            {
                QVector3D pt;
                //0节点=当前帧坐标 / 1单元=形心 / 2部件=部件节点均值（引线非文字端；
                //2026-11-12 用户需求：单元/部件以中心作锚，前处理单元域几何引擎
                //暂无接口，查询失败走下方统一提示）
                if (!pickAnchorWorld(objType, _pickedIds.at(i), pt))
                    continue;
                if (planned > 0)
                    break;  //固定单签：只取第一个可查询对象（原"每对象一签"复选已随操作行改版移除）
                plans.append({ planned == 0 ? name
                    : QStringLiteral("%1_%2").arg(name).arg(planned + 1), pt, true,
                    _pickedIds.at(i) });
                ++planned;
            }
            if (planned == 0)
            {
                setStatusLabel(lang == 0 ? QString::fromUtf8("拾取对象坐标查询失败")
                    : QString("Failed to query picked object coordinates"));
                return;
            }
        }

        //同名创建=属性面板更新语义：改字体/字号/对齐/文字后再点"创建"，直接
        //刷新已有标签的文字与样式（位置/锚点不动——位置经拖拽、锚点经拾取）；
        //名字不存在才走新建
        int submitted = 0;
        if (tool)
        {
            const std::vector<QString> existing = tool->getAnnotationLabelNames();
            const QString& firstName = plans.first().name;
            if (std::find(existing.begin(), existing.end(), firstName) != existing.end())
            {
                //先查面板记录拿锚点供 {{字段}} 解析（{{结果数值}} 随更新重取当前帧值）
                int updDi = -1;
                for (int i = 0; i < _records.count(); ++i)
                {
                    if (_records.at(i).name == firstName) { updDi = i; break; }
                }
                tool->setAnnotationLabelText(firstName, updDi >= 0
                    ? resolveFields(text, _records.at(updDi).typeIndex, _records.at(updDi).anchorId)
                    : text);
                tool->updateAnnotationLabelStyle(firstName, style);

                //面板记录同步（内容/颜色）并刷新列表行
                if (updDi >= 0)
                {
                    _records[updDi].content = _contentEdit->toPlainText();
                    _records[updDi].color = _nameColorSwatch->property("color").value<QColor>();
                    if (!_records[updDi].color.isValid())
                        _records[updDi].color = QColor(0, 0, 0);
                    _records[updDi].pending = false;	//渲染层已有同名标签即已定义（含外部创建场景）
                    for (int r = 0; r < _table->rowCount(); ++r)
                    {
                        QTableWidgetItem* nameIt = _table->item(r, 1);
                        if (nameIt && nameIt->data(Qt::UserRole).toInt() == _records.at(updDi).id)
                        {
                            _table->setSortingEnabled(false);
                            renderRow(r);
                            _table->setSortingEnabled(true);
                            _table->horizontalHeader()->setSortIndicatorShown(false);
                            break;
                        }
                    }
                }

                _pickedIds.clear();
                exitPickMode();
                setStatusLabel(lang == 0 ? QString::fromUtf8("已更新标签 [%1] 的文字与样式").arg(firstName)
                    : QString("Updated text and style of label [%1]").arg(firstName));
                return;
            }
            for (const auto& p : plans)
            {
                //窗口标签落点即锚点分数位（不加场景偏移、无引线）；实体/坐标标签
                //浮于锚点上方（sceneLabelOffset）并以锚点为端点出引线+锚点圆。
                //文字先过 {{字段}} 解析（{{结果数值}}=实体+节点锚当前帧值，非占位原样）
                const QString resolved = resolveFields(text, method == 0 ? objType : -1, p.anchorId);
                tool->createAnnotationLabelAt(p.name, resolved,
                    p.leader ? p.anchor + sceneLabelOffset() : p.anchor,
                    p.leader,
                    p.leader ? p.anchor : QVector3D(),
                    style);
                ++submitted;
            }

            //窗口标签出屏钳制（clampWindowLabelIntoView：与"保存修改"重定位共用；
            //任意水平/垂直百分比下文字+框+背景完整可见）
            if (method == 1)
                clampWindowLabelIntoView(tool, plans.first().name);
        }

        //记录与列表行（与渲染层标签一一对应；anchorId 供 {{字段}} 换帧重取；
        //upsert：同名"新建"占位条目转正，否则新增行）
        for (const auto& p : plans)
            upsertRecordRow(p.name, p.anchorId);

        applyManagerFilter();
        refreshEmptyState();
        syncManagerState();

        //先清空拾取收集并退出拾取态（会按拾取态清状态行），随后再写结果——顺序不能反
        _pickedIds.clear();
        exitPickMode();

        if (tool == nullptr)
        {
            setStatusLabel(lang == 0
                ? QString::fromUtf8("标签参数已保存（视口工具未就绪）")
                : QString("Label saved (viewport tool unavailable)"));
        }
        else
        {
            //窗口/坐标/实体同为程序化直建，统一"已创建 N 个"
            setStatusLabel(lang == 0
                ? QString::fromUtf8("已创建 %1 个标签").arg(submitted)
                : QString("Created %1 label(s)").arg(submitted));
        }
    }

    void mPostTagPanel::slot_cancelEditClicked()
    {
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        const int di = selectedRecordIndex();
        if (di < 0)
        {
            //无选中：表单清回空态
            _nameEdit->clear();
            _contentEdit->clear();
            setStatusLabel(lang == 0 ? QString::fromUtf8("已取消编辑")
                : QString("Edit cancelled"));
            return;
        }
        formFromRecord(_records.at(di));
        setStatusLabel(lang == 0
            ? QString::fromUtf8("已取消编辑，回退到标签 [%1] 的已存值").arg(_records.at(di).name)
            : QString("Edit cancelled, reverted to saved values of [%1]").arg(_records.at(di).name));
    }

    void mPostTagPanel::slot_applyEditClicked()
    {
        //保存修改（2026-11-13 用户需求）：列表有选中标签时把面板编辑应用到该标签
        //——文字+文字框+背景：setAnnotationLabelText 刷文字（含 {{字段}} 重解析）、
        //updateAnnotationLabelStyle 刷样式（currentLabelStyle 与创建同口径）；
        //附着方式参数响应（2026-11-13 用户需求：屏幕标签要响应附着方式的编辑）：
        //窗口=按九宫格+水平/垂直%重定位（文字+框+背景整体移动，窗口标签无引线，
        //旧引线解绑）；坐标=仅改绑引线锚点（文字位置不动）；实体=有新拾取时改绑
        //锚点（换帧跟随与 {{对象字段}} 解析换到新对象），无拾取不动。按记录名寻址
        //（名称编辑框改动不参与，改名经创建分支同名更新）。
        //无选中记录=创建分支：转 slot_createButton 按当前输入在视口创建新标签
        //（2026-11-13 用户反馈：此前仅提示"先选标签"导致屏幕不出现文字+框+背景）
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        const int di = selectedRecordIndex();
        if (di < 0)
        {
            slot_createButton();
            return;
        }
        if (_records.at(di).pending)
        {
            //选中占位条目（"新建"预建、渲染层尚无此标签）：按记录名走创建分支
            //落地视口并转正——名称框强制回填记录名（名称框改动不参与应用）
            _nameEdit->setText(_records.at(di).name);
            slot_createButton();
            return;
        }
        if (!_isCurrStateValid)
            newTagPanel(_caseId);
        if (!_isCurrStateValid)
        {
            setStatusLabel(lang == 0 ? QString::fromUtf8("视口不可用，无法应用修改")
                : QString("Viewport unavailable, cannot apply changes"));
            return;
        }

        const QString text = _contentEdit->toPlainText();
        if (text.trimmed().isEmpty())
        {
            setStatusLabel(lang == 0 ? QString::fromUtf8("请输入标签内容")
                : QString("Enter label content"));
            return;
        }

        TagRecord& rec = _records[di];
        QString full = text;
        if (_checkShowName && _checkShowName->isChecked())
            full = rec.name + QStringLiteral("\n") + full;

        if (mxr::InteractionAnnotationLabelTool* tool = annotationLabelTool())
        {
            //附着方式参数先于文字解析应用——实体改绑后 {{对象编号/名称/坐标}} 即取新对象值。
            //放置方式与数据锚解耦（2026-11-13 用户反馈：切窗口/坐标后 {{结果数值}} 变
            //N/A 是错的——数据关联不清除，只改摆放；anchorId 仅供字段解析/换帧取值）
            const int method = _methodChoice ? _methodChoice->currentIndex() : 0;
            if (method == 1)
            {
                //窗口：重定位到新屏幕锚点（解绑旧引线——窗口标签无引线）；
                //数据锚保留（{{结果数值}} 等继续解析），换帧不再 re-bind 引线
                QVector3D winPt;
                if (windowAnchorWorld(winPt))
                {
                    tool->unbindAnnotationLabelAnchor(rec.name);
                    tool->setAnnotationLabelPosition(rec.name, winPt);
                    clampWindowLabelIntoView(tool, rec.name);
                    rec.attachMethod = 1;
                }
            }
            else if (method == 2)
            {
                //坐标：仅改绑引线锚点（文字+框+背景位置不动）；数据锚保留
                tool->bindAnnotationLabelAnchor(rec.name, QVector3D(float(_coordXSpin->value()),
                    float(_coordYSpin->value()), float(_coordZSpin->value())));
                rec.attachMethod = 2;
            }
            else
            {
                //实体：有新拾取才改绑（与创建同口径取第一个可查询对象）；
                //无拾取保留原锚不动
                const int objType = _objectChoice ? _objectChoice->currentIndex() : 0;
                for (int id : _pickedIds)
                {
                    QVector3D pt;
                    if (pickAnchorWorld(objType, id, pt))
                    {
                        tool->bindAnnotationLabelAnchor(rec.name, pt);
                        rec.anchorId = id;
                        rec.typeIndex = objType;
                        rec.attachMethod = 0;
                        break;
                    }
                }
            }

            tool->setAnnotationLabelText(rec.name,
                resolveFields(full, rec.typeIndex, rec.anchorId));
            tool->updateAnnotationLabelStyle(rec.name, currentLabelStyle());
        }

        //面板记录同步（内容/颜色）并刷新列表行
        rec.content = text;
        rec.color = _nameColorSwatch->property("color").value<QColor>();
        if (!rec.color.isValid())
            rec.color = QColor(0, 0, 0);
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            QTableWidgetItem* nameIt = _table->item(r, 1);
            if (nameIt && nameIt->data(Qt::UserRole).toInt() == rec.id)
            {
                _table->setSortingEnabled(false);
                renderRow(r);
                _table->setSortingEnabled(true);
                _table->horizontalHeader()->setSortIndicatorShown(false);
                break;
            }
        }
        setStatusLabel(lang == 0
            ? QString::fromUtf8("已应用修改到标签 [%1]").arg(rec.name)
            : QString("Applied changes to [%1]").arg(rec.name));
    }

    void mPostTagPanel::slot_insertFieldClicked()
    {
        //把 {{当前字段}} 插入标签内容光标处（原型 setRangeText 同款语义）
        QTextCursor cursor = _contentEdit->textCursor();
        const QString token = QString("{{%1}}").arg(_fieldCombo->currentText());
        cursor.insertText(token);
        _contentEdit->setTextCursor(cursor);
        _contentEdit->setFocus();
    }

    void mPostTagPanel::slot_frameChanged(int frame)
    {
        Q_UNUSED(frame)
        //换帧跟随：①实体锚随节点位移移动——重算锚点世界坐标（节点/形心/部件均值
        //链路均叠加当前帧位移），仅重绑引线锚点，标签文字/框/背景位置不动
        //（2026-11-13 用户裁定：只锚点跟随，文字留在原地）；②实体+节点锚且含
        //{{字段}} 的标签重取当前帧值刷新文字（{{结果数值}} 跟随动画）；仅后处理视口生效
        if (!_isCurrPostViewport)
            return;
        mxr::InteractionAnnotationLabelTool* tool = annotationLabelTool();
        if (!tool)
            return;
        for (const TagRecord& rec : _records)
        {
            //①引线锚点跟随：仅实体放置方式（窗口标签无引线；坐标锚为用户固定坐标
            //不随帧动——放置方式与数据锚解耦，由 attachMethod 区分）；数据锚
            //anchorId 与放置无关，仍供②的字段重取
            if (rec.attachMethod == 0 && rec.anchorId >= 0)
            {
                QVector3D newAnchor;
                if (pickAnchorWorld(rec.typeIndex, rec.anchorId, newAnchor))
                {
                    tool->bindAnnotationLabelAnchor(rec.name, newAnchor);
                }
            }
            //②含 {{字段}} 的文字重解析（全局字段随帧/工况变化，任意附着方式都刷新）
            if (!rec.content.contains(QStringLiteral("{{")))
                continue;
            //与创建时同口径：显示名称勾选时文字首行拼标签名
            QString full = rec.content;
            if (_checkShowName && _checkShowName->isChecked())
                full = rec.name + QStringLiteral("\n") + full;
            tool->setAnnotationLabelText(rec.name,
                resolveFields(full, rec.typeIndex, rec.anchorId));
        }
    }

    void mPostTagPanel::slot_attachMethodChanged(int index)
    {
        //窗口/坐标条件行显隐（实体=都隐藏）；切换即退出拾取（选项控件切换契约）
        _toolFold->setRowVisible(_windowAnchorCombo, index == 1);
        _toolFold->setRowVisible(_windowXSpin, index == 1);
        _toolFold->setRowVisible(_windowYSpin, index == 1);
        _toolFold->setRowVisible(_systemCombo, index == 2);
        _toolFold->setRowVisible(_coordXSpin, index == 2);
        _toolFold->setRowVisible(_coordYSpin, index == 2);
        _toolFold->setRowVisible(_coordZSpin, index == 2);
        //附着对象/拾取对象仅实体方式显示（2026-11-12 参考图3/4：窗口、坐标无实体附着，
        //两行整行摘除）；离开实体域同时清掉已选集，切回实体时为干净态
        const bool entity = (index == 0);
        _toolFold->setRowVisible(_objectChoice, entity);
        _toolFold->setRowVisible(_pickRowHost, entity);
        if (!entity)
        {
            _pickedIds.clear();
        }
        exitPickMode();
        refreshPickEntryText();
    }

    void mPostTagPanel::slot_attachObjectChanged(int index)
    {
        Q_UNUSED(index)
        //拾取域联动：清空已选并退出拾取
        _pickedIds.clear();
        exitPickMode();
        refreshPickEntryText();
    }

    void mPostTagPanel::slot_searchChanged(const QString& text)
    {
        Q_UNUSED(text)
        applyManagerFilter();
        syncManagerState();
        refreshEmptyState();
    }

    void mPostTagPanel::slot_typeFilterChanged(int index)
    {
        Q_UNUSED(index)
        applyManagerFilter();
        syncManagerState();
        refreshEmptyState();
    }

    void mPostTagPanel::slot_selectAllClicked()
    {
        //存在未勾选则全勾，否则全不勾（仅可见行）
        bool anyUnchecked = false;
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            if (_table->isRowHidden(r)) continue;
            if (_table->item(r, 0) && _table->item(r, 0)->checkState() != Qt::Checked)
            {
                anyUnchecked = true;
                break;
            }
        }
        _table->blockSignals(true);
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            if (_table->isRowHidden(r)) continue;
            if (_table->item(r, 0))
                _table->item(r, 0)->setCheckState(anyUnchecked ? Qt::Checked : Qt::Unchecked);
        }
        _table->blockSignals(false);
        for (int r = 0; r < _table->rowCount(); ++r)
            applyCheckedRowBackground(r);
        syncManagerState();
    }

    void mPostTagPanel::slot_showSelectedClicked()
    {
        setCheckedRecordsVisible(true);
    }

    void mPostTagPanel::slot_hideSelectedClicked()
    {
        setCheckedRecordsVisible(false);
    }

    void mPostTagPanel::slot_deleteButton()
    {
        //删除勾选记录（含筛选隐藏行；从后往前删防下标漂移）
        std::vector<QString> names;
        for (int r = _table->rowCount() - 1; r >= 0; --r)
        {
            if (!_table->item(r, 0) || _table->item(r, 0)->checkState() != Qt::Checked) continue;
            QTableWidgetItem* nameIt = _table->item(r, 1);
            const int di = nameIt ? dataIndexById(nameIt->data(Qt::UserRole).toInt()) : -1;
            if (di >= 0)
            {
                names.push_back(_records.at(di).name);
                _records.removeAt(di);
            }
            _table->removeRow(r);
        }
        //同步渲染层（视口标签与引线一并删除）
        if (!names.empty())
            if (auto* tool = annotationLabelTool())
                tool->deleteAnnotationLabel(names);
        refreshEmptyState();
        syncManagerState();
        notifyTreeRecordsChanged();
    }

    void mPostTagPanel::slot_newButton()
    {
        //新建（2026-11-13 用户参考图：表尾最右蓝实心钮）——列表新增占位条目：
        //标签_N 递增命名（编号只增不回收；跳过与现存记录重名），选中该行并把
        //名称回填标签定义区"标签名称"；条目 pending 态（名称列"（待定义）"），
        //在标签定义区编辑内容后点"保存修改"按此名在视口创建文字+文字框+背景并转正
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        QString name;
        for (;;)
        {
            name = (lang == 0 ? QString::fromUtf8("标签_%1") : QString("Label_%1"))
                .arg(_nextLabelNum);
            bool clash = false;
            for (const TagRecord& r : _records)
            {
                if (r.name == name) { clash = true; break; }
            }
            if (!clash)
                break;
            ++_nextLabelNum;
        }
        ++_nextLabelNum;

        TagRecord rec;
        rec.id = _nextId++;
        rec.name = name;
        rec.typeIndex = _objectChoice ? _objectChoice->currentIndex() : 0;
        rec.anchorId = -1;
        rec.color = _nameColorSwatch->property("color").value<QColor>();
        if (!rec.color.isValid())
            rec.color = QColor(0, 0, 0);
        rec.visible = true;
        rec.pending = true;
        _records.append(rec);
        appendRecordRow(rec);

        applyManagerFilter();
        refreshEmptyState();
        syncManagerState();
        notifyTreeRecordsChanged();

        //名称回填标签定义区（图：标签名称输入框=新建条目名）
        _nameEdit->setText(name);
    }

    void mPostTagPanel::slot_itemChanged(QTableWidgetItem* item)
    {
        //复选列变化：同步整行高亮与计数
        if (item->column() != 0) return;
        applyCheckedRowBackground(item->row());
        syncManagerState();
    }

    void mPostTagPanel::slot_showColorPopup(int row)
    {
        //懒创建取色弹窗（复用控件 FbdColorPicker），按行记录取色
        if (!_colorPicker)
        {
            _colorPicker = new FbdColorPicker(this);
            connect(_colorPicker, &FbdColorPicker::colorChanged, this, [this](const QColor& color) {
                if (_colorRecordId >= 0)
                {
                    const int di = dataIndexById(_colorRecordId);
                    if (di >= 0)
                    {
                        _records[di].color = color;
                        for (int r = 0; r < _table->rowCount(); ++r)
                        {
                            QTableWidgetItem* nameIt = _table->item(r, 1);
                            if (nameIt && nameIt->data(Qt::UserRole).toInt() == _colorRecordId)
                            {
                                _table->setSortingEnabled(false);
                                renderRow(r);
                                _table->setSortingEnabled(true);
                                break;
                            }
                        }
                        //同步渲染层文字颜色（读改写整包，GUI 面板同款用法）
                        if (auto* tool = annotationLabelTool())
                        {
                            mxr::AnnotationLabelStyle style;
                            if (tool->getAnnotationLabelStyle(_records.at(di).name, style))
                            {
                                style.textColor = { float(color.red()), float(color.green()), float(color.blue()) };
                                tool->updateAnnotationLabelStyle(_records.at(di).name, style);
                            }
                        }
                    }
                }
                else
                {
                    //标签名称行色块
                    applySwatchColor(_nameColorSwatch, color);
                    _nameColorSwatch->setProperty("color", color);
                    instantApplyToSelection();	//即时保存勾选时颜色编辑即时生效
                }
                });
        }
        QTableWidgetItem* nameIt = _table->item(row, 1);
        _colorRecordId = nameIt ? nameIt->data(Qt::UserRole).toInt() : -1;
        const int di = _colorRecordId >= 0 ? dataIndexById(_colorRecordId) : -1;
        _colorPicker->setCurrentColor(di >= 0 ? _records.at(di).color
            : _nameColorSwatch->property("color").value<QColor>());
        _colorPicker->popupAt(QCursor::pos());
    }

    void mPostTagPanel::slot_nameColorClicked()
    {
        //名称行色块取色（复用行取色弹窗，_colorRecordId=-1 表示名称色块）
        slot_showColorPopup(-1);
    }

    void mPostTagPanel::slot_borderEnabledChanged(int state)
    {
        //显示边框勾选 → 边框样式下拉可用（L6 复选联动：取消勾选禁用关联控件保留数据）
        _borderCombo->setEnabled(state == Qt::Checked);
    }
}
