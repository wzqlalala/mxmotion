#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图视图（CurveView，chartType=5，Plot3D 渲染管线；R28 入树）
//
// 绑定三维图节点（mCurveViewportFactory::materializeChart 按
// RenderKind::Plot3D 分流创建），以曲线文档为唯一事实源全量重建：
//   - 曲线节点：按 series.plot3dType 建 Plot3D Series
//     （0 瀑布/1 三维曲线/2 曲面/3 柱状），严格调用规范=先 addItem 借 GL
//     context 再 setData；线色取 series.color，柱宽/色带口径迁自 T2 弹窗实现
//   - 轴组（chart.axis3d，曲线下 x/y/z 壳经重定向编辑）：
//     label→setLabels、范围→setRange、tickCount→setTickCounts、
//     gridVisible→setGridVisible（逐轴）
//   - autoFit（默认开）：范围=可见曲线数据并集现算（追加曲线自动扩；
//     隐藏曲线 series.visible=false 不建系列、不参与范围/色带统计，T8 接通）；
//     取消=series 范围用 axis3d 手动 min/max
//   - scene()->setCoordinateLimits(轴范围)：多曲线统一按轴范围归一化——
//     Plot3D 序列默认按各自数据范围归一化（各自充满盒），共轴多曲线必须
//     统一到轴范围才不重叠失真（引擎既有机制，DemoApp 同款）
//   - 图例（R32）：宿主侧自绘覆盖层 m3DChartLegendOverlay（子控件叠于画布
//     之上，鼠标穿透；绘制口径对齐 2D applyLegend/mPlotLegendItem）——
//     图例专属键（Legend 节点 12 项参数栏键，含曲线名改名）仅刷覆盖层，
//     禁止整场景重建（相机状态不丢）；曲线增删/数据直写/轴键仍走整场景
//     重建，重建尾覆盖层随 rebuild 重放（新建视图首显即有图例）。
//   - 曲线样式增量刷新（R33）：曲线节点的 kColor/kLineWidth 提交经
//     CurveId→Line3DSeries 映射（rebuild 建序列时记录）直接 setColor/
//     setLineWidth（绘制期消费，改成员下一帧自然生效，无需重建 GPU 缓冲）
//     + update()——保视角增量刷新，不整场景重建（kVisible 仍走全量重建：
//     隐藏曲线不建系列、并集范围/色带随可见集变化）。不可见期不碰画布，
//     改走 scheduleRebuild（B03 口径：_rebuildPending 挂起、showEvent 补跑
//     全量重建重放正确样式），语义闭合无丢样式窗口。
//   - 色带类显示模式（R38）：曲面序列（plot3dType==2）rebuild 建
//     Surface3DSeries 后 setData 之下发 setDrawStyle（模型 drawStyle，等高线
//     VBO 在 setData 时已建，样式绘制期即时生效）；曲线id→Surface3DSeries
//     映射（仿 R33 _lineSeries，shared_ptr 防悬挂）+ 样式增量分流纳入
//     kDrawStyle（plot3dType==2 专享；引擎 setter 自带 update，宿主不再手动
//     刷画布）。缺省 0=引擎默认 StyleSurface 与旧渲染逐位一致零回归。
//   - 画布注释（R42 二期建、同日用户拍板修订为三维数据空间锚点）：每个可见
//     注释一个 m3DChartAnnotationItem（视图子控件叠画布之上），锚点=posX/
//     posY/posZ 三维数据坐标，按相机投影（proj*view）落到画布像素、随视角
//     旋转/缩放实时重投影（画布每帧 Paint 后统一重摆）；注释专属键分流仅重
//     投影/重绘注释项（禁止整场景重建，相机不丢）；{X}/{Y}/{Z} 模板 token=
//     注释自身锚点数据坐标。拖拽=屏幕位移经反投影（过锚点、视向为法向的相
//     机平行面求交）回写三键。增删随既有结构信号 scheduleRebuild→rebuild 尾
//     重放。
//   - 基准面（R41）：本图表 DatumPlane3D 子节点（visible 且图表
//     kDatum3DVisible 为真）rebuild 建引擎 DatumPlane3D 图元（半透明面+边框+
//     billboard 标签，T41-1）——先 addItem 借 GL 上下文再下发属性（法向/位置
//     [法向轴对应坐标]/颜色/不透明度/标签(text 空取本名)/锚点），盒范围由
//     scene setCoordinateLimits 统一下发（图元覆写标脏、draw 前按当前盒重建）。
//     插入次序=序列之后（多半透明图元按插入次序混合，T41-1 备案③）；隐藏
//     基准/总开关关闭不建项；基准面键集（含 kName 类型守卫）走全量 rebuild
//     刷新（面数少重建廉价），消费侧 opacity/位置 qIsFinite 防御（脏流 NaN
//     回退出厂默认）。与 R42 注释画布项共存（注释项为 QWidget 覆盖层不在 GL
//     场景次序内）。
//   - 网格样式/背景面配色（R44，图表级）：rebuild 轴组段按图表 gridStyle 重算
//     有效旗标——线框=三轴全关（宿主侧复用引擎 setGridVisible"关=该轴面填充
//     与网格都不画"语义=仅轴线/刻度/数字，引擎零新 API）；三维框=按各轴模型
//     gridVisible 原样；并恒下发 setBackdropFillColor/setBackdropGridColor
//     （T44-1 引擎 setter，成员绘制期消费，代价为零；默认=原硬编码等效，
//     缺省观感零变化）。三键（kChart3DGridStyle/kChart3DBackdropColor/
//     kChart3DBackdropGridColor）保视角增量刷新（R33 先例）：可见期重算旗标+
//     两色 setter+update()（引擎 setter 均绘制期消费，相机不丢），白名单不
//     追加三键防双触发；不可见期 scheduleRebuild 挂起（B03 口径）。
//   - 基准面鼠标拖动（R48）：常态（注释模式外）画布左键按下先宿主解析拾取
//     基准面（引擎 DatumPlane3D 非拾取图元——按法向轴位置四边形投影到画布
//     做点包含测试，树序最后命中优先=后绘制在上），命中即截获（引擎不进入
//     旋转态），拖动=像素差分反投影（过模型提交位、视向为法向的相机平行面，
//     与注释拖拽同一 seam）取法向轴分量实时预览（引擎 setPosition 成员写+
//     update，文档不动、相机不动），松开一次宏"移动基准面"提交法向轴对应
//     坐标键（与模型现值比对无变化不发）；悬停基准面显 SizeAll 光标反馈
//     （消费 move 事件——引擎 cursorChanged 恒发 Arrow 会覆盖，非命中照常
//     放行引擎拾取/图例光标；色带图例矩形内不抢事件=引擎图例拖动优先）。
//     配套基准面属性保视角增量分流（datumPropRelevant/applyDatumIncremental，
//     R33/R38 先例）：提交触发不再走全量 rebuild（rebuild 尾 fitToView 会
//     重置相机=拖拽松开视角跳变），按键映射引擎 setter（成员写+update）；
//     不可见期 scheduleRebuild 挂起（B03 口径）。kVisible（建/销图元）与
//     kDatumType/kReverseClip（零渲染消费）仍走全量 rebuild。
// 重建策略：整视图重建（删 Qt3DPlotWidget 重建再全量重放）——最简正确，
// 规避场景残留/GL 状态累积；LRU 卸载=视图析构，再物化=新视图重放，无损。
// 信号：sig_requestClose 与 mCurveChartView 同名同义（工厂按图表 id 关闭卡片）。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QWidget>
#include <QTimer>
#include <QHash>
#include <QList>
#include <QMatrix4x4>
#include <QVector2D>
#include <memory>

