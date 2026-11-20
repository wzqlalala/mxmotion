#include "mTreeDelegate.h"

#include <QAbstractItemView>
#include <QImage>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOption>
#include <QTreeView>

#include "mTreeModel.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mViewportModel.h"
#include "mViewportModelManager.h"
#include "mContainerOperation.h"

using namespace MechPreData;
using namespace MxFunctions;
namespace MechMainWindow
{
    /*
    * 层级虚线导引线：父子节点虚线连接，与前处理树一致。
    * 竖线画在第k层祖先列上，是否贯穿本行取决于链上位于其下的节点（chainChild）是否还有后续兄弟；
    * 直接父级的竖线取决于本节点是否为末子节点：末子节点只画到本行中线形成拐角收尾。
    * 横线右端：叶子节点连到内容左缘；带展开箭头的节点止于箭头左侧。
    * 展开箭头用原生样式渲染：非根层与父行文字前的状态符号列对齐，根层右缘贴近自身内容左缘，
    * 不再调用原生 drawBranches——原生箭头居中在 indent 区间内，与节点内容距离偏远。
    */

    // 把箭头交给视图自己的原生 drawBranches 画到离屏透明位图（与屏幕上原生箭头同一条代码路径，
    // 样式/样式表/DPI 行为完全一致），再扫描非透明像素量出字形边界——按字形右缘对齐才能贴近节点。
    // 按展开/收起两态缓存（假定运行期间样式与 DPI 不变）。
    struct NativeArrow { QPixmap pm; QRectF glyph; };

