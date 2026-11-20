#include "mMotionBushing.h"

namespace MechMotionData {

    static QVector<double> makeDefault3(double v = 0.0) {
        return QVector<double>() << v << v << v;
    }

    static QVector<double> makeDefault6(double v = 0.0) {
        return QVector<double>() << v << v << v << v << v << v;
    }

    mMotionBushing::mMotionBushing()
        : mMotionVectorForce(),
        m_stiffness(makeDefault6()),
        m_damping(makeDefault6()),
        m_offset(0.0, 0.0, 0.0),
        m_preloadForce(makeDefault3()),
        m_preloadTorque(makeDefault3())
    {
        setLoadType(VectorLoadType::Bushing);
    }

    mMotionBushing::mMotionBushing(int id, const QString& name)
        : mMotionVectorForce(id, VectorLoadType::Bushing, name),
        m_stiffness(makeDefault6()),
        m_damping(makeDefault6()),
        m_offset(0.0, 0.0, 0.0),
        m_preloadForce(makeDefault3()),
        m_preloadTorque(makeDefault3())
    {
    }

    mMotionBushing::~mMotionBushing() {}

    // --- 刚度 / 阻尼 ---
    void mMotionBushing::setStiffness(const QVector<double>& k) { m_stiffness = k; }
    QVector<double> mMotionBushing::getStiffness() const { return m_stiffness; }

    void mMotionBushing::setDamping(const QVector<double>& c) { m_damping = c; }
    QVector<double> mMotionBushing::getDamping() const { return m_damping; }

    // --- 基本体 / marker_J ---

    void mMotionBushing::setApplyOffset(bool apply) { m_applyOffset = apply; }
    bool mMotionBushing::getApplyOffset() const { return m_applyOffset; }

    void mMotionBushing::setOffset(const QVector3D& offset) { m_offset = offset; }
    QVector3D mMotionBushing::getOffset() const { return m_offset; }

    // --- 预载 ---
    void mMotionBushing::setPreloadForce(const QVector<double>& f0) { m_preloadForce = f0; }
    QVector<double> mMotionBushing::getPreloadForce() const { return m_preloadForce; }

    void mMotionBushing::setPreloadTorque(const QVector<double>& t0) { m_preloadTorque = t0; }
    QVector<double> mMotionBushing::getPreloadTorque() const { return m_preloadTorque; }

    mMotionVectorForce* mMotionBushing::copyForce(int newId, const QString& newName)
    {
        mMotionBushing* newBushing = new mMotionBushing(newId, newName);
        // 基类通用字段
        newBushing->setActionPoint(this->getActionPoint());
        newBushing->setDirection(this->getDirection());
        newBushing->setMagnitude(this->getMagnitude());
        newBushing->setExpression(this->getExpression());
        newBushing->setMagnitudeMode(this->getMagnitudeMode());
        newBushing->setDirectionMode(this->getDirectionMode());
        newBushing->setStiffness(this->getStiffness());
        newBushing->setDamping(this->getDamping());
        // Bushing 专属字段
        newBushing->setApplyOffset(this->getApplyOffset());
        newBushing->setOffset(this->getOffset());
        newBushing->setPreloadForce(this->getPreloadForce());
        newBushing->setPreloadTorque(this->getPreloadTorque());
        return newBushing;
    }

    mMotionForceElement* mMotionBushing::copyForceElement(int newId, const QString& newName)
    {
        return copyForce(newId, newName);
    }

    // operator>>/operator<< 已集中到 mMotionDataInStream.h / mMotionDataOutStream.h

}
