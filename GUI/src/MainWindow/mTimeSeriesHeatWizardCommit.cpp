#include "mTimeSeriesHeatWizardCommit.h"
#pragma execution_character_set("utf-8")

#include <QHash>
#include <QStringList>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mPhysicalFieldEnum.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mSetManager.h"
#include "mSetBase.h"
#include "mFieldManager.h"
#include "mAnalysisField.h"
#include "mCaseManager.h"
#include "mCaseImplicitTransientHeat.h"
#include "mLoadBoundaryManager.h"
#include "mLoadHeatFlux.h"
#include "mLoadConvection.h"
#include "mLoadRadiation.h"
#include "mBoundaryTempNew.h"
#include "mBoundaryTemp.h"
#include "mSetBase.h"
#include "mMechGlobalSignals.h"
#include "mGlobalSignals.h"

namespace MechMainWindow
{
namespace TimeSeriesHeatWizardCommit
{
    namespace
    {
        /*
        * 创建并登记一个 TextFile 定义模式的分析场，并关联到所选集合
        * locate：点热流/温度边界→Node；面热流/对流/辐射→ElementFace；体热流→Element
        */
        int createAnalysisField(const QString& name, const QString& filePath,
            const QString& setName, MxFunctions::PickDataType locate, QString* error)
        {
            auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
            if (!modelData)
            {
                if (error) *error = QStringLiteral("当前没有可用模型");
                return -1;
            }

            auto& fieldManager = modelData->getManager<MechDiscreteFieldData::mFieldManager>(MxFunctions::Physical_Field);
            const int fieldId = fieldManager.createID();
            auto* field = new MechDiscreteFieldData::mAnalysisField(fieldId, name);
            field->setDefineMethod(MechDiscreteFieldData::mAnalysisField::DefineMethod::TextFile);
            field->setTextFilePath(filePath);
            field->setLocate(locate);
            fieldManager.addFieldData(field);

            // 分析场的作用对象 = 所选集合
            auto& mapRel = modelData->getMapRelationManager();
            int setId = -1;
            if (!setName.isEmpty())
            {
                auto& setManager = modelData->getManager<MechSetData::mSetManager>(MxFunctions::Physical_Set);
                auto* setData = setManager.getSetData(setName);
                if (setData)
                    setId = setData->getSetId();
            }
            mapRel.appendMapRelation({ MxFunctions::Physical_Field, fieldId },
                { MxFunctions::Physical_Set, setId });

            return fieldId;
        }

        /*
        * 创建瞬态传热工况并写入时间步定义（步长已折算为秒），返回工况ID
        * 各工况彼此独立，不设置接续分析步
        */
        int createTransientHeatCase(const TimeSeriesCaseSpec& spec, const TimeSeriesStepSpec& stepSpec)
        {
            auto* modelData = getCVM->getModelData();
            auto& caseManager = modelData->getManager<MechCaseData::mCaseManager>(MxFunctions::Physical_Case);

            const int id = caseManager.createID();
            auto* caseData = new MechCaseData::mCaseImplicitTransientHeat(
                id, MxFunctions::ImplicitTransientHeat, spec.name);
            caseData->_timeStepType = stepSpec.isAuto ? QStringLiteral("自动") : QStringLiteral("固定");
            caseData->_initialStepValue = QString::number(stepSpec.initial);
            caseData->_totolTimeValue = QString::number(spec.duration);
            caseData->_minTimeStep = QString::number(stepSpec.minStep);
            caseData->_maxTimeStep = QString::number(stepSpec.maxStep);
            caseData->_cntnlsub = QStringLiteral("关闭");

            modelData->addPhysicalField<MechCaseData::mCaseBase>(caseData);
            return id;
        }

