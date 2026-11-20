#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"

#include <QObject>

namespace MechIOFile
{
	class MIOFILE_EXPORT mIConfigurationFile : public QObject
	{
		Q_OBJECT

	public:

		/*
		 * 默认构造函数
		 */
		mIConfigurationFile();

		/*
		 * 默认析构函数
		 */
		~mIConfigurationFile();

		/*
		 * 读软件配置文件
		 */
		void readConfigureData();

	private:

		/*
		 * 保证工作目录/最近目录真实存在（不存在则自动创建，Windows/Linux通用）
		 */
		void ensureDefaultWorkingDirs();
	};
}
