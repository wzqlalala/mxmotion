#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 曲线统计对话框（功能弹窗，单例）
// 入口：曲线页-曲线组-曲线统计按钮（mPostMenuContributor::buildNVHCurvePage，
// 设计图：曲线统计弹窗 + design/curve-statistics/ 导出表格设计稿）。
// 语义（超限频段统计，20261029 需求，替换旧整段特征值表）：
//   - 曲线形式：单选 动刚度（默认）/VTF/NTF——统计对象即该类别**实测曲线**
//     （CurveSeriesData::curveCategory 标记，由动刚度/VTF/NTF 曲线生成功能写入；
//     目标值曲线为 TargetCurve 标记，同图表内与实测配对；普通导入/定义曲线与
//     升级前生成的旧曲线（无标记）不在统计范围）。文档内无对应类别曲线时
//     提示后不导出。
//   - 超限方向自实测曲线来源结果类型（series.schema）判定：动刚度+DISPLACEMENTS
//     （位移形式）低于目标为超限、段内取最小值；其余（速度/加速度/VTF/NTF）
//     高于目标为超限、段内取最大值。schema 缺失/不可识别的曲线跳过并在导出
//     提示中说明。
//   - 频率范围：两输入框（min — max），留空侧=不限制；闭区间内且有限、非
//     "编辑曲线"隐藏区间的实测点才参与；目标值按实测点频率在目标曲线上线性
//     插值（目标参与采样同样剔除隐藏/非有限点，频率范围外跳过）。连续超限
//     采样点合并为一个超限频段（单点自成段），每段输出 (f1-f2)-(f极值@极值)
//     单元格（数值 'g'6）。
//   - 导出布局（严格还原图面排版）：动刚度=行式（设计稿01-03，第1行=工况角标+
//     合并标题"xxHz动刚度统计结果"，数据行=工况列+逐段单元格，无列头行/汇总列）；
//     VTF/NTF=矩阵（设计稿04-05，无标题行，第1行=工况角标+对象号分组合并表头，
//     组宽=该组各行最大频段数，频段按组内子列排布，无汇总列）。工况列的子工况
//     名尾部 pch 激励描述括号段剥离（normalizeSubcase，贴图面样例）。xlsx 全表
//     带框样式+列宽（工况 45/频段 24），导出临时脚本每次覆盖释放防旧副本续用。
//   - 保存：QFileDialog 选保存路径/名称/格式（excel=.xlsx / ppt=.pptx），
//     统计上下文写临时 JSON 后 QProcess 调程序目录自带 python 执行
//     scripts/export_stats.py（纯标准库手写 OOXML，qrc 释放到临时目录，
//     与报告导出 export_pptx.py 同一机制）生成文件。
//
// 风格照 mApplyStyleDlg/mOpenCurveDlg：无边框白卡片 + M徽标标题栏 + ✕，
// 非模态单例（showInstance 唤回 raise/activate）、可拖动、首开居中于主窗口。
// =============================================================================

#include "PostCurveGlobal.h"
#include "CurveCore/CurveDefine.h"
#include "CurveStatsBand.h"

//QT
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QVector>

class QLineEdit;
class QMouseEvent;
class QPushButton;
class QRadioButton;

namespace PostCurve
{
    //统计导出上下文（宿主组装 -> export_stats.py 渲染，JSON 字段一一对应；
    //见任务单 docs/tasks/20261029-01B 与 design/curve-statistics/ 设计稿）
    struct StatsExportTable
    {
        QString title;               //标题（行式："50-1000Hz动刚度统计结果"；矩阵：空=无标题行）
        QString layout;              //"rows"（动刚度行式）/ "matrix"（VTF/NTF 矩阵）
        QString corner;              //表左上角标（两布局首列，恒"工况"）
        QStringList groups;          //矩阵列头分组标签（对象号尾 token；行式为空）
        QList<int> spans;            //矩阵各组跨列数（=组内各行最大频段数，≥1；行式为空）
        QVector<QStringList> rows;   //数据行（自第 2 行起与表格等宽；数值形字符串由脚本按数值写出）
        QString note;                //导出成功提示附注（跳过统计的实测曲线说明/频段统计跳过原因；空=无）
        //频段统计表上下文（第二张表，add-curve-stats-band-count）：generated=false
        //时导出 JSON 不写 band_stats 键，脚本不加 Sheet/页（缺席即跳过契约）
        CurveStatsBand::BandStatsResult bandStats;
    };

    class POSTCURVE_EXPORT mCurveStatisticsDlg : public QWidget
    {
        Q_OBJECT
    public:
        //单例弹窗唤回（已打开则 raise/activateWindow 置前）
        static void showInstance(QWidget* parent = nullptr);

        mCurveStatisticsDlg(const mCurveStatisticsDlg&) = delete;
        mCurveStatisticsDlg& operator=(const mCurveStatisticsDlg&) = delete;

    private:
        explicit mCurveStatisticsDlg(QWidget* parent);
        ~mCurveStatisticsDlg() override;

        //--- UI 构建 ---
        void buildUi();

        //--- 统计与导出 ---
        //频率范围两输入框解析为 [min, max]（留空侧不限制）；非法（非数字/
        //min>max）时弹提示并返回 false，*hasAny 带出"至少一侧有限定"
        bool parseFreqRange(double* freqMin, double* freqMax, bool* hasAny);
        //遍历当前文档全部页->图表，按"曲线形式"所选类别收集实测曲线与同图
        //目标值曲线（TargetCurve）的超限频段统计并组装导出表（行式/矩阵两种
        //布局，语义见类头注释）。返回是否收集到可统计数据
        //（无数据时弹提示不导出；跳过明细记入 ctx->note）
        bool collectStatistics(StatsExportTable* ctx);
        //写统计上下文 JSON 到临时目录并调 python 脚本生成文件；
        //失败经 err 带原因（python 缺失/脚本报错/文件未生成）
        bool exportStatistics(const QString& outputPath,
            const StatsExportTable& ctx, QString* err);

    protected:
        //标题栏按下/移动拖动窗口（超阈值进入拖动，双击不挪窗）
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private slots:
        void onSave();

    private:
        static mCurveStatisticsDlg* _instance;   //单例

        bool _needCenter = true;        //无已存几何时首次打开居中于父窗口

        QWidget* _titleBar = nullptr;   //标题栏（拖动热区）
        QPoint _dragOffset;             //拖动起始偏移
        QPoint _pressPos;               //标题栏按下点（拖动阈值起算，防双击微动挪窗）
        bool _dragging = false;         //拖动中标志

        QRadioButton* _stiffnessRadio = nullptr;   //动刚度（默认选中）
        QRadioButton* _vtfRadio = nullptr;         //VTF
        QRadioButton* _ntfRadio = nullptr;         //NTF
        QLineEdit* _freqMinEdit = nullptr;         //频率范围下限（空=不限制）
        QLineEdit* _freqMaxEdit = nullptr;         //频率范围上限（空=不限制）
    };
}
