#include "AllRenderSymbolManager.h"
#include <QVector3D>


#include <utility>
#include "mWorkingConditions.h"

#include "mBoundaryFixed.h"
#include "mBoundaryDisplacement.h"
#include "mBoundaryTemp.h"
#include "mBoundaryVirtual.h"
#include "mBoundarySuperelement.h"

#include "mConnectorBind.h"

#include "mConnectorRigid.h"
#include "mConnectorFlexible.h"
#include "mConnectorMPC.h"
#include "mConnectorDistributedCoupling.h"
#include "mConnectingDeputy.h"
#include "mConnectorFastener.h"
#include "mConnectorBolt.h"
#include "mConnectorBearingUnits.h"

#include "mElementFunction.h"
#include "mElementSet.h"
#include "mNodeFace.h"
#include "mLoadEdge.h"

#include "PickCommonFunction.h"
#include "mContactManager.h"
#include "mGetSetFunction.h"
#include "mPeriodicBoundaryManager.h"
#include "mPeriodicBoundary.h"

//#include <TopoDS_Edge.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//include <TopExp.hxx> //【OCC摘除·S10】毒头（拉 OCC 链/冻结岛/出列件）死引用清剿
//#include <TopoDS_Vertex.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <BRep_Tool.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <gp_Pnt.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役

//#include <BRepGProp.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <GProp_GProps.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <TopoDS_Face.hxx> //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）


using namespace MechMainWindow;
using namespace MechCaseData;
using namespace MechPreData;
using namespace MechContactData;

AllRenderSymbolManager* AllRenderSymbolManager::m_instance = nullptr;

AllRenderSymbolManager::AllRenderSymbolManager(QObject* parent)
    : QObject(parent)
{

}

AllRenderSymbolManager* AllRenderSymbolManager::getInstance()
{
    if (!m_instance)
        m_instance = new AllRenderSymbolManager;
    return m_instance;
}

//载荷渲染
void AllRenderSymbolManager::showLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    if (loadData == nullptr)
        return;
    QString symbol = QString("load_%1").arg(loadData->getLoadBoundaryId());
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    vector<QString> showSymbol;
    showSymbol.push_back(symbol);

    if (!symbolController->isExistSymbol(symbol))
    {
        rendLoad(loadData);
    }
}

void AllRenderSymbolManager::rendLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    if (loadData == nullptr) return;

    auto type = loadData->getLoadBoundaryType();

    switch (type)
    {
    case MxFunctions::LOAD_IM_FORCE:
    {
        std::shared_ptr<MechCaseData::mLoadForce> load = std::dynamic_pointer_cast<MechCaseData::mLoadForce>(loadData);
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;
        // 分析场方式无作用对象，不渲染符号
        if (!load->isAnalysisFieldMode_ && load->way_ == 2)
        {
            pos_dir = getIndependentLoadSetForce(load);
        }
        else
        {
            // 统一赋值（way_ == 0 各向分量 或 way_ == 1 方向与大小）：从集合获取数据
            // 分析场方式无作用对象，跳过方向合成与集合展开
            if (load->isAnalysisFieldMode_)
            {
                assignLoad(loadData, pos_dir, "Load_Force", QVector3D(255.0f, 165.0f, 0.0f));
                break;
            }
            QVector3D dir;
            QVariant dirdata = load->dir_;
            if (dirdata.isNull() || !dirdata.isValid())
            {
                if (load->force_x.isNull() && !load->force_x.isValid() &&
                    load->force_y.isNull() && !load->force_y.isValid() &&
                    load->force_z.isNull() && !load->force_z.isValid())
                {
                    return;
                }
                QVector3D dirtmp;
                dirtmp.setX(load->force_x.toDouble());
                dirtmp.setY(load->force_y.toDouble());
                dirtmp.setZ(load->force_z.toDouble());
                load->dir_ = dirtmp;
                dir = dirtmp;
            }
            else
            {
                dir = dirdata.value<QVector3D>();
            }
            // 方向与大小（way_ == 1）：渲染方向由方向控件与载荷大小符号共同决定，
            // dir_ 存储的是无符号单位方向，大小为负数时方向取反；
            // 各向分量（way_ == 0）方向由带符号分量合成，负值已自然翻转，不再处理。
            if (load->way_ == 1 && load->force_.isValid() && !load->force_.isNull()
                && load->force_.toDouble() < 0)
            {
                dir = -dir;
            }
            auto objs = getLoadSet(loadData);
            for (auto obj : objs)
            {
                pos_dir.push_back(std::make_pair(obj, dir));
            }
        }
        assignLoad(loadData, pos_dir, "Load_Force", QVector3D(255.0f, 165.0f, 0.0f));
    }
    break;
    case MxFunctions::LOAD_IM_TORQUE:
    {
        std::shared_ptr<MechCaseData::mLoadForce> load = std::dynamic_pointer_cast<MechCaseData::mLoadForce>(loadData);
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;
        if (load->way_ == 1)
        {
            pos_dir = getIndependentLoadSetForce(load);
        }
        else
        {
            // 统一赋值（way_ == 0）：从集合获取数据
            QVector3D dir;
            dir.setX(load->force_x.toDouble());
            dir.setY(load->force_y.toDouble());
            dir.setZ(load->force_z.toDouble());
            if (dir.isNull())
            {
                return;
            }
            auto objs = getLoadSet(loadData);
            for (auto obj : objs)
            {
                pos_dir.push_back(std::make_pair(obj, dir));
            }
        }
        assignLoad(loadData, pos_dir, "Load_Torque", QVector3D(255.0f, 165.0f, 0.0f));
    }
    break;
    case MxFunctions::LOAD_IM_PRESSURE:
    {
        std::shared_ptr<MechCaseData::mLoadPressure> load = std::dynamic_pointer_cast<MechCaseData::mLoadPressure>(loadData);
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;
        // 分析场方式无作用对象，不渲染符号
        if (load->pressureType_ == 0 && load->isAnalysisFieldMode_)
        {
            assignLoad(loadData, pos_dir, "Load_Pressure", QVector3D(255.0f, 165.0f, 0.0f));
            break;
        }
        if (load->way_ == 1)
        {
            // 独立赋值：逐面压强为负时方向取反（getIndependentLoadSetPressure 内处理），无值默认正方向
            pos_dir = getIndependentLoadSetPressure(load);
        }
        else if (load->pressureType_ == 1)
        {
            // 静水压力：方向为深度方向（与面板实时渲染一致），参考压强值为负时取反；无值默认正方向
            QVector3D dir = load->depthDirection_;
            if (dir.isNull())
            {
                dir = QVector3D(0, 0, 1);
            }
            if (load->refPressure_.isValid() && !load->refPressure_.isNull()
                && load->refPressure_.toDouble() < 0)
            {
                dir = -dir;
            }
            auto objs = getLoadSetPressure(loadData);
            for (auto obj : objs)
            {
                pos_dir.push_back(std::make_pair(obj, dir));
            }
        }
        else
        {
            // 统一赋值：从集合获取数据
            QVector<QVector3D> dirs;
            auto objs = getLoadSetPressure(loadData, dirs);
            // 常规压力：渲染方向与压强值符号关联，负值时取反；无值默认正方向（保持原有 -法向 不变）
            bool negativePa = (load->pa_.isValid() && !load->pa_.isNull()
                && load->pa_.toDouble() < 0);
            for (int i = 0; i < objs.size(); ++i)
            {
                pos_dir.push_back(std::make_pair(objs[i], negativePa ? dirs[i] : -dirs[i]));
            }
        }
        assignLoad(loadData, pos_dir, "Load_Pressure", QVector3D(255.0f, 165.0f, 0.0f));
    }
    break;
    case MxFunctions::LOAD_IM_HEATFLUX:
    {
        std::shared_ptr<MechCaseData::mLoadHeatFlux> load = std::dynamic_pointer_cast<MechCaseData::mLoadHeatFlux>(loadData);
        QVector<QVector3D> objs;
        if (load->defineType_ == 1)
        {
            objs = getIndependentLoadSetHeatflux(load);
        }
        else
        {
            // 统一赋值：从集合获取数据
            objs = getLoadSetHeatflux(loadData);
        }
        assignThermalLoad(loadData, objs, "Load_HeatFlux", QVector3D(255.0f, 255.0f, 0.0f), load->way_);
    }
    break;
    case MxFunctions::LOAD_IM_RADIATION:
    {
        std::shared_ptr<MechCaseData::mLoadRadiation> load = std::dynamic_pointer_cast<MechCaseData::mLoadRadiation>(loadData);
        QVector<QVector3D> objs;
        if (load->defineType_ == 1)
        {
            objs = getIndependentLoadSetRadiation(load);
        }
        else
        {
            // 统一赋值：从集合获取数据
            objs = getLoadSetRadiation(loadData);
        }
        assignThermalLoad(loadData, objs, "Load_Radiation", QVector3D(255.0f, 97.0f, 0.0f));
    }
    break;
    case MxFunctions::LOAD_IM_CONVECTION:
    {
        std::shared_ptr<MechCaseData::mLoadConvection> load = std::dynamic_pointer_cast<MechCaseData::mLoadConvection>(loadData);
        QVector<QVector3D> objs;
        if (load->defineType_ == 1)
        {
            objs = getIndependentLoadSetConvection(load);
        }
        else
        {
            // 统一赋值：从集合获取数据
            objs = getLoadSetConvection(loadData);
        }
        assignThermalLoad(loadData, objs, "Load_Convection", QVector3D(255.0f, 255.0f, 0.0f));
    }
    break;
    case MxFunctions::LOAD_IM_ACOCONCFORCE:
    {
        std::shared_ptr<MechCaseData::mLoadAcousticConcForce> load = std::dynamic_pointer_cast<MechCaseData::mLoadAcousticConcForce>(loadData);

        QVector<QVector3D> dirs;
        //作用对象
        auto objs = getLoadSetAcoconcforce(loadData);

        assignThermalLoad(loadData, objs, "Load_AcousticConcForce", QVector3D(255.0f, 255.0f, 0.0f));
    }
    break;
    case MxFunctions::LOAD_IM_SURFACE:
    {
        std::shared_ptr<MechCaseData::mLoadSurface> load = std::dynamic_pointer_cast<MechCaseData::mLoadSurface>(loadData);
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;

        if (load->way_ == 1)
        {
            // 独立赋值：读取 surfaceForceMap_ 表格v1v2v3
            pos_dir = getIndependentLoadSetSurface(load);
        }
        else
        {
            // 统一赋值：集合对象全部共用参考方向
            QVector<QVector3D> dirsTemp;
            auto objs = getLoadSetPressure(loadData, dirsTemp);

            QVector3D refDir;
            const auto& state = load->dirPickState_;
            if (state.mode == 1) refDir = QVector3D(1, 0, 0);
            else if (state.mode == 2) refDir = QVector3D(0, 1, 0);
            else if (state.mode == 3) refDir = QVector3D(0, 0, 1);
            else if (state.mode == 6) refDir = state.directInputDir;
            else if ((state.mode == 4 || state.mode == 5) && state.startPoint.isValid() && state.endPoint.isValid())
            {
                QVector3D start = state.startPoint.value<QVector3D>();
                QVector3D end = state.endPoint.value<QVector3D>();
                refDir = end - start;
            }

            if (refDir.lengthSquared() < 1e-9)
            {
                // 参考方向未输入，不渲染任何箭头
                break;
            }
        if (state.reversed) refDir = -refDir;
        // 统一赋值：渲染方向与载荷值符号关联，载荷值为负时取反；无值默认正方向
        if (load->loadValue_.isValid() && !load->loadValue_.isNull()
            && load->loadValue_.toDouble() < 0)
        {
            refDir = -refDir;
        }
        refDir.normalize();

            for (const auto& pos : objs)
            {
                pos_dir.push_back(std::make_pair(pos, refDir));
            }
        }
        // 表面载荷渲染颜色，按需调整RGB
        assignLoad(loadData, pos_dir, "Load_Surface", QVector3D(60.0f, 180.0f, 220.0f));
    }
    break;
    case MxFunctions::LOAD_IM_EDGE:
    {
        //边载荷：几何线/几何面/单元线，方向由方向模式决定（面内法向/面外法向/边切向/通用）
        auto pos_dir = getLoadSetEdge(loadData);
        // 渲染方向与载荷值符号关联：载荷值为负时方向取反；无值默认正方向
        auto edgeLoad = std::dynamic_pointer_cast<MechCaseData::mLoadEdge>(loadData);
        if (edgeLoad)
        {
            bool sizeOk = false;
            double sizeVal = edgeLoad->size_.trimmed().toDouble(&sizeOk);
            if (sizeOk && sizeVal < 0)
            {
                for (auto& pd : pos_dir)
                {
                    pd.second = -pd.second;
                }
            }
        }
        assignLoad(loadData, pos_dir, "Load_Edge", QVector3D(33.0f, 150.0f, 243.0f));
    }
    break;
    default:
        break;
    }
}

