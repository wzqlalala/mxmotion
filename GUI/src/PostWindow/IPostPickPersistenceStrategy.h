#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 复制自 src/MainWindow/IPickPersistenceStrategy.h（前处理拾取控件栈，dev_base@8d278d784）
// 移植改动：命名空间→PostWindow；删除 MapRelationPersistenceStrategy（前处理
// MechMapRelation/MixSet 业务概念，后处理无此数据链路）；接口与 DataClass 系列写入器
// 保持一致。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件（删除项除外），不得单侧漂移。
// =============================================================================

#include <QObject>
#include <QString>
#include <QSet>

#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

#include "mPostPickWidgetData.h"

namespace PostWindow
{
    /**
     * @brief 通用拾取控件持久化策略接口
     * 职责：在 Controller 触发 commit 时，将最终合并数据写入业务模型。
     */
    class IPostPickPersistenceStrategy
    {
    public:
        virtual ~IPostPickPersistenceStrategy() = default;

        /**
         * @brief 保存数据
         * @param data 最终合并后的数据（按 PickDataType 分组）
         */
        virtual void save(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data) = 0;

        // 单集合无拾取：直接以集合名提交（默认空实现，需要集合语义的策略重写）
        virtual void saveSingleSet(const QString& setName) {};

        /**
         * @brief 名称/调试用途
         */
        virtual QString strategyName() const { return "IPostPickPersistenceStrategy"; }
    };

    /**
     * @brief 用于连接类数据：将数据直接写入业务数据的 mainnodes_/subnodes_ 等字段
     */
    class PostDataClassPersistenceStrategy : public IPostPickPersistenceStrategy
    {
    public:
        using DataWriter = std::function<void(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>&)>;

        explicit PostDataClassPersistenceStrategy(DataWriter writer)
            : _writer(std::move(writer))
        {}

        void save(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data) override
        {
            if (_writer)
                _writer(data);
        }

        QString strategyName() const override { return "PostDataClassPersistenceStrategy"; }

    private:
        DataWriter _writer;
    };

    /**
     * @brief 工厂：创建"单值写入"策略 writer
     * 典型场景：主节点/参考点等只能保存一个拾取对象的数据类字段。
     * @param data 目标数据对象（外部强引用保证生命周期）
     * @param member 指向数据类字段的指针（类型为 pair<PickDataType, RenderPickResultData>）
     * @param emptyValue 空数据时写入的值，默认 {PickDataType::None, -1}
     */
    template <typename DataClass>
    PostDataClassPersistenceStrategy::DataWriter makePostSingleValueWriter(
        std::shared_ptr<DataClass> data,
        std::pair<MxFunctions::PickDataType, RenderPickResultData> DataClass::* member,
        std::pair<MxFunctions::PickDataType, RenderPickResultData> emptyValue =
            { MxFunctions::PickDataType::None, RenderPickResultData(-1, -1, -1) });

    // 通用：把 vector<PickDataType> 形式的拾取 ID 写入数据类（用于以 vector<int> 保存的场景）。
    template <typename DataClass>
    PostDataClassPersistenceStrategy::DataWriter makePostPickIdVectorWriter(
        std::shared_ptr<DataClass> data,
        std::vector<std::pair<MxFunctions::PickDataType, int>> DataClass::* member);

    /**
     * @brief 工厂：创建"按类型分组写入"策略 writer
     * 典型场景：从节点/作用对象等按 PickDataType 分组存储多个拾取对象的数据类字段。
     * @param data 目标数据对象（外部强引用保证生命周期）
     * @param member 指向数据类字段的指针（类型为 unordered_map<PickDataType, vector<RenderPickResultData>>）
     */
    template <typename DataClass>
    PostDataClassPersistenceStrategy::DataWriter makePostTypedMapWriter(
        std::shared_ptr<DataClass> data,
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> DataClass::* member);
}

// 模板工厂定义必须放在头文件中，确保跨编译单元可实例化。
namespace PostWindow
{
    template <typename DataClass>
    PostDataClassPersistenceStrategy::DataWriter makePostSingleValueWriter(
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
    PostDataClassPersistenceStrategy::DataWriter makePostTypedMapWriter(
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
    PostDataClassPersistenceStrategy::DataWriter makePostPickIdVectorWriter(
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
