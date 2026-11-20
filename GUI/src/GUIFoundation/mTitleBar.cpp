#include "mTitleBar.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QFile>
#include <QMouseEvent>
#include<QStyle>
#include<QDockWidget>
#include<QDesktopWidget>
#include<QApplication>
#include<QPainterPath>
#define BUTTON_HEIGHT 30		//按钮高度;
#define BUTTON_WIDTH 30			//按钮宽度;
#define TITLE_HEIGHT 30			//标题栏高度;

TitleBarWidget::TitleBarWidget(QWidget* parent, QString titlecontent, int height, int r, int g, int b)
	: QWidget(parent)
	, m_colorR(r)
	, m_colorG(g)
	, m_colorB(b)
	, m_isPressed(false)
	, m_windowBorderWidth(0)
	, m_isTransParent(false)
	, titleheight(height)
{
	//初始化
	initControl();
	initConnections();
	m_widget = parent;
	if (!titlecontent.isEmpty())
	{
		setTitleContent(titlecontent);
	}
}

		
TitleBarWidget::~TitleBarWidget()
{

}
//初始化控件
void TitleBarWidget::initControl()
{
	m_pIcon = new QLabel;
	m_pTitleContent = new QLabel;
	m_pIcon->setStyleSheet(QString("background-color: rgb(%1,%2,%3);border:none;").arg(m_colorR).arg(m_colorG).arg(m_colorB));
	m_pTitleContent->setStyleSheet(QString("background-color: rgb(%1,%2,%3);border:none;").arg(m_colorR).arg(m_colorG).arg(m_colorB));
	QIcon minIcon(style()->standardPixmap(QStyle::SP_TitleBarMinButton));
	m_pButtonMin = new QPushButton(minIcon, "", this);
	m_pButtonMin->setStyleSheet(QString("background-color: rgb(%1,%2,%3);border:none;").arg(m_colorR).arg(m_colorG).arg(m_colorB));
	QIcon floatIcon(style()->standardPixmap(QStyle::SP_TitleBarNormalButton));
	m_pButtonFloating = new QPushButton(QIcon(":/mMainWidgets/Icon/floating.png"), "", this);
	m_pButtonFloating->setStyleSheet(QString("background-color: rgb(%1,%2,%3);border:none;").arg(m_colorR).arg(m_colorG).arg(m_colorB));
	QIcon maxIcon(style()->standardPixmap(QStyle::SP_TitleBarMaxButton));
	m_pButtonMax = new QPushButton(maxIcon, "", this);
	m_pButtonMax->setStyleSheet(QString("background-color: rgb(%1,%2,%3);border:none;").arg(m_colorR).arg(m_colorG).arg(m_colorB));
	QIcon closeIcon(style()->standardPixmap(QStyle::SP_TitleBarCloseButton));
	m_pButtonClose = new QPushButton(QIcon(":/mMainWidgets/Icon/close.png"), "", this);
	m_pButtonClose->setStyleSheet(QString("background-color: rgb(%1,%2,%3);border:none;").arg(m_colorR).arg(m_colorG).arg(m_colorB));
	m_pButtonFloating->setFixedSize(QSize(titleheight, titleheight));
	m_pButtonMax->setFixedSize(QSize(titleheight, titleheight));
	m_pButtonMin->setFixedSize(QSize(titleheight, titleheight));
	m_pButtonClose->setFixedSize(QSize(titleheight, titleheight));
	m_pButtonFloating->setToolTip(QStringLiteral("浮动"));
	m_pButtonMin->setToolTip(QStringLiteral("停止浮动"));
	m_pButtonMax->setToolTip(QStringLiteral("最大化"));
	m_pButtonClose->setToolTip(QStringLiteral("关闭"));
	QHBoxLayout* mylayout = new QHBoxLayout(this);
	mylayout->addWidget(m_pIcon);
	mylayout->addWidget(m_pTitleContent);
	mylayout->addWidget(m_pButtonFloating);
	//mylayout->addWidget(m_pButtonMin);
	//mylayout->addWidget(m_pButtonMax);
	mylayout->addWidget(m_pButtonClose);
	mylayout->setContentsMargins(0, 0, 0, 0);
	mylayout->setSpacing(0);
	m_pTitleContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	this->setFixedHeight(titleheight);
	this->setWindowFlags(Qt::FramelessWindowHint);
}
//信号槽的绑定;
void TitleBarWidget::initConnections()
{
	connect(m_pButtonFloating, SIGNAL(clicked()), this, SLOT(onButtonFloatingClicked()));
	connect(m_pButtonMin, SIGNAL(clicked()), this, SLOT(onButtonMinClicked()));
	connect(m_pButtonMax, SIGNAL(clicked()), this, SLOT(onButtonMaxClicked()));
	connect(m_pButtonClose, SIGNAL(clicked()), this, SLOT(onButtonCloseClicked()));
}

