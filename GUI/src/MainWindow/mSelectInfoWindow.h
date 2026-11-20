#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mDockBase.h"
#include <QFrame>
#include <QSize>

#include <QTabWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QTableWidget>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <memory>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QDialog>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QMdiSubWindow>
namespace MechPreData
{
	class mViewportPrivate;
}
namespace MechMainWindow
{
	class CenterAlignDelegate : public QStyledItemDelegate {
	public:
		using QStyledItemDelegate::QStyledItemDelegate;
		void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override {
			QStyledItemDelegate::initStyleOption(option, index);
			option->displayAlignment = Qt::AlignCenter; // 设置居中
		}
	};
	class mSelectInfoWindow : public QWidget
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针
		*/
		static mSelectInfoWindow* getInstance();

		/*
		* 析构函数
		*/
		~mSelectInfoWindow();

	private:
		/*
		* 默认构造函数
		*/
		mSelectInfoWindow(QWidget* parent = 0);

		/*
		* 设置表的横向表头
		*/
		void setTableHorizontalHeader(QStringList header);

		/*
		* 设置表的纵向表头
		*/
		void setTableVerticalHeader(QStringList header);

		/*
		* 设置表的行数据
		*/
		void setTableRowQVector3DData(int row, int col, QVector3D data);

		//添加表格数据
		void addRow(QTableWidget* table, const QStringList& rowData);

	private slots:
		//切换语言
		void slot_changeLanguage(int language);

		/*
		* 高亮几何数据变化
		*/
		void slot_highlightGeoData();

		/*
		* 高亮网格数据变化
		*/
		void slot_highlightMeshData();

		/*
		* 高亮部件数据变化
		*/
		void slot_highlightPartData();

		/*
		* 高亮后处理数据
		*/
		void slot_highlightPostData();

		/*
		* 填充后处理数据
		*/
		void slot_setPostData(std::vector<int> ids, int type);

		/*
		* 右键功能
		*/
		void slot_contextMenuTableData(const QPoint& pos);

		/*
		* 切换视口
		*/
		void slot_subWindowActivated(int viewportID);

		/*
		* 单位值切换
		*/
		void slot_unitSystemChanged(int index);

	signals:

		/*
		* 更新状态栏数据
		*/
		void updateStatusBarSelectInfo(QString text= "");

	private:
		//单例指针
		static mSelectInfoWindow* _instance;

		QScrollArea* _scrollArea = nullptr;

		//单位制
		QString _length = "mm";//字符串
		double _scale = 1.0;//相对于mm的倍数

		//后处理复选框
		QWidget* _controlsWidget;

		//后处理复选框显示状态
		QMap<int, bool> _postCheckedState;

		//Table
		QTableWidget* _table;

		//右键菜单
		QAction* _copyAction = nullptr;
		QAction* _flushedAction = nullptr;
		QAction* _reselectAction = nullptr;

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;

		QMetaObject::Connection conn_post;

		//当前视口数据
		MechPreData::mViewportPrivate* _currentViewportPrivate;

	};
}
