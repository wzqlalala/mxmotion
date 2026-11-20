#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMotionControllerBase.h"

namespace MechMotionData {

    /**
     * @brief PID 控制器
     * @details 以速度型驱动为执行机构：每个求解步用驱动自由度的实际值与目标值之差
     *          计算 PID 输出，注入为驱动的速度设定值，实现位置/速度伺服控制
     */
    class MXMOTIONDATA_EXPORT mMotionPIDController : public mMotionControllerBase {
    public:
        /**
         * @brief 构造函数
         * @param id 控制器ID
         * @param name 名称
         */
        mMotionPIDController(int id, const QString& name = "", bool isUserRename = false);

        /**
         * @brief 默认构造函数（用于序列化）
         */
        mMotionPIDController();

        ~mMotionPIDController() override;

        ControllerType getType() const override;

        // --- 被控驱动 ---

        // --- 反馈量 ---
        void setFeedbackType(PidFeedbackType type);
        PidFeedbackType getFeedbackType() const;

        // --- 反馈源（驱动自由度 / 传感器） ---
        void setFeedbackSource(PidFeedbackSource source);
        PidFeedbackSource getFeedbackSource() const;

        // --- 反馈传感器ID（反馈源为传感器时有效，-1 未指定） ---

        // --- 反馈符号（+1/-1，传感器分量方向与驱动方向相反时翻转） ---
        void setFeedbackSign(double sign);
        double getFeedbackSign() const;

        // --- 目标值 ---
        void setTargetValue(double value);
        double getTargetValue() const;

        // --- 目标轨迹（引用控制输入，-1 表示用常数目标值） ---

        // --- PID 参数 ---
        void setGains(double kp, double ki, double kd);
        double getKp() const;
        double getKi() const;
        double getKd() const;

        // --- 输出限幅（速度设定值上限，<=0 表示不限制） ---
        void setOutputLimit(double limit);
        double getOutputLimit() const;

        // --- 复制 ---
        mMotionControllerBase* copyController(int newId, const QString& newName) const override;

    protected:
        void readFromStream(QDataStream& in) override;
        void writeToStream(QDataStream& out) const override;

    private:
        PidFeedbackType m_feedbackType; ///< 反馈量类型（驱动自由度反馈时有效）
        PidFeedbackSource m_feedbackSource; ///< 反馈源类型
        double m_feedbackSign;          ///< 反馈符号（+1/-1）
        double m_targetValue;           ///< 目标值（单位与反馈量一致）

        double m_kp;                    ///< 比例系数
        double m_ki;                    ///< 积分系数
        double m_kd;                    ///< 微分系数

        double m_outputLimit;           ///< 输出限幅（<=0 不限制）
    };

}
