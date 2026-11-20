#include "mRibbonWidget.h"
#include "mUiMenu.h"

//QT
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QScrollBar>
#include <QWheelEvent>
#include <QRadioButton>
#include <QMap>
#include <QLineEdit>

#include "mCalcLineEdit.h"
#include "UiPalette.h"
#include "UiMetrics.h"
#include "ThemeManager.h"
#include "mRibbonButton.h"

using namespace MxFunctions;

namespace MechMainWindow
{
	using MxFunctions::mCalcLineEdit;

	namespace
	{
		// 组内容布局统一对齐规则：网格单元内控件紧贴左侧边框且上下居中；
		// 左侧水平条内大按钮上下居中（默认撑满高度会因高度不齐而错位）
		constexpr Qt::Alignment kCellAlignment = Qt::AlignLeft | Qt::AlignVCenter;
	}

	/*	mGroupWidget	*/
	mGroupWidget::mGroupWidget(const QString& title, QWidget* parent)
		: QWidget(parent)
	{
		this->setObjectName("mGroupWidget");
		QVBoxLayout* vLayout = new QVBoxLayout;
		// 规则 1.3：组内 = 按钮区(ribbonFull) + 分类标题区(caption=H−G)，
		// 两区之间无额外间距，功能区上下留白 G−2 由页容器提供
		vLayout->setSpacing(0);
		vLayout->setContentsMargins(0, 0, 0, 0);

		// 水平布局： 水平布局 | 网格布局
		m_hLayout_ = new QHBoxLayout();
		// 同组按钮列间距 menuGap = G−2（规则 1.3：由父容器提供一次，子按钮不追加 margin）
		m_hLayout_->setSpacing(UiMetrics::menuGap());
		m_hLayout_->setContentsMargins(UiMetrics::groupInset(), 0, UiMetrics::groupInset(), 0);
		m_hLayout_->setAlignment(Qt::AlignTop);

		// 左侧：子水平布局
		m_L_hLayout_ = new QHBoxLayout();
		m_L_hLayout_->setSpacing(UiMetrics::menuGap());
		m_L_hLayout_->setContentsMargins(0, 0, 0, 0);

		// 右侧：网格布局（上下二分按钮行距 stackGap = 2，规则 0.1 固定常量）
		m_R_gridLayout_ = new QGridLayout();
		m_R_gridLayout_->setContentsMargins(0, 0, 0, 0);
		m_R_gridLayout_->setHorizontalSpacing(UiMetrics::menuGap());
		m_R_gridLayout_->setVerticalSpacing(UiMetrics::stackGap());

		m_hLayout_->addLayout(m_L_hLayout_);
		m_hLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum));
		m_hLayout_->addLayout(m_R_gridLayout_);


		// 把水平布局套一个 QWidget，方便设置最小高度
		QWidget* contentWidget = new QWidget(this);
		contentWidget->setObjectName("mGroupContentWidget");
		contentWidget->setLayout(m_hLayout_);
		vLayout->insertWidget(0, contentWidget);
		contentWidget->setFixedHeight(UiMetrics::groupContentHeight());

		// 分类标题（规则 1.3：captionFont 标准 12px、标题区高 caption=H−G、水平垂直居中）
		m_groupTitleLabel_ = new QLabel(title);
		m_groupTitleLabel_->setObjectName("mGroupTitleLabel");
		m_groupTitleLabel_->setMinimumHeight(UiMetrics::groupTitleHeight());
		m_groupTitleLabel_->setMaximumHeight(UiMetrics::groupTitleHeight());
		m_groupTitleLabel_->setAlignment(Qt::AlignCenter);
		vLayout->addWidget(m_groupTitleLabel_);
		this->setLayout(vLayout);
		// 组字体/文字色与组标题样式由 ThemeManager::applyGroupWidgetPalette 统一下发
		//（mMenuWidget 构造末尾与后处理页注册后都会 applyAll，此处不再自设一份；
		// QSS 里也绝不写 background，避免 QStyleSheetStyle 覆盖 paintEvent 自绘的组间分隔线）


	}

	mGroupWidget::~mGroupWidget()
	{
		m_buttons.clear();
	}

	QBoxLayout* mGroupWidget::addBoxLayout(QBoxLayout* layout, int row, int col, int rowspan, int columnspan)
	{
		m_R_gridLayout_->addLayout(layout, row, col, rowspan, columnspan, kCellAlignment);
		return layout;
	}

	QToolButton* mGroupWidget::addToolBtn(const QString& buttonName, const QString& icon, int row, int col, int rowspan, int columnSpan, QSize iconSize, int extrawidth)
	{
		// 全自绘按钮 mRibbonButton：图标/文字/箭头/状态背景全部 paintEvent 画，
		// 不再经 Qt 底座样式，从根源消除"双三角/丢三角/文字被图标挤出"
		const bool isBig = (iconSize.width() > 24);
		mRibbonButton* rb = new mRibbonButton(isBig ? mRibbonButton::Mode::Big : mRibbonButton::Mode::Small, this);
		rb->setText(buttonName);
		rb->setIcon(QIcon(icon));
		// 记录意图图标尺寸（仅用于大/小形态判定），运行时图标尺寸由自绘按规则取（H 或 16）
		rb->setProperty("uiIconSize", iconSize);
		rb->setProperty("uiBigButton", isBig);
		rb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		if (row == -1 && col == -1)
		{
			m_L_hLayout_->addWidget(rb, 0, Qt::AlignVCenter);
		}
		else
		{
			m_R_gridLayout_->addWidget(rb, row, col, rowspan, columnSpan, kCellAlignment);
		}
		m_buttons.append(std::shared_ptr<QAbstractButton>(rb));
		return rb;
	}

	QToolButton* mGroupWidget::addToolBtn(const QString& buttonName, const QString& icon, QLayout* layout, QSize iconSize, int extrawidth)
	{
		// 全自绘按钮（二分形态：图标在左文字在右）
		mRibbonButton* rb = new mRibbonButton(mRibbonButton::Mode::Small, this);
		rb->setText(buttonName);
		rb->setIcon(QIcon(icon));
		rb->setProperty("uiIconSize", iconSize);
		rb->setProperty("uiBigButton", false);
		rb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		layout->addWidget(rb);

		m_buttons.append(std::shared_ptr<QAbstractButton>(rb));
		return rb;
	}

	QToolButton* mGroupWidget::addDropDownButton(const QString& buttonName, int row, int col)
	{
		// 全高下拉按钮：全自绘 mRibbonButton（Mode::Big）+ 文字右侧箭头。
		// 取代旧 QPushButton("文字\n▼") 方案——文字里嵌的 ▼ 字形与文字顶对齐、
		// 且 QSS 的 QPushButton::menu-indicator 与字形叠成双箭头，是"箭头贴顶"
		// 修多次无效的根因之一；自绘路径箭头与文字共用基线，视觉中心严格对齐
		mRibbonButton* rb = new mRibbonButton(mRibbonButton::Mode::Big, this);
		rb->setText(buttonName);
		rb->setHasArrow(true);
		rb->setProperty("uiBigButton", true);
		rb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		rb->setFixedHeight(UiMetrics::bigButtonHeight());
		rb->setFixedWidth(rb->sizeHint().width());

		// 菜单（外部 addAction 填充）；展开定位由 mRibbonButton::showMenu 统一负责
		QMenu* menu = new mUiMenu(rb);
		rb->setMenu(menu);

		// 如果指定了行列，放入右侧网格布局
		if (row >= 0 && col >= 0)
			m_R_gridLayout_->addWidget(rb, row, col, 1, 1, kCellAlignment);
		else
			m_L_hLayout_->addWidget(rb, 0, Qt::AlignVCenter);
		m_buttons.append(std::shared_ptr<QAbstractButton>(rb));
		return rb;
	}

	namespace
	{
		// 把行编辑宽度锁死为首次加入布局时的 sizeHint：
		// 样式表由 QStyleSheetStyle 逐条规则求值，sizeHint 取决于获得/失去焦点后
		// 命中的 :focus 规则；若只设最小宽度，Fixed 策略的实际宽度会在 styleHint 变化时
		// 跟着收缩（0.5px 边框规则 → 1px 边框规则），导致整个功能区重排闪烁。
		// 这里把 min/max 宽度都钉在首个 sizeHint 上，焦点样式怎么换宽度都不变。
		void lockLineEditWidth(QLineEdit* lineEdit)
		{
			if (!lineEdit)
				return;
			lineEdit->ensurePolished();
			lineEdit->setFixedWidth(lineEdit->sizeHint().width());
		}
	}

	QLineEdit* mGroupWidget::addLineEdit(QLineEdit* lineEdit, int row, int col, int rowspan, int columnspan)
	{

		lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		if (row >= 0 && col >= 0)
			m_R_gridLayout_->addWidget(lineEdit, row, col, rowspan, columnspan, kCellAlignment);
		else
			m_L_hLayout_->addWidget(lineEdit, 0, Qt::AlignVCenter);

		lockLineEditWidth(lineEdit);
		return lineEdit;
	}

	mCalcLineEdit* mGroupWidget::addCalcLineEdit(mCalcLineEdit* lineEdit, int row, int col, int rowspan, int columnspan)
	{

		lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		if (row >= 0 && col >= 0)
			m_R_gridLayout_->addWidget(lineEdit, row, col, rowspan, columnspan, kCellAlignment);
		else
			m_L_hLayout_->addWidget(lineEdit, 0, Qt::AlignVCenter);

		lockLineEditWidth(lineEdit);
		return lineEdit;
	}

	QLabel* mGroupWidget::addLabel(const QString& labelText, int row, int col, int rowspan, int columnspan)
	{
		QLabel* label = new QLabel(labelText, this);
		label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		// 样式由 ThemeManager::applyGroupWidgetPalette 统一下发（applyAll 必然重发一份，
		// 此处自设的会在首次 applyAll 时被整体替换，不再重复写）
		label->setAlignment(Qt::AlignCenter);

		if (row >= 0 && col >= 0)
			m_R_gridLayout_->addWidget(label, row, col, rowspan, columnspan, kCellAlignment);
		else
			m_L_hLayout_->addWidget(label, 0, Qt::AlignVCenter);
		return label;
	}

    MxFunctions::mCheckComboBox* mGroupWidget::addCombobox(const QString& objectName, int row, int col, int rowspan, int columnspan)
	{
        MxFunctions::mCheckComboBox* combobox = new MxFunctions::mCheckComboBox(this);
		combobox->setObjectName(objectName);

		combobox->setMinimumWidth(100);

		// 字号随档位取功能区字号 menuFont（规则 0.1：与组内按钮文字一致）
		QFont font = combobox->font();
		font.setPixelSize(UiMetrics::buttonFontSize());
		combobox->setFont(font);

		if (row >= 0 && col >= 0)
			m_R_gridLayout_->addWidget(combobox, row, col, rowspan, columnspan, kCellAlignment);
		else
			m_L_hLayout_->addWidget(combobox, 0, Qt::AlignVCenter);
		return combobox;
	}

	QButtonGroup* mGroupWidget::addButtonGroup(const QStringList& names, int row, int col)
	{
		QButtonGroup* group = new QButtonGroup(this);
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(8);

		int count = names.size();
		for (int i = 0; i < count; i++)
		{
			QRadioButton* radio = new QRadioButton(names[i], this);
			group->addButton(radio, i);
			layout->addWidget(radio);

			if (i == 0) radio->setChecked(true);

			if (row >= 0 && col >= 0)
				m_R_gridLayout_->addLayout(layout, row, col, count, 1, kCellAlignment);
			else
				m_L_hLayout_->addWidget(radio, 0, Qt::AlignVCenter);
		}

		return group;
	}

	QDoubleSpinBox* mGroupWidget::addDoubleSpinBox(const QString& objectName, int row, int col, int rowspan, int columnspan)
	{
		QDoubleSpinBox* doubleSpinBox = new QDoubleSpinBox(this);


		doubleSpinBox->setObjectName(objectName);
		doubleSpinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

		if (row >= 0 && col >= 0)
			m_R_gridLayout_->addWidget(doubleSpinBox, row, col, rowspan, columnspan, kCellAlignment);
		else
			m_L_hLayout_->addWidget(doubleSpinBox, 0, Qt::AlignVCenter);
		return doubleSpinBox;
	}

	QToolButton* mGroupWidget::addIconToolButton(const QString& objectName, const QString& icon, int row, int col)
	{
		QToolButton* toolButton = new QToolButton(this);
		toolButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
		toolButton->setObjectName(objectName);
		toolButton->setIcon(QIcon(icon));

		// 悬停/按下底色由 ThemeManager::applyGroupWidgetPalette 的纯图标按钮分支统一下发
		//（applyAll 必然重发，此处自设的会被整体替换，不再重复写）

		if (row == -1 && col == -1)
		{
			m_L_hLayout_->addWidget(toolButton, 0, Qt::AlignVCenter);
		}
		else
		{
			m_R_gridLayout_->addWidget(toolButton, row, col, 1, 1, kCellAlignment);
		}
		return toolButton;
	}

	QCheckBox* mGroupWidget::addCheckBox(const QString& name, int row, int col, int rowspan, int columnspan)
	{
		QCheckBox* checkbox = new QCheckBox(this);
		checkbox->setText(name);

		// 文字/禁用色由 ThemeManager::applyGroupWidgetPalette 统一下发
		//（applyAll 必然重发，此处自设的会被整体替换，不再重复写）

		if (row == -1 && col == -1)
		{
			m_L_hLayout_->addWidget(checkbox, 0, Qt::AlignVCenter);
		}
		else
		{
			m_R_gridLayout_->addWidget(checkbox, row, col, rowspan, columnspan, kCellAlignment);
		}
		return checkbox;
	}

	QSlider* mGroupWidget::addSlider(const QString& name, int row, int col, int rowspan, int columnspan)
	{
		QSlider* slider = new QSlider(this);
		slider->setObjectName(name);
		slider->setOrientation(Qt::Horizontal);
		slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		slider->setFixedWidth(120);

		if (row == -1 && col == -1)
		{
			m_L_hLayout_->addWidget(slider, 0, Qt::AlignVCenter);
		}
		else
		{
			m_R_gridLayout_->addWidget(slider, row, col, rowspan, columnspan, kCellAlignment);
		}
		return slider;
	}

	QPushButton* mGroupWidget::addPushButton(const QString& name, int row, int col, int rowspan, int columnspan)
	{
		QPushButton* pushBtn = new QPushButton(this);
		pushBtn->setText(name);

		if (row == -1 && col == -1)
		{
			m_L_hLayout_->addWidget(pushBtn, 0, Qt::AlignVCenter);
		}
		else
		{
			m_R_gridLayout_->addWidget(pushBtn, row, col, rowspan, columnspan, kCellAlignment);
		}
		return pushBtn;
	}

	QWidget* mGroupWidget::addWidget(QWidget* widget, int row, int col, int rowspan, int columnspan)
	{
		if (row >= 0 && col >= 0)
			m_R_gridLayout_->addWidget(widget, row, col, rowspan, columnspan, kCellAlignment);
		else
			m_L_hLayout_->addWidget(widget, 0, Qt::AlignVCenter);
		return widget;
	}

	void mGroupWidget::applyMetrics(bool activateNow)
	{
		m_hLayout_->setSpacing(UiMetrics::menuGap());
		m_hLayout_->setContentsMargins(UiMetrics::groupInset(), 0,
			UiMetrics::groupInset() + (m_drawSeparator_ ? 1 : 0), 0);
		m_L_hLayout_->setSpacing(UiMetrics::menuGap());
		m_R_gridLayout_->setHorizontalSpacing(UiMetrics::menuGap());
		m_R_gridLayout_->setVerticalSpacing(UiMetrics::stackGap());
		if (QWidget* content = findChild<QWidget*>(QStringLiteral("mGroupContentWidget"),
			Qt::FindDirectChildrenOnly))
			content->setFixedHeight(UiMetrics::groupContentHeight());
		m_groupTitleLabel_->setFixedHeight(UiMetrics::groupTitleHeight());
		m_hLayout_->invalidate();
		m_R_gridLayout_->invalidate();
		if (activateNow && layout())
		{
			layout()->invalidate();
			layout()->activate();
		}
		updateGeometry();
		update();
	}

	void mGroupWidget::setDrawSeparator(bool on)
	{
		m_drawSeparator_ = on;
		applyMetrics();
	}

	void mGroupWidget::updateLayout()
	{
		// 只做同列小按钮的列宽对齐；组底色/子控件主题由 ThemeManager::applyPalette
		// 统一下发（applyAll 内先 applyPalette 再走本函数，这里不再整组重发一遍，
		// 避免每次切档/重排都把全部子控件样式表重建三次）
		// 上下排列的同列按钮（rowspan=1 的 TextBesideIcon 按钮）保持同宽：
		// 悬浮色铺满按钮自身矩形（mRibbonButton 按 rect() 铺色），同列两个小按钮
		// 只有宽度一致悬浮色宽度才一致。取该列全部小按钮（含二分按钮，如"保存"
		// 与"另存为"）规则公式宽的最大值统一下发，与较宽的对齐，避免上下悬浮色
		// 一宽一窄。
		// 注意：mRibbonButton（全自绘）的"二分形态"靠 uiBigButton=false 标记，
		// 不再用 toolButtonStyle 判断（它没有这个 API）
		auto isStackedSmallBtn = [](QAbstractButton* btn) {
			// 大/小判定：优先 uiBigButton 标记，否则按意图图标尺寸
			const QVariant big = btn->property("uiBigButton");
			if (big.isValid())
				return !big.toBool();
			const QVariant isz = btn->property("uiIconSize");
			if (isz.isValid())
				return isz.toSize().width() <= 24;
			return false;
		};
		// 扫描网格：列宽取该列全部上下堆叠小按钮 sizeHint 的最大值
		//（sizeHint 由 mRibbonButton 按规则 1.3 公式给出；二分按钮一并参与统计，
		// 否则"保存"与"另存为"这类整列皆二分按钮的列取不到列宽，各自紧凑宽度不等）
		QMap<int, int> colMaxWidth;
		for (int i = 0; i < m_R_gridLayout_->count(); ++i) {
			QWidget* widget = m_R_gridLayout_->itemAt(i)->widget();
			if (QAbstractButton* btn = qobject_cast<QAbstractButton*>(widget)) {
				int row, col, rowspan, colspan;
				m_R_gridLayout_->getItemPosition(i, &row, &col, &rowspan, &colspan);
				// 只处理上下堆叠的单行小按钮
				if (rowspan == 1 && isStackedSmallBtn(btn))
				{
					colMaxWidth[col] = qMax(colMaxWidth.value(col, 0), btn->sizeHint().width());
				}
			}
		}
		// 下发宽度：同列小按钮统一到列宽（含二分按钮；列宽 ≥ 各自公式宽，文字不会被裁）。
		// 本函数必须是按钮宽度的最终下发者：ThemeManager::applyMetrics 的公式宽兜底
		// 若在其后执行，会把同列已对齐的宽度重新冲开
		for (int i = 0; i < m_R_gridLayout_->count(); ++i) {
			QWidget* widget = m_R_gridLayout_->itemAt(i)->widget();
			if (QAbstractButton* btn = qobject_cast<QAbstractButton*>(widget)) {
				int row, col, rowspan, colspan;
				m_R_gridLayout_->getItemPosition(i, &row, &col, &rowspan, &colspan);
				if (rowspan == 1 && isStackedSmallBtn(btn) && colMaxWidth.contains(col))
				{
					// 先解除固定宽度，否则旧 min/max 会拦住新值
					btn->setMinimumWidth(0);
					btn->setMaximumWidth(QWIDGETSIZE_MAX);
					btn->setFixedWidth(colMaxWidth.value(col));
				}
			}
		}

		// 同排按钮等宽（setUniformRowButtons 开启的组）：同一行 rowspan=1 的按钮
		// 统一为该行最宽一钮。宽度取按钮公式 sizeHint（mRibbonButton 的 sizeHint
		// 不受固定宽度影响，运行期换字后重跑本函数仍能得到正确的行宽）；
		// 在列对齐之后执行，是这类组按钮宽度的最终下发者
		if (m_uniformRowButtons_)
		{
			QMap<int, int> rowMaxWidth;
			for (int i = 0; i < m_R_gridLayout_->count(); ++i) {
				QWidget* widget = m_R_gridLayout_->itemAt(i)->widget();
				if (QAbstractButton* btn = qobject_cast<QAbstractButton*>(widget)) {
					int row, col, rowspan, colspan;
					m_R_gridLayout_->getItemPosition(i, &row, &col, &rowspan, &colspan);
					if (rowspan == 1)
						rowMaxWidth[row] = qMax(rowMaxWidth.value(row, 0), btn->sizeHint().width());
				}
			}
			for (int i = 0; i < m_R_gridLayout_->count(); ++i) {
				QWidget* widget = m_R_gridLayout_->itemAt(i)->widget();
				if (QAbstractButton* btn = qobject_cast<QAbstractButton*>(widget)) {
					int row, col, rowspan, colspan;
					m_R_gridLayout_->getItemPosition(i, &row, &col, &rowspan, &colspan);
					if (rowspan == 1 && rowMaxWidth.contains(row))
					{
						btn->setMinimumWidth(0);
						btn->setMaximumWidth(QWIDGETSIZE_MAX);
						btn->setFixedWidth(rowMaxWidth.value(row));
					}
				}
			}
		}
	}

	void mGroupWidget::paintEvent(QPaintEvent* event)
	{
		Q_UNUSED(event);
		// 组底色用 palette 显式填充（不走 QSS background：QSS 背景会触发
		// QStyleSheetStyle 接管本控件绘制，paintEvent 里的自绘内容会丢失）。
		// 同一控件同一时刻只能有一个 QPainter：先前底色/分隔线各开一个 painter，
		// 第二个 begin 必然失败（每帧告警且分隔线实际画不出来）
		QPainter p(this);
		p.fillRect(rect(), palette().color(QPalette::Window));

		// 功能组分区线（规则 1.3）：高 = 按钮区＋分类标题区 = 3H+G（标准 84），
		// 不含功能区上下留白；画在本组右侧 1px 边界槽，与覆盖层使用相同坐标。
		if (m_drawSeparator_)
		{
			const int x = width() - 2;
			p.setPen(UiPalette::border());
			p.drawLine(x, 0, x, UiMetrics::groupSeparatorHeight() - 1);
		}
	}

	/*	mRibbonSeparatorOverlay：画在 viewport 之上的透明覆盖层	*/
	mRibbonSeparatorOverlay::mRibbonSeparatorOverlay(RibbonPage* page, QWidget* parent)
		: QWidget(parent), m_page_(page)
	{
		// 完全透明、不接收鼠标，只做绘制：覆盖在所有组控件之上，
		// 因此它画的线不会被任何子控件实底盖住（此前在 viewport/组内
		// 画线反复"消失/只剩一截"的根源就是绘制层在子控件之下）
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_TranslucentBackground);
	}

	void mRibbonSeparatorOverlay::paintEvent(QPaintEvent*)
	{
		if (!m_page_)
			return;
		QPainter p(this);
		p.setPen(UiPalette::border());
		// 功能区上下分界线（页顶/页底各一行，全宽）：原由 QSS border 绘制，
		// 但 QSS 边框会把 viewport 缩进边框宽度，使内容最小高（准则 ribbonHeight）
		// 超出可视区 2px 产生竖向滚动范围（滑轮可小幅滚动菜单）；
		// 页自身 paintEvent 画线又会被 viewport 的不透明背景盖住，
		// 因此改画在本覆盖层（viewport 之上、所有组控件之上），几何与视觉两全
		p.drawLine(0, 0, width() - 1, 0);
		p.drawLine(0, height() - 1, width() - 1, height() - 1);
		// 功能组分区线（规则 1.3）：高 = 按钮区＋分类标题区 = 3H+G（标准 84），
		// 不含功能区上下留白；画在本组右侧 1px 边界槽。
		// 覆盖层浮在所有组控件之上，线不会被任何子控件实底盖住
		//（此前在 viewport/组内画线反复"消失/只剩一截"的根源就是绘制层在子控件之下）
		const int y0 = UiMetrics::ribbonTopInset();
		const int y1 = y0 + UiMetrics::groupSeparatorHeight() - 1;
		const auto groups = m_page_->groups();
		for (mGroupWidget* g : groups)
		{
			//隐藏组不画分隔线：隐藏控件不参与布局，x()/width() 是滞留几何，
			//按它画线会落进相邻组内容区凭空多出竖线（如多体形态整组隐藏的
			//"实体化"组）。用 isHidden 而非 isVisible：页签切走时整页后代
			//isVisible 皆 false，isHidden 只反映组自身的显式隐藏
			if (!g || g->isHidden() || !g->drawsSeparator())
				continue;
			// 组在 viewport 内的 x：用 content 坐标减去滚动值
			const int x = g->x() + g->width() - 2
				- m_page_->horizontalScrollBar()->value();
			if (x < -1 || x > width() + 1)
				continue;
			p.drawLine(x, y0, x, y1);
		}
	}

	/*	RibbonPage	*/
	RibbonPage::RibbonPage(QWidget* parent) : QScrollArea(parent)
	{
		// 样式由 ThemeManager::applyAll 统一下发（此处不再硬编码颜色）

		m_widget_ = new QWidget();
		m_widget_->setObjectName("mRibbonContentWidget");
		m_widget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		QHBoxLayout* hlay = new QHBoxLayout();
		hlay->addStretch();
		// 功能区顶部留白 ribbonTopInset = G−2（规则 1.3：上4＋按钮64＋分类20＋下4）
		hlay->setContentsMargins(0, UiMetrics::ribbonTopInset(), 0, UiMetrics::ribbonBottomInset());
		hlay->setSpacing(0);
		m_widget_->setLayout(hlay);

		this->setWidget(m_widget_);
		this->setWidgetResizable(true);

		this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		m_leftScrollBtn_ = new QToolButton(this);
		m_leftScrollBtn_->setObjectName("m_leftScrollBtn");
		m_leftScrollBtn_->setArrowType(Qt::LeftArrow);
		m_leftScrollBtn_->setVisible(false);

		m_rightScrollBtn_ = new QToolButton(this);
		m_rightScrollBtn_->setObjectName("m_rightScrollBtn");
		m_rightScrollBtn_->setArrowType(Qt::RightArrow);
		m_rightScrollBtn_->setVisible(false);

		m_maxScrollValue_ = this->horizontalScrollBar()->maximum();
		connect(m_leftScrollBtn_, &QToolButton::clicked, this, &RibbonPage::slot_LeftScrollButtonClicked);
		connect(m_rightScrollBtn_, &QToolButton::clicked, this, &RibbonPage::slot_RightScrollButtonClicked);

		// 分隔线覆盖层：浮在所有组之上，滚动/缩放时同步几何并重绘
		m_sepOverlay_ = new mRibbonSeparatorOverlay(this, viewport());
		m_sepOverlay_->setGeometry(viewport()->rect());
		m_sepOverlay_->raise();
		m_sepOverlay_->show();
		connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
			[this](int value) {
				m_XBase_ = value;
				if (m_sepOverlay_)
					m_sepOverlay_->update();
			});
		connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this,
			[this](int, int) { updateMenuGeometry(); });

		// 翻页按钮样式由 ThemeManager::applyAll 统一下发
	}

	void RibbonPage::paintEvent(QPaintEvent* event)
	{
		// 组间分隔线由各 mGroupWidget 自绘（见 mGroupWidget::paintEvent）。
		// 曾经尝试在 viewport 层统一画"满高"线：widget 链上任意一环变成不透明
		//（QSS 背景、setAutoFillBackground、ThemeManager 的 .QWidget 类选择器实底），
		// 线就会被整段盖住，多轮修复反复失效，故不再采用该路径。
		// 功能区上下分界线同理不能画在本页（viewport 不透明背景会盖住），
		// 由 mRibbonSeparatorOverlay 覆盖层统一绘制（见其 paintEvent）。
		QScrollArea::paintEvent(event);
	}

	RibbonPage::~RibbonPage()
	{

	}

	void RibbonPage::wheelEvent(QWheelEvent* event)
	{
		// 竖向滑轮一律吞掉（菜单栏高度固定，不允许竖向滚动）；
		// 横向分量（触摸板横滑/倾斜滚轮）仍交由基类走横向滚动条
		if (qAbs(event->angleDelta().x()) < qAbs(event->angleDelta().y()))
		{
			event->ignore();
			return;
		}
		QScrollArea::wheelEvent(event);
	}

	int RibbonPage::totalWidth()
	{
		if (m_widget_ && m_widget_->layout())
		{
			m_widget_->layout()->activate();
			return m_widget_->layout()->sizeHint().width();
		}
		return 0;
	}

	QHBoxLayout* RibbonPage::contentLayout() const
	{
		return m_widget_ ? qobject_cast<QHBoxLayout*>(m_widget_->layout()) : nullptr;
	}

	void RibbonPage::applyMetrics()
	{
		if (QHBoxLayout* lay = contentLayout())
		{
			lay->setContentsMargins(0, UiMetrics::ribbonTopInset(), 0,
				UiMetrics::ribbonBottomInset());
			lay->invalidate();
			lay->activate();
		}
		// 宽度收口/翻页按钮几何统一在 updateMenuGeometry 内完成
		//（开头三行与 applyMetrics 原先重复执行一遍，全为幂等操作，去掉）
		updateMenuGeometry();
	}

	void RibbonPage::updateMenuGeometry()
	{
		m_totalWidth_ = totalWidth();
		m_widget_->setMinimumWidth(m_totalWidth_);
		m_widget_->updateGeometry();

		int temMin = horizontalScrollBar()->minimum();
		int temMax = horizontalScrollBar()->maximum();

		m_XBase_ = qBound(temMin, m_XBase_, temMax);
		m_isLeftScrollBtnShow_ = temMax > temMin && m_XBase_ > temMin;
		m_isRightScrollBtnShow_ = temMax > temMin && m_XBase_ < temMax;

		m_leftScrollBtn_->setVisible(m_isLeftScrollBtnShow_);
		m_rightScrollBtn_->setVisible(m_isRightScrollBtnShow_);
		if (m_isLeftScrollBtnShow_)
		{
			m_leftScrollBtn_->raise();
		}
			if (m_rightScrollBtn_->isVisible())
				m_rightScrollBtn_->raise();
		// 分隔线覆盖层始终在最顶（翻页按钮在上一步 raise 过，这里压回线层之下）
		if (m_sepOverlay_)
		{
			m_sepOverlay_->raise();
			m_sepOverlay_->update();
		}
		// 滚动按钮位置随档位重算：垂直方向与组内容区（顶边距5起）对齐，不再硬编码偏移6，
		// 高度由 applyMetrics 统一设置，此处仅保持宽度12
		const int scrollBtnH = UiMetrics::groupContentHeight() - 2;
		m_leftScrollBtn_->setGeometry(0, UiMetrics::ribbonTopInset(), 12, scrollBtnH);
		m_rightScrollBtn_->setGeometry(viewport()->width() - 12,
			UiMetrics::ribbonTopInset(), 12, scrollBtnH);

		horizontalScrollBar()->setValue(m_XBase_);
	}

	void RibbonPage::appendGroup(mGroupWidget* group)
	{
		// 调用方已把分组插入尾部拉伸项之前，这里不能再次 insertWidget：
		// Qt 会把已有项移动到拉伸项之后，使空白跑到所有分组左侧。
		if (groupList.contains(group))
			return;
		groupList.append(group);
		for (int i = 0; i < groupList.size(); ++i)
			if (groupList.at(i))
				groupList.at(i)->setDrawSeparator(i < groupList.size() - 1);
		if (m_sepOverlay_)
		{
			m_sepOverlay_->raise();
			m_sepOverlay_->update();
		}
	}

	void RibbonPage::resizeEvent(QResizeEvent* event)
	{
		QScrollArea::resizeEvent(event);

		// 覆盖层跟随 viewport 几何，保证分隔线满高且随尺寸变化
		if (m_sepOverlay_)
		{
			m_sepOverlay_->setGeometry(viewport()->rect());
			m_sepOverlay_->raise();
		}

		m_XBase_ = horizontalScrollBar()->value();
		this->updateMenuGeometry();
	}

	void RibbonPage::slot_LeftScrollButtonClicked()
	{
		// 当前滚动条的值
		m_XBase_ = horizontalScrollBar()->value();

		// 当内容的总宽度大于可见区域时，说明有一部分被遮挡，可以滚动菜单栏
		if (m_totalWidth_ > viewport()->width())
			m_XBase_ = qMax(m_XBase_ - horizontalScrollBar()->pageStep(), 0);
		else
			m_XBase_ = 0;
		updateMenuGeometry();
	}

	void RibbonPage::slot_RightScrollButtonClicked()
	{
		// 当前滚动条的值
		m_XBase_ = horizontalScrollBar()->value();

		if (m_totalWidth_ > viewport()->width())
			m_XBase_ += horizontalScrollBar()->pageStep();
		else
			m_XBase_ = 0;
		updateMenuGeometry();
	}
}
