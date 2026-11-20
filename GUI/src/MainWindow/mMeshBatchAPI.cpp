#pragma execution_character_set("utf-8")
#include "mMeshBatchAPI.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHash>
#include <QSet>
#include <QFileInfo>
#include <QDir>

using namespace MGUIMesh;

namespace MGUIMesh
{
    bool mMeshBatchAPI::parseTasksFromJson(const QString& json, QList<BatchMeshTask>& outTasks, QString& error)
    {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            error = QString("JSON 解析失败：%1").arg(parseError.errorString());
            return false;
        }
        return parseTasksFromJson(doc, outTasks, error);
    }

    bool mMeshBatchAPI::parseTasksFromJson(const QJsonDocument& doc, QList<BatchMeshTask>& outTasks, QString& error)
    {
        outTasks.clear();
        if (!doc.isObject())
        {
            error = QString("批量任务配置根节点必须是 JSON 对象");
            return false;
        }

        QJsonObject root = doc.object();
        QJsonArray tasksArray = root.value("tasks").toArray();
        if (tasksArray.isEmpty())
        {
            error = QString("未找到 tasks 任务数组或为空");
            return false;
        }

        for (int i = 0; i < tasksArray.size(); ++i)
        {
            QJsonObject taskObj = tasksArray.at(i).toObject();
            BatchMeshTask task;
            task.taskId = taskObj.value("taskId").toString();
            if (task.taskId.isEmpty())
            {
                task.taskId = QString("t%1").arg(i + 1);
            }
            task.name = taskObj.value("name").toString();
            task.dim = taskObj.value("dim").toInt(-1);

            QJsonArray geoIdsArray = taskObj.value("geoIds").toArray();
            for (const QJsonValue& v : geoIdsArray)
            {
                task.geoIds.insert(v.toInt());
            }

            // 任务负责的文件夹：支持 "folder"（单个）与 "folders"（数组）两种写法；
            // 解析后立即规范化为绝对路径，重复归属校验在 validateTasks 中完成
            QString singleFolder = taskObj.value("folder").toString().trimmed();
            if (!singleFolder.isEmpty())
            {
                task.folders.append(singleFolder);
            }
            QJsonArray foldersArray = taskObj.value("folders").toArray();
            for (const QJsonValue& v : foldersArray)
            {
                QString folderPath = v.toString().trimmed();
                if (!folderPath.isEmpty())
                {
                    task.folders.append(folderPath);
                }
            }
            task.folders.removeDuplicates();

            // 立即规范化为带结尾斜杠的绝对路径，供后续前缀匹配与唯一性校验使用
            for (QString& folderPath : task.folders)
            {
                QString normalized = QDir::cleanPath(QFileInfo(folderPath).absoluteFilePath());
                if (!normalized.endsWith('/'))
                    normalized.append('/');
                folderPath = normalized;
            }
            task.folders.removeDuplicates();

            QJsonObject paramObj = taskObj.value("param").toObject();
            task.meshSize = paramObj.value("meshSize").toDouble(2.0);
            task.maxSize = paramObj.value("maxSize").toDouble(10.0);
            task.minSize = paramObj.value("minSize").toDouble(1.0);
            task.smoothness = paramObj.value("smoothness").toBool(false);
            task.smoothnessRatio = paramObj.value("smoothnessRatio").toDouble(1.2);
            task.curvature = paramObj.value("curvature").toBool(false);
            task.curvatureEigenValue = paramObj.value("curvatureEigenValue").toDouble(15.0);
            task.proximity = paramObj.value("proximity").toBool(false);
            task.proximityValue = paramObj.value("proximityValue").toDouble(2.0);
            task.autoMesh = taskObj.value("autoMesh").toBool(false);
            task.defaultSize = taskObj.value("defaultSize").toBool(true);
            task.meshShape = taskObj.value("meshShape").toString().trimmed();

            outTasks.append(task);
        }

        return validateTasks(outTasks, error);
    }

    bool mMeshBatchAPI::validateTasks(const QList<BatchMeshTask>& tasks, QString& error)
    {
        if (tasks.isEmpty())
        {
            error = QString("任务列表为空");
            return false;
        }

        // 批量剖分任务已统一委托给 mMeshThread::slot_createMeshBatch()，
        // 由该槽函数自动识别并剖分全部 1D/2D/3D 几何，因此不再校验 dim/geoIds。
        // 此处仅校验 folder(s) 归属：每个文件夹最多只能指定给一个任务。
        // 约定：folders 中的路径已按 normalizeTaskFolders 规范化为带结尾斜杠的绝对路径。
        QHash<QString, QString> folderOwner; // 规范化路径 -> taskId
        static const QSet<QString> validShapes = {
            QString("三角形"), QString("四边形主导"), QString("全四边形"),
            QString("四面体"), QString("六面体")
        };
        for (const auto& task : tasks)
        {
            if (!task.meshShape.isEmpty() && !validShapes.contains(task.meshShape))
            {
                error = QString("任务 %1 的 meshShape 无效：%2（可选值：三角形/四边形主导/全四边形/四面体/六面体）")
                    .arg(task.taskId).arg(task.meshShape);
                return false;
            }

            for (const QString& folderPath : task.folders)
            {
                QFileInfo info(folderPath);
                if (!info.exists() || !info.isDir())
                {
                    error = QString("任务 %1 指定的文件夹不存在或不是目录：%2").arg(task.taskId).arg(folderPath);
                    return false;
                }

                QString ownerTaskId = folderOwner.value(folderPath);
                if (!ownerTaskId.isEmpty() && ownerTaskId != task.taskId)
                {
                    error = QString("文件夹 %1 同时被任务 %2 和任务 %3 指定，每个文件夹只能指定给一个任务")
                        .arg(folderPath).arg(ownerTaskId).arg(task.taskId);
                    return false;
                }

                folderOwner.insert(folderPath, task.taskId);
            }
        }

        return true;
    }
}
