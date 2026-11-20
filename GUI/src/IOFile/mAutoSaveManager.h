#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*==================================================================================
* 自动保存功能总开关（该功能暂时不上线）
*
* 屏蔽方式：注释掉下面的 MX_AUTOSAVE_ENABLED 宏定义并重新编译。主页菜单栏的
* “自动保存”按钮和启动时的恢复询问弹窗一起消失（mMenuWidget中仅这两处入口
* 调用以该宏包裹，其余代码不受影响）。
* 注意：mAutoSaveManager类与mMenuWidget的槽声明始终参与编译——moc生成物不随
* 开关变化，避免注释宏后出现moc文件与类定义不一致的编译错误；屏蔽状态下
* 管理器不会被实例化启用，不产生任何行为。
==================================================================================*/
//#define MX_AUTOSAVE_ENABLED

#include "miofile_global.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>

namespace MechIOFile
{
	/*
	* 自动保存目录（安装目录同级）
	*/
	inline QString autoSaveDir()
	{
		//与 Setting、WorkingDir 同级，位于安装目录旁的独立自动保存文件夹
		return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../AutoSave");
	}

	/*
	* 自动保存文件完整路径
	*/
	inline QString autoSaveFilePath()
	{
		return autoSaveDir() + "/autosave.mxcae";
	}

	/*
	* 判断给定文件名是否为自动保存文件（功能屏蔽后恒为false）
	* 用于写线程、完成回调等处识别自动保存写入：不压缩、不输出提示信息
	*/
	inline bool isAutoSaveFileName(const QString& name)
	{
#ifdef MX_AUTOSAVE_ENABLED
		return QString::compare(QDir::cleanPath(name), autoSaveFilePath(), Qt::CaseInsensitive) == 0;
#else
		return false;
#endif
	}

	/*
	* 工程文件自动保存管理器（单例，始终编译；是否生效由mMenuWidget中的
	* MX_AUTOSAVE_ENABLED入口调用与isAutoSaveFileName共同控制）
	* 1. 定时将当前所有前处理模型写入自动保存目录下的 mxcae 文件（复用 mOMxCaeFile 写线程）；
	* 2. 保存间隔默认5分钟，可运行期通过 setIntervalMinutes 调整（不写死）；
	* 3. 自动保存写入不压缩（压缩级别0）、不置灰界面、不输出提示信息，避免干扰用户操作；
	* 4. 软件正常退出时自动删除自动保存文件，恢复询问弹窗只出现在异常退出（崩溃/断电）之后。
	*/
	class MIOFILE_EXPORT mAutoSaveManager : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针
		*/
		static mAutoSaveManager* getInstance();

		/*
		* 默认自动保存间隔（分钟）
		*/
		static int defaultIntervalMinutes() { return 5; }

		/*
		* 自动保存使用的压缩级别：0即不压缩（zlib存储模式），
		* 文件格式与压缩保存完全兼容，读取端qUncompress透明解包
		*/
		static int autoSaveCompressLevel() { return 0; }

		/*
		* 是否存在可恢复的自动保存文件
		*/
		static bool hasRecoveryFile();

		/*
		* 删除自动保存文件（放弃恢复或恢复正常现场后调用）
		*/
		static void clearRecoveryFile();

		/*
		* 开启/关闭自动保存
		*/
		void setAutoSaveEnabled(bool enable);

		/*
		* 自动保存是否开启
		*/
		bool isAutoSaveEnabled() const { return _enabled; }

		/*
		* 设置自动保存间隔（分钟，最小1分钟），开启状态下立即生效
		*/
		void setIntervalMinutes(int minutes);

		/*
		* 当前自动保存间隔（分钟）
		*/
		int intervalMinutes() const { return _intervalMinutes; }

		/*
		* 立即执行一次自动保存（定时器到点也调用该函数）；
		* 正在读/写文件或无可保存的前处理模型时本轮跳过
		*/
		void triggerAutoSave();

	private slots:
		/*
		* 定时器到点
		*/
		void slot_timeout();

		/*
		* 软件正常退出：停止定时器并清除自动保存文件
		*/
		void slot_aboutToQuit();

	private:
		/*
		* 构造函数
		*/
		mAutoSaveManager();

		/*
		* 析构函数
		*/
		~mAutoSaveManager();

	private:
		//单例指针
		static mAutoSaveManager* _instance;

		//是否开启自动保存
		bool _enabled = false;

		//自动保存间隔（分钟）
		int _intervalMinutes = defaultIntervalMinutes();

		//自动保存定时器
		QTimer* _timer = nullptr;
	};
}