#pragma once
#pragma execution_character_set("utf-8")

#include <QSet>
#include <QString>
#include <QStringList>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <algorithm>

#include "mPickWidgetData.h"

namespace MechMainWindow
{
    /**
     * @brief 通用拾取控件纯数据模型
     * 职责：只管理 selectedSetNames 与 pickedData，不依赖 UI 和视口。
     */
    class mPickWidgetDataModel
    {
    public:
        mPickWidgetDataModel() = default;
        ~mPickWidgetDataModel() = default;

        // ------------------------------------------------------------------
        // 拾取数据操作
        // ------------------------------------------------------------------
        void setPickedData(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data)
        {
            _data.pickedData.clear();
            for (const auto& pair : data)
            {
                appendPickedDataToKey(pair.first, pair.second);
            }
        }

        void appendPickedData(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data)
        {
            for (const auto& pair : data)
            {
                appendPickedDataToKey(pair.first, pair.second);
            }
        }

        void removePickedData(MxFunctions::PickDataType type, const RenderPickResultData& data)
        {
            auto it = _data.pickedData.find(type);
            if (it == _data.pickedData.end())
                return;

            auto& vec = it->second;
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                [&data](const RenderPickResultData& item) {
                    return item.result_id == data.result_id &&
                           item.result_id2 == data.result_id2 &&
                           item.part_id == data.part_id;
                }), vec.end());

            if (vec.empty())
                _data.pickedData.erase(it);
        }

        void clearPickedDataByType(MxFunctions::PickDataType type)
        {
            _data.pickedData.erase(type);
        }

        // ------------------------------------------------------------------
        // 集合选择操作
        // ------------------------------------------------------------------
        void setSelectedSetNames(const QSet<QString>& names)
        {
            _data.selectedSetNames = names;
        }

        void addSelectedSetName(const QString& name)
        {
            _data.selectedSetNames.insert(name);
        }

        void removeSelectedSetName(const QString& name)
        {
            _data.selectedSetNames.remove(name);
        }

        bool hasSelectedSetName(const QString& name) const
        {
            return _data.selectedSetNames.contains(name);
        }

        // ------------------------------------------------------------------
        // 整体操作
        // ------------------------------------------------------------------
        void clear()
        {
            _data.clear();
        }

        void clearPickedData()
        {
            _data.pickedData.clear();
        }

        const PickWidgetData& getAllData() const
        {
            return _data;
        }

        // 说明：集合+拾取的"按真实 PickDataType 分组去重合并"逻辑统一由
        // mPickWidgetController::commit 实现（保留 MixSet 内部真实类型），
        // 此处不再维护第二套规则，避免两处规则不一致。

    private:
        void appendPickedDataToKey(MxFunctions::PickDataType key, const std::vector<RenderPickResultData>& data)
        {
            appendUnique(_data.pickedData[key], data);
        }

        static void appendUnique(
            std::vector<RenderPickResultData>& target,
            const std::vector<RenderPickResultData>& source)
        {
            std::unordered_set<RenderPickResultData, RenderPickResultDataHash, RenderPickResultDataEqual> seen(
                target.begin(), target.end());

            for (const auto& item : source)
            {
                if (seen.insert(item).second)
                {
                    target.push_back(item);
                }
            }
        }

        PickWidgetData _data;
    };
}
