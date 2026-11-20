#include "m3DChartView.h"
#include "m3DChartLegendOverlay.h"
#include "m3DChartAnnotationItem.h"
#include "mCurveChartView.h"   //transformValue/inverseTransformValue 公开静态（R43 刻度预变换单一实现，禁止复制公式）
#include "mCurveViewportFactory.h"   //sig_annotation3DModeChanged（R46 三维注释模式广播承接）
#include "../CurveCore/mCurveDocController.h"
#include "../CurveCore/CurveDocument.h"

//Plot3D（3D绘图引擎，仅 PostCurve 模块内消费；严格调用规范见类头注释）
#include <Qt3DPlotWidget.h>
#include <Axis3D.h>
#include <Plot3DScene.h>
#include <Surface3DSeries.h>
#include <WaterfallSeries.h>
#include <Line3DSeries.h>
#include <Bar3DSeries.h>
#include <DatumPlane3D.h>

//QT
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QPolygonF>
#include <QUndoStack>
#include <algorithm>
#include <cmath>
#include <exception>
#include <vector>

namespace PostCurve
{
    namespace
    {
        //注释框拾取吸附半径（像素，R46）：三维注释模式内点击创建的最近采样点
        //判定阈值，与二维注释 snapper 同值（mCurveChartView kAnnotationSnapPx
        //同口径，禁止两处分叉的含义对齐）
        const double kAnno3DSnapPx = 30.0;
        //单击判定阈值（px，曼哈顿距离）：按下到松开未超阈值=单击创建，超过=
        //拖拽（模式内画布左键被消费不进引擎，拖拽无相机语义，不误创建）
        const int kAnno3DClickPx = 4;

        //寻点标记覆盖层（R47）：宿主视图子控件（非画布子控件——rebuild 重建
        //画布不受影响，R32 图例覆盖层/R42 注释项先例），鼠标穿透叠于 GL 画布
        //之上（相机旋转/缩放零影响）。内容=橙点（半径 4px，1px 深橙描边）+
        //坐标文本 "X:%.3f Y:%.3f Z:%.3f"（点右侧 8px 垂直居中，深灰）。
        //宿主 repositionSeekMarker 算好画布锚点像素与文本后 placeAt 重摆
        //（本项几何=点∪文本包围盒含余量，锚点换算为项内局部坐标），paintEvent
        //自绘；隐藏由宿主控制（placeAt 后 show）
        class SeekMarkerOverlay : public QWidget
        {
        public:
            explicit SeekMarkerOverlay(QWidget* host)
                : QWidget(host)
            {
                //鼠标穿透：标记区域下拖拽仍旋转视角（相机交互零影响）；
                //背身透明不挡 GL 画布
                setAttribute(Qt::WA_TransparentForMouseEvents);
                setAttribute(Qt::WA_NoSystemBackground);
                hide();
            }
            //重摆：canvasPx=画布像素（画布与宿主视图同矩形 0 边距，坐标系
            //一致）；text=坐标文本（宿主按文档现值格式化）
            void placeAt(const QPointF& canvasPx, const QString& text)
            {
                _text = text;
                //几何=橙点∪文本包围盒（各留 2px 笔画余量；文本测宽上界给足
                //防截断）
                const QFontMetrics fm(font());
                const QRect textRect = fm.boundingRect(
                    QRect(0, 0, 4096, fm.height() + 4),
                    Qt::AlignLeft | Qt::AlignVCenter, text);
                const QPointF textTopLeft(canvasPx.x() + 8.0,
                    canvasPx.y() - textRect.height() / 2.0);
                QRectF bounds(canvasPx.x() - 6.0, canvasPx.y() - 6.0,
                    12.0, 12.0);   //橙点半径 4px + 描边/抗锯齿余量 2px
                bounds = bounds.united(QRectF(textTopLeft,
                    QSizeF(textRect.width() + 2.0, textRect.height() + 2.0)));
                _anchorLocal = canvasPx - bounds.topLeft();
                _textTopLeft = textTopLeft - bounds.topLeft();
                setGeometry(bounds.toRect());
                show();
                raise();   //叠画布与其他覆盖层之上（寻点反馈最显眼）
                update();
            }
        protected:
            void paintEvent(QPaintEvent*) override
            {
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing, true);
                //绘制坐标系=本项局部坐标（setGeometry 后原点=包围盒左上角，非
                //画布原点）：橙点用 _anchorLocal（画布锚点-包围盒左上角），文本
                //_textTopLeft 同为局部坐标（placeAt 已换算）
                p.setBrush(QColor(255, 140, 0));
                p.setPen(QPen(QColor(204, 102, 0), 1.0));
                p.drawEllipse(_anchorLocal, 4.0, 4.0);
                //坐标文本（深灰 #333 同参数栏页口径）
                p.setPen(QColor(51, 51, 51));
                p.drawText(_textTopLeft + QPointF(0, fontMetrics().ascent()), _text);
            }
        private:
            QPointF _anchorLocal;    //锚点在本项内的局部坐标（paint 用）
            QPointF _textTopLeft;    //文本左上角局部坐标
            QString _text;
        };

