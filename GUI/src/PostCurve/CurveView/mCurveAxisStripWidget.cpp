#include "mCurveAxisStripWidget.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "mCurveChartView.h"
#include "mCurveAxisTickEngine.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QFontMetrics>
#include <QtGlobal>

#include <qwt_plot.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_div.h>
#include <qwt_scale_widget.h>
#include <qwt_text.h>
#include <cmath>

namespace PostCurve
{
	namespace
	{
		//条带内侧留白（基线与绘图区方向的间隙）与标题区间距
		const int kStripInnerPad = 2;
		const int kStripTitleSpacing = 4;

		//选中态标题色（与曲线选中/主轴标题激活同一主题蓝）
		const QColor kStripSelectedColor(0, 102, 204);
	}

	mCurveAxisStripWidget::mCurveAxisStripWidget(bool top, QWidget* parent)
		: QWidget(parent)
		, _top(top)
	{
		//悬停高亮需要鼠标跟踪；整体不抢键盘焦点（对齐轴控件 ClickFocus 语义）
		setMouseTracking(true);
		setAttribute(Qt::WA_NoSystemBackground);
		setFocusPolicy(Qt::ClickFocus);
	}

	void mCurveAxisStripWidget::setAxis(int axisNodeId, QwtPlot* qwt)
	{
		_axisNodeId = axisNodeId;
		_qwt = qwt;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(axisNodeId);
		if (!n || (n->type != CurveNodeType::XAxis2 && n->type != CurveNodeType::YAxis2))
		{
			_thickness = 0;
			_title.clear();
			if (_top)
				setFixedHeight(0);
			return;
		}
	const CurveAxisData& a = n->axis;
	//轴隐藏（"可见性"关）= 整条条带不显示：清厚度并隐藏控件（右侧条带为视图
	//外扩边距按 thickness 排布，隐藏后边距随之收起；重新显示随属性变化重建恢复）
	if (!a.visible)
	{
		_title.clear();
		_thickness = 0;
		if (_top)
			setFixedHeight(0);
		hide();
		update();
		return;
	}
	_title = a.titleVisible ? a.title : QString();
		_titleFont = a.titleFont;
		_labelFont = a.labelFont;
		_titleColor = a.titleColor;
		//背景与图表边距区同色（引擎 setMarginBackground 同源取图表背景色）；
		//条带可能落在视图外扩边距的窗口灰底区（右侧条带），须自绘填充不透底
		if (CurveNode* chartNode = doc->node(doc->parentNodeId(axisNodeId)))
			_bgColor = chartNode->chart.backgroundColor;
		if (!_bgColor.isValid())
			_bgColor = QColor(Qt::white);
		rebuildScale(qwt);
		//顶部条带由引擎内部布局排布，厚度即占位高度（右侧条带为手动几何不设限）
		if (_top)
			setFixedHeight(_thickness);
		update();
	}

	void mCurveAxisStripWidget::rebuildScale(QwtPlot* qwt)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(_axisNodeId);
		if (!n)
			return;
		const CurveAxisData& a = n->axis;

		//范围：手动=模型原始值按刻度模式变换（与主轴同策略；标量界值无频率
		//上下文，不做计权修正）；自动=镜像同侧引擎轴
		//（首枚副轴）当前范围——含 fit 与用户缩放后的最新值，保持各轴刻度可读
		double lo = 0.0, hi = 10.0;
		const QwtPlot::Axis mirrorAxis = _top ? QwtPlot::xTop : QwtPlot::yRight;
		if (!a.autoScale)
		{
			lo = mCurveChartView::transformValue(a.min, a.scaleMode, a.dbRef);
			hi = mCurveChartView::transformValue(a.max, a.scaleMode, a.dbRef);
		}
		else if (qwt && qwt->axisEnabled(mirrorAxis))
		{
			const QwtScaleDiv& div = qwt->axisScaleDiv(mirrorAxis);
			lo = div.lowerBound();
			hi = div.upperBound();
		}
		if (qIsNaN(lo) || qIsNaN(hi) || !(lo < hi))
		{
			lo = 0.0;
			hi = 10.0;
		}

		//刻度数字格式/精度与主轴同一模型键（kLabelFormat/kPrecision）
		_scaleDraw.setNumberFormat(a.labelFormat, a.precision);

