#include "mAppTranslators.h"

//MxFunctions
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include <QCoreApplication>

namespace MxMech
{
	const static QStringList TranslationFiles =
	{
		":/mMainWidgets/Translations/MeshPanel.qm"
	};

	mAppTranslators::mAppTranslators()
	{
		_translators.clear();
		const int tcount = TranslationFiles.size();
		for (int i = 0; i < tcount; ++i)
		{
			_translators.append(new QTranslator);
		}

		//连接信号与槽
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));

		//初始化
		this->slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
	}

	mAppTranslators::~mAppTranslators()
	{
		const int tcount = _translators.size();
		for (int i = 0; i < tcount; ++i)
		{
			if (_translators.at(i) != nullptr)
			{
				delete _translators.at(i);
			}
		}
		_translators.clear();
	}

	bool mAppTranslators::translateToChinese()
	{
		const int count1 = _translators.size();
		const int count2 = TranslationFiles.size();
		if (count1 != count2)
		{
			return false;
		}

		for (int i = 0; i < count1; ++i)
		{
			QTranslator* tepTran = _translators.at(i);
			qApp->removeTranslator(tepTran);
			const QString tranFile = TranslationFiles.at(i);
			if (!tepTran->load(tranFile))
			{
				return false;
			}
			qApp->installTranslator(tepTran);
		}
		return true;
	}

	bool mAppTranslators::translateToEnglish()
	{
		const int count1 = _translators.size();
		const int count2 = TranslationFiles.size();
		if (count1 != count2)
		{
			return false;
		}

		for (int i = 0; i < count1; ++i)
		{
			QTranslator* tepTran = _translators.at(i);
			qApp->removeTranslator(tepTran);
		}
		return true;
	}

	void mAppTranslators::slot_changeLanguage(int lan)
	{
		//存储配置
		MAppConfiguration::mAppConfiguration::getInstance()->_language = lan;

		//加载/卸载翻译文件
		if (lan == 0)
		{
			this->translateToChinese();
		}
		else if (lan == 1)
		{
			this->translateToEnglish();
		}
		else
		{
			return;
		}

		//更新界面
		MxFunctions::mGlobalSignals::getInstance()->updateWidgetLanguages();
	}
}