class Qt3DPlotWidget;
class Line3DSeries;   //Plot3D 引擎三维曲线序列（全局命名空间，仅 shared_ptr 成员用）
class Surface3DSeries;   //Plot3D 引擎曲面序列（R38 色带类显示模式消费，同上）
class DatumPlane3D;   //Plot3D 引擎基准平面图元（R41 基准面渲染消费，同上）

namespace PostCurve
{
    //图例覆盖层（R32，本头仅前向声明；完整类型见 m3DChartLegendOverlay.h）
    class m3DChartLegendOverlay;
    //画布注释项（R42 二期，本头仅前向声明；完整类型见 m3DChartAnnotationItem.h）
    class m3DChartAnnotationItem;

    class POSTCURVE_EXPORT m3DChartView : public QWidget
    {
        Q_OBJECT
    public:
        explicit m3DChartView(int chartId, QWidget* parent = nullptr);

        //当前生效轴范围查询（纯文档计算，不依赖视图实例——三维图无视口/未
        //物化时也可用；CurvePages 属性页 autoFit 回显消费，CurvePages→CurveView
        //单向依赖与 mCurveChartView::queryAxisDisplayRange 先例同向）：曲线
        //收集按 series.visible 过滤与 rebuild 镜像同口径（所见即所得），逐轴
        //autoFit=可见曲线数据并集现算 / 手动=模型 min/max，退化轴兜底 max+=1
        //与 rebuild 同口径。outMin/outMax 为三轴下标数组（0=X 1=Y 2=Z）；
        //图表不存在/非三维图/无可见曲线数据时返回 false
        static bool queryAxis3DDisplayRange(int chartId, double outMin[3], double outMax[3]);