		//刻度：与主轴 applyAxis 同一套生成方式分派（值=面板下拉索引）——
		//均匀分步法按"刻度总数"等分范围（主刻度数字个数=刻度总数，含两端；
		//log/dB 下即刻度变换空间），并按"每主刻度网格数"细分次刻度；固定步
		//长法步长>0 时下发（0=自动）；每十倍分步法（log/dB 默认）按"每十倍
		//刻度数/每十倍网格线数"划分每个数量级
		if (a.scaleMode == 0 && a.tickGenMode == 0)          //linear 均匀分步法
			_scaleDraw.setScaleDiv(mCurveUniformTickDiv(lo, hi, a.tickCount, a.minorCount, 5));
		else if (a.scaleMode != 0 && a.tickGenMode == 1)     //log/dB 均匀分步法
			_scaleDraw.setScaleDiv(mCurveUniformTickDiv(lo, hi, a.tickCount, a.minorCount, 5));
		else if (a.scaleMode != 0 && a.tickGenMode == 2)     //log/dB 固定步长法
		{
			QwtLinearScaleEngine engine;
			_scaleDraw.setScaleDiv(engine.divideScale(lo, hi, 8, 5,
				a.majorStep > 0 ? a.majorStep : 0.0));
		}
		else if (a.scaleMode != 0)                           //log/dB 每十倍分步法（默认）
			_scaleDraw.setScaleDiv(mCurveDecadeTickDiv(lo, hi, decadeWidthOf(a.scaleMode),
				a.decadeTickCount, a.decadeMinorCount, 5));
		else                                                 //linear 固定步长法
		{
			QwtLinearScaleEngine engine;
			const double step = (a.tickGenMode == 1 && a.majorStep > 0) ? a.majorStep : 0.0;
			_scaleDraw.setScaleDiv(engine.divideScale(lo, hi, 8, 5, step));
		}

		//对齐引擎侧观感：主刻度8px；次刻度 linear 4px，log/dB 不画
		//（次刻度位置只承载画布网格线，轴上不出现小刻度标记）。
		//主干线（基线）必须画：条带位于画布边框之外，无框可借——首枚副轴
		//（引擎原生轴）的基线由画布闭合边框充当，条带关掉主干线只剩刻度悬空
		_scaleDraw.setAlignment(_top ? QwtScaleDraw::TopScale : QwtScaleDraw::RightScale);
		_scaleDraw.enableComponent(QwtAbstractScaleDraw::Backbone, true);
		_scaleDraw.setTickLength(QwtScaleDiv::MajorTick, 8);
		_scaleDraw.setTickLength(QwtScaleDiv::MinorTick, a.scaleMode != 0 ? 0 : 4);
		_scaleDraw.setTickLength(QwtScaleDiv::MediumTick, 0);

