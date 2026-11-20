#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图画布注释项（CurveView，chartType=5，R42 二期建、同日用户
// 拍板修订为三维数据空间锚点；R46 起支持牵引线形态）
//
// 每个可见注释一个实例的 QWidget 子控件：父对象=宿主 m3DChartView（非 _plot
// 画布子控件，R32 图例覆盖层先例——rebuild deleteLater 重建画布不受影响），
// 叠于 GL 画布之上（rebuild 尾 raise，最上层）。**锚点=posX/posY/posZ 三维
// 数据坐标**（注释锚定三维：宿主 projectDataToPixel 按相机 proj*view 投影到
// 画布像素定位本项，视角旋转/缩放/平移后随画布每帧 Paint 重投影、像素位随
// 动——"注释在三维场景中"的载体；投影失败=锚点在相机身后，本项收场隐藏）。
// 两类形态（kAttachMode 按档位分流，字段复用 2D 注释类别值）：
//   - 0 内容锚定（R46 注释模式打点创建，对齐 2D"曲线类带引出线"）：牵引线+
//     注释框——框左上角=锚点像素+固定偏移（+35,-32，PlotView curveBoxOff 同
//     款），本项几何=框∪锚点的包围盒（含 2px 笔画余量）；paintEvent 自绘牵引
//     线（锚点→框中心方向止于入框边，边框色 1.2px 局部抗锯齿，对齐 PlotView
//     引出线画法）+注释框；框/锚点越界收敛钳回画布。
//   - 1 窗口（R42 二期既有口径，Ribbon 旧入口/剪贴板存量）：框左上角=锚点像
//     素（越界收敛钳回画布），无牵引线，本项几何=框矩形。
// paintEvent 自绘注释框：
//   - 背景=kBgColor（无效→半透明白 QColor(255,255,255,200)，图例同款）；
//   - 边框=kBorderShow 且 kBorderWidth>0 时画；kBorderMode==1 且 kBorderColor
//     有效→自定义色，否则默认深灰 QColor(64,64,64)；
//   - 多行文本=kFont+kColor（色无效→深灰 #333 同参数栏页口径）；对齐 kAlign
//     0左/1中/2右=多行文本水平对齐；文本=模板求值结果（渲染期现算，kText 恒
//     存模板原文）。固定内边距 4px 仿注释框观感。
// 模板求值口径（evaluateTemplate）：{Label}→节点本名；{X}/{Y}/{Z}→注释自身
// 锚点数据坐标（QString::number(v,'g',6)，posX/posY/posZ 现读——用户拍板
// "XYZ 是注释的位置而非鼠标"）；{U}/{V}/{W} 及未知 token 字面保留（含花括号
// 原样）。
// 交互（项覆盖 框∪锚点 包围盒[形态 1=框矩形]，围外事件天然到 GL 画布，相机
// 旋转/缩放零影响）：
//   左键按下记录起点→移动超 4px 判拖拽——项内实时跟随（视觉平移，不逐帧
//   提交）；松开分形态提交（宏"移动注释"一次）：内容锚定形态=**框左上角像
//   素**反投影（过原框位/未摆位回落锚点的相机平行面）提交 **框位三键
//   kBoxPosX/kBoxPosY/kBoxPosZ**——锚点三键不动=牵引线拉长/转向且 {X}{Y}{Z}
//   稳定（R46 问题3，对齐二维拖框语义）；窗口形态=框左上角即锚点，提交锚点
//   三键 kPosX+kPosY+kPosZ（R42 既有口径）。均与模型现值比对无变化不发；
//   按下到松开未超阈值=点击选中→setCurrentNodeId(该注释 id)
//  （树回选+参数栏换绑全链既有）。
// 生命周期/防悬挂：项为宿主视图子控件随视图析构；rebuildAnnotationItems 集
//   合级重建走 hide+deleteLater（本帧内不再消费事件）+列表立即换代，文档信号
//   不引用旧项指针。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QWidget>
#include <QColor>
#include <QFont>
#include <QPoint>
#include <QString>

namespace PostCurve
{
    class m3DChartView;

    class POSTCURVE_EXPORT m3DChartAnnotationItem : public QWidget
    {
        Q_OBJECT
    public:
        //host=宿主 m3DChartView（投影/反投影经宿主 seam；同时作 parent widget，
        //项随宿主析构）
        m3DChartAnnotationItem(int annoId, m3DChartView* host);

        int annoId() const { return _annoId; }

        //唯一刷新入口：重读文档现值（样式键+锚点三键+模板原文求值）重算内容
        //与几何并按锚点投影重摆。rebuild 尾重放/注释专属键文档信号/拖拽提交
        //回落三路汇聚于此
        void refreshFromModel();

