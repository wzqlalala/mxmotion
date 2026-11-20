#include "mCurveChartView.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveCore/ChartTypeProfile.h"
//计权增益（A/B/C IEC 公式，transformPoint 逐点修正用）
#include "../CurveCore/mCurveWeighting.h"

#include <QVBoxLayout>
#include "mUiMenu.h"
#include <QLayout>
#include <QShortcut>
#include <QKeySequence>
#include <QFont>
#include <QFontMetrics>
#include <QFontInfo>
#include <QFrame>
#include <QTimer>
#include <QPainter>
#include <QLineF>
#include <QUndoStack>
#include <QMenu>
#include <QIcon>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QCursor>
#include <QPen>
#include <QPainter>
#include <QtMath>
#include <QEvent>
#include <QLineF>
#include <QUndoStack>
#include <QCoreApplication>
#include <QFileInfo>
#include <QStringList>
#include <QVector>
#include <cmath>
#include <limits>

#include "mPlotView.h"
#include "mPlotSymbol.h"
#include "mPlotLegendItem.h"
#include "mPlotAnnotation.h"
#include "mPlotSnapper.h"

// --- [PERF] 临时计时插桩（对数轴切换卡慢排查用，定位后移除） ---
#include <QDebug>
#include <QElapsedTimer>
namespace { QElapsedTimer g_perfTimer; qint64 g_perfLast = 0; }
#define PERF_MARK(msg) do {     const qint64 now = g_perfTimer.isValid() ? g_perfTimer.elapsed() : (g_perfTimer.start(), 0);     qDebug().noquote() << QString("[PERF] +%1ms %2 (view %3)").arg(now - g_perfLast).arg(QStringLiteral(msg)).arg(_chartId);     g_perfLast = now; } while (0)
// ------------------------------------------------------------

#include "mPlotMarkerLine.h"
#include "mPlotDataProcessor.h"
#include "mCurveReadoutItem.h"
//刻度生成方式引擎（均匀分步法=精确"刻度总数"个主刻度数字）
#include "mCurveAxisTickEngine.h"
//副轴自绘条带（多坐标轴；mCurveScaleDraw 亦定义于其头文件，本视图共用）
#include "mCurveAxisStripWidget.h"

//注释模式全局开关（Ribbon"注释"按钮）经视口工厂广播
#include "mCurveViewportFactory.h"

//编辑曲线会话的裁剪线图元（旁路自管，不进引擎注册表）
#include "mCurveTrimMarker.h"

//基准线线上标签图元（旁路自管，不进引擎注册表）
#include "mCurveBarLabelItem.h"
#include "mCurveBaselineLabel.h"
#include "mCurveFrequencyCursor.h"

//极坐标画布接管图元（design D5：底图+向量层，旁路自管不进引擎注册表）
#include "mCurvePolarFrameItem.h"
#include "mCurvePolarSeriesItem.h"

//应用样式对话框（图表背景右键"应用样式"）
#include "../mApplyStyleDlg.h"

//右键菜单自绘纳管（复数图图表级菜单与全局命令菜单同观感：mUiMenu 全自绘 +
//ThemeManager::applyMenuStyle 清 QSS/打标，见 onAxisZoneContextMenu 复数分支）
#include "mUiMenu.h"
#include "ThemeManager.h"

//注释文本编辑对话框（PostCommon）
#include "mPostAnnotationDialog.h"

//Qwt：刻度文本格式化（'g'4位有效数字）、符号样式
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_text.h>
#include <qwt_symbol.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_item.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_div.h>

namespace PostCurve
{
	using PlotView::AxisId;
	using PlotView::AxisSettings;
	using PlotView::CurveSettings;
	using PlotView::TecLinePattern;
	using PlotView::ComplexFormat;

	//注释吸附半径（像素）：注释模式内设给引擎 snapper 的吸附阈值
	static const double kAnnotationSnapPx = 30.0;

	//基准线命中带宽（像素）：退出基准线模式后双击/右键/单击的命中判定阈值
	static const double kBaselineHitPx = 5.0;

	namespace
	{
		//画布边框覆盖层：在画布最上层绘制 1px 闭合矩形（4 条边框线）。
		//不依赖轴主干线充当边框：Qwt 6.2 的主干线受 borderDist（随首尾
		//刻度标签宽度变化，且布局每次激活都会按标签提示重设，构造期清零
		//无效）、canvasMargin/spacing 影响，公开 API 拼不出闭合矩形
		class BorderOverlay : public QWidget
		{
		public:
			using QWidget::QWidget;

			//极坐标画布不画直角边框（原型 05：纯极坐标画布，2026-11-12 轮）；
			//直角图表维持 1px 闭合矩形（默认开，存量行为不变）
			void setFrameVisible(bool on)
			{
				if (_frameVisible == on)
					return;
				_frameVisible = on;
				update();
			}

		protected:
			void paintEvent(QPaintEvent* event) override
			{
				if (!_frameVisible)
					return;
				QPainter p(this);
				p.setPen(QPen(Qt::black, 1.0));
				p.drawRect(rect().adjusted(0, 0, -1, -1));
			}

		private:
			bool _frameVisible = true;
		};
	}

//刻度数字格式化 'g'4（mCurveScaleDraw）与副轴自绘条带共用，见 mCurveAxisStripWidget.h

	//数值变换（对齐 NVHCurveWgt::transformValue 语义）：
	//0 Linear 原值 / 1 Logarithmic=log10 / 2 dB20=20log10(val/ref) / 3 dB10=10log10(val/ref)；
	//ref 为轴参考值（axis.dbRef），仅 dB 模式参与——修改参考值即整条曲线平移
	//常数偏移 20/10·log10(ref)，ref 非法（<=0/NaN）按 1 兜底。
	//非法点（<=0）返回 NaN，由调用方过滤（对齐原型断点行为）。
	//公开静态（类成员）：编辑曲线会话（工厂算数据界/浮动窗口"裁剪"落盘）
	//与本视图渲染共用同一变换，保证范围值与画面刻度一致
	double mCurveChartView::transformValue(double val, int scaleMode, double dbRef)
	{
		const double ref = (qIsFinite(dbRef) && dbRef > 0.0) ? dbRef : 1.0;
		switch (scaleMode)
		{
		case 1:
			if (val <= 0) return qQNaN();
			return std::log10(val);
		case 2:
			if (val <= 0) return qQNaN();
			return 20.0 * std::log10(val / ref);
		case 3:
			if (val <= 0) return qQNaN();
			return 10.0 * std::log10(val / ref);
		default:
			return val;
		}
	}

	//逆变换（渲染空间 -> 原始数据空间；Linear 恒等）："裁剪"按显示范围过滤
	//模型原始数据时使用（对数/dB 轴下必须逆变换回去再过滤）；dB 同样带参考
	//值（ref·10^(t/系数)），与正变换严格互逆
	double mCurveChartView::inverseTransformValue(double val, int scaleMode, double dbRef)
	{
		const double ref = (qIsFinite(dbRef) && dbRef > 0.0) ? dbRef : 1.0;
		switch (scaleMode)
		{
		case 1:
			return std::pow(10.0, val);
		case 2:
			return ref * std::pow(10.0, val / 20.0);
		case 3:
			return ref * std::pow(10.0, val / 10.0);
		default:
			return val;
		}
	}

	//统一逐点入口：计权修正（线性增益）-> 刻度模式变换（dB 除入参考值）。
	//X 计权以自身原始值为频率，Y 计权以同一索引的 X 原始值为频率（对齐原型
	//applyWeight 语义）。增益经 effectiveWeightGain 档位闸（2026-11-16 裁定①）：
	//仅 dB 档生效，Linear/Log 档恒 1.0 渲染原始值；增益对 0 值中性（0×gain=0），
	//dB/Log 下变换 NaN 仍走原剔除约定
	void mCurveChartView::transformPoint(double rawX, double rawY,
		int xMode, int yMode, int xWeight, int yWeight,
		double& tx, double& ty, double xDbRef, double yDbRef)
	{
		tx = transformValue(rawX * effectiveWeightGain(xMode, xWeight, rawX), xMode, xDbRef);
		ty = transformValue(rawY * effectiveWeightGain(yMode, yWeight, rawX), yMode, yDbRef);
	}

