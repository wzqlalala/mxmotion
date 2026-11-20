#include "mSectionShapePanel.h"
#include "mPanelBase.h"
#include "mSectionShapeBase.h"
#include <QTableWidget>
#include <QSplitter>
#include <QGridLayout>
#include <QHeaderView>
#include <QTableWidgetItem>

#include "H_SectionVisual.h"
#include "Rectangle_SectionVisual.h"
#include "Circle_SectionVisual.h"
#include "I_SectionVisual.h"
#include "L_SectionVisual.h"
#include "Annular_SectionVisual.h"
#include "Box_SectionVisual.h"
#include "Polygon_SectionVisual.h"
#include "Channel1_SectionVisual.h"
#include "mPBeam.h"
#include "Channel_SectionVisual.h"
#include "mCustomSectionShape.h"
#include "Channel2_SectionVisual.h"

#include "Box1_SectionVisual.h"
#include "Cross_SectionVisual.h"
#include "Hat_SectionVisual.h"
#include "mGlobalSignals.h"
//#include "TreeWidget.h"
#include "I1_SectionVisual.h"
#include "T_SectionVisual.h"
#include "T1_SectionVisual.h"
#include "T2_SectionVisual.h"
#include "Z_SectionVisual.h"

//MxFunctions
#include "mGlobalSignals.h"
#include "mContainerOperation.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
//label
#include "mxr/label/mSymbolController.h"

//BasicData
#include "mPhysicalFieldEnum.h"
#include "mSectionShapeManager.h"
#include "mModelData.h"
#include "mMeshData.h"

//Geo
#include "mGeoManager.h"
//include "mGeoDataModelSet.h" //【OCC摘除·S10】毒头（拉 OCC 链/冻结岛/出列件）死引用清剿
#include "PickCommonFunction.h"
#include <mEditDiagnosis.h>
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include <mMechGlobalSignals.h>
//
#include "mDockLayoutManager.h"
#include "DockWidget.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
//截面形状
//include "GeoWidget.h" //【OCC摘除·S10】GeoWidget 出列（OCC viewer）
#include <Sample2D_Markers.h>
  //【恢复】头实际存在（清剿误伤回补）

//#include <Prs3d_Drawer.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <AIS_InteractiveContext.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <AIS_LengthDimension.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <GProp_GProps.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役

//#include "BRepGProp_Face.hxx" //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include "BRepGProp.hxx" //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <BRepBndLib.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
#include "beamView.h"
#include "mOtherFunction.h"

//截面属性计算(occsp: SolidSection/ShellSection/SectionGeometry)
//include "SolidSection.h" //【OCC摘除·S10】旧栈冻结岛/出列件死引用（终扫）
//include "ShellSection.h" //【OCC摘除·S10】旧栈冻结岛/出列件死引用（终扫）
//include "SectionGeometry.h" //【OCC摘除·S10】occsp 族随冻结岛退役

#include <sstream>
#include <string>
#include <vector>
#include <utility>

using namespace MechCommon;
using namespace MechSectionShapeData;
using namespace MxBeam;
namespace MechMainWindow {

    mSectionShapePanel::mSectionShapePanel(QWidget* parent)
        : mGeoPanelBase(parent)
    {
        buildUI();
        _sectionShapeData = nullptr;

    }

