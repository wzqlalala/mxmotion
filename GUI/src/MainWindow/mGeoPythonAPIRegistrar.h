#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include <QObject>
#include "mAPIServer.h"

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mGeoPythonAPIRegistrar : public QObject
    {

        Q_OBJECT

    public:
        static mGeoPythonAPIRegistrar* instance();

        mGeoPythonAPIRegistrar(const mGeoPythonAPIRegistrar&) = delete;
        mGeoPythonAPIRegistrar& operator=(const mGeoPythonAPIRegistrar&) = delete;

        QVariant onCreateCubeAPI(const APIServer::APIParams& params, QString& error);

        QVariant onCreateCylinderAPI(const APIServer::APIParams& params, QString& error);

        QVariant onCreateSphereAPI(const APIServer::APIParams& params, QString& error);

        QVariant onCreateConeAPI(const APIServer::APIParams& params, QString& error);

        QVariant onCreateTorusAPI(const APIServer::APIParams& params, QString& error);

        QVariant onCreateFrustumAPI(const APIServer::APIParams& params, QString& error);

        QVariant onImportGeoFile(const APIServer::APIParams& params, QString& error);

        /**
         * @brief headless 模式下的几何导入，不依赖 PreRenderWidget。
         * @param excludeAuxiliaryGeometry 是否排除辅助曲面、辅助线、标注等无关几何
         */
        bool importGeoFileHeadless(const QString& filePath, QString& error,bool excludeAuxiliaryGeometry = false);

        /** 偏置面：faceId/faceIds 二选一，distance 必需，expression 可选。 */
        QVariant onDirectModelingOffsetFaceAPI(const APIServer::APIParams& params, QString& error);

        /** 替换面：源面和目标面分别使用单数/复数 ID 二选一，offset 必需。 */
        QVariant onDirectModelingReplaceFaceAPI(const APIServer::APIParams& params, QString& error);

        /** 旋转面：faceId/faceIds 二选一，angle 必需，公共 axisEdgeId 可选。 */
        QVariant onDirectModelingRotateFaceAPI(const APIServer::APIParams& params, QString& error);

        /** 调整圆角：elementId/elementIds 二选一，elementType 和 radius 必需。 */
        QVariant onDirectModelingResizeFilletAPI(const APIServer::APIParams& params, QString& error);


        void registerGeometryAPIs();
        void unregisterGeometryAPIs();

    public:
        /*
        * 如有必要，将商软 CAD 格式转换为 STEP。
        * 该转换函数与 GUI 无关，批量/headless 模式也可复用。
        * @param excludeAuxiliaryGeometry 是否排除辅助曲面、辅助线、标注等无关几何
        */
        static bool convertToStepIfNeeded(const QString& fileName,
                                   QString& outStepFile,
                                   bool excludeAuxiliaryGeometry = false); //【GUI修复】改 static：mMenuWidget 以类名调用

    private:
        mGeoPythonAPIRegistrar() = default;
    };
}
