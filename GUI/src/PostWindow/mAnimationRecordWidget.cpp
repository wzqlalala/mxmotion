#include "mAnimationRecordWidget.h"

#include <QScreen>
#include <QPixmap>
#include <QDesktopWidget>
#include <QPainter>
#include <QApplication>
#include "mAppConfiguration.h"

//解决中文乱码
#pragma execution_character_set("utf-8")


namespace PostWindow
{
	void mAnimationRecordWidget::getStartAndEndPos(ScreenType1 type, QWidget* parent)
	{
		if (screenWidget_ != nullptr)
		{
			screenWidget_->startScreen(type);
		}
	}

	void mAnimationRecordWidget::closeScreenWidget()
	{
		if (screenWidget_ != nullptr)
			screenWidget_->close();
	}

	void mAnimationRecordWidget::setStart(QPoint pos)
	{
		screenWidget_->setStart(pos);
	}

	void mAnimationRecordWidget::setEnd(QPoint pos)
	{
		screenWidget_->setEnd(pos);
	}

	void mAnimationRecordWidget::cancelBtn()
	{
		screenWidget_->slot_cancelButton();
	}

	mAnimationRecordWidget::mAnimationRecordWidget(QSize size, QPoint point, RecordType type, QWidget* parent)
		: QWidget(parent)
	{
		screenWidget_ = new mScreenWidget(size, point, type, parent);
		connect(screenWidget_, SIGNAL(sig_sendPos(QPoint, QPoint)), this, SIGNAL(sig_sendPos(QPoint, QPoint)));
		connect(screenWidget_, SIGNAL(sig_saveShot()), this, SIGNAL(sig_saveShot()));
		connect(screenWidget_, SIGNAL(sig_saveVideo()), this, SIGNAL(sig_saveVideo()));
	}

	mAnimationRecordWidget::~mAnimationRecordWidget()
	{}

	mScreen::mScreen(QSize size)
	{
		maxWidth_ = size.width();
		maxHeight_ = size.height();

		startPos_ = QPoint(-1, -1);
		endPos_ = startPos_;
		leftUpPos_ = startPos_;
		rightDownPos_ = startPos_;
		status_ = SELECT;
	}

	int mScreen::width()
	{
		return maxWidth_;
	}

	int mScreen::height()
	{
		return maxHeight_;
	}

	mScreen::STATUS mScreen::getStatus()
	{
		return status_;
	}

	void mScreen::setStatus(STATUS status)
	{
		this->status_ = status;
	}

	void mScreen::setEnd(QPoint pos)
	{
		endPos_ = pos;
		leftUpPos_ = startPos_;
		rightDownPos_ = endPos_;
		cmpPoint(leftUpPos_, rightDownPos_);
	}

	void mScreen::setStart(QPoint pos)
	{
		startPos_ = pos;
	}

	QPoint mScreen::getEnd()
	{
		return endPos_;
	}

	QPoint mScreen::getStart()
	{
		return startPos_;
	}

	QPoint mScreen::getLeftUp()
	{
		return leftUpPos_;
	}

	QPoint mScreen::getRightDown()
	{
		return rightDownPos_;
	}

	bool mScreen::isInArea(QPoint pos)
	{
		if (pos.x() > leftUpPos_.x() && pos.x() < rightDownPos_.x() && pos.y() > leftUpPos_.y() && pos.y() < rightDownPos_.y())
			return true;
		return false;
	}

	HitRegion mScreen::hitTest(const QPoint& pos)
	{
		if (leftUpPos_.x() < 0 || rightDownPos_.x() < 0)
			return None;

		int x = pos.x();
		int y = pos.y();

		int l = leftUpPos_.x();
		int r = rightDownPos_.x();
		int t = leftUpPos_.y();
		int b = rightDownPos_.y();

		bool nearLeft = qAbs(x - l) <= RESIZE_MARGIN;
		bool nearRight = qAbs(x - r) <= RESIZE_MARGIN;
		bool nearTop = qAbs(y - t) <= RESIZE_MARGIN;
		bool nearBottom = qAbs(y - b) <= RESIZE_MARGIN;

		if (nearLeft && nearTop)     return TopLeft;
		if (nearRight && nearTop)    return TopRight;
		if (nearLeft && nearBottom)  return BottomLeft;
		if (nearRight && nearBottom) return BottomRight;

		if (nearLeft)   return Left;
		if (nearRight)  return Right;
		if (nearTop)    return Top;
		if (nearBottom) return Bottom;

		if (isInArea(pos))
			return Inside;

		return None;
	}

