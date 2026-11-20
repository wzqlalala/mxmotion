#include "mDefineCurveDlg.h"
#include "mCurveDataSourceStore.h"
#include "mCurveImportCoordinator.h"
#include <memory>
#include "mCurveTreeSelectDelegate.h"   //对象树选中条代理（与曲线树同款浅蓝圆角条）
#include "mDlgGeometry.h"
#include "mEditCurveDlg.h"              //编辑模式应用前压撤回快照（snapshotCurve）
#include "CurveCore/CurveDocument.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveView/mCurveViewportFactory.h"

#include <QPalette>
#include "mUiMenu.h"

//QT
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScreen>
#include <QSet>
#include <QSizeGrip>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QUndoStack>
#include <QVBoxLayout>

#include <exception>

namespace PostCurve
{
    mDefineCurveDlg* mDefineCurveDlg::_instance = nullptr;

    namespace
    {
        //"全部"分类项文案（函数分类下拉首项）
        const QString kAllCategories = QStringLiteral("全部");

        //列表按 UserRole 数据选行（未命中清选；返回是否命中）
        bool selectListRowByData(QListWidget* list, const QVariant& value)
        {
            for (int i = 0; i < list->count(); ++i)
                if (list->item(i)->data(Qt::UserRole) == value)
                {
                    list->setCurrentRow(i);
                    return true;
                }
            list->setCurrentRow(-1);
            return false;
        }

        //列表按文字选行（未命中清选）
        void selectListRowByText(QListWidget* list, const QString& text)
        {
            for (int i = 0; i < list->count(); ++i)
                if (list->item(i)->text() == text)
                {
                    list->setCurrentRow(i);
                    return;
                }
            list->setCurrentRow(-1);
        }

        //数值序列解析：逗号/分号/空白分隔（含全角），非法片段忽略
        QList<double> parseNumberList(const QString& text)
        {
            QList<double> out;
            static const QRegularExpression sep(QStringLiteral("[\\s,;，；]+"));
            for (const QString& tok : text.split(sep, QString::SkipEmptyParts))
            {
                bool ok = false;
                const double v = tok.toDouble(&ok);
                if (ok)
                    out.push_back(v);
            }
            return out;
        }

        //模型溯源文本反解尾部编号："Subcase 3" -> 3、"Point id 8001" -> 8001
        //（不匹配返回 0=未选，编辑模式还原通道用）
        int trailingNumber(const QString& text)
        {
            static const QRegularExpression re(QStringLiteral("(\\d+)\\s*$"));
            const auto m = re.match(text.trimmed());
            return m.hasMatch() ? m.captured(1).toInt() : 0;
        }

        //R22：数据文件是否 ascii 列数据集（csv/txt，不分大小写）。定义曲线的
        //四级列填充/装配/命名取值按此分流，R20/R21 按侧来源状态机不加特判
        bool isAsciiDataset(const QString& filePath)
        {
            return mCurveDataSourceStore::fileKind(filePath)
                == mCurveDataSourceStore::DataSourceKind::AsciiColumn;
        }

        //三路数据按 6 位小数写进条目数值表三串（X=横坐标，Ym=上面板幅值，
        //Yp=下面板相位；Pt=行号不入串）。编辑模式倒曲线数据与新建模式倒所选
        //文件通道数据共用
        void fillPairTexts(const QVector<double>& xs, const QVector<double>& yms,
            const QVector<double>& yps, PostCurve::DefineCurveEntry& e)
        {
            QStringList xT, ypT, ymT;
            for (int i = 0; i < xs.size(); ++i)
            {
                xT << QString::number(xs.at(i), 'f', 6);
                ymT << QString::number(yms.value(i), 'f', 6);
                ypT << QString::number(yps.value(i), 'f', 6);
            }
            e.pairXText = xT.join(QStringLiteral(", "));
            e.pairYpText = ypT.join(QStringLiteral(", "));
            e.pairYmText = ymT.join(QStringLiteral(", "));
        }

        //全模块 p/w/c 引用编号的唯一出处：按文档顺序遍历全部曲线（页/视口/曲线
        //均 1 基编号，跳过其余类型节点），每条曲线回调一次 (p, pageId, w, chartId,
        //c, curveId)。引用树/选择器弹窗树/引用前缀合成统一走这里，编号规则改动
        //只此一处
        template <typename Fn>
        void forEachCurveRef(Fn&& fn)
        {
            CurveDocument* doc = mCurveDocController::getInstance()->document();
            int p = 0;
            for (int pageId : doc->childNodeIds(doc->rootNodeId()))
            {
                if (doc->nodeType(pageId) != CurveNodeType::Page)
                    continue;
                ++p;
                int w = 0;
                for (int chartId : doc->childNodeIds(pageId))
                {
                    if (doc->nodeType(chartId) != CurveNodeType::Chart)
                        continue;
                    ++w;
                    int c = 0;
                    for (int curveId : doc->childNodeIds(chartId))
                    {
                        if (doc->nodeType(curveId) != CurveNodeType::Curve)
                            continue;
                        ++c;
                        fn(p, pageId, w, chartId, c, curveId);
                    }
                }
            }
        }

        //引用前缀 pNwMcK（编号即 forEachCurveRef 的 p/w/c）
        QString curveRefPrefix(int p, int w, int c)
        {
            return QStringLiteral("p%1w%2c%3").arg(p).arg(w).arg(c);
        }

        //曲线引用树填充（照参考软件"曲线选择器"树样式）：工程 -> 页 -> 视口 ->
        //曲线名直列，无 p/w/c 前缀、无 x/y 子级，各行带状态图标（绿√+类型图标，
        //与曲线树同款，经 curveNodeStatusIcon）。曲线节点 data=引用前缀 pNwMcK、
        //UserRole+1=1：选中后经"直接插入"按当前激活向量补 .x/.y（弹窗双击默认
        //取 .y，作函数向量参数）
        void populateCurveRefTree(QTreeWidget* tree)
        {
            tree->clear();
            tree->setIconSize(QSize(28, 16));   //状态图标为 28x16 逻辑px，防压缩发虚
            CurveDocument* doc = mCurveDocController::getInstance()->document();
            auto* projectItem = new QTreeWidgetItem(tree);
            const QString rootName = doc->nodeName(doc->rootNodeId());
            projectItem->setText(0, rootName.isEmpty() ? QStringLiteral("工程") : rootName);
            projectItem->setIcon(0, curveNodeStatusIcon(CurveNodeType::Project));
            //层级条目随编号推进创建（遍历按文档序，页/视口编号连续递增）
            QTreeWidgetItem* pageItem = nullptr;
            QTreeWidgetItem* chartItem = nullptr;
            int curP = 0;
            int curW = 0;
            forEachCurveRef([&](int p, int pageId, int w, int chartId, int c, int curveId)
                {
                    if (p != curP)
                    {
                        curP = p;
                        curW = 0;
                        pageItem = new QTreeWidgetItem(projectItem);
                        pageItem->setText(0, doc->nodeName(pageId));
                        pageItem->setIcon(0, curveNodeStatusIcon(CurveNodeType::Page));
                    }
                    if (w != curW)
                    {
                        curW = w;
                        chartItem = new QTreeWidgetItem(pageItem);
                        chartItem->setText(0, doc->nodeName(chartId));
                        chartItem->setIcon(0, curveNodeStatusIcon(CurveNodeType::Chart));
                    }
                    const QString base = curveRefPrefix(p, w, c);
                    auto* curveItem = new QTreeWidgetItem(chartItem);
                    curveItem->setText(0, doc->nodeName(curveId));
                    curveItem->setIcon(0, curveNodeStatusIcon(CurveNodeType::Curve));
                    curveItem->setToolTip(0, base);   //引用前缀悬停可见
                    curveItem->setData(0, Qt::UserRole, base);
                    curveItem->setData(0, Qt::UserRole + 1, 1);   //曲线级：插入时补向量后缀
                });
            tree->expandAll();
        }

        //曲线选择器弹窗树（照参考截图，与"曲线"页直列样式不同）：pN : 页名 ->
        //wM : 视口名 -> 曲线名 -> 向量叶子 x/y。叶子 data=完整引用（双击即插）；
        //曲线节点 data=引用前缀、UserRole+1=1（双击默认取 .y 作函数向量参数）
        void populatePickerTree(QTreeWidget* tree)
        {
            tree->clear();
            tree->setIconSize(QSize(28, 16));
            CurveDocument* doc = mCurveDocController::getInstance()->document();
            QTreeWidgetItem* pageItem = nullptr;
            QTreeWidgetItem* chartItem = nullptr;
            int curP = 0;
            int curW = 0;
            forEachCurveRef([&](int p, int pageId, int w, int chartId, int c, int curveId)
                {
                    if (p != curP)
                    {
                        curP = p;
                        curW = 0;
                        pageItem = new QTreeWidgetItem(tree);
                        pageItem->setText(0, QStringLiteral("p%1 : %2").arg(p).arg(doc->nodeName(pageId)));
                        pageItem->setIcon(0, curveNodeStatusIcon(CurveNodeType::Page));
                    }
                    if (w != curW)
                    {
                        curW = w;
                        chartItem = new QTreeWidgetItem(pageItem);
                        chartItem->setText(0, QStringLiteral("w%1 : %2").arg(w).arg(doc->nodeName(chartId)));
                        chartItem->setIcon(0, curveNodeStatusIcon(CurveNodeType::Chart));
                    }
                    const QString base = curveRefPrefix(p, w, c);
                    auto* curveItem = new QTreeWidgetItem(chartItem);
                    curveItem->setText(0, doc->nodeName(curveId));
                    curveItem->setIcon(0, curveNodeStatusIcon(CurveNodeType::Curve));
                    curveItem->setToolTip(0, base);
                    curveItem->setData(0, Qt::UserRole, base);
                    curveItem->setData(0, Qt::UserRole + 1, 1);
                    for (int v = 0; v < 2; ++v)
                    {
                        auto* leaf = new QTreeWidgetItem(curveItem);
                        const QString suffix = v == 0 ? QStringLiteral("x") : QStringLiteral("y");
                        leaf->setText(0, suffix);
                        leaf->setToolTip(0, base + QLatin1Char('.') + suffix);
                        leaf->setData(0, Qt::UserRole, base + QLatin1Char('.') + suffix);
                    }
                });
            tree->expandAll();
        }

        //曲线选择器浮动面板（fx 按钮弹出；双击叶子向量 -> 插入引用后关闭）。
        //Qt::Popup 形态，白卡片 + 迷你标题栏（照参考截图"曲线选择器"）；
        //标题栏可拖动移动（照弹窗拖动方式）
        class RefPickerPopup : public QWidget
        {
        public:
            explicit RefPickerPopup(QWidget* parent)
                : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
            {
                setFixedSize(280, 360);
                setStyleSheet(QStringLiteral(
                    "QWidget{background:#FFFFFF;}"
                    "QTreeWidget{border:none;background:#FFFFFF;border-top:1px solid #E5E7EB;}"));

                auto* lay = new QVBoxLayout(this);
                lay->setContentsMargins(1, 1, 1, 1);
                lay->setSpacing(0);

                _bar = new QWidget(this);
                _bar->setFixedHeight(30);
                auto* barLay = new QHBoxLayout(_bar);
                barLay->setContentsMargins(8, 0, 4, 0);
                barLay->setSpacing(3);
                QLabel* logo = new QLabel(_bar);
                logo->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
                    .scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                QLabel* title = new QLabel(QStringLiteral("曲线选择器"), _bar);
                title->setStyleSheet("QLabel{color:#333333;font-size:12px;font-weight:600;}");
                QToolButton* closeBtn = new QToolButton(_bar);
                closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
                closeBtn->setIconSize(QSize(24, 24));
                closeBtn->setToolTip(QStringLiteral("关闭"));
                closeBtn->setStyleSheet(
                    "QToolButton{background:transparent;border:none;}"
                    "QToolButton:hover{background:#EFF1F2;}");
                connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
                barLay->addWidget(logo);
                barLay->addWidget(title, 1);
                barLay->addWidget(closeBtn);

                _tree = new QTreeWidget(this);
                _tree->setHeaderHidden(true);
                _tree->setUniformRowHeights(true);

                lay->addWidget(_bar);
                lay->addWidget(_tree, 1);
            }

            QTreeWidget* tree() const { return _tree; }

        protected:
            //标题栏按下开始拖动 / 拖动中移动（照 mDefineCurveDlg 标题栏拖动）
            void mousePressEvent(QMouseEvent* event) override
            {
                if (event->button() == Qt::LeftButton && _bar->underMouse())
                {
                    _dragging = true;
                    _dragOffset = event->globalPos() - frameGeometry().topLeft();
                }
                QWidget::mousePressEvent(event);
            }
            void mouseMoveEvent(QMouseEvent* event) override
            {
                if (_dragging && (event->buttons() & Qt::LeftButton))
                    move(event->globalPos() - _dragOffset);
                QWidget::mouseMoveEvent(event);
            }
            void mouseReleaseEvent(QMouseEvent* event) override
            {
                _dragging = false;
                QWidget::mouseReleaseEvent(event);
            }

        private:
            QWidget* _bar = nullptr;    //标题栏（拖动热区）
            QTreeWidget* _tree = nullptr;
            bool _dragging = false;     //拖动中标志
            QPoint _dragOffset;         //拖动起始偏移
        };

        //参数值输入框激活显示 fx 按钮（照参考交互⑦⑧：只有左键点入输入框后
        //右侧按钮才出现）：获得焦点时显示。注意不做"失焦隐藏"——点击 fx 后
        //弹窗 show 会转移焦点，若此时同步隐藏按钮，Qt::Popup 在 Windows 上
        //会因重入的布局/焦点事件被当作外部点击立即关闭（WA_DeleteOnClose
        //直接销毁），表现为弹窗无法浮现。按钮保持可见至参数表重建
        class FxRevealFilter : public QObject
        {
        public:
            explicit FxRevealFilter(QToolButton* btn) : QObject(btn), _btn(btn) {}

            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (event->type() == QEvent::FocusIn)
                    _btn->setVisible(true);
                return QObject::eventFilter(watched, event);
            }

        private:
            QToolButton* _btn = nullptr;
        };
        //函数叶子图标
        QIcon makeFuncFxIcon()
        {
            static const QIcon icon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/函数.svg"));
            return icon;
        }

        //曲线节点id -> 引用前缀 pNwMcK（编号经 forEachCurveRef，与引用树一致）；
        //非曲线节点返回空
        QString curveRefPrefixById(int curveNodeId)
        {
            QString found;
            forEachCurveRef([&](int p, int, int w, int, int c, int curveId)
                {
                    if (curveId == curveNodeId && found.isEmpty())
                        found = curveRefPrefix(p, w, c);
                });
            return found;
        }

