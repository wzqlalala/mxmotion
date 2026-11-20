#include "mPostExportAPIRegistrar.h"

#include "mAPIServer.h"
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostResultDataStruct.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPhysicalFieldEnum.h"

#include "mPostReportRenderer.h"
#include "mPostHtmlReportRenderer.h"
#include "mPostWordReportRenderer.h"
#include "mPostPptReportRenderer.h"

// Qt
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QHash>
#include <QStringList>
#include <QPointer>

namespace
{
    PostData::mPostDataBase* getCurrentPostDataBase()
    {
        MechPreData::mViewportModel* currentModel = MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel();
        if (currentModel == nullptr || currentModel->getModelData() == nullptr)
            return nullptr;

        return currentModel->getModelData()->getManager<PostData::mPostDataManager>(MxFunctions::Physical_Post).getPostDatabase();
    }

    int getCurrentActiveFrame(PostData::mPostDataBase* postDB)
    {
        if (postDB == nullptr)
            return -1;

        QList<int> frameIds = postDB->getFrameIds();
        if (!frameIds.isEmpty())
            return frameIds.first();
        return -1;
    }

    bool writePostVariableToText(PostData::mPostDataBase* postDB, const QString& varName,
                                 const QString& compoName, const QString& filePath,
                                 QString& error)
    {
        if (postDB == nullptr)
        {
            error = "post database is null";
            return false;
        }

        int frameId = getCurrentActiveFrame(postDB);
        if (frameId < 0)
        {
            error = "no active frame";
            return false;
        }

        QStringList availableVars = postDB->getVariableByFrameID(frameId);
        if (!availableVars.contains(varName))
        {
            error = QString("variable not found: %1").arg(varName);
            return false;
        }

        QStringList availableCompos = postDB->getVariableCompos(varName);
        if (!availableCompos.isEmpty() && !availableCompos.contains(compoName))
        {
            error = QString("component not found: %1").arg(compoName);
            return false;
        }

        int varType = postDB->getVariableType(varName);
        if (varType != 0 && varType != 1)
        {
            error = QString("unsupported variable type: %1").arg(varType);
            return false;
        }

        PostData::mPostModelData* modelData = postDB->getModelData(frameId).data();
        if (modelData == nullptr)
        {
            error = "model data is null";
            return false;
        }

        int flag = 0;
        QHash<int, double> values = postDB->getResultDataByFrameID(frameId, flag);
        if (values.isEmpty())
        {
            error = "result data is empty";
            return false;
        }

        QFileInfo fileInfo(filePath);
        if (!fileInfo.dir().exists())
            fileInfo.dir().mkpath(".");

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            error = QString("failed to open file: %1").arg(filePath);
            return false;
        }

        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        for (auto it = values.begin(); it != values.end(); ++it)
        {
            stream << it.key() << " " << it.value() << "\n";
        }
        file.close();
        return true;
    }

    QVariant exportPostVariableToTextAPI(const APIServer::APIParams& params, QString& error)
    {
        QString variableName = APIServer::mAPISchedulingServer::getParamValue(params, "variableName").value(0);
        QString componentName = APIServer::mAPISchedulingServer::getParamValue(params, "componentName").value(0);
        QString filePath = APIServer::mAPISchedulingServer::getParamValue(params, "filePath").value(0);

        if (variableName.isEmpty() || filePath.isEmpty())
        {
            error = "missing required parameters";
            return QVariant();
        }

        PostData::mPostDataBase* postDB = getCurrentPostDataBase();
        if (postDB == nullptr)
        {
            error = "no active post database";
            return QVariant();
        }

        bool ok = writePostVariableToText(postDB, variableName, componentName, filePath, error);
        if (!ok)
        {
            return QVariant();
        }

        return QVariant(true);
    }
}

void registerPostExportRenderers()
{
    PostData::mPostReportRendererFactory::instance()->registerRenderer(
        QSharedPointer<PostData::mPostReportRenderer>(new PostData::mPostHtmlReportRenderer()));
    PostData::mPostReportRendererFactory::instance()->registerRenderer(
        QSharedPointer<PostData::mPostReportRenderer>(new PostData::mPostWordReportRenderer()));
    PostData::mPostReportRendererFactory::instance()->registerRenderer(
        QSharedPointer<PostData::mPostReportRenderer>(new PostData::mPostPptReportRenderer()));
}

void registerPostExportAPIs()
{
    registerPostExportRenderers();

    getAPISchedulingServer->registerAPI("exportPostVariableToText",
                                         &exportPostVariableToTextAPI,
                                         "Export post-processing variable to text file");
}

void unregisterPostExportAPIs()
{
    getAPISchedulingServer->unregisterAPI("exportPostVariableToText");
}
