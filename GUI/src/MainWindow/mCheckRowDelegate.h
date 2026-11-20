#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPolygonF>
#include <QApplication>

namespace MechMainWindow
{
	/*
	* 勾选列表行委托：对齐载荷管理HTML原型——
	* 复选框为17px蓝底白对勾（样式表自定义indicator后Qt不再绘制对勾，需自行绘制）；
	* 被勾选行淡蓝背景(#edf3f9)，选中行淡灰背景(#e9edf2)，悬停行浅灰(#f7f9fb)。
	* 行勾选状态取同行第0列的CheckStateRole，管理器/引用面板勾选列表共用。
	*/
	class mCheckRowDelegate : public QStyledItemDelegate
	{
	public:
		explicit mCheckRowDelegate(QObject* parent = Q_NULLPTR)
			: QStyledItemDelegate(parent)
		{
		}

		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
		{
			QStyleOptionViewItem opt = option;
			initStyleOption(&opt, index);
			QStyle* style = opt.widget != nullptr ? opt.widget->style() : QApplication::style();

			const bool rowChecked = index.sibling(index.row(), 0).data(Qt::CheckStateRole).toInt() == Qt::Checked;
			const bool selected = (opt.state & QStyle::State_Selected) != 0;
			const bool hovered = (opt.state & QStyle::State_MouseOver) != 0;
			const bool enabled = (opt.state & QStyle::State_Enabled) != 0;

			painter->save();

			//行背景：勾选淡蓝 > 选中淡灰 > 悬停浅灰
			QColor bg("#ffffff");
			if (rowChecked)
			{
				bg = QColor("#edf3f9");
			}
			else if (selected)
			{
				bg = QColor("#e9edf2");
			}
			else if (hovered)
			{
				bg = QColor("#f7f9fb");
			}
			painter->fillRect(opt.rect, bg);

			//行底部分隔线（对齐HTML原型的soft-line）
			painter->setPen(QColor("#edf0f3"));
			painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());

			//复选框：蓝底白对勾；绘制区域与subElementRect一致，保证点击热区吻合
			if (opt.features & QStyleOptionViewItem::HasCheckIndicator)
			{
				QRect checkRect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &opt, opt.widget);
				drawCheckIndicator(painter, checkRect, opt.checkState == Qt::Checked, enabled);
			}

			//文本（禁用项置灰，与HTML原型禁用色一致）
			if (!opt.text.isEmpty())
			{
				QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
				QString text = opt.fontMetrics.elidedText(opt.text, opt.textElideMode, textRect.width());
				int align = int(opt.displayAlignment) != 0 ? int(opt.displayAlignment) : int(Qt::AlignLeft | Qt::AlignVCenter);
				QPalette palette = opt.palette;
				palette.setColor(QPalette::Text, enabled ? QColor("#17191c") : QColor("#a3a9b1"));
				style->drawItemText(painter, textRect, align, palette, enabled, text, QPalette::Text);
			}

			painter->restore();
		}

	private:
		static void drawCheckIndicator(QPainter* painter, const QRect& rect, bool checked, bool enabled)
		{
			QRect box(0, 0, qMin(17, rect.width()), qMin(17, rect.height()));
			box.moveCenter(rect.center());
			painter->save();
			painter->setRenderHint(QPainter::Antialiasing);

			QColor border;
			QColor fill;
			if (!enabled)
			{
				border = QColor("#e2e5eb");
				fill = checked ? QColor("#b6c3cf") : QColor("#f5f6f8");
			}
			else if (checked)
			{
				border = QColor("#1689ee");
				fill = QColor("#1689ee");
			}
			else
			{
				border = QColor("#d8dde5");
				fill = QColor("#ffffff");
			}
			painter->setPen(QPen(border, 1));
			painter->setBrush(fill);
			painter->drawRoundedRect(box, 2.5, 2.5);

			if (checked)
			{
				//白色对勾（样式表无法绘制，手工折线绘制）
				painter->setPen(QPen(QColor("#ffffff"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
				painter->setBrush(Qt::NoBrush);
				const qreal l = box.left();
				const qreal t = box.top();
				const qreal w = box.width();
				const qreal h = box.height();
				painter->drawPolyline(QPolygonF()
					<< QPointF(l + w * 0.26, t + h * 0.52)
					<< QPointF(l + w * 0.44, t + h * 0.70)
					<< QPointF(l + w * 0.76, t + h * 0.28));
			}
			painter->restore();
		}
	};
}
