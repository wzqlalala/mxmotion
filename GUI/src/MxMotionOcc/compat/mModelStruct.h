#pragma once

#include "mBasicEnum.h"
#include "mxr/data/SelectDataStruct.h"
#include <QString>
#include <QVariant>
#include <QVector3D>
#include <utility>

struct Unit {
    QString name;
    QString symbol;
    double scale = 1.0;
};

struct DirectionPickState {
    int mode = 0;
    bool reversed = false;
    QVariant startPoint;
    QVariant endPoint;
    std::pair<MxFunctions::PickDataType, RenderPickResultData> selectedObject{
        MxFunctions::PickDataType::None, RenderPickResultData{}};
    QVector3D directInputDir;
};