        /*
        * 将输出设置写入工况：输出文件类型 + “热学”分组下选中的输出变量及其集合
        * 变量名对齐产品热学组（见 mCaseTransientHeatPanel / _bhStringOutput）
        */
        void applyOutputs(int caseId, const TimeSeriesOutputSpec& output)
        {
            auto* modelData = getCVM->getModelData();
            auto& caseManager = modelData->getManager<MechCaseData::mCaseManager>(MxFunctions::Physical_Case);
            auto* caseData = caseManager.getCaseData(caseId);
            if (!caseData)
                return;

            caseData->setOutputType(output.format);

            // 与工况“热学”组一一对应（_bhStringOutput：温度/节点温度梯度/单元温度梯度/节点热流密度）
            static const QHash<QString, QString> variableNames = {
                { QStringLiteral("NT"),     QStringLiteral("温度") },
                { QStringLiteral("GRADT"),  QStringLiteral("节点温度梯度") },
                { QStringLiteral("EGRADT"), QStringLiteral("单元温度梯度") },
                { QStringLiteral("HFL"),    QStringLiteral("节点热流密度") }
            };
            QStringList selected;
            QHash<QString, QStringList> selectedSets;
            for (const QString& solverName : output.variables)
            {
                const QString displayName = variableNames.value(solverName);
                if (displayName.isEmpty())
                    continue;
                selected.append(displayName);
                // 集合名称列表与工况设置面板/导出逻辑保持一致（空=全模型）
                selectedSets.insert(displayName, output.variableSets.value(solverName));
            }
            caseData->setOutputs({ { QStringLiteral("热学"), selected } });
            caseData->setSelectedSet({ { QStringLiteral("热学"), selectedSets } });
        }

        /*
        * 关联载荷/边界到其所属工况
        * 方向与工况树一致：{Physical_LoadBoundary, 载荷id} → {Physical_Case, 工况id}
        */
        void attachToCase(int loadBoundaryId, int caseId)
        {
            auto& mapRel = getCVM->getModelData()->getMapRelationManager();
            mapRel.appendMapRelation({ MxFunctions::Physical_LoadBoundary, loadBoundaryId },
                { MxFunctions::Physical_Case, caseId });
        }

        /*
        * 关联载荷/边界到全部时序工况（边界用：一个边界本体映射所有工况）
        */
        void attachToAllCases(int loadBoundaryId, const QList<int>& caseIds)
        {
            for (int caseId : caseIds)
                attachToCase(loadBoundaryId, caseId);
        }
    }

    bool commit(const QList<TimeSeriesCaseSpec>& cases,
        const TimeSeriesStepSpec& stepSpec,
        const QList<TimeSeriesLoadItem>& loadItems,
        const QList<TimeSeriesBoundaryItem>& boundaryItems,
        const TimeSeriesOutputSpec& outputSpec,
        QString* createdSummary, QString* error)
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (!modelData)
        {
            if (error) *error = QStringLiteral("当前没有可用模型，请先打开或创建模型");
            return false;
        }

        // ===== 1. 工况（瞬态传热） =====
        // 先完成全部对象创建，最后统一发信号刷新界面，避免逐个发信号导致的界面反复刷新卡顿
        QList<int> caseIds;
        for (const TimeSeriesCaseSpec& spec : cases)
        {
            const int caseId = createTransientHeatCase(spec, stepSpec);
            caseIds.append(caseId);
            applyOutputs(caseId, outputSpec);
        }

        auto& loadBoundaryManager = modelData->getManager<MechCaseData::mLoadBoundaryManager>(MxFunctions::Physical_LoadBoundary);
        auto& mapRel = modelData->getMapRelationManager();
        QList<int> createdLoadIds;
        QList<int> createdBoundaryIds;
        QList<int> createdFieldIds;

