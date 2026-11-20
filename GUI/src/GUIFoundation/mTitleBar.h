#pragma once
#include "guifoundation_global.h"
//#ifndef MYDOCKTITLEBARH
//#define MYDOCKTITLEBARH

#include <QSize>
#include <QDockWidget>
#include <QMouseEvent>
#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

class GUIFOUNDATION_EXPORT TitleBarWidget :public QWidget
{
	Q_OBJECT

public:
	TitleBarWidget(QWidget* parent, QString titlecontent = "", int height = 30, int r = 219, int g = 229, int b = 238);
	//这里parent没有给默认值NULL，保证在创建TitleBarwidget对象时父指针必须得赋值;且赋值不为NULL;
	~TitleBarWidget();

	//设置标题栏背景色及是否设置标题栏背景色透明;
	void setBackgroundColor(int r, int g, int b, bool isTransparent = false);
	//设置标题栏图标:
	void setTitleIcon(QString filePath, QSize Iconsize = QSize(25, 25));
	//设置标题内容;
	void setTitleContent(QString titlecontent, int titleFontsize = 9);
	//设置标题栏高度;
	void setTitleHeight(int width);
	//设置标题栏中的标题是否会滚动 : 具体可以看效果 :
	void setTitleRoll();
	//设置窗口边框宽度;
	void setwindowBorderWidth(int borderwidth);

	//保存/获取 最大化前窗口的位置及大小 :
	void saveRestoreInfo(const QPoint point, const QSize size);
	void getRestoreInfo(QPoint& point, QSize& size);

private:
	void paintEvent(QPaintEvent* event);
	void mouseDoubleclickEvent(QMouseEvent* event);
	void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent * event);

	//初始化控件;
	void initControl();
	//信号槽的绑定;
	void initConnections();


private slots:
	//按钮触发的槽;
	void onButtonFloatingClicked();
	void onButtonMinClicked();
	void onButtonMaxClicked();
	void onButtonCloseClicked();
	void onRollTitle();

private:
	QLabel* m_pIcon;	// 标题栏图标;
	QLabel* m_pTitleContent;	//标题栏内容;
	QPushButton* m_pButtonMin;	// 最小化按钮;
	QPushButton* m_pButtonFloating;	//悬浮按钮
	QPushButton * m_pButtonMax;		//最大化按钮;
	QPushButton* m_pButtonClose;		//关闭按钮;

	//标题栏背景色,
	int m_colorR;
	int m_colorG;
	int m_colorB;

	//最大化，最小化变量;
	QPoint m_restorePos;
	QSize m_restoreSize;
	// 移动窗口的变量;
	bool m_isPressed;
	QPoint m_startMovePos;
	//标题栏跑马灯效果时钟;
	QTimer m_titleRollTimer;
	//标题栏内容;
	QString m_titlecontent;
	//窗口边框宽度;
	int m_windowBorderWidth;
	//标题栏是否透明;
	bool m_isTransParent;
	QWidget* m_widget;

	int titleheight;
};
//#endif // MYDOCKTITLEBAR H