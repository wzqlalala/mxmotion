#pragma once
#include <QColor>
#include <QVector>

namespace PlotView {
// The fallback uses the established native Qwt palette. Per-request colors
// from the original motion curve data always take precedence.
inline const QVector<QColor> PlotColor{
    QColor("#2176c1"),QColor("#d35f39"),QColor("#24946b"),QColor("#9864ba"),
    QColor("#b18a13"),QColor("#cf547a"),QColor("#298a9a"),QColor("#6e7790")};
}
