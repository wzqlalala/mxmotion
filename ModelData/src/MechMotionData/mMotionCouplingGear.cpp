#include "mMotionCouplingGear.h"

#include <QMatrix4x4>
#include <cmath>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionCouplingGear::mMotionCouplingGear(int id, CouplingType type, const QString& name)
        : mMotionCouplingBase(id, type, name),
        m_gearRatio(1.0),
        m_toothCount1(20),
        m_toothCount2(20),
        m_pitchRadius1(100.0),
        m_pitchRadius2(100.0)
    {
    }

    mMotionCouplingGear::~mMotionCouplingGear() {}

    void mMotionCouplingGear::setGearRatio(double ratio) {
        if (ratio == 0.0)
            ratio = (m_gearRatio < 0.0) ? -0.001 : 0.001; // 0 无意义：保号取最小幅值
        m_gearRatio = ratio;
        syncParametersFromGearRatio();
    }

    double mMotionCouplingGear::getGearRatio() const { return m_gearRatio; }

    void mMotionCouplingGear::setToothCount1(int count) {
        m_toothCount1 = count;
        if (count > 0) {
            // 齿数/分度圆半径是物理正量，幅值按 |传动比| 同步；
            // 传动比符号（负=内啮合）不受齿数编辑影响
            const double mag = std::fabs(m_gearRatio);
            m_toothCount2 = qRound(count * mag);
            m_pitchRadius1 = 100.0;
            m_pitchRadius2 = m_pitchRadius1 * mag;
        }
    }

    int mMotionCouplingGear::getToothCount1() const { return m_toothCount1; }

    void mMotionCouplingGear::setToothCount2(int count) {
        m_toothCount2 = count;
        if (count > 0 && m_toothCount1 > 0) {
            // 由齿数反推传动比时保留符号：内啮合设置不因编辑齿数丢失
            const double sign = (m_gearRatio < 0.0) ? -1.0 : 1.0;
            m_gearRatio = sign * static_cast<double>(count) / m_toothCount1;
            m_pitchRadius1 = 100.0;
            m_pitchRadius2 = m_pitchRadius1 * std::fabs(m_gearRatio);
        }
    }

    int mMotionCouplingGear::getToothCount2() const { return m_toothCount2; }

    void mMotionCouplingGear::setPitchRadius1(double radius) {
        m_pitchRadius1 = radius;
        if (radius > 0.0) {
            const double mag = std::fabs(m_gearRatio);
            m_pitchRadius2 = radius * mag;
            m_toothCount1 = 20;
            m_toothCount2 = qRound(20 * mag);
        }
    }

    double mMotionCouplingGear::getPitchRadius1() const { return m_pitchRadius1; }

    void mMotionCouplingGear::setPitchRadius2(double radius) {
        m_pitchRadius2 = radius;
        if (radius > 0.0 && m_pitchRadius1 > 0.0) {
            const double sign = (m_gearRatio < 0.0) ? -1.0 : 1.0;
            m_gearRatio = sign * radius / m_pitchRadius1;
            m_toothCount1 = 20;
            m_toothCount2 = qRound(20 * std::fabs(m_gearRatio));
        }
    }

    double mMotionCouplingGear::getPitchRadius2() const { return m_pitchRadius2; }

    mMotionCouplingBase* mMotionCouplingGear::copyCoupling(int newId, const QString& newName)
    {
        mMotionCouplingGear* newCoupling = new mMotionCouplingGear(newId, m_type, newName);

        newCoupling->setGearRatio(this->getGearRatio());
        newCoupling->setToothCount1(this->getToothCount1());
        newCoupling->setToothCount2(this->getToothCount2());
        newCoupling->setPitchRadius1(this->getPitchRadius1());
        newCoupling->setPitchRadius2(this->getPitchRadius2());

        return newCoupling;
    }

    void mMotionCouplingGear::syncParametersFromGearRatio()
    {
        // 幅值同步齿数/半径；符号（负=内啮合，两轮同向）只保留在传动比里
        const double mag = std::fabs(m_gearRatio);
        m_toothCount1 = 20;
        m_toothCount2 = qRound(20 * mag);
        m_pitchRadius1 = 100.0;
        m_pitchRadius2 = m_pitchRadius1 * mag;
    }

    QDataStream& operator<<(QDataStream& out, mMotionCouplingGear& mgr) {
        out << static_cast<mMotionCouplingBase&>(mgr);
        out << mgr.m_gearRatio;
        out << mgr.m_toothCount1;
        out << mgr.m_toothCount2;
        out << mgr.m_pitchRadius1;
        out << mgr.m_pitchRadius2;
        return out;
    }

    QDataStream& operator>>(QDataStream& in, mMotionCouplingGear& mgr) {
        in >> static_cast<mMotionCouplingBase&>(mgr);
        in >> mgr.m_gearRatio;
        in >> mgr.m_toothCount1;
        in >> mgr.m_toothCount2;
        in >> mgr.m_pitchRadius1;
        in >> mgr.m_pitchRadius2;
        return in;
    }
}