        //--- 三维锚点投影/反投影 seam（R42 二期修订；annotest 可测） ---
        //数据坐标 ↔ 归一化盒坐标（BOX_SIZE=10 约定：数据→(v-min)/range*20-10
        //∈[-10,+10]³，引擎 Line3DSeries 同源；range 与 queryAxis3DDisplayRange
        //同源=渲染归一化同一 true 源）。精度约定：换算在双精度域进行，渲染端
        //归一化为 float——'g',6 位有效数字下两端一致（差异远小于末位）
        static double dataToNormalizedBox(double data, double axisMin, double axisMax);
        static double normalizedBoxToData(double world, double axisMin, double axisMax);
        //三维数据锚点 → 画布像素（dataToNormalizedBox 三轴入盒 → proj*view 四
        //分量除 w 得 NDC → 视口像素；本实例 _plot 相机矩阵/尺寸现读）。画布未
        //建/锚点在相机身后（w≤0）返回 false（注释项收场隐藏）
        bool projectDataToPixel(const double xyz[3], QPointF& outPx) const;
        //画布像素位移 → 三维数据坐标（拖拽反投影，像素差分法）：把"锚点当前
        //投影像素→目标像素"的 NDC 差量经 inverse(proj*view) 映射为世界盒位移叠
        //加到锚点。构造性保证 提交锚点的投影 ≡ 目标像素（project∘unproject 恒
        //等闭合，正交=精确仿射；透视=锚点处线性化），从根上消灭拖拽后闪回。
        //锚点投影失败/画布未建返回 false（调用方保持原值不提交）
        bool unprojectPixelThroughData(const QPointF& px, const double throughData[3],
            double outData[3]) const;

        //--- 三维图注释模式（R46，对齐二维注释模式交互） ---
        //模式开关（工厂 sig_annotation3DModeChanged 广播/新物化承接）：开启=
        //画布左键被本视图截获（不再进引擎相机控制器），点击拾取内容点创建
        //牵引线注释（kAttachMode=0）；内容外（吸附半径内无数据点）点击=创建
        //窗口注释（kAttachMode=1，无牵引线，对齐二维"曲线外=视图注释"口径）
        //——两类点击均创建且模式保持现状，可连续打点；关闭=恢复引擎相机交互
        void setAnnotation3DMode(bool on);
        bool isAnnotation3DMode() const { return _annotation3DMode; }

        //--- 寻点标记（R47，三维数值页寻点；视图态） ---
        //画布橙点（半径约 4px）+坐标文本 "X:%.3f Y:%.3f Z:%.3f" 覆盖层（全局
        //单标记，工厂持有 {chartId,curveId,pointIndex} 状态经此下发；
        //curveId<=0/pointIndex<0 = 清除）。标记数据坐标从文档现读（按 curveId
        //取 x/y/z 第 pointIndex 点，不缓存数据副本），随画布每帧 Paint 后既有
        //重摆通道（layoutAnnotationItems 同路）每帧重投影——视角旋转/缩放随
        //动；投影失败（相机身后/画布未建）或点失效（曲线删除/下标越界/任一
        //坐标非 finite）收场隐藏；rebuild 尾照常重放。标记为**视图态**：不进
        //命令栈、不写文档、不序列化、不触发 rebuild；覆盖层鼠标穿透
        //（WA_TransparentForMouseEvents，相机交互零影响）
        void setSeekMarker(int curveId, int pointIndex);