//边界渲染
void AllRenderSymbolManager::showBoundary(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
{
    if (boundaryData == nullptr)
        return;
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();

    int boundaryId = boundaryData->getLoadBoundaryId();
    // 与 showLoad 一致改为懒渲染：符号已存在（数据由面板编辑时实时刷新维护）
    // 则直接复用，避免每次点击树父节点都删除重建、重算全部作用对象坐标；
    // 符号缺失（首次渲染/导入）时仍先清除本边界所有可能 symbol，
    // 避免独立赋值分量 symbol 残留
    QString mainSymbol = QString("boundary_%1").arg(boundaryId);
    if (!symbolController->isExistSymbol(mainSymbol))
    {
        symbolController->deleteSymbol(getBoundarySymbolNames(boundaryId));
        rendBoundary(boundaryData);
    }
}

void AllRenderSymbolManager::rendBoundary(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
{
    if (boundaryData == nullptr) return;

    auto type = boundaryData->getLoadBoundaryType();

    switch (type) {
    case MxFunctions::BOUNDARY_IM_FIXED:
    {
        std::shared_ptr<MechCaseData::mBoundaryFixed> boundary = std::dynamic_pointer_cast<MechCaseData::mBoundaryFixed>(boundaryData);
        if (!boundary) break;
        auto objs = getBoundarySet(boundaryData);
        QMap<uint32_t, float>value;
        int fixedtype = boundary->_fixedType;
        QVector<int>idx;
        switch (fixedtype)
        {
        case 0: idx = { 1,2,3,4,5,6 }; break;        // 完全固定
        case 1: idx = { 1,2,3 }; break;             // 铰链
        case 2: idx = { 1,5,6 }; break;             // 关于X轴对称
        case 3: idx = { 2,4,6 }; break;             // 关于Y轴对称
        case 4: idx = { 3,4,5 }; break;             // 关于Z轴对称
        case 5: idx = { 2,3,4 }; break;             // 关于X轴反对称
        case 6: idx = { 1,3,5 }; break;             // 关于Y轴反对称
        case 7: idx = { 1,2,6 }; break;             // 关于Z轴反对称
        default: break;
        }
        for (int i = 0; i < idx.size(); ++i)
        {
            value[idx[i]] = 0;
        }
        assignBoundary(boundaryData, objs, value, QVector3D(255.0f, 165.0f, 0.0f));
    }
    break;
    case MxFunctions::BOUNDARY_IM_TEMP:
    {
        std::shared_ptr<MechCaseData::mBoundaryTemp> boundary = std::dynamic_pointer_cast<MechCaseData::mBoundaryTemp>(boundaryData);
        // 分析场方式无作用对象，不渲染符号
        if (boundary && boundary->isAnalysisFieldMode_)
        {
            assignBoundary(boundaryData, {}, QVector3D(255.0f, 165.0f, 0.0f));
            break;
        }
        auto objs = getBoundarySet(boundaryData);
        assignBoundary(boundaryData, objs, QVector3D(255.0f, 165.0f, 0.0f));
    }
    break;
    case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
    case MxFunctions::BOUNDARY_IM_SPEED:
    case MxFunctions::BOUNDARY_IM_ACC:
    {
        std::shared_ptr<MechCaseData::mBoundaryDisplacement> boundary = std::dynamic_pointer_cast<MechCaseData::mBoundaryDisplacement>(boundaryData);
        if (boundary->_definitionMode == 1)
        {
            // 独立赋值模式：按节点-分量精确渲染
            assignBoundaryIndependentDisplacement(boundaryData, boundary->_independentTable, QVector3D(255.0f, 165.0f, 0.0f));
        }
        else {
            auto objs = getBoundarySet(boundaryData);
            // U 与 UR 分量独立传入（key 1..3 对应 U1..U3，4..6 对应 UR1..UR3），
            // 由 assignBoundaryDisplacement 按轴合并：方向一致只画一个，方向相反都画
            QMap<uint32_t, float>value;
            if (!boundary->_U1.isNull() && boundary->_U1.isValid())
            {
                value[1] = boundary->_U1.toDouble();
            }
            if (!boundary->_UR1.isNull() && boundary->_UR1.isValid())
            {
                value[4] = boundary->_UR1.toDouble();
            }
            if (!boundary->_U2.isNull() && boundary->_U2.isValid())
            {
                value[2] = boundary->_U2.toDouble();
            }
            if (!boundary->_UR2.isNull() && boundary->_UR2.isValid())
            {
                value[5] = boundary->_UR2.toDouble();
            }
            if (!boundary->_U3.isNull() && boundary->_U3.isValid())
            {
                value[3] = boundary->_U3.toDouble();
            }
            if (!boundary->_UR3.isNull() && boundary->_UR3.isValid())
            {
                value[6] = boundary->_UR3.toDouble();
            }
            // 与集中力相同的箭头符号渲染，方向按轴（U1/UR1->X、U2/UR2->Y、U3/UR3->Z）并随数值正负翻转
            assignBoundaryDisplacement(boundaryData, objs, value, QVector3D(255.0f, 165.0f, 0.0f));
        }
    }
    break;
    case MxFunctions::BOUNDARY_IM_SUPERELEMENT:
    {
        std::shared_ptr<MechCaseData::mBoundarySuperelement> boundary = std::dynamic_pointer_cast<MechCaseData::mBoundarySuperelement>(boundaryData);
        auto objs = getBoundarySet(boundaryData);
        QMap<uint32_t, float>value;

        if (boundary->_DOF[0])
        {
            value[1] = 1;
        }
        else if (boundary->_DOF[3])
        {
            value[1] = 4;
        }


        if (boundary->_DOF[1])
        {
            value[2] = 2;
        }
        else if (boundary->_DOF[4])
        {
            value[2] = 5;
        }


        if (boundary->_DOF[2])
        {
            value[3] = 3;
        }
        else if (boundary->_DOF[5])
        {
            value[3] = 6;
        }
        assignBoundary(boundaryData, objs, value, QVector3D(0.0f, 255.0f, 255.0f));
    }
    break;
    default:
        break;
    }
}

//连接渲染
void AllRenderSymbolManager::showConnector(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData)
{
    if (_connectorData == nullptr)
        return;
    QString symbol = QString("connector_%1").arg(_connectorData->getConnectorId());
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    vector<QString> showSymbol;
    showSymbol.push_back(symbol);

    if (!symbolController->isExistSymbol(symbol))
    {
        rendConnector(_connectorData, true);
    }
}

void AllRenderSymbolManager::rendConnector(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData, bool showhide)
{
    if (_connectorData == nullptr) return;

    auto type = _connectorData->getConnectorType();

    // 清除旧符号渲染，避免清空主从节点后视觉残留
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("connector_%1").arg(_connectorData->getConnectorId());
    symbolController->deleteSymbol({ newsymbol });

    switch (type)
    {
    case MxFunctions::Connector_Bind:
    {
        mainNodesBind_.clear();
        subNodesBind_.clear();

        std::shared_ptr<MechConnectorData::mConnectorBind> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBind>(_connectorData);
        if (connectorPtr == nullptr) return;
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        auto widget_ptr = getCVM->getPreRenderWidget();
        auto highlight_controller = widget_ptr->getHighlightController();

        //主面
        int masterSetId = mapRelationManager.getMapRelation<int>({ PhysicalFieldType::Physical_Connector,_connectorData->getConnectorId() }, Physical_Set, MapRelation_Master);
        if (masterSetId != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(masterSetId);
            if (set->getSetType() != Mix)
            {
                std::vector<int> ids;
                std::vector<RenderPickResultData> data;
                PickDataType pickDataType = PickDataType::None;
                if (set->getSetType() == GeoFace)
                {
                    auto geoset = dynamic_cast<mGeoSet*>(set);
                    ids = geoset->getGeoSetData();
                    pickDataType = PickDataType::GeoFace;
                }
                else if (set->getSetType() == ElementSurface)
                {
                    auto elesurfaceset = dynamic_cast<mElementSurface*>(set);
                    data = elesurfaceset->getElementSurfaceData();
                    pickDataType = PickDataType::ElementFace;
                }

                //创建主面内部集合
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                mMixSet* set1 = new mMixSet(setID, SetType::Mix, "contain", true);
                set1->replaceMixSetData(pickDataType, data, ids);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set1);
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact, _connectorData->getConnectorId() }, { Physical_Set, setID }, MapRelation_Master);
                set = set1;
            }
            mMixSet* eleset = dynamic_cast<mMixSet*>(set);
            auto goefaces = eleset->getMixSetData()[PickDataType::GeoFace];
            auto elemfaces = eleset->getMixSetData()[PickDataType::ElementFace ];

            for (auto face : goefaces)
            {
                mainNodesBind_.push_back(std::make_pair(PickDataType::GeoFace, face));
            }
            for (auto face : elemfaces)
            {
                mainNodesBind_.push_back(std::make_pair(PickDataType::ElementFace, face));
            }

            // 高亮主面几何面 - 红色
            if (!goefaces.empty())
            {
                auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"));
                geoHighLightRender->setHighLightColor(255, 0, 0);
                auto res = vector<int>{};
                mGeoSelectData* geoData = new mGeoSelectData();
                for (const auto& resultData : goefaces)
                {
                    auto id = resultData.result_id;
                    auto partId = meshData->getPartIdByGeoFace(id);
                    auto partData = meshData->getPartData(partId);
                    if (partData && !partData->getPartMask())
                    {
                        res.push_back(id);
                    }
                }
                geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
                geoHighLightRender->updateHighLightRender(geoData);
            }

            // 高亮主面单元面 - 红色
            if (!elemfaces.empty())
            {
                std::vector<MeshSelectInfoData> res;
                for (const auto& pickResultData : elemfaces)
                {
                    res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));
                }
                auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
                meshHighLightRender->setHighLightColor(255, 0, 0);
                MeshSelectMemoryData* meshDataHL = new MeshSelectMemoryData();
                meshDataHL->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
                meshHighLightRender->updateHighLightRender(meshDataHL);
            }
        }
        else
        {
            mainNodesBind_.clear();
        }

        //从面
        int slaveSetId = mapRelationManager.getMapRelation<int>({ PhysicalFieldType::Physical_Connector,_connectorData->getConnectorId() }, Physical_Set, MapRelation_Slave);
        if (slaveSetId != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(slaveSetId);
            if (set->getSetType() != Mix)
            {
                std::vector<int> ids;
                std::vector<RenderPickResultData> data;
                PickDataType pickDataType = PickDataType::None;
                if (set->getSetType() == GeoFace)
                {
                    auto geoset = dynamic_cast<mGeoSet*>(set);
                    ids = geoset->getGeoSetData();
                    pickDataType = PickDataType::GeoFace;
                }
                else if (set->getSetType() == ElementSurface)
                {
                    auto elesurfaceset = dynamic_cast<mElementSurface*>(set);
                    data = elesurfaceset->getElementSurfaceData();
                    pickDataType = PickDataType::ElementFace;
                }

                //创建从面内部集合
                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                mMixSet* set1 = new mMixSet(setID, SetType::Mix, "contain", true);
                set1->replaceMixSetData(pickDataType, data, ids);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(set1);
                mapRelationManager.appendMapRelation({ PhysicalFieldType::Physical_Contact,  _connectorData->getConnectorId() }, { Physical_Set, setID }, MapRelation_Slave);
                set = set1;
            }
            mMixSet* eleset = dynamic_cast<mMixSet*>(set);
            auto goefaces = eleset->getMixSetData()[PickDataType::GeoFace];
            auto elemfaces = eleset->getMixSetData()[PickDataType::ElementFace];
            for (auto face : goefaces)
            {
                subNodesBind_.push_back(std::make_pair(PickDataType::GeoFace, face));
            }
            for (auto face : elemfaces)
            {
                subNodesBind_.push_back(std::make_pair(PickDataType::ElementFace, face));
            }

            // 高亮从面几何面 - 蓝色
            if (!goefaces.empty())
            {
                auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), false);
                geoHighLightRender->setHighLightColor(0, 0, 255);
                auto res = vector<int>{};
                mGeoSelectData* geoData = new mGeoSelectData();
                for (const auto& resultData : goefaces)
                {
                    auto id = resultData.result_id;
                    auto partId = meshData->getPartIdByGeoFace(id);
                    auto partData = meshData->getPartData(partId);
                    if (partData && !partData->getPartMask())
                    {
                        res.push_back(id);
                    }
                }
                geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
                geoHighLightRender->updateHighLightRender(geoData);
            }

            // 高亮从面单元面 - 蓝色
            if (!elemfaces.empty())
            {
                std::vector<MeshSelectInfoData> res;
                for (const auto& pickResultData : elemfaces)
                {
                    res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));
                }
                auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"), false);
                meshHighLightRender->setHighLightColor(0, 0, 255);
                MeshSelectMemoryData* meshDataHL = new MeshSelectMemoryData();
                meshDataHL->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
                meshHighLightRender->updateHighLightRender(meshDataHL);
            }
        }
        else
        {
            subNodesBind_.clear();
        }
    }
    break;
    case MxFunctions::Connector_Rigid:
    {
        mainNodesRigid_.clear();
        subNodesRigid_.clear();
        mainNodesRigid_id.clear();
        subNodesRigid_id.clear();
        renderposRigid.clear();

        std::shared_ptr<MechConnectorData::mConnectorRigid> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorRigid>(_connectorData);
        if (connectorPtr == nullptr) return;
        if (connectorPtr->mainnodes_.first != MxFunctions::PickDataType::None)
        {
            mainNodesRigid_.push_back(connectorPtr->mainnodes_);
            mainNodesRigid_id.push_back(std::make_pair(connectorPtr->mainnodes_.first, connectorPtr->mainnodes_.second.result_id));
        }

        if (connectorPtr->subnodes_.size() != 0)
        {
            for (auto it = connectorPtr->subnodes_.begin(); it != connectorPtr->subnodes_.end(); it++)
            {
                for (int i = 0; i < it->second.size(); ++i)
                {
                    subNodesRigid_.push_back(std::make_pair(it->first, it->second[i]));
                }
            }

            for (int i = 0; i < subNodesRigid_.size(); ++i)
            {
                subNodesRigid_id.push_back(std::make_pair(subNodesRigid_[i].first, subNodesRigid_[i].second.result_id));
            }
        }

        if (connectorPtr->way_ == 1)
        {
            //将渲染位置计算出来
            renderposRigid.clear();
            renderposRigid = transPickedToNodes(subNodesRigid_id);
        }


        if (mainNodesRigid_.empty() || subNodesRigid_.empty())
        {
            assignConnector(_connectorData, {}, {}, ConnectorType::Connector_Rigid);
            return;
        }

        int idx = connectorPtr->way_;
        if (idx == 0)
        {
            QVector3D masterpos;
            if (mainNodesRigid_[0].first == PickDataType::Node)
            {
                masterpos = getCVM->getModelData()->getMeshData()->getVectorById(mainNodesRigid_[0].second);
            }
            else
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ mainNodesRigid_id[0].second });
                if (points.empty())
                {
                    return;
                }
                masterpos = points[0].m_coor;
            }
            QVector<QVector3D>slaveposs;
            for (int i = 0; i < subNodesRigid_id.size(); ++i)
            {
                QVector3D slavepos;
                if (subNodesRigid_id[i].first == PickDataType::Node)
                {
                    slavepos = getCVM->getModelData()->getMeshData()->getVectorById(subNodesRigid_id[i].second);
                }
                else
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ subNodesRigid_id[i].second });
                    if (points.empty())
                    {
                        return;
                    }
                    slavepos = points[0].m_coor;
                }
                slaveposs.append(slavepos);
            }
            assignConnector(_connectorData, masterpos, slaveposs, ConnectorType::Connector_Rigid);
        }
        else if (idx == 1)
        {
            QVector3D masterpos;
            if (mainNodesRigid_id[0].first == PickDataType::Node)
            {
                masterpos = getCVM->getModelData()->getMeshData()->getVectorById(mainNodesRigid_id[0].second);
            }
            else
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ mainNodesRigid_id[0].second });
                if (points.empty())
                {
                    return;
                }
                masterpos = points[0].m_coor;
            }
            assignConnector(_connectorData, masterpos, renderposRigid, ConnectorType::Connector_Rigid);
        }
    }
    break;
    case MxFunctions::Connector_Flexible:
    {
        mainNodesFlexible_.clear();
        subNodesFlexible_.clear();
        mainNodesFlexible_id.clear();
        subNodesFlexible_id.clear();
        renderposFlexible.clear();

        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);

        if (connectorPtr->subnodes_.first != MxFunctions::PickDataType::None)
        {
            mainNodesFlexible_.push_back(connectorPtr->subnodes_);
            mainNodesFlexible_id.push_back(std::make_pair(connectorPtr->subnodes_.first, connectorPtr->subnodes_.second.result_id));
        }
        else
        {
            mainNodesFlexible_.clear();
            mainNodesFlexible_id.clear();
        }

        if (connectorPtr->mainnodes_.size() != 0)
        {
            for (auto it = connectorPtr->mainnodes_.begin(); it != connectorPtr->mainnodes_.end(); it++)
            {
                for (int i = 0; i < it->second.size(); ++i)
                {
                    subNodesFlexible_.push_back(std::make_pair(it->first, it->second[i]));
                }
            }
            for (int i = 0; i < subNodesFlexible_.size(); ++i)
            {
                subNodesFlexible_id.push_back(std::make_pair(subNodesFlexible_[i].first, subNodesFlexible_[i].second.result_id));
            }
        }
        else
        {
            subNodesFlexible_.clear();
            subNodesFlexible_id.clear();
        }

        if (connectorPtr->way_ == 1)
        {
            //将渲染位置计算出来
            renderposFlexible.clear();
            renderposFlexible = transPickedToNodes(subNodesFlexible_id);
        }

        if (mainNodesFlexible_id.empty() || subNodesFlexible_id.empty())
        {
            assignConnector(_connectorData, {}, {}, ConnectorType::Connector_Flexible);
            return;
        }

        int idx = connectorPtr->way_;
        if (idx == 0)
        {

            QVector3D masterpos;
            if (mainNodesFlexible_id[0].first == PickDataType::Node)
            {
                masterpos = getCVM->getModelData()->getMeshData()->getVectorById(mainNodesFlexible_id[0].second);
            }
            else
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ mainNodesFlexible_id[0].second });
                if (points.empty())
                {
                    return;
                }
                masterpos = points[0].m_coor;
            }
            QVector<QVector3D>slaveposs;
            for (int i = 0; i < subNodesFlexible_id.size(); ++i)
            {
                QVector3D slavepos;
                if (subNodesFlexible_id[i].first == PickDataType::Node)
                {
                    slavepos = getCVM->getModelData()->getMeshData()->getVectorById(subNodesFlexible_id[i].second);
                }
                else
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ subNodesFlexible_id[i].second });
                    if (points.empty())
                    {
                        return;
                    }
                    slavepos = points[0].m_coor;
                }
                slaveposs.append(slavepos);
            }
            assignConnector(_connectorData, masterpos, slaveposs, ConnectorType::Connector_Flexible);

        }
        else if (idx == 1)
        {
            QVector3D masterpos;
            if (mainNodesFlexible_id[0].first == PickDataType::Node)
            {
                masterpos = getCVM->getModelData()->getMeshData()->getVectorById(mainNodesFlexible_id[0].second);
            }
            else
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ mainNodesFlexible_id[0].second });
                if (points.empty())
                {
                    return;
                }
                masterpos = points[0].m_coor;
            }

            assignConnector(_connectorData, masterpos, renderposFlexible, ConnectorType::Connector_Flexible);

        }
    }
    break;
    case MxFunctions::Connector_MPC:
    {
        mainNodesMPC_.clear();
        subNodesMPC_.clear();
        mainNodesMPC_id.clear();
        subNodesMPC_id.clear();
        renderposMPC.clear();

        std::shared_ptr<MechConnectorData::mConnectorMPC> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);

        if (connectorPtr->subnodes_.first != MxFunctions::PickDataType::None)
        {
            mainNodesMPC_.push_back(connectorPtr->subnodes_);
            mainNodesMPC_id.push_back(std::make_pair(connectorPtr->subnodes_.first, connectorPtr->subnodes_.second.result_id));
        }
        else
        {
            mainNodesMPC_.clear();
            mainNodesMPC_id.clear();
        }

        if (connectorPtr->mainnodes_.size() != 0)
        {
            for (auto it = connectorPtr->mainnodes_.begin(); it != connectorPtr->mainnodes_.end(); it++)
            {
                for (int i = 0; i < it->second.size(); ++i)
                {
                    subNodesMPC_.push_back(std::make_pair(it->first, it->second[i]));
                }
            }
            for (int i = 0; i < subNodesMPC_.size(); ++i)
            {
                subNodesMPC_id.push_back(std::make_pair(subNodesMPC_[i].first, subNodesMPC_[i].second.result_id));
            }
        }
        else
        {
            subNodesMPC_.clear();
            subNodesMPC_id.clear();
        }

        //将渲染位置计算出来
        renderposMPC.clear();
        renderposMPC = transPickedToNodes(subNodesMPC_id);

        if (mainNodesMPC_id.empty() || subNodesMPC_id.empty())
        {
            assignConnector(_connectorData, {}, {}, ConnectorType::Connector_MPC);
            return;
        }

        QVector3D masterpos;
        if (mainNodesMPC_id[0].first == PickDataType::Node)
        {
            masterpos = getCVM->getModelData()->getMeshData()->getVectorById(mainNodesMPC_id[0].second);
        }
        else
        {
            auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ mainNodesMPC_id[0].second });
            if (points.empty())
            {
                return;
            }
            masterpos = points[0].m_coor;
        }

        assignConnector(_connectorData, masterpos, renderposMPC, ConnectorType::Connector_MPC);


    }
    break;
    case MxFunctions::Connector_DistributedCoupling:
    {
        mainNodesDistributedCoupling_.clear();
        subNodesDistributedCoupling_.clear();
        mainNodesDistributedCoupling_id.clear();
        subNodesDistributedCoupling_id.clear();
        renderposDistributedCoupling.clear();

        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);

        if (connectorPtr->subnodes_.first != MxFunctions::PickDataType::None)
        {
            mainNodesDistributedCoupling_.push_back(connectorPtr->subnodes_);
            mainNodesDistributedCoupling_id.push_back(std::make_pair(connectorPtr->subnodes_.first, connectorPtr->subnodes_.second.result_id));
        }
        else
        {
            mainNodesDistributedCoupling_.clear();
            mainNodesDistributedCoupling_id.clear();
        }

        if (connectorPtr->mainnodes_.size() != 0)
        {
            for (auto it = connectorPtr->mainnodes_.begin(); it != connectorPtr->mainnodes_.end(); it++)
            {
                for (int i = 0; i < it->second.size(); ++i)
                {
                    subNodesDistributedCoupling_.push_back(std::make_pair(it->first, it->second[i]));
                }
            }
            for (int i = 0; i < subNodesDistributedCoupling_.size(); ++i)
            {
                subNodesDistributedCoupling_id.push_back(std::make_pair(subNodesDistributedCoupling_[i].first, subNodesDistributedCoupling_[i].second.result_id));
            }
        }
        else
        {
            subNodesDistributedCoupling_.clear();
            subNodesDistributedCoupling_id.clear();
        }

        if (connectorPtr->way_ == 1)
        {
            //将渲染位置计算出来
            renderposDistributedCoupling.clear();
            renderposDistributedCoupling = transPickedToNodes(subNodesDistributedCoupling_id);
        }

        if (mainNodesDistributedCoupling_id.empty() || subNodesDistributedCoupling_id.empty())
        {
            assignConnector(_connectorData, {}, {}, ConnectorType::Connector_DistributedCoupling);
            return;
        }

        int idx = connectorPtr->way_;
        if (idx == 0)
        {

            QVector3D masterpos;
            if (mainNodesDistributedCoupling_id[0].first == PickDataType::Node)
            {
                masterpos = getCVM->getModelData()->getMeshData()->getVectorById(mainNodesDistributedCoupling_id[0].second);
            }
            else
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ mainNodesDistributedCoupling_id[0].second });
                if (points.empty())
                {
                    return;
                }
                masterpos = points[0].m_coor;
            }
            QVector<QVector3D>slaveposs;
            for (int i = 0; i < subNodesDistributedCoupling_id.size(); ++i)
            {
                QVector3D slavepos;
                if (subNodesDistributedCoupling_id[i].first == PickDataType::Node)
                {
                    slavepos = getCVM->getModelData()->getMeshData()->getVectorById(subNodesDistributedCoupling_id[i].second);
                }
                else
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ subNodesDistributedCoupling_id[i].second });
                    if (points.empty())
                    {
                        return;
                    }
                    slavepos = points[0].m_coor;
                }
                slaveposs.append(slavepos);
            }
            assignConnector(_connectorData, masterpos, slaveposs, ConnectorType::Connector_DistributedCoupling);

        }
        else if (idx == 1)
        {
            QVector3D masterpos;
            if (mainNodesDistributedCoupling_id[0].first == PickDataType::Node)
            {
                masterpos = getCVM->getModelData()->getMeshData()->getVectorById(mainNodesDistributedCoupling_id[0].second);
            }
            else
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ mainNodesDistributedCoupling_id[0].second });
                if (points.empty())
                {
                    return;
                }
                masterpos = points[0].m_coor;
            }

            assignConnector(_connectorData, masterpos, renderposDistributedCoupling, ConnectorType::Connector_DistributedCoupling);

        }
    }
    break;
    case MxFunctions::Connector_BearingUnits:
    {
        node1BearingUnits_.clear();
        node2BearingUnits_.clear();
        node1BearingUnits_id.clear();
        node2BearingUnits_id.clear();
        renderposBearingUnits.clear();

        std::shared_ptr<MechConnectorData::mConnectorBearingUnits> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (connectorPtr == nullptr) return;
        if (connectorPtr->node1_.first != PickDataType::None)
        {
            node1BearingUnits_.push_back(std::make_pair(PickDataType::Node, RenderPickResultData(connectorPtr->node1_.second.result_id)));
            node1BearingUnits_id.push_back(std::make_pair(PickDataType::Node, connectorPtr->node1_.second.result_id));

        }
        else
        {
            node1BearingUnits_.clear();
            node1BearingUnits_id.clear();
        }
        if (connectorPtr->type_ == 1)
        {

            if (connectorPtr->node2_.first == PickDataType::Node)
            {

                node2BearingUnits_.push_back(std::make_pair(PickDataType::Node, RenderPickResultData(connectorPtr->node2_.second.result_id)));
                node2BearingUnits_id.push_back(std::make_pair(PickDataType::Node, connectorPtr->node2_.second.result_id));

            }
            else
            {
                node2BearingUnits_.clear();
                node2BearingUnits_id.clear();
            }
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        int node1Id = -1;
        int node2Id = -1;
        if (!node1BearingUnits_id.empty() && !node2BearingUnits_id.empty())
        {
            node1Id = node1BearingUnits_id[0].second;
            node2Id = node2BearingUnits_id[0].second;
        }
        if (node1Id <= 0 || node2Id <= 0)
            break;
        if (!meshData || !meshData->NodeIsHaveByID(node1Id) || !meshData->NodeIsHaveByID(node2Id))
            break;
        QVector3D node1pos = meshData->getVectorById(node1Id);
        QVector3D node2pos = meshData->getVectorById(node2Id);
        QVector<QVector3D> node2posV;
        node2posV.push_back(node2pos);
        assignConnector(_connectorData, node1pos, node2posV, ConnectorType::Connector_BearingUnits);
    }
    break;
    case MxFunctions::Connector_Disperse:
        break;
    case MxFunctions::Connector_Bolt:
    {
        std::shared_ptr<MechConnectorData::mConnectorBolt> connector = std::dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(_connectorData);

        //true为作用面上；false为作用线上
        //作用对象
        if (connector->type_ == 0) //一维作用于几何线和单元线
        {
            vector<std::pair<QVector3D, QVector3D>> pos_dir;
            QVector<QVector3D> dirs;
            auto objs = getConnectorBolt(connector, connector->type_, dirs);
            for (int i = 0; i < objs.size(); ++i)
            {
                pos_dir.push_back(std::make_pair(objs[i], dirs[i]));
            }
            assignConnectorBolt(connector, pos_dir, "Connector_Bolt", QVector3D(255.0f, 255.0f, 0.0f), false);
        }
        else if (connector->type_ == 1) //三维作用于几何面和单元面
        {
            vector<std::pair<QVector3D, QVector3D>> pos_dir;
            QVector<QVector3D> dirs;
            auto objs = getConnectorBolt(connector, connector->type_, dirs);
            for (int i = 0; i < objs.size(); ++i)
            {
                pos_dir.push_back(std::make_pair(objs[i], dirs[i]));
            }
            assignConnectorBolt(connector, pos_dir, "Connector_Bolt", QVector3D(255.0f, 255.0f, 0.0f), true);
        }
    }
    break;
    default:
        break;
    }

}

