#pragma once
#pragma once
#include <QVector3D>

namespace MechCommon
{
    struct DefaultLine
    {
        QVector3D start;
        QVector3D end;
        bool isValid() const { return !start.isNull() || !end.isNull(); }
    };
}