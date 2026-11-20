#include "mCurvePolarSeriesItem.h"

#include "../CurveCore/CurveDefine.h"   //curveHighlightBlue（选中/hover 高亮同链同色）

#include <QPainter>
#include <QPainterPath>
#include <QLineF>
#include <qwt_text.h>
#include <cmath>

namespace PostCurve
{
	//相位回绕 [0,360)
	static double wrapPhaseDeg(double deg)
	{
		deg = std::fmod(deg, 360.0);
		if (deg < 0.0)
			deg += 360.0;
		return deg;
	}

	mCurvePolarSeriesItem::mCurvePolarSeriesItem()
		: QwtPlotItem(QwtText(QStringLiteral("PolarSeriesItem")))
	{
		//向量层层级：底图（FrameItem Z5）之上、基准线标签（Z50/51）与红线游标（Z990）之下
		setZ(20.0);
		setItemAttribute(QwtPlotItem::Legend, false);
	}

	void mCurvePolarSeriesItem::clearSeriesData()
	{
		if (m_entries.isEmpty())
			return;
		m_entries.clear();
		itemChanged();
	}

	void mCurvePolarSeriesItem::addSeries(const PolarSeriesEntry& entry)
	{
		m_entries.append(entry);
		itemChanged();
	}

	void mCurvePolarSeriesItem::setChainForm(bool chain)
	{
		if (m_chainForm == chain)
			return;
		m_chainForm = chain;
		itemChanged();
	}

	void mCurvePolarSeriesItem::setScale(const PolarRadiusScale& scale)
	{
		m_scale = scale;
		itemChanged();
	}

	void mCurvePolarSeriesItem::setSelectedNodeId(int nodeId)
	{
		if (m_selectedNodeId == nodeId)
			return;
		m_selectedNodeId = nodeId;
		itemChanged();
	}

	void mCurvePolarSeriesItem::setHoveredNodeId(int nodeId)
	{
		if (m_hoveredNodeId == nodeId)
			return;
		m_hoveredNodeId = nodeId;
		itemChanged();
	}

	QString mCurvePolarSeriesItem::hitTest(const QRectF& canvasRect, const QPointF& pos) const
	{
		QString bestName;
		double bestDist = 12.0;   //命中阈值（px，端点圆点视觉半径的宽松包络）
		for (const PolarSeriesEntry& e : m_entries)
		{
			const int nPts = qMin(qMin(e.phaseDeg.size(), e.magnitude.size()),
				e.freqHz.isEmpty() ? e.magnitude.size() : e.freqHz.size());
			for (int i = 0; i < nPts; ++i)
			{
				const QPointF tip = entryTip(e, i, polarCanvasGeometry(canvasRect));
				const double dist = QLineF(QPointF(pos), tip).length();
				if (dist < bestDist)
				{
					bestDist = dist;
					bestName = e.name;
				}
			}
		}
		return bestName;
	}

	//条目+点下标 -> 端点像素位（缩放归一+角度回绕 [0,360)）
	QPointF mCurvePolarSeriesItem::entryTip(const PolarSeriesEntry& e, int pointIndex,
		const PolarCanvasGeometry& g) const
	{
		const int i = pointIndex;
		const int n = qMin(qMin(e.phaseDeg.size(), e.magnitude.size()), e.freqHz.size());
		if (i < 0 || i >= n)
			return g.center;
		const double mag = e.magnitude.at(i);
		const double rNorm = m_scale.normalizedRadius(mag);
		const double ang = wrapPhaseDeg(e.phaseDeg.at(i));
		return polarPixel(g, rNorm, ang);
	}

