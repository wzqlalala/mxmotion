#pragma once
#include "mCommon_global.h"
#pragma execution_character_set("utf-8")
//QT
#include <QStringList>
#include <QObject>

namespace MechCommon
{	
	extern COMMON_EXPORT QHash<QString, QString> CommonEnglish;
	extern COMMON_EXPORT QHash<QString, QString> CommonChinese;

	class COMMON_EXPORT mCommonLanguage : public QObject
	{
	public:
		
		static void initLanguageVarible();

		static QString FindLanguageValue(const QString & key);

	};

}