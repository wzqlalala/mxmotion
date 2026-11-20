#pragma once
#pragma execution_character_set("utf-8")
#include "guifoundation_global.h"

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEvent>
#include <QHoverEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QLineEdit>


QT_BEGIN_NAMESPACE
namespace Ui { class mCustomTooltipClass; }
QT_END_NAMESPACE

//AISolverBtn->installEventFilter(new TooltipInstaller("AI快速预测",
//	QPixmap(":/mMainWidgets/Icon/AIPredict.png"),
//	"选择计算文件和模型后\n进行快速预测", 500, AISolverBtn));

namespace MechMainWindow
{
	class GUIFOUNDATION_EXPORT mCustomTooltip : public QWidget
	{
		Q_OBJECT
		Q_PROPERTY(float opacity READ opacity WRITE setOpacity)

	public:	
		/*
		* 获取单例
		*/
		static mCustomTooltip* getInstance();

		/*
		* 显示提示
		*/
		void showTip(const QString& title,
			const QPixmap& icon,
			const QString& body, 
			const QPoint& pos = QCursor::pos(),
			QWidget* anchor = nullptr);

		/*
		* 隐藏提示
		*/
		void hideTip();

		/*
		* 按当前主题（UiPalette）刷新气泡底色/边框/文字色。
		* showTip 每次弹出时调用，主题切换后下一次弹出即生效
		*/
		void refreshTheme();

	protected:
		/*
		* 事件过滤
		*/
		bool eventFilter(QObject*, QEvent*) override;
		void paintEvent(QPaintEvent*) override;
		void leaveEvent(QEvent*) override;

	private:
		/*
		* 构造函数
		*/
		explicit mCustomTooltip(QWidget* parent = nullptr);

		/*
		* 开始淡出
		*/
		void startFadeOut();
		
		/*
		* 不透明度
		*/
		float opacity() const;
		
		/*
		* 设置不透明度
		*/
		void  setOpacity(float op); 
	private:
		Ui::mCustomTooltipClass* ui;
		static mCustomTooltip* _instance;

		QPropertyAnimation* m_fade;
		QWidget* m_anchor = nullptr;
		bool m_mouseInside = false;
	};

	class GUIFOUNDATION_EXPORT TooltipInstaller : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 安装监听
		*/
		explicit TooltipInstaller(
			const QString& title,
			const QPixmap& icon,
			const QString& body,int delayMs = 450, QObject* parent = nullptr);

	protected:
		/*
		* 事件过滤
		*/
		bool eventFilter(QObject* watched, QEvent* event) override;

	private slots:
		/*
		* 显示
		*/
		void slotShow();

	private:
		int m_delay;
		QTimer m_timer;
		QObject* m_target = nullptr;

		QString _title = "";
		QPixmap _icon;
		QString _body = "";
		QPoint _curpos;
	};

	class GUIFOUNDATION_EXPORT ToolbarTipInstaller : public QObject
	{
		Q_OBJECT

	public:
		/*
		* 安装监听
		*/
		explicit ToolbarTipInstaller(QToolBar* toolbar, int delayMs = 450, QObject* parent = nullptr);

	protected:
		/*
		* 重写事件过滤
		*/
		bool eventFilter(QObject* watched, QEvent* event) override;

	private slots:
		/*
		* 显示
		*/
		void slotShow();

	private:
		/*
		* 重置计时器
		*/
		void restartTimer();

	private:
		QToolBar* m_toolBar;
		QTimer    m_timer;
		QAction* m_pendingAction = nullptr;
		QPoint    m_lastPos;
	};

	class GUIFOUNDATION_EXPORT MenuTipFilter : public QObject
	{
		Q_OBJECT
	public:
		explicit MenuTipFilter(QMenu* menu, int delayMs = 450, QObject* parent = nullptr);

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override;

	private slots:
		void slotShowTip();

	private:
		void restartTimer();

		QMenu* m_menu;
		QTimer m_timer;
		QAction* m_pendingAction = nullptr;
		QPoint _curpos;
	};

	class GUIFOUNDATION_EXPORT InputIDLineEditTipInstaller : public QObject
	{
		Q_OBJECT

	public:
		/*
		 * 安装监听
		 */
		explicit InputIDLineEditTipInstaller(QLineEdit* lineEdit, int delayMs = 450, QObject* parent = nullptr);

	protected:
		/*
		 * 重写事件过滤
		 */
		bool eventFilter(QObject* watched, QEvent* event) override;

	private slots:
		/*
		 * 显示提示
		 */
		void slotShow();

	private:
		/*
		 * 重置计时器
		 */
		void restartTimer();

	private:
		QLineEdit* m_lineEdit;
		QTimer     m_timer;
		int m_delay;
		QLineEdit* m_pendingLineEdit = nullptr;
		QPoint     m_lastPos;
	};

}
