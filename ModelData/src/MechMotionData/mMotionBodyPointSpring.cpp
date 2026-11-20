#include "mMotionBodyPointSpring.h"

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionBodyPointSpring::mMotionBodyPointSpring(int id, const QString& name)
        : mMotionSpringBase(id, SpringType::BodyPoint, name),
        m_localI(0.0f, 0.0f, 0.0f),
        m_localJ(0.0f, 0.0f, 0.0f),
        m_localRotI(),
        m_localRotJ()
    {
    }

    mMotionBodyPointSpring::~mMotionBodyPointSpring() {}

    mMotionSpringBase* mMotionBodyPointSpring::copySpring(int newId, const QString& newName)
    {
        mMotionBodyPointSpring* newSpring = new mMotionBodyPointSpring(newId, newName);

        newSpring->setLocalI(this->getLocalI());
        newSpring->setLocalJ(this->getLocalJ());
        newSpring->setLocalRotI(this->getLocalRotI());
        newSpring->setLocalRotJ(this->getLocalRotJ());

        newSpring->setStiffness(this->getStiffness());
        newSpring->setDamping(this->getDamping());
        newSpring->setPreload(this->getPreload());
        newSpring->setSpringDirectionMode(this->getSpringDirectionMode());
        newSpring->setDampingDirectionMode(this->getDampingDirectionMode());
        newSpring->setFreeValue(this->getFreeValue());
        newSpring->setInstallValue(this->getInstallValue());
        newSpring->setTable(this->getTableX(), this->getTableY());
        newSpring->setForceFunction(this->getForceFunction());
        newSpring->setDampingFunction(this->getDampingFunction());
        newSpring->setDampingEnabled(this->isDampingEnabled());

        return newSpring;
    }


    void mMotionBodyPointSpring::setLocalI(const QVector3D& pos) { m_localI = pos; }
    QVector3D mMotionBodyPointSpring::getLocalI() const { return m_localI; }

    void mMotionBodyPointSpring::setLocalRotI(const QQuaternion& rot) { m_localRotI = rot; }
    QQuaternion mMotionBodyPointSpring::getLocalRotI() const { return m_localRotI; }


    void mMotionBodyPointSpring::setLocalJ(const QVector3D& pos) { m_localJ = pos; }
    QVector3D mMotionBodyPointSpring::getLocalJ() const { return m_localJ; }

    void mMotionBodyPointSpring::setLocalRotJ(const QQuaternion& rot) { m_localRotJ = rot; }
    QQuaternion mMotionBodyPointSpring::getLocalRotJ() const { return m_localRotJ; }
}
