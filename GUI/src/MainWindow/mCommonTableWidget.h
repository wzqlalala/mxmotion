#pragma once
#pragma execution_character_set("utf-8")

#include <QTableWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QItemSelectionModel>
#include <QAbstractItemView>
#include <QSet>

namespace MechMainWindow
{
	/*************************************************通用表格控件（支持回车进入下一行同列）**************************************************************/
	class mCommonTableWidget : public QTableWidget
	{
		Q_OBJECT
	public:
		explicit mCommonTableWidget(QWidget* parent = nullptr)
			: QTableWidget(parent) {}

	protected:
		void keyPressEvent(QKeyEvent* event) override
		{
			// 回车：结束编辑（如有）并移动到“最后一个选中单元格”的下一行同列
			if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
			{
				// 在编辑器关闭前先保存当前选区，避免 closeEditor 后选区被清空
				QModelIndex editIdx = currentIndex();
				QList<QTableWidgetItem*> prevSelected = selectedItems();
				bool editorCommitted = false;

				QWidget* editor = indexWidget(editIdx);
				if (editor)
				{
					commitData(editor);
					closeEditor(editor, QAbstractItemDelegate::NoHint);
					editorCommitted = true;
				}
				else if (state() == QAbstractItemView::EditingState)
				{
					QTableWidget::keyPressEvent(event);
					editorCommitted = true;
				}

				if (editorCommitted && editIdx.isValid())
				{
					batchSyncCurrentColumn(editIdx, prevSelected);
				}

				moveToNextRowSameColumn();
				return;
			}

			QTableWidget::keyPressEvent(event);
		}

	private:
		// 批量同步：将基准单元格所在列的值同步到 prevSelected 中所有同列的选中单元格
		void batchSyncCurrentColumn(const QModelIndex& editIdx, const QList<QTableWidgetItem*>& prevSelected)
		{
			if (!editIdx.isValid()) return;

			int col = editIdx.column();
			if (col <= 0) return; // 第0列通常为编号列，不批量同步

			QTableWidgetItem* sourceItem = item(editIdx.row(), col);
			if (!sourceItem) return;

			QString newText = sourceItem->text();
			bool blocked = blockSignals(true);
			for (QTableWidgetItem* selectedItem : prevSelected)
			{
				if (selectedItem && selectedItem->column() == col && selectedItem->row() != editIdx.row())
				{
					selectedItem->setText(newText);
				}
			}
			blockSignals(blocked);

			// 强制刷新视图，确保批量修改后的值显示出来
			viewport()->update();
		}

		void moveToNextRowSameColumn()
		{
			QModelIndexList selected = selectedIndexes();
			if (selected.isEmpty()) return;

			// 以最后选中的单元格作为基准
			QModelIndex lastIdx = selected.last();
			int nextRow = lastIdx.row() + 1;
			int col = lastIdx.column();

			// 到达最后一行则保持在当前最后一行
			if (nextRow >= rowCount()) {
				nextRow = rowCount() - 1;
			}

			QTableWidgetItem* nextItem = item(nextRow, col);
			if (!nextItem) return;

			// 清除旧选区，选中目标单元格，并滚动到视野内
			setCurrentCell(nextRow, col, QItemSelectionModel::ClearAndSelect);
			scrollToItem(nextItem, QAbstractItemView::EnsureVisible);
		}
	};
}
