#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include <QObject>
#include <unordered_map>
#include <vector>
#include "mAPIServer.h"
#include "mBasicEnum.h"
#include "mMapRelationEnum.h"
#include "mModelStruct.h"

namespace MechMainWindow
{
    /**
     * @brief 接触 API 注册器
     * 统一管理接触相关的 API 注册/注销与实现，与 mPythonCommand 解耦。
     * 采用与 mGeoPythonAPIRegistrar 相同的单例模式。
     */
    class MMAINWINDOW_EXPORT mContactAPIRegistrar : public QObject
    {
        Q_OBJECT

    public:
        static mContactAPIRegistrar* instance();

        mContactAPIRegistrar(const mContactAPIRegistrar&) = delete;
        mContactAPIRegistrar& operator=(const mContactAPIRegistrar&) = delete;

        /**
         * @brief 接触统一入口（JSON全字符串参数，isCreate 标志位区分创建/更新）
         */
        QVariant onContactAPI(const APIServer::APIParams& params, QString& error);

        /**
         * @brief 通用集合接口：更新或创建指定映射关系的内部集合数据
         * @param contactID 接触ID
         * @param relationType 映射类型（MapRelation_Master/MapRelation_Slave）
         * @param data 集合数据：按 PickDataType 分组的拾取结果，直接对应 mMixSet 数据格式
         * @param error 错误信息出参
         * @return 成功返回 true
         */
        bool updateContactSetData(int contactID, MxFunctions::MapRelationType relationType,
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data,
            QString& error);

        /**
         * @brief 通用集合接口：把指定映射关系改为引用已有集合（而非内部集合）
         * @param contactID 接触ID
         * @param relationType 映射类型
         * @param setName 已有集合名称
         * @param error 错误信息出参
         * @return 成功返回 true
         */
        bool linkContactSet(int contactID, MxFunctions::MapRelationType relationType,
            const QString& setName, QString& error);

        void registerContactAPIs();
        void unregisterContactAPIs();

    private:
        mContactAPIRegistrar() = default;
    };
}
