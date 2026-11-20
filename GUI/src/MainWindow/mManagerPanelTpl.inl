//mManagerPanelTpl 模板实现：由 mManagerPanelTpl.h 末尾包含，不单独编译
#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mManagerTableWidget.h"
#include "mCheckRowDelegate.h"

//视口数据
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//QT
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSet>

#include <algorithm>
#include <functional>

namespace mManagerPanelDetail
{
	//配置工况入口按钮配色：默认白色，弹窗激活期间淡蓝
	inline QString matrixEntryButtonStyle(bool active)
	{
		if (active)
		{
			return QStringLiteral(
				"QPushButton { background: #e6f4ff; border: 1px solid #91caff; border-radius: 4px;"
				" color: #1689ee; min-height: 30px; }"
				"QPushButton:hover { background: #d6ecff; border-color: #69b1ff; }"
				"QPushButton:pressed { background: #bae0ff; border-color: #69b1ff; }");
		}
		return QStringLiteral(
			"QPushButton { background: #ffffff; border: 1px solid #d9d9d9; border-radius: 4px;"
			" color: #333333; min-height: 30px; }"
			"QPushButton:hover { background: #f0f7ff; border-color: #91caff; color: #1689ee; }"
			"QPushButton:pressed { background: #e6f4ff; border-color: #69b1ff; }");
	}

	//按钮内的文本/箭头是带独立颜色样式的QLabel，不随按钮样式表继承颜色，需同步切换
	inline void applyMatrixEntryStyle(QPushButton* btn, bool active)
	{
		btn->setStyleSheet(matrixEntryButtonStyle(active));
		const QString textColor = active ? QStringLiteral("#1689ee") : QStringLiteral("#333333");
		const QString arrowColor = QStringLiteral("#1689ee");
		const auto labels = btn->findChildren<QLabel*>();
		for (QLabel* label : labels)
		{
			if (label->objectName() == QLatin1String("matrixEntryArrow"))
			{
				label->setStyleSheet(QStringLiteral("color: %1; background: transparent; font-size: 22px; font-weight: bold;").arg(arrowColor));
			}
			else
			{
				label->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg(textColor));
			}
		}
	}
}