	bool mCurveChartView::displayedAxisRange(int axisNodeId, double* lo, double* hi) const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc ? doc->node(axisNodeId) : nullptr;
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!n || !qwt || doc->parentNodeId(axisNodeId) != _chartId)
			return false;
		//极坐标（D25）：径向域由 applyPolarDecor 解析并存值，不经引擎轴映射——
		//Y1 回读存域（面板回显/自动档退出回写所见即所得，替代载体 [-1.05,1.05]）；
		//角度轴 X1 无径向域语义（角度恒 0~360 由底图绘制），返回 false 走面板
		//回退且不回写模型
		{
			CurveNode* chartNode = doc->node(_chartId);
			if (chartNode && chartNode->chart.chartType == 3)
			{
				if (n->type == CurveNodeType::YAxis && _polarDomainValid)
				{
					if (lo) *lo = _polarDomainLo;
					if (hi) *hi = _polarDomainHi;
					return true;
				}
				return false;
			}
		}
		QwtPlot::Axis engineAxis = QwtPlot::yLeft;
		switch (n->type)
		{
		case CurveNodeType::XAxis:  engineAxis = QwtPlot::xBottom; break;
		case CurveNodeType::YAxis:  engineAxis = QwtPlot::yLeft;   break;
			//副轴同侧共用引擎轴：首枚占引擎原生轴，第2枚起自绘条带镜像的
			//就是这根轴的当前区间（fit/缩放后的现值）
		case CurveNodeType::XAxis2: engineAxis = QwtPlot::xTop;    break;
		case CurveNodeType::YAxis2:
		{
			//复数图：Phase 轴（YAxis2）落在引擎下面板 Y（不占上面板右轴）
			CurveNode* chartNode = doc->node(_chartId);
			if (chartNode && chartNode->chart.chartType == 2)
			{
				if (QwtPlot* lowerQwt = _plot ? _plot->getLowerQwtPlot() : nullptr)
				{
					const QwtScaleDiv& ldiv = lowerQwt->axisScaleDiv(QwtPlot::yLeft);
					const double lLo = ldiv.lowerBound();
					const double lHi = ldiv.upperBound();
					if (std::isfinite(lLo) && std::isfinite(lHi) && lLo < lHi)
					{
						if (lo) *lo = lLo;
						if (hi) *hi = lHi;
						return true;
					}
				}
				return false;
			}
			engineAxis = QwtPlot::yRight;
			break;
		}
		default: return false;
		}
		const QwtScaleDiv& div = qwt->axisScaleDiv(engineAxis);
		//返回"轴上所见"的刻度变换空间界值（面板最小值/最大值所见即所得：
		//Log 模式下就是轴上数字本身，如 0~2；dB 同理），不再逆变换回原始
		//空间。界值无频率上下文，不做计权
		const double rLo = div.lowerBound();
		const double rHi = div.upperBound();
		if (!std::isfinite(rLo) || !std::isfinite(rHi) || !(rLo < rHi))
			return false;
		if (lo)
			*lo = rLo;
		if (hi)
			*hi = rHi;
		return true;
	}

	bool mCurveChartView::queryAxisDisplayRange(int axisNodeId, double* lo, double* hi)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int chartId = doc ? doc->parentNodeId(axisNodeId) : -1;
		if (chartId <= 0)
			return false;
		mCurveChartView* view = mCurveViewportFactory::getInstance()->chartView(chartId);
		if (view)
			return view->displayedAxisRange(axisNodeId, lo, hi);
		//懒加载回退：视图未物化时取文档轴配置——手动范围（autoScale=false）已持久化
		//在文档节点，直接回读；自动范围无法离线算出，返回 false 由调用方按数据适配处理
		CurveNode* axis = doc ? doc->node(axisNodeId) : nullptr;
		if (!axis || (axis->type != CurveNodeType::XAxis && axis->type != CurveNodeType::YAxis))
			return false;
		if (axis->axis.autoScale)
			return false;
		if (lo) *lo = axis->axis.min;
		if (hi) *hi = axis->axis.max;
		return true;
	}

	//符号形状映射：模型 symbolStyle -> QwtSymbol::Style
	//（0无 1圆 2方 3菱 4三角 5十字 6倒三角 7星(五角星) 8交叉 9点，与绘图选项
	//symbolCombo 一致；7 映射 PlotView 扩展样式 Sym_Star5，由 mPlotSymbol 渲染；
	//米字 Star1 仍在库中保留，PlotView 样式面板可直接选用）
	static QwtSymbol::Style symbolStyleToQwt(int style)
	{
		switch (style)
		{
		case 1: return QwtSymbol::Ellipse;
		case 2: return QwtSymbol::Rect;
		case 3: return QwtSymbol::Diamond;
		case 4: return QwtSymbol::UTriangle;
		case 5: return QwtSymbol::Cross;
		case 6: return QwtSymbol::DTriangle;
		case 7: return static_cast<QwtSymbol::Style>(PlotView::Sym_Star5);
		case 8: return QwtSymbol::XCross;
		case 9: return QwtSymbol::Ellipse;
		default: return QwtSymbol::NoSymbol;
		}
	}

	//页眉/页脚对齐映射：模型 align（0居中/1左/2右，同绘图选项 headerAlign/
	//footerAlign 下拉经 alignOf 归一后的值）-> Qt 水平对齐
	static Qt::Alignment bandAlignmentOf(int align)
	{
		if (align == 1)
			return Qt::AlignLeft;
		if (align == 2)
			return Qt::AlignRight;
		return Qt::AlignHCenter;
	}

	//多级标题字号折算：基准字号依级缩小 4（基准可能来自 QFontDialog 的磅值
	//选择，像素/磅两种口径都要覆盖；基准字体未显式设字号（pixel/point 均
	//-1，如老文档的默认 QFont）时按 QFontInfo 解析出的实际渲染字号递减，
	//否则各级原样返回会退化成同字号）
	QFont scaledLevelFont(const QFont& base, int delta)
	{
		QFont f = base;
		if (f.pixelSize() > 0)
			f.setPixelSize(qMax(1, f.pixelSize() - delta));
		else if (f.pointSizeF() > 0)
			f.setPointSizeF(qMax<qreal>(1.0, f.pointSizeF() - delta));
		else
		{
			QFontInfo info(f);
			if (info.pixelSize() > 0)
				f.setPixelSize(qMax(1, info.pixelSize() - delta));
		}
		return f;
	}

	//该级标题字体：属性页显式选择过（≠"未选"的默认字体值）用所选值，否则
	//按一级字号递减。判据必须用默认字体值整体比较——family() 对未设族名的
	//字体返回的是解析出的应用默认族名（如 SimSun，非空），不能用作"未选"判据
	//（否则未选的二级/三级都按默认 9pt 渲染，小且同级同字号）
	QFont levelFontOf(const QFont& base, const QFont& chosen, int delta)
	{
		static const QFont kUnsetFont;
		if (chosen == kUnsetFont)
			return scaledLevelFont(base, delta);
		return chosen;
	}

	//页眉/页脚多级标题按行分级（"编辑字符"弹窗按行输入）：第 1 行=一级标题
	//（一级字体原样），第 2 行=二级标题，第 3 行起恒为三级标题（行数不限）；
	//各级字体取模型 font2/font3（未选按一级字号依级缩小 4：二级 -4/三级 -8），
	//各行共用模型对齐方式（属性页显示组"对齐方式"统一控制），空行保留占位
	//（空文本整条隐藏由调用方与引擎兜底）。
	//color/bold：逐行文字颜色与加粗（引擎 PageBandLine::color 能力）——选中态
	//下发主题蓝+加粗、未选中下发模型色（颜色字段由调用方算好，含无效兜底）
	QVector<PlotView::PageBandLine> bandLinesOf(const QString& text,
		const QFont& baseFont, const QFont& chosenFont2, const QFont& chosenFont3,
		Qt::Alignment align, const QColor& color = QColor(), bool bold = false, int emphLine = -1)
	{
		QVector<PlotView::PageBandLine> lines;
		const QStringList rows = text.split(QLatin1Char('\n'));
		for (int i = 0; i < rows.size(); ++i)
		{
			PlotView::PageBandLine line;
			line.text = rows.at(i);
			line.align = align;
			//强调行（需求02表头层级）：加粗+一级字号+2 放大；显式选过二级字体用所选值
			if (i == emphLine)
			{
				line.font = levelFontOf(baseFont, chosenFont2, -2);
				line.font.setBold(true);
			}
			else if (i == 1)
				line.font = levelFontOf(baseFont, chosenFont2, 4);
			else if (i >= 2)
				line.font = levelFontOf(baseFont, chosenFont3, 8);
			else
				line.font = baseFont;
			if (bold)
				line.font.setBold(true);
			line.color = color;
			lines.append(line);
		}
		return lines;
	}

	//--- 复数图（图表类型2，设计图02-02）显示辅助 ---

	//复数曲线已合并为单节点（series 自带上/下面板两路数据，按 isComplex()
	//分派），旧"按命名约定配对"的 classifyComplexName/ComplexRole 已退役。

	//复数图显示文本去分量令牌：如"MAG|X-Trans"-> "X-Trans"。
	//用于页眉/页脚/轴标题模板——ySideComponent 存通道选择器原形（"MAG|X-Trans"），
	//模板解析后经此去掉分量令牌（仅显示层，模型不动）
	static QString complexDisplayName(const QString& text)
	{
		static const QString kToks[] = { QStringLiteral("MAG|"),
			QStringLiteral("PHA|"), QStringLiteral("REAL|"),
			QStringLiteral("IMAG|") };
		QString out = text;
		for (const QString& tok : kToks)
			out.replace(tok, QString(), Qt::CaseInsensitive);
		return out;
	}

	//复数图面板分量标题（HyperGraph 2024 语义：面板标题为图表固有分量名，随
	//显示格式派生，不取轴节点标题）：magSide=true 为幅值/实部侧（Y1 侧）
	static QString complexPanelComponentTitle(bool magSide, int displayFmt)
	{
		if (magSide)
			return displayFmt == 0 ? QStringLiteral("Magnitude")
				: QStringLiteral("Real");
		return displayFmt == 0 ? QStringLiteral("Phase")
			: QStringLiteral("Imaginary");
	}

	//复数两路数据的显示换算（设计图01 幅值-相位与实部-虚部灵活切换）：节点存
	//创建时的原始形态（fromFmt：0=幅相 a=幅值/b=相位°，1=实虚 a=实部/b=虚部），
	//图表显示格式（kComplexDisplayFormat）可经右键切换——渲染层现算互转（幅相↔
	//实虚），不改模型，来回切换无损。同格式原样拷贝
	void convertComplexDisplay(const QVector<double>& aIn, const QVector<double>& bIn,
		int fromFmt, int toFmt, QVector<double>& aOut, QVector<double>& bOut)
	{
		if (fromFmt != 0 && fromFmt != 1)
			fromFmt = 0;
		if (toFmt != 0 && toFmt != 1)
			toFmt = 0;
		if (fromFmt == toFmt)
		{
			aOut = aIn;
			bOut = bIn;
			return;
		}
		const int n = qMin(aIn.size(), bIn.size());
		aOut.resize(n);
		bOut.resize(n);
		for (int i = 0; i < n; ++i)
		{
			const double a = aIn.at(i);
			const double b = bIn.at(i);
			if (fromFmt == 0)
			{
				//幅相 -> 实虚：实 = 幅值·cos(相位)，虚 = 幅值·sin(相位)（相位为度）
				const double ph = qDegreesToRadians(b);
				aOut[i] = a * std::cos(ph);
				bOut[i] = a * std::sin(ph);
			}
			else
			{
				//实虚 -> 幅相：幅值 = hypot(实, 虚)，相位 = atan2(虚, 实)（度）
				aOut[i] = std::hypot(a, b);
				bOut[i] = qRadiansToDegrees(std::atan2(b, a));
			}
		}
	}

	//--- 曲线标签模板解析（参数栏/弹窗"曲线标签"）
	//曲线单侧（X/Y）来源信息取值
	struct CurveLabelSideSource { QString subcase, datatype, request, component; };

	//侧来源取值（与 CurveDocument x/y 数据节点回显约定同源，CurveDocument.cpp
	//nodeProperty 读写重定向）：X 侧 subcase 空回退锚通道工况，
	//schema/object/component 取侧独立值（横坐标形态下为空，不回退）；Y 侧
	//侧独立 schema 为空 = 旧数据未记录，回退创建时写入的五元组。
	//无来源身份回退曲线名：数值/数学来源的手工定义曲线结果类型与对象皆空
	//（component 仅为创建时的数据摘要），MX 身份字段回退曲线名、摘要置空——
	//标签/表头模板解析出 "Curve N" 而非数据转储（component 置空后模板中的
	//分隔符悬空，由 resolveBandTemplate/composeCurveLegendText 收尾去除）
	static CurveLabelSideSource curveLabelSideSource(const CurveSeriesData& s, bool xSide)
	{
		CurveLabelSideSource v;
		if (xSide)
		{
			v.subcase = s.xSideSubcase.isEmpty() ? s.subcase : s.xSideSubcase;
			v.datatype = s.xSideSchema;
			v.request = s.xSideObject;
			v.component = s.xSideComponent;
		}
		else
		{
			const bool hasSide = !s.ySideSchema.isEmpty();
			v.subcase = hasSide ? s.ySideSubcase : s.subcase;
			v.datatype = hasSide ? s.ySideSchema : s.schema;
			v.request = hasSide ? s.ySideObject : s.object;
			v.component = hasSide ? s.ySideComponent : s.component;
		}
		if (v.datatype.isEmpty() && v.request.isEmpty())
		{
			v.request = s.name;
			v.component.clear();
		}
		return v;
	}

	//Y 最大值所在点索引（统计占位符 {max(y)}/{x} 共用）：按 y 侧 缩放/偏移 后的
	//显示值比较（与曲线渲染同源）；跳过非有限值（NaN/Inf，对齐渲染层"非法点
	//过滤"约定）与"编辑曲线"隐藏区间内的点（模型保留但渲染为断口，不参与
	//统计）；按 X/Y 公共长度取，无有效点返回 -1
	static int finiteMaxYIndex(const CurveSeriesData& s)
	{
		int best = -1;
		const int n = qMin(s.xData.size(), s.yData.size());
		for (int i = 0; i < n; ++i)
		{
			if (s.isPointHidden(i))
				continue;
			const double y = s.yData.at(i);
			if (!qIsFinite(y))
				continue;
			if (best < 0 || s.yRef.applyTo(y) > s.yRef.applyTo(s.yData.at(best)))
				best = i;
		}
		return best;
	}

	//标签模板解析：替换来源占位符 {x|y.MXFilePath/FileName/Subcase/Datatype/
	//Request/Component} 与统计占位符 {max(y)}（Y 最大值）、{x}
	//（最大值所在点 X）；两者均按 x/y 侧 缩放系数->偏移量 变换后的显示值取
	//（与画面同源）。数值 'g'6 位有效数字（与
	//读数一致）；占位符对应来源缺失时替换为空（同绘图选项标题空值跳过），
	//未知占位符原样保留
	//单个占位符 token -> 显示值（标签模板与表头模板共用）：支持 <x|y.>来源字段
	//（MXFilePath/FileName/Subcase/Datatype/Request/Component）、统计占位符
	//{max(y)}（Y 最大值）与 {x}（最大值所在点 X）；均按 x/y 侧 缩放系数->偏移量
	//变换后的显示值取（与画面同源）。数值 'g'6 位有效数字（与
	//读数一致）；占位符对应来源缺失时替换为空（同绘图选项标题空值跳过），
	//未知 token 返回 false（调用方原样保留）
	static bool resolveLabelToken(const QString& tok, const CurveSeriesData& s, QString* val)
	{
		QString v;
		bool known = false;
		const bool xSide = tok.startsWith(QLatin1String("x."), Qt::CaseInsensitive);
		if (xSide || tok.startsWith(QLatin1String("y."), Qt::CaseInsensitive))
		{
			known = true;
			const CurveLabelSideSource src = curveLabelSideSource(s, xSide);
			const QString field = tok.mid(2);
			if        (field.compare(QLatin1String("MXFilePath"),   Qt::CaseInsensitive) == 0)
				v = s.sourceFile;
			else if (field.compare(QLatin1String("MXFileName"),   Qt::CaseInsensitive) == 0)
				v = QFileInfo(s.sourceFile).fileName();
			else if (field.compare(QLatin1String("MXSubcase"),    Qt::CaseInsensitive) == 0)
				v = src.subcase;
			else if (field.compare(QLatin1String("MXDatatype"),   Qt::CaseInsensitive) == 0)
				v = src.datatype;
			else if (field.compare(QLatin1String("MXRequest"),    Qt::CaseInsensitive) == 0)
				v = src.request;
			else if (field.compare(QLatin1String("MXComponent"),  Qt::CaseInsensitive) == 0)
				v = src.component;
			else
				known = false;
		}
		else if (tok.compare(QLatin1String("max(y)"), Qt::CaseInsensitive) == 0)
		{
			known = true;
			const int imax = finiteMaxYIndex(s);
			if (imax >= 0)
				v = QString::number(s.yRef.applyTo(s.yData.at(imax)), 'g', 6);
		}
		else if (tok.compare(QLatin1String("x"), Qt::CaseInsensitive) == 0)
		{
			known = true;
			const int imax = finiteMaxYIndex(s);
			if (imax >= 0)
				v = QString::number(s.xRef.applyTo(s.xData.at(imax)), 'g', 6);
		}
		if (known && val)
			*val = v;
		return known;
	}

	static QString resolveCurveLabelTemplate(const QString& tmpl, const CurveSeriesData& s)
	{
		QString out;
		out.reserve(tmpl.size());
		for (int i = 0; i < tmpl.size();)
		{
			if (tmpl.at(i) == QLatin1Char('{'))
			{
				const int close = tmpl.indexOf(QLatin1Char('}'), i);
				if (close > i)
				{
					QString val;
					if (resolveLabelToken(tmpl.mid(i + 1, close - i - 1), s, &val))
					{
						out += val;
						i = close + 1;
						continue;
					}
				}
			}
			out += tmpl.at(i);
			++i;
		}
		return out;
	}

	//p<w>c 位置引用 -> 曲线节点：编号规则同 nodeDisplayText 的位置前缀
	//（页序=根下 Page 序、视口序=页下 Chart 序、曲线序=图表下 Curve 序，均 1 基，
	//与"定义曲线"表达式引用同一套）。ref 形如 "p1w2c3"，restOut 输出其后 '.'
	//起的剩余部分（如 "y.MXRequest"）；格式不符/引用越界返回 nullptr
	static const CurveNode* findCurveByPositionRef(const QString& ref, QString* restOut)
	{
		//手工解析 p<数>w<数>c<数>，随后必须是 '.'
		int i = 0;
		int nums[3] = { 0, 0, 0 };
		const QLatin1Char seps[3] = { QLatin1Char('p'), QLatin1Char('w'), QLatin1Char('c') };
		for (int k = 0; k < 3; ++k)
		{
			if (i >= ref.size() || ref.at(i) != seps[k])
				return nullptr;
			++i;
			int v = 0, digits = 0;
			while (i < ref.size() && ref.at(i).isDigit())
			{
				v = v * 10 + ref.at(i).digitValue();
				++i;
				++digits;
			}
			if (digits == 0)
				return nullptr;
			nums[k] = v;
		}
		if (i >= ref.size() || ref.at(i) != QLatin1Char('.'))
			return nullptr;
		*restOut = ref.mid(i + 1);

		CurveDocument* doc = mCurveDocController::getInstance()->document();
		auto nthChild = [doc](int parentId, CurveNodeType type, int n) -> int
		{
			for (int childId : doc->childNodeIds(parentId))
			{
				if (doc->nodeType(childId) == type && --n == 0)
					return childId;
			}
			return -1;
		};
		const int pageId = nthChild(doc->rootNodeId(), CurveNodeType::Page, nums[0]);
		if (pageId < 0)
			return nullptr;
		const int chartId = nthChild(pageId, CurveNodeType::Chart, nums[1]);
		if (chartId < 0)
			return nullptr;
		const int curveId = nthChild(chartId, CurveNodeType::Curve, nums[2]);
		return curveId < 0 ? nullptr : doc->node(curveId);
	}

	//解析文本收尾：去尾部悬空分隔符（来源字段解析为空时模板中的"-"会留在
	//尾部，如表头"{y.MXRequest} - {y.MXComponent}"解析为"Curve 1 - "）
	static QString trimTrailingSeparators(QString text)
	{
		static const QRegularExpression re(
			QStringLiteral("[\\s\\-\u2013\u2014]+$"));
		return text.remove(re);
	}

	//图表条带文本/轴标题模板解析（标签模板的超集）：token 带 p<w>c 位置引用时
	//（如{p1w1c1.y.MXRequest}）按引用曲线解析其来源字段——引用写在树上的位置序，
	//图表增删后按当前结构动态解析；不带引用的 token 回退首条可见曲线
	//（fallback 为空时原样保留 token，与未知占位符同约定）
	static QString resolveBandTemplate(const QString& tmpl, const CurveSeriesData* fallback)
	{
		QString out;
		out.reserve(tmpl.size());
		for (int i = 0; i < tmpl.size();)
		{
			if (tmpl.at(i) == QLatin1Char('{'))
			{
				const int close = tmpl.indexOf(QLatin1Char('}'), i);
				if (close > i)
				{
					const QString tok = tmpl.mid(i + 1, close - i - 1);
					QString rest;
					const CurveNode* refCurve = findCurveByPositionRef(tok, &rest);
					QString val;
					if (refCurve && resolveLabelToken(rest, refCurve->series, &val))
					{
						out += val;
						i = close + 1;
						continue;
					}
					if (!refCurve && fallback && resolveLabelToken(tok, *fallback, &val))
					{
						out += val;
						i = close + 1;
						continue;
					}
					}
				}
				out += tmpl.at(i);
				++i;
			}
			return trimTrailingSeparators(out);
		}

		//图表内首条可见曲线的来源数据（表头/轴标题模板中无引用占位符的解析上下文）；
		//无可见图例返回空
	static const CurveSeriesData* firstVisibleCurveSeries(CurveDocument* doc, int chartId)
	{
		for (int childId : doc->childNodeIds(chartId))
		{
			const CurveNode* n = doc->node(childId);
			if (n && n->type == CurveNodeType::Curve && n->series.visible)
				return &n->series;
		}
		return nullptr;
	}

	//标签 -> 图例文本：未勾选"显示标签"返回空（配合 legendFallbackToName=false
	//让图例文字消失，不回退曲线名）；勾选时解析前缀（受"显示标签前缀"）/内容/
	//后缀（受"显示标签后缀"），解析为空的部分跳过，"-"连接（同绘图选项标题
	//设置 spliceTitle 的拼装效果）；全空亦返回空（makeCurveSettings 同步关闭
	//回退，图例留空不回退曲线名）
	static QString composeCurveLegendText(const CurveSeriesData& s)
	{
		if (!s.labelVisible)
			return QString();
		QStringList parts;
		if (s.labelPrefixVisible && !s.labelPrefix.isEmpty())
			parts << resolveCurveLabelTemplate(s.labelPrefix, s);
		if (!s.labelText.isEmpty())
			parts << resolveCurveLabelTemplate(s.labelText, s);
		if (s.labelSuffixVisible && !s.labelSuffix.isEmpty())
			parts << resolveCurveLabelTemplate(s.labelSuffix, s);
		parts.removeAll(QString());
		return trimTrailingSeparators(parts.join(QLatin1Char('-')));
	}

	//符号基准大小（px）：大圆点(样式1，实心大圆) 12，其余（含小点样式9） 8；
	//实际大小 = 基准 × 缩放尺寸（新建曲线 缩放尺寸=1.0 即按基准大小渲染）
	static int baseSymbolSize(int symbolStyle)
	{
		return symbolStyle == 1 ? 12 : 8;
	}

	//曲线模型数据 -> 引擎曲线样式（XY 曲线形态；柱状/复数路径在此基础上覆写）。
	//engineKey：本节点本次下发的引擎键（R23 曲线允许重名，键可≠名）——键≠名
	//（重名碰撞）且无标签文本时图例兜底显示模型名，图例文本优先级 =
	//标签文本 > 模型名 > 键名（键名兜底由引擎 legendFallbackToName 承担）；
	//无碰撞（键=名）不写 legendText，既有留空回退语义不变。引擎侧
	//CurveSettings::name 存键（getCurveSettings 回读/图例回退名一致）
	static CurveSettings makeCurveSettings(const CurveSeriesData& s,
		const QString& engineKey = QString())
	{
		CurveSettings cs;
		cs.name = engineKey.isEmpty() ? s.name : engineKey;
		//曲线标签（设计图03.1）：图例文本 = 解析后的标签文本（前缀/内容/后缀，
		//受各自"显示"勾选控制）；解析全空（如清空了标签内容）或取消"显示标签"
		//= 图例文字消失（legendFallbackToName=false：不回退曲线名，图例只留线样图标）
		cs.legendText = composeCurveLegendText(s);
		//重名碰撞且标签未提供文本：图例显示模型名（键名 "#2" 后缀不示人）。
		//"显示标签"关闭仍视为用户明确要求无文字，不兜底（既有勾选语义不变）
		if (cs.legendText.isEmpty() && s.labelVisible
			&& !engineKey.isEmpty() && engineKey != s.name)
			cs.legendText = s.name;
		cs.legendFallbackToName = s.labelVisible && !cs.legendText.isEmpty();
		cs.lineColor = s.color;
		cs.lineWidth = s.lineWidth;
		cs.linePatternType = static_cast<TecLinePattern>(s.lineStyle);
		cs.showLine = true;
		cs.showSymbol = (s.symbolStyle > 0);
		cs.symbolStyle = symbolStyleToQwt(s.symbolStyle);
		cs.symbolColor = s.symbolColor;
		//符号实际大小 = 基准大小 × 缩放尺寸（密度 N = 每 N 点画一个 → 引擎 skip N-1）
		cs.symbolSize = qMax(1, qRound(baseSymbolSize(s.symbolStyle) * s.symbolScale));
		cs.symbolSkip = qMax(0, s.symbolDensity - 1);
		//区域填充（设计图03.1）：填充色随线色，透明度/基准值用 PlotView 默认（0.3 / Y=0）
		cs.showFill = s.areaFill;
		cs.fillColor = s.color;
		return cs;
	}

	mCurveChartView::mCurveChartView(int chartId, QWidget* parent)
		: QWidget(parent)
		, _chartId(chartId)
	{
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		//页眉/页脚条带由 PlotView 内部提供（绘图区上方/下方，空文本自动隐藏）；
		//本视图只按模型下发文本/字体/对齐
		_plot = new PlotView::mPlotView(this);
		//画布外（轴刻度数字所在的边距区）默认是窗口灰底，这里显式刷白；
		//rebuild() 时随图表背景色 kBackgroundColor 同步重设
		_plot->setMarginBackground(Qt::white);
		//视图自身的边距区（右侧副轴条带所在的外扩区）同步刷白，与引擎边距区
		//同色；rebuild() 时同样随图表背景色重设
		setAutoFillBackground(true);
		QPalette viewPal = palette();
		viewPal.setColor(QPalette::Window, Qt::white);
		setPalette(viewPal);
		layout->addWidget(_plot, 1);
		setLayout(layout);

		//刻度文本 'g'4 格式（四根轴统一安装：下/左主轴 + 上/右副轴）
		if (QwtPlot* qwt = _plot->getQwtPlot())
		{
			qwt->setAxisScaleDraw(QwtPlot::xBottom, new mCurveScaleDraw);
			qwt->setAxisScaleDraw(QwtPlot::yLeft, new mCurveScaleDraw);
			qwt->setAxisScaleDraw(QwtPlot::xTop, new mCurveScaleDraw);
			qwt->setAxisScaleDraw(QwtPlot::yRight, new mCurveScaleDraw);

			//上/右轴完全不显示（无主干线、无刻度、无数字）；下/左轴隐藏
			//主干线但保留刻度与数字。闭合矩形边框由画布上的 BorderOverlay
			//绘制：Qwt 6.2 的轴主干线受 borderDist（随首尾刻度标签宽度变化，
			//且布局每次激活都会按标签提示重设，构造期清零无效）、
			//canvasMargin/spacing 影响，公开 API 拼不出闭合矩形
			for (int axis : { QwtPlot::xTop, QwtPlot::yRight })
			{
				if (QwtAbstractScaleDraw* sd = qwt->axisScaleDraw(axis))
				{
					sd->enableComponent(QwtAbstractScaleDraw::Backbone, false);
					sd->enableComponent(QwtAbstractScaleDraw::Ticks, false);
					sd->enableComponent(QwtAbstractScaleDraw::Labels, false);
				}
			}
			for (int axis : { QwtPlot::xBottom, QwtPlot::yLeft })
			{
				if (QwtAbstractScaleDraw* sd = qwt->axisScaleDraw(axis))
					sd->enableComponent(QwtAbstractScaleDraw::Backbone, false);
			}

			//间距清零：让下/左轴的刻度与数字尽量贴近边框
			qwt->plotLayout()->setCanvasMargin(0);
			qwt->plotLayout()->setSpacing(0);

			//闭合边框覆盖层与画布事件过滤统一由 ensureCanvasFilter 挂接
			//（画布可能被引擎 setUseOpenGL 重建，届时在其中重建覆盖层）
			ensureCanvasFilter();

			//基准线模式悬停读数项（纯视图临时元素，本视图持有，析构随 detach）
			_baselineReadout = new mCurveReadoutItem;
			_baselineReadout->attach(qwt);

			//纵/横轴标题双击（设计图05/06）：轴标题画在 QwtScaleWidget（画布之外），
			//画布过滤器覆盖不到，对左轴/下轴控件单独挂过滤器；上/右轴（副轴原生轴）
			//同样挂过滤器：双击标题弹轴编辑器 + 右键轴区菜单（多坐标轴功能）
			for (int axis : { QwtPlot::yLeft, QwtPlot::xBottom, QwtPlot::xTop, QwtPlot::yRight })
			{
				if (QwtScaleWidget* scaleWgt = qwt->axisWidget(axis))
				{
					scaleWgt->installEventFilter(this);
					scaleWgt->setMouseTracking(true);   //轴区 hover 高亮
				}
			}

			//轴区 hover 高亮覆盖层（浅蓝、鼠标穿透）：显示在各轴控件/条带之上
			_zoneHighlight = new QWidget(this);
			_zoneHighlight->setObjectName(QStringLiteral("curveZoneHighlight"));
			_zoneHighlight->setAttribute(Qt::WA_TransparentForMouseEvents);
			_zoneHighlight->setAttribute(Qt::WA_NoSystemBackground);
			_zoneHighlight->setStyleSheet(QStringLiteral("background: rgba(43, 125, 233, 40);"));
			_zoneHighlight->hide();
		}

		//单击可激活所在 MDI 视口：QMdiSubWindow 依靠应用焦点变化激活子窗口，
		//被点击的控件必须接受焦点才行。曲线视图/Qwt 画布默认 NoFocus（对比云图
		//视口 PostViewportWidget 给渲染表面的 StrongFocus），导致点击视口无法
		//选中/切换、只有新建时 setFocus 过的那个保持激活。整棵子树设 ClickFocus
		setFocusPolicy(Qt::ClickFocus);
		_plot->setFocusPolicy(Qt::ClickFocus);
	if (QwtPlot* qwt = _plot->getQwtPlot())
	{
		qwt->setFocusPolicy(Qt::ClickFocus);
		if (QWidget* canvas = qwt->canvas())
			canvas->setFocusPolicy(Qt::ClickFocus);
		//轴控件同样接受点击焦点：双击轴标题/点击轴区时所在 MDI 视口可被激活
		//（QMdiSubWindow 依靠焦点变化激活，见上方整棵子树设 ClickFocus 的说明）；
		//上/右轴控件一并设置（副轴启用后同样可点击激活视口）
		for (int axis : { QwtPlot::xBottom, QwtPlot::yLeft, QwtPlot::xTop, QwtPlot::yRight })
			if (QWidget* scaleWgt = qwt->axisWidget(axis))
				scaleWgt->setFocusPolicy(Qt::ClickFocus);
	}

		//画布外框（对齐原型观感）：必须走 PlotView 公开接口——
		//PlotView 可能使用 OpenGL 画布（QwtPlotGLCanvas 继承 QOpenGLWidget 而非 QFrame），
		//手工 static_cast<QFrame*>(canvas()) 会写坏控件内存在首次绘制时崩溃
		_plot->setFrameStyle(QFrame::Box);

	//图例字体加粗对齐原型：随 applyLegend 统一下发（模型字体+加粗），此处不再单独设置

		mCurveDocController* controller = mCurveDocController::getInstance();
		connect(controller, &mCurveDocController::sig_nodeAdded, this, &mCurveChartView::onNodeAdded);
		connect(controller, &mCurveDocController::sig_nodeRemoved, this, &mCurveChartView::onNodeRemoved);
		connect(controller, &mCurveDocController::sig_nodePropertyChanged, this, &mCurveChartView::onNodePropertyChanged);

		//注释模式：Ribbon"注释"按钮 -> 视口工厂广播 -> 本视图开关
		connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_annotationModeChanged,
			this, &mCurveChartView::setAnnotationMode);

		//基准线模式：Ribbon"基准线"下拉 -> 视口工厂广播 -> 本视图开关/切向
		connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_baselineModeChanged,
			this, &mCurveChartView::setBaselineMode);
		//编辑曲线会话：Ribbon"编辑曲线"按钮 -> 视口工厂广播（目标视窗建立
		//裁剪线/压暗覆盖层，范围/目标变化同步刷新）
		mCurveViewportFactory* viewportFactory = mCurveViewportFactory::getInstance();
		connect(viewportFactory, &mCurveViewportFactory::sig_editCurveStarted,
			this, &mCurveChartView::onEditCurveStarted);
		connect(viewportFactory, &mCurveViewportFactory::sig_editCurveTargetsChanged,
			this, &mCurveChartView::onEditCurveTargetChanged);
		connect(viewportFactory, &mCurveViewportFactory::sig_editCurveRangeChanged,
			this, &mCurveChartView::onEditCurveRangeChanged);
		connect(viewportFactory, &mCurveViewportFactory::sig_editCurveDirectionChanged,
			this, &mCurveChartView::onEditCurveDirectionChanged);
		connect(viewportFactory, &mCurveViewportFactory::sig_editCurveStopped,
			this, &mCurveChartView::onEditCurveStopped);

		//注释吸附统一走引擎 snapper（仅吸附模式：不画标签/不建标记，半径=原
		//30px；悬停预览与创建锚点均由其信号驱动），注释模式启停时随之启停
		if (PlotView::mPlotSnapper* snapper = _plot->getPlotSnapper())
		{
			snapper->setMode(PlotView::mPlotSnapper::SnapOnly);
			snapper->setSnapRadius(kAnnotationSnapPx);
			connect(snapper, &PlotView::mPlotSnapper::snapHoverChanged,
				this, &mCurveChartView::onSnapHover);
		}

		//PlotView 的信息/警告/错误 -> 控制器上报通道（主窗口接到状态栏）
		connect(_plot, &PlotView::mPlotView::messageInfo, this, [this](const QString& msg)
		{
			mCurveDocController::getInstance()->reportInfo(msg);
		});
		connect(_plot, &PlotView::mPlotView::messageWarning, this, [this](const QString& msg)
		{
			mCurveDocController::getInstance()->reportWarning(msg);
		});
		connect(_plot, &PlotView::mPlotView::messageError, this, [this](const QString& msg)
		{
			mCurveDocController::getInstance()->reportError(msg);
		});

		//用户平移/缩放（引擎手势期间逐步广播）：同步自绘副轴条带刻度跟随
		connect(_plot, &PlotView::mPlotView::signalViewChanged,
			this, &mCurveChartView::onViewRangeChanged);

	//撤销/重做快捷键（图表获得焦点时生效）
	QShortcut* undoShortcut = new QShortcut(QKeySequence::Undo, this);
	connect(undoShortcut, &QShortcut::activated, this, []()
	{
		mCurveDocController::getInstance()->undo();
	});
	QShortcut* redoShortcut = new QShortcut(QKeySequence::Redo, this);
	connect(redoShortcut, &QShortcut::activated, this, []()
	{
		mCurveDocController::getInstance()->redo();
	});

	//--- 曲线选中/图例交互（设计图03.1） ---
	//关闭引擎"单击图例项隐藏曲线"（引擎内部状态，不回写模型且被 rebuild 冲掉，
	//按要求取消），改为 legendItemClicked 信号 -> 选中联动；
	//画布双击的样式窗口路径一并关闭（本模块未绑定样式窗口，避免无效告警）
	_plot->setLegendClickTogglesEnable(false);
	_plot->setDoubleClickEnabled(false);

	//画布事件过滤（后装先执行：先于引擎过滤器看到画布事件）
	ensureCanvasFilter();

	//图例项单击 -> 选中对应曲线（变蓝/树联动经选择中枢闭环）。信号携带引擎键
	//（图例项身份=键，显示文本可经 legendText 覆盖），经映射反查节点（R23）
	connect(_plot, &PlotView::mPlotView::legendItemClicked, this, [this](const QString& name)
	{
		selectCurve(engineNodeByKey(name));
	});
	//图例项双击 -> 浮动属性编辑器（视窗左上角）
	connect(_plot, &PlotView::mPlotView::legendItemDoubleClicked, this, [this](const QString& name)
	{
		int curveId = engineNodeByKey(name);
		if (curveId > 0)
			emit sig_requestEditorPopup(curveId, mapToGlobal(QPoint(12, 12)));
	});
	//图内-用户自定义图例拖拽结束 -> 悬浮位置回写模型（posX/posY，画布百分比
	//0-100）。不回写的话，之后任何触发图例重应用的操作（改属性/重选节点等）
	//都会按模型里的旧坐标把图例拉回拖拽前的位置
	connect(_plot, &PlotView::mPlotView::legendFloatingMoved, this, [this](double pctX, double pctY)
	{
		const int legendId = legendNodeId();
		if (legendId <= 0)
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		if (!controller->canSelectNode(legendId))
			return;
		controller->setNodeProperty(legendId, CurveProp::kPosX, pctX * 100.0);
		controller->setNodeProperty(legendId, CurveProp::kPosY, pctY * 100.0);
	});
	//画布右键（含图例区域）：命中图例项/曲线时弹上下文菜单。下面板画布的
	//ContextMenu 由引擎过滤器以"下面板画布局部"坐标透传本信号，上面板则是
	//"上面板画布局部"——统一由 globalPos 反算上面板画布坐标后再命中，消除
	//双坐标系歧义（此前下面板右键按上面板坐标命中必然 miss -> 落成图表菜单
	//并顺带取消曲线选中）
	connect(_plot, &PlotView::mPlotView::customContextMenuRequestedAt,
		this, [this](const QPoint& pos, const QPoint& globalPos)
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		QWidget* upperCanvas = qwt ? qwt->canvas() : nullptr;
		onCanvasContextMenu(upperCanvas ? upperCanvas->mapFromGlobal(globalPos) : pos,
			globalPos);
	});
	//全局选中变化：树上/其他视口选中曲线时，本视口同步高亮或清除
	connect(controller, &mCurveDocController::sig_currentNodeChanged,
		this, &mCurveChartView::onCurrentNodeChanged);

	rebuild();
}

	mCurveChartView::~mCurveChartView()
	{
		//先摘除上/下面板画布的事件过滤器再放行引擎（子对象）析构：下面板销毁
		//过程中 Qt 会向其派发 Hide/Leave 等销毁期事件，过滤逻辑若此时运行，
		//applyHighlightVisuals 会对销毁中的下面板 replot（写访问违例）
		if (_plot)
		{
			if (QwtPlot* qwt = _plot->getQwtPlot())
			{
				if (qwt->canvas())
					qwt->canvas()->removeEventFilter(this);
			}
			if (QwtPlot* lowerQwt = _plot->getLowerQwtPlot())
			{
				if (lowerQwt->canvas())
					lowerQwt->canvas()->removeEventFilter(this);
			}
			//页眉/页脚条带过滤器一并摘除（销毁期防御，同画布；分支本身只认
			//MouseButtonDblClick，此处为对称保险）
			if (QWidget* w = _headerBandWgt.data())
				w->removeEventFilter(this);
			if (QWidget* w = _footerBandWgt.data())
				w->removeEventFilter(this);
		}
		//注释渲染项由本视图自管（QwtPlot 不拥有 attach 的 item），
		//先于引擎画布销毁前删除（析构即自动 detach）
		clearAnnotationMarkers();
		hideAnnotationHover();
		delete _hoverItem;
		_hoverItem = nullptr;
		//悬停读数项同为自管（QwtPlot 不拥有 attach 的 item）
		delete _baselineReadout;
		_baselineReadout = nullptr;
		//基准线标签图元同为自管，随析构一并摘除
		clearBaselineLabels();
		//柱顶标注图元同为自管，随析构一并摘除
		clearBarLabelItem();
		//极坐标画布接管图元同为自管，随析构一并摘除
		clearPolarItems();
		//频率红线游标同为自管，随析构一并摘除（不 replot，析构期静默摘除）
		delete _freqCursorTop;
		_freqCursorTop = nullptr;
		delete _freqCursorBottom;
		_freqCursorBottom = nullptr;
		//编辑曲线覆盖层（截取段曲线/裁剪线）同为旁路自管，一并先于引擎销毁
		clearEditOverlay();
		//副轴自绘条带同为视图自管（布局边距随析构失效，先于引擎销毁）
		clearAxisStrips();
	}

	//画布曲线命中（上/下面板通用）：先试上面板；复数模式下再试下面板（相位/
	//虚部侧）。下面板命中换算：调用方 pos 为上面板画布局部坐标（与 hitTestCurve
	//的 canvasMap 约定一致），经两画布 mapFromGlobal 换到下面板画布局部再调
	//hitTestLowerCurve；命中的名字即复数曲线节点名（两面板同键），选中/hover/
	//双击/右键与上面板同语义
	bool mCurveChartView::hitCurveAnyPanel(const QPoint& pos, QString& name, double maxDistPx) const
	{
		name.clear();
		if (!_plot)
			return false;
		if (_plot->hitTestCurve(pos, name, maxDistPx))
			return true;
		if (!_plot->isComplexMode())
			return false;
		QwtPlot* lowerQwt = _plot->getLowerQwtPlot();
		const QWidget* lowerCanvas = lowerQwt ? lowerQwt->canvas() : nullptr;
		const QWidget* upperCanvas = _plot->getQwtPlot() ? _plot->getQwtPlot()->canvas() : nullptr;
		if (!lowerCanvas || !upperCanvas)
			return false;
		const QPoint lowerPos = lowerCanvas->mapFromGlobal(upperCanvas->mapToGlobal(pos));
		return _plot->hitTestLowerCurve(lowerPos, name, maxDistPx);
	}

	bool mCurveChartView::eventFilter(QObject* watched, QEvent* event)
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		//下面板轴控件（复数模式，ensureCanvasFilter 于面板就位后补装）与上面板
		//轴区同走轴区分支（右键轴区菜单/hover 高亮）；画布分支不含轴控件，先行判定
		QwtPlot* lowerPlot = _plot ? _plot->getLowerQwtPlot() : nullptr;
		const bool fromLowerYAxis = lowerPlot && watched == lowerPlot->axisWidget(QwtPlot::yLeft);
		const bool fromLowerXAxis = lowerPlot && watched == lowerPlot->axisWidget(QwtPlot::xBottom);
	if (qwt && (watched == qwt->canvas()
		|| (_plot && _plot->getLowerQwtPlot() && watched == _plot->getLowerQwtPlot()->canvas())))
	{
		//图例区域的事件（含拖拽/单击/双击）留给引擎处理（其信号驱动图例联动），
		//这里只管曲线、注释与空白。复数下面板画布事件同入本过滤：鼠标坐标先经
		//normPos 归一到上面板画布局部（曲线命中 hitCurveAnyPanel 的坐标约定；
		//图例/注释/基准线/轴标题仅上面板有——下面板事件归一后必然越界 miss）
		QwtPlot* lowerQwt = _plot ? _plot->getLowerQwtPlot() : nullptr;
		const bool fromLower = lowerQwt && watched == lowerQwt->canvas();
		//下面板画布响应鼠标交互与 Enter/Leave（hover 进出清选）；Hide/Destroy 等
		//销毁期事件一概放行不过滤（防止对销毁中的下面板做命中/重绘；
		//视图析构已先摘双画布过滤器，此处为二次保险）
		if (fromLower && !(event->type() == QEvent::MouseMove
			|| event->type() == QEvent::Enter
			|| event->type() == QEvent::Leave
			|| event->type() == QEvent::MouseButtonPress
			|| event->type() == QEvent::MouseButtonRelease
			|| event->type() == QEvent::MouseButtonDblClick))
			return false;
		QWidget* upperCanvas = qwt->canvas();
		QWidget* lowerCanvas = fromLower ? lowerQwt->canvas() : nullptr;
		const auto normPos = [&](const QPoint& p) -> QPoint
		{
			return fromLower
				? upperCanvas->mapFromGlobal(lowerCanvas->mapToGlobal(p)) : p;
		};
		PlotView::mPlotLegendItem* legend = _plot->getPlotLegendItem();

			switch (event->type())
			{
			case QEvent::Resize:
				//画布尺寸变化（Qwt 布局/窗口变化/轴启用隐藏）：边框覆盖层与副轴
				//条带的重排统一推迟到事件循环（scheduleAxisStripLayout 去重合并）。
				//不能在此同步处理：本 Resize 可能是 applyAxis 内 enableAxis 在布局
				//激活中同步派发的，此时同步改画布子控件几何/布局边距会重入导致崩溃
				scheduleAxisStripLayout();
				//柱几何随画布定型重算（D4：applySeries 落在布局前 canvas 仅 ~80px，
				//针状柱根因；同 0ms 去重通道延迟到事件循环）
				scheduleBarGeometryRefresh();
				break;

			//注释/基准线模式内按 Esc：先接受 ShortcutOverride 抑制全局 Esc 快捷键
			//（主窗口"清空全部拾取"为 WindowShortcut 上下文，会截走按键），
			//使按键以普通 KeyPress 送达画布（画布 ClickFocus，模式内左键创建
			//注释/基准线时已获得焦点）；消费事件避免画布默认处理把 accept 复位
			case QEvent::ShortcutOverride:
				if ((_annotationMode || _baselineMode)
					&& static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
				{
					event->accept();
					return true;
				}
				break;

			//注释/基准线模式：Esc 退出激活（经工厂广播退出：各视口停 snapper/
			//收注释预览与基准线读数，Ribbon 两按钮按下态随各自模式信号同步；
			//两模式互斥，按当前激活者退出）
			case QEvent::KeyPress:
				if ((_annotationMode || _baselineMode)
					&& static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
				{
					mCurveViewportFactory* factory = mCurveViewportFactory::getInstance();
					if (_baselineMode)
						factory->setBaselineMode(false, _baselineOrientation);
					else
						factory->setAnnotationMode(false);
					return true;
				}
				break;

			//极坐标滚轮缩放（2026-11-12 新增交互；2026-11-16 D35 商软对齐重
			//裁定，五样张实证）：半径有效域乘子缩放（视图态，不动模型）。本
			//过滤器后装先执行（ensureCanvasFilter 注释在案），在此消费掉滚轮
			//即不进引擎 magnifier——直角轴缩放对极坐标画布不可见，空转且会被
			//引擎 syncAxisSettingsFromPlot 当作轴范围变化污染状态同步。缩放
			//档位（D35）：每档域 ×0.8（滚轮放大）/×1.2（滚轮缩小）连乘无极态
			//（商软可缩至向量不可见、放至向量穿出画布，环标签始终正常）；界
			//钳 [1e-6,1e6] 系浮点溢出防御（商软本身无界）。用户缩放态不写
			//模型（Y1 autoScale/min/max 保持），重建不保视图时复位
			case QEvent::Wheel:
				if (isPolarChart() && !fromLower)
				{
					const QWheelEvent* we = static_cast<QWheelEvent*>(event);
					const double notch = we->angleDelta().y() / 120.0;
					if (notch != 0.0)
					{
						const double zoomBefore = _polarRadiusZoom;
						_polarRadiusZoom = qBound(1e-6,
							(notch > 0.0 ? 0.8 : 1.2) * _polarRadiusZoom, 1e6);
						//首次离开 1.0 捕获上一帧渲染步长（D28 捕获机制保留）；
						//回到 1.0 解冻交还面板口径并复位脏标记（D35）
						if (zoomBefore == 1.0 && _polarRadiusZoom != 1.0)
							_polarZoomTickStep = _polarLastMajorStep;
						else if (_polarRadiusZoom == 1.0)
						{
							_polarZoomTickStep = 0.0;
							_polarZoomStepDirty = false;
						}
						applyPolarDecor();
						if (QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr)
							qwt->replot();
					}
					return true;
				}
				break;

			case QEvent::MouseMove:
			{
				//选中注释拖动中：实时改渲染项框位（不进模型，无撤销垃圾）；消费
				//移动，不触发 hover/十字线/图例语义
				if (_dragAnnotationId >= 0)
				{
					if (static_cast<QMouseEvent*>(event)->buttons() & Qt::LeftButton)
					{
						moveAnnotationDrag(normPos(static_cast<QMouseEvent*>(event)->pos()));
						return true;
					}
					_dragAnnotationId = -1;   //左键已放开（防御，正常走 Release）
				}
				//编辑曲线：拖拽中的裁剪线实时更新范围（工厂钳制+广播：视窗线/
				//截取段与浮动窗口输入同步刷新）；悬停裁剪线切换拖拽光标。
				//消费拖拽事件，不触发选中/hover/图例语义
				if (_editMode)
				{
					QMouseEvent* me = static_cast<QMouseEvent*>(event);
					mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
					if (_editDragLine >= 0)
					{
						if (me->buttons() & Qt::LeftButton)
						{
							const double v = editAxisPixelToValue(normPos(me->pos()));
							if (_editDragLine == 0)
								f->setEditRange(v, f->editEnd());
							else
								f->setEditRange(f->editStart(), v);
							return true;
						}
						_editDragLine = -1;   //左键已放开（防御，正常走 Release）
					}
					if (QWidget* cvs = qwt->canvas())
					{
						if (hitEditTrimLine(normPos(me->pos())) >= 0)
						{
							cvs->setCursor(f->editDirection() == 0
								? Qt::SizeHorCursor : Qt::SizeVerCursor);
						}
						else
						{
							const Qt::CursorShape shape = cvs->cursor().shape();
							if (shape == Qt::SizeHorCursor || shape == Qt::SizeVerCursor)
								cvs->unsetCursor();
						}
					}
				}
				//注释/基准线模式：悬停反馈由引擎 snapper 吸附信号驱动（onSnapHover：
				//注释=预览框、基准线=X/Y 读数），这里跳过曲线 hover 高亮（保持模式内
				//不高亮曲线的原行为），事件放行，引擎靠移动事件处理光标形状/十字线
				if (_annotationMode || _baselineMode)
					break;
				//hover 曲线仅变主题蓝不加粗（放行：引擎靠移动事件处理光标形状/十字线）
				{
					QMouseEvent* me = static_cast<QMouseEvent*>(event);
					QString name;
					int hoverId = -1;
					if (!legend->hitTest(normPos(me->pos())) && hitCurveAnyPanel(normPos(me->pos()), name))
						hoverId = engineNodeByKey(name);   //命中键 -> 节点（重名取最近者，R23）
					if (hoverId != _hoveredCurveId)
					{
						_hoveredCurveId = hoverId;
						applyHighlightVisuals();
					}
					//hover 基准线：主题蓝+加粗1px，挪开恢复（视图状态，同曲线 hover 策略）
					const int hoverBaselineId = hitBaseline(normPos(me->pos()));
					if (hoverBaselineId != _hoveredBaselineId)
					{
						_hoveredBaselineId = hoverBaselineId;
						applyBaselineHoverVisuals();
					}
					//最小化小方块悬停：命中临时还原完整注释样式
					updateAnnotationMinimizeHover(normPos(me->pos()));
				}
				break;
			}

			case QEvent::Leave:
				if (_annotationMode)
					hideAnnotationHover();
				//基准线模式：离画布即收读数（snapper 不再发移出广播）
				if (_baselineMode)
					hideBaselineReadout();
				//最小化小方块：离画布即还原（画布外坐标必不命中）
				updateAnnotationMinimizeHover(QPoint(-1, -1));
				//离开画布必须清 hover：曲线常铺满画布宽度，沿曲线方向移出
				//画布边缘时最后一个 MouseMove 仍命中曲线，此后不再有移动
				//事件进来清它，_hoveredCurveId 残留会让曲线停在主题蓝
				//（rebuild 的重放也会沿用该残留值）；基准线同理（残留会让线
				//停在加粗高亮态）
				if (_hoveredCurveId != -1)
				{
					_hoveredCurveId = -1;
					applyHighlightVisuals();
				}
				if (_hoveredBaselineId != -1)
				{
					_hoveredBaselineId = -1;
					applyBaselineHoverVisuals();
				}
				break;

			case QEvent::MouseButtonPress:
				//编辑曲线：左键命中裁剪线 -> 进入拖拽；Ctrl+左键点曲线 -> 换目标
				//（设计图16；两路径都消费事件，避免同时触发选中/注释语义）
				if (_editMode && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
				{
					QMouseEvent* me = static_cast<QMouseEvent*>(event);
					const int trim = hitEditTrimLine(normPos(me->pos()));
					if (trim >= 0)
					{
						_editDragLine = trim;
						return true;
					}
					if (me->modifiers() & Qt::ControlModifier)
					{
						QString name;
						if (!legend->hitTest(normPos(me->pos())) && hitCurveAnyPanel(normPos(me->pos()), name))
						{
							const int curveId = engineNodeByKey(name);
							if (curveId > 0)
							{
								mCurveViewportFactory::getInstance()->setEditTargetCurve(curveId);
								return true;
							}
						}
					}
				}
				//注释模式：左键打点创建注释（拦截，避免触发图例点选/拖拽）
				if (_annotationMode && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
				{
					hideAnnotationHover();
					addAnnotationAt(normPos(static_cast<QMouseEvent*>(event)->pos()));
					return true;
				}
				//基准线模式：左键单击创建基准线（拦截；模式保持激活可连续创建，
				//纵向取X轴值/横向取Y轴值，悬停曲线时优先取吸附点）
				if (_baselineMode && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
				{
					addBaselineAt(normPos(static_cast<QMouseEvent*>(event)->pos()));
					return true;
				}
				//Shift+左键点曲线：曲线拾取广播（定义曲线弹窗"数学运算"来源
				//X/Y 向量引用的拾取通道，经视口工厂中继；消费事件避免触发
				//常规点选语义。未命中曲线则交给下方常规点选）
				{
					QMouseEvent* me = static_cast<QMouseEvent*>(event);
					if (me->button() == Qt::LeftButton
						&& (me->modifiers() & Qt::ShiftModifier)
						&& !legend->hitTest(normPos(me->pos())))
					{
						QString name;
						if (hitCurveAnyPanel(normPos(me->pos()), name))
						{
							const int pickedId = engineNodeByKey(name);
							if (pickedId > 0)
							{
								emit sig_curvePicked(pickedId);
								return true;
							}
						}
					}
				}
				//左键单击注释选中/单击曲线选中/点空白清除（不消费按下事件：画布
				//（ClickFocus）需收到点击才能激活所在 MDI 视口）
				{
					QMouseEvent* me = static_cast<QMouseEvent*>(event);
					if (me->button() == Qt::LeftButton && !legend->hitTest(normPos(me->pos())))
					{
					//注释绘制于曲线上层，先于曲线判定（选中出现主题蓝选中框，设计图15）
					const int annotationId = hitAnnotation(normPos(me->pos()));
					if (annotationId > 0)
					{
						//数据隔离：类型筛选下作用域外类型不选中（如曲线筛选模式下点注释）
						if (mCurveDocController::getInstance()->canSelectNode(annotationId))
						{
							//选中后拖动：已选中注释再按即进入拖动；未选中首按只选中
							//（防误拖），下次按下才拖。按下不消费：画布仍收点击激活视口
							if (annotationId == _selectedAnnotationId)
								startAnnotationDrag(annotationId, normPos(me->pos()));
							else
								mCurveDocController::getInstance()->setCurrentNodeId(annotationId);
						}
					}
					else
					{
						//单击基准线选中（基准线 Z50 在曲线上层，先于曲线判定；树/参数栏
						//经选择中枢联动，数据隔离同注释）
						const int baselineId = hitBaseline(normPos(me->pos()));
						if (baselineId > 0)
						{
							if (mCurveDocController::getInstance()->canSelectNode(baselineId))
								mCurveDocController::getInstance()->setCurrentNodeId(baselineId);
						}
						else
						{
							//极坐标接管画布（chartType=3）：引擎曲线为不可见图例
							//载体（直角 hitTestCurve 失真），命中走极坐标向量端
							//点查询（design D5 hit 通道：选中/图例联动沿用现有链）
							if (!isPolarChart())
							{
								QString name;
								if (hitCurveAnyPanel(normPos(me->pos()), name))
									selectCurve(engineNodeByKey(name));
								else if (_selectedCurveId != -1 || _selectedAnnotationId != -1)
									mCurveDocController::getInstance()->setCurrentNodeId(-1);
							}
						}
						//NVH 模态贡献量：画布左键取 X 吸附最近数据点，广播频率拾取
						//（面板按产出集合过滤并三路回写；视图态红线不进命令栈；
						//极坐标画布相位非频率，跳过广播）
						if (!isPolarChart())
							emit sig_canvasFrequencyPicked(
								snapToNearestDataX(qwt->canvasMap(QwtPlot::xBottom)
									.invTransform(normPos(me->pos()).x())));
					}
					}
				}
				break;

			case QEvent::MouseButtonDblClick:
				//注释/基准线模式内双击不产生编辑语义（吞掉：双击序列的第二击
				//不创建、也不进入下方的弹编辑器路径）
				if (_annotationMode || _baselineMode)
					return true;
				if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
				{
					//双击注释边框：弹出注释文本对话框（注释绘制于曲线上层，先于曲线判定）
					const int annotationId = hitAnnotation(normPos(static_cast<QMouseEvent*>(event)->pos()));
					if (annotationId > 0)
					{
						showAnnotationEditor(annotationId);
						return true;
					}
					//双击基准线：请求弹出浮动属性编辑器（视窗左上角，设计图14）；
					//基准线 Z50 在曲线上层，先于曲线判定
					const int baselineId = hitBaseline(normPos(static_cast<QMouseEvent*>(event)->pos()));
					if (baselineId > 0)
					{
						//数据隔离：作用域外类型不选中也不弹编辑器（仍消费双击）
						if (mCurveDocController::getInstance()->canSelectNode(baselineId))
						{
							mCurveDocController::getInstance()->setCurrentNodeId(baselineId);
							emit sig_requestEditorPopup(baselineId, mapToGlobal(QPoint(12, 12)));
						}
						return true;
					}
					//双击图例空白区域（非图例项，设计图08）：激活图例属性编辑器——
					//经选择中枢选中图例节点（主题蓝外框/树回选/参数栏换页由
					//sig_currentNodeChanged 闭环），再请求弹窗（视窗左上角，同曲线双击）；
					//命中图例项时不拦截，交给引擎 legendItemDoubleClicked（曲线编辑器）
					{
						QMouseEvent* me = static_cast<QMouseEvent*>(event);
						QString name;
						if (legend->hitTest(normPos(me->pos())) && !legend->hitTestItem(normPos(me->pos()), name))
						{
							const int legendId = legendNodeId();
							if (legendId > 0)
							{
								//数据隔离：作用域外类型不选中也不弹编辑器（仍消费双击）
								if (mCurveDocController::getInstance()->canSelectNode(legendId))
								{
									mCurveDocController::getInstance()->setCurrentNodeId(legendId);
									emit sig_requestEditorPopup(legendId, mapToGlobal(QPoint(12, 12)));
								}
							}
							return true;
						}
					}
					//双击曲线：请求弹出浮动属性编辑器（视窗左上角）；未命中曲线
					//且不在图例区域=双击图表空白区域 -> 图表属性编辑器（参考软件）
					QMouseEvent* me = static_cast<QMouseEvent*>(event);
					if (!legend->hitTest(normPos(me->pos())))
					{
						QString name;
						if (hitCurveAnyPanel(normPos(me->pos()), name))
						{
							int curveId = engineNodeByKey(name);
							//数据隔离：作用域外类型不弹编辑器（仍消费双击，不进引擎样式窗）
							if (curveId > 0 && mCurveDocController::getInstance()->canSelectNode(curveId))
								emit sig_requestEditorPopup(curveId, mapToGlobal(QPoint(12, 12)));
							return true;   //消费：双击曲线不再进入引擎的样式窗口路径
						}
						//双击图表空白区域：经选择中枢选中图表节点（树回选/参数栏
						//换页由 sig_currentNodeChanged 闭环），再请求弹窗（视窗左上角，
						//同图例空白双击）
					mCurveDocController::getInstance()->setCurrentNodeId(_chartId);
					emit sig_requestEditorPopup(_chartId, mapToGlobal(QPoint(12, 12)));
					return true;
				}
			}
			break;

			case QEvent::MouseButtonRelease:
				//编辑曲线：裁剪线拖拽结束（消费，避免松手触发点选/图例语义）
				if (_editMode && _editDragLine >= 0
					&& static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
				{
					_editDragLine = -1;
					return true;
				}
				//选中注释拖动结束：最终框位一次提交（撤销宏）；消费避免松手触发
				//点选语义（如拖完停在曲线上误选曲线）
				if (_dragAnnotationId >= 0
					&& static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
				{
					finishAnnotationDrag();
					return true;
				}
				break;

			default:
				break;
			}
		}
		else if ((qwt && (watched == qwt->axisWidget(QwtPlot::yLeft)
			|| watched == qwt->axisWidget(QwtPlot::xBottom)
			|| watched == qwt->axisWidget(QwtPlot::xTop)
			|| watched == qwt->axisWidget(QwtPlot::yRight)))
			|| fromLowerYAxis || fromLowerXAxis)
		{
			//--- 轴控件（QwtScaleWidget，画布之外）交互 ---
			//轴区=整个轴控件区域（截图蓝色热区）：右键=轴区菜单（复制/粘贴/添加
			//坐标轴/编辑）、hover=浅蓝高亮；标题带双击=轴属性编辑器（设计图05/06，
			//上/右轴为副轴原生轴，对称扩展）。左/下=主轴 Y1/X1；上/右=首枚副轴
			//（X2/Y2，副轴不存在时该控件不可见、收不到事件）。
			//注意：控件还会收到 Resize/LayoutRequest 等非鼠标事件（轴启用隐藏时
			//Qwt 会同步派发布局事件），QMouseEvent 转换必须限定在鼠标事件类型内，
			//否则对非鼠标事件强转读 pos() 是未定义行为
			QWidget* zoneWidget = static_cast<QWidget*>(watched);
			const bool verticalZone = (watched == qwt->axisWidget(QwtPlot::yLeft)
				|| watched == qwt->axisWidget(QwtPlot::yRight)
				|| fromLowerYAxis);
			int zoneAxisId = -1;
			if (watched == qwt->axisWidget(QwtPlot::yLeft))
				zoneAxisId = yAxisNodeId();
			else if (watched == qwt->axisWidget(QwtPlot::xBottom))
				zoneAxisId = xAxisNodeId();
			else if (watched == qwt->axisWidget(QwtPlot::xTop))
				zoneAxisId = nativeSecondaryAxisId(false);
			else if (watched == qwt->axisWidget(QwtPlot::yRight))
				zoneAxisId = nativeSecondaryAxisId(true);
			//下面板轴区：镜像上面板主 Y/X（复数图右键走图表级菜单，onAxisZoneContextMenu
			//复数分支只用图表id，轴id 仅供非复数回退分支；下面板仅在复数模式存在）
			else if (fromLowerYAxis)
				zoneAxisId = yAxisNodeId();
			else if (fromLowerXAxis)
				zoneAxisId = xAxisNodeId();
			//复数图排布映射（HyperGraph）：上面板轴区=上面板轴节点（默认相位/
			//虚部侧 Y2，互换态幅值/实部侧 Y1），下面板 Y 轴区=下面板轴节点——
			//双击标题开对应节点编辑器/选中联动随之；Y1 节点标题=数据类型走中缝
			//标签位，不占面板标题
			CurveNode* zoneChart = mCurveDocController::getInstance()->document()->node(_chartId);
			if (zoneChart && zoneChart->chart.chartType == 2)
			{
				int topAxisId = -1, bottomAxisId = -1;
				complexPanelAxisIds(&topAxisId, &bottomAxisId);
				if (watched == qwt->axisWidget(QwtPlot::yLeft))
					zoneAxisId = topAxisId;
				else if (fromLowerYAxis)
					zoneAxisId = bottomAxisId;
			}
			switch (event->type())
			{
			case QEvent::MouseButtonDblClick:
			{
				//标题带命中 -> 激活轴属性编辑器：经选择中枢选中轴节点（标题主题蓝+
				//加粗、树回选、参数栏换轴页均由 sig_currentNodeChanged 闭环），再请求
				//弹窗（视窗左上角，同曲线双击）
				QMouseEvent* me = static_cast<QMouseEvent*>(event);
				bool titleHit = false;
				if (watched == qwt->axisWidget(QwtPlot::yLeft))
					titleHit = hitYAxisTitle(me->pos());
				else if (watched == qwt->axisWidget(QwtPlot::xBottom))
					titleHit = hitXAxisTitle(me->pos());
				else if (watched == qwt->axisWidget(QwtPlot::xTop))
					titleHit = hitTopAxisTitle(me->pos());
				else if (watched == qwt->axisWidget(QwtPlot::yRight))
					titleHit = hitRightAxisTitle(me->pos());
				//下面板轴区标题命中：几何/标题取下面板控件（面板分量名），双击
				//选中对应轴节点并弹编辑器（与上面板同款交互）
				else if (fromLowerYAxis)
					titleHit = hitLowerYAxisTitle(me->pos());
				else if (fromLowerXAxis)
					titleHit = hitLowerXAxisTitle(me->pos());
				if (me->button() == Qt::LeftButton && titleHit && zoneAxisId > 0)
				{
					if (mCurveDocController::getInstance()->canSelectNode(zoneAxisId))
					{
						mCurveDocController::getInstance()->setCurrentNodeId(zoneAxisId);
						emit sig_requestEditorPopup(zoneAxisId, mapToGlobal(QPoint(12, 12)));
					}
					return true;
				}
				break;
			}
			case QEvent::MouseButtonPress:
				//右键 -> 轴区菜单（多坐标轴功能：添加坐标轴/编辑等）
				if (static_cast<QMouseEvent*>(event)->button() == Qt::RightButton && zoneAxisId > 0)
				{
					onAxisZoneContextMenu(zoneAxisId, verticalZone,
						static_cast<QMouseEvent*>(event)->globalPos());
					return true;
				}
				break;
			case QEvent::MouseMove:
				//hover -> 轴区浅蓝高亮（整块轴控件区域）
				showZoneHighlight(QRect(zoneWidget->mapTo(this, QPoint(0, 0)), zoneWidget->size()));
				break;
			case QEvent::Leave:
				hideZoneHighlight();
				break;
			default:
				break;
			}
		}
		else if ((_headerBandWgt && watched == _headerBandWgt)
			|| (_footerBandWgt && watched == _footerBandWgt))
		{
			//--- 页眉/页脚条带（引擎自绘控件，画布之外）交互 ---
			//左键双击 = 选中 Header/Footer 节点并请求弹属性编辑器（视窗左上角，
			//同轴标题双击先例）：经选择中枢选中（条带文字主题蓝+加粗、树回选、
			//参数栏换页均由 sig_currentNodeChanged 闭环），弹窗页未注册
			//Header/Footer 类型按既有回退策略落参数栏页。条带隐藏（不可见/无
			//文本）时 Qt 不向隐藏控件派发鼠标事件，不会进入本分支；
			//非左键双击放行走默认处理
			if (event->type() == QEvent::MouseButtonDblClick
				&& static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
			{
				const int bandId = bandNodeId(watched == _headerBandWgt
					? CurveNodeType::Header : CurveNodeType::Footer);
				if (bandId > 0
					&& mCurveDocController::getInstance()->canSelectNode(bandId))
				{
					mCurveDocController::getInstance()->setCurrentNodeId(bandId);
					emit sig_requestEditorPopup(bandId, mapToGlobal(QPoint(12, 12)));
				}
				return true;   //消费左键双击（不透传引擎）
			}
		}
		return QWidget::eventFilter(watched, event);
	}

	int mCurveChartView::chartId() const
	{
		return _chartId;
	}

	void mCurveChartView::rebuild()
	{
		PERF_MARK("rebuild enter");
		
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	CurveNode* chart = doc->node(_chartId);
	if (!chart)
		return;

	//canvas 可能因 setUseOpenGL 等被引擎重建，重建后需把事件过滤器挂到新画布
	ensureCanvasFilter();

	//重建前视图捕获：样式/标签/标题/增删曲线等"值空间不变"的变更后恢复用户
	//缩放，不被全量重建重置。仅恢复"用户缩放过"的轴（自动范围轴保持按数据
	//适配）；数据形态（刻度模式/计权/图表类型）变化或重建前无可见曲线（首条
	//曲线入图）时仍按数据重新适配
	const bool preserveView = _rebuildPreserveView;
	_rebuildPreserveView = false;
	QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
	QwtScaleDiv savedDiv[4] = {};
	bool savedAuto[4] = { true, true, true, true };
	bool hadVisibleCurve = false;
	QHash<int, QPair<int, int>> axisModesBefore;   //轴节点id -> (刻度模式, 计权)
	if (qwt && preserveView)
	{
		const int axes[] = { QwtPlot::xBottom, QwtPlot::yLeft, QwtPlot::xTop, QwtPlot::yRight };
		for (int k = 0; k < 4; ++k)
		{
			savedDiv[k] = qwt->axisScaleDiv(axes[k]);
			savedAuto[k] = qwt->axisAutoScale(axes[k]);
		}
		//新副轴初值对齐用：记录重建前主轴当前显示范围（fitChartAxes 的副轴
		//"无挂载曲线回退"取此值——缩放后再创建的副轴与当前视图一致）
		_preRebuildXDiv[0] = savedDiv[0].lowerBound();
		_preRebuildXDiv[1] = savedDiv[0].upperBound();
		_preRebuildYDiv[0] = savedDiv[1].lowerBound();
		_preRebuildYDiv[1] = savedDiv[1].upperBound();
		_hasPreRebuildDiv = qwt->axisEnabled(QwtPlot::xBottom)
			&& qwt->axisEnabled(QwtPlot::yLeft)
			&& savedDiv[0].lowerBound() < savedDiv[0].upperBound()
			&& savedDiv[1].lowerBound() < savedDiv[1].upperBound();
	}
	else
	{
		_hasPreRebuildDiv = false;
		//极坐标滚轮缩放视图态随全量重建复位（与直角轴"非保视图重建重算自动域"
		//同语义）；preserveView 重建保留（同直角轴恢复 savedDiv 口径）
		_polarRadiusZoom = 1.0;
		//步长冻结随缩放复位一并解除（D28）：面板口径交还——下次缩放重新捕获；
		//脏标签标记同步复位（D35）
		_polarZoomTickStep = 0.0;
		_polarZoomStepDirty = false;
	}
	const int chartTypeBefore = chart->chart.chartType;
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (!n)
			continue;
		if (n->type == CurveNodeType::Curve && n->series.visible)
			hadVisibleCurve = true;
		if (n->type == CurveNodeType::XAxis || n->type == CurveNodeType::YAxis)
			axisModesBefore[childId] = { n->axis.scaleMode, n->axis.weightMode };
	}

	//复数图模式（图表类型2）先切换面板形态：开启后复数对上下面板成对渲染、
	//单曲线落上面板（applyComplexSeries）；退出时引擎销毁下面板并恢复 X 轴。
	//幂等：模式与展开方式均未变时为空操作
	_plot->setComplexMode(chart->chart.chartType == 2);
	//复数模式激活会在引擎内新建下面板：其画布的事件过滤（hover/点选/双击）
	//须在面板创建后补挂（此前的 ensureCanvasFilter 调用点都在本行之前，
	//下面板尚不存在挂不上）。函数幂等，上/下面板已在位时无操作
	ensureCanvasFilter();

	_plot->clearAll();
		_plot->clearMarkerLines();
	clearBaselineLabels();      //基准线标签图元由本视图自管，随全量重建一并清理
	clearAnnotationMarkers();   //注释标记由本视图自管，随全量重建一并清理
	clearBarLabelItem();        //柱顶标注图元由本视图自管，随全量重建一并清理
	clearPolarItems();          //极坐标接管图元由本视图自管，随全量重建一并清理
	hideAnnotationHover();
	clearAxisStrips();          //副轴自绘条带同为自管，随全量重建一并清理

	//引擎键映射随重建全量重算（R23 曲线允许重名）：引擎注册表已被 clearAll
	//清空，重算后的键与随后 applySeries/applyComplexSeries 的下发一一对应；
	//增删曲线/改名/撤销重做等增量触发都经 scheduleRebuild 收口到本函数
	ensureEngineCurveKeys();

	//图表级外观：背景色走 PlotSettings（图表属性编辑器"高级"组，引擎已有字段；
	//网格色在各轴 applyAxis 按图表节点下发）
	PlotView::PlotSettings ps = _plot->getPlotSettings();
	ps.backgroundColor = chart->chart.backgroundColor;
	_plot->setPlotSettings(ps);
	//画布外边距区（引擎默认窗口灰底）同步图表背景色，与画布同色
	_plot->setMarginBackground(chart->chart.backgroundColor);
	//视图自身边距区（右侧副轴条带所在的外扩区）同色（构造时已开 autofill）
	{
		QPalette viewPal = palette();
		viewPal.setColor(QPalette::Window, chart->chart.backgroundColor.isValid()
			? chart->chart.backgroundColor : QColor(Qt::white));
		setPalette(viewPal);
	}

//图例子节点在循环内只记录：自动定位的遮挡评估依赖曲线最终像素位置，
	//须等 fitChartAxes()（自动范围适配）之后才能应用
	int legendChildId = -1;
	const bool complexChart = (chart->chart.chartType == 2);
	//复数图排布映射（HyperGraph 2024：相位/虚部侧在上）：上面板轴节点落引擎
	//上 Y（标题=面板分量名），下面板轴节点经 applySecondaryAxes 下发
	int complexTopAxis = -1;
	if (complexChart)
		complexPanelAxisIds(&complexTopAxis, nullptr);
		const int complexDisplayFmt = chart->chart.complexDisplayFormat;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* n = doc->node(childId);
			if (!n)
				continue;
			switch (n->type)
			{
			case CurveNodeType::Curve:
				if (complexChart)
					break;   //复数图：循环后统一经 applyComplexSeries 成对/单发
				applySeries(childId);
				break;
			case CurveNodeType::XAxis:
				applyAxis(childId, AxisId::X_Bottom);
				break;
			case CurveNodeType::YAxis:
			case CurveNodeType::YAxis2:
				//复数图：仅上面板轴节点落引擎上 Y（标题=面板分量名随显示格式
				//派生，不取节点标题）；下面板轴节点经 applySecondaryAxes 下发
				//（fitChartAxes 之后）。非复数：Y1 落引擎上 Y，Y2 走副轴路径
				if (complexChart)
				{
					if (childId == complexTopAxis)
						applyAxis(childId, AxisId::Y_Left,
							complexPanelComponentTitle(
								n->type == CurveNodeType::YAxis, complexDisplayFmt));
				}
				else if (n->type == CurveNodeType::YAxis)
					applyAxis(childId, AxisId::Y_Left);
				break;
			case CurveNodeType::Legend:
				legendChildId = childId;
				break;
			case CurveNodeType::Header:
				applyHeader();
				break;
			case CurveNodeType::Footer:
				applyFooter();
				break;
			case CurveNodeType::Baseline:
				//建/改 markerLine（含显隐判定），与增量刷新共用（设计图14）
				applyBaseline(childId);
				break;
			case CurveNodeType::Annotation:
				applyAnnotation(childId);
				break;
			default:
				break;
			}
		}

	//复数图：配对成对下发 + 单曲线落上面板（须在 fitChartAxes 之前完成，
	//引擎曲线才可供后续适配/图例遮挡评估/高亮重放使用）
	if (complexChart)
		applyComplexSeries();

	//自动范围适配（在曲线应用之后按数据计算）+ 零线十字
	fitChartAxes();
	addZeroCrossLines();
	applyBarLabels();           //柱状图柱顶标注：与曲线同参重建（自管图元）
	applyPolarDecor();          //极坐标接管：底图+向量层随重建重挂（自管图元）
	//复数图中间共享标签位（HyperGraph 2024 语义）：注入 Y1 轴标题（数据类型
	//模板解析，如"Non Struct Disp"/VELOCITY）；空/不可见时引擎侧回退"Y 轴"。
	//面板分量名不占此位（随格式在 applyAxis/applyComplexLowerAxis 派生）
	if (complexChart)
	{
		CurveNode* y1Node = nullptr;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* c = doc->node(childId);
			if (c && c->type == CurveNodeType::YAxis)
			{
				y1Node = c;
				break;
			}
		}
		QString midLabel;
		if (y1Node && y1Node->axis.titleVisible)
			midLabel = resolveBandTemplate(y1Node->axis.title,
				firstVisibleCurveSeries(doc, _chartId));
		_plot->setComplexYLabelText(midLabel.trimmed());
	}
	//图例最后应用：自动定位按曲线的最终像素位置评估四角遮挡（优先右上）
	if (legendChildId > 0)
		applyLegend(legendChildId);
	//副轴最后应用（多坐标轴）：引擎上/右轴复位 -> 首枚副轴走原生轴（fit 已按
	//挂轴曲线算好范围）-> 第2枚起自绘条带（镜像同侧原生轴范围）-> 条带排布
	applySecondaryAxes();

	//视图恢复：数据形态（刻度模式/计权/图表类型）未变且重建前已有可见曲线时，
	//把"用户缩放过"的轴恢复到重建前的分隔（样式/标题/增删曲线等纯外观变更
	//不打断用户视图）；自动范围轴保持本次按数据适配的结果
	if (qwt && preserveView && hadVisibleCurve)
	{
		QHash<int, QPair<int, int>> axisModesAfter;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* n = doc->node(childId);
			if (n && (n->type == CurveNodeType::XAxis || n->type == CurveNodeType::YAxis))
				axisModesAfter[childId] = { n->axis.scaleMode, n->axis.weightMode };
		}
		if (chart->chart.chartType == chartTypeBefore && axisModesAfter == axisModesBefore)
		{
			const int axes[] = { QwtPlot::xBottom, QwtPlot::yLeft, QwtPlot::xTop, QwtPlot::yRight };
			for (int k = 0; k < 4; ++k)
				if (!savedAuto[k])   //仅恢复用户缩放过（非自动范围）的轴
					qwt->setAxisScaleDiv(axes[k], savedDiv[k]);
		}
		//恢复用户缩放后原生轴范围已变：自绘副轴条带重镜像一次（条带在
		//applySecondaryAxes 时镜像的是重建中按数据重算的范围），保持各轴一致
		onViewRangeChanged();
	}
		//注释渲染项在本次重建中先于引擎重绘挂载，显式补一帧确保立即可见
		if (QwtPlot* qwt = _plot->getQwtPlot())
			qwt->replot();
	//全量重建重置了画笔，选中/hover 高亮需重放
	applyHighlightVisuals();
	//频率红线游标重放：复数下面板随重建销毁重建，重挂+回写值（视图态）
	applyFrequencyCursorVisuals();
	//基准线 hover 高亮同样被重置（applyBaseline 写回模型值），重放
	applyBaselineHoverVisuals();
	//轴标题外观同样被重置为模型值，轴激活态（主题蓝+加粗）需重放
	applyAxisTitleHighlight();
	//编辑曲线会话覆盖层重放：重建把引擎曲线画笔回满不透明度并清了注册表，
	//压暗需重做；旁路图元（截取段/裁剪线）不经引擎清理仍在，重采样即可
	if (_editMode)
	{
		refreshEditSegment();
		applyEditDimming();
	}
}

	void mCurveChartView::scheduleRebuild(bool preserveView)
	{
		//防抖：同一事件循环内的多次变更只触发一次全量重建。
		//pch 导入宏内几十条属性命令若逐条重建，PlotView/Qwt 高频清空重建会
		//破坏内部状态（曾导致绘制期访问冲突），且伴随严重闪烁。
		//preserveView 以同批最后一次入队为准（样式与形态键混批时按形态处理）
		_rebuildPreserveView = preserveView;
		if (_rebuildPending)
			return;
		_rebuildPending = true;
		QTimer::singleShot(0, this, [this]()
		{
			_rebuildPending = false;
			rebuild();
		});
	}

	//柱状图柱顶数值标注（chartType=1）：取首条可见曲线（柱状单序列参与量），
	//标注位置=与引擎曲线同参变换后的数据坐标（draw 时经轴映射取像素，缩放/
	//刻度切换随全量重建联动），文本=原始 Y 值；变换滤点致索引错位时文本退化为
	//变换值（柱状常配 Linear 不触发，dB/Log 配负值数据才可能）
	void mCurveChartView::applyBarLabels()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chartNode = doc->node(_chartId);
		const bool barChart = chartNode && chartNode->chart.chartType == 1;
		if (!barChart)
		{
			clearBarLabelItem();
			return;
		}
		const CurveSeriesData* barSeries = nullptr;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* n = doc->node(childId);
			if (n && n->type == CurveNodeType::Curve && n->series.visible)
			{
				barSeries = &n->series;
				break;
			}
		}
		if (!barSeries)
		{
			clearBarLabelItem();
			return;
		}
		if (_barLabelItem == nullptr)
		{
			_barLabelItem = new mCurveBarLabelItem();
			if (QwtPlot* qwt = _plot->getQwtPlot())
				_barLabelItem->attach(qwt);
		}
		QVector<double> xs, ys;
		if (!transformedSeries(*barSeries, xs, ys,
			axisScaleModeOf(CurveNodeType::XAxis), axisScaleModeOf(CurveNodeType::YAxis),
			axisWeightModeOf(CurveNodeType::XAxis), axisWeightModeOf(CurveNodeType::YAxis),
			axisDbRefOf(CurveNodeType::XAxis), axisDbRefOf(CurveNodeType::YAxis)))
		{
			_barLabelItem->clearBars();
			return;
		}
		const int rawCount = barSeries->yData.size();
		const bool aligned = (xs.size() == rawCount && ys.size() == rawCount);
		QStringList texts;
		texts.reserve(ys.size());
		for (int i = 0; i < ys.size(); ++i)
		{
			//柱标注恒原始量值（任务 3.2/D5）：与缩放域无关（对数档不得显示变换后
			//坐标值）。变换滤点（Log/dB 剔非正值）致索引错位时按类目位置映射回
			//原始值，位置仍取存活点变换坐标；映射不中回退变换值兜底。
			//格式='g' 4 位无 + 号（2026-11-11 需求 05 定稿：-1245/681.8/5.098e-06
			//——带 + 号系 Note 行/曲线点标注口径，柱标注不适用）
			const double raw = aligned
				? barSeries->yData.at(i)
				: (barSeries->xData.indexOf(xs.at(i)) >= 0
					? barSeries->yData.at(barSeries->xData.indexOf(xs.at(i)))
					: ys.at(i));
			texts << QString::number(raw, 'g', 4);
		}
		_barLabelItem->setBarData(xs, ys, texts);
	}

	void mCurveChartView::clearBarLabelItem()
	{
		delete _barLabelItem;
		_barLabelItem = nullptr;
	}

	//极坐标画布接管实现（design D5），见头文件极坐标方法组注释
	bool mCurveChartView::isPolarChart() const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chartNode = doc->node(_chartId);
		return chartNode && chartNode->chart.chartType == 3;
	}

	//画布点选命中：极坐标向量端点查询（坐标=画布局部，与 draw 的 canvasRect
	//同约定），返回曲线名（空=未命中）
	bool mCurveChartView::hitPolarSeries(const QPoint& pos, QString& name) const
	{
		name.clear();
		if (!_plot || !_polarSeriesItem || !_plot->getQwtPlot())
			return false;
		QWidget* canvas = _plot->getQwtPlot()->canvas();
		if (!canvas)
			return false;
		name = _polarSeriesItem->hitTest(canvas->rect(), pos);
		return !name.isEmpty();
	}

	//摘除极坐标接管图元（重建起点/析构/非极坐标图调用；QwtPlot 不拥有
	//attach 的 item，须本视图显式 delete）
	void mCurveChartView::clearPolarItems()
	{
		delete _polarFrameItem;
		_polarFrameItem = nullptr;
		delete _polarSeriesItem;
		_polarSeriesItem = nullptr;
	}

	//极坐标底图+向量层重算重挂（rebuild 在 fitChartAxes 后调用；半径域与
	//刻度归一收口于此——FrameItem/SeriesItem 只认 [0,1] 归一化半径）
	//刻度标注文本（D25 统一管线：0自动='g'+精度/1科学='e'/2定点='f'，精度钳
	//[0,15]、'g' 至少 1 位——mCurveScaleDraw 同语义；Log 档径向标签先取 10^t
	//再入管线，角度度数直接入管线）
	static QString polarTickLabelText(double t, int format, int precision)
	{
		if (format == 1)
			return QString::number(t, 'e', qBound(0, precision, 15));
		if (format == 2)
			return QString::number(t, 'f', qBound(0, precision, 15));
		return QString::number(t, 'g', qMax(1, qBound(0, precision, 15)));
	}

	void mCurveChartView::applyPolarDecor()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chartNode = doc->node(_chartId);
		if (!chartNode || chartNode->chart.chartType != 3)
		{
			clearPolarItems();
			//非极坐标图：直角边框恢复（极坐标轮关闭；本函数随每次重建回放，
			//两向幂等）。成员以 QPointer<QWidget> 持有（匿名命名空间类不入
			//头文件），调用点在本 .cpp 类定义之后，下行转型安全
			if (_borderOverlay)
				static_cast<BorderOverlay*>(_borderOverlay.data())
					->setFrameVisible(true);
			return;
		}
		//极坐标画布独占（原型 05：纯极坐标画布，2026-11-12 轮）：直角轴
		//（刻度/数字/轴标题）随极坐标接管全关——圆环/辐条/径向刻度数字由
		//PolarFrameItem 自绘；直角边框覆盖层同轮关闭（BorderOverlay 类注）。
		//本函数在 rebuild 时序 applyAxis → fitChartAxes 之后收尾，置回不被
		//覆盖；域非法等退化路径亦不再露出直角骨架
		if (QwtPlot* qwt = _plot->getQwtPlot())
		{
			qwt->enableAxis(QwtPlot::xBottom, false);
			qwt->enableAxis(QwtPlot::xTop, false);
			qwt->enableAxis(QwtPlot::yLeft, false);
			qwt->enableAxis(QwtPlot::yRight, false);
		}
		if (_borderOverlay)
			static_cast<BorderOverlay*>(_borderOverlay.data())
				->setFrameVisible(false);
		//轴节点：X1=角度放射线数（刻度总数直接生效）、Y1=半径缩放/计权/
		//标题/字体（kText 模板解析同直角轴 applyAxis 口径）
		CurveNode* x1Node = nullptr;
		CurveNode* y1Node = nullptr;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* c = doc->node(childId);
			if (!c)
				continue;
			if (c->type == CurveNodeType::XAxis && !x1Node)
				x1Node = c;
			if (c->type == CurveNodeType::YAxis && !y1Node)
				y1Node = c;
		}

		if (!y1Node)
		{
			clearPolarItems();
			return;
		}

		//半径缩放契约入参：缩放/计权/参考值取 Y1 轴节点。计权频率取首条可见
		//曲线携带的 xData[0]（D25：贡献量极坐标=单频图，全曲线同频，scale 级
		//即逐条目级；旧恒 0 使 A/B/C 计权死在 weightGain(·,0)=1）
		PolarRadiusScale scale;
		scale.scaleMode = y1Node->axis.scaleMode;
		//极坐标 Linear/Log 档计权屏蔽（2026-11-16 原型逐字实证+用户裁定：HyperGraph
		//加权勾选+A 灰显下 Linear 极坐标仍渲染原始值——环标 1.0E-04、向量原始长度，
		//11-13 D30②"存储计权值仍对产出生效"语义就此作废；Log 档同轮收口
		//nvh-log-scale-prototype-align——负十倍程刻度语义下计权同样不可达，
		//存量存档 Y1 带计权值重开同样对齐。屏蔽收在渲染契约处；dB10/dB20 档
		//（仅存量存档可达）计权维持，随「曲线计权档位边界」通用档位闸）
		scale.weightMode = (scale.scaleMode == 0 || scale.scaleMode == 1)
			? 0 : y1Node->axis.weightMode;
		scale.dbRef = y1Node->axis.dbRef;
		scale.freqHz = 0.0;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* c = doc->node(childId);
			if (c && c->type == CurveNodeType::Curve && c->series.visible
				&& !c->series.xData.isEmpty())
			{
				scale.freqHz = c->series.xData.first();
				break;
			}
		}
		scale.domainMin = 0.0;
		scale.domainMax = 1.0;

		//数据域：可见曲线幅值经变换（含计权）后取范围，与默认档取并集
		//（默认档：Linear [0,max] / Log [-3,1] / dB [-30,10]；手动态直接采用）
		double dataLo = std::numeric_limits<double>::max();
		double dataHi = std::numeric_limits<double>::lowest();
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* c = doc->node(childId);
			if (!c || c->type != CurveNodeType::Curve || !c->series.visible)
				continue;
			PolarRadiusScale cs = scale;
			cs.freqHz = c->series.xData.isEmpty() ? 0.0 : c->series.xData.first();
			const int nPts = c->series.yData.size();
			for (int i = 0; i < nPts; ++i)
			{
				if (c->series.isPointHidden(i))
					continue;
				const double t = cs.transformed(c->series.yData.at(i));
				if (qIsFinite(t))
				{
					dataLo = qMin(dataLo, t);
					dataHi = qMax(dataHi, t);
				}
			}
		}
		double domainLo = 0.0, domainHi = 1.0;
		const int m = scale.scaleMode;
		bool manual = !y1Node->axis.autoScale;
		if (manual)
		{
			//D25：模型 kMin/kMax 存原始值空间（面板经 inverseTransformValue 提交），
			//经 scale 变换（含计权）后作径向域——旧直用把原始值当变换空间，dB 档
			//手填域全部向量贴极点；非有限/序颠倒回落自动域（L16：退化不再清整画布）
			const double tLo = scale.transformed(y1Node->axis.min);
			const double tHi = scale.transformed(y1Node->axis.max);
			if (std::isfinite(tLo) && std::isfinite(tHi) && tHi > tLo)
			{
				domainLo = tLo;
				domainHi = tHi;
			}
			else
			{
				manual = false;
			}
		}
		if (!manual && m == 0)
		{
			domainLo = 0.0;
			if (dataHi > dataLo)
			{
				//步长优先整洁域（需求 04 原型逐字 2026-11-16 放大复核：8.01e-05
				//→ 六刻度 2.0E-05 步、域 1.0E-04；旧"1 位有效数字向上取整"
				//（7.65e-05→8.0E-05）系小字误读口径作废）：区间=数据上界/5 自
				//{1,2,2.5,5}×10^n 向上取整，域=5×区间——0 与上界双端齐整
				const double raw = dataHi / 5.0;
				const double e = std::floor(std::log10(raw));
				const double base = std::pow(10.0, e);
				const double mant = raw / base;
				double pick = 10.0;
				for (double c : {1.0, 2.0, 2.5, 5.0})
				{
					if (mant <= c + 1e-9)
					{
						pick = c;
						break;
					}
				}
				domainHi = pick * base * 5.0;
			}
			else
			{
				domainHi = 1.0;
			}
		}
		else if (!manual && m == 1)
		{
			domainLo = qMin(-3.0, dataLo);
			domainHi = qMax(1.0, dataHi);
		}
		else if (!manual)
		{
			//D25（原型 05 面板四裁定）：dB20 自动域默认 [-60,20]（旧 -30/10
			//作废）；dB10 维持 [-30,10]；默认域与数据域取并集
			if (m == 2)
			{
				domainLo = qMin(-60.0, dataLo);
				domainHi = qMax(20.0, dataHi);
			}
			else
			{
				domainLo = qMin(-30.0, dataLo);
				domainHi = qMax(10.0, dataHi);
			}
		}
		if (!(domainHi > domainLo))
		{
			clearPolarItems();
			return;
		}
		//滚轮缩放视图态（2026-11-16 D35 商软对齐重裁定，五样张实证）：以极点
		//为锚缩放有效域——每档 ×0.8/×1.2 连乘；格数=域/step 钳 [4,6]：浅档
		//step 不动（图13/14 实证 2.0E-05 恒定），越界 step=域/6 或 域/4 重算
		//（脏值继承，图15 0.17118=域/6、图16 5.55030E-07=域/4）；域恒=格数×
		//step 齐网格，主环/次环/刻度随域重算（下方刻度生成消费 domainLo/
		//domainHi），不动 Y1 模型值
		if (_polarRadiusZoom != 1.0)
		{
			//乘子直乘有效域（2026-11-16 真机回归钉死：旧 / 缩放变量把每档
			//×0.8/×1.2 等效成 ×1.25/×0.8333 且滚轮上=域外扩，双向偏离裁定
			//口径——截图 9.44499E-05=×1.25/×0.8333 链唯一落点，×0.8/×1.2
			//链无解；乘子语义见 mCurveChartView.h _polarRadiusZoom 注释）
			domainHi = domainLo + (domainHi - domainLo) * _polarRadiusZoom;
			if (_polarZoomTickStep > 0.0)
			{
				//格数判定的浮点容差 1e-9：恰 6/4 格时 ×1.2/0.8 连乘的表示误差
				//（6.000000000000001）不得误判越界——浅档 step 与标签格式保持
				const double rawCells = (domainHi - domainLo) / _polarZoomTickStep;
				if (rawCells > 6.0 + 1e-9)
				{
					_polarZoomTickStep = (domainHi - domainLo) / 6.0;
					_polarZoomStepDirty = true;
				}
				else if (rawCells < 4.0 - 1e-9)
				{
					_polarZoomTickStep = (domainHi - domainLo) / 4.0;
					_polarZoomStepDirty = true;
				}
			}
		}
		scale.domainMin = domainLo;
		scale.domainMax = domainHi;
		//D25：径向域存值入回显链（displayedAxisRange 极坐标分支回读）
		_polarDomainValid = true;
		_polarDomainLo = domainLo;
		_polarDomainHi = domainHi;

		//半径刻度（变换空间生成后归一下发）：刻度生成方式全量消费 Y1 面板
		//设置（2026-11-12 原型 05 轮——旧实现恒均匀分步+硬编码 E 格式，面板
		//"固定步长/刻度总数/格式/精度"在极坐标不生效）。分派与直角轴
		//applyAxis makeTickEngine 同口径：linear 0 均匀分步法（刻度总数等分
		//含两端）/1 固定步长法（majorStep 绝对倍数对齐）；log/dB 0 每十倍/1
		//均匀/2 固定步长（变换空间取步长）。步长过细与域宽失配（>200 条主
		//刻度）回落自动划分（mCurveDecadeTickDiv 同款守卫）
		const int genMode = y1Node->axis.tickGenMode;
		const double fixedStep = y1Node->axis.majorStep;
		const bool useFixedStep = fixedStep > 0.0
			&& (domainHi - domainLo) / fixedStep <= 200.0
			&& ((m == 0 && genMode == 1) || (m != 0 && genMode == 2));
		const bool useTickCount = ((m == 0 && genMode == 0)
				|| (m != 0 && genMode == 1))
			&& y1Node->axis.tickCount >= 2.0;
		//滚轮缩放期刻度生成（2026-11-16 D35 商软对齐重裁定）：以缩放步长横跨
		//缩放域产主环——格数钳 [4,6] 已在上方域构建处维持，此处按整数格铺环
		//（域恒=格数×step 齐网格，无错位残环）。>200 条主刻度守卫保留兜底：
		//step 未捕获等异常回落自动划分。步长非法（未捕获/非正值）走原分派
		const bool useZoomStep = _polarZoomTickStep > 0.0
			&& (domainHi - domainLo) / _polarZoomTickStep <= 200.0;
		QwtScaleDiv div;
		if (useZoomStep)
		{
			QList<double> ticks[QwtScaleDiv::NTickTypes];
			const int kFirst = static_cast<int>(std::ceil(domainLo / _polarZoomTickStep - 1e-9));
			const int kLast = static_cast<int>(std::floor(domainHi / _polarZoomTickStep + 1e-9));
			for (int k = kFirst; k <= kLast; ++k)
				ticks[QwtScaleDiv::MajorTick] += k * _polarZoomTickStep;
			div = QwtScaleDiv(QwtInterval(domainLo, domainHi), ticks);
		}
		else if (useFixedStep)
		{
			QList<double> ticks[QwtScaleDiv::NTickTypes];
			const int kFirst = static_cast<int>(std::ceil(domainLo / fixedStep - 1e-9));
			const int kLast = static_cast<int>(std::floor(domainHi / fixedStep + 1e-9));
			for (int k = kFirst; k <= kLast; ++k)
				ticks[QwtScaleDiv::MajorTick] += k * fixedStep;
			div = QwtScaleDiv(QwtInterval(domainLo, domainHi), ticks);
		}
		else if (useTickCount)
		{
			//次刻度本函数自补（极坐标口径见下），引擎内建份数传 0 关闭
			div = mCurveUniformTickDiv(domainLo, domainHi,
				y1Node->axis.tickCount, -1, 0);
		}
		else if (m != 0)
		{
			div = mCurveDecadeTickDiv(domainLo, domainHi, decadeWidthOf(m),
				y1Node->axis.decadeTickCount, y1Node->axis.decadeMinorCount, 5);
		}
		else
		{
			div = QwtLinearScaleEngine().divideScale(domainLo, domainHi, 8, 0, 0.0);
		}
		//主刻度步长存档（D28）：滚轮首次离开 1.0 时经 _polarZoomTickStep 捕获。
		//各分派（固定步长/均匀/每十倍/Qwt 线性）主刻度在变换空间均匀，首差即
		//步长；不足 2 条主刻度不更新（保持旧值，捕获端有 >0 判定兜底）
		const QList<double> polarDivMajors = div.ticks(QwtScaleDiv::MajorTick);
		if (polarDivMajors.size() >= 2)
			_polarLastMajorStep = polarDivMajors.at(1) - polarDivMajors.at(0);
		QVector<double> normMajors, normMinors;
		QStringList majorLabels;
		//缩放衍生脏步长的标签模式（D35 商软深档实证）：按当前 step 量级全轴
		//统一——[0.01,1e4) 定点 5 位小数（图15 "0.17118"/零 "0.00000"）、之外
		//科学大写 E 5 位小数（图16 "5.55030E-07"/零 "0.00000E+00"）；浅档整洁
		//值仍走 'E',1（D31 口径不动）。step 非正走科学（防御）
		const bool zoomLabelSci = !(_polarZoomTickStep >= 0.01
			&& _polarZoomTickStep < 10000.0);
		for (double t : div.ticks(QwtScaleDiv::MajorTick))
		{
			const double rNorm = qBound(0.0, (t - domainLo) / (domainHi - domainLo), 1.0);
			normMajors << rNorm;
			//D25/L15：Log 档标签同走格式/精度管线——变换空间刻度经 10^t 反变换
			//回原值空间后按档位格式化（撤 10^t+'g',6 硬编码，mCurveScaleDraw 同语义）
			const double labelValue = (m == 1) ? std::pow(10.0, t) : t;
			//Linear 档径向数字原型形态（2026-11-16 裁定）："自动"=定点科学计数
			//1 位小数大写 E（"0.0E+00"/"2.0E-05"/"1.0E-04"，旧 'g'+精度剥尾零
			//形态作废）；显式科学计数法/定点档仍走格式/精度管线（D25/L15）
			majorLabels << (m == 0 && y1Node->axis.labelFormat == 0
				? (_polarZoomStepDirty
					? (zoomLabelSci ? QString::number(labelValue, 'E', 5)
					                : QString::number(labelValue, 'f', 5))
					: QString::number(labelValue, 'E', 1))
				: polarTickLabelText(labelValue,
					y1Node->axis.labelFormat, y1Node->axis.precision));
		}
		//次环（原型 05 口径裁定）："每主刻度网格数"=每主刻度段细分的份数——
		//2 即段中点 1 条次环、3 即三分点 2 条（与直角轴引擎"次刻度条数"语义
		//不同，极坐标按原型执行）；<=1（含 0=关次网格，2026-11-12 既有裁定）
		//不下发。每十倍分步法（log/dB）保留引擎内建"每十倍网格线数"次环，
		//不套本口径
		if (!(m != 0 && !useZoomStep && !useFixedStep && !useTickCount))
		{
			const QList<double> majors = div.ticks(QwtScaleDiv::MajorTick);
			const int parts = y1Node->axis.minorCount;
			for (int i = 0; parts >= 2 && i + 1 < majors.size(); ++i)
				for (int k = 1; k < parts; ++k)
				{
					const double t = majors.at(i)
						+ (majors.at(i + 1) - majors.at(i)) * k / parts;
					normMinors << qBound(0.0,
						(t - domainLo) / (domainHi - domainLo), 1.0);
				}
		}
		else
		{
			//每十倍分步法（D25/M9）：引擎内建次刻度此前已产但无人消费——
			//"每十倍网格线数"在极坐标照直生效（归一口径同主刻度）
			for (double t : div.ticks(QwtScaleDiv::MinorTick))
				normMinors << qBound(0.0,
					(t - domainLo) / (domainHi - domainLo), 1.0);
		}
		if (_polarFrameItem == nullptr)
		{
			_polarFrameItem = new mCurvePolarFrameItem();
			if (QwtPlot* qwt = _plot->getQwtPlot())
				_polarFrameItem->attach(qwt);
		}
		_polarFrameItem->setRadiusTicks(normMajors, majorLabels, normMinors);
		_polarFrameItem->setAngleTickCount(
			x1Node ? qMax(3, static_cast<int>(x1Node->axis.tickCount)) : 8);
		//D25/M6+M7：角度标签独立通道（字体=刻度字体设置/颜色=显示颜色/文本=
		//格式精度管线预构建，撤 Y1 labelFont 劫持与黑硬编码）+两轴可见性
		//（kVisible：Y1=环网+径向数字+标题，X1=辐条+角度度数）；个数对齐
		//底图钳制口径（<3 钳 3，D25/L13）
		if (x1Node)
		{
			const int rawAngles = static_cast<int>(x1Node->axis.tickCount);
			const int nAngles = qMax(3, rawAngles);
			QStringList angleLabels;
			for (int k = 0; k < nAngles; ++k)
				angleLabels << polarTickLabelText(360.0 * k / nAngles,
					x1Node->axis.labelFormat, x1Node->axis.precision);
			_polarFrameItem->setAngleLabels(angleLabels);
			_polarFrameItem->setAngleLabelFont(x1Node->axis.labelFont);
			_polarFrameItem->setAngleLabelColor(
				x1Node->axis.titleColor.isValid()
					? x1Node->axis.titleColor : QColor(Qt::black));
			_polarFrameItem->setAngularVisible(x1Node->axis.visible);
		}
		else
		{
			_polarFrameItem->setAngleLabels(QStringList());
			_polarFrameItem->setAngleLabelFont(QFont());
			_polarFrameItem->setAngleLabelColor(QColor(Qt::black));
			_polarFrameItem->setAngularVisible(true);
		}
		_polarFrameItem->setRadialVisible(y1Node->axis.visible);
		_polarFrameItem->setTitle(y1Node->axis.titleVisible
			? resolveBandTemplate(y1Node->axis.title,
				firstVisibleCurveSeries(doc, _chartId))
			: QString());
		_polarFrameItem->setTitleFont(y1Node->axis.titleFont);
		_polarFrameItem->setTitleColor(y1Node->axis.titleColor);
		_polarFrameItem->setLabelFont(y1Node->axis.labelFont);
		_polarFrameItem->setGridColor(chartNode->chart.gridColor);

		//向量层：可见曲线逐条入列（树序=图例载体序；条目 nodeId 供高亮对号）
		if (_polarSeriesItem == nullptr)
		{
			_polarSeriesItem = new mCurvePolarSeriesItem();
			if (QwtPlot* qwt = _plot->getQwtPlot())
				_polarSeriesItem->attach(qwt);
		}
		_polarSeriesItem->clearSeriesData();
		_polarSeriesItem->setScale(scale);
		_polarSeriesItem->setChainForm(_polarChainForm);
		//D32 链序对齐（2026-11-16 裁定实施）：链成员按树序逆序入列——原型为
		//升序生长形态（小模态短棒留极点、最大模态收尾，链尖收敛 Response），
		//现状降序把小段全堆链外端成"线头簇"（真机实锤）。证据与判读清单=
		//需求袋 reference/2026-11-16-polar-linear/chain-order-diagnosis.md。
		//先收集再入列：链成员逆序；Response/Others 独立射线仍按图例序槽位
		//（图例序由曲线树序经引擎侧生成，与本向量层无关；散开态遍历与序无关；
		//高亮按 nodeId 对号；链段无箭头/零值不推锚点语义保持）
		QList<PolarSeriesEntry> chainEntries;
		QList<PolarSeriesEntry> rayEntries;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* c = doc->node(childId);
			if (!c || c->type != CurveNodeType::Curve || !c->series.visible)
				continue;
			PolarSeriesEntry e;
			e.nodeId = childId;
			e.name = c->series.name;
			//链形态成员判定（spec：各模态贡献向量按序相连；Response/Others 恒
			//独立射线——2026-11-12 裁定。判定走产出链名称契约："<值> :Response"/
			//"<值> :Others" 之外的曲线一律链成员）
			e.chainLink = !e.name.contains(QStringLiteral(":Response"))
				&& !e.name.contains(QStringLiteral(":Others"));
			e.color = c->series.color;
			e.lineWidth = c->series.lineWidth;
			e.lineStyle = c->series.lineStyle;
			const double curveFreq = c->series.xData.isEmpty()
				? 0.0 : c->series.xData.first();
			const int nPts = qMin(c->series.yData.size(),
				c->series.complexYData.size());
			for (int i = 0; i < nPts; ++i)
			{
				if (c->series.isPointHidden(i))
					continue;
				e.phaseDeg << c->series.complexYData.at(i);
				e.magnitude << c->series.yData.at(i);
				e.freqHz << curveFreq;
			}
			if (e.phaseDeg.isEmpty())
				continue;
			(e.chainLink ? chainEntries : rayEntries).append(e);
		}
		for (int i = chainEntries.size() - 1; i >= 0; --i)
			_polarSeriesItem->addSeries(chainEntries.at(i));
		for (const PolarSeriesEntry& e : rayEntries)
			_polarSeriesItem->addSeries(e);

		//高亮对号（重建尾部 applyHighlightVisuals 亦按节点 id 重放，双保险）
		_polarSeriesItem->setSelectedNodeId(_selectedCurveId);
		_polarSeriesItem->setHoveredNodeId(_hoveredCurveId);

		//D25：径向域变化广播（轴属性页回查回显，只动面板数字不写模型——
		//替代旧 fitChartAxes 极坐标早退对通知链的绕过）
		notifyAxisDisplayRanges();
	}

	//极坐标"首尾相接"形态开关（视图态）：重建回放（applyPolarDecor 下发）
	void mCurveChartView::setPolarChainForm(bool chain)
	{
		if (_polarChainForm == chain)
			return;
		_polarChainForm = chain;
		scheduleRebuild(false);
	}

	//特定频率红线游标（NVH 模态贡献量，视图态不进命令栈）：freq<0 摘除；否则
	//值回写+上/复数下面板挂载重放。缩放/刻度切换经数据坐标映射天然保持
	void mCurveChartView::setFrequencyCursor(double freq)
	{
		_freqCursorX = freq;
		applyFrequencyCursorVisuals();
		if (QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr)
			qwt->replot();
		if (QwtPlot* lowerQwt = _plot ? _plot->getLowerQwtPlot() : nullptr)
			lowerQwt->replot();
	}

	//游标重放（rebuild 尾部）：复数下面板随重建销毁重建，底部游标重挂新面板
	//（attach 换绑；旧面板析构已自行 detach）；非复数模式底部游标摘除
	void mCurveChartView::applyFrequencyCursorVisuals()
	{
		if (_freqCursorX < 0.0)
		{
			delete _freqCursorTop;
			_freqCursorTop = nullptr;
			delete _freqCursorBottom;
			_freqCursorBottom = nullptr;
			return;
		}
		if (QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr)
		{
			if (_freqCursorTop == nullptr)
			{
				_freqCursorTop = new mCurveFrequencyCursor();
				_freqCursorTop->attach(qwt);
			}
			_freqCursorTop->setXValue(_freqCursorX);
		}
		QwtPlot* lowerQwt = _plot ? _plot->getLowerQwtPlot() : nullptr;
		if (lowerQwt != nullptr)
		{
			if (_freqCursorBottom == nullptr)
			{
				_freqCursorBottom = new mCurveFrequencyCursor();
				_freqCursorBottom->attach(lowerQwt);
			}
			_freqCursorBottom->setXValue(_freqCursorX);
		}
		else if (_freqCursorBottom != nullptr)
		{
			delete _freqCursorBottom;
			_freqCursorBottom = nullptr;
		}
	}

	//画布点选 X 吸附（NVH 模态贡献量）：首条可见曲线 xData 最近点（线性扫
	//最小距离；无可见曲线原值返回，面板侧仍会做最近文件频率归一兜底）
	double mCurveChartView::snapToNearestDataX(double x) const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (doc == nullptr || doc->node(_chartId) == nullptr)
			return x;
		const CurveSeriesData* first = nullptr;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* n = doc->node(childId);
			if (n && n->type == CurveNodeType::Curve && n->series.visible
				&& !n->series.xData.isEmpty())
			{
				first = &n->series;
				break;
			}
		}
		if (first == nullptr)
			return x;
		int best = -1;
		double bestDist = 0.0;
		for (int i = 0; i < first->xData.size(); ++i)
		{
			const double dist = qAbs(first->xData.at(i) - x);
			if (best < 0 || dist < bestDist)
			{
				best = i;
				bestDist = dist;
			}
		}
		return best >= 0 ? first->xData.at(best) : x;
	}

	bool mCurveChartView::transformedSeries(const CurveSeriesData& s,
		QVector<double>& xs, QVector<double>& ys, int xMode, int yMode,
		int xWeight, int yWeight, double xDbRef, double yDbRef) const
	{
		//按给定刻度/计权模式做数据预变换（模型存原始值，切换模式即时生效），
		//先经 x/y 侧 缩放系数->偏移量 预变换（x/y 数据属性页，设计图03.2），
		//负值=取主轴当前模式（复数收集等主轴场景）；参考值同语义（dB 模式
		//除入挂轴侧 axis.dbRef）
		if (xMode < 0)
			xMode = axisScaleModeOf(CurveNodeType::XAxis);
		if (yMode < 0)
			yMode = axisScaleModeOf(CurveNodeType::YAxis);
		if (xWeight < 0)
			xWeight = axisWeightModeOf(CurveNodeType::XAxis);
		if (yWeight < 0)
			yWeight = axisWeightModeOf(CurveNodeType::YAxis);
		if (xDbRef < 0)
			xDbRef = axisDbRefOf(CurveNodeType::XAxis);
		if (yDbRef < 0)
			yDbRef = axisDbRefOf(CurveNodeType::YAxis);
		xs.clear();
		ys.clear();
		xs.reserve(s.xData.size());
		ys.reserve(s.yData.size());
		int count = qMin(s.xData.size(), s.yData.size());
		for (int i = 0; i < count; ++i)
		{
			//编辑曲线"隐藏"区间内的点：以 NaN 原位占位（经离线像素验证，
			//Qwt/Qt 折线遇 NaN 断线）——与剔除不同，断口精确落在隐藏段两端，
			//点本身保留在模型中，"撤回"可整段恢复
			if (s.isPointHidden(i))
			{
				xs.append(qQNaN());
				ys.append(qQNaN());
				continue;
			}
			double tx = 0.0, ty = 0.0;
			transformPoint(s.xRef.applyTo(s.xData.at(i)),
				s.yRef.applyTo(s.yData.at(i)),
				xMode, yMode, xWeight, yWeight, tx, ty, xDbRef, yDbRef);
			if (qIsNaN(tx) || qIsNaN(ty))
				continue;
			xs.append(tx);
			ys.append(ty);
		}
		return !xs.isEmpty();
	}

	void mCurveChartView::applySeries(int curveNodeId)
	{
		PERF_MARK("applySeries enter");
		
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(curveNodeId);
		if (!n || n->type != CurveNodeType::Curve)
			return;

		const CurveSeriesData& s = n->series;
		if (!s.visible)
			return;

		//挂轴：x 侧 axisAssign 1=X2（引擎上轴）、y 侧 1=Y2（引擎右轴），>=1 均
		//挂引擎副轴（引擎只有一条上/右轴，多枚副轴取首枚），对应副轴节点不存在
		//时钳回主轴（如撤销副轴创建后残留的挂轴设置）；数据预变换同样按挂轴侧
		//节点的刻度/计权模式取
		int xMode = axisScaleModeOf(CurveNodeType::XAxis);
		int yMode = axisScaleModeOf(CurveNodeType::YAxis);
		int xWeight = axisWeightModeOf(CurveNodeType::XAxis);
		int yWeight = axisWeightModeOf(CurveNodeType::YAxis);
		double xDbRef = axisDbRefOf(CurveNodeType::XAxis);
		double yDbRef = axisDbRefOf(CurveNodeType::YAxis);
		AxisId xEngineAxis = AxisId::X_Bottom;
		AxisId yEngineAxis = AxisId::Y_Left;
		if (s.xRef.axisAssign >= 1)
		{
			const int topAxisId = nativeSecondaryAxisId(false);
			if (topAxisId > 0)
			{
				CurveNode* topAxis = doc->node(topAxisId);
				xEngineAxis = AxisId::X_Top;
				xMode = topAxis->axis.scaleMode;
				xWeight = topAxis->axis.weightMode;
				xDbRef = topAxis->axis.dbRef;
			}
		}
		if (s.yRef.axisAssign >= 1)
		{
			const int rightAxisId = nativeSecondaryAxisId(true);
			if (rightAxisId > 0)
			{
				CurveNode* rightAxis = doc->node(rightAxisId);
				yEngineAxis = AxisId::Y_Right;
				yMode = rightAxis->axis.scaleMode;
				yWeight = rightAxis->axis.weightMode;
				yDbRef = rightAxis->axis.dbRef;
			}
		}
		QVector<double> xs, ys;
		//极坐标载体：原始值装载（相位/幅值在极坐标绘制层经 PolarRadiusScale
		//现算，不做直角预变换——避免与绘制层双重变换）
		CurveNode* chartNodeEarly = doc->node(_chartId);
		const bool polarSeries = chartNodeEarly
			&& chartNodeEarly->chart.chartType == 3;
		if (polarSeries)
		{
			xs = s.xData;
			ys = s.yData;
		}
		else if (!transformedSeries(s, xs, ys, xMode, yMode, xWeight, yWeight, xDbRef, yDbRef))
			return;

		//引擎键（R23 曲线允许重名）：键=模型名首现，重复依次 名#2/名#3；
		//无碰撞图表键=名，下发行为与既有逐位一致
		const QString engineKey = keyFor(curveNodeId);
		_plot->updateCurveData(engineKey, xs, ys, xEngineAxis, yEngineAxis);

		//图表类型：0 XY曲线图 / 1 柱状图 / 3 极坐标（复数图走
		//applyComplexSeries，蛛网引擎暂无绘制能力）。切换经 kChartType
		//命令走全量 rebuild——clearAll 之后重画每条曲线，语义是"重绘"而非叠加
		CurveNode* chartNode = doc->node(_chartId);
		const bool barChart = chartNode && chartNode->chart.chartType == 1;

		CurveSettings cs = makeCurveSettings(s, engineKey);
		if (polarSeries)
		{
			//极坐标形态：向量射线由自绘 SeriesItem 接管（design D5），引擎曲线
			//退化为图例/命中链载体——线/填充/柱/符号全关；图例走名称通道
			//（2026-11-12：PlotView legendSeries 图例门放行纯载体曲线——凭
			//legendFallbackToName 入列，线样图标兜底；文本=曲线名 "<值> :Response"
			//原型逐字）
			cs.showLine = false;
			cs.showFill = false;
			cs.showBars = false;
			cs.showSymbol = false;
			cs.legendText = s.name;
			cs.legendFallbackToName = true;
		}
		else if (barChart)
		{
			//柱状形态隐藏折线（柱即数据形态），不做折线区域填充（语义属于曲线图）
			cs.showLine = false;
			cs.showFill = false;
			//柱色随曲线色；柱宽槽位百分比定标（任务 3.1/D4）：槽宽=画布宽/类目数，
			//柱宽=槽宽×0.6（需求稿常态约占槽位 60%），上限钳槽宽×0.8（留间隙
			//不粘连）、下限 2px（点多收敛但不得呈针状细线）
			cs.showBars = true;
			cs.barColor = s.color;
			double canvasW = 800.0;
			if (QwtPlot* qwt = _plot->getQwtPlot())
			{
				if (QWidget* canvas = qwt->canvas())
					canvasW = qMax(1, canvas->width());
			}
			//类目槽宽（D4）：画布宽/类目数
			const double slotW = canvasW / qMax(1, xs.size());
			cs.barWidth = qBound(2.0, slotW * 0.6, slotW * 0.8);
			//柱基按轴域锚定（任务 3.3/D6）：线性=零基线（正上负下）；对数/dB=
			//轴域下限——取数据最小值下探一个十倍程（decadeWidthOf 与刻度引擎
			//同源），基线低于轴底由画布裁剪兜底，柱呈现为"自轴底升起"。恒关
			//barAutoBase：引擎 auto 规则对变换域负值数据会锚到零线=轴顶（2026-11-11
			//真机"柱自轴顶悬垂"缺陷根因——log10(<1) 全负、auto 规则 maxY<0→maxY
			//把基线锚到最靠近 0 的负值，柱从高处垂下）
			cs.barAutoBase = false;
			const int yModeBar = axisScaleModeOf(CurveNodeType::YAxis);
			if (yModeBar == 0)
			{
				cs.barBaseValue = 0.0;   //线性/ %Plot：零基线，正上负下
			}
			else
			{
				double yMin = std::numeric_limits<double>::max();
				for (double v : ys)
				{
					if (qIsFinite(v) && v < yMin)
						yMin = v;
				}
				cs.barBaseValue = (yMin < std::numeric_limits<double>::max())
					? yMin - decadeWidthOf(yModeBar) : 0.0;
			}
		}
		_plot->setCurveStyle(engineKey, cs);
	}

	void mCurveChartView::applyComplexSeries()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chartNode = doc->node(_chartId);
		//图表级显示格式与面板互换（设计图01，右键"切换至实部/虚部/互换复数坐标
		//轴"提交的 kComplexDisplayFormat/kComplexSwapped）：节点仍存创建时的原始
		//两路数据，显示格式不同则渲染层现算换算（幅相↔实虚互转），来回切换无损
		const int displayFmt = chartNode ? chartNode->chart.complexDisplayFormat : 0;
		const bool swapped = chartNode && chartNode->chart.complexSwapped;

		//复数曲线为单节点（x/ym/yp 三壳，series 自带上/下面板两路数据），按
		//isComplex() 分派：复数节点 -> updateComplexCurveData（引擎上下面板各一条
		//子曲线，名字即图例/命中键）；普通节点（含相位侧为空的降级）走上面板常规
		//路径。旧"按命名约定跨节点配对"（classifyComplexName 收集）已随模型合并退役
		struct ComplexEntry
		{
			int nodeId = -1;
			QVector<double> xs, top, bottom;   //top/bottom=互换后的上/下面板显示数据
		};
		struct PlainEntry
		{
			int nodeId = -1;
			QVector<double> xs, ys;
		};
		QList<ComplexEntry> complexes;
		QList<PlainEntry> plains;

		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* n = doc->node(childId);
			if (!n || n->type != CurveNodeType::Curve || !n->series.visible)
				continue;
			const CurveSeriesData& s = n->series;
			if (!s.isComplex())
			{
				PlainEntry pe;
				pe.nodeId = childId;
				if (transformedSeries(s, pe.xs, pe.ys))
					plains.append(pe);
				continue;
			}
			ComplexEntry ce;
			ce.nodeId = childId;
			QVector<double> aRaw, bRaw;
			//上/下面板单趟同索引对齐变换：隐藏/非法点两路同步剔除（严格等长，
			//满足引擎 updateComplexCurveData 的等长校验——此前上面板走
			//transformedSeries 剔点、下面板走占位不剔点，长度不一致时引擎拒发，
			//全量重建清空后两面板都得不到重绘）
			if (!transformedComplexPair(n, ce.xs, aRaw, bRaw))
				continue;
			//显示格式换算（节点原始形态 -> 图表显示格式），再按排布决定上下面板：
			//基础排布对齐 HyperGraph 2024 复数图——相位/虚部侧（B 路）在上、
			//幅值/实部侧（A 路）在下；互换态（kComplexSwapped）上下对调
			QVector<double> dispA, dispB;
			convertComplexDisplay(aRaw, bRaw, s.complexFormat, displayFmt, dispA, dispB);
			if (swapped)
			{
				ce.top = dispA;
				ce.bottom = dispB;
			}
			else
			{
				ce.top = dispB;
				ce.bottom = dispA;
			}
			complexes.append(ce);
		}

		//引擎展开方式随图表显示格式（决定下面板轴标题语义等；标题被 Phase 轴
		//节点覆盖口接管，见 applyComplexLowerAxis）
		const ComplexFormat chartFmt = (displayFmt == 0)
			? ComplexFormat::MagPhase : ComplexFormat::RealImag;

		//上下面板高度占比随显示格式（幅值-相位 幅值:相位=3:1、实部-虚部 1:1）；
		//占比跟着幅值面板所在侧走——基础排布幅值侧（A 路）在下，互换态对调
		if (displayFmt == 0)
			_plot->setComplexPanelStretch(swapped ? 3 : 1, swapped ? 1 : 3);
		else
			_plot->setComplexPanelStretch(1, 1);

		for (const ComplexEntry& ce : complexes)
		{
			CurveNode* n = doc->node(ce.nodeId);
			if (!n)
				continue;
			//复数曲线单节点按键下发（R23）：上面板子曲线名=键、下面板子曲线
			//同名内部管理，图例/命中的键经 _nodeByEngineKey 反查节点；键规则
			//与普通曲线一致（无碰撞键=名，引擎复数展开与键规则无耦合）
			const QString engineKey = keyFor(ce.nodeId);
			if (ce.bottom.isEmpty() || ce.top.isEmpty())
			{
				//相位侧为空（缺 b 侧）：上面板单曲线降级
				_plot->updateCurveData(engineKey, ce.xs, ce.top);
				_plot->setCurveStyle(engineKey, makeCurveSettings(n->series, engineKey));
				continue;
			}
			_plot->updateComplexCurveData(engineKey, ce.xs, ce.top, ce.bottom, chartFmt);
			//样式/图例取本节点配置（两面板同色同线型）
			_plot->setComplexCurveStyle(engineKey, makeCurveSettings(n->series, engineKey));
		}

		//普通曲线（非复数分量，如定义曲线数学结果）：走上面板常规路径
		for (const PlainEntry& pe : plains)
		{
			CurveNode* n = doc->node(pe.nodeId);
			if (!n)
				continue;
			const QString engineKey = keyFor(pe.nodeId);
			_plot->updateCurveData(engineKey, pe.xs, pe.ys);
			_plot->setCurveStyle(engineKey, makeCurveSettings(n->series, engineKey));
		}
	}

	//复数曲线单节点显示序列（上/下面板索引对齐）：逐点同时变换两路——上面板
	//y 走 yRef 预变换+主轴刻度/计权，下面板 y 走 complexRef 预变换+Phase 轴
	//（复数图 YAxis2）刻度/计权，x 同源主 X；隐藏区间同索引 NaN 占位（上下面板
	//断口一致且序列严格等长）；非法点两路同索引置 NaN。全部非法返回 false
	bool mCurveChartView::transformedComplexPair(const CurveNode* n, QVector<double>& xs,
		QVector<double>& aOut, QVector<double>& bOut) const
	{
		xs.clear();
		aOut.clear();
		bOut.clear();
		if (!n)
			return false;
		const CurveSeriesData& s = n->series;
		const int nPts = qMin(s.xData.size(), qMin(s.yData.size(), s.complexYData.size()));
		if (nPts <= 0)
			return false;
		const int xMode = axisScaleModeOf(CurveNodeType::XAxis);
		const int yMode = axisScaleModeOf(CurveNodeType::YAxis);
		const int xWeight = axisWeightModeOf(CurveNodeType::XAxis);
		const int yWeight = axisWeightModeOf(CurveNodeType::YAxis);
		const double xDbRef = axisDbRefOf(CurveNodeType::XAxis);
		const double yDbRef = axisDbRefOf(CurveNodeType::YAxis);
		const int yModeB = axisScaleModeOf(CurveNodeType::YAxis2);   //Phase 轴（下面板）
		const int yWeightB = axisWeightModeOf(CurveNodeType::YAxis2);
		const double yDbRefB = axisDbRefOf(CurveNodeType::YAxis2);
		xs.resize(nPts);
		aOut.resize(nPts);
		bOut.resize(nPts);
		int kept = 0;
		for (int i = 0; i < nPts; ++i)
		{
			//编辑曲线"隐藏"区间：上/下面板同索引 NaN 占位（Qwt 折线遇 NaN 断线）
			if (s.isPointHidden(i))
			{
				xs[i] = qQNaN();
				aOut[i] = qQNaN();
				bOut[i] = qQNaN();
				continue;
			}
			double tx = 0.0, tyA = 0.0, tyB = 0.0;
			transformPoint(s.xRef.applyTo(s.xData.at(i)), s.yRef.applyTo(s.yData.at(i)),
				xMode, yMode, xWeight, yWeight, tx, tyA, xDbRef, yDbRef);
			transformPoint(s.xRef.applyTo(s.xData.at(i)), s.complexRef.applyTo(s.complexYData.at(i)),
				xMode, yModeB, xWeight, yWeightB, tx, tyB, xDbRef, yDbRefB);
			if (qIsNaN(tyA) || qIsNaN(tyB))
			{
				//任一路非法（如对数下 ≤0）：两路同索引 NaN，保持对齐
				xs[i] = qQNaN();
				aOut[i] = qQNaN();
				bOut[i] = qQNaN();
				continue;
			}
			xs[i] = tx;
			aOut[i] = tyA;
			bOut[i] = tyB;
			++kept;
		}
		return kept > 0;
	}

	//图表的类别刻度名（柱状图 X 轴）：取图表下第一条带刻度名曲线的
	//xTickLabels（定义曲线"数值"来源写入）；无则返回空（数字刻度）
	static QStringList chartCategoryTickLabels(CurveDocument* doc, int chartId)
	{
		return doc->chartCategoryTickLabels(chartId);
	}

	void mCurveChartView::applyAxis(int axisNodeId, AxisId engineAxis,
		const QString& titleOverride)
	{
		PERF_MARK("applyAxis enter");
		
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(axisNodeId);
		if (!n)
			return;

		//类别刻度（柱状图 X 轴，截图流程"数值定义→柱状图"）：图表带刻度名曲线
		//时下轴按类别渲染；其余轴/图表类型（XY/复数）一律数字刻度
		CurveNode* axisChartNode = doc->node(_chartId);
		const bool barChart = axisChartNode && axisChartNode->chart.chartType == 1;
		const bool complexChart = axisChartNode && axisChartNode->chart.chartType == 2;
		const QStringList categoryLabels =
			(barChart && engineAxis == AxisId::X_Bottom)
			? chartCategoryTickLabels(doc, _chartId) : QStringList();

	//复数图 X1：缩放方式词表仅 Linear/Logarithmic（dB 对频率/时间横轴无意
	//义），历史存量 dB20/dB10 渲染一律按 Logarithmic（与 axisScaleModeOf/
	//面板显示钳制同源，模型值不动），拷贝一份有效配置下发
	CurveAxisData aEff;
	const bool x1LegacyDb = complexChart && n->type == CurveNodeType::XAxis
		&& n->axis.scaleMode > 1;
	if (x1LegacyDb)
	{
		aEff = n->axis;
		aEff.scaleMode = 1;
	}
	const CurveAxisData& a = x1LegacyDb ? aEff : n->axis;
	AxisSettings as;
	//名称不可见时清空标题（引擎无独立标题显隐字段）；颜色映射引擎标题色
	//轴标题同表头支持模板占位符：{p1w1c1.y.MX*} 按位置引用曲线解析、
	//{y.MX*} 等无引用占位符按本图表首条可见曲线解析，非模板文本原样。
	//titleOverride 非空时优先（复数图上面板轴标题=面板分量名，随显示格式
	//派生，不取节点标题）
	as.title = a.titleVisible
		? (titleOverride.isEmpty()
			? resolveBandTemplate(a.title, firstVisibleCurveSeries(doc, _chartId))
			: titleOverride)
		: QString();
	//复数图：分量令牌（MAG|/PHA|/...）合并渲染后无区分意义，轴标题同页眉/图例去令牌
	if (complexChart)
		as.title = complexDisplayName(as.title);
	as.titleColor = a.titleColor;
		as.isVisible = a.visible;
		as.isLog = false;               //恒线性：刻度模式经数据预变换实现（对齐原型变换空间语义）
		as.autoScale = a.autoScale;
		//对数预变换轴（数据已 log10 预变换、刻度=每十倍细分）：轴范围以对数
		//空间显式下发（见 updateAxisRange 的 logPreTransform 分支），中间态
		//replot 不再在原始值空间上跑十倍刻度引擎（实测百万刻度卡死）
		as.logPreTransform = (a.scaleMode != 0);
		as.titleFont = a.titleFont;
		as.labelFont = a.labelFont;
		//网格：主轴常开（对齐原型：1px 实线网格），副轴不画（主轴已画，再画叠出双线）；
		//网格色读所属图表节点（图表属性编辑器"高级"组），取不到图表节点时退回默认浅灰
		//极坐标接管画布（chartType=3）：直角主/次网格一律不画（需求 04 原型卡上
		//只有极坐标底图，2026-11-12 真机复验实锤 8 竖 5 横直角网格残留）
		const bool isPrimary = (engineAxis == AxisId::X_Bottom || engineAxis == AxisId::Y_Left);
		const CurveNode* chartNodeForGrid = doc->node(_chartId);
		const bool polarChart = chartNodeForGrid
			&& chartNodeForGrid->chart.chartType == 3;
		as.showMajorGrid = isPrimary && a.scaleMode == 0 && !polarChart;
		//网格分工（HyperGraph2024 语义）：主刻度（每十倍刻度数）只作用于轴上
		//的刻度标记不生成网格——log/dB 轴关主网格，画布网格仅由"每十倍网格
		//线数"决定（次网格承载，含数量级边界，见 mCurveDecadeTickDiv）；
		//linear 轴主网格照旧画在主刻度上，次网格看"每主刻度网格数"。副轴不
		//开网格（范围与主轴不同步，叠画错位）
		as.showMinorGrid = isPrimary
			&& ((a.scaleMode == 0 && a.minorCount > 0)
				|| (a.scaleMode != 0 && a.decadeMinorCount > 0))
			&& !polarChart;
		//主/次网格样式统一：颜色取所属图表的网格色，1px 实线
		QColor gridColor(220, 220, 220);
		if (CurveNode* chartNode = doc->node(_chartId))
			gridColor = chartNode->chart.gridColor;
		as.majorGridColor = gridColor;
		as.majorGridWidth = 1.0;
		as.majorGridStyle = Qt::SolidLine;
		as.minorGridColor = gridColor;
		as.minorGridWidth = 1.0;
		as.minorGridStyle = Qt::SolidLine;

		//引擎上/右轴为副轴专用：启用刻度与数字组件（构造期默认全关，对齐"上/右轴
		//完全不显示"的初始状态）；主干线保持关——与下/左轴观感一致（闭合边框由
		//BorderOverlay 绘制）
		if (!isPrimary)
		{
			QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
			if (QwtAbstractScaleDraw* sd = qwt ? qwt->axisScaleDraw(static_cast<QwtPlot::Axis>(engineAxis)) : nullptr)
			{
				sd->enableComponent(QwtAbstractScaleDraw::Backbone, false);
				sd->enableComponent(QwtAbstractScaleDraw::Ticks, true);
				sd->enableComponent(QwtAbstractScaleDraw::Labels, true);
			}
		}

		//刻度生成方式（设计图11/12）：均匀分步法按"刻度总数"等分当前范围
		//（主刻度数字个数=刻度总数，含两端，缩放后引擎重入仍保持个数），并按
		//"每主刻度网格数"细分次刻度；固定步长法下发固定步长（绝对倍数对齐，
		//0=自动）；每十倍分步法（log/dB 默认）装"每十倍"引擎——每十倍刻度数
		//把每个数量级等分出主刻度、每十倍网格线数细分次刻度（HyperGraph2024
		//语义，见 mCurveDecadeTickDiv）。轴为数据预变换+线性轴，均匀/固定步长
		//引擎与缩放方式无关（log/dB 下即在刻度变换空间等分/取步长）。
		//每次重装引擎：面板切换后旧引擎配置不残留。
		//类别轴优先：主刻度恒步长 1（每个类别一格）、无次刻度，面板刻度设置
		//（刻度总数/固定步长/对数十倍）不参与——刻度文字由 mCurveScaleDraw 按
		//类别名渲染
		//刻度引擎按同类设置各造一份实例（Qwt 轴拥有引擎，两面板不能共享对象）
		auto makeTickEngine = [&a]() -> QwtScaleEngine* {
			//生成方式按缩放方式分派（值=面板下拉索引）：linear 0均匀（tickCount<=1 自动档）/1固定
			//步长；log/dB 0每十倍（遗留默认）/1均匀/2固定步长，非法/缺省值
			//回落每十倍（log/dB）或自动（linear）
			if (a.scaleMode != 0)
			{
				if (a.tickGenMode == 1 && a.tickCount >= 2.0)
					return new mCurveUniformTickEngine(a.tickCount, a.minorCount);
				if (a.tickGenMode == 2 && a.majorStep > 0.0)
				{
					auto* tickEngine = new PlotView::TecPlotLinearScaleEngine();
					tickEngine->setFixedStep(a.majorStep);
					if (a.minorCount > 0)
						tickEngine->setMinTickCount(a.minorCount);
					return tickEngine;
				}
				return new mCurveDecadeTickEngine(decadeWidthOf(a.scaleMode),
					a.decadeTickCount, a.decadeMinorCount);
			}
			//tickCount<=1 走引擎内自动档（12 主刻度上限，2026-11-16 裁定③自动档）
			if (a.tickGenMode == 0)
				return new mCurveUniformTickEngine(a.tickCount, a.minorCount);
			//tickGenMode 2=列表标注法（2d-chart-split-align）：主刻度=频率点列表
			//界内成员（空/界内无成员回落 Qwt 自动——引擎内回落）
			if (a.tickGenMode == 2)
				return new mCurveListTickEngine(a.tickValues);
			auto* tickEngine = new PlotView::TecPlotLinearScaleEngine();
			if (a.tickGenMode == 1 && a.majorStep > 0.0)
				tickEngine->setFixedStep(a.majorStep);
			if (a.minorCount > 0)
				tickEngine->setMinTickCount(a.minorCount);   //每主刻度段细分个数
			return tickEngine;
		};
		if (!categoryLabels.isEmpty())
		{
			auto* tickEngine = new PlotView::TecPlotLinearScaleEngine();
			tickEngine->setFixedStep(1.0);
			tickEngine->setMinTickCount(0);
			_plot->getQwtPlot()->setAxisScaleEngine(
				static_cast<QwtPlot::Axis>(engineAxis), tickEngine);
		}
		else
		{
			_plot->getQwtPlot()->setAxisScaleEngine(
				static_cast<QwtPlot::Axis>(engineAxis), makeTickEngine());
		}

		//复数图下面板装同配置引擎（X 下/Y 左两根共享轴）：否则下面板缩放/平移
		//走默认引擎不保持"刻度总数/每主刻度网格数"等设置，且 X 联动
		//（scaleDivChanged 双向同步）会把下面板的默认刻度排布同步回上面板，
		//上面板的设置也跟着失效
		if (complexChart && (engineAxis == AxisId::X_Bottom
			|| engineAxis == AxisId::Y_Left))
		{
			if (QwtPlot* lowerQwt = _plot->getLowerQwtPlot())
			{
				lowerQwt->setAxisScaleEngine(
					static_cast<QwtPlot::Axis>(engineAxis), makeTickEngine());
				//刻度数字格式/精度同上面板：下面板默认装的是 SafeScaleDraw，
				//换成 mCurveScaleDraw（其继承 SafeScaleDraw，引擎内向下转型不受
				//影响，见 mCurveAxisStripWidget.h 注释）
				const QwtPlot::Axis qwtAxis = static_cast<QwtPlot::Axis>(engineAxis);
				if (!dynamic_cast<mCurveScaleDraw*>(lowerQwt->axisScaleDraw(qwtAxis)))
					lowerQwt->setAxisScaleDraw(qwtAxis, new mCurveScaleDraw());
				if (auto* scaleDraw = dynamic_cast<mCurveScaleDraw*>(
					lowerQwt->axisScaleDraw(qwtAxis)))
				{
					scaleDraw->setNumberFormat(a.labelFormat, a.precision);
					//主干线关（同 createLowerPlot 对默认 SafeScaleDraw 的处理）：
					//新换的 mCurveScaleDraw 主干线默认开，会在画布边框外侧
					//再画一圈平行线，观感成"边框外的线"
					scaleDraw->enableComponent(QwtAbstractScaleDraw::Backbone, false);
				}
				//强制按新引擎重排当前区间刻度：下面板刻度 Div 常是联动拷贝来的
				//固定划分（setAxisScaleDiv），对固定划分 replot 不会重跑引擎，
				//只换引擎会出现"上面板已按新设置重排、下面板还是旧刻度划分"
				const QwtScaleDiv div = lowerQwt->axisScaleDiv(qwtAxis);
				if (div.lowerBound() < div.upperBound())
				{
					lowerQwt->setAxisScale(qwtAxis,
						div.lowerBound(), div.upperBound());
					lowerQwt->replot();   //立即按新引擎/格式重排当前区间刻度
				}
			}
		}

		//刻度数字格式/精度（kLabelFormat/kPrecision，坐标轴刻度设置）：引擎轴
		//刻度数字由构造期统一安装的 mCurveScaleDraw 格式化，此处把当前轴的
		//格式选择下发。注意 QwtScaleWidget::setScaleDiv 在刻度划分未变化时
		//不重布局（格式/精度只改数字样式不改刻度位置，下方 updateAxisScale
		//的 replot 不会重画刻度条），须显式重paint刻度条让 label() 重跑
		if (QwtPlot* qwt = _plot->getQwtPlot())
		{
			const QwtPlot::Axis qwtAxis = static_cast<QwtPlot::Axis>(engineAxis);
			if (auto* scaleDraw = dynamic_cast<mCurveScaleDraw*>(qwt->axisScaleDraw(qwtAxis)))
			{
				scaleDraw->setNumberFormat(a.labelFormat, a.precision);
				scaleDraw->setTickLabels(categoryLabels);   //类别刻度（空=数字刻度）
				//对数轴标签还原（数据预变换架构）：Log 档刻度位置为 log10 工程
				//值，开启后按 10^v 还原科学计数显示（对齐原型 1.0E-04 形态）。
				//档位收窄 Log 档：dB 档刻度值是 dB 工程值应直显，旧 `!= 0` 会
				//把正 dB 刻度还原成 1.0E+xx（2026-11-16 nvh-log-scale-prototype-
				//align 裁定修正）
				scaleDraw->setLogValueLabels(a.scaleMode == 1);
				//类别轴标签对齐方式（kCategoryLabelAlign，参数栏"标签对齐方式"，
				//仅柱状图 X 下轴消费）：0水平/1对角 / 斜向/2垂直。Qt 正角顺时针，
				//负角逆时针——斜向 -45° 文字从左下往右上斜出（matplotlib
				//rotation=45 同款观感），垂直 -90° 自下而上；右对齐让文字末端钩在
				//刻度下方（旋转后不遮曲线）。数字刻度同样旋转（文件来源柱状图无
				//类别名，刻度仍是数字）；其余图表/轴恢复 Qwt 默认对齐，防残留
				//（见下方 else 分支）
				const int labelAlign = a.categoryLabelAlign;
				const bool barXAxis = barChart && engineAxis == AxisId::X_Bottom;
				//档位 3=需求稿 02 斜排约 30°（fix-modal-contribution-chart-display
				//任务 3.4/D10 新增；1=对角 -45°、2=垂直 -90° 既有档不动，参数栏
				//"标签对齐方式"枚举下拉同轮补选项）
				if (barXAxis && (labelAlign == 1 || labelAlign == 2 || labelAlign == 3))
				{
					scaleDraw->setLabelRotation(
						(labelAlign == 1) ? -45.0 : (labelAlign == 3) ? -30.0 : -90.0);
					scaleDraw->setLabelAlignment(
						Qt::Alignment(Qt::AlignRight | Qt::AlignBottom));
				}
				else
				{
					scaleDraw->setLabelRotation(0.0);
					//清 0 对齐标志：Qwt（labelTransformation）在 flags==0 时按轴
					//方位取默认——左轴 AlignLeft|VCenter（标签整体画在锚点左侧）、
					//右轴 AlignRight|VCenter、下轴 HCenter|Bottom、上轴 HCenter|Top。
					//不能按直觉手写：这里的 AlignLeft/Right 指"画在锚点哪一侧"
					//（AlignRight=x0 起向右画），写反会把标签推进画布被盖住
					//（左轴数字末位缺半截），下轴写 AlignTop 会贴住刻度线
					scaleDraw->setLabelAlignment(Qt::Alignment());
				}
			}
			if (QwtScaleWidget* scaleWidget = qwt->axisWidget(qwtAxis))
				scaleWidget->update();
		}

		//log/dB 轴：次刻度标记不画（用户约定"网格线下方的刻度去掉"），次刻度
		//位置只承载画布网格线（QwtPlotGrid 按 tick 位置画线、与刻度长度无关）
		if (a.scaleMode != 0)
			as.minorTickLen = 0;

		//复数图共享 X（截图交互）：X 刻度数值与标题（页脚位）只落下面板，
		//上面板 X 轴整体隐藏——rebuild 中 updateAxisScale 会按轴节点写回可见
		//性，此处按图表类型压回；上面板刻度 Div 仍正常维护（下面板经
		//syncComplexXFrom 跟随同一范围）
		if (complexChart && engineAxis == AxisId::X_Bottom)
			as.isVisible = false;

		_plot->updateAxisScale(engineAxis, as);
		PERF_MARK("applyAxis updateAxisScale done");
		_plot->updateAxisAppearance(engineAxis, as);
		PERF_MARK("applyAxis updateAxisAppearance done");
		_plot->updateAxisGrid(engineAxis, as);   //网格必须走专用接口（Scale/Appearance 不处理网格字段）
		PERF_MARK("applyAxis updateAxisGrid done");

		//复数图：刷新下面板轴外观/网格画笔。下面板网格画笔只在模式切换时
		//（refreshComplexPanelAxes 的定时路径）配置过，宿主改刻度设置不经过
		//它——"每主刻度网格数=0"时上面板次网格画笔随 updateAxisGrid 关闭，
		//下面板却残留为开，网格显得比上面板密。此调用须在 updateAxisScale
		//之后（其内部读取刚存入的 m_axisSettings）
		if (complexChart && (engineAxis == AxisId::X_Bottom
			|| engineAxis == AxisId::Y_Left))
			_plot->refreshComplexPanelAxes();

		//手动范围：模型存原始值，变换到显示空间后下发；
		//自动范围由 fitChartAxes() 在曲线应用后按数据统一计算。
		//边界：手动范围为无频率上下文的标量，仅做刻度变换不加权（计权
		//按数据点频率逐点生效，见 transformPoint；与 displayedAxisRange
		//的标量逆变换对称）
		if (!a.autoScale)
		{
			AxisSettings rs = as;
			rs.autoScale = false;
			//log/dB 下原始值<=0 变换为 NaN（数学无定义），NaN 直接进轴范围会
			//得到坏轴（曲线整体压扁/裁没）：先在原始空间钳到正数域再变换，
			//保证轴范围恒有效（linear 恒等不受影响）
			double rawLo = a.min;
			double rawHi = a.max;
			if (a.scaleMode != 0)
			{
				rawLo = qMax(rawLo, 1e-6);
				rawHi = qMax(rawHi, rawLo);
			}
			rs.min = transformValue(rawLo, a.scaleMode, a.dbRef);
			rs.max = transformValue(rawHi, a.scaleMode, a.dbRef);
			_plot->updateAxisRange(engineAxis, rs);
		}
	}

	void mCurveChartView::applyLegend(int legendNodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(legendNodeId);
		if (!n || n->type != CurveNodeType::Legend)
			return;
		const CurveLegendData& lg = n->legend;

		//显隐/字体/背景走 PlotSettings（引擎统一应用）；字体加粗对齐原型图例
		PlotView::PlotSettings ps = _plot->getPlotSettings();
		ps.showLegend = lg.visible;
		ps.legendFont = lg.font;
		ps.legendFont.setBold(true);
		ps.legendBgColor = lg.bgColor;
		_plot->setPlotSettings(ps);

		PlotView::mPlotLegendItem* legend = _plot->getPlotLegendItem();
		if (!legend)
			return;
		legend->setBorderColor(lg.borderColor);
		//线宽与线型联动：线宽 0 = 无边框（引擎笔取 NoPen）；从"无"改回具体线宽
		//时线型若仍为 0（原样式已无从得知）恢复实线
		legend->setBorderStyle(lg.borderWidth > 0 ? qMax(1, lg.borderStyle) : 0);
		legend->setBorderWidth(lg.borderWidth);            //0无边框 1..4=线宽px（"边框线宽"行）
		legend->setTextFollowCurveColor(lg.textColorMode == 0);
		legend->setTextColor(lg.textColor);
		legend->setIconBelowText(lg.leaderPos == 1);       //引线位置：0左侧 1下方
		legend->setSelected(legendNodeId == _selectedLegendId);   //选中态重放

		if (lg.autoPosition)
			applyLegendAutoPosition();
		else
			applyLegendPosition(lg);
	}

	void mCurveChartView::applyLegendPosition(const CurveLegendData& lg)
	{
		PlotView::mPlotLegendItem* legend = _plot->getPlotLegendItem();
		if (!legend)
			return;
		using LP = PlotView::LegendPosition;
		switch (lg.position)
		{
		case 0: legend->setPosition(LP::TopLeft); break;
		case 1: legend->setPosition(LP::BottomLeft); break;
		case 3: legend->setPosition(LP::BottomRight); break;
		case 4:   //图内-用户自定义：画布百分比下发浮点位置
			legend->setPosition(LP::Floating);
			legend->setFloatingPos(lg.posX / 100.0, lg.posY / 100.0);
			break;
		case 5: legend->setPosition(LP::OutsideTop); break;
		case 6: legend->setPosition(LP::OutsideBottom); break;
		case 7: legend->setPosition(LP::OutsideLeft); break;
		case 8: legend->setPosition(LP::OutsideRight); break;
		default:
			//2=图内-右上角
			legend->setPosition(LP::TopRight);
			break;
		}
	}

	void mCurveChartView::applyLegendAutoPosition()
	{
		QwtPlot* qwt = _plot->getQwtPlot();
		PlotView::mPlotLegendItem* legend = _plot->getPlotLegendItem();
		if (!qwt || !legend || !legend->isVisible())
			return;
		//图内四角按偏好顺序试摆（优先右上，设计图08提示语义）：取首个不遮挡
		//可见曲线的位置；全部遮挡时取遮挡采样点最少者。图例包围盒经 replot
		// 后的 legendRect() 读取（引擎 draw 时缓存）
		using LP = PlotView::LegendPosition;
		const LP candidates[] = { LP::TopRight, LP::TopLeft, LP::BottomRight, LP::BottomLeft };
		LP best = candidates[0];
		int bestOverlap = std::numeric_limits<int>::max();
		for (LP cand : candidates)
		{
			legend->setPosition(cand);
			qwt->replot();
			const int overlap = countCurvePointsInRect(legend->legendRect());
			if (overlap == 0)
			{
				best = cand;
				break;
			}
			if (overlap < bestOverlap)
			{
				bestOverlap = overlap;
				best = cand;
			}
		}
		legend->setPosition(best);
	}

	void mCurveChartView::applyLegendSelection()
	{
		PlotView::mPlotLegendItem* legend = _plot ? _plot->getPlotLegendItem() : nullptr;
		if (!legend)
			return;
		legend->setSelected(_selectedLegendId > 0);
		if (QwtPlot* qwt = _plot->getQwtPlot())
			qwt->replot();
	}

	int mCurveChartView::countCurvePointsInRect(const QRect& rect) const
	{
		if (!rect.isValid())
			return 0;
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!qwt)
			return 0;
		const QwtScaleMap xm = qwt->canvasMap(QwtPlot::xBottom);
		const QwtScaleMap ym = qwt->canvasMap(QwtPlot::yLeft);
		int count = 0;
		for (const QString& name : _plot->getCurveNames())
		{
			const QwtPlotCurve* c = _plot->getCurve(name);
			if (!c || !c->isVisible() || c->dataSize() == 0)
				continue;
			//pch 采样点可达上万，按约 200 点/线抽样（足以判定遮挡关系）
			const int n = static_cast<int>(c->dataSize());
			const int step = qMax(1, n / 200);
			for (int i = 0; i < n; i += step)
			{
				const QPointF p = c->sample(i);
				if (rect.contains(qRound(xm.transform(p.x())), qRound(ym.transform(p.y()))))
					++count;
			}
		}
		return count;
	}

	void mCurveChartView::applyHeader()
	{
		//页眉数据挂在图表节点上（属性分发与属性页均按 Chart 类型读写 kHeader*）；
		//Header 子节点只是树的占位节点，自身数据结构不用
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chart = doc->node(_chartId);
		if (!chart)
			return;

		//PlotView 页眉条带（绘图区上方）：开关打开且有文本时显示，否则隐藏条带
		//（引擎约定：空文本即隐藏）。多级标题按行分级下发（一级/二级/三级），
		//对齐统一取模型值，保持导出取值一致。
		//逐行颜色/加粗（仿 applyComplexLowerAxis"按选中态着色"先例）：选中态=
		//主题蓝+全行加粗（视图状态不进模型/命令栈）；未选中=模型色
		//（CurveTextData.color，属性页"颜色"行 kHeaderColor 下发；无效兜底黑）
		//+模型字体原样。kHeaderColor 走 Chart 节点 scheduleRebuild 默认路径，
		//重建内经本函数下发，改色即时生效
		const CurveTextData& h = chart->header;
		QColor lineColor;
		bool lineBold = false;
		if (_selectedBandId > 0 && _selectedBandId == bandNodeId(CurveNodeType::Header))
		{
			lineColor = curveHighlightBlue();
			lineBold = true;
		}
		else
			lineColor = h.color.isValid() ? h.color : QColor(Qt::black);
		if (h.visible && !h.text.isEmpty())
		{
			//表头支持"标签内容"同款模板占位符：{p1w1c1.y.MXRequest} 按位置引用
			//曲线解析（同参考软件建图默认头），{y.MXRequest} 等无引用占位符按
			//图表内首条可见曲线解析；两者都不可用（无可见曲线/未知占位符）时
			//token 原样保留，手工输入的非模板文本不受影响
			QString text = resolveBandTemplate(h.text,
				firstVisibleCurveSeries(doc, _chartId));
			//复数图：MAG/PHA（REAL/IMAG）已合并渲染，分量令牌显示无区分意义，
			//同树显示约定去令牌（XY 图不合并，令牌保留）
			if (chart->chart.chartType == 2)
				text = complexDisplayName(text);
			_plot->setHeaderLines(bandLinesOf(text, h.font, h.font2, h.font3,
				bandAlignmentOf(h.align), lineColor, lineBold, h.emphLine));
		}
		else
			_plot->setHeaderText(QString());
	}

	void mCurveChartView::applyFooter()
	{
		//页脚数据同样挂在图表节点上（kFooter*），接入方式与页眉对称
		//（PlotView 页脚条带位于绘图区下方）。选中态/模型色着色同 applyHeader
		//（kFooterColor 下发，无效兜底黑）
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chart = doc->node(_chartId);
		if (!chart)
			return;

		const CurveTextData& f = chart->footer;
		QColor lineColor;
		bool lineBold = false;
		if (_selectedBandId > 0 && _selectedBandId == bandNodeId(CurveNodeType::Footer))
		{
			lineColor = curveHighlightBlue();
			lineBold = true;
		}
		else
			lineColor = f.color.isValid() ? f.color : QColor(Qt::black);
		if (f.visible && !f.text.isEmpty())
		{
			//页脚与页眉同款：模板占位符（{p1w1c1.y.MX*}/{y.MX*}）解析后下发，
			//非模板文本原样；复数图去分量令牌（同页眉）
			QString text = resolveBandTemplate(f.text,
				firstVisibleCurveSeries(doc, _chartId));
			if (chart->chart.chartType == 2)
				text = complexDisplayName(text);
			_plot->setFooterLines(bandLinesOf(text, f.font, f.font2, f.font3,
				bandAlignmentOf(f.align), lineColor, lineBold));
		}
		else
			_plot->setFooterText(QString());
	}

	void mCurveChartView::fitChartAxes()
	{
		PERF_MARK("fitChartAxes enter");
		
		CurveDocument* doc = mCurveDocController::getInstance()->document();

		//柱状图形态：柱基线在 Y=0（引擎 barBaseValue 默认 0），
		//自动范围须把 0 包进 Y 轴，否则全正/全负数据的柱子被轴范围裁剪悬空
		CurveNode* chartNode = doc->node(_chartId);
		const bool barChart = chartNode && chartNode->chart.chartType == 1;
		//复数图形态（曲线单节点自带上/下面板两路数据）：基础排布（HyperGraph
		//2024 对齐）上面板显示相位/虚部侧（B 路），Y_Left 引擎轴按排布归属
		//上面板轴节点，Y 适配按显示数据计算（见下方 fitTopDisp 分支）；互换态
		//上面板为幅值/实部侧（A 路）。下面板 Y 范围由引擎自动适配；X 侧照常
		//参与（上下面板共享横坐标）
		const bool complexChart = chartNode && chartNode->chart.chartType == 2;
		//互换态（设计图01"互换复数坐标轴"）：上下面板对调（上面板回到幅值/
		//实部侧），上面板 Y 适配数据随之切换（见下方 fitTopDisp 分支）
		const bool chartSwapped = chartNode && chartNode->chart.complexSwapped;
		const int displayFmt = chartNode ? chartNode->chart.complexDisplayFormat : 0;

		//极坐标画布接管（design D5）：直角轴带隐藏（复用轴可见性通道），两轴
		//定死 [-1.05,1.05] 仅为不可见载体曲线提供合法映射空间（自绘 item 直接
		//按 canvasRect 计算几何，不经轴映射，圆不随画布拉伸变椭圆）；半径域/
		//刻度归一收口 applyPolarDecor
		if (chartNode && chartNode->chart.chartType == 3)
		{
			if (QwtPlot* qwt = _plot->getQwtPlot())
			{
				qwt->setAxisScale(QwtPlot::xBottom, -1.05, 1.05);
				qwt->setAxisScale(QwtPlot::yLeft, -1.05, 1.05);
				qwt->enableAxis(QwtPlot::xBottom, false);
				qwt->enableAxis(QwtPlot::yLeft, false);
			}
			return;
		}

		struct AxisFit
		{
			CurveNodeType nodeType;   //轴节点类型（副轴取同侧首枚）
			AxisId axisId;
			bool isX;
			int requireAssign;    //-1=全部可见曲线；1=挂本副轴的曲线（无匹配回退全部）
		};
		const AxisFit fits[] = {
			{ CurveNodeType::XAxis, AxisId::X_Bottom, true, -1 },
			{ CurveNodeType::YAxis, AxisId::Y_Left, false, -1 },
			{ CurveNodeType::XAxis2, AxisId::X_Top, true, 1 },
			{ CurveNodeType::YAxis2, AxisId::Y_Right, false, 1 },
		};

		//收集全部可见曲线的（变换后）数据范围
		for (const AxisFit& fit : fits)
		{
			//复数图：Y_Right 引擎轴恒隐藏，YAxis2 拟合无目标（下面板 Y 范围由
			//引擎自动适配，见 applyComplexLowerAxis），跳过
			if (complexChart && fit.nodeType == CurveNodeType::YAxis2)
				continue;
			CurveNode* axisNode = nullptr;
			for (int childId : doc->childNodeIds(_chartId))
			{
				CurveNode* c = doc->node(childId);
				if (c && c->type == fit.nodeType)
				{
					axisNode = c;
					break;
				}
			}
			//复数图：Y_Left 引擎轴按排布归属上面板轴节点（默认相位/虚部侧
			//YAxis2，互换态为幅值/实部侧 YAxis）——适配门槛/刻度计权取该节点
			if (complexChart && fit.nodeType == CurveNodeType::YAxis)
			{
				int topAxisId = -1;
				complexPanelAxisIds(&topAxisId, nullptr);
				axisNode = topAxisId > 0 ? doc->node(topAxisId) : nullptr;
			}
			if (!axisNode || !axisNode->axis.autoScale)
				continue;

			int mode = axisNode->axis.scaleMode;
			//复数图 X1：历史存量 dB 缩放按 Logarithmic 适配（词表收窄同源）
			if (complexChart && fit.nodeType == CurveNodeType::XAxis && mode > 1)
				mode = 1;
			int weightMode = axisNode->axis.weightMode;   //与渲染同源：范围按计权后的点取
			const double axisDbRef = axisNode->axis.dbRef;   //与渲染同源：dB 除入参考值
			double tMin = std::numeric_limits<double>::max();
			double tMax = std::numeric_limits<double>::lowest();
			double fMin = tMin, fMax = tMax;   //挂本副轴曲线的范围（requireAssign 用）
			int fitCount = 0, assignCount = 0;
			for (int childId : doc->childNodeIds(_chartId))
			{
				CurveNode* c = doc->node(childId);
				if (!c || c->type != CurveNodeType::Curve || !c->series.visible)
					continue;
				//复数曲线单节点：上面板显示哪一路由排布决定（默认相位/虚部侧
				//dispB，互换态 dispA），Y_Left 适配按"显示格式换算后的上面板
				//序列"计算（y 走 complexRef 预变换；隐藏区间同掩码 NaN，适配
				//跳过与渲染一致）。幅值/实部侧在下面板，由引擎自动适配，不经
				//本循环（见 applyComplexLowerAxis）
				QVector<double> dispXs, dispYs;
				const bool fitTopDisp = (!fit.isX && complexChart
					&& c->series.isComplex());
				if (fitTopDisp)
				{
					//上面板显示序列：同索引对齐变换 + 显示格式换算后按排布取
					QVector<double> aRaw, bRaw;
					transformedComplexPair(c, dispXs, aRaw, bRaw);
					QVector<double> dispA, dispB;
					convertComplexDisplay(aRaw, bRaw, c->series.complexFormat,
						displayFmt, dispA, dispB);
					dispYs = chartSwapped ? dispA : dispB;
				}
				const int assign = fit.isX ? c->series.xRef.axisAssign : c->series.yRef.axisAssign;
				if (fitTopDisp)
				{
					const int nPts = qMin(dispXs.size(), dispYs.size());
					for (int i = 0; i < nPts; ++i)
					{
						const double tx = dispXs.at(i);
						const double ty = dispYs.at(i);
						if (qIsNaN(tx) || qIsNaN(ty))
							continue;
						const double t = ty;
						tMin = qMin(tMin, t);
						tMax = qMax(tMax, t);
						++fitCount;
					}
					continue;
				}
				int count = qMin(c->series.xData.size(), c->series.yData.size());
				for (int i = 0; i < count; ++i)
				{
					if (c->series.isPointHidden(i))
						continue;   //隐藏区间内点不参与自动范围（与渲染断口一致）
					double tx = 0.0, ty = 0.0;
					//与渲染同源：先经 x/y 侧 缩放系数->偏移量 预变换再计权/刻度变换
					transformPoint(c->series.xRef.applyTo(c->series.xData.at(i)),
						c->series.yRef.applyTo(c->series.yData.at(i)),
						mode, mode, weightMode, weightMode, tx, ty, axisDbRef, axisDbRef);
					double t = fit.isX ? tx : ty;
					if (qIsNaN(t))
						continue;
					tMin = qMin(tMin, t);
					tMax = qMax(tMax, t);
					++fitCount;
					if (assign >= 1)
					{
						fMin = qMin(fMin, t);
						fMax = qMax(fMax, t);
						++assignCount;
					}
				}
			}
			//副轴优先取挂本轴曲线的范围；无挂载曲线（如刚创建）时回退同侧主轴
			//重建前的当前显示范围——缩放后再创建的副轴与当前视图一致，而非按
			//全量数据重新适配（非保留视图的重建无记录，保持全部曲线范围回退）
			if (fit.requireAssign == 1 && assignCount > 0)
			{
				tMin = fMin;
				tMax = fMax;
			}
			else if (fit.requireAssign == 1 && _hasPreRebuildDiv)
			{
				const double* pre = fit.isX ? _preRebuildXDiv : _preRebuildYDiv;
				tMin = pre[0];
				tMax = pre[1];
			}
			if (fitCount == 0 || tMin > tMax)
				continue;   //无有效数据，保持当前范围

			//柱状形态：Y 轴范围扩到基线 0
			if (barChart && !fit.isX)
			{
				tMin = qMin(tMin, 0.0);
				tMax = qMax(tMax, 0.0);
			}
			//类别柱状（带刻度名）：X 范围外扩半格，柱子画在整数位上，精确贴合
			//会把首尾柱裁半
			if (barChart && fit.isX
				&& !chartCategoryTickLabels(doc, _chartId).isEmpty())
			{
				tMin -= 0.5;
				tMax += 0.5;
			}

			//适配精确贴合数据边界（不留白）：屏幕左缘=曲线最小值、右缘=X 最大值、
			//下缘=Y 最小值、上缘=Y 最大值（变换空间内取值，刻度模式切换后范围
			//随之可见变化）；仅退化范围（单点/常数列）扩 ±1 保底成轴
			if (tMax - tMin < 1e-12)
			{
				tMin -= 1.0;
				tMax += 1.0;
			}

			AxisSettings as = _plot->getAxisSettings(fit.axisId);
			as.autoScale = false;
			as.min = tMin;
			as.max = tMax;
			as.logPreTransform = (mode != 0);   //对数预变换轴：范围即对数空间，显式下发
			_plot->updateAxisRange(fit.axisId, as);
		}

		//自动范围刚被重算（缩放方式切换/曲线增删后）：广播新现值，轴属性页
		//"最小值/最大值"随新刻度空间联动回显（面板只回显数字，不写模型）
		notifyAxisDisplayRanges();
	}

	void mCurveChartView::addZeroCrossLines()
	{
		//对齐原型：X/Y 都为 Linear 时绘制黑色 2px 实线零线，样式按图表类型
		//profile（ZeroLineStyle，ChartTypeProfile 渲染行为节）：Cross=十字
		//x=0/y=0（XY/复数现状）；BaselineOnly=仅水平基线"一"（柱状目标形态，
		//柱基线 y=0，竖线 x=0 不画）；None=不画
		if (axisScaleModeOf(CurveNodeType::XAxis) != 0 || axisScaleModeOf(CurveNodeType::YAxis) != 0)
			return;

		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chartNode = doc->node(_chartId);
		const auto zeroStyle = chartNode
			? chartTypeProfile(chartNode->chart.chartType).zeroLineStyle
			: ChartTypeProfile::ZeroLineStyle::Cross;

		if (PlotView::mPlotMarkerLine* v = _plot->getMarkerLine("_zeroV"))
			_plot->removeMarkerLine("_zeroV");
		if (PlotView::mPlotMarkerLine* h = _plot->getMarkerLine("_zeroH"))
			_plot->removeMarkerLine("_zeroH");

		//竖线 x=0（"I"）仅 Cross 绘制——柱状图切类型后旧线不残留
		if (zeroStyle == ChartTypeProfile::ZeroLineStyle::Cross)
		{
			_plot->addMarkerLine("_zeroV", PlotView::mPlotMarkerLine::Vertical, 0.0);
			if (PlotView::mPlotMarkerLine* v = _plot->getMarkerLine("_zeroV"))
			{
				v->setLineColor(Qt::black);
				v->setLineWidth(2.0);
				v->setLineStyle(Qt::SolidLine);
			}
		}
		//横线 y=0（"一"）Cross/BaselineOnly 都绘制
		if (zeroStyle != ChartTypeProfile::ZeroLineStyle::None)
		{
			_plot->addMarkerLine("_zeroH", PlotView::mPlotMarkerLine::Horizontal, 0.0);
			if (PlotView::mPlotMarkerLine* h = _plot->getMarkerLine("_zeroH"))
			{
				h->setLineColor(Qt::black);
				h->setLineWidth(2.0);
				h->setLineStyle(Qt::SolidLine);
			}
		}

		//复数图下面板同绘零线十字（相位面板与幅值面板同语义；随重建移除重建）
		if (_plot->isComplexMode())
		{
			if (PlotView::mPlotMarkerLine* v = _plot->getMarkerLine("_zeroV", true))
				_plot->removeMarkerLine("_zeroV", true);
			if (PlotView::mPlotMarkerLine* h = _plot->getMarkerLine("_zeroH", true))
				_plot->removeMarkerLine("_zeroH", true);
			_plot->addMarkerLine("_zeroV", PlotView::mPlotMarkerLine::Vertical, 0.0, true);
			_plot->addMarkerLine("_zeroH", PlotView::mPlotMarkerLine::Horizontal, 0.0, true);
			if (PlotView::mPlotMarkerLine* v = _plot->getMarkerLine("_zeroV", true))
			{
				v->setLineColor(Qt::black);
				v->setLineWidth(2.0);
				v->setLineStyle(Qt::SolidLine);
			}
			if (PlotView::mPlotMarkerLine* h = _plot->getMarkerLine("_zeroH", true))
			{
				h->setLineColor(Qt::black);
				h->setLineWidth(2.0);
				h->setLineStyle(Qt::SolidLine);
			}
		}
	}

	int mCurveChartView::axisScaleModeOf(CurveNodeType type) const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* c = doc->node(childId);
			if (c && c->type == type)
			{
				//复数图 X1：历史存量 dB 缩放按 Logarithmic 参与数据变换/适配
				//（词表仅 Linear/Logarithmic，dB 对频率/时间横轴无意义；与
				//applyAxis/面板显示钳制同源，模型值不动）
				if (type == CurveNodeType::XAxis && c->axis.scaleMode > 1)
				{
					CurveNode* chart = doc->node(_chartId);
					if (chart && chart->chart.chartType == 2)
						return 1;
				}
				return c->axis.scaleMode;
			}
		}
		return 0;
	}

	int mCurveChartView::axisWeightModeOf(CurveNodeType type) const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* c = doc->node(childId);
			if (c && c->type == type)
				return c->axis.weightMode;
		}
		return 0;
	}

	double mCurveChartView::axisDbRefOf(CurveNodeType type) const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* c = doc->node(childId);
			if (c && c->type == type)
				return c->axis.dbRef;
		}
		return 1.0;
	}

	//--- 注释交互（Ribbon"注释"按钮经视口工厂广播到此） ---

	//视图注释占位文本 kViewAnnotationPlaceholder 见 CurveDefine.h（与属性页共用）

	//三点 Menger 曲率（外接圆半径倒数）：2*两倍叉积面积/(三边长乘积)；共线/重合 -> 0
	static double mengerCurvature(const QPointF& a, const QPointF& b, const QPointF& c)
	{
		const double area2 = std::fabs((b.x() - a.x()) * (c.y() - a.y())
			- (c.x() - a.x()) * (b.y() - a.y()));
		if (area2 < 1e-300)
			return 0.0;
		const double la = QLineF(b, c).length();
		const double lb = QLineF(a, c).length();
		const double lc = QLineF(a, b).length();
		const double denom = la * lb * lc;
		if (denom < 1e-300)
			return 0.0;
		return 2.0 * area2 / denom;
	}


	void mCurveChartView::setAnnotationMode(bool on)
	{
		if (_annotationMode == on)
			return;
		_annotationMode = on;
		if (_plot)
			_plot->setEnableMarker(on);   //引擎 snapper 启停（已置仅吸附模式）
		_lastSnapCtx = CurvePointCtx();
		hideAnnotationHover();
	}

	void mCurveChartView::clearAnnotationMarkers()
	{
		qDeleteAll(_annotationMarkers);
		_annotationMarkers.clear();
	}

	void mCurveChartView::hideAnnotationHover()
	{
		if (_hoverItem && _hoverItem->isVisible())
		{
			_hoverItem->hide();
			if (QwtPlot* qwt = _plot->getQwtPlot())
				qwt->replot();
		}
	}

	QPoint mCurveChartView::dataToCanvas(double x, double y) const
	{
		QwtPlot* qwt = _plot->getQwtPlot();
		if (!qwt)
			return QPoint();
		const QwtScaleMap xm = qwt->canvasMap(QwtPlot::xBottom);
		const QwtScaleMap ym = qwt->canvasMap(QwtPlot::yLeft);
		return QPoint(qRound(xm.transform(x)), qRound(ym.transform(y)));
	}

	//在指定曲线上找离画布位置最近的采样点（不限半径），填充上下文；
	//曲线不存在/不可见/无数据返回 false。
	//engineKey 为引擎键（R23）：引擎查找按键；ctx.curveName 记模型名
	//（{Label} 模板显示与注释 kAttachCurve 模型写入用）
	bool mCurveChartView::nearestCurveContextOn(const QString& engineKey, const QPoint& canvasPos, CurvePointCtx& ctx) const
	{
		ctx = CurvePointCtx();
		QwtPlot* qwt = _plot->getQwtPlot();
		if (!qwt)
			return false;

		//指定曲线（须可见且有数据）上找像素距离最近的采样点（Qwt closestPoint
		//的 dist 即像素距离）；不限半径——曲线类注释的吸附/求值只认这条曲线
		const QwtPlotCurve* c = _plot->getCurve(engineKey);
		if (!c || !c->isVisible() || c->dataSize() == 0)
			return false;
		const int idx = c->closestPoint(QPointF(canvasPos), nullptr);
		if (idx < 0)
			return false;

		ctx.valid = true;
		ctx.curveName = curveModelName(engineKey);
		const QPointF p = c->sample(idx);
		ctx.x = p.x();
		ctx.y = p.y();
		ctx.index = idx;

		//斜率/曲率：邻域三点估计（渲染空间，端点退化为单侧差分）
		const int n = static_cast<int>(c->dataSize());
		const QPointF p0 = c->sample(qMax(0, idx - 1));
		const QPointF p2 = c->sample(qMin(n - 1, idx + 1));
		const double dx = p2.x() - p0.x();
		ctx.slope = (std::fabs(dx) > 1e-300) ? (p2.y() - p0.y()) / dx : 0.0;
		ctx.curv = mengerCurvature(p0, p, p2);
		return true;
	}

	//取指定曲线第 index（0基，越界钳制）个采样点的上下文；曲线无效返回 false。
	//engineKey 为引擎键（R23），ctx.curveName 记模型名（同 nearestCurveContextOn）
	bool mCurveChartView::curvePointContextAt(const QString& engineKey, int index, CurvePointCtx& ctx) const
	{
		ctx = CurvePointCtx();
		QwtPlot* qwt = _plot->getQwtPlot();
		if (!qwt)
			return false;

		//指定曲线（须可见且有数据）第 index（0基）个采样点，越界钳制到有效区间
		const QwtPlotCurve* c = _plot->getCurve(engineKey);
		if (!c || !c->isVisible() || c->dataSize() == 0)
			return false;
		const int idx = qBound(0, index, static_cast<int>(c->dataSize()) - 1);

		ctx.valid = true;
		ctx.curveName = curveModelName(engineKey);
		const QPointF p = c->sample(idx);
		ctx.x = p.x();
		ctx.y = p.y();
		ctx.index = idx;

		//斜率/曲率：邻域三点估计（渲染空间，端点退化为单侧差分）
		const QPointF p0 = c->sample(qMax(0, idx - 1));
		const QPointF p2 = c->sample(qMin(static_cast<int>(c->dataSize()) - 1, idx + 1));
		const double dx = p2.x() - p0.x();
		ctx.slope = (std::fabs(dx) > 1e-300) ? (p2.y() - p0.y()) / dx : 0.0;
		ctx.curv = mengerCurvature(p0, p, p2);
		return true;
	}

	QString mCurveChartView::evaluateAnnotationText(const QString& tpl, double x, double y,
		const CurvePointCtx& ctx, bool fmtOn, int fmtMode, int fmtPrecision) const
	{
		//数值样式：未勾选"标量值格式化"用默认 g/6；勾选后按格式（0科学计数法
		//'e' / 1定点格式'f'）与精度显示（设计图15）
		auto num = [fmtOn, fmtMode, fmtPrecision](double v)
		{
			if (!fmtOn)
				return QString::number(v, 'g', 6);
			return QString::number(v, fmtMode == 0 ? 'e' : 'f', fmtPrecision);
		};
		QString out = tpl;
		out.replace(QStringLiteral("{X}"), num(x));
		out.replace(QStringLiteral("{Y}"), num(y));
		out.replace(QStringLiteral("{Time}"), num(x));
		out.replace(QStringLiteral("{Label}"), ctx.valid ? ctx.curveName : QStringLiteral("-"));
		out.replace(QStringLiteral("{Slope}"), ctx.valid ? num(ctx.slope) : QStringLiteral("-"));
		out.replace(QStringLiteral("{Curv}"), ctx.valid ? num(ctx.curv) : QStringLiteral("-"));
		return out;
	}

	const CurveAnnotationData* mCurveChartView::templateSourceAnnotation() const
	{
		//本图表最后一个有效注释节点（有非占位文本）——悬停预览/新建注释沿用它
		//的模板文本与标量值格式化设置
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const QList<int> children = doc->childNodeIds(_chartId);
		for (int i = children.size() - 1; i >= 0; --i)
		{
			CurveNode* n = doc->node(children.at(i));
			if (n && n->type == CurveNodeType::Annotation
				&& !n->annotation.text.isEmpty()
				&& n->annotation.text != kViewAnnotationPlaceholder)
			{
				return &n->annotation;
			}
		}
		return nullptr;
	}

	QString mCurveChartView::currentAnnotationTemplate() const
	{
		//沿用本图表最近一个注释节点的模板（双击编辑过的模板对后续注释生效）；
		//"视图注释"占位文本不是模板，跳过（否则曲线外注释会污染后续曲线注释）；
		//无可用模板时用对话框默认模板
		const CurveAnnotationData* src = templateSourceAnnotation();
		return src ? src->text : QStringLiteral("Y={Y} @ X={X}");
	}

	void mCurveChartView::updateAnnotationHover(const CurvePointCtx& ctx)
	{
		QwtPlot* qwt = _plot->getQwtPlot();
		if (!qwt)
			return;

		if (!_hoverItem)
		{
			//悬停预览与曲线注释同款画法（引出线+边框文本框），蓝色区分；
			//绘制锚点符号指示吸附位置（生成的注释不带锚点）
			PlotView::AnnotationSettings hoverSettings;
			hoverSettings.borderColor = QColor(43, 125, 233);
			hoverSettings.textColor = QColor(43, 125, 233);
			hoverSettings.bgColor = QColor(255, 255, 255, 230);
			_hoverItem = new PlotView::mPlotAnnotation;
			_hoverItem->setAnnotationSettings(hoverSettings);
			_hoverItem->setAttachMode(PlotView::mPlotAnnotation::OnCurve);
			_hoverItem->setShowAnchorSymbol(true);
			_hoverItem->setZ(21.0);
			_hoverItem->attach(qwt);
		}
		auto* hover = _hoverItem;
		//模板与标量值格式化均沿用本图表最近一个有效注释（预览即新建所得）
		const CurveAnnotationData* src = templateSourceAnnotation();
		const QString tpl = src ? src->text : QStringLiteral("Y={Y} @ X={X}");
		hover->setText(evaluateAnnotationText(tpl, ctx.x, ctx.y, ctx,
			src && src->scalarFmtOn, src ? src->scalarFmtMode : 0,
			src ? src->scalarFmtPrecision : 5));
		hover->setAnchor(ctx.x, ctx.y);
		hover->show();
		qwt->replot();
	}

	void mCurveChartView::onSnapHover(bool active, const QString& curveName,
		int sampleIndex, const QPointF& valuePos)
	{
		Q_UNUSED(valuePos);
		//curveName 为引擎 snapper 广播的引擎键（R23）：上下文回查按引擎键，
		//ctx.curveName 由上下文函数回填为模型名（显示/模型写入用）
		//基准线模式：吸附结果同样缓存（创建时优先取吸附点），预览为 X/Y 读数
		//（设计图14）；与注释模式互斥，不会同时走到
		if (_baselineMode)
		{
			if (!active || sampleIndex < 0
				|| !curvePointContextAt(curveName, sampleIndex, _lastSnapCtx))
			{
				_lastSnapCtx = CurvePointCtx();
				hideBaselineReadout();
			}
			else
				updateBaselineReadout(_lastSnapCtx);
			return;
		}
		//引擎 snapper（仅吸附模式）广播：按采样点序号回查上下文——锚点/附着
		//序号/斜率/曲率语义仍是采样点（与原 closestPoint 吸附一致）；
		//结果缓存供左键创建取用，同时驱动蓝色悬停预览
		if (!active || sampleIndex < 0
			|| !curvePointContextAt(curveName, sampleIndex, _lastSnapCtx))
		{
			_lastSnapCtx = CurvePointCtx();
			hideAnnotationHover();
			return;
		}
		updateAnnotationHover(_lastSnapCtx);
	}

	void mCurveChartView::addAnnotationAt(const QPoint& canvasPos)
	{
		QwtPlot* qwt = _plot->getQwtPlot();
		if (!qwt)
			return;

		//锚点：用引擎 snapper 最近一次吸附结果（onSnapHover 缓存，与悬停预览
		//同源同半径；注释类别=曲线类，框位置与锚点解耦），未吸附为纯视图注释
		const bool onCurve = _lastSnapCtx.valid;
		const CurvePointCtx ctx = _lastSnapCtx;

		//模板与标量值格式化沿用本图表最近一个有效注释（与悬停预览一致）
		const CurveAnnotationData* src = templateSourceAnnotation();
		const QString tpl = src ? src->text : QStringLiteral("Y={Y} @ X={X}");
		const bool fmtOn = src ? src->scalarFmtOn : false;
		const int fmtMode = src ? src->scalarFmtMode : 0;
		const int fmtPrec = src ? src->scalarFmtPrecision : 5;
		//求值结果只用于初摆框宽高估算；模型 kText 存模板（渲染层按锚点求值，
		//改标量值格式化/精度即可重渲染，设计文档"模板文本求值显示"）
		const QString evalText = onCurve
			? evaluateAnnotationText(tpl, ctx.x, ctx.y, ctx, fmtOn, fmtMode, fmtPrec)
			: kViewAnnotationPlaceholder;

		//注释框中心（画布像素 -> 数据坐标，posX/posY 语义）：曲线类挂锚点右上
		//（引出线端），视图类挂光标右上；框宽高与默认偏移用库图元同款默认值
		//（AnnotationSettings 默认内边距/字体/偏移，与渲染首次摆位一致）
		const QwtScaleMap xm = qwt->canvasMap(QwtPlot::xBottom);
		const QwtScaleMap ym = qwt->canvasMap(QwtPlot::yLeft);
		PlotView::AnnotationSettings annoDefaults;
		PlotView::mPlotAnnotation probe;
		probe.setText(evalText);
		const double bw = probe.estimatedTextSize().width();
		const double bh = probe.estimatedTextSize().height();
		const QPointF boxRef = onCurve
			? QPointF(dataToCanvas(ctx.x, ctx.y)) + QPointF(annoDefaults.curveBoxOffX + bw / 2.0,
				annoDefaults.curveBoxOffY - bh / 2.0)
			: QPointF(canvasPos) + QPointF(annoDefaults.viewBoxOffX + bw / 2.0,
				annoDefaults.viewBoxOffY - bh / 2.0);

		//新建 + 类别/文本/位置/格式化写入包成一个撤销宏（一次撤销整组）；
		//信号经 scheduleAnnotationRefresh 增量渲染（不走全量 rebuild，保留缩放）。
		//注释按创建位置分类：曲线上=曲线类（记附着曲线/附着点，kText 存模板、
		//渲染层按锚点求值显示）；曲线外=视图类（先只置"视图注释"占位，双击
		//注释边框弹出对话框才改为用户选择的内容）
		mCurveDocController* controller = mCurveDocController::getInstance();
		QUndoStack* stack = controller->undoStack();
		stack->beginMacro(QStringLiteral("添加注释"));
		const int id = controller->addAnnotation(_chartId);
		if (id > 0)
		{
			controller->setNodeProperty(id, CurveProp::kAttachMode, onCurve ? 0 : 1);
			if (onCurve)
			{
				//kAttachCurve 存模型名（R23）：ctx.curveName 已由上下文函数
				//从引擎键反查为模型名，附着渲染时再经 engineKeyForCurveName 回查键
				controller->setNodeProperty(id, CurveProp::kAttachCurve, ctx.curveName);
				controller->setNodeProperty(id, CurveProp::kAttachIndex, ctx.index + 1);
			}
			controller->setNodeProperty(id, CurveProp::kText, onCurve ? tpl : evalText);
			controller->setNodeProperty(id, CurveProp::kPosX, xm.invTransform(boxRef.x()));
			controller->setNodeProperty(id, CurveProp::kPosY, ym.invTransform(boxRef.y()));
			controller->setNodeProperty(id, CurveProp::kScalarFmtOn, fmtOn);
			controller->setNodeProperty(id, CurveProp::kScalarFmtMode, fmtMode);
			controller->setNodeProperty(id, CurveProp::kScalarFmtPrecision, fmtPrec);
		}
		stack->endMacro();
	}

	//--- 基准线交互（Ribbon"基准线"下拉经视口工厂广播到此，设计图14） ---

	void mCurveChartView::setBaselineMode(bool on, int orientation)
	{
		if (_baselineMode == on && (!on || _baselineOrientation == orientation))
			return;
		_baselineMode = on;
		if (on)
			_baselineOrientation = orientation;
		if (_plot)
			_plot->setEnableMarker(on);   //引擎 snapper 启停（吸附驱动读数与创建取值）
		_lastSnapCtx = CurvePointCtx();
		hideBaselineReadout();
	}

	void mCurveChartView::addBaselineAt(const QPoint& canvasPos)
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!qwt)
			return;

		//位置值：悬停曲线时优先取 snapper 吸附点（与读数同源同半径，参考软件
		//语义），否则取点击处的轴坐标（画布像素 -> 数据坐标；纵向取X值，横向取Y值）
		const bool horizontal = (_baselineOrientation == 0);
		double value = 0.0;
		if (_lastSnapCtx.valid)
			value = horizontal ? _lastSnapCtx.y : _lastSnapCtx.x;
		else if (horizontal)
			value = qwt->canvasMap(QwtPlot::yLeft).invTransform(canvasPos.y());
		else
			value = qwt->canvasMap(QwtPlot::xBottom).invTransform(canvasPos.x());

		//新建 + 方向/位置值写入包成一个撤销宏（一次撤销整组）；节点加入与属性
		//变化经 scheduleBaselineRefresh 增量渲染（不走全量 rebuild，保留缩放）
		mCurveDocController* controller = mCurveDocController::getInstance();
		QUndoStack* stack = controller->undoStack();
		stack->beginMacro(QStringLiteral("添加基准线"));
		const int id = controller->addBaseline(_chartId);
		if (id > 0)
		{
			controller->setNodeProperty(id, CurveProp::kOrientation, _baselineOrientation);
			controller->setNodeProperty(id, CurveProp::kValue, value);
		}
		stack->endMacro();
	}

	int mCurveChartView::hitBaseline(const QPoint& canvasPos) const
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!qwt)
			return -1;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const QwtScaleMap xm = qwt->canvasMap(QwtPlot::xBottom);
		const QwtScaleMap ym = qwt->canvasMap(QwtPlot::yLeft);
		//后画的基准线在上层，逆序优先；纵向线比 x 距离、横向线比 y 距离（像素）；
		//隐藏线不参与命中
		const auto children = doc->childNodeIds(_chartId);
		for (int i = children.size() - 1; i >= 0; --i)
		{
			const int childId = children.at(i);
			CurveNode* n = doc->node(childId);
			if (!n || n->type != CurveNodeType::Baseline || !n->baseline.visible)
				continue;
			const bool horizontal = (n->baseline.orientation == 0);
			const double linePx = horizontal ? ym.transform(n->baseline.value)
				: xm.transform(n->baseline.value);
			const double dist = horizontal ? qAbs(canvasPos.y() - linePx)
				: qAbs(canvasPos.x() - linePx);
			if (dist <= kBaselineHitPx)
				return childId;
		}
		return -1;
	}

	void mCurveChartView::applyBaseline(int baselineNodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(baselineNodeId);
		if (!n || n->type != CurveNodeType::Baseline)
			return;
		const QString markerName = QStringLiteral("baseline_%1").arg(baselineNodeId);
		//显隐：隐藏即摘除渲染项（重新显示时由下方逻辑重建），标签图元一并摘除
		if (!n->baseline.visible)
		{
			if (_plot->getMarkerLine(markerName))
				_plot->removeMarkerLine(markerName);
			removeBaselineLabel(baselineNodeId);
			return;
		}
		const PlotView::mPlotMarkerLine::Orientation orient = (n->baseline.orientation == 0)
			? PlotView::mPlotMarkerLine::Horizontal
			: PlotView::mPlotMarkerLine::Vertical;
		//引擎 addMarkerLine 对重名会告警：仅缺失时创建，已存在走 setter 增量改
		PlotView::mPlotMarkerLine* line = _plot->getMarkerLine(markerName);
		if (!line)
			line = _plot->addMarkerLine(markerName, orient, n->baseline.value);
		if (!line)
			return;
		line->setOrientation(orient);
		line->setValue(n->baseline.value);
		line->setLineColor(n->baseline.color);
		line->setLineWidth(n->baseline.lineWidth);
		line->setLineStyle(static_cast<Qt::PenStyle>(n->baseline.lineStyle));

		//线上标签（设计图09/14）：文本=标签（空不显示；创建时初值=基准线名称），
		//字体/9宫格位置随属性页"显示"组（kLabelFont/kLabelPos），颜色随线色；
		//与线同值同映射保证始终贴线（线值越出画布时图元不绘制）
		if (QwtPlot* qwt = _plot->getQwtPlot())
		{
			mCurveBaselineLabel* label = _baselineLabels.value(baselineNodeId, nullptr);
			if (!label)
			{
				label = new mCurveBaselineLabel;
				label->attach(qwt);
				_baselineLabels.insert(baselineNodeId, label);
			}
			label->setLineHorizontal(n->baseline.orientation == 0);
			label->setLineValue(n->baseline.value);
			label->setLabelText(n->baseline.label);
			label->setLabelFont(n->baseline.labelFont);
			label->setLabelColor(n->baseline.color);
			label->setLabelPos(n->baseline.labelPos);
		}
	}

	void mCurveChartView::removeBaselineLabel(int baselineNodeId)
	{
		//QwtPlot 不拥有 attach 的 item，delete 即摘除（析构自动 detach）
		delete _baselineLabels.take(baselineNodeId);
	}

	void mCurveChartView::clearBaselineLabels()
	{
		qDeleteAll(_baselineLabels);
		_baselineLabels.clear();
	}

	void mCurveChartView::scheduleBaselineRefresh(int baselineNodeId)
	{
		_pendingBaselineIds.insert(baselineNodeId);
		if (_baselineRefreshPending)
			return;
		_baselineRefreshPending = true;
		//0ms 合并：一次撤销宏内 nodeAdded + 两条 nodePropertyChanged 只重画一遍
		QTimer::singleShot(0, this, [this]()
		{
			flushBaselineRefresh();
		});
	}

	void mCurveChartView::flushBaselineRefresh()
	{
		_baselineRefreshPending = false;
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!qwt)
		{
			_pendingBaselineIds.clear();
			return;
		}
	const QSet<int> pending = _pendingBaselineIds;
	_pendingBaselineIds.clear();
	for (int id : pending)
		applyBaseline(id);
	//applyBaseline 把画笔写回模型值，hover 高亮需重放（非 hover 线无视觉变化）
	applyBaselineHoverVisuals();
	qwt->replot();
	}

	//轴刻度细项轻量刷新：只重推受影响轴的刻度引擎/网格/数字格式（与注释/
	//基准线增量刷新同策略——全量重建经 fitChartAxes 重算自动范围会重置缩放）
	void mCurveChartView::scheduleAxisRefresh(int axisNodeId)
	{
		_pendingAxisIds.insert(axisNodeId);
		if (_axisRefreshPending)
			return;
		_axisRefreshPending = true;
		//0ms 合并：连续拖动步长/网格数 spin 只刷一遍
		QTimer::singleShot(0, this, [this]()
		{
			flushAxisRefresh();
		});
	}

	void mCurveChartView::flushAxisRefresh()
	{
		_axisRefreshPending = false;
		const QSet<int> pending = _pendingAxisIds;
		_pendingAxisIds.clear();
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		//极坐标画布接管（chartType=3）：直角引擎轴不可见，刻度细项改动（X1 辐条
		//数/Y1 主环数/网格数/格式等）不落 applyAxis，统一重算底图+向量层下发——
		//否则改动入模型后无渲染通道消费，目测"不生效"（2026-11-12 用户实锤）。
		//不经全量重建（fitChartAxes 会重算自动域，非本轮语义）；滚轮缩放视图态
		//（_polarRadiusZoom）不动模型，重算自然保持
		if (isPolarChart())
		{
			applyPolarDecor();
			if (QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr)
				qwt->replot();
			return;
		}
		for (int id : pending)
		{
			CurveNode* n = doc->node(id);
			if (!n)
				continue;
			switch (n->type)
			{
			case CurveNodeType::XAxis:
				applyAxis(id, AxisId::X_Bottom);
				break;
			case CurveNodeType::YAxis:
				applyAxis(id, AxisId::Y_Left);
				break;
			case CurveNodeType::XAxis2:
			case CurveNodeType::YAxis2:
				//副轴统一走 applySecondaryAxes：首枚映射引擎上/右轴，其余重建
				//自绘条带（自动范围镜像同侧引擎轴当前范围，缩放状态不丢）
				applySecondaryAxes();
				break;
			default:
				break;
			}
		}
	}

	//曲线外观增量刷新（线色/线宽/线型/区域填充/符号属性）：按节点直发引擎
	//（applySeries 重推数据+样式但不触碰轴范围，用户缩放/平移保留）；
	//一次宏内多条外观属性变化 0ms 合并重画
	void mCurveChartView::scheduleSeriesStyleRefresh(int curveNodeId)
	{
		_pendingStyleIds.insert(curveNodeId);
		if (_styleRefreshPending)
			return;
		_styleRefreshPending = true;
		QTimer::singleShot(0, this, [this]()
		{
			flushSeriesStyleRefresh();
		});
	}

	void mCurveChartView::flushSeriesStyleRefresh()
	{
		_styleRefreshPending = false;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const QSet<int> pending = _pendingStyleIds;
		_pendingStyleIds.clear();
		for (int id : pending)
		{
			CurveNode* n = doc->node(id);
			if (!n)
				continue;
			CurveNode* chart = doc->node(doc->chartNodeId(id));
			const bool complexCurve = chart && chart->chart.chartType == 2 && n->series.isComplex();
			if (n->series.visible)
			{
				if (complexCurve)
					refreshComplexCurve(id);   //复数曲线：数据+样式一并增量下发（曲线缺失时自动重建）
				else
					applySeries(id);   //可见：重推数据+样式
			}
			else if (complexCurve)
			{
				scheduleRebuild();   //复数曲线隐藏：全量重建（重建清空后不再重画，实现隐藏）
			}
			else
			{
				//隐藏：从引擎摘除（图例同步消失，与重建语义一致）；按键（R23）
				const QString engineKey = keyFor(id);
				if (_plot->getCurve(engineKey))
					_plot->removeCurve(engineKey);
			}
		}
		if (QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr)
			qwt->replot();   //图例线样图标/读数同步（setCurveStyle 内部已 replot，此处兜底）
		//复数下面板兜底重绘（setComplexCurveStyle 内部已 replot 下面板，此处兜底）
		if (_plot->isComplexMode())
		{
			if (QwtPlot* lowerQwt = _plot->getLowerQwtPlot())
				lowerQwt->replot();
		}
	}

	//复数曲线增量刷新（不经全量重建）：数据+样式一并下发——
	//updateComplexCurveData 在引擎缺失该曲线时会自动重建上/下面板（重新勾选
	//"可见性"等场景），setComplexCurveStyle 上/下面板样式一并下发
	//（线型/颜色/线宽/符号样式/密度/图例文本，两面板同色同型）
	void mCurveChartView::refreshComplexCurve(int curveNodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(curveNodeId);
		if (!n || n->type != CurveNodeType::Curve || !n->series.isComplex())
			return;
		CurveNode* chartNode = doc->node(_chartId);
		const int displayFmt = chartNode ? chartNode->chart.complexDisplayFormat : 0;
		const bool swapped = chartNode && chartNode->chart.complexSwapped;
		QVector<double> xs, aRaw, bRaw;
		if (!transformedComplexPair(n, xs, aRaw, bRaw))
			return;
		//显示格式换算（节点原始形态 -> 图表显示格式），再按排布决定上下面板：
		//基础排布对齐 HyperGraph 2024 复数图——相位/虚部侧（B 路）在上；互换态对调
		QVector<double> dispA, dispB;
		convertComplexDisplay(aRaw, bRaw, n->series.complexFormat, displayFmt, dispA, dispB);
		if (!swapped)
			std::swap(dispA, dispB);
		const ComplexFormat chartFmt = (displayFmt == 0)
			? ComplexFormat::MagPhase : ComplexFormat::RealImag;
		//复数单节点按键下发（R23），同 applyComplexSeries
		const QString engineKey = keyFor(curveNodeId);
		if (dispB.isEmpty() || dispA.isEmpty())
		{
			//相位侧为空（缺 b 侧）：上面板单曲线降级
			_plot->updateCurveData(engineKey, xs, dispA);
			_plot->setCurveStyle(engineKey, makeCurveSettings(n->series, engineKey));
			return;
		}
		_plot->updateComplexCurveData(engineKey, xs, dispA, dispB, chartFmt);
		_plot->setComplexCurveStyle(engineKey, makeCurveSettings(n->series, engineKey));
	}

	void mCurveChartView::updateBaselineReadout(const CurvePointCtx& ctx)
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!qwt || !_baselineReadout || !ctx.valid)
			return;
		//单行"y= …  @ x= …"（y 前置，同参考软件探针读数）；值文本走轴刻度格式（mCurveScaleDraw 'g'4 位有效数字），与轴显示一致
		const QString text = QStringLiteral("y= %1  @ x= %2")
			.arg(qwt->axisScaleDraw(QwtPlot::yLeft)->label(ctx.y).text(),
				qwt->axisScaleDraw(QwtPlot::xBottom)->label(ctx.x).text());
		_baselineReadout->showAt(dataToCanvas(ctx.x, ctx.y), text);
		qwt->replot();
	}

	void mCurveChartView::hideBaselineReadout()
	{
		if (_baselineReadout && _baselineReadout->isVisible())
		{
			_baselineReadout->hideReadout();
			if (QwtPlot* qwt = _plot->getQwtPlot())
				qwt->replot();
		}
	}

	int mCurveChartView::hitAnnotation(const QPoint& canvasPos) const
	{
		//逆序（后画的注释在上层，优先命中）；命中区域=注释边框内部
		//（渲染项 draw 时记录的最新文本框几何，与画面完全一致）
		for (auto it = _annotationMarkers.constEnd(); it != _annotationMarkers.constBegin();)
		{
			--it;
			const auto* item = static_cast<const PlotView::mPlotAnnotation*>(it.value());
			if (item->lastBox().contains(canvasPos))
				return it.key();
		}
		return -1;
	}

	void mCurveChartView::showAnnotationEditor(int annotationNodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(annotationNodeId);
		if (!n || n->type != CurveNodeType::Annotation)
			return;

		//对话框回填：占位文本"视图注释"回填默认模板，其余回填当前模板文本；
		//保存则经命令栈写回 kText（增量刷新）
		QString tpl = n->annotation.text;
		if (tpl == kViewAnnotationPlaceholder)
			tpl = QStringLiteral("Y={Y} @ X={X}");
		PostCommon::mPostAnnotationDialog dlg(this);
		dlg.setAnnotationText(tpl);
		if (dlg.exec() != QDialog::Accepted)
			return;
		mCurveDocController::getInstance()->setNodeProperty(
			annotationNodeId, CurveProp::kText, dlg.annotationText());
	}

	//--- 选中注释左键拖动（选中后拖动）：按下进入、移动实时跟手、松手提交 ---

	void mCurveChartView::startAnnotationDrag(int annotationNodeId, const QPoint& canvasPos)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(annotationNodeId);
		auto it = _annotationMarkers.find(annotationNodeId);
		//最小化（曲线类方块锚在附着点上）不拖；自动定位仅曲线类生效（框位由
		//策略绕锚点派生，拖框看不到位移），视图类自动定位本就不生效可拖
		if (!n || n->type != CurveNodeType::Annotation || n->annotation.minimized
			|| (n->annotation.attachMode == 0 && n->annotation.autoPos)
			|| it == _annotationMarkers.end() || !_plot)
			return;
		//抓取偏移：按下点->框中心（画布像素），拖动中保持，框不跳变
		_dragGrabPx = QPointF(dataToCanvas(n->annotation.posX, n->annotation.posY))
			- QPointF(canvasPos);
		_dragPosX = n->annotation.posX;
		_dragPosY = n->annotation.posY;
		_dragAnnotationId = annotationNodeId;
	}

	void mCurveChartView::moveAnnotationDrag(const QPoint& canvasPos)
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		auto it = _annotationMarkers.find(_dragAnnotationId);
		if (_dragAnnotationId < 0 || !qwt || it == _annotationMarkers.end())
		{
			_dragAnnotationId = -1;   //渲染项没了（撤销/删除/重建竞态）：终止拖动
			return;
		}
		//框中心=光标+抓取偏移（画布像素->数据坐标），只改渲染项不进模型
		const QPointF centerPx = QPointF(canvasPos) + _dragGrabPx;
		const double bx = qwt->canvasMap(QwtPlot::xBottom).invTransform(centerPx.x());
		const double by = qwt->canvasMap(QwtPlot::yLeft).invTransform(centerPx.y());
		static_cast<PlotView::mPlotAnnotation*>(it.value())->setBoxPos(bx, by);
		_dragPosX = bx;
		_dragPosY = by;
		qwt->replot();
	}

	void mCurveChartView::finishAnnotationDrag()
	{
		const int id = _dragAnnotationId;
		_dragAnnotationId = -1;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(id);
		if (!n || n->type != CurveNodeType::Annotation)
			return;
		//框位没动（单击选中）不提交，不产生空撤销项；动了则一个宏撤销整次拖动。
		//kPosX/kPosY 不触发吸附（吸附只看 attachMode/attachCurve/attachIndex），
		//提交后的增量刷新按同位重建渲染项，画面无跳变
		if (n->annotation.posX == _dragPosX && n->annotation.posY == _dragPosY)
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		QUndoStack* stack = controller->undoStack();
		stack->beginMacro(QStringLiteral("移动注释"));
		controller->setNodeProperty(id, CurveProp::kPosX, _dragPosX);
		controller->setNodeProperty(id, CurveProp::kPosY, _dragPosY);
		stack->endMacro();
	}

	void mCurveChartView::applyAnnotation(int annotationNodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(annotationNodeId);
		if (!n || n->type != CurveNodeType::Annotation)
			return;
		//不可见注释不建渲染项（树勾选/属性页开关；增量刷新摘除旧项即隐藏）
		if (!n->annotation.visible)
			return;

		QwtPlot* qwt = _plot->getQwtPlot();
		if (!qwt)
			return;

		const CurveAnnotationData& a = n->annotation;

		//类别（创建位置决定，设计图15）：曲线类=带引出线，视图类=纯文本框
		//（即使靠近曲线样式也不变）；附着曲线缺失/不可见时退化为视图注释画法。
		//锚点解析（曲线类）：附着点序号优先（注释始终连在该点上，框调整位置只
		//改引出线）；未指定序号则取附着曲线上离注释框最近的采样点
		CurvePointCtx ctx;
		bool onCurve = false;
		QString attachKey;   //附着曲线的引擎键（R23；非曲线类/解析失败为空）
		if (a.attachMode == 0)
		{
			//附着曲线按模型名存储（kAttachCurve），引擎查找须经键（R23）：
			//未指定名时取本图表第一条曲线（键经映射取，重名模型取首个）
			QString attachName = a.attachCurve;
			if (attachName.isEmpty())
			{
				for (int childId : doc->childNodeIds(_chartId))
				{
					CurveNode* c = doc->node(childId);
					if (c && c->type == CurveNodeType::Curve)
					{
						attachKey = keyFor(childId);
						attachName = c->series.name;
						break;
					}
				}
			}
			else
				attachKey = engineKeyForCurveName(attachName);
			if (a.attachIndex >= 1)
				onCurve = curvePointContextAt(attachKey, a.attachIndex - 1, ctx);
			if (!onCurve)
				onCurve = nearestCurveContextOn(attachKey, dataToCanvas(a.posX, a.posY), ctx);
		}

		//所附曲线色（曲线注释"跟随曲线颜色"的边框/引出线取色）：经键反查
		//确切节点（重名曲线各自取色，与命中/选中的节点一致）
		QColor curveColor;
		if (onCurve)
		{
			const int attachNodeId = _nodeByEngineKey.value(attachKey, -1);
			if (attachNodeId > 0)
			{
				if (CurveNode* c = doc->node(attachNodeId))
					curveColor = c->series.color;
			}
		}

		//配色（属性页"显示"组，设计图15）：边框自定义生效时用自定义色，否则
		//曲线注释跟随曲线色；视图注释传模型原值。文本/背景也传模型原值，
		//无效色由库图元兜底（边框深灰/文本随边框/背景半透明白，与旧自绘一致）
		QColor borderColor;
		if (onCurve)
			borderColor = (a.borderMode == 1 && a.borderColor.isValid())
				? a.borderColor : curveColor;
		else
			borderColor = a.borderColor;
		PlotView::AnnotationSettings settings;
		//边框可见性：borderMode 2=无框裸文字（需求 05 In-Phase 相位标注，0/1 既有框形态不动）
		settings.frameVisible = (a.borderMode != 2);
		settings.borderColor = borderColor;
		settings.textColor = a.color;
		settings.bgColor = a.bgColor;
		settings.borderWidth = a.borderWidth;
		auto* item = new PlotView::mPlotAnnotation;
		item->setAnnotationSettings(settings);
		//类别映射：模型 attachMode 0=曲线类/1=视图类 -> 库 OnCurve/Free
		item->setAttachMode(onCurve ? PlotView::mPlotAnnotation::OnCurve
			: PlotView::mPlotAnnotation::Free);
		//文本按模板求值（数值按标量值格式化设置显示）：曲线类取锚点（附着点）
		//坐标，视图类取框中心坐标
		item->setText(evaluateAnnotationText(a.text, ctx.valid ? ctx.x : a.posX,
			ctx.valid ? ctx.y : a.posY, ctx,
			a.scalarFmtOn, a.scalarFmtMode, a.scalarFmtPrecision));
		if (onCurve)
			item->setAnchor(ctx.x, ctx.y);   //锚点=附着曲线上的点（不随框位置移动）
		item->setBoxPos(a.posX, a.posY);     //注释框中心（数据坐标）
		item->setSelected(annotationNodeId == _selectedAnnotationId);   //选中态重放
		//最小化：锚点处实心小方块（边长=最小化半径），悬停显示内容——曲线类
		//方块保留旁现内容，视图类方块隐藏仅现内容
		bool mini = a.minimized;
		bool content = false;
		if (a.minimized && annotationNodeId == _hoveredAnnotationId)
		{
			if (a.attachMode == 0)
				content = true;
			else
				mini = false;
		}
		item->setMinimizeAppearance(a.minimizeRadius, a.minimizeColor);
		item->setMinimized(mini);
		item->setContentVisible(content);
		//自动定位：仅曲线类且有锚点时生效（文本框按策略绕锚点排布，偏移=
		//注释到点的距离）；视图类不生效，框位即模型位置（附着曲线缺失退化
		//为视图画法时同样不生效）
		item->setAutoPosition(onCurve && a.autoPos, a.autoAnchor, a.autoOffset);
		item->attach(qwt);
		_annotationMarkers.insert(annotationNodeId, item);
	}

	void mCurveChartView::updateAnnotationMinimizeHover(const QPoint& canvasPos)
	{
		//最小化小方块悬停：命中显示其中内容——曲线类方块保留旁现内容，
		//视图类方块隐藏仅现内容；移出方块/离画布复原
		//（画布外坐标必不命中，Leave 事件传 (-1,-1) 即为清除）
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		int hit = -1;
		for (auto it = _annotationMarkers.constBegin(); it != _annotationMarkers.constEnd(); ++it)
		{
			CurveNode* n = doc->node(it.key());
			if (!n || n->type != CurveNodeType::Annotation || !n->annotation.minimized)
				continue;
			if (static_cast<const PlotView::mPlotAnnotation*>(it.value())->lastBox().contains(canvasPos))
			{
				hit = it.key();
				break;
			}
		}
		if (hit == _hoveredAnnotationId)
			return;
		_hoveredAnnotationId = hit;

		//只翻转显隐状态变化的渲染项并重绘（不做增量重建）
		bool needReplot = false;
		for (auto it = _annotationMarkers.begin(); it != _annotationMarkers.end(); ++it)
		{
			CurveNode* n = doc->node(it.key());
			if (!n || n->type != CurveNodeType::Annotation)
				continue;
			bool mini = n->annotation.minimized;
			bool content = false;
			if (n->annotation.minimized && it.key() == _hoveredAnnotationId)
			{
				if (n->annotation.attachMode == 0)
					content = true;               //曲线类：方块保留旁现内容
				else
					mini = false;                 //视图类：方块隐藏仅现内容
			}
			auto* item = static_cast<PlotView::mPlotAnnotation*>(it.value());
			if (item->isMinimized() != mini || item->isContentVisible() != content)
			{
				item->setMinimizeAppearance(n->annotation.minimizeRadius,
					n->annotation.minimizeColor);
				item->setMinimized(mini);
				item->setContentVisible(content);
				needReplot = true;
			}
		}
		if (needReplot)
		{
			if (QwtPlot* qwt = _plot->getQwtPlot())
				qwt->replot();
		}
	}

	void mCurveChartView::snapAnnotationToCurve(int annotationNodeId, bool byPosition)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(annotationNodeId);
		if (!n || n->type != CurveNodeType::Annotation || n->annotation.attachMode != 0)
			return;
		const CurveAnnotationData& a = n->annotation;

		//附着曲线：模型名存储（kAttachCurve），引擎查找经键（R23）。未指定时
		//取本图表第一条曲线（键经映射取，重名模型取首个）；回写模型仍用模型名
		QString attachName = a.attachCurve;
		QString engineKey;
		if (attachName.isEmpty())
		{
			for (int childId : doc->childNodeIds(_chartId))
			{
				CurveNode* c = doc->node(childId);
				if (c && c->type == CurveNodeType::Curve)
				{
					engineKey = keyFor(childId);
					attachName = c->series.name;
					break;
				}
			}
		}
		else
			engineKey = engineKeyForCurveName(attachName);

		//目标点：byPosition（视图切曲线）直接按框当前位置找最近采样点（不限
		//吸附半径），模型里的旧序号是上次曲线附着残留，按它回跳不符合预期；
		//其余情况附着点序号优先（直接附着到该点），未指定序号则找离注释框
		//最近的采样点，找不到保持原样
		CurvePointCtx ctx;
		bool found = false;
		if (!byPosition && a.attachIndex >= 1)
			found = curvePointContextAt(engineKey, a.attachIndex - 1, ctx);
		if (!found)
			found = nearestCurveContextOn(engineKey, dataToCanvas(a.posX, a.posY), ctx);
		if (!found)
			return;

		//附着曲线/点已一致且无需回填时不重复提交（吸附提交会再触发本入口，防止回环）
		const bool needAttach = a.attachCurve != attachName;
		const int wantIndex = ctx.index + 1;   //序号越界时回写钳制值（属性页同步）
		const bool needIndex = a.attachIndex != wantIndex;
		const bool needText = a.text == kViewAnnotationPlaceholder;

		//框位置对齐"曲线上创建"样式：锚点右上 curveBoxOff 短引出线摆框（与
		//创建时的 boxRef 摆位同款），框宽高按回填后文本估算，画布像素->
		//数据坐标回写；切换附着时注释整体挪到新锚点旁，而非原地只延长引出线
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!qwt)
			return;
		const QString tpl = needText ? currentAnnotationTemplate() : a.text;
		const QString evalText = evaluateAnnotationText(tpl, ctx.x, ctx.y, ctx,
			a.scalarFmtOn, a.scalarFmtMode, a.scalarFmtPrecision);
		PlotView::AnnotationSettings annoDefaults;
		PlotView::mPlotAnnotation probe;
		probe.setText(evalText);
		const double bw = probe.estimatedTextSize().width();
		const double bh = probe.estimatedTextSize().height();
		const QPointF boxRef = QPointF(dataToCanvas(ctx.x, ctx.y))
			+ QPointF(annoDefaults.curveBoxOffX + bw / 2.0,
				annoDefaults.curveBoxOffY - bh / 2.0);
		const double wantPosX = qwt->canvasMap(QwtPlot::xBottom).invTransform(boxRef.x());
		const double wantPosY = qwt->canvasMap(QwtPlot::yLeft).invTransform(boxRef.y());
		const bool needPos = a.posX != wantPosX || a.posY != wantPosY;
		if (!needAttach && !needIndex && !needText && !needPos)
			return;

		//附着/序号/文本/框位置回填包成一个撤销宏（对齐"视图切曲线=样式对齐
		//曲线上创建"）；之后用户仍可自由拖动框（拖动提交 kPosX/kPosY 不触发
		//吸附，引出线由渲染层连接框与锚点）
		_snappingAnnotation = true;
		mCurveDocController* controller = mCurveDocController::getInstance();
		QUndoStack* stack = controller->undoStack();
		stack->beginMacro(QStringLiteral("注释附着曲线"));
		if (needAttach)
			controller->setNodeProperty(annotationNodeId, CurveProp::kAttachCurve, attachName);
		if (needIndex)
			controller->setNodeProperty(annotationNodeId, CurveProp::kAttachIndex, wantIndex);
		if (needText)
			controller->setNodeProperty(annotationNodeId, CurveProp::kText, tpl);
		if (needPos)
		{
			controller->setNodeProperty(annotationNodeId, CurveProp::kPosX, wantPosX);
			controller->setNodeProperty(annotationNodeId, CurveProp::kPosY, wantPosY);
		}
		stack->endMacro();
		_snappingAnnotation = false;
	}

	void mCurveChartView::applyAnnotationSelection()
	{
		//选中是视图状态：只把标志写到现有渲染项并重绘，不重建项
		for (auto it = _annotationMarkers.begin(); it != _annotationMarkers.end(); ++it)
			static_cast<PlotView::mPlotAnnotation*>(it.value())->setSelected(it.key() == _selectedAnnotationId);
		if (QwtPlot* qwt = _plot->getQwtPlot())
			qwt->replot();
	}

	void mCurveChartView::scheduleAnnotationRefresh(int annotationNodeId)
	{
		_pendingAnnotationIds.insert(annotationNodeId);
		if (_annotationRefreshPending)
			return;
		_annotationRefreshPending = true;
		//0ms 合并：一次撤销宏内 nodeAdded + 三条 nodePropertyChanged 只重画一遍
		QTimer::singleShot(0, this, [this]()
		{
			flushAnnotationRefresh();
		});
	}

	void mCurveChartView::flushAnnotationRefresh()
	{
		_annotationRefreshPending = false;
		QwtPlot* qwt = _plot->getQwtPlot();
		if (!qwt)
		{
			_pendingAnnotationIds.clear();
			return;
		}

		const QSet<int> pending = _pendingAnnotationIds;
		_pendingAnnotationIds.clear();
		for (int id : pending)
		{
			//先摘除旧渲染项再按模型现状重建（节点可能已被删除：applyAnnotation 校验）
			if (QwtPlotItem* old = _annotationMarkers.take(id))
				delete old;
			applyAnnotation(id);
		}
		qwt->replot();
	}

	void mCurveChartView::onNodeAdded(int nodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (doc->chartNodeId(nodeId) != _chartId && nodeId != _chartId)
			return;
		//注释/基准线增量渲染（不走全量 rebuild，保留用户缩放状态）；其余节点照旧全量重建
		if (doc->nodeType(nodeId) == CurveNodeType::Annotation)
		{
			scheduleAnnotationRefresh(nodeId);
			return;
		}
		if (doc->nodeType(nodeId) == CurveNodeType::Baseline)
		{
			scheduleBaselineRefresh(nodeId);
			return;
		}
		//增删曲线/标题等子节点：重建但保留用户缩放（首条曲线入图时重建内
		//检测到"重建前无可见曲线"仍会按数据适配）
		scheduleRebuild(true);
	}

