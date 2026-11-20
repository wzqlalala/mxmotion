#include "mMaterialWindow.h"
#include "ui_mMaterialWindow.h"
#include "mMaterialModel.h"
#include "mCurveWindow.h"
#include "mInfoWindow.h"
#include "mMechGlobalSignals.h"
#include "mGlobalSignals.h"
//QT
#include <QToolTip>
#include <QStackedLayout>
#include <QListView>
#include <QStringListModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QCompleter>
#include <QSplitter>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QStyledItemDelegate>

//MxFunctions
#include "mUnitEnum.h"
#include "mOtherFunction.h"

//Basicdata
#include "mMaterialBase.h"

//C++
#include <set>

using namespace MxFunctions;
namespace MechMainWindow
{
	mMaterialWindow::mMaterialWindow(QWidget* parent)
		: QWidget(parent)
		, ui(new Ui::mMaterialWindowClass())
	{
		ui->setupUi(this);
        this->setStyleSheet(R"(
QWidget{background:#F0F3F6;}
QComboBox
{
    background-color: white;
}
QLineEdit
{
    font-family: 'Microsoft YaHei';
    font-size:12px;
	height:24px;
	border:1px solid #dbe5ee;
	border-radius:4px;
	background-color: white;
}
QLineEdit:hover
{
	color:#303313;
	border: 1px solid #909399;
}
QLineEdit:focus
{
	color:#303313;
	border: 1px solid #1385F6;
}
QLineEdit:disabled
{
	background-color: #F5F7FA;
	color: #A8ABB2;
}
QPushButton
{
font-size:12px;
font-family: 'Microsoft YaHei';
	width:80px;
	height:24px;
	background-color:#FFFFFF;
	 
	border:1px solid #DBE5EE;
	border-radius:3px;
}
QPushButton:hover
{
	background-color:#F0F3F6;
	border-radius:4px;
	border:1px solid #DCDFE6;
}
QPushButton:pressed,
QPushButton:checked
{
	background-color:#D0E7FD;
	border-radius:4px;
	border:1px solid #1385F6;
}
QPushButton:disabled
{
	color:#A8ABB2;
	background-color:#F5F7FA;
	border-radius:4px;
}
)");
		this->setWindowTitle("工程材料库");
		QIcon icon(":/mMainWidgets/Icon/logo.png"); // 或者使用其他资源
		this->setWindowIcon(icon);
		m_MaterialService = new mMaterialService(this);
		setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
		setWindowFlags(Qt::Window); // 保持在最前
		// 或者更明确地禁用最小化按钮
		setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);

		QIcon icon1(":/mMainWidgets/Icon/daoru3x.png");
		ui->pushButton_Insert->setIcon(icon1);
		ui->pushButton_Insert->setIconSize(QSize(17, 17));
		QIcon icon2(":/mMainWidgets/Icon/xinjian3x.png");
		ui->pushButton_Add->setIcon(icon2);
		ui->pushButton_Add->setIconSize(QSize(17, 17));
		QIcon icon3(":/mMainWidgets/Icon/shanchu3x.png");
		ui->pushButton_Remove->setIcon(icon3);
		ui->pushButton_Remove->setIconSize(QSize(17, 17));
		initConnect();
		initPropertyTables();
		initShow();
		initMaterialTree();
		initCurveTable();

		ui->pushButton_Insert->setEnabled(false);
		ui->pushButton_Remove->setEnabled(false);
	}

	mMaterialWindow::~mMaterialWindow()
	{
		delete ui;
	}

	void mMaterialWindow::initShow()
	{
		ui->lineEdit_Filter->setPlaceholderText("输入关键字");
		ui->comboBox_Type->hide();
		ui->lineEdit_Type->hide();
		ui->lineEdit_Type->setPlaceholderText("输入材料类型");
		ui->lineEdit_Name->setPlaceholderText("输入材料名");
		ui->lineEdit_Name->hide();
		ui->label_2->hide();
		ui->label_3->hide();
		ui->tableWidget_Curve->hide();
		ui->widget_Curve->hide();
		ui->pushButton_Sure->hide();
        ui->pushButton_Cancel->hide();
		ui->splitter_2->widget(1)->setVisible(false);
		ui->comboBox_Type->clear();
		ui->comboBox_Type->setView(new QListView);
		ui->comboBox_Type->addItem("自定义");
		auto alltype = m_MaterialService->getAllMaterialTypes();
		for (auto type : alltype)
		{
			ui->comboBox_Type->addItem(type);
		}
		connect(ui->comboBox_Type, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, [this](int idx) {
				bool custom = (idx == 0);
				ui->lineEdit_Type->setDisabled(!custom);
				if (!custom)
				{
					ui->lineEdit_Type->setText(ui->comboBox_Type->currentText());
				}
			});
		ui->comboBox_Type->setCurrentIndex(0);

		ui->label_Name->setText("材料名称：");
		ui->label_MatName->setText("--");
	}

	void mMaterialWindow::initMaterialTree()
	{
        ui->treeWidget_Materials->setStyleSheet(R"(
	QTreeWidget
	{
	font-size:12px;
    background: #FFFFFF;
	}
	QTreeWidget::item {

	}
	QTreeWidget::item:hover
	{
	color:#303313;	
	background-color: #F0F3F6;	
	}
	QTreeWidget::item:selected
	{
	color:#303313;	
	background-color: #D0E7FD;	
	}
	QTreeWidget::branch:hover
	{	
	height:30px;	/*item高度*/
	margin:0px;		/*item间距*/
	border-left:0px;
	border-top:0px;
	margin-left:0px;		/*item间距*/

	margin-right:0px;		/*item间距*/
	margin-top:0px;		/*item间距*/
	margin-bottom:0px;		/*item间距*/
	background-color: transparent;
	}
	QTreeWidget::branch:selected
	{	
	height:30px;	/*item高度*/
	margin:0px;		/*item间距*/
	border-left:0px;
	border-top:0px;
	margin-left:0px;		/*item间距*/

	margin-right:0px;		/*item间距*/
	margin-top:0px;		/*item间距*/
	margin-bottom:0px;		/*item间距*/
	background-color: #FFFFFF;	
	}
    QTreeWidget::branch:open:has-children:hover {
        image: url(:/mMainWidgets/Icon/expand.png);
    }
    QTreeWidget::branch:open:has-children:selected {
        image: url(:/mMainWidgets/Icon/expand.png);
    }
    QTreeWidget::branch:closed:has-children:hover {
        image: url(:/mMainWidgets/Icon/fold.png);
    }
  QTreeWidget::branch:closed:has-children:selected {
        image: url(:/mMainWidgets/Icon/fold.png);
    }
)");
		ui->treeWidget_Materials->setFrameShape(QFrame::NoFrame);
		ui->treeWidget_Materials->setEditTriggers(QAbstractItemView::NoEditTriggers);
		ui->treeWidget_Materials->setSelectionMode(QAbstractItemView::ExtendedSelection);
		ui->treeWidget_Materials->setHeaderLabels({ "Name" });
		ui->treeWidget_Materials->setRootIsDecorated(true);
		ui->treeWidget_Materials->setUniformRowHeights(true);
		ui->treeWidget_Materials->setIndentation(20);
		ui->treeWidget_Materials->header()->setStretchLastSection(true);
		ui->treeWidget_Materials->header()->setSectionResizeMode(0, QHeaderView::Stretch);
		ui->treeWidget_Materials->setHeaderHidden(true);

		reloadTreeWidgetFromModel(); 

		connect(ui->treeWidget_Materials->selectionModel(), &QItemSelectionModel::selectionChanged,
			this, &mMaterialWindow::slot_MaterialSelected);

		connect(ui->lineEdit_Filter, &QLineEdit::textChanged, this, [this](const QString& text) {
			MaterialFilter f{ text };
			m_MaterialService->setFilter(f);
			reloadTreeWidgetFromModel();
			if (!text.isEmpty())expandParentNodes();
			else collapseAllParentNodes();
			});

		ui->treeWidget_Materials->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(ui->treeWidget_Materials, &QTreeWidget::customContextMenuRequested,
			this, &mMaterialWindow::onTreeContextMenu);

		// 构造函数里
		ui->treeWidget_Materials->setEditTriggers(QAbstractItemView::NoEditTriggers); // 1. 默认不许编辑

		/*connect(ui->treeWidget_Materials, &QTreeWidget::itemDoubleClicked,
			this, &mMaterialWindow::startRename);

		connect(ui->treeWidget_Materials, &QTreeWidget::itemChanged,
			this, &mMaterialWindow::onItemEdited);*/
	}

