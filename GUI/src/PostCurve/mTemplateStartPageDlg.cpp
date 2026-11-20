#pragma execution_character_set("utf-8")

// =============================================================================
// mTemplateStartPageDlg —— 选择起始页面对话框（实现），语义见 mTemplateStartPageDlg.h
// =============================================================================

#include "mTemplateStartPageDlg.h"
#include "ThemeManager.h"   //自样式弹窗豁免标记（kSelfStyledProp）

//QT
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace PostCurve
{
	int mTemplateStartPageDlg::pickGroup(QWidget* parent,
		const QList<mCurveLayerManager::GroupInfo>& groups)
	{
		if (groups.isEmpty())
			return -1;   //无组场景调用方负责告警终止
		mTemplateStartPageDlg dlg(groups, parent);
		dlg.exec();
		const int row = dlg._list ? dlg._list->currentRow() : -1;
		if (dlg.result() != QDialog::Accepted || row < 0 || row >= groups.size())
			return -1;
		return groups.at(row).layerId;
	}

	mTemplateStartPageDlg::mTemplateStartPageDlg(
		const QList<mCurveLayerManager::GroupInfo>& groups, QWidget* parent)
		: QDialog(parent, Qt::Window | Qt::FramelessWindowHint)
		, _groups(groups)
	{
		//自样式弹窗：豁免 ThemeManager 整窗重发，保留自带设计稿 QSS
		setProperty(MechMainWindow::ThemeManager::kSelfStyledProp, true);
		setWindowTitle(QStringLiteral("选择起始页面"));
		//需求图形态：无边框白卡片（对齐模板曲线/图层）；尺寸沿用已验收 500×410
		//（原型内框实测 480×388，×1.042/1.057 等比放大；验收标准为等比一致）
		resize(500, 410);

		//白卡片 + M 徽标标题栏（对齐 mLayerManageDlg）；✗ 语义=放弃选择（reject）
		setObjectName("templateStartPageDlg");
		setAttribute(Qt::WA_StyledBackground, true);
		setStyleSheet(
			"QWidget#templateStartPageDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");

		QVBoxLayout* outer = new QVBoxLayout(this);
		outer->setContentsMargins(1, 1, 1, 1);
		outer->setSpacing(0);

		_titleBar = new QWidget(this);
		//"mWindowTitleBar" 为 ui-spec §2 约定名：本框是 QDialog，显示时 ThemeManager
		//会整树补发（applyDialogTheme），只有挂此名的标题栏及其直接子件被跳过——
		//标题 13px/600、✗ 无边框 QSS 靠它存活（对齐 mLayerManageDlg）
		_titleBar->setObjectName("mWindowTitleBar");
		_titleBar->setFixedHeight(52);
		_titleBar->setStyleSheet("background:#FFFFFF;");
		QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
		titleLayout->setContentsMargins(20, 20, 20, 10);
		titleLayout->setSpacing(3);
		QLabel* logoLabel = new QLabel(_titleBar);
		logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
			.scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		QLabel* titleLabel = new QLabel(QStringLiteral("选择起始页面"), _titleBar);
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
		connect(titleCloseBtn, &QToolButton::clicked, this, &QDialog::reject);
		titleLayout->addWidget(logoLabel);
		titleLayout->addWidget(titleLabel, 1);
		titleLayout->addWidget(titleCloseBtn);

		QWidget* body = new QWidget(this);
		body->setStyleSheet("QWidget{background:#FFFFFF;}");
		QVBoxLayout* layout = new QVBoxLayout(body);
		//边距/间距原型实测（原型内框 480×388 → 对话框 500×410，×1.042/1.057 折算）：
		//左右 20（列表卡两侧 20 实测）、顶 16（标题栏底→"选择…"文案 ≈16）、
		//底 20（按钮行下缘 20 实测）；文案→列表卡 ≈8
		layout->setContentsMargins(20, 16, 20, 20);
		layout->setSpacing(8);

		layout->addWidget(new QLabel(
			QStringLiteral("选择一个起始页面："), body));   //文案对齐需求图

		//组列表：单选，默认首行（层序=应用序）；形态=模板曲线模板列表同款
		//（1px 边框圆角 + 选中浅蓝 #CCE8FF，已验收兄弟框先例，原型同形态）
		_list = new QListWidget(body);
		_list->setSelectionMode(QAbstractItemView::SingleSelection);
		//条目高 23=模板曲线列表同款（其原型节距 24×0.9688≈23，兄弟框口径一致；
		//本框原型选中带是贴文字细带、系示意图粗糙——10px 带容不下 12px 文字行盒）。
		//组列表：单选，默认首行（层序=应用序）；形态=模板曲线模板列表同款
		//（1px 边框圆角 + 选中浅蓝 #CCE8FF，已验收兄弟框先例，原型同形态）。
		//条目高 23=模板曲线列表同款（其原型节距 24×0.9688≈23，兄弟框口径一致；
		//本框原型选中带是贴文字细带、系示意图粗糙——10px 带容不下 12px 文字行盒）。
		//height/margin 三态钉死防全局 QSS 26px+margin 8/8/5 漏入（同模板曲线列表）。
		//注意：QSS 不认 // 注释，解释性注释只能写在原始字符串外——写进串内会把
		//解析打断、其后所有 ::item 规则整段静默失效
		_list->setStyleSheet(R"(
			QListWidget{
				background:#FFFFFF; border:1px solid #D9DCE0; border-radius:4px;
				font-size:12px; color:#333333;
			}
			QListWidget::item{ height:23px; margin:0; padding:0 8px; }
			QListWidget::item:hover{ height:23px; }
			QListWidget::item:selected{ height:23px; background:#CCE8FF; color:#333333; }
		)");
		for (const mCurveLayerManager::GroupInfo& g : _groups)
		{
			QListWidgetItem* item = new QListWidgetItem(g.label, _list);
			item->setData(Qt::UserRole, g.layerId);
			item->setToolTip(g.label);
		}
		if (_list->count() > 0)
			_list->setCurrentRow(0);
		layout->addWidget(_list, 1);

		//右下 关闭/确认（需求图：关闭在左，确认=蓝底主按钮，同模板曲线"应用"）。
		//按钮原型实测 44×26（×1.042 折算 ≈46×26，窄于图层框的 60——各随自家原型）；
		//钮间原型 ≈13；列表卡底→按钮行气口 ≈22（8+15）
		QHBoxLayout* buttons = new QHBoxLayout();
		buttons->setSpacing(12);
		const QString whiteBtnStyle = R"(
			QPushButton{
				background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
				color:#333333; padding:2px 14px;
			}
			QPushButton:hover{ background:#EFF1F2; }
		)";
		const QString primaryBtnStyle = R"(
			QPushButton{
				background:#0066CC; border:1px solid #0066CC; border-radius:4px;
				color:#FFFFFF; padding:2px 14px;
			}
			QPushButton:hover{ background:#0073E0; }
		)";
		QPushButton* closeBtn = new QPushButton(QStringLiteral("关闭"), body);
		closeBtn->setFixedSize(46, 26);
		closeBtn->setStyleSheet(whiteBtnStyle);
		connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
		QPushButton* okBtn = new QPushButton(QStringLiteral("确认"), body);
		okBtn->setFixedSize(46, 26);
		okBtn->setStyleSheet(primaryBtnStyle);
		connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
		buttons->addStretch(1);
		buttons->addWidget(closeBtn);   //需求图：关闭 在左
		buttons->addWidget(okBtn);      //确认 在右（主按钮）
		layout->addSpacing(13);   //原型：列表卡底边 y334→按钮顶 y355 白隙 20 行×1.057≈21（spacing 8+13）
		layout->addLayout(buttons);

		outer->addWidget(_titleBar);
		outer->addWidget(body, 1);
	}

	/*************************** 标题栏拖动（对齐 mLayerManageDlg） ***************************/

	void mTemplateStartPageDlg::mousePressEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton && _titleBar->underMouse())
			_pressPos = event->globalPos();
		QDialog::mousePressEvent(event);
	}

	void mTemplateStartPageDlg::mouseMoveEvent(QMouseEvent* event)
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

	void mTemplateStartPageDlg::mouseReleaseEvent(QMouseEvent* event)
	{
		_dragging = false;
		QDialog::mouseReleaseEvent(event);
	}
}