void mCurveChartView::onNodeRemoved(int nodeId)
{
	//被删节点若是选中/hover 目标，先清状态（后续 rebuild 的重放不会再引用它）
	if (nodeId == _selectedCurveId)
		_selectedCurveId = -1;
	if (nodeId == _hoveredCurveId)
		_hoveredCurveId = -1;
	if (nodeId == _hoveredBaselineId)
		_hoveredBaselineId = -1;
	if (nodeId == _selectedAnnotationId)
		_selectedAnnotationId = -1;   //渲染项随后删除，无需再重放选中框
	if (nodeId == _dragAnnotationId)
		_dragAnnotationId = -1;       //拖动目标被删：终止拖动（后续 Release 空返回）
	if (nodeId == _selectedAxisId)
		_selectedAxisId = -1;         //轴为图表固定子节点（随图表删），此处仅保险清引用
	if (nodeId == _selectedLegendId)
		_selectedLegendId = -1;       //图例同为图表固定子节点，仅保险清引用
	if (nodeId == _selectedBandId)
		_selectedBandId = -1;         //页眉/页脚同为图表固定子节点，仅保险清引用

	CurveDocument* doc = mCurveDocController::getInstance()->document();
	if (nodeId == _chartId)
	{
		emit sig_requestClose();   //图表本身被删（撤销新建），通知工厂关闭视口
		return;
	}
		//nodeRemoved 时节点已从文档移除无法反查类型：
		//若渲染着该注释（含撤销注释创建），增量摘除即可；否则保守全量重建
		_pendingAnnotationIds.remove(nodeId);
		if (QwtPlotItem* item = _annotationMarkers.take(nodeId))
		{
			delete item;
			if (QwtPlot* qwt = _plot->getQwtPlot())
				qwt->replot();
			return;
		}
		//基准线渲染项在引擎侧（名 baseline_%1，节点id全局唯一不会撞名）：
		//命中即增量摘除（含撤销基准线创建），标签图元一并摘除；否则保守全量重建
		_pendingBaselineIds.remove(nodeId);
		const QString baselineMarker = QStringLiteral("baseline_%1").arg(nodeId);
		const bool hadLine = (_plot && _plot->getMarkerLine(baselineMarker) != nullptr);
		const bool hadLabel = _baselineLabels.contains(nodeId);
		removeBaselineLabel(nodeId);
		if (hadLine)
		{
			_plot->removeMarkerLine(baselineMarker);   //内部已 replot
			return;
		}
		if (hadLabel)
		{
			if (QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr)
				qwt->replot();
			return;
		}
		//删除曲线等子节点：重建但保留用户缩放（删除后已无可见曲线时重建内
		//仍会按数据适配）
		scheduleRebuild(true);
	}

	void mCurveChartView::onNodePropertyChanged(int nodeId, const QString& prop)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (doc->chartNodeId(nodeId) != _chartId && nodeId != _chartId)
			return;
		//注释属性（text/posX/posY/color）增量刷新（保留用户缩放状态）；
		//其余节点类型照旧全量重建
		if (nodeId != _chartId && doc->nodeType(nodeId) == CurveNodeType::Annotation)
		{
			//附着于切到曲线类/附着曲线或附着点变化：锚点吸附目标点（含占位文本
			//回填、序号钳制/框位置回写）；其后续提交再走下方增量刷新
			//（_snappingAnnotation 防再入）。kAttachMode 变更即"附着于"切换：
			//切到曲线类时按框当前位置取最近点（切到视图类时入口处 attachMode!=0
			//直接返回，标志无效）
			if (!_snappingAnnotation
				&& (prop == CurveProp::kAttachMode || prop == CurveProp::kAttachCurve
					|| prop == CurveProp::kAttachIndex))
			{
				snapAnnotationToCurve(nodeId, prop == CurveProp::kAttachMode);
			}
			scheduleAnnotationRefresh(nodeId);
			return;
		}
		//基准线属性（方向/位置值/颜色/线型/线宽/显隐，属性页实时改动）增量刷新
		//（保留用户缩放状态）
		if (nodeId != _chartId && doc->nodeType(nodeId) == CurveNodeType::Baseline)
		{
			scheduleBaselineRefresh(nodeId);
			return;
		}
		//轴刻度细项（刻度总数/生成方式/固定步长/网格数/格式/精度/每十倍参数）：
		//只影响刻度线与数字外观，走轻量轴刷新保留用户缩放/平移——全量重建会经
		//fitChartAxes 重算自动范围，把缩放状态清掉。改数据变换的键（缩放方式/
		//参考值/计权/范围）不在此列，仍走全量重建
		{
			const CurveNodeType type = doc->nodeType(nodeId);
			const bool isAxis = (type == CurveNodeType::XAxis || type == CurveNodeType::YAxis
				|| type == CurveNodeType::XAxis2 || type == CurveNodeType::YAxis2);
			static const QSet<QString> tickKeys = {
				CurveProp::kTickCount, CurveProp::kTickGenMode, CurveProp::kMajorStep,
				CurveProp::kMinorCount, CurveProp::kLabelFormat, CurveProp::kPrecision,
				CurveProp::kDecadeTickCount, CurveProp::kDecadeMinorCount };
			if (nodeId != _chartId && isAxis && tickKeys.contains(prop))
			{
				scheduleAxisRefresh(nodeId);
				return;
			}
		}
		//曲线外观/标签/可见性键（线色/线宽/线型/区域填充/符号属性/标签/可见性）：
		//applySeries 直发引擎不经全量重建（rebuild 经 fitChartAxes 重算自动范围
		//会重置用户缩放，且清空重画引发闪烁）。可见性隐藏在 flush 时从引擎摘除
		//曲线（图例同步消失，与重建语义一致）。复数曲线同为增量：flush 时经
		//applyComplexSeriesStyle 上/下面板样式一并下发（setComplexCurveStyle）；
		//编辑曲线会话（压暗符号需整体重放）仍走全量
		{
			static const QSet<QString> styleKeys = {
				CurveProp::kColor, CurveProp::kLineWidth, CurveProp::kLineStyle,
				CurveProp::kAreaFill, CurveProp::kVisible,
				CurveProp::kSymbolStyle, CurveProp::kSymbolSize, CurveProp::kSymbolColor,
				CurveProp::kSymbolDensity, CurveProp::kSymbolScale,
				CurveProp::kLabelVisible, CurveProp::kLabelContent, CurveProp::kLabelText,
				CurveProp::kLabelPrefix, CurveProp::kLabelSuffix,
				CurveProp::kLabelPrefixVisible, CurveProp::kLabelSuffixVisible };
			if (nodeId != _chartId && doc->nodeType(nodeId) == CurveNodeType::Curve
				&& styleKeys.contains(prop)
				&& _editCurveIds.isEmpty())
			{
				scheduleSeriesStyleRefresh(nodeId);
				return;
			}
		}
		//复数显示格式切换（设计图01，kComplexDisplayFormat）：上/下面板标题为
		//图表固有分量名（Phase/Imaginary、Magnitude/Real），随格式经
		//complexPanelComponentTitle 在 applyAxis/applyComplexLowerAxis 派生下发，
		//轴节点标题不参与面板显示（Y1 标题=数据类型，走中间共享标签位），
		//此处无标题改写；渲染换算随本属性变更走全量重建（下方 scheduleRebuild）
		//轴标题/标题字体/颜色为纯外观（不影响范围），全量重建但保留用户缩放
		const CurveNodeType chgType = doc->nodeType(nodeId);
		if (chgType == CurveNodeType::XAxis || chgType == CurveNodeType::YAxis
			|| chgType == CurveNodeType::XAxis2 || chgType == CurveNodeType::YAxis2)
		{
			if (prop == CurveProp::kTitle || prop == CurveProp::kTitleFont
				|| prop == CurveProp::kTitleColor)
			{
				if (nodeId != _chartId)
				{
					scheduleRebuild(true);
					return;
				}
			}
		}
		//其余属性（含曲线数据 "data" 直写与曲线改名 kName）全量重建：改名会
		//改变引擎键映射（键随 rebuild 重算，重复组的"首个"归属可能移动——如
		//首条 STRAIN 改名后第二条的键由 "STRAIN#2" 回归 "STRAIN"），任一键
		//变化都伴随引擎 clearAll 全量重画，映射与引擎注册表在 rebuild 内同步
		//重建，不做增量键补算（改名低频，安全优先）
		scheduleRebuild();
	}

