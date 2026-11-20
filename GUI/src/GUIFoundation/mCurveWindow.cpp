#include "mCurveWindow.h"
#include "mGlobalSignals.h"
#include "UiPalette.h"
#include "UiMetrics.h"
#include "mMechGlobalSignals.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QStyle>
#include <cmath>

#include "mPlotEnum.h"
#include "mPlotView.h"
#include "mPlotViewStyleWidget.h"
#include "UiPalette.h"	//曲线窗分割条把手底/中线/折叠按钮取语义色板（规则 0.2）
#include <QDebug>
#include <QSplitter>

namespace MechMainWindow
{
    class CurveSplitterHandle : public QSplitterHandle
    {
        Q_OBJECT
    public:
        CurveSplitterHandle(Qt::Orientation orientation, QSplitter* parent)
            : QSplitterHandle(orientation, parent), m_btn(nullptr), m_hover(false)
        {
            setMouseTracking(true);

            m_btn = new QToolButton(this);
            m_btn->hide();
            m_btn->setFixedSize(12, 50);
            m_btn->setText(">");
            m_btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
            applyTheme(); // 初始主题色（底色/边线/文字全部取色板）
            m_btn->installEventFilter(this);

            connect(m_btn, &QToolButton::clicked, this, [this]() {
                QSplitter* s = splitter();
                if (!s) return;

                int total = s->width() - s->handleWidth();
                QList<int> sizes = s->sizes();
                if (sizes.size() < 2) return;
                int right = sizes[1];

                const int LEFT_MIN = 144;
                const int MIDDLE = 800;

                int leftTarget, rightTarget;

                if (right <= 50) {
                    // 右侧已收起，回到中间
                    leftTarget = qMin(MIDDLE, total - LEFT_MIN);
                    rightTarget = total - leftTarget;
                }
                else {
                    // 中间状态，收起右侧
                    leftTarget = total;
                    rightTarget = 0;
                }

                s->setSizes({ leftTarget, rightTarget });
                updateArrow();
                });

            if (parent) {
                connect(parent, &QSplitter::splitterMoved, this, &CurveSplitterHandle::updateArrow);
            }

            updateArrow();
        }

        void updateArrow()
        {
            QSplitter* s = splitter();
            if (!s || !m_btn) return;

            QList<int> sizes = s->sizes();
            if (sizes.size() < 2) return;

            int right = sizes[1];
            int btnW = m_btn->width();
            int lineW = 2;
            int lineRightX = width() / 2 + lineW / 2;
            int lineLeftX = width() / 2 - lineW / 2;

            if (right <= 50) {
                m_btn->setText("<");
                m_btn->move(lineLeftX - btnW, m_btn->y());
            }
            else {
                m_btn->setText(">");
                m_btn->move(lineRightX, m_btn->y());
            }
            applyTheme(); // 按钮朝向变化时重发（边线左右侧不同）
        }

        /*
        * 主题重发（规则 0.2）：把手底 = header、中线 = border（悬停 primary）、
        * 折叠按钮 = surface 底 / border 边 / text 字，悬停 hover、按下 selected。
        * 原写死 #FFFFFF/#A0A0A0/#D0D0D0/#3A8DE8/#D0E7FD 全部移除（与信息输出窗
        * CollapsibleSplitterHandle 同一批收口；此前深/护眼下把手为固定白条不随主题）。
        * 由 mDockLayoutManager::applyTheme() 经元方法统一驱动，构造时自调一次。幂等。
        */
        Q_INVOKABLE void applyTheme()
        {
            if (!m_btn) return;
            using namespace MxFunctions;
            QSplitter* s = splitter();
            bool collapsed = false;
            if (s)
            {
                QList<int> sizes = s->sizes();
                if (sizes.size() >= 2)
                    collapsed = sizes[1] <= 50;
            }
            // 收起态按钮在线左侧贴右缘；展开态按钮在线右侧贴左缘
            const QString edge = collapsed
                ? QStringLiteral("border-right:none; border-left:1px solid %2;")
                : QStringLiteral("border-left:none; border-right:1px solid %2;");
            m_btn->setStyleSheet(QStringLiteral(
                "QToolButton {"
                "  background-color: %1;"
                "  border-top: 1px solid %2;"
                "  border-bottom: 1px solid %2;"
                "  %4"
                "  border-radius: 3px;"
                "  color: %3;"
                "  font-weight: bold;"
                "}"
                "QToolButton:hover { background-color: %5; }"
                "QToolButton:pressed { background-color: %6; }"
            ).arg(UiPalette::surface().name())
             .arg(UiPalette::border().name())
             .arg(UiPalette::text().name())
             .arg(edge)
             .arg(UiPalette::hover().name())
             .arg(UiPalette::selected().name()));
            update();
        }