void AllRenderSymbolManager::rendConnector(QVector<int> phyIds)
{
    auto meshData = getCVM->getModelData()->getMeshData();
    auto widget_ptr = getCVM->getPreRenderWidget();
    auto highlight_controller = widget_ptr->getHighlightController();

    // 统一创建高亮对象，只创建一次
    mGeoSelectData* geoMasterData = new mGeoSelectData();
    mGeoSelectData* geoSlaveData = new mGeoSelectData();
    MeshSelectMemoryData* meshMasterData = new MeshSelectMemoryData();
    MeshSelectMemoryData* meshSlaveData = new MeshSelectMemoryData();

    for (auto phyId : phyIds)
    {
        auto _connectorData = getCVM->getModelData()->getManager<mConnectorManager>(Physical_Connector).getConnectorData(phyId);

        if (!_connectorData)
            continue;

        if (_connectorData->getConnectorType() != Connector_Bind)
            continue;

        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();

        // 主面 Master (红色)
        int masterSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Connector, _connectorData->getConnectorId() },
            Physical_Set, MapRelation_Master);

        if (masterSetId != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(masterSetId);
            if (!set) continue;

            if (set->getSetType() == Mix)
            {
                auto mixset = dynamic_cast<mMixSet*>(set);
                auto mixData = mixset->getMixSetData();
                for (const auto& pair : mixData)
                {
                    if (pair.second.empty()) continue;

                    if (pair.first == MxFunctions::PickDataType::GeoFace)
                    {
                        std::vector<int> res;
                        for (const auto& resultData : pair.second)
                        {
                            auto id = resultData.result_id;
                            auto partId = meshData->getPartIdByGeoFace(id);
                            auto partData = meshData->getPartData(partId);
                            if (partData && !partData->getPartMask())
                                res.push_back(id);
                        }
                        geoMasterData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
                    }
                    else if (pair.first == MxFunctions::PickDataType::ElementFace)
                    {
                        std::vector<MeshSelectInfoData> res;
                        for (const auto& pickResultData : pair.second)
                            res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));

                        meshMasterData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
                    }
                }
            }
            else if (set->getSetType() == GeoFace)
            {
                auto geoset = dynamic_cast<mGeoSet*>(set);
                std::vector<int> ids = geoset->getGeoSetData();
                std::vector<int> res;
                for (auto id : ids)
                {
                    auto partId = meshData->getPartIdByGeoFace(id);
                    auto partData = meshData->getPartData(partId);
                    if (partData && !partData->getPartMask())
                        res.push_back(id);
                }
                geoMasterData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
            }
            else if (set->getSetType() == ElementSurface)
            {
                auto eleset = dynamic_cast<mElementSurface*>(set);
                std::vector<RenderPickResultData> data = eleset->getElementSurfaceData();
                std::vector<MeshSelectInfoData> res;
                for (auto& pickResultData : data)
                    res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));

                meshMasterData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
            }
        }

        // 从面 Slave (蓝色)
        int slaveSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Connector, _connectorData->getConnectorId() },
            Physical_Set, MapRelation_Slave);

        if (slaveSetId != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(slaveSetId);
            if (!set) continue;

            if (set->getSetType() == Mix)
            {
                auto mixset = dynamic_cast<mMixSet*>(set);
                auto mixData = mixset->getMixSetData();
                for (const auto& pair : mixData)
                {
                    if (pair.second.empty()) continue;

                    if (pair.first == PickDataType::GeoFace)
                    {
                        std::vector<int> res;
                        for (const auto& resultData : pair.second)
                        {
                            auto id = resultData.result_id;
                            auto partId = meshData->getPartIdByGeoFace(id);
                            auto partData = meshData->getPartData(partId);
                            if (partData && !partData->getPartMask())
                                res.push_back(id);
                        }
                        geoSlaveData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
                    }
                    else if (pair.first == PickDataType::ElementFace)
                    {
                        std::vector<MeshSelectInfoData> res;
                        for (const auto& pickResultData : pair.second)
                            res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));

                        meshSlaveData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
                    }
                }
            }
            else if (set->getSetType() == GeoFace)
            {
                auto geoset = dynamic_cast<mGeoSet*>(set);
                std::vector<int> ids = geoset->getGeoSetData();
                std::vector<int> res;
                for (auto id : ids)
                {
                    auto partId = meshData->getPartIdByGeoFace(id);
                    auto partData = meshData->getPartData(partId);
                    if (partData && !partData->getPartMask())
                        res.push_back(id);
                }
                geoSlaveData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
            }
            else if (set->getSetType() == ElementSurface)
            {
                auto eleset = dynamic_cast<mElementSurface*>(set);
                std::vector<RenderPickResultData> data = eleset->getElementSurfaceData();
                std::vector<MeshSelectInfoData> res;
                for (auto& pickResultData : data)
                    res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));

                meshSlaveData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
            }
        }
    }

    // 循环结束，统一更新高亮
    if (geoMasterData)
    {
        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender("red");
        geoHighLightRender->setHighLightColor(255, 0, 0);
        geoHighLightRender->updateHighLightRender(geoMasterData);
    }
    else
    {
        delete geoMasterData;
    }

    if (geoSlaveData)
    {
        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender("blue", false);
        geoHighLightRender->setHighLightColor(0, 0, 255);
        geoHighLightRender->updateHighLightRender(geoSlaveData);
    }
    else
    {
        delete geoSlaveData;
    }

    if (meshMasterData)
    {
        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender("red");
        meshHighLightRender->setHighLightColor(255, 0, 0);
        meshHighLightRender->updateHighLightRender(meshMasterData);
    }
    else
    {
        delete meshMasterData;
    }

    if (meshSlaveData)
    {
        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender("blue", false);
        meshHighLightRender->setHighLightColor(0, 0, 255);
        meshHighLightRender->updateHighLightRender(meshSlaveData);
    }
    else
    {
        delete meshSlaveData;
    }
}

//连接副渲染
void AllRenderSymbolManager::showDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData)
{
    if (_deputyData == nullptr)
        return;
    QString symbol = QString("deputy_%1").arg(_deputyData->getDeputyId());
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    vector<QString> showSymbol;
    showSymbol.push_back(symbol);

    if (!symbolController->isExistSymbol(symbol))
    {
        rendDeputy(_deputyData, true);
    }
}

void AllRenderSymbolManager::rendDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData, bool showhide)
{
    if (_deputyData == nullptr) return;

    auto type = _deputyData->getDeputyType();

    switch (type)
    {
    case MxFunctions::Deputy_Connector:
    {
        mainNodesDeputy_.clear();
        subNodesDeputy_.clear();
        mainNodesDeputy_id.clear();
        subNodesDeputy_id.clear();

        std::shared_ptr<MechConnectorData::mConnectingDeputy> deputyPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectingDeputy>(_deputyData);

        if (deputyPtr == nullptr) return;

        if (deputyPtr->refNodeIds_.size() != 0)
        {
            for (int i = 0; i < deputyPtr->refNodeIds_.size(); ++i)
            {
                mainNodesDeputy_.push_back(std::make_pair(PickDataType::Node, RenderPickResultData(deputyPtr->refNodeIds_[i])));
                mainNodesDeputy_id.push_back(std::make_pair(PickDataType::Node, deputyPtr->refNodeIds_[i]));
            }
        }
        else
        {
            mainNodesDeputy_.clear();
            mainNodesDeputy_id.clear();
        }

        if (deputyPtr->movNodeIds_.size() != 0)
        {
            for (int i = 0; i < deputyPtr->movNodeIds_.size(); ++i)
            {
                subNodesDeputy_.push_back(std::make_pair(PickDataType::Node, RenderPickResultData(deputyPtr->movNodeIds_[i])));
                subNodesDeputy_id.push_back(std::make_pair(PickDataType::Node, deputyPtr->movNodeIds_[i]));
            }
        }
        else
        {
            subNodesDeputy_.clear();
            subNodesDeputy_id.clear();
        }

        if (mainNodesDeputy_.empty() || subNodesDeputy_.empty())
        {
            clearDeputy(_deputyData);
            return;
        }

        QVector<QVector3D> allRefPos;
        QVector<QVector3D> allMovPos;
        auto meshData = getCVM->getModelData()->getMeshData();
        for (int i = 0; i < mainNodesDeputy_id.size() && i < subNodesDeputy_id.size(); ++i)
        {
            int refId = mainNodesDeputy_id[i].second;
            int movId = subNodesDeputy_id[i].second;
            if (refId <= 0 || movId <= 0)
                continue;
            if (!meshData || !meshData->NodeIsHaveByID(refId) || !meshData->NodeIsHaveByID(movId))
                continue;
            QVector3D refPos = meshData->getVectorById(refId);
            QVector3D movPos = meshData->getVectorById(movId);
            allRefPos.append(refPos);
            allMovPos.append(movPos);
        }

        if (!allRefPos.isEmpty() && allRefPos.size() == allMovPos.size())
        {
            assignDeputy(_deputyData, allRefPos, allMovPos);
        }
        else
        {
            clearDeputy(_deputyData);
        }
    }
    break;
    case MxFunctions::Deputy_Fastener:
    {/*
        std::shared_ptr<MechConnectorData::mConnectorFastener> fastenerPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFastener>(_deputyData);
        if (fastenerPtr == nullptr) return;
        // fastener 节点渲染逻辑，暂时复用节点获取方式
        if (fastenerPtr->defineType_ == 0 && fastenerPtr->nodeId_ != -1)
        {
            QVector3D pos = getCVM->getModelData()->getMeshData()->getVectorById(fastenerPtr->nodeId_);
            assignDeputy(_deputyData, pos, pos);
        }*/
    }
    break;
    default:
        break;
    }
}

//预定义场渲染
void AllRenderSymbolManager::showPredefinedField(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData)
{
    if (fieldData == nullptr)
        return;
    QString symbol = QString("predefinedfield_%1").arg(fieldData->getPredefinedFieldId());
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    vector<QString> showSymbol;
    showSymbol.push_back(symbol);

    if (!symbolController->isExistSymbol(symbol))
    {
        rendPredefinedField(fieldData);
    }
}

void AllRenderSymbolManager::rendPredefinedField(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData)
{
    if (fieldData == nullptr) return;

    auto type = fieldData->getPredefinedFieldType();

    switch (type)
    {
    case MxFunctions::PREDEFINED_FIELD_INIT_TEMP:
    {
        std::shared_ptr<MechCaseData::mPredefinedFieldInitTemp> field = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldInitTemp>(fieldData);
        QVector<QVector3D> objs;
        if (field->_defineType == 1)
        {
            objs = getIndependentPredefinedFieldSetInitTemp(field);
        }
        else
        {
            // 统一赋值：从集合获取数据
            objs = getPredefinedFieldSet(fieldData);
        }
        assignPredefinedField(fieldData, objs, "", QVector3D(255.0f, 165.0f, 0.0f));
    }
    break;
    case MxFunctions::PREDEFINED_FIELD_TEMP_LOAD:
    {
        std::shared_ptr<MechCaseData::mPredefinedFieldTempLoad> field = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldTempLoad>(fieldData);
        QVector<QVector3D> objs;
        if (field->_defineType == 1)
        {
            objs = getIndependentPredefinedFieldSetTempLoad(field);
        }
        else
        {
            // 统一赋值（_defineType == 0）或文件定义（_defineType == 2）：从集合获取数据
            objs = getPredefinedFieldSet(fieldData);
        }
        assignPredefinedField(fieldData, objs, "T", QVector3D(255.0f, 165.0f, 0.0f));
    }
    break;
    default:
        break;
    }
}

//接触渲染
namespace
{
    // 节点拾取数据 → 网格高亮节点数据（点面接触从面节点模式高亮用）
    void appendNodeRenderData(const std::vector<RenderPickResultData>& nodes, std::vector<mxr::MeshSelectInfoData>& nodeRenderData)
    {
        nodeRenderData.reserve(nodeRenderData.size() + nodes.size());
        for (const auto& d : nodes)
        {
            mxr::MeshSelectInfoData n;
            n.data_id.node_id = d.result_id;
            n.part_id = d.part_id;
            nodeRenderData.push_back(n);
        }
    }