void mCurveChartView::onCurrentNodeChanged(int nodeId)
{
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	//仅本图表的曲线节点点亮高亮；其他节点（别的图表/轴/树根等）清除本图高亮
	int newSel = -1;
	if (doc->nodeType(nodeId) == CurveNodeType::Curve && doc->chartNodeId(nodeId) == _chartId)
		newSel = nodeId;
	//本图表的注释节点选中 -> 点亮主题蓝选中框（图上点选/树上点选同经选择中枢）
	int newAnnSel = -1;
	if (doc->nodeType(nodeId) == CurveNodeType::Annotation && doc->chartNodeId(nodeId) == _chartId)
		newAnnSel = nodeId;
	//本图表的坐标轴节点（X/Y主轴与 X2/Y2 副轴）选中 -> 标题主题蓝+加粗
	//（设计图05/06：双击轴标题/轴区右键/树上点选同经选择中枢；副轴条带以
	//setSelected 重放选中态）
	int newAxisSel = -1;
	if ((doc->nodeType(nodeId) == CurveNodeType::YAxis
		|| doc->nodeType(nodeId) == CurveNodeType::XAxis
		|| doc->nodeType(nodeId) == CurveNodeType::YAxis2
		|| doc->nodeType(nodeId) == CurveNodeType::XAxis2)
		&& doc->chartNodeId(nodeId) == _chartId)
		newAxisSel = nodeId;
	//本图表的图例节点选中 -> 图例主题蓝外框（设计图08：双击图例空白/
	//右键图例"编辑"/树上点选同经选择中枢）
	int newLegendSel = -1;
	if (doc->nodeType(nodeId) == CurveNodeType::Legend && doc->chartNodeId(nodeId) == _chartId)
		newLegendSel = nodeId;
	//本图表的页眉/页脚节点选中 -> 条带文字主题蓝+加粗（双击条带/树上点选同经
	//选择中枢；Header/Footer 共用 _selectedBandId，applyHeader/applyFooter 内
	//按选中态着色）
	int newBandSel = -1;
	if ((doc->nodeType(nodeId) == CurveNodeType::Header
		|| doc->nodeType(nodeId) == CurveNodeType::Footer)
		&& doc->chartNodeId(nodeId) == _chartId)
		newBandSel = nodeId;
	if (newSel == _selectedCurveId && newAnnSel == _selectedAnnotationId
		&& newAxisSel == _selectedAxisId && newLegendSel == _selectedLegendId
		&& newBandSel == _selectedBandId)
		return;
	_selectedCurveId = newSel;
	applyHighlightVisuals();
	_selectedAnnotationId = newAnnSel;
	applyAnnotationSelection();
	_selectedAxisId = newAxisSel;
	applyAxisTitleHighlight();
	_selectedLegendId = newLegendSel;
	applyLegendSelection();
	_selectedBandId = newBandSel;
	applyHeader();
	applyFooter();
}

