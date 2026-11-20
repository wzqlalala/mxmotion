#include "m3DChartAnnotationItem.h"
#include "m3DChartView.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveCore/CurveDocument.h"

//QT
#include <QMouseEvent>
#include <QPainter>
#include <QFontMetrics>
#include <QUndoStack>

namespace PostCurve
{
    namespace
    {
        //注释框固定内边距（px，仿注释框观感；contentSize/anchorPixel/paint 三处
        //同值单一来源）
        const int kAnnoPad = 4;
        //边框默认深灰（borderMode!=1 或自定义色无效时；深灰与参数栏页文本兜底
        //口径一致但独立取值，避免边框与文字同色粘连）
        const QColor kDefaultBorderColor(64, 64, 64);
        //文本默认深灰（kColor 无效时；#333 与 mAnnotation3DPropPage 回显兜底同值）
        const QColor kDefaultTextColor(51, 51, 51);
        //选中态边框色（R46 问题5：主题蓝，与二维选中框/吸附标记同色系）
        const QColor kSelectionColor(43, 125, 233);
        //牵引线形态（attachMode==0，R46）框左上角相对锚点的固定偏移（px）：
        //引出线端右上，与 PlotView AnnotationSettings::curveBoxOff（2D 曲线类
        //注释同款默认）一致——锚点右上摆框、牵引线自锚点连到框
        const int kLeaderBoxOffX = 35;
        const int kLeaderBoxOffY = -32;
        //牵引线线宽（px）与包围盒笔画余量：对齐 PlotView leaderWidth 默认 1.2；
        //余量=牵引线局部抗锯齿笔画不出界
        const double kLeaderLineWidth = 1.2;
        const int kLeaderMargin = 2;
    }

    m3DChartAnnotationItem::m3DChartAnnotationItem(int annoId, m3DChartView* host)
        : QWidget(host)
        , _annoId(annoId)
        , _host(host)
    {
        //子控件置顶由宿主 rebuild 尾统一 raise（与图例覆盖层的相对层级=注释项
        //在最上）；此处不设 WA_TransparentForMouseEvents——注释框需要接收鼠标
        //（点击选中/拖拽），框外事件天然穿透到 GL 画布（项仅覆盖自身矩形）
        //选中态同步（R46 问题5）：连选择中枢——本注释被选中→高亮框显形，取消
        //选中→回落普通边框；信号对本项即重绘（两态翻转才 update，其余节点变
        //化零成本）
        connect(mCurveDocController::getInstance(),
            &mCurveDocController::sig_currentNodeChanged, this,
            [this](int nodeId)
        {
            const bool selected = (nodeId == _annoId);
            if (selected != _selected)
            {
                _selected = selected;
                update();
            }
        });
    }

    //--- 模板求值（渲染期现算；kText 恒存模板原文） ---
    //口径：{Label}→节点本名；{X}/{Y}/{Z}→注释自身锚点数据坐标('g',6)——用户
    //拍板"XYZ 是注释的位置而非鼠标"；{U}/{V}/{W} 及未知 token 字面保留（含
    //花括号原样回填）
    QString m3DChartAnnotationItem::evaluateTemplate(const QString& tmpl) const
    {
        const double xyz[3] = { _posX, _posY, _posZ };
        return evaluateTemplate(tmpl, xyz);
    }

