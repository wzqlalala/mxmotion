#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - "从文件新建三维图"对话框（功能弹窗，单例）
// 入口："3D曲线"Ribbon页签-文件组-新建按钮（mPostMenuContributor::build3DCurvePage，
// 参考图 design/3d-curve/ref-dialog-create-3dplot.png，样式不必对齐、布局大致相同）。
//
// 语义（R28 入树改造；T1~T4 直建视口卡片的独立链路已退役；R30 单视口化——
// "目标"行（新建/追加单选+目标下拉）整体移除，三维图全文档唯一）：
//   - 数据文件：txt/csv 单选（QFileDialog，过滤器"数据文件|所有文件"），路径只读
//     回显（过长省略显示，tooltip 带完整路径）；
//   - 绘图类型四单选：瀑布图/曲线（默认）/曲面/柱状图；
//   - 单视口定位（R30）：解析成功后查全文档是否已有三维图表（chartType=5，
//     遍历 工程->页->图表，不限定页）——无则新建宏（addPage→add3DChart→
//     add3DCurve，总是新建视图页）；有则"追加三维曲线"宏（仅 add3DCurve 追加
//     到该图，曲线名=文件基名）；用户删除三维图后再绘=重新新建；
//   - 数据解析走宿主侧 m3DPlotDataParser（复用 R22 mCurveAsciiReader，支持
//     逗号/空白点列与矩阵表；Plot3D MapFileLoader 仅作曲面/瀑布扁平网格兜底）：
//       曲线 = 点列 XYZ（矩阵表 → 红字提示不建）；
//       柱状图 = 点列（每行一根柱）或矩阵表（每格一根柱）；
//       曲面/瀑布图 = 矩阵表优先（行列数随数据落模），回退规则网格 MapData；
//   - 成功：经 mCurveDocController 命令栈写入文档（三维图表/曲线/x-y-z 轴壳，
//     一次撤销整组还原），数据经 set3DSeriesData 直写（与 pch 导入同口径）；
//     视口卡片由视口工厂按 nodeAdded 自动承接（m3DChartView 渲染）；随后关闭
//     弹窗。装配含无效单元按 0 兜底时照常入树，提示行灰字带计数说明且暂不关窗；
//   - 失败（空选/文件不存在/解析失败/非有效数据）：提示行红字带原因，不写文档。
//   - 渲染（Series 构建/轴量程/相机/色带）在 m3DChartView 重建时按文档执行，
//     本弹窗不再直建 Qt3DPlotWidget。
//
// 边界（本期不做）：单文件多条曲线、列映射选列、格式自动识别切换；三维图表
// 的"新建曲线"树入口（追加仅经本弹窗）；存量多三维图会话不合并（R30 用户
// 拍板：仅新逻辑保证新会话只产一个，追加落首个命中的三维图）。
//
// 形态照 mCurveStatisticsDlg/mApplyStyleDlg：无边框白卡片（M徽标标题栏+✕，可拖动），
// 非模态单例（showInstance 唤回 raise/activate），DlgGeom::bind 记忆大小/位置。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QString>
#include <QWidget>
#include <QPoint>
#include <QVector>

class QLabel;
class QLineEdit;
class QMouseEvent;
class QPushButton;
class QRadioButton;

namespace PostCurve
{
    //一次"绘图"装配出的三维数据（写文档前的中间载体）
    struct Plot3DRecord
    {
        int plot3dType = 1;              //0 瀑布/1 曲线/2 曲面/3 柱状（对齐弹窗单选序）
        QVector<double> xs;
        QVector<double> ys;
        QVector<double> zs;
        int rows = 0;                    //曲面/瀑布规则网格行列（rows*cols==点数；
        int cols = 0;                    //线/柱状=0，渲染退化为点序）
    };

    class POSTCURVE_EXPORT mCreate3DPlotDlg : public QWidget
    {
        Q_OBJECT
    public:
        //单例弹窗唤回（已打开则 raise/activateWindow 置前）
        static void showInstance(QWidget* parent = nullptr);

        mCreate3DPlotDlg(const mCreate3DPlotDlg&) = delete;
        mCreate3DPlotDlg& operator=(const mCreate3DPlotDlg&) = delete;

    private:
        explicit mCreate3DPlotDlg(QWidget* parent);
        ~mCreate3DPlotDlg() override;

        //--- UI 构建 ---
        void buildUi();

        //--- 绘图 ---
        //"浏览..."：选数据文件（txt/csv），路径回显（过长省略）；
        //取消选择不清空既有回显
        void slot_browseFile();
        //"绘图"：前置校验（路径/文件存在）-> 按类型解析装配 -> 单视口定位
        //（全文档查 chartType=5 图表：无则新建宏/有则追加宏，R30）->
        //命令栈写文档 -> set3DSeriesData 直写 -> 关闭弹窗；
        //任一步失败提示行红字带原因、不写文档
        void slot_plot();

        //按当前选择的类型解析装配三维数据；失败返回 false 并置 *err 带原因
        //（解析失败/非有效数据/异常）；装配含无效单元按 0 兜底等"成功但有提示"
        //场景置 _plotNote
        bool assemblePlotData(Plot3DRecord* rec, QString* err);

    protected:
        //标题栏按下/移动拖动窗口（超阈值进入拖动，双击不挪窗）
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private:
        static mCreate3DPlotDlg* _instance;   //单例

        bool _needCenter = true;        //无已存几何时首次打开居中于父窗口

        QWidget* _titleBar = nullptr;   //标题栏（拖动热区）
        QPoint _dragOffset;             //拖动起始偏移
        QPoint _pressPos;               //标题栏按下点（拖动阈值起算，防双击微动挪窗）
        bool _dragging = false;         //拖动中标志

        QLineEdit* _fileEdit = nullptr;      //数据文件路径回显（只读，过长省略显示）
        QString _filePath;                   //完整路径（回显省略时的真相源）
        QString _plotNote;                   //成功但有提示的说明（如无效单元按 0 计数；
                                             //slot_plot 入树后回显提示行且暂不关窗）
        QRadioButton* _waterfallRadio = nullptr;   //瀑布图（矩阵表优先，扁平网格兜底）
        QRadioButton* _lineRadio = nullptr;        //曲线（默认，点列）
        QRadioButton* _surfaceRadio = nullptr;     //曲面（矩阵表优先，扁平网格兜底）
        QRadioButton* _barRadio = nullptr;         //柱状图（点列每行一根柱/矩阵表每格一根柱）
        QLabel* _hintLabel = nullptr;        //提示行（默认灰字，错误红字）
        QPushButton* _plotBtn = nullptr;     //绘图（主蓝主钮）
    };
}