    void appendMeshRenderData(const std::vector<RenderPickResultData>& data,
        mxr::SelectionInfoType type, MeshSelectMemoryData* meshData)
    {
        std::vector<mxr::MeshSelectInfoData> meshIds;
        meshIds.reserve(data.size());
        for (const auto& d : data)
        {
            mxr::MeshSelectInfoData n;
            n.data_id.element_id = d.result_id;
            n.data_index.line_index = d.result_id2;
            n.part_id = d.part_id;
            meshIds.push_back(n);
        }
        meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, type, meshIds);
    }

    void appendContactTypedData(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data,
        std::vector<RenderPickResultData>& meshRenderData, std::vector<int>& geoRenderData,
        std::vector<mxr::MeshSelectInfoData>& nodeRenderData,
        std::vector<RenderPickResultData>* meshEdgeRenderData = nullptr,
        std::vector<int>* geoEdgeRenderData = nullptr)
    {
        for (const auto& pair : data)
        {
            if (pair.first == PickDataType::GeoFace)
            {
                for (const auto& d : pair.second) geoRenderData.push_back(d.result_id);
            }
            else if (pair.first == PickDataType::ElementFace)
            {
                meshRenderData.insert(meshRenderData.end(), pair.second.begin(), pair.second.end());
            }
            else if (pair.first == PickDataType::Node)
            {
                appendNodeRenderData(pair.second, nodeRenderData);
            }
            else if (pair.first == PickDataType::GeoEdge && geoEdgeRenderData)
            {
                for (const auto& d : pair.second) geoEdgeRenderData->push_back(d.result_id);
            }
            else if (pair.first == PickDataType::ElementEdge && meshEdgeRenderData)
            {
                meshEdgeRenderData->insert(meshEdgeRenderData->end(), pair.second.begin(), pair.second.end());
            }
        }
    }

    void updateContactHighlight(mxr::CCSMRendWidget* widget, const char* color,
        const std::vector<RenderPickResultData>& meshRenderData,
        const std::vector<int>& geoRenderData,
        const std::vector<mxr::MeshSelectInfoData>& nodeRenderData,
        const std::vector<RenderPickResultData>& meshEdgeRenderData = {},
        const std::vector<int>& geoEdgeRenderData = {})
    {
        if (!widget) return;
        auto* highlightController = widget->getHighlightController();
        const int r = std::string(color) == "red" ? 255 : 0;
        const int b = std::string(color) == "blue" ? 255 : 0;

        if (!geoRenderData.empty() || !geoEdgeRenderData.empty())
        {
            auto* render = highlightController->getExtraPreGeoHighLightRender(color, true, -2.0f);
            render->setHighLightColor(r, 0, b);
            auto* geoData = new mGeoSelectData();
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoFace, geoRenderData);
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoEdge, geoEdgeRenderData);
            render->updateHighLightRender(geoData);
            delete geoData;
        }

        if (!meshRenderData.empty() || !meshEdgeRenderData.empty() || !nodeRenderData.empty())
        {
            auto* render = highlightController->getExtraPreMeshHighLightRender(color);
            render->setHighLightColor(r, 0, b);
            auto* meshData = new MeshSelectMemoryData();
            appendMeshRenderData(meshRenderData, SelectionInfoType::MeshFace, meshData);
            appendMeshRenderData(meshEdgeRenderData, SelectionInfoType::MeshLine, meshData);
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshNode, nodeRenderData);
            render->updateHighLightRender(meshData);
            delete meshData;
        }
    }

    void getContactSetRenderData(const std::set<int>& setIDs,
        std::vector<RenderPickResultData>& meshRenderData, std::vector<int>& geoRenderData,
        std::vector<mxr::MeshSelectInfoData>& nodeRenderData,
        std::vector<RenderPickResultData>& meshEdgeRenderData,
        std::vector<int>& geoEdgeRenderData)
    {
        for (int setID : setIDs)
        {
            auto* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
            if (!set) continue;
            if (set->getSetType() == Mix)
            {
                if (auto* mixSet = dynamic_cast<mMixSet*>(set))
                {
                    appendContactTypedData(mixSet->getMixSetData(), meshRenderData, geoRenderData,
                        nodeRenderData, &meshEdgeRenderData, &geoEdgeRenderData);
                }
            }
            else
            {
                appendContactTypedData(MechMainWindow::resolveSetToTypedData(set->getSetName()),
                    meshRenderData, geoRenderData, nodeRenderData, &meshEdgeRenderData, &geoEdgeRenderData);
            }
        }
    }
}

void AllRenderSymbolManager::rendContact(MechContactData::mContactBase* _contactData)
{
    if (!_contactData || _contactData->getContactType() == CommonContact) return;
    auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
    auto* widget = getCVM->getPreRenderWidget();
    if (!widget) return;

    const int contactId = _contactData->getContactId();
    const int masterSetId = mapRelationManager.getMapRelation<int>(
        { PhysicalFieldType::Physical_Contact, contactId }, Physical_Set, MapRelation_Master);
    const int slaveSetId = mapRelationManager.getMapRelation<int>(
        { PhysicalFieldType::Physical_Contact, contactId }, Physical_Set, MapRelation_Slave);

    auto renderSet = [&](int setId, const char* color) {
        if (setId == -1) return;
        std::vector<RenderPickResultData> meshRenderData, meshEdgeRenderData;
        std::vector<int> geoRenderData, geoEdgeRenderData;
        std::vector<mxr::MeshSelectInfoData> nodeRenderData;
        getContactSetRenderData({ setId }, meshRenderData, geoRenderData, nodeRenderData,
            meshEdgeRenderData, geoEdgeRenderData);
        updateContactHighlight(widget, color, meshRenderData, geoRenderData, nodeRenderData,
            meshEdgeRenderData, geoEdgeRenderData);
        };

    renderSet(masterSetId, "red");
    if (_contactData->getContactType() != SelfContact) renderSet(slaveSetId, "blue");
}

void AllRenderSymbolManager::rendContact(QVector<int> phyIds)
{
    auto* widget = getCVM->getPreRenderWidget();
    if (!widget) return;
    std::set<int> masterData, slaveData;

    for (auto phyId : phyIds)
    {
        auto* contact = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getContactData(phyId);
        if (!contact || contact->getContactType() == CommonContact) continue;

        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        const int masterSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Contact, contact->getContactId() }, Physical_Set, MapRelation_Master);
        const int slaveSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Contact, contact->getContactId() }, Physical_Set, MapRelation_Slave);
        if (masterSetId != -1) masterData.insert(masterSetId);
        if (slaveSetId != -1 && contact->getContactType() != SelfContact) slaveData.insert(slaveSetId);
    }

    auto renderSets = [&](const std::set<int>& setIDs, const char* color) {
        std::vector<RenderPickResultData> meshRenderData, meshEdgeRenderData;
        std::vector<int> geoRenderData, geoEdgeRenderData;
        std::vector<mxr::MeshSelectInfoData> nodeRenderData;
        getContactSetRenderData(setIDs, meshRenderData, geoRenderData, nodeRenderData,
            meshEdgeRenderData, geoEdgeRenderData);
        updateContactHighlight(widget, color, meshRenderData, geoRenderData, nodeRenderData,
            meshEdgeRenderData, geoEdgeRenderData);
        };
    renderSets(masterData, "red");
    renderSets(slaveData, "blue");
}

void AllRenderSymbolManager::rendContact(const MechMainWindow::PickWidgetData& data, std::vector<RenderPickResultData>& meshRenderData, std::vector<int>& geoRenderData, std::vector<mxr::MeshSelectInfoData>* nodeRenderData,
    std::vector<RenderPickResultData>* meshEdgeRenderData, std::vector<int>* geoEdgeRenderData)
{
    std::vector<mxr::MeshSelectInfoData> localNodeData;
    std::vector<mxr::MeshSelectInfoData>& nodeData = nodeRenderData ? *nodeRenderData : localNodeData;

    // 接触集合转换支持几何边/单元边；节点和面保持原有输出接口。
    std::vector<RenderPickResultData> localMeshEdges;
    std::vector<int> localGeoEdges;
    auto& meshEdges = meshEdgeRenderData ? *meshEdgeRenderData : localMeshEdges;
    auto& geoEdges = geoEdgeRenderData ? *geoEdgeRenderData : localGeoEdges;

    for (const auto& setName : data.selectedSetNames)
    {
        appendContactTypedData(MechMainWindow::resolveSetToTypedData(setName),
            meshRenderData, geoRenderData, nodeData, &meshEdges, &geoEdges);
    }
    appendContactTypedData(data.pickedData, meshRenderData, geoRenderData, nodeData, &meshEdges, &geoEdges);
}

//周期边界渲染
void AllRenderSymbolManager::rendPeriodicBoundary(int pbId)
{
    if (pbId <= 0) return;
    auto _pbData = getCVM->getModelData()->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getPeriodicBoundaryData(pbId);
    if (!_pbData) return;

    auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
    auto meshData = getCVM->getModelData()->getMeshData();
    auto widget_ptr = getCVM->getPreRenderWidget();
    if (!widget_ptr) return;
    auto highlight_controller = widget_ptr->getHighlightController();
    if (!highlight_controller) return;

    auto addToGeoData = [&](mSetBase* set, mGeoSelectData* geoData) {
        if (!set || !geoData) return;
        if (set->getSetType() == Mix)
        {
            auto mixset = dynamic_cast<mMixSet*>(set);
            auto mixData = mixset->getMixSetData();
            for (const auto& pair : mixData)
            {
                if (pair.first != PickDataType::GeoFace || pair.second.empty()) continue;
                std::vector<int> res;
                for (const auto& resultData : pair.second)
                {
                    auto id = resultData.result_id;
                    auto partId = meshData->getPartIdByGeoFace(id);
                    auto partData = meshData->getPartData(partId);
                    if (partData && !partData->getPartMask())
                        res.push_back(id);
                }
                geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
            }
        }
        else if (set->getSetType() == GeoFace)
        {
            auto geoset = dynamic_cast<mGeoSet*>(set);
            std::vector<int> ids = geoset->getGeoSetData();
            std::vector<int> res;
            for (auto id : ids)
            {
                auto partId = meshData->getPartIdByGeoFace(id);
                auto partData = meshData->getPartData(partId);
                if (partData && !partData->getPartMask())
                    res.push_back(id);
            }
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
        }
        };

    auto addToMeshData = [&](mSetBase* set, MeshSelectMemoryData* meshDataHL) {
        if (!set || !meshDataHL) return;
        if (set->getSetType() == Mix)
        {
            auto mixset = dynamic_cast<mMixSet*>(set);
            auto mixData = mixset->getMixSetData();
            for (const auto& pair : mixData)
            {
                if (pair.first != PickDataType::ElementFace || pair.second.empty()) continue;
                std::vector<MeshSelectInfoData> res;
                for (const auto& pickResultData : pair.second)
                    res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));
                meshDataHL->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
            }
        }
        else if (set->getSetType() == ElementSurface)
        {
            auto eleset = dynamic_cast<mElementSurface*>(set);
            std::vector<RenderPickResultData> data = eleset->getElementSurfaceData();
            std::vector<MeshSelectInfoData> res;
            for (auto& pickResultData : data)
                res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));
            meshDataHL->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
        }
        };

    // 反显主面集合
    int masterSetId = mapRelationManager.getMapRelation<int>({ PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() }, Physical_Set, MapRelation_Master);
    if (masterSetId != -1)
    {
        mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(masterSetId);
        if (set)
        {
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
            addToGeoData(set, geoData);
            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"));
            geoHighLightRender->setHighLightColor(255, 0, 0);
            geoHighLightRender->updateHighLightRender(geoData);

            MeshSelectMemoryData* meshDataHL = new MeshSelectMemoryData();
            meshDataHL->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
            addToMeshData(set, meshDataHL);
            auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
            meshHighLightRender->setHighLightColor(255, 0, 0);
            meshHighLightRender->updateHighLightRender(meshDataHL);
        }
    }

    // 反显从面集合
    int slaveSetId = mapRelationManager.getMapRelation<int>({ PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() }, Physical_Set, MapRelation_Slave);
    if (slaveSetId != -1)
    {
        mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(slaveSetId);
        if (set)
        {
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
            addToGeoData(set, geoData);
            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), false);
            geoHighLightRender->setHighLightColor(0, 0, 255);
            geoHighLightRender->updateHighLightRender(geoData);

            MeshSelectMemoryData* meshDataHL = new MeshSelectMemoryData();
            meshDataHL->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
            addToMeshData(set, meshDataHL);
            auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"), false);
            meshHighLightRender->setHighLightColor(0, 0, 255);
            meshHighLightRender->updateHighLightRender(meshDataHL);
        }
    }
}

void AllRenderSymbolManager::rendPeriodicBoundary(QVector<int> pbIds)
{
    auto meshData = getCVM->getModelData()->getMeshData();
    auto widget_ptr = getCVM->getPreRenderWidget();
    if (!widget_ptr) return;
    auto highlight_controller = widget_ptr->getHighlightController();
    if (!highlight_controller) return;

    mGeoSelectData* geoMasterData = new mGeoSelectData();
    mGeoSelectData* geoSlaveData = new mGeoSelectData();
    MeshSelectMemoryData* meshMasterData = new MeshSelectMemoryData();
    MeshSelectMemoryData* meshSlaveData = new MeshSelectMemoryData();

    for (auto pbId : pbIds)
    {
        if (pbId <= 0) continue;
        auto _pbData = getCVM->getModelData()->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getPeriodicBoundaryData(pbId);
        if (!_pbData) continue;

        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();

        // 主面 Master (红色)
        int masterSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() },
            Physical_Set, MapRelation_Master);

        if (masterSetId != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(masterSetId);
            if (!set) continue;

            if (set->getSetType() == Mix)
            {
                auto mixset = dynamic_cast<mMixSet*>(set);
                auto mixData = mixset->getMixSetData();
                for (const auto& pair : mixData)
                {
                    if (pair.second.empty()) continue;

                    if (pair.first == PickDataType::GeoFace)
                    {
                        std::vector<int> res;
                        for (const auto& resultData : pair.second)
                        {
                            auto id = resultData.result_id;
                            auto partId = meshData->getPartIdByGeoFace(id);
                            auto partData = meshData->getPartData(partId);
                            if (partData && !partData->getPartMask())
                                res.push_back(id);
                        }
                        geoMasterData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
                    }
                    else if (pair.first == PickDataType::ElementFace)
                    {
                        std::vector<MeshSelectInfoData> res;
                        for (const auto& pickResultData : pair.second)
                            res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));
                        meshMasterData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
                    }
                }
            }
            else if (set->getSetType() == GeoFace)
            {
                auto geoset = dynamic_cast<mGeoSet*>(set);
                std::vector<int> ids = geoset->getGeoSetData();
                std::vector<int> res;
                for (auto id : ids)
                {
                    auto partId = meshData->getPartIdByGeoFace(id);
                    auto partData = meshData->getPartData(partId);
                    if (partData && !partData->getPartMask())
                        res.push_back(id);
                }
                geoMasterData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
            }
            else if (set->getSetType() == ElementSurface)
            {
                auto eleset = dynamic_cast<mElementSurface*>(set);
                std::vector<RenderPickResultData> data = eleset->getElementSurfaceData();
                std::vector<MeshSelectInfoData> res;
                for (auto& pickResultData : data)
                    res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));
                meshMasterData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
            }
        }

        // 从面 Slave (蓝色)
        int slaveSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_PeriodicBoundary, _pbData->getPeriodicBoundaryId() },
            Physical_Set, MapRelation_Slave);

        if (slaveSetId != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(slaveSetId);
            if (!set) continue;

            if (set->getSetType() == Mix)
            {
                auto mixset = dynamic_cast<mMixSet*>(set);
                auto mixData = mixset->getMixSetData();
                for (const auto& pair : mixData)
                {
                    if (pair.second.empty()) continue;

                    if (pair.first == PickDataType::GeoFace)
                    {
                        std::vector<int> res;
                        for (const auto& resultData : pair.second)
                        {
                            auto id = resultData.result_id;
                            auto partId = meshData->getPartIdByGeoFace(id);
                            auto partData = meshData->getPartData(partId);
                            if (partData && !partData->getPartMask())
                                res.push_back(id);
                        }
                        geoSlaveData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
                    }
                    else if (pair.first == PickDataType::ElementFace)
                    {
                        std::vector<MeshSelectInfoData> res;
                        for (const auto& pickResultData : pair.second)
                            res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));
                        meshSlaveData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
                    }
                }
            }
            else if (set->getSetType() == GeoFace)
            {
                auto geoset = dynamic_cast<mGeoSet*>(set);
                std::vector<int> ids = geoset->getGeoSetData();
                std::vector<int> res;
                for (auto id : ids)
                {
                    auto partId = meshData->getPartIdByGeoFace(id);
                    auto partData = meshData->getPartData(partId);
                    if (partData && !partData->getPartMask())
                        res.push_back(id);
                }
                geoSlaveData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
            }
            else if (set->getSetType() == ElementSurface)
            {
                auto eleset = dynamic_cast<mElementSurface*>(set);
                std::vector<RenderPickResultData> data = eleset->getElementSurfaceData();
                std::vector<MeshSelectInfoData> res;
                for (auto& pickResultData : data)
                    res.push_back(PickData2SelectData(PickDataType::ElementFace, pickResultData));
                meshSlaveData->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
            }
        }
    }

    // 统一更新高亮
    auto geoMasterHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"));
    geoMasterHighLightRender->setHighLightColor(255, 0, 0);
    geoMasterHighLightRender->updateHighLightRender(geoMasterData);

    auto geoSlaveHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), false);
    geoSlaveHighLightRender->setHighLightColor(0, 0, 255);
    geoSlaveHighLightRender->updateHighLightRender(geoSlaveData);

    auto meshMasterHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
    meshMasterHighLightRender->setHighLightColor(255, 0, 0);
    meshMasterHighLightRender->updateHighLightRender(meshMasterData);

    auto meshSlaveHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"), false);
    meshSlaveHighLightRender->setHighLightColor(0, 0, 255);
    meshSlaveHighLightRender->updateHighLightRender(meshSlaveData);
}