    static const NativeArrow& nativeBranchArrow(const QTreeView* view, const QModelIndex& idx,
        const std::function<void(QPainter*, const QRect&, const QModelIndex&)>& drawNativeBranch)
    {
        static NativeArrow cache[2];
        const bool expanded = view->isExpanded(idx);
        NativeArrow& a = cache[expanded ? 1 : 0];
        if (!a.pm.isNull()) return a;

        const int w = qMax(view->indentation(), 16);   // 分支区宽度
        const int h = 24;                              // 行高（delegate sizeHint 为 24）
        const qreal dpr = view->devicePixelRatioF();
        QPixmap pm(qRound(w * dpr), qRound(h * dpr));
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        {
            QPainter p(&pm);
            drawNativeBranch(&p, QRect(0, 0, w, h), idx);
        }
        // 原生 drawBranches 可能附带画出经典样式的辅助点/线段（表现为虚线后的小点），
        // 按连通域分割，只保留包围盒最大的那块（箭头本身），其余像素全部清掉
        const QImage img = pm.toImage();
        const int W = img.width(), H = img.height();
        QVector<int> label(W * H, 0);
        QVector<QPoint> stack;
        int next = 0, bestLabel = 0, bestRank = -1, bestL = 0, bestR = -1, bestT = 0, bestB = -1;
        for (int sy = 0; sy < H; ++sy)
            for (int sx = 0; sx < W; ++sx)
            {
                if (label[sy * W + sx] || qAlpha(img.pixel(sx, sy)) == 0) continue;
                ++next;
                int l = W, r = -1, t = H, b = -1, cnt = 0;
                stack.push_back(QPoint(sx, sy));
                label[sy * W + sx] = next;
                while (!stack.isEmpty())
                {
                    const QPoint c = stack.takeLast();
                    ++cnt;
                    if (c.x() < l) l = c.x();
                    if (c.x() > r) r = c.x();
                    if (c.y() < t) t = c.y();
                    if (c.y() > b) b = c.y();
                    const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
                    for (int k = 0; k < 4; ++k)
                    {
                        const int nx = c.x() + dx[k], ny = c.y() + dy[k];
                        if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                        if (!label[ny * W + nx] && qAlpha(img.pixel(nx, ny)) > 0)
                        {
                            label[ny * W + nx] = next;
                            stack.push_back(QPoint(nx, ny));
                        }
                    }
                }
                const int rank = (r - l + 1) * (b - t + 1) * 1000 + cnt;  // 先比包围盒面积，再比像素数
                if (rank > bestRank)
                {
                    bestRank = rank; bestLabel = next;
                    bestL = l; bestR = r; bestT = t; bestB = b;
                }
            }
        if (!bestLabel)
        {
            // 原生路径未渲染出字形（理论上不应发生）：退化为 8px 简单三角形，保证箭头不消失
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(view->palette().color(QPalette::Text));
            QPolygonF tri;
            if (expanded)
                tri << QPointF(1, 1) << QPointF(7.0, 1) << QPointF(4.0, 7.0);
            else
                tri << QPointF(1, 1) << QPointF(1, 7.0) << QPointF(7.0, 4.0);
            p.drawPolygon(tri);
            a.glyph = QRectF(0, 0, 8, 8);
        }
        else
        {
            QImage out(W, H, QImage::Format_ARGB32_Premultiplied);
            out.fill(Qt::transparent);
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x)
                    if (label[y * W + x] == bestLabel)
                        out.setPixelColor(x, y, img.pixelColor(x, y));
            pm = QPixmap::fromImage(out);
            pm.setDevicePixelRatio(dpr);
            a.glyph = QRectF(bestL / dpr, bestT / dpr, (bestR - bestL + 1) / dpr, (bestB - bestT + 1) / dpr);
        }
        a.pm = pm;
        return a;
    }

    void PaintDashBranches(const QTreeView* view, QPainter* painter, const QRect& rect, const QModelIndex& index,
        const std::function<void(QPainter*, const QRect&, const QModelIndex&)>& drawNativeBranch)
    {
        if (!view || !painter || !view->model() || !index.isValid())
        {
            return;
        }

        const int indent = view->indentation();
        const int midY = rect.center().y();

        // 节点深度：根层为0
        int depth = 0;
        for (QModelIndex p = index.parent(); p.isValid(); p = p.parent()) ++depth;

        // 展开箭头（根层也画）：经离屏渲染取得原生字形（见 nativeBranchArrow）。
        // 箭头中心x：非根层与父行文字前的状态符号列对齐（mTreeDelegate::layout 中 icon1
        // 位于内容左缘+4、宽8，中心在+8）；根层右缘仍贴自身内容左缘-2px
        const bool arrowsOn = view->itemsExpandable();
        const bool expandable = arrowsOn && view->model()->hasChildren(index);
        const NativeArrow* arrow = expandable ? &nativeBranchArrow(view, index, drawNativeBranch) : nullptr;
        const qreal arrowCenterX = arrow
            ? (depth >= 1 ? qreal(depth * indent + 8)
                : (rect.right() + 1 - 2 - arrow->glyph.width() / 2))
            : 0;
        const qreal arrowDstX = arrow ? (arrowCenterX - arrow->glyph.width() / 2) : 0;
        // 横线右端：叶子节点连到内容左缘（rect 右边界）；带箭头时止于箭头字形左缘-2px
        const int endX = arrow ? (qRound(arrowDstX) - 2) : (rect.right() - 1);

        // 竖线x基准：对齐该层展开箭头（向下）的角尖——可见行的祖先必然处于展开态。
        // 非根层箭头中心=该层indent+8（状态符号列）；根层箭头右缘=内容左缘-2，
        // 半宽取父级（必然展开）的字形（所有箭头同款式同宽）
        const qreal rootArrowHalf = (arrowsOn && depth >= 1)
            ? nativeBranchArrow(view, index.parent(), drawNativeBranch).glyph.width() / 2
            : 0.0;

        QPen pen(view->palette().color(QPalette::Text));
        pen.setStyle(Qt::CustomDashLine);
        pen.setDashPattern({ 3, 2 });   // 显式 3划2隙，配合终点吸附保证末尾是完整划（短横线也有虚线感）
        pen.setWidth(1);
        pen.setColor(QColor(pen.color().red(), pen.color().green(), pen.color().blue(), 80));
        painter->save();
        painter->setPen(pen);

        QModelIndex ancestor = index.parent();
        QModelIndex chainChild = index;
        for (int level = depth - 1; level >= 0 && ancestor.isValid(); --level, chainChild = ancestor, ancestor = ancestor.parent())
        {
            // 竖线从该层向下箭头的角尖垂下：根层箭头右缘贴内容-2，非根层箭头中心=该层indent+8
            const int x = (level == 0)
                ? qRound(indent - 2 - rootArrowHalf)
                : (level * indent + 8);
            const bool chainChildHasSiblingBelow = chainChild.sibling(chainChild.row() + 1, 0).isValid();
            if (level == depth - 1)
            {
                // 末子节点时父级竖线只画到本行中线，否则贯穿整行
                painter->drawLine(x, rect.top(), x, chainChildHasSiblingBelow ? rect.bottom() : midY);
                // 横线终点：叶子节点连到内容左缘；带箭头时吸附到完整虚线周期(5px)的划末，
                // 避免最后一个 dash 被截成 1px 短划（视觉上的“小点”）
                int hx = endX;
                if (arrow)
                {
                    const int len = endX - x;
                    if (len >= 3) hx = x + 5 * ((len - 3) / 5) + 3;
                }
                painter->drawLine(x, midY, qMax(x + 1, hx), midY);
            }
            else if (chainChildHasSiblingBelow)
            {
                painter->drawLine(x, rect.top(), x, rect.bottom());
            }
        }
        painter->restore();

        if (arrow)
        {
            painter->drawPixmap(QPointF(arrowDstX, midY - arrow->glyph.height() / 2), arrow->pm, arrow->glyph);
        }
    }

    /*
    * 部件子节点的几何/网格当前是否隐藏（整体部件显隐层已删除，直接按类型掩码判断）
    * 按节点自身的模型ID查询对应模型的掩码，非当前模型的部件也能反映真实显隐状态
    */
    static bool partDisplayHidden(const QModelIndex& idx, bool isGeo)
    {
        MechPreData::mViewportModel* viewportModel =
            VMManager->getViewportModel(idx.data(Roles::ModelRole).toInt());
        if (viewportModel == nullptr || viewportModel->getModelData() == nullptr) return false;
        mMeshData* meshData = viewportModel->getModelData()->getMeshData();
        if (meshData == nullptr) return false;

        const int partID = idx.data(Roles::PhyidRole).toInt();
        return isGeo ? meshData->getGeoPartMask(partID) : meshData->getMeshPartMask(partID);
    }

    mTreeDelegate::HitAreas mTreeDelegate::layout(const QStyleOptionViewItem& opt, const QModelIndex& idx) const

    {
        QStyleOptionViewItem o(opt);
        initStyleOption(&o, idx);

        const int h = o.rect.height();
        const int iconSz = qMin(h - 4, 16);
        const int iconSz1 = iconSz / 2;               // 第一个图标大小（一半高度，等比例）


        int x = o.rect.x() + 4;
        const int cy = o.rect.y() + (h - iconSz) / 2;

        HitAreas a;

        // 部件子节点：状态图标 + 两张同尺寸的几何/网格显隐图标（可点击）
        const bool isPartChild = idx.data(Roles::TypeRole).toInt() == MxFunctions::ModelChildItem;
        if (isPartChild)
        {
            a.iconGeo = QRect(x + iconSz1 + 2, cy, iconSz, iconSz);
            a.iconMesh = QRect(a.iconGeo.right() + 2, cy, iconSz, iconSz);
            // 状态图标保持原有样式：半高、与几何图标底部对齐
            a.icon1 = QRect(x, a.iconGeo.bottom() - iconSz1, iconSz1, iconSz1);
            x = a.iconMesh.right() + 6;
        }
        else
        {
            if (_showStatusIcon)
            {
                // ★★★ 先计算第二个图标位置 ★★★
                // 预留第一个图标的空间（iconSz1 + 间距2）
                int x2 = x + iconSz1 + 2;                     // 第二个图标起始X
                a.icon2 = QRect(x2, cy, iconSz, iconSz);

                // ★★★ 第一个图标：第二个图标的左边，底部对齐 ★★★
                // 水平：第二个图标左侧，间距2px
                int icon1X = x;                                // 从起始位置开始
                // 垂直：底部与第二个图标底部对齐
                int icon1Y = a.icon2.bottom() - iconSz1;     // 底部对齐

                a.icon1 = QRect(icon1X, icon1Y, iconSz1, iconSz1);
            }
            else
            {
                //无状态图标：仅类型图标 + 文本
                a.icon2 = QRect(x, cy, iconSz, iconSz);
            }

            // 文本区域从第二个图标右侧开始
            x = a.icon2.right() + 6;                      // 第二个图标右侧 + 间距
        }

        const int left = x;
        const int right = o.rect.right() - 4;
        const int availW = qMax(0, right - left + 1);

        const QString text = idx.data(Qt::DisplayRole).toString();

        // 用“最终绘制字体”计算文本宽度（选中时加粗）
        QFont f = o.font;
        f.setPixelSize(13);
        //if (o.state & QStyle::State_Selected) f.setBold(true);
        QFontMetrics fm(f);

        const int naturalW = fm.horizontalAdvance(text);

        // padding：左右各给一点命中/点击热区
        constexpr int padL = 3;
        constexpr int padR = 3;

        const int w = qMin(availW, naturalW + padL + padR);

        // 注意：x 往左扩 padL，但不能越过 left（以免侵入图标间隔）
        const int xText = left; // 保持不侵入左侧（更安全）
        a.text = QRect(xText, o.rect.y(), w, h);

        return a;
    }

    bool mTreeDelegate::hitTest(const QStyleOptionViewItem& opt, const QModelIndex& idx, const QPoint& p) const
    {
        auto a = layout(opt, idx);
        // 几何/网格显隐图标不参与选中判断（icon2 在部件子节点布局下为空矩形）
        return a.icon1.contains(p) || a.icon2.contains(p) || a.text.contains(p);
    }

    mTreeDelegate::HitArea mTreeDelegate::hitTestArea(const QStyleOptionViewItem& opt, const QModelIndex& idx, const QPoint& p) const
    {
        auto a = layout(opt, idx);
        if (a.iconGeo.contains(p)) return HitArea::IconGeo;
        if (a.iconMesh.contains(p)) return HitArea::IconMesh;
        if (a.icon1.contains(p)) return HitArea::Icon1;
        if (a.icon2.contains(p)) return HitArea::Icon2;
        if (a.text.contains(p)) return HitArea::Text;
        return HitArea::None;
    }

    void mTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& idx) const
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, idx);

        painter->save();

        auto areas = layout(opt, idx);
        // ★★★ 新增：选中时绘制文本区域背景（在图标之前或之后均可）★★★
        const bool selected = (opt.state & QStyle::State_Selected);
        const bool itemFocus = (opt.state & QStyle::State_HasFocus);
        const bool viewFocus = (opt.widget && opt.widget->hasFocus());
        const bool selFocused = selected && itemFocus && viewFocus;
        const bool selUnfocused = selected && !selFocused;
        if (selUnfocused)
        {
            QRect textBgRect = areas.text.adjusted(-6, 2, 6, -2);  // 比文本区域稍大

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#F0F0F0"));  // 淡蓝背景，或改用 QColor(19, 133, 246, 30)
            painter->drawRect(textBgRect);  // 4px圆角
            painter->restore();
        }
        else if (opt.state & QStyle::State_Selected) {
            QRect textBgRect = areas.text.adjusted(-6, 2, 6, -2);  // 比文本区域稍大

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#D0E7FD"));  // 淡蓝背景，或改用 QColor(19, 133, 246, 30)
            painter->drawRect(textBgRect);  // 4px圆角
            painter->restore();
        }
        //绘制图标
        QIcon ic1 = getPic1Status((MxFunctions::BrowserTreeStatus)idx.data(Roles::StatusRole).toInt());
        const bool isPartChild = idx.data(Roles::TypeRole).toInt() == MxFunctions::ModelChildItem;
        if (isPartChild)
        {
            // 部件子节点：状态图标 + 几何/网格显隐图标（隐藏时半透明表示）
            ic1.paint(painter, areas.icon1);
            QIcon geoIcon(":/mMainWidgets/Icon/BrowserTree/jihe3x.png");
            QIcon meshIcon(":/mMainWidgets/Icon/BrowserTree/wangge3x.png");
            painter->save();
            painter->setOpacity(partDisplayHidden(idx, true) ? 0.3 : 1.0);
            geoIcon.paint(painter, areas.iconGeo);
            painter->restore();
            painter->save();
            painter->setOpacity(partDisplayHidden(idx, false) ? 0.3 : 1.0);
            meshIcon.paint(painter, areas.iconMesh);
            painter->restore();
        }
        else
        {
            QIcon ic2(idx.data(Roles::Icon2Role).toString());
            if (_showStatusIcon)
            {
                ic1.paint(painter, areas.icon1);
            }
            ic2.paint(painter, areas.icon2);
        }
        //绘制文本
        QString text = idx.data(Qt::DisplayRole).toString();

        QFont f = opt.font;
        f.setPixelSize(13);
        QColor color = opt.palette.color(QPalette::Text);
     

        if (opt.state & QStyle::State_Selected) {
          /*  f.setBold(true);*/
            color = opt.palette.color(QPalette::Text);
        }
        else if (opt.state & QStyle::State_MouseOver) {
            // 全局坐标（屏幕坐标）
            QPoint globalPos = QCursor::pos();

            // 转成 view->viewport() 的坐标
            const QWidget* vp = option.widget; // 对于 item view delegate，通常是 viewport()
            QPoint vpPos = vp ? vp->mapFromGlobal(globalPos) : QPoint();
            if (hitTest(opt, idx, vpPos))
            {
                // 悬浮状态：同样使用 #1385F6
                color = QColor("#1385F6");
            }
        }
     
        painter->setFont(f);
        painter->setPen(color);

        // 用最终字体计算省略文本（重要：选中加粗后宽度会变）
        QFontMetrics fm(painter->font());
        const QString elided = fm.elidedText(text, Qt::ElideRight, areas.text.width());

        painter->drawText(areas.text, Qt::AlignVCenter | Qt::TextSingleLine, elided);

        painter->restore();
    }

    QSize mTreeDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& idx) const
    {
        Q_UNUSED(idx);
        return QSize(option.rect.width(), 24);
    }

    bool mTreeDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        if (event->type() == QEvent::MouseMove) {
            // option.widget 通常是 viewport()
            if (option.widget) {
                if (auto* view = qobject_cast<const QAbstractItemView*>(option.widget->parentWidget())) {
                    auto* vp = view->viewport(); // QWidget*
                    vp->update(view->visualRect(index)); // 精确重绘该 index
                    // 部件子节点：鼠标悬浮在几何/网格图标上时显示手型，提示可点击
                    const auto* me = static_cast<const QMouseEvent*>(event);
                    const bool overDisplayIcon =
                        index.data(Roles::TypeRole).toInt() == MxFunctions::ModelChildItem
                        && (hitTestArea(option, index, me->pos()) == HitArea::IconGeo
                            || hitTestArea(option, index, me->pos()) == HitArea::IconMesh);
                    vp->setCursor(overDisplayIcon ? Qt::PointingHandCursor : Qt::ArrowCursor);
                }
                else {
                    // 兜底：至少重绘 option.rect
                    const_cast<QWidget*>(option.widget)->update(option.rect);
                }
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    QIcon mTreeDelegate::getPic1Status(MxFunctions::BrowserTreeStatus statusPic) const
    {
        switch (statusPic)
        {
        case MxFunctions::Tree_Yes:
            return QIcon(":/mMainWidgets/Icon/BrowserTree/gou3x.png");
        case MxFunctions::Tree_Failure:
            return QIcon(":/mMainWidgets/Icon/BrowserTree/wenhao3x.png");
        case MxFunctions::Tree_Update:
            return QIcon(":/mMainWidgets/Icon/BrowserTree/update3x.png");
        case MxFunctions::Tree_Wrong:
            return QIcon(":/mMainWidgets/Icon/BrowserTree/cha3x.png");
        default:
            break;
        }
        return QIcon();
    }

    void mTreeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        //前处理树识别经属性标记（mTreeView构造设置），代理保持对两树通用
        QAbstractItemView* view = qobject_cast<QAbstractItemView*>(parent());
        if (!view || !view->property("mechPreTreeView").toBool())
            view = nullptr;
        if (!view || !model || !index.isValid()) {
            QStyledItemDelegate::setModelData(editor, model, index);
            return;
        }

        QModelIndexList selected = view->selectionModel()->selectedIndexes();
        if (selected.size() <= 1) {
            QStyledItemDelegate::setModelData(editor, model, index);
            return;
        }

        auto* line = qobject_cast<QLineEdit*>(editor);
        if (!line) return;
        const QString newName = line->text();
        if (newName.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, "名称不能为空！");
            return;
        }

        BrowserTreeType treeType = (BrowserTreeType)selected[0].data(Roles::TypeRole).toInt();
        //后处理未开发多选重命名
        QStringList names = getCVM->getModelData()->getAllNames(mContainerOperation::mapPhyTypeForTreeType(treeType));
        for (const auto& idx : selected)
        {
            names.removeOne(idx.data(Qt::DisplayRole).toString());
        }

        model->blockSignals(true);
        int i = 1; QString name;
        for (const auto& idx : selected)
        {
            do {
                name = QString("%1_%2").arg(newName).arg(i++);
            } while (names.contains(name));
            model->setData(idx, name, Qt::EditRole);
        }
        model->blockSignals(false);
    }

}