    void mSectionShapePanel::newSectionShapePanel(int type)
    {
        if (!getCVM)
        {
            return;
        }

        // 注意：视口切换(PropShape/主渲染)不能放在这里，
        // 因为后续 showGeoWidget/slot_RebuildSec 还会 showPropShapeViewport 覆盖掉，
        // 移到本函数末尾统一处理

        //新建
        int id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).createID();
        // 新建后立即设为当前ID，避免 slot_RebuildSec 用旧 _currentID 取到错误截面
        getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).setCurrentID(id);
        //创建数据

        if (type == MxFunctions::Sec_H)
        {
            _sectionShapeData = new H_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "H"), id);
        }
        else if (type == MxFunctions::Sec_I)
        {
            _sectionShapeData = new I_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "I"), id);
        }
        else if (type == MxFunctions::Sec_L)
        {
            _sectionShapeData = new L_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "L"), id);
        }
        else if (type == MxFunctions::Sec_T)
        {
            _sectionShapeData = new T_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "T"), id);
        }
        else if (type == MxFunctions::Sec_Box)
        {
            _sectionShapeData = new Box_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Box"), id);
        }
        else if (type == MxFunctions::Sec_Rec)
        {
            _sectionShapeData = new Rectangle_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Rec"), id);
        }
        else if (type == MxFunctions::Sec_Cir)
        {
            _sectionShapeData = new Circle_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Cir"), id);
        }
        else if (type == MxFunctions::Sec_Ann)
        {
            _sectionShapeData = new Annular_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Ann"), id);
        }
        else if (type == MxFunctions::Sec_Channel)
        {
            _sectionShapeData = new Channel_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Channel"), id);
        }
        else if (type == MxFunctions::Sec_Channel1)
        {
            _sectionShapeData = new Channel1_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Channel1"), id);
        }
        else if (type == MxFunctions::Sec_Channel2)
        {
            _sectionShapeData = new Channel2_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Channel2"), id);
        }
        else if (type == MxFunctions::Sec_I1)
        {
            _sectionShapeData = new I1_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "I1"), id);
        }
        else if (type == MxFunctions::Sec_T1)
        {
            _sectionShapeData = new T1_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "T1"), id);
        }
        else if (type == MxFunctions::Sec_T2)
        {
            _sectionShapeData = new T2_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "T2"), id);
        }
        else if (type == MxFunctions::Sec_Box1)
        {
            _sectionShapeData = new Box1_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Box1"), id);
        }
        else if (type == MxFunctions::Sec_Cross)
        {
            _sectionShapeData = new Cross_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Cross"), id);
        }
        else if (type == MxFunctions::Sec_Hat)
        {
            _sectionShapeData = new Hat_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Hat"), id);
        }
        else if (type == MxFunctions::Sec_Z)
        {
            _sectionShapeData = new Z_SectionVisual(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Z"), id);
        }
        else if (type == MxFunctions::Sec_None)
        {
            _sectionShapeData = new mCustomSectionShape(getCVM->getModelData()->createNameByType(Physical_PropertyShape, "Custom"), id);
        }
//MIG         getCVM->getModelData()->addPhysicalField<mSectionShapeBase>(_sectionShapeData);



        //截面形状视图
//MIG         showGeoWidget(type);


        //创建成功发送消息
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建截面形状 %1").arg(_sectionShapeData->getSectionShapeName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSectionShapeSig(true, _sectionShapeData->getSectionShapeId());

        // 统一在最后切换视口：自定义截面恢复主渲染视口(便于拾取几何)，
        // 其他截面保持 PropShape 预览图；不能提前，否则会被 showGeoWidget 覆盖
        updateGeoPartsVisibleForSection(type == MxFunctions::Sec_None);
    }
    void mSectionShapePanel::createCopyPanel(std::vector<int> copyId)
    {
        for (auto id : copyId)
        {
            _sectionShapeData = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(id);
//MIG             showGeoWidget(getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(id)->getSectionShapeType());
        }
    }
    void mSectionShapePanel::clearGeoWidget()
    {
    //【OCC摘除·S10 保档】截面形状子窗口载体 GeoWidget(OCC viewer)随出列下线
//{
//        QWidget* widget = VMManager->getPropShapeWidget();
//        GeoWidget* geo = qobject_cast<GeoWidget*>(widget);
//        geo->getAisContext()->EraseAll(true);
//    }
}
    void mSectionShapePanel::showGeoWidget(int type)
    {
    //【OCC摘除·S10 保档】同上（showPropShapeViewport 保壳，见 mViewportModelManager）
//{
//        //VMManager->showPropShapeViewport();

//        QWidget* widget = VMManager->getPropShapeWidget();
//        GeoWidget* geo = qobject_cast<GeoWidget*>(widget);
//        if (geo == nullptr)
//        {
//            return;
//        }
//        if (type == MxFunctions::Sec_None)
//        {
//            mCustomSectionShape* sec =
//                dynamic_cast<mCustomSectionShape*>(_sectionShapeData);
//            /*H_SectionVisual* HSec = new H_SectionVisual();*/
//            sec->CreateSec();
//            _customPara = sec->getCustomPara();
//            setCustomPara(_customPara);
//            geo->getAisContext()->EraseAll(true);
//            //slotShowParaOnPanel(sec->getPara(), sec->getNsm(), sec->getSectionShapeName());
//            //没有data面板
//            //没有occ视图
//        }
//        // 只有自定义截面才显示构建梁截面拾取区
//        updateBuildSectionFoldVisible();
//        // 自定义截面不显示截面参数区
//        if (type == MxFunctions::Sec_None)
//            _foldParam->hide();
//        else
//            _foldParam->show();
//        // 新建自定义截面时按当前拾取下拉框(几何面/几何线)切换拾取模式，
//        // 不依赖 currentIndexChanged 信号(初始不会触发)
//        if (type == MxFunctions::Sec_None)
//        {
//            geoPickWidget->_featPick = None;
//            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
//            geoPickWidget->_featPick = FeatureObj;
//        }
//        //disconnect(this, &mSectionShapePanel::signal_RebuildSec, nullptr, nullptr);
//        if (type == MxFunctions::Sec_H)
//        {
//            H_SectionVisual* HSec =
//                dynamic_cast<H_SectionVisual*>(_sectionShapeData);
//            /*H_SectionVisual* HSec = new H_SectionVisual();*/
//            HSec->CreateHSec();
//            _para = HSec->getPara();
//            _para = HSec->getPara();
//            slotShowParaOnPanel(_para, HSec->getNsm(), "H_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(HSec->getData());


//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = HSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();

//            geo->getAisContext()->Display(HSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(HSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(HSec->_a25LenDim, true);
//            geo->getAisContext()->Display(HSec->_a67LenDim, true);
//            geo->getAisContext()->Display(HSec->_a1112LenDim, true);
//            geo->getAisContext()->Display(HSec->_a310LenDim, true);
//            geo->setReferencepoint();


//        }
//        else if (type == MxFunctions::Sec_L)
//        {


//            L_SectionVisual* LSec =
//                dynamic_cast<L_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            LSec->CreateLSec();
//            _para = LSec->getPara();
//            slotShowParaOnPanel(_para, LSec->getNsm(), "L_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(LSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = LSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(LSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(LSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(LSec->_a16LenDim, true);
//            geo->getAisContext()->Display(LSec->_a12LenDim, true);
//            geo->getAisContext()->Display(LSec->_a23LenDim, true);
//            geo->getAisContext()->Display(LSec->_a34LenDim, true);
//            geo->setReferencepoint();
//            geo->_aisshape = LSec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_I)
//        {

//            I_SectionVisual* ISec =
//                dynamic_cast<I_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            ISec->CreateISec();
//            _para = ISec->getPara();
//            slotShowParaOnPanel(_para, ISec->getNsm(), "I_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(ISec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = ISec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(ISec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(ISec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(ISec->_a112LenDim, true);
//            geo->getAisContext()->Display(ISec->_E612LenDim, true);
//            geo->getAisContext()->Display(ISec->_E39LenDim, true);
//            geo->getAisContext()->Display(ISec->_P1112LenDim, true);
//            geo->getAisContext()->Display(ISec->_P78LenDim, true);
//            geo->getAisContext()->Display(ISec->_P67LenDim, true);
//            geo->setReferencepoint();
//            geo->_aisshape = ISec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_I1)
//        {

//            I1_SectionVisual* I1Sec =
//                dynamic_cast<I1_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            I1Sec->CreateI1Sec();
//            _para = I1Sec->getPara();
//            slotShowParaOnPanel(_para, I1Sec->getNsm(), "I1_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(I1Sec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = I1Sec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(I1Sec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(I1Sec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(I1Sec->_a811LenDim, true);
//            geo->getAisContext()->Display(I1Sec->_a23LenDim, true);
//            geo->getAisContext()->Display(I1Sec->_a16LenDim, true);
//            geo->getAisContext()->Display(I1Sec->_a310LenDim, true);
//            geo->setReferencepoint();
//            geo->_aisshape = I1Sec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_T)
//        {

//            T_SectionVisual* TSec =
//                dynamic_cast<T_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            TSec->CreateTSec();
//            _para = TSec->getPara();
//            slotShowParaOnPanel(_para, TSec->getNsm(), "T_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(TSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = TSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(TSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(TSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(TSec->_a18LenDim, true);
//            geo->getAisContext()->Display(TSec->_a12LenDim, true);
//            geo->getAisContext()->Display(TSec->_a36LenDim, true);
//            geo->getAisContext()->Display(TSec->_E84LenDim, true);
//            geo->setReferencepoint();
//            geo->_aisshape = TSec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_T1)
//        {


//            T1_SectionVisual* T1Sec =
//                dynamic_cast<T1_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            T1Sec->CreateT1Sec();
//            _para = T1Sec->getPara();
//            slotShowParaOnPanel(_para, T1Sec->getNsm(), "T1_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(T1Sec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = T1Sec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(T1Sec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(T1Sec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(T1Sec->_a45LenDim, true);
//            geo->getAisContext()->Display(T1Sec->_E68LenDim, true);
//            geo->getAisContext()->Display(T1Sec->_a34LenDim, true);
//            geo->getAisContext()->Display(T1Sec->_a27LenDim, true);
//            geo->setReferencepoint();
//            geo->_aisshape = T1Sec->_aisshape;


//        }
//        else if (type == MxFunctions::Sec_T2)
//        {

//            T2_SectionVisual* T2Sec =
//                dynamic_cast<T2_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            T2Sec->CreateT2Sec();
//            _para = T2Sec->getPara();
//            slotShowParaOnPanel(_para, T2Sec->getNsm(), "T2_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(T2Sec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = T2Sec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(T2Sec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(T2Sec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(T2Sec->_a45LenDim, true);
//            geo->getAisContext()->Display(T2Sec->_E68LenDim, true);
//            geo->getAisContext()->Display(T2Sec->_a34LenDim, true);
//            geo->getAisContext()->Display(T2Sec->_a27LenDim, true);
//            geo->setReferencepoint();
//            geo->_aisshape = T2Sec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_Z)
//        {

//            Z_SectionVisual* ZSec =
//                dynamic_cast<Z_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            ZSec->CreateZSec();
//            _para = ZSec->getPara();
//            slotShowParaOnPanel(_para, ZSec->getNsm(), "Z_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(ZSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = ZSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(ZSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(ZSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(ZSec->_a12LenDim, true);
//            geo->getAisContext()->Display(ZSec->_E27LenDim, true);
//            geo->getAisContext()->Display(ZSec->_a3LenDim, true);
//            geo->getAisContext()->Display(ZSec->_a15LenDim, true);
//            geo->setReferencepoint();
//            geo->_aisshape = ZSec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_Box)
//        {

//            Box_SectionVisual* BoxSec =
//                dynamic_cast<Box_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            BoxSec->CreateBoxSec();
//            _para = BoxSec->getPara();
//            slotShowParaOnPanel(_para, BoxSec->getNsm(), "Box_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(BoxSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = BoxSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(BoxSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(BoxSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(BoxSec->_a14LenDim, true);
//            geo->getAisContext()->Display(BoxSec->_a12LenDim, true);
//            geo->getAisContext()->Display(BoxSec->_E37LenDim, true);
//            geo->getAisContext()->Display(BoxSec->_E26LenDim, true);
//            geo->setReferencepoint();
//            geo->_aisshape = BoxSec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_Box1)
//        {

//            Box1_SectionVisual* Box1Sec =
//                dynamic_cast<Box1_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            Box1Sec->CreateBox1Sec();
//            _para = Box1Sec->getPara();
//            slotShowParaOnPanel(_para, Box1Sec->getNsm(), "Box1_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(Box1Sec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = Box1Sec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(Box1Sec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(Box1Sec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(Box1Sec->_LenDim1, true);
//            geo->getAisContext()->Display(Box1Sec->_aLenDim2, true);
//            geo->getAisContext()->Display(Box1Sec->_aLenDim3, true);
//            geo->getAisContext()->Display(Box1Sec->_LenDim4, true);
//            geo->getAisContext()->Display(Box1Sec->_LenDim5, true);
//            geo->getAisContext()->Display(Box1Sec->_LenDim6, true);
//            geo->setReferencepoint();
//            geo->_aisshape = Box1Sec->_aisshape;


//        }
//        else if (type == MxFunctions::Sec_Rec)
//        {

//            Rectangle_SectionVisual* RecSec =
//                dynamic_cast<Rectangle_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            RecSec->CreatRecSec();
//            _para = RecSec->getPara();
//            slotShowParaOnPanel(_para, RecSec->getNsm(), "Rec_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(RecSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = RecSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(RecSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(RecSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(RecSec->_a14LenDim, true);
//            geo->getAisContext()->Display(RecSec->_a12LenDim, true);

//            geo->setReferencepoint();
//            geo->_aisshape = RecSec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_Cir)
//        {
//            Circle_SectionVisual* CirSec =
//                dynamic_cast<Circle_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            CirSec->CreateCirSec();
//            _para = CirSec->getPara();
//            slotShowParaOnPanel(_para, CirSec->getNsm(), "Cir_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(CirSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = CirSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(CirSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(CirSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(CirSec->_a14LenDim, true);


//            geo->setReferencepoint();
//            geo->_aisshape = CirSec->_aisshape;


//        }
//        else if (type == MxFunctions::Sec_Ann)
//        {

//            Annular_SectionVisual* AnnSec =
//                dynamic_cast<Annular_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            AnnSec->CreateAnuSec();
//            _para = AnnSec->getPara();
//            slotShowParaOnPanel(_para, AnnSec->getNsm(), "Ann_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(AnnSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = AnnSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(AnnSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(AnnSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(AnnSec->_a14LenDim, true);
//            geo->getAisContext()->Display(AnnSec->_aLenDim, true);


//            geo->setReferencepoint();
//            geo->_aisshape = AnnSec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_Channel)
//        {

//            Channel_SectionVisual* ChanSec =
//                dynamic_cast<Channel_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            ChanSec->CreateChanSec();
//            _para = ChanSec->getPara();
//            slotShowParaOnPanel(_para, ChanSec->getNsm(), "Chan_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(ChanSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = ChanSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(ChanSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//            geo->getAisContext()->Display(ChanSec->_a14LenDim, true);
//            geo->getAisContext()->Display(ChanSec->_a12LenDim, true);
//            geo->getAisContext()->Display(ChanSec->_a84LenDim, true);
//            geo->getAisContext()->Display(ChanSec->_aE26LenDim, true);
//            geo->getAisContext()->Display(ChanSec->_shearMarker, Standard_False);

//            geo->setReferencepoint();
//            geo->_aisshape = ChanSec->_aisshape;



//        }
//        else if (type == MxFunctions::Sec_Channel1)
//        {

//            Channel1_SectionVisual* Chan1Sec =
//                dynamic_cast<Channel1_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            Chan1Sec->CreateChan1Sec();
//            _para = Chan1Sec->getPara();
//            slotShowParaOnPanel(_para, Chan1Sec->getNsm(), "Chan1_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(Chan1Sec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = Chan1Sec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(Chan1Sec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//            geo->getAisContext()->Display(Chan1Sec->_aL18LenDim, true);
//            geo->getAisContext()->Display(Chan1Sec->_aR18LenDim, true);
//            geo->getAisContext()->Display(Chan1Sec->_a25LenDim, true);
//            geo->getAisContext()->Display(Chan1Sec->_a16LenDim, true);
//            geo->getAisContext()->Display(Chan1Sec->_shearMarker, Standard_False);

//            geo->setReferencepoint();
//            geo->_aisshape = Chan1Sec->_aisshape;


//            }
//        else if (type == MxFunctions::Sec_Channel2)
//        {

//            Channel2_SectionVisual* Chan2Sec =
//                dynamic_cast<Channel2_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            Chan2Sec->CreateChan2Sec();
//            _para = Chan2Sec->getPara();
//            slotShowParaOnPanel(_para, Chan2Sec->getNsm(), "Chan2_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(Chan2Sec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = Chan2Sec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(Chan2Sec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//            geo->getAisContext()->Display(Chan2Sec->_a18LenDim, true);
//            geo->getAisContext()->Display(Chan2Sec->_a12LenDim, true);
//            geo->getAisContext()->Display(Chan2Sec->_a23LenDim, true);
//            geo->getAisContext()->Display(Chan2Sec->_aE26LenDim, true);
//            geo->getAisContext()->Display(Chan2Sec->_shearMarker, Standard_False);

//            geo->setReferencepoint();
//            geo->_aisshape = Chan2Sec->_aisshape;


//        }
//        else if (type == MxFunctions::Sec_Cross)
//        {
//            Cross_SectionVisual* CrossSec =
//                dynamic_cast<Cross_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            CrossSec->CreateCrossSec();
//            _para = CrossSec->getPara();
//            slotShowParaOnPanel(_para, CrossSec->getNsm(), "Cross_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(CrossSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = CrossSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(CrossSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(CrossSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(CrossSec->_a1012LenDim, true);
//            geo->getAisContext()->Display(CrossSec->_E68LenDim, true);
//            geo->getAisContext()->Display(CrossSec->_a17LenDim, true);
//            geo->getAisContext()->Display(CrossSec->_a911LenDim, true);


//            geo->setReferencepoint();
//            geo->_aisshape = CrossSec->_aisshape;


//        }
//        else if (type == MxFunctions::Sec_Hat)
//        {

//            Hat_SectionVisual* HatSec =
//                dynamic_cast<Hat_SectionVisual*>(_sectionShapeData);
//            /*MxBeam::H_SectionVisual* HSec = new MxBeam::H_SectionVisual();*/
//            HatSec->CreateHatSec();
//            _para = HatSec->getPara();
//            slotShowParaOnPanel(_para, HatSec->getNsm(), "Hat_Section-" + MxFunctions::mNumberToString(mPBeam::getInstance()->_beamNamelist.size(), 10));
//            slotShowDataOnPanel(HatSec->getData());

//            geo->getAisContext()->EraseAll(true);

//            Bnd_Box bbox = Bnd_Box();
//            bbox.SetGap(1e-4);
//            geo->getV3dView()->FitAll(bbox, 0.5, true);
//            geo->_aisshape = HatSec->_aisshape;
//            geo->SetRectangularGridGraphicValues();
//            geo->getAisContext()->Display(HatSec->_aisshape, false);
//            geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//            geo->getAisContext()->Display(HatSec->_shearMarker, Standard_False);
//            geo->getAisContext()->Display(HatSec->_a19LenDim, true);
//            geo->getAisContext()->Display(HatSec->_a812LenDim, true);
//            geo->getAisContext()->Display(HatSec->_a112LenDim, true);
//            geo->getAisContext()->Display(HatSec->_P23LenDim, true);


//            geo->setReferencepoint();
//            geo->_aisshape = HatSec->_aisshape;


//        }
//    }
}
    void mSectionShapePanel::slot_RebuildSec(QList<double> para, int Id, MxFunctions::SecType type)
    {
        VMManager->showPropShapeViewport();
        auto manager = &getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape);
        QWidget* widget = VMManager->getPropShapeWidget();
