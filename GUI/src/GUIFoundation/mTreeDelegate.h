#pragma once
#pragma execution_character_set("utf-8")

#include <functional>
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QPainter>
#include "mModelEnum.h"
#include "guifoundation_global.h"

class QTreeView;

namespace MechMainWindow
{
    /*
    * 树视图通用的层级虚线导引线 + 展开箭头绘制（父子节点虚线连接）。
    * drawNativeBranch：调用方传入“基类 drawBranches”的回调——函数内部用它把指定行的原生箭头
    * 画到离屏位图（与屏幕原生箭头同一条绘制路径，样式完全一致），
    * 再按字形右缘平移贴近节点内容，替代居中在 indent 区间的原生位置；
    * 竖向虚线相应对齐各层展开箭头（向下）的角尖。
    */
    GUIFOUNDATION_EXPORT void PaintDashBranches(const QTreeView* view, QPainter* painter, const QRect& rect, const QModelIndex& index,
        const std::function<void(QPainter*, const QRect&, const QModelIndex&)>& drawNativeBranch);

    class GUIFOUNDATION_EXPORT mTreeDelegate : public QStyledItemDelegate {
        Q_OBJECT
    public:
        explicit mTreeDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

        /*
        * 是否绘制状态勾选图标（视图树等不需要状态语义的树设为 false，
        * 仅保留类型图标与文本，其余风格不变）
        */
        void setShowStatusIcon(bool show) { _showStatusIcon = show; }
        bool showStatusIcon() const { return _showStatusIcon; }

        struct HitAreas { QRect icon1, icon2, text, iconGeo, iconMesh; };

        /*
        * 命中的具体区域（部件子节点的几何/网格图标可点击切换显隐，不参与选中）
        */
        enum class HitArea { None, Icon1, Icon2, IconGeo, IconMesh, Text };

        mTreeDelegate::HitAreas layout(const QStyleOptionViewItem& opt,
            const QModelIndex& idx) const;

        bool hitTest(const QStyleOptionViewItem& opt, const QModelIndex& idx, const QPoint& p) const;

        HitArea hitTestArea(const QStyleOptionViewItem& opt, const QModelIndex& idx, const QPoint& p) const;

        void paint(QPainter* painter,
            const QStyleOptionViewItem& option,
            const QModelIndex& idx) const;

        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& idx) const override;

        bool editorEvent(QEvent* event, QAbstractItemModel* model,
            const QStyleOptionViewItem& option,
            const QModelIndex& index);

        QIcon getPic1Status(MxFunctions::BrowserTreeStatus statusPic) const;

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

    private:
        //是否绘制状态勾选图标（默认绘制）
        bool _showStatusIcon = true;
    };
}



