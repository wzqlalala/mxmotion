#include "m3DChartLegendOverlay.h"

//模型只读消费（零模型改动）：图例节点 legend 数据 + 本图表曲线子节点收集
#include "../CurveCore/mCurveDocController.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/CurveDefine.h"

//QT
#include <QPainter>
#include <QFontMetrics>
#include <QPaintEvent>

namespace PostCurve
{
    m3DChartLegendOverlay::m3DChartLegendOverlay(int chartId, QWidget* parent)
        : QWidget(parent)
        , _chartId(chartId)
    {
        //鼠标穿透（拍板口径：仅显示不做画布交互）——图例区域下拖拽仍旋转视角，
        //编辑入口=树选 Legend 节点→参数栏（现状已通）
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        //背身透明不挡 GL 画布：子控件默认无自绘即透明，显式关系统背景防主题
        //强制刷底色盖住 GL 内容
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAutoFillBackground(false);
    }

    void m3DChartLegendOverlay::refreshLegend(const QRect& colormapLegendRect)
    {
        _colormapRect = colormapLegendRect;
        const LegendPlan plan = buildPlan();

        //无内容（图例节点缺失/无曲线条目）或模型 visible==false：整框隐藏
        //（含边框全隐藏）；脏键清零，重显时强制重绘
        if (!plan.valid || !plan.visible)
        {
            if (isVisible())
                hide();
            _lastKey.clear();
            return;
        }

        //脏键比对：画布每帧 Paint 均会触发本入口（相机旋转期色带矩形/图例
        //内容均不变），零变化免重绘；变化时 show+update（update 与实际绘制
        //间模型可能再变，paintEvent 届时现算，本键只作重绘调度判据）
        const QString key = planKey(plan);
        if (key == _lastKey && isVisible())
            return;
        _lastKey = key;
        show();
        update();
    }