//MIG         MxBeam::GeoWidget* geo = qobject_cast<MxBeam::GeoWidget*>(widget);
        if (type == MxFunctions::Sec_H)
        {
            H_SectionVisual* HSec =
                dynamic_cast<H_SectionVisual*>(manager->getSectionShapeData(Id));
            if (HSec->getSectionShapeId() == Id)
            {
                HSec->ReBuildHSec(para);
                //截面属性表更新
                if (HSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }

                slotShowDataOnPanel(HSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);

//MIG                 geo->_aisshape = HSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
                auto toLinear = [](std::uint8_t v)
                    {
                        double s = v / 255.0;
                        return s <= 0.04045 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
                    };
//MIG                 Quantity_Color qc(toLinear(63), toLinear(191), toLinear(242), Quantity_TOC_RGB);
//MIG                 geo->_aisshape->SetColor(qc);
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();
//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);

//MIG                 geo->SetRectangularGridGraphicValues();
                //白色图形的更新            


//MIG                 geo->getAisContext()->Display(HSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(HSec->_a25LenDim, true);
//MIG                 geo->getAisContext()->Display(HSec->_a67LenDim, true);
//MIG                 geo->getAisContext()->Display(HSec->_a1112LenDim, true);
//MIG                 geo->getAisContext()->Display(HSec->_a310LenDim, true);
//MIG                 geo->setReferencepoint();
                //下面是绿色图形的更新

//MIG                 geo->getAisContext()->Display(HSec->_aisshape, false);

//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();
            }
        }
        if (type == MxFunctions::Sec_L)
        {
            L_SectionVisual* LSec =
                dynamic_cast<L_SectionVisual*>(manager->getSectionShapeData(Id));
            if (LSec->getSectionShapeId() == Id)
            {
                LSec->initData();
                LSec->ReBuildLSec(para);
                if (LSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(LSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(0);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);

//MIG                 geo->_aisshape = LSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);

//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(LSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(LSec->_a16LenDim, true);
//MIG                 geo->getAisContext()->Display(LSec->_a12LenDim, true);
//MIG                 geo->getAisContext()->Display(LSec->_a23LenDim, true);
//MIG                 geo->getAisContext()->Display(LSec->_a34LenDim, true);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(LSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_I)
        {
            I_SectionVisual* ISec =
                dynamic_cast<I_SectionVisual*>(manager->getSectionShapeData(Id));
            if (ISec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                ISec->ReBuildISec(para);
                if (ISec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(ISec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = ISec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();
//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);

//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(ISec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(ISec->_a112LenDim, true);
//MIG                 geo->getAisContext()->Display(ISec->_E612LenDim, true);
//MIG                 geo->getAisContext()->Display(ISec->_E39LenDim, true);
//MIG                 geo->getAisContext()->Display(ISec->_P1112LenDim, true);
//MIG                 geo->getAisContext()->Display(ISec->_P78LenDim, true);
//MIG                 geo->getAisContext()->Display(ISec->_P67LenDim, true);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(ISec->_aisshape, false);



//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_I1)
        {
            I1_SectionVisual* I1Sec =
                dynamic_cast<I1_SectionVisual*>(manager->getSectionShapeData(Id));
            if (I1Sec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                I1Sec->ReBuildI1Sec(para);
                if (I1Sec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(I1Sec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = I1Sec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();



//MIG                 geo->getAisContext()->Display(I1Sec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(I1Sec->_a811LenDim, true);
//MIG                 geo->getAisContext()->Display(I1Sec->_a23LenDim, true);
//MIG                 geo->getAisContext()->Display(I1Sec->_a16LenDim, true);
//MIG                 geo->getAisContext()->Display(I1Sec->_a310LenDim, true);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(I1Sec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_T)
        {
            T_SectionVisual* TSec =
                dynamic_cast<T_SectionVisual*>(manager->getSectionShapeData(Id));
            if (TSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                TSec->ReBuildTSec(para);
                if (TSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(TSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = TSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(TSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(TSec->_a18LenDim, true);
//MIG                 geo->getAisContext()->Display(TSec->_a12LenDim, true);
//MIG                 geo->getAisContext()->Display(TSec->_a36LenDim, true);
//MIG                 geo->getAisContext()->Display(TSec->_E84LenDim, true);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(TSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_T1)
        {
            T1_SectionVisual* T1Sec =
                dynamic_cast<T1_SectionVisual*>(manager->getSectionShapeData(Id));
            if (T1Sec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                T1Sec->ReBuildT1Sec(para);
                if (T1Sec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(T1Sec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = T1Sec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(T1Sec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(T1Sec->_a45LenDim, true);
//MIG                 geo->getAisContext()->Display(T1Sec->_E68LenDim, true);
//MIG                 geo->getAisContext()->Display(T1Sec->_a34LenDim, true);
//MIG                 geo->getAisContext()->Display(T1Sec->_a27LenDim, true);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(T1Sec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_T2)
        {
            T2_SectionVisual* T2Sec =
                dynamic_cast<T2_SectionVisual*>(manager->getSectionShapeData(Id));
            if (T2Sec->getSectionShapeId() == Id)
            {
                //ISec->initData();

                T2Sec->ReBuildT2Sec(para);
                if (T2Sec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }

                slotShowDataOnPanel(T2Sec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);

//MIG                 geo->_aisshape = T2Sec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();
//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(T2Sec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(T2Sec->_a45LenDim, true);
//MIG                 geo->getAisContext()->Display(T2Sec->_E68LenDim, true);
//MIG                 geo->getAisContext()->Display(T2Sec->_a34LenDim, true);
//MIG                 geo->getAisContext()->Display(T2Sec->_a27LenDim, true);
//MIG                 geo->setReferencepoint();

//MIG                 geo->getAisContext()->Display(T2Sec->_aisshape, false);



//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Z)
        {
            Z_SectionVisual* ZSec =
                dynamic_cast<Z_SectionVisual*>(manager->getSectionShapeData(Id));
            if (ZSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                ZSec->ReBuildZSec(para);
                if (ZSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(ZSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);

//MIG                 geo->_aisshape = ZSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(ZSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(ZSec->_a12LenDim, true);
//MIG                 geo->getAisContext()->Display(ZSec->_E27LenDim, true);
//MIG                 geo->getAisContext()->Display(ZSec->_a3LenDim, true);
//MIG                 geo->getAisContext()->Display(ZSec->_a15LenDim, true);
//MIG                 geo->setReferencepoint();

//MIG                 geo->getAisContext()->Display(ZSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Box)
        {
            Box_SectionVisual* BoxSec =
                dynamic_cast<Box_SectionVisual*>(manager->getSectionShapeData(Id));
            if (BoxSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                BoxSec->ReBuildBoxSec(para);
                if (BoxSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(BoxSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = BoxSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(BoxSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(BoxSec->_a14LenDim, true);
//MIG                 geo->getAisContext()->Display(BoxSec->_a12LenDim, true);
//MIG                 geo->getAisContext()->Display(BoxSec->_E37LenDim, true);
//MIG                 geo->getAisContext()->Display(BoxSec->_E26LenDim, true);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(BoxSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Box1)
        {
            Box1_SectionVisual* Box1Sec =
                dynamic_cast<Box1_SectionVisual*>(manager->getSectionShapeData(Id));
            if (Box1Sec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                Box1Sec->ReBuildBox1Sec(para);
                if (Box1Sec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(Box1Sec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = Box1Sec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(Box1Sec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(Box1Sec->_LenDim1, true);
//MIG                 geo->getAisContext()->Display(Box1Sec->_aLenDim2, true);
//MIG                 geo->getAisContext()->Display(Box1Sec->_aLenDim3, true);
//MIG                 geo->getAisContext()->Display(Box1Sec->_LenDim4, true);
//MIG                 geo->getAisContext()->Display(Box1Sec->_LenDim5, true);
//MIG                 geo->getAisContext()->Display(Box1Sec->_LenDim6, true);

//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(Box1Sec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Rec)
        {
            Rectangle_SectionVisual* RecSec =
                dynamic_cast<Rectangle_SectionVisual*>(manager->getSectionShapeData(Id));
            if (RecSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                RecSec->ReBuildRecSec(para);
                if (RecSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(RecSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = RecSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(RecSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(RecSec->_a14LenDim, true);
//MIG                 geo->getAisContext()->Display(RecSec->_a12LenDim, true);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(RecSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Cir)
        {
            Circle_SectionVisual* CirSec =
                dynamic_cast<Circle_SectionVisual*>(manager->getSectionShapeData(Id));
            if (CirSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                CirSec->ReBuildCirSec(para);
                if (CirSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(CirSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);

//MIG                 geo->_aisshape = CirSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();
//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(CirSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(CirSec->_a14LenDim, true);

//MIG                 geo->setReferencepoint();

//MIG                 geo->getAisContext()->Display(CirSec->_aisshape, false);



//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Ann)
        {
            Annular_SectionVisual* AnnSec =
                dynamic_cast<Annular_SectionVisual*>(manager->getSectionShapeData(Id));
            if (AnnSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                AnnSec->ReBuildAnnSec(para);
                if (AnnSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(AnnSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = AnnSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();
//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();

//MIG                 geo->getAisContext()->Display(AnnSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(AnnSec->_a14LenDim, true);
//MIG                 geo->getAisContext()->Display(AnnSec->_aLenDim, true);

//MIG                 geo->setReferencepoint();



//MIG                 geo->getAisContext()->Display(AnnSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Channel)
        {
            Channel_SectionVisual* ChanSec =
                dynamic_cast<Channel_SectionVisual*>(manager->getSectionShapeData(Id));
            if (ChanSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                ChanSec->ReBuildChanSec(para);
                if (ChanSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(ChanSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = ChanSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();



//MIG                 geo->getAisContext()->Display(ChanSec->_a14LenDim, true);
//MIG                 geo->getAisContext()->Display(ChanSec->_a12LenDim, true);
//MIG                 geo->getAisContext()->Display(ChanSec->_a84LenDim, true);
//MIG                 geo->getAisContext()->Display(ChanSec->_aE26LenDim, true);
//MIG                 geo->getAisContext()->Display(ChanSec->_shearMarker, Standard_False);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(ChanSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Channel1)
        {
            Channel1_SectionVisual* Chan1Sec =
                dynamic_cast<Channel1_SectionVisual*>(manager->getSectionShapeData(Id));
            if (Chan1Sec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                Chan1Sec->ReBuildChan1Sec(para);
                if (Chan1Sec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(Chan1Sec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = Chan1Sec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();
//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(Chan1Sec->_aL18LenDim, true);
//MIG                 geo->getAisContext()->Display(Chan1Sec->_aR18LenDim, true);
//MIG                 geo->getAisContext()->Display(Chan1Sec->_a25LenDim, true);
//MIG                 geo->getAisContext()->Display(Chan1Sec->_a16LenDim, true);
//MIG                 geo->getAisContext()->Display(Chan1Sec->_shearMarker, Standard_False);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(Chan1Sec->_aisshape, false);



//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Channel2)
        {
            Channel2_SectionVisual* Chan2Sec =
                dynamic_cast<Channel2_SectionVisual*>(manager->getSectionShapeData(Id));
            if (Chan2Sec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                Chan2Sec->ReBuildChan2Sec(para);
                if (Chan2Sec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(Chan2Sec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = Chan2Sec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();
//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(Chan2Sec->_a18LenDim, true);
//MIG                 geo->getAisContext()->Display(Chan2Sec->_a12LenDim, true);
//MIG                 geo->getAisContext()->Display(Chan2Sec->_a23LenDim, true);
//MIG                 geo->getAisContext()->Display(Chan2Sec->_aE26LenDim, true);
//MIG                 geo->getAisContext()->Display(Chan2Sec->_shearMarker, Standard_False);
//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(Chan2Sec->_aisshape, false);



//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Cross)
        {
            Cross_SectionVisual* CrossSec =
                dynamic_cast<Cross_SectionVisual*>(manager->getSectionShapeData(Id));
            if (CrossSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                CrossSec->ReBuildCrossSec(para);
                if (CrossSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(CrossSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);

//MIG                 geo->_aisshape = CrossSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(CrossSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(CrossSec->_a1012LenDim, true);
//MIG                 geo->getAisContext()->Display(CrossSec->_E68LenDim, true);
//MIG                 geo->getAisContext()->Display(CrossSec->_a17LenDim, true);
//MIG                 geo->getAisContext()->Display(CrossSec->_a911LenDim, true);

//MIG                 geo->setReferencepoint();

//MIG                 geo->getAisContext()->Display(CrossSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }
        if (type == MxFunctions::Sec_Hat)
        {
            Hat_SectionVisual* HatSec =
                dynamic_cast<Hat_SectionVisual*>(manager->getSectionShapeData(Id));
            if (HatSec->getSectionShapeId() == Id)
            {
                //ISec->initData();
                HatSec->ReBuildHatSec(para);
                if (HatSec->getData().isEmpty())
                {
//MIG                     geo->getAisContext()->EraseAll(true);
                    return;
                }
                slotShowDataOnPanel(HatSec->getData());
//MIG                 Bnd_Box bbox = Bnd_Box();
//MIG                 bbox.SetGap(1e-4);
//MIG                 geo->getAisContext()->EraseAll(true);
//MIG                 geo->getV3dView()->FitAll(bbox, 0.5, true);
//MIG                 geo->_aisshape = HatSec->_aisshape;
//MIG                 geo->_aisshape->SetDisplayMode(AIS_Shaded);
//MIG                 geo->_aisshape->SetColor(Quantity_Color(0.046, 0.523, 0.896, Quantity_TOC_RGB));
//MIG                 geo->_aisshape->SetTransparency(0.4);
//MIG                 auto shape = geo->_aisshape->Shape();

//MIG                 bbox.SetGap(1e-4);
//MIG                 BRepBndLib  brepbndlib;
//MIG                 brepbndlib.Add(shape, bbox, true);
//MIG                 geo->SetRectangularGridGraphicValues();


//MIG                 geo->getAisContext()->Display(HatSec->_shearMarker, Standard_False);
//MIG                 geo->getAisContext()->Display(HatSec->_a19LenDim, true);
//MIG                 geo->getAisContext()->Display(HatSec->_a812LenDim, true);
//MIG                 geo->getAisContext()->Display(HatSec->_a112LenDim, true);
//MIG                 geo->getAisContext()->Display(HatSec->_P23LenDim, true);

//MIG                 geo->setReferencepoint();


//MIG                 geo->getAisContext()->Display(HatSec->_aisshape, false);


//MIG                 geo->getAisContext()->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));

//MIG                 geo->slotShowaisOriginTrihedron();

            }
        }


    }
    mSectionShapePanel::~mSectionShapePanel() = default;


    void mSectionShapePanel::buildUI()
    {
        // 基类 mGeoPanelBase 已创建 _mainLayout，这里直接复用，
        // 不能再 new QVBoxLayout(this)，否则整个面板布局失效
        auto* mainLay = _mainLayout;
        mainLay->setContentsMargins(2, 2, 2, 2);
        mainLay->setSpacing(2);
        //拾取区

        QPushButton* sectionOkBtn = new QPushButton("确认", this);
        connect(sectionOkBtn, &QPushButton::clicked, this, &mSectionShapePanel::slot_sectionAction);
        sectionShape = new QComboBox(this);
        sectionShape->addItems({ "实体截面","薄壁壳截面" });
        connect(sectionShape, QOverload<int>::of(&QComboBox::activated), [this](int) {
            updateSectionPickMode();
            });

        // 拾取类型切换时清空表格、切换拾取模式并刷新布局
        connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int) {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            clearTableItems(pickObjTable);
            geoPickWidget->_featPick = FeatureObj;
            if (!m_FoldSections.isEmpty())
                m_FoldSections[0]->refreshLayout();
            });

        // 点拾取按钮时切回几何对象拾取模式并高亮已拾取几何(参照 mGeoTransformPanel)
        connect(geoPickWidget->pickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            geoPickWidget->clearHighlight();
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = FeatureObj;
            });

        // 拾取到的几何ID填充进 pickObjTable；视口切换时参照
        // mGeoTransformPanel::slot_subWindowActivated 的方式重连拾取信号
        connect(VMManager, &MechPreData::mViewportModelManager::currentViewportChanged,
            this, &mSectionShapePanel::slot_subWindowActivated);
        slot_subWindowActivated();

        // 网格尺寸输入(实体截面用)
        sectionMeshAuto = new QCheckBox("自动", this);
        sectionMeshAuto->setChecked(true);
        sectionMeshSize = new MxFunctions::mCalcLineEdit("0", this);
        sectionMeshSize->setEnabled(false);
        //sectionMeshSizeUnit = new QLabel("mm", this);
        connect(sectionMeshAuto, &QCheckBox::toggled, this, [this](bool checked) {
            sectionMeshSize->setEnabled(!checked);
            });
        // 壳厚度(薄壁壳截面用)，修改后同步到表格中每条几何线
        sectionShellThickness = new MxFunctions::mCalcLineEdit("0.1", this);
        sectionShellThicknessUnit = new QLabel("mm", this);
        connect(sectionShellThickness, &QLineEdit::editingFinished, this, [this]() {
            if (1 != sectionShape->currentIndex())
                return;
            for (int i = 0; i < pickObjTable->rowCount(); ++i)
            {
                auto* item = pickObjTable->item(i, 1);
                if (item)
                    item->setText(sectionShellThickness->text());
                else
                    pickObjTable->setItem(i, 1, new QTableWidgetItem(sectionShellThickness->text()));
            }
            });
        // 重合容差，用于由拾取线构面/壳网络端点合并
        sectionTolerance = new MxFunctions::mCalcLineEdit("0.01", this);
        sectionToleranceUnit = new QLabel("mm", this);

        sectionToleranceLabel = new PanelLabel("重合容差");
        sectionShellThicknessLabel = new PanelLabel("壳厚度");
        sectionMeshSizeLabel = new PanelLabel("网格尺寸");

        // ============ 梁截面平面法向(参照分割面 initSplitFaceFold 的自定义向量逻辑) ============
        sectionNormalCombo = new QComboBox(this);
        sectionNormalCombo->addItems({ "X轴","Y轴","Z轴","垂直于屏幕","平行于屏幕","自定义","与拾取几何垂直" });
        sectionNormalCombo->setCurrentIndex(6);  // 默认"与拾取几何垂直"(FitToEntities)
        sectionNormalCustomLabel = new QLabel("自定义向量", this);
        sectionNormalCustomVec = new QLineEdit(this);
        sectionNormalCustomVec->setPlaceholderText("1,0,0");
        MxFunctions::mEditDiagnosis::setOnlyVec3(sectionNormalCustomVec);
        sectionNormalPickBtn = new QToolButton(this);
        sectionNormalPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        sectionNormalPickBtn->setToolTip("拾取几何线填充方向");
        connect(sectionNormalPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);  // 几何线
            Highlight(sectionNormalLastLineID, "几何线");
            geoPickWidget->_featPick = SectionNormalCustomVec;
            });
        sectionNormalReverseBtn = new QToolButton(this);
        sectionNormalReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        sectionNormalReverseBtn->setToolTip("反向");
        connect(sectionNormalReverseBtn, &QToolButton::clicked, this, [this]() {
            reverseVecAction(sectionNormalCustomVec);
            });
        connect(sectionNormalCombo, QOverload<int>::of(&QComboBox::activated), [this](int index) {
            const bool isCustom = (index == 5);
            sectionNormalCustomLabel->setVisible(isCustom);
            sectionNormalCustomVec->setVisible(isCustom);
            sectionNormalPickBtn->setVisible(isCustom);
            sectionNormalReverseBtn->setVisible(isCustom);
            if (!isCustom)
                geoPickWidget->_featPick = FeatureObj;
            if (_foldSectionBuild)
                _foldSectionBuild->refreshLayout();
            });
        sectionNormalCustomLabel->setVisible(false);
        sectionNormalCustomVec->setVisible(false);
        sectionNormalPickBtn->setVisible(false);
        sectionNormalReverseBtn->setVisible(false);

        // ============ 截面基准点(剪切中心/形心/自定义，拾取几何点自动填充坐标) ============
        sectionBasePointCombo = new QComboBox(this);
        sectionBasePointCombo->addItems({ "剪切中心","形心","自定义" });
        sectionBasePointCustomLabel = new QLabel("自定义坐标", this);
        sectionBasePointCustomPnt = new QLineEdit(this);
        sectionBasePointCustomPnt->setPlaceholderText("0,0,0");
        MxFunctions::mEditDiagnosis::setOnlyVec3(sectionBasePointCustomPnt);
        sectionBasePointPickBtn = new QToolButton(this);
        sectionBasePointPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        sectionBasePointPickBtn->setToolTip("拾取几何点填充坐标");
        connect(sectionBasePointPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);  // 几何点
            Highlight(sectionBasePointLastPntID, "几何点");
            geoPickWidget->_featPick = SectionBasePointCustomPnt;
            });
        connect(sectionBasePointCombo, QOverload<int>::of(&QComboBox::activated), [this](int index) {
            const bool isCustom = (index == 2);
            sectionBasePointCustomLabel->setVisible(isCustom);
            sectionBasePointCustomPnt->setVisible(isCustom);
            sectionBasePointPickBtn->setVisible(isCustom);
            if (!isCustom)
                geoPickWidget->_featPick = FeatureObj;
            if (_foldSectionBuild)
                _foldSectionBuild->refreshLayout();
            });
        sectionBasePointCustomLabel->setVisible(false);
        sectionBasePointCustomPnt->setVisible(false);
        sectionBasePointPickBtn->setVisible(false);

        // ============ Y轴方向向量(X/Y/Z轴、垂直/平行屏幕、自定义、自动) ============
        sectionYAxisCombo = new QComboBox(this);
        sectionYAxisCombo->addItems({ "X轴","Y轴","Z轴","垂直于屏幕","平行于屏幕","自定义","自动" });
        sectionYAxisCombo->setCurrentIndex(6);  // 默认"自动"(AutoOrient)
        sectionYAxisCustomLabel = new QLabel("自定义向量", this);
        sectionYAxisCustomVec = new QLineEdit(this);
        sectionYAxisCustomVec->setPlaceholderText("0,1,0");
        MxFunctions::mEditDiagnosis::setOnlyVec3(sectionYAxisCustomVec);
        sectionYAxisPickBtn = new QToolButton(this);
        sectionYAxisPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        sectionYAxisPickBtn->setToolTip("拾取几何线填充方向");
        connect(sectionYAxisPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);  // 几何线
            Highlight(sectionYAxisLastLineID, "几何线");
            geoPickWidget->_featPick = SectionYAxisCustomVec;
            });
        sectionYAxisReverseBtn = new QToolButton(this);
        sectionYAxisReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        sectionYAxisReverseBtn->setToolTip("反向");
        connect(sectionYAxisReverseBtn, &QToolButton::clicked, this, [this]() {
            reverseVecAction(sectionYAxisCustomVec);
            });
        connect(sectionYAxisCombo, QOverload<int>::of(&QComboBox::activated), [this](int index) {
            const bool isCustom = (index == 5);
            sectionYAxisCustomLabel->setVisible(isCustom);
            sectionYAxisCustomVec->setVisible(isCustom);
            sectionYAxisPickBtn->setVisible(isCustom);
            sectionYAxisReverseBtn->setVisible(isCustom);
            if (!isCustom)
                geoPickWidget->_featPick = FeatureObj;
            if (_foldSectionBuild)
                _foldSectionBuild->refreshLayout();
            });
        sectionYAxisCustomLabel->setVisible(false);
        sectionYAxisCustomVec->setVisible(false);
        sectionYAxisPickBtn->setVisible(false);
        sectionYAxisReverseBtn->setVisible(false);

        FoldSection* sectionFold = new FoldSection("构建梁截面", this);
        sectionFold->addRow(new PanelLabel("梁截面类型"), sectionShape);
        sectionFold->addRow(sectionToleranceLabel, sectionTolerance, sectionToleranceUnit);
        // 重合容差行下面依次是：梁截面平面法向、截面基准点、Y轴方向向量
        sectionFold->addRow(new PanelLabel("梁截面平面法向"), sectionNormalCombo, sectionNormalPickBtn);
        sectionFold->addRow(sectionNormalCustomLabel, sectionNormalCustomVec, sectionNormalReverseBtn);
        sectionFold->addRow(new PanelLabel("截面基准点"), sectionBasePointCombo, sectionBasePointPickBtn);
        sectionFold->addRow(sectionBasePointCustomLabel, sectionBasePointCustomPnt);
        sectionFold->addRow(new PanelLabel("Y轴方向向量"), sectionYAxisCombo, sectionYAxisPickBtn);
        sectionFold->addRow(sectionYAxisCustomLabel, sectionYAxisCustomVec, sectionYAxisReverseBtn);
        sectionFold->addRow(sectionShellThicknessLabel, sectionShellThickness, sectionShellThicknessUnit);
        sectionFold->addRow(sectionMeshSizeLabel, sectionMeshSize, sectionMeshAuto);
        sectionFold->addRow(geoPickWidget);
        sectionFold->addRow(pickObjTable);
        sectionFold->addRow(sectionOkBtn);
        mainLay->addWidget(sectionFold);
        m_FoldSections.append(sectionFold);
        m_foldWidgets.append(sectionFold);
        // 只有创建自定义截面(mCustomSectionShape)时才显示构建梁截面区
        _foldSectionBuild = sectionFold;

        // 实体截面默认隐藏壳厚度行，薄壁壳截面隐藏网格尺寸行
        sectionFold->refreshLayout();
        updateSectionPickMode();


        /* 1. 参数折叠区 */
        _foldParam = new FoldSection("截面参数", this);
        _paramTable = new QTableWidget(0, 2);
        _paramTable->setHorizontalHeaderLabels({ "参数", "数值" });
        _paramTable->horizontalHeader()->setStretchLastSection(true);
        _paramTable->verticalHeader()->setVisible(false);
        _paramTable->setStyleSheet("font-size:12px");
        _foldParam->addRow(_paramTable);
        mainLay->addWidget(_foldParam);

        /* 2. 数据折叠区 */
        _foldData = new FoldSection("截面属性", this);
        _dataTable = new QTableWidget(0, 2);
        _dataTable->setStyleSheet("font-size:12px");
        _dataTable->setHorizontalHeaderLabels({ "属性", "数值" });
        _dataTable->horizontalHeader()->setStretchLastSection(true);
        _dataTable->verticalHeader()->setVisible(false);

        // 1. 让 table 声明垂直抢占
        _dataTable->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        // 2. 把 table 当成一个 **跨两列的独立控件** 插进去
        //    注意：不要 addRow("标签",table) ！
        auto* hut = new QWidget;
        hut->setFixedHeight(500);

        _foldData->layout()->addRow(hut);   // 标签空着，整行只放 table
        _dataTable->setParent(hut);
        auto* vbox = new QVBoxLayout(hut);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->addWidget(_dataTable);
        /* _foldData->layout()->setWidget(_foldData->layout()->rowCount() - 1,
             QFormLayout::SpanningRole,
             _dataTable);*/
        // _foldData->addRow(_dataTable);

        mainLay->addWidget(_foldData);

        mainLay->addStretch();

        /* 3. 默认收起数据区，只留参数区 */
        //_foldData->setExpanded(false);

        /* 4. 整体缩小 */
        //setMaximumWidth(340);   // 原来约 680
        //setMaximumHeight(400);  // 原来约 800


        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
//MIG         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::clearGeoWidget, this, &mSectionShapePanel::clearGeoWidget);
    }

    void mSectionShapePanel::slotGetParaFormTabel(int r, int c)
    {
        if (_para.isEmpty())
            return;
        int a = _para.at(0);
        // 截面类型与当前数据对象不一致时直接返回，避免切换截面后残留 cellChanged 触发错误编辑
        const SecType curType = _sectionShapeData ? _sectionShapeData->getSectionShapeType() : MxFunctions::Sec_None;
        if (static_cast<int>(curType) != a)
            return;
        switch (a)
        {
        case 0:editCustomPara();
            break;
        case 1: editHSecPara();
            break;
        case 2:editISecPara();
            break;
        case 3:editLSecPara();
            break;
        case 4:editTSecPara();
            break;
        case 5:editBoxSecPara();
            break;
        case 6:editRecSecPara();
            break;
        case 7:editCirSecPara();
            break;
        case 8:editAnnSecPara();
            break;
        case 9:editPolySecPara();
            break;
        case 10:editChanSecPara();
            break;
        case 11:editChan1SecPara();
            break;
        case 12:editChan2SecPara();
            break;
        case 13:editI1SecPara();
            break;

        case 14:editT1SecPara();
            break;
        case 15:editT2SecPara();
            break;
        case 16:editBox1SecPara();
            break;
        case 17:editCrossSecPara();
            break;
        case 18:editHatSecPara();
            break;
        case 19:editZSecPara();
            break;
        }
    }
    void mSectionShapePanel::slotShowParaOnPanel(QList<double> para, QString nsm, QString name)
    {
        _name = name;
        _nsm = nsm;
        _para = para;
        if (_para.isEmpty())
        {
            return;
        }
        int a = _para.at(0);

        switch (a)
        {
        case 0://setCustomPara(para);
            break;
        case 1: setHSecPara(para, nsm);
            break;
        case 2:setISecPara(para, nsm);
            break;
        case 3:setLSecPara(para, nsm);
            break;
        case 4:setTSecPara(para, nsm);
            break;
        case 5:setBoxSecPara(para, nsm);
            break;
        case 6:setRecSecPara(para, nsm);
            break;
        case 7:setCirSecPara(para, nsm);
            break;
        case 8:setAnnSecPara(para, nsm);
            break;
        case 9:setPolySecPara(para, nsm);
            break;
        case 10:setChanSecPara(para, nsm);
            break;
        case 11:setChan1SecPara(para, nsm);
            break;
        case 12:setChan2SecPara(para, nsm);
            break;
        case 13:setI1SecPara(para, nsm);
            break;
        case 14:setT1SecPara(para, nsm);
            break;
        case 15:setT2SecPara(para, nsm);
            break;
        case 16:setBox1SecPara(para, nsm);
            break;
        case 17:setCrossSecPara(para, nsm);
            break;
        case 18:setHatSecPara(para, nsm);
            break;
        case 19:setZSecPara(para, nsm);
            break;
        }
    }

    void mSectionShapePanel::editCustomPara()
    {
        if (_edit)
        {
            for (int i = 0; i < 24; ++i)
            {
                if (_dataTable->item(i, 1) == nullptr)
                    return;
            }

            _para.clear();
            _para.push_back(0.0); // MxFunctions::Sec_None

            QList<QString> customParaList;
            for (int i = 0; i < 24; ++i)
            {
                QString str = _dataTable->item(i, 1)->text();
                double num = str.toDouble();
                _para.push_back(num);
                customParaList.append(str);
            }

            mCustomSectionShape* sec = dynamic_cast<mCustomSectionShape*>(_sectionShapeData);
            if (sec)
            {
                sec->setCustomPara(customParaList);
            }
        }
    }
    void mSectionShapePanel::editHSecPara()
    {

        if (_edit)
        {
            if (_paramTable->item(0, 1) == nullptr ||
                _paramTable->item(1, 1) == nullptr ||
                _paramTable->item(2, 1) == nullptr ||
                _paramTable->item(3, 1) == nullptr
                )
                return;
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int


            if (num3 <= num4)
            {
                setHSecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(1);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            //H_SectionVisual* HSec = new H_SectionVisual(_mainW, _g);
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            H_SectionVisual* HSec =
                dynamic_cast<H_SectionVisual*>(_sectionShapeData);
            HSec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            HSec->setNsm(nsm);
            //emit signal_RebuildSec(_para, Id, MxFunctions::Sec_H);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_H);

        }
    }
    void mSectionShapePanel::editLSecPara()
    {

        if (_edit)
        {
            if (_paramTable->item(0, 1) == nullptr ||
                _paramTable->item(1, 1) == nullptr ||
                _paramTable->item(2, 1) == nullptr ||
                _paramTable->item(3, 1) == nullptr
                )
                return;
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int

            if (num1 <= num3 || num2 <= num4)
            {
                setLSecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(3);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            //H_SectionVisual* HSec = new H_SectionVisual(_mainW, _g);
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            L_SectionVisual* LSec =
                dynamic_cast<L_SectionVisual*>(_sectionShapeData);
            LSec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            LSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_L);


        }
    }
    void mSectionShapePanel::editISecPara()
    {
        if (_edit)
        {
            double temp = 0;
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            QString str5 = _paramTable->item(4, 1)->text();//取出字符串
            double num5 = str5.toDouble();//转成int
            QString str6 = _paramTable->item(5, 1)->text();//取出字符串
            double num6 = str6.toDouble();//转成int
            num1 <= num2 ? temp = num1 : temp = num2;

            if (temp <= num4 || num5 + num6 >= num2)
            {
                setISecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(2);
            for (int i = 0; i < 6; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            I_SectionVisual* ISec =
                dynamic_cast<I_SectionVisual*>(_sectionShapeData);
            ISec->clearData();
            QString nsm = _paramTable->item(6, 1)->text();
            ISec->setNsm(nsm);
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_I);
        }
    }
    void mSectionShapePanel::editI1SecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num1 <= 0 || num3 >= num4)
            {
                setI1SecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(13);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            I1_SectionVisual* I1Sec =
                dynamic_cast<I1_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            I1Sec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            I1Sec->setNsm(nsm);
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_I1);
        }
    }
    void mSectionShapePanel::editTSecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num1 <= num3 || num2 <= num4)
            {
                setTSecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(4);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            T_SectionVisual* TSec =
                dynamic_cast<T_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            TSec->clearData();
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            QString nsm = _paramTable->item(4, 1)->text();
            TSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_T);
        }
    }
    void mSectionShapePanel::editBoxSecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int

            if (num4 >= num1 / 2.0 || num3 >= num2 / 2.0)
            {
                setBoxSecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(5);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Box_SectionVisual* BoxSec =
                dynamic_cast<Box_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            BoxSec->clearData();
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            QString nsm = _paramTable->item(4, 1)->text();
            BoxSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Box);
        }
    }

    void mSectionShapePanel::editRecSecPara()
    {
        if (_edit)
        {
            _para.clear();
            _para.push_back(6);
            for (int i = 0; i < 2; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Rectangle_SectionVisual* RecSec =
                dynamic_cast<Rectangle_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            RecSec->clearData();
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            QString nsm = _paramTable->item(2, 1)->text();
            RecSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Rec);
        }
    }

    void mSectionShapePanel::editCirSecPara()
    {
        if (_edit)
        {

            _para.clear();
            _para.push_back(7);
            for (int i = 0; i < 1; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Circle_SectionVisual* CirSec =
                dynamic_cast<Circle_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.remove(_name);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            CirSec->clearData();
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            QString nsm = _paramTable->item(1, 1)->text();
            CirSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Cir);



            // MBasicFunction::mGlobalSignals::getInstance()->updateBeamInfoSig();
        }
    }

    void mSectionShapePanel::editAnnSecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            if (num1 <= num2)
            {
                setAnnSecPara(_para, _nsm);
                return;
            }

            _para.clear();
            _para.push_back(8);
            for (int i = 0; i < 2; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Annular_SectionVisual* AnnSec =
                dynamic_cast<Annular_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            AnnSec->clearData();
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            QString nsm = _paramTable->item(2, 1)->text();
            AnnSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Ann);
        }
    }

    void mSectionShapePanel::editPolySecPara() {}

    void mSectionShapePanel::editChanSecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num1 <= num4 || 2 * num4 >= num2 || num3 >= num1)
            {
                setChanSecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(10);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Channel_SectionVisual* ChanSec =
                dynamic_cast<Channel_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            ChanSec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            ChanSec->setNsm(nsm);
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Channel);
        }
    }

    void mSectionShapePanel::editChan1SecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if ( num1 <= 0 || num2 <= 0 || num1 <= (num4 - num3)*0.5 || num2 <= (num4 - num3) * 0.5 ||
                num3 >= num4 || num3 <= 0)
            {
                setChan1SecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(11);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Channel1_SectionVisual* Chan1Sec =
                dynamic_cast<Channel1_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            Chan1Sec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            Chan1Sec->setNsm(nsm);
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Channel1);
        }
    }

    void mSectionShapePanel::editChan2SecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num2 >= num3 || 2 * num1 >= num4)
            {
                setChan2SecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(12);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }

            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Channel2_SectionVisual* Chan2Sec =
                dynamic_cast<Channel2_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            Chan2Sec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            Chan2Sec->setNsm(nsm);
            //mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Channel2);
        }
    }


    void mSectionShapePanel::editT1SecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num1 < 1e-6 || num2 < 1e-6 || num3 < 1e-6 || num4 < 1e-6)
            {
                setT1SecPara(_para, _nsm);
                return;
            }
            if (num1 <= num4)
            {
                setT1SecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(14);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            T1_SectionVisual* T1Sec =
                dynamic_cast<T1_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            T1Sec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            T1Sec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_T1);
        }
    }

    void mSectionShapePanel::editT2SecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num1 < 1e-6 || num2 < 1e-6 || num3 < 1e-6 || num4 < 1e-6)
            {
                setT2SecPara(_para, _nsm);
                return;
            }
            if (num1 <= num4 || num2 <= num3)
            {
                setT2SecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(15);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            T2_SectionVisual* T2Sec =
                dynamic_cast<T2_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            T2Sec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            T2Sec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_T2);
        }
    }

    void mSectionShapePanel::editBox1SecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            QString str5 = _paramTable->item(4, 1)->text();//取出字符串
            double num5 = str5.toDouble();//转成int
            QString str6 = _paramTable->item(5, 1)->text();//取出字符串
            double num6 = str6.toDouble();//转成int
            if (num1 < 1e-6 || num2 < 1e-6 || num3 < 1e-6 || num4 < 1e-6 || num5 < 1e-6 || num6 < 1e-6)
            {
                setBox1SecPara(_para, _nsm);
                return;
            }
            if (num5 + num6 >= num2 || num3 + num4 >= num2)
            {
                setBox1SecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(16);
            for (int i = 0; i < 6; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Box1_SectionVisual* Box1Sec =
                dynamic_cast<Box1_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            Box1Sec->clearData();
            QString nsm = _paramTable->item(6, 1)->text();
            Box1Sec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Box1);
        }
    }

    void mSectionShapePanel::editCrossSecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num1 < 1e-6 || num2 < 1e-6 || num3 < 1e-6 || num4 < 1e-6)
            {
                setCrossSecPara(_para, _nsm);
                return;
            }
            if (num3 <= num4)
            {
                setCrossSecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(17);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Cross_SectionVisual* CrossSec =
                dynamic_cast<Cross_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            CrossSec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            CrossSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Cross);
        }
    }



    void mSectionShapePanel::editZSecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num1 < 1e-6 || num2 < 1e-6 || num3 < 1e-6 || num4 < 1e-6)
            {
                setZSecPara(_para, _nsm);
                return;
            }
            if (num3 >= num4)
            {
                setZSecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(19);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Z_SectionVisual* ZSec =
                dynamic_cast<Z_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            ZSec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            ZSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Z);
        }
    }
    void mSectionShapePanel::editHatSecPara()
    {
        if (_edit)
        {
            QString str1 = _paramTable->item(0, 1)->text();//取出字符串
            double num1 = str1.toDouble();//转成int
            QString str2 = _paramTable->item(1, 1)->text();//取出字符串
            double num2 = str2.toDouble();//转成int
            QString str3 = _paramTable->item(2, 1)->text();//取出字符串
            double num3 = str3.toDouble();//转成int
            QString str4 = _paramTable->item(3, 1)->text();//取出字符串
            double num4 = str4.toDouble();//转成int
            if (num1 < 1e-6 || num2 < 1e-6 || num3 < 1e-6 || num4 < 1e-6)
            {
                setHatSecPara(_para, _nsm);
                return;
            }
            if (2 * num2 >= num3 || 2 * num2 >= num1)
            {
                setHatSecPara(_para, _nsm);
                return;
            }
            _para.clear();
            _para.push_back(18);
            for (int i = 0; i < 4; i++)
            {
                QString str = _paramTable->item(i, 1)->text();//取出字符串
                double num = str.toDouble();//转成int
                _para.push_back(num);
            }
            int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
            Hat_SectionVisual* HatSec =
                dynamic_cast<Hat_SectionVisual*>(_sectionShapeData);
            mPBeam::getInstance()->_nameParaListPair.insert(_name, _para);
            HatSec->clearData();
            QString nsm = _paramTable->item(4, 1)->text();
            HatSec->setNsm(nsm);
            slot_RebuildSec(_para, Id, MxFunctions::Sec_Hat);
        }
    }
    void mSectionShapePanel::editSectionShapePanel(int sectionShapeId)
    {
        //connecSig
        _sectionShapeData = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(sectionShapeId);
        //showGeoWidget(_sectionShapeData->getSectionShapeType());
        //_sectionShapeData->clearData();
        updatePanel();
        // 统一在最后切换视口：自定义截面恢复主渲染视口(便于拾取几何)，
        // 其他截面保持 PropShape 预览图；不能提前，否则会被 updatePanel 覆盖
        updateGeoPartsVisibleForSection(_sectionShapeData
            && _sectionShapeData->getSectionShapeType() == MxFunctions::Sec_None);
    }

    void mSectionShapePanel::updatePanel()
    {
        if (_sectionShapeData == nullptr)
        {
            return;
        }
        SecType type = _sectionShapeData->getSectionShapeType();
        // 只有自定义截面才显示构建梁截面拾取区
        updateBuildSectionFoldVisible();
        int Id = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getCurrentID();
        _para = _sectionShapeData->getPara();
        _foldData->show();
        if (type == MxFunctions::Sec_None)
        {
            _foldParam->hide();   // 自定义截面不显示截面参数区
            _paramTable->setMinimumHeight(0);
        }
        else
        {
            _foldParam->show();
            _paramTable->setMinimumHeight(0);
        }
        QTimer::singleShot(0, _foldParam, [=]() {
            _foldParam->refreshLayout();
            _foldParam->updateGeometry();
            });
        QTimer::singleShot(0, _foldData, [=]() {
            _foldData->refreshLayout();
            _foldData->updateGeometry();
            });
        switch (type)
        {
        case MxFunctions::Sec_None:
        {
            mCustomSectionShape* CrossSec =
                dynamic_cast<mCustomSectionShape*>(_sectionShapeData);
            setCustomPara(CrossSec->getCustomPara());
            //slot_RebuildSec(dynamic_cast<H_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_H);
            //slotShowDataOnPanel(dynamic_cast<H_SectionVisual*>(getSectionShapeManager()->getSectionShapeData(Id))->getData());
        }
            break;
        case MxFunctions::Sec_H:
            setHSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<H_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_H);
            //slotShowDataOnPanel(dynamic_cast<H_SectionVisual*>(getSectionShapeManager()->getSectionShapeData(Id))->getData());
            break;
        case MxFunctions::Sec_L:
            setLSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<L_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_L);
            break;
        case MxFunctions::Sec_I:
            setISecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<I_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_I);
            break;
        case MxFunctions::Sec_I1:
            setI1SecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<I1_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_I1);
            break;
        case MxFunctions::Sec_T:
            setTSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<T_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_T);
            break;
        case MxFunctions::Sec_T1:
            setT1SecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<T1_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_T1);
            break;
        case MxFunctions::Sec_T2:
            setT2SecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<T2_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_T2);
            break;
        case MxFunctions::Sec_Z:
            setZSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Z_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Z);
            break;
        case MxFunctions::Sec_Box:
            setBoxSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Box_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Box);
            break;
        case MxFunctions::Sec_Box1:
            setBox1SecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Box1_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Box1);
            break;
        case MxFunctions::Sec_Rec:
            setRecSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Rectangle_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Rec);
            break;
        case MxFunctions::Sec_Cir:
            setCirSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Circle_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Cir);
            break;
        case MxFunctions::Sec_Ann:
            setAnnSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Annular_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Ann);
            break;
        case MxFunctions::Sec_Channel:
            setChanSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Channel_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Channel);
            break;
        case MxFunctions::Sec_Channel1:
            setChan1SecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Channel1_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Channel1);
            break;
        case MxFunctions::Sec_Channel2:
            setChan2SecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Channel2_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Channel2);
            break;
        case MxFunctions::Sec_Cross:
            setCrossSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Cross_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Cross);
            break;
        case MxFunctions::Sec_Hat:
            setHatSecPara(_sectionShapeData->getPara(), _sectionShapeData->getNsm());
            slot_RebuildSec(dynamic_cast<Hat_SectionVisual*>(_sectionShapeData)->getPara(), Id, MxFunctions::Sec_Hat);
            break;
        default:
            break;
        }



    }
    void mSectionShapePanel::initParamTable()
    {
        _paramTable->setColumnCount(2);
        QStringList heads{ tr("参数"), tr("数值") };
        _paramTable->setHorizontalHeaderLabels(heads);
        _paramTable->horizontalHeader()->setStretchLastSection(true);
        _paramTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        _paramTable->horizontalHeader()->setStyleSheet(
            "QHeaderView::section{background:rgb(255,255,255);color: black;}");
        _paramTable->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
        _paramTable->verticalHeader()->setVisible(false);
    }

    void mSectionShapePanel::initDataTable()
    {
        _dataTable->setColumnCount(2);
        _dataTable->horizontalHeader()->setVisible(false);
        _dataTable->verticalHeader()->setVisible(false);
        _dataTable->horizontalHeader()->setStretchLastSection(true);
        _dataTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        _dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    /* 对外唯一入口：后续把老代码的 setXxxSecPara / slotShowDataOnPanel 搬进来即可 */
    //void mSectionShapePanel::rebuildPanel(const QList<double>& para, const QString& name)
    //{
    //

    //    QList<double> secData = mPBeam::getInstance()->_nameDataListPair.value(name);
    //    _para = para;
    //    _name = name;
    //    if (para.isEmpty()) return;

    //    int type = int(para.at(0));

    //    /* ---------- 参数表 ---------- */
    //    _paramTable->clearContents();
    //    int rows = para.size() - 1;
    //    _paramTable->setRowCount(rows);
    //    //
    //   
    //    for (int i = 0; i < rows; ++i)
    //        addParam(i, QString("DIM%1").arg(i + 1), para.value(i + 1));
    //    switch (type) {
    //    case 1:   // H-Section
    //        setHSecPara(para);
    //        setHSecData(secData);

    //        break;
    //        case 2:   // I-Section
    //            setISecPara(para);
    //            setISecPara(secData);
    //            break;
    //        //case 3:   // L-Section
    //        //    setLSecPara(para);
    //        //    break;
    //        //    // … 18 种全部照抄，只写函数名
    //        //case 18:  // Z-Section
    //        //    setZSecPara(para);
    //        //    break;
    //    default:  // 兜底
    //     
    //        break;
    //        //}

    //        ///* ---------- 数据表 ---------- */
    //        //_dataTable->clearContents();
    //        //QList<double> data = mPBeam::getInstance()->_nameDataListPair.value(name);

    //        //switch (type) {                       // ← 数据表分发器
    //        //case 1:   // H-Section
    //        //    setHSecData(data);
    //        //    break;
    //        //    //case 2:   // I-Section
    //        //    //    setISecData(data);
    //        //    //    break;
    //        //    //case 3:   // L-Section
    //        //    //    setLSecData(data);
    //        //    //    break;
    //        //        // … 18 种全部照抄
    //        //    //case 18:  // Z-Section
    //        //    //    setZSecData(data);
    //        //    //    break;
    //        //default:  // 兜底 11 行通用
    //        //    setGenericData(data);
    //        //    break;
    //        //}
    //    }

    //}

    void mSectionShapePanel::clearTableWidget()
    {
        for (int i = _paramTable->rowCount(); i > 0; i--)
        {
            _paramTable->removeRow(0);
            if (_paramTable->item(0, 1) != nullptr)
            {
                QTableWidgetItem* aitem = _paramTable->item(0, 1);
                delete aitem;
                aitem = nullptr;

            }
            if (_paramTable->item(0, 0) != nullptr)
            {
                QTableWidgetItem* aitem = _paramTable->item(0, 1);
                delete aitem;
                aitem = nullptr;

            }

        }
        // _paramTable->clear();
    }

    void mSectionShapePanel::setCustomPara(const QList<QString>& para)
    {
        _edit = false;
//MIG         clearGeoWidget();
        disconnect(_dataTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        // 自定义截面参数统一填入截面属性表 _dataTable
        _dataTable->clearContents();
        _dataTable->setRowCount(24);

        QStringList labels;
        labels << tr("截面面积A")
            << tr("惯性矩Ixy")
            << tr("惯性矩Ixz")
            << tr("扭转矩J")
            << tr("剪切系数K1")
            << tr("剪切系数K2")
            << tr("惯性积I12")
            << tr("非结构质量NSM")
            << tr("应力恢复点C1")
            << tr("应力恢复点C2")
            << tr("应力恢复点D1")
            << tr("应力恢复点D2")
            << tr("应力恢复点E1")
            << tr("应力恢复点E2")
            << tr("应力恢复点F1")
            << tr("应力恢复点F2")
            << tr("A端非结构质量质心y")
            << tr("A端非结构质量质心z")
            << tr("B端非结构质量质心y")
            << tr("B端非结构质量质心z")
            << tr("A端中性轴y")
            << tr("A端中性轴z")
            << tr("B端中性轴y")
            << tr("B端中性轴z");

        for (int i = 0; i < 24; ++i)
        {
            _dataTable->setItem(i, 0, new QTableWidgetItem(labels[i]));
            _dataTable->item(i, 0)->setFlags(_dataTable->item(i, 0)->flags() & ~Qt::ItemIsEditable);
            _dataTable->setItem(i, 1, new QTableWidgetItem(para[i]));
        }

        _edit = true;
        connect(_dataTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    //十八种类型
    void mSectionShapePanel::setHSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();                      // 你已有的清空函数
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);             // H 有 4 行

        // 第 0 列：行标题（老项目硬编码）
        _paramTable->setItem(0, 0, new QTableWidgetItem("DIM1"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("0.5*DIM2"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("DIM3"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("DIM4"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("NSM"));

        // 第 1 列：数值（顺序必须跟老项目一致）
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);   // 标题列只读
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        // 居中对齐（老项目同款）
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);


        //初始化
        QList<double> _map;




        //manager->updatePara(_name, _map);

        /*  H_SectionVisual* HSec =
            dynamic_cast<H_SectionVisual*>(getSectionShapeManager()->getSectionShapeData(Id));*/
            //HSec->updatePara(_name,_map);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));

    }

    void mSectionShapePanel::setI1SecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(0.5*DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    void mSectionShapePanel::setISecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(7);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(DIM5)"));
        _paramTable->setItem(5, 0, new QTableWidgetItem("(DIM6)"));
        _paramTable->setItem(6, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 6; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(6, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 7; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    void mSectionShapePanel::setLSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("NSM"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ////显示绿色形状
        //int MaxId = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getMaxID();
        //L_SectionVisual* LSec =
        //    dynamic_cast<L_SectionVisual*>(getSectionShapeManager()->getSectionShapeData(MaxId));
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    void mSectionShapePanel::setTSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    void mSectionShapePanel::setRecSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(3);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 2; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(2, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }

    void mSectionShapePanel::setCirSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(2);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(0, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(1), 'f', 6)));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(NSM)"));
        _paramTable->setItem(1, 1, new QTableWidgetItem(nsm));
        _paramTable->item(0, 0)->setFlags(Qt::ItemIsEnabled);
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }

    void mSectionShapePanel::setAnnSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header{ tr("参数"), tr("数值") };
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(3);          // 环形截面只需要外径、内径
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 2; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(2, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 2; ++j)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }

    void mSectionShapePanel::setBoxSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    void mSectionShapePanel::setPolySecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(3);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(R)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(N)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(t)"));
        for (int i = 0; i < 3; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }

    void mSectionShapePanel::setChanSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }

    void mSectionShapePanel::setChan1SecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }

    void mSectionShapePanel::setChan2SecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }

    void mSectionShapePanel::setT1SecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    void mSectionShapePanel::setT2SecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));

    }

    void mSectionShapePanel::setBox1SecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(7);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(DIM5)"));
        _paramTable->setItem(5, 0, new QTableWidgetItem("(DIM6)"));
        _paramTable->setItem(6, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 6; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(6, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 7; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    void mSectionShapePanel::setCrossSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(0.5*DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }

    void mSectionShapePanel::setHatSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }

    void mSectionShapePanel::setZSecPara(const QList<double>& para, QString nsm)
    {
        _edit = false;
        clearTableWidget();
        QStringList header;
        header << tr("参数") << tr("数值");
        _paramTable->setHorizontalHeaderLabels(header);
        _paramTable->setRowCount(5);
        _paramTable->setItem(0, 0, new QTableWidgetItem("(DIM1)"));
        _paramTable->setItem(1, 0, new QTableWidgetItem("(DIM2)"));
        _paramTable->setItem(2, 0, new QTableWidgetItem("(DIM3)"));
        _paramTable->setItem(3, 0, new QTableWidgetItem("(DIM4)"));
        _paramTable->setItem(4, 0, new QTableWidgetItem("(NSM)"));
        for (int i = 0; i < 4; ++i) {
            _paramTable->setItem(i, 1, new QTableWidgetItem(MxFunctions::mNumberToString(para.at(i + 1), 'f', 6)));
            _paramTable->item(i, 0)->setFlags(Qt::ItemIsEnabled);
        }
        _paramTable->setItem(4, 1, new QTableWidgetItem(nsm));
        for (int i = 0; i < 5; i++)
            for (int j = 0; j < 2; j++)
                _paramTable->item(i, j)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        _edit = true;
    }





    void mSectionShapePanel::addParam(int r, const QString& title, double val)
    {
        auto* itemTitle = new QTableWidgetItem(title);
        itemTitle->setFlags(itemTitle->flags() & ~Qt::ItemIsEditable);
        auto* itemVal = new QTableWidgetItem(MxFunctions::mNumberToString(val, 'f', 6));
        _paramTable->setItem(r, 0, itemTitle);
        _paramTable->setItem(r, 1, itemVal);
    }

    void mSectionShapePanel::addData(int r, const QString& txt, double v)
    {
        _dataTable->setItem(r, 0, new QTableWidgetItem(txt));
        if (!qIsNaN(v))
            _dataTable->setItem(r, 1, new QTableWidgetItem(MxFunctions::mNumberToString(v, 'f', 6)));
    }

    void mSectionShapePanel::slotShowDataOnPanel(const QList<double>& data)
    {
        // 断开所有与 slotGetParaFormTabel 相关的连接，避免切换截面时残留 cellChanged 触发编辑
        disconnect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        disconnect(_dataTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        _dataTable->clearContents();
        _dataTable->setRowCount(14);
        _dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        QStringList labels = { "Area","Centroid:","Local","Yc","Zc","Moments Of Inertia:","Centroidal","Iy","Iz","Iyz","Torsional Constant:","Shear factors","Ky","Kz" };
        for (int r = 0; r < 14; ++r) {
            _dataTable->setItem(r, 0, new QTableWidgetItem(labels.at(r)));

        }
        _dataTable->setItem(0, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(0), 10, 6)));
        _dataTable->setItem(3, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(1), 10, 6)));
        _dataTable->setItem(4, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(2), 10, 6)));
        _dataTable->setItem(7, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(3), 10, 6)));
        _dataTable->setItem(8, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(4), 10, 6)));
        _dataTable->setItem(9, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(5), 10, 6)));
        _dataTable->setItem(10, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(6), 10, 6)));
        _dataTable->setItem(12, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(9), 10, 6)));
        _dataTable->setItem(13, 1, new QTableWidgetItem(MxFunctions::mNumberToString(data.at(10), 10, 6)));
        connect(_paramTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
        connect(_dataTable, SIGNAL(cellChanged(int, int)), this, SLOT(slotGetParaFormTabel(int, int)));
    }


    // 自定义截面隐藏 PropShape 预览窗口并恢复主渲染视口显示(便于拾取几何)，
    // 其他截面保持原逻辑(PropShape 显示截面图)
    void mSectionShapePanel::updateGeoPartsVisibleForSection(bool showParts)
    {
        if (!getCVM)
            return;
        // 参照模型树节点点击的视口切换方式(mTreeView::slot_singleClickItem
        // -> switchMenuBarWindow(name))：非"梁截面"走 hidePropShapeViewport，
        // "梁截面"走 showPropShapeViewport，与全局视口切换机制保持一致
        if (showParts)
            MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("几何");
        else
            MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("梁截面");
    }

    // 按截面类型显隐构建梁截面区(仅自定义截面可见)，
    // 参照 mGeoTransformPanel::showFold 的方式用 insertWidget 控制顶层布局位置
    void mSectionShapePanel::updateBuildSectionFoldVisible()
    {        if (!_foldSectionBuild)
            return;
        const bool isCustom = _sectionShapeData
            && _sectionShapeData->getSectionShapeType() == MxFunctions::Sec_None;
        // 先从布局移除再按需插入到最前面，避免残留占位形成黑线
        _mainLayout->removeWidget(_foldSectionBuild);
        _foldSectionBuild->setVisible(isCustom);
        if (isCustom)
            _mainLayout->insertWidget(0, _foldSectionBuild);
        // 非自定义截面退出拾取状态并清空拾取表，避免遗留高亮/拾取数据
        if (!isCustom && geoPickWidget && pickObjTable)
        {
            geoPickWidget->_featPick = None;
            clearTableItems(pickObjTable);
        }
    }

    // 按 sectionShape 当前选项切换拾取模式/表格列/壳厚度行显隐
    void mSectionShapePanel::updateSectionPickMode()
    {
        if (!sectionShape || !geoPickWidget || !pickObjTable || !_foldSectionBuild)
            return;
        geoPickWidget->_featPick = None;
        geoPickWidget->pickObjTypeCombo->blockSignals(true);
        geoPickWidget->pickObjTypeCombo->clear();

        // 注意：不能用 FoldSection::removeRow(QWidget*)，它会 delete 掉行内控件，
        // 导致 sectionShellThicknessLabel 等成员变成野指针下次崩溃。
        // 这里只隐藏控件，并用 0 高度占位行消除空白/左侧残影。
        if (0 == sectionShape->currentIndex())
        {
            // 实体截面：拾取几何面或几何线，列表只有几何ID一列
            geoPickWidget->pickObjTypeCombo->addItems({ "几何面", "几何线" });
            pickObjTable->setColumnCount(1);
            pickObjTable->setHorizontalHeaderLabels({ "几何ID" });
            pickObjTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

            _foldSectionBuild->hideRow(sectionShellThicknessLabel, sectionShellThickness, sectionShellThicknessUnit);
            _foldSectionBuild->showRow(sectionMeshSizeLabel, sectionMeshSize, sectionMeshAuto);
            //sectionMeshSizeUnit->setVisible(true);
        }
        else
        {
            // 薄壁壳截面：仅拾取几何线，列表为几何ID+壳厚度两列
            geoPickWidget->pickObjTypeCombo->addItems({ "几何线" });
            pickObjTable->setColumnCount(2);
            pickObjTable->setHorizontalHeaderLabels({ "几何ID", "壳厚度" });
            pickObjTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

            _foldSectionBuild->hideRow(sectionMeshSizeLabel, sectionMeshSize, sectionMeshAuto);
           // sectionMeshSizeUnit->setVisible(false);
            _foldSectionBuild->showRow(sectionShellThicknessLabel, sectionShellThickness, sectionShellThicknessUnit);
        }
        geoPickWidget->pickObjTypeCombo->blockSignals(false);
        geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        clearTableItems(pickObjTable);
        geoPickWidget->_featPick = FeatureObj;
        _foldSectionBuild->refreshLayout();
    }

    // 参照 mGeoTransformPanel::slot_subWindowActivated：视口切换时断开旧连接，
    // 向当前视口的 SelectionDataContainer 重新连接拾取填充槽
    void mSectionShapePanel::slot_subWindowActivated()
    {
        if (conn_sectionPick)
            QObject::disconnect(conn_sectionPick);
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }
        conn_sectionPick = connect(getCVM->getPreRenderWidget()->getSelectionContainer(),
            &mxr::SelectionDataContainer::ReadyToReadContainer,
            this, &mSectionShapePanel::slot_sectionPickChanged);
    }

    // 拾取变化时填充：FeatureObj 填充 pickObjTable(薄壁壳同时按行写入默认壳厚度)；
    // SectionNormalCustomVec/SectionYAxisCustomVec 拾取几何线自动填充方向；
    // SectionBasePointCustomPnt 拾取几何点自动填充坐标(参照分割面 initSplitFaceFold 逻辑)
    void mSectionShapePanel::slot_sectionPickChanged()
    {
        if (!getCVM || !getCVM->getPreRenderWidget() || !geoPickWidget || !pickObjTable)
            return;

        // 拾取几何线 -> 填充自定义向量(梁截面平面法向 / Y轴方向)
        if (geoPickWidget->_featPick == SectionNormalCustomVec
            || geoPickWidget->_featPick == SectionYAxisCustomVec)
        {
            const std::vector<int> ids = getCVM->getPreRenderWidget()->getSelectionContainer()
                ->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoEdge);
            auto* targetEdit = (geoPickWidget->_featPick == SectionNormalCustomVec)
                ? sectionNormalCustomVec : sectionYAxisCustomVec;
            int& lastLineID = (geoPickWidget->_featPick == SectionNormalCustomVec)
                ? sectionNormalLastLineID : sectionYAxisLastLineID;
            auto geoManager = getCVM->getModelData()->getGeoManager();
            auto lines = geoManager->CalculateEdgeAttr(ids);
            if (lines.size() <= 0)
            {
                setVectorToLineEdit(targetEdit, QVector3D{});
                return;
            }
            setVectorToLineEdit(targetEdit, lines[lines.size() - 1].m_dir);
            disHighlight(lastLineID, "几何线");
            lastLineID = lines[lines.size() - 1].id;
            return;
        }

        // 拾取几何点 -> 填充自定义坐标(截面基准点)
        if (geoPickWidget->_featPick == SectionBasePointCustomPnt)
        {
            const std::vector<int> ids = getCVM->getPreRenderWidget()->getSelectionContainer()
                ->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoPoint);
            auto geoManager = getCVM->getModelData()->getGeoManager();
            auto points = geoManager->CalculateVertexAttr(ids);
            if (points.size() <= 0)
            {
                setVectorToLineEdit(sectionBasePointCustomPnt, QVector3D{});
                return;
            }
            setVectorToLineEdit(sectionBasePointCustomPnt, points[points.size() - 1].m_coor);
            disHighlight(sectionBasePointLastPntID, "几何点");
            sectionBasePointLastPntID = points[points.size() - 1].id;
            return;
        }

        if (geoPickWidget->_featPick != FeatureObj)
            return;
        const QString type = geoPickWidget->strPickType;
        mxr::SelectionInfoType selType;
        if (type == "几何面")
            selType = mxr::SelectionInfoType::GeoFace;
        else if (type == "几何线")
            selType = mxr::SelectionInfoType::GeoEdge;
        else
            return;
        const std::vector<int> ids = getCVM->getPreRenderWidget()->getSelectionContainer()
            ->getGeoSelectData()->getSelectDataByType(selType);

        // 先移除同类型的旧行，再按当前拾取结果重新填充
        for (int i = pickObjTable->rowCount() - 1; i >= 0; --i)
        {
            auto* idItem = pickObjTable->item(i, 0);
            if (idItem && idItem->text().startsWith(type))
                pickObjTable->removeRow(i);
        }
        for (int id : ids)
        {
            const int row = pickObjTable->rowCount();
            pickObjTable->insertRow(row);
            auto* idItem = new QTableWidgetItem(QString(type + ": %1").arg(id));
            idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
            pickObjTable->setItem(row, 0, idItem);
            // 薄壁壳截面每条几何线厚度初始为壳厚度设置值，可单独编辑
            if (2 == pickObjTable->columnCount())
                pickObjTable->setItem(row, 1, new QTableWidgetItem(sectionShellThickness->text()));
        }
        setGeoHighlight(pickObjTable, type);
    }

    // 确认按钮：按截面类型/拾取模式调用实体或薄壁壳属性计算
    void mSectionShapePanel::slot_sectionAction()
    {
        if (!sectionShape)
            return;
        try
        {
            if (1 == sectionShape->currentIndex())
            {
                // 薄壁壳截面：由拾取几何线构建线网络并计算壳截面属性
//MIG                 computeShellSectionAction();
            }
            else
            {
                // 实体截面：拾取几何面直接计算，拾取几何线先由线构面再计算
                const bool isFace = geoPickWidget->pickObjTypeCombo->currentText() == "几何面";
//MIG                 computeSolidSectionAction(isFace);
            }
        }
        catch (const std::exception& e)
        {
            // occsp 异常信息为 UTF-8 中文，直接按 UTF-8 转换，避免 fromLocal8Bit 在
            // 非 UTF-8 系统本地编码(如 GBK)下产生乱码
            mGlobalSignals::getInstance()->outputMessageSig(
               2, QString("截面属性计算失败: %1").arg(QString::fromUtf8(e.what())), true);
        }
    }

    // 薄壁壳截面属性计算
    void mSectionShapePanel::computeShellSectionAction()
    {
    //【OCC摘除·S10 保档】壳截面属性计算依赖退役件 SectionAnalysis(occsp)，随旧栈冻结岛保档
    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
        QStringLiteral("截面属性计算已随旧几何栈退役，等待引擎化移植（见集成记录）"));
