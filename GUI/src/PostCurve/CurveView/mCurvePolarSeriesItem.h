#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 极坐标向量图元（QwtPlotItem，design D5）
//
// chartType=3 极坐标画布的数据层：全部可见曲线逐条入列（每条=一.contribution
// 向量，散开形态画 极点->端点 射线+箭头；"首尾相接"链形态按入列序连线端点）。
// 半径经 PolarRadiusScale 归一（计权先加权后变换，域归一收口视图侧
// applyPolarDecor），角度为相位度数（数学正向，屏幕 y 翻转，几何契约同
// mCurvePolarFrameItem.h）。
//
// 选中/hover 高亮：entry.nodeId 对应引擎曲线节点（图例/命中链载体），视图
// applyHighlightVisuals 按现有链重放（选中=主题蓝+加粗，hover 仅变色）。
//
// 纯视图元素：不进模型/命令栈，随全量重建重建（策略同基准线标签/柱顶标注）。
// =============================================================================

#include "mCurvePolarFrameItem.h"   //几何/半径缩放契约（同族共用）

#include <qwt_plot_item.h>
#include <QColor>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

class QwtScaleMap;

namespace PostCurve
{
	//极坐标向量条目（一条引擎曲线一条目；多点曲线=多向量/链多折点）
	struct PolarSeriesEntry
	{
		int nodeId = -1;                //引擎曲线节点 id（选中/hover 高亮对号）
		QString name;                   //曲线名（命中返回名，视图按名查节点）
		QColor color = Qt::blue;        //向量色（随曲线色）
		double lineWidth = 1.0;         //线宽（随曲线线宽）
		int lineStyle = 0;              //0 实线/1 虚线（Others 虚线形态）
		QVector<double> phaseDeg;       //相位（度，0 沿 +x 逆时针）
		QVector<double> magnitude;      //幅值（原值空间，绘制时经缩放契约归一）
		QVector<double> freqHz;         //逐点计权频率（weightGain 入参；单点曲线=1 元素）
		bool chainLink = true;          //链形态成员（首尾相接形态下跨条目按入列序相连；false=Response/Others 独立射线——2026-11-12 裁定）
	};

	class POSTCURVE_EXPORT mCurvePolarSeriesItem : public QwtPlotItem
	{
	public:
		mCurvePolarSeriesItem();

		void clearSeriesData();
		void addSeries(const PolarSeriesEntry& entry);
		//首尾相接形态（false=散开：极点->端点射线+箭头）
		void setChainForm(bool chain);
		//半径缩放契约（域已按 applyPolarDecor 收口）
		void setScale(const PolarRadiusScale& scale);
		//选中/hover 高亮（节点 id 对号，-1=无；命中查询返回节点名）
		void setSelectedNodeId(int nodeId);
		void setHoveredNodeId(int nodeId);

		//端点命中查询：返回最近向量端点（阈值内）的曲线名，未命中返回空串
		QString hitTest(const QRectF& canvasRect, const QPointF& pos) const;

	protected:
		void draw(QPainter* painter,
			const QwtScaleMap& xMap,
			const QwtScaleMap& yMap,
			const QRectF& canvasRect) const override;

	private:
		//条目+点下标 -> 端点像素位（缩放归一+角度回绕 [0,360)）
		QPointF entryTip(const PolarSeriesEntry& e, int pointIndex,
			const PolarCanvasGeometry& g) const;

		QList<PolarSeriesEntry> m_entries;
		PolarRadiusScale m_scale;
		bool m_chainForm = false;
		int m_selectedNodeId = -1;
		int m_hoveredNodeId = -1;
	};
}