    m3DChartLegendOverlay::LegendPlan m3DChartLegendOverlay::buildPlan() const
    {
        LegendPlan plan;
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller ? controller->document() : nullptr;
        CurveNode* chart = doc ? doc->node(_chartId) : nullptr;
        if (!chart || chart->type != CurveNodeType::Chart || chart->chart.chartType != 5)
            return plan;

        //图例节点 = 本图表子节点中 type==Legend（3D 固定骨架必含；缺失如
        //异常场景 → valid=false 覆盖层隐藏，不崩溃）
        const CurveLegendData* lg = nullptr;
        const QList<int> children = doc->childNodeIds(_chartId);
        for (int childId : children)
        {
            CurveNode* c = doc->node(childId);
            if (c && c->type == CurveNodeType::Legend)
            {
                lg = &c->legend;
                break;
            }
        }
        if (!lg)
            return plan;

        //条目 = 本图表渲染曲线（树序）——与 m3DChartView::rebuild 的收集
        //同一口径（series.visible 过滤，T8）：隐藏曲线不建系列也不进图例
        for (int childId : children)
        {
            CurveNode* c = doc->node(childId);
            if (!c || c->type != CurveNodeType::Curve || !c->series.visible)
                continue;
            LegendPlan::Entry e;
            e.iconColor = c->series.color;
            //图标线宽跟随本曲线模型线宽（R33：维持"与 3D 曲线渲染同值"语义；
            //渲染合理范围钳 1..4——与属性页线宽档位同口径，异常值不放大观感）
            e.iconLineWidth = qBound(1.0f,
                static_cast<float>(c->series.lineWidth), 4.0f);
            e.textColor = (lg->textColorMode == 0) ? c->series.color : lg->textColor;
            e.text = c->name;
            plan.entries.append(e);
        }
        if (plan.entries.isEmpty())
            return plan;   //无曲线不画框（对齐引擎 allSeries.isEmpty 早退）
        plan.valid = true;
        plan.visible = lg->visible;
        plan.iconBelowText = (lg->leaderPos == 1);
        plan.bgColor = lg->bgColor;   //含 alpha（默认白半透明必须保留）

        //文字字体 = lg.font 加粗（对齐 2D applyLegend 的 setBold(true)）
        plan.font = lg->font;
        plan.font.setBold(true);
        const QFontMetrics fm(plan.font);

        //布局常量与引擎 mPlotLegendItem 同值（观感一致）；引线在下方时每条目
        //多占一行图标区（线样行高 12 + 间距 4，引擎同式）
        plan.margin = 15;
        plan.padding = 10;
        plan.itemGap = 6;
        plan.iconWidth = 35;
        plan.textOffset = 8;
        plan.textHeight = fm.height();
        plan.iconRowH = plan.iconBelowText ? (12 + 4) : 0;
        plan.maxTextWidth = 0;
        for (const LegendPlan::Entry& e : plan.entries)
            plan.maxTextWidth = qMax(plan.maxTextWidth, fm.horizontalAdvance(e.text));

        //边框笔：线宽 0=无边框 NoPen；样式映射 1实线/2虚线/3划线(长虚线
        //CustomDashLine)/4点划线；borderWidth>0 且 borderStyle==0 按实线兜底
        //（对齐 2D applyLegend 的 qMax(1, borderStyle) 口径）
        const int effStyle = (lg->borderWidth > 0) ? qMax(1, lg->borderStyle) : 0;
        plan.borderPen = QPen(lg->borderColor, lg->borderWidth);
        switch (effStyle)
        {
        case 0: plan.borderPen.setStyle(Qt::NoPen); break;
        case 2: plan.borderPen.setStyle(Qt::DashLine); break;
        case 3: plan.borderPen.setStyle(Qt::CustomDashLine);
            plan.borderPen.setDashPattern({ 12.0, 4.0 }); break;
        case 4: plan.borderPen.setStyle(Qt::DashDotLine); break;
        default: plan.borderPen.setStyle(Qt::SolidLine); break;
        }

        //框尺寸 = 内容自适应（引擎同式：padding*2 + iconWidth + textOffset +
        //最宽条目；高按条目行高累加，引线在下方时逐条目加图标行）
        const int boxWidth = plan.padding * 2 + plan.iconWidth + plan.textOffset
            + plan.maxTextWidth;
        const int boxHeight = plan.padding * 2
            + plan.entries.size() * (plan.textHeight + plan.iconRowH)
            + (plan.entries.size() - 1) * plan.itemGap;

        //位置（现算）：autoPosition 或 图内右上(2)/图外四向(5~8，与 2D 同口径
        //映回右上) → 图内右上角；0/1/3 = 图内左上/左下/右下角；4 = 用户自定义
        const bool defaultTopRight = lg->autoPosition
            || lg->position == 2 || (lg->position >= 5 && lg->position <= 8);
        int x = 0, y = 0;
        if (defaultTopRight)
        {
            x = width() - boxWidth - plan.margin;
            y = plan.margin;
            //色带避让（仅默认右上路径，手动位置不避让）：Plot3D 色带默认贴
            //右上角（m_relPos=1.0,0.0），色带矩形无效=色带未画跳过；相交时
            //图例水平左移至色带左侧留约 8px，越界钳到左缘边距
            if (_colormapRect.isValid()
                && QRect(x, y, boxWidth, boxHeight).intersects(_colormapRect))
            {
                x = qMax(plan.margin, _colormapRect.left() - boxWidth - 8);
            }
        }
        else if (lg->position == 0)          //图内-左上
        {
            x = plan.margin;
            y = plan.margin;
        }
        else if (lg->position == 1)          //图内-左下
        {
            x = plan.margin;
            y = height() - boxHeight - plan.margin;
        }
        else if (lg->position == 3)          //图内-右下
        {
            x = width() - boxWidth - plan.margin;
            y = height() - boxHeight - plan.margin;
        }
        else                                 //4 图内-用户自定义
        {
            //posX/posY=画布百分比，框左上角锚点（语义对齐 2D setFloatingPos
            //(posX/100, posY/100)）；越界收敛对齐引擎（窗口变小时不被挤出）
            x = qRound(lg->posX / 100.0 * width());
            y = qRound(lg->posY / 100.0 * height());
            x = qBound(0, x, qMax(0, width() - boxWidth));
            y = qBound(0, y, qMax(0, height() - boxHeight));
        }
        plan.boxRect = QRect(x, y, boxWidth, boxHeight);
        return plan;
    }

