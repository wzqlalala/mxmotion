#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 副坐标轴条带（自绘；多坐标轴功能中第 2 枚起的顶部/右侧副轴）
//
// 引擎（PlotView/Qwt）只有 4 根原生轴（下X1/左Y1/上X2/右Y2）。多坐标轴功能里，
// 每侧首枚副轴映射引擎 xTop/yRight 原生轴；再往外的副轴（X3..Xn / Y3..Yn）由
// 本控件绘制：一条带刻度/数字/标题的条带。顶部条带插入 mPlotView 内部布局
// （页眉与 QwtPlot 之间——页眉在引擎顶部，条带放引擎之外会跑到页眉上方）；
// 右侧条带由 mCurveChartView 放在为其外扩的视图布局边距内，纵向与画布对齐；
// 两种摆法的横向都与画布严格对齐（刻度逐像素对位，几何/对位见视图与 paintEvent）。
// 条带自绘背景（图表背景色，与引擎 setMarginBackground 同源），不透宿主底色。
//
// 范围取模型：手动范围按刻度模式变换（与主轴同为"数据预变换+线性轴"策略）；
// 自动范围镜像同侧引擎轴的当前范围（首枚副轴 fit 后的值，含用户缩放）。
// 刻度经 QwtLinearScaleEngine 生成，数字格式与主轴一致（'g'4 位有效数字，
// mCurveScaleDraw 定义在本文件，与 mCurveChartView 引擎侧安装共用）。
//
// 交互转发宿主视图：右键=轴区菜单（复制/粘贴/添加坐标轴/编辑）、双击=轴属性
// 编辑器、悬停=轴区高亮；选中态（标题主题蓝+加粗）由视图经 setSelected 下发。
// =============================================================================

#include "../PostCurveGlobal.h"

#include <QWidget>
#include <QColor>
#include <QFont>
#include <QString>
#include <cmath>

#include "mPlotStructs.h"
#include <qwt_scale_draw.h>
#include <qwt_text.h>

class QwtPlot;

namespace PostCurve
{
	//刻度数字格式化："自动"为 'g' 格式，有效数字位数取"精度"（kPrecision，
	//模型默认 3；历史行为恒为 4 位，现与刻度设置联动）；"科学计数法/定点
	//格式"按所选格式与"精度"输出（坐标轴刻度设置 kLabelFormat/kPrecision，
	//applyAxis/条带回显时下发）。
	//必须继承 PlotView::SafeScaleDraw（而非直接继承 QwtScaleDraw）：
	//PlotView 内部按 SafeScaleDraw 访问刻度标志，替换成其他类型会破坏其向下转型。
	//定义于此处共用：mCurveChartView（引擎四轴安装）与副轴条带（自绘刻度）同一格式
	class mCurveScaleDraw : public PlotView::SafeScaleDraw
	{
	public:
		mCurveScaleDraw() = default;

		//刻度数字格式（kLabelFormat：0自动/1科学计数法/2定点格式）与精度
		//(kPrecision)：自动='g'+精度（有效数字位数），科学计数法/定点按精度
		//输出小数位。必须 invalidateCache()：QwtScaleDraw::tickLabel 按刻度值
		//缓存已渲染文本，同值刻度（格式/精度切换不改刻度位置）会命中旧格式
		//缓存而不重跑 label
		void setNumberFormat(int format, int precision)
		{
			_format = format;
			_precision = qBound(0, precision, 15);
			invalidateCache();
		}

		//类别刻度名（柱状图 X 轴，数值定义曲线写入）：非空时整数刻度位按名
		//渲染（1月/3月…），非整数位与越界回退数字（缩放产生的中间刻度）；
		//清空恢复纯数字刻度。同样须 invalidateCache 防旧文本缓存
		void setTickLabels(const QStringList& labels)
		{
			_tickLabels = labels;
			invalidateCache();
		}

		//对数轴标签还原（数据预变换架构）：刻度位置为 log10 工程值，开启后
		//label 按 10^v 还原为工程值再格式化（对齐 HyperGraph/设计稿 1.0E+01
		//形态）。须 invalidateCache 防旧文本缓存
		void setLogValueLabels(bool on)
		{
			if (_logValues == on)
				return;
			_logValues = on;
			invalidateCache();
		}