	void mMaterialWindow::onTreeContextMenu(const QPoint& pos)
	{
		// 先找节点
		QTreeWidgetItem* item = ui->treeWidget_Materials->itemAt(pos);
		if (!item) return;          // 在空白处右键，直接忽略

		// 构造菜单
		QMenu menu(this);
		QAction* actCopy = menu.addAction("复制材料");

		// 弹菜单
		QAction* selected = menu.exec(ui->treeWidget_Materials->mapToGlobal(pos));
		if (!selected) return;

		if (selected == actCopy)
		{
			slot_copyMaterial();
		}
	}

	void mMaterialWindow::startRename(QTreeWidgetItem* item, int col)
	{
		if (!item) return;
		ui->treeWidget_Materials->openPersistentEditor(item, col);
		QWidget* editor = ui->treeWidget_Materials->itemWidget(item, col);
		if (editor) {
			editor->installEventFilter(this);          // 拦截键盘/焦点
			if (auto* le = qobject_cast<QLineEdit*>(editor))
				le->selectAll();
		}
	}

	bool mMaterialWindow::eventFilter(QObject* obj, QEvent* ev)
	{
		QWidget* editor = qobject_cast<QWidget*>(obj);
		if (!editor) return false;

		// 回车
		if (ev->type() == QEvent::KeyPress) {
			auto* ke = static_cast<QKeyEvent*>(ev);
			if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
				// 延迟关闭
				QMetaObject::invokeMethod(this, [this, editor]() {
					ui->treeWidget_Materials->closePersistentEditor(
						ui->treeWidget_Materials->currentItem(),
						ui->treeWidget_Materials->currentColumn());
					}, Qt::QueuedConnection);
				return true;      // 吃掉回车
			}
		}
		// 失焦
		else if (ev->type() == QEvent::FocusOut) {
			QMetaObject::invokeMethod(this, [this, editor]() {
				ui->treeWidget_Materials->closePersistentEditor(
					ui->treeWidget_Materials->currentItem(),
					ui->treeWidget_Materials->currentColumn());
				}, Qt::QueuedConnection);
		}
		return QObject::eventFilter(obj, ev);
	}

	// 真正提交逻辑（只在这里处理重命名）
	void mMaterialWindow::onItemEdited(QTreeWidgetItem* item, int col)
	{
		int id = item->data(0, Qt::UserRole).toInt();
		Material mat = m_MaterialService->materialById(id - 1);
		QString oldName = mat.name;
		QString newName = item->text(col).trimmed();
		if (m_MaterialService->findMateiral(newName))
		{
			item->setText(0, oldName);
			return;
		}
		renameMaterial(oldName, newName);
	}

	void mMaterialWindow::reloadTreeWidgetFromModel()
	{
		//屏蔽itemChanged，避免刷新模型带来新的信号
		disconnect(ui->treeWidget_Materials, &QTreeWidget::itemChanged,
			this, &mMaterialWindow::onItemEdited);

		ui->treeWidget_Materials->clear();
		const QAbstractItemModel* m = m_MaterialService->model();
		if (!m) return;

		// 1. 先放顶层
		for (int i = 0; i < m->rowCount(); ++i) {
			QModelIndex idx = m->index(i, 0);
			if (!idx.isValid()) continue;

			QTreeWidgetItem* top = new QTreeWidgetItem(ui->treeWidget_Materials);
			// 顶层 UserRole 存纯分类名（模型顶层原本无 UserRole 数据）：
			// 显示文本带数量后缀（如"钢(12)"）后仍可用于展开状态记录
			QString catName = idx.data(Qt::DisplayRole).toString();
			top->setData(0, Qt::UserRole, catName);
			top->setText(0, catName);

			// 2. 再放这层节点的孩子（仅一层）
			for (int j = 0; j < m->rowCount(idx); ++j) {
				QModelIndex childIdx = m->index(j, 0, idx);
				if (!childIdx.isValid()) continue;

				QTreeWidgetItem* child = new QTreeWidgetItem(top);
				child->setText(0, childIdx.data(Qt::DisplayRole).toString());
				child->setData(0, Qt::UserRole, childIdx.data(Qt::UserRole));
			}

			// 分类节点显示该分类下的材料数量
			top->setText(0, QStringLiteral("%1（%2）").arg(catName).arg(top->childCount()));

			if (expandedItems.contains(catName))
			{
				top->setExpanded(true);
			}
		}

		ui->comboBox_Type->clear();
		ui->comboBox_Type->addItem("自定义");
		auto alltype = m_MaterialService->getAllMaterialTypes();
		for (auto type : alltype)
		{
			ui->comboBox_Type->addItem(type);
		}

		connect(ui->treeWidget_Materials, &QTreeWidget::itemChanged,
			this, &mMaterialWindow::onItemEdited);
	}

	void mMaterialWindow::expandParentNodes()
	{
		for (int i = 0; i < ui->treeWidget_Materials->topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = ui->treeWidget_Materials->topLevelItem(i);
			if (item)
			{
				ui->treeWidget_Materials->expandItem(item);
			}
		}
	}

	void mMaterialWindow::collapseAllParentNodes()
	{
		for (int i = 0; i < ui->treeWidget_Materials->topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = ui->treeWidget_Materials->topLevelItem(i);
			if (item)
			{
				ui->treeWidget_Materials->collapseItem(item);
			}
		}
	}

	void mMaterialWindow::renameMaterial(const QString& oldname, const QString& newname)
	{
		m_MaterialService->renameMaterial(oldname, newname);
	}

	void mMaterialWindow::initCurveTable()
	{
		ui->tableWidget_Curve->setEditTriggers(QAbstractItemView::NoEditTriggers);
		ui->tableWidget_Curve->setColumnCount(3);
		ui->tableWidget_Curve->verticalHeader()->setHidden(false);
		ui->tableWidget_Curve->resizeColumnsToContents();
		QStringList headerText;
		headerText << QString("序号") << QString("X") << QString("Y");
		ui->tableWidget_Curve->setHorizontalHeaderLabels(headerText);
		ui->tableWidget_Curve->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
		for (int c = 0; c < 3; ++c)
		{
			ui->tableWidget_Curve->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
		}
	}

	void mMaterialWindow::initConnect()
	{
		connect(ui->pushButton_Add, &QPushButton::clicked, this, &mMaterialWindow::slot_showAddMaterial);
		connect(ui->pushButton_Sure, &QPushButton::clicked, this, &mMaterialWindow::slot_addMaterial);
		connect(ui->pushButton_Cancel, &QPushButton::clicked, this, &mMaterialWindow::slot_cancelAddMaterial);
		connect(ui->pushButton_Remove, &QPushButton::clicked, this, &mMaterialWindow::slot_removeMaterial);
		connect(ui->pushButton_Insert, &QPushButton::clicked, this, &mMaterialWindow::slot_insertMaterial);

	}

	void mMaterialWindow::initPropertyTables()
	{
		auto wid = ui->scrollArea->widget();
		wid->setStyleSheet("background:white;");
		QVBoxLayout* vbox = qobject_cast<QVBoxLayout*>(wid->layout());
		if (vbox)
		{
			vbox->setContentsMargins(0, 0, 0, 0);
			vbox->setSpacing(0);
		}
		m_ptCommon = new PropertyTable("通用",this);
		m_ptStructral = new PropertyTable("结构", this);
		m_ptThermal = new PropertyTable("传热", this);
		m_ptElectric = new PropertyTable("导电", this);

		connect(m_ptCommon, &PropertyTable::sg_showCurve, this, &mMaterialWindow::slot_showCurve);
		connect(m_ptStructral, &PropertyTable::sg_showCurve, this, &mMaterialWindow::slot_showCurve);
		connect(m_ptThermal, &PropertyTable::sg_showCurve, this, &mMaterialWindow::slot_showCurve);
		connect(m_ptElectric, &PropertyTable::sg_showCurve, this, &mMaterialWindow::slot_showCurve);

		wid->layout()->addWidget(m_ptCommon);
		wid->layout()->addWidget(m_ptStructral);
		wid->layout()->addWidget(m_ptThermal);
		wid->layout()->addWidget(m_ptElectric);

		if (vbox) vbox->addStretch(1);
	}

	void mMaterialWindow::slot_MaterialSelected(const QItemSelection& selected,const QItemSelection& deselected)
	{
		ui->comboBox_Type->hide();
		ui->lineEdit_Type->hide();
		ui->lineEdit_Name->hide();
		ui->label_2->hide();
		ui->label_3->hide();
		ui->pushButton_Sure->hide();
        ui->pushButton_Cancel->hide();
		ui->label_Name->show();
        ui->label_MatName->show();
		m_ptCommon->initPropsShowTable();
		m_ptStructral->initPropsShowTable();
		m_ptThermal->initPropsShowTable();
		m_ptElectric->initPropsShowTable();
		ui->tableWidget_Curve->hide();
		ui->widget_Curve->hide();
		ui->splitter_2->widget(1)->setVisible(false);
		ui->label_Name->setText("材料名称：");
		ui->label_MatName->setText("--");
		QList<QTreeWidgetItem*> selecteditems = ui->treeWidget_Materials->selectedItems();
		ui->pushButton_Insert->setEnabled(selecteditems.size() == 1 && selecteditems.first()->parent());
		ui->pushButton_Remove->setEnabled(selecteditems.size() > 0 && selecteditems.first()->parent());

		disconnect(m_ptCommon, &PropertyTable::sg_editFinished, this, &mMaterialWindow::slot_updateMaterial);
		disconnect(m_ptStructral, &PropertyTable::sg_editFinished, this, &mMaterialWindow::slot_updateMaterial);
		disconnect(m_ptThermal, &PropertyTable::sg_editFinished, this, &mMaterialWindow::slot_updateMaterial);
		disconnect(m_ptElectric, &PropertyTable::sg_editFinished, this, &mMaterialWindow::slot_updateMaterial);
		if (selecteditems.size()!=1)
		{
			return;
		}

		QTreeWidgetItem* item = selecteditems.first();
		curSelectedItem = item;
		// 2. 如果是顶层节点，只做展开/折叠
		QTreeWidgetItem* parent = item->parent();
		if (!parent)                 // 顶层
		{
			// 用 UserRole 中的纯分类名记录展开状态，避免带上数量后缀后失配
			QString catName = item->data(0, Qt::UserRole).toString();
			if (ui->treeWidget_Materials->isItemExpanded(item))
			{
				ui->treeWidget_Materials->collapseItem(item);
				expandedItems.remove(catName);
			}
			else
			{
				ui->treeWidget_Materials->expandItem(item);
				expandedItems.insert(catName);
			}
			return;
		}

		// 3. 非顶层 → 拿 id 显示属性
		int id = item->data(0, Qt::UserRole).toInt();
		Material mat = m_MaterialService->materialById(id - 1);
		showProperty(mat);

		connect(m_ptCommon, &PropertyTable::sg_editFinished, this, &mMaterialWindow::slot_updateMaterial, Qt::UniqueConnection);
		connect(m_ptStructral, &PropertyTable::sg_editFinished, this, &mMaterialWindow::slot_updateMaterial, Qt::UniqueConnection);
		connect(m_ptThermal, &PropertyTable::sg_editFinished, this, &mMaterialWindow::slot_updateMaterial, Qt::UniqueConnection);
		connect(m_ptElectric, &PropertyTable::sg_editFinished, this, &mMaterialWindow::slot_updateMaterial, Qt::UniqueConnection);
	}

	void mMaterialWindow::slot_showCurve(const MaterialCurve& curve)
	{
		ui->tableWidget_Curve->show();
		ui->widget_Curve->show();
		ui->splitter_2->widget(1)->setVisible(true);
		//取出曲线
		auto curves = curve.curves;


		std::set<double>setX;
		QHash<double, QHash<int, double>>hashXY;

		//列更新
		ui->tableWidget_Curve->setRowCount(0);
		QStringList headerText;
		QString xname = "X";
		if (curve.xUnit == "K")
		{
			xname = "温度";
		}
		headerText << QString("序号") << xname;
		int col = 0;
		for (auto it = curves.constBegin(); it != curves.constEnd(); it++)
		{
			headerText <<  it.key();

			for (int i = 0; i < it.value().x.size(); ++i)
			{
				setX.insert(it.value().x[i]);
				hashXY[it.value().x[i]].insert(2 + col, it.value().y[i]);
			}
			++col;
		}
		ui->tableWidget_Curve->setColumnCount(2 + curves.size());
		ui->tableWidget_Curve->setHorizontalHeaderLabels(headerText);

		
		//加载值和x
		for (auto x : setX)
		{
			int row = ui->tableWidget_Curve->rowCount();
			ui->tableWidget_Curve->insertRow(row);

			//id
			QTableWidgetItem* item0 = new QTableWidgetItem(MxFunctions::mNumberToString(row + 1));
			item0->setTextAlignment(Qt::AlignCenter);
			ui->tableWidget_Curve->setItem(row, 0, item0);

			//X
			QTableWidgetItem* item1 = new QTableWidgetItem(MxFunctions::mNumberToString(x));
			item1->setTextAlignment(Qt::AlignCenter);
			ui->tableWidget_Curve->setItem(row, 1, item1);

			//Y
			for (auto it = hashXY[x].constBegin(); it != hashXY[x].constEnd(); it++)
			{
				QTableWidgetItem* itemy = new QTableWidgetItem(MxFunctions::mNumberToString(it.value()));
				itemy->setTextAlignment(Qt::AlignCenter);
				ui->tableWidget_Curve->setItem(row, it.key(), itemy);
			}
		}

		//画曲线
		if (!m_plot) {
			m_plot = new QwtPlot(ui->widget_Curve);
			m_plot->setCanvasBackground(Qt::white);
			m_plot->setAutoReplot(true);
			m_plot->setMinimumSize(240, 240);  // 或者更小
			QVBoxLayout* lay = new QVBoxLayout(ui->widget_Curve);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->addWidget(m_plot);
		}
		m_plot->detachItems(QwtPlotItem::Rtti_PlotCurve);
		m_plot->setAxisTitle(QwtPlot::xBottom, curve.xUnit);
		m_plot->setAxisTitle(QwtPlot::yLeft, curve.yUnit);
		int index = 0;
		for (auto it = curve.curves.begin(); it != curve.curves.end(); ++it) {
			if (!m_plot->legend()) 
			{
				QwtLegend* legend = new QwtLegend;
				legend->setDefaultItemMode(QwtLegendData::Checkable);
				m_plot->insertLegend(legend, QwtPlot::RightLegend);
			}
			QwtPlotCurve* c = new QwtPlotCurve(it.key());
			c->setSamples(it.value().x, it.value().y);
			c->setPen(QColor::fromHsv((index * 60) % 360, 255, 200), 2);
			c->attach(m_plot);
			
			++index;
		}
		m_plot->setAxisAutoScale(QwtPlot::xBottom, true);
		m_plot->setAxisAutoScale(QwtPlot::yLeft, true);
		m_plot->replot();

	}

	void mMaterialWindow::slot_showAddMaterial()
	{
		//新增界面
		ui->label_Name->hide();
        ui->label_MatName->hide();
		ui->tableWidget_Curve->hide();
		ui->widget_Curve->hide();
		ui->comboBox_Type->show();
		ui->lineEdit_Type->show();
		ui->lineEdit_Name->show();
		ui->label_2->show();
		ui->label_3->show();
		ui->pushButton_Sure->show();
        ui->pushButton_Cancel->show();
		m_ptCommon->initPropsAddTable();
		m_ptStructral->initPropsAddTable();
		m_ptThermal->initPropsAddTable();
		m_ptElectric->initPropsAddTable();
	}

	void mMaterialWindow::slot_addMaterial()
	{
		//确定材料名称
		QString materialname = ui->lineEdit_Name->text().trimmed();
		if (materialname.isEmpty())
		{
			//材料名为空
			return;
		}
		//确定材料类型
		QString materialtype= ui->lineEdit_Type->text();
		if (materialtype.isEmpty())
		{
			//材料类型为空
			return;
		}
		//获取各属性并赋予属性类型
		QHash<QString, MaterialProperty> allProp;
		auto commonprop = m_ptCommon->collectProps();
		auto structralprop = m_ptStructral->collectProps();
		auto thermalprop = m_ptThermal->collectProps();
		auto electricprop = m_ptElectric->collectProps();
		for (auto it=commonprop.begin();it!=commonprop.end();it++)
		{
			it.value().category = "通用";
			allProp.insert(it.key(),it.value());
		}
		for (auto it = structralprop.begin(); it != structralprop.end(); it++)
		{
			it.value().category = "结构";
			allProp.insert(it.key(), it.value());
		}
		for (auto it = thermalprop.begin(); it != thermalprop.end(); it++)
		{
			it.value().category = "传热";
			allProp.insert(it.key(), it.value());
		}
		for (auto it = electricprop.begin(); it != electricprop.end(); it++)
		{
			it.value().category = "导电";
			allProp.insert(it.key(), it.value());
		}
		

		//添加材料
		Material mat;
		mat.name = materialname;
		mat.props = allProp;
		mat.category = materialtype;
		if (m_MaterialService->addMaterial(mat))
		{
			ui->lineEdit_Type->clear();
			ui->lineEdit_Name->clear();
			m_ptCommon->clearProps();
			m_ptStructral->clearProps();
			m_ptThermal->clearProps();
			m_ptElectric->clearProps();
		}
		reloadTreeWidgetFromModel();

		mGlobalSignals::getInstance()->outputMessageSig(QString("新建材料 %1").arg(materialname), true);
	}

	void mMaterialWindow::slot_cancelAddMaterial()
	{
		ui->lineEdit_Type->clear();
		ui->lineEdit_Name->clear();
		m_ptCommon->clearProps();
		m_ptStructral->clearProps();
		m_ptThermal->clearProps();
		m_ptElectric->clearProps();

		ui->label_Name->show();
		ui->label_MatName->show();
		ui->label_MatName->setText("--");
		ui->label_Name->setText("材料名称：");
		ui->comboBox_Type->hide();
		ui->lineEdit_Type->hide();
		ui->lineEdit_Name->hide();
		ui->label_2->hide();
		ui->label_3->hide();
		ui->pushButton_Sure->hide();
		ui->pushButton_Cancel->hide();
	}

	void mMaterialWindow::slot_removeMaterial()
	{
		//过滤选中材料
		//当前视图里的选中项
		QList<QTreeWidgetItem*> selecteditems = ui->treeWidget_Materials->selectedItems();
		QSet<int>remids;
		if (selecteditems.size() < 1)
		{
			return;
		}
		int parentcnt = 0;
		for (auto item : selecteditems)
		{
			QTreeWidgetItem* parent = item->parent();
			if (!parent)
			{
				++parentcnt;
			}
		}
		if (parentcnt == selecteditems.size())
		{
			return;
		}
		QMessageBox box(QMessageBox::Question,
			tr("删除确认"),
			tr("确定要删除所选材料吗？"),
			QMessageBox::NoButton, this);

		QPushButton* yesBtn = box.addButton(tr("是"), QMessageBox::AcceptRole);
		QPushButton* noBtn = box.addButton(tr("否"), QMessageBox::RejectRole);
		box.setDefaultButton(noBtn);

		box.exec();
		if (box.clickedButton() != yesBtn)
			return;

		for (auto item : selecteditems)
		{
			QTreeWidgetItem* parent = item->parent();
			if (!parent)
			{
				continue;
			}
			int id = item->data(0, Qt::UserRole).toInt();
			remids.insert(id);
		}
		m_MaterialService->removeMaterial(remids);
	

		//刷新属性
		ui->label_Name->setText("材料名称：");
		ui->label_MatName->setText("--");
		m_ptCommon->clearProps();
		m_ptStructral->clearProps();
		m_ptThermal->clearProps();
		m_ptElectric->clearProps();
		reloadTreeWidgetFromModel();

		ui->pushButton_Insert->setEnabled(false);
		ui->pushButton_Remove->setEnabled(false);
	}

	void mMaterialWindow::slot_insertMaterial()
	{
		if (curSelectedItem == nullptr)
		{
			return;
		}
		QTreeWidgetItem* parent = curSelectedItem->parent();
		if (!parent)
		{
			return;
		}

		QList<QTreeWidgetItem*> selecteditems = ui->treeWidget_Materials->selectedItems();

		for (int i = 0; i < selecteditems.size(); ++i)
		{
			curSelectedItem = selecteditems[i];
			int id = curSelectedItem->data(0, Qt::UserRole).toInt();
			Material mat = m_MaterialService->materialById(id - 1);
			QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>>props;
			QHash<QString, MaterialProperty> matprop = mat.props;
			QHash<QString, QPair<QVector<Unit>, QVariant>>propcommon;
			QHash<QString, QPair<QVector<Unit>, QVariant>>propstractual;
			QHash<QString, QPair<QVector<Unit>, QVariant>>propthermal;
			QHash<QString, QPair<QVector<Unit>, QVariant>>propelectic;

			for (auto p = matprop.begin(); p != matprop.end(); p++)
			{
				QString propname = p.key();
				QString category = p.value().category;
				QVector<Unit> units ;
				QVariant value;
				if (p.value().valuetype == 0)
				{
					MaterialScalar scalar = p.value().value.value<MaterialScalar>();
					double v = scalar.value;
					value = QVariant::fromValue<double>(v);
					units.append(mOtherFunction::getUnitOfStr(scalar.unit));
				}
				else
				{
					MaterialCurve fromcurve = p.value().value.value<MaterialCurve>();
					QString unitxstr = fromcurve.xUnit;
					QString unitystr = fromcurve.yUnit;
					QVector<QVector<double>>data;
					QVector<double>X;
					QVector<QVector<double>>Y;
					units.append(mOtherFunction::getUnitOfStr(unitxstr));
					for (auto cv = fromcurve.curves.constBegin(); cv != fromcurve.curves.constEnd(); cv++)
					{
						X = cv.value().x;
						Y.append(cv.value().y);
						units.append(mOtherFunction::getUnitOfStr(unitystr));
					}
					data.append(X);
					for (auto y : Y)
						data.append(y);
					value = QVariant::fromValue<QVector<QVector<double>>>(data);
				}
				if (category == "通用")
				{
					propcommon[propname] = { units ,value };
				}
				else if (category == "结构")
				{
					propstractual[propname] = { units ,value };
				}
				else if (category == "传热")
				{
					propthermal[propname] = { units ,value };
				}
				else if (category == "导电")
				{
					propelectic[propname] = { units ,value };
				}
			}
			props[MAT_LibCommon] = propcommon;
			props[MAT_LibStructural] = propstractual;
			props[MAT_LibThermal] = propthermal;
			props[MAT_LibElectic] = propelectic;
			mInfoWindow::getInstance()->addMaterialFromLib(mat.name, props);
		}
	}

	void mMaterialWindow::slot_updateMaterial()
	{
		//获取目标材料
		if (curSelectedItem == nullptr)
		{
			return;
		}
		int id = curSelectedItem->data(0, Qt::UserRole).toInt();
		Material mat = m_MaterialService->materialById(id - 1);

		//获取各属性并赋予属性类型
		QHash<QString, MaterialProperty> allProp;
		auto commonprop = m_ptCommon->getUpdatedProps();
		auto structralprop = m_ptStructral->getUpdatedProps();
		auto thermalprop = m_ptThermal->getUpdatedProps();
		auto electricprop = m_ptElectric->getUpdatedProps();
		for (auto it = commonprop.begin(); it != commonprop.end(); it++)
		{
			it.value().category = "通用";
			allProp.insert(it.key(), it.value());
		}
		for (auto it = structralprop.begin(); it != structralprop.end(); it++)
		{
			it.value().category = "结构";
			allProp.insert(it.key(), it.value());
		}
		for (auto it = thermalprop.begin(); it != thermalprop.end(); it++)
		{
			it.value().category = "传热";
			allProp.insert(it.key(), it.value());
		}
		for (auto it = electricprop.begin(); it != electricprop.end(); it++)
		{
			it.value().category = "导电";
			allProp.insert(it.key(), it.value());
		}

		mat.props = allProp;
	
		m_MaterialService->updateMaterial(mat);
	}

	void mMaterialWindow::slot_copyMaterial()
	{
		//获取目标材料
		if (curSelectedItem == nullptr)
		{
			return;
		}
		QTreeWidgetItem* parent = curSelectedItem->parent();
		int id = curSelectedItem->data(0, Qt::UserRole).toInt();
		m_MaterialService->copyMaterial(id - 1);
		reloadTreeWidgetFromModel();
	}

	void mMaterialWindow::slot_renameMaterial()
	{

	}

	void mMaterialWindow::showProperty(const Material& mat)
	{
		if (mat.props.isEmpty())
		{
			return;
		}
		ui->label_Name->setText("材料名称：");
		ui->label_MatName->setText(mat.name);
		m_ptCommon->clearProps();
		m_ptStructral->clearProps();
		m_ptThermal->clearProps();
		m_ptElectric->clearProps();
		bool hascurve = false;
		PropertyTable* tb = nullptr;
		for (auto p = mat.props.constBegin(); p != mat.props.constEnd(); p++)
		{
			QString category = p.value().category;
			if (category == "通用")
			{
				tb = m_ptCommon;
			}
			else if (category == "结构")
			{
				tb = m_ptStructral;

			}
			else if (category == "传热")
			{
				tb = m_ptThermal;

			}
			else if (category == "导电")
			{
				tb = m_ptElectric;

			}
			tb->addMat(p.key(), p.value());
		}
	}