    protected:
        bool eventFilter(QObject* obj, QEvent* ev) override
        {
            if (obj == m_btn && ev->type() == QEvent::Enter) {
                if (m_btn) m_btn->show();
            }
            return QSplitterHandle::eventFilter(obj, ev);
        }

        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            QRect r = rect();
            p.fillRect(r, MxFunctions::UiPalette::header());

            int lineWidth = 2;
            int offset = (r.width() - lineWidth) / 2;
            int margin = 5;
            QRect lineRect(offset, margin, lineWidth, r.height() - 2 * margin);
            p.fillRect(lineRect, m_hover ? MxFunctions::UiPalette::primary()
                                         : MxFunctions::UiPalette::border());
        }

        void resizeEvent(QResizeEvent* event) override
        {
            QSplitterHandle::resizeEvent(event);
            if (m_btn) {
                int btnH = m_btn->height();
                m_btn->move(m_btn->x(), (height() - btnH) / 2);
            }
            updateArrow();
        }

        void enterEvent(QEvent* event) override
        {
            m_hover = true;
            if (m_btn) m_btn->show();
            update();
            QSplitterHandle::enterEvent(event);
        }

        void leaveEvent(QEvent* event) override
        {
            m_hover = false;
            QTimer::singleShot(50, this, [this]() {
                if (!m_hover && m_btn && !m_btn->underMouse()) m_btn->hide();
                });
            update();
            QSplitterHandle::leaveEvent(event);
        }