//{
//        if (!getCVM || !getCVM->getModelData())
//            return;
//        const std::vector<int> ids = GetTableIdsUseVector();
//        if (ids.empty())
//        {
//            mGlobalSignals::getInstance()->outputMessageSig(2, "请先拾取几何线!", true);
//            return;
//        }
//        auto* geometryData = getCVM->getModelData()->getGeoData();
//        // 读取重合容差
//        bool tolOk = false;
//        const double tolerance = sectionTolerance->text().toDouble(&tolOk);
//        if (!tolOk || tolerance <= 0.0)
//        {
//            mGlobalSignals::getInstance()->outputMessageSig(2, "重合容差必须为正数!", true);
//            return;
//        }

//        // 逐行读取几何线与壳厚度，part 编号按列表行号自动设置
//        std::vector<occsp::ShellEdge> shellEdges;
//        for (int i = 0; i < pickObjTable->rowCount(); ++i)
//        {
//            QString strId = pickObjTable->item(i, 0)->text();
//            strId = strId.mid(strId.lastIndexOf(":") + 1);
//            const int id = strId.toInt();
//            auto* set = geometryData->getPartSetByGeoLineID(id);
//            if (!set)
//            {
//                mGlobalSignals::getInstance()->outputMessageSig(2, QString("未找到几何线 %1 的几何数据!").arg(id), true);
//                return;
//            }
//            occsp::ShellEdge shellEdge;
//            shellEdge.edge = set->getEdgeByTag(id);
//            bool tOk = false;
//            double thickness = sectionShellThickness->text().toDouble();
//            if (auto* tItem = pickObjTable->item(i, 1))
//            {
//                const double rowThickness = tItem->text().toDouble(&tOk);
//                if (tOk)
//                    thickness = rowThickness;
//            }
//            if (thickness <= 0.0)
//            {
//                mGlobalSignals::getInstance()->outputMessageSig(2, QString("几何线 %1 的壳厚度必须为正数!").arg(id), true);
//                return;
//            }
//            shellEdge.thickness = thickness;
//            shellEdge.part = i;
//            shellEdges.push_back(shellEdge);
//        }

