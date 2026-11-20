#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector>
#include <QPair>

#include "MXMotionData_global.h"

namespace MechMotionData {

    enum class MXMOTIONDATA_EXPORT ControlInputType {
        Constant,      ///< 常量
        Linear,        ///< 线性
        Quadratic,     ///< 二次多项式
        Cubic,         ///< 三次多项式
        CustomCurve    ///< 自定义曲线
    };

    class MXMOTIONDATA_EXPORT mMotionControlInput {
    public:
        mMotionControlInput(int id, const QString& name = "");
        mMotionControlInput();
        ~mMotionControlInput();

        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        QString getName() const;

        void setInputType(ControlInputType type);
        ControlInputType getInputType() const;

        void setTimeStart(double t);
        double getTimeStart() const;

        void setTimeEnd(double t);
        double getTimeEnd() const;

        // 多项式系数 a + b*t + c*t^2 + d*t^3
        void setPolyCoeffs(const QVector<double>& coeffs);
        QVector<double> getPolyCoeffs() const;

        // 自定义曲线点 (t, value)
        void setCurvePoints(const QVector<QPair<double, double>>& points);
        QVector<QPair<double, double>> getCurvePoints() const;

        friend QDataStream& operator>>(QDataStream& in, mMotionControlInput& obj);
        friend QDataStream& operator<<(QDataStream& out, mMotionControlInput& obj);

    private:
        int m_id;
        QString m_name;
        ControlInputType m_inputType;
        double m_timeStart;
        double m_timeEnd;
        QVector<double> m_polyCoeffs;
        QVector<QPair<double, double>> m_curvePoints;
    };

}
