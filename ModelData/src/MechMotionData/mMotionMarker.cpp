#include "mMotionMarker.h"

namespace MechMotionData {

    mMotionMarker::mMotionMarker()
        : m_id(-1),
        m_name(),
        m_localPosition(0.0, 0.0, 0.0),
        m_localRotation(1.0, 0.0, 0.0, 0.0),
        m_color(Qt::red),
        m_size(5.0),
        m_visible(true)
    {
    }

    mMotionMarker::mMotionMarker(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_localPosition(0.0, 0.0, 0.0),
        m_localRotation(1.0, 0.0, 0.0, 0.0),
        m_color(Qt::red),
        m_size(5.0),
        m_visible(true)
    {
        if (m_name.isEmpty()) {
            m_name = QString("Marker_%1").arg(m_id);
        }
    }

    mMotionMarker::~mMotionMarker() {}

    void mMotionMarker::setId(int id) { m_id = id; }
    int mMotionMarker::getId() const { return m_id; }

    void mMotionMarker::setName(const QString& name) { m_name = name; }
    QString mMotionMarker::getName() const { return m_name; }


    void mMotionMarker::setLocalPosition(const QVector3D& pos) { m_localPosition = pos; }
    QVector3D mMotionMarker::getLocalPosition() const { return m_localPosition; }

    void mMotionMarker::setLocalRotation(const QQuaternion& rot) { m_localRotation = rot; }
    QQuaternion mMotionMarker::getLocalRotation() const { return m_localRotation; }

    void mMotionMarker::setColor(const QColor& color) { m_color = color; }
    QColor mMotionMarker::getColor() const { return m_color; }

    void mMotionMarker::setSize(double size) { m_size = size; }
    double mMotionMarker::getSize() const { return m_size; }

    void mMotionMarker::setVisible(bool visible) { m_visible = visible; }
    bool mMotionMarker::getVisible() const { return m_visible; }

    mMotionMarker* mMotionMarker::copyMarker(int newId, const QString& newName)
    {
        mMotionMarker* newMarker = new mMotionMarker(newId, newName);
        newMarker->setLocalPosition(this->getLocalPosition());
        newMarker->setLocalRotation(this->getLocalRotation());
        newMarker->setColor(this->getColor());
        newMarker->setSize(this->getSize());
        newMarker->setVisible(this->getVisible());
        return newMarker;
    }

    // operator>>/operator<< 已集中到 mMotionDataInStream.h / mMotionDataOutStream.h

}
