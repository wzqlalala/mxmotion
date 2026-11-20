#pragma once

#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mDockBase.h"
#include "mmainwindow_global.h"

//QT
#include <QTabWidget>
#include <QStackedWidget>
#include <QAction>
#include <QToolBar>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QEvent>
#include "mCalcLineEdit.h" // 包含计算编辑框的头文件

using namespace MxFunctions;

namespace MechMainWindow
{
	class mBrowserTreeBase;
	class mBrowserTree;
    class mTreeView;
    class MMAINWINDOW_EXPORT mBrowserWindow : public QWidget
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针
		*/
		static mBrowserWindow* getInstance();

		/*
		* 析构函数
		*/
		~mBrowserWindow();
		/*
		* 获取前处理模型树
		*/
        mTreeView* getBrowserTree();

		/*
		* 树窗口工具条（过滤/排序）按当前 UiMetrics/UiPalette 重发（切档/切主题时调用，幂等）。
		* 构造期调用一次，ThemeManager::applyAll 遍历树时经 invokeMethod 再调。
		*/
		Q_INVOKABLE void refreshToolBarTheme();

	private slots:
		//切换语言
		void slot_changeLanguage(int language);

	private:
		/*
		* 默认构造函数
		*/
		mBrowserWindow(QWidget* parent = 0);

	signals:


	private:
		//单例指针
		static mBrowserWindow* _instance;
		//前处理模型树
        mTreeView* _treeView;

		QToolBar* _sortTool = nullptr;   // 排序工具条（图标/条高随档位重发）
		QLabel* _filterLabel = nullptr;
		QLabel* _sortLabel = nullptr;

		QComboBox* _nameComboBox = nullptr;
		QStackedWidget* _stackedWidget = nullptr;
		QLineEdit* _findLineEdit = nullptr;
		QComboBox* _statusComboBox = nullptr;

		QComboBox* _sortTypeComboBox = nullptr;
	};
}