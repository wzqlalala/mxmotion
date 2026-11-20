#pragma once
#include "miofile_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mIOEnum.h"

#include <QObject>
#include <QTreeWidgetItem>
#include <QTime>
#include <QList>
#include "mMxsesFormat.h"
#include <memory>
#include "mModelData.h"
namespace MechMaterialData {
    class mMaterialBase;
}

namespace MechModelData {
    class mModelData;
}

struct meshReadPart;

namespace MechIOFile
{
    extern MIOFILE_EXPORT QTime time;

	class mIMxCaeFile;
	class mOMxCaeFile;
	class mIMxsesFile;
	class mOMxsesFile;
	class mIThirdFile;
	class mOThirdFile;
    class mIMeshIOFile;
    class mOMfFile;
    class mIMfFile;
    class mIMotionJsonFile;
    class mOMotionJsonFile;
    class MIOFILE_EXPORT mIOFileManager : public QObject
    {
        Q_OBJECT
    public:
        /*
        * 获取单例指针
        */
        static mIOFileManager* getInstance();

        /*
        * 判断是否有未完成的线程
        */
        bool hasRunningThread();

        /*
        * 判断写线程是否为空
        */
        bool hasRunningWrtiteMxcaeThread();

        /*
        * 强制关闭所有读写文件的线程
        */
        void killFileThread();

        /*****************************************工程文件*****************************************/
        /*
        * 读取工程文件
        */
        void readMxcaeFile(const QString& filename);

        /*
        * 写自定义材料库文件
        */
        void writeCustomMaterialFile(const QString& filename, std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 读自定义材料库文件
        */
        void readCustomMaterialFile(const QString& filename, std::shared_ptr<MechMaterialData::mMaterialBase>& mat);

        /*
        * 获取读工程文件是否成功
        */
        bool isSuccessReadMxcae();

        /*
        * 获取最近一次读工程文件是否成功
        * （读线程释放后 isSuccessReadMxcae 不再可用，该结果在读取完成时记录）
        */
        bool lastReadMxcaeSucceeded() const { return _lastReadMxcaeSuccess; }

        /*
        * 获取读取的工程文件名
        */
        QString getReadMxcaeFileName();

        /*
        * 写工程文件
        * compressLevel：压缩级别，默认9（最高压缩）；自动保存传0（不压缩）
        */
        void writeMxcaeFile(const QString& filename, int compressLevel = 9);

        /*
        * 获取写的工程文件名
        */
        QString getWriteMxcaeFileName();

        /*
        * 关闭读工程文件的线程
        */
        void releaseReadMxcaeThread();

        /*
        * 关闭写工程文件的线程
        */
        void releaseWriteMxcaeThread();

		/*****************************************会话文件(.mxses)*****************************************/
		/*
		* 读会话文件（异步线程；数据由界面线程采集后传入写线程，读线程只解析到内存）
		*/
		void readMxsesFile(const QString& filename);

		/*
		* 写会话文件（data 为界面线程采集完成的会话数据，写线程接管压缩落盘）
		*/
		void writeMxsesFile(const QString& filename, const MxsesFileData& data, int compressLevel = 9);

		/*
		* 获取读会话文件是否成功
		*/
		bool isSuccessReadMxses();

		/*
		* 获取读取的会话文件名
		*/
		QString getReadMxsesFileName();

		/*
		* 获取写的会话文件名
		*/
		QString getWriteMxsesFileName();

		/*
		* 取走读取到的会话数据（仅读取成功后调用）
		*/
		const MxsesFileData& getMxsesFileData() const;

		/*
		* 关闭读会话文件的线程
		*/
		void releaseReadMxsesThread();

		/*
		* 关闭写会话文件的线程
		*/
		void releaseWriteMxsesThread();

		/*****************************************计算文件*****************************************/
		/*
		* 获取读计算文件是否成功
		*/
		bool isSuccessReadSolve();

        /*
        * 获取读取的计算文件名
        */
        QString getReadSolveFileName();

        /*
        * 关闭读计算文件的线程
        */
        void releaseReadSolveThread();

        /*
        * 获取写工程文件是否成功
        */
        bool isSuccessWriteSolve();

        /*
        * 获取写的计算文件名
        */
        QString getWriteSolveFileName();

        /*
        * 关闭写计算文件的线程
        */
        void releaseWriteSolveThread();

        /*****************************************网格面片文件*****************************************/
        /*
        * 读取网格面片文件(obj/off/ply)
        */
        void readMeshIOFile(const QString& filename);

        /*
        * 获取读网格面片文件是否成功
        */
        bool isSuccessReadMesh();

        /*
        * 获取读取的网格面片文件名
        */
        QString getReadMeshFileName();

        /*
        * 获取读取到的部件数据
        */
        QVector<meshReadPart> getMeshParts();

		/*
		* 关闭读网格面片文件的线程
		*/
		void releaseReadMeshThread();

		/*
		* 通过序列化/反序列化深拷贝模型数据（复制模型功能用）
		*/
        static void copyModelData(MechModelData::mModelData*& oldModel, MechModelData::mModelData*& newModel);

        /*****************************************mf文件*****************************************/
        /*
        * 导出mf文件
        */
        void slot_exportMfFile(const QString& fname, const QString& modelName=QString());

        /*
        * 获取写mf文件是否成功
        */
        bool isSuccessWriteMf();
        /*
        * 获取写的mf文件名
        */
        QString getWriteMfFileName();
        /*
        * 关闭写mf文件的线程
        */
        void releaseWriteMfThread();

        /*
        * 获取读mf文件是否成功
        */
        bool isSuccessReadMf();

        /*
        * 获取读取的mf文件名
        */
        QString getReadMfFileName();

        /*
        * 关闭读mf文件的线程
        */
        void releaseReadMfThread();

        /*
        * 导入mf文件
        */
        void slot_importMfFile(const QString& fname);

    private:
        /*
        * 构造函数
        */
        mIOFileManager();

        /*
        * 析构函数
        */
        ~mIOFileManager();

        /*
        * 开始导入文件
        */
        void importFile(const QString& fname, bool isMeshFile = false);

    public slots:
        /*
        * 导入计算文件
        */
        void slot_importSolveFile(const QString& fname);

        /*
        * 导入网格文件
        */
        void slot_importMeshFile(const QString& fname);

        /*
        * 导出计算文件
        */
        void slot_exportSolveFile(const QString& fname);

        /*
        * 导入运动数据文件（Motion JSON；fromPlatform=true 表示平台触发不回传）
        */
        void slot_importMotionJsonFile(const QString& fname, bool fromPlatform = false);

        /*
        * 导出运动数据文件（Motion JSON）
        */
        void slot_exportMotionJsonFile(const QString& fname, bool autoSave = false);

        /*************************************************
        函数名称：slot_AddItemOnModelItem
        函数功能：读取（创建）几何之后添加模型树
        输入参数：i无
        输出参数：无
        *************************************************/
        void slot_AddGeoTreeItem(QTreeWidgetItem* item);

        /*
        * 运动数据导入完成
        */
        void slot_motionJsonImportFinished(bool success, const QString& msg);

        /*
        * 运动数据导出完成
        */
        void slot_motionJsonExportFinished(bool success, const QString& msg);
    signals:
        /*
        * 读写文件错误信号，适用所有文件
        */
        void ioFileErrorManagerSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);