    signals:
        //预留对齐 mCurveChartView：卡片内关闭入口（当前三维图无画布内关闭钮，
        //卡片关闭走节点删除/视图树删除路径）
        void sig_requestClose();

    private:
        //全量重建：删旧 Qt3DPlotWidget 重建（scene 全新），按文档重放数据与轴；
        //不可见时跳过并置 _rebuildPending（B03 生命周期口径，见 cpp 注释）
        void rebuild();
        //0ms 合并重建（一次宏多信号只重建一次）
        void scheduleRebuild();
        //信号相关性：节点属于本图表（或即本图表）且属性键影响三维渲染。
        //R41 起基准面键集在本白名单（DatumPlane3D 节点×全键含 kName 类型守卫；
        //Chart 节点×kDatum3DVisible）——命中全量 rebuild 刷新（面数少重建廉价，
        //不做增量样式分支）；Curve kName 不在键表（图例覆盖层分流，防 R32 式
        //冗余重建）
        bool rebuildRelevant(int nodeId, const QString& prop) const;
        //图例专属键分流（R32；评审 R1 分立结构）：命中=仅刷覆盖层，禁止整场
        //景重建（改图例颜色不应闪重建、丢相机状态）。两条分立分支、互不放行
        //对方键：①本图表 Legend 子节点 × 12 项参数栏图例键；②本图表 Curve
        //子节点 × kName（图例条目文本=曲线本名，改名同步覆盖层）。与
        //rebuildRelevant 互斥（后者对 Legend 节点属性键返回 false，键表亦无
        //kName——Curve 改名自然不触发重建）
        bool legendPropRelevant(int nodeId, const QString& prop) const;
        //曲线样式键分流（R33 两键；R36 扩至外观与着色全键集；R38 增 kDrawStyle
        //色带类专享）：本图表 Curve 子节点 × {kColor, kLineWidth, 线型/符号四件/
        //填充两件/云图两件, kDrawStyle}。与 legendPropRelevant/rebuildRelevant
        //三者互斥（后者键表不含这些键，防双触发）；命中走 applyStyleIncremental，
        //永不落入整场景重建
        bool styleRefreshRelevant(int nodeId, const QString& prop) const;
        //注释专属键分流（R42 二期）：本图表 Annotation 子节点×{kText,kAlign,
        //kColor,kFont,kBgColor,kBorderWidth,kBorderShow,kVisible,kPosX,kPosY,
        //kPosZ,kName} ∪ 本图表 Chart 节点×kAnnotationsVisible → 仅重摆/重绘注
        //释项，禁止整场景重建（相机不丢）。kPosZ 缺失即实机"拖拽后闪回"根因
        //（三键提交 z 单键不刷新，项停旧 z 投影位，探针实证）。与
        //legendPropRelevant/styleRefreshRelevant/rebuildRelevant 四分支互斥；
        //判定序置于前两者之后、rebuildRelevant 之前（在 sig_nodePropertyChanged
        //处理链内 return 隔离，rebuildRelevant 白名单不追加这些键防双触发，R33
        //教训）
        bool annotationPropRelevant(int nodeId, const QString& prop) const;
        //基准面属性分流（R48）：本图表 DatumPlane3D 子节点×{kPosX,kPosY,kPosZ,
        //kNormalAxis,kColor,kDatumOpacity,kText,kLabelPos,kName}——命中走
        //applyDatumIncremental 保视角增量刷新，禁止整场景重建（拖拽/属性页改
        //基准位置不应重置相机）。与 annotationPropRelevant/rebuildRelevant 互斥
        //（判定序在注释分流之后；rebuildRelevant 基准键表已剪除这些键防双触发，
        //R33 教训）；kVisible 建销图元与 kDatumType/kReverseClip 零消费键不在
        //本分支（仍走 rebuild 既有路径）
        bool datumPropRelevant(int nodeId, const QString& prop) const;
        //保视角增量样式刷新（样式键分流命中且可见时调用）：按模型现值下发对应
        //序列 setter（曲线类键→Line3DSeries + update()；kDrawStyle→
        //Surface3DSeries，引擎 setter 自带 update）；kColor 另刷图例覆盖层
        //（条目图标=series.color 现读，须同步）。映射无该 id（隐藏曲线/非对应
        //类型序列）安全短路；不可见期由调用方改走 scheduleRebuild 挂起
        void applyStyleIncremental(int nodeId, const QString& prop);
        //图表级网格样式/背景配色保视角增量刷新（R44）：kChart3DGridStyle/
        //kChart3DBackdropColor/kChart3DBackdropGridColor 命中且可见时调用——
        //按模型现值重算有效旗标（线框=三轴全关）+setGridVisible+两色 setter
        //+update()（引擎三 setter 均成员写、draw 期消费，相机不丢）。取值口径
        //与 rebuild 轴组段同一实现（applyBackdropStyle 单一真源）
        void applyChart3DStyleIncremental();
        //基准面保视角增量刷新（R48）：datumPropRelevant 命中且可见时调用——
        //按模型现值下发对应 DatumPlane3D setter（全部成员写+标脏，draw 期重建
        //几何/标签，相机不丢）；位置键=法向轴坐标按轴刻度变换后 setPosition
        //（与 rebuild 下发同口径），kName/kText 合流标签=text 空取本名。映射无
        //该项（隐藏基准/总开关关=未建图元）安全短路；不可见期由调用方改走
        //scheduleRebuild 挂起
        void applyDatumIncremental(int nodeId, const QString& prop);
        //首显/重显补建：不可见期挂起的重建在此补跑
        void showEvent(QShowEvent* event) override;
        //覆盖层随视图几何铺满（与画布同矩形：布局 0 边距）
        void resizeEvent(QResizeEvent* event) override;
        //画布每帧 Paint 后同步图例覆盖层（色带矩形 GL 侧现算，Queued 在本次
        //Paint 之后重读——首帧避让即正确；见 cpp eventFilter 注释），同一 Queued
        //内注释项随相机重投影（视角旋转/缩放后锚点像素位随动）
        bool eventFilter(QObject* obj, QEvent* event) override;

