//-----------------------------------------------------------------------------
// mPostNVHContributionPanel —— NVH工具集面板实现
//
// 骨架纪律（openspec §2）：三页签容器 + 加载结果页 + 结果显示页 + 应用链
// 结构；§3 图表产出链已接（produceCharts，应用/显示共链），视觉细节待真机
// 对照需求大图（01 加载显示 / 04 结果显示）复验。
// 控件规格族：行控件统一高 26（家族令牌 H=UiMetrics::baseH，2026-11-16 家族
// 对齐轮；原 24 系 §4 弹窗常数误用到面板行）；图标钮 24×24+图标 18（§4 家族
// 常数）；控件态样式随 mPanelBase 家族基座（applyParamBarStyle 单一来源+
// applyTheme 重发），四处 UiPalette 取色 QSS 挂容器级经 refreshPaletteStyles
// 统一重发。
//-----------------------------------------------------------------------------
#include "mPostNVHContributionPanel.h"

#include "mPostNVHContributionModel.h"
#include "mPostNVHDisplayOptions.h"     //显示选项状态（三页签共享 struct）
#include "mNVHDisplayOptionsDlg.h"      //§6.1 显示选项对话框（模态 exec 回写）
#include "mNVHContributorPickDlg.h"     //§7.2 贡献项对话框（部分和排除集合确认）
#include "mFoldSection.h"               //MechMainWindow::FoldSection（共享折叠分组）
#include "UiPalette.h"                  //MxFunctions::UiPalette 语义槽（图标钮选中/悬停取色）
#include "mFramelessMessageBox.h"       //覆盖确认/加载失败提示框（全工程统一自绘）
#include "mPostDockProvider.h"          //dock 提供者（本文件自注册，NVHToolSet dock）
#include "MyTabBar.h"                   //MechMainWindow::MyTabBar（规则 1.2 扁平页签钦定件）
#include "mDockLayoutManager.h"         //产出链收尾回抬本面板 dock（reassertToolSetDock）
#include "CurveView/mCurveViewportFactory.h"   //激活图表谓词（覆盖确认判定）
#include "CurveView/mCurveChartView.h"         //频率红线下发（§4，chartView(chartId)）
#include "CurveCore/mCurveDocController.h"     //产出链门面（§3，D1：同向合法）
#include "CurveCore/CurveDocument.h"           //文档查询（childNodeIds/nodeType/parentNodeId）
#include "CurveCore/CurveDefine.h"             //CurveProp/CurveNodeType
#include "PostCurveGlobal.h"                   //curveDefaultColorWheel
#include "mCurveDataSourceStore.h"             //工况显示文本同源口径（subcaseDisplayText，定义曲线工况列）
#include "CurveWidgets/mCurveEditorWidgets.h"  //PostCurve::mCurveCheckBox（通用勾选框）
#include "mpostwindow_global.h"

#include <QCheckBox>
#include <QComboBox>
#include <QApplication>                 //表头三级字体（行 3/4 显式可读字号）
#include <QCoreApplication>             //closingDown 守卫（reassertToolSetDock）
#include <QButtonGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <QUndoStack>
#include <string>

namespace PostWindow {

namespace {

//面板页签容器：唯一目的是暴露 protected setTabBar 换装工程扁平页签 MyTabBar
//（规则 1.2；裸 QTabWidget 自带原生盒状页签，与全工程页签语言不符——mUiStyle
//刻意跳过 QTabBar、页签交 MyTabBar 自绘）。documentMode 关掉原生页签基座与
//pane 边框，页签行下边界由 MyTabBar 整行基线承担（原型 01 实测 y62-63：通栏
//灰线与选中段蓝线同一 2px 带）
class PanelTabWidget : public QTabWidget
{
public:
    explicit PanelTabWidget(QWidget* parent = nullptr)
        : QTabWidget(parent)
    {
        auto* bar = new MechMainWindow::MyTabBar(this);
        bar->setBaselineEnabled(true);
        setTabBar(bar);
        setDocumentMode(true);
    }
};

} // namespace

//单例（容器形态与曲线树同族：堆常驻，无父级，随主窗口生命周期）
mPostNVHContributionPanel* mPostNVHContributionPanel::_instance = nullptr;

mPostNVHContributionPanel* mPostNVHContributionPanel::getInstance()
{
    if (_instance == nullptr) {
        _instance = new mPostNVHContributionPanel();
    }
    return _instance;
}

mPostNVHContributionPanel::~mPostNVHContributionPanel()
{
    delete _model;
    _instance = nullptr;
}

mPostNVHContributionPanel::mPostNVHContributionPanel(QWidget* parent)
    : MechMainWindow::mPanelBase(parent)
    , _model(new ModalContributionModel)
{
    //语境缩放起步档（context-display-options-state D2 规格表：Load/Show2D
    //缺省 Log 已随 NVHContextScale 缺省值成立，此处置三处非缺省）
    _ctxScale[NVHCtx_ShowBar].scale = NVHScale_PercentPlot;
    _ctxScale[NVHCtx_ShowPolar].scale = NVHScale_Linear;
    _ctxScale[NVHCtx_Research].scale = NVHScale_PercentPlot;
    setObjectName(QStringLiteral("mPostNVHContributionPanel"));
    //面板边距对齐家族（曲线提取面板同款 2,0,2,0，内缩由折叠框自理；
    //2026-11-16 家族对齐轮，原 4,4,4,4 双重内缩）
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 0, 2, 0);
    mainLayout->setSpacing(0);

    _tabs = new PanelTabWidget(this);
    _tabs->addTab(createLoadPage(), tr("加载结果"));
    _tabs->addTab(createResultDisplayPage(), tr("结果显示"));
    _tabs->addTab(createResearchPage(), tr("结果研究"));   //§7 研究页 UI
    mainLayout->addWidget(_tabs);

    //§4.3 视图→面板：画布频率拾取经工厂中继（单次连接覆盖全部/晚物化视口，
    //面板按产出集合过滤）
    connect(PostCurve::mCurveViewportFactory::getInstance(),
        &PostCurve::mCurveViewportFactory::sig_canvasFrequencyPicked,
        this, &mPostNVHContributionPanel::onCanvasFrequencyPicked);

    //三页构建完成后首次设色（图标行/滑条/模态列表/右轨四处 UiPalette QSS；
    //此后切主题经 applyTheme 覆写重发）
    refreshPaletteStyles();
}

QWidget* mPostNVHContributionPanel::createLoadPage()
{
    auto* page = new QWidget(this);
    auto* pageLayout = new QVBoxLayout(page);
    //顶边距=rowGap（原型 01 实测：页签行基线→首条折叠头 11px 气口，token 9 同族）
    pageLayout->setContentsMargins(0, MxFunctions::UiMetrics::rowGap(), 0, 0);
    pageLayout->setSpacing(4);

    //—— 文件选择组 ——
    auto* fileFold = new MechMainWindow::FoldSection(tr("文件选择"), page);
    _filePathEdit = new QLineEdit(page);
    _filePathEdit->setReadOnly(true);
    _filePathEdit->setPlaceholderText(tr("请选择 PFMODE pch 结果文件"));
    _browseBtn = new QPushButton(page);
    _browseBtn->setFixedHeight(26);   //行控件统一高 26（家族令牌 H；2026-11-16 对齐，
                                      //原 24 系 §4 弹窗常数误用到面板行）
    _browseBtn->setFixedWidth(26);
    //浏览图标：需求供图（原型 01 浏览钮原样字形，整备褪烤底/剔边残点）入库
    //Icon/PostCurve 并注册 qrc（ui-spec §5 供图入库次序；禁自绘，字形未改动）
    _browseBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/PostCurve/nvh_file_browse.png")));
    _browseBtn->setIconSize(QSize(18, 16));   //供图原生 18×16，免缩放畸变
    _loadBtn = new QPushButton(tr("加载"));
    auto* fileLbl = new PanelLabel(tr("模态贡献量结果"));    //全局命名空间（mFoldSection.h）
    fileFold->addRow(fileLbl, _filePathEdit, _browseBtn);
    //加载钮独立行（原型 01 实测 144×26：左缘落字段列、右缘对齐行右线；空标签
    //行落 QFormLayout 字段列，AllNonFixedFieldsGrow 自动铺满，不走行内钮定宽）
    _loadBtn->setFixedHeight(26);
    _loadBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    fileFold->addRow(QString(), _loadBtn);

    //—— 结果选择组（六项；行序照原型 01：工况/结果类型/响应点ID/响应点名称/矢量分量/结果集）——
    auto* resultFold = new MechMainWindow::FoldSection(tr("结果选择"), page);
    _subcaseCombo = new QComboBox(page);
    _resultTypeCombo = new QComboBox(page);
    _pointIdCombo = new QComboBox(page);
    _pointNameEdit = new QLineEdit(page);       //可编辑，默认空（design D2：不自动回显）
    _componentCombo = new QComboBox(page);
    _resultSetsCombo = new QComboBox(page);
    resultFold->addRow(tr("工况"), _subcaseCombo);
    resultFold->addRow(tr("结果类型"), _resultTypeCombo);
    resultFold->addRow(tr("响应点ID"), _pointIdCombo);
    resultFold->addRow(tr("响应点名称"), _pointNameEdit);
    resultFold->addRow(tr("矢量分量"), _componentCombo);
    resultFold->addRow(tr("结果集"), _resultSetsCombo);

    //—— 显示选项/应用钮行（原型 01 实测：两钮等宽铺满内容区、缝=rowGap、
    //高 26，左右缘对齐表单内容区；无数据态置灰）——
    _displayOptionsBtn = new QPushButton(tr("显示选项"));
    _displayOptionsBtn->setFixedHeight(26);
    _displayOptionsBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(_displayOptionsBtn, &QPushButton::clicked, this,
        &mPostNVHContributionPanel::onDisplayOptions);   //§6.1 对话框接通解禁
    _applyBtn = new QPushButton(tr("应用"));
    _applyBtn->setFixedHeight(26);
    _applyBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _applyBtn->setEnabled(false);               //加载成功后解禁
    auto* btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 0,
        MxFunctions::UiMetrics::fieldInset(), 0);
    btnRow->setSpacing(MxFunctions::UiMetrics::rowGap());
    btnRow->addWidget(_displayOptionsBtn, 1);
    btnRow->addWidget(_applyBtn, 1);

    pageLayout->addWidget(fileFold);
    pageLayout->addWidget(resultFold);
    pageLayout->addLayout(btnRow);
    pageLayout->addStretch();

    //—— 接线 ——
    connect(_browseBtn, &QPushButton::clicked, this, [this]()
        {
            const QString path = QFileDialog::getOpenFileName(this, tr("打开文件"),
                QString(), tr("PFMODE PCH File (*.pch)"));
            if (!path.isEmpty()) {
                _filePathEdit->setText(path);
            }
        });
    connect(_loadBtn, &QPushButton::clicked, this, &mPostNVHContributionPanel::slot_loadFile);
    connect(_applyBtn, &QPushButton::clicked, this, &mPostNVHContributionPanel::slot_apply);
    return page;
}

// =============================================================================
// 结果显示页与页签交互（§3.1，spec 结果显示页签设置项）
// =============================================================================

//结果显示页：图表类型三选/特定频率（2D线图下替换为频率范围行）/显示最高贡献量
//1~30/复数分量/排序依据 + 显示选项/显示钮。选项集随图表类型收敛（spec）；
//图标：图标库与需求均无该行供图 → 文本下拉兜底（ui-spec §5）；行控件统一
//高 26（家族令牌 H=26，2026-11-16 家族对齐轮；原 24 系 §4 弹窗常数误用），
//无逐控件 QSS
QWidget* mPostNVHContributionPanel::createResultDisplayPage()
{
    auto* page = new QWidget(this);
    auto* pageLayout = new QVBoxLayout(page);
    //顶边距=rowGap（页签行基线→首行图标气口：原型 02 实测 10px，token 6 同族+
    //图标钮内衬 3px；2026-11-18 补，原漏设致图标墨迹贴基线，与 createLoadPage 同款）
    pageLayout->setContentsMargins(0, MxFunctions::UiMetrics::rowGap(), 0, 0);
    pageLayout->setSpacing(4);

    //—— 图表类型三选：图标行 + 类型下拉（spec 38：三图标 + 类型下拉；原型 04
    //图标行墨迹：独立圆角图标钮、选中铺浅蓝底。图标库无该三类切图，按原型
    //供图自绘 SVG 入 Icon/post/（自绘门槛=原型即需求供图）——
    _iconRow = new QWidget(page);
    auto* iconLay = new QHBoxLayout(_iconRow);
    iconLay->setContentsMargins(0, 0, 0, 0);
    iconLay->setSpacing(4);   //缝 4：家族图标钮 24×24 + 缝 4（2026-11-16 家族对齐，
                              //原 28×28 块原型 04 实测；§4 家族常数 24×24/18 优先）
    const QStringList chartNames = { tr("柱状图"), tr("极坐标图"), tr("2D线图") };
    const QStringList chartIcons = {
        QStringLiteral(":/mMainWidgets/Icon/post/nvh_chart_bar.svg"),
        QStringLiteral(":/mMainWidgets/Icon/post/nvh_chart_polar.svg"),
        QStringLiteral(":/mMainWidgets/Icon/post/nvh_chart_line.png") };
    for (int i = 0; i < 3; ++i)
    {
        _chartTypeBtns[i] = new QToolButton(_iconRow);
        _chartTypeBtns[i]->setCheckable(true);
        _chartTypeBtns[i]->setFixedSize(24, 24);   //家族图标钮 24×24（ui-spec-mxmech §4）
        _chartTypeBtns[i]->setIcon(QIcon(chartIcons[i]));
        _chartTypeBtns[i]->setIconSize(QSize(18, 18));   //家族图标 18×18
        _chartTypeBtns[i]->setToolTip(chartNames[i]);
        iconLay->addWidget(_chartTypeBtns[i]);
    }
    //贴左防线：一行全 Fixed 且无 stretch 时 Qt qGeomCalc 会把剩余空间均分到
    //n+1 个缝（真机实测 57|61|61|57 摊开，探针 2026-11-10 确诊）；尾部 stretch
    //吸收剩余量后按 spacing 4 贴左（家族块 24+缝 4 节距 28）
    iconLay->addStretch(1);
    //互斥组：防"再点已选项取消选中"（无组 checkable 钮二次点击会弹起）
    _chartTypeBtnGroup = new QButtonGroup(this);
    _chartTypeBtnGroup->setExclusive(true);
    for (int i = 0; i < 3; ++i)
        _chartTypeBtnGroup->addButton(_chartTypeBtns[i], i);
    pageLayout->addWidget(_iconRow);

    //图表档折叠框（单框动态标题，2026-11-11 用户裁定）：三图标钮切换时
    //setTitle 柱状图/极坐标图/2D线图 + setRowVisible 收敛行集（权威大图
    //02/04/08：柱状/极坐标同构行集仅下拉选项集不同；2D 特定频率+滑条替换为
    //频率范围提示行+起始/终止行且收编进框内）——此前单框标题写死「柱状图」、
    //范围两行游离框外
    auto* fileFold = new MechMainWindow::FoldSection(tr("柱状图"), page);
    _rdFold = fileFold;
    {
        //—— 特定频率（不钉宽：随字段列铺满，与显示最高贡献量/复数分量/排序依据
        //三行同宽同右缘——复核原型 2026-11-11：四行值框右缘对齐一处异类都不留；
        //2D线图档整行摘除）
        _freqEdit = new QLineEdit();
        _freqEdit->setFixedHeight(26);   //行控件统一高 26（家族令牌 H）
        fileFold->addRow(tr("特定频率"), _freqEdit);
        _freqSlider = new QSlider(Qt::Horizontal);
        _freqSlider->setFixedHeight(26);
        fileFold->addRow(_freqSlider);

        //—— 频率范围两行（2D线图档显示，与特定频率/滑条行互斥；权威大图 08：
        //提示行在上 + 起始/终止行内联标签、行尾 Hz，默认回显文件全范围——行
        //结构与结果研究页范围两行同族）——
        _freqRangeHint = new QLabel(page);
        fileFold->addRow(_freqRangeHint);
        _freqRangeRow = new QWidget(page);
        {
            auto* rangeLayout = new QHBoxLayout(_freqRangeRow);
            rangeLayout->setContentsMargins(0, 0, 0, 0);
            rangeLayout->setSpacing(6);
            rangeLayout->addWidget(new QLabel(tr("起始"), _freqRangeRow));
            _freqStartEdit = new QLineEdit(_freqRangeRow);
            _freqStartEdit->setFixedHeight(26);   //行控件统一高 26（家族令牌 H）
            _freqStartEdit->setFixedWidth(70);
            rangeLayout->addWidget(_freqStartEdit);
            _freqEndEdit = new QLineEdit(_freqRangeRow);
            _freqEndEdit->setFixedHeight(26);
            _freqEndEdit->setFixedWidth(70);
            rangeLayout->addWidget(new QLabel(tr("终止"), _freqRangeRow));
            rangeLayout->addWidget(_freqEndEdit);
            rangeLayout->addStretch();
            rangeLayout->addWidget(new QLabel(QStringLiteral("Hz"), _freqRangeRow));
        }
        fileFold->addRow(_freqRangeRow);

        //—— 显示最高贡献量（1~30，默认 10，spec）——
        _topNCombo = new QComboBox(page);
        _topNCombo->setFixedHeight(26);   //行控件统一高 26（家族令牌 H）
        for (int n = 1; n <= 30; ++n) {
            _topNCombo->addItem(QString::number(n));
        }
        _topNCombo->setCurrentIndex(9);
        fileFold->addRow(tr("显示最高贡献量"), _topNCombo);

        //—— 复数分量/排序依据（选项集随图表类型收敛，slot_chartTypeChanged 回填）——
        _complexCombo = new QComboBox(page);
        _complexCombo->setFixedHeight(26);
        fileFold->addRow(tr("复数分量"), _complexCombo);

        _complexSortCombo = new QComboBox(page);
        _complexSortCombo->setFixedHeight(26);
        fileFold->addRow(tr("排序依据"), _complexSortCombo);

        //追加行契约（mFoldSection「追加后需 refreshLayout()」）：内容区
        //maximumHeight 只在 ctor(0 行)/折叠切换/主题档位刷新时重算，行追加后
        //不刷新会把内容压进构造期 0 行高度——modeFold addBlock 先例同款
        fileFold->refreshLayout();

    }
    //裸子件不进布局流（2026-11-11 真机实锤：fold 落自身 sizeHint 100×30、
    //与图标行同 y 叠画、内容 202×147 悬空溢出——裸父子件不参与 pageLayout
    //排布，refreshLayout 只调自身高度救不了页面流，加载/研究页 fold 均有
    //addWidget 对照）
    pageLayout->addWidget(fileFold);

    //—— 显示选项/显示钮行（原型 04 同加载结果页钮行形态：两钮等宽铺满内容
    //区、缝=rowGap、高 26、左右缘对齐表单内容区；显示选项钮 §6 已接对话框）——
    _rdOptionsBtn = new QPushButton(tr("显示选项"));
    _rdOptionsBtn->setFixedHeight(26);
    _rdOptionsBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(_rdOptionsBtn, &QPushButton::clicked, this,
        &mPostNVHContributionPanel::onDisplayOptions);   //§6.1 对话框接通解禁
    _showBtn = new QPushButton(tr("显示"));
    _showBtn->setFixedHeight(26);
    _showBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* showBtnRow = new QHBoxLayout();
    showBtnRow->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 0,
        MxFunctions::UiMetrics::fieldInset(), 0);
    showBtnRow->setSpacing(MxFunctions::UiMetrics::rowGap());
    showBtnRow->addWidget(_rdOptionsBtn, 1);
    showBtnRow->addWidget(_showBtn, 1);
    pageLayout->addLayout(showBtnRow);
    pageLayout->addStretch();


    if (_chartTypeBtns[0] != nullptr && !_chartTypeBtns[0]->isChecked())
        _chartTypeBtns[0]->setChecked(true);   //默认柱状图（第 0 档）
    //图表类型三选接线（缺线修复 2026-11-11：此前类型索引硬编码 0——钮仅高亮，
    //不驱动折叠框切换/选项集收敛/产出类型；QButtonGroup buttonClicked(int) 为
    //仓内 Qt 5.13 惯例接法，mPostCurve 同款）
    connect(_chartTypeBtnGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
        this, &mPostNVHContributionPanel::slot_chartTypeChanged);
    //复数分量↔排序依据双向联动（2026-11-13 用户裁定，原型语义）：两框档位
    //一一对应（投影族=index 0、幅值族=index 1，三类型选项集均两档），任一框
    //切换另一框跟随——实际排序/画线/图例口径单源随复数分量（producer 只读
    //复数分量），双向联动后两框文案恒一致（单向会让排序依据独立可改却不变
    //生效=假象）。同值 setCurrentIndex 不发信号，双向接法无递归；切类型 refill
    //在 QSignalBlocker 下重置两框同回首档，天然同步
    connect(_complexCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        _complexSortCombo, QOverload<int>::of(&QComboBox::setCurrentIndex));
    connect(_complexSortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        _complexCombo, QOverload<int>::of(&QComboBox::setCurrentIndex));
    connect(_freqSlider, &QSlider::valueChanged, this, [this](int index)
        {
            //滑条步进：索引→频率值进三联动漏斗（§4 输入侧）
            if (_model == nullptr || !_model->isLoaded()) {
                return;
            }
            const PointSelection sel = currentSelection();
            const auto freqs = _model->frequencies(sel.subcaseId, sel.resultType,
                sel.gridId, sel.component);
            if (index < 0 || index >= static_cast<int>(freqs.size())) {
                return;
            }
            applyFrequencyState(freqs[static_cast<std::size_t>(index)]);
        });
    connect(_freqEdit, &QLineEdit::editingFinished, this, [this]()
        {
            //数值框输入：原文进三联动漏斗（最近频率点归一在回写内统一做，
            //spec 四舍五入规则）
            if (_model == nullptr || !_model->isLoaded()) {
                return;
            }
            applyFrequencyState(_freqEdit->text().toDouble());
        });
    connect(_showBtn, &QPushButton::clicked, this, &mPostNVHContributionPanel::slot_show);

    slot_chartTypeChanged();   //按默认类型（柱状图）收敛选项集与行显隐
    return page;
}