/**************************************************PropertyTable************************************************************/
	 
	PropertyTable::PropertyTable(const QString& title, QWidget* parent)
	{
		m_btnAdd = new QPushButton("", this);
        m_btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_btnAdd->setIconSize(QSize(14, 14));
		m_btnDel = new QPushButton("", this);
        m_btnDel->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_btnDel->setIconSize(QSize(14, 14));
		m_btnAdd->setFixedWidth(20);
		m_btnDel->setFixedWidth(20);
		m_btnAdd->setFixedHeight(20);
		m_btnDel->setFixedHeight(20);
		QString btnStyle = "QPushButton{"
			"  border:none;"
			"  background-color:transparent;"
			"  color:#606266;"
			"  font-size:16px;"
			"  font-family:\"Microsoft YaHei\";"
			"  font-weight:normal;"
			"  padding:0;"
			"  margin:0;"
			"}"
			"QPushButton:hover{"
			"  background-color:transparent;"
			"  border:none;"
			"}"
			"QPushButton:pressed{"
			"  background-color:transparent;"
			"  border:none;"
			"}";
		m_btnAdd->setStyleSheet(btnStyle);
		m_btnDel->setStyleSheet(btnStyle);
		connect(m_btnAdd, &QPushButton::clicked, this, &PropertyTable::slot_addProp);
		connect(m_btnDel, &QPushButton::clicked, this, &PropertyTable::slot_delProp);

		m_btn = new QPushButton(title, this);
		m_btn->setIcon(getIcon(true));
		m_btn->setLayoutDirection(Qt::LeftToRight);
		m_btn->setContentsMargins(0, 0, 0, 0);
		m_btn->setFixedHeight(24);
		m_btn->setStyleSheet("QPushButton{"
			"  text-align:left; padding:0 2px; border:none; margin:0; background:#F0F3F6;color:#606266;}"
			"QPushButton:hover{background:#F0F3F6;}"
			"QPushButton:checked{background:#F0F3F6;}");
		connect(m_btn, &QPushButton::clicked, this, &PropertyTable::onFoldClicked);

		m_table = new QTableWidget(parent);
  QString styleSheet = R"(
QTableWidget{
font-size:12px;
 background - color: #FFFFFF;

}
QTableWidget::item:selected{
 background-color: #D0E7FD;
 color: #303313;
}
QTableWidget::item:hover{
 background-color: #F0F3F6;
}
QHeaderView::section:horizontal
{
font-size:12px;
    background: #F7F8FA;
    border: 1px solid #E4E7ED;
    border-left: 0;

 padding:4px;
 font-weight:normal;
}
QTableWidget{
  border: 1px solid #E4E7ED;
}
)";
		m_table->setStyleSheet(styleSheet);
		m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

		m_nameDelegate = new PropertyNameDelegate(this);
		m_table->setItemDelegateForColumn(0, m_nameDelegate);

		connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int col) {
			if (col != 0) return;
			QModelIndex index = m_table->model()->index(row, col);
			m_table->edit(index);
		});

		initPropsShowTable();
	
		QHBoxLayout* hLay = new QHBoxLayout();
		hLay->setContentsMargins(0, 0, 0, 0);
		hLay->setSpacing(1);
		hLay->addWidget(m_btn);
		hLay->addWidget(m_btnAdd);
        hLay->addWidget(m_btnDel);
        QVBoxLayout* vLay = new QVBoxLayout(this);
        vLay->setContentsMargins(4, 4, 4, 4);
        vLay->setSpacing(0);
        QWidget* topWidget = new QWidget(this);
        topWidget->setStyleSheet("background-color:#F0F3F6;");
        topWidget->setLayout(hLay);
        vLay->addWidget(topWidget);
        vLay->addWidget(m_table);
		
	}

	PropertyTable::~PropertyTable()
	{
		
	}

	/**************************************************PropertyNameDelegate************************************************************/

	PropertyNameDelegate::PropertyNameDelegate(QObject* parent)
		: QStyledItemDelegate(parent)
		, m_placeholderText(QObject::tr("属性名"))
		, m_placeholderColor(QColor("#8c8d91"))
	{
	}

	bool PropertyNameDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
		const QStyleOptionViewItem& option, const QModelIndex& index)
	{
		return QStyledItemDelegate::editorEvent(event, model, option, index);
	}

	void PropertyNameDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const
	{
		QString text = index.data(Qt::DisplayRole).toString();
		if (!text.isEmpty())
		{
			QStyledItemDelegate::paint(painter, option, index);
			return;
		}

		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);
		opt.text = m_placeholderText;
		opt.palette.setColor(QPalette::Text, m_placeholderColor);
		opt.palette.setColor(QPalette::HighlightedText, m_placeholderColor);

		const QWidget* widget = opt.widget;
		QStyle* style = widget ? widget->style() : QApplication::style();
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
	}

	QWidget* PropertyNameDelegate::createEditor(QWidget* parent,
		const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		QLineEdit* editor = new QLineEdit(parent);
		editor->setFrame(false);
		editor->setAlignment(Qt::AlignCenter);
		editor->setPlaceholderText(m_placeholderText);
		return editor;
	}

	void PropertyNameDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
	{
		QString text = index.data(Qt::EditRole).toString();
		QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
		if (lineEdit)
			lineEdit->setText(text);
	}

	void PropertyNameDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const
	{
		QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
		if (!lineEdit)
			return;
		model->setData(index, lineEdit->text(), Qt::EditRole);
	}

	void PropertyNameDelegate::updateEditorGeometry(QWidget* editor,
		const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		editor->setGeometry(option.rect);
	}

	QIcon PropertyTable::getIcon(bool open)
	{
		qreal dpr = devicePixelRatioF();
		QIcon sys = style()->standardIcon(open ? QStyle::SP_ArrowDown : QStyle::SP_ArrowRight);
		return drawArrowIcon(open, QColor("#909399"), QColor(0, 0, 0, 70), 64, dpr);
	}
	QIcon PropertyTable::drawArrowIcon(bool open, const QColor& fill, const QColor& outline, int px, qreal dpr)
	{
		QPixmap pm(px * dpr, px * dpr);
		pm.setDevicePixelRatio(dpr);
		pm.fill(Qt::transparent);

		QPainter p(&pm);
		p.setRenderHint(QPainter::Antialiasing, true);

		const qreal w = px, h = px;

		QPainterPath path;
		if (open) {
			// ↓ 三角
			path.moveTo(0.20 * w, 0.35 * h);
			path.lineTo(0.80 * w, 0.35 * h);
			path.lineTo(0.50 * w, 0.75 * h);
		}
		else {
			// → 三角
			path.moveTo(0.35 * w, 0.20 * h);
			path.lineTo(0.75 * w, 0.50 * h);
			path.lineTo(0.35 * w, 0.80 * h);
		}
		path.closeSubpath();

		// 填充
		p.setPen(Qt::NoPen);
		p.setBrush(fill);
		p.drawPath(path);

		// 描边（让它不是“纯色块”，边缘更立体）
		if (outline.alpha() > 0) {
			p.setBrush(Qt::NoBrush);
			p.setPen(QPen(outline, 1.2 * dpr));
			p.drawPath(path);
		}

		return QIcon(pm);
	}

	void PropertyTable::onFoldClicked()
	{
		_isopen = !_isopen;
		m_btn->setIcon(getIcon(_isopen));
		m_table->setVisible(_isopen);
	}

	void PropertyTable::onCellChanged(int row, int col)
	{
		emit sg_editFinished();
	}

	void PropertyTable::slot_addProp()
	{
		int row = m_table->rowCount();
		m_table->insertRow(row);
		// 1. 属性名（空字符串，由委托显示占位符）
		QTableWidgetItem* nameItem = new QTableWidgetItem("");
		nameItem->setTextAlignment(Qt::AlignCenter);
		nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
		m_table->setItem(row, 0, nameItem);

		// 2. 值类型下拉框
		QComboBox* typeCombo = new QComboBox;
		typeCombo->setView(new QListView);
		typeCombo->addItems({ "常量", "变量" });
		typeCombo->setStyleSheet(
			"QComboBox { border: none; background: transparent; padding-left: 4px; padding-right: 0px; padding-top: 0px; padding-bottom: 0px; }"
			"QComboBox::drop-down { border: none; width: 18px; subcontrol-position: right center; padding-right: 2px; }"
			"QComboBox QAbstractItemView { border: 1px solid #d0d0d0; background: white; }"
		);
		m_table->setCellWidget(row, 1, typeCombo);

		//3. 单位下拉框
		QComboBox* unitCombo = new QComboBox;
		QComboBox* unitComboX = new QComboBox;
		QComboBox* unitComboY = new QComboBox;
		unitCombo->setObjectName("unit");
		unitComboX->setObjectName("unitX");
		unitComboY->setObjectName("unitY");
		unitCombo->setInsertPolicy(QComboBox::NoInsert);
		unitCombo->setView(new QListView);
		unitCombo->setEditable(true);
		unitComboX->setInsertPolicy(QComboBox::NoInsert);
		unitComboX->setView(new QListView);
		unitComboX->setEditable(true);
		unitComboY->setInsertPolicy(QComboBox::NoInsert);
		unitComboY->setView(new QListView);
		unitComboY->setEditable(true);
		QString unitComboStyle =
			"QComboBox { border: none; background: transparent; padding-left: 4px; padding-right: 0px; padding-top: 0px; padding-bottom: 0px; }"
			"QComboBox::drop-down { border: none; width: 18px; subcontrol-position: right center; padding-right: 2px; }"
			"QComboBox QAbstractItemView { border: 1px solid #d0d0d0; background: white; }"
			"QComboBox QLineEdit { border: none; background: transparent; padding: 0px; }";
		unitCombo->setStyleSheet(unitComboStyle);
		unitComboX->setStyleSheet(unitComboStyle);
		unitComboY->setStyleSheet(unitComboStyle);
		QSet<QString> unitsset;
		QStringList allunits;
		for (int i = 1; i < 25; ++i)
		{
			for (int j = 0; j < MxFunctions::UnitsLibrary[i].size(); ++j)
			{
				unitsset.insert(MxFunctions::UnitsLibrary[i][j]);
			}
		}
        allunits.append(" ");
		for (auto unitit : unitsset)
		{
			allunits.append(unitit);
		}
		allunits.sort();
		//创建代理模型
		auto* srcModel = new QStringListModel(allunits,this);
		auto* proxyModel = new QSortFilterProxyModel(this);
		proxyModel->setSourceModel(srcModel);
		proxyModel->setFilterKeyColumn(0);
		unitCombo->setModel(proxyModel);
		unitCombo->setMaxVisibleItems(10);
		unitCombo->setCurrentIndex(1);
		unitComboX->setModel(proxyModel);
		unitComboX->setMaxVisibleItems(10);
		unitComboX->setCurrentIndex(1);
		unitComboY->setModel(proxyModel);
		unitComboY->setMaxVisibleItems(10);
		unitComboY->setCurrentIndex(1);

		//单位过滤
		QCompleter* completer = new QCompleter(proxyModel, unitCombo);
		unitCombo->setCompleter(completer);
		QCompleter* completerX = new QCompleter(proxyModel, unitComboX);
		unitComboX->setCompleter(completerX);
		QCompleter* completerY = new QCompleter(proxyModel, unitComboY);
		unitComboY->setCompleter(completerY);

		QObject::connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
			unitCombo->lineEdit(), &QLineEdit::setText);
		QObject::connect(completerX, QOverload<const QString&>::of(&QCompleter::activated),
			unitComboX->lineEdit(), &QLineEdit::setText);
		QObject::connect(completerY, QOverload<const QString&>::of(&QCompleter::activated),
			unitComboY->lineEdit(), &QLineEdit::setText);

		QObject::connect(unitCombo->lineEdit(), &QLineEdit::textEdited,
			[proxyModel, completer, unitCombo](const QString& txt) {
				proxyModel->setFilterFixedString(txt);
				completer->complete();
				unitCombo->setEditText(txt);
			});
		QObject::connect(unitComboX->lineEdit(), &QLineEdit::textEdited,
			[proxyModel, completerX, unitComboX](const QString& txt) {
				proxyModel->setFilterFixedString(txt);
				completerX->complete();
				unitComboX->setEditText(txt);
			});
		QObject::connect(unitComboY->lineEdit(), &QLineEdit::textEdited,
			[proxyModel, completerY, unitComboY](const QString& txt) {
				proxyModel->setFilterFixedString(txt);
				completerY->complete();
				unitComboY->setEditText(txt);
			});
		
		QWidget* container = new QWidget;
		container->setStyleSheet(
			"QWidget { background: transparent; }"
			"QWidget:hover { background-color: #F0F3F6; }"
		);
		QHBoxLayout* hLayout = new QHBoxLayout(container);
		hLayout->setContentsMargins(0, 0, 0, 0);
		hLayout->setSpacing(2);
		hLayout->addWidget(unitCombo);
		hLayout->addWidget(unitComboX);
		hLayout->addWidget(unitComboY);
		m_table->setCellWidget(row, 3, container);
		unitComboX->setVisible(false);
		unitComboY->setVisible(false);

		

		// 4. 值单元格容器
		QWidget* valueWidget = new QWidget;
		valueWidget->setStyleSheet("QWidget { background: transparent; }");
		QStackedLayout* stack = new QStackedLayout(valueWidget);
		valueWidget->setLayout(stack);
		// 4-A 常量：LineEdit
		QLineEdit* constEdit = new QLineEdit;
		constEdit->setPlaceholderText("输入值");
		constEdit->setAlignment(Qt::AlignCenter);
		constEdit->setFrame(false);
		constEdit->setStyleSheet(
			"QLineEdit { border: none; background: transparent; padding: 0px; }"
			"QLineEdit:hover { background-color: #F0F3F6; }"
			"QLineEdit:focus { background-color: #F0F3F6; }"
		);
		// 值列只能输入正实数
		QDoubleValidator* constValidator = new QDoubleValidator(1e-12, 1e11, 8, constEdit);
		constValidator->setNotation(QDoubleValidator::ScientificNotation);
		constEdit->setValidator(constValidator);
		stack->addWidget(constEdit);
		// 4-B 变量：PushButton
		QPushButton* varBtn = new QPushButton("导入");
		varBtn->setStyleSheet(
			"QPushButton { border: none; background: transparent; padding: 0px; text-align: center; }"
			"QPushButton:hover { background-color: #F0F3F6; }"
			"QPushButton:pressed { background-color: #D0E7FD; }"
		);
		connect(varBtn, &QPushButton::clicked, this, [this] {
			//打开文件选择
			QString filePath = QFileDialog::getOpenFileName(
				this,
				"选择曲线文件",
				QCoreApplication::applicationDirPath(),
				"csv文件 (*.csv);;txt文件 (*.txt)"
			);
			if (filePath.isEmpty())
			{
				return;
			}
			//构造曲线结构体
			MaterialCurve curve;
			//解析文件
			QString suffix = QFileInfo(filePath).suffix().toLower();
			if (suffix == "csv")
			{
				curve = getCurveFromCSV(filePath);
			}
			else if (suffix == "txt")
			{
				curve = getCurveFromTXT(filePath);
			}
			if (curve.curves.size() == 0)
			{
				//导入失败
				return;
			}

			auto* btn = qobject_cast<QPushButton*>(sender());
			if (btn) btn->setProperty("curve", QVariant::fromValue(curve));

			//导入成功将其设置成已导入
			btn->setText("已导入");
			});
		stack->addWidget(varBtn);
		m_table->setCellWidget(row, 2, valueWidget);

		connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) {
			stack->setCurrentIndex(idx);
			unitCombo->setVisible(idx == 0);
			unitComboX->setVisible(idx != 0);
			unitComboY->setVisible(idx != 0);
			});
		stack->setCurrentIndex(0);
	}

    void PropertyTable::slot_delProp()
    {
        // 1️⃣ 取得所有被选中的行号
        // QItemSelectionModel::selectedRows() 会返回每一列的 QModelIndex，
        // 只取其 row() 即可得到唯一的行号集合。
        const QList<QModelIndex> selIdx = m_table->selectionModel()->selectedRows();

        if (selIdx.isEmpty())
            return;                     // 没有选中任何行 → 直接返回

        // 2️⃣ 把行号提取出来并按降序排序（先删大的，后删小的，防止行号错位）
        QList<int> rowsToRemove;
        rowsToRemove.reserve(selIdx.size());
        for (const QModelIndex& idx : selIdx)
            rowsToRemove.append(idx.row());

        std::sort(rowsToRemove.begin(), rowsToRemove.end(),
            std::greater<int>());   // 降序

        // 3️⃣ 逐行删除（并手动删除 cell‑widget，防止泄漏）
        for (int row : rowsToRemove)
        {
            // ---- ① 先把该行的所有 cell‑widget 删除 ----
            // 列 0：属性名 QLineEdit
            if (auto* w = m_table->cellWidget(row, 0))
                delete w;               // QLineEdit 是通过 setCellWidget 放进去的

            // 列 1：值类型 QComboBox
            if (auto* w = m_table->cellWidget(row, 1))
                delete w;

            // 列 2：值容器（QStackedLayout 包裹的 QWidget）
            if (auto* w = m_table->cellWidget(row, 2))
            {
                // QStackedLayout 中的子控件已经是 w 的子对象，直接 delete w 即可
                delete w;
            }

            // 列 3：单位组合框容器（内部包含 3 个 QComboBox）
            if (auto* container = m_table->cellWidget(row, 3))
            {
                // container 中的 QComboBox、QHBoxLayout 都是它的子对象，delete container 即可
                delete container;
            }

            // ---- ② 再把整行从 QTableWidget 中移除 ----
            m_table->removeRow(row);
        }
    }

	MaterialCurve PropertyTable::getCurveFromCSV(const QString& csvfile)
	{
		MaterialCurve curve;
		QFile f(csvfile);
		if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return curve;
		}
		
		QTextStream ts(&f);
		qint64 posBeforeRead = ts.pos();
		while (!ts.atEnd())
		{
			posBeforeRead = ts.pos();
			QString headerLine = ts.readLine().trimmed();
			if (headerLine.isNull()||!headerLine.startsWith("*"))
			{
				return curve;
			}
			if (headerLine.endsWith(","))
			{
				headerLine.chop(1);
			}
			QStringList header = headerLine.split(',');
			QVector<QVector<double>>points(header.size());
			//读点
			while (!ts.atEnd())
			{
				QString line = ts.readLine().trimmed();
				if (line.isEmpty())
				{
					continue;
				}
				if (line.startsWith("*"))
				{
					ts.seek(posBeforeRead);
					break;
				}
				if (line.endsWith(","))
				{
					line.chop(1);
				}
				posBeforeRead = ts.pos();
				QStringList cols = line.split(',');
				for (int i = 0; i < cols.size(); ++i)
				{
					points[i].append(cols[i].toDouble());
				}
			}
			if (points.size() <= 1)
			{
				return curve;
			}
			for (int p = 1; p < points.size(); ++p)
			{
				MaterialSingleCurve singlecurve;
				singlecurve.x = points[0];
				singlecurve.y = points[p];
				curve.curves[header[p]] = singlecurve;
			}
		}
		return curve;
	}


	MaterialCurve PropertyTable::getCurveFromTXT(const QString& txtfile)
	{
		MaterialCurve curve;

		QFile f(txtfile);
		if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return curve;
		}
		QTextStream ts(&f);
		qint64 posBeforeRead = ts.pos();
		while (!ts.atEnd())
		{
			posBeforeRead = ts.pos();
			QString headerLine = ts.readLine().trimmed();
			if (headerLine.isNull() || !headerLine.startsWith("*"))
			{
				return curve;
			}
			QStringList header = headerLine.mid(1).trimmed().split(',');
			QVector<QVector<double>>points(header.size()+1);
			//读点
			while (!ts.atEnd())
			{
				QString line = ts.readLine().trimmed();
				if (line.isEmpty())
				{
					continue;
				}
				if (line.startsWith("*"))
				{
					ts.seek(posBeforeRead);
					break;
				}
				posBeforeRead = ts.pos();
				QStringList cols = line.split(',');
				for (int i = 0; i < cols.size(); ++i)
				{
					points[i].append(cols[i].toDouble());
				}
			}
			if (points.size() <= 1)
			{
				return curve;
			}
			for (int p = 1; p < points.size(); ++p)
			{
				MaterialSingleCurve singlecurve;
				singlecurve.x = points[0];
				singlecurve.y = points[p];
				curve.curves[header[p-1]] = singlecurve;
			}
		}

		return curve;
	}

	void PropertyTable::addMat(const QString& name, const MaterialProperty& prop)
	{
		int valuetype = prop.valuetype;
		int row = m_table->rowCount();
		m_table->insertRow(row);
		//属性名
		QTableWidgetItem* item = new QTableWidgetItem(name);
		item->setTextAlignment(Qt::AlignCenter);
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 0, item);
		if (valuetype == 0)
		{
			MaterialScalar scalar = prop.value.value<MaterialScalar>();
			
			//属性值
			QTableWidgetItem* item1 = new QTableWidgetItem(MxFunctions::mNumberToString(scalar.value));
			item1->setTextAlignment(Qt::AlignCenter);
			m_table->setItem(row, 1, item1);

			//单位
			QTableWidgetItem* item2 = new QTableWidgetItem(scalar.unit);
			item2->setTextAlignment(Qt::AlignCenter);
			m_table->setItem(row, 2, item2);
		}
		else
		{
			MaterialCurve curve = prop.value.value<MaterialCurve>();
			if (curve.curves.size() == 0)
			{
				return;
			}
			//查看按钮
			QPushButton* btn = new QPushButton("查看", m_table);
			btn->setStyleSheet(
				"QPushButton{"
				"  background-color:#299DFF;"   
				"  color:#FFFFFF;"              
				"  font-weight:bold;"
				"  border:none;"                
				"  padding:3px 8px;"
				"}"
				"QPushButton:hover{"
				"  background-color:#0078D4;"   
				"}"
				"QPushButton:pressed{"
				"  background-color:#005A9E;"   
				"}"
			);
			btn->setProperty("curve", QVariant::fromValue(curve));
			connect(btn, &QPushButton::clicked, this, [this, btn] {
				emit sg_showCurve(btn->property("curve").value<MaterialCurve>());
				});

			m_table->setCellWidget(row, 1, btn);
		}
	}

	void PropertyTable::clearProps()
	{
		m_table->setRowCount(0);
	}

	void PropertyTable::initPropsShowTable()
	{
		m_btnAdd->hide();
        m_btnDel->hide();
        m_table->setRowCount(0);
		m_table->setFixedHeight(200);
		m_table->setColumnCount(3);
		m_table->verticalHeader()->setHidden(false);
		m_table->verticalHeader()->setFixedWidth(40);
        m_table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
		m_table->resizeColumnsToContents();
		QStringList headerText;
		headerText << QString("属性") << QString("值") << QString("单位");
		m_table->setHorizontalHeaderLabels(headerText);
		//设置列宽
		for (int c = 0; c < 3; ++c)
		{
			m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
		}
		connect(m_table, &QTableWidget::cellChanged,
			this, &PropertyTable::onCellChanged, Qt::UniqueConnection);
	}

	void PropertyTable::initPropsAddTable()
	{
		m_btnAdd->show();
        m_btnDel->show();
        m_table->setRowCount(0);
		m_table->setFixedHeight(200);
		m_table->setColumnCount(4);
		m_table->verticalHeader()->setHidden(false);
		m_table->verticalHeader()->setFixedWidth(40);
        m_table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
		m_table->resizeColumnsToContents();
		QStringList headerText;
		headerText << QString("属性") << QString("值类型") << QString("值") << QString("单位");
		m_table->setHorizontalHeaderLabels(headerText);
		//设置列宽
		for (int c = 0; c < 4; ++c)
		{
			m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
		}
		disconnect(m_table, &QTableWidget::cellChanged,
			this, &PropertyTable::onCellChanged);
	}

	QHash<QString, MaterialProperty> PropertyTable::collectProps()
	{
		QHash<QString, MaterialProperty> props;
		int rowcnt = m_table->rowCount();
		for (int row = 0; row < rowcnt; ++row)
		{
			MaterialProperty prop;
			//属性名
			auto* nameItem = m_table->item(row, 0);
			if (!nameItem || nameItem->text().trimmed().isEmpty())
			{
				continue;
			}
			QString name = nameItem->text().trimmed();

			//属性值类型
			auto* typeCombo = qobject_cast<QComboBox*>(m_table->cellWidget(row, 1));
			int valuetype = typeCombo ? typeCombo->currentIndex() : 0;

			//属性值
			QVariant variant;
			auto* valueWidget = m_table->cellWidget(row, 2);
			auto* stack = valueWidget ? valueWidget->findChild<QStackedLayout*>() : nullptr;
			if (!stack) continue;
			QWidget* current = stack->currentWidget();
			
			prop.valuetype = valuetype;
			if (valuetype == 0)//常量
			{
				//获取单位
				QString valueunit = "";
				QWidget* container = qobject_cast<QWidget*>(m_table->cellWidget(row, 3));
				if (container) {
					QComboBox* unitCombo = container->findChild<QComboBox*>("unit");
					valueunit = unitCombo ? unitCombo->currentText() : QString();
				}
				//获取值
				auto* constEdit = qobject_cast<QLineEdit*>(current);
				if (constEdit->text().trimmed() == "")
				{
					continue;
				}
				double val = constEdit ? constEdit->text().trimmed().toDouble() : 0.0;
				prop.value = QVariant::fromValue(MaterialScalar{val,valueunit});
			}
			else//变量
			{
				//获取单位
				QString valueunitX = "";
				QString valueunitY = "";
				QWidget* container = qobject_cast<QWidget*>(m_table->cellWidget(row, 3));
				if (container) {
					QComboBox* unitComboX = container->findChild<QComboBox*>("unitX");
					valueunitX = unitComboX ? unitComboX->currentText() : QString();
					QComboBox* unitComboY = container->findChild<QComboBox*>("unitY");
					valueunitY = unitComboY ? unitComboY->currentText() : QString();
				}

				//从按钮取导入的曲线，没有导入直接pass
				auto* constBtn = qobject_cast<QPushButton*>(current);
				QVariant val = constBtn->property("curve");
				if (!val.isValid() || !val.canConvert<MaterialCurve>())
				{
					continue;
				}
				MaterialCurve curve = val.value<MaterialCurve>();
				curve.xUnit = valueunitX;
				curve.yUnit = valueunitY;
				prop.value = QVariant::fromValue(curve);
			}
			props[name] = prop;
		}
		return props;
	}

	QHash<QString, MaterialProperty> PropertyTable::getUpdatedProps()
	{
		QHash<QString, MaterialProperty> props;
		int rowcnt = m_table->rowCount();
		for (int row = 0; row < rowcnt; ++row)
		{
			MaterialProperty prop;
			//属性名
			QString name = "";
			QTableWidgetItem* item = m_table->item(row, 0);
			if (item)
			{
				name = item->text().trimmed();
			}
			if (name.isEmpty())
			{
				continue;
			}

			//属性值
			QVariant variant;
			auto* valueWidget = m_table->cellWidget(row, 1);
			QPushButton* btn = qobject_cast<QPushButton*>(valueWidget);

			if (btn == nullptr)//常量
			{
				//获取单位
				QTableWidgetItem* item = m_table->item(row, 2);
				QString valueunit = item ? m_table->item(row, 2)->text().trimmed():"";

				//获取值
				item = m_table->item(row, 1);
				double val = item ? item->text().trimmed().toDouble() : 0.0;
				prop.value = QVariant::fromValue(MaterialScalar{ val,valueunit });

				prop.valuetype = 0;
			}
			else//变量
			{
				QVariant v = btn->property("curve");
				prop.value = v;
				prop.valuetype = 1;
			}

			props[name] = prop;
		}
		return props;
	}
}