        //按模型锚点现投影重摆（宿主 resizeEvent 与画布每帧 Paint 后调用；模型
        //值不变，相机矩阵/视口尺寸变化后像素位随动）
        void reposition();

        //模板求值（渲染期现算；kText 恒存模板原文，_text=求值结果）。公开为
        //可测 seam（annotest 断言 {Label}/{X}{Y}{Z}/{U} 字面口径），宿主流程
        //不经此调用（refreshFromModel 内部使用）
        QString evaluateTemplate(const QString& tmpl) const;
        //指定锚点坐标的求值重载（窗口类拖拽实时预览用：拖动中锚点未提交，
        //按临时坐标现算文本，松开提交后 refreshFromModel 按模型值重算）
        QString evaluateTemplate(const QString& tmpl, const double xyz[3]) const;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        //内容自适应尺寸（多行文本 + 固定内边距 4px）
        QSize contentSize() const;
        //锚点三维数据坐标→画布像素（宿主 projectDataToPixel，越界收敛钳回
        //画布内；投影失败返回 false=调用方收场隐藏）。canvasSize 一并回传
        //(reposition 布局复用，免二次查询)
        bool anchorCanvasPixel(QPoint& outPx, QSize& canvasSize) const;
        //锚点→注释框摆位（形态 0=框左上角=锚点+固定偏移后钳回画布；形态 1=
        //框左上角=锚点）。入参=钳制后锚点像素与画布尺寸，回传框左上角
        QPoint boxTopLeftFor(const QPoint& anchorPx, const QSize& canvasSize) const;
        //边框色统一取值（牵引线/框边共用）：borderMode==1 且自定义色有效→
        //自定义色，否则默认深灰
        QColor borderColorEffective() const;

        int _annoId = -1;            //绑定的注释节点id（宿主 _chartId 的子节点）
        m3DChartView* _host = nullptr;   //宿主视图（投影/反投影 seam；非所有）
        QString _rawText;            //模板原文（kText 模型值，求值输入）
        QString _text;               //求值后文本（paintEvent 消费）
        int _align = 0;              //0左 1中 2右
        QFont _font;                 //文本字体
        QColor _textColor;           //文本色（无效→#333 兜底在 paint 期）
        QColor _bgColor;             //背景色（无效→半透明白兜底在 paint 期）
        bool _borderShow = true;     //显示边框
        double _borderWidth = 1.0;   //边框线宽 px
        int _borderMode = 0;         //0跟随默认色 1自定义色
        QColor _borderColor;         //边框自定义色
        int _attachMode = 0;         //0内容锚定（牵引线+框偏移，R46） 1窗口（框左上角=锚点，R42 既有）
        double _posX = 0.0;          //锚点 X（三维数据坐标）
        double _posY = 0.0;          //锚点 Y（三维数据坐标）
        double _posZ = 0.0;          //锚点 Z（三维数据坐标）
        double _boxPosX = qQNaN();   //框位置 X（R46 问题3，仅内容锚定形态消费：拖拽框体提交
        double _boxPosY = qQNaN();   //框位三键、锚点不动=牵引线拉长且 {X}{Y}{Z} 稳定；NaN=
        double _boxPosZ = qQNaN();   //未手动摆位，框=锚点+固定偏移；框左上角三维数据坐标）
        bool _selected = false;      //选中态（中枢当前节点==本注释→原边框加粗+变色为主题蓝，
                                     //R46 问题5；构造连中枢信号+回填双路同步；拖拽完成保持选中，
                                     //取消只经点击画布空白处/点选另一注释）
        QRect _boxRectLocal;         //注释框在本项内的矩形（形态 0 由 reposition 现算；形态 1=rect()）
        QPoint _anchorLocal;         //锚点像素在本项内的位置（形态 0 由 reposition 现算；形态 1 不消费）
        bool _dragging = false;      //拖拽态（超阈值置位；松开提交复位）
        QPoint _pressPos;            //按下点（项内局部坐标，保持抓取偏移用）
        QPoint _dragStartTl;         //拖拽起始项左上角（父系坐标；实时拖拽重摆基准，R46 问题5）
        QPoint _dragAnchorPx;        //拖拽起始锚点像素（父系坐标；内容锚定形态拖拽期恒定=
                                     //牵引线实时拉长的固定端，R46 问题5）
        QPoint _dragBoxTl;           //拖拽起始框左上角（父系坐标；随抓取平移实时重摆，R46 问题5）
    };
}