//§7.1 结果研究页（页签 2）：响应研究组 + 模态列表组 + 钮行（spec 研究页设置项；
//频率范围回显经 refreshResearchFreqRange 随加载刷新）
QWidget* mPostNVHContributionPanel::createResearchPage()
{
    auto* page = new QWidget(this);
    auto* pageLayout = new QVBoxLayout(page);
    //顶边距=rowGap（页签行基线→首条折叠头气口，原型 01 实测 11px 同族；
    //2026-11-18 补，与 createLoadPage/createResultDisplayPage 同款）
    pageLayout->setContentsMargins(0, MxFunctions::UiMetrics::rowGap(), 0, 0);
    pageLayout->setSpacing(4);

    //—— 响应研究组（原型 07 行序：频率范围提示 → 起始/终止/Hz → 计算类型 →
    //去除比例(%) → 选择操作）——
    auto* researchFold = new MechMainWindow::FoldSection(tr("响应研究"), page);

    //频率范围提示行（文案同结果显示页 _freqRangeHint；加载后经
    //refreshResearchFreqRange 回写实际范围，未加载时留空）
    _rFreqRangeHint = new QLabel(page);
    researchFold->addRow(_rFreqRangeHint);   //跨列行（行距随表单令牌）

    //起始/终止行（原型 07：行内短标签 + 行尾 Hz 单位；编辑框成员不变只动行结构）
    _rFreqStartEdit = new QLineEdit(page);
    _rFreqStartEdit->setFixedHeight(26);   //行控件统一高 26（家族令牌 H）
    _rFreqStartEdit->setFixedWidth(70);
    _rFreqEndEdit = new QLineEdit(page);
    _rFreqEndEdit->setFixedHeight(26);
    _rFreqEndEdit->setFixedWidth(70);
    {
        auto* rangeRow = new QWidget(page);
        auto* rangeLayout = new QHBoxLayout(rangeRow);
        rangeLayout->setContentsMargins(0, 0, 0, 0);
        rangeLayout->setSpacing(6);
        rangeLayout->addWidget(new QLabel(tr("起始"), rangeRow));
        rangeLayout->addWidget(_rFreqStartEdit);
        rangeLayout->addWidget(new QLabel(tr("终止"), rangeRow));
        rangeLayout->addWidget(_rFreqEndEdit);
        rangeLayout->addStretch();
        rangeLayout->addWidget(new QLabel(QStringLiteral("Hz"), rangeRow));
        researchFold->addRow(rangeRow);   //跨列行（行距随表单令牌）
    }

    //计算类型/去除比例/选择操作
    _calcTypeCombo = new QComboBox(page);
    _calcTypeCombo->setFixedHeight(26);   //行控件统一高 26（家族令牌 H）
    _calcTypeCombo->addItems({ tr("部分和"), tr("贡献量叠加") });
    researchFold->addRow(tr("计算类型"), _calcTypeCombo);
    _excludePctSpin = new QSpinBox(page);
    _excludePctSpin->setRange(0, 100);
    _excludePctSpin->setValue(100);
    _excludePctSpin->setFixedHeight(26);
    researchFold->addRow(tr("去除比例 (%)"), _excludePctSpin);   //原型 07：括号前带空格
    _pickOpCombo = new QComboBox(page);
    _pickOpCombo->setFixedHeight(26);
    _pickOpCombo->addItems({ tr("Exclude"), tr("Include") });
    researchFold->addRow(tr("选择操作"), _pickOpCombo);

    //贡献量叠加档置灰簇（2026-11-18 裁定，openspec change research-overlay-gating）：
    //叠加档下"去除比例/「…」钮（选择贡献项入口）/差值行两控件"一并置灰——四者
    //均只被部分和产出链消费（producePartialSumChart）；"选择操作"与第 4 钮不入簇
    //（用户 AskUserQuestion 钉死：前者叠加档保持可用，后者维持 D2 柱状成柱启用逻辑，
    //与计算类型正交）；部分和档对称恢复；默认档=部分和（addItems 先于 connect，
    //构造期不触发，四控件构造默认可用即正确初态）
    connect(_calcTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int idx)
    {
        const bool partialSum = (idx == 0);   //部分和=0 档全簇恢复可用
        if (_excludePctSpin != nullptr) {
            _excludePctSpin->setEnabled(partialSum);
        }
        if (_selContribBtn != nullptr) {
            _selContribBtn->setEnabled(partialSum);
        }
        if (_diffCheck != nullptr) {
            _diffCheck->setEnabled(partialSum);
        }
        if (_diffModeCombo != nullptr) {
            _diffModeCombo->setEnabled(partialSum);
        }
    });

    //—— 模态列表组：多选列表 + 右侧竖排图标轨（原型 07：3 钮 24×24 缝 10，
    //全选/清空/反选；图标=原型供图自绘 SVG 入 Icon/post/，nvh_chart_* 先例）——
    auto* modeFold = new MechMainWindow::FoldSection(tr("模态列表"), page);
    {
        auto* listWrap = new QWidget(page);
        auto* listLayout = new QHBoxLayout(listWrap);
        //块横边距=fieldInset（2026-11-18 补）：addBlock 块挂内容区 VBox 零边距，
        //不随表单 fieldInset 内缩——现状列表框贴 dock 左缘（x0）、右轨贴右缘，
        //而原型列表框左缘/右轨右缘与表单内容区同一条线（实测 22≈字段 23、
        //257≈下拉右缘 258）；块内自补横边距对齐，先例 mCurveEditorPages
        //_positionBlock（同 addBlock 机制自带横边距 10/10）
        listLayout->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 0,
            MxFunctions::UiMetrics::fieldInset(), 0);
        listLayout->setSpacing(10);   //原型 07 实测列表→右轨缝 10
        _modeList = new QListWidget(page);
        _modeList->setSelectionMode(QAbstractItemView::MultiSelection);
        //取色 QSS 在 refreshPaletteStyles（全局 qss 对 QListWidget::item 的
        //height:26+margin 8/8/5 漏入防线：三态全钉，先例 mTemplateCurveDlg；
        //挂到 this 使切主题经 applyTheme 重取色）
        //列表可见行数对齐原型（复核原型 07 实测可见 8.5 行、节距 23.4；钉 23 后
        //8.5 行 ≈ 196+框缘取 198，原 160 只见 6 行）
        _modeList->setFixedHeight(198);
        listLayout->addWidget(_modeList, 1);   //列表吸收余量，右轨贴列表同高
        //右侧竖排图标轨（原型 07 实测块 24×24 缝 10；ui-spec-mxmech §4 图标钮
        //24×24+图标 18 家族常数；QSS 下沉容器级一次，取色在 refreshPaletteStyles）
        _modeRail = new QWidget(listWrap);
        auto* railLay = new QVBoxLayout(_modeRail);
        railLay->setContentsMargins(0, 0, 0, 0);
        railLay->setSpacing(10);   //原型 07 实测块节距 34（块 24+缝 10）
        const auto railBtn = [this, railLay](const char* iconRes, const QString& tip)
        {
            auto* btn = new QToolButton(_modeRail);
            btn->setFixedSize(24, 24);   //ui-spec-mxmech §4 图标钮 24×24
            btn->setIconSize(QSize(18, 18));
            btn->setIcon(QIcon(QLatin1String(iconRes)));
            btn->setToolTip(tip);
            railLay->addWidget(btn);
            return btn;
        };
        _selAllBtn = railBtn(":/mMainWidgets/Icon/post/nvh_mode_all.svg", tr("全选"));
        _selNoneBtn = railBtn(":/mMainWidgets/Icon/post/nvh_mode_none.svg", tr("清空"));
        _selInvBtn = railBtn(":/mMainWidgets/Icon/post/nvh_mode_invert.svg", tr("反选"));
        //第 4 钮（需求图 03）：2026-11-17 D2 裁定翻活——柱状图成柱后记录成柱模态集
        //并启用本钮；点击=清当前列表选择→按集合选中命中行（spec 图三语义：点第 4 钮
        //把柱状图正在显示的模态行选中，供后续研究操作）。图标仍挂资源库现成件
        //zhuzhuangtu.png（黑轴灰柱单蓝柱，与原型彩色三柱形态有差、待供图——禁自绘
        //约定维持，供图后另轮替换）；构造期 disabled 维持（未成柱时明示不可用）
        _selBarBtn = railBtn(":/mMainWidgets/Icon/post/zhuzhuangtu.png",
            tr("选中柱状图显示模态"));
        _selBarBtn->setEnabled(false);
        connect(_selBarBtn, &QToolButton::clicked, this, [this]()
        {
            _modeList->clearSelection();
            for (int i = 0; i < _modeList->count(); ++i)
            {
                auto* it = _modeList->item(i);
                if (_barChartBarModes.contains(it->data(Qt::UserRole).toInt())) {
                    it->setSelected(true);
                }
            }
        });
        //第 5 钮"…"：图标禁自绘约定（2026-11-11 用户裁定：今后所有图标禁止自绘）
        //且资源库无省略号件、QStyle 无对应标准图标——三点是文本字符，走文字钮；
        //动作语义无权威源，按面板内最近实体接"选择贡献项"编辑入口（exec 结果
        //不驱动产出，产出仍走"显示"漏斗），待需求补充后调整。2026-11-18 升成员
        //_selContribBtn：叠加档随计算类型联动置灰（openspec change
        //research-overlay-gating，局部变量联动 lambda 够不着）
        _selContribBtn = new QToolButton(_modeRail);
        _selContribBtn->setFixedSize(24, 24);
        _selContribBtn->setText(QStringLiteral("…"));
        _selContribBtn->setToolTip(tr("更多"));
        connect(_selContribBtn, &QToolButton::clicked, this, [this]()
            {
                if (_model == nullptr || !_model->isLoaded()) {
                    MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
                        tr("请先加载模态贡献量结果文件"));
                    return;
                }
                const PointSelection sel = currentSelection();
                if (!sel.valid()) {
                    return;
                }
                const auto modeTable = _model->modeFrequencies(sel.subcaseId);
                if (modeTable.empty()) {
                    return;
                }
                //页内模态列表选中集回显预选（行选中语义，spec「选中 Mode N」）
                std::vector<int> preselectModes;
                for (auto* item : _modeList->selectedItems()) {
                    preselectModes.push_back(item->data(Qt::UserRole).toInt());
                }
                mNVHContributorPickDlg dlg(modeTable, _excludePctSpin->value(),
                    preselectModes, this);
                dlg.exec();
            });
        railLay->addWidget(_selContribBtn);
        railLay->addStretch(1);   //贴顶防线（pair3 实诊：全 Fixed 行会被均分摊开）
        listLayout->addWidget(_modeRail);

        //全选/清空/反选：列表重建时经 QSignalBlocker 防误触（回填在
        //slot_loadFile 级联回填处）
        connect(_selAllBtn, &QAbstractButton::clicked, this, [this]()
        {
            for (int i = 0; i < _modeList->count(); ++i)
                _modeList->item(i)->setSelected(true);
        });
        connect(_selNoneBtn, &QAbstractButton::clicked, this, [this]()
        {
            _modeList->clearSelection();
        });
        connect(_selInvBtn, &QAbstractButton::clicked, this, [this]()
        {
            for (int i = 0, n = _modeList->count(); i < n; ++i)
            {
                auto* it = _modeList->item(i);
                it->setSelected(!it->isSelected());
            }
        });
        //内容块走 addBlock（挂 FoldSection 内容布局）：contentLayout() 访问器
        //实为基类 m_mainLayout（标题头+内容区的顶层布局，mFoldSection.h:100-102），
        //经它 addWidget 会把块加成 fold 本体的兄弟项——pageLayout 撑 fold 多少
        //块就吞多少（探针 2026-11-10 实锤 fold=464 时 wrap=426、父链直挂 fold）
        modeFold->addBlock(listWrap);
        //追加块契约（mFoldSection.h「追加后需 refreshLayout()」）：刷新折叠高度
        modeFold->refreshLayout();
    }

    //—— 显示差值曲线行（原型 07：模态列表组下方、钮行之上独立行；控件序=
    //勾选框→标签→下拉，默认勾选（原型 07 勾选态）；成员 _diffCheck/_diffModeCombo
    //不变只挪容器）——
    auto* diffRow = new QWidget(page);
    {
        auto* diffLayout = new QHBoxLayout(diffRow);
        diffLayout->setContentsMargins(
            MxFunctions::UiMetrics::fieldInset(), 0,
            MxFunctions::UiMetrics::fieldInset(), 0);   //与钮行同内容区左缘
        diffLayout->setSpacing(6);
        _diffCheck = new PostCurve::mCurveCheckBox(page);   //通用勾选框（曲线属性页同款，勿裸 QCheckBox）
        _diffCheck->setChecked(true);   //原型 07 默认勾选态
        diffLayout->addWidget(_diffCheck);
        diffLayout->addWidget(new PanelLabel(tr("显示差值曲线")));
        _diffModeCombo = new QComboBox(page);
        _diffModeCombo->setFixedHeight(26);   //行控件统一高 26（家族令牌 H）
        _diffModeCombo->addItems({ tr("% of Response"), tr("Scale same as Response") });
        diffLayout->addWidget(_diffModeCombo);
        diffLayout->addStretch();
    }

    //—— 钮行：显示选项/显示（原型 07 同结果显示页钮行形态：两钮等宽铺满内容
    //区、缝=rowGap、高 26、左右缘对齐表单内容区）——
    _rOptionsBtn = new QPushButton(tr("显示选项"));
    _rOptionsBtn->setFixedHeight(26);
    _rOptionsBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(_rOptionsBtn, &QPushButton::clicked, this,
        &mPostNVHContributionPanel::onDisplayOptions);
    _rShowBtn = new QPushButton(tr("显示"));
    _rShowBtn->setFixedHeight(26);
    _rShowBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(_rShowBtn, &QPushButton::clicked, this,
        &mPostNVHContributionPanel::onResearchShow);   //§7.2 研究页"显示"漏斗
    auto* rBtnRow = new QHBoxLayout();
    rBtnRow->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 0,
        MxFunctions::UiMetrics::fieldInset(), 0);
    rBtnRow->setSpacing(MxFunctions::UiMetrics::rowGap());
    rBtnRow->addWidget(_rOptionsBtn, 1);
    rBtnRow->addWidget(_rShowBtn, 1);
    pageLayout->addWidget(researchFold);
    pageLayout->addWidget(modeFold);
    pageLayout->addWidget(diffRow);   //页尾行（插入序=可见序，须在两折叠组之后）
    pageLayout->addLayout(rBtnRow);
    pageLayout->addStretch();

    return page;
}

