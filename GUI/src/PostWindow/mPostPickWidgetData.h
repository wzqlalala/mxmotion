#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 复制自 src/MainWindow/mPickWidgetData.h（前处理拾取控件栈，dev_base@8d278d784）
// 移植改动：仅命名空间 MechMainWindow→PostWindow，其余保持一致。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件，不得单侧漂移。
// =============================================================================

// Qt
#include <QSet>
#include <QString>
#include <QStringList>
#include <QMetaType>

// STL
#include <unordered_map>
#include <vector>

// MxFunctions
#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

namespace PostWindow
{
    /**
     * @brief 数据来源类型
     */
    enum class PickDataSource
    {
        Set,  // 来自集合选择
        Pick, // 来自视口拾取
    };

    /**
     * @brief 用于 UI 显示的单条拾取/集合数据项
     */
    struct PickDisplayItem
    {
        MxFunctions::PickDataType type = MxFunctions::PickDataType::None;   // 拾取数据类型
        RenderPickResultData data;                                          // 具体数据（id、part_id、sub_id）
        QString sourceName;                                                  // 集合名称，若来源为 Pick 则为空
        PickDataSource source = PickDataSource::Pick;                        // 数据来源

        PickDisplayItem() = default;
        PickDisplayItem(MxFunctions::PickDataType t, const RenderPickResultData& d, const QString& name = "", PickDataSource src = PickDataSource::Pick)
            : type(t), data(d), sourceName(name), source(src) {}
    };

    inline bool operator==(const PickDisplayItem& a, const PickDisplayItem& b)
    {
        return a.type == b.type && a.data.result_id == b.data.result_id &&
               a.data.result_id2 == b.data.result_id2 && a.data.part_id == b.data.part_id &&
               a.sourceName == b.sourceName && a.source == b.source;
    }

    inline bool operator!=(const PickDisplayItem& a, const PickDisplayItem& b)
    {
        return !(a == b);
    }

    /**
     * @brief 通用拾取控件内部数据聚合
     */
    struct PickWidgetData
    {
        QSet<QString> selectedSetNames;                                                  // 当前选中的集合名称集合
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> pickedData; // 视口拾取数据

        void clear()
        {
            selectedSetNames.clear();
            pickedData.clear();
        }

        bool isEmpty() const
        {
            // 选中集合非空 → 整体非空
            if (!selectedSetNames.isEmpty())
            {
                return false;
            }
            // 遍历所有类型的拾取数据，只要有一个vector非空 → 整体非空
            for (const auto& item : pickedData)
            {
                if (!item.second.empty())
                {
                    return false;
                }
            }
            // 选中集合为空，且所有vector都为空 → 整体为空
            return true;
        }
    };

    // ------------------------------------------------------------------
    // RenderPickResultData 的哈希/相等辅助（供 std::unordered_set 使用）
    // ------------------------------------------------------------------
    struct RenderPickResultDataHash
    {
        std::size_t operator()(const RenderPickResultData& d) const noexcept
        {
            return std::hash<int>{}(d.result_id) ^
                   (std::hash<int>{}(d.result_id2) << 1) ^
                   (std::hash<int>{}(d.part_id) << 2);
        }
    };

    struct RenderPickResultDataEqual
    {
        bool operator()(const RenderPickResultData& a, const RenderPickResultData& b) const noexcept
        {
            return a.result_id == b.result_id &&
                   a.result_id2 == b.result_id2 &&
                   a.part_id == b.part_id;
        }
    };

    // ------------------------------------------------------------------
    // 类型转换辅助函数
    // ------------------------------------------------------------------
    inline MxFunctions::PickDataType SetTypeToPickDataType(MxFunctions::SetType type)
    {
        switch (type)
        {
        case MxFunctions::GeoPoint:     return MxFunctions::PickDataType::GeoPoint;
        case MxFunctions::GeoEdge:        return MxFunctions::PickDataType::GeoEdge;
        case MxFunctions::GeoFace:        return MxFunctions::PickDataType::GeoFace;
        case MxFunctions::GeoSolid:      return MxFunctions::PickDataType::GeoSolid;
        case MxFunctions::Node:           return MxFunctions::PickDataType::Node;
        case MxFunctions::Element:      return MxFunctions::PickDataType::Element;
        case MxFunctions::ElementSurface: return MxFunctions::PickDataType::ElementFace;
        case MxFunctions::ElementEdge:    return MxFunctions::PickDataType::ElementEdge;
        case MxFunctions::Part:           return MxFunctions::PickDataType::Part;
        default:                         return MxFunctions::PickDataType::None;
        }
    }

    inline MxFunctions::SetType PickDataTypeToSetType(MxFunctions::PickDataType type)
    {
        switch (type)
        {
        case MxFunctions::PickDataType::GeoPoint:     return MxFunctions::GeoPoint;
        case MxFunctions::PickDataType::GeoEdge:        return MxFunctions::GeoEdge;
        case MxFunctions::PickDataType::GeoFace:        return MxFunctions::GeoFace;
        case MxFunctions::PickDataType::GeoSolid:      return MxFunctions::GeoSolid;
        case MxFunctions::PickDataType::Node:           return MxFunctions::Node;
        case MxFunctions::PickDataType::Element:      return MxFunctions::Element;
        case MxFunctions::PickDataType::ElementFace:   return MxFunctions::ElementSurface;
        case MxFunctions::PickDataType::ElementEdge:   return MxFunctions::ElementEdge;
        case MxFunctions::PickDataType::Part:           return MxFunctions::Part;
        default:                                       return MxFunctions::Mix;
        }
    }

    inline QString PickDataTypeToString(MxFunctions::PickDataType type)
    {
        switch (type)
        {
        case MxFunctions::PickDataType::GeoPoint:     return "几何点";
        case MxFunctions::PickDataType::GeoEdge:        return "几何边";
        case MxFunctions::PickDataType::GeoFace:        return "几何面";
        case MxFunctions::PickDataType::GeoSolid:      return "几何实体";
        case MxFunctions::PickDataType::Node:           return "节点";
        case MxFunctions::PickDataType::ElementEdge:    return "单元边";
        case MxFunctions::PickDataType::ElementFace:   return "单元面";
        case MxFunctions::PickDataType::Element:      return "单元";
        case MxFunctions::PickDataType::Part:           return "部件";
        default:                                       return "未知";
        }
    }

    inline QString SetTypeToString(MxFunctions::SetType type)
    {
        return PickDataTypeToString(SetTypeToPickDataType(type));
    }
}

//Q_DECLARE_METATYPE 会在当前命名空间特例化全局模板 QMetaTypeId，
//放在自定义命名空间内 MSVC 接受但 GCC(-fpermissive) 拒绝，需置于全局作用域
Q_DECLARE_METATYPE(PostWindow::PickDisplayItem)
