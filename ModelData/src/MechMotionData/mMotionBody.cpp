#include "mMotionBody.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData {

    mMotionBody::mMotionBody(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_isFixed(false),
        m_isCustomMassProps(false),
        m_mass(0.0),
        m_inertia(6, 0.0), m_centerOfMass(0.0, 0.0, 0.0),
        m_initialVel(0.0, 0.0, 0.0)
    {
        if (m_name.isEmpty()) {
            m_name = QString("Body_%1").arg(m_id);
        }
    }

    mMotionBody::~mMotionBody() {}

    void mMotionBody::setId(int id) {
        m_id = id;
    }

    int mMotionBody::getId() const {
        return m_id;
    }

    void mMotionBody::setName(const QString& name) {
        m_name = name;
    }

    QString mMotionBody::getName() const {
        return m_name;
    }

    void mMotionBody::setInitialVelocity(const QVector3D& vel) { m_initialVel = vel; }
    QVector3D mMotionBody::getInitialVelocity() const { return m_initialVel; }

    void mMotionBody::setIsCustomMassProps(bool isCustom)
    {
        m_isCustomMassProps = isCustom;
    }

    void mMotionBody::setInitialAngularVelocity(const QVector3D& omega)
    {
        m_initialOmega = omega;
    }

    QVector3D mMotionBody::getInitialAngularVelocity() const
    {
        return m_initialOmega;
    }
    bool mMotionBody::getIsCustomMassProps() const 
    {
        return m_isCustomMassProps;
    }

    void mMotionBody::setIsFixed(bool fixed) {
        m_isFixed = fixed;
    }

    bool mMotionBody::getIsFixed() const {
        return m_isFixed;
    }

    void mMotionBody::setMass(double mass) {
        m_mass = mass;
    }

    double mMotionBody::getMass() const {
        return m_mass;
    }

    void mMotionBody::setInertia(const QVector<double>& inertia) {
        // 确保传入的惯量数组尺寸合法
        if (inertia.size() == 6) {
            m_inertia = inertia;
        }
    }

    QVector<double> mMotionBody::getInertia() const {
        return m_inertia;
    }

    void mMotionBody::setCenterOfMass(const QVector3D& cm)
    { 
        m_centerOfMass = cm; 
    }
    QVector3D mMotionBody::getCenterOfMass() const 
    { 
        return m_centerOfMass; 
    }

    mMotionBody* mMotionBody::copyBody(int newId, const QString& newName)
    {
        mMotionBody* newBody = new mMotionBody(newId, newName);

        // 拷贝物理属性
        newBody->setIsFixed(this->getIsFixed());
        newBody->setIsCustomMassProps(this->getIsCustomMassProps());
        newBody->setMass(this->getMass());
        newBody->setCenterOfMass(this->getCenterOfMass());
        newBody->setInertia(this->getInertia());
        newBody->setInitialVelocity(this->getInitialVelocity());
        newBody->setInitialAngularVelocity(this->getInitialAngularVelocity());
        // 绑定部件集合由映射关系承载，拷贝走 copyMapRelation，不在数据类内复制

        return newBody;
    }
}