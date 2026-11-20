#include "mMotionControllerType.h"

namespace MechMotionData {

    QString getControllerTypeName(ControllerType type) {
        switch (type) {
        case ControllerType::PID:
            return QStringLiteral("PID控制器");
        }
        return QStringLiteral("未知控制器");
    }

    QString getPidFeedbackTypeName(PidFeedbackType type) {
        switch (type) {
        case PidFeedbackType::Position:
            return QStringLiteral("位移");
        case PidFeedbackType::Velocity:
            return QStringLiteral("速度");
        }
        return QStringLiteral("未知");
    }

}
