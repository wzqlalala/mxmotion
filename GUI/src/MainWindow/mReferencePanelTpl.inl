//mReferencePanelTpl 模板实现：由 mReferencePanelTpl.h 末尾包含，不单独编译
#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//视口数据
#include "mViewportModelManager.h"
#include "mViewportModel.h"

#include "mCheckRowDelegate.h"

//QT
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSet>

namespace MechMainWindow
{
	template<typename Traits, bool HasScale>
	mReferencePanelTpl<Traits, HasScale>::mReferencePanelTpl(QWidget* parent)
		: mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->addStretch();

		//对齐HTML原型的列表样式：行背景/复选框对勾由mCheckRowDelegate绘制，
		//样式表仅提供表格边框与表头底色（不再用selection/item背景，避免与行底色规则冲突）
		const QString listStyle = QStringLiteral(R"(
			QTableWidget { border: 1px solid #dce1e8; border-radius: 2px; background: #ffffff; }
			QHeaderView::section { background: #f5f6f8; color: #666d76; border: none;
				border-bottom: 1px solid #dce1e8; border-right: 1px solid #edf0f3; padding: 6px 4px; font-weight: normal; min-height: 28px; }
		)");

		//折叠框：xxx引用
		_foldWidget = new FoldSection(Traits::referenceTitle(), this);
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

		//表格：名称(复选框) / 类型 / [缩放系数] / 操作(编辑)
		_tableData = new QTableWidget(_foldWidget);
		QStringList headers;
		headers << Traits::nameHeader() << "类型";
		if (HasScale)
		{
			headers << "缩放系数";
		}
		headers << "操作";
		_tableData->setColumnCount(headers.size());
		_tableData->setHorizontalHeaderLabels(headers);
		//无数据时ResizeToContents列收缩为0会挤住名称列表头，固定表头最小宽度兜底
		_tableData->horizontalHeader()->setMinimumWidth(360);
		//分段最小宽度兜底：空表时类型/缩放系数/操作列提示尺寸塌陷导致表头文字被裁切（需容纳"缩放系数"表头）
		_tableData->horizontalHeader()->setMinimumSectionSize(72);
		_tableData->verticalHeader()->setHidden(true);
		//名称列拉伸，其余列按内容收缩，避免宽面板下表头被等分撑散
		_tableData->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		for (int col = 1; col < headers.size(); ++col)
		{
			_tableData->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
		}
		_tableData->setSelectionBehavior(QAbstractItemView::SelectRows);
		_tableData->setSelectionMode(QAbstractItemView::SingleSelection);
		_tableData->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_tableData->setShowGrid(false);
		_tableData->setStyleSheet(listStyle);
		_tableData->setMouseTracking(true);
		//勾选行淡蓝/选中行淡灰/复选框白对勾均由委托绘制
		_tableData->setItemDelegate(new mCheckRowDelegate(_tableData));
		//列表按内容自适应高度（约12行+表头），多余空间由主布局末尾stretch承担
		_tableData->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
		_tableData->setMinimumHeight(_tableData->horizontalHeader()->sizeHint().height()
			+ _tableData->verticalHeader()->defaultSectionSize() * 6 + 4);
		_foldWidget->addRow(_tableData);

		//底部按钮：全部引用 + 全部取消（仅作用于当前筛选结果，右对齐对齐HTML原型）
		auto* btnWidget = new QWidget;
		auto* btnLayout = new QHBoxLayout(btnWidget);
		btnLayout->setContentsMargins(0, 0, 0, 0);
		_btnRefAll = new QPushButton("全部引用", btnWidget);
		_btnCancelAll = new QPushButton("全部取消", btnWidget);
		_btnCancelAll->setStyleSheet("QPushButton { color: #bf5151; } QPushButton:disabled { color: #a3a9b1; }");
		btnLayout->addStretch();
		btnLayout->addWidget(_btnRefAll);
		btnLayout->addWidget(_btnCancelAll);
		_foldWidget->addRow(btnWidget);

		//将折叠框插入到末尾stretch之前，多余空白留在折叠块之后
		QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
		if (lastItem != nullptr && lastItem->spacerItem() != nullptr)
		{
			_mainLayout->removeItem(lastItem);
			_mainLayout->addWidget(_foldWidget);
			_mainLayout->addStretch();
		}
		else
		{
			_mainLayout->addWidget(_foldWidget);
		}

		//信号连接
		connect(_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { slot_typeFilterChanged(); });
		connect(_editNameFilter, &QLineEdit::textChanged, this, [this](const QString& text) { slot_nameFilterChanged(text); });
		connect(_tableData, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) { slot_tableItemChanged(item); });
		connect(_btnRefAll, &QPushButton::clicked, this, [this]() { slot_referenceAllBtn(); });
		connect(_btnCancelAll, &QPushButton::clicked, this, [this]() { slot_cancelAllBtn(); });

		//对象增删时，若面板可见则刷新列表（新建对象经筛选后需立即可勾选引用）
		Traits::connectRefreshSignals(this, [this]() {
			if (isVisible())
			{
				refreshList();
			}
			});
	}

