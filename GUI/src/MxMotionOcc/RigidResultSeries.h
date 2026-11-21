#pragma once

#include "RigidMotion.h"

#include <QString>
#include <QVector>

namespace MotionOcc {

// The same extraction is used for the plot and its CSV export. Object IDs,
// physical timestamps and units are kept intact; missing values are errors,
// never zero-filled or connected across a missing frame.
struct RigidCurveSeries {
    QVector<double> times;
    QVector<double> values;
    QString quantity;
    QString unit;
    QString error;
    bool isValid() const { return error.isEmpty() && !times.isEmpty() && times.size() == values.size(); }
};

bool isBodyCurve(RigidCurveMetric metric);
QString rigidCurveQuantity(RigidCurveMetric metric, JointKind jointKind = JointKind::Revolute);
QString rigidCurveUnit(RigidCurveMetric metric, JointKind jointKind = JointKind::Revolute);
QString rigidChannelSourceType(const QString& sourceType);
QString rigidChannelSourceLabel(const QString& sourceType);
QString rigidChannelObjectName(const RigidInput& input, const QString& sourceType, int sourceId);
QString rigidChannelVariableLabel(const QString& variable);
QString rigidChannelComponentLabel(int components, int component);
const ResultChannel* rigidCurveChannel(const RigidResult& result, const RigidCurveRequest& request);
QString rigidCurveTitle(const RigidInput& input, const RigidCurveRequest& request,
                        const RigidResult* result = nullptr);
RigidCurveSeries rigidCurveSeries(const RigidInput& input, const RigidResult& result,
                                 const RigidCurveRequest& request);

} // namespace MotionOcc