//--- 引擎键 <-> 节点映射（R23 曲线允许重名：渲染键与模型名解耦） ---

void mCurveChartView::ensureEngineCurveKeys()
{
	_engineKeyByNode.clear();
	_nodeByEngineKey.clear();
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	if (!doc->node(_chartId))
		return;

	//先收集全部曲线模型名：生成键必须避开"任何模型名"——防用户真建了名为
	//"STRAIN#2" 的曲线时，重复 STRAIN 的生成键与它串键
	QSet<QString> allNames;
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (n && n->type == CurveNodeType::Curve)
			allNames.insert(n->series.name);
	}

	//按树序逐节点生成键：模型名首次出现 -> 键=名（无碰撞图表键=名，存量
	//行为逐位不变，图例/命中/导出与既有完全一致）；第 N 次重复 -> 依次尝试
	//名#2/名#3… 取首个不与任何模型名及已生成键冲突者。复数曲线（单节点）
	//同规则，上/下面板共用同一键
	QHash<QString, int> seenName;   //模型名 -> 已出现次数
	QSet<QString> usedKeys;         //已生成的键
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (!n || n->type != CurveNodeType::Curve)
			continue;
		const QString& name = n->series.name;
		const int nth = ++seenName[name];
		QString key = name;
		if (nth > 1)
		{
			int suffix = 2;
			do
			{
				key = name + QStringLiteral("#") + QString::number(suffix);
				++suffix;
			} while (allNames.contains(key) || usedKeys.contains(key));
		}
		usedKeys.insert(key);
		_engineKeyByNode.insert(childId, key);
		_nodeByEngineKey.insert(key, childId);
	}
}

