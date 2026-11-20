#include "mGeoPythonAPIRegistrar.h"
#include "mGeoDevPreviewGate.h" //【GUI 门控】直模 V2 依赖未推送 API
//#include <mGeoCommandFactory.h> //【OCC摘除·S10/P0-1 终局】退役头（DirectModelingCommand/枚举经 mDirectModelingController.h 中性获得）
#include <mViewportModelManager.h>
#include "mModelData.h"
#include "mViewportModel.h"
#include "mGeoManager.h"
#include "mDirectModelingController.h"
#include "mGeoServer.h" //【OCC摘除·S10】脚本创建接入引擎造形族（getGEOServer.primitives）
#include <QDir>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>

#include <exception>
#include <memory>

#ifdef HAS_CGM_INTEROP
#ifdef CGM_VERSION_WIN10
#include "CGMDataExchangeWin10.h"
#else
#include "CGMDataExchange.h"
#endif
#endif

#include <mAppConfiguration.h>
#include <qsettings.h>
#include "mxr/view/CCSMRendWidget.h"
#include "mOtherFunction.h"
#include"mAppConfiguration.h"

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
using namespace MechPreData;
namespace MechMainWindow
{
    // 全局单例指针
    static mGeoPythonAPIRegistrar* g_geoPythonAPIRegistrar = nullptr;
    mGeoPythonAPIRegistrar* mGeoPythonAPIRegistrar::instance()
    {
        if (!g_geoPythonAPIRegistrar) {
            g_geoPythonAPIRegistrar = new mGeoPythonAPIRegistrar();
        }
        return g_geoPythonAPIRegistrar;
    }

    //------------------------辅助函数----------------------//

    bool mGeoPythonAPIRegistrar::convertToStepIfNeeded(const QString& fileName,
                                                        QString& outStepFile,
                                                        bool excludeAuxiliaryGeometry)
    {
        outStepFile = fileName;

#if defined(MXGEO_BACKEND_CGM) && MXGEO_BACKEND_CGM
        // CGM 后端：引擎 mCgmDataExchangeAdapter 直读商用 CAD（InterOp
        // Direct Suite），不再做"CAD→临时 STEP→再导入"的两跳转换。
        Q_UNUSED(excludeAuxiliaryGeometry)
#else
        QFileInfo fileInfo(fileName);
        QString suffix = fileInfo.suffix().toLower();
        bool needConvert = false;

#ifdef HAS_CGM_INTEROP
        static const QStringList cgmSuffixes = {
            "catpart", "catproduct", "ipt", "iam",
            "sldprt", "sldasm", "x_t", "x_b",
            "par", "psm"
        };
        needConvert = cgmSuffixes.contains(suffix);
        if (!needConvert && suffix == "prt")
            needConvert = true;
        if (!needConvert && suffix == "asm")
            needConvert = true;

        if (needConvert)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("正在进行CAD文件转换！"));
            QString safeBaseName = fileInfo.baseName();
            QRegularExpression re("\\p{Zs}+");
            safeBaseName.replace(re, "_");
            QString stepDir = fileInfo.absolutePath();
            QString stepBase = QDir(stepDir).filePath(safeBaseName);
            QString stepFile = stepBase + ".step";
            int uniqueIdx = 1;
            while (QFileInfo::exists(stepFile))
            {
                stepFile = stepBase + "_" + MxFunctions::mNumberToString(uniqueIdx++) + ".step";
            }
#ifdef CGM_VERSION_WIN10
            bool ok = CGMDataExchangeWin10::importToStep(fileName, stepFile, excludeAuxiliaryGeometry);
#else
            bool ok = CGMDataExchange::importToStep(fileName, stepFile, excludeAuxiliaryGeometry);
#endif
            if (!ok)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("CAD文件转换失败：%1").arg(fileName));
                return false;
            }
            outStepFile = stepFile;
        }