//        occsp::ShellOptions options;
//        options.merge_tolerance = tolerance;
//        options.curve_deflection = tolerance / 10.0;
//        options.inertia = occsp::ShellInertia::Strip;
//        options.plastic_integration = occsp::ShellPlasticIntegration::ThreePoint;
//        options.part_generation = occsp::ShellPartGeneration::Explicit;
//        options.shear_factor_override = std::array<double, 2>{1.0, 1.0};

//        // 读取界面 梁截面平面法向/截面基准点/Y轴方向向量 设置
//        const occsp::SectionCoordinateOptions coordinates = makeSectionCoordinateOptions();
//        const auto analysis = occsp::analyze_shell_edges(shellEdges, coordinates, options);
//        const auto& p = analysis.properties;

//        // 全部属性数值按自定义截面 24 行参数顺序填入列表，
//        // 目前 showSectionPropertyRows 只填入面积/扭转矩/惯性积，其余后续再补
//        QList<double> sectionValues;
//        for (int i = 0; i < 24; ++i) sectionValues.append(0.0);
//        sectionValues[0] = p.area;   // 截面面积A
//        sectionValues[1] = p.iy;     // 惯性矩Ixy
//        sectionValues[2] = p.iz;     // 惯性矩Ixz
//        sectionValues[3] = p.j;      // 扭转矩J
//        sectionValues[4] = p.ky;     // 剪切系数K1
//        sectionValues[5] = p.kz;     // 剪切系数K2
//        sectionValues[6] = p.iyz;    // 惯性积I12
//        sectionValues[20] = p.yc;    // A端中性轴y
//        sectionValues[21] = p.zc;    // A端中性轴z
//        sectionValues[22] = p.yc;    // B端中性轴y
//        sectionValues[23] = p.zc;    // B端中性轴z
//        showSectionPropertyRows(sectionValues);
//        mGlobalSignals::getInstance()->outputMessageSig("薄壁壳截面属性计算完成", true);
//    }
}

    // 实体截面属性计算(isFace=true 直接拾取面，否则由线构面)
    void mSectionShapePanel::computeSolidSectionAction(bool isFace)
    {
    //【OCC摘除·S10 保档】实体截面属性计算依赖退役件 SectionAnalysis(occsp)，随旧栈冻结岛保档
    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
        QStringLiteral("截面属性计算已随旧几何栈退役，等待引擎化移植（见集成记录）"));
