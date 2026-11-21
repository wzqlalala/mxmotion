#pragma once

#include <QJsonArray>
#include <QString>
#include <QVector>

class QWidget;

namespace MotionOcc {

struct NativeFunctionSamples {
    QVector<double> time;
    QVector<double> value;
};

// Two numeric columns; UTF-8/UTF-16 BOM, quoted CSV/TSV, and whitespace TXT
// are accepted. A recognized time header is optional. Invalid or unordered
// rows report their source line and leave the caller's table unchanged.
bool readNativeFunctionTable(const QString& path, QJsonArray& tableX,
                             QJsonArray& tableY, QString& error);
bool writeNativeFunctionTable(const QString& path, const QJsonArray& tableX,
                              const QJsonArray& tableY, const QString& valueUnits,
                              QString& error);

// Preview and validation use the very same engine classes as the solve path.
// A nonempty table takes precedence over an inactive expression. Table knots
// are included in the samples, so short segments remain visible in the plot.
// The output is unchanged on failure. x/v are available only when requested.
bool sampleNativeFunction(const QString& expression, const QJsonArray& tableX,
                          const QJsonArray& tableY, double start, double end,
                          int sampleCount, NativeFunctionSamples& output, QString& error,
                          bool allowStateVariables = false, double x = 0, double v = 0);

// Dedicated editor for the original constant / formula / template / table
// workflow. Cancel leaves all three values unchanged. Selecting a formula
// clears active table data explicitly; table mode retains the inactive formula.
bool editNativeFunction(QWidget* parent, QString& expression, QJsonArray& tableX,
                        QJsonArray& tableY, const QString& units, double duration = 10.0,
                        bool allowStateVariables = false, bool allowTables = true);

} // namespace MotionOcc
