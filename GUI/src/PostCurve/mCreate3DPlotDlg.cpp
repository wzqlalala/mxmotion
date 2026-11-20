#include "mCreate3DPlotDlg.h"
#include "m3DPlotDataParser.h"
#include "mDlgGeometry.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"

//引擎兜底读取（曲面/瀑布矩阵判定不成立时的扁平网格口径，R24 起）
#include <MapFileLoader.h>

//QT
#include <QApplication>
#include <QFileInfo>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>

namespace PostCurve
{
    mCreate3DPlotDlg* mCreate3DPlotDlg::_instance = nullptr;

    namespace
    {
        //回显框内路径的最大显示宽度（过长中段省略，tooltip 带完整路径）
        constexpr int kFileEditWidth = 380;

        //路径过长时的省略回显（中段省略保留文件名可辨识）
        QString elidedPath(const QString& path, int maxWidth)
        {
            const QFontMetrics fm = QApplication::fontMetrics();
            return fm.elidedText(path, Qt::ElideMiddle, maxWidth);
        }

        //std::vector<float> -> QVector<double>（MapFileLoader 扁平网格兜底的
        //产出为 float 序列；记录与模型统一 double 精度）
        QVector<double> toQVector(const std::vector<float>& values)
        {
            QVector<double> out;
            out.reserve(static_cast<int>(values.size()));
            for (const float v : values)
                out.push_back(static_cast<double>(v));
            return out;
        }
    }

    mCreate3DPlotDlg::mCreate3DPlotDlg(QWidget* parent)
        : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        buildUi();
        resize(520, 280);   //两行表单 + 提示行 + 按钮行的小体量弹窗
        //几何记忆（大小+位置，仅当次软件运行内）：构造末尾一行接入
        DlgGeom::bind(this, "Create3DPlotDlg");

        //本次运行无已存几何时打开居中于父窗口（有几何则 bind 已恢复位置，不再居中）
        _needCenter = !DlgGeom::hasSessionGeometry("Create3DPlotDlg");

