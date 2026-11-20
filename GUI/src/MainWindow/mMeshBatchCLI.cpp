#pragma execution_character_set("utf-8")
#include "mMeshBatchCLI.h"

#include "mMeshBatchAPI.h"
#include "mBatchMeshController.h"
#include "mIOFileManager.h"
#include "mGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mGeoPythonAPIRegistrar.h"
#include "mMeshIO.h"
#include "GlobalMeshMessage.h"
#include "mBrowserWindow.h"
#include "mTreeView.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QUuid>
#include <QTextStream>
#include <QElapsedTimer>
#include <QDebug>

using namespace MechPreData;

namespace MGUIMesh
{

    bool mMeshBatchCLI::parseArguments(const QStringList& arguments, BatchCLIConfig& config)
    {
        config = BatchCLIConfig();
        for (int i = 1; i < arguments.size(); ++i)
        {
            const QString arg = arguments.at(i);
            if (arg == QString("--batch"))
            {
                if (i + 1 >= arguments.size())
                    return false;
                config.batchConfigPath = arguments.at(++i);
                config.batchMode = true;
            }
            else if (arg == QString("--report"))
            {
                if (i + 1 >= arguments.size())
                    return false;
                config.reportPath = arguments.at(++i);
            }
            else if (arg == QString("--log"))
            {
                if (i + 1 >= arguments.size())
                    return false;
                config.logPath = arguments.at(++i);
            }
            else if (arg == QString("--concurrency"))
            {
                if (i + 1 >= arguments.size())
                    return false;
                config.concurrency = arguments.at(++i).toInt();
            }
            else if (arg == QString("--timeout"))
            {
                if (i + 1 >= arguments.size())
                    return false;
                config.timeoutMs = arguments.at(++i).toInt();
            }
            else if (arg == QString("--headless"))
            {
                config.headless = true;
            }
        }
        return config.isValid();
    }

    mMeshBatchCLI::mMeshBatchCLI(const BatchCLIConfig& config, QObject* parent)
        : QObject(parent), _config(config)
    {
        _batchId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // headless 几何导入：通过 MainWindow 中已链接 MxGeoManager 的 API 完成导入。
    // 导入完成后 modelData->getGeoData() / getGeoManager() 将包含几何数据。
    bool mMeshBatchCLI::importGeoFileHeadless(const QString& filePath)
    {
        QString apiError;
        bool ok = MechMainWindow::mGeoPythonAPIRegistrar::instance()->importGeoFileHeadless(filePath, apiError);
        if (!ok)
        {
            _errorMsg = QString("headless 几何导入失败：%1").arg(apiError);
            logMessage(_errorMsg);
            return false;
        }

        logMessage(QString("headless 几何导入成功：%1").arg(filePath));
        return true;
    }

    void mMeshBatchCLI::start()
    {
        logMessage(QString("批量任务启动：batchId=%1").arg(_batchId));

        // 解析配置文件
        if (!loadTasksFromConfig())
        {
            finish(static_cast<int>(BatchCLIExitCode::ConfigParseFailed));
            return;
        }

        // 准备几何文件列表（支持目录/通配符）
        if (!prepareGeoFiles())
        {
            finish(static_cast<int>(BatchCLIExitCode::ModelLoadFailed));
            return;
        }

        if (_geoFiles.isEmpty())
        {
            _errorMsg = QString("未找到任何可导入的几何文件");
            logMessage(_errorMsg);
            finish(static_cast<int>(BatchCLIExitCode::ModelLoadFailed));
            return;
        }

        logMessage(QString("共发现 %1 个待处理几何文件").arg(_geoFiles.size()));

        // 处理第一个几何文件，后续在 onModelLoaded 中链式处理
        _currentGeoIndex = 0;
        processNextGeoFile();
    }

    bool mMeshBatchCLI::loadTasksFromConfig()
    {
        QFile file(_config.batchConfigPath);
        if (!file.exists())
        {
            _errorMsg = QString("批量任务配置文件不存在：%1").arg(_config.batchConfigPath);
            logMessage(_errorMsg);
            return false;
        }

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            _errorMsg = QString("无法打开批量任务配置文件：%1").arg(_config.batchConfigPath);
            logMessage(_errorMsg);
            return false;
        }

        QString json = QString::fromUtf8(file.readAll());
        file.close();

        QString error;
        if (!mMeshBatchAPI::parseTasksFromJson(json, _tasks, error))
        {
            _errorMsg = QString("解析批量任务配置失败：%1").arg(error);
            logMessage(_errorMsg);
            return false;
        }

        // 读取配置文件中并发度与日志路径，命令行参数优先
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject())
        {
            QJsonObject root = doc.object();
            if (_config.concurrency < 1 && root.contains("concurrency"))
            {
                _config.concurrency = root.value("concurrency").toInt(1);
            }
            if (_config.logPath.isEmpty() && root.contains("output"))
            {
                QJsonObject outputObj = root.value("output").toObject();
                if (outputObj.contains("log"))
                {
                    _config.logPath = outputObj.value("log").toString();
                }
            }
        }
        if (_config.concurrency < 1)
        {
            _config.concurrency = 1; // 首期限制为 1，避免共享数据写冲突
        }

