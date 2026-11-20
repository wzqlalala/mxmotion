#include <mGeoManager.h>
#include "mPostMeasureTool.h"
#include "mFbdColorPicker.h"
#include "mPostObjectEditors.h"
#include "mPanelTable.h"
#include "mCommonFunctions.h"
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostGUIData.h"
#include "mPost3DResult.h"      //P2-2 审计修复：云图面板实时播放态查询（isRealtimeAnimPlaying）
#include "mPostInfoHub.h"                //P2-2 审计修复：录制簿记聚合（isRecordingActive）/云图面板入口
#include "mViewportModelManager.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
// S5 门面直接替换：会话头已删除（REBUILD_PostRenderSession 转私有宿主）——
// 值类型经 mxr/view/REBUILD_PostRender.h（内含 REBUILD_PostRenderTypes.h）带入
#include "mxr/view/PostSelectionCalculator.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mViewportModel.h"
#include "mAppConfiguration.h"
#include "mDockLayoutManager.h"
#include "mCurveWindow.h"
#include "mPlotView.h"
#include "mxr/view/TempDimensionController.h"
#include "mxr/basic/Camera.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mMechGlobalSignals.h"
#include "mGlobalSignals.h"

#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QItemSelectionModel>
#include <QShortcut>
#include <QToolTip>
#include <QHelpEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QtMath>

#include <cmath>
#include <algorithm>
#include <functional>
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace PostData;
using namespace PostCommon;
namespace PostWindow
{
    namespace
    {
        //“通用结构软件界面UI设计规则”标准档令牌（--std-*，与截面力面板同源）
        const char* kText       = "#424B57";	//正文 --std-text
        const char* kMuted      = "#788391";	//辅助文字 --std-muted
        const char* kLabel      = "#5C6674";	//表头/字段标签 --std-label
        const char* kDisabled   = "#A6AFBB";	//禁用文字 --std-disabled
        const char* kCtrlBorder = "#DCE2E9";	//控件描边 --std-border
        const char* kHeadBg     = "#F5F7FA";	//分区标题底色 --std-panel-color
        const char* kTableHead  = "#E9EDF3";	//表头底色 --std-header-color
        const char* kPrimary    = "#1890FF";	//主蓝（选中/焦点/显隐指示） --std-primary
        const char* kSelBg      = "#E8F3FF";	//选中底色 --std-selected
        const char* kSelText    = "#176BA6";	//选中文字 --std-selected-text
        const char* kHoverBg    = "#F0F7FF";	//悬停底色 --std-hover
        const char* kDanger     = "#BF3434";	//删除红 --std-error

        //标准档尺寸令牌：控件 26 / 表头 26 / 管理表格行 26 / 筛选 88 / 标签列 84 /
        //行距 6 / 列距 8 / 圆角 2 / 勾选 13 / 箭头槽 12
        const int kControl = 26;			//--std-control
        //管理表格行外高=control 26（规则文档 2.2.5 M2：表头与数据行外高为 control 含边框；
        //“可编辑数据行=control+4=30”适用于可编辑数据表，管理/列表表格不适用）
        const int kFilterW = 88;			//管理筛选宽 --std-manager-filter-width
        const int kLabelW = 84;				//弹窗标签列宽 --std-label-width
        const int kPopupHeadH = 28;			//弹窗标题栏高（原型标题条实测 28：×钮 y3 h24）

        //幅值文案（标签详情摘要用）
        const char* kMagnitudeCn[] = { "无", "X+Y+Z", "X+Y", "X+Z", "Y+Z", "X", "Y", "Z" };
        const char* kMagnitudeEn[] = { "None", "X+Y+Z", "X+Y", "X+Z", "Y+Z", "X", "Y", "Z" };

        //主操作蓝按钮（创建/绘制）：蓝底白字，悬停深蓝，禁用灰（--std-action/-hover）
        const char* kPrimaryBtnQss =
            "QPushButton{background:#1677C8;border:1px solid #1677C8;color:#FFFFFF;border-radius:2px;}"
            "QPushButton:hover{background:#1267B0;border:1px solid #1267B0;}"
            "QPushButton:disabled{background:#F5F7FA;border:1px solid #DCE2E9;color:#A6AFBB;}";

        //分段连体格（编号/名称、度数/弧度，对齐原型 mirror-plane：容器描边、格间竖线、选中蓝底蓝字）
        const char* kPairSegQss =
            "QWidget{border:1px solid #DCE2E9;border-radius:2px;background:#FFFFFF;}"
            "QCheckBox{border:0;border-right:1px solid #DCE2E9;border-radius:0;background:transparent;padding:0 2px;}"
            "QCheckBox[cellRole=\"last\"]{border-right:0;}"
            "QCheckBox:hover{background:#F0F7FF;}"
            "QCheckBox:checked{background:#E8F3FF;color:#176BA6;}"
            "QRadioButton{border:0;border-right:1px solid #DCE2E9;border-radius:0;background:transparent;padding:0 2px;}"
            "QRadioButton[cellRole=\"last\"]{border-right:0;}"
            "QRadioButton:hover{background:#F0F7FF;}"
            "QRadioButton:checked{background:#E8F3FF;color:#176BA6;}";

        //「测量数据」下拉定义（前 11 项对齐原型 measurementData 选项序；末 2 项为引擎
        //既有能力的实现增补。calc 计算分支：0两点距离/1增量距离/2位置/3相对位移/4相对角度/
        //5夹角/6增量角度/7圆半径/8节点云图/9节点路径/10最小距离 = 节点位置原语求值；
        //11体积/12面积 = 引擎按拾取对象（网格单元/几何体、面）计算）
        struct MeasureDataDef
        {
            const char* cn;
            const char* en;
            int calc;	//计算分支（见 nodesForCalc）
            int nodes;	//所需最少节点数
        };
        const MeasureDataDef kMeasureDataDefs[] =
        {
            { "两点距离",	"Distance",				 0, 2 },
            { "增量距离",	"Delta Distance",		 1, 2 },
            { "位置",		"Position",				 2, 1 },
            { "相对位移",	"Rel. Displacement",	 3, 2 },
            { "相对角度",	"Rel. Angle",			 4, 2 },
            { "夹角",		"Angle",				 5, 3 },
            { "增量角度",	"Delta Angle",			 6, 3 },
            { "圆半径",		"Circle Radius",		 7, 3 },
            { "节点云图",	"Nodal Contour",		 8, 1 },
            { "节点路径",	"Node Path",			 9, 2 },
            { "最小距离",	"Minimum Distance",		10, 2 },
            { "体积",		"Volume",				11, 1 },
            { "面积",		"Area",					12, 1 },
        };
        const int kMeasureDataCount = sizeof(kMeasureDataDefs) / sizeof(kMeasureDataDefs[0]);
        const int kDefaultMeasureData = 0;	//设计默认"两点距离"（原型 measurementData 首选项）

        //三点外接圆半径（圆半径测量：三点确定外接圆，海伦公式面积 + a*b*c/4Δ）
        double circumRadius3D(const QVector3D& a, const QVector3D& b, const QVector3D& c)
        {
            const double la = (b - c).length();
            const double lb = (a - c).length();
            const double lc = (a - b).length();
            const double s = (la + lb + lc) / 2.0;
            const double area2 = s * (s - la) * (s - lb) * (s - lc);
            if (area2 <= 0.0)
                return 0.0;
            return la * lb * lc / (4.0 * std::sqrt(area2));
        }

        //向量分量取值（yIdx：0X/1Y/2Z/3合量=模）
        double compValue(const QVector3D& v, int yIdx)
        {
            switch (yIdx)
            {
            case 0: return v.x();
            case 1: return v.y();
            case 2: return v.z();
            }
            return v.length();
        }

        double angleBetweenVectors(const QVector3D& a, const QVector3D& b)
        {
            const double la = a.length(), lb = b.length();
            if (la <= 0.0 || lb <= 0.0)
                return 0.0;
            double c = QVector3D::dotProduct(a, b) / (la * lb);
            c = qBound(-1.0, c, 1.0);
            return qRadiansToDegrees(std::acos(c));
        }

        double polyLineLength(const QVector<QVector3D>& c)
        {
            double len = 0.0;
            for (int i = 1; i < c.size(); ++i)
                len += (c[i] - c[i - 1]).length();
            return len;
        }

        //各计算分支取值：c=当前帧位置（拾取序） c0=首帧位置（delta 类基准，
        //缺失时 delta 退化为当前值） yIdx：0X/1Y/2Z/3合量（仅两点距离/
        //位置/相对位移向量类分支生效，标量类忽略；1增量距离为标量间距差）
        double calcValue(int calc, const QVector<QVector3D>& c, const QVector<QVector3D>& c0, int yIdx)
        {
            const QVector<QVector3D>& c0u = c0.size() >= c.size() ? c0 : c;
            switch (calc)
            {
            case 0: { QVector3D v = c[1] - c[0]; return compValue(v, yIdx); }                   // 两点距离
            case 1: return (c[1] - c[0]).length() - (c0u[1] - c0u[0]).length();                 // 增量距离（P1：间距标量差 d(t)−d(t0)）
            case 2: return compValue(c[0], yIdx);                                               // 位置
            case 3: { QVector3D d = (c[0] - c0u[0]) - (c[1] - c0u[1]); return compValue(d, yIdx); } // 相对位移
            case 4: { QVector3D a = c[0] - c0u[0], b = c[1] - c0u[1];                           // 相对角度
                      return angleBetweenVectors(a, b); }
            case 5: return angleBetweenVectors(c[0] - c[1], c[2] - c[1]);                       // 夹角（第2拾取为角点）
            case 6: { const double a0 = angleBetweenVectors(c0u[0] - c0u[1], c0u[2] - c0u[1]);  // 增量角度
                      return angleBetweenVectors(c[0] - c[1], c[2] - c[1]) - a0; }
            case 7: return circumRadius3D(c[0], c[1], c[2]);                                    // 圆半径
            case 8: return 0.0;   // 节点云图：由 plotRecords 按激活云图变量逐帧取值（P0），不走位置原语
            case 9: return polyLineLength(c);                                                   // 节点路径（折线总长）
            case 10:                                                                            // 最小距离（P2：基准点到对比集合）
            {
                double best = -1.0;
                for (int i = 1; i < static_cast<int>(c.size()); ++i)
                {
                    const double d = (c[i] - c[0]).length();
                    if (best < 0.0 || d < best)
                        best = d;
                }
                return best < 0.0 ? 0.0 : best;
            }
            }
            return 0.0;
        }

	//计算分支所需最少节点数
	int nodesForCalc(int calc)
	{
		switch (calc)
		{
		case 2: case 8: return 1;
		case 5: case 6: case 7: return 3;
		case 11: case 12: return 1;		//体积/面积：1 个拾取对象（单元/几何体/面）
		}
		return 2;
	}

	//向量类计算分支：纵轴分量 X/Y/Z 对其有意义（其余标量类纵轴仅"结果"，
	//需求 V1.1 §5：不把标量结果强行当作 X、Y、Z 分量）
	bool calcHasComponents(int calc)
	{
		return calc == 0 || calc == 1 || calc == 2 || calc == 3;
	}

	//幅值分量组合（幅值下拉序：0无/1X+Y+Z/2X+Y/3X+Z/4Y+Z/5X/6Y/7Z）：
	//向量类记录按所选分量组合取幅值
	double magnitudeOverComponents(const QVector3D& v, int mag)
	{
		switch (mag)
		{
		case 1: return v.length();
		case 2: return std::sqrt(v.x() * v.x() + v.y() * v.y());
		case 3: return std::sqrt(v.x() * v.x() + v.z() * v.z());
		case 4: return std::sqrt(v.y() * v.y() + v.z() * v.z());
		case 5: return v.x();
		case 6: return v.y();
		case 7: return v.z();
		}
		return 0.0;
	}

	//幅值组合的标签标注（HyperView 测量标签口径：合量=Mag，分量组合=分量名）
	const char* magnitudeTag(int mag)
	{
		switch (mag)
		{
		case 1: return "Mag";
		case 2: return "XY";
		case 3: return "XZ";
		case 4: return "YZ";
		case 5: return "X";
		case 6: return "Y";
		case 7: return "Z";
		}
		return "";
	}

        //绘图区轴角图标（对齐原型 plot-axis-control 的 16 视Box SVG：X 轴箭头朝右、Y 轴箭头朝上）
        QPixmap axisCornerIcon(bool yAxis, const QColor& color)
        {
            QPixmap pm(32, 32);
            pm.setDevicePixelRatio(2.0);	//16 逻辑像素
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing, true);
            QPen pen(color, 1.2);
            pen.setCapStyle(Qt::FlatCap);
            pen.setJoinStyle(Qt::MiterJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            QPainterPath path;
            path.moveTo(2, 2);
            path.lineTo(2, 14);
            path.lineTo(14, 14);
            if (!yAxis)
            {
                //X：M2 2v12h12 m-3-3 3 3-3 2
                path.moveTo(11, 11);
                path.lineTo(14, 14);
                path.lineTo(11, 16);
            }
            else
            {
                //Y：M2 2v12h12 M0 5l2-3 3 3
                path.moveTo(0, 5);
                path.lineTo(2, 2);
                path.lineTo(5, 5);
            }
            p.drawPath(path);
            return pm;
        }

    }

    mPostMeasureTool::mPostMeasureTool(QWidget *parent)
        : mPanelBase(parent)
        , _postDB(nullptr)
        , _preRend (nullptr)
        , _postRend (nullptr)
        , _isCurrPostViewport(true)
        , _isCurrStateValid(false)
    {
        setupUI();

        // 与云图面板统一：折叠区注册到基类统一管理
        m_foldWidgets.append(_manageFold);
        m_foldWidgets.append(_toolFold);

        // 测量数据下拉 / 选择节点拾取按钮
        connect(_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mPostMeasureTool::slot_dataTypeChanged);
        connect(_btnPickNode, &QPushButton::clicked, this, &mPostMeasureTool::slot_pickNodeClicked);

        // 创建按钮（替代原"测量"按钮：结果写入测量数据列表）
        connect(_btnCreate, &QPushButton::clicked, this, &mPostMeasureTool::slot_createButton);

        // 管理列表：复选联动/显隐切换/颜色取色
        connect(_table, &QTableWidget::itemChanged, this, &mPostMeasureTool::slot_itemChanged);
        connect(_table, &QTableWidget::cellClicked, this, [this](int row, int col) {
            if (col == 1)
            {
                //V1.3：名称格点击=选中编辑
                QTableWidgetItem* nameIt = _table->item(row, 1);
                if (nameIt)
                    selectMeasureRecord(nameIt->data(Qt::UserRole).toInt());
            }
            else if (col == 3)
            {
                //显隐格点击：切换记录显隐
                QTableWidgetItem* nameIt = _table->item(row, 1);
                const int di = nameIt ? dataIndexById(nameIt->data(Qt::UserRole).toInt()) : -1;
                if (di < 0) return;
                _records[di].visible = !_records[di].visible;
                syncMeasureCache();
                _table->setSortingEnabled(false);
                renderRow(row);
                _table->setSortingEnabled(true);
                _table->horizontalHeader()->setSortIndicatorShown(false);
                updateMeasureAnnotations();	//显隐列 → 3D 测量标注同步
            }
            else if (col == 4)
            {
                slot_showColorPopup(row);
            }
            });
        connect(_btnSelectAll, &QPushButton::clicked, this, &mPostMeasureTool::slot_selectAllClicked);
        connect(_btnShow, &QPushButton::clicked, this, &mPostMeasureTool::slot_showSelectedClicked);
        connect(_btnHide, &QPushButton::clicked, this, &mPostMeasureTool::slot_hideSelectedClicked);
        connect(_btnDelete, &QPushButton::clicked, this, &mPostMeasureTool::slot_deleteButton);
        connect(_searchEdit, &QLineEdit::textChanged, this, &mPostMeasureTool::slot_searchChanged);
        connect(_typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mPostMeasureTool::slot_typeFilterChanged);

        // 标签内容自定义 / 测量数据管理及曲线生成弹窗
        connect(_btnLabel, &QPushButton::clicked, this, &mPostMeasureTool::slot_labelButtonClicked);
        connect(_btnManager, &QPushButton::clicked, this, &mPostMeasureTool::slot_showReviewPopup);

        //Esc 取消拾取（对齐原型：keydown Escape → setMeasurementPicking(false)；
        //面板隐藏时快捷键自动失效，不干扰其他页面）
        QShortcut* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
        escShortcut->setContext(Qt::WindowShortcut);
        connect(escShortcut, &QShortcut::activated, this, &mPostMeasureTool::exitPickMode);

        //对齐原型：measurementSection toggle 收起即 setMeasurementPicking(false)
        connect(_toolFold, &FoldSection::foldExpandChanged, this, [this](bool expand) {
            if (!expand) exitPickMode();
            });

        // 切换界面语言（与截面力面板一致：随全局语言信号刷新）
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

        refreshEmptyState();
        syncManagerState();

        this->setMinimumWidth(319);	//原型侧栏宽 319（measurementParameters 面板宽）
    }

    mPostMeasureTool::~mPostMeasureTool()
    {
        if (_pickConn)
        {
            disconnect(_pickConn);
        }
    }

    void mPostMeasureTool::setupUI()
    {
        _mainLayout = new QVBoxLayout(this);
        //原型 parameter-panel：内容左右 4、顶部 6（蓝钮 x4/y6 实测）
        _mainLayout->setContentsMargins(4, 6, 4, 0);

        /********************测量数据管理及曲线生成入口********************/
        //设计截图：全宽主操作蓝钮（同"创建"钮样式，白字居中，无 › 箭头）
        _btnManager = new QPushButton(this);
        _btnManager->setObjectName("mManagerEntryBtn");
        _btnManager->setStyleSheet(kPrimaryBtnQss);
        _btnManager->setFixedHeight(kControl);
        _btnManager->setCursor(Qt::PointingHandCursor);
        _mainLayout->addWidget(_btnManager);

        /********************测量数据列表********************/
        _manageFold = new FoldSection(QString::fromUtf8("测量数据列表"), this);
        //本区全部为跨列行（筛选/表格/表尾）：QFormLayout 跨列行几何=表单边距，
        //原型实测筛选行 x6/表尾 x6（面板边距 4 + 行自身 2 内缩，Qt 取整贴边差 ≤3 容忍）；
        //负边距会被 QLayout 判无效回退样式默认 9（踩坑），故边距必须保持 0。
        //底边距 4：管理状态行改行级摘除后失去隐藏行 6px 残留，此处补 4px 使
        //工具区首行对齐原型 y366（0.20 轮逐控件实测）
        if (auto* form = _manageFold->layout())
        {
            form->setContentsMargins(0, 10, 0, 4);
        }

        //筛选行：类型筛选 88（--std-manager-filter-width；原型渲染 88×26 + 搜索占满 213，
        //设计截图像素实测 86/211 一致——交接文档"等分"系误读，已回退）+ 名称搜索
        QWidget* filterRow = new QWidget(this);
        QHBoxLayout* filterLayout = new QHBoxLayout(filterRow);
        filterLayout->setContentsMargins(0, 0, 0, 0);
        filterLayout->setSpacing(10);	//原型 88+10+213=311 实测
        _typeFilterCombo = new QComboBox(filterRow);
        _typeFilterCombo->setObjectName("mMeasureTypeFilter");
        _typeFilterCombo->setFixedHeight(kControl);
        _typeFilterCombo->setFixedWidth(kFilterW);
        filterLayout->addWidget(_typeFilterCombo);
        _searchEdit = new QLineEdit(filterRow);
        _searchEdit->setObjectName("mMeasureSearchEdit");
        _searchEdit->setClearButtonEnabled(true);
        _searchEdit->setFixedHeight(kControl);
        filterLayout->addWidget(_searchEdit, 1);
        _manageFold->addRow(filterRow);

        //记录列表（复选/名称/类型/显隐/颜色；共享管理表格件 mPanelTable：样式/行高/表头/
        //行线与眼睛、颜色格委托统一（UiPalette/UiMetrics 令牌），空态覆盖层内置；
        //2026-11-16 收编说明：原 #F5F7FA 表头底/#E4E8EE 行线随之归一到共享口径，
        //差异见 doc/管理表格共享控件替换差异表-20261116.md）
        _table = new MxFunctions::mPanelTable(0, 5, this);
        _table->setObjectName("mMeasureTable");
        _table->setEyeColumn(3);
        _table->setColorColumn(4);
        _table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        _table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        _table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        _table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        _table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
        //列宽：复选 26(--std-manager-row)/类型 60/显隐 28/颜色 28（--std-manager-edit-column，
        //M2：行内控件 24 左右各留 2px）
        _table->setColumnWidth(0, 26);
        _table->setColumnWidth(2, 60);
        _table->setColumnWidth(3, 28);
        _table->setColumnWidth(4, 28);
        _table->setSelectionBehavior(QAbstractItemView::SelectRows);
        _table->setSelectionMode(QAbstractItemView::SingleSelection);
        //排序列由点击表头生效
        _table->horizontalHeader()->setSortIndicatorShown(false);
        _table->setFixedHeight(184);	//M2：表头 26 + 最小六行 26×6 + 边框 2
        _manageFold->addRow(_table);

        //表尾：全选/显示/隐藏 靠左，删除（红字）靠右（按钮高 24 = --ui-button-height）
        QWidget* footerRow = new QWidget(this);
        QHBoxLayout* footerLayout = new QHBoxLayout(footerRow);
        footerLayout->setContentsMargins(0, 0, 0, 0);
        footerLayout->setSpacing(6);
        _btnSelectAll = new QPushButton(footerRow);
        _btnSelectAll->setStyleSheet(
            "QPushButton{border:0;background:transparent;color:#424B57;border-radius:2px;padding:0 6px;}"
            "QPushButton:hover{background:#F0F7FF;}"
            "QPushButton:disabled{color:#A6AFBB;background:transparent;}");
        _btnSelectAll->setFixedSize(40, 26);	//全局 QSS width:80 会撑宽，钉死 40（设计稿实测）
        _btnSelectAll->setEnabled(false);
        footerLayout->addWidget(_btnSelectAll);
        //设计截图：全选靠左，显示/隐藏/删除成组靠右
        footerLayout->addStretch();
        _btnShow = new QPushButton(footerRow);
        _btnShow->setStyleSheet("QPushButton{padding:0 6px;}");
        _btnShow->setFixedSize(40, 26);
        _btnShow->setEnabled(false);
        footerLayout->addWidget(_btnShow);
        _btnHide = new QPushButton(footerRow);
        _btnHide->setStyleSheet("QPushButton{padding:0 6px;}");
        _btnHide->setFixedSize(40, 26);
        _btnHide->setEnabled(false);
        footerLayout->addWidget(_btnHide);
        _btnDelete = new QPushButton(footerRow);
        _btnDelete->setStyleSheet(QString("QPushButton{padding:0 6px;}QPushButton:enabled{color:%1;}").arg(kDanger));
        _btnDelete->setFixedSize(40, 26);
        _btnDelete->setEnabled(false);
        footerLayout->addWidget(_btnDelete);
        _btnNew = new QPushButton(footerRow);	//V1.3：新建（主蓝，登记待定义记录）
        _btnNew->setStyleSheet(kPrimaryBtnQss);
        _btnNew->setFixedSize(44, 26);
        footerLayout->addWidget(_btnNew);
        _manageFold->addRow(footerRow);
        connect(_btnNew, &QPushButton::clicked, this, [this]() { beginCreateMeasure(); });

        //管理状态行（共N条/已选M条）
        _managerStatusLabel = new QLabel(this);
        _managerStatusLabel->setStyleSheet(
            QString("QLabel{color:%1;font-size:11px;border:0;background:transparent;}").arg(kMuted));
        _manageFold->addRow(_managerStatusLabel);

        _mainLayout->addWidget(_manageFold);

        /********************测量工具********************/
        _toolFold = new FoldSection(QString::fromUtf8("测量工具"), this);
        //标签列 x12（面板边距 4 + 表单边距 8，对齐原型实测）；顶边距 6 对齐原型
        //首行 y366（标题条下 6px，0.19 轮逐控件实测）；底边距 0（面板底部由
        //_mainLayout 收尾，原型 创建 行下沿距面板底 6）
        if (auto* form = _toolFold->layout())
        {
            form->setContentsMargins(8, 6, 8, 0);
        }

        //标签内容（条目按钮：文字左对齐、箭头靠右，弹窗锚定其下）
        _labelContent = new PanelLabel(QString::fromUtf8("标签内容"), this);
        _labelContent->setFixedWidth(kLabelW);	//原型测量区标签列 84（--parameter-label-width），覆 PanelLabel 默认 90
        _btnLabel = new QPushButton(this);
        {
            _btnLabel->setStyleSheet(
                "QPushButton{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:2px;text-align:left;}"
                "QPushButton:hover{background:#F0F7FF;}");
            _btnLabel->setFixedHeight(26);
            auto* l = new QHBoxLayout(_btnLabel);
            l->setContentsMargins(8, 0, 8, 0);
            _labelBtnText = new QLabel(QString::fromUtf8("自定义"), _btnLabel);
            _labelBtnText->setAttribute(Qt::WA_TransparentForMouseEvents);
            _labelBtnText->setStyleSheet("QLabel{color:#424B57;background:transparent;font-size:12px;}");
            l->addWidget(_labelBtnText);
            l->addStretch();
            QLabel* arrow = new QLabel(QString::fromUtf8("›"), _btnLabel);
            arrow->setAttribute(Qt::WA_TransparentForMouseEvents);
            arrow->setStyleSheet("color:#788391;background:transparent;font-size:12px;");
            l->addWidget(arrow);
        }
        _toolFold->addRow(_labelContent, _btnLabel);

        //测量数据（单下拉：设计 11 项，默认"节点云图"，对齐原型 measurementData）
        _comboBox = new QComboBox(this);
        _comboBox->setObjectName(QString::fromUtf8("mMeasureDataCombo"));
        _comboBox->setFixedHeight(kControl);
        for (int i = 0; i < kMeasureDataCount; ++i)
            _comboBox->addItem(typeName(i, 0));
        _comboBox->setCurrentIndex(kDefaultMeasureData);
        _dataLabel = new PanelLabel(QString::fromUtf8("测量数据"), this);
        _dataLabel->setFixedWidth(kLabelW);
        _toolFold->addRow(_dataLabel, _comboBox);

        //选择节点（L10 全宽拾取按钮：点击武装拾取，文本随状态切换）
        _btnPickNode = new QPushButton(this);
        _btnPickNode->setObjectName(QString::fromUtf8("mMeasurePickNode"));
        _btnPickNode->setCheckable(true);
        _btnPickNode->setFixedHeight(kControl);
        _btnPickNode->setCursor(Qt::PointingHandCursor);
        _btnPickNode->setStyleSheet(
            "QPushButton{background:#E8F3FF;border:1px solid #91CAFF;color:#176BA6;border-radius:2px;}"
            "QPushButton:hover{background:#D0E7FD;}"
            "QPushButton:checked{background:#D0E7FD;font-weight:600;}"
            "QPushButton:disabled{background:#F5F7FA;border-color:#DCE2E9;color:#A6AFBB;}");
        _pickLabel = new PanelLabel(QString::fromUtf8("选择节点"), this);
        _pickLabel->setFixedWidth(kLabelW);
        _toolFold->addRow(_pickLabel, _btnPickNode);
        _toolFold->refreshLayout();

        //创建行：自动创建/自动分组 + 创建按钮（原型：自动分组置灰待接入）
        QWidget* createRow = new QWidget(this);
        QHBoxLayout* createLayout = new QHBoxLayout(createRow);
        createLayout->setContentsMargins(0, 0, 0, 0);
        createLayout->setSpacing(6);
        _checkAutoCreate = new QCheckBox(createRow);
        _checkAutoCreate->setObjectName("mMeasureAutoCreate");
        createLayout->addWidget(_checkAutoCreate);
        _checkAutoGroup = new QCheckBox(createRow);
        _checkAutoGroup->setObjectName("mMeasureAutoGroup");
        _checkAutoGroup->setEnabled(false);
        createLayout->addWidget(_checkAutoGroup);
        createLayout->addStretch();
        _btnCreate = new QPushButton(createRow);
        _btnCreate->setObjectName("mMeasureCreateBtn");
        _btnCreate->setStyleSheet(kPrimaryBtnQss);
        _btnCreate->setFixedSize(44, kControl);	//原型 symmetry-execute 实测 44×26（--std-control）
        createLayout->addWidget(_btnCreate);
        _toolFold->addRow(createRow);
        //V1.3：自动创建/创建行由统一编辑契约（取消编辑/保存修改/即时保存）取代
        _toolFold->setRowVisible(createRow, false);

        //测量状态行（拾取中/最新结果）；空文案整行收起（对齐原型空态不留白）。
        //显隐必须走 setRowVisible 行级摘/插：直接 setVisible 不会同步 FoldSection
        //内容高度帽（展开时定格），表单只能在旧高度内挤压上方行距（0.20 轮实证）
        _statusLabel = new QLabel(this);
        _statusLabel->setStyleSheet(
            QString("QLabel{color:%1;font-size:11px;border:0;background:transparent;}").arg(kMuted));
        _statusLabel->setVisible(false);
        _toolFold->addRow(_statusLabel);

        /******** 统一编辑契约操作区（V1.3：状态行之后） ********/
        _editor = new mPostObjectEditController(this);
        _editor->setSnapshotFn([this]() { return _editingId >= 0 ? editingSignature() : QString(); });
        _editor->setSaveFn([this](bool immediate) { return commitEditMeasure(immediate); });
        _editor->setCancelFn([this]() { cancelEditMeasure(); });
        _editor->setCurrentNameFn([this]()
           	{
                const MeasureRecord* rec = editingRecord();
                return rec ? rec->name : QString::fromUtf8("当前测量");
            });
        connect(_editor, &mPostObjectEditController::saveRequested, this,
            [this](bool immediate) { if (!immediate) commitEditMeasure(false); });
        connect(_editor, &mPostObjectEditController::cancelRequested, this, [this]() { cancelEditMeasure(); });
        _editor->attachFooter(nullptr, -1, _toolFold);
        _toolFold->addRow(_editor->footerWidget());

        _mainLayout->addWidget(_toolFold);

        _mainLayout->addStretch();

        //原型测量区行距实测 32（控件 26 + 间距 6，逐控件对比实测）：实例级覆写
        //表单行距 10→6，不改共享 FoldSection 默认值（踩坑 10）；须在折叠组创建后执行
        for (FoldSection* fold : { _manageFold, _toolFold })
        {
            if (auto* form = fold->findChild<QFormLayout*>())
                form->setVerticalSpacing(6);
        }
    }