QString mCurveChartView::keyFor(int curveNodeId) const
{
	const QString key = _engineKeyByNode.value(curveNodeId);
	if (!key.isEmpty())
		return key;
	//映射缺失兜底（理论上 rebuild 后本图表曲线节点必有键）：回退模型名，
	//等同既有按名行为（无碰撞时键本就=名，行为不变）
	CurveNode* n = mCurveDocController::getInstance()->document()->node(curveNodeId);
	if (n && n->type == CurveNodeType::Curve)
		return n->series.name;
	return QString();
}

QString mCurveChartView::engineKeyForCurveName(const QString& modelName) const
{
	//模型名 -> 引擎键：本图表内首个同名曲线节点（注释 kAttachCurve 按模型名
	//存储，渲染查找须经键；重名时附着语义=首个同名节点）。无匹配回退原名——
	//键=名的无碰撞路径两者恒等
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (n && n->type == CurveNodeType::Curve && n->series.name == modelName)
			return keyFor(childId);
	}
	return modelName;
}

QString mCurveChartView::curveModelName(const QString& engineKey) const
{
	//引擎键 -> 模型名（{Label} 模板显示与注释 kAttachCurve 模型写入用；
	//键未登记时回退键本身——键=名的无碰撞路径两者恒等）
	const int nodeId = _nodeByEngineKey.value(engineKey, -1);
	if (nodeId > 0)
	{
		CurveNode* n = mCurveDocController::getInstance()->document()->node(nodeId);
		if (n && n->type == CurveNodeType::Curve)
			return n->series.name;
	}
	return engineKey;
}

