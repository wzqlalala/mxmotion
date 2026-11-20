#include "mMeshPythonAPIRegistrar.h"
#include"mAPISchedulingServer.h"

//#include"MeshMessage.h"
#include"mViewportModelManager.h"
#include "mGeoManager.h"
#include"mViewportModel.h"

#include "mMeshPanelManager.h"
#include "mDataElement.h"
#include "mElementBase.h"
#include <QMessageBox>
#include "mMechGlobalSignals.h"
#include "GlobalMeshMessage.h"
#include "mModelData.h"
#include "MXMesh.h"
//using namespace MDatabase;
//using namespace MMeshView;
using namespace MeshPanelData;
using namespace MechElementData;
using namespace MechModelData;
using namespace MechMainWindow;

namespace MechMainWindow
{
    // 全局单例指针
    static mMeshPythonAPIRegistrar* g_meshPythonAPIRegistrar = nullptr;
    mMeshPythonAPIRegistrar* mMeshPythonAPIRegistrar::instance()
    {
        if (!g_meshPythonAPIRegistrar) {
            g_meshPythonAPIRegistrar = new mMeshPythonAPIRegistrar();
        }
        return g_meshPythonAPIRegistrar;
    }
    QVariant mMeshPythonAPIRegistrar::onMeshGenerateAPI(const APIServer::APIParams& params, QString& error)
    {
        Q_UNUSED(error)
            QStringList factor = params.value("factor");
        if (factor.size() != 4)
        {
            return false;
        }
        int dim = factor[0].toInt();
        int meshType = factor[1].toInt();
        float meshSize = factor[2].toFloat();
        int meshOrder = factor[3].toInt();
        QString shape;
        if (meshType == 1)
        {
            shape = "三角形";
        }
        else if(meshType==2)
        {
            shape = "四边形";
        }
        else if(meshType ==3)
        {
            shape = "四面体";
        }

        auto getMeshShape = [&](const QString& shape, int dim, MeshShapeType& current_type)
            {
                switch (dim)
                {
                    // 二维网格
                case 2:
                {
                    if (strcmp(shape.toStdString().c_str(), "三角形") == 0)
                    {
                        current_type = MeshShapeType::TRI_MESH;
                    }
                    else if (strcmp(shape.toStdString().c_str(), "四边形主导") == 0)
                    {
                        current_type = MeshShapeType::QUAD_DOMINT_MESH;
                    }
                    else if (strcmp(shape.toStdString().c_str(), "全四边形") == 0)
                    {
                        current_type = MeshShapeType::QUAD_MESH;
                    }

                }break;

                // 三维网格
                case 3:
                {
                    if (strcmp(shape.toStdString().c_str(), "四面体") == 0)
                    {
                        current_type = MeshShapeType::TET_MESH;
                    }
                    else if (strcmp(shape.toStdString().c_str(), "六面体") == 0)
                    {
                        current_type = MeshShapeType::HEX_MESH;
                    }
                }break;

                default:break;
                }
            };
        std::unordered_map<uint, MeshShapeType>mesh_shape_type;
        MeshShapeType current_type;
        getMeshShape(shape, dim, current_type);

        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();
        MeshEntity* mesh_entity = meshdata->getMeshEntity();

        set<int> meshGeoId;
        if (dim == 2)
        {
            for (auto ita : mesh_entity->getGeoFaces())
            {
                meshGeoId.insert(ita->getGeoTag());
            }
        }
        else if (dim == 3)
        {
            for (auto ita : mesh_entity->getGeoSolids())
            {
                meshGeoId.insert(ita->getGeoTag());
            }
        }
        for (auto mesh_geo_id : meshGeoId)
        {
            mesh_shape_type[mesh_geo_id] = current_type;
        }
        std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();

        mesh_entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
        mesh_entity->resetPreMeshSize(dim, MAX_LC);   // 重置尺寸！！！！
        mesh_entity->setPreMeshSize(dim, meshSize);
        mesh->saveMeshShape(dim, mesh_shape_type);
        mesh->setMeshData(meshdata);
        std::set<int> toggleEdgeIds;
        mesh->MXMeshInterface(dim, mesh_entity, meshGeoId, toggleEdgeIds);  // 真正剖分
        meshdata->ensureMesh();//网格剖分后里存底层数据的接口
        if (meshOrder == 2)
        {
            vector<int> partIds = meshdata->getAllPartIds();
            std::set<QString> partNames;
            for (int pid : partIds)
            {
                partNames.insert(meshdata->getPartNameByPartId(pid));
            }

            std::set<int> oldIds, newIds;
            meshdata->setMeshSecond(partNames, oldIds, newIds);

            if (!oldIds.empty() || !newIds.empty())
            {
                mGlobalSignals::getInstance()->MeshOrderChangeSig(oldIds, newIds);
            }

        }

        // 默认赋予单元：直接从 meshdata 获取所有元素并分类
                // 避免依赖未发射的 assignElemnetTypeBymeshEdit 信号
        std::vector<int> beamIds, triIds, quadIds, tetIds, wedgeIds, hexIds;
        std::vector<RenderPickResultData> allElements = meshdata->getAllElements();
        for (const auto& ele : allElements)
        {
            int eleId = ele.result_id;
            MeshType meshType = meshdata->getMeshTypeByEleId(eleId);
            switch (meshType)
            {
            case MeshBeam: beamIds.push_back(eleId); break;
            case MeshTri:  triIds.push_back(eleId);  break;
            case MeshQuad: quadIds.push_back(eleId); break;
            case MeshTet:  tetIds.push_back(eleId);  break;
            case MeshWedge: wedgeIds.push_back(eleId); break;
            case MeshHex:  hexIds.push_back(eleId);  break;
            default: break;
            }
        }
        //赋予默认单元
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        ElementType elementType;
        for (auto elementsid : tetIds)
        {
            elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshTet, meshOrder);
            elementData->assignElementTypeToMesh(elementsid, elementType);
        }
        for (auto elementsid : hexIds)
        {
            elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshHex, meshOrder);
            elementData->assignElementTypeToMesh(elementsid, elementType);
        }
        for (auto elementsid : wedgeIds)
        {
            elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshWedge, meshOrder);
            elementData->assignElementTypeToMesh(elementsid, elementType);
        }
        for (auto elementsid : triIds)
        {
            elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshTri, meshOrder);
            mElementBase* elementBase = elementData->getElementDataByMeshID(elementsid);
            mShellElement* element = nullptr;
            if (elementBase != nullptr && elementBase->getElementType() == elementType)
            {
                element = dynamic_cast<mShellElement*>(elementBase);
                element->setElementType(elementType);
            }
            if (elementBase == nullptr || element == nullptr || elementBase->getElementType() != elementType)
            {
                auto element = new mShellElement(elementType);
                elementData->assignElementToMesh(elementsid, element);
            }
            else
            {
                element;//修改参数
            }

        }
        for (auto elementsid : quadIds)
        {
            elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshQuad, meshOrder);
            mElementBase* elementBase = elementData->getElementDataByMeshID(elementsid);
            mShellElement* element = nullptr;
            if (elementBase != nullptr && elementBase->getElementType() == elementType)
            {
                element = dynamic_cast<mShellElement*>(elementBase);
                element->setElementType(elementType);
            }
            if (elementBase == nullptr || element == nullptr || elementBase->getElementType() != elementType)
            {
                auto element = new mShellElement(elementType);
                elementData->assignElementToMesh(elementsid, element);
            }
            else
            {
                element;//修改参数
            }

        }

        for (auto elementsid : beamIds)
        {
            elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshBeam, meshOrder);
            mElementBase* elementBase = elementData->getElementDataByMeshID(elementsid);
            mBeamElement* element = nullptr;
            if (elementBase != nullptr && elementBase->getElementType() == elementType)
            {
                element = dynamic_cast<mBeamElement*>(elementBase);
                element->setElementType(elementType);
            }
            if (elementBase == nullptr || element == nullptr || elementBase->getElementType() != elementType)
            {
                auto element = new mBeamElement(elementType);
                elementData->assignElementToMesh(elementsid, element);
            }
            else
            {
                element;//修改参数
            }
        }


        return QVariant();
    }
    void mMeshPythonAPIRegistrar::registerMeshAPIs()
    {
        getAPISchedulingServer->registerAPI("meshGenerate", this, &mMeshPythonAPIRegistrar::onMeshGenerateAPI, "网格生成");
    }
    void mMeshPythonAPIRegistrar::unregisterMeshAPIs()
    {
        getAPISchedulingServer->unregisterAPI("meshGenerate");
    }
}