        //窗口细边框：白底弹窗加描边保证轮廓可辨识。须用 objectName 选择器：
        //本类在 PostCurve 命名空间内，QSS 裸类名选择器按 metaObject className
        //（PostCurve::mCreate3DPlotDlg）匹配不上（同 mCurveStatisticsDlg）
        setObjectName("create3DPlotDlg");
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget#create3DPlotDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");
    }

    mCreate3DPlotDlg::~mCreate3DPlotDlg()
    {
        if (_instance == this)
            _instance = nullptr;
    }

    void mCreate3DPlotDlg::showInstance(QWidget* parent)
    {
        //父窗口须为真正的顶层主窗口：调用方传的 window() 在停靠/MDI 环境里可能
        //是承载弹窗请求的某个小面板——沿父链走到顶层；顶层不是 QMainWindow 时
        //兜底取应用主窗口（同 mOpenCurveDlg/mCurveStatisticsDlg 的取父策略）
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
            _instance = new mCreate3DPlotDlg(host);
        //本次运行首开居中于父窗口（主窗口）：无边框窗口无系统定位，缺省落点随环境
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

    void mCreate3DPlotDlg::buildUi()
    {
        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮（照 mCurveStatisticsDlg）
        _titleBar = new QWidget(this);
        _titleBar->setObjectName("create3DPlotTitleBar");
        _titleBar->setFixedHeight(52);
        _titleBar->setStyleSheet("QWidget#create3DPlotTitleBar{background:#FFFFFF;}");
        QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(3);
        QLabel* logoLabel = new QLabel(_titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("从文件新建三维图"), _titleBar);
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

        //主体：区块标签 + 文件行 + 类型行 + 提示行 + 底部按钮行（R30 起"目标"行
        //移除：三维图全文档唯一，无则新建/有则追加，定位收口在 slot_plot）
        QWidget* body = new QWidget(this);
        body->setStyleSheet("QWidget{background:#FFFFFF;}");
        QVBoxLayout* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(20, 6, 20, 16);
        bodyLayout->setSpacing(16);

        //区块标签"三维绘图"（参考图2：主题蓝小节标签）
        QLabel* sectionLabel = new QLabel(QStringLiteral("三维绘图"), body);
        sectionLabel->setStyleSheet(
            "QLabel{color:#1385F6; font-size:13px; font-weight:600; background:transparent;}");
        bodyLayout->addWidget(sectionLabel);

        const QString rowLabelStyle =
            "QLabel{color:#333333; font-size:13px; background:transparent;}";

        //行1：数据文件（只读回显过长省略 + "浏览..."）
        QHBoxLayout* fileRow = new QHBoxLayout();
        fileRow->setSpacing(12);
        QLabel* fileLabel = new QLabel(QStringLiteral("数据文件"), body);
        fileLabel->setStyleSheet(rowLabelStyle);
        fileLabel->setFixedWidth(64);
        fileRow->addWidget(fileLabel);
        _fileEdit = new QLineEdit(body);
        _fileEdit->setReadOnly(true);
        _fileEdit->setPlaceholderText(QStringLiteral("选择数据文件"));
        _fileEdit->setFixedWidth(kFileEditWidth);
        _fileEdit->setStyleSheet("QLineEdit{background:#FFFFFF;}");
        fileRow->addWidget(_fileEdit);
        QPushButton* browseBtn = new QPushButton(QStringLiteral("浏览..."), body);
        browseBtn->setAutoDefault(false);   //阻止回车键触发按钮
        fileRow->addWidget(browseBtn);
        fileRow->addStretch(1);
        connect(browseBtn, &QPushButton::clicked, this, &mCreate3DPlotDlg::slot_browseFile);
        bodyLayout->addLayout(fileRow);

        //行2：绘图类型（四单选，默认"曲线"；图标省略——样式不必对齐参考图）
        QHBoxLayout* typeRow = new QHBoxLayout();
        typeRow->setSpacing(0);
        QLabel* typeLabel = new QLabel(QStringLiteral("绘图类型"), body);
        typeLabel->setStyleSheet(rowLabelStyle);
        typeLabel->setFixedWidth(64);
        typeRow->addWidget(typeLabel);
        _waterfallRadio = new QRadioButton(QStringLiteral("瀑布图"), body);
        _lineRadio = new QRadioButton(QStringLiteral("曲线"), body);
        _surfaceRadio = new QRadioButton(QStringLiteral("曲面"), body);
        _barRadio = new QRadioButton(QStringLiteral("柱状图"), body);
        _lineRadio->setChecked(true);
        for (QRadioButton* radio : { _waterfallRadio, _lineRadio, _surfaceRadio, _barRadio })
        {
            radio->setStyleSheet("QRadioButton{color:#333333; font-size:13px; background:transparent;}");
        }
        typeRow->addWidget(_waterfallRadio);
        typeRow->addSpacing(24);
        typeRow->addWidget(_lineRadio);
        typeRow->addSpacing(24);
        typeRow->addWidget(_surfaceRadio);
        typeRow->addSpacing(24);
        typeRow->addWidget(_barRadio);
        typeRow->addStretch(1);
        bodyLayout->addLayout(typeRow);

        //提示行：默认灰字，校验失败/加载失败红字带原因
        _hintLabel = new QLabel(QStringLiteral("请选择数据文件和绘图类型。"), body);
        _hintLabel->setStyleSheet("QLabel{color:#A8ABB2; font-size:12px; background:transparent;}");
        bodyLayout->addWidget(_hintLabel);
        bodyLayout->addStretch(1);

        //底部右对齐：取消 + 绘图（主蓝主钮，44x26 同"从文件创建曲线"）
        QHBoxLayout* btnRow = new QHBoxLayout();
        btnRow->setSpacing(12);
        btnRow->addStretch();
        QPushButton* cancelBtn = new QPushButton(QStringLiteral("取消"), body);
        _plotBtn = new QPushButton(QStringLiteral("绘图"), body);
        _plotBtn->setObjectName("btnCreate3DPlot");
        cancelBtn->setFixedSize(44, 26);
        _plotBtn->setFixedSize(44, 26);
        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(_plotBtn);
        connect(cancelBtn, &QPushButton::clicked, this, &QWidget::close);
        connect(_plotBtn, &QPushButton::clicked, this, &mCreate3DPlotDlg::slot_plot);

        outer->addWidget(_titleBar);
        outer->addWidget(body, 1);
        outer->addLayout(btnRow);

        //主钮样式（本系统主钮规范：主蓝 #1385F6 / 44x26，同 mOpenCurveDlg btnCreate）
        _plotBtn->setStyleSheet(R"(
            QPushButton#btnCreate3DPlot{
                background-color:#1385F6;
                color:#FFFFFF;
                border:none;
                border-radius:4px;
                padding:0px;
            }
            QPushButton#btnCreate3DPlot:hover{
                background-color:#3D9EF8;
            }
            QPushButton#btnCreate3DPlot:pressed{
                background-color:#0F6ACB;
            }
        )");
    }

    void mCreate3DPlotDlg::slot_browseFile()
    {
        //过滤器对齐任务单口径："数据文件 (*.txt *.csv);;所有文件 (*.*)"
        const QString path = QFileDialog::getOpenFileName(this,
            QStringLiteral("选择数据文件"), QString(),
            QStringLiteral("数据文件 (*.txt *.csv);;所有文件 (*.*)"));
        if (path.isEmpty())
            return;   //取消选择不清空既有回显
        _filePath = path;
        //过长路径中段省略显示，完整路径挂 tooltip
        _fileEdit->setText(elidedPath(path, kFileEditWidth - 16));
        _fileEdit->setToolTip(path);
        //选好文件即恢复默认灰字提示（清掉上一次失败的红色遗留）
        _hintLabel->setText(QStringLiteral("请选择数据文件和绘图类型。"));
        _hintLabel->setStyleSheet("QLabel{color:#A8ABB2; font-size:12px; background:transparent;}");
    }

    bool mCreate3DPlotDlg::assemblePlotData(Plot3DRecord* rec, QString* err)
    {
        _plotNote.clear();
        //曲线类型序（对齐弹窗单选序：瀑布0/曲线1/曲面2/柱状3，即 series.plot3dType）
        const int plot3dType = _waterfallRadio->isChecked() ? 0
            : _lineRadio->isChecked() ? 1
            : _surfaceRadio->isChecked() ? 2 : 3;

        //宿主侧解析（T2 起数据口径）：复用 R22 mCurveAsciiReader 完成字节→
        //文本→切分（逗号/空白自适应、BOM/GBK、表头判定），形态判定后按类型
        //装配；曲面/瀑布图在矩阵表判定不成立或装配失败时回退 R23 的
        //MapFileLoader::loadFile 扁平网格（Plot3D 旧样例行为不回归）
        AsciiColumnFile parsed;
        if (!m3DPlotDataParser::readFile(_filePath, &parsed, err))
            return false;
        const bool isMatrix = m3DPlotDataParser::isMatrixTable(parsed);

        //矩阵装配含无效单元时记提示说明（slot_plot 入树后回显提示行）
        auto noteMatrixZeroFill = [this](const MatrixSeries3D& m) {
            if (m.zeroFilledCells > 0)
            {
                _plotNote = QStringLiteral("已按矩阵表装配 %1×%2 网格，%3 个无效单元按 0 处理。")
                    .arg(m.rows).arg(m.cols).arg(m.zeroFilledCells);
            }
        };

        try
        {
            if (plot3dType == 1)
            {
                //曲线：点列口径；矩阵表不入树（红字提示换类型）
                if (isMatrix)
                {
                    *err = QStringLiteral("该文件为矩阵表，请选择曲面/瀑布图/柱状图。");
                    return false;
                }
                PointSeries3D points;
                if (!m3DPlotDataParser::assemblePointSeries(parsed, &points, err))
                    return false;
                rec->plot3dType = 1;
                rec->xs = points.xs;
                rec->ys = points.ys;
                rec->zs = points.zs;
                return true;
            }
            if (plot3dType == 3)
            {
                if (isMatrix)
                {
                    //矩阵表柱状图：每格一根柱（22801 根量级，Plot3D 实例化渲染可承受）
                    MatrixSeries3D m;
                    if (!m3DPlotDataParser::assembleMatrixSeries(parsed, &m, err))
                        return false;
                    noteMatrixZeroFill(m);
                    rec->plot3dType = 3;
                    rec->xs = m.xs;
                    rec->ys = m.ys;
                    rec->zs = m.zs;
                    return true;
                }
                PointSeries3D points;
                if (!m3DPlotDataParser::assemblePointSeries(parsed, &points, err))
                    return false;
                rec->plot3dType = 3;
                rec->xs = points.xs;
                rec->ys = points.ys;
                rec->zs = points.zs;
                return true;
            }

            //曲面/瀑布图：矩阵表优先；判定不成立或装配失败回退 R23 扁平
            //网格口径（MapResult.txt 等 Plot3D 旧样例行为不变）
            MatrixSeries3D m;
            QString matrixErr;
            bool matrixOk = false;
            if (isMatrix)
            {
                matrixOk = m3DPlotDataParser::assembleMatrixSeries(parsed, &m, &matrixErr);
                //退化维度防线：曲面/瀑布消费方（Surface/Waterfall Series）与
                //MapData::isValid 同口径要求 rows/cols ≥2，仅 1 行/1 列的矩阵
                //视为装配失败 → 走下方扁平网格兜底（柱状图矩阵路径无此防线：
                //1 行矩阵=一排柱，语义有效）
                if (matrixOk && (m.rows < 2 || m.cols < 2))
                {
                    matrixErr = QStringLiteral("退化矩阵 %1×%2（曲面/瀑布要求至少 2×2）")
                        .arg(m.rows).arg(m.cols);
                    matrixOk = false;
                }
                if (matrixOk)
                    noteMatrixZeroFill(m);
            }

            if (!matrixOk)
            {
                //规则网格（loadFile，z 列默认第 3 列；MapData 口径）
                const MapData data = MapFileLoader::loadFile(_filePath, 2);
                if (!data.isValid())
                {
                    //两路都失败：红字带原因（矩阵路失败过则附其原因）
                    if (!matrixErr.isEmpty())
                    {
                        *err = QStringLiteral("文件解析失败：矩阵表装配失败（%1），"
                            "扁平网格兜底亦失败：非有效网格数据（每行至少 4 列数值，"
                            "同值 X 连续排列）。").arg(matrixErr);
                    }
                    else
                    {
                        *err = QStringLiteral("文件解析失败：非有效网格数据（每行至少 4 列数值，"
                            "同值 X 连续排列）。");
                    }
                    return false;
                }
                rec->plot3dType = plot3dType;   //2 曲面 / 0 瀑布
                rec->xs = toQVector(data.xs);
                rec->ys = toQVector(data.ys);
                rec->zs = toQVector(data.zs);
                rec->rows = data.rows;
                rec->cols = data.cols;
                return true;
            }

            rec->plot3dType = plot3dType;
            rec->xs = m.xs;
            rec->ys = m.ys;
            rec->zs = m.zs;
            rec->rows = m.rows;
            rec->cols = m.cols;
            return true;
        }
        catch (const std::exception& e)
        {
            //解析/装配异常：红字提示不入树（不产半成品节点）
            *err = QStringLiteral("加载失败：%1。").arg(QString::fromUtf8(e.what()));
            return false;
        }
        catch (...)
        {
            *err = QStringLiteral("加载失败：未知异常。");
            return false;
        }
    }

    void mCreate3DPlotDlg::slot_plot()
    {
        //前置校验：路径非空 + 文件存在（绘图类型单选恒有选中态，无需校验）
        if (_filePath.isEmpty())
        {
            _hintLabel->setText(QStringLiteral("请选择数据文件。"));
            _hintLabel->setStyleSheet("QLabel{color:#F56C6C; font-size:12px; background:transparent;}");
            return;
        }
        if (!QFileInfo::exists(_filePath))
        {
            _hintLabel->setText(QStringLiteral("数据文件不存在，请重新选择。"));
            _hintLabel->setStyleSheet("QLabel{color:#F56C6C; font-size:12px; background:transparent;}");
            return;
        }

        //按类型解析装配；失败红字带原因、不入树
        Plot3DRecord rec;
        QString err;
        if (!assemblePlotData(&rec, &err))
        {
            _hintLabel->setText(err);
            _hintLabel->setStyleSheet("QLabel{color:#F56C6C; font-size:12px; background:transparent;}");
            return;
        }

        //单视口定位（R30，目标行已移除）与命令栈写入（R28 入树）：结构变更
        //（图表/曲线/轴壳）走撤销宏，数据 set3DSeriesData 直写（宏外，与 pch
        //导入同口径）；视口卡片由视口工厂按 nodeAdded 自动承接（m3DChartView 渲染）
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller->document();
        QUndoStack* stack = controller->undoStack();
        const QString curveName = QFileInfo(_filePath).completeBaseName();

        //查全文档是否已有三维图表（chartType=5）：遍历 工程->页->图表，
        //全文档唯一、不限定页；存量多三维图会话不合并（R30 用户拍板），取首个命中
        int exist3DChart = -1;
        const int rootId = doc->rootNodeId();
        for (int pageId : doc->childNodeIds(rootId))
        {
            for (int chartId : doc->childNodeIds(pageId))
            {
                if (doc->nodeType(chartId) != CurveNodeType::Chart)
                    continue;
                if (doc->nodeProperty(chartId, CurveProp::kChartType).toInt() != 5)
                    continue;
                exist3DChart = chartId;
                break;
            }
            if (exist3DChart > 0)
                break;
        }

        int chartId = -1;
        int curveId = -1;
        if (exist3DChart <= 0)
        {
            //无三维图→新建（总是新建视图页，用户预期 20261110：一次新建一个新页，
            //页随宏创建后在曲线树/视图树均可见；视图页绑定经卡片请求 -1 由主
            //窗口新建并回填）
            stack->beginMacro(QStringLiteral("新建三维图"));
            const int pageId = controller->addPage();
            chartId = controller->add3DChart(pageId);
            if (chartId > 0)
                curveId = controller->add3DCurve(chartId, curveName);
            stack->endMacro();
        }
        else
        {
            //已有三维图→仅追加曲线到该图（曲线名=文件基名，同名允许既有口径；
            //用户删除三维图后再绘=此处重新走新建）
            chartId = exist3DChart;
            stack->beginMacro(QStringLiteral("追加三维曲线"));
            curveId = controller->add3DCurve(chartId, curveName);
            stack->endMacro();
        }
        if (chartId <= 0 || curveId <= 0)
        {
            _hintLabel->setText(QStringLiteral("三维图创建失败，请重试。"));
            _hintLabel->setStyleSheet("QLabel{color:#F56C6C; font-size:12px; background:transparent;}");
            return;
        }
        controller->set3DSeriesData(curveId, rec.xs, rec.ys, rec.zs,
            rec.plot3dType, rec.rows, rec.cols);

        //成功（装配含无效单元按 0 兜底等场景 _plotNote 非空）：提示行回显说明
        //且暂不关窗——让用户能看到"N 个无效单元按 0 处理"的原因说明，可关窗或
        //换文件再绘；无说明直接关窗
        if (!_plotNote.isEmpty())
        {
            _hintLabel->setText(_plotNote);
            _hintLabel->setStyleSheet("QLabel{color:#A8ABB2; font-size:12px; background:transparent;}");
            return;
        }
        close();
    }

    void mCreate3DPlotDlg::mousePressEvent(QMouseEvent* event)
    {
        //仅记录按下点：拖动延迟到 mouseMoveEvent 中移动超过系统拖动阈值后才
        //真正开始（双击微动不再挪动弹窗）
        if (event->button() == Qt::LeftButton && _titleBar->underMouse())
            _pressPos = event->globalPos();
        QWidget::mousePressEvent(event);
    }

    void mCreate3DPlotDlg::mouseMoveEvent(QMouseEvent* event)
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

    void mCreate3DPlotDlg::mouseReleaseEvent(QMouseEvent* event)
    {
        _dragging = false;   //松手结束拖动（双击的第二击不延续上一次拖动态）
        QWidget::mouseReleaseEvent(event);
    }

    void mCreate3DPlotDlg::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //双击不进入拖动链路：默认实现会转发 mousePressEvent 重新置位拖动状态，
        //配合手部微动会挪动弹窗；此处仅刷新阈值起算点
        if (event->button() == Qt::LeftButton)
            _pressPos = event->globalPos();
    }
}
