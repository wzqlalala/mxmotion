#include "mMotionSpringBase.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionSpringBase::mMotionSpringBase(int id, SpringType type, const QString& name, bool isUserRename)
        : m_id(id),
        m_type(type),
        m_name(name),
        m_isUserRename(isUserRename),
        m_stiffness(0.0),
        m_damping(0.0),
        m_preload(0.0),
        m_springDirectionMode(SpringDirectionMode::Bidirectional),
        m_dampingDirectionMode(SpringDirectionMode::Bidirectional),
        m_freeValue(0.0),
        m_installValue(0.0),
        m_hasTable(false),
        m_dampingEnabled(false)
    {
        if (m_name.isEmpty()) {
            m_name = QString("Spring_%1").arg(m_id);
        }
    }

    mMotionSpringBase::~mMotionSpringBase() {}

    bool mMotionSpringBase::getIsUserRename()
    {
        return m_isUserRename;
    }

    int mMotionSpringBase::getId() const { return m_id; }

    void mMotionSpringBase::setName(const QString& name, bool isUserRename) {
        m_name = name;
        m_isUserRename = isUserRename;
    }
    QString mMotionSpringBase::getName() const { return m_name; }

    void mMotionSpringBase::setType(SpringType type) {
        if (m_type != type) {
            m_type = type;
            clearTypeSpecificData();
        }
    }
    SpringType mMotionSpringBase::getType() const { return m_type; }

    void mMotionSpringBase::clearTypeSpecificData() {
        m_freeValue = 0.0;
        m_installValue = 0.0;
        m_tableX.clear();
        m_tableY.clear();
        m_hasTable = false;
        m_forceFunction.clear();
        m_dampingFunction.clear();
    }

    QString mMotionSpringBase::getTypeName() const {
        return getTypeName(m_type);
    }

    QString mMotionSpringBase::getTypeName(SpringType type) {
        switch (type) {
        case SpringType::Translational: return QString::fromUtf8("拉压弹簧");
        case SpringType::Torsional: return QString::fromUtf8("扭转弹簧");
        case SpringType::BodyPoint: return QString::fromUtf8("体点弹簧");
        default: return QString::fromUtf8("未知弹簧");
        }
    }


    void mMotionSpringBase::setStiffness(double k) { m_stiffness = k; }
    double mMotionSpringBase::getStiffness() const { return m_stiffness; }

    void mMotionSpringBase::setDamping(double c) { m_damping = c; }
    double mMotionSpringBase::getDamping() const { return m_damping; }

    void mMotionSpringBase::setPreload(double preload) { m_preload = preload; }
    double mMotionSpringBase::getPreload() const { return m_preload; }

    void mMotionSpringBase::setSpringDirectionMode(SpringDirectionMode mode) { m_springDirectionMode = mode; }
    SpringDirectionMode mMotionSpringBase::getSpringDirectionMode() const { return m_springDirectionMode; }

    void mMotionSpringBase::setDampingDirectionMode(SpringDirectionMode mode) { m_dampingDirectionMode = mode; }
    SpringDirectionMode mMotionSpringBase::getDampingDirectionMode() const { return m_dampingDirectionMode; }

    void mMotionSpringBase::setFreeValue(double value) { m_freeValue = value; }
    double mMotionSpringBase::getFreeValue() const { return m_freeValue; }

    void mMotionSpringBase::setInstallValue(double value) { m_installValue = value; }
    double mMotionSpringBase::getInstallValue() const { return m_installValue; }

    void mMotionSpringBase::setTable(const QVector<double>& x, const QVector<double>& y) {
        m_tableX = x;
        m_tableY = y;
        m_hasTable = !x.isEmpty() && !y.isEmpty() && x.size() == y.size();
    }
    QVector<double> mMotionSpringBase::getTableX() const { return m_tableX; }
    QVector<double> mMotionSpringBase::getTableY() const { return m_tableY; }
    bool mMotionSpringBase::hasTable() const { return m_hasTable; }

    void mMotionSpringBase::setForceFunction(const QString& expr) { m_forceFunction = expr; }
    QString mMotionSpringBase::getForceFunction() const { return m_forceFunction; }

    void mMotionSpringBase::setDampingFunction(const QString& expr) { m_dampingFunction = expr; }
    QString mMotionSpringBase::getDampingFunction() const { return m_dampingFunction; }

    void mMotionSpringBase::setDampingEnabled(bool enabled) { m_dampingEnabled = enabled; }
    bool mMotionSpringBase::isDampingEnabled() const { return m_dampingEnabled; }

}
