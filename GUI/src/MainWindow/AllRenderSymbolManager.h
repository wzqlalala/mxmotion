#pragma once

#include <QObject>
#include <QMap>
#include <QColor>
#include <QList>
#include <unordered_map>


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
#include "mPickWidgetData.h"

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

#include "mContactBase.h"
#include "mPredefinedFieldBase.h"


// 全局符号管理器（核心！）
class AllRenderSymbolManager : public QObject
{
    Q_OBJECT
public:
    static AllRenderSymbolManager* getInstance(); //获取单例
    void showLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData); //显示创建载荷符号，会判断渲染符号是否存在，存在则显示，不存在则创建
    void rendLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData); //创建载荷符号

    void showBoundary(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData);
    void rendBoundary(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData); //创建边界符号

    void showConnector(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData);
    void rendConnector(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData, bool showhide);
    void rendConnector(QVector<int> phyIds);

    //周期边界渲染
    void rendPeriodicBoundary(int pbId);
    void rendPeriodicBoundary(QVector<int> pbIds);

    void showDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData);
    void rendDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData, bool showhide);

    void showPredefinedField(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData);
    void rendPredefinedField(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData);

    void rendContact(MechContactData::mContactBase* _contactData);
    void rendContact(QVector<int> phyIds);
    void rendContact(const MechMainWindow::PickWidgetData& data, std::vector<RenderPickResultData>& meshRenderData, std::vector<int>& geoRenderData, std::vector<mxr::MeshSelectInfoData>* nodeRenderData = nullptr,
        std::vector<RenderPickResultData>* meshEdgeRenderData = nullptr, std::vector<int>* geoEdgeRenderData = nullptr);

public:
    //载荷获取作用对象函数（重载形式，避免非const引用绑定到临时对象）
    QVector<QVector3D> getLoadSet(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData);
    QVector<QVector3D> getLoadSet(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs);
    QVector<QVector3D> getLoadSetPressure(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData);
    QVector<QVector3D> getLoadSetPressure(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs);
    QVector<QVector3D> getLoadSetHeatflux(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData);
    QVector<QVector3D> getLoadSetHeatflux(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs);
    QVector<QVector3D> getLoadSetConvection(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData);
    QVector<QVector3D> getLoadSetConvection(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs);
    QVector<QVector3D> getLoadSetRadiation(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData);
    QVector<QVector3D> getLoadSetRadiation(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs);
    QVector<QVector3D> getLoadSetAcoconcforce(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData);
    QVector<QVector3D> getLoadSetAcoconcforce(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, QVector<QVector3D>& dirs);    //独立赋值获取作用对象函数
    //边载荷获取作用对象位置与方向（几何线/几何面/单元线）
    QVector<std::pair<QVector3D, QVector3D>> getLoadSetEdge(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData);
    //计算边载荷各作用边的位置与方向（面板临时渲染与符号渲染共用）
    //targets：作用对象；dirMode：0面内法向 1面外法向 2边切向 3通用；generalDir：通用模式方向（方向控件）
    static QVector<std::pair<QVector3D, QVector3D>> calcEdgeLoadPosDir(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& targets,
        int dirMode, const QVector3D& generalDir);
    QVector<std::pair<QVector3D, QVector3D>> getIndependentLoadSetForce(std::shared_ptr<MechCaseData::mLoadForce> loadData);
    QVector<std::pair<QVector3D, QVector3D>> getIndependentLoadSetPressure(std::shared_ptr<MechCaseData::mLoadPressure> loadData);
    QVector<QVector3D> getIndependentLoadSetHeatflux(std::shared_ptr<MechCaseData::mLoadHeatFlux> loadData);
    QVector<QVector3D> getIndependentLoadSetRadiation(std::shared_ptr<MechCaseData::mLoadRadiation> loadData);
    QVector<QVector3D> getIndependentLoadSetConvection(std::shared_ptr<MechCaseData::mLoadConvection> loadData);
    QVector<QVector3D> getIndependentPredefinedFieldSetInitTemp(std::shared_ptr<MechCaseData::mPredefinedFieldInitTemp> fieldData);
    QVector<QVector3D> getIndependentPredefinedFieldSetTempLoad(std::shared_ptr<MechCaseData::mPredefinedFieldTempLoad> fieldData);
    QVector<std::pair<QVector3D, QVector3D>> getIndependentLoadSetSurface(std::shared_ptr<MechCaseData::mLoadSurface> loadData);
    //边界获取作用对象
    QVector<QVector3D> getBoundarySet(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData);
    // 位移边界独立赋值获取作用对象
    QVector<QVector3D> getIndependentBoundarySetDisplacement(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData);
    //预定义场获取作用对象
    QVector<QVector3D> getPredefinedFieldSet(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData);
    //螺栓约束获取作用对象（重载形式）
    QVector<QVector3D> getConnectorBolt(std::shared_ptr<MechConnectorData::mConnectorBase> loadData, int way_);
    QVector<QVector3D> getConnectorBolt(std::shared_ptr<MechConnectorData::mConnectorBase> loadData, int way_, QVector<QVector3D>& dirs);

    //赋予载荷
    void assignLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, const QVector<std::pair<QVector3D, QVector3D>>& pos_dir, const QString& txt, const QVector3D& color);
    void assignThermalLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData,
        const QVector<QVector3D>& pos_dir, const QString& txt, const QVector3D& color, int index = 0);
     //赋予边界
    void assignBoundary(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData,
        const QVector<QVector3D>& pos, const QMap<uint32_t, float>& value, const QVector3D& color);
    void assignBoundary(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData,
        const QVector<QVector3D>& pos, const QVector3D& color);

    // 位移/速度/加速度边界：使用与集中力相同的箭头符号（仅颜色区分）
    // 分量与方向映射：key 1/4（U1/UR1）->X、2/5（U2/UR2）->Y、3/6（U3/UR3）->Z，方向受数值正负影响（负值反向）
    // 同轴 U 与 UR 同时存在时：方向一致只渲染一个箭头，方向相反时两个箭头都渲染
    void assignBoundaryDisplacement(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData,
        const QVector<QVector3D>& pos, const QMap<uint32_t, float>& value, const QVector3D& color);

    // 独立赋值位移/速度/加速度边界：按节点-分量精确渲染
    // 同一分量有值的节点合并为一个 symbol（boundary_<id>_u<key>），避免 value 全局生效导致所有节点都渲染该方向
    void assignBoundaryIndependentDisplacement(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData,
        const std::map<RenderPickResultData, QVector<QVariant>>& independentTable, const QVector3D& color);

    // 获取某边界所有可能的 symbol 名称（主 symbol + 独立赋值分量 symbol），供外部统一删除
    static std::vector<QString> getBoundarySymbolNames(int boundaryId);
    //赋予预定义场；返回符号数据是否发生变化（未变化时调用方可跳过 UpdateAllSymbol 全量重建）
    bool assignPredefinedField(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData,
        const QVector<QVector3D>& pos, const QString& txt, const QVector3D& color);

    // ---------- 面板实时渲染防重签名 ----------
    // 面板把提交给符号的最终数据（对象ID+位置/方向/分量值）哈希成签名，与上次
    // 一致时跳过符号赋值与 UpdateAllSymbol 全量重建（进入面板防抖触发/切出冲刷
    // 时数据往往与上次相同，全量重建大模型下代价很高）
    static size_t beginRenderSignature();
    static void combineRenderSignature(size_t& sig, size_t value);
    static void combineRenderSignature(size_t& sig, const QVector3D& v);
    static void combineRenderSignature(size_t& sig, const QVector<QVector3D>& vs);
    static void combineRenderSignature(size_t& sig, const QVector<std::pair<QVector3D, QVector3D>>& posDir);
    static void combineRenderSignature(size_t& sig, const QMap<uint32_t, float>& value);
    static void combineRenderSignature(size_t& sig, const std::map<RenderPickResultData, QVector<QVariant>>& table);
    //赋予相互接触
    void assignConnector(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData,const QVector3D& masterpos, const QVector<QVector3D>& slavepos, const ConnectorType& conntype, bool isSpring1 = false);
    void assignConnector(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData,const QVector<QVector3D>& masterpos, const QVector<QVector3D>& slavepos);

    //赋予连接副
    void assignDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData, const QVector3D& masterpos, const QVector3D& slavepos);
    void assignDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData, const QVector<QVector3D>& masterpos, const QVector<QVector3D>& slavepos);

    //清除连接副渲染
    void clearDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData);

    //赋予螺栓约束
    void assignConnectorBolt(std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData, const vector<std::pair<QVector3D, QVector3D>>& pos_dir,
        const QString& txt, const QVector3D& color, const bool type);


    QVector<QVector3D> transPickedToNodes(const std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);//从节点取多个位置信息

    void getSetRenderData(std::set<int> setIDs, std::vector<RenderPickResultData>& meshRenderData, std::vector<int>& geoRenderData, std::vector<mxr::MeshSelectInfoData>& nodeRenderData);

