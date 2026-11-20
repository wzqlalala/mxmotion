#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QPair>
#include <QSet>
#include "BatchCLIConfig.h"
#include "mBatchMeshTask.h"
#include "mmainwindow_global.h"

namespace MGUIMesh
{
    /**
     * @brief 命令行批量几何网格剖分执行器
     */
    class MMAINWINDOW_EXPORT mMeshBatchCLI : public QObject
    {
        Q_OBJECT
    public:
        static bool parseArguments(const QStringList& arguments, BatchCLIConfig& config);

        explicit mMeshBatchCLI(const BatchCLIConfig& config, QObject* parent = nullptr);

        /**
         * @brief 开始执行批量任务，完成后通过 finished 信号返回退出码
         */
        void start();

    signals:
        void finished(int exitCode);

    private slots:
        void onModelLoaded();
        void onAllFinished(int successCount, int failCount);

    private:
        bool loadModel();
        bool loadTasksFromConfig();
        bool prepareGeoFiles();
        bool importGeoFileHeadless(const QString& filePath);
        bool saveAndExport();
        void writeReport(int successCount, int failCount);
        void logMessage(const QString& msg);
        void finish(int exitCode);

    private:
        void runBatchTasks();
        void processNextGeoFile();
        void clearCurrentModelData();
        QString resolveOutputPath(const QString& configuredPath, const QString& defaultSuffix) const;
        QList<BatchMeshTask> tasksForFile(const QString& filePath) const;

    private:
        struct FileResult
        {
            QString filePath;
            int successCount = 0;
            int failCount = 0;
            QHash<QString, QPair<bool, QString>> taskResults;
            QHash<QString, qint64> taskElapsed;
        };

        BatchCLIConfig _config;
        QString _batchId;
        QString _errorMsg;
        QStringList _geoFiles;
        int _currentGeoIndex = -1;
        QList<BatchMeshTask> _tasks;
        QList<FileResult> _fileResults;
        QHash<QString, QPair<bool, QString>> _taskResults;
        QHash<QString, qint64> _taskElapsed;
        QList<BatchMeshTask> _currentFileTasks;   // 当前文件实际执行的任务（按文件夹过滤后）
        bool _currentFileSkipped = false;         // 当前文件无匹配任务被跳过
        int _skippedFileCount = 0;                // 未匹配到任何任务而被跳过的文件数
        mutable QSet<QString> _usedOutputPaths;   // 已使用的输出路径（规范化），用于检测重名覆盖（resolveOutputPath 为 const，故可变）
        int _successCount = 0;
        int _failCount = 0;
        int _totalSuccessCount = 0;
        int _totalFailCount = 0;
        bool _modelLoaded = false;
        bool _geoImportStarted = false;
    };
}
