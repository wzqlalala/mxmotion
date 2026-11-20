#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
//#include "MPreData_global.h"

//std
#include <memory>

//Qt
#include <QMdiSubWindow>
#include <QString>
#include <set>
#include <QTreeWidgetItem>

#include "mxr/basic/PublicStruct.h"

#include "mBasicEnum.h"
//#include "mPostGUIData.h"
#include "mModelStruct.h"
//#include "mModelData.h"
#include "mxr/base/mxr_export.h"
#include "mxr/basic/PublicStruct.h"
#include "mxr/base/PublicFunction.h"

#include"MPreData_global.h"
//#include "MPreData_global.h"

#include "mBasicEnum.h"
#include "calculate_line.h"
#include "../../src/Common/default.h"
class PickManager  {
    
public:

    static MPREDATA_EXPORT bool pickPointMultiAverage(QVector3D& result);
    static MPREDATA_EXPORT  bool pickTwoEdgesForAxis(MechCommon::DefaultLine& outLine); 
    static MPREDATA_EXPORT bool pickDirectionAuto(QVector3D& direction);
    static MPREDATA_EXPORT bool pickSinglePoint(QVector3D& result);

    static MPREDATA_EXPORT int getSelectedEdgeCount();
    static MPREDATA_EXPORT std::vector<QVector3D>  getSelectedPoints();
    static MPREDATA_EXPORT bool getDirectionFromLastPick(QVector3D& direction);
    static MPREDATA_EXPORT QVector3D getDirectionFromLastPick();


    //新增多面拾取
    // 获取所有选中几何面的面心及法向量（用于单面法线）
    static MPREDATA_EXPORT  bool getSelectedFaceCenterAndNormal(QVector3D& center, QVector3D& normal);
    // 获取所有选中几何面的面心列表（用于多面拟合）
    static  MPREDATA_EXPORT std::vector<QVector3D> getSelectedFaceCenters();
    static  MPREDATA_EXPORT float getModelDiagonalLength(float defaultLen = 10.0f);
};