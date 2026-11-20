#include "mBoundaryPanelBase.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mInfoWindow.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include "PickCommonFunction.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>

#include "mLoadBoundaryBase.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mBoundaryPanelBase::mBoundaryPanelBase(QWidget* parent)
        : mPanelBase(parent)
    {}

    mBoundaryPanelBase::~mBoundaryPanelBase()
    {}

    void mBoundaryPanelBase::setBoundaryData(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData, const int& caseId)
    {
        _boundaryData = boundaryData;
        _caseId = caseId;
    }

}
