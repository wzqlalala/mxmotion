#include "mTimeSeriesFileParser.h"
#pragma execution_character_set("utf-8")

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace MechMainWindow
{
namespace TimeSeriesFileParser
{
    namespace
    {
        /*
        * 公共解析流程：读取文件、校验表头、按行拆分、校验时间列与附加列
        * expectedHeaders：附加列（第3列起）的期望表头，空串表示不校验该列
        * fileColumns：引用场文件的附加列（0基），需校验文件存在（相对载荷集文件目录）
        */
        TimeSeriesParseResult parseFile(const QString& filePath, int requiredColumns,
            const QStringList& expectedHeaders, const QList<int>& numericExtraColumns,
            const QList<int>& fileColumns)
        {
            TimeSeriesParseResult result;

            QFile file(filePath);
            if (!file.exists())
            {
                result.error = QStringLiteral("文件不存在：%1").arg(filePath);
                return result;
            }
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                result.error = QStringLiteral("无法打开文件：%1").arg(filePath);
                return result;
            }

            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            stream.setAutoDetectUnicode(true);   // 兼容带 BOM 的 UTF-8 文件

            const QFileInfo fileInfo(filePath);
            const QString fileName = fileInfo.fileName();
            const QDir fileDir = fileInfo.dir();
            const QString expectedBase = fileInfo.completeBaseName();   // 场文件应位于同名子文件夹

            int lineNo = 0;
            while (!stream.atEnd())
            {
                const QString line = stream.readLine();
                ++lineNo;

                // 首行为表头：校验前两列与类型相关的附加列表头
                if (lineNo == 1)
                {
                    const QStringList headers = line.trimmed().split(QLatin1Char('\t'), QString::KeepEmptyParts);
                    const QString col0 = headers.value(0).trimmed();
                    const QString col1 = headers.value(1).trimmed();
                    if (col0 != QStringLiteral("开始时间_s") || col1 != QStringLiteral("结束时间_s"))
                    {
                        result.error = QStringLiteral("%1：文件格式不符，表头前两列应为「开始时间_s」「结束时间_s」，实际为「%2」「%3」")
                            .arg(fileName, col0, col1);
                        return result;
                    }
                    for (int i = 0; i < expectedHeaders.size(); ++i)
                    {
                        if (expectedHeaders.at(i).isEmpty())
                            continue;
                        const QString actual = headers.value(2 + i).trimmed();
                        if (actual != expectedHeaders.at(i))
                        {
                            result.error = QStringLiteral("%1：文件格式不符，表头第%2列应为「%3」，实际为「%4」")
                                .arg(fileName).arg(3 + i).arg(expectedHeaders.at(i), actual);
                            return result;
                        }
                    }
                    continue;
                }

                const QString trimmed = line.trimmed();
                if (trimmed.isEmpty())
                    continue;

                const QStringList fields = trimmed.split(QLatin1Char('\t'), QString::KeepEmptyParts);
                if (fields.size() < requiredColumns)
                {
                    result.error = QStringLiteral("%1 第%2行：列数不足（需要至少%3列，实际%4列）")
                        .arg(fileName).arg(lineNo).arg(requiredColumns).arg(fields.size());
                    return result;
                }

                bool okStart = false, okEnd = false;
                const double start = fields.at(0).trimmed().toDouble(&okStart);
                const double end = fields.at(1).trimmed().toDouble(&okEnd);
                if (!okStart || !okEnd)
                {
                    result.error = QStringLiteral("%1 第%2行：开始/结束时间必须为数值").arg(fileName).arg(lineNo);
                    return result;
                }

                for (int column : numericExtraColumns)
                {
                    if (column < 0 || column >= fields.size())
                        continue;
                    bool ok = false;
                    fields.at(column).trimmed().toDouble(&ok);
                    if (!ok)
                    {
                        result.error = QStringLiteral("%1 第%2行：第%3列必须为数值")
                            .arg(fileName).arg(lineNo).arg(column + 1);
                        return result;
                    }
                }

                // 场文件列：校验文件存在，且位于载荷集文件同级的同名子文件夹中
                for (int column : fileColumns)
                {
                    if (column < 0 || column >= fields.size())
                        continue;
                    const QString fieldPath = fields.at(column).trimmed();
                    if (fieldPath.isEmpty())
                    {
                        result.error = QStringLiteral("%1 第%2行：第%3列场文件不能为空")
                            .arg(fileName).arg(lineNo).arg(column + 1);
                        return result;
                    }
                    const QFileInfo fieldInfo(fileDir.absoluteFilePath(fieldPath));
                    if (!fieldInfo.exists() || !fieldInfo.isFile())
                    {
                        result.error = QStringLiteral("%1 第%2行：场文件不存在「%3」")
                            .arg(fileName).arg(lineNo).arg(fieldPath);
                        return result;
                    }
                    if (fieldInfo.dir().dirName() != expectedBase)
                    {
                        result.error = QStringLiteral("%1 第%2行：场文件「%3」应放在载荷集文件同级的「%4」文件夹中")
                            .arg(fileName).arg(lineNo).arg(fieldPath, expectedBase);
                        return result;
                    }
                }

                result.rows.append(fields);
                result.timeRanges.append({ start, end });
            }

            if (result.rows.isEmpty())
            {
                result.error = QStringLiteral("%1：文件中没有有效数据行").arg(fileName);
                return result;
            }

            if (!validateTimeRanges(result.timeRanges, &result.error))
            {
                result.error = result.error.arg(fileName);
                return result;
            }

            result.ok = true;
            return result;
        }
    }

    TimeSeriesParseResult parseTimeSequenceFile(const QString& filePath)
    {
        return parseFile(filePath, 2, {}, {}, {});
    }

    TimeSeriesParseResult parseLoadSetFile(const QString& filePath, int requiredColumns,
        const QStringList& expectedHeaders, const QList<int>& numericExtraColumns,
        const QList<int>& fileColumns)
    {
        return parseFile(filePath, qMax(2, requiredColumns), expectedHeaders,
            numericExtraColumns, fileColumns);
    }

    bool validateTimeRanges(const QList<QPair<double, double>>& ranges, QString* error)
    {
        constexpr double tolerance = 1e-6;
        double previousEnd = -1.0;
        for (int i = 0; i < ranges.size(); ++i)
        {
            const double start = ranges.at(i).first;
            const double end = ranges.at(i).second;
            const int fileLine = i + 2; // 数据首行为文件第2行

            if (start < 0.0 || end < 0.0)
            {
                if (error) *error = QStringLiteral("%1 第") + QString::number(fileLine) + QStringLiteral("行：时间不能为负");
                return false;
            }
            if (start >= end)
            {
                if (error) *error = QStringLiteral("%1 第") + QString::number(fileLine) + QStringLiteral("行：开始时间必须小于结束时间");
                return false;
            }
            if (previousEnd >= 0.0 && qAbs(start - previousEnd) > tolerance)
            {
                if (error) *error = QStringLiteral("%1 第") + QString::number(fileLine) + QStringLiteral("行：时间范围不连续（与上一行结束时间不衔接）");
                return false;
            }
            previousEnd = end;
        }
        return true;
    }
}
}