//设置标题栏背景色, 在paintEvent事件中进行绘制标题栏背景色;
//在构造函数中给了默认值，可以外部设置颜色值改变标题栏背景色，
void TitleBarWidget::setBackgroundColor(int r, int g, int b, bool isTransParent)
{
	m_colorR = r;
	m_colorG = g;
	m_colorB = b;
	m_isTransParent = isTransParent;
	//重新绘制(调用paintEvent事件);
	update();
}

//设置标题栏图标;
void TitleBarWidget::setTitleIcon(QString filePath, QSize Iconsize)
{
	QPixmap titleIcon(filePath);
	m_pIcon->setPixmap(titleIcon.scaled(Iconsize));
}

//设置标题内容:
void TitleBarWidget::setTitleContent(QString titleContent, int titleFontsize)
{
	//设置标题字体大小;
	QFont font = m_pTitleContent->font();
	font.setPointSize(titleFontsize);
	m_pTitleContent->setFont(font);
	//设置标题内容;
	m_pTitleContent->setText(titleContent);
	m_titlecontent = titleContent;
}
//设置标题栏高度;
void TitleBarWidget::setTitleHeight(int height)
{
	this->setFixedHeight(height);
}

//设置标题栏中的标题是否会自动滚动，跑马灯的效果;
//一般情况下标题栏中的标题内容是不滚动的，但是既然自定义就看自己需要嘛，想怎么设计就怎么搞0(n n)0!
void TitleBarWidget::setTitleRoll()
{
	connect(&m_titleRollTimer, SIGNAL(timeout()), this, SLOT(onRollTitle()));
	m_titleRollTimer.start(200);
}

//设置窗口边框宽度 :
void TitleBarWidget::setwindowBorderWidth(int borderWidth)
{
	m_windowBorderWidth = borderWidth;
}
	
//保存窗口最大化前窗口的位置以及大小 :
void TitleBarWidget::saveRestoreInfo(const QPoint point, const QSize size)
{
	m_restorePos = point;
	m_restoreSize = size;
}
//获取窗口最大化前窗口的位置以及大小;
void TitleBarWidget::getRestoreInfo(QPoint& point, QSize& size)
{
	point = m_restorePos;
	size = m_restoreSize;
}
//绘制标题栏背景色
void TitleBarWidget::paintEvent(QPaintEvent* event)
{
	//是否设置标题透明;
	if (!m_isTransParent)
	{
		//设置背景色;
		QPainter painter(this);
		QPainterPath pathBack;
		pathBack.setFillRule(Qt::WindingFill);
		pathBack.addRoundedRect(QRect(0, 0, this->width(), this->height()), 3, 3);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.fillPath(pathBack, QBrush(QColor(m_colorR, m_colorG, m_colorB)));
	}

	// 当窗口最大化或者还原后，窗口长度变了，标题栏的长度应当一起改变;
	// 这里减去m_windowBorderwidth ，是因为窗口可能设置了不同宽度的边框;
	//如果窗口有边框则需要设置m_windowBorderwidth的值，否则m_windowBorderwidth默认为0
	if (this->width() != (m_widget->width() - m_windowBorderWidth))
	{
		this->setFixedWidth(m_widget->width() - m_windowBorderWidth);
	}
	QWidget::paintEvent(event);
}

