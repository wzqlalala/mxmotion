#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>
#include <QTableWidget>
#include <QItemDelegate>
#include <QRegExpValidator>
#include <QKeyEvent>
#include <QTimer>
#include <QItemSelectionModel>
#include <QApplication>

#include <map>

#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mMechGlobalSignals.h"
#include "mPointPickerWidget.h"

using namespace MxFunctions;
using namespace MechCommon;

namespace MechDiscreteFieldData
{
	class mDiscreteField;
}

namespace MechMainWindow
{
    class mDiscreteFieldTableWidget;
	//离散场面板
	class MMAINWINDOW_EXPORT mDiscreteFieldPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		mDiscreteFieldPanel(QWidget* parent = Q_NULLPTR);

		~mDiscreteFieldPanel();

		//新建离散场面板
		void newDiscreteFieldPanel(MxFunctions::DiscreteSpace spaceType = MxFunctions::DiscreteSpace::ThicknessSpace);

		//编辑离散场
		void editDiscreteFieldPanel(int fieldID = 0);

		//取消编辑离散场面板
		void uneditDiscreteFieldPanel();

	public slots:
	    void onTypeChanged(int index);
	    void onLocateChanged();

	    void slot_angelDataTable();
	    void slot_thickDataTable();
	    void slot_coorDataTable();

    protected:
        //离散场数据
        MechDiscreteFieldData::mDiscreteField* _discreteFieldData = nullptr;
        int _discreteFieldID = -1;

	private:
	    void initUI();
	    void syncTableDataToField();
	    void updateTypeComboByLocate();
        void updateDefaultThicknessVisibility();
	    /*
	    * 点数据，设置颜色
	    */
	    void updatePointRender(QString str, QVector3D point, QVector3D color);

	    enum class TableDataType { None, Angel, Thick, Coor };
	    TableDataType _currentTableType = TableDataType::None;

		

        QVBoxLayout* mainLayout = nullptr;
		//定义位置
		QRadioButton* _radioElement = nullptr;
		QRadioButton* _radioNode = nullptr;

		//类型下拉框
		QComboBox* _comboType = nullptr;
		QComboBox* _comboTypeNodeOnly = nullptr;

        FoldSection* defineFold = nullptr;

		//堆叠容器，用于切换三种场类型的界面
		QStackedWidget* _stackedWidget = nullptr;

		//角度场页面
		QWidget* _pageAngel = nullptr;
		QLineEdit* _editDefaultAngel = nullptr;
		QPushButton* _btnAngelTable = nullptr;

		//厚度场页面
		QWidget* _pageThickness = nullptr;
		QLineEdit* _editDefaultThickness = nullptr;
		QPushButton* _btnThicknessTable = nullptr;
        PanelLabel* _labelDefaultThickness = nullptr;

		//坐标系场页面
		QWidget* _pageCoor = nullptr;
		QHBoxLayout* _coorLayout = nullptr;
		QComboBox* _comboCoorType = nullptr;
		PanelLabel* _coorTypeLabel = nullptr;
		QWidget* _coorWidget = nullptr;

        //计算点控件
        mPointPickerWidget* _directPoint1Widget = nullptr;
        mPointPickerWidget* _directPoint2Widget = nullptr;
        mPointPickerWidget* _directPoint3Widget = nullptr;

		QPushButton* _btnCoorTable = nullptr;

    private:
        //退出编辑时失活并清空所有计算点控件
        void resetDirectPointPickers();
        //按当前页面自动激活/取消激活计算点控件
        void updatePickerActivationByPage();