//四处 UiPalette 取色 QSS 统一重发（构造期首次设色 + applyTheme 切主题重取色；
//串内不写解释性注释——QSS 静默失效坑，注释在串外）：
//  ①图标行 QToolButton：透明底/悬停浅蓝/选中蓝底（UiPalette 语义槽三主题成立）；
//  ②特定频率滑条：全工程无 QSlider 样式分支（全局 qss/mParamBarStyle 均无），
//    家族先例 mSTLRemeshPanel=裸原生——2026-11-16 用户裁定方案 b：容器级
//    UiPalette 自绘槽/柄跟主题（groove 灰带/sub-page 主蓝/handle 白芯描边，
//    悬停/按压/禁用三态齐）；
//  ③模态列表 item：全局 qss height:26+margin 漏入防线三态全钉（selected 置末）；
//    选中字色 #333333=先例 mTemplateCurveDlg 同款（2026-11-18 勘误裁定：白字压
//    selectedHex #e8f3ff 浅蓝底对比度不足选中行近隐形——先例实为浅蓝底+深字，
//    原"记录不修"系对先例形态误记，同日推翻）；
//  ④右轨 QToolButton：1px 描边透明底/悬停浅蓝
void mPostNVHContributionPanel::refreshPaletteStyles()
{
    if (_iconRow != nullptr)
    {
        _iconRow->setStyleSheet(QStringLiteral(
            "QToolButton { background:transparent; border:none; border-radius:4px; }"
            "QToolButton:hover { background:%1; }"
            "QToolButton:checked { background:%2; }")
            .arg(MxFunctions::UiPalette::hoverHex(), MxFunctions::UiPalette::selectedHex()));
    }
    if (_freqSlider != nullptr)
    {
        _freqSlider->setStyleSheet(QStringLiteral(
            "QSlider::groove:horizontal { height:4px; border:none; border-radius:2px; background:%1; }"
            "QSlider::sub-page:horizontal { height:4px; border-radius:2px; background:%2; }"
            "QSlider::handle:horizontal { width:14px; height:14px; margin:-6px 0px; border-radius:8px; background:%3; border:1px solid %2; }"
            "QSlider::handle:horizontal:hover { background:%4; }"
            "QSlider::handle:horizontal:pressed { background:%5; }"
            "QSlider::groove:horizontal:disabled { background:%6; }"
            "QSlider::sub-page:horizontal:disabled { background:%6; }"
            "QSlider::handle:horizontal:disabled { background:%6; border:1px solid %7; }")
            .arg(MxFunctions::UiPalette::borderHex(),      //%1 槽灰带
                 MxFunctions::UiPalette::primaryHex(),     //%2 已选段/柄描边主蓝
                 MxFunctions::UiPalette::surfaceHex(),     //%3 柄常态白芯
                 MxFunctions::UiPalette::hoverHex(),       //%4 柄悬停
                 MxFunctions::UiPalette::selectedHex(),    //%5 柄按压
                 MxFunctions::UiPalette::panelHex(),       //%6 禁用底
                 MxFunctions::UiPalette::disabledHex()));  //%7 禁用柄描边
    }
    if (_modeList != nullptr)
    {
        _modeList->setStyleSheet(QStringLiteral(
            "QListWidget::item { height:23px; margin:0px; border:none; }"
            "QListWidget::item:hover { height:23px; margin:0px; background:%1; }"
            "QListWidget::item:selected { height:23px; margin:0px; background:%2; color:#333333; }")
            .arg(MxFunctions::UiPalette::hoverHex(), MxFunctions::UiPalette::selectedHex()));
    }
    if (_modeRail != nullptr)
    {
        _modeRail->setStyleSheet(QStringLiteral(
            "QToolButton { border:1px solid %1; border-radius:4px; background:transparent; }"
            "QToolButton:hover { background:%2; }")
            .arg(MxFunctions::UiPalette::borderHex(), MxFunctions::UiPalette::hoverHex()));
    }
}

//主题/档位重发（mPanelBase 覆写）：基座重发参数栏控件态样式后，
//本面板四处 UiPalette 取色 QSS 同步重取色（此前切主题三处 QSS 停留旧主题色）
void mPostNVHContributionPanel::applyTheme()
{
    MechMainWindow::mPanelBase::applyTheme();
    refreshPaletteStyles();
}


//图表类型三图标钮切换：折叠框标题/行集随类型切换（柱状图/极坐标图/2D线图，
//单框动态标题）、复数分量/排序依据选项集随类型收敛（spec 结果显示页签设置项）；
//特定频率行与频率范围行互斥显隐（2D线图替换，起始/终止默认回显文件全范围）
void mPostNVHContributionPanel::slot_chartTypeChanged()
{
    const int typeIdx = currentChartType();
    //柱状图：投影/幅值；极坐标图：投影/幅值-相位；2D线图：投影绝对值/幅值-相位
    QStringList complexItems;
    //排序依据：柱状/极坐标=投影绝对值/幅值；2D线图=投影绝对值 RSS/幅值 RSS
    QStringList sortItems;
    switch (typeIdx) {
    case 1:     //极坐标图
        complexItems = QStringList{ tr("投影"), tr("幅值-相位") };
        sortItems = QStringList{ tr("投影绝对值"), tr("幅值") };
        break;
    case 2:     //2D线图
        complexItems = QStringList{ tr("投影绝对值"), tr("幅值-相位") };
        sortItems = QStringList{ tr("投影绝对值 RSS"), tr("幅值 RSS") };
        break;
    default:    //柱状图
        complexItems = QStringList{ tr("投影"), tr("幅值") };
        sortItems = QStringList{ tr("投影绝对值"), tr("幅值") };
        break;
    }
    auto refill = [](QComboBox* combo, const QStringList& items)
    {
        const QSignalBlocker blocker(combo);
        combo->clear();
        combo->addItems(items);
    };
    refill(_complexCombo, complexItems);
    refill(_complexSortCombo, sortItems);

    const bool is2D = typeIdx == 2;
    //折叠框标题/行集随类型切换（权威大图 02/04/08）：柱状/极坐标档显示特定
    //频率+滑条行，2D 线图档整行摘除两行、插回频率范围提示行+起始/终止行——
    //行级摘除/插回走 setRowVisible（QFormLayout 行隐藏仍占位，机制见
    //mFoldSection.h），标题复用图标钮 tooltip 文案单源
    if (_rdFold != nullptr) {
        _rdFold->setTitle(_chartTypeBtns[typeIdx]->toolTip());
        _rdFold->setRowVisible(_freqEdit, !is2D);
        _rdFold->setRowVisible(_freqSlider, !is2D);
        _rdFold->setRowVisible(_freqRangeHint, is2D);
        _rdFold->setRowVisible(_freqRangeRow, is2D);
        _rdFold->refreshLayout();
    }
    if (is2D && _model != nullptr && _model->isLoaded()) {
        const PointSelection sel = currentSelection();
        const auto freqs = _model->frequencies(sel.subcaseId, sel.resultType,
            sel.gridId, sel.component);
        if (!freqs.empty()) {
            const QString startText = QString::number(freqs.front(), 'g', 6);
            const QString endText = QString::number(freqs.back(), 'g', 6);
            _freqStartEdit->setText(startText);
            _freqEndEdit->setText(endText);
            _freqRangeHint->setText(tr("频率范围：%1 Hz ~ %2 Hz").arg(startText, endText));
        }
    }
}

//当前图表类型（三图标钮互斥组选中档：0 柱状/1 极坐标/2 2D线图；无选中兜底
//柱状——构造期/编程置态时 checkedId 可能短暂无值）
int mPostNVHContributionPanel::currentChartType() const
{
    const int id = _chartTypeBtnGroup != nullptr ? _chartTypeBtnGroup->checkedId() : -1;
    return (id >= 0 && id < 3) ? id : 0;
}

//显示按钮（结果显示页）：走产出链显示链——只产当前图表类型一张（需求大图
//06 交互流程：切类型后"显示"仅产出该类型图；旧版顺带重产响应曲线=多生成
//一张，2026-11-13 用户裁定移除）。覆盖确认随布局选项同规
void mPostNVHContributionPanel::slot_show()
{
    if (_model == nullptr || !_model->isLoaded()) {
        MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
            tr("请先在加载结果页加载结果文件"));
        return;
    }
    produceCharts(true);
}

//当前选择元组解析：下拉选区→模型键。工况号走 item userData（currentSubcaseId，
//工况文案带 (label) 后缀后不再从文案剥前缀）；结果类型/分量文本即模型侧原始
//令牌（回填同源）；响应点ID 直接取整
mPostNVHContributionPanel::PointSelection mPostNVHContributionPanel::currentSelection() const
{
    PointSelection sel;
    if (_model == nullptr || !_model->isLoaded()) {
        return sel;
    }
    sel.subcaseId = currentSubcaseId();
    sel.resultType = _resultTypeCombo->currentText().toStdString();
    sel.gridId = _pointIdCombo->currentText().toInt();
    sel.component = _componentCombo->currentText().toStdString();
    return sel;
}

//当前工况号：item userData 回取（slot_loadFile 回填时经 addItem 第二参挂号）；
//空下拉/未加载返回 -1（维持 currentSelection 有效性门语义）
int mPostNVHContributionPanel::currentSubcaseId() const
{
    const QVariant data = _subcaseCombo->currentData();
    return data.isValid() ? data.toInt() : -1;
}

//响应点表头令牌：响应点名称框非空→以名称替换表头行内 ID 令牌（"Response @/
//… @/Node <ID>" 各段整体换名，2026-11-16 用户裁定；simplified 折叠换行防破坏
//表头行结构）；空→ID 数字（现行为）。仅表头消费——曲线名/kName/来源五元组/
//Note 维持 ID 口径
QString mPostNVHContributionPanel::pointHeaderToken(const PointSelection& sel) const
{
    const QString name = _pointNameEdit->text().simplified();
    return name.isEmpty() ? QString::number(sel.gridId) : name;
}

//加载：读 pch → 模型聚合 → 回填级联选项集（工况/结果类型/响应点ID/矢量分量）
void mPostNVHContributionPanel::slot_loadFile()
{
    const QString path = _filePathEdit->text();
    if (path.isEmpty()) {
        return;
    }
    std::string error;
    //窄字符 CRT API 按本地 8 位编码（Windows ACP）解释路径；toStdString 给 UTF-8，
    //中文路径打不开 → 空流 → 误报"不是 PFMODE 文件"。须 toLocal8Bit（mPost3DResult/
    //mIMxdbFile1 等同款惯例，2026-11-10 真机实锤修复）
    if (!_model->loadFromFile(path.toLocal8Bit().toStdString(), &error)) {
        //2026-11-18 供图裁定（openspec change contribution-load-fail-dialog）：失败弹窗
        //统一友好文案+「知道了」钮；prototypeStyle=true=原型三段式（白标题栏只留✕/
        //橙圆！/蓝底白字主钮 400×156，供图同款；首版漏传尾参落通用形态三键标题栏，
        //用户真机指正）——调用形态照抄本面板覆盖确认框 L1040-1043 先例；不区分
        //错误类别、不展示明细（error 仍作出参接住，仅不上屏）；范围仅本按钮——
        //模型层与其他面板不动（用户明裁勿扩大范围）
        MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
            tr("加载失败，当前选择文件不是面板贡献量结果文件，请替换文件后，再尝试重新加载。"),
            tr("知道了"), QString(), QString(), 0, 0, true);
        return;
    }
    //级联选项集回填（首项默认选中；响应点名称不自动回显——design D2）
    auto fillCombo = [](QComboBox* combo, const QStringList& items)
    {
        combo->clear();
        combo->addItems(items);
        if (combo->count() > 0) {
            combo->setCurrentIndex(0);
        }
    };
    //工况显示文本同源"定义曲线"工况列（mCurveDataSourceStore::subcaseDisplayText：
    //"Subcase <id> (label)"，label=该工况首块非空 $LABEL；2026-11-16 用户裁定
    //NVH 工况下拉对齐定义曲线口径）；工况号挂 item userData，解析不再从文案剥
    //前缀（currentSubcaseId）
    const auto subcaseIds = _model->subcaseIds();
    const auto file = _model->file();       //加载成功后恒非空
    _subcaseCombo->clear();
    for (int id : subcaseIds) {
        _subcaseCombo->addItem(
            PostCurve::mCurveDataSourceStore::subcaseDisplayText(*file, id), id);
    }
    if (_subcaseCombo->count() > 0) {
        _subcaseCombo->setCurrentIndex(0);
    }

    QStringList typeNames;
    const int subcaseId = _model->subcaseIds().empty() ? -1 : _model->subcaseIds().front();
    for (const auto& name : _model->resultTypeNames(subcaseId)) {
        typeNames << QString::fromStdString(name);
    }
    fillCombo(_resultTypeCombo, typeNames);

    QStringList pointIds;
    const auto& type = _resultTypeCombo->currentText().toStdString();
    for (int id : _model->responsePointIds(subcaseId, type)) {
        pointIds << QString::number(id);
    }
    fillCombo(_pointIdCombo, pointIds);

    QStringList components;
    for (const auto& comp : _model->components(subcaseId, type,
        _pointIdCombo->currentText().toInt())) {
        components << QString::fromStdString(comp);
    }
    fillCombo(_componentCombo, components);

    //结果集：PFMODE 单一集合，固定项（原型 01 显示 "Structure Mode"）
    fillCombo(_resultSetsCombo, { QStringLiteral("Structure Mode") });

    _applyBtn->setEnabled(true);

    //特定频率滑条量程随文件频率集刷新（页签 1，§4 红线联动的输入侧基座）
    const auto freqsForSlider = _model->frequencies(subcaseId,
        _resultTypeCombo->currentText().toStdString(),
        _pointIdCombo->currentText().toInt(),
        _componentCombo->currentText().toStdString());
    _freqSlider->setRange(0, static_cast<int>(freqsForSlider.size()) - 1);
    if (!freqsForSlider.empty()) {
        _freqEdit->setText(QString::number(freqsForSlider.front(), 'g', 6));
    }

    //D2：换文件后旧柱状图成柱集失效——清空集合并禁用第 4 钮（待下次柱状图
    //产出重新记录/启用）
    _barChartBarModes.clear();
    if (_selBarBtn != nullptr) {
        _selBarBtn->setEnabled(false);
    }

    //研究页随加载刷新：范围回显 + 模态列表回填（§7.1，spec 回显文件实际范围）
    refreshResearchFreqRange();
}

//应用链（2.4+§3，加载结果页）：覆盖确认（布局=当前激活窗口且激活视口有内容
//时，原型 01 提示框；精确判定收口进 produceCharts）→ 图表产出（只产响应曲线
//一张，需求大图 01 权威——不再顺带产结果显示页签当前类型图）→ 自动切"结果
//显示"页签。产出激活链尾环会置前「参数」dock（editCurveNode），收尾经
//reassertToolSetDock 回抬本面板——应用后落点=NVH工具集·结果显示（2026-11-17
//真机缺陷裁定，图二）
void mPostNVHContributionPanel::slot_apply()
{
    if (_model == nullptr || !_model->isLoaded()) {
        return;     //未加载数据不动作（按钮常态置灰，防御）
    }
    if (!produceCharts(false).isEmpty()) {
        _tabs->setCurrentIndex(1);
    }
}

