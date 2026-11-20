#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mTimeSeriesHeatWizardTypes.h"

#include <QList>
#include <QString>

namespace MechMainWindow
{
    /*
    * 时序热分析向导落库：点“完成”时统一创建全部模型对象。
    *
    * 创建内容：
    * - N 个瞬态传热工况（mCaseImplicitTransientHeat，彼此独立、不接续）；
    * - 每个载荷 × 每个工况：一份独立的载荷本体 + 一份独立的
    *   mAnalysisField（TextFile 定义模式、来源于文件、作用对象=所选集合）；
    * - 每个边界：单个 mBoundaryTemp（直接指定温度值，不走分析场），
    *   映射到全部时序工况；非空集合时写入边界→集合映射；
    * - MechMapRelation 关联：载荷→分析场、分析场→集合、载荷→其所属工况、
    *   边界→全部工况。
    */
    namespace TimeSeriesHeatWizardCommit
    {
        /*
        * 执行落库。成功返回 true 并通过 createdSummary 返回摘要；
        * 失败返回 false 并通过 error 返回原因。
        */
        MMAINWINDOW_EXPORT bool commit(const QList<TimeSeriesCaseSpec>& cases,
            const TimeSeriesStepSpec& stepSpec,
            const QList<TimeSeriesLoadItem>& loadItems,
            const QList<TimeSeriesBoundaryItem>& boundaryItems,
            const TimeSeriesOutputSpec& outputSpec,
            QString* createdSummary, QString* error);
    }
}