        mDiscreteFieldTableWidget* table = nullptr;
	};

    class mDiscreteFieldItemDelegate : public QItemDelegate
    {
        Q_OBJECT
    public:
        explicit mDiscreteFieldItemDelegate(QObject* parent = nullptr) : QItemDelegate(parent) {}

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QWidget* editor = QItemDelegate::createEditor(parent, option, index);
            QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
            if (lineEdit)
            {
                if (index.column() == 0)
                    lineEdit->setValidator(new QRegExpValidator(QRegExp("^-?[0-9]+$"), lineEdit));
                else
                {
                    // 获取列名
                    QString colName = index.model()->headerData(index.column(), Qt::Horizontal).toString();
                    if (colName != "厚度")
                        lineEdit->setValidator(new QRegExpValidator(QRegExp("^[0-9.,-]+$"), lineEdit));
                    else
                        lineEdit->setValidator(new QRegExpValidator(QRegExp("^[0-9.-]+$"), lineEdit));
                }
                    
            }
            return editor;
        }

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
        {
            // 先提交当前编辑器内容到当前单元格
            QItemDelegate::setModelData(editor, model, index);

            // 对第 1 列及之后的列，批量同步到所有选中的同列单元格
            int col = index.column();
            if (col <= 0) return;

            QTableWidget* table = qobject_cast<QTableWidget*>(model->parent());
            if (!table) return;

            QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
            if (!lineEdit) return;

            // 在编辑器关闭前保存选区，避免 closeEditor 后选区被清空导致批量同步失败
            QString newText = lineEdit->text();
            QList<QTableWidgetItem*> prevSelected = table->selectedItems();

            bool blocked = table->blockSignals(true);
            for (QTableWidgetItem* selectedItem : prevSelected)
            {
                if (selectedItem && selectedItem->column() == col && selectedItem->row() != index.row())
                {
                    selectedItem->setText(newText);
                }
            }
            table->blockSignals(blocked);

            // 强制刷新视图，确保批量修改后的值显示出来
            table->viewport()->update();
        }
    };

    // 支持回车换行/最后一行回车添加新行的表格控件
    class mDiscreteFieldTableEdit : public QTableWidget
    {
        Q_OBJECT
    public:
        explicit mDiscreteFieldTableEdit(QWidget* parent = nullptr) : QTableWidget(parent) {}

    protected:
        void keyPressEvent(QKeyEvent* event) override
        {
            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            {
                QModelIndex editIdx = currentIndex();
                QList<QTableWidgetItem*> prevSelected = selectedItems();
                bool editorCommitted = false;
                QString newValue;

                QWidget* editor = indexWidget(editIdx);
                if (editor)
                {
                    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
                    if (lineEdit) newValue = lineEdit->text();
                    commitData(editor);
                    closeEditor(editor, QAbstractItemDelegate::NoHint);
                    editorCommitted = true;
                }
                else if (state() == QAbstractItemView::EditingState)
                {
                    QTableWidget::keyPressEvent(event);
                    editorCommitted = true;
                    if (editIdx.isValid())
                    {
                        QTableWidgetItem* curItem = item(editIdx.row(), editIdx.column());
                        if (curItem) newValue = curItem->text();
                    }
                }

                if (editorCommitted && editIdx.isValid() && editIdx.column() > 0)
                {
                    batchSyncCurrentColumn(editIdx, prevSelected, newValue);
                }

                handleEnter();
                return;
            }
            QTableWidget::keyPressEvent(event);
        }

    private:
        void handleEnter()
        {
            QModelIndexList selected = selectedIndexes();
            if (selected.isEmpty()) return;

            QModelIndex lastIdx = selected.last();
            int nextRow = lastIdx.row() + 1;
            int col = lastIdx.column();

            if (nextRow >= rowCount())
            {
                emit requestAddRow();
                nextRow = rowCount() - 1;
            }

            QTableWidgetItem* nextItem = item(nextRow, col);
            if (!nextItem) return;

            setCurrentCell(nextRow, col, QItemSelectionModel::ClearAndSelect);
            scrollToItem(nextItem, QAbstractItemView::EnsureVisible);
        }

        void batchSyncCurrentColumn(const QModelIndex& editIdx, const QList<QTableWidgetItem*>& prevSelected, const QString& newValue)
        {
            int col = editIdx.column();
            bool blocked = blockSignals(true);
            for (QTableWidgetItem* selectedItem : prevSelected)
            {
                if (selectedItem && selectedItem->column() == col && selectedItem->row() != editIdx.row())
                {
                    selectedItem->setText(newValue);
                }
            }
            blockSignals(blocked);
            viewport()->update();
        }

    signals:
        void requestAddRow();
    };

    class mDiscreteFieldTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit mDiscreteFieldTableWidget(QWidget* parent = nullptr);

        void setHeadList(const QStringList& heads, int columnWidth = 105);
        void setAngelData(std::map<int, QString> twoColData);
        std::map<int, QString> getAngelData();

        void setThickData(std::map<int, QString> twoColData);
        std::map<int, QString> getThickData();
 
        void setCoorData(std::map<int, QStringList> multColData);
        std::map<int, QStringList> getCoorData();

        
        bool setCurrentIndex(int row, int index);
        void clearTableData();

    signals:
        void rowAdded(int row);
        void rowRemoved(int row);
        void tableCleared();
        void rowCountSetByInput();
        void cellTextChanged();
        void currentTextSet(int row, int col, QString& text);

    private slots:
        void addRow();
        void removeRow();
        void clearTable();
        void setRowByInput();
        bool setCurrentText(int row, int col, QString& text);

    private:
        void initUI();

    private:
        QPushButton* m_btnAdd;
        QPushButton* m_btnRemove;
        QPushButton* m_btnClear;
        QLineEdit* m_editRowNum;
        mDiscreteFieldTableEdit* m_table;
        mDiscreteFieldItemDelegate* m_delegate = nullptr;

        std::map<int, QString> _angelMapData;
        std::map<int, QString> _thickMapData;
        std::map<int, QStringList> _coorMapData;

    };

}