//双击响应事件，主要是实现双击标题栏进行最大化和最小化操作;
void TitleBarWidget::mouseDoubleclickEvent(QMouseEvent* event)
{
	// 通过最大化按钮的状态判断当前窗口是处于最大化还是原始大小状态;
	//或者通过单独设置变量来表示当前窗口状态;
	if (m_pButtonMax->isVisible())
	{
		onButtonMaxClicked();
	}
	return QWidget::mouseDoubleClickEvent(event);
}
//以下通过mousePressEvent、mouseMoveEvent、mouseReleaseEvent三个事件实现了鼠标拖动标题栏移动窗口的效果;
void TitleBarWidget::mousePressEvent(QMouseEvent * event)
{
	m_isPressed = true;
	m_startMovePos = event->globalPos();
	return QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isPressed)
	{
		QPoint movePoint = event->globalPos() - m_startMovePos;
		QPoint widgetPos = m_widget->pos();
		m_startMovePos = event->globalPos();
		m_widget->resize(m_widget->width(), m_widget->height());
		//m_widget->setStyleSheet("background-color: #e7eef6;border:none;");
		m_widget->move(widgetPos.x() + movePoint.x(), widgetPos.y() + movePoint.y());
	}
	return QWidget::mouseMoveEvent(event);
}
void TitleBarWidget::mouseReleaseEvent(QMouseEvent* event)
{
	m_isPressed = false;
	return QWidget::mouseReleaseEvent(event);
}
//以下为按钮操作响应的槽;
void TitleBarWidget::onButtonFloatingClicked()
{
	m_pButtonMax->setVisible(true);
	QDockWidget* dw = qobject_cast<QDockWidget*>(m_widget);
	Q_ASSERT(dw != 0);
	if (!dw->isFloating()) //判断是否变为浮动窗口
	{
		//this->resize(600，400)://改变初始化大小
		dw->setFloating(true);
		QDesktopWidget* desk = QApplication::desktop();
		//dw->move((desk->width() - dw->width()) / 2, (desk->height() - dw->height()) / 2);
		dw->move((dw->width()) / 2, (dw->height()) / 2);
		dw->resize(m_widget->width(), m_widget->height());
		//dw->setStyleSheet("background-color: #e7eef6;border:none;");
		dw->show();
	}
}

void TitleBarWidget::onButtonMinClicked()
{
	QDockWidget* dw = qobject_cast<QDockWidget*>(m_widget);
	Q_ASSERT(dw != 0);
	if (dw->isFloating())//判断是否变为浮动窗口
	{
		dw->setFloating(false);//设置浮动标志为false
	}	
}
void TitleBarWidget::onButtonMaxClicked()
{
	m_pButtonMax->setVisible(false);
	QDockWidget* dw = qobject_cast<QDockWidget*>(m_widget);
	Q_ASSERT(dw != 0);
	dw->showMaximized();
}
void TitleBarWidget::onButtonCloseClicked()
{
	QDockWidget* dw = qobject_cast<QDockWidget*>(m_widget);
	Q_ASSERT(dw != 0);
	dw->close();
}

//该方法主要是让标题栏中的标题显示为滚动的效果，
void TitleBarWidget::onRollTitle()
{
	static int nPos = 0;
	QString titleContent = m_titlecontent;
	// 当截取的位置比字符串长时，从头开始;
	if (nPos > titleContent.length())
		nPos = 0;
	m_pTitleContent->setText(titleContent.mid(nPos));
	nPos++;
}