#include "mCurveChannelSelector.h"

#include "../mCurveDataSourceStore.h"
#include "../mCurveImportCoordinator.h"
#include <memory>
#include "../CurveCore/mCurveDocController.h"

//QT
#include <QComboBox>
#include <QCursor>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

//STL
#include <set>

namespace PostCurve
{
	namespace
	{
		//列表按 UserRole 数据选行（信号阻断；未命中保持当前行——回填容错，
		//调用方随后的下级重填按实际选中行计算）
		void selectRowByData(QListWidget* list, const QVariant& value)
		{
			for (int i = 0; i < list->count(); ++i)
				if (list->item(i)->data(Qt::UserRole) == value)
				{
					list->blockSignals(true);
					list->setCurrentRow(i);
					list->blockSignals(false);
					return;
				}
		}

		//列表按文字选行（信号阻断；未命中保持当前行）
		void selectRowByText(QListWidget* list, const QString& text)
		{
			if (text.isEmpty())
				return;
			for (int i = 0; i < list->count(); ++i)
				if (list->item(i)->text() == text)
				{
					list->blockSignals(true);
					list->setCurrentRow(i);
					list->blockSignals(false);
					return;
				}
		}
	}

	// =============================================================================
	// 构造 / 值读写
	// =============================================================================

	mCurveChannelSelector::mCurveChannelSelector(QWidget* parent)
		: QWidget(parent)
	{
		buildUi();
		//初始态：有已导入文件时选首个并联动四级列表（全程信号阻断，不发信号）
		refillFileCombo(QString());
		refillSubcases();
		refillSchemas();
		refillObjects();
		refillComponents();
	}

	void mCurveChannelSelector::buildUi()
	{
		QVBoxLayout* lay = new QVBoxLayout(this);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(6);

		//数据文件行：下拉（共享仓库，登记顺序）+ "导入..."（与"定义曲线"文件页同形态）
		QHBoxLayout* fileRow = new QHBoxLayout();
		fileRow->setSpacing(6);
		QLabel* fileLabel = new QLabel(QStringLiteral("数据文件："), this);
		fileLabel->setStyleSheet("QLabel{background:transparent;}");
		_fileCombo = new QComboBox(this);
		QPushButton* importBtn = new QPushButton(this);
		importBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/打开文件.svg")));
		importBtn->setFixedSize(24, 24);   //图标按钮统一规格（同"应用样式"树工具钮）
		importBtn->setToolTip(QStringLiteral(
			"导入数据文件（pch/csv/txt，与\"从文件创建曲线\"共享已导入数据）"));
		connect(_fileCombo, qOverload<int>(&QComboBox::currentIndexChanged),
			this, &mCurveChannelSelector::onFileComboChanged);
		connect(importBtn, &QPushButton::clicked, this, &mCurveChannelSelector::onImportFile);
		fileRow->addWidget(fileLabel);
		fileRow->addWidget(_fileCombo, 1);
		fileRow->addWidget(importBtn);
		lay->addLayout(fileRow);

		//四级联动单选列表：工况 -> 结果类型 -> 对象 -> 分量（逐级过滤）
		auto makeCascadeColumn = [this](const QString& caption, QListWidget** out)
		{
			QWidget* col = new QWidget(this);
			QVBoxLayout* colLay = new QVBoxLayout(col);
			colLay->setContentsMargins(0, 0, 0, 0);
			colLay->setSpacing(4);
			QLabel* label = new QLabel(caption, col);
			label->setStyleSheet("QLabel{background:transparent;}");
			*out = new QListWidget(col);
			(*out)->setAlternatingRowColors(true);
			colLay->addWidget(label);
			colLay->addWidget(*out, 1);
			return col;
		};
		QHBoxLayout* listsRow = new QHBoxLayout();
		listsRow->setSpacing(6);
		listsRow->addWidget(makeCascadeColumn(QStringLiteral("工况"), &_subcaseList), 1);
		//工况列容器留存：ascii 数据集无工况级语义，整列隐藏（refillSubcases 按文件类型切换）
		_subcaseCol = _subcaseList->parentWidget();
		listsRow->addWidget(makeCascadeColumn(QStringLiteral("结果类型"), &_schemaList), 1);
		listsRow->addWidget(makeCascadeColumn(QStringLiteral("对象"), &_objectList), 1);
		listsRow->addWidget(makeCascadeColumn(QStringLiteral("分量"), &_componentList), 1);
		connect(_subcaseList, &QListWidget::currentRowChanged,
			this, &mCurveChannelSelector::onSubcaseRowChanged);
		connect(_schemaList, &QListWidget::currentRowChanged,
			this, &mCurveChannelSelector::onSchemaRowChanged);
		connect(_objectList, &QListWidget::currentRowChanged,
			this, &mCurveChannelSelector::onObjectRowChanged);
		connect(_componentList, &QListWidget::currentRowChanged, this, [this]
		{
			emit sig_selectionChanged();   //分量是末级，无下级可联动
		});
		lay->addLayout(listsRow, 1);
	}

