#pragma once
#include <QSplashScreen>

namespace MxMech
{
	class mStartPage : public QSplashScreen
	{
		Q_OBJECT
	public:
		/*
		* 默认构造函数
		*/
		mStartPage();

		/*
		* 析构函数
		*/
		~mStartPage();
	};
}