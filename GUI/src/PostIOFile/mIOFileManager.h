#pragma once
#include "miofile_global.h"
#pragma execution_character_set("utf-8")
#include "mIOEnum.h"
#include "mIMxdbFile1.h"
#include "mDataPost1.h"
#include <QObject>
#include <QThread>
#include <QHash>
#include <mutex>

namespace MIOFile
{
	class mIMxdbFile1;
	class mIF06File;

	class MIOFILE_EXPORT mIOFileManager : public QObject
	{
		Q_OBJECT
	public:

		/** 获取单例指针 */
		static mIOFileManager* getInstance();

		/** 判断是否有未完成的线程 */
		bool hasRunningThread();

		/** 强制关闭所有读写文件的线程 */
		void killFileThread();

		/** 读mxdb后处理文件 */
		void readMxdbFile(const QString& initfile, const QStringList& framefiles, QPointer<mPostDataBase> modeldb);

		/** 获取类线程 */
		QHash<mIMxdbFile1*, QThread*> getMxdbThreads();

		/** 通过数据库名称获取mIMxdbFile1 */
		mIMxdbFile1* getMxdb(const QString& postname);

		/** 通过mIMxdbFile1获取QThread */
		QThread* getThread1(mIMxdbFile1* mxdb);

		/** 添加后处理数据 */
		void appendDataPost(PostData::mDataPost1* datapost);

		/** 获取后处理数据 */
		PostData::mDataPost1* getDataPostFromName(const QString& name);

		/** 获取所有后处理名称 */
		QStringList getPostDataNames();

		/** 删除后处理数据 */
		void deleteDataPost(const QString& name);

		/** 删除全部后处理数据 */
		void deleteDataPost();

		/** 判断数据库是否为空 */
		bool isDatabaseEmpty();

		/** 获取是否有数据同名的 */
		bool isExist(const QString& postname);

		/** 按照名称来将数据结构的后处理数据删除 */
		PostData::mDataPost1* getmDataPostByName(const QString& postname);

		/** 将后处理数据库加入到 */
		void addmDataPost(PostData::mDataPost1* postdata);

		/** 初始化 */
		void inital();
	public slots:

		/** 将数据库存入视口 */
		void saveToDataPosttoRend();

		/** 将后处理数据转移一下 */
		void StorageMoveToTemporary(const QString &name);

		/** 暂时存储 */
		void TemporaryMoveToStorage(const QString &name);

	private:
		/** 构造函数 */
		mIOFileManager();

		/** 析构函数 */
		~mIOFileManager();

		/** 禁止拷贝构造函数 */
		mIOFileManager(const mIOFileManager&) = delete;

		/** 禁止赋值操作符 */
		mIOFileManager& operator=(const mIOFileManager&) = delete;

	signals:
		/** 读写文件错误信号，适用所有文件 */
		void ioFileErrorManagerSig(MIOFile::IOFileErrorType type = UnknownError);

		/** 开始读后处理文件 */
		void readMxdbFileManagerSig();

		/** 成功读取后处理文件 */
		void finishedMxdbManagerSig(const QString&);

		/** 完成所有文件的读取 */
		void finishedAllMxdbManagerSig(const QString& postname);

		/** 成功读取信号 */
		void readFileSuccessSig(const QString&);

		//
		void signal_updateHightTreeByRepeatName(const QString& name);
	private:
		static mIOFileManager*				_instance;					//单例指针
		static std::mutex					_mutex_mIOFileManager;		// 静态成员变量，用于保证线程安全
		QHash<mIMxdbFile1*, QThread*>		_mxdbThreads{};				//读文件线程
		QList<PostData::mDataPost1*>					_dataPosts{};				//存储数据库
		QList<PostData::mDataPost1*>					_tempList{};				//临时的数据库
	};
}