    private:
        QToolButton* m_btn;
        bool m_hover;
    };


    class CurveSplitter : public QSplitter
    {
    public:
        explicit CurveSplitter(QWidget* parent = nullptr)
            : QSplitter(Qt::Horizontal, parent)
        {
            setHandleWidth(30);
            setStyleSheet("QSplitter::handle { background: transparent; }");
            setCollapsible(0, false);

            connect(this, &QSplitter::splitterMoved, this, [this](int pos, int index) {
                Q_UNUSED(pos);
                Q_UNUSED(index);
                QList<int> sizes = this->sizes();
                if (sizes.size() >= 2) {
                    bool changed = false;
                    const int MIN_W = 144;
                    if (sizes[0] < MIN_W) {
                        sizes[0] = MIN_W;
                        changed = true;
                    }
                    if (sizes[1] < MIN_W) {
                        int need = MIN_W - sizes[1];
                        sizes[1] = MIN_W;
                        sizes[0] = qMax(MIN_W, sizes[0] - need);
                        changed = true;
                    }
                    if (changed) setSizes(sizes);
                }
                for (int i = 0; i < count() - 1; ++i) {
                    CurveSplitterHandle* h = static_cast<CurveSplitterHandle*>(handle(i));
                    if (h) h->updateArrow();
                }
                });
        }

        QSplitterHandle* createHandle() override
        {
            return new CurveSplitterHandle(orientation(), this);
        }
    };

    /* 单例实例指针 */
    mCurveWindow* mCurveWindow::_instance = nullptr;

    /*================== 单例获取 ==================*/
    mCurveWindow* mCurveWindow::getInstance()
    {
        if (!_instance)
            _instance = new mCurveWindow();
        return _instance;
    }

    mPlotView* mCurveWindow::getPlotView()
    {
        return _plotView;
    }

    void mCurveWindow::SetAxisTitle(const QString& title, bool isXAxis)
    {
        AxisId _axis = isXAxis ? X_Top : Y_Right;
        AxisSettings s = _plotView->getAxisSettings(_axis);
        s.title = title;
        _plotView->updateAxisScale(_axis, s);
    }

    //窗口底色随主题（原写死白色）；图表画布底色经 mPlotView 设置通道跟随。
    //_plotView 构造后段创建，此前调用判空跳过，创建点后补调
    void mCurveWindow::applyWindowTheme()
    {
        const QColor surface = MxFunctions::UiPalette::surface();
        setStyleSheet(QStringLiteral("background-color: %1;").arg(surface.name()));
        if (_plotView)
        {
            PlotView::PlotSettings s = _plotView->getPlotSettings();
            s.backgroundColor = surface;
            _plotView->setPlotSettings(s);
        }
    }

    /*================== 构造：初始化 UI ==================*/
    mCurveWindow::mCurveWindow(QWidget* parent)
        : QWidget(parent)
    {
        this->setAttribute(Qt::WA_StyledBackground, true);

        applyWindowTheme();

        this->setMinimumSize(0, 0);
        this->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

        //QWidget* w = new QWidget();
       
    /*    QSplitter* splitter = new QSplitter();
        splitter->setStyleSheet(R"(
    QSplitter::handle:horizontal {
        background:#DCDFE6;
        width: 1px;
        margin: 0 1px;
    }
    
    QSplitter::handle:horizontal:hover {
        background: #DCDFE6;
        width: 1px;
        margin: 0;
    }
    
    QSplitter::handle:vertical {
        background: transparent;
        height: 1px;
        margin: 1px 0;
    }
    
    QSplitter::handle:vertical:hover {
        background: #1385F6;
        height: 3px;
        margin: 0;
    }
)");*/
        CurveSplitter* splitter = new CurveSplitter(this);
        /* 主布局 */
        auto* vLayout = new QHBoxLayout(this);
        vLayout->setContentsMargins(4, 4, 4, 4);
        vLayout->setSpacing(6);
        /*  曲线 */
        _plotView = new mPlotView(this);
        _plotView->setMinimumSize(144, 0);
        //_plotView->setPlotType(LineChart);
        //_plotView->setLegendVisual(false);
        _plotView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        _plotView->setMinimumSize(0, 0);
        _plotView->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        //画布初始背景随主题（构造默认白色，见 applyWindowTheme 注释）
        applyWindowTheme();

        mPlotViewStyleWidget* styleDlg = new mPlotViewStyleWidget(_plotView, this);
        styleDlg->setMinimumWidth(144);
        styleDlg->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        styleDlg->setMinimumSize(0, 0);
        styleDlg->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        styleDlg->setTableGridVisible(true);
        styleDlg->setTableAlternatingRowColors(true);


        //右上轴隐藏 lry
        AxisSettings s = _plotView->getAxisSettings(X_Top);
        s.isVisible = false;
        _plotView->updateAxisScale(X_Top, s);
        s = _plotView->getAxisSettings(Y_Right);
        s.isVisible = false;
        _plotView->updateAxisScale(Y_Right, s);

        styleDlg->refreshData();
        _plotView->setPlotViewStyleWidget(styleDlg);

        splitter->addWidget(_plotView);
        splitter->addWidget(styleDlg);
        splitter->setStretchFactor(0, 2);
        splitter->setStretchFactor(1, 1);
        splitter->setSizes(QList<int>{800, 400});
        
        vLayout->addWidget(splitter);

        
        /* 语言切换信号 */
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
            this, SLOT(slot_changeLanguage(int)));

        /* 曲线页"曲线样式"按钮（发送方为PostCurve模块的mCurveMenuContributor，经全局信号总线解耦） */
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::clickCurveStyleSig, this, [this]() {
            getPlotView()->getPlotViewStyleWidget()->show();
            });

        QFont font(MxFunctions::UiMetrics::uiFontFamily(), 10);   // 第二参数是点阵大小
        _plotView->getQwtPlot()->setAxisFont(X_Bottom, font);
        _plotView->getQwtPlot()->setAxisFont(X_Top, font);
        _plotView->getQwtPlot()->setAxisFont(Y_Left, font);
        _plotView->getQwtPlot()->setAxisFont(Y_Right, font);
    }

    /*================== 析构：释放曲线 ==================*/
    mCurveWindow::~mCurveWindow()
    {
        //clearBaseCurve();
        //clearMultiCurves();
        if (_plotView)
        {
            delete _plotView;
            _plotView = nullptr;
        }
    }

    void mCurveWindow::setPlotTitle(QString title)
    {
        _plotView->getQwtPlot()->setTitle(title);
    }

    void mCurveWindow::clearAllPlot()
    {
        _plotView->clearAll();
        _plotView->getPlotViewStyleWidget()->refreshData();
    }

    void mCurveWindow::setViewStyleVisible(bool show)
    {
        _plotView->getPlotViewStyleWidget()->setVisible(show);
    }

    /*=================== 曲线样式备份/恢复 ===================
     * mPlotView::clearAll() 会连同 m_curveSettings 一起清空，
     * 重建曲线时同名曲线会回到默认样式(实线)，因此清空前先备份，
     * 重建后按曲线名恢复，保证切换树节点后线型/颜色/线宽等不丢失。
     * =======================================================*/
    static QMap<QString, PlotView::CurveSettings> backupCurveStyles(PlotView::mPlotView* plotView)
    {
        QMap<QString, PlotView::CurveSettings> styles;
        const QStringList names = plotView->getCurveNames();
        for (const QString& name : names)
        {
            styles.insert(name, plotView->getCurveSettings(name));
        }
        return styles;
    }

    static void restoreCurveStyles(PlotView::mPlotView* plotView, const QMap<QString, PlotView::CurveSettings>& styles)
    {
        if (styles.isEmpty())
        {
            return;
        }
        //未重建出来的曲线跳过，避免 setCurveStyle 内部报"未找到曲线"警告
        const QStringList names = plotView->getCurveNames();
        for (auto it = styles.constBegin(); it != styles.constEnd(); ++it)
        {
            if (names.contains(it.key()))
            {
                plotView->setCurveStyle(it.key(), it.value());
            }
        }
    }

    void mCurveWindow::setAxisAuto(PlotView::AxisId axisId, bool isAuto)
    {
        AxisSettings s = _plotView->getAxisSettings(axisId);
        s.autoScale = isAuto;
        _plotView->updateAxisRange(axisId, s);
        if (_plotView->getPlotViewStyleWidget())
        {
            _plotView->getPlotViewStyleWidget()->refreshAxisRangeData();
        }
    }

    void mCurveWindow::setAxisLog(PlotView::AxisId axisId, bool isLog)
    {
        AxisSettings s = _plotView->getAxisSettings(axisId);
        s.isLog = isLog;
        _plotView->updateAxisLog(axisId, s);
        if (_plotView->getPlotViewStyleWidget())
        {
            _plotView->getPlotViewStyleWidget()->refreshData();
        }
    }

    /*================== 单曲线更新 ==================*/
    void mCurveWindow::updateCurve(const QVector<double>& x,
        const QVector<double>& y, PlotView::PlotType plotType,
        const QString& title, bool isClear, bool showLegend)
    {
        if (x.size() != y.size())
        {
            return;
        }
        QMap<QString, PlotView::CurveSettings> styleBackup;
        if (isClear)
        {
            styleBackup = backupCurveStyles(_plotView);
            _plotView->clearAll();
        }
        //_plotView->setPlotType(plotType);
        _plotView->updateData(title, x, y);
        if (showLegend)
        {
            //_plotView->setLegendVisual(showLegend);
        }

        restoreCurveStyles(_plotView, styleBackup);

        _plotView->getPlotViewStyleWidget()->refreshData();
    }

    /*================== 多曲线更新 ==================*/
    void mCurveWindow::updateMultiCurves(const QVector<double> &xData,
        const QVector<QVector<double>>& yData, PlotView::PlotType plotType,
        const QStringList& titles,
        bool isClear, bool showLegend)
    {
        QMap<QString, PlotView::CurveSettings> styleBackup;
        if (isClear)
        {
            styleBackup = backupCurveStyles(_plotView);
            _plotView->clearAll();
        }
        for (int i = 0; i < yData.size(); i++)
        {
            //_plotView->setPlotType(plotType);
            _plotView->updateData(titles.isEmpty() ? "" : titles[i], xData, yData[i]);
        }

        _plotView->rescaleAxes();

        if (showLegend)
        {
            //_plotView->setLegendVisual(showLegend);
        }

        restoreCurveStyles(_plotView, styleBackup);

        _plotView->getPlotViewStyleWidget()->refreshData();
    }

    /*================== 多曲线更新 ==================*/
    void mCurveWindow::updateMultiCurves(const QVector<QVector<double>>& xData,
        const QVector<QVector<double>>& yData, PlotView::PlotType plotType,
        const QStringList& titles, bool isClear, bool showLegend)
    {
        if (xData.size() != yData.size())
        {
            return;
        }
        QMap<QString, PlotView::CurveSettings> styleBackup;
        if (isClear)
        {
            styleBackup = backupCurveStyles(_plotView);
            _plotView->clearAll();
        }
        /* ---- 调试打印：总共收到几条曲线 ---- */
        //qDebug() << "updateMultiCurves called with" << xData.size() << "curves";

        for (int i = 0; i < xData.size(); i++)
        {
            /* ---- 调试打印：每条曲线的长度和前后几个点 ---- */
            //_plotView->setPlotType(plotType);
            _plotView->updateData(titles.isEmpty() ? "" : titles[i], xData[i], yData[i]);
        }

        _plotView->rescaleAxes();

        if (showLegend)
        {
            //_plotView->setLegendVisual(showLegend);
        }

        restoreCurveStyles(_plotView, styleBackup);

        _plotView->getPlotViewStyleWidget()->refreshData();
    }

    /*================== 语言切换占位 ==================*/
    void mCurveWindow::slot_changeLanguage(int) {}

} // namespace MechMainWindow

// CurveSplitterHandle 带 Q_OBJECT 且定义在本 cpp，需包含同源 moc 产物
#include "mCurveWindow.moc"