// =============================================================================
// §3 图表产出链（produceCharts → 三类型 producer；产出集合记录供 §4 频率
// 联动圈定作用域）
// =============================================================================

namespace {

//产出链收尾回抬本面板 dock（应用/显示/研究三链收尾共用）：图表激活链各环同步
//直连——activateChartViewport → sig_requestActivateChartCard → 容器激活 → 回投
//notifyChartViewportActivated → syncTreeSelectionForChart → sig_currentNodeChanged
//→ mInfoWindow::editCurveNode，尾环 activateDock("Info") 把与 NVHToolSet 同标签组
//的「参数」dock 置前盖住本面板（真机缺陷：点"应用"跳到参数页）。链至此已走完，
//此处即最终态：把本面板抬回前台——应用后落点=NVH工具集·结果显示页签（2026-11-17
//用户裁定，图二）；"显示"/研究链同理，面板正被操作不得被产出动作顶掉
void reassertToolSetDock()
{
    if (QCoreApplication::closingDown()) {
        return;
    }
    MechMainWindow::mDockLayoutManager::getInstance()->activateDock(QStringLiteral("NVHToolSet"));
}

//带号科学计数法（Note 行值恒带 +/− 号，spec："+1.16e-06"）
QString formatSci(double value)
{
    const QString text = QString::number(std::fabs(value), 'e', 2);
    return value < 0 ? QStringLiteral("-") + text : QStringLiteral("+") + text;
}

//% 值文本 trim 尾部小数（100.0→"100"、12.5→"12.5"——研究页 Note 首行 D4
//口径，样张 100% 非 100.0%）
QString formatPctTrim(double value)
{
    QString text = QString::number(value, 'f', 1);
    while (text.endsWith(QLatin1Char('0'))) {
        text.chop(1);
    }
    if (text.endsWith(QLatin1Char('.'))) {
        text.chop(1);
    }
    return text;
}

//复用布局清空重建（§3.2 布局二）：删除图表下全部曲线/基准线/注释子树（轴/
//图例/页眉页脚固定子节点与图表本体保留；须处于 beginMacro 宏内随整组撤销）
void clearChartContent(PostCurve::mCurveDocController* controller, int chartId)
{
    auto* doc = controller->document();
    const auto children = doc->childNodeIds(chartId);
    for (int childId : children) {
        const auto type = doc->nodeType(childId);
        if (type == PostCurve::CurveNodeType::Curve
            || type == PostCurve::CurveNodeType::Baseline
            || type == PostCurve::CurveNodeType::Annotation) {
            controller->removeNode(childId);
        }
    }
}

//布局0（当前视图页打开新窗口）落点解析（A0 修复）：落点=当前视图页绑定的
//曲线页（页对齐反查 curvePageIdForViewPage）；无绑定页时新建曲线页并绑到
//当前视图页+页名对齐（同登记链建页分支语义）。返回落点曲线页 id（<0=解析失败）。
//2026-11-11 前的旧逻辑取"当前激活图表所在页"，空页时退回 addPage 产生未绑定
//新页→openChart 携 viewPageId=-1→主窗口新建视图页（页计数 2/1 违背 spec
//"新窗口模式不新建视图页"，真机缺陷 A0）
int resolveCurrentViewPageTarget(PostCurve::mCurveViewportFactory* factory,
    PostCurve::mCurveDocController* controller, const QString& fallbackName)
{
    const int viewPageId = factory->currentViewPageId();
    int pageId = viewPageId >= 0 ? controller->curvePageIdForViewPage(viewPageId) : -1;
    if (pageId < 0)
    {
        pageId = controller->addPage(fallbackName);
        if (viewPageId >= 0)
        {
            controller->setPageViewPageId(pageId, viewPageId);
            const QString name = factory->viewPageName(viewPageId);
            if (!name.isEmpty())
                controller->setPageName(pageId, name);
        }
    }
    return pageId;
}

//研究图产出落点解析（§3.2 同口径）：布局 1=当前激活窗口复用（有内容弹覆盖
//确认，取消返回 false）；0=当前页新窗口；2=新视图页。输出 pageId/reuseChartId
bool resolveResearchPlacement(QWidget* parent, int layout, int* pageId, int* reuseChartId)
{
    auto* factory = PostCurve::mCurveViewportFactory::getInstance();
    auto* controller = PostCurve::mCurveDocController::getInstance();
    *reuseChartId = -1;
    if (layout == 1) {
        *reuseChartId = factory->currentChartId();
        if (*reuseChartId >= 0) {
            const int choice = MechMainWindow::mFramelessMessageBox::warning(parent,
                QObject::tr("提示"),
                QObject::tr("当前窗口已有内容，此操作将覆盖当前内容\n您希望继续吗?"),
                QObject::tr("取消"), QObject::tr("继续"), QString(), 1, 0, true);
            if (choice != 1) {
                return false;
            }
        }
    }
    if (*reuseChartId >= 0) {
        *pageId = controller->document()->parentNodeId(*reuseChartId);
    } else if (layout == 2) {
        *pageId = controller->addPage(QObject::tr("模态贡献量分析"));
    } else {
        *pageId = resolveCurrentViewPageTarget(factory, controller, QObject::tr("模态贡献量分析"));
    }
    return *pageId >= 0;
}

} // namespace

//产出入口（§3.1/3.2）：覆盖确认（布局=当前激活窗口且激活视口有内容时，原型
//01 提示框）→ 布局映射（0 当前页新窗口[默认] 1 当前激活窗口复用清空重建
//2 新视图页）→ beginMacro 组命令内按链产出 → 收尾 activateChartViewport
//统一切换物化。两链拆分（2026-11-13 用户裁定，需求大图 01/06 权威）：
//showChain=false 应用链=只产响应曲线一张；showChain=true 显示链=只产当前
//图表类型一张（旧版两链恒产"响应曲线先行+类型图"两张=多生成一张，作废）。
//返回本次产出 chartId 集合（§4 频率联动作用域），取消/失败返回空
QList<int> mPostNVHContributionPanel::produceCharts(bool showChain)
{
    _producedChartIds.clear();
    _producedAuxChartId = -1;
    _producedAuxChartType = -1;
    if (_model == nullptr || !_model->isLoaded()) {
        return _producedChartIds;
    }
    const PointSelection sel = currentSelection();
    if (!sel.valid()) {
        MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
            tr("请先选择工况/结果类型/响应点ID/矢量分量"));
        return _producedChartIds;
    }

    auto* factory = PostCurve::mCurveViewportFactory::getInstance();
    auto* controller = PostCurve::mCurveDocController::getInstance();

    //覆盖确认：仅"当前激活窗口打开"布局且激活视口有内容时弹（原型 01 提示框；
    //布局 0/2 落新窗口不覆盖既有内容）
    int reuseChartId = -1;
    if (_commonOpts.layout == 1) {
        reuseChartId = factory->currentChartId();
        if (reuseChartId >= 0) {
            const int choice = MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
                tr("当前窗口已有内容，此操作将覆盖当前内容\n您希望继续吗?"),
                tr("取消"), tr("继续"), QString(), 1, 0, true);
            if (choice != 1) {
                return _producedChartIds;
            }
        }
    }

    //落点页解析（§3.2）：布局1=当前激活图表所在页；布局0=当前视图页绑定曲线页
    //（A0 修复，见 resolveCurrentViewPageTarget）；布局2=新建视图页
    factory->setDeferCardActivation(true);
    int pageId = -1;
    if (reuseChartId >= 0) {
        pageId = controller->document()->parentNodeId(reuseChartId);
    } else if (_commonOpts.layout == 2) {
        pageId = controller->addPage(tr("模态贡献量分析"));
    } else {
        pageId = resolveCurrentViewPageTarget(factory, controller, tr("模态贡献量分析"));
    }
    if (reuseChartId >= 0) {
        factory->setPlacementOverride(PostCurve::ChartPlacement::ReuseCurrent);
    }
    controller->undoStack()->beginMacro(tr("模态贡献量分析-生成图表"));

    bool produced = false;
    try {
        if (showChain) {
            //显示链：只产当前图表类型一张（需求大图 06 交互流程——切类型后
            //"显示"仅产出该类型图）。布局1 复用当前窗口：清空后同图重建
            //（kChartType 复位在各 producer 内——clearChartContent 只删子树
            //不清节点属性）
            if (reuseChartId >= 0) {
                clearChartContent(controller, reuseChartId);
            }
            switch (currentChartType()) {
            case 1: {
                const int chartId = producePolarChart(pageId, reuseChartId);
                _producedChartIds << chartId;
                _producedAuxChartId = chartId;
                _producedAuxChartType = 1;
                break;
            }
            case 2:
                _producedChartIds << produce2DLineChart(pageId, reuseChartId);
                break;
            default: {
                const int chartId = produceBarChart(pageId, reuseChartId);
                _producedChartIds << chartId;
                _producedAuxChartId = chartId;
                _producedAuxChartType = 0;
                break;
            }
            }
        } else {
            //应用链：只产响应曲线一张（需求大图 01 权威——"点击'应用'按照
            //设置显示对应曲线"，产出仅响应曲线，不再顺带产当前类型图）
            if (reuseChartId >= 0) {
                //布局1：复用当前激活窗口——同页同图清空重建
                clearChartContent(controller, reuseChartId);
                _producedChartIds << produceResponseCurve(pageId, reuseChartId);
            } else {
                _producedChartIds << produceResponseCurve(pageId, -1);
            }
        }
        produced = true;
    } catch (...) {
        //异常安全：宏闭合防撤销栈卡在半开状态（半途内容随宏整体可撤销）
        controller->undoStack()->endMacro();
        factory->clearPlacementOverride();
        factory->setDeferCardActivation(false);
        throw;
    }
    controller->undoStack()->endMacro();
    factory->clearPlacementOverride();
    factory->setDeferCardActivation(false);

    if (produced && pageId >= 0) {
        if (!_producedChartIds.isEmpty()) {
            //落位收口（chart-viewport-placement 单写者）：先按本次产出集合平铺
            //（切页→最小格数布局→呈现集=本次集合→一次 applyLayout，旧图溢出
            //隐藏），再激活本次首图——首图已在格内，激活换位不触发重排
            //（2026-11-11 前的旧收尾只激活不布局：双图落页后柱状图悬在布局外
            //不可见，二次显示时新卡以建卡几何浮于旧图上）
            factory->presentProducedCharts(pageId, _producedChartIds);
            factory->activateChartViewport(_producedChartIds.first());
        } else {
            factory->activatePageFirstChart(pageId);
        }
        if (_producedAuxChartType == 1) {
            applyPolarChainForm(_producedAuxChartId);   //§5.3 形态随物化补发（视图态）
        }
        reassertToolSetDock();   //激活链尾环置前「参数」dock 的回抬（本面板保持前台）
    }
    return _producedChartIds;
}

//§4 特定频率红线三联动漏斗（spec：任一路变化，输入框/滑条/红线一致更新，
//跨产出视口同频状态）：最近频率点归一 → 三路状态回写 → 红线下发产出集合内
//全部视口。图表内容不随频率重绘（2026-11-11 用户需求澄清：新增视口/替换/
//增加只在"显示"链；辅助图内容随下一次"显示"按当期频率再产出）
void mPostNVHContributionPanel::applyFrequencyState(double freq)
{
    if (_model == nullptr || !_model->isLoaded()) {
        return;
    }
    const PointSelection sel = currentSelection();
    if (!sel.valid()) {
        return;
    }
    const auto freqs = _model->frequencies(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component);
    if (freqs.empty()) {
        return;
    }
    //最近频率点归一（spec 四舍五入规则；画布点选已在视图侧吸附数据点，二次
    //归一幂等）
    const double target = ModalContributionModel::nearestFrequency(freqs, freq);

    //三路回写①②：输入框/滑条（信号阻塞防回环）
    {
        const QSignalBlocker blockerEdit(_freqEdit);
        _freqEdit->setText(QString::number(target, 'g', 6));
    }
    for (std::size_t i = 0; i < freqs.size(); ++i) {
        if (freqs[i] == target) {
            const QSignalBlocker blockerSlider(_freqSlider);
            _freqSlider->setValue(static_cast<int>(i));
            break;
        }
    }

    //三路回写③：红线视图态下发（不进命令栈，design D4）——本功能产出集合
    //内全部视口同频；视图未物化时 chartView 返回空跳过（卡片激活物化后由
    //rebuild 重放/下次下发补齐）
    auto* factory = PostCurve::mCurveViewportFactory::getInstance();
    for (int chartId : _producedChartIds) {
        auto* view = factory->chartView(chartId);
        if (view != nullptr) {
            view->setFrequencyCursor(target);
        }
    }

    //柱状/极坐标辅助图不再随频率重建（2026-11-11 用户需求澄清：改变特定频率
    //只动红线与输入框/滑条读数，图表内容不重绘；新增视口/替换/增加只发生在
    //"显示"链——原"随频率清空重建"整图重绘已按澄清移除，辅助图内容随下一次
    //"显示"按当期频率再产出）
}

//§4.3 视图→面板（design D4）：画布左键频率拾取（工厂中继带 chartId，X 已在
//视图侧吸附首条可见曲线最近数据点=文件频率点）；仅处理本功能产出集合内事件
void mPostNVHContributionPanel::onCanvasFrequencyPicked(int chartId, double xValue)
{
    if (!_producedChartIds.contains(chartId)) {
        return;
    }
    applyFrequencyState(xValue);
}

//§6.1 显示选项对话框：模态 exec 回显本语境存量档；确定经双结果回写语境缩放
//+公共行（应用到后续生成——已产出视口不自动重产，重产经"显示"或"应用"链路），
//取消不动。五语境路由（2026-11-17 用户裁定 context-display-options-state）：
//加载=NVHCtx_Load、研究=NVHCtx_Research、结果显示页按当前图表类型三档——
//缩放状态按语境独立存取（互不覆盖），公共行（显示/加权/单位/绘图布局/设为
//全局）仍单份项级同步
void mPostNVHContributionPanel::onDisplayOptions()
{
    NVHDisplayCtx ctx = NVHCtx_Load;
    if (sender() == _rdOptionsBtn) {
        const int t = currentChartType();
        ctx = (t == 1) ? NVHCtx_ShowPolar : (t == 2) ? NVHCtx_Show2D
                                                     : NVHCtx_ShowBar;
    } else if (sender() == _rOptionsBtn) {
        ctx = NVHCtx_Research;
    }
    mNVHDisplayOptionsDlg dlg(ctx, _ctxScale[ctx], _commonOpts, this);
    if (dlg.exec() == QDialog::Accepted) {
        _ctxScale[ctx] = dlg.ctxScaleResult();
        _commonOpts = dlg.commonResult();
        //极坐标已产出且显示行=首尾相接态变化时即时补发（视图态不进命令栈，
        //同 applyPolarChainForm 通道；其余选项按 spec 应用到后续生成）
        if (_producedAuxChartType == 1 && _producedAuxChartId >= 0) {
            applyPolarChainForm(_producedAuxChartId);
        }
    }
}

