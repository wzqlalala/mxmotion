#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QEvent>
#include <QStyledItemDelegate>
//QWT
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_legend.h>
#include <qwt_plot_picker.h>

QT_BEGIN_NAMESPACE
namespace Ui { class mMaterialWindowClass; };
QT_END_NAMESPACE

namespace MechMainWindow
{
	class PropertyTable;
	class mMaterialService;
	struct MaterialProperty;
	struct MaterialCurve;
	struct Material;
	struct MaterialFilter;
	class MMAINWINDOW_EXPORT mMaterialWindow : public QWidget
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mMaterialWindow(QWidget* parent = nullptr);
		
		/*
		* 析构函数
		*/
		~mMaterialWindow();

	private:
		
		/*
		* 初始化槽函数
		*/
		void initConnect();

		/*
		* 初始化属性表
		*/
		void initPropertyTables();

		/*
		* 初始化显示
		*/
		void initShow();

		/*
		* 初始化树结构
		*/
		void initMaterialTree();

		/*
		* 初始化曲线表
		*/
		void initCurveTable();

		/*
		* 显示材料属性
		*/
		void showProperty(const Material& mat);

		/*
		* 将数据模型载入treewidget
		*/
		void reloadTreeWidgetFromModel();

		/*
		* 展开所有父节点
		*/
		void expandParentNodes();

		/*
		* 收纳所有父节点
		*/
		void collapseAllParentNodes();

		/*
		* 重命名材料
		*/
		void renameMaterial(const QString& oldname, const QString& newname);

	private slots:
		/*
		* 选中材料
		*/
		void slot_MaterialSelected(const QItemSelection& selected,const QItemSelection& deselected);

		/*
		* 显示曲线
		*/
		void slot_showCurve(const MaterialCurve& curve);

		/*
		* 新增材料显示
		*/
		void slot_showAddMaterial();

		/*
		* 新增材料
		*/
		void slot_addMaterial();

		/*
		* 取消新增材料
		*/
		void slot_cancelAddMaterial();

		/*
		* 移除材料
		*/
		void slot_removeMaterial();

		/*
		* 插入材料
		*/
		void slot_insertMaterial();

		/*
		* 更新材料
		*/
		void slot_updateMaterial();

		/*
		* 复制材料
		*/
		void slot_copyMaterial();

		/*
		* 重命名材料
		*/
		void slot_renameMaterial();

		/*
		* 右键菜单
		*/
		void onTreeContextMenu(const QPoint& pos);

		/*
		* 开始重命名
		*/
		void startRename(QTreeWidgetItem* item, int column);

		/*
		* 编辑
		*/
		void onItemEdited(QTreeWidgetItem* item, int column);

	protected:
		/*
		* 失焦
		*/
		bool eventFilter(QObject* obj, QEvent* ev) override;

	private:
		Ui::mMaterialWindowClass* ui;

		QTreeWidgetItem* curSelectedItem = nullptr;

		//已展开节点
		QSet<QString> expandedItems;
		
		//属性表
		PropertyTable* m_ptCommon = nullptr;
		PropertyTable* m_ptStructral = nullptr;
		PropertyTable* m_ptThermal = nullptr;
		PropertyTable* m_ptElectric = nullptr;

		//材料代理
		mMaterialService* m_MaterialService = nullptr;

		//qwt
		QwtPlot* m_plot = nullptr;
	};

	class MMAINWINDOW_EXPORT PropertyNameDelegate : public QStyledItemDelegate
	{
		Q_OBJECT
	public:
		explicit PropertyNameDelegate(QObject* parent = nullptr);

	protected:
		bool editorEvent(QEvent* event, QAbstractItemModel* model,
			const QStyleOptionViewItem& option, const QModelIndex& index) override;
		void paint(QPainter* painter, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override;
		QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override;
		void setEditorData(QWidget* editor, const QModelIndex& index) const override;
		void setModelData(QWidget* editor, QAbstractItemModel* model,
			const QModelIndex& index) const override;
		void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override;

	private:
		QString m_placeholderText;
		QColor m_placeholderColor;
	};

/**************************************************PropertyTable************************************************************/

	class MMAINWINDOW_EXPORT PropertyTable : public QWidget
	{
		Q_OBJECT
	public:
		PropertyTable(const QString& title, QWidget* parent = nullptr);
		~PropertyTable();

		/*
		* 添加属性
		*/
		void addMat(const QString& name, const MaterialProperty& prop);

		/*
		* 清空表
		*/
		void clearProps();

		/*
		* 初始化表为属性显示表
		*/
		void initPropsShowTable();

		/*
		* 初始化表为属性新增表
		*/
		void initPropsAddTable();

		/*
		* 收集属性
		*/
		QHash<QString, MaterialProperty> collectProps();

		/*
		* 获取更新后的属性
		*/
		QHash<QString, MaterialProperty> getUpdatedProps();

	private:
		QIcon getIcon(bool open);
		QIcon drawArrowIcon(bool open, const QColor& fill, const QColor& outline, int px, qreal dpr);
		/*
		* 解析csv文件
		*/
		MaterialCurve getCurveFromCSV(const QString& csvfile);

		/*
		* 解析txt文件
		*/
		MaterialCurve getCurveFromTXT(const QString& txtfile);

	private slots:
		/*
		* 表折叠
		*/
		void onFoldClicked();

		/*
		* 添加属性
		*/
		void slot_addProp();

        /*
        * 删除属性
        */
        void slot_delProp();

		/*
		* 编辑
		*/
		void onCellChanged(int row, int col);
	signals:
		/*
		* 显示曲线
		*/
		void sg_showCurve(const MaterialCurve& curve);

		/*
		* 编辑完成
		*/
		void sg_editFinished();
	
	private:
		bool _isopen = true;
		QPushButton* m_btn;
		QPushButton* m_btnAdd;
        QPushButton* m_btnDel;
		QTableWidget* m_table;
		PropertyNameDelegate* m_nameDelegate = nullptr;
	};
}