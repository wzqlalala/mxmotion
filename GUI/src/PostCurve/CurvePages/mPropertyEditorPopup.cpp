#include "mPropertyEditorPopup.h"
#include "mCurvePropPageBase.h"
#include "../mDlgGeometry.h"

#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizeGrip>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QPixmap>

namespace PostCurve
{
	mPropertyEditorPopup::mPropertyEditorPopup(QWidget* parent)
		//属性浮窗与曲线页面并行操作：置顶避免点击页面后被原生视口覆盖
		: QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
	{
		buildUi();
		resize(280, 400);   //UI设计图03.1：宽固定280，高默认400
		//只记高（宽度恒280不随用户调整；位置每次由 showEditorPopupAt 落到图元旁，不持久化）
		DlgGeom::bind(this, "PropertyEditorPopup", DlgGeom::Mode::SizeOnly);
		setFixedWidth(280);   //宽度锁死280（放在 bind 后：覆盖已持久化的旧宽度）
	}

	void mPropertyEditorPopup::buildUi()
	{
		setAutoFillBackground(true);

		QVBoxLayout* outer = new QVBoxLayout(this);
		outer->setContentsMargins(1, 1, 1, 1);
		outer->setSpacing(0);

		//窗口细边框：白底标题栏与内容区同为浅色，加描边保证弹窗轮廓可辨识。
		//须用 objectName 选择器：本类在 PostCurve 命名空间内，QSS 裸类名选择器
		//按 metaObject className（PostCurve::mPropertyEditorPopup）匹配不上，规则
		//整体失效，底部 QSizeGrip 行等未覆盖区域露出调色板灰底（mApplyStyleDlg 同修）
		setObjectName("propEditorPopup");
		setAttribute(Qt::WA_StyledBackground, true);
		setStyleSheet("QWidget#propEditorPopup{background:#FFFFFF;border:1px solid #D9DCE0;}");

		//标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮（设计图03.1）
		_titleBar = new QWidget(this);
		_titleBar->setObjectName("propEditorTitleBar");
		_titleBar->setFixedHeight(36);
		_titleBar->setStyleSheet(R"(
            QWidget#propEditorTitleBar{
                background:#FFFFFF;
            }
        )");
		QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
		titleLayout->setContentsMargins(10, 0, 6, 0);
		titleLayout->setSpacing(3);
		QLabel* logoLabel = new QLabel(_titleBar);
		logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
			.scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		QLabel* titleLabel = new QLabel(QStringLiteral("属性编辑器"), _titleBar);
		titleLabel->setStyleSheet(
			"QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");
		QToolButton* closeBtn = new QToolButton(_titleBar);
		closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
		closeBtn->setIconSize(QSize(24, 24));
		closeBtn->setToolTip(QStringLiteral("关闭"));
		closeBtn->setStyleSheet(R"(
            QToolButton{
                background:transparent; border:none;
            }
            QToolButton:hover{ background:#EFF1F2; border-radius:2px; }
        )");
		//close() 而非 hide()：close 走关闭语义——弹窗是最后可见窗口时触发
		//lastWindowClosed -> 事件循环退出；hide 不触发，进程会滞留不退出
		connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
		titleLayout->addWidget(logoLabel);
		titleLayout->addWidget(titleLabel, 1);
		titleLayout->addWidget(closeBtn);

		//内容区
		QWidget* body = new QWidget(this);
		body->setStyleSheet("QWidget{background:white;}");
		QVBoxLayout* bodyOuter = new QVBoxLayout(body);
		bodyOuter->setContentsMargins(0, 0, 0, 0);
		bodyOuter->setSpacing(0);

		_bodyLayout = new QVBoxLayout();
		_bodyLayout->setContentsMargins(0, 0, 0, 0);
		_bodyLayout->setSpacing(0);
		bodyOuter->addLayout(_bodyLayout, 1);

		outer->addWidget(_titleBar);
		outer->addWidget(body, 1);

		//右下角缩放手柄：不占布局行（单独成行会在底部多出一条手柄高度的
		//空白，对齐 mApplyStyleDlg），改为覆盖子件贴角，resizeEvent 跟随
		_grip = new QSizeGrip(this);
		_grip->setFixedSize(_grip->sizeHint());
	}

	void mPropertyEditorPopup::host(mCurvePropPageBase* page)
	{
		//移除旧页面（复用的页面即将重新挂载，不能删）
		QLayoutItem* item = nullptr;
		while ((item = _bodyLayout->takeAt(0)) != nullptr)
		{
			if (item->widget() && item->widget() != page)
				delete item->widget();
			delete item;
		}
		if (page)
		{
			page->setParent(this);
			_bodyLayout->addWidget(page);
		}
	}

	void mPropertyEditorPopup::mousePressEvent(QMouseEvent* event)
	{
		//仅记录按下点：拖动延迟到 mouseMoveEvent 中移动超过系统拖动阈值后才
		//真正开始（双击微动不再挪动弹窗，见 mouseDoubleClickEvent）
		if (event->button() == Qt::LeftButton && _titleBar->underMouse())
			_pressPos = event->globalPos();
		QWidget::mousePressEvent(event);
	}

	void mPropertyEditorPopup::mouseMoveEvent(QMouseEvent* event)
	{
		//只有用户拖动才改变位置：左键按住且自按下点（双击则自双击点）移动超过
		//系统拖动阈值才进入拖动并固定偏移，此后跟随手部移动
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
		QWidget::mouseMoveEvent(event);
	}

	void mPropertyEditorPopup::mouseReleaseEvent(QMouseEvent* event)
	{
		_dragging = false;   //松手结束拖动（双击的第二击不延续上一次拖动态）
		QWidget::mouseReleaseEvent(event);
	}

	void mPropertyEditorPopup::mouseDoubleClickEvent(QMouseEvent* event)
	{
		//双击不进入拖动链路：默认实现会转发 mousePressEvent 重新置位拖动状态，
		//配合手部微动会挪动弹窗；此处仅刷新阈值起算点，属性编辑器双击无其他语义
		if (event->button() == Qt::LeftButton)
			_pressPos = event->globalPos();
	}

	void mPropertyEditorPopup::resizeEvent(QResizeEvent* event)
	{
		if (_grip)
			_grip->move(width() - _grip->width(), height() - _grip->height());
		QWidget::resizeEvent(event);
	}
}