//力学载荷采用此筛选对象函数，其他函数使用别的
QVector<QVector3D> AllRenderSymbolManager::getLoadSet(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    QVector<QVector3D> dirs;
    return getLoadSet(loadData, dirs);
}
QVector<QVector3D> AllRenderSymbolManager::getLoadSet(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs)
{
    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
        if (set->getInternal())
        {
            auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
            //获取集合的作用对象
            for (auto& tar : mixsetdata)
            {
                std::vector<int>ids;
                for (auto& id : tar.second)
                {
                    ids.push_back(id.result_id);
                }
                switch (tar.first)
                {
                case PickDataType::GeoPoint:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(ids);
                    for (auto point : points)
                    {
                        objs.push_back(point.m_coor);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case PickDataType::GeoEdge:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateEdgeAttr(ids);
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case PickDataType::GeoFace:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case PickDataType::GeoSolid:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case PickDataType::Node:
                {
                    for (auto id : ids)
                    {
                        objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id));
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else
        {
            MxFunctions::SetType settype = set->getSetType();
            switch (settype)
            {
            case MxFunctions::GeoPoint:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(geosetids);
                for (auto point : points)
                {
                    objs.push_back(point.m_coor);
                    dirs.push_back(QVector3D());
                }
            }
            break;
            case MxFunctions::GeoEdge:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateEdgeAttr(geosetids);
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                    dirs.push_back(points[0].m_dir);
                }
            }
            break;
            case MxFunctions::GeoFace:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                    dirs.push_back(points[0].m_dir);
                }
            }
            break;
            case MxFunctions::GeoSolid:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                    dirs.push_back(QVector3D());
                }
            }
            break;
            case MxFunctions::Node:
            {
                auto nodesetids = dynamic_cast<mNode*>(set)->getNodeData();
                for (auto point : nodesetids)
                {
                    objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(point.result_id));
                    dirs.push_back(QVector3D());
                }
            }
            break;
            default:
                break;
            }
        }
    }
    return objs;
}

//压力采用此筛选对象函数
QVector<QVector3D> AllRenderSymbolManager::getLoadSetPressure(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    QVector<QVector3D> dirs;
    return getLoadSetPressure(loadData, dirs);
}
QVector<QVector3D> AllRenderSymbolManager::getLoadSetPressure(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs)
{
    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
        if (set->getInternal())
        {
            auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
            //获取集合的作用对象
            for (auto& tar : mixsetdata)
            {
                std::vector<int>ids;
                for (auto& id : tar.second)
                {
                    ids.push_back(id.result_id);
                }
                switch (tar.first)
                {
                case PickDataType::GeoFace:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case PickDataType::ElementFace:
                {
                    for (auto& id : tar.second)
                    {
                        QVector3D centerPos;
                        QVector3D vertex;
                        getCVM->getModelData()->getMeshData()->GetOutVectorByMeshFaceID(id, vertex, centerPos);
                        objs.push_back(centerPos);
                        dirs.push_back(vertex);
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else
        {
            MxFunctions::SetType settype = set->getSetType();
            switch (settype)
            {
            case MxFunctions::GeoFace:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                    dirs.push_back(points[0].m_dir);
                }
            }
            break;
            case MxFunctions::ElementSurface:
            {
                auto elesurfaceset = dynamic_cast<MechSetData::mElementSurface*>(set);
                if (elesurfaceset != nullptr && !elesurfaceset->getElementSurfaceData().empty())
                {
                    auto surfaceids = elesurfaceset->getElementSurfaceData();
                    for (auto id : surfaceids)
                    {
                        QVector3D  centerPos;
                        QVector3D   vertexs;
                        getCVM->getModelData()->getMeshData()->GetOutVectorByMeshFaceID(id, vertexs, centerPos);
                        objs.push_back(centerPos);
                        dirs.push_back(vertexs);
                    }
                }
            }
            break;
            default:
                break;
            }
        }
    }
    return objs;
}

//边载荷采用此筛选对象函数（几何线/几何面/单元线）
QVector<std::pair<QVector3D, QVector3D>> AllRenderSymbolManager::getLoadSetEdge(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    QVector<std::pair<QVector3D, QVector3D>> pos_dir;
    std::shared_ptr<MechCaseData::mLoadEdge> load = std::dynamic_pointer_cast<MechCaseData::mLoadEdge>(loadData);
    if (!load) return pos_dir;

    //方向模式转索引：0面内法向 1面外法向 2边切向 3通用
    int dirMode = 0;
    const QString engMode = load->getDirectionModeEng();
    if (engMode == "edtra") dirMode = 1;
    else if (engMode == "edshr") dirMode = 2;
    else if (engMode == "edld") dirMode = 3;

    //通用模式方向（方向控件保存的方向）
    QVector3D generalDir;
    if (load->dir_.isValid() && !load->dir_.isNull())
        generalDir = load->dir_.value<QVector3D>();

    //汇总作用对象
    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> targets;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
    if (setid == -1) return pos_dir;

    auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
    if (set == nullptr) return pos_dir;

    if (set->getInternal())
    {
        targets = dynamic_cast<mMixSet*>(set)->getMixSetData();
    }
    else
    {
        switch (set->getSetType())
        {
        case MxFunctions::GeoEdge:
        case MxFunctions::GeoFace:
        {
            auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
            auto& vec = targets[SetTypeToPickDataType(set->getSetType())];
            vec.reserve(geosetids.size());
            for (int id : geosetids) vec.emplace_back(id, -1, -1);
        }
        break;
        case MxFunctions::ElementEdge:
        {
            auto elementSet = dynamic_cast<mElementSet*>(set);
            if (elementSet) targets[PickDataType::ElementEdge] = elementSet->getElementSetData();
        }
        break;
        default:
            break;
        }
    }

    return calcEdgeLoadPosDir(targets, dirMode, generalDir);
}

//边载荷位置与方向计算（面板临时渲染与符号渲染共用）
QVector<std::pair<QVector3D, QVector3D>> AllRenderSymbolManager::calcEdgeLoadPosDir(
    const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& targets,
    int dirMode, const QVector3D& generalDir)
{
    QVector<std::pair<QVector3D, QVector3D>> pos_dir;

    //通用模式使用方向控件方向，方向未设置时不渲染
    QVector3D unifiedDir;
    if (dirMode == 3)
    {
        if (generalDir.lengthSquared() < 1e-12) return pos_dir;
        unifiedDir = generalDir.normalized();
    }

    auto* geoManager = getCVM->getModelData()->getGeoManager();
    auto* meshData = getCVM->getModelData()->getMeshData();

    for (const auto& pair : targets)
    {
        if (pair.second.empty()) continue;

        switch (pair.first)
        {
        case PickDataType::GeoEdge:
        case PickDataType::GeoFace:
        {
            if (geoManager == nullptr) break;

            std::vector<int> ids;
            ids.reserve(pair.second.size());
            for (const auto& d : pair.second) ids.push_back(d.result_id);

            //几何线按线计算方向；几何面展开为面上所有边
            auto attrs = (pair.first == PickDataType::GeoEdge)
                ? geoManager->CalculateEdgeLoadDirByEdgeIDs(ids)
                : geoManager->CalculateEdgeLoadDirByFaceIDs(ids);
            if (attrs.empty()) break;

            //边中点（质心）作为符号作用位置
            std::vector<int> edgeIds;
            edgeIds.reserve(attrs.size());
            for (const auto& attr : attrs) edgeIds.push_back(attr.edge_id);
            auto edgeAttrs = geoManager->CalculateEdgeAttr(edgeIds);
            std::unordered_map<int, QVector3D> posById;
            for (const auto& attr : edgeAttrs) posById[attr.id] = attr.m_coor;

            for (const auto& attr : attrs)
            {
                auto it = posById.find(attr.edge_id);
                if (it == posById.end()) continue;

                QVector3D dir;
                if (dirMode == 3) dir = unifiedDir;                //通用：方向控件方向
                else if (dirMode == 0) dir = attr.m_inward_dir;    //面内法向
                else if (dirMode == 1) dir = attr.m_outward_dir;   //面外法向
                else dir = attr.m_tangent_dir;                     //边切向

                if (dir.lengthSquared() < 1e-12) continue;
                pos_dir.push_back(std::make_pair(it->second, dir.normalized()));
            }
            break;
        }
        case PickDataType::ElementEdge:
        {
            if (meshData == nullptr) break;

            //单元线方向类型：0边切向 1面内法向 2面外法向
            int dirType = 1;
            if (dirMode == 1) dirType = 2;
            else if (dirMode == 2) dirType = 0;

            //通用模式仅取边中点，方向统一替换为方向控件方向
            auto pointDirs = meshData->getEdgePointAndDirByEdgeNodes(pair.second, dirMode == 3 ? 0 : dirType);
            for (const auto& pd : pointDirs)
            {
                QVector3D dir = (dirMode == 3) ? unifiedDir : pd.second;
                if (dir.lengthSquared() < 1e-12) continue;
                pos_dir.push_back(std::make_pair(pd.first, dir.normalized()));
            }
            break;
        }
        default:
            break;
        }
    }
    return pos_dir;
}

//热流密度采用此筛选对象
QVector<QVector3D> AllRenderSymbolManager::getLoadSetHeatflux(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    QVector<QVector3D> dirs;
    return getLoadSetHeatflux(loadData, dirs);
}
QVector<QVector3D> AllRenderSymbolManager::getLoadSetHeatflux(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs)  //热流密度需区分0.点热流 1.面热流 2.体热流
{
    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);

        std::shared_ptr<MechCaseData::mLoadHeatFlux> load = std::dynamic_pointer_cast<MechCaseData::mLoadHeatFlux>(loadData);
        int index = load->way_;

        if (set->getInternal())
        {
            auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
            //获取集合的作用对象
            for (auto& tar : mixsetdata)
            {
                std::vector<int>ids;
                for (auto& id : tar.second)
                {
                    ids.push_back(id.result_id);
                }

                //点热流 作用在几何点，线，面，体，网格节点
                if (index == 0)
                {
                    switch (tar.first)
                    {
                    case PickDataType::GeoPoint:
                    {
                        auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(ids);
                        for (auto point : points)
                        {
                            objs.push_back(point.m_coor);
                            dirs.push_back(QVector3D());
                        }
                    }
                    break;
                    case PickDataType::GeoEdge:
                    {
                        auto points = getCVM->getModelData()->getGeoManager()->CalculateEdgeAttr(ids);
                        QVector<int>pids;
                        for (auto id : ids)
                            pids.append(id);
                        QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                        for (auto coord : coords)
                        {
                            objs.push_back(coord);
                            dirs.push_back(points[0].m_dir);
                        }
                    }
                    break;
                    case PickDataType::GeoFace:
                    {
                        auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);
                        QVector<int>pids;
                        for (auto id : ids)
                            pids.append(id);
                        QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                        for (auto coord : coords)
                        {
                            objs.push_back(coord);
                            dirs.push_back(points[0].m_dir);
                        }
                    }
                    break;
                    case PickDataType::GeoSolid:
                    {
                        QVector<int>pids;
                        for (auto id : ids)
                            pids.append(id);
                        QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                        for (auto coord : coords)
                        {
                            objs.push_back(coord);
                            dirs.push_back(QVector3D());
                        }
                    }
                    break;
                    case PickDataType::Node:
                    {
                        for (auto id : ids)
                        {
                            objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id));
                            dirs.push_back(QVector3D());
                        }
                    }
                    break;
                    default:
                        break;
                    }
                }
                //面热流 作用在几何面，体，单元面
                else if (index == 1)
                {
                    switch (tar.first)
                    {
                    case PickDataType::GeoFace:
                    {
                        auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);
                        QVector<int>pids;
                        for (auto id : ids)
                            pids.append(id);
                        QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                        for (auto coord : coords)
                        {
                            objs.push_back(coord);
                            dirs.push_back(points[0].m_dir);
                        }
                    }
                    break;
                    case PickDataType::GeoSolid:
                    {
                        QVector<int>pids;
                        for (auto id : ids)
                            pids.append(id);
                        QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                        for (auto coord : coords)
                        {
                            objs.push_back(coord);
                            dirs.push_back(QVector3D());
                        }
                    }
                    break;
                    case PickDataType::ElementFace:
                    {
                        for (auto& id : tar.second)
                        {
                            QVector3D centerPos;
                            QVector<QVector3D> vertex;
                            getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(id, centerPos, vertex);
                            objs.push_back(centerPos);
                        }
                    }
                    break;
                    default:
                        break;
                    }
                }
                //体热流 作用在几何面，体，单元
                else if (index == 2)
                {
                    switch (tar.first)
                    {
                    case PickDataType::GeoFace:
                    {
                        auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);
                        QVector<int>pids;
                        for (auto id : ids)
                            pids.append(id);
                        QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                        for (auto coord : coords)
                        {
                            objs.push_back(coord);
                            dirs.push_back(points[0].m_dir);
                        }
                    }
                    break;
                    case PickDataType::GeoSolid:
                    {
                        QVector<int>pids;
                        for (auto id : ids)
                            pids.append(id);
                        QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                        for (auto coord : coords)
                        {
                            objs.push_back(coord);
                            dirs.push_back(QVector3D());
                        }
                    }
                    break;
                    case PickDataType::Element:
                    {
                        for (auto& id : tar.second)
                        {
                            QVector3D centerPos;
                            QVector<QVector3D> vertex;
                            //getCVM->getModelData()->getMeshData()->GetOutVectorByMeshID(id.result_id, vertex, centerPos);
                            getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshID(id.result_id, centerPos, vertex, 5);
                            objs.push_back(centerPos);
                            //dirs.push_back(vertex);
                        }
                    }
                    break;
                    default:
                        break;
                    }

                }
            }
        }
        else
        {
            MxFunctions::SetType settype = set->getSetType();
            //点热流 作用在几何点，线，面，体，网格节点
            if (index == 0)
            {
                switch (settype)
                {
                case MxFunctions::GeoPoint:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(geosetids);
                    for (auto point : points)
                    {
                        objs.push_back(point.m_coor);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case MxFunctions::GeoEdge:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateEdgeAttr(geosetids);
                    QVector<int>pids;
                    for (auto id : geosetids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case MxFunctions::GeoFace:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);
                    QVector<int>pids;
                    for (auto id : geosetids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case MxFunctions::GeoSolid:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    QVector<int>pids;
                    for (auto id : geosetids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case MxFunctions::Node:
                {
                    auto nodesetids = dynamic_cast<mNode*>(set)->getNodeData();
                    for (auto point : nodesetids)
                    {
                        objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(point.result_id));
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                default:
                    break;
                }
            }
            //面热流 作用在几何面，体，单元面
            else if (index == 1)
            {
                switch (settype)
                {
                case MxFunctions::GeoFace:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);
                    QVector<int>pids;
                    for (auto id : geosetids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case MxFunctions::GeoSolid:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    QVector<int>pids;
                    for (auto id : geosetids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case MxFunctions::ElementSurface:
                {
                    auto elesurfaceset = dynamic_cast<MechSetData::mElementSurface*>(set);
                    if (elesurfaceset != nullptr && !elesurfaceset->getElementSurfaceData().empty())
                    {
                        auto surfaceids = elesurfaceset->getElementSurfaceData();
                        for (auto id : surfaceids)
                        {
                            QVector3D  centerPos;
                            QVector<QVector3D> vertexs;
                            getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(id, centerPos, vertexs);
                            objs.push_back(centerPos);
                        }
                    }
                }
                break;
                default:
                    break;
                }
            }
            //体热流 作用在几何面，体，单元
            else if (index == 2)
            {
                switch (settype)
                {
                case MxFunctions::GeoFace:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);
                    QVector<int>pids;
                    for (auto id : geosetids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case MxFunctions::GeoSolid:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    QVector<int>pids;
                    for (auto id : geosetids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case MxFunctions::Element:
                {
                    auto eleset = dynamic_cast<MechSetData::mElementSet*>(set);
                    if (eleset != nullptr && eleset->getElementSetData().empty())
                    {
                        auto surfaceids = eleset->getElementSetData();
                        std::vector<int>ids;
                        for (auto& id : surfaceids)
                        {
                            QVector3D  centerPos;
                            QVector<QVector3D> vertexs;
                            getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshID(id.result_id, centerPos, vertexs);
                            objs.push_back(centerPos);
                        }
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
    }
    return objs;
}

//对流换热采用此筛选对象函数
QVector<QVector3D> AllRenderSymbolManager::getLoadSetConvection(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    QVector<QVector3D> dirs;
    return getLoadSetConvection(loadData, dirs);
}
QVector<QVector3D> AllRenderSymbolManager::getLoadSetConvection(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs)
{

    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
        if (set->getInternal())
        {
            auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
            //获取集合的作用对象
            for (auto& tar : mixsetdata)
            {
                std::vector<int>ids;
                for (auto& id : tar.second)
                {
                    ids.push_back(id.result_id);
                }
                switch (tar.first)
                {
                case PickDataType::GeoFace:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case PickDataType::GeoSolid:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case PickDataType::ElementFace:
                {
                    for (auto& id : tar.second)
                    {
                        QVector3D centerPos;
                        QVector<QVector3D> vertex;
                        getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(id, centerPos, vertex);
                        objs.push_back(centerPos);
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else
        {
            MxFunctions::SetType settype = set->getSetType();
            switch (settype)
            {
            case MxFunctions::GeoFace:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                    dirs.push_back(points[0].m_dir);
                }
            }
            break;
            case MxFunctions::GeoSolid:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                    dirs.push_back(QVector3D());
                }
            }
            break;
            case MxFunctions::ElementSurface:
            {
                auto elesurfaceset = dynamic_cast<MechSetData::mElementSurface*>(set);
                if (elesurfaceset != nullptr && !elesurfaceset->getElementSurfaceData().empty())
                {
                    auto surfaceids = elesurfaceset->getElementSurfaceData();
                    for (auto id : surfaceids)
                    {
                        QVector3D  centerPos;
                        QVector<QVector3D> vertexs;
                        getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(id, centerPos, vertexs);
                        objs.push_back(centerPos);
                    }
                }
            }
            break;
            default:
                break;
            }
        }
    }
    return objs;
}

//热辐射采用此筛选对象
QVector<QVector3D> AllRenderSymbolManager::getLoadSetRadiation(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    QVector<QVector3D> dirs;
    return getLoadSetRadiation(loadData, dirs);
}
QVector<QVector3D> AllRenderSymbolManager::getLoadSetRadiation(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs)
{

    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
        if (set->getInternal())
        {
            auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
            //获取集合的作用对象
            for (auto& tar : mixsetdata)
            {
                std::vector<int>ids;
                for (auto& id : tar.second)
                {
                    ids.push_back(id.result_id);
                }
                switch (tar.first)
                {
                case PickDataType::GeoFace:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(points[0].m_dir);
                    }
                }
                break;
                case PickDataType::GeoSolid:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case PickDataType::ElementFace:
                {
                    for (auto& id : tar.second)
                    {
                        QVector3D centerPos;
                        QVector<QVector3D> vertex;
                        getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(id, centerPos, vertex);
                        objs.push_back(centerPos);
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else
        {
            MxFunctions::SetType settype = set->getSetType();
            switch (settype)
            {
            case MxFunctions::GeoFace:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                    dirs.push_back(points[0].m_dir);
                }
            }
            break;
            case MxFunctions::GeoSolid:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                    dirs.push_back(QVector3D());
                }
            }
            break;
            case MxFunctions::ElementSurface:
            {
                auto elesurfaceset = dynamic_cast<MechSetData::mElementSurface*>(set);
                if (elesurfaceset != nullptr && !elesurfaceset->getElementSurfaceData().empty())
                {
                    auto surfaceids = elesurfaceset->getElementSurfaceData();
                    for (auto id : surfaceids)
                    {
                        QVector3D  centerPos;
                        QVector<QVector3D> vertexs;
                        getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(id, centerPos, vertexs);
                        objs.push_back(centerPos);
                    }
                }
            }
            break;
            default:
                break;
            }
        }
    }
    return objs;
}

//声学载荷采用此筛选对象
QVector<QVector3D> AllRenderSymbolManager::getLoadSetAcoconcforce(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
{
    QVector<QVector3D> dirs;
    return getLoadSetAcoconcforce(loadData, dirs);
}
QVector<QVector3D> AllRenderSymbolManager::getLoadSetAcoconcforce(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs)
{

    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
        if (set->getInternal())
        {
            auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
            //获取集合的作用对象
            for (auto& tar : mixsetdata)
            {
                std::vector<int>ids;
                for (auto& id : tar.second)
                {
                    ids.push_back(id.result_id);
                }
                switch (tar.first)
                {
                case PickDataType::GeoPoint:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(ids);
                    for (auto point : points)
                    {
                        objs.push_back(point.m_coor);
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                case PickDataType::Node:
                {
                    for (auto id : ids)
                    {
                        objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id));
                        dirs.push_back(QVector3D());
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else
        {
            MxFunctions::SetType settype = set->getSetType();
            switch (settype)
            {
            case MxFunctions::GeoPoint:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(geosetids);
                for (auto point : points)
                {
                    objs.push_back(point.m_coor);
                    dirs.push_back(QVector3D());
                }
            }
            break;
            case MxFunctions::Node:
            {
                auto nodesetids = dynamic_cast<mNode*>(set)->getNodeData();
                for (auto point : nodesetids)
                {
                    objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(point.result_id));
                    dirs.push_back(QVector3D());
                }
            }
            break;
            default:
                break;
            }
        }
    }
    return objs;
}
//预定义场筛选作用对象
QVector<QVector3D> AllRenderSymbolManager::getPredefinedFieldSet(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData)
{
    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_PredefinedField, fieldData->getPredefinedFieldId() }, Physical_Set);
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
        if (set->getInternal())
        {
            auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
            //获取集合的作用对象
            for (auto& tar : mixsetdata)
            {
                std::vector<int>ids;
                for (auto& id : tar.second)
                {
                    ids.push_back(id.result_id);
                }
                switch (tar.first)
                {
                case PickDataType::GeoPoint:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(ids);
                    for (auto point : points)
                    {
                        objs.push_back(point.m_coor);
                    }
                }
                break;
                case PickDataType::GeoEdge:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                    }
                }
                break;
                case PickDataType::GeoFace:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                    }
                }
                break;
                case PickDataType::GeoSolid:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                    }
                }
                break;
                case PickDataType::Node:
                {
                    for (auto id : ids)
                    {
                        objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id));
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else
        {
            MxFunctions::SetType settype = set->getSetType();
            switch (settype)
            {
            case MxFunctions::GeoPoint:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(geosetids);
                for (auto point : points)
                {
                    objs.push_back(point.m_coor);
                }
            }
            break;
            case MxFunctions::GeoEdge:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                }
            }
            break;
            case MxFunctions::GeoFace:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                }
            }
            break;
            case MxFunctions::GeoSolid:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                }
            }
            break;
            case MxFunctions::Node:
            {
                auto nodesetids = dynamic_cast<mNode*>(set)->getNodeData();
                for (auto point : nodesetids)
                {
                    objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(point.result_id));
                }
            }
            break;
            default:
                break;
            }
        }
    }
    return objs;
}