	template<typename Traits, bool HasScale>
	mReferencePanelTpl<Traits, HasScale>::~mReferencePanelTpl()
	{
		if (_mainLayout != nullptr)
		{
			delete _mainLayout;
			_mainLayout = nullptr;
		}
	}

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::showReference(int caseID)
	{
		_caseId = caseID;
		refreshList();
	}

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::slot_typeFilterChanged()
	{
		refreshList();
	}

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::slot_nameFilterChanged(const QString& text)
	{
		Q_UNUSED(text);
		refreshList();
	}

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::refreshTypeCombo()
	{
		//保留原选中类型，重建后若仍存在则恢复选中
		QString currentType = _comboType->currentText();
		_comboType->blockSignals(true);
		_comboType->clear();
		_comboType->addItem("全部类型");

		//动态列举当前工况可引用且实际存在的类型（按id顺序去重）
		QSet<QString> addedTypes;
		for (int id : Traits::allIds())
		{
			if (!Traits::isSupportedByCase(id, _caseId))
			{
				continue;
			}
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

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::refreshList()
	{
		_syncing = true;

		_tableData->setRowCount(0);

		if (getCVM == nullptr || _caseId == 0)
		{
			_syncing = false;
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
			//仅列出当前工况可引用的对象
			if (!Traits::isSupportedByCase(id, _caseId))
			{
				continue;
			}
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

			bool referenced = Traits::isReferenced(id, _caseId);
			//强制挂载的对象勾选且禁用（接触除过盈接触外）；被抑制对象禁用勾选
			bool checkEnabled = !Traits::isForceReferenced(id) && !Traits::isSuppressedObject(id);

			QTableWidgetItem* nameItem = new QTableWidgetItem(name);
			nameItem->setFlags((nameItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
			if (!checkEnabled)
			{
				nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEnabled);
			}
			nameItem->setCheckState((referenced || Traits::isForceReferenced(id)) ? Qt::Checked : Qt::Unchecked);
			nameItem->setData(Qt::UserRole, id);
			_tableData->setItem(row, 0, nameItem);

			QTableWidgetItem* typeItem = new QTableWidgetItem(Traits::typeNameOf(id));
			typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
			_tableData->setItem(row, 1, typeItem);

			int editColumn = 2;
			if (HasScale)
			{
				//缩放系数：已引用且该类型的编辑面板提供缩放因子时才可编辑，否则置灰
				auto* scaleSpin = new QDoubleSpinBox;
				scaleSpin->setDecimals(6);
				scaleSpin->setRange(-1e12, 1e12);
				scaleSpin->setValue(Traits::scaleOf(id, _caseId));
				scaleSpin->setEnabled(referenced && Traits::hasScaleFactor(id));
				connect(scaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, id](double value) {
					if (_syncing || getCVM == nullptr || _caseId == 0)
					{
						return;
					}
					Traits::setScale(id, _caseId, value);
					});
				_tableData->setCellWidget(row, 2, scaleSpin);
				editColumn = 3;
			}

			//编辑按钮：跳转到编辑面板，未引用时禁用
			auto* editBtn = new QPushButton("编辑");
			editBtn->setEnabled(referenced);
			connect(editBtn, &QPushButton::clicked, this, [this, id]() {
				//以当前工况为语境打开对应类型的编辑面板
				Traits::editObject(id, _caseId);
				//选中工况下父节点下的对应节点
				Traits::selectCaseItem(_caseId, id);
				});
			_tableData->setCellWidget(row, editColumn, editBtn);
		}

		_syncing = false;
		//行数变化后刷新折叠块高度，多余高度由主布局末尾的stretch承担
		_foldWidget->refreshLayout();
	}

	template<typename Traits, bool HasScale>
	QTableWidgetItem* mReferencePanelTpl<Traits, HasScale>::nameItemOfRow(int row)
	{
		return _tableData->item(row, 0);
	}

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::setRowReferenced(int row, bool referenced, bool dataChange)
	{
		QTableWidgetItem* nameItem = nameItemOfRow(row);
		if (nameItem == nullptr)
		{
			return;
		}
		_syncing = true;
		nameItem->setCheckState(referenced ? Qt::Checked : Qt::Unchecked);
		if (HasScale)
		{
			if (auto* scaleSpin = qobject_cast<QDoubleSpinBox*>(_tableData->cellWidget(row, 2)))
			{
				int id = nameItem->data(Qt::UserRole).toInt();
				scaleSpin->setEnabled(referenced && Traits::hasScaleFactor(id));
			}
		}
		int editColumn = HasScale ? 3 : 2;
		if (auto* editBtn = qobject_cast<QPushButton*>(_tableData->cellWidget(row, editColumn)))
		{
			editBtn->setEnabled(referenced);
		}
		_syncing = false;

		if (dataChange)
		{
			int id = nameItem->data(Qt::UserRole).toInt();
			if (referenced)
			{
				Traits::reference(id, _caseId);
			}
			else
			{
				Traits::unreference(id, _caseId);
			}
		}
	}

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::slot_tableItemChanged(QTableWidgetItem* item)
	{
		if (_syncing || item == nullptr || item->column() != 0)
		{
			return;
		}
		setRowReferenced(item->row(), item->checkState() == Qt::Checked, true);
	}

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::slot_referenceAllBtn()
	{
		//仅作用于当前筛选结果（表格当前列出的行）；禁用行（强制挂载/被抑制）不可操作
		for (int row = 0; row < _tableData->rowCount(); ++row)
		{
			QTableWidgetItem* nameItem = nameItemOfRow(row);
			if (nameItem != nullptr && (nameItem->flags() & Qt::ItemIsEnabled)
				&& nameItem->checkState() != Qt::Checked)
			{
				setRowReferenced(row, true, true);
			}
		}
	}

	template<typename Traits, bool HasScale>
	void mReferencePanelTpl<Traits, HasScale>::slot_cancelAllBtn()
	{
		//仅作用于当前筛选结果（表格当前列出的行）；禁用行（强制挂载/被抑制）不可操作
		for (int row = 0; row < _tableData->rowCount(); ++row)
		{
			QTableWidgetItem* nameItem = nameItemOfRow(row);
			if (nameItem != nullptr && (nameItem->flags() & Qt::ItemIsEnabled)
				&& nameItem->checkState() != Qt::Unchecked)
			{
				setRowReferenced(row, false, true);
			}
		}
	}
}
