#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QList>
#include <QString>
#include <QJsonDocument>
#include "mmainwindow_global.h"
#include "mBatchMeshTask.h"

namespace APIServer
{
    class mAPISchedulingServer;
}

namespace MGUIMesh
{
    /**
     * @brief 批量几何网格剖分 API/脚本入口
     *        负责 JSON 任务解析、校验；执行逻辑下沉到 mMeshBatchCLI / mBatchMeshController。
     */
    class MMAINWINDOW_EXPORT mMeshBatchAPI : public QObject
    {
        Q_OBJECT
    public:
        /**
         * @brief 从 JSON 字符串解析任务列表
         */
        static bool parseTasksFromJson(const QString& json, QList<BatchMeshTask>& outTasks, QString& error);

        /**
         * @brief 从 JSON 文档解析任务列表
         */
        static bool parseTasksFromJson(const QJsonDocument& doc, QList<BatchMeshTask>& outTasks, QString& error);

        /**
         * @brief 校验批量任务参数
         */
        static bool validateTasks(const QList<BatchMeshTask>& tasks, QString& error);
    };
}