        //吸附高亮标记覆盖层（R46 问题4，注释模式内 hover 反馈）：宿主视图子
        //控件（非画布子控件——rebuild 重建画布不受影响，寻点标记先例），鼠标
        //穿透叠于 GL 画布之上。画法=空心圆环（半径 6px，主题蓝 2px 描边，对齐
        //二维注释悬停预览蓝色系 QColor(43,125,233)）+中心 2px 实心点——吸附候
        //选点提示，提示用户当前落点可打点。宿主 updateSnapMarker 算好投影像素
        //后 placeAt 重摆
        class SnapMarkerOverlay : public QWidget
        {
        public:
            explicit SnapMarkerOverlay(QWidget* host)
                : QWidget(host)
            {
                setAttribute(Qt::WA_TransparentForMouseEvents);
                setAttribute(Qt::WA_NoSystemBackground);
                hide();
            }
            void placeAt(const QPointF& canvasPx)
            {
                //几何=圆环外接方（半径 6px + 描边/抗锯齿余量 2px）；圆环中心
                //恒在几何中心 (8,8)——paintEvent 按本项局部坐标绘制
                setGeometry(int(canvasPx.x()) - 8, int(canvasPx.y()) - 8, 16, 16);
                show();
                raise();
                update();
            }
        protected:
            void paintEvent(QPaintEvent*) override
            {
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing, true);
                QPen pen(QColor(43, 125, 233));
                pen.setWidthF(2.0);
                p.setPen(pen);
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(QPointF(8.0, 8.0), 6.0, 6.0);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(43, 125, 233));
                p.drawEllipse(QPointF(8.0, 8.0), 2.0, 2.0);
            }
        };

        //QVector<double> -> std::vector<float>（Plot3D Series API 消费口径；
        //模型保 double 精度，渲染入口转 float）
        std::vector<float> toFloats(const QVector<double>& values)
        {
            std::vector<float> out;
            out.reserve(values.size());
            for (const double v : values)
                out.push_back(static_cast<float>(v));
            return out;
        }

        //唯一值序列的平均点距（柱底宽估算：矩阵口径逐单元 xs/ys 大量重复值，
        //不去重会把步距算成 0 而退化兜底；点列口径唯一化后与全量口径一致——
        //T2 弹窗两口径在此统一为唯一值口径）
        double meanStepUnique(QVector<double> values)
        {
            if (values.size() < 2)
                return 0.0;
            std::sort(values.begin(), values.end());
            const auto last = std::unique(values.begin(), values.end());
            const int n = static_cast<int>(last - values.begin());
            if (n < 2)
                return 0.0;
            double sum = 0.0;
            for (int i = 1; i < n; ++i)
                sum += values.at(i) - values.at(i - 1);
            return sum / static_cast<double>(n - 1);
        }

        //三轴等长向量指针组逐轴并集核心（统计口径单一真源：unionAxisRange
        //原始路径与 R43 刻度预变换路径共用，禁止两处分叉）。任一分量非有限
        //的点不参与统计（种子防护优先于取种）；退化轴（|max-min|<1e-4）统一
        //max+=1；全点非有限返回 false
        bool unionAxisCore(const QList<const QVector<double>*>& xs,
            const QList<const QVector<double>*>& ys,
            const QList<const QVector<double>*>& zs,
            double& minX, double& maxX, double& minY, double& maxY,
            double& minZ, double& maxZ)
        {
            bool any = false;
            minX = minY = minZ = 0.0;
            maxX = maxY = maxZ = 0.0;
            const int nCurves = xs.size();
            for (int k = 0; k < nCurves; ++k)
            {
                const QVector<double>& X = *xs.at(k);
                const QVector<double>& Y = *ys.at(k);
                const QVector<double>& Z = *zs.at(k);
                const int nPts = qMin(X.size(), qMin(Y.size(), Z.size()));
                for (int i = 0; i < nPts; ++i)
                {
                    const double x = X.at(i);
                    const double y = Y.at(i);
                    const double z = Z.at(i);
                    if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(z))
                        continue;   //非有限点不参与统计（种子防护优先于取种）
                    if (!any)
                    {
                        any = true;
                        minX = maxX = x;
                        minY = maxY = y;
                        minZ = maxZ = z;
                        continue;
                    }
                    minX = qMin(minX, x); maxX = qMax(maxX, x);
                    minY = qMin(minY, y); maxY = qMax(maxY, y);
                    minZ = qMin(minZ, z); maxZ = qMax(maxZ, z);
                }
            }
            if (any)
            {
                if (std::abs(maxX - minX) < 1e-4) maxX += 1.0;
                if (std::abs(maxY - minY) < 1e-4) maxY += 1.0;
                if (std::abs(maxZ - minZ) < 1e-4) maxZ += 1.0;
            }
            return any;
        }

        //全部曲线逐轴数据并集（autoFit 范围口径）；退化轴（|max-min|<1e-4）
        //统一 max+=1（Line/Surface/Bar 序列 setData 同款兜底；Waterfall 无兜底
        //仅着色器防护，统一兜底对观感无差），保证归一化分母恒正。
        //任一分量非有限的点不参与统计（R40：含首点种子——NaN/±inf 种子经
        //qMin/qMax 污染整个并集，autoFit/色带 z 范围随之失效；文件矩阵空单元格
        //NaN、数学断口 NaN 同此防御）；全点非有限返回 false（=查询失败，走
        //既有"无数据"路径，rebuild 侧轴组按模型范围兜底）。
        //返回是否取到任何数据点（rebuild 忽略返回值；queryAxis3DDisplayRange
        //以它判定"无曲线/无数据"=查询失败）——并集口径单一实现，两处共用
        bool unionAxisRange(const QList<CurveNode*>& curves,
            double& minX, double& maxX, double& minY, double& maxY,
            double& minZ, double& maxZ)
        {
            //指针组收集（QVector 隐式共享仅取地址零拷贝），统计进核心单一实现
            QList<const QVector<double>*> xs, ys, zs;
            xs.reserve(curves.size());
            ys.reserve(curves.size());
            zs.reserve(curves.size());
            for (CurveNode* c : curves)
            {
                xs.append(&c->series.xData);
                ys.append(&c->series.yData);
                zs.append(&c->series.zData);
            }
            return unionAxisCore(xs, ys, zs, minX, maxX, minY, maxY, minZ, maxZ);
        }

        //--- R43 三维轴刻度类型（宿主侧数据预变换+线性轴策略，引擎零改动） ---

        //本图表逐轴刻度类型（0 Linear/1 log10/2 dB20/3 dB10，qBound 防越界；
        //图表无效/非三维图保持全 0=线性恒等）。投影/反投影等无 chart 上下文
        //的消费点现读文档（一次 id 反查+三 int，界面线程可负担）
        void axis3dScaleTypes(int chartId, int st[3])
        {
            st[0] = st[1] = st[2] = 0;
            CurveDocument* doc = mCurveDocController::getInstance()->document();
            CurveNode* chart = doc ? doc->node(chartId) : nullptr;
            if (!chart || chart->type != CurveNodeType::Chart
                || chart->chart.chartType != 5)
                return;
            for (int i = 0; i < 3; ++i)
                st[i] = qBound(0, chart->chart.axis3d.axes[i].scaleType, 3);
        }

        //单轴逐点变换副本（R43）：复用 mCurveChartView::transformValue 公开
        //静态（对齐 NVHCurveWgt 语义：0 原值/1 log10/2 dB20/3 dB10；dbRef 恒
        //1.0 纯 dB——三维页无参考值行，用户拍板）。变换返回 NaN（原始值 ≤0
        //或原 NaN）该分量置 NaN——对齐二维"非法点剔除"断点语义。线性轴
        //（mode==0）浅共享零拷贝=快路径零回归。模型不动（副本仅 setData/统计）
        QVector<double> transformAxisCopy(const QVector<double>& values, int mode)
        {
            if (mode == 0)
                return values;
            QVector<double> out;
            out.reserve(values.size());
            for (const double v : values)
                out.append(mCurveChartView::transformValue(v, mode, 1.0));
            return out;
        }

        //R43 逐轴生效范围（变换空间；rebuild 轴组下发与显示范围缓存镜像同一
        //实现）：autoFit=变换空间并集 / 手动=模型值按轴刻度变换。变换 NaN
        //（模型值 ≤0 于 log/dB 轴——合法休眠值或脏流）单侧兜底=该轴变换后
        //并集对应侧；并集不可得=线性缺省 0/10 再退化兜底（比二维条带 0..10
        //兜底更优，ReadMe 备案差异点）；退化轴 hi-lo<1e-4 → hi=lo+1（变换
        //空间，与 unionAxisCore 兜底同口径）
        void effectiveAxisRanges(const CurveAxis3DState* axes, const int st[3],
            const double uTMin[3], const double uTMax[3], bool uAny,
            double outMin[3], double outMax[3])
        {
            for (int i = 0; i < 3; ++i)
            {
                double lo = axes[i].autoFit ? uTMin[i]
                    : mCurveChartView::transformValue(axes[i].min, st[i], 1.0);
                double hi = axes[i].autoFit ? uTMax[i]
                    : mCurveChartView::transformValue(axes[i].max, st[i], 1.0);
                if (!qIsFinite(lo))
                    lo = uAny ? uTMin[i] : 0.0;
                if (!qIsFinite(hi))
                    hi = uAny ? uTMax[i] : 10.0;
                if (hi - lo < 1e-4)
                    hi = lo + 1.0;
                outMin[i] = lo;
                outMax[i] = hi;
            }
        }

        //逐曲线渲染向量（R43）：刻度轴=逐点变换副本；线性轴=浅共享原数据
        struct Axis3DRenderVecs
        {
            QVector<double> x, y, z;
        };

        //可见曲线收集前置声明（定义在本匿名 namespace 尾部——axis3dRenderData
        //先于其定义使用，实现同文件单一来源）
        QList<CurveNode*> collectRenderCurves(CurveDocument* doc, int chartId);

        //R43 轴渲染数据单一实现（rebuild 系列下发/轴组范围与
        //refreshDisplayRangeCache 缓存镜像共用，禁止两处分叉）：读逐轴刻度 →
        //逐曲线变换副本（仅非 0 轴变换，线性轴浅共享零拷贝）→ 变换空间并集
        //（对副本跑 unionAxisCore——非有限整点排除先例自动生效[变换 NaN 点
        //不入统计]，退化兜底 max+=1 在变换空间）→ 生效范围（effectiveAxisRanges
        //单一实现）。outVecs 非空时回传逐曲线渲染向量（与 collectRenderCurves
        //同序；线性路径回传空表=调用方直接用原数据）。返回=是否取到存活数据
        //点（仅语义标记——出参恒有效：无数据 uT=0、eff=退化兜底，rebuild 照常
        //消费、缓存恒镜像）
        bool axis3dRenderData(int chartId, int st[3],
            QList<Axis3DRenderVecs>* outVecs,
            double uTMin[3], double uTMax[3],
            double effMin[3], double effMax[3])
        {
            st[0] = st[1] = st[2] = 0;
            for (int i = 0; i < 3; ++i)
            {
                uTMin[i] = 0.0;
                uTMax[i] = 0.0;
                effMin[i] = 0.0;
                effMax[i] = 1.0;
            }
            if (outVecs)
                outVecs->clear();
            CurveDocument* doc = mCurveDocController::getInstance()->document();
            CurveNode* chart = doc ? doc->node(chartId) : nullptr;
            if (!chart || chart->type != CurveNodeType::Chart
                || chart->chart.chartType != 5)
                return false;
            const CurveAxis3DState* axes = chart->chart.axis3d.axes;
            for (int i = 0; i < 3; ++i)
                st[i] = qBound(0, axes[i].scaleType, 3);
            const bool scaled = st[0] != 0 || st[1] != 0 || st[2] != 0;

            //可见曲线收集与 rebuild/queryAxis3DDisplayRange 同一实现（树序）
            const QList<CurveNode*> curves = collectRenderCurves(doc, chartId);

            bool any = false;
            if (scaled)
            {
                //刻度轴：逐曲线变换副本 + 变换空间并集（非 0 轴变换、线性轴
                //浅共享；变换 NaN 分量不入统计=该点在变换空间被排除）。
                //副本入**循环外存活**容器 localVecs（Qt5 QList 对含 QVector
                //成员的类型走节点堆分配、元素地址跨 append 稳定——
                //queryAxis3DDisplayRange 过滤副本同款安全模式），统计指针仅对
                //该容器取址。R43 返工修复：原实现对循环体栈局部 v 取址，循环
                //结束后 unionAxisCore 读已析构对象（UB 两恶果：多曲线时槽位
                //逐迭代复用=指针全部别名末条迭代、并集只统计最后一条曲线；
                //缓存路径[outVecs=nullptr]并读已释放堆=注释锚点投影错位）
                QList<Axis3DRenderVecs> localVecs;
                QList<const QVector<double>*> px, py, pz;
                px.reserve(curves.size());
                py.reserve(curves.size());
                pz.reserve(curves.size());
                for (CurveNode* c : curves)
                {
                    Axis3DRenderVecs v;
                    v.x = transformAxisCopy(c->series.xData, st[0]);
                    v.y = transformAxisCopy(c->series.yData, st[1]);
                    v.z = transformAxisCopy(c->series.zData, st[2]);
                    localVecs.append(v);
                    px.append(&localVecs.last().x);
                    py.append(&localVecs.last().y);
                    pz.append(&localVecs.last().z);
                }
                any = unionAxisCore(px, py, pz,
                    uTMin[0], uTMax[0], uTMin[1], uTMax[1], uTMin[2], uTMax[2]);
                if (outVecs)
                    *outVecs = localVecs;   //逐曲线渲染向量整体移交（QList COW 浅拷贝）
            }
            else
            {
                //线性快路径：原始并集即变换并集（恒等），与旧实现逐位一致
                double rx0 = 0.0, rx1 = 0.0, ry0 = 0.0, ry1 = 0.0, rz0 = 0.0, rz1 = 0.0;
                any = unionAxisRange(curves, rx0, rx1, ry0, ry1, rz0, rz1);
                uTMin[0] = rx0; uTMax[0] = rx1;
                uTMin[1] = ry0; uTMax[1] = ry1;
                uTMin[2] = rz0; uTMax[2] = rz1;
            }
            effectiveAxisRanges(axes, st, uTMin, uTMax, any, effMin, effMax);
            return any;
        }

        //R44 网格样式/背景两色下发单一实现（rebuild 轴组段与 kChart3D* 增量
        //分流共用，禁止两处分叉）。T44-3 实机修正新语义：网格样式（图表级
        //gridStyle）→ 填充开关+有效旗标——线框=setBackdropFillVisible(false)
        //（无背景面）+三轴网格全关（仅轴线/刻度/数字，轴线由引擎恒绘=T44-3
        //补绘，引擎零新样式 API）；三维框=setBackdropFillVisible(true)+按各轴
        //模型 gridVisible 原样（显示网格只管网格线——引擎 setGridVisible 关断
        //不再连带背景面填充，T44-3 语义解耦）。两色恒下发（引擎 setter 成员
        //绘制期消费，代价为零；模型缺省=原引擎硬编码等效，缺省底色零变化）
        void applyBackdropStyle(Qt3DPlotWidget* plot, const CurveChart3DData& d3)
        {
            const bool wireframe = qBound(0, d3.gridStyle, 1) == 1;
            plot->scene()->axis()->setBackdropFillVisible(!wireframe);
            plot->scene()->axis()->setGridVisible(
                wireframe ? false : d3.axes[0].gridVisible,
                wireframe ? false : d3.axes[1].gridVisible,
                wireframe ? false : d3.axes[2].gridVisible);
            plot->scene()->axis()->setBackdropFillColor(d3.backdropColor);
            plot->scene()->axis()->setBackdropGridColor(d3.backdropGridColor);
        }

        //三维曲线云图值域（R36）：取图内"云图开启的可见曲线"（plot3dType==1
        //且 series.contour）按首条命中曲线 contourVar 分量的数据并集；变量
        //不一致的云图曲线不并入（一条色带只能表达一个维度，混合变量为已知
        //限制随 R36 备案）；无命中返回 false（调用方回落既有 z 并集口径）。
        //色带图例与线体渐变共用该值域单一真源（引擎 bindColormap 同源消费）
        bool contourValueRange(const QList<CurveNode*>& curves, double& lo, double& hi)
        {
            bool any = false;
            int comp = -1;
            lo = hi = 0.0;
            for (CurveNode* c : curves)
            {
                if (c->series.plot3dType != 1 || !c->series.contour)
                    continue;
                if (comp < 0)
                    comp = qBound(0, c->series.contourVar, 2);
                else if (qBound(0, c->series.contourVar, 2) != comp)
                    continue;
                const QVector<double>& v = comp == 0 ? c->series.xData
                    : comp == 1 ? c->series.yData : c->series.zData;
                for (double d : v)
                {
                    if (!qIsFinite(d))
                        continue;   //非有限值不参与统计（含首点种子，R40 同 unionAxisRange 口径）
                    if (!any)
                    {
                        any = true;
                        lo = hi = d;
                        continue;
                    }
                    lo = qMin(lo, d);
                    hi = qMax(hi, d);
                }
            }
            if (any && std::abs(hi - lo) < 1e-4)
                hi += 1.0;
            return any;
        }

        //模型符号大小（1..100，默认 6）→ 引擎世界缩放：坐标盒边长 20，
        //0.03 系数下默认档 0.18（直径约盒宽 0.9%，屏上约 6px 小标记观感），
        //随档位线性缩放
        float symbolWorldSize(int symbolSize)
        {
            return static_cast<float>(symbolSize) * 0.03f;
        }

        //本图表参与渲染的曲线列表（树序，按 series.visible 过滤——可见性=
        //渲染与范围统计共同过滤，对齐 2D fit 只统计可见曲线口径；rebuild 与
        //queryAxis3DDisplayRange 共用同一收集实现，禁止两处口径分叉）。
        //全部隐藏→空列表走既有"无曲线"路径（并集 any=false、轴组按模型范围
        //+退化兜底），无特判分支
        QList<CurveNode*> collectRenderCurves(CurveDocument* doc, int chartId)
        {
            QList<CurveNode*> curves;
            if (!doc)
                return curves;
            for (int childId : doc->childNodeIds(chartId))
            {
                CurveNode* c = doc->node(childId);
                if (c && c->type == CurveNodeType::Curve && c->series.visible)
                    curves.append(c);
            }
            return curves;
        }
    }

    m3DChartView::m3DChartView(int chartId, QWidget* parent)
        : QWidget(parent)
        , _chartId(chartId)
    {
        //0 边距：画布铺满卡片内容区（同 mCurveChartView 布局口径）
        QVBoxLayout* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        //0ms 合并重建：一次宏（创建/追加/删除）会连发多条文档信号，只重建一次
        _rebuildTimer = new QTimer(this);
        _rebuildTimer->setSingleShot(true);
        _rebuildTimer->setInterval(0);
        connect(_rebuildTimer, &QTimer::timeout, this, [this]() { rebuild(); });

        //图例覆盖层（R32）：宿主侧自绘，叠于画布之上；视图子控件（非画布子
        //控件），rebuild deleteLater 重建画布时无需随建随销，几何/层级由
        //resizeEvent 与 rebuild 尾同步；构造期隐藏，首次 rebuild 尾刷新显形
        _legendOverlay = new m3DChartLegendOverlay(_chartId, this);

        mCurveDocController* controller = mCurveDocController::getInstance();
        connect(controller, &mCurveDocController::sig_nodeAdded, this,
            [this](int id)
        {
            //注释节点新增（R46 分流）：可见期仅重建注释项集合（相机/GL 场景
            //不动——模式内连续打点创建不丢视角，对齐 2D"注释信号不整场景重
            //建"体验）；不可见期走既有 scheduleRebuild 挂起（B03 口径，
            //showEvent 补跑 rebuild 尾重放）。归属过滤按 chartNodeId（新建
            //节点可查，与 rebuildRelevant 广谱分支"新增侧节点可查"同口径）；
            //非本图表注释（含他图表/页面级节点）走既有分支
            CurveDocument* doc = mCurveDocController::getInstance()->document();
            if (doc && isVisible() && _plot
                && doc->nodeType(id) == CurveNodeType::Annotation
                && doc->chartNodeId(id) == _chartId)
            {
                rebuildAnnotationItems();
                return;
            }
            if (rebuildRelevant(id, QString())) scheduleRebuild();
        });
        connect(controller, &mCurveDocController::sig_nodeRemoved, this,
            [this](int id)
        {
            //注释节点删除（R46 分流）：删除通知到达时节点已从文档摘除、类型
            //不可查（rebuildRelevant 广谱分支保守全量重建的根因）——按注释项
            //集合成员反推：id 在 _annoItems=本图表可见注释删除 → 仅重建注释项
            //集合（相机不动，撤销/重做删除不丢视角）。集合外（隐藏注释/他图表
            //节点/真结构变化）走既有广谱分支
            if (isVisible() && _plot)
            {
                for (m3DChartAnnotationItem* item : _annoItems)
                {
                    if (item->annoId() == id)
                    {
                        rebuildAnnotationItems();
                        return;
                    }
                }
            }
            if (rebuildRelevant(id, QString())) scheduleRebuild();
        });
        //三维注释模式：Ribbon"3D曲线-标注-注释"按钮 -> 视口工厂广播 -> 本视图
        //开关（R46；构造期只连信号不查状态，晚物化视口由工厂 materializeChart
        //承接当前模式补齐）
        connect(mCurveViewportFactory::getInstance(),
            &mCurveViewportFactory::sig_annotation3DModeChanged,
            this, &m3DChartView::setAnnotation3DMode);
        connect(controller, &mCurveDocController::sig_nodePropertyChanged, this,
            [this](int id, const QString& prop)
        {
            //图例专属键分流（R32）：仅刷覆盖层，禁止整场景重建——改图例颜色
            //不应整场景闪重建、丢相机状态；视口隐藏期挂起重建（showEvent 补
            //跑）会随 rebuild 尾统一重放，此处不重复刷
            if (legendPropRelevant(id, prop))
            {
                if (_legendOverlay && _plot && isVisible())
                    _legendOverlay->refreshLegend(
                        _plot->getColormap()->getLegendRect());
                return;
            }
            //曲线样式键分流（R33）：kColor/kLineWidth 保视角增量刷新，处理完
            //return 不落入 scheduleRebuild——样式改写 Line3DSeries 成员绘制期
            //自然生效，整场景重建反而重置相机（历史体验缺陷）。不可见期不碰
            //画布（B03 口径）：改走 scheduleRebuild 挂起，showEvent 补跑全量
            //重建重放正确样式，语义闭合无丢样式窗口
            if (styleRefreshRelevant(id, prop))
            {
                if (isVisible() && _plot)
                    applyStyleIncremental(id, prop);
                else
                    scheduleRebuild();
                return;
            }
            //注释专属键分流（R42 二期）：判定序在 legendPropRelevant/
            //styleRefreshRelevant 之后、rebuildRelevant 之前——命中仅重摆/重绘
            //注释项并 return，禁止整场景重建（相机不丢）；rebuildRelevant 白名单
            //不追加这些键（防双触发，R33 教训；kVisible 虽在白名单但注释节点
            //先被本分支截获，曲线节点 kVisible 仍走重建）。不可见期不碰画布
            //（B03 口径）：改走 scheduleRebuild 挂起，showEvent 补跑 rebuild 尾
            //注释项重放
            if (annotationPropRelevant(id, prop))
            {
                if (isVisible())
                {
                    if (id == _chartId)
                        rebuildAnnotationItems();   //图表级总开关：集合级显隐
                    else
                        refreshAnnotationItem(id);  //单项样式/位置/显隐刷新
                }
                else
                    scheduleRebuild();
                return;
            }
            //基准面属性分流（R48）：保视角增量刷新，处理完 return 不落入
            //scheduleRebuild——基准面 setter 为成员写+标脏（绘制期消费），拖拽/
            //属性页改基准位置/颜色不应整场景重建（rebuild 尾 fitToView 重置相
            //机=拖拽松开视角跳变）。不可见期不碰画布（B03 口径）：改走
            //scheduleRebuild 挂起，showEvent 补跑全量重建重放正确状态。判定序
            //在注释分流之后、图表级网格分流之前——rebuildRelevant 基准键表已
            //剪除这些键（防双触发，R33 教训）
            if (datumPropRelevant(id, prop))
            {
                if (isVisible() && _plot)
                    applyDatumIncremental(id, prop);
                else
                    scheduleRebuild();
                return;
            }
            //图表级网格样式/背景配色分流（R44）：本图表节点 × 三键——可见期
            //重算有效旗标+两色 setter+update()（引擎 setter 均绘制期消费，相机
            //不丢=保视角增量刷新，R33 先例）；处理完 return，rebuildRelevant
            //白名单不追加三键（防双触发，R33 口径）。不可见期不碰画布（B03
            //口径）：scheduleRebuild 挂起，showEvent 补跑全量重建重放正确样式。
            //判定序在注释分流之后、rebuildRelevant 之前——仅精确匹配三键，
            //不吞其他图表级键（kAnnotationsVisible 既有注释分流不动）
            if (id == _chartId
                && (prop == CurveProp::kChart3DGridStyle
                    || prop == CurveProp::kChart3DBackdropColor
                    || prop == CurveProp::kChart3DBackdropGridColor))
            {
                if (isVisible() && _plot)
                    applyChart3DStyleIncremental();
                else
                    scheduleRebuild();
                return;
            }
            if (rebuildRelevant(id, prop))
                scheduleRebuild();
        });

        rebuild();
    }

    bool m3DChartView::rebuildRelevant(int nodeId, const QString& prop) const
    {
        //本图表自身（kChartVisible 由工厂管卡片显隐，不触发渲染重建）
        if (nodeId == _chartId)
            return prop == QLatin1String("data")
                || prop == CurveProp::kChartType
                || prop == CurveProp::kDatum3DVisible;   //基准面图表级总开关（R41：关=全隐走重建）
        //本图表后代（曲线增删/数据直写/曲线可见性/轴壳属性/基准面键集）
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        if (!doc)
            return false;
        //结构类通知（prop 空）分流须先于 chartNodeId 守卫：删除通知到达时
        //节点已从文档移除，chartNodeId 反查失效返回 -1、被守卫一律吞掉，
        //场景残留已删曲线/曲面（树/图例读文档现状正常消失）。删除侧节点已
        //不可查、无法判定原属图表，广谱接受（对齐 2D onNodeRemoved"已不可
        //查→保守全量重建"口径）；新增/撤销恢复侧节点可查，仍按所属图表
        //精确过滤
        if (prop.isEmpty())
            return !doc->node(nodeId) || doc->chartNodeId(nodeId) == _chartId;
        if (doc->chartNodeId(nodeId) != _chartId)
            return false;
        //图例/曲线名（R32，评审 R1）：Legend 属性键与 Curve kName（改名）由
        //legendPropRelevant 分立分流仅刷覆盖层，永不整场景重建——键表本就不含
        //kName，故 Curve 改名在此自然返回 false；结构增删仍走上方 prop.isEmpty()
        //广谱分支；曲线可见性 kVisible 等同名键按节点类型区分——Legend 命中
        //此处、Curve 走下方
        if (doc->nodeType(nodeId) == CurveNodeType::Legend)
            return false;
        //基准面键集（R41，T41-3 接线；R48 剪枝）：几何/样式键（kPosX/kPosY/
        //kPosZ/kNormalAxis/kColor/kDatumOpacity/kText/kLabelPos/kName）已分流
        //datumPropRelevant 增量刷新（保视角，R33/R38 先例）——白名单不再含这
        //些键防双触发（R33 教训）。仅保留：kVisible（翻可见需建图元/翻隐藏需
        //撤图元，无图元级 remove 语义走全量重建）；kDatumType/kReverseClip
        //（本期零渲染消费，字段入模保留——消费侧下一期启用时再分流）。kVisible
        //语义：T8 既有曲线 kVisible 口径不变（曲线节点走下方既有键表），基准
        //节点 kVisible 在此命中；注释节点 kVisible 在 sig 处理链先被
        //annotationPropRelevant 截获，不达此处
        if (doc->nodeType(nodeId) == CurveNodeType::DatumPlane3D)
        {
            return prop == CurveProp::kVisible
                || prop == CurveProp::kDatumType
                || prop == CurveProp::kReverseClip;
        }
        return prop == QLatin1String("data")
            || prop == CurveProp::kVisible   //曲线可见性（T8：取消勾选即时重建）
            || prop == CurveProp::kAxis3DLabel
            || prop == CurveProp::kAxis3DMin
            || prop == CurveProp::kAxis3DMax
            || prop == CurveProp::kAxis3DTickCount
            || prop == CurveProp::kAxis3DGridVisible
            || prop == CurveProp::kAxis3DAutoFit
            || prop == CurveProp::kAxis3DScaleType;   //刻度类型（R43：数据预变换重建）
    }

    bool m3DChartView::legendPropRelevant(int nodeId, const QString& prop) const
    {
        //图例专属键分流（R32；评审 R1 修正分立结构）：命中=仅刷覆盖层不重建
        //GL 场景。两条分立分支、互不放行对方键：
        //  ① Legend 节点 × 12 项参数栏图例键（kPosX/kPosY 与注释侧同名、按
        //     节点类型区分；Legend 自身 kName 改名无渲染消费，不命中）
        //  ② Curve 节点 × kName（图例条目文本=曲线本名，改名必须同步覆盖层；
        //     仅刷覆盖层，3D 场景不消费名字不重建。评审 R1：此前节点类型守卫
        //     先于键判断致本分支死代码，改名后图例文本滞留旧名）
        if (prop.isEmpty())
            return false;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        if (!doc || doc->chartNodeId(nodeId) != _chartId)
            return false;
        if (doc->nodeType(nodeId) == CurveNodeType::Legend)
        {
            return prop == CurveProp::kVisible           //可见性（false=整框隐藏）
                || prop == CurveProp::kAutoPosition      //自动定位（true=图内右上）
                || prop == CurveProp::kPosition          //位置（图外四向映回右上）
                || prop == CurveProp::kPosX              //自定义 X（画布百分比）
                || prop == CurveProp::kPosY              //自定义 Y（画布百分比）
                || prop == CurveProp::kLegendBorderWidth //边框线宽（0=无边框）
                || prop == CurveProp::kBorderColor       //边框颜色
                || prop == CurveProp::kLegendFont        //图例字体（渲染加粗）
                || prop == CurveProp::kLeaderPos         //引线位置（左/下）
                || prop == CurveProp::kBgColor           //背景填充色（含 alpha）
                || prop == CurveProp::kTextColorMode     //曲线文本色两态
                || prop == CurveProp::kTextColor;        //统一自定义文本色
        }
        if (doc->nodeType(nodeId) == CurveNodeType::Curve)
        {
            return prop == CurveProp::kName;             //曲线改名→条目文本同步
        }
        return false;
    }

    bool m3DChartView::styleRefreshRelevant(int nodeId, const QString& prop) const
    {
        //曲线样式键分流（R33 两键；R36 扩至外观与着色全键集；R38 增 kDrawStyle
        //色带类显示模式专享）：本图表 Curve 子节点 × {kColor, kLineWidth,
        //kLineStyle, kSymbolStyle, kSymbolColor, kSymbolDensity, kSymbolSize,
        //kAreaFill, kAreaFillOpacity, kContour, kContourVar, kDrawStyle}。
        //rebuildRelevant 白名单不追加这些键（与本增量分支互斥，防双触发，
        //R33 同款）；chartNodeId 失效（删除通知等）时 prop 非样式键本就不命中
        if (prop != CurveProp::kColor && prop != CurveProp::kLineWidth
            && prop != CurveProp::kLineStyle && prop != CurveProp::kSymbolStyle
            && prop != CurveProp::kSymbolColor && prop != CurveProp::kSymbolDensity
            && prop != CurveProp::kSymbolSize && prop != CurveProp::kAreaFill
            && prop != CurveProp::kAreaFillOpacity && prop != CurveProp::kContour
            && prop != CurveProp::kContourVar && prop != CurveProp::kDrawStyle)
            return false;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        if (!doc || doc->chartNodeId(nodeId) != _chartId)
            return false;
        return doc->nodeType(nodeId) == CurveNodeType::Curve;
    }

    bool m3DChartView::annotationPropRelevant(int nodeId, const QString& prop) const
    {
        //注释专属键分流（R42 二期）：两条互斥分支——
        //  ① 本图表 Chart 节点 × kAnnotationsVisible（图表级总开关→集合级重建
        //     注释项；本图表其余键不命中——kChartType 等仍走 rebuildRelevant）
        //  ② 本图表 Annotation 子节点 × 11 项注释画布键（kName=改名 {Label} 同步；
        //     kPosX/kPosY=拖拽/属性页定位提交；kVisible=单项显隐；其余=样式键）
        //prop 空（结构增删）不命中——注释增删走既有 sig_nodeAdded/sig_nodeRemoved
        //→ rebuildRelevant(id, QString()) 广谱分支 → scheduleRebuild 整场景重建，
        //注释项随 rebuild 尾重放（R35 广谱删除+nodeAdded 归属过滤既有路径可达）
        if (prop.isEmpty())
            return false;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        if (!doc)
            return false;
        if (nodeId == _chartId)
            return prop == CurveProp::kAnnotationsVisible;
        if (doc->chartNodeId(nodeId) != _chartId)
            return false;
        if (doc->nodeType(nodeId) != CurveNodeType::Annotation)
            return false;
        return prop == CurveProp::kText
            || prop == CurveProp::kAlign
            || prop == CurveProp::kColor
            || prop == CurveProp::kFont
            || prop == CurveProp::kBgColor
            || prop == CurveProp::kBorderWidth
            || prop == CurveProp::kBorderShow
            || prop == CurveProp::kVisible
            || prop == CurveProp::kPosX
            || prop == CurveProp::kPosY
            || prop == CurveProp::kPosZ   //三维锚点 Z（R42 修订：缺此键=拖拽/置位提
                                          //交三键后 z 单键不刷新，项停在旧 z 投影位
                                          //=实机"拖拽后闪回"根因，探针实证）
            || prop == CurveProp::kBoxPosX   //3D 框位置三键（R46 问题3：拖拽框体
            || prop == CurveProp::kBoxPosY   //提交框位、锚点不动=牵引线拉长；缺
            || prop == CurveProp::kBoxPosZ   //此三键=框拖拽后不刷新，同闪回口径）
            || prop == CurveProp::kName;
    }

    bool m3DChartView::datumPropRelevant(int nodeId, const QString& prop) const
    {
        //基准面属性分流（R48）：本图表 DatumPlane3D 子节点 × 几何/样式九键——
        //命中走 applyDatumIncremental 保视角增量刷新（拖拽提交/属性页改基准
        //位置不应整场景重建重置相机）。prop 空（结构增删）不命中——基准增删
        //走既有 sig_nodeAdded/sig_nodeRemoved → rebuildRelevant 广谱分支整场
        //景重建（新建/删除图元本就随换代重放）；kVisible（建销图元）与
        //kDatumType/kReverseClip（零渲染消费）不在键表（仍走 rebuild 既有键表）
        if (prop.isEmpty())
            return false;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        if (!doc || doc->chartNodeId(nodeId) != _chartId)
            return false;
        if (doc->nodeType(nodeId) != CurveNodeType::DatumPlane3D)
            return false;
        return prop == CurveProp::kPosX
            || prop == CurveProp::kPosY
            || prop == CurveProp::kPosZ
            || prop == CurveProp::kNormalAxis
            || prop == CurveProp::kColor
            || prop == CurveProp::kDatumOpacity
            || prop == CurveProp::kText
            || prop == CurveProp::kLabelPos
            || prop == CurveProp::kName;
    }

    void m3DChartView::applyDatumIncremental(int nodeId, const QString& prop)
    {
        //保视角增量刷新（R48）：映射无该项（隐藏基准/总开关关=未建图元；kVisible
        //翻可见走 rebuild 重放建项）安全短路；节点失效（删除信号窗口期）同短路
        const auto it = _datumPlanes.constFind(nodeId);
        if (it == _datumPlanes.constEnd())
            return;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        const CurveNode* c = doc ? doc->node(nodeId) : nullptr;
        if (!c || c->type != CurveNodeType::DatumPlane3D)
            return;
        const CurveDatumPlane3DData& d = c->datumPlane3D;
        const std::shared_ptr<DatumPlane3D>& plane = it.value();
        //位置族（拖拽提交/属性页坐标/法向切向）：取值口径与 rebuild 基准段同
        //一实现——法向轴坐标 qIsFinite 防御（脏流 NaN 回退 0）+轴刻度变换到
        //渲染空间（线性恒等零变化）；变换 NaN（位置 ≤0 于 log/dB 轴）跳过保
        //持现位（rebuild 同款防御）。setNormalAxis 对非位置键是冗余成员写（同
        //值幂等，无几何代价），共用分支换取口径单一
        if (prop == CurveProp::kPosX || prop == CurveProp::kPosY
            || prop == CurveProp::kPosZ || prop == CurveProp::kNormalAxis)
        {
            const int axis = qBound(0, d.normalAxis, 2);
            const double coords[3] = {
                qIsFinite(d.posX) ? d.posX : 0.0,
                qIsFinite(d.posY) ? d.posY : 0.0,
                qIsFinite(d.posZ) ? d.posZ : 0.0 };
            int st[3];
            axis3dScaleTypes(_chartId, st);
            const double posT = mCurveChartView::transformValue(
                coords[axis], st[axis], 1.0);
            if (!qIsFinite(posT))
                return;
            plane->setNormalAxis(axis);
            plane->setPosition(posT);
        }
        else if (prop == CurveProp::kColor)
        {
            plane->setColor(d.color);
        }
        else if (prop == CurveProp::kDatumOpacity)
        {
            //非有限回退出厂默认 0.25（rebuild 基准段同款防御）
            plane->setOpacity(static_cast<float>(
                qIsFinite(d.opacity) ? qBound(0.0, d.opacity, 1.0) : 0.25));
        }
        else if (prop == CurveProp::kLabelPos)
        {
            plane->setLabelAnchor(qBound(0, d.labelPos, 8));
        }
        else if (prop == CurveProp::kText || prop == CurveProp::kName)
        {
            //标签=text 空取本名（rebuild 基准段同口径；kName 改名与 kText 编辑
            //两路合流单点）
            plane->setLabel(d.text.isEmpty() ? c->name : d.text);
        }
        _plot->update();
    }

    void m3DChartView::refreshDisplayRangeCache()
    {
        //显示范围缓存=数据↔盒坐标双向换算真源。R43：缓存值=引擎归一化真源
        //=**变换空间**——经 axis3dRenderData 单一实现取值（与 rebuild 轴组
        //setCoordinateLimits 下发同一实现同一取值口径=渲染归一化同一 true 源）；
        //queryAxis3DDisplayRange 契约保持原始数据空间不变（属性页回显/退出
        //自动回写依赖）。返回 false（图表无效/无可见存活数据）时同样缓存其
        //退化兜底值（rebuild 消费该值下发引擎，缓存必须镜像——出参恒缓存、
        //返回值仅作语义标记，口径不变）
        int st[3] = { 0, 0, 0 };
        double uTMin[3] = { 0.0, 0.0, 0.0 };
        double uTMax[3] = { 0.0, 0.0, 0.0 };
        double effMin[3] = { 0.0, 0.0, 0.0 };
        double effMax[3] = { 1.0, 1.0, 1.0 };
        _dispRangeValid = axis3dRenderData(_chartId, st, nullptr,
            uTMin, uTMax, effMin, effMax);
        for (int i = 0; i < 3; ++i)
        {
            _dispRangeMin[i] = effMin[i];
            _dispRangeMax[i] = effMax[i];
        }
    }

    void m3DChartView::rebuildAnnotationItems()
    {
        //旧项全部退役：hide（本帧不再消费事件/不再绘制）+deleteLater，列表立即
        //换代——文档信号与宿主逻辑均不持有旧项指针（防悬挂；项为视图子控件，
        //随宿主析构兜底）
        for (m3DChartAnnotationItem* item : _annoItems)
        {
            item->hide();
            item->deleteLater();
        }
        _annoItems.clear();

        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller ? controller->document() : nullptr;
        CurveNode* chart = doc ? doc->node(_chartId) : nullptr;
        if (!chart || chart->type != CurveNodeType::Chart || chart->chart.chartType != 5)
            return;

        refreshDisplayRangeCache();

        //图表级总开关关闭 → 零项不崩溃（annotationsVisible=false 全隐）
        if (!chart->chart.annotationsVisible)
            return;

        //按文档树序逐可见注释建项（可见=注释 kVisible；kName 供 {Label}）
        for (int childId : doc->childNodeIds(_chartId))
        {
            CurveNode* c = doc->node(childId);
            if (!c || c->type != CurveNodeType::Annotation || !c->annotation.visible)
                continue;
            m3DChartAnnotationItem* item = new m3DChartAnnotationItem(childId, this);
            item->refreshFromModel();   //读模型+投影定位+显形（内部 raise）
            _annoItems.append(item);
        }
    }

    void m3DChartView::refreshAnnotationItem(int nodeId)
    {
        for (m3DChartAnnotationItem* item : _annoItems)
        {
            if (item->annoId() == nodeId)
            {
                item->refreshFromModel();
                return;
            }
        }
        //列表无此项（注释刚从隐藏翻可见/总开关刚打开等集合变化窗口）：集合级
        //重建补齐（幂等，零变化时不产生多余项）
        rebuildAnnotationItems();
    }

    void m3DChartView::layoutAnnotationItems()
    {
        //相机/尺寸变化后：注释项按模型锚点现投影重摆（模型值不变；投影失败=
        //锚点在相机身后，项内部收场隐藏）
        for (m3DChartAnnotationItem* item : _annoItems)
            item->reposition();
    }

    //--- 三维注释模式（R46，对齐二维注释模式交互：按钮检出激活→内容点击创建
    //牵引线注释→再次点击退出；内容外点击不创建且模式保持现状） ---

    void m3DChartView::setAnnotation3DMode(bool on)
    {
        if (_annotation3DMode == on)
            return;
        _annotation3DMode = on;
        _annoPressing = false;   //模式切换即弃用按下捕获（跨态残留无意义）
        //基准面捕获态一并弃用（R48 跨态兜底，工厂广播理论上可穿拖拽期到达）：
        //不提交（模式语义已变），预览位回弹模型值（增量分流 setPosition 同路，
        //图元不在映射[已隐藏/已删]内部短路）
        if (_datumDragging)
        {
            _datumDragging = false;
            const int abandonedId = _dragDatumId;
            _dragDatumId = -1;
            if (_plot && isVisible())
                applyDatumIncremental(abandonedId, CurveProp::kPosX);
        }
        if (!on)
            updateSnapMarker(false);   //退出模式=吸附态消失，标记收场隐藏
        //画布十字光标反馈（模式内画布左键=打点创建语义，区别于常态相机交互；
        //rebuild 重建画布后由 rebuild 尾按本标志重设）
        if (_plot)
            _plot->setCursor(on ? Qt::CrossCursor : Qt::ArrowCursor);
    }

    //--- 寻点标记（R47；视图态——不进命令栈、不写文档、不序列化、不触发 rebuild） ---

    void m3DChartView::setSeekMarker(int curveId, int pointIndex)
    {
        //非法入参归一为"无标记"（清除口径；工厂跨图寻点/曲线删除/页面数据
        //失效三路清除共用本入口）
        if (curveId <= 0 || pointIndex < 0)
            curveId = pointIndex = -1;
        //同值守卫（幂等：重复寻同一点不重复重摆）
        if (curveId == _seekCurveId && pointIndex == _seekPointIndex)
            return;
        _seekCurveId = curveId;
        _seekPointIndex = pointIndex;
        if (_seekCurveId < 0)
        {
            if (_seekOverlay)
                _seekOverlay->hide();
            return;
        }
        //覆盖层懒创建（首次寻点才建；宿主视图子控件随视图析构兜底）
        if (!_seekOverlay)
            _seekOverlay = new SeekMarkerOverlay(this);
        repositionSeekMarker();
    }

    void m3DChartView::repositionSeekMarker()
    {
        if (_seekCurveId <= 0 || _seekPointIndex < 0)
            return;
        if (!_seekOverlay)
            return;   //防御：无标记状态不建覆盖层（setSeekMarker 懒创建先于本调用）
        //具体类型在本 cpp 匿名类（头仅持 QWidget*），此处还原真实类型
        auto* overlay = static_cast<SeekMarkerOverlay*>(_seekOverlay);
        //标记点数据坐标从文档现读（不缓存数据副本——数据改写后下一帧自然
        //跟随新值）：曲线已删/非曲线节点/下标越界/任一坐标非 finite → 失效
        //收场隐藏（安全清除不崩溃；工厂/页面侧的清除通道另行同步状态）
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller ? controller->document() : nullptr;
        const CurveNode* c = doc ? doc->node(_seekCurveId) : nullptr;
        double xyz[3] = { 0.0, 0.0, 0.0 };
        bool ok = c && c->type == CurveNodeType::Curve
            && doc->chartNodeId(_seekCurveId) == _chartId;   //仍属本图表（防跨图串标）
        if (ok)
        {
            const int nPts = qMin(c->series.xData.size(),
                qMin(c->series.yData.size(), c->series.zData.size()));
            ok = _seekPointIndex < nPts;
        }
        if (ok)
        {
            xyz[0] = c->series.xData.at(_seekPointIndex);
            xyz[1] = c->series.yData.at(_seekPointIndex);
            xyz[2] = c->series.zData.at(_seekPointIndex);
            ok = qIsFinite(xyz[0]) && qIsFinite(xyz[1]) && qIsFinite(xyz[2]);
        }
        //投影（数据坐标→画布像素；相机身后/画布未建返回 false）——失败收场
        //隐藏，不崩不闪
        QPointF px;
        if (!ok || !projectDataToPixel(xyz, px))
        {
            overlay->hide();
            return;
        }
        //坐标文本 'f',3 三位小数无空格键值（对齐用户截图 "X:-2.000 Y:-0.800 Z:1.370"）
        const QString text = QStringLiteral("X:%1 Y:%2 Z:%3")
            .arg(QString::number(xyz[0], 'f', 3),
                QString::number(xyz[1], 'f', 3),
                QString::number(xyz[2], 'f', 3));
        overlay->placeAt(px, text);
    }

    bool m3DChartView::pickDataPoint(const QPoint& canvasPos, double outData[3]) const
    {
        outData[0] = outData[1] = outData[2] = 0.0;
        if (!_plot || !isVisible())
            return false;
        const QSize cs = _plot->size();
        if (cs.width() <= 0 || cs.height() <= 0)
            return false;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        const CurveNode* chart = doc ? doc->node(_chartId) : nullptr;
        if (!chart || chart->type != CurveNodeType::Chart || chart->chart.chartType != 5)
            return false;

        //逐轴刻度类型/投影矩阵/画布尺寸各读一次（逐点循环内零文档查询）；
        //拾取空间=渲染空间（刻度轴=变换空间，与系列 setData 下发同空间）
        int st[3];
        axis3dScaleTypes(_chartId, st);
        const QMatrix4x4 mvp = _plot->camera()->getProjectionMatrix()
            * _plot->camera()->getViewMatrix();
        const QList<CurveNode*> curves = collectRenderCurves(doc, _chartId);

        const double r2 = kAnno3DSnapPx * kAnno3DSnapPx;
        double bestD2 = r2;
        bool hit = false;
        double bestData[3] = { 0.0, 0.0, 0.0 };
        //单候选点测试（原始数据坐标入参）：变换→有限性→投影→像素距离比较；
        //顶点/段中点/单元中心三类候选共用本实现（R46 问题2 加密——稀疏曲线/
        //粗网格曲面"段中/面中"点击不再漏检，锚点=候选点原始坐标）
        auto testPoint = [&](double rx, double ry, double rz)
        {
            const double t[3] = {
                mCurveChartView::transformValue(rx, st[0], 1.0),
                mCurveChartView::transformValue(ry, st[1], 1.0),
                mCurveChartView::transformValue(rz, st[2], 1.0) };
            if (!qIsFinite(t[0]) || !qIsFinite(t[1]) || !qIsFinite(t[2]))
                return;
            //渲染空间→归一化盒→裁剪→NDC→像素（与 projectDataToPixel 同式；
            //矩阵外提后逐点直乘，相机身后 w≤0 不可见跳过）
            const QVector4D clip = mvp * QVector4D(
                float(dataToNormalizedBox(t[0], _dispRangeMin[0], _dispRangeMax[0])),
                float(dataToNormalizedBox(t[1], _dispRangeMin[1], _dispRangeMax[1])),
                float(dataToNormalizedBox(t[2], _dispRangeMin[2], _dispRangeMax[2])),
                1.0f);
            if (clip.w() <= 0.0f || qFuzzyIsNull(float(clip.w())))
                return;
            const QPointF px((clip.x() / clip.w() * 0.5f + 0.5f) * cs.width(),
                (0.5f - clip.y() / clip.w() * 0.5f) * cs.height());
            const double dx = px.x() - canvasPos.x();
            const double dy = px.y() - canvasPos.y();
            const double d2 = dx * dx + dy * dy;
            if (d2 <= bestD2)
            {
                bestD2 = d2;
                hit = true;
                bestData[0] = rx;   //回传候选点原始数据坐标
                bestData[1] = ry;
                bestData[2] = rz;
            }
        };

        for (const CurveNode* c : curves)
        {
            const int nPts = qMin(c->series.xData.size(),
                qMin(c->series.yData.size(), c->series.zData.size()));
            if (c->series.plot3dType == 2)
            {
                //曲面（栅格 rows×cols）：顶点+单元中心（R46 问题2 加密——粗网格
                //曲面面中点击命中；中心=四角原始坐标平均，即双线性面近似点）。
                //网格行列无效退化 (N,1) 点序时与 rebuild 同口径，退化为纯顶点+
                //相邻对中点（下方曲线分支同款——薄条带仍可吸附）
                const int rows = c->series.plot3dRows;
                const int cols = c->series.plot3dCols;
                const bool grid = rows > 0 && cols > 0
                    && rows * cols == nPts;
                for (int i = 0; i < nPts; ++i)
                    testPoint(c->series.xData.at(i), c->series.yData.at(i),
                        c->series.zData.at(i));
                if (grid)
                {
                    for (int r = 0; r + 1 < rows; ++r)
                    {
                        for (int col = 0; col + 1 < cols; ++col)
                        {
                            const int i00 = r * cols + col;
                            const int i10 = i00 + 1;
                            const int i01 = i00 + cols;
                            const int i11 = i01 + 1;
                            testPoint((c->series.xData.at(i00) + c->series.xData.at(i10)
                                + c->series.xData.at(i01) + c->series.xData.at(i11)) * 0.25,
                                (c->series.yData.at(i00) + c->series.yData.at(i10)
                                    + c->series.yData.at(i01) + c->series.yData.at(i11)) * 0.25,
                                (c->series.zData.at(i00) + c->series.zData.at(i10)
                                    + c->series.zData.at(i01) + c->series.zData.at(i11)) * 0.25);
                        }
                    }
                }
                else
                {
                    for (int i = 0; i + 1 < nPts; ++i)
                        testPoint((c->series.xData.at(i) + c->series.xData.at(i + 1)) * 0.5,
                            (c->series.yData.at(i) + c->series.yData.at(i + 1)) * 0.5,
                            (c->series.zData.at(i) + c->series.zData.at(i + 1)) * 0.5);
                }
                continue;
            }
            //顶点（柱状=柱顶离散点；曲线/瀑布/退化曲面=采样点）
            for (int i = 0; i < nPts; ++i)
                testPoint(c->series.xData.at(i), c->series.yData.at(i),
                    c->series.zData.at(i));
            if (c->series.plot3dType == 0 || c->series.plot3dType == 1)
            {
                //曲线/瀑布相邻采样对中点（R46 问题2 加密——稀疏曲线段中点击
                //命中，锚点=段中点原始坐标，对齐二维吸附"曲线上任意位置"语义）
                for (int i = 0; i + 1 < nPts; ++i)
                    testPoint((c->series.xData.at(i) + c->series.xData.at(i + 1)) * 0.5,
                        (c->series.yData.at(i) + c->series.yData.at(i + 1)) * 0.5,
                        (c->series.zData.at(i) + c->series.zData.at(i + 1)) * 0.5);
            }
        }
        if (hit)
        {
            outData[0] = bestData[0];
            outData[1] = bestData[1];
            outData[2] = bestData[2];
        }
        return hit;
    }

    QString m3DChartView::currentAnnotationTemplate() const
    {
        //沿用本图表最近一个含模板 token 的注释文本（对齐二维
        //currentAnnotationTemplate"双击编辑过的模板对后续注释生效"口径；3D 判
        //据=文本含 '{'——旧默认"新注释"等纯标签无 token，不作为模板传播）
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        const QList<int> children = doc ? doc->childNodeIds(_chartId) : QList<int>();
        for (int i = children.size() - 1; i >= 0; --i)
        {
            const CurveNode* n = doc->node(children.at(i));
            if (n && n->type == CurveNodeType::Annotation
                && n->annotation.text.contains(QLatin1Char('{')))
                return n->annotation.text;
        }
        //无可用模板：三维默认模板（三行 XYZ，创建即显示拾取点坐标，
        //{X}{Y}{Z}=注释自身锚点渲染期现算）
        return QStringLiteral("X={X}\nY={Y}\nZ={Z}");
    }

    void m3DChartView::createAnnotationAt(const QPoint& canvasPos)
    {
        //内容拾取：吸附半径内无数据点=内容外点击 → 创建窗口注释（R46 修订，
        //用户口径对齐二维"曲线外点击=视图注释"：两类点击均产生注释且模式保持
        //现状，可连续打点；选中新建注释=参数栏跳出）
        double data[3] = { 0.0, 0.0, 0.0 };
        if (!pickDataPoint(canvasPos, data))
        {
            createWindowAnnotationAt(canvasPos);
            return;
        }

        //创建（宏一条撤销）：kAttachMode=0（3D 语义=内容锚定带牵引线，复用
        //2D"曲线类=0 带引出线"档位值，渲染分流见 m3DChartAnnotationItem）+
        //kText=模板原文（渲染期按锚点求值 {X}{Y}{Z}，对齐 2D"模型存模板"纪律）
        //+锚点三键=拾取点原始坐标（框位三键缺省 NaN=框在锚点+固定偏移）
        mCurveDocController* controller = mCurveDocController::getInstance();
        QUndoStack* stack = controller->undoStack();
        stack->beginMacro(QStringLiteral("添加注释"));
        const int id = controller->addAnnotation(_chartId);
        if (id > 0)
        {
            controller->setNodeProperty(id, CurveProp::kAttachMode, 0);
            controller->setNodeProperty(id, CurveProp::kText, currentAnnotationTemplate());
            controller->setNodeProperty(id, CurveProp::kPosX, data[0]);
            controller->setNodeProperty(id, CurveProp::kPosY, data[1]);
            controller->setNodeProperty(id, CurveProp::kPosZ, data[2]);
        }
        stack->endMacro();
        if (id > 0)
            controller->setCurrentNodeId(id);   //选中新建注释：树回选+参数栏换绑
    }

    void m3DChartView::createWindowAnnotationAt(const QPoint& canvasPos)
    {
        //窗口注释锚点：点击像素反投影到"过显示范围中心、视向为法向"的相机平
        //行面（差分法 through=变换空间中点，与拖拽反投影同一实现——正交=精确
        //仿射、透视=锚点处线性化）；反投影失败（画布未建等）静默不创建
        int st[3];
        axis3dScaleTypes(_chartId, st);
        double through[3] = { 0.0, 0.0, 0.0 };
        for (int i = 0; i < 3; ++i)
        {
            through[i] = mCurveChartView::inverseTransformValue(
                normalizedBoxToData(0.0, _dispRangeMin[i], _dispRangeMax[i]),
                st[i], 1.0);
        }
        double out[3] = { 0.0, 0.0, 0.0 };
        if (!unprojectPixelThroughData(canvasPos, through, out))
            return;

        //创建（宏一条撤销）：kAttachMode=1（窗口=框左上角即锚点、无牵引线，
        //对齐二维"曲线外=视图类"档位值）+kText="新注释"（三维既有默认；二维
        //视图注释的占位文本依赖双击编辑对话框补内容，三维无该入口故直接用默
        //认名）+锚点三键=反投影点
        mCurveDocController* controller = mCurveDocController::getInstance();
        QUndoStack* stack = controller->undoStack();
        stack->beginMacro(QStringLiteral("添加注释"));
        const int id = controller->addAnnotation(_chartId);
        if (id > 0)
        {
            controller->setNodeProperty(id, CurveProp::kAttachMode, 1);
            controller->setNodeProperty(id, CurveProp::kText, QStringLiteral("新注释"));
            controller->setNodeProperty(id, CurveProp::kPosX, out[0]);
            controller->setNodeProperty(id, CurveProp::kPosY, out[1]);
            controller->setNodeProperty(id, CurveProp::kPosZ, out[2]);
        }
        stack->endMacro();
        if (id > 0)
            controller->setCurrentNodeId(id);   //选中新建注释：树回选+参数栏换绑
    }

    void m3DChartView::updateSnapMarker(bool showIt, const double point[3])
    {
        if (showIt && point)
        {
            _snapValid = true;
            _snapPoint[0] = point[0];
            _snapPoint[1] = point[1];
            _snapPoint[2] = point[2];
        }
        else if (!showIt)
            _snapValid = false;
        if (!_snapValid)
        {
            if (_snapOverlay)
                _snapOverlay->hide();
            return;
        }
        //覆盖层懒创建（模式内首次命中才建；宿主视图子控件随视图析构兜底）
        if (!_snapOverlay)
            _snapOverlay = new SnapMarkerOverlay(this);
        //命中点原始数据坐标→画布像素（与注释项同一投影 seam；相机身后/画布
        //未建=投影失败收场隐藏，下一帧或下一次 hover 恢复）
        QPointF px;
        if (!projectDataToPixel(_snapPoint, px))
        {
            _snapOverlay->hide();
            return;
        }
        static_cast<SnapMarkerOverlay*>(_snapOverlay)->placeAt(px);
    }

    //--- 三维锚点投影/反投影（数学口径见 .h seam 注释；BOX_SIZE=10 归一化约定） ---

    double m3DChartView::dataToNormalizedBox(double data, double axisMin, double axisMax)
    {
        //(v-min)/range*20-10（与引擎 Line3DSeries normalizedPoint 同式；range
        //恒正——rebuild/queryAxis3DDisplayRange 退化轴兜底 max+=1 已保证）
        return (data - axisMin) / (axisMax - axisMin) * 20.0 - 10.0;
    }

    double m3DChartView::normalizedBoxToData(double world, double axisMin, double axisMax)
    {
        //(v-min)/range*20-10 反算：v = min + (w+10)/20*range
        return axisMin + (world + 10.0) / 20.0 * (axisMax - axisMin);
    }

    bool m3DChartView::projectDataToPixel(const double xyz[3], QPointF& outPx) const
    {
        if (!_plot)
            return false;
        const QSize cs = _plot->size();
        if (cs.width() <= 0 || cs.height() <= 0)
            return false;
        //R43：锚点=模型原始值（kPosX/kPosY/kPosZ 不随刻度类型改写）——先按
        //轴刻度变换到渲染空间再入盒（缓存 _dispRangeMin/Max=变换空间；线性轴
        //变换恒等零变化）。任一轴变换 NaN（锚点 ≤0 于 log/dB 轴）→ 无法投影
        //返回 false（注释项收场隐藏，防御不崩）
        int st[3];
        axis3dScaleTypes(_chartId, st);
        const double tx[3] = {
            mCurveChartView::transformValue(xyz[0], st[0], 1.0),
            mCurveChartView::transformValue(xyz[1], st[1], 1.0),
            mCurveChartView::transformValue(xyz[2], st[2], 1.0) };
        if (!qIsFinite(tx[0]) || !qIsFinite(tx[1]) || !qIsFinite(tx[2]))
            return false;
        //数据坐标→归一化盒（三轴；缓存范围=渲染归一化同一 true 源）
        const QVector3D world(
            float(dataToNormalizedBox(tx[0], _dispRangeMin[0], _dispRangeMax[0])),
            float(dataToNormalizedBox(tx[1], _dispRangeMin[1], _dispRangeMax[1])),
            float(dataToNormalizedBox(tx[2], _dispRangeMin[2], _dispRangeMax[2])));
        //盒坐标→裁剪坐标→NDC（正交 w=1 恒定；透视 w=视线深度，锚点在相机身后
        //w≤0 无意义返回 false）
        const QVector4D clip = _plot->camera()->getProjectionMatrix()
            * _plot->camera()->getViewMatrix() * QVector4D(world, 1.0f);
        if (clip.w() <= 0.0f || qFuzzyIsNull(float(clip.w())))
            return false;
        const float ndcX = clip.x() / clip.w();
        const float ndcY = clip.y() / clip.w();
        //NDC→像素（视口原点左上：x=(ndc+1)/2*w，y=(1-ndc)/2*h）
        outPx = QPointF((ndcX * 0.5f + 0.5f) * cs.width(),
            (0.5f - ndcY * 0.5f) * cs.height());
        return true;
    }

    bool m3DChartView::unprojectPixelThroughData(const QPointF& px, const double throughData[3],
        double outData[3]) const
    {
        if (!_plot)
            return false;
        const QSize cs = _plot->size();
        if (cs.width() <= 0 || cs.height() <= 0)
            return false;
        //R43：差分基准锚点=原始值先按轴刻度变换入盒（盒位移在变换空间叠加，
        //线性恒等零变化）；变换 NaN（锚点 ≤0 于 log/dB 轴）→ 无法定义差分
        //返回 false（调用方保持原值不提交）。**像素差分法**（实机缺陷修复
        //终版，R42 逻辑原样保留、仅外围包变换）：不构射线不求交——把"锚点
        //当前投影像素→松开像素"的 NDC 差量经 inverse(proj*view) 映射回世界
        //盒位移，叠加到锚点世界坐标。构造性保证 提交锚点的投影 ≡ 松开像素
        //（project∘unproject 恒等闭合），从根上消灭"拖拽后闪回"（此前射线
        //求交两轮方案在边缘钳位/深度外锚点上数值退化，探针实证提交值塌回
        //原锚点）。透视投影下该式为锚点处线性化（拖拽小位移标准技法，默认
        //相机为正交=精确仿射）
        int st[3];
        axis3dScaleTypes(_chartId, st);
        const double throughT[3] = {
            mCurveChartView::transformValue(throughData[0], st[0], 1.0),
            mCurveChartView::transformValue(throughData[1], st[1], 1.0),
            mCurveChartView::transformValue(throughData[2], st[2], 1.0) };
        if (!qIsFinite(throughT[0]) || !qIsFinite(throughT[1]) || !qIsFinite(throughT[2]))
            return false;
        QPointF p0Pix;
        if (!projectDataToPixel(throughData, p0Pix))
            return false;   //锚点投影失败（相机身后等）：无法定义差分，保持原位
        const float dx = 2.0f * float(px.x() - p0Pix.x()) / cs.width();
        const float dy = -2.0f * float(px.y() - p0Pix.y()) / cs.height();
        const QMatrix4x4 inv = (_plot->camera()->getProjectionMatrix()
            * _plot->camera()->getViewMatrix()).inverted();
        //NDC 差量（z=0：屏面内移动不改变深度线性化坐标）→ 世界位移（w=0 方向
        //向量用法，取 xyz 分量——toVector3DAffine 会除以 w=0 除零毒化，实机
        //"拖拽闪回"探针实证根因之一，禁用）
        const QVector4D deltaW = inv * QVector4D(dx, dy, 0.0f, 0.0f);
        const QVector3D through(
            float(dataToNormalizedBox(throughT[0], _dispRangeMin[0], _dispRangeMax[0])),
            float(dataToNormalizedBox(throughT[1], _dispRangeMin[1], _dispRangeMax[1])),
            float(dataToNormalizedBox(throughT[2], _dispRangeMin[2], _dispRangeMax[2])));
        const QVector3D hit = through + QVector3D(deltaW.x(), deltaW.y(), deltaW.z());
        //反算数据坐标（不设盒内约束——拖拽允许把锚点移到当前轴范围外，超出
        //范围由渲染侧越界收敛保证注释框可见），末端按轴刻度逆变换回**原始
        //数据空间**再落 kPosX/kPosY/kPosZ（正逆变换闭合；逆变换恒有限——
        //幂运算对有限输入恒有限，无需 NaN 防御）
        for (int i = 0; i < 3; ++i)
        {
            outData[i] = mCurveChartView::inverseTransformValue(
                normalizedBoxToData(hit[i], _dispRangeMin[i], _dispRangeMax[i]),
                st[i], 1.0);
        }
        return true;
    }

    void m3DChartView::applyStyleIncremental(int nodeId, const QString& prop)
    {
        //色带类显示模式（R38，plot3dType==2 专享）：查曲面映射下发
        //setDrawStyle（引擎 setter=成员写+update，绘制期消费，等高线 VBO 在
        //setData 时已建——无需手动 _plot->update()，处理完 return 不落曲线
        //分支）。映射无该 id（隐藏曲面不建系列/曲线类·瀑布·柱状序列不入
        //映射）安全短路；钳 0..2 与序列化读侧同口径
        if (prop == CurveProp::kDrawStyle)
        {
            const auto itS = _surfaceSeries.constFind(nodeId);
            if (itS == _surfaceSeries.constEnd())
                return;
            CurveDocument* docS = mCurveDocController::getInstance()->document();
            CurveNode* cS = docS ? docS->node(nodeId) : nullptr;
            if (!cS)
                return;
            itS.value()->setDrawStyle(static_cast<DrawStyle>(
                qBound(0, cS->series.drawStyle, 2)));
            return;
        }

        //保视角增量样式刷新：映射无该 id（隐藏曲线 kVisible=false 不建系列/
        //色带类序列不入映射）安全短路——不崩溃、不重建、不刷图例（隐藏曲线
        //也不进图例条目，无可刷内容）
        const auto it = _lineSeries.constFind(nodeId);
        if (it == _lineSeries.constEnd())
            return;

        CurveDocument* doc = mCurveDocController::getInstance()->document();
        CurveNode* c = doc ? doc->node(nodeId) : nullptr;
        if (!c)
            return;

        //改成员即下一帧生效（引擎样式 setter 为绘制期消费：draw() 内
        //setUniformValue/discard/实例重排，几何类改动标脏后 draw 前重建缓冲，
        //均无需整场景重建）。R36 起键集扩至外观与着色全键
        const std::shared_ptr<Line3DSeries>& series = it.value();
        if (prop == CurveProp::kColor)
        {
            series->setColor(c->series.color);
            //图例条目图标=series.color 现读（须同步；脏键含色值/线宽，零变化
            //免重绘）；线宽键的图标线宽由画布每帧 Paint 后的既定同步链路兜底
            //（eventFilter Queued→refreshLegend），此处只处理状态最易滞后的
            //颜色键
            if (_legendOverlay)
                _legendOverlay->refreshLegend(_plot->getColormap()->getLegendRect());
        }
        else if (prop == CurveProp::kLineWidth)
        {
            series->setLineWidth(static_cast<float>(c->series.lineWidth));
        }
        else if (prop == CurveProp::kLineStyle)
        {
            series->setLineStyle(c->series.lineStyle);
        }
        else if (prop == CurveProp::kSymbolStyle)
        {
            series->setSymbolStyle(c->series.symbolStyle);
        }
        else if (prop == CurveProp::kSymbolColor)
        {
            series->setSymbolColor(c->series.symbolColor);
        }
        else if (prop == CurveProp::kSymbolDensity)
        {
            series->setSymbolDensity(c->series.symbolDensity);
        }
        else if (prop == CurveProp::kSymbolSize)
        {
            series->setSymbolSize(symbolWorldSize(c->series.symbolSize));
        }
        else if (prop == CurveProp::kAreaFill)
        {
            series->setAreaFill(c->series.areaFill);
        }
        else if (prop == CurveProp::kAreaFillOpacity)
        {
            series->setAreaFillOpacity(c->series.areaFillOpacity);
        }
        else if (prop == CurveProp::kContour || prop == CurveProp::kContourVar)
        {
            //云图开关/变量：序列 setter + 色带值域跟随（云图开启=分量值域，
            //全关回落 z 并集口径；contourValueRange 与 rebuild 单一实现共用，
            //禁止两处分叉）
            series->setContour(c->series.contour);
            series->setContourComponent(c->series.contourVar);
            const QList<CurveNode*> curves = collectRenderCurves(
                mCurveDocController::getInstance()->document(), _chartId);
            double cLo = 0.0, cHi = 0.0;
            if (contourValueRange(curves, cLo, cHi))
                _plot->getColormap()->setDataRange(
                    static_cast<float>(cLo), static_cast<float>(cHi));
            else
            {
                double ux0 = 0.0, ux1 = 0.0, uy0 = 0.0, uy1 = 0.0, uz0 = 0.0, uz1 = 0.0;
                unionAxisRange(curves, ux0, ux1, uy0, uy1, uz0, uz1);
                _plot->getColormap()->setDataRange(
                    static_cast<float>(uz0), static_cast<float>(uz1));
            }
            //值域刻度文字变化会影响色带矩形，图例避让随既定链路同步
            if (_legendOverlay)
                _legendOverlay->refreshLegend(_plot->getColormap()->getLegendRect());
        }
        _plot->update();
    }

    void m3DChartView::applyChart3DStyleIncremental()
    {
        //R44 保视角增量刷新（图表级网格样式/背景配色）：按模型现值重算有效
        //旗标+两色 setter——与 rebuild 轴组段同一实现（applyBackdropStyle 单一
        //真源）；引擎三 setter 均成员写、draw 期 setUniformValue/分支消费，改
        //成员下一帧自然生效，无需整场景重建（相机不丢）。调用方已守卫可见期
        //与 _plot 非空；图表无效（已删）安全短路
        if (!_plot)
            return;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        CurveNode* chart = doc ? doc->node(_chartId) : nullptr;
        if (!chart || chart->type != CurveNodeType::Chart || chart->chart.chartType != 5)
            return;
        applyBackdropStyle(_plot, chart->chart.axis3d);
        _plot->update();
    }

    void m3DChartView::scheduleRebuild()
    {
        _rebuildTimer->start();
    }

    void m3DChartView::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        //不可见期挂起的重建在首显/重显补跑（沿用 0ms 合并定时器：与可见期的
        //文档信号重建同一出口，多次 show 不重复排程——定时器单发合并）
        if (_rebuildPending)
            scheduleRebuild();
    }

    void m3DChartView::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        //覆盖层随视图几何铺满（与画布同矩形：布局 0 边距）；重绘由画布 resize
        //后的 Paint 同步（refreshLegend 脏键含框矩形）触发，此处只同步几何
        if (_legendOverlay)
            _legendOverlay->setGeometry(rect());
        //注释项随画布尺寸重摆（R42 二期）：kPosX/kPosY 百分比不变、按新尺寸
        //换算像素（anchorPos 内部越界收敛）
        layoutAnnotationItems();
        //寻点标记随画布尺寸重投影（R47）
        repositionSeekMarker();
    }

    //--- 基准面鼠标拖动（R48）：常态（注释模式外）画布左键=先拾取基准面、命中
    //即拖拽移动（事件在本视图截获消费，引擎相机交互不进入），未命中照常放行
    //引擎旋转——拾取/预览/提交三段如下 ---

    bool m3DChartView::hitTestDatumPlane(const QPoint& canvasPos, int& outDatumId) const
    {
        outDatumId = -1;
        if (!_plot || !isVisible())
            return false;
        const QSize cs = _plot->size();
        if (cs.width() <= 0 || cs.height() <= 0)
            return false;
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        const CurveNode* chart = doc ? doc->node(_chartId) : nullptr;
        if (!chart || chart->type != CurveNodeType::Chart || chart->chart.chartType != 5)
            return false;
        //色带图例区域让路（引擎图例拖动/范围调整优先——与引擎 mousePressEvent
        //同三矩形判定；图例不可见零开销）
        if (_plot->getColormap()->isVisible()
            && (_plot->getColormap()->getLegendRect().contains(canvasPos)
                || _plot->getColormap()->getMaxRect().contains(canvasPos)
                || _plot->getColormap()->getMinRect().contains(canvasPos)))
            return false;
        //图表级总开关关=无图元（rebuild 未建项），拾取无从谈起
        if (!chart->chart.datum3DVisible)
            return false;

        //投影矩阵/轴刻度各读一次（逐面循环内零文档查询）；拾取空间=渲染空间
        //（与 rebuild 基准段 setPosition 下发同空间）
        int st[3];
        axis3dScaleTypes(_chartId, st);
        const QMatrix4x4 mvp = _plot->camera()->getProjectionMatrix()
            * _plot->camera()->getViewMatrix();
        //树序迭代取最后命中（场景按插入次序绘制=后建在上，与半透明混合观感
        //次序一致）；单面判定=法向轴位置四边形（另两轴盒空间满幅 [-10,10]，
        //与 rebuild 基准段同一坐标约定）投影成画布凸四边形做点包含测试——
        //引擎 DatumPlane3D 非拾取图元（getDataCount=0/drawSelection 空实现），
        //宿主解析拾取零引擎改动（R46 pickDataPoint 同口径）
        for (int childId : doc->childNodeIds(_chartId))
        {
            const CurveNode* c = doc->node(childId);
            if (!c || c->type != CurveNodeType::DatumPlane3D || !c->datumPlane3D.visible)
                continue;
            const CurveDatumPlane3DData& d = c->datumPlane3D;
            const int axis = qBound(0, d.normalAxis, 2);
            const double coords[3] = {
                qIsFinite(d.posX) ? d.posX : 0.0,
                qIsFinite(d.posY) ? d.posY : 0.0,
                qIsFinite(d.posZ) ? d.posZ : 0.0 };
            const double posT = mCurveChartView::transformValue(
                coords[axis], st[axis], 1.0);
            if (!qIsFinite(posT))
                continue;   //变换 NaN（位置 ≤0 于 log/dB 轴）=rebuild 跳过未建面
            //法向轴位置钳入盒（引擎 draw 前重建几何按当前盒钳制——拾取与观感
            //同一位置，越界模型值不再放大命中区）
            const double boxAxis = qBound(-10.0,
                dataToNormalizedBox(posT, _dispRangeMin[axis], _dispRangeMax[axis]),
                10.0);
            //四角盒空间→像素（与 projectDataToPixel 同式；w≤0=面在相机身后，
            //该面本帧不可拾取）；(u,v) 四点循环序即凸四边形环绕序
            const int u = (axis + 1) % 3;
            const int v = (axis + 2) % 3;
            static const double kCorners[4][2] = {
                { -10.0, -10.0 }, { 10.0, -10.0 }, { 10.0, 10.0 }, { -10.0, 10.0 } };
            QPolygonF quad;
            bool visibleQuad = true;
            for (const auto& corner : kCorners)
            {
                //分量装配经数组下标（QVector3D 构造器直填——Qt5 QVector3D 无
                //可写 operator[]，分量轴随法向轴轮换不可用 setX/setY 直写）
                float w[3] = { 0.0f, 0.0f, 0.0f };
                w[axis] = float(boxAxis);
                w[u] = float(corner[0]);
                w[v] = float(corner[1]);
                const QVector4D clip = mvp * QVector4D(QVector3D(w[0], w[1], w[2]), 1.0f);
                if (clip.w() <= 0.0f || qFuzzyIsNull(float(clip.w())))
                {
                    visibleQuad = false;
                    break;
                }
                quad.append(QPointF(
                    (clip.x() / clip.w() * 0.5f + 0.5f) * cs.width(),
                    (0.5f - clip.y() / clip.w() * 0.5f) * cs.height()));
            }
            if (visibleQuad && quad.containsPoint(QPointF(canvasPos), Qt::OddEvenFill))
                outDatumId = childId;   //最后命中保留（后绘制在上）
        }
        return outDatumId > 0;
    }

    void m3DChartView::datumDragMove(const QPoint& canvasPos)
    {
        //捕获期逐次预览：图元/节点失效（拖拽中场景换代、基准被删/隐藏）安全
        //终止捕获——后续 move/释放按未捕获态放行引擎
        const auto it = _datumPlanes.constFind(_dragDatumId);
        if (it == _datumPlanes.constEnd())
        {
            _datumDragging = false;
            _dragDatumId = -1;
            return;
        }
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        const CurveNode* c = doc ? doc->node(_dragDatumId) : nullptr;
        if (!c || c->type != CurveNodeType::DatumPlane3D)
        {
            _datumDragging = false;
            _dragDatumId = -1;
            return;
        }
        const CurveDatumPlane3DData& d = c->datumPlane3D;
        const int axis = qBound(0, d.normalAxis, 2);
        //像素差分反投影（与注释拖拽同一 seam）：差分基准=模型提交位（拖拽期
        //文档不动=固定参考，多次 move 绝对映射自洽——投影失败步自动跳过不积
        //误差）；结果取**法向轴分量**（平面只沿法向移动，平面内两轴分量丢弃；
        //法向指向相机时屏面内任意拖动仍产生该轴分量=正交投影下摄像机平行面
        //含法向轴，交互无死区）
        const double coords[3] = {
            qIsFinite(d.posX) ? d.posX : 0.0,
            qIsFinite(d.posY) ? d.posY : 0.0,
            qIsFinite(d.posZ) ? d.posZ : 0.0 };
        double through[3] = { 0.0, 0.0, 0.0 };
        through[axis] = coords[axis];
        double out[3] = { 0.0, 0.0, 0.0 };
        if (!unprojectPixelThroughData(canvasPos, through, out))
            return;   //反投影失败（画布未建/参考位投影失败）保持现位
        //法向轴分量钳入显示范围（盒空间 [-10,10]；视觉=引擎钳制边界一致，模
        //型提交值=屏上位置不再放大越界），换回渲染空间值下发预览（引擎 setter
        //成员写+标脏，下一帧重建几何）
        int st[3];
        axis3dScaleTypes(_chartId, st);
        const double outT = mCurveChartView::transformValue(out[axis], st[axis], 1.0);
        if (!qIsFinite(outT))
            return;
        const double box = qBound(-10.0,
            dataToNormalizedBox(outT, _dispRangeMin[axis], _dispRangeMax[axis]),
            10.0);
        it.value()->setPosition(
            normalizedBoxToData(box, _dispRangeMin[axis], _dispRangeMax[axis]));
        _plot->update();
    }

    void m3DChartView::datumDragCommit()
    {
        const auto it = _datumPlanes.constFind(_dragDatumId);
        CurveDocument* doc = mCurveDocController::getInstance()->document();
        const CurveNode* c = doc ? doc->node(_dragDatumId) : nullptr;
        if (it != _datumPlanes.constEnd() && c
            && c->type == CurveNodeType::DatumPlane3D)
        {
            const CurveDatumPlane3DData& d = c->datumPlane3D;
            const int axis = qBound(0, d.normalAxis, 2);
            const double coords[3] = {
                qIsFinite(d.posX) ? d.posX : 0.0,
                qIsFinite(d.posY) ? d.posY : 0.0,
                qIsFinite(d.posZ) ? d.posZ : 0.0 };
            //预览位（图元 position=最后成功 move 下发的渲染空间值）逆变换回
            //原始数据值：与模型现值比对无变化不发（原地点击/拖回原位不压空操
            //作宏，对齐注释拖拽"无变化不发"纪律）
            int st[3];
            axis3dScaleTypes(_chartId, st);
            const double raw = mCurveChartView::inverseTransformValue(
                it.value()->position(), st[axis], 1.0);
            if (raw != coords[axis])
            {
                const char* const posKeys[3] = {
                    CurveProp::kPosX, CurveProp::kPosY, CurveProp::kPosZ };
                mCurveDocController* controller = mCurveDocController::getInstance();
                controller->undoStack()->beginMacro(QStringLiteral("移动基准面"));
                controller->setNodeProperty(_dragDatumId, posKeys[axis], raw);
                controller->undoStack()->endMacro();
                //提交信号经 datumPropRelevant 分流回写同值图元=视觉零跳变；
                //基准页属性行随文档信号回显（列表选中态不受影响——refreshList
                //保持当前选中）
            }
        }
        _datumDragging = false;
        _dragDatumId = -1;
    }

    bool m3DChartView::eventFilter(QObject* obj, QEvent* event)
    {
        //点击画布空白处清除注释选中态（R46 问题5 追加，用户口径"点击选中后
        //再点击别的地方选中状态消失"）：命中注释项框体的按下被项自身消费不达
        //画布=本分支不触发（点另一注释→该注释自身点击选中，单一选中自动顶掉
        //旧选中）；未命中任何注释项的画布左键按下在此清除注释选中。不消费事
        //件=相机交互零影响；仅当前选中为注释节点时写空，不误伤图表/曲线选中
        if (obj == _plot && event->type() == QEvent::MouseButtonPress
            && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
        {
            mCurveDocController* ctrl = mCurveDocController::getInstance();
            const int curSel = ctrl ? ctrl->currentNodeId() : -1;
            CurveDocument* docClear = (ctrl && curSel > 0) ? ctrl->document() : nullptr;
            if (docClear && docClear->nodeType(curSel) == CurveNodeType::Annotation)
                ctrl->setCurrentNodeId(-1);
        }
        //三维注释模式（R46）：画布左键按下/松开在本过滤器截获消费——不进引擎
        //输入控制器（相机旋转不再触发），单击（≤4px）拾取内容点创建牵引线注
        //释/未命中创建窗口注释（R46 修订），拖拽（>4px）不创建（模式保持现状
        //可继续打点）。命中注释项框体的点击被项自身消费（选中/拖拽），不达画
        //布=本分支不触发，框体交互不受模式影响。其余键（右键/中键/滚轮）不截
        //获=模式内仍可平移/缩放导航
        if (obj == _plot && _annotation3DMode)
        {
            if (event->type() == QEvent::MouseButtonPress
                && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
            {
                _annoPressing = true;
                _annoPressPos = static_cast<QMouseEvent*>(event)->pos();
                return true;   //消费：引擎相机不进入旋转态
            }
            if (event->type() == QEvent::MouseButtonRelease
                && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton
                && _annoPressing)
            {
                _annoPressing = false;
                //≤4px=单击创建；拖拽（视角未动——左键按下已被消费）静默放弃
                if ((static_cast<QMouseEvent*>(event)->pos() - _annoPressPos)
                    .manhattanLength() <= kAnno3DClickPx)
                    createAnnotationAt(static_cast<QMouseEvent*>(event)->pos());
                //创建后标记重拾（光标停留处可能已有注释框，按新候选态刷新）
                const QPoint pos = static_cast<QMouseEvent*>(event)->pos();
                double data[3] = { 0.0, 0.0, 0.0 };
                updateSnapMarker(pickDataPoint(pos, data), data);
                return true;
            }
            //吸附高亮（R46 问题4）：无按键 hover 移动在本过滤器消费不上送引擎
            //（引擎 InputController 对无按键 move 非空操作[图例悬停判定+
            //requestPicking 离屏拾取]，R42 读数先例同款——消费后引擎收到的
            //move 事件集与模式开启前逐位一致，交互语义零变化）；带按键 move 照
            //常上送=旋转/缩放/色带拖动不受影响。拾取=内容最近候选点（顶点+段
            //中/单元中心，半径 30px），命中显示蓝色吸附标记，未命中隐藏
            if (event->type() == QEvent::MouseMove
                && static_cast<QMouseEvent*>(event)->buttons() == Qt::NoButton)
            {
                double data[3] = { 0.0, 0.0, 0.0 };
                updateSnapMarker(pickDataPoint(static_cast<QMouseEvent*>(event)->pos(), data), data);
                return true;
            }
            if (event->type() == QEvent::Leave)
            {
                updateSnapMarker(false);   //光标离画布=无吸附态，标记收场
                return false;              //离开事件照常传递（无引擎语义）
            }
        }
        //基准面鼠标拖动（R48）：常态（注释模式外）画布左键按下先宿主解析拾取
        //基准面——命中即截获消费（引擎相机不进入旋转态，拖拽语义=移动基准面），
        //未命中照常放行引擎（旋转/平移/图例拖动交互零变化）。拖动期预览由
        //datumDragMove 独占（文档不动、相机不动），松开 datumDragCommit 一次
        //提交（原地点击/拖回原位无宏副作用）。无按键 hover move：命中基准面显
        //SizeAll 光标并消费（引擎 cursorChanged 对无按键 move 恒发 Arrow 会覆
        //盖自设光标；基准面本就非引擎拾取图元，跳过 requestPicking 无功能损
        //失），未命中放行（引擎箭头光标+离屏拾取语义零变化，R46 模式外同款放
        //行口径）；带按键 move 非捕获态照常放行。捕获中左键丢失（move 无左键
        //态，Alt+Tab 等场景释放事件丢失兜底）按提交收场，不留悬挂捕获态
        if (obj == _plot && !_annotation3DMode)
        {
            if (event->type() == QEvent::MouseButtonPress
                && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton
                && hitTestDatumPlane(static_cast<QMouseEvent*>(event)->pos(),
                    _dragDatumId))
            {
                _datumDragging = true;
                _plot->setCursor(Qt::SizeAllCursor);   //移动语义光标（悬停反馈同形）
                return true;   //消费：引擎相机不进入旋转态
            }
            if (event->type() == QEvent::MouseMove)
            {
                if (_datumDragging
                    && !(static_cast<QMouseEvent*>(event)->buttons() & Qt::LeftButton))
                {
                    datumDragCommit();   //捕获中左键丢失兜底：按松开提交收场
                    _plot->setCursor(Qt::ArrowCursor);
                    return true;
                }
                if (_datumDragging
                    && (static_cast<QMouseEvent*>(event)->buttons() & Qt::LeftButton))
                {
                    datumDragMove(static_cast<QMouseEvent*>(event)->pos());
                    return true;   //捕获期 move 全消费（拖拽中无相机语义）
                }
                if (static_cast<QMouseEvent*>(event)->buttons() == Qt::NoButton)
                {
                    int hoverId = -1;
                    if (hitTestDatumPlane(static_cast<QMouseEvent*>(event)->pos(),
                        hoverId))
                    {
                        _plot->setCursor(Qt::SizeAllCursor);
                        return true;   //消费：防引擎 Arrow 覆盖悬停反馈光标
                    }
                    //未命中放行：引擎箭头/图例悬停光标+requestPicking 离屏拾取
                    //语义零变化（基准面非拾取图元无功能损失）
                }
            }
            if (event->type() == QEvent::MouseButtonRelease
                && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton
                && _datumDragging)
            {
                datumDragCommit();
                _plot->setCursor(Qt::ArrowCursor);
                return true;
            }
        }
        //画布每帧绘制后同步图例覆盖层（R32）：色带矩形由引擎在 GL 绘制期现算
        //（Colormap::getLegendRect 为 paintGL 写入的缓存），过滤器先于本次
        //Paint 收到事件，故用 Queued 排到本次 Paint 处理之后重读——新建画布
        //首帧避让即正确；refreshLegend 内部脏键比对，相机旋转期色带矩形不变，
        //零变化不产生多余重绘。同一 Queued 内注释项随相机重投影（R42 修订：
        //视角旋转/缩放/平移后锚点像素位随动——相机变化必触发 GL 重绘，Paint
        //即重投影时机；矩阵现读与绘制同一状态）
        if (obj == _plot && event->type() == QEvent::Paint)
        {
            QMetaObject::invokeMethod(this, [this]()
            {
                if (_plot && isVisible())
                {
                    if (_legendOverlay)
                        _legendOverlay->refreshLegend(
                            _plot->getColormap()->getLegendRect());
                    layoutAnnotationItems();   //注释项随相机重投影（R42 修订）
                    //吸附标记随相机重投影（R46；hover 态命中点数据坐标现存，
                    //null 入参=按现态重投影；模式外/无命中=内部收场隐藏）
                    if (_annotation3DMode && _snapValid)
                        updateSnapMarker(true);
                    repositionSeekMarker();    //寻点标记随相机重投影（R47；同一
                                               //重摆通道，视角旋转/缩放随动）
                }
            }, Qt::QueuedConnection);
        }
        return QWidget::eventFilter(obj, event);
    }

    void m3DChartView::rebuild()
    {
        //B03 生命周期口径：不可见跳过整次重建、showEvent 补建。视口不可见时
        //新建画布只会得到"未首显即 addItem/setData/update"的引擎调用序（无
        //GL 上下文跑序列建链、对隐藏画布连发 update()），且隐藏期画布几何无谓
        //翻动——挂起至重显一次性补建，渲染口径与可见期全量重建完全一致
        if (!isVisible())
        {
            _rebuildPending = true;
            return;
        }
        _rebuildPending = false;

        //曲线→序列映射随场景整体换代（画布销毁重建，旧映射条目全部失效，
        //rebuild 开头清空——悬挂防线靠换代而非逐条摘除；增量样式刷新只消费
        //本代映射）。R38 曲面映射同口径一并清空；R41 基准面映射同口径
        _lineSeries.clear();
        _surfaceSeries.clear();
        _datumPlanes.clear();

        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller ? controller->document() : nullptr;
        CurveNode* chart = doc ? doc->node(_chartId) : nullptr;
        if (!chart || chart->type != CurveNodeType::Chart || chart->chart.chartType != 5)
            return;

        //本图表参与渲染的曲线（树序，可见性过滤单一收集点）：过滤后列表同
        //源供 unionAxisRange（autoFit 范围+色带 z 范围）与下方系列创建循环，
        //隐藏曲线既不建系列也不参与范围统计
        const QList<CurveNode*> curves = collectRenderCurves(doc, _chartId);

        //整视图重建：删旧画布（scene 随之销毁，无残留）建新画布，规避
        //场景残留/GL 状态累积。B03 拆除顺序：先从布局摘除+hide 再 deleteLater
        //——QOpenGLWidget 隐藏即同步失效（FBO 释放、Qt 不再向隐藏 widget
        //投递 paint/resize），旧画布在 deleteLater 等待期不再参与布局几何
        //（否则新旧两画布并存对半挤动几何、已初始化画布在失效态收到 resize，
        //命中 Qt5.13 qopenglwidget.cpp recreateFbo 族 "ctx && fbo" 断言窗口）
        if (_plot)
        {
            layout()->removeWidget(_plot);
            _plot->hide();
            _plot->deleteLater();
            _plot = nullptr;
        }
        _plot = new Qt3DPlotWidget(this);
        layout()->addWidget(_plot);
        _plot->installEventFilter(this);   //画布每帧 Paint 同步图例+注释项重投影（eventFilter）
        //鼠标跟踪（R46 问题4）：无按键 hover move 投递到本视图过滤器（吸附高
        //亮拾取用；模式内该路径被消费不上送引擎，引擎语义零变化——见
        //eventFilter 注释），带按键 move 照常进引擎
        _plot->setMouseTracking(true);
        //三维注释模式跨 rebuild 保持（画布换代重设十字光标；模式内左键截获在
        //eventFilter，对新画布自动生效）
        if (_annotation3DMode)
            _plot->setCursor(Qt::CrossCursor);

        //逐曲线建序列（严格调用规范：先 addItem 借 GL context，再 setData）。
        //样式口径迁自 T2 弹窗实现：曲线=模型线色+模型线宽（R33 线宽接模，
        //原 1.5f 硬编码退役）；曲面=Cool 色带；
        //瀑布=Jet 色带+RibbonWidth 5；柱状=Rainbow 色带+柱底宽按唯一值点距
        //0.85（退化兜底 0.5）。混合类型图表色带取首条曲线口径、范围=并集 z
        //可见曲线逐轴数据并集（autoFit 范围与色带 z 范围共用；隐藏曲线不
        //参与统计，与 2D fit 只统计可见曲线同口径）。
        //R43 刻度预变换（宿主侧，引擎零改动）：任一轴刻度非 0 时系列数据与
        //轴范围走变换空间——逐曲线变换副本仅用于 setData/统计（模型不动），
        //三轴全 0 快路径零变化（renderVecs 空=直接浅共享原数据）。轴组生效
        //范围经 axis3dRenderData 单一实现（与 refreshDisplayRangeCache 缓存
        //镜像同源；线性=旧口径逐位一致）
        int st[3] = { 0, 0, 0 };
        QList<Axis3DRenderVecs> renderVecs;
        double uTMin[3] = { 0.0, 0.0, 0.0 };
        double uTMax[3] = { 0.0, 0.0, 0.0 };
        double effMin[3] = { 0.0, 0.0, 0.0 };
        double effMax[3] = { 1.0, 1.0, 1.0 };
        axis3dRenderData(_chartId, st, &renderVecs, uTMin, uTMax, effMin, effMax);
        const bool scaled = st[0] != 0 || st[1] != 0 || st[2] != 0;
        //色带值域回落=原始 z 并集（口径不变，R43 备案：刻度类型不改色带值域
        //与云图值域——contourValueRange 同口径不动）；线性=变换并集即原始
        //并集零额外开销，刻度轴=对原始数据另行统计（未过滤旧口径）
        double ux0 = 0.0, ux1 = 0.0, uy0 = 0.0, uy1 = 0.0, uz0 = 0.0, uz1 = 0.0;
        if (scaled)
            unionAxisRange(curves, ux0, ux1, uy0, uy1, uz0, uz1);
        else
        {
            ux0 = uTMin[0]; ux1 = uTMax[0];
            uy0 = uTMin[1]; uy1 = uTMax[1];
            uz0 = uTMin[2]; uz1 = uTMax[2];
        }
        QString colormapName;
        bool firstSeries = true;
        try
        {
            const bool useVecs = !renderVecs.isEmpty();   //刻度轴=变换副本下发
            int vecIdx = 0;
            for (CurveNode* c : curves)
            {
                const QVector<double>& rx = useVecs ? renderVecs[vecIdx].x : c->series.xData;
                const QVector<double>& ry = useVecs ? renderVecs[vecIdx].y : c->series.yData;
                const QVector<double>& rz = useVecs ? renderVecs[vecIdx].z : c->series.zData;
                ++vecIdx;
                const std::vector<float> xsF = toFloats(rx);
                const std::vector<float> ysF = toFloats(ry);
                const std::vector<float> zsF = toFloats(rz);
                switch (c->series.plot3dType)
                {
                case 0:   //瀑布图
                case 2:   //曲面
                {
                    //网格行列：模型持久化的 rows/cols（rows*cols==点数）有效即用，
                    //无效（旧数据/异常）退化为 (N,1) 点序——曲面呈薄条带属数据缺
                    //形状的诚实呈现，不伪造栅格
                    int rows = c->series.plot3dRows;
                    int cols = c->series.plot3dCols;
                    if (rows <= 0 || cols <= 0
                        || rows * cols != static_cast<int>(xsF.size()))
                    {
                        rows = static_cast<int>(xsF.size());
                        cols = 1;
                    }
                    if (c->series.plot3dType == 0)
                    {
                        auto series = std::make_shared<WaterfallSeries>();
                        _plot->addItem(series);
                        series->setData(xsF, ysF, zsF, rows, cols);
                        series->setRibbonWidth(5.0);
                        series->setFlat(false);
                        if (firstSeries) colormapName = QStringLiteral("Jet");
                    }
                    else
                    {
                        auto series = std::make_shared<Surface3DSeries>();
                        _plot->addItem(series);
                        series->setData(xsF, ysF, zsF, rows, cols);
                        //显示模式下发（R38）：模型 drawStyle 钳 0..2 直传引擎
                        //DrawStyle（同值语义）。顺序纪律：setData 先行——等高线
                        //VBO 在 setData 的 rebuildContours 已建，样式切换仅绘制
                        //期分支消费；缺省 0=引擎默认 StyleSurface 与旧渲染逐位
                        //一致零回归。登记映射供 kDrawStyle 增量消费（仿 R33
                        //_lineSeries，shared_ptr 防悬挂）
                        series->setDrawStyle(static_cast<DrawStyle>(
                            qBound(0, c->series.drawStyle, 2)));
                        _surfaceSeries.insert(c->id, series);
                        if (firstSeries) colormapName = QStringLiteral("Cool");
                    }
                    break;
                }
                case 3:   //柱状图
                {
                    float barW = static_cast<float>(meanStepUnique(c->series.xData) * 0.85);
                    float barH = static_cast<float>(meanStepUnique(c->series.yData) * 0.85);
                    if (barW < 0.01f) barW = 0.5f;
                    if (barH < 0.01f) barH = 0.5f;
                    auto series = std::make_shared<Bar3DSeries>();
                    _plot->addItem(series);
                    series->setData(xsF, ysF, zsF, barW, barH);
                    if (firstSeries) colormapName = QStringLiteral("Rainbow");
                    break;
                }
                default:  //1 三维曲线（默认）
                {
                    auto series = std::make_shared<Line3DSeries>();
                    series->setColor(c->series.color);
                    //线宽接模（R33）：硬编码 1.5f 退役——存量三维曲线渲染略
                    //变粗（默认 2.0），属接模的预期变化（需求备案）
                    series->setLineWidth(static_cast<float>(c->series.lineWidth));
                    //外观与着色接模（R36）：线型/符号四件/幕帘填充两件/云图
                    //两件直传模型值；符号大小=模型 1..100 → 世界缩放。引擎
                    //setter 均为成员写+标脏，下方 setData 重建时一次成型
                    series->setLineStyle(c->series.lineStyle);
                    series->setSymbolStyle(c->series.symbolStyle);
                    series->setSymbolColor(c->series.symbolColor);
                    series->setSymbolDensity(c->series.symbolDensity);
                    series->setSymbolSize(symbolWorldSize(c->series.symbolSize));
                    series->setAreaFill(c->series.areaFill);
                    series->setAreaFillOpacity(c->series.areaFillOpacity);
                    series->setContour(c->series.contour);
                    series->setContourComponent(c->series.contourVar);
                    _plot->addItem(series);
                    series->setData(xsF, ysF, zsF);
                    //记录映射供增量样式刷新（R33）：曲线id→序列；addItem 后
                    //序列已入场景，后续 setData 异常也不影响映射条目有效性
                    //（setColor/setLineWidth 为纯成员写，无 GL 依赖）
                    _lineSeries.insert(c->id, series);
                    if (firstSeries) colormapName = QStringLiteral("Jet");
                    break;
                }
                }
                firstSeries = false;
            }
        }
        catch (const std::exception&)
        {
            //渲染异常不产半成品：画布保留空场景（轴组仍下发），等待下次重建
        }
        catch (...)
        {
        }
        if (!colormapName.isEmpty())
        {
            _plot->setColormap(colormapName, 256);
            //云图值域优先（R36）：图内有云图开启的可见曲线时色带=其分量值域
            //（与线体渐变引擎端同一 u_MinZ/u_MaxZ 单一真源）；否则既有口径
            //=可见曲线 z 并集
            double cLo = 0.0, cHi = 0.0;
            if (contourValueRange(curves, cLo, cHi))
                _plot->getColormap()->setDataRange(
                    static_cast<float>(cLo), static_cast<float>(cHi));
            else
                _plot->getColormap()->setDataRange(
                    static_cast<float>(uz0), static_cast<float>(uz1));
        }

        //基准面重放（R41，T41-3 接线）：本图表 DatumPlane3D 子节点树序逐个建
        //引擎图元——单基准 kVisible 且图表级 kDatum3DVisible 均为真才建项（隐藏
        //基准/总开关关闭不建项，零特判路径）。调用规范=先 addItem 借 GL 上下文
        //再下发属性（序列族同款；DatumPlane3D 全部 setter 为成员写+标脏，draw
        //前按下方 scene setCoordinateLimits 当前盒重建几何——本块位于该调用
        //之前，limits 广播可及）。插入次序=序列之后（场景按插入次序绘制：轴
        //背景→序列[幕帘填充]→基准面；多半透明图元间无深度排序、按插入次序
        //混合=T41-1 评审备案③，基准面最后入场景=叠在幕帘之上；深度测试保留=
        //前景不透明序列正确遮挡基准面）。消费侧 qIsFinite 防御（T41-2 评审建议
        //④，必做）：kDatumOpacity 非有限值（脏流 NaN 可穿透模型层 qBound）回退
        //出厂默认 0.25，位置三键同查回退 0——其余 double 键消费处即此两处。
        //kDatumType/kReverseClip 本期零渲染消费（字段入模保留，需求备案）。
        //标签=text 空取本名（kName 类型守卫见 rebuildRelevant）；基准面走全量
        //rebuild 刷新（面数少重建廉价，不做增量样式分支——保视角机制 rebuild
        //首尾捕获恢复自然生效）。与 R42 注释画布项共存：注释项为 QWidget 覆盖层
        //不在 GL 场景次序内，基准面×注释框×幕帘三体同屏=GL 半透明混合+覆盖层
        //叠加（回归像素断言覆盖）
        if (chart->chart.datum3DVisible)
        {
            try
            {
                for (int childId : doc->childNodeIds(_chartId))
                {
                    CurveNode* c = doc->node(childId);
                    if (!c || c->type != CurveNodeType::DatumPlane3D
                        || !c->datumPlane3D.visible)
                        continue;
                    const CurveDatumPlane3DData& d = c->datumPlane3D;
                    const int axis = qBound(0, d.normalAxis, 2);
                    const double coords[3] = {
                        qIsFinite(d.posX) ? d.posX : 0.0,
                        qIsFinite(d.posY) ? d.posY : 0.0,
                        qIsFinite(d.posZ) ? d.posZ : 0.0 };
                    //R43：基准面位置=原始模型值，按法向轴刻度变换到渲染空间再
                    //下发（与数据/轴范围同空间；线性恒等零变化）。变换 NaN
                    //（位置 ≤0 于 log/dB 轴）→ 跳过该面（防御，不崩）
                    const double posT = mCurveChartView::transformValue(
                        coords[axis], st[axis], 1.0);
                    if (!qIsFinite(posT))
                        continue;
                    auto plane = std::make_shared<DatumPlane3D>();
                    _plot->addItem(plane);   //先入场景借 GL 上下文
                    plane->setNormalAxis(axis);
                    plane->setPosition(posT);   //参考平面只消费法向轴坐标
                    plane->setColor(d.color);
                    plane->setOpacity(static_cast<float>(
                        qIsFinite(d.opacity) ? qBound(0.0, d.opacity, 1.0) : 0.25));
                    plane->setLabel(d.text.isEmpty() ? c->name : d.text);
                    plane->setLabelAnchor(qBound(0, d.labelPos, 8));
                    _datumPlanes.insert(c->id, plane);
                }
            }
            catch (const std::exception&)
            {
                //渲染异常不产半成品（已建项随下次 rebuild 换代，同序列口径）
            }
            catch (...)
            {
            }
        }

        //轴组：逐轴生效范围经 axis3dRenderData 单一实现（autoFit=变换空间
        //并集 / 手动=模型值按轴刻度变换+单侧 NaN 并集兜底；effMin/effMax 在
        //开头已算好——与 refreshDisplayRangeCache 缓存镜像同源）；统一走
        //setCoordinateLimits（多曲线按轴范围归一化，单曲线=轴范围恰为数据
        //范围，观感同旧口径）+ axis setRange（刻度数字=变换空间值，对齐二维
        //口径）。R43：labels/tickCounts/gridVisible 不变（恒等语义，三维页无
        //参考值/计权等附加变换）
        const CurveAxis3DState& ax = chart->chart.axis3d.axes[0];
        const CurveAxis3DState& ay = chart->chart.axis3d.axes[1];
        const CurveAxis3DState& az = chart->chart.axis3d.axes[2];
        const double minX = effMin[0];
        const double maxX = effMax[0];
        const double minY = effMin[1];
        const double maxY = effMax[1];
        const double z0 = effMin[2];
        const double z1 = effMax[2];
        _plot->scene()->setCoordinateLimits(
            static_cast<float>(minX), static_cast<float>(maxX),
            static_cast<float>(minY), static_cast<float>(maxY),
            static_cast<float>(z0), static_cast<float>(z1));
        _plot->scene()->axis()->setLabels(ax.label, ay.label, az.label);
        _plot->scene()->axis()->setRange(
            static_cast<float>(minX), static_cast<float>(maxX),
            static_cast<float>(minY), static_cast<float>(maxY),
            static_cast<float>(z0), static_cast<float>(z1));
        _plot->scene()->axis()->setTickCounts(
            qMax(2, ax.tickCount), qMax(2, ay.tickCount), qMax(2, az.tickCount));
        //R44 网格样式/背景两色：有效旗标（线框=三轴全关复用引擎关断语义）+
        //恒下发两色——与 kChart3D* 增量分流同一实现（applyBackdropStyle 单一
        //真源）；缺省三维框+两色缺省=原引擎硬编码等效，缺省观感零变化
        applyBackdropStyle(_plot, chart->chart.axis3d);

        //数据就绪后相机适配（渲染端按轴盒归一化，fit 默认盒即可）
        _plot->camera()->fitToView();
        _plot->update();

        //图例覆盖层重放（R32）：rebuild 尾统一刷新——曲线增删/轴键等整场景
        //重建后图例随 rebuild 正确重放，新建视图首显即有图例。raise 保证
        //覆盖层在新建画布之上（子控件后建者在上）；几何同步兜底（首建早于
        //首个 resizeEvent 时）。色带矩形此刻为空矩形（新画布尚未 paintGL），
        //首帧避让由 eventFilter 的 Queued 同步在画布首绘后补正
        if (_legendOverlay)
        {
            _legendOverlay->setGeometry(rect());
            _legendOverlay->raise();
            _legendOverlay->refreshLegend(_plot->getColormap()->getLegendRect());
        }

        //注释项重放（R42 二期）：按文档树序重建全部注释项（可见=图表
        //annotationsVisible 且注释 kVisible；总开关关闭/无注释→零项不崩溃）。
        //注释项为视图子控件（非画布子控件），rebuild 整画布换代不受影响，此处
        //集合级重建保证"新建视图首显即有注释/增删随结构信号重建后重放"；
        //raise 在图例覆盖层之后（注释项叠画布之上且最上层）。显示范围缓存随
        //rebuild 尾刷新（与本次 setCoordinateLimits 同一取值）
        rebuildAnnotationItems();
        //寻点标记重放（R47）：标记存 curveId+pointIndex（视图态，状态不受
        //rebuild 影响），覆盖层为视图子控件整画布换代不受影响；此处按刷新后
        //的显示范围缓存重投影一次（新建画布首显/场景重建后标记照常在）
        repositionSeekMarker();
    }

    bool m3DChartView::queryAxis3DDisplayRange(int chartId, double outMin[3], double outMax[3])
    {
        if (!outMin || !outMax)
            return false;
        mCurveDocController* controller = mCurveDocController::getInstance();
        CurveDocument* doc = controller ? controller->document() : nullptr;
        CurveNode* chart = doc ? doc->node(chartId) : nullptr;
        if (!chart || chart->type != CurveNodeType::Chart || chart->chart.chartType != 5)
            return false;

        //本图表可见曲线（树序）——与 rebuild 共用 collectRenderCurves 单一
        //收集实现（可见性过滤镜像同口径，属性页回显=渲染取景）
        const QList<CurveNode*> curves = collectRenderCurves(doc, chartId);

        //R43 契约保持"原始数据空间"（属性页回显/退出自动回写依赖，零 UI
        //改动）：并集统计前按"整点可变换"过滤——任一分量在其轴刻度下变换
        //NaN（原始值 ≤0/非有限）→ 整点排除（镜像 unionAxisRange 非有限整点
        //排除先例），返回存活点的**原始值**并集；线性轴过滤恒真零变化
        const CurveAxis3DState* axes = chart->chart.axis3d.axes;
        const int st[3] = { qBound(0, axes[0].scaleType, 3),
            qBound(0, axes[1].scaleType, 3), qBound(0, axes[2].scaleType, 3) };
        double ux0 = 0.0, ux1 = 0.0, uy0 = 0.0, uy1 = 0.0, uz0 = 0.0, uz1 = 0.0;
        bool any = false;
        if (st[0] != 0 || st[1] != 0 || st[2] != 0)
        {
            //存活点原始值副本（过滤进 copies，统计进核心单一实现）
            QList<QVector<double>> fxs, fys, fzs;
            QList<const QVector<double>*> px, py, pz;
            px.reserve(curves.size());
            py.reserve(curves.size());
            pz.reserve(curves.size());
            for (CurveNode* c : curves)
            {
                QVector<double> cx, cy, cz;
                const int nPts = qMin(c->series.xData.size(),
                    qMin(c->series.yData.size(), c->series.zData.size()));
                cx.reserve(nPts);
                cy.reserve(nPts);
                cz.reserve(nPts);
                for (int i = 0; i < nPts; ++i)
                {
                    const double x = c->series.xData.at(i);
                    const double y = c->series.yData.at(i);
                    const double z = c->series.zData.at(i);
                    if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(z))
                        continue;
                    if ((st[0] != 0 && !qIsFinite(mCurveChartView::transformValue(x, st[0], 1.0)))
                        || (st[1] != 0 && !qIsFinite(mCurveChartView::transformValue(y, st[1], 1.0)))
                        || (st[2] != 0 && !qIsFinite(mCurveChartView::transformValue(z, st[2], 1.0))))
                        continue;   //整点不可变换 → 整点排除（镜像非有限排除先例）
                    cx.append(x);
                    cy.append(y);
                    cz.append(z);
                }
                fxs.append(cx);
                fys.append(cy);
                fzs.append(cz);
                px.append(&fxs.last());
                py.append(&fys.last());
                pz.append(&fzs.last());
            }
            any = unionAxisCore(px, py, pz,
                ux0, ux1, uy0, uy1, uz0, uz1);
        }
        else
            any = unionAxisRange(curves, ux0, ux1, uy0, uy1, uz0, uz1);
        //无曲线/无可变换存活数据=查询失败，由调用方按模型休眠值回退
        //（属性页取消 autoFit 的兜底口径）
        if (!any)
            return false;

        //逐轴当前生效范围（原始空间，口径不变）：autoFit=存活点原始并集 /
        //手动=模型 min/max；退化轴兜底 max+=1 与 rebuild 轴组下发同口径
        const double uMin[3] = { ux0, uy0, uz0 };
        const double uMax[3] = { ux1, uy1, uz1 };
        for (int i = 0; i < 3; ++i)
        {
            const double lo = axes[i].autoFit ? uMin[i] : axes[i].min;
            double hi = axes[i].autoFit ? uMax[i] : axes[i].max;
            if (hi - lo < 1e-4)
                hi = lo + 1.0;
            outMin[i] = lo;
            outMax[i] = hi;
        }
        return true;
    }
}