	void mScreen::resetPos()
	{
		QPoint pos = QPoint(-1, -1);
		endPos_ = pos;
		leftUpPos_ = pos;
		rightDownPos_ = pos;
		status_ = SELECT;
	}

	void mScreen::move(QPoint p)
	{
        int targetLeft = leftUpPos_.x() + p.x();
        int targetRight = rightDownPos_.x() + p.x();
        int targetTop = leftUpPos_.y() + p.y();
        int targetBottom = rightDownPos_.y() + p.y();

        // 检查 X 轴
        if (targetLeft < 0 || targetRight > maxWidth_) {
            // 如果移动会导致越界，则不允许移动 X 轴
            // 保持 left, right 不变
        }
        else {
            leftUpPos_.setX(targetLeft);
            rightDownPos_.setX(targetRight);
        }

        // 检查 Y 轴
        if (targetTop < 0 || targetBottom > maxHeight_) {
            // 保持 top, bottom 不变
        }
        else {
            leftUpPos_.setY(targetTop);
            rightDownPos_.setY(targetBottom);
        }

		startPos_ = leftUpPos_;
		endPos_ = rightDownPos_;
	}

	void mScreen::resizeLeft(int dx)
	{
		int newX = leftUpPos_.x() + dx;
		newX = qBound(0, newX, rightDownPos_.x() - MIN_RECT_SIZE);

		leftUpPos_.setX(newX);
		startPos_ = leftUpPos_;
	}


	void mScreen::resizeRight(int dx)
	{
		int newX = rightDownPos_.x() + dx;
		newX = qBound(leftUpPos_.x() + MIN_RECT_SIZE, newX, maxWidth_);

		rightDownPos_.setX(newX);
		endPos_ = rightDownPos_;
	}


	void mScreen::resizeTop(int dy)
	{
		int newY = leftUpPos_.y() + dy;
		newY = qBound(0, newY, rightDownPos_.y() - MIN_RECT_SIZE);

		leftUpPos_.setY(newY);
		startPos_ = leftUpPos_;
	}


	void mScreen::resizeBottom(int dy)
	{
		int newY = rightDownPos_.y() + dy;
		newY = qBound(leftUpPos_.y() + MIN_RECT_SIZE, newY, maxHeight_);

		rightDownPos_.setY(newY);
		endPos_ = rightDownPos_;
	}

	void mScreen::cmpPoint(QPoint& leftTop, QPoint& rightDown)
	{
		QPoint l = leftTop;
		QPoint r = rightDown;

		if (l.x() <= r.x())
		{
			if (l.y() <= r.y())
			{
			}
			else
			{
				leftTop.setY(r.y());
				rightDown.setY(l.y());
			}
		}
		else
		{
			if (l.y() < r.y())
			{
				leftTop.setX(r.x());
				rightDown.setX(l.x());
			}
			else
			{
				QPoint tmp;
				tmp = leftTop;
				leftTop = rightDown;
				rightDown = tmp;
			}
		}
	}

	mScreenWidget::mScreenWidget(QSize size, QPoint point, RecordType type, QWidget* parent)
		:QWidget(parent)
	{
		setMouseTracking(true);
		setAttribute(Qt::WA_MouseTracking);
		setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);//窗口无边框，窗口置顶，窗口任务栏隐藏
		setFocusPolicy(Qt::StrongFocus);
		//取得屏幕大小
		if (screen_ != nullptr)
		{
			delete screen_;
			screen_ = nullptr;
		}
		screen_ = new mScreen(size);
		fullScreen_ = new QPixmap();
		screen_->setStart(point);
		_bgPos = point;
		_defaultStart = QPoint(0,0);
		_defaultEnd = QPoint(size.width(), size.height());
		_type = type;