        //--- 注释项集合（R42 二期） ---
        //rebuild 尾重放：按文档树序重建全部注释项（可见注释=图表 annotationsVisible
        //且注释 kVisible；总开关关闭/无注释→零项不崩溃）。旧项 hide+deleteLater
        //防悬挂（列表立即换代，文档信号不引用旧项）；同时刷新显示范围缓存
        void rebuildAnnotationItems();
        //注释专属键命中（可见期）单项刷新；列表无此项（隐藏翻可见等）回落集合级重建
        void refreshAnnotationItem(int nodeId);
        //全部注释项按模型锚点现投影重摆（宿主 resizeEvent 与画布每帧 Paint 后
        //调用；相机矩阵/视口尺寸变化后像素位随动，模型值不变）
        void layoutAnnotationItems();
        //轴显示范围缓存刷新（rebuildAnnotationItems 内调用；数据↔盒坐标换算的
        //true 源，与 rebuild setCoordinateLimits 同一取值口径=渲染归一化同一
        //true 源；queryAxis3DDisplayRange 返回 false（无可见曲线）时同样缓存其
        //退化兜底值——rebuild 消费该值下发引擎，缓存必须镜像）
        void refreshDisplayRangeCache();
        //--- 三维注释模式内私有件（R46） ---
        //宿主侧内容拾取（注释打点/吸附高亮共用）：可见曲线逐采样点+**段中点/
        //单元中心**（曲线/瀑布=相邻采样对中点、曲面=栅格单元四角平均——稀疏
        //曲线/粗网格曲面"段中/面中"点击不再漏检，对齐二维吸附"曲线上任意位置"
        //语义）投影（proj*view 现读一次+逐点矩阵乘），取点击位最近候选点（吸附
        //半径 30px，对齐二维 snapper 阈值）；命中返回 true 并回传**候选点原始
        //数据坐标**（插值点）。引擎选区缓冲拾取（离屏 ID 渲染）为引擎私有路径
        //不可达（R42 拆除 MouseMove 跟踪后 requestPicking 无触发源），且需改引
        //擎公开 API——宿主投影拾取与二维"最近点吸附"语义对齐，零引擎改动
        bool pickDataPoint(const QPoint& canvasPos, double outData[3]) const;
        //点击创建牵引线注释（模式内画布左键单击命中内容时调用）：宏"添加注释"
        //内 addAnnotation+kAttachMode=0（3D 语义=内容锚定带牵引线）+kText=沿用
        //本图表最近注释模板（无则默认三维模板）+kPosX/kPosY/kPosZ=拾取点原始
        //坐标（框位三键缺省 NaN=框在锚点+固定偏移）；创建后 setCurrentNodeId
        void createAnnotationAt(const QPoint& canvasPos);
        //点击创建窗口注释（模式内画布左键单击未命中内容时调用，对齐二维"曲线
        //外点击=视图注释"口径）：像素反投影到"过显示范围中心、视向为法向"的
        //相机平行面得三维数据坐标，宏"添加注释"内 addAnnotation+kAttachMode=1
        //（窗口=框左上角即锚点，无牵引线）+kText="新注释"+锚点三键=反投影点；
        //创建后 setCurrentNodeId（树回选+参数栏换绑）
        void createWindowAnnotationAt(const QPoint& canvasPos);
        //本图表注释模板源（沿用口径对齐二维 templateSourceAnnotation）：树序
        //最后一个含模板 token '{' 的注释文本；无则返回三维默认模板
        QString currentAnnotationTemplate() const;
        //吸附高亮标记同步（R46 问题4，模式内 hover 反馈）：showIt 且投影成功
        //→标记定位到 point（原始数据坐标）投影位显形；否则收场隐藏。point 供
        //相机 Paint 同步链重投影（视角旋转/缩放随动），nullptr=仅按现态重投影
        void updateSnapMarker(bool showIt, const double point[3] = nullptr);
        //寻点标记重投影（R47；setSeekMarker/resizeEvent/画布 Paint 后重摆通道/
        //rebuild 尾四路汇聚）：按文档现读标记点数据坐标（曲线已删/下标越界/
        //任一坐标非 finite → 失效）→ projectDataToPixel → 覆盖层重摆显形；
        //任一步失败覆盖层收场隐藏（安全清除不崩溃）
        void repositionSeekMarker();
        //--- 基准面鼠标拖动（R48，常态交互非注释模式专属） ---
        //宿主解析拾取（引擎 DatumPlane3D 非拾取图元零改动）：画布像素处命中
        //可见基准面（图表级总开关×单基准 kVisible 双开才参与）返回 true 并回
        //传基准节点 id。判定=按法向轴位置四边形（盒空间另两轴满幅 [-10,10]）
        //经 proj*view 投影成画布凸四边形做点包含测试；树序迭代取最后命中
        //（=后绘制在上，对齐场景半透明混合次序）；色带图例矩形内不参与（引擎
        //图例拖动优先）
        bool hitTestDatumPlane(const QPoint& canvasPos, int& outDatumId) const;
        //拖动预览（左键捕获期 move 逐次调用）：像素差分反投影（过模型提交位、
        //视向为法向的相机平行面——拖拽期模型不动=固定差分基准，与注释拖拽同
        //一 seam）取法向轴分量、钳入显示范围换回渲染空间值，引擎 setPosition
        //实时预览+update()。全程不动文档（相机不动、无重建）；反投影失败/图元
        //或节点失效（拖拽中场景换代/基准删除）安全终止捕获
        void datumDragMove(const QPoint& canvasPos);
        //松开提交（左键释放且捕获中调用）：预览位（图元 position 现读）逆变换
        //回原始数据值，与模型现值比对无变化不发（拖回原位/原地点击不压空操作
        //宏）；有变化=宏"移动基准面"单键提交法向轴对应坐标键（一次 Ctrl+Z 整
        //体撤销；提交信号经增量分流回写同值图元=视觉零跳变，页面属性行随文档
        //信号回显）
        void datumDragCommit();

