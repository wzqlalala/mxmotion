#pragma once
#pragma execution_character_set("utf-8")

#ifdef MX_MOTION_OCC_COMPAT
#include "picking/OccPickPersistence.h"
#else

#include <QObject>
#include <QString>
#include <QSet>

#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

#include "mPickWidgetData.h"
#include "mSetBase.h"
#include "mMapRelationEnum.h"

using namespace MechSetData;

namespace mxr
{
    class CCSMRendWidget;
}

namespace MechSetData
{
    class mMixSet;
}

namespace MechMainWindow
{
    /**
     * @brief 通用拾取控件持久化策略接口
     * 职责：在 Controller 触发 commit 时，将最终合并数据写入业务模型。
     */
    class IPickPersistenceStrategy
    {
    public:
        virtual ~IPickPersistenceStrategy() = default;

        /**
         * @brief 保存数据
         * @param data 最终合并后的数据（按 PickDataType 分组）
         * @param loadData 当前载荷/边界数据对象（可选）
         */
        virtual void save(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data) = 0;

        // 单集合无拾取：直接复用该已有集合建立映射（不新建内部 MixSet）
        virtual void saveSingleSet(const QString& setName) {};

        /**
         * @brief 名称/调试用途
         */
        virtual QString strategyName() const { return "IPickPersistenceStrategy"; }
    };

    /**
     * @brief 用于 MechMapRelation 型持久化：单集合 -> 直接映射；多集合/混合 -> 创建 MixSet
     */
    class MapRelationPersistenceStrategy : public IPickPersistenceStrategy
    {
    public:
        MapRelationPersistenceStrategy(
            MxFunctions::PhysicalFieldType fieldType,
            int idProvider,
            MxFunctions::MapRelationType mapType = MxFunctions::MapRelation_None)
            : _fieldType(fieldType)
            , _idProvider(idProvider)
            , _mapType(mapType)
        {}

        void save(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data) override;

        // 单集合无拾取：直接复用该已有集合建立映射（不新建内部 MixSet）
        void saveSingleSet(const QString& setName);

        QString strategyName() const override { return "MapRelationPersistenceStrategy"; }

    private:
        MxFunctions::PhysicalFieldType _fieldType;
        MxFunctions::MapRelationType _mapType;
        int _idProvider;
    };

    /**
     * @brief API 型持久化策略：与 MapRelationPersistenceStrategy 接口一致，
     * 但不直接操作数据与映射，而是根据 _fieldType/_idProvider/_mapType
     * 组装增量参数调用对应模块的 API，并静默记录可回放的 Python 命令。
     * 当前支持：Physical_Contact（contact API）。
     */
    class APIPersistenceStrategy : public IPickPersistenceStrategy
    {
    public:
        APIPersistenceStrategy(
            MxFunctions::PhysicalFieldType fieldType,
            int idProvider,
            MxFunctions::MapRelationType mapType = MxFunctions::MapRelation_None)
            : _fieldType(fieldType)
            , _idProvider(idProvider)
            , _mapType(mapType)
        {}

        void save(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data) override;

        // 单集合无拾取：通过 API 将映射改为引用已有集合
        void saveSingleSet(const QString& setName) override;

        QString strategyName() const override { return "APIPersistenceStrategy"; }

    private:
        // 拾取数据 → "GeoFace:1,2,3;Element:10@1:0" 通用ID字符串
        static QString serializePickData(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data);
        // 根据 _fieldType 组装 {api键, python setter}（contact: master/slave）
        bool resolveFieldMapping(QString& apiKey, QString& pySetter) const;
        // 按 _fieldType 查询业务对象名称（contact: 接触名称）
        QString resolveObjectName() const;

        MxFunctions::PhysicalFieldType _fieldType;
        MxFunctions::MapRelationType   _mapType;
        int                            _idProvider;
    };

    /**
     * @brief 用于连接类数据：将数据直接写入 mConnectorBase 的 mainnodes_/subnodes_ 等
     */
    class DataClassPersistenceStrategy : public IPickPersistenceStrategy
    {
    public:
        using DataWriter = std::function<void(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>&)>;
        // 集合名 -> 类型化拾取数据（与 mPickWidgetController 的 SetResolver 一致）
        using SetResolver = std::function<std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>(const QString&)>;

        explicit DataClassPersistenceStrategy(DataWriter writer, SetResolver setResolver = nullptr)
            : _writer(writer)
            , _setResolver(std::move(setResolver))
        {}

        void save(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data) override
        {
            if (_writer)
                _writer(data);
        }

        // 仅选集合时 commit 走 saveSingleSet 而非 save：
        // 需解析集合后经同一写入通道写入数据类字段，
        // 否则连接数据字段不更新，完整性检查会误报"未选择"。
        void saveSingleSet(const QString& setName) override
        {
            if (_writer && _setResolver)
                _writer(_setResolver(setName));
        }