	void mCurveChannelSelector::setValue(const ChannelSelection& sel)
	{
		//程序回填：全链信号阻断 + 显式逐级重算（refillXxx/selectXxx 内部均阻断，
		//selectXxx 未命中保持该级首行，下级按实际选中行重算）。
		//ascii 形态：refillSubcases 留空列隐藏（selectRowByData 空转），结果类型/
		//对象唯一项自动选中，分量按列名定位——空串/失配保持首行=第 1 列
		//（数学/数值等失配来源落默认 [Unknown]/[Block 1]/第 1 列 的兜底口径）
		refillFileCombo(sel.filePath);
		refillSubcases();
		selectRowByData(_subcaseList, sel.subcaseId);
		refillSchemas();
		selectRowByText(_schemaList, sel.schema);
		refillObjects();
		selectRowByData(_objectList, sel.pointId);
		refillComponents();
		selectRowByText(_componentList, sel.component);
		emit sig_selectionChanged();   //宿主据此刷新确定按钮态等
	}

	ChannelSelection mCurveChannelSelector::value() const
	{
		ChannelSelection sel;
		const bool ascii = asciiFileMode();
		sel.filePath = _fileCombo->currentData().toString();
		if (ascii)
			sel.subcaseId = mCurveDataSourceStore::asciiSubcaseId();   //工况级隐藏，id 落常量
		else if (auto* item = _subcaseList->currentItem())
			sel.subcaseId = item->data(Qt::UserRole).toInt();
		if (auto* item = _schemaList->currentItem())
			sel.schema = item->data(Qt::UserRole).toString();
		if (auto* item = _objectList->currentItem())
			sel.pointId = item->data(Qt::UserRole).toInt();
		if (auto* item = _componentList->currentItem())
			sel.component = item->data(Qt::UserRole).toString();
		//ascii 三级形态不校验隐藏的工况级（其余各级同口径）
		sel.complete = _fileCombo->currentIndex() >= 0
			&& (ascii || _subcaseList->currentRow() >= 0)
			&& _schemaList->currentRow() >= 0
			&& _objectList->currentRow() >= 0
			&& _componentList->currentRow() >= 0;
		return sel;
	}

	QString mCurveChannelSelector::subcaseText(int subcaseId)
	{
		return QStringLiteral("Subcase %1").arg(subcaseId);
	}

	QString mCurveChannelSelector::objectText(int pointId)
	{
		return QStringLiteral("Point id %1").arg(pointId);
	}

	ChannelSelection mCurveChannelSelector::fromModelTexts(const QString& filePath,
		const QString& subcaseText, const QString& schema, const QString& objectText,
		const QString& component)
	{
		//"Subcase %1"/"Point id %1" 显示串取串尾数字反解 id
		auto trailingInt = [](const QString& text) -> int
		{
			int end = text.size();
			while (end > 0 && text.at(end - 1).isSpace())
				--end;
			int start = end;
			while (start > 0 && text.at(start - 1).isDigit())
				--start;
			return start < end ? text.mid(start, end - start).toInt() : 0;
		};

		ChannelSelection sel;
		sel.filePath = mCurveDataSourceStore::normalizePath(filePath);
		sel.subcaseId = trailingInt(subcaseText);
		sel.schema = schema;
		sel.pointId = trailingInt(objectText);
		sel.component = component;
		sel.complete = false;   //能否定位由 setValue 实际回显决定
		return sel;
	}

