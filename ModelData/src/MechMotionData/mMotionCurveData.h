#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>

#include "MXMotionData_global.h"
#include "mPhysicalFieldEnum.h"

#include <QVector3D>
#include <QVector>
#include <QString>

using namespace MxFunctions;
namespace MechMotionData {
    /**
      * @brief 驱动基类
      */
    class MXMOTIONDATA_EXPORT mMotionCurveData {
    public:
        mMotionCurveData(int id, const QString& name = "");
        mMotionCurveData() {};
        ~mMotionCurveData();

        int getId() const;
        void setName(const QString& name);
        QString getName() const;

        // --- 曲线数据属性 ---
        QString m_xAxisLabel;        // X轴标签
        QString m_yAxisLabel;        // Y轴标签
        std::vector<double> m_xData; // X轴数据序列
        std::vector<double> m_yData; // Y轴数据序列

        friend QDataStream& operator>>(QDataStream& in, mMotionCurveData& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionCurveData& mgr);

    private:
        int m_id;
        QString m_name;
    };

}
