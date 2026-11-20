#include "mOpenCurveDlg.h"

#include <QFileDialog>
#include "mAppConfiguration.h"
#include "mDlgGeometry.h"
#include "mCurveDataSourceStore.h"
#include "mCurveImportCoordinator.h"
#include "mGlobalSignals.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveView/mCurveViewportFactory.h"
#include "CurveCore/CurveDocument.h"
#include "ThemeManager.h"   //自样式弹窗豁免标记（kSelfStyledProp）

#include <QUndoStack>
#include <exception>
#include <memory>
#include <QToolButton>
#include <QMainWindow>

namespace
{
    //列表项文本 -> 工况 id：文本为 "Subcase N" 或 "Subcase N (label内容)"，
    //只解析编号词元，末尾的 label 附加文字不参与解析
    int subcaseNumberOf(const QString& itemText)
    {
        const QString prefix = QStringLiteral("Subcase ");
        if (!itemText.startsWith(prefix))
            return -1;

        const QString rest = itemText.mid(prefix.length()).trimmed();
        const int sp = rest.indexOf(QLatin1Char(' '));
        return (sp < 0 ? rest : rest.left(sp)).toInt();
    }

    //栏头计数标签刷新为"已选中数/总行数"（前面的数随选中增长：选中两个
    //分量=2/X，多选列表实时增减；空列表显示空，与各清空路径的 setText("") 一致）
    void updateCountLabel(QLabel* label, QListWidget* list)
    {
        if (list->count() == 0)
            label->setText(QString());
        else
            label->setText(QString::number(list->selectedItems().size())
                + QLatin1Char('/') + QString::number(list->count()));
    }
}

namespace PostCurve
{
    namespace
    {
        //复数图分量合并（截图交互）：成对分量 MAG|X-Trans/PHA|X-Trans（或
        //REAL|/IMAG|）按方向合并显示为 X-Trans，去重保序；绘制时对每个方向
        //装配两侧数据各建一条曲线（复数图按命名约定配对）
        QStringList mergedComplexComponents(const QStringList& components)
        {
            QStringList out;
            for (const QString& c : components)
            {
                const int bar = c.indexOf('|');
                const QString dir = bar >= 0 ? c.mid(bar + 1) : c;
                if (!dir.isEmpty() && !out.contains(dir))
                    out.append(dir);
            }
            return out;
        }
    }

    mOpenCurveDlg* mOpenCurveDlg::m_instance = nullptr;

    mOpenCurveDlg* mOpenCurveDlg::getInstance(QWidget* parent)
    {
        if (!m_instance)
        {
            m_instance = new mOpenCurveDlg(parent);
        }
        return m_instance;
    }

    void mOpenCurveDlg::showInstance(QWidget* parent)
    {
        //父窗口须为真正的顶层主窗口：调用方传的 window() 在停靠/MDI 环境里可能
        //是承载弹窗请求的某个小面板（dock 内容、MDI 子窗，甚至切树瞬间暂离布局
        //的树控件自身），以它为父时弹窗会被"居中"到小面板上（左/上溢出屏幕），
        //层级也被卷进面板管理——先沿父链走到顶层；顶层仍不是 QMainWindow 时
        //兜底取应用主窗口（同 mDefineCurveDlg/mApplyStyleDlg 的取父策略）
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
        mOpenCurveDlg* dlg = getInstance(host);
        //本次运行首开居中于父窗口（主窗口）：无边框窗口无系统定位，缺省落点
        //随环境（部分环境半离屏），且标题栏拖动此前失效无法拉回——首开显式居中
        if (dlg->_needCenter)
        {
            dlg->_needCenter = false;
            if (QWidget* pw = dlg->parentWidget())
                dlg->move(pw->frameGeometry().center()
                    - QPoint(dlg->width() / 2, dlg->height() / 2));
        }
        if (dlg->isMinimized())
            dlg->showNormal();
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
        //隐藏期间目标图表类型可能已切换（复数图合并状态变化）：显示时重算
        //分量列表的合并/展开状态
        dlg->refreshYComponents();
        //首开整窗强制重绘：首次显示的布局/QSS 打磨时序曾在列头区留下白色残影
        //（部分区域未被表头背景覆盖），一次全量 update 清掉陈旧区域
        dlg->update();
    }