	// =============================================================================
	// 四级联动重填（数据现取现用共享仓库，不缓存其内部指针）
	// =============================================================================

	bool mCurveChannelSelector::asciiFileMode() const
	{
		//R22 分流收口点：当前文件为 csv/txt 列数据（AsciiColumn）时四级列表
		//退化为三级（工况列隐藏/结果类型=Unknown/对象=Block 1/分量=列名）。
		//内嵌与弹出（pickChannel 内嵌同一控件）两宿主形态共用本判定
		return mCurveDataSourceStore::fileKind(_fileCombo->currentData().toString())
			== mCurveDataSourceStore::DataSourceKind::AsciiColumn;
	}

	void mCurveChannelSelector::refillFileCombo(const QString& selectPath)
	{
		_fileCombo->blockSignals(true);
		_fileCombo->clear();
		for (const QString& path : mCurveDataSourceStore::getInstance()->filePaths())
		{
			_fileCombo->addItem(QFileInfo(path).fileName(), path);
			_fileCombo->setItemData(_fileCombo->count() - 1, path, Qt::ToolTipRole);
		}
		//优先选指定项（键为规范化路径，查询侧同步归一）；未指定/失配且有
		//文件时选首个（下级重填由调用方显式联动）
		int row = selectPath.isEmpty() ? -1
			: _fileCombo->findData(mCurveDataSourceStore::normalizePath(selectPath));
		if (row < 0 && _fileCombo->count() > 0)
			row = 0;
		_fileCombo->setCurrentIndex(row);
		_fileCombo->blockSignals(false);
	}

	void mCurveChannelSelector::refillSubcases()
	{
		_subcaseList->blockSignals(true);
		_subcaseList->clear();
		const QString path = _fileCombo->currentData().toString();
		if (asciiFileMode())
		{
			//ascii 数据集无工况级语义（T1 索引固定骨架仅占位工况 1）：列表留空 +
			//整列隐藏，五元组的工况 id 由 value() 落 asciiSubcaseId() 常量
			if (_subcaseCol)
				_subcaseCol->setVisible(false);
			_subcaseList->blockSignals(false);
			return;
		}
		if (_subcaseCol)
			_subcaseCol->setVisible(true);
		if (const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(path))
		{
			//工况文本由共享仓库生成（与"定义曲线"文件页工况列同源，内容一致：
			//"Subcase {id} (label)"，label 取该工况首个非空 $LABEL 行）
			const PchReader::PchFile* f = mCurveDataSourceStore::getInstance()->file(path);
			for (int subcaseId : idx->operatingcondition)   //std::set 升序
			{
				auto* item = new QListWidgetItem(f
					? mCurveDataSourceStore::subcaseDisplayText(*f, subcaseId)
					: subcaseText(subcaseId), _subcaseList);
				item->setData(Qt::UserRole, subcaseId);
			}
		}
		if (_subcaseList->count() > 0)
			_subcaseList->setCurrentRow(0);
		_subcaseList->blockSignals(false);
	}

