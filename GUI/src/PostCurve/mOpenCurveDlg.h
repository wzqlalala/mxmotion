#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>
#include <QHash>
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QMouseEvent>

#include "PostCurveGlobal.h"
#include "mDrawingOptions.h"

#include "NVHCurveWgt.h"
#include "mViewportModelManager.h"
#include <QMdiSubWindow>
#include "mImageGridComboBox.h"
#include "mPlotView.h"

namespace PchReader { struct PchFile; }

namespace PostCurve
{
    //点击空白处保持选中的列表控件：点击有效项走正常单选/多选，
    //点击空白拦截事件不清空选中（四级联动列表的交互保护）
    class KeepSelectionListWidget : public QListWidget {
        Q_OBJECT
    public:
        using QListWidget::QListWidget;

    protected:
        // 重写鼠标按下事件
        void mousePressEvent(QMouseEvent* event) override {
            // 判断点击位置是否有有效的列表项
            if (indexAt(event->pos()).isValid()) {
                // 如果点击了有效项，正常交给基类处理（实现单选/多选逻辑）
                QListWidget::mousePressEvent(event);
            }
            else {
                // 如果点击的是空白处，拦截事件，什么都不做，保持现有选中状态
                event->accept();
            }
        }
    };

    struct ListWgtStruct;   //数据集层级索引（定义在 mCurveDataSourceStore.h）

    // =============================================================================
    // 打开曲线对话框（从文件创建曲线，单例）
    //
    // 入口：Ribbon"曲线模块"页 -> 工具组"打开"。流程：
    //   选数据文件（R22 起 .pch / .csv / .txt，按 mCurveDataSourceStore::fileKind
    //   分流；解析登记统一入共享数据源仓库）-> 联动列表 ->（可选）mDrawingOptions
    //   绘图选项 -> "创建"（slot_createLine）按布局模式出图：
    //     0=一图多线 / 1=按对象分图 / 2=按分量分图 / 3=单线单图。
    //
    // 双数据源形态（R22）：
    //   - .pch：Y 轴四级联动（Subcase -> 结果类型 -> 对象 -> 分量），X 轴为
    //     结果类型/对象/分量 三列联动（结果类型 = 文件解析出的横坐标类型
    //     Time[Sec]/Frequency[Hz] 置首位默认选中 + 当前 Y 侧工况的 Y 结果类型，
    //     对象/分量联动规则与 Y 轴相同）。交互与赋值随 X 结果类型分流：选横坐标
    //     类型时对象/分量两栏隐藏，创建时 X 取记录横坐标；选实际结果类型时两栏
    //     显示，X 取该 对象×分量 通道数值（与 Y 侧装配同规则）。
    //   - .csv/.txt（ascii 列数据）：HyperGraph"Create Curves by File"三级列表
    //     ——工况列整列隐藏，结果类型=[Unknown]、对象=[Block 1]（均自动选中）、
    //     分量=全部列名（Y 侧多选，X 侧单选默认选中第 1 列）；X 轴数据源页同
    //     三级（ascii 无横坐标类型）。曲线名=列名（不带 "对象-" 前缀），来源
    //     五元组按 ascii 语义落模（路径/空工况文本/Unknown/Block 1/列名）。
    //
    // 已并入曲线模块框架：创建经 createChartFromData() 写入 CurveDocument
    // （beginMacro 整组命令：图表+曲线+数据+来源+样式），CurveTree 刷新与
    // 视口创建由框架自动联动，渲染走 mCurveChartView（PlotView）。
    //
    // "绘图选项"子弹窗（mDrawingOptions）：页眉页脚/轴标题/线色线型等，
    // 关键字段映射到模型属性（页眉开关、X/Y轴标题、统一线色）。
    // =============================================================================
    class POSTCURVE_EXPORT mOpenCurveDlg : public QDialog
    {
        Q_OBJECT
    public:
        static mOpenCurveDlg* getInstance(QWidget* parent = nullptr);

        //单例弹窗唤回（自 dev_base mMenuWidget::slot_NVHCurve 合入）：非模态
        //单例必须挂主窗口为父（否则会被主窗口盖住），且对已可见窗口 show()
        //不改变 Z 序，弹窗落到软件后面后重复点击需 raise/activate 才能唤回
        static void showInstance(QWidget* parent = nullptr);