        // 记录任务使用的文件夹，供过滤任务与输出路径消歧使用
        int folderTaskCount = 0;
        for (const auto& task : _tasks)
        {
            if (!task.folders.isEmpty())
                folderTaskCount++;
        }

        if (folderTaskCount > 0)
        {
            for (const auto& task : _tasks)
            {
                if (!task.folders.isEmpty())
                    logMessage(QString("任务 %1 限定文件夹：%2").arg(task.taskId).arg(task.folders.join(", ")));
            }
            logMessage(QString("共 %1 个任务指定了文件夹，未匹配到任务的文件将被跳过").arg(folderTaskCount));
        }

        logMessage(QString("成功解析 %1 个批量任务").arg(_tasks.size()));
        return true;
    }

    bool mMeshBatchCLI::prepareGeoFiles()
    {
        QFile file(_config.batchConfigPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            _errorMsg = QString("无法打开批量任务配置文件：%1").arg(_config.batchConfigPath);
            logMessage(_errorMsg);
            return false;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
        file.close();
        if (parseError.error != QJsonParseError::NoError)
        {
            _errorMsg = QString("配置文件 JSON 解析失败：%1").arg(parseError.errorString());
            logMessage(_errorMsg);
            return false;
        }

        QJsonObject root = doc.object();
        QJsonObject inputObj = root.value("input").toObject();
        QString projectFile = inputObj.value("project").toString();
        QJsonArray geoFiles = inputObj.value("geoFiles").toArray();

        if (!projectFile.isEmpty())
        {
            // 工程文件模式：不支持目录展开，直接作为单一文件处理
            _geoFiles.append(projectFile);
            return true;
        }

        static const QSet<QString> supportedGeoSuffixes = {
            "stp", "step", "igs", "iges", "x_t", "x_b", "sat", "sab", "stl",
            "inp", "obj", "bdf", "cgns", "plot3d", "fem", "cdb", "k", "msh", "cas", "mx"
        };

        for (const QJsonValue& value : geoFiles)
        {
            QString rawPath = value.toString();
            if (rawPath.isEmpty())
                continue;

            QFileInfo info(rawPath);
            if (info.isDir())
            {
                // 目录：递归扫描所有支持的 CAD/网格后缀
                QDir dir(rawPath);
                QDirIterator it(dir.absolutePath(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                while (it.hasNext())
                {
                    QString filePath = it.next();
                    QFileInfo fileInfo(filePath);
                    if (supportedGeoSuffixes.contains(fileInfo.suffix().toLower()))
                    {
                        _geoFiles.append(fileInfo.absoluteFilePath());
                    }
                }
            }
            else if (rawPath.contains('*') || rawPath.contains('?'))
            {
                // 通配符：按 QDir 进行匹配
                QFileInfo patternInfo(rawPath);
                QDir dir = patternInfo.absoluteDir();
                QString pattern = patternInfo.fileName();
                QStringList matched = dir.entryList(QStringList() << pattern, QDir::Files | QDir::NoDotAndDotDot);
                for (const QString& name : matched)
                {
                    _geoFiles.append(dir.absoluteFilePath(name));
                }
            }
            else
            {
                _geoFiles.append(info.absoluteFilePath());
            }
        }

        // 去重并保持顺序
        QStringList uniqueFiles;
        QSet<QString> seen;
        for (const QString& filePath : _geoFiles)
        {
            if (!seen.contains(filePath))
            {
                seen.insert(filePath);
                uniqueFiles.append(filePath);
            }
        }
        _geoFiles = uniqueFiles;
        return true;
    }

    void mMeshBatchCLI::processNextGeoFile()
    {
        if (_currentGeoIndex < 0 || _currentGeoIndex >= _geoFiles.size())
        {
            // 所有文件处理完成，汇总报告
            logMessage(QString("所有几何文件处理完成：成功 %1，失败 %2（文件级），跳过 %3 个未匹配文件")
                .arg(_totalSuccessCount).arg(_totalFailCount).arg(_skippedFileCount));

            if (_totalFailCount > 0)
            {
                finish(static_cast<int>(BatchCLIExitCode::TaskFailed));
            }
            else
            {
                writeReport(_totalSuccessCount, _totalFailCount);
                finish(static_cast<int>(BatchCLIExitCode::Success));
            }
            return;
        }

        // 任务-文件夹过滤前置到加载模型之前：未匹配到任何任务的文件直接跳过，
        // 不加载模型、不剖分，仅在日志和报告中标记
        QString currentPath = _geoFiles[_currentGeoIndex];
        _currentFileTasks = tasksForFile(currentPath);
        if (_currentFileTasks.isEmpty())
        {
            _currentFileSkipped = true;
            _skippedFileCount++;
            logMessage(QString("文件未被任何任务的 folders 覆盖，跳过：%1").arg(currentPath));

            FileResult result;
            result.filePath = currentPath;
            _fileResults.append(result);

            _currentGeoIndex++;
            processNextGeoFile();
            return;
        }
        _currentFileSkipped = false;

        if (_currentGeoIndex > 0)
        {
            // 在切换文件前清理当前模型数据，避免新旧几何混合
            clearCurrentModelData();
        }

        logMessage(QString("正在处理第 %1/%2 个文件：%3")
            .arg(_currentGeoIndex + 1).arg(_geoFiles.size()).arg(_geoFiles[_currentGeoIndex]));

        if (!loadModel())
        {
            _totalFailCount++;
            _currentGeoIndex++;
            processNextGeoFile();
        }
        // loadModel 成功后会进入 onModelLoaded 继续
    }

    void mMeshBatchCLI::clearCurrentModelData()
    {
        //完整清空当前模型（数据/视口/模型树），保留当前模型名称与ID
        if (getCVM != nullptr)
        {
            MechMainWindow::mBrowserWindow::getInstance()->getBrowserTree()->clearCurrentModelData();
            logMessage(QString("已清空当前模型数据"));
        }
    }

    bool mMeshBatchCLI::loadModel()
    {
        if (_currentGeoIndex < 0 || _currentGeoIndex >= _geoFiles.size())
        {
            _errorMsg = QString("当前几何文件索引越界");
            logMessage(_errorMsg);
            return false;
        }

        QString currentPath = _geoFiles[_currentGeoIndex];
        QFileInfo info(currentPath);
        if (!info.exists())
        {
            _errorMsg = QString("文件不存在：%1").arg(currentPath);
            logMessage(_errorMsg);
            return false;
        }

        MechIOFile::mIOFileManager* ioManager = MechIOFile::mIOFileManager::getInstance();
        QString suffix = info.suffix().toLower();

        // 工程文件分支：仅用于 input.project，实际 prepareGeoFiles 已处理
        if (info.suffix().compare("mxcae", Qt::CaseInsensitive) == 0)
        {
            connect(ioManager, &MechIOFile::mIOFileManager::readMxcaeFinishedSig,
                this, &mMeshBatchCLI::onModelLoaded, Qt::UniqueConnection);

            ioManager->readMxcaeFile(currentPath);
            logMessage(QString("开始读取工程文件：%1").arg(currentPath));
            return true;
        }

        // 判断是否为网格文件：这些文件走 mIOFileManager 的网格/求解文件导入通道
        static const QSet<QString> meshSuffixes = {
            "inp", "obj", "stl", "bdf", "cgns", "plot3d",
            "fem", "cdb", "k", "msh", "cas", "mx"
        };

        if (meshSuffixes.contains(suffix))
        {
            connect(ioManager, &MechIOFile::mIOFileManager::readSolveFinishedSig,
                this, &mMeshBatchCLI::onModelLoaded, Qt::UniqueConnection);

            ioManager->slot_importMeshFile(currentPath);
            logMessage(QString("开始导入网格文件：%1").arg(currentPath));
            return true; // 异步等待 readSolveFinishedSig
        }
        else
        {
            // STEP/IGES/CAD 几何文件在 headless 模式下使用无渲染导入路径。
            if (!importGeoFileHeadless(currentPath))
            {
                return false;
            }

            logMessage(QString("几何文件导入完成：%1").arg(currentPath));
            // headless 导入是同步等待完成的，直接继续后续流程
            onModelLoaded();
            return true;
        }
    }

    // 按任务声明的 folders 过滤出适用于指定文件的任务。
    // 规则：文件路径以某任务的文件夹为前缀（含子目录）则匹配；不带 folders 的任务
    // 视为全局任务，匹配所有文件；任何任务指定了 folders 时，未匹配的文件返回空列表。
    QList<BatchMeshTask> mMeshBatchCLI::tasksForFile(const QString& filePath) const
    {
        bool anyFolderTask = false;
        for (const auto& task : _tasks)
        {
            if (!task.folders.isEmpty())
            {
                anyFolderTask = true;
                break;
            }
        }

        if (!anyFolderTask)
            return _tasks; // 兼容旧配置：所有任务对所有文件生效

        QString normalizedFile = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
        QList<BatchMeshTask> matched;
        for (const auto& task : _tasks)
        {
            if (task.folders.isEmpty())
            {
                matched.append(task);
                continue;
            }

            for (const QString& folder : task.folders)
            {
                if (normalizedFile.startsWith(folder, Qt::CaseInsensitive))
                {
                    matched.append(task);
                    break;
                }
            }
        }
        return matched;
    }

    void mMeshBatchCLI::onModelLoaded()
    {
        MechIOFile::mIOFileManager* ioManager = MechIOFile::mIOFileManager::getInstance();
        disconnect(ioManager, &MechIOFile::mIOFileManager::readMxcaeFinishedSig,
            this, &mMeshBatchCLI::onModelLoaded);
        disconnect(ioManager, &MechIOFile::mIOFileManager::readSolveFinishedSig,
            this, &mMeshBatchCLI::onModelLoaded);

        QString currentPath = _geoFiles[_currentGeoIndex];

        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData)
        {
            _errorMsg = QString("模型加载后无模型数据：%1").arg(currentPath);
            logMessage(_errorMsg);
            _totalFailCount++;
            _currentGeoIndex++;
            processNextGeoFile();
            return;
        }

        // 导入 STEP 后仅有几何数据，尚未生成网格，因此不强制要求 meshdata->getMeshEntity()
        logMessage(QString("模型加载完成，开始执行 %1 个批量剖分任务：%2").arg(_currentFileTasks.size()).arg(currentPath));
        runBatchTasks();
    }

    void mMeshBatchCLI::onAllFinished(int successCount, int failCount)
    {
        _successCount = successCount;
        _failCount = failCount;
    }

    void mMeshBatchCLI::runBatchTasks()
    {
        mBatchMeshController* controller = new mBatchMeshController(this);
        controller->setMaxConcurrency(_config.concurrency);

        QEventLoop loop;
        QTimer timeoutTimer;
        bool timeout = false;

        if (_config.timeoutMs > 0)
        {
            timeoutTimer.setSingleShot(true);
            QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
                timeout = true;
                controller->cancelAll();
                loop.quit();
            });
            timeoutTimer.start(_config.timeoutMs);
        }

        QObject::connect(controller, &mBatchMeshController::taskStarted, &loop,
            [&](QString taskId) {
                logMessage(QString("任务开始：%1").arg(taskId));
            });

        QObject::connect(controller, &mBatchMeshController::taskFinished, &loop,
            [&](QString taskId, bool ok, QString msg, qint64 elapsedMs) {
                logMessage(QString("任务结束：%1，%2，耗时 %3 ms")
                    .arg(taskId).arg(ok ? QString("成功") : QString("失败-%1").arg(msg)).arg(elapsedMs));
            });

        QObject::connect(controller, &mBatchMeshController::allFinished, &loop,
            [&](int s, int f) {
                _successCount = s;
                _failCount = f;
                loop.quit();
            });

        QElapsedTimer timer;
        timer.start();
        controller->submitTasks(_currentFileTasks);
        loop.exec();

        logMessage(QString("批量剖分完成：成功 %1，失败 %2，总耗时 %3 ms")
            .arg(_successCount).arg(_failCount).arg(timer.elapsed()));

        // 保存任务结果到报告结构
        _taskResults = controller->taskResults();
        _taskElapsed = controller->taskElapsed();

        // 记录当前文件的处理结果
        FileResult result;
        result.filePath = _geoFiles[_currentGeoIndex];
        result.successCount = _successCount;
        result.failCount = _failCount;
        result.taskResults = _taskResults;
        result.taskElapsed = _taskElapsed;
        _fileResults.append(result);

        _totalSuccessCount += _successCount;
        _totalFailCount += _failCount;

        if (timeout)
        {
            _currentGeoIndex++;
            processNextGeoFile();
            return;
        }

        // 保存/导出：多文件场景下自动按文件名派生输出路径
        if (!saveAndExport())
        {
            _currentGeoIndex++;
            processNextGeoFile();
            return;
        }

        _currentGeoIndex++;
        processNextGeoFile();
    }

    QString mMeshBatchCLI::resolveOutputPath(const QString& configuredPath, const QString& defaultSuffix) const
    {
        if (configuredPath.isEmpty())
            return QString();

        QString currentPath = _geoFiles.value(_currentGeoIndex);
        if (currentPath.isEmpty())
            return configuredPath;

        QFileInfo currentInfo(currentPath);
        QString baseName = currentInfo.completeBaseName();

        QFileInfo outInfo(configuredPath);
        QDir dir = outInfo.absoluteDir();

        // 如果配置路径以斜杠结尾，或仅指定了目录，则使用默认文件名
        QString fileName = outInfo.fileName();
        QString suffix = outInfo.suffix().isEmpty() ? defaultSuffix : outInfo.suffix();
        if (fileName.isEmpty() || configuredPath.endsWith('/') || configuredPath.endsWith('\\'))
        {
            fileName = QString("%1.%2").arg(baseName).arg(defaultSuffix);
        }
        else
        {
            // 保留原后缀，将主文件名替换为当前几何文件名
            fileName = QString("%1.%2").arg(baseName).arg(suffix);
        }

        QString resolved = dir.absoluteFilePath(fileName);

        // 多文件场景下不同文件夹可能存在同名几何文件：检测输出路径冲突，
        // 冲突时在文件名前追加来源文件夹名作为前缀，避免互相覆盖
        if (_geoFiles.size() > 1)
        {
            QString resolvedKey = QDir::cleanPath(resolved).toLower();
            if (_usedOutputPaths.contains(resolvedKey))
            {
                QString parentDirName = currentInfo.absoluteDir().dirName();
                QString newFileName = QString("%1_%2").arg(parentDirName).arg(fileName);
                QString candidate = dir.absoluteFilePath(newFileName);
                int counter = 2;
                while (_usedOutputPaths.contains(QDir::cleanPath(candidate).toLower()))
                {
                    candidate = dir.absoluteFilePath(QString("%1_%2_%3").arg(parentDirName).arg(counter++).arg(fileName));
                }
                resolved = candidate;
                resolvedKey = QDir::cleanPath(resolved).toLower();
            }
            _usedOutputPaths.insert(resolvedKey);
        }

        return resolved;
    }

    bool mMeshBatchCLI::saveAndExport()
    {
        QFile file(_config.batchConfigPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return true; // 无 output 配置时不视为失败

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
        file.close();
        if (parseError.error != QJsonParseError::NoError)
            return true;

        QJsonObject root = doc.object();
        QJsonObject outputObj = root.value("output").toObject();
        QString saveProject = outputObj.value("saveProject").toString();
        QString exportMesh = outputObj.value("exportMesh").toString();

        // 多文件模式：为每个文件生成独立输出路径
        if (_geoFiles.size() > 1)
        {
            saveProject = resolveOutputPath(saveProject, "mxcae");
            exportMesh = resolveOutputPath(exportMesh, "inp");
        }

        MechIOFile::mIOFileManager* ioManager = MechIOFile::mIOFileManager::getInstance();

        if (!saveProject.isEmpty())
        {
            QFileInfo info(saveProject);
            QDir dir = info.dir();
            if (!dir.exists())
            {
                dir.mkpath(".");
            }

            QEventLoop loop;
            connect(ioManager, &MechIOFile::mIOFileManager::writeMxcaeFinishedSig,
                &loop, &QEventLoop::quit, Qt::UniqueConnection);
            ioManager->writeMxcaeFile(saveProject);
            loop.exec();

            logMessage(QString("工程已保存：%1").arg(saveProject));
        }

        if (!exportMesh.isEmpty())
        {
            QFileInfo info(exportMesh);
            QDir dir = info.dir();
            if (!dir.exists())
            {
                dir.mkpath(".");
            }

            QString suffix = info.suffix().toLower();
            static const QSet<QString> meshSuffixes = {
                "inp", "stl", "obj", "vtk"
            };

            if (meshSuffixes.contains(suffix))
            {
                MechModelData::mModelData* modelData = getCVM->getModelData();
                if (modelData && modelData->getMeshData())
                {
                    GlobalMeshMessage::getInstance()->setMeshData(modelData->getMeshData());
                    if (suffix == "inp")
                        meshOutputINP(false, false, exportMesh);
                    else if (suffix == "stl")
                        meshOutputSTL(false, false, exportMesh);
                    else if (suffix == "obj")
                        meshOutputOBJ(false, exportMesh);
                    else if (suffix == "vtk")
                        meshOutputVTK(false, exportMesh);
                    logMessage(QString("网格已导出：%1").arg(exportMesh));
                }
                else
                {
                    _errorMsg = QString("导出网格失败：模型中无可用网格数据");
                    logMessage(_errorMsg);
                    return false;
                }
            }
            else
            {
                // 计算模型/求解文件走 mIOFileManager 原有通道
                QEventLoop loop;
                connect(ioManager, &MechIOFile::mIOFileManager::writeSolveFinishedSig,
                    &loop, &QEventLoop::quit, Qt::UniqueConnection);
                ioManager->slot_exportSolveFile(exportMesh);
                loop.exec();

                logMessage(QString("计算模型已导出：%1").arg(exportMesh));
            }
        }

        return true;
    }

    void mMeshBatchCLI::writeReport(int successCount, int failCount)
    {
        if (_config.reportPath.isEmpty())
            return;

        QJsonObject root;
        root["batchId"] = _batchId;
        root["startTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        root["endTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        root["successCount"] = successCount;
        root["failCount"] = failCount;
        root["totalFiles"] = static_cast<int>(_geoFiles.size());

        QJsonArray filesArray;
        for (const auto& fileResult : _fileResults)
        {
            QJsonObject fileObj;
            fileObj["filePath"] = fileResult.filePath;
            fileObj["successCount"] = fileResult.successCount;
            fileObj["failCount"] = fileResult.failCount;

            QList<BatchMeshTask> fileTasks = tasksForFile(fileResult.filePath);
            if (fileTasks.isEmpty())
            {
                fileObj["skipped"] = true;
                fileObj["message"] = QString("未匹配到任何任务的文件夹，已跳过");
            }

            QJsonArray tasksArray;
            for (const auto& task : fileTasks)
            {
                QJsonObject taskObj;
                taskObj["taskId"] = task.taskId;
                taskObj["name"] = task.name;
                taskObj["dim"] = task.dim;
                if (!task.folders.isEmpty())
                {
                    taskObj["folders"] = QJsonArray::fromStringList(task.folders);
                }
                auto result = fileResult.taskResults.value(task.taskId, qMakePair(false, QString("未知")));
                taskObj["success"] = result.first;
                taskObj["message"] = result.second;
                taskObj["elapsedMs"] = static_cast<int>(fileResult.taskElapsed.value(task.taskId, 0));
                tasksArray.append(taskObj);
            }
            fileObj["tasks"] = tasksArray;
            filesArray.append(fileObj);
        }
        root["files"] = filesArray;
        root["skippedFiles"] = _skippedFileCount;

        if (!_errorMsg.isEmpty())
        {
            root["error"] = _errorMsg;
        }

        QFileInfo info(_config.reportPath);
        QDir dir = info.dir();
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        QFile file(_config.reportPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            file.close();
            logMessage(QString("报告已写入：%1").arg(_config.reportPath));
        }
        else
        {
            logMessage(QString("报告写入失败：%1").arg(_config.reportPath));
        }
    }

    void mMeshBatchCLI::logMessage(const QString& msg)
    {
        QString line = QString("[%1] %2")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(msg);
        qInfo().noquote() << line;

        if (!_config.logPath.isEmpty())
        {
            QFileInfo info(_config.logPath);
            QDir dir = info.dir();
            if (!dir.exists())
            {
                dir.mkpath(".");
            }

            QFile file(_config.logPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            {
                QTextStream stream(&file);
                stream << line << "\n";
                file.close();
            }
        }
    }

    void mMeshBatchCLI::finish(int exitCode)
    {
        logMessage(QString("批量任务结束，退出码：%1").arg(exitCode));
        emit finished(exitCode);
    }
}