//§3.3 响应曲线 w1 先行（三类型共用）：显示相位关=单幅值 XY；开=Phase+Magnitude
//双面板复数图（chartType=2，引擎 reconcile 补 Y2"Phase"）。图表头默认四行
//（spec），Y 轴标题/刻度随显示选项（默认对数——spec 响应曲线先行）
int mPostNVHContributionPanel::produceResponseCurve(int pageId, int reuseChartId)
{
    auto* controller = PostCurve::mCurveDocController::getInstance();
    const PointSelection sel = currentSelection();

    const auto freqs = _model->frequencies(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component);
    if (freqs.empty()) {
        return reuseChartId >= 0 ? reuseChartId : -1;
    }
    const auto series = _model->responseSeries(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component, freqs.front(), freqs.back());
    if (!series.has_value() || series->frequencies.empty()) {
        return reuseChartId >= 0 ? reuseChartId : -1;
    }

    const int chartId = reuseChartId >= 0 ? reuseChartId : controller->addChart(pageId);
    //落型按显示相位（复用窗口先清空后须复位类型——上一批可能为柱状/极坐标；
    //kChartType 宏清注释，此刻尚无注释，安全）
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kChartType,
        _commonOpts.displayPhase ? 2 : 0);

    QVector<double> xData;
    xData.reserve(static_cast<int>(series->frequencies.size()));
    for (double freqValue : series->frequencies) {
        xData << freqValue;
    }
    QVector<double> magData;
    QVector<double> phaseData;
    magData.reserve(xData.size());
    phaseData.reserve(xData.size());
    for (const auto& value : series->total) {
        magData << std::abs(value);
        phaseData << ModalContributionModel::phaseDegrees(value);
    }

    const QString gridText = QString::number(sel.gridId);
    //分量 pch 原名（T3/R1…，2026-11-16 裁定④：不再经 componentDisplayName
    //映射 X/Z/RX 显示名——曲线名/图例/图表头行 3/来源参数一致；该函数保留
    //给选择树显示）
    const QString compName = QString::fromStdString(sel.component);
    const QString curveName = tr("Node %1/%2Structure Mode - Response @ %3")
        .arg(gridText, compName, gridText);

    if (_commonOpts.displayPhase) {
        //Phase+Magnitude 双面板复数图（上面板幅值/下面板相位，X 共用）
        const int curveId = controller->addComplexCurve(chartId, curveName);
        controller->setComplexSeriesData(curveId, xData, magData, phaseData);
        controller->setComplexSeriesSource(curveId, _filePathEdit->text(),
            QStringLiteral("Subcase %1").arg(sel.subcaseId),
            QString::fromStdString(sel.resultType),
            QStringLiteral("Node %1").arg(sel.gridId),
            compName, compName, 0);
    } else {
        //单幅值 XY 曲线（spec：显示相位关闭时）
        const int curveId = controller->addCurve(chartId, curveName);
        controller->setSeriesData(curveId, xData, magData);
        controller->setSeriesSource(curveId, _filePathEdit->text(),
            QStringLiteral("Subcase %1").arg(sel.subcaseId),
            QString::fromStdString(sel.resultType),
            QStringLiteral("Node %1").arg(sel.gridId), compName);
    }

    //轴标题/刻度随显示选项
    const auto children = controller->document()->childNodeIds(chartId);
    for (int childId : children) {
        const auto type = controller->document()->nodeType(childId);
        if (type == PostCurve::CurveNodeType::XAxis) {
            //X 轴形态（2026-11-16 裁定③④）：自动档（kTickCount=0 走引擎内
            //自动档）+ 定点 2 位小数 + 标题 "Frequency"（去 (Hz) 后缀）
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle, tr("Frequency"));
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTickCount, 0.0);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kLabelFormat, 2);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kPrecision, 2);
        } else if (type == PostCurve::CurveNodeType::YAxis) {
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle, responseAxisTitle());
            controller->setNodeProperty(childId, PostCurve::CurveProp::kScaleMode, _ctxScale[NVHCtx_Load].axisScaleMode());
            controller->setNodeProperty(childId, PostCurve::CurveProp::kDbRef, _ctxScale[NVHCtx_Load].axisDbRef());
            controller->setNodeProperty(childId, PostCurve::CurveProp::kWeightMode, _commonOpts.axisWeightMode());
            //Y 对数档每十倍刻度数 1（2026-11-16 裁定②：原型每十倍程 1 主刻度；
            //全局默认 5 不动，极坐标/柱状图 D25 口径不受扰）
            controller->setNodeProperty(childId, PostCurve::CurveProp::kDecadeTickCount, 1);
        }
    }

    //图表头默认四行（spec 3.3：Subcase / Response 行 / Node 行 / 文件路径）。
    //行 1 结果类型描述（2026-11-16 裁定④）：PFMODE STRUCTURE 参与因子
    //（ITEM=MODAL DISPLACEMENT，simplified 归一+不区分大小写比对）映射参考
    //工具内部命名；其余结果类型回退 "Subcase <ID>" 现行为
    const bool structureParticipation = QString::fromStdString(sel.resultType)
        .simplified().compare(QStringLiteral("MODAL DISPLACEMENT"),
            Qt::CaseInsensitive) == 0;
    const QString subcaseLine = structureParticipation
        ? tr("Subcase %1 - System Structure Modal Participation to Structure Grid (Global)")
            .arg(QString::number(sel.subcaseId))
        : QStringLiteral("Subcase %1").arg(sel.subcaseId);
    const QString headerText = QStringLiteral("\n%1\nResponse @ %2 (Structure Mode)"
        "\nNode %3/%4Structure Mode Participation\n%5")
        .arg(subcaseLine, pointHeaderToken(sel), pointHeaderToken(sel), compName,
            _filePathEdit->text());
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderText, headerText);
    //行 3/4 显式三级字体（fix 6.1 同通道收口：响应曲线 w1 与其余产出图同一
    //QFont() 判据坑，修法同柱状图 1577）
    QFont headerL3Font = QApplication::font();
    headerL3Font.setPointSizeF(qBound(7.0, headerL3Font.pointSizeF() - 1.0, 12.0));
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderFont3, headerL3Font);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderVisible, true);
    //需求02表头形态：左对齐 + 第 2 行强调（加粗+放大）
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderAlign, 0);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderEmphLine, 1);
    return chartId;
}

//§3.4 2D线图：模态贡献量频率曲线族（排序前 N）+ Response 蓝粗线 + Others 红虚线
//+ Ranked By 独立粉框标签（spec：标题行无频率后缀；频率范围行接线在此消费；
//Y 轴随显示选项——默认对数）
int mPostNVHContributionPanel::produce2DLineChart(int pageId, int reuseChartId)
{
    auto* controller = PostCurve::mCurveDocController::getInstance();
    const PointSelection sel = currentSelection();
    const bool projected = _complexCombo->currentIndex() == 0;   //投影绝对值 / 幅值-相位

    const auto fileFreqs = _model->frequencies(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component);
    if (fileFreqs.empty()) {
        return reuseChartId >= 0 ? reuseChartId : -1;
    }
    //频率范围（页签输入；非法/空缺回退文件全范围）
    double fMin = fileFreqs.front();
    double fMax = fileFreqs.back();
    const double startInput = _freqStartEdit->text().toDouble();
    const double endInput = _freqEndEdit->text().toDouble();
    if (startInput < endInput) {
        fMin = startInput;
        fMax = endInput;
    }

    const auto series = _model->responseSeries(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component, fMin, fMax);
    if (!series.has_value() || series->frequencies.empty()) {
        return reuseChartId >= 0 ? reuseChartId : -1;
    }
    const auto ranking = ModalContributionModel::rssRanking(series.value(),
        projected ? ModalContributionModel::SortMetric::ProjectedAbsRss
                  : ModalContributionModel::SortMetric::MagnitudeRss);
    if (ranking.empty()) {
        return reuseChartId >= 0 ? reuseChartId : -1;
    }
    const int topN = qBound(1, _topNCombo->currentIndex() + 1,
        static_cast<int>(ranking.size()));

    //逐模态量值（投影绝对值=|proj|；幅值=|mode|——复数分量选项集 spec）
    auto metricValue = [projected](const std::complex<double>& total,
        const std::complex<double>& mode) -> double
    {
        return projected
            ? std::abs(ModalContributionModel::projectedContribution(total, mode))
            : ModalContributionModel::magnitude(mode);
    };

    const int chartId = reuseChartId >= 0 ? reuseChartId : controller->addChart(pageId);
    if (reuseChartId >= 0) {
        //复用窗口复位 XY 类型（clearChartContent 只删子树不清节点属性——
        //上一批可能为柱状/极坐标；响应曲线"复用先复位类型"同款道理）
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kChartType, 0);
    }
    QVector<double> xData;
    xData.reserve(static_cast<int>(series->frequencies.size()));
    for (double freqValue : series->frequencies) {
        xData << freqValue;
    }
    const auto modeTable = _model->modeFrequencies(sel.subcaseId);
    const auto wheel = PostCurve::curveDefaultColorWheel();

    //模态曲线族（RSS 降序前 N）
    QSet<int> topModes;
    int colorIdx = 0;
    int rank = 0;
    for (const auto& pair : ranking) {
        if (rank++ >= topN) {
            break;
        }
        topModes.insert(pair.first);
        const auto& values = series->modes.at(pair.first);
        QVector<double> yData;
        yData.reserve(xData.size());
        for (std::size_t i = 0; i < series->frequencies.size(); ++i) {
            yData << metricValue(series->total[i], values[i]);
        }
        const double modeFreq = modeTable.count(pair.first) > 0
            ? modeTable.at(pair.first) : 0.0;
        const int curveId = controller->addCurve(chartId,
            tr("Mode %1 : %2 Hz").arg(QString::number(pair.first),
                QString::number(modeFreq, 'f', 2)));
        controller->setSeriesData(curveId, xData, yData);
        controller->setSeriesSource(curveId, _filePathEdit->text(),
            QStringLiteral("Subcase %1").arg(sel.subcaseId),
            QString::fromStdString(sel.resultType),
            QStringLiteral("Node %1").arg(sel.gridId),
            QString::fromStdString(sel.component));
        controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
            wheel[colorIdx++ % wheel.size()]);
    }

    //Others 红虚线（未入选模态量值逐点和，spec 3.4；2026-11-17 第六批①裁定：
    //贡献族 2D 显示链只产贡献族一张——旧 Response 蓝粗线块已删，Response 全量
    //曲线归 Response 图产出）
    {
        QVector<double> yData(xData.size(), 0.0);
        for (const auto& entry : series->modes) {
            if (topModes.contains(entry.first)) {
                continue;
            }
            const auto& values = entry.second;
            for (std::size_t i = 0; i < series->frequencies.size(); ++i) {
                yData[static_cast<int>(i)] += metricValue(series->total[i], values[i]);
            }
        }
        const int curveId = controller->addCurve(chartId, tr("Others"));
        controller->setSeriesData(curveId, xData, yData);
        controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor, QColor(255, 0, 0));
        controller->setNodeProperty(curveId, PostCurve::CurveProp::kLineStyle, 1);   //虚线
    }

    //X 轴列表标注法值列表（2026-11-17 第六批③/C2：贡献族 X 轴主刻度=频率点
    //全集界内成员——tickGenMode 2 + tickValues 下发，替换 11-16 裁定③自动档）
    QVector<double> freqTickList;
    freqTickList.reserve(static_cast<int>(series->frequencies.size()));
    for (double f : series->frequencies) {
        freqTickList << f;
    }

    //轴标题/刻度（X=频率 Hz；Y=参与量随显示选项）
    const auto children = controller->document()->childNodeIds(chartId);
    for (int childId : children) {
        const auto type = controller->document()->nodeType(childId);
        if (type == PostCurve::CurveNodeType::XAxis) {
            //X 轴形态（2026-11-16 裁定③④定点 2 位 + "Frequency" 维持；2026-11-17
            //第六批③：主刻度改频率点列表标注法 tickGenMode 2+值列表下发）
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle, tr("Frequency"));
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTickGenMode, 2);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTickValues,
                QVariant::fromValue(freqTickList));
            controller->setNodeProperty(childId, PostCurve::CurveProp::kLabelFormat, 2);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kPrecision, 2);
        } else if (type == PostCurve::CurveNodeType::YAxis) {
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle, participationAxisTitle(NVHCtx_Show2D));
            controller->setNodeProperty(childId, PostCurve::CurveProp::kScaleMode, _ctxScale[NVHCtx_Show2D].axisScaleMode());
            controller->setNodeProperty(childId, PostCurve::CurveProp::kDbRef, _ctxScale[NVHCtx_Show2D].axisDbRef());
            controller->setNodeProperty(childId, PostCurve::CurveProp::kWeightMode, _commonOpts.axisWeightMode());
            //Y 对数档每十倍刻度数 1（2026-11-16 裁定②；全局默认 5 不动）
            controller->setNodeProperty(childId, PostCurve::CurveProp::kDecadeTickCount, 1);
        }
    }

    //Ranked By 粉框标签（视图类注释；2026-11-17 第六批 C3 裁定：Note 边框色
    //随档位——投影档品红(255,0,255)/幅值档青(0,255,255)）
    const int noteId = controller->addAnnotation(chartId);
    controller->setNodeProperty(noteId, PostCurve::CurveProp::kText,
        tr("Ranked By - %1").arg(sortMetricEnglishName(true)));
    controller->setNodeProperty(noteId, PostCurve::CurveProp::kAttachMode, 1);
    controller->setNodeProperty(noteId, PostCurve::CurveProp::kBorderMode, 1);
    controller->setNodeProperty(noteId, PostCurve::CurveProp::kBorderColor,
        projected ? QColor(255, 0, 255) : QColor(0, 255, 255));
    controller->setNodeProperty(noteId, PostCurve::CurveProp::kAutoPos, true);
    controller->setNodeProperty(noteId, PostCurve::CurveProp::kAutoAnchor, 4);

    //图表头四行（标题行无频率后缀，随复数分量两形态——spec 3.4；响应点名称框
    //非空时行内 ID 令牌以名称替换——2026-11-16 用户裁定，pointHeaderToken）
    //分量 pch 原名（2026-11-16 裁定④：图表头行 3 不再经 componentDisplayName
    //映射 X/Z/RX 显示名；函数保留给选择树显示）
    const QString compName = QString::fromStdString(sel.component);
    const QString pointTok = pointHeaderToken(sel);
    const QString titleLine = projected
        ? tr("Projected Participation @ %1 (Structure Mode)").arg(pointTok)
        : tr("Participation Magnitude @ %1 (Structure Mode)").arg(pointTok);
    const QString headerText = QStringLiteral("Subcase %1\n%2"
        "\nNode %3/%4Structure Mode Participation\n%5")
        .arg(QString::number(sel.subcaseId), titleLine, pointTok, compName,
            _filePathEdit->text());
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderText, headerText);
    //行 3/4 显式三级字体（fix 6.1 同通道收口，修法同柱状图 1577）
    QFont headerL3Font = QApplication::font();
    headerL3Font.setPointSizeF(qBound(7.0, headerL3Font.pointSizeF() - 1.0, 12.0));
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderFont3, headerL3Font);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderVisible, true);
    //需求02表头形态：居中 + 第 2 行强调（加粗+放大——kHeaderAlign 0=居中，
    //2026-11-11 晚用户裁定恢复原型居中形态，"左排"措辞作废）
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderAlign, 0);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderEmphLine, 1);
    return chartId;
}