        mOpenCurveDlg(const mOpenCurveDlg&) = delete;
        mOpenCurveDlg& operator=(const mOpenCurveDlg&) = delete;
    private:
        explicit mOpenCurveDlg(QWidget* parent = nullptr);
        static mOpenCurveDlg* m_instance;

        void clearAllList();

        void ParseFile(QString path);

        //当前文件的解析数据/层级索引（经 mCurveDataSourceStore 共享缓存读取；
        //当前文件未导入时返回空数据，行为与原先 QHash 缺省构造一致）
        const PchReader::PchFile* currentFile() const;
        const ListWgtStruct& currentIndexData() const;

        void filterList(QListWidget* list, const QString& text);

        //绘图按钮可用性刷新：X/Y 两侧选择条件齐备才启用（与 slot_createLine
        //的前置校验同源），随各列表 itemSelectionChanged 实时更新
        void updateCreateButtonState();

        void setListWig();

        //当前文件是否 ascii 列数据（csv/txt，经 mCurveDataSourceStore::fileKind）：
        //联动列呈现（隐藏工况级/固定 Unknown+Block 1/分量=列名）与创建装配按此分流
        bool isAsciiCurrentFile() const;

        //ascii 文件的列表填充（Y 三级 = Unknown/Block 1/全部列名，X 三级同款且
        //分量默认选中第 1 列）：工况列整列隐藏，各级自动选中后由既有联动级联填充
        void setAsciiListWig();

        //X 轴结果类型是否为文件解析出的横坐标类型（xresulttype 集合内，
        //Time[Sec]/Frequency[Hz] 随文件数据形态解析；此类无对象/分量）
        bool isAbscissaXResult(const QString& schema) const;

        //X轴页面联动（对象/分量联动规则与 Y 轴相同）：
        //结果类型 = 当前 Y 侧工况的 Y 结果类型 + 文件解析出的横坐标类型
        void fillXResults();
        void fillXObjects();      //对象列表 <- 当前X结果类型（横坐标类型无对象/分量）
        void fillXComponents();   //分量列表 <- 当前X对象（分量展开规则同 Y 轴）

        int getDataIndex(QString text);

        //pch数据并入曲线模块框架：一次"创建"=一页（工程->页->视口->曲线），
        //在指定页下按数据创建视口+曲线节点（数据/样式/来源信息入模型），
        //CurveTree刷新与视口创建由框架自动联动（视口工厂监听节点信号）。
        //chartType=新建图表承接的目标类型（1 柱状图/2 复数图，0 xy），
        //经 kChartType 命令下发（自带骨架协调：复数补 Phase 副轴、默认名随类型）
        void createChartFromData(int pageId,
            const QHash<QString, QVector<double>>& XHash,
            const QHash<QString, QVector<double>>& YHash,
            int subcaseid, const QString& schema, const QString& xResult,
            int chartType = 0,
            //R22-B01：ascii 且 X 按所选分量列装配成功时=X 页所选列名（透传
            //appendCurvesToChart 补写 X 侧来源四元组）；空=不写（pch 横坐标
            //形态/装配失败退行号保持未记录）
            const QString& xComponent = QString());

        //收集所选对象×分量的数据并归入一表（feature = "对象-分量"），
        //四种布局共用的唯一数据装配路径（分量校验/缺虚部防线告警跳过均在此，
        //瞬态实数块不产出 IMAG/PHA 假数据）。X 轴来源由 xSchema 指定：
        //空或文件解析出的横坐标类型（Time[Sec]/Frequency[Hz]）时 X 取记录
        //横坐标；为文件实际结果类型时 X 取 xObjectId×xComponent 通道数值
        //（规则同 Y 侧），与各 feature 的 Y 数据点数一致才配对成曲线，
        //不一致告警跳过。targetChartType=本批目标图表类型（1 柱状图/2 复数图）：
        //复数图目标按方向合并分量展开为 MAG/PHA 两侧特征——批次前快照传参，
        //不实时查当前视口（批次中途激活链切到新建图表会翻转装配状态）。
        //返回 X 是否按所选分量列装配成功（横坐标形态/通道装配失败退行号均
        //false）——创建侧据此决定是否补写 X 侧来源四元组（R22-B01）
        bool buildSeriesHash(const QList<QListWidgetItem*>& objects,
            const QList<QListWidgetItem*>& components, int subcaseid, const QString& schema,
            QHash<QString, QVector<double>>& XHash, QHash<QString, QVector<double>>& YHash,
            const QString& xSchema = QString(), int xObjectId = 0,
            const QString& xComponent = QString(), int targetChartType = 0);