    mOpenCurveDlg::mOpenCurveDlg(QWidget* parent)
        //从文件创建曲线与曲线页面并行操作：置顶避免点击页面后被原生视口覆盖
        : QDialog(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        //自样式弹窗：豁免 ThemeManager 整窗重发，保留自带设计稿 QSS
        setProperty(MechMainWindow::ThemeManager::kSelfStyledProp, true);
        resize(960, 748);
        DlgGeom::bind(this, "OpenCurveDlg");   //记住大小/位置：恢复上次值，隐藏时自动保存
        //本次运行无已存几何时打开居中于父窗口（有几何则 bind 已恢复位置，不再
        //居中；几何只记当次运行，重启软件后回到默认尺寸+首开居中）
        _needCenter = !DlgGeom::hasSessionGeometry("OpenCurveDlg");
        setAutoFillBackground(true);
        //窗口细边框：白底弹窗落在浅色界面上轮廓不可辨识，加描边保证窗口边界
        //可见（同 mDefineCurveDlg/mApplyStyleDlg；须用 objectName 选择器——本类
        //在 PostCurve 命名空间内，QSS 裸类名选择器按 metaObject className 匹配不上）
        setObjectName("openCurveDlg");
        setAttribute(Qt::WA_StyledBackground, true);

        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动，事件过滤在 eventFilter）
        QWidget* titleBar = new QWidget(this);
        titleBar->setObjectName("OpenCurveTitleBar");
        titleBar->setFixedHeight(52);
        titleBar->setStyleSheet(
            "QWidget#OpenCurveTitleBar{background:#FFFFFF;border: none;}");
        QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(8);
        QLabel* logoLabel = new QLabel(titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("从文件创建曲线"), titleBar);
        titleLabel->setStyleSheet(
            "QLabel{color:#333333; font-size:14px; font-weight:600; background:transparent;}");
        QToolButton* closeBtn1 = new QToolButton(titleBar);
        closeBtn1->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
        closeBtn1->setIconSize(QSize(24, 24));
        closeBtn1->setToolTip(QStringLiteral("关闭"));
        closeBtn1->setFixedSize(20, 20);
        closeBtn1->setStyleSheet(R"(
            QToolButton{
                background:transparent; border:none;
            }
            QToolButton:hover{ background:#EFF1F2; border-radius:4px; }
        )");
        connect(closeBtn1, &QToolButton::clicked, this, &QWidget::close);
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn1);
        _titleBar = titleBar;
        _titleBar->installEventFilter(this);
        outer->addWidget(titleBar);


        // 设置样式
        setStyleSheet(R"(
            QWidget#openCurveDlg{border:1px solid #D9DCE0;}
            QDialog{
                background-color:#FFFFFF;
            }
            QStackedWidget{
                background-color:#FFFFFF;
            }
            QComboBox{
                background-color:#FFFFFF;
            }
            QTabBar::tab {
                background: transparent;
                border: none;
                border-bottom: 2px solid transparent;
                padding: 8px 15px;
                margin-right: 2px;
                color: black;
            }
            QTabBar::tab:selected {
                color: #1385F6;
                border-bottom: 2px solid #1385F6;
            }
            #srcTabRow{
                border-bottom: 1px solid #D9DCE0;
            }
            #myContainer{
                background-color:#F0F3F6;
                border:1px solid #D9DCE0;
                border-top-left-radius:7px;
                border-top-right-radius:7px;
                border-bottom-left-radius:0px;
                border-bottom-right-radius:0px;
            }
            /*表头标签禁画背景：个别环境下 QLabel 会在灰底上绘制白色底色块
            *（视觉如文字后的白色阴影），显式透明保证只显示文字*/
            #myContainer QLabel{
                background:transparent;
            }
            #Container{
                background-color:#FFFFFF;
                border:1px solid #D9DCE0;
            }
            QLineEdit{
                background-color:#FFFFFF;
            }
            #Container QListWidget{
                background-color:#FFFFFF;
                border:none;
            }
            QPushButton#btnCreate{
                background-color:#1385F6;
                color:#FFFFFF;
                border:none;
                border-radius:4px;
                padding:0px;
            }
            QPushButton#btnCreate:hover{
                background-color:#3D9EF8;
            }
            QPushButton#btnCreate:pressed{
                background-color:#0F6ACB;
            }
            QPushButton#btnCreate:disabled{
                background-color:#9CC3E5;
                color:#FFFFFF;
            }
        )");


        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        // 顶部第一行：文件选择
        QHBoxLayout* fileRowLayout = new QHBoxLayout();
        fileRowLayout->setSpacing(12);

        QLabel* fileLabel = new QLabel("选择数据文件");
        fileLabel->setFixedWidth(80);
        fileRowLayout->addWidget(fileLabel);

        m_fileCombo = new QComboBox();
        fileRowLayout->addWidget(m_fileCombo);

        QPushButton* browseBtn = new QPushButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/打开文件.svg")), QString());
        browseBtn->setToolTip(QStringLiteral("打开数据文件（pch/csv/txt）"));
        browseBtn->setAutoDefault(false); // 阻止回车键触发按钮
        browseBtn->setFixedSize(24, 24);   //图标按钮统一规格（同"应用样式"树工具钮）
        fileRowLayout->addWidget(browseBtn);
        mainLayout->addLayout(fileRowLayout);

        //行间距精确控制（默认样式间距不参与）：文件行-页签行 16px，
        //页签行-内容区/内容区-按钮行沿用各处显式 addSpacing 的 15px
        mainLayout->setSpacing(0);
        mainLayout->addSpacing(16);

        // 顶部第二行：X/Y 数据源页签 + 绘图选项。
        //容器整行画 1px 浅灰基线（#srcTabRow），选中页签的 2px 蓝色下划线
        //叠在其上（与设计图一致）；documentMode 关掉 QTabBar 原生基线/页签框，
        //下划线完全由样式表绘制
        QHBoxLayout* xyRowLayout = new QHBoxLayout();
        xyRowLayout->setContentsMargins(0, 0, 0, 0);
        QTabBar* tabBar = new QTabBar();
        tabBar->setExpanding(false);
        tabBar->setDocumentMode(true);
        tabBar->addTab("X轴数据源");
        tabBar->addTab("Y轴数据源");
        tabBar->setCurrentIndex(0);

        xyRowLayout->addWidget(tabBar, 0, Qt::AlignBottom);
        xyRowLayout->addStretch(0);
        QPushButton* plotOptionsBtn = new QPushButton("绘图选项");
        plotOptionsBtn->setAutoDefault(false); // 阻止回车键触发按钮
        plotOptionsBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/NVH/设置.svg")));
        plotOptionsBtn->setIconSize(QSize(16, 16));
        plotOptionsBtn->setFixedSize(84, 24);
        xyRowLayout->addWidget(plotOptionsBtn, 0, Qt::AlignRight);

        QWidget* tabRow = new QWidget();
        tabRow->setObjectName("srcTabRow");
        //普通 QWidget 须开启此属性，样式表的 border/background 才会绘制
        tabRow->setAttribute(Qt::WA_StyledBackground);
        tabRow->setLayout(xyRowLayout);
        mainLayout->addWidget(tabRow);
        mainLayout->addSpacing(15);

        // 内容区
        QStackedWidget* stackedWidget = new QStackedWidget();
        mainLayout->addWidget(stackedWidget);

        //  X轴页面：结果类型 / 对象 / 分量 三列联动（结果类型 = Y轴结果类型 +
        //  文件解析出的横坐标类型 Time[Sec]/Frequency[Hz]；对象/分量联动规则与Y轴相同）
        QWidget* xWidget = new QWidget();
        QHBoxLayout* listXLayout = new QHBoxLayout(xWidget);
        listXLayout->setContentsMargins(0, 0, 0, 0);
        listXLayout->setSpacing(16);                     //结果类型/对象/分量列间隔 16px

        // 列1：结果类型
        QFrame* xframe1 = new QFrame();
        xframe1->setObjectName("myContainer");
        QHBoxLayout* x_list1 = new QHBoxLayout(xframe1);
        QLabel* xLabel1 = new QLabel("结果类型");
        xLabel_1 = new QLabel();
        x_list1->addWidget(xLabel1);
        x_list1->addWidget(xLabel_1, 0, Qt::AlignRight);
        QFrame* xframe1_1 = new QFrame();
        xframe1_1->setObjectName("Container");
        QVBoxLayout* x_list1_1 = new QVBoxLayout(xframe1_1);
        QLineEdit* xSearchEdit = new QLineEdit();
        xSearchEdit->setPlaceholderText("搜索...");
        m_xResultList = new QListWidget();
        x_list1_1->addWidget(xSearchEdit);
        x_list1_1->addWidget(m_xResultList);

        QVBoxLayout* xCol1Layout = new QVBoxLayout();
        xCol1Layout->setContentsMargins(0, 0, 0, 0);
        xCol1Layout->setSpacing(0);
        xCol1Layout->addWidget(xframe1);
        xCol1Layout->addWidget(xframe1_1);

        listXLayout->addLayout(xCol1Layout);

        // 列2+列3 容器：对象/分量两栏整体显隐——横坐标类型（文件解析的
        // Time[Sec]/Frequency[Hz]）无对象/分量，选中时隐藏；实际结果类型时显示
        m_xObjCompPanel = new QWidget();
        QHBoxLayout* xObjCompLayout = new QHBoxLayout(m_xObjCompPanel);
        xObjCompLayout->setContentsMargins(0, 0, 0, 0);
        xObjCompLayout->setSpacing(16);                  //对象/分量列间隔 16px

        // 列2：对象
        QFrame* xframe2 = new QFrame();
        xframe2->setObjectName("myContainer");
        QHBoxLayout* x_list2 = new QHBoxLayout(xframe2);
        QLabel* xLabel2 = new QLabel("对象");
        xLabel_2 = new QLabel();
        x_list2->addWidget(xLabel2);
        x_list2->addWidget(xLabel_2, 0, Qt::AlignRight);
        QFrame* xframe2_1 = new QFrame();
        xframe2_1->setObjectName("Container");
        QVBoxLayout* x_list2_1 = new QVBoxLayout(xframe2_1);
        QLineEdit* xSearch2 = new QLineEdit();
        xSearch2->setPlaceholderText("搜索...");
        m_xObjectList = new KeepSelectionListWidget();
        x_list2_1->addWidget(xSearch2);
        x_list2_1->addWidget(m_xObjectList);

        QVBoxLayout* xCol2Layout = new QVBoxLayout();
        xCol2Layout->setContentsMargins(0, 0, 0, 0);
        xCol2Layout->setSpacing(0);
        xCol2Layout->addWidget(xframe2);
        xCol2Layout->addWidget(xframe2_1);

        xObjCompLayout->addLayout(xCol2Layout);

        // 列3：分量
        QFrame* xframe3 = new QFrame();
        xframe3->setObjectName("myContainer");
        QHBoxLayout* x_list3 = new QHBoxLayout(xframe3);
        QLabel* xLabel3 = new QLabel("分量");
        xLabel_3 = new QLabel();
        x_list3->addWidget(xLabel3);
        x_list3->addWidget(xLabel_3, 0, Qt::AlignRight);
        QFrame* xframe3_1 = new QFrame();
        xframe3_1->setObjectName("Container");
        QVBoxLayout* x_list3_1 = new QVBoxLayout(xframe3_1);
        QLineEdit* xSearch3 = new QLineEdit();
        xSearch3->setPlaceholderText("搜索...");
        m_xComponentList = new KeepSelectionListWidget();
        x_list3_1->addWidget(xSearch3);
        x_list3_1->addWidget(m_xComponentList);

        QVBoxLayout* xCol3Layout = new QVBoxLayout();
        xCol3Layout->setContentsMargins(0, 0, 0, 0);
        xCol3Layout->setSpacing(0);
        xCol3Layout->addWidget(xframe3);
        xCol3Layout->addWidget(xframe3_1);

        xObjCompLayout->addLayout(xCol3Layout);

        xObjCompLayout->setStretch(0, 1);
        xObjCompLayout->setStretch(1, 1);
        m_xObjCompPanel->hide();   //初始隐藏，选定实际结果类型后显示

        listXLayout->addWidget(m_xObjCompPanel);

        listXLayout->setStretch(0, 1);
        listXLayout->setStretch(1, 2);   //容器占 2 份 -> 内部两栏各 1 份（与结果类型列等宽）
        stackedWidget->addWidget(xWidget);

        //  Y轴页面 
        QWidget* yWidget = new QWidget();
        QVBoxLayout* yMainLayout = new QVBoxLayout(yWidget);
        yMainLayout->setContentsMargins(0, 0, 0, 0);

        // Y轴特有的布局下拉框
        QHBoxLayout* selectLayout = new QHBoxLayout();
        QLabel* layoutLabel = new QLabel("布局");
        layoutLabel->setFixedWidth(72);
        selectLayout->addWidget(layoutLabel);
        m_layoutCombo = new QComboBox();
        m_layoutCombo->addItem("使用当前绘图");
        m_layoutCombo->addItem("一个对象生成一个图表");
        m_layoutCombo->addItem("一个分量生成一个图表");
        m_layoutCombo->addItem("一条曲线生成一个图表");
        m_layoutCombo->setFixedWidth(320);

        QListView* listView = qobject_cast<QListView*>(m_layoutCombo->view());
        if (listView) {
            // 开启统一尺寸，提高性能
            listView->setUniformItemSizes(true);
            listView->setGridSize(QSize(20, 20));
        }

        selectLayout->addWidget(m_layoutCombo);

        m_selectBtn = new MechMainWindow::mImageGridComboBox();
        m_selectBtn->setFixedSize(24, 24);
        m_selectBtn->setIconSize(QSize(18, 18));

        //视口布局图标与页面属性"页面布局"共用一份路径表（PostCurveGlobal.h）
        m_selectBtn->setImages(curveViewportIconPaths(), 4);
        m_selectBtn->setCurrentImage(":/mMainWidgets/Icon/viewport/1.svg"); // 设置默认选中

        selectLayout->addWidget(m_selectBtn);

        //"使用当前绘图"复用当前视口，不产生新视口排布，后方视口布局网格
        //置灰禁选；其余布局选项恢复可选（初始默认"使用当前绘图"，初始即禁用）
        connect(m_layoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) { m_selectBtn->setEnabled(index != 0); });
        m_selectBtn->setEnabled(m_layoutCombo->currentIndex() != 0);


        selectLayout->addStretch();
        yMainLayout->addLayout(selectLayout);
        yMainLayout->setSpacing(16);   //布局行与下方列表区间距 16px

        // Y轴的四个列表
        QHBoxLayout* listYLayout = new QHBoxLayout();
        listYLayout->setSpacing(16);                     //工况/结果类型/对象/分量列间隔 16px

        // 列1：工况
        QFrame* yframe1 = new QFrame();
        yframe1->setObjectName("myContainer");
        QHBoxLayout* y_list1 = new QHBoxLayout(yframe1);
        QLabel* yLabel1 = new QLabel("工况");
        yLabel_1 = new QLabel();
        y_list1->addWidget(yLabel1);
        y_list1->addWidget(yLabel_1, 0, Qt::AlignRight);
        QFrame* yframe1_1 = new QFrame();
        yframe1_1->setObjectName("Container");
        QVBoxLayout* y_list1_1 = new QVBoxLayout(yframe1_1);
        QLineEdit* ySearch1 = new QLineEdit();
        ySearch1->setPlaceholderText("搜索...");
        m_ySubcaseList = new QListWidget();
        y_list1_1->addWidget(ySearch1);
        y_list1_1->addWidget(m_ySubcaseList);
        //工况列包整列容器（_ySubcaseCol）：R22 起 ascii 文件（csv/txt）无工况
        //层级需整列隐藏——隐藏列内控件会残留布局边距/占位，包一层容器整体
        //显隐才干净；pch 路径保持显示。
        //边距必须显式归零：col1 原经 addLayout 嵌套为子布局（Qt 对子布局默认
        //内容边距 0），setLayout 后成为容器 widget 的顶层布局，会改用样式默认
        //边距（约 9px）——不归零则工况列（含 pch 模式）整体内缩，pch 几何漂移
        QVBoxLayout* col1 = new QVBoxLayout();
        col1->setContentsMargins(0, 0, 0, 0);
        col1->setSpacing(0);
        col1->addWidget(yframe1);
        col1->addWidget(yframe1_1);
        QWidget* subcaseCol = new QWidget();
        subcaseCol->setLayout(col1);
        listYLayout->addWidget(subcaseCol);
        _ySubcaseCol = subcaseCol;

        // 列2：结果类型
        QFrame* yframe2 = new QFrame();
        yframe2->setObjectName("myContainer");
        QHBoxLayout* y_list2 = new QHBoxLayout(yframe2);
        QLabel* yLabel2 = new QLabel("结果类型");
        yLabel_2 = new QLabel();
        y_list2->addWidget(yLabel2);
        y_list2->addWidget(yLabel_2, 0, Qt::AlignRight);
        QFrame* yframe2_1 = new QFrame();
        yframe2_1->setObjectName("Container");
        QVBoxLayout* y_list2_1 = new QVBoxLayout(yframe2_1);
        QLineEdit* ySearch2 = new QLineEdit();
        ySearch2->setPlaceholderText("搜索...");
        m_yResultList = new QListWidget();
        y_list2_1->addWidget(ySearch2);
        y_list2_1->addWidget(m_yResultList);
        QVBoxLayout* col2 = new QVBoxLayout();
        col2->setSpacing(0);
        col2->addWidget(yframe2);
        col2->addWidget(yframe2_1);
        listYLayout->addLayout(col2);

        // 列3：对象
        QFrame* yframe3 = new QFrame();
        yframe3->setObjectName("myContainer");
        QHBoxLayout* y_list3 = new QHBoxLayout(yframe3);
        QLabel* yLabel3 = new QLabel("对象");
        yLabel_3 = new QLabel();
        y_list3->addWidget(yLabel3);
        y_list3->addWidget(yLabel_3, 0, Qt::AlignRight);
        QFrame* yframe3_1 = new QFrame();
        yframe3_1->setObjectName("Container");
        QVBoxLayout* y_list3_1 = new QVBoxLayout(yframe3_1);
        QLineEdit* ySearch3 = new QLineEdit();
        ySearch3->setPlaceholderText("搜索...");
        m_yObjectList = new KeepSelectionListWidget();
        m_yObjectList->setSelectionMode(QAbstractItemView::ExtendedSelection);


        y_list3_1->addWidget(ySearch3);
        y_list3_1->addWidget(m_yObjectList);
        QVBoxLayout* col3 = new QVBoxLayout();
        col3->setSpacing(0);
        col3->addWidget(yframe3);
        col3->addWidget(yframe3_1);
        listYLayout->addLayout(col3);

        // 列4：分量
        QFrame* yframe4 = new QFrame();
        yframe4->setObjectName("myContainer");
        QHBoxLayout* y_list4 = new QHBoxLayout(yframe4);
        QLabel* yLabel4 = new QLabel("分量");
        yLabel_4 = new QLabel();
        y_list4->addWidget(yLabel4);
        y_list4->addWidget(yLabel_4, 0, Qt::AlignRight);
        QFrame* yframe4_1 = new QFrame();
        yframe4_1->setObjectName("Container");
        QVBoxLayout* y_list4_1 = new QVBoxLayout(yframe4_1);
        QLineEdit* ySearch4 = new QLineEdit();
        ySearch4->setPlaceholderText("搜索...");
        m_yComponentList = new KeepSelectionListWidget();
        m_yComponentList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        y_list4_1->addWidget(ySearch4);
        y_list4_1->addWidget(m_yComponentList);

        QVBoxLayout* col4 = new QVBoxLayout();
        col4->setSpacing(0);
        col4->addWidget(yframe4);
        col4->addWidget(yframe4_1);

        listYLayout->addLayout(col4);

        listYLayout->setStretch(0, 1);
        listYLayout->setStretch(1, 1);
        listYLayout->setStretch(2, 1);
        listYLayout->setStretch(3, 1);
        yMainLayout->addLayout(listYLayout);

        stackedWidget->addWidget(yWidget);

        // 底部按钮区
        mainLayout->addSpacing(27);
        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(12);   //关闭/绘图按钮间距 12px
        //底距 11px + 主布局样式默认底距 9px = 按钮距窗口底边 20px（对齐 mApplyStyleDlg）
        btnLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->addLayout(btnLayout);
        btnLayout->addStretch();
        QPushButton* closeBtn = new QPushButton("关闭");
        m_createBtn = new QPushButton("绘图");
        m_createBtn->setObjectName("btnCreate");
        closeBtn->setFixedSize(44, 26);
        m_createBtn->setFixedSize(44, 26);
        btnLayout->addWidget(closeBtn);
        btnLayout->addWidget(m_createBtn);

        outer->addLayout(mainLayout);
        drawOption = new mDrawingOptions();
        drawOption->hide();

        // 信号槽连接
        connect(tabBar, &QTabBar::currentChanged, stackedWidget, &QStackedWidget::setCurrentIndex);
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
        connect(plotOptionsBtn, &QPushButton::clicked, this, &mOpenCurveDlg::slot_drawOptions);
        connect(m_createBtn, &QPushButton::clicked, this, &mOpenCurveDlg::slot_createLine);
        connect(browseBtn, &QPushButton::clicked, this, &mOpenCurveDlg::slot_openFile);
        // X轴三个搜索框
        connect(xSearchEdit, &QLineEdit::textChanged, this, [=](const QString& text) {filterList(m_xResultList, text); });
        connect(xSearch2, &QLineEdit::textChanged, this, [=](const QString& text) {filterList(m_xObjectList, text); });
        connect(xSearch3, &QLineEdit::textChanged, this, [=](const QString& text) {filterList(m_xComponentList, text); });
        // X轴联动：结果类型 -> 对象 -> 分量（规则同Y轴）
        connect(m_xResultList, &QListWidget::itemSelectionChanged, this, &mOpenCurveDlg::fillXObjects);
        connect(m_xObjectList, &QListWidget::itemSelectionChanged, this, &mOpenCurveDlg::fillXComponents);
        // Y轴四个搜索框 
        connect(ySearch1, &QLineEdit::textChanged, this, [=](const QString& text) { filterList(m_ySubcaseList, text); });
        connect(ySearch2, &QLineEdit::textChanged, this, [=](const QString& text) {filterList(m_yResultList, text); });
        connect(ySearch3, &QLineEdit::textChanged, this, [=](const QString& text) {filterList(m_yObjectList, text); });
        connect(ySearch4, &QLineEdit::textChanged, this, [=](const QString& text) {filterList(m_yComponentList, text); });

        //栏头"已选/总数"计数随选中实时刷新（X 结果类型与 Y 侧四列；多选的
        //对象/分量列随 Ctrl/Shift 多选增减）
        connect(m_ySubcaseList, &QListWidget::itemSelectionChanged, this, [this]() {
            updateCountLabel(yLabel_1, m_ySubcaseList); });
        connect(m_yResultList, &QListWidget::itemSelectionChanged, this, [this]() {
            updateCountLabel(yLabel_2, m_yResultList); });
        connect(m_yObjectList, &QListWidget::itemSelectionChanged, this, [this]() {
            updateCountLabel(yLabel_3, m_yObjectList); });
        connect(m_yComponentList, &QListWidget::itemSelectionChanged, this, [this]() {
            updateCountLabel(yLabel_4, m_yComponentList); });
        connect(m_xResultList, &QListWidget::itemSelectionChanged, this, [this]() {
            updateCountLabel(xLabel_1, m_xResultList); });
        connect(m_xObjectList, &QListWidget::itemSelectionChanged, this, [this]() {
            updateCountLabel(xLabel_2, m_xObjectList); });
        connect(m_xComponentList, &QListWidget::itemSelectionChanged, this, [this]() {
            updateCountLabel(xLabel_3, m_xComponentList); });

        //绘图按钮可用性随 X/Y 各列选中状态实时刷新（清空路径信号被阻塞，
        //由 clearAllList 末尾显式刷新）
        const QList<QListWidget*> selLists = { m_xResultList, m_xObjectList,
            m_xComponentList, m_ySubcaseList, m_yResultList,
            m_yObjectList, m_yComponentList };
        for (QListWidget* list : selLists)
            connect(list, &QListWidget::itemSelectionChanged,
                this, &mOpenCurveDlg::updateCreateButtonState);

        connect(m_fileCombo, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this, [=](QString path) {
            _currentlyFilePath = path;
            clearAllList();
            setListWig();
            });

        connect(m_ySubcaseList, &QListWidget::itemSelectionChanged, this, [=]() {
            m_yResultList->clear();
            m_yObjectList->clear();

            int num = subcaseNumberOf(m_ySubcaseList->currentItem()->text());
            int size = currentIndexData().yresulttype[num].size();
            if (size != 0)
            {
                yLabel_2->setText("1/" + QString::number(size));
                for (auto list : currentIndexData().yresulttype[num])
                {
                    m_yResultList->addItem(list);
                }
                m_yResultList->setCurrentRow(0);
            }

            //X 轴结果类型随 Y 侧工况联动（横坐标类型置首位，后接 Y结果类型）
            fillXResults();
            });

        connect(m_yResultList, &QListWidget::itemSelectionChanged, this, [=]() {
            m_yObjectList->clear();
            m_yComponentList->clear();

            //R22：ascii 文件（csv/txt）对象列固定 [Block 1]（无工况级，无
            //"Point id " 前缀可解析），自动选中后经 m_yObjectList 联动
            //（refreshYComponents）以全部列名填充分量列
            if (isAsciiCurrentFile())
            {
                m_yObjectList->addItem(mCurveDataSourceStore::asciiObjectDisplayText());
                updateCountLabel(yLabel_3, m_yObjectList);   //填充后未选中=0/1
                m_yObjectList->setCurrentRow(0);
                return;
            }

            int num = subcaseNumberOf(m_ySubcaseList->currentItem()->text());

            QString yResult = m_yResultList->currentItem()->text();

            int size = currentIndexData().object[num][yResult].size();
            if (size != 0)
            {
                for (auto pointid : currentIndexData().object[num][yResult])
                {
                    QString strCase = "Point id ";
                    m_yObjectList->addItem(strCase + QString::number(pointid));
                }
                //填充后无选中=0/N，随用户多选实时增长（ExtendedSelection）
                updateCountLabel(yLabel_3, m_yObjectList);
            }
            });
        //分量列表填充提取为成员函数：级联选择变化与图表类型切换（复数图合并
        //状态变化）共用同一刷新入口
        connect(m_yObjectList, &QListWidget::itemSelectionChanged,
            this, &mOpenCurveDlg::refreshYComponents);
        //图表类型切换即时刷新（截图流程）：弹窗开着时把目标图表切为/切出
        //复数图，分量列表按方向合并/展开实时重算。批量建图期间抑制（新图表
        //下发 kChartType 会走到这里，clear 会删掉 slot_createLine 持有的
        //选集条目形成悬空指针）
        connect(mCurveDocController::getInstance(),
            &mCurveDocController::sig_nodePropertyChanged, this,
            [this](int nodeId, const QString& prop)
        {
            if (prop == CurveProp::kChartType && isVisible() && !_creatingCharts)
                refreshYComponents();
        });

        //初始无任何选择/文件：绘图钮为禁用态，待选择条件齐备再启用
        updateCreateButtonState();
    }

    void mOpenCurveDlg::clearAllList()
    {
        xLabel_1->setText("");
        xLabel_2->setText("");
        xLabel_3->setText("");
        yLabel_1->setText("");
        yLabel_2->setText("");
        yLabel_3->setText("");
        yLabel_4->setText("");

        //清理时禁用信号
        m_xResultList->blockSignals(true);
        m_xObjectList->blockSignals(true);
        m_xComponentList->blockSignals(true);
        m_ySubcaseList->blockSignals(true);
        m_yResultList->blockSignals(true);
        m_yObjectList->blockSignals(true);
        m_yComponentList->blockSignals(true);

        m_yComponentList->clear();
        m_yObjectList->clear();
        m_yResultList->clear();
        m_ySubcaseList->clear();
        m_xObjCompPanel->hide();   //对象/分量两栏随列表清空先隐藏（后续联动按需显示）
        m_xComponentList->clear();
        m_xObjectList->clear();
        m_xResultList->clear();

        m_xResultList->blockSignals(false);
        m_xObjectList->blockSignals(false);
        m_xComponentList->blockSignals(false);
        m_ySubcaseList->blockSignals(false);
        m_yResultList->blockSignals(false);
        m_yObjectList->blockSignals(false);
        m_yComponentList->blockSignals(false);

        //清空后选择条件不再满足（清理时信号被阻塞，联动刷新未触发），
        //显式同步绘图按钮可用性
        updateCreateButtonState();
    }

    //绘图按钮可用性：与 slot_createLine 的前置校验同源——X 结果类型必选，
    //X 为文件实际结果类型时还需 对象×分量；Y 侧 工况/结果类型必选且
    //对象/分量至少选中一项。条件齐备才启用，条件不满足时按钮置灰，
    //替代点击后逐项告警的交互
    void mOpenCurveDlg::updateCreateButtonState()
    {
        //R22：ascii 文件（csv/txt）无工况级（工况列隐藏、列表恒空），选择条件
        //不含工况项；X 结果类型恒为通道形态（ascii 无横坐标类型，自然走
        //对象×分量分支）
        const bool asciiFile = isAsciiCurrentFile();
        bool ready = false;
        if (m_xResultList->currentItem()
            && (asciiFile || m_ySubcaseList->currentItem())
            && m_yResultList->currentItem()
            && !m_yObjectList->selectedItems().isEmpty()
            && !m_yComponentList->selectedItems().isEmpty())
        {
            if (!asciiFile && isAbscissaXResult(m_xResultList->currentItem()->text()))
            {
                //横坐标类型：X 直接取记录横坐标，无需对象/分量
                ready = true;
            }
            else
            {
                ready = m_xObjectList->currentItem()
                    && m_xComponentList->currentItem();
            }
        }
        m_createBtn->setEnabled(ready);
    }

    void mOpenCurveDlg::ParseFile(QString filePath)
    {
        //解析与层级索引统一入共享数据源仓库（"定义曲线"弹窗等共用同一份缓存）。
        //导入异步化（FR-01）：解析移后台线程，成功续接链路移入
        //sig_importFinished 分支；入口受理即返回，不再冻结界面
        auto* store = mCurveDataSourceStore::getInstance();
        //Accepted/DuplicateInFlight：解析在后台执行，成败经信号续接。接收上下文
        //=this（C3：解析中可关本对话框——D5，Qt 随对象销毁自动断连防悬垂）；
        //按键匹配防多导入串扰；命中后自断连。
        //协调器须在 addFileAsync 之前就位（否则首个 sig_importStarted 无订阅方）
        mCurveImportCoordinator* coordinator = mCurveImportCoordinator::getInstance(window());
        const auto accepted = store->addFileAsync(filePath);
        if (accepted == mCurveDataSourceStore::AddFileAsyncResult::AlreadyRegistered)
        {
            //已登记幂等成功：走原内联刷新链路（现状语义原样保留，C1）
            //清空所有列表
            clearAllList();

            //列表中数据添加
            setListWig();
            return;
        }
        if (accepted == mCurveDataSourceStore::AddFileAsyncResult::DuplicateInFlight)
            coordinator->notifyDuplicateInFlight(filePath);
        const QString key = mCurveDataSourceStore::normalizePath(filePath);
        const auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(store, &mCurveDataSourceStore::sig_importFinished, this,
            [this, key, conn](const QString& finishedKey, bool ok, const QString& err)
            {
                if (finishedKey != key)
                    return;
                QObject::disconnect(*conn);
                if (ok)
                {
                    //原内联成功链路（逐字保留）
                    //清空所有列表
                    clearAllList();

                    //列表中数据添加
                    setListWig();
                }
                else
                {
                    qDebug() << (err.isEmpty() ? QStringLiteral("文件存在问题") : err);
                }
            });
    }

    const PchReader::PchFile* mOpenCurveDlg::currentFile() const
    {
        //未导入当前文件时返回空数据（调用方直接遍历 blocks，行为同原 QHash 缺省构造）
        static const PchReader::PchFile kEmpty;
        const PchReader::PchFile* f = mCurveDataSourceStore::getInstance()->file(_currentlyFilePath);
        return f ? f : &kEmpty;
    }

    const ListWgtStruct& mOpenCurveDlg::currentIndexData() const
    {
        static const ListWgtStruct kEmpty;
        const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(_currentlyFilePath);
        return idx ? *idx : kEmpty;
    }

    void mOpenCurveDlg::filterList(QListWidget* list, const QString& text)
    {
        if (!list) return; // 防止空指针

        // 遍历所有列表项
        for (int i = 0; i < list->count(); ++i) {
            QListWidgetItem* item = list->item(i);

            // 如果搜索框为空，显示全部
            if (text.isEmpty()) {
                item->setHidden(false);
            }
            else {
                item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
            }
        }
    }

    bool mOpenCurveDlg::isAsciiCurrentFile() const
    {
        //数据源类型判定收口在共享仓库（.csv/.txt 不分大小写 -> AsciiColumn）
        return mCurveDataSourceStore::fileKind(_currentlyFilePath)
            == mCurveDataSourceStore::DataSourceKind::AsciiColumn;
    }

    void mOpenCurveDlg::setListWig()
    {
        //R22：ascii 文件（csv/txt）按三级列表填充（无工况层级）；注意 ascii 的
        //层级索引也有 operatingcondition={1} 骨架，但 currentFile() 为空，pch 的
        //工况文本生成（subcaseDisplayText(*currentFile(), id)）不可走
        if (isAsciiCurrentFile())
        {
            setAsciiListWig();
            return;
        }

        //pch：工况列保持显示（从 ascii 文件切回时恢复；首次打开默认亦为显示态）
        _ySubcaseCol->show();

        //工况部分（块的 label（$LABEL = 行内容，如 1001_X）有内容时在文本末尾追加）
        int size = currentIndexData().operatingcondition.size();
        if (size != 0) {
            yLabel_1->setText("1/" + QString::number(size));
            for (int id : currentIndexData().operatingcondition)
            {
                //工况文本统一由共享数据源仓库生成（"定义曲线"同源调用，内容一致）
                m_ySubcaseList->addItem(
                    mCurveDataSourceStore::subcaseDisplayText(*currentFile(), id));
            }
            //设置选中第一个（联动填充 Y 结果类型与 X 结果类型/对象/分量）
            m_ySubcaseList->setCurrentRow(0);
        }

        //X 结果类型列表依赖 Y 侧工况选择（横坐标类型置首位默认选中，后接
        //Y结果类型）；无工况数据时也至少给出横坐标类型
        fillXResults();
    }

    void mOpenCurveDlg::setAsciiListWig()
    {
        //csv/txt 无工况层级：整列隐藏（pch 文件切换回来时由 setListWig 恢复显示）
        yLabel_1->setText("");
        _ySubcaseCol->hide();

        //Y 侧结果类型列 = [Unknown]（ascii 语义常量，HyperGraph Types 列唯一项），
        //自动选中后经既有 itemSelectionChanged 联动级联填充 对象/分量 两列
        QStringList yResults;
        yResults << mCurveDataSourceStore::asciiSchemaDisplayText();
        m_yResultList->addItems(yResults);
        updateCountLabel(yLabel_2, m_yResultList);   //填充后未选中=0/1，选中后刷为1/1
        m_yResultList->setCurrentRow(0);

        //X 侧三级列表：结果类型同 Y（ascii 无横坐标类型），分量默认选中第 1 列
        fillXResults();
    }

    bool mOpenCurveDlg::isAbscissaXResult(const QString& schema) const
    {
        //文件解析出的 X 结果类型集合（Time[Sec]/Frequency[Hz]，随文件数据形态
        //解析而定）：集合内的类型为横坐标类型，X 数据直接取记录横坐标
        return currentIndexData().xresulttype.count(schema) > 0;
    }

    void mOpenCurveDlg::fillXResults()
    {
        //先清下游再清本级（清对象列表会触发 fillXComponents，此时结果项尚未
        //变化，但对象已空会被其守卫拦下）
        m_xComponentList->clear();
        m_xObjectList->clear();
        m_xResultList->clear();
        xLabel_2->setText("");
        xLabel_3->setText("");

        //X 结果类型 = 文件解析出的横坐标类型（置首位：瞬态块解析为 Time[Sec]，
        //频响复数块解析为 Frequency[Hz]，随文件数据形态而定）+ 当前 Y 侧工况的
        //Y 结果类型；首位默认选中（默认按记录横坐标赋值）
        //R22：ascii 文件（csv/txt）无横坐标类型（xresulttype 空集）且无工况级，
        //结果类型列固定 [Unknown]（通道形态语义，X 走列装配）
        QStringList results;
        if (isAsciiCurrentFile())
        {
            results << mCurveDataSourceStore::asciiSchemaDisplayText();
        }
        else
        {
            for (const auto& t : currentIndexData().xresulttype)
            {
                results << t;
            }
            if (m_ySubcaseList->currentItem())
            {
                int num = subcaseNumberOf(m_ySubcaseList->currentItem()->text());
                for (const auto& r : currentIndexData().yresulttype[num])
                {
                    results << r;
                }
            }
        }

        m_xResultList->addItems(results);
        updateCountLabel(xLabel_1, m_xResultList);   //无结果=空；下方默认选中后由选择连接刷为1/N
        //默认选中首位（横坐标类型：默认按记录横坐标赋值，联动对象/分量显隐）
        m_xResultList->setCurrentRow(0);
    }

    void mOpenCurveDlg::fillXObjects()
    {
        m_xComponentList->clear();
        xLabel_3->setText("");
        m_xObjectList->clear();

        //交互：横坐标类型（文件解析的 Time[Sec]/Frequency[Hz]）无对象/分量，
        //隐藏两栏（创建时 X 按记录横坐标赋值）；实际结果类型显示两栏，
        //按 Y 侧方式联动（创建时 X 按对象×分量通道数值装配）
        if (!m_xResultList->currentItem()
            || isAbscissaXResult(m_xResultList->currentItem()->text()))
        {
            xLabel_2->setText("");
            m_xObjCompPanel->hide();
            return;
        }
        m_xObjCompPanel->show();

        //R22：ascii 文件（csv/txt）对象列固定 [Block 1]，自动选中后级联分量列
        //（不查工况级索引、不做 "Point id " 解析）
        if (isAsciiCurrentFile())
        {
            m_xObjectList->addItem(mCurveDataSourceStore::asciiObjectDisplayText());
            updateCountLabel(xLabel_2, m_xObjectList);   //填充后未选中=0/1
            m_xObjectList->setCurrentRow(0);
            return;
        }

        if (!m_ySubcaseList->currentItem())
        {
            xLabel_2->setText("");
            return;
        }

        QString xResult = m_xResultList->currentItem()->text();

        int num = subcaseNumberOf(m_ySubcaseList->currentItem()->text());

        int size = currentIndexData().object[num][xResult].size();
        if (size != 0)
        {
            for (auto pointid : currentIndexData().object[num][xResult])
            {
                QString strCase = "Point id ";
                m_xObjectList->addItem(strCase + QString::number(pointid));
            }
            updateCountLabel(xLabel_2, m_xObjectList);   //填充后无选中=0/N
        }
        else
        {
            xLabel_2->setText("");
        }
    }

    void mOpenCurveDlg::fillXComponents()
    {
        m_xComponentList->clear();
        xLabel_3->setText("");

        //R22：ascii 文件（csv/txt）X 分量列 = 全部列名，默认选中第 1 列（用户
        //拍板：X 默认取第 1 列，如 STRAIN，可改选其他列）。置于工况守卫之前——
        //ascii 无工况级，m_ySubcaseList 恒空
        if (isAsciiCurrentFile())
        {
            const QStringList cols =
                mCurveDataSourceStore::getInstance()->asciiColumnNames(_currentlyFilePath);
            if (!cols.isEmpty())
            {
                m_xComponentList->addItems(cols);
                updateCountLabel(xLabel_3, m_xComponentList);   //填充后未选中=0/N
                m_xComponentList->setCurrentRow(0);   //默认选中第 1 列
            }
            return;
        }

        if (!m_ySubcaseList->currentItem() ||
            !m_xResultList->currentItem() ||
            !m_xObjectList->currentItem())
        {
            return;
        }

        QString xResult = m_xResultList->currentItem()->text();
        if (isAbscissaXResult(xResult))
        {
            return;
        }

        int num = subcaseNumberOf(m_ySubcaseList->currentItem()->text());

        QString prefix2 = "Point id ";
        int num2 = m_xObjectList->currentItem()->text().mid(prefix2.length()).trimmed().toInt();

        int size = currentIndexData().ycomponent[num][xResult][num2].size();

        if (size != 1)
        {
            qDebug() << "文件部分错误，请检查";
            return;
        }

        //结果类型非节点矢量不展开分量（规则同Y轴）
        if (!mCurveDataSourceStore::isVectorSchema(xResult))
        {
            return;
        }

        const std::set<NumericFmtInfo> setstr = currentIndexData().ycomponent[num][xResult][num2];

        //分量展开统一走共享数据源仓库（瞬态标志随索引叶子下发，块级判定）
        for (const NumericFmtInfo& info : setstr)
        {
            const QStringList components =
                mCurveDataSourceStore::expandedComponents(info.fmt, info.transient);

            if (!components.isEmpty())
            {
                m_xComponentList->addItems(components);
                updateCountLabel(xLabel_3, m_xComponentList);   //填充后无选中=0/N
            }
        }
    }

    int mOpenCurveDlg::getDataIndex(QString text)
    {
        //分量名 -> 数据下标规则已迁至共享数据源仓库（"定义曲线"弹窗共用）
        return mCurveDataSourceStore::componentIndex(text);
    }

    void mOpenCurveDlg::slot_drawOptions()
    {
        drawOption->show();
    }

    bool mOpenCurveDlg::buildSeriesHash(const QList<QListWidgetItem*>& objects,
        const QList<QListWidgetItem*>& components, int subcaseid, const QString& schema,
        QHash<QString, QVector<double>>& XHash, QHash<QString, QVector<double>>& YHash,
        const QString& xSchema, int xObjectId, const QString& xComponent,
        int targetChartType)
    {
        //R22：ascii 文件（csv/txt）装配分支——currentFile() 对 ascii 返回空
        //（PchFile/AsciiColumnFile 分存），不得进入下方 pch 块遍历。component=
        //列名经实例版 assembleChannelData 匹配列（subcaseId/schema/pointId 传
        //ascii 骨架常量，仓库不参与匹配），feature=列名本身（不带 "对象-" 前缀，
        //曲线名=列名）。复数图目标不做 MAG/PHA 方向展开（列名无配对语义，
        //appendCurvesToChart 侧按普通单曲线落图）；柱状图目标不加装配分支
        //（appendCurvesToChart 按目标节点类型分流）
        if (isAsciiCurrentFile())
        {
            auto* store = mCurveDataSourceStore::getInstance();
            const int subId = mCurveDataSourceStore::asciiSubcaseId();
            const int pointId = mCurveDataSourceStore::asciiPointId();

            //X 轴：ascii 无横坐标类型，X 结果类型恒为通道形态——按 X 页所选
            //分量列装配一份共享 X 序列（默认第 1 列）；装配失败退回行号
            //（assembleChannelData 的默认横坐标），避免整批不可建
            bool xUseComponent = !xSchema.isEmpty() && !isAbscissaXResult(xSchema);
            QVector<double> xShared;
            if (xUseComponent)
            {
                mCurveDataSourceStore::ChannelDataResult xr;
                QString xErr;
                if (!store->assembleChannelData(_currentlyFilePath, subId, xSchema,
                    pointId, xComponent, &xr, &xErr))
                {
                    mCurveDocController::getInstance()->reportWarning(
                        QStringLiteral("X轴数据装配失败（%1），X轴退回行号")
                            .arg(xErr.isEmpty() ? xComponent : xErr));
                    xUseComponent = false;
                }
                else
                {
                    //列数值在装配结果的 yData(xData 恒为行号 1..N 默认横坐标),
                    //X 轴须取所选列的值序列——评审返工(20261109):原取 xData
                    //致 X 页选列不生效、落图恒为行号
                    xShared = xr.yData;
                }
            }

            //ascii 恒单对象（Block 1），对象文本不参与装配，直接按所选分量列
            //逐列装配（布局 1 的按对象分图因单对象天然等价一图多线）
            for (auto component : components)
            {
                const QString compText = component->text();
                mCurveDataSourceStore::ChannelDataResult r;
                QString err;
                if (!store->assembleChannelData(_currentlyFilePath, subId,
                    schema, pointId, compText, &r, &err))
                {
                    if (!err.isEmpty())
                    {
                        mCurveDocController::getInstance()->reportWarning(
                            QStringLiteral("曲线“%1”装配失败：%2，已跳过")
                                .arg(compText, err));
                    }
                    continue;   //列装配失败：该分量不入表
                }
                //X 用所选列时同一文件行位天然对齐；点数不一致按 pch 路径
                //同款文案告警跳过
                if (xUseComponent && r.yData.size() != xShared.size())
                {
                    mCurveDocController::getInstance()->reportWarning(
                        QStringLiteral("曲线“%1”的X/Y数据点数不一致（X:%2 Y:%3），已跳过")
                            .arg(compText).arg(xShared.size()).arg(r.yData.size()));
                    continue;
                }
                const QString feature = compText;   //曲线名 = 列名
                YHash[feature] = std::move(r.yData);
                XHash[feature] = xUseComponent ? xShared : std::move(r.xData);
            }
            //R22-B01：返回 X 是否按所选列装配成功（失败退行号时 false——创建侧
            //不补写 X 侧来源四元组，保持未记录）
            return xUseComponent;
        }

        //X 轴数据来源：文件解析出的横坐标类型（Time[Sec]/Frequency[Hz]，未指定
        //时同）取记录横坐标；文件实际结果类型则按 X 侧 对象×分量 取该通道数值
        //（规则同 Y 侧装配）
        bool xUseComponent = !xSchema.isEmpty() && !isAbscissaXResult(xSchema);

        //分量来源的共享 X 序列：先装配一份，所有 feature 共用
        QVector<double> xShared;
        if (xUseComponent)
        {
            //分量索引无效（分量名不识别）时退回记录横坐标，避免整批不可建
            const int xIdx = mCurveDataSourceStore::componentIndex(xComponent);
            if (xIdx < 0)
            {
                mCurveDocController::getInstance()->reportWarning(
                    QStringLiteral("未识别的X轴分量名称:%1，X轴退回记录横坐标").arg(xComponent));
                xUseComponent = false;
            }
            else
            {
                //防线：PHA/IMAG 分量（xIdx>=6）在瞬态实数块（无 secondary）没有
                //真实数据，该块跳过并告警（与 Y 侧同规则，每通道只告警一次）
                bool warnedNoSecondaryX = false;

                for (const auto& file : currentFile()->blocks)
                {
                    if (file.subcaseId == subcaseid
                        && QString::fromStdString(file.schemaName) == xSchema
                        && file.pointId == xObjectId)
                    {
                        if (xIdx >= 6)
                        {
                            bool hasSecondary = false;
                            for (const auto& rec : file.records)
                            {
                                if (rec.secondary.has_value())
                                {
                                    hasSecondary = true;
                                    break;
                                }
                            }
                            if (!hasSecondary)
                            {
                                if (!warnedNoSecondaryX)
                                {
                                    mCurveDocController::getInstance()->reportWarning(
                                        QStringLiteral("X轴通道“%1-Point id %2-%3”的数据块无虚部/相位数据（瞬态实数块），已跳过")
                                            .arg(xSchema).arg(xObjectId).arg(xComponent));
                                    warnedNoSecondaryX = true;
                                }
                                continue;
                            }
                        }
                        for (const auto& record : file.records)
                        {
                            if (xIdx <= 5)
                            {
                                xShared.push_back(record.primary[xIdx]);
                            }
                            else if (record.secondary.has_value())
                            {
                                xShared.push_back(record.secondary.value()[xIdx - 6]);
                            }
                            else
                            {
                                xShared.push_back(0.0);
                            }
                        }
                    }
                }
            }
        }

        for (auto pointid : objects)
        {
            QString yObject = pointid->text();
            QString prefix = "Point id ";
            QString numStr = yObject.mid(prefix.length()).trimmed();
            int objectid = numStr.toInt();

        //复数图目标：Y 分量按方向显示（X-Trans），绘制时展开为 MAG/PHA 两侧
        //各装配一条曲线（feature=对象-MAG|X-Trans / 对象-PHA|X-Trans），复数图
        //渲染按命名约定配对成上下面板；非复数图维持单分量路径。目标类型由
        //调用方快照传参（批次中途激活链切到新建图表，实时查询会翻转装配状态）
        const bool complexTarget = (targetChartType == 2);
        static const char* const kComplexToks[] = { "MAG", "PHA" };

        //单分量装配（对象×分量 -> feature 数据），校验/告警/缺虚部防线同原路径
        auto assembleFeature = [&](const QString& compText) -> bool
        {
            //分量名不识别时整组跳过（保持原告警文案），避免 -1 下标越界
            if (mCurveDataSourceStore::componentIndex(compText) < 0)
            {
                mCurveDocController::getInstance()->reportWarning(
                    QStringLiteral("未识别的分量名称:%1，已跳过").arg(compText));
                return false;
            }

            //feature key = "对象-分量"；装配规则（块匹配/缺虚部防线/取值次序）
            //统一在共享仓库 assembleChannelData（"定义曲线"/"更改曲线"同源）
            const QString feature = pointid->text() + "-" + compText;
            mCurveDataSourceStore::ChannelDataResult r;
            if (!mCurveDataSourceStore::assembleChannelData(
                *currentFile(), subcaseid, schema, objectid, compText, &r))
            {
                return false;   //无匹配数据块：feature 不入表，创建时按原行为静默跳过
            }
            if (r.skippedNoSecondary)
            {
                mCurveDocController::getInstance()->reportWarning(
                    QStringLiteral("通道“%1”的数据块无虚部/相位数据（瞬态实数块），已跳过").arg(feature));
            }
            XHash[feature] = std::move(r.xData);
            YHash[feature] = std::move(r.yData);
            return true;
        };

        for (auto component : components)
        {
            const QString compText = component->text();
            if (complexTarget && !compText.contains('|'))
            {
                //合并方向分量：逐侧装配（缺侧/瞬态实数块按原防线告警跳过，
                //至少一侧成功即视为已产出）
                for (const char* tok : kComplexToks)
                    assembleFeature(QStringLiteral("%1|%2").arg(tok, compText));
                continue;
            }
            assembleFeature(compText);
        }
        }

        //分量来源 X：与各 feature 的 Y 数据点数一致才配对成曲线，不一致告警跳过
        if (xUseComponent)
        {
            for (auto it = YHash.begin(); it != YHash.end();)
            {
                if (it.value().size() == xShared.size())
                {
                    XHash[it.key()] = xShared;
                    ++it;
                }
                else
                {
                    mCurveDocController::getInstance()->reportWarning(
                        QStringLiteral("曲线“%1”的X/Y数据点数不一致（X:%2 Y:%3），已跳过")
                            .arg(it.key()).arg(xShared.size()).arg(it.value().size()));
                    it = YHash.erase(it);
                }
            }
        }
        //返回值仅 ascii 分支被消费（创建侧据此决定是否补写 X 侧四元组）；pch
        //分支恒传空 xComponent，返回值不被消费
        return xUseComponent;
    }

    void mOpenCurveDlg::refreshYComponents()
    {
        //Y 分量列表按当前 工况×结果类型×对象 重填（级联选择变化与图表类型
        //切换共用）；复数图目标：成对分量按方向合并显示（截图），绘制时一次
        //装配两侧
        m_yComponentList->clear();

        //R22：ascii 文件（csv/txt）分量列 = 全部列名（多选），不经 pch 的数值
        //格式展开、不做复数图方向合并（列名无 MAG/PHA 配对语义）。置于工况守卫
        //之前——ascii 无工况级；结果/对象须已选中，拦下级联清空中的过渡触发
        if (isAsciiCurrentFile())
        {
            if (!m_yResultList->currentItem() || !m_yObjectList->currentItem())
                return;
            const QStringList cols =
                mCurveDataSourceStore::getInstance()->asciiColumnNames(_currentlyFilePath);
            if (!cols.isEmpty())
            {
                m_yComponentList->addItems(cols);
                //填充后无选中=0/N，随用户多选实时增长（ExtendedSelection）
                updateCountLabel(yLabel_4, m_yComponentList);
            }
            return;
        }

        if (!m_ySubcaseList->currentItem() ||
            !m_yResultList->currentItem() ||
            !m_yObjectList->currentItem())
        {
            return;
        }
        int num = subcaseNumberOf(m_ySubcaseList->currentItem()->text());

        QString yResult = m_yResultList->currentItem()->text();

        QString yObject = m_yObjectList->currentItem()->text();
        QString prefix2 = "Point id ";
        int num2 = yObject.mid(prefix2.length()).trimmed().toInt();

        int size = currentIndexData().ycomponent[num][yResult][num2].size();

        if (size != 1)
        {
            qDebug() << "文件部分错误，请检查";
            return;
        }

        const std::set<NumericFmtInfo> setstr = currentIndexData().ycomponent[num][yResult][num2];

        // 判断结果类型是否为节点矢量
        bool isVectorResult = (yResult == "VELOCITY" ||
            yResult == "DISPLACEMENTS" ||
            yResult == "ACCELERATION" ||
            yResult == "FORCE");

        if (!isVectorResult)
        {
            return;
        }

        //分量展开统一走共享数据源仓库；瞬态标志随索引叶子下发（块级判定，
        //数据形态优先），不再按文件级 xresulttype 反推——无 IDENTIFIED BY TIME
        //标签的瞬态 deck 旧逻辑会被误判为频响而展开出 12 个分量
        for (const NumericFmtInfo& info : setstr)
        {
            QStringList components =
                mCurveDataSourceStore::expandedComponents(info.fmt, info.transient);
            if (drawIntoComplexChart() && !components.isEmpty())
                components = mergedComplexComponents(components);

            if (!components.isEmpty())
            {
                m_yComponentList->addItems(components);
                //填充后无选中=0/N，随用户多选实时增长（ExtendedSelection）
                updateCountLabel(yLabel_4, m_yComponentList);
            }
        }
    }

    bool mOpenCurveDlg::drawIntoComplexChart() const
    {
        //"使用当前绘图"命中的当前视口图表为复数图（图表类型2）时，Y 分量按
        //方向合并显示并成对装配
        const int chartId = mCurveViewportFactory::getInstance()->currentChartId();
        if (chartId < 0)
            return false;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        CurveNode* n = doc ? doc->node(chartId) : nullptr;
        return n && n->type == CurveNodeType::Chart && n->chart.chartType == 2;
    }

    void mOpenCurveDlg::appendCurvesToChart(int chartId,
        const QHash<QString, QVector<double>>& XHash,
        const QHash<QString, QVector<double>>& YHash,
        int subcaseid, const QString& schema, const QString& xComponent)
    {
        if (XHash.isEmpty())
            return;

        mCurveDocController* controller = mCurveDocController::getInstance();

        //与 NVHCurveWgt 一致的轮换配色（共享定义见 PostCurveGlobal.h：curveDefaultColorWheel）
        const QVector<QColor> kColors = curveDefaultColorWheel();
        const DrawingOptionsStr& opt = drawOption->getDrawOption();

        //线型方案映射（对齐原型：1虚线 2/3点划线）；线宽方案 = scheme+1
        int lineStyle = 0;
        if (opt.selectLineStyleScheme)
        {
            if (opt.lineStyleScheme == 1) lineStyle = 1;
            else if (opt.lineStyleScheme == 2 || opt.lineStyleScheme == 3) lineStyle = 3;
        }
        double lineWidth = opt.selectLineWidthScheme ? (opt.lineWidthScheme + 1) : 2.0;

        //轮换配色从既有曲线数续起：并入追加时不从头取色，避免与已有曲线同色
        int index = 0;
        for (int childId : controller->document()->childNodeIds(chartId))
        {
            if (controller->document()->nodeType(childId) == CurveNodeType::Curve)
                ++index;
        }

        //复数图目标：MAG/PHA（REAL/IMAG）特征对合并为单条复数曲线（设计图
        //Complex Plot：树上单节点挂 x/ym/yp 三壳，引擎上下面板各一条子曲线；
        //不再产出两条按命名约定配对的曲线节点）
        //R22：ascii 文件（csv/txt）特征=列名本身，无 MAG/PHA 配对语义（按'-'或
        //'|'拆分特征会误切含'-'列名），复数图目标下按现有降级路径落普通单曲线
        const bool asciiSource = isAsciiCurrentFile();
        CurveNode* chartNode = controller->document()->node(chartId);
        const bool complexTarget = !asciiSource && chartNode
            && chartNode->type == CurveNodeType::Chart
            && chartNode->chart.chartType == 2;

        //特征名解析："对象-MAG|方向" → 键="对象|方向" + 对象/方向/分量令牌/面板侧；
        //非复数分量特征返回 false（普通单曲线路径）
        QString cKey, cObject, cDir, cToken;
        bool cUpper = false;
        auto splitComplexFeature = [&](const QString& feature) -> bool
        {
            cKey.clear(); cObject.clear(); cDir.clear(); cToken.clear(); cUpper = false;
            const int bar = feature.indexOf(QLatin1Char('|'));
            if (bar <= 0)
                return false;
            const QString head = feature.left(bar);
            static const char* const kToks[] = { "MAG", "PHA", "REAL", "IMAG" };
            for (const char* raw : kToks)
            {
                const QString t = QStringLiteral("-%1").arg(raw);
                if (!head.endsWith(t, Qt::CaseInsensitive))
                    continue;
                cObject = head.left(head.size() - t.size());
                cKey = cObject + QLatin1Char('|') + feature.mid(bar + 1);
                cDir = feature.mid(bar + 1);
                cToken = feature.mid(bar - t.size() + 1);   //"MAG|X-Trans" 形态
                cUpper = (qstricmp(raw, "MAG") == 0 || qstricmp(raw, "REAL") == 0);
                return true;
            }
            return false;
        };

        if (complexTarget)
        {
            //按键分组（QMap 键序确定）：上下侧分别缓存数据与令牌
            struct PairAcc
            {
                QVector<double> x, ay, by;
                QString object, dir, tokenA, tokenB;
            };
            QMap<QString, PairAcc> pairs;
            for (auto it = XHash.constBegin(); it != XHash.constEnd(); ++it)
            {
                if (!splitComplexFeature(it.key()))
                    continue;
                PairAcc& p = pairs[cKey];
                p.object = cObject;
                p.dir = cDir;
                if (cUpper)
                {
                    p.x = it.value();
                    p.ay = YHash.value(it.key());
                    p.tokenA = cToken;
                }
                else
                {
                    p.by = YHash.value(it.key());
                    p.tokenB = cToken;
                }
            }

            controller->undoStack()->beginMacro(QStringLiteral("添加pch曲线"));
            for (auto it = pairs.constBegin(); it != pairs.constEnd(); ++it, ++index)
            {
                const PairAcc& p = it.value();
                const QString dispName = p.object + QStringLiteral(" - ") + p.dir;
                int curveId = -1;
                if (!p.ay.isEmpty() && !p.by.isEmpty())
                {
                    //成对：单条复数曲线（上=幅值/实部，下=相位/虚部）
                    curveId = controller->addComplexCurve(chartId, dispName);
                    if (curveId >= 0)
                    {
                        controller->setComplexSeriesData(curveId, p.x, p.ay, p.by);
                        controller->setComplexSeriesSource(curveId, _currentlyFilePath,
                            QStringLiteral("Subcase %1").arg(subcaseid), schema,
                            p.object, p.tokenA, p.tokenB,
                            p.tokenA.startsWith(QStringLiteral("MAG"), Qt::CaseInsensitive) ? 0 : 1);
                    }
                }
                else
                {
                    //缺对侧：降级普通单曲线（原渲染层缺侧防线前移到创建侧）
                    const QVector<double>& y = p.ay.isEmpty() ? p.by : p.ay;
                    if (p.x.isEmpty() || y.isEmpty())
                        continue;
                    curveId = controller->addCurve(chartId, dispName);
                    if (curveId >= 0)
                    {
                        controller->setSeriesData(curveId, p.x, y);
                        controller->setSeriesSource(curveId, _currentlyFilePath,
                            QStringLiteral("Subcase %1").arg(subcaseid), schema,
                            p.object, p.ay.isEmpty() ? p.tokenB : p.tokenA);
                    }
                }
                if (curveId < 0)
                    continue;

                //线条样式/符号（与普通路径同一份映射）
                QColor color = opt.selectLineColor ? QColor(opt.lineColor)
                    : kColors.at(index % kColors.size());
                controller->setNodeProperty(curveId, CurveProp::kColor, color);
                controller->setNodeProperty(curveId, CurveProp::kLineWidth, lineWidth);
                if (opt.selectLineStyleScheme)
                    controller->setNodeProperty(curveId, CurveProp::kLineStyle, lineStyle);
                if (opt.selectSymbol && opt.symbol != 0)
                {
                    controller->setNodeProperty(curveId, CurveProp::kSymbolStyle, opt.symbol);
                    controller->setNodeProperty(curveId, CurveProp::kSymbolColor, QColor(opt.symbolColor));
                }
            }
            controller->undoStack()->endMacro();
            return;
        }

        controller->undoStack()->beginMacro(QStringLiteral("添加pch曲线"));
        //柱状图目标走柱状曲线变体（仅 y 单壳：无横坐标数据语义，横轴为类目轴）
        const bool barTarget = chartNode && chartNode->type == CurveNodeType::Chart
            && chartNode->chart.chartType == 1;
        for (auto it = XHash.constBegin(); it != XHash.constEnd(); ++it, ++index)
        {
            const QString& feature = it.key();
            const QVector<double>& yVec = YHash.value(feature);
            if (it.value().isEmpty() || yVec.isEmpty())
            {
                controller->reportWarning(QStringLiteral("曲线“%1”无有效数据点，未创建").arg(feature));
                continue;
            }
            int curveId = barTarget ? controller->addBarCurve(chartId, feature)
                : controller->addCurve(chartId, feature);
            if (curveId < 0)
                continue;
            controller->setSeriesData(curveId, it.value(), yVec);

            //来源信息（R22 分流）：pch 特征 = "对象-分量"，按首个'-'拆分（对象
            //文本自身不含'-'）；ascii 特征 = 列名本身（列名可含'-'，如 EI-Magd，
            //不得拆分），对象固定 Block 1、分量=列名，来源工况文本留空
            //（ascii 无工况级，{y.MXSubcase} 模板解析为空）
            QString object = feature;
            QString component;
            if (asciiSource)
            {
                object = mCurveDataSourceStore::asciiObjectDisplayText();
                component = feature;
            }
            else
            {
                int dash = feature.indexOf(QLatin1Char('-'));
                if (dash > 0)
                {
                    object = feature.left(dash);
                    component = feature.mid(dash + 1);
                }
            }
            controller->setSeriesSource(curveId, _currentlyFilePath,
                asciiSource ? QString()
                            : QStringLiteral("Subcase %1").arg(subcaseid),
                schema, object, component);

            //R22-B01：ascii 且 X 按所选分量列装配成功时补写 X 侧来源四元组
            //（工况空串/Unknown/Block 1/X 列名，x 数据页据此回显与改选，与
            //"更改曲线"侧换源落模同款）。退行号装配（xComponent 空）保持未
            //记录同现状；柱状目标仅 y 壳无横坐标语义不写；pch 路径横坐标形态
            //语义保留不写（slot_createLine 侧已按 ascii 分流，此处再守一道）
            if (asciiSource && !barTarget && !xComponent.isEmpty())
            {
                controller->setCurveSideSource(curveId, true, QString(),
                    mCurveDataSourceStore::asciiSchemaDisplayText(),
                    mCurveDataSourceStore::asciiObjectDisplayText(), xComponent);
            }

            //线条样式（对齐原型映射）
            QColor color = opt.selectLineColor ? QColor(opt.lineColor)
                : kColors.at(index % kColors.size());
            controller->setNodeProperty(curveId, CurveProp::kColor, color);
            controller->setNodeProperty(curveId, CurveProp::kLineWidth, lineWidth);
            if (opt.selectLineStyleScheme)
                controller->setNodeProperty(curveId, CurveProp::kLineStyle, lineStyle);

            //符号（对齐原型：selectSymbol 且非 0 时绘制）
            if (opt.selectSymbol && opt.symbol != 0)
            {
                controller->setNodeProperty(curveId, CurveProp::kSymbolStyle, opt.symbol);
                controller->setNodeProperty(curveId, CurveProp::kSymbolColor, QColor(opt.symbolColor));
            }
        }
        controller->undoStack()->endMacro();
    }

    void mOpenCurveDlg::createChartFromData(int pageId,
        const QHash<QString, QVector<double>>& XHash,
        const QHash<QString, QVector<double>>& YHash,
        int subcaseid, const QString& schema, const QString& xResult,
        int chartType, const QString& xComponent)
    {
        if (XHash.isEmpty())
            return;

        mCurveDocController* controller = mCurveDocController::getInstance();
        const DrawingOptionsStr& opt = drawOption->getDrawOption();

        //绘图选项刻度字符串 -> 模型刻度模式（0 Linear/1 Log10/2 dB20/3 dB10）
        auto scaleModeOf = [](const QString& scale) -> int
        {
            if (scale == "Logarithmic") return 1;
            if (scale == "dB20") return 2;
            if (scale == "dB10") return 3;
            return 0;
        };
        //绘图选项加权字符串 -> 模型计权模式（0无/1A/2B/3C/4U；U=unweighted
        //与无等效，保留独立选项仅为与绘图选项所选值回显一致）
        auto weightModeOf = [](const QString& weight) -> int
        {
            if (weight == "A") return 1;
            if (weight == "B") return 2;
            if (weight == "C") return 3;
            if (weight == "U") return 4;
            return 0;
        };
        //对齐下拉索引（0左对齐/1居中/2右对齐）-> 模型对齐（0中/1左/2右）
        auto alignOf = [](int comboIdx) -> int
        {
            if (comboIdx == 0) return 1;
            if (comboIdx == 2) return 2;
            return 0;
        };

        //整组一次撤销：视口 + 各曲线 + 样式属性（页由调用方创建，同样在宏内）
        controller->undoStack()->beginMacro(QStringLiteral("创建pch曲线"));
        int chartId = controller->addChart(pageId);
        //新图表承接目标类型（1 柱状图/2 复数图，0 xy 保持默认）：kChartType 命令
        //自带骨架协调（复数补 Phase 副轴、默认名随类型改 Complex/Bar Plot），
        //随后的曲线装配（appendCurvesToChart 按目标节点类型分流）与渲染形态即
        //按同型目标走——否则复数装配的 MAG/PHA 特征会落进 xy 图表成普通曲线
        if (chartType != 0)
            controller->setNodeProperty(chartId, CurveProp::kChartType, chartType);

        //曲线入模型（数据/样式/来源）；内嵌宏的命令并入本次撤销组
        //（xComponent 透传：ascii 且 X 按列装配成功时补写 X 侧来源四元组）
        appendCurvesToChart(chartId, XHash, YHash, subcaseid, schema, xComponent);

        //绘图选项映射：页眉/页脚（文本=Y侧来源模板，对齐原型 firstLabel；占位符带
        //p<w>c 曲线位置引用——编号规则同树上显示/“定义曲线”引用，页序=根下
        //Page 序、视口序=页下 Chart 序、首条曲线 c1；渲染时按引用曲线解析，
        //见 mCurveChartView::resolveBandTemplate）/轴刻度模式/轴标题字体。
        //仅建图时应用一次；"使用当前绘图"并入既有图表只追加曲线，不覆盖这些设置
        CurveDocument* doc = controller->document();
        auto positionAmongSameType = [doc](int parentId, int nodeId) -> int
        {
            int pos = 0;
            for (int childId : doc->childNodeIds(parentId))
            {
                if (doc->nodeType(childId) != doc->nodeType(nodeId))
                    continue;
                ++pos;
                if (childId == nodeId)
                    return pos;
            }
            return pos;
        };
        const int pageIdOfChart = doc->parentNodeId(chartId);
        const QString curveRef = QStringLiteral("p%1w%2c1")
            .arg(pageIdOfChart > 0 ? positionAmongSameType(doc->rootNodeId(), pageIdOfChart) : 1)
            .arg(positionAmongSameType(pageIdOfChart, chartId));
        //页标题模板（Y 轴标题同款花括号占位符：{p<w>c1.y.MXDatatype}，渲染时解析；
        //入 kPageTitle 而非 kName——页名/树显示名不变）。同下方绘图选项映射：
        //仅建图时应用一次
        controller->setNodeProperty(pageIdOfChart, CurveProp::kPageTitle,
            QStringLiteral("{%1.y.MXDatatype}").arg(curveRef));
        controller->setNodeProperty(chartId, CurveProp::kHeaderVisible, opt.header);
        controller->setNodeProperty(chartId, CurveProp::kHeaderText,
            QStringLiteral("{%1.y.MXRequest} - {%1.y.MXComponent}").arg(curveRef));
        controller->setNodeProperty(chartId, CurveProp::kHeaderAlign, alignOf(drawOption->headerAlignIndex()));
        controller->setNodeProperty(chartId, CurveProp::kHeaderFont, opt.headerFont);
        controller->setNodeProperty(chartId, CurveProp::kFooterVisible, opt.footer);
        controller->setNodeProperty(chartId, CurveProp::kFooterText,
            QStringLiteral("{%1.y.MXRequest} - {%1.y.MXComponent}").arg(curveRef));
        controller->setNodeProperty(chartId, CurveProp::kFooterAlign, alignOf(drawOption->footerAlignIndex()));
        controller->setNodeProperty(chartId, CurveProp::kFooterFont, opt.footerFont);
        for (int childId : controller->document()->childNodeIds(chartId))
        {
            CurveNode* child = controller->document()->node(childId);
            if (!child)
                continue;
            switch (child->type)
            {
            case CurveNodeType::XAxis:
                controller->setNodeProperty(childId, CurveProp::kScaleMode, scaleModeOf(opt.xScale));
                controller->setNodeProperty(childId, CurveProp::kWeightMode, weightModeOf(opt.xWeight));
                //轴标题默认 = X 轴数据源中选中的结果类型（原拼接外观；横坐标形态
                //与通道形态统一落文本——通道形态的 xSideSchema 记录在建图时不填，
                //模板无值可解析）。拼接旧体系不再下发文本
                controller->setNodeProperty(childId, CurveProp::kTitle, xResult);
                controller->setNodeProperty(childId, CurveProp::kTitleFont, opt.xFont);
                break;
            case CurveNodeType::YAxis:
                controller->setNodeProperty(childId, CurveProp::kScaleMode, scaleModeOf(opt.yScale));
                controller->setNodeProperty(childId, CurveProp::kWeightMode, weightModeOf(opt.yWeight));
                //Y1 标题 = 数据形态模板（渲染时解析为 MXDatatype，如 VELOCITY/
                //Non Struct Disp；HyperGraph 复数图 Y1 同语义，经中缝标签位显示，
                //面板分量名 Phase/Magnitude 等由图表随格式派生，不占 Y1 标题）
                controller->setNodeProperty(childId, CurveProp::kTitle,
                    QStringLiteral("{%1.y.MXDatatype}").arg(curveRef));
                controller->setNodeProperty(childId, CurveProp::kTitleFont, opt.yFont);
                break;
            default:
                break;
            }
        }

        controller->undoStack()->endMacro();
        mCurveViewportFactory::getInstance()->clearPlacementOverride();
        //视口创建与 CurveTree 刷新由框架自动完成（视口工厂监听节点信号）
    }

    void mOpenCurveDlg::slot_createLine()
    {
        //入口兜底：任何异常（如 pch 数据缺失/格式异常）转为界面错误提示，不崩溃
        try
        {
        //批量建图期间抑制本弹窗的分量列表重填（kChartType 联动会 clear 列表，
        //把本函数持有的 QListWidgetItem 选集变成悬空指针）；RAII 兼顾异常路径
        struct CreatingChartsGuard {
            bool& _f;
            explicit CreatingChartsGuard(bool& f) : _f(f) { _f = true; }
            ~CreatingChartsGuard() { _f = false; }
        } creatingChartsGuard{ _creatingCharts };

        int subcaseid;
        QString yResult;

        //X 轴数据源选择：结果类型必选；为文件实际结果类型时还需 对象×分量
        //（文件解析出的横坐标类型 Time[Sec]/Frequency[Hz] 直接取记录横坐标）
        QString xResult;
        int xObjectId = 0;
        QString xComponent;

        //R22：当前文件数据源类型（csv/txt -> ascii 语义：无工况级、对象固定
        //Block 1、分量=列名，装配与来源五元组按 ascii 常量落模）
        const bool asciiFile = isAsciiCurrentFile();

        CoordinateAxisTitle title;

        int selectedIndex = m_selectBtn->currentIndex();//获取布局索引

        title.xFilePath = _currentlyFilePath;
        title.yFilePath = _currentlyFilePath;

        // 1. 获取完整的文件名（含后缀）
        QFileInfo fileInfo(_currentlyFilePath);
        QString fileName = fileInfo.fileName();

        title.xFileName = fileName;
        title.yFileName = fileName;

        // 布局
        QString layout = m_layoutCombo->currentText();

        // X轴结果类型列表
        if (m_xResultList->currentItem())
        {
            xResult = m_xResultList->currentItem()->text();
            title.xType = xResult;
        }
        else
        {
            mCurveDocController::getInstance()->reportWarning(QStringLiteral("请先选择X轴结果类型"));
            return;
        }

        //X 轴为文件实际结果类型时需选择 对象×分量
        if (!isAbscissaXResult(xResult))
        {
            if (m_xObjectList->currentItem())
            {
                title.xRequest = m_xObjectList->currentItem()->text();
                if (asciiFile)
                {
                    //ascii：对象文本为 "Block 1"（无 "Point id " 前缀可解析），
                    //装配 id 用 ascii 语义常量
                    xObjectId = mCurveDataSourceStore::asciiPointId();
                }
                else
                {
                    QString prefix = "Point id ";
                    xObjectId = title.xRequest.mid(prefix.length()).trimmed().toInt();
                }
            }
            else
            {
                mCurveDocController::getInstance()->reportWarning(QStringLiteral("请先选择X轴对象"));
                return;
            }

            if (m_xComponentList->currentItem())
            {
                xComponent = m_xComponentList->currentItem()->text();
                title.xComponent = xComponent;
            }
            else
            {
                mCurveDocController::getInstance()->reportWarning(QStringLiteral("请先选择X轴分量"));
                return;
            }
        }

        //Y轴工况列表（R22：ascii 文件无工况级——工况列隐藏，工况 id 用语义
        //常量，title.ySubcase/xSubcase 留空，{y.MXSubcase} 模板按"来源缺失
        //替换为空"既有规则解析）
        if (asciiFile)
        {
            subcaseid = mCurveDataSourceStore::asciiSubcaseId();
        }
        else if (m_ySubcaseList->currentItem())
        {
            QString str = m_ySubcaseList->currentItem()->text();
            subcaseid = subcaseNumberOf(str);   //只取编号词元（末尾 label 不参与）
            title.ySubcase = str;
            if (title.xSubcase == "")
            {
                title.xSubcase = str;
            }
        }
        else
        {
            mCurveDocController::getInstance()->reportWarning(QStringLiteral("请先选择Y轴工况"));
            return;
        }

        // Y轴结果类型列表
        if (m_yResultList->currentItem())
        {
            yResult = m_yResultList->currentItem()->text();
            title.yType = yResult;
        }
        else
        {
            mCurveDocController::getInstance()->reportWarning(QStringLiteral("请先选择Y轴结果类型"));
            return;
        }

        QList<QListWidgetItem*> selectedObjects = m_yObjectList->selectedItems();
        if (selectedObjects.isEmpty()) {
            mCurveDocController::getInstance()->reportWarning(QStringLiteral("请先选择对象"));
            return;
        }
        title.yRequest = selectedObjects[0]->text();

        QList<QListWidgetItem*> selectedComponents = m_yComponentList->selectedItems();
        if (selectedComponents.isEmpty()) {
            mCurveDocController::getInstance()->reportWarning(QStringLiteral("请先选择分量"));
            return;
        }
        title.yComponent = selectedComponents[0]->text();

        drawOption->setCoordinateAxisTitle(title);

        mCurveDocController* controller = mCurveDocController::getInstance();

        //目标图表类型快照（批次前取一次）：新建图表承接当前图表类型（1 柱状图/
        //2 复数图）——分量列表的方向合并显示（refreshYComponents）即以当前图表
        //为准，新建图表不承接同型，复数装配的 MAG/PHA 特征会落进 xy 图表成普通
        //曲线，且批次中后续装配因激活链切到新建图表而状态翻转。故快照后经参数
        //下传，不用 drawIntoComplexChart 实时查询
        int targetChartType = 0;
        {
            CurveDocument* doc = controller->document();
            CurveNode* curChart = doc
                ? doc->node(mCurveViewportFactory::getInstance()->currentChartId())
                : nullptr;
            if (curChart && curChart->type == CurveNodeType::Chart
                && (curChart->chart.chartType == 1 || curChart->chart.chartType == 2))
            {
                targetChartType = curChart->chart.chartType;
            }
        }

        //R22-B01：X 侧来源四元组补写参数（下传两条创建路径）。ascii 且本批
        //X 按所选分量列装配成功（buildSeriesHash 返回 true）才传 X 页所选列名；
        //空=不写——pch 横坐标形态语义保留、装配失败退行号保持未记录（页面
        //回显同现状），柱状/复数-缺侧等无 x 壳目标在创建侧再守
        auto xSideSourceArg = [asciiFile, &xComponent](bool xByColumn) -> QString
        {
            return (asciiFile && xByColumn) ? xComponent : QString();
        };

        //"使用当前绘图"且当前视口已承载本模块图表：新曲线并入该图表节点，
        //不新建页/视口（曲线树在该图表下自动长出曲线子节点，保证树与视口一致；
        //不覆盖建图时的页眉/轴设置；撤销只撤本批新增曲线；不平铺重排视口）
        if (m_layoutCombo->currentIndex() == 0)
        {
            int reuseChartId = mCurveViewportFactory::getInstance()->currentChartId();
            if (reuseChartId != -1)
            {
                QHash<QString, QVector<double>> XHash;
                QHash<QString, QVector<double>> YHash;
                const bool xByColumn = buildSeriesHash(selectedObjects, selectedComponents, subcaseid, yResult, XHash, YHash, xResult, xObjectId, xComponent, targetChartType);
                appendCurvesToChart(reuseChartId, XHash, YHash, subcaseid, yResult, xSideSourceArg(xByColumn));
                //出图后弹窗保持打开（可继续选通道连续出图），点"关闭"才收起
                return;
            }
        }

        //按布局指定视口放置策略："使用当前绘图"(索引0，当前视口为空/非曲线内容时)
        //复用当前视口不新建；其余布局各自新建 ViewportPost2D+名字 的视口（宏结束后在工厂还原默认）
        mCurveViewportFactory::getInstance()->setPlacementOverride(
            m_layoutCombo->currentIndex() == 0
            ? ChartPlacement::ReuseCurrent
            : ChartPlacement::CreateNew);
        
        controller->undoStack()->beginMacro(QStringLiteral("创建pch页"));
        //按页对两棵树分离对齐：一次"创建"=按布局格数拆成若干曲线页，图表依序
        //落页（每页至多一格数个）；每个曲线页经首图挂载与新建视图页 1:1 对齐
        //（页名回填"视图页-N"，两棵树同名成对）。页数量在此一次定死，之后切换
        //布局只做页内重排（主窗口同步不再增删页）；"使用当前绘图"时网格禁用、
        //按单视口单页落图
        const int comboIdx = m_layoutCombo->currentIndex();
        const int layoutIdx = (comboIdx == 0) ? 0 : selectedIndex;
        const int cells = qMax(1, curveViewportLayoutSlots().value(layoutIdx, 1));
        int chartTotal = 1;
        if (comboIdx == 1)
            chartTotal = selectedObjects.size();
        else if (comboIdx == 2)
            chartTotal = selectedComponents.size();
        else if (comboIdx == 3)
            chartTotal = selectedObjects.size() * selectedComponents.size();
        const int pageCount = qMax(1, (chartTotal + cells - 1) / cells);
        QList<int> pageIds;
        for (int i = 0; i < pageCount; ++i)
        {
            const int pid = controller->addPage();
            controller->setNodeProperty(pid, CurveProp::kLayout, layoutIdx);
            pageIds.append(pid);
        }
        int chartSeq = 0;
        auto pageForChart = [&]() -> int
        {
            return pageIds.at(qMin(chartSeq++ / cells, pageIds.size() - 1));
        };

        if (m_layoutCombo->currentIndex() == 0)
        {
            QHash<QString, QVector<double>> XHash;
            QHash<QString, QVector<double>> YHash;
            //数据装配统一走 buildSeriesHash（分量校验/缺虚部告警跳过等都在其中；
            //不要在调用前再手工填充 XHash/YHash，会造成数据点重复翻倍）
            const bool xByColumn = buildSeriesHash(selectedObjects, selectedComponents, subcaseid, yResult, XHash, YHash, xResult, xObjectId, xComponent, targetChartType);
            //并入曲线模块框架：节点入模型（数据/样式/来源），视口与树自动联动
            createChartFromData(pageForChart(), XHash, YHash, subcaseid, yResult, xResult, targetChartType,
                xSideSourceArg(xByColumn));
        }
        else if (comboIdx == 1)
        {
            //按对象分图：每对象一图（该对象的各分量同图多线）；
            //数据装配统一走 buildSeriesHash（分量校验/缺虚部防线等都在其中）
            for (auto pointid : selectedObjects)
            {
                QHash<QString, QVector<double>> XHash;
                QHash<QString, QVector<double>> YHash;
                const bool xByColumn = buildSeriesHash({ pointid }, selectedComponents, subcaseid, yResult, XHash, YHash, xResult, xObjectId, xComponent, targetChartType);
                //并入曲线模块框架：节点入模型（数据/样式/来源），视口与树自动联动
                createChartFromData(pageForChart(), XHash, YHash, subcaseid, yResult, xResult, targetChartType,
                    xSideSourceArg(xByColumn));
            }
        }
        else if (comboIdx == 2)
        {
            //按分量分图：每分量一图（各对象的该分量同图多线）
            for (auto component : selectedComponents)
            {
                QHash<QString, QVector<double>> XHash;
                QHash<QString, QVector<double>> YHash;
                const bool xByColumn = buildSeriesHash(selectedObjects, { component }, subcaseid, yResult, XHash, YHash, xResult, xObjectId, xComponent, targetChartType);
                //并入曲线模块框架：节点入模型（数据/样式/来源），视口与树自动联动
                createChartFromData(pageForChart(), XHash, YHash, subcaseid, yResult, xResult, targetChartType,
                    xSideSourceArg(xByColumn));
            }
        }
        else if (comboIdx == 3)
        {
            //单线单图：每 对象×分量 一图
            for (auto pointid : selectedObjects)
            {
                for (auto component : selectedComponents)
                {
                    QHash<QString, QVector<double>> XHash;
                    QHash<QString, QVector<double>> YHash;
                    const bool xByColumn = buildSeriesHash({ pointid }, { component }, subcaseid, yResult, XHash, YHash, xResult, xObjectId, xComponent, targetChartType);
                    //并入曲线模块框架：节点入模型（数据/样式/来源），视口与树自动联动
                    createChartFromData(pageForChart(), XHash, YHash, subcaseid, yResult, xResult, targetChartType,
                        xSideSourceArg(xByColumn));
                }
            }
        }

        controller->undoStack()->endMacro();

        //多页分块时激活链停在末块尾部（半空页），落点切回首页并激活其首张图表
        if (pageIds.size() > 1)
            mCurveViewportFactory::getInstance()->activatePageFirstChart(pageIds.first());

        VMManager->tileAllViewportModel(true);

        //出图后弹窗保持打开（可继续选通道连续出图），点"关闭"才收起
        }
        catch (const std::exception& e)
        {
            mCurveDocController::getInstance()->reportError(
                QStringLiteral("创建曲线失败：%1").arg(QString::fromUtf8(e.what())));
        }
        catch (...)
        {
            mCurveDocController::getInstance()->reportError(QStringLiteral("创建曲线失败：未知异常"));
        }
    }

    void mOpenCurveDlg::slot_openFile()
    {
        QString dir;
        QString filePath;

        if (!_currentlyFilePath.isEmpty())
            dir = QFileInfo(_currentlyFilePath).absolutePath();
        else
            dir = "C://";

        //R22：过滤器含 pch/csv/txt 三类（解析登记按 mCurveDataSourceStore::fileKind
        //分流，入口链路两类共用）
        filePath = QFileDialog::getOpenFileName(this, QString("打开文件"), dir,
            QString("数据文件(*.pch *.csv *.txt)"));

        if (filePath.isEmpty())
            return;

        int index = m_fileCombo->findText(filePath);

        if (index == -1)
        {
            _currentlyFilePath = filePath;
            //文件解析函数
            try
            {
            ParseFile(filePath);
            }
            catch (const std::exception& e)
            {
                mCurveDocController::getInstance()->reportError(
                    QStringLiteral("数据文件解析失败：%1").arg(QString::fromUtf8(e.what())));
                _currentlyFilePath.clear();
                return;
            }

            m_fileCombo->addItem(filePath);
            m_fileCombo->setCurrentIndex(m_fileCombo->findText(filePath));

        }
        else {
            _currentlyFilePath = filePath;
            m_fileCombo->setCurrentIndex(index);
        }
    }

    void mOpenCurveDlg::closeEvent(QCloseEvent* event)
    {
        hide();
    }

    void mOpenCurveDlg::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
    }

    //标题栏拖动（无边框窗口自绘标题栏，事件过滤器挂标题栏——构造函数
    //installEventFilter）。标题栏子控件未接受的事件（空白/徽标/标题文字）
    //会冒泡到标题栏进入本过滤器一并参与拖动；关闭按钮自身消费点击不触发
    bool mOpenCurveDlg::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == _titleBar)
        {
            switch (event->type())
            {
            case QEvent::MouseButtonPress:
            {
                auto* e = static_cast<QMouseEvent*>(event);
                if (e->button() == Qt::LeftButton)
                {
                    _dragging = true;
                    _dragOffset = e->globalPos() - frameGeometry().topLeft();
                    return true;   //消费按下（不改变标题栏子控件的悬停态）
                }
                break;
            }
            case QEvent::MouseMove:
            {
                auto* e = static_cast<QMouseEvent*>(event);
                if (_dragging && (e->buttons() & Qt::LeftButton))
                    move(e->globalPos() - _dragOffset);
                break;
            }
            case QEvent::MouseButtonRelease:
                _dragging = false;
                break;
            default:
                break;
            }
        }
        return QDialog::eventFilter(watched, event);
    }
}