#endif
#endif
        return true;
    }

    bool ReadSingleValue(
        const APIServer::APIParams& params,
        const QString& key,
        QString& value,
        QString& error,
        bool required = true)
    {
        const QStringList values = params.value(key);
        if (values.isEmpty() && !required)
        {
            value.clear();
            return true;
        }
        if (values.size() != 1 || values.front().trimmed().isEmpty())
        {
            error = QStringLiteral("%1 requires exactly one non-empty value")
                .arg(key);
            return false;
        }
        value = values.front().trimmed();
        return true;
    }

    bool ReadElementId(
        const APIServer::APIParams& params,
        const QString& key,
        int& value,
        QString& error,
        bool required = true)
    {
        QString text;
        if (!ReadSingleValue(params, key, text, error, required))
            return false;
        if (text.isEmpty() && !required)
        {
            value = -1;
            return true;
        }

        bool ok = false;
        value = text.toInt(&ok);
        if (!ok || value < 0)
        {
            error = QStringLiteral("%1 must be a non-negative integer geometry ID")
                .arg(key);
            return false;
        }
        return true;
    }

    /**
     * 读取几何对象 ID，兼容旧版单数参数和新版复数参数。
     *
     * 例如 faceId=12 与 faceIds=[12, 15] 都会转换为统一的 ID 列表。
     * 两种写法不能同时出现，复数数组不能为空，也不允许重复 ID，避免同一个
     * 拓扑对象在一次事务中被重复执行直接建模。
     */
    bool ReadElementIds(
        const APIServer::APIParams& params,
        const QString& singularKey,
        const QString& pluralKey,
        QVector<int>& values,
        QString& error)
    {
        const bool hasSingular = params.contains(singularKey);
        const bool hasPlural = params.contains(pluralKey);
        if (hasSingular == hasPlural)
        {
            error = hasSingular
                ? QStringLiteral("%1 and %2 cannot be specified together")
                .arg(singularKey, pluralKey)
                : QStringLiteral("exactly one of %1 or %2 is required")
                .arg(singularKey, pluralKey);
            return false;
        }

        const QString activeKey = hasSingular ? singularKey : pluralKey;
        const QStringList texts = params.value(activeKey);
        if (texts.isEmpty())
        {
            error = QStringLiteral("%1 requires at least one geometry ID")
                .arg(activeKey);
            return false;
        }
        if (hasSingular && texts.size() != 1)
        {
            error = QStringLiteral("%1 requires exactly one geometry ID")
                .arg(activeKey);
            return false;
        }

        values.clear();
        values.reserve(texts.size());
        QSet<int> uniqueIds;
        for (const QString& textValue : texts)
        {
            const QString text = textValue.trimmed();
            bool ok = false;
            const int id = text.toInt(&ok);
            if (text.isEmpty() || !ok || id < 0)
            {
                error = QStringLiteral(
                    "every value in %1 must be a non-negative integer geometry ID")
                    .arg(activeKey);
                values.clear();
                return false;
            }
            if (uniqueIds.contains(id))
            {
                error = QStringLiteral("%1 contains duplicate geometry ID %2")
                    .arg(activeKey)
                    .arg(id);
                values.clear();
                return false;
            }

            uniqueIds.insert(id);
            values.push_back(id);
        }
        return true;
    }

    /**
    * 读取 JSON 脚本执行器注入的内部上下文。该参数不会出现在公开 Python
    * API 中；为空表示调用方传入的是当前几何 ID，保持原有行为。
    */
    bool ReadOptionalOriginalFaceIdContext(
        const APIServer::APIParams& params,
        QString& contextId,
        QString& error)
    {
        return ReadSingleValue(
            params,
            QStringLiteral("__directModelingOriginalFaceIdContext"),
            contextId,
            error,
            false);
    }

    bool ReadFiniteNumber(
        const APIServer::APIParams& params,
        const QString& key,
        double& value,
        QString& error)
    {
        QString text;
        if (!ReadSingleValue(params, key, text, error))
            return false;

        bool ok = false;
        value = text.toDouble(&ok);
        if (!ok || !std::isfinite(value))
        {
            error = QStringLiteral("%1 must be a finite number").arg(key);
            return false;
        }
        return true;
    }

    QVariant ExecuteDirectModelingCommand(
        const DirectModelingCommand& command,
        QString& error)
    {
        auto* controller = DirectModelingController::getInstance();
        if (!controller)
        {
            error = QStringLiteral("Direct modeling controller is not available");
            return false;
        }
        // Panel 在调用 API 前已经进入直接建模模式；命令行调用则通常从普通
            // 模式开始，并由 executeCommand 临时激活 Controller。只释放本次 API
            // 自己取得的模式，既保证命令行在成功、失败和异常返回时都退出，也不会
            // 让 Panel 点击“确定”后意外离开交互模式。
        struct ScopedTemporaryDirectModelingMode final
        {
            DirectModelingController* controller = nullptr;
            bool exitWhenFinished = false;

            ~ScopedTemporaryDirectModelingMode()
            {
                if (exitWhenFinished && controller && controller->isActive())
                    controller->exitDirectModelingMode(false);
            }
        };
        ScopedTemporaryDirectModelingMode temporaryMode{
            controller, !controller->isActive() };

        if (!controller->executeCommand(command, &error))
        {
            if (error.isEmpty())
                error = QStringLiteral("Direct modeling command failed");
            return false;
        }
        return true;
    }

    //【OCC摘除·S10】从引擎操作结果提取部件 ID：primitives().create* 经 createPart
    // 将 Part 类型引用追加到 generatedObjects 尾部；取不到 Part 引用视为创建失败。
    static int partIdFromEngineResult(const EventDispatcher::mGeoOperationResult& res)
    {
        for (const auto& obj : res.generatedObjects)
        {
            if (obj.id.type == GeoNeutralData::GN_ShapeType::Part && obj.id.id > 0)
            {
                return obj.id.id;
            }
        }
        return 0;
    }

    //-------------------------------API-------------------------------//

    QVariant mGeoPythonAPIRegistrar::onCreateCubeAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList origin = params.value("origin");
        QStringList size = params.value("size");
        if (origin.size() != 3 || size.size() != 3)
        {
            error = QStringLiteral("createCube requires origin[3] and size[3]");
            return false;
        }

        float x = origin[0].toFloat();
        float y = origin[1].toFloat();
        float z = origin[2].toFloat();
        float length = size[0].toFloat();
        float width = size[1].toFloat();
        float height = size[2].toFloat();

        float vertexA[3] = { x, y, z };
        float vertexC[3] = { x + length, y + width, z + height };
        float NVec[3] = { 0.0f, 0.0f, 1.0f };
        float distance = height;

        auto modelData = getCVM->getModelData();
        if (!modelData)
        {
            error = QStringLiteral("createCube: model data is not ready");
            return false;
        }
        //【OCC摘除·S10】脚本创建接入引擎造形族（S9 退役旧命令链后的恢复）：
        // getGEOServer.primitives().createBox 的 way2（法向+对角点+拉伸距离）与旧
        // CreateSolidCube(NVec, vertexA, vertexC, distance) 语义一致；成功后经
        // createGeoModelSignal_2 走新轨注册（mMeshData/渲染/模型树，与特征面板同轨）
        EventDispatcher::mGeoBoxRequest req;
        req.way = EventDispatcher::createBoxWay::DiagonalAndExtrusion;
        req.vec[0] = NVec[0]; req.vec[1] = NVec[1]; req.vec[2] = NVec[2];
        req.vA[0] = vertexA[0]; req.vA[1] = vertexA[1]; req.vA[2] = vertexA[2];
        req.vC[0] = vertexC[0]; req.vC[1] = vertexC[1]; req.vC[2] = vertexC[2];
        req.distance = distance;
        req.name = "Cube";
        EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createBox(req);
        int partId = partIdFromEngineResult(res);
        if (!res.succeeded() || partId <= 0)
        {
            error = QStringLiteral("createCube: geometry creation failed");
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("立方体创建失败！"));
            return false;
        }
        modelData->getGeoManager()->registerCreatedPart(partId);
        return true;
    }

    QVariant mGeoPythonAPIRegistrar::onCreateCylinderAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList origin = params.value("origin");
        QStringList radiusList = params.value("radius");
        QStringList heightList = params.value("height");
        if (origin.size() != 3 || radiusList.size() != 1 || heightList.size() != 1)
        {
            error = QStringLiteral("createCylinder requires origin[3], radius and height");
            return false;
        }

        float x = origin[0].toFloat();
        float y = origin[1].toFloat();
        float z = origin[2].toFloat();
        float radius = radiusList[0].toFloat();
        float height = heightList[0].toFloat();

        float NVec[3] = { 0.0f, 0.0f, 1.0f };
        float circlePnt[3] = { x, y, z };

        auto modelData = getCVM->getModelData();
        if (!modelData)
        {
            error = QStringLiteral("createCylinder: model data is not ready");
            return false;
        }
        //【OCC摘除·S10】脚本创建接入引擎造形族（语义同旧 CreateSolidColumn）
        EventDispatcher::mGeoCylinderRequest req;
        req.NVec[0] = NVec[0]; req.NVec[1] = NVec[1]; req.NVec[2] = NVec[2];
        req.circlePnt[0] = circlePnt[0]; req.circlePnt[1] = circlePnt[1]; req.circlePnt[2] = circlePnt[2];
        req.radius = radius;
        req.distance = height;
        req.name = "Cylinder";
        EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createCylinder(req);
        int partId = partIdFromEngineResult(res);
        if (!res.succeeded() || partId <= 0)
        {
            error = QStringLiteral("createCylinder: geometry creation failed");
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆柱创建失败！"));
            return false;
        }
        modelData->getGeoManager()->registerCreatedPart(partId);
        return true;
    }
    QVariant mGeoPythonAPIRegistrar::onCreateSphereAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList origin = params.value("origin");
        QStringList radiusList = params.value("radius");
        if (origin.size() != 3 || radiusList.size() != 1)
        {
            error = QStringLiteral("createSphere requires origin[3] and radius");
            return false;
        }

        float x = origin[0].toFloat();
        float y = origin[1].toFloat();
        float z = origin[2].toFloat();
        float radius = radiusList[0].toFloat();

        float NVec[3] = { 0.0f, 0.0f, 1.0f };
        float circlePnt[3] = { x, y, z };

        auto modelData = getCVM->getModelData();
        if (!modelData)
        {
            error = QStringLiteral("createSphere: model data is not ready");
            return false;
        }
        //【OCC摘除·S10】脚本创建接入引擎造形族（语义同旧 CreateSolidGlobe）
        EventDispatcher::mGeoSphereRequest req;
        req.NVec[0] = NVec[0]; req.NVec[1] = NVec[1]; req.NVec[2] = NVec[2];
        req.circlePnt[0] = circlePnt[0]; req.circlePnt[1] = circlePnt[1]; req.circlePnt[2] = circlePnt[2];
        req.radius = radius;
        req.name = "Sphere";
        EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createSphere(req);
        int partId = partIdFromEngineResult(res);
        if (!res.succeeded() || partId <= 0)
        {
            error = QStringLiteral("createSphere: geometry creation failed");
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("球体创建失败！"));
            return false;
        }
        modelData->getGeoManager()->registerCreatedPart(partId);
        return true;
    }

    QVariant mGeoPythonAPIRegistrar::onCreateConeAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList origin = params.value("origin");
        QStringList radiusList = params.value("radius");
        QStringList heightList = params.value("height");
        if (origin.size() != 3 || radiusList.size() != 1 || heightList.size() != 1)
        {
            error = QStringLiteral("createCone requires origin[3], radius and height");
            return false;
        }

        float x = origin[0].toFloat();
        float y = origin[1].toFloat();
        float z = origin[2].toFloat();
        float radius = radiusList[0].toFloat();
        float height = heightList[0].toFloat();

        float NVec[3] = { 0.0f, 0.0f, 1.0f };
        float circlePnt[3] = { x, y, z };

        auto modelData = getCVM->getModelData();
        if (!modelData)
        {
            error = QStringLiteral("createCone: model data is not ready");
            return false;
        }
        //【OCC摘除·S10】脚本创建接入引擎造形族（语义同旧 CreateSolidConoid）
        EventDispatcher::mGeoConeRequest req;
        req.NVec[0] = NVec[0]; req.NVec[1] = NVec[1]; req.NVec[2] = NVec[2];
        req.circlePnt[0] = circlePnt[0]; req.circlePnt[1] = circlePnt[1]; req.circlePnt[2] = circlePnt[2];
        req.radius = radius;
        req.distance = height;
        req.name = "Cone";
        EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createCone(req);
        int partId = partIdFromEngineResult(res);
        if (!res.succeeded() || partId <= 0)
        {
            error = QStringLiteral("createCone: geometry creation failed");
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆锥创建失败！"));
            return false;
        }
        modelData->getGeoManager()->registerCreatedPart(partId);
        return true;
    }

    QVariant mGeoPythonAPIRegistrar::onCreateTorusAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList origin = params.value("origin");
        QStringList bigRadiusList = params.value("bigRadius");
        QStringList smallRadiusList = params.value("smallRadius");
        if (origin.size() != 3 || bigRadiusList.size() != 1 || smallRadiusList.size() != 1)
        {
            error = QStringLiteral("createTorus requires origin[3], bigRadius and smallRadius");
            return false;
        }

        float x = origin[0].toFloat();
        float y = origin[1].toFloat();
        float z = origin[2].toFloat();
        float bigRadius = bigRadiusList[0].toFloat();
        float smallRadius = smallRadiusList[0].toFloat();

        float NVec[3] = { 0.0f, 0.0f, 1.0f };
        float circlePnt[3] = { x, y, z };

        auto modelData = getCVM->getModelData();
        if (!modelData)
        {
            error = QStringLiteral("createTorus: model data is not ready");
            return false;
        }
        //【OCC摘除·S10】脚本创建接入引擎造形族（语义同旧 CreateSolidTorus）
        EventDispatcher::mGeoTorusRequest req;
        req.NVec[0] = NVec[0]; req.NVec[1] = NVec[1]; req.NVec[2] = NVec[2];
        req.circlePnt[0] = circlePnt[0]; req.circlePnt[1] = circlePnt[1]; req.circlePnt[2] = circlePnt[2];
        req.BRadius = bigRadius;
        req.SRadius = smallRadius;
        req.name = "Torus";
        EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createTorus(req);
        int partId = partIdFromEngineResult(res);
        if (!res.succeeded() || partId <= 0)
        {
            error = QStringLiteral("createTorus: geometry creation failed");
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆环创建失败！"));
            return false;
        }
        modelData->getGeoManager()->registerCreatedPart(partId);
        return true;
    }
    QVariant mGeoPythonAPIRegistrar::onCreateFrustumAPI(const APIServer::APIParams& params, QString& error)
    {
        QStringList origin = params.value("origin");
        QStringList bottomRadiusList = params.value("bottomRadius");
        QStringList topRadiusList = params.value("topRadius");
        QStringList heightList = params.value("height");
        if (origin.size() != 3 || bottomRadiusList.size() != 1 || topRadiusList.size() != 1 || heightList.size() != 1)
        {
            error = QStringLiteral(
                "createFrustum requires origin[3], bottomRadius, topRadius and height");
            return false;
        }

        float x = origin[0].toFloat();
        float y = origin[1].toFloat();
        float z = origin[2].toFloat();
        float bottomRadius = bottomRadiusList[0].toFloat();
        float topRadius = topRadiusList[0].toFloat();
        float height = heightList[0].toFloat();

        float NVec[3] = { 0.0f, 0.0f, 1.0f };
        float BCircle[3] = { x, y, z };
        float SCircle[3] = { x, y, z + height };

        auto modelData = getCVM->getModelData();
        if (!modelData)
        {
            error = QStringLiteral("createFrustum: model data is not ready");
            return false;
        }
        //【OCC摘除·S10】脚本创建接入引擎造形族（语义同旧 CreateSolidCircularTruncatedCone）
        EventDispatcher::mGeoFrustumRequest req;
        req.NVec[0] = NVec[0]; req.NVec[1] = NVec[1]; req.NVec[2] = NVec[2];
        req.BCircle[0] = BCircle[0]; req.BCircle[1] = BCircle[1]; req.BCircle[2] = BCircle[2];
        req.BRadius = bottomRadius;
        req.SCircle[0] = SCircle[0]; req.SCircle[1] = SCircle[1]; req.SCircle[2] = SCircle[2];
        req.SRadius = topRadius;
        req.distance = height;
        req.name = "Frustum";
        EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createFrustum(req);
        int partId = partIdFromEngineResult(res);
        if (!res.succeeded() || partId <= 0)
        {
            error = QStringLiteral("createFrustum: geometry creation failed");
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆台创建失败！"));
            return false;
        }
        modelData->getGeoManager()->registerCreatedPart(partId);
        return true;
    }

    QVariant mGeoPythonAPIRegistrar::onImportGeoFile(const APIServer::APIParams& params, QString& error)
    {
        QStringList paths = params.value("filePath");
        if (paths.size() != 1 || paths[0].trimmed().isEmpty())
        {
            error = QStringLiteral("importGeoFile requires one non-empty filePath");
            return false;
        }

        QString filePath = paths[0];

        // 是否排除辅助曲面、辅助线、标注等无关几何
        bool excludeAuxiliaryGeometry = true;
        QStringList auxFlags = params.value("excludeAuxiliaryGeometry");
        if (!auxFlags.isEmpty())
        {
            QString v = auxFlags.first().trimmed().toLower();
            excludeAuxiliaryGeometry = (v == QStringLiteral("true") ||
                                        v == QStringLiteral("1") ||
                                        v == QStringLiteral("yes"));
        }

        QString importFile;
        if (!convertToStepIfNeeded(filePath, importFile, excludeAuxiliaryGeometry))
        {
            error = QStringLiteral("importGeoFile failed to read or convert: %1")
                .arg(filePath);
            return false;
        }

        if (getCVM == nullptr || getCVM->getModelData() == nullptr ||
            getCVM->getPreRenderWidget() == nullptr)
        {
            error = QStringLiteral("importGeoFile: geometry subsystem is not ready");
            return false;
        }

        auto modeldata = getCVM->getModelData();
        auto geomanager = modeldata->getGeoManager();
        auto renderWidget = getCVM->getPreRenderWidget();
        if (geomanager == nullptr)
        {
            error = QStringLiteral("importGeoFile: geometry manager is not ready");
            return false;
        }

        //加入最近打开
        MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(filePath).absolutePath();
        auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
        QSettings Settings(path, QSettings::IniFormat);
        // 添加一条“最近打开”
        QString file = filePath;
        QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());//当前时间距离 1970-01-01 00:00:00 UTC 的毫秒数（int64）

        Settings.beginGroup("recent");
        for (auto k : Settings.childKeys())
        {
            if (Settings.value(k) == filePath)
                Settings.remove(QString("%1").arg(k));
        }
        Settings.endGroup();
        Settings.setValue(key, file);

        // 必须先连接完成信号再启动导入，避免快速完成时漏掉通知。
        auto importFinishedConnection = std::make_shared<QMetaObject::Connection>();
        *importFinishedConnection = QObject::connect(
            renderWidget,
            &CCSMRendWidget::importGeoModelRenderFinished,
            this,
            [=]() {
                QObject::disconnect(*importFinishedConnection);
                QTimer::singleShot(2000, this, [=]() {
                    QString dir = QDir::currentPath();
                    QString num = key.split('/').last();
                    QString name = dir + "/../../Setting/Recent/" +
                        QFileInfo(importFile).baseName() + num + ".png";
                    QDir().mkpath(dir + "/../../Setting/Recent");

                    if (getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr)
                        getCVM->getPreRenderWidget()->FastCapture(name);

                    auto viewportPrivate = VMManager->getCurrentViewportPrivate();
                    if (viewportPrivate != nullptr)
                    {
                        mBaseRend* rend = viewportPrivate->getRendWidget();
                        if (rend != nullptr)
                        {
                            rend->FitView();
                            VMManager->setCameraSyncTimer(500, 30, [rend]() {
                                emit rend->signal_CameraSync();
                                });
                        }
                    }

                    if (importFile != filePath)
                        QFile::remove(importFile);

                    // 所有当前导入后处理结束后才允许脚本推进下一步骤。
                    mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
                    });
            });

        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        try
        {
            geomanager->slotImportGeoFiles(importFile);
        }
        catch (const std::exception& exception)
        {
            QObject::disconnect(*importFinishedConnection);
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
            error = QStringLiteral("importGeoFile exception: %1").arg(exception.what());
            return false;
        }
        catch (...)
        {
            QObject::disconnect(*importFinishedConnection);
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
            error = QStringLiteral("importGeoFile unknown exception");
            return false;
        }

        return true;
    }

    bool mGeoPythonAPIRegistrar::importGeoFileHeadless(const QString& filePath,
                                                        QString& error,
                                                        bool excludeAuxiliaryGeometry)
    {
        QString importFile;
        if (!convertToStepIfNeeded(filePath, importFile, excludeAuxiliaryGeometry))
        {
            error = QStringLiteral("importGeoFileHeadless failed to read or convert: %1").arg(filePath);
            return false;
        }

        if (getCVM == nullptr || getCVM->getModelData() == nullptr)
        {
            error = QStringLiteral("importGeoFileHeadless: geometry subsystem is not ready");
            return false;
        }

        auto modeldata = getCVM->getModelData();
        auto geomanager = modeldata->getGeoManager();
        if (geomanager == nullptr)
        {
            error = QStringLiteral("importGeoFileHeadless: geometry manager is not ready");
            return false;
        }

        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        bool ok = false;
        try
        {
            // headless 模式下使用同步导入接口，避免依赖渲染完成信号
            ok = geomanager->importGeoFileSync(importFile, QString());
        }
        catch (const std::exception& exception)
        {
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
            error = QStringLiteral("importGeoFileHeadless exception: %1").arg(exception.what());
            return false;
        }
        catch (...)
        {
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
            error = QStringLiteral("importGeoFileHeadless unknown exception");
            return false;
        }
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);

        if (!ok)
        {
            error = QStringLiteral("几何导入失败：%1").arg(importFile);
            return false;
        }

        if (modeldata->getGeoData() == nullptr)
        {
            error = QStringLiteral("几何导入后模型中无几何数据：%1").arg(importFile);
            return false;
        }
        return true;
    }

    QVariant mGeoPythonAPIRegistrar::onDirectModelingOffsetFaceAPI(
        const APIServer::APIParams& params,
        QString& error)
    {
        DirectModelingCommand command;
        command.operation = MGeoCommand::DirectModelingOperation::OffsetFace;
        command.elementDim = 2;
        if (!ReadElementIds(params,
            QStringLiteral("faceId"), QStringLiteral("faceIds"),
            command.primaryElementIds, error)
            || !ReadFiniteNumber(params, QStringLiteral("distance"),
                command.value, error)
            || !ReadOptionalOriginalFaceIdContext(
                    params, command.originalFaceIdContext, error))
        {
            return false;
        }
        return ExecuteDirectModelingCommand(command, error);
    }

    QVariant mGeoPythonAPIRegistrar::onDirectModelingReplaceFaceAPI(
        const APIServer::APIParams& params,
        QString& error)
    {
        DirectModelingCommand command;
        command.operation = MGeoCommand::DirectModelingOperation::ReplaceFace;
        command.elementDim = 2;
        if (!ReadElementIds(params,
            QStringLiteral("sourceFaceId"), QStringLiteral("sourceFaceIds"),
            command.primaryElementIds, error)
            || !ReadElementIds(params,
                QStringLiteral("targetFaceId"), QStringLiteral("targetFaceIds"),
                command.referenceElementIds, error)
            || !ReadFiniteNumber(params, QStringLiteral("offset"),
                command.value, error))
        {
            return false;
        }
        return ExecuteDirectModelingCommand(command, error);
    }

    QVariant mGeoPythonAPIRegistrar::onDirectModelingRotateFaceAPI(
        const APIServer::APIParams& params,
        QString& error)
    {
        //【S11·旋转面】引擎链路已通（轴 = axisEdgeId 或面中轴线），解除 V2 门控。
        DirectModelingCommand command;
        command.operation = MGeoCommand::DirectModelingOperation::RotateFace;
        command.elementDim = 2;
        if (!ReadElementIds(params,
            QStringLiteral("faceId"), QStringLiteral("faceIds"),
            command.primaryElementIds, error)
            || !ReadFiniteNumber(params, QStringLiteral("angle"),
                command.value, error)
            || !ReadElementId(params, QStringLiteral("axisEdgeId"),
                command.referenceElementId, error, false))
        {
            return false;
        }
        return ExecuteDirectModelingCommand(command, error);
    }

    QVariant mGeoPythonAPIRegistrar::onDirectModelingResizeFilletAPI(
        const APIServer::APIParams& params,
        QString& error)
    {
        DirectModelingCommand command;
        QString elementType;
        if (!ReadSingleValue(params, QStringLiteral("elementType"),
            elementType, error)
            || !ReadElementIds(params,
                QStringLiteral("elementId"), QStringLiteral("elementIds"),
                command.primaryElementIds, error)
            || !ReadFiniteNumber(params, QStringLiteral("radius"),
                command.value, error))
        {
            return false;
        }

        elementType = elementType.toLower();
        if (elementType == QStringLiteral("face"))
        {
            command.elementDim = 2;
            command.operation = MGeoCommand::DirectModelingOperation::ResizeFillet;
        }
        else if (elementType == QStringLiteral("edge"))
        {
            command.elementDim = 1;
            command.operation = MGeoCommand::DirectModelingOperation::EdgeFillet;
        }
        else
        {
            error = QStringLiteral("elementType must be 'face' or 'edge'");
            return false;
        }
        return ExecuteDirectModelingCommand(command, error);
    }

    void mGeoPythonAPIRegistrar::registerGeometryAPIs()
    {
        // 注册 API Server 回调：createCube -> onCreateCubeAPI
        getAPISchedulingServer->registerAPI(
            "createCube", this, &mGeoPythonAPIRegistrar::onCreateCubeAPI,
            "创建立方体", QStringList({ "origin", "size" }));
        // 注册 API Server 回调：createCylinder -> onCreateCylinderAPI
        getAPISchedulingServer->registerAPI(
            "createCylinder", this, &mGeoPythonAPIRegistrar::onCreateCylinderAPI,
            "创建圆柱", QStringList({ "origin", "radius", "height" }));
        // 注册 API Server 回调：createSphere -> onCreateSphereAPI
        getAPISchedulingServer->registerAPI(
            "createSphere", this, &mGeoPythonAPIRegistrar::onCreateSphereAPI,
            "创建球体", QStringList({ "origin", "radius" }));
        // 注册 API Server 回调：createCone -> onCreateConeAPI
        getAPISchedulingServer->registerAPI(
            "createCone", this, &mGeoPythonAPIRegistrar::onCreateConeAPI,
            "创建圆锥", QStringList({ "origin", "radius", "height" }));
        // 注册 API Server 回调：createTorus -> onCreateTorusAPI
        getAPISchedulingServer->registerAPI(
            "createTorus", this, &mGeoPythonAPIRegistrar::onCreateTorusAPI,
            "创建圆环", QStringList({ "origin", "bigRadius", "smallRadius" }));
        // 注册 API Server 回调：createFrustum -> onCreateFrustumAPI
        getAPISchedulingServer->registerAPI(
            "createFrustum", this, &mGeoPythonAPIRegistrar::onCreateFrustumAPI,
            "创建圆台",
            QStringList({ "origin", "bottomRadius", "topRadius", "height" }));
        // 注册 API Server 回调：importGeoFile -> onImportGeoFile
        getAPISchedulingServer->registerAPI(
            "importGeoFile", this, &mGeoPythonAPIRegistrar::onImportGeoFile,
            "导入几何模型", QStringList({ "filePath"}));

        getAPISchedulingServer->registerAPI(
            "directModelingOffsetFace",
            this, &mGeoPythonAPIRegistrar::onDirectModelingOffsetFaceAPI,
            "直接建模-偏置面",
            QStringList({"faceIds", "distance" }));

        getAPISchedulingServer->registerAPI(
            "directModelingReplaceFace", this,
            &mGeoPythonAPIRegistrar::onDirectModelingReplaceFaceAPI,
            "直接建模-替换面",
            QStringList({ "sourceFaceIds","targetFaceIds","offset"}));


        getAPISchedulingServer->registerAPI(
            "directModelingRotateFace", this,
            &mGeoPythonAPIRegistrar::onDirectModelingRotateFaceAPI,
            "直接建模-旋转面",
            QStringList({ "faceIds", "angle"}));


        getAPISchedulingServer->registerAPI(
            "directModelingResizeFillet", this,
            &mGeoPythonAPIRegistrar::onDirectModelingResizeFilletAPI,
            "直接建模-调整圆角",
            QStringList({ "elementIds","elementType", "radius" }));
    }

    void mGeoPythonAPIRegistrar::unregisterGeometryAPIs()
    {
        getAPISchedulingServer->unregisterAPI("createCube");
        getAPISchedulingServer->unregisterAPI("createCylinder");
        getAPISchedulingServer->unregisterAPI("createSphere");
        getAPISchedulingServer->unregisterAPI("createCone");
        getAPISchedulingServer->unregisterAPI("createTorus");
        getAPISchedulingServer->unregisterAPI("createFrustum");
        getAPISchedulingServer->unregisterAPI("importGeoFile");
        getAPISchedulingServer->unregisterAPI("directModelingOffsetFace");
        getAPISchedulingServer->unregisterAPI("directModelingReplaceFace");
        getAPISchedulingServer->unregisterAPI("directModelingRotateFace");
        getAPISchedulingServer->unregisterAPI("directModelingResizeFillet");
    }
}