//§3.5/3.6 柱状图：单柱曲线 M+2 柱（Response+前 N+Others；引擎口径一侧一曲线
//多柱，类目签挂曲线 setSeriesTickLabels——裁定见台账）+ 投影组合附加物（4 条
//Datum 树节点（隐藏不画线）+ In-Phase 顶部红字注解 + Note 贡献列表粉框注释）。
//缩放档集 {Linear,%Plot}（2026-11-11 需求 02/05 裁定+design D12）：%Plot=数据级
//归一化（y=贡献/总响应×100，响应恒 100，轴 Linear——design D6）
int mPostNVHContributionPanel::produceBarChart(int pageId, int reuseChartId)
{
    auto* controller = PostCurve::mCurveDocController::getInstance();
    const PointSelection sel = currentSelection();
    const bool projected = _complexCombo->currentIndex() == 0;   //投影 / 幅值
    //有效档：柱状图档集仅 {Linear,%Plot}（语境存量恒在域内——折算删除，
    //context-display-options-state；design D12 档集语义不变）
    const bool percentPlot = _ctxScale[NVHCtx_ShowBar].scale != NVHScale_Linear;

    const auto freqs = _model->frequencies(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component);
    if (freqs.empty()) {
        return -1;
    }
    const double targetFreq = ModalContributionModel::nearestFrequency(freqs,
        _freqEdit->text().toDouble());
    const auto point = _model->pointData(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component, targetFreq);
    if (!point.has_value()) {
        return -1;
    }
    //排序按投影绝对值（降序）；柱值带符号投影——含负值柱（spec 场景 79 行）
    const auto ranking = ModalContributionModel::rankAtPoint(point.value(),
        projected ? ModalContributionModel::SortMetric::ProjectedAbs
                  : ModalContributionModel::SortMetric::Magnitude);
    if (ranking.empty()) {
        return -1;
    }
    const int topN = qBound(1, _topNCombo->currentIndex() + 1,
        static_cast<int>(ranking.size()));
    const auto modeTable = _model->modeFrequencies(sel.subcaseId);

    //量值口径（投影=带符号投影；幅值=模态幅值）
    auto metricOf = [projected](const std::complex<double>& total,
        const std::complex<double>& mode) -> double
    {
        return projected
            ? ModalContributionModel::projectedContribution(total, mode)
            : ModalContributionModel::magnitude(mode);
    };
    const double totalAbs = std::abs(point->totalResponse);
    //%Plot=贡献/总响应×100（design D6：与 Note 占比段同分母；响应恒 100%——
    //需求 05 Note 数值核对：模态投影占比求和恰为 100）
    auto valueOf = [percentPlot, totalAbs](double raw) -> double
    {
        return (percentPlot && totalAbs != 0.0) ? raw / totalAbs * 100.0 : raw;
    };
    const double responseValue = valueOf(totalAbs);

    const int chartId = reuseChartId >= 0 ? reuseChartId : controller->addChart(pageId);
    //落型柱状（kChartType 宏清注释——须在基准线/注释创建之前，spec 顺序语义）
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kChartType, 1);

    //画布标题（spec 83 行：投影/幅值两形态；落地=kName 视口标题，待真机复核）
    const QString titleLine = projected
        ? tr("Projected Participation @ %1 (Frequency = %2 Hz) (Structure Mode)")
            .arg(QString::number(sel.gridId), QString::number(targetFreq, 'f', 2))
        : tr("Participation Magnitude @ %1 (Frequency = %2 Hz) (Structure Mode)")
            .arg(QString::number(sel.gridId), QString::number(targetFreq, 'f', 2));
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kName, titleLine);

    //单柱曲线（类目 Response+前 N+Others；x=0 基索引约定，类目签挂曲线）
    const int curveId = controller->addBarCurve(chartId, titleLine);
    QStringList tickLabels;
    QVector<double> xData;
    QVector<double> yData;
    tickLabels << tr("Response");
    xData << 0.0;
    yData << responseValue;
    QSet<int> topModes;
    int rank = 0;
    for (const auto& pair : ranking) {
        if (rank++ >= topN) {
            break;
        }
        topModes.insert(pair.first);
        tickLabels << tr("Mode %1").arg(pair.first);
        xData << static_cast<double>(xData.size());
        yData << valueOf(metricOf(point->totalResponse, point->modeContributions.at(pair.first)));
    }
    double othersValue = 0.0;
    for (const auto& entry : point->modeContributions) {
        if (!topModes.contains(entry.first)) {
            othersValue += metricOf(point->totalResponse, entry.second);
        }
    }
    tickLabels << tr("Others");
    xData << static_cast<double>(xData.size());
    yData << valueOf(othersValue);

    controller->setSeriesData(curveId, xData, yData);
    controller->setSeriesTickLabels(curveId, tickLabels);
    controller->setSeriesSource(curveId, _filePathEdit->text(),
        QStringLiteral("Subcase %1").arg(sel.subcaseId),
        QString::fromStdString(sel.resultType),
        QStringLiteral("Node %1").arg(sel.gridId),
        QString::fromStdString(sel.component));

    //Y 自动域（design D2，2026-11-17 裁定商软口径）：柱值全域恰域 + 固定步长
    //主刻度，Datum 线位与域边重合的视觉基准（两档共用；幅值档零基线）
    const auto yDomain = ModalContributionModel::barYAutoDomain(yData, projected);

    //轴标题/刻度（X="Contributors"，spec；Y=参与量随显示选项）
    const auto children = controller->document()->childNodeIds(chartId);
    for (int childId : children) {
        const auto type = controller->document()->nodeType(childId);
        if (type == PostCurve::CurveNodeType::XAxis) {
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle, tr("Contributors"));
            //类目标签斜排约 30°（kCategoryLabelAlign 3；需求 05 X 轴形态，D10。
            //渲染端 applyAxis 下轴消费该档，其余图表/轴自动回退水平）
            controller->setNodeProperty(childId, PostCurve::CurveProp::kCategoryLabelAlign, 3);
        } else if (type == PostCurve::CurveNodeType::YAxis) {
            //柱状图两档均线性轴（design D6）；轴标题按折算后的有效档——不走
            //participationAxisTitle()（其读面板存量档，Log/dB 折算后语义不符）
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle,
                percentPlot ? tr("% Participation")
                            : tr("Participation (Scale=linear)"));
            controller->setNodeProperty(childId, PostCurve::CurveProp::kScaleMode, 0);
            //Y 自动域下发（design D2）：轴自动缩放关、域恰为 barYAutoDomain 结果、
            //主刻度固定步长（tickGenMode 1）——Datum 域边线与刻度端点重合
            controller->setNodeProperty(childId, PostCurve::CurveProp::kAutoScale, false);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kMin, yDomain.min);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kMax, yDomain.max);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTickGenMode, 1);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kMajorStep, yDomain.step);
        } else if (type == PostCurve::CurveNodeType::Legend) {
            //需求 05 权威图柱状图绘图区无图例（树节点保留；2026-11-11 用户裁定）
            controller->setNodeProperty(childId, PostCurve::CurveProp::kVisible, false);
        }
    }

    if (projected) {
        //—— 投影组合附加物（§3.6，spec：投影时早于本段生成；幅值组合无）——
        //4 条 Datum 基准树节点（Datum 1~4 自顶向下）。2026-11-17 第六批②A 裁定：
        //四线可见渲染，红/灰/蓝/灰；Datum1/2=域顶、Datum3/4=域底（Position 绑
        //域边，随域缩放联动）；仅 Datum1/3 带标签（In-Phase/Out-of-Phase，需求 03
        //批注"定位到基准线下方"→ kLabelPos 7=Lower Center），中间两条无标签。
        //幅值组合不生成（if (projected) 守卫维持，2026-11-11 裁定不变）
        const struct DatumSpec { double value; QColor color; const char* label; } datums[4] = {
            { yDomain.max, QColor(255, 0, 0),   "In-Phase" },
            { yDomain.max, QColor(128, 128, 128), "" },
            { yDomain.min, QColor(0, 0, 255),   "Out-of-Phase" },
            { yDomain.min, QColor(128, 128, 128), "" },
        };
        for (const auto& datum : datums) {
            const int baseId = controller->addBaseline(chartId);
            if (datum.label[0] != '\0') {
                controller->setNodeProperty(baseId, PostCurve::CurveProp::kLabel,
                    tr(datum.label));
                controller->setNodeProperty(baseId, PostCurve::CurveProp::kLabelPos, 7);
            }
            controller->setNodeProperty(baseId, PostCurve::CurveProp::kOrientation, 0);   //横向
            controller->setNodeProperty(baseId, PostCurve::CurveProp::kValue, datum.value);
            controller->setNodeProperty(baseId, PostCurve::CurveProp::kColor, datum.color);
            controller->setNodeProperty(baseId, PostCurve::CurveProp::kVisible, true);
        }

        //Note 贡献列表注释（粉框；行 "<±值> : <名称>[ : <频率 Hz>][ : [<±占比%>]]"——
        //值列=与柱同源显示口径（%Plot=归一化百分比/Linear=原始值，valueOf 同源）；
        //占比段带符号仅 %Plot 两位小数（需求 05：[-1245.43 %]）、Response/Others
        //无频率段、模态行含频率段、末行 Ranked By 英文名）
        //（旧 In-Phase 独立注解已删——2026-11-17 第六批②A：In-Phase 改走 Datum1
        //标签通道 kLabelPos 7，见上方 Datum 表）
        auto pctSeg = [percentPlot, totalAbs](double rawValue) -> QString
        {
            if (!percentPlot || totalAbs == 0.0) {
                return QString();
            }
            return QStringLiteral(" : [%1 %]")
                .arg(QString::number(rawValue / totalAbs * 100.0, 'f', 2));
        };
        QString noteText = QStringLiteral("%1 : Response%2")
            .arg(formatSci(valueOf(totalAbs)), pctSeg(totalAbs));
        int noteRank = 0;
        for (const auto& pair : ranking) {
            if (noteRank++ >= topN) {
                break;
            }
            const double rawValue = metricOf(point->totalResponse,
                point->modeContributions.at(pair.first));
            const double modeFreq = modeTable.count(pair.first) > 0
                ? modeTable.at(pair.first) : 0.0;
            noteText += QStringLiteral("\n%1 : Mode %2 : %3 Hz%4")
                .arg(formatSci(valueOf(rawValue)), QString::number(pair.first),
                    QString::number(modeFreq, 'f', 2), pctSeg(rawValue));
        }
        noteText += QStringLiteral("\n%1 : Others%2")
            .arg(formatSci(valueOf(othersValue)), pctSeg(othersValue));
        noteText += QStringLiteral("\nRanked By - %1").arg(sortMetricEnglishName(false));
        const int noteId = controller->addAnnotation(chartId);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kText, noteText);
        //kAttachMode 1=视图类（0=曲线类会吸附最近柱顶画引出线——真机粉色斜线
        //即此误配；响应图 Note 同参数既有先例 1961 行）
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kAttachMode, 1);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kBorderMode, 1);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kBorderColor, QColor(255, 0, 255));
        //Note 定位：显式数据坐标（x 右缘外半槽+全域中值，钳位兜底——autoPos 中心锚
        //不钳画布系 D7 缺陷根因，故改显式 kPosX/kPosY，另见 PlotView 钳位修复）
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kPosX, xData.last() + 0.5);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kPosY,
            (yDomain.min + yDomain.max) / 2.0);   //域中值（vMin/vMax 已由 yDomain 取代）
    }

    //图表头四行（需求 05 定稿：行 1=固定模板文案（HyperGraph 模板——2026-11-11
    //用户裁定，非 pch $TITLE）；行 2=标题行（titleLine 单源兼作 kName/柱曲线名）；
    //行 3="Node <ID>/<原始分量>Structure Mode Participation"（原文拼接无空格、
    //无 Vector type 段——需求 05 实拍）；行 4=文件路径。对齐=居中+第 2 行强调）
    const QString compRaw = QString::fromStdString(sel.component);
    //表头行 2 名称形态：titleLine 兼作 kName/柱曲线名（维持 ID 口径），表头行
    //内 ID 令牌按响应点名称替换（2026-11-16 用户裁定）
    const QString pointTok = pointHeaderToken(sel);
    const QString headerTitle = projected
        ? tr("Projected Participation @ %1 (Frequency = %2 Hz) (Structure Mode)")
            .arg(pointTok, QString::number(targetFreq, 'f', 2))
        : tr("Participation Magnitude @ %1 (Frequency = %2 Hz) (Structure Mode)")
            .arg(pointTok, QString::number(targetFreq, 'f', 2));
    const QString headerText = tr("Subcase %1 - System Structure Modal Participation to Structure Grid (Global)\n"
        "%2\nNode %3/%4Structure Mode Participation\n%5")
        .arg(QString::number(sel.subcaseId), headerTitle, pointTok, compRaw,
            _filePathEdit->text());
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderText, headerText);
    //行 3/4（Node 行/文件路径）走三级字体：渲染层"未选"判据=chosen==QFont()，
    //而 QFont() 本身解析为应用默认字体（9pt）——传应用字体与其相等，仍被判
    //"未选"按一级-8pt 缩至 ~1pt 不可读（2026-11-11 真机插桩实锤 font3Pt=9.00
    //且 font3IsDefault=1）。显式 8pt（≠默认 9pt 判据成立）+ 与行 1 同族，行 3/4
    //呈可读小字（需求 05 形态）
    QFont headerL3Font = QApplication::font();
    headerL3Font.setPointSizeF(qBound(7.0, headerL3Font.pointSizeF() - 1.0, 12.0));
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderFont3, headerL3Font);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderVisible, true);
    //需求 05 表头形态：居中 + 第 2 行强调（kHeaderAlign 0=居中，2026-11-11 晚
    //裁定恢复原型居中形态；kHeaderEmphLine 1=第 2 行加粗放大）
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderAlign, 0);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderEmphLine, 1);

    //D2：成柱成功路径记录成柱模态集并启用第 4 钮（点击选中成柱模态行；
    //loadFile 换文件时清空+禁用——slot_loadFile 级联处）
    _barChartBarModes = topModes;
    if (_selBarBtn != nullptr) {
        _selBarBtn->setEnabled(true);
    }
    return chartId;
}

//§3.7 极坐标图：模态向量（θ=相位 r=幅值）复数三壳曲线族占位产出（chartType=3
//落型，渲染走 mCurvePolarFrameItem/mCurvePolarSeriesItem 自管图元）。
//曲线树/图例显示名 "<值> :Mode N :<频率> Hz"（原图 04 图例逐字：冒号紧贴后词、
//值不带 +/− 号）；图例序=Response 居首（2026-11-12 裁定）→排序模态→Others；
//Ranked By 入图表头第三行
int mPostNVHContributionPanel::producePolarChart(int pageId, int reuseChartId)
{
    auto* controller = PostCurve::mCurveDocController::getInstance();
    const PointSelection sel = currentSelection();

    const auto freqs = _model->frequencies(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component);
    if (freqs.empty()) {
        return -1;
    }
    const double targetFreq = ModalContributionModel::nearestFrequency(freqs,
        _freqEdit->text().toDouble());
    const auto point = _model->pointData(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component, targetFreq);
    if (!point.has_value()) {
        return -1;
    }
    const bool projected = _complexCombo->currentIndex() == 0;
    const auto ranking = ModalContributionModel::rankAtPoint(point.value(),
        projected ? ModalContributionModel::SortMetric::ProjectedAbs
                  : ModalContributionModel::SortMetric::Magnitude);
    if (ranking.empty()) {
        return -1;
    }
    const int topN = qBound(1, _topNCombo->currentIndex() + 1,
        static_cast<int>(ranking.size()));
    const auto modeTable = _model->modeFrequencies(sel.subcaseId);

    //量值/向量口径（D27 终审 2026-11-13 用户裁定：画线向量恒=原始复向量自身
    //相位——需求 04 大图四图 PCA 无圆心直线拟合终审实证，§02/§03 两组合画线
    //均自身相位形态、投影锁相在原型零存在；组合只改图例值/排序/表头口径。
    //metricVector（D20③ 相位回装）自此仅余 Others 投影代数残差口径使用，
    //Others 公式=D27 d2 另轮裁定本轮不动；D24"统一投影"与前版"画线随组合"
    //一并作废）
    const double totalAbs = std::abs(point->totalResponse);
    auto metricVector = [projected, totalAbs](const std::complex<double>& total,
        const std::complex<double>& mode) -> std::complex<double>
    {
        if (!projected) {
            return mode;
        }
        if (totalAbs == 0.0) {
            return std::complex<double>(0.0, 0.0);
        }
        return ModalContributionModel::projectedContribution(total, mode)
            * (total / totalAbs);
    };
    //极坐标树/图例值不带 +/− 号（原图图例逐字 "7.65e-05"；柱状 Note 值恒带号
    //系 spec 专门要求走 formatSci，两者分用）
    auto formatPolarValue = [](double value) -> QString
    {
        return QString::number(std::fabs(value), 'e', 2);
    };

    const int chartId = reuseChartId >= 0 ? reuseChartId : controller->addChart(pageId);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kChartType, 3);

    //复数三壳曲线族（x=目标频率（D25 A/B/C 计权 weightGain 入参）/ ym=幅值 /
    //yp=相位；极坐标渲染 §5 消费）
    auto addPolarCurve = [this, controller, chartId, targetFreq](const QString& name,
        const std::complex<double>& value) -> int
    {
        const int curveId = controller->addComplexCurve(chartId, name);
        QVector<double> xData(1, targetFreq);
        QVector<double> magData(1, std::abs(value));
        QVector<double> phaseData(1, ModalContributionModel::phaseDegrees(value));
        controller->setComplexSeriesData(curveId, xData, magData, phaseData);
        controller->setComplexSeriesSource(curveId, _filePathEdit->text(),
            QStringLiteral("Subcase %1").arg(currentSubcaseId()),
            _resultTypeCombo->currentText(),
            QStringLiteral("Node %1").arg(_pointIdCombo->currentText().toInt()),
            _componentCombo->currentText(), _componentCombo->currentText(), 0);
        return curveId;
    };

    const auto wheel = PostCurve::curveDefaultColorWheel();
    int colorIdx = 0;

    //Response 先入列（原图 04 图例序 Response 居首，2026-11-12 裁定；向量/图例
    //值=总响应原样，投影组合下同为 |total|——ex3 逐字 7.65e-05）
    {
        const int curveId = addPolarCurve(QStringLiteral("%1 :Response")
            .arg(formatPolarValue(totalAbs)), point->totalResponse);
        controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
            QColor(0, 0, 255));
        controller->setNodeProperty(curveId, PostCurve::CurveProp::kLineWidth, 2);
    }

    //排序模态向量族（链形态成员——名称契约 ":Response"/":Others" 之外的曲线
    //一律链成员，视图侧 applyPolarDecor 按此判定链归属）
    //D27 a1+c1（2026-11-13 用户裁定；2026-11-16 扩充）：画线向量恒=原始复向量
    //（组合不再改变画线几何）；投影档负投影整行剔除不占名额（原型 §03 M3
    //缺席于 M5→M7 之间实证），两档零值缺席模态不占池，剔除后不足最高贡献量
    //以缺席模态 Mode 17 零值行补位（2026-11-16 真机 A/B 勘误：原型补位行=
    //固定身份 Mode 17 零值行，非末行克隆，见下方补位段注）
    int shown = 0;
    for (const auto& pair : ranking) {
        if (shown >= topN) {
            break;
        }
        const double modeFreq = modeTable.count(pair.first) > 0
            ? modeTable.at(pair.first) : 0.0;
        const std::complex<double>& rawMode = point->modeContributions.at(pair.first);
        //图例值口径随组合（需求 04 原图逐字：投影=|投影值|；幅值-相位=原始
        //幅值，如 1.08e-05 :Mode 5）；画线向量恒原始（D27 a1 终审）
        const double projectedValue = ModalContributionModel::projectedContribution(
            point->totalResponse, rawMode);
        if (projected && projectedValue < 0.0) {
            continue;
        }
        const double legendValue = projected
            ? std::fabs(projectedValue)
            : ModalContributionModel::magnitude(rawMode);
        //零值缺席模态不占池（2026-11-16 pch 地面真值+用户裁定：416/417Hz 数据
        //块仅 8 行模态，Mode 1/9/17 等系读取端合成零；原型补位行固定身份
        //Mode 17 而非列任意零值模态，补位段注详——审计见需求袋 2026-11-16 批次）
        if (legendValue == 0.0) {
            continue;
        }
        //模态频率十进制进位（原型逐字 1123.39；二进制 double 直 'f',2 得
        //1123.38 截断态，+1e-12 相对垫差对齐十进制四舍五入，非边界值无感）
        const QString entryName = QStringLiteral("%1 :Mode %2 :%3 Hz")
            .arg(formatPolarValue(legendValue), QString::number(pair.first),
                QString::number(modeFreq + std::abs(modeFreq) * 1e-12, 'f', 2));
        const int curveId = addPolarCurve(entryName, rawMode);
        controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
            wheel[colorIdx++ % wheel.size()]);
        ++shown;
    }
    //缺席模态零行补位（D27 c1 扩幅值档；2026-11-16 真机 A/B 勘误裁定）：
    //剔除负投影（投影档）/零值缺席模态（两档）后行数不足 → 合成缺席模态零值
    //行补齐到最高贡献量。身份=EIGEN 模态表第 17 阶（对齐原型实证常量：幅值
    //档/dB 档/投影档四样张交叉恒 "0.00e+00 :Mode 17 :1652.42 Hz" 逐字重复，
    //topN=20 时同身份 ×12 缺口大小无关；旧"末行克隆"系裁定记录误写，与原型
    //样张 crop5_legend.png+真机图二矛盾，spec 同轮勘误）。向量=零：链上零段
    //不推锚点（极点圆点）、散开态极点圆点，与原型 Mode 17 零值点在极点一致；
    //表缺 17 阶回退表末阶防越界（防御性，原型行为未知）；图例值走
    //formatPolarValue(0.0)="0.00e+00" 与原型逐字；补位曲线随色轮取色
    if (shown > 0) {
        constexpr int kCloneModeNumber = 17;
        int cloneMode = 0;
        double cloneFreq = 0.0;
        if (!modeTable.empty()) {
            const auto it = modeTable.find(kCloneModeNumber);
            cloneMode = it != modeTable.end()
                ? kCloneModeNumber : modeTable.rbegin()->first;
            cloneFreq = modeTable.at(cloneMode);
        }
        //模态频率十进制进位（同上：+1e-12 相对垫差对齐十进制四舍五入；
        //1652.416 → "1652.42" 与原型逐字）
        const QString cloneName = QStringLiteral("%1 :Mode %2 :%3 Hz")
            .arg(formatPolarValue(0.0), QString::number(cloneMode),
                QString::number(cloneFreq + std::abs(cloneFreq) * 1e-12,
                    'f', 2));
        const std::complex<double> cloneVec(0.0, 0.0);
        while (shown < topN) {
            const int curveId = addPolarCurve(cloneName, cloneVec);
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
                wheel[colorIdx++ % wheel.size()]);
            ++shown;
        }
    }

    //Others（2026-11-16 深夜原型钉死，证据=需求袋 2026-11-16-polar-linear/
    //others-formula-probe.md）：幅值-相位=总响应减 topN 原始向量代数和的残差
    //复向量（416Hz |残差|=3.22e-08 与原型 top10 图例 3.21e-08 一致，末位差在
    //pch 相位两位小数舍入内）；投影=除最大投影模态外全部正投影行的投影绝对
    //值之和（topN 无关——原型 top3/top10 双档同显 1.01e-06 实证；pch 416Hz
    //真值 1.008e-06 逐字命中；旧“未入选模态投影代数和”口径 7.70e-07 随 topN
    //浮动、与原型两档同值矛盾出局）；画线向量=各行 metricVector 代数和（同
    //向 Response 相位，模长即上和）；样式=蓝实线（2026-11-16 原型 crop 双帧
    //放大逐字实证，推翻 11-12 红虚线裁定，用户当轮确认）
    {
        std::complex<double> othersValue(0.0, 0.0);
        if (projected) {
            //非主导正投影合计：排序池内跳过负投影行（整行剔除语义不变）
            //后，首行（最大投影模态）不计入，其余正投影行累加
            bool firstKept = true;
            for (const auto& pair : ranking) {
                const std::complex<double>& rawMode =
                    point->modeContributions.at(pair.first);
                const double projectedValue =
                    ModalContributionModel::projectedContribution(
                        point->totalResponse, rawMode);
                if (projectedValue <= 0.0) {
                    continue;
                }
                if (firstKept) {
                    firstKept = false;
                    continue;
                }
                othersValue += metricVector(point->totalResponse, rawMode);
            }
        } else {
            othersValue = point->totalResponse;
            int kept = 0;
            for (const auto& pair : ranking) {
                if (kept++ < topN) {
                    othersValue -= point->modeContributions.at(pair.first);
                }
            }
        }
        const int curveId = addPolarCurve(QStringLiteral("%1 :Others")
            .arg(formatPolarValue(std::abs(othersValue))), othersValue);
        controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
            QColor(0, 0, 255));
        controller->setNodeProperty(curveId, PostCurve::CurveProp::kLineStyle, 0);
    }

    //轴形态（需求 04 原型）：半径轴仅 Linear（design 69——产出链钳制，存量
    //Log/dB 面板档不下发，与对话框置灰口径一致；计权不下发——2026-11-16 原型
    //逐字实证+用户裁定，渲染契约处另有 Linear 档屏蔽兜底存量存档）；辐条 8 根
    //（45° 步长，X1 tickCount）、半径主刻度 6（原型 0.0E+00~1.0E-04 六环标；
    //minorCount=0 关次网格）；图例
    //钉图内右上角（2026-11-12 用户裁定：默认图内右上角 position=2，原图对照
    //形态；旧 OutsideRight 钉位作废——圆右侧白区钉位随辐条加多占位过宽）
    const auto children = controller->document()->childNodeIds(chartId);
    for (int childId : children) {
        const auto type = controller->document()->nodeType(childId);
        if (type == PostCurve::CurveNodeType::XAxis) {
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTickCount, 8.0);
        } else if (type == PostCurve::CurveNodeType::YAxis) {
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle,
                tr("Participation (Scale=linear)"));
            controller->setNodeProperty(childId, PostCurve::CurveProp::kScaleMode, 0);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kDbRef,
                _ctxScale[NVHCtx_ShowPolar].axisDbRef());
            controller->setNodeProperty(childId, PostCurve::CurveProp::kWeightMode, 0.0);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kTickCount, 6.0);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kMinorCount, 0);
        } else if (type == PostCurve::CurveNodeType::Legend) {
            controller->setNodeProperty(childId, PostCurve::CurveProp::kAutoPosition, false);
            controller->setNodeProperty(childId, PostCurve::CurveProp::kPosition, 2);
        }
    }

    //图表头四行（需求 04 定稿：行 1=固定模板文案（同柱状图 05 轮已验收口径——
    //原图逐字）；行 2=标题行（频率去尾零——原图 "417 Hz" 整数形态）；行 3=
    //"Node <ID>/<原始分量>Structure Mode Participation, Ranked By - <metric>"
    //（原图逗号分隔+原始分量 T3，不过 display 映射）；行 4=文件路径）
    //响应点名称框非空时表头行内 ID 令牌以名称替换（2026-11-16 用户裁定，
    //pointHeaderToken）
    const QString pointTok = pointHeaderToken(sel);
    const QString compRaw = QString::fromStdString(sel.component);
    QString freqText = QString::number(targetFreq, 'f', 2);
    while (freqText.endsWith(QLatin1Char('0'))) {
        freqText.chop(1);
    }
    if (freqText.endsWith(QLatin1Char('.'))) {
        freqText.chop(1);
    }
    //Ranked By 随真实排序口径（2026-11-12 晚：极坐标排序/画线/图例全随复数
    //分量组合框，排序依据组合框不参与极坐标排序——表头失配实证：幅值档曾打
    //"Abs of Projected"；需求对照图两档表头与画线口径锁定一致）
    const QString nodeLine = tr("Node %1/%2Structure Mode Participation, Ranked By - %3")
        .arg(pointTok, compRaw, sortMetricEnglishText(false, projected));
    const QString titleLine = tr("Participation Magnitude @ %1 (Frequency = %2 Hz) (Structure Mode)")
        .arg(pointTok, freqText);
    const QString headerText = tr("Subcase %1 - System Structure Modal Participation to Structure Grid (Global)\n"
        "%2\n%3\n%4")
        .arg(QString::number(sel.subcaseId), titleLine, nodeLine, _filePathEdit->text());
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderText, headerText);
    //行 3/4 显式三级字体（同柱状图 1577 口径：QFont() 解析为应用默认 9pt 与
    //"未选"判据恒真 → 缩至不可读；显式 8pt 判据成立呈可读小字）
    QFont headerL3Font = QApplication::font();
    headerL3Font.setPointSizeF(qBound(7.0, headerL3Font.pointSizeF() - 1.0, 12.0));
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderFont3, headerL3Font);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderVisible, true);
    //需求 04 表头形态：居中 + 第 2 行强调（kHeaderAlign 0=居中，kHeaderEmphLine
    //1=第 2 行加粗放大；与柱状图 05 轮已验收形态同族）
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderAlign, 0);
    controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderEmphLine, 1);
    return chartId;
}