        // ===== 2. 载荷（全部走分析场模式） =====
        // 每个工况对应一个独立载荷，每个载荷对应一个独立分析场：
        // N 个工况 × M 个载荷 → N×M 个载荷 + N×M 个分析场
        for (const TimeSeriesLoadItem& item : loadItems)
        {
            MxFunctions::PickDataType locate = MxFunctions::PickDataType::ElementFace;
            switch (item.kind)
            {
            case TimeSeriesLoadKind::PointHeatFlux:
                locate = MxFunctions::PickDataType::Node; break;
            case TimeSeriesLoadKind::VolumeHeatFlux:
                locate = MxFunctions::PickDataType::Element; break;
            default: break;
            }

            for (const TimeSeriesCaseSpec& spec : cases)
            {
                // 按工况命名：载荷名_开始_结束 / 载荷名_开始_结束_Field
                const QString suffix = QStringLiteral("_%1_%2")
                    .arg(timeSeriesTimeToken(spec.start), timeSeriesTimeToken(spec.end));
                const QString loadName = item.name + suffix;

                const int fieldId = createAnalysisField(loadName + QStringLiteral("_Field"),
                    item.filePath, item.setName, locate, error);
                if (fieldId < 0)
                    return false;
                createdFieldIds.append(fieldId);

                std::shared_ptr<MechCaseData::mLoadBoundaryBase> load;
                switch (item.kind)
                {
                case TimeSeriesLoadKind::PointHeatFlux:
                case TimeSeriesLoadKind::SurfaceHeatFlux:
                case TimeSeriesLoadKind::VolumeHeatFlux:
                {
                    auto heatFlux = std::make_shared<MechCaseData::mLoadHeatFlux>(
                        loadBoundaryManager.createID(), MxFunctions::LOAD_IM_HEATFLUX, loadName);
                    heatFlux->way_ = item.kind == TimeSeriesLoadKind::PointHeatFlux ? 0
                        : item.kind == TimeSeriesLoadKind::SurfaceHeatFlux ? 1 : 2;
                    heatFlux->defineType_ = 2;   // 分析场
                    load = heatFlux;
                    break;
                }
                case TimeSeriesLoadKind::Convection:
                {
                    auto convection = std::make_shared<MechCaseData::mLoadConvection>(
                        loadBoundaryManager.createID(), MxFunctions::LOAD_IM_CONVECTION, loadName);
                    convection->defineType_ = 3; // 分析场（环境温度、换热系数均由分析场承载）
                    convection->path_ = item.filePath;
                    load = convection;
                    break;
                }
                case TimeSeriesLoadKind::Radiation:
                {
                    auto radiation = std::make_shared<MechCaseData::mLoadRadiation>(
                        loadBoundaryManager.createID(), MxFunctions::LOAD_IM_RADIATION, loadName);
                    radiation->defineType_ = 2;  // 分析场
                    load = radiation;
                    break;
                }
                }

                if (!load)
                {
                    if (error) *error = QStringLiteral("载荷“%1”创建失败").arg(loadName);
                    return false;
                }
                modelData->addPhysicalFieldP(load);
                createdLoadIds.append(load->getLoadBoundaryId());

                // 载荷 → 分析场
                mapRel.appendMapRelation({ MxFunctions::Physical_LoadBoundary, load->getLoadBoundaryId() },
                    { MxFunctions::Physical_Field, fieldId }, MxFunctions::MapRelation_AnalysisField);

                // 载荷 → 所属工况
                attachToCase(load->getLoadBoundaryId(), caseIds.at(spec.no - 1));
            }
        }

        // ===== 3. 边界（温度边界：直接指定温度值，不与工况一一对应创建） =====
        // 每个边界只创建一个本体（mBoundaryTemp，数值模式），映射到全部时序工况
        for (const TimeSeriesBoundaryItem& item : boundaryItems)
        {
            auto boundary = std::make_shared<MechCaseData::mBoundaryTemp>(
                loadBoundaryManager.createID(), MxFunctions::BOUNDARY_IM_TEMP, item.name);
            boundary->_temp = item.temperature;
            boundary->isAnalysisFieldMode_ = false;
            modelData->addPhysicalFieldP(boundary);
            const int boundaryId = boundary->getLoadBoundaryId();
            createdBoundaryIds.append(boundaryId);

            // 边界 → 作用集合（空集合名表示全模型，不写映射）
            if (!item.setName.isEmpty())
            {
                auto& setManager = modelData->getManager<MechSetData::mSetManager>(MxFunctions::Physical_Set);
                auto* setData = setManager.getSetData(item.setName);
                if (setData)
                {
                    mapRel.appendMapRelation({ MxFunctions::Physical_LoadBoundary, boundaryId },
                        { MxFunctions::Physical_Set, setData->getSetId() });
                }
            }

            // 边界 → 全部时序工况
            attachToAllCases(boundaryId, caseIds);
        }

        // ===== 4. 统一刷新模型树 =====
        // 工况/载荷/边界/分析场均已写入 MechMapRelation（_caseToListLoadBC / _discreteFieldMap 等），
        // 整树重建会按 MechMapRelation 恢复各工况下的载荷/边界挂载，避免依赖树选中状态
        // 和逐个插入时的锚点错乱。
        emit MxFunctions::mGlobalSignals::getInstance()->updateModelTree();

        if (createdSummary)
        {
            *createdSummary = QStringLiteral("已创建 %1 个工况、%2 个载荷、%3 个边界、%4 个分析场")
                .arg(caseIds.size()).arg(createdLoadIds.size()).arg(createdBoundaryIds.size()).arg(createdFieldIds.size());        }
        return true;
    }
}
}
