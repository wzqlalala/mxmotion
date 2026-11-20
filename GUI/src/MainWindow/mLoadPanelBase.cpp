#include "mLoadPanelBase.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）


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
#include<mElementSurface.h>
#include<mElementSet.h>
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mLoadPanelBase::mLoadPanelBase(QWidget* parent)
        : mPanelBase(parent)
    {}

    mLoadPanelBase::~mLoadPanelBase()
    {}

    void mLoadPanelBase::setLoadData(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, const int& caseId)
    {
        _loadData = loadData;
        _caseId = caseId;
    }

}