//§5.3 极坐标"首尾相接"形态下发（视图态不进命令栈，design D5）：随显示选项
//"显示"行（displayPhase）应用到本图视口；视图默认值与面板默认对齐（首尾相接
//=true，2026-11-12：旧视图默认散开+产出链补发时序落空致首渲染形态错——探针
//箭头实证），未物化空跳过后由默认值兜底
void mPostNVHContributionPanel::applyPolarChainForm(int chartId)
{
    if (chartId < 0) {
        return;
    }
    auto* factory = PostCurve::mCurveViewportFactory::getInstance();
    auto* view = factory->chartView(chartId);
    if (view != nullptr) {
        //链形态全组合生效（D27 b1 2026-11-13 裁定拆 D26② 门控：大图 04
        //§02-left=幅值-相位+首尾相接原型示例实锤链形态在幅值组合成立；用户
        //真机"切档画面零变化"=D26② 门控使该开关沦为投影档专属空操作。链
        //构成语义=模态段按排名首尾续接自身相位向量，两组合同规）
        view->setPolarChainForm(_commonOpts.displayPhase);
    }
}

//§7.1 加载成功后回显（spec：回显文件实际范围如 1 Hz ~ 1000 Hz）：当前级联
//选择（复用 currentSelection 口径）的频率集首尾 → 研究页范围框；并回填模态
//列表 EIGEN MODES 全集（"Mode N : <频率> Hz"，item data=模态号，QSignalBlocker
//防工具条误触；默认全不选，全选钮一键选满）
void mPostNVHContributionPanel::refreshResearchFreqRange()
{
    if (_model == nullptr || !_model->isLoaded()) {
        return;
    }
    const PointSelection sel = currentSelection();
    if (sel.valid()) {
        const auto freqs = _model->frequencies(sel.subcaseId, sel.resultType,
            sel.gridId, sel.component);
        if (!freqs.empty()) {
            const QString startText = QString::number(freqs.front(), 'g', 6);
            const QString endText = QString::number(freqs.back(), 'g', 6);
            _rFreqStartEdit->setText(startText);
            _rFreqEndEdit->setText(endText);
            if (_rFreqRangeHint != nullptr) {
                _rFreqRangeHint->setText(tr("频率范围：%1 Hz ~ %2 Hz")
                    .arg(startText, endText));
            }
        }
    }
    const auto modeTable = _model->modeFrequencies(currentSubcaseId());
    {
        const QSignalBlocker blocker(_modeList);
        _modeList->clear();
        for (const auto& pair : modeTable) {
            auto* item = new QListWidgetItem(QStringLiteral("Mode %1 : %2 Hz")
                .arg(QString::number(pair.first),
                    QString::number(pair.second, 'f', 2)), _modeList);
            item->setData(Qt::UserRole, pair.first);
        }
    }
}

//§7.2 研究页"显示"漏斗：部分和=先弹贡献项对话框确认排除集合（design D8：
//先弹框确认 → 数据函数合成 → 产出），取消/未勾选不产出；贡献量叠加=模态
//列表多选集直接产出
void mPostNVHContributionPanel::onResearchShow()
{
    if (_model == nullptr || !_model->isLoaded()) {
        MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
            tr("请先加载模态贡献量结果文件"));
        return;
    }
    const PointSelection sel = currentSelection();
    if (!sel.valid()) {
        MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
            tr("请先选择工况/结果类型/响应点ID/矢量分量"));
        return;
    }
    if (_calcTypeCombo->currentIndex() == 0) {
        //部分和：先弹选择贡献项对话框。2026-11-17 第六批 D1 裁定：贡献项表=
        //模态列表当前多选集（按选中集过滤后的 pickedMap），空选警退同叠加分支；
        //对话框整表预选（传入子集全部键）——用户在表内增删勾选后确定
        const auto modeTable = _model->modeFrequencies(sel.subcaseId);
        if (modeTable.empty()) {
            return;
        }
        std::map<int, double> pickedMap;
        for (auto* item : _modeList->selectedItems()) {
            const int modeId = item->data(Qt::UserRole).toInt();
            const auto it = modeTable.find(modeId);
            if (it != modeTable.end()) {
                pickedMap.emplace(modeId, it->second);
            }
        }
        if (pickedMap.empty()) {
            MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
                tr("请在模态列表中选择至少一个模态"));
            return;
        }
        std::vector<int> preselectModes;
        preselectModes.reserve(pickedMap.size());
        for (const auto& entry : pickedMap) {
            preselectModes.push_back(entry.first);
        }
        mNVHContributorPickDlg dlg(pickedMap, _excludePctSpin->value(),
            preselectModes, this);
        if (dlg.exec() != QDialog::Accepted) {
            return;     //取消不产出（spec 确认集合语义）
        }
        const auto ratios = dlg.selectedRatios();
        if (ratios.empty()) {
            return;     //未选中任何贡献项不产出（行选中语义）
        }
        producePartialSumChart(ratios, _pickOpCombo->currentIndex() == 0);
    } else {
        //贡献量叠加：模态列表多选集（item data=模态号）
        std::vector<int> selectedModes;
        const auto picked = _modeList->selectedItems();
        for (auto* item : picked) {
            selectedModes.push_back(item->data(Qt::UserRole).toInt());
        }
        if (selectedModes.empty()) {
            MechMainWindow::mFramelessMessageBox::warning(this, tr("提示"),
                tr("请在模态列表中选择至少一个模态"));
            return;
        }
        produceOverlayChart(selectedModes);
    }
}

