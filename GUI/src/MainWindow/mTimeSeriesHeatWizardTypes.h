#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

namespace MechMainWindow
{
    /*
    * 时序工况规格：一个时间分段对应一个瞬态传热工况
    */
    struct TimeSeriesCaseSpec
    {
        int no = 0;             // 序号（从1开始）
        QString name;           // 工况名：分析序列名称_开始_结束
        double start = 0.0;     // 开始时间 s
        double end = 0.0;       // 结束时间 s
        double duration = 0.0;  // 持续时间 s
    };

    /*
    * 时间步定义：单位在向导内统一折算为秒
    */
    struct TimeSeriesStepSpec
    {
        bool isAuto = false;    // false=固定 true=自动
        double fixed = 1.0;     // 固定步长 s
        double initial = 1.0;   // 初始步长 s
        double minStep = 0.1;   // 最小步长 s
        double maxStep = 10.0;  // 最大步长 s
    };

    /*
    * 时序载荷类型
    */
    enum class TimeSeriesLoadKind
    {
        PointHeatFlux,      // 点热流（NodeSet）
        SurfaceHeatFlux,    // 面热流（ElementSurface）
        VolumeHeatFlux,     // 体热流（ElementSet）
        Convection,         // 对流换热（ElementSurface）
        Radiation           // 热辐射（ElementSurface）
    };

    /*
    * 时序载荷项：载荷与边界一律走分析场定义模式，数值由分析场承载
    */
    struct TimeSeriesLoadItem
    {
        QString name;                   // 载荷集名称
        TimeSeriesLoadKind kind = TimeSeriesLoadKind::SurfaceHeatFlux;
        QString setName;                // 作用集合名，空串表示未定义（全模型）
        QString filePath;               // 载荷集文件路径
        bool imported = false;          // 是否已导入
        QList<QStringList> rows;        // 载荷集文件数据行（Tab分列后的字段）
    };

    /*
    * 温度边界项：直接指定温度值，不引用文件、不关联分析场；
    * 落库时创建一个边界本体并映射到全部时序工况
    */
    struct TimeSeriesBoundaryItem
    {
        QString name;                   // 边界条件名称
        QString setName;                // 作用集合名，空串表示未定义（全模型）
        double temperature = 20.0;      // 温度值（℃）
    };

    /*
    * 结果输出配置
    */
    struct TimeSeriesOutputSpec
    {
        QString format = QStringLiteral("mxdb");    // mxdb / op2
        QSet<QString> variables;                    // 勾选的求解器变量名：NT/GRADT/EGRADT/HFL
        // 每个勾选变量的集合选择（求解器变量名 → 集合名称列表，与工况数据结构
        // setSelectedSet 的取值一致；空列表表示全局/全模型）
        QHash<QString, QStringList> variableSets;
    };

    /*
    * 时间值转名称片段：保留4位小数并去尾零（与原型 timeToken 一致）
    */
    inline QString timeSeriesTimeToken(double value)
    {
        QString text = QString::number(value, 'f', 4);
        while (text.contains(QLatin1Char('.')) && text.endsWith(QLatin1Char('0')))
            text.chop(1);
        if (text.endsWith(QLatin1Char('.')))
            text.chop(1);
        if (text == QStringLiteral("-0"))
            text = QStringLiteral("0");
        return text;
    }

    /*
    * 载荷类型显示名
    */
    inline QString timeSeriesLoadKindName(TimeSeriesLoadKind kind)
    {
        switch (kind)
        {
        case TimeSeriesLoadKind::PointHeatFlux:   return QStringLiteral("点热流");
        case TimeSeriesLoadKind::SurfaceHeatFlux: return QStringLiteral("面热流");
        case TimeSeriesLoadKind::VolumeHeatFlux:  return QStringLiteral("体热流");
        case TimeSeriesLoadKind::Convection:      return QStringLiteral("对流换热");
        case TimeSeriesLoadKind::Radiation:       return QStringLiteral("热辐射");
        }
        return QString();
    }

    /*
    * 载荷类型默认载荷集名称
    */
    inline QString timeSeriesLoadDefaultName(TimeSeriesLoadKind kind)
    {
        switch (kind)
        {
        case TimeSeriesLoadKind::PointHeatFlux:   return QStringLiteral("PointHeatFlux");
        case TimeSeriesLoadKind::SurfaceHeatFlux: return QStringLiteral("HeatFlux");
        case TimeSeriesLoadKind::VolumeHeatFlux:  return QStringLiteral("VolumeHeatFlux");
        case TimeSeriesLoadKind::Convection:      return QStringLiteral("ConvectionFilm");
        case TimeSeriesLoadKind::Radiation:       return QStringLiteral("RadiationFlux");
        }
        return QString();
    }
}
