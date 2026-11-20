#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include <QString>

namespace MechMotionData {

    /**
     * @brief 控制器类型
     */
    enum class MXMOTIONDATA_EXPORT ControllerType {
        PID = 0,    ///< PID 控制器
    };

    /**
     * @brief PID 反馈量类型
     * @details 反馈取自被控驱动所驱动自由度的实际值
     */
    enum class MXMOTIONDATA_EXPORT PidFeedbackType {
        Position = 0,   ///< 位移（旋转驱动为角度，弧度；平移驱动为长度，米）
        Velocity = 1,   ///< 速度（旋转驱动为角速度，弧度/秒；平移驱动为速度，米/秒）
    };

    /**
     * @brief PID 反馈源类型
     */
    enum class MXMOTIONDATA_EXPORT PidFeedbackSource {
        DriveDof = 0,   ///< 被控驱动的自由度（马达角度/位置或其导数）
        Sensor = 1      ///< 传感器值（求解期对引擎活状态求值）
    };

    /**
     * @brief 获取控制器类型中文名称
     */
    MXMOTIONDATA_EXPORT QString getControllerTypeName(ControllerType type);

    /**
     * @brief 获取 PID 反馈量类型中文名称
     */
    MXMOTIONDATA_EXPORT QString getPidFeedbackTypeName(PidFeedbackType type);

}