int mCurveChartView::engineNodeByKey(const QString& key) const
{
	if (key.isEmpty())
		return -1;
	//优先经引擎键反向映射：键唯一，重名曲线各自命中（图例点击/画布点选
	//落在哪条即选哪条，与 HyperGraph 一致）
	const int byKey = _nodeByEngineKey.value(key, -1);
	if (byKey > 0)
		return byKey;
	//键未登记（映射尚未建立等瞬态）回退按模型名匹配首条，历史路径兜底；
	//复数曲线已合并为单节点（名字即引擎复数曲线名），精确匹配首遍直接命中，
	//旧"对键→上面板侧节点"令牌回查随模型合并退役
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (n && n->type == CurveNodeType::Curve && n->series.name == key)
			return childId;
	}
	return -1;
}

	void mCurveChartView::selectCurve(int curveNodeId)
	{
		if (curveNodeId <= 0)
			return;
		//数据隔离：类型筛选下作用域外类型不选中（如注释筛选模式下点曲线）
		if (!mCurveDocController::getInstance()->canSelectNode(curveNodeId))
			return;
		//经选择中枢分发：树回选、参数栏换页、本图高亮全部由 sig_currentNodeChanged 闭环
		mCurveDocController::getInstance()->setCurrentNodeId(curveNodeId);
	}

void mCurveChartView::applyHighlightVisuals()
{
	//编辑曲线会话期间不做选中/hover 高亮：20%压暗是该期间的视图状态
	//（设计图16），会话结束/重建时由 restoreEditCurveStyles/applyEditDimming 接管
	if (_editMode)
		return;
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (!n || n->type != CurveNodeType::Curve || !n->series.visible)
			continue;
		//复数曲线单节点：键即引擎上/下面板子曲线的键（命中/高亮都按键取）。
		//R23 曲线允许重名：高亮重放按 nodeId -> 键，两条同名曲线可分别高亮
		const QString engineKey = keyFor(childId);
		QwtPlotCurve* curve = _plot->getCurve(engineKey);
		if (!curve)
			continue;

		//以引擎当前设置为基础恢复，再按状态覆盖：只改画笔不写引擎设置，
		//图例图标本色不受影响（选中是视图状态，不进模型/命令栈）
		PlotView::CurveSettings base = _plot->getCurveSettings(engineKey);
		QPen pen = curve->pen();
		pen.setColor(base.lineColor);
		pen.setWidthF(base.lineWidth);

		if (childId == _selectedCurveId || childId == _hoveredCurveId)
		{
			pen.setColor(curveHighlightBlue());
			if (childId == _selectedCurveId)
				pen.setWidthF(base.lineWidth + 1.0);   //设计图03.1：选中加粗1px，hover仅变色
		}
		curve->setPen(pen);

		//柱状图形态（图表类型1）：主曲线为 NoCurve，高亮重放到柱状辅曲线
		//（仅变色不加粗——柱宽是形态参数，随选中变化会误导为线宽属性）
		if (base.showBars)
		{
			if (QwtPlotCurve* bar = _plot->getBarCurve(engineKey))
			{
				QPen barPen = bar->pen();
				barPen.setColor(base.barColor);
				if (childId == _selectedCurveId || childId == _hoveredCurveId)
					barPen.setColor(curveHighlightBlue());
				bar->setPen(barPen);
			}
		}

		//复数图形态（图表类型2）：下面板子曲线同步高亮/恢复（选中=主题蓝+
		//加粗1px，hover 仅变色——对齐上面板选中效果）
		if (n->series.isComplex())
		{
			if (QwtPlotCurve* low = _plot->getLowerCurve(engineKey))
			{
				QPen lowPen = low->pen();
				lowPen.setColor(base.lineColor);
				lowPen.setWidthF(base.lineWidth);
				if (childId == _selectedCurveId || childId == _hoveredCurveId)
				{
					lowPen.setColor(curveHighlightBlue());
					if (childId == _selectedCurveId)
						lowPen.setWidthF(base.lineWidth + 1.0);
				}
				low->setPen(lowPen);
			}
		}
	}
	//极坐标形态：引擎载体曲线不可见，选中/hover 高亮重放到自绘向量层（同链
	//同规：选中=主题蓝+加粗 1px，hover 仅变色）
	if (_polarSeriesItem)
	{
		_polarSeriesItem->setSelectedNodeId(_selectedCurveId);
		_polarSeriesItem->setHoveredNodeId(_hoveredCurveId);
	}

	if (QwtPlot* qwt = _plot->getQwtPlot())
		qwt->replot();
	//复数下面板是独立 QwtPlot：改其子曲线画笔必须单独 replot（qwt->replot 不覆盖
	//下面板，否则画笔已改而画面不刷新，看上去"下面板没有选中效果"）
	if (_plot->isComplexMode())
	{
		if (QwtPlot* lowerQwt = _plot->getLowerQwtPlot())
			lowerQwt->replot();
	}
}

void mCurveChartView::applyBaselineHoverVisuals()
{
	//编辑曲线会话期间不做 hover 高亮（同曲线高亮策略：压暗是该期间的视图状态）
	if (_editMode)
		return;
	//直改引擎 markerLine 画笔（hover 是视图状态不进模型/命令栈），恢复时
	//颜色/线宽从模型重读（applyBaseline 刚写回的就是模型值）
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (!n || n->type != CurveNodeType::Baseline || !n->baseline.visible)
			continue;
		PlotView::mPlotMarkerLine* line =
			_plot->getMarkerLine(QStringLiteral("baseline_%1").arg(childId));
		if (!line)
			continue;
		if (childId == _hoveredBaselineId)
		{
			//主题蓝+加粗1px：默认蓝基准线（0,0,255）与主题蓝接近，线宽变化
			//保证高亮可见（选中/hover 配色与曲线侧一致）
			line->setLineColor(curveHighlightBlue());
			line->setLineWidth(n->baseline.lineWidth + 1.0);
		}
		else
		{
			line->setLineColor(n->baseline.color);
			line->setLineWidth(n->baseline.lineWidth);
		}
	}
	if (QwtPlot* qwt = _plot->getQwtPlot())
		qwt->replot();
}

//--- 编辑曲线会话（Ribbon"编辑曲线"按钮经视口工厂广播到此；状态只存工厂，
//    本视图为纯投影：覆盖层随会话信号增删/刷新，rebuild 后重放） ---

void mCurveChartView::onEditCurveStarted(int chartId)
{
	if (chartId != _chartId)
		return;
	mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
	_editMode = true;
	_editCurveIds = f->editCurveIds();
	_editDragLine = -1;
	buildEditOverlay();   //含各目标截取段重采样与全曲线压暗
}

void mCurveChartView::onEditCurveTargetChanged()
{
	if (!_editMode)
		return;
	_editCurveIds = mCurveViewportFactory::getInstance()->editCurveIds();
	refreshEditSegment();   //段集合随新目标集重建（段样式各自在内部重取）
	applyEditDimming();
	if (QwtPlot* qwt = _plot->getQwtPlot())
		qwt->replot();
}

void mCurveChartView::onEditCurveRangeChanged(double start, double end)
{
	Q_UNUSED(start)
	Q_UNUSED(end)
	if (!_editMode)
		return;
	refreshEditSegment();
	if (QwtPlot* qwt = _plot->getQwtPlot())
		qwt->replot();
}

void mCurveChartView::onEditCurveDirectionChanged(int direction)
{
	Q_UNUSED(direction)
	if (!_editMode)
		return;
	refreshEditSegment();   //含裁剪线方向切换与截取段按新轴重采样
	if (QwtPlot* qwt = _plot->getQwtPlot())
		qwt->replot();
}

void mCurveChartView::onEditCurveStopped()
{
	if (!_editMode)
		return;
	_editMode = false;
	_editCurveIds.clear();
	_editDragLine = -1;
	clearEditOverlay();
	restoreEditCurveStyles();
	if (QwtPlot* qwt = _plot->getQwtPlot())
		qwt->replot();
}

void mCurveChartView::buildEditOverlay()
{
	QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
	if (!qwt)
		return;
	//截取段旁路曲线（每条目标一段）在 refreshEditSegment 内按目标集同步创建
	if (!_editTrimStart)
	{
		_editTrimStart = new mCurveTrimMarker(mCurveTrimMarker::Start);
		_editTrimStart->attach(qwt);
		_editTrimStart->setZ(60.0);
	}
	if (!_editTrimEnd)
	{
		_editTrimEnd = new mCurveTrimMarker(mCurveTrimMarker::End);
		_editTrimEnd->attach(qwt);
		_editTrimEnd->setZ(60.0);
	}
	refreshEditSegment();
	applyEditDimming();
}

void mCurveChartView::clearEditOverlay()
{
	//析构即自动 detach；先删截取段再删裁剪线（同为旁路自管图元）
	qDeleteAll(_editSegments);
	_editSegments.clear();
	delete _editTrimStart;
	_editTrimStart = nullptr;
	delete _editTrimEnd;
	_editTrimEnd = nullptr;
}

void mCurveChartView::applyEditDimming()
{
	QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
	if (!qwt)
		return;
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (!n || n->type != CurveNodeType::Curve || !n->series.visible)
			continue;
		QwtPlotCurve* curve = _plot->getCurve(keyFor(childId));   //按键（R23）
		if (!curve)
			continue;

		//设计图16：编辑态下其他曲线及目标曲线全段 20% 不透明度。
		//直改画笔（同选中/hover 高亮策略，视图状态不进模型/命令栈），
		//恢复由 restoreEditCurveStyles 按引擎设置整体重设
		QPen pen = curve->pen();
		QColor c = pen.color();
		if (c.isValid())
		{
			c.setAlphaF(0.2);
			pen.setColor(c);
			curve->setPen(pen);
		}
		//符号同步压暗（有符号曲线保持观感一致；QwtSymbol 拷贝构造已删除，
		//按四元组重建，注意构造顺序为 Brush、Pen；用 mPlotSymbol 重建以支持
		//Sym_Star5 等扩展样式，基类按原样式重建会画不出形状）
		if (const QwtSymbol* sym = curve->symbol())
		{
			QwtSymbol* dim = new mPlotSymbol(sym->style(), sym->brush(), sym->pen(), sym->size());
			if (dim->pen().color().isValid())
			{
				QPen sp = dim->pen();
				QColor sc = sp.color();
				sc.setAlphaF(0.2);
				sp.setColor(sc);
				dim->setPen(sp);
			}
			if (dim->brush().color().isValid())
			{
				QBrush sb = dim->brush();
				QColor bc = sb.color();
				bc.setAlphaF(0.2);
				sb.setColor(bc);
				dim->setBrush(sb);
			}
			curve->setSymbol(dim);
		}
	}
	qwt->replot();
}

void mCurveChartView::restoreEditCurveStyles()
{
	//按引擎缓存设置整体重设样式：画笔/符号/填充全部回满不透明度
	//（与 setCurveStyle 下发路径一致，不残留压暗）。按键取（R23）
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (!n || n->type != CurveNodeType::Curve || !n->series.visible)
			continue;
		const QString engineKey = keyFor(childId);
		if (!_plot->getCurve(engineKey))
			continue;
		const CurveSettings cs = _plot->getCurveSettings(engineKey);
		if (cs.name == engineKey)
			_plot->setCurveStyle(engineKey, cs);
	}
}

void mCurveChartView::refreshEditSegment()
{
	QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
	if (!qwt || !_editTrimStart || !_editTrimEnd)
		return;
	mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
	const int direction = f->editDirection();
	const double startV = f->editStart();
	const double endV = f->editEnd();

	CurveDocument* doc = mCurveDocController::getInstance()->document();

	//旁路段集合与目标集同步（多目标：每条目标曲线一段）。段直接 attach 到
	//QwtPlot，不进引擎注册表——不占图例、不参与引擎曲线命中（Ctrl+点选
	//命中的是底下压暗的原曲线，同名同几何，语义一致；设计图16）
	QHash<int, QwtPlotCurve*> keep;
	for (int curveId : _editCurveIds)
	{
		QwtPlotCurve* seg = _editSegments.value(curveId, nullptr);
		if (!seg)
		{
			seg = new QwtPlotCurve;
			seg->attach(qwt);
			seg->setZ(12.0);   //普通曲线(10)之上、基准线(50)之下
			_editSegments.insert(curveId, seg);
		}
		keep.insert(curveId, seg);

		CurveNode* n = doc->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
		{
			seg->setSamples(QVector<double>(), QVector<double>());
			continue;
		}
		//与 applySeries 相同的预变换（含 缩放/偏移 与计权）/滤点，再按编辑方向裁剪到范围内
		const CurveSeriesData& s = n->series;
		const int xMode = axisScaleModeOf(CurveNodeType::XAxis);
		const int yMode = axisScaleModeOf(CurveNodeType::YAxis);
		const int xWeight = axisWeightModeOf(CurveNodeType::XAxis);
		const int yWeight = axisWeightModeOf(CurveNodeType::YAxis);
		const double xDbRef = axisDbRefOf(CurveNodeType::XAxis);
		const double yDbRef = axisDbRefOf(CurveNodeType::YAxis);
		const int count = qMin(s.xData.size(), s.yData.size());
		QVector<double> xs, ys;
		xs.reserve(count);
		ys.reserve(count);
		for (int i = 0; i < count; ++i)
		{
			double tx = 0.0, ty = 0.0;
			transformPoint(s.xRef.applyTo(s.xData.at(i)),
				s.yRef.applyTo(s.yData.at(i)),
				xMode, yMode, xWeight, yWeight, tx, ty, xDbRef, yDbRef);
			if (qIsNaN(tx) || qIsNaN(ty))
				continue;
			const double v = (direction == 0) ? tx : ty;
			if (v < startV || v > endV)
				continue;
			xs.append(tx);
			ys.append(ty);
		}
		seg->setSamples(xs, ys);

		//段样式随各自目标曲线：目标在引擎中有渲染（可见）时取其当前画笔并
		//回满不透明度（虚线样式一并保留）；不可见目标退回模型样式（编辑
		//隐藏曲线时截取段仍可见）。按键取（R23）
		if (QwtPlotCurve* target = _plot->getCurve(keyFor(curveId)))
		{
			seg->setXAxis(target->xAxis());
			seg->setYAxis(target->yAxis());
			QPen pen = target->pen();
			QColor c = pen.color();
			if (c.isValid())
			{
				c.setAlphaF(1.0);
				pen.setColor(c);
			}
			seg->setPen(pen);
		}
		else
		{
			seg->setXAxis(QwtPlot::xBottom);
			seg->setYAxis(QwtPlot::yLeft);
			QPen pen(s.color);
			pen.setWidthF(s.lineWidth);
			seg->setPen(pen);
		}
	}
	//清理已不在目标集里的段
	for (auto it = _editSegments.begin(); it != _editSegments.end();)
	{
		if (!keep.contains(it.key()))
		{
			delete it.value();
			it = _editSegments.erase(it);
		}
		else
			++it;
	}

	//裁剪线位置/方向同步 + 拖拽高亮（拖拽中的一根加粗反馈）
	const mCurveTrimMarker::Orientation orient = (direction == 0)
		? mCurveTrimMarker::Vertical     //水平截取 = 竖直裁剪线
		: mCurveTrimMarker::Horizontal;  //垂直截取 = 水平裁剪线
	_editTrimStart->setOrientation(orient);
	_editTrimEnd->setOrientation(orient);
	_editTrimStart->setValue(startV);
	_editTrimEnd->setValue(endV);
	_editTrimStart->setHighlighted(_editDragLine == 0);
	_editTrimEnd->setHighlighted(_editDragLine == 1);
}

int mCurveChartView::hitEditTrimLine(const QPoint& canvasPos) const
{
	if (!_editMode)
		return -1;
	QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
	if (!qwt || !_editTrimStart || !_editTrimEnd)
		return -1;
	mCurveViewportFactory* f = mCurveViewportFactory::getInstance();
	const bool horizontal = (f->editDirection() == 0);
	const QwtScaleMap map = horizontal ? qwt->canvasMap(QwtPlot::xBottom)
		: qwt->canvasMap(QwtPlot::yLeft);
	const double pos = horizontal ? canvasPos.x() : canvasPos.y();
	const double s = map.transform(f->editStart());
	const double e = map.transform(f->editEnd());
	//±5px 命中带（设计图16：鼠标按住拖拽裁剪线）
	if (std::abs(pos - s) <= 5.0)
		return 0;
	if (std::abs(pos - e) <= 5.0)
		return 1;
	return -1;
}

double mCurveChartView::editAxisPixelToValue(const QPoint& canvasPos) const
{
	QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
	if (!qwt)
		return 0.0;
	const bool horizontal = (mCurveViewportFactory::getInstance()->editDirection() == 0);
	const QwtScaleMap map = horizontal ? qwt->canvasMap(QwtPlot::xBottom)
		: qwt->canvasMap(QwtPlot::yLeft);
	return map.invTransform(horizontal ? canvasPos.x() : canvasPos.y());
}

int mCurveChartView::yAxisNodeId() const
{
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (n && n->type == CurveNodeType::YAxis)
			return childId;
	}
	return -1;
}

int mCurveChartView::xAxisNodeId() const
{
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (n && n->type == CurveNodeType::XAxis)
			return childId;
	}
	return -1;
}

int mCurveChartView::legendNodeId() const
{
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (n && n->type == CurveNodeType::Legend)
			return childId;
	}
	return -1;
}

int mCurveChartView::bandNodeId(CurveNodeType type) const
{
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	for (int childId : doc->childNodeIds(_chartId))
	{
		CurveNode* n = doc->node(childId);
		if (n && n->type == type)
			return childId;
	}
	return -1;
}

bool mCurveChartView::hitYAxisTitle(const QPoint& pos) const
{
	QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
	QwtScaleWidget* scale = qwt ? qwt->axisWidget(QwtPlot::yLeft) : nullptr;
	if (!scale)
		return false;
	//QwtScaleWidget 无公开标题区域 API，按布局近似：左轴标题旋转90°绘制在
	//控件最左侧条带，带宽 = 边距 + 标题文本高度（旋转后即文字行高方向）+ 间距，
	//外加 2px 容差便于命中细字体；刻度数字在条带之外，天然区分。
	//空标题不占条带，不命中（否则容差条带会盖到刻度数字区）
	const AxisSettings as = _plot->getAxisSettings(AxisId::Y_Left);
	if (as.title.isEmpty())
		return false;
	const int band = scale->margin() + QFontMetrics(as.titleFont).height()
		+ scale->spacing() + 2;
	return pos.x() <= band;
}

	bool mCurveChartView::hitXAxisTitle(const QPoint& pos) const
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		QwtScaleWidget* scale = qwt ? qwt->axisWidget(QwtPlot::xBottom) : nullptr;
		if (!scale)
			return false;
		//同纵轴按布局近似，方向不同：下轴标题水平绘制在控件最下侧条带，
		//带高 = 边距 + 标题文本高度 + 间距 + 2px 容差，自底边起算；
		//刻度数字在条带之外，空标题不占条带不命中
		const AxisSettings as = _plot->getAxisSettings(AxisId::X_Bottom);
		if (as.title.isEmpty())
			return false;
		const int band = scale->margin() + QFontMetrics(as.titleFont).height()
			+ scale->spacing() + 2;
		return pos.y() >= scale->height() - band;
	}

	bool mCurveChartView::hitTopAxisTitle(const QPoint& pos) const
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		QwtScaleWidget* scale = qwt ? qwt->axisWidget(QwtPlot::xTop) : nullptr;
		if (!scale)
			return false;
		//上轴（首枚副轴 X2）标题画在控件最上侧条带，带高自顶边起算
		//（与下轴镜像）；空标题不占条带不命中
		const AxisSettings as = _plot->getAxisSettings(AxisId::X_Top);
		if (as.title.isEmpty())
			return false;
		const int band = scale->margin() + QFontMetrics(as.titleFont).height()
			+ scale->spacing() + 2;
		return pos.y() <= band;
	}

	bool mCurveChartView::hitRightAxisTitle(const QPoint& pos) const
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		QwtScaleWidget* scale = qwt ? qwt->axisWidget(QwtPlot::yRight) : nullptr;
		if (!scale)
			return false;
		//右轴（首枚副轴 Y2）标题旋转90°画在控件最右侧条带，带宽自右边起算
		//（与左轴镜像）；空标题不占条带不命中
		const AxisSettings as = _plot->getAxisSettings(AxisId::Y_Right);
		if (as.title.isEmpty())
			return false;
		const int band = scale->margin() + QFontMetrics(as.titleFont).height()
			+ scale->spacing() + 2;
		return pos.x() >= scale->width() - band;
	}

	//复数图下面板 Y 轴标题命中：布局近似同上面板左轴（旋转90°，控件最左侧
	//条带），但几何/标题取下面板控件——标题字体取轴标题实际字体（覆盖口下发，
	//选中加粗后带宽随之变化，读实际值更稳）；空标题不占条带不命中
	bool mCurveChartView::hitLowerYAxisTitle(const QPoint& pos) const
	{
		QwtPlot* lowerQwt = _plot ? _plot->getLowerQwtPlot() : nullptr;
		QwtScaleWidget* scale = lowerQwt ? lowerQwt->axisWidget(QwtPlot::yLeft) : nullptr;
		if (!scale)
			return false;
		const QwtText title = lowerQwt->axisTitle(QwtPlot::yLeft);
		if (title.text().isEmpty())
			return false;
		const int band = scale->margin() + QFontMetrics(title.font()).height()
			+ scale->spacing() + 2;
		return pos.x() <= band;
	}

	//复数图下面板 X 轴标题命中：下轴标题水平绘制在控件最下侧条带，带高自底边
	//起算（同上面板下轴近似，几何取下面板控件）；空标题不占条带不命中
	bool mCurveChartView::hitLowerXAxisTitle(const QPoint& pos) const
	{
		QwtPlot* lowerQwt = _plot ? _plot->getLowerQwtPlot() : nullptr;
		QwtScaleWidget* scale = lowerQwt ? lowerQwt->axisWidget(QwtPlot::xBottom) : nullptr;
		if (!scale)
			return false;
		const QwtText title = lowerQwt->axisTitle(QwtPlot::xBottom);
		if (title.text().isEmpty())
			return false;
		const int band = scale->margin() + QFontMetrics(title.font()).height()
			+ scale->spacing() + 2;
		return pos.y() >= scale->height() - band;
	}

	//--- 多坐标轴（顶部 X2..Xn / 右侧 Y2..Yn） ---

	QList<int> mCurveChartView::secondaryAxisNodes(bool vertical) const
	{
		QList<int> out;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const CurveNodeType type = vertical ? CurveNodeType::YAxis2 : CurveNodeType::XAxis2;
		for (int childId : doc->childNodeIds(_chartId))
		{
			if (doc->nodeType(childId) == type)
				out.append(childId);
		}
		//childInsertionRow 保证副轴按创建顺序追加在轴区尾部：列表序=堆叠由内到外
		return out;
	}

	int mCurveChartView::nativeSecondaryAxisId(bool vertical) const
	{
		const QList<int> nodes = secondaryAxisNodes(vertical);
		return nodes.isEmpty() ? -1 : nodes.first();
	}

	//复数图上/下面板轴节点（HyperGraph 2024 排布：相位/虚部侧（Y2）默认在上，
	//幅值/实部侧（Y1）在下；kComplexSwapped 互换态对调）。节点缺失时出参 -1
	void mCurveChartView::complexPanelAxisIds(int* topAxisId, int* bottomAxisId) const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chartNode = doc ? doc->node(_chartId) : nullptr;
		const bool swapped = chartNode && chartNode->chart.complexSwapped;
		int y1 = -1, y2 = -1;
		if (doc)
		{
			for (int childId : doc->childNodeIds(_chartId))
			{
				CurveNode* c = doc->node(childId);
				if (!c)
					continue;
				if (c->type == CurveNodeType::YAxis && y1 < 0)
					y1 = childId;
				else if (c->type == CurveNodeType::YAxis2 && y2 < 0)
					y2 = childId;
			}
		}
		if (topAxisId)
			*topAxisId = swapped ? y1 : y2;
		if (bottomAxisId)
			*bottomAxisId = swapped ? y2 : y1;
	}

	void mCurveChartView::applySecondaryAxes()
	{
		resetSecondaryEngineAxes();
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chartNode = doc ? doc->node(_chartId) : nullptr;
		const bool complexChart = chartNode && chartNode->chart.chartType == 2;
		if (complexChart)
		{
			//复数形态：下面板轴节点（排布映射：默认幅值/实部侧 Y1，互换态为
			//相位/虚部侧 Y2）配置下发引擎下面板 Y，上/右原生副轴与自绘条带均
			//不适用（双面板共享 X，无右侧轴区）
			int topAxisId = -1, bottomAxisId = -1;
			complexPanelAxisIds(&topAxisId, &bottomAxisId);
			Q_UNUSED(topAxisId);
			bool hasPhase = false;
			if (bottomAxisId > 0)
			{
				applyComplexLowerAxis(bottomAxisId);
				hasPhase = true;
			}
			if (!hasPhase)
			{
				//防御：无下面板轴节点时清除宿主覆盖、恢复引擎自动适配
				_plot->setLowerAxisTitleOverride(false);
				_plot->setLowerAxisAutoFit(true);
			}
			return;
		}
		//极坐标（D25/M10）：直角副轴/自绘条带无语义——applyPolarDecor 已摘直角
		//骨架（4 轴隐藏+边框隐藏），本处不再置回（旧 rebuild 时序本函数后跑，
		//加副轴后上/右直角轴复活与辐条叠画）
		if (chartNode && chartNode->chart.chartType == 3)
			return;
		//首枚副轴 -> 引擎原生上/右轴（刻度/数字/标题/字体全走引擎，fitChartAxes
		//已按挂轴曲线算好自动范围、手动范围在 applyAxis 内下发）
		const int topNative = nativeSecondaryAxisId(false);
		if (topNative > 0)
			applyAxis(topNative, AxisId::X_Top);
		const int rightNative = nativeSecondaryAxisId(true);
		if (rightNative > 0)
			applyAxis(rightNative, AxisId::Y_Right);
		//第2枚起 -> 自绘条带（读模型+镜像同侧原生轴当前范围）
		rebuildAxisStrips();
		updateAxisStrips();
	}

	//复数图下面板轴节点（排布映射见 complexPanelAxisIds：默认幅值/实部侧 Y1，
	//互换态为相位/虚部侧 Y2）属性下发到引擎下面板 Y 轴：标题=面板分量名（随显
	//示格式派生；引擎标题覆盖口，防 refreshComplexPanelAxes 格式派生标题冲掉）/
	//可见/刻度数字字体/刻度引擎/数字格式/范围（手动=刻度变换后 setAxisScale 并
	//关引擎自动适配；自动=恢复引擎按数据满幅）。下面板在复数形态无原生宿主轴，
	//全部经 getLowerQwtPlot + 引擎 AutoFit/标题覆盖口实现
	void mCurveChartView::applyComplexLowerAxis(int axisNodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(axisNodeId);
		QwtPlot* lowerQwt = _plot ? _plot->getLowerQwtPlot() : nullptr;
		if (!n || !lowerQwt
			|| (n->type != CurveNodeType::YAxis && n->type != CurveNodeType::YAxis2))
			return;
		CurveNode* chartNode = doc->node(_chartId);
		const int displayFmt = chartNode ? chartNode->chart.complexDisplayFormat : 0;
		const CurveAxisData& a = n->axis;

		//标题=面板分量名（随显示格式派生，HyperGraph 语义：面板标题为图表固有
		//分量名）；选中该轴节点时标题主题蓝+加粗（applyAxisTitleHighlight 经
		//本函数重推，与上面板选中观感一致）；titleVisible=false 即清空标题
		//（引擎约定"无独立标题显隐字段"）。覆盖口只暂存文本，须随即
		//refreshComplexPanelAxes 物化到下面板（镜像+标题应用），否则 replot
		//仍画上一次的标题——格式切换后下面板标题不更新的根因
		QColor titleColor = a.titleColor;
		QFont titleFont = a.titleFont;
		if (_selectedAxisId == axisNodeId)
		{
			titleColor = curveHighlightBlue();
			titleFont.setBold(true);
		}
		_plot->setLowerAxisTitleOverride(true,
			a.titleVisible ? complexPanelComponentTitle(
				n->type == CurveNodeType::YAxis, displayFmt) : QString(),
			titleFont, titleColor);
		_plot->refreshComplexPanelAxes();

		//轴可见性 / 刻度数字字体
		lowerQwt->enableAxis(QwtPlot::yLeft, a.visible);
		lowerQwt->setAxisFont(QwtPlot::yLeft, a.labelFont);

		//刻度引擎（同 applyAxis 的 makeTickEngine 分派语义，读 Phase 轴节点
		//配置；log/dB 下均匀/固定步长在刻度变换空间生效，缺省回落每十倍）
		if (a.scaleMode != 0)
		{
			if (a.tickGenMode == 1 && a.tickCount >= 2.0)
				lowerQwt->setAxisScaleEngine(QwtPlot::yLeft, new mCurveUniformTickEngine(a.tickCount, a.minorCount));
			else if (a.tickGenMode == 2 && a.majorStep > 0.0)
			{
				auto* tickEngine = new PlotView::TecPlotLinearScaleEngine();
				tickEngine->setFixedStep(a.majorStep);
				if (a.minorCount > 0)
					tickEngine->setMinTickCount(a.minorCount);
				lowerQwt->setAxisScaleEngine(QwtPlot::yLeft, tickEngine);
			}
			else
				lowerQwt->setAxisScaleEngine(QwtPlot::yLeft, new mCurveDecadeTickEngine(decadeWidthOf(a.scaleMode), a.decadeTickCount, a.decadeMinorCount));
		}
		else if (a.tickGenMode == 0)   //tickCount<=1 走引擎内自动档（同 makeTickEngine 线性口径）
			lowerQwt->setAxisScaleEngine(QwtPlot::yLeft, new mCurveUniformTickEngine(a.tickCount, a.minorCount));
		else
		{
			auto* tickEngine = new PlotView::TecPlotLinearScaleEngine();
			if (a.tickGenMode == 1 && a.majorStep > 0.0)
				tickEngine->setFixedStep(a.majorStep);
			if (a.minorCount > 0)
				tickEngine->setMinTickCount(a.minorCount);
			lowerQwt->setAxisScaleEngine(QwtPlot::yLeft, tickEngine);
		}
		//刻度数字格式/精度（mCurveScaleDraw；主干线关同 createLowerPlot 约定）
		if (!dynamic_cast<mCurveScaleDraw*>(lowerQwt->axisScaleDraw(QwtPlot::yLeft)))
			lowerQwt->setAxisScaleDraw(QwtPlot::yLeft, new mCurveScaleDraw());
		if (auto* scaleDraw = dynamic_cast<mCurveScaleDraw*>(lowerQwt->axisScaleDraw(QwtPlot::yLeft)))
		{
			scaleDraw->setNumberFormat(a.labelFormat, a.precision);
			scaleDraw->enableComponent(QwtAbstractScaleDraw::Backbone, false);
		}

		//范围：手动=模型值经 Phase 轴刻度模式变换后下发（标量无频率上下文，
		//仅刻度变换不加权，同 applyAxis 约定）并关引擎自动适配；自动=恢复引擎适配。
		//log/dB 下原始值<=0 变换为 NaN，NaN 进轴范围得坏轴：先钳到正数域再变换
		//（同 applyAxis 防线，linear 恒等不受影响）
		if (!a.autoScale)
		{
			_plot->setLowerAxisAutoFit(false);
			double rawLo = a.min;
			double rawHi = a.max;
			if (a.scaleMode != 0)
			{
				rawLo = qMax(rawLo, 1e-6);
				rawHi = qMax(rawHi, rawLo);
			}
			double lo = transformValue(rawLo, a.scaleMode, a.dbRef);
			double hi = transformValue(rawHi, a.scaleMode, a.dbRef);
			if (lo > hi)
				std::swap(lo, hi);
			lowerQwt->setAxisScale(QwtPlot::yLeft, lo, hi);
		}
		else
		{
			_plot->setLowerAxisAutoFit(true);
		}
		lowerQwt->replot();
	}

	void mCurveChartView::resetSecondaryEngineAxes()
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!qwt)
			return;
		//全量重建的复位语义：无副轴时上/右轴回到"完全不显示"（构造期同款状态，
		//无主干线/无刻度/无数字），不残留上次启用外观
		for (int axis : { QwtPlot::xTop, QwtPlot::yRight })
		{
			if (QwtAbstractScaleDraw* sd = qwt->axisScaleDraw(axis))
			{
				sd->enableComponent(QwtAbstractScaleDraw::Backbone, false);
				sd->enableComponent(QwtAbstractScaleDraw::Ticks, false);
				sd->enableComponent(QwtAbstractScaleDraw::Labels, false);
			}
			AxisSettings as = _plot->getAxisSettings(static_cast<AxisId>(axis));
			as.isVisible = false;
			as.title.clear();
			_plot->updateAxisScale(static_cast<AxisId>(axis), as);
		}
	}

	//用户平移/缩放（引擎实时平移/滚轮缩放手势期间逐步广播）：画布内曲线/网格
	//由引擎 replot 现场重生成，这里只同步引擎管不到的自绘副轴条带——自动范围
	//重镜像同侧引擎轴当前范围，手动范围重算结果不变。轻量刷新，严禁走
	//rebuild/scheduleRebuild（全量重建会重置用户缩放状态，且手势期间成风暴）
	void mCurveChartView::onViewRangeChanged()
	{
		for (mCurveAxisStripWidget* strip : _topStrips)
			strip->refreshScale();
		for (mCurveAxisStripWidget* strip : _rightStrips)
			strip->refreshScale();

		//轴属性页"最小值/最大值"联动：把手势期间的当前显示范围广播出去，
		//各轴页收到后回查回显（只影响面板数字，不写模型）
		notifyAxisDisplayRanges();
	}

	//向控制器广播本图表全部轴的"当前显示范围已变化"（缩放/平移/fit 后），
	//轴属性页收到后回查回显（只影响面板数字，不写模型）
	void mCurveChartView::notifyAxisDisplayRanges()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc)
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		for (int childId : doc->childNodeIds(_chartId))
		{
			const CurveNodeType type = doc->nodeType(childId);
			if (type == CurveNodeType::XAxis || type == CurveNodeType::YAxis
				|| type == CurveNodeType::XAxis2 || type == CurveNodeType::YAxis2)
				controller->notifyAxisDisplayRangeChanged(childId);
		}
	}

	void mCurveChartView::rebuildAxisStrips()
	{
		clearAxisStrips();
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		if (!qwt)
			return;
		//同侧第2枚起建条带（首枚占引擎原生轴）；交互经信号转回本视图
		//（轴区菜单/双击编辑器/悬停高亮）。
		//摆位分两种：顶部条带插入 mPlotView 内部布局（页眉与 QwtPlot 之间）——
		//页眉画在引擎顶部、X2 在 QwtPlot 内部，条带放引擎之外（视图外扩边距）
		//会跑到页眉上方；在"QwtPlot 当前位点"反复插入，后插的排在先插的上方，
		//实现由内向外堆叠（X3 紧贴 X2、X4 更靠上，页眉保持最顶）。
		//右侧条带维持视图外扩边距 + 手动几何（右侧无页眉类条带冲突）
		auto buildStrips = [this, qwt](const QList<int>& nodes, bool top,
			QList<mCurveAxisStripWidget*>& out)
		{
			//顶部条带的引擎内部布局插入点：QwtPlot 当前位点（页眉与绘图区之间）
			QBoxLayout* engineLayout = nullptr;
			int engineInsertIndex = -1;
			if (top)
			{
				engineLayout = qobject_cast<QBoxLayout*>(_plot->layout());
				if (engineLayout)
					engineInsertIndex = engineLayout->indexOf(qwt);
			}
			for (int i = 1; i < nodes.size(); ++i)
			{
				const int axisNodeId = nodes.at(i);
				//隐藏的副轴（"可见性"关）不建条带：显隐切换经属性变化触发重建，
				//重新显示随下次重建恢复
				CurveNode* axisNode = mCurveDocController::getInstance()->document()->node(axisNodeId);
				if (!axisNode || !axisNode->axis.visible)
					continue;
				QWidget* stripParent = top ? static_cast<QWidget*>(_plot)
					: static_cast<QWidget*>(this);
				mCurveAxisStripWidget* strip = new mCurveAxisStripWidget(top, stripParent);
				strip->setAxis(axisNodeId, qwt);
				if (top && engineLayout)
				{
					if (engineInsertIndex >= 0)
						engineLayout->insertWidget(engineInsertIndex, strip);
					//引擎布局不含 QwtPlot 时（结构变更）退化为压顶插入，仍可见可交互
					else
						engineLayout->insertWidget(0, strip);
				}
				strip->show();
				connect(strip, &mCurveAxisStripWidget::sig_contextMenuRequested, this,
					[this, axisNodeId, top](const QPoint& globalPos)
				{
					onAxisZoneContextMenu(axisNodeId, !top, globalPos);
				});
				connect(strip, &mCurveAxisStripWidget::sig_doubleClicked, this,
					[this, axisNodeId](const QPoint& globalPos)
				{
					Q_UNUSED(globalPos)
					//双击条带=双击轴标题同语义：选中轴节点 + 视窗左上角弹轴编辑器
					mCurveDocController* controller = mCurveDocController::getInstance();
					if (controller->canSelectNode(axisNodeId))
					{
						controller->setCurrentNodeId(axisNodeId);
						emit sig_requestEditorPopup(axisNodeId, mapToGlobal(QPoint(12, 12)));
					}
				});
				connect(strip, &mCurveAxisStripWidget::sig_hoverChanged, this,
					[this, strip](bool hovered)
				{
					//顶部条带挂引擎内、右侧条带挂本视图，统一映射回本视图坐标
					//（高亮覆盖层是本视图子控件）
					if (hovered)
						showZoneHighlight(QRect(strip->mapTo(this, QPoint(0, 0)), strip->size()));
					else
						hideZoneHighlight();
				});
				out.append(strip);
			}
		};
		buildStrips(secondaryAxisNodes(false), true, _topStrips);
		buildStrips(secondaryAxisNodes(true), false, _rightStrips);
	}

	void mCurveChartView::scheduleAxisStripLayout()
	{
		//0ms 去重合并：Resize 事件可能在布局激活中被同步派发（applyAxis 的
		//enableAxis 等），那时同步改布局边距/条带几何会重入布局导致崩溃；
		//统一推迟到事件循环执行
		if (_axisStripLayoutPending)
			return;
		_axisStripLayoutPending = true;
		QTimer::singleShot(0, this, [this]()
		{
			_axisStripLayoutPending = false;
			updateAxisStrips();
		});
	}

	//柱几何 resize 重算（见头文件注释）：当前图表为柱状图时逐条曲线重跑
	//applySeries——applySeries 内按"当前画布宽/类目数"现算柱宽/柱基，幂等
	//（数据与其余样式字段同源重建）；Resize 风暴经 0ms 去重合并为一轮
	void mCurveChartView::scheduleBarGeometryRefresh()
	{
		if (_barGeomPending)
			return;
		_barGeomPending = true;
		QTimer::singleShot(0, this, [this]()
		{
			_barGeomPending = false;
			refreshBarGeometry();
		});
	}

	void mCurveChartView::refreshBarGeometry()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chart = doc->node(_chartId);
		//仅柱状图消费；XY/复数/极坐标曲线无柱几何（复数走 applyComplexSeries）
		if (!chart || chart->chart.chartType != 1)
			return;
		for (int childId : doc->childNodeIds(_chartId))
		{
			CurveNode* n = doc->node(childId);
			if (n && n->type == CurveNodeType::Curve)
				applySeries(childId);
		}
	}

	void mCurveChartView::updateAxisStrips()
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		QWidget* canvas = qwt ? qwt->canvas() : nullptr;
		//边框覆盖层跟随画布几何（原在 Resize 事件内同步做；现与条带一并延迟，
		//避免布局激活中同步改画布子控件几何的重入崩溃）
		if (_borderOverlay && canvas)
			_borderOverlay->setGeometry(canvas->rect());
		//顶部条带在引擎内部布局中（页眉下方），几何由 Qt 布局自动排布，无需
		//视图参与；视图布局边距只服务右侧条带（右侧条带仍为手动几何，纵向与
		//画布对齐）。无右侧条带时边距复位（布局改动会引起重排，避免残留外扩）
		if (_rightStrips.isEmpty())
		{
			if (layout() && layout()->contentsMargins() != QMargins(0, 0, 0, 0))
			{
				layout()->setContentsMargins(0, 0, 0, 0);
				hideZoneHighlight();
			}
			return;
		}
		if (!canvas)
			return;
		int rightW = 0;
		for (const mCurveAxisStripWidget* s : _rightStrips)
			rightW += s->thickness();
		//注意参数序 left/top/right/bottom：右侧条带占的是**右边距**（曾误传成
		//左边距——引擎整体右移、左轴旁让出一条空白列，条带反而被推出视图外）
		layout()->setContentsMargins(0, 0, rightW, 0);
		//边距改完立即激活布局再取映射：布局激活默认推迟到事件循环，此刻读
		//引擎/画布位置是旧值，条带会按旧右缘摆放错位（此处不在布局激活中，
		//同步 activate 安全；Resize 路径仍经 scheduleAxisStripLayout 延迟）
		layout()->activate();
		//右侧条带几何：纵向与画布对齐；堆叠方向=由内向外（首条紧贴绘图区
		//一侧，后建的更靠外）
		const QPoint plotTopLeft = _plot->mapTo(this, QPoint(0, 0));
		const QPoint canvasTopLeft = canvas->mapTo(this, QPoint(0, 0));
		int x = plotTopLeft.x() + _plot->width();
		for (mCurveAxisStripWidget* s : _rightStrips)
		{
			s->setGeometry(x, canvasTopLeft.y(), s->thickness(), canvas->height());
			x += s->thickness();
		}
		hideZoneHighlight();   //条带几何变化，悬停高亮待下一次鼠标移动重定位
	}

	//宿主自绘层几何显式同步（公开，幂等；缺陷 20261102-B02）。
	//为什么需要：自绘层的常规同步链是"画布 Resize 事件→scheduleAxisStripLayout
	//的 0ms 去重→updateAxisStrips"，终点在事件循环里兑现。报告导出对懒物化视图
	//"materialize→grab"全程同步执行、中间不进事件循环——真实容器形态实测
	//（离屏 harness，链接本模块真实源码）：attach 物化即触发 Resize 级联
	//（视图/引擎/QwtPlot/画布 4 连发），画布已到最终几何，但挂在其后的 0ms
	//延迟同步还压在事件队列里，grab 拿到的 BorderOverlay 停留构造期几何
	//（实测 147x172 vs 画布 871x634，即"绘图区左上角小框+整图无外框"）；
	//B01 的 show+事件循环守卫对"attach 后已可见"的视图早退，等不到任何东西。
	//本入口把同步终点显式化，由导出守卫在截图前调用，不赌事件巧合：
	//  - 先激活本视图与引擎布局：隐藏态构造的视图布局激活被 Qt 推迟，显式
	//    activate 补齐引擎/画布几何（仅待激活时实际生效，已激活为廉价短路）；
	//  - 再走 updateAxisStrips 收口：边框覆盖层贴画布 + 右侧条带纵向对齐 +
	//    布局边距，与 Resize 事件链终点完全一致（顶部条带挂引擎布局，几何
	//    由上面 activate 一并排布）；
	//  - 幂等：已同步视图重复调用为同值写（setGeometry 同矩形 Qt 原生短路，
	//    不产生重绘；边距同值有分支守卫），对已显示页零干扰；
	//  - 只做几何，不碰用户缩放/激活态/焦点（017a5d0eb 守卫还原语义不变）；
	//  - 非布局激活中调用安全（导出守卫在事件循环之后/之外调用；本函数自身
	//    的 activate 与 updateAxisStrips 内部 activate 均为顺序调用，不重入）
	void mCurveChartView::syncSelfDrawnGeometry()
	{
		if (layout())
			layout()->activate();
		if (_plot && _plot->layout())
			_plot->layout()->activate();   //顶部条带所在引擎布局（页眉与绘图区之间）
		updateAxisStrips();
	}

	void mCurveChartView::clearAxisStrips()
	{
		qDeleteAll(_topStrips);
		_topStrips.clear();
		qDeleteAll(_rightStrips);
		_rightStrips.clear();
		hideZoneHighlight();
	}

