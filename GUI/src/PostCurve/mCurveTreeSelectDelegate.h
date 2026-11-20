#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 结构树行代理（选中条样式，多树共用）：
// 曲线树/曲线选取器/定义曲线对象树的选中态统一——选中时在文字区自绘浅蓝圆角条
// （树失焦转灰 #F0F0F0），并清除原生选中/焦点态后交给基类绘制：无原生深蓝
// 选中底、无反白字与焦点虚线框，文字保持正常前景色；图标仍走 DecorationRole。
// =============================================================================

#include <QStyledItemDelegate>
#include <QPainter>

namespace PostCurve
{
	class CurveTreeItemDelegate : public QStyledItemDelegate
	{
	public:
		using QStyledItemDelegate::QStyledItemDelegate;

	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option,
			const QModelIndex& idx) const override
		{
			QStyleOptionViewItem opt(option);
			initStyleOption(&opt, idx);
			if (opt.state & QStyle::State_Selected)
			{
				//文字区起点：DecorationRole 组合图标（28px）+ 4px 间距（与原生
				//条目图标/文本布局一致）；选中条从文字处起、右侧稍作延伸
				const int iconW = (opt.features & QStyleOptionViewItem::HasDecoration)
					? opt.decorationSize.width() : 0;
				const int textLeft = opt.rect.x() + iconW + (iconW ? 4 : 0);
				const int availW = qMax(0, opt.rect.right() - textLeft - 2);
				QFontMetrics fm(opt.font);
				const int barW = qMin(availW, fm.horizontalAdvance(opt.text) + 6);
				const bool focused = (opt.state & QStyle::State_HasFocus)
					&& opt.widget && opt.widget->hasFocus();
				painter->save();
				painter->setRenderHint(QPainter::Antialiasing);
				painter->setPen(Qt::NoPen);
				painter->setBrush(focused ? QColor("#D0E7FD") : QColor("#F0F0F0"));
				painter->drawRoundedRect(QRect(textLeft, opt.rect.y() + 2, barW,
					opt.rect.height() - 4), 4, 4);
				painter->restore();
				//基类按"未选中"绘制：无原生选中底/反白字，无焦点虚线框
				opt.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
			}
			QStyledItemDelegate::paint(painter, opt, idx);
		}
	};
}