	//D35 深档视口裁剪（2026-11-16 真机实锤：深放大域 ~1.4E-09 时向量端点画布
	//坐标达 ±百万像素级，Qt 粗线三角化描边器（qtriangulatingstroker）float
	//中间量把相邻端点舍入坍缩成零长度段触发 ASSERT 弹窗——QPainter 自身裁剪
	//发生在三角化之后挡不住，须提交前手动裁段；商软同态不崩系其渲染栈裁剪在
	//先。Liang-Barsky 段-矩形求交：就地改写端点，不相交/裁剪后零段返 false；
	//两符号分支对调致全射线误弃的补修见 clipEdge 内注（2026-11-17 真机实锤）
	static bool clipSegToRect(QPointF& a, QPointF& b, const QRectF& r)
	{
		const double dx = b.x() - a.x();
		const double dy = b.y() - a.y();
		double t0 = 0.0, t1 = 1.0;
		auto clipEdge = [&t0, &t1](double p, double q)
		{
			if (p == 0.0)
				return q >= 0.0;
			const double t = q / p;
			//p<0 入边：t=q/p 为下界，越出 t1 即整段在外，否则抬 t0
			//（2026-11-17 真机实锤补修：本两支曾整对调——圆心恒在裁剪框内、
			//方向非零的射线首条边检验即算出负 t 误判整段在外，全部射线被
			//整条丢弃，真机"线全消失、只剩悬空箭头"）
			if (p < 0.0)
			{
				if (t > t1) return false;
				if (t > t0) t0 = t;
			}
			else
			{
				if (t < t0) return false;
				if (t < t1) t1 = t;
			}
			return true;
		};
		if (!clipEdge(-dx, a.x() - r.left()))
			return false;
		if (!clipEdge(dx, r.right() - a.x()))
			return false;
		if (!clipEdge(-dy, a.y() - r.top()))
			return false;
		if (!clipEdge(dy, r.bottom() - a.y()))
			return false;
		//先以原始 a 算双端点再赋值（a 若先改写，t0 位移会重复进 b）
		const QPointF na(a.x() + t0 * dx, a.y() + t0 * dy);
		const QPointF nb(a.x() + t1 * dx, a.y() + t1 * dy);
		a = na;
		b = nb;
		return true;
	}

	//实心三角箭头（需求 04 原型形态——旧开放后掠短须作废）：tip 处沿 dirDeg
	//画填充三角；返回射线的回缩短结点（防穿头）
	static QPointF arrowHead(QPainter* painter, const QPointF& tip, double dirDeg,
		const QColor& color, double width)
	{
		const double rad = dirDeg * kPolarPi / 180.0;
		const double dirx = std::cos(rad), diry = -std::sin(rad);
		const double len = qMax(9.0, 5.0 + width * 2.0);
		const double halfw = len * 0.38;
		const QPointF base(tip.x() - dirx * len, tip.y() - diry * len);
		const QPointF perp(-diry, dirx);
		QPainterPath head;
		head.moveTo(tip);
		head.lineTo(base.x() + perp.x() * halfw, base.y() + perp.y() * halfw);
		head.lineTo(base.x() - perp.x() * halfw, base.y() - perp.y() * halfw);
		head.closeSubpath();
		painter->save();
		painter->setPen(Qt::NoPen);
		painter->setBrush(color);
		painter->drawPath(head);
		painter->restore();
		return QPointF(tip.x() - dirx * len * 0.65, tip.y() - diry * len * 0.65);
	}

	void mCurvePolarSeriesItem::draw(QPainter* painter,
		const QwtScaleMap& xMap,
		const QwtScaleMap& yMap,
		const QRectF& canvasRect) const
	{
		Q_UNUSED(xMap)
		Q_UNUSED(yMap)
		const PolarCanvasGeometry g = polarCanvasGeometry(canvasRect);
		//D35 深档视口裁剪矩形：canvasRect 外扩 50%（线宽/抗锯齿/箭头回退余量）
		//——只裁提交给 QPainter 的喂料段，链锚推进/命中测试等几何语义仍用未
		//裁剪坐标（商软同态：线穿出画布缘视觉消失）
		const QRectF clipRect = canvasRect.adjusted(
			-canvasRect.width() * 0.5, -canvasRect.height() * 0.5,
			canvasRect.width() * 0.5, canvasRect.height() * 0.5);
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing, true);

		//条目公共态（色/宽/线型；选中/hover 高亮覆盖同链同规：选中=主题蓝+加粗
		//1px，hover 仅变色）
		auto entryPen = [this](const PolarSeriesEntry& e, QColor& color, double& width)
		{
			color = e.color;
			width = e.lineWidth;
			if (e.nodeId == m_hoveredNodeId || e.nodeId == m_selectedNodeId)
				color = curveHighlightBlue();
			if (e.nodeId == m_selectedNodeId)
				width += 1.0;
			QPen pen(color, width);
			pen.setCapStyle(Qt::FlatCap);
			pen.setStyle(e.lineStyle == 1 ? Qt::DashLine : Qt::SolidLine);
			return pen;
		};

		//零幅值退化（rNorm<=1e-3）画 1.5px 端点圆点
		auto drawDot = [painter](const QPointF& p)
		{
			painter->drawEllipse(p, 1.5, 1.5);
		};

