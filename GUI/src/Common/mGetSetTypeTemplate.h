#pragma once
#include "mCommon_global.h"

#include "mPhysicalFieldEnum.h"
#include <QSet>
#include "mBasicEnum.h"

namespace MechCommon
{
    /**
     * @brief 仅拾取节点
     */
    static QSet<MxFunctions::PickDataType> pickOnlyNode()
    {
        return { MxFunctions::PickDataType::Node };
    }

    /**
     * @brief 拾取：几何点、几何线、几何面、几何实体、节点、部件
     */
    static  QSet<MxFunctions::PickDataType> pickGeoPointEdgeFaceSolidNodePart()
    {
        return { MxFunctions::PickDataType::GeoPoint,
                 MxFunctions::PickDataType::GeoEdge,
                 MxFunctions::PickDataType::GeoFace,
                 MxFunctions::PickDataType::GeoSolid,
                 MxFunctions::PickDataType::Node,
                 MxFunctions::PickDataType::Part };
    }

    /**
     * @brief 拾取：几何点、节点
     */
    static  QSet<MxFunctions::PickDataType> pickGeoPointAndNode()
    {
        return { MxFunctions::PickDataType::GeoPoint, MxFunctions::PickDataType::Node };
    }

    /**
     * @brief 拾取：几何面、单元面
     */
    static  QSet<MxFunctions::PickDataType> pickGeoFaceAndElementSurface()
    {
        return { MxFunctions::PickDataType::GeoFace, MxFunctions::PickDataType::ElementFace };
    }

    /**
     * @brief 仅拾取单元面
     */
    static  QSet<MxFunctions::PickDataType> pickOnlyElementSurface()
    {
        return { MxFunctions::PickDataType::ElementFace };
    }

    /**
     * @brief 拾取：几何点/线/面/实体、节点、单元、单元边、单元面、部件
     */
    static  QSet<MxFunctions::PickDataType> pickAllGeoAndMeshItem()
    {
        return { MxFunctions::PickDataType::GeoPoint,
                 MxFunctions::PickDataType::GeoEdge,
                 MxFunctions::PickDataType::GeoFace,
                 MxFunctions::PickDataType::GeoSolid,
                 MxFunctions::PickDataType::Node,
                 MxFunctions::PickDataType::Element,
                 MxFunctions::PickDataType::ElementEdge,
                 MxFunctions::PickDataType::ElementFace,
                 MxFunctions::PickDataType::Part };
    }

    /**
     * @brief 拾取：几何面、单元、单元面
     */
    static  QSet<MxFunctions::PickDataType> pickGeoFaceAndElement()
    {
        return { MxFunctions::PickDataType::GeoFace,
                 MxFunctions::PickDataType::Element,
                 MxFunctions::PickDataType::ElementFace };
    }

    /**
     * @brief 拾取：几何线、几何面、单元边、单元面（用于方向拾取）
     */
    static  QSet<MxFunctions::PickDataType> pickGeoEdgeFaceAndElementEdgeSurface()
    {
        return { MxFunctions::PickDataType::GeoEdge,
                 MxFunctions::PickDataType::GeoFace,
                 MxFunctions::PickDataType::ElementEdge,
                 MxFunctions::PickDataType::ElementFace };
    }

    /**
     * @brief 拾取：几何点、几何线、几何面、几何实体、节点
     */
    static  QSet<MxFunctions::PickDataType> pickGeoPointEdgeFaceSolidAndNode()
    {
        return { MxFunctions::PickDataType::GeoPoint,
                 MxFunctions::PickDataType::GeoEdge,
                 MxFunctions::PickDataType::GeoFace,
                 MxFunctions::PickDataType::GeoSolid,
                 MxFunctions::PickDataType::Node };
    }

    /**
     * @brief 拾取：几何面、几何实体、单元面
     */
    static  QSet<MxFunctions::PickDataType> pickGeoFaceSolidAndElementSurface()
    {
        return { MxFunctions::PickDataType::GeoFace, MxFunctions::PickDataType::GeoSolid, MxFunctions::PickDataType::ElementFace };
    }

    /**
     * @brief 拾取：几何面、几何实体、单元
     */
    static  QSet<MxFunctions::PickDataType> pickGeoFaceSolidAndElement()
    {
        return { MxFunctions::PickDataType::GeoFace, MxFunctions::PickDataType::GeoSolid, MxFunctions::PickDataType::Element };
    }

    /**
     * @brief 仅拾取单元
     */
    static  QSet<MxFunctions::PickDataType> pickOnlyElement()
    {
        return { MxFunctions::PickDataType::Element };
    }

    /**
     * @brief 拾取：几何线、几何面、单元（一维螺栓场景，上层需过滤单元维度）
     */
    static  QSet<MxFunctions::PickDataType> pickGeoEdgeFaceAndElement()
    {
        return { MxFunctions::PickDataType::GeoEdge, MxFunctions::PickDataType::GeoFace, MxFunctions::PickDataType::Element };
    }

    /**
    * @brief 拾取：几何线、几何面、单元边(部件、几何实体 暂时不需要)
    */
    static  QSet<MxFunctions::PickDataType> pickGeoEdgeFaceSolidAndElement()
    {
        return { MxFunctions::PickDataType::GeoEdge, MxFunctions::PickDataType::GeoFace,MxFunctions::PickDataType::ElementEdge/*,MxFunctions::PickDataType::GeoSolid,MxFunctions::PickDataType::Part*/};
    }
}