	void mCurveChannelSelector::refillSchemas()
	{
		_schemaList->blockSignals(true);
		_schemaList->clear();
		const QString path = _fileCombo->currentData().toString();
		if (asciiFileMode())
		{
			//ascii 数据集结果类型唯一项 "Unknown"（T1 固定骨架；不走
			//isVectorSchema 过滤——该过滤对 "Unknown" 恒假，仅适用于 pch 矢量结果）
			auto* item = new QListWidgetItem(mCurveDataSourceStore::asciiSchemaDisplayText(),
				_schemaList);
			item->setData(Qt::UserRole, mCurveDataSourceStore::asciiSchemaDisplayText());
			_schemaList->setCurrentRow(0);
			_schemaList->blockSignals(false);
			return;
		}
		if (const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(path))
		{
			//仅节点矢量结果有分量展开（与"从文件创建曲线"列表联动一致），排序求稳
			QStringList schemas = idx->object.value(currentSubcaseId()).keys();
			schemas.sort();
			for (const QString& schema : schemas)
			{
				if (!mCurveDataSourceStore::isVectorSchema(schema))
					continue;
				auto* item = new QListWidgetItem(schema, _schemaList);
				item->setData(Qt::UserRole, schema);
			}
		}
		if (_schemaList->count() > 0)
			_schemaList->setCurrentRow(0);
		_schemaList->blockSignals(false);
	}

	void mCurveChannelSelector::refillObjects()
	{
		_objectList->blockSignals(true);
		_objectList->clear();
		if (asciiFileMode())
		{
			//ascii 数据集对象唯一项 "Block 1"（显示文本 vs pch 的 "Point id {id}"；
			//UserRole 仍落内部 id 常量，value() 输出 id 形态不变）
			auto* item = new QListWidgetItem(mCurveDataSourceStore::asciiObjectDisplayText(),
				_objectList);
			item->setData(Qt::UserRole, mCurveDataSourceStore::asciiPointId());
			_objectList->setCurrentRow(0);
			_objectList->blockSignals(false);
			return;
		}
		if (const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(
			_fileCombo->currentData().toString()))
			for (int pointId : idx->object.value(currentSubcaseId()).value(currentSchema()))
			{
				auto* item = new QListWidgetItem(objectText(pointId), _objectList);
				item->setData(Qt::UserRole, pointId);
			}
		if (_objectList->count() > 0)
			_objectList->setCurrentRow(0);
		_objectList->blockSignals(false);
	}

	void mCurveChannelSelector::refillComponents()
	{
		_componentList->blockSignals(true);
		_componentList->clear();
		if (asciiFileMode())
		{
			//ascii 数据集分量=列名清单（显示序；同名列不去重，装配按首个匹配
			//下标取列，经 T1 asciiColumnNames 下发，不走 expandedComponents）
			const QStringList cols = mCurveDataSourceStore::getInstance()
				->asciiColumnNames(_fileCombo->currentData().toString());
			for (const QString& col : cols)
			{
				auto* item = new QListWidgetItem(col, _componentList);
				item->setData(Qt::UserRole, col);
			}
			if (_componentList->count() > 0)
				_componentList->setCurrentRow(0);   //默认第 1 列（失配回显兜底亦落此）
			_componentList->blockSignals(false);
			return;
		}
		if (const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(
			_fileCombo->currentData().toString()))
		{
			//分量数值格式（同一对象唯一；异常多格式按"定义曲线"联动语义跳过该
			//对象）；瞬态标志随索引叶子下发（块级判定），分量展开 6/12 按此分流
			const std::set<NumericFmtInfo> fmts = idx->ycomponent.value(currentSubcaseId())
				.value(currentSchema()).value(currentPointId());
			if (fmts.size() == 1)
				for (const QString& comp : mCurveDataSourceStore::expandedComponents(
					fmts.begin()->fmt, fmts.begin()->transient))
				{
					auto* item = new QListWidgetItem(comp, _componentList);
					item->setData(Qt::UserRole, comp);
				}
		}
		if (_componentList->count() > 0)
			_componentList->setCurrentRow(0);
		_componentList->blockSignals(false);
	}

	int mCurveChannelSelector::currentSubcaseId() const
	{
		return _subcaseList->currentItem()
			? _subcaseList->currentItem()->data(Qt::UserRole).toInt() : 0;
	}

	QString mCurveChannelSelector::currentSchema() const
	{
		return _schemaList->currentItem()
			? _schemaList->currentItem()->data(Qt::UserRole).toString() : QString();
	}

	int mCurveChannelSelector::currentPointId() const
	{
		return _objectList->currentItem()
			? _objectList->currentItem()->data(Qt::UserRole).toInt() : 0;
	}

