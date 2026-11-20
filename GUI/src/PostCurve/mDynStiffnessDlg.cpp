#include "mDynStiffnessDlg.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QFileDialog>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QApplication>
#include <QMouseEvent>
#include <QMainWindow>
#include <QUndoStack>
#include <cmath>
#include <exception>

#include "mImageGridComboBox.h"
#include "mDlgGeometry.h"
#include "mCurveDataSourceStore.h"
#include "mCurveImportCoordinator.h"
#include <memory>
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"
#include "CurveView/mCurveViewportFactory.h"
#include "PostCurveGlobal.h"
#include "mViewportModelManager.h"
#include "ThemeManager.h"   //自样式弹窗豁免标记（kSelfStyledProp）

namespace PostCurve
{
    namespace
    {
        const double kTwoPi = 6.283185307179586;   //2π（速度/加速度形式目标线系数）

        //图表页眉/页脚与页标题模板的曲线位置引用（编号规则同"从文件创建曲线"：
        //页序=根下 Page 序、视口序=页下 Chart 序、首条曲线 c1，渲染时按引用
        //曲线解析来源五元组，见 mCurveChartView）
        QString firstCurveRefOf(CurveDocument* doc, int pageId, int chartId)
        {
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
            const int rootId = doc->rootNodeId();
            const QString ref = QStringLiteral("p%1w%2c1")
                .arg(pageId > 0 ? positionAmongSameType(rootId, pageId) : 1)
                .arg(positionAmongSameType(pageId, chartId));
            return ref;
        }
    }

    mDynStiffnessDlg* mDynStiffnessDlg::m_instance = nullptr;

    mDynStiffnessDlg* mDynStiffnessDlg::getInstance(QWidget* parent)
    {
        if (!m_instance)
            m_instance = new mDynStiffnessDlg(parent);
        return m_instance;
    }

    void mDynStiffnessDlg::showInstance(QWidget* parent)
    {
        //父窗口沿父链走到顶层主窗口（同 mOpenCurveDlg::showInstance：调用方传的
        //window() 在停靠/MDI 环境里可能是小面板，直接作父会居中到面板上）
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
        mDynStiffnessDlg* dlg = getInstance(host);
        //本次运行首开居中于父窗口（有会话几何则 bind 已恢复位置，不再居中）
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
    }

    mDynStiffnessDlg::mDynStiffnessDlg(QWidget* parent)
        //与曲线页面并行操作：置顶避免点击页面后被原生视口覆盖（同 mOpenCurveDlg）
        : QDialog(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        //自样式弹窗：豁免 ThemeManager 整窗重发，保留自带设计稿 QSS
        setProperty(MechMainWindow::ThemeManager::kSelfStyledProp, true);
        resize(640, 340);
        DlgGeom::bind(this, "DynStiffnessDlg");
        _needCenter = !DlgGeom::hasSessionGeometry("DynStiffnessDlg");
        setAutoFillBackground(true);
        //窗口细边框+objectName 选择器（PostCurve 命名空间内 QSS 裸类名匹配不上）
        setObjectName("dynStiffnessDlg");
        setAttribute(Qt::WA_StyledBackground, true);
        buildUi();
    }