        //按函数名查实现状态（红色标注/禁用插入的依据）：表内 implemented=false
        //为未实现项（《函数类型.xlsx》目录对齐标记）；未选中或未收录名返回 false
        bool funcUnimplemented(const QString& funcName)
        {
            if (funcName.isEmpty())
                return false;
            for (const mCurveExpressionEvaluator::MathFuncInfo& f
                : mCurveExpressionEvaluator::functionTable())
                if (f.name == funcName)
                    return !f.implemented;
            return false;
        }
    }

    // =============================================================================
    // 生命周期 / 入口
    // =============================================================================

    mDefineCurveDlg::mDefineCurveDlg(QWidget* parent)
        : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        buildUi();
        rebuildFuncTree();
        fillFileCombo();
        refreshActionStates();
        refreshVectorEditability();   //初始态：向量框按来源/激活侧启停
        resize(1230, 748);   //左曲线列表 + 右向量/来源定义区
        setMinimumSize(960, 520);
        DlgGeom::bind(this, "DefineCurveDlg", DlgGeom::Mode::Geometry, QSize(960, 520));

        //本次运行无已存几何时打开居中于父窗口（有几何则 bind 已恢复位置，不再
        //居中；几何只记当次运行，重启软件后回到默认尺寸+首开居中）
        _needCenter = !DlgGeom::hasSessionGeometry("DefineCurveDlg");

        //数据集变化：新导入文件 -> 重建文件下拉并选中新文件
        connect(mCurveDataSourceStore::getInstance(), &mCurveDataSourceStore::sig_fileAdded,
            this, &mDefineCurveDlg::onFileAdded);
        //目标图表（或其祖先页）被删 -> 关闭（此时父子链仍可查询；mApplyStyleDlg 先例）
        connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeAboutToBeRemoved,
            this, &mDefineCurveDlg::onNodeAboutToBeRemoved);
        //文档结构变化（加/删页、视口、曲线）-> 重建曲线引用树
        connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeAdded,
            this, &mDefineCurveDlg::onDocStructureChanged);
        connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeRemoved,
            this, &mDefineCurveDlg::onDocStructureChanged);
        //视窗 Shift+左键点曲线：数学运算来源 X/Y 向量引用的拾取通道
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_curvePicked,
            this, &mDefineCurveDlg::onCurvePicked);
        //定义曲线模式联动（互斥收口在视口工厂）：模式开启 -> 重绑目标并弹出；
        //模式结束（再次点击按钮/其他功能开启/目标图表删除）-> 关闭本窗口，
        //未应用的定义不落模型直接丢弃（close() 而非 hide()：对齐 mPropertyEditorPopup
        //的关闭语义，进程退出联动）
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_defineCurveStarted,
            this, &mDefineCurveDlg::onDefineStarted);
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_defineCurveStopped,
            this, &QWidget::close);
        //目标图表类型切换（弹窗开着时经参数栏切 XY/柱状/复数）：向量区形态
        //（XY 两行/柱状表达式行/复数三槽）与左侧按钮排布即时联动
        connect(mCurveDocController::getInstance(),
            &mCurveDocController::sig_nodePropertyChanged, this,
            [this](int nodeId, const QString& prop)
        {
            if (prop == CurveProp::kChartType && nodeId == _targetChartId)
            {
                //整条↔按侧字段互同步（弹窗存活期间经参数栏切图表类型，条目无
                //损续用）：柱状路径只消费整条字段（source/filePath），非柱状
                //路径只消费按侧字段——切入柱状=主侧按侧字段回写整条；切出柱
                //状=整条字段播撒按侧（柱状期只维护整条，按侧字段停留在柱状期
                //前，以柱状期编辑为准）。同向切换（XY↔复数）不 sync，按侧字段
                //两态通用
                const bool nowBar = targetIsBarChart();
                if (_lastChartType >= 0 && nowBar != (_lastChartType == 1))
                {
                    //切入柱状不得经退化访问器取值（信号触发时 targetIsBarChart()
                    //已反映新类型，sideSource/sideFilePath 命中整条退化分支=自
                    //我赋值空操作）——直接读按侧字段，主侧按旧类型(_lastChartType)
                    //判定：XY 期=Y 侧、复数期=ym 幅值侧
                    const bool fromComplex = (_lastChartType == 2);
                    auto migrateToBar = [fromComplex](DefineCurveEntry& it)
                    {
                        //按侧原始字段可能是哨兵 -1（未显式设置）/侧路径为空：
                        //钳制为出厂文件来源、保留原整条路径——哨兵/空路径不得
                        //写进整条字段（R20-B01，柱状路径只消费整条字段）
                        const int rawSource = fromComplex ? it.ymSource : it.ySource;
                        it.source = rawSource < 0 ? 0 : rawSource;
                        const QString& sidePath = fromComplex ? it.ymFilePath : it.yFilePath;
                        if (!sidePath.isEmpty())
                            it.filePath = sidePath;
                    };
                    auto migrateFromBar = [](DefineCurveEntry& it)
                    {
                        it.xSource = it.source;
                        it.ySource = it.source;
                        it.ypSource = it.source;
                        it.ymSource = it.source;
                        it.xFilePath = it.filePath;
                        it.yFilePath = it.filePath;
                        it.ypFilePath = it.filePath;
                        it.ymFilePath = it.filePath;
                    };
                    for (DefineCurveEntry& it : _entries)
                    {
                        if (nowBar)
                            migrateToBar(it);
                        else
                            migrateFromBar(it);
                    }
                    //同族：条目剪贴板随型迁移（"复制条目→切型→粘贴"不插入字段
                    //未同步的条目）
                    if (_hasClip)
                    {
                        if (nowBar)
                            migrateToBar(_clipEntry);
                        else
                            migrateFromBar(_clipEntry);
                    }
                }
                _lastChartType = nowBar ? 1 : (targetIsComplexChart() ? 2 : 0);
                //柱状 ↔ 其他类型：左栏 Categories 行增删 + 数值表形态重建
                //（常规 3 列单条目 / 柱状共享 Index+Categories+每条目一列）
                syncBarListShape();
                const int cur = currentEntryIndex();
                _syncing = true;
                if (cur >= 0)
                    _curveList->setCurrentRow(entryToListRow(cur));
                else
                    _curveList->setCurrentRow(-1);
                _syncing = false;
                if (cur < 0 && !categoriesRowActive())
                {
                    _current = -1;
                    clearPanelState();
                }
                else if (cur >= 0)
                {
                    //不变式维护（R21）：切型后来源单选与页面必须等于激活侧来源
                    //（柱状=整条来源、向量单选恒表达式侧）——setCurrentRow 在
                    //syncing 下不触发选择信号，须显式回填，否则单选/页面与
                    //条目激活侧分家（切型迁移只改字段不改激活侧，来源取有效
                    //口径）
                    DefineCurveEntry& ce = _entries[cur];
                    int activeSide = qBound(0, ce.activeVector, 2);
                    //非柱状非复数目的地（XY）：复数期残留的 activeVector==2
                    //归一为主侧 1（XY 无 ym 侧——不归一则向量单选点亮 x 而激
                    //活侧仍 2，数值表无可编辑列/来源点击按 side=2 静默错侧，
                    //R21 评审）；同族切型不迁移字段，激活侧残留只在目的地为
                    //XY 时成立
                    if (!nowBar && !targetIsComplexChart() && activeSide == 2)
                    {
                        ce.activeVector = 1;
                        activeSide = 1;
                    }
                    const int src = nowBar ? ce.source : sideSource(ce, activeSide);
                    _syncing = true;
                    if (src == 1)
                        _mathRadio->setChecked(true);
                    else if (src == 2)
                        _valueRadio->setChecked(true);
                    else
                        _fileRadio->setChecked(true);
                    _sourceStack->setCurrentIndex(src);
                    if (nowBar)
                        _yRadio->setChecked(true);   //柱状：向量区恒表达式侧
                    else if (targetIsComplexChart())
                    {
                        _xRadio->setChecked(activeSide == 0);
                        _yRadio->setChecked(activeSide == 1);
                        _ymRadio->setChecked(activeSide == 2);
                    }
                    else
                        (activeSide == 1 ? _yRadio : _xRadio)->setChecked(true);
                    if (src == 0)
                    {
                        //文件页：下拉回显激活侧文件 + 四级列按该侧通道回填
                        const int fi = _fileCombo->findData(nowBar
                            ? ce.filePath : sideFilePath(ce, activeSide));
                        _fileCombo->setCurrentIndex(fi);
                        refreshFileColumns();
                    }
                    _syncing = false;
                }
                rebuildValueTableView();
                refreshVectorEditability();
                refreshInsertButtons();
                refreshActionStates();
            }
        });

    }

    mDefineCurveDlg::~mDefineCurveDlg()
    {
        if (_instance == this)
            _instance = nullptr;
    }

    mDefineCurveDlg* mDefineCurveDlg::getInstance(QWidget* parent)
    {
        //父窗口须为顶层主窗口：QMdiSubWindow 也带 Window 标志，直接作父会把
        //对话框卷进 MDI 管理——传入为空时取当前活动顶层窗口（Ribbon 点击时即
        //主窗口，同 mEditCurveDlg 策略）
        QWidget* host = parent;
        while (host && host->parentWidget())
            host = host->parentWidget();
        if (!_instance)
            _instance = new mDefineCurveDlg(host);
        return _instance;
    }

    void mDefineCurveDlg::onDefineStarted(int chartId)
    {
        _targetChartId = chartId;
        //每次开启先复位（未应用的定义不落模型，直接丢弃，列表清空、仅
        //"新增/关闭"可用）；先重填文件下拉再复位（复位会清空下拉选中，照设计图显示空下拉）
        fillFileCombo();
        resetDialog();
        //整条↔按侧同步基线=本次目标类型（addEntryFromCurve 载入条目时两套字
        //段一致写入，无需播撒/回写）
        _lastChartType = targetIsBarChart() ? 1 : (targetIsComplexChart() ? 2 : 0);
        setPickingVector(-1);   //上次会话可能停在曲线拾取态：重开先退出（清占位提示）
        //编辑模式（设计图）：当前视口有曲线时，把目标图表内全部曲线按树序
        //载入为条目（XY/复数图/柱状图一致；内容随各自来源还原呈现，应用时
        //原位更新），打开前选中的曲线（或其 x/y/ym/yp 壳）作为初始选中条目
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        int sel = mCurveDocController::getInstance()->currentNodeId();
        const CurveNodeType selType = doc->nodeType(sel);
        if (selType == CurveNodeType::CurveXData
            || selType == CurveNodeType::CurveYData
            || selType == CurveNodeType::CurveY2Data)
            sel = doc->parentNodeId(sel);
        CurveNode* chartNode = doc->node(chartId);
        if (chartNode)
        {
            int firstIdx = -1;
            int selIdx = -1;
            for (int childId : doc->childNodeIds(chartId))
            {
                if (doc->nodeType(childId) != CurveNodeType::Curve)
                    continue;
                addEntryFromCurve(childId);
                if (firstIdx < 0)
                    firstIdx = _entries.size() - 1;
                if (childId == sel)
                    selIdx = _entries.size() - 1;
            }
            //初始选中：打开前选中的曲线，否则树序第一条。_current 是选中中枢
            //（currentEntryIndex 直读），setCurrentRow 在 syncing 下不回写，
            //须显式同步，否则列表高亮与右栏面板分家
            const int idx = selIdx >= 0 ? selIdx : firstIdx;
            if (idx >= 0 && _current != idx)
            {
                _current = idx;
                _syncing = true;
                _curveList->setCurrentRow(entryToListRow(idx));
                _syncing = false;
                loadEntryToPanel();
            }
        }
        rebuildObjectTree();

        if (_needCenter)
        {
            _needCenter = false;
            if (QWidget* pw = parentWidget())
                move(pw->frameGeometry().center()
                    - QPoint(width() / 2, height() / 2));
        }
        show();
        raise();
        activateWindow();
    }

    // =============================================================================
    // UI 构建
    // =============================================================================

    void mDefineCurveDlg::buildUi()
    {
        setAutoFillBackground(true);
        //窗口细边框：白底标题栏与内容区同为浅色，加描边保证弹窗轮廓可辨识。
        //须用 objectName 选择器：本类在 PostCurve 命名空间内，QSS 裸类名选择器
        //按 metaObject className（PostCurve::mDefineCurveDlg）匹配不上，规则整体
        //失效，底部 QSizeGrip 行等未覆盖区域露出调色板灰底（mApplyStyleDlg 同修）
        setObjectName("defineCurveDlg");
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget#defineCurveDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");

        QVBoxLayout* outer = new QVBoxLayout(this);
        outer->setContentsMargins(1, 1, 1, 1);
        outer->setSpacing(0);

        //标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮（照 mApplyStyleDlg）
        _titleBar = new QWidget(this);
        _titleBar->setObjectName("defineCurveTitleBar");
        _titleBar->setFixedHeight(52);
        _titleBar->setStyleSheet(R"(
            QWidget#defineCurveTitleBar{
                background:#FFFFFF;
            }
        )");
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
        //关闭即退出定义曲线模式（模式结束信号反过来 close 本窗口），再走 close
        //关闭语义（对齐 mPropertyEditorPopup 的关闭语义，进程退出联动）
        connect(closeBtn, &QToolButton::clicked, this, [this]()
            {
                mCurveViewportFactory::getInstance()->setDefineCurveMode(false);
                close();
            });
        titleLayout->addWidget(logoLabel);
        titleLayout->addWidget(titleLabel, 1);
        titleLayout->addWidget(closeBtn);

        //主体：副标题 + 左右两栏 + 底部按钮行
        QWidget* body = new QWidget(this);
        //统一控件样式（含禁用态——宿主级样式表优先级高于应用级 :disabled 规则，
        //此处必须自带禁用配色，否则初始态的置灰会退化为白底黑字）
        body->setStyleSheet(R"(
            QWidget{background:#FFFFFF;color:#333333;font-size:12px;}
            QLabel{background:transparent;}
            QLineEdit,QComboBox,QListWidget,QTreeWidget,QTableWidget,QPlainTextEdit{
                background:#FFFFFF;border:1px solid #D9DCE0;border-radius:4px;padding:2px 0px;
                selection-background-color:#0066CC;selection-color:#FFFFFF;
            }
            /*对象树（曲线选择）选中条由行代理自绘：原生选中背景透明（分支区不再
            * 出现深蓝块）*/
            QTreeWidget#objRefTree{
                selection-background-color:transparent;
            }
            /*函数库树同上：选中条由行代理自绘，原生选中背景透明*/
            QTreeWidget#funcLibTree{
                selection-background-color:transparent;
            }
            QListWidget,QTreeWidget,QTableWidget,QPlainTextEdit{padding:1px;}
            QLineEdit:disabled,QComboBox:disabled,QListWidget:disabled,QTreeWidget:disabled,
            QTableWidget:disabled,QPlainTextEdit:disabled{
                background:#F5F7FA;color:#A8ABB2;border-color:#E4E7ED;
            }
            QComboBox QAbstractItemView{
                background:#FFFFFF;color:#333333;selection-background-color:#0066CC;
            }
            QCheckBox,QRadioButton{background:transparent;color:#333333;}
            QCheckBox:disabled,QRadioButton:disabled{color:#A8ABB2;}
            QPushButton{
                background:#FFFFFF;border:1px solid #C9CDD4;border-radius:4px;
                color:#333333;padding:4px 10px;
            }
            QPushButton:hover{background:#EFF1F2;}
            QPushButton:disabled{background:#F5F7FA;border-color:#E4E7ED;color:#A8ABB2;}
            QToolButton{
                background:#FFFFFF;border:1px solid #C9CDD4;border-radius:4px;
                color:#333333;padding:2px 6px;
            }
            QToolButton:hover{background:#EFF1F2;}
            QToolButton:disabled{background:#F5F7FA;border-color:#E4E7ED;color:#A8ABB2;}
            QFrame#myContainer{
                background:#F0F3F6;border:1px solid #DCDFE6;
                border-top-left-radius:7px;border-top-right-radius:7px;
            }
            QFrame#Container{border:1px solid #DCDFE6; background-color: #FFFFFF; border-top: none;}
            QFrame#Container QListWidget,QFrame#Container QTreeWidget{ background-color: #FFFFFF;border:none;border-radius:0;}
            QHeaderView::section{
                background:#F5F7FA;color:#333333;border:none;
                border-bottom:1px solid #E4E7EB;padding:3px 6px;
            }
            QTableWidget{gridline-color:#EFEFEF;}
            /* 内容面板：无边框、无边距，整体向下偏移1px，给标签底边框留出完整绘制空间 */
            QTabWidget::pane{
                border:none;
                background:#FFFFFF;
                margin:0px;
                padding:0px;
            }
            QTabWidget QTabBar{
                margin:0px;
                padding:0px;
                background:transparent;
            }
            QTabBar::tab{
                background:transparent;
                color:#333333;
                border-radius:4px 4px 0 0;
                padding:4px 14px;
                border:1px solid #DCDFE6; 
            }
            /* 选中标签：全闭合蓝色边框 */
            QTabBar::tab:selected{
                background: #E7F3FE; 
                border: 1px solid #1385F6;
                border-radius:4px 4px 0 0;
                color:#1385F6;
            }
            /* 悬停未选中标签文字变蓝 */
            QTabBar::tab:hover:!selected{
                color:#1385F6;
            }
        )");
        QVBoxLayout* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(20, 20, 20, 20);
        bodyLayout->setSpacing(12);

        QHBoxLayout* columns = new QHBoxLayout();
        columns->setSpacing(20);
        QWidget* left = buildListPanel();
        left->setFixedWidth(300);
        columns->addWidget(left);
        columns->addWidget(buildRightPanel(), 1);
        bodyLayout->addLayout(columns, 1);

        //底部按钮行：关闭 / 应用（应用为蓝底主按钮，条目定义完整才启用）
        QHBoxLayout* buttonRow = new QHBoxLayout();
        buttonRow->setContentsMargins(0, 18, 0, 0);
        buttonRow->setSpacing(12);
        _closeBtn = new QPushButton(QStringLiteral("关闭"), body);
        _applyBtn = new QPushButton(QStringLiteral("应用"), body);
        _closeBtn->setFixedSize(44, 26);
        _applyBtn->setFixedSize(44, 26);
        //应用=模块统一主蓝 #1385F6（同"从文件创建曲线"绘图钮），禁用态浅蓝
        _applyBtn->setStyleSheet(R"(
            QPushButton{
                background:#1385F6; border:none; border-radius:4px;
                color:#FFFFFF; padding:0px;
            }
            QPushButton:hover{ background:#3D9EF8; }
            QPushButton:pressed{ background:#0F6ACB; }
            QPushButton:disabled{ background:#9CC3E5; color:#FFFFFF; }
        )");
        //关闭即退出定义曲线模式（模式结束信号反过来 close 本窗口）
        connect(_closeBtn, &QPushButton::clicked, this, [this]()
            {
                mCurveViewportFactory::getInstance()->setDefineCurveMode(false);
                close();
            });
        connect(_applyBtn, &QPushButton::clicked, this, &mDefineCurveDlg::onApply);
        buttonRow->addStretch(1);
        buttonRow->addWidget(_closeBtn);
        buttonRow->addWidget(_applyBtn);
        bodyLayout->addLayout(buttonRow);

        outer->addWidget(_titleBar);
        outer->addWidget(body, 1);

        //右下角缩放手柄：不占布局行（单独成行会把按钮行抬高、底部留白超出
        //设计间距，对齐 mApplyStyleDlg），改为覆盖子件贴角，resizeEvent 跟随
        _grip = new QSizeGrip(this);
        _grip->setFixedSize(_grip->sizeHint());
    }

    QWidget* mDefineCurveDlg::buildListPanel()
    {
        QWidget* panel = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(panel);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(12);

        //曲线名称：勾选=用编辑框文本命名（未勾选按来源自动命名）
        _nameCheck = new QCheckBox(QStringLiteral("曲线名称"), panel);
        _nameEdit = new QLineEdit(panel);
        _nameEdit->setFixedHeight(24);
        connect(_nameCheck, &QCheckBox::toggled, this, &mDefineCurveDlg::onNameCheckToggled);
        connect(_nameEdit, &QLineEdit::textEdited, this, &mDefineCurveDlg::onNameEdited);
        QHBoxLayout* nameRow = new QHBoxLayout();
        nameRow->setContentsMargins(0, 0, 0, 0);
        nameRow->setSpacing(12);
        nameRow->addWidget(_nameCheck);
        nameRow->addWidget(_nameEdit, 1);
        lay->addLayout(nameRow);

        //待新增曲线列表（单列名称；选中条目 -> 右栏编辑其定义）
        _curveList = new QListWidget(panel);
        connect(_curveList, &QListWidget::currentRowChanged,
            this, &mDefineCurveDlg::onListSelectionChanged);
        lay->addWidget(_curveList, 1);

        //剪切 / 复制 / 粘贴 / 新增（初始态仅"新增"可用）
        _cutBtn = new QPushButton(QStringLiteral("剪切"), panel);
        _copyBtn = new QPushButton(QStringLiteral("复制"), panel);
        _pasteBtn = new QPushButton(QStringLiteral("粘贴"), panel);
        _addBtn = new QPushButton(QStringLiteral("新增"), panel);
        _addPmBtn = new QPushButton(QStringLiteral("新增 P/M"), panel);
        _addRiBtn = new QPushButton(QStringLiteral("新增 R/I"), panel);
        //按钮排布：等分填满按钮行（复数图 5 钮 / 常规 4 钮，切类型自动重排不
        //留空）；最小宽 38/64 + 局部压掉全局 QSS 水平内边距，保证不截字
        const QString btnStyle = QStringLiteral("QPushButton{padding:1px;}");
        for (QPushButton* b : { _cutBtn, _copyBtn, _pasteBtn, _addBtn, _addPmBtn, _addRiBtn })
        {
            b->setFixedHeight(24);
            b->setStyleSheet(btnStyle);
        }
        _cutBtn->setMinimumWidth(38);
        _copyBtn->setMinimumWidth(38);
        _pasteBtn->setMinimumWidth(38);
        _addBtn->setMinimumWidth(64);
        _addPmBtn->setMinimumWidth(64);
        _addRiBtn->setMinimumWidth(64);
        connect(_cutBtn, &QPushButton::clicked, this, &mDefineCurveDlg::onCutClicked);
        connect(_copyBtn, &QPushButton::clicked, this, &mDefineCurveDlg::onCopyClicked);
        connect(_pasteBtn, &QPushButton::clicked, this, &mDefineCurveDlg::onPasteClicked);
        connect(_addBtn, &QPushButton::clicked, this, &mDefineCurveDlg::onAddClicked);
        //复数图目标：新增拆成 P/M 与 R/I 两个成对新增入口（命名风格随入口）
        connect(_addPmBtn, &QPushButton::clicked,
            this, [this]() { addEntryWithStyle(0); });
        connect(_addRiBtn, &QPushButton::clicked,
            this, [this]() { addEntryWithStyle(1); });
        QHBoxLayout* btnRow = new QHBoxLayout();
        btnRow->setContentsMargins(0, 0, 0, 0);
        btnRow->setSpacing(12);
        btnRow->addWidget(_cutBtn, 1);
        btnRow->addWidget(_copyBtn, 1);
        btnRow->addWidget(_pasteBtn, 1);
        btnRow->addWidget(_addBtn, 1);
        btnRow->addWidget(_addPmBtn, 1);
        btnRow->addWidget(_addRiBtn, 1);
        lay->addLayout(btnRow);
        return panel;
    }

    QWidget* mDefineCurveDlg::buildRightPanel()
    {
        _rightPanel = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(_rightPanel);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(12);

        auto makeCaption = [this](const QString& text)
            {
                QLabel* label = new QLabel(text, _rightPanel);
                label->setStyleSheet(
                    "QLabel{color:#333333;font-size:12px;font-weight:600;background:transparent;}");
                return label;
            };

        //向量：X/Y 各占一行（单选 + 整行回显编辑框，照设计图）；数学运算来源
        //默认激活拾取，视窗 Shift+左键点曲线即回填到激活向量侧（点击输入框
        //可切换目标侧），向量行不带按钮（照设计图）
        _xRadio = new QRadioButton(QStringLiteral("x"), _rightPanel);
        _yRadio = new QRadioButton(QStringLiteral("Y"), _rightPanel);
        _yRadio->setChecked(true);   //默认 Y 侧（主通道；X 侧可选，未选时 X=横坐标）
        _ymRadio = new QRadioButton(QStringLiteral("ym"), _rightPanel);   //复数图幅值侧
        //五个单选钮同为 _rightPanel 子控件，须关掉兄弟互斥，互斥改由两个
        //QButtonGroup 承担（否则点"文件"会把"X"也取消）
        _xRadio->setAutoExclusive(false);
        _yRadio->setAutoExclusive(false);
        _ymRadio->setAutoExclusive(false);
        _xEdit = new QLineEdit(_rightPanel);
        _yEdit = new QLineEdit(_rightPanel);
        _ymEdit = new QLineEdit(_rightPanel);
        _yEdit->setReadOnly(true);   //初始只读，可编辑性/启停由 refreshVectorEditability 管
        _ymEdit->setReadOnly(true);  //同上
        _xEdit->installEventFilter(this);   //点击 -> 切换拾取目标侧（数学来源默认激活拾取）
        _yEdit->installEventFilter(this);
        _ymEdit->installEventFilter(this);
        _xEdit->setFixedHeight(24);
        _yEdit->setFixedHeight(24);
        _ymEdit->setFixedHeight(24);
        connect(_xEdit, &QLineEdit::textEdited, this, &mDefineCurveDlg::onXEdited);
        connect(_yEdit, &QLineEdit::textEdited, this, &mDefineCurveDlg::onYEdited);
        connect(_ymEdit, &QLineEdit::textEdited, this, &mDefineCurveDlg::onYmEdited);
        _vectorGroup = new QButtonGroup(_rightPanel);
        _vectorGroup->addButton(_xRadio, 0);
        _vectorGroup->addButton(_yRadio, 1);
        _vectorGroup->addButton(_ymRadio, 2);
        connect(_xRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onVectorChanged(0); });
        connect(_yRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onVectorChanged(1); });
        connect(_ymRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onVectorChanged(2); });
        auto makeVectorRow = [this](QRadioButton* radio, QLineEdit* edit) -> QWidget*
            {
                QWidget* rowW = new QWidget(_rightPanel);
                QHBoxLayout* row = new QHBoxLayout(rowW);
                row->setContentsMargins(0, 0, 0, 0);
                row->setSpacing(6);
                row->addWidget(radio);
                row->addWidget(edit, 1);
                return rowW;
            };
        //向量区容器（标题 + X/Y 行）：数值来源 X 自动按索引生成、无向量概念，
        //选中"数值"时整区隐藏（截图交互）；柱状图目标下文件同样隐藏、数学来源
        //只留一行"表达式"（Y 行文案切换）；复数图目标三行 x/yp/ym（y 行文案切
        //"yp"）；显隐/形态随 refreshVectorEditability 联动
        _vectorSection = new QWidget(_rightPanel);
        QVBoxLayout* vecLay = new QVBoxLayout(_vectorSection);
        vecLay->setContentsMargins(0, 0, 0, 0);
        vecLay->setSpacing(12);
        vecLay->addWidget(makeCaption(QStringLiteral("向量：")));
        _xRowWidget = makeVectorRow(_xRadio, _xEdit);
        _yRowWidget = makeVectorRow(_yRadio, _yEdit);
        _ymRowWidget = makeVectorRow(_ymRadio, _ymEdit);
        vecLay->addWidget(_xRowWidget);
        vecLay->addWidget(_yRowWidget);
        vecLay->addWidget(_ymRowWidget);
        lay->addWidget(_vectorSection);

        //来源：文件 / 数学运算 / 数值
        QVBoxLayout* lay1 = new QVBoxLayout();
        lay1->setContentsMargins(0, 8, 0, 8);
        lay1->setSpacing(12);
        QHBoxLayout* sourceRow = new QHBoxLayout();
        sourceRow->setSpacing(12);
        _fileRadio = new QRadioButton(QStringLiteral("文件"), _rightPanel);
        _mathRadio = new QRadioButton(QStringLiteral("数学运算"), _rightPanel);
        _valueRadio = new QRadioButton(QStringLiteral("数值"), _rightPanel);
        _fileRadio->setAutoExclusive(false);
        _mathRadio->setAutoExclusive(false);
        _valueRadio->setAutoExclusive(false);
        _fileRadio->setChecked(true);
        _sourceGroup = new QButtonGroup(_rightPanel);
        _sourceGroup->addButton(_fileRadio, 0);
        _sourceGroup->addButton(_mathRadio, 1);
        _sourceGroup->addButton(_valueRadio, 2);
        connect(_fileRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onSourceChanged(0); });
        connect(_mathRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onSourceChanged(1); });
        connect(_valueRadio, &QRadioButton::toggled, this,
            [this](bool on) { if (on) onSourceChanged(2); });
        //sourceRow->addWidget(makeCaption(QStringLiteral("来源：")));
        sourceRow->addWidget(_fileRadio);
        sourceRow->addWidget(_mathRadio);
        sourceRow->addWidget(_valueRadio);
        sourceRow->addStretch(1);
        lay1->addWidget(makeCaption(QStringLiteral("来源：")));
        lay1->addLayout(sourceRow);
        lay->addLayout(lay1);

        //三张来源定义页
        _sourceStack = new QStackedWidget(_rightPanel);
        _sourceStack->addWidget(buildFilePage());
        _sourceStack->addWidget(buildMathPage());
        _sourceStack->addWidget(buildValuePage());
        lay->addWidget(_sourceStack, 1);
        return _rightPanel;
    }

    QWidget* mDefineCurveDlg::buildFilePage()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(12);

        //数据文件行（照"从文件创建曲线"：标签 + 全路径下拉 + 文件夹按钮）；
        //共享数据源仓库（与"从文件创建曲线"同一份已导入 pch）
        QHBoxLayout* fileRow = new QHBoxLayout();
        fileRow->setSpacing(12);
        QLabel* fileLabel = new QLabel(QStringLiteral("选择数据文件"), page);
        fileLabel->setStyleSheet("QLabel{background:transparent;}");
        _fileCombo = new QComboBox(page);
        _importBtn = new QToolButton(page);
        _importBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/打开文件.svg")));
        _importBtn->setFixedSize(24, 24);   //图标按钮统一规格（同"应用样式"树工具钮）
        _fileCombo->setFixedHeight(24);
        _importBtn->setToolTip(QStringLiteral(
            "导入数据文件（pch/csv/txt，与\"从文件创建曲线\"共享已导入数据）"));
        connect(_fileCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &mDefineCurveDlg::onFileComboChanged);
        connect(_importBtn, &QToolButton::clicked, this, &mDefineCurveDlg::onImportFile);
        fileRow->addWidget(fileLabel);
        fileRow->addWidget(_fileCombo, 1);
        fileRow->addWidget(_importBtn);
        lay->addLayout(fileRow);

        //四级联动列：工况 -> 结果类型 -> 对象 -> 分量（逐级过滤）。列结构照
        //"从文件创建曲线"：灰底标题条（标题 + 1/N 计数）+ 边框列表容器
        auto makeCascadeColumn = [this, page](const QString& caption, QListWidget** out,
            QLabel** count)
            {
                QFrame* head = new QFrame(page);
                head->setObjectName("myContainer");
                QHBoxLayout* headLay = new QHBoxLayout(head);
                headLay->setContentsMargins(8, 4, 8, 4);
                headLay->setSpacing(6);
                QLabel* label = new QLabel(caption, head);
                label->setStyleSheet("QLabel{background:transparent;}");
                *count = new QLabel(head);
                (*count)->setStyleSheet("QLabel{background:transparent;}");
                headLay->addWidget(label);
                headLay->addStretch(1);
                headLay->addWidget(*count);

                QFrame* bodyFrame = new QFrame(page);
                bodyFrame->setObjectName("Container");
                QVBoxLayout* bodyLay = new QVBoxLayout(bodyFrame);
                bodyLay->setContentsMargins(2, 2, 2, 2);
                bodyLay->setSpacing(0);
                *out = new QListWidget(bodyFrame);
                bodyLay->addWidget(*out, 1);

                QWidget* col = new QWidget(page);
                QVBoxLayout* colLay = new QVBoxLayout(col);
                colLay->setContentsMargins(0, 0, 0, 0);
                colLay->setSpacing(0);
                colLay->addWidget(head);
                colLay->addWidget(bodyFrame, 1);
                return col;
            };
        QHBoxLayout* listsRow = new QHBoxLayout();
        listsRow->setSpacing(12);
        //工况列容器留存成员：ascii 列数据集（csv/txt）无工况级，整列隐藏——
        //四级列退化为三级（R22）；pch 恒显
        _subcaseColumn = makeCascadeColumn(
            QStringLiteral("工况"), &_subcaseList, &_subcaseCount);
        listsRow->addWidget(_subcaseColumn, 1);
        listsRow->addWidget(makeCascadeColumn(
            QStringLiteral("结果类型"), &_schemaList, &_schemaCount), 1);
        listsRow->addWidget(makeCascadeColumn(
            QStringLiteral("对象"), &_objectList, &_objectCount), 1);
        listsRow->addWidget(makeCascadeColumn(
            QStringLiteral("分量"), &_componentList, &_componentCount), 1);
        connect(_subcaseList, &QListWidget::currentRowChanged,
            this, &mDefineCurveDlg::onSubcaseRowChanged);
        connect(_schemaList, &QListWidget::currentRowChanged,
            this, &mDefineCurveDlg::onSchemaRowChanged);
        connect(_objectList, &QListWidget::currentRowChanged,
            this, &mDefineCurveDlg::onObjectRowChanged);
        connect(_componentList, &QListWidget::currentRowChanged,
            this, &mDefineCurveDlg::onComponentRowChanged);
        lay->addLayout(listsRow, 1);
        return page;
    }

    QWidget* mDefineCurveDlg::buildMathPage()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(12);

        QLabel* builderLabel = new QLabel(QStringLiteral("表达式构建器："), page);
        builderLabel->setStyleSheet(
            "QLabel{color:#333333;font-size:12px;font-weight:600;background:transparent;}");
        lay->addWidget(builderLabel);

        //树列表容器（照文件页四级列样式）：灰底标题条 + 边框树体
        auto makeHeaderedTree = [page](QTreeWidget** out, const QString& caption)
            {
                QWidget* col = new QWidget(page);
                QVBoxLayout* colLay = new QVBoxLayout(col);
                colLay->setContentsMargins(0, 0, 0, 0);
                colLay->setSpacing(0);
                QFrame* head = new QFrame(col);
                head->setObjectName("myContainer");
                QHBoxLayout* headLay = new QHBoxLayout(head);
                headLay->setContentsMargins(8, 4, 8, 4);
                QLabel* label = new QLabel(caption, head);
                label->setStyleSheet("QLabel{background:transparent;}");
                headLay->addWidget(label);
                headLay->addStretch(1);
                QFrame* bodyFrame = new QFrame(col);
                bodyFrame->setObjectName("Container");
                QVBoxLayout* bodyLay = new QVBoxLayout(bodyFrame);
                bodyLay->setContentsMargins(2, 2, 2, 2);
                bodyLay->setSpacing(0);
                *out = new QTreeWidget(bodyFrame);
                (*out)->setHeaderHidden(true);
                (*out)->setUniformRowHeights(true);
                bodyLay->addWidget(*out, 1);
                colLay->addWidget(head);
                colLay->addWidget(bodyFrame, 1);
                return col;
            };

        //左栏：函数/曲线选项卡（搜索 + 树）+ 名称/值参数表 + 插入按钮
        _builderTabs = new QTabWidget(page);

        QWidget* funcTab = new QWidget(_builderTabs);
        QVBoxLayout* funcLay = new QVBoxLayout(funcTab);
        funcLay->setContentsMargins(0, 12, 0, 0);
        funcLay->setSpacing(12);
        _funcSearch = new QLineEdit(funcTab);
        _funcSearch->setPlaceholderText(QStringLiteral("搜索"));
        _funcSearch->setClearButtonEnabled(true);
        _funcSearch->setFixedHeight(24);
        //搜索放大镜用资源切图（不再用 Qt 主题标准图标，保证各主题下外观一致）
        _funcSearch->addAction(QIcon(QStringLiteral(":/mMainWidgets/Icon/display/fangda.png")),
            QLineEdit::LeadingPosition);
        _funcCategory = new QComboBox(funcTab);
        _funcCategory->setStyleSheet("QComboBox{padding-left:8px;}");
        _funcCategory->addItem(kAllCategories, kAllCategories);
        _funcCategory->setFixedHeight(24);
        for (const QString& cat : mCurveExpressionEvaluator::functionCategories())
            _funcCategory->addItem(cat, cat);
        connect(_funcSearch, &QLineEdit::textChanged,
            this, &mDefineCurveDlg::onFuncSearchChanged);
        connect(_funcCategory, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &mDefineCurveDlg::onFuncCategoryChanged);
        QHBoxLayout* searchRow = new QHBoxLayout();
        searchRow->setSpacing(12);
        searchRow->addWidget(_funcSearch, 1);
        searchRow->addWidget(_funcCategory);
        funcLay->addLayout(searchRow);
        funcLay->addWidget(makeHeaderedTree(&_funcTree, QStringLiteral("Templex 函数库")), 1);
        //函数库树选中条与曲线树对齐：文字区浅蓝圆角条（失焦转灰）；宿主级 QSS
        //的 selection-background-color 对分支区仍会画深蓝块，显式声明选中背景
        //透明（同对象树处理）
        _funcTree->setObjectName("funcLibTree");
        _funcTree->setItemDelegate(new CurveTreeItemDelegate(_funcTree));
        connect(_funcTree, &QTreeWidget::itemSelectionChanged,
            this, &mDefineCurveDlg::onFuncTreeSelectionChanged);
        //双击函数库条目不直接插入表达式：仅选中（联动下方参数表），插入只经
        //底部"直接插入"按钮（照设计交互）

        QWidget* objTab = new QWidget(_builderTabs);
        QVBoxLayout* objLay = new QVBoxLayout(objTab);
        objLay->setContentsMargins(0, 12, 0, 0);
        objLay->setSpacing(12);
        //"对象"标题条 + 曲线树（与曲线选择器弹窗同一棵树：p/w/c -> x/y 叶子；
        //选中曲线级行经直接插入按当前激活向量补 .x/.y，选中叶子直接插入）
        objLay->addWidget(makeHeaderedTree(&_objTree, QStringLiteral("对象")), 1);
        //对象树（曲线选择）选中条与曲线树对齐：文字区浅蓝圆角条（失焦转灰），
        //无原生深蓝选中底
        _objTree->setItemDelegate(new CurveTreeItemDelegate(_objTree));
        //宿主级 QSS 给 QTreeWidget 统一配了 selection-background-color:#0066CC
        //（优先级高于调色板，行代理管不到分支区），对象树显式声明选中背景透明
        //——选中条由行代理自绘，分支区不再出现深蓝块
        _objTree->setObjectName("objRefTree");
        connect(_objTree, &QTreeWidget::itemSelectionChanged,
            this, &mDefineCurveDlg::onObjTreeSelectionChanged);
        connect(_objTree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem*, int) { onInsertDirectClicked(); });

        _builderTabs->addTab(funcTab, QStringLiteral("函数"));
        _builderTabs->addTab(objTab, QStringLiteral("曲线"));
        connect(_builderTabs, &QTabWidget::currentChanged, this, [this](int)
        {
            //参数表（名称/值）只属于函数页：切到曲线页隐藏（照参考图二，
            //曲线页仅对象树 + 插入按钮），避免残留上次所选函数的参数行
            _funcParamTable->setVisible(_builderTabs->currentIndex() == 0);
            refreshInsertButtons();
        });

        _funcParamTable = new QTableWidget(page);
        _funcParamTable->setColumnCount(2);
        _funcParamTable->setHorizontalHeaderLabels({
            QStringLiteral("名称"), QStringLiteral("值") });
        _funcParamTable->verticalHeader()->setVisible(false);
        _funcParamTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _funcParamTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        _funcParamTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        _funcParamTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

        _directInsertBtn = new QPushButton(QStringLiteral("直接插入"), page);
        _useExprBtn = new QPushButton(QStringLiteral("用于表达式"), page);
        _directInsertBtn->setToolTip(
            QStringLiteral("插入到当前向量：X=替换 X 引用，Y=插入表达式光标处"));
        _useExprBtn->setToolTip(QStringLiteral("插入到表达式编辑框光标处"));
        _directInsertBtn->setFixedHeight(24);
        _useExprBtn->setFixedHeight(24);
        connect(_directInsertBtn, &QPushButton::clicked, this, &mDefineCurveDlg::onInsertDirectClicked);
        connect(_useExprBtn, &QPushButton::clicked, this, &mDefineCurveDlg::onInsertToExprClicked);
        QHBoxLayout* insRow = new QHBoxLayout();
        insRow->setSpacing(12);
        insRow->addWidget(_directInsertBtn, 1);
        insRow->addWidget(_useExprBtn, 1);

        QVBoxLayout* leftCol = new QVBoxLayout();
        leftCol->setSpacing(12);
        leftCol->addWidget(_builderTabs, 5);
        leftCol->addWidget(_funcParamTable, 3);
        leftCol->addLayout(insRow);

        //右栏：表达式填写（简写编辑框 + 清空）+ 计算求解 + 值预览
        QVBoxLayout* rightCol = new QVBoxLayout();
        rightCol->setSpacing(12);
        QLabel* exprCaption = new QLabel(QStringLiteral("表达式："), page);
        exprCaption->setStyleSheet(
            "QLabel{color:#333333;font-size:12px;font-weight:600;background:transparent;}");
        rightCol->addWidget(exprCaption);
        QHBoxLayout* exprRow = new QHBoxLayout();
        exprRow->setSpacing(12);
        QLabel* exprLabel = new QLabel(QStringLiteral("表达式简写"), page);
        exprLabel->setStyleSheet("QLabel{background:transparent;}");
        _exprEdit = new QLineEdit(page);
        _exprEdit->setPlaceholderText(
            QStringLiteral("表达式填写：y = f(x)，x 为自变量，p1w1c1.y 为曲线向量引用"));
        _exprEdit->setFixedHeight(24);
        _exprClearBtn = new QToolButton(page);
        //清空用资源切图（post/qingkong，语义同 tooltip"清空表达式"）
        _exprClearBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/post/qingkong.png")));
        _exprClearBtn->setIconSize(QSize(16, 16));
        _exprClearBtn->setToolTip(QStringLiteral("清空表达式"));
        _exprClearBtn->setFixedSize(24, 24);
        connect(_exprEdit, &QLineEdit::textEdited, this, &mDefineCurveDlg::onExprEdited);
        connect(_exprClearBtn, &QToolButton::clicked, this, &mDefineCurveDlg::onExprClearClicked);
        exprRow->addWidget(exprLabel);
        exprRow->addWidget(_exprEdit, 1);
        exprRow->addWidget(_exprClearBtn);
        rightCol->addLayout(exprRow);

        //计算求解：勾选=按 X 采样求值；不勾=表达式须为单个曲线向量引用（直接取值）
        _solveCheck = new QCheckBox(QStringLiteral("计算求解"), page);
        _solveCheck->setToolTip(QStringLiteral(
            "勾选：按 X 向量采样逐点求值（引用向量按 x 线性插值）；\n"
            "不勾选：表达式必须为单个曲线向量引用（如 p1w1c1.y），数据直接复制"));
        _solveCheck->setChecked(true);
        connect(_solveCheck, &QCheckBox::toggled, this, &mDefineCurveDlg::onSolveToggled);
        rightCol->addWidget(_solveCheck);

        QLabel* previewCaption = new QLabel(QStringLiteral("预览："), page);
        previewCaption->setStyleSheet(
            "QLabel{color:#333333;font-size:12px;font-weight:600;background:transparent;}");
        rightCol->addWidget(previewCaption);
        _previewTable = new QTableWidget(page);
        _previewTable->setColumnCount(1);
        _previewTable->setHorizontalHeaderLabels({ QStringLiteral("值") });
        _previewTable->verticalHeader()->setVisible(false);
        _previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _previewTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        _previewTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        rightCol->addWidget(_previewTable, 1);

        QHBoxLayout* columns = new QHBoxLayout();
        columns->setSpacing(12);
        columns->addLayout(leftCol, 6);
        QFrame* vLine = new QFrame();
        vLine->setFixedWidth(1);
        vLine->setStyleSheet("background:#DDE0E7;");
        columns->addWidget(vLine);
        columns->addLayout(rightCol, 5);
        lay->addLayout(columns, 1);
        return page;
    }

    QWidget* mDefineCurveDlg::buildValuePage()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(8);

        //行操作工具条（截图参考软件：新增/删除行图标钮悬在表格左上）
        QHBoxLayout* toolBar = new QHBoxLayout();
        toolBar->setSpacing(6);
        _addValueRowBtn = new QToolButton(page);
        _addValueRowBtn->setIcon(
            QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/添加.svg")));
        _addValueRowBtn->setFixedSize(24, 24);   //图标按钮统一规格（同"导入"钮）
        _addValueRowBtn->setToolTip(QStringLiteral("新增一行数值"));
        connect(_addValueRowBtn, &QToolButton::clicked,
            this, &mDefineCurveDlg::onAddValueRow);
        _delValueRowBtn = new QToolButton(page);
        _delValueRowBtn->setIcon(
            QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/删除.svg")));
        _delValueRowBtn->setFixedSize(24, 24);
        _delValueRowBtn->setToolTip(QStringLiteral("删除选中行"));
        _delValueRowBtn->setEnabled(false);   //无选中行时不可删（随选择联动）
        connect(_delValueRowBtn, &QToolButton::clicked,
            this, &mDefineCurveDlg::onDeleteValueRow);
        toolBar->addWidget(_addValueRowBtn);
        toolBar->addWidget(_delValueRowBtn);
        //清空钮（设计稿工具条 [+][-][清空] 第三钮）：一键清空数值表全部行。
        //资源图标库（resource/Icon/Curve/操作/）无垃圾桶/清空类图标，按用户拍板
        //先以文字占位，不自行绘制。TODO：清空图标后续提供后补一行 setIcon
        //（照"添加/删除"钮资源路径风格），ToolButtonTextBesideIcon 已预留，
        //届时即成"图标+文字"形态
        _clearValueBtn = new QToolButton(page);
        _clearValueBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        _clearValueBtn->setText(QStringLiteral("清空"));
        _clearValueBtn->setFixedHeight(24);   //高度同既有图标钮，宽度随文字自适应
        _clearValueBtn->setToolTip(QStringLiteral("清空全部行"));
        _clearValueBtn->setEnabled(false);   //表无行不可清（随行数联动，与选中无关）
        connect(_clearValueBtn, &QToolButton::clicked,
            this, &mDefineCurveDlg::onClearValueRows);
        toolBar->addWidget(_clearValueBtn);
        toolBar->addStretch(1);
        lay->addLayout(toolBar);

        //数值表（截图样式，三列）：Index 列只读回显序号（0 起，与 X 自动生成
        //0..N-1 同源）；Categories 列输入横坐标刻度名；第三列表头=条目显示名
        //（随"曲线名称"联动，refreshValueColumnHeader），输入纵坐标数值
        _valueTable = new QTableWidget(page);
        _valueTable->setColumnCount(3);
        _valueTable->setHorizontalHeaderLabels(
            { QStringLiteral("Index"), QStringLiteral("Categories"), QString() });
        _valueTable->setEditTriggers(QAbstractItemView::DoubleClicked
            | QAbstractItemView::SelectedClicked
            | QAbstractItemView::EditKeyPressed
            | QAbstractItemView::AnyKeyPressed);
        //整行选择（截图交互）：单击该行任意位置选中整行；Ctrl 反选多行、
        //Shift 范围多选（ExtendedSelection 自带）
        _valueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        _valueTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        //右键菜单：在上方/下方插入一行、剪切、复制、粘贴、删除（截图）
        _valueTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(_valueTable, &QWidget::customContextMenuRequested,
            this, &mDefineCurveDlg::onValueContextMenu);
        _valueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        _valueTable->setColumnWidth(0, 90);
        _valueTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        _valueTable->setColumnWidth(1, 120);
        _valueTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        _valueTable->horizontalHeader()->setDefaultAlignment(
            Qt::AlignLeft | Qt::AlignVCenter);   //与预览表表头对齐方式一致
        //选中行淡蓝（截图 #D0E7FD，与左栏列表选中条目同配色），文字保持深色；
        //行头选中格同步淡蓝。表级 QSS 规则收口：全局 QSS 未定义表格
        //::item:selected 时 Windows 主题按系统高亮色绘制，调色板压不过主题，
        //须以表级样式规则（优先级高于应用样式表/主题）显式指定
        _valueTable->setStyleSheet(QStringLiteral(
            "QTableWidget::item:selected{background:#D0E7FD;color:#303133;}"
            "QHeaderView::section:vertical:selected{background:#D0E7FD;}"));
        connect(_valueTable, &QTableWidget::cellChanged,
            this, &mDefineCurveDlg::onValueCellChanged);
        connect(_valueTable, &QTableWidget::itemSelectionChanged, this, [this]()
        {
            refreshValueToolBarStates();   //删除钮随选中行、清空钮随表行数一并刷新
        });
        lay->addWidget(_valueTable, 1);
        return page;
    }

    // =============================================================================
    // 条目与状态
    // =============================================================================

    QString mDefineCurveDlg::nextCurveName() const
    {
        //与 curveNodeDefaultName 同款 HyperGraph 风格（弹窗内编号会话自增）
        return QStringLiteral("Curve %1").arg(_counter + 1);
    }

    int mDefineCurveDlg::currentEntryIndex() const
    {
        return (_current >= 0 && _current < _entries.size()) ? _current : -1;
    }

    QString mDefineCurveDlg::entryAutoName(const DefineCurveEntry& e) const
    {
        if (targetIsBarChart())
        {
            //柱状图目标：整条来源语义（现状零变化）
            switch (e.source)
            {
            case 0:
                if (e.fileComplete())
                {
                    //ascii 列数据集自动名=列名（R22）；pch=对象-分量（现状零变化）
                    if (isAsciiDataset(e.filePath))
                        return e.yChannel.component;
                    return e.yChannel.objectText + QStringLiteral("-") + e.yChannel.component;
                }
                break;
            case 1:
            {
                const QString t = e.expression.trimmed();
                if (!t.isEmpty())
                    return t.size() > 24 ? t.left(24) + QStringLiteral("…") : t;
                break;
            }
            case 2:
                if (!parseValues(e.valuesText).isEmpty())
                    return QStringLiteral("数值曲线");
                break;
            default:
                break;
            }
            return QString();
        }
        //常规/复数图目标：自动命名按主侧（Y/ym）自身来源口径
        const bool complexChart = targetIsComplexChart();
        const int mainSide = complexChart ? 2 : 1;
        switch (sideSource(e, mainSide))
        {
        case 0:
            if (complexChart)
            {
                //复数对自动名（对齐参考"对象 - 方向"）：方向取幅值通道分量去
                //MAG|令牌、连字符转空格（X-Rot -> X Rot）
                if (sideFileComplete(e, 2))
                {
                    //ascii 列数据集自动名=列名（R22，无幅值/相位令牌语义）
                    if (isAsciiDataset(sideFilePath(e, 2)))
                        return e.ymChannel.component;
                    QString dir = e.ymChannel.component;
                    if (dir.startsWith(QStringLiteral("MAG|"), Qt::CaseInsensitive))
                        dir = dir.mid(4);
                    return e.ymChannel.objectText + QStringLiteral(" - ")
                        + QString(dir).replace(QLatin1Char('-'), QLatin1Char(' '));
                }
                break;
            }
            if (sideFileComplete(e, 1))
            {
                //ascii 列数据集自动名=列名（R22）；pch=对象-分量（现状零变化）
                if (isAsciiDataset(sideFilePath(e, 1)))
                    return e.yChannel.component;
                return e.yChannel.objectText + QStringLiteral("-") + e.yChannel.component;
            }
            break;
        case 1:
        {
            const QString t = sideMathText(e, mainSide).trimmed();
            if (!t.isEmpty())
                return t.size() > 24 ? t.left(24) + QStringLiteral("…") : t;
            break;
        }
        case 2:
            if (!parseValues(sideValueText(e, mainSide)).isEmpty())
                return QStringLiteral("数值曲线");
            break;
        default:
            break;
        }
        return QString();
    }

    bool mDefineCurveDlg::entryComplete(const DefineCurveEntry& e) const
    {
        if (targetIsBarChart())
        {
            //柱状图目标：整条来源语义（现状零变化）
            switch (e.source)
            {
            case 0:
                return e.fileComplete();
            case 1:
                return !e.expression.trimmed().isEmpty();
            case 2:
                return !parseValues(e.valuesText).isEmpty();
            default: return false;
            }
        }
        //完整性按各侧自身来源口径判定：Y/yp/ym 主侧必选、X 侧可选有兜底
        //(现行语义)；混合来源条目=各侧均达自身完整口径
        const bool mainOk = [&]
        {
            switch (sideSource(e, 1))
            {
            case 0:  return sideFileComplete(e, 1);
            case 1:  return !sideMathText(e, 1).trimmed().isEmpty();
            case 2:  return !parseValues(sideValueText(e, 1)).isEmpty();
            default: return false;
            }
        }();
        if (!mainOk)
            return false;
        if (!targetIsComplexChart())
            return true;
        const bool ymOk = [&]
        {
            switch (sideSource(e, 2))
            {
            case 0:  return sideFileComplete(e, 2);
            case 1:  return !sideMathText(e, 2).trimmed().isEmpty();
            case 2:  return !parseValues(sideValueText(e, 2)).isEmpty();
            default: return false;
            }
        }();
        return ymOk;
    }

    // =============================================================================
    // 按向量侧的状态访问（0=x 1=Y/yp 2=ym）
    //
    // 常规 XY 与复数图目标的来源按向量侧独立持有；柱状图目标保留整条 source
    // 语义——下列访问器在柱状图目标下统一退化为整条字段（sideSource/
    // sideFilePath 映射 e.source/e.filePath，其余映射 Y 侧字段），保证柱状
    // 路径行为与按侧拆分前逐字一致。sideSource 为有效来源口径：未显式设置
    // （哨兵 -1）的侧跟随整条镜像（R20-B01，见函数内注）
    // =============================================================================

    int mDefineCurveDlg::sideSource(const DefineCurveEntry& e, int vector) const
    {
        if (targetIsBarChart())
            return e.source;
        //有效来源口径（R20-B01）：未显式设置（哨兵 -1）的侧跟随整条 source
        //镜像（=最近一次显式选择；新条目镜像 0=文件，与拆分前出厂默认同值，
        //既有 ==0/1/2 判定点对新条目行为逐字不变）；显式设置过（≥0）的侧取
        //自身值——各侧自此独立
        const int raw = [&]
        {
            switch (vector)
            {
            case 0:  return e.xSource;
            case 1:  return targetIsComplexChart() ? e.ypSource : e.ySource;
            default: return e.ymSource;
            }
        }();
        return raw < 0 ? e.source : raw;
    }

    void mDefineCurveDlg::setSideSource(DefineCurveEntry& e, int vector, int source) const
    {
        //恒写按侧原始字段（只收显式选择；哨兵判定/镜像同步在调用方）
        if (targetIsBarChart())
        {
            e.source = source;
            return;
        }
        switch (vector)
        {
        case 0:  e.xSource = source; break;
        case 1:
            if (targetIsComplexChart()) e.ypSource = source; else e.ySource = source;
            break;
        default: e.ymSource = source; break;
        }
    }

    QString mDefineCurveDlg::sideFilePath(const DefineCurveEntry& e, int vector) const
    {
        if (targetIsBarChart())
            return e.filePath;
        switch (vector)
        {
        case 0:  return e.xFilePath;
        case 1:  return targetIsComplexChart() ? e.ypFilePath : e.yFilePath;
        default: return e.ymFilePath;
        }
    }

    void mDefineCurveDlg::setSideFilePath(DefineCurveEntry& e, int vector, const QString& path) const
    {
        if (targetIsBarChart())
        {
            e.filePath = path;
            return;
        }
        switch (vector)
        {
        case 0:  e.xFilePath = path; break;
        case 1:
            if (targetIsComplexChart()) e.ypFilePath = path; else e.yFilePath = path;
            break;
        default: e.ymFilePath = path; break;
        }
    }

    DefineCurveEntry::FileChannel& mDefineCurveDlg::sideChannel(DefineCurveEntry& e, int vector) const
    {
        switch (vector)
        {
        case 0:  return e.xChannel;
        case 1:  return targetIsComplexChart() ? e.ypChannel : e.yChannel;
        default: return e.ymChannel;
        }
    }

    const DefineCurveEntry::FileChannel& mDefineCurveDlg::sideChannel(
        const DefineCurveEntry& e, int vector) const
    {
        switch (vector)
        {
        case 0:  return e.xChannel;
        case 1:  return targetIsComplexChart() ? e.ypChannel : e.yChannel;
        default: return e.ymChannel;
        }
    }

    QString& mDefineCurveDlg::sideValueText(DefineCurveEntry& e, int vector) const
    {
        switch (vector)
        {
        case 0:  return e.pairXText;
        case 1:  return targetIsComplexChart() ? e.pairYpText : e.valuesText;
        default: return e.pairYmText;
        }
    }

    const QString& mDefineCurveDlg::sideValueText(const DefineCurveEntry& e, int vector) const
    {
        switch (vector)
        {
        case 0:  return e.pairXText;
        case 1:  return targetIsComplexChart() ? e.pairYpText : e.valuesText;
        default: return e.pairYmText;
        }
    }

    bool mDefineCurveDlg::sideValueDirty(const DefineCurveEntry& e, int vector) const
    {
        switch (vector)
        {
        case 0:  return e.xValueDirty;
        case 1:  return targetIsComplexChart() ? e.ypValueDirty : e.yValueDirty;
        default: return e.ymValueDirty;
        }
    }

    void mDefineCurveDlg::setSideValueDirty(DefineCurveEntry& e, int vector, bool dirty) const
    {
        switch (vector)
        {
        case 0:  e.xValueDirty = dirty; break;
        case 1:
            if (targetIsComplexChart()) e.ypValueDirty = dirty; else e.yValueDirty = dirty;
            break;
        default: e.ymValueDirty = dirty; break;
        }
    }

    QString mDefineCurveDlg::sideMathText(const DefineCurveEntry& e, int vector) const
    {
        //该侧数学内容：X=单引用（xText），Y=表达式（XY），yp/ym=各自表达式槽；
        //柱状图目标恒 Y 侧表达式
        switch (vector)
        {
        case 0:  return e.xText;
        case 1:  return targetIsComplexChart() ? e.ypText : e.expression;
        default: return e.ymText;
        }
    }

    bool mDefineCurveDlg::sideFileComplete(const DefineCurveEntry& e, int vector) const
    {
        return !sideFilePath(e, vector).isEmpty() && sideChannel(e, vector).complete();
    }

    bool mDefineCurveDlg::sideHasContent(const DefineCurveEntry& e, int vector) const
    {
        //该侧任一来源有内容（文件=通道完整可用；数学=内容非空；数值=数值串非空）
        switch (sideSource(e, vector))
        {
        case 0:  return sideFileComplete(e, vector);
        case 1:  return !sideMathText(e, vector).trimmed().isEmpty();
        case 2:  return !sideValueText(e, vector).trimmed().isEmpty();
        default: return false;
        }
    }

    void mDefineCurveDlg::refreshActionStates()
    {
        const int cur = currentEntryIndex();
        const bool hasSel = cur >= 0;
        const bool catRow = categoriesRowActive();   //柱状图 Categories 行
        _cutBtn->setEnabled(hasSel);
        _copyBtn->setEnabled(hasSel);
        _pasteBtn->setEnabled(_hasClip);
        _nameCheck->setEnabled(hasSel);
        _nameEdit->setEnabled(hasSel && _entries.at(cur).useName);
        //右栏：条目选中，或柱状图选中 Categories 行（共享数值表仍可编辑）时可用
        _rightPanel->setEnabled(hasSel || (catRow && !_entries.isEmpty()));

        //复数图目标：新增拆成 P/M 与 R/I 两个成对入口（截图按钮形态）
        const bool complexTarget = targetIsComplexChart();
        _addBtn->setVisible(!complexTarget);
        _addPmBtn->setVisible(complexTarget);
        _addRiBtn->setVisible(complexTarget);
        //可见集变化后布局等分自动重排（最小宽防截字），无需固定尺寸

        bool anyComplete = false;
        for (const DefineCurveEntry& e : _entries)
            anyComplete = anyComplete || entryComplete(e);
        _applyBtn->setEnabled(anyComplete);
    }

    void mDefineCurveDlg::loadEntryToPanel()
    {
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];   //可写：XY 目的地归一残留 ym 激活侧（下）
        //XY 目的地：复数期残留的 activeVector==2 先归一为主侧 1（同 ctor 切型
        //口径——须在 activeSide/来源/四级列计算之前，否则本轮回显仍按 side=2
        //分家；不归一则点亮 x 而激活侧仍 2，数值表无可编辑列/来源点击按
        //side=2 静默错侧，R21 评审）
        if (!targetIsBarChart() && !targetIsComplexChart() && e.activeVector == 2)
            e.activeVector = 1;

        _syncing = true;
        _nameCheck->setChecked(e.useName);
        _nameEdit->setText(e.name);
        //来源单选/定义页显示激活向量侧的来源（柱状图目标=整条来源）
        const int activeSide = qBound(0, e.activeVector, 2);
        const int source = targetIsBarChart() ? e.source : sideSource(e, activeSide);
        switch (source)
        {
        case 1:  _mathRadio->setChecked(true); break;
        case 2:  _valueRadio->setChecked(true); break;
        default: _fileRadio->setChecked(true); break;
        }
        _sourceStack->setCurrentIndex(source);

        //文件页级联回填（四级列按当前向量侧的通道选择回填；选择器信号因
        //syncing 不写回条目）；文件下拉回显该侧文件（按侧独立）
        const QString sidePath = targetIsBarChart()
            ? e.filePath : sideFilePath(e, activeSide);
        const int fi = _fileCombo->findData(sidePath);
        _fileCombo->setCurrentIndex(fi);
        refreshFileColumns();

        //向量回显（复数图：Y 行即相位侧）
        _xEdit->setText(e.xText);
        _yEdit->setText(targetIsComplexChart() ? e.ypText : e.yText);
        _ymEdit->setText(e.ymText);
        //向量单选：柱状图恒表达式侧；复数图三槽按条目记录回填；XY 的
        //activeVector==2 残留已在函数头归一
        if (targetIsBarChart())
            _yRadio->setChecked(true);
        else if (targetIsComplexChart())
        {
            _xRadio->setChecked(e.activeVector == 0);
            _yRadio->setChecked(e.activeVector == 1);
            _ymRadio->setChecked(e.activeVector == 2);
        }
        else
            (e.activeVector == 1 ? _yRadio : _xRadio)->setChecked(true);

        //数学页
        fillFuncParams(QString());
        _exprEdit->setText(e.expression);
        _solveCheck->setChecked(e.solve);
        rebuildObjectTree();

        //数值页
        if (targetIsBarChart())
        {
            //柱状图：共享数值表常驻（编辑即时落库），切条目只同步列形态/表头；
            //行被清且条目带数据（复位/类型切换后）时整表重建
            syncValueTableShape();
            bool anyData = false;
            for (const DefineCurveEntry& it : _entries)
                anyData = anyData || !it.categoriesText.trimmed().isEmpty()
                    || !it.valuesText.trimmed().isEmpty();
            if (_valueTable->rowCount() == 0 && anyData)
            {
                for (int i = 0; i < _entries.size(); ++i)
                    fillBarColumnFromEntry(i);
            }
        }
        else if (targetIsComplexChart())
        {
            //复数图：每条目独立 X/Yp/Ym 表，切条目整表按各侧来源展开（编辑即时读回条目）
            syncValueTableShape();
            fillValueTableSides(e);
        }
        else
        {
            //XY/复数图（设计图 Pt|X|Y / Pt|X|Yp|Ym）：每条目独立表，切条目整表
            //按各侧来源展开（数值侧列=该侧数值串，非数值侧列=该侧装配数据只读）
            syncValueTableShape();
            fillValueTableSides(e);
        }
        _syncing = false;

        refreshVectorEditability();
        refreshInsertButtons();
        refreshPreview();
        refreshValueToolBarStates();   //切条目回填不触发选择信号（插行辅助静默），重申启停
    }

    void mDefineCurveDlg::syncPanelToEntry()
    {
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        e.useName = _nameCheck->isChecked();
        e.name = _nameEdit->text();
        e.activeVector = qBound(0, _vectorGroup->checkedId(), 2);
        e.xText = _xEdit->text();
        e.yText = _yEdit->text();
        if (targetIsComplexChart())
        {
            //复数图：yp/ym 框文本 = 通道回显（文件）或表达式（数学）
            e.ypText = _yEdit->text();
            e.ymText = _ymEdit->text();
        }
        //来源单选只写激活向量侧（柱状图目标=整条 source 语义不拆）。非柱状
        //兜底收集只在单选与该侧有效来源不一致时落笔：单选正常只是镜像显示
        //未设置侧（哨兵 -1）的来源，照写会把未设置冒充成显式选择，破坏
        //"首次显式选择全侧落定"（R20-B01）；显式选择本就经 onSourceChanged
        //落库，此处仅为防御性兜底
        const int source = _fileRadio->isChecked() ? 0 : (_mathRadio->isChecked() ? 1 : 2);
        if (targetIsBarChart())
            e.source = source;
        else if (source != sideSource(e, e.activeVector))
            setSideSource(e, e.activeVector, source);
        e.expression = _exprEdit->text();
        e.solve = _solveCheck->isChecked();
        //柱状图共享表编辑即落库（commitValueTableToEntry 全列写入），无需兜底读回
        if (!targetIsBarChart())
            readValueTableIntoEntry(e);
    }

    void mDefineCurveDlg::clearPanelState()
    {
        _syncing = true;
        _nameCheck->setChecked(false);
        _nameEdit->clear();
        _yRadio->setChecked(true);
        _xEdit->clear();
        _yEdit->clear();
        _ymEdit->clear();
        _ymRadio->setChecked(false);
        _fileRadio->setChecked(true);
        _sourceStack->setCurrentIndex(0);
        _fileCombo->setCurrentIndex(-1);
        refreshSubcaseColumnVisible();   //工况列复位为显示（清空后无 ascii 文件）
        _subcaseList->clear();
        _schemaList->clear();
        _objectList->clear();
        _componentList->clear();
        _subcaseCount->clear();
        _schemaCount->clear();
        _objectCount->clear();
        _componentCount->clear();
        fillFuncParams(QString());
        _exprEdit->clear();
        _solveCheck->setChecked(true);
        _valueTable->setRowCount(0);
        refreshValueToolBarStates();   //删除/清空钮一并置灰（表已空）
        syncValueTableShape();   //列数/表头随模式复位（常规 3 列 / 柱状共享表按条目集）
        _previewTable->setRowCount(0);
        refreshInsertButtons();
        refreshVectorEditability();
        _syncing = false;
    }

    void mDefineCurveDlg::resetDialog()
    {
        _entries.clear();
        _current = -1;
        _hasClip = false;
        _counter = 0;
        _curveList->blockSignals(true);
        _curveList->clear();
        _curveList->blockSignals(false);
        syncBarListShape();   //柱状图目标：列表首行挂 Categories（共享刻度名编辑入口）
        clearPanelState();
        refreshActionStates();
    }

    //曲线拾取目标侧记录（数学来源默认激活、随激活向量侧跟随，点击输入框仅
    //切换目标侧；无视觉提示，-1=非数学来源/数值输入时不响应拾取）
    void mDefineCurveDlg::setPickingVector(int vector)
    {
        _pickingVector = vector;
    }

    // =============================================================================
    // 左栏：列表与按钮
    // =============================================================================

    void mDefineCurveDlg::updateListItemText(int entryIndex)
    {
        if (entryIndex < 0 || entryIndex >= _entries.size())
            return;
        const int row = entryToListRow(entryIndex);   //柱状图首行=Categories，条目行号 +1
        if (row < 0 || row >= _curveList->count())
            return;
        //列表名与图例/曲线名同源：名称框文本非空即用，空则回退自动名
        _curveList->item(row)->setText(entryDisplayName(entryIndex));
    }

    void mDefineCurveDlg::onAddClicked()
    {
        addEntryWithStyle(0);   //常规新增：P/M 命名风格
    }

    void mDefineCurveDlg::addEntryWithStyle(int pairStyle)
    {
        syncPanelToEntry();   //切走前保存当前条目编辑
        const int cur = currentEntryIndex();
        const bool bar = targetIsBarChart();
        //当前条目已有数据时先确认（截图流程）：新增条目不携带当前数据，编辑
        //视图切到空条目；取消则停留。空条目/首次新增不打扰。
        //柱状图数值来源例外：数值表为全部条目共享，新增只是表尾右扩一列、
        //已有行数据保留（设计图交互），直接新增不打扰
        if (cur >= 0)
        {
            const DefineCurveEntry& ce = _entries.at(cur);
            bool hasData;
            if (bar)
            {
                //柱状图整条语义（现状零变化）
                hasData = ce.fileComplete()
                    || !ce.expression.trimmed().isEmpty()
                    || !ce.valuesText.trimmed().isEmpty()
                    || !ce.pairXText.trimmed().isEmpty()
                    || !ce.pairYpText.trimmed().isEmpty()
                    || !ce.pairYmText.trimmed().isEmpty();
            }
            else
            {
                //常规/复数图：任一向量侧有内容即视为有数据（按侧口径）
                hasData = false;
                const int sides = targetIsComplexChart() ? 3 : 2;
                for (int s = 0; s < sides; ++s)
                    hasData = hasData || sideHasContent(ce, s);
            }
            //柱状图共享表/复数图每条目独立表均编辑即落库，新增条目各表数据保留。
            //柱状=整条数值语义；复数图=任一向量侧为数值（内容在表，按侧口径）
            const bool sharedValues = bar
                ? ce.source == 2
                : (targetIsComplexChart()
                    && (sideSource(ce, 0) == 2 || sideSource(ce, 1) == 2
                        || sideSource(ce, 2) == 2));
            if (hasData && !sharedValues
                && MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("提示"),
                    QStringLiteral("当前条目已存在数据，新增条目不保留当前数据。是否继续？"),
                    QStringLiteral("取消"), QStringLiteral("继续"), QString(), 1, 0) != 1)
                return;
        }
        DefineCurveEntry e;
        e.name = nextCurveName();
        e.activeVector = 0;   //新增条目默认选中 X（先定义横坐标向量；柱状图目标强制表达式侧）
        e.filePath = _fileCombo->currentData().toString();   //预置当前数据文件（柱状整条语义）
        //新增条目继承当前来源（截图交互：数学→数学、文件→文件、数值→数值，
        //不跳回文件页，X/Y 栏形态随来源保持一致）；无当前条目时默认文件。
        //常规/复数图目标按向量侧继承各侧来源与文件（通道不继承，重新选择）
        if (cur >= 0)
        {
            const DefineCurveEntry& ce = _entries.at(cur);
            e.source = ce.source;
            if (!bar)
            {
                e.xSource = ce.xSource;
                e.ySource = ce.ySource;
                e.ypSource = ce.ypSource;
                e.ymSource = ce.ymSource;
                e.xFilePath = ce.xFilePath;
                e.yFilePath = ce.yFilePath;
                e.ypFilePath = ce.ypFilePath;
                e.ymFilePath = ce.ymFilePath;
            }
        }
        e.pairStyle = pairStyle;   //复数图数学来源成对命名风格（P/M 或 R/I）
        ++_counter;
        _entries.push_back(e);

        _curveList->blockSignals(true);
        _curveList->addItem(e.name);   //柱状图列表首行为 Categories，条目顺序追加其后
        _curveList->blockSignals(false);
        _current = _entries.size() - 1;
        if (bar)
        {
            //共享数值表直接右扩新曲线列（设计图：新增即在列表右侧出现 Curve N 列）。
            //新条目无数据时回填不会给既有行造格（缺 item 的格不可编辑），故补格
            //并置默认值 "0"（对齐既有列观感，应用即可用），随后落库使条目与表
            //一致（categoriesText 共享列一并同步）
            _syncing = true;
            syncValueTableShape();
            fillBarColumnFromEntry(_current);
            const int col = 2 + _current;
            for (int r = 0; r < _valueTable->rowCount(); ++r)
            {
                if (!_valueTable->item(r, col))
                    _valueTable->setItem(r, col, new QTableWidgetItem);
                if (QTableWidgetItem* v = _valueTable->item(r, col))
                {
                    v->setText(QStringLiteral("0"));
                    markValueCellValidity(r, col);
                }
            }
            _syncing = false;
            commitValueTableToEntry();
        }
        _syncing = true;                 //选中行信号不写回（面板仍是旧条目内容）
        _curveList->setCurrentRow(entryToListRow(_current));
        _syncing = false;
        loadEntryToPanel();              //右栏随选中载入新条目
        //按剪贴板/定义完整度维持置灰）
        refreshActionStates();
        _nameEdit->setFocus();
        _nameEdit->selectAll();
    }

    void mDefineCurveDlg::addEntryFromCurve(int curveId)
    {
        //编辑模式（设计图：选中复数/XY曲线点"定义曲线"）：把既有曲线载入为
        //单条目——名称随节点，来源按溯源字段还原（"数值"/"数学运算"为保留字，
        //其余=文件路径），应用时原位更新该曲线节点
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        CurveNode* n = doc->node(curveId);
        if (!n || n->type != CurveNodeType::Curve)
            return;
        const CurveSeriesData& s = n->series;
        const bool complexCurve = s.isComplex();

        DefineCurveEntry e;
        e.editCurveId = curveId;
        e.useName = true;
        e.name = n->name;
        e.activeVector = 1;
        e.pairStyle = complexCurve && s.complexFormat == 1 ? 1 : 0;   //展开方式随模型

        if (s.sourceFile == kNumericSourceTag)
        {
            //数值来源曲线：各向量侧均还原为数值（当前数据倒各侧列）
            e.source = 2;
            e.xSource = 2;
            e.ySource = 2;
            if (complexCurve)
            {
                e.ypSource = 2;
                e.ymSource = 2;
            }
            dumpCurveValuesToEntry(curveId, e);   //数值来源：当前数据直接进表
        }
        else if (s.sourceFile == kMathSourceTag)
        {
            //数学来源曲线：各向量侧均还原为数学（表达式持久化在 x/y(ym/yp)
            //壳节点——定义曲线数学来源落模点；X 侧未持久化引用时为空，
            //求值采样按现状兜底）
            e.source = 1;
            e.xSource = 1;
            e.ySource = 1;
            if (complexCurve)
            {
                e.ypSource = 1;
                e.ymSource = 1;
            }
            for (int childId : doc->childNodeIds(curveId))
            {
                const CurveNodeType t = doc->nodeType(childId);
                if (t == CurveNodeType::CurveYData)
                {
                    if (complexCurve)
                        e.ymText = doc->nodeProperty(childId, CurveProp::kExpression).toString();
                    else
                        e.expression = doc->nodeProperty(childId, CurveProp::kExpression).toString();
                }
                else if (t == CurveNodeType::CurveY2Data)
                    e.ypText = doc->nodeProperty(childId, CurveProp::kExpression).toString();
                else if (t == CurveNodeType::CurveXData)
                    e.xText = doc->nodeProperty(childId, CurveProp::kExpression).toString();
            }
            //XY 数学页 Y 框与表达式同源互显（onExprEdited 双写口径），还原时同步
            if (!complexCurve)
                e.yText = e.expression;
        }
        else if (!s.sourceFile.isEmpty())
        {
            //文件来源曲线：各向量侧均还原为文件（模型单来源字段所限，混合
            //来源条目落模后按主侧来源整体还原）；侧四元组还原各通道（subcase/
            //object 文本反解 id；横坐标形态侧 object/component 为空，
            //complete()=false 走横坐标回显）。单文件来源：各侧同文件
            auto channel = [](const QString& sub, const QString& schema,
                const QString& obj, const QString& comp)
            {
                DefineCurveEntry::FileChannel ch;
                ch.subcaseId = trailingNumber(sub);
                ch.schema = schema;
                ch.pointId = trailingNumber(obj);
                ch.objectText = obj;
                ch.component = comp;
                return ch;
            };
            e.source = 0;
            e.xSource = 0;
            e.ySource = 0;
            if (complexCurve)
            {
                e.ypSource = 0;
                e.ymSource = 0;
            }
            e.filePath = s.sourceFile;
            e.xFilePath = s.sourceFile;
            e.yFilePath = s.sourceFile;
            e.ypFilePath = s.sourceFile;
            e.ymFilePath = s.sourceFile;
            e.xChannel = channel(s.xSideSubcase, s.xSideSchema,
                s.xSideObject, s.xSideComponent);
            e.yChannel = channel(s.subcase, s.schema, s.object, s.component);
            if (complexCurve)
            {
                e.ypChannel = channel(s.complexSideSubcase, s.complexSideSchema,
                    s.complexSideObject, s.complexSideComponent);
                e.ymChannel = channel(s.ySideSubcase, s.ySideSchema,
                    s.ySideObject, s.ySideComponent);
            }
            if (isAsciiDataset(s.sourceFile))
            {
                //R22 ascii 来源曲线：工况级不落模型文本（五元组工况为空串），
                //trailingNumber 反解得 0 会致 complete() 恒假/回显失败——ascii
                //数据集为固定单块骨架，工况/对象内部标识直取常量补齐（结果
                //类型/对象/分量取模型文本原样）
                const auto fixAsciiIds = [](DefineCurveEntry::FileChannel& ch)
                {
                    if (ch.subcaseId <= 0)
                        ch.subcaseId = mCurveDataSourceStore::asciiSubcaseId();
                    if (ch.pointId <= 0)
                    {
                        ch.pointId = mCurveDataSourceStore::asciiPointId();
                        ch.objectText = mCurveDataSourceStore::asciiObjectDisplayText();
                    }
                };
                fixAsciiIds(e.xChannel);
                fixAsciiIds(e.yChannel);
                if (complexCurve)
                {
                    fixAsciiIds(e.ypChannel);
                    fixAsciiIds(e.ymChannel);
                }
            }
        }

        _entries.push_back(e);
        _curveList->blockSignals(true);
        _curveList->addItem(entryDisplayName(_entries.size() - 1));
        _curveList->blockSignals(false);
        _current = _entries.size() - 1;
        _syncing = true;                 //选中行信号不写回（面板尚未载入）
        _curveList->setCurrentRow(entryToListRow(_current));
        _syncing = false;
        refreshFileVectorTexts();        //文件来源：按还原通道重建向量回显
        loadEntryToPanel();
        refreshActionStates();
    }

    void mDefineCurveDlg::dumpCurveValuesToEntry(int curveId, DefineCurveEntry& e) const
    {
        //编辑模式切"数值"（设计图）：曲线当前数据倒进数值表条目——柱状=共享表
        //Categories（刻度名）+数值列；复数=三路（X/Ym=上面板幅值，Yp=下面板
        //相位）；XY=X/Y 两列（Pt=行号不入串）
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        CurveNode* n = doc->node(curveId);
        if (!n || n->type != CurveNodeType::Curve)
            return;
        const CurveSeriesData& s = n->series;
        if (targetIsBarChart())
        {
            //柱状图共享表：Categories=横坐标刻度名（未记刻度留空），数值列=Y；
            //X 列为行号语义不入条目
            e.pairXText.clear();
            e.pairYpText.clear();
            e.pairYmText.clear();
            e.categoriesText = s.xTickLabels.join(QStringLiteral(", "));
            QStringList ys;
            for (int i = 0; i < s.yData.size(); ++i)
                ys << QString::number(s.yData.at(i), 'f', 6);
            e.valuesText = ys.join(QStringLiteral(", "));
            return;
        }
        if (s.isComplex())
        {
            fillPairTexts(s.xData, s.yData, s.complexYData, e);
            return;
        }
        e.pairYpText.clear();
        e.pairYmText.clear();
        QStringList xs, ys;
        for (int i = 0; i < qMin(s.xData.size(), s.yData.size()); ++i)
        {
            xs << QString::number(s.xData.at(i), 'f', 6);
            ys << QString::number(s.yData.at(i), 'f', 6);
        }
        e.pairXText = xs.join(QStringLiteral(", "));
        e.valuesText = ys.join(QStringLiteral(", "));
    }

    void mDefineCurveDlg::removeEntry(int row)
    {
        if (row < 0 || row >= _entries.size())
            return;
        //柱状图共享表先撤该条目列（行与其余条目列数据保持，列位=条目序 +2）
        if (targetIsBarChart() && _valueTable && row + 2 < _valueTable->columnCount())
            _valueTable->removeColumn(row + 2);
        _entries.removeAt(row);
        delete _curveList->takeItem(entryToListRow(row));
        if (_entries.isEmpty())
        {
            _current = -1;
            clearPanelState();
        }
        else
        {
            const int r = qMin(row, _entries.size() - 1);
            _current = r;
            _syncing = true;
            _curveList->setCurrentRow(entryToListRow(r));
            _syncing = false;
            loadEntryToPanel();
        }
        refreshActionStates();
    }

    void mDefineCurveDlg::onListSelectionChanged()
    {
        if (_syncing)
            return;
        syncPanelToEntry();   //旧条目编辑落盘
        _current = listRowToEntryIndex(_curveList->currentRow());
        if (currentEntryIndex() >= 0)
            loadEntryToPanel();
        else if (categoriesRowActive())
        {
            //柱状图 Categories 行（设计图）：右栏切共享数值表，刻度名列即编辑
            //目标；共享表内容与条目定义保持，不整版清空
            _syncing = true;
            _valueRadio->setChecked(true);
            _sourceStack->setCurrentIndex(2);
            _syncing = false;
            refreshVectorEditability();
        }
        else
            clearPanelState();
        refreshActionStates();
    }

    void mDefineCurveDlg::onCutClicked()
    {
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        syncPanelToEntry();
        _clipEntry = _entries.at(cur);
        _hasClip = true;
        removeEntry(cur);
    }

    void mDefineCurveDlg::onCopyClicked()
    {
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        syncPanelToEntry();
        _clipEntry = _entries.at(cur);
        _hasClip = true;
        refreshActionStates();
    }

    void mDefineCurveDlg::onPasteClicked()
    {
        if (!_hasClip)
            return;
        syncPanelToEntry();
        DefineCurveEntry e = _clipEntry;
        e.name = nextCurveName();
        ++_counter;
        e.useName = true;
        e.editCurveId = -1;              //粘贴条目是新建副本，解除原曲线编辑绑定
        const int at = currentEntryIndex() + 1;   //当前条目之后插入，无选中则追加
        const int row = at <= 0 ? _entries.size() : qMin(at, _entries.size());
        _entries.insert(row, e);

        _curveList->blockSignals(true);
        _curveList->insertItem(entryToListRow(row), e.name);
        _curveList->blockSignals(false);
        if (targetIsBarChart())
        {
            //共享数值表在插入位同步插列并按条目回填（列位=条目序 +2）
            _syncing = true;
            _valueTable->insertColumn(2 + row);
            syncValueTableShape();
            fillBarColumnFromEntry(row);
            _syncing = false;
        }
        _current = row;
        _syncing = true;
        _curveList->setCurrentRow(entryToListRow(row));
        _syncing = false;
        loadEntryToPanel();
        refreshActionStates();
    }

    void mDefineCurveDlg::onNameCheckToggled(bool checked)
    {
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        _entries[cur].useName = checked;
        _nameEdit->setEnabled(checked);
        updateListItemText(cur);
        refreshValueColumnHeader();   //数值页值列表头随显示名联动
    }

    void mDefineCurveDlg::onNameEdited(const QString& text)
    {
        Q_UNUSED(text);
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        _entries[cur].name = _nameEdit->text();
        updateListItemText(cur);
        refreshValueColumnHeader();   //数值页值列表头随显示名联动
    }

    void mDefineCurveDlg::onVectorChanged(int id)
    {
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        if (targetIsBarChart())
        {
            e.activeVector = id;
            if (e.source == 0)
                refreshFileColumns();   //柱状图目标：整条来源语义（现状零变化）
            //向量框点亮/置灰随激活侧切换（照设计图）
            refreshVectorEditability();
            refreshInsertButtons();
            return;
        }
        //--- R21 交互模型：点向量=选中该侧并套用当前页来源，页面只随"来源"
        //--- 单选切换，点向量永不跳页；不变式：激活侧来源==当前页来源 ---
        const int pageSrc = _sourceGroup->checkedId();
        if (pageSrc < 0)
        {
            //防御（正常恒有选中页）：无来源页可套用时仅切激活侧焦点
            e.activeVector = id;
            refreshVectorEditability();
            refreshInsertButtons();
            return;
        }
        if (sideSource(e, id) == pageSrc)
        {
            //同源：仅焦点刷新——激活侧=该侧；文件页四级列编辑目标/下拉回显切
            //到该侧，数学页插入/拾取目标随激活侧，数值页编辑权仅激活列（整表
            //按各侧来源重新展开收口，各列数据保留）
            e.activeVector = id;
            if (pageSrc == 0)
            {
                refreshFileColumns();
                const int fi = _fileCombo->findData(sideFilePath(e, id));
                _fileCombo->setCurrentIndex(fi);
            }
            else if (pageSrc == 2)
            {
                _syncing = true;
                fillValueTableSides(e);
                _syncing = false;   //程序回填不触发 onValueCellChanged
            }
            refreshVectorEditability();
            refreshInsertButtons();
            return;
        }
        //异源：按当前页来源转换该侧（与点"来源"单选完全相同的链路：脏确认/
        //数学暂存/倒表预填/x 快照/落值落定/页面回显）。调用前不改 activeVector
        //——链路取消时回退来源+向量两个单选组（向量回退=原激活侧），成功时
        //链路内激活该侧；不触碰 _sourceStack 当前页（页面只随来源单选）
        applySourceChange(id, pageSrc);
    }

    //"离开数值"脏确认框（照参考交互同款文案）：切回时该侧数值表会按切出侧
    //内容重新装配/清空，用户改过的数据将会丢失。自动装配/清空不算改动（脏
    //标记只由用户编辑置位），未动过的侧直接通过。返回 false=用户取消
    bool mDefineCurveDlg::leaveValueConfirmed(bool dirty)
    {
        if (!dirty)
            return true;
        return QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("确认继续操作，刚才编辑的所有数据将会丢失。是否继续？"),
            QStringLiteral("取消"), QStringLiteral("继续"), QString(), 1, 0) == 1;
    }

    //来源切换共用链路（非柱状，R21）："来源"单选点击（side=当前激活侧）与
    //向量单选按当前页来源转换该侧（side=新选中侧）共用；页面只随"来源"单选
    //切换（向量转换路径不触碰当前页）。链路：脏确认→离开数学暂存→切入数值
    //倒表（B02）→主侧切离文件 x 快照（B03）→落值+镜像→落定（B01）→
    //激活侧=side→切页/回显。返回 false=用户取消（已回退来源+向量两个单选组，
    //状态零变化）
    bool mDefineCurveDlg::applySourceChange(int side, int id)
    {
        const int cur = currentEntryIndex();
        if (cur < 0)
            return false;
        DefineCurveEntry& e = _entries[cur];
        side = qBound(0, side, 2);
        const int old = sideSource(e, side);
        if (old == 2 && id != 2 && sideValueDirty(e, side) && !leaveValueConfirmed(true))
        {
            //"取消"：来源单选按当前栈页恢复（取消时刻页面从未切换，栈值即
            //真实当前页——来源点击路径栈恒数值页（old==2 门前提）→回数值
            //单选，与原行为逐字一致；向量转换路径栈=当前页来源→来源单选
            //原地不动、页面不跳，避免取消把不变式"激活侧来源==来源单选"
            //破坏成错页）；向量单选回退原激活侧（向量转换路径 activeVector
            //未变，重检=无操作）——屏蔽 syncing，不触发
            //onSourceChanged/onVectorChanged
            const int restorePage = _sourceStack->currentIndex();
            _syncing = true;
            if (restorePage == 1)
                _mathRadio->setChecked(true);
            else if (restorePage == 2)
                _valueRadio->setChecked(true);
            else
                _fileRadio->setChecked(true);
            (e.activeVector == 0 ? _xRadio
                : (e.activeVector == 2 ? _ymRadio : _yRadio))->setChecked(true);
            _syncing = false;
            return false;
        }
        if (old == 1 && id != 1)
        {
            //离开数学运算：暂存该侧数学回显（XY 的 Y 侧与表达式框同源
            //免暂存——expression 常驻；x/yp/ym 侧回显会被文件/数值形态
            //覆盖，须暂存）
            if (side == 0)
            {
                e.mathXRef = e.xText;
                e.hasXMathStash = true;
            }
            else if (targetIsComplexChart())
            {
                if (side == 1)
                {
                    e.mathYpText = e.ypText;
                    e.hasYpMathStash = true;
                }
                else
                {
                    e.mathYmText = e.ymText;
                    e.hasYmMathStash = true;
                }
            }
        }
        //切入数值：先把激活侧切出前来源内容装配进该侧数值串（文件=该侧
        //通道数据/兜底横坐标，数学=该侧表达式/引用求值）——倒表须在
        //setSideSource 落新值之前：此时 sideSource 仍读旧来源，函数内
        //"old==2"卫语句与 assembleSideData 分派才指向切出侧（R20-B02：
        //修前调用点在落值之后，卫语句恒真恒早退，倒表自 R20 拆分起从未
        //生效）。该侧旧来源无内容时保留已存数值串（数值可能是该侧唯一
        //内容，切走再切回不丢）；"取消"回退在上方已 return，不会倒表
        if (id == 2 && old != 2)
        {
            dumpSideToValueText(e, side);
            setSideValueDirty(e, side, false);   //表内容为程序装配，非用户改动
        }
        //主侧切离"文件"：把 x 侧当时可解析的横坐标数据快照进 x 侧数值串
        //（R20-B03）——x 侧未显式选通道时其数据=主侧数据块横坐标，主侧
        //切离文件后该回退断源（X 列回显空白+混合应用装配失败）。此刻
        //（落新值前）主侧仍是文件来源，assembleSideData(e,0) 走主侧文件
        //横坐标回退分支可解析；仅 x 侧数值串为空时写（不覆盖用户自输内
        //容），装配失败/异常静默跳过（装配端另有数值串兜底回退）；程序
        //快照非用户改动，不置脏
        if (old == 0 && id != 0
            && side == (targetIsComplexChart() ? 2 : 1)
            && sideSource(e, 0) == 0 && !e.xChannel.complete()
            && sideValueText(e, 0).trimmed().isEmpty())
        {
            try
            {
                QVector<double> xs;
                QString snapErr;
                if (assembleSideData(e, 0, xs, snapErr) && !xs.isEmpty())
                {
                    QStringList xToks;
                    for (int i = 0; i < xs.size(); ++i)
                        xToks << QString::number(xs.at(i), 'f', 6);
                    sideValueText(e, 0) = xToks.join(QStringLiteral(", "));
                }
            }
            catch (...)
            {
                //装配异常按"该时刻给不出横坐标"处理：不写快照
            }
        }
        setSideSource(e, side, id);
        //首次显式选择：所有仍为未显式设置（哨兵 -1）的侧一并落定为本次值
        //——此后各侧只随各自的显式切换分叉（R20-B01：未设置过的侧不按出厂
        //默认把用户带离当前来源页）。落定须在下方切页分支（数值表按各侧
        //来源展开/回显）之前，保证表形态与落定后的全侧口径一致；整条
        //source 镜像同步为本次选择。"取消"回退在上方已 return，不会走到
        //这里，不落定任何侧
        const auto rawSideSource = [this, &e](int vector)
        {
            switch (vector)
            {
            case 0:  return e.xSource;
            case 1:  return targetIsComplexChart() ? e.ypSource : e.ySource;
            default: return e.ymSource;
            }
        };
        for (int s = 0; s < 3; ++s)
            if (rawSideSource(s) < 0)
                setSideSource(e, s, id);
        e.source = id;   //镜像同步（最近一次显式选择）
        e.activeVector = side;   //向量转换路径在此激活新侧（来源点击路径同值）
        _sourceStack->setCurrentIndex(id);
        if (id == 1 && old != 1)
        {
            //切入数学运算：恢复/清空仅作用于激活侧（XY 的 Y 框与表达式框
            //同源，直接回显表达式；有暂存恢复、无暂存（文件/数值新建切入）
            //清空——文件/数值的回显文本不是有效引用）
            if (side == 0)
                e.xText = e.hasXMathStash ? e.mathXRef : QString();
            else if (targetIsComplexChart())
            {
                if (side == 1)
                    e.ypText = e.hasYpMathStash ? e.mathYpText : QString();
                else
                    e.ymText = e.hasYmMathStash ? e.mathYmText : QString();
            }
            else
                e.yText = e.expression;
        }
        else if (id == 0 && old != 0)
        {
            //切入文件：回显该侧向量文本（按侧）+ 四级列按该侧通道回填
            refreshFileVectorTexts();
            refreshFileColumns();
        }
        else if (id == 2 && old != 2)
        {
            //切入数值：数值页整表按各侧来源重新展开——激活侧已倒表预填
            //（上方 setSideSource 落值前装配切出侧内容），其余列按其当前
            //来源只读回显
            _syncing = true;
            fillValueTableSides(e);
            _syncing = false;   //程序回填不触发 onValueCellChanged
            refreshValueVectorTexts();
        }
        //文件页：文件下拉回显激活侧文件（向量转换路径激活侧已切换，须跟随；
        //来源点击路径同侧回显=无操作）
        if (id == 0)
        {
            const int fi = _fileCombo->findData(sideFilePath(e, side));
            _fileCombo->setCurrentIndex(fi);
        }
        refreshVectorEditability();
        refreshInsertButtons();
        refreshActionStates();
        refreshPreview();
        return true;
    }

    void mDefineCurveDlg::onSourceChanged(int id)
    {
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        if (!targetIsBarChart())
        {
            //非柱状：来源单选只作用于当前激活侧（R21 共用链路），页面切到
            //该来源页
            applySourceChange(qBound(0, _entries.at(cur).activeVector, 2), id);
            return;
        }
        //--- 柱状图目标：整条 source 语义（现状零变化）---
        DefineCurveEntry& e = _entries[cur];
        const int old = e.source;
        if (old == 2 && id != 2 && e.yValueDirty && !leaveValueConfirmed(true))
        {
            //"取消"回退单选不切换（屏蔽 syncing，不触发本函数）
            _syncing = true;
            if (id == 0)
                _fileRadio->setChecked(false);
            else if (id == 1)
                _mathRadio->setChecked(false);
            _valueRadio->setChecked(true);
            _syncing = false;
            return;
        }
        if (old == 1 && id != 1)
        {
            //离开数学运算：暂存数学回显（表达式框内容 expression 不在切换中
            //清理，免暂存）——应用后条目留存，来源来回切换不丢数学内容。
            //整条语义：三侧暂存标志一并置位（等价拆分前的 hasMathStash）
            e.mathXRef = e.xText;
            e.mathYpText = e.ypText;
            e.mathYmText = e.ymText;
            e.hasXMathStash = true;
            e.hasYpMathStash = true;
            e.hasYmMathStash = true;
        }
        e.source = id;
        _sourceStack->setCurrentIndex(id);
        if (id == 1 && old != 1)
        {
            //切入数学运算：有暂存先恢复数学回显（XY 的 Y 框与表达式框同源，
            //直接回显表达式）；无暂存（文件/数值新建切入）才清空——文件/数值
            //的回显文本不是有效引用
            if (e.hasXMathStash)
            {
                e.xText = e.mathXRef;
                e.ypText = e.mathYpText;
                e.ymText = e.mathYmText;
                if (!targetIsComplexChart())
                    e.yText = e.expression;
            }
            else
            {
                e.xText.clear();
                e.yText.clear();
                if (targetIsComplexChart())
                {
                    e.ypText.clear();
                    e.ymText.clear();
                }
            }
            _xEdit->setText(e.xText);
            _yEdit->setText(targetIsComplexChart() ? e.ypText : e.yText);
            if (targetIsComplexChart())
                _ymEdit->setText(e.ymText);
        }
        else if (id == 0 && old != 0)
            refreshFileVectorTexts();
        else if (id == 2 && old != 2)
        {
            //切"数值"时以切出侧当前内容装配数值表（XY/复数图每条目独立表，
            //柱状图共享表只落本条目 valuesText 列，编辑模式条目同规则）：
            //可装配则整表刷新，给不出数据则清空——数值表始终对齐切出侧当前
            //内容（柱状经整表重建，其余条目列以各自记录回填无损）。例外：文件
            //与数学都没有内容时保留原表（数值来源创建的曲线条目，数值是它
            //唯一的内容，切出去看了眼再切回不丢数据）
            bool dumped = false;
            bool cleared = false;
            //双侧内容判定（与各自装配条件同口径）：文件=通道选择完整可用；
            //数学=表达式文本非空
            const bool fileHasContent = targetIsComplexChart()
                ? (e.ypChannel.complete() && e.ymChannel.complete())
                : e.fileComplete();
            const bool mathHasContent = targetIsComplexChart()
                ? (!e.ymText.trimmed().isEmpty() && !e.ypText.trimmed().isEmpty())
                : !e.expression.trimmed().isEmpty();
            {
                //文件/数学切"数值"，把切出侧当前内容装配进表供微调
                try
                {
                    if (old == 0 && targetIsComplexChart()
                        && e.ypChannel.complete() && e.ymChannel.complete())
                    {
                        //文件复数图：x/yp/ym 三路装配（X 未选通道时取数据块
                        //记录横坐标）
                        QVector<double> xP, yM, yP;
                        QString dumpErr;
                        if (buildFilePairData(e, xP, yM, yP, dumpErr))
                        {
                            fillPairTexts(xP, yM, yP, e);
                            dumped = true;
                        }
                    }
                    else if (old == 0 && !targetIsComplexChart()
                        && e.fileComplete())
                    {
                        //文件 XY/柱状：Y 侧通道数据进 Y 列（柱状无横坐标语义，
                        //X 列串不落），X 侧通道/记录横坐标进 X 列
                        QVector<double> xs, ys;
                        QString dumpErr;
                        if (buildFileData(e, xs, ys, dumpErr) && !ys.isEmpty())
                        {
                            QStringList yToks;
                            for (int i = 0; i < ys.size(); ++i)
                                yToks << QString::number(ys.value(i), 'f', 6);
                            e.valuesText = yToks.join(QStringLiteral(", "));
                            if (!targetIsBarChart())
                            {
                                QStringList xToks;
                                for (int i = 0; i < ys.size(); ++i)
                                    xToks << QString::number(xs.value(i), 'f', 6);
                                e.pairXText = xToks.join(QStringLiteral(", "));
                            }
                            dumped = true;
                        }
                    }
                    else if (old == 1 && targetIsComplexChart()
                        && !e.ymText.trimmed().isEmpty()
                        && !e.ypText.trimmed().isEmpty())
                    {
                        //数学复数图：幅值/相位两表达式成对求值（ym=幅值、yp=相位）
                        QVector<double> xP, yM, yP;
                        QString dumpErr;
                        if (buildMathPairData(e, xP, yM, yP, dumpErr))
                        {
                            fillPairTexts(xP, yM, yP, e);
                            dumped = true;
                        }
                    }
                    else if (old == 1 && !targetIsComplexChart()
                        && !e.expression.trimmed().isEmpty())
                    {
                        //数学 XY/柱状：表达式全量求值进 Y 列（柱状无横坐标
                        //语义，X 列串不落）
                        QVector<double> xs, ys;
                        QString dumpErr;
                        if (buildMathData(e, xs, ys, dumpErr) && !ys.isEmpty())
                        {
                            QStringList yToks;
                            for (int i = 0; i < ys.size(); ++i)
                                yToks << QString::number(ys.value(i), 'f', 6);
                            e.valuesText = yToks.join(QStringLiteral(", "));
                            if (!targetIsBarChart())
                            {
                                QStringList xToks;
                                for (int i = 0; i < ys.size(); ++i)
                                    xToks << QString::number(xs.value(i), 'f', 6);
                                e.pairXText = xToks.join(QStringLiteral(", "));
                            }
                            dumped = true;
                        }
                    }
                }
                catch (...)
                {
                    //装配/求值异常按"切出侧给不出数据"处理：走下方保留/清空
                }
                if (!dumped)
                {
                    if (fileHasContent || mathHasContent)
                    {
                        //至少一侧有内容但装配不出（表达式写错、通道选了一半等）：
                        //清数值串并整表重建为空（残留上次倒表内容会误导读条目
                        //"数值"来源的数据）
                        e.valuesText.clear();
                        e.pairXText.clear();
                        e.pairYpText.clear();
                        e.pairYmText.clear();
                        cleared = true;
                        e.yValueDirty = false;   //表内容为程序清空，非用户改动
                    }
                    //else：文件与数学都无内容——保留原表与原脏标记（数值是该
                    //条目唯一的内容，不装配不清空不重置编辑态）
                }
                else
                {
                    e.yValueDirty = false;   //表内容为程序装配，非用户改动
                }
            }
            if (dumped || cleared)
                rebuildValueTableView();   //切来源路径不经 loadEntryToPanel，补整表回填
            refreshValueVectorTexts();
        }
        refreshVectorEditability();
        refreshActionStates();
        refreshPreview();
    }

    // =============================================================================
    // 文件来源页：四级联动
    // =============================================================================

    //当前向量单选对应的文件通道（四级列的编辑目标：X 侧 / Y 侧）
    DefineCurveEntry::FileChannel& mDefineCurveDlg::activeFileChannel(DefineCurveEntry& e)
    {
        //复数图目标三槽：0=x 1=yp 2=ym；常规目标两槽：0=x 1=y
        switch (_vectorGroup->checkedId())
        {
        case 0:  return e.xChannel;
        case 1:  return targetIsComplexChart() ? e.ypChannel : e.yChannel;
        default: return e.ymChannel;
        }
    }

    const DefineCurveEntry::FileChannel& mDefineCurveDlg::activeFileChannel(
        const DefineCurveEntry& e) const
    {
        //与可写重载同逻辑（refreshFileColumns 等只读路径经此取通道）
        switch (_vectorGroup->checkedId())
        {
        case 0:  return e.xChannel;
        case 1:  return targetIsComplexChart() ? e.ypChannel : e.yChannel;
        default: return e.ymChannel;
        }
    }

    //四级列按当前向量侧的通道选择回填（X/Y 单选切换编辑目标，两轴相互独立：
    //工况等各级选择各自保存、互不相通）。存恢复 syncing，兼容 loadEntryToPanel
    //内嵌调用（其外层 syncing 状态不被破坏）
    void mDefineCurveDlg::refreshFileColumns()
    {
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];   //可写：ascii 数据集通道默认补全（下）
        //工况列容器随当前侧文件类型显隐（ascii 列数据集无工况级，四级退化三级）
        refreshSubcaseColumnVisible();
        //ascii 列数据集（R22）：通道默认补全——[Unknown]/[Block 1] 恒选、X 侧
        //分量默认第 1 列（用户拍板，替代 pch 的"数据块横坐标"默认；仅补空缺，
        //编辑载入/用户已选不覆盖），后续 fill/select 序列按补全后的通道回显
        completeAsciiChannel(activeFileChannel(e), qBound(0, _vectorGroup->checkedId(), 2));
        const DefineCurveEntry::FileChannel& ch = activeFileChannel(_entries.at(cur));
        const bool wasSyncing = _syncing;
        _syncing = true;
        fillSubcases();
        selectListRowByData(_subcaseList, ch.subcaseId);
        fillSchemas();
        selectListRowByText(_schemaList, ch.schema);
        fillObjects();
        selectListRowByData(_objectList, ch.pointId);
        fillComponents();
        selectListRowByText(_componentList, ch.component);
        refreshXColumnStates();
        _syncing = wasSyncing;
    }

    //ascii 列数据集（R22）通道默认补全：结果类型/对象恒 [Unknown]/[Block 1]
    //（单块骨架语义），X 侧分量默认第 1 列（用户拍板，替代 pch 的"数据块横坐
    //标"默认）。仅补空缺值（编辑载入/用户已选不覆盖）；当前文件下拉非 ascii
    //或列名不可得时不动作（pch 路径零变化）
    void mDefineCurveDlg::completeAsciiChannel(DefineCurveEntry::FileChannel& ch, int vector)
    {
        const QString path = _fileCombo->currentData().toString();
        if (!isAsciiDataset(path))
            return;
        if (ch.subcaseId <= 0)
            ch.subcaseId = mCurveDataSourceStore::asciiSubcaseId();
        if (ch.schema.isEmpty())
            ch.schema = mCurveDataSourceStore::asciiSchemaDisplayText();
        if (ch.pointId <= 0)
        {
            ch.pointId = mCurveDataSourceStore::asciiPointId();
            ch.objectText = mCurveDataSourceStore::asciiObjectDisplayText();
        }
        if (vector == 0 && ch.component.isEmpty())
            ch.component = mCurveDataSourceStore::getInstance()->asciiColumnNames(path).value(0);
    }

    //工况列容器显隐（R22）：当前数据文件为 ascii 列数据集（csv/txt）时隐藏
    //（无工况级）；pch/未选文件恒显示（现状零变化）
    void mDefineCurveDlg::refreshSubcaseColumnVisible()
    {
        if (_subcaseColumn)
            _subcaseColumn->setVisible(!isAsciiDataset(_fileCombo->currentData().toString()));
    }

    //X 侧对象/分量级联所用工况：优先 X 侧自身工况；该结果类型在其下无对象时
    //回退 Y 侧工况（X 侧结果类型列表并入了 Y 侧工况的结果类型——文件路径按
    //侧独立后仅同文件回退，跨文件无意义）
    int mDefineCurveDlg::xCascadeSubcase(const QString& schema) const
    {
        const QString path = _fileCombo->currentData().toString();
        const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(path);
        if (!idx)
            return 0;
        const int xSub = _subcaseList->currentRow() >= 0
            ? _subcaseList->currentItem()->data(Qt::UserRole).toInt() : 0;
        if (!idx->object.value(xSub).value(schema).empty())
            return xSub;
        const int cur = currentEntryIndex();
        if (cur < 0 || (!targetIsBarChart()
            && sideFilePath(_entries.at(cur), 0) != sideFilePath(_entries.at(cur), 1)))
            return xSub;   //无条目/跨文件：不回退 Y 侧工况
        const int ySub = cur >= 0 ? _entries.at(cur).yChannel.subcaseId : 0;
        if (!idx->object.value(ySub).value(schema).empty())
            return ySub;
        return xSub;
    }

    //X 侧选中横坐标类型时无对象/分量（照"从文件创建曲线"，X 按记录横坐标
    //赋值）：两列置灰（列表内容联动后本就为空）
    void mDefineCurveDlg::refreshXColumnStates()
    {
        const bool abscissa = _vectorGroup->checkedId() == 0
            && mCurveDataSourceStore::isAbscissaSchema(_schemaList->currentItem()
                ? _schemaList->currentItem()->text() : QString());
        _objectList->setEnabled(!abscissa);
        _componentList->setEnabled(!abscissa);
    }

    void mDefineCurveDlg::fillFileCombo()
    {
        _fileCombo->blockSignals(true);
        _fileCombo->clear();
        for (const QString& path : mCurveDataSourceStore::getInstance()->filePaths())
        {
            _fileCombo->addItem(path, path);   //照"从文件创建曲线"：直接展示全路径
            _fileCombo->setItemData(_fileCombo->count() - 1, path, Qt::ToolTipRole);
        }
        _fileCombo->blockSignals(false);
    }

    void mDefineCurveDlg::fillSubcases()
    {
        _subcaseList->blockSignals(true);
        _subcaseList->clear();
        const QString path = _fileCombo->currentData().toString();
        const PchReader::PchFile* f = mCurveDataSourceStore::getInstance()->file(path);
        if (f && mCurveDataSourceStore::getInstance()->index(path))
            for (int subcaseId : mCurveDataSourceStore::getInstance()->index(path)->operatingcondition)   //std::set 升序
            {
                //工况文本由共享仓库生成（与"从文件创建曲线"同源，内容一致）
                auto* item = new QListWidgetItem(
                    mCurveDataSourceStore::subcaseDisplayText(*f, subcaseId), _subcaseList);
                item->setData(Qt::UserRole, subcaseId);
            }
        _subcaseList->blockSignals(false);
        _subcaseCount->setText(_subcaseList->count() > 0
            ? QStringLiteral("1/%1").arg(_subcaseList->count()) : QString());
    }

    void mDefineCurveDlg::fillSchemas()
    {
        _schemaList->blockSignals(true);
        _schemaList->clear();
        const QString path = _fileCombo->currentData().toString();
        const int subcaseId = _subcaseList->currentRow() >= 0
            ? _subcaseList->currentItem()->data(Qt::UserRole).toInt() : 0;
        if (const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(path))
        {
            if (isAsciiDataset(path))
            {
                //R22 ascii 列数据集：结果类型唯一项 Unknown（无横坐标类型/矢量
                //过滤语义——isVectorSchema("Unknown")==false，须经此分流绕过，
                //X 侧亦同，两侧三级化一致）
                const QString schema = mCurveDataSourceStore::asciiSchemaDisplayText();
                auto* item = new QListWidgetItem(schema, _schemaList);
                item->setData(Qt::UserRole, schema);
            }
            else if (_vectorGroup->checkedId() == 0)
            {
                //X 侧（照"从文件创建曲线"X 轴数据源：横坐标类型置首），结果
                //类型并入 X 侧与 Y 侧各自工况下的结果类型（两轴工况独立，Y 侧
                //的按需求追加），去重保序；横坐标类型无对象/分量（X=记录横坐标）。
                //文件路径按侧独立后，Y 侧工况的结果类型仅在同文件时并入
                //(跨文件无意义)
                const int cur = currentEntryIndex();
                const int ySub = cur >= 0 ? _entries.at(cur).yChannel.subcaseId : 0;
                const bool sameFile = cur < 0 || targetIsBarChart()
                    || sideFilePath(_entries.at(cur), 0) == sideFilePath(_entries.at(cur), 1);
                QStringList schemas;
                QSet<QString> seen;
                const auto appendUnique = [&schemas, &seen](const QString& s)
                    {
                        if (!seen.contains(s))
                        {
                            seen.insert(s);
                            schemas << s;
                        }
                    };
                for (const QString& t : idx->xresulttype)
                    appendUnique(t);
                for (const QString& r : idx->yresulttype.value(subcaseId))
                    appendUnique(r);
                if (sameFile)
                    for (const QString& r : idx->yresulttype.value(ySub))
                        appendUnique(r);
                for (const QString& schema : schemas)
                {
                    auto* item = new QListWidgetItem(schema, _schemaList);
                    item->setData(Qt::UserRole, schema);
                }
            }
            else
            {
                //Y 侧：仅节点矢量结果有分量展开（与"从文件创建曲线"列表联动
                //一致），排序求稳
                QStringList schemas = idx->object.value(subcaseId).keys();
                schemas.sort();
                for (const QString& schema : schemas)
                {
                    if (!mCurveDataSourceStore::isVectorSchema(schema))
                        continue;
                    auto* item = new QListWidgetItem(schema, _schemaList);
                    item->setData(Qt::UserRole, schema);
                }
            }
        }
        _schemaList->blockSignals(false);
        _schemaCount->setText(_schemaList->count() > 0
            ? QStringLiteral("1/%1").arg(_schemaList->count()) : QString());
    }

    void mDefineCurveDlg::fillObjects()
    {
        _objectList->blockSignals(true);
        _objectList->clear();
        const QString path = _fileCombo->currentData().toString();
        const QString schema = _schemaList->currentRow() >= 0
            ? _schemaList->currentItem()->text() : QString();
        //X 侧对象级联工况：优先自身工况，该结果类型在其下无对象时回退 Y 侧
        const int subcaseId = _vectorGroup->checkedId() == 0
            ? xCascadeSubcase(schema)
            : (_subcaseList->currentRow() >= 0
                ? _subcaseList->currentItem()->data(Qt::UserRole).toInt() : 0);
        if (const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(path))
        {
            if (isAsciiDataset(path))
            {
                //R22 ascii 列数据集：对象唯一项 Block 1（内部标识恒 1，
                //asciiObjectDisplayText/asciiPointId 同源）
                auto* item = new QListWidgetItem(
                    mCurveDataSourceStore::asciiObjectDisplayText(), _objectList);
                item->setData(Qt::UserRole, mCurveDataSourceStore::asciiPointId());
            }
            else
                for (int pointId : idx->object.value(subcaseId).value(schema))
                {
                    auto* item = new QListWidgetItem(
                        QStringLiteral("Point id %1").arg(pointId), _objectList);
                    item->setData(Qt::UserRole, pointId);
                }
        }
        _objectList->blockSignals(false);
        _objectCount->setText(_objectList->count() > 0
            ? QStringLiteral("1/%1").arg(_objectList->count()) : QString());
    }

    void mDefineCurveDlg::fillComponents()
    {
        _componentList->blockSignals(true);
        _componentList->clear();
        const QString path = _fileCombo->currentData().toString();
        const QString schema = _schemaList->currentRow() >= 0
            ? _schemaList->currentItem()->text() : QString();
        const int pointId = _objectList->currentRow() >= 0
            ? _objectList->currentItem()->data(Qt::UserRole).toInt() : 0;
        //X 侧分量级联工况与对象列同源（优先自身工况，回退 Y 侧，见
        //xCascadeSubcase）；Y 侧用自身工况
        const int subcaseId = _vectorGroup->checkedId() == 0
            ? xCascadeSubcase(schema)
            : (_subcaseList->currentRow() >= 0
                ? _subcaseList->currentItem()->data(Qt::UserRole).toInt() : 0);
        if (const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(path))
        {
            if (isAsciiDataset(path))
            {
                //R22 ascii 列数据集：分量=全部列名（具名列原样+表头空位回退
                //"Column N"；不走 expandedComponents——"ASCII" 格式不在分量
                //展开目录，asciiColumnNames 为唯一下发口）
                for (const QString& name
                    : mCurveDataSourceStore::getInstance()->asciiColumnNames(path))
                {
                    auto* item = new QListWidgetItem(name, _componentList);
                    item->setData(Qt::UserRole, name);
                }
            }
            else
            {
                //分量数值格式（同一对象唯一；异常多格式按原列表联动语义跳过该对象）；
                //瞬态标志随索引叶子下发（块级判定），不再按文件级 xresulttype 反推
                const std::set<NumericFmtInfo> fmts =
                    idx->ycomponent.value(subcaseId).value(schema).value(pointId);
                if (fmts.size() == 1)
                    for (const QString& comp : mCurveDataSourceStore::expandedComponents(
                        fmts.begin()->fmt, fmts.begin()->transient))
                    {
                        auto* item = new QListWidgetItem(comp, _componentList);
                        item->setData(Qt::UserRole, comp);
                    }
            }
        }
        _componentList->blockSignals(false);
        _componentCount->setText(_componentList->count() > 0
            ? QStringLiteral("1/%1").arg(_componentList->count()) : QString());
    }

    void mDefineCurveDlg::onFileComboChanged(int index)
    {
        Q_UNUSED(index);
        //工况列容器随文件类型显隐（ascii 列数据集三级化，R22；程序回填路径也刷新）
        refreshSubcaseColumnVisible();
        fillSubcases();
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        const QString path = _fileCombo->currentData().toString();
        if (targetIsBarChart())
        {
            //柱状图目标：整条文件语义（换文件两侧通道选择全部失效，现状零变化）
            e.filePath = path;
            e.xChannel = DefineCurveEntry::FileChannel{};
            e.yChannel = DefineCurveEntry::FileChannel{};
            //ascii 列数据集（R22）：[Unknown]/[Block 1] 恒选补全（柱状无 X 语义，
            //只补主通道，分量留给用户选）
            completeAsciiChannel(e.yChannel, 1);
        }
        else
        {
            //文件路径按向量侧独立：写激活侧，仅该侧通道选择失效（他侧文件
            //与通道不受影响）
            setSideFilePath(e, e.activeVector, path);
            sideChannel(e, e.activeVector) = DefineCurveEntry::FileChannel{};
            //ascii 列数据集（R22）：通道默认补全（X 侧含默认第 1 列）
            completeAsciiChannel(sideChannel(e, e.activeVector), e.activeVector);
        }
        fillSchemas();
        fillObjects();
        fillComponents();
        //按补全后的通道同步列表选中态（程序回填：屏蔽选择信号——select 触发的
        //onXxxRowChanged 级联会清掉默认补全值；pch 通道已重置为空=清选，与
        //现状一致）
        _syncing = true;
        const DefineCurveEntry::FileChannel& chSel = targetIsBarChart()
            ? e.yChannel : sideChannel(e, e.activeVector);
        selectListRowByText(_schemaList, chSel.schema);
        selectListRowByData(_objectList, chSel.pointId);
        selectListRowByText(_componentList, chSel.component);
        _syncing = false;
        refreshFileVectorTexts();
        refreshActionStates();
    }

    void mDefineCurveDlg::onSubcaseRowChanged(int row)
    {
        Q_UNUSED(row);
        fillSchemas();
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        DefineCurveEntry::FileChannel& ch = activeFileChannel(e);
        ch.subcaseId = _subcaseList->currentRow() >= 0
            ? _subcaseList->currentItem()->data(Qt::UserRole).toInt() : 0;
        ch.schema.clear();
        ch.pointId = 0;
        ch.objectText.clear();
        ch.component.clear();
        fillObjects();
        fillComponents();
        refreshFileVectorTexts();
        refreshActionStates();
    }

    void mDefineCurveDlg::onSchemaRowChanged(int row)
    {
        Q_UNUSED(row);
        fillObjects();
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        DefineCurveEntry::FileChannel& ch = activeFileChannel(e);
        ch.schema = _schemaList->currentRow() >= 0
            ? _schemaList->currentItem()->text() : QString();
        ch.pointId = 0;
        ch.objectText.clear();
        ch.component.clear();
        fillComponents();
        refreshXColumnStates();
        refreshFileVectorTexts();
        refreshActionStates();
    }

    void mDefineCurveDlg::onObjectRowChanged(int row)
    {
        Q_UNUSED(row);
        fillComponents();
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        DefineCurveEntry::FileChannel& ch = activeFileChannel(e);
        ch.pointId = _objectList->currentRow() >= 0
            ? _objectList->currentItem()->data(Qt::UserRole).toInt() : 0;
        //对象显示名取列表项文本（pch="Point id n"、ascii="Block 1"，与列填充
        //同源一致）
        ch.objectText = ch.pointId > 0
            ? _objectList->currentItem()->text() : QString();
        ch.component.clear();
        //X 侧：记录对象实际级联所用工况（该结果类型在 X 侧工况下无对象时
        //回退为 Y 侧工况，见 xCascadeSubcase）
        if (_vectorGroup->checkedId() == 0)
            ch.subcaseId = xCascadeSubcase(ch.schema);
        refreshFileVectorTexts();
        refreshActionStates();
    }

    void mDefineCurveDlg::onComponentRowChanged(int row)
    {
        Q_UNUSED(row);
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        DefineCurveEntry::FileChannel& ch = activeFileChannel(e);
        ch.component = _componentList->currentRow() >= 0
            ? _componentList->currentItem()->text() : QString();
        refreshFileVectorTexts();
        refreshActionStates();
    }

    void mDefineCurveDlg::refreshFileVectorTexts()
    {
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        //仅文件来源侧重建回显：数学/数值侧的向量框是表达式/数据语义，
        //编辑模式还原的回显不可被文件通道重建清空（柱状图整条语义沿用
        //e.source 判定——sideSource 退化映射）
        if (targetIsBarChart() && e.source != 0)
            return;
        //照设计图：回显=文件:对象/…。Y=文件:对象/分量；X 侧选了通道时
        //=文件:对象/分量，选了横坐标类型时=该类型（X=记录横坐标），
        //都未选时=文件:Y侧对象/结果类型（按侧文件：兜底取 Y 侧自身文件）
        if (sideSource(e, 0) == 0)
        {
            const QString xp = sideFilePath(e, 0);
            if (e.xChannel.complete())
                e.xText = QStringLiteral("%1:%2/%3")
                .arg(xp, e.xChannel.objectText, e.xChannel.component);
            else if (!e.xChannel.schema.isEmpty() && mCurveDataSourceStore::isAbscissaSchema(e.xChannel.schema))
                e.xText = e.xChannel.schema;
            else if (sideFileComplete(e, 1))
                e.xText = QStringLiteral("%1:%2/%3")
                .arg(sideFilePath(e, 1), e.yChannel.objectText, e.yChannel.schema);
            else
                e.xText.clear();
        }
        if (!targetIsComplexChart())
        {
            if (sideSource(e, 1) == 0)
            {
                if (sideFileComplete(e, 1))
                    e.yText = QStringLiteral("%1:%2/%3")
                    .arg(sideFilePath(e, 1), e.yChannel.objectText, e.yChannel.component);
                else
                    e.yText.clear();
            }
        }
        else
        {
            //复数图目标三槽：yp/ym 各自回显（所选通道任选，成对与否由方向决定）
            if (sideSource(e, 1) == 0)
            {
                if (sideFileComplete(e, 1))
                    e.ypText = QStringLiteral("%1:%2/%3")
                    .arg(sideFilePath(e, 1), e.ypChannel.objectText, e.ypChannel.component);
                else
                    e.ypText.clear();
            }
            if (sideSource(e, 2) == 0)
            {
                if (sideFileComplete(e, 2))
                    e.ymText = QStringLiteral("%1:%2/%3")
                    .arg(sideFilePath(e, 2), e.ymChannel.objectText, e.ymChannel.component);
                else
                    e.ymText.clear();
            }
        }
        _xEdit->setText(e.xText);
        if (targetIsComplexChart())
        {
            _yEdit->setText(e.ypText);
            _ymEdit->setText(e.ymText);
        }
        else
            _yEdit->setText(e.yText);
    }

    // =============================================================================
    // 数学运算页：表达式构建器
    // =============================================================================

    void mDefineCurveDlg::rebuildObjectTree()
    {
        _objTree->blockSignals(true);
        populateCurveRefTree(_objTree);   //直列样式（工程->页->视口->曲线名）
        _objTree->blockSignals(false);
        refreshInsertButtons();
    }

    void mDefineCurveDlg::onDocStructureChanged(int nodeId)
    {
        Q_UNUSED(nodeId);
        //单例 close() 仅隐藏：隐藏期间不重建引用树（文档增删会频繁触发），
        //模式开启时 onDefineStarted 本就重填，无需补
        if (!isVisible())
            return;
        rebuildObjectTree();
    }

    void mDefineCurveDlg::rebuildFuncTree()
    {
        _funcTree->blockSignals(true);
        _funcTree->clear();
        //顶层=分类分组（库名"Templex 函数库"由列表标题条承载），叶子=函数；
        //未实现项（目录红色占位）不上树，分类下拉/树分组同步只保留有已实现
        //函数的分类
        const QStringList categories = mCurveExpressionEvaluator::functionCategories();
        for (const QString& cat : categories)
        {
            QTreeWidgetItem* catItem = nullptr;
            for (const mCurveExpressionEvaluator::MathFuncInfo& f
                : mCurveExpressionEvaluator::functionTable())
            {
                if (f.category != cat || !f.implemented)
                    continue;
                if (!catItem)
                {
                    catItem = new QTreeWidgetItem(_funcTree);
                    catItem->setText(0, cat);
                }
                QTreeWidgetItem* leaf = new QTreeWidgetItem(catItem);
                leaf->setText(0, f.name);
                leaf->setIcon(0, makeFuncFxIcon());
                leaf->setToolTip(0, QStringLiteral("%1\n%2")
                    .arg(f.signature, f.description));
                leaf->setData(0, Qt::UserRole, f.name);
            }
        }
        _funcTree->expandAll();
        _funcTree->blockSignals(false);
        applyFuncTreeFilter();
    }

    void mDefineCurveDlg::applyFuncTreeFilter()
    {
        const QString text = _funcSearch->text().trimmed();
        const QString cat = _funcCategory->currentData().toString();
        for (int i = 0; i < _funcTree->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* c = _funcTree->topLevelItem(i);
            if (cat != kAllCategories && c->text(0) != cat)
            {
                c->setHidden(true);
                continue;
            }
            bool any = false;
            for (int j = 0; j < c->childCount(); ++j)
            {
                QTreeWidgetItem* f = c->child(j);
                const bool hit = text.isEmpty()
                    || f->text(0).contains(text, Qt::CaseInsensitive)
                    || f->toolTip(0).contains(text, Qt::CaseInsensitive);
                f->setHidden(!hit);
                any = any || hit;
            }
            c->setHidden(!any);
        }
    }

    void mDefineCurveDlg::onFuncSearchChanged(const QString& text)
    {
        Q_UNUSED(text);
        applyFuncTreeFilter();
    }

    void mDefineCurveDlg::onFuncCategoryChanged(int index)
    {
        Q_UNUSED(index);
        applyFuncTreeFilter();
    }

    void mDefineCurveDlg::fillFuncParams(const QString& funcName)
    {
        _funcParamTable->setRowCount(0);
        _currentFuncName.clear();

        const mCurveExpressionEvaluator::MathFuncInfo* info = nullptr;
        for (const mCurveExpressionEvaluator::MathFuncInfo& f
            : mCurveExpressionEvaluator::functionTable())
            if (f.name == funcName)
            {
                info = &f;
                break;
            }
        if (!info)
        {
            refreshInsertButtons();
            return;
        }
        _currentFuncName = info->name;

        //参数默认值 = 首条曲线的 .y 向量引用（对齐本弹窗"双击默认取 .y 作函数
        //向量参数"约定；工程尚无曲线时留空，由用户经 fx 选择器填入）
        QString defaultVectorRef;
        {
            CurveDocument* doc = mCurveDocController::getInstance()->document();
            bool found = false;
            for (int pageId : doc->childNodeIds(doc->rootNodeId()))
            {
                for (int chartId : doc->childNodeIds(pageId))
                {
                    for (int curveId : doc->childNodeIds(chartId))
                    {
                        if (doc->nodeType(curveId) != CurveNodeType::Curve)
                            continue;
                        defaultVectorRef = curveRefPrefixById(curveId)
                            + QStringLiteral(".y");
                        found = true;
                        break;
                    }
                    if (found)
                        break;
                }
                if (found)
                    break;
            }
        }

        //照参考截图参数表分组：通用信息（name 行）+ 参数（各参数行），组标题
        //行跨两列
        const int rows = 3 + int(info->params.size());
        _funcParamTable->setRowCount(rows);

        auto readOnlyItem = [](const QString& text)
            {
                QTableWidgetItem* cell = new QTableWidgetItem(text);
                cell->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                return cell;
            };
        auto groupItem = [](const QString& text)
            {
                QTableWidgetItem* cell = new QTableWidgetItem(text);
                cell->setFlags(Qt::ItemIsEnabled);
                cell->setForeground(QBrush(QColor(0x66, 0x66, 0x66)));
                return cell;
            };
        _funcParamTable->setItem(0, 0, groupItem(QStringLiteral("通用信息")));
        _funcParamTable->setSpan(0, 0, 1, 2);
        _funcParamTable->setItem(1, 0, readOnlyItem(QStringLiteral("  name")));
        _funcParamTable->setItem(1, 1, readOnlyItem(info->name));
        _funcParamTable->setItem(2, 0, groupItem(QStringLiteral("参数")));
        _funcParamTable->setSpan(2, 0, 1, 2);

        //参数行：可编辑值 + fx（曲线选择器填入引用，参考截图步骤⑦⑧⑨）
        for (int i = 0; i < int(info->params.size()); ++i)
        {
            const int row = 3 + i;
            _funcParamTable->setItem(row, 0, readOnlyItem(QStringLiteral("  ") + info->params.at(i)));
            QWidget* cell = new QWidget(_funcParamTable);
            QHBoxLayout* cellLay = new QHBoxLayout(cell);
            cellLay->setContentsMargins(0, 0, 0, 0);
            cellLay->setSpacing(4);
            QLineEdit* edit = new QLineEdit(cell);
            edit->setText(defaultVectorRef);   //默认值=首条曲线 .y 向量引用（可编辑）
            QToolButton* fx = new QToolButton(cell);
            fx->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/选取.svg")));
            fx->setIconSize(QSize(16, 16));
            fx->setFixedSize(24, 24);
            fx->setToolTip(QStringLiteral("曲线选择器：双击曲线向量插入引用"));
            //照参考交互⑦⑧：只有左键点入输入框激活后才显示 fx 按钮（显示后
            //保持可见，直至参数表重建）。按钮不抢焦点，点击时输入框不失焦
            fx->setFocusPolicy(Qt::NoFocus);
            fx->setVisible(false);
            edit->installEventFilter(new FxRevealFilter(fx));
            connect(edit, &QLineEdit::textChanged, this, [this](const QString&)
                { refreshInsertButtons(); });   //参数变化 -> 插入串现取，刷新按钮态
            connect(fx, &QToolButton::clicked, this, [this, edit]()
                { openRefPicker(edit); });
            cellLay->addWidget(edit, 1);
            cellLay->addWidget(fx);
            _funcParamTable->setCellWidget(row, 1, cell);
        }
        refreshInsertButtons();
    }

    //按当前函数与参数值现合成调用串（如 log10(p1w1c1.y)）；未选函数返回空。
    //合成结果不落表（参数表只承载名称/值，照参考截图），插入时经
    //currentInsertText 现取
    QString mDefineCurveDlg::composeFuncCall() const
    {
        if (_currentFuncName.isEmpty())
            return QString();
        QStringList args;
        for (int r = 0; r < _funcParamTable->rowCount(); ++r)
            if (QWidget* w = _funcParamTable->cellWidget(r, 1))
                if (QLineEdit* edit = w->findChild<QLineEdit*>())
                    args << edit->text();
        return args.isEmpty()
            ? _currentFuncName
            : QStringLiteral("%1(%2)").arg(_currentFuncName, args.join(QStringLiteral(", ")));
    }

    void mDefineCurveDlg::onFuncTreeSelectionChanged()
    {
        QTreeWidgetItem* cur = _funcTree->currentItem();
        fillFuncParams(cur && !cur->data(0, Qt::UserRole).toString().isEmpty()
            ? cur->data(0, Qt::UserRole).toString() : QString());
    }

    void mDefineCurveDlg::onObjTreeSelectionChanged()
    {
        refreshInsertButtons();
    }

    QString mDefineCurveDlg::currentInsertText() const
    {
        if (_builderTabs->currentIndex() == 0)
        {
            //函数页：按当前函数与参数值现合成调用串（如 log10(p1w1c1.y)，
            //照参考截图步骤⑩填入所选输入框）
            return composeFuncCall();
        }
        //曲线页：选中曲线节点的引用前缀按当前激活向量补后缀（X 激活=.x 横坐标
        //向量，Y 激活=.y 纵坐标向量，照设计图"选择曲线->直接插入"）。页/视口
        //行无引用数据，按钮维持置灰
        QTreeWidgetItem* item = _objTree->currentItem();
        if (!item)
            return QString();
        const QString ref = item->data(0, Qt::UserRole).toString();
        if (ref.isEmpty())
            return QString();
        const int cur = currentEntryIndex();
        const bool xActive = cur >= 0 && _entries.at(cur).activeVector == 0;
        return ref + (xActive ? QStringLiteral(".x") : QStringLiteral(".y"));
    }

    void mDefineCurveDlg::refreshInsertButtons()
    {
        //X 向量仅支持曲线向量引用：函数页在 X 侧禁用插入（函数只能插入 Y 侧
        //表达式），避免产生"log10(...) 作 X 引用"的无效向量（预览红字报错项）；
        //未实现函数（红色标注项）同样禁用插入——求值器尚未支持，插入即报错
        const bool funcTab = _builderTabs->currentIndex() == 0;
        const bool xSide = _vectorGroup->checkedId() == 0;
        const bool notImpl = funcTab && funcUnimplemented(_currentFuncName);
        const bool blocked = (funcTab && xSide) || notImpl;
        const bool enabled = !blocked && !currentInsertText().isEmpty();
        _directInsertBtn->setEnabled(enabled);
        _useExprBtn->setEnabled(enabled);
        QString tip;
        if (notImpl)
            tip = QStringLiteral("未实现函数（红色标注项）暂不可插入：求值器尚未支持该函数");
        else if (funcTab && xSide)
            tip = QStringLiteral("X 向量仅支持曲线向量引用：函数请切换到 Y 侧后插入");
        _directInsertBtn->setToolTip(tip);
        _useExprBtn->setToolTip(tip);
    }

    void mDefineCurveDlg::onInsertDirectClicked()
    {
        syncPanelToEntry();
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        const QString text = currentInsertText();
        if (text.isEmpty())
            return;
        DefineCurveEntry& e = _entries[cur];
        //插入按钮只在数学运算页（激活向量侧来源为数学）
        if (sideSource(e, e.activeVector) != 1)
            return;
        if (e.activeVector == 0)
        {
            //X 向量仅接受曲线向量引用：函数调用串不允许替换 X（按钮已禁用，
            //此处兜底拦截）
            if (_builderTabs->currentIndex() == 0)
                return;
            //X 向量：直接替换引用（参考截图"对应选择的向量X将插入对应的内容"）
            e.xText = text;
            _xEdit->setText(text);
        }
        else if (targetIsComplexChart())
        {
            //复数图：按激活向量侧直落对应表达式框（yp=Y 行相位侧 / ym=幅值侧），
            //不经"表达式"框——"用于表达式"才只写表达式框
            QLineEdit* edit = e.activeVector == 1 ? _yEdit : _ymEdit;
            edit->insert(text);   //程序插入只发 textChanged，手动同步条目
            if (e.activeVector == 1)
                e.ypText = edit->text();
            else
                e.ymText = edit->text();
        }
        else
        {
            insertIntoExpression(text);
        }
        refreshPreview();
        refreshActionStates();
    }

    void mDefineCurveDlg::onInsertToExprClicked()
    {
        syncPanelToEntry();
        const QString text = currentInsertText();
        if (text.isEmpty())
            return;
        insertIntoExpression(text);
    }

    void mDefineCurveDlg::insertIntoExpression(const QString& text)
    {
        const int cur = currentEntryIndex();
        if (cur < 0 || text.isEmpty()
            || sideSource(_entries.at(cur), qBound(0, _entries.at(cur).activeVector, 2)) != 1)
            return;   //仅数学运算页（激活向量侧来源为数学）
        _exprEdit->insert(text);   //程序插入只发 textChanged，手动走一次同步
        onExprEdited(_exprEdit->text());
    }

    void mDefineCurveDlg::onXEdited(const QString& text)
    {
        Q_UNUSED(text);
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        _entries[cur].xText = _xEdit->text();
        refreshPreview();
    }

    void mDefineCurveDlg::onExprEdited(const QString& text)
    {
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        //表达式框只属于数学运算页（页可见性=激活向量侧来源为数学）
        if (sideSource(e, qBound(0, e.activeVector, 2)) != 1)
            return;
        e.expression = text;
        if (targetIsComplexChart())
        {
            //复数图：表达式框独立（"用于表达式"只落此处，不镜像 yp/ym 槽；
            //向量槽经"直接插入"按激活侧直写），预览仍跟随表达式框
            refreshPreview();
            return;
        }
        //XY：表达式框即 Y 侧数学内容，同源互显——仅 Y 侧为数学来源时镜像
        //（Y 侧文件/数值时表达式框只是暂存区，不污染文件回显）
        if (sideSource(e, 1) == 1)
        {
            e.yText = text;
            _yEdit->setText(text);
        }
        refreshPreview();
        refreshActionStates();
    }

    //Y 向量框直接编辑（Y/yp 侧数学来源时可编辑，照设计图⑪"可在输入框中
    //修改对应的表达式"）：内容即该侧表达式；XY 与表达式简写同源互同步
    void mDefineCurveDlg::onYEdited(const QString& text)
    {
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        if (sideSource(e, 1) != 1)
            return;   //Y/yp 侧非数学来源：框为回显，不接受编辑写回
        if (targetIsComplexChart())
        {
            e.ypText = text;            //复数图：Y 行即相位侧表达式
            refreshActionStates();
            return;
        }
        e.expression = text;
        e.yText = text;
        _exprEdit->setText(text);   //setText 不回发 textEdited，无回环
        refreshPreview();
        refreshActionStates();
    }

    void mDefineCurveDlg::onYmEdited(const QString& text)
    {
        //复数图幅值框（数学来源）：独立表达式槽，不回写"表达式"框
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        if (sideSource(e, 2) != 1 || !targetIsComplexChart())
            return;
        e.ymText = text;
        refreshActionStates();
    }

    void mDefineCurveDlg::onExprClearClicked()
    {
        _exprEdit->clear();
        onExprEdited(QString());
    }

    void mDefineCurveDlg::onSolveToggled(bool checked)
    {
        if (_syncing)
            return;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        _entries[cur].solve = checked;
        refreshPreview();
    }

    bool mDefineCurveDlg::targetIsBarChart() const
    {
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        CurveNode* n = doc ? doc->node(_targetChartId) : nullptr;
        return n && n->type == CurveNodeType::Chart && n->chart.chartType == 1;
    }

    bool mDefineCurveDlg::targetIsComplexChart() const
    {
        //复数图目标：向量区三槽 x/yp/ym，应用时按命名约定成对建曲线
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        CurveNode* n = doc ? doc->node(_targetChartId) : nullptr;
        return n && n->type == CurveNodeType::Chart && n->chart.chartType == 2;
    }

    void mDefineCurveDlg::refreshVectorEditability()
    {
        //按目标图表类型切形态（向量框按各向量侧来源启停与回显）：
        //  XY 曲线图：文件/数学 X/Y 两行——各侧按自身来源：数学侧=激活时点亮
        //            可编辑，文件侧=置灰回显，数值侧=置灰占位 values（数据在
        //            下方表格按行输入，设计图）
        //  柱状图  ：文件/数值整区隐藏；数学只留一行"表达式"（原 Y 行改文案）；
        //            整条来源语义零变化
        //  复数图  ：三行 x/yp/ym 常显——各侧规则同 XY
        const int cur = currentEntryIndex();
        if (targetIsBarChart())
        {
            //柱状图：整条来源语义（现状零变化）
            const int source = cur >= 0 ? _entries.at(cur).source : 0;
            const bool math = source == 1;
            if (_vectorSection)
                _vectorSection->setVisible(source == 1);
            if (_xRowWidget)
                _xRowWidget->setVisible(false);
            if (_yRowWidget)
                _yRowWidget->setVisible(true);
            if (_ymRowWidget)
                _ymRowWidget->setVisible(false);
            _yRadio->setText(QStringLiteral("表达式"));
            if (!_yRadio->isChecked())
                _yRadio->setChecked(true);   //仅表达式一侧（会触发 onVectorChanged(1)）
            setPickingVector(math ? 1 : -1);
            _xEdit->setEnabled(false);
            _yEdit->setEnabled(math);
            _xEdit->setPlaceholderText(QString());
            _yEdit->setPlaceholderText(QString());
            _xEdit->setReadOnly(true);
            _yEdit->setReadOnly(!math);
            return;
        }
        const bool complexChart = targetIsComplexChart();
        if (_vectorSection)
            _vectorSection->setVisible(true);   //复数图/XY：数值来源不隐藏向量区（置灰占位，设计图）
        if (_xRowWidget)
            _xRowWidget->setVisible(true);
        if (_yRowWidget)
            _yRowWidget->setVisible(true);
        if (_ymRowWidget)
            _ymRowWidget->setVisible(complexChart);
        _yRadio->setText(complexChart ? QStringLiteral("yp") : QStringLiteral("Y"));
        if (cur < 0)
        {
            //无条目：全部置灰清空（clearPanelState 已清文本，此处收口启停）
            setPickingVector(-1);
            _xEdit->setEnabled(false);
            _yEdit->setEnabled(false);
            _ymEdit->setEnabled(false);
            _xEdit->setReadOnly(true);
            _yEdit->setReadOnly(true);
            _ymEdit->setReadOnly(true);
            _xEdit->setPlaceholderText(QString());
            _yEdit->setPlaceholderText(QString());
            _ymEdit->setPlaceholderText(QString());
            return;
        }
        const DefineCurveEntry& e = _entries.at(cur);
        const int activeSide = qBound(0, e.activeVector, 2);
        //单行收口：数学侧=可编辑并回显数学内容（复数图三框常可编辑——现状
        //"激活侧语义仅用于拾取目标侧跟随"；XY 数学侧仅激活侧点亮，照设计图），
        //文件侧=置灰回显，数值侧=置灰占位 values（文本清空，数据在表格按行输入）
        auto applySideRow = [this, &e, activeSide, complexChart](int side, QLineEdit* edit)
        {
            const int src = sideSource(e, side);
            if (src == 1)
            {
                const bool editable = complexChart ? true : side == activeSide;
                edit->setEnabled(editable);
                edit->setReadOnly(!editable);
                edit->setPlaceholderText(QString());
                edit->setText(sideMathText(e, side));
            }
            else if (src == 0)
            {
                edit->setEnabled(false);
                edit->setReadOnly(true);
                edit->setPlaceholderText(QString());
                //文件回显（refreshFileVectorTexts 维护的条目字段）
                if (side == 0)
                    edit->setText(e.xText);
                else if (side == 1)
                    edit->setText(complexChart ? e.ypText : e.yText);
                else
                    edit->setText(e.ymText);
            }
            else
            {
                edit->setEnabled(false);
                edit->setReadOnly(true);
                edit->setPlaceholderText(QStringLiteral("values"));
                edit->setText(QString());
            }
        };
        applySideRow(0, _xEdit);
        applySideRow(1, _yEdit);
        if (complexChart)
            applySideRow(2, _ymEdit);
        //曲线拾取目标侧：激活侧来源为数学时默认激活（点击输入框仅切换目标侧）
        setPickingVector(sideSource(e, activeSide) == 1 ? activeSide : -1);
    }

    void mDefineCurveDlg::refreshPreview()
    {
        _previewTable->setRowCount(0);
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        const DefineCurveEntry& e = _entries.at(cur);
        //预览表只在数学运算页（页可见性=激活向量侧来源为数学；柱状图=整条来源）
        const int activeSrc = targetIsBarChart()
            ? e.source
            : sideSource(e, qBound(0, e.activeVector, 2));
        if (activeSrc != 1)
            return;

        QVector<double> xData;
        QVector<double> yData;
        QString errMsg;
        bool ok = false;
        if (!targetIsBarChart() && !targetIsComplexChart()
            && sideSource(e, 1) == 1 && sideSource(e, 0) != 1)
        {
            //混合来源 XY（Y 侧数学）：预览按混合装配口径（X 侧数值/文件数据
            //采样，8 点均匀抽取），与实际应用同源
            ok = buildMixedXYData(e, xData, yData, errMsg, 8);
        }
        else
            ok = buildMathData(e, xData, yData, errMsg, 8);
        if (!ok)
        {
            _previewTable->setRowCount(1);
            QTableWidgetItem* cell = new QTableWidgetItem(errMsg);
            cell->setFlags(Qt::ItemIsEnabled);
            cell->setForeground(QColor(200, 60, 60));
            _previewTable->setItem(0, 0, cell);
            return;
        }
        const int rows = qMin(8, yData.size());
        _previewTable->setRowCount(rows);
        for (int r = 0; r < rows; ++r)
        {
            QTableWidgetItem* cell = new QTableWidgetItem(
                QString::number(yData.at(r), 'g', 6));
            cell->setFlags(Qt::ItemIsEnabled);
            _previewTable->setItem(r, 0, cell);
        }
    }

    // =============================================================================
    // fx 曲线选择器
    // =============================================================================

    void mDefineCurveDlg::openRefPicker(QLineEdit* cellEdit)
    {
        RefPickerPopup* popup = new RefPickerPopup(this);
        popup->setAttribute(Qt::WA_DeleteOnClose);
        populatePickerTree(popup->tree());   //弹窗树照参考截图：p/w -> 曲线 -> x/y 叶子
        //选中条与曲线树对齐：文字区浅蓝圆角条（失焦转灰）；原生分支区选中底
        //（Highlight 深蓝）置白屏蔽
        popup->tree()->setItemDelegate(new CurveTreeItemDelegate(popup->tree()));
        {
            QPalette pal = popup->tree()->palette();
            pal.setColor(QPalette::Highlight, Qt::white);
            popup->tree()->setPalette(pal);
        }
        connect(popup->tree(), &QTreeWidget::itemDoubleClicked, this,
            [this, popup, cellEdit](QTreeWidgetItem* item, int)
            {
                const QString base = item->data(0, Qt::UserRole).toString();
                if (base.isEmpty())
                    return;
                //曲线级节点（树无 x/y 子级）：函数向量参数默认取曲线纵坐标 .y
                applyPickedRef(cellEdit, item->data(0, Qt::UserRole + 1).toInt() == 1
                    ? base + QStringLiteral(".y") : base);
                popup->close();
            });

        //落位：触发按钮正上方（向上弹出，不遮挡按钮下方的页面曲线）；上方放
        //不下时回退按钮正下方，均钳制在屏幕内。无按钮上下文则跟随鼠标
        QPoint pos = QCursor::pos();
        if (QToolButton* btn = qobject_cast<QToolButton*>(sender()))
        {
            const QPoint below = btn->mapToGlobal(QPoint(0, btn->height() + 2));
            const QPoint above = btn->mapToGlobal(QPoint(0, -popup->height() - 2));
            const QRect availBelow = QApplication::screenAt(below)
                ? QApplication::screenAt(below)->availableGeometry() : QRect();
            pos = (availBelow.isValid() && above.y() >= availBelow.top()) ? above : below;
        }
        if (QScreen* screen = QApplication::screenAt(pos))
        {
            const QRect avail = screen->availableGeometry();
            pos.setX(qMin(pos.x(), avail.right() - popup->width()));
            pos.setY(qBound(avail.top(), pos.y(), avail.bottom() - popup->height()));
        }
        popup->move(pos);
        popup->show();
    }

    void mDefineCurveDlg::applyPickedRef(QLineEdit* cellEdit, const QString& ref)
    {
        //名称/值参数格替换（textChanged 触发重新合成函数调用串）
        if (cellEdit && !ref.isEmpty())
            cellEdit->setText(ref);
    }

    // =============================================================================
    // 数值来源页
    // =============================================================================

    QList<double> mDefineCurveDlg::parseValues(const QString& text)
    {
        return parseNumberList(text);
    }

    //条目显示名（列表行/数值表列头同源）：名称框文本优先，空则自动名，再退
    //回 Curve N（N=条目序，柱状图共享表各列随条目位次命名）
    QString mDefineCurveDlg::entryDisplayName(int entryIndex) const
    {
        if (entryIndex < 0 || entryIndex >= _entries.size())
            return QString();
        const DefineCurveEntry& e = _entries.at(entryIndex);
        QString text = !e.name.trimmed().isEmpty() ? e.name.trimmed() : entryAutoName(e);
        if (text.trimmed().isEmpty())
            text = QStringLiteral("Curve %1").arg(entryIndex + 1);
        return text;
    }

    //柱状图左栏首行为 Categories（共享刻度名编辑入口，设计图），条目行号 +1
    bool mDefineCurveDlg::categoriesRowActive() const
    {
        return targetIsBarChart() && _curveList && _curveList->currentRow() == 0;
    }

    int mDefineCurveDlg::entryToListRow(int entryIndex) const
    {
        return targetIsBarChart() ? entryIndex + 1 : entryIndex;
    }

    int mDefineCurveDlg::listRowToEntryIndex(int listRow) const
    {
        return targetIsBarChart() ? listRow - 1 : listRow;   //Categories 行 -> -1
    }

    //左栏 Categories 行随图表类型增删（弹窗开着时经参数栏切 XY/柱状即时联动）
    void mDefineCurveDlg::syncBarListShape()
    {
        if (!_curveList)
            return;
        const bool bar = targetIsBarChart();
        const bool has = _curveList->count() > 0
            && _curveList->item(0)->text() == QStringLiteral("Categories");
        if (bar == has)
            return;
        _curveList->blockSignals(true);   //行结构变化不走选中信号（选中态由调用方重定）
        if (bar)
            _curveList->insertItem(0, QStringLiteral("Categories"));
        else
            delete _curveList->takeItem(0);
        _curveList->blockSignals(false);
    }

    //数值表列数/表头/列宽随模式与条目集（柱状共享表：Index + Categories +
    //每条目一列，列头=条目显示名 / 复数图：Pt + X + Yp + Ym / XY：Pt + X + Y，
    //设计图三列表，表头固定）
    void mDefineCurveDlg::syncValueTableShape()
    {
        if (!_valueTable)
            return;
        const bool complexChart = targetIsComplexChart();
        const bool bar = targetIsBarChart();
        QStringList heads;
        if (complexChart)
        {
            heads << QStringLiteral("Pt") << QStringLiteral("X")
                << QStringLiteral("Yp") << QStringLiteral("Ym");
        }
        else if (bar)
        {
            heads << QStringLiteral("Index") << QStringLiteral("Categories");
            for (int i = 0; i < _entries.size(); ++i)
                heads << entryDisplayName(i);
        }
        else
        {
            heads << QStringLiteral("Pt") << QStringLiteral("X") << QStringLiteral("Y");
        }
        _valueTable->setColumnCount(heads.size());
        _valueTable->setHorizontalHeaderLabels(heads);
        //列宽逐次重申（列数增减后新列会回退默认宽）：Pt/Index/Categories
        //定宽，数值列均分拉伸
        QHeaderView* hh = _valueTable->horizontalHeader();
        hh->setSectionResizeMode(0, QHeaderView::Fixed);
        _valueTable->setColumnWidth(0, 90);
        if (!bar)
        {
            for (int c = 1; c < heads.size(); ++c)
                hh->setSectionResizeMode(c, QHeaderView::Stretch);
            return;
        }
        hh->setSectionResizeMode(1, QHeaderView::Fixed);
        _valueTable->setColumnWidth(1, 120);
        for (int c = 2; c < heads.size(); ++c)
            hh->setSectionResizeMode(c, QHeaderView::Stretch);
    }

    //数值表按当前模式整表重建（柱状 ↔ 复数 ↔ XY 切换/复位后）：共享表
    //内容以条目记录为源回填（编辑路径均已即时落库，重建无损）
    void mDefineCurveDlg::rebuildValueTableView()
    {
        if (!_valueTable)
            return;
        _syncing = true;
        _valueTable->setRowCount(0);
        syncValueTableShape();
        if (targetIsBarChart())
        {
            for (int i = 0; i < _entries.size(); ++i)
                fillBarColumnFromEntry(i);
        }
        else
        {
            const int cur = currentEntryIndex();
            if (cur >= 0)
                fillValueTableSides(_entries.at(cur));   //复数图/XY 共用按侧展开
        }
        _syncing = false;
        refreshValueToolBarStates();   //整表重建后行数变化（回填不触发选择信号），重申启停
    }

    //共享表单条目数值列回填（列位=条目序 +2）：行不足自动扩行（新行全列补
    //item，否则格不可编辑）；刻度名仅在共享列为空处补写（列内容以表内编辑为准）
    void mDefineCurveDlg::fillBarColumnFromEntry(int entryIndex)
    {
        if (!_valueTable || entryIndex < 0 || entryIndex >= _entries.size())
            return;
        const DefineCurveEntry& e = _entries.at(entryIndex);
        static const QRegularExpression sep(QStringLiteral("[\\s,;，；]+"));
        const QStringList cats = e.categoriesText.split(sep, QString::SkipEmptyParts);
        const QStringList vals = e.valuesText.split(sep, QString::SkipEmptyParts);
        const int col = 2 + entryIndex;
        if (col >= _valueTable->columnCount())
            return;   //列形态未同步（syncValueTableShape 须先行）
        const int n = qMax(cats.size(), vals.size());
        for (int r = _valueTable->rowCount(); r < n; ++r)
        {
            _valueTable->insertRow(r);
            QTableWidgetItem* idx = new QTableWidgetItem(QString::number(r));
            idx->setFlags(idx->flags() & ~Qt::ItemIsEditable);   //Index 列只读
            _valueTable->setItem(r, 0, idx);
            for (int c = 1; c < _valueTable->columnCount(); ++c)
                _valueTable->setItem(r, c, new QTableWidgetItem);
        }
        for (int r = 0; r < n; ++r)
        {
            if (r < cats.size())
            {
                QTableWidgetItem* cat = _valueTable->item(r, 1);
                if (cat && cat->text().trimmed().isEmpty())
                    cat->setText(cats.at(r));
            }
            if (r < vals.size())
            {
                if (QTableWidgetItem* v = _valueTable->item(r, col))
                {
                    v->setText(vals.at(r));
                    markValueCellValidity(r, col);
                }
            }
        }
        //全列统一默认值 0（设计图"设置初值"同款）：新增曲线的列后于既有行
        //加出时格缺 item（不可编辑的死格），条目值不足行时尾部空格一并补 0；
        //已有数值格不动
        for (int r = 0; r < _valueTable->rowCount(); ++r)
        {
            QTableWidgetItem* v = _valueTable->item(r, col);
            if (!v)
            {
                v = new QTableWidgetItem(QStringLiteral("0"));
                _valueTable->setItem(r, col, v);
            }
            else if (r >= vals.size() && v->text().trimmed().isEmpty())
                v->setText(QStringLiteral("0"));
            markValueCellValidity(r, col);
        }
    }

    void mDefineCurveDlg::readValueTableIntoEntry(DefineCurveEntry& e) const
    {
        //柱状图共享表走 commitValueTableToEntry 全列落库，不经此；
        //复数图：可编辑列（数值侧）X/Yp/Ym 按行读回 pairXText/pairYpText/
        //pairYmText；XY（设计图 Pt|X|Y）：可编辑列 X/Y 读回 pairXText/
        //valuesText（该行可编辑格全空跳过）。只读回显列跳过——内容属该侧
        //当前来源装配数据，不属"数值"，读回会污染该侧数值串
        if (!_valueTable)
            return;
        const bool complexChart = targetIsComplexChart();
        //数值页仅在激活向量侧为数值时展开（fillValueTableSides 同口径）：
        //页不可见时表与该条目数值串不同步（可能为空表），跳过读回防抹掉
        //已存内容（柱状图不经此，共享表常驻）
        if (!targetIsBarChart()
            && sideSource(e, qBound(0, e.activeVector, 2)) != 2)
            return;
        //各数据列可编辑性（R21：仅激活列读回——其余列只读，内容属该侧当前
        //来源装配/数值串展示，不在本次编辑范围）
        const bool xEditable = qBound(0, e.activeVector, 2) == 0;
        const bool mainEditable = qBound(0, e.activeVector, 2) == 1;
        const bool ymEditable = complexChart && qBound(0, e.activeVector, 2) == 2;
        QStringList xs, vals, yps, yms;
        for (int r = 0; r < _valueTable->rowCount(); ++r)
        {
            const QTableWidgetItem* xIt = _valueTable->item(r, 1);
            const QTableWidgetItem* ypIt = _valueTable->item(r, 2);
            const QTableWidgetItem* ymIt = _valueTable->item(r, 3);
            const QString x = xEditable && xIt ? xIt->text().trimmed() : QString();
            const QString yp = complexChart && mainEditable && ypIt
                ? ypIt->text().trimmed() : QString();
            const QString ym = ymEditable && ymIt ? ymIt->text().trimmed() : QString();
            const QString v = !complexChart && mainEditable && ypIt
                ? ypIt->text().trimmed() : QString();
            const bool any = !x.isEmpty() || !yp.isEmpty() || !ym.isEmpty() || !v.isEmpty();
            if (!any)
                continue;   //可编辑格全空跳过
            if (complexChart)
            {
                xs.append(x);
                yps.append(yp);
                yms.append(ym);
            }
            else
            {
                xs.append(x);
                vals.append(v);
            }
        }
        if (complexChart)
        {
            e.pairXText = xs.join(QStringLiteral(", "));
            e.pairYpText = yps.join(QStringLiteral(", "));
            e.pairYmText = yms.join(QStringLiteral(", "));
            return;
        }
        e.pairXText = xs.join(QStringLiteral(", "));
        e.valuesText = vals.join(QStringLiteral(", "));
    }

    void mDefineCurveDlg::insertXYValueRow(int row, const QString& x, const QString& y)
    {
        //XY 数值表插行（设计图三列）：Pt 只读行号，col1..2=X/Y 数值格（初值由
        //调用方传入，"+"与右键插入均为 "0"——与复数图"设置初值"同款）
        if (row < 0 || row > _valueTable->rowCount())
            row = _valueTable->rowCount();   //越界一律表尾
        _valueTable->insertRow(row);
        QTableWidgetItem* idx = new QTableWidgetItem(QString::number(row));   //0 起（截图）
        idx->setFlags(idx->flags() & ~Qt::ItemIsEditable);   //Pt 列只读（行号自动）
        _valueTable->setItem(row, 0, idx);
        const QStringList cells { x, y };
        for (int c = 1; c < _valueTable->columnCount(); ++c)
        {
            _valueTable->setItem(row, c,
                new QTableWidgetItem(cells.value(c - 1)));
            markValueCellValidity(row, c);
        }
    }

    void mDefineCurveDlg::insertValueRow(int row, const QString& name, const QString& value)
    {
        //柱状图共享表插行：Categories 列落刻度名，各条目数值列统一落默认值
        //（调用方传 "0"=设置初值；粘贴路径传空串由剪贴板逐格覆盖），缺 item
        //的格不可编辑故全列补齐
        if (row < 0 || row > _valueTable->rowCount())
            row = _valueTable->rowCount();   //越界一律表尾
        _valueTable->insertRow(row);
        QTableWidgetItem* idx = new QTableWidgetItem(QString::number(row));   //0 起（截图）
        idx->setFlags(idx->flags() & ~Qt::ItemIsEditable);   //Index 列只读（行号自动）
        _valueTable->setItem(row, 0, idx);
        for (int c = 1; c < _valueTable->columnCount(); ++c)
        {
            _valueTable->setItem(row, c,
                new QTableWidgetItem(c == 1 ? name : value));
            if (c >= 2)
                markValueCellValidity(row, c);
        }
    }

    void mDefineCurveDlg::insertComplexValueRow(int row, const QString& x,
        const QString& yp, const QString& ym)
    {
        //复数图数值表插行（设计图四列）：Pt 只读行号，col1..3=X/Yp/Ym 数值格
        //（初值由调用方传入，"+"与右键插入均为 "0"——设计图"设置初值"）
        if (row < 0 || row > _valueTable->rowCount())
            row = _valueTable->rowCount();   //越界一律表尾
        _valueTable->insertRow(row);
        QTableWidgetItem* idx = new QTableWidgetItem(QString::number(row));   //0 起（截图）
        idx->setFlags(idx->flags() & ~Qt::ItemIsEditable);   //Pt 列只读（行号自动）
        _valueTable->setItem(row, 0, idx);
        const QStringList cells { x, yp, ym };
        for (int c = 1; c < _valueTable->columnCount(); ++c)
        {
            _valueTable->setItem(row, c,
                new QTableWidgetItem(cells.value(c - 1)));
            markValueCellValidity(row, c);
        }
    }

    void mDefineCurveDlg::renumberValueRows()
    {
        for (int r = 0; r < _valueTable->rowCount(); ++r)
        {
            if (QTableWidgetItem* idx = _valueTable->item(r, 0))
                idx->setText(QString::number(r));
        }
    }

    QList<int> mDefineCurveDlg::selectedValueRows() const
    {
        QList<int> rows;
        if (!_valueTable)
            return rows;
        for (int r = 0; r < _valueTable->rowCount(); ++r)
        {
            if (_valueTable->item(r, 1) && _valueTable->item(r, 1)->isSelected())
                rows.append(r);
        }
        return rows;   //升序
    }

    //数值页行操作工具条启停（设计稿 [+][-][清空]）：删除钮随选中行（无选中
    //不可删）、清空钮随表行数（无行不可清，与选中无关）
    void mDefineCurveDlg::refreshValueToolBarStates()
    {
        _delValueRowBtn->setEnabled(_valueTable->currentRow() >= 0);
        _clearValueBtn->setEnabled(_valueTable->rowCount() > 0);
    }

    void mDefineCurveDlg::commitValueTableToEntry()
    {
        if (targetIsBarChart())
        {
            //柱状图共享表落库：Categories 列全条目共享，各数值列按列位写入对
            //应条目——与当前选中条目无关（选中 Categories 行亦可编辑）。
            //行保留规则同常规表：刻度名或任一数值列非空的行视为数据行
            QStringList cats;
            QVector<QStringList> vals(_entries.size());
            for (int r = 0; r < _valueTable->rowCount(); ++r)
            {
                const QTableWidgetItem* cat = _valueTable->item(r, 1);
                const QString c = cat ? cat->text().trimmed() : QString();
                QStringList rowVals;
                bool any = !c.isEmpty();
                for (int i = 0; i < _entries.size(); ++i)
                {
                    const QTableWidgetItem* v = _valueTable->item(r, 2 + i);
                    const QString t = v ? v->text().trimmed() : QString();
                    rowVals.append(t);
                    any = any || !t.isEmpty();
                }
                if (!any)
                    continue;   //整行皆空跳过
                cats.append(c);
                for (int i = 0; i < vals.size(); ++i)
                    vals[i].append(rowVals.at(i));
            }
            //列尾空 token 去除（各列独立，行位对齐不受影响；空刻度格=留空，
            //应用时按索引刻度回退，设计图末行同款）
            while (!cats.isEmpty() && cats.last().isEmpty())
                cats.removeLast();
            for (int i = 0; i < vals.size(); ++i)
            {
                while (!vals[i].isEmpty() && vals[i].last().isEmpty())
                    vals[i].removeLast();
                _entries[i].categoriesText = cats.join(QStringLiteral(", "));
                _entries[i].valuesText = vals[i].join(QStringLiteral(", "));
            }
            refreshValueVectorTexts();
            refreshActionStates();
            return;
        }
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        readValueTableIntoEntry(_entries[cur]);
        refreshValueVectorTexts();
        refreshActionStates();
    }

    //数据列可编辑性（非柱状，R21 交互模型）：仅"激活向量列"可编辑（col1=X
    //侧、col2=Y/yp 侧、col3=ym 侧）——页面只随来源单选，激活侧来源恒等于
    //当前页来源（数值页激活列恒为数值侧）；柱状图=数值列（col2 起）全可编辑、
    //Categories 列不属数值列
    bool mDefineCurveDlg::isEditableValueColumn(int col) const
    {
        if (!_valueTable || col < 1 || col >= _valueTable->columnCount())
            return false;
        if (targetIsBarChart())
            return col >= 2;
        const int cur = currentEntryIndex();
        if (cur < 0)
            return false;
        return col - 1 == qBound(0, _entries.at(cur).activeVector, 2);
    }

    //剪切/复制/粘贴作用的列（按位映射剪贴板 token）：非柱状=激活向量列
    //（唯一可编辑列，R21）；柱状图=Categories+全部条目数值列（现状零变化）
    QList<int> mDefineCurveDlg::valueClipColumns() const
    {
        QList<int> cols;
        if (!_valueTable)
            return cols;
        if (targetIsBarChart())
        {
            for (int c = 1; c < _valueTable->columnCount(); ++c)
                cols.append(c);
            return cols;
        }
        const int sides = targetIsComplexChart() ? 3 : 2;
        for (int s = 0; s < sides; ++s)
            if (isEditableValueColumn(s + 1))
                cols.append(s + 1);
        return cols;
    }

    //首个可编辑数据列（新行落焦点用；无可编辑列返回 1——对只读格 editItem
    //无效果，无害兜底）
    int mDefineCurveDlg::firstEditableValueColumn() const
    {
        const QList<int> cols = valueClipColumns();
        return cols.isEmpty() ? 1 : cols.first();
    }

    //非数值侧列的回显 token 现算缓存（'f' 6，行位对齐）：文件=该侧通道数据/
    //兜底横坐标，数学=该侧表达式/引用求值；装配不出给空列（回显留空）。
    //数值侧列缓存空表（自身内容来自条目数值串，激活列可编辑/非激活列只读
    //展示，R21）
    void mDefineCurveDlg::computeSideEchoTokens(const DefineCurveEntry& e)
    {
        _sideEchoTokens.clear();
        const int sides = targetIsComplexChart() ? 3 : 2;
        for (int s = 0; s < sides; ++s)
        {
            QStringList toks;
            if (sideSource(e, s) != 2)
            {
                try
                {
                    QVector<double> v;
                    QString err;
                    if (assembleSideData(e, s, v, err))
                        for (int i = 0; i < v.size(); ++i)
                            toks << QString::number(v.at(i), 'f', 6);
                }
                catch (...)
                {
                    //装配异常按"该侧给不出回显"处理：留空
                }
            }
            _sideEchoTokens.append(toks);
        }
    }

    //行内格状态收口（非柱状，R21）：仅激活向量列可编辑（保持编辑标志并重申
    //红字校验）；非激活列去掉编辑标志（双击/键入/粘贴均不可改）——非数值侧
    //按行位回显该侧装配数据（超出留空），数值侧保持填表时写入的数值串 token
    //（fillValueTableSides/插行已按行位落；computeSideEchoTokens 对数值侧缓存
    //空表，不回填）。行插入/整表回填后调用
    void mDefineCurveDlg::applySideCellStates(int row)
    {
        const int cur = currentEntryIndex();
        if (!_valueTable || cur < 0 || targetIsBarChart()
            || row < 0 || row >= _valueTable->rowCount())
            return;
        const DefineCurveEntry& e = _entries.at(cur);
        const int sides = targetIsComplexChart() ? 3 : 2;
        const int active = qBound(0, e.activeVector, 2);
        for (int s = 0; s < sides; ++s)
        {
            QTableWidgetItem* it = _valueTable->item(row, s + 1);
            if (!it)
                continue;
            if (s == active)
            {
                it->setFlags(it->flags() | Qt::ItemIsEditable);
                markValueCellValidity(row, s + 1);
            }
            else
            {
                it->setFlags(it->flags() & ~Qt::ItemIsEditable);
                if (sideSource(e, s) != 2)   //数值侧列保留填表时的数值串（内容语义按来源判定）
                    it->setText(_sideEchoTokens.value(s).value(row, QString()));
            }
        }
    }

    //非柱状数值表按各侧来源展开（设计图 Pt|X|Y / Pt|X|Yp|Ym）：数值侧列=
    //该侧数值串按行 token（激活列可编辑，非激活数值列只读展示，R21），
    //非数值侧列=该侧当前来源装配的实际数据（'f' 6 只读回显，行位对齐、
    //超出留空）；行数=各列最大值。全数值侧时与既有"两/三串按行位配对展开"
    //完全一致。
    //激活向量侧非数值时数值页不可见：直接空表收场（回显装配成本省去）——
    //切入数值（applySourceChange）与数值页同源点向量（onVectorChanged）路径
    //总会带着可见页重新展开，不会漏填
    void mDefineCurveDlg::fillValueTableSides(const DefineCurveEntry& e)
    {
        if (!_valueTable)
            return;
        _valueTable->setRowCount(0);
        _sideEchoTokens.clear();
        const bool complexChart = targetIsComplexChart();
        const int sides = complexChart ? 3 : 2;
        if (sideSource(e, qBound(0, e.activeVector, 2)) != 2)
        {
            //页不可见收口为空表：工具条随行数归零重申启停（单点收口出口之一，
            //R21-B01）
            refreshValueToolBarStates();
            return;
        }
        computeSideEchoTokens(e);
        static const QRegularExpression sep(QStringLiteral("[\\s,;，；]+"));
        QList<QStringList> cols;
        for (int s = 0; s < sides; ++s)
        {
            //数值侧=条目数值串切分；非数值侧=装配数据回显缓存
            cols.append(sideSource(e, s) == 2
                ? sideValueText(e, s).split(sep, QString::SkipEmptyParts)
                : _sideEchoTokens.value(s));
        }
        int n = 0;
        for (const QStringList& c : cols)
            n = qMax(n, c.size());
        for (int i = 0; i < n; ++i)
        {
            const int row = _valueTable->rowCount();
            if (complexChart)
                insertComplexValueRow(row,
                    cols.value(0).value(i, QString()),
                    cols.value(1).value(i, QString()),
                    cols.value(2).value(i, QString()));
            else
                insertXYValueRow(row,
                    cols.value(0).value(i, QString()),
                    cols.value(1).value(i, QString()));
            applySideCellStates(row);
        }
        //工具条启停收口单点化（R21-B01）：本函数被 loadEntryToPanel/
        //applySourceChange 切页/onVectorChanged 同源焦点等直接调用（不经带
        //刷新的 rebuildValueTableView），0 行↔N 行切换后"清空"钮须随行数
        //即时恢复（启用口径=有行即可，与选中无关）；loadEntryToPanel/rebuild
        //处已有刷新，重复调用无害
        refreshValueToolBarStates();
    }

    void mDefineCurveDlg::markValueCellValidity(int row, int col)
    {
        //数值列自 XY/复数图的 X 列（col 1）起，柱状自第 3 列（col 2）起；
        //Pt/Index/Categories 不标红。红字只标可编辑列（激活列，R21）——非激活
        //只读列（装配回显/数值串展示）不参与校验
        const int firstValueCol = targetIsBarChart() ? 2 : 1;
        if (col < firstValueCol)
            return;
        if (!isEditableValueColumn(col))
            return;
        QTableWidgetItem* it = _valueTable ? _valueTable->item(row, col) : nullptr;
        if (!it)
            return;
        const QString t = it->text().trimmed();
        bool ok = true;
        if (!t.isEmpty())
            t.toDouble(&ok);
        //非法数值红字（配色同预览错误文案），留空视为未输入不标红
        it->setForeground(ok ? QBrush() : QBrush(QColor(200, 60, 60)));
    }

    void mDefineCurveDlg::refreshValueColumnHeader()
    {
        //数值列头=条目显示名（截图"Curve 1"），与列表行文字同源（entryDisplayName）：
        //柱状图共享表各条目列随名称联动刷新；复数图/XY 表头固定（Pt/X/Yp/Ym、
        //Pt/X/Y），不随条目名联动
        if (!_valueTable)
            return;
        if (!targetIsBarChart())
            return;
        if (targetIsBarChart())
        {
            for (int i = 0; i < _entries.size() && 2 + i < _valueTable->columnCount(); ++i)
            {
                if (QTableWidgetItem* head = _valueTable->horizontalHeaderItem(2 + i))
                    head->setText(entryDisplayName(i));
            }
            return;
        }
        const int cur = currentEntryIndex();
        QString text = cur >= 0 ? entryDisplayName(cur) : QString();
        if (QTableWidgetItem* head = _valueTable->horizontalHeaderItem(2))
            head->setText(text);
    }

    void mDefineCurveDlg::onValueCellChanged(int row, int col)
    {
        if (col < 1)
            return;   //Pt/Index 列只读，正常不会触发
        if (row >= 0)
            markValueCellValidity(row, col);   //数值列起点按模式界定（内部跳过非数值/只读列）
        if (_syncing)
            return;
        if (currentEntryIndex() >= 0)
        {
            //用户改过数值内容（脏标记按向量侧持有：col-1 即向量侧；
            //柱状图整条数值语义映射 yValueDirty）
            if (targetIsBarChart())
                _entries[currentEntryIndex()].yValueDirty = true;
            else
                setSideValueDirty(_entries[currentEntryIndex()], col - 1, true);
        }
        commitValueTableToEntry();   //柱状图共享表全列落库 / 常规与复数表读回当前条目
    }

    void mDefineCurveDlg::onAddValueRow()
    {
        //柱状图选中 Categories 行时同样可增行（共享表编辑不依赖选中条目）
        const int cur = currentEntryIndex();
        if (cur < 0 && !categoriesRowActive())
            return;
        //数值页仅在激活向量侧为数值时展开：页不可见（表未展开）时行操作无意义
        if (cur >= 0 && !targetIsBarChart()
            && sideSource(_entries.at(cur), qBound(0, _entries.at(cur).activeVector, 2)) != 2)
            return;
        _syncing = true;
        if (targetIsComplexChart())
        {
            //设计图"设置初值"：新增行 X/Yp/Ym 三格置 0
            insertComplexValueRow(-1, QStringLiteral("0"), QStringLiteral("0"),
                QStringLiteral("0"));   //表尾追加
        }
        else if (targetIsBarChart())
        {
            //柱状共享表新增行（统一默认值 0）：Categories + 全部曲线列置 0
            insertValueRow(-1, QStringLiteral("0"), QStringLiteral("0"));   //表尾追加
        }
        else
        {
            //XY（设计图 Pt|X|Y）：新增行 X/Y 两格置 0
            insertXYValueRow(-1, QStringLiteral("0"), QStringLiteral("0"));   //表尾追加
        }
        const int r = _valueTable->rowCount() - 1;
        applySideCellStates(r);   //只读列按行位回显该侧数据（超出留空），可编辑列重申校验
        renumberValueRows();
        _syncing = false;
        if (currentEntryIndex() >= 0)
            markValueRowOpDirty(_entries[currentEntryIndex()]);   //增行=数值侧内容改动
        //新行落首个可编辑数值格直接可输（复数图=X，常规=X 列；只读侧列跳过；
        //空行不写回条目，输入后经 onValueCellChanged 入库）
        const int col = firstEditableValueColumn();
        _valueTable->setCurrentCell(r, col);
        _valueTable->editItem(_valueTable->item(r, col));
        refreshValueToolBarStates();   //增行后重申启停（清空钮随行数启用）
    }

    //行级操作（增/删/清空/右键插入删除）影响整表行：行数变化实际改写的是
    //激活列（唯一可编辑列）的行位内容——置脏激活侧；非激活列只读（其数值
    //串/来源回显不随行操作改写，下次展开按各自记录/来源现算）；柱状图整条
    //数值语义映射 yValueDirty（现状零变化）
    void mDefineCurveDlg::markValueRowOpDirty(DefineCurveEntry& e)
    {
        if (targetIsBarChart())
        {
            e.yValueDirty = true;
            return;
        }
        setSideValueDirty(e, qBound(0, e.activeVector, 2), true);
    }

    void mDefineCurveDlg::onDeleteValueRow()
    {
        //支持多选删除（截图交互：Ctrl/Shift 一次选中多行）
        const QList<int> rows = selectedValueRows();
        if (rows.isEmpty())
            return;
        _syncing = true;
        for (int i = rows.size() - 1; i >= 0; --i)
            _valueTable->removeRow(rows.at(i));
        renumberValueRows();
        _syncing = false;
        if (currentEntryIndex() >= 0)
            markValueRowOpDirty(_entries[currentEntryIndex()]);   //删行=数值侧内容改动
        commitValueTableToEntry();
        refreshValueToolBarStates();   //删行后重申启停（行数减少，清空钮可能置灰）
    }

    void mDefineCurveDlg::onClearValueRows()
    {
        //设计稿工具条 [+][-][清空] 第三钮（用户增强项，HyperGraph 仅逐行增删）：
        //一键清空数值表全部行，等同"全选+删除"，行为口径与 onDeleteValueRow
        //一致。破坏性操作（柱状共享表清空影响全部条目列），执行前弹与"离开
        //数值"脏确认同款确认框（R21-B01，用户拍板）
        const int cur = currentEntryIndex();
        if (cur < 0 && !categoriesRowActive())
            return;   //与 onAddValueRow 同款卫语句
        //数值页仅在激活向量侧为数值时展开：页不可见时清空无意义（照旧拒绝）
        if (cur >= 0 && !targetIsBarChart()
            && sideSource(_entries.at(cur), qBound(0, _entries.at(cur).activeVector, 2)) != 2)
            return;
        if (_valueTable->rowCount() <= 0)
            return;   //无行可清（按钮置灰兜底，同删行空选：直接返回不置脏）
        //确认框（样式同"离开数值"脏确认：提示/取消/继续，默认继续）；取消=
        //不清空不置脏，继续=走既有清空逻辑（置脏/落库/刷新不变）
        if (QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("确认清空全部行数据？是否继续？"),
            QStringLiteral("取消"), QStringLiteral("继续"), QString(), 1, 0) != 1)
            return;
        _syncing = true;
        _valueTable->setRowCount(0);
        renumberValueRows();   //程序回填不触发 onValueCellChanged
        _syncing = false;
        if (currentEntryIndex() >= 0)
            markValueRowOpDirty(_entries[currentEntryIndex()]);   //清空=数值侧内容改动
        commitValueTableToEntry();   //条目数值串清空落库 + 向量回显刷新 + 值预览联动
        refreshValueToolBarStates();   //行数归零，清空钮随即置灰
    }

    void mDefineCurveDlg::onValueContextMenu(const QPoint& pos)
    {
        //柱状图选中 Categories 行时同样可操作（共享表编辑不依赖选中条目）
        const int cur = currentEntryIndex();
        if (cur < 0 && !categoriesRowActive())
            return;
        //数值页仅在激活向量侧为数值时展开：页不可见时右键操作无意义（照旧拒绝）
        if (cur >= 0 && !targetIsBarChart()
            && sideSource(_entries.at(cur), qBound(0, _entries.at(cur).activeVector, 2)) != 2)
            return;
        const int row = _valueTable->rowAt(pos.y());
        //右键落在某行且该行不在选中集中时，先选中该行（右键即选中，照常见表格交互）
        if (row >= 0 && _valueTable->item(row, 1)
            && !_valueTable->item(row, 1)->isSelected())
            _valueTable->setCurrentCell(row, 1);
        const bool hasRow = row >= 0;
        const QList<int> rows = selectedValueRows();
        const bool hasSel = !rows.isEmpty();

        MechMainWindow::mUiMenu menu(this);
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

        //启停（截图）：无行不可插、无选中不可剪切/复制/删除、剪贴板空不可粘贴
        insAbove->setEnabled(hasRow);
        insBelow->setEnabled(hasRow);
        cutAct->setEnabled(hasSel);
        copyAct->setEnabled(hasSel);
        pasteAct->setEnabled(!_valueClipRows.isEmpty());
        delAct->setEnabled(hasSel);

        QAction* act = menu.exec(_valueTable->viewport()->mapToGlobal(pos));
        if (!act)
            return;
        if (act == insAbove || act == insBelow)
        {
            _syncing = true;
            if (targetIsComplexChart())
            {
                //设计图"设置初值"：复数图插入行 X/Yp/Ym 三格置 0
                insertComplexValueRow(act == insAbove ? row : row + 1,
                    QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("0"));
            }
            else if (targetIsBarChart())
                insertValueRow(act == insAbove ? row : row + 1,
                    QStringLiteral("0"), QStringLiteral("0"));   //统一默认值 0
            else
            {
                //XY：插入行 X/Y 两格置 0（同复数图"设置初值"）
                insertXYValueRow(act == insAbove ? row : row + 1,
                    QStringLiteral("0"), QStringLiteral("0"));
            }
            const int r = act == insAbove ? row : row + 1;
            applySideCellStates(r);   //只读列按行位回显该侧数据（超出留空）
            renumberValueRows();
            _syncing = false;
            const int col = firstEditableValueColumn();
            _valueTable->setCurrentCell(r, col);
            _valueTable->editItem(_valueTable->item(r, col));   //新行落首个可编辑格直接可输
            //显式重申工具条启停（行数变化不依赖 setCurrentCell 选择信号间接
            //刷新，R21-B01；本分支在共用收口前提前 return）
            refreshValueToolBarStates();
            return;
        }
        if (act == cutAct || act == copyAct)
        {
            //选中行按序放入行级剪贴板（弹窗会话内，与左侧条目剪贴板互不相通）。
            //每行=[各可编辑列值...]（只读回显列不参与剪切/复制）——柱状图=
            //Categories+全部数值列（现状），常规/复数=数值侧列按位
            _valueClipRows.clear();
            const QList<int> clipCols = valueClipColumns();
            for (int r : rows)
            {
                QStringList rowClip;
                for (int c : clipCols)
                {
                    const QTableWidgetItem* it = _valueTable->item(r, c);
                    rowClip.append(it ? it->text() : QString());
                }
                _valueClipRows.append(rowClip);
            }
            if (act == copyAct)
                return;   //复制到此为止；剪切继续走删除
        }
        if (act == cutAct || act == delAct)
        {
            _syncing = true;
            for (int i = rows.size() - 1; i >= 0; --i)
                _valueTable->removeRow(rows.at(i));
            renumberValueRows();
            _syncing = false;
        }
        else if (act == pasteAct)
        {
            //粘贴到最后一选中行之后（无选中/空白处=表尾）；剪贴板 token 按
            //位落到可编辑列（valueClipColumns 顺序映射，只读列跳过；跨模式
            //剪贴以可编辑列数截断），数值格随写随校验
            _syncing = true;
            const QList<int> clipCols = valueClipColumns();
            const int at = rows.isEmpty() ? _valueTable->rowCount() : rows.last() + 1;
            for (int i = 0; i < _valueClipRows.size(); ++i)
            {
                if (targetIsComplexChart())
                    insertComplexValueRow(at + i, QString(), QString(), QString());
                else if (targetIsBarChart())
                    insertValueRow(at + i, QString(), QString());
                else
                    insertXYValueRow(at + i, QString(), QString());
                const QStringList& clip = _valueClipRows.at(i);
                for (int j = 0; j < clip.size() && j < clipCols.size(); ++j)
                {
                    if (QTableWidgetItem* it = _valueTable->item(at + i, clipCols.at(j)))
                    {
                        it->setText(clip.at(j));
                        markValueCellValidity(at + i, clipCols.at(j));   //数值列起点内部界定
                    }
                }
                applySideCellStates(at + i);   //只读列回显收口
            }
            renumberValueRows();
            _syncing = false;
        }
        if (currentEntryIndex() >= 0)
            markValueRowOpDirty(_entries[currentEntryIndex()]);   //插入/剪切/删除/粘贴=数值侧内容改动（复制已在上方返回）
        commitValueTableToEntry();
        refreshValueToolBarStates();   //粘贴等不触发选择信号的行数变化，重申启停
    }

    void mDefineCurveDlg::refreshValueVectorTexts()
    {
        const int cur = currentEntryIndex();
        if (cur < 0)
            return;
        DefineCurveEntry& e = _entries[cur];
        if (!targetIsBarChart())
        {
            //复数图/XY（设计图"数值置灰模式"）：数值在表格按行输入，数值侧
            //向量框仅置灰占位，无回显文本；非数值侧（文件/数学）回显保持不动
            for (int side = 0; side < (targetIsComplexChart() ? 3 : 2); ++side)
            {
                if (sideSource(e, side) != 2)
                    continue;
                QLineEdit* edit = side == 0 ? _xEdit
                    : (side == 1 ? _yEdit : _ymEdit);
                edit->setText(QString());
                switch (side)
                {
                case 0:  e.xText.clear(); break;
                case 1:
                    if (targetIsComplexChart()) e.ypText.clear(); else e.yText.clear();
                    break;
                default: e.ymText.clear(); break;
                }
            }
            return;
        }
        const QList<double> vals = parseNumberList(e.valuesText);
        if (vals.isEmpty())
        {
            e.xText.clear();
            e.yText.clear();
        }
        else
        {
            e.xText = QStringLiteral("索引 0..%1").arg(vals.size() - 1);
            e.yText = vals.size() == 1
                ? QStringLiteral("常量 %1").arg(vals.first())
                : QStringLiteral("%1 个数值").arg(vals.size());
        }
        _xEdit->setText(e.xText);
        _yEdit->setText(e.yText);
    }

    // =============================================================================
    // 引用解析与数据装配
    // =============================================================================

    bool mDefineCurveDlg::resolveRef(const QString& ref, int* curveId, bool* isX,
        QString* normalized) const
    {
        static const QRegularExpression re(
            QStringLiteral("^\\s*[pP](\\d+)[wW](\\d+)[cC](\\d+)\\.([xy])\\s*$"));
        const QRegularExpressionMatch m = re.match(ref);
        if (!m.hasMatch())
            return false;
        const int pi = m.captured(1).toInt();
        const int wi = m.captured(2).toInt();
        const int ci = m.captured(3).toInt();
        const bool wantX = m.captured(4).toLower() == QLatin1String("x");

        CurveDocument* doc = mCurveDocController::getInstance()->document();
        int p = 0;
        for (int pageId : doc->childNodeIds(doc->rootNodeId()))
        {
            if (doc->nodeType(pageId) != CurveNodeType::Page)
                continue;
            ++p;
            if (p != pi)
                continue;
            int w = 0;
            for (int chartId : doc->childNodeIds(pageId))
            {
                if (doc->nodeType(chartId) != CurveNodeType::Chart)
                    continue;
                ++w;
                if (w != wi)
                    continue;
                int c = 0;
                for (int id : doc->childNodeIds(chartId))
                {
                    if (doc->nodeType(id) != CurveNodeType::Curve)
                        continue;
                    ++c;
                    if (c != ci)
                        continue;
                    if (curveId)
                        *curveId = id;
                    if (isX)
                        *isX = wantX;
                    if (normalized)
                        *normalized = QStringLiteral("p%1w%2c%3.%4")
                        .arg(pi).arg(wi).arg(ci).arg(wantX ? "x" : "y");
                    return true;
                }
            }
        }
        return false;
    }

    bool mDefineCurveDlg::buildRefData(const QString& ref, CurveRefData* out, QString& errMsg) const
    {
        int curveId = -1;
        bool isX = false;
        QString norm;
        if (!resolveRef(ref, &curveId, &isX, &norm))
        {
            errMsg = QStringLiteral("引用向量无效或不存在：%1（可能已被删除）")
                .arg(ref.trimmed());
            return false;
        }
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        const CurveNode* node = doc->node(curveId);
        if (!node)
        {
            errMsg = QStringLiteral("引用曲线节点不存在");
            return false;
        }
        const CurveSeriesData& s = node->series;
        if (isX)
        {
            //x 向量：取值=曲线 X 数据，自变量=采样索引
            out->x.resize(s.xData.size());
            for (int i = 0; i < s.xData.size(); ++i)
                out->x[i] = double(i);
            out->y = s.xData;
        }
        else
        {
            out->x = s.xData;
            out->y = s.yData;
        }
        if (out->x.isEmpty() || out->x.size() != out->y.size())
        {
            errMsg = QStringLiteral("引用向量 “%1” 无有效数据").arg(norm);
            return false;
        }
        out->ref = norm;
        return true;
    }

    bool mDefineCurveDlg::buildFileData(const DefineCurveEntry& e, QVector<double>& xData,
        QVector<double>& yData, QString& errMsg) const
    {
        //文件路径按向量侧独立（R20）：Y 侧（主侧）用 yFilePath 装配主通道，X 侧
        //选了通道时用 xFilePath（同文件/柱状图目标两侧退化映射整条 filePath 时
        //与拆分前行为逐字一致）。R22 起 csv/txt（ascii 列数据集）走实例版
        //assembleChannelData（列名首个匹配取列，yData=列值）
        const QString yPath = sideFilePath(e, 1);
        const bool yAscii = isAsciiDataset(yPath);
        //装配规则（块匹配/缺虚部防线/取值次序）统一在共享仓库 assembleChannelData
        //（"从文件创建曲线"/"更改曲线"同源），本函数仅薄包装 + 上报告警。
        //Y 侧主通道先行装配
        const PchReader::PchFile* dataFile = nullptr;   //pch 分支解析数据（ascii 为空）
        mCurveDataSourceStore::ChannelDataResult r;
        if (yAscii)
        {
            if (!mCurveDataSourceStore::getInstance()->assembleChannelData(
                yPath, e.yChannel.subcaseId, e.yChannel.schema, e.yChannel.pointId,
                e.yChannel.component, &r, &errMsg))
            {
                return false;
            }
        }
        else
        {
            dataFile = mCurveDataSourceStore::getInstance()->file(yPath);
            if (!dataFile)
            {
                errMsg = QStringLiteral("数据文件不在共享仓库：%1").arg(QFileInfo(yPath).fileName());
                return false;
            }
            if (!mCurveDataSourceStore::assembleChannelData(
                *dataFile, e.yChannel.subcaseId, e.yChannel.schema, e.yChannel.pointId,
                e.yChannel.component, &r, &errMsg))
            {
                return false;
            }
        }
        if (r.skippedNoSecondary)
        {
            mCurveDocController::getInstance()->reportWarning(
                QStringLiteral("[定义曲线] 通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
                .arg(e.yChannel.objectText, e.yChannel.component));
        }
        //X 侧：未选通道时取记录横坐标（ascii 无块横坐标——回退主侧文件第 1 列，
        //R22 用户拍板）；选了通道则按 X 侧自身文件装配该通道分量值
        //作 X（X 侧使用自己的工况选择，与 Y 侧相互独立；点数须与 Y 一致才配对——
        //既有长度校验/报错口径保留，全文件条目不静默截断）
        if (e.xChannel.complete())
        {
            const QString xPath = sideFilePath(e, 0);
            const bool xAscii = isAsciiDataset(xPath);
            const PchReader::PchFile* xFile = xAscii ? nullptr
                : (xPath == yPath ? dataFile
                    : mCurveDataSourceStore::getInstance()->file(xPath));
            if (!xFile && !xAscii)
            {
                errMsg = QStringLiteral("数据文件不在共享仓库：%1").arg(QFileInfo(xPath).fileName());
                return false;
            }
            mCurveDataSourceStore::ChannelDataResult rx;
            if (xFile)
            {
                if (!mCurveDataSourceStore::assembleChannelData(
                    *xFile, e.xChannel.subcaseId, e.xChannel.schema, e.xChannel.pointId,
                    e.xChannel.component, &rx, &errMsg))
                {
                    return false;
                }
            }
            else if (!mCurveDataSourceStore::getInstance()->assembleChannelData(
                xPath, e.xChannel.subcaseId, e.xChannel.schema, e.xChannel.pointId,
                e.xChannel.component, &rx, &errMsg))
            {
                return false;
            }
            if (rx.skippedNoSecondary)
            {
                mCurveDocController::getInstance()->reportWarning(
                    QStringLiteral("[定义曲线] X侧通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
                    .arg(e.xChannel.objectText, e.xChannel.component));
            }
            if (rx.yData.size() != r.yData.size())
            {
                errMsg = QStringLiteral("X/Y 数据点数不一致（X:%1 Y:%2）")
                    .arg(rx.yData.size()).arg(r.yData.size());
                return false;
            }
            xData = std::move(rx.yData);
        }
        else if (yAscii)
        {
            //R22：ascii 无块横坐标语义——X 未选通道回退主侧文件第 1 列（与
            //四级列 X 侧默认选中第 1 列同口径；同文件同列数，行位天然对齐）
            const QStringList cols =
                mCurveDataSourceStore::getInstance()->asciiColumnNames(yPath);
            mCurveDataSourceStore::ChannelDataResult rx;
            if (cols.isEmpty()
                || !mCurveDataSourceStore::getInstance()->assembleChannelData(
                    yPath, mCurveDataSourceStore::asciiSubcaseId(),
                    mCurveDataSourceStore::asciiSchemaDisplayText(),
                    mCurveDataSourceStore::asciiPointId(), cols.first(), &rx, &errMsg))
            {
                if (errMsg.isEmpty())
                    errMsg = QStringLiteral("数据文件无可用数据列：%1")
                        .arg(QFileInfo(yPath).fileName());
                return false;
            }
            xData = std::move(rx.yData);
        }
        else
        {
            xData = std::move(r.xData);
        }
        yData = std::move(r.yData);
        return true;
    }

    bool mDefineCurveDlg::buildFilePairData(const DefineCurveEntry& e, QVector<double>& xData,
        QVector<double>& yMag, QVector<double>& yPha, QString& errMsg) const
    {
        //复数图目标（文件）：幅值/相位两通道分别装配（ym/yp 各按自身侧文件——
        //文件路径按向量侧独立（R20），同文件/柱状图目标退化映射时与拆分前逐字
        //一致；用户在 ym/yp 槽各自选通道，方向一致才成对，否则渲染层降级单曲线）；
        //X 取 X 侧通道或幅值侧记录横坐标（R22：ascii 幅值侧无块横坐标，回退
        //幅值侧文件第 1 列）。既有长度校验/报错口径保留，全文件条目不静默截断。
        //R22 起 csv/txt（ascii 列数据集）走实例版 assembleChannelData
        const QString magPath = sideFilePath(e, 2);
        const bool magAscii = isAsciiDataset(magPath);
        const PchReader::PchFile* magFile = magAscii
            ? nullptr : mCurveDataSourceStore::getInstance()->file(magPath);
        if (!magFile && !magAscii)
        {
            errMsg = QStringLiteral("数据文件不在共享仓库：%1").arg(QFileInfo(magPath).fileName());
            return false;
        }
        const QString phaPath = sideFilePath(e, 1);
        const bool phaAscii = isAsciiDataset(phaPath);
        const PchReader::PchFile* phaFile = phaAscii ? nullptr
            : (phaPath == magPath ? magFile
                : mCurveDataSourceStore::getInstance()->file(phaPath));
        if (!phaFile && !phaAscii)
        {
            errMsg = QStringLiteral("数据文件不在共享仓库：%1").arg(QFileInfo(phaPath).fileName());
            return false;
        }
        auto assembleSide = [&](const QString& dataPath, const PchReader::PchFile* dataFile,
            const DefineCurveEntry::FileChannel& ch,
            QVector<double>& y, const QString& side) -> bool
        {
            mCurveDataSourceStore::ChannelDataResult r;
            if (dataFile)
            {
                if (!mCurveDataSourceStore::assembleChannelData(
                    *dataFile, ch.subcaseId, ch.schema, ch.pointId, ch.component, &r, &errMsg))
                    return false;
            }
            else
            {
                //R22 ascii 列数据集：实例版装配（列名首个匹配取列）
                if (!mCurveDataSourceStore::getInstance()->assembleChannelData(
                    dataPath, ch.subcaseId, ch.schema, ch.pointId, ch.component, &r, &errMsg))
                    return false;
            }
            if (r.skippedNoSecondary)
            {
                mCurveDocController::getInstance()->reportWarning(
                    QStringLiteral("[定义曲线] %1侧通道“%2/%3”的数据块无虚部/相位数据（瞬态实数块），已跳过")
                    .arg(side, ch.objectText, ch.component));
            }
            y = std::move(r.yData);
            return true;
        };
        if (!assembleSide(magPath, magFile, e.ymChannel, yMag, QStringLiteral("幅值"))
            || !assembleSide(phaPath, phaFile, e.ypChannel, yPha, QStringLiteral("相位")))
            return false;
        if (yMag.size() != yPha.size())
        {
            errMsg = QStringLiteral("幅值/相位数据点数不一致（MAG:%1 PHA:%2）")
                .arg(yMag.size()).arg(yPha.size());
            return false;
        }
        //X 侧：未选通道时取幅值侧记录横坐标（ascii 回退幅值侧文件第 1 列，R22）；
        //选了通道则按 X 侧自身文件装配该通道并校验点数
        if (e.xChannel.complete())
        {
            const QString xPath = sideFilePath(e, 0);
            const bool xAscii = isAsciiDataset(xPath);
            const PchReader::PchFile* xFile = xAscii ? nullptr
                : (xPath == magPath ? magFile
                    : mCurveDataSourceStore::getInstance()->file(xPath));
            if (!xFile && !xAscii)
            {
                errMsg = QStringLiteral("数据文件不在共享仓库：%1").arg(QFileInfo(xPath).fileName());
                return false;
            }
            mCurveDataSourceStore::ChannelDataResult rx;
            if (xFile)
            {
                if (!mCurveDataSourceStore::assembleChannelData(
                    *xFile, e.xChannel.subcaseId, e.xChannel.schema, e.xChannel.pointId,
                    e.xChannel.component, &rx, &errMsg))
                    return false;
            }
            else if (!mCurveDataSourceStore::getInstance()->assembleChannelData(
                xPath, e.xChannel.subcaseId, e.xChannel.schema, e.xChannel.pointId,
                e.xChannel.component, &rx, &errMsg))
                return false;
            if (rx.yData.size() != yMag.size())
            {
                errMsg = QStringLiteral("X/复数对数据点数不一致（X:%1 Y:%2）")
                    .arg(rx.yData.size()).arg(yMag.size());
                return false;
            }
            xData = std::move(rx.yData);
        }
        else if (magAscii)
        {
            //R22：ascii 幅值侧无块横坐标——X 回退幅值侧文件第 1 列（与四级列
            //X 侧默认选中第 1 列同口径）
            const QStringList cols =
                mCurveDataSourceStore::getInstance()->asciiColumnNames(magPath);
            mCurveDataSourceStore::ChannelDataResult r;
            if (cols.isEmpty()
                || !mCurveDataSourceStore::getInstance()->assembleChannelData(
                    magPath, mCurveDataSourceStore::asciiSubcaseId(),
                    mCurveDataSourceStore::asciiSchemaDisplayText(),
                    mCurveDataSourceStore::asciiPointId(), cols.first(), &r, &errMsg))
            {
                if (errMsg.isEmpty())
                    errMsg = QStringLiteral("数据文件无可用数据列：%1")
                        .arg(QFileInfo(magPath).fileName());
                return false;
            }
            xData = std::move(r.yData);
        }
        else
        {
            mCurveDataSourceStore::ChannelDataResult r;
            if (!mCurveDataSourceStore::assembleChannelData(
                *magFile, e.ymChannel.subcaseId, e.ymChannel.schema, e.ymChannel.pointId,
                e.ymChannel.component, &r, &errMsg))
                return false;
            xData = std::move(r.xData);
        }
        return true;
    }

    bool mDefineCurveDlg::buildMathPairData(const DefineCurveEntry& e, QVector<double>& xData,
        QVector<double>& yMag, QVector<double>& yPha, QString& errMsg) const
    {
        //复数图目标（数学）：幅值/相位两个表达式在同一 X 采样上求值；X 取 X 行
        //向量引用，未填时取幅值表达式内的首个引用曲线 x
        xData.clear();
        yMag.clear();
        yPha.clear();
        errMsg.clear();
        const QString magExpr = e.ymText.trimmed();
        const QString phaExpr = e.ypText.trimmed();
        if (magExpr.isEmpty() || phaExpr.isEmpty())
        {
            errMsg = QStringLiteral("幅值/相位表达式为空");
            return false;
        }
        //X 采样：X 行向量引用优先，其次取幅值表达式首个引用曲线的 x
        QVector<double> xs;
        if (!e.xText.trimmed().isEmpty())
        {
            int curveId = -1;
            bool isX = false;
            QString norm;
            if (!resolveRef(e.xText, &curveId, &isX, &norm))
            {
                errMsg = QStringLiteral("X 向量引用无效：%1").arg(e.xText.trimmed());
                return false;
            }
            const CurveSeriesData& s = mCurveDocController::getInstance()
                ->document()->node(curveId)->series;
            xs = isX ? s.xData : s.yData;
        }
        else
        {
            const QStringList refs = mCurveExpressionEvaluator::extractRefs(magExpr);
            CurveRefData d;
            if (refs.isEmpty() || !buildRefData(refs.first(), &d, errMsg))
            {
                if (errMsg.isEmpty())
                    errMsg = QStringLiteral("请先指定 X 向量（向量行 fx 选择曲线向量，或在表达式中引用曲线）");
                return false;
            }
            xs = d.x;
        }
        if (xs.isEmpty())
        {
            errMsg = QStringLiteral("X 向量无数据");
            return false;
        }
        //两侧各自收集引用并求值（引用集可不同，X 采样一致即可配对）
        auto evalSide = [&](const QString& expr, QVector<double>& y) -> bool
        {
            const QStringList refs = mCurveExpressionEvaluator::extractRefs(expr);
            QList<CurveRefData> refData;
            for (const QString& ref : refs)
            {
                CurveRefData d;
                if (!buildRefData(ref, &d, errMsg))
                    return false;
                refData.push_back(d);
            }
            return mCurveExpressionEvaluator::evaluate(expr, refData, xs, y, errMsg);
        };
        if (!evalSide(magExpr, yMag) || !evalSide(phaExpr, yPha))
            return false;
        xData = xs;
        return true;
    }

    bool mDefineCurveDlg::buildMathData(const DefineCurveEntry& e, QVector<double>& xData,
        QVector<double>& yData, QString& errMsg, int maxSamples) const
    {
        xData.clear();
        yData.clear();
        errMsg.clear();
        const QString expr = e.expression.trimmed();
        if (expr.isEmpty())
        {
            errMsg = QStringLiteral("表达式为空");
            return false;
        }
        const QStringList refs = mCurveExpressionEvaluator::extractRefs(expr);

        //X 采样：X 向量引用优先，其次取首个引用曲线的 x，都没有则报错
        QVector<double> xs;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        if (!e.xText.trimmed().isEmpty())
        {
            int curveId = -1;
            bool isX = false;
            QString norm;
            if (!resolveRef(e.xText, &curveId, &isX, &norm))
            {
                errMsg = QStringLiteral("X 向量引用无效：%1").arg(e.xText.trimmed());
                return false;
            }
            const CurveSeriesData& s = doc->node(curveId)->series;
            xs = isX ? s.xData : s.yData;
        }
        else if (!refs.isEmpty())
        {
            CurveRefData d;
            if (!buildRefData(refs.first(), &d, errMsg))
                return false;
            xs = d.x;
        }
        else
        {
            errMsg = QStringLiteral("请先指定 X 向量（向量行 fx 选择曲线向量，或在表达式中引用曲线）");
            return false;
        }
        if (xs.isEmpty())
        {
            errMsg = QStringLiteral("X 向量无数据");
            return false;
        }
        //预览模式：全量 X 上均匀抽取 maxSamples 个采样点
        if (maxSamples > 1 && xs.size() > maxSamples)
        {
            QVector<double> sampled;
            const int n = xs.size();
            for (int i = 0; i < maxSamples; ++i)
                sampled.push_back(xs.at(int(double(i) * (n - 1) / (maxSamples - 1))));
            xs = sampled;
        }

        QList<CurveRefData> refData;
        for (const QString& ref : refs)
        {
            CurveRefData d;
            if (!buildRefData(ref, &d, errMsg))
                return false;
            refData.push_back(d);
        }

        if (!e.solve)
        {
            //未勾"计算求解"：表达式必须为单个直接引用（复制其数据）
            if (refs.size() == 1 && expr.toLower() == refs.first())
            {
                yData = refData.first().y;
                xData = xs;
                return true;
            }
            errMsg = QStringLiteral("未勾选\"计算求解\"时，表达式必须为单个曲线向量引用（如 p1w1c1.y）");
            return false;
        }
        if (!mCurveExpressionEvaluator::evaluate(expr, refData, xs, yData, errMsg))
            return false;
        xData = xs;
        return true;
    }

    bool mDefineCurveDlg::buildValueData(const DefineCurveEntry& e, QVector<double>& xData,
        QVector<double>& yData, QString& errMsg) const
    {
        const QList<double> vals = parseNumberList(e.valuesText);
        if (vals.isEmpty())
        {
            errMsg = QStringLiteral("数值序列为空或无有效数值");
            return false;
        }
        //X 列（设计图 Pt|X|Y）：数值表 X 列有数据时按行取值（空/非法格回退
        //行号），无 X 列（手工只录 Y）按索引 0..N-1
        static const QRegularExpression sep(QStringLiteral("[\\s,;，；]+"));
        const QStringList xs = e.pairXText.split(sep, QString::SkipEmptyParts);
        xData.reserve(vals.size());
        yData.reserve(vals.size());
        for (int i = 0; i < vals.size(); ++i)
        {
            double x = double(i);   //缺省按索引 0..N-1
            if (i < xs.size())
            {
                bool ok = false;
                x = xs.at(i).toDouble(&ok);
                if (!ok)
                    x = double(i);
            }
            xData.push_back(x);
            yData.push_back(vals.at(i));
        }
        return true;
    }

    void mDefineCurveDlg::normalizeComplexPairChannels(DefineCurveEntry& e) const
    {
        //复数对防呆：ym=幅值槽应持 MAG/REAL 令牌、yp=相位槽应持 PHA/IMAG 令牌；
        //两侧都完整但选反（如默认落在 yp 单选先选了 MAG）时自动对调通道与回显，
        //避免成对曲线上下两侧语义颠倒（幅值数据落相位面板）。仅对两侧均为
        //文件来源生效——数值/数学侧内容与通道分量令牌无对应关系
        if (sideSource(e, 1) != 0 || sideSource(e, 2) != 0
            || !e.ypChannel.complete() || !e.ymChannel.complete())
            return;
        const auto isUpper = [](const QString& token)
        {
            return token.startsWith(QStringLiteral("MAG"), Qt::CaseInsensitive)
                || token.startsWith(QStringLiteral("REAL"), Qt::CaseInsensitive);
        };
        const auto isLower = [](const QString& token)
        {
            return token.startsWith(QStringLiteral("PHA"), Qt::CaseInsensitive)
                || token.startsWith(QStringLiteral("IMAG"), Qt::CaseInsensitive);
        };
        if (isLower(e.ymChannel.component) && isUpper(e.ypChannel.component))
        {
            qSwap(e.ymChannel, e.ypChannel);
            qSwap(e.ymText, e.ypText);
            qSwap(e.ymFilePath, e.ypFilePath);   //文件路径跟随通道对调（路径按侧
                                                 //独立后两侧可不同文件，防错配装配）
        }
    }

    bool mDefineCurveDlg::buildValuePairData(const DefineCurveEntry& e,
        QVector<double>& xData, QVector<double>& yMag, QVector<double>& yPha,
        QString& errMsg) const
    {
        //复数图目标（数值来源，设计图）：表格 X/Yp/Ym 三列按行装配为成对数据
        //（Ym=上面板幅值，Yp=下面板相位，Pt=行号不入模）；空/非法格按 0 兜底
        //（保持行位对齐，表格已红字提示）
        xData.clear();
        yMag.clear();
        yPha.clear();
        errMsg.clear();
        static const QRegularExpression sep(QStringLiteral("[\\s,;，；]+"));
        const QStringList xs = e.pairXText.split(sep, QString::SkipEmptyParts);
        const QStringList yps = e.pairYpText.split(sep, QString::SkipEmptyParts);
        const QStringList yms = e.pairYmText.split(sep, QString::SkipEmptyParts);
        const int n = qMax(xs.size(), qMax(yps.size(), yms.size()));
        if (n == 0)
        {
            errMsg = QStringLiteral("数值序列为空或无有效数值");
            return false;
        }
        auto cell = [](const QStringList& tokens, int i) -> double
        {
            double v = 0.0;
            if (i < tokens.size())
                v = tokens.at(i).toDouble();
            return v;   //空/非法 toDouble 返回 0，行位不丢
        };
        xData.reserve(n);
        yMag.reserve(n);
        yPha.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            xData.push_back(cell(xs, i));
            yMag.push_back(cell(yms, i));
            yPha.push_back(cell(yps, i));
        }
        return true;
    }

    // =============================================================================
    // 混合来源（按向量侧）装配：各侧按自身来源装配取值序列，按行位配对、
    // 行数取较少者截断（用户拍板）；全数值/全数学保持既有装配口径
    // =============================================================================

    //数学表达式按采样求值（现状口径）：solve 不勾=单引用直接复制其数据；
    //采样 xs 为空时取表达式首个引用曲线的 x 兜底，xs 以入/出参回填实际采样
    //（X 侧给不出数据时供配对兜底，保持 X/ys 行位天然对齐）
    bool mDefineCurveDlg::evalMathSide(const DefineCurveEntry& e, const QString& expr,
        QVector<double>& xs, QVector<double>& out, QString& errMsg) const
    {
        out.clear();
        const QString trimmed = expr.trimmed();
        const QStringList refs = mCurveExpressionEvaluator::extractRefs(trimmed);
        QList<CurveRefData> refData;
        for (const QString& ref : refs)
        {
            CurveRefData d;
            if (!buildRefData(ref, &d, errMsg))
                return false;
            refData.push_back(d);
        }
        if (!e.solve)
        {
            //未勾"计算求解"：表达式必须为单个直接引用（复制其数据，现状口径）
            if (refs.size() == 1 && trimmed.toLower() == refs.first())
            {
                out = refData.first().y;
                return true;
            }
            errMsg = QStringLiteral("未勾选\"计算求解\"时，表达式必须为单个曲线向量引用（如 p1w1c1.y）");
            return false;
        }
        if (xs.isEmpty())
        {
            //采样兜底（现状口径）：表达式首个引用曲线的 x
            if (refs.isEmpty())
            {
                errMsg = QStringLiteral("请先指定 X 向量（向量行 fx 选择曲线向量，或在表达式中引用曲线）");
                return false;
            }
            CurveRefData d;
            if (!buildRefData(refs.first(), &d, errMsg))
                return false;
            xs = d.x;
        }
        if (xs.isEmpty())
        {
            errMsg = QStringLiteral("X 向量无数据");
            return false;
        }
        return mCurveExpressionEvaluator::evaluate(trimmed, refData, xs, out, errMsg);
    }

    //单侧按该侧来源装配取值序列（0=X 1=Y/yp 2=Ym）：文件=该侧文件+通道
    //（经 mCurveDataSourceStore::assembleChannelData 同源装配；X 侧未选通道
    //取主侧文件块横坐标——现状口径），数值=该侧数值串按行解析（混合条目口径，
    //保持行位不压缩：X 列空/非法格退行号、值列空/非法格按 0 兜底——单格兜底
    //规则分别与 buildValueData 的 X 列/buildValuePairData 一致；注意值列与全
    //数值条目 buildValueData 经 parseNumberList"跳过非法 token"的列表压缩口径
    //不同），数学=X 侧取引用向量数据，Y/yp/ym 侧按表达式求值（采样优先
    //X 侧数值/文件装配数据，两侧均非数值/文件时现状采样兜底）
    bool mDefineCurveDlg::assembleSideData(const DefineCurveEntry& e, int vector,
        QVector<double>& out, QString& errMsg) const
    {
        out.clear();
        errMsg.clear();
        const int src = sideSource(e, vector);
        if (src == 2)
        {
            //数值侧：按行解析（保持行位，不丢行）
            static const QRegularExpression sep(QStringLiteral("[\\s,;，；]+"));
            const QStringList toks = sideValueText(e, vector).split(sep, QString::SkipEmptyParts);
            if (toks.isEmpty())
            {
                errMsg = QStringLiteral("数值序列为空或无有效数值");
                return false;
            }
            out.reserve(toks.size());
            for (int i = 0; i < toks.size(); ++i)
            {
                bool ok = false;
                const double v = toks.at(i).toDouble(&ok);
                //X 列空/非法格退行号（单格规则同 buildValueData）；值列按 0
                //兜底（同 buildValuePairData）——混合条目口径保持行位不压缩，
                //与全数值条目 buildValueData 跳过非法 token 的口径不同
                out.push_back(ok ? v : (vector == 0 ? double(i) : 0.0));
            }
            return true;
        }
        if (src == 0)
        {
            const QString sidePath = sideFilePath(e, vector);
            const bool sideAscii = isAsciiDataset(sidePath);
            const PchReader::PchFile* dataFile = sideAscii
                ? nullptr : mCurveDataSourceStore::getInstance()->file(sidePath);
            if (!dataFile && !sideAscii)
            {
                errMsg = QStringLiteral("数据文件不在共享仓库：%1")
                    .arg(QFileInfo(sidePath).fileName());
                return false;
            }
            if (vector != 0)
            {
                //Y/yp/ym 侧：该侧通道分量值序列（R22：ascii 列数据集走实例版装配）
                mCurveDataSourceStore::ChannelDataResult r;
                const DefineCurveEntry::FileChannel& ch = sideChannel(e, vector);
                if (dataFile)
                {
                    if (!mCurveDataSourceStore::assembleChannelData(
                        *dataFile, ch.subcaseId, ch.schema, ch.pointId, ch.component, &r, &errMsg))
                        return false;
                }
                else if (!mCurveDataSourceStore::getInstance()->assembleChannelData(
                    sidePath, ch.subcaseId, ch.schema, ch.pointId, ch.component, &r, &errMsg))
                    return false;
                if (r.skippedNoSecondary)
                {
                    mCurveDocController::getInstance()->reportWarning(
                        QStringLiteral("[定义曲线] 通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
                        .arg(ch.objectText, ch.component));
                }
                out = std::move(r.yData);
                return true;
            }
            //X 侧：选了通道取该通道分量值；未选通道取主侧（Y/yp/ym 文件侧）
            //数据块横坐标——ascii 无块横坐标，回退主侧文件第 1 列（R22 用户拍板）
            if (e.xChannel.complete())
            {
                mCurveDataSourceStore::ChannelDataResult rx;
                if (dataFile)
                {
                    if (!mCurveDataSourceStore::assembleChannelData(
                        *dataFile, e.xChannel.subcaseId, e.xChannel.schema, e.xChannel.pointId,
                        e.xChannel.component, &rx, &errMsg))
                        return false;
                }
                else if (!mCurveDataSourceStore::getInstance()->assembleChannelData(
                    sidePath, e.xChannel.subcaseId, e.xChannel.schema, e.xChannel.pointId,
                    e.xChannel.component, &rx, &errMsg))
                    return false;
                if (rx.skippedNoSecondary)
                {
                    mCurveDocController::getInstance()->reportWarning(
                        QStringLiteral("[定义曲线] X侧通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
                        .arg(e.xChannel.objectText, e.xChannel.component));
                }
                out = std::move(rx.yData);
                return true;
            }
            const int mainSide = targetIsComplexChart() ? 2 : 1;
            if (sideSource(e, mainSide) == 0 && sideFileComplete(e, mainSide))
            {
                const QString mainPath = sideFilePath(e, mainSide);
                const DefineCurveEntry::FileChannel& ch = sideChannel(e, mainSide);
                if (isAsciiDataset(mainPath))
                {
                    //R22：ascii 主侧无块横坐标——X 回退主侧文件第 1 列（同文件
                    //行位天然对齐；B03 x 快照等块横坐标兜底链路同走此分支）
                    const QStringList cols =
                        mCurveDataSourceStore::getInstance()->asciiColumnNames(mainPath);
                    mCurveDataSourceStore::ChannelDataResult r;
                    if (cols.isEmpty()
                        || !mCurveDataSourceStore::getInstance()->assembleChannelData(
                            mainPath, mCurveDataSourceStore::asciiSubcaseId(),
                            mCurveDataSourceStore::asciiSchemaDisplayText(),
                            mCurveDataSourceStore::asciiPointId(), cols.first(), &r, &errMsg))
                    {
                        if (errMsg.isEmpty())
                            errMsg = QStringLiteral("数据文件无可用数据列：%1")
                                .arg(QFileInfo(mainPath).fileName());
                        return false;
                    }
                    out = std::move(r.yData);   //第 1 列值序列
                    return true;
                }
                const PchReader::PchFile* mainFile =
                    mCurveDataSourceStore::getInstance()->file(mainPath);
                if (!mainFile)
                {
                    errMsg = QStringLiteral("数据文件不在共享仓库：%1")
                        .arg(QFileInfo(mainPath).fileName());
                    return false;
                }
                mCurveDataSourceStore::ChannelDataResult r;
                if (!mCurveDataSourceStore::assembleChannelData(
                    *mainFile, ch.subcaseId, ch.schema, ch.pointId, ch.component, &r, &errMsg))
                    return false;
                out = std::move(r.xData);   //主侧数据块记录横坐标
                return true;
            }
            //第三级回退（R20-B03）：主侧已切离"文件"（或主侧文件回退不可用）
            //时横坐标断源——主侧切离时刻已把当时可解析的横坐标快照进 x 侧数值
            //串（onSourceChanged 落新值前），此处按行位解析该串兜底（非法格
            //退行号，同数值侧 X 列解析口径；串=快照或用户自输内容）；仍无才报错
            static const QRegularExpression sep(QStringLiteral("[\\s,;，；]+"));
            const QStringList xToks =
                sideValueText(e, vector).split(sep, QString::SkipEmptyParts);
            if (!xToks.isEmpty())
            {
                out.reserve(xToks.size());
                for (int i = 0; i < xToks.size(); ++i)
                {
                    bool ok = false;
                    const double v = xToks.at(i).toDouble(&ok);
                    out.push_back(ok ? v : double(i));   //X 列非法格退行号
                }
                return true;
            }
            errMsg = QStringLiteral("X 侧未选通道且主侧无文件横坐标可用");
            return false;
        }
        //数学侧
        if (vector == 0)
        {
            //X 侧数学=单引用：取引用向量数据（.x=曲线 X 数据，.y=曲线 Y 数据，
            //与 buildMathData 采样口径一致）
            const QString ref = e.xText.trimmed();
            if (ref.isEmpty())
            {
                errMsg = QStringLiteral("请先指定 X 向量引用（向量行 fx 选择曲线向量）");
                return false;
            }
            int curveId = -1;
            bool isX = false;
            QString norm;
            if (!resolveRef(ref, &curveId, &isX, &norm))
            {
                errMsg = QStringLiteral("X 向量引用无效：%1").arg(ref);
                return false;
            }
            const CurveSeriesData& s = mCurveDocController::getInstance()
                ->document()->node(curveId)->series;
            out = isX ? s.xData : s.yData;
            if (out.isEmpty())
            {
                errMsg = QStringLiteral("X 向量无数据");
                return false;
            }
            return true;
        }
        //Y/yp/ym 侧数学：表达式求值，采样优先 X 侧数值/文件装配数据（X 侧
        //数学或缺数据时 evalMathSide 现状采样兜底）
        QVector<double> samp;
        QString xErr;
        const int xsSrc = sideSource(e, 0);
        if (xsSrc == 0 || xsSrc == 2)
        {
            //X 侧非数学：其装配数据即采样（文件 X 侧未选通道时
            //assembleSideData 内部已取主侧横坐标）；装配失败则留空走兜底
            QVector<double> xsTmp;
            if (assembleSideData(e, 0, xsTmp, xErr))
                samp = xsTmp;
        }
        return evalMathSide(e, sideMathText(e, vector), samp, out, errMsg);
    }

    //混合来源 XY 装配：X/Y 各侧按自身来源装配，按行位配对、行数取较少者截断。
    //maxSamples>1 时 X 侧采样先均匀抽点再求值（buildMathData 预览同款）
    bool mDefineCurveDlg::buildMixedXYData(const DefineCurveEntry& e, QVector<double>& xData,
        QVector<double>& yData, QString& errMsg, int maxSamples) const
    {
        xData.clear();
        yData.clear();
        errMsg.clear();
        QVector<double> xs;
        QString xErr;
        const bool xOk = assembleSideData(e, 0, xs, xErr);
        if (xOk && maxSamples > 1 && xs.size() > maxSamples)
        {
            //预览模式：全量 X 上均匀抽取 maxSamples 个采样点（buildMathData 同款）
            QVector<double> sampled;
            const int n0 = xs.size();
            for (int i = 0; i < maxSamples; ++i)
                sampled.push_back(xs.at(int(double(i) * (n0 - 1) / (maxSamples - 1))));
            xs = sampled;
        }
        QVector<double> ys;
        if (sideSource(e, 1) == 1)
        {
            //Y 侧数学：xs 入/出=实际采样（X 缺数据时现状兜底采样回填，X/ys 对齐）
            if (!evalMathSide(e, e.expression, xs, ys, errMsg))
                return false;
        }
        else if (!assembleSideData(e, 1, ys, errMsg))
            return false;
        if (!xOk)
        {
            errMsg = xErr.isEmpty() ? QStringLiteral("X 侧无有效数据") : xErr;
            return false;
        }
        if (ys.isEmpty())
        {
            errMsg = QStringLiteral("Y 侧无有效数据");
            return false;
        }
        //按行位配对，行数取较少者截断（用户拍板）
        const int n = qMin(xs.size(), ys.size());
        if (n <= 0)
        {
            errMsg = QStringLiteral("无有效数据点");
            return false;
        }
        xData = xs.mid(0, n);
        yData = ys.mid(0, n);
        return true;
    }

    //混合来源复数图装配：x/yp/ym 三侧各按自身来源装配，按行位配对截断
    //(ym=上面板幅值、yp=下面板相位；yp/ym 同为数学时共用同一采样)
    bool mDefineCurveDlg::buildMixedPairData(const DefineCurveEntry& e, QVector<double>& xData,
        QVector<double>& yMag, QVector<double>& yPha, QString& errMsg) const
    {
        xData.clear();
        yMag.clear();
        yPha.clear();
        errMsg.clear();
        QVector<double> xs;
        QString xErr;
        const bool xOk = assembleSideData(e, 0, xs, xErr);
        QVector<double> ymV, ypV;
        if (sideSource(e, 2) == 1)
        {
            if (!evalMathSide(e, e.ymText, xs, ymV, errMsg))
                return false;
        }
        else if (!assembleSideData(e, 2, ymV, errMsg))
            return false;
        if (sideSource(e, 1) == 1)
        {
            QVector<double> samp = xs;   //与幅值侧同一采样
            if (!evalMathSide(e, e.ypText, samp, ypV, errMsg))
                return false;
        }
        else if (!assembleSideData(e, 1, ypV, errMsg))
            return false;
        if (!xOk)
        {
            errMsg = xErr.isEmpty() ? QStringLiteral("X 侧无有效数据") : xErr;
            return false;
        }
        if (ymV.isEmpty() || ypV.isEmpty())
        {
            errMsg = QStringLiteral("幅值/相位无有效数据");
            return false;
        }
        const int n = qMin(xs.size(), qMin(ymV.size(), ypV.size()));
        if (n <= 0)
        {
            errMsg = QStringLiteral("无有效数据点");
            return false;
        }
        xData = xs.mid(0, n);
        yMag = ymV.mid(0, n);
        yPha = ypV.mid(0, n);
        return true;
    }

    //切入"数值"（按侧）：把该侧切出前来源内容装配进该侧数值串，供微调。
    //调用时机约束（R20-B02）：须在 setSideSource 落新值之前调用——此时
    //sideSource 仍读旧来源，下方卫语句与 assembleSideData 分派才指向切出
    //侧（修前调用点在落值后，"old==2"恒真恒早退，倒表从未生效）。
    //该侧旧来源无内容（文件通道不全/数学表达式空）时保留已存数值串——数值
    //可能是该侧唯一内容，切走再切回不丢；有内容但装配不出才清空该侧串
    bool mDefineCurveDlg::dumpSideToValueText(DefineCurveEntry& e, int vector) const
    {
        const int old = sideSource(e, vector);
        if (old == 2)
            return true;   //数值→数值无装配语义
        if (!sideHasContent(e, vector))
            return true;   //无内容：保留已存数值串（"切出去看了眼再切回不丢"）
        QString& text = sideValueText(e, vector);
        try
        {
            QVector<double> v;
            QString err;
            if (assembleSideData(e, vector, v, err) && !v.isEmpty())
            {
                QStringList toks;
                for (int i = 0; i < v.size(); ++i)
                    toks << QString::number(v.at(i), 'f', 6);
                text = toks.join(QStringLiteral(", "));
                return true;
            }
        }
        catch (...)
        {
            //装配/求值异常按"该侧给不出数据"处理：清空
        }
        text.clear();
        return false;
    }

    // =============================================================================
    // 应用：列表条目整批写入目标图表
    // =============================================================================

    bool mDefineCurveDlg::applyEntries()
    {
        syncPanelToEntry();
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller->document();

        //目标图表仍有效（对话框存活期间可能被删除/撤销）
        if (!doc->node(_targetChartId) || doc->nodeType(_targetChartId) != CurveNodeType::Chart)
        {
            controller->reportWarning(QStringLiteral("[定义曲线] 目标图表已不存在"));
            return false;
        }
        if (_entries.isEmpty())
        {
            controller->reportWarning(
                QStringLiteral("[定义曲线] 请先点击\"新增\"创建曲线定义条目"));
            return false;
        }

        //轮换配色从目标图表既有曲线数续起（避免与已有曲线同色）
        const QVector<QColor> colorWheel = curveDefaultColorWheel();
        int colorIndex = 0;
        for (int childId : doc->childNodeIds(_targetChartId))
            if (doc->nodeType(childId) == CurveNodeType::Curve)
                ++colorIndex;

        int added = 0;
        int updated = 0;
        int skipped = 0;
        //编辑模式条目（绑定既有复数曲线）原位更新，其余新建；撤销宏名随主操作
        bool anyEdit = false;
        for (const DefineCurveEntry& e : _entries)
            anyEdit = anyEdit || e.editCurveId >= 0;
        controller->undoStack()->beginMacro(anyEdit
            ? QStringLiteral("定义曲线-修改曲线") : QStringLiteral("定义曲线-新增曲线"));
        for (int i = 0; i < _entries.size(); ++i)
        {
            DefineCurveEntry e = _entries.at(i);   //副本：复数对防呆可能对调 ym/yp
            QVector<double> xData;
            QVector<double> yData;
            QString errMsg;
            const QString entryLabel = e.useName && !e.name.trimmed().isEmpty()
                ? e.name.trimmed() : entryAutoName(e);

            //曲线名 = "曲线名称"框文本（定义时的名字，图例随名显示）；框为空时
            //回退自动名（单曲线/复数对共用）
            QString name = e.name.trimmed();
            if (name.isEmpty())
            {
                name = entryAutoName(e);
                if (name.trimmed().isEmpty())
                    name = QStringLiteral("Curve %1").arg(i + 1);
            }

			//复数图目标（文件/数学/数值，截图交互）：三来源均装配为单条复数曲线
			//（设计图 Complex Plot：树上单节点挂 x/ym/yp 三壳，引擎上下面板各一条
			//子曲线；不再按命名约定建两条曲线节点——旧”-MAG|分量”/”-PHA|分量”
			//成对建曲线的方式废弃）
			if (targetIsComplexChart())
			{
				//幅值/相位槽选反防呆（如默认 yp 单选先选了 MAG）：自动对调后再装配
				normalizeComplexPairChannels(e);
				//对调结果写回条目：应用后条目留存继续编辑，回显须与实际装配一致
				//（否则下次应用再次触发对调，回显来回翻转）；文件路径随通道/
				//回显一并写回（按侧独立，两侧可不同文件）
				_entries[i].ymChannel = e.ymChannel;
				_entries[i].ypChannel = e.ypChannel;
				_entries[i].ymText = e.ymText;
				_entries[i].ypText = e.ypText;
				_entries[i].ymFilePath = e.ymFilePath;
				_entries[i].ypFilePath = e.ypFilePath;
				QVector<double> xP, yM, yP;
				QString pairErr;
				bool okPair = false;
				try
				{
					//同源条目走既有装配（现状口径回归：全文件保留点数校验/显式
					//报错、全数学保留采样/求解口径、全数值保留 0 兜底）；仅真
					//混合来源按用户拍板按行位配对截断
					const int xsSrc = sideSource(e, 0);
					const int ypSrc = sideSource(e, 1);
					const int ymSrc = sideSource(e, 2);
					if (xsSrc == 0 && ypSrc == 0 && ymSrc == 0)
						okPair = buildFilePairData(e, xP, yM, yP, pairErr);
					else if (xsSrc == 1 && ypSrc == 1 && ymSrc == 1)
						okPair = buildMathPairData(e, xP, yM, yP, pairErr);
					else if (xsSrc == 2 && ypSrc == 2 && ymSrc == 2)
						okPair = buildValuePairData(e, xP, yM, yP, pairErr);
					else
						okPair = buildMixedPairData(e, xP, yM, yP, pairErr);
				}
				catch (const std::exception& ex)
				{
					pairErr = QString::fromUtf8(ex.what());
				}
				catch (...)
				{
					pairErr = QStringLiteral("未知异常");
				}
				if (!okPair || (yM.isEmpty() && yP.isEmpty()))
				{
					controller->reportWarning(QStringLiteral("[定义曲线] 条目“%1”跳过：%2")
						.arg(name).arg(pairErr.isEmpty() ? QStringLiteral("无有效数据") : pairErr));
					++skipped;
					continue;
				}
				//编辑模式（设计图）：条目绑定既有复数曲线时原位更新——数据/来源
				//直写（同"更改曲线"语义），经编辑曲线快照可撤回，不新建节点
				int curveId = -1;
				if (e.editCurveId >= 0)
				{
					if (doc->nodeType(e.editCurveId) != CurveNodeType::Curve)
					{
						controller->reportWarning(QStringLiteral("[定义曲线] 条目“%1”跳过：原曲线已不存在")
							.arg(name));
						++skipped;
						continue;
					}
					curveId = e.editCurveId;
					//X 为通道形态时新数据须与现存 X 点数成对（不过校验零副作用退出）
					if (!controller->checkSourcePairing(curveId, yM))
					{
						++skipped;
						continue;
					}
					mEditCurveDlg::getInstance()->snapshotCurve(curveId);
				}
				else
				{
					curveId = controller->addComplexCurve(_targetChartId, name);
					if (curveId < 0)
					{
						++skipped;
						continue;
					}
					//应用后条目留存继续编辑：新建条目绑定生成曲线，再次应用
					//原位更新，不重复新建
					_entries[i].editCurveId = curveId;
				}
				controller->setComplexSeriesData(curveId, xP, yM, yP);
				//来源落模按各侧来源（模型单来源字段跟随主侧幅值 ym；相位侧摘要
				//按其自身来源形态）。展开方式：幅值侧文件按 ym 分量令牌
				//（MAG/PHA=幅相，REAL/IMAG=实虚）；其余按界面幅相/实虚选择
				const int xsSrc = sideSource(e, 0);
				const int ypSrc = sideSource(e, 1);
				const int ymSrc = sideSource(e, 2);
				const int fmt = (ymSrc == 0)
					? (e.ymChannel.component.startsWith(QStringLiteral("MAG"), Qt::CaseInsensitive) ? 0 : 1)
					: (e.pairStyle == 1 ? 1 : 0);
				//b 侧（相位）摘要按其自身来源：文件=分量令牌，数学=表达式摘要，
				//数值=数值串摘要（与既有各来源落模形态一致）
				const QString bSummary = ypSrc == 0 ? e.ypChannel.component
					: (ypSrc == 1 ? e.ypText.left(60) : e.pairYpText.trimmed().left(60));
				if (ymSrc == 0)
				{
					//工况显示文本（模型溯源）：pch="Subcase {id}"（现状零变化）；
					//ascii 列数据集无工况级——落空串（不落假 "Subcase 1"，R22）。
					//判型与落模文件取 ym 侧按侧文件（sideFilePath(e,2)==ymFilePath）：
					//复数会话 e.filePath 仅"新增"时预置、按侧选文件不回写整条字段
					//（onFileComboChanged 非柱状只写按侧），判型/落模须按真实数据
					//所在文件——与 XY 分支 yFilePath 同口径（R22 评审修复；柱状分支
					//维持整条 filePath 现状）
					const QString mainPath = sideFilePath(e, 2);
					controller->setComplexSeriesSource(curveId, mainPath,
						isAsciiDataset(mainPath)
							? QString()
							: QStringLiteral("Subcase %1").arg(e.ymChannel.subcaseId),
						e.ymChannel.schema, e.ymChannel.objectText,
						e.ymChannel.component, e.ypChannel.component, fmt);
				}
				else if (ymSrc == 1)
				{
					//数学来源：来源回显取两侧内容摘要；表达式持久化到模型
					//ym 壳（幅值式）/yp 壳（相位式，仅相位侧为数学）/x 壳
					//（自变量引用，仅 X 侧为数学——混合来源的文件回显/数值串
					//不是有效引用）——设计图12 挂点
					controller->setComplexSeriesSource(curveId, kMathSourceTag,
						QString(), QString(), QString(),
						e.ymText.left(60), bSummary, fmt);
					const QString xRef = e.xText.trimmed();
					for (int childId : doc->childNodeIds(curveId))
					{
						if (doc->nodeType(childId) == CurveNodeType::CurveYData)
						{
							if (ymSrc == 1)
								controller->setNodeProperty(childId, CurveProp::kExpression, e.ymText);
						}
						else if (doc->nodeType(childId) == CurveNodeType::CurveY2Data)
						{
							if (ypSrc == 1)
								controller->setNodeProperty(childId, CurveProp::kExpression, e.ypText);
						}
						else if (doc->nodeType(childId) == CurveNodeType::CurveXData
							&& xsSrc == 1 && !xRef.isEmpty())
							controller->setNodeProperty(childId, CurveProp::kExpression, xRef);
					}
				}
				else
				{
					//数值来源（设计图）：来源回显取各侧数值摘要，无表达式
					//持久化；数据已在 setComplexSeriesData 写入
					controller->setComplexSeriesSource(curveId, kNumericSourceTag,
						QString(), QString(), QString(),
						e.pairYmText.trimmed().left(60), bSummary, fmt);
				}
				if (e.editCurveId >= 0)
				{
					//原位更新收尾：数据范围可能变化，既有隐藏区间失效清空（对齐
					//"更改曲线"换源）；曲线名随"曲线名称"框（勾选且改名才写，走
					//可撤销属性命令）；配色/线宽保持用户既有设置
					controller->setSeriesHiddenRanges(curveId, QVector<double>());
					if (e.useName && name != doc->nodeName(curveId))
						controller->setNodeProperty(curveId, CurveProp::kName, name);
					++updated;
					continue;
				}
				controller->setNodeProperty(curveId, CurveProp::kColor,
					colorWheel.at(colorIndex % colorWheel.size()));
				controller->setNodeProperty(curveId, CurveProp::kLineWidth, 2.0);
				++colorIndex;
				++added;
				continue;
			}

            bool ok = false;
            //来源分派：柱状图目标=整条 source 语义（现状零变化）；常规 XY
            //同源条目（全文件/全数学/全数值）走既有装配——全文件保留点数校验
            //与显式报错口径，仅真混合来源按行位配对、行数取较少者截断（用户拍板）
            const int xsSrc = sideSource(e, 0);
            const int ysSrc = sideSource(e, 1);
            try
            {
                if (targetIsBarChart())
                {
                    if (e.source == 0)
                        ok = buildFileData(e, xData, yData, errMsg);
                    else if (e.source == 1)
                        ok = buildMathData(e, xData, yData, errMsg);
                    else
                        ok = buildValueData(e, xData, yData, errMsg);
                }
                else if (xsSrc == 0 && ysSrc == 0)
                    ok = buildFileData(e, xData, yData, errMsg);
                else if (xsSrc == 1 && ysSrc == 1)
                    ok = buildMathData(e, xData, yData, errMsg);
                else if (xsSrc == 2 && ysSrc == 2)
                    ok = buildValueData(e, xData, yData, errMsg);
                else
                    ok = buildMixedXYData(e, xData, yData, errMsg);
            }
            catch (const std::exception& ex)
            {
                errMsg = QString::fromUtf8(ex.what());
                ok = false;
            }
            catch (...)
            {
                errMsg = QStringLiteral("未知异常");
                ok = false;
            }
            if (!ok)
            {
                controller->reportWarning(QStringLiteral("[定义曲线] 条目“%1”跳过：%2")
                    .arg(entryLabel.isEmpty() ? QStringLiteral("Curve %1").arg(i + 1) : entryLabel)
                    .arg(errMsg));
                ++skipped;
                continue;
            }

            //编辑模式（设计图）：条目绑定既有曲线时原位更新——数据/来源直写
            //（同"更改曲线"语义），经编辑曲线快照可撤回，不新建节点
            int curveId = -1;
            if (e.editCurveId >= 0)
            {
                if (doc->nodeType(e.editCurveId) != CurveNodeType::Curve)
                {
                    controller->reportWarning(QStringLiteral("[定义曲线] 条目“%1”跳过：原曲线已不存在")
                        .arg(name));
                    ++skipped;
                    continue;
                }
                curveId = e.editCurveId;
                //X 为通道形态时新数据须与现存 X 点数成对（不过校验零副作用退出）
                if (!controller->checkSourcePairing(curveId, yData))
                {
                    ++skipped;
                    continue;
                }
                mEditCurveDlg::getInstance()->snapshotCurve(curveId);
            }
            else
            {
                //柱状图目标走柱状曲线变体（仅 y 单壳：无横坐标数据语义，横轴为
                //类目轴），其余 x/y 两壳
                curveId = targetIsBarChart()
                    ? controller->addBarCurve(_targetChartId, name)
                    : controller->addCurve(_targetChartId, name);
                if (curveId < 0)
                {
                    ++skipped;
                    continue;
                }
                //应用后条目留存继续编辑：新建条目绑定生成曲线，再次应用原位
                //更新，不重复新建
                _entries[i].editCurveId = curveId;
            }
            controller->setSeriesData(curveId, xData, yData);
            //来源落模按主侧（Y）来源（模型单来源字段跟随主侧；柱状图=整条来源）
            const bool filePrimary = targetIsBarChart() ? e.source == 0 : ysSrc == 0;
            const bool mathPrimary = targetIsBarChart() ? e.source == 1 : ysSrc == 1;
            if (filePrimary)
            {
                //工况显示文本（模型溯源）：pch="Subcase {id}"（现状零变化）；
                //ascii 列数据集无工况级——落空串（回显统一入口 subcaseDisplayText
                //同空，标签模板 {MXSubcase} 替换为空），不落假 "Subcase 1"（R22）
                const QString mainPath = targetIsBarChart() ? e.filePath : e.yFilePath;
                controller->setSeriesSource(curveId, mainPath,
                    isAsciiDataset(mainPath)
                        ? QString()
                        : QStringLiteral("Subcase %1").arg(e.yChannel.subcaseId),
                    e.yChannel.schema, e.yChannel.objectText, e.yChannel.component);
                //R22-B01：X 侧来源=文件且 X 数据实际来自 ascii 列时补写 X 侧
                //来源四元组（x 数据页据此回显与改选，与"更改曲线"侧换源落模
                //同款）：X 侧通道已选=X 侧所选分量列（X 侧文件同为 ascii 列数据
                //集才落，跨类型混选保持未记录）；未选通道时 buildFileData 回退
                //主侧文件第 1 列，同口径取第 1 列。横坐标形态（pch）/数学/数值
                //来源不写（现状语义）；柱状目标仅 y 壳无 x 语义不写
                if (!targetIsBarChart() && xsSrc == 0 && isAsciiDataset(mainPath))
                {
                    QString xColumn;
                    if (e.xChannel.complete())
                    {
                        if (isAsciiDataset(sideFilePath(e, 0)))
                            xColumn = e.xChannel.component;
                    }
                    else
                    {
                        xColumn = mCurveDataSourceStore::getInstance()
                            ->asciiColumnNames(mainPath).value(0);
                    }
                    if (!xColumn.isEmpty())
                    {
                        controller->setCurveSideSource(curveId, true, QString(),
                            mCurveDataSourceStore::asciiSchemaDisplayText(),
                            mCurveDataSourceStore::asciiObjectDisplayText(), xColumn);
                    }
                }
            }
            else if (mathPrimary)
            {
                controller->setSeriesSource(curveId, kMathSourceTag,
                    QString(), QString(), QString(), e.expression.left(60));
                //表达式持久化到模型 y 侧 expression 字段（设计图12 挂点）；x 侧
                //仅 X 侧来源为数学时写引用——混合来源的文件回显/数值串不是
                //有效引用（柱状图整条语义：xRef 非空即写，现状零变化）
                const QString xRef = e.xText.trimmed();
                const bool xMath = targetIsBarChart() || xsSrc == 1;
                for (int childId : doc->childNodeIds(curveId))
                {
                    if (doc->nodeType(childId) == CurveNodeType::CurveYData)
                        controller->setNodeProperty(childId, CurveProp::kExpression, e.expression);
                    else if (doc->nodeType(childId) == CurveNodeType::CurveXData
                        && xMath && !xRef.isEmpty())
                        controller->setNodeProperty(childId, CurveProp::kExpression, xRef);
                }
            }
            else
            {
                controller->setSeriesSource(curveId, kNumericSourceTag,
                    QString(), QString(), QString(), e.valuesText.left(60));
                //刻度名（横坐标类别）入模：柱状图 X 轴按此渲染类别刻度
                static const QRegularExpression catSep(QStringLiteral("[\\s,;，；]+"));
                controller->setSeriesTickLabels(curveId,
                    e.categoriesText.split(catSep, QString::SkipEmptyParts));
            }
            if (e.editCurveId >= 0)
            {
                //原位更新收尾：数据范围可能变化，既有隐藏区间失效清空（对齐
                //"更改曲线"换源）；曲线名随"曲线名称"框（勾选且改名才写，走
                //可撤销属性命令）；配色/线宽保持用户既有设置
                controller->setSeriesHiddenRanges(curveId, QVector<double>());
                if (e.useName && name != doc->nodeName(curveId))
                    controller->setNodeProperty(curveId, CurveProp::kName, name);
                ++updated;
                continue;
            }
            controller->setNodeProperty(curveId, CurveProp::kColor,
                colorWheel.at(colorIndex % colorWheel.size()));
            controller->setNodeProperty(curveId, CurveProp::kLineWidth, 2.0);
            ++colorIndex;
            ++added;
        }
        controller->undoStack()->endMacro();

        if (added == 0 && updated == 0)
        {
            controller->reportWarning(QStringLiteral("[定义曲线] 未新增或修改任何曲线"));
            return false;
        }
        QString info;
        if (added == 0)
            info = QStringLiteral("[定义曲线] 已更新图表[%1]的 %2 条曲线")
                .arg(doc->nodeName(_targetChartId)).arg(updated);
        else
            info = QStringLiteral("[定义曲线] 已向图表[%1]新增 %2 条曲线")
                .arg(doc->nodeName(_targetChartId)).arg(added);
        if (added > 0 && updated > 0)
            info += QStringLiteral("（更新 %1 条）").arg(updated);
        if (skipped > 0)
            info += QStringLiteral("（跳过 %1 条）").arg(skipped);
        controller->reportInfo(info);
        return true;
    }

    void mDefineCurveDlg::onApply()
    {
        //数据装配异常兜底：报错不崩溃（对齐"从文件创建曲线"的创建入口）
        try
        {
            //应用后条目留存不清空，可一直修改再应用：新建条目已绑定生成曲线，
            //再次应用走原位更新，不重复新建
            applyEntries();
        }
        catch (const std::exception& e)
        {
            mCurveDocController::getInstance()->reportError(
                QStringLiteral("[定义曲线] 应用曲线定义失败：%1").arg(QString::fromUtf8(e.what())));
        }
        catch (...)
        {
            mCurveDocController::getInstance()->reportError(
                QStringLiteral("[定义曲线] 应用曲线定义失败：未知异常"));
        }
    }

    // =============================================================================
    // 生命周期联动
    // =============================================================================

    void mDefineCurveDlg::onImportFile()
    {
        //目录沿用已导入数据集所在目录，否则 C://（对齐"从文件创建曲线"）
        const QStringList paths = mCurveDataSourceStore::getInstance()->filePaths();
        const QString dir = paths.isEmpty()
            ? QStringLiteral("C://") : QFileInfo(paths.first()).absolutePath();
        const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("打开文件"),
            dir, QStringLiteral("数据文件(*.pch *.csv *.txt)"));
        if (filePath.isEmpty())
            return;

        mCurveDocController* controller = mCurveDocController::getInstance();
        //下拉已有的文件不再重复导入（现状幂等语义原样保留）：直接选中显示
        const QString key = mCurveDataSourceStore::normalizePath(filePath);
        const bool known = _fileCombo->findData(key) >= 0;
        if (known)
        {
            const int row = _fileCombo->findData(key);
            if (row >= 0)
                _fileCombo->setCurrentIndex(row);
            return;
        }

        //导入异步化（FR-01）：解析移后台线程，成功/失败续接移入
        //sig_importFinished 分支；已登记（其他入口先导入过、下拉未含）走原
        //幂等成功链路（C1）。
        //协调器须在 addFileAsync 之前就位（否则首个 sig_importStarted 无订阅方）
        mCurveImportCoordinator* coordinator = mCurveImportCoordinator::getInstance(window());
        auto* store = mCurveDataSourceStore::getInstance();
        const auto accepted = store->addFileAsync(filePath);
        if (accepted == mCurveDataSourceStore::AddFileAsyncResult::AlreadyRegistered)
        {
            controller->reportInfo(QStringLiteral("[定义曲线] 已导入数据集：%1")
                .arg(QFileInfo(filePath).fileName()));
            const int row = _fileCombo->findData(key);
            if (row >= 0)
                _fileCombo->setCurrentIndex(row);
            return;
        }

        //Accepted/DuplicateInFlight：解析在后台执行，成败经信号续接。接收上下文
        //=this（C3：解析中可关本对话框——D5，Qt 随对象销毁自动断连防悬垂）；
        //按键匹配防多导入串扰；命中后自断连
        if (accepted == mCurveDataSourceStore::AddFileAsyncResult::DuplicateInFlight)
            coordinator->notifyDuplicateInFlight(filePath);
        const auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(store, &mCurveDataSourceStore::sig_importFinished, this,
            [this, key, filePath, conn](const QString& finishedKey, bool ok, const QString& err)
            {
                if (finishedKey != key)
                    return;
                QObject::disconnect(*conn);
                mCurveDocController* controller = mCurveDocController::getInstance();
                if (!ok)
                {
                    controller->reportError(QStringLiteral("[定义曲线] 数据文件导入失败：%1").arg(err));
                    return;
                }
                //原成功续接链路（逐字保留）
                controller->reportInfo(QStringLiteral("[定义曲线] 已导入数据集：%1")
                    .arg(QFileInfo(filePath).fileName()));
                //选中该文件并联动级联列表（sig_fileAdded 已联动选中，此处幂等）
                const int row = _fileCombo->findData(key);
                if (row >= 0)
                    _fileCombo->setCurrentIndex(row);
            });
    }

    void mDefineCurveDlg::onFileAdded(const QString& filePath)
    {
        fillFileCombo();
        //自动选中新导入文件（联动填充级联列表；无编辑条目时仅刷新列表）
        const int row = _fileCombo->findData(filePath);
        if (row >= 0)
            _fileCombo->setCurrentIndex(row);
    }

    void mDefineCurveDlg::onNodeAboutToBeRemoved(int nodeId)
    {
        if (_targetChartId <= 0)
            return;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        //删的是目标图表自身或其祖先（如所在页）-> 退出定义曲线模式（结束信号
        //close 本窗口；此时父子链仍可查询，mApplyStyleDlg 先例）
        if (nodeId == _targetChartId)
        {
            mCurveViewportFactory::getInstance()->setDefineCurveMode(false);
            close();
            return;
        }
        for (int p = doc->parentNodeId(_targetChartId); p > 0; p = doc->parentNodeId(p))
        {
            if (p == nodeId)
            {
                mCurveViewportFactory::getInstance()->setDefineCurveMode(false);
                close();
                return;
            }
        }
    }

    //视窗 Shift+左键点曲线（视口工厂中继）-> 拾取激活时回填所点曲线的向量
    //引用：X 侧整体替换为横坐标引用 pNwMcK.x（单引用槽）；Y/yp/ym 侧在表达式
    //光标处插入纵坐标引用 pNwMcK.y（与构建器"直接插入"同语义）。引用按文档
    //p/w/c 全局编号，任一曲线视窗拾取皆可；拾取态保持（数学来源默认激活），
    //可连续点选多曲线
    void mDefineCurveDlg::onCurvePicked(int chartId, int curveNodeId)
    {
        Q_UNUSED(chartId);
        const int cur = currentEntryIndex();
        if (cur < 0 || _pickingVector < 0)
            return;
        //拾取目标侧须为数学来源（侧来源非数学时该框是回显，不接收引用）
        if (sideSource(_entries.at(cur), _pickingVector) != 1)
            return;
        const QString base = curveRefPrefixById(curveNodeId);
        if (base.isEmpty())
            return;
        DefineCurveEntry& e = _entries[cur];
        if (_pickingVector == 0)
        {
            //X：单引用槽，整体替换（照参考截图"对应选择的向量X将插入对应的内容"）
            e.xText = base + QStringLiteral(".x");
            _xEdit->setText(e.xText);
        }
        else
        {
            //Y/yp/ym：表达式槽，光标处插入引用（QLineEdit::insert 只发
            //textChanged，按目标槽手动同步条目）
            QLineEdit* edit = _pickingVector == 2 ? _ymEdit : _yEdit;
            edit->insert(base + QStringLiteral(".y"));
            if (targetIsComplexChart())
            {
                if (_pickingVector == 2)
                    e.ymText = edit->text();
                else
                    e.ypText = edit->text();
            }
            else
            {
                //XY/柱状图：Y 框与"表达式"框同源，走一次手动同步
                onYEdited(edit->text());
            }
        }
        refreshPreview();
        refreshActionStates();
        //拾取态保持（数学来源默认激活），可连续 Shift+左键点选多曲线
    }

    // =============================================================================
    // 标题栏拖动（照 mApplyStyleDlg）
    // =============================================================================

    bool mDefineCurveDlg::eventFilter(QObject* watched, QEvent* event)
    {
        //数学来源拾取默认激活（目标侧随激活向量行），点击向量输入框仅把拾取
        //目标侧切到该框——复数图三槽常可编辑，点哪行拾取落哪行；XY/柱状图
        //非激活侧禁用收不到事件，点击本身无切换效果。不消费事件（编辑框照常
        //获得焦点/光标）
        if (event->type() == QEvent::MouseButtonPress
            && currentEntryIndex() >= 0
            && sideSource(_entries.at(currentEntryIndex()),
                qBound(0, _entries.at(currentEntryIndex()).activeVector, 2)) == 1)
        {
            if (watched == _xEdit)
                setPickingVector(0);
            else if (watched == _yEdit)
                setPickingVector(1);
            else if (watched == _ymEdit)
                setPickingVector(2);
        }
        return QWidget::eventFilter(watched, event);
    }

    void mDefineCurveDlg::mousePressEvent(QMouseEvent* event)
    {
        //仅记录按下点：拖动延迟到 mouseMoveEvent 中移动超过系统拖动阈值后才
        //真正开始——双击的第二击经 mouseDoubleClickEvent 也不重置拖动状态，
        //双击的手部微动不再挪动弹窗
        if (event->button() == Qt::LeftButton && _titleBar->underMouse())
            _pressPos = event->globalPos();
        QWidget::mousePressEvent(event);
    }

    void mDefineCurveDlg::mouseMoveEvent(QMouseEvent* event)
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

    void mDefineCurveDlg::mouseReleaseEvent(QMouseEvent* event)
    {
        _dragging = false;   //松手结束拖动（双击的第二击不延续上一次拖动态）
        QWidget::mouseReleaseEvent(event);
    }

    void mDefineCurveDlg::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //双击不进入拖动链路：默认实现会转发 mousePressEvent 重新置位拖动状态，
        //配合手部微动会挪动弹窗；此处仅刷新阈值起算点（双击后按住继续移动即
        //从双击点起算正常拖动），定义曲线弹窗双击本身无其他语义
        if (event->button() == Qt::LeftButton)
            _pressPos = event->globalPos();
    }

    void mDefineCurveDlg::resizeEvent(QResizeEvent* event)
    {
        if (_grip)
            _grip->move(width() - _grip->width(), height() - _grip->height());
        QWidget::resizeEvent(event);
    }
}