		_btnBar = new ConfirmCancelBar(this);
		_btnBar->hide();
		connect(_btnBar->btnConfirm, &QPushButton::clicked, this, &mScreenWidget::slot_saveButton);
		connect(_btnBar->btnCancel, &QPushButton::clicked, this, &mScreenWidget::slot_cancelButton);

	}

	void mScreenWidget::startScreen(ScreenType1 screenType, QRect rect)
	{
		if (screen_ == nullptr)
			return;
		QScreen* pscreen = QApplication::primaryScreen();
		if (pscreen == nullptr)return;
		int width = screen_->width();
		int height = screen_->height();
		int x = screen_->getStart().x();
		int y = screen_->getStart().y();
		int id = QApplication::desktop()->winId();
		*fullScreen_ = pscreen->grabWindow(id, x, y,width ,height );
		screenType_ = screenType;
		this->showFullScreen();
		this->setGeometry(x, y, width, height);
	}

	void mScreenWidget::setStart(QPoint pos)
	{
		screen_->setStart(pos);
		this->update();

	}

	void mScreenWidget::setEnd(QPoint pos)
	{
		screen_->setEnd(pos);
		this->update();

	}

	void mScreenWidget::updateCursor(HitRegion region)
	{
		switch (region)
		{
		case Inside:
			setCursor(Qt::SizeAllCursor);
			break;
		case Left:
		case Right:
			setCursor(Qt::SizeHorCursor);
			break;
		case Top:
		case Bottom:
			setCursor(Qt::SizeVerCursor);
			break;
		case TopLeft:
		case BottomRight:
			setCursor(Qt::SizeFDiagCursor);
			break;
		case TopRight:
		case BottomLeft:
			setCursor(Qt::SizeBDiagCursor);
			break;
		default:
			setCursor(Qt::CrossCursor); // 新建选择
			break;
		}
	}

	void mScreenWidget::mouseMoveEvent(QMouseEvent* e)
	{
		QPoint pos = e->pos();

		if (!(e->buttons() & Qt::LeftButton))
		{
			HitRegion region = screen_->hitTest(pos);
			updateCursor(region);
			return;
		}

		QPoint delta = e->pos() - _pressPos;
		_pressPos = e->pos();

		switch (_activeRegion)
		{
		case Inside:
			screen_->move(delta);
			break;

		case Left:
			screen_->resizeLeft(delta.x());
			break;

		case Right:
			screen_->resizeRight(delta.x());
			break;

		case Top:
			screen_->resizeTop(delta.y());
			break;

		case Bottom:
			screen_->resizeBottom(delta.y());
			break;

		case TopLeft:
			screen_->resizeLeft(delta.x());
			screen_->resizeTop(delta.y());
			break;

		case TopRight:
			screen_->resizeRight(delta.x());
			screen_->resizeTop(delta.y());
			break;

		case BottomLeft:
			screen_->resizeLeft(delta.x());
			screen_->resizeBottom(delta.y());
			break;

		case BottomRight:
			screen_->resizeRight(delta.x());
			screen_->resizeBottom(delta.y());
			break;

		default:
			screen_->setEnd(e->pos());
			break;
		}

		update();
		emit sig_sendPos(screen_->getStart(), screen_->getEnd());
		this->update();
	}

	void mScreenWidget::mousePressEvent(QMouseEvent* e)
	{
		if (e->button() != Qt::LeftButton)
			return;

		_pressPos = e->pos();
		_activeRegion = screen_->hitTest(_pressPos);

		if (_activeRegion == None)
		{
			// 重新选择
			screen_->setStart(_pressPos);
			screen_->setEnd(_pressPos);
		}
	}

	void mScreenWidget::mouseReleaseEvent(QMouseEvent*)
	{
		if (screen_->getStatus() == mScreen::SELECT) {
			screen_->setStatus(mScreen::MOV);
		}
		else if (screen_->getStatus() == mScreen::MOV) {
			this->setCursor(Qt::ArrowCursor);
		}
	}

	void mScreenWidget::paintEvent(QPaintEvent*)
	{
		int x = screen_->getLeftUp().x();
		int y = screen_->getLeftUp().y();
		int w = screen_->getRightDown().x() - x;
		int h = screen_->getRightDown().y() - y;

		QPainter painter(this);

		QPen pen;
		pen.setColor(QColor(19,133,246));
		pen.setWidth(2);
		pen.setStyle(Qt::SolidLine);
		painter.setPen(pen);
		int width = bgScreen_->width();
		int wid = fullScreen_->width();
		painter.drawPixmap(0, 0, *bgScreen_);//画模糊背景

		if (w != 0 && h != 0)
			painter.drawPixmap(x, y, fullScreen_->copy(x, y, w, h));//画图像
		painter.drawRect(x, y, w, h);//画边框
		pen.setColor(Qt::white);

		QFont font;
		font.setPointSize(14);
		painter.setFont(font);
		painter.setPen(pen);

		// ===== 绘制选择框 =====
		QPen pen1(QColor(0, 147, 250));
		pen1.setWidth(2);
		pen1.setStyle(Qt::SolidLine);
		painter.setPen(pen1);
		painter.drawRect(x, y, w, h);

		// ===== 文字内容 =====
		QString text = tr("( %1, %2 ) - ( %3, %4 )  ( %5 x %6 )")
			.arg(x).arg(y)
			.arg(x + w).arg(y + h)
			.arg(w).arg(h);
		QFontMetrics fm(painter.font());
		QRect textRect = fm.boundingRect(text);
		const int padding = 6;

		textRect.adjust(-padding, -padding, padding, padding);

		// ===== 文字位置策略 =====
		QPoint textPos;

		// 优先放在矩形上方
		if (y - textRect.height() - 4 >= 0)
		{
			textPos = QPoint(x, y - textRect.height() - 4);
		}
		else
		{
			textPos = QPoint(x + 4, y + 4);
		}
		textRect.moveTopLeft(textPos);
		if (w != 0 && h != 0)
		{
			// ===== 背景条 =====
			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(0, 0, 0, 160));
			painter.drawRoundedRect(textRect, 4, 4);

			// ===== 文字 =====
			painter.setPen(Qt::white);
			painter.drawText(textRect.adjusted(padding, padding, -padding, -padding),
				Qt::AlignLeft | Qt::AlignVCenter,
				text);
		}

		if (screen_->getRightDown().y() > 0 && screen_->getRightDown().x() >0)
		{
			int btnWidth = _btnBar->width();
			int btnHeight = _btnBar->height();

			int buttonY, buttonX;
			if (w < btnWidth)
			{
				buttonX = screen_->getLeftUp().x();
			}
			else
			{
				buttonX = screen_->getRightDown().x() - btnWidth;
			}

			if (fullScreen_->height() -(y + h) - (btnHeight + 4) >= 0)
			{
				buttonY = screen_->getRightDown().y() + 4;
			}
			else
			{
				buttonY = screen_->getRightDown().y() - 4 - btnHeight;
			}
			_btnBar->move(buttonX, buttonY);
			_btnBar->show();
		}
	}

	void mScreenWidget::showEvent(QShowEvent* e)
	{
		/*QPoint point(-1, -1);
		screen_->setStart(point);
		screen_->setEnd(point);*/
	
		//设置透明度实现模糊背景
		int width = screen_->width();
		int height = screen_->height();
		QPixmap pix(width, height);
		pix.fill((QColor(160, 160, 160, 200)));
		bgScreen_ = new QPixmap(*fullScreen_);
		QPainter p(bgScreen_);
		p.drawPixmap(0, 0, pix);
	}

	void mScreenWidget::keyPressEvent(QKeyEvent* event)
	{
		if (event->key() == Qt::Key_Escape)
		{
			slot_cancelButton();
		}
		else
		{
			QWidget::keyPressEvent(event);
		}
	}

	void mScreenWidget::slot_saveButton()
	{
		if (_type == RecordType::ShotRecord)
		{
			emit sig_saveShot();
		}
		else
		{
			emit sig_saveVideo();
		}
	}

	void mScreenWidget::slot_cancelButton()
	{
		_btnBar->hide();
		close();
		screen_->resetPos();
		screen_->setStart(_bgPos);
		startScreen(QuadScreen);
		emit sig_sendPos(_defaultStart, _defaultEnd);
	}

	ConfirmCancelBar::ConfirmCancelBar(QWidget* parent) : QWidget(parent)
	{
		btnConfirm = new QPushButton(this);
		btnCancel = new QPushButton(this);
		btnConfirm->setIcon(QIcon(":/mMainWidgets/Icon/post/dui.png"));
		btnCancel->setIcon(QIcon(":/mMainWidgets/Icon/post/cuo.png"));

		auto* layout = new QHBoxLayout(this);
		layout->setSpacing(0); 
		layout->setMargin(0);
		layout->addWidget(btnCancel);
		layout->addWidget(btnConfirm);

		setStyleSheet(R"(
		QPushButton {
			border: 1px solid #459EDA;
			background: #fff;
			min-width: 72px;
			min-height: 32px;
			font-size: 15px;
			color: #222;
		}
		QPushButton:first-child {
			border-top-left-radius: 7px;
			border-bottom-left-radius: 7px;
			border-right: none; /* 去掉中间线，仅分割 */
		}
		QPushButton:last-child {
			border-top-right-radius: 7px;
			border-bottom-right-radius: 7px;
		}
		QPushButton + QPushButton {
			border-left: 1px solid #d0d0d0;
		}
		QPushButton:hover {
			background: #e3f0fc;
		}
		QPushButton:pressed {
			background: #cde3fa;
		}
		)");

		setFixedHeight(34);
		setAttribute(Qt::WA_TranslucentBackground);
	}
}