    QString m3DChartLegendOverlay::planKey(const LegendPlan& plan) const
    {
        //脏键摘要：内容+样式+框矩形全量串接（rgba 取整避免 QColor::name() 丢
        //alpha；QFont::key() 含族名/字号/粗细/斜体等完整描述）
        QString k;
        k += plan.visible ? QLatin1Char('1') : QLatin1Char('0');
        k += QLatin1Char('|');
        k += QString::number(plan.boxRect.x());
        k += QLatin1Char(',');
        k += QString::number(plan.boxRect.y());
        k += QLatin1Char(',');
        k += QString::number(plan.boxRect.width());
        k += QLatin1Char(',');
        k += QString::number(plan.boxRect.height());
        k += QLatin1Char('|');
        k += plan.font.key();
        k += QLatin1Char('|');
        k += QString::number(plan.bgColor.rgba());
        k += QLatin1Char('|');
        k += QString::number(int(plan.borderPen.style()));
        k += QLatin1Char(',');
        k += QString::number(plan.borderPen.color().rgba());
        k += QLatin1Char(',');
        k += QString::number(plan.borderPen.widthF());
        k += QLatin1Char('|');
        k += plan.iconBelowText ? QLatin1Char('B') : QLatin1Char('L');
        k += QLatin1Char('|');
        for (const LegendPlan::Entry& e : plan.entries)
        {
            k += e.text;
            k += QString::number(e.iconColor.rgba());
            k += QString::number(e.iconLineWidth);   //线宽入脏键（R33：线宽提交后图标须重绘）
            k += QString::number(e.textColor.rgba());
            k += QLatin1Char(';');
        }
        return k;
    }

    void m3DChartLegendOverlay::paintEvent(QPaintEvent* event)
    {
        QWidget::paintEvent(event);
        //绘制现算：模型/尺寸/色带矩形即时重读（单一事实源=文档），本函数
        //不做缓存；无效计划直接空刷（背身透明，不挡 GL 画布）
        const LegendPlan plan = buildPlan();
        if (!plan.valid || !plan.visible)
            return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        //框：背景（含 alpha）+边框笔（样式映射见 buildPlan）
        p.setPen(plan.borderPen);
        p.setBrush(plan.bgColor);
        p.drawRect(plan.boxRect);

        //条目竖排：线样图标（曲线色短线段，线宽=本曲线模型线宽钳 1..4——
        //R33 起跟随模型，与 3D 曲线渲染同值语义，对齐 2D 引擎图标口径）
        //+ 曲线名（加粗，文本色两态见 buildPlan）
        int y = plan.boxRect.top() + plan.padding;
        for (const LegendPlan::Entry& e : plan.entries)
        {
            //图标中心线：引线在左=文字行垂直居中；引线在下=图标行单独一行
            //（文字行 + 4 间距 + 行内居中，引擎同式）
            const int iconCenterY = plan.iconBelowText
                ? (y + plan.textHeight + 4 + 6)
                : (y + plan.textHeight / 2);
            const int iconLeft = plan.boxRect.left() + plan.padding;
            const int iconRight = iconLeft + plan.iconWidth;

            QPen iconPen(e.iconColor, e.iconLineWidth);
            iconPen.setCapStyle(Qt::FlatCap);
            p.setPen(iconPen);
            p.drawLine(iconLeft, iconCenterY, iconRight, iconCenterY);

            p.setFont(plan.font);
            p.setPen(e.textColor);
            const QRect textRect(iconRight + plan.textOffset, y,
                plan.maxTextWidth, plan.textHeight);
            p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, e.text);

            y += plan.textHeight + plan.iconRowH + plan.itemGap;
        }
    }
}
