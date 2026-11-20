#pragma execution_character_set("utf-8")

// =============================================================================
// mChannelMapDlg —— 通道映射对话框（实现），交互语义见 mChannelMapDlg.h
// =============================================================================

#include "mChannelMapDlg.h"
#include "mCurveDataSourceStore.h"
#include "CurveWidgets/mCurveChannelSelector.h"
#include "ThemeManager.h"	//自样式弹窗豁免标记（kSelfStyledProp）

//QT
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringListModel>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace PostCurve
{
	namespace
	{
		//列号（文件名称|工况类型|结果类型|对象|分量，设计图12 五列表）
		enum Col { CFile = 0, CSubcase, CSchema, CObject, CComponent, CCount };

		//状态标识图（需求供图 @3x 资源，一次性缓存 16×16 逻辑尺寸）：
		//未匹配=红叉；已匹配未确认=浅灰勾；确认=绿勾
		QPixmap stateMarkPixmap(bool complete, bool confirmed)
		{
			static const QPixmap redMark = [] {
				QPixmap pm(QStringLiteral(":/mMainWidgets/Icon/PostCurve/hongcha3x.png"));
				return pm.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}();
			static const QPixmap greenDim = [] {
				QPixmap pm(QStringLiteral(":/mMainWidgets/Icon/PostCurve/qianhuigou3x.png"));
				return pm.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}();
			static const QPixmap greenMark = [] {
				QPixmap pm(QStringLiteral(":/mMainWidgets/Icon/PostCurve/lvgou3x.png"));
				return pm.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}();
			if (complete)
				return confirmed ? greenMark : greenDim;
			return redMark;
		}

		//预配档归一："自动填充"UI 已随需求图移除、恒"无"=文件名档（复选框默认勾选）
		int prefillMode(int autoFillMode)
		{
			return autoFillMode == mChannelMapCore::AutoFillSuffix
				? mChannelMapCore::AutoFillSuffix : mChannelMapCore::AutoFillFileName;
		}

		//映射差异判定（批次5 差量刷新：任一字段变化即重填该行）
		bool selDiffers(const ChannelSelection& a, const ChannelSelection& b)
		{
			return a.complete != b.complete || a.subcaseId != b.subcaseId
				|| a.schema != b.schema || a.pointId != b.pointId
				|| a.component != b.component;
		}
	}

	bool mChannelMapDlg::resolveMapping(QWidget* parent, const QString& templatePath,
		const QStringList& effectiveParams, int autoFillMode,
		QList<mChannelMapCore::Row>* rows, QString* err)
	{
		QList<mChannelMapCore::Row> built = mChannelMapCore::rowsFromTemplate(
			templatePath, effectiveParams, err);
		if (built.isEmpty())
		{
			QMessageBox::warning(parent, QStringLiteral("通道映射"),
				err && !err->isEmpty() ? *err
				: QStringLiteral("模板内没有文件来源曲线，无通道可映射"));
			return false;
		}
		//数据文件预登记（首次打开即匹配/可手选的前提，语义见核心函数注释）：
		//同步解析大文件可能秒级，等待光标兜底（恢复后再走下方告警分支）
		QApplication::setOverrideCursor(Qt::WaitCursor);
		mChannelMapCore::ensureFilesRegistered(built);
		QApplication::restoreOverrideCursor();
		mChannelMapCore::initExactMatches(&built);
		//自动连接相似通道默认勾选（需求图12）：预配档随"自动填充"，无=文件名档
		mChannelMapCore::autoConnectSimilar(&built, prefillMode(autoFillMode));

		mChannelMapDlg dlg(built, autoFillMode, parent);
		dlg.exec();
		if (rows)
			*rows = dlg._rows;
		return true;
	}

	mChannelMapDlg::mChannelMapDlg(QList<mChannelMapCore::Row> rows, int autoFillMode,
		QWidget* parent)
		: QDialog(parent, Qt::Window | Qt::FramelessWindowHint)
		, _rows(rows)
		, _initRows(rows)
		, _autoFillMode(autoFillMode)
	{
		//自样式弹窗：豁免 ThemeManager 整窗重发，保留自带设计稿 QSS
		setProperty(MechMainWindow::ThemeManager::kSelfStyledProp, true);
		setWindowTitle(QStringLiteral("通道映射"));
		//需求图形态：无边框白卡片（对齐模板曲线/图层）；尺寸对齐原型卡面
		//（原型卡面实测 880×720；910×750 为含黑底展示边整图）
		resize(900, 740);
		buildUi();
		rebuildTable();
	}

	void mChannelMapDlg::buildUi()
	{
		//白卡片 + M 徽标标题栏（对齐 mTemplateCurveDlg/mLayerManageDlg）：objectName
		//选择器只作用于对话框本体（1px 边框），表格基色一并在对话框级声明
		setObjectName("channelMapDlg");
		setAttribute(Qt::WA_StyledBackground, true);
		setStyleSheet("QWidget#channelMapDlg{background:#FFFFFF;border:1px solid #D9DCE0;}"
			"QTableWidget{background:#FFFFFF;}");

		QVBoxLayout* outer = new QVBoxLayout(this);
		outer->setContentsMargins(1, 1, 1, 1);
		outer->setSpacing(0);

		//标题栏：logo + 标题 + ✗（close3x 资源，禁自绘），样式同 mLayerManageDlg；
		//✗ 语义=接受当前映射继续生成（与底部"关闭"一致）。objectName 挂
		//"mWindowTitleBar" 约定名：ThemeManager 整树补发对自绘标题栏及其直接
		//子控件跳过（applyDialogTheme），✗ 的无边框 QSS 不被主题的 QToolButton
		//边框规则替换（否则 ✗ 呈带边框长方形，与原型方形悬停区不符）
		_titleBar = new QWidget(this);
		_titleBar->setObjectName("mWindowTitleBar");
		_titleBar->setFixedHeight(52);
		_titleBar->setStyleSheet("background:#FFFFFF;");
		QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
		titleLayout->setContentsMargins(20, 20, 20, 10);
		titleLayout->setSpacing(3);
		QLabel* logoLabel = new QLabel(_titleBar);
		logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
			.scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		QLabel* titleLabel = new QLabel(QStringLiteral("通道映射"), _titleBar);
		titleLabel->setStyleSheet(
			"QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");
		QToolButton* titleCloseBtn = new QToolButton(_titleBar);
		titleCloseBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
		titleCloseBtn->setIconSize(QSize(24, 24));
		titleCloseBtn->setToolTip(QStringLiteral("关闭"));
		titleCloseBtn->setStyleSheet(R"(
			QToolButton{ background:transparent; border:none; }
			QToolButton:hover{ background:#EFF1F2; border-radius:2px; }
		)");
		connect(titleCloseBtn, &QToolButton::clicked, this, &QDialog::accept);
		titleLayout->addWidget(logoLabel);
		titleLayout->addWidget(titleLabel, 1);
		titleLayout->addWidget(titleCloseBtn);

		QWidget* body = new QWidget(this);
		body->setStyleSheet("QWidget{background:#FFFFFF;}");
		QVBoxLayout* layout = new QVBoxLayout(body);
		layout->setContentsMargins(20, 20, 20, 14);   //边距对齐原型实测：左右/上 20、下 14
		layout->setSpacing(16);   //工具行-表格 16（原型实测）

		//工具行（设计图12）：显示通道 | 选择最佳匹配 | 确认 | 重置 | CSV 保存/导入
		//（用户供图 导出到/导入到项目 @3x）| 弹性 | 自动连接相似通道（右端）
		QHBoxLayout* tools = new QHBoxLayout();
		tools->setSpacing(12);   //元素间隙原型实测 ≈13
		tools->addWidget(new QLabel(QStringLiteral("显示通道"), this));
		_filterCombo = new QComboBox(this);
		_filterCombo->addItems({ QStringLiteral("全部"),
			QStringLiteral("已匹配"), QStringLiteral("未匹配") });
		//工具行控件统一 24 高（原型实测 ≈22–24），宽 116 对齐原型下拉
		_filterCombo->setFixedSize(116, 24);
		connect(_filterCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mChannelMapDlg::onFilterChanged);
		tools->addWidget(_filterCombo);

		QPushButton* bestBtn = new QPushButton(QStringLiteral("选择最佳匹配"), this);
		//文字钮定宽定高：全局 QSS 的 QPushButton width:80 下限会把短文案钮撑肥，
		//固定尺寸绕过 sizeHint；宽=文字12×6+主题padding20+边2+2 松量
		bestBtn->setFixedSize(96, 24);
		connect(bestBtn, &QPushButton::clicked, this, &mChannelMapDlg::onBestMatchClicked);
		tools->addWidget(bestBtn);
		QPushButton* okBtn = new QPushButton(QStringLiteral("确认"), this);
		okBtn->setFixedSize(48, 24);
		connect(okBtn, &QPushButton::clicked, this, &mChannelMapDlg::onConfirmClicked);
		tools->addWidget(okBtn);
		QPushButton* resetBtn = new QPushButton(QStringLiteral("重置"), this);
		resetBtn->setFixedSize(48, 24);
		connect(resetBtn, &QPushButton::clicked, this, &mChannelMapDlg::onResetClicked);
		tools->addWidget(resetBtn);
		QToolButton* saveBtn = new QToolButton(this);
		saveBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/PostCurve/daochu3x.png")));
		saveBtn->setToolTip(QStringLiteral("保存配置（CSV）"));
		saveBtn->setAutoRaise(true);
		//图标钮 24×24 / 图标 18×18：对齐 mTemplateCurveDlg 工具钮（供图同款 18 设计）
		saveBtn->setFixedSize(24, 24);
		saveBtn->setIconSize(QSize(18, 18));
		connect(saveBtn, &QToolButton::clicked, this, &mChannelMapDlg::onSaveCsvClicked);
		tools->addWidget(saveBtn);
		QToolButton* loadBtn = new QToolButton(this);
		loadBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/PostCurve/daoru3x.png")));
		loadBtn->setToolTip(QStringLiteral("导入配置（CSV）"));
		loadBtn->setAutoRaise(true);
		loadBtn->setFixedSize(24, 24);
		loadBtn->setIconSize(QSize(18, 18));
		connect(loadBtn, &QToolButton::clicked, this, &mChannelMapDlg::onLoadCsvClicked);
		tools->addWidget(loadBtn);
		tools->addStretch(1);

		_autoConnectCheck = new QCheckBox(QStringLiteral("自动连接相似通道"), this);
		_autoConnectCheck->setChecked(true);   //默认勾选（需求图12，非示意态）
		connect(_autoConnectCheck, &QCheckBox::toggled,
			this, &mChannelMapDlg::onAutoConnectToggled);
		tools->addWidget(_autoConnectCheck);
		layout->addLayout(tools);

		//映射表：文件名称|工况类型|结果类型|对象|分量（四列=同款常驻下拉，
		//无边框无下箭头选即改配；值后缀状态标识图）
		_table = new QTableWidget(this);
		_table->setColumnCount(CCount);
		_table->setHorizontalHeaderLabels({ QStringLiteral("文件名称"), QStringLiteral("工况类型"),
			QStringLiteral("结果类型"), QStringLiteral("对象"), QStringLiteral("分量") });
		//列宽对齐需求原型比例（原型表区 ≈814px：文件 250/工况 183/结果 140/
		//对象 115/分量 126 → 等比到本框表区 ≈866px：266/195/149/122/134）：
		//文件列 Stretch 吸收余量，四列定宽起步
		_table->horizontalHeader()->setSectionResizeMode(CFile, QHeaderView::Stretch);
		_table->horizontalHeader()->setSectionResizeMode(CSubcase, QHeaderView::Interactive);
		_table->horizontalHeader()->setSectionResizeMode(CSchema, QHeaderView::Interactive);
		_table->horizontalHeader()->setSectionResizeMode(CObject, QHeaderView::Interactive);
		_table->horizontalHeader()->setSectionResizeMode(CComponent, QHeaderView::Interactive);
		_table->setColumnWidth(CSubcase, 195);
		_table->setColumnWidth(CSchema, 149);
		_table->setColumnWidth(CObject, 122);
		_table->setColumnWidth(CComponent, 134);
		_table->verticalHeader()->setVisible(false);
		_table->setSelectionBehavior(QAbstractItemView::SelectRows);
		_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		layout->addWidget(_table, 1);

		//底部：仅关闭（按当前映射继续生成；确认在工具栏，设计图12）
		QHBoxLayout* buttons = new QHBoxLayout();
		buttons->setContentsMargins(0, 8, 0, 0);   //表格-关闭 24 = 布局间距 16 + 此 8（原型实测）
		QPushButton* closeBtn = new QPushButton(QStringLiteral("关闭"), this);
		closeBtn->setFixedSize(54, 26);   //底部钮对齐 mTemplateCurveDlg 底钮（54×26）
		connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
		buttons->addStretch(1);
		buttons->addWidget(closeBtn);
		layout->addLayout(buttons);

		outer->addWidget(_titleBar);
		outer->addWidget(body, 1);
	}

	void mChannelMapDlg::rebuildTable()
	{
		//全行构建 + 过滤原位隐藏（批次5 统一模型，design D6）：过滤只影响可见性
		//不再物理删行——勾选切换/最佳匹配的差量刷新依赖"行永远在表中"。
		//不变式：全行模型下表行 vr = 行集下标 ri（顺序构建保证）
		_table->blockSignals(true);
		_table->clearContents();
		_table->setRowCount(_rows.size());
		for (int ri = 0; ri < _rows.size(); ++ri)
		{
			const mChannelMapCore::Row& row = _rows.at(ri);

			//文件列：文件名 + 侧行后缀（add-complex-side-channel-parity：复数曲线
			//按侧出行，yp/x 侧行带"· yp"/"· x"标，锚行无后缀）；tooltip=完整路径
			//+行名（限定名即 CSV 对齐键）；Qt::UserRole 存行集下标供改配回查
			const QString sideSuffix = (row.side == mChannelMapCore::Row::SideYp)
				? QStringLiteral(" · yp")
				: (row.side == mChannelMapCore::Row::SideX)
				? QStringLiteral(" · x") : QString();
			auto* fileItem = new QTableWidgetItem(
				QFileInfo(row.mapped.filePath).fileName() + sideSuffix);
			fileItem->setToolTip(QStringLiteral("%1\n%2").arg(
				row.mapped.filePath, row.curveName));
			fileItem->setData(Qt::UserRole, ri);
			fileItem->setFlags(Qt::ItemIsEnabled);
			_table->setItem(ri, CFile, fileItem);

			//四列：先写单元格存档文本到 item（rowSelection/cellValue 与控件解耦），
			//再叠同款选择组控件：常驻下拉（无边框无下箭头）+状态图（需求图12）
			QString curSub = row.mapped.complete
				? mCurveChannelSelector::subcaseText(row.mapped.subcaseId) : row.subcase;
			QString curSchema = row.mapped.complete ? row.mapped.schema : row.schema;
			QString curObject = row.mapped.complete
				? mCurveChannelSelector::objectText(row.mapped.pointId) : row.object;
			QString curComp = row.mapped.complete ? row.mapped.component : row.component;
			struct CellText { int col; QString text; };
			const CellText cells[] = { { CSubcase, curSub }, { CSchema, curSchema },
				{ CObject, curObject }, { CComponent, curComp } };
			for (const CellText& c : cells)
			{
				auto* it = new QTableWidgetItem(c.text);
				it->setFlags(Qt::ItemIsEnabled);
				_table->setItem(ri, c.col, it);
			}
			_table->setCellWidget(ri, CSubcase, makeChoiceCell(ri, CSubcase, row, curSub));
			_table->setCellWidget(ri, CSchema, makeChoiceCell(ri, CSchema, row, curSchema));
			_table->setCellWidget(ri, CObject, makeChoiceCell(ri, CObject, row, curObject));
			_table->setCellWidget(ri, CComponent, makeChoiceCell(ri, CComponent, row, curComp));
		}
		_table->blockSignals(false);
		for (int vr = 0; vr < _table->rowCount(); ++vr)
			updateRowVisibility(vr);
	}

	void mChannelMapDlg::updateRowVisibility(int vr)
	{
		//当前过滤模式 -> 行可见性（0全部/1已匹配/2未匹配；全行模型下过滤只动
		//可见性，隐藏行留原位——design D6 用户裁定语义）
		const int mode = _filterCombo ? _filterCombo->currentIndex() : 0;
		QTableWidgetItem* fileItem = _table->item(vr, CFile);
		const int ri = fileItem ? fileItem->data(Qt::UserRole).toInt() : -1;
		bool visible = true;
		if (ri >= 0 && ri < _rows.size())
		{
			if (mode == 1 && !_rows.at(ri).mapped.complete)
				visible = false;   //已匹配视图：藏未匹配行
			if (mode == 2 && _rows.at(ri).mapped.complete)
				visible = false;   //未匹配视图：藏已匹配行
		}
		_table->setRowHidden(vr, !visible);
	}

	QString mChannelMapDlg::cellValue(int vr, int col) const
	{
		//存档文本在同行 item 上（控件只做呈现与交互）
		if (auto* it = _table->item(vr, col))
			return it->text();
		return QString();
	}

	QWidget* mChannelMapDlg::makeChoiceCell(int vr, int col,
		const mChannelMapCore::Row& row, const QString& cur)
	{
		//四列同款单元格（需求图12）：常驻下拉（无边框无下箭头，选即改配）+ 状态
		//标识图；候选走 _candCache（同上下文全表一次现算），当前值不在候选（失配）
		//时补入私有模型回显；选即改配——存档文本写 item，队列化提交（提交走本行
		//差量重填 refillRow，延迟一拍为脱离组合框信号栈）
		QWidget* cell = new QWidget(_table);
		QHBoxLayout* lay = new QHBoxLayout(cell);
		lay->setContentsMargins(2, 0, 6, 0);
		lay->setSpacing(4);
		QComboBox* combo = new QComboBox(cell);
		combo->setToolTip(cur);   //长通道名悬停回显（对齐旧文本单元格）
		//需求图12：四列同款选择组控件——隐藏边框与下箭头（纯文本观感）。对话框显示
		//时主题整树替换控件 QSS（ThemeManager::applyDialogTheme），先打豁免属性
		//保住本样式表（约定见 ThemeManager 组合框分支）
		combo->setProperty("mxComboNoTheme", true);
		combo->setStyleSheet(QStringLiteral(
			"QComboBox{background:transparent;border:none;}"
			"QComboBox::drop-down{width:0px;border:none;}"
			"QComboBox::down-arrow{width:0px;height:0px;}"));
		fillCombo(combo, col, row.mapped, cur);
		QLabel* mark = new QLabel(cell);
		mark->setPixmap(stateMarkPixmap(row.mapped.complete, row.confirmed));
		mark->setFixedSize(16, 16);
		lay->addWidget(combo, 1);
		lay->addWidget(mark);
		connect(combo, &QComboBox::currentTextChanged, cell,
			[this, vr, col](const QString& text) {
				if (auto* it = _table->item(vr, col))
					it->setText(text);
				QTimer::singleShot(0, this, [this, vr, col]() { commitCell(vr, col); });
			});
		return cell;
	}

	QString mChannelMapDlg::candKey(int col, const ChannelSelection& sel) const
	{
		//\x1f 不可见分隔（显示串本身可含 '|' 等字符，拼接键须无歧义）
		return sel.filePath + QChar(0x1F) + QString::number(col) + QChar(0x1F)
			+ QString::number(sel.subcaseId) + QChar(0x1F) + sel.schema + QChar(0x1F)
			+ QString::number(sel.pointId);
	}

	QStringList mChannelMapDlg::candidatesFor(int col, const ChannelSelection& sel)
	{
		//候选缓存（optimize-channel-map-perf）：同 (列,映射上下文) 只现算一次；
		//候选枚举本体仍由 mChannelMapCore 提供（规则唯一实现处不变）
		const QString key = candKey(col, sel);
		const auto it = _candCache.constFind(key);
		if (it != _candCache.constEnd())
			return it.value();
		QStringList items;
		if (col == CSubcase)
			items = mChannelMapCore::subcaseTexts(sel.filePath);
		else if (col == CSchema)
			items = mChannelMapCore::schemaTexts(sel.filePath, sel.subcaseId);
		else if (col == CObject)
			items = mChannelMapCore::objectTexts(sel.filePath, sel.subcaseId, sel.schema);
		else
			items = mChannelMapCore::componentTexts(sel.filePath, sel.subcaseId,
				sel.schema, sel.pointId);
		_candCache.insert(key, items);
		return items;
	}

	QStringListModel* mChannelMapDlg::sharedModelFor(const QString& key,
		const QStringList& items)
	{
		//同上下文共享一个列表模型（optimize-channel-map-perf）：N×P 行复用同一
		//QComboBox 模型，模型 parent=对话框随其销毁，缓存无悬挂
		const auto it = _modelCache.constFind(key);
		if (it != _modelCache.constEnd())
			return it.value();
		QStringListModel* model = new QStringListModel(items, this);
		_modelCache.insert(key, model);
		return model;
	}

	void mChannelMapDlg::fillCombo(QComboBox* combo, int col,
		const ChannelSelection& sel, const QString& cur)
	{
		//四列下拉统一回填（QSignalBlocker + prepend 回显/失配选首规则同现行）：
		//cur 在候选 -> 共享模型（候选原序，含 cur）；不在 -> 私有模型补 cur 回显
		const QSignalBlocker blocker(combo);   //回填不触发改配
		const QStringList items = candidatesFor(col, sel);
		if (items.contains(cur))
		{
			combo->setModel(sharedModelFor(candKey(col, sel), items));
		}
		else
		{
			QStringList modelItems = items;
			modelItems.prepend(cur);
			combo->setModel(new QStringListModel(modelItems, combo));
		}
		int idx = combo->findText(cur);
		idx = idx < 0 ? 0 : idx;
		combo->setCurrentIndex(idx);
	}

	void mChannelMapDlg::commitCell(int vr, int col)
	{
		Q_UNUSED(col);
		QTableWidgetItem* fileItem = _table->item(vr, CFile);
		if (!fileItem)
			return;
		const int ri = fileItem->data(Qt::UserRole).toInt();
		if (ri < 0 || ri >= _rows.size())
			return;

		//行映射按四列单元格现值重建（confirmed 退回——改配即未确认）
		const ChannelSelection sel = rowSelection(ri);
		_rows[ri].mapped = sel;
		_rows[ri].confirmed = false;

		//差量刷新（optimize-channel-map-perf）：延迟一拍只重填本行四格（此刻仍
		//在组合框信号槽内，脱离信号栈后再 setModel/改 tooltip；延迟语义是出栈
		//而非防销毁——本行控件全程不销毁）
		QTimer::singleShot(0, this, [this, vr]() { refillRow(vr); });
	}

	void mChannelMapDlg::refillRow(int vr)
	{
		QTableWidgetItem* fileItem = _table->item(vr, CFile);
		if (!fileItem)
			return;
		const int ri = fileItem->data(Qt::UserRole).toInt();
		if (ri < 0 || ri >= _rows.size())
			return;
		const mChannelMapCore::Row& row = _rows.at(ri);

		//四列现值与 rebuildTable 同规：已配=映射通道显示串，未配=模板原串
		const QString curSub = row.mapped.complete
			? mCurveChannelSelector::subcaseText(row.mapped.subcaseId) : row.subcase;
		const QString curSchema = row.mapped.complete ? row.mapped.schema : row.schema;
		const QString curObject = row.mapped.complete
			? mCurveChannelSelector::objectText(row.mapped.pointId) : row.object;
		const QString curComp = row.mapped.complete ? row.mapped.component : row.component;
		struct CellText { int col; QString text; };
		const CellText cells[] = { { CSubcase, curSub }, { CSchema, curSchema },
			{ CObject, curObject }, { CComponent, curComp } };

		//仅本行四格：item 存档同步 + 下拉重填 + 状态图（其余行零扰动）
		for (const CellText& c : cells)
		{
			if (auto* it = _table->item(vr, c.col))
				it->setText(c.text);   //存档文本与控件解耦的根基（cellValue/rowSelection）
			QWidget* cell = _table->cellWidget(vr, c.col);
			if (!cell)
				continue;
			if (QComboBox* combo = cell->findChild<QComboBox*>())
			{
				combo->setToolTip(c.text);
				fillCombo(combo, c.col, row.mapped, c.text);
			}
			if (QLabel* mark = cell->findChild<QLabel*>())
				mark->setPixmap(stateMarkPixmap(row.mapped.complete, row.confirmed));
		}
	}

	ChannelSelection mChannelMapDlg::rowSelection(int row) const
	{
		ChannelSelection sel;
		const mChannelMapCore::Row& r = _rows.at(row);
		sel.filePath = r.mapped.filePath;
		//按行集下标反查可视行（过滤后行号会变；差量过滤隐藏行 item 仍在原位，反查兼容）
		int vr = -1;
		for (int i = 0; i < _table->rowCount(); ++i)
		{
			if (_table->item(i, CFile)
				&& _table->item(i, CFile)->data(Qt::UserRole).toInt() == row)
			{
				vr = i;
				break;
			}
		}
		if (vr < 0)
			return sel;
		//一次四参全传反解（optimize-channel-map-perf：原两次两参调用合并——各段
		//反解相互独立、schema/component 纯透传，结果逐字段相等）
		const ChannelSelection located = mCurveChannelSelector::fromModelTexts(
			r.mapped.filePath, cellValue(vr, CSubcase), cellValue(vr, CSchema),
			cellValue(vr, CObject), cellValue(vr, CComponent));
		sel.subcaseId = located.subcaseId;
		sel.schema = located.schema;
		sel.pointId = located.pointId;
		sel.component = located.component;
		//即时存在性探针（optimize-channel-map-perf）：与装配成败严格互充，零数据
		//复制（原试装配整份拷出通道序列只为丢弃）
		QString perr;
		if (mCurveDataSourceStore::getInstance()->hasChannelData(sel.filePath,
			sel.subcaseId, sel.schema, sel.pointId, sel.component, &perr))
			sel.complete = true;
		return sel;
	}

	void mChannelMapDlg::onFilterChanged(int)
	{
		//差量过滤（optimize-channel-map-perf）：原位 setRowHidden 零重建（全行
		//模型，批次5 起为唯一过滤语义）
		for (int vr = 0; vr < _table->rowCount(); ++vr)
			updateRowVisibility(vr);
	}

	void mChannelMapDlg::onAutoConnectToggled(bool checked)
	{
		//批次5 差量化（design D6）：仅映射状态实际变化的行重填，零全表重建
		QVector<int> changed;
		if (checked)
		{
			//勾选：失配行按相似度预配（档随"自动填充"，无=默认文件名档）；
			//快照失配行 before 映射，跑完后逐行 diff
			QHash<int, ChannelSelection> before;
			for (int i = 0; i < _rows.size(); ++i)
			{
				if (!_rows.at(i).mapped.complete && !_rows.at(i).exactHit)
					before.insert(i, _rows.at(i).mapped);
			}
			mChannelMapCore::autoConnectSimilar(&_rows, prefillMode(_autoFillMode));
			for (auto it = before.constBegin(); it != before.constEnd(); ++it)
			{
				if (selDiffers(_rows.at(it.key()).mapped, it.value()))
					changed << it.key();
			}
		}
		else
		{
			//取消勾选：失配行退回未配（精确命中不动）；只记实际发生退回的行
			for (int i = 0; i < _rows.size(); ++i)
			{
				mChannelMapCore::Row& r = _rows[i];
				if (!r.exactHit && (r.mapped.complete || r.confirmed))
				{
					r.mapped.complete = false;
					r.confirmed = false;
					changed << i;
				}
			}
		}
		for (int ri : changed)
		{
			refillRow(ri);            //全行模型：表行 = 行集下标
			updateRowVisibility(ri);   //complete 翻转需同步过滤可见性
		}
	}

	void mChannelMapDlg::onBestMatchClicked()
	{
		//只对未匹配行（精确命中不动）；映射变化行差量刷新（批次5，零全表重建）
		QHash<int, ChannelSelection> before;
		for (int i = 0; i < _rows.size(); ++i)
		{
			if (!_rows.at(i).mapped.complete && !_rows.at(i).exactHit)
				before.insert(i, _rows.at(i).mapped);
		}
		mChannelMapCore::bestMatchUnmatched(&_rows);
		for (auto it = before.constBegin(); it != before.constEnd(); ++it)
		{
			if (selDiffers(_rows.at(it.key()).mapped, it.value()))
			{
				refillRow(it.key());            //全行模型：表行 = 行集下标
				updateRowVisibility(it.key());
			}
		}
	}

	void mChannelMapDlg::onResetClicked()
	{
		_rows = _initRows;                         //恢复初始映射
		mChannelMapCore::initExactMatches(&_rows); //（重置=仅精确匹配）
		rebuildTable();
	}

	void mChannelMapDlg::onSaveCsvClicked()
	{
		const QString filePath = QFileDialog::getSaveFileName(this,
			QStringLiteral("保存通道映射配置"), QString(),
			QStringLiteral("CSV (*.csv)"));
		if (filePath.isEmpty())
			return;
		QString err;
		if (!mChannelMapCore::saveCsv(_rows, filePath, &err))
			QMessageBox::warning(this, QStringLiteral("通道映射"), err);
	}

	void mChannelMapDlg::onLoadCsvClicked()
	{
		const QString filePath = QFileDialog::getOpenFileName(this,
			QStringLiteral("导入通道映射配置"), QString(),
			QStringLiteral("CSV (*.csv)"));
		if (filePath.isEmpty())
			return;
		QString err;
		if (!mChannelMapCore::loadCsv(filePath, &_rows, &err))
			QMessageBox::warning(this, QStringLiteral("通道映射"), err);
		rebuildTable();
	}

	void mChannelMapDlg::onConfirmClicked()
	{
		//已匹配行全部转绿勾（不关闭对话框，对齐设计图确认后绿勾仍可见）
		for (int i = 0; i < _rows.size(); ++i)
		{
			if (_rows[i].mapped.complete)
				_rows[i].confirmed = true;
		}
		//差量刷新（optimize-channel-map-perf）：只翻可视行四格状态图，零表格重建
		//（行集/模型/存档文本均无变化，无需重建）
		for (int vr = 0; vr < _table->rowCount(); ++vr)
		{
			QTableWidgetItem* fileItem = _table->item(vr, CFile);
			if (!fileItem)
				continue;
			const int ri = fileItem->data(Qt::UserRole).toInt();
			if (ri < 0 || ri >= _rows.size() || !_rows.at(ri).mapped.complete)
				continue;   //未匹配行红叉不动
			for (int col = CSubcase; col <= CComponent; ++col)
			{
				QWidget* cell = _table->cellWidget(vr, col);
				if (!cell)
					continue;
				if (QLabel* mark = cell->findChild<QLabel*>())
					mark->setPixmap(stateMarkPixmap(true, true));
			}
		}
	}

	/*************************** 标题栏拖动（对齐 mLayerManageDlg） ***************************/

	void mChannelMapDlg::mousePressEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton && _titleBar->underMouse())
			_pressPos = event->globalPos();
		QDialog::mousePressEvent(event);
	}

	void mChannelMapDlg::mouseMoveEvent(QMouseEvent* event)
	{
		if ((event->buttons() & Qt::LeftButton) && !_dragging
			&& _titleBar->underMouse()
			&& (event->globalPos() - _pressPos).manhattanLength()
				>= QApplication::startDragDistance())
		{
			_dragging = true;
			_dragOffset = event->globalPos() - frameGeometry().topLeft();
		}
		if (_dragging && (event->buttons() & Qt::LeftButton))
			move(event->globalPos() - _dragOffset);
		QDialog::mouseMoveEvent(event);
	}

	void mChannelMapDlg::mouseReleaseEvent(QMouseEvent* event)
	{
		_dragging = false;
		QDialog::mouseReleaseEvent(event);
	}
}
