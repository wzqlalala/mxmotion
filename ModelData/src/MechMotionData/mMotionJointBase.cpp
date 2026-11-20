#include "mMotionJointBase.h"

#include <QMatrix4x4>
#include <QStringLiteral>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointBase::mMotionJointBase(int id, JointType type, const QString& name, bool isUserRename)
        : m_id(id), m_type(type), m_name(name), m_isUserRename(isUserRename)
    {
        if (m_name.isEmpty()) {
            m_name = QString("Joint_%1").arg(m_id);
        }
    }

    QString mMotionJointBase::getTypeName() const {
        // 直接复用静态函数，传入自身的枚举类型
        return getTypeName(m_type);
    }

    QString mMotionJointBase::getTypeName(JointType type) {
        switch (type) {
        case JointType::Revolute:
            return QStringLiteral("旋转副");
        case JointType::Translational:
            return QStringLiteral("移动副");
        case JointType::Cylindrical:
            return QStringLiteral("柱面副");
        case JointType::Spherical:
            return QStringLiteral("球面副");
        case JointType::Fixed:
            return QStringLiteral("固定副");
        case JointType::Universal:
            return QStringLiteral("万向节");
        case JointType::PointPlane:
            return QStringLiteral("点面副");
        case JointType::Line:
            return QStringLiteral("直线副");
        case JointType::Parallel:
            return QStringLiteral("平行约束");
        case JointType::Perpendicular:
            return QStringLiteral("垂直约束");
        case JointType::Direction:
            return QStringLiteral("方向约束");
        case JointType::Centripetal:
            return QStringLiteral("向心约束");
        case JointType::ConstantVelocity:
            return QStringLiteral("恒速副");
        case JointType::Hooke:
            return QStringLiteral("胡克副");
        case JointType::Screw:
            return QStringLiteral("螺纹副");
        case JointType::Planar:
            return QStringLiteral("平面副");
        case JointType::Coupling:
            return QStringLiteral("关联副");
        case JointType::Gear:
            return QStringLiteral("齿轮副");
        case JointType::PointLine:
            return QStringLiteral("点线副");
        case JointType::PointOnCurve:
            return QStringLiteral("点在线上副");
        case JointType::PointOnSurface:
            return QStringLiteral("点在面上副");
        case JointType::CurveOnCurve:
            return QStringLiteral("线在线上副");
        default:
            return QStringLiteral("未知类型");
        }
    }

    mMotionJointBase::~mMotionJointBase() {}

    bool mMotionJointBase::getIsUserRename()
    {
        return m_isUserRename;
    }

    int mMotionJointBase::getId() const { return m_id; }

    QString mMotionJointBase::getName() const { return m_name; }

    void mMotionJointBase::setName(const QString& name, bool isUserRename) { 
        m_name = name; 
        m_isUserRename = isUserRename;
    }

    void mMotionJointBase::setType(JointType type)
    {
        m_type = type;
    }

    JointType mMotionJointBase::getType() const { return m_type; }





    const mMotionJointFrictionData& mMotionJointBase::getFrictionData() const noexcept {
        return m_frictionData;
    }

    void mMotionJointBase::setFrictionData(const mMotionJointFrictionData& data) {
        m_frictionData = data;
    }

    void mMotionJointBase::clearFriction() {
        m_frictionData = mMotionJointFrictionData{};
    }

    bool mMotionJointBase::supportsFriction() const noexcept {
        switch (m_type) {
        case JointType::Revolute:
        case JointType::Translational:
        case JointType::Cylindrical:
        case JointType::Universal:
        case JointType::Spherical:
            return true;
        default:
            return false;
        }
    }

    const mMotionJointLimitData& mMotionJointBase::getLimitData() const noexcept {
        return m_limitData;
    }

    void mMotionJointBase::setLimitData(const mMotionJointLimitData& data) {
        m_limitData = data;
    }

    void mMotionJointBase::clearLimit() {
        m_limitData = mMotionJointLimitData{};
    }

    bool mMotionJointBase::supportsLimit() const noexcept {
        return m_type == JointType::Revolute ||
            m_type == JointType::Translational;
    }
    void mMotionJointBase::copyCommonDataTo(mMotionJointBase& target) const
    {
        target.m_type = m_type;

        target.m_frictionData = m_frictionData;
        target.m_limitData = m_limitData;
    }
}
