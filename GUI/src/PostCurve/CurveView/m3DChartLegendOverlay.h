#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图图例覆盖层（CurveView，chartType=5，R32）
//
// 宿主侧自绘覆盖层：QWidget 子控件叠于 Qt3DPlotWidget 之上，paintEvent 自绘
// 三维图图例（不动 Plot3D 引擎）。绘制口径逐项对齐 2D mCurveChartView::
// applyLegend 下发的引擎渲染（PlotView mPlotLegendItem）：
//   - 条目竖排 = 线样图标（该曲线 series.color 短线段，线宽=本曲线模型线宽
//     series.lineWidth 钳 1..4——R33 起跟随模型，维持"与 3D 曲线渲染同值"
//     语义；渲染端 setLineWidth 消费模型值同源）+ 曲线名（节点本名，3D 无曲
//     线标签系统不用 legendText），
//     文本字体 = lg.font 加粗（对齐 2D applyLegend 的 setBold(true) 口径）；
//   - 文本色两态：textColorMode==0 每条目跟随该曲线颜色 / 1 统一 lg.textColor；
//   - 边框四样式映射（0无 NoPen / 1实线 / 2虚线 / 3划线 CustomDashLine 长
//     虚线 / 4点划线；borderWidth>0 且 borderStyle==0 按 1 实线兜底，对齐
//     2D qMax(1, borderStyle) 口径），线宽 = lg.borderWidth（px）；
//   - 背景 = lg.bgColor（默认白半透明 QColor(255,255,255,200)，alpha 必须保留）；
//   - 引线位置 leaderPos：0 图标在文字左侧（默认）/ 1 图标在文字下方
//     （2D setIconBelowText 同语义，图标行单独占一行）。
// 布局常量（margin=15/padding=10/itemGap=6/iconWidth=35/textOffset=8/
// iconRowH=16）与引擎 mPlotLegendItem 同值，观感一致。
// 位置（paintEvent 内经 buildPlan 现算）：
//   - autoPosition==true 或 position==2（图内右上）/5~8（图外四向与 2D 同
//     口径映回右上）→ 图内右上角（留边距）；引擎色带默认贴右上角
//     （Colormap m_relPos=1.0,0.0），色带矩形（宿主传入的
//     Colormap::getLegendRect，无效=色带未画跳过）与图例矩形相交时图例
//     水平左移至色带左侧留约 8px；手动指定位置（0/1/3/4）不避让；
//   - position 0/1/3 = 图内左上/左下/右下角；
//   - position==4 用户自定义：lg.posX/lg.posY 为画布百分比（0-100），框左上
//     角锚点（语义对齐 2D setFloatingPos(posX/100, posY/100)），越界收敛。
// 交互：WA_TransparentForMouseEvents 鼠标穿透——图例区域下拖拽仍旋转视角
//（拍板口径：仅显示不做画布交互，编辑入口=树选 Legend 节点→参数栏，现状已通）；
// 背身透明不挡 GL 画布。
// 数据：只读 CurveLegendData 与本图表曲线子节点（零模型改动）；条目集合与
// m3DChartView::rebuild 的渲染曲线收集同一口径（树序，series.visible 过滤，
// T8 接通后隐藏曲线不建系列也不进图例——所见即所得）。
// 刷新：refreshLegend(colormapLegendRect) 为唯一刷新入口（rebuild 尾统一
// 重放 / 图例专属键文档信号 / 画布每帧 Paint 同步三路汇聚），内部脏键比对
// 零变化免重绘——图例属性提交只刷本覆盖层，绝不触发 GL 场景重建（相机状态
// 不丢、画布不闪）。图例节点缺失（异常场景）或无条目时整框隐藏不崩溃。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QWidget>
#include <QRect>
#include <QPen>
#include <QFont>
#include <QColor>
#include <QString>
#include <QList>

namespace PostCurve
{
    class POSTCURVE_EXPORT m3DChartLegendOverlay : public QWidget
    {
        Q_OBJECT
    public:
        explicit m3DChartLegendOverlay(int chartId, QWidget* parent = nullptr);

        //唯一刷新入口：重读文档现算绘制计划并按需重绘。colormapLegendRect =
        //引擎色带矩形（Qt3DPlotWidget::getColormap()->getLegendRect()，无效=
        //色带未画）由宿主传入，本层不依赖 Plot3D 头（宿主→覆盖层单向传矩形）
        void refreshLegend(const QRect& colormapLegendRect = QRect());

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        //绘制计划：模型+当前尺寸+色带矩形现算的全部绘制参数（paintEvent 消费；
        //refreshLegend 以其摘要做脏键比对，零变化不重绘）
        struct LegendPlan
        {
            bool valid = false;        //图例节点存在且有条目（false=无内容可画）
            bool visible = false;      //模型 lg.visible
            QRect boxRect;             //图例框（定位/色带避让/越界收敛后）
            struct Entry
            {
                QColor iconColor;      //线样图标色 = series.color
                float iconLineWidth = 1.5f; //线样图标线宽 = series.lineWidth（钳 1..4，R33 起跟随模型）
                QColor textColor;      //文本色（跟随曲线色/统一自定义两态）
                QString text;          //曲线名（节点本名）
            };
            QList<Entry> entries;
            QFont font;                //lg.font + setBold(true)
            QColor bgColor;            //含 alpha
            QPen borderPen;            //边框四样式映射（borderWidth=0 → NoPen）
            bool iconBelowText = false;
            //布局常量与度量（引擎 mPlotLegendItem 同值/同算法）
            int margin = 0;
            int padding = 0;
            int itemGap = 0;
            int iconWidth = 0;
            int textOffset = 0;
            int textHeight = 0;
            int iconRowH = 0;
            int maxTextWidth = 0;
        };

        LegendPlan buildPlan() const;
        //脏键摘要：全部影响绘制的输入（可见性/条目/样式/字体/框矩形）串接，
        //零变化时 refreshLegend 免重绘（画布每帧 Paint 均会触发刷新入口）
        QString planKey(const LegendPlan& plan) const;

        int _chartId = -1;        //绑定的三维图节点id（与宿主 m3DChartView 一致）
        QRect _colormapRect;      //最近一次刷新传入的引擎色带矩形（无效=未画）
        QString _lastKey;         //脏键（空=从未绘制/已隐藏，重显强制重绘）
    };
}