//{
//        if (!getCVM || !getCVM->getModelData())
//            return;
//        const std::vector<int> ids = GetTableIdsUseVector();
//        if (ids.empty())
//        {
//            mGlobalSignals::getInstance()->outputMessageSig(2, isFace ? "请先拾取几何面!" : "请先拾取几何线!", true);
//            return;
//        }
//        auto* geometryData = getCVM->getModelData()->getGeoData();

//        TopoDS_Face face;
//        if (isFace)
//        {
//            auto* set = geometryData->getPartSetByGeoFaceID(ids.front());
//            if (!set)
//            {
//                mGlobalSignals::getInstance()->outputMessageSig(2, QString("未找到几何面 %1 的几何数据!").arg(ids.front()), true);
//                return;
//            }
//            face = set->getFaceByTag(ids.front());
//        }
//        else
//        {
//            // 先由无序拾取的几何线构建实体面，再计算实体截面属性
//            std::vector<TopoDS_Edge> edges;
//            for (int id : ids)
//            {
//                auto* set = geometryData->getPartSetByGeoLineID(id);
//                if (!set)
//                {
//                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("未找到几何线 %1 的几何数据!").arg(id), true);
//                    return;
//                }
//                edges.push_back(set->getEdgeByTag(id));
//            }
//            bool tolOk = false;
//            const double tolerance = sectionTolerance->text().toDouble(&tolOk);
//            if (!tolOk || tolerance <= 0.0)
//            {
//                mGlobalSignals::getInstance()->outputMessageSig(2, "重合容差必须为正数!", true);
//                return;
//            }
//            face = occsp::make_solid_face(edges, tolerance);
//        }