        //向既有图表节点追加曲线（"使用当前绘图"命中当前视口已承载本模块图表时）：
        //只添加曲线节点（数据/样式/来源），不新建页/视口，不覆盖建图时的页眉/轴设置；
        //独立撤销宏，撤销只撤本批新增曲线。xComponent=R22-B01 X 侧来源补写参数：
        //ascii 且 X 按所选分量列装配成功时为 X 页所选列名（创建后补写 X 侧来源
        //四元组供 x 数据页回显/改选）；空=不写（pch 横坐标形态/退行号保持未记录，
        //柱状目标无 x 壳亦不写）
        void appendCurvesToChart(int chartId,
            const QHash<QString, QVector<double>>& XHash,
            const QHash<QString, QVector<double>>& YHash,
            int subcaseid, const QString& schema,
            const QString& xComponent = QString());

        //当前绘图目标为复数图（图表类型2）：Y 分量列表按方向合并显示
        //（MAG|X-Trans/PHA|X-Trans -> X-Trans），绘制时一次装配两侧各建一条
        //曲线，复数图渲染按命名约定配对成上下面板
        bool drawIntoComplexChart() const;

        //重填 Y 分量列表（级联选择变化/图表类型切换/弹窗显示共用）：
        //复数图目标按方向合并显示
        void refreshYComponents();
    private slots:
        void slot_drawOptions();

        void slot_createLine();

        void slot_openFile();

    protected:
        void closeEvent(QCloseEvent* event) override;
        void showEvent(QShowEvent* event) override;
        //标题栏拖动（无边框窗口，照 mApplyStyleDlg/定义曲线弹窗的标题栏拖动）：
        //按下记录偏移开始拖动，移动跟随，松开结束
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        QWidget* _titleBar = nullptr;   //标题栏（拖动热区，事件过滤在 eventFilter）
        bool _dragging = false;         //拖动中标志
        QPoint _dragOffset;             //拖动偏移（按下点到窗口左上角）
        bool _needCenter = true;        //无已存几何时首次打开居中于父窗口

        // 数据源
        QComboBox* m_fileCombo;
        // X轴页面（结果类型/对象/分量 三列联动）
        QLabel* xLabel_1;
        QLabel* xLabel_2;
        QLabel* xLabel_3;
        QListWidget* m_xResultList;
        QWidget* m_xObjCompPanel;               //对象/分量两栏容器：横坐标类型时隐藏
        KeepSelectionListWidget* m_xObjectList;
        KeepSelectionListWidget* m_xComponentList;
        // Y轴页面
        QLabel* yLabel_1;
        QLabel* yLabel_2;
        QLabel* yLabel_3;
        QLabel* yLabel_4;
        QComboBox* m_layoutCombo;

        MechMainWindow::mImageGridComboBox* m_selectBtn; //

        QListWidget* m_ySubcaseList;
        QListWidget* m_yResultList;

        //Y 侧"工况"整列容器（列表+搜索+栏头）：R22 起 ascii 文件（csv/txt）无
        //工况层级，整列隐藏用；pch 文件保持显示（构造时包住原 col1 布局）
        QWidget* _ySubcaseCol = nullptr;

        KeepSelectionListWidget* m_yObjectList;
        KeepSelectionListWidget* m_yComponentList;

        mDrawingOptions* drawOption;

        QPushButton* m_createBtn;   //绘图按钮：选择条件齐备才启用（updateCreateButtonState）

        QString _currentlyFilePath;//当前选择文件的文件路径（解析数据在 mCurveDataSourceStore 共享缓存）

        //批量建图进行中（slot_createLine）：期间新图表的 kChartType 下发会触发
        //sig_nodePropertyChanged，本弹窗的分量列表重填（refreshYComponents 会
        //clear 列表）若照常执行，会把调用方持有的 QListWidgetItem 选集变成悬空
        //指针，故批内抑制
        bool _creatingCharts = false;
    };
}