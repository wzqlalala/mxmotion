#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mTimeSeriesHeatWizardTypes.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace MechMainWindow
{
    /*
    * 时序文件 / 载荷集文件解析结果
    */
    struct TimeSeriesParseResult
    {
        bool ok = false;
        QString error;                  // 错误描述（含行号定位）
        QList<QStringList> rows;        // 数据行（Tab分列）
        QList<QPair<double, double>> timeRanges;    // 每行 开始/结束时间
    };

    /*
    * 时序热分析工况向导：文本文件解析
    *
    * 文件规范（见开发说明文档 §3）：
    * - UTF-8 文本、制表符分列、首行为表头；
    * - 前两列表头固定为 开始时间_s / 结束时间_s（需校验）；
    * - 点/面/体热流：第3列场文件（表头「场文件」，且引用的文件必须存在于
    *   载荷集文件同级的同名子文件夹中）；
    * - 对流换热：第3列环境温度、第4列对流换热系数文件（文件须存在）；
    * - 热辐射：第3列环境温度、第4列热辐射系数（数值）；
    * - 温度边界：第3列温度场文件（文件须存在）。
    */
    namespace TimeSeriesFileParser
    {
        /*
        * 解析时序文件（仅需前两列时间列，校验时间列表头）
        * minColumns 之外不校验附加列
        */
        MMAINWINDOW_EXPORT TimeSeriesParseResult parseTimeSequenceFile(const QString& filePath);

        /*
        * 解析载荷集 / 边界条件文件
        * requiredColumns 为该载荷类型要求的最小列数（含前两列时间）
        * expectedHeaders 为附加列的期望表头（与列序一一对应，空串表示不校验该列表头）
        * numericExtraColumns 为必须为数值的附加列（0基列号，如 2 表示第3列）
        * fileColumns 为引用场文件的附加列（0基列号），其文件必须存在于载荷集文件
        *   同级的同名子文件夹中（路径以载荷集文件目录为基准解析）
        */
        MMAINWINDOW_EXPORT TimeSeriesParseResult parseLoadSetFile(const QString& filePath,
            int requiredColumns, const QStringList& expectedHeaders = {},
            const QList<int>& numericExtraColumns = {}, const QList<int>& fileColumns = {});

        /*
        * 校验时间列：非负、开始<结束、范围连续（前行结束=后行开始，容差1e-6）
        * 错误信息中标注文件行号（含表头，数据首行为第2行）
        */
        MMAINWINDOW_EXPORT bool validateTimeRanges(const QList<QPair<double, double>>& ranges, QString* error);
    }
}