//        occsp::AnalysisOptions options;
//        if (!sectionMeshAuto->isChecked())
//        {
//            bool sizeOk = false;
//            const double meshSize = sectionMeshSize->text().toDouble(&sizeOk);
//            if (sizeOk && meshSize > 0.0)
//            {
//                options.mesh_size = meshSize;
//                options.boundary_deflection = meshSize / 16.0;
//            }
//        }
//        options.solid_element_order = 2;
//        options.poisson_ratio = 0.0;

//        // 读取界面 梁截面平面法向/截面基准点/Y轴方向向量 设置
//        const occsp::SectionCoordinateOptions coordinates = makeSectionCoordinateOptions();
//        const auto analysis = occsp::analyze_solid_face(face, coordinates, options);
//        const auto h = analysis.properties;

//        // 全部属性数值按自定义截面 24 行参数顺序填入列表，
//        // 目前 showSectionPropertyRows 只填入面积/扭转矩/惯性积，其余后续再补
//        QList<double> sectionValues;
//        for (int i = 0; i < 24; ++i) sectionValues.append(0.0);
//        sectionValues[0] = h.area;                 // 截面面积A
//        sectionValues[1] = h.iy;                   // 惯性矩Ixy
//        sectionValues[2] = h.iz;                   // 惯性矩Ixz
//        sectionValues[3] = h.torsional_constant;   // 扭转矩J
//        sectionValues[4] = h.ky;                   // 剪切系数K1
//        sectionValues[5] = h.kz;                   // 剪切系数K2
//        sectionValues[6] = h.iyz;                  // 惯性积I12
//        sectionValues[20] = h.yc;                  // A端中性轴y
//        sectionValues[21] = h.zc;                  // A端中性轴z
//        sectionValues[22] = h.yc;                  // B端中性轴y
//        sectionValues[23] = h.zc;                  // B端中性轴z
//        showSectionPropertyRows(sectionValues);
//        mGlobalSignals::getInstance()->outputMessageSig("实体截面属性计算完成", true);
//    }
}

    // 把界面上 梁截面平面法向/截面基准点/Y轴方向向量 三个下拉框的设置
    // 转换为 occsp::SectionCoordinateOptions(参照分割面 getDir 的方向提取逻辑)
    static void makeSectionCoordinateOptions_stub_note() //【occsp 保档壳】类外独立（原类方法声明已随 occsp 退役）
    {
    //【OCC摘除·S10 保档】截面坐标系选项构造依赖 occsp，随旧栈保档
    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
        QStringLiteral("截面属性计算已随旧几何栈退役，等待引擎化移植（见集成记录）"));
