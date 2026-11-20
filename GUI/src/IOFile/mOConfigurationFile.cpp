#include "mOConfigurationFile.h"

//Qt
#include <QFile>
#include <QFileInfoList>
#include <QApplication>
#include <QTextStream>
#include <QDir>

//MAppConfiguration
#include "mAppConfiguration.h"

namespace MechIOFile
{
	mOConfigurationFile::mOConfigurationFile()
	{

	}

	mOConfigurationFile::~mOConfigurationFile()
	{

	}

	void mOConfigurationFile::writeConfigureData()
	{
		QDir dir = QDir(qApp->applicationDirPath() + "/../ConfigFiles");
		if (!dir.exists())
		{
			//创建文件夹
			dir.mkdir(qApp->applicationDirPath() + "/../ConfigFiles");
		}
		QString defaultDir = QDir(qApp->applicationDirPath() + "/../ConfigFiles/mxcaefile.config").absolutePath();
		QFile file(defaultDir);
		if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly))//以写的方式打开文件
		{
			return;
		}
		QTextStream out(&file);
		//写最近打开的文件
		const QList<QString> &recentOpenMxCaeFileDir = MAppConfiguration::mAppConfiguration::getInstance()->getRecentMxCaeFiles();
		for (int i = 0;i< recentOpenMxCaeFileDir.size();++i)
		{
			file.write(QString("*MXRECENTFILE = %1\r\n").arg(recentOpenMxCaeFileDir.at(i)).toUtf8());
		}


		// 写许可方式
		QString licenseType = MAppConfiguration::mAppConfiguration::getInstance()->_licenseType;
		licenseType = QString("*MXLICENSETYPE = %1\r\n").arg(licenseType);
		file.write(licenseType.toUtf8());//转为uft-8写文件即可


		//写工作目录
		QString workingdir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
		workingdir = QString("*MXWORKINGDIR = %1\r\n").arg(workingdir);
		file.write(workingdir.toUtf8());//转为uft-8写文件即可

		QString recentDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
		recentDir = QString("*MXRECENTDIR = %1\r\n").arg(recentDir);
		file.write(recentDir.toUtf8());//转为uft-8写文件即可

		//写日志文件位置
		QString LogFileDir = MAppConfiguration::mAppConfiguration::getInstance()->_logFileDir;
		LogFileDir = QString("*MXLOGFILEDIR = %1\r\n").arg(LogFileDir);
		file.write(LogFileDir.toUtf8());//转为uft-8写文件即可

		//写单位制
		QString unit = MAppConfiguration::mAppConfiguration::getInstance()->_unitSystem;
		unit = QString("*MXUNIT = %1\r\n").arg(unit);
		file.write(unit.toUtf8());//转为uft-8写文件即可

		//写菜单栏风格
		bool menustyle = MAppConfiguration::mAppConfiguration::getInstance()->_isRibbon;
		QString stylestr = QString("*MXRIBBONMENU = %1\r\n").arg(QString::number(menustyle));
		file.write(stylestr.toUtf8());//转为uft-8写文件即可

		//写Mechanical
		bool hasmech = MAppConfiguration::mAppConfiguration::getInstance()->_hasMechanical;
		QString mechstr = QString("*MXSIMMECHANICAL = %1\r\n").arg(QString::number(hasmech));
		file.write(mechstr.toUtf8());//转为uft-8写文件即可

		//写Dyna
		bool hasdyna = MAppConfiguration::mAppConfiguration::getInstance()->_hasDyna;
		QString dynastr = QString("*MXSIMDYNA = %1\r\n").arg(QString::number(hasdyna));
		file.write(dynastr.toUtf8());//转为uft-8写文件即可

		//写CFD
		bool hascfd = MAppConfiguration::mAppConfiguration::getInstance()->_hasCFD;
		QString cfdstr = QString("*MXSIMCFD = %1\r\n").arg(QString::number(hascfd));
		file.write(cfdstr.toUtf8());//转为uft-8写文件即可

		//写Emag
		bool hasemag = MAppConfiguration::mAppConfiguration::getInstance()->_hasEmag;
		QString emagstr = QString("*MXSIMEMAG = %1\r\n").arg(QString::number(hasemag));
		file.write(emagstr.toUtf8());//转为uft-8写文件即可

		//写MultiPhysics
		bool hasmulti = MAppConfiguration::mAppConfiguration::getInstance()->_hasMultiPhysics;
		QString multistr = QString("*MXSIMMULTIPHYSICS = %1\r\n").arg(QString::number(hasmulti));
		file.write(multistr.toUtf8());//转为uft-8写文件即可

		//写Post
		bool haspost = MAppConfiguration::mAppConfiguration::getInstance()->_hasPost;
		QString poststr = QString("*MXSIMPOST = %1\r\n").arg(QString::number(haspost));
		file.write(poststr.toUtf8());//转为uft-8写文件即可

		//写快捷键
		QStringList funclist = MAppConfiguration::mAppConfiguration::getInstance()->getFunctionList();
		for (QString funcname : funclist)
		{
			QString shortkey = MAppConfiguration::mAppConfiguration::getInstance()->getShortcut(funcname);
			QString keystr = QString("*MXSHORTCUT = %1:%2\r\n").arg(funcname).arg(shortkey);
			file.write(keystr.toUtf8());//转为uft-8写文件即可
		}

		//写视图风格
		QString viewStyle = MAppConfiguration::mAppConfiguration::getInstance()->_viewStyle;
		unit = QString("*MXVIEWSTYLE = %1\r\n").arg(viewStyle);
		file.write(unit.toUtf8());//转为uft-8写文件即可

		//写中英文
		int Language = MAppConfiguration::mAppConfiguration::getInstance()->_language;
		unit = QString("*MXLANGUAGE = %1\r\n").arg(Language);
		file.write(unit.toUtf8());//转为uft-8写文件即可


		//写窗口固定
		int WidgetFix = MAppConfiguration::mAppConfiguration::getInstance()->_isWidgetFix;
		unit = QString("*MXWIDGETFIX = %1\r\n").arg(WidgetFix);
		file.write(unit.toUtf8());//转为uft-8写文件即可

		//写背景颜色
		QString pureColor = MAppConfiguration::mAppConfiguration::getInstance()->_pureColor;
		unit = QString("*MXPURECOLOR = %1\r\n").arg(pureColor);
		file.write(unit.toUtf8());//转为uft-8写文件即可

        //写软件名称
        QString softwareName = MAppConfiguration::mAppConfiguration::getInstance()->_softwareName;
        unit = QString("*MXSOFTWARENAME = %1\r\n").arg(softwareName);
        file.write(unit.toUtf8());//转为uft-8写文件即可

		//关闭文件
		file.close();
	}
}
