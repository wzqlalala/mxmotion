#include "mIConfigurationFile.h"

//Qt
#include <QFile>
#include <QFileInfoList>
#include <QApplication>
#include <QDir>
#include <QStorageInfo>
#include <QSettings>
#include <QStandardPaths>

//MAppConfiguration
#include "mAppConfiguration.h"

namespace MechIOFile
{
	mIConfigurationFile::mIConfigurationFile()
	{

	}

	mIConfigurationFile::~mIConfigurationFile()
	{

	}

	void mIConfigurationFile::readConfigureData()
	{
		//工作目录
		auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
		QSettings Settings(path, QSettings::IniFormat);
		Settings.beginGroup("WorkingDir");
		auto map = Settings.childKeys();
		QStringList keys;
		QStringList values;
		if (Settings.childKeys().isEmpty())
		{
			auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
			QSettings Settings(path, QSettings::IniFormat);
			// 添加一条“最近打开”
			QString value1 = QCoreApplication::applicationDirPath() + "/../WorkingDir";
			QString key1 = "WorkingDir/WorkingDir";
			Settings.setValue(key1, value1);
		}
		for (const QString& k : Settings.childKeys()) {
			MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = Settings.value(k).toString();
		}
		Settings.endGroup();
		//
		QString defaultDir = QDir(qApp->applicationDirPath() + "/../ConfigFiles/mxcaefile.config").absolutePath();
		QFile file(defaultDir);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))//以读的方式打开文件
		{
			ensureDefaultWorkingDirs();
			return;
		}
		QString currentline;
		QStringList data;
		QList<QString> recentOpenMxCaeFileDir;//最近打开的工程文件
		while (!file.atEnd())
		{
			currentline = file.readLine().simplified();
			////最近打开的工程文件
			if (currentline.contains("*MXRECENTFILE"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					recentOpenMxCaeFileDir.append(data.last().simplified());
				}
			}
			////工作目录
			else if (currentline.contains("*MXWORKINGDIR"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_workingDir = data.last().simplified();
					QFileInfo fileInfo(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
					if (!fileInfo.exists())//创建
					{
						QDir folder(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
						QString rootPath = folder.rootPath();
						// 判断指定磁盘是否存在
						QStorageInfo storage(rootPath);
						if (!storage.isValid() || !storage.isReady()) //不存在则创建
						{
							MAppConfiguration::mAppConfiguration::getInstance()->_workingDir = qApp->applicationDirPath() + "/WorkingDir/";
							QDir dir(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
							if (!dir.exists())//不存在则创建
							{
								bool ok = dir.mkdir(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);//只创建一级子目录，即必须保证上级目录存在
							}
						}
						else
						{
							bool ok = folder.mkdir(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);//只创建一级子目录，即必须保证上级目录存在
						}
					}
					else//判断是否是文件夹
					{
						if (!fileInfo.isDir())//是文件则去掉文件名
						{
							MAppConfiguration::mAppConfiguration::getInstance()->_workingDir = fileInfo.absolutePath();
						}
					}
				}
			}
			////日志文件位置
			else if (currentline.contains("*MXLOGFILEDIR"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_logFileDir = data.last().simplified();
					QFileInfo fileInfo(MAppConfiguration::mAppConfiguration::getInstance()->_logFileDir);
					if (!fileInfo.exists())//创建
					{
						QDir folder(MAppConfiguration::mAppConfiguration::getInstance()->_logFileDir);
						QString rootPath = folder.rootPath();
						// 判断指定磁盘是否存在
						QStorageInfo storage(rootPath);
						if (!storage.isValid() || !storage.isReady()) //不存在则创建
						{
							QDir dir(MAppConfiguration::mAppConfiguration::getInstance()->_logFileDir);
							if (!dir.exists())//不存在则创建
							{
								bool ok = dir.mkdir(MAppConfiguration::mAppConfiguration::getInstance()->_logFileDir);//只创建一级子目录，即必须保证上级目录存在
							}
						}
						else
						{
							bool ok = folder.mkdir(MAppConfiguration::mAppConfiguration::getInstance()->_logFileDir);//只创建一级子目录，即必须保证上级目录存在
						}
					}
					else//判断是否是文件夹
					{
						if (!fileInfo.isDir())//是文件则去掉文件名
						{
							MAppConfiguration::mAppConfiguration::getInstance()->_logFileDir = fileInfo.absolutePath();
						}
					}

				}
			}
			////最近工作目录
			else if (currentline.contains("*MXRECENTDIR"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = data.last().simplified();
					QFileInfo fileInfo(MAppConfiguration::mAppConfiguration::getInstance()->_recentDir);
					if (!fileInfo.exists())//创建
					{
						QDir folder(MAppConfiguration::mAppConfiguration::getInstance()->_recentDir);
						QString rootPath = folder.rootPath();
						// 判断指定磁盘是否存在
						QStorageInfo storage(rootPath);
						if (!storage.isValid() || !storage.isReady()) //不存在则创建
						{
							MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = qApp->applicationDirPath() + "/WorkingDir/";
							QDir dir(MAppConfiguration::mAppConfiguration::getInstance()->_recentDir);
							if (!dir.exists())//不存在则创建
							{
								bool ok = dir.mkdir(MAppConfiguration::mAppConfiguration::getInstance()->_recentDir);//只创建一级子目录，即必须保证上级目录存在
							}
						}
						else
						{
							bool ok = folder.mkdir(MAppConfiguration::mAppConfiguration::getInstance()->_recentDir);//只创建一级子目录，即必须保证上级目录存在
						}
					}
					else//判断是否是文件夹
					{
						if (!fileInfo.isDir())//是文件则去掉文件名
						{
							MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = fileInfo.absolutePath();
						}
					}
				}
			}
			////单位制
			else if (currentline.contains("*MXUNIT"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_unitSystem = data.last().simplified();
				}
			}
			//许可证方式
			else if (currentline.contains("*MXLICENSETYPE")) {
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_licenseType = data.last().simplified();
				}
			}
			////菜单栏样式
			else if (currentline.contains("*MXRIBBONMENU"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_isRibbon = QVariant(data.last().simplified()).toBool();
				}
			}

			////Mechanical
			else if (currentline.contains("*MXSIMMECHANICAL"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_hasMechanical = QVariant(data.last().simplified()).toBool();
				}
			}

			////Dyna
			else if (currentline.contains("*MXSIMDYNA"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_hasDyna = QVariant(data.last().simplified()).toBool();
				}
			}

			////CFD
			else if (currentline.contains("*MXSIMCFD"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_hasCFD = QVariant(data.last().simplified()).toBool();
				}
			}

			////Emag
			else if (currentline.contains("*MXSIMEMAG"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_hasEmag = QVariant(data.last().simplified()).toBool();
				}
			}

			////MutliPhysics
			else if (currentline.contains("*MXSIMMULTIPHYSICS"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_hasMultiPhysics = QVariant(data.last().simplified()).toBool();
				}
			}

			////Post
			else if (currentline.contains("*MXSIMPOST"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_hasPost = QVariant(data.last().simplified()).toBool();
				}
			}

			////快捷键
			else if (currentline.contains("*MXSHORTCUT"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					QString validstr = data.last().simplified();
					QStringList templist = validstr.split(":");
					MAppConfiguration::mAppConfiguration::getInstance()->insertShortcut(templist.first().simplified(), templist.last().simplified());
				}
			}

			////视图样式
			else if (currentline.contains("*MXVIEWSTYLE"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_viewStyle = data.last().simplified();
				}
			}

			////中英文
			else if (currentline.contains("*MXLANGUAGE"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_language = QVariant(data.last().simplified()).toInt();
				}
			}

			////窗口固定
			else if (currentline.contains("*MXWIDGETFIX"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_isWidgetFix = QVariant(data.last().simplified()).toInt();
				}
			}

			////背景颜色
			else if (currentline.contains("*MXPURECOLOR"))
			{
				data = currentline.split("=");
				if (data.size() == 2)
				{
					MAppConfiguration::mAppConfiguration::getInstance()->_pureColor = data.last().simplified();
				}
			}

            ////软件名称
            else if (currentline.contains("*MXSOFTWARENAME"))
            {
                data = currentline.split("=");
                if (data.size() == 2)
                {
                    MAppConfiguration::mAppConfiguration::getInstance()->_softwareName = data.last().simplified();
                }
            }
		}
		file.close();
		MAppConfiguration::mAppConfiguration::getInstance()->setRecentMxCaeFiles(recentOpenMxCaeFileDir);
		ensureDefaultWorkingDirs();
	}

	void mIConfigurationFile::ensureDefaultWorkingDirs()
	{
		MAppConfiguration::mAppConfiguration* config = MAppConfiguration::mAppConfiguration::getInstance();
		//Windows/Linux首次启动时工作目录均不会被自动创建（配置文件在首次正常退出
		//时才写出），配置指向的目录也可能已被删除；目录不存在时文件对话框会把
		//路径末段（如"WorkingDir"）当作默认文件名，这里统一补建
		auto ensureDir = [&config](QString& dir)
		{
			if (QFileInfo(dir).isDir())
				return true;
			if (QDir().mkpath(dir))
				return true;
			//安装目录只读等场景创建失败，回退到当前用户可写的数据目录
			dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/WorkingDir").absolutePath();
			return QDir().mkpath(dir);
		};
		const bool workingOk = ensureDir(config->_workingDir);
		//最近目录已失效且无法重建时，不再沿用失效路径，回退到已确保存在的工作目录
		if (!ensureDir(config->_recentDir) && workingOk)
		{
			config->_recentDir = config->_workingDir;
		}
	}
}