    void mPostMeasureTool::ensureLabelPopup()
    {
        if (_labelPopup) return;

        _labelPopup = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
        //样式必须带 ID 选择器：无选择器规则会级联到全部子部件
        _labelPopup->setObjectName("mLabelPopup");
        _labelPopup->setAttribute(Qt::WA_StyledBackground, true);
        //对齐原型 .object-picker-popup 基类：蓝描边（--std-focus #1890FF）1px +
        //圆角 3px（--ui-popup-radius）。注意详情弹窗有专属覆盖变灰，本弹窗没有——
        //两弹窗描边色不可互相抄（0.21 轮）
        _labelPopup->setStyleSheet("#mLabelPopup{background:#FFFFFF;border:1px solid #1890FF;border-radius:3px;}");
        _labelPopup->setFixedWidth(260);	//原型标签弹窗实测 260×405（交接文档 224 系旧值）

        auto* popupLayout = new QVBoxLayout(_labelPopup);
        //同复核弹窗：边距避开半径 3 弧段内凹深度（约 1.4px，取 2）
        popupLayout->setContentsMargins(2, 2, 2, 2);
        popupLayout->setSpacing(0);

        //标题栏（高 32，--std 规范）
        QWidget* head = new QWidget(_labelPopup);
        head->setFixedHeight(kPopupHeadH);
        head->setStyleSheet("border-bottom:1px solid #E4E8EE;");
        QHBoxLayout* headLayout = new QHBoxLayout(head);
        headLayout->setContentsMargins(8, 0, 8, 0);
        headLayout->setSpacing(4);
        QLabel* title = new QLabel(QString::fromUtf8("标签内容"), head);
        title->setObjectName("mLabelPopupTitle");
        title->setStyleSheet("QLabel{color:#424B57;font-size:12px;font-weight:600;border:0;background:transparent;}");
        headLayout->addWidget(title);
        headLayout->addStretch();
        QPushButton* closeBtn = new QPushButton(QString::fromUtf8("×"), head);
        closeBtn->setFixedSize(24, 24);
        closeBtn->setFlat(true);
        closeBtn->setStyleSheet("QPushButton{border:0;background:transparent;color:#788391;font-size:16px;}"
            "QPushButton:hover{background:#F0F7FF;color:#424B57;}");
        headLayout->addWidget(closeBtn);
        popupLayout->addWidget(head);
        connect(closeBtn, &QPushButton::clicked, _labelPopup, &QWidget::close);

        //表单体：显示 / 全局 两个可折叠分组（对齐原型 details 结构，
        //复用共享 FoldSection，标题条带箭头可点击折叠）
        QWidget* body = new QWidget(_labelPopup);
        body->setObjectName("mLabelBody");	//折叠收尾的弹窗尺寸同步按此查找（见 eventFilter）
        QGridLayout* gl = new QGridLayout(body);
        //FoldSection 自带内容边距 12（分组标题条/字段 x 实测由此形成），body 不再加边距
        gl->setContentsMargins(0, 0, 0, 0);
        gl->setHorizontalSpacing(8);
        //组间距 6：收起态=标题条到标题条（原型 显示29..55→全局61 实测）；展开态
        //再加显示组表单底边距 6（下），合计 12（原型 角度行215→全局227 实测）
        gl->setVerticalSpacing(6);

        auto sectionLabel = [body](const QString& text) {
            auto* lbl = new QLabel(text, body);
            lbl->setObjectName("mLabelField_" + text);	//语言切换按此查找
            lbl->setFixedWidth(kLabelW);	//84，--std-label-width
            lbl->setStyleSheet(QString("QLabel{color:%1;font-size:12px;border:0;background:transparent;}").arg(kLabel));
            return lbl;
        };

        //── 显示分组（可折叠 details） ──
        FoldSection* showSec = new FoldSection(QString::fromUtf8("显示"), body);
        showSec->setObjectName("mLabelShowSec");
        if (auto* form = showSec->findChild<QFormLayout*>())
        {
            form->setVerticalSpacing(6);	//原型行距实测 32（26+6，逐控件对比）
            auto m = form->contentsMargins();	//首行 y61 实测：内容顶边距 10→6；底 10→6
            form->setContentsMargins(m.left(), 6, m.right(), 6);	//+组间距 6=展开组间 12（原型实测）
        }
        gl->addWidget(showSec, 0, 0, 1, 2);

        //字号（下拉 + 标签颜色色块）
        QWidget* fontRow = new QWidget(showSec);
        QHBoxLayout* fontLayout = new QHBoxLayout(fontRow);
        fontLayout->setContentsMargins(0, 0, 0, 0);
        fontLayout->setSpacing(6);
        _lpFontCombo = new QComboBox(fontRow);
        _lpFontCombo->setObjectName("mLabelFontCombo");
        _lpFontCombo->setFixedHeight(26);
        for (int size : { 8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 32, 36, 48, 72 })
            _lpFontCombo->addItem(QString::number(size));
        _lpFontCombo->setCurrentText("12");
        fontLayout->addWidget(_lpFontCombo, 1);
        _lpColorSwatch = new QPushButton(fontRow);
        _lpColorSwatch->setObjectName("mLabelColorSwatch");
        _lpColorSwatch->setFixedSize(24, 24);	//原型 .label-color 24×24
        _lpColorSwatch->setToolTip(QString::fromUtf8("标签颜色"));
        fontLayout->addWidget(_lpColorSwatch);
        showSec->addRow(sectionLabel(QString::fromUtf8("字号")), fontRow);
        //色块取色走按压过滤（eventFilter 消费按压打开取色器，防止标签弹窗连带关闭）
        _lpColorSwatch->installEventFilter(this);

        //显示项（编号/名称，对齐原型 measurement-pair 连体分段格：选中蓝底蓝字）
        QWidget* showRow = new QWidget(showSec);
        showRow->setAttribute(Qt::WA_StyledBackground, true);
        showRow->setStyleSheet(kPairSegQss);
        showRow->setFixedHeight(kControl);
        QHBoxLayout* showLayout = new QHBoxLayout(showRow);
        showLayout->setContentsMargins(0, 0, 0, 0);
        showLayout->setSpacing(0);
        _lpShowId = new QCheckBox(showRow);
        _lpShowId->setObjectName("mLabelShowId");
        _lpShowId->setChecked(true);
        showLayout->addWidget(_lpShowId, 1);
        _lpShowName = new QCheckBox(showRow);
        _lpShowName->setObjectName("mLabelShowName");
        _lpShowName->setProperty("cellRole", "last");
        showLayout->addWidget(_lpShowName, 1);
        showSec->addRow(sectionLabel(QString::fromUtf8("显示")), showRow);

        //幅值
        _lpMagnitudeCombo = new QComboBox(showSec);
        _lpMagnitudeCombo->setObjectName("mLabelMagnitude");
        _lpMagnitudeCombo->setFixedHeight(26);
        _lpMagnitudeCombo->setFixedWidth(142);	//原型实测 142
        showSec->addRow(sectionLabel(QString::fromUtf8("幅值")), _lpMagnitudeCombo);

        //数值（格式 + 精度）
        QWidget* numRow = new QWidget(showSec);
        QHBoxLayout* numLayout = new QHBoxLayout(numRow);
        numLayout->setContentsMargins(0, 0, 0, 0);
        numLayout->setSpacing(6);
        _lpFormatCombo = new QComboBox(numRow);
        _lpFormatCombo->setObjectName("mLabelFormat");
        _lpFormatCombo->setFixedHeight(26);
        _lpFormatCombo->setFixedWidth(71);	//原型实测 71
        numLayout->addWidget(_lpFormatCombo);
        _lpPrecisionSpin = new QSpinBox(numRow);
        _lpPrecisionSpin->setObjectName("mLabelPrecision");
        _lpPrecisionSpin->setRange(0, 15);
        _lpPrecisionSpin->setValue(3);
        _lpPrecisionSpin->setFixedHeight(26);
        numLayout->addWidget(_lpPrecisionSpin, 1);
        showSec->addRow(sectionLabel(QString::fromUtf8("数值")), numRow);

        //角度单位（度数/弧度，连体分段格）
        QWidget* angleRow = new QWidget(showSec);
        angleRow->setAttribute(Qt::WA_StyledBackground, true);
        angleRow->setStyleSheet(kPairSegQss);
        angleRow->setFixedHeight(kControl);
        QHBoxLayout* angleLayout = new QHBoxLayout(angleRow);
        angleLayout->setContentsMargins(0, 0, 0, 0);
        angleLayout->setSpacing(0);
        _lpDegreeRadio = new QRadioButton(angleRow);
        _lpDegreeRadio->setObjectName("mLabelDegree");
        _lpDegreeRadio->setChecked(true);
        angleLayout->addWidget(_lpDegreeRadio, 1);
        _lpRadianRadio = new QRadioButton(angleRow);
        _lpRadianRadio->setObjectName("mLabelRadian");
        _lpRadianRadio->setProperty("cellRole", "last");
        angleLayout->addWidget(_lpRadianRadio, 1);
        showSec->addRow(sectionLabel(QString::fromUtf8("角度")), angleRow);

        //── 全局分组（可折叠 details） ──
        FoldSection* globalSec = new FoldSection(QString::fromUtf8("全局"), body);
        globalSec->setObjectName("mLabelGlobalSec");
        if (auto* form = globalSec->findChild<QFormLayout*>())
            form->setVerticalSpacing(6);	//同显示分组
        gl->addWidget(globalSec, 1, 0, 1, 2);

        //填充（复选 + 色块紧跟，对齐原型）
        QWidget* fillRow = new QWidget(globalSec);
        QHBoxLayout* fillLayout = new QHBoxLayout(fillRow);
        fillLayout->setContentsMargins(0, 0, 0, 0);
        fillLayout->setSpacing(6);
        _lpFillCheck = new QCheckBox(fillRow);
        _lpFillCheck->setObjectName("mLabelFill");
        _lpFillCheck->setChecked(true);
        fillLayout->addWidget(_lpFillCheck);
        _lpFillSwatch = new QPushButton(fillRow);
        fillLayout->addWidget(_lpFillSwatch);
        fillLayout->addStretch();
        _lpFillSwatch->setObjectName("mLabelFillSwatch");
        _lpFillSwatch->setFixedSize(24, 24);	//原型 .label-color 24×24
        _lpFillSwatch->setToolTip(QString::fromUtf8("填充颜色"));
        globalSec->addRow(fillRow);
        _lpFillSwatch->installEventFilter(this);
        //对齐原型 updateMeasurementFillColor：未勾选填充时填充颜色禁用
        connect(_lpFillCheck, &QCheckBox::toggled, _lpFillSwatch, &QPushButton::setEnabled);

        //自动隐藏 / 显示前缀
        _lpAutoHideCheck = new QCheckBox(globalSec);
        _lpAutoHideCheck->setObjectName("mLabelAutoHide");
        globalSec->addRow(_lpAutoHideCheck);
        _lpPrefixCheck = new QCheckBox(globalSec);
        _lpPrefixCheck->setObjectName("mLabelPrefix");
        _lpPrefixCheck->setChecked(true);
        globalSec->addRow(_lpPrefixCheck);

        //折叠收尾同步弹窗高度（对齐原型 details：浏览器 reflow 让弹窗高度随内容）。
        //Qt 顶层弹窗不随子控件 sizeHint 变化自动缩放——收起后弹窗保持原高，剩余
        //空间被布局摊给分组（标题条拉高/底部留白），展开则被旧高度裁切。FoldSection
        //以 0 时长动画收尾，内容件 Hide（收起定型）/Resize（展开定型）时尺寸已最终，
        //经 eventFilter 延迟一拍按最新 sizeHint 纠正弹窗尺寸
        for (FoldSection* sec : { showSec, globalSec })
        {
            if (QWidget* content = sec->contentWidget())
            {
                content->setObjectName(sec == showSec ? "mLabelShowSecBody" : "mLabelGlobalSecBody");
                content->installEventFilter(this);
            }
        }

        popupLayout->addWidget(body);

        //底部按钮行：恢复默认（扁平）+ 确认（主操作蓝）
        //对齐原型 .measurement-label-footer：padding 8 + 顶部分隔线 + flex-end
        QWidget* footer = new QWidget(_labelPopup);
        footer->setStyleSheet("border-top:1px solid #E4E8EE;");
        QHBoxLayout* footerLayout = new QHBoxLayout(footer);
        footerLayout->setContentsMargins(8, 14, 8, 8);
        footerLayout->setSpacing(6);
        footerLayout->addStretch();
        _lpResetBtn = new QPushButton(footer);
        _lpResetBtn->setObjectName("mLabelResetBtn");
        _lpResetBtn->setStyleSheet(
            "QPushButton{border:0;background:transparent;color:#424B57;border-radius:2px;}"
            "QPushButton:hover{background:#F0F7FF;}");
        _lpResetBtn->setFixedSize(62, 26);	//原型实测 62×26
        footerLayout->addWidget(_lpResetBtn);
        _lpOkBtn = new QPushButton(footer);
        _lpOkBtn->setObjectName("mLabelOkBtn");
        _lpOkBtn->setStyleSheet(kPrimaryBtnQss);
        _lpOkBtn->setFixedSize(44, 26);	//原型实测 44×26
        footerLayout->addWidget(_lpOkBtn);
        popupLayout->addWidget(footer);

        connect(_lpResetBtn, &QPushButton::clicked, this, &mPostMeasureTool::slot_labelReset);
        connect(_lpOkBtn, &QPushButton::clicked, this, &mPostMeasureTool::slot_labelConfirm);

        applyPopupsLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
    }

    void mPostMeasureTool::ensureReviewPopup()
    {
        if (_reviewPopup) return;

        _reviewPopup = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
        _reviewPopup->setObjectName("mReviewPopup");
        _reviewPopup->setAttribute(Qt::WA_StyledBackground, true);
        //对齐原型 .measurement-details-popup：该弹窗把基类蓝描边覆盖为控制灰
        //描边蓝 1px #1890FF + 圆角 2px（用户原型截图实证为蓝描边，0.21 轮定案）
        _reviewPopup->setStyleSheet("#mReviewPopup{background:#FFFFFF;border:1px solid #1890FF;border-radius:2px;}");
        //对齐原型 .measurement-details-popup：宽 1000；高 = 标题栏 32 + 网格 446
        //+ 行距 32 + 内边距 20 + 防弧段边距 6 = 536（原型 532，差值为防弧段边距）
        _reviewPopup->setFixedSize(1000, 536);

        auto* popupLayout = new QVBoxLayout(_reviewPopup);
        //边距须大于圆角弧段内凹深度（r=2 时 ≈1.1px，取 3）：否则全宽子控件从 (1,1)
        //起盖住四角弧段，描边在拐角断线（原型为 HTML 自带裁剪，无此问题）
        popupLayout->setContentsMargins(3, 3, 3, 3);
        popupLayout->setSpacing(0);

        //标题栏（高 32，--std 规范）
        QWidget* head = new QWidget(_reviewPopup);
        head->setFixedHeight(kPopupHeadH);
        head->setStyleSheet("border-bottom:1px solid #E4E8EE;");
        QHBoxLayout* headLayout = new QHBoxLayout(head);
        headLayout->setContentsMargins(8, 0, 8, 0);
        headLayout->setSpacing(4);
        QLabel* title = new QLabel(QString::fromUtf8("测量数据详细信息"), head);
        title->setObjectName("mReviewTitle");
        title->setStyleSheet("QLabel{color:#424B57;font-size:12px;font-weight:600;border:0;background:transparent;}");
        headLayout->addWidget(title);
        headLayout->addStretch();
        QPushButton* closeBtn = new QPushButton(QString::fromUtf8("×"), head);
        closeBtn->setFixedSize(24, 24);
        closeBtn->setFlat(true);
        closeBtn->setStyleSheet("QPushButton{border:0;background:transparent;color:#788391;font-size:16px;}"
            "QPushButton:hover{background:#F0F7FF;color:#424B57;}");
        headLayout->addWidget(closeBtn);
        popupLayout->addWidget(head);
        connect(closeBtn, &QPushButton::clicked, _reviewPopup, &QWidget::close);

        //主体：左右两栏（对齐原型 measurement-review-layout：左列表 + 右绘图区）
        QWidget* body = new QWidget(_reviewPopup);
        QHBoxLayout* bodyLayout = new QHBoxLayout(body);
        bodyLayout->setContentsMargins(10, 10, 10, 10);
        bodyLayout->setSpacing(10);

        //──── 左栏：栏标题/筛选/表格/选项行/目标行 ────
        QWidget* leftCol = new QWidget(body);
        QVBoxLayout* leftLayout = new QVBoxLayout(leftCol);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(5);	//原型实测：表格→目标行 5（452→457 逐控件对比）

        QLabel* leftTitle = new QLabel(QString::fromUtf8("测量数据"), leftCol);
        leftTitle->setObjectName("mReviewLeftTitle");
        leftTitle->setFixedHeight(28);
        leftTitle->setStyleSheet(
            QString("QLabel{background:%1;color:%2;font-size:12px;font-weight:600;border:0;padding-left:8px;}")
            .arg(kHeadBg, kText));
        leftLayout->addWidget(leftTitle);

        //筛选行：类型筛选(88) + 搜索
        QWidget* filterRow = new QWidget(leftCol);
        QHBoxLayout* filterLayout = new QHBoxLayout(filterRow);
        filterLayout->setContentsMargins(0, 0, 0, 0);
        filterLayout->setSpacing(6);
        _rvTypeCombo = new QComboBox(filterRow);
        _rvTypeCombo->setObjectName("mReviewTypeFilter");
        _rvTypeCombo->setFixedHeight(kControl);
        _rvTypeCombo->setFixedWidth(kFilterW);
        filterLayout->addWidget(_rvTypeCombo);
        _rvSearchEdit = new QLineEdit(filterRow);
        _rvSearchEdit->setObjectName("mReviewSearch");
        _rvSearchEdit->setClearButtonEnabled(true);
        _rvSearchEdit->setFixedHeight(kControl);
        filterLayout->addWidget(_rvSearchEdit, 1);
        leftLayout->addWidget(filterRow);

        //记录列表（实体▶/编号/颜色/类型/坐标系；列宽对齐原型 colgroup 40/38/90/84；
        //共享管理表格件：表头高 28/行高 34 为原型 .measurement-review-table 专值，
        //经 setHeaderHeight/setBodyRowHeight 覆写（区别于管理表 26/26 的 M2 口径））
        _rvTable = new MxFunctions::mPanelTable(0, 5, leftCol);
        _rvTable->setObjectName("mReviewTable");
        _rvTable->setHeaderHeight(28);
        _rvTable->setBodyRowHeight(34);
        _rvTable->setColorColumn(2);
        _rvTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        _rvTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        _rvTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        _rvTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        _rvTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
        _rvTable->setColumnWidth(1, 40);
        _rvTable->setColumnWidth(2, 38);
        _rvTable->setColumnWidth(3, 90);
        _rvTable->setColumnWidth(4, 84);
        _rvTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        _rvTable->setSelectionMode(QAbstractItemView::SingleSelection);
        //空态不设占位文案：原型详情列表空态仅剩表头（tbody 清空，无"暂无测量数据"行，
        //0.19 轮逐控件对齐实测）——不调 setEmptyTexts 即不启用覆盖层
        _rvTable->setFixedHeight(340);
        leftLayout->addWidget(_rvTable);

        //目标行：绘制位置（预览图/曲线窗口）+ 绘制（设计截图：目标行在选项行上方）
        QWidget* targetRow = new QWidget(leftCol);
        QHBoxLayout* targetLayout = new QHBoxLayout(targetRow);
        targetLayout->setContentsMargins(0, 0, 0, 0);
        targetLayout->setSpacing(10);	//原型 预览图 x357 实测（右簇：标签…拉伸…下拉10绘制44）
        QLabel* targetLabel = new QLabel(QString::fromUtf8("绘制位置"), targetRow);
        targetLabel->setObjectName("mReviewTargetLabel");
        targetLabel->setStyleSheet("QLabel{color:#5C6674;font-size:12px;border:0;background:transparent;}");
        targetLayout->addStretch();	//原型：标签+下拉+绘制整体靠右成簇（标签 x265/下拉 x357/钮 x463 实测）
        targetLayout->addWidget(targetLabel);
        _rvTargetCombo = new QComboBox(targetRow);
        _rvTargetCombo->setObjectName("mReviewTargetCombo");
        _rvTargetCombo->setFixedHeight(26);
        _rvTargetCombo->setFixedWidth(98);	//原型实测 98
        targetLayout->addWidget(_rvTargetCombo);
        _rvPlotBtn = new QPushButton(targetRow);
        _rvPlotBtn->setObjectName("mReviewPlotBtn");
        _rvPlotBtn->setStyleSheet(kPrimaryBtnQss);
        _rvPlotBtn->setFixedSize(44, 26);	//原型实测 44（同面板创建钮规格）
        _rvPlotBtn->setEnabled(false);
        targetLayout->addWidget(_rvPlotBtn);
        leftLayout->addWidget(targetRow);

        //选项行：实时关联(置灰待接入)/单条曲线/收起绘图（对齐原型 measurement-review-options：
        //三个控件整体靠右成簇，原型 x 实测 251/341/445——stretch 前置实现右聚）
        QWidget* optRow = new QWidget(leftCol);
        QHBoxLayout* optLayout = new QHBoxLayout(optRow);
        optLayout->setContentsMargins(0, 0, 0, 0);
        optLayout->setSpacing(6);
        optLayout->addStretch();
        _rvLiveLink = new QCheckBox(optRow);
        _rvLiveLink->setObjectName("mReviewLiveLink");
        _rvLiveLink->setEnabled(false);
        optLayout->addWidget(_rvLiveLink);
        _rvSingleCurve = new QCheckBox(optRow);
        _rvSingleCurve->setObjectName("mReviewSingleCurve");
        _rvSingleCurve->setChecked(true);
        optLayout->addWidget(_rvSingleCurve);
        optLayout->addStretch();
        _btnCollapsePlot = new QPushButton(optRow);
        _btnCollapsePlot->setObjectName("mReviewCollapseBtn");
        _btnCollapsePlot->setText(QString::fromUtf8("收起绘图"));	//语言函数按折叠状态刷新
        //对齐原型 .label-defaults-button：白底描边钮（高 26），悬停浅蓝
        _btnCollapsePlot->setStyleSheet(
            "QPushButton{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:2px;color:#424B57;}"
            "QPushButton:hover{background:#F0F7FF;}");
        _btnCollapsePlot->setFixedSize(62, 26);	//原型实测 62
        optLayout->addWidget(_btnCollapsePlot);
        leftLayout->addWidget(optRow);
        //左栏 496px 定宽（原型 --measurement-review-list-width），绘图区吃剩余宽度
        leftCol->setFixedWidth(496);
        bodyLayout->addWidget(leftCol);

        //──── 右栏：绘图区（占位；外部统一绘图组件后续接入） ────
        _plotSection = new QWidget(body);
        QVBoxLayout* plotLayout = new QVBoxLayout(_plotSection);
        plotLayout->setContentsMargins(0, 0, 0, 0);
        plotLayout->setSpacing(6);

        QLabel* plotTitle = new QLabel(QString::fromUtf8("绘图"), _plotSection);
        plotTitle->setObjectName("mReviewPlotTitle");
        plotTitle->setFixedHeight(28);
        plotTitle->setStyleSheet(
            QString("QLabel{background:%1;color:%2;font-size:12px;font-weight:600;border:0;padding-left:8px;}")
            .arg(kHeadBg, kText));
        plotLayout->addWidget(plotTitle);

        //轴工具条：X轴(时间/帧号) + Y轴(X/Y/Z/合量)（对齐原型 plot-axis-toolbar）
        QWidget* axisRow = new QWidget(_plotSection);
        QHBoxLayout* axisLayout = new QHBoxLayout(axisRow);
        axisLayout->setContentsMargins(0, 0, 0, 0);
        axisLayout->setSpacing(6);
        //轴角图标 + X轴（对齐原型 plot-axis-control 前置 SVG 图标）
        QLabel* xIcon = new QLabel(axisRow);
        xIcon->setPixmap(axisCornerIcon(false, QColor(kLabel)));
        xIcon->setFixedSize(14, 14);
        axisLayout->addWidget(xIcon);
        QLabel* xLabel = new QLabel(QString::fromUtf8("X轴"), axisRow);
        xLabel->setObjectName("mReviewXLabel");
        xLabel->setStyleSheet("QLabel{color:#5C6674;font-size:12px;border:0;background:transparent;}");
        axisLayout->addWidget(xLabel);
        _rvXCombo = new QComboBox(axisRow);
        _rvXCombo->setObjectName("mReviewXCombo");
        _rvXCombo->setFixedHeight(kControl);
        _rvXCombo->setFixedWidth(182);	//原型 select 宽 182
        axisLayout->addWidget(_rvXCombo);
        axisLayout->addStretch();
        QLabel* yIcon = new QLabel(axisRow);
        yIcon->setPixmap(axisCornerIcon(true, QColor(kLabel)));
        yIcon->setFixedSize(14, 14);
        axisLayout->addWidget(yIcon);
        QLabel* yLabel = new QLabel(QString::fromUtf8("Y轴"), axisRow);
        yLabel->setObjectName("mReviewYLabel");
        yLabel->setStyleSheet("QLabel{color:#5C6674;font-size:12px;border:0;background:transparent;}");
        axisLayout->addWidget(yLabel);
        _rvYCombo = new QComboBox(axisRow);
        _rvYCombo->setObjectName("mReviewYCombo");
        _rvYCombo->setFixedHeight(kControl);
        _rvYCombo->setFixedWidth(183);	//原型 select 宽 183
        axisLayout->addWidget(_rvYCombo);
        //轴变量切换即重绘（原型 D9"数据修改后刷新绘图"）；plotRecords 内部读取
        //两个下拉的当前值（X=帧号/时间，Y=分量）
        connect(_rvXCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mPostMeasureTool::slot_reviewAxisChanged);
        connect(_rvYCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mPostMeasureTool::slot_reviewAxisChanged);
        plotLayout->addWidget(axisRow);

        //绘图区：PlotView 曲线图（绘制位置=预览图的绘制目标，与曲线窗口同款绘图组件）
        //+ 空态占位（暂无测量结果）；绘制后显示曲线图、占位隐藏
        _rvPlotView = new PlotView::mPlotView(_plotSection);
        _rvPlotView->setObjectName("mReviewPlotView");
        plotLayout->addWidget(_rvPlotView, 1);
        _rvPlotView->hide();

        _plotPlaceholder = new QWidget(_plotSection);
        _plotPlaceholder->setObjectName("mReviewPlotPlaceholder");
        _plotPlaceholder->setStyleSheet("QWidget{background:#FFFFFF;border:1px solid #DCE2E9;}");
        QVBoxLayout* phLayout = new QVBoxLayout(_plotPlaceholder);
        phLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* phHint = new QLabel(QString::fromUtf8("暂无测量结果"), _plotPlaceholder);
        phHint->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);	//原型空态文案贴绘图区底部
        phHint->setStyleSheet(QString("QLabel{color:%1;font-size:11px;border:0;background:transparent;padding-bottom:6px;}").arg(kMuted));
        phLayout->addWidget(phHint);
        plotLayout->addWidget(_plotPlaceholder, 1);
        bodyLayout->addWidget(_plotSection, 1);