namespace MechMainWindow
{
	template<typename Traits>
	mManagerPanelTpl<Traits>::mManagerPanelTpl(QWidget* parent)
		: mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 2, 2, 2);
		_mainLayout->setSpacing(0);
		_mainLayout->addStretch();

		//对齐HTML原型的列表样式：行背景/复选框对勾由mCheckRowDelegate绘制，
		//样式表仅提供表格边框与表头底色（不再用selection/item背景，避免与行底色规则冲突）
		const QString listStyle = QStringLiteral(R"(
			QTableWidget { border: 1px solid #dce1e8; border-radius: 2px; background: #ffffff; }
			QHeaderView::section { background: #f5f6f8; color: #666d76; border: none;
				border-bottom: 1px solid #dce1e8; border-right: 1px solid #edf0f3; padding: 6px 4px; font-weight: normal; min-height: 28px; }
		)");

		//配置工况载荷入口（默认白色按钮，弹窗激活期间切淡蓝；尾部大号箭头提示进入弹窗）
		_btnMatrixEntry = new QPushButton(this);
		_btnMatrixEntry->setCursor(Qt::PointingHandCursor);
		//按钮内容用布局承载：文本居左、箭头贴右端，箭头单独放大（文本拼空格无法保证贴尾）
		auto* entryLayout = new QHBoxLayout(_btnMatrixEntry);
		entryLayout->setContentsMargins(10, 0, 8, 0);
		auto* entryTextLabel = new QLabel(Traits::matrixEntryText(), _btnMatrixEntry);
		entryTextLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
		auto* entryArrowLabel = new QLabel(QStringLiteral("›"), _btnMatrixEntry);
		entryArrowLabel->setObjectName(QStringLiteral("matrixEntryArrow"));
		entryArrowLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
		entryLayout->addWidget(entryTextLabel);
		entryLayout->addStretch();
		entryLayout->addWidget(entryArrowLabel);
		mManagerPanelDetail::applyMatrixEntryStyle(_btnMatrixEntry, false);

		//折叠框：载荷列表
		_foldWidget = new FoldSection("载荷列表", this);
		m_foldWidgets.append(_foldWidget);

		//筛选行：名称搜索 + 类型下拉（对齐HTML原型顺序，下拉框宽度限制避免Info dock被撑宽）
		auto* filterWidget = new QWidget;
		auto* filterLayout = new QHBoxLayout(filterWidget);
		filterLayout->setContentsMargins(0, 0, 0, 0);
		_editNameFilter = new QLineEdit(filterWidget);
		_editNameFilter->setPlaceholderText(Traits::nameFilterPlaceholder());
		_editNameFilter->setClearButtonEnabled(true);
		_comboType = new QComboBox(filterWidget);
		_comboType->addItem("全部类型");
		//宽度按约8个字符的可见内容调整，不随最长类型名撑宽，也不会被搜索框挤没
		_comboType->setMinimumContentsLength(8);
		_comboType->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		filterLayout->addWidget(_editNameFilter, 1);
		filterLayout->addWidget(_comboType);
		_foldWidget->addRow(filterWidget);

		//表格：名称(复选框) / 类型 / 操作(编辑)
		_tableData = new QTableWidget(_foldWidget);
		_tableData->setColumnCount(3);
		_tableData->setHorizontalHeaderLabels(QStringList() << Traits::nameHeader() << "类型" << "操作");
		//设置默认行高后再固定表头最小宽度：无数据时ResizeToContents列收缩为0会挤住名称列表头
		_tableData->horizontalHeader()->setMinimumWidth(360);
		//分段最小宽度兜底：空表时类型/操作列提示尺寸塌陷导致表头文字被裁切
		_tableData->horizontalHeader()->setMinimumSectionSize(56);
		_tableData->verticalHeader()->setHidden(true);
		//名称列拉伸，类型/操作列按内容收缩，避免宽面板下表头被等分撑散
		_tableData->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		_tableData->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
		_tableData->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
		_tableData->setSelectionBehavior(QAbstractItemView::SelectRows);
		_tableData->setSelectionMode(QAbstractItemView::ExtendedSelection);
		_tableData->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_tableData->setShowGrid(false);
		_tableData->setStyleSheet(listStyle);
		_tableData->setMouseTracking(true);
		//勾选行淡蓝/选中行淡灰/复选框白对勾均由委托绘制
		_tableData->setItemDelegate(new mCheckRowDelegate(_tableData));
		//列表与矩阵共用高度：展开状态留给矩阵空间，表格按内容自适应（约12行+表头）
		_tableData->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
		_tableData->setMinimumHeight(_tableData->horizontalHeader()->sizeHint().height()
			+ _tableData->verticalHeader()->defaultSectionSize() * 6 + 4);
		_foldWidget->addRow(_tableData);

		//底部按钮：全选 + 删除（右对齐，对齐HTML原型）
		auto* btnWidget = new QWidget;
		auto* btnLayout = new QHBoxLayout(btnWidget);
		btnLayout->setContentsMargins(0, 0, 0, 0);
		_btnSelectAll = new QPushButton("全选", btnWidget);
		_btnDelete = new QPushButton("删除", btnWidget);
		_btnDelete->setStyleSheet("QPushButton { color: #bf5151; } QPushButton:disabled { color: #a3a9b1; }");
		btnLayout->addStretch();
		btnLayout->addWidget(_btnSelectAll);
		btnLayout->addWidget(_btnDelete);
		_foldWidget->addRow(btnWidget);

		//将入口按钮与折叠框插入到末尾stretch之前：按钮在上、折叠框在下，多余空白留在折叠块之后
		QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
		if (lastItem != nullptr && lastItem->spacerItem() != nullptr)
		{
			_mainLayout->removeItem(lastItem);
			_mainLayout->addWidget(_btnMatrixEntry);
			_mainLayout->addSpacing(6);
			_mainLayout->addWidget(_foldWidget);
			_mainLayout->addStretch();
		}
		else
		{
			_mainLayout->addWidget(_btnMatrixEntry);
			_mainLayout->addSpacing(6);
			_mainLayout->addWidget(_foldWidget);
		}

		//矩阵弹出对话框按需创建（首次点击入口时），避免面板构造即生成大表格
		_matrixDialog = nullptr;

		//信号连接
		connect(_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { slot_typeFilterChanged(); });
		connect(_editNameFilter, &QLineEdit::textChanged, this, [this](const QString& text) { slot_nameFilterChanged(text); });
		connect(_tableData, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) { slot_tableItemChanged(item); });
		connect(_btnSelectAll, &QPushButton::clicked, this, [this]() { slot_selectAllBtn(); });
		connect(_btnDelete, &QPushButton::clicked, this, [this]() { slot_deleteBtn(); });
		connect(_btnMatrixEntry, &QPushButton::clicked, this, [this]() { slot_matrixEntryBtn(); });
	}

	template<typename Traits>
	mManagerPanelTpl<Traits>::~mManagerPanelTpl()
	{
		if (_matrixDialog != nullptr)
		{
			delete _matrixDialog;
			_matrixDialog = nullptr;
		}
		if (_mainLayout != nullptr)
		{
			delete _mainLayout;
			_mainLayout = nullptr;
		}
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::showManager()
	{
		refreshList();
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::slot_typeFilterChanged()
	{
		refreshList();
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::slot_nameFilterChanged(const QString& text)
	{
		Q_UNUSED(text);
		refreshList();
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::refreshTypeCombo()
	{
		//保留原选中类型，重建后若仍存在则恢复选中
		QString currentType = _comboType->currentText();
		_comboType->blockSignals(true);
		_comboType->clear();
		_comboType->addItem("全部类型");

		//动态列举当前模型中实际存在的类型（按id顺序去重）
		QSet<QString> addedTypes;
		for (int id : Traits::allIds())
		{
			QString typeName = Traits::typeNameOf(id);
			if (!typeName.isEmpty() && !addedTypes.contains(typeName))
			{
				addedTypes.insert(typeName);
				_comboType->addItem(typeName);
			}
		}

		int typeIndex = _comboType->findText(currentType);
		_comboType->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
		_comboType->blockSignals(false);
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::refreshList()
	{
		_syncingCheck = true;

		_tableData->setRowCount(0);

		if (getCVM == nullptr)
		{
			_syncingCheck = false;
			syncActionButtons();
			return;
		}

		refreshTypeCombo();
		//刷新前取消折叠块的最大高度限制，避免内容尺寸被旧上限裁剪后量取
		_foldWidget->contentWidget()->setMaximumHeight(QWIDGETSIZE_MAX);
		QString typeFilter = _comboType->currentText();
		QString nameFilter = _editNameFilter->text();

		// 先按筛选收集待显示对象，再一次性分配行数后填充：
		// 逐行 insertRow 伴随重复的行高/布局重算，对象数多时刷新明显变慢
		QVector<QPair<int, QString>> shownItems;
		for (int id : Traits::allIds())
		{
			//类型筛选
			if (typeFilter != "全部类型" && Traits::typeNameOf(id) != typeFilter)
			{
				continue;
			}
			//名称筛选
			QString name = Traits::nameOf(id);
			if (!nameFilter.isEmpty() && !name.contains(nameFilter, Qt::CaseInsensitive))
			{
				continue;
			}
			shownItems.append(qMakePair(id, name));
		}

		_tableData->setRowCount(static_cast<int>(shownItems.size()));
		for (int row = 0; row < shownItems.size(); ++row)
		{
			const int id = shownItems[row].first;
			const QString& name = shownItems[row].second;

			QTableWidgetItem* nameItem = new QTableWidgetItem(name);
			nameItem->setFlags((nameItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
			nameItem->setCheckState(Qt::Unchecked);
			nameItem->setData(Qt::UserRole, id);
			_tableData->setItem(row, 0, nameItem);

			QTableWidgetItem* typeItem = new QTableWidgetItem(Traits::typeNameOf(id));
			typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
			_tableData->setItem(row, 1, typeItem);

			auto* editBtn = new QPushButton("编辑");
			connect(editBtn, &QPushButton::clicked, this, [this, id]() {
				//跳转到对应类型的编辑面板（取第一个挂载工况作为语境，未挂载为0）
				Traits::editObject(id, Traits::firstCaseId(id));
				//选中管理器树下的对应节点
				Traits::selectManagerItem(id);
				});
			_tableData->setCellWidget(row, 2, editBtn);
		}

		_syncingCheck = false;
		syncActionButtons();
		//行数变化后刷新折叠块高度，多余高度由主布局末尾的stretch承担
		_foldWidget->refreshLayout();
	}

	template<typename Traits>
	QTableWidgetItem* mManagerPanelTpl<Traits>::nameItemOfRow(int row)
	{
		return _tableData->item(row, 0);
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::syncActionButtons()
	{
		int rowCount = _tableData->rowCount();
		bool allChecked = rowCount > 0;
		bool anyChecked = false;
		for (int row = 0; row < rowCount; ++row)
		{
			QTableWidgetItem* nameItem = nameItemOfRow(row);
			bool checked = nameItem != nullptr && nameItem->checkState() == Qt::Checked;
			allChecked = allChecked && checked;
			anyChecked = anyChecked || checked;
		}
		_btnSelectAll->setText(allChecked ? "取消全选" : "全选");
		_btnDelete->setEnabled(anyChecked);
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::slot_tableItemChanged(QTableWidgetItem* item)
	{
		if (item == nullptr || item->column() != 0)
		{
			return;
		}

		//Ctrl多选场景联动：勾选发生变化的行在当前选中对象内时，
		//把相同勾选状态应用到其他选中行
		if (!_syncingCheck && _tableData->selectionModel() != nullptr
			&& _tableData->selectionModel()->isRowSelected(item->row(), QModelIndex()))
		{
			Qt::CheckState state = item->checkState();
			_syncingCheck = true;
			QModelIndexList selectedRows = _tableData->selectionModel()->selectedRows();
			for (const QModelIndex& idx : selectedRows)
			{
				QTableWidgetItem* nameItem = nameItemOfRow(idx.row());
				if (nameItem != nullptr && nameItem != item && nameItem->checkState() != state)
				{
					nameItem->setCheckState(state);
				}
			}
			_syncingCheck = false;
		}

		syncActionButtons();
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::slot_selectAllBtn()
	{
		//存在未勾选行时全选，否则取消全选
		bool anyUnchecked = false;
		for (int row = 0; row < _tableData->rowCount(); ++row)
		{
			QTableWidgetItem* nameItem = nameItemOfRow(row);
			if (nameItem == nullptr || nameItem->checkState() == Qt::Unchecked)
			{
				anyUnchecked = true;
				break;
			}
		}
		Qt::CheckState state = anyUnchecked ? Qt::Checked : Qt::Unchecked;

		//批量设置期间屏蔽联动勾选，由末尾统一刷新按钮状态
		_syncingCheck = true;
		for (int row = 0; row < _tableData->rowCount(); ++row)
		{
			QTableWidgetItem* nameItem = nameItemOfRow(row);
			if (nameItem != nullptr)
			{
				nameItem->setCheckState(state);
			}
		}
		_syncingCheck = false;

		syncActionButtons();
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::slot_deleteBtn()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		QVector<int> checkedIDs;
		QList<int> checkedRows;
		for (int row = 0; row < _tableData->rowCount(); ++row)
		{
			QTableWidgetItem* nameItem = nameItemOfRow(row);
			if (nameItem != nullptr && nameItem->checkState() == Qt::Checked)
			{
				checkedIDs.push_back(nameItem->data(Qt::UserRole).toInt());
				checkedRows.push_back(row);
			}
		}
		if (checkedIDs.isEmpty())
		{
			return;
		}

		//先从表格移除，删除数据与树同步由mTreeView接收对应删除信号完成
		_syncingCheck = true;
		std::sort(checkedRows.begin(), checkedRows.end(), std::greater<int>());
		for (int row : checkedRows)
		{
			_tableData->removeRow(row);
		}
		_syncingCheck = false;

		Traits::emitDelete(checkedIDs);

		//删除后类型下拉可能变化，整体刷新（矩阵弹窗为模态，激活期间不会触发删除，无需同步）
		refreshList();
	}

	template<typename Traits>
	void mManagerPanelTpl<Traits>::slot_matrixEntryBtn()
	{
		//首次点击时创建矩阵弹出对话框（随面板销毁，避免泄漏与重复弹出）
		if (_matrixDialog == nullptr)
		{
			_matrixDialog = new mManagerTablePopupDialog(this);
		}
		//模态弹出期间入口按钮切淡蓝提示激活状态，弹窗关闭后恢复默认白色
		mManagerPanelDetail::applyMatrixEntryStyle(_btnMatrixEntry, true);
		_matrixDialog->execMatrix(Traits::matrixEntryText(), _menuWidget, Traits::matrixTreeType(), _btnMatrixEntry);
		mManagerPanelDetail::applyMatrixEntryStyle(_btnMatrixEntry, false);
	}
}