//§7.3 部分和图（design D8）：图表标题 "Partial Sum Response @ <ID> (Structure
//Mode)"、左轴 "% Structural Response"；总响应蓝实线恒 100% 平线（% 归一，不设
//独立基准线）、部分和红虚线 |partial|/|total|×100；差值绿线（勾选时）挂 Y2
//右轴（% of Response 档右轴 "% Difference"、Scale same as Response 档右轴
//"Difference"）；注释粉框 Contributor(s) Excluded/Included + 勾选模态列表
int mPostNVHContributionPanel::producePartialSumChart(
    const std::map<int, double>& modeRatiosPercent, bool excludeOp)
{
    auto* factory = PostCurve::mCurveViewportFactory::getInstance();
    auto* controller = PostCurve::mCurveDocController::getInstance();
    const PointSelection sel = currentSelection();

    const auto fileFreqs = _model->frequencies(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component);
    if (fileFreqs.empty()) {
        return -1;
    }
    //频率范围（研究页范围框；非法/空缺回退文件全范围——同 2D 线图口径）
    double fMin = fileFreqs.front();
    double fMax = fileFreqs.back();
    const double startInput = _rFreqStartEdit->text().toDouble();
    const double endInput = _rFreqEndEdit->text().toDouble();
    if (startInput < endInput) {
        fMin = startInput;
        fMax = endInput;
    }
    const auto series = _model->responseSeries(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component, fMin, fMax);
    if (!series.has_value() || series->frequencies.empty()) {
        return -1;
    }
    int pageId = -1;
    int reuseChartId = -1;
    if (!resolveResearchPlacement(this, _commonOpts.layout,
        &pageId, &reuseChartId)) {
        return -1;
    }
    if (reuseChartId >= 0) {
        factory->setPlacementOverride(PostCurve::ChartPlacement::ReuseCurrent);
    }
    //本次产出 chartId（复用=原窗口；新建=addChart 产物）——收尾激活新产出图
    int producedChartId = -1;
    controller->undoStack()->beginMacro(tr("模态贡献量分析-部分和"));
    try {
        factory->setDeferCardActivation(true);
        if (reuseChartId >= 0) {
            clearChartContent(controller, reuseChartId);
        }
        const int chartId = reuseChartId >= 0 ? reuseChartId
            : controller->addChart(pageId);
        producedChartId = chartId;
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kChartType, 0);
        const QString gridText = QString::number(sel.gridId);
        const QString compDisp = QString::fromStdString(
            ModalContributionModel::componentDisplayName(sel.component));
        const QString titleLine = tr("Partial Sum Response @ %1 (Structure Mode)")
            .arg(gridText);

        QVector<double> xData;
        xData.reserve(static_cast<int>(series->frequencies.size()));
        for (double freqValue : series->frequencies) {
            xData << freqValue;
        }
        //总响应恒 100% 平线（蓝实线——|total|/|total|×100 归一，design D8）；
        //曲线名=Node %1/%2Structure Mode - Response @ %1（第六批 D5：g_w2_title
        //裁片逐字——gridId+分量名无空格拼接，@ 后同 ID；曲线树节点名同源随名）
        {
            QVector<double> yData(xData.size(), 100.0);
            const int curveId = controller->addCurve(chartId,
                tr("Node %1/%2Structure Mode - Response @ %1")
                    .arg(gridText, compDisp));
            controller->setSeriesData(curveId, xData, yData);
            controller->setSeriesSource(curveId, _filePathEdit->text(),
                QStringLiteral("Subcase %1").arg(sel.subcaseId),
                QString::fromStdString(sel.resultType),
                QStringLiteral("Node %1").arg(sel.gridId), compDisp);
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
                QColor(0, 0, 255));
        }
        //部分和红虚线 |partial|/|total|×100（D2 数据函数合成）
        const auto partial = ModalContributionModel::partialSumSeries(series.value(),
            modeRatiosPercent, excludeOp);
        {
            QVector<double> yData;
            yData.reserve(xData.size());
            for (std::size_t i = 0; i < series->total.size(); ++i) {
                const double totalAbs = std::abs(series->total[i]);
                yData << (totalAbs > 0.0
                    ? std::abs(partial[i]) / totalAbs * 100.0 : 0.0);
            }
            //曲线名=Partial sum Response @ <ID>（第六批 D5：小写 sum——商软
            //标题大写/图例小写并存，裁定书§六.3；图表标题/页眉大写 Sum 不动）
            const int curveId = controller->addCurve(chartId,
                tr("Partial sum Response @ %1").arg(gridText));
            controller->setSeriesData(curveId, xData, yData);
            controller->setSeriesSource(curveId, _filePathEdit->text(),
                QStringLiteral("Subcase %1").arg(sel.subcaseId),
                QString::fromStdString(sel.resultType),
                QStringLiteral("Node %1").arg(sel.gridId), compDisp);
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
                QColor(255, 0, 0));
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kLineStyle, 1);
        }

        //轴标题（X=频率；Y1 左轴 "% Structural Response"，spec 7.3）
        const auto children = controller->document()->childNodeIds(chartId);
        for (int childId : children) {
            const auto type = controller->document()->nodeType(childId);
            if (type == PostCurve::CurveNodeType::XAxis) {
                controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle,
                    tr("Frequency (Hz)"));
            } else if (type == PostCurve::CurveNodeType::YAxis) {
                controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle,
                    tr("% Structural Response"));
            }
        }

        //差值曲线（勾选时）：绿线挂 Y2 副轴（kAxisAssign=1）——% of Response 档
        //|diff|/|total|×100 右轴 "% Difference"；Scale 档 |diff| 绝对值右轴
        //"Difference"（spec 7.3）
        if (_diffCheck->isChecked()) {
            const auto diff = ModalContributionModel::differenceSeries(series.value(),
                partial);
            const bool pctMode = _diffModeCombo->currentIndex() == 0;
            QVector<double> yData;
            yData.reserve(xData.size());
            for (std::size_t i = 0; i < series->total.size(); ++i) {
                const double diffAbs = std::abs(diff[i]);
                const double totalAbs = std::abs(series->total[i]);
                yData << (pctMode && totalAbs > 0.0
                    ? diffAbs / totalAbs * 100.0 : diffAbs);
            }
            const int y2Id = controller->addYAxis2(chartId);
            controller->setNodeProperty(y2Id, PostCurve::CurveProp::kTitle,
                pctMode ? tr("% Difference") : tr("Difference"));
            //第六批⑤（D3）右轴恰域下发表：自动缩放关 + ±100（% of Response 档）
            //或 ±10（Scale same as Response 档）+ 均匀 8 刻度（七等分）+ 定点
            //%.5f；路径 applySecondaryAxes→applyAxis(Y_Right) 手动范围分支直发引擎
            controller->setNodeProperty(y2Id, PostCurve::CurveProp::kAutoScale, false);
            controller->setNodeProperty(y2Id, PostCurve::CurveProp::kMin,
                pctMode ? -100.0 : -10.0);
            controller->setNodeProperty(y2Id, PostCurve::CurveProp::kMax,
                pctMode ? 100.0 : 10.0);
            controller->setNodeProperty(y2Id, PostCurve::CurveProp::kTickGenMode, 0);
            controller->setNodeProperty(y2Id, PostCurve::CurveProp::kTickCount, 8.0);
            controller->setNodeProperty(y2Id, PostCurve::CurveProp::kLabelFormat, 2);
            controller->setNodeProperty(y2Id, PostCurve::CurveProp::kPrecision, 5);
            const int curveId = controller->addCurve(chartId, tr("Difference"));
            controller->setSeriesData(curveId, xData, yData);
            controller->setSeriesSource(curveId, _filePathEdit->text(),
                QStringLiteral("Subcase %1").arg(sel.subcaseId),
                QString::fromStdString(sel.resultType),
                QStringLiteral("Node %1").arg(sel.gridId), compDisp);
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
                QColor(0, 128, 0));
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kAxisAssign, 1);
        }

        //注释粉框（第六批 D4 三行改版）：首行 Contributor(s) Excluded/Included
        //+% 公共值（选中模态 %To Exclude 全等取该值，否则页内去除比例框值，
        //trim 小数——样张 100% 非 100.0%）；次行频率范围 'f'1（series 实际
        //范围）；模态行两空格+冒号贴值（区别于柱状 Note「 : 」带空格形态）
        const auto modeTable = _model->modeFrequencies(sel.subcaseId);
        bool pctUniform = !modeRatiosPercent.empty();
        const double commonPct = pctUniform
            ? modeRatiosPercent.begin()->second : 0.0;
        for (const auto& pair : modeRatiosPercent) {
            if (pair.second != commonPct) {
                pctUniform = false;
                break;
            }
        }
        const double notePct = pctUniform ? commonPct
            : static_cast<double>(_excludePctSpin->value());
        QString noteText = excludeOp
            ? tr("Contributor(s) Excluded %1%").arg(formatPctTrim(notePct))
            : tr("Contributor(s) Included %1%").arg(formatPctTrim(notePct));
        noteText += QStringLiteral("\n") + tr("Frequency range %1 to %2")
            .arg(QString::number(fMin, 'f', 1), QString::number(fMax, 'f', 1));
        for (const auto& pair : modeRatiosPercent) {
            const double modeFreq = modeTable.count(pair.first) > 0
                ? modeTable.at(pair.first) : 0.0;
            noteText += QStringLiteral("\nMode %1  :%2 Hz")
                .arg(QString::number(pair.first),
                    QString::number(modeFreq, 'f', 2));
        }
        const int noteId = controller->addAnnotation(chartId);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kText, noteText);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kAttachMode, 1);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kBorderMode, 1);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kBorderColor,
            QColor(255, 0, 255));
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kAutoPos, true);
        controller->setNodeProperty(noteId, PostCurve::CurveProp::kAutoAnchor, 4);

        //图表头四行（标题行=图表标题，spec 7.3）；行 2 名称形态（titleLine 兼作
        //kName 维持 ID 口径），行内 ID 令牌按响应点名称替换（2026-11-16 用户裁定）
        const QString headerTitle = tr("Partial Sum Response @ %1 (Structure Mode)")
            .arg(pointHeaderToken(sel));
        const QString headerText = QStringLiteral("Subcase %1\n%2"
            "\nNode %3/%4Structure Mode Participation\n%5")
            .arg(QString::number(sel.subcaseId), headerTitle,
                pointHeaderToken(sel), compDisp, _filePathEdit->text());
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderText, headerText);
        //行 3/4 显式三级字体（fix 6.1 同通道收口，修法同柱状图 1577）
        QFont headerL3Font = QApplication::font();
        headerL3Font.setPointSizeF(qBound(7.0, headerL3Font.pointSizeF() - 1.0, 12.0));
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderFont3, headerL3Font);
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderVisible, true);
        //需求02表头形态：左对齐 + 第 2 行强调（加粗+放大）
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderAlign, 0);
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderEmphLine, 1);
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kName, titleLine);
    } catch (...) {
        //异常安全：宏闭合防撤销栈卡半开（同 produceCharts 口径）
        controller->undoStack()->endMacro();
        factory->clearPlacementOverride();
        factory->setDeferCardActivation(false);
        throw;
    }
    controller->undoStack()->endMacro();
    factory->clearPlacementOverride();
    factory->setDeferCardActivation(false);
    //激活本次新产出（同 produceCharts 收尾口径——页内既有图时 activatePageFirstChart
    //会激活旧图、新图反不得呈现，2026-11-11 真机缺陷同款修）；无产出时兜底激活页首图
    if (producedChartId >= 0) {
        factory->activateChartViewport(producedChartId);
    } else {
        factory->activatePageFirstChart(pageId);
    }
    reassertToolSetDock();   //同 produceCharts 收尾：回抬被激活链顶掉的本面板
    return producedChartId;
}

//§7.4 贡献量叠加图：图表标题 "Response & Contributor Overlay @ <ID> (Structure
//Mode)"；总响应蓝粗线 + 逐选中模态曲线，图例即曲线名 "Mode N : <频率> Hz"
//（spec 7.4）；Y 轴缩放/计权随显示选项（§7.5 全局项生效口径，同 2D 线图轴链）
int mPostNVHContributionPanel::produceOverlayChart(const std::vector<int>& selectedModes)
{
    auto* factory = PostCurve::mCurveViewportFactory::getInstance();
    auto* controller = PostCurve::mCurveDocController::getInstance();
    const PointSelection sel = currentSelection();

    const auto fileFreqs = _model->frequencies(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component);
    if (fileFreqs.empty()) {
        return -1;
    }
    //频率范围（研究页范围框；非法/空缺回退文件全范围——同 2D 线图口径）
    double fMin = fileFreqs.front();
    double fMax = fileFreqs.back();
    const double startInput = _rFreqStartEdit->text().toDouble();
    const double endInput = _rFreqEndEdit->text().toDouble();
    if (startInput < endInput) {
        fMin = startInput;
        fMax = endInput;
    }
    const auto series = _model->responseSeries(sel.subcaseId, sel.resultType,
        sel.gridId, sel.component, fMin, fMax);
    if (!series.has_value() || series->frequencies.empty()) {
        return -1;
    }
    int pageId = -1;
    int reuseChartId = -1;
    if (!resolveResearchPlacement(this, _commonOpts.layout,
        &pageId, &reuseChartId)) {
        return -1;
    }
    if (reuseChartId >= 0) {
        factory->setPlacementOverride(PostCurve::ChartPlacement::ReuseCurrent);
    }
    //本次产出 chartId（复用=原窗口；新建=addChart 产物）——收尾激活新产出图
    int producedChartId = -1;
    controller->undoStack()->beginMacro(tr("模态贡献量分析-叠加"));
    try {
        factory->setDeferCardActivation(true);
        if (reuseChartId >= 0) {
            clearChartContent(controller, reuseChartId);
        }
        const int chartId = reuseChartId >= 0 ? reuseChartId
            : controller->addChart(pageId);
        producedChartId = chartId;
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kChartType, 0);
        const QString gridText = QString::number(sel.gridId);
        const QString compDisp = QString::fromStdString(
            ModalContributionModel::componentDisplayName(sel.component));
        const QString titleLine = tr("Response & Contributor Overlay @ %1 (Structure Mode)")
            .arg(gridText);
        const auto modeTable = _model->modeFrequencies(sel.subcaseId);
        const auto wheel = PostCurve::curveDefaultColorWheel();
        QVector<double> xData;
        xData.reserve(static_cast<int>(series->frequencies.size()));
        for (double freqValue : series->frequencies) {
            xData << freqValue;
        }
        //总响应蓝粗线（同 2D 线图 Response 口径）
        {
            QVector<double> yData;
            yData.reserve(xData.size());
            for (const auto& value : series->total) {
                yData << std::abs(value);
            }
            const int curveId = controller->addCurve(chartId, tr("Response"));
            controller->setSeriesData(curveId, xData, yData);
            controller->setSeriesSource(curveId, _filePathEdit->text(),
                QStringLiteral("Subcase %1").arg(sel.subcaseId),
                QString::fromStdString(sel.resultType),
                QStringLiteral("Node %1").arg(sel.gridId), compDisp);
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
                QColor(0, 0, 255));
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kLineWidth, 2);
        }
        //逐选中模态曲线（图例即曲线名 "Mode N : <频率> Hz"，spec 7.4）
        int colorIdx = 0;
        for (int mode : selectedModes) {
            const auto it = series->modes.find(mode);
            if (it == series->modes.end()) {
                continue;
            }
            QVector<double> yData;
            yData.reserve(xData.size());
            for (const auto& value : it->second) {
                yData << ModalContributionModel::magnitude(value);
            }
            const double modeFreq = modeTable.count(mode) > 0
                ? modeTable.at(mode) : 0.0;
            const int curveId = controller->addCurve(chartId,
                tr("Mode %1 : %2 Hz").arg(QString::number(mode),
                    QString::number(modeFreq, 'f', 2)));
            controller->setSeriesData(curveId, xData, yData);
            controller->setSeriesSource(curveId, _filePathEdit->text(),
                QStringLiteral("Subcase %1").arg(sel.subcaseId),
                QString::fromStdString(sel.resultType),
                QStringLiteral("Node %1").arg(sel.gridId),
                QString::fromStdString(sel.component));
            controller->setNodeProperty(curveId, PostCurve::CurveProp::kColor,
                wheel[colorIdx++ % wheel.size()]);
        }

        //轴标题/刻度随显示选项（同 2D 线图口径，§7.5 全局项生效）
        const auto children = controller->document()->childNodeIds(chartId);
        for (int childId : children) {
            const auto type = controller->document()->nodeType(childId);
            if (type == PostCurve::CurveNodeType::XAxis) {
                controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle,
                    tr("Frequency (Hz)"));
            } else if (type == PostCurve::CurveNodeType::YAxis) {
                controller->setNodeProperty(childId, PostCurve::CurveProp::kTitle,
                    participationAxisTitle(NVHCtx_Research));
                controller->setNodeProperty(childId, PostCurve::CurveProp::kScaleMode,
                    _ctxScale[NVHCtx_Research].axisScaleMode());
                controller->setNodeProperty(childId, PostCurve::CurveProp::kDbRef,
                    _ctxScale[NVHCtx_Research].axisDbRef());
                controller->setNodeProperty(childId, PostCurve::CurveProp::kWeightMode,
                    _commonOpts.axisWeightMode());
            }
        }

        //图表头四行（标题行=图表标题，spec 7.4）；行 2 名称形态（titleLine 兼作
        //kName 维持 ID 口径），行内 ID 令牌按响应点名称替换
        //（2026-11-16 用户裁定）
        const QString headerTitle = tr("Response & Contributor Overlay @ %1 (Structure Mode)")
            .arg(pointHeaderToken(sel));
        const QString headerText = QStringLiteral("Subcase %1\n%2"
            "\nNode %3/%4Structure Mode Participation\n%5")
            .arg(QString::number(sel.subcaseId), headerTitle,
                pointHeaderToken(sel), compDisp, _filePathEdit->text());
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderText, headerText);
        //行 3/4 表头三级字体（fix 6.1 同通道收口，修法同柱状图 1577）
        QFont headerL3Font = QApplication::font();
        headerL3Font.setPointSizeF(qBound(7.0, headerL3Font.pointSizeF() - 1.0, 12.0));
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderFont3, headerL3Font);
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderVisible, true);
        //需求02表头形态：左对齐 + 第 2 行强调（加粗+放大）
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderAlign, 0);
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kHeaderEmphLine, 1);
        controller->setNodeProperty(chartId, PostCurve::CurveProp::kName, titleLine);
    } catch (...) {
        //异常安全：宏闭合防撤销栈卡半开（同 produceCharts 口径）
        controller->undoStack()->endMacro();
        factory->clearPlacementOverride();
        factory->setDeferCardActivation(false);
        throw;
    }
    controller->undoStack()->endMacro();
    factory->clearPlacementOverride();
    factory->setDeferCardActivation(false);
    //激活本次新产出（同 produceCharts 收尾口径——页内既有图时 activatePageFirstChart
    //会激活旧图、新图反不得呈现，2026-11-11 真机缺陷同款修）；无产出时兜底激活页首图
    if (producedChartId >= 0) {
        factory->activateChartViewport(producedChartId);
    } else {
        factory->activatePageFirstChart(pageId);
    }
    reassertToolSetDock();   //同 produceCharts 收尾：回抬被激活链顶掉的本面板
    return producedChartId;
}

//响应曲线 Y 轴标题："Structural Response" + 缩放后缀（随显示选项联动，spec 3.3：
//Linear "(Scale=linear)" / Log "(Scale=log)" / dB "(Scale=dB20(A) Ref=1.0)"）
QString mPostNVHContributionPanel::responseAxisTitle() const
{
    return tr("Structural Response ")
        + _ctxScale[NVHCtx_Load].axisScaleSuffix(_commonOpts);
}

//参与量轴标题：%Plot="% Participation"（数据级换算档，轴 Linear——design D6），
//其余 "Participation (Scale=…)"
QString mPostNVHContributionPanel::participationAxisTitle(int ctx) const
{
    if (_ctxScale[ctx].scale == NVHScale_PercentPlot) {
        return tr("% Participation");
    }
    return tr("Participation ")
        + _ctxScale[ctx].axisScaleSuffix(_commonOpts);
}

//排序依据英文名（Note 末行/Ranked By 标签，spec：投影绝对值→Abs of Projected、
//幅值→Magnitude、RSS 档→Abs of Projected RSS / Magnitude RSS；读排序依据
//组合框——柱状/2D 路径口径）
QString mPostNVHContributionPanel::sortMetricEnglishName(bool rss) const
{
    return sortMetricEnglishText(rss, _complexSortCombo->currentIndex() == 0);
}

//纯映射（单源字符串；极坐标表头按真实排序口径传值，见 producePolarChart）
QString mPostNVHContributionPanel::sortMetricEnglishText(bool rss, bool projected)
{
    return projected
        ? (rss ? QStringLiteral("Abs of Projected RSS") : QStringLiteral("Abs of Projected"))
        : (rss ? QStringLiteral("Magnitude RSS") : QStringLiteral("Magnitude"));
}

} // namespace PostWindow

//后处理 dock 提供者自注册（DLL 加载期执行；主窗口注册 "NVHToolSet" dock 时经
//MechCommon::nvhToolSetDockProvider 间接获取本面板，PostWindow 隔离不破——
//模式先例 mPostBrowserWindow/mVideoWindow；落点=底部条「函数与曲线」标签组右侧
//并排分栏，入口=曲线页诊断组按钮，见 openspec add-nvh-toolset-window-page）
static const bool s_registerNvhToolSetDockProvider = []()
{
    MechCommon::nvhToolSetDockProvider() = []() -> QWidget*
    {
        return PostWindow::mPostNVHContributionPanel::getInstance();
    };
    return true;
}();