//边界筛选作用对象
QVector<QVector3D> AllRenderSymbolManager::getBoundarySet(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
{
    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_LoadBoundary,boundaryData->getLoadBoundaryId() }, Physical_Set);
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
        if (set->getInternal())
        {
            auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
            //获取集合的作用对象
            for (auto& tar : mixsetdata)
            {
                std::vector<int>ids;
                for (auto& id : tar.second)
                {
                    ids.push_back(id.result_id);
                }
                switch (tar.first)
                {
                case PickDataType::GeoPoint:
                {
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(ids);
                    for (auto point : points)
                    {
                        objs.push_back(point.m_coor);
                    }
                }
                break;
                case PickDataType::GeoEdge:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                    }
                }
                break;
                case PickDataType::GeoFace:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                    }
                }
                break;
                case PickDataType::GeoSolid:
                {
                    QVector<int>pids;
                    for (auto id : ids)
                        pids.append(id);
                    QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                    for (auto coord : coords)
                    {
                        objs.push_back(coord);
                    }
                }
                break;
                case PickDataType::Node:
                {
                    for (auto id : ids)
                    {
                        objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id));
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else
        {
            MxFunctions::SetType settype = set->getSetType();
            switch (settype)
            {
            case MxFunctions::GeoPoint:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr(geosetids);
                for (auto point : points)
                {
                    objs.push_back(point.m_coor);
                }
            }
            break;
            case MxFunctions::GeoEdge:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                }
            }
            break;
            case MxFunctions::GeoFace:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                }
            }
            break;
            case MxFunctions::GeoSolid:
            {
                auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                QVector<int>pids;
                for (auto id : geosetids)
                    pids.append(id);
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto coord : coords)
                {
                    objs.push_back(coord);
                }
            }
            break;
            case MxFunctions::Node:
            {
                auto nodesetids = dynamic_cast<mNode*>(set)->getNodeData();
                for (auto point : nodesetids)
                {
                    objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(point.result_id));
                }
            }
            break;
            default:
                break;
            }
        }
    }
    return objs;
}

//位移边界独立赋值获取作用对象
QVector<QVector3D> AllRenderSymbolManager::getIndependentBoundarySetDisplacement(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
{
    QVector<QVector3D> objs;
    std::shared_ptr<MechCaseData::mBoundaryDisplacement> boundary = std::dynamic_pointer_cast<MechCaseData::mBoundaryDisplacement>(boundaryData);
    if (!boundary) return objs;
    for (const auto& entry : boundary->_independentTable)
    {
        int nodeId = entry.first.result_id;
        objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(nodeId));
    }
    return objs;
}

//螺栓约束作用对象
QVector<QVector3D> AllRenderSymbolManager::getConnectorBolt(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData, int type_)
{
    QVector<QVector3D> dirs;
    return getConnectorBolt(connectorData, type_, dirs);
}
QVector<QVector3D> AllRenderSymbolManager::getConnectorBolt(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData, int type_, QVector<QVector3D>& dirs)
{
    QVector<QVector3D> objs;
    auto& maprelation = getCVM->getModelData()->getMapRelationManager();
    int setid = maprelation.getMapRelation<int>({ Physical_Connector,connectorData->getConnectorId() }, Physical_Set);
    if (getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setid) == connectorData->getConnectorName())return objs;
    if (setid == -1)
    {
        return objs;
    }
    else
    {
        if (type_ == 0) //一维作用与几何线和单元线
        {
            auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
            if (set->getInternal())
            {
                auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
                //获取集合的作用对象
                for (auto& tar : mixsetdata)
                {
                    std::vector<int>ids;
                    for (auto& id : tar.second)
                    {
                        ids.push_back(id.result_id);
                    }
                    switch (tar.first)
                    {
                    case PickDataType::GeoEdge:
                    {
                        for (auto id : ids)
                        {
                            auto set = getCVM->getModelData()->getGeoData()->getPartSetByGeoLineID(id);
//MIG                             TopoDS_Edge edge = set->getEdgeByTag(id);

//MIG                             TopTools_IndexedMapOfShape vertexMap;
//MIG                             TopExp::MapShapes(edge, TopAbs_VERTEX, vertexMap);
//MIG                             QVector<gp_Pnt> points;
//MIG                             for (Standard_Integer i = 1; i <= vertexMap.Extent(); ++i)
                            {
//MIG                                 TopoDS_Vertex vertex = TopoDS::Vertex(vertexMap.FindKey(i));
//MIG                                 gp_Pnt point = BRep_Tool::Pnt(vertex);
//MIG                                 points.push_back(point);
                            }

//MIG                             if (points.size() >= 2)
                            {
//MIG                                 QVector3D point1 = { (float)points[0].X() ,(float)points[0].Y(), (float)points[0].Z() };
//MIG                                 QVector3D point2 = { (float)points[1].X() ,(float)points[1].Y(), (float)points[1].Z() };
//MIG                                 objs.push_back(point1);
//MIG                                 dirs.push_back(point2);
                            }
                        }
                    }
                    break;
                    case PickDataType::GeoFace:
                    {
                        auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);

                        int i = 0;
                        for (auto id : ids)
                        {
//MIG                             auto set = getCVM->getModelData()->getGeoData()->getPartSetByGeoFaceID(id);
                            if (nullptr == set) continue;

                            QVector<QVector3D> coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, { id });
                            QVector3D normal = points[i++].m_dir;
                            for (auto coord : coords)
                            {
                                objs.push_back(coord);
                                dirs.push_back(coord + normal);
                            }
                        }
                    }
                    break;
                    case PickDataType::Element:
                    {
                        auto meshData = getCVM->getModelData()->getMeshData();
                        for (auto& id : tar.second)
                        {
                            if (!meshData || meshData->getDim(id) != 1)
                                continue;

                            std::vector<RenderPickResultData> nodes = meshData->getNodeIdsByEleIds({ id });
                            if (nodes.size() < 2)
                                continue;

                            QVector3D p0 = meshData->getVectorById(nodes[0].result_id);
                            QVector3D p1 = meshData->getVectorById(nodes[1].result_id);
                            objs.push_back(p0);
                            dirs.push_back(p1);
                        }
                    }
                    break;
                    case PickDataType::ElementEdge:
                    {
                        QVector<std::pair<int, int>> ids;
                        for (auto& id : tar.second)
                        {
                            ids.push_back(std::make_pair(id.result_id, id.result_id2));
                        }

                        for (auto id : ids)
                        {
                            objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id.first));
                            dirs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id.second));
                        }
                    }
                    break;
                    default:
                        break;
                    }
                }
            }
            else
            {
                MxFunctions::SetType settype = set->getSetType();
                switch (settype)
                {
                case MxFunctions::GeoEdge:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    for (auto id : geosetids)
                    {
                        auto set = getCVM->getModelData()->getGeoData()->getPartSetByGeoLineID(id);
//MIG                         TopoDS_Edge edge = set->getEdgeByTag(id);

//MIG                         TopTools_IndexedMapOfShape vertexMap;
//MIG                         TopExp::MapShapes(edge, TopAbs_VERTEX, vertexMap);
//MIG                         QVector<gp_Pnt> points;
//MIG                         for (Standard_Integer i = 1; i <= vertexMap.Extent(); ++i)
                        {
//MIG                             TopoDS_Vertex vertex = TopoDS::Vertex(vertexMap.FindKey(i));
//MIG                             gp_Pnt point = BRep_Tool::Pnt(vertex);
//MIG                             points.push_back(point);
                        }

//MIG                         if (points.size() >= 2)
                        {
//MIG                             QVector3D point1 = { (float)points[0].X() ,(float)points[0].Y(), (float)points[0].Z() };
//MIG                             QVector3D point2 = { (float)points[1].X() ,(float)points[1].Y(), (float)points[1].Z() };
//MIG                             objs.push_back(point1);
//MIG                             dirs.push_back(point2);
                        }
                    }
                }
                break;
                case MxFunctions::GeoFace:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);

                    int i = 0;
                    for (auto id : geosetids)
                    {
//MIG                         auto set = getCVM->getModelData()->getGeoData()->getPartSetByGeoFaceID(id);
                        if (nullptr == set) continue;

                        QVector<QVector3D> coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, { id });
                        QVector3D normal = points[i++].m_dir;
                        for (auto coord : coords)
                        {
                            objs.push_back(coord);
                            dirs.push_back(coord + normal);
                        }
                    }
                }
                break;
                case MxFunctions::Element:
                {
                    auto eleset = dynamic_cast<MechSetData::mElementSet*>(set);
                    if (eleset != nullptr)
                    {
                        auto meshData = getCVM->getModelData()->getMeshData();
                        auto eleIds = eleset->getElementSetData();
                        for (auto& id : eleIds)
                        {
                            if (!meshData || meshData->getDim(id) != 1)
                                continue;

                            std::vector<RenderPickResultData> nodes = meshData->getNodeIdsByEleIds({ id });
                            if (nodes.size() < 2)
                                continue;

                            QVector3D p0 = meshData->getVectorById(nodes[0].result_id);
                            QVector3D p1 = meshData->getVectorById(nodes[1].result_id);
                            objs.push_back(p0);
                            dirs.push_back(p1);
                        }
                    }
                }
                break;
                case MxFunctions::ElementEdge:
                {
                    //单元边和单元面同一个数据结构
                    auto elesurfaceset = dynamic_cast<MechSetData::mElementSurface*>(set);
                    if (elesurfaceset != nullptr)
                    {
                        QVector<std::pair<int, int>> ids;

                        auto surfaceids = elesurfaceset->getElementSurfaceData();
                        for (auto id : surfaceids)
                        {
                            ids.push_back(std::make_pair(id.result_id, id.result_id2));
                        }

                        for (auto id : ids)
                        {
                            objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id.first));
                            dirs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(id.second));
                        }
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        else if (type_ == 1) //三维作用于几何面和单元面
        {
            auto set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
            if (set->getInternal())
            {
                auto mixsetdata = dynamic_cast<mMixSet*>(set)->getMixSetData();
                //获取集合的作用对象
                for (auto& tar : mixsetdata)
                {
                    std::vector<int>ids;
                    for (auto& id : tar.second)
                    {
                        ids.push_back(id.result_id);
                    }
                    switch (tar.first)
                    {
                    case PickDataType::GeoFace:
                    {
                        auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(ids);

                        int i = 0;
                        for (auto id : ids)
                        {
//MIG                             auto set = getCVM->getModelData()->getGeoData()->getPartSetByGeoFaceID(id);
                            if (nullptr == set) objs;
//MIG                             TopoDS_Face face = set->getFaceByTag(id);

//MIG                             GProp_GProps props;
                            // 计算面的几何属性（面积和质心）
//MIG                             BRepGProp::SurfaceProperties(face, props);

                            // 获取质心（形心）坐标
//MIG                             gp_Pnt centerOfMass = props.CentreOfMass();

//MIG                             objs.push_back({ (float)centerOfMass.X() ,(float)centerOfMass.Y(),(float)centerOfMass.Z() });
                            dirs.push_back(points[i++].m_dir);
                        }
                    }
                    break;
                    case PickDataType::ElementFace:
                    {
                        for (auto& id : tar.second)
                        {
                            QVector3D centerPos;
                            QVector3D vertex;
                            getCVM->getModelData()->getMeshData()->GetOutVectorByMeshFaceID(id, vertex, centerPos);
                            objs.push_back(centerPos);
                            dirs.push_back(vertex);
                        }
                    }
                    break;
                    default:
                        break;
                    }
                }
            }
            else
            {
                MxFunctions::SetType settype = set->getSetType();
                switch (settype)
                {
                case MxFunctions::GeoFace:
                {
                    auto geosetids = dynamic_cast<mGeoSet*>(set)->getGeoSetData();
                    auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr(geosetids);
                    QVector<int>pids;
                    int i = 0;
                    for (auto id : geosetids)
                    {
//MIG                         auto set = getCVM->getModelData()->getGeoData()->getPartSetByGeoFaceID(id);
                        if (nullptr == set) objs;
//MIG                         TopoDS_Face face = set->getFaceByTag(id);

//MIG                         GProp_GProps props;
                        // 计算面的几何属性（面积和质心）
//MIG                         BRepGProp::SurfaceProperties(face, props);

                        // 获取质心（形心）坐标
//MIG                         gp_Pnt centerOfMass = props.CentreOfMass();

//MIG                         objs.push_back({ (float)centerOfMass.X() ,(float)centerOfMass.Y(),(float)centerOfMass.Z() });
                        dirs.push_back(points[i++].m_dir);
                    }
                }
                break;
                case MxFunctions::ElementSurface:
                {
                    auto elesurfaceset = dynamic_cast<MechSetData::mElementSurface*>(set);
                    if (elesurfaceset != nullptr)
                    {
                        auto surfaceids = elesurfaceset->getElementSurfaceData();
                        for (auto id : surfaceids)
                        {
                            QVector3D  centerPos;
                            QVector3D   vertexs;
                            getCVM->getModelData()->getMeshData()->GetOutVectorByMeshFaceID(id, vertexs, centerPos);
                            objs.push_back(centerPos);
                            dirs.push_back(vertexs);
                        }
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
    }
    return objs;
}

void AllRenderSymbolManager::assignLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, const QVector<std::pair<QVector3D, QVector3D>>& pos_dir, const QString& txt, const QVector3D& color)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("load_%1").arg(loadData->getLoadBoundaryId());
    auto loadtype = loadData->getLoadBoundaryType();

    // 作用对象为空（面板刚创建、对象未选或文件模式）时不创建符号，并清除旧符号，
    // 避免注册空符号干扰 showLoad 的 isExistSymbol 判断
    if (pos_dir.isEmpty())
    {
        symbolController->deleteSymbol({ newsymbol });
        return;
    }

    switch (loadtype)
    {
    case MxFunctions::LOAD_IM_FORCE:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Arrow_Font_Load_Implicit>(newsymbol);
        arrow->_pos_dir = pos_dir;
        arrow->_txt = "";
        arrow->_color = color;
        arrow->_depthState = false;
        arrow->_isFixedPixel = false;
        arrow->_color = QVector3D(133, 19, 33);
        symbolController->createSymbol(arrow);
    }
    break;
    case MxFunctions::LOAD_IM_TORQUE:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Moment>(newsymbol);
        arrow->_pos_normal = pos_dir;
        arrow->_isFixedPixel = false;
        arrow->_depthState = false;
        QVector<bool> t(pos_dir.size(), false);
        arrow->_isClockwise = t;
        arrow->_color = QVector3D(133, 19, 33);
        symbolController->createSymbol(arrow);
    }
    break;
    case MxFunctions::LOAD_IM_PRESSURE:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Arrow_Font_Load1_Implicit>(newsymbol);
        arrow->_pos_dir = pos_dir;
        arrow->_depthState = false;
        arrow->_isFixedPixel = false;
        arrow->_txt = "";
        arrow->_color = color;
        arrow->_color = QVector3D(121, 93, 1);
        symbolController->createSymbol(arrow);
    }
    break;
    case MxFunctions::LOAD_IM_SURFACE:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Arrow_Font_Load1_Implicit>(newsymbol);
        arrow->_pos_dir = pos_dir;
        arrow->_depthState = false;
        arrow->_isFixedPixel = false;
        arrow->_txt = "";
        arrow->_color = color;
        symbolController->createSymbol(arrow);
    }
    break;
    case MxFunctions::LOAD_IM_EDGE:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Arrow_Font_Load_Implicit>(newsymbol);
        arrow->_pos_dir = pos_dir;
        arrow->_depthState = false;
        arrow->_isFixedPixel = false;
        arrow->_txt = "";
        arrow->_color = color;
        symbolController->createSymbol(arrow);
    }
    break;
    default:
        break;
    }
}