    void mDynStiffnessDlg::buildUi()
    {
        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动，事件过滤在 eventFilter；样式同 mOpenCurveDlg）
        QWidget* titleBar = new QWidget(this);
        titleBar->setObjectName("DynStiffnessTitleBar");
        titleBar->setFixedHeight(52);
        titleBar->setStyleSheet(
            "QWidget#DynStiffnessTitleBar{background:#FFFFFF;border: none;}");
        QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(20, 20, 20, 10);
        titleLayout->setSpacing(8);
        QLabel* logoLabel = new QLabel(titleBar);
        logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
            .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        QLabel* titleLabel = new QLabel(QStringLiteral("动刚度曲线"), titleBar);
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

        setStyleSheet(R"(
            QWidget#dynStiffnessDlg{border:1px solid #D9DCE0;}
            QDialog{
                background-color:#FFFFFF;
            }
            QLineEdit{
                background-color:#FFFFFF;
                border:1px solid #D9DCE0;
                border-radius:4px;
                padding:3px 8px;
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

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setContentsMargins(20, 24, 20, 20);
        mainLayout->setSpacing(18);

        //行 1：导入结果文件（设计稿：弹出对话框选择 pch 格式的结果文件，作为数据来源）
        QHBoxLayout* resultRow = new QHBoxLayout();
        resultRow->setSpacing(12);
        QLabel* resultLabel = new QLabel(QStringLiteral("导入结果文件"));
        resultLabel->setFixedWidth(88);
        _resultEdit = new QLineEdit();
        _resultEdit->setPlaceholderText(QStringLiteral("pch 格式的结果文件"));
        resultRow->addWidget(resultLabel);
        resultRow->addWidget(_resultEdit, 1);
        QPushButton* resultBrowseBtn = new QPushButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/打开文件.svg")), QString());
        resultBrowseBtn->setToolTip(QStringLiteral("打开 pch 结果文件"));
        resultBrowseBtn->setAutoDefault(false);   //阻止回车键触发按钮
        resultBrowseBtn->setFixedSize(24, 24);    //图标按钮统一规格（同"从文件创建曲线"）
        resultRow->addWidget(resultBrowseBtn);
        mainLayout->addLayout(resultRow);

        //行 2：导入配置文件（设计稿：弹出对话框选择上一步编辑好的 csv 格式的
        //配置文件，作为模板）
        QHBoxLayout* configRow = new QHBoxLayout();
        configRow->setSpacing(12);
        QLabel* configLabel = new QLabel(QStringLiteral("导入配置文件"));
        configLabel->setFixedWidth(88);
        _configEdit = new QLineEdit();
        _configEdit->setPlaceholderText(QStringLiteral("模板配置生成的 csv 格式配置文件"));
        configRow->addWidget(configLabel);
        configRow->addWidget(_configEdit, 1);
        QPushButton* configBrowseBtn = new QPushButton(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/打开文件.svg")), QString());
        configBrowseBtn->setToolTip(QStringLiteral("打开 csv 配置文件"));
        configBrowseBtn->setAutoDefault(false);
        configBrowseBtn->setFixedSize(24, 24);
        configRow->addWidget(configBrowseBtn);
        mainLayout->addLayout(configRow);

        //行 3：动刚度形式（设计稿：导入配置文件后自动识别，只读回显）
        QHBoxLayout* formRow = new QHBoxLayout();
        formRow->setSpacing(12);
        QLabel* formLabel = new QLabel(QStringLiteral("动刚度形式"));
        formLabel->setFixedWidth(88);
        _formValue = new QLabel(QStringLiteral("--"));
        _formValue->setStyleSheet("QLabel{color:#333333; background:transparent;}");
        formRow->addWidget(formLabel);
        formRow->addWidget(_formValue);
        formRow->addStretch(1);
        mainLayout->addLayout(formRow);

        //行 4：窗口布局（设计稿：网格选择，默认 2×3 六格对齐设计稿图表栅格）
        QHBoxLayout* layoutRow = new QHBoxLayout();
        layoutRow->setSpacing(12);
        QLabel* layoutLabel = new QLabel(QStringLiteral("窗口布局"));
        layoutLabel->setFixedWidth(88);
        _layoutGrid = new MechMainWindow::mImageGridComboBox();
        _layoutGrid->setFixedSize(24, 24);
        _layoutGrid->setIconSize(QSize(18, 18));
        //视口布局图标与页面属性"页面布局"共用一份路径表（PostCurveGlobal.h）
        _layoutGrid->setImages(curveViewportIconPaths(), 4);
        _layoutGrid->setCurrentIndex(12);   //默认 6@上3下3
        layoutRow->addWidget(layoutLabel);
        layoutRow->addWidget(_layoutGrid);
        layoutRow->addStretch(1);
        mainLayout->addLayout(layoutRow);

        mainLayout->addStretch(1);

        //底部按钮区（关闭 + 创建；两份文件就绪前创建钮置灰）
        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(12);
        btnLayout->addStretch();
        QPushButton* closeBtn = new QPushButton(QStringLiteral("关闭"));
        _createBtn = new QPushButton(QStringLiteral("创建"));
        _createBtn->setObjectName("btnCreate");
        closeBtn->setFixedSize(44, 26);
        _createBtn->setFixedSize(44, 26);
        _createBtn->setEnabled(false);
        btnLayout->addWidget(closeBtn);
        btnLayout->addWidget(_createBtn);
        mainLayout->addLayout(btnLayout);

        outer->addLayout(mainLayout);

        connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
        connect(resultBrowseBtn, &QPushButton::clicked, this, &mDynStiffnessDlg::slot_browseResultFile);
        connect(configBrowseBtn, &QPushButton::clicked, this, &mDynStiffnessDlg::slot_browseConfigFile);
        connect(_createBtn, &QPushButton::clicked, this, &mDynStiffnessDlg::slot_create);
    }

    void mDynStiffnessDlg::updateCreateButtonState()
    {
        _createBtn->setEnabled(!_pchFilePath.isEmpty() && !_rows.isEmpty());
    }

    bool mDynStiffnessDlg::eventFilter(QObject* watched, QEvent* event)
    {
        //标题栏拖动（模式同 mOpenCurveDlg：按下记偏移，移动跟随，释放收尾）
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

    void mDynStiffnessDlg::slot_browseResultFile()
    {
        QString dir = _resultEdit->text().trimmed();
        dir = dir.isEmpty() ? QString() : QFileInfo(dir).absolutePath();

        const QString filePath = QFileDialog::getOpenFileName(
            this, QStringLiteral("打开文件"), dir, QStringLiteral("PCH File(*.pch)"));
        if (filePath.isEmpty())
            return;

        //解析并登记到共享数据源仓库（模板配置已导入过则幂等直取缓存）；
        //导入异步化（FR-01）：路径框即时回显，解析成败经 sig_importFinished 续接
        _resultEdit->setText(filePath);
        _pchFilePath.clear();

        //Accepted/DuplicateInFlight：解析在后台执行，成败经信号续接。接收上下文
        //=this（C3：解析中可关本对话框——D5，Qt 随对象销毁自动断连防悬垂）；
        //按键匹配防多导入串扰；命中后自断连。
        //协调器须在 addFileAsync 之前就位（否则首个 sig_importStarted 无订阅方）
        mCurveImportCoordinator* coordinator = mCurveImportCoordinator::getInstance(window());
        auto* store = mCurveDataSourceStore::getInstance();
        const auto accepted = store->addFileAsync(filePath);
        if (accepted == mCurveDataSourceStore::AddFileAsyncResult::AlreadyRegistered)
        {
            //已登记幂等成功：走原内联成功链路（逐字保留，C1）
            _pchFilePath = mCurveDataSourceStore::normalizePath(filePath);
            updateCreateButtonState();
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
                    _pchFilePath = key;   //键已规范化（与原实现一致）
                }
                else
                {
                    //原失败分支（文案/控件行为逐字保留）
                    mCurveDocController::getInstance()->reportError(
                        QStringLiteral("[曲线] pch 解析失败：%1").arg(err));
                    _resultEdit->clear();
                }
                updateCreateButtonState();
            });
    }

    void mDynStiffnessDlg::slot_browseConfigFile()
    {
        QString dir = _configEdit->text().trimmed();
        dir = dir.isEmpty() ? QString() : QFileInfo(dir).absolutePath();

        const QString filePath = QFileDialog::getOpenFileName(
            this, QStringLiteral("打开文件"), dir, QStringLiteral("CSV File(*.csv)"));
        if (filePath.isEmpty())
            return;

        _configEdit->setText(filePath);
        _configFilePath.clear();
        _rows.clear();
        _formValue->setText(QStringLiteral("--"));

        CurveTemplateKind kind = CurveTemplateKind::DynStiffness;
        DynStiffForm form = DynStiffForm::Displacement;
        QList<DynStiffRow> rows;
        QString err;
        //读失败 或 模板类型不是动刚度（VTF/NTF 模板里没有动刚度参数）都走设计稿
        //失败分支：模态提示具体原因（动刚度只有 位移/速度/加速度 形式，文案词
        //先归一化，避免拼出"无动刚度声压参数"这类不通顺措辞）
        if (!readMappingFile(filePath, kind, form, rows, &err)
            || kind != CurveTemplateKind::DynStiffness)
        {
            if (form == DynStiffForm::SoundPressure || form == DynStiffForm::SoundPressureIpi)
                form = DynStiffForm::Displacement;
            MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("提示"),
                QStringLiteral("创建曲线失败，导入的配置文件中无动刚度%1参数。"
                    "请检查文件和设置之后，再尝试重新创建。").arg(dynStiffFormText(form)),
                QStringLiteral("知道了"));
            updateCreateButtonState();
            return;
        }

        _configFilePath = filePath;
        _form = form;
        _rows = rows;
        _formValue->setText(dynStiffFormText(form));
        updateCreateButtonState();
    }

    void mDynStiffnessDlg::slot_create()
    {
        //入口兜底：任何异常（如 pch 数据缺失/格式异常）转为界面错误提示，不崩溃
        try
        {
        mCurveDataSourceStore* store = mCurveDataSourceStore::getInstance();
        const PchReader::PchFile* file = store->file(_pchFilePath);
        if (!file || _rows.isEmpty())
            return;
        mCurveDocController* controller = mCurveDocController::getInstance();

        //逐行装配实测数据与目标值数据（装配/计算失败行经输出窗口告警跳过；
        //全部失败走设计稿模态提示）
        struct RowCurves
        {
            DynStiffRow row;
            QVector<double> x;        //频率
            QVector<double> y;        //实测
            QVector<double> targetY;  //目标值
            QString schema;           //块 schemaName（=结果类型文本）
        };
        QList<RowCurves> built;
        bool targetFallbackWarned = false;
        for (const DynStiffRow& row : _rows)
        {
            //定位数据块：结果类型 + 工况 + 对象（位移/速度/加速度 FRF 块每
            //工况×对象一块，6 自由度按分量下标取值）
            const PchReader::ResultType type = dynStiffFormResultType(_form);
            const PchReader::PchResultBlock* blk =
                PchReader::findBlock(*file, row.subcaseId, row.pointId, type);
            if (!blk)
            {
                controller->reportWarning(QStringLiteral("[曲线] 动刚度曲线：配置行 %1（%2）在结果文件中无匹配数据块，已跳过")
                    .arg(row.objectText, row.componentText));
                continue;
            }

            //通道装配（分量令牌 MAG|X-Trans 等，与 store 展开规则同一套命名）
            mCurveDataSourceStore::ChannelDataResult data;
            QString chanErr;
            const QString schema = QString::fromStdString(blk->schemaName);
            //实测曲线取通道幅值（随块数值格式分流：MAG / 复数模 / REAL 原值；
            //实部/虚部格式若只取单路，动刚度 1/实部 会在实部过零点爆尖峰）
            const QString direction = row.componentText.section(QLatin1Char('|'), 1);
            if (!mCurveDataSourceStore::assembleChannelMagnitude(*file, row.subcaseId, schema,
                row.pointId, direction, &data, &chanErr)
                || data.xData.isEmpty() || data.yData.isEmpty())
            {
                controller->reportWarning(QStringLiteral("[曲线] 动刚度曲线：通道 %1 - %2 装配失败（%3），已跳过")
                    .arg(row.objectText, row.componentText, chanErr.isEmpty() ? QStringLiteral("无有效数据点") : chanErr));
                continue;
            }

            //目标值数值（"10000N/mm" -> 10000；解析失败按默认参数兜底并告警一次）
            double target = 0.0;
            if (!parseTargetValue(row.targetText, target) || target == 0.0)
            {
                target = 10000.0;
                if (!targetFallbackWarned)
                {
                    controller->reportWarning(QStringLiteral("[曲线] 动刚度曲线：目标值“%1”无法解析，按默认参数 %2 处理")
                        .arg(row.targetText, dynStiffDefaultTargetText()));
                    targetFallbackWarned = true;
                }
            }

            RowCurves out;
            out.row = row;
            out.x = data.xData;
            out.schema = schema;
            const QVector<double>& h = data.yData;
            const int n = h.size();
            if (_form == DynStiffForm::Displacement)
            {
                //实测 kd = 1/|H|（动刚度 Ka=F(ω)/x(ω)，|H| 为位移导纳幅值）；
                //幅值过零点防除零置 0
                out.y.resize(n);
                for (int i = 0; i < n; ++i)
                    out.y[i] = (std::abs(h.at(i)) > 1e-12) ? 1.0 / h.at(i) : 0.0;
                //目标 = 常数 K（水平目标线）
                out.targetY.fill(target, n);
            }
            else if (_form == DynStiffForm::Velocity)
            {
                //实测 = H 原样（Velocity Mobility = v(ω)/F(ω)）
                out.y = h;
                //目标 = 2πf/K
                out.targetY.resize(n);
                for (int i = 0; i < n; ++i)
                    out.targetY[i] = kTwoPi * data.xData.at(i) / target;
            }
            else
            {
                //实测 = H 原样（IPI = a(ω)/F(ω)）
                out.y = h;
                //目标 = (2πf)²/K
                out.targetY.resize(n);
                for (int i = 0; i < n; ++i)
                {
                    const double w = kTwoPi * data.xData.at(i);
                    out.targetY[i] = w * w / target;
                }
            }
            //剔除非正/非有限样本：对数轴下 log(0)=-inf 会让 Qwt 折线绘制
            //进入病态路径（卡死）；物理上这些点（DC 点/防除零置 0 点）在
            //对数图上也无意义。目标线与实测同下标同步过滤
            {
                QVector<double> fx, fy, fty;
                fx.reserve(out.x.size());
                fy.reserve(out.y.size());
                fty.reserve(out.targetY.size());
                for (int i = 0; i < out.x.size(); ++i)
                {
                    const double yv = out.y.at(i);
                    if (!qIsFinite(yv) || yv <= 0.0)
                        continue;
                    fx.append(out.x.at(i));
                    fy.append(yv);
                    if (!out.targetY.isEmpty())
                        fty.append(out.targetY.at(i));
                }
                out.x = fx;
                out.y = fy;
                out.targetY = fty;
                if (out.x.isEmpty())
                {
                    controller->reportWarning(QStringLiteral("[曲线] 动刚度曲线：通道 %1 - %2 无正值数据点，已跳过")
                        .arg(row.objectText, row.componentText));
                    continue;
                }
            }
            
            built.append(out);
        }

        if (built.isEmpty())
        {
            MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("提示"),
                QStringLiteral("创建曲线失败，导入的配置文件中无动刚度%1参数。"
                    "请检查文件和设置之后，再尝试重新创建。").arg(dynStiffFormText(_form)),
                QStringLiteral("知道了"));
            return;
        }

        //按布局格数分页落图（一图一行；页机制同"从文件创建曲线"：每页至多
        //一格数个图表，页设 kLayout 属性，视口创建由框架联动）
        mCurveViewportFactory* factory = mCurveViewportFactory::getInstance();
        factory->setPlacementOverride(ChartPlacement::CreateNew);
        //懒加载批量建卡：登记不切页不激活（落点页由收尾 activatePageFirstChart
        //统一切换并物化），数百图导入不再逐卡触发切页/物化
        factory->setDeferCardActivation(true);

        const int layoutIdx = _layoutGrid->currentIndex();
        const int cells = qMax(1, curveViewportLayoutSlots().value(layoutIdx, 1));
        const int pageCount = qMax(1, (built.size() + cells - 1) / cells);

        //轮换配色（共享定义见 PostCurveGlobal.h：curveDefaultColorWheel）；
        //目标值曲线固定取 0 号色（红），实测从 1 号轮换避免同图同色
        const QVector<QColor> kColors = curveDefaultColorWheel();

        controller->undoStack()->beginMacro(QStringLiteral("动刚度曲线-生成曲线"));
        QList<int> pageIds;
        for (int i = 0; i < pageCount; ++i)
        {
            const int pid = controller->addPage();
            controller->setNodeProperty(pid, CurveProp::kLayout, layoutIdx);
            pageIds.append(pid);
        }

        int chartSeq = 0;
        for (const RowCurves& rc : built)
        {
            const int pageId = pageIds.at(qMin(chartSeq / cells, pageIds.size() - 1));
            const int chartId = controller->addChart(pageId);
            ++chartSeq;
            if (chartId < 0)
                continue;

            //实测曲线（名=对象-分量，同树上"Point id 1001 - MAG | X-Trans"形态）
            const QString measuredName = rc.row.objectText + QStringLiteral(" - ") + rc.row.componentText;
            const int measuredId = controller->addCurve(chartId, measuredName);
            if (measuredId >= 0)
            {
                controller->setSeriesData(measuredId, rc.x, rc.y);
                controller->setSeriesSource(measuredId, _pchFilePath, rc.row.subcaseText,
                    rc.schema, rc.row.objectText, rc.row.componentText);
                //类别打标（曲线统计按类别过滤；经命令栈，随本撤销宏整体撤销/重做）
                controller->setCurveCategory(measuredId, CurveCategory::DynamicStiffness);
                controller->setNodeProperty(measuredId, CurveProp::kColor,
                    kColors.at(1 + chartSeq % (kColors.size() - 1)));
                controller->setNodeProperty(measuredId, CurveProp::kLineWidth, 2.0);
            }

            //目标值曲线（名=目标值文本，如"10000N/mm"）
            const int targetId = controller->addCurve(chartId, rc.row.targetText);
            if (targetId >= 0)
            {
                controller->setSeriesData(targetId, rc.x, rc.targetY);
                controller->setCurveCategory(targetId, CurveCategory::TargetCurve);
                controller->setNodeProperty(targetId, CurveProp::kColor, kColors.at(0));
                controller->setNodeProperty(targetId, CurveProp::kLineWidth, 2.0);
            }

            //页标题/图表页眉模板（Y 侧来源占位符渲染时按引用曲线解析，同
            //"从文件创建曲线"；纵轴标题按设计稿由用户手动添加，不自动设置）
            CurveDocument* doc = controller->document();
            const QString curveRef = firstCurveRefOf(doc, pageId, chartId);
            controller->setNodeProperty(pageId, CurveProp::kPageTitle,
                QStringLiteral("{%1.y.MXDatatype}").arg(curveRef));
            controller->setNodeProperty(chartId, CurveProp::kHeaderVisible, true);
            controller->setNodeProperty(chartId, CurveProp::kHeaderText,
                QStringLiteral("{%1.y.MXRequest} - {%1.y.MXComponent}").arg(curveRef));
            controller->setNodeProperty(chartId, CurveProp::kHeaderAlign, 0);
            controller->setNodeProperty(chartId, CurveProp::kFooterVisible, false);
            controller->setNodeProperty(chartId, CurveProp::kFooterText,
                QStringLiteral("{%1.y.MXRequest} - {%1.y.MXComponent}").arg(curveRef));
            controller->setNodeProperty(chartId, CurveProp::kFooterAlign, 0);
        }
        controller->undoStack()->endMacro();

        factory->clearPlacementOverride();
        factory->setDeferCardActivation(false);

        //懒加载批量建卡：收尾统一切到落点页并物化其首图（单页批次同样需要——
        //defer 模式下登记全程不切页不物化，没有这次收尾用户看到的是空占位卡片）
        factory->activatePageFirstChart(pageIds.first());

        VMManager->tileAllViewportModel(true);

        controller->reportInfo(QStringLiteral("[曲线] 动刚度%1曲线已生成：%2 张图表")
            .arg(dynStiffFormText(_form)).arg(built.size()));

        this->hide();
        }
        catch (const std::exception& e)
        {
            //异常路径重置批量建卡/放置策略（正常路径在 try 尾清理，异常不得泄漏）
            auto* factory = mCurveViewportFactory::getInstance();
            factory->setDeferCardActivation(false);
            factory->clearPlacementOverride();
            mCurveDocController::getInstance()->reportError(
                QStringLiteral("创建曲线失败：%1").arg(QString::fromUtf8(e.what())));
        }
        catch (...)
        {
            auto* factory = mCurveViewportFactory::getInstance();
            factory->setDeferCardActivation(false);
            factory->clearPlacementOverride();
            mCurveDocController::getInstance()->reportError(QStringLiteral("创建曲线失败：未知异常"));
        }
    }
}