        int _chartId = -1;             //绑定的三维图节点id
        Qt3DPlotWidget* _plot = nullptr;   //Plot3D 画布（每次 rebuild 整体重建）
        QTimer* _rebuildTimer = nullptr;   //0ms 合并重建
        bool _rebuildPending = false;      //不可见期挂起的重建（showEvent 补跑）
        //图例覆盖层（R32）：视图子控件（非画布子控件）——rebuild deleteLater
        //重建画布时无需随建随销，几何/层级由 resizeEvent 与 rebuild 尾同步
        m3DChartLegendOverlay* _legendOverlay = nullptr;
        //三维曲线序列映射（R33）：曲线id → rebuild 建的 Line3DSeries（保视角
        //增量样式刷新用）。rebuild 开头 clear（画布销毁重建，映射随场景整体
        //换代；shared_ptr 防悬挂——引擎按 addItem 持有引用，容器侧引用与场景
        //生命周期解耦）；仅 plot3dType==1 分支记录（色带类序列无线色/线宽
        //消费，不入映射=其样式提交自然短路）
        QHash<int, std::shared_ptr<Line3DSeries>> _lineSeries;
        //曲面序列映射（R38，仿 R33 _lineSeries 同款口径）：曲线id → rebuild
        //建的 Surface3DSeries（kDrawStyle 保视角增量消费用）。rebuild 开头随
        //_lineSeries 一并 clear（画布销毁重建映射随场景整体换代，shared_ptr
        //防悬挂）；仅 plot3dType==2 分支记录（瀑布/柱状序列无样式 API 不入
        //映射=其 kDrawStyle 提交自然短路）
        QHash<int, std::shared_ptr<Surface3DSeries>> _surfaceSeries;
        //基准平面图元映射（R41）：基准节点 id → rebuild 建的 DatumPlane3D（键=
        //基准节点 id，非曲线 id）。rebuild 开头随 _lineSeries/_surfaceSeries 一并
        //clear（画布销毁重建，映射随场景整体换代；shared_ptr 防悬挂——引擎按
        //addItem 持有引用）。基准面属性变更走全量 rebuild 刷新（面数少重建廉价，
        //不做增量样式分支），本映射仅生命周期管理（随场景换代清空重放）
        QHash<int, std::shared_ptr<DatumPlane3D>> _datumPlanes;
        //画布注释项集合（R42 二期）：每个可见注释一项，视图子控件（非画布子
        //控件，rebuild 重建画布不受影响；rebuild 尾集合级重建重放）。项随视图
        //析构，列表换代=hide+deleteLater 防悬挂
        QList<m3DChartAnnotationItem*> _annoItems;
        //轴显示范围缓存（refreshDisplayRangeCache 于 rebuild 尾维护；数据↔盒
        //坐标换算真源，与渲染归一化 setCoordinateLimits 同一取值）。valid=
        //queryAxis3DDisplayRange 原样返回值（false=无可见曲线数据，取值为退化
        //兜底——rebuild 同样消费该值，投影换算照常可用）
        double _dispRangeMin[3] = { 0.0, 0.0, 0.0 };
        double _dispRangeMax[3] = { 1.0, 1.0, 1.0 };
        bool _dispRangeValid = false;
        //--- 三维注释模式状态（R46） ---
        bool _annotation3DMode = false;    //模式开关（工厂广播/新物化承接）
        bool _annoPressing = false;        //模式内画布左键按下捕获中（消费事件不进引擎）
        QPoint _annoPressPos;              //按下点（画布坐标；松开 ≤4px 判单击创建）
        //吸附高亮标记状态（R46 问题4，模式内 hover 反馈）：覆盖层同寻点标记
        //口径（视图子控件+懒创建+鼠标穿透），状态=最近 hover 命中的数据点
        //（相机 Paint 同步链据此重投影；invalid=无命中标记隐藏）
        QWidget* _snapOverlay = nullptr;   //吸附高亮圆点覆盖层（本 cpp 匿名类 SnapMarkerOverlay）
        bool _snapValid = false;           //当前 hover 是否命中内容点
        double _snapPoint[3] = { 0.0, 0.0, 0.0 };   //命中点原始数据坐标（重投影输入）
        //--- 寻点标记状态（R47） ---
        //覆盖层为视图子控件（非画布子控件，rebuild 整画布换代不受影响——R32
        //图例覆盖层/R42 注释项先例），懒创建（首次 setSeekMarker 才建）；几何/
        //层级由重摆通道与 rebuild 尾同步
        int _seekCurveId = -1;             //标记所在曲线节点 id（-1=无标记）
        int _seekPointIndex = -1;          //标记点下标（-1=无标记）
        QWidget* _seekOverlay = nullptr;   //橙点+坐标文本覆盖层（本 cpp 匿名类 SeekMarkerOverlay）
        //--- 基准面拖动状态（R48，视图交互态不进文档） ---
        bool _datumDragging = false;       //左键捕获拖拽中（事件被本视图消费不进引擎）
        int _dragDatumId = -1;             //捕获中的基准节点 id（-1=未捕获）
    };
}