namespace
{
	//右键菜单/工具栏操作图标（UI设计图03.1/04/05/10/14/15：Icon/Curve/操作/ 16px 线性 SVG）
	QIcon curveOpIcon(const char* name)
	{
		return QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/%1.svg")
			.arg(QString::fromUtf8(name)));
	}
}

void mCurveChartView::onAxisZoneContextMenu(int axisNodeId, bool vertical, const QPoint& globalPos)
{
	//关闭销毁阶段不再弹菜单（qApp 析构中仍可能触发）
	if (QCoreApplication::closingDown())
		return;
	mCurveDocController* controller = mCurveDocController::getInstance();
	CurveDocument* zoneDoc = controller->document();
	CurveNode* zoneChart = zoneDoc ? zoneDoc->node(_chartId) : nullptr;
	//复数图（设计图01）：绘图区外部的右键菜单替换为图表级菜单——复制 /
	//粘贴(置灰) / 编辑 / 切换显示格式（文案随当前格式动态）/ 互换复数坐标轴
	//（替换常规轴区菜单），右键同时选中图表节点；原型"复数右键菜单交互"
	//（2026-11-04）：三组间两条分隔线，菜单全自绘（mUiMenu，观感同全局命令
	//菜单——圆角/投影/悬停高亮；行距 25、分隔线占位 9 与原型 1:1）。宽度不
	//设固定值：mUiMenu 行宽按 UiMetrics 自算（左 2G + 图标 + 间隙 + 右
	//dropdownSlot，标准档自然宽约 158），沿用裸 QMenu 的 144 会截断最长项
	//"互换复数坐标轴"（2026-11-04 真机复现）
	if (zoneChart && zoneChart->chart.chartType == 2)
	{
		controller->setCurrentNodeId(_chartId);
		MechMainWindow::mUiMenu menu(this);
		MechMainWindow::ThemeManager::applyMenuStyle(&menu);
		QAction* copyAct = menu.addAction(curveOpIcon("复制"), QStringLiteral("复制"));
		QAction* pasteAct = menu.addAction(curveOpIcon("粘贴"), QStringLiteral("粘贴"));
		pasteAct->setEnabled(false);
		//复制：图表子树入模块节点剪贴板（可在页/工程层级粘贴为副本）
		connect(copyAct, &QAction::triggered, this, [this, controller]()
		{
			controller->copyNode(_chartId);
		});
		menu.addSeparator();   //复制/粘贴 组 | 编辑 组（原型分隔线 1）
		QAction* editAct = menu.addAction(curveOpIcon("编辑"), QStringLiteral("编辑"));
		connect(editAct, &QAction::triggered, this, [this]()
		{
			emit sig_requestEditorPopup(_chartId, mapToGlobal(QPoint(12, 12)));
		});
		menu.addSeparator();   //编辑 | 切换/互换 组（原型分隔线 2）
		const bool magPhase = zoneChart->chart.complexDisplayFormat == 0;
		//切换图标（2026-11-04 供图入库 PostCurve/qiehuan3x，原误用"编辑"占位）
		QAction* fmtAct = menu.addAction(QIcon(QStringLiteral(
			":/mMainWidgets/Icon/PostCurve/qiehuan3x.png")), magPhase
			? QStringLiteral("切换至实部 / 虚部") : QStringLiteral("切换至相位 / 幅值"));
		connect(fmtAct, &QAction::triggered, this, [this, controller]()
		{
			const int cur = controller->document()->nodeProperty(_chartId,
				CurveProp::kComplexDisplayFormat).toInt();
			controller->setNodeProperty(_chartId, CurveProp::kComplexDisplayFormat,
				cur == 0 ? 1 : 0);
		});
		//上下互换图标（供图入库 PostCurve/shangxiahuhuan3x，原误用"编辑"占位）
		QAction* swapAct = menu.addAction(QIcon(QStringLiteral(
			":/mMainWidgets/Icon/PostCurve/shangxiahuhuan3x.png")),
			QStringLiteral("互换复数坐标轴"));
		connect(swapAct, &QAction::triggered, this, [this, controller]()
		{
			controller->setNodeProperty(_chartId, CurveProp::kComplexSwapped,
				!controller->document()->nodeProperty(_chartId, CurveProp::kComplexSwapped).toBool());
		});
		menu.exec(globalPos);
		return;
	}
	//对齐参考软件（HyperGraph）轴区菜单：复制/粘贴占位、添加坐标轴（同侧
	//再叠加一枚：X 区->顶部 X2/X3...，Y 区->右侧 Y2/Y3...）、编辑（该轴
	//属性编辑器，链路同双击轴标题）
	MechMainWindow::mUiMenu menu(this);
	menu.setFixedWidth(144);   //右键菜单固定宽度
	QAction* copyAct = menu.addAction(curveOpIcon("复制"), QStringLiteral("复制"));
	QAction* pasteAct = menu.addAction(curveOpIcon("粘贴"), QStringLiteral("粘贴"));
	//轴剪贴板尚未引入（与曲线/注释侧置灰占位同策略）
	copyAct->setEnabled(false);
	pasteAct->setEnabled(false);
	QAction* addAct = menu.addAction(curveOpIcon("添加"), QStringLiteral("添加坐标轴"));
	QAction* editAct = menu.addAction(curveOpIcon("编辑"), QStringLiteral("编辑"));
		connect(addAct, &QAction::triggered, this, [this, controller, vertical]()
		{
			controller->addAxisNode(_chartId, vertical);   //走命令栈，Ctrl+Z 可撤销
		});
		connect(editAct, &QAction::triggered, this, [this, controller, axisNodeId]()
		{
			if (axisNodeId > 0 && controller->canSelectNode(axisNodeId))
			{
				//右键同时选中该轴（标题高亮/树回选/参数栏换轴页经中枢闭环）
				controller->setCurrentNodeId(axisNodeId);
				emit sig_requestEditorPopup(axisNodeId, mapToGlobal(QPoint(12, 12)));
			}
		});
		//次坐标轴（XAxis2/YAxis2，用户经"添加坐标轴"创建）：菜单最后追加删除；
		//主轴为固定子节点不提供（挂轴曲线随删除钳回主轴，Ctrl+Z 可整体恢复）
		const CurveNodeType axisType = controller->document()->nodeType(axisNodeId);
		if (axisType == CurveNodeType::XAxis2 || axisType == CurveNodeType::YAxis2)
		{
			QAction* removeAct = menu.addAction(curveOpIcon("删除"), QStringLiteral("删除坐标轴"));
			connect(removeAct, &QAction::triggered, this, [controller, axisNodeId]()
			{
				controller->removeSecondaryAxis(axisNodeId);
			});
		}
		menu.exec(globalPos);
	}

	void mCurveChartView::showZoneHighlight(const QRect& rectInView)
	{
		if (!_zoneHighlight || !rectInView.isValid())
			return;
		//去重：同一区域不重复 show/raise（MouseMove 高频触发）
		if (_zoneHighlight->isVisible() && rectInView == _zoneHighlightRect)
			return;
		_zoneHighlightRect = rectInView;
		_zoneHighlight->setGeometry(rectInView);
		_zoneHighlight->raise();
		_zoneHighlight->show();
	}

	void mCurveChartView::hideZoneHighlight()
	{
		if (_zoneHighlight && _zoneHighlight->isVisible())
		{
			_zoneHighlight->hide();
			_zoneHighlightRect = QRect();
		}
	}

	void mCurveChartView::applyAxisTitleHighlight()
	{
		if (!_plot)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* chartNode = doc->node(_chartId);
		//复数图（HyperGraph 排布）：Y_Left 引擎轴按排布归属上面板轴节点（标题
		//=面板分量名），选中态只覆盖颜色/粗细、恢复读该节点模型值；下面板标题
		//走引擎覆盖口（applyComplexLowerAxis 内按选中态着色）。不走下方通用
		//四轴循环——其"Y1=Y_Left"映射是旧排布假设，会把幅值侧行节点的选中态
		//涂到上面板的相位标题上
		if (chartNode && chartNode->chart.chartType == 2)
		{
			int topAxisId = -1, bottomAxisId = -1;
			complexPanelAxisIds(&topAxisId, &bottomAxisId);
			AxisSettings as = _plot->getAxisSettings(AxisId::Y_Left);
			if (_selectedAxisId == topAxisId)
			{
				as.titleColor = curveHighlightBlue();
				as.titleFont.setBold(true);
			}
			else
			{
				//颜色/字体从模型重读（同通用路径：引擎缓存可能残留上次高亮加粗）
				as.titleColor = doc->nodeProperty(topAxisId, CurveProp::kTitleColor).value<QColor>();
				if (!as.titleColor.isValid())
					as.titleColor = Qt::black;
				as.titleFont = doc->nodeProperty(topAxisId, CurveProp::kTitleFont).value<QFont>();
			}
			_plot->updateAxisAppearance(AxisId::Y_Left, as);
			if (bottomAxisId > 0)
				applyComplexLowerAxis(bottomAxisId);
			return;
		}
		//X/Y 主轴与首枚副轴（引擎上/右轴）逐轴应用：以引擎当前设置为基础
		//（rebuild 的 applyAxis 已按模型重置外观），只覆盖标题颜色与粗细：
		//选中=主题蓝+加粗；恢复=模型颜色（kTitleColor 无效兜底黑）+模型字体
		//（引擎缓存可能残留上次高亮的加粗，必须从模型重读）。视图状态不进模型/
		//命令栈（同曲线高亮策略）；自绘条带的选中态经 setSelected 重放
		const struct { int nodeId; AxisId engineAxis; } axes[] = {
			{ xAxisNodeId(), AxisId::X_Bottom },
			{ yAxisNodeId(), AxisId::Y_Left },
			{ nativeSecondaryAxisId(false), AxisId::X_Top },
			{ nativeSecondaryAxisId(true), AxisId::Y_Right },
		};
		for (const auto& axis : axes)
		{
			if (axis.nodeId <= 0)
				continue;

			AxisSettings as = _plot->getAxisSettings(axis.engineAxis);
			if (_selectedAxisId == axis.nodeId)
			{
				as.titleColor = curveHighlightBlue();
				as.titleFont.setBold(true);
			}
			else
			{
				//颜色从模型重读（属性编辑器"颜色"行 kTitleColor），无效兜底黑；
				//字体同样从模型重读（缓存可能残留上次高亮的加粗）
				as.titleColor = doc->nodeProperty(axis.nodeId, CurveProp::kTitleColor).value<QColor>();
				if (!as.titleColor.isValid())
					as.titleColor = Qt::black;
				as.titleFont = doc->nodeProperty(axis.nodeId, CurveProp::kTitleFont).value<QFont>();
			}
			_plot->updateAxisAppearance(axis.engineAxis, as);
		}
		//自绘副轴条带选中态（标题主题蓝+加粗，条带自绘）
		for (mCurveAxisStripWidget* strip : _topStrips)
			strip->setSelected(strip->axisNodeId() == _selectedAxisId);
		for (mCurveAxisStripWidget* strip : _rightStrips)
			strip->setSelected(strip->axisNodeId() == _selectedAxisId);
	}

	void mCurveChartView::ensureCanvasFilter()
	{
		QwtPlot* qwt = _plot ? _plot->getQwtPlot() : nullptr;
		QWidget* canvas = qwt ? qwt->canvas() : nullptr;
		if (!canvas)
			return;
		//复数下面板画布过滤（必须在下方"上面板未变化即返回"的短路之前执行）：
		//下面板是复数模式激活时才创建的，多数 ensureCanvasFilter 调用时上面板
		//早已在位，短路会错过下面板安装。QPointer 防悬挂——退出复数模式下面板
		//销毁自动置空，下一轮重装
		QwtPlot* lowerQwt = _plot ? _plot->getLowerQwtPlot() : nullptr;
		QWidget* lowerCanvas = lowerQwt ? lowerQwt->canvas() : nullptr;
		if (lowerCanvas && lowerCanvas != _filteredLowerCanvas)
		{
			//悬停高亮依赖不带按键的 MouseMove：下面板画布默认关闭鼠标追踪
			//（上面板由引擎开启），不开启则悬停事件不派发、hover 永不触发
			lowerCanvas->setMouseTracking(true);
			lowerCanvas->installEventFilter(this);
			//下面板画布右键同图表菜单（含复数"切换显示格式/互换面板"两项）：
			//坐标归一到上面板画布（onCanvasContextMenu 的命中约定）后走同一入口
			lowerCanvas->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(lowerCanvas, &QWidget::customContextMenuRequested, this,
				[this, lowerCanvas](const QPoint& p)
			{
				QwtPlot* qwt2 = _plot ? _plot->getQwtPlot() : nullptr;
				QWidget* upperCanvas = qwt2 ? qwt2->canvas() : nullptr;
				if (!upperCanvas)
					return;
				const QPoint upperPos = upperCanvas->mapFromGlobal(lowerCanvas->mapToGlobal(p));
				onCanvasContextMenu(upperPos, lowerCanvas->mapToGlobal(p));
			});
			_filteredLowerCanvas = lowerCanvas;
		}
		//下面板轴控件（左轴/下轴）同上面板轴区挂过滤器：右键弹轴区菜单（复数
		//图按 onAxisZoneContextMenu 的 chartType==2 分支整体替换为图表级复数
		//菜单）、hover 浅蓝高亮。上面板的轴控件在构造时安装，下面板轴控件随
		//复数模式激活才由引擎创建，必须在面板就位后补装（本函数自 rebuild 于
		//setComplexMode 之后调用，恰逢其时）。逐控件 QPointer 守卫（面板销毁
		//自动置空，下一轮重装），与 _filteredLowerCanvas 套路一致
		if (lowerQwt)
		{
			for (int axis : { QwtPlot::yLeft, QwtPlot::xBottom })
			{
				QWidget* scaleWgt = lowerQwt->axisWidget(axis);
				QPointer<QWidget>& guard = (axis == QwtPlot::yLeft)
					? _filteredLowerYAxis : _filteredLowerXAxis;
				if (scaleWgt && scaleWgt != guard)
				{
					//轴区 hover 高亮依赖不带按键的 MouseMove（同上面板轴区）
					scaleWgt->setMouseTracking(true);
					scaleWgt->installEventFilter(this);
					guard = scaleWgt;
				}
			}
		}
		//页眉/页脚条带控件（引擎 headerWidget()/footerWidget() 访问器）：挂事件
		//过滤器消费条带左键双击（选中 Header/Footer 节点并弹属性编辑器，同轴
		//标题先例）。条带由引擎构造期一次性创建、setUseOpenGL 只重建画布不动
		//条带——正常整个视图生命周期仅装一次，重复安装幂等（控件指针未变即跳过）。
		//须在本函数下方"画布未变即返回"的短路之前执行（同下面板控件套路）。
		//QPointer 防悬挂：视图析构随引擎销毁自动置空；条带隐藏（不可见/无文本）
		//时 Qt 不派发鼠标事件，双击天然无响应
		if (_plot)
		{
			QWidget* headerWgt = _plot->headerWidget();
			if (headerWgt && headerWgt != _headerBandWgt)
			{
				headerWgt->installEventFilter(this);
				_headerBandWgt = headerWgt;
			}
			QWidget* footerWgt = _plot->footerWidget();
			if (footerWgt && footerWgt != _footerBandWgt)
			{
				footerWgt->installEventFilter(this);
				_footerBandWgt = footerWgt;
			}
		}
		//画布未变且覆盖层在位：无需处理（QPointer 随旧画布销毁自动置空）
		const bool canvasChanged = (canvas != _filteredCanvas);
		if (!canvasChanged && _borderOverlay)
			return;
		if (canvasChanged)
		{
			//后装先执行：本过滤器先于引擎的过滤器收到画布事件
			//（引擎在其构造函数 initPlotStyle 里已安装自己的过滤器）
			canvas->installEventFilter(this);
			_filteredCanvas = canvas;
		}
		//闭合边框覆盖层：铺满画布绘制在最上层，尺寸跟随画布（Resize 分支同步）。
		//覆盖层必须是画布的子控件；画布被引擎重建（setUseOpenGL）时旧覆盖层随旧
		//画布销毁（QPointer 自动置空），此处/下一轮在新画布上重建——覆盖层还挂在
		//旧画布上（未销毁）时也先在新画布重建一份，旧实例随旧画布回收
		_borderOverlay = new BorderOverlay(canvas);
		_borderOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
		_borderOverlay->setAttribute(Qt::WA_NoSystemBackground);
		_borderOverlay->setGeometry(canvas->rect());
		_borderOverlay->raise();
	}

void mCurveChartView::onCanvasContextMenu(const QPoint& pos, const QPoint& globalPos)
{
	//关闭销毁阶段不再弹菜单（qApp 析构中仍可能触发）
	if (QCoreApplication::closingDown())
		return;

	//注释/基准线模式内右键不弹菜单：模式是创建态（左键打点、Esc 退出），注释/
	//基准线等编辑菜单仅在按钮弹起（模式退出）后开放，与模式内双击吞掉同策略
	if (_annotationMode || _baselineMode)
		return;

	mCurveDocController* controller = mCurveDocController::getInstance();

	//注释命中（设计图15）：注释菜单与曲线菜单一致（剪切/复制/粘贴/编辑/
	//删除，同图标与分隔线结构），"编辑"弹出注释属性编辑器（弹窗专属页面族，
	//同曲线侧链路）；右键同时选中该注释（主题蓝选中框/树联动经选择中枢闭环）
	const int annotationId = hitAnnotation(pos);
	if (annotationId > 0)
	{
		//右键同时选中该注释（主题蓝选中框/树联动经选择中枢闭环）；
		//数据隔离：作用域外类型只出菜单不选中
		if (controller->canSelectNode(annotationId))
			controller->setCurrentNodeId(annotationId);

		MechMainWindow::mUiMenu menu(this);
		menu.setFixedWidth(144);   //右键菜单固定宽度
		QAction* cutAct = menu.addAction(curveOpIcon("剪切"), QStringLiteral("剪切"));
		QAction* copyAct = menu.addAction(curveOpIcon("复制"), QStringLiteral("复制"));
		QAction* pasteAct = menu.addAction(curveOpIcon("粘贴"), QStringLiteral("粘贴"));
		//模块节点剪贴板（与曲线侧同链路）：剪切=复制+删除（可撤销）；粘贴=
		//剪贴板子树粘进本图表（新 id），可撤销；剪贴板为空时粘贴置灰
		pasteAct->setEnabled(controller->hasClipboard());
		menu.addSeparator();
		QAction* editAct = menu.addAction(curveOpIcon("编辑"), QStringLiteral("编辑"));
		menu.addSeparator();
		QAction* removeAct = menu.addAction(curveOpIcon("删除"), QStringLiteral("删除"));

		const int pasteTargetChart = controller->document()->chartNodeId(annotationId);
		connect(cutAct, &QAction::triggered, this, [controller, annotationId]()
		{
			controller->cutNode(annotationId);
		});
		connect(copyAct, &QAction::triggered, this, [controller, annotationId]()
		{
			controller->copyNode(annotationId);
		});
		connect(pasteAct, &QAction::triggered, this, [controller, pasteTargetChart]()
		{
			controller->pasteNode(pasteTargetChart);
		});
		connect(editAct, &QAction::triggered, this, [this, annotationId]()
		{
			emit sig_requestEditorPopup(annotationId, mapToGlobal(QPoint(12, 12)));
		});
		connect(removeAct, &QAction::triggered, this, [controller, annotationId]()
		{
			controller->removeNode(annotationId);   //走命令栈，可 Ctrl+Z 撤销
		});
		menu.exec(globalPos);
		return;
	}

	//基准线命中（设计图14，图5）：基准线菜单（剪切/复制/粘贴置灰占位/编辑/
	//删除，结构与曲线菜单一致），"编辑"弹出基准线属性编辑器（视窗左上角，
	//同曲线侧链路）；右键同时选中该基准线（树/参数栏经选择中枢闭环）
	const int baselineId = hitBaseline(pos);
	if (baselineId > 0)
	{
		//数据隔离：作用域外类型只出菜单不选中
		if (controller->canSelectNode(baselineId))
			controller->setCurrentNodeId(baselineId);

		MechMainWindow::mUiMenu menu(this);
		menu.setFixedWidth(144);   //右键菜单固定宽度
		QAction* cutAct = menu.addAction(curveOpIcon("剪切"), QStringLiteral("剪切"));
		QAction* copyAct = menu.addAction(curveOpIcon("复制"), QStringLiteral("复制"));
		QAction* pasteAct = menu.addAction(curveOpIcon("粘贴"), QStringLiteral("粘贴"));
		//模块节点剪贴板（与曲线侧同链路）：剪切=复制+删除（可撤销）；粘贴=
		//剪贴板子树粘进本图表（新 id，同名自动加序号后缀），可撤销
		pasteAct->setEnabled(controller->hasClipboard());
		menu.addSeparator();
		QAction* editAct = menu.addAction(curveOpIcon("编辑"), QStringLiteral("编辑"));
		menu.addSeparator();
		QAction* removeAct = menu.addAction(curveOpIcon("删除"), QStringLiteral("删除"));
		const int pasteTargetChart = controller->document()->chartNodeId(baselineId);
		connect(cutAct, &QAction::triggered, this, [controller, baselineId]()
		{
			controller->cutNode(baselineId);
		});
		connect(copyAct, &QAction::triggered, this, [controller, baselineId]()
		{
			controller->copyNode(baselineId);
		});
		connect(pasteAct, &QAction::triggered, this, [controller, pasteTargetChart]()
		{
			controller->pasteNode(pasteTargetChart);
		});
		connect(editAct, &QAction::triggered, this, [this, baselineId]()
		{
			emit sig_requestEditorPopup(baselineId, mapToGlobal(QPoint(12, 12)));
		});
		connect(removeAct, &QAction::triggered, this, [controller, baselineId]()
		{
			controller->removeNode(baselineId);   //走命令栈，可 Ctrl+Z 撤销
		});
		menu.exec(globalPos);
		return;
	}

	//图例空白区域（非图例项，设计图08）：图例菜单（仅"编辑"）；右键同时选中
	//图例节点（主题蓝外框/树联动经选择中枢闭环）。命中图例项则落到下方曲线菜单
	{
		QString name;
		if (_plot->getPlotLegendItem()->hitTest(pos)
			&& !_plot->getPlotLegendItem()->hitTestItem(pos, name))
		{
			const int legendId = legendNodeId();
			if (legendId > 0)
			{
				//数据隔离：作用域外类型只出菜单不选中
				if (controller->canSelectNode(legendId))
					controller->setCurrentNodeId(legendId);

				MechMainWindow::mUiMenu menu(this);
				menu.setFixedWidth(144);   //右键菜单固定宽度
				QAction* editAct = menu.addAction(curveOpIcon("编辑"), QStringLiteral("编辑"));
				connect(editAct, &QAction::triggered, this, [this, legendId]()
				{
					emit sig_requestEditorPopup(legendId, mapToGlobal(QPoint(12, 12)));
				});
				menu.exec(globalPos);
			}
			return;
		}
	}

	//命中判定：图例项优先（图例悬浮绘制于画布之上），其次曲线拾取。右键拾取
	//半径放宽到 20px（悬停仍 10px）：陡峭峰线/粗线边缘"视觉在线上"的点击
	//按 10px 常命中不了，会落成图表空白菜单（复数下面板相位曲线尤甚）
	QString name;
	int curveId = -1;
	if (_plot->getPlotLegendItem()->hitTestItem(pos, name))
		curveId = engineNodeByKey(name);   //图例项身份=引擎键，映射反查节点（R23）
	else if (hitCurveAnyPanel(pos, name, 20.0))
		curveId = engineNodeByKey(name);
	if (curveId <= 0)
	{
		//图表空白区域（非注释/图例/曲线）：图表菜单，对齐参考软件——
		//粘贴(置灰占位)/编辑/应用样式(置灰占位)；右键同时选中图表节点
		//（树回选/参数栏换页经选择中枢闭环）。
		//复数"切换显示格式/互换面板"两项按设计图01在绘图区外部（轴区）菜单，
		//不在绘图区内（见 onAxisZoneContextMenu 复数分支）
		controller->setCurrentNodeId(_chartId);

		MechMainWindow::mUiMenu menu(this);
		menu.setFixedWidth(144);   //右键菜单固定宽度
		QAction* pasteAct = menu.addAction(curveOpIcon("粘贴"), QStringLiteral("粘贴"));
		menu.addSeparator();
		QAction* editAct = menu.addAction(curveOpIcon("编辑"), QStringLiteral("编辑"));
		QAction* applyStyleAct = menu.addAction(curveOpIcon("应用样式"), QStringLiteral("应用样式"));
		//粘贴模块节点剪贴板内容到本图表（剪切曲线后在此处粘贴即移动到本图）；
		//剪贴板为空置灰（剪贴板类型不容纳时 pasteNode 安全返回 -1）
		pasteAct->setEnabled(controller->hasClipboard());
		connect(pasteAct, &QAction::triggered, this, [this, controller]()
		{
			controller->pasteNode(_chartId);
		});
		//应用样式：把样式按勾选类别批量复制到其他页/图表，
		//对话框为模块内单例（mApplyStyleDlg），非模态可反复"应用"
		connect(applyStyleAct, &QAction::triggered, this, [this]()
		{
			mApplyStyleDlg::showForChart(_chartId, window());
		});
		connect(editAct, &QAction::triggered, this, [this]()
		{
			emit sig_requestEditorPopup(_chartId, mapToGlobal(QPoint(12, 12)));
		});
		menu.exec(globalPos);
		return;
	}

	//右键同时选中该曲线（菜单动作作用于它，树/参数栏随中枢联动）
	selectCurve(curveId);

	MechMainWindow::mUiMenu menu(this);
	menu.setFixedWidth(144);   //右键菜单固定宽度
	QAction* cutAct = menu.addAction(curveOpIcon("剪切"), QStringLiteral("剪切"));
	QAction* copyAct = menu.addAction(curveOpIcon("复制"), QStringLiteral("复制"));
	QAction* pasteAct = menu.addAction(curveOpIcon("粘贴"), QStringLiteral("粘贴"));
	//模块节点剪贴板：剪切=复制+删除（删除走命令栈可撤销）；粘贴=剪贴板子树
	//粘进本图表（新 id，同名自动加" (N)"序号后缀），走命令栈可撤销；
	//剪贴板为空时粘贴置灰
	pasteAct->setEnabled(controller->hasClipboard());
	menu.addSeparator();
	QAction* editAct = menu.addAction(curveOpIcon("编辑"), QStringLiteral("编辑"));
	menu.addSeparator();
	QAction* removeAct = menu.addAction(curveOpIcon("删除"), QStringLiteral("删除"));

	const int pasteTargetChart = controller->document()->chartNodeId(curveId);
	connect(cutAct, &QAction::triggered, this, [controller, curveId]()
	{
		controller->cutNode(curveId);      //复制入剪贴板 + 删除（可撤销）
	});
	connect(copyAct, &QAction::triggered, this, [controller, curveId]()
	{
		controller->copyNode(curveId);     //子树快照入剪贴板，文档不变
	});
	connect(pasteAct, &QAction::triggered, this, [controller, pasteTargetChart]()
	{
		controller->pasteNode(pasteTargetChart);   //粘进本图表（可撤销）
	});
	connect(editAct, &QAction::triggered, this, [this, curveId]()
	{
		emit sig_requestEditorPopup(curveId, mapToGlobal(QPoint(12, 12)));
	});
	connect(removeAct, &QAction::triggered, this, [controller, curveId]()
	{
		controller->removeNode(curveId);   //走命令栈，可 Ctrl+Z 撤销
	});
	menu.exec(globalPos);
}
}