	protected:
		QwtText label(double v) const override
		{
			//对数轴标签还原（数据预变换架构）：刻度位置为 log10 工程值，按
			//10^v 还原为工程值科学计数呈现（对齐原型 1.0E-04 形态）。负十倍程
			//同样还原——旧 v > 0.0 正值守卫把 NVH 量级（<1）的全负刻度挡回
			//'g' 裸值格式（nvh-log-scale-prototype-align 2026-11-16 实锤修正；
			//正值守卫系 10-28 机制落地时消费场景数据 >1 暴露不了的残留）
			if (_logValues && qIsFinite(v))
				return QwtText(QString::number(std::pow(10.0, v), 'e', 1).toUpper());
			if (!_tickLabels.isEmpty())
			{
				const int idx = qRound(v);
				if (idx >= 0 && idx < _tickLabels.size() && qAbs(v - idx) < 1e-9)
					return QwtText(_tickLabels.at(idx));
			}
			if (_format == 1)
				return QwtText(QString::number(v, 'e', _precision));
			if (_format == 2)
				return QwtText(QString::number(v, 'f', _precision));
			return QwtText(QString::number(v, 'g', qMax(1, _precision)));   //自动：'g'+精度（有效数字位数）
		}

	private:
		bool _logValues = false;   //对数轴标签还原（10^v 工程值显示）
		int _format = 0;      //0自动/1科学计数法/2定点格式
		int _precision = 3;   //小数位（科学计数法/定点格式）
		QStringList _tickLabels;   //类别刻度名（空=数字刻度）
	};

	//副坐标轴条带控件（详见文件头注释）
	class POSTCURVE_EXPORT mCurveAxisStripWidget : public QWidget
	{
		Q_OBJECT
	public:
		//top=true 顶部横轴条带（刻度朝上/向外），false 右侧纵轴条带（刻度朝右/向外）
		explicit mCurveAxisStripWidget(bool top, QWidget* parent = nullptr);

		//按模型配置条带（节点须为 XAxis2/YAxis2 类型）。qwt 用于自动范围时镜像
		//同侧引擎轴（xTop/yRight）当前范围；调用后 thickness() 即为最终厚度
		void setAxis(int axisNodeId, QwtPlot* qwt);

		//视图范围实时跟随（用户平移/缩放后由 mCurveChartView 调用）：自动范围
		//时重镜像同侧引擎轴当前范围并重绘；手动范围重算结果不变，无副作用
		void refreshScale();
		int axisNodeId() const { return _axisNodeId; }

		//条带厚度（像素）：顶部条=高度，右侧条=宽度（刻度 extent+标题+边距）
		int thickness() const { return _thickness; }

		//选中态：标题主题蓝+加粗（视图状态不进模型，同主轴标题激活策略）
		void setSelected(bool selected);

	signals:
		//右键请求轴区菜单（globalPos 为菜单弹出点）
		void sig_contextMenuRequested(const QPoint& globalPos);
		//双击请求轴属性编辑器（视窗左上角弹窗，同主轴双击标题链路）
		void sig_doubleClicked(const QPoint& globalPos);
		//悬停进入/离开（宿主显示/隐藏轴区高亮覆盖层）
		void sig_hoverChanged(bool hovered);

	protected:
		void paintEvent(QPaintEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void leaveEvent(QEvent* event) override;

	private:
		//按模型（手动范围）或同侧引擎轴镜像（自动范围）生成刻度分段并计算条带厚度
		void rebuildScale(QwtPlot* qwt);

	bool _top;                        //true 顶部横轴 / false 右侧纵轴
	int _axisNodeId = -1;             //对应轴节点id
	int _thickness = 0;               //条带厚度（px；顶部=高度/右侧=宽度）
	bool _selected = false;           //选中（标题主题蓝+加粗）
	QString _title;                   //轴标题（名称不可见时为空）
	QFont _titleFont;                 //标题字体
	QFont _labelFont;                 //刻度数字字体
	QColor _titleColor;               //标题颜色
	QColor _bgColor{Qt::white};       //条带背景（图表背景色，与引擎边距区同色）
	QwtPlot* _qwt = nullptr;          //引擎图：自动范围镜像 + 顶部条带画布横向对位
	mCurveScaleDraw _scaleDraw;       //刻度绘制器（'g'4 数字格式）
	};
}
