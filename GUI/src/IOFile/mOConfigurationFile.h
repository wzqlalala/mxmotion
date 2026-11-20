#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"

#include <QObject>

namespace MechIOFile
{
	class MIOFILE_EXPORT mOConfigurationFile : public QObject
	{
		Q_OBJECT
	public:

		/*
		 * 默认构造函数
		 */
		mOConfigurationFile();

		/*
		 * 默认析构函数
		 */
		~mOConfigurationFile();

		/*
		 * 读软件配置文件
		 */
		void writeConfigureData();
	};
}
