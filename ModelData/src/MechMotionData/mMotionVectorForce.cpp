#include "mMotionVectorForce.h"

namespace MechMotionData {

    mMotionVectorForce::mMotionVectorForce()
        : mMotionForceElement(-1, QString()),
        m_loadType(VectorLoadType::Force),
        m_actionPoint(0.0, 0.0, 0.0),
        m_direction(1.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_expression(),
        m_magnitudeMode(MagnitudeMode::Constant),
        m_directionMode(DirectionMode::GlobalFixed)
    {
    }

    mMotionVectorForce::mMotionVectorForce(int id, VectorLoadType loadType, const QString& name)
        : mMotionForceElement(id, name),
        m_loadType(loadType),
        m_actionPoint(0.0, 0.0, 0.0),
        m_direction(1.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_expression(),
        m_magnitudeMode(MagnitudeMode::Constant),
        m_directionMode(DirectionMode::GlobalFixed)
    {
        if (m_name.isEmpty()) {
            QString prefix;
            switch (m_loadType) {
            case VectorLoadType::Force:           prefix = "VectorForce"; break;
            case VectorLoadType::Torque:          prefix = "VectorTorque"; break;
            case VectorLoadType::UniForce:        prefix = "UniForce"; break;
            case VectorLoadType::UniTorque:       prefix = "UniTorque"; break;
            case VectorLoadType::TriForce:        prefix = "TriForce"; break;
            case VectorLoadType::TriTorque:       prefix = "TriTorque"; break;
            case VectorLoadType::GeneralForce:    prefix = "GeneralForce"; break;
            case VectorLoadType::Gravity:         prefix = "Gravity"; break;
            case VectorLoadType::ForceField:      prefix = "ForceField"; break;
            case VectorLoadType::LinearSpringDamper: prefix = "SpringDamper"; break;
            case VectorLoadType::TireForce:       prefix = "TireForce"; break;
            case VectorLoadType::Bushing:         prefix = "Bushing"; break;
            default:                              prefix = "VectorForce"; break;
            }
            m_name = QString("%1_%2").arg(prefix).arg(m_id);
        }
    }

    mMotionVectorForce::~mMotionVectorForce() {}

    void mMotionVectorForce::setLoadType(VectorLoadType type) { m_loadType = type; }
    VectorLoadType mMotionVectorForce::getLoadType() const { return m_loadType; }


    void mMotionVectorForce::setActionPoint(const QVector3D& point) { m_actionPoint = point; }
    QVector3D mMotionVectorForce::getActionPoint() const { return m_actionPoint; }

    void mMotionVectorForce::setDirection(const QVector3D& dir) { m_direction = dir; }
    QVector3D mMotionVectorForce::getDirection() const { return m_direction; }

    void mMotionVectorForce::setMagnitude(double magnitude) { m_magnitude = magnitude; }
    double mMotionVectorForce::getMagnitude() const { return m_magnitude; }

    void mMotionVectorForce::setExpression(const QString& expr) { m_expression = expr; }
    QString mMotionVectorForce::getExpression() const { return m_expression; }

    void mMotionVectorForce::setMagnitudeMode(MagnitudeMode mode) { m_magnitudeMode = mode; }
    MagnitudeMode mMotionVectorForce::getMagnitudeMode() const { return m_magnitudeMode; }

    void mMotionVectorForce::setDirectionMode(DirectionMode mode) { m_directionMode = mode; }
    DirectionMode mMotionVectorForce::getDirectionMode() const { return m_directionMode; }

    QString mMotionVectorForce::getLoadTypeName() const {
        return getLoadTypeName(m_loadType);
    }

    QString mMotionVectorForce::getLoadTypeName(VectorLoadType type) {
        switch (type) {
        case VectorLoadType::Force:           return QString("\u77e2\u91cf\u529b");      // 矢量力
        case VectorLoadType::Torque:          return QString("\u77e2\u91cf\u529b\u77e9");  // 矢量力矩
        case VectorLoadType::UniForce:        return QString("\u5355\u5411\u529b");      // 单向力
        case VectorLoadType::UniTorque:       return QString("\u5355\u5411\u529b\u77e9");  // 单向力矩
        case VectorLoadType::TriForce:        return QString("三点力");  // 三点力
        case VectorLoadType::TriTorque:       return QString("三点力矩"); // 三点力矩
        case VectorLoadType::GeneralForce:    return QString("\u4e00\u822c\u4f5c\u7528\u529b"); // 一般作用力
        case VectorLoadType::Gravity:         return QString("\u91cd\u529b");            // 重力
        case VectorLoadType::ForceField:      return QString("\u529b\u573a");            // 力场
        case VectorLoadType::LinearSpringDamper: return QString("\u7ebf\u6027\u5f39\u7c27\u963b\u5c3c"); // 线性弹簧阻尼
        case VectorLoadType::TireForce:       return QString("\u8f6e\u80ce\u529b");       // 轮胎力
        case VectorLoadType::Bushing:         return QString("\u8f74\u5957");             // 轴套
        case VectorLoadType::ScalarForce:     return QString("\u6807\u91cf\u529b");       // 标量力
        default: return QString("Unknown");
        }
    }

    mMotionVectorForce* mMotionVectorForce::copyForce(int newId, const QString& newName)
    {
        mMotionVectorForce* newForce = new mMotionVectorForce(newId, m_loadType, newName);
        newForce->setActionPoint(this->getActionPoint());
        newForce->setDirection(this->getDirection());
        newForce->setMagnitude(this->getMagnitude());
        newForce->setExpression(this->getExpression());
        newForce->setMagnitudeMode(this->getMagnitudeMode());
        newForce->setDirectionMode(this->getDirectionMode());
        return newForce;
    }

    mMotionForceElement* mMotionVectorForce::copyForceElement(int newId, const QString& newName)
    {
        return copyForce(newId, newName);
    }

    // operator>>/operator<< 已集中到 mMotionDataInStream.h / mMotionDataOutStream.h

}