void AllRenderSymbolManager::assignThermalLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData,
    const QVector<QVector3D>& pos_dir, const QString& txt, const QVector3D& color, int index)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("load_%1").arg(loadData->getLoadBoundaryId());
    auto loadtype = loadData->getLoadBoundaryType();

    // 作用对象为空（面板刚创建、对象未选）时不创建符号，并清除旧符号，
    // 避免注册空符号干扰 showLoad 的 isExistSymbol 判断
    if (pos_dir.isEmpty())
    {
        symbolController->deleteSymbol({ newsymbol });
        return;
    }

    switch (loadtype)
    {
    case MxFunctions::LOAD_IM_HEATFLUX:
    {
        if (index == 0 || index == 1) //点热源，面热源 三角 HF 颜色255 255 0
        {
            //删除逻辑（清理可能存在的旧符号，避免类型切换时残留）
            std::vector<QString> names;
            names.push_back(newsymbol);
            symbolController->deleteSymbol(names);

            const auto& triangle = symbolController->getOrCreateSymbolData<Triangle_Font_Load_Implicit>(newsymbol);
            triangle->_pos = pos_dir;
            triangle->_txt = "HF";
            triangle->_isFixedPixel = false;
            triangle->_depthState = false;
            triangle->_color = color;
            symbolController->createSymbol(triangle);
        }
        else if (index == 2) //体热源 十字 IH 颜色255 255 0
        {
            //删除逻辑（清理可能存在的旧符号，避免类型切换时残留）
            std::vector<QString> names;
            names.push_back(newsymbol);
            symbolController->deleteSymbol(names);

            const auto& x = symbolController->getOrCreateSymbolData<X_Font_Load_Implicit>(newsymbol);
            x->_pos = pos_dir;
            x->_txt = "IH";
            x->_isFixedPixel = false;
            x->_depthState = false;
            x->_color = color;
            symbolController->createSymbol(x);
        }
    }
    break;
    case MxFunctions::LOAD_IM_RADIATION:
    {
        const auto& rhombus = symbolController->getOrCreateSymbolData<Rhombus_Font_Load_Implicit>(newsymbol);
        rhombus->_pos = pos_dir;
        rhombus->_txt = "CH";
        rhombus->_isFixedPixel = false;
        rhombus->_depthState = false;
        rhombus->_color = color;
        symbolController->createSymbol(rhombus);
    }
    break;
    case MxFunctions::LOAD_IM_CONVECTION:
    {
        const auto& rhombus = symbolController->getOrCreateSymbolData<Rhombus_Font_Load_Implicit>(newsymbol);
        rhombus->_pos = pos_dir;
        rhombus->_txt = "CH";
        rhombus->_isFixedPixel = false;
        rhombus->_depthState = false;
        rhombus->_color = color;
        symbolController->createSymbol(rhombus);
    }
    break;
    case MxFunctions::LOAD_IM_ACOCONCFORCE:
    {
        const auto& home = symbolController->getOrCreateSymbolData<Home_BC_Implicit>(newsymbol);
        home->_pos = pos_dir;
        home->_txt = "";
        home->_isFixedPixel = false;
        home->_depthState = false;
        home->_color = color;
        symbolController->createSymbol(home);
    }
    break;
    default:
        break;
    }
}

void AllRenderSymbolManager::assignBoundary(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData,
    const QVector<QVector3D>& pos, const QMap<uint32_t, float>& value, const QVector3D& color)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("boundary_%1").arg(boundaryData->getLoadBoundaryId());

    // 作用对象为空（面板刚创建、对象未选）时不创建符号，并清除旧符号，
    // 避免注册空符号干扰 showBoundary 的 isExistSymbol 判断
    if (pos.isEmpty())
    {
        symbolController->deleteSymbol({ newsymbol });
        return;
    }

    const auto& arrow = symbolController->getOrCreateSymbolData<DF_BC_Implicit>(newsymbol);
    arrow->_pos = pos;
    arrow->_value = value;
    arrow->_color = color;
    arrow->_depthState = false;
    arrow->_isFixedPixel = false;
    symbolController->createSymbol(arrow);
    symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { newsymbol }, 0.05);
}

void AllRenderSymbolManager::assignBoundary(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData,
    const QVector<QVector3D>& pos, const QVector3D& color)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("boundary_%1").arg(boundaryData->getLoadBoundaryId());

    // 作用对象为空（面板刚创建、对象未选）时不创建符号，并清除旧符号，
    // 避免注册空符号干扰 showBoundary 的 isExistSymbol 判断
    if (pos.isEmpty())
    {
        symbolController->deleteSymbol({ newsymbol });
        return;
    }

    const auto& arrow = symbolController->getOrCreateSymbolData<Home_BC_Implicit>(newsymbol);
    arrow->_pos = pos;
    arrow->_color = color;
    arrow->_depthState = false;
    arrow->_isFixedPixel = false;
    symbolController->createSymbol(arrow);
    symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { newsymbol }, 0.05);
}

void AllRenderSymbolManager::assignBoundaryDisplacement(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData,
    const QVector<QVector3D>& pos, const QMap<uint32_t, float>& value, const QVector3D& color)
{
    if (boundaryData == nullptr || pos.isEmpty())
        return;
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    if (!symbolController)
        return;

    // 每个坐标轴对应一对分量：U1/UR1->X、U2/UR2->Y、U3/UR3->Z，方向受数值正负影响（负值反向）。
    // 同轴 U 与 UR 同时存在时：方向一致（正负同号）只渲染一个箭头，方向相反时两个箭头都渲染
    auto axisDir = [](uint32_t key) -> QVector3D {
        switch (key)
        {
        case 1: case 4: return QVector3D(1, 0, 0);
        case 2: case 5: return QVector3D(0, 1, 0);
        case 3: case 6: return QVector3D(0, 0, 1);
        default: return QVector3D();
        }
        };

    QVector<std::pair<QVector3D, QVector3D>> pos_dir;
    for (uint32_t axisKey = 1; axisKey <= 3; ++axisKey)
    {
        QVector3D axis = axisDir(axisKey);
        if (axis.isNull())
            continue;

        bool hasU = value.contains(axisKey);
        bool hasUR = value.contains(axisKey + 3);
        if (!hasU && !hasUR)
            continue;

        QVector3D uDir, urDir;
        if (hasU)
        {
            uDir = axis;
            if (value.value(axisKey) < 0)
                uDir = -uDir;
        }
        if (hasUR)
        {
            urDir = axis;
            if (value.value(axisKey + 3) < 0)
                urDir = -urDir;
        }

        QVector<QVector3D> dirs;
        if (hasU)
            dirs.push_back(uDir);
        if (hasUR && (!hasU || urDir != uDir))
            dirs.push_back(urDir);

        for (const auto& d : dirs)
        {
            for (const auto& p : pos)
                pos_dir.push_back(std::make_pair(p, d));
        }
    }
    if (pos_dir.isEmpty())
        return;

    // 与集中力（LOAD_IM_FORCE）相同的箭头符号，通过颜色区分
    QString newsymbol = QString("boundary_%1").arg(boundaryData->getLoadBoundaryId());
    const auto& arrow = symbolController->getOrCreateSymbolData<Arrow_Font_Load_Implicit>(newsymbol);
    arrow->_pos_dir = pos_dir;
    arrow->_txt = "";
    arrow->_depthState = false;
    arrow->_isFixedPixel = false;
    arrow->_color = color;
    symbolController->createSymbol(arrow);
    symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { newsymbol }, 0.05);
}

std::vector<QString> AllRenderSymbolManager::getBoundarySymbolNames(int boundaryId)
{
    std::vector<QString> names;
    QString base = QString("boundary_%1").arg(boundaryId);
    names.push_back(base);
    for (int key = 1; key <= 6; ++key)
    {
        names.push_back(QString("%1_u%2").arg(base).arg(key));
    }
    return names;
}

void AllRenderSymbolManager::assignBoundaryIndependentDisplacement(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData,
    const std::map<RenderPickResultData, QVector<QVariant>>& independentTable, const QVector3D& color)
{
    if (boundaryData == nullptr)
        return;

    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    if (!symbolController)
        return;

    int boundaryId = boundaryData->getLoadBoundaryId();
    auto meshData = getCVM->getModelData()->getMeshData();

    // 先清除本边界所有可能 symbol，避免旧符号残留
    symbolController->deleteSymbol(getBoundarySymbolNames(boundaryId));

    // 按节点逐轴生成箭头：value 列 0..5 对应 U1..UR3，轴 key 使用 1..3（X/Y/Z）
    // 与集中力符号一致，方向随数值正负翻转；
    // 同轴 U 与 UR 方向一致（正负同号）只渲染一个箭头，方向相反时两个箭头都渲染
    auto axisDir = [](int axisKey) -> QVector3D {
        switch (axisKey)
        {
        case 1: return QVector3D(1, 0, 0);
        case 2: return QVector3D(0, 1, 0);
        case 3: return QVector3D(0, 0, 1);
        default: return QVector3D();
        }
        };
    QMap<int, QVector<std::pair<QVector3D, QVector3D>>> axisPosDir;
    for (const auto& entry : independentTable)
    {
        int nodeId = entry.first.result_id;
        QVector3D pos = meshData->getVectorById(nodeId);
        if (pos.isNull())
            continue;

        const QVector<QVariant>& values = entry.second;
        auto validAt = [&values](int idx) -> bool {
            return idx < values.size() && values[idx].isValid() && !values[idx].isNull();
            };

        for (int axisIdx = 0; axisIdx < 3; ++axisIdx)
        {
            QVector3D axis = axisDir(axisIdx + 1);
            bool hasU = validAt(axisIdx);
            bool hasUR = validAt(axisIdx + 3);
            if (!hasU && !hasUR)
                continue;

            QVector3D uDir, urDir;
            if (hasU)
            {
                uDir = axis;
                if (values[axisIdx].toFloat() < 0)
                    uDir = -uDir;
            }
            if (hasUR)
            {
                urDir = axis;
                if (values[axisIdx + 3].toFloat() < 0)
                    urDir = -urDir;
            }

            if (hasU)
                axisPosDir[axisIdx + 1].push_back({ pos, uDir });
            if (hasUR && (!hasU || urDir != uDir))
                axisPosDir[axisIdx + 1].push_back({ pos, urDir });
        }
    }

    // 每个轴单独创建一个箭头 symbol：boundary_<id>_u<axis>
    for (auto it = axisPosDir.constBegin(); it != axisPosDir.constEnd(); ++it)
    {
        const QVector<std::pair<QVector3D, QVector3D>>& pos_dir = it.value();
        if (pos_dir.isEmpty())
            continue;

        QString symbolName = QString("boundary_%1_u%2").arg(boundaryId).arg(it.key());
        const auto& arrow = symbolController->getOrCreateSymbolData<Arrow_Font_Load_Implicit>(symbolName);
        arrow->_pos_dir = pos_dir;
        arrow->_txt = "";
        arrow->_depthState = false;
        arrow->_isFixedPixel = false;
        arrow->_color = color;
        symbolController->createSymbol(arrow);
        symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { symbolName }, 0.05);
    }
}


bool AllRenderSymbolManager::assignPredefinedField(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData,
    const QVector<QVector3D>& pos, const QString& txt, const QVector3D& color)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("predefinedfield_%1").arg(fieldData->getPredefinedFieldId());
    auto type = fieldData->getPredefinedFieldType();

    // 作用对象为空（面板刚创建、文件定义模式）时不创建符号，并清除旧符号，
    // 避免注册空符号干扰 showPredefinedField 的 isExistSymbol 判断
    if (pos.isEmpty())
    {
        const bool existed = symbolController->isExistSymbol(newsymbol);
        symbolController->deleteSymbol({ newsymbol });
        return existed;
    }

    switch (type)
    {
    case MxFunctions::PREDEFINED_FIELD_INIT_TEMP:
    {
        const auto& home = symbolController->getOrCreateSymbolData<Home_BC_Implicit>(newsymbol);
        // 数据未变化时跳过重写：面板进入/防抖刷新会以相同数据反复调用，
        // 返回 false 供调用方跳过 UpdateAllSymbol 的全量符号重建（大模型下代价很高）
        if (home->_pos == pos && home->_txt == txt && home->_color == color)
            return false;
        home->_pos = pos;
        home->_txt = txt;
        home->_isFixedPixel = false;
        home->_depthState = false;
        home->_color = color;
        symbolController->createSymbol(home);
    }
    break;
    case MxFunctions::PREDEFINED_FIELD_TEMP_LOAD:
    {
        const auto& x = symbolController->getOrCreateSymbolData<X_Font_Load_Implicit>(newsymbol);
        if (x->_pos == pos && x->_txt == txt && x->_color == color)
            return false;
        x->_pos = pos;
        x->_txt = txt;
        x->_isFixedPixel = false;
        x->_depthState = false;
        x->_color = color;
        symbolController->createSymbol(x);
    }
    break;
    default:
        return false;
    }
    symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { newsymbol }, 0.05);
    return true;
}

// ---------- 面板实时渲染防重签名（FNV-1a）----------
// 浮点按位取整哈希：相同输入必得相同签名（0.0f/-0.0f 差异无实际影响）
static size_t floatBitsToSize(float f)
{
    union { float f; quint32 u; } bits;
    bits.f = f;
    return static_cast<size_t>(bits.u);
}

size_t AllRenderSymbolManager::beginRenderSignature()
{
    return 14695981039346656037ULL;
}

void AllRenderSymbolManager::combineRenderSignature(size_t& sig, size_t value)
{
    sig ^= value;
    sig *= 1099511628211ULL;
}

void AllRenderSymbolManager::combineRenderSignature(size_t& sig, const QVector3D& v)
{
    combineRenderSignature(sig, floatBitsToSize(v.x()));
    combineRenderSignature(sig, floatBitsToSize(v.y()));
    combineRenderSignature(sig, floatBitsToSize(v.z()));
}

void AllRenderSymbolManager::combineRenderSignature(size_t& sig, const QVector<QVector3D>& vs)
{
    combineRenderSignature(sig, vs.size());
    for (const auto& v : vs)
        combineRenderSignature(sig, v);
}

void AllRenderSymbolManager::combineRenderSignature(size_t& sig, const QVector<std::pair<QVector3D, QVector3D>>& posDir)
{
    combineRenderSignature(sig, posDir.size());
    for (const auto& pd : posDir)
    {
        combineRenderSignature(sig, pd.first);
        combineRenderSignature(sig, pd.second);
    }
}

void AllRenderSymbolManager::combineRenderSignature(size_t& sig, const QMap<uint32_t, float>& value)
{
    combineRenderSignature(sig, static_cast<size_t>(value.size()));
    for (auto it = value.begin(); it != value.end(); ++it)
    {
        combineRenderSignature(sig, it.key());
        combineRenderSignature(sig, floatBitsToSize(it.value()));
    }
}

void AllRenderSymbolManager::combineRenderSignature(size_t& sig, const std::map<RenderPickResultData, QVector<QVariant>>& table)
{
    combineRenderSignature(sig, table.size());
    for (const auto& entry : table)
    {
        combineRenderSignature(sig, static_cast<size_t>(entry.first.result_id));
        combineRenderSignature(sig, static_cast<size_t>(entry.first.result_id2));
        combineRenderSignature(sig, static_cast<size_t>(entry.first.part_id));
        combineRenderSignature(sig, static_cast<size_t>(entry.second.size()));
        // 独立表值哈希其字符串表示：数值/空值均确定（QVariant::toString 对数值确定）
        for (const auto& v : entry.second)
        {
            const QString s = v.toString();
            combineRenderSignature(sig, static_cast<size_t>(s.size()));
            for (const QChar& c : s)
                combineRenderSignature(sig, static_cast<size_t>(c.unicode()));
        }
    }
}