		//厚度 = 内侧留白 + 刻度/数字 extent + 标题区（空标题不占）
		const int titleBand = _title.isEmpty()
			? 0 : (QFontMetrics(_titleFont).height() + kStripTitleSpacing);
		_thickness = static_cast<int>(std::ceil(_scaleDraw.extent(_labelFont))) + titleBand + kStripInnerPad * 2 + 2;
	}

	void mCurveAxisStripWidget::refreshScale()
	{
		if (_axisNodeId < 0 || !_qwt)
			return;
		rebuildScale(_qwt);
		update();
	}

	void mCurveAxisStripWidget::setSelected(bool selected)
	{
		if (_selected == selected)
			return;
		_selected = selected;
		update();
	}

	void mCurveAxisStripWidget::paintEvent(QPaintEvent* event)
	{
		Q_UNUSED(event)
		QPainter p(this);
		p.setFont(_labelFont);
		QPalette pal;
		pal.setColor(QPalette::Text, Qt::black);

		//标题字体/颜色：选中=主题蓝+加粗（视图状态），否则模型值（无效兜底黑）
		QFont titleFont = _titleFont;
		QColor titleColor = _titleColor.isValid() ? _titleColor : QColor(Qt::black);
		if (_selected)
		{
			titleFont.setBold(true);
			titleColor = kStripSelectedColor;
		}

		if (_top)
		{
			//背景与图表边距区同色（WA_NoSystemBackground 不擦底，须自绘填充）
			p.fillRect(rect(), _bgColor);
			//横向与首枚副轴（引擎 xTop 原生轴）刻度像素级同源：直接取其刻度绘制器
			//的原点/长度（含 Qwt 布局的边距内缩）。此前用画布矩形+全局坐标往返，
			//高 DPI 取整与布局边距差会让刻度相对原生轴轻微偏移；布局未就绪
			//（长度 0）时退化为画布矩形
			int x0 = 0;
			int span = width();
			QWidget* cv = _qwt ? _qwt->canvas() : nullptr;
			QwtScaleWidget* topScale = _qwt ? _qwt->axisWidget(QwtPlot::xTop) : nullptr;
			const QwtScaleDraw* topDraw = topScale ? topScale->scaleDraw() : nullptr;
			if (topDraw && topDraw->length() > 0.0 && parentWidget())
			{
				x0 = topScale->mapTo(parentWidget(), QPoint(0, 0)).x()
					+ static_cast<int>(topDraw->pos().x());
				span = static_cast<int>(topDraw->length());
			}
			else if (cv)
			{
				x0 = mapFromGlobal(cv->mapToGlobal(QPoint(0, 0))).x();
				span = cv->width();
			}
			//基线贴近条带下边缘（朝向绘图区），刻度与数字向上（朝外）；
			//自 x0 起以原生轴同长映射（刻度逐像素对位；末端刻度数字允许
			//越界裁剪，对齐引擎轴行为）
			_scaleDraw.move(x0, height() - kStripInnerPad - 1);
			_scaleDraw.setLength(span);
			_scaleDraw.draw(&p, pal);
			if (!_title.isEmpty())
			{
				p.setFont(titleFont);
				p.setPen(titleColor);
				p.drawText(QRect(x0, kStripInnerPad,
					span, QFontMetrics(titleFont).height()),
					Qt::AlignHCenter | Qt::AlignVCenter, _title);
			}
		}
		else
		{
			p.fillRect(rect(), _bgColor);
			//纵向与首枚副轴（引擎 yRight 原生轴）刻度像素级同源（原点/长度，
			//含 Qwt 布局边距内缩；未就绪时退化为条带全长，几何由视图按画布排布）
			int y0 = 0;
			int len = height();
			QwtScaleWidget* rightScale = _qwt ? _qwt->axisWidget(QwtPlot::yRight) : nullptr;
			const QwtScaleDraw* rightDraw = rightScale ? rightScale->scaleDraw() : nullptr;
			if (rightDraw && rightDraw->length() > 0.0 && parentWidget())
			{
				y0 = rightScale->mapTo(parentWidget(), QPoint(0, 0)).y()
					+ static_cast<int>(rightDraw->pos().y());
				len = static_cast<int>(rightDraw->length());
			}
			//基线贴近条带左边缘（朝向绘图区），刻度与数字向右（朝外）
			_scaleDraw.move(kStripInnerPad, y0);
			_scaleDraw.setLength(len);
			_scaleDraw.draw(&p, pal);
			if (!_title.isEmpty())
			{
				//标题竖排在条带外侧（与引擎原生右轴标题同向：自上而下，
				//Qwt RightScale 同款 +90°——各 Y 轴标题阅读方向统一）
				p.save();
				p.setFont(titleFont);
				p.setPen(titleColor);
				p.translate(width() - kStripInnerPad - QFontMetrics(titleFont).height() / 2.0,
					height() / 2.0);
				p.rotate(90);
				p.drawText(QRect(-height() / 2, -1000, height(), 2000),
					Qt::AlignHCenter | Qt::AlignVCenter, _title);
				p.restore();
			}
		}
	}

	void mCurveAxisStripWidget::mousePressEvent(QMouseEvent* event)
	{
		//右键=轴区菜单（复制/粘贴/添加坐标轴/编辑）；左键不做事（双击另路处理）
		if (event->button() == Qt::RightButton)
		{
			emit sig_contextMenuRequested(event->globalPos());
			return;
		}
		QWidget::mousePressEvent(event);
	}

	void mCurveAxisStripWidget::mouseDoubleClickEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton)
		{
			emit sig_doubleClicked(event->globalPos());
			return;
		}
		QWidget::mouseDoubleClickEvent(event);
	}

	void mCurveAxisStripWidget::mouseMoveEvent(QMouseEvent* event)
	{
		Q_UNUSED(event)
		emit sig_hoverChanged(true);
	}

	void mCurveAxisStripWidget::leaveEvent(QEvent* event)
	{
		Q_UNUSED(event)
		emit sig_hoverChanged(false);
	}
}
