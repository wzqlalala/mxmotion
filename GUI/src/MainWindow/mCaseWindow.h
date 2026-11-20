#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QTreeWidget>
#include <QToolBar>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWebEngineView>

#include "mDockBase.h"
#include "mModelStruct.h"
class HttpServer;
namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mCaseWindow : public QWidget
	{
		Q_OBJECT

	public:

		static mCaseWindow* getInstance();


		~mCaseWindow();

		QWebEngineView* getView();

		/*
		* 检测 QtWebEngineProcess.exe 是否存在
		*/
		static bool checkQtWebEngineProcessExists();


	private:
		mCaseWindow(QWidget* parent = Q_NULLPTR);

		QWebEngineView* view = nullptr;
	
	protected:
		static mCaseWindow* _instance;
		
	};
}