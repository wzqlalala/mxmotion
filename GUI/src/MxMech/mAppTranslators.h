#pragma once
#include <QObject>
#include <QTranslator>
#include <QList>

namespace MxMech
{
	class mAppTranslators : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 构造函数
		*/
		mAppTranslators();

		/*
		* 析构函数
		*/
		~mAppTranslators();

	public slots:
		/*
		* 中英文切换
		*/
		void slot_changeLanguage(int lan);

	private:
		/*
		* 翻译成中文
		*/
		bool translateToChinese();

		/*
		* 翻译成英文
		*/
		bool translateToEnglish();

	private:
		//翻译列表，每个翻译文件对应一个
		QList<QTranslator*> _translators;
	};
}