private:
    explicit AllRenderSymbolManager(QObject* parent = nullptr);
    static AllRenderSymbolManager* m_instance;

    //绑定所需的两个容器
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>mainNodesBind_;
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>subNodesBind_;

    //弹簧阻尼所需的两个容器
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>mainNodesSpringDamp_;
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>subNodesSpringDamp_;

    //刚性连接所需的五个容器
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>mainNodesRigid_;
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>subNodesRigid_;
    std::vector<std::pair<MxFunctions::PickDataType, int>>mainNodesRigid_id;
    std::vector<std::pair<MxFunctions::PickDataType, int>>subNodesRigid_id;
    QVector<QVector3D> renderposRigid;

    //柔性连接所需的五个容器
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>mainNodesFlexible_;
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>subNodesFlexible_;
    std::vector<std::pair<MxFunctions::PickDataType, int>>mainNodesFlexible_id;
    std::vector<std::pair<MxFunctions::PickDataType, int>>subNodesFlexible_id;
    QVector<QVector3D> renderposFlexible;

    //MPC约束所需的五个容器
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>mainNodesMPC_;
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>subNodesMPC_;
    std::vector<std::pair<MxFunctions::PickDataType, int>>mainNodesMPC_id;
    std::vector<std::pair<MxFunctions::PickDataType, int>>subNodesMPC_id;
    QVector<QVector3D> renderposMPC;

    //分布式耦合所需的五个容器
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>mainNodesDistributedCoupling_;
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>subNodesDistributedCoupling_;
    std::vector<std::pair<MxFunctions::PickDataType, int>>mainNodesDistributedCoupling_id;
    std::vector<std::pair<MxFunctions::PickDataType, int>>subNodesDistributedCoupling_id;
    QVector<QVector3D> renderposDistributedCoupling;

    //轴承单元所需的五个容器
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>node1BearingUnits_;
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>node2BearingUnits_;
    std::vector<std::pair<MxFunctions::PickDataType, int>>node1BearingUnits_id;
    std::vector<std::pair<MxFunctions::PickDataType, int>>node2BearingUnits_id;
    QVector<QVector3D> renderposBearingUnits;
    //连接副所需的四个容器
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>mainNodesDeputy_;
    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>subNodesDeputy_;
    std::vector<std::pair<MxFunctions::PickDataType, int>>mainNodesDeputy_id;
    std::vector<std::pair<MxFunctions::PickDataType, int>>subNodesDeputy_id;
};