void AllRenderSymbolManager::assignConnector(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData,
    const QVector3D& masterpos, const QVector<QVector3D>& slavepos, const ConnectorType& conntype, bool isSpring1)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("connector_%1").arg(_connectorData->getConnectorId());

    // 数据不完整（面板刚创建、主从节点未选全）时不创建符号，并清除旧符号，
    // 避免在原点渲染出只有文字的空符号；注意不能判断 masterpos.isNull()，
    // 主节点可以合法位于原点 (0,0,0)，空数据调用的从位置列表必为空
    if (slavepos.isEmpty())
    {
        symbolController->deleteSymbol({ newsymbol });
        return;
    }

    switch (conntype)
    {
    case MxFunctions::Connector_Rigid:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Line_Connect_Implicit>(newsymbol);
        arrow->_masterPos = masterpos;
        arrow->_slavePos = slavepos;
        arrow->_txt = "Rigid";
        arrow->_font_pixel_size = 25.0f;
        arrow->_color = QVector3D(74.0f, 160.0f, 67.0f);
        symbolController->createSymbol(arrow);
        break;
    }
    case MxFunctions::Connector_Flexible:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Line_Connect_Implicit>(newsymbol);
        arrow->_masterPos = masterpos;
        arrow->_slavePos = slavepos;
        arrow->_txt = "Flexible";
        arrow->_font_pixel_size = 25.0f;
        arrow->_color = QVector3D(66.0f, 128.0f, 220.0f);
        symbolController->createSymbol(arrow);
        break;
    }
    case MxFunctions::Connector_MPC:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Line_Connect_Implicit>(newsymbol);
        arrow->_masterPos = masterpos;
        arrow->_slavePos = slavepos;
        arrow->_txt = "MPC";
        arrow->_font_pixel_size = 25.0f;
        arrow->_color = QVector3D(216.0f, 139.0f, 44.0f);
        symbolController->createSymbol(arrow);
        break;
    }
    case MxFunctions::Connector_DistributedCoupling:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Line_Connect_Implicit>(newsymbol);
        arrow->_masterPos = masterpos;
        arrow->_slavePos = slavepos;
        arrow->_txt = "";
        arrow->_color = QVector3D(66.0f, 128.0f, 220.0f);
        arrow->_depthState = false;
        symbolController->createSymbol(arrow);
        break;
    }
    case MxFunctions::Connector_BearingUnits:
    {
        const auto& arrow = symbolController->getOrCreateSymbolData<Line_Connect_Implicit>(newsymbol);
        arrow->_masterPos = masterpos;
        arrow->_slavePos = slavepos;
        arrow->_txt = "";
        arrow->_depthState = false;
        arrow->_color = QVector3D(66.0f, 128.0f, 220.0f);

        symbolController->createSymbol(arrow);
        break;
    }
    default:
        break;
    }
}

void AllRenderSymbolManager::assignConnector(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData, const QVector<QVector3D>& masterpos, const QVector<QVector3D>& slavepos)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("connector_%1").arg(_connectorData->getConnectorId());

    if (slavepos.isEmpty() || masterpos.isEmpty())
    {
        return;
    }
    const auto& arrow = symbolController->getOrCreateSymbolData<FaceToFace_Connect_Implicit>(newsymbol);
    arrow->_masterPos = masterpos;
    arrow->_slavePos = slavepos;
    symbolController->createSymbol(arrow);
}

void AllRenderSymbolManager::assignDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData, const QVector3D& masterpos, const QVector3D& slavepos)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("deputy_%1").arg(_deputyData->getDeputyId());

    if (masterpos.isNull() || slavepos.isNull())
    {
        return;
    }
    const auto& arrow = symbolController->getOrCreateSymbolData<Type1_RC_Implicit>(newsymbol);
    QVector<std::pair<QVector3D, QVector3D>> pos_dir;
    pos_dir.push_back(std::make_pair(masterpos, slavepos));
    arrow->_pos_dir = pos_dir;
    arrow->_color = QVector3D(216.0f, 139.0f, 44.0f);
    symbolController->createSymbol(arrow);
}

void AllRenderSymbolManager::assignDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData, const QVector<QVector3D>& masterpos, const QVector<QVector3D>& slavepos)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    if (!symbolController)
        return;

    QString symbol = QString("deputy_%1").arg(_deputyData->getDeputyId());
    symbolController->deleteSymbol({ symbol });

    if (masterpos.isEmpty() || slavepos.isEmpty() || masterpos.size() != slavepos.size())
    {
        return;
    }

    QVector<QVector3D> vp;
    std::vector<unsigned int> lineIndex;
    std::vector<std::pair<QString, QVector3D>> _txt_pos;//文字和位置
    for (int i = 0; i < masterpos.size(); ++i)
    {
        vp.push_back(masterpos[i]);
        vp.push_back(slavepos[i]);
        _txt_pos.push_back(std::make_pair(QString::fromUtf8("Conn3d2"), (masterpos[i] + slavepos[i]) / 2));
        lineIndex.push_back(static_cast<unsigned int>(2 * i));
        lineIndex.push_back(static_cast<unsigned int>(2 * i + 1));
    }

    const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbol);
    un->_vexter = vp;
    un->_lineIndex = lineIndex;
    un->_lineColor = { 255, 0, 0 };
    un->_faceColor = { 0, 255, 0 };
    un->_txt_pos = _txt_pos;
    un->_font_pixel_size = 25.0f;
    un->_textColor = { 255, 0, 0 };
    un->_size = 1.0f;
    un->_pointDepthState = true;
    un->_lineIsFixedPixel = false;
    un->_lineDepthState = false;
    un->_faceIsFixedPixel = false;
    un->_faceDepthState = false;
    un->_showState = true;
    symbolController->createSymbol(un);
}

void AllRenderSymbolManager::clearDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData)
{
    if (!_deputyData)
        return;
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    if (!symbolController)
        return;

    QString symbol = QString("deputy_%1").arg(_deputyData->getDeputyId());
    symbolController->deleteSymbol({ symbol });
}

void AllRenderSymbolManager::assignConnectorBolt(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData, const vector<std::pair<QVector3D, QVector3D>>& pos_dir,
    const QString& txt, const QVector3D& color, const bool type)
{
    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
    QString newsymbol = QString("connector_%1").arg(_connectorData->getConnectorId());
    auto connectorType = _connectorData->getConnectorType();

    // 作用对象为空（面板刚创建、对象未选）时不创建符号，并清除旧符号，
    // 避免注册空符号干扰 showConnector 的 isExistSymbol 判断
    if (pos_dir.empty())
    {
        symbolController->deleteSymbol({ newsymbol });
        return;
    }

    switch (connectorType)
    {
    case MxFunctions::Connector_Bolt:
    {
        const auto& PRL = symbolController->getOrCreateSymbolData<PreLoad>(newsymbol);
        PRL->_Type = type;
        PRL->_pos_dir = pos_dir;
        PRL->_pos_pos = pos_dir;
        PRL->_size = 1.0f;
        PRL->_isFixedPixel = false;
        PRL->_depthState = false;
        PRL->_showState = true;
        PRL->_Color = QVector3D(255, 255, 0);
        symbolController->createSymbol(PRL);
    }
    default:
        break;
    }
    symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { newsymbol }, 0.05);
}

QVector<QVector3D> AllRenderSymbolManager::transPickedToNodes(const std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
{
    QVector<QVector3D> rendpos;
    for (int i = 0; i < nodes_id.size(); ++i)
    {
        auto pickedtype = nodes_id[i].first;
        switch (pickedtype)
        {
        case MxFunctions::PickDataType::Node:
        {
            rendpos.push_back(getCVM->getModelData()->getMeshData()->getVectorById(nodes_id[i].second));
        }
        break;
        case MxFunctions::PickDataType::GeoPoint:
        {
            auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ nodes_id[i].second });
            rendpos.push_back(points[0].m_coor);
        }
        break;
        case MxFunctions::PickDataType::GeoEdge:
        {
            QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, { nodes_id[i].second });
            rendpos.append(coords);
        }
        break;
        case MxFunctions::PickDataType::GeoFace:
        {
            QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, { nodes_id[i].second });
            rendpos.append(coords);
        }
        break;
        case MxFunctions::PickDataType::GeoSolid:
        {
            QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, { nodes_id[i].second });
            rendpos.append(coords);
        }
        break;
        case MxFunctions::PickDataType::Part:
        {
            QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(4, { nodes_id[i].second });
            rendpos.append(coords);
        }
        break;
        default:
            break;
        }
    }
    return rendpos;
}

void AllRenderSymbolManager::getSetRenderData(std::set<int> setIDs, std::vector<RenderPickResultData>& meshRenderData, std::vector<int>& geoRenderData, std::vector<mxr::MeshSelectInfoData>& nodeRenderData)
{
    //获取映射
    auto meshData = getCVM->getModelData()->getMeshData();

    for (auto setID : setIDs)
    {
        mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
        if (!set) continue;

        if (set->getSetType() == Mix)
        {
            auto mixset = dynamic_cast<mMixSet*>(set);
            auto mixData = mixset->getMixSetData();
            for (const auto& pair : mixData)
            {
                if (pair.first == PickDataType::GeoFace)
                {
                    vector<int> datas; datas.reserve(pair.second.size());
                    for (const auto& id : pair.second) { datas.push_back(id.result_id); }
                    geoRenderData.insert(geoRenderData.end(), datas.begin(), datas.end());
                }
                else if (pair.first == PickDataType::ElementFace)
                {
                    meshRenderData.insert(meshRenderData.end(), pair.second.begin(), pair.second.end());
                }
                else if (pair.first == PickDataType::Node)
                {
                    appendNodeRenderData(pair.second, nodeRenderData);
                }
            }
        }
        else if (set->getSetType() == GeoFace)
        {
            auto geoset = dynamic_cast<mGeoSet*>(set);
            std::vector<int> ids = geoset->getGeoSetData();
            geoRenderData.insert(geoRenderData.end(), ids.begin(), ids.end());
        }
        else if (set->getSetType() == ElementSurface)
        {
            auto elesurfaceset = dynamic_cast<mElementSurface*>(set);
            std::vector<RenderPickResultData>datas = elesurfaceset->getElementSurfaceData();
            meshRenderData.insert(meshRenderData.end(), datas.begin(), datas.end());
        }
        else if (set->getSetType() == MxFunctions::Node)
        {
            auto nodeset = dynamic_cast<MechSetData::mNode*>(set);
            if (!nodeset) continue;
            appendNodeRenderData(nodeset->getNodeData(), nodeRenderData);
        }
        else if (set->getSetType() == MxFunctions::NodeSurface)
        {
            // 节点面集（点面接触从面可为节点表面集）：数据为节点，同样走节点高亮
            auto nodeFaceSet = dynamic_cast<MechSetData::mNodeFace*>(set);
            if (!nodeFaceSet) continue;
            appendNodeRenderData(nodeFaceSet->getNodeData(), nodeRenderData);
        }
    }
}

QVector<std::pair<QVector3D, QVector3D>> AllRenderSymbolManager::getIndependentLoadSetForce(std::shared_ptr<MechCaseData::mLoadForce> loadData)
{
    QVector<std::pair<QVector3D, QVector3D>> pos_dir;
    for (auto it = loadData->nodeForceMap_.begin(); it != loadData->nodeForceMap_.end(); ++it)
    {
        int nodeId = it->first.result_id;
        QVector3D pos = getCVM->getModelData()->getMeshData()->getVectorById(nodeId);
        QVector3D dir(0, 0, 0);
        const QVector<QVariant>& values = it->second;
        if (values.size() > 0 && values[0].isValid() && !values[0].isNull())
            dir.setX(values[0].toDouble());
        if (values.size() > 1 && values[1].isValid() && !values[1].isNull())
            dir.setY(values[1].toDouble());
        if (values.size() > 2 && values[2].isValid() && !values[2].isNull())
            dir.setZ(values[2].toDouble());
        pos_dir.push_back(std::make_pair(pos, dir));
    }
    return pos_dir;
}

QVector<std::pair<QVector3D, QVector3D>> AllRenderSymbolManager::getIndependentLoadSetPressure(std::shared_ptr<MechCaseData::mLoadPressure> loadData)
{
    QVector<std::pair<QVector3D, QVector3D>> pos_dir;
    for (const auto& entry : loadData->cellFaceMap_)
    {
        const RenderPickResultData& meshFaceId = entry.first;
        QVector3D centerPos;
        QVector3D vertex;
        getCVM->getModelData()->getMeshData()->GetOutVectorByMeshFaceID(meshFaceId, vertex, centerPos);
        // 渲染方向与压强值符号关联：默认为面外法向的反向，该面压强为负时取反；无值默认正方向
        QVector3D dir = -vertex;
        const QVector<QVariant>& vals = entry.second;
        if (!vals.isEmpty() && vals[0].isValid() && !vals[0].isNull())
        {
            bool ok = false;
            double v = vals[0].toDouble(&ok);
            if (ok && v < 0)
            {
                dir = vertex;
            }
        }
        pos_dir.push_back(std::make_pair(centerPos, dir));
    }
    return pos_dir;
}

QVector<QVector3D> AllRenderSymbolManager::getIndependentLoadSetHeatflux(std::shared_ptr<MechCaseData::mLoadHeatFlux> loadData)
{
    QVector<QVector3D> objs;
    for (const auto& entry : loadData->cellFaceData_)
    {
        const RenderPickResultData& key = entry.first;
        if (loadData->way_ == 0)
        {
            objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(key.result_id));
        }
        else if (loadData->way_ == 1)
        {
            QVector3D centerPos;
            QVector<QVector3D> vertex;
            getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(key, centerPos, vertex);
            objs.push_back(centerPos);
        }
        else if (loadData->way_ == 2)
        {
            QVector3D centerPos;
            QVector<QVector3D> vertexs;
            getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshID(key.result_id, centerPos, vertexs);
            objs.push_back(centerPos);
        }
    }
    return objs;
}

QVector<QVector3D> AllRenderSymbolManager::getIndependentLoadSetRadiation(std::shared_ptr<MechCaseData::mLoadRadiation> loadData)
{
    QVector<QVector3D> objs;
    for (const auto& entry : loadData->cellFaceData_)
    {
        const RenderPickResultData& meshFaceId = entry.first;
        QVector3D centerPos;
        QVector<QVector3D> vertex;
        getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(meshFaceId, centerPos, vertex);
        objs.push_back(centerPos);
    }
    return objs;
}

QVector<QVector3D> AllRenderSymbolManager::getIndependentLoadSetConvection(std::shared_ptr<MechCaseData::mLoadConvection> loadData)
{
    QVector<QVector3D> objs;
    for (const auto& entry : loadData->cellFaceData_)
    {
        const RenderPickResultData& meshFaceId = entry.first;
        QVector3D centerPos;
        QVector<QVector3D> vertex;
        getCVM->getModelData()->getMeshData()->GetShrinkVertexByMeshFaceID(meshFaceId, centerPos, vertex);
        objs.push_back(centerPos);
    }
    return objs;
}

QVector<QVector3D> AllRenderSymbolManager::getIndependentPredefinedFieldSetInitTemp(std::shared_ptr<MechCaseData::mPredefinedFieldInitTemp> fieldData)
{
    QVector<QVector3D> objs;
    for (const auto& entry : fieldData->_nodeTempMap)
    {
        int nodeId = entry.first.result_id;
        objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(nodeId));
    }
    return objs;
}

QVector<QVector3D> AllRenderSymbolManager::getIndependentPredefinedFieldSetTempLoad(std::shared_ptr<MechCaseData::mPredefinedFieldTempLoad> fieldData)
{
    QVector<QVector3D> objs;
    for (const auto& entry : fieldData->_nodeTempMap)
    {
        int nodeId = entry.first.result_id;
        objs.push_back(getCVM->getModelData()->getMeshData()->getVectorById(nodeId));
    }
    return objs;
}

QVector<std::pair<QVector3D, QVector3D>> AllRenderSymbolManager::getIndependentLoadSetSurface(std::shared_ptr<MechCaseData::mLoadSurface> loadData)
{
    QVector<std::pair<QVector3D, QVector3D>> pos_dir;
    // 独立赋值表格数据 surfaceForceMap_
    const auto& mapData = loadData->surfaceForceMap_;

    for (const auto& entry : mapData)
    {
        const RenderPickResultData& meshFaceId = entry.first;
        const QVector<QVariant>& varData = entry.second;
        // varData[0]:载荷值，varData[1]:v1，varData[2]:v2，varData[3]:v3
        if (varData.size() < 4)
            continue;

        // 任一方向值为空/无效，代表方向未输入，跳过渲染
        if ((!varData[1].isValid() || varData[1].isNull()) &&
            (!varData[2].isValid() || varData[2].isNull()) &&
            (!varData[3].isValid() || varData[3].isNull()))
            continue;

        bool ok1 = false, ok2 = false, ok3 = false;
        float v1 = varData[1].toFloat(&ok1);
        float v2 = varData[2].toFloat(&ok2);
        float v3 = varData[3].toFloat(&ok3);
        if (!ok1 && !ok2 && !ok3)
            continue;

        QVector3D dir(
            ok1 ? v1 : 0.0f,
            ok2 ? v2 : 0.0f,
            ok3 ? v3 : 0.0f);
        // 浮点判定：零向量跳过
        if (dir.lengthSquared() < 1e-9)
            continue;

        // 该面压强值为负时方向取反；无值默认正方向
        if (varData[0].isValid() && !varData[0].isNull())
        {
            bool ok0 = false;
            float p = varData[0].toFloat(&ok0);
            if (ok0 && p < 0)
            {
                dir = -dir;
            }
        }

        // 获取面中心点，法线丢弃，方向使用表格v1v2v3
        QVector3D centerPos;
        QVector3D dummyNormal;
        getCVM->getModelData()->getMeshData()->GetOutVectorByMeshFaceID(meshFaceId, dummyNormal, centerPos);

        pos_dir.push_back(std::make_pair(centerPos, dir));
    }
    return pos_dir;
}