	// =============================================================================
	// 用户交互槽（每级变化 -> 重填全部下级 + 发一次变化信号）
	// =============================================================================

	void mCurveChannelSelector::onFileComboChanged()
	{
		refillSubcases();
		refillSchemas();
		refillObjects();
		refillComponents();
		emit sig_selectionChanged();
	}

	void mCurveChannelSelector::onSubcaseRowChanged()
	{
		refillSchemas();
		refillObjects();
		refillComponents();
		emit sig_selectionChanged();
	}

	void mCurveChannelSelector::onSchemaRowChanged()
	{
		refillObjects();
		refillComponents();
		emit sig_selectionChanged();
	}

	void mCurveChannelSelector::onObjectRowChanged()
	{
		refillComponents();
		emit sig_selectionChanged();
	}

	void mCurveChannelSelector::onImportFile()
	{
		//目录沿用已导入数据集所在目录，否则 C://（对齐"从文件创建曲线"/"定义曲线"）
		const QStringList paths = mCurveDataSourceStore::getInstance()->filePaths();
		const QString dir = paths.isEmpty()
			? QStringLiteral("C://") : QFileInfo(paths.first()).absolutePath();
		const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("打开文件"),
			dir, QStringLiteral("数据文件(*.pch *.csv *.txt);;PCH File(*.pch);;"
				"CSV File(*.csv);;TXT File(*.txt)"));
		if (filePath.isEmpty())
			return;

		mCurveDocController* controller = mCurveDocController::getInstance();

		//导入异步化（FR-01）：解析移后台线程，成功/失败续接移入
		//sig_importFinished 分支；已登记走原幂等成功链路（C1）。
		//协调器须在 addFileAsync 之前就位（否则首个 sig_importStarted 无订阅方）
		mCurveImportCoordinator* coordinator = mCurveImportCoordinator::getInstance(window());
		auto* store = mCurveDataSourceStore::getInstance();
		const auto accepted = store->addFileAsync(filePath);
		if (accepted == mCurveDataSourceStore::AddFileAsyncResult::AlreadyRegistered)
		{
			//原成功续接链路（逐字保留）
			controller->reportInfo(QStringLiteral("[通道选择] 已导入数据集：%1")
				.arg(QFileInfo(filePath).fileName()));
			refillAfterImport(filePath);
			return;
		}