    QString m3DChartAnnotationItem::evaluateTemplate(const QString& tmpl,
        const double xyz[3]) const
    {
        CurveDocument* doc = mCurveDocController::getInstance()
            ? mCurveDocController::getInstance()->document() : nullptr;
        const CurveNode* n = doc ? doc->node(_annoId) : nullptr;
        const QString label = n ? n->name : QString();
        const double* anchor = xyz;

        QString out;
        out.reserve(tmpl.size());
        int i = 0;
        const int size = tmpl.size();
        while (i < size)
        {
            if (tmpl.at(i) == QLatin1Char('{'))
            {
                const int close = tmpl.indexOf(QLatin1Char('}'), i + 1);
                if (close > i)
                {
                    const QString token = tmpl.mid(i + 1, close - i - 1);
                    QString value;
                    bool known = true;
                    if (token == QLatin1String("Label"))
                        value = label;
                    else if (token == QLatin1String("X")
                        || token == QLatin1String("Y")
                        || token == QLatin1String("Z"))
                    {
                        //锚点数据坐标（'g',6 与渲染端双精度换算精度约定一致）
                        const int axis = (token == QLatin1String("X")) ? 0
                            : (token == QLatin1String("Y")) ? 1 : 2;
                        value = QString::number(anchor[axis], 'g', 6);
                    }
                    else
                        known = false;   //{U}/{V}/{W}/未知 token：字面保留（含花括号）
                    if (known)
                    {
                        out += value;
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

    //--- 布局（内容自适应宽高 + 锚点投影定位） ---
    QSize m3DChartAnnotationItem::contentSize() const
    {
        const QFontMetrics fm(_font);
        const QStringList lines = _rawText.isEmpty()
            ? QStringList(QString()) : _text.split(QLatin1Char('\n'));
        int w = 0;
        for (const QString& line : lines)
            w = qMax(w, fm.horizontalAdvance(line));
        const int h = lines.size() * fm.lineSpacing();
        return QSize(w + kAnnoPad * 2, h + kAnnoPad * 2);
    }

    bool m3DChartAnnotationItem::anchorCanvasPixel(QPoint& outPx, QSize& canvasSize) const
    {
        //锚点三维数据坐标→画布像素（宿主投影 seam；失败=画布未建/锚点在相机
        //身后=调用方收场隐藏）；投影像素越界收敛钳回画布内保证可见
        const double xyz[3] = { _posX, _posY, _posZ };
        QPointF px;
        if (!_host || !_host->projectDataToPixel(xyz, px))
            return false;
        const QWidget* canvas = parentWidget();
        canvasSize = canvas ? canvas->size() : QSize(0, 0);
        const int maxW = qMax(0, canvasSize.width() - 1);
        const int maxH = qMax(0, canvasSize.height() - 1);
        outPx = QPoint(qBound(0, int(px.x() + 0.5), maxW),
            qBound(0, int(px.y() + 0.5), maxH));
        return true;
    }

    QPoint m3DChartAnnotationItem::boxTopLeftFor(const QPoint& anchorPx,
        const QSize& canvasSize) const
    {
        //形态 0：框左上角=锚点+固定偏移（引出线端右上），框整体钳回画布；
        //形态 1：框左上角=锚点（既有口径），同口径钳回画布
        const QSize boxSize = contentSize();
        QPoint tl = (_attachMode == 0)
            ? anchorPx + QPoint(kLeaderBoxOffX, kLeaderBoxOffY)
            : anchorPx;
        tl.setX(qBound(0, tl.x(), qMax(0, canvasSize.width() - boxSize.width())));
        tl.setY(qBound(0, tl.y(), qMax(0, canvasSize.height() - boxSize.height())));
        return tl;
    }

    QColor m3DChartAnnotationItem::borderColorEffective() const
    {
        return (_borderMode == 1 && _borderColor.isValid())
            ? _borderColor : kDefaultBorderColor;
    }

    void m3DChartAnnotationItem::reposition()
    {
        //拖拽态守卫（实机缺陷修复：拖拽中途任何一次 GL 重绘都会经宿主
        //layoutAnnotationItems 触发本函数——按模型旧锚点重摆=拖拽中被拽回旧位
        //（"拖动后闪回"根因）。拖拽期视觉位置由 mouseMoveEvent 独占，松开提交
        //后文档信号回落本函数时 _dragging 已复位，正常按新锚点重摆
        if (_dragging)
            return;
        QPoint anchorPx;
        QSize cs;
        if (!anchorCanvasPixel(anchorPx, cs))
        {
            if (isVisible())
                hide();   //锚点投影失败（相机身后/画布未就绪）：收场隐藏
            return;
        }
        if (_attachMode == 0)
        {
            //牵引线形态：本项几何=框∪锚点包围盒（含笔画余量，框/锚点均已钳
            //回画布，包围盒天然在画布内）。框内矩形与锚点项内偏移随几何现算
            //（paintEvent 画牵引线/框、松开反投影回传锚点/框位像素均消费）。
            //框左上角位=R46 问题3——框位三键有效（已拖拽过）按框位投影摆框
            //（牵引线随框位拉长/转向，锚点{X}{Y}{Z} 稳定），缺省（未拖拽）=
            //锚点+固定偏移；框位投影失败（相机身后等）回落固定偏移
            QRect boxRect(QPoint(boxTopLeftFor(anchorPx, cs)), contentSize());
            if (qIsFinite(_boxPosX) && qIsFinite(_boxPosY) && qIsFinite(_boxPosZ))
            {
                const double box[3] = { _boxPosX, _boxPosY, _boxPosZ };
                QPointF bpx;
                if (_host && _host->projectDataToPixel(box, bpx))
                {
                    const QSize boxSize = contentSize();
                    const QPoint tl(qBound(0, int(bpx.x() + 0.5),
                            qMax(0, cs.width() - boxSize.width())),
                        qBound(0, int(bpx.y() + 0.5),
                            qMax(0, cs.height() - boxSize.height())));
                    boxRect = QRect(tl, boxSize);
                }
            }
            QRect bounds = boxRect.united(QRect(anchorPx, anchorPx))
                .adjusted(-kLeaderMargin, -kLeaderMargin, kLeaderMargin, kLeaderMargin);
            bounds = bounds.intersected(QRect(QPoint(0, 0), cs));
            _boxRectLocal = boxRect.translated(-bounds.topLeft());
            _anchorLocal = anchorPx - bounds.topLeft();
            setGeometry(bounds);
        }
        else
        {
            //窗口形态（既有口径）：本项几何=框矩形，框左上角=锚点（钳回画布）
            const QPoint tl = boxTopLeftFor(anchorPx, cs);
            _boxRectLocal = QRect(QPoint(0, 0), contentSize());
            _anchorLocal = QPoint(0, 0);
            setGeometry(QRect(tl, contentSize()));
        }
        if (!isVisible())
            show();   //此前因投影失败隐藏（相机转回可见侧）：恢复显形
    }

    void m3DChartAnnotationItem::refreshFromModel()
    {
        CurveDocument* doc = mCurveDocController::getInstance()
            ? mCurveDocController::getInstance()->document() : nullptr;
        CurveNode* n = doc ? doc->node(_annoId) : nullptr;
        if (!n || n->type != CurveNodeType::Annotation)
        {
            hide();   //节点已删（删除信号先于集合级重建到达窗口期）：安全收场
            return;
        }
        const CurveAnnotationData& a = n->annotation;
        //单项显隐（kVisible）：false=隐藏（与 rebuildAnnotationItems 的收集
        //过滤同口径——总开关×单项两态均不过绘制路径）
        if (!a.visible)
        {
            hide();
            return;
        }
        _rawText = a.text;
        _align = qBound(0, a.align, 2);
        _font = a.font;
        _textColor = a.color;
        _bgColor = a.bgColor;
        _borderShow = a.borderShow;
        _borderWidth = a.borderWidth;
        _borderMode = a.borderMode;
        _borderColor = a.borderColor;
        _attachMode = (a.attachMode == 0) ? 0 : 1;   //0 内容锚定 1 窗口（其余值收敛 1，防御）
        _posX = a.posX;
        _posY = a.posY;
        _posZ = a.posZ;
        _boxPosX = a.boxPosX;   //R46 问题3：框位三键（NaN=未手动摆位→框=锚点+固定偏移）
        _boxPosY = a.boxPosY;
        _boxPosZ = a.boxPosZ;
        //选中态回填（R46 问题5）：rebuild 重建项/单项刷新时按中枢现值重放
        //（增删/样式提交回落本函数，选中框不因重建丢失）
        _selected = (mCurveDocController::getInstance()->currentNodeId() == _annoId);
        _text = evaluateTemplate(_rawText);
        reposition();   //锚点投影定位（内部按形态定几何，失败=项内收场隐藏）
        raise();        //维持最上层（图例覆盖层之上，宿主 rebuild 尾同序）
        update();
    }

    //--- 绘制（牵引线/背景/边框/多行文本，口径见类头） ---
    void m3DChartAnnotationItem::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event)
        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        const QRect box = (_attachMode == 0) ? _boxRectLocal : rect();

        //牵引线（attachMode==0）：锚点→框中心方向止于入框处框边（框底色与边框
        //随后绘制正好覆盖接点；斜向摆框取中心取向，不会斜插到框角——PlotView
        //2D 曲线注释引出线同款画法）；锚点在框内不画。线色=边框有效色（牵引线
        //与框一体观感），线宽 1.2px 局部抗锯齿（无 AA 斜线栅格化成断续列）
        if (_attachMode == 0 && !box.contains(_anchorLocal))
        {
            const QPointF a(_anchorLocal);
            const QPointF c(box.center());
            const double dx = c.x() - a.x();
            const double dy = c.y() - a.y();
            double tEnter = 0.0;   //锚点→中心线段与框边的进入参数
            if (dx != 0.0)
                tEnter = qMax(tEnter, qMin((box.left() - a.x()) / dx,
                    (box.right() - a.x()) / dx));
            if (dy != 0.0)
                tEnter = qMax(tEnter, qMin((box.top() - a.y()) / dy,
                    (box.bottom() - a.y()) / dy));
            if (tEnter > 0.0)
            {
                QPen leaderPen(borderColorEffective());
                leaderPen.setWidthF(kLeaderLineWidth);
                p.setRenderHint(QPainter::Antialiasing, true);
                p.setPen(leaderPen);
                p.drawLine(a, a + tEnter * QPointF(dx, dy));
                p.setRenderHint(QPainter::Antialiasing, false);
            }
        }

        //背景（kBgColor 无效→半透明白，图例同款 alpha 200）
        p.setPen(Qt::NoPen);
        p.setBrush(_bgColor.isValid() ? _bgColor : QColor(255, 255, 255, 200));
        p.drawRect(box);

        //边框（选中态=R46 问题5 修订：**本框边框加粗+变色**为主题蓝——宽度=
        //常态有效线宽+1px（显示边框关断/零宽时按 1px 基准加粗到 2px，选中提
        //示恒可见），非选中=既有口径（kBorderShow 且 kBorderWidth>0 时画；
        //borderMode==1 且自定义色有效→自定义色，否则默认深灰；线宽钳 >=1px）。
        //替换首版"框外另描细线"方案——用户拍板直接加粗变色原边框
        const double baseWidth = (_borderShow && _borderWidth > 0)
            ? qMax(1.0, _borderWidth) : 1.0;
        if (_selected || (_borderShow && _borderWidth > 0))
        {
            QPen pen;
            if (_selected)
            {
                pen.setColor(kSelectionColor);
                pen.setWidthF(baseWidth + 1.0);   //加粗一档
            }
            else
            {
                pen.setColor(borderColorEffective());
                pen.setWidthF(qMax(1.0, _borderWidth));
            }
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            const qreal half = pen.widthF() / 2.0;
            p.drawRect(QRectF(box).adjusted(half, half, -half, -half));
        }

        //多行文本（kFont+kColor 无效→#333；对齐=多行逐行水平对齐 0左 1中 2右）
        p.setFont(_font);
        p.setPen(_textColor.isValid() ? _textColor : kDefaultTextColor);
        const QFontMetrics fm(_font);
        const QRect content = box.adjusted(kAnnoPad, kAnnoPad, -kAnnoPad, -kAnnoPad);
        const QStringList lines = _text.split(QLatin1Char('\n'));
        int y = content.top();
        for (const QString& line : lines)
        {
            int x = content.left();
            if (_align == 1)
                x = content.left() + (content.width() - fm.horizontalAdvance(line)) / 2;
            else if (_align == 2)
                x = content.right() - fm.horizontalAdvance(line) + 1;
            p.drawText(x, y + fm.ascent(), line);
            y += fm.lineSpacing();
        }
    }

    //--- 画布交互（点击选中/拖拽移动；项仅覆盖自身矩形，框外事件天然到 GL 画布） ---

    void m3DChartAnnotationItem::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            _pressPos = event->pos();
            _dragging = false;
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void m3DChartAnnotationItem::mouseMoveEvent(QMouseEvent* event)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            //未选中不可拖（R46 问题5 追加，用户口径"先选中后拖动"）：按下已
            //被本项消费（画布不会在其下创建新注释/转相机），移动静默忽略——
            //松开 ≤4px 仍走点击选中，>4px 不选不拖
            if (!_selected)
            {
                event->accept();
                return;
            }
            if (!_dragging)
            {
                //移动超 ~4px 判拖拽（阈值内=抖动，不进入拖拽态）；进入即记录
                //拖拽起始基准（项左上角/锚点像素/框左上角，父系坐标——实时拖
                //拽重摆的三基准，R46 问题5）
                if ((event->pos() - _pressPos).manhattanLength() <= 4)
                    return;
                _dragging = true;
                _dragStartTl = pos();
                _dragAnchorPx = pos() + _anchorLocal;
                _dragBoxTl = pos() + _boxRectLocal.topLeft();
            }
            //视觉跟随：按抓取偏移平移、钳回画布内；不逐帧提交（松开一次提交。
            //拖拽期相机不动——左键被本项消费不达画布，投影/反投影口径自洽）
            const QWidget* canvas = parentWidget();
            const QSize cs = canvas ? canvas->size() : QSize(0, 0);
            const QPoint newTl = mapToParent(event->pos() - _pressPos);
            if (_attachMode == 0)
            {
                //内容锚定形态（R46 问题5 实时刷新）：**锚点像素恒定**（钉在拾
                //取点上），仅框随抓取平移——牵引线实时拉长/转向，{X}{Y}{Z} 文
                //本天然稳定（锚点未变）。几何=框∪锚点重算+update() 重绘牵引线
                const QPoint delta = newTl - _dragStartTl;
                const QSize boxSize = contentSize();
                QPoint boxTl = _dragBoxTl + delta;
                boxTl.setX(qBound(0, boxTl.x(), qMax(0, cs.width() - boxSize.width())));
                boxTl.setY(qBound(0, boxTl.y(), qMax(0, cs.height() - boxSize.height())));
                const QRect boxRect(boxTl, boxSize);
                QRect bounds = boxRect.united(QRect(_dragAnchorPx, _dragAnchorPx))
                    .adjusted(-kLeaderMargin, -kLeaderMargin, kLeaderMargin, kLeaderMargin);
                bounds = bounds.intersected(QRect(QPoint(0, 0), cs));
                _boxRectLocal = boxRect.translated(-bounds.topLeft());
                _anchorLocal = _dragAnchorPx - bounds.topLeft();
                setGeometry(bounds);
                update();
                event->accept();
                return;
            }
            //窗口形态（框左上角=锚点）：整项平移（既有口径，钳回画布内）+
            //**位置信息实时刷新**——拖动中锚点像素=项左上角，反投影（过模型
            //锚点相机平行面）得临时数据坐标，模板按临时坐标现算文本（不提交；
            //松开提交后 refreshFromModel 按模型值重算）。反投影失败保持现文本
            QPoint tlWin = newTl;
            tlWin.setX(qBound(0, tlWin.x(), qMax(0, cs.width() - width())));
            tlWin.setY(qBound(0, tlWin.y(), qMax(0, cs.height() - height())));
            move(tlWin);
            if (_host)
            {
                CurveDocument* doc = mCurveDocController::getInstance()->document();
                const CurveNode* n = doc ? doc->node(_annoId) : nullptr;
                if (n)
                {
                    const double through[3] = { n->annotation.posX,
                        n->annotation.posY, n->annotation.posZ };
                    double provisional[3] = { 0.0, 0.0, 0.0 };
                    if (_host->unprojectPixelThroughData(pos(), through, provisional))
                    {
                        _text = evaluateTemplate(_rawText, provisional);
                        update();
                    }
                }
            }
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void m3DChartAnnotationItem::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            mCurveDocController* controller = mCurveDocController::getInstance();
            if (_dragging)
            {
                _dragging = false;
                //松开（R46 问题3 分形态）：内容锚定形态=**框左上角像素**反投影
                //到"过原框位（未摆位回落锚点）相机平行面"提交 **框位三键**
                //kBoxPosX/kBoxPosY/kBoxPosZ——锚点三键不动=牵引线拉长/转向且
                //{X}{Y}{Z} 稳定（用户口径"和二维曲线一样"）；差分基准=已摆位
                //框位自身（深度保持平面内移动），首拖未摆位=锚点（框被放到锚
                //点深度平面的目标像素处）。窗口形态=框左上角即锚点，提交锚点
                //三键（既有口径不变）。一条撤销宏"移动注释"；提交触发的文档信
                //号回落 refreshFromModel→按已提交值重投影=原位无视觉跳变；与
                //模型现值比对"无变化不发"（框位 NaN 与有限值恒不等=首拖必提
                //交；控制器无同值短路拖回原位不压空操作宏）。反投影失败（射线
                //平行/退化）保持原值不动
                CurveDocument* doc = mCurveDocController::getInstance()->document();
                CurveNode* n = doc ? doc->node(_annoId) : nullptr;
                if (_host && n)
                {
                    double through[3] = { 0.0, 0.0, 0.0 };
                    if (_attachMode == 0)
                    {
                        const double boxNow[3] = { n->annotation.boxPosX,
                            n->annotation.boxPosY, n->annotation.boxPosZ };
                        const bool placed = qIsFinite(boxNow[0])
                            && qIsFinite(boxNow[1]) && qIsFinite(boxNow[2]);
                        if (placed)
                        {
                            through[0] = boxNow[0];
                            through[1] = boxNow[1];
                            through[2] = boxNow[2];
                        }
                        else
                        {
                            through[0] = n->annotation.posX;
                            through[1] = n->annotation.posY;
                            through[2] = n->annotation.posZ;
                        }
                    }
                    else
                    {
                        through[0] = n->annotation.posX;
                        through[1] = n->annotation.posY;
                        through[2] = n->annotation.posZ;
                    }
                    const QPoint releasePx = (_attachMode == 0)
                        ? pos() + _boxRectLocal.topLeft()   //框左上角新像素
                        : pos();                            //窗口=框左上角即锚点
                    double outData[3] = { 0.0, 0.0, 0.0 };
                    if (_host->unprojectPixelThroughData(releasePx, through, outData)
                        && (outData[0] != through[0] || outData[1] != through[1]
                            || outData[2] != through[2]))
                    {
                        controller->undoStack()->beginMacro(QStringLiteral("移动注释"));
                        if (_attachMode == 0)
                        {
                            controller->setNodeProperty(_annoId, CurveProp::kBoxPosX, outData[0]);
                            controller->setNodeProperty(_annoId, CurveProp::kBoxPosY, outData[1]);
                            controller->setNodeProperty(_annoId, CurveProp::kBoxPosZ, outData[2]);
                        }
                        else
                        {
                            controller->setNodeProperty(_annoId, CurveProp::kPosX, outData[0]);
                            controller->setNodeProperty(_annoId, CurveProp::kPosY, outData[1]);
                            controller->setNodeProperty(_annoId, CurveProp::kPosZ, outData[2]);
                        }
                        controller->undoStack()->endMacro();
                    }
                    //拖拽完成**保持选中**（R46 问题5 终版口径，用户拍板推翻二
                    //轮"移动完成即清除"）——高亮随选中保持，取消只经"点击画布
                    //空白处"或"点选另一注释"（宿主 eventFilter 空白清除分流/
                    //单一选中顶替）
                }
            }
            else
            {
                //按下到松开未超阈值=点击选中（setCurrentNodeId，树回选+参数栏
                //换绑全链既有；类型筛选作用域守卫由中枢内部兜底）。超阈值松开
                //=未选中项上的拖拽尝试（未选中不可拖，R46 问题5 追加）——既不
                //选中也不移动，静默放弃
                if ((event->pos() - _pressPos).manhattanLength() <= 4)
                    controller->setCurrentNodeId(_annoId);
            }
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }
}