        /*
        * 读工程文件线程完成信号
        */
        void readMxcaeFinishedSig();

        /*
        * 写工程文件线程完成信号
        */
        void writeMxcaeFinishedSig();

        /*
        * 读计算文件线程完成信号
        */
        void readSolveFinishedSig();

        /*
        * 写计算文件线程完成信号
        */
        void writeSolveFinishedSig();

        /*
        * 读会话文件线程完成信号
        */
        void readMxsesFinishedSig();

        /*
        * 写会话文件线程完成信号
        */
        void writeMxsesFinishedSig();

        /*
        * 读网格面片文件线程完成信号
        */
        void readMeshFinishedSig();

        /*
        * 写mf文件线程完成信号
        */
        void writeMfFinishedSig();

        /*
        * 读mf文件线程完成信号
        */
        void readMfFinishedSig();

        /*
        * 读运动数据文件线程完成信号
        */
        void readMotionJsonFinishedSig();

        /*
        * 写运动数据文件线程完成信号
        */
        void writeMotionJsonFinishedSig();

		/*
		* 打开文件转换窗口
		*/
		void openTransManagerSig(const QString& name);

        /*
        * 发送导入的文件名称
        */
        void openFileNameSig(const QString& name);

        /*
        * 发送导出的文件名称
        */
        void exportFileNameSig(const QString& name);

    private:
        //单例指针
        static mIOFileManager* _instance;

        //读工程文件线程
        mIMxCaeFile* _imxcaeThread;

        //最近一次读工程文件是否成功（线程释放后仍可查询）
        bool _lastReadMxcaeSuccess = false;

		//写工程文件线程
		mOMxCaeFile* _omxcaeThread;

		//读会话文件线程
		mIMxsesFile* _imxsesThread = nullptr;

		//写会话文件线程
		mOMxsesFile* _omxsesThread = nullptr;

		//读计算文件线程
		mIThirdFile* _ithirdThread;

		//最近一次导入的计算文件名（线程释放后仍可查询，供报告等使用）
		QString _lastReadSolveFileName;

		//写计算文件线程
		mOThirdFile* _othirdThread;

        //读网格面片文件线程
        mIMeshIOFile* _imeshThread;

        //读写mf文件线程
        mOMfFile* _oMfThread;
        mIMfFile* _iMfThread;

        //读写运动数据文件线程
        mIMotionJsonFile* _imotionJsonThread;
        mOMotionJsonFile* _omotionJsonThread;

        //运动数据导入导出路径与平台触发标记
        QString _motionJsonPath;
        bool _motionJsonFromPlatform = false;
        QString _motionJsonExportPath;

		//几何装配树
		QTreeWidgetItem* _modelItem = nullptr;

    public:
        //内部测试使用
        QString _cmdImportFilePath;
    };
}