		//Accepted/DuplicateInFlight：解析在后台执行，成败经信号续接。接收上下文
		//=this（C3：解析中可关本对话框——D5，Qt 随对象销毁自动断连防悬垂）；
		//按键匹配防多导入串扰；命中后自断连
		if (accepted == mCurveDataSourceStore::AddFileAsyncResult::DuplicateInFlight)
			coordinator->notifyDuplicateInFlight(filePath);
		const QString key = mCurveDataSourceStore::normalizePath(filePath);
		const auto conn = std::make_shared<QMetaObject::Connection>();
		*conn = connect(store, &mCurveDataSourceStore::sig_importFinished, this,
			[this, key, filePath, conn](const QString& finishedKey, bool ok, const QString& err)
			{
				if (finishedKey != key)
					return;
				QObject::disconnect(*conn);
				mCurveDocController* controller = mCurveDocController::getInstance();
				if (!ok)
				{
					controller->reportError(QStringLiteral("[通道选择] 数据文件导入失败：%1").arg(err));
					return;
				}
				//原成功续接链路（逐字保留）
				controller->reportInfo(QStringLiteral("[通道选择] 已导入数据集：%1")
					.arg(QFileInfo(filePath).fileName()));
				refillAfterImport(filePath);
			});
	}

	void mCurveChannelSelector::refillAfterImport(const QString& filePath)
	{
		//重建下拉选中新文件（addFile 键已规范化必命中）并联动四级列表；
		//仓库 sig_fileAdded 的其他订阅方（定义曲线等）各自刷新互不影响
		refillFileCombo(filePath);
		refillSubcases();
		refillSchemas();
		refillObjects();
		refillComponents();
		emit sig_selectionChanged();
	}

	// =============================================================================
	// 弹出式选择（Qt::Popup 白卡片 + 确定/取消）
	// =============================================================================

	bool mCurveChannelSelector::pickChannel(QWidget* anchor, const ChannelSelection& current,
		ChannelSelection* out)
	{
		if (!out)
			return false;

		//Qt::Popup 点外即关（视为取消）；堆分配 + WA_DeleteOnClose 现建现销，
		//局部事件循环同步取结果（close 后 destroyed 退出循环）
		auto* popup = new QWidget(anchor ? anchor->window() : nullptr,
			Qt::Popup | Qt::FramelessWindowHint);
		popup->setAttribute(Qt::WA_DeleteOnClose);
		popup->setFixedSize(680, 430);
		popup->setStyleSheet(QStringLiteral(
			"QWidget{background:#FFFFFF;}"
			"QListWidget{border:1px solid #E5E7EB;background:#FFFFFF;}"));

		QVBoxLayout* lay = new QVBoxLayout(popup);
		lay->setContentsMargins(8, 8, 8, 8);
		lay->setSpacing(6);

		QLabel* title = new QLabel(QStringLiteral("通道选择器"), popup);
		title->setStyleSheet(
			"QLabel{color:#333333;font-size:12px;font-weight:600;background:transparent;}");
		lay->addWidget(title);

		auto* selector = new mCurveChannelSelector(popup);
		selector->setValue(current);
		lay->addWidget(selector, 1);

		QWidget* btnRow = new QWidget(popup);
		QHBoxLayout* btnLay = new QHBoxLayout(btnRow);
		btnLay->setContentsMargins(0, 0, 0, 0);
		QPushButton* okBtn = new QPushButton(QStringLiteral("确定"), btnRow);
		QPushButton* cancelBtn = new QPushButton(QStringLiteral("取消"), btnRow);
		//确定=模块统一主蓝 #1385F6（同各弹窗右下角主钮），禁用态浅蓝；调至最右
		okBtn->setStyleSheet(
			"QPushButton{background:#1385F6; border:none; border-radius:4px;"
			"color:#FFFFFF; padding:0px;}"
			"QPushButton:hover{ background:#3D9EF8; }"
			"QPushButton:pressed{ background:#0F6ACB; }"
			"QPushButton:disabled{ background:#9CC3E5; color:#FFFFFF; }");
		okBtn->setFixedSize(44, 26);
		cancelBtn->setFixedSize(44, 26);
		btnLay->addStretch(1);
		btnLay->addWidget(cancelBtn);
		btnLay->addWidget(okBtn);
		lay->addWidget(btnRow);

		//选择完整才可确定（避免无分量即确认的空通道）
		okBtn->setEnabled(selector->value().isValid());
		connect(selector, &mCurveChannelSelector::sig_selectionChanged, popup,
			[selector, okBtn] { okBtn->setEnabled(selector->value().isValid()); });
		bool accepted = false;
		connect(okBtn, &QPushButton::clicked, popup, [selector, out, &accepted, popup]
		{
			*out = selector->value();
			accepted = true;
			popup->close();
		});
		connect(cancelBtn, &QPushButton::clicked, popup, &QWidget::close);

		//落位：anchor 正下方，越出屏幕底部时改到上方；横向往屏幕内收
		if (anchor)
		{
			QPoint pos = anchor->mapToGlobal(QPoint(0, anchor->height() + 4));
			const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
			if (pos.y() + popup->height() > screen.bottom())
				pos = anchor->mapToGlobal(QPoint(0, -popup->height() - 4));
			pos.setX(qMax(screen.left(), qMin(pos.x(), screen.right() - popup->width())));
			popup->move(pos);
		}
		else
		{
			popup->move(QCursor::pos());
		}

		QEventLoop loop;
		connect(popup, &QObject::destroyed, &loop, &QEventLoop::quit);
		popup->show();
		loop.exec();
		return accepted;
	}
}
