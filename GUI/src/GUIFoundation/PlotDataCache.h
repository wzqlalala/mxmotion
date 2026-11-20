#pragma once
#include <QString>
#include <QVector>
#include <QStringList>
#include <QPair>

namespace MechMainWindow
{
    struct PlotDataCache
    {
        QString viewName;
        QString xTitle;
        QVector<QString> yTitles;
        QVector<double> xValue;
        QStringList xStringValue;
        QList<QPair<QString, QVector<double>>> curveDatas;
        QStringList curveNames;
        QString legendInfoText;
    };
}