        popupLayout->addWidget(body);

        connect(_rvSearchEdit, &QLineEdit::textChanged, this, &mPostMeasureTool::slot_reviewSearchChanged);
        connect(_rvTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mPostMeasureTool::slot_reviewTypeChanged);
        connect(_rvTable, &QTableWidget::itemSelectionChanged, this, [this]() { syncReviewState(); });
        connect(_rvSingleCurve, &QCheckBox::toggled, this, [this]() { syncReviewState(); });
        connect(_rvPlotBtn, &QPushButton::clicked, this, &mPostMeasureTool::slot_plotClicked);
        connect(_btnCollapsePlot, &QPushButton::clicked, this, &mPostMeasureTool::slot_plotCollapseClicked);

        applyPopupsLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
    }

    void mPostMeasureTool::ensureColorPopup()
    {
        if (_colorPicker)
            return;
        _colorPicker = new FbdColorPicker(this);
        connect(_colorPicker, &FbdColorPicker::colorChanged, this, [this](const QColor& color) {
            //标签内容色块取色
            if (_colorTarget == 1)
            {
                _labelDraft.color = color;
                applySwatchColor(_lpColorSwatch, color);
                return;
            }
            if (_colorTarget == 2)
            {
                _labelDraft.fillColor = color;
                applySwatchColor(_lpFillSwatch, color);
                return;
            }
            //实时同步到记录数据与列表颜色格
            if (_colorTarget != 0)
                return;
            const int di = dataIndexById(_colorRecordId);
            if (di < 0)
                return;
            _records[di].color = color;
            syncMeasureCache();
            for (int r = 0; r < _table->rowCount(); ++r)
            {
                QTableWidgetItem* nameIt = _table->item(r, 1);
                if (nameIt && nameIt->data(Qt::UserRole).toInt() == _colorRecordId)
                {
                    //排序激活时改颜色格文本会触发行重排（拖动取色期间连续跳行），
                    //与其他变异路径一致：关排序改完再开
                    _table->setSortingEnabled(false);
                    renderRow(r);
                    _table->setSortingEnabled(true);
                    _table->horizontalHeader()->setSortIndicatorShown(false);
                    updateMeasureAnnotations();	//颜色列 → 3D 标注线/文字颜色同步
                    break;
                }
            }
            });
    }

    void mPostMeasureTool::popupUnder(QWidget* popup, QWidget* anchor)
    {
        if (!popup || !anchor)
            return;
        popup->adjustSize();
        QPoint pos = anchor->mapToGlobal(QPoint(0, anchor->height() + 2));
        QScreen* scr = QGuiApplication::screenAt(anchor->mapToGlobal(anchor->rect().center()));
        if (!scr) scr = QGuiApplication::primaryScreen();
        if (scr)
        {
            const QRect avail = scr->availableGeometry();
            pos.setX(qBound(avail.left() + 8, pos.x(), qMax(avail.left() + 8, avail.right() - popup->width() - 8)));
            pos.setY(qBound(avail.top() + 8, pos.y(), qMax(avail.top() + 8, avail.bottom() - popup->height() - 8)));
        }
        popup->move(pos);
        popup->show();
    }

    void mPostMeasureTool::popupCenter(QWidget* popup)
    {
        if (!popup)
            return;
        popup->adjustSize();
        QScreen* scr = QGuiApplication::screenAt(mapToGlobal(rect().center()));
        if (!scr) scr = QGuiApplication::primaryScreen();
        QRect avail(0, 0, 1280, 800);
        if (scr)
            avail = scr->availableGeometry();
        QPoint pos((avail.width() - popup->width()) / 2 + avail.left(),
            (avail.height() - popup->height()) / 2 + avail.top());
        pos.setX(qMax(avail.left() + 8, pos.x()));
        pos.setY(qMax(avail.top() + 8, pos.y()));
        popup->move(pos);
        popup->show();
    }

    void mPostMeasureTool::applySwatchColor(QPushButton* swatch, const QColor& color)
    {
        if (!swatch)
            return;
        //禁用态（填充未勾选）：面板底色+灰边，启用后自动恢复当前颜色
        swatch->setStyleSheet(QString("QPushButton{background:%1;border:1px solid %2;border-radius:2px;}"
            "QPushButton:hover:enabled{border-color:%3;}"
            "QPushButton:disabled{background:%4;border-color:%2;}")
            .arg(color.name(), QString::fromLatin1(kCtrlBorder),
                 QString::fromLatin1(kPrimary), QString::fromLatin1(kHeadBg)));
    }

    void mPostMeasureTool::setPanelPickAndSignal(bool isshow)
    {
        if (!_isCurrStateValid) return;

        if (_isCurrPostViewport) {
            // 后处理
            if (_postRend == nullptr) return;

            auto postRender = _postRend->getPostRender();
            if (!postRender || !postRender.get()) return;

            auto sc = _postRend->getSelectionCaculator();       //getSelectionContainer();//
            if (sc == nullptr) return;

            if (isshow)
            {
                _buttonPickFilters[_btnPickNode] = mxr::PickFilter::PickMeshNode;

                if (_pickConn)
                    disconnect(_pickConn);

                //if (!_pickConn)
                {
                    _pickConn = connect(sc, &mxr::PostSelectionCalculator::signals_FinishPick,
                        this, &mPostMeasureTool::slot_finishPick);
                    //_pickConn = connect(sc, &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPostMeasureTool::slot_finishPick);
                }

                //帧变化 → 3D 测量标注跟随当前帧（节点位移后的坐标）；激活即按当前帧重建
                if (_frameConn)
                    disconnect(_frameConn);
                // S5 门面直接替换：标注重估 hook 单通道接门面 frameChanged(frameId, source)
                //（B1b(M10) 语义延续）；PlayAdvance（播放推进）才即时重估，手动切帧
                //（UserSeek）由 postCloudFrameChanged 常驻链承接重估。播放中新建放行：
                //订阅不再区分播放/非播放态。改动集中于此单一订阅段
                _frameConn = connect(_postRend->getPostRender().get(), &mxr::REBUILD_PostRender::frameChanged,
                    this, [this](int frameId, mxr::FrameChangeSource source)
                    {
                        Q_UNUSED(frameId);
                        if (source == mxr::FrameChangeSource::PlayAdvance)
                            updateMeasureAnnotations();
                    }, Qt::QueuedConnection);
                //相机变化 → 自动隐藏标签按视点重评估（updateMeasureAnnotations 内
                //仅存在开启自动隐藏的记录时才做遮挡判定，代价可控）。
                //QueuedConnection：重评估排队到事件循环执行，不在绘制过程内联
                //嵌套（渲染层已做状态变化去重，双保险防递归）
                if (_cameraConn)
                    disconnect(_cameraConn);
                _cameraConn = connect(postRender.get(), &mxr::REBUILD_PostRender::cameraChanged,
                    this, &mPostMeasureTool::updateMeasureAnnotations, Qt::QueuedConnection);
                //变形系数变化 → 标注位置随渲染变形重算（标注须与渲染模型同位）
                if (_deformConn)
                    disconnect(_deformConn);
                _deformConn = connect(postRender.get(), &mxr::REBUILD_PostRender::deformationChanged,
                    this, &mPostMeasureTool::updateMeasureAnnotations, Qt::QueuedConnection);
                //【审计 C-01】参数化插值档推进 → 标注随插值变形逐 tick 重算（unify-anim-
                //playback）：Parametric playTick 仅写插值不发行域事件——frame/deform 两钩子
                //均不可达，改前 GUI 直驱链同样无逐 tick 事件（标注播放期不跟随、停机点
                //补发又被帧号去重吞掉而错位不收敛）。事件仅在参数化引擎档播放期发射
                //（浮层/预生成同源），暂停/停止自然停发=标注冻结于所停档位（与冻结插值
                //画面一致，停机收敛达成）。QueuedConnection 同帧通道防绘制内联嵌套；
                //每 tick 重估与帧播放 frameChanged 钩子同代价级
                if (_interpConn)
                    disconnect(_interpConn);
                _interpConn = connect(postRender.get(), &mxr::REBUILD_PostRender::interpolationAdvanced,
                    this, [this](int, int) { updateMeasureAnnotations(); }, Qt::QueuedConnection);
                //云图/动画换帧全局通知 → 换帧标注跟随（postCloudFrameChanged 为 GUI 全局
                //单例信号：云图菜单/3D 结果页/动画面板入口在发；本缺陷修复点——既有
                //frameChanged 仅离线录制路径发射，手动换帧不可达）。QueuedConnection：
                //消费排队到事件循环执行，不在换帧入口/绘制栈内联嵌套（0.42 教训同款双保险；
                //槽内再做播放态门控 + 本视口帧号去重，吸收同帧重应用与跨视口换帧）。
                //本连接不随面板显隐断开（false 分支不断开）：隐藏期间换帧标注仍跟随；
                //此处 pre-disconnect 仅为激活路径幂等重连
                if (_cloudFrameConn)
                    disconnect(_cloudFrameConn);
                _cloudFrameConn = connect(MechCommon::mMechGlobalSignals::getInstance(),
                    &MechCommon::mMechGlobalSignals::postCloudFrameChanged,
                    this, &mPostMeasureTool::slot_onCloudFrameChanged, Qt::QueuedConnection);
                updateMeasureAnnotations();
            }
            else
            {
                if (_pickConn)
                {
                    disconnect(_pickConn);
                    _pickConn = QMetaObject::Connection{};
                }
                if (_frameConn)
                {
                    disconnect(_frameConn);
                    _frameConn = QMetaObject::Connection{};
                }
                if (_cameraConn)
                {
                    disconnect(_cameraConn);
                    _cameraConn = QMetaObject::Connection{};
                }
                if (_deformConn)
                {
                    disconnect(_deformConn);
                    _deformConn = QMetaObject::Connection{};
                }
                if (_interpConn)
                {
                    disconnect(_interpConn);
                    _interpConn = QMetaObject::Connection{};
                }
                //_cloudFrameConn 有意不断开（变更 2026-11-02，用户裁定）：面板隐藏≠测量
                //不存在，标注画在视口标注层持续可见，隐藏期间换帧标注也须跟随。隐藏态
                //安全性由 slot_onCloudFrameChanged 守卫链保证（状态有效性→播放门控→帧号
                //去重），updateMeasureAnnotations 仅写渲染层标注、不触面板控件；激活时
                //true 分支 pre-disconnect + 重连幂等，无双连接
                _postRend->setPickFilter(mxr::PickFilter::PickNothing);
            }
        }
        else {
            // 前处理
            if (_preRend == nullptr) return;	//QPointer：视口销毁后自动置空（ui-shot 空环境实测 AV）
            auto sc = _preRend->getSelectionCaculator();      //getSelectionContainer();//
            if (sc == nullptr) return;

            if (isshow)
            {
                _buttonPickFilters[_btnPickNode] = mxr::PickFilter::PickGeoPoint;

                if (_pickConn)
                    disconnect(_pickConn);

                //if (!_pickConn)
                {
                    _pickConn = connect(sc, &mxr::SelectionCaculator::signals_FinishPick,
                        this, &mPostMeasureTool::slot_finishPick);
                    //_pickConn = connect(sc, &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPostMeasureTool::slot_finishPick);
                }
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

    void mPostMeasureTool::newMeasureToolPanel(int caseId)
    {
        _isCurrStateValid = false;

        if (!_comboBox) return;

        const bool wasPost = _isCurrPostViewport;
        PostData::mPostDataBase* prevDB = _postDB.data();

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
            return;

        //需判断前处理还是后处理
        _isCurrPostViewport = (viewportPrivate->getViewportType() == ViewportPost3D);       // Viewport3D

        if (_isCurrPostViewport) {
            // 后处理
            mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
            if (postRend == nullptr) return;

            auto postRender = postRend->getPostRender();
            if (!postRender) return;

            _postRend = postRend;

            QString postname = postRend->getPostDataName();
            _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        }
        else {
            // 前处理
            _preRend = getCVM->getPreRenderWidget();
            if (!_preRend) return;

        }

        //上下文切换（前/后处理切换或后处理数据库变化）后旧记录对象编号不再可靠，清空重建
        if (!_records.isEmpty() && (wasPost != _isCurrPostViewport || prevDB != _postDB.data()))
            loadMeasureCache();

        //上下文切换后旧拾取节点同样失效，清空并退出拾取态
        _pickedIds.clear();

        _isCurrStateValid = true;

        exitPickMode();
    }

    bool mPostMeasureTool::nodeCoordinate(int nodeId, QVector3D& out)
    {
        out = QVector3D();
        if (!_isCurrStateValid || nodeId < 0)
            return false;

        if (_isCurrPostViewport) {
            if (_postRend == nullptr) return false;

            mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_postRend);
            if (guidata == nullptr || _postDB == nullptr)
            {
                return false;
            }

            //获取当前帧数据
            mPostModelData* modeldata = _postDB->getModelData(guidata->_cloudFrame);
            if (modeldata != nullptr)
            {
                QHash<int, QVector3D> dis = _postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
                int cId = modeldata->_nodedata.nodeidmap.value(nodeId);
                out = modeldata->getVectorById(nodeId) + dis.value(cId);
                return true;
            }
            return false;
        }
        else {
            // 前处理
            MechModelData::mModelData* modelData = getCVM->getModelData();
            if (!modelData || !modelData->getGeoManager()) return false;

            auto points = modelData->getGeoEngineDataMgr()->engine().geoCalculate().CalculateVertexAttr({ nodeId });
            if (points.size() > 0)
            {
                out = points[0].m_coor;
                return true;
            }
            return false;
        }
    }

    //mxr::SelectionInfoType::MeshNode
    int mPostMeasureTool::getPickResultID(mxr::SelectionInfoType selType)
    {
        if (!_isCurrStateValid) return -1;

#define GET_PICK_MESH_NODE_ID(REND) \
        if (REND == nullptr) return -1; \
        auto sc = REND->getSelectionCaculator(); \
        if (sc == nullptr) return -1; \
 \
        const auto& nodes_itr = sc->getMeshSelectComputeData(); \
        if (nodes_itr.empty()) return -1; \
 \
        auto it = nodes_itr.find(selType); \
        if (it == nodes_itr.end() || it->second.empty()) return -1; \
 \
        return it->second[0].data_id.node_id;

        if (_isCurrPostViewport) {
            // 后处理
            GET_PICK_MESH_NODE_ID(_postRend);
        }
        else {
            // 前处理
            if (_preRend == nullptr) return -1;
            auto sc = _preRend->getSelectionCaculator();
            if (sc == nullptr) return -1;

            const auto& nodes_itr = sc->getGeoSelectInfo();
            if (nodes_itr.empty()) return -1;

            auto it = nodes_itr.find(selType);
            if (it == nodes_itr.end() || it->second.empty()) return -1;

            return it->second[0].id;
        }
    }

    bool mPostMeasureTool::getPickResultIDs(mxr::SelectionInfoType selType, std::vector<int>& outAllIds)
    {
        outAllIds.clear();

        if (!_isCurrStateValid) return false;

        mxr::SelectionDataContainer* _selectionContainer = nullptr;
        if (_isCurrPostViewport) {
            // 后处理
            if (_postRend == nullptr) return false;
            // getSelectDataByType
            _selectionContainer = _postRend->getSelectionContainer();
            if (_selectionContainer) {
                mxr::MeshSelectMemoryData* _meshSelData = _selectionContainer->getMeshSelectData();
                if (_meshSelData) {
                    const std::vector<MeshSelectInfoData>& _selectDatas = _meshSelData->getSelectDataByType(selType);
                    for (auto& _selInfo : _selectDatas) {
                        if (selType == mxr::SelectionInfoType::MeshPart) {
                            outAllIds.push_back(_selInfo.part_id);
                        }
                        else if (selType == mxr::SelectionInfoType::MeshElement) {
                            outAllIds.push_back(_selInfo.data_id.element_id);
                        }
                        else {
                            outAllIds.push_back(_selInfo.data_id.node_id);
                        }
                    }
                }
            }
        }
        else {
            // 前处理
            if (_preRend == nullptr) return false;
            // getSelectDataByType
            _selectionContainer = _preRend->getSelectionContainer();
            if (_selectionContainer) {
                mxr::mGeoSelectData* _meshSelData = _selectionContainer->getGeoSelectData();
                if (_meshSelData) {
                    outAllIds = _meshSelData->getSelectDataByType(selType);
                }
            }
        }

        return true;
    }

    int mPostMeasureTool::currentCalcKind() const
    {
        const int i = _comboBox ? _comboBox->currentIndex() : -1;
        if (i < 0 || i >= kMeasureDataCount)
            return -1;
        return kMeasureDataDefs[i].calc;
    }

    int mPostMeasureTool::requiredNodeCount() const
    {
        const int calc = currentCalcKind();
        return calc < 0 ? 1 : nodesForCalc(calc);
    }

    QString mPostMeasureTool::typeName(int typeIndex, int lang) const
    {
        if (typeIndex < 0 || typeIndex >= kMeasureDataCount)
            return QString();
        return lang == 0 ? QString::fromUtf8(kMeasureDataDefs[typeIndex].cn)
            : QString::fromLatin1(kMeasureDataDefs[typeIndex].en);
    }

    double mPostMeasureTool::measurePrimaryValue(const MeasureRecord& rec) const
    {
        //结构修：记录存创建帧原始结果——向量类分支（0两点距离/2位置/3相对位移）
        //raw={X,Y,Z}，按标签幅值分量组合取值；1增量距离已对齐 HyperView 标量口径
        //（间距差 d(t)−d(t0)，无幅值组合）；标量类 raw={值}。
        //幅值"无"返回 0（文本层据此省略数值）
        const bool vectorClass = (rec.page == 0 || rec.page == 2 || rec.page == 3);
        if (vectorClass)
        {
            const QVector3D v(float(rec.rawValues.value(0)), float(rec.rawValues.value(1)),
                float(rec.rawValues.value(2)));
            return magnitudeOverComponents(v, rec.labels.magnitude);
        }
        return rec.rawValues.isEmpty() ? 0.0 : rec.rawValues.at(0);
    }

    QString mPostMeasureTool::measurePrimaryText(const MeasureRecord& rec, int lang) const
    {
        const LabelSettings& lb = rec.labels;
        auto fmt = [&lb](double v) {
            return lb.scientific ? QString::number(v, 'e', lb.precision)
                : QString::number(v, 'f', lb.precision);
        };
        //角度类：值存度，显示随角度单位设置换算并带单位（增量/相对角度同样生效）
        if (rec.page == 4 || rec.page == 5 || rec.page == 6)
        {
            const double a = lb.radians ? qDegreesToRadians(rec.rawValues.value(0))
                : rec.rawValues.value(0);
            return fmt(a) + (lb.radians
                ? (lang == 0 ? QString::fromUtf8(" 弧度") : QString(" rad"))
                : (lang == 0 ? QString::fromUtf8(" 度") : QString(" deg")));
        }
        //向量类（0/2/3）：幅值"无"不显示数值；显示前缀（0.40 裁定）=幅值标注
        //开关：开 → "Mag = v"，关 → 裸值。1增量距离为标量口径，无幅值组合
        if (rec.page == 0 || rec.page == 2 || rec.page == 3)
        {
            if (lb.magnitude == 0)
                return QString();
            const QString v = fmt(measurePrimaryValue(rec));
            if (!lb.prefix)
                return v;
            return QString("%1 = %2").arg(magnitudeTag(lb.magnitude), v);
        }
        return fmt(measurePrimaryValue(rec));
    }

    QString mPostMeasureTool::formatMeasureText(const MeasureRecord& rec, int lang) const
    {
        //结果全文按记录标签设置现场格式化（列表 tooltip/详情/状态行同源）
        const LabelSettings& lb = rec.labels;
        auto fmt = [&lb](double v) {
            return lb.scientific ? QString::number(v, 'e', lb.precision)
                : QString::number(v, 'f', lb.precision);
        };
        const QString primary = measurePrimaryText(rec, lang);
        const QVector3D v(float(rec.rawValues.value(0)), float(rec.rawValues.value(1)),
            float(rec.rawValues.value(2)));
        switch (rec.page)
        {
        case 0:
            return lang == 0
                ? QString::fromUtf8("距离:%1\nX轴距离:%2\nY轴距离:%3\nZ轴距离:%4")
                    .arg(primary, fmt(std::fabs(v.x())), fmt(std::fabs(v.y())), fmt(std::fabs(v.z())))
                : QString("Distance:%1\nDX:%2\nDY:%3\nDZ:%4")
                    .arg(primary, fmt(std::fabs(v.x())), fmt(std::fabs(v.y())), fmt(std::fabs(v.z())));
        case 1:
            return lang == 0 ? QString::fromUtf8("增量距离:%1（相对首帧）").arg(primary)
                : QString("Delta Distance:%1 (vs first frame)").arg(primary);
        case 2:
            return lang == 0 ? QString::fromUtf8("位置:\nX:%1\nY:%2\nZ:%3")
                    .arg(fmt(v.x()), fmt(v.y()), fmt(v.z()))
                : QString("Position:\nX:%1\nY:%2\nZ:%3")
                    .arg(fmt(v.x()), fmt(v.y()), fmt(v.z()));
        case 3:
            return lang == 0 ? QString::fromUtf8("相对位移:%1").arg(primary)
                : QString("Rel. Displacement:%1").arg(primary);
        case 4:
            return lang == 0 ? QString::fromUtf8("相对角度:%1").arg(primary)
                : QString("Rel. Angle:%1").arg(primary);
        case 5:
            return lang == 0 ? QString::fromUtf8("角度:%1").arg(primary)
                : QString("Angle:%1").arg(primary);
        case 6:
            return lang == 0 ? QString::fromUtf8("增量角度:%1").arg(primary)
                : QString("Delta Angle:%1").arg(primary);
        case 7:
            return lang == 0 ? QString::fromUtf8("半径:%1").arg(primary)
                : QString("Radius:%1").arg(primary);
        case 8:
        {
            //P0 对齐 HyperView Contour measure：显示当前激活云图变量的节点值
            //（创建帧快照+变量名；变量名缺失时退化为纯值）
            if (rec.varName.isEmpty())
                return lang == 0 ? QString::fromUtf8("节点云图:%1").arg(primary)
                    : QString("Contour:%1").arg(primary);
            return lang == 0 ? QString::fromUtf8("节点云图:%1（%2）").arg(primary, rec.varName)
                : QString("Contour:%1 (%2)").arg(primary, rec.varName);
        }
        case 9:
            return lang == 0 ? QString::fromUtf8("路径长度:%1").arg(primary)
                : QString("Path Length:%1").arg(primary);
        case 10:
            return lang == 0 ? QString::fromUtf8("最小距离:%1").arg(primary)
                : QString("Minimum Distance:%1").arg(primary);
        case 11:
            return lang == 0 ? QString::fromUtf8("体积:%1").arg(primary)
                : QString("Volume:%1").arg(primary);
        case 12:
            return lang == 0 ? QString::fromUtf8("面积:%1").arg(primary)
                : QString("Area:%1").arg(primary);
        }
        return QString();
    }

    QString mPostMeasureTool::formatMeasureLabelLines(const MeasureRecord& rec, int lang) const
    {
        //3D 标签文本（0.40 语义）：
        //  单节点测量（位置/节点云图）：首行=拾取节点号（恒显，用户要求）可并
        //  名称（显示名称开关）；位置其后按幅值分量逐行取值（显示前缀开带
        //  "X = "标注，关为裸值），节点云图为值
        //  多节点测量：标题=#记录序号（显示编号）+测量项命名（显示名称）+主值；
        //  各拾取节点旁的节点号小标签恒显（updateMeasureAnnotations 绘制）
        const LabelSettings& lb = rec.labels;
        auto fmt = [&lb](double v) {
            return lb.scientific ? QString::number(v, 'e', lb.precision)
                : QString::number(v, 'f', lb.precision);
        };

        if (rec.page == 2 || rec.page == 8)
        {
            QStringList headParts;
            if (lb.showId)
                headParts << QString("#%1").arg(rec.id);	//显示编号：与多节点测量同口径
            if (lb.showName)
                headParts << rec.name;
            if (!rec.ids.isEmpty())
                headParts << QString::number(rec.ids.first());	//首行节点号（恒显）
            QString first = headParts.join(' ');
            QStringList lines;
            if (!first.isEmpty())
                lines << first;

            if (rec.page == 8)
            {
                const QString v = measurePrimaryText(rec, lang);
                if (!v.isEmpty())
                    lines << v;
            }
            else if (lb.magnitude != 0)
            {
                const QVector3D v(float(rec.rawValues.value(0)), float(rec.rawValues.value(1)),
                    float(rec.rawValues.value(2)));
                if (lb.magnitude == 1 || lb.magnitude == 2 || lb.magnitude == 3 || lb.magnitude == 5)
                    lines << (lb.prefix ? QString("X = %1").arg(fmt(v.x())) : fmt(v.x()));
                if (lb.magnitude == 1 || lb.magnitude == 2 || lb.magnitude == 4 || lb.magnitude == 6)
                    lines << (lb.prefix ? QString("Y = %1").arg(fmt(v.y())) : fmt(v.y()));
                if (lb.magnitude == 1 || lb.magnitude == 3 || lb.magnitude == 4 || lb.magnitude == 7)
                    lines << (lb.prefix ? QString("Z = %1").arg(fmt(v.z())) : fmt(v.z()));
            }
            if (lines.isEmpty())
                lines << (lb.showId ? QString("#%1").arg(rec.id) : QString());
            return lines.join('\n');
        }

        QString head;
        if (lb.showId)
            head += QString("#%1 ").arg(rec.id);
        if (lb.showName)
            head += rec.name;
        head = head.trimmed();
        if (lb.magnitude == 0 && rec.page >= 0 && rec.page <= 3)
            return head;	//向量类幅值"无"：仅标题
        return (head.isEmpty() ? QString() : head + QString::fromUtf8("："))
            + measurePrimaryText(rec, lang);
    }

    bool mPostMeasureTool::inputsComplete() const
    {
        if (!_isCurrStateValid)
            return false;
        const int calc = currentCalcKind();
        if (calc < 0)
            return false;
        return _pickedIds.size() >= requiredNodeCount();
    }

    bool mPostMeasureTool::buildRecordFromInput(MeasureRecord& rec)
    {
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        const int typeIdx = _comboBox ? _comboBox->currentIndex() : -1;
        if (typeIdx < 0 || typeIdx >= kMeasureDataCount)
            return false;
        const MeasureDataDef* def = &kMeasureDataDefs[typeIdx];
        const int calc = def->calc;
        const QString typeText = typeName(typeIdx, lang);

        //delta/云图类需要结果帧数据（仅后处理）
        const bool needResult = (calc == 1 || calc == 3 || calc == 4 || calc == 6 || calc == 8);
        if (needResult && !_isCurrPostViewport)
        {
            setStatusLabel(lang == 0 ? QString::fromUtf8("「%1」需结果数据，仅后处理可用。").arg(typeText)
                : QString("\"%1\" needs result data; post-processing only.").arg(typeText));
            return false;
        }

        if (_pickedIds.size() < def->nodes)
        {
            const bool elemDomain = (calc == 11 || calc == 12);
            setStatusLabel(lang == 0
                ? (elemDomain ? QString::fromUtf8("请先选择单元。") : QString::fromUtf8("请先选择节点。"))
                : (elemDomain ? QString("Please pick elements first.") : QString("Please pick nodes first.")));
            return false;
        }

        //当前帧位置
        QVector<QVector3D> c;
        for (int id : _pickedIds)
        {
            QVector3D p;
            if (!nodeCoordinate(id, p))
            {
                MechMainWindow::mFramelessMessageBox::warning(this, "Warning", lang == 0 ? "测量失败！节点坐标不可用。" : "Measurement failure! Node coordinate unavailable.");
                return false;
            }
            c.append(p);
        }

        //delta 类：首帧位置基准（后处理第一帧；取不到时 calcValue 内退化为当前值）
        QVector<QVector3D> c0;
        if (needResult && _postDB != nullptr)
        {
            if (mPostGUIData* gd = getCVM->getGUIDataByPostRender(_postRend))
            {
                std::vector<QPair<QString, int>> frames;
                _postDB->getCaseFrameNamesAndIDs(gd->_cloudCase, frames);
                for (auto& f : frames)
                {
                    mPostModelData* md = _postDB->getModelData(f.second);
                    if (md == nullptr) continue;
                    const QHash<int, QVector3D> dis = _postDB->getDisplacementDataByFrameID(f.second);
                    for (int id : _pickedIds)
                    {
                        const int lx = md->_nodedata.nodeidmap.value(id);
                        c0.append(md->getVectorById(id) + dis.value(lx));
                    }
                    break;	//首帧
                }
            }
        }

        rec.typeIndex = typeIdx;
        rec.page = calc;
        rec.ids = _pickedIds;
        rec.source = _postDB ? _postDB->getName() : QString();	//来源结果（历史归属/绘制过滤）

        //体积/面积：引擎按拾取对象（网格单元/几何体、面）计算，不走节点位置
        if (calc == 11 || calc == 12)
        {
            double value = 0.0;
            std::vector<int> objIds(_pickedIds.cbegin(), _pickedIds.cend());
            if (_isCurrPostViewport)
            {
                if (_postDB == nullptr)
                {
                    MechMainWindow::mFramelessMessageBox::warning(this, "Warning", lang == 0 ? "测量失败！结果数据不可用。" : "Measurement failure! Result data unavailable.");
                    return false;
                }
                mPostGUIData* gd = getCVM->getGUIDataByPostRender(_postRend);
                const int frameId = gd ? gd->_cloudFrame : 0;
                value = (calc == 11) ? _postDB->CaluElementsVolumeByFrame(objIds, frameId)
                                     : _postDB->CaluElementsAreaByFrame(objIds, frameId);
            }
            else if (calc == 11)
            {
                MechModelData::mModelData* modelData = getCVM->getModelData();
                if (modelData == nullptr || modelData->getGeoManager() == nullptr)
                {
                    MechMainWindow::mFramelessMessageBox::warning(this, "Warning", lang == 0 ? "测量失败！几何数据不可用。" : "Measurement failure! Geometry unavailable.");
                    return false;
                }
                {//【OCC摘除·集成】原 getGeoManager()->GetSolidVolume → 引擎 CalculateSolidAttr 求和
                value = 0.0;
                auto attrs = modelData->getGeoEngineDataMgr()->engine().geoCalculate().CalculateSolidAttr(objIds);
                for (const auto& attr : attrs)
                    if (attr.m_volume > 0) value += attr.m_volume;
            }
            }
            else
            {
                MechModelData::mModelData* modelData = getCVM->getModelData();
                if (modelData == nullptr || modelData->getGeoManager() == nullptr)
                {
                    MechMainWindow::mFramelessMessageBox::warning(this, "Warning", lang == 0 ? "测量失败！几何数据不可用。" : "Measurement failure! Geometry unavailable.");
                    return false;
                }
                {//【OCC摘除·集成】原 getGeoManager()->GetFaceArea → 引擎 CalculateFaceAttr 求和
                value = 0.0;
                auto attrs = modelData->getGeoEngineDataMgr()->engine().geoCalculate().CalculateFaceAttr(objIds);
                for (const auto& attr : attrs)
                    if (attr.m_area > 0) value += attr.m_area;
            }
            }
            rec.rawValues = { double(value) };	//结构修：存原始值，显示时按标签设置格式化
            return true;
        }

        //结构修：存创建帧原始结果（向量类={X,Y,Z}分量、标量类={值}、角度类存度），
        //显示文本由 formatMeasureText 按记录标签设置现场生成
        const QVector<QVector3D>& c0u = c0.size() >= c.size() ? c0 : c;
        QVector<double> raw;
        switch (calc)
        {
        case 0: { const QVector3D v = c[1] - c[0]; raw = { v.x(), v.y(), v.z() }; break; }
        case 1: raw = { (c[1] - c[0]).length() - (c0u[1] - c0u[0]).length() }; break;	//P1：间距标量差
        case 2: raw = { c[0].x(), c[0].y(), c[0].z() }; break;
        case 3: { const QVector3D v = (c[0] - c0u[0]) - (c[1] - c0u[1]); raw = { v.x(), v.y(), v.z() }; break; }
        case 4: raw = { angleBetweenVectors(c[0] - c0u[0], c[1] - c0u[1]) }; break;	//度
        case 5: raw = { mCommonFunctions::getThreePointAngle(c[0], c[2], c[1]) }; break;	//度
        case 6: raw = { angleBetweenVectors(c[0] - c[1], c[2] - c[1])
                - angleBetweenVectors(c0u[0] - c0u[1], c0u[2] - c0u[1]) }; break;	//度
        case 7: raw = { circumRadius3D(c[0], c[1], c[2]) }; break;
        case 8:
        {
            //P0 对齐 HyperView Contour measure：当前激活云图变量在拾取节点处的值
            //（连续 ID 键查值，节点型结果才可用；变量名随记录快照，显示带出）
            mPostGUIData* gd = (_postDB != nullptr) ? getCVM->getGUIDataByPostRender(_postRend) : nullptr;
            const int frameId = gd ? gd->_cloudFrame : 0;
            mPostModelData* md = _postDB ? _postDB->getModelData(frameId) : nullptr;
            int resultFlag = -1;
            const QHash<int, double> frameVals = _postDB
                ? _postDB->getResultDataByFrameID(frameId, resultFlag) : QHash<int, double>();
            const int cId = md ? md->_nodedata.nodeidmap.value(_pickedIds.at(0)) : -1;
            if (resultFlag != 0 || cId < 0 || !frameVals.contains(cId))
            {
                setStatusLabel(lang == 0
                    ? QString::fromUtf8("当前云图为单元结果或无节点值，节点云图测量不可用。")
                    : QString("Nodal contour unavailable: current result is element data or has no nodal values."));
                return false;
            }
            rec.varName = _postDB->getResultVarNameByFrameID(frameId);
            raw = { frameVals.value(cId) };
            break;
        }
        case 9: raw = { polyLineLength(c) }; break;
        case 10:
        {
            //P2 对齐 HyperView：基准节点（首拾取）到对比集合的最小距离
            double best = -1.0;
            for (int i = 1; i < c.size(); ++i)
            {
                const double d = (c[i] - c[0]).length();
                if (best < 0.0 || d < best)
                    best = d;
            }
            raw = { best < 0.0 ? 0.0 : best };
            break;
        }
        default:
            return false;
        }
        rec.rawValues = raw;
        return true;
    }

    QString mPostMeasureTool::recordObjectText(const MeasureRecord& rec, int lang) const
    {
        QStringList ids;
        for (int id : rec.ids)
            ids.append(QString::number(id));
        //按计算分支描述拾取对象（13 类型全为节点/单元拾取，无空分支）
        switch (rec.page)
        {
        case 5:	//夹角：拾取序为 点1/角点/点2
            return lang == 0 ? QString("节点 %1（角点%2）").arg(ids.join(",")).arg(ids.size() > 1 ? ids.at(1) : QString())
                : QString("Nodes %1 (corner %2)").arg(ids.join(",")).arg(ids.size() > 1 ? ids.at(1) : QString());
        case 10:	//最小距离（P2 点对集）：基准节点与对比集合分述
        {
            if (rec.ids.isEmpty())
                break;
            QStringList rest;
            for (int i = 1; i < rec.ids.size(); ++i)
                rest << QString::number(rec.ids.at(i));
            return lang == 0
                ? QString::fromUtf8("基准节点 %1；对比节点 %2").arg(QString::number(rec.ids.first()), rest.join(","))
                : QString("Base node %1; comparison nodes %2").arg(QString::number(rec.ids.first()), rest.join(","));
        }
        case 11:
            return lang == 0 ? QString("单元 %1").arg(ids.join(",")) : QString("Elements %1").arg(ids.join(","));
        case 12:
            return lang == 0 ? QString("面/单元 %1").arg(ids.join(",")) : QString("Faces/Elements %1").arg(ids.join(","));
        default:
            return lang == 0 ? QString("节点 %1").arg(ids.join(",")) : QString("Nodes %1").arg(ids.join(","));
        }
    }

    QString mPostMeasureTool::recordLabelSummary(const MeasureRecord& rec, int lang) const
    {
        //对齐原型复核详情行（measurement-detail-values）的标签字段摘要
        const LabelSettings& lb = rec.labels;
        static const char* kMagnitudeCn[] = { "无", "X+Y+Z", "X+Y", "X+Z", "Y+Z", "X", "Y", "Z" };
        static const char* kMagnitudeEn[] = { "None", "X+Y+Z", "X+Y", "X+Z", "Y+Z", "X", "Y", "Z" };

        QStringList parts;
        parts << (lang == 0 ? QString("字号 %1").arg(lb.fontSize) : QString("Font %1").arg(lb.fontSize));
        const QString magText = (lb.magnitude >= 0 && lb.magnitude < 8)
            ? QString::fromUtf8(lang == 0 ? kMagnitudeCn[lb.magnitude] : kMagnitudeEn[lb.magnitude])
            : QString();
        parts << QString(lang == 0 ? "幅值 %1" : "Magnitude %1").arg(magText);
        parts << (lang == 0 ? QString(lb.scientific ? "科学 %1 位" : "小数 %1 位").arg(lb.precision)
            : QString("%1 %2 digits").arg(lb.scientific ? "Scientific" : "Decimal").arg(lb.precision));
        parts << (lang == 0 ? (lb.radians ? "弧度" : "度数") : (lb.radians ? "Radians" : "Degrees"));
        parts << (lang == 0 ? QString("标签颜色 %1").arg(lb.color.name()) : QString("Color %1").arg(lb.color.name()));
        if (lb.fill)
            parts << (lang == 0 ? QString("填充 %1").arg(lb.fillColor.name()) : QString("Fill %1").arg(lb.fillColor.name()));
        QStringList display;
        if (lb.showId) display << (lang == 0 ? "编号" : "ID");
        if (lb.showName) display << (lang == 0 ? "名称" : "Name");
        parts << (lang == 0 ? QString("显示 %1").arg(display.isEmpty() ? "无" : display.join("/"))
            : QString("Display %1").arg(display.isEmpty() ? "None" : display.join("/")));
        if (lb.autoHide)
            parts << (lang == 0 ? "自动隐藏" : "Auto Hide");
        if (lb.prefix)
            parts << (lang == 0 ? "显示前缀" : "Prefix");
        return parts.join("，");
    }

    void mPostMeasureTool::exitPickMode()
    {
        //必须在取消勾选前捕获拾取态：循环之后 isChecked 恒为 false，
        //下面的状态行清空判断会永远走不到（实测 bug）
        const bool wasPicking = _btnPickNode && _btnPickNode->isChecked();
        for (auto iter = _buttonPickFilters.cbegin(); iter != _buttonPickFilters.cend(); ++iter)
        {
            iter.key()->setChecked(false);
        }
        mxr::mBaseRend* _pickRend = nullptr;
        if (_isCurrPostViewport)
            _pickRend = _postRend;
        else
            _pickRend = _preRend;
        if (_pickRend)
            _pickRend->setPickFilter(mxr::PickFilter::PickNothing);
        if (_btnPickNode)
        {
            int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
            _btnPickNode->setText(pickButtonLabel(false, lang));
        }
        //仅在按钮确实处于拾取态（被取消勾选）时清状态行：创建成功等路径先写结果
        //再调本函数，不能把刚写入的结果擦掉（对齐原型 setMeasurementPicking(false) 语义）
        if (wasPicking)
            setStatusLabel(QString());
        else if (_statusLabel)
            setStatusLabel(_statusLabel->text());
    }

    void mPostMeasureTool::hideEvent(QHideEvent* event)
    {
        //面板切走（共享 dock 切页/总复位）时退出拾取并断开拾取信号，避免隐藏期间
        //视口拾取仍回填本面板、自动创建静默建记录（对齐原型：面板切换即取消拾取）
        if (_isCurrStateValid)
        {
            exitPickMode();
            setPanelPickAndSignal(false);
        }
        mPanelBase::hideEvent(event);
    }

    bool mPostMeasureTool::eventFilter(QObject* watched, QEvent* event)
    {
        //列表 tooltip 拉模式（02 FR-04 消费面矩阵）：名称列悬停时按当前帧现场格式化
        //（数值随帧），不随换帧推送、不全表刷新；其余列静态提示不涉数值照旧默认。
        //复核弹窗 tooltip/详情行由弹窗打开重建与 refreshExpandedReviewDetails 覆盖
        if (watched == _table->viewport() && event->type() == QEvent::ToolTip)
        {
            const auto* help = static_cast<QHelpEvent*>(event);
            QTableWidgetItem* it = _table->itemAt(help->pos());
            if (it != nullptr && it->column() == 1)
            {
                const int di = dataIndexById(it->data(Qt::UserRole).toInt());
                if (di >= 0)
                {
                    int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
                    MeasureRecord cur = _records.at(di);
                    mPostGUIData* gd = (_postRend != nullptr && _postDB != nullptr)
                        ? getCVM->getGUIDataByPostRender(_postRend) : nullptr;
                    if (gd != nullptr)
                    {
                        const QVector<double> rv = computeRawValuesForFrame(cur,
                            gd->_cloudFrame, nullptr, nullptr, nullptr, nullptr);
                        if (!rv.isEmpty())
                            cur.rawValues = rv;	//取值失败回退创建帧快照
                    }
                    QToolTip::showText(help->globalPos(), formatMeasureText(cur, lang),
                        _table->viewport());
                    return true;
                }
            }
        }
        //标签弹窗折叠组内容件：收起动画收尾置 Hide、展开布局定型置 Resize——此刻
        //内容尺寸已最终，延迟一拍让折叠组布局链激活后按最新 sizeHint 同步弹窗
        //尺寸（顶层窗口不随子控件 hint 变化自动缩放；adjustSize 受定宽 260 约束，
        //只纠正高度）。初始布局/语言刷新等引发的 Resize 同样触发，但尺寸已匹配
        //时 adjustSize 为无操作
        const QString watchedName = watched->objectName();
        if (watchedName == QLatin1String("mLabelShowSecBody") || watchedName == QLatin1String("mLabelGlobalSecBody"))
        {
            const QEvent::Type t = event->type();
            if (t == QEvent::Hide || t == QEvent::Resize)
                QTimer::singleShot(0, this, [this]() {
                    if (!_labelPopup)
                        return;
                    //顶层弹窗不随子树 sizeHint 变化自动缩放，且有两层互锁的旧值：
                    //①布局 item 的 sizeHint 缓存不随子树塌缩刷新（QLayout::invalidate
                    //  不清 item 缓存，只有该布局自身几何重算才刷新）——直接 adjustSize
                    //  永远读到收起前的旧 hint；②该旧 hint 已被激活期写入弹窗
                    //  minimumSize，resize 被钳在旧高度。重建 body 的布局 item（生成
                    //  新 item、按最新子树 hint 重算）并解除旧最小高后，再按最新
                    //  sizeHint 纠正弹窗尺寸；布局收敛（子几何不再变化）后不再触发
                    QWidget* body = _labelPopup->findChild<QWidget*>("mLabelBody");
                    auto* pl = qobject_cast<QVBoxLayout*>(_labelPopup->layout());
                    if (body && pl)
                    {
                        pl->removeWidget(body);
                        pl->insertWidget(1, body);	//0=标题栏，body 原位，2=footer
                    }
                    _labelPopup->setMinimumHeight(0);
                    _labelPopup->adjustSize();
                });
        }
        //标签弹窗内色块按压即打开取色器并消费按压：取色器(第二 Popup)打开时
        //标签弹窗不会被连带关闭，取色结果经 colorChanged 实时写回草稿
        //（同 mPostSectionForce 显示设置色块的成熟处理）
        if (event->type() == QEvent::MouseButtonPress)
        {
            if (watched == _lpColorSwatch)
            {
                slot_labelColorClicked();
                return true;
            }
            if (watched == _lpFillSwatch)
            {
                slot_fillColorClicked();
                return true;
            }
        }
        return mPanelBase::eventFilter(watched, event);
    }

    void mPostMeasureTool::maybeAutoCreate()
    {
        if (!_checkAutoCreate || !_checkAutoCreate->isChecked())
            return;
        if (!inputsComplete())
            return;
        slot_createButton();
    }

    int mPostMeasureTool::dataIndexById(int id) const
    {
        for (int i = 0; i < _records.size(); ++i)
            if (_records.at(i).id == id) return i;
        return -1;
    }

    //测量历史管理器（全局会话级；面板/视口未就绪时返回空）
    PostData::mPostDataManager* mPostMeasureTool::measureManager() const
    {
        return (getCVM && getCVM->getModelData())
            ? &getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post)
            : nullptr;
    }

    //测量历史缓存写回（面板镜像 → 全局管理器）：所有记录变更后调用
    void mPostMeasureTool::syncMeasureCache()
    {
        if (auto* mgr = measureManager())
            mgr->setMeasureHistory(_records);
        //广播记录变更：后处理树"测量"分支据此同步子节点（无后处理树/上下文失效时槽内自守卫）
        MechCommon::mMechGlobalSignals::getInstance()->postMeasureRecordsChangedSig();
    }

    //从全局管理器加载测量历史并重建列表行（面板重建/上下文切换时）
    void mPostMeasureTool::loadMeasureCache()
    {
        if (auto* mgr = measureManager())
        {
            _records = mgr->measureHistory();
            _nextId = 1;
            for (const auto& r : _records)
                _nextId = qMax(_nextId, r.id + 1);
        }
        //按历史记录重建各类型命名计数器（否则重载后新记录命名从 1 起，与历史重名）
        _typeCounters.clear();
        for (const auto& r : _records)
        {
            const int seq = r.name.section('_', -1).toInt();
            if (seq > 0)
                _typeCounters[r.typeIndex] = qMax(_typeCounters.value(r.typeIndex, 0), seq);
        }
        {
            MxFunctions::mPanelTable::RowUpdateGuard guard(_table);
            _table->setRowCount(0);
            for (int i = 0; i < _records.size(); ++i)
            {
                _table->insertRow(i);
                _table->setItem(i, 0, MxFunctions::mPanelTable::makeCheckItem());
                _table->setItem(i, 1, MxFunctions::mPanelTable::makeNameItem(QString(), _records.at(i).id));
                for (int col = 2; col <= 4; ++col)
                    _table->setItem(i, col, MxFunctions::mPanelTable::makeTextItem(QString()));
                renderRow(i);
            }
        }
        _table->horizontalHeader()->setSortIndicatorShown(false);
        applyManagerFilter();
        refreshEmptyState();
        syncManagerState();
        updateMeasureAnnotations();	//缓存重载 → 3D 标注重建
    }

    void mPostMeasureTool::clearRecords()
    {
        _records.clear();
        _typeCounters.clear();
        _nextId = 1;
        _table->setRowCount(0);
        clearMeasureAnnotations();	//3D 标注同步清空
        refreshEmptyState();
        syncManagerState();
    }

    void mPostMeasureTool::renderRow(int row)
    {
        QTableWidgetItem* nameIt = _table->item(row, 1);
        if (!nameIt) return;
        int di = dataIndexById(nameIt->data(Qt::UserRole).toInt());
        if (di < 0) return;
        const MeasureRecord& rec = _records.at(di);

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

        //V1.3：待定义记录（缺拾取引用）名称加标记
        if (_incompleteIds.contains(rec.id))
        {
            nameIt->setText(rec.name + QString::fromUtf8("（待定义）"));
            nameIt->setToolTip(rec.name + QString::fromUtf8("：请补充有效测量对象后保存"));
        }
        else
        {
            nameIt->setText(rec.name);
            nameIt->setToolTip(formatMeasureText(rec, lang));
        }
        //类型列辅助灰（对齐原型 td:nth-child(3) { color:var(--ui-muted) }）
        QTableWidgetItem* typeIt = _table->item(row, 2);
        typeIt->setText(typeName(rec.typeIndex, lang));
        typeIt->setForeground(QBrush(QColor(kMuted)));
        typeIt->setToolTip(typeName(rec.typeIndex, lang));

        QTableWidgetItem* visIt = _table->item(row, 3);
        //文本仅作排序键，显示由共享件 mPanelEyeDelegate 覆盖绘制（睁眼主蓝/斜杠眼灰）
        visIt->setText(lang == 0 ? (rec.visible ? QString::fromUtf8("显") : QString::fromUtf8("隐"))
            : (rec.visible ? QString("On") : QString("Off")));
        visIt->setData(Qt::UserRole + 1, rec.visible);
        visIt->setTextAlignment(Qt::AlignCenter);
        visIt->setToolTip(lang == 0 ? QString::fromUtf8("点击切换显隐") : QString("Click to toggle visibility"));

        //颜色格写入颜色值文本供表头排序（委托只画色块不画文本，对齐原型"颜色"列可排序）
        QTableWidgetItem* colorIt = _table->item(row, 4);
        colorIt->setText(rec.color.name());
        colorIt->setBackground(QBrush(rec.color));
        colorIt->setToolTip(lang == 0 ? QString::fromUtf8("设置颜色") : QString("Set color"));

        applyCheckedRowBackground(row);
    }

    void mPostMeasureTool::applyCheckedRowBackground(int row)
    {
        //对齐原型 .measurement-table tr:has(input:checked)：勾选行整行套选中底色
        //（共享件口径：selected/surface 随 UiPalette；颜色格 BackgroundRole 是色源数据，
        //排除不参与高亮）
        if (!_table->item(row, 0))
            return;
        const bool checked = _table->item(row, 0)->checkState() == Qt::Checked;
        _table->setRowChecked(row, checked, 4);
    }

    void mPostMeasureTool::refreshEmptyState()
    {
        //对齐原型 renderMeasurementList：按"可见记录数"判断空表（筛选全隐藏时也要出提示），
        //并区分"暂无测量数据"与"没有匹配的测量数据"——空态覆盖层由共享表格件内置
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        _table->setEmptyTexts(lang == 0 ? QString::fromUtf8("暂无测量数据") : QString("No measurements"),
            lang == 0 ? QString::fromUtf8("没有匹配的测量数据") : QString("No matching measurements"));
    }

    void mPostMeasureTool::syncManagerState()
    {
        //状态行按可见行/勾选数刷新；显示/隐藏/删除随之启停。
        //勾选统计含被筛选隐藏的已选行（需求 V1.1 §2：筛选隐藏的已选记录保留选择，
        //批量操作作用于全部已选，另提示隐藏的选中数量）
        int checked = 0;
        int hiddenChecked = 0;
        int visibleCount = 0;
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            const bool hidden = _table->isRowHidden(r);
            if (!hidden) ++visibleCount;
            if (_table->item(r, 0) && _table->item(r, 0)->checkState() == Qt::Checked)
            {
                if (hidden) ++hiddenChecked;
                else ++checked;
            }
        }
        const int checkedAll = checked + hiddenChecked;

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        //对齐原型/设计截图：空态不显示管理状态行（measurementManagerStatus 初始为空）
        QString mgrText;
        if (visibleCount > 0 || checkedAll > 0)
        {
            mgrText = lang == 0
                ? QString::fromUtf8("共 %1 条，已选 %2 条").arg(visibleCount).arg(checkedAll)
                : QString("%1 records, %2 selected").arg(visibleCount).arg(checkedAll);
            if (hiddenChecked > 0)
                mgrText += lang == 0
                    ? QString::fromUtf8("，另有 %1 条已选被隐藏").arg(hiddenChecked)
                    : QString(", %1 selected hidden").arg(hiddenChecked);
        }
        _managerStatusLabel->setText(mgrText);
        //同 _statusLabel：行级摘/插同步折叠高度帽，防 setVisible 压缩上方行距
        _manageFold->setRowVisible(_managerStatusLabel, !mgrText.isEmpty());
        _btnDelete->setEnabled(checkedAll > 0);
        _btnShow->setEnabled(checkedAll > 0);
        _btnHide->setEnabled(checkedAll > 0);
        _btnSelectAll->setEnabled(visibleCount > 0);

        //对齐原型：可见行全部勾选时全选按钮切换为"取消"
        const bool allSelected = visibleCount > 0 && checked == visibleCount;
        _btnSelectAll->setText(lang == 0 ? (allSelected ? QString::fromUtf8("取消") : QString::fromUtf8("全选"))
            : (allSelected ? QString("None") : QString("All")));
    }

    void mPostMeasureTool::applyManagerFilter()
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

    void mPostMeasureTool::setCheckedRecordsVisible(bool visible)
    {
        bool changed = false;
        _table->setSortingEnabled(false);
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            if (_table->item(r, 0)->checkState() != Qt::Checked) continue;
            QTableWidgetItem* nameIt = _table->item(r, 1);
            const int di = nameIt ? dataIndexById(nameIt->data(Qt::UserRole).toInt()) : -1;
            if (di < 0) continue;
            _records[di].visible = visible;
            renderRow(r);
            changed = true;
        }
        _table->setSortingEnabled(true);
        _table->horizontalHeader()->setSortIndicatorShown(false);
        if (changed)
        {
            syncMeasureCache();	//批量显隐 → 镜像/树同步（原漏写：manager 显隐位陈旧）
            syncManagerState();
            updateMeasureAnnotations();	//批量显隐 → 3D 标注同步
        }
    }

    void mPostMeasureTool::slot_debugInjectDistanceRecord()
    {
        //[调试] ui-shot 数据态注入：免拾取交互生成一条两点距离记录（取当前结果
        //前两个节点），行勾选置位以便显隐按钮联动。供自动化验证 3D 标注与
        //详情弹窗数据态（0.10 轮遗留升级路径落地）
        if (!_isCurrStateValid || !_isCurrPostViewport)
            return;
        if (_postDB == nullptr || _postRend == nullptr)
            return;
        mPostGUIData* gd = getCVM->getGUIDataByPostRender(_postRend);
        mPostModelData* md = (gd != nullptr) ? _postDB->getModelData(gd->_cloudFrame) : nullptr;
        if (md == nullptr)
            return;

        QList<int> nodeIds;
        {
            //取 X 方向最小/最大的两个节点：连线横贯模型，标注与显隐效果肉眼可辨
            //（取哈希序前两个会得到相邻节点，线段短到无法验证）
            int minXId = -1, maxXId = -1;
            float minX = 0.0f, maxX = 0.0f;
            bool first = true;
            for (auto it = md->_nodedata.nodeidmap.constBegin();
                 it != md->_nodedata.nodeidmap.constEnd(); ++it)
            {
                const QVector3D p = md->getVectorById(it.key());
                if (first || p.x() < minX) { minX = p.x(); minXId = it.key(); }
                if (first || p.x() > maxX) { maxX = p.x(); maxXId = it.key(); }
                first = false;
            }
            if (minXId >= 0 && maxXId >= 0 && minXId != maxXId)
                nodeIds << minXId << maxXId;
        }
        if (nodeIds.size() < 2)
            return;

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        //注入一条两点距离记录（visible=显隐；ui-shot 树联动验证按 一显一隐 两次注入）
        auto injectRecord = [this, md, nodeIds, lang](bool visible)
        {
            MeasureRecord rec;
            rec.typeIndex = 0;
            rec.page = 0;
            rec.ids = nodeIds;
            rec.color = _labelSettings.color;	//与新建口径一致：默认取标签文本色
            rec.visible = visible;
            rec.labels = _labelSettings;
            rec.source = _postDB->getName();
            rec.id = _nextId++;
            const int seq = _typeCounters.value(rec.typeIndex) + 1;
            _typeCounters[rec.typeIndex] = seq;
            rec.name = QString("%1_%2").arg(typeName(rec.typeIndex, lang)).arg(seq);
            //原始结果：两注入节点的基准位置差（X 最远两点；向量类存分量，
            //显示文本按注入记录的标签设置现场格式化）
            const QVector3D p0 = md->getVectorById(nodeIds.front());
            const QVector3D p1 = md->getVectorById(nodeIds.back());
            rec.rawValues = { double(p1.x() - p0.x()), double(p1.y() - p0.y()), double(p1.z() - p0.z()) };
            _records.append(rec);
            syncMeasureCache();

            {
                MxFunctions::mPanelTable::RowUpdateGuard guard(_table);
                const int r = _table->rowCount();
                _table->insertRow(r);
                //首条（显示）勾选置位：显隐按钮注入后可直接联动；次条（隐藏）不勾
                _table->setItem(r, 0, MxFunctions::mPanelTable::makeCheckItem(visible));
                _table->setItem(r, 1, MxFunctions::mPanelTable::makeNameItem(QString(), rec.id));
                for (int col = 2; col <= 4; ++col)
                    _table->setItem(r, col, MxFunctions::mPanelTable::makeTextItem(QString()));
                renderRow(r);
                if (visible)
                    _table->selectRow(r);	//选中注入行：单条曲线模式（默认勾选）绘制需要选中行
            }
        };
        injectRecord(true);
        injectRecord(false);	//隐藏条：验证树节点 Tree_Wrong 置灰徽标与面板眼睛列联动
        applyManagerFilter();
        refreshEmptyState();
        syncManagerState();
        updateMeasureAnnotations();	//注入即出标注
    }

    void mPostMeasureTool::updateMeasureAnnotations()
    {
        //仅后处理视口：标注连线挂在 REBUILD_PostRender 的标注层（前处理视口无此层）
        if (!_isCurrStateValid || !_isCurrPostViewport)
            return;
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        if (_postRend == nullptr)
            return;
        auto postRender = _postRend->getPostRender();
        auto* dimRender = postRender ? postRender->annotations() : nullptr;   // S5 门面直接替换：getDimRender→annotations()（G6，会话一份）
        auto fontRender = postRender ? postRender->getFontRender() : nullptr;
        if (dimRender == nullptr || fontRender == nullptr)
            return;

        //当前显示帧的节点位置（与绘制同式：基准坐标+位移）
        mPostGUIData* guidata = (_postDB != nullptr) ? getCVM->getGUIDataByPostRender(_postRend) : nullptr;
        mPostModelData* modeldata = nullptr;
        QHash<int, QVector3D> dis;
        if (guidata != nullptr && _postDB != nullptr)
        {
            modeldata = _postDB->getModelData(guidata->_cloudFrame);
            if (modeldata != nullptr)
                dis = _postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
        }

        //渲染同款位置参数（0.49）：标注绘制与遮挡判定必须与渲染模型同位——渲染
        //模型按"基准 + 位移×变形系数 + 部件爆炸偏移"显示，原实现用全额位移，
        //变形系数≠1 或开爆炸时标注点与渲染表面错开（斜视深度差达 15-25%，用户
        //实测 60° 误隐藏）。测量数值仍用真实位移（不受显示夸大影响），仅位置同步
        QVector3D deformScale(1.0f, 1.0f, 1.0f);
        QHash<int, QVector3D> explodeDis;
        if (postRender != nullptr)
        {
            if (auto* dataProxy = postRender->frameProxy(postRender->currentFrame()))   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
            {
                if (auto* queryProxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(
                        dataProxy->getRendQueryDataProxy()))
                {
                    deformScale = queryProxy->getDeformationScale();
                    explodeDis = queryProxy->getPartExplodeDis();
                }
            }
        }
        //渲染空间节点位置（连续 id 口径）：基准 + 位移×变形系数 + 部件爆炸偏移
        auto renderPosCid = [&](int cId) {
            const int partId = modeldata->getPartIDByNodeID(cId);
            const QVector3D d = dis.value(cId);
            return modeldata->getVector(cId)
                + QVector3D(d.x() * deformScale.x(), d.y() * deformScale.y(),
                            d.z() * deformScale.z())
                + explodeDis.value(partId);
        };
        auto renderPosReal = [&](int realId) {
            return renderPosCid(modeldata->_nodedata.nodeidmap.value(realId));
        };

        //自动隐藏（0.41 裁定口径 + 0.43 深度图判定）：节点不可见 = 投影出视口
        //（直接判不可见）或该像素场景深度被挡（渲染层 isWorldPointVisible 单像
        //素深度回读，内部自带上下文切换）；多节点测量需全部节点均不可见才隐藏
        //标签（判据见节点分支循环）
        auto anchorOccluded = [&postRender](const QVector3D& p) {
            return postRender != nullptr && !postRender->isWorldPointVisible(p);
        };

        //FR-04 随帧重算（Q2=随帧，用户裁定）：标签数值按当前帧真实计算值现场格式化，
        //创建帧 rawValues 快照仅作取值失败回退。delta 类（1/3/4/6）首帧基准一轮
        //重建只预取一次（评审 S6：避免逐记录大 QHash 拷贝）
        const int frameId = (guidata != nullptr) ? guidata->_cloudFrame : -1;
        mPostModelData* modeldata0 = nullptr;	//首帧基准数据（首个非空帧，plotRecords c0 同款）
        QHash<int, QVector3D> dis0;
        bool haveFirstFrame = false;
        if (guidata != nullptr && _postDB != nullptr
            && std::any_of(_records.cbegin(), _records.cend(), [](const MeasureRecord& r) {
                return r.visible && (r.page == 1 || r.page == 3 || r.page == 4 || r.page == 6);
            }))
        {
            std::vector<QPair<QString, int>> frames;
            _postDB->getCaseFrameNamesAndIDs(guidata->_cloudCase, frames);
            for (const auto& f : frames)
            {
                mPostModelData* md0 = _postDB->getModelData(f.second);
                if (md0 == nullptr) continue;
                modeldata0 = md0;
                dis0 = _postDB->getDisplacementDataByFrameID(f.second);
                haveFirstFrame = true;
                break;
            }
        }
        //标签文本消费"当前帧真实值"记录副本（0.49 口径：数值用真实位移，仅位置/
        //遮挡锚点用上方渲染同款公式；取值失败回退快照，不留空）
        auto recordForFrame = [&](const MeasureRecord& rec) {
            MeasureRecord cur = rec;
            if (frameId >= 0)
            {
                const QVector<double> rv = computeRawValuesForFrame(rec, frameId,
                    modeldata, &dis, modeldata0, haveFirstFrame ? &dis0 : nullptr);
                if (!rv.isEmpty())
                    cur.rawValues = rv;
            }
            return cur;
        };

        std::vector<std::vector<glm::vec3>> polylines;
        std::vector<glm::vec4> colors;
        std::vector<glm::vec3> markerPoses;   //测量节点标记批量（HyperView 口径：拾取节点高亮）
        std::vector<glm::vec4> markerColors;
        QSet<QString> aliveKeys;
        // 测量标注收编（unify-addobject-arch C3）：批量口直调退场（门4）——写路径改为
        // 逐条台账对象提交；本批累积器仅作单条几何切片的中间载体。id 关联=recordId→
        // 台账 objectId 平行表（首建 addObject 接 id、此后恒 updateObject；updateObject
        // 返 false=台账无此 id（含 detachModel 换绑后）→addObject 重建回填=失效自愈）
        auto submitAnnotationObject = [this, &postRender](int recordId, const mxr::MeasureAnnotationParams& p)
        {
            if (postRender == nullptr) return;
            const int known = _recordObjectIds.value(recordId, -1);
            if (known >= 0 && postRender->updateObject(known, p))
                return;
            const int newId = postRender->addObject(p);
            if (newId >= 0)
                _recordObjectIds.insert(recordId, newId);
            else
                _recordObjectIds.remove(recordId);
        };
        // 隐藏条目：已有台账 id 才定向 visible=false（掩码段合并保留几何快照）；无 id
        // 的隐藏条目不建账（防台账膨胀，画面本就无该条）
        auto hideAnnotationObject = [this, &postRender](int recordId)
        {
            if (postRender == nullptr) return;
            const int known = _recordObjectIds.value(recordId, -1);
            if (known < 0) return;
            mxr::MeasureAnnotationParams p;
            p.updateMask = mxr::MeasureAnnotationParams::Mask_Visible;
            p.visible = false;
            if (!postRender->updateObject(known, p))
                _recordObjectIds.remove(recordId);   // 台账已无此 id（换绑）：摘表，下轮可见时重建
        };
        for (const MeasureRecord& rec : _records)
        {
            const size_t plSlice = polylines.size();
            const size_t clSlice = colors.size();
            const size_t mkSlice = markerPoses.size();
            const size_t mcSlice = markerColors.size();
            // 本条切片→台账 params（计算留 GUI，逐条产出；visible 条目带几何）
            auto buildRecordParams = [&rec, &polylines, &colors, &markerPoses, &markerColors,
                                      plSlice, clSlice, mkSlice, mcSlice](bool visible)
            {
                mxr::MeasureAnnotationParams p;
                p.annotationType = rec.page;
                p.visible = visible;
                for (size_t i = plSlice; i < polylines.size(); ++i)
                {
                    QVector<QVector3D> pl;
                    for (const auto& pt : polylines[i])
                        pl.append(QVector3D(pt.x, pt.y, pt.z));
                    p.polylines.append(pl);
                }
                for (size_t i = clSlice; i < colors.size(); ++i)
                    p.colors.append(QVector4D(colors[i].x, colors[i].y, colors[i].z, colors[i].w));
                for (size_t i = mkSlice; i < markerPoses.size(); ++i)
                    p.markerPoses.append(QVector3D(markerPoses[i].x, markerPoses[i].y, markerPoses[i].z));
                for (size_t i = mcSlice; i < markerColors.size(); ++i)
                    p.markerColors.append(QVector4D(markerColors[i].x, markerColors[i].y, markerColors[i].z, markerColors[i].w));
                return p;
            };
            const QString fontKey = QString("mMeasure_%1").arg(rec.id);
            const QString nodeKey = fontKey + "_n";	//节点 ID 标签键（随主键族显隐/回收）
            aliveKeys.insert(fontKey);
            aliveKeys.insert(nodeKey);
            //与绘制同口径：仅当前结果来源的记录参与标注；显隐列直接控制 3D 显示。
            //单节点测量（位置/节点云图）同样参与——标记/节点号/值标签照画，
            //仅"连线"要求 ≥2 点（下方分支处理）
            const bool elemDomain = (rec.page == 11 || rec.page == 12);
            const bool drawable = rec.visible && _postDB != nullptr
                && rec.source == _postDB->getName()
                && modeldata != nullptr
                && !rec.ids.isEmpty();
            if (!drawable)
            {
                //隐藏/不可绘制：文字走 setCommonFontIsShow 隐藏（活消费链 ScalarGraph
                //同款路径；clearCommonFont 仅用于记录删除后的条目回收）
                if (_measureFontKeys.contains(fontKey))
                    fontRender->setCommonFontIsShow(fontKey, false);
                if (_measureFontKeys.contains(nodeKey))
                    fontRender->setCommonFontIsShow(nodeKey, false);
                hideAnnotationObject(rec.id);   // C3：已有台账 id 才定向 visible=false
                continue;
            }

            if (elemDomain)
            {
                //体积/面积（单元拾取）：拾取单元以轮廓线框展示（配方同拾取高亮
                //getHighLightContPointIDsByElementID：轮廓连续ID两两成对；多边形/
                //多面体轮廓表未覆盖，退按单元面逐边补齐），结果文字置于全部拾取
                //单元节点的均值点（形心）——单元间连线无几何意义，不画节点连线
                QVector3D centroid;
                int centroidCount = 0;
                bool hasEdge = false;
                //线框先记插入起点：自动隐藏判定为不可见时回退（整组隐藏）
                const size_t edgeStart = polylines.size();
                const size_t colorStart = colors.size();
                const glm::vec4 recColor(rec.color.redF(), rec.color.greenF(),
                                         rec.color.blueF(), 1.0f);
                for (int eleRid : rec.ids)
                {
                    const int partId = modeldata->getPartIDByEleID(eleRid);
                    if (partId < 0)
                        continue;
                    auto* part = modeldata->getPartData(partId).data();
                    if (part == nullptr)
                        continue;
                    const auto cit = part->_parteledata.cell_rid_to_cid.find(eleRid);
                    if (cit == part->_parteledata.cell_rid_to_cid.end())
                        continue;
                    const PostCelllType eleType = part->_parteledata.cell_types[cit->second];
                    const bool polyType = (eleType == PostCelllType::MX_POLYGON
                        || eleType == PostCelllType::MX_POLYHEDRON);

                    //形心：单元全节点（含高阶中节点，各计一次）——渲染空间位置
                    const QList<int> nodeCIds = modeldata->getElementNodeId(partId, eleRid);
                    for (int cId : nodeCIds)
                    {
                        centroid += renderPosCid(cId);
                        ++centroidCount;
                    }

                    if (polyType)
                    {
                        for (int faceId : modeldata->getFaceIDsByElementID(partId, eleRid))
                        {
                            const std::vector<int> faceNodes =
                                modeldata->getFaceNodeIdsById(partId, faceId);
                            if (faceNodes.size() < 2)
                                continue;
                            for (size_t i = 0; i < faceNodes.size(); ++i)
                            {
                                const int nxt = faceNodes[(i + 1) % faceNodes.size()];
                                QVector<QVector3D> edgePts{
                                    modeldata->getVector(faceNodes[i]) + dis.value(faceNodes[i]),
                                    modeldata->getVector(nxt) + dis.value(nxt) };
                                std::vector<glm::vec3> edge;
                                edge.push_back({ edgePts[0].x(), edgePts[0].y(), edgePts[0].z() });
                                edge.push_back({ edgePts[1].x(), edgePts[1].y(), edgePts[1].z() });
                                polylines.push_back(std::move(edge));
                                colors.push_back(recColor);
                                hasEdge = true;
                            }
                        }
                    }
                    else
                    {
                        std::vector<int> contour;
                        if (!modeldata->getHighLightContPointIDsByElementID(eleRid, partId, contour))
                            continue;
                        for (size_t k = 0; k + 1 < contour.size(); k += 2)
                        {
                            QVector<QVector3D> edgePts{
                                modeldata->getVector(contour[k]) + dis.value(contour[k]),
                                modeldata->getVector(contour[k + 1]) + dis.value(contour[k + 1]) };
                            std::vector<glm::vec3> edge;
                            edge.push_back({ edgePts[0].x(), edgePts[0].y(), edgePts[0].z() });
                            edge.push_back({ edgePts[1].x(), edgePts[1].y(), edgePts[1].z() });
                            polylines.push_back(std::move(edge));
                            colors.push_back(recColor);
                            hasEdge = true;
                        }
                    }
                }
                if (!hasEdge && centroidCount <= 0)
                {
                    if (_measureFontKeys.contains(fontKey))
                        fontRender->setCommonFontIsShow(fontKey, false);
                    if (_measureFontKeys.contains(nodeKey))
                        fontRender->setCommonFontIsShow(nodeKey, false);
                    hideAnnotationObject(rec.id);   // C3：已有台账 id 才定向 visible=false
                    continue;
                }
                //自动隐藏（0.46）：形心被遮挡 → 线框与文字整组隐藏（线框画在恒
                //可见层，需与文本同步摘除，回退本记录的线框批次）
                const bool annotationVisible = !rec.labels.autoHide
                    || centroidCount <= 0
                    || !anchorOccluded(centroid / float(centroidCount));
                if (!annotationVisible)
                {
                    polylines.resize(edgeStart);
                    colors.resize(colorStart);
                    if (_measureFontKeys.contains(fontKey))
                        fontRender->setCommonFontIsShow(fontKey, false);
                    if (_measureFontKeys.contains(nodeKey))
                        fontRender->setCommonFontIsShow(nodeKey, false);
                    hideAnnotationObject(rec.id);   // C3：自动隐藏→台账条目 visible=false（画面等价整组隐藏）
                    continue;
                }
                if (centroidCount > 0)
                {
                    //结果文字：拾取单元节点均值点（形心）。文本/字号消费记录标签
                    //设置；颜色=记录色（默认取标签文本色，列表调整后随调整色）；
                    //背景框按标签设置的填充开关与填充颜色。走到此处必可见
                    const QVector3D centroidPt = centroid / float(centroidCount);
                    fontRender->appendCommonFont(fontKey,
                        QVector<QVector3D>{ centroidPt },
                        QVector<QString>{ formatMeasureLabelLines(recordForFrame(rec), lang) },
                        QVector3D(rec.color.redF(), rec.color.greenF(), rec.color.blueF()),
                        float(rec.labels.fontSize),
                        rec.labels.fill,
                        QVector3D(rec.labels.fillColor.redF(), rec.labels.fillColor.greenF(),
                                  rec.labels.fillColor.blueF()));
                    fontRender->setCommonFontIsShow(fontKey, true);
                }
                submitAnnotationObject(rec.id, buildRecordParams(true));   // C3：单元线框条=纯折线无标记
                continue;
            }

            QVector<QVector3D> pts;
            for (int id : rec.ids)
                pts.append(renderPosReal(id));	//渲染空间位置（含变形系数/爆炸偏移）
            if (pts.isEmpty())
            {
                if (_measureFontKeys.contains(fontKey))
                    fontRender->setCommonFontIsShow(fontKey, false);
                if (_measureFontKeys.contains(nodeKey))
                    fontRender->setCommonFontIsShow(nodeKey, false);
                hideAnnotationObject(rec.id);   // C3：已有台账 id 才定向 visible=false
                continue;
            }

            //自动隐藏（0.41/0.46 裁定）：标注点/线/节点号/文本作为**整体**随测量
            //对象可见性隐现（HyperView 口径：测量是单一显示实体）——多节点需全部
            //节点被遮挡才隐藏，任一节点可见即整组显示；单节点即该节点自身
            bool annotationVisible = !rec.labels.autoHide;
            if (rec.labels.autoHide)
            {
                for (const QVector3D& p : pts)
                {
                    if (!anchorOccluded(p))
                    {
                        annotationVisible = true;
                        break;
                    }
                }
            }
            if (!annotationVisible)
            {
                //整组隐藏：文本与节点号标签隐藏，标记/线不入批（批次内无该记录）
                if (_measureFontKeys.contains(fontKey))
                    fontRender->setCommonFontIsShow(fontKey, false);
                if (_measureFontKeys.contains(nodeKey))
                    fontRender->setCommonFontIsShow(nodeKey, false);
                hideAnnotationObject(rec.id);   // C3：自动隐藏→台账条目 visible=false（画面等价整组隐藏）
                continue;
            }

            //节点标记（HyperView 口径：拾取节点高亮，记录色圆点恒可见，随换帧/
            //显隐/删除联动）。单节点测量（位置/节点云图）同样显示
            for (const QVector3D& p : pts)
                markerPoses.push_back({ p.x(), p.y(), p.z() });
            {
                const glm::vec4 recColor(rec.color.redF(), rec.color.greenF(),
                                         rec.color.blueF(), 1.0f);
                for (int i = 0; i < pts.size(); ++i)
                    markerColors.push_back(recColor);
            }

            //结果文字锚点：角度类（夹角/增量角度，第2拾取为角点）置于角点附近——
            //沿角平分线向角内偏移短边长的 1/4（标注贴近夹角，且避开角点上的节点
            //号标签）；单节点（位置/节点云图）锚在节点处；其余多节点=首末点中点
            QVector3D labelAnchor;
            if ((rec.page == 5 || rec.page == 6) && pts.size() >= 3)
            {
                const QVector3D vtx = pts[1];
                const QVector3D d1 = pts[0] - vtx;
                const QVector3D d2 = pts[2] - vtx;
                const float l1 = d1.length();
                const float l2 = d2.length();
                if (l1 > 1e-6f && l2 > 1e-6f)
                {
                    const QVector3D bisector = (d1 / l1 + d2 / l2).normalized();
                    labelAnchor = vtx + bisector * (0.25f * qMin(l1, l2));
                }
                else
                    labelAnchor = vtx;	//退化（共点）：直接锚角点
            }
            else
            {
                labelAnchor = (pts.size() >= 2)
                    ? (pts.front() + pts.back()) / 2.0f : pts.front();
            }

            //单节点测量（位置/节点云图）：值标签挂到节点**右下方**（HyperView
            //口径）——沿相机右向偏 10px、向下偏"标签高度+6px"，节点圆点完全
            //露出，首行节点号悬在框顶清晰可读。标签高度按行数×行距×字号估算
            if (pts.size() == 1 && postRender != nullptr)
            {
                const mxr::CameraState& cam = postRender->getLastCameraState();
                const glm::dvec3 fwd(cam.eye_dir.x, cam.eye_dir.y, cam.eye_dir.z);
                const glm::dvec3 upv(cam.up.x, cam.up.y, cam.up.z);
                const glm::dvec3 rightv = glm::normalize(glm::cross(fwd, upv));
                const double wpp = cam.world_per_pixel;
                //锚点 = 首行基线：框顶在锚点上方 ≈0.425×字号（首行上伸部分），
                //后续行向下生长——下移量只需首行上伸+间隙，与行数无关
                const double offRight = wpp * 10.0;
                const double offDown = wpp * (0.425 * rec.labels.fontSize + 6.0);
                labelAnchor += QVector3D(
                    float((rightv.x * offRight) - (upv.x * offDown)),
                    float((rightv.y * offRight) - (upv.y * offDown)),
                    float((rightv.z * offRight) - (upv.z * offDown)));
            }

            //节点号小标签：多节点测量逐节点画（小字号随记录色，恒显——节点号
            //标注不挂在显示前缀上）；单节点测量的节点号在值标签首行（见
            //formatMeasureLabelLines，避免与值标签同锚重叠）
            if (pts.size() >= 2)
            {
                QVector<QVector3D> idPos;
                QVector<QString> idTxt;
                for (int i = 0; i < pts.size(); ++i)
                {
                    idPos.append(pts[i]);
                    idTxt.append(QString::number(rec.ids.at(i)));
                }
                fontRender->appendCommonFont(nodeKey, idPos, idTxt,
                    QVector3D(rec.color.redF(), rec.color.greenF(), rec.color.blueF()),
                    qMax(12.0f, float(rec.labels.fontSize) * 0.75f));
                fontRender->setCommonFontIsShow(nodeKey, true);
            }
            else if (_measureFontKeys.contains(nodeKey))
            {
                fontRender->setCommonFontIsShow(nodeKey, false);
            }

            //连线仅多节点测量绘制（≥2）；最小距离（P2 点对集）画基准→当前帧最近
            //对比节点，其余按拾取序连线
            if (pts.size() >= 2)
            {
                std::vector<glm::vec3> pl;
                if (rec.page == 10)
                {
                    int nearest = 1;
                    float bestLen = (pts[1] - pts[0]).length();
                    for (int i = 2; i < pts.size(); ++i)
                    {
                        const float d = (pts[i] - pts[0]).length();
                        if (d < bestLen)
                        {
                            bestLen = d;
                            nearest = i;
                        }
                    }
                    pl.push_back({ pts[0].x(), pts[0].y(), pts[0].z() });
                    pl.push_back({ pts[nearest].x(), pts[nearest].y(), pts[nearest].z() });
                }
                else
                {
                    for (const QVector3D& p : pts)
                        pl.push_back({ p.x(), p.y(), p.z() });
                }
                polylines.push_back(std::move(pl));
                colors.push_back({ float(rec.color.redF()), float(rec.color.greenF()),
                                   float(rec.color.blueF()), 1.0f });
            }

            //结果文字：文本/字号消费记录标签设置（位置测量多行 X/Y/Z 且首行节点
            //号，向量类主值按显示前缀开关带 Mag/X 标注）；颜色=记录色；背景框按
            //标签设置的填充开关与填充颜色。走到此处必可见（不可见已提前整组隐藏）
            fontRender->appendCommonFont(fontKey,
                QVector<QVector3D>{ labelAnchor },
                QVector<QString>{ formatMeasureLabelLines(recordForFrame(rec), lang) },
                QVector3D(rec.color.redF(), rec.color.greenF(), rec.color.blueF()),
                float(rec.labels.fontSize),
                rec.labels.fill,
                QVector3D(rec.labels.fillColor.redF(), rec.labels.fillColor.greenF(),
                          rec.labels.fillColor.blueF()));
            fontRender->setCommonFontIsShow(fontKey, true);
            submitAnnotationObject(rec.id, buildRecordParams(true));   // C3：本条对象提交（标记+折线几何切片）
        }
        //回收已删除记录的字体条目（删除后不在 _records，循环够不到）
        for (const QString& k : _measureFontKeys)
            if (!aliveKeys.contains(k))
                fontRender->clearCommonFont(k);
        _measureFontKeys = aliveKeys;
        // 测量标注收编（unify-addobject-arch）：批量口直调退场（门4）——上述循环内
        // 已逐条提交台账对象（渲染端 rebuildMeasureAnnotationsInternal 统一 clear+
        // 批量提交，净产物与原"整批提交"链逐像素等价）
        //FR-05：无论哪个入口触发重建，同步刷新换帧去重基准——任何入口触发后缓存
        //不陈旧，后续真实换帧不会被误跳过
        _annotatedFrame = (guidata != nullptr) ? guidata->_cloudFrame : -1;
        //Capability 层更新按需消费：不请求重绘时视口停留旧帧（断点暂停反而"生效"
        //即此因）——标注/文字变更后强制视口重绘（同标注面板 updatePaint 惯例）
        if (_postRend != nullptr)
            _postRend->updatePaint();
        //FR-04：复核弹窗已展开详情行可见时随本次重建同步重算刷新（不可见内部自跳过）
        refreshExpandedReviewDetails();
    }

    void mPostMeasureTool::clearMeasureAnnotations()
    {
        if (!_isCurrStateValid || !_isCurrPostViewport || _postRend == nullptr)
            return;
        auto postRender = _postRend->getPostRender();
        auto* dimRender = postRender ? postRender->annotations() : nullptr;   // S5 门面直接替换：getDimRender→annotations()（G6，会话一份）
        Q_UNUSED(dimRender);
        auto fontRender = postRender ? postRender->getFontRender() : nullptr;
        // 测量标注收编（unify-addobject-arch）：批量口直调退场（门4）——清台账
        //（removeAllSceneObjects 逐条 removeObject 触发 rebuild 收敛为空槽）并重置
        // 平行 id 表；字体标签照旧直清
        if (postRender != nullptr)
            PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::MeasureAnnotation);
        _recordObjectIds.clear();
        if (fontRender != nullptr)
            for (const QString& k : _measureFontKeys)
                fontRender->clearCommonFont(k);
        _measureFontKeys.clear();
        if (_postRend != nullptr)
            _postRend->updatePaint();
    }

    //FR-04 随帧重算原语（plotRecords 同口径，评审 B2 已核）：节点位置=基准坐标+全位移
    //（真实位移，不含变形系数/爆炸偏移——0.49 裁定）；calcValue 桶={1,4,5,6,7,9,10}、
    //c0 基准桶={1,3,4,6}（评审 S1 点名）、calc8 走激活云图变量逐帧查值、calc11/12 走引擎
    //逐帧。返回空向量=当前帧取值失败（激活结果非节点数据/缺值/空帧），调用方回退创建帧
    //快照，不留空、不崩溃
    QVector<double> mPostMeasureTool::computeRawValuesForFrame(const MeasureRecord& rec, int frameId,
        PostData::mPostModelData* curModel, const QHash<int, QVector3D>* curDis,
        PostData::mPostModelData* firstModel, const QHash<int, QVector3D>* firstDis) const
    {
        if (_postDB == nullptr || frameId < 0 || rec.ids.isEmpty())
            return {};

        //节点云图：按当前激活云图变量查该帧节点值（plotRecords calc8 同款，flag≠0/缺值失败）
        if (rec.page == 8)
        {
            mPostModelData* md = (curModel != nullptr) ? curModel : _postDB->getModelData(frameId);
            int resultFlag = -1;
            const QHash<int, double> frameVals = _postDB->getResultDataByFrameID(frameId, resultFlag);
            const int cId = (md != nullptr) ? md->_nodedata.nodeidmap.value(rec.ids.at(0)) : -1;
            if (resultFlag != 0 || cId < 0 || !frameVals.contains(cId))
                return {};
            return { frameVals.value(cId) };
        }

        //体积/面积：引擎按拾取对象逐帧计算（plotRecords calc11/12 同款）
        if (rec.page == 11 || rec.page == 12)
        {
            std::vector<int> objIds(rec.ids.cbegin(), rec.ids.cend());	//引擎接口为非 const 引用
            const double v = (rec.page == 11)
                ? _postDB->CaluElementsVolumeByFrame(objIds, frameId)
                : _postDB->CaluElementsAreaByFrame(objIds, frameId);
            return { v };
        }

        //其余：节点位置原语求值（plotRecords else 分支同款）
        mPostModelData* md = (curModel != nullptr) ? curModel : _postDB->getModelData(frameId);
        if (md == nullptr)
            return {};
        QHash<int, QVector3D> localDis;
        if (curDis == nullptr)
        {
            localDis = _postDB->getDisplacementDataByFrameID(frameId);
            curDis = &localDis;
        }
        QVector<QVector3D> c;
        c.reserve(rec.ids.size());
        for (int id : rec.ids)
        {
            const int lx = md->_nodedata.nodeidmap.value(id);
            c.append(md->getVectorById(id) + curDis->value(lx));
        }

        //delta 类（1/3/4/6）：首帧基准位置（首个非空帧；取不到时 calcValue 内退化为当前值）
        QVector<QVector3D> c0;
        if (rec.page == 1 || rec.page == 3 || rec.page == 4 || rec.page == 6)
        {
            mPostModelData* md0 = firstModel;
            QHash<int, QVector3D> localDis0;
            if (md0 == nullptr || firstDis == nullptr)
            {
                mPostGUIData* gd = (_postRend != nullptr)
                    ? getCVM->getGUIDataByPostRender(_postRend) : nullptr;
                if (gd != nullptr)
                {
                    std::vector<QPair<QString, int>> frames;
                    _postDB->getCaseFrameNamesAndIDs(gd->_cloudCase, frames);
                    for (const auto& f : frames)
                    {
                        mPostModelData* m0 = _postDB->getModelData(f.second);
                        if (m0 == nullptr) continue;
                        localDis0 = _postDB->getDisplacementDataByFrameID(f.second);
                        md0 = m0;
                        break;
                    }
                }
            }
            const QHash<int, QVector3D>* d0 = (firstDis != nullptr) ? firstDis : &localDis0;
            if (md0 != nullptr)
            {
                for (int id : rec.ids)
                {
                    const int lx = md0->_nodedata.nodeidmap.value(id);
                    c0.append(md0->getVectorById(id) + d0->value(lx));
                }
            }
        }

        //raw 向量按 buildRecordFromInput 同款契约组装；标量/角度类取值走 calcValue
        //（与详情曲线 plotRecords 严格同源，FR-04 同源对拍验收口径）
        const QVector<QVector3D>& c0u = c0.size() >= c.size() ? c0 : c;
        switch (rec.page)
        {
        case 0: { const QVector3D v = c[1] - c[0]; return { v.x(), v.y(), v.z() }; }		//两点距离
        case 1: return { calcValue(1, c, c0, 3) };										//增量距离（标量差）
        case 2: return { c[0].x(), c[0].y(), c[0].z() };									//位置
        case 3: { const QVector3D v = (c[0] - c0u[0]) - (c[1] - c0u[1]);				//相对位移
                  return { v.x(), v.y(), v.z() }; }
        case 4: return { calcValue(4, c, c0, 3) };										//相对角度（度）
        case 5: return { calcValue(5, c, c0, 3) };										//夹角（度）
        case 6: return { calcValue(6, c, c0, 3) };										//增量角度（度）
        case 7: return { calcValue(7, c, c0, 3) };										//圆半径
        case 9: return { calcValue(9, c, c0, 3) };										//节点路径（折线总长）
        case 10: return { calcValue(10, c, c0, 3) };									//最小距离
        }
        return {};
    }

    //云图/动画换帧全局通知（postCloudFrameChanged，全局单例信号、不带视口名）：
    //以"本视口帧号是否真实变化"为去重判据（02 §2 FR-01 视口隔离口径：他视口换帧
    //本视口帧号未变 → 去重跳过）；播放中按 Q3 落帧口径门控
    void mPostMeasureTool::slot_onCloudFrameChanged()
    {
        //守卫与 updateMeasureAnnotations 同款（评审 S4：_postRend 空判前置）
        if (!_isCurrStateValid || !_isCurrPostViewport || _postRend == nullptr || _postDB == nullptr)
            return;
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_postRend);
        if (guidata == nullptr)
            return;
        //Q3-B 落帧口径（用户裁定）：播放/一键生成自动播放态不刷（含遮挡重评），
        //停止/暂停/拖帧等非播放态落定即刷——停止点补发通知在 _aniIsStart 置 false
        //后到达，此处门控放行。
        //门控用 _generateAnimate && _aniIsStart 两旗标（用户真机修正）：_aniIsStart
        //曾在非播放态残留 true（一键生成路径启动自动播放时置位），单旗标门控会误杀
        //全部手动换帧——症状为换帧后标注不跟随、回测量界面才重绘。
        //真播放态两旗标同真，落帧语义不变；idle/无动画场景 _generateAnimate=false 放行
        if (guidata->_generateAnimate && guidata->_aniIsStart)
            return;
        //P2-2 审计修复：实时动画播放态并入同门控——实时浮层逐帧 tick 每帧经
        //slot_showCloud 尾广播 postCloudFrameChanged，旧门控只认预生成动画面板两旗标，
        //实时播放中逐帧放行 updateMeasureAnnotations（含自动隐藏的 GL 深度读路径），
        //50fps 深度回读卡顿，且与预生成动画"播放中不刷、停止补刷"裁定口径分叉。
        //停止点补发通知（浮层停机补刷）到达时浮层已非播放态，此处门控放行补刷——
        //语义=Q3-B 口径的等价扩展（D15 登记：不引入新时序补丁）
        if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
        {
            if (cloudPanel->isRealtimeAnimPlaying())
                return;
        }
        //帧号去重：本视口帧号未真实变化（同帧重应用/他视口换帧）不重建、不发生 GL 深度读
        if (guidata->_cloudFrame == _annotatedFrame)
            return;
        updateMeasureAnnotations();
    }

    //FR-04：复核弹窗已展开详情行随帧重算刷新（02 §2 消费面矩阵"推"侧；单条记录
    //重算代价可忽略）。弹窗不可见/无展开行时不刷——下次打开弹窗 renderReviewTable
    //按当前帧现场格式化（"拉"口径自然刷新）
    void mPostMeasureTool::refreshExpandedReviewDetails()
    {
        if (_rvTable == nullptr || _reviewPopup == nullptr || !_reviewPopup->isVisible())
            return;
        if (_expandedReviewIds.isEmpty())
            return;
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        mPostGUIData* gd = (_postRend != nullptr && _postDB != nullptr)
            ? getCVM->getGUIDataByPostRender(_postRend) : nullptr;
        const int frameId = (gd != nullptr) ? gd->_cloudFrame : -1;
        for (int recordId : qAsConst(_expandedReviewIds))
        {
            const int di = dataIndexById(recordId);
            if (di < 0) continue;
            int row = -1;
            for (int r = 0; r < _rvTable->rowCount(); ++r)
            {
                if (QTableWidgetItem* it = _rvTable->item(r, 0);
                    it && it->data(Qt::UserRole).toInt() == recordId) { row = r; break; }
            }
            if (row < 0 || row + 1 >= _rvTable->rowCount()) continue;
            MeasureRecord cur = _records.at(di);
            if (frameId >= 0)
            {
                const QVector<double> rv = computeRawValuesForFrame(cur, frameId,
                    nullptr, nullptr, nullptr, nullptr);
                if (!rv.isEmpty())
                    cur.rawValues = rv;
            }
            _rvTable->removeRow(row + 1);
            insertReviewDetailRow(row + 1, cur);
        }
    }

    void mPostMeasureTool::plotRecords(const QList<int>& recordIds)
    {
        if (!_isCurrStateValid) return;
        if (!_isCurrPostViewport) return;
        if (_postDB == nullptr) return;
        if (_postRend == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_postRend);
        if (guidata == nullptr)
        {
            int langGuard = MAppConfiguration::mAppConfiguration::getInstance()->_language;
            setStatusLabel(langGuard == 0 ? QString::fromUtf8("当前视口无结果数据，无法绘制曲线。")
                : QString("No result data in current viewport."));
            return;
        }
        std::vector<QPair<QString, int>> fnameAndIds;           // <帧名, 帧ID>
        _postDB->getCaseFrameNamesAndIDs(guidata->_cloudCase, fnameAndIds);

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        const bool byFrameNo = _rvXCombo && _rvXCombo->currentIndex() == 1;
        //纵轴 yIdx 由选项 userData 携带（0X/1Y/2Z/3合量；标量"结果"项同为 3）
        const int yIdx = (_rvYCombo && _rvYCombo->currentData().isValid())
            ? _rvYCombo->currentData().toInt() : 3;

        //X轴先按有效帧一次性生成（剔除空帧），各记录结果与之严格对齐
        QVector<double> xs;
        std::vector<int> frameIds;
        int frameIndex = 0;
        for (auto& f : fnameAndIds)
        {
            mPostModelData* modeldata = _postDB->getModelData(f.second);
            ++frameIndex;
            if (modeldata == nullptr) continue;
            xs.push_back(byFrameNo ? frameIndex : modeldata->_frameInfo._frameTime);
            frameIds.push_back(f.second);
        }

        QVector<QVector<double>> _ysList;                        // 每条记录一组结果
        QStringList             headers;
        QString                 lastTypeName;

        for (int recordId : recordIds)
        {
            const int di = dataIndexById(recordId);
            if (di < 0) continue;
            const MeasureRecord rec = _records.at(di);
            const int calc = rec.page;

            //仅绘制当前结果来源的记录（其他结果的节点/帧不适用；历史记录保留在列表）
            if (rec.source != _postDB->getName()) continue;

            //节点数量不足的记录不参与绘制
            if ((int)rec.ids.size() < nodesForCalc(calc)) continue;
            lastTypeName = typeName(rec.typeIndex, lang);

            //delta 类：首帧位置基准
            QVector<QVector3D> c0;
            if (!frameIds.empty() && (calc == 1 || calc == 3 || calc == 4 || calc == 6 || calc == 8))
            {
                if (mPostModelData* md0 = _postDB->getModelData(frameIds.front()))
                {
                    const QHash<int, QVector3D> dis0 = _postDB->getDisplacementDataByFrameID(frameIds.front());
                    for (int id : rec.ids)
                    {
                        const int lx = md0->_nodedata.nodeidmap.value(id);
                        c0.append(md0->getVectorById(id) + dis0.value(lx));
                    }
                }
            }

            //逐帧：当前记录各节点位置 → 统一求值；体积/面积按拾取对象走引擎逐帧
            //计算；节点云图按当前激活云图变量逐帧查值（P0）
            QVector<double> results;
            if (calc == 8)
            {
                bool usable = true;
                std::vector<double> contourVals;
                for (int frameId : frameIds)
                {
                    mPostModelData* modeldata = _postDB->getModelData(frameId);	//frameIds 已过滤空帧
                    int resultFlag = -1;
                    const QHash<int, double> frameVals = _postDB->getResultDataByFrameID(frameId, resultFlag);
                    const int cId = modeldata ? modeldata->_nodedata.nodeidmap.value(rec.ids.at(0)) : -1;
                    if (resultFlag != 0 || cId < 0 || !frameVals.contains(cId))
                    {
                        usable = false;	//激活结果非节点数据或该帧缺值：整条不参与绘制（不画 0）
                        break;
                    }
                    contourVals.push_back(frameVals.value(cId));
                }
                if (!usable)
                    continue;
                results.reserve(static_cast<int>(contourVals.size()));
                for (double v : contourVals)
                    results.append(v);
            }
            else if (calc == 11 || calc == 12)
            {
                std::vector<int> objIds(rec.ids.cbegin(), rec.ids.cend());	//引擎接口为非 const 引用
                for (int frameId : frameIds)
                    results.append((calc == 11)
                        ? _postDB->CaluElementsVolumeByFrame(objIds, frameId)
                        : _postDB->CaluElementsAreaByFrame(objIds, frameId));
            }
            else
            {
                for (int frameId : frameIds)
                {
                    mPostModelData* modeldata = _postDB->getModelData(frameId);	//frameIds 已过滤空帧
                    const QHash<int, QVector3D> dis = _postDB->getDisplacementDataByFrameID(frameId);
                    QVector<QVector3D> ct;
                    for (int id : rec.ids)
                    {
                        const int lx = modeldata->_nodedata.nodeidmap.value(id);
                        ct.append(modeldata->getVectorById(id) + dis.value(lx));
                    }
                    results.append(calcValue(calc, ct, c0, yIdx));
                }
            }
            if (!results.isEmpty())
            {
                _ysList.append(results);
                headers.append(rec.name);
            }
        }

        if (_ysList.isEmpty())
        {
            setStatusLabel(lang == 0 ? QString::fromUtf8("暂无可绘制的测量记录。") : QString("No measurable records."));
            return;
        }

        //绘制到详情弹窗右侧的 PlotView（预览图；与曲线窗口同款绘图组件）
        if (_rvPlotView == nullptr)
            return;
        _rvPlotView->clearAll();
        for (int i = 0; i < _ysList.size(); ++i)
            _rvPlotView->updateData(headers.isEmpty() ? QString() : headers.at(i), xs, _ysList.at(i));
        _rvPlotView->rescaleAxes();
        //轴标题同曲线窗口口径（X=帧号/秒；Y=单条取类型名/多条取"测量值"）
        PlotView::AxisSettings xs_ = _rvPlotView->getAxisSettings(PlotView::X_Top);
        xs_.title = byFrameNo ? (lang == 0 ? QString::fromUtf8("帧号") : QString("Frame"))
            : (lang == 0 ? QString::fromUtf8("秒") : QString("s"));
        _rvPlotView->updateAxisScale(PlotView::X_Top, xs_);
        PlotView::AxisSettings ys_ = _rvPlotView->getAxisSettings(PlotView::Y_Right);
        ys_.title = _ysList.size() == 1 ? lastTypeName
            : (lang == 0 ? QString::fromUtf8("测量值") : QString("Measure"));
        _rvPlotView->updateAxisScale(PlotView::Y_Right, ys_);
        //创建时显式 hide() 过，布局不会自动唤起——不 show() 画了也看不见（0.25 轮实证）
        _rvPlotView->show();
        if (_plotPlaceholder)
            _plotPlaceholder->hide();
        _reviewPlotted = true;	//轴切换重绘的门控（slot_reviewAxisChanged）
    }

    void mPostMeasureTool::slot_plotCollapseClicked()
    {
        if (!_plotSection) return;
        const bool show = !_plotSection->isVisible();
        _plotSection->setVisible(show);
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        _btnCollapsePlot->setText(show ? (lang == 0 ? QString::fromUtf8("收起绘图") : QString("Collapse Plot"))
            : (lang == 0 ? QString::fromUtf8("展开绘图") : QString("Expand Plot")));
        //收起后收窄弹窗（单列 496 + 边距），展开恢复全宽；高度同 536 口径
        _reviewPopup->setFixedSize(show ? 1000 : 540, 536);
        _reviewPopup->adjustSize();
    }

    void mPostMeasureTool::syncReviewState()
    {
        if (!_rvTable) return;

        //纵轴选项随绘制目标记录的适用性重建（选中/单条曲线开关/列表内容变化均经过本函数）
        refreshYAxisOptions();

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

        const auto rows = _rvTable->selectionModel() ? _rvTable->selectionModel()->selectedRows() : QModelIndexList{};
        const bool hasSel = rows.size() == 1;

        //单条曲线：绘制所选记录，需先选中；多曲线：绘制全部"显示"记录，无需选择但至少一条可见
        const bool single = _rvSingleCurve && _rvSingleCurve->isChecked();
        bool hasVisible = false;
        for (const MeasureRecord& rec : _records)
            if (rec.visible) { hasVisible = true; break; }
        const bool hasSource = single ? hasSel : hasVisible;

        const bool canPlot = hasSource && _isCurrPostViewport && _postDB != nullptr && _isCurrStateValid;
        _rvPlotBtn->setEnabled(canPlot);
        QString tip;
        if (!_isCurrPostViewport)
            tip = lang == 0 ? QString::fromUtf8("仅后处理支持曲线生成") : QString("Curves are available in postprocessing only");
        else if (_records.isEmpty())
            tip = lang == 0 ? QString::fromUtf8("尚无测量结果，暂不可绘制曲线") : QString("No measurement results to plot yet");
        else if (!hasSource)
            tip = single ? (lang == 0 ? QString::fromUtf8("请选择测量记录") : QString("Select a record"))
                : (lang == 0 ? QString::fromUtf8("没有显示状态的测量记录") : QString("No visible records"));
        _rvPlotBtn->setToolTip(tip);
    }

    void mPostMeasureTool::refreshYAxisOptions()
    {
        if (!_rvYCombo)
            return;
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

        //绘制目标记录的适用纵轴（需求 V1.1 §5：只提供适用的纵轴内容，不把标量结果
        //当作 X、Y、Z 分量）：单条曲线=当前选中记录；多条=全部显示记录。
        //目标全为向量类分支（两点距离/增量距离/位置/相对位移）时给 X/Y/Z/合量分量，
        //含标量类型时仅"结果"——标量分支本就忽略分量取值。
        //无适用目标（空态/未选中）时维持原型默认全量四项（此时无可绘制记录，
        //且保持与原型空态逐控件一致）
        QSet<int> calcs;
        if (_rvSingleCurve && _rvSingleCurve->isChecked())
        {
            if (_rvTable && _rvTable->selectionModel())
            {
                const auto rows = _rvTable->selectionModel()->selectedRows();
                if (rows.size() == 1)
                {
                    if (QTableWidgetItem* it = _rvTable->item(rows.at(0).row(), 0))
                    {
                        const int di = dataIndexById(it->data(Qt::UserRole).toInt());
                        if (di >= 0)
                            calcs.insert(_records.at(di).page);
                    }
                }
            }
        }
        else
        {
            for (const MeasureRecord& rec : _records)
                if (rec.visible)
                    calcs.insert(rec.page);
        }

        bool vectorOnly = true;
        for (int calc : calcs)
        {
            if (!calcHasComponents(calc))
            {
                vectorOnly = false;
                break;
            }
        }

        //按 userData 恢复原选择（跨重建/语言切换保留已选轴设置）
        const int keepY = _rvYCombo->currentData().isValid() ? _rvYCombo->currentData().toInt() : 0;
        QSignalBlocker block(_rvYCombo);	//重建不触发轴切换重绘
        _rvYCombo->clear();
        if (vectorOnly)
        {
            _rvYCombo->addItem(QStringLiteral("X"), 0);
            _rvYCombo->addItem(QStringLiteral("Y"), 1);
            _rvYCombo->addItem(QStringLiteral("Z"), 2);
            _rvYCombo->addItem(lang == 0 ? QString::fromUtf8("合量") : QString("Sum"), 3);
        }
        else
        {
            _rvYCombo->addItem(lang == 0 ? QString::fromUtf8("结果") : QString("Result"), 3);
        }
        const int restore = _rvYCombo->findData(keepY);
        _rvYCombo->setCurrentIndex(restore >= 0 ? restore : 0);
    }

    void mPostMeasureTool::insertReviewDetailRow(int row, const MeasureRecord& rec)
    {
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        const LabelSettings& lb = rec.labels;
        auto yn = [lang](bool v) { return v ? (lang == 0 ? QString::fromUtf8("是") : QString("Yes"))
            : (lang == 0 ? QString::fromUtf8("否") : QString("No")); };
        QString mag;
        if (lb.magnitude >= 0 && lb.magnitude < 8)
            mag = QString::fromUtf8(lang == 0 ? kMagnitudeCn[lb.magnitude] : kMagnitudeEn[lb.magnitude]);
        const QList<QPair<QString, QString>> fields =
        {
            { lang == 0 ? QString::fromUtf8("名称") : QString("Name"), rec.name.toHtmlEscaped() },
            { lang == 0 ? QString::fromUtf8("类型") : QString("Type"), typeName(rec.typeIndex, lang) },
            { lang == 0 ? QString::fromUtf8("颜色") : QString("Color"), rec.color.name().toHtmlEscaped() },
            { lang == 0 ? QString::fromUtf8("显示状态") : QString("Visible"),
                rec.visible ? (lang == 0 ? QString::fromUtf8("显示") : QString("On"))
                            : (lang == 0 ? QString::fromUtf8("隐藏") : QString("Off")) },
            { lang == 0 ? QString::fromUtf8("节点/对象") : QString("Objects"), recordObjectText(rec, lang).toHtmlEscaped() },
            { lang == 0 ? QString::fromUtf8("测量结果") : QString("Result"),
                formatMeasureText(rec, lang).replace("\n", "  ").toHtmlEscaped() },
            { lang == 0 ? QString::fromUtf8("坐标系") : QString("Coord Sys"),
                lang == 0 ? QString::fromUtf8("未指定") : QString("Unspecified") },	//记录暂无坐标系概念，与列表"坐标系"列同口径
            { lang == 0 ? QString::fromUtf8("自动分组") : QString("Auto Group"),
                lang == 0 ? QString::fromUtf8("否") : QString("No") },
            { lang == 0 ? QString::fromUtf8("字号") : QString("Font Size"), QString::number(lb.fontSize) },
            { lang == 0 ? QString::fromUtf8("显示编号") : QString("Show ID"), yn(lb.showId) },
            { lang == 0 ? QString::fromUtf8("显示名称") : QString("Show Name"), yn(lb.showName) },
            { lang == 0 ? QString::fromUtf8("幅值") : QString("Magnitude"), mag },
            { lang == 0 ? QString::fromUtf8("数值格式") : QString("Format"),
                lb.scientific ? (lang == 0 ? QString::fromUtf8("科学") : QString("Scientific"))
                              : (lang == 0 ? QString::fromUtf8("小数") : QString("Decimal")) },
            { lang == 0 ? QString::fromUtf8("数值精度") : QString("Precision"), QString::number(lb.precision) },
            { lang == 0 ? QString::fromUtf8("角度") : QString("Angle"),
                lb.radians ? (lang == 0 ? QString::fromUtf8("弧度") : QString("Radians"))
                           : (lang == 0 ? QString::fromUtf8("度数") : QString("Degrees")) },
            { lang == 0 ? QString::fromUtf8("填充") : QString("Fill"), yn(lb.fill) },
            { lang == 0 ? QString::fromUtf8("填充颜色") : QString("Fill Color"), lb.fillColor.name().toHtmlEscaped() },
            { lang == 0 ? QString::fromUtf8("自动隐藏") : QString("Auto Hide"), yn(lb.autoHide) },
            { lang == 0 ? QString::fromUtf8("显示前缀") : QString("Prefix"), yn(lb.prefix) }
        };

        QLabel* lbl = new QLabel;
        lbl->setTextFormat(Qt::RichText);
        lbl->setWordWrap(true);
        lbl->setStyleSheet("QLabel{color:#424B57;font-size:12px;border:0;background:#FFFFFF;padding:8px;}");
        //两列 dl 布局：原型 .measurement-detail-values 为 68px 术语列 + 值列网格
        //（gap 6/8、padding 8）——此前"术语：值"单列长条与原型不符
        QString rows;
        for (const auto& f : fields)
            rows += QString::fromUtf8(
                "<tr><td width=\"68\" style=\"padding:0 8px 6px 0;vertical-align:top;\">%1</td>"
                "<td style=\"padding:0 0 6px 0;vertical-align:top;\">%2</td></tr>").arg(f.first, f.second);
        lbl->setText(QString::fromUtf8("<html><body><table cellspacing=\"0\" cellpadding=\"0\">%1</table></body></html>").arg(rows));

        _rvTable->insertRow(row);
        _rvTable->setSpan(row, 0, 1, _rvTable->columnCount());
        _rvTable->setCellWidget(row, 0, lbl);
        _rvTable->setRowHeight(row, qMax(24, lbl->sizeHint().height()));
    }

    void mPostMeasureTool::toggleReviewDetail(int recordId)
    {
        const int di = dataIndexById(recordId);
        if (di < 0) return;
        int row = -1;
        for (int r = 0; r < _rvTable->rowCount(); ++r)
        {
            if (QTableWidgetItem* it = _rvTable->item(r, 0);
                it && it->data(Qt::UserRole).toInt() == recordId) { row = r; break; }
        }
        if (row < 0) return;

        if (_expandedReviewIds.contains(recordId))
        {
            _expandedReviewIds.remove(recordId);
            _rvTable->removeRow(row + 1);
        }
        else
        {
            _expandedReviewIds.insert(recordId);
            insertReviewDetailRow(row + 1, _records.at(di));
        }
        if (QWidget* w = _rvTable->cellWidget(row, 0))
            if (QToolButton* b = w->findChild<QToolButton*>())
                b->setText(_expandedReviewIds.contains(recordId)
                    ? QString::fromUtf8("▾") : QString::fromUtf8("▸"));
    }

    void mPostMeasureTool::renderReviewTable()
    {
        if (!_rvTable) return;

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        const QString key = _rvSearchEdit ? _rvSearchEdit->text().trimmed() : QString();
        int typeFilter = -1;
        if (_rvTypeCombo && _rvTypeCombo->currentData().isValid())
            typeFilter = _rvTypeCombo->currentData().toInt();

        _expandedReviewIds.clear();
        _rvTable->setSortingEnabled(false);	//填充期间关排序
        _rvTable->setRowCount(0);
        int visibleCount = 0;
        for (const MeasureRecord& rec : _records)
        {
            if (typeFilter >= 0 && rec.typeIndex != typeFilter) continue;
            //对齐原型：搜索匹配 名称/编号/类型
            const QString typeText = typeName(rec.typeIndex, lang);
            if (!key.isEmpty()
                && !rec.name.contains(key, Qt::CaseInsensitive)
                && !QString::number(rec.id).contains(key)
                && !typeText.contains(key, Qt::CaseInsensitive))
                continue;
            ++visibleCount;

            const int r = _rvTable->rowCount();
            _rvTable->insertRow(r);

            //实体格：▶ 展开钮 + 名称（对齐原型 measurement-review-entity）。表格项只承载
            //UserRole 供详情定位，不设 DisplayRole 文本——项文本与透明控件内的
            //名称 QLabel 同格双层绘制会叠成乱码（0.24 轮用户报告"实体列文本重叠"根因）
            QTableWidgetItem* nameIt = new QTableWidgetItem();
            nameIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            nameIt->setData(Qt::UserRole, rec.id);
            _rvTable->setItem(r, 0, nameIt);
            QWidget* ent = new QWidget;
            ent->setStyleSheet("QWidget{background:transparent;}");
            QHBoxLayout* entLayout = new QHBoxLayout(ent);
            entLayout->setContentsMargins(2, 0, 2, 0);
            entLayout->setSpacing(4);
            QToolButton* ex = new QToolButton(ent);
            ex->setText(QString::fromUtf8("▸"));
            ex->setFixedSize(18, 26);	//原型 .measurement-review-expand 18×button-height(26)
            ex->setAutoRaise(true);
            ex->setStyleSheet(QString("QToolButton{border:0;background:transparent;color:%1;font-size:12px;}QToolButton:hover{background:#F0F7FF;}").arg(kMuted));
            entLayout->addWidget(ex);
            QLabel* nm = new QLabel(rec.name, ent);
            nm->setStyleSheet(QString("QLabel{color:%1;border:0;background:transparent;}").arg(kText));
            entLayout->addWidget(nm);
            entLayout->addStretch();
            _rvTable->setCellWidget(r, 0, ent);
            const QString resultText = formatMeasureText(rec, lang);
            const QString detail = resultText + "\n" + recordLabelSummary(rec, lang);
            nm->setToolTip(resultText);
            connect(ex, &QToolButton::clicked, this, [this, rid = rec.id]() { toggleReviewDetail(rid); });

            QTableWidgetItem* idIt = new QTableWidgetItem();
            idIt->setData(Qt::DisplayRole, rec.id);	//int：数值排序
            idIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            idIt->setTextAlignment(Qt::AlignCenter);
            _rvTable->setItem(r, 1, idIt);

            QTableWidgetItem* colorIt = new QTableWidgetItem();
            colorIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            colorIt->setBackground(QBrush(rec.color));
            colorIt->setToolTip(rec.color.name().toUpper());	//对齐原型色值大写提示
            _rvTable->setItem(r, 2, colorIt);

            QTableWidgetItem* typeIt = new QTableWidgetItem(typeText);
            typeIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            typeIt->setToolTip(detail);
            _rvTable->setItem(r, 3, typeIt);

            //坐标系列：对齐原型渲染"未指定"（记录暂无坐标系概念），对象信息保留在 tooltip 与详情
            QTableWidgetItem* coordIt = new QTableWidgetItem(lang == 0 ? QString::fromUtf8("未指定") : QString("Unspecified"));
            coordIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            coordIt->setForeground(QBrush(QColor(kMuted)));
            coordIt->setToolTip(detail);
            _rvTable->setItem(r, 4, coordIt);
        }

        //空表不设占位文案：原型详情列表空态仅剩表头（tbody 清空，无"暂无测量数据"行，
        //0.19 轮逐控件对齐实测）
        _rvTable->setSortingEnabled(true);	//填充完成后允许表头点击排序
        syncReviewState();
    }

    void mPostMeasureTool::slot_changeLanguage(int language)
    {
        int lang = language;

        _manageFold->setTitle(lang == 0 ? QString::fromUtf8("测量数据列表") : QString("Measurement Data"));
        _searchEdit->setPlaceholderText(lang == 0 ? QString::fromUtf8("搜索测量名称") : QString("Search measurements"));

        //类型筛选（保持当前选择）
        int keepFilter = _typeFilterCombo->currentData().isValid() ? _typeFilterCombo->currentData().toInt() : -1;
        _typeFilterCombo->blockSignals(true);
        _typeFilterCombo->clear();
        _typeFilterCombo->addItem(lang == 0 ? QString::fromUtf8("全部类型") : QString("All Types"), -1);
        for (int i = 0; i < kMeasureDataCount; ++i)
            _typeFilterCombo->addItem(typeName(i, lang), i);
        int restoreFilter = _typeFilterCombo->findData(keepFilter);
        _typeFilterCombo->setCurrentIndex(restoreFilter >= 0 ? restoreFilter : 0);
        _typeFilterCombo->blockSignals(false);

        _table->setHorizontalHeaderItem(0, new QTableWidgetItem(QString()));
        _table->setHorizontalHeaderItem(1, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("名称") : QString("Name")));
        _table->setHorizontalHeaderItem(2, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("类型") : QString("Type")));
        _table->setHorizontalHeaderItem(3, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("显隐") : QString("Show")));
        _table->setHorizontalHeaderItem(4, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("颜色") : QString("Color")));
        _btnSelectAll->setText(lang == 0 ? QString::fromUtf8("全选") : QString("All"));
        _btnShow->setText(lang == 0 ? QString::fromUtf8("显示") : QString("Show"));
        _btnHide->setText(lang == 0 ? QString::fromUtf8("隐藏") : QString("Hide"));
        _btnDelete->setText(lang == 0 ? QString::fromUtf8("删除") : QString("Delete"));

        _toolFold->setTitle(lang == 0 ? QString::fromUtf8("测量工具") : QString("Measure Tools"));
        _labelContent->setText(lang == 0 ? QString::fromUtf8("标签内容") : QString("Label"));
        _labelBtnText->setText(lang == 0 ? QString::fromUtf8("自定义") : QString("Customize"));

        //测量数据下拉（保持当前选择）
        const int keepData = _comboBox->currentIndex();
        _comboBox->blockSignals(true);
        _comboBox->clear();
        for (int i = 0; i < kMeasureDataCount; ++i)
            _comboBox->addItem(typeName(i, lang));
        _comboBox->setCurrentIndex((keepData >= 0 && keepData < kMeasureDataCount) ? keepData : kDefaultMeasureData);
        _comboBox->blockSignals(false);

        _dataLabel->setText(lang == 0 ? QString::fromUtf8("测量数据") : QString("Data"));
        _pickLabel->setText(pickButtonLabel(false, lang));
        _btnPickNode->setText(pickButtonLabel(_btnPickNode->isChecked(), lang));

        _checkAutoCreate->setText(lang == 0 ? QString::fromUtf8("自动创建") : QString("Auto Create"));
        _checkAutoGroup->setText(lang == 0 ? QString::fromUtf8("自动分组") : QString("Auto Group"));
        _btnCreate->setText(lang == 0 ? QString::fromUtf8("创建") : QString("Create"));
        if (_btnNew)
            _btnNew->setText(lang == 0 ? QString::fromUtf8("新建") : QString("New"));
        if (_editor)
            _editor->retranslate(lang == 1);
        _btnManager->setText(lang == 0 ? QString::fromUtf8("测量数据管理及曲线生成") : QString("Manager & Curves"));

        //列表整行按数据重渲染（类型/显隐列文本随语言变化）
        _table->setSortingEnabled(false);
        for (int r = 0; r < _table->rowCount(); ++r)
            renderRow(r);
        _table->setSortingEnabled(true);
        _table->horizontalHeader()->setSortIndicatorShown(false);

        applyPopupsLanguage(lang);

        refreshEmptyState();
        syncManagerState();
        syncReviewState();
    }

    void mPostMeasureTool::applyPopupsLanguage(int lang)
    {
        if (_labelPopup)
        {
            _labelPopup->findChild<QLabel*>("mLabelPopupTitle")->setText(lang == 0 ? QString::fromUtf8("标签内容") : QString("Label"));
            //分组标题：可折叠 FoldSection 的标题条（重构后无 mLabelSection_* QLabel）
            if (auto* showSec = _labelPopup->findChild<FoldSection*>("mLabelShowSec"))
                showSec->setTitle(lang == 0 ? QString::fromUtf8("显示") : QString("Display"));
            if (auto* globalSec = _labelPopup->findChild<FoldSection*>("mLabelGlobalSec"))
                globalSec->setTitle(lang == 0 ? QString::fromUtf8("全局") : QString("Global"));
            _labelPopup->findChild<QLabel*>("mLabelField_" + QString::fromUtf8("字号"))->setText(lang == 0 ? QString::fromUtf8("字号") : QString("Font Size"));
            _labelPopup->findChild<QLabel*>("mLabelField_" + QString::fromUtf8("显示"))->setText(lang == 0 ? QString::fromUtf8("显示") : QString("Display"));
            _labelPopup->findChild<QLabel*>("mLabelField_" + QString::fromUtf8("幅值"))->setText(lang == 0 ? QString::fromUtf8("幅值") : QString("Magnitude"));
            _labelPopup->findChild<QLabel*>("mLabelField_" + QString::fromUtf8("数值"))->setText(lang == 0 ? QString::fromUtf8("数值") : QString("Number"));
            _labelPopup->findChild<QLabel*>("mLabelField_" + QString::fromUtf8("角度"))->setText(lang == 0 ? QString::fromUtf8("角度") : QString("Angle"));
            _lpColorSwatch->setToolTip(lang == 0 ? QString::fromUtf8("标签颜色") : QString("Label color"));
            _lpShowId->setText(lang == 0 ? QString::fromUtf8("编号") : QString("ID"));
            _lpShowName->setText(lang == 0 ? QString::fromUtf8("名称") : QString("Name"));

            _lpMagnitudeCombo->blockSignals(true);
            _lpMagnitudeCombo->clear();
            if (lang == 0)
                for (const QString& t : { QString::fromUtf8("无"), QString::fromUtf8("X+Y+Z"), QString::fromUtf8("X+Y"), QString::fromUtf8("X+Z"), QString::fromUtf8("Y+Z"), QString("X"), QString("Y"), QString("Z") })
                    _lpMagnitudeCombo->addItem(t);
            else
                for (const QString& t : { QString("None"), QString("X+Y+Z"), QString("X+Y"), QString("X+Z"), QString("Y+Z"), QString("X"), QString("Y"), QString("Z") })
                    _lpMagnitudeCombo->addItem(t);
            _lpMagnitudeCombo->blockSignals(false);

            _lpFormatCombo->blockSignals(true);
            _lpFormatCombo->clear();
            _lpFormatCombo->addItem(lang == 0 ? QString::fromUtf8("科学") : QString("Sci"));
            _lpFormatCombo->addItem(lang == 0 ? QString::fromUtf8("小数") : QString("Dec"));
            _lpFormatCombo->blockSignals(false);
            _lpPrecisionSpin->setToolTip(lang == 0 ? QString::fromUtf8("数值精度") : QString("Precision"));

            _lpDegreeRadio->setText(lang == 0 ? QString::fromUtf8("度数") : QString("Deg"));
            _lpRadianRadio->setText(lang == 0 ? QString::fromUtf8("弧度") : QString("Rad"));
            _lpFillCheck->setText(lang == 0 ? QString::fromUtf8("填充") : QString("Fill"));
            _lpFillSwatch->setToolTip(lang == 0 ? QString::fromUtf8("填充颜色") : QString("Fill color"));
            _lpAutoHideCheck->setText(lang == 0 ? QString::fromUtf8("自动隐藏") : QString("Auto Hide"));
            _lpPrefixCheck->setText(lang == 0 ? QString::fromUtf8("显示前缀") : QString("Prefix"));
            _lpResetBtn->setText(lang == 0 ? QString::fromUtf8("恢复默认") : QString("Reset"));
            _lpOkBtn->setText(lang == 0 ? QString::fromUtf8("确认") : QString("OK"));
        }

        if (_reviewPopup)
        {
            _reviewPopup->findChild<QLabel*>("mReviewTitle")->setText(lang == 0
                ? QString::fromUtf8("测量数据详细信息") : QString("Measurement Details"));
            _rvSearchEdit->setPlaceholderText(lang == 0 ? QString::fromUtf8("搜索测量名称、编号或类型") : QString("Search name, ID or type"));

            _rvLiveLink->setText(lang == 0 ? QString::fromUtf8("实时关联") : QString("Live Link"));
            _rvLiveLink->setToolTip(lang == 0 ? QString::fromUtf8("待接入") : QString("Pending"));

            int keepFilter = _rvTypeCombo->currentData().isValid() ? _rvTypeCombo->currentData().toInt() : -1;
            _rvTypeCombo->blockSignals(true);
            _rvTypeCombo->clear();
            _rvTypeCombo->addItem(lang == 0 ? QString::fromUtf8("全部类型") : QString("All Types"), -1);
            for (int i = 0; i < kMeasureDataCount; ++i)
                _rvTypeCombo->addItem(typeName(i, lang), i);
            int restore = _rvTypeCombo->findData(keepFilter);
            _rvTypeCombo->setCurrentIndex(restore >= 0 ? restore : 0);
            _rvTypeCombo->blockSignals(false);

            _rvTable->setHorizontalHeaderItem(0, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("实体") : QString("Entities")));
            _rvTable->setHorizontalHeaderItem(1, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("编号") : QString("ID")));
            _rvTable->setHorizontalHeaderItem(2, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("颜色") : QString("Color")));
            _rvTable->setHorizontalHeaderItem(3, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("类型") : QString("Type")));
            _rvTable->setHorizontalHeaderItem(4, new QTableWidgetItem(lang == 0 ? QString::fromUtf8("坐标系") : QString("Coord Sys")));

            _rvSingleCurve->setText(lang == 0 ? QString::fromUtf8("单条曲线") : QString("Single Curve"));
            _reviewPopup->findChild<QLabel*>("mReviewXLabel")->setText(lang == 0 ? QString::fromUtf8("X轴") : QString("X Axis"));
            QSignalBlocker xb(_rvXCombo);
            const int keepX = _rvXCombo->currentIndex();
            _rvXCombo->clear();
            _rvXCombo->addItem(lang == 0 ? QString::fromUtf8("时间") : QString("Time"));
            _rvXCombo->addItem(lang == 0 ? QString::fromUtf8("帧号") : QString("Frame"));
            _rvXCombo->setCurrentIndex(keepX >= 0 ? keepX : 0);
            //Y轴选项由 refreshYAxisOptions 按绘制目标记录适用性重建（本函数尾部
            //renderReviewTable → syncReviewState 会触发，语言文案随重建生成）
            _reviewPopup->findChild<QLabel*>("mReviewTargetLabel")->setText(lang == 0 ? QString::fromUtf8("绘制位置") : QString("Plot At"));
            QSignalBlocker tb(_rvTargetCombo);
            const int keepTarget = _rvTargetCombo->currentIndex();
            _rvTargetCombo->clear();
            //原型 measurementPlotTarget 仅"预览图"一项（绘制统一到右侧 PlotView）
            _rvTargetCombo->addItem(lang == 0 ? QString::fromUtf8("预览图") : QString("Preview"));
            _rvTargetCombo->setCurrentIndex(keepTarget >= 0 ? keepTarget : 0);
            _rvPlotBtn->setText(lang == 0 ? QString::fromUtf8("绘制") : QString("Plot"));

            //收起/展开绘图按钮文本随折叠状态刷新（用 isHidden：弹窗未上屏时
            //isVisible 恒为 false，首开会误设为"展开绘图"）
            if (_btnCollapsePlot)
                _btnCollapsePlot->setText(_plotSection && !_plotSection->isHidden()
                    ? (lang == 0 ? QString::fromUtf8("收起绘图") : QString("Collapse Plot"))
                    : (lang == 0 ? QString::fromUtf8("展开绘图") : QString("Expand Plot")));

            //列表文本随语言刷新（空表文案在 renderReviewTable 内按语言生成）
            renderReviewTable();
        }
    }

    void mPostMeasureTool::slot_dataTypeChanged(int index)
    {
        Q_UNUSED(index);
        //对齐原型：切换测量数据即取消拾取（setMeasurementPicking(false)）并清空已选
        _pickedIds.clear();
        exitPickMode();

        //待接入类型即时提示（创建时同样拦截）
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        if (currentCalcKind() < 0)
        {
            setStatusLabel(lang == 0
                ? QString::fromUtf8("「%1」待接入。").arg(_comboBox->currentText())
                : QString("\"%1\" pending integration.").arg(_comboBox->currentText()));
        }
        notifyMeasureEditChanged();	//V1.3：类型切换写编辑缓冲
    }

    void mPostMeasureTool::slot_pickNodeClicked(bool checked)
    {
        //以参数为准设置勾选态：真实点击由 clicked(bool) 传入 true/false，
        //程序化调用（ui-shot 等）可显式传 true 进入拾取态
        _btnPickNode->setChecked(checked);
        if (!_isCurrStateValid)
        {
            if (!checked) _btnPickNode->setChecked(false);
            return;
        }

        mxr::mBaseRend* _pickRend = nullptr;
        if (_isCurrPostViewport)
            _pickRend = _postRend;
        else
            _pickRend = _preRend;

        if (!_pickRend)
        {
            //无可用渲染器（视口已关闭等）：回弹勾选，避免按钮卡在拾取态
            _btnPickNode->setChecked(false);
            return;
        }

        if (checked)
        {
            //拾取域随当前类型联动：体积/面积拾取单元（后处理）/几何体、面（前处理），
            //其余拾取节点（后处理网格点/前处理几何点）。广播号对齐工具栏按钮组：
            //0智能/1几何点/2几何边/3几何面/4几何体/5网格点/6网格线/7网格面/8网格单元/9网格部件
            const int calc = currentCalcKind();
            mxr::PickFilter filter = mxr::PickFilter::PickNothing;
            int pickKind = 0;
            if (_isCurrPostViewport)
            {
                if (calc == 11 || calc == 12)
                {
                    filter = mxr::PickFilter::PickMeshElement;
                    pickKind = 8;
                }
                else
                {
                    filter = mxr::PickFilter::PickMeshNode;
                    pickKind = 5;
                }
            }
            else
            {
                if (calc == 11)
                {
                    filter = mxr::PickFilter::PickGeoSolid;
                    pickKind = 4;
                }
                else if (calc == 12)
                {
                    filter = mxr::PickFilter::PickGeoFace;
                    pickKind = 3;
                }
                else
                {
                    filter = mxr::PickFilter::PickGeoPoint;
                    pickKind = 1;
                }
            }
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(pickKind);
            _pickRend->setPickFilter(filter);

            //清空拾取高亮
            getCVM->clearPick();

            //对齐原型 setMeasurementPicking：armed 文案在按钮上，状态行清空
            int langArm = MAppConfiguration::mAppConfiguration::getInstance()->_language;
            _btnPickNode->setText(pickButtonLabel(true, langArm));
            setStatusLabel(QString());
        }
        else
        {
            _pickRend->setPickFilter(mxr::PickFilter::PickNothing);
            int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
            _btnPickNode->setText(pickButtonLabel(false, lang));
            setStatusLabel(QString());
        }
    }

    void mPostMeasureTool::setStatusLabel(const QString& text)
    {
        if (!_statusLabel)
            return;
        _statusLabel->setText(text);
        //行级摘/插并同步 FoldSection 高度帽：直接 setVisible 会留旧帽不更新，
        //表单在旧高度内挤压上方行距（0.20 轮"创建空态提示压缩行距"实证）
        _toolFold->setRowVisible(_statusLabel, !text.isEmpty());
    }

    void mPostMeasureTool::updatePickStatus()
    {
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        if (_pickedIds.isEmpty())
        {
            setStatusLabel(QString());	//对齐原型：armed 空拾取时状态行为空（文案在按钮上）
            return;
        }
        QStringList ids;
        for (int id : _pickedIds)
            ids.append(QString::number(id));
        const QString pickedList = ids.join(",");
        const int calc = currentCalcKind();
        if (calc < 0)
        {
            setStatusLabel(lang == 0
                ? QString::fromUtf8("已选节点：%1（「%2」待接入）").arg(pickedList, _comboBox->currentText())
                : QString("Picked: %1 (\"%2\" pending)").arg(pickedList, _comboBox->currentText()));
            return;
        }
        const int more = requiredNodeCount() - _pickedIds.size();
        //最小距离（P2 点对集口径）：首拾取为基准节点，其后为对比集合
        if (calc == 10)
        {
            if (_pickedIds.size() <= 1)
            {
                setStatusLabel(lang == 0
                    ? QString::fromUtf8("基准节点：%1（继续拾取对比节点）").arg(pickedList)
                    : QString("Base node: %1 (pick comparison nodes)").arg(pickedList));
            }
            else
            {
                QStringList setIds;
                for (int i = 1; i < _pickedIds.size(); ++i)
                    setIds << QString::number(_pickedIds.at(i));
                setStatusLabel(lang == 0
                    ? QString::fromUtf8("基准：%1，对比：%2（可点击“创建”）")
                          .arg(_pickedIds.first()).arg(setIds.join(","))
                    : QString("Base: %1, comparison: %2 (ready to create)")
                          .arg(_pickedIds.first()).arg(setIds.join(",")));
            }
            return;
        }
        //体积/面积拾取对象为单元，措辞按拾取域调整
        const QString objName = (currentCalcKind() == 11 || currentCalcKind() == 12)
            ? QString::fromUtf8("已选单元") : QString::fromUtf8("已选节点");
        if (more > 0)
            setStatusLabel(lang == 0
                ? QString::fromUtf8("%1：%2（还需 %3 个）").arg(objName).arg(pickedList).arg(more)
                : QString("%1: %2 (%3 more)").arg(objName).arg(pickedList).arg(more));
        else
            setStatusLabel(lang == 0
                ? QString::fromUtf8("%1：%2（可点击“创建”）").arg(objName).arg(pickedList)
                : QString("%1: %2 (ready to create)").arg(objName).arg(pickedList));
    }

    QString mPostMeasureTool::pickButtonLabel(bool armed, int lang) const
    {
        //拾取域随当前类型联动（体积/面积拾取单元/几何体/面，其余拾取节点）
        const int calc = currentCalcKind();
        const bool elemDomain = (calc == 11 || calc == 12);
        if (armed)
        {
            if (!elemDomain)
                return lang == 0 ? QString::fromUtf8("正在拾取节点…") : QString("Picking node...");
            if (_isCurrPostViewport)
                return lang == 0 ? QString::fromUtf8("正在拾取单元…") : QString("Picking elements...");
            return calc == 11
                ? (lang == 0 ? QString::fromUtf8("正在拾取几何体…") : QString("Picking solids..."))
                : (lang == 0 ? QString::fromUtf8("正在拾取几何面…") : QString("Picking faces..."));
        }
        if (!elemDomain)
            return lang == 0 ? QString::fromUtf8("选择节点") : QString("Pick Node");
        if (_isCurrPostViewport)
            return lang == 0 ? QString::fromUtf8("选择单元") : QString("Pick Element");
        return calc == 11
            ? (lang == 0 ? QString::fromUtf8("选择几何体") : QString("Pick Solid"))
            : (lang == 0 ? QString::fromUtf8("选择几何面") : QString("Pick Face"));
    }

    void mPostMeasureTool::slot_finishPick(mxr::FinishPickInfo info)
    {
        if (info.pick_operate == PickOperate::PreView)
            return;

        if (!_isCurrStateValid) return;

        QTimer::singleShot(0, this, [=]() {
            //在途信号防护：singleShot 期间面板可能已被隐藏/切换上下文（hideEvent 已断信号，
            //但仍可能收到断开前最后一次拾取），复检有效性避免写入失效上下文
            if (!_isCurrStateValid) return;

            const int calc = currentCalcKind();
            if (calc == 11 || calc == 12)
            {
                //体积/面积：拾取对象为单元/几何体/面，多选收集（全部用于计算）
                mxr::SelectionInfoType selType = mxr::SelectionInfoType::MeshElement;
                if (!_isCurrPostViewport)
                    selType = (calc == 11) ? mxr::SelectionInfoType::GeoSolid : mxr::SelectionInfoType::GeoFace;
                std::vector<int> idList;
                if (!getPickResultIDs(selType, idList)) return;
                if (idList.empty()) return;
                _pickedIds.clear();
                for (int id : idList) _pickedIds.append(id);
                updatePickStatus();
                maybeAutoCreate();
                return;
            }

            //节点类：拾取节点按序入列（两点距离 2 个 / 夹角、增量角度 3 个 / 路径、最小距离 ≥2 个）。
            //定数类型选满即止（超出会让 rec.ids 混入多余节点：计算虽只取前 N 个，
            //3D 标注连线与详情对象列表都会被污染）；路径/最小距离 ≥2 不限个数
            const int need = nodesForCalc(calc);
            const bool openEnded = (calc == 9 || calc == 10);
            if (!openEnded && _pickedIds.size() >= need)
            {
                int langCap = MAppConfiguration::mAppConfiguration::getInstance()->_language;
                setStatusLabel(langCap == 0
                    ? QString::fromUtf8("「%1」最多选择 %2 个节点。").arg(_comboBox->currentText()).arg(need)
                    : QString("\"%1\" accepts up to %2 nodes.").arg(_comboBox->currentText()).arg(need));
                return;
            }

            int nodeID = getPickResultID(_isCurrPostViewport ? mxr::SelectionInfoType::MeshNode : mxr::SelectionInfoType::GeoPoint);
            if (nodeID < 0) return;

            _pickedIds.append(nodeID);
            updatePickStatus();
            notifyMeasureEditChanged();	//V1.3：拾取写编辑缓冲

            //自动创建：勾选后拾取齐备即创建记录
            maybeAutoCreate();
            });
    }

    void mPostMeasureTool::slot_createButton()
    {
        if (!_isCurrStateValid) return;

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;

        MeasureRecord rec;
        if (!buildRecordFromInput(rec))
            return;

        rec.id = _nextId++;
        //记录色 = 标签内容的文本颜色（默认色）；后续在列表颜色列调整即整体生效
        //（3D 文字/连线/线框/列表/曲线同一颜色来源）
        rec.color = _labelSettings.color;
        rec.visible = true;
        rec.labels = _labelSettings;	//对齐原型：记录创建时快照当前标签内容设置
        const int seq = _typeCounters.value(rec.typeIndex) + 1;
        _typeCounters[rec.typeIndex] = seq;
        rec.name = QString("%1_%2").arg(typeName(rec.typeIndex, lang)).arg(seq);
        _records.append(rec);
        syncMeasureCache();

        //插入列表行（排序期间插入会错位，先关排序）
        _table->setSortingEnabled(false);
        const int r = _table->rowCount();
        _table->insertRow(r);

        QTableWidgetItem* checkIt = new QTableWidgetItem();
        checkIt->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        checkIt->setCheckState(Qt::Unchecked);
        checkIt->setTextAlignment(Qt::AlignCenter);
        _table->setItem(r, 0, checkIt);

        QTableWidgetItem* nameIt = new QTableWidgetItem();
        nameIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        nameIt->setData(Qt::UserRole, rec.id);
        _table->setItem(r, 1, nameIt);

        QTableWidgetItem* typeIt = new QTableWidgetItem();
        typeIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        _table->setItem(r, 2, typeIt);

        QTableWidgetItem* visIt = new QTableWidgetItem();
        visIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        _table->setItem(r, 3, visIt);

        QTableWidgetItem* colorIt = new QTableWidgetItem();
        colorIt->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        _table->setItem(r, 4, colorIt);

        renderRow(r);
        _table->setSortingEnabled(true);
        _table->horizontalHeader()->setSortIndicatorShown(false);

        //对齐原型：列表重渲时重新套用筛选（新记录不匹配当前搜索/类型时不显示）
        applyManagerFilter();

        //新行滚动可见并选中（排序/筛选后行号有变，按 item 反查可视行）
        const int visualRow = _table->row(nameIt);
        if (visualRow >= 0 && !_table->isRowHidden(visualRow))
        {
            _table->selectRow(visualRow);
            _table->scrollToItem(_table->item(visualRow, 1));
        }

        refreshEmptyState();
        syncManagerState();

        //先清空拾取收集并退出拾取态（会按拾取态清状态行），随后再写最新结果——
        //顺序不能反：创建路径下按钮仍是勾选态，先写结果会被 exitPickMode 立即擦掉
        _pickedIds.clear();
        exitPickMode();

        QString firstLine = formatMeasureText(rec, lang);
        firstLine.replace("\r\n", "\n");
        setStatusLabel(lang == 0 ? QString::fromUtf8("%1：%2").arg(rec.name, firstLine.section('\n', 0, 0))
            : QString("%1: %2").arg(rec.name, firstLine.section('\n', 0, 0)));

        updateMeasureAnnotations();	//新记录 → 3D 标注重建
    }

    // ============ 统一编辑契约（V1.3：新建即入表待定义，保存完成同一记录） ============

    mPostMeasureTool::MeasureRecord* mPostMeasureTool::editingRecord()
    {
        const int di = dataIndexById(_editingId);
        return di < 0 ? nullptr : &_records[di];
    }

    int mPostMeasureTool::editingRowIndex() const
    {
        if (_editingId < 0) return -1;
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            QTableWidgetItem* nameIt = _table->item(r, 1);
            if (nameIt && nameIt->data(Qt::UserRole).toInt() == _editingId)
                return r;
        }
        return -1;
    }

    QString mPostMeasureTool::editingSignature() const
    {
        //编辑缓冲=类型+拾取序（V1.3 draft：name/type/selections）
        QString sig = QString::number(_comboBox ? _comboBox->currentIndex() : -1);
        for (int id : _pickedIds)
            sig += QChar(1) + QString::number(id);
        return sig;
    }

    void mPostMeasureTool::notifyMeasureEditChanged()
    {
        if (!_loadingEdit && _editor)
            _editor->notifyEditChanged();
    }

    QTableWidgetItem* mPostMeasureTool::insertRecordRow(const mPostMeasureTool::MeasureRecord& rec)
    {
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
        return nameIt;
    }

    void mPostMeasureTool::beginCreateMeasure()
    {
        if (!_isCurrStateValid) return;
        const auto action = [this]()
        {
            //V1.3：新建立即登记唯一 ID+默认名先入表（待定义），选中并清空拾取
            int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
            MeasureRecord rec;
            rec.id = _nextId++;
            QStringList used;
            for (const MeasureRecord& r : _records) used.append(r.name);
            rec.name = postObjectUniqueName(lang == 0
                ? QString::fromUtf8("测量_%1").arg(rec.id) : QString("Measure_%1").arg(rec.id), used);
            rec.typeIndex = _comboBox ? qMax(0, _comboBox->currentIndex()) : 0;
            rec.color = _labelSettings.color;
            rec.labels = _labelSettings;
            rec.visible = false;
            _records.append(rec);
            _incompleteIds.insert(rec.id);

            QTableWidgetItem* nameIt = insertRecordRow(rec);
            _editingId = rec.id;
            _pickedIds.clear();
            exitPickMode();
            _searchEdit->clear();				//V1.3：新建清搜索与类型筛选
            _typeFilterCombo->setCurrentIndex(0);
            applyManagerFilter();
            _editor->beginEdit();
            setStatusLabel(lang == 0 ? QString::fromUtf8("待定义：请补充拾取对象后保存。")
                : QString("Pending: pick objects and save."));
            const int visualRow = _table->row(nameIt);
            if (visualRow >= 0)
                _table->scrollToItem(_table->item(visualRow, 1));
            refreshEmptyState();
            syncManagerState();
            syncMeasureCache();	//新建即登记 → 树"测量"分支同步出节点（原型 beginCreate 即入树）
            //新记录成为编辑对象 → 树高亮跟随
            MechCommon::mMechGlobalSignals::getInstance()->measureEditingChangedSig(_editingId);
        };
        _editor->checkSwitch(action);
    }

    void mPostMeasureTool::selectMeasureRecord(int id)
    {
        if (id == _editingId) return;
        const auto action = [this, id]()
        {
            const int di = dataIndexById(id);
            if (di < 0) return;
            _loadingEdit = true;
            _editingId = id;
            //类型切换槽会清拾取缓冲——先切类型再回填记录拾取序
            _comboBox->setCurrentIndex(_records.at(di).typeIndex);
            _pickedIds = _records.at(di).ids;
            _loadingEdit = false;
            exitPickMode();
            _editor->beginEdit();
            setStatusLabel(QString());
            const int row = editingRowIndex();
            if (row >= 0)
                _table->scrollToItem(_table->item(row, 1));
            //切换成功 → 树"测量"分支高亮跟随（被"继续编辑"拦截时 action 不执行，高亮不动）
            MechCommon::mMechGlobalSignals::getInstance()->measureEditingChangedSig(_editingId);
        };
        _editor->checkSwitch(action);
    }

    void mPostMeasureTool::cancelEditMeasure()
    {
        const int di = dataIndexById(_editingId);
        if (di < 0) return;
        _loadingEdit = true;
        _comboBox->setCurrentIndex(_records.at(di).typeIndex);
        _pickedIds = _records.at(di).ids;
        _loadingEdit = false;
        exitPickMode();
        _editor->markSaved();
        setStatusLabel(_incompleteIds.contains(_editingId)
            ? QString::fromUtf8("待定义：请补充拾取对象后保存。") : QString());
    }

    bool mPostMeasureTool::commitEditMeasure(bool immediate)
    {
        if (!_isCurrStateValid) return false;
        const int di = dataIndexById(_editingId);

        MeasureRecord rec;
        if (!buildRecordFromInput(rec))
            return false;	//校验失败：状态行文案已由 buildRecordFromInput 写入

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        if (di < 0)
        {
            //无编辑对象（兜底）：等同旧创建路径
            slot_createButton();
            _editor->markSaved();
            return true;
        }

        //完成当前记录（保留 id/名称/颜色/标签快照/显隐）
        MeasureRecord& target = _records[di];
        rec.id = target.id;
        rec.name = target.name;
        rec.color = target.color;
        rec.labels = target.labels;
        rec.visible = target.visible || _incompleteIds.contains(target.id);
        target = rec;
        _incompleteIds.remove(target.id);
        syncMeasureCache();

        _table->setSortingEnabled(false);
        const int row = editingRowIndex();
        if (row >= 0)
            renderRow(row);
        _table->setSortingEnabled(true);
        _table->horizontalHeader()->setSortIndicatorShown(false);

        if (!immediate)
            exitPickMode();
        QString firstLine = formatMeasureText(rec, lang);
        firstLine.replace("\r\n", "\n");
        setStatusLabel(lang == 0 ? QString::fromUtf8("%1：%2").arg(rec.name, firstLine.section('\n', 0, 0))
            : QString("%1: %2").arg(rec.name, firstLine.section('\n', 0, 0)));

        updateMeasureAnnotations();	//记录完成 → 3D 测量标注同步
        _editor->markSaved();
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // 树联动公开接口（树"测量"分支子节点 ↔ 测量记录；V1.3 原型测量树入口）
    // 编辑对象变化经全局总线 measureEditingChangedSig 广播（postMeasureRecordsChangedSig
    // 同款通道），树侧高亮跟随——"延迟切换后高亮与编辑对象一致"
    //////////////////////////////////////////////////////////////////////////

    bool mPostMeasureTool::ensureRecordLoaded(int id)
    {
        if (dataIndexById(id) >= 0) return true;
        //树侧入口兜底：面板可能在无记录的上下文打开过（newMeasureToolPanel 的重载
        //守卫要求镜像非空，首库无记录时不会自动重载），目标记录在全局管理器而镜像缺它
        if (auto* mgr = measureManager())
        {
            for (const auto& r : mgr->measureHistory())
            {
                if (r.id == id)
                {
                    loadMeasureCache();
                    return dataIndexById(id) >= 0;
                }
            }
        }
        return false;
    }

    bool mPostMeasureTool::editMeasureRecord(int id)
    {
        if (!ensureRecordLoaded(id)) return false;
        //树入口清搜索/类型筛选（原型：单击树节点重置面板筛选），保证目标行可见
        _searchEdit->clear();
        _typeFilterCombo->setCurrentIndex(0);
        applyManagerFilter();
        if (dataIndexById(id) < 0) return false;
        selectMeasureRecord(id);	//内建三选一确认；被"继续编辑"拦截时编辑对象不变
        return _editingId == id;
    }

    int mPostMeasureTool::currentEditingMeasureId() const
    {
        return _editingId;
    }

    void mPostMeasureTool::setMeasureRecordVisible(int id, bool visible)
    {
        if (!ensureRecordLoaded(id)) return;
        const int di = dataIndexById(id);
        if (di < 0 || _records.at(di).visible == visible) return;
        _records[di].visible = visible;
        syncMeasureCache();

        //按 UserRole 定位行渲染（排序/筛选态安全；行可能被筛选隐藏）
        _table->setSortingEnabled(false);
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            QTableWidgetItem* nameIt = _table->item(r, 1);
            if (nameIt && nameIt->data(Qt::UserRole).toInt() == id)
            {
                renderRow(r);
                break;
            }
        }
        _table->setSortingEnabled(true);
        _table->horizontalHeader()->setSortIndicatorShown(false);
        updateMeasureAnnotations();	//显隐 → 3D 测量标注同步
    }

    bool mPostMeasureTool::deleteMeasureRecord(int id)
    {
        if (!ensureRecordLoaded(id)) return false;
        const int di = dataIndexById(id);
        if (di < 0) return false;

        // 测量标注收编（C3）：删除单条=removeObject 台账对象（余条不动），并摘平行表
        if (auto postRender = (_postRend != nullptr ? _postRend->getPostRender() : nullptr))
        {
            const int oid = _recordObjectIds.take(id);
            if (oid >= 0)
                postRender->removeObject(oid);
        }
        else
        {
            _recordObjectIds.remove(id);
        }
        _records.removeAt(di);
        _incompleteIds.remove(id);
        _table->setSortingEnabled(false);
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            QTableWidgetItem* nameIt = _table->item(r, 1);
            if (nameIt && nameIt->data(Qt::UserRole).toInt() == id)
            {
                _table->removeRow(r);
                break;
            }
        }
        _table->setSortingEnabled(true);
        _table->horizontalHeader()->setSortIndicatorShown(false);

        //编辑记录被删则回落首条，删空退出编辑态（与勾选删除语义一致）
        if (id == _editingId)
        {
            _editingId = _records.isEmpty() ? -1 : _records.first().id;
            if (_editingId >= 0)
            {
                selectMeasureRecord(_editingId);
                _editor->beginEdit();
            }
            else
            {
                _editor->endEdit();
            }
            MechCommon::mMechGlobalSignals::getInstance()->measureEditingChangedSig(_editingId);
        }

        syncMeasureCache();
        refreshEmptyState();
        syncManagerState();
        updateMeasureAnnotations();	//删除 → 3D 标注同步
        return true;
    }

    void mPostMeasureTool::slot_searchChanged(const QString& text)
    {
        Q_UNUSED(text)
        //需求 V1.1 §2：筛选隐藏的已选记录保留选择（隐藏选中数在 syncManagerState 提示），
        //批量操作仍作用于全部已选行（含筛选隐藏行）
        applyManagerFilter();
        syncManagerState();
    }

    void mPostMeasureTool::slot_typeFilterChanged(int index)
    {
        Q_UNUSED(index)
        applyManagerFilter();
        syncManagerState();
    }

    void mPostMeasureTool::slot_selectAllClicked()
    {
        //存在未勾选则全勾，否则全不勾（仅可见行）
        bool allChecked = _table->rowCount() > 0;
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            if (_table->isRowHidden(r)) continue;
            if (_table->item(r, 0)->checkState() != Qt::Checked) { allChecked = false; break; }
        }
        const Qt::CheckState next = allChecked ? Qt::Unchecked : Qt::Checked;

        _table->blockSignals(true);
        for (int r = 0; r < _table->rowCount(); ++r)
        {
            if (_table->isRowHidden(r)) continue;
            _table->item(r, 0)->setCheckState(next);
            applyCheckedRowBackground(r);
        }
        _table->blockSignals(false);
        syncManagerState();
    }

    void mPostMeasureTool::slot_showSelectedClicked()
    {
        setCheckedRecordsVisible(true);
    }

    void mPostMeasureTool::slot_hideSelectedClicked()
    {
        setCheckedRecordsVisible(false);
    }

    void mPostMeasureTool::slot_deleteButton()
    {
        //移除勾选行
        QList<int> rows;
        for (int r = 0; r < _table->rowCount(); ++r)
            if (_table->item(r, 0)->checkState() == Qt::Checked)
                rows.append(r);

        if (rows.isEmpty()) return;
        std::sort(rows.begin(), rows.end(), std::greater<int>());
        bool editingRemoved = false;
        _table->setSortingEnabled(false);
        for (int r : rows)
        {
            const int id = _table->item(r, 1)->data(Qt::UserRole).toInt();
            if (id == _editingId)
                editingRemoved = true;
            int di = dataIndexById(id);
            if (di >= 0) _records.removeAt(di);
            // 测量标注收编（C3）：删除单条=removeObject 台账对象，并摘平行表
            if (auto postRender = (_postRend != nullptr ? _postRend->getPostRender() : nullptr))
            {
                const int oid = _recordObjectIds.take(id);
                if (oid >= 0)
                    postRender->removeObject(oid);
            }
            else
            {
                _recordObjectIds.remove(id);
            }
            _incompleteIds.remove(id);
            _table->removeRow(r);
        }
        _table->setSortingEnabled(true);
        _table->horizontalHeader()->setSortIndicatorShown(false);

        //V1.3：编辑记录被删则回落首条，删空退出编辑态
        if (editingRemoved)
        {
            _editingId = _records.isEmpty() ? -1 : _records.first().id;
            if (_editingId >= 0)
            {
                selectMeasureRecord(_editingId);
                _editor->beginEdit();
            }
            else
                _editor->endEdit();
            //回落/退出 → 树高亮跟随（selectMeasureRecord 早退不重发，此处单点广播）
            MechCommon::mMechGlobalSignals::getInstance()->measureEditingChangedSig(_editingId);
        }

        syncMeasureCache();
        refreshEmptyState();
        syncManagerState();
        updateMeasureAnnotations();	//删除 → 3D 标注同步
    }

    void mPostMeasureTool::slot_itemChanged(QTableWidgetItem* item)
    {
        if (!item || item->column() != 0) return;
        applyCheckedRowBackground(item->row());
        syncManagerState();
    }

    void mPostMeasureTool::slot_showColorPopup(int row)
    {
        ensureColorPopup();

        QTableWidgetItem* nameIt = _table->item(row, 1);
        if (!nameIt)
            return;
        _colorTarget = 0;
        _colorRecordId = nameIt->data(Qt::UserRole).toInt();
        const int di = dataIndexById(_colorRecordId);
        if (di < 0)
            return;
        _colorPicker->setCurrentColor(_records.at(di).color);

        //锚定颜色单元格下方
        QTableWidgetItem* colorIt = _table->item(row, 4);
        const QPoint pos = _table->visualItemRect(colorIt).bottomLeft()
            + _table->viewport()->mapToGlobal(QPoint(0, 0));
        _colorPicker->popupAt(pos);
    }

    void mPostMeasureTool::slot_labelButtonClicked()
    {
        ensureLabelPopup();
        if (_labelPopup->isVisible())
        {
            _labelPopup->close();
            return;
        }

        //对齐原型：打开弹窗即退出拾取态（measurementLabelOpen → setMeasurementPicking(false)）
        exitPickMode();

        //载入草稿（确认后才写回 _labelSettings）
        _labelDraft = _labelSettings;
        _lpFontCombo->setCurrentText(QString::number(_labelDraft.fontSize));
        applySwatchColor(_lpColorSwatch, _labelDraft.color);
        _lpShowId->setChecked(_labelDraft.showId);
        _lpShowName->setChecked(_labelDraft.showName);
        _lpMagnitudeCombo->setCurrentIndex(_labelDraft.magnitude);
        _lpFormatCombo->setCurrentIndex(_labelDraft.scientific ? 0 : 1);
        _lpPrecisionSpin->setValue(_labelDraft.precision);
        (_labelDraft.radians ? _lpRadianRadio : _lpDegreeRadio)->setChecked(true);
        _lpFillCheck->setChecked(_labelDraft.fill);
        applySwatchColor(_lpFillSwatch, _labelDraft.fillColor);
        _lpFillSwatch->setEnabled(_labelDraft.fill);
        _lpAutoHideCheck->setChecked(_labelDraft.autoHide);
        _lpPrefixCheck->setChecked(_labelDraft.prefix);

        popupUnder(_labelPopup, _btnLabel);
    }

    void mPostMeasureTool::slot_labelColorClicked()
    {
        ensureColorPopup();
        _colorTarget = 1;
        _colorRecordId = -1;
        _colorPicker->setCurrentColor(_labelDraft.color);
        popupUnder(_colorPicker, _lpColorSwatch);
    }

    void mPostMeasureTool::slot_fillColorClicked()
    {
        ensureColorPopup();
        _colorTarget = 2;
        _colorRecordId = -1;
        _colorPicker->setCurrentColor(_labelDraft.fillColor);
        popupUnder(_colorPicker, _lpFillSwatch);
    }

    void mPostMeasureTool::slot_labelReset()
    {
        //恢复默认参数（仅草稿，确认后生效）
        const LabelSettings def;
        _lpFontCombo->setCurrentText(QString::number(def.fontSize));
        applySwatchColor(_lpColorSwatch, def.color);
        _lpShowId->setChecked(def.showId);
        _lpShowName->setChecked(def.showName);
        _lpMagnitudeCombo->setCurrentIndex(def.magnitude);
        _lpFormatCombo->setCurrentIndex(def.scientific ? 0 : 1);
        _lpPrecisionSpin->setValue(def.precision);
        (def.radians ? _lpRadianRadio : _lpDegreeRadio)->setChecked(true);
        _lpFillCheck->setChecked(def.fill);
        applySwatchColor(_lpFillSwatch, def.fillColor);
        _lpFillSwatch->setEnabled(def.fill);
        _lpAutoHideCheck->setChecked(def.autoHide);
        _lpPrefixCheck->setChecked(def.prefix);
        _labelDraft = def;
    }

    void mPostMeasureTool::slot_labelConfirm()
    {
        _labelDraft.fontSize = _lpFontCombo->currentText().toInt();
        _labelDraft.showId = _lpShowId->isChecked();
        _labelDraft.showName = _lpShowName->isChecked();
        _labelDraft.magnitude = _lpMagnitudeCombo->currentIndex();
        _labelDraft.scientific = _lpFormatCombo->currentIndex() == 0;
        _labelDraft.precision = _lpPrecisionSpin->value();
        _labelDraft.radians = _lpRadianRadio->isChecked();
        _labelDraft.fill = _lpFillCheck->isChecked();
        _labelDraft.autoHide = _lpAutoHideCheck->isChecked();
        _labelDraft.prefix = _lpPrefixCheck->isChecked();
        _labelSettings = _labelDraft;
        //应用范围（暂定口径，待产品确认）：标签设置为全局样式——除作为新建记录
        //默认外，同步回刷全部已有记录；显示文本现场格式化，改设置即改显示。
        //颜色例外（用户裁定）：标签文本色只作新建默认，记录显示色一律用
        //rec.color（创建时取默认，列表颜色列调整后即固定，确认不覆盖）
        for (MeasureRecord& rec : _records)
            rec.labels = _labelSettings;
        syncMeasureCache();
        _labelPopup->close();

        //列表 tooltip/3D 标签文本-字号-颜色即时刷新（曲线在下次绘制时生效）
        _table->setSortingEnabled(false);
        for (int r = 0; r < _table->rowCount(); ++r)
            renderRow(r);
        _table->setSortingEnabled(true);
        _table->horizontalHeader()->setSortIndicatorShown(false);
        updateMeasureAnnotations();

        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        setStatusLabel(lang == 0 ? QString::fromUtf8("标签内容已更新。") : QString("Label settings updated."));
    }

    void mPostMeasureTool::slot_showReviewPopup()
    {
        ensureReviewPopup();
        if (_reviewPopup->isVisible())
        {
            _reviewPopup->close();
            return;
        }

        //对齐原型弹窗互斥模式：打开详情弹窗即退出拾取态
        exitPickMode();

        if (_rvSearchEdit)
            _rvSearchEdit->clear();
        if (_rvTypeCombo)
            _rvTypeCombo->setCurrentIndex(0);
        renderReviewTable();
        popupCenter(_reviewPopup);
    }

    void mPostMeasureTool::slot_reviewSearchChanged(const QString& text)
    {
        Q_UNUSED(text)
            renderReviewTable();
    }

    void mPostMeasureTool::slot_reviewTypeChanged(int index)
    {
        Q_UNUSED(index)
            renderReviewTable();
    }

    QList<int> mPostMeasureTool::currentPlotRecordIds() const
    {
        //绘制目标记录集合：单条曲线=当前选中行；多条曲线=全部"显示"状态的记录
        QList<int> ids;
        if (_rvSingleCurve && _rvSingleCurve->isChecked())
        {
            const auto rows = _rvTable->selectionModel()->selectedRows();
            if (rows.size() == 1)
            {
                QTableWidgetItem* nameIt = _rvTable->item(rows.at(0).row(), 0);
                if (nameIt)
                    ids.append(nameIt->data(Qt::UserRole).toInt());
            }
        }
        else
        {
            for (const MeasureRecord& rec : _records)
                if (rec.visible)
                    ids.append(rec.id);
        }
        return ids;
    }

    void mPostMeasureTool::slot_reviewAxisChanged(int index)
    {
        Q_UNUSED(index);
        //原型 D9：切换 X/Y 变量即按源数据重绘；从未绘制过时不主动出图
        if (_reviewPlotted)
            plotRecords(currentPlotRecordIds());
    }

    void mPostMeasureTool::slot_plotClicked()
    {
        //绘制位置仅"预览图"（原型 measurementPlotTarget 单选项）：统一画到弹窗右侧 PlotView。
        //单条曲线：仅绘制所选记录，未选中行时明确提示（此前静默返回，用户点绘制无任何反应）
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        if (_rvSingleCurve && _rvSingleCurve->isChecked()
            && _rvTable->selectionModel()->selectedRows().size() != 1)
        {
            setStatusLabel(lang == 0 ? QString::fromUtf8("请先在列表中选择一条测量记录。")
                : QString("Select one record in the list first."));
            return;
        }
        plotRecords(currentPlotRecordIds());
    }

}
