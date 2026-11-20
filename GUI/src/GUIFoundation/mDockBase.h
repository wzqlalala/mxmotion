#pragma once

#include "guifoundation_global.h"

#include <QDockWidget>

namespace MechMainWindow
{
	class GUIFOUNDATION_EXPORT mDockBase: public QDockWidget
	{
		Q_OBJECT
	public:
		/*
		* 默认构造函数
		*/
		mDockBase(QWidget* parent = 0);

		/*
		* 析构函数
		*/
		~mDockBase();

		/*
		* 设置窗口名称
		*/
		void setDockWindowTitle(const QString& title);

		/*
		* 获取窗口名称
		*/
		QString getDockWindowTitle();

		/*
		* 设置窗口位置
		*/
		void setDockWindowArea(Qt::DockWidgetAreas area);

		/*
		* 获取窗口位置
		*/
		Qt::DockWidgetAreas getDockWindowArea();

		/*
		* 设置窗口可见性
		*/
		void setDockWindowVisible(bool visi);

		/*
		* 获取窗口可见性
		*/
		bool getDockWindowVisible();

	private:
		//窗口名称
		QString _windowTitle;

		//窗口位置
		Qt::DockWidgetAreas _windowArea;

		//窗口可见性
		bool _windowVisible;
	};
}