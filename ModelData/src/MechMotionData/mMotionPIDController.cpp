#include "mMotionPIDController.h"

namespace MechMotionData {

    mMotionPIDController::mMotionPIDController(int id, const QString& name, bool isUserRename)
        : mMotionControllerBase(id, ControllerType::PID, name, isUserRename)
        
        , m_feedbackType(PidFeedbackType::Position)
        , m_feedbackSource(PidFeedbackSource::DriveDof)
        
        , m_feedbackSign(1.0)
        , m_targetValue(0.0)
        
        , m_kp(1.0)
        , m_ki(0.0)
        , m_kd(0.0)
        , m_outputLimit(0.0)
    {
    }

    mMotionPIDController::mMotionPIDController()
        : mMotionControllerBase()
        
        , m_feedbackType(PidFeedbackType::Position)
        , m_feedbackSource(PidFeedbackSource::DriveDof)
        
        , m_feedbackSign(1.0)
        , m_targetValue(0.0)
        
        , m_kp(1.0)
        , m_ki(0.0)
        , m_kd(0.0)
        , m_outputLimit(0.0)
    {
    }

    mMotionPIDController::~mMotionPIDController() {}

    ControllerType mMotionPIDController::getType() const {
        return ControllerType::PID;
    }



    void mMotionPIDController::setFeedbackType(PidFeedbackType type) {
        m_feedbackType = type;
    }

    PidFeedbackType mMotionPIDController::getFeedbackType() const {
        return m_feedbackType;
    }

    void mMotionPIDController::setFeedbackSource(PidFeedbackSource source) {
        m_feedbackSource = source;
    }

    PidFeedbackSource mMotionPIDController::getFeedbackSource() const {
        return m_feedbackSource;
    }



    void mMotionPIDController::setFeedbackSign(double sign) {
        m_feedbackSign = (sign < 0.0) ? -1.0 : 1.0;
    }

    double mMotionPIDController::getFeedbackSign() const {
        return m_feedbackSign;
    }

    void mMotionPIDController::setTargetValue(double value) {
        m_targetValue = value;
    }

    double mMotionPIDController::getTargetValue() const {
        return m_targetValue;
    }



    void mMotionPIDController::setGains(double kp, double ki, double kd) {
        m_kp = kp;
        m_ki = ki;
        m_kd = kd;
    }

    double mMotionPIDController::getKp() const {
        return m_kp;
    }

    double mMotionPIDController::getKi() const {
        return m_ki;
    }

    double mMotionPIDController::getKd() const {
        return m_kd;
    }

    void mMotionPIDController::setOutputLimit(double limit) {
        m_outputLimit = limit;
    }

    double mMotionPIDController::getOutputLimit() const {
        return m_outputLimit;
    }

    mMotionControllerBase* mMotionPIDController::copyController(int newId, const QString& newName) const {
        auto* copy = new mMotionPIDController(newId, newName, getIsUserRename());
        copy->setEnabled(isEnabled());
        copy->setFeedbackType(m_feedbackType);
        copy->setFeedbackSource(m_feedbackSource);
        copy->setFeedbackSign(m_feedbackSign);
        copy->setTargetValue(m_targetValue);
        copy->setGains(m_kp, m_ki, m_kd);
        copy->setOutputLimit(m_outputLimit);
        return copy;
    }

    void mMotionPIDController::readFromStream(QDataStream& in) {
        int fbInt = 0;
        in >> fbInt;
        m_feedbackType = static_cast<PidFeedbackType>(fbInt);
        in >> m_targetValue;
        in >> m_kp;
        in >> m_ki;
        in >> m_kd;
        in >> m_outputLimit;
        // 目标轨迹/反馈源字段为后追加，旧文件按默认值处理
        if (!in.atEnd()) {
        }
        else {
        }
        if (!in.atEnd()) {
            int srcInt = 0;
            in >> srcInt;
            m_feedbackSource = static_cast<PidFeedbackSource>(srcInt);
            in >> m_feedbackSign;
        }
        else {
            m_feedbackSource = PidFeedbackSource::DriveDof;
            m_feedbackSign = 1.0;
        }
    }

    void mMotionPIDController::writeToStream(QDataStream& out) const {
        out << static_cast<int>(m_feedbackType);
        out << m_targetValue;
        out << m_kp;
        out << m_ki;
        out << m_kd;
        out << m_outputLimit;
        out << static_cast<int>(m_feedbackSource);
        out << m_feedbackSign;
    }

}