        QString strategyName() const override { return "DataClassPersistenceStrategy"; }

    private:
        DataWriter _writer;
        SetResolver _setResolver;
    };

    /**
     * @brief 集合面板持久化策略：将拾取数据按集合类型直接写入当前编辑的 mSetBase 派生对象
     * 典型场景：集合面板（mSetPanel），拾取数据即集合本体数据，不走 MechMapRelation 映射。
     */
    class SetDataPersistenceStrategy : public IPickPersistenceStrategy
    {
    public:
        // 集合对象提供器：commit 时现取（面板可能按 ID 重建集合对象，避免裸指针失效）
        using SetProvider = std::function<mSetBase*()>;

        explicit SetDataPersistenceStrategy(SetProvider provider)
            : _provider(std::move(provider))
        {}

        void save(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data) override;

        QString strategyName() const override { return "SetDataPersistenceStrategy"; }

    private:
        SetProvider _provider;
    };

    // 通用：根据集合名获取 mSetBase 指针，供 MapRelationPersistenceStrategy 的 SetResolver 使用。
    mSetBase* resolveSetBaseByName(const QString& setName);

    /**
     * @brief 工厂：创建"单值写入"策略 writer
     * 典型场景：主节点/参考点等只能保存一个拾取对象的数据类字段。
     * @param data 目标数据对象（外部强引用保证生命周期）
     * @param member 指向数据类字段的指针（类型为 pair<PickDataType, RenderPickResultData>）
     * @param emptyValue 空数据时写入的值，默认 {PickDataType::None, -1}
     */
    template <typename DataClass>
    DataClassPersistenceStrategy::DataWriter makeSingleValueWriter(
        std::shared_ptr<DataClass> data,
        std::pair<MxFunctions::PickDataType, RenderPickResultData> DataClass::* member,
        std::pair<MxFunctions::PickDataType, RenderPickResultData> emptyValue =
            { MxFunctions::PickDataType::None, RenderPickResultData(-1, -1, -1) });

    // 通用：把 vector<PickDataType> 形式的拾取 ID 写入数据类（用于旧离散刚体等以 vector<int> 保存的场景）。
    template <typename DataClass>
    DataClassPersistenceStrategy::DataWriter makePickIdVectorWriter(
        std::shared_ptr<DataClass> data,
        std::vector<std::pair<MxFunctions::PickDataType, int>> DataClass::* member);

    /**
     * @brief 工厂：创建"按类型分组写入"策略 writer
     * 典型场景：从节点/作用对象等按 PickDataType 分组存储多个拾取对象的数据类字段。
     * @param data 目标数据对象（外部强引用保证生命周期）
     * @param member 指向数据类字段的指针（类型为 unordered_map<PickDataType, vector<RenderPickResultData>>）
     */
    template <typename DataClass>
    DataClassPersistenceStrategy::DataWriter makeTypedMapWriter(
        std::shared_ptr<DataClass> data,
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> DataClass::* member);
}

// 模板工厂定义必须放在头文件中，确保跨编译单元可实例化。
namespace MechMainWindow
{
    template <typename DataClass>
    DataClassPersistenceStrategy::DataWriter makeSingleValueWriter(
        std::shared_ptr<DataClass> data,
        std::pair<MxFunctions::PickDataType, RenderPickResultData> DataClass::* member,
        std::pair<MxFunctions::PickDataType, RenderPickResultData> emptyValue)
    {
        return [data, member, emptyValue](
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& src) {
            if (!data) return;
            if (src.empty())
            {
                (data.get()->*member) = emptyValue;
                return;
            }
            for (const auto& pair : src)
            {
                if (!pair.second.empty())
                {
                    (data.get()->*member).first = pair.first;
                    (data.get()->*member).second = pair.second.front();
                    return;
                }
            }
            (data.get()->*member) = emptyValue;
        };
    }

    template <typename DataClass>
    DataClassPersistenceStrategy::DataWriter makeTypedMapWriter(
        std::shared_ptr<DataClass> data,
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> DataClass::* member)
    {
        return [data, member](
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& src) {
            if (!data) return;
            (data.get()->*member).clear();
            for (const auto& pair : src)
            {
                (data.get()->*member)[pair.first] = pair.second;
            }
        };
    }

    template <typename DataClass>
    DataClassPersistenceStrategy::DataWriter makePickIdVectorWriter(
        std::shared_ptr<DataClass> data,
        std::vector<std::pair<MxFunctions::PickDataType, int>> DataClass::* member)
    {
        return [data, member](
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& src) {
            if (!data) return;
            (data.get()->*member).clear();
            for (const auto& pair : src)
            {
                MxFunctions::PickDataType type = pair.first;
                for (const auto& d : pair.second)
                {
                    (data.get()->*member).push_back({ type, d.result_id });
                }
            }
        };
    }
}

#endif // MX_MOTION_OCC_COMPAT
