#include "mNVHContributorPickDlg.h"

#include "mDlgGeometry.h"   //PostCurve::DlgGeom（本次运行内几何记忆）

#include "UiMetrics.h"      //MxFunctions::UiMetrics::zoneRadius（主钮圆角档）
#include "UiPalette.h"      //MxFunctions::UiPalette 语义槽（主钮取色三主题各自成立）
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace PostWindow
{
	namespace
	{
		/*
		* 主按钮形态（蓝底白字圆角 4）：照抄 mNVHDisplayOptionsDlg::makePrimaryButton
		* 家族先例（mFramelessMessageBox 原型形态块同源）——取色全走 UiPalette 语义
		* 槽，三主题各自成立；mxBtnNoTheme=ThemeManager 按钮分支豁免约定，运行时
		* 主题重发不覆盖（ui-spec §1）
		*/
		void makePrimaryButton(QPushButton* primaryBtn)
		{
			primaryBtn->setProperty("mxBtnNoTheme", true);
			primaryBtn->setStyleSheet(QStringLiteral(
				"QPushButton { background:%1; color:%2; border:none; border-radius:%3px; }"
				"QPushButton:hover { background:%4; }"
				"QPushButton:pressed { background:%5; }")
				.arg(MxFunctions::UiPalette::primaryHex(), MxFunctions::UiPalette::onActionHex())
				.arg(MxFunctions::UiMetrics::zoneRadius())
				.arg(MxFunctions::UiPalette::primary().lighter(108).name())
				.arg(MxFunctions::UiPalette::actionHex()));
		}

		/*
		* % To Exclude 编辑器代理：原型=纯文本观感（100.0，1 位小数右对齐），编辑
		* 时才现 QDoubleSpinBox（0~100）。不走常驻 spinbox+QSS 文本化三件套：本框
		* 是白卡弹窗，ThemeManager 整树补发有 QDoubleSpinBox 分支，构造期控件级
		* QSS 运行时被抹（ui-spec §2/§3）——delegate 按需创建编辑器零 QSS 风险。
		*/
		class PctEditDelegate : public QStyledItemDelegate
		{
		public:
			explicit PctEditDelegate(double defaultPct, QObject* parent = nullptr)
				: QStyledItemDelegate(parent), _defaultPct(defaultPct) {}

			QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&,
				const QModelIndex&) const override
			{
				auto* spin = new QDoubleSpinBox(parent);
				spin->setRange(0.0, 100.0);
				spin->setDecimals(1);
				return spin;
			}

			void setEditorData(QWidget* editor, const QModelIndex& index) const override
			{
				auto* spin = qobject_cast<QDoubleSpinBox*>(editor);
				if (spin == nullptr)
					return;
				const double v = index.data(Qt::EditRole).toDouble();
				spin->setValue(v > 0.0 ? v : _defaultPct);   //空/非法回默认（页内去除比例）
			}

			void setModelData(QWidget* editor, QAbstractItemModel* model,
				const QModelIndex& index) const override
			{
				auto* spin = qobject_cast<QDoubleSpinBox*>(editor);
				if (spin == nullptr)
					return;
				//存档文本落 item（ui-spec §4：取值与控件解耦），1 位小数右对齐形态
				model->setData(index, QString::number(spin->value(), 'f', 1),
					Qt::EditRole);
			}

		private:
			double _defaultPct;
		};
	}

	mNVHContributorPickDlg::mNVHContributorPickDlg(const std::map<int, double>& modeFreqs,
		double defaultPct, const std::vector<int>& preselectedModes, QWidget* parent)
		: MechMainWindow::mFramelessDialog(parent)
		, _defaultPct(defaultPct)
	{
		setWindowTitle(QStringLiteral("选择贡献项"));

		//白卡弹窗家族标题栏（常数与接线照 mNVHDisplayOptionsDlg 同款：带高 62、
		//logo 14×16 左缘 20 紧排、标题 13px/600、仅✗关闭钮、无标题栏下分隔线；
		//mxBtnNoTheme 豁免约定见 makePrimaryButton）
		setWindowIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/logo.png")));
		setTitleBarSeparatorVisible(false);
		setTitleBarCloseOnly(true);
		setTitleBarHeight(62);
		setTitleBarIconSize(QSize(14, 16));
		setTitleBarFont(13, 600);
		setTitleBarLayoutCompact(20, 5);
		setTitleBarDialogFlatClose(true);

		buildUi(modeFreqs, preselectedModes);
		//原型整窗 480×468（标题带 62 + 内容 406）。可缩放白卡真机可见卡=编程尺寸
		//+16×39 家族膨胀档（rules/ui-spec-mxmech §6 口径，同 mNVHDisplayOptionsDlg）
		resize(480, 468);
		PostCurve::DlgGeom::bind(this, "NVHContributorPickDlg",
			PostCurve::DlgGeom::Mode::Geometry);
	}

	std::map<int, double> mNVHContributorPickDlg::selectedRatios() const
	{
		//选中行带出 模态号→%To Exclude（存档：模态号在 Contributor 单元格
		//UserRole、% 值在单元格文本，控件销毁不丢——ui-spec §4）
		std::map<int, double> ratios;
		const auto rows = _table->selectionModel()->selectedRows();
		for (const auto& rowIndex : rows)
		{
			auto* modeItem = _table->item(rowIndex.row(), 0);
			if (modeItem == nullptr)
				continue;
			auto* pctItem = _table->item(rowIndex.row(), 1);
			//解析失败（空/非数）回默认；显式 0 是合法输入（0%=保留该模态），勿吞
			bool ok = false;
			const double pct = pctItem != nullptr
				? pctItem->text().toDouble(&ok) : 0.0;
			ratios.emplace(modeItem->data(Qt::UserRole).toInt(),
				(ok && pct >= 0.0) ? pct : _defaultPct);
		}
		return ratios;
	}

	void mNVHContributorPickDlg::onAccept()
	{
		accept();
	}

	void mNVHContributorPickDlg::buildUi(const std::map<int, double>& modeFreqs,
		const std::vector<int>& preselectedModes)
	{
		//行结构（原型 2026-11-11 实测 480×468：表格上缘距标题带 10、表格右缘
		//x474=右边距 20、表格底 y405、表格→底钮气口 30、底钮 44×26 右贴下边距
		//20）：表格（行号竖头+两列）+ 底钮行，无顶部说明行（原型即无）。
		//裸布局——setExternalLayout 契约（mFramelessDialog.cpp:96：未挂部件的
		//布局才被 content 容器接管，家族正确模式见 mMenuWidget 各调用点）
		auto* lay = new QVBoxLayout;
		lay->setContentsMargins(20, 10, 20, 20);
		lay->setSpacing(30);   //表格→底钮气口（原型 405→435 实测）

		//—— 表格：行号竖头 + Contributor + % To Exclude（可编辑，默认页内去除比例） ——
		//原型列几何：竖头灰带宽 41（x34-75）、Contributor 列 stretch（199）、
		//% 列定宽 200（值右对齐右衬 ~10）；行节距 26、表头带 28（随主题）
		//子件不带 parent（家族同款：布局安装时统一 reparent 到 content 容器）
		_table = new QTableWidget(static_cast<int>(modeFreqs.size()), 2);
		_table->setHorizontalHeaderLabels({ QStringLiteral("Contributor"),
			QStringLiteral("% To Exclude") });
		//表头文字左对齐（原型实测左贴；Qt 默认本即左，真机呈居中系宿主主题
		//默认对齐改写——显式钉回，先例 mDefineCurveDlg:1249）
		_table->horizontalHeader()->setDefaultAlignment(
			Qt::AlignLeft | Qt::AlignVCenter);
		_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
		_table->setColumnWidth(1, 200);
		_table->verticalHeader()->setVisible(true);          //原型行号列（灰带 41px）
		_table->verticalHeader()->setFixedWidth(41);
		_table->verticalHeader()->setDefaultSectionSize(26); //原型行节距 26
		_table->setSortingEnabled(false);
		//选择语义=行选中（spec「选中 Mode N」；ExtendedSelection 支持多选，
		//无勾选框列——原型 2026-11-11 即无，裁剪放大复核过）
		_table->setSelectionBehavior(QAbstractItemView::SelectRows);
		_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
		//% 列双击/编辑键进编辑器（delegate 现出 spinbox）；Contributor 列只呈现
		_table->setEditTriggers(QAbstractItemView::DoubleClicked
			| QAbstractItemView::EditKeyPressed);
		_table->setItemDelegateForColumn(1, new PctEditDelegate(_defaultPct, _table));

		int row = 0;
		int firstPreselectRow = -1;
		QItemSelection preselect;
		//QAbstractItemView::model() 是 protected，行号取公开链 selectionModel()
		const auto tableModel = _table->selectionModel()->model();
		for (const auto& pair : modeFreqs)
		{
			//Contributor 列：模态号存档（UserRole），文字不可编辑（只呈现）
			auto* item = new QTableWidgetItem(
				QStringLiteral("Mode %1 : %2 Hz")
					.arg(QString::number(pair.first),
						QString::number(pair.second, 'f', 2)));
			item->setData(Qt::UserRole, pair.first);
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			_table->setItem(row, 0, item);
			//% 列：文本即存档（1 位小数右对齐=原型 100.0 形态），编辑走 delegate
			auto* pctItem = new QTableWidgetItem(
				QString::number(_defaultPct, 'f', 1));
			pctItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			_table->setItem(row, 1, pctItem);
			//页内选中集预选（spec 确认集合语义：回显页内模态列表选中行）
			if (std::find(preselectedModes.begin(), preselectedModes.end(),
				pair.first) != preselectedModes.end())
			{
				if (firstPreselectRow < 0)
					firstPreselectRow = row;
				preselect.append(QItemSelectionRange(tableModel->index(row, 0),
					tableModel->index(row, 1)));
			}
			++row;
		}
		if (!preselect.isEmpty())
		{
			_table->selectionModel()->select(preselect,
				QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
			//当前格随预选行走（不然 (0,0) 残留 current-cell 高亮框——真机复验实象）
			_table->setCurrentCell(firstPreselectRow, 0);
		}
		lay->addWidget(_table, 1);

		//—— 底钮行：取消/确定 44×26（原型实测；右对齐、下边距 20=布局边距；
		//确定=主钮形态 makePrimaryButton 家族先例） ——
		auto* btnRow = new QHBoxLayout();
		btnRow->setSpacing(13);
		btnRow->addStretch(1);
		_cancelBtn = new QPushButton(QStringLiteral("取消"));
		_cancelBtn->setFixedSize(44, 26);
		_okBtn = new QPushButton(QStringLiteral("确定"));
		_okBtn->setFixedSize(44, 26);
		_okBtn->setDefault(true);
		makePrimaryButton(_okBtn);
		btnRow->addWidget(_cancelBtn);
		btnRow->addWidget(_okBtn);
		lay->addLayout(btnRow);

		setExternalLayout(lay, true);   //内容区按当前主题上色（家族同链路）
		connect(_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
		connect(_okBtn, &QPushButton::clicked, this,
			&mNVHContributorPickDlg::onAccept);
	}
}