//{
//        occsp::SectionCoordinateOptions c;

//        // 1. 梁截面平面法向：索引6=与拾取几何垂直 -> FitToEntities；
//        //    其余(X/Y/Z轴/垂直屏幕/平行屏幕/自定义)按分割面 getDir 提取方向向量
//        if (sectionNormalCombo->currentIndex() == 6)
//        {
//            c.plane_mode = occsp::SectionPlaneMode::FitToEntities;
//            // cross_section_normal 保持默认 {0,0,1}，由 FitToEntities 按几何拟合
//        }
//        else
//        {
//            c.plane_mode = occsp::SectionPlaneMode::ProjectToPlane;
//            double* mdir[3];
//            getDir(mdir, sectionNormalCombo->currentIndex(), sectionNormalCustomVec);
//            c.cross_section_normal = gp_Vec(*mdir[0], *mdir[1], *mdir[2]);
//            for (int i = 0; i < 3; ++i) delete[] mdir[i];
//        }

//        // 2. 截面基准点：0=剪切中心 1=形心 2=自定义(拾取/手输坐标)
//        switch (sectionBasePointCombo->currentIndex())
//        {
//        case 0:
//            c.base_node = occsp::SectionBaseNode::ShearCenter;
//            break;
//        case 1:
//            c.base_node = occsp::SectionBaseNode::Centroid;
//            break;
//        case 2:
//        {
//            c.base_node = occsp::SectionBaseNode::CustomPoint;
//            const QVector3D pnt = extractVectorFromLineEdit(sectionBasePointCustomPnt->text());
//            c.base_point = gp_Pnt(pnt.x(), pnt.y(), pnt.z());
//            break;
//        }
//        }

//        // 3. Y轴方向向量：索引6=自动 -> AutoOrient；
//        //    其余按分割面 getDir 提取方向向量
//        if (sectionYAxisCombo->currentIndex() == 6)
//        {
//            c.orientation = occsp::SectionOrientation::AutoOrient;
//            // orientation_vector 保持默认 {0,1,0}
//        }
//        else
//        {
//            c.orientation = occsp::SectionOrientation::CustomVector;
//            double* mdir[3];
//            getDir(mdir, sectionYAxisCombo->currentIndex(), sectionYAxisCustomVec);
//            c.orientation_vector = gp_Vec(*mdir[0], *mdir[1], *mdir[2]);
//            for (int i = 0; i < 3; ++i) delete[] mdir[i];
//        }
//        return c;
//    }
}

    // 自定义截面：属性计算完成后把属性数值填入 _dataTable 自定义表。
    // 目前只填入面积/扭转矩/惯性积，其余行后续再补。
    void mSectionShapePanel::showSectionPropertyRows(const QList<double>& sectionValues)
    {
        if (!_dataTable || _dataTable->rowCount() <= 0 || sectionValues.isEmpty())
            return;
        // 填入时不触发自定义参数回写，避免循环触发 cellChanged
        const bool wasEditing = _edit;
        _edit = false;
        _dataTable->blockSignals(true);
        const QVector<int> rowsToFill{ 0, 1, 2, 3, 4, 5, 6, 20, 21, 22, 23 };  
        for (int row : rowsToFill)
        {
            if (row >= sectionValues.size() || row >= _dataTable->rowCount())
                continue;
            const QString text = MxFunctions::mNumberToString(sectionValues.at(row), 'g', 10);
            auto* valueItem = _dataTable->item(row, 1);
            if (valueItem)
                valueItem->setText(text);
            else
                _dataTable->setItem(row, 1, new QTableWidgetItem(text));
        }
        _dataTable->blockSignals(false);
        _edit = wasEditing;

        // 把填入的数值同步保存到自定义截面数据对象，避免切换模型树节点后
        // setCustomPara 用旧数据重建表格时丢失刚计算出的属性值
        auto* sec = dynamic_cast<mCustomSectionShape*>(_sectionShapeData);
        if (sec)
        {
            QList<QString> customParaList;
            for (int i = 0; i < 24; ++i)
            {
                QString str;
                if (i < _dataTable->rowCount())
                {
                    if (auto* item = _dataTable->item(i, 1))
                        str = item->text();
                }
                customParaList.append(str);
            }
            sec->setCustomPara(customParaList);
        }
    }





}