		if (!m_chainForm)
		{
			//散开形态：每点一根 极点->端点 射线 + 端点实心三角箭头（spec：末端
			//SHALL 带箭头；零幅值退化极点圆点）
			for (const PolarSeriesEntry& e : m_entries)
			{
				const int nPts = qMin(qMin(e.phaseDeg.size(), e.magnitude.size()),
					e.freqHz.isEmpty() ? e.magnitude.size() : e.freqHz.size());
				if (nPts <= 0)
					continue;
				QColor color;
				double width = 0.0;
				painter->setPen(entryPen(e, color, width));
				painter->setBrush(Qt::NoBrush);
				for (int i = 0; i < nPts; ++i)
				{
					const double rNorm = m_scale.normalizedRadius(e.magnitude.at(i));
					const QPointF tip = entryTip(e, i, g);
					if (rNorm > 1e-3)
					{
						//箭头仅 tip 在视口内绘制（D35 深档：tip 出画布随线裁剪
						//消失，商软同态）；线段裁剪后提交（防深档端点 float 坍缩
						//零段触发描边器 ASSERT）
						const QPointF tail = clipRect.contains(tip)
							? arrowHead(painter, tip, wrapPhaseDeg(e.phaseDeg.at(i)),
								color, width)
							: tip;
						painter->setPen(entryPen(e, color, width));
						painter->setBrush(Qt::NoBrush);
						QPointF a = g.center, b = tail;
						if (clipSegToRect(a, b, clipRect))
							painter->drawLine(a, b);
					}
					else
					{
						drawDot(tip);
					}
				}
			}
		}
		else
		{
			//首尾相接形态（spec：各模态贡献向量按序首尾相连成折线——每段向量
			//终点=下段起点，链自极点起步跨条目累加；2026-11-12 修复：旧实现只在
			//条目内连点，单点曲线退化为圆点链不可用）。Response/Others 恒独立
			//射线带箭头（chainLink=false，原型 ex1 形态）；链段不带箭头；零幅值
			//向量不推进链锚点（相位无定义），锚点处画圆点
			QPointF chainAnchor = g.center;
			for (const PolarSeriesEntry& e : m_entries)
			{
				const int nPts = qMin(qMin(e.phaseDeg.size(), e.magnitude.size()),
					e.freqHz.isEmpty() ? e.magnitude.size() : e.freqHz.size());
				if (nPts <= 0)
					continue;
				QColor color;
				double width = 0.0;
				painter->setPen(entryPen(e, color, width));
				painter->setBrush(Qt::NoBrush);
				if (!e.chainLink)
				{
					for (int i = 0; i < nPts; ++i)
					{
						const double rNorm = m_scale.normalizedRadius(e.magnitude.at(i));
						const QPointF tip = entryTip(e, i, g);
						if (rNorm > 1e-3)
						{
							//箭头仅 tip 在视口内绘制（D35 深档：tip 出画布随线
							//裁剪消失，商软同态）；线段裁剪后提交（防深档端点
							//float 坍缩零段触发描边器 ASSERT）
							const QPointF tail = clipRect.contains(tip)
								? arrowHead(painter, tip, wrapPhaseDeg(e.phaseDeg.at(i)),
									color, width)
								: tip;
							painter->setPen(entryPen(e, color, width));
							painter->setBrush(Qt::NoBrush);
							QPointF a = g.center, b = tail;
							if (clipSegToRect(a, b, clipRect))
								painter->drawLine(a, b);
						}
						else
						{
							drawDot(tip);
						}
					}
					continue;
				}
				for (int i = 0; i < nPts; ++i)
				{
					const double rNorm = m_scale.normalizedRadius(e.magnitude.at(i));
					//真·首尾相接：本段终点=锚点+向量偏移（偏移取该向量绝对端点
					//相对极点的位移）。旧实现直接把绝对端点当段终点连线——投影
					//模式下各模态共线，负投影/小向量的绝对端点回折，长段来回
					//覆盖后只剩末位短模态的色（2026-11-12 真机复验实锤）
					const QPointF tip = chainAnchor
						+ (entryTip(e, i, g) - g.center);
					if (rNorm > 1e-3)
					{
						painter->setPen(entryPen(e, color, width));
						painter->setBrush(Qt::NoBrush);
						//段裁剪后提交（D35 防深档端点 float 坍缩零段触发描边
						//器 ASSERT）；链锚推进/节点圆点仍用未裁剪值（几何语义
						//不变，商软同态：段穿出画布缘视觉消失）
						QPointF a = chainAnchor, b = tip;
						if (clipSegToRect(a, b, clipRect))
							painter->drawLine(a, b);
						chainAnchor = tip;
						//节点圆点出视口不画（深档中心坐标巨，fill 退化防御）
						if (clipRect.contains(chainAnchor))
							painter->drawEllipse(chainAnchor, 1.5, 1.5);
					}
					else
					{
						//链锚出视口零值点不画（同防御）
						if (clipRect.contains(chainAnchor))
							drawDot(chainAnchor);
					}
				}
			}
		}
		painter->restore();
	}
}
