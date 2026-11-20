#pragma once

#include "mmainwindow_global.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

namespace MechMainWindow
{
    /**
     * @brief 一条由直接建模“确定”操作自动创建的参数。
     *
     * id 由 Controller 根据部件、几何对象和操作类型生成，不对用户开放；name 根据
     * 当前操作中文名称自动生成，用户可修改 name 和 expression。公式引用其他
     * 参数时必须使用“{参数名}”；改名不会改变 id。value 由公式计算得到；
     * valid/errorMessage 同时反映公式计算和
     * Controller 写入的几何历史回放状态。
     */
    struct MMAINWINDOW_EXPORT GeoFeatureParameter
    {
        QString id;
        QString name;
        QString operationName;
        QString expression;
        double value = 0.0;
        bool valid = false;
        QString errorMessage;
    };

    /**
     * @brief 直接建模全局标量参数计算器。
     *
     * 本类不继承 QObject，也不访问几何、面板或数据层。生命周期完全由
     * DirectModelingController 的 unique_ptr 管理。它只负责：
     * 1. 按几何对象键自动新增/更新操作参数；
     * 2. 计算参数依赖关系并检测未知引用、重名和循环依赖；
     * 3. 解析数学、三角、比较、条件和逻辑表达式；
     * 4. 保证对外返回的结果是有限 double。
     */
    class MMAINWINDOW_EXPORT mGeoFeatureParameterCalculator
    {
    public:
        mGeoFeatureParameterCalculator() = default;
        mGeoFeatureParameterCalculator(const mGeoFeatureParameterCalculator&) = default;
        mGeoFeatureParameterCalculator& operator=(const mGeoFeatureParameterCalculator&) = default;
        mGeoFeatureParameterCalculator(mGeoFeatureParameterCalculator&&) noexcept = default;
        mGeoFeatureParameterCalculator& operator=(mGeoFeatureParameterCalculator&&) noexcept = default;
        ~mGeoFeatureParameterCalculator() = default;

        /**
         * 当前参数快照。表达式计算结果会叠加 Controller 写入的几何重建状态；
         * 因此公式正确但对应操作无法回放时，返回项同样会显示为失效。
         */
        QVector<GeoFeatureParameter> parameters() const;

        /**
         * 由 Controller 在所属模型删除后按稳定 id 批量移除参数，返回实际删除数量。
         * 不向参数表用户开放删除操作；保留其他模型的公式并重新求值，因此引用
         * {已删除参数} 的存量参数会显示失效，不会被连带删除或替换为常量。
         */
        int removeParameters(const QSet<QString>& parameterIds);

        /**
         * 按稳定对象键新增或更新参数。
         *
         * objectKey 相同表示当前几何对象已经拥有该操作参数，此时保持参数名不变，
         * 仅更新表达式和计算值。expression 为空时使用 fallbackValue 生成数值表达式。
         * 方法会保存逐行计算状态；返回 false 时调用方不得提交对应几何。
         */
        bool upsertOperationParameter(
            const QString& objectKey,
            const QString& operationName,
            const QString& expression,
            double fallbackValue,
            QString* parameterName = nullptr,
            QString* errorMessage = nullptr);

        /**
         * 按稳定 id 批量更新参数名和公式。
         *
         * 所有改名在同一个旧名称空间中解析，然后一次性将公式中的
         * 完整“{旧参数名}”标记改写为“{新参数名}”，因而支持 A/B 同时互换
         * 名称，也不会误修改名称只是子串相同的其他参数。
         * expressionById 中显式提交的新公式按最终参数名解释，不再参与旧名改写；
         * 未显式修改的存量公式才自动完成“旧名 -> 新名”迁移。
         *
         * nameById/expressionById 应只包含需修改的项。任何 id 不存在、
         * 最终名称非法，或改名时公式的花括号结构不完整，整批修改都不提交。
         * 名称和引用结构合法后，公式求值失败仍会保存用户输入，并将参数
         * 标记为失效，与原 updateExpressions 的行为一致。
         */
        bool updateDefinitions(
            const QHash<QString, QString>& nameById,
            const QHash<QString, QString>& expressionById,
            QString* errorMessage = nullptr);

        /**
         * 只更新已有参数的公式。保留该入口以兼容原调用方，内部转发到
         * updateDefinitions，不提供任何创建或删除入口。
         */
        bool updateExpressions(
            const QHash<QString, QString>& expressionById,
            QString* errorMessage = nullptr);

        /**
         * 按稳定 id 取得参数当前的公式计算值。
         *
         * 该查询只检查公式计算状态，不叠加 Controller 写入的几何回放错误，
         * 因此几何历史可以在修正后再次尝试用同一参数值重建。
         */
        bool parameterValue(
            const QString& parameterId,
            double& value,
            QString* parameterName = nullptr,
            QString* errorMessage = nullptr) const;

        /**
         * 使用当前参数表计算任意操作表达式，可带可选前导 '='。
         * 参数引用必须写成“{参数名}”，函数名和内置常量仍使用原有语法。
         */
        bool evaluateExpression(
            const QString& expression,
            double& value,
            QString* errorMessage = nullptr) const;

        /** 清除上一轮几何重建写入的失效状态，不影响公式和值。 */
        void clearGeometryValidation();

        /**
         * 将已有参数标记为几何失效。参数不存在时返回 false；本接口只修改状态，
         * 不提供参数创建入口，也不会改变其表达式计算值。
         */
        bool setGeometryInvalid(
            const QString& parameterId,
            const QString& errorMessage);

    private:
        static bool evaluateDefinitions(
            QVector<GeoFeatureParameter>& parameters,
            QString* firstError);
        QString createUniqueParameterName(const QString& operationName) const;

        QVector<GeoFeatureParameter> m_parameters;
        // 几何回放错误与表达式错误分开保存，避免重新计算公式时误清除拓扑失效信息。
        QHash<QString, QString> m_geometryErrors;
    };
}
