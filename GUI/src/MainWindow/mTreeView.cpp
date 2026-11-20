#include "mTreeView.h"
#include "mUiMenu.h"
#include "mTreeDelegate.h"
#include "mTreeModel.h"
#include "mDockLayoutManager.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "mSectionWindow.h"

#include <QHeaderView>
#include <QDebug>
#include "mModelEnum.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QContextMenuEvent>
#include <QShortcut>
#include <QKeySequence>

#include <functional>
#include "mInfoWindow.h"
#include "mMeshPanel.h"
#include "mSetManagerPanel.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mMenuWidget.h"
#include "mOutputInfoWindow.h"
#include "mSelectMenuFunction.h"
#include "PickCommonFunction.h"
#include "mViewTreeFunction.h"
#include "mViewBrowserWindow.h"
#include "ThemeManager.h"

#include "mDisplayMenuFunction.h"
//MxFunctions
#include "mGlobalSignals.h"

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
#include "mSetManager.h"
#include "mSetBase.h"
#include "mNode.h"
#include "mCoordinateBase.h"
#include "mCoordinateManager.h"
#include "mFunctionManager.h"
#include "mMeshPanelManager.h"
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitModal.h"
#include "mMaterialBase.h"
#include "mMaterialManager.h"
#include "mPropertyBase.h"
#include "mNonStructuralQuality.h"
#include "mPropertyManager.h"
#include "mFunctionBase.h"
#include "mFreqSamplPointBase.h"
#include "mLaminateBase.h"
#include "mLaminateManager.h"
#include "mDiscreteField.h"
#include "mFieldManager.h"
#include "mAnalysisField.h"
#include "mConnectorBase.h"
#include "mConnectorManager.h"
#include "mDeputyBase.h"
#include "mDeputyManager.h"
#include "mPeriodicBoundary.h"
#include "mPeriodicBoundaryManager.h"


//
#include "mMeshPanelBase.h"
#include "mSectionShapeBase.h"
#include "mSectionShapeManager.h"
#include "mContactBase.h"
#include "mContactManager.h"
#include "mModelFixManager.h"
#include "mModelFixDataBase.h"
#include "mStructOption.h"
#include "mOptimizationDesignData.h"
#include "mOptimizationConstraintData.h"
#include "mRunnerStructOption.h"
#include "mRunnerOptimizationGoalData.h"
#include "mRunnerOptimizationDesignData.h"
#include "mRunnerOptimizationConstraintData.h"
#include "mGenericAssignData.h"
#include "mGenericMatrixData.h"
#include "mGenericParamData.h"
#include "mGlobalParam.h"

//
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostModelData.h"
#include "mPostParseMesh.h"#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mLoadBoundaryBase.h"
#include "mLoadBoundaryManager.h"
#include "mPredefinedFieldManager.h"
#include "mPredefinedFieldBase.h"
#include "mRoatateAnalyseBase.h"
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"

//label
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"
#include "mxr/view/HighlightController.h"

//commond
#include "mCommondSystem.h"
#include "mCommonds.h"
#include <QFontDatabase>
#include "PickCommonFunction.h"

#include "mGeoManager.h"
#include "MXMesh.h"
#include "GlobalMeshMessage.h"
#include "mFramelessMessageBox.h"	//提示框统一自绘标题栏（树右键清空/删除确认）
#include <QMessageBox>
#include <QPushButton>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QCoreApplication>
#include <QString>
#include <algorithm>
#include <mGeoSketchSignalManager.h>
#include "mAppConfiguration.h"
#include "mIOFileManager.h"

#include "AllRenderSymbolManager.h"
#include "mDockLayoutManager.h"
#include "mCaseImplicitFrequency.h"
#include "mCaseImplASCoupling.h"
#include "mMotionBodyManager.h"
#include "mMotionRefMap.h"
#include "mMotionBody.h"
#include "mMotionJointManager.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointSymbolHelper.h"
#include "mMotionCouplingManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionCouplingGear.h"
#include "mMotionCouplingGearRack.h"
#include "mMotionDriveManager.h"
#include "mMotionJointDrive.h"
#include "mMotionSpringPanel.h"
#include "mMotionVectorSymbolHelper.h"
#include "mMotionScenarioManager.h"
#include "mMotionScenario.h"
#include "mMotionMarker.h"
#include "mMotionVectorForce.h"
#include "mMotionScalarForce.h"
#include "mMotionForceElement.h"
#include "mMotionSpringBase.h"
#include "mMotionSpringManager.h"
#include "mMotionForceElementManager.h"
#include "mMotionContact.h"
#include "mMotionContactManager.h"
#include "mMotionControlInput.h"
#include "mMotionControlInputManager.h"
#include "mMotionElectroImport.h"
#include "mMotionElectroImportManager.h"
#include "mMotionElectroExport.h"
#include "mMotionElectroExportManager.h"
#include "mMotionHydraulic.h"
#include "mMotionHydraulicManager.h"
#include "mMotionRL.h"
#include "mMotionRLManager.h"
#include "mMotionSensorBase.h"
#include "mMotionSensorManager.h"
#include "mMotionControllerBase.h"
#include "mMotionControllerManager.h"
#include "mMotionPIDController.h"
#include "mMotionAutoFlex.h"
#include "mMotionAutoFlexManager.h"
#include "mAPIServer.h"
#include "mPythonAgent.h"

using namespace MxFunctions;
using namespace MechModelData;
using namespace MechCommon;
using namespace MechPreData;
using namespace MechSetData;
using namespace MechCoordinateData;
using namespace MechCaseData;
using namespace MechMaterialData;
using namespace MechPropertyData;
using namespace MechFunctionData;
using namespace MeshPanelData;
using namespace PostData;
using namespace MechSectionShapeData;
using namespace MechConnectorData;
using namespace MechContactData;
using namespace MechMotionData;
using namespace MechModelFixData;
using namespace MechMapRelation;
using namespace MechGlobalParamData;
using namespace MechDiscreteFieldData;namespace MechMainWindow
{
    /*
    * 部件分组节点：统一经 ModelParentItem 取（多体形态下该分组挂在多体动力学根首位、
    * 模型根工厂跳过，见 mTreeModel::setPartGroupUnderMotionRoot；键按模型唯一，
    * 挂载位置对寻址透明）。部件的增删/遍历/选中各处统一经此取组
    */
    static mTreeItem* partGroupItem(MechPreData::mTreeModel* treeModel)
    {
        return treeModel->getParentItem(ModelParentItem);
    }

    // 力元树节点图标（与菜单栏各载荷类型图标一致），供树创建/更新各处统一使用
    static QString getForceElementPic(MechMotionData::VectorLoadType type)
    {
        using namespace MechMotionData;
        QString pic;
        switch (type)
        {
        case VectorLoadType::Force:              pic = ":/mMainWidgets/Icon/Motion/force_vector.png"; break;
        case VectorLoadType::Torque:             pic = ":/mMainWidgets/Icon/Motion/force_torque.png"; break;
        case VectorLoadType::UniForce:           pic = ":/mMainWidgets/Icon/Motion/force_vector.png"; break;
        case VectorLoadType::UniTorque:          pic = ":/mMainWidgets/Icon/Motion/force_torque.png"; break;
        case VectorLoadType::TriForce:           pic = ":/mMainWidgets/Icon/Motion/force_tri.png"; break;
        case VectorLoadType::TriTorque:          pic = ":/mMainWidgets/Icon/Motion/force_tri_torque.png"; break;
        case VectorLoadType::GeneralForce:       pic = ":/mMainWidgets/Icon/Motion/force_general.png"; break;
        case VectorLoadType::Gravity:            pic = ":/mMainWidgets/Icon/Motion/force_gravity.png"; break;
        case VectorLoadType::ForceField:         pic = ":/mMainWidgets/Icon/Motion/force_field.png"; break;
        case VectorLoadType::LinearSpringDamper: pic = ":/mMainWidgets/Icon/Motion/force_spring.png"; break;
        case VectorLoadType::TireForce:          pic = ":/mMainWidgets/Icon/Motion/force_tire.png"; break;
        case VectorLoadType::Bushing:            pic = ":/mMainWidgets/Icon/Motion/bush.png"; break;
        case VectorLoadType::ScalarForce:        pic = ":/mMainWidgets/Icon/Motion/force.png"; break;
        case VectorLoadType::ScalarTorque:       pic = ":/mMainWidgets/Icon/Motion/moment.png"; break;
        default:                                 pic = ":/mMainWidgets/Icon/BrowserTree/motion_liYuanParentItem.png"; break;
        }
        return pic;
    }

    static const QVector<MxFunctions::SetType>& getSetTypeMenuOrder()
    {
        static const QVector<MxFunctions::SetType> order = {
            MxFunctions::GeoPoint, MxFunctions::GeoEdge, MxFunctions::GeoFace, MxFunctions::GeoSolid,
            MxFunctions::Node, MxFunctions::ElementEdge, MxFunctions::SurfaceAnalysis, MxFunctions::ElementSurface,
            MxFunctions::Element, MxFunctions::Part, MxFunctions::FrequencyPoint, MxFunctions::DOFNode,
            MxFunctions::Panel, MxFunctions::NodeSurface
        };
        return order;
    }

    mTreeView::mTreeView(QWidget* parent) : QTreeView(parent)
    {
        //mTreeDelegate(居GUIFoundation)按此属性识别前处理树（原dynamic_cast<mTreeView*>，解跨库类型依赖）
        setProperty("mechPreTreeView", true);
        setMouseTracking(true);  // 启用悬浮事件
        viewport()->setMouseTracking(true);
        viewport()->setAttribute(Qt::WA_Hover);  // 启用 hover 属性
        initBrowserTree();
        initConnect();
    }

    MechPreData::mTreeItem* mTreeView::getInsertLocation(BrowserTreeType InsertedItem) {
        switch (InsertedItem)
        {
        case SetParentItem:
        {
            return _treeModel->getParentItem(CoordinateParentItem);
        }
        case MeshOptParentItem:
        {
            return _treeModel->getParentItem(MeshParentItem);
        }
        case NonStructuralQualityParentItem:
        {
            return _treeModel->getParentItem(BoundaryManagerParentItem);
        }
        case OptimizationParentItem:
        {
            return _treeModel->getParentItem(GlobalParamParentItem);
        }
        case FunctionParentItem:
        {
            if (_treeModel->getParentItem(SetParentItem) != nullptr)
                return _treeModel->getParentItem(SetParentItem);
            else
                return _treeModel->getParentItem(CoordinateParentItem);
        }
        case FieldParentItem:
        {
            if (_treeModel->getParentItem(FunctionParentItem) != nullptr)
                return _treeModel->getParentItem(FunctionParentItem);
            else if (_treeModel->getParentItem(SetParentItem) != nullptr)
                return _treeModel->getParentItem(SetParentItem);
            else
                return _treeModel->getParentItem(CoordinateParentItem);
        }
        case PredefinedFieldManagerParentItem:
        {
            if (_treeModel->getParentItem(NonStructuralQualityParentItem) != nullptr)
                return _treeModel->getParentItem(NonStructuralQualityParentItem);
            else
                return _treeModel->getParentItem(BoundaryManagerParentItem);
        }
        case FrequencySamplingPointParentItem:
        {
            if (_treeModel->getParentItem(PredefinedFieldManagerParentItem) != nullptr)
                return _treeModel->getParentItem(PredefinedFieldManagerParentItem);
            else if (_treeModel->getParentItem(NonStructuralQualityParentItem) != nullptr)
                return _treeModel->getParentItem(NonStructuralQualityParentItem);
            else
                return _treeModel->getParentItem(BoundaryManagerParentItem);
        }
        case CaseParentItem:
        {
            vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
            if (caseIDs.size() > 1)
            {
                mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseIDs[caseIDs.size() - 2]);
                BrowserTreeType seltype = (BrowserTreeType)(caseBase->getCaseType() + CaseImplicitLinearStaticParentItem - 1);
                return _treeModel->getParentItem(seltype, caseBase->getCaseId());
            }

            if (_treeModel->getParentItem(FrequencySamplingPointParentItem) != nullptr)
                return _treeModel->getParentItem(FrequencySamplingPointParentItem);
            else if (_treeModel->getParentItem(PredefinedFieldManagerParentItem) != nullptr)
                return _treeModel->getParentItem(PredefinedFieldManagerParentItem);
            else if (_treeModel->getParentItem(NonStructuralQualityParentItem) != nullptr)
                return _treeModel->getParentItem(NonStructuralQualityParentItem);
            else
                return _treeModel->getParentItem(BoundaryManagerParentItem);
        }
        case SglLayerPlateParentItem:
        {
            //单层板直接插入截面属性下方
            return nullptr;
        }
        case LamPlateParentItem:
        {
            //层合板 如果单层板父节点存在 插入单层板父节点下方 不存在直接插入截面属性父节点
            if (_treeModel->getParentItem(SglLayerPlateParentItem) != nullptr)
                return _treeModel->getParentItem(SglLayerPlateParentItem);
            else
                return nullptr;
        }
        case SectionShapeParentItem:
        {
            //截面形状 如果层合板父节点存在 插入层合板父节点下方 不存在插入单层板父节点下方 还不在直接插入截面属性父节点
            if (_treeModel->getParentItem(LamPlateParentItem) != nullptr)
                return _treeModel->getParentItem(LamPlateParentItem);
            else  if(_treeModel->getParentItem(SglLayerPlateParentItem) != nullptr)
                return _treeModel->getParentItem(SglLayerPlateParentItem);
            else
                return nullptr;
        }
        default:
            return nullptr;
        }
    }

    void mTreeView::connectSig()
    {
        disconnectSig();
        m_viewportPrivate = VMManager->getCurrentViewportPrivate();
        m_viewportChangeConn = connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mTreeView::slot_subWindowActivated);
        //门面操作（新建/删除视口、模型）期间 currentViewportChanged 被断开，
        //此处按最终激活视口补一次分区显隐刷新
        updateTreeSectionsForViewport();
    }

    void mTreeView::disconnectSig()
    {
        if (m_viewportChangeConn)
        {
            disconnect(m_viewportChangeConn);
        }
    }

    QString mTreeView::getModelBrowserTreeName()
    {
        QVariant var = _treeModel->data(_treeModel->getParentIndex(ModelRootItem), Qt::DisplayRole);
        if (var.isValid()) return var.toString();
        return "";
    }

    void mTreeView::addModelBrowserTree()
    {
        newModelBrowserTree();
        auto* item = _treeModel->getParentItem(ModelRootItem);
        //新建模型树不折叠：selectedItem 会展开根节点显示一级分类，保留其他模型树的展开状态。
        //多体形态下模型根整树隐藏，改为展开/选中多体动力学顶层根（一级分类在多体根下）
        if (MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion)
        {
            item = _treeModel->getParentItem(Motion_RootItem);
        }
        selectedItem(item);
    }



    void mTreeView::nameFilterTree(const QString& filterStr)
    {
        _treeModel->nameFilterTree(filterStr);
        _treeProxyModel->invalidate();
    }

    void mTreeView::statusFilterTree(MxFunctions::BrowserTreeStatus filterStatus)
    {
        _treeModel->statusFilterTree(filterStatus);
        _treeProxyModel->invalidate();
    }

    void mTreeView::sortTree(int sortWay, Qt::SortOrder sortType)
    {
        _treeProxyModel->setSortMode((mTreeProxyModel::SortMode)sortWay, sortType);
    }

    void mTreeView::slot_updateMeshStatus()
    {
        //获取线、面、体网格
        QList<QStringList> lists;
        auto LineDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(LineMesh);
        const int lineSize = LineDatas.size();
        for (int i = 0; i < lineSize; ++i)
        {
            LineDatas[i]->setStaticsPic(Tree_Yes);
            if (LineDatas[i]->_list.isEmpty())
            {
                LineDatas[i]->setStaticsPic(Tree_Failure); continue;
            }
            for (int j = 0; j < lineSize; ++j)
            {
                if (i == j)
                    continue;
                bool covered = true;
                for (const QString& b : LineDatas[i]->_list)
                {
                    if (!LineDatas[j]->_list.contains(b))
                    {
                        covered = false; break;
                    }
                }
                bool same = QSet<QString>::fromList(LineDatas[i]->_list)
                    == QSet<QString>::fromList(LineDatas[j]->_list);
                if (same)
                {
                    if (i < j)
                        LineDatas[i]->setStaticsPic(Tree_Wrong);
                    else
                        LineDatas[j]->setStaticsPic(Tree_Wrong);
                }
                else if (covered)
                {
                    LineDatas[i]->setStaticsPic(Tree_Wrong);
                }
            }
        }
        auto FaceDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(FaceMesh);
        const int FaceSize = FaceDatas.size();
        for (int i = 0; i < FaceSize; ++i)
        {
            FaceDatas[i]->setStaticsPic(Tree_Yes);
            if (FaceDatas[i]->_list.isEmpty())
            {
                FaceDatas[i]->setStaticsPic(Tree_Failure); continue;
            }
            for (int j = 0; j < FaceSize; ++j)
            {
                if (i == j)
                    continue;
                bool covered = true;
                for (const QString& b : FaceDatas[i]->_list)
                {
                    if (!FaceDatas[j]->_list.contains(b))
                    {
                        covered = false; break;
                    }
                }
                bool same = QSet<QString>::fromList(FaceDatas[i]->_list)
                    == QSet<QString>::fromList(FaceDatas[j]->_list);
                if (same)
                {
                    if (i < j)
                        FaceDatas[i]->setStaticsPic(Tree_Wrong);
                    else
                        FaceDatas[j]->setStaticsPic(Tree_Wrong);
                }
                else if (covered)
                {
                    FaceDatas[i]->setStaticsPic(Tree_Wrong);
                }
            }
        }
        auto SolidDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(SolidMesh);
        const int SolidSize = SolidDatas.size();
        for (int i = 0; i < SolidSize; ++i)
        {
            SolidDatas[i]->setStaticsPic(Tree_Yes);
            if (SolidDatas[i]->_list.isEmpty())
            {
                SolidDatas[i]->setStaticsPic(Tree_Failure); continue;
            }
            for (int j = 0; j < SolidSize; ++j)
            {
                if (i == j)
                    continue;
                bool covered = true;
                for (const QString& b : SolidDatas[i]->_list)
                {
                    if (!SolidDatas[j]->_list.contains(b))
                    {
                        covered = false; break;
                    }
                }
                bool same = QSet<QString>::fromList(SolidDatas[i]->_list)
                    == QSet<QString>::fromList(SolidDatas[j]->_list);
                if (same)
                {
                    if (i < j)
                        SolidDatas[i]->setStaticsPic(Tree_Wrong);
                    else
                        SolidDatas[j]->setStaticsPic(Tree_Wrong);
                }
                else if (covered)
                {
                    SolidDatas[i]->setStaticsPic(Tree_Wrong);
                }

            }
        }


        auto LocalDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(MeshPanelType::LocalSize);
        const int LocalSize = LocalDatas.size();
        for (int i = 0; i < LocalSize; ++i)
        {
            LocalDatas[i]->setStaticsPic(Tree_Yes);
            if (LocalDatas[i]->_list.isEmpty())
            {
                LocalDatas[i]->setStaticsPic(Tree_Failure); continue;
            }
            for (int j = 0; j < LocalSize; ++j)
            {
                if (i == j)
                    continue;
                bool covered = true;
                for (const QString& b : LocalDatas[i]->_list)
                {
                    if (!LocalDatas[j]->_list.contains(b))
                    {
                        covered = false; break;
                    }
                }
                bool same = QSet<QString>::fromList(LocalDatas[i]->_list)
                    == QSet<QString>::fromList(LocalDatas[j]->_list);
                if (same)
                {
                    if (i < j)
                        LocalDatas[i]->setStaticsPic(Tree_Wrong);
                    else
                        LocalDatas[j]->setStaticsPic(Tree_Wrong);
                }
                else if (covered)
                {
                    LocalDatas[i]->setStaticsPic(Tree_Wrong);
                }
            }
        }
        auto AutoAdaptDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(MeshPanelType::AutoAdapt);
        const int AutoAdaptSize = AutoAdaptDatas.size();
        for (int i = 0; i < AutoAdaptSize; ++i)
        {
            AutoAdaptDatas[i]->setStaticsPic(Tree_Yes);
            if (AutoAdaptDatas[i]->_list.isEmpty())
            {
                AutoAdaptDatas[i]->setStaticsPic(Tree_Failure); continue;
            }
            for (int j = 0; j < AutoAdaptSize; ++j)
            {
                if (i == j)
                    continue;
                bool covered = true;
                for (const QString& b : AutoAdaptDatas[i]->_list)
                {
                    if (!AutoAdaptDatas[j]->_list.contains(b))
                    {
                        covered = false; break;
                    }
                }
                bool same = QSet<QString>::fromList(AutoAdaptDatas[i]->_list)
                    == QSet<QString>::fromList(AutoAdaptDatas[j]->_list);
                if (same)
                {
                    if (i < j)
                        AutoAdaptDatas[i]->setStaticsPic(Tree_Wrong);
                    else
                        AutoAdaptDatas[j]->setStaticsPic(Tree_Wrong);
                }
                else if (covered)
                {
                    AutoAdaptDatas[i]->setStaticsPic(Tree_Wrong);
                }
            }
        }
        auto EdgeSuppressDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(EdgeSuppress);
        const int EdgeSuppressSize = EdgeSuppressDatas.size();
        for (int i = 0; i < EdgeSuppressSize; ++i)
        {
            EdgeSuppressDatas[i]->setStaticsPic(Tree_Yes);
            if (EdgeSuppressDatas[i]->_list.isEmpty())
            {
                EdgeSuppressDatas[i]->setStaticsPic(Tree_Failure); continue;
            }
            for (int j = 0; j < EdgeSuppressSize; ++j)
            {
                if (i == j)
                    continue;
                bool covered = true;
                for (const QString& b : EdgeSuppressDatas[i]->_list)
                {
                    if (!EdgeSuppressDatas[j]->_list.contains(b))
                    {
                        covered = false; break;
                    }
                }
                bool same = QSet<QString>::fromList(EdgeSuppressDatas[i]->_list)
                    == QSet<QString>::fromList(EdgeSuppressDatas[j]->_list);
                if (same)
                {
                    if (i < j)
                        EdgeSuppressDatas[i]->setStaticsPic(Tree_Wrong);
                    else
                        EdgeSuppressDatas[j]->setStaticsPic(Tree_Wrong);
                }
                else if (covered)
                {
                    EdgeSuppressDatas[i]->setStaticsPic(Tree_Wrong);
                }
            }
        }
        auto datas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelData();        // 只展开当前层
        if (datas.empty())
            return;

        auto panerentItem = _treeModel->getParentItem(MeshParentItem);
        if (!panerentItem)
            return;
        BrowserTreeStatus parentStatus = Tree_Yes;

        for (auto item : panerentItem->getChildrenItem())
        {
            if (getTextByItem(item) == "全局尺寸")
                break;
        }
        for (int i = 0; i < datas.size(); ++i)
        {
            auto item = _treeModel->getChildrenItem(MeshChildItem, datas[i]->getMeshPanelId());
            if (!item)
                continue;
            BrowserTreeStatus childStatus = datas[i]->getStatusPic();
            setTreeItemStatus(item, childStatus);

            if (childStatus == Tree_Failure)
                parentStatus = Tree_Failure; // 有问号父节点也变问号
            else if (childStatus == Tree_Update && parentStatus != Tree_Failure)
                parentStatus = Tree_Update;
        }
        setTreeItemStatus(panerentItem, parentStatus);
    }

    void mTreeView::slot_updateMeshList(std::map<int, std::set<int>> map)
    {
        std::set<int> SolidIds = map[3];
        std::set<int> FaceIds = map[2];
        std::set<int> LineIds = map[1];
        //遍历所有子节点
        auto LineDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(LineMesh);
        const int lineSize = LineDatas.size();
        for (int i = 0; i < lineSize; ++i)
        {
            for (auto id : LineIds)
            {
                LineDatas[i]->_list.removeAll(QString("几何线_%1").arg(id));
            }
        }
        auto FaceDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(FaceMesh);
        const int FaceSize = FaceDatas.size();
        for (int i = 0; i < FaceSize; ++i)
        {
            for (auto id : FaceIds)
            {
                FaceDatas[i]->_list.removeAll(QString("几何面_%1").arg(id));
            }
        }
        auto SolidDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(SolidMesh);
        const int SolidSize = SolidDatas.size();
        for (int i = 0; i < SolidSize; ++i)
        {
            for (auto id : SolidIds)
            {
                SolidDatas[i]->_list.removeAll(QString("实体_%1").arg(id));
            }
        }
        auto LocalDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(MeshPanelType::LocalSize);
        const int LocalSize = LocalDatas.size();
        for (int i = 0; i < LocalSize; ++i)
        {
            for (auto id : LineIds)
            {
                LocalDatas[i]->_list.removeAll(QString("几何线_%1").arg(id));
            }
        }
        auto AutoAdaptDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(MeshPanelType::AutoAdapt);
        const int AutoAdaptSize = AutoAdaptDatas.size();
        //几何面，几何线，几何体
        for (int i = 0; i < AutoAdaptSize; ++i)
        {
            int dim = AutoAdaptDatas[i]->getMeshGeo();
            if (dim == 0)
            {
                for (auto id : FaceIds)
                {
                    AutoAdaptDatas[i]->_list.removeAll(QString("几何面_%1").arg(id));
                }
            }
            else if (dim == 1)
            {
                for (auto id : LineIds)
                {
                    AutoAdaptDatas[i]->_list.removeAll(QString("几何线_%1").arg(id));
                }
            }
            else if (dim == 2)
            {
                for (auto id : SolidIds)
                {
                    AutoAdaptDatas[i]->_list.removeAll(QString("实体_%1").arg(id));
                }
            }
        }
        auto EdgeSuppressDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(EdgeSuppress);
        const int EdgeSuppressSize = EdgeSuppressDatas.size();
        for (int i = 0; i < EdgeSuppressSize; ++i)
        {
            for (auto id : LineIds)
            {
                EdgeSuppressDatas[i]->_list.removeAll(QString("几何线_%1").arg(id));
            }
        }

    }

    void mTreeView::slot_updateLoadBoundaryTreeView(int caseId, int loadBoundaryId, int way)
    {
        //先判断添加还是删除
        if (way == 0) //添加
        {
            //然后判断是载荷还是边界
            auto loadBoundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadBoundaryId);
            if (!loadBoundaryData) return;
            if (loadBoundaryData->isLoadOrBoundary() == 0) //载荷
            {
                if (caseId == -1) //添加到载荷管理树
                {
                    mTreeItem* _loadManagerItem = _treeModel->getParentItem(LoadManagerParentItem);
                    if (_loadManagerItem == nullptr)
                    {
                        return;
                    }
                    auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseLoadChildItem, loadBoundaryData->getLoadBoundaryName(), loadBoundaryId);
                    _treeModel->appendChildren(_loadManagerItem, childitem);
                    getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, loadBoundaryId);
                    return; //caseid=-1不往下进行
                }

                //添加到工况树载荷
                mTreeItem* _loadItem = _treeModel->getParentItem(CaseLoadParentItem, caseId);
                if (_loadItem == nullptr)
                {
                    return;
                }
                auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseLoadChildItem, loadBoundaryData->getLoadBoundaryName(), loadBoundaryId);
                childitem->setCaseID(caseId);
                _treeModel->appendChildren(_loadItem, childitem);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, loadBoundaryId);
            }
            else if (loadBoundaryData->isLoadOrBoundary() == 1)//边界
            {
                if (caseId == -1) //添加到边界管理树
                {
                    mTreeItem* _boundaryManagerItem = _treeModel->getParentItem(BoundaryManagerParentItem);
                    if (_boundaryManagerItem == nullptr)
                    {
                        return;
                    }
                    auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseBCChildItem, loadBoundaryData->getLoadBoundaryName(), loadBoundaryId);
                    _treeModel->appendChildren(_boundaryManagerItem, childitem);
                    getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, loadBoundaryId);
                    return; //caseid = -1不往下走
                }

                //边界数据挂载到工况下
                mTreeItem* _loadItem = _treeModel->getParentItem(CaseBCParentItem, caseId);
                if (_loadItem == nullptr)
                {
                    return;
                }
                auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseBCChildItem, loadBoundaryData->getLoadBoundaryName(), loadBoundaryId);
                childitem->setCaseID(caseId);
                _treeModel->appendChildren(_loadItem, childitem);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, loadBoundaryId);
            }            
        }
        else if (way == 1) //删除
        {
            auto loadBoundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadBoundaryId);
            if (!loadBoundaryData) return;
            if (caseId == -1) //删除管理器下节点
            {
                if (loadBoundaryData->isLoadOrBoundary() == 0) //载荷
                {
                    mTreeItem* item = _treeModel->getChildrenItem(CaseLoadChildItem, loadBoundaryId, -1);
                    QModelIndex idx = _treeModel->indexFromItem(item);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                    removeProxyIndexs({ proxyIndex });
                    getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_LoadBoundary, { loadBoundaryId });
                    return;
                }
                else if (loadBoundaryData->isLoadOrBoundary() == 1) //边界
                {
                    mTreeItem* item = _treeModel->getChildrenItem(CaseBCChildItem, loadBoundaryId, -1);
                    QModelIndex idx = _treeModel->indexFromItem(item);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                    removeProxyIndexs({ proxyIndex });
                    getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_LoadBoundary, { loadBoundaryId });
                    return;
                }
            }

            BrowserTreeType type;
            if (loadBoundaryData->isLoadOrBoundary() == 0) //载荷
            {
                type = CaseLoadChildItem;
            }
            else if (loadBoundaryData->isLoadOrBoundary() == 1) //边界
            {
                type = CaseBCChildItem;
            }
            mTreeItem* item = _treeModel->getChildrenItem(type, loadBoundaryId, caseId);
            QModelIndex idx = _treeModel->indexFromItem(item);
            QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
            removeProxyIndexs({ proxyIndex });
        }
    }

    void mTreeView::slot_updatePredefinedFieldTreeView(int caseId, int fieldID, int way)
    {
        //先判断添加还是删除
        if (way == 0) //添加
        {
            if (caseId == -1) //添加到预定义场管理树
            {
                mTreeItem* _fieldManagerItem = _treeModel->getParentItem(PredefinedFieldManagerParentItem);
                if (_fieldManagerItem == nullptr)
                {
                    auto rootItem = _treeModel->getParentItem(ModelRootItem);
                    _fieldManagerItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), PredefinedFieldManagerParentItem, "预定义场");
                    _treeModel->insertChildren(rootItem, _fieldManagerItem, getInsertLocation(PredefinedFieldManagerParentItem));
                }
                auto fieldData = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(fieldID);
                if (!fieldData) return;
                auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CasePredefinedFieldChildItem, fieldData->getPredefinedFieldName(), fieldID);
                _treeModel->appendChildren(_fieldManagerItem, childitem);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PredefinedField, fieldID);
                return; //caseid=-1不往下进行
            }

            //添加到工况树预定义场
            mTreeItem* _fieldItem = _treeModel->getParentItem(CasePredefinedFieldParentItem, caseId);
            if (_fieldItem == nullptr)
            {
                return;
            }
            auto fieldData = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(fieldID);
            if (!fieldData) return;
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CasePredefinedFieldChildItem, fieldData->getPredefinedFieldName(), fieldID);
            childitem->setCaseID(caseId);
            _treeModel->appendChildren(_fieldItem, childitem);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PredefinedField, fieldID);
        }
        else if (way == 1) //删除
        {
            if (caseId == -1) //删除管理器下节点
            {
                mTreeItem* item = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldID, -1);
                QModelIndex idx = _treeModel->indexFromItem(item);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                removeProxyIndexs({ proxyIndex });
                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PredefinedField, { fieldID });
                return;
            }

            mTreeItem* item = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldID, caseId);
            QModelIndex idx = _treeModel->indexFromItem(item);
            QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
            removeProxyIndexs({ proxyIndex });
        }
    }

    void mTreeView::slot_updateContactTreeView(int caseId, int fieldID, int way)
    {
        //先判断添加还是删除
        if (way == 0) //添加
        {
            if (caseId == -1) //添加到接触管理树
            {
                mTreeItem* _contactManagerItem = _treeModel->getParentItem(ContactParentItem);
                if (_contactManagerItem == nullptr)
                {
                    return;
                }
                auto contactData = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getContactData(fieldID);
                if (!contactData) return;
                auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), ContactChildItem, contactData->getContactName(), fieldID);
                _treeModel->appendChildren(_contactManagerItem, childitem);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, fieldID);
                return; //caseid=-1不往下进行
            }

            //添加到工况树相互作用下直接作为子节点（过盈接触）
            mTreeItem* _interactionItem = _treeModel->getParentItem(CaseInteractionParentItem, caseId);
            if (_interactionItem == nullptr)
            {
                return;
            }
            auto contactData = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getContactData(fieldID);
            if (!contactData) return;
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseInteractionChildItem, contactData->getContactName(), fieldID);
            childitem->setCaseID(caseId);
            _treeModel->appendChildren(_interactionItem, childitem);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, fieldID);
        }
        else if (way == 1) //删除
        {
            if (caseId == -1) //删除管理器下节点
            {
                mTreeItem* item = _treeModel->getChildrenItem(ContactChildItem, fieldID, -1);
                QModelIndex idx = _treeModel->indexFromItem(item);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                removeProxyIndexs({ proxyIndex });
                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Contact, { fieldID });
                return;
            }

            //删除工况树下节点（直接在相互作用下查找）
            mTreeItem* _interactionItem = _treeModel->getParentItem(CaseInteractionParentItem, caseId);
            if (_interactionItem == nullptr)
            {
                return;
            }
            QVector<mTreeItem*> children = _interactionItem->getChildrenItem();
            for (auto child : children)
            {
                if (getPhyIdByItem(child) == fieldID)
                {
                    QModelIndex idx = _treeModel->indexFromItem(child);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                    removeProxyIndexs({ proxyIndex });
                    return;
                }
            }
        }
    }

    void mTreeView::slot_updateModelFixTreeView(int caseId, int modelFixID, int way)
    {
        // 先判断添加还是删除
        if (way == 0) // 添加
        {
            auto modelFixData = getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(modelFixID);
            if (!modelFixData) return;

            // 统一取当前数据状态作为初始状态，避免管理树与工况树初始状态不一致
            QVector<QPair<QString, QString>> info;
            BrowserTreeStatus initStatus = getCVM->getModelData()->getDataStatus(info, Physical_ModelFix, modelFixID);

            if (caseId == -1) // 添加到模型修改管理树
            {
                mTreeItem* _modelFixManagerItem = _treeModel->getParentItem(ModelFixParentItem);
                if (_modelFixManagerItem == nullptr)
                {
                    return;
                }
                auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), ModelFixChildItem, modelFixData->getModelFixName(), modelFixID, initStatus);
                _treeModel->appendChildren(_modelFixManagerItem, childitem);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModelFix, modelFixID);
                return; // caseid=-1 不往下进行
            }

            // 添加到工况树模型修改节点下
            mTreeItem* _caseModelFixItem = _treeModel->getParentItem(CaseModelFixManagerItem, caseId);
            if (_caseModelFixItem == nullptr)
            {
                return;
            }
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseModelFixChildItem, modelFixData->getModelFixName(), modelFixID, initStatus);
            childitem->setCaseID(caseId);
            _treeModel->appendChildren(_caseModelFixItem, childitem);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModelFix, modelFixID);
        }
        else if (way == 1) // 删除
        {
            if (caseId == -1) // 删除管理器下节点
            {
                mTreeItem* item = _treeModel->getChildrenItem(ModelFixChildItem, modelFixID, -1);
                QModelIndex idx = _treeModel->indexFromItem(item);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                removeProxyIndexs({ proxyIndex });
                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModelFix, { modelFixID });
                return;
            }

            // 删除工况树模型修改节点下子节点
            mTreeItem* _caseModelFixItem = _treeModel->getParentItem(CaseModelFixManagerItem, caseId);
            if (_caseModelFixItem == nullptr)
            {
                return;
            }
            QVector<mTreeItem*> children = _caseModelFixItem->getChildrenItem();
            for (auto child : children)
            {
                if (getPhyIdByItem(child) == modelFixID)
                {
                    QModelIndex idx = _treeModel->indexFromItem(child);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                    removeProxyIndexs({ proxyIndex });
                    return;
                }
            }
        }
    }

    void mTreeView::deleteSketchCompound()
    {

        slot_menuDeleteGeo();
    }

    mItemKey mTreeView::getParentKey(mItemKey childKey)
    {
       QModelIndex idx = _treeModel->indexFromKey(childKey);
       if (idx.isValid())
       {
           return _treeModel->itemKeyFromIndex(idx.parent());
       }
        return mItemKey();
    }

    QRect mTreeView::branchHitRect(const QModelIndex& index) const
    {
        if (!index.isValid()) return {};

        // 通常只有第0列绘制/响应展开箭头
        if (index.column() != 0) return {};

        // 这些开关会影响是否画箭头
        if (!rootIsDecorated()) return {};
        if (!itemsExpandable()) return {};

        // 没孩子 => 没箭头
        if (!model() || !model()->hasChildren(index)) return {};

        const QRect vr = visualRect(index);
        if (!vr.isValid()) return {};

        int depth = 0;
        for (QModelIndex p = index.parent(); p.isValid(); p = p.parent()) ++depth;

        const int indent = indentation();
        int s = style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, this);
        if (s <= 0) s = 16;

        const int blockLeft = depth * indent;
        // 与 PaintDashBranches 箭头位置同口径：根层箭头右缘对齐内容左缘-2；
        // 其余层箭头中心对齐父行状态符号列（父内容左缘+8），命中区同中心
        int x = (depth == 0) ? (blockLeft + indent - 2 - s) : (blockLeft + 8 - s / 2);
        if (x < 0) x = 0;

        const int y = vr.center().y() - s / 2;
        QRect r(x, y, s, s);
        return r.intersected(vr);
    }

    void mTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
    {
        // 层级虚线导引线 + 展开箭头（原生箭头经离屏渲染后右缘贴近节点内容，实现见 PaintDashBranches）。
        // 原生 drawBranches 只经由回调离屏调用：直接画会居中在 indent 区间，与节点距离偏远
        PaintDashBranches(this, painter, rect, index,
            [this](QPainter* p, const QRect& r, const QModelIndex& i) { QTreeView::drawBranches(p, r, i); });
    }

    void mTreeView::initBrowserTree()
    {
        setStyleSheet(R"(
QTreeView{
border:none;
}



)");
        QPalette pal = this->palette();
        pal.setColor(QPalette::Highlight, Qt::white);        // 选中背景变白
        this->setPalette(pal);

        //多体形态："部件"分组（ModelParentItem）挂多体根首位、模型根工厂跳过部件。
        //标志必须随构造传入——mTreeModel 构造内即建树，事后设置无效
        _treeModel = new mTreeModel(getCVM->getModelData()->getModelDataID(),
            MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion);
        _treeProxyModel = new mTreeProxyModel;
        _treeProxyModel->setSourceModel(_treeModel);
        _treeProxyModel->setDynamicSortFilter(true);

        setModel(_treeProxyModel);
        setItemDelegate(new mTreeDelegate(this));
        setUniformRowHeights(true);
        // 缩进与 PaintDashBranches 箭头口径配套：箭头对齐父行状态符号列、且距自身内容约2px
        setIndentation(14);
        setAnimated(false);
        setHeaderHidden(true);
        setContextMenuPolicy(Qt::CustomContextMenu);
        setEditTriggers(QAbstractItemView::NoEditTriggers);

        createGlobalCoordinate();
        //启动默认展开当前形态的顶层根（selectedItem 展开根节点显示一级分类）：
        //多体形态下模型根整树隐藏，改为展开多体动力学顶层根
        selectedItem(MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion
            ? _treeModel->getParentItem(Motion_RootItem)
            : _treeModel->getParentItem(ModelRootItem));

        //按当前视口刷新前处理树可见性与树窗口显隐
        updateTreeSectionsForViewport();

        m_osgPressTimer = new QTimer(this);
    }

    void mTreeView::initConnect()
    {
         connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
         connect(this, &QTreeView::customContextMenuRequested, this, [&]() {
                 slot_rightClickItem();
              });

         //视口/模型切换前联动：跨模型切换时旧视口按退出面板语义收尾符号显示。
         //DirectConnection：必须在管理器切换当前模型指针前同步执行（此时 getCVM 仍为旧模型）
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::viewportModelAboutToChange,
             this, &mTreeView::slot_viewportModelAboutToChange, Qt::DirectConnection);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::clikNodesOnTree, this, &mTreeView::slot_singleClickItem);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateSetSig, this, &mTreeView::slot_updateSetTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::deleteSetsSig, this, &mTreeView::slot_deleteSets);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::deleteLoadsSig, this, &mTreeView::slot_deleteLoads);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::deleteBoundarysSig, this, &mTreeView::slot_deleteBoundarys);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::deletePredefinedFieldsSig, this, &mTreeView::slot_deletePredefinedFields);

         //左键"集合"根节点（phyID=0，类型分组节点不触发）：打开集合管理器。
         //使用clicked信号仅响应真实用户点击，程序化选中（如删除后选中"集合"）不弹面板
         connect(this, &QTreeView::clicked, this, [this](const QModelIndex& proxyIdx) {
                 if (!proxyIdx.isValid())
                 {
                     return;
                 }
                 if (getTreeTypeByModelIndex(proxyIdx) == SetParentItem && getPhyIdByModelIndex(proxyIdx) == 0)
                 {
                     mInfoWindow::getInstance()->showSetManager();
                 }
             });
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateCoordinateSig, this, &mTreeView::slot_updateCoordinateTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::batchCreateContactSig, this, &mTreeView::slot_batchCreateContactTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateContactSig, this, &mTreeView::slot_updateContactTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateModelFixSig, this, &mTreeView::slot_updateModelFixTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateSectionShapeSig, this, &mTreeView::slot_updateSectionShapeTree);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateFunctionSig, this, &mTreeView::slot_updateFunctionTree);
         
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateConnectorSig, this, &mTreeView::slot_updateConnectorTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateConnectorDeputySig, this, &mTreeView::slot_updateDeputyTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateConnectorName, this, &mTreeView::slot_updateConnChildItemName);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateCaseSig, this, &mTreeView::slot_updateCaseTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::caseRotorSettingChangedSig, this, &mTreeView::slot_updateRotorParentItem);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateLoadSig, this, &mTreeView::slot_updateLoadOnCaseTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateBoundarySig, this, &mTreeView::slot_updateBoundaryOnCaseTree);
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateRotateAnalyseSig, this, &mTreeView::slot_updateRotateAnalyseOnCaseTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdatePredefinedFieldSig, this, &mTreeView::slot_updatePredefinedFieldOnCaseTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMaterialSig, this, &mTreeView::slot_updateMatTree);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateOptimizationSig, this, &mTreeView::slot_updateOptimizationTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateOptimizationDesignSig, this, &mTreeView::slot_updateOptimizationDesignTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateOptimizationConstraintSig, this, &mTreeView::slot_updateOptimizationConstraintTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateRunnerOptimizationSig, this, &mTreeView::slot_updateRunnerOptimizationTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateRunnerOptimizationDesignSig, this, &mTreeView::slot_updateRunnerOptimizationDesignTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateRunnerOptimizationConstraintSig, this, &mTreeView::slot_updateRunnerOptimizationConstraintTree);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateGenericParamSig, this, &mTreeView::slot_updateGenericParamTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateGenericAssignSig, this, &mTreeView::slot_updateGenericAssignTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateGenericMatrixSig, this, &mTreeView::slot_updateGenericMatrixTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateGenericPartSig, this, &mTreeView::slot_updateGenericPartTree);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateSolveControlSig, this, &mTreeView::slot_updateSolveControlTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateGlobalOutputControlSig, this, &mTreeView::slot_updateGlobalOutputControlTree);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateASCouplingSearchSig, this, &mTreeView::slot_updateASCouplingSearchTree);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdatePropertySig, this, &mTreeView::slot_updatePropertyTree);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateNonStructuralQualitySig, this, &mTreeView::slot_updateNonStructuralQualityTree);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateFieldSig, this, &mTreeView::slot_updateFieldTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdatePeriodicBoundarySig, this, &mTreeView::slot_updatePeriodicBoundaryTree);


         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionBodySig, this, &mTreeView::slot_updateMotionBodyTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionBodyBatchSig, this, &mTreeView::slot_updateMotionBodyTreeBatch);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionJointSig, this, &mTreeView::slot_updateMotionJointTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionCouplingSig, this, &mTreeView::slot_updateMotionCouplingTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionDriveSig, this, &mTreeView::slot_updateMotionDriveTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionMarkerSig, this, &mTreeView::slot_updateMotionMarkerTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionVectorForceSig, this, &mTreeView::slot_updateMotionVectorForceTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionContactSig, this, &mTreeView::slot_updateMotionContactTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionSpringSig, this, &mTreeView::slot_updateMotionSpringTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionScenarioSig, this, &mTreeView::slot_updateMotionScenarioTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionControlInputSig, this, &mTreeView::slot_updateMotionControlInputTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionElectroImportSig, this, &mTreeView::slot_updateMotionElectroImportTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionElectroExportSig, this, &mTreeView::slot_updateMotionElectroExportTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionHydraulicSig, this, &mTreeView::slot_updateMotionHydraulicTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionRLSig, this, &mTreeView::slot_updateMotionRLTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionSensorSig, this, &mTreeView::slot_updateMotionSensorTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionControllerSig, this, &mTreeView::slot_updateMotionControllerTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMotionAutoFlexSig, this, &mTreeView::slot_updateMotionAutoFlexTree);
         connect(mGlobalSignals::getInstance(), SIGNAL(updateModelTree()), this, SLOT(slot_updateModelTree()));

         connect(mGlobalSignals::getInstance(), &mGlobalSignals::fastCapture, [=](const QString& path) {
             //模型快照
             getCVM->modelFastCapture(path, "");
             });

         connect(mGlobalSignals::getInstance(), &mGlobalSignals::createUpdatePartSig, this, &mTreeView::slot_updatePartTree);
         connect(mGlobalSignals::getInstance(), &mGlobalSignals::createUpdatePartSig_2, this, &mTreeView::slot_updatePartTree_2);
         connect(mGlobalSignals::getInstance(), &mGlobalSignals::deletePartSig, this, &mTreeView::slot_deletePartTree);
         connect(mGlobalSignals::getInstance(), &mGlobalSignals::updateAssemblyTreeSig, this, &mTreeView::slot_updateAssemblyTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateGlobalDimSig, this, &mTreeView::slot_updateGlobalDimTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMeshSig, this, &mTreeView::slot_updateMeshTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateMeshOptSig, this, &mTreeView::slot_updateMeshOptTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::signal_osgWidgetEvent, this, &mTreeView::slot_signal_osgWidgetEvent);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateBrowserTreeStatus, this, &mTreeView::slot_updateBrowserTreeStatus);

         connect(m_osgPressTimer, &QTimer::timeout, this, &mTreeView::longPressDetected);
         m_viewportChangeConn = connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mTreeView::slot_subWindowActivated);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateMeshStatus, this, &mTreeView::slot_updateMeshStatus);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateMeshList, this, &mTreeView::slot_updateMeshList);
         connect(getCVM->getPreRenderWidget(), &CCSMRendWidget::appendOneGeoModelRenderFinished, this, &mTreeView::slot_appendOneGeoModelRenderFinished);

         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updataLoadBoundaryTreeView, this, &mTreeView::slot_updateLoadBoundaryTreeView);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updatePredefinedFieldTreeView, this, &mTreeView::slot_updatePredefinedFieldTreeView);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateContactTreeView, this, &mTreeView::slot_updateContactTreeView);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateModelFixTreeView, this, &mTreeView::slot_updateModelFixTreeView);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateFreqSamplPointTree, this, &mTreeView::slot_updateFreqSamplPointTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateLaminateTree, this, &mTreeView::slot_updateLaminateTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateContriSig, this, &mTreeView::slot_updateContriTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdatePeakFilterSig, this, &mTreeView::slot_updatePeakFilterTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createUpdateCasePeakFilterSig, this, &mTreeView::slot_updateCasePeakFilterTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::deleteGeoTree, this, &mTreeView::slot_deleteGeoTree);
         connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::importSuccessful,this, &mTreeView::slot_importSuccessful);
    }

    void mTreeView::hightPart()
    {
        vector<int> partID;
        for (auto& idx : getSelectIndex())
        {
            if (getTreeTypeByModelIndex(idx) == ModelChildItem)
            {
                partID.push_back(getPhyIdByModelIndex(idx));
            }
        }
        addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), partID);
        UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->updateRender();
    }

    void mTreeView::setSymbolShow()
    {
        if (getCVM->isPostModel() || !getCVM->getPreRenderWidget())
            return;

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        //获取自定义加密的包围盒
        map<QString, bool> symbolState;
        auto meshDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelDataByType(CustomEncryption);
        for (auto meshData : meshDatas)
        {
            for (auto renderlabel : meshData->_Renderlabel)
            {
               bool showstate= symbolController->getOrCreateSymbolData<SymbolDescription>(renderlabel)->_showState;
               symbolState[renderlabel] = showstate;
            }                
        }

        symbolController->setAllSymbolShowState(false);
        vector<QString> showSymbol;
        QModelIndexList modelIndexs = getSelectIndex();
        for (auto& idx : modelIndexs)
        {
            switch (getTreeTypeByModelIndex(idx))
            {
            case CoordinateParentItem:
            {
                const auto& idstrs = getCVM->getModelData()->getManager<mCoordinateManager>(Physical_Coordinate).getAllCoorinateIDStr();
                showSymbol.insert(showSymbol.end(), idstrs.begin(), idstrs.end());
                break;
            }
            case CoordinateChildItem:
            {
                showSymbol.push_back(QString("coordinate_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case CaseLoadParentItem:
            {
                //获取所属caseId
                int caseId = getPhyIdByModelIndex(idx.parent());
                std::set<int> allLoad = getCVM->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_Case ,caseId }, Physical_LoadBoundary);
                for (auto id : allLoad)
                {
                    showSymbol.push_back(QString("load_%1").arg(id));
                }
                break;
            }
            case CaseLoadChildItem:
            {
                showSymbol.push_back(QString("load_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case CaseBCParentItem:
            {
                //获取所属caseId
                int caseId = getPhyIdByModelIndex(idx.parent());
                std::set<int> allBC = getCVM->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_Case ,caseId }, Physical_LoadBoundary);
                for (auto id : allBC)
                {
                    showSymbol.push_back(QString("boundary_%1").arg(id));
                }
                break;
            }
            case CaseBCChildItem:
            {
                showSymbol.push_back(QString("boundary_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case CasePredefinedFieldParentItem:
            {
                //获取所属caseId
                int caseId = getPhyIdByModelIndex(idx.parent());
                std::set<int> allField = getCVM->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_Case ,caseId }, Physical_PredefinedField);
                for (auto id : allField)
                {
                    showSymbol.push_back(QString("predefinedfield_%1").arg(id));
                }
                break;
            }
            case CasePredefinedFieldChildItem:
            {
                showSymbol.push_back(QString("predefinedfield_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case LoadManagerParentItem:
            {
                //获取当前节点item
                mTreeItem* loadManagerItem = getTreeItemByIndex(idx);
                //获取子节点
                QVector<mTreeItem*> childItems = loadManagerItem->getChildrenItem();
                for (auto childitem : childItems)
                {
                    int loadId = getPhyIdByItem(childitem);
                    showSymbol.push_back(QString("load_%1").arg(loadId));
                }
            }
            break;
            case BoundaryManagerParentItem:
            {
                //获取当前节点item
                mTreeItem* loadManagerItem = getTreeItemByIndex(idx);
                //获取子节点
                QVector<mTreeItem*> childItems = loadManagerItem->getChildrenItem();
                for (auto childitem : childItems)
                {
                    int boundaryId = getPhyIdByItem(childitem);
                    showSymbol.push_back(QString("boundary_%1").arg(boundaryId));
                }

            }
            break;
            case PredefinedFieldManagerParentItem:
            {
                //获取当前节点item
                mTreeItem* fieldManagerItem = getTreeItemByIndex(idx);
                //获取子节点
                QVector<mTreeItem*> childItems = fieldManagerItem->getChildrenItem();
                for (auto childitem : childItems)
                {
                    int fieldId = getPhyIdByItem(childitem);
                    showSymbol.push_back(QString("predefinedfield_%1").arg(fieldId));
                }

            }
            break;
            case InteractionParentItem:
            case ConnectorParentItem:
            {
                const auto& ids = getCVM->getModelData()->getManager<mConnectorManager>(Physical_Connector).getAllConnectorID();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("connector_%1").arg(id));
                }
                break;
            }
            case DeputyParentItem:
            {
                const auto& ids = getCVM->getModelData()->getManager<mDeputyManager>(Physical_Deputy).getAllDeputyID();
                for (auto id : ids)
                {
                    QString baseSymbol = QString("deputy_%1").arg(id);
                    if (symbolController->isExistSymbol(baseSymbol))
                        showSymbol.push_back(baseSymbol);
                }
                break;
            }
            case ConnectorChildItem:
            {
                showSymbol.push_back(QString("connector_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case DeputyChildItem:
            {
                QString baseSymbol = QString("deputy_%1").arg(getPhyIdByModelIndex(idx));
                if (symbolController->isExistSymbol(baseSymbol))
                    showSymbol.push_back(baseSymbol);
                break;
            }
            case Motion_ConnectorParentItem:
            {
                auto& ids = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getAllIDs();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("motionJoint_%1").arg(id));
                }
                break;
            }
            case Motion_ConnectorChildItem:
            {
                showSymbol.push_back(QString("motionJoint_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case Motion_SpringParentItem:
            {
                auto& ids = getCVM->getModelData()->getManager<mMotionSpringManager>(Physical_MotionSpring).getAllIDs();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("motionSpring_%1").arg(id));
                }
                break;
            }
            case Motion_SpringChildItem:
            {
                showSymbol.push_back(QString("motionSpring_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case Motion_DriveParentItem:
            {
                auto& driveManager = getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive);
                for (auto id : driveManager.getAllIDs())
                {
                    auto* drive = dynamic_cast<mMotionJointDrive*>(driveManager.getData(id));
                    int tj = mMotionRefMap::get(Physical_MotionDrive, id, Physical_MotionJoint);
                    if (drive && tj >= 0)
                        showSymbol.push_back(QString("motionJoint_%1").arg(tj));
                }
                break;
            }
            case Motion_DriveChildItem:
            {
                int driveId = getPhyIdByModelIndex(idx);
                auto* drive = dynamic_cast<mMotionJointDrive*>(getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive).getData(driveId));
                int tj = mMotionRefMap::get(Physical_MotionDrive, driveId, Physical_MotionJoint);
                if (drive && tj >= 0)
                    showSymbol.push_back(QString("motionJoint_%1").arg(tj));
                break;
            }
            case Motion_CouplingParentItem:
            {
                auto& couplingManager = getCVM->getModelData()->getManager<mMotionCouplingManager>(Physical_MotionCoupling);
                for (auto id : couplingManager.getAllIDs())
                {
                    auto* coupling = couplingManager.getData(id);
                    if (!coupling) continue;
                    int firstId = mMotionRefMap::get(Physical_MotionCoupling, coupling->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint);
                    int secondId = mMotionRefMap::get(Physical_MotionCoupling, coupling->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint);
                    if (firstId >= 0)
                        showSymbol.push_back(QString("motionJoint_%1").arg(firstId));
                    if (secondId >= 0)
                        showSymbol.push_back(QString("motionJoint_%1").arg(secondId));
                    showSymbol.push_back(QString("motionCoupling_%1").arg(id));
                }
                break;
            }
            case Motion_CouplingChildItem:
            {
                int couplingId = getPhyIdByModelIndex(idx);
                auto* coupling = getCVM->getModelData()->getManager<mMotionCouplingManager>(Physical_MotionCoupling).getData(couplingId);
                if (coupling)
                {
                    int firstId = mMotionRefMap::get(Physical_MotionCoupling, coupling->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint);
                    int secondId = mMotionRefMap::get(Physical_MotionCoupling, coupling->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint);
                    if (firstId >= 0)
                        showSymbol.push_back(QString("motionJoint_%1").arg(firstId));
                    if (secondId >= 0)
                        showSymbol.push_back(QString("motionJoint_%1").arg(secondId));
                    showSymbol.push_back(QString("motionCoupling_%1").arg(couplingId));
                }
                break;
            }
            case Motion_LiYuanParentItem:
            {
                auto& ids = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getAllIDs();
                for (auto id : ids)
                {
                    mMotionForceElement* fe = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getData(id);
                    if (fe) {
                        if (fe && fe->getLoadType() == VectorLoadType::ScalarForce) {
                            showSymbol.push_back(QString("scalarForce_%1").arg(id));
                        }
                        else if (fe && fe->getLoadType() == VectorLoadType::ScalarTorque) {
                            showSymbol.push_back(QString("scalarTorque_%1").arg(id));
                        }
                        else if (fe && fe->getLoadType() == VectorLoadType::TriForce) {
                            showSymbol.push_back(QString("threePointForce_%1").arg(id));
                        }
                        else if (fe && fe->getLoadType() == VectorLoadType::TriTorque) {
                            showSymbol.push_back(QString("threePointTorque_%1").arg(id));
                        }
                        else {
                            showSymbol.push_back(QString("vectorForce_%1").arg(id));
                        }
                    }
                }
                break;
            }
            case Motion_LiYuanChildItem:
            {
                int feId = getPhyIdByModelIndex(idx);
                mMotionForceElement* fe = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getData(feId);
                if (fe) {
                    if (fe && fe->getLoadType() == VectorLoadType::ScalarForce) {
                        showSymbol.push_back(QString("scalarForce_%1").arg(feId));
                    }
                    else if (fe && fe->getLoadType() == VectorLoadType::ScalarTorque) {
                        showSymbol.push_back(QString("scalarTorque_%1").arg(feId));
                    }
                    else if (fe && fe->getLoadType() == VectorLoadType::TriForce) {
                        showSymbol.push_back(QString("threePointForce_%1").arg(feId));
                    }
                    else if (fe && fe->getLoadType() == VectorLoadType::TriTorque) {
                        showSymbol.push_back(QString("threePointTorque_%1").arg(feId));
                    }
                    else {
                        showSymbol.push_back(QString("vectorForce_%1").arg(feId));
                    }
                }
                break;
            }
            case Motion_AnimationChildItem:
            {
                auto& ids = getCVM->getModelData()->getManager<mMotionSpringManager>(Physical_MotionSpring).getAllIDs();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("motionSpring_%1").arg(getPhyIdByModelIndex(idx)));
                }
                break;
            }
            case MeshChildItem:
            {
                for (auto pair : symbolState)
                {
                    std::vector<QString> vec{ pair.first };
                    symbolController->setSymbolShowState(vec, pair.second);
                }             
                break;
            }
            case MeshParentItem:
            {
              
      
            }
            default:
                break;
            }
        }

        symbolController->setSymbolShowState(showSymbol, true);

        // 应用新显隐后可见符号集与点击前快照一致（如预定义场父节点→其唯一
        // 子节点，或选中项与之前显示的符号完全相同）时，已编译 GL 数据仍然
        // 有效：跳过 UpdateAllSymbol 全量重建（它会重建所有可见符号，大模型下代价很高）
        std::set<QString> afterVisible;
        for (const auto& s : symbolController->getAllSymbols())
            if (s.second && s.second->_showState)
                afterVisible.insert(QString::fromStdString(s.first));
        if (afterVisible != _preClickVisibleSymbols)
        {
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mTreeView::deleteSymbol()
    {
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        vector<QString> showSymbol;
        QModelIndexList modelIndexs = getSelectIndex();
        for (auto& idx : modelIndexs)
        {
            switch (getTreeTypeByModelIndex(idx))
            {
            case CoordinateParentItem:
            {
                const auto& idstrs = getCVM->getModelData()->getManager<mCoordinateManager>(Physical_Coordinate).getAllCoorinateIDStr();
                showSymbol.insert(showSymbol.end(), idstrs.begin(), idstrs.end());
                break;
            }
            case CoordinateChildItem:
            {
                showSymbol.push_back(QString("coordinate_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case LoadManagerParentItem:
            {
                const auto& ids = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadID();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("load_%1").arg(id));
                }
                break;
            }
            case CaseLoadChildItem:
            {
                int caseID = getCaseIdByModelIndex(idx);
                if(caseID == -1) showSymbol.push_back(QString("load_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case BoundaryManagerParentItem:
            {
                const auto& ids = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getAllBoundaryID();
                for (auto id : ids)
                {
                    auto names = AllRenderSymbolManager::getBoundarySymbolNames(id);
                    showSymbol.insert(showSymbol.end(), names.begin(), names.end());
                }
                break;
            }
            case PredefinedFieldManagerParentItem:
            {
                mTreeItem* fieldManagerItem = getTreeItemByIndex(idx);
                QVector<mTreeItem*> childItems = fieldManagerItem->getChildrenItem();
                for (auto childitem : childItems)
                {
                    int fieldId = getPhyIdByItem(childitem);
                    showSymbol.push_back(QString("predefinedfield_%1").arg(fieldId));
                }
                break;
            }
            case CasePredefinedFieldParentItem:
            {
                mTreeItem* fieldParentItem = getTreeItemByIndex(idx);
                QVector<mTreeItem*> childItems = fieldParentItem->getChildrenItem();
                for (auto childitem : childItems)
                {
                    int fieldId = getPhyIdByItem(childitem);
                    showSymbol.push_back(QString("predefinedfield_%1").arg(fieldId));
                }
                break;
            }
            case CaseBCChildItem:
            {
                int caseID = getCaseIdByModelIndex(idx);
                if (caseID == -1)
                {
                    auto names = AllRenderSymbolManager::getBoundarySymbolNames(getPhyIdByModelIndex(idx));
                    showSymbol.insert(showSymbol.end(), names.begin(), names.end());
                }
                break;
            }
            case ConnectorParentItem:
            {
                const auto& ids = getCVM->getModelData()->getManager<mConnectorManager>(Physical_Connector).getAllConnectorID();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("connector_%1").arg(id));
                }
                break;
            }
            case DeputyParentItem:
            {
                const auto& ids = getCVM->getModelData()->getManager<mDeputyManager>(Physical_Deputy).getAllDeputyID();
                for (auto id : ids)
                {
                    QString baseSymbol = QString("deputy_%1").arg(id);
                    if (symbolController->isExistSymbol(baseSymbol))
                        showSymbol.push_back(baseSymbol);
                }
                break;
            }
            case ConnectorChildItem:
            {
                showSymbol.push_back(QString("connector_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case DeputyChildItem:
            {
                QString baseSymbol = QString("deputy_%1").arg(getPhyIdByModelIndex(idx));
                if (symbolController->isExistSymbol(baseSymbol))
                    showSymbol.push_back(baseSymbol);
                break;
            }
            case ContactParentItem:
            {
                const auto& ids = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getAllContactID();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("contact_%1").arg(id));
                }
                break;
            }
            case ContactChildItem:
            {
                showSymbol.push_back(QString("contact_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case Motion_ConnectorParentItem:
            {
                auto& ids = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getAllIDs();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("motionJoint_%1").arg(id));
                }
                break;
            }
            case Motion_ConnectorChildItem:
            {
                showSymbol.push_back(QString("motionJoint_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case Motion_SpringParentItem:
            {
                auto& ids = getCVM->getModelData()->getManager<mMotionSpringManager>(Physical_MotionSpring).getAllIDs();
                for (auto id : ids)
                {
                    showSymbol.push_back(QString("motionSpring_%1").arg(id));
                }
                break;
            }
            case Motion_SpringChildItem:
            {
                showSymbol.push_back(QString("motionSpring_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            case Motion_DriveParentItem:
            {
                //驱动自身无符号，显隐其目标运动副符号（与 setSymbolShow 保持一致）
                auto& ids = getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive).getAllIDs();
                for (auto id : ids)
                {
                    auto* drive = dynamic_cast<mMotionJointDrive*>(
                        getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive).getData(id));
                    int tj = mMotionRefMap::get(Physical_MotionDrive, id, Physical_MotionJoint);
                    if (drive && tj >= 0)
                        showSymbol.push_back(QString("motionJoint_%1").arg(tj));
                }
                break;
            }
            case Motion_DriveChildItem:
            {
                int driveId = getPhyIdByModelIndex(idx);
                auto* drive = dynamic_cast<mMotionJointDrive*>(
                    getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive).getData(driveId));
                int tj = mMotionRefMap::get(Physical_MotionDrive, driveId, Physical_MotionJoint);
                if (drive && tj >= 0)
                    showSymbol.push_back(QString("motionJoint_%1").arg(tj));
                break;
            }
            case Motion_LiYuanParentItem:
            {
                auto& ids = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getAllIDs();
                for (auto id : ids)
                {
                    mMotionForceElement* fe = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getData(id);
                    if (fe) {
                        if (fe && fe->getLoadType() == VectorLoadType::ScalarForce) {
                            showSymbol.push_back(QString("scalarForce_%1").arg(id));
                        }
                        else if (fe && fe->getLoadType() == VectorLoadType::ScalarTorque) {
                            showSymbol.push_back(QString("scalarTorque_%1").arg(id));
                        }
                        else if (fe && fe->getLoadType() == VectorLoadType::TriForce) {
                            showSymbol.push_back(QString("threePointForce_%1").arg(id));
                        }
                        else if (fe && fe->getLoadType() == VectorLoadType::TriTorque) {
                            showSymbol.push_back(QString("threePointTorque_%1").arg(id));
                        }
                        else {
                            showSymbol.push_back(QString("vectorForce_%1").arg(id));
                        }
                    }
                }
                break;
            }
            case Motion_LiYuanChildItem:
            {
                int feId = getPhyIdByModelIndex(idx);
                mMotionForceElement* fe = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getData(feId);
                if (fe) {
                    if (fe && fe->getLoadType() == VectorLoadType::ScalarForce) {
                        showSymbol.push_back(QString("scalarForce_%1").arg(feId));
                    }
                    else if (fe && fe->getLoadType() == VectorLoadType::ScalarTorque) {
                        showSymbol.push_back(QString("scalarTorque_%1").arg(feId));
                    }
                    else if (fe && fe->getLoadType() == VectorLoadType::TriForce) {
                        showSymbol.push_back(QString("threePointForce_%1").arg(feId));
                    }
                    else if (fe && fe->getLoadType() == VectorLoadType::TriTorque) {
                        showSymbol.push_back(QString("threePointTorque_%1").arg(feId));
                    }
                    else {
                        showSymbol.push_back(QString("vectorForce_%1").arg(feId));
                    }
                }
                break;
            }
            case Motion_CouplingParentItem:
            {
                auto& couplingManager = getCVM->getModelData()->getManager<mMotionCouplingManager>(Physical_MotionCoupling);
                for (auto id : couplingManager.getAllIDs())
                {
                    showSymbol.push_back(QString("motionCoupling_%1").arg(id));
                }
                break;
            }
            case Motion_CouplingChildItem:
            {
                showSymbol.push_back(QString("motionCoupling_%1").arg(getPhyIdByModelIndex(idx)));
                break;
            }
            default:
                break;
            }
        }

        if (showSymbol.size() == 0)
            return;

        symbolController->deleteSymbol(showSymbol);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mTreeView::clearCurrentModelData()
    {
        //清空当前模型：数据/视口/模型树全部还原为按原名称新建的空模型状态
        //（打开工程文件前、右键"清空"共用；本方法不含确认弹窗，由调用方决定是否询问）
        if (getCVM == nullptr || getCVM->getModelData() == nullptr)
        {
            return;
        }
        int modelID = getCVM->getModelData()->getModelDataID();
        //网格剖分全局单例持有按旧模型几何 ID 记录的 MeshEntity/MeshData/尺寸映射，
        //模型销毁重建后这些状态指向悬空对象或错误实体，需在清空模型前复位
        GlobalMeshMessage::getInstance()->resetModelScopedState();
        //记录原模型单位制（清空是销毁重建新 mModelData，原单位制会随旧对象销毁，
        //需在重建后恢复到新模型底层数据中，保持清空前后单位制一致）
        mUnitSystemManager* oldUnitMgr = getCVM->getModelData()->getUnitSystemManager();
        const QVector<int> curUnitsIdx = oldUnitMgr->getCurUnitsIndex();
        const QVector<int> hisUnitsIdx = oldUnitMgr->getHisUnitsIndex();
        //先记录原模型第一个视口的"视图页位置"（页ID+页内行号）：
        //必须在删除前取快照，删除后节点已移除、剩余节点重排，位置会失真
        int anchorPageID = -1;
        int anchorRow = -1;
        if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
        {
            const QList<int> vpIDs = getCVM->getAllViewportIDs();
            if (!vpIDs.isEmpty())
            {
                anchorPageID = viewTree->pageOfViewport(vpIDs.first());
                anchorRow = viewTree->rowOfViewport(vpIDs.first());
            }
        }
        //清空模型：先按原名称就地重建新模型（含一个新视口），再删除旧模型
        const int newModelID = VMManager->clearViewportModel(modelID);
        if (newModelID < 0)
        {
            return;
        }
        //把原模型单位制恢复到新模型底层数据（不触发换算与信号），
        //并同步界面单位制显示（clearViewportModel 内的新视口激活发生在恢复之前，
        //其触发的 UI 同步读到的是默认单位制，需按恢复后的单位索引重发）
        if (curUnitsIdx.size() == 4 && hisUnitsIdx.size() == 4)
        {
            if (auto* newViewModel = VMManager->getViewportModel(newModelID))
            {
                newViewModel->getModelData()->getUnitSystemManager()->setUnitsIndex(curUnitsIdx, hisUnitsIdx);
            }
            emit mMechGlobalSignals::getInstance()->unitSystemChanged(curUnitsIdx[0]);
            emit mMechGlobalSignals::getInstance()->angleUnitChanged(curUnitsIdx[1]);
            emit mMechGlobalSignals::getInstance()->speedUnitChanged(curUnitsIdx[2]);
            emit mMechGlobalSignals::getInstance()->tempUnitChanged(curUnitsIdx[3]);
            mInfoWindow::getInstance()->updateAllPanelUnits();
        }
        //把新视口迁回原模型第一个视口所在的视图页位置（原模型多视口时对齐第一个视口）
        if (anchorPageID >= 0)
        {
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
            {
                const int newViewportID = VMManager->currentViewportID();
                if (newViewportID >= 0)
                {
                    viewTree->moveViewportToPageRow(newViewportID, anchorPageID, qMax(anchorRow, 0));
                    //节点是迁移重建的（原节点已删除，原激活态随之丢失），
                    //迁移后补齐激活表现：目标节点加粗并程序化选中
                    viewTree->syncActiveViewportNode(newViewportID);
                }
            }
        }
        auto item = _treeModel->getChildrenItem(ModelRootItem, 0, 0, modelID);
        _treeModel->clearModelTree(item, newModelID);
        createGlobalCoordinate();
        //模型树"仅显示当前模型"用视图级行隐藏实现：节点换绑新模型ID后，
        //旧的行隐藏状态不会自动复位，需按新当前模型重新刷新显隐
        updateTreeSectionsForViewport();

        // mpcomm 平台会话中，清空模型后通知平台清空单元格状态
        emit MechCommon::mMechGlobalSignals::getInstance()->mpcommModelCleared();
    }


    void mTreeView::createGlobalCoordinate()
    {
        //创建全局坐标系
        mCoordinateBase* coor = getCVM->getModelData()->getManager<mCoordinateManager>(Physical_Coordinate).getCoordinateData(-1);
        if (coor == nullptr)
            return;

        mTreeItem* _coordinateItem = _treeModel->getParentItem(CoordinateParentItem);
        mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CoordinateChildItem, coor->getCoordinateName(), -1);
        _treeModel->appendChildren(_coordinateItem, childitem);

        //headless 批量模式下无渲染视口，坐标系符号创建跳过
        if (MechPreData::mViewportModelManager::isHeadlessMode())
            return;

        auto* preRenderWidget = getCVM->getPreRenderWidget();
        if (preRenderWidget == nullptr)
            return;

        auto symbolController = preRenderWidget->GetSymbolController();
        const auto& arrow3 = symbolController->getOrCreateSymbolData<Arrow_Font_Axis_Implicit>(QString("coordinate_%1").arg(-1));
        arrow3->_color_x = { 255,0,0 };
        arrow3->_color_y = { 0,255,0 };
        arrow3->_color_z = { 0,0,255 };
        arrow3->_point_origin = QVector3D(0, 0, 0);
        arrow3->_depthState = false;
        arrow3->_vec_x = QVector3D(1, 0, 0);
        arrow3->_vec_xy = QVector3D(0, 1, 0);
        arrow3->_type = LocalAxisType_Rectangular;
        arrow3->_showState = false;
        symbolController->createSymbol(arrow3);
    }

    void mTreeView::selectedItem(MechPreData::mTreeItem* item)
    {
        //多体形态：模型根整树隐藏，各处隐式流程收尾仍会选中模型根——重定向到多体根，
        //避免隐藏节点持焦（右键空白处按当前选中节点弹菜单，会带出隐式菜单）
        if (item && MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion
            && item == _treeModel->getParentItem(ModelRootItem))
        {
            item = _treeModel->getParentItem(Motion_RootItem);
        }
        QModelIndex idx = _treeModel->indexFromItem(item);
        QModelIndex proxyIdx = _treeProxyModel->mapFromSource(idx);

        if (proxyIdx.isValid())
        {
            this->expandItem(proxyIdx);
            this->scrollTo(proxyIdx, QAbstractItemView::PositionAtCenter);
            if (getSelectIndex().size() == 1 && getSelectIndex()[0] == proxyIdx)
            {
                slot_itemSelectionChanged(QItemSelection(), QItemSelection());
            }
            else
            {
                auto* sm = this->selectionModel();
                sm->setCurrentIndex(proxyIdx, QItemSelectionModel::NoUpdate);
                sm->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
        }
    }

    void mTreeView::selectLoadManagerLoadItem(int loadID)
    {
        //选中载荷管理器树下指定载荷节点（CaseLoadChildItem且父节点为载荷管理器）
        mTreeItem* loadItem = _treeModel->getChildrenItem(CaseLoadChildItem, loadID, -1);
        if (loadItem != nullptr)
        {
            selectedItem(loadItem);
        }
    }

    void mTreeView::selectCaseLoadItem(int caseID, int loadID)
    {
        //选中指定工况下载荷父节点下的指定载荷节点（CaseLoadChildItem且caseID匹配）
        mTreeItem* loadItem = _treeModel->getChildrenItem(CaseLoadChildItem, loadID, caseID);
        if (loadItem != nullptr)
        {
            selectedItem(loadItem);
        }
    }

    void mTreeView::addCaseLoadChildItem(int caseID, int loadID)
    {
        //工况不支持载荷时父节点不存在，直接返回
        mTreeItem* caseLoadItem = _treeModel->getParentItem(CaseLoadParentItem, caseID);
        if (caseLoadItem == nullptr)
        {
            return;
        }
        //已存在则跳过（全量刷新等场景会重复调用）
        if (_treeModel->getChildrenItem(CaseLoadChildItem, loadID, caseID) != nullptr)
        {
            return;
        }
        auto loadData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadID);
        if (loadData == nullptr)
        {
            return;
        }
        auto* childItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseLoadChildItem, loadData->getLoadBoundaryName(), loadID);
        childItem->setCaseID(caseID);
        _treeModel->appendChildren(caseLoadItem, childItem);
    }

    void mTreeView::removeCaseLoadChildItem(int caseID, int loadID)
    {
        mTreeItem* childItem = _treeModel->getChildrenItem(CaseLoadChildItem, loadID, caseID);
        if (childItem != nullptr)
        {
            removeTreeItem(childItem);
        }
    }

    void mTreeView::selectBoundaryManagerBoundaryItem(int boundaryID)
    {
        //选中边界管理器树下指定边界节点（CaseBCChildItem且父节点为边界管理器）
        mTreeItem* boundaryItem = _treeModel->getChildrenItem(CaseBCChildItem, boundaryID, -1);
        if (boundaryItem != nullptr)
        {
            selectedItem(boundaryItem);
        }
    }

    void mTreeView::selectCaseBoundaryItem(int caseID, int boundaryID)
    {
        //选中指定工况下边界父节点下的指定边界节点（CaseBCChildItem且caseID匹配）
        mTreeItem* boundaryItem = _treeModel->getChildrenItem(CaseBCChildItem, boundaryID, caseID);
        if (boundaryItem != nullptr)
        {
            selectedItem(boundaryItem);
        }
    }

    void mTreeView::addCaseBoundaryChildItem(int caseID, int boundaryID)
    {
        //工况不支持边界时父节点不存在，直接返回
        mTreeItem* caseBCItem = _treeModel->getParentItem(CaseBCParentItem, caseID);
        if (caseBCItem == nullptr)
        {
            return;
        }
        //已存在则跳过（全量刷新等场景会重复调用）
        if (_treeModel->getChildrenItem(CaseBCChildItem, boundaryID, caseID) != nullptr)
        {
            return;
        }
        auto boundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(boundaryID);
        if (boundaryData == nullptr)
        {
            return;
        }
        auto* childItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseBCChildItem, boundaryData->getLoadBoundaryName(), boundaryID);
        childItem->setCaseID(caseID);
        _treeModel->appendChildren(caseBCItem, childItem);
    }

    void mTreeView::removeCaseBoundaryChildItem(int caseID, int boundaryID)
    {
        mTreeItem* childItem = _treeModel->getChildrenItem(CaseBCChildItem, boundaryID, caseID);
        if (childItem != nullptr)
        {
            removeTreeItem(childItem);
        }
    }

    void mTreeView::selectPredefinedFieldManagerItem(int fieldID)
    {
        //选中预定义场管理器树下指定预定义场节点（CasePredefinedFieldChildItem且父节点为预定义场管理器）
        mTreeItem* fieldItem = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldID, -1);
        if (fieldItem != nullptr)
        {
            selectedItem(fieldItem);
        }
    }

    void mTreeView::selectCasePredefinedFieldItem(int caseID, int fieldID)
    {
        //选中指定工况下预定义场父节点下的指定预定义场节点（CasePredefinedFieldChildItem且caseID匹配）
        mTreeItem* fieldItem = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldID, caseID);
        if (fieldItem != nullptr)
        {
            selectedItem(fieldItem);
        }
    }

    void mTreeView::addCasePredefinedFieldChildItem(int caseID, int fieldID)
    {
        //工况不支持预定义场时父节点不存在，直接返回
        mTreeItem* caseFieldItem = _treeModel->getParentItem(CasePredefinedFieldParentItem, caseID);
        if (caseFieldItem == nullptr)
        {
            return;
        }
        //已存在则跳过（全量刷新等场景会重复调用）
        if (_treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldID, caseID) != nullptr)
        {
            return;
        }
        auto fieldData = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(fieldID);
        if (fieldData == nullptr)
        {
            return;
        }
        auto* childItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CasePredefinedFieldChildItem, fieldData->getPredefinedFieldName(), fieldID);
        childItem->setCaseID(caseID);
        _treeModel->appendChildren(caseFieldItem, childItem);
    }

    void mTreeView::removeCasePredefinedFieldChildItem(int caseID, int fieldID)
    {
        mTreeItem* childItem = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldID, caseID);
        if (childItem != nullptr)
        {
            removeTreeItem(childItem);
        }
    }

    void mTreeView::selectedItem(QVector<MechPreData::mTreeItem*>& items)
    {
        QModelIndexList idxs;
        QModelIndex idx;
        for (auto item : items)
        {
            idx = _treeModel->indexFromItem(item);
            idxs.push_back(_treeProxyModel->mapFromSource(idx));
        }

        selectedItem(idxs);
    }

    void mTreeView::selectedItem(QModelIndexList proxyIdx)
    {
        if (proxyIdx.isEmpty()) return;

        QItemSelection sel;
        for (const auto& idx : proxyIdx)
        {
            if (!idx.isValid()) continue;
            sel.select(idx, idx); // 单点也可以
        }

        // Qt 在选区未变化时不会发出 selectionChanged，此时与单条目分支一样
        // 手动补一次刷新，保证属性面板更新逻辑仍会执行。
        const QModelIndexList before = getSelectIndex();
        selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        if (before == getSelectIndex())
        {
            slot_itemSelectionChanged(QItemSelection(), QItemSelection());
        }
    }

    void mTreeView::slot_itemSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        //视口模型已删除或未就绪时直接返回：函数内 getCVM 解引用会崩溃
        if (getCVM == nullptr)
        {
            return;
        }
        mInfoWindow::getInstance()->unEditInfoWindow();
        QModelIndexList modelIndexs = getSelectIndex();
        if (modelIndexs.size() == 1)
        {
            updateModelView(modelIndexs[0]);
        }
        else
        {
            int currentModelID = getCVM->getModelData()->getModelDataID();
            QItemSelection deselectSelection;
            for (QModelIndex idx : selected.indexes())
            {
                if (getModelIdByModelIndex(idx) != currentModelID)
                    deselectSelection.select(idx, idx);
            }

            if (deselectSelection.size() > 0)
            {
                selectionModel()->select(deselectSelection, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
                return;
            }
        }

        // 本函数入口的 unEditInfoWindow 已做过完整退出清理；下面 slot_singleClickItem
        // 内 show*Manager/show*Reference 会再次调用 unEditInfoWindow——短路那一次，
        // 避免 clearPick/高亮清理/表格复位在同一次点击内双份执行
        mInfoWindow::getInstance()->suppressUneditOnce();
        slot_singleClickItem();
        // 链路结束复位（本次点击内未消费也要复位，避免误伤下一次真实调用）
        mInfoWindow::getInstance()->clearUneditSuppression();
    }

    void mTreeView::updateModelView(QModelIndex proxyIndex)
    {
        int modelDataID = _treeModel->data(toSourceIndex(proxyIndex), Roles::ModelRole).toInt();
        BrowserTreeType treeType = (BrowserTreeType)_treeModel->data(toSourceIndex(proxyIndex), Roles::TypeRole).toInt();
        if (modelDataID != getCVM->getModelData()->getModelDataID())
        {
            VMManager->setCurrentViewportModel(modelDataID);
            _treeModel->setCurrentModelID(modelDataID);
            //目标模型已删除时切换结果为空，继续访问会崩溃
            if (getCVM == nullptr)
            {
                return;
            }
            bool isPost = getCVM->isPostModel();
            if (!isPost)
            {
                mViewTreeFunction::activatePreViewport();
            }
            else
            {
                mViewTreeFunction::activatePostViewport();
            }
        }
        else if (treeType == ResultParentItem)
        {
            return;
        }
        else if (VMManager->getCurrentViewportPrivate() && treeType < PostTreeRootItem && VMManager->getCurrentViewportPrivate()->toType<mxr::REBUILD_PostRend>() != nullptr)
        {
            //与前分支保持一致：经门面激活，避免激活期间树信号未被断开保护
            mViewTreeFunction::activatePreViewport();
        }
    }

    //多体对象树类型 → 对应多体功能区页名（原"多体动力学"单页已按原型拆分为六个常驻页）。
    //隐式形态下六页非常驻，switchMenuBarWindow 按动态页签位规则显示，两侧形态共用本映射。
    static QString motionTabNameForType(MxFunctions::BrowserTreeType type)
    {
        switch (type)
        {
        case MxFunctions::Motion_RootItem:
        case MxFunctions::Motion_MovingBodyParentItem:
        case MxFunctions::Motion_MovingBodyChildItem:
        case MxFunctions::Motion_AutoFlexParentItem:
        case MxFunctions::Motion_AutoFlexChildItem:
        case MxFunctions::Motion_MarkerParentItem:
        case MxFunctions::Motion_MarkerChildItem:
            return "体定义";
        case MxFunctions::Motion_ConnectorParentItem:
        case MxFunctions::Motion_ConnectorChildItem:
        case MxFunctions::Motion_CouplingParentItem:
        case MxFunctions::Motion_CouplingChildItem:
            return "运动副与约束";
        case MxFunctions::Motion_DriveParentItem:
        case MxFunctions::Motion_DriveChildItem:
            return "驱动";
        case MxFunctions::Motion_SpringParentItem:
        case MxFunctions::Motion_SpringChildItem:
        case MxFunctions::Motion_LiYuanParentItem:
        case MxFunctions::Motion_LiYuanChildItem:
        case MxFunctions::Motion_ContactParentItem:
        case MxFunctions::Motion_ContactChildItem:
            return "连接与力元";
        case MxFunctions::Motion_ScenarioParentItem:
        case MxFunctions::Motion_AnimationChildItem:
        case MxFunctions::Motion_LoadExportChildItem:
        case MxFunctions::Motion_CurveChildItem:
        case MxFunctions::Motion_ChartChildItem:
            return "求解";
        case MxFunctions::Motion_SensorParentItem:
        case MxFunctions::Motion_SensorChildItem:
        case MxFunctions::Motion_ControllerParentItem:
        case MxFunctions::Motion_ControllerChildItem:
        case MxFunctions::Motion_ControlInputParentItem:
        case MxFunctions::Motion_ControlInputChildItem:
        case MxFunctions::Motion_ElectroImportParentItem:
        case MxFunctions::Motion_ElectroImportChildItem:
        case MxFunctions::Motion_ElectroExportParentItem:
        case MxFunctions::Motion_ElectroExportChildItem:
        case MxFunctions::Motion_HydraulicParentItem:
        case MxFunctions::Motion_HydraulicChildItem:
        case MxFunctions::Motion_RLParentItem:
        case MxFunctions::Motion_RLChildItem:
            return "传感与控制";
        default:
            return "体定义";
        }
    }

    void mTreeView::slot_singleClickItem()
    {
        //视口模型已删除（如删除模型后）或未就绪时直接返回：分支内 getCVM 解引用会崩溃
        if (getCVM == nullptr)
        {
            return;
        }

        QModelIndexList modelIndexs = getSelectIndex();
        // 拍摄当前可见符号名快照（在任何分支创建/删除符号之前），
        // 供末尾 setSymbolShow 判断本次选中变化是否真的改变了符号可见性
        _preClickVisibleSymbols.clear();
        if (getCVM && !getCVM->isPostModel() && getCVM->getPreRenderWidget())
        {
            if (auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController())
            {
                for (const auto& s : symbolController->getAllSymbols())
                    if (s.second && s.second->_showState)
                        _preClickVisibleSymbols.insert(QString::fromStdString(s.first));
            }
        }
        if (modelIndexs.size() == 1)
        {
            QString name;
            BrowserTreeType type = getTreeTypeByModelIndex(modelIndexs[0]);
            int phyID = getPhyIdByModelIndex(modelIndexs[0]);
            emit MechCommon::mMechGlobalSignals::getInstance()->disableAllMaterialProps(true);
            switch (type)
            {
            case MxFunctions::ModelRootItem:
                name = "模型";
                break;
            case MxFunctions::ConnectorChildItem:
            {
                name = "相互作用";
                mInfoWindow::getInstance()->editConnector(phyID);
                auto _loadBoundaryData = getCVM->getModelData()->getManager<mConnectorManager>(Physical_Connector).getConnectorData(phyID);
                auto render = AllRenderSymbolManager::getInstance();
                render->showConnector(_loadBoundaryData);
            }
                break;
            case MxFunctions::DeputyChildItem:
            {
                name = "相互作用";
                mInfoWindow::getInstance()->editDeputy(phyID);
                auto _deputyData = getCVM->getModelData()->getManager<mDeputyManager>(Physical_Deputy).getDeputyData(phyID);
                auto render = AllRenderSymbolManager::getInstance();
                render->showDeputy(_deputyData);
            }
                break;
            case MxFunctions::ModelFixChildItem:
            {
                name = "相互作用";
                mInfoWindow::getInstance()->editModelFix(phyID);
            }
            break;
            case MxFunctions::ContactChildItem:
            {
                name = "相互作用";
                mInfoWindow::getInstance()->editContact(phyID);
            }
                break;
            case MxFunctions::PeriodicBoundaryChildItem:
            {
                name = "相互作用";
                mInfoWindow::getInstance()->editPeriodicBoundary(phyID);
                auto render = AllRenderSymbolManager::getInstance();
                if (render) render->rendPeriodicBoundary(phyID);
            }
                break;
            case MxFunctions::CaseInteractionChildItem:
            {
                name = "相互作用";
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent());
                mInfoWindow::getInstance()->editContact(phyID);
            }
                break;
            case MxFunctions::CaseInteractionParentItem:
            {
                //打开接触引用面板（节点phyId即所属工况id）
                mInfoWindow::getInstance()->showContactReference(phyID);
                name = "相互作用";
            }
                break;
            case MxFunctions::InteractionParentItem:
            {
                {//连接进入此处
                    MechConnectorData::mConnectorManager& manager = getCVM->getModelData()->getManager<mConnectorManager>(Physical_Connector);
                    auto render = AllRenderSymbolManager::getInstance();
                    std::vector<int> allIds = manager.getAllConnectorID();
                    for (auto id : allIds)
                    {
                        auto Data = manager.getConnectorData(id);
                        render->rendConnector(Data, true);
                    }
                    render->rendConnector(QVector<int>::fromStdVector(allIds));
                }

                {//连接副进入此处
                    MechConnectorData::mDeputyManager& manager = getCVM->getModelData()->getManager<mDeputyManager>(Physical_Deputy);
                    auto render = AllRenderSymbolManager::getInstance();
                    std::vector<int> allIds = manager.getAllDeputyID();
                    for (auto id : allIds)
                    {
                        auto _deputyData = manager.getDeputyData(id);
                        render->rendDeputy(_deputyData, true);
                    }
                }

                {//接触
                    std::vector<int> allIds = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getAllContactID();
                    auto render = AllRenderSymbolManager::getInstance();
                    render->rendContact(QVector<int>::fromStdVector(allIds));
                }
                name = "相互作用";
            }
            break;
            case MxFunctions::ConnectorParentItem:
            {
                //连接进入此处
                MechConnectorData::mConnectorManager& manager = getCVM->getModelData()->getManager<mConnectorManager>(Physical_Connector);
                auto render = AllRenderSymbolManager::getInstance();
                std::vector<int> allIds = manager.getAllConnectorID();
                for (auto id : allIds)
                {
                    auto Data = manager.getConnectorData(id);
                    render->rendConnector(Data, true);
                }
                render->rendConnector(QVector<int>::fromStdVector(allIds));
                name = "相互作用";
            }
            break;
            case MxFunctions::DeputyParentItem:
            {
                //连接副进入此处
                MechConnectorData::mDeputyManager& manager = getCVM->getModelData()->getManager<mDeputyManager>(Physical_Deputy);
                auto render = AllRenderSymbolManager::getInstance();
                std::vector<int> allIds = manager.getAllDeputyID();
                for (auto id : allIds)
                {
                    auto _deputyData = manager.getDeputyData(id);
                    render->rendDeputy(_deputyData, true);
                }
                name = "相互作用";
            }
            break;
            case MxFunctions::ContactParentItem:
            {
                //接触进入此处（管理器面板内嵌矩阵，Table dock矩阵已移除）
                mInfoWindow::getInstance()->showContactManager();
                std::vector<int> allIds = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getAllContactID();
                auto render = AllRenderSymbolManager::getInstance();
                render->rendContact(QVector<int>::fromStdVector(allIds));
                name = "相互作用";
            }
            break;
            case MxFunctions::PeriodicBoundaryParentItem:
            {
                //周期性边界进入此处
                mTreeItem* parentItem = getTreeItemByIndex(modelIndexs[0]);
                QVector<mTreeItem*> childItems = parentItem->getChildrenItem();
                QVector<int> phyIds;
                for (auto child : childItems)
                {
                    int phyID = getPhyIdByItem(child);
                    phyIds.push_back(phyID);
                }
                auto render = AllRenderSymbolManager::getInstance();
                if (render) render->rendPeriodicBoundary(phyIds);
                name = "相互作用";
            }
            break;
            case MxFunctions::MaterialChildItem:
                mInfoWindow::getInstance()->editMaterial(phyID);
            case MxFunctions::MaterialParentItem:
                name = "材料";
                break;
            case MxFunctions::OptimizationParentItem:
                name = "优化";
                mInfoWindow::getInstance()->editOptimization();
                break;
            case MxFunctions::OptimizationGoalParentItem:
                name = "优化";
                mInfoWindow::getInstance()->editOptimizationGoal();
                break;
            case MxFunctions::OptimizationDesignChildItem:
                mInfoWindow::getInstance()->editOptimizationDesign(phyID);
            case MxFunctions::OptimizationDesignParentItem:
                name = "优化";
                break;
            case MxFunctions::OptimizationConstraintsChildItem:
                mInfoWindow::getInstance()->editOptimizationConstraint(phyID);
            case MxFunctions::OptimizationConstraintsParentItem:
                name = "优化";
                break;
            case MxFunctions::RunnerOptimizationParentItem:
                name = "流道优化";
                mInfoWindow::getInstance()->editRunnerOptimization();
                break;
            case MxFunctions::RunnerOptimizationGoalParentItem:
                name = "流道优化";
                mInfoWindow::getInstance()->editRunnerOptimizationGoal();
                break;
            case MxFunctions::RunnerOptimizationDesignChildItem:
                mInfoWindow::getInstance()->editRunnerOptimizationDesign(phyID);
            case MxFunctions::RunnerOptimizationDesignParentItem:
                name = "流道优化";
                break;
            case MxFunctions::RunnerOptimizationConstraintsChildItem:
                mInfoWindow::getInstance()->editRunnerOptimizationConstraint(phyID);
            case MxFunctions::RunnerOptimizationConstraintsParentItem:
                name = "流道优化";
                break;
            case MxFunctions::SetChildItem:
                mInfoWindow::getInstance()->editSet(phyID);
            case MxFunctions::SetParentItem:
                name = "集合";
                break;
            case MxFunctions::SglLayerPlateParentItem:
            case MxFunctions::LamPlateParentItem:
                name = "截面属性";
                break;
            case MxFunctions::SglLayerPlateChildItem:
            case MxFunctions::LamPlateChildItem:
            {
                mInfoWindow::getInstance()->editLaminate(phyID);
                name = "属性";
            }
                 break;
            case MxFunctions::PropertyChildItem:
                mInfoWindow::getInstance()->editProperty(phyID);
            case MxFunctions::PropertyParentItem:
                name = "属性";
                break;
            case MxFunctions::NonStructuralQualityChildItem:
                mInfoWindow::getInstance()->editNonStructuralQuality(phyID);
                break;
            case MxFunctions::FieldChildItem:
            {
                //场子节点按数据子类型打开对应面板
                auto fieldData = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field).getFieldData(phyID);
                if (fieldData != nullptr && fieldData->getFieldType() == MxFunctions::FieldType::Analysis)
                    mInfoWindow::getInstance()->editAnalysisField(phyID);
                else
                    mInfoWindow::getInstance()->editDiscreteField(phyID);
            }
            case MxFunctions::FieldParentItem:
                name = "场";
                break;
            case MxFunctions::CaseNSMManagerItem:
                mInfoWindow::getInstance()->editNsmManagerUnderCasePanel(phyID);
                name = "载荷";
                break;
            case MxFunctions::CaseModelFixManagerItem:
            {
                //打开模型修改引用面板（节点phyId即所属工况id）
                mInfoWindow::getInstance()->showModelFixReference(phyID);
                name = "相互作用";
            }
            break;
            case MxFunctions::CaseModelFixChildItem:
            {
                mInfoWindow::getInstance()->editModelFix(phyID);
                name = "相互作用";
            }
            break;
            case MxFunctions::CoordinateChildItem:
                mInfoWindow::getInstance()->editCoordinate(phyID);
            case MxFunctions::CoordinateParentItem:
                name = "坐标系";
                break;
            case MxFunctions::CaseSettingChildItem:
                mInfoWindow::getInstance()->editCase(phyID);
                name = "分析工况";
                break;
            case MxFunctions::CaseLoadChildItem:
            {
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent().parent());
                mInfoWindow::getInstance()->editLoad(phyID, caseId);
                // 该载荷符号由面板实时渲染负责（editLoad 进入编辑态后，面板 setInputsValue 末尾
                // 防抖统一重建），此处不再 showLoad 全量重建同一符号，避免双重计算与渲染。
                name = "载荷";
            }
            break;
            case MxFunctions::CaseBCChildItem:
            {
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent().parent());
                mInfoWindow::getInstance()->editBoundary(phyID, caseId);
                // 该边界符号由面板实时渲染负责（editBoundary 进入编辑态后，面板 setInputsValue 末尾
                // 统一重建），此处不再 showBoundary 全量重建同一符号，避免双重计算与渲染。
                name = "边界";
            }
            break;
            case MxFunctions::CaseRotateChildItem:
            {
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent().parent());
                mInfoWindow::getInstance()->editRotateAnalyse(phyID, caseId);
                name = "边界"; // 转子分析在边界页
            }
            break;
            case MxFunctions::CaseLoadParentItem:
            {
                //根据index获取父节点item
                mTreeItem* parentItem = getTreeItemByIndex(modelIndexs[0]);
                //根据父节点item获取子节点item
                QVector<mTreeItem*> childItems = parentItem->getChildrenItem();
                //根据子节点item获取载荷id
                for (auto child : childItems)
                {
                    int phyID = getPhyIdByItem(child);
                    auto _loadBoundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(phyID);
                    auto render = AllRenderSymbolManager::getInstance();
                    render->showLoad(_loadBoundaryData);
                }
                //打开载荷引用面板（节点phyId即所属工况id）
                mInfoWindow::getInstance()->showLoadReference(phyID);
                name = "载荷";
            }
            break;
            case MxFunctions::CaseBCParentItem:
            {
                //根据index获取父节点item
                mTreeItem* parentItem = getTreeItemByIndex(modelIndexs[0]);
                //根据父节点item获取子节点item
                QVector<mTreeItem*> childItems = parentItem->getChildrenItem();
                //根据子节点item获取载荷id
                for (auto child : childItems)
                {
                    int phyID = getPhyIdByItem(child);
                    auto _loadBoundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(phyID);
                    auto render = AllRenderSymbolManager::getInstance();
                    render->showBoundary(_loadBoundaryData);
                }
                //打开边界引用面板（节点phyId即所属工况id）
                mInfoWindow::getInstance()->showBoundaryReference(phyID);
                name = "边界";
            }
            break;
            case MxFunctions::CaseImplicitLinearStaticParentItem:
            case MxFunctions::CaseImplicitNonLinearStaticParentItem:
            case MxFunctions::CaseImplicitModalParentItem:
            case MxFunctions::CaseImplicitResponseSpectrumParentItem:
            case MxFunctions::CaseImplicitRandomVibrationParentItem:
            case MxFunctions::CaseImplicitFrequencyParentItem:
            case MxFunctions::CaseAsClouplingParentItem:
            case MxFunctions::CaseImplicitGenericParentItem:
            case MxFunctions::CaseImplicitTransientParentItem:
            case MxFunctions::CaseImplicitNonLinearDynamicParentItem:
            case MxFunctions::CaseImplicitLinearBucklingParentItem:
            case MxFunctions::CaseImplicitPostBucklingParentItem:
            case MxFunctions::CaseImplicitSteadyHeatParentItem:
            case MxFunctions::CaseImplicitTransientHeatParentItem:
                name = "分析工况";
                break;
            case MxFunctions::MeshChildItem:
                //全局尺寸&局部尺寸&体网格
                if (getTextByModelIndex(modelIndexs[0]) == QString("全局尺寸"))
                {
                    mInfoWindow::getInstance()->editGlobalDim(phyID);
                }
                else
                {
                    mInfoWindow::getInstance()->editMeshPanel(phyID);
                }
            case MxFunctions::MeshParentItem:
                name = "网格";
                break;
            case MxFunctions::MeshOptChildItem:
                //全局尺寸&局部尺寸&体网格
                if (getTextByModelIndex(modelIndexs[0]) == QString("全局尺寸"))
                {
                    mInfoWindow::getInstance()->editGlobalDim(phyID);
                }
                else
                {
                    mInfoWindow::getInstance()->editOptMeshPanel(phyID);
                }
            case MxFunctions::MeshOptParentItem:
                name = "网格";
                break;
            case MxFunctions::ModelChildItem:
                mMechGlobalSignals::getInstance()->activePickSig(9);
                hightPart();
            case MxFunctions::ModelParentItem:
                //多体形态下"部件"分组即本节点（挂多体根首位）：单击切"几何"页并开几何信息面板
                name = "几何";
                if ((getChildrenSize(modelIndexs[0]) > 0 || phyID > 0 ) && !isSketch)
                {
                    mInfoWindow::getInstance()->createGeoInfo(type, std::vector<int>{phyID});
                }
                break;
            case MxFunctions::SectionShapeChildItem:
                mInfoWindow::getInstance()->editSectionShape(phyID);
            case MxFunctions::SectionShapeParentItem:
                name = "梁截面";
                break;
            case MxFunctions::FunctionChildItem:
                mInfoWindow::getInstance()->editFunction(phyID);
            case MxFunctions::FunctionParentItem:
                name = "函数";
                break;
            case MxFunctions::LoadManagerParentItem:
            {
                mInfoWindow::getInstance()->showLoadManager();
                auto render = AllRenderSymbolManager::getInstance();
                MechCaseData::mLoadBoundaryManager& manager = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
                //获取所有载荷id 载荷管理器下一定是所有载荷
                std::vector<int> allloadids = manager.getAllLoadID();
                for (auto id : allloadids)
                {
                    auto _loadBoundaryData = manager.getLoadBoundaryData(id);
                    render->showLoad(_loadBoundaryData);
                }
                name = "载荷";
            }
                break;
            case MxFunctions::ModelFixParentItem:
            {
                mInfoWindow::getInstance()->showModelFixManager();
                name = "相互作用";
            }
            break;
            case MxFunctions::BoundaryManagerParentItem:
            {
                mInfoWindow::getInstance()->showBoundaryManager();
                auto render = AllRenderSymbolManager::getInstance();
                MechCaseData::mLoadBoundaryManager& manager = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
                //获取所有边界id 边界管理器下一定是所有边界
                std::vector<int> allboundaryids = manager.getAllBoundaryID();
                for (auto id : allboundaryids)
                {
                    auto _loadBoundaryData = manager.getLoadBoundaryData(id);
                    render->showLoad(_loadBoundaryData);
                }
                name = "边界";
            }
                break;
            case MxFunctions::PredefinedFieldManagerParentItem:
            {
                mInfoWindow::getInstance()->showPredefinedFieldManager();
                auto render = AllRenderSymbolManager::getInstance();
                MechCaseData::mPredefinedFieldManager& manager = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField);
                //获取所有预定义场id
                std::vector<int> allfieldids = manager.getAllPredefinedFieldID();
                for (auto id : allfieldids)
                {
                    auto fieldData = manager.getPredefinedFieldData(id);
                    render->showPredefinedField(fieldData);
                }
                name = "边界"; // 预定义场在边界页
            }
                break;
            case MxFunctions::CasePredefinedFieldParentItem:
            {
                //根据index获取父节点item
                mTreeItem* parentItem = getTreeItemByIndex(modelIndexs[0]);
                //根据父节点item获取子节点item
                QVector<mTreeItem*> childItems = parentItem->getChildrenItem();
                //根据子节点item获取预定义场id
                for (auto child : childItems)
                {
                    int fieldId = getPhyIdByItem(child);
                    auto fieldData = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(fieldId);
                    auto render = AllRenderSymbolManager::getInstance();
                    render->showPredefinedField(fieldData);
                }
                //打开预定义场引用面板（节点phyId即所属工况id）
                mInfoWindow::getInstance()->showPredefinedFieldReference(phyID);
                name = "边界"; // 预定义场在边界页
            }
                break;
            case MxFunctions::CasePredefinedFieldChildItem:
            {
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent().parent());
                mInfoWindow::getInstance()->editPredefinedField(phyID, caseId);
                // 该预定义场符号由面板实时渲染负责（editPredefinedField 进入编辑态后，
                // 面板 setInputsValue 末尾防抖统一重建），此处不再 showPredefinedField
                // 全量重建同一符号，避免双重计算与渲染。
                name = "边界"; // 预定义场在边界页
            }
            break;
            case MxFunctions::FrequencySamplingPointChildItem:
                //暂时不用做操作
                mInfoWindow::getInstance()->editFREQ(phyID);
            case MxFunctions::FrequencySamplingPointParentItem:
                name = "载荷";
                break;
            case MxFunctions::GenericParamChildItem:
                mInfoWindow::getInstance()->editGenericParam(phyID);
            case MxFunctions::GenericParamParentItem:
            case MxFunctions::GlobalParamParentItem:
                name = "全局参数";
                break;
            case MxFunctions::SolveTaskParentItem:
            {
                //树上"求解任务"节点 -> 参数面板（面板内"管理任务"再打开配置弹窗）
                mInfoWindow::getInstance()->showSolveTaskPanel();
            }
                break;
            case MxFunctions::SolveControlParentItem:
                mInfoWindow::getInstance()->editSolveControl();
                name = "全局参数";
                break;
            case MxFunctions::GlobalOutputControlParentItem:
                mInfoWindow::getInstance()->editGlobalOutputControl();
                name = "全局参数";
                break;
            case MxFunctions::GenericAssignParentItem:
                mInfoWindow::getInstance()->editGenericAssign();
                name = "全局参数";
                break;
            case MxFunctions::GenericMatrixParentItem:
                mInfoWindow::getInstance()->editGenericMatrix();
                name = "全局参数";
                break;
            case MxFunctions::Motion_MovingBodyChildItem:
                mInfoWindow::getInstance()->editMotionBody(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ConnectorChildItem:
                mInfoWindow::getInstance()->editMotionJoint(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_CouplingChildItem:
                mInfoWindow::getInstance()->editMotionCoupling(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_DriveChildItem:
                mInfoWindow::getInstance()->editMotionDrive(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_MarkerChildItem:
                mInfoWindow::getInstance()->editMotionMarker(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_SpringChildItem:
                mInfoWindow::getInstance()->editMotionSpring(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_LiYuanChildItem:
                mInfoWindow::getInstance()->editMotionVectorForce(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ContactParentItem:
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ContactChildItem:
                mInfoWindow::getInstance()->editMotionContact(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ScenarioParentItem:
                mInfoWindow::getInstance()->editMotionScenario(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_AnimationChildItem:
                mInfoWindow::getInstance()->editMotionAnimation(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_LoadExportChildItem:
                mInfoWindow::getInstance()->editMotionLoadExport(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_CurveChildItem:
                mInfoWindow::getInstance()->editMotionCurveSelector(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ControlInputChildItem:
                mInfoWindow::getInstance()->editMotionControlInput(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ElectroImportChildItem:
                mInfoWindow::getInstance()->editMotionElectroImport(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ElectroExportChildItem:
                mInfoWindow::getInstance()->editMotionElectroExport(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_HydraulicChildItem:
                mInfoWindow::getInstance()->editMotionHydraulic(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_RLChildItem:
                mInfoWindow::getInstance()->editMotionRL(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_SensorChildItem:
                mInfoWindow::getInstance()->editMotionSensor(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ControllerChildItem:
                mInfoWindow::getInstance()->editMotionController(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_AutoFlexChildItem:
                mInfoWindow::getInstance()->editMotionAutoFlex(phyID);
                name = "多体动力学";
                break;
            case MxFunctions::Motion_RootItem:
            case MxFunctions::Motion_MovingBodyParentItem:
            case MxFunctions::Motion_MarkerParentItem:
            case MxFunctions::Motion_ControlInputParentItem:
            case MxFunctions::Motion_ElectroImportParentItem:
            case MxFunctions::Motion_ElectroExportParentItem:
            case MxFunctions::Motion_HydraulicParentItem:
            case MxFunctions::Motion_RLParentItem:
            case MxFunctions::Motion_SensorParentItem:
            case MxFunctions::Motion_ControllerParentItem:
            case MxFunctions::Motion_AutoFlexParentItem:
                name = "多体动力学";
                break;
            case MxFunctions::Motion_ConnectorParentItem:
            {
                //照载荷模块模式：点击父节点时为全部运动副补建符号（面板未编辑过也能渲染），
                //已存在的不动，显隐仍由末尾 setSymbolShow 按树选中统一控制
                if (getCVM && getCVM->getPreRenderWidget())
                {
                    auto& jointManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);
                    auto* symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                    for (auto id : jointManager.getAllIDs())
                    {
                        mMotionJointSymbolHelper::ensureJointSymbolByData(symbolController,
                            jointManager.getData(id), getCVM->getPreRenderWidget());
                    }
                }
                name = "多体动力学";
            }
                break;
            case MxFunctions::Motion_SpringParentItem:
            {
                //弹簧父节点：批量补建弹簧符号
                if (getCVM && getCVM->getPreRenderWidget())
                {
                    auto& springManager = getCVM->getModelData()->getManager<mMotionSpringManager>(Physical_MotionSpring);
                    auto* symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                    for (auto id : springManager.getAllIDs())
                    {
                        mMotionSpringPanel::ensureSpringSymbolByData(symbolController,
                            springManager.getData(id), getCVM->getPreRenderWidget());
                    }
                }
                name = "多体动力学";
            }
                break;
            case MxFunctions::Motion_DriveParentItem:
            {
                //驱动父节点：驱动自身无符号，为其目标运动副补建符号
                if (getCVM && getCVM->getPreRenderWidget())
                {
                    auto& driveManager = getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive);
                    auto& jointManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);
                    auto* symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                    for (auto id : driveManager.getAllIDs())
                    {
                        auto* driveData = driveManager.getData(id);
                        if (!driveData || driveData->getTargetType() != MxFunctions::DriveTargetType::Joint)
                            continue;
                        int tj = mMotionRefMap::get(Physical_MotionDrive, id, Physical_MotionJoint);
                        if (tj >= 0)
                            mMotionJointSymbolHelper::ensureJointSymbolByData(symbolController,
                                jointManager.getData(tj), getCVM->getPreRenderWidget());
                    }
                }
                name = "多体动力学";
            }
                break;
            case MxFunctions::Motion_CouplingParentItem:
            {
                //耦合副父节点：为两侧运动副补建符号，并补建耦合副自身符号
                if (getCVM && getCVM->getPreRenderWidget())
                {
                    auto& couplingManager = getCVM->getModelData()->getManager<mMotionCouplingManager>(Physical_MotionCoupling);
                    auto& jointManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);
                    auto* symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                    for (auto id : couplingManager.getAllIDs())
                    {
                        auto* coupling = couplingManager.getData(id);
                        if (!coupling) continue;
                        const int firstId = mMotionRefMap::get(Physical_MotionCoupling, id, Physical_MotionJoint, MapRelation_MotionFirstJoint);
                        const int secondId = mMotionRefMap::get(Physical_MotionCoupling, id, Physical_MotionJoint, MapRelation_MotionSecondJoint);
                        mMotionJointSymbolHelper::ensureJointSymbolByData(symbolController,
                            jointManager.getData(firstId), getCVM->getPreRenderWidget());
                        mMotionJointSymbolHelper::ensureJointSymbolByData(symbolController,
                            jointManager.getData(secondId), getCVM->getPreRenderWidget());

                        if (firstId >= 0 && secondId >= 0
                            && !symbolController->isExistSymbol(QString("motionCoupling_%1").arg(id)))
                        {
                            auto* firstJoint = dynamic_cast<mMotionJointBasic*>(jointManager.getData(firstId));
                            auto* secondJoint = dynamic_cast<mMotionJointBasic*>(jointManager.getData(secondId));
                            if (firstJoint && secondJoint)
                            {
                                double ratio = 1.0;
                                if (coupling->getType() == MxFunctions::CouplingType::Gear) {
                                    if (auto* gear = dynamic_cast<mMotionCouplingGear*>(coupling)) ratio = gear->getGearRatio();
                                }
                                else if (coupling->getType() == MxFunctions::CouplingType::GearRack) {
                                    if (auto* gearRack = dynamic_cast<mMotionCouplingGearRack*>(coupling)) ratio = gearRack->getRadius();
                                }
                                mMotionJointSymbolHelper::createOrUpdateCouplingSymbol(symbolController,
                                    id, coupling->getType(),
                                    firstJoint->getOrigin(), firstJoint->getAxis(),
                                    secondJoint->getOrigin(), secondJoint->getAxis(),
                                    ratio, getCVM->getPreRenderWidget());
                            }
                        }
                    }
                }
                name = "多体动力学";
            }
                break;
            case MxFunctions::Motion_LiYuanParentItem:
            {
                //力元父节点：批量补建力元符号（标量力/力矩两端体未映射、矢量力作用体未映射时不建，与面板一致）
                if (getCVM && getCVM->getPreRenderWidget())
                {
                    auto& feManager = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce);
                    auto* symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                    for (auto id : feManager.getAllIDs())
                    {
                        mMotionVectorSymbolHelper::ensureForceElementSymbolByData(symbolController,
                            feManager.getData(id), getCVM->getPreRenderWidget());
                    }
                }
                name = "多体动力学";
            }
                break;
            case MxFunctions::ModalContriChildItem: //模型树下模态贡献量
                mInfoWindow::getInstance()->editModalContri(0);
                name = "全局参数";
                break;
            case MxFunctions::CaseModalContriChildItem://分析工况下模态贡献量
            {
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent().parent());
                mInfoWindow::getInstance()->editModalContri(caseId);
                name = "全局参数";
            }
                break;
            case MxFunctions::NodeContriChildItem://模型树下节点贡献量
                mInfoWindow::getInstance()->editNodeContri(0);
                name = "全局参数";
                break;
            case MxFunctions::CaseNodeContriChildItem://分析工况下节点贡献量
            {
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent().parent());
                mInfoWindow::getInstance()->editNodeContri(caseId);
                name = "全局参数";
            }
                break;
            case MxFunctions::PanelContriChildItem://模型树下面板贡献量
                mInfoWindow::getInstance()->editPanelContri(0);
                name = "全局参数";
                break;
            case MxFunctions::CasePanelContriChildItem://分析工况下面板贡献量
            {
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent().parent());
                mInfoWindow::getInstance()->editPanelContri(caseId);
                name = "全局参数";
            }
                break;
            case MxFunctions::TPContriChildItem://模型树下传递路径贡献量
                mInfoWindow::getInstance()->editTPContri(0);
                name = "全局参数";
                break;
            case MxFunctions::ERPParentItem://模型树下等效辐射功率
                mInfoWindow::getInstance()->editERP(0);
                name = "全局参数";
                break;
            case MxFunctions::PeakFilterChildItem://模型树下峰值筛选
            {
                int peakFilterId = getPhyIdByModelIndex(modelIndexs[0]);
                mInfoWindow::getInstance()->editPeakFilter(peakFilterId);
                name = "全局参数";
            }
                break;
            case MxFunctions::CasePeakFilterChildItem://工况下峰值筛选
            {
                int caseId = getPhyIdByModelIndex(modelIndexs[0].parent().parent());
                mInfoWindow::getInstance()->editCasePeakFilter(caseId);
                name = "全局参数";
            }
                break;
            case MxFunctions::GenericPartParentItem:
                mInfoWindow::getInstance()->editGenericPart();
                name = "全局参数";
                break;
            case MxFunctions::ASCouplingSearchParentItem:
                mInfoWindow::getInstance()->editASCouplingSearchPanel();
                name = "全局参数";
                break;
            default:
                break;
            }
            // 如果上次选中的是分区显示节点，而当前节点不是分区显示，则取消分区显示颜色覆盖
            static BrowserTreeType lastTreeType = MxFunctions::ModelRootItem;
            if (lastTreeType == MxFunctions::partitionChildItem &&
                type != MxFunctions::partitionChildItem)
            {
                const auto& r = getCVM->getPostRenderWidget()->getPostRender();
                if (r)
                {
                    r->setElementColorOverrideEnabled(false);
                }
            }
            lastTreeType = type;
            //多体对象按类型联动到对应多体功能区页（原"多体动力学"单页已拆分为六页）
            if (name == QString::fromUtf8("多体动力学"))
            {
                name = motionTabNameForType(type);
            }
            MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow(name);
        }
        else if (modelIndexs.size() > 1)
        {
            BrowserTreeType type = getTreeTypeByModelIndex(modelIndexs[0]);
            int phyID = getPhyIdByModelIndex(modelIndexs[0]);   
            std::vector<int> partIds;
            for (auto& idx : modelIndexs)
            {
                if (getTreeTypeByModelIndex(idx) != type)
                {
                    mMechGlobalSignals::getInstance()->browsertreeSelectedChanged(getItemKeyByIndex(modelIndexs), false);
                    return;
                }
                if (getTextByModelIndex(idx) == QString("全局尺寸"))
                    return;
                partIds.push_back(getPhyIdByModelIndex(idx));
            }

            switch (type)
            {
            case MxFunctions::ModelChildItem:
                hightPart();
                mInfoWindow::getInstance()->createGeoInfo(type, partIds);
                break;
            case MxFunctions::MeshChildItem:
                mInfoWindow::getInstance()->createMeshPanelMultiSelect(partIds);
                break;
            }
        }
        updateItemSelected();
        mMechGlobalSignals::getInstance()->browsertreeSelectedChanged(getItemKeyByIndex(modelIndexs), true);
        setSymbolShow();
    }

    void mTreeView::updateItemSelected()
    {
        auto subWindow = VMManager->getCurrentViewportPrivate();
        if (!subWindow)
            return;

        QModelIndexList modelIndexs = getSelectIndex();
        if (subWindow->toType<mxr::REBUILD_PostRend>() != nullptr)
        {
            if (modelIndexs.isEmpty()) return;
            subWindow->setTreeSelectItems({ getItemKeyByIndex(modelIndexs[0]) });
        }
        else
        {
            subWindow->setTreeSelectItems(getItemKeyByIndex(modelIndexs));
        }
    }

    void mTreeView::newModelBrowserTree()
    {
        int modelDataID = getCVM->getModelData()->getModelDataID();
        _treeModel->AddModelTree(modelDataID);

        createGlobalCoordinate();

        //新建模型的根行在本函数才插入：此前的显隐补刷（resumeBrowserTreeSig→connectSig）
        //都发生在建树之前，新"模型-x"根行没过行隐藏——多体形态下会与多体根并排露出，
        //直到下次切视口才被藏起。建树后立即补一次（与 clearModelTree/copyModelBrowserTree 收尾同款）
        updateTreeSectionsForViewport();
    }

    void mTreeView::copyModelBrowserTree()
    {
        mModelData* oldModelData = getCVM->getModelData();
        if (!oldModelData)
            return;
        const int oldModelDataID = oldModelData->getModelDataID();

        //记录原模型第一个视口在视图树中的位置（页ID+行号），复制出的新视口放到其后
        int anchorPageID = -1;
        int anchorRow = -1;
        if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
        {
            const QList<int> oldVpIDs = VMManager->getViewportModel(oldModelDataID)->getAllViewportIDs();
            if (!oldVpIDs.isEmpty())
            {
                anchorPageID = viewTree->pageOfViewport(oldVpIDs.first());
                anchorRow = viewTree->rowOfViewport(oldVpIDs.first());
            }
        }

        //新建模型（含一个默认视口），newViewportModel 返回新模型ID并把当前模型切到新模型
        const int newModelDataID = mViewTreeFunction::newViewportModel();
        mModelData* newModelData = getCVM->getModelData();
        if (!newModelData || newModelData->getModelDataID() != newModelDataID)
            return;

        //通过序列化与反序列化拷贝数据（_modelDataID 在构造时已生成唯一值，不参与序列化）
        MechIOFile::mIOFileManager::copyModelData(oldModelData, newModelData);

        //拷贝完成后同步界面单位制：newViewportModel() 里的视口激活发生在数据拷贝之前，
        //其触发的单位制 UI 同步读到的是新模型默认单位制而非拷贝过来的单位制，
        //此处按拷贝后的单位索引重新发出 UI 同步信号（与打开工程文件的恢复逻辑一致，
        //走 UI 同步信号避免重复换算数据）
        QVector<int> unitsIdx = newModelData->getUnitSystemManager()->getCurUnitsIndex();
        if (unitsIdx.size() == 4)
        {
            emit mMechGlobalSignals::getInstance()->unitSystemChanged(unitsIdx[0]);
            emit mMechGlobalSignals::getInstance()->angleUnitChanged(unitsIdx[1]);
            emit mMechGlobalSignals::getInstance()->speedUnitChanged(unitsIdx[2]);
            emit mMechGlobalSignals::getInstance()->tempUnitChanged(unitsIdx[3]);
        }
        mInfoWindow::getInstance()->updateAllPanelUnits();

        //序列化把原模型名也拷过来了，需改为唯一新名称（"xxx-copy"、"xxx-copy2"...）
        const QString baseName = oldModelData->getModelDataName();
        QString newName = baseName + QString::fromUtf8("-copy");
        int suffix = 1;
        while (VMManager->isExistModelDataName(newName, newModelDataID))
        {
            newName = QString("%1-copy%2").arg(baseName).arg(++suffix);
        }
        newModelData->setModelDataName(newName);

        //新视口名称需携带新模型名前缀（"模型-N· 视口-1"）
        int newViewportID = -1;
        if (auto* newViewModel = VMManager->getViewportModel(newModelDataID))
        {
            const QList<int> vpIDs = newViewModel->getAllViewportIDs();
            if (!vpIDs.isEmpty())
            {
                newViewportID = vpIDs.first();
                newViewModel->renameViewport(newViewportID, newName + QString::fromUtf8("· 视口-1"));
            }
        }

        //重建模型树节点（AddModelTree 按新模型名建根节点，slot_updateModelTree 填充全部数据节点）
        _treeModel->AddModelTree(newModelDataID);
        createGlobalCoordinate();
        slot_updateModelTree();

        //刷新渲染：反序列化过程中 GeoManager 会为每个部件创建 XCAF 文档实体并发出
        //AppendOnePart 更新信号，但新模型新建的视口尚无 XCAF 文档与渲染数据，
        //需显式驱动一次全量几何+网格重建（与打开工程文件后的刷新方式一致）
        if (auto* newViewModel = VMManager->getViewportModel(newModelDataID))
        {
            newViewModel->updateAllModelOperate();
        }

        //把新视口移到原模型视口之后，并补齐视图树激活表现
        if (anchorPageID >= 0 && newViewportID >= 0)
        {
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
            {
                viewTree->moveViewportToPageRow(newViewportID, anchorPageID, anchorRow + 1);
                viewTree->syncActiveViewportNode(newViewportID);
            }
        }

        //模型树"仅显示当前模型"按视图级行隐藏实现，切到新模型后需刷新显隐
        updateTreeSectionsForViewport();
    }


    MxFunctions::CaseType mTreeView::getCurrentCaseTypeFromSelection()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty())
            return (MxFunctions::CaseType)0;
        BrowserTreeType seltype = getTreeTypeByModelIndex(idxs[0]);
        if (seltype >= CaseImplicitLinearStaticParentItem && seltype < CaseSettingParentItem)
        {
            return static_cast<MxFunctions::CaseType>(seltype - CaseImplicitLinearStaticParentItem + 1);
        }
        // 选中的是工况子节点
        if (seltype == CaseLoadParentItem || seltype == CaseBCParentItem ||
            seltype == CaseRotateParentItem ||
            seltype == CasePredefinedFieldParentItem || seltype == CaseInteractionParentItem ||
            seltype == CaseSettingChildItem || seltype == CaseDiagOptParentItem ||
            seltype == CaseLoadChildItem || seltype == CaseBCChildItem ||
            seltype == CaseRotateChildItem ||
            seltype == CasePredefinedFieldChildItem || seltype == CaseInteractionChildItem ||
            seltype == CaseModalContriChildItem || seltype == CaseNodeContriChildItem ||
            seltype == CasePanelContriChildItem || seltype == CasePeakFilterChildItem ||
            seltype == CaseNSMManagerItem || seltype == CaseModelFixManagerItem ||
            seltype == ResultParentItem)
        {
            auto parentIdx = idxs[0].parent();
            while (parentIdx.isValid())
            {
                BrowserTreeType parentType = getTreeTypeByModelIndex(parentIdx);
                if (parentType >= CaseImplicitLinearStaticParentItem && parentType < CaseSettingParentItem)
                {
                    return static_cast<MxFunctions::CaseType>(parentType - CaseImplicitLinearStaticParentItem + 1);
                }
                parentIdx = parentIdx.parent();
            }
        }
        return (MxFunctions::CaseType)0;
    }

    bool mTreeView::isLoadBoundarySupportedByCase(MxFunctions::CaseType caseType, MxFunctions::LoadBoundaryType type)
    {
        // 按表核对 - 各工况下可创建的载荷/边界类型
        // 当前实现概要（与图片核对中）：
        // - 线性静力学/非线性静力学：力、力矩、压强、重力、螺栓载荷、旋转体力、
        //   固定约束、位移/转角、虚拟约束、位移边界
        // - 模态：固定约束、位移/转角
        // - 响应谱：固定约束、位移/转角、位移边界、谱载荷
        // - 随机振动：全部置灰（但图片显示可能支持某些 dload，需核对）
        // - 频响/直接频响：频域载荷、组合载荷、固定约束、位移/转角、位移边界
        // - 瞬态/直接瞬态：时域载荷、组合载荷、固定约束、位移/转角、位移边界
        // - 声固耦合：频域载荷、组合载荷、声学频域载荷、固定约束、位移/转角、位移边界
        // - 超单元：全部置灰（与图片一致）
        // - 非线性动力学：力、力矩、压强、重力、螺栓载荷、旋转体力、
        //   固定约束、位移/转角、速度、加速度、位移边界
        // - 线性屈曲/后屈曲：力、力矩、压强、重力、螺栓载荷、旋转体力、
        //   固定约束、位移/转角、位移边界
        // - 稳态传热/瞬态传热：热流密度、热辐射、对流换热、温度、温度边界
        // 注意：图片中黄色高亮的 LOAD_IM_COMBINED（组合载荷）、LOAD_ROTATIONAL_SPEED、
        //       LOAD_ROTATIONAL_ACC 等在当前菜单中尚未加入，需按表补充。
        if (caseType == (MxFunctions::CaseType)0)
            return true; // 未指定工况类型时不限制

        switch (caseType)
        {
        case MxFunctions::ImplicitLinearStatic:
        case MxFunctions::ImplicitNonLinearStatic:
            switch (type)
            {
            case MxFunctions::LOAD_IM_FORCE:
            case MxFunctions::LOAD_IM_TORQUE:
            case MxFunctions::LOAD_IM_PRESSURE:
            case MxFunctions::LOAD_IM_SURFACE:
            case MxFunctions::LOAD_IM_GRAVITY:
            case MxFunctions::LOAD_IM_BOLT:
            case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_IM_VIRTUAL:
            case MxFunctions::BOUNDARY_MOVE:
                return true;
            default:
                return false;
            }
        case MxFunctions::ImplicitModal:
            switch (type)
            {
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
                return true;
            default:
                return false;
            }
        case MxFunctions::ImplicitResponseSpectrum:
            switch (type)
            {
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_MOVE:
            case MxFunctions::LOAD_IM_SPECTRUM:
                return true;
            default:
                return false;
            }
        case MxFunctions::ImplicitRandomVibration:
            return false; // 全部置灰
        case MxFunctions::ImplicitFrequency:
            switch (type)
            {
            case MxFunctions::LOAD_IM_FREQDOMAIN:
            case MxFunctions::LOAD_IM_COMBINED:
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_MOVE:
                return true;
            default:
                return false;
            }
        case MxFunctions::ImplicitTransient:
            switch (type)
            {
            case MxFunctions::LOAD_IM_TRANSIENT:
            case MxFunctions::LOAD_IM_COMBINED:
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_MOVE:
                return true;
            default:
                return false;
            }
        case MxFunctions::ASCoupling:
            switch (type)
            {
            case MxFunctions::LOAD_IM_FREQDOMAIN:
            case MxFunctions::LOAD_IM_COMBINED:
            case MxFunctions::LOAD_IM_ACOFREQDOMAIN:
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_MOVE:
                return true;
            default:
                return false;
            }
        case MxFunctions::ImplicitGeneric:
            switch (type)
            {
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
                return true;
            default:
                return false;
            }
        case MxFunctions::ImplicitNonLinearDynamic:
            switch (type)
            {
            case MxFunctions::LOAD_IM_FORCE:
            case MxFunctions::LOAD_IM_TORQUE:
            case MxFunctions::LOAD_IM_PRESSURE:
            case MxFunctions::LOAD_IM_SURFACE:
            case MxFunctions::LOAD_IM_GRAVITY:
            case MxFunctions::LOAD_IM_BOLT:
            case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_IM_SPEED:
            case MxFunctions::BOUNDARY_IM_ACC:
            case MxFunctions::BOUNDARY_MOVE:
                return true;
            default:
                return false;
            }
        case MxFunctions::ImplicitLinearBuckling:
        case MxFunctions::ImplicitPostBuckling:
            switch (type)
            {
            case MxFunctions::LOAD_IM_FORCE:
            case MxFunctions::LOAD_IM_TORQUE:
            case MxFunctions::LOAD_IM_PRESSURE:
            case MxFunctions::LOAD_IM_SURFACE:
            case MxFunctions::LOAD_IM_GRAVITY:
            case MxFunctions::LOAD_IM_BOLT:
            case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
            case MxFunctions::BOUNDARY_IM_FIXED:
            case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
            case MxFunctions::BOUNDARY_MOVE:
                return true;
            default:
                return false;
            }
        case MxFunctions::ImplicitSteadyHeat:
        case MxFunctions::ImplicitTransientHeat:
            switch (type)
            {
            case MxFunctions::LOAD_IM_HEATFLUX:
            case MxFunctions::LOAD_IM_RADIATION:
            case MxFunctions::LOAD_IM_CONVECTION:
            case MxFunctions::BOUNDARY_IM_TEMP:
            case MxFunctions::BOUNDARY_TEMP_NEW:
                return true;
            default:
                return false;
            }
        default:
            // 其他未显式配置的工况保持默认可用
            return true;
        }
    }

    bool mTreeView::isPredefinedFieldSupportedByCase(MxFunctions::CaseType caseType, MxFunctions::PredefinedFieldType type)
    {
        // 按表核对 - 各工况下可创建的预定义场类型
        // 当前实现概要（与图片核对中）：
        // - 线性静力学/非线性静力学/非线性动力学/稳态传热/瞬态传热：
        //   初始温度、温度载荷
        // - 模态/响应谱/随机振动/频响/瞬态/声固耦合/超单元/线性屈曲/后屈曲：全部禁用
        // 注意：图片中黄色高亮的 PREDEFINED_FIELD_TEMP_LOAD（温度载荷）在部分工况下
        //       可能仍应支持，需按表逐项核对。
        if (caseType == (MxFunctions::CaseType)0)
            return true;

        switch (caseType)
        {
        case MxFunctions::ImplicitLinearStatic:
        case MxFunctions::ImplicitNonLinearStatic:
        case MxFunctions::ImplicitNonLinearDynamic:
        case MxFunctions::ImplicitSteadyHeat:
        case MxFunctions::ImplicitTransientHeat:
            return type == MxFunctions::PREDEFINED_FIELD_INIT_TEMP ||
                type == MxFunctions::PREDEFINED_FIELD_TEMP_LOAD;
        case MxFunctions::ImplicitModal:
        case MxFunctions::ImplicitResponseSpectrum:
        case MxFunctions::ImplicitRandomVibration:
        case MxFunctions::ImplicitFrequency:
        case MxFunctions::ImplicitTransient:
        case MxFunctions::ASCoupling:
        case MxFunctions::ImplicitGeneric:
        case MxFunctions::ImplicitLinearBuckling:
        case MxFunctions::ImplicitPostBuckling:
            return false;
        default:
            return true;
        }
    }

    QString mTreeView::getCaseIconPath(MxFunctions::CaseType caseType)
    {
        // 与分析菜单栏(getCaseMenu)使用的图标保持一致
        switch (caseType)
        {
        case MxFunctions::ImplicitLinearStatic:             return ":/mMainWidgets/Icon/CaseType/linearStatic.svg";
        case MxFunctions::ImplicitNonLinearStatic:          return ":/mMainWidgets/Icon/CaseType/nonlinearStatic.svg";
        case MxFunctions::ImplicitModal:                    return ":/mMainWidgets/Icon/CaseType/modal.svg";
        case MxFunctions::ImplicitResponseSpectrum:         return ":/mMainWidgets/Icon/CaseType/responseSpectrum.svg";
        case MxFunctions::ImplicitRandomVibration:          return ":/mMainWidgets/Icon/CaseType/randomVibration.svg";
        case MxFunctions::ImplicitFrequency:                return ":/mMainWidgets/Icon/CaseType/frequencyResponse.svg";
        case MxFunctions::ImplicitTransient:                return ":/mMainWidgets/Icon/CaseType/linearTransient.svg";
        case MxFunctions::ImplicitNonLinearDynamic:         return ":/mMainWidgets/Icon/CaseType/nonlinearTransient.svg";
        case MxFunctions::ASCoupling:                       return ":/mMainWidgets/Icon/CaseType/acousticCoupling.svg";
        case MxFunctions::ImplicitGeneric:                  return ":/mMainWidgets/Icon/CaseType/superElement.svg";
        case MxFunctions::ImplicitLinearBuckling:           return ":/mMainWidgets/Icon/CaseType/linearBuckling.svg";
        case MxFunctions::ImplicitPostBuckling:             return ":/mMainWidgets/Icon/CaseType/postBuckling.svg";
        case MxFunctions::ImplicitSteadyHeat:               return ":/mMainWidgets/Icon/CaseType/steadyHeat.svg";
        case MxFunctions::ImplicitTransientHeat:            return ":/mMainWidgets/Icon/CaseType/transientHeat.svg";
        case MxFunctions::ImplicitStructuralCouplingHeat:   return ":/mMainWidgets/Icon/CaseType/structuralCoupledHeat.svg";
        default:                                            return ":/mMainWidgets/Icon/BrowserTree/fenxi.png";
        }
    }

    void mTreeView::createCaseFirstLevelItems(int modelID, MxFunctions::CaseType caseType, int caseID, MechPreData::mTreeItem* caseParentItem, MxFunctions::BrowserTreeStatus resultStatus)
    {
        if (caseParentItem == nullptr)
            return;

        // 分析设置
        auto caseSettingItem = new mTreeItem(modelID, CaseSettingChildItem, "分析设置", caseID);
        _treeModel->appendChildren(caseParentItem, caseSettingItem);

        // 载荷
        auto caseLoadItem = new mTreeItem(modelID, CaseLoadParentItem, "载荷", caseID);
        _treeModel->appendChildren(caseParentItem, caseLoadItem);

        // 边界
        auto BCItem = new mTreeItem(modelID, CaseBCParentItem, "边界", caseID);
        _treeModel->appendChildren(caseParentItem, BCItem);

        // 诊断优化（频响/声固耦合）
        if (caseType == MxFunctions::ImplicitFrequency || caseType == MxFunctions::ASCoupling)
        {
            auto diagOptItem = new mTreeItem(modelID, CaseDiagOptParentItem, "诊断优化", caseID);
            _treeModel->appendChildren(caseParentItem, diagOptItem);
        }

        // 相互作用（线性静力学工况不显示）
        if (caseType != MxFunctions::ImplicitLinearStatic)
        {
            auto interItem = new mTreeItem(modelID, CaseInteractionParentItem, "相互作用", caseID);
            _treeModel->appendChildren(caseParentItem, interItem);
        }

        // 预定义场
        auto casePredefinedFieldItem = new mTreeItem(modelID, CasePredefinedFieldParentItem, "预定义场", caseID);
        _treeModel->appendChildren(caseParentItem, casePredefinedFieldItem);

        // 非结构质量
        auto nsmItem = new mTreeItem(modelID, CaseNSMManagerItem, "非结构质量", caseID);
        _treeModel->appendChildren(caseParentItem, nsmItem);

        // 转子分析（仅复模态分析且启用转子动力学时）
        if (caseType == MxFunctions::ImplicitModal)
        {
            mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
            mCaseImplicitModal* modalData = dynamic_cast<mCaseImplicitModal*>(caseBase);
            bool showRotational = modalData && (modalData->_modalType == "复模态分析") && modalData->_rotationalChecked;
            if (showRotational)
            {
                auto caseRotateItem = new mTreeItem(modelID, CaseRotateParentItem, "转子分析", caseID);
                _treeModel->appendChildren(caseParentItem, caseRotateItem);
            }
        }

        auto modelFixItem = new mTreeItem(modelID, CaseModelFixManagerItem, "模型修改", caseID);
        _treeModel->appendChildren(caseParentItem, modelFixItem);

        // 根据已有 Case -> ModelFix 映射填充工况树模型修改子节点
        QList<int> caseModelFixIds = getCVM->getModelData()->getMapRelationManager()
            .getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
        QVector<mTreeItem*> caseModelFixChildren;
        caseModelFixChildren.reserve(caseModelFixIds.size());
        for (int modelFixID : caseModelFixIds)
        {
            auto modelFixData = getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(modelFixID);
            if (!modelFixData) continue;

            // 统一取当前数据状态，避免创建时与对应管理树节点状态不一致
            QVector<QPair<QString, QString>> info;
            BrowserTreeStatus initStatus = getCVM->getModelData()->getDataStatus(info, Physical_ModelFix, modelFixID);

            auto childitem = new mTreeItem(modelID, CaseModelFixChildItem, modelFixData->getModelFixName(), modelFixID, initStatus);
            childitem->setCaseID(caseID);
            caseModelFixChildren.push_back(childitem);
        }
        if (!caseModelFixChildren.isEmpty())
        {
            _treeModel->appendChildren(modelFixItem, std::move(caseModelFixChildren));
        }


        // 根据分析类型移除不支持的父节点
        removeCaseParentItemsByCaseType(caseType, caseParentItem);
    }

    void mTreeView::removeCaseParentItemsByCaseType(MxFunctions::CaseType caseType, MechPreData::mTreeItem* caseParentItem)
    {
        // 按表核对 - 创建/切换工况时应移除的父节点
        // 当前实现概要（与图片核对中）：
        // - 模态/响应谱：移除载荷、预定义场、相互作用节点
        // - 随机振动：移除载荷、边界、预定义场、相互作用节点
        // - 频响/瞬态/声固耦合/超单元：移除预定义场、相互作用节点
        // - 线性屈曲/后屈曲：移除预定义场节点
        // 注意：该函数控制树节点是否显示，需与 isLoadBoundarySupportedByCase 保持一致，
        //       否则会出现菜单允许创建但树中无对应父节点的情况。
        if (caseParentItem == nullptr)
            return;

        bool removeLoad = false;
        bool removeBC = false;
        bool removePredefinedField = false;
        bool removeInteraction = false;

        switch (caseType)
        {
        case MxFunctions::ImplicitModal:
        case MxFunctions::ImplicitResponseSpectrum:
            removeLoad = true;
            removePredefinedField = true;
            removeInteraction = true;
            break;
        case MxFunctions::ImplicitRandomVibration:
            removeLoad = true;
            removeBC = true;
            removePredefinedField = true;
            removeInteraction = true;
            break;
        case MxFunctions::ImplicitFrequency:
        case MxFunctions::ImplicitTransient:
        case MxFunctions::ASCoupling:
        case MxFunctions::ImplicitGeneric:
            removePredefinedField = true;
            removeInteraction = true;
            break;
        case MxFunctions::ImplicitLinearBuckling:
        case MxFunctions::ImplicitPostBuckling:
            removePredefinedField = true;
            break;
        case ImplicitLinearStatic:
            removeInteraction = true;
        default:
            break;
        }

        // 转子分析节点：仅复模态分析且启用转子动力学时保留；否则移除
        bool keepRotational = false;
        if (caseType == MxFunctions::ImplicitModal)
        {
            int caseID = _treeModel->data(_treeModel->indexFromItem(caseParentItem), Roles::PhyidRole).toInt();
            auto caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
            if (auto modalData = dynamic_cast<MechCaseData::mCaseImplicitModal*>(caseBase))
            {
                keepRotational = (modalData->_modalType == "复模态分析") && modalData->_rotationalChecked;
            }
        }

        QVector<MechPreData::mTreeItem*> toRemove;
        for (auto* child : caseParentItem->getChildrenItem())
        {
            if (child == nullptr)
                continue;
            auto idx = _treeModel->indexFromItem(child);
            auto treeType = static_cast<MxFunctions::BrowserTreeType>(_treeModel->data(idx, Roles::TypeRole).toInt());
            if ((removeLoad && treeType == CaseLoadParentItem) ||
                ((!keepRotational) && treeType == CaseRotateParentItem) ||
                (removeBC && treeType == CaseBCParentItem) ||
                (removePredefinedField && treeType == CasePredefinedFieldParentItem) ||
                (removeInteraction && treeType == CaseInteractionParentItem))
            {
                toRemove.push_back(child);
            }
        }
        for (auto* item : toRemove)
        {
            removeTreeItem(item);
        }
    }

    void mTreeView::registCommond()
    {
        //材料
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_CREATEUNDO,
        //    std::bind(&mTreeView::slot_commondDeleteMaterial, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_DELREDO,
        //    std::bind(&mTreeView::slot_commondDeleteMaterial, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_CREATEREDO,
        //    std::bind(&mTreeView::slot_commondCreateMaterial, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_DELUNDO,
        //    std::bind(&mTreeView::slot_commondCreateMaterial, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_PROPMODUNDO,
        //    std::bind(&mTreeView::slot_commondModMatProp, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_PROPMODREDO,
        //    std::bind(&mTreeView::slot_commondModMatProp, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_PROPADDUNDO,
        //    std::bind(&mTreeView::slot_commondModMatProp, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_PROPADDREDO,
        //    std::bind(&mTreeView::slot_commondModMatProp, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_PROPDELUNDO,
        //    std::bind(&mTreeView::slot_commondModMatProp, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::MAT_PROPDELREDO,
        //    std::bind(&mTreeView::slot_commondModMatProp, this));
        ////载荷
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::LOAD_CREATEUNDO,
        //    std::bind(&mTreeView::slot_commondDeleteLoad, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::LOAD_DELREDO,
        //    std::bind(&mTreeView::slot_commondDeleteLoad, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::LOAD_CREATEREDO,
        //    std::bind(&mTreeView::slot_commondCreateLoad, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::LOAD_DELUNDO,
        //    std::bind(&mTreeView::slot_commondCreateLoad, this));
        ////边界
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::BOUNDARY_CREATEUNDO,
        //    std::bind(&mTreeView::slot_commondDeleteBoundary, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::BOUNDARY_DELREDO,
        //    std::bind(&mTreeView::slot_commondDeleteBoundary, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::BOUNDARY_CREATEREDO,
        //    std::bind(&mTreeView::slot_commondCreateBoundary, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::BOUNDARY_DELUNDO,
        //    std::bind(&mTreeView::slot_commondCreateBoundary, this));
        ////连接
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::CONNECTOR_CREATEUNDO,
        //    std::bind(&mTreeView::slot_commondDeleteConnector, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::CONNECTOR_DELREDO,
        //    std::bind(&mTreeView::slot_commondDeleteConnector, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::CONNECTOR_CREATEREDO,
        //    std::bind(&mTreeView::slot_commondCreateConnector, this));
        //mCommondSystem::getInstance()->getCommondBase()->regCallBackFunc(mCommondBase::CommondType::CONNECTOR_DELUNDO,
        //    std::bind(&mTreeView::slot_commondCreateConnector, this));
    }

    void mTreeView::createModelRootItem()
    {
        // 按表核对 - 模型树根节点右键菜单项
        // 当前菜单项：创建(含材料/梁截面/截面属性/连接/连接副/接触/分析工况/载荷/边界/预定义场/
        //            层合结构/全局参数/非结构质量/集合/函数/坐标系/场/频率点/结构优化/
        //            一键剖分/提交计算)、重命名(可选)、清空、删除模型
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");
        QMenu* menuCreate = new mUiMenu();
        action->setMenu(menuCreate);

        //创建材料
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/cailiao.png"), "材料");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMaterial();
            });

        /*auto actionMaterial = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/cailiao.png"), "材料");
        QMenu* menuCreateMaterial = new QMenu();
        actionMaterial->setMenu(menuCreateMaterial);
        auto addMaterialAction = [&](const QString& icon, const QString& text, int type) {
            action = menuCreateMaterial->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createMaterial();
                });
            };*/

            //创建截面形状
        auto actionSectionShape = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/jiemianxinzhuang.png"), "梁截面");
        SectionShapeParent(actionSectionShape, false);

        //创建属性
        auto CreateProperty = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/jiemianshuxing.png"), "截面属性");
        menuCreateProperty(CreateProperty, false);

        //创建连接
        auto actionConnector = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/lianjie.png"), "连接");
        ConnectorsParentMenu(actionConnector, false);

        //创建连接副
        auto actionConnectorF = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/lianjiefu.png"), "连接副");
        QMenu* menucreateConnectorF = new mUiMenu();
        actionConnectorF->setMenu(menucreateConnectorF);
        auto addConnectorFAction = [&](const QString& icon, const QString& text, MxFunctions::DeputyType type) {
            action = menucreateConnectorF->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createDeputy(type);
                });
            };
        addConnectorFAction(":/mMainWidgets/Icon/BrowserTree/lianjieqi.png", "连接器", MxFunctions::Deputy_Connector);
        addConnectorFAction(":/mMainWidgets/Icon/Connector/fastener.png", "fastener", MxFunctions::Deputy_Fastener);

        //创建接触
        auto actionCreateContact = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/jiechu.png"), "接触");
        QMenu* menuCreateContact = new mUiMenu();
        actionCreateContact->setMenu(menuCreateContact);
        auto addmenuContactAction = [&](const QString& icon, const QString& text, ContactType type) {
            action = menuCreateContact->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createContact(type);
                });
            };
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/mianmianjiechu.png", "面面接触", SurfaceToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/dianmianjiechu.png", "点面接触", NodeToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/dianmianjiechu.png", "线面接触", LineToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/tongyongjiechu.png", "通用接触", CommonContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/zijiechu.png", "自接触", SelfContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/guoyingjiechu.png", "过盈接触", InterferenceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/mianmianjiechu.png", "二维接触", TwoDimensionalContact);


        //创建工况
        auto actionCase = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fenxi.png"), "分析工况");
        QMenu* menuCreateCase = new mUiMenu();
        actionCase->setMenu(menuCreateCase);
        // 按表核对 - 分析工况子菜单项列表
        // 当前支持：线性静力学、非线性静力学、模态、响应谱、随机振动、频响、线性瞬态、
        //          非线性瞬态、声固耦合、超单元、线性屈曲、后屈曲、稳态传热、瞬态传热、结构耦合传热
        auto addAnalysisAction = [&](const QString& text, MxFunctions::CaseType type) {
            action = menuCreateCase->addAction(QIcon(getCaseIconPath(type)), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createCase(type);
                });
            };
        //  静态分析
        addAnalysisAction("线性静力学分析", ImplicitLinearStatic);
        addAnalysisAction("非线性静力学分析", ImplicitNonLinearStatic);

        // 隐式动力学
        addAnalysisAction("非线性瞬态分析", ImplicitNonLinearDynamic);

        // 稳定性分析
        addAnalysisAction("线性屈曲分析", ImplicitLinearBuckling);
        addAnalysisAction("后屈曲分析", ImplicitPostBuckling);

        // 传热分析
        addAnalysisAction("稳态传热分析", ImplicitSteadyHeat);
        addAnalysisAction("瞬态传热分析", ImplicitTransientHeat);

        // 线性动态分析
        addAnalysisAction("模态分析", ImplicitModal);
        addAnalysisAction("响应谱分析", ImplicitResponseSpectrum);
        addAnalysisAction("随机振动分析", ImplicitRandomVibration);
        addAnalysisAction("频响分析", ImplicitFrequency);
        addAnalysisAction("线性瞬态分析", ImplicitTransient);
        addAnalysisAction("声固耦合分析", ASCoupling);
        addAnalysisAction("超单元分析", ImplicitGeneric);
        /*
        0630版本屏蔽
        addAnalysisAction("结构耦合传热分析", ImplicitStructuralCouplingHeat);
        */

        // 按表核对 - 载荷/边界/预定义场菜单过滤规则
        // 当前规则：根据当前选中的工况类型(currentCaseType)过滤不支持的载荷/边界/预定义场类型
        // 具体过滤逻辑见 isLoadBoundarySupportedByCase / isPredefinedFieldSupportedByCase
        MxFunctions::CaseType currentCaseType = getCurrentCaseTypeFromSelection();

        //创建载荷
        auto actionCreateLoad = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/zaiheguanliqi.png"), "载荷");
        LoadParentMenu(actionCreateLoad, false, currentCaseType);

        //创建边界条件
        auto actionCreateBoundary = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/bianjieguanliqi.png"), "边界条件");
        BoundaryParentMenu(actionCreateBoundary, false, currentCaseType);

        //创建预定义场
        auto actionPredefinedField = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yudingyichang.png"), "预定义场");
        PredefinedFieldParentMenu(actionPredefinedField, false, currentCaseType);

        //层合结构
        auto actionLaminate = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/cenghejiegou.png"), "层合结构");
        LaminateManagerMenu(actionLaminate, false);

        //创建全局参数设置
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/globalParam.png"), "全局参数设置");
        GlobalParameterMenu(action, false);

        menuCreate->addSeparator();

        //创建非结构质量
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/Case/nsm@3x.png"), "非结构质量");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createNonStructuralQuality();
            });

        //创建集合
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/homepage/jihe3x.png"), "集合");
        add2SetParentMenu(action, false);


        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createSet();
            });

        //创建函数
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"), "函数");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);
            });

        //创建坐标系		  
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/zuobiaoxixiao.png"), "坐标系");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createCoordinate();
            });

        //创建场（二级菜单：厚度场/角度场/坐标系场/分析场）
        auto actionField = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/lisanchang.png"), "场");
        QMenu* menuCreateField = new mUiMenu();
        actionField->setMenu(menuCreateField);
        auto addFieldAction = [&](const QString& text, MxFunctions::DiscreteSpace space) {
            action = menuCreateField->addAction(text);
            connect(action, &QAction::triggered, this, [this, space]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createDiscreteField(space);
                });
            };
        addFieldAction("厚度场", MxFunctions::DiscreteSpace::ThicknessSpace);
        addFieldAction("角度场", MxFunctions::DiscreteSpace::AngelSpace);
        menuCreateField->addSeparator();
        addFieldAction("坐标系场", MxFunctions::DiscreteSpace::CoorSpace);
        menuCreateField->addSeparator();
        action = menuCreateField->addAction("分析场");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createAnalysisField();
            });

        //创建频率点采样方式
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/homepage/caiyangpinlvdian.png"), "采样频率点");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->creatFREQ();
            });

        //创建结构优化
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/jiegouyouhua.png"), "优化");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createOptimization();
            });

        //创建流道优化
        //action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/TopologyOptimization/runner.png"), "流道优化");
        //connect(action, &QAction::triggered, this, [this]() {
        //    mInfoWindow::getInstance()->unEditInfoWindow();
        //    mInfoWindow::getInstance()->createRunnerOptimization();
        //    });

        menuCreate->addSeparator();

        //创建一键剖分
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "一键剖分");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMeshAuto();

            });
        menuCreate->addSeparator();

        //创建提交计算
        //action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/tijiaojisuan.png"), "提交计算");
        //connect(action, &QAction::triggered, this, [this]() {
        //    mInfoWindow::getInstance()->unEditInfoWindow();
        //    if (getCVM->isPostModel())
        //    {
        //        return;
        //    }
        //    vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
        //    if (caseIDs.empty())
        //    {
        //        mGlobalSignals::getInstance()->outputMessageSig("无分析工况可求解！");
        //        return;
        //    }

        //    mStartSolve::getInstance()->solver(caseIDs[0]);

        //    }
        //);

        //创建多体运动体
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_movingBodyParentItem.png"), "运动体");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMotionBody();
            });

        //创建多体运动副
        auto actionMotionJoint = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_connectorParentItem.png"), "运动副");
        QMenu* menuCreateMotionJoint = new QMenu();
        actionMotionJoint->setMenu(menuCreateMotionJoint);
        auto addMotionJointAction = [&](const QString& icon, const QString& text, int type) {
            action = menuCreateMotionJoint->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionJointNew(JointType(type));
                });
            };
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_revolute.png", "旋转副", 0);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_prismatic.png", "移动副", 1);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_cylindrical.png", "柱面副", 2);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_spherical.png", "球面副", 3);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_fixed.png", "固定副", 4);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_universal.png", "万向节", 5);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_pointPlane.png", "点面副", 6);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_parallel.png", "平行约束", 8);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_perpendicular.png", "垂直约束", 9);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_direction.png", "方向约束", 10);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_cv.png", "恒速副", 12);
        //addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_hooke.png", "胡克副", 13);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_screw.png", "螺纹副", 14);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_planar.png", "平面副", 15);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_pointLine.png", "点线副", 18);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_pointOnCurve.png", "点在线上副", 19);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_pointOnSurface.png", "点在面上副", 20);
        addMotionJointAction(":/mMainWidgets/Icon/Motion/joint_curveOnCurve.png", "线在线上副", 21);

        //创建多体耦合副
        auto actionMotionCoupling = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_couplingParentItem.png"), "耦合副");
        QMenu* menuCreateMotionCoupling = new QMenu();
        actionMotionCoupling->setMenu(menuCreateMotionCoupling);
        auto addMotionCouplingAction = [&](const QString& icon, const QString& text, MxFunctions::CouplingType type) {
            action = menuCreateMotionCoupling->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionCouplingNew(type);
                });
            };
        addMotionCouplingAction(":/mMainWidgets/Icon/Motion/coupling_gearRack.png", "齿轮齿条副", CouplingType::GearRack);
        addMotionCouplingAction(":/mMainWidgets/Icon/Motion/joint_gear.png", "齿轮副", CouplingType::Gear);
        addMotionCouplingAction(":/mMainWidgets/Icon/Motion/coupling_cable.png", "线缆副", CouplingType::Cable);

        //创建多体驱动
        auto actionMotionDrive = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_driveParentItem.png"), "驱动");
        QMenu* menuCreateMotionDrive = new QMenu();
        actionMotionDrive->setMenu(menuCreateMotionDrive);
        auto addMotionDriveAction = [&](const QString& icon, const QString& text, int type) {
            action = menuCreateMotionDrive->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionDriveNew(DriveTargetType(type));
                });
            };
        addMotionDriveAction(":/mMainWidgets/Icon/Motion/rotaryJoint.png", "运动副驱动", 0);

        //创建传感器
        auto actionCreateSensor = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/Motion/sensor.png"), "传感器");
        QMenu* menuCreateSensor = new QMenu();
        actionCreateSensor->setMenu(menuCreateSensor);
        auto addCreateSensorAction = [&](const QString& icon, const QString& text, MechMotionData::SensorType type) {
            action = menuCreateSensor->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createMotionSensor(type);
                });
            };
        addCreateSensorAction(":/mMainWidgets/Icon/Motion/sensor_displacement.png", "体位移传感器", MechMotionData::SensorType::BodyDisplacement);
        addCreateSensorAction(":/mMainWidgets/Icon/Motion/sensor_velocity.png", "体速度传感器", MechMotionData::SensorType::BodyVelocity);
        addCreateSensorAction(":/mMainWidgets/Icon/Motion/sensor_acceleration.png", "体加速度传感器", MechMotionData::SensorType::BodyAcceleration);
        addCreateSensorAction(":/mMainWidgets/Icon/Motion/sensor_jointReaction.png", "关节反力传感器", MechMotionData::SensorType::JointReaction);
        addCreateSensorAction(":/mMainWidgets/Icon/Motion/sensor_driveActuation.png", "驱动输出传感器", MechMotionData::SensorType::DriveActuation);
        addCreateSensorAction(":/mMainWidgets/Icon/Motion/sensor_distance.png", "距离传感器", MechMotionData::SensorType::Distance);
        addCreateSensorAction(":/mMainWidgets/Icon/Motion/sensor_angle.png", "角度传感器", MechMotionData::SensorType::Angle);

        //创建控制器
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/Motion/controlCalculation.png"), "控制器");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMotionController();
            });

        //创建标记点
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/caotu/dian.png"), "标记点");
        connect(action, &QAction::triggered, this, [this]() {
            slot_menuMotionMarkerNew();
            });

        //创建多体求解方案
        action = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_solveParentItem.png"), "求解方案");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMotionScenario();
            });


        if (MechCommon::mMechGlobalSignals::getInstance()->_isTreeRight)
        {
            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
            connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        }

        m_Menu->addSeparator();

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "复制模型");
        connect(action, &QAction::triggered, this, [this]() {
            //先退出草图模式
            if (mGeoSketchSignalManager::instance()->currentStatus())
            {
                mGeoSketchSignalManager::instance()->exitSketch();
            }

            //判断当前节点是否存在，不存在则直接创建一个新模型
            if (_treeModel->isExistItem(ModelRootItem, getCVM->getModelData()->getModelDataID()))
            {
                //复制模型
                copyModelBrowserTree();
            }
            selectedItem(_treeModel->getParentItem(ModelRootItem));
            });

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "清空");
        connect(action, &QAction::triggered, this, [this]() {
            //自绘标题栏提示框（按钮文案保持 确定/取消，默认取消）
            const int r = MechMainWindow::mFramelessMessageBox::information(this, "提示",
                "确定要清空当前模型吗？", "确定", "取消", QString(), 1, 0);
            if (r != 0)
            {
                return;
            }
            clearCurrentModelData();
            });

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除模型");
        connect(action, &QAction::triggered, this, [this]() {
            //自绘标题栏提示框（按钮文案保持 确定/取消，默认取消）
            const int r = MechMainWindow::mFramelessMessageBox::warning(this, "警告",
                "确定要删除当前模型吗？", "确定", "取消", QString(), 1, 0);
            if (r != 0)
            {
                return;
            }
            //先退出草图模式
            if (mGeoSketchSignalManager::instance()->currentStatus())
            {
                mGeoSketchSignalManager::instance()->exitSketch();
            }
            //模型删除时同步移除其顶层多体动力学根（与模型根平级，直挂隐形根）
            removeTreeItem(_treeModel->getParentItem(Motion_RootItem));
            removeTreeItem(_treeModel->getParentItem(ModelRootItem));
            mGlobalSignals::getInstance()->deleteViewportModel();
            if (!_treeModel->isExistItem(ModelRootItem, getCVM->getModelData()->getModelDataID()))
            {
                newModelBrowserTree();
            }
            else
            {
                _treeModel->setCurrentModelID(getCVM->getModelData()->getModelDataID());
            }
            //多体形态下模型根隐藏，回到多体动力学顶层根（模型删除后聚焦当前模型的多体树）
            selectedItem(MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion
                ? _treeModel->getParentItem(Motion_RootItem)
                : _treeModel->getParentItem(ModelRootItem));
            });

        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createAssemblyParentMenu()
    {
        QAction* action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建装配");
        connect(action, &QAction::triggered, this, [this]()
        {
            mModelData* modelData = getCVM->getModelData();
            int cnt = 1;
            QString name = QString("装配-%1").arg(cnt);
            while (modelData->isExistName(Physical_Assembly, name))
            {
                name = QString("装配-%1").arg(++cnt);
            }
            modelData->getAssemblyManager().createAssembly(name);
            emit mGlobalSignals::getInstance()->updateAssemblyTreeSig();
        });
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createAssemblyChildMenu()
    {
        const QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty())
            return;
        //右键选中实例的 phyID 即装配实体ID（parentID 仅用于区分多父引用的树实例）
        const int assemblyId = getPhyIdByModelIndex(idxs[0]);
        mModelData* modelData = getCVM->getModelData();
        AssemblyData::mAssemblyManager& asmMgr = modelData->getAssemblyManager();
        if (!asmMgr.containsAssembly(assemblyId))
            return;

        QAction* action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建子装配");
        connect(action, &QAction::triggered, this, [this, assemblyId]()
        {
            mModelData* modelData = getCVM->getModelData();
            AssemblyData::mAssemblyManager& asmMgr = modelData->getAssemblyManager();
            int cnt = 1;
            QString name = QString("装配-%1").arg(cnt);
            while (modelData->isExistName(Physical_Assembly, name))
            {
                name = QString("装配-%1").arg(++cnt);
            }
            const int childId = asmMgr.createAssembly(name);
            //addChildAssembly 内含 DAG 校验（新建装配不可能成环，防御性处理失败）
            asmMgr.addChildAssembly(assemblyId, childId);
            emit mGlobalSignals::getInstance()->updateAssemblyTreeSig();
        });

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));

        m_Menu->addSeparator();

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, &QAction::triggered, this, [this, assemblyId]()
        {
            //自绘标题栏提示框（按钮文案保持 确定/取消，默认取消）
            const int r = MechMainWindow::mFramelessMessageBox::warning(this, "警告",
                "确定要删除该装配吗？\n删除装配仅解除层级从属关系，不会删除其下部件的几何与网格数据。", "确定", "取消", QString(), 1, 0);
            if (r != 0)
            {
                return;
            }
            getCVM->getModelData()->getAssemblyManager().removeAssembly(assemblyId);
            emit mGlobalSignals::getInstance()->updateAssemblyTreeSig();
        });

        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createGeoItemMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/daoru3x.png"), "导入");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_importGeoFile()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/daochu3x.png"), "导出");
        QMenu* menuCreate = new mUiMenu();
        action->setMenu(menuCreate);
        QAction* actionAll = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "全部");//导出下级菜单导出全部
        connect(actionAll, SIGNAL(triggered()), this, SLOT(slot_exportGeoFileAll()));
        QAction* actionVisible = menuCreate->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "当前显示");//导出下级菜单导出当前显示
        connect(actionVisible, SIGNAL(triggered()), this, SLOT(slot_exportGeoFileVisible()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xianshiquanbu.png"), "显示全部");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowAllPart())); 
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/yingcangquanbu.png"), "隐藏全部");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideAllPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fanxiangxianshi.png"), "反向显示");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuReverseShow()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xianshibujian.png"), "显示模型树中部件");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowTreePart()));
        //m_Menu->addSeparator();
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "批量重命名");
        //connect(action, SIGNAL(triggered()), this, SLOT(slot_menuBatchRename()));

        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/zhutiyanse.png"), "颜色");
        QMenu* menuColorCreate = new mUiMenu();
        action->setMenu(menuColorCreate);
        QAction* action_partColor = menuColorCreate->addAction("部件颜色");
        action_partColor->setCheckable(true);
        connect(action_partColor, &QAction::triggered, this, []() {
            getCVM->setPartType(PartColorType::partColor);
        });
        QAction* action_matColor = menuColorCreate->addAction("材料颜色");
        action_matColor->setCheckable(true);
        connect(action_matColor, &QAction::triggered, this, []() {
            getCVM->setPartType(PartColorType::MatColor);
        });
        if (getCVM->getPartType() == PartColorType::partColor)
        {
            action_partColor->setChecked(true);
        }
        else
        {
            action_matColor->setChecked(true);
        }
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createGeoChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chuangjianwangge.png"), "生成网格");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCreateMesh()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除网格");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuDeleteMesh()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "显示当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowCurrentPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideCurrentPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "隐藏其他");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideOtherPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshiquanbu.png"), "显示全部");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowAllPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示模型树中部件");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowTreePart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/fanzhuanxianshi.png"), "反向显示");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuTreePartReverseShow()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/geometry/shanchuti.png"), "删除几何");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuDeleteGeo()));

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "删除其他几何");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuDeleteOtherGeo()));
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "抑制几何");
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "抑制其他几何");
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/homepage/daochu3x.png"), "导出");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_exportGeoFileSelected()));

        m_Menu->exec(QCursor::pos());
    }

    //set模块
    void mTreeView::createSetParentMenu()
    {
        QAction* action = nullptr;

        //类型分组节点（如"单元集"）右键：提供创建/删除该类型集合
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 1 && getTreeTypeByModelIndex(idxs[0]) == SetParentItem)
        {
            MxFunctions::SetType groupType = static_cast<MxFunctions::SetType>(getPhyIdByModelIndex(idxs[0]));
            if (groupType != static_cast<MxFunctions::SetType>(0))
            {
                QString typeName = getSetTypeNameByType(groupType);
                action = m_Menu->addAction(QIcon(slot_setTypePic(groupType)), QString("创建%1").arg(typeName));
                connect(action, &QAction::triggered, this, [this, typeName]() {
                    mInfoWindow::getInstance()->createSet(typeName);
                    });
                action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QString("删除所有%1").arg(typeName));
                connect(action, &QAction::triggered, this, [this, groupType]() {
                    slot_menuSetTypeDelete(groupType);
                    });
                m_Menu->exec(QCursor::pos());
                return;
            }
        }

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建集合");
        add2SetParentMenu(action, true);

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有集合");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSetDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createSetChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSetChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopySet()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCoordinateParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建坐标系");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCoordinateNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有坐标系");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCoordinateDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCoordinateChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCoordinateChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyCoordinate()));
        m_Menu->exec(QCursor::pos());
    }


    void mTreeView::createStructOptionParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");

        auto menu = new mUiMenu();
        action->setMenu(menu);
        action = menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建设计域");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuOptimizationDesignNew()));
        menu->addAction(action);
        action = menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建优化约束");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuOptimizationConstraintNew()));
        menu->addAction(action);

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuStructOptionDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createOptimizationDesignParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建设计域");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuOptimizationDesignNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有设计域");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuOptimizationDesignDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createOptimizationDesignChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuOptimizationDesignChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyOptimizationDesign()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createOptimizationConstraintParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建优化约束");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuOptimizationConstraintNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有优化约束");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuOptimizationConstraintDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createOptimizationConstraintChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuOptimizationConstraintChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyOptimizationConstraint()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createRunnerOptimizationParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");

        auto menu = new mUiMenu();
        action->setMenu(menu);
        action = menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建设计域");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerOptimizationDesignNew()));
        menu->addAction(action);
        action = menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建优化约束");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerOptimizationConstraintNew()));
        menu->addAction(action);

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerStructOptionDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createRunnerOptimizationDesignParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建设计域");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerOptimizationDesignNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有设计域");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerOptimizationDesignDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createRunnerOptimizationDesignChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerOptimizationDesignChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyRunnerOptimizationDesign()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createRunnerOptimizationConstraintParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建优化约束");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerOptimizationConstraintNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有优化约束");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerOptimizationConstraintDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createRunnerOptimizationConstraintChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRunnerOptimizationConstraintChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyRunnerOptimizationConstraint()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createPropertyParentMenu()
    {
        QAction* action = nullptr;
        QAction* action1 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建截面属性");
        menuCreateProperty(action1, true);
        m_Menu->addSeparator();
        QAction* action2 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建层合结构");
        LaminateManagerMenu(action2, false);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除全部层合结构");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuDeleteAllLaminate()));
        m_Menu->addSeparator();
        QAction* action4 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建梁截面");
        SectionShapeParent(action4, true);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createNonStructQualityParentMenu()
    {
        QAction* action = nullptr;
        QAction* action1 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建非结构质量");
        connect(action1, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createNonStructuralQuality();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除全部非结构质量");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuNonStructuralQualityDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createNonStructQualityChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyNonStructuralQuality()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuNonStructuralQualityDel()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCaseNonStructuralQualityParentMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建非结构质量");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createNonStructuralQuality();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除全部非结构质量");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCaseNonStructuralQualityDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCaseNonStructuralQualityChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCaseNonStructuralQualityChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyCaseNonStructuralQuality()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createFieldParentMenu()
    {
        //新建场（二级菜单：厚度场/角度场/坐标系场/分析场）
        QMenu* fieldMenu = m_Menu->addMenu(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建场");
        QAction* thickAction = fieldMenu->addAction("厚度场");
        connect(thickAction, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createDiscreteField(MxFunctions::DiscreteSpace::ThicknessSpace);
            });
        QAction* angelAction = fieldMenu->addAction("角度场");
        connect(angelAction, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createDiscreteField(MxFunctions::DiscreteSpace::AngelSpace);
            });
        fieldMenu->addSeparator();
        QAction* coorAction = fieldMenu->addAction("坐标系场");
        connect(coorAction, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createDiscreteField(MxFunctions::DiscreteSpace::CoorSpace);
            });
        fieldMenu->addSeparator();
        QAction* analysisAction = fieldMenu->addAction("分析场");
        connect(analysisAction, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createAnalysisField();
            });
        QAction* action3 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有场");
        connect(action3, &QAction::triggered, this, [this]() {
            //删除所有类型的场：按子类型（离散场/分析场）逐类删除
            slot_menuFieldDelete(MxFunctions::FieldType::Discrete);
            slot_menuFieldDelete(MxFunctions::FieldType::Analysis);
            });
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createFieldChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyField()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuFieldDel()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::menuCreateProperty(QAction* menuTitle , bool addDelete) //属性列表
    {
        QAction* action = nullptr;
        QMenu* CreateProperty = new mUiMenu();
        menuTitle->setMenu(CreateProperty);
        auto addPropertyAction = [&](const QString& icon, const QString& text, PropertyType type) {
            action = CreateProperty->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->createProperty(type);
                });
            };
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/gan.png", "杆", RodProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/guan.png", "管", TubeProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/liang.png", "梁", ResultantProperty);
        //addPropertyAction(":/mMainWidgets/Icon/Property/bianjiemianliang.png", "变截面梁", CrossProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/tanhuang.png", "弹簧", Elas1Property);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/zuni.png", "阻尼", ViscousDampProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/tanhuangzuni.png", "弹簧阻尼", CbushProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/jianxi.png", "间隙", GapuniProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/pingmianyingbian.png", "平面应变", PlaneStrainProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/pingmianyingli.png", "平面应力", PlaneStressProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/ke.png", "均质壳", ShellProperty);
        //addPropertyAction(":/mMainWidgets/Icon/Property/mo.png", "膜", MembraneProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/ban.png", "剪切板", ShearPlateProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/sanweishiti.png", "均质三维实体", SolidProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/dianpian.png", "垫片", GasketProperty);
        //addPropertyAction(":/mMainWidgets/Icon/Property/3Dshitiqiao.png", "均质实体壳", SolidShellProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/pucengfucai.png", "铺层复材", PlyLaminateProperty);
        addPropertyAction(":/mMainWidgets/Icon/BrowserTree/pucengfucai.png", "宏观等效截面", MacroEquivalentSection);
        if (addDelete)
        {
            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除全部截面属性");
            connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPropertyDelete()));
        }
    }

    void mTreeView::createPropertyChildMenu()
    {
        QAction* action = nullptr;

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyProperty()));
        //m_Menu->addSeparator();
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "抑制");

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPropertyChildDel()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createFunctionParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建函数");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuFunctionNew()));

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有函数");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuFunctionDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createFunctionChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuFunctionChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyFunction()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCaseParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");//添加载荷和边界条件的子动作
        auto menu = new mUiMenu();
        menu->addActions({});
        action->setMenu(menu);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyCase()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCaseDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createLoadParentMenu(MxFunctions::CaseType caseType)
    {
        m_Menu->clear();
        QAction* action = nullptr;
        QAction* menuaction = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建载荷");
        LoadParentMenu(menuaction, true, caseType);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createRotateParentMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        QAction* menuaction = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建转子分析");
        RotateParentMenu(menuaction, true);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::RotateParentMenu(QAction* menuTitle, bool addDelete)
    {
        QAction* menuaction = nullptr;
        auto menu = new mUiMenu();
        menuTitle->setMenu(menu);

        QAction* action = new QAction(QIcon(":/mMainWidgets/Icon/Case/旋转速度@3x.png"), "旋转速度");
        connect(action, &QAction::triggered, this, [this]() {
            slot_createRotateAnalyse(MxFunctions::ROTATIONAL_SPEED);
            });
        menu->addAction(action);

        action = new QAction(QIcon(":/mMainWidgets/Icon/Case/旋转加速度@3x.png"), "旋转加速度");
        connect(action, &QAction::triggered, this, [this]() {
            slot_createRotateAnalyse(MxFunctions::ROTATIONAL_ACC);
            });
        menu->addAction(action);

        if (addDelete)
        {
            QAction* action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有转子分析");
            connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRotateDelete()));
        }
    }

    void mTreeView::LoadParentMenu(QAction* menuTitle, bool addDelete, MxFunctions::CaseType caseType)//载荷列表
    {
        // 按表核对 - 载荷(Load)创建菜单项及按工况过滤规则
        // 当前菜单项：力、力矩、压强、重力、热流密度、热辐射、对流换热、时域载荷、
        //          频域载荷、组合载荷、螺栓载荷、声学载荷、声学频域载荷、谱载荷、旋转体力
        // 过滤规则：isLoadBoundarySupportedByCase(caseType, type) 返回 false 时不显示该项
        QAction* menuaction = nullptr;
        auto menu = new mUiMenu();
        menuTitle->setMenu(menu);
        auto addLoadAction = [&](const QString& icon, const QString& text, MxFunctions::LoadBoundaryType type) {
            if (!isLoadBoundarySupportedByCase(caseType, type))
                return;
            QAction* action = new QAction(QIcon(icon), text);
            connect(action, &QAction::triggered, this, [this, type]() {
                slot_createLoad(type);
                });
            menu->addAction(action);
            };
        addLoadAction(":/mMainWidgets/Icon/Case/li.png", "力", LOAD_IM_FORCE);
        addLoadAction(":/mMainWidgets/Icon/Case/liju.png", "力矩", LOAD_IM_TORQUE);
        addLoadAction(":/mMainWidgets/Icon/Case/yaqiang.png", "压强", LOAD_IM_PRESSURE);
        addLoadAction(":/mMainWidgets/Icon/Case/zhongli.png", "重力", LOAD_IM_GRAVITY);
        addLoadAction(":/mMainWidgets/Icon/Case/luoshuanzaihe.png", "螺栓载荷", LOAD_IM_BOLT);
        addLoadAction(":/mMainWidgets/Icon/load/旋转体力@3x.png", "旋转体力", LOAD_ROTATE_PHYSICAL_POWER);
        addLoadAction(":/mMainWidgets/Icon/load/biaomianzaihe.png", "表面载荷", LOAD_IM_SURFACE);
        addLoadAction(":/mMainWidgets/Icon/Case/bianzaihe.png", "边载荷", LOAD_IM_EDGE);

        addLoadAction(":/mMainWidgets/Icon/Case/reliumidu.png", "热流密度", LOAD_IM_HEATFLUX);
        addLoadAction(":/mMainWidgets/Icon/Case/refushe.png", "热辐射", LOAD_IM_RADIATION);
        addLoadAction(":/mMainWidgets/Icon/Case/duiliuhuanre.png", "对流换热", LOAD_IM_CONVECTION);

        addLoadAction(":/mMainWidgets/Icon/Case/shuntaizaihe.png", "时域载荷", LOAD_IM_TRANSIENT);
        addLoadAction(":/mMainWidgets/Icon/Case/pinyuzaihe.png", "频域载荷", LOAD_IM_FREQDOMAIN);
        addLoadAction(":/mMainWidgets/Icon/Case/zuhezaihe.png", "组合载荷", LOAD_IM_COMBINED);
        addLoadAction(":/mMainWidgets/Icon/Case/puzaihe.png", "谱载荷", LOAD_IM_SPECTRUM);
        addLoadAction(":/mMainWidgets/Icon/Case/shengxuejizhongli.png", "声学载荷", LOAD_IM_ACOCONCFORCE);
        addLoadAction(":/mMainWidgets/Icon/Case/shengxuepinyuzaihe.png", "声学频域载荷", LOAD_IM_ACOFREQDOMAIN);
       
        

        if (addDelete)
        {
            QAction* action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有载荷");
            connect(action, SIGNAL(triggered()), this, SLOT(slot_menuLoadDelete()));
        }
    }

    void mTreeView::createLoadChildMenu()
    {
        // 按表核对 - 载荷子节点右键菜单项
        // 当前菜单项：删除、重命名、复制、抑制/取消抑制
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuLoadChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyLoad()));
        m_Menu->addSeparator();
        const bool isSuppressed = isCurrentSelectedLoadBoundarySuppressed();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yizhi.png"), isSuppressed ? "取消抑制" : "抑制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSuppressLoadBoundary()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createRotateChildMenu()
    {
        // 转子分析子节点右键菜单项：删除、重命名、复制
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuRotateChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyRotate()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createBoundaryParentMenu(MxFunctions::CaseType caseType)
    {
        QAction* action = nullptr;
        QAction* menuaction = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建边界");
        BoundaryParentMenu(menuaction, true, caseType);
        m_Menu->exec(QCursor::pos());
    }

 
    void mTreeView::add2SetParentMenu(QAction* menuTitle, bool addDelete)
    {
        QAction* menuaction = nullptr;
        auto menu = new mUiMenu();
        menuTitle->setMenu(menu);
        auto addSetAction = [&](const QString& icon, const QString type)
            {
            QAction* action = new QAction(QIcon(icon), type);
            connect(action, &QAction::triggered, this, [this, type]() {
                //常量名字创建后有用于后面的判断
                mInfoWindow::getInstance()->createSet(type);
                });
            menu->addAction(action);
            };

        addSetAction(":/mMainWidgets/Icon/choice/dingdian3x.png","几何点集");
        addSetAction(":/mMainWidgets/Icon/choice/bian3x.png", "几何线集");
        addSetAction(":/mMainWidgets/Icon/choice/mian3x.png", "几何面集");
        addSetAction(":/mMainWidgets/Icon/choice/jiheti3x.png", "几何实体集");
        addSetAction(":/mMainWidgets/Icon/choice/jiedian3x.png", "节点集");
        addSetAction(":/mMainWidgets/Icon/choice/danyuanbian3x.png", "单元边集");
        addSetAction(":/mMainWidgets/Icon/choice/jiexibiaomian3x.png", "解析表面集");
        addSetAction(":/mMainWidgets/Icon/choice/danyuanmian3x.png", "单元表面集");
        addSetAction(":/mMainWidgets/Icon/choice/danyuan3x.png", "单元集");
        addSetAction(":/mMainWidgets/Icon/choice/bujian3x.png", "部件集");
        addSetAction(":/mMainWidgets/Icon/choice/freqPoint@3x.png", "频率点集");
        addSetAction(":/mMainWidgets/Icon/choice/dofNode@3x.png", "自由度节点集");
        addSetAction(":/mMainWidgets/Icon/choice/panel@3x.png", "面板集");
        addSetAction(":/mMainWidgets/Icon/choice/jiedianmian.png", "节点面集");

    }

    void mTreeView::BoundaryParentMenu(QAction* menuTitle, bool addDelete, MxFunctions::CaseType caseType)//边界条件列表
    {
        // 按表核对 - 边界条件(Boundary)创建菜单项及按工况过滤规则
        // 当前菜单项：固定约束、位移/转角、温度、速度、加速度、虚拟约束、超单元边界、位移边界、温度边界
        // 过滤规则：isLoadBoundarySupportedByCase(caseType, type) 返回 false 时不显示该项
        QAction* menuaction = nullptr;
        auto menu = new mUiMenu();
        menuTitle->setMenu(menu);
        auto addBoundaryAction = [&](const QString& icon, const QString& text, MxFunctions::LoadBoundaryType type) {
            if (!isLoadBoundarySupportedByCase(caseType, type))
                return;
            QAction* action1 = new QAction(QIcon(icon), text);
            connect(action1, &QAction::triggered, this, [this, type]() {
                slot_createBoundary(type);
                });
            menu->addAction(action1);
            };
        addBoundaryAction(":/mMainWidgets/Icon/Case/gudingyueshu.png", "固定约束", BOUNDARY_IM_FIXED);
        addBoundaryAction(":/mMainWidgets/Icon/Case/weiyizhuanjiao.png", "位移/转角", BOUNDARY_IM_DISPLACEMENT);
        addBoundaryAction(":/mMainWidgets/Icon/Case/旋转速度@3x.png", "速度", BOUNDARY_IM_SPEED);
        addBoundaryAction(":/mMainWidgets/Icon/Case/旋转加速度@3x.png", "加速度", BOUNDARY_IM_ACC);
        addBoundaryAction(":/mMainWidgets/Icon/Case/wendu.png", "温度边界", BOUNDARY_IM_TEMP);
        addBoundaryAction(":/mMainWidgets/Icon/Case/xuniyueshu.png", "虚拟约束", BOUNDARY_IM_VIRTUAL);
        addBoundaryAction(":/mMainWidgets/Icon/Case/tongyongyueshu.png", "超单元边界", BOUNDARY_IM_SUPERELEMENT);

        addBoundaryAction(":/mMainWidgets/Icon/load/位移边界@3x.png", "子模型位移边界", BOUNDARY_MOVE);
        addBoundaryAction(":/mMainWidgets/Icon/load/温度边界@3x.png", "子模型温度边界", BOUNDARY_TEMP_NEW);

        if (addDelete)
        {
            QAction* action1 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有边界");
            connect(action1, SIGNAL(triggered()), this, SLOT(slot_menuBoundaryDelete()));
        }
    }

    void mTreeView::createPredefinedFieldParentMenu(MxFunctions::CaseType caseType)
    {
        m_Menu->clear();
        QAction* action = nullptr;
        QAction* menuaction = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建预定义场");
        PredefinedFieldParentMenu(menuaction, true, caseType);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::PredefinedFieldParentMenu(QAction* menuTitle, bool addDelete, MxFunctions::CaseType caseType)
    {
        // 按表核对 - 预定义场(PredefinedField)创建菜单项及按工况过滤规则
        // 当前菜单项：初始温度、温度载荷
        // 过滤规则：isPredefinedFieldSupportedByCase(caseType, type) 返回 false 时不显示该项
        QAction* menuaction = nullptr;
        auto menu = new mUiMenu();
        menuTitle->setMenu(menu);
        auto addPredefinedFieldAction = [&](const QString& icon, const QString& text, MxFunctions::PredefinedFieldType type) {
            if (!isPredefinedFieldSupportedByCase(caseType, type))
                return;
            QAction* action = new QAction(QIcon(icon), text);
            connect(action, &QAction::triggered, this, [this, type]() {
                slot_createPredefinedField(type);
                });
            menu->addAction(action);
            };
        addPredefinedFieldAction(":/mMainWidgets/Icon/Case/chushiwendu.png", "初始温度", MxFunctions::PREDEFINED_FIELD_INIT_TEMP);
        addPredefinedFieldAction(":/mMainWidgets/Icon/Case/wenduzaihe.png", "温度载荷", MxFunctions::PREDEFINED_FIELD_TEMP_LOAD);

        if (addDelete)
        {
            QAction* action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有预定义场");
            connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPredefinedFieldDelete()));
        }
    }

    void mTreeView::createPredefinedFieldChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPredefinedFieldChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyPredefinedField()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createBoundaryChildMenu()
    {
        // 按表核对 - 边界条件子节点右键菜单项
        // 当前菜单项：删除、重命名、复制、抑制/取消抑制
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuBoundaryChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyBoundary()));
        m_Menu->addSeparator();
        const bool isSuppressed = isCurrentSelectedLoadBoundarySuppressed();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yizhi.png"), isSuppressed ? "取消抑制" : "抑制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSuppressLoadBoundary()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCaseSettingChildMenu()
    {

    }












    void mTreeView::createNoteParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "二维线");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickLineSig();
        });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "箭头");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickArrowSig();
        });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "文本");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickTextSig();
        });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "Log");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickLogSig();
        });
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMaterialsParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_createMaterial()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "批量重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuBatchRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除全部材料");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMaterialDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMaterialsChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMaterial()));
        //m_Menu->addSeparator();
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "抑制");

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMaterialChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "显示当前材料");
        connect(action, &QAction::triggered, this, [this]() { slot_menuShowHideMaterial(false); });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏当前材料");
        connect(action, &QAction::triggered, this, [this]() { slot_menuShowHideMaterial(true); });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/MaterialWindow/addtomaterials@3x.png"), "添加到自定义材料库");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuAddToMaterialDepot()));


        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createInteractionsParentMenu()
    {
        // 按表核对 - 相互作用(Interaction)父节点右键菜单项
        // 当前菜单项：创建接触（面面/点面/自接触/过盈/通用）、删除所有接触、
        //          创建连接、创建连接副、创建周期性边界（平移/旋转）、删除所有周期性边界
        // 注意：该节点在部分工况下会被 removeCaseParentItemsByCaseType 移除，
        //       需与图片中“相互作用节点”列的规则保持一致。
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建接触");

        QMenu* menuCreateContact = new mUiMenu();
        action->setMenu(menuCreateContact);
        auto addmenuContactAction = [&](const QString& icon, const QString& text, ContactType type) {
            action = menuCreateContact->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createContact(type);
                });
            };
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/mianmianjiechu.png", "面面接触", SurfaceToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/dianmianjiechu.png", "点面接触", NodeToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/dianmianjiechu.png", "线面接触", LineToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/tongyongjiechu.png", "通用接触", CommonContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/zijiechu.png", "自接触", SelfContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/guoyingjiechu.png", "过盈接触", InterferenceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/mianmianjiechu.png", "二维接触", TwoDimensionalContact);


        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有接触");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuContactDelete()));
        m_Menu->addSeparator();

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建连接");
        ConnectorsParentMenu(action, true);
        m_Menu->addSeparator();

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建连接副");
        DeputiesParentMenu(action, true);

        m_Menu->addSeparator();

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建周期性边界");
        QMenu* menuCreatePeriodicBoundary = new mUiMenu();
        action->setMenu(menuCreatePeriodicBoundary);
        auto addPeriodicBoundaryAction = [&](const QString& icon, const QString& text, MxFunctions::PeriodicBoundaryType type) {
            action = menuCreatePeriodicBoundary->addAction(QIcon(icon), text);
            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createPeriodicBoundary(type);
                });
            };
        addPeriodicBoundaryAction(":/mMainWidgets/Icon/Connector/pingyibianjie.png", "平移边界", MxFunctions::PeriodicBoundary_Translation);
        addPeriodicBoundaryAction(":/mMainWidgets/Icon/Connector/xuanzhuanbianjie.png", "旋转边界", MxFunctions::PeriodicBoundary_Rotation);

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有周期性边界");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPeriodicBoundaryDelete()));

        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createConnectorsParentMenu()
    {
        m_Menu->clear();
        QAction* action2 = nullptr;
        action2 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建连接");
        ConnectorsParentMenu(action2,true);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createDeputiesParentMenu()
    {
        m_Menu->clear();
        QAction* action2 = nullptr;
        action2 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建连接副");
        DeputiesParentMenu(action2,true);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::ConnectorsParentMenu(QAction* menuTitle, bool addDelete) //连接列表
    {
        QAction* action = nullptr;
        QMenu* ConnectorsParent = new mUiMenu();
        menuTitle->setMenu(ConnectorsParent);
        auto addConnectorsParentAction = [&](const QString& icon, const QString& text, MxFunctions::ConnectorType type) {
            action = ConnectorsParent->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createConnector(type);
                });
            };
        addConnectorsParentAction(":/mMainWidgets/Icon/Connector/zuni.png", "绑定", MxFunctions::Connector_Bind);
        addConnectorsParentAction(":/mMainWidgets/Icon/Connector/gangxinglianjie.png", "刚性连接", MxFunctions::Connector_Rigid);
        addConnectorsParentAction(":/mMainWidgets/Icon/Connector/rouxinglianjie.png", "柔性连接", MxFunctions::Connector_Flexible);
        addConnectorsParentAction(":/mMainWidgets/Icon/Connector/duodianyueshu.png", "MPC约束", MxFunctions::Connector_MPC);
        addConnectorsParentAction(":/mMainWidgets/Icon/Connector/lisangangti.png", "离散刚体", MxFunctions::Connector_Disperse);
        addConnectorsParentAction(":/mMainWidgets/Icon/Connector/luoshuanyueshu.png", "螺栓约束", MxFunctions::Connector_Bolt);
        addConnectorsParentAction(":/mMainWidgets/Icon/Connector/jiexigangti.png", "解析刚体", MxFunctions::Connector_Rigidbody);
        addConnectorsParentAction(":/mMainWidgets/Icon/BrowserTree/fenbushiouhelianjie.png", "分布式耦合", MxFunctions::Connector_DistributedCoupling);
        addConnectorsParentAction(":/mMainWidgets/Icon/BrowserTree/zhouchengdanyuan.png", "轴承单元", MxFunctions::Connector_BearingUnits);

        if (addDelete)
        {
            QAction* action2 = nullptr;
            action2 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有连接");
            connect(action2, SIGNAL(triggered()), this, SLOT(slot_menuConnectorDelete()));
        }
    }

    void mTreeView::DeputiesParentMenu(QAction* menuTitle, bool addDelete) //连接副列表
    {
        QAction* action = nullptr;
        QMenu* DeputiesParent = new mUiMenu();
        menuTitle->setMenu(DeputiesParent);
        auto addDeputiesParentAction = [&](const QString& icon, const QString& text, MxFunctions::DeputyType type) {
            action = DeputiesParent->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createDeputy(type);
                });
            };
        addDeputiesParentAction(":/mMainWidgets/Icon/Connector/lianjieqi.png", "连接器", MxFunctions::Deputy_Connector);
        addDeputiesParentAction(":/mMainWidgets/Icon/Connector/fastener.png", "fastener", MxFunctions::Deputy_Fastener);

        if (addDelete)
        {
            QAction* action2 = nullptr;
            action2 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有连接副");
            connect(action2, SIGNAL(triggered()), this, SLOT(slot_menuDeputyDelete()));
        }
    }

    void mTreeView::createConnectorsChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuConnectorChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyConnector()));
        m_Menu->addSeparator();
        const bool isSuppressed = isCurrentSelectedConnectorSuppressed();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yizhi.png"), isSuppressed ? "取消抑制" : "抑制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSuppressConnector()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createDeputiesChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuDeputyChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyDeputy()));
        m_Menu->addSeparator();
        const bool isSuppressed = isCurrentSelectedDeputySuppressed();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yizhi.png"), isSuppressed ? "取消抑制" : "抑制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSuppressDeputy()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createModelFixParentMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建单元生死");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createElementSetCtrl();
            });

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有模型修改");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuModelFixDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createModelFixChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuModelFixChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyModelFix()));
        m_Menu->addSeparator();
        const bool isSuppressed = isCurrentSelectedModelFixSuppressed();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yizhi.png"), isSuppressed ? "取消抑制" : "抑制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSuppressModelFix()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCaseModelFixParentMenu()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)
        {
            return;
        }
        int caseID = getPhyIdByModelIndex(idxs[0]);

        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建单元生死");
        connect(action, &QAction::triggered, this, [this, caseID]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createElementSetCtrl(caseID);
            });

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有模型修改");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCaseModelFixDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCaseModelFixChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCaseModelFixChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyCaseModelFix()));
        m_Menu->addSeparator();
        const bool isSuppressed = isCurrentSelectedModelFixSuppressed();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yizhi.png"), isSuppressed ? "取消抑制" : "抑制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSuppressModelFix()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createContactParentMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建接触");

        QMenu* menuCreateContact = new mUiMenu();
        action->setMenu(menuCreateContact);
        auto addmenuContactAction = [&](const QString& icon, const QString& text, ContactType type) {
            action = menuCreateContact->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createContact(type);
                });
            };
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/mianmianjiechu.png", "面面接触", SurfaceToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/dianmianjiechu.png", "点面接触", NodeToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/dianmianjiechu.png", "线面接触", LineToSurfaceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/tongyongjiechu.png", "通用接触", CommonContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/zijiechu.png", "自接触", SelfContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/guoyingjiechu.png", "过盈接触", InterferenceContact);
        addmenuContactAction(":/mMainWidgets/Icon/BrowserTree/mianmianjiechu.png", "二维接触", TwoDimensionalContact);


        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有接触");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuContactDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createPeriodicBoundaryParentMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建周期性边界");
        QMenu* menuCreatePeriodicBoundary = new mUiMenu();
        action->setMenu(menuCreatePeriodicBoundary);
        auto addPeriodicBoundaryAction = [&](const QString& icon, const QString& text, MxFunctions::PeriodicBoundaryType type) {
            action = menuCreatePeriodicBoundary->addAction(QIcon(icon), text);
            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createPeriodicBoundary(type);
                });
            };
        addPeriodicBoundaryAction(":/mMainWidgets/Icon/BrowserTree/pingyibianjie.png", "平移边界", MxFunctions::PeriodicBoundary_Translation);
        addPeriodicBoundaryAction(":/mMainWidgets/Icon/BrowserTree/xuanzhuanbianjie.png", "旋转边界", MxFunctions::PeriodicBoundary_Rotation);

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有周期性边界");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPeriodicBoundaryDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createPeriodicBoundaryChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPeriodicBoundaryChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyPeriodicBoundary()));
        m_Menu->addSeparator();
        const bool isSuppressed = isCurrentSelectedPeriodicBoundarySuppressed();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yizhi.png"), isSuppressed ? "取消抑制" : "抑制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSuppressPeriodicBoundary()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createContactChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuContactChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyContact()));
        m_Menu->addSeparator();
        const bool isSuppressed = isCurrentSelectedContactSuppressed();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/yizhi.png"), isSuppressed ? "取消抑制" : "抑制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSuppressContact()));
        m_Menu->exec(QCursor::pos());
    }

    bool mTreeView::isCurrentSelectedPeriodicBoundarySuppressed()
    {
        QModelIndexList idxs = getSelectIndex();
        bool allSuppressed = true;
        bool hasItem = false;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == PeriodicBoundaryChildItem)
            {
                hasItem = true;
                int phyID = getPhyIdByModelIndex(idx);
                auto pb = getCVM->getModelData()->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getPeriodicBoundaryData(phyID);
                if (pb == nullptr || !pb->isSuppressed())
                {
                    allSuppressed = false;
                    break;
                }
            }
        }
        return hasItem && allSuppressed;
    }

    void mTreeView::slot_menuSuppressPeriodicBoundary()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        bool toSuppress = !isCurrentSelectedPeriodicBoundarySuppressed();
        std::vector<int> phyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != PeriodicBoundaryChildItem) continue;
            int phyID = getPhyIdByModelIndex(idx);
            auto pb = getCVM->getModelData()->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getPeriodicBoundaryData(phyID);
            if (pb == nullptr) continue;
            pb->setSuppressed(toSuppress);
            phyIDs.push_back(phyID);
        }
        if (!phyIDs.empty())
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_PeriodicBoundary, phyIDs } });
        }
    }

    void mTreeView::slot_menuPeriodicBoundaryDelete()
    {
        deleteSymbol();
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_PeriodicBoundary);
        //先通知面板丢弃悬垂指针（此时周期边界对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyPeriodicBoundaryDataAboutToBeDestroyed(-1);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_PeriodicBoundary);
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        mTreeItem* _pbItem = _treeModel->getParentItem(PeriodicBoundaryParentItem);
        removeTreeItem(_pbItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PeriodicBoundary, phyIDs);
        selectedItem(_interItem);
    }

    void mTreeView::slot_menuPeriodicBoundaryChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _pbItem = _treeModel->getParentItem(PeriodicBoundaryParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == PeriodicBoundaryChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时周期边界对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyPeriodicBoundaryDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_PeriodicBoundary, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PeriodicBoundary, phyIDs);

        int pbCount = getCVM->getModelData()->getAllPhyID(Physical_PeriodicBoundary).size();
        if (pbCount == 0 && _pbItem != nullptr)
        {
            removeProxyIndexs(deleteItems);
            removeTreeItem(_pbItem);
            selectedItem(_treeModel->getParentItem(InteractionParentItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_pbItem);
        }
    }

    void mTreeView::slot_menuCopyPeriodicBoundary()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _pbItem = _treeModel->getParentItem(PeriodicBoundaryParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == PeriodicBoundaryChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_PeriodicBoundary, getPhyIdByModelIndex(idx));
                mPeriodicBoundary* pb = getCVM->getModelData()->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getPeriodicBoundaryData(copyId);
                if (pb == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), PeriodicBoundaryChildItem, pb->getPeriodicBoundaryName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_pbItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_PeriodicBoundary, copyIDs } });

        if (lastItem) selectedItem(lastItem);    }

    void mTreeView::slot_updatePeriodicBoundaryTree(bool isCreate, int pbID)
    {
        auto pb = getCVM->getModelData()->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getPeriodicBoundaryData(pbID);
        if (pb == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        mTreeItem* _pbItem = _treeModel->getParentItem(PeriodicBoundaryParentItem);
        if (_pbItem == nullptr)
        {
            _pbItem = new mTreeItem(modelID, PeriodicBoundaryParentItem, "周期性边界");
            _treeModel->appendChildren(_interItem, _pbItem);
        }

        QVector<QPair<QString, QString>> info;
        BrowserTreeStatus status = getCVM->getModelData()->getDataStatus(info, Physical_PeriodicBoundary, pbID);
        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, PeriodicBoundaryChildItem, pb->getPeriodicBoundaryName(), pbID, status);
            _treeModel->appendChildren(_pbItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto childitem = _treeModel->getChildrenItem(PeriodicBoundaryChildItem, pbID);
            setTreeItemText(childitem, pb->getPeriodicBoundaryName());
            setTreeItemStatus(childitem, status);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeriodicBoundary, pbID);
    }

    void mTreeView::createCaseInteractionParentMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建过盈接触");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createContact(InterferenceContact);
        });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有接触");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuContactDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCaseInteractionChildMenu()
    {
        m_Menu->clear();
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCaseInteractionChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyCaseInteraction()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMeshPanelParentMenu()    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shanchu.png"), "一键删除");
        if (mInfoWindow::getInstance()->isMeshing == false)
        {
            action->setEnabled(true);
        }
        else
        {
            action->setEnabled(false);
        }
        connect(action, SIGNAL(triggered()), this, SLOT(slot_deleteMeshAuto()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "生成");
        connect(action, &QAction::triggered, this, [=]() {
            // 集成一键生成：若网格剖分父节点无子节点则一键生成，否则按设置生成
            auto* meshParentItem = _treeModel->getParentItem(MeshParentItem);
            bool hasChildren = (meshParentItem && meshParentItem->getChildrenSize() > 0);
            if (hasChildren)
                slot_createMeshBySetting();
            else
                slot_createMeshAuto();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除设置");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMeshPanelDelete()));
        m_Menu->addSeparator();
        auto actionMesh = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "定义");
        QMenu* menuCreateMesh = new mUiMenu();
        actionMesh->setMenu(menuCreateMesh);
        auto addMeshAction = [&](QMenu* menu, const QString& icon, const QString& text, int type) {
            action = menu->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createMeshPanel(type);
                });
            };
        auto addMeshOptAction = [&](QMenu* menu, const QString& icon, const QString& text, int type) {
            action = menu->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createMeshOptPanel(type);
                });
            };
        addMeshAction(menuCreateMesh, ":/mMainWidgets/Icon/Mesh/ti.png", "体网格", SolidMesh);
        addMeshAction(menuCreateMesh, ":/mMainWidgets/Icon/Mesh/mian.png", "面网格", FaceMesh);
        addMeshAction(menuCreateMesh, ":/mMainWidgets/Icon/Mesh/xian.png", "线网格", LineMesh);


        auto actionControl = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "网格控制");
        QMenu* menuControl = new mUiMenu();
        actionControl->setMenu(menuControl);

        
        //action = menuControl->addAction(QIcon(":/mMainWidgets/Icon/Mesh/jubuchicun.png"), "全局尺寸");

        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createGlobalDimensions();
            });
            
        addMeshAction(menuControl, ":/mMainWidgets/Icon/Mesh/jubuchicun.png", "局部尺寸", MeshPanelType::LocalSize);
        addMeshAction(menuControl, ":/mMainWidgets/Icon/Mesh/zishiyin.png", "网格自适应", AutoAdapt);
        addMeshAction(menuControl, ":/mMainWidgets/Icon/Mesh/jiami.png", "自定义加密", CustomEncryption);
        addMeshAction(menuControl, ":/mMainWidgets/Icon/Mesh/splitEdge.png", "边抑制", EdgeSuppress);

        auto actionCreate = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "网格创建");
        QMenu* menuCreate = new mUiMenu();
        actionCreate->setMenu(menuCreate);

        addMeshOptAction(menuCreate, ":/mMainWidgets/Icon/Mesh/createPoint.png", "创建节点", CreatePoint);
        addMeshOptAction(menuCreate, ":/mMainWidgets/Icon/Mesh/createMesh.png", "创建网格", CreateMesh);
        addMeshOptAction(menuCreate, ":/mMainWidgets/Icon/Mesh/createMeta.png", "创建元网格", CreateMetaMesh);
        addMeshOptAction(menuCreate, ":/mMainWidgets/Icon/Mesh/extractSuface.png", "提取表面网格", ExtractSurface);
        addMeshOptAction(menuCreate, ":/mMainWidgets/Icon/Mesh/stretch.png", "拉伸网格", MeshPanelType::MeshStretch);
        addMeshOptAction(menuCreate, ":/mMainWidgets/Icon/Mesh/rotateMesh.png", "旋转网格", RotateMesh);

        auto actionTran = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "网格变换");
        QMenu* menuTran = new mUiMenu();
        actionTran->setMenu(menuTran);

        addMeshOptAction(menuTran, ":/mMainWidgets/Icon/Mesh/translate.png", "平移", MeshPanelType::Translation);
        addMeshOptAction(menuTran, ":/mMainWidgets/Icon/Mesh/xuanzhuan2.png", "旋转", MeshPanelType::Rotation);
        addMeshOptAction(menuTran, ":/mMainWidgets/Icon/Mesh/mirror.png", "镜像", MeshPanelType::Mirroring);
        addMeshOptAction(menuTran, ":/mMainWidgets/Icon/Mesh/zhenglie.png", "阵列", MeshPanelType::Matrix);
        addMeshOptAction(menuTran, ":/mMainWidgets/Icon/Mesh/zoom.png", "缩放", MeshPanelType::MeshZoom);

        auto actionEdit = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "网格编辑");
        QMenu* menuEdit = new mUiMenu();
        actionEdit->setMenu(menuEdit);

        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/MovePoint.png", "移动点", MovePoint);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/changeEdge.png", "交换边", MeshPanelType::MeshSmooth);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/collapseEdge.png", "合并边", MeshPanelType::MeshSmooth);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/splitEdge.png", "分割边", MeshPanelType::MeshSmooth);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/mergetri.png", "合并三角形", MeshPanelType::MeshSmooth);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/splitQuad.png", "分割四边形", MeshPanelType::MeshSmooth);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/changeOrder.png", "改变阶数", ChangeOrder);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/meshSmooth.png", "网格光滑", MeshPanelType::MeshSmooth);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/sharedNode.png", "共节点", SharedNode);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/remesh.png", "重剖分", Remeshing);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/renumber.png", "重编号", Renumber);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/normal.png", "法向", MeshNormal);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/moveMesh.png", "移动网格", MoveMesh);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/stlRemesh.png", "STL重剖分", STLRemesh);
        addMeshOptAction(menuEdit, ":/mMainWidgets/Icon/Mesh/deleteMesh.png", "网格删除", DeleteMesh);

        auto actionCheck = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "网格检查");
        QMenu* menuCheck = new mUiMenu();
        actionCheck->setMenu(menuCheck);
        addMeshOptAction(menuCheck, ":/mMainWidgets/Icon/Mesh/freeEdge.png", "自由边", MeshPanelType::MeshFreeEdge);
        addMeshOptAction(menuCheck, ":/mMainWidgets/Icon/Mesh/TEdge.png", "T形边", TEdge);
        addMeshOptAction(menuCheck, ":/mMainWidgets/Icon/Mesh/intersect.png", "相交", MeshIntersect);
        addMeshOptAction(menuCheck, ":/mMainWidgets/Icon/Mesh/overlap.png", "重叠", MeshOverlap);
        addMeshOptAction(menuCheck, ":/mMainWidgets/Icon/Mesh/interference.png", "干涉", MeshInterfere);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMeshPanelChildMenu()
    {
        QAction* action = nullptr;
        auto& meshData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel);
        int meshPanelType = meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]))->getMeshPanelType();
        if (meshPanelType == EdgeSuppress)
        {
            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chuangjianwangge.png"), "抑制");
            if (mInfoWindow::getInstance()->isMeshing == false)
                action->setEnabled(true);
            else
                action->setEnabled(false);
            if (meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]))->_list.isEmpty())
                action->setEnabled(false);
            connect(action, &QAction::triggered, this, [=]() { slot_createMeshEdgeSuppress(true); });

            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "取消抑制");
            if (mInfoWindow::getInstance()->isMeshing == false)
                action->setEnabled(true);
            else
                action->setEnabled(false);
            if (meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]))->_list.isEmpty())
                action->setEnabled(false);
            connect(action, &QAction::triggered, this, [=]() { slot_createMeshEdgeSuppress(false); });
        }
        else
        {
            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chuangjianwangge.png"), "生成网格");
            if (mInfoWindow::getInstance()->isMeshing == false)
                action->setEnabled(true);
            else
                action->setEnabled(false);
            if (meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]))->_list.isEmpty())
                action->setEnabled(false);
            connect(action, SIGNAL(triggered()), this, SLOT(slot_createMesh()));
        }
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        if (mInfoWindow::getInstance()->isMeshing == false)
            action->setEnabled(true);
        else
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMeshPanelChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/copyTargetObject3x.png"), "复制作用对象");
        QMenu* menuChildCopy = new mUiMenu(m_Menu);
        action->setMenu(menuChildCopy);

        int currentId = getPhyIdByModelIndex(getSelectIndex()[0]);
        auto* currentData = meshData.getMeshPanelData(currentId);
        if (!currentData)
        {
            m_Menu->exec(QCursor::pos());
            return;
        }
        auto currentType = currentData->getMeshPanelType();

        // 诊断：没有同类型数据时，至少显示一个提示项，避免子菜单为空
        auto ensureMenuNotEmpty = [&]() {
            if (menuChildCopy->isEmpty())
            {
                QAction* emptyAction = menuChildCopy->addAction("（无同类型可复用对象）");
                emptyAction->setEnabled(false);
            }
            };

        auto menuChildCopyAction = [&](const QString& text, int id) {
            QAction* subAction = menuChildCopy->addAction(text);
            connect(subAction, &QAction::triggered, this, [this, id, currentId]() {
                auto& meshData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel);
                auto* srcData = meshData.getMeshPanelData(id);
                auto* dstData = meshData.getMeshPanelData(currentId);
                if (srcData && dstData)
                {
                    // 追加模式：保留当前节点原有对象，添加源节点中不存在的对象
                    for (const QString& item : srcData->_list)
                    {
                        if (!dstData->_list.contains(item))
                            dstData->_list.append(item);
                    }
                    // 复制尺寸相关参数
                    dstData->setMeshShape(srcData->getMeshShape());
                    dstData->setMeshOrder(srcData->getMeshOrder());
                    // 切换 Info 面板到目标网格
                    mInfoWindow::getInstance()->editMeshPanel(currentId);
                    // 重新计算尺寸值并更新面板显示
                    auto* panel = mInfoWindow::getInstance()->getMeshPanel();
                    if (panel)
                    {
                        panel->recalculateSizeFromList();
                    }
                }
                });
            };

        vector<int> ids = meshData.getAllMeshPanelIDByType(currentType);
        for (auto id : ids)
        {
            if (id == currentId)
                continue;
            QString name = meshData.getMeshPanelNameByID(id);
            if (name.isEmpty())
                continue;
            menuChildCopyAction(name, id);
        }

        ensureMenuNotEmpty();
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createLocalSizePanelChildMenu()
    {
        QAction* action = nullptr;
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chuangjianwangge.png"), "生成网格");
        //auto& meshData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel);
        //if (mInfoWindow::getInstance()->isMeshing == false)
        //    action->setEnabled(true);
        //else
        //    action->setEnabled(false);
        //if (meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]))->_list.isEmpty())
        //    action->setEnabled(false);
        //connect(action, SIGNAL(triggered()), this, SLOT(slot_createMeshLocalSize()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "查看种子节点");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuLocalSizePreview()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        if (mInfoWindow::getInstance()->isMeshing == false)
            action->setEnabled(true);
        else
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMeshPanelChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/copyTargetObject3x.png"), "复制作用对象");
        QMenu* menuChildCopy = new mUiMenu(m_Menu);
        action->setMenu(menuChildCopy);
        auto menuChildCopyAction = [&](const QString& text, int id) {
            action = menuChildCopy->addAction(text);
            connect(action, &QAction::triggered, this, [this, id]() {
                auto& meshData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel);
                auto* srcData = meshData.getMeshPanelData(id);
                auto* dstData = meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]));
                if (srcData && dstData)
                {
                    // 追加模式：保留当前节点原有对象，添加源节点中不存在的对象
                    for (const QString& item : srcData->_list)
                    {
                        if (!dstData->_list.contains(item))
                            dstData->_list.append(item);
                    }
                    mInfoWindow::getInstance()->copyMeshChild();
                }
                });
            };
        if (getTextByModelIndex(getSelectIndex()[0]).contains("局部尺寸"))
        {
            vector<int> ids = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelIDByType(LineMesh);
            for (auto id : ids)
            {
                QString name = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getMeshPanelNameByID(id);
                if (name == getTextByModelIndex(getSelectIndex()[0]))
                    continue;
                menuChildCopyAction(name, id);
            }
        }
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCustomEncryption()
    {
        QAction* action = nullptr;
 
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        if (mInfoWindow::getInstance()->isMeshing == false)
            action->setEnabled(true);
        else
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCustomPanelChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "保存");
        //connect(action, SIGNAL(triggered()), this, SLOT(slot_menuBoxSave()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重置");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuBoxDelete()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "全部显示");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuBoxShow()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "全部隐藏");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuBoxHide()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "保存当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuBoxPreview()));
        /*action = m_Menu->addAction("复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMeshPanel()));*/
        m_Menu->exec(QCursor::pos());
    }
    void mTreeView::createAutoAdapt()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chuangjianwangge.png"), "生成网格");
        auto& meshData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel);
        if (mInfoWindow::getInstance()->isMeshing == false)
            action->setEnabled(true);
        else
            action->setEnabled(false);
        if (meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]))->_list.isEmpty())
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(slot_createMeshAutoAdapt()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        if (mInfoWindow::getInstance()->isMeshing == false)
            action->setEnabled(true);
        else
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMeshPanelChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/copyTargetObject3x.png"), "复制作用对象");
        QMenu* menuChildCopy = new mUiMenu(m_Menu);
        action->setMenu(menuChildCopy);
        auto menuChildCopyAction = [&](const QString& text, int id) {
            action = menuChildCopy->addAction(text);
            connect(action, &QAction::triggered, this, [this, id]() {
                auto& meshData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel);
                auto* srcData = meshData.getMeshPanelData(id);
                auto* dstData = meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]));
                if (srcData && dstData)
                {
                    // 追加模式：保留当前节点原有对象，添加源节点中不存在的对象
                    for (const QString& item : srcData->_list)
                    {
                        if (!dstData->_list.contains(item))
                            dstData->_list.append(item);
                    }
                    mInfoWindow::getInstance()->copyMeshChild();
                }
                });
            };
        if (getTextByModelIndex(getSelectIndex()[0]).contains("网格自适应"))
        {
            vector<int> ids = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getAllMeshPanelIDByType(AutoAdapt);
            for (auto id : ids)
            {
                QString name = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getMeshPanelNameByID(id);
                if (name == getTextByModelIndex(getSelectIndex()[0]))
                    continue;
                menuChildCopyAction(name, id);
            }
        }

        m_Menu->exec(QCursor::pos());
    }
    void mTreeView::createMeshOptPanelParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有网格操作");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMeshOptPanelDelete()));
        m_Menu->exec(QCursor::pos());
    }
    void mTreeView::createMeshOptPanelChildMenu()
    {
        QAction* action = nullptr;

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        if (mInfoWindow::getInstance()->isMeshing == false)
            action->setEnabled(true);
        else
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMeshOptPanelChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/copyTargetObject3x.png"), "复制作用对象");
        QMenu* menuChildCopy = new mUiMenu(m_Menu);
        action->setMenu(menuChildCopy);
        auto menuChildCopyAction = [&](const QString& text, int id) {
            action = menuChildCopy->addAction(text);
            connect(action, &QAction::triggered, this, [this, id]() {
                auto& meshData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel);
                auto* srcData = meshData.getMeshPanelData(id);
                auto* dstData = meshData.getMeshPanelData(getPhyIdByModelIndex(getSelectIndex()[0]));
                if (srcData && dstData)
                {
                    // 追加模式：保留当前节点原有对象，添加源节点中不存在的对象
                    for (const QString& item : srcData->_list)
                    {
                        if (!dstData->_list.contains(item))
                            dstData->_list.append(item);
                    }
                    mInfoWindow::getInstance()->copyMeshChild();
                }
                });
            };

        m_Menu->exec(QCursor::pos());
    }
    void mTreeView::createGlobalSimChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chuangjianwangge.png"), "删除");
        if (mInfoWindow::getInstance()->isMeshing == false)
            action->setEnabled(true);
        else
            action->setEnabled(false);
        connect(action, SIGNAL(triggered()), this, SLOT(slot_deleteGlobalMesh()));
        m_Menu->exec(QCursor::pos());
    }
    void mTreeView::createSectionShapeParentMenu()
    {
        QAction* action = nullptr;
        QAction* action3 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建梁截面");
        SectionShapeParent(action3, true);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::SectionShapeParent(QAction* menuTitle, bool addDelete) //截面形状列表
    {
        QAction* action = nullptr;
        QMenu* SectionShape = new mUiMenu();
        menuTitle->setMenu(SectionShape);
        auto addShapeAction = [&](const QString& icon, const QString& text, int type) {
            action = SectionShape->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createSectionShape(type);
                });
            };
        addShapeAction(":/mMainWidgets/Icon/sectionShape/H.png", "H", MxFunctions::Sec_H);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/L.png", "L", MxFunctions::Sec_L);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/I.png", "I", MxFunctions::Sec_I);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/I1.png", "I1", MxFunctions::Sec_I1);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/T.png", "T", MxFunctions::Sec_T);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/T1.png", "T1", MxFunctions::Sec_T1);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/T2.png", "T2", MxFunctions::Sec_T2);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/Z-3.png", "Z", MxFunctions::Sec_Z);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/he.png", "盒", MxFunctions::Sec_Box);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/he.png", "盒1", MxFunctions::Sec_Box1);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/juxing.png", "矩形", MxFunctions::Sec_Rec);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/yuan.png", "圆", MxFunctions::Sec_Cir);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/yuanhuan.png", "圆环", MxFunctions::Sec_Ann);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/cao.png", "槽", MxFunctions::Sec_Channel);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/cao.png", "槽1", MxFunctions::Sec_Channel1);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/cao1.png", "槽2", MxFunctions::Sec_Channel2);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/shizi.png", "十字", MxFunctions::Sec_Cross);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/maoxing.png", "帽形", MxFunctions::Sec_Hat);
        addShapeAction(":/mMainWidgets/Icon/sectionShape/maoxing.png", "自定义", MxFunctions::Sec_None);

        if (addDelete)
        {
            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除全部梁截面");
            connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSectionShapeDelete()));
        }
    }


    void mTreeView::createSectionShapeChildMenu()
    {
        QAction* action = nullptr;
        
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopySectionShape()));
        //m_Menu->addSeparator();
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Mesh/shengcheng.png"), "抑制");

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSectionShapeChildDel()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createFreqSamplPointParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuFreqSamplPoint()));

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除全部");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuFreqSamplPointDelete()));
        m_Menu->exec(QCursor::pos());

    }

    void mTreeView::createFreqSamplPointChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuFreqSamplPointChilDelete()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyFreqSamplPoint()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createLaminateManagerMenu()
    {
        m_Menu->clear();
        QAction* action2 = nullptr;
        action2 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");
        LaminateManagerMenu(action2, true);
        m_Menu->exec(QCursor::pos());

    }

    void mTreeView::LaminateManagerMenu(QAction* menuTitle, bool addDelete)
    {
        QAction* action = nullptr;
        QMenu* LaminateManager = new mUiMenu();
        menuTitle->setMenu(LaminateManager);
        auto addLaminateManagerAction = [&](const QString& icon, const QString& text, MxFunctions::LaminateType type, const QString& datatype = "") {
            action = LaminateManager->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->creatLaminate(type);
                });
            };
        addLaminateManagerAction(":/mMainWidgets/Icon/BrowserTree/dancengban.png", "单层板", MxFunctions::SglLayerPlate);
        addLaminateManagerAction(":/mMainWidgets/Icon/BrowserTree/quanjudancengban.png", "全局单层板", MxFunctions::GlobalSglLam);
        addLaminateManagerAction(":/mMainWidgets/Icon/BrowserTree/cengheban.png", "层合板", MxFunctions::LamPlate);

        if (addDelete)
        {
            QAction* action2 = nullptr;
            action2 = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有");
            connect(action2, SIGNAL(triggered()), this, SLOT(slot_menuDeleteAllLaminate()));
        }
    }

    void mTreeView::createSglLayerPlateParentMenu()
    {
        QAction* action = nullptr;

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建单层板");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSglLayerPlate()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建全局单层板");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGlobalSglLam()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有单层板");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSglLayerPlateDelete()));

        m_Menu->exec(QCursor::pos());

    }

    void mTreeView::createLamPlateParentMenu()
    {
        QAction* action = nullptr;

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建层合板");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuLamPlate()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有层合板");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuLamPlateDelete()));

        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createLaminateChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuLaminateChildDelete()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuLaminateChildCopy()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createGlobalParameterMenu()
    {
        QAction* action = nullptr;
        QAction* menuaction = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");
        GlobalParameterMenu(menuaction);
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::GlobalParameterMenu(QAction* menuTitle, bool addDelete)
    {
        QAction* menuaction = nullptr;
        auto menu = new mUiMenu();
        menuTitle->setMenu(menu);

        QAction*  action = new QAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "超单元参数设置");
        menuCreateSEParamSet(action);
        menu->addAction(action);

        action = new QAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "诊断优化");
        menuCreateDiagOpt(action);
        menu->addAction(action);

        action = new QAction(QIcon(":/mMainWidgets/Icon/BrowserTree/quanjushuchu.png"), "全局输出控制");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->createGlobalOutputControl();
            });
        menu->addAction(action);

        action = new QAction(QIcon(":/mMainWidgets/Icon/BrowserTree/qiujiekongzhi.png"), "求解控制");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->createSolveControl();
            });
        menu->addAction(action);

        if (addDelete)
        {
            menu->addSeparator();
            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有");
            connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGlobalParamDelete()));
        }
    }

    void mTreeView::menuCreateDiagOpt(QAction* menuTitle)
    {
        QAction* action = nullptr;
        QMenu* CreateDiagOpt = new mUiMenu();
        menuTitle->setMenu(CreateDiagOpt);
        auto addDiagOptAction = [&](const QString& icon, const QString& text)
            {
                action = CreateDiagOpt->addAction(QIcon(icon), text);
                connect(action, &QAction::triggered, this, [this, text]() {
                    if (text == "模态贡献量")
                    {
                        mInfoWindow::getInstance()->createModalContri(0);
                    }
                    else if (text == "节点贡献量")
                    {
                        mInfoWindow::getInstance()->createNodeContri(0);
                    }
                    else if (text == "面板贡献量")
                    {
                        mInfoWindow::getInstance()->createPanelContri(0);
                    }
                    else if (text == "传递路径贡献量")
                    {
                        mInfoWindow::getInstance()->createTPContri(0);
                    }
                    else if (text == "等效辐射功率")
                    {
                        mInfoWindow::getInstance()->createERP(0);
                    }
                    else if (text == "峰值筛选")
                    {
                        mInfoWindow::getInstance()->createPeakFilter();
                    }
                    });
            };

        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/motaigongxianliang.png", "模态贡献量");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/jiediangongxianliang.png", "节点贡献量");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/mianbangongxianliang.png", "面板贡献量");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/chuandilujinggongxianliang.png", "传递路径贡献量");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/dengxiaofushegonglv.png", "等效辐射功率");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/peakfilter.png", "峰值筛选");
    }

    void mTreeView::menuCreateSEParamSet(QAction* menuTitle)
    {
        QAction* action = nullptr;
        QMenu* CreateDiagOpt = new mUiMenu();
        menuTitle->setMenu(CreateDiagOpt);
        auto addDiagOptAction = [&](const QString& icon, const QString& text)
            {
                action = CreateDiagOpt->addAction(QIcon(icon), text);
                connect(action, &QAction::triggered, this, [this, text]() {
                    if (text == "超单元缩聚方法")
                    {
                        mInfoWindow::getInstance()->createGenericParam();
                    }
                    else if (text == "超单元调用")
                    {
                        mInfoWindow::getInstance()->createGenericAssign();
                    }
                    else if (text == "声固耦合对搜索")
                    {
                        mInfoWindow::getInstance()->createASCouplingSearch();
                    }
                    else if (text == "超单元矩阵")
                    {
                        mInfoWindow::getInstance()->createGenericMatrix();
                    }
                    else if (text == "超单元部件")
                    {
                        mInfoWindow::getInstance()->createGenericPart();
                    }
                    });
            };

        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/genericParam.png", "超单元缩聚方法");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/genericAssign.png", "超单元调用");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/ascouplingSearch.png", "声固耦合对搜索");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/genericMatrix.png", "超单元矩阵");
        addDiagOptAction(":/mMainWidgets/Icon/BrowserTree/genericPart.png", "超单元部件");
    }

    void mTreeView::createGenericParamParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->createGenericParam();
            });


        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGenericParamDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createGenericParamChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGenericParamChildDelete()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGenericParamChildCopy()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createGenericAssignMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGenericAssignDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createASCouplingSearchMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuASCouplingSearchDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createGenericMatrixMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGenericMatrixDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createERPParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuERPDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createPeakFilterParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->createPeakFilter();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPeakFilterParentDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createPeakFilterChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPeakFilterChildDelete()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuPeakFilterChildCopy()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createCasePeakFilterMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_casePeakFilterDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createDiagOptMenu()
    {
        QAction* action = nullptr;
        //获取当前工况id
        QModelIndexList idxs = getSelectIndex();
        BrowserTreeType type = getTreeTypeByModelIndex(idxs[0].parent());
        //频响下只有模态和峰值筛选 声固耦合下三个贡献量都有
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motaigongxianliang.png"), "模态贡献量");
        connect(action, &QAction::triggered, this, [this]()
            {
                //获取当前选择节点
                QModelIndexList idxs = getSelectIndex();
                //诊断优化父节点为工况
                int casId = getPhyIdByModelIndex(idxs[0].parent());
                mInfoWindow::getInstance()->createModalContri(casId);
            });

        if (type == CaseAsClouplingParentItem)
        {
            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/jiediangongxianliang.png"), "节点贡献量");
            connect(action, &QAction::triggered, this, [this]()
                {
                    //获取当前选择节点
                    QModelIndexList idxs = getSelectIndex();
                    //诊断优化父节点为工况
                    int casId = getPhyIdByModelIndex(idxs[0].parent());
                    mInfoWindow::getInstance()->createNodeContri(casId);
                });

            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/mianbangongxianliang.png"), "面板贡献量");
            connect(action, &QAction::triggered, this, [this]() {
                //获取当前选择节点
                QModelIndexList idxs = getSelectIndex();
                //诊断优化父节点为工况
                int casId = getPhyIdByModelIndex(idxs[0].parent());
                mInfoWindow::getInstance()->createPanelContri(casId);
                });
        }

        // 峰值筛选（频响分析 + 声固耦合分析）
        if (type == CaseImplicitFrequencyParentItem || type == CaseAsClouplingParentItem)
        {
            action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "峰值筛选");
            connect(action, &QAction::triggered, this, [this]() {
                QModelIndexList idxs = getSelectIndex();
                int casId = getPhyIdByModelIndex(idxs[0].parent());
                mInfoWindow::getInstance()->createCasePeakFilter(casId);
                });
        }

        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuDiagOptDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createContriParentMenu()
    {
        QAction* action = nullptr;
        QAction* menuaction = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建");
        ContriParentMenu(menuaction);
        m_Menu->addSeparator();

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuContriParentDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createContriChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuContriChildDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::ContriParentMenu(QAction* menuTitle, bool addDelete)
    {
        QAction* menuaction = nullptr;
        auto menu = new mUiMenu();
        menuTitle->setMenu(menu);
        QAction* action = new QAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motaigongxianliang.png"), "模态贡献量");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->createModalContri(0);
            });
        menu->addAction(action);
        action = new QAction(QIcon(":/mMainWidgets/Icon/BrowserTree/jiediangongxianliang.png"), "节点贡献量");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->createNodeContri(0);
            });
        menu->addAction(action);
        action = new QAction(QIcon(":/mMainWidgets/Icon/BrowserTree/mianbangongxianliang.png"), "面板贡献量");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->createPanelContri(0);
            });
        menu->addAction(action);
        action = new QAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chuandilujinggongxianliang.png"), "传递路径贡献量");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->createTPContri(0);
            });
        menu->addAction(action);
    }

    void mTreeView::createGenericPartMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGenericPartDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createSolveControlMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuSolveControlDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createGlobalOutputControlMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuGlobalOutputControlDelete()));
        m_Menu->exec(QCursor::pos());
    }
 
    void mTreeView::slot_viewportModelAboutToChange(int newViewportID)
    {
        //跨模型切换前（旧模型仍为当前模型）按退出面板语义收尾旧视口的符号显示。
        //setSymbolShow 只在树选中变化后对"当前模型视口"重算符号显示（先全部隐藏、
        //再按选中项显示），模型切换后它只会作用于新模型视口，旧视口的载荷/边界/
        //预定义场符号显示状态无人重算，会残留编辑会话的箭头。
        //此处将旧视口符号全部隐藏（与单模型下退出面板、选中变为空的表现一致），
        //切回该模型时 slot_subWindowActivated 的选中链路会按选中重新显示
        if (!getCVM)
        {
            return;
        }

        //同模型视口切换：符号显示状态保持（拾取会话不中断，与拾取控件处理一致）
        if (newViewportID != -1)
        {
            int newModelID = -1;
            if (VMManager->getViewportPrivateByID(newViewportID, &newModelID)
                && newModelID == getCVM->getModelData()->getModelDataID())
            {
                return;
            }
        }

        if (getCVM->isPostModel() || !getCVM->getPreRenderWidget())
        {
            return;
        }
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
        {
            return;
        }
        symbolController->setAllSymbolShowState(false);
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mTreeView::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
        if (!getCVM)
        {
            return;
        }
        if (m_viewportPrivate == VMManager->getCurrentViewportPrivate())
        {
            return;
        }
        m_viewportPrivate = VMManager->getCurrentViewportPrivate();

        //当前无有效渲染视口（如激活的是曲线图表卡片——卡片ID不在渲染视口注册表，
        //getCurrentViewportPrivate按ID查无返回空）：无模型树同步可做，直接返回。
        //注意此判空必须在Post2D判断之前：原"m_viewportPrivate && ..."写法在空指针时
        //反而不return，会带着空指针走到下方getTreeSelectItems()解引用崩溃
        if (m_viewportPrivate == nullptr)
        {
            return;
        }

        //曲线视口（ViewportPost2D，承载曲线模块图表）：不做模型树选中同步。
        //否则程序化改选触发 slot_itemSelectionChanged -> updateModelView，其中
        //"树选中前处理项且当前视口为后处理(REBUILD_PostRend)"分支会强制
        //activatePreViewport 把激活抢回模型视口，曲线视口点击后即被抢走无法选中/切换
        if (m_viewportPrivate && m_viewportPrivate->getViewportType() == ViewportPost2D)
            return;

        //曲线视口（ViewportPost2D，承载曲线模块图表）：不做模型树选中同步。
        //否则程序化改选触发 slot_itemSelectionChanged -> updateModelView，其中
        //"树选中前处理项且当前视口为后处理(REBUILD_PostRend)"分支会强制
        //activatePreViewport 把激活抢回模型视口，曲线视口点击后即被抢走无法选中/切换
        if (m_viewportPrivate && m_viewportPrivate->getViewportType() == ViewportPost2D)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        if (modelID != _treeModel->getCurrentModelID())
        {
            _treeModel->setCurrentModelID(modelID);
        }

        //按当前视口刷新前处理树窗口与各树窗口显隐
        updateTreeSectionsForViewport();

        QList<mItemKey> itemKeys = m_viewportPrivate->getTreeSelectItems();
        QModelIndexList poxyIndexs;
        QModelIndex idx, poxyIdx;
        for (auto key : itemKeys)
        {
            idx = _treeModel->indexFromKey(key);
            if (!idx.isValid()) continue;
            poxyIdx = _treeProxyModel->mapFromSource(idx);
            if (!poxyIdx.isValid()) continue;
            poxyIndexs.push_back(poxyIdx);
        }

        if (poxyIndexs.size() == 0)
        {
            //后处理模型激活：后处理树根不在本树模型中（已独立），
            //本树（此时被后处理树 dock 遮蔽）不做选中同步
            if (getCVM->isPostModel())
            {
                return;
            }
            mTreeItem* item = _treeModel->getParentItem(ModelRootItem);
            selectedItem(item);
        }
        else
        {
            // selectedItem 内部的 selectionChanged（或选区未变时的兜底）已经触发
            // slot_itemSelectionChanged -> slot_singleClickItem，这里不再重复调用，
            // 避免窗口切换时整条面板刷新/重绘链路执行两遍。
            selectedItem(poxyIndexs);
        }
    }

    void mTreeView::updateTreeSectionsForViewport()
    {
        if (!getCVM)
        {
            return;
        }
        auto priv = VMManager->getCurrentViewportPrivate();
        MxFunctions::ViewportType curType = priv ? priv->getViewportType() : MxFunctions::Viewport3D;
        int curModel = getCVM->getModelData()->getModelDataID();

        //同步当前模型ID：门面操作期间激活信号被断开，slot_subWindowActivated 不会执行，
        //若不同步，getParentItem 仍按旧模型查树，后续节点插入会落到错误（隐藏）的模型树上
        if (curModel != _treeModel->getCurrentModelID())
        {
            _treeModel->setCurrentModelID(curModel);
        }

        //前处理树窗口：仅显示当前模型的模型树。
        //用视图级行隐藏（setRowHidden）而非代理过滤：行不进出代理模型，
        //切回时保持原展开状态（代理过滤会让子树按默认折叠重新映射进来）
        //多体形态：隐式模型根整树隐藏（部件对象节点改挂多体"部件"固定组管理），
        //树上只保留多体动力学顶层根——与原型多体工作区的单根树一致
        const bool motionForm = MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion;
        for (int row = 0; row < _treeModel->rowCount(QModelIndex()); ++row)
        {
            QModelIndex srcIdx = _treeModel->index(row, 0, QModelIndex());
            const int rowType = srcIdx.data(Roles::TypeRole).toInt();
            if (rowType != ModelRootItem && rowType != Motion_RootItem)
            {
                continue;//后处理模型根由本窗口代理过滤，不在此处理
            }
            QModelIndex proxyIdx = _treeProxyModel->mapFromSource(srcIdx);
            if (!proxyIdx.isValid())
            {
                continue;
            }
            const bool hiddenByForm = (motionForm && rowType == ModelRootItem);
            //多体动力学顶层根与模型根同构：同样按 modelID 只显示当前模型的
            setRowHidden(proxyIdx.row(), QModelIndex(),
                hiddenByForm || srcIdx.data(Roles::ModelRole).toInt() != curModel);
        }

        //按当前视口类型切换 视图树/前处理树/后处理树/NVH 树窗口显隐（视图树常显）
        mDockLayoutManager::getInstance()->updateTreeWindows(curType);
    }

    void mTreeView::slot_rightClickItem()
    {
        // 按表核对 - 模型树右键菜单分发规则
        // 当前规则：
        // - 仅当 _isTreeRight 为 true 时响应右键
        // - 多选时要求所有选中项类型一致，否则不弹出菜单
        // - 根据当前节点类型(BrowserTreeType)分发到对应的 createXxxMenu() 函数
        //   例如：ModelRootItem -> createModelRootItem();
        //        CaseLoadParentItem/CaseRotateParentItem -> createLoadParentMenu(当前工况类型);
        //        CaseBCParentItem -> createBoundaryParentMenu(当前工况类型);
        //        CasePredefinedFieldParentItem -> createPredefinedFieldParentMenu(当前工况类型);
        if (MechCommon::mMechGlobalSignals::getInstance()->_isTreeRight)
        {
            QModelIndexList modelIndexs = getSelectIndex();;
            if (modelIndexs.size() == 0)
                return;

            BrowserTreeType type = getTreeTypeByModelIndex(modelIndexs[0]);
            for (auto idx : modelIndexs)
            {
                if (type != getTreeTypeByModelIndex(idx))
                    return;
            }

            if (m_Menu != nullptr)
            {
                delete m_Menu;
                m_Menu = nullptr;
            }

        m_Menu = new mUiMenu(this);
        ThemeManager::applyMenuStyle(m_Menu);
        switch (type)
            {
            case ModelRootItem:
                //多体形态：模型根隐藏（隐式菜单不可达），兜底弹多体根菜单
                if (MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion)
                {
                    this->createMotionRootMenu();
                }
                else
                {
                    this->createModelRootItem();
                }
                break;
            //后处理子树（PostTreeRootItem 及其后代）被本视图代理过滤，
            //其右键菜单由后处理树窗口（mPostTreeView）自有派发
            case ModelParentItem:
                this->createGeoItemMenu();
                break;
            case ModelChildItem:
                this->createGeoChildMenu();
                break;
            case SetParentItem:
                this->createSetParentMenu();
                break;
            case SetChildItem:
                this->createSetChildMenu();
                break;
            case OptimizationParentItem:
                this->createStructOptionParentMenu();
                break;
            case OptimizationDesignParentItem:
                this->createOptimizationDesignParentMenu();
                break;
            case OptimizationDesignChildItem:
                this->createOptimizationDesignChildMenu();
                break;
            case OptimizationConstraintsParentItem:
                this->createOptimizationConstraintParentMenu();
                break;
            case OptimizationConstraintsChildItem:
                this->createOptimizationConstraintChildMenu();
                break;
            case RunnerOptimizationParentItem:
                this->createRunnerOptimizationParentMenu();
                break;
            case RunnerOptimizationDesignParentItem:
                this->createRunnerOptimizationDesignParentMenu();
                break;
            case RunnerOptimizationDesignChildItem:
                this->createRunnerOptimizationDesignChildMenu();
                break;
            case RunnerOptimizationConstraintsParentItem:
                this->createRunnerOptimizationConstraintParentMenu();
                break;
            case RunnerOptimizationConstraintsChildItem:
                this->createRunnerOptimizationConstraintChildMenu();
                break;
            case PropertyParentItem:
                this->createPropertyParentMenu();
                break;
            case PropertyChildItem:
                this->createPropertyChildMenu();
                break;
            case NonStructuralQualityParentItem:
                this->createNonStructQualityParentMenu();
                break;
            case NonStructuralQualityChildItem:
                this->createNonStructQualityChildMenu();
                break;
            case FieldParentItem:
                this->createFieldParentMenu();
                break;
            case FieldChildItem:
                this->createFieldChildMenu();
                break;
            case CoordinateParentItem:
                this->createCoordinateParentMenu();
                break;
            case CoordinateChildItem:
            {
                if (getPhyIdByModelIndex(getSelectIndex()[0]) != -1)//不为全局坐标系才有右键菜单
                {
                    this->createCoordinateChildMenu();
                }
            }
            break;
            case CaseImplicitLinearStaticParentItem:
            case CaseImplicitNonLinearStaticParentItem:
            case CaseImplicitModalParentItem:
            case CaseImplicitResponseSpectrumParentItem:
            case CaseImplicitRandomVibrationParentItem:
            case CaseImplicitFrequencyParentItem:
            case CaseAsClouplingParentItem:
            case CaseImplicitGenericParentItem:
            case CaseImplicitTransientParentItem:
            case CaseImplicitNonLinearDynamicParentItem:
            case CaseImplicitLinearBucklingParentItem:
            case CaseImplicitPostBucklingParentItem:
            case CaseImplicitSteadyHeatParentItem:
            case CaseImplicitTransientHeatParentItem:
                this->createCaseParentMenu();
                break;
            case CaseLoadParentItem:
                this->createLoadParentMenu(getCurrentCaseTypeFromSelection());
                break;
            case CaseRotateParentItem:
                this->createRotateParentMenu();
                break;
            case CaseLoadChildItem:
                this->createLoadChildMenu();
                break;
            case CaseRotateChildItem:
                this->createRotateChildMenu();
                break;
            case CaseBCParentItem:
                this->createBoundaryParentMenu(getCurrentCaseTypeFromSelection());
                break;
            case CaseBCChildItem:
                this->createBoundaryChildMenu();
                break;
            case CasePredefinedFieldParentItem:
                this->createPredefinedFieldParentMenu(getCurrentCaseTypeFromSelection());
                break;
            case CasePredefinedFieldChildItem:
                this->createPredefinedFieldChildMenu();
                break;
            case CaseInteractionParentItem:
                this->createCaseInteractionParentMenu();
                break;
            case CaseInteractionChildItem:
                this->createCaseInteractionChildMenu();
                break;
            case PeriodicBoundaryParentItem:
                this->createPeriodicBoundaryParentMenu();
                break;
            case PeriodicBoundaryChildItem:
                this->createPeriodicBoundaryChildMenu();
                break;
            case CaseSettingChildItem:
                this->createCaseSettingChildMenu();
                break;
            //结果子节点（云图/动画/曲线/报告等）属后处理子树，其菜单由后处理树窗口派发
            case AssemblyParentItem:
                this->createAssemblyParentMenu();
                break;
            case AssemblyChildItem:
                this->createAssemblyChildMenu();
                break;
            //后处理结果/三维结果等子树右键已随树模型独立迁出（PostWindow::mPostTreeView 承接）
            case NoteParentItem:
                this->createNoteParentMenu();
                break;
            case FunctionParentItem:
                this->createFunctionParentMenu();
                break;
            case FunctionChildItem:
                this->createFunctionChildMenu();
                break;
            case MaterialParentItem:
                this->createMaterialsParentMenu();
                break;
            case MaterialChildItem:
                this->createMaterialsChildMenu();
                break;
            case MeshParentItem:
                this->createMeshPanelParentMenu();
                break;
            case MeshChildItem:
                if (getTextByModelIndex(getSelectIndex()[0]).contains("自定义加密"))
                {
                    this->createCustomEncryption();
                }
                else if (getTextByModelIndex(getSelectIndex()[0]).contains("网格自适应"))
                {
                    this->createAutoAdapt();
                }
                else if (getTextByModelIndex(getSelectIndex()[0]).contains("局部尺寸"))
                {
                    this->createLocalSizePanelChildMenu();
                }
                else if (getTextByModelIndex(getSelectIndex()[0]) != "全局尺寸")//不为全局尺寸才有右键菜单
                {
                    this->createMeshPanelChildMenu();
                }
                else if (getTextByModelIndex(getSelectIndex()[0]) == "全局尺寸")
                {
                    this->createGlobalSimChildMenu();
                }
                break;
            case MeshOptParentItem:
                this->createMeshOptPanelParentMenu();
                break;
            case MeshOptChildItem:
                if (getTextByModelIndex(getSelectIndex()[0]) != "全局尺寸")//不为全局尺寸才有右键菜单
                {
                    this->createMeshOptPanelChildMenu();
                }
                else if (getTextByModelIndex(getSelectIndex()[0]) == "全局尺寸")
                {
                    this->createGlobalSimChildMenu();
                }
                break;
            case SectionShapeParentItem:
                this->createSectionShapeParentMenu();
                break;
            case SectionShapeChildItem:
                this->createSectionShapeChildMenu();
                break;
            case InteractionParentItem:
                this->createInteractionsParentMenu();
                break;
            case ConnectorChildItem:
                this->createConnectorsChildMenu();
                break;
            case DeputyChildItem:
                this->createDeputiesChildMenu();
                break;
            case ConnectorParentItem:
                this->createConnectorsParentMenu();
                break;
            case DeputyParentItem:
                this->createDeputiesParentMenu();
                break;
            case ContactParentItem:
                this->createContactParentMenu();
                break;
            case ContactChildItem:
                this->createContactChildMenu();
                break;
            case ModelFixParentItem:
                this->createModelFixParentMenu();
                break;
            case ModelFixChildItem:
                this->createModelFixChildMenu();
                break;
            case CaseModelFixManagerItem:
                this->createCaseModelFixParentMenu();
                break;
            case CaseModelFixChildItem:
                this->createCaseModelFixChildMenu();
                break;
            case LoadManagerParentItem:
                this->createLoadParentMenu();
                break;
            case BoundaryManagerParentItem:
                this->createBoundaryParentMenu();
                break;
            case PredefinedFieldManagerParentItem:
                this->createPredefinedFieldParentMenu();
                break;
            case FrequencySamplingPointParentItem:
                this->createFreqSamplPointParentMenu();
                break;
            case FrequencySamplingPointChildItem:
                this->createFreqSamplPointChildMenu();
                break;
            case SglLayerPlateParentItem:
                this->createSglLayerPlateParentMenu();
                break;
            case LamPlateParentItem:
                this->createLamPlateParentMenu();
                break;
            case SglLayerPlateChildItem:
            case LamPlateChildItem:
                this->createLaminateChildMenu();
                break;
            case GlobalParamParentItem:
                this->createGlobalParameterMenu();
                break;
            case GenericParamParentItem:
                this->createGenericParamParentMenu();
                break;
            case GenericParamChildItem:
                this->createGenericParamChildMenu();
                break;
            case GenericAssignParentItem:
                this->createGenericAssignMenu();
                break;
            case ASCouplingSearchParentItem:
                this->createASCouplingSearchMenu();
                break;
            case GenericMatrixParentItem:
                this->createGenericMatrixMenu();
                break;
            case SolveControlParentItem:
                this->createSolveControlMenu();
                break;
            case GlobalOutputControlParentItem:
                this->createGlobalOutputControlMenu();
                break;
            case CaseDiagOptParentItem:
                this->createDiagOptMenu();
                break;
            case ContriParentItem:
                this->createContriParentMenu();
                break;
            case ERPParentItem:
                this->createERPParentMenu();
                break;
            case PeakFilterParentItem:
                this->createPeakFilterParentMenu();
                break;
            case PeakFilterChildItem:
                this->createPeakFilterChildMenu();
                break;
            case ModalContriChildItem:
            case NodeContriChildItem:
            case PanelContriChildItem:
            case TPContriChildItem:
            case CaseModalContriChildItem:
            case CaseNodeContriChildItem:
            case CasePanelContriChildItem:
            case CaseTPContriChildItem:
                this->createContriChildMenu();
                break;
                break;
            case GenericPartParentItem:
                this->createGenericPartMenu();
                break;
            case CasePeakFilterChildItem:
                this->createCasePeakFilterMenu();
                break;
            case Motion_RootItem:
                this->createMotionRootMenu();
                break;
            case Motion_MovingBodyParentItem:
                this->createMotionBodyParentMenu();
                break;
            case Motion_MovingBodyChildItem:
                this->createMotionBodyChildMenu();
                break;
            case Motion_ConnectorParentItem:
                this->createMotionJointParentMenu();
                break;
            case Motion_CouplingParentItem:
                this->createMotionCouplingParentMenu();
                break;
            case Motion_CouplingChildItem:
                this->createMotionCouplingChildMenu();
                break;
            case Motion_ConnectorChildItem:
                this->createMotionJointChildMenu();
                break;
            case Motion_DriveParentItem:
                this->createMotionDriveParentMenu();
                break;
            case Motion_DriveChildItem:
                this->createMotionDriveChildMenu();
                break;
            case Motion_MarkerParentItem:
                this->createMotionMarkerParentMenu();
                break;
            case Motion_MarkerChildItem:
                this->createMotionMarkerChildMenu();
                break;
            case Motion_LiYuanParentItem:
                this->createMotionVectorForceParentMenu();
                break;
            case Motion_LiYuanChildItem:
                this->createMotionVectorForceChildMenu();
                break;
            case Motion_ContactParentItem:
                this->createMotionContactParentMenu();
                break;
            case Motion_ContactChildItem:
                this->createMotionContactChildMenu();
                break;
            case Motion_ScenarioParentItem:
                this->createMotionScenarioParentMenu();
                break;
            case Motion_SpringParentItem:
                this->createMotionSpringParentMenu();
                break;
            case Motion_SpringChildItem:
                this->createMotionSpringChildMenu();
                break;
            case Motion_ControlInputParentItem:
                this->createMotionControlInputParentMenu();
                break;
            case Motion_ControlInputChildItem:
                this->createMotionControlInputChildMenu();
                break;
            case Motion_ElectroImportParentItem:
                this->createMotionElectroImportParentMenu();
                break;
            case Motion_ElectroImportChildItem:
                this->createMotionElectroImportChildMenu();
                break;
            case Motion_ElectroExportParentItem:
                this->createMotionElectroExportParentMenu();
                break;
            case Motion_ElectroExportChildItem:
                this->createMotionElectroExportChildMenu();
                break;
            case Motion_HydraulicParentItem:
                this->createMotionHydraulicParentMenu();
                break;
            case Motion_HydraulicChildItem:
                this->createMotionHydraulicChildMenu();
                break;
            case Motion_RLParentItem:
                this->createMotionRLParentMenu();
                break;
            case Motion_RLChildItem:
                this->createMotionRLChildMenu();
                break;
            case Motion_SensorParentItem:
                this->createMotionSensorParentMenu();
                break;
            case Motion_SensorChildItem:
                this->createMotionSensorChildMenu();
                break;
            case Motion_ControllerParentItem:
                this->createMotionControllerParentMenu();
                break;
            case Motion_ControllerChildItem:
                this->createMotionControllerChildMenu();
                break;
            case Motion_AutoFlexParentItem:
                this->createMotionAutoFlexParentMenu();
                break;
            case Motion_AutoFlexChildItem:
                this->createMotionAutoFlexChildMenu();
                break;
                break;
            default:
                break;
            }

        }
        else
        {
            //空白处右键（未命中任何节点）：多体形态弹多体根菜单（与右键多体动力学节点一致，
            //隐式入口不可达）；隐式形态保持原"模型根菜单"（新建模型/导入等全局入口）
            if (m_Menu != nullptr)
            {
                delete m_Menu;
                m_Menu = nullptr;
            }

            m_Menu = new mUiMenu(this);
            ThemeManager::applyMenuStyle(m_Menu);
            if (MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion)
            {
                this->createMotionRootMenu();
            }
            else
            {
                this->createModelRootItem();
            }
        }

        MechCommon::mMechGlobalSignals::getInstance()->_isTreeRight = false;
    }

    void mTreeView::slot_menuChildRename()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        if (idxs.size() > 1)
            _treeModel->setSelectRenames(true);
        else
            _treeModel->setSelectRenames(false);

        edit(idxs[0]);
    }







    void mTreeView::slot_menuBatchRename()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1) return;

        QModelIndex proxyParent = idxs[0];
        QModelIndex sourceParent = toSourceIndex(proxyParent);
        if (!sourceParent.isValid()) return;

        int childCount = _treeModel->rowCount(sourceParent);
        if (childCount == 0) return;

        QModelIndexList childProxyIdxs;
        for (int i = 0; i < childCount; ++i)
        {
            QModelIndex sourceChild = _treeModel->index(i, 0, sourceParent);
            QModelIndex proxyChild = _treeProxyModel->mapFromSource(sourceChild);
            if (proxyChild.isValid())
                childProxyIdxs.append(proxyChild);
        }

        if (childProxyIdxs.isEmpty()) return;

        _treeModel->setSelectRenames(true);
        selectedItem(childProxyIdxs);
        edit(childProxyIdxs[0]);
    }


    void mTreeView::slot_menuBoxDelete()
    {
        mInfoWindow::getInstance()->boxDelete();
    }

    void mTreeView::slot_updateModelTree()
    {
        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
        mModelData* modelData = getCVM->getModelData();
        int modelID = modelData->getModelDataID();
        _treeModel->AddModelTree(modelID);

        auto rootItem = _treeModel->getParentItem(ModelRootItem);
        const QList<QWidget*> subWindow = getCVM->getSubWindows();
        for (auto& it : subWindow)
        {
            mViewportPrivate* preWindow = getCVM->getViewportPrivate(it);
            preWindow->setTreeSelectItems(QList<mItemKey>());
        }

        QList<outputInfoData>& dynamicInfoData = getCVM->getDataStatusManager()->updataDynamicInfoData();
        BrowserTreeStatus status = Tree_Yes;
        QVector<QPair<QString, QString>> info;

        mTreeItem* partItem, *childitem;
        int phyID; vector<int> phyIDs; QString phyName;
        //部件
        vector<int> partIDs = modelData->getAllPhyID(Physical_Part);
        if (partIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(partIDs.size());
            //多体形态下 partGroupItem 取多体"部件"固定组（ModelChildItem 单击联动
            //本就切"几何"页并开几何信息面板，两形态共用）
            partItem = partGroupItem(_treeModel);
            mMeshData* meshData = modelData->getMeshData();
            for (int i = 0; i < partIDs.size(); i++)
            {
                phyName = meshData->getPartNameByPartId(partIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Part, partIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "部件", phyName, partIDs[i]);
                childitem = new mTreeItem(modelID, ModelChildItem, phyName, partIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //装配：顶层装配递归构建层级，自由部件（不属于任何装配）按名称排序附底
        fillAssemblyTree(modelData);

        //材料
        vector<int> matIDs = modelData->getAllPhyID(Physical_Material);
        if (matIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(matIDs.size());
            partItem = _treeModel->getParentItem(MaterialParentItem);
            for (int i = 0; i < matIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Material, matIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Material, matIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "材料", phyName, matIDs[i]);
                childitem = new mTreeItem(modelID, MaterialChildItem, phyName, matIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //截面属性
        vector<int> proIDs = modelData->getAllPhyID(Physical_Property);
        if (proIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(proIDs.size());
            partItem = _treeModel->getParentItem(PropertyParentItem);
            for (int i = 0; i < proIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Property, proIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Property, proIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData,
                    info, "截面属性", phyName, proIDs[i]);
                childitem = new mTreeItem(modelID, PropertyChildItem, phyName, proIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //非结构质量
        vector<int> nonIds = modelData->getAllPhyID(Physical_NonStructuralQuality);
        if (nonIds.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(nonIds.size());
            partItem = _treeModel->getParentItem(NonStructuralQualityParentItem);
            if (!partItem) //根节点不存在先创建
            {
                int modelDataID = _treeModel->getCurrentModelID();
                mTreeItem* children = new mTreeItem(modelDataID,
                    NonStructuralQualityParentItem, "非结构质量");
                _treeModel->insertChildren(rootItem, children, getInsertLocation(NonStructuralQualityParentItem));
                partItem = _treeModel->getParentItem(NonStructuralQualityParentItem);
            }
            for (int i = 0; i < nonIds.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_NonStructuralQuality, nonIds[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info,
                    Physical_NonStructuralQuality, nonIds[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData,
                    info, "非结构质量", phyName, nonIds[i]);
                childitem = new mTreeItem(modelID, NonStructuralQualityChildItem, phyName,
                    nonIds[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //场（离散场/分析场统一挂载在"场"父节点下）
        vector<int> fieldIds = modelData->getAllPhyID(Physical_Field);
        if (fieldIds.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(fieldIds.size());
            partItem = _treeModel->getParentItem(FieldParentItem);
            if (!partItem) //根节点不存在先创建
            {
                int modelDataID = _treeModel->getCurrentModelID();
                mTreeItem* children = new mTreeItem(modelDataID,
                    FieldParentItem, "场");
                _treeModel->insertChildren(rootItem, children, getInsertLocation(FieldParentItem));
                partItem = _treeModel->getParentItem(FieldParentItem);
            }
            auto& fieldMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
            for (int i = 0; i < fieldIds.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Field, fieldIds[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info,
                    Physical_Field, fieldIds[i]);
                auto fieldData = fieldMgr.getFieldData(fieldIds[i]);
                QString moduleName = (fieldData != nullptr && fieldData->getFieldType() == MxFunctions::FieldType::Analysis)
                    ? "分析场" : "离散场";
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData,
                    info, moduleName, phyName, fieldIds[i]);
                childitem = new mTreeItem(modelID, FieldChildItem, phyName,
                    fieldIds[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //网格
        auto parentStatus = Tree_Update;
        vector<int> meshIDs = modelData->getAllPhyID(Physical_MeshPanel);
        if (meshIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(meshIDs.size());
            partItem = _treeModel->getParentItem(MeshParentItem);
            for (int i = 0; i < meshIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MeshPanel, meshIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MeshPanel, meshIDs[i]);
                status = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshIDs[i])->getStatusPic();
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "网格", phyName, meshIDs[i]);
                childitem = new mTreeItem(modelID, MeshChildItem, phyName, meshIDs[i], status);
                childItems.push_back(childitem);
                if (status == Tree_Failure)
                    parentStatus = Tree_Failure;
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
            setTreeItemStatus(partItem, parentStatus);
        }

        //网格操作
        vector<int> meshOptIDs = modelData->getAllPhyID(Physical_MeshOptPanel);
        if (meshOptIDs.size() > 0)
        {
            partItem = new mTreeItem(modelID, MeshOptParentItem, "网格操作");
           _treeModel->insertChildren(rootItem, partItem, getInsertLocation(MeshOptParentItem));
           QVector<mTreeItem*> childItems;
           childItems.reserve(meshOptIDs.size());
            for (int i = 0; i < meshOptIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MeshOptPanel, meshOptIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MeshOptPanel, meshOptIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "网格操作", phyName, meshOptIDs[i]);
                childitem = new mTreeItem(modelID, MeshOptChildItem, phyName, meshOptIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //截面形状
        vector<int> secIDs = modelData->getAllPhyID(Physical_PropertyShape);
        if (secIDs.size() > 0)
        {
            mTreeItem* _secParentItem = _treeModel->getParentItem(SectionShapeParentItem);
            if (_secParentItem == nullptr)
            {
                partItem = new mTreeItem(modelID, SectionShapeParentItem, "梁截面");
                _treeModel->insertChildren(_treeModel->getParentItem(PropertyParentItem), partItem, getInsertLocation(SectionShapeParentItem));
                _secParentItem = partItem;
            }
            QVector<mTreeItem*> childItems;
            childItems.reserve(secIDs.size());
            for (int i = 0; i < secIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_PropertyShape, secIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_PropertyShape, secIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "梁截面", phyName, secIDs[i]);
                childitem = new mTreeItem(modelID, SectionShapeChildItem, phyName, secIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(_secParentItem, std::move(childItems));
        }

        //坐标系
       vector<int> coorIDs = modelData->getAllPhyID(Physical_Coordinate);
        if (coorIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(coorIDs.size());
            partItem = _treeModel->getParentItem(CoordinateParentItem);
            for (int i = 0; i < coorIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Coordinate, coorIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Coordinate, coorIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "坐标系", phyName, coorIDs[i]);
                childitem = new mTreeItem(modelID, CoordinateChildItem, phyName, coorIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //连接
        vector<int> connectorIDs = modelData->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getAllConnectorID();
        if (connectorIDs.size() > 0)
        {
            auto interactionItem = _treeModel->getParentItem(InteractionParentItem);
            partItem = new mTreeItem(modelID, ConnectorParentItem, "连接");
            _treeModel->appendChildren(interactionItem, partItem);

            QVector<mTreeItem*> childItems;
            childItems.reserve(connectorIDs.size());
            for (int i = 0; i < connectorIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Connector, connectorIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Connector, connectorIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "连接", phyName, connectorIDs[i]);
                childitem = new mTreeItem(modelID, ConnectorChildItem, phyName, connectorIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //连接副
        vector<int> deputyIDs = modelData->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getAllDeputyID();
        if (deputyIDs.size() > 0)
        {
            auto interactionItem = _treeModel->getParentItem(InteractionParentItem);
            partItem = new mTreeItem(modelID, DeputyParentItem, "连接副");
            _treeModel->appendChildren(interactionItem, partItem);

            QVector<mTreeItem*> childItems;
            childItems.reserve(deputyIDs.size());
            for (int i = 0; i < deputyIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Deputy, deputyIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Deputy, deputyIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "连接副", phyName, deputyIDs[i]);
                childitem = new mTreeItem(modelID, DeputyChildItem, phyName, deputyIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //接触
        vector<int> contactIDs = modelData->getAllPhyID(Physical_Contact);
        if (contactIDs.size() > 0)
        {
            auto interactionItem = _treeModel->getParentItem(InteractionParentItem);
            partItem = new mTreeItem(modelID, ContactParentItem, "接触");
            _treeModel->appendChildren(interactionItem, partItem);

            QVector<mTreeItem*> childItems;
            childItems.reserve(contactIDs.size());
            for (int i = 0; i < contactIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Contact, contactIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Contact, contactIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "接触", phyName, contactIDs[i]);
                childitem = new mTreeItem(modelID, ContactChildItem, phyName, contactIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //周期性边界
        vector<int> pbIDs = modelData->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getAllPeriodicBoundaryID();
        if (pbIDs.size() > 0)
        {
            auto interactionItem = _treeModel->getParentItem(InteractionParentItem);
            mTreeItem* pbParentItem = new mTreeItem(modelID, PeriodicBoundaryParentItem, "周期性边界");
            _treeModel->appendChildren(interactionItem, pbParentItem);

            QVector<mTreeItem*> childItems;
            childItems.reserve(pbIDs.size());
            for (int i = 0; i < pbIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_PeriodicBoundary, pbIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_PeriodicBoundary, pbIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "周期性边界", phyName, pbIDs[i]);
                childitem = new mTreeItem(modelID, PeriodicBoundaryChildItem, phyName, pbIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(pbParentItem, std::move(childItems));
        }

        //模型修改
        vector<int> modelFixIDs = modelData->getAllPhyID(Physical_ModelFix);
        if (modelFixIDs.size() > 0)
        {
            auto interactionItem = _treeModel->getParentItem(InteractionParentItem);
            partItem = new mTreeItem(modelID, ModelFixParentItem, "模型修改");
            _treeModel->appendChildren(interactionItem, partItem);

            QVector<mTreeItem*> childItems;
            childItems.reserve(modelFixIDs.size());
            for (int i = 0; i < modelFixIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_ModelFix, modelFixIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_ModelFix, modelFixIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "模型修改", phyName, modelFixIDs[i]);
                childitem = new mTreeItem(modelID, ModelFixChildItem, phyName, modelFixIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //集合
        vector<int> setIDs = modelData->getAllPhyID(Physical_Set);
        if (setIDs.size() > 0)
        {
            partItem = new mTreeItem(modelID, SetParentItem, "集合");
            _treeModel->insertChildren(rootItem, partItem, getInsertLocation(SetParentItem));

            QMap<int, QVector<mTreeItem*>> typeChildItems;
            for (int i = 0; i < setIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Set, setIDs[i]);
                mSetBase* setBase = modelData->getManager<mSetManager>(Physical_Set).getSetData(setIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Set, setIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "集合", phyName, setIDs[i]);
                childitem = new mTreeItem(modelID, SetChildItem, phyName, setIDs[i], status, slot_setTypePic(setBase->getSetType()));
                typeChildItems[static_cast<int>(setBase->getSetType())].push_back(childitem);
            }

            //按二级菜单类型顺序创建分组节点并挂载
            QVector<MxFunctions::SetType> orderedTypes = getSetTypeMenuOrder();
            for (auto it = typeChildItems.keyBegin(); it != typeChildItems.keyEnd(); ++it)
            {
                if (!orderedTypes.contains(static_cast<MxFunctions::SetType>(*it)))
                    orderedTypes.push_back(static_cast<MxFunctions::SetType>(*it));
            }
            for (MxFunctions::SetType setType : orderedTypes)
            {
                auto found = typeChildItems.find(static_cast<int>(setType));
                if (found == typeChildItems.end() || found.value().isEmpty())
                    continue;
                mTreeItem* groupItem = ensureSetTypeGroupItem(partItem, setType);
                if (groupItem != nullptr)
                    _treeModel->appendChildren(groupItem, std::move(found.value()));
            }
        }

        //函数
        vector<int> funcIDs = modelData->getAllPhyID(Physical_Function);
        if (funcIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(funcIDs.size());
            partItem = new mTreeItem(modelID, FunctionParentItem, "函数");
            _treeModel->insertChildren(rootItem, partItem, getInsertLocation(FunctionParentItem));
            for (int i = 0; i < funcIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_Function, funcIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_Function, funcIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "函数", phyName, funcIDs[i]);
                childitem = new mTreeItem(modelID, FunctionChildItem, phyName, funcIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }

        //结构优化
        if (getCVM->getModelData()->getStructOptionBase())
        {
            partItem = new mTreeItem(modelID, OptimizationParentItem, "优化");
            _treeModel->insertChildren(rootItem, partItem, getInsertLocation(OptimizationParentItem));

            mTreeItem* designItem = new mTreeItem(modelID, OptimizationDesignParentItem, "设计域");
            _treeModel->appendChildren(partItem, designItem);

            vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_OptimationDesign);
            QVector<mTreeItem*>  designChildItems;
            designChildItems.reserve(phyIDs.size());
            for (int i = 0; i < phyIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_OptimationDesign, phyIDs[i]);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_OptimationDesign, phyIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "结构设计域", phyName, phyIDs[i]);
                childitem = new mTreeItem(modelID, OptimizationDesignChildItem, phyName, phyIDs[i], status);
                designChildItems.push_back(childitem);
            }
            _treeModel->appendChildren(designItem, std::move(designChildItems));

            {
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_OptimationGoal, 0);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "结构优化目标", "优化目标", modelID);
                mTreeItem* goalItem = new mTreeItem(modelID, OptimizationGoalParentItem, "优化目标", 0, status);
                _treeModel->appendChildren(partItem, goalItem);
            }

            mTreeItem* constraintItem = new mTreeItem(modelID, OptimizationConstraintsParentItem, "优化约束");
            _treeModel->appendChildren(partItem, constraintItem);

            phyIDs = getCVM->getModelData()->getAllPhyID(Physical_OptimationConstraint);
            QVector<mTreeItem*> constraintChildItems;
            constraintChildItems.reserve(phyIDs.size());
            for (int i = 0; i < phyIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_OptimationConstraint, phyIDs[i]);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_OptimationConstraint, phyIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "结构优化约束", phyName, phyIDs[i]);
                childitem = new mTreeItem(modelID, OptimizationConstraintsChildItem, phyName, phyIDs[i], status);
                constraintChildItems.push_back(childitem);
            }
            _treeModel->appendChildren(constraintItem, std::move(constraintChildItems));
        }

        //流道优化
        if (getCVM->getModelData()->getRunnerStructOption())
        {
            partItem = new mTreeItem(modelID, RunnerOptimizationParentItem, "流道优化");
            _treeModel->insertChildren(rootItem, partItem, BoundaryManagerParentItem);

            mTreeItem* designItem = new mTreeItem(modelID, RunnerOptimizationDesignParentItem, "设计域");
            _treeModel->appendChildren(partItem, designItem);

            vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_RunnerOptimationDesign);
            QVector<mTreeItem*>  designChildItems;
            designChildItems.reserve(phyIDs.size());
            for (int i = 0; i < phyIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_RunnerOptimationDesign, phyIDs[i]);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_RunnerOptimationDesign, phyIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "设计域", phyName, phyIDs[i]);
                childitem = new mTreeItem(modelID, RunnerOptimizationDesignChildItem, phyName, phyIDs[i], status);
                designChildItems.push_back(childitem);
            }
            _treeModel->appendChildren(designItem, std::move(designChildItems));

            mTreeItem* goalItem = new mTreeItem(modelID, RunnerOptimizationGoalParentItem, "优化目标");
            _treeModel->appendChildren(partItem, goalItem);

            mTreeItem* constraintItem = new mTreeItem(modelID, RunnerOptimizationConstraintsParentItem, "优化约束");
            _treeModel->appendChildren(partItem, constraintItem);

            phyIDs = getCVM->getModelData()->getAllPhyID(Physical_RunnerOptimationConstraint);
            QVector<mTreeItem*> runnerConstraintChildItems;
            runnerConstraintChildItems.reserve(phyIDs.size());
            for (int i = 0; i < phyIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_RunnerOptimationConstraint, phyIDs[i]);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_RunnerOptimationConstraint, phyIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "优化约束", phyName, phyIDs[i]);
                childitem = new mTreeItem(modelID, RunnerOptimizationConstraintsChildItem, phyName, phyIDs[i], status);
                runnerConstraintChildItems.push_back(childitem);
            }
            _treeModel->appendChildren(constraintItem, std::move(runnerConstraintChildItems));
        }

        //分析工况
        vector<int> caseIDs = modelData->getAllPhyID(Physical_Case);
        mMapRelationManager& mapRelationManager = modelData->getMapRelationManager();

        mTreeItem* lastItem = nullptr;
        if (_treeModel->getParentItem(FrequencySamplingPointParentItem) != nullptr)
            lastItem = _treeModel->getParentItem(FrequencySamplingPointParentItem);
        else if (_treeModel->getParentItem(PredefinedFieldManagerParentItem) != nullptr)
            lastItem = _treeModel->getParentItem(PredefinedFieldManagerParentItem);
        else if (_treeModel->getParentItem(NonStructuralQualityParentItem) != nullptr)
            lastItem = _treeModel->getParentItem(NonStructuralQualityParentItem);
        else
            lastItem = _treeModel->getParentItem(BoundaryManagerParentItem);

        for (int i = 0; i < caseIDs.size(); i++)
        {
            mCaseBase* caseBase = modelData->getManager<mCaseManager>(Physical_Case).getCaseData(caseIDs[i]);
            if (caseBase == nullptr)
                return;
            auto caseType = caseBase->getCaseType();
            BrowserTreeType bt = CaseImplicitLinearStaticParentItem;
            bt = BrowserTreeType(int(bt) + caseType - 1);
            QString name = caseBase->getCaseName();
            QString iconPath = getCaseIconPath(caseType);

            auto parentItem = new mTreeItem(modelID, bt, name, caseIDs[i], Tree_Yes, iconPath);
            _treeModel->insertChildren(rootItem, parentItem, lastItem);
            lastItem = parentItem;

            // 结果节点状态
            MxFunctions::BrowserTreeStatus resultStatus = Tree_Failure;
            if (!caseBase->getSolveDir().isEmpty())
            {
                resultStatus = Tree_Yes;
            }

            // 统一创建工况下一级菜单节点（已内部处理各工况的显示规则，并包含结果节点）
            createCaseFirstLevelItems(modelID, caseType, caseIDs[i], parentItem, resultStatus);

            // 载荷子节点
            set<int> loadBcIDs = mapRelationManager.getMapRelation<set<int>>({ Physical_Case, caseIDs[i] }, Physical_LoadBoundary);
            vector<int> loadIDs, bcIDs;
            getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).differentiateLoadBoundary(loadBcIDs, loadIDs, bcIDs);
            auto caseLoadItem = _treeModel->getParentItem(CaseLoadParentItem, caseIDs[i]);
            QVector<mTreeItem*> LoadChildItems;
            LoadChildItems.reserve(loadIDs.size());
            for (auto loadID : loadIDs)
            {
                phyName = modelData->getNameByPhyID(Physical_LoadBoundary, loadID);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_LoadBoundary, loadID);
                childitem = new mTreeItem(modelID, CaseLoadChildItem, phyName, loadID, status);
                childitem->setCaseID(caseIDs[i]);
                LoadChildItems.push_back(childitem);
            }
            _treeModel->appendChildren(caseLoadItem, std::move(LoadChildItems));

            // 转子分析子节点
            if (caseType == ImplicitModal && _treeModel->getParentItem(CaseRotateParentItem, caseIDs[i]) != nullptr)
            {
                set<int> rotateAnalyseIDs = mapRelationManager.getMapRelation<set<int>>({ Physical_Case, caseIDs[i] }, Physical_RotateAnalyse);
                auto caseRotateItem = _treeModel->getParentItem(CaseRotateParentItem, caseIDs[i]);
                QVector<mTreeItem*> rotateChildItems;
                rotateChildItems.reserve(rotateAnalyseIDs.size());
                for (auto rotateID : rotateAnalyseIDs)
                {
                    phyName = modelData->getNameByPhyID(Physical_RotateAnalyse, rotateID);
                    if (phyName.isEmpty())
                        continue;
                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_RotateAnalyse, rotateID);
                    childitem = new mTreeItem(modelID, CaseRotateChildItem, phyName, rotateID, status);
                    childitem->setCaseID(caseIDs[i]);
                    rotateChildItems.push_back(childitem);
                }
                _treeModel->appendChildren(caseRotateItem, std::move(rotateChildItems));
            }

            // 边界子节点
            auto BCItem = _treeModel->getParentItem(CaseBCParentItem, caseIDs[i]);
            QVector<mTreeItem*> bcChildItems;
            bcChildItems.reserve(bcIDs.size());
            for (auto bcID : bcIDs)
            {
                phyName = modelData->getNameByPhyID(Physical_LoadBoundary, bcID);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_LoadBoundary, bcID);
                childitem = new mTreeItem(modelID, CaseBCChildItem, phyName, bcID, status);
                childitem->setCaseID(caseIDs[i]);
                bcChildItems.push_back(childitem);
            }
            _treeModel->appendChildren(BCItem, std::move(bcChildItems));

            // 过盈接触子节点
            if (caseType != ImplicitLinearStatic)
            {
                auto caseInterItem = _treeModel->getParentItem(CaseInteractionParentItem, caseIDs[i]);
                set<int> contactIDs = mapRelationManager.getMapRelation<set<int>>({ Physical_Case, caseIDs[i] }, Physical_Contact);
                QVector<mTreeItem*> interChildItems;
                interChildItems.reserve(contactIDs.size());
                for (auto contactID : contactIDs)
                {
                    auto contactData = modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(contactID);
                    if (!contactData) continue;
                    if (contactData->getContactType() != InterferenceContact) continue;
                    phyName = contactData->getContactName();
                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_Contact, contactID);
                    childitem = new mTreeItem(modelID, CaseInteractionChildItem, phyName, contactID, status);
                    childitem->setCaseID(caseIDs[i]);
                    interChildItems.push_back(childitem);
                }
                _treeModel->appendChildren(caseInterItem, std::move(interChildItems));
            }

            // 预定义场子节点
            auto casePredefinedFieldItem = _treeModel->getParentItem(CasePredefinedFieldParentItem, caseIDs[i]);
            set<int> fieldIDs = mapRelationManager.getMapRelation<set<int>>({ Physical_Case, caseIDs[i] }, Physical_PredefinedField);
            QVector<mTreeItem*> fieldChildItems;
            fieldChildItems.reserve(fieldIDs.size());
            for (auto fieldID : fieldIDs)
            {
                auto fieldData = modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(fieldID);
                if (!fieldData) continue;
                phyName = fieldData->getPredefinedFieldName();
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_PredefinedField, fieldID);
                childitem = new mTreeItem(modelID, CasePredefinedFieldChildItem, phyName, fieldID, status);
                childitem->setCaseID(caseIDs[i]);
                fieldChildItems.push_back(childitem);
            }
            _treeModel->appendChildren(casePredefinedFieldItem, std::move(fieldChildItems));

            // 诊断优化子节点
            if (caseType == ImplicitFrequency || caseType == ASCoupling)
            {
                auto diagOptItem = _treeModel->getParentItem(CaseDiagOptParentItem, caseIDs[i]);

                // 模态贡献量
                if (caseType == ImplicitFrequency)
                {
                    mCaseImplicitFrequency* freqCase = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
                    if (freqCase && freqCase->getModalContriData())
                    {
                        info.clear();
                        status = getCVM->getModelData()->getDataStatus(info, Physical_ModalContri, caseIDs[i]);
                        getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "模态贡献量", "模态贡献量", caseIDs[i]);
                        auto modalItem = new mTreeItem(modelID, CaseModalContriChildItem, "模态贡献量", caseIDs[i], status);
                        modalItem->setCaseID(caseIDs[i]);
                        _treeModel->appendChildren(diagOptItem, modalItem);
                    }
                }
                else if (caseType == ASCoupling)
                {
                    mCaseImplASCoupling* asCase = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                    if (asCase)
                    {
                        if (asCase->getModalContriData())
                        {
                            info.clear();
                            status = getCVM->getModelData()->getDataStatus(info, Physical_ModalContri, caseIDs[i]);
                            getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "模态贡献量", "模态贡献量", caseIDs[i]);
                            auto modalItem = new mTreeItem(modelID, CaseModalContriChildItem, "模态贡献量", caseIDs[i], status);
                            modalItem->setCaseID(caseIDs[i]);
                            _treeModel->appendChildren(diagOptItem, modalItem);
                        }
                        if (asCase->getNodeContriData())
                        {
                            info.clear();
                            status = getCVM->getModelData()->getDataStatus(info, Physical_NodeContri, caseIDs[i]);
                            getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "节点贡献量", "节点贡献量", caseIDs[i]);
                            auto nodeItem = new mTreeItem(modelID, CaseNodeContriChildItem, "节点贡献量", caseIDs[i], status);
                            nodeItem->setCaseID(caseIDs[i]);
                            _treeModel->appendChildren(diagOptItem, nodeItem);
                        }
                        if (asCase->getPanelContriData())
                        {
                            info.clear();
                            status = getCVM->getModelData()->getDataStatus(info, Physical_PanelContri, caseIDs[i]);
                            getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "面板贡献量", "面板贡献量", caseIDs[i]);
                            auto panelItem = new mTreeItem(modelID, CasePanelContriChildItem, "面板贡献量", caseIDs[i], status);
                            panelItem->setCaseID(caseIDs[i]);
                            _treeModel->appendChildren(diagOptItem, panelItem);
                        }
                    }
                }

                // 工况峰值筛选
                QList<int> casePfIDs = mapRelationManager.getMapRelation<QList<int>>({ Physical_Case, caseIDs[i] }, Physical_PeakFilter);
                if (casePfIDs.size() > 0)
                {
                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_PeakFilter, caseIDs[i]);
                    getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "峰值筛选", "峰值筛选", caseIDs[i]);
                    auto casePfItem = new mTreeItem(modelID, CasePeakFilterChildItem, "峰值筛选", caseIDs[i], status);
                    casePfItem->setCaseID(caseIDs[i]);
                    _treeModel->appendChildren(diagOptItem, casePfItem);
                }
            }

            //云图
            set<int> resultIDs = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCloudIdsByCase(caseIDs[i]);
            mTreeItem* resultItem = _treeModel->getParentItem(ResultParentItem, -1);
            for (auto resID : resultIDs)
            {
                phyName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemNameByID(PostData::CloudItem, resID);
                auto cloudItem = new mTreeItem(modelID, threeDResultParentItem, phyName, resID);
                _treeModel->appendChildren(resultItem, cloudItem);

                QVector<mTreeItem*> resultChildItems;
                resultChildItems.reserve(resultIDs.size());
                //动画
                set<int> aniIDs = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getAnimationIdsByCaseCloud(caseIDs[i], resID);
                for (auto aniID : aniIDs)
                {
                    phyName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemNameByID(PostData::AnimationItem, aniID);
                    auto aniItem = new mTreeItem(modelID, animationChildItem, phyName, aniID);
                    resultChildItems.push_back(aniItem);
                }

                //线性叠加
                set<int> linearIDs = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getLinearIdsByCaseCloud(caseIDs[i], resID);
                for (auto linearID : linearIDs)
                {
                    phyName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemNameByID(PostData::LinearItem, linearID);
                    auto linearItem = new mTreeItem(modelID, linearChildItem, phyName, linearID);
                    resultChildItems.push_back(linearItem);
                }

                //等值面图
                set<int> isoIDs = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getIsoIdsByCaseCloud(caseIDs[i], resID);
                for (auto isoID : isoIDs)
                {
                    phyName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemNameByID(PostData::IsosurfaceItem, isoID);
                    auto isoItem = new mTreeItem(modelID, isosurfaceChildItem, phyName, isoID);
                    resultChildItems.push_back(isoItem);
                }

                //矢量图
                set<int> vectorIDs = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorIdsByCaseCloud(caseIDs[i], resID);
                for (auto vectorID : vectorIDs)
                {
                    phyName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemNameByID(PostData::VectorItem, vectorID);
                    auto vecItem = new mTreeItem(modelID, vectorChildItem, phyName, vectorID);
                    resultChildItems.push_back(vecItem);
                }

                _treeModel->appendChildren(cloudItem, std::move(resultChildItems));
            }

            //曲线
            set<int> curveIDs = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getCurveIdsByCase(caseIDs[i]);
            for (auto curveID : curveIDs)
            {
                phyName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemNameByID(PostData::CurveItem, curveID);
                auto curveItem = new mTreeItem(modelID, CurveChildItem, phyName, curveID);
                _treeModel->appendChildren(resultItem, curveItem);
            }

            //路径子节点
            set<int> pathIDs = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPathIdsByCase(caseIDs[i]);
            mTreeItem* pathParentItem;
            if (pathIDs.size() > 0)
            {
                //路径父节点
                pathParentItem = new mTreeItem(modelID, PathParentItem, "路径", caseIDs[i]);
                _treeModel->appendChildren(resultItem, pathParentItem);
            }
            for (auto pathID : pathIDs)
            {
                phyName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemNameByID(PostData::PathItem, pathID);
                auto pathItem = new mTreeItem(modelID, PathChildItem, phyName, pathID);
                _treeModel->appendChildren(pathParentItem, pathItem);
            }
        }

        //载荷边界管理器
        vector<int> loadBounaryIDs = modelData->getAllPhyID(Physical_LoadBoundary);
        for (int i = 0; i < loadBounaryIDs.size(); i++)
        {
            //获取载荷边界数据
            auto loadBoundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadBounaryIDs[i]);
            info.clear();
            status = getCVM->getModelData()->getDataStatus(info, Physical_LoadBoundary, loadBounaryIDs[i]);
            getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "载荷边界", loadBoundaryData->getLoadBoundaryName(), loadBounaryIDs[i]);

            if (loadBoundaryData->isLoadOrBoundary() == 0) //载荷
            {
                mTreeItem* _loadManagerItem = _treeModel->getParentItem(LoadManagerParentItem);
                if (_loadManagerItem != nullptr)
                {
                    auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseLoadChildItem, loadBoundaryData->getLoadBoundaryName(), loadBounaryIDs[i], status);
                    _treeModel->appendChildren(_loadManagerItem, childitem);
                }
            }
            else if (loadBoundaryData->isLoadOrBoundary() == 1) //边界
            {
                mTreeItem* _boundaryManagerItem = _treeModel->getParentItem(BoundaryManagerParentItem);
                if (_boundaryManagerItem != nullptr)
                {
                    auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseBCChildItem, loadBoundaryData->getLoadBoundaryName(), loadBounaryIDs[i], status);
                    _treeModel->appendChildren(_boundaryManagerItem, childitem);
                }
            }
        }

        //采样频率点
        vector<int> FreqSamplPointIDs = modelData->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getAllFreqSamplPointID();
        for (int i = 0; i < FreqSamplPointIDs.size(); i++)
        {
            //获取数据
            MechCaseData::mFreqSamplPointBase* newData =
                modelData->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getFreqSamplPointData(FreqSamplPointIDs[i]);
            mTreeItem* _parentItem = _treeModel->getParentItem(FrequencySamplingPointParentItem);

            if (_parentItem == nullptr) //无父节点，则创建父节点
            {
                _parentItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), FrequencySamplingPointParentItem, "采样频率点");
                _treeModel->insertChildren(rootItem, _parentItem, getInsertLocation(FrequencySamplingPointParentItem));
            }
            info.clear();
            status = getCVM->getModelData()->getDataStatus(info, Physical_FreqSamplPoint, FreqSamplPointIDs[i]);
            getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "采样频率点", newData->getFreqSamplPointName(), FreqSamplPointIDs[i]);

            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), FrequencySamplingPointChildItem, newData->getFreqSamplPointName(), FreqSamplPointIDs[i], status);
            _treeModel->appendChildren(_parentItem, childitem);
        }

        //层合结构
        vector<int> laminateIDs = modelData->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getAllLaminateID();
        for (int i = 0; i < laminateIDs.size(); ++i)
        {
            int modelID = getCVM->getModelData()->getModelDataID();

            mLaminateBase* laminate = getCVM->getModelData()->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getLaminateData(laminateIDs[i]);
            if (laminate == nullptr)
                continue;
            //判断类型，单层板，全局单层板，层合板
            MxFunctions::LaminateType type = laminate->getLaminateType();

            info.clear();
            status = getCVM->getModelData()->getDataStatus(info, Physical_Laminate, laminateIDs[i]);
            getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "层合结构", laminate->getLaminateName(), laminateIDs[i]);

            switch (type)
            {
            case MxFunctions::GlobalSglLam:
            case MxFunctions::SglLayerPlate:
            {
                auto childitem = new mTreeItem(modelID, SglLayerPlateChildItem, laminate->getLaminateName(), laminate->getLaminateId(), status);
                auto _parentItem = _treeModel->getParentItem(SglLayerPlateParentItem);
                if (!_parentItem)
                {
                    _parentItem = new mTreeItem(modelID, SglLayerPlateParentItem, "单层板");
                    _treeModel->insertChildren(_treeModel->getParentItem(PropertyParentItem), _parentItem, getInsertLocation(SglLayerPlateParentItem)); //插到截面属性父节点下
                }
                if (childitem && _parentItem)
                {
                    _treeModel->appendChildren(_parentItem, childitem);
                }
            }
            break;
            case MxFunctions::LamPlate:
            {
                auto childitem = new mTreeItem(modelID, LamPlateChildItem, laminate->getLaminateName(), laminate->getLaminateId(), status);
                auto _parentItem = _treeModel->getParentItem(LamPlateParentItem);
                if (!_parentItem)
                {
                    _parentItem = new mTreeItem(modelID, LamPlateParentItem, "层合板");
                    _treeModel->insertChildren(_treeModel->getParentItem(PropertyParentItem), _parentItem, getInsertLocation(LamPlateParentItem));
                }
                if (childitem && _parentItem)
                {
                    _treeModel->appendChildren(_parentItem, childitem);
                }
            }
            break;
            default:
                break;
            }
        }

        //全局参数
        {
            auto globalParam = getCVM->getModelData()->getGlobalParam();
            mTreeItem* parentItem = _treeModel->getParentItem(GlobalParamParentItem);
            if (globalParam->getSolveControlData())
            {
                mTreeItem* solveControlItem = new mTreeItem(modelID, SolveControlParentItem, "求解控制");
                _treeModel->appendChildren(parentItem, solveControlItem);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_SolveControl, 0);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "求解控制", "求解控制", 0);
            }

            if (globalParam->getGlobalOutputControlData())
            {
                mTreeItem* globalOutputControlItem = new mTreeItem(modelID, GlobalOutputControlParentItem, "全局输出控制");
                _treeModel->appendChildren(parentItem, globalOutputControlItem);
            
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_GlobalOutputControl, 0);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "全局输出控制", "全局输出控制", 0);
            }

            if (globalParam->getGenericAssignData())
            {
                mTreeItem* assignItem = new mTreeItem(modelID, GenericAssignParentItem, "超单元调用");
                _treeModel->appendChildren(parentItem, assignItem);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_GenericAssign, 0);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "超单元调用", "超单元调用", 0);
            }

            if (globalParam->getASCouplingSearchData())
            {
                mTreeItem* asCouplingItem = new mTreeItem(modelID, ASCouplingSearchParentItem, "声固耦合对搜索");
                _treeModel->appendChildren(parentItem, asCouplingItem);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_ASCouplingSearch, 0);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "声固耦合对搜索", "声固耦合对搜索", 0);
            }

            if (globalParam->getGenericMatrixData())
            {
                mTreeItem* matrixItem = new mTreeItem(modelID, GenericMatrixParentItem, "超单元矩阵");
                _treeModel->appendChildren(parentItem, matrixItem);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_GenericMatrix, 0);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "超单元矩阵", "超单元矩阵", 0);
            }

            if (globalParam->getERPData())
            {
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_ERP, 0);
                mTreeItem* erpItem = new mTreeItem(modelID, ERPParentItem, "等效辐射功率", 0, status);
                _treeModel->appendChildren(parentItem, erpItem);

                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "等效辐射功率", "等效辐射功率", 0);
            }

            vector<int> pfIDs = getCVM->getModelData()->getAllPhyID(Physical_PeakFilter);
            if (pfIDs.size() > 0)
            {
                mTreeItem* pfItem = new mTreeItem(modelID, PeakFilterParentItem, "峰值筛选");
                _treeModel->appendChildren(parentItem, pfItem);

                QVector<mTreeItem*> childItems;
                for (auto phyID : pfIDs)
                {
                    phyName = modelData->getNameByPhyID(Physical_PeakFilter, phyID);

                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_PeakFilter, phyID);
                    getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "峰值筛选", phyName, phyID);
                    childitem = new mTreeItem(modelID, PeakFilterChildItem, phyName, phyID, status);
                    childItems.push_back(childitem);
                }
                _treeModel->appendChildren(pfItem, std::move(childItems));
            }

                        // 贡献量
            {
                QVector<mTreeItem*> contriChildItems;
                mTreeItem* contriItem = nullptr;

                if (globalParam->getModalContriData())
                {
                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_ModalContri, 0);
                    getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "贡献量", "模态贡献量", 0);
                    childitem = new mTreeItem(modelID, ModalContriChildItem, "模态贡献量", 0, status);
                    contriChildItems.push_back(childitem);
                }
                if (globalParam->getNodeContriData())
                {
                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_NodeContri, 0);
                    getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "贡献量", "节点贡献量", 0);
                    childitem = new mTreeItem(modelID, NodeContriChildItem, "节点贡献量", 0, status);
                    contriChildItems.push_back(childitem);
                }
                if (globalParam->getPanelContriData())
                {
                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_PanelContri, 0);
                    getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "贡献量", "面板贡献量", 0);
                    childitem = new mTreeItem(modelID, PanelContriChildItem, "面板贡献量", 0, status);
                    contriChildItems.push_back(childitem);
                }
                if (globalParam->getTPContriData())
                {
                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_TPContri, 0);
                    getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "贡献量", "传递路径贡献量", 0);
                    childitem = new mTreeItem(modelID, TPContriChildItem, "传递路径贡献量", 0, status);
                    contriChildItems.push_back(childitem);
                }

                if (!contriChildItems.isEmpty())
                {
                    partItem = _treeModel->getParentItem(GlobalParamParentItem);
                    contriItem = _treeModel->getParentItem(ContriParentItem);
                    if (!contriItem)
                    {
                        contriItem = new mTreeItem(modelID, ContriParentItem, "贡献量");
                        _treeModel->appendChildren(partItem, contriItem);
                    }
                    _treeModel->appendChildren(contriItem, std::move(contriChildItems));
                }
            }

            if (globalParam->getGenericPartData())
            {
                mTreeItem* partItem = new mTreeItem(modelID, GenericPartParentItem, "超单元部件");
                _treeModel->appendChildren(parentItem, partItem);

                //info.clear();
                //status = getCVM->getModelData()->getDataStatus(info, Physical_GenericMatrix, 0);
                //getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "超单元部件", "超单元部件", 0);
            }

            vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_GenericParam);
            if (phyIDs.size() > 0)
            {
                mTreeItem* paramItem = new mTreeItem(modelID, GenericParamParentItem, "超单元缩聚方法");
                _treeModel->appendChildren(parentItem, paramItem);

                QVector<mTreeItem*> childItems;
                for (auto phyID : phyIDs)
                {
                    phyName = modelData->getNameByPhyID(Physical_GenericParam, phyID);

                    info.clear();
                    status = getCVM->getModelData()->getDataStatus(info, Physical_GenericParam, phyID);
                    getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "超单元缩聚方法", phyName, phyID);
                    childitem = new mTreeItem(modelID, GenericParamChildItem, phyName, phyID, status);
                    childItems.push_back(childitem);
                }
                _treeModel->appendChildren(paramItem, std::move(childItems));
            }
        }

        //预定义场管理器
        vector<int> fieldIDs = modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getAllPredefinedFieldID();
        if (fieldIDs.size() > 0)
        {
            partItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), PredefinedFieldManagerParentItem, "预定义场");
            _treeModel->insertChildren(rootItem, partItem, getInsertLocation(PredefinedFieldManagerParentItem));

            QVector<mTreeItem*> childItems;
            for (int i = 0; i < fieldIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_PredefinedField, fieldIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_PredefinedField, fieldIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "预定义场", phyName, fieldIDs[i]);
                childitem = new mTreeItem(modelID, CasePredefinedFieldChildItem, phyName, fieldIDs[i], status);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //多体动力学：固定分组由 addMotionRootTree 常驻创建，此处仅按分组填充对象节点
        //自动柔性体（刚柔耦合；此前整树刷新缺此段，导入后被抹掉）
        {
            vector<int> autoFlexIDs = modelData->getAllPhyID(Physical_MotionAutoFlex);
            if (autoFlexIDs.size() > 0)
            {
                mTreeItem* flexParent = _treeModel->getParentItem(Motion_AutoFlexParentItem);
                QVector<mTreeItem*> flexItems;
                flexItems.reserve(autoFlexIDs.size());
                for (size_t i = 0; i < autoFlexIDs.size(); ++i)
                {
                    auto* flexData = modelData->getManager<MechMotionData::mMotionAutoFlexManager>(Physical_MotionAutoFlex).getData(autoFlexIDs[i]);
                    if (!flexData) continue;
                    flexItems.push_back(new mTreeItem(modelData->getModelDataID(), Motion_AutoFlexChildItem,
                        flexData->getName(), autoFlexIDs[i], MxFunctions::Tree_Yes,
                        ":/mMainWidgets/Icon/Motion/mnf.png"));
                }
                if (flexParent && !flexItems.isEmpty())
                    _treeModel->appendChildren(flexParent, std::move(flexItems));
            }
        }
        //运动体
        vector<int> motionBodyIDs = modelData->getAllPhyID(Physical_MotionBody);
        if (motionBodyIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionBodyIDs.size());
            partItem = _treeModel->getParentItem(Motion_MovingBodyParentItem);
            for (int i = 0; i < motionBodyIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionBody, motionBodyIDs[i]);
                mMotionBody* motionBody = modelData->getManager<mMotionBodyManager>(Physical_MotionBody).getData(motionBodyIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionBody, motionBodyIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "运动体", phyName, motionBodyIDs[i]);
                childitem = new mTreeItem(modelID, Motion_MovingBodyChildItem, phyName, motionBodyIDs[i], status, getMotionBodyPic(motionBody->getIsFixed()));
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //运动副
        vector<int> motionJointIDs = modelData->getAllPhyID(Physical_MotionJoint);
        if (motionJointIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionJointIDs.size());
            partItem = _treeModel->getParentItem(Motion_ConnectorParentItem);

            for (int i = 0; i < motionJointIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionJoint, motionJointIDs[i]);
                mMotionJointBase* motionJoint = modelData->getManager<mMotionJointManager>(Physical_MotionJoint).getData(motionJointIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionJoint, motionJointIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "运动副", phyName, motionJointIDs[i]);
                childitem = new mTreeItem(modelID, Motion_ConnectorChildItem, phyName, motionJointIDs[i], status, getMotionJointPic(motionJoint->getType()));
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //耦合副
        vector<int> motionCouplingIDs = modelData->getAllPhyID(Physical_MotionCoupling);
        if (motionCouplingIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionCouplingIDs.size());
            partItem = _treeModel->getParentItem(Motion_CouplingParentItem);

            for (int i = 0; i < motionCouplingIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionCoupling, motionCouplingIDs[i]);
                mMotionCouplingBase* motionCoupling = modelData->getManager<mMotionCouplingManager>(Physical_MotionCoupling).getData(motionCouplingIDs[i]);
                if (motionCoupling == nullptr)
                    continue;

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionCoupling, motionCouplingIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "耦合副", phyName, motionCouplingIDs[i]);
                childitem = new mTreeItem(modelID, Motion_CouplingChildItem, phyName, motionCouplingIDs[i], status, getMotionCouplingPic(motionCoupling->getType()));
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //驱动
        vector<int> motionDriveIDs = modelData->getAllPhyID(Physical_MotionDrive);
        if (motionDriveIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionDriveIDs.size());
            partItem = _treeModel->getParentItem(Motion_DriveParentItem);
            for (int i = 0; i < motionDriveIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionDrive, motionDriveIDs[i]);
                mMotionDriveBase* motionDrive = modelData->getManager<mMotionDriveManager>(Physical_MotionDrive).getData(motionDriveIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionDrive, motionDriveIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "驱动", phyName, motionDriveIDs[i]);
                childitem = new mTreeItem(modelID, Motion_DriveChildItem, phyName, motionDriveIDs[i], status, getMotionDrivePic(motionDrive->getTargetType()));
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //标记点
        vector<int> motionMarkerIDs = modelData->getAllPhyID(Physical_MotionMarker);
        if (motionMarkerIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionMarkerIDs.size());
            partItem = _treeModel->getParentItem(Motion_MarkerParentItem);
            for (int i = 0; i < motionMarkerIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionMarker, motionMarkerIDs[i]);
                mMotionMarker* motionMarker = modelData->getManager<mMotionMarkerManager>(Physical_MotionMarker).getData(motionMarkerIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionMarker, motionMarkerIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "标记点", phyName, motionMarkerIDs[i]);
                childitem = new mTreeItem(modelID, Motion_MarkerChildItem, phyName, motionMarkerIDs[i], status, ":/mMainWidgets/Icon/caotu/dian.png");
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //力元（包含矢量力和标量力）
        vector<int> motionForceElementIDs = modelData->getAllPhyID(Physical_MotionVectorForce);
        if (motionForceElementIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionForceElementIDs.size());
            partItem = _treeModel->getParentItem(Motion_LiYuanParentItem);
            for (int i = 0; i < motionForceElementIDs.size(); i++)
            {
                int feId = motionForceElementIDs[i];
                phyName = modelData->getNameByPhyID(Physical_MotionVectorForce, feId);
                mMotionForceElement* fe = modelData->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getData(feId);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionVectorForce, feId);
                {
                    QString feLabel = "力元";
                    if (fe && fe->getLoadType() == VectorLoadType::ScalarForce) feLabel = "标量力";
                    getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, feLabel, phyName, feId);
                    childitem = new mTreeItem(modelID, Motion_LiYuanChildItem, phyName, feId, status,
                        fe ? getForceElementPic(fe->getLoadType()) : QString(":/mMainWidgets/Icon/BrowserTree/motion_liYuanParentItem.png"));
                }
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //弹簧
        vector<int> motionSpringIDs = modelData->getAllPhyID(Physical_MotionSpring);
        if (motionSpringIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionSpringIDs.size());
            partItem = _treeModel->getParentItem(Motion_SpringParentItem);
            for (int i = 0; i < motionSpringIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionSpring, motionSpringIDs[i]);
                mMotionSpringBase* spring = modelData->getManager<mMotionSpringManager>(Physical_MotionSpring).getData(motionSpringIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionSpring, motionSpringIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "弹簧", phyName, motionSpringIDs[i]);
                childitem = new mTreeItem(modelID, Motion_SpringChildItem, phyName, motionSpringIDs[i], status, getMotionSpringPic(spring->getType()));
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //接触
        vector<int> motionContactIDs = modelData->getAllPhyID(Physical_MotionContact);
        if (motionContactIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionContactIDs.size());
            partItem = _treeModel->getParentItem(Motion_ContactParentItem);
            for (int i = 0; i < motionContactIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionContact, motionContactIDs[i]);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionContact, motionContactIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "接触", phyName, motionContactIDs[i]);
                childitem = new mTreeItem(modelID, Motion_ContactChildItem, phyName, motionContactIDs[i], status, ":/mMainWidgets/Icon/Motion/contact.png");
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //传感器
        vector<int> motionSensorIDs = modelData->getAllPhyID(Physical_MotionSensor);
        if (motionSensorIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionSensorIDs.size());
            partItem = _treeModel->getParentItem(Motion_SensorParentItem);
            for (int i = 0; i < motionSensorIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionSensor, motionSensorIDs[i]);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionSensor, motionSensorIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "传感器", phyName, motionSensorIDs[i]);
                //按传感器类型取图标（同运动副/耦合副子节点口径）
                auto* sensorData = getCVM->getModelData()->getManager<mMotionSensorManager>(Physical_MotionSensor).getData(motionSensorIDs[i]);
                const QString sensorPic = (sensorData != nullptr) ? getMotionSensorPic(sensorData->getType())
                    : QStringLiteral(":/mMainWidgets/Icon/Motion/sensor.png");
                childitem = new mTreeItem(modelID, Motion_SensorChildItem, phyName, motionSensorIDs[i], status, sensorPic);
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //控制器
        vector<int> motionControllerIDs = modelData->getAllPhyID(Physical_MotionController);
        if (motionControllerIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionControllerIDs.size());
            partItem = _treeModel->getParentItem(Motion_ControllerParentItem);
            for (int i = 0; i < motionControllerIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionController, motionControllerIDs[i]);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionController, motionControllerIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "控制器", phyName, motionControllerIDs[i]);
                childitem = new mTreeItem(modelID, Motion_ControllerChildItem, phyName, motionControllerIDs[i], status, ":/mMainWidgets/Icon/Motion/controlCalculation.png");
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //控制输入
        vector<int> motionControlInputIDs = modelData->getAllPhyID(Physical_MotionControlInput);
        if (motionControlInputIDs.size() > 0)
        {
            QVector<mTreeItem*> childItems;
            childItems.reserve(motionControlInputIDs.size());
            partItem = _treeModel->getParentItem(Motion_ControlInputParentItem);
            for (int i = 0; i < motionControlInputIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionControlInput, motionControlInputIDs[i]);
                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionControlInput, motionControlInputIDs[i]);
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info, "控制输入", phyName, motionControlInputIDs[i]);
                childitem = new mTreeItem(modelID, Motion_ControlInputChildItem, phyName, motionControlInputIDs[i], status, ":/mMainWidgets/Icon/BrowserTree/motion_driveParentItem.png");
                childItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(childItems));
        }
        //求解方案：方案对象节点直接浮动挂多体根下（沿用原有行为，不设固定父分组），
        //每个方案自带固定的 动画/结果导出/运动曲线 三个子节点
        vector<int> motionScenarioIDs = modelData->getAllPhyID(Physical_MotionScenario);
        if (motionScenarioIDs.size() > 0)
        {
            for (int i = 0; i < motionScenarioIDs.size(); i++)
            {
                phyName = modelData->getNameByPhyID(Physical_MotionScenario, motionScenarioIDs[i]);
                mMotionScenario* motionScenario = modelData->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(motionScenarioIDs[i]);

                info.clear();
                status = getCVM->getModelData()->getDataStatus(info, Physical_MotionScenario, motionScenarioIDs[i]);
                //模块名必须经 moduleOf 取规范名"方案"：单对象刷新（updateDynamicInfoData）
                //的列表/表格删除按此名匹配，此前手写"求解方案"导致删除永不命中——
                //求解完成后"多体方案未求解"警告行永久残留
                getCVM->getDataStatusManager()->AddListOutputInfoData(dynamicInfoData, info,
                    mDataStatusManager::moduleOf(Physical_MotionScenario), phyName, motionScenarioIDs[i]);

                partItem = new mTreeItem(modelID, Motion_ScenarioParentItem, phyName, motionScenarioIDs[i], status);
                _treeModel->appendChildren(_treeModel->getParentItem(Motion_RootItem), partItem);

                //动画
                auto aniItem = new mTreeItem(modelID, Motion_AnimationChildItem, "动画", motionScenarioIDs[i], status);
                _treeModel->appendChildren(partItem, aniItem);

                //结果导出
                auto loadExportItem = new mTreeItem(modelID, Motion_LoadExportChildItem, "结果导出", motionScenarioIDs[i], status);
                _treeModel->appendChildren(partItem, loadExportItem);

                //运动曲线
                auto curveSelItem = new mTreeItem(modelID, Motion_CurveChildItem, "运动曲线", motionScenarioIDs[i], status);
                _treeModel->appendChildren(partItem, curveSelItem);
            }
        }
        connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
        //多体形态：重建后选中并展开多体动力学顶层根（模型根整树已隐藏，选中它会落空）
        selectedItem(MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion
            ? _treeModel->getParentItem(Motion_RootItem) : rootItem);
        mOutputInfoWindow::getInstance()->slot_tableInfoDataShow(true);
    }



    void mTreeView::slot_updateBrowserTreeStatus(MxFunctions::PhysicalFieldType phyType, QHash<int, MxFunctions::BrowserTreeStatus> phyData)
    {
        mTreeItem* item = nullptr;
        int phyID;
        BrowserTreeType type = TreeRootItem;
        switch (phyType)
        {
        case MxFunctions::Physical_Part:
            type = ModelChildItem;
            break;
        case MxFunctions::Physical_Material:
            type = MaterialChildItem;
            break;
        case MxFunctions::Physical_LoadBoundary:
        {
            mCaseBase* caseBase; 
            for (auto it = phyData.begin(); it != phyData.end(); ++it)
            {
                phyID = it.key();
                auto load = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(phyID);
                if (load == nullptr) continue;

                if (load->isLoadOrBoundary() == 0)
                {
                    type = CaseLoadChildItem;
                }
                else
                {
                    type = CaseBCChildItem;
                }

                const unordered_map<int, double>& caseIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<unordered_map<int, double>>({ phyType, it.key() }, Physical_Case);
                for (auto& caseID : caseIDs)
                {
                    caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID.first);
                    if (caseBase == nullptr)
                        continue;

                    item = _treeModel->getChildrenItem(type, phyID, caseID.first);
                    setTreeItemStatus(item, phyData[phyID]);
                }

                //载荷边界管理器
                item = _treeModel->getChildrenItem(type, phyID);
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_PredefinedField:
        {
            for (auto it = phyData.begin(); it != phyData.end(); ++it)
            {
                phyID = it.key();

                const unordered_map<int, double>& caseIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<unordered_map<int, double>>({ phyType, it.key() }, Physical_Case);
                for (auto& caseID : caseIDs)
                {
                    mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID.first);
                    if (caseBase == nullptr)
                        continue;

                    item = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, phyID, caseID.first);
                    setTreeItemStatus(item, phyData[phyID]);
                }

                //预定义场管理器
                item = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, phyID);
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_Connector:
            type = ConnectorChildItem;
            break;
        case MxFunctions::Physical_Deputy:
            type = DeputyChildItem;
            break;
        case MxFunctions::Physical_Contact:
            type = ContactChildItem;
            break;
        case MxFunctions::Physical_Set:
            type = SetChildItem;
            break;
        case MxFunctions::Physical_Coordinate:
            type = CoordinateChildItem;
            break;
        case MxFunctions::Physical_Function:
            type = FunctionChildItem;
            break;
        case MxFunctions::Physical_Property:
            type = PropertyChildItem;
            break;
        case MxFunctions::Physical_NonStructuralQuality:
            type = NonStructuralQualityChildItem;
            break;
        case MxFunctions::Physical_Field:
            type = FieldChildItem;
            break;
        case MxFunctions::Physical_PropertyShape:
            type = SectionShapeChildItem;
            break;
        case MxFunctions::Physical_MeshPanel:
            type = MeshChildItem;
            break;
        case MxFunctions::Physical_MeshOptPanel:
            type = MeshOptChildItem;
            break;
        case MxFunctions::Physical_PeriodicBoundary:
            type = PeriodicBoundaryChildItem;
            break;
        case MxFunctions::Physical_OptimationGoal:
            {
                for (auto it = phyData.begin(); it != phyData.cend(); ++it)
                {
                    item = _treeModel->getParentItem(OptimizationGoalParentItem);
                    setTreeItemStatus(item, it.value());
                }
                return;
            }
        case MxFunctions::Physical_OptimationDesign:
            type = OptimizationDesignChildItem;
            break;
        case MxFunctions::Physical_OptimationConstraint:
            type = OptimizationConstraintsChildItem;
            break;
        case MxFunctions::Physical_RunnerOptimationGoal:
            {
                for (auto it = phyData.begin(); it != phyData.cend(); ++it)
                {
                    item = _treeModel->getParentItem(RunnerOptimizationGoalParentItem);
                    setTreeItemStatus(item, it.value());
                }
                return;
            }
        case MxFunctions::Physical_RunnerOptimationDesign:
            type = RunnerOptimizationDesignChildItem;
            break;
        case MxFunctions::Physical_RunnerOptimationConstraint:
            type = RunnerOptimizationConstraintsChildItem;
            break;
        case MxFunctions::Physical_FreqSamplPoint:
            type = FrequencySamplingPointChildItem;
            break;
        case MxFunctions::Physical_MotionBody:
            type = Motion_MovingBodyChildItem;
            break;
        case MxFunctions::Physical_MotionJoint:
            type = Motion_ConnectorChildItem;
            break;
        case MxFunctions::Physical_MotionCoupling:
            type = Motion_CouplingChildItem;
            break;
        case MxFunctions::Physical_MotionDrive:
            type = Motion_DriveChildItem;
            break;
        case MxFunctions::Physical_MotionMarker:
            type = Motion_MarkerChildItem;
            break;
        case MxFunctions::Physical_MotionVectorForce:
            type = Motion_LiYuanChildItem;
            break;
        case MxFunctions::Physical_MotionContact:
            type = Motion_ContactChildItem;
            break;
        case MxFunctions::Physical_MotionScenario:
            int phyID;
            {
                auto parentItem = _treeModel->getParentItem(Motion_RootItem);
                for (auto item : parentItem->getChildrenItem())
                {
                    phyID = getPhyIdByItem(item);
                    if (phyData.contains(phyID))
                    {
                        for (auto item1 : item->getChildrenItem())
                        {
                            setTreeItemStatus(item1, phyData[phyID]);
                        }
                    }
                }
            }
            break;
        case MxFunctions::Physical_GenericParam:
            type = GenericParamChildItem;
            break;
        case MxFunctions::Physical_GenericAssign:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getParentItem(GenericAssignParentItem);
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_GenericMatrix:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getParentItem(GenericMatrixParentItem);
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_ERP:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getParentItem(ERPParentItem);
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_ModalContri:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getChildrenItem(ModalContriChildItem, it.key());
                setTreeItemStatus(item, it.value());

                item = _treeModel->getChildrenItem(CaseModalContriChildItem, it.key(), it.key());
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_NodeContri:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getChildrenItem(NodeContriChildItem, it.key());
                setTreeItemStatus(item, it.value());

                item = _treeModel->getChildrenItem(CaseNodeContriChildItem, it.key(), it.key());
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_PanelContri:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getChildrenItem(PanelContriChildItem, it.key());
                setTreeItemStatus(item, it.value());

                item = _treeModel->getChildrenItem(CasePanelContriChildItem, it.key(), it.key());
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_TPContri:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getChildrenItem(TPContriChildItem, it.key());
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_PeakFilter:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getChildrenItem(PeakFilterChildItem, it.key());
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_ASCouplingSearch:
        {
            for (auto it = phyData.begin(); it != phyData.cend(); ++it)
            {
                item = _treeModel->getParentItem(ASCouplingSearchParentItem);
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        case MxFunctions::Physical_Laminate:
        {
            for (auto it = phyData.begin(); it != phyData.end(); ++it)
            {
                //单层板父节点
                phyID = it.key();
                auto ply = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getLaminateData(phyID);
                if (ply == nullptr) continue;

                if (ply->getLaminateType() == SglLayerPlate)
                {
                    type = SglLayerPlateChildItem;
                }
                else if (ply->getLaminateType() == LamPlate)
                {
                    type = LamPlateChildItem;
                }

                item = _treeModel->getChildrenItem(type, phyID);
                setTreeItemStatus(item, it.value());
            }
        }
            return;
        case MxFunctions::Physical_ModelFix:
        {
            type = ModelFixChildItem;
            for (auto it = phyData.begin(); it != phyData.end(); ++it)
            {
                phyID = it.key();

                const unordered_map<int, double>& caseIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<unordered_map<int, double>>({ phyType, it.key() }, Physical_Case);
                for (auto& caseID : caseIDs)
                {
                    mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID.first);
                    if (caseBase == nullptr)
                        continue;

                    item = _treeModel->getChildrenItem(CaseModelFixChildItem, phyID, caseID.first);
                    setTreeItemStatus(item, phyData[phyID]);
                }

                // 模型修改管理器
                item = _treeModel->getChildrenItem(ModelFixChildItem, phyID);
                setTreeItemStatus(item, it.value());
            }
            return;
        }
        default:
            break;
        }

        if (type == TreeRootItem) return;

        for (auto it = phyData.begin(); it != phyData.cend(); ++it)
        {
            item = _treeModel->getChildrenItem(type, it.key());
            setTreeItemStatus(item, it.value());
        }
    }

    void mTreeView::slot_signal_osgWidgetEvent(QEvent* event)
    {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            osg_mousePressEvent(mouseEvent);
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            osg_mouseReleaseEvent(mouseEvent);
        }
    }

    void mTreeView::longPressDetected()
    {
        m_osgPressTimer->stop();
        m_isPressing = false;
    }

    void mTreeView::osg_mousePressEvent(QMouseEvent* event)
    {
        Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
        if ((modifiers & Qt::ControlModifier) || (modifiers & Qt::ShiftModifier))
        {
            return;
        }

        if (event->button() == Qt::RightButton) {
            m_osgPressTimer->start(150);
            m_isPressing = true;
        }
    }

    void mTreeView::osg_mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::RightButton) {
            m_osgPressTimer->stop();
            if (m_isPressing)
            {
                if (!getCVM)
                {
                    return;
                }

                if (m_Menu)
                {
                    delete m_Menu;
                    m_Menu = nullptr;
                }
                m_Menu = new mUiMenu;
                ThemeManager::applyMenuStyle(m_Menu);

                auto partIds = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
                QAction* action = nullptr;
                if (partIds.size() > 0)
                {
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏当前");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideCurrentPart()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "隐藏其他");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideOtherPart()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xianshiquanbu.png"), "显示全部");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowAllPart()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xianshibujian.png"), "显示模型树中部件");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowTreePart()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fanxiangxianshi.png"), "反向显示");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_menuReverseShow()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/filterTree.png"), "基于可见部件过滤模型树");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_filterTreeByPartMask()));
                }
                else
                {
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xianshiquanbu.png"), "显示全部");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowAllPart()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xianshibujian.png"), "显示模型树中部件");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowTreePart()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fanxiangxianshi.png"), "反向显示");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_menuReverseShow()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/filterTree.png"), "基于可见部件过滤模型树");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_filterTreeByPartMask()));
                    action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/mapTree.png"), "映射到模型树");
                    connect(action, SIGNAL(triggered()), this, SLOT(slot_mapRelationTree()));
                }
                m_Menu->exec(QCursor::pos());
                m_isPressing = false;
            }
        }
    }

    void mTreeView::slot_menuCreateMesh()
    {
        vector<int> partIds;
        for (auto& idx : getSelectIndex())
        {
            partIds.push_back(getPhyIdByModelIndex(idx));
        }
        mInfoWindow::getInstance()->createMeshByPartId(partIds);
    }

    void mTreeView::slot_menuDeleteGeo()
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        int partID, partType;
        set<int> deletePartIDs;
        vector<int> deleteItems;
        QModelIndexList deleteIndexs;
        QModelIndexList idxs = getSelectIndex();
        for (auto& idx : idxs)
        {
            partID = getPhyIdByModelIndex(idx);
            partType = meshData->returnPartType(getTextByModelIndex(idx));
            if (partType == 1)
            {
                deleteIndexs.push_back(idx);
                deletePartIDs.insert(partID);
                deleteItems.push_back(partID);
            }
            else if (partType == 3)
            {
                deletePartIDs.insert(partID);
            }
        }

        if (deletePartIDs.size() == 0)
            return;

        // 撤消接入：本删除直写 PartManager（removePart），不经 document().remove——
        // 手工三段式包裹；undo 后由 refreshAfterHistoryRestore 重建树/渲染/_partdata 镜像。
        void* historyToken = getGEOServer.historyOperationBegin();

        //删除装配父节点时级联收集全部子孙部件：子节点的网格数据、几何数据、渲染与注册信息必须一并清理，
        //否则树节点随父节点整行移除后，子部件的模型仍会残留在渲染窗口中。
        const std::set<int> selectedIDs = deletePartIDs;
        deletePartIDs = getCVM->getModelData()->getGeoManager()->collectPartsWithChildren(selectedIDs);
        for (int id : deletePartIDs)
        {
            if (selectedIDs.count(id) == 0)
            {
                deleteItems.push_back(id);
            }
        }

        removeProxyIndexs(deleteIndexs);
        
        auto geoDatabase = getCVM->getModelData()->getGeoData();
        std::map<int, std::set<int>> geoMap;
        std::set<int> meshSolidId = getGEOServer.geoCalculate().getAllGeoID(3);
        std::set<int> meshFaceId = getGEOServer.geoCalculate().getAllGeoID(2);
        std::set<int> meshLineId = getGEOServer.geoCalculate().getAllGeoID(1);
        meshData->deleteGeoPart(deletePartIDs);
        //先释放网格数据，再释放几何数据，否则会出现无法找到几何数据的情况导致崩溃。
        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(deletePartIDs);
        std::set<int> meshSolidId1 = getGEOServer.geoCalculate().getAllGeoID(3);
        std::set<int> meshFaceId1 = getGEOServer.geoCalculate().getAllGeoID(2);
        std::set<int> meshLineId1 = getGEOServer.geoCalculate().getAllGeoID(1);
        std::set<int> diffSolid;//删除的几何体
        std::set<int> diffFace;//删除的几何面
        std::set<int> diffLine;//删除的几何线
        std::set_difference(meshSolidId.begin(), meshSolidId.end(), meshSolidId1.begin(), meshSolidId1.end(), std::inserter(diffSolid, diffSolid.begin()));
        std::set_difference(meshFaceId.begin(), meshFaceId.end(), meshFaceId1.begin(), meshFaceId1.end(), std::inserter(diffFace, diffFace.begin()));
        std::set_difference(meshLineId.begin(), meshLineId.end(), meshLineId1.begin(), meshLineId1.end(), std::inserter(diffLine, diffLine.begin()));
        geoMap[3] = diffSolid;
        geoMap[2] = diffFace;
        geoMap[1] = diffLine;
        emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshList(geoMap);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Part, deleteItems);
        if (idxs.size() == deleteIndexs.size())
        {
            selectedItem(partGroupItem(_treeModel));
        }
        slot_singleClickItem();
        // 撤消收口（删除完成）：前态快照入撤消栈（整个删除 = 一个撤消条目）。
        getGEOServer.historyOperationSucceeded(
            historyToken, EventDispatcher::mGeoOperationKind::Delete);
    }

    void mTreeView::slot_menuDeleteOtherGeo()
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty())
            return;

        // 收集当前选中的部件ID
        set<int> selectedPartIDs;
        for (auto& idx : idxs)
        {
            selectedPartIDs.insert(getPhyIdByModelIndex(idx));
        }

        // 收集需要删除的部件ID（所有部件中排除选中的）
        set<int> deletePartIDs;
        vector<int> deleteItems;
        QModelIndexList deleteIndexs;
        auto parentItem = partGroupItem(_treeModel);
        for (auto& item : parentItem->getChildrenItem())
        {
            int partID = getPhyIdByItem(item);
            if (selectedPartIDs.find(partID) == selectedPartIDs.end())
            {
                
                QModelIndex index = _treeModel->indexFromItem(item);
                int partType = meshData->returnPartType(
                    getTextByModelIndex(_treeProxyModel->mapFromSource(index)));

                if (partType == 1)
                {
                    deletePartIDs.insert(partID);
                    deleteItems.push_back(partID);
                    deleteIndexs.push_back(_treeProxyModel->mapFromSource(index));
                }
            }
        }

        if (deletePartIDs.size() == 0)
            return;

        // 撤消接入：同 slot_menuDeleteGeo（直写路径，手工三段式包裹）。
        void* historyToken = getGEOServer.historyOperationBegin();
        removeProxyIndexs(deleteIndexs);

        auto geoDatabase = getCVM->getModelData()->getGeoData();
        std::map<int, std::set<int>> geoMap;
        std::set<int> meshSolidId = getGEOServer.geoCalculate().getAllGeoID(3);
        std::set<int> meshFaceId = getGEOServer.geoCalculate().getAllGeoID(2);
        std::set<int> meshLineId = getGEOServer.geoCalculate().getAllGeoID(1);
        meshData->deleteGeoPart(deletePartIDs);
        //先释放网格数据，再释放几何数据，否则会出现无法找到几何数据的情况导致崩溃。
        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet(deletePartIDs);
        std::set<int> meshSolidId1 = getGEOServer.geoCalculate().getAllGeoID(3);
        std::set<int> meshFaceId1 = getGEOServer.geoCalculate().getAllGeoID(2);
        std::set<int> meshLineId1 = getGEOServer.geoCalculate().getAllGeoID(1);
        std::set<int> diffSolid;//删除的几何体
        std::set<int> diffFace;//删除的几何面
        std::set<int> diffLine;//删除的几何线
        std::set_difference(meshSolidId.begin(), meshSolidId.end(), meshSolidId1.begin(), meshSolidId1.end(), std::inserter(diffSolid, diffSolid.begin()));
        std::set_difference(meshFaceId.begin(), meshFaceId.end(), meshFaceId1.begin(), meshFaceId1.end(), std::inserter(diffFace, diffFace.begin()));
        std::set_difference(meshLineId.begin(), meshLineId.end(), meshLineId1.begin(), meshLineId1.end(), std::inserter(diffLine, diffLine.begin()));
        geoMap[3] = diffSolid;
        geoMap[2] = diffFace;
        geoMap[1] = diffLine;
        emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshList(geoMap);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Part, deleteItems);
        slot_singleClickItem();
        // 撤消收口（删除完成）。
        getGEOServer.historyOperationSucceeded(
            historyToken, EventDispatcher::mGeoOperationKind::Delete);
    }

    void mTreeView::slot_menuDeleteMesh()
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        QString partName; vector<int> deleteItems; vector<int> updateItems;
        int partID, partType;
        QModelIndexList deleteIndexs;
        QModelIndexList idxs = getSelectIndex();
        for (auto& idx : idxs)
        {
            partName = getTextByModelIndex(idx);
            partID = getPhyIdByModelIndex(idx);
            partType = meshData->returnPartType(getTextByModelIndex(idx));
            if (partType == 2)
            {
                meshData->deleteMeshPart(partName);
                deleteIndexs.push_back(idx);
                deleteItems.push_back(partID);
            }
            else if (partType == 3)
            {
                meshData->deleteMeshPart(partName);
                updateItems.push_back(partID);
            }
        }

        removeProxyIndexs(deleteIndexs);

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Part, deleteItems);
        getCVM->getDataStatusManager()->updateDynamicInfoData({ {Physical_Part, updateItems} });
        if (idxs.size() == deleteIndexs.size())
        {
            selectedItem(partGroupItem(_treeModel));
        }
        slot_singleClickItem();
    }

    void mTreeView::slot_menuShowCurrentPart()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;

        set<int> showPartIDs;
        vector<int> partIDs;
        for (auto idx : idxs)
        {
            showPartIDs.insert(getPhyIdByModelIndex(idx));
            partIDs.push_back(getPhyIdByModelIndex(idx));
        }
        getCVM->getModelData()->getMeshData()->setSelectPartMask(showPartIDs, false);
        hightPart();
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), showPartIDs);
    }

    void mTreeView::slot_menuHideCurrentPart()
    {
        vector<int> partIDs = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        set<int> hidePartIDs;
        for_each(partIDs.begin(), partIDs.end(), [&](int id) {hidePartIDs.insert(id); });
        getCVM->getModelData()->getMeshData()->setSelectPartMask(hidePartIDs, true);
        MechPreData::clearPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->updateRender();
        mInfoWindow::getInstance()->updateGeoInfo(partIDs);
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), hidePartIDs);
    }

    void mTreeView::slot_menuHideOtherPart()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;

        vector<int> partIDs;
        for (auto idx : idxs)
        {
            partIDs.push_back(getPhyIdByModelIndex(idx));
        }

        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        int phyID;
        set<int> hidePartIDs;
        //部件
        vector<int> allPartIDs = meshData->getAllPartIds();
        for (int i = 0; i < allPartIDs.size(); i++)
        {
            phyID = allPartIDs[i];
            if (std::find(partIDs.begin(), partIDs.end(), phyID) == partIDs.cend())
                hidePartIDs.insert(phyID);
        }

        getCVM->getModelData()->getMeshData()->setSelectPartMask(hidePartIDs, true);
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), hidePartIDs);
    }

    void mTreeView::slot_menuShowAllPart()
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        set<int> partIDs;
        //部件
        vector<int> allPartIDs = meshData->getAllPartIds();
        for (int i = 0; i < allPartIDs.size(); i++)
        {
            partIDs.insert(allPartIDs[i]);
        }

        getCVM->getModelData()->getMeshData()->setSelectPartMask(partIDs, false);
        hightPart();
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), partIDs);
    }

    void mTreeView::slot_menuHideAllPart()
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        set<int> partIDs;
        //部件
        vector<int> allPartIDs = meshData->getAllPartIds();
        for (int i = 0; i < allPartIDs.size(); i++)
        {
            partIDs.insert(allPartIDs[i]);
        }

        getCVM->getModelData()->getMeshData()->setSelectPartMask(partIDs, true);
        MechPreData::clearPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->updateRender();
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), partIDs);
    }

    void mTreeView::slot_menuShowTreePart()
    {
        set<int> partIDs;
        auto parentItem = partGroupItem(_treeModel);
        for (auto& item : parentItem->getChildrenItem())
        {
            if (!getHiddenByItem(item))
                partIDs.insert(getPhyIdByItem(item));
        }

        getCVM->getModelData()->getMeshData()->setSelectPartMask(partIDs, false);
        hightPart();
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), partIDs);
    }

    void mTreeView::slot_menuReverseShow()
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        meshData->reverseShowPart();
        hightPart();
        set<int> partIDs;
        vector<int> allPartIDs = meshData->getAllPartIds();
        for (int i = 0; i < allPartIDs.size(); i++)
        {
            partIDs.insert(allPartIDs[i]);
        }
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), partIDs);
    }

    void mTreeView::slot_menuTreePartReverseShow()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;

        set<int> showPartIDs; vector<int> showPartIDs_vec;
        set<int> hidePartIDs;
        for (auto idx : idxs)
        {
            int partID = getPhyIdByModelIndex(idx);
            bool mask = getCVM->getModelData()->getMeshData()->getPartMask(partID);
            if (mask)
            {
                showPartIDs.insert(partID);
                showPartIDs_vec.push_back(partID);
            }
            else
            {
                hidePartIDs.insert(partID);
            }
        }
        getCVM->getModelData()->getMeshData()->setSelectPartMask(showPartIDs, false);
        getCVM->getModelData()->getMeshData()->setSelectPartMask(hidePartIDs, true);
        MechPreData::clearPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        MechPreData::addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), showPartIDs_vec);
        UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->updateRender();
        set<int> changedIDs = showPartIDs;
        changedIDs.insert(hidePartIDs.begin(), hidePartIDs.end());
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), changedIDs);
    }

    void mTreeView::slot_filterTreeByPartMask()
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        auto partItem = partGroupItem(_treeModel);

        //部件
        int size = getSelectIndex().size();
        QVector<mTreeItem*> deSelectItems;
        for (auto& item : partItem->getChildrenItem())
        {
            bool isMask = meshData->getPartMask(getPhyIdByItem(item));
            setTreeItemHidden(item, isMask);
            if (isMask && isItemSelect(item))
            {
                deSelectItems.push_back(item);
            }
        }

        deSelectItem(deSelectItems);
        if (size == deSelectItems.size())
        {
            selectedItem(partItem);
        }
    }

    void mTreeView::slot_mapRelationTree()
    {
        mSelectMenuFunction::getInstance()->slot_mappingPart(false);
        vector<int> partIDs = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
        if (partIDs.empty())
            return;

        auto parentItem = partGroupItem(_treeModel);
        QVector<mTreeItem*> selectItems;
        for (auto& item : parentItem->getChildrenItem())
        {
            if (!getHiddenByItem(item) && std::find(partIDs.begin(), partIDs.end(), getPhyIdByItem(item)) != partIDs.cend())
            {
                selectItems.push_back(item);
            }
        }

        selectedItem(selectItems);
    }

    void mTreeView::slot_updatePartTree(bool isCreate, set<int> partID)
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        if (meshData == nullptr)
            return;

        QString phyName;
        int modelID = getCVM->getModelData()->getModelDataID();
        auto parentItem = partGroupItem(_treeModel);
        mTreeItem* lastItem = nullptr;
        if (isCreate)
        {
            QVector<mTreeItem*> childitems;
            mTreeItem* childitem;
            for (auto& id : partID)
            {
                phyName = meshData->getPartNameByPartId(id);
                childitem = new mTreeItem(modelID, ModelChildItem, phyName, id);
                childitems.push_back(childitem);
                lastItem = childitem;
            }
            _treeModel->appendChildren(parentItem, std::move(childitems));
        }
        else
        {

        }

   /*     if (!lastItem)
            return;*/

        if (!isSketch)
        {
            //selectedItem(lastItem);
        }
        else if (lastItem != nullptr)
        {
            //curSketchItem = lastItem;
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Part, vector<int>{partID.begin(), partID.end()}} });
        //setTreeItemStatus(_treeModel->getParentItem(MeshParentItem), Tree_Failure);
    }

    void mTreeView::slot_updatePartTree_2(bool isCreate, std::set<int> partID)
    {
        //中性数据管理器
        GeoNeutralData::mGeoPartManager* partMgr = getCVM->getModelData()->getGeoManager()->getGeoPartMgr();
        if (!partMgr)
            return;

        // 过滤非当前模型的信号，避免多窗口时跨模型污染
        bool hasAnyPartInCurrentModel = false;
        for (int id : partID)
        {
            if (partMgr->containsPart(id))
            {
                hasAnyPartInCurrentModel = true;
                break;
            }
        }
        if (!hasAnyPartInCurrentModel)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto modelParentItem = partGroupItem(_treeModel);
        mTreeItem* lastItem = nullptr;

        // 缓存本次已创建的树节点，避免同一 Part 重复创建
        std::map<int, mTreeItem*> createdItems;

        std::function<mTreeItem*(int)> createOrGetPartItem = [&](int partId) -> mTreeItem* {
            // Part 节点不使用 caseID；第四个实参才是 modelID，不能用 modelID 查 caseID。
            mTreeItem* existing = _treeModel->getChildrenItem(ModelChildItem, partId, -1, modelID);
            if (existing)
                return existing;

            // 本次已创建则直接返回
            auto it = createdItems.find(partId);
            if (it != createdItems.end())
                return it->second;

            if (!partMgr->containsPart(partId))
                return nullptr;

            GeoNeutralData::GN_Part part = partMgr->getPartById(partId);
            //【部件树扁平化】结构节点（无 Compound，仅记录父子关系）不建树节点
            if (part.getShape() <= 0)
                return nullptr;

            const GeoNeutralData::GN_Metadata& metadata = part.getMetadata();
            QString name = metadata.name();
            if (name.isEmpty())
                name = QString("Part_%1").arg(partId);

            mTreeItem* item = new mTreeItem(modelID, ModelChildItem, name, partId);
            createdItems[partId] = item;

            //【部件树扁平化】部件节点一律直接挂"部件"分组：层级关系仅保留在
            // GN_Part 的 parentId/childIds 中，由后续装配树做层级管理
            _treeModel->appendChildren(modelParentItem, item);

            return item;
        };

        if (isCreate)
        {
            for (int id : partID)
            {
                mTreeItem* item = createOrGetPartItem(id);
                if (item)
                {
                    lastItem = item;
                }
            }
        }
        else
        {
            // 更新分支：按当前 mGeoPartManager 的父子关系重建这些 Part 节点
            for (int id : partID)
            {
                // 如果旧节点存在，先删除（会连带删除旧子树）
                mTreeItem* oldItem = _treeModel->getChildrenItem(ModelChildItem, id, -1, modelID);
                if (oldItem)
                {
                    _treeModel->removeTreeItem(oldItem);
                }
            }

            // 删除后清空缓存，避免悬挂指针
            createdItems.clear();

            // 重新创建，保证与当前父子关系一致
            for (int id : partID)
            {
                mTreeItem* item = createOrGetPartItem(id);
                if (item)
                {
                    lastItem = item;
                }
            }
        }

        /*     if (!lastItem)
                 return;*/

        if (!isSketch)
        {
            //selectedItem(lastItem);
        }
        else if (lastItem != nullptr)
        {
            //curSketchItem = lastItem;
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Part, vector<int>{partID.begin(), partID.end()}} });
    }

    void mTreeView::fillAssemblyTree(MechModelData::mModelData* modelData)
    {
        AssemblyData::mAssemblyManager& asmMgr = modelData->getAssemblyManager();
        mTreeItem* asmParentItem = _treeModel->getParentItem(AssemblyParentItem);
        if (asmParentItem == nullptr)
            return;

        const int modelID = modelData->getModelDataID();
        mMeshData* meshData = modelData->getMeshData();

        //递归挂载装配实例节点：parentID 记录父装配ID（顶层为 0），同一装配被多父引用时树实例按此区分
        std::function<void(int, int, mTreeItem*)> mountAssembly = [&](int assemblyId, int parentAssemblyId, mTreeItem* parentTreeItem)
        {
            mTreeItem* asmItem = new mTreeItem(modelID, AssemblyChildItem, asmMgr.getAssemblyName(assemblyId), assemblyId, Tree_Yes);
            _treeModel->appendAssemblyItem(parentTreeItem, asmItem, parentAssemblyId);

            for (int childAsmId : asmMgr.childAssemblies(assemblyId))
            {
                mountAssembly(childAsmId, assemblyId, asmItem);
            }

            //装配下的部件引用节点（部件为叶子）
            for (int partId : asmMgr.childParts(assemblyId))
            {
                const QString partName = meshData->getPartNameByPartId(partId);
                if (partName.isEmpty())
                    continue;
                mTreeItem* refItem = new mTreeItem(modelID, AssemblyPartRefItem, partName, partId, Tree_Yes);
                _treeModel->appendAssemblyItem(asmItem, refItem, assemblyId);
            }
        };

        for (int topId : asmMgr.topAssemblies())
        {
            mountAssembly(topId, 0, asmParentItem);
        }

        //自由区：不属于任何装配的部件，按名称自然排序附底
        const QSet<int> referencedParts = asmMgr.referencedPartIds();
        const std::vector<int> allPartIds = meshData->getAllPartIds();
        QVector<QPair<QString, int>> freeParts;
        for (int partId : allPartIds)
        {
            if (referencedParts.contains(partId))
                continue;
            freeParts.push_back({ meshData->getPartNameByPartId(partId), partId });
        }
        if (!freeParts.isEmpty())
        {
            QCollator collator;
            collator.setNumericMode(true);
            collator.setCaseSensitivity(Qt::CaseInsensitive);
            std::sort(freeParts.begin(), freeParts.end(), [&collator](const QPair<QString, int>& a, const QPair<QString, int>& b)
            {
                return collator.compare(a.first, b.first) < 0;
            });

            for (const auto& freePart : freeParts)
            {
                mTreeItem* refItem = new mTreeItem(modelID, AssemblyPartRefItem, freePart.first, freePart.second, Tree_Yes);
                _treeModel->appendAssemblyItem(asmParentItem, refItem, 0);
            }
        }
    }

    void mTreeView::slot_updateAssemblyTree()
    {
        mTreeItem* asmParentItem = _treeModel->getParentItem(AssemblyParentItem);
        if (asmParentItem == nullptr)
            return;

        //装配分组整体重建（装配数量级小，全量填充保证与关系数据始终一致）
        _treeModel->removeTreeChildrenItem(asmParentItem);
        fillAssemblyTree(getCVM->getModelData());
    }

    //部件删除后清理装配引用：被删部件从所有装配下摘除，装配树相应刷新
    static void removeDeletedPartAssemblyRefs(const std::set<int>& partID)
    {
        AssemblyData::mAssemblyManager& asmMgr = getCVM->getModelData()->getAssemblyManager();
        bool removedAny = false;
        for (int id : partID)
        {
            if (asmMgr.isPartReferenced(id))
            {
                asmMgr.removePartReferences(id);
                removedAny = true;
            }
        }
        if (removedAny)
        {
            emit mGlobalSignals::getInstance()->updateAssemblyTreeSig();
        }
    }

    void mTreeView::slot_deletePartTree(std::set<int> partID)
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        if (meshData == nullptr)
            return;

        auto _parentItem = partGroupItem(_treeModel);
        QModelIndexList deleteItems;
        int size = getSelectIndex().size();
        for (auto& item : _parentItem->getChildrenItem())
        {
            if (std::find(partID.begin(), partID.end(), getPhyIdByItem(item)) != partID.end() && !meshData->getPartData(getPhyIdByItem(item)))
            {
                QModelIndex index = _treeModel->indexFromItem(item);
                deleteItems.push_back(_treeProxyModel->mapFromSource(index));
            }
        }

        removeProxyIndexs(deleteItems);

        removeDeletedPartAssemblyRefs(partID);

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Part, vector<int>{partID.begin(), partID.end()});
    }

    void mTreeView::slot_deletePartTree_2(std::set<int> partID)
    {
        int modelID = getCVM->getModelData()->getModelDataID();
        for (int id : partID)
        {
            mTreeItem* item = _treeModel->getChildrenItem(ModelChildItem, id, -1, modelID);
            if (item)
            {
                _treeModel->removeTreeItem(item);
            }
        }

        removeDeletedPartAssemblyRefs(partID);

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Part, vector<int>{partID.begin(), partID.end()});
    }

    void mTreeView::slot_appendOneGeoModelRenderFinished()
    {
        hightPart();
    }

    void mTreeView::slot_importSuccessful()
    {
        //mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(_resultCaseID);
        //if (!MechIOFile::mIOFileManager::getInstance()->_cmdImportFilePath.isEmpty())
        //{
        //    QTimer::singleShot(2000, [caseBase]() {
        //        //自动快照执行体在PostWindow::mPostInfoHub，经总线解耦
        //        emit MechCommon::mMechGlobalSignals::getInstance()->clickAniQuickScreenshotPathSig(caseBase->getSolveDir());
        //        });

        //    //关闭程序
        //    QTimer::singleShot(3000, qApp, &QCoreApplication::quit);
        //}
    }

    void mTreeView::slot_importGeoFile()
    {
        emit MechCommon::mMechGlobalSignals::getInstance()->updateStatusImportGeo();
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导入该模型，请选择前处理模型！"));
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        // 后端感知的导入格式清单（与菜单栏导入共用 geoImportFileFilter：
        // OCC→STEP/IGES/BREP/IDF；CGM→STEP/IGES + 九种商用 CAD 直读）
        QString filesuffix = mMenuWidget::geoImportFileFilter();

        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("导入几何模型"),
            defaultDir,
            filesuffix
        );

        //STL/OBJ为三角面片格式，已改为通过“导入网格模型”导入
        if (!fileName.isEmpty())
        {
            const QString importSuffix = QFileInfo(fileName).suffix().toLower();
            if (importSuffix == "stl" || importSuffix == "obj")
            {
                mGlobalSignals::getInstance()->outputMessageSig(1,
                    QString("几何模型导入不支持%1文件，请通过“导入网格模型”导入！")
                        .arg(importSuffix.toUpper()), true);
                return;
            }
        }

        if (!fileName.isEmpty()) {
            mMenuWidget::importGeometryViaServer(this, fileName);
            //加入最近打开
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
            QSettings Settings(path, QSettings::IniFormat);
            // 添加一条"最近打开"
            QString file = fileName;
            QString key = QString("recent/%1").arg(QDateTime::currentMSecsSinceEpoch());

            Settings.beginGroup("recent");
            for (auto k : Settings.childKeys())
            {
                if (Settings.value(k) == fileName)
                    Settings.remove(QString("%1").arg(k));
            }
            Settings.endGroup();
            Settings.setValue(key, file);

            connect(getCVM->getPreRenderWidget(), &CCSMRendWidget::importGeoModelRenderFinished, [=]()
                {
                    QTimer::singleShot(2000, this, [=]() {
                        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
                        QString dir = QDir::currentPath();
                        QString num = key.split('/').last();
                        QString name = dir + "/../../Setting/Recent/" + QFileInfo(fileName).baseName() + num + ".png";
                        QDir().mkpath(dir + "/../../Setting/Recent");
                        if (getCVM->getPreRenderWidget() == nullptr)
                            return;
                        getCVM->getPreRenderWidget()->FastCapture(name);

                        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
                        if (viewportPrivate == nullptr)
                        {
                            return;
                        }
                        mBaseRend* rend = viewportPrivate->getRendWidget();
                        if (rend == nullptr)
                        {
                            return;
                        }

                        rend->FitView();
                        VMManager->setCameraSyncTimer(500, 30, [rend]() {
                            emit rend->signal_CameraSync();
                            });
                        });
                });
        }
    }

    void mTreeView::slot_exportGeoFileAll()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导出该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(STP(*.stp);;
    				STEP(*.step);;
    				IGES(*.iges);;
    				IGS(*.igs))";
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出几何模型"),
            defaultDir,
            filesuffix
        );

        if (!fileName.isEmpty()) {
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            getCVM->getModelData()->getGeoManager()->slotExportGeoFiles(fileName);
        }
    }

    void mTreeView::slot_exportGeoFileVisible()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导出该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        auto geoData = getCVM->getModelData()->getGeoData();
        auto meshData = getCVM->getModelData()->getMeshData();
        if (geoData == nullptr || meshData == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("几何数据或网格数据为空，无法导出！"));
            return;
        }

        //【OCC摘除·S9】可见部件收集改 GN：旧 getSetList 遍历随旧栈退役（恒空），
        // 从部件管理器收集 ID + meshData 隐藏过滤（等价语义）
        std::set<int> visibleSets;
        {
            auto geoMgr = getCVM->getModelData()->getGeoManager();
            auto partMgr = geoMgr ? geoMgr->getGeoPartMgr() : nullptr;
            if (partMgr != nullptr)
            {
                for (int pid : partMgr->getPartList())
                {
                    if (!meshData->getGeoPartMask(pid))
                    {
                        visibleSets.insert(pid);
                    }
                }
            }
        }

        if (visibleSets.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前没有可见的几何部件，无法导出！"));
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(STP(*.stp);;
    				STEP(*.step);;
    				IGES(*.iges);;
    				IGS(*.igs))";
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出几何模型"),
            defaultDir,
            filesuffix
        );

        if (!fileName.isEmpty()) {
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            getCVM->getModelData()->getGeoManager()->slotExportGeoFiles(fileName, visibleSets);
        }
    }

    void mTreeView::slot_exportGeoFileSelected()
    {
        if (getCVM->isPostModel())
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("无法导出该模型，请选择前处理模型！"));
            return;
        }

        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前没有选中的部件，无法导出！"));
            return;
        }

        auto geoData = getCVM->getModelData()->getGeoData();
        if (geoData == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("几何数据为空，无法导出！"));
            return;
        }

        //【OCC摘除·S9】选中部件收集改 GN：旧栈 set 反查退役（getSetList 恒空），
        // 模型树选中 ID 即部件 ID
        std::set<int> selectedSets;
        for (auto idx : idxs)
        {
            selectedSets.insert(getPhyIdByModelIndex(idx));
        }

        if (selectedSets.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("选中的部件没有对应的几何数据，无法导出！"));
            return;
        }

        //默认目录
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }

        QString filesuffix = R"(STP(*.stp);;
    				STEP(*.step);;
    				IGES(*.iges);;
    				IGS(*.igs))";
        QString fileName = QFileDialog::getSaveFileName(
            this,
            tr("导出几何模型"),
            defaultDir,
            filesuffix
        );

        if (!fileName.isEmpty()) {
            MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
            getCVM->getModelData()->getGeoManager()->slotExportGeoFiles(fileName, selectedSets);
        }
    }

    void mTreeView::slot_menuSetNew()
    {
        mInfoWindow::getInstance()->createSet();
    }

    void mTreeView::slot_menuSetDelete()
    {
        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_Set);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_Set);
        auto _setItem = _treeModel->getParentItem(SetParentItem);
        removeTreeItem(_setItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Set, phyIDs);
        selectedItem(_treeModel->getParentItem(ModelRootItem));
    }

    void mTreeView::slot_menuSetTypeDelete(MxFunctions::SetType setType)
    {
        //仅收集该类型的集合，其他类型分组不受影响
        MechSetData::mSetManager& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
        vector<int> phyIDs;
        for (int setID : setManager.getAllSetID())
        {
            MechSetData::mSetBase* set = setManager.getSetData(setID);
            if (set != nullptr && set->getSetType() == setType)
            {
                phyIDs.push_back(setID);
            }
        }
        if (phyIDs.empty())
        {
            return;
        }

        deleteSetsFromModel(phyIDs);
    }

    void mTreeView::deleteSetsFromModel(const std::vector<int>& phyIDs, bool reselect)
    {
        if (phyIDs.empty())
        {
            return;
        }

        //删除数据
        for (int setID : phyIDs)
        {
            getCVM->getModelData()->deleteByPhysicalType(Physical_Set, setID);
        }

        //删除树上对应节点，并清理变空的类型分组节点
        //断开选中变化信号期间完成全部树结构变更，避免触发unEditInfoWindow把当前面板清掉
        auto _setItem = _treeModel->getParentItem(SetParentItem);
        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
        for (int setID : phyIDs)
        {
            mTreeItem* item = _treeModel->getChildrenItem(SetChildItem, setID);
            if (item != nullptr)
            {
                _treeModel->removeTreeItem(item);
            }
        }
        if (_setItem != nullptr)
        {
            QVector<mTreeItem*> groupItems;
            for (mTreeItem* child : _setItem->getChildrenItem())
            {
                if (getTreeTypeByItem(child) == SetParentItem && getPhyIdByItem(child) != 0)
                    groupItems.push_back(child);
            }
            for (mTreeItem* group : groupItems)
            {
                removeEmptySetTypeGroupItem(group);
            }
        }

        //模型中已无任何集合：连同"集合"节点一起删除
        bool noSetsLeft = _setItem != nullptr
            && getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetID().empty();
        if (noSetsLeft)
        {
            removeTreeItem(_setItem);
        }
        connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);

        //面板删除路径(reselect=false)不改动选中项：选中变化会触发unEditInfoWindow使面板消失
        if (reselect)
        {
            if (noSetsLeft)
            {
                selectedItem(_treeModel->getParentItem(ModelRootItem));
            }
            else
            {
                selectedItem(_setItem);
            }
        }

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Set, phyIDs);
    }

    //集合管理器面板勾选删除集合：接收信号后同步删除数据与树节点
    void mTreeView::slot_deleteSets(const QVector<int>& setIDs)
    {
        std::vector<int> phyIDs(setIDs.begin(), setIDs.end());
        //保持集合管理器面板显示，不改动树选中项
        deleteSetsFromModel(phyIDs, false);
    }

    void mTreeView::slot_deleteLoads(const QVector<int>& loadIDs)
    {
        if (loadIDs.isEmpty())
        {
            return;
        }

        mTreeItem* _loadItem = _treeModel->getParentItem(LoadManagerParentItem);
        if (_loadItem == nullptr)
        {
            return;
        }

        deleteSymbol();

        QSet<int> deleteSet;
        for (int loadID : loadIDs)
        {
            deleteSet.insert(loadID);
        }

        QModelIndexList deleteItems;
        std::vector<int> phyIDs;
        //载荷管理器下一定是所有载荷，按勾选id过滤管理器子节点
        for (mTreeItem* loaditem : _loadItem->getChildrenItem())
        {
            int loadId = getPhyIdByItem(loaditem);
            if (!deleteSet.contains(loadId))
            {
                continue;
            }

            QModelIndex idx = _treeModel->indexFromItem(loaditem);
            deleteItems.push_back(_treeProxyModel->mapFromSource(idx));
            phyIDs.push_back(loadId);

            //根据载荷id获取与其映射的工况id，同步删除各工况下的载荷树节点
            std::unordered_map<int, double> caseMap =
                getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, loadId }, Physical_Case);
            for (auto caseIds : caseMap)
            {
                mTreeItem* load = _treeModel->getChildrenItem(CaseLoadChildItem, loadId, caseIds.first);
                if (load == nullptr) continue;
                QModelIndex idx1 = _treeModel->indexFromItem(load);
                deleteItems.push_back(_treeProxyModel->mapFromSource(idx1));
            }

            //从管理器删除：删除数据，同时所有调用此载荷的工况下的树节点已被收集
            getCVM->getModelData()->deleteByPhysicalType(Physical_LoadBoundary, loadId);
        }

        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_LoadBoundary, phyIDs);
        //保持载荷管理器面板显示，选中管理器父节点
        selectedItem(_loadItem);
    }

    void mTreeView::slot_deleteBoundarys(const QVector<int>& boundaryIDs)
    {
        if (boundaryIDs.isEmpty())
        {
            return;
        }

        mTreeItem* _boundaryItem = _treeModel->getParentItem(BoundaryManagerParentItem);
        if (_boundaryItem == nullptr)
        {
            return;
        }

        deleteSymbol();

        QSet<int> deleteSet;
        for (int boundaryID : boundaryIDs)
        {
            deleteSet.insert(boundaryID);
        }

        QModelIndexList deleteItems;
        std::vector<int> phyIDs;
        //边界管理器下一定是所有边界，按勾选id过滤管理器子节点
        for (mTreeItem* boundaryitem : _boundaryItem->getChildrenItem())
        {
            int boundaryId = getPhyIdByItem(boundaryitem);
            if (!deleteSet.contains(boundaryId))
            {
                continue;
            }

            QModelIndex idx = _treeModel->indexFromItem(boundaryitem);
            deleteItems.push_back(_treeProxyModel->mapFromSource(idx));
            phyIDs.push_back(boundaryId);

            //根据边界id获取与其映射的工况id，同步删除各工况下的边界树节点
            std::unordered_map<int, double> caseMap =
                getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, boundaryId }, Physical_Case);
            for (auto caseIds : caseMap)
            {
                mTreeItem* boundary = _treeModel->getChildrenItem(CaseBCChildItem, boundaryId, caseIds.first);
                if (boundary == nullptr) continue;
                QModelIndex idx1 = _treeModel->indexFromItem(boundary);
                deleteItems.push_back(_treeProxyModel->mapFromSource(idx1));
            }

            //从管理器删除：删除数据，同时所有调用此边界的工况下的树节点已被收集
            getCVM->getModelData()->deleteByPhysicalType(Physical_LoadBoundary, boundaryId);
        }

        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_LoadBoundary, phyIDs);
        //保持边界管理器面板显示，选中管理器父节点
        selectedItem(_boundaryItem);
    }

    void mTreeView::slot_deletePredefinedFields(const QVector<int>& fieldIDs)
    {
        if (fieldIDs.isEmpty())
        {
            return;
        }

        mTreeItem* _fieldManagerItem = _treeModel->getParentItem(PredefinedFieldManagerParentItem);
        if (_fieldManagerItem == nullptr)
        {
            return;
        }

        deleteSymbol();

        QSet<int> deleteSet;
        for (int fieldID : fieldIDs)
        {
            deleteSet.insert(fieldID);
        }

        QModelIndexList deleteItems;
        std::vector<int> phyIDs;
        //预定义场管理器下一定是所有预定义场，按勾选id过滤管理器子节点
        for (mTreeItem* fielditem : _fieldManagerItem->getChildrenItem())
        {
            int fieldId = getPhyIdByItem(fielditem);
            if (!deleteSet.contains(fieldId))
            {
                continue;
            }

            QModelIndex idx = _treeModel->indexFromItem(fielditem);
            deleteItems.push_back(_treeProxyModel->mapFromSource(idx));
            phyIDs.push_back(fieldId);

            //根据预定义场id获取与其映射的工况id，同步删除各工况下的预定义场树节点
            std::unordered_map<int, double> caseMap =
                getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_PredefinedField, fieldId }, Physical_Case);
            for (auto caseIds : caseMap)
            {
                mTreeItem* field = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldId, caseIds.first);
                if (field == nullptr) continue;
                QModelIndex idx1 = _treeModel->indexFromItem(field);
                deleteItems.push_back(_treeProxyModel->mapFromSource(idx1));
            }

            //从管理器删除：删除数据，同时所有调用此预定义场的工况下的树节点已被收集
            getCVM->getModelData()->deleteByPhysicalType(Physical_PredefinedField, fieldId);
        }

        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PredefinedField, phyIDs);
        //保持预定义场管理器面板显示，选中管理器父节点
        selectedItem(_fieldManagerItem);
    }

    void mTreeView::slot_menuSetChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _setItem = _treeModel->getParentItem(SetParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == SetChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_Set, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //判断集合个数
        int setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetID().size();
        if (setId == 0 && _setItem != nullptr)
        {
            removeTreeItem(_setItem);
            selectedItem(_treeModel->getParentItem(ModelRootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);

            //清理删除后变空的类型分组节点
            if (_setItem != nullptr)
            {
                QVector<mTreeItem*> groupItems;
                for (mTreeItem* child : _setItem->getChildrenItem())
                {
                    if (getTreeTypeByItem(child) == SetParentItem && getPhyIdByItem(child) != 0)
                        groupItems.push_back(child);
                }
                for (mTreeItem* group : groupItems)
                {
                    removeEmptySetTypeGroupItem(group);
                }
            }

            selectedItem(_setItem);
        }

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Set, phyIDs);
    }

    void mTreeView::slot_menuCopySet()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _setItem = _treeModel->getParentItem(SetParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == SetChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Set, getPhyIdByModelIndex(idx));
                MechSetData::mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(copyId);
                if (set == nullptr)
                    continue;

                mTreeItem* groupItem = ensureSetTypeGroupItem(_setItem, set->getSetType());
                if (groupItem == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), SetChildItem, set->getSetName(), copyId,
                    Tree_Yes, slot_setTypePic(set->getSetType()));

                _treeModel->appendChildren(groupItem, childitem);

                lastItem = childitem;
                copyIDs.push_back(copyId);
            }
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Set, copyIDs } });

        //高亮且点击复制的最后一个
        if(lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updateSetTree(bool isCreate, int setID)
    {
        MechSetData::mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
        if (set == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _setItem = _treeModel->getParentItem(SetParentItem);
        auto rootItem = _treeModel->getParentItem(ModelRootItem);
        if (_setItem == nullptr)
        {
            _setItem = new mTreeItem(modelID, SetParentItem, "集合");
            _treeModel->insertChildren(rootItem, _setItem, getInsertLocation(SetParentItem));
        }

        //集合按类型挂载到对应分组节点下
        auto groupItem = ensureSetTypeGroupItem(_setItem, set->getSetType());

        auto cusItem = _treeModel->getChildrenItem(SetChildItem, setID);
        if (isCreate || cusItem == nullptr)
        {
            auto childitem = new mTreeItem(modelID, SetChildItem, set->getSetName(), setID, Tree_Yes, slot_setTypePic(set->getSetType()));
            _treeModel->appendChildren(groupItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            QString pic = slot_setTypePic(set->getSetType());
            if (getIconStringByItem(cusItem) != pic)
            {
                updatePreSetData(setID);
                setTreeItemPic(cusItem, pic);
            }
            setTreeItemText(cusItem, set->getSetName());

            //集合类型变化：迁移到新类型对应的分组节点下
            MechPreData::mTreeItem* parentItem = cusItem->getParentItem();
            if (groupItem != nullptr
                && (parentItem == nullptr
                    || getTreeTypeByItem(parentItem) != SetParentItem
                    || getPhyIdByItem(parentItem) != static_cast<int>(set->getSetType())))
            {
                auto childitem = new mTreeItem(modelID, SetChildItem, set->getSetName(), setID, Tree_Yes, pic);

                //先移除旧节点再挂载新节点：新旧节点key相同，顺序反了会破坏索引表
                disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
                _treeModel->removeTreeItem(cusItem);
                connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
                removeEmptySetTypeGroupItem(parentItem);

                _treeModel->appendChildren(groupItem, childitem);
                selectedItem(childitem);
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Set, setID);
    }

    void mTreeView::updatePreSetData(int setID)
    {
        mSetManager& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
        MechSetData::mSetBase* setData = setManager.getSetData(setID);
        if (setData == nullptr)
            return;

        SetType setType = setData->getSetType();

        QHash<MxFunctions::PhysicalFieldType, std::vector<int>> phyData;

        mMapRelationManager& mapManager = getCVM->getModelData()->getMapRelationManager();
        set<int> loadIDs = mapManager.getMapRelation<set<int>>({ Physical_Set, setID }, Physical_LoadBoundary);
        vector<int> vec_load;
        for (int id : loadIDs)
        {
            shared_ptr<mLoadBoundaryBase> load = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(id);
            if (load == nullptr)
                continue;
            bool isUse = setManager.beingUsed(setType, load->getLoadBoundaryType());
            if (!isUse)
            {
                mapManager.deleteMapRelation({ Physical_LoadBoundary, load->getLoadBoundaryId() }, { Physical_Set, setID });
                vec_load.push_back(id);
            }
        }

        if (!vec_load.empty())
            phyData[Physical_LoadBoundary] = vec_load;

        set<int> bcIDs = mapManager.getMapRelation<set<int>>({ Physical_Set, setID }, Physical_LoadBoundary);
        vector<int> vec_bc;
        for (int id : bcIDs)
        {
            shared_ptr<mLoadBoundaryBase> bc = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(id);
            if (bc == nullptr)
                continue;
            bool isUse = setManager.beingUsed(setType, bc->getLoadBoundaryType());
            if (!isUse)
            {
                mapManager.deleteMapRelation({ Physical_LoadBoundary, bc->getLoadBoundaryId() }, { Physical_Set, setID });
                vec_bc.push_back(id);
            }
        }

        if (!vec_bc.empty())
            phyData[Physical_LoadBoundary] = vec_bc;

        set<int> contactIDs = mapManager.getMapRelation<set<int>>({ Physical_Set, setID }, Physical_Contact);
        vector<int> vec_contact;
        for (int id : contactIDs)
        {
            mContactBase* contact = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getContactData(id);
            if (contact == nullptr)
                continue;
            bool isUse = setManager.beingUsed(setType, contact->getContactType());
            if (!isUse)
            {
                mapManager.deleteMapRelation({ Physical_Contact, contact->getContactId() }, { Physical_Set, setID });
                vec_contact.push_back(id);
            }
        }

        if (!vec_contact.empty())
            phyData[Physical_Contact] = vec_contact;

        getCVM->getDataStatusManager()->updateDynamicInfoData(phyData);
    }


    QString mTreeView::slot_setTypePic(MxFunctions::SetType type)
    {
        switch (type)
        {
        case MxFunctions::GeoPoint:
            return ":/mMainWidgets/Icon/choice/dingdian3x.png";
        case MxFunctions::GeoEdge:
            return ":/mMainWidgets/Icon/choice/bian3x.png";
        case MxFunctions::GeoFace:
            return ":/mMainWidgets/Icon/choice/mian3x.png";
        case MxFunctions::GeoSolid:
            return ":/mMainWidgets/Icon/choice/jiheti3x.png";
        case MxFunctions::Element:
            return ":/mMainWidgets/Icon/choice/danyuan3x.png";
        case MxFunctions::Node:
            return ":/mMainWidgets/Icon/choice/jiedian3x.png";
        case MxFunctions::Part:
            return ":/mMainWidgets/Icon/choice/jiheti3x.png";
        case MxFunctions::ElementSurface:
            return ":/mMainWidgets/Icon/choice/danyuanmian3x.png";
        case MxFunctions::ElementEdge:
            return ":/mMainWidgets/Icon/choice/danyuanbian3x.png";
        case MxFunctions::DOFNode:
            return ":/mMainWidgets/Icon/choice/dofNode@3x.png";
        case MxFunctions::FrequencyPoint:
            return ":/mMainWidgets/Icon/choice/freqPoint@3x.png";
        case MxFunctions::Panel:
            return ":/mMainWidgets/Icon/choice/panel@3x.png";
        case MxFunctions::NodeSurface:
            return ":/mMainWidgets/Icon/choice/panel@3x.png";
        default:
            break;
        }

        return QString();
    }

    QString mTreeView::getSetTypeNameByType(MxFunctions::SetType type)
    {
        switch (type)
        {
        case MxFunctions::GeoPoint:
            return "几何点集";
        case MxFunctions::GeoEdge:
            return "几何线集";
        case MxFunctions::GeoFace:
            return "几何面集";
        case MxFunctions::GeoSolid:
            return "几何实体集";
        case MxFunctions::Node:
            return "节点集";
        case MxFunctions::ElementEdge:
            return "单元边集";
        case MxFunctions::SurfaceAnalysis:
            return "解析表面集";
        case MxFunctions::ElementSurface:
            return "单元表面集";
        case MxFunctions::Element:
            return "单元集";
        case MxFunctions::Part:
            return "部件集";
        case MxFunctions::FrequencyPoint:
            return "频率点集";
        case MxFunctions::DOFNode:
            return "自由度节点集";
        case MxFunctions::Panel:
            return "面板集";
        case MxFunctions::NodeSurface:
            return "节点面集";
        default:
            return "集合";
        }
    }

    MechPreData::mTreeItem* mTreeView::ensureSetTypeGroupItem(MechPreData::mTreeItem* setRootItem, MxFunctions::SetType setType)
    {
        if (setRootItem == nullptr)
        {
            return nullptr;
        }

        //分组节点复用SetParentItem类型，phyID存SetType值与"集合"节点(phyID=0)区分
        QVector<MechPreData::mTreeItem*> siblings = setRootItem->getChildrenItem();
        for (MechPreData::mTreeItem* child : siblings)
        {
            if (getTreeTypeByItem(child) == SetParentItem && getPhyIdByItem(child) == static_cast<int>(setType))
            {
                return child;
            }
        }

        auto groupItem = new mTreeItem(getModelIdByItem(setRootItem), SetParentItem, getSetTypeNameByType(setType),
            static_cast<int>(setType), Tree_Yes, slot_setTypePic(setType));

        //按二级菜单类型顺序插入：插到第一个排在之后的分组前
        const QVector<MxFunctions::SetType>& order = getSetTypeMenuOrder();
        int myIndex = order.indexOf(setType);
        MechPreData::mTreeItem* lastItem = nullptr;
        for (MechPreData::mTreeItem* child : siblings)
        {
            if (getTreeTypeByItem(child) != SetParentItem)
                continue;
            int childPhyId = getPhyIdByItem(child);
            if (childPhyId == 0)
                continue;
            if (myIndex >= 0 && order.indexOf(static_cast<MxFunctions::SetType>(childPhyId)) > myIndex)
                break;
            lastItem = child;
        }
        _treeModel->insertChildren(setRootItem, groupItem, lastItem);
        return groupItem;
    }

    void mTreeView::removeEmptySetTypeGroupItem(MechPreData::mTreeItem* groupItem)
    {
        if (groupItem == nullptr)
        {
            return;
        }
        //仅处理类型分组节点（SetParentItem且phyID非0），不动"集合"节点本身
        if (getTreeTypeByItem(groupItem) != SetParentItem || getPhyIdByItem(groupItem) == 0)
        {
            return;
        }
        if (groupItem->getChildrenSize() == 0)
        {
            _treeModel->removeTreeItem(groupItem);
        }
    }

    void mTreeView::slot_menuPropertyDelete()
    {
        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_Property);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_Property);
        auto _propertyItem = _treeModel->getParentItem(PropertyParentItem);
        // 只删除 PropertyChildItem 类型的子节点，保留单层板/层合板/梁截面等常驻父节点
        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
        QVector<mTreeItem*> children = _propertyItem->getChildrenItem();
        for (auto* child : children)
        {
            if (getTreeTypeByItem(child) == PropertyChildItem)
            {
                _treeModel->removeTreeItem(child);
            }
        }
        connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Property, phyIDs);
        getCVM->updatePartShowColor(PartColorType::MatColor, phyIDs);
        selectedItem(_propertyItem);
    }

    void mTreeView::slot_menuPropertyChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(PropertyParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == PropertyChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_Property, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Property, phyIDs);
        getCVM->updatePartShowColor(PartColorType::MatColor, phyIDs);
        selectedItem(_propertyItem);
    }

    void mTreeView::slot_menuNonStructuralQualityDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(NonStructuralQualityParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == NonStructuralQualityChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_NonStructuralQuality, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        int nonCount = getCVM->getModelData()->getAllPhyID(Physical_NonStructuralQuality).size();
        if (nonCount == 0 && _propertyItem != nullptr)
        {
            removeTreeItem(_propertyItem);
            selectedItem(_treeModel->getParentItem(ModelRootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_NonStructuralQuality, phyIDs);
            selectedItem(_propertyItem);
        }

      
    }

    void mTreeView::slot_menuNonStructuralQualityDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_NonStructuralQuality);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_NonStructuralQuality);
        auto _nonStructQItem = _treeModel->getParentItem(NonStructuralQualityParentItem);
        removeTreeItem(_nonStructQItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_NonStructuralQuality, phyIDs);
        selectedItem(_treeModel->getParentItem(ModelRootItem));
    }

    void mTreeView::slot_menuCopyNonStructuralQuality()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _nonStructQItem = _treeModel->getParentItem(NonStructuralQualityParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == NonStructuralQualityChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_NonStructuralQuality, getPhyIdByModelIndex(idx));
                mNonStructuralQuality* nonStructurakQuality = getCVM->getModelData()->getManager<MechCaseData::mNonStructQualityManager>(Physical_NonStructuralQuality).getNonStructuralQualityData(copyId);
                if (nonStructurakQuality == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), NonStructuralQualityChildItem, nonStructurakQuality->getNonStructuralQualityName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_nonStructQItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_NonStructuralQuality, copyIDs } });

        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_menuCaseNonStructuralQualityDelete()
    {

    }

    void mTreeView::slot_menuCaseNonStructuralQualityChildDel()
    {

    }

    void mTreeView::slot_menuCopyCaseNonStructuralQuality()
    {

    }

    void mTreeView::slot_menuFieldDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _fieldItem = _treeModel->getParentItem(FieldParentItem);
        auto& fieldMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == FieldChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针并安全关闭数据表对话框（此时数据对象仍有效），
                //再真正删除数据对象，否则复位流程会经绑定模型访问已释放的行存储
                mInfoWindow::getInstance()->notifyFieldDataAboutToBeDestroyed(phyID);
                //先清理引用该场的关联关系，再删除数据对象：
                //getDeleteMapRelation 需重新校验被引用方，此时场对象必须已失效，
                //否则会留下"DiscreteField_1 离散场数据不完整"这类残留警告
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Field, phyID });
                fieldMgr.deleteField(phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        int fieldCount = getCVM->getModelData()->getAllPhyID(Physical_Field).size();
        if (fieldCount == 0 && _fieldItem != nullptr)
        {
            removeTreeItem(_fieldItem);
            selectedItem(_treeModel->getParentItem(ModelRootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_fieldItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Field, phyIDs);
    }

    void mTreeView::slot_menuFieldDelete(MxFunctions::FieldType fieldType)
    {
        //按子类型删除全部场（离散场/分析场）
        auto& fieldMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        vector<int> phyIDs = fieldMgr.getAllFieldID(fieldType);
        //先通知面板做安全收尾（-1 = 即将删除全部场），再逐个删除数据对象，
        //否则复位流程会经表格绑定模型访问已释放的行存储
        mInfoWindow::getInstance()->notifyFieldDataAboutToBeDestroyed(-1);
        for (int phyID : phyIDs)
        {
            //先清理引用该场的关联关系，再删除数据对象：
            //getDeleteMapRelation 需重新校验被引用方，此时场对象必须已失效，
            //否则会留下"DiscreteField_1 离散场数据不完整"这类残留警告
            getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Field, phyID });
            fieldMgr.deleteField(phyID);
        }

        auto _fieldItem = _treeModel->getParentItem(FieldParentItem);
        if (_fieldItem != nullptr)
        {
            int fieldCount = getCVM->getModelData()->getAllPhyID(Physical_Field).size();
            if (fieldCount == 0)
            {
                removeTreeItem(_fieldItem);
                selectedItem(_treeModel->getParentItem(ModelRootItem));
            }
            else
            {
                //移除该子类型的子节点（对应场数据已删除，取不到即为该子类型节点）
                auto childs = _fieldItem->getChildrenItem();
                for (auto child : childs)
                {
                    if (child == nullptr) continue;
                    if (fieldMgr.getFieldData(getPhyIdByItem(child)) == nullptr)
                        removeTreeItem(child);
                }
                selectedItem(_fieldItem);
            }
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Field, phyIDs);
    }

    void mTreeView::slot_menuCopyField()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _fieldItem = _treeModel->getParentItem(FieldParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        auto& fieldMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == FieldChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Field, getPhyIdByModelIndex(idx));
                auto field = fieldMgr.getFieldData(copyId);
                if (field == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), FieldChildItem, field->getFieldName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_fieldItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Field, copyIDs } });

        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_updateFieldTree(bool isCreate, int fieldId)
    {
        auto& fieldMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        auto fieldData = fieldMgr.getFieldData(fieldId);
        if (fieldData == nullptr) return;
        auto fieldParentItem = _treeModel->getParentItem(FieldParentItem);
        auto rootItem = _treeModel->getParentItem(ModelRootItem);
        if (!fieldParentItem)
        {
            //创建场父节点
            int modelDataID = _treeModel->getCurrentModelID();
            mTreeItem* children = new mTreeItem(modelDataID, FieldParentItem, "场");
            _treeModel->insertChildren(rootItem, children, getInsertLocation(FieldParentItem));
            fieldParentItem = _treeModel->getParentItem(FieldParentItem);
        }
        if (isCreate)
        {
            auto childItem = new mTreeItem(getCVM->getModelData()->getModelDataID(),
                FieldChildItem,
                fieldData->getFieldName(), fieldId);
            _treeModel->appendChildren(fieldParentItem, childItem);
            selectedItem(childItem);
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, fieldId);
    }

    void mTreeView::slot_menuCopyProperty()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(PropertyParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == PropertyChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Property, getPhyIdByModelIndex(idx));
                mPropertyBase* property = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), PropertyChildItem, property->getPropertyName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Property, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updatePropertyTree(bool isCreate, int propertyID)
    {
        mPropertyBase* property = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(propertyID);
        if (property == nullptr)
            return;

        auto _propertyItem = _treeModel->getParentItem(PropertyParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), PropertyChildItem, property->getPropertyName(), propertyID);
            _treeModel->appendChildren(_propertyItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Property, propertyID);
    }

    void mTreeView::slot_updateNonStructuralQualityTree(bool isCreate, int nonStructuralQualityID)
    {
        mNonStructuralQuality* nonStructurakQuality = getCVM->getModelData()->getManager<MechCaseData::mNonStructQualityManager>(Physical_NonStructuralQuality).getNonStructuralQualityData(nonStructuralQualityID);
        auto rootItem = _treeModel->getParentItem(ModelRootItem);
        if (nonStructurakQuality == nullptr)
            return;

        auto _nonStructQItem = _treeModel->getParentItem(NonStructuralQualityParentItem);
        if (!_nonStructQItem)
        {
            int modelDataID = _treeModel->getCurrentModelID();
            _nonStructQItem = new mTreeItem(modelDataID, NonStructuralQualityParentItem, "非结构质量");
            _treeModel->insertChildren(rootItem, _nonStructQItem, getInsertLocation(NonStructuralQualityParentItem));
        }

        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(),
                NonStructuralQualityChildItem,
                nonStructurakQuality->getNonStructuralQualityName(),
                nonStructuralQualityID);
            _treeModel->appendChildren(_nonStructQItem, childitem);
            selectedItem(childitem);

        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_NonStructuralQuality, nonStructuralQualityID);
    }

    void mTreeView::slot_menuFunctionNew()
    {
        mInfoWindow::getInstance()->createFunction(2);
    }

    void mTreeView::slot_menuFunctionDelete()
    {
        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_Function);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_Function);
        auto _functionItem = _treeModel->getParentItem(FunctionParentItem);
        removeTreeItem(_functionItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Function, phyIDs);
        selectedItem(_treeModel->getParentItem(ModelRootItem));
    }

    void mTreeView::slot_menuFunctionChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _functionItem = _treeModel->getParentItem(FunctionParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == FunctionChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_Function, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        int setId = getCVM->getModelData()->getAllPhyID(Physical_Function).size();
        if (setId == 0 && _functionItem != nullptr)
        {
            removeTreeItem(_functionItem);
            selectedItem(_treeModel->getParentItem(ModelRootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_functionItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Function, phyIDs);
    }

    void mTreeView::slot_menuCopyFunction()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _functionItem = _treeModel->getParentItem(FunctionParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == FunctionChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Function, getPhyIdByModelIndex(idx));
                mFunctionBase* func = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(copyId);
                if (func == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), FunctionChildItem, func->getFunctionName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_functionItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Function, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updateFunctionTree(bool isCreate, int functionID)
    {
        mFunctionBase* func = getCVM->getModelData()->getManager<mFunctionManager>(Physical_Function).getFunctionData(functionID);
        auto rootItem = _treeModel->getParentItem(ModelRootItem);
        if (func == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _functionItem = _treeModel->getParentItem(FunctionParentItem);
        if (_functionItem == nullptr)
        {
            _functionItem = new mTreeItem(modelID, FunctionParentItem, "函数");
            _treeModel->insertChildren(rootItem, _functionItem, getInsertLocation(FunctionParentItem));
        }

        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, FunctionChildItem, func->getFunctionName(), functionID);
            _treeModel->appendChildren(_functionItem, childitem);
            selectedItem(childitem);
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Function, functionID);
    }

    void mTreeView::slot_menuCoordinateNew()
    {
        mInfoWindow::getInstance()->createCoordinate();
    }

    void mTreeView::slot_menuCoordinateDelete()
    {
        //删除渲染
        deleteSymbol();
        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_Coordinate);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_Coordinate);
        mTreeItem* _coordinateItem = _treeModel->getParentItem(CoordinateParentItem);
        removeTreeChildrenItem(_coordinateItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Coordinate, phyIDs);
        createGlobalCoordinate();
        selectedItem(_coordinateItem);
    }

    void mTreeView::slot_menuCoordinateChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        mTreeItem* _coordinateItem = _treeModel->getParentItem(CoordinateParentItem);
        //删除渲染
        deleteSymbol();
        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == CoordinateChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_Coordinate, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Coordinate, phyIDs);
        selectedItem(_coordinateItem);
    }

    void mTreeView::slot_menuCopyCoordinate()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _coordinateItem = _treeModel->getParentItem(CoordinateParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == CoordinateChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Coordinate, getPhyIdByModelIndex(idx));
                mCoordinateBase* coord = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(copyId);
                if (coord == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CoordinateChildItem, coord->getCoordinateName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }
        _treeModel->appendChildren(_coordinateItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Coordinate, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updateCoordinateTree(bool isCreate, int coordinateID)
    {
        mCoordinateBase* coor = getCVM->getModelData()->getManager<mCoordinateManager>(Physical_Coordinate).getCoordinateData(coordinateID);
        if (coor == nullptr)
            return;

        mTreeItem* _coordinateItem = _treeModel->getParentItem(CoordinateParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CoordinateChildItem, coor->getCoordinateName(), coordinateID);
            _treeModel->appendChildren(_coordinateItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            //for (int i = 0; i < _coordinateItem->childCount(); i++)
            //{
            //	QTreeWidgetItem* item = _coordinateItem->child(i);
            //	mTreeItem* cusItem = dynamic_cast<mTreeItem*>(item);
            //	if (cusItem != nullptr && cusItem->getPhysicalID() == coordinateID)
            //	{
            //		//cusItem->setTypePic(slot_setTypePic(coor->get()));
            //	}
            //}
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Coordinate, coordinateID);
    }

    void mTreeView::slot_menuStructOptionDelete()
    {
        if (getCVM->getModelData()->getStructOptionBase() == nullptr)
            return;

        vector<int> designIDs = getCVM->getModelData()->getAllPhyID(Physical_OptimationDesign);
        vector<int> constraintIDs = getCVM->getModelData()->getAllPhyID(Physical_OptimationConstraint);

        getCVM->getModelData()->deleteStructOption();
        removeTreeItem(_treeModel->getParentItem(OptimizationParentItem));
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_OptimationGoal, { getCVM->getModelData()->getModelDataID() });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_OptimationDesign, designIDs);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_OptimationConstraint, constraintIDs);
        selectedItem(_treeModel->getParentItem(ModelRootItem));
    }
    void mTreeView::slot_menuOptimizationDesignNew()
    {
        mInfoWindow::getInstance()->createOptimizationDesign();
    }
    void mTreeView::slot_menuOptimizationDesignDelete()
    {
        //删除数据
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        if (!structOption)
        {
            return;
        }
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_OptimationDesign);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_OptimationDesign);
        auto _propertyItem = _treeModel->getParentItem(OptimizationDesignParentItem);
        removeTreeChildrenItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_OptimationDesign, phyIDs);
        selectedItem(_propertyItem);
    }
    void mTreeView::slot_menuOptimizationDesignChildDel()
    {
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        if (!structOption)
        {
            return;
        }
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(OptimizationDesignParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == OptimizationDesignChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_OptimationDesign, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_OptimationDesign, phyIDs);
        selectedItem(_propertyItem);
    }
    void mTreeView::slot_menuCopyOptimizationDesign()
    {
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        if (!structOption)
        {
            return;
        }
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(OptimizationDesignParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == OptimizationDesignChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_OptimationDesign, getPhyIdByModelIndex(idx));
                auto property = structOption->getOptimizationDesignData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), OptimizationDesignChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_OptimationDesign, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }
    void mTreeView::slot_updateOptimizationDesignTree(bool isCreate, int ID)
    {
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        auto data = structOption->getOptimizationDesignData(ID);
        if (data == nullptr)
            return;

        mTreeItem* _coordinateItem = _treeModel->getParentItem(OptimizationDesignParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), OptimizationDesignChildItem, data->getName(), ID);
            _treeModel->appendChildren(_coordinateItem, childitem);
            selectedItem(childitem);
        }
        else
        {
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, ID);
    }

    void mTreeView::slot_menuOptimizationConstraintNew()
    {
        mInfoWindow::getInstance()->createOptimizationConstraint();
    }
    void mTreeView::slot_menuOptimizationConstraintDelete()
    {
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        if (!structOption)
        {
            return;
        }
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_OptimationConstraint);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_OptimationConstraint);
        auto _propertyItem = _treeModel->getParentItem(OptimizationConstraintsParentItem);
        removeTreeChildrenItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_OptimationConstraint, phyIDs);
        selectedItem(_propertyItem);
    }
    void mTreeView::slot_menuOptimizationConstraintChildDel()
    {
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        if (!structOption)
        {
            return;
        }
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(OptimizationConstraintsParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == OptimizationConstraintsChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_OptimationConstraint, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_OptimationConstraint, phyIDs);
        selectedItem(_propertyItem);
    }
    void mTreeView::slot_menuCopyOptimizationConstraint()
    {
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        if (!structOption)
        {
            return;
        }
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(OptimizationConstraintsParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == OptimizationConstraintsChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_OptimationConstraint, getPhyIdByModelIndex(idx));
                auto property = structOption->getOptimizationConstraintData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), OptimizationConstraintsChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_OptimationConstraint, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }
    void mTreeView::slot_updateOptimizationConstraintTree(bool isCreate, int ID)
    {
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        auto data = structOption->getOptimizationConstraintData(ID);
        if (data == nullptr)
            return;

        mTreeItem* _coordinateItem = _treeModel->getParentItem(OptimizationConstraintsParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), OptimizationConstraintsChildItem, data->getName(), ID);
            _treeModel->appendChildren(_coordinateItem, childitem);
            selectedItem(childitem);
        }
        else
        {
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationConstraint, ID);
    }

    void mTreeView::slot_updateRunnerOptimizationTree(bool isCreate)
    {
        int modelID = getCVM->getModelData()->getModelDataID();
        //树上查找连接父节点
        auto _optItem = _treeModel->getParentItem(RunnerOptimizationParentItem);
        auto rootItem = _treeModel->getParentItem(ModelRootItem);
        if (_optItem == nullptr)
        {
            mTreeItem* parentItem = new mTreeItem(modelID, RunnerOptimizationParentItem, "流道优化");
            _treeModel->insertChildren(rootItem, parentItem, BoundaryManagerParentItem);

            QVector<mTreeItem*> allChildItem;
            //设计域
            mTreeItem* item = new mTreeItem(modelID, RunnerOptimizationDesignParentItem, "设计域");
            allChildItem.push_back(item);
            //优化目标
            item = new mTreeItem(modelID, RunnerOptimizationGoalParentItem, "优化目标");
            allChildItem.push_back(item);
            //优化约束
            item = new mTreeItem(modelID, RunnerOptimizationConstraintsParentItem, "优化约束");
            allChildItem.push_back(item);

            _treeModel->appendChildren(parentItem, std::move(allChildItem));

            selectedItem(parentItem);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_RunnerOptimationGoal, modelID);
        }
        else
        {


        }

    }

    void mTreeView::slot_menuRunnerStructOptionDelete()
    {
        if (getCVM->getModelData()->getRunnerStructOption() == nullptr)
            return;

        vector<int> designIDs = getCVM->getModelData()->getAllPhyID(Physical_RunnerOptimationDesign);
        vector<int> constraintIDs = getCVM->getModelData()->getAllPhyID(Physical_RunnerOptimationConstraint);

        getCVM->getModelData()->deleteRunnerStructOption();
        removeTreeItem(_treeModel->getParentItem(RunnerOptimizationParentItem));
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RunnerOptimationGoal, { getCVM->getModelData()->getModelDataID() });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RunnerOptimationDesign, designIDs);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RunnerOptimationConstraint, constraintIDs);
        selectedItem(_treeModel->getParentItem(ModelRootItem));

    }

    void mTreeView::slot_menuRunnerOptimizationDesignNew()
    {
        mInfoWindow::getInstance()->createRunnerOptimizationDesign();
    }
    void mTreeView::slot_menuRunnerOptimizationDesignDelete()
    {
        auto structOption = getCVM->getModelData()->getRunnerStructOption();
        if (!structOption)
        {
            return;
        }
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_RunnerOptimationDesign);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_RunnerOptimationDesign);
        auto _propertyItem = _treeModel->getParentItem(RunnerOptimizationDesignParentItem);
        removeTreeChildrenItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RunnerOptimationDesign, phyIDs);
        selectedItem(_propertyItem);
    }
    void mTreeView::slot_menuRunnerOptimizationDesignChildDel()
    {
        auto structOption = getCVM->getModelData()->getRunnerStructOption();
        if (!structOption)
        {
            return;
        }
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(RunnerOptimizationDesignParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == RunnerOptimizationDesignChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_RunnerOptimationDesign, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RunnerOptimationDesign, phyIDs);
        selectedItem(_propertyItem);
    }
    void mTreeView::slot_menuCopyRunnerOptimizationDesign()
    {
        auto structOption = getCVM->getModelData()->getRunnerStructOption();
        if (!structOption)
        {
            return;
        }
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(RunnerOptimizationDesignParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == RunnerOptimizationDesignChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_RunnerOptimationDesign, getPhyIdByModelIndex(idx));
                auto property = structOption->getOptimizationDesignData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), RunnerOptimizationDesignChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_RunnerOptimationDesign, copyIDs } });

        if (lastItem)  selectedItem(lastItem);
    }
    void mTreeView::slot_updateRunnerOptimizationDesignTree(bool isCreate, int ID)
    {
        auto structOption = getCVM->getModelData()->getRunnerStructOption();
        auto data = structOption->getOptimizationDesignData(ID);
        if (data == nullptr)
            return;

        mTreeItem* _coordinateItem = _treeModel->getParentItem(RunnerOptimizationDesignParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), RunnerOptimizationDesignChildItem, data->getName(), ID);
            _treeModel->appendChildren(_coordinateItem, childitem);
            selectedItem(childitem);
        }
        else
        {
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_RunnerOptimationDesign, ID);
    }

    void mTreeView::slot_menuRunnerOptimizationConstraintNew()
    {
        mInfoWindow::getInstance()->createRunnerOptimizationConstraint();
    }
    void mTreeView::slot_menuRunnerOptimizationConstraintDelete()
    {
        auto structOption = getCVM->getModelData()->getRunnerStructOption();
        if (!structOption)
        {
            return;
        }
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_RunnerOptimationConstraint);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_RunnerOptimationConstraint);
        auto _propertyItem = _treeModel->getParentItem(RunnerOptimizationConstraintsParentItem);
        removeTreeChildrenItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RunnerOptimationConstraint, phyIDs);
        selectedItem(_propertyItem);
    }
    void mTreeView::slot_menuRunnerOptimizationConstraintChildDel()
    {
        auto structOption = getCVM->getModelData()->getRunnerStructOption();
        if (!structOption)
        {
            return;
        }
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(RunnerOptimizationConstraintsParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == RunnerOptimizationConstraintsChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_RunnerOptimationConstraint, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RunnerOptimationConstraint, phyIDs);
        selectedItem(_propertyItem);
    }
    void mTreeView::slot_menuCopyRunnerOptimizationConstraint()
    {
        auto structOption = getCVM->getModelData()->getRunnerStructOption();
        if (!structOption)
        {
            return;
        }
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(RunnerOptimizationConstraintsParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == RunnerOptimizationConstraintsChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_RunnerOptimationConstraint, getPhyIdByModelIndex(idx));
                auto property = structOption->getOptimizationConstraintData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), RunnerOptimizationConstraintsChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_RunnerOptimationConstraint, copyIDs } });

        if (lastItem)  selectedItem(lastItem);
    }
    void mTreeView::slot_updateRunnerOptimizationConstraintTree(bool isCreate, int ID)
    {
        auto structOption = getCVM->getModelData()->getRunnerStructOption();
        auto data = structOption->getOptimizationConstraintData(ID);
        if (data == nullptr)
            return;

        mTreeItem* _coordinateItem = _treeModel->getParentItem(RunnerOptimizationConstraintsParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), RunnerOptimizationConstraintsChildItem, data->getName(), ID);
            _treeModel->appendChildren(_coordinateItem, childitem);
            selectedItem(childitem);
        }
        else
        {
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_RunnerOptimationConstraint, ID);
    }

    void mTreeView::slot_menuGlobalParamDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_GenericParam);
        globalParam->deleteSolveControlData();
        getCVM->getModelData()->deleteGlobalParam();
        auto _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);
        removeTreeChildrenItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GenericParam, phyIDs);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GenericAssign, { 0 });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GenericMatrix, { 0 });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModalContri, { 0 });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_NodeContri, { 0 });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_TPContri, { 0 });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PanelContri, { 0 });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ERP, { 0 });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ASCouplingSearch, { 0 });
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_SolveControl, { 0 });

    }

    void mTreeView::slot_updateGenericParamTree(bool isCreate, int ID)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        auto data = globalParam->getGenericParamData(ID);
        if (data == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        auto _genericParamItem = _treeModel->getParentItem(GenericParamParentItem);
        if (_genericParamItem == nullptr)
        {
            _genericParamItem = new mTreeItem(modelID, GenericParamParentItem, "超单元缩聚方法");
            _treeModel->appendChildren(_globalItem, _genericParamItem);
        }

        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, GenericParamChildItem, data->getName(), ID);
            _treeModel->appendChildren(_genericParamItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_GenericParam, ID);
    }

    void mTreeView::slot_menuGenericParamDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_GenericParam);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_GenericParam);
        auto _panrentItem = _treeModel->getParentItem(GenericParamParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GenericParam, phyIDs);

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);
        selectedItem(_panrentItem);
    }

    void mTreeView::slot_menuGenericParamChildDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _panrentItem = _treeModel->getParentItem(GenericParamParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == GenericParamChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_GenericParam, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GenericParam, phyIDs);

        if (_panrentItem->getChildrenSize() == 0)
        {
            removeTreeItem(_panrentItem);
            _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);
        }

        selectedItem(_panrentItem);
    }

    void mTreeView::slot_menuGenericParamChildCopy()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _panrentItem = _treeModel->getParentItem(GenericParamParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == GenericParamChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_GenericParam, getPhyIdByModelIndex(idx));
                auto data = globalParam->getGenericParamData(copyId);
                if (data == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), GenericParamChildItem, data->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_panrentItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_GenericParam, copyIDs } });

        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updateGenericAssignTree(bool isCreate)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        auto data = globalParam->getGenericAssignData();
        if (data == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, GenericAssignParentItem, "超单元调用");
            _treeModel->appendChildren(_globalItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_GenericAssign, 0);
    }

    void mTreeView::slot_updateASCouplingSearchTree(bool isCreate)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        auto data = globalParam->getASCouplingSearchData();
        if (data == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, ASCouplingSearchParentItem, "声固耦合对搜索");
            _treeModel->appendChildren(_globalItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ASCouplingSearch, 0);
    }

    void mTreeView::slot_menuASCouplingSearchDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();

        getCVM->getModelData()->deleteByPhysicalType(Physical_ASCouplingSearch, 0);
        auto _panrentItem = _treeModel->getParentItem(ASCouplingSearchParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ASCouplingSearch, {0});

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);
        
        selectedItem(_panrentItem);
    }

    void mTreeView::slot_menuGenericAssignDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();

        getCVM->getModelData()->deleteByPhysicalType(Physical_GenericAssign, 0);
        auto _panrentItem = _treeModel->getParentItem(GenericAssignParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GenericAssign, {0});

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);

        selectedItem(_panrentItem);
    }

    void mTreeView::slot_updateGenericMatrixTree(bool isCreate)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        auto data = globalParam->getGenericMatrixData();
        if (data == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, GenericMatrixParentItem, "超单元矩阵");
            _treeModel->appendChildren(_globalItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_GenericMatrix, 0);
    }

    void mTreeView::slot_menuGenericMatrixDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();

        getCVM->getModelData()->deleteByPhysicalType(Physical_GenericMatrix, 0);
        auto _panrentItem = _treeModel->getParentItem(GenericMatrixParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GenericMatrix, { 0 });

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);
        
        selectedItem(_panrentItem);
    }

    void mTreeView::slot_menuERPDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();

        getCVM->getModelData()->deleteByPhysicalType(Physical_ERP, 0);
        auto _panrentItem = _treeModel->getParentItem(ERPParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ERP, { 0 });

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);

        selectedItem(_panrentItem);
    }

    void mTreeView::slot_updatePeakFilterTree(bool isCreate, int ID)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        auto data = globalParam->getPeakFilterData(ID);
        if (data == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        auto _peakFilterItem = _treeModel->getParentItem(PeakFilterParentItem);
        if (_peakFilterItem == nullptr)
        {
            _peakFilterItem = new mTreeItem(modelID, PeakFilterParentItem, "峰值筛选");
            _treeModel->appendChildren(_globalItem, _peakFilterItem);
        }

        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, PeakFilterChildItem, data->getName(), ID);
            _treeModel->appendChildren(_peakFilterItem, childitem);
            selectedItem(childitem);
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PeakFilter, ID);
    }

    void mTreeView::slot_menuPeakFilterParentDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_PeakFilter);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_PeakFilter);
        globalParam->deleteAllPeakFilterData();
        auto _panrentItem = _treeModel->getParentItem(PeakFilterParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PeakFilter, phyIDs);

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);

        selectedItem(_panrentItem);
    }

    void mTreeView::slot_menuPeakFilterChildDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _panrentItem = _treeModel->getParentItem(PeakFilterParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == PeakFilterChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_PeakFilter, phyID);
                globalParam->deletePeakFilterData(phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PeakFilter, phyIDs);

        if (_panrentItem->getChildrenSize() == 0)
        {
            removeTreeItem(_panrentItem);
            _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);
        }

        selectedItem(_panrentItem);
    }

    void mTreeView::slot_solveMotionScenario()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ScenarioParentItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                mInfoWindow::getInstance()->solveMotionScenario(phyID);
                break;
            }
        }
    }

    void mTreeView::slot_menuPeakFilterChildCopy()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _panrentItem = _treeModel->getParentItem(PeakFilterParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == PeakFilterChildItem)
            {
                copyId = globalParam->copyPeakFilterData(getPhyIdByModelIndex(idx));
                auto data = globalParam->getPeakFilterData(copyId);
                if (data == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), PeakFilterChildItem, data->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_panrentItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_PeakFilter, copyIDs } });

        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updateGenericPartTree(bool isCreate)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        auto data = globalParam->getGenericPartData();
        if (data == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, GenericPartParentItem, "超单元部件");
            _treeModel->appendChildren(_globalItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
       // getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_GenericPart, 0);
    }

    void mTreeView::slot_menuGenericPartDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();

        getCVM->getModelData()->deleteByPhysicalType(Physical_GenericPart, 0);
        auto _panrentItem = _treeModel->getParentItem(GenericPartParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GenericPart, { 0 });

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);

        selectedItem(_panrentItem);
    }

    void mTreeView::slot_updateGlobalOutputControlTree(bool isCreate)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        auto data = globalParam->getGlobalOutputControlData();
        if (data == nullptr)
            return;
    
        int modelID = getCVM->getModelData()->getModelDataID();
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, GlobalOutputControlParentItem, "全局输出控制");
            _treeModel->appendChildren(_globalItem, childitem);
            selectedItem(childitem);
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_GlobalOutputControl, 0);
    }
    
    void mTreeView::slot_menuGlobalOutputControlDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();

        globalParam->deleteGlobalOutputControlData();
        getCVM->getModelData()->deleteByPhysicalType(Physical_GlobalOutputControl, 0);
        auto _panrentItem = _treeModel->getParentItem(GlobalOutputControlParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_GlobalOutputControl, { 0 });

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);

        selectedItem(_panrentItem);
    }

    void mTreeView::slot_updateSolveControlTree(bool isCreate)
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        auto data = globalParam->getSolveControlData();
        if (data == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, SolveControlParentItem, "求解控制");
            _treeModel->appendChildren(_globalItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_SolveControl, 0);
    }

    void mTreeView::slot_menuSolveControlDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();

        globalParam->deleteSolveControlData();
        getCVM->getModelData()->deleteByPhysicalType(Physical_SolveControl, 0);
        auto _panrentItem = _treeModel->getParentItem(SolveControlParentItem);
        removeTreeItem(_panrentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_SolveControl, { 0 });

        _panrentItem = _treeModel->getParentItem(GlobalParamParentItem);

        selectedItem(_panrentItem);
    }

    void mTreeView::slot_updateGlobalDimTree(bool isCreate)
    {
        mTreeItem* _globalDimItem = _treeModel->getParentItem(MeshParentItem);
        bool isCreatedGlobalDim = false;
        if (isCreate)
        {
            //判断是否创建过全局尺寸
          
            for (auto& item : _globalDimItem->getChildrenItem())
            {
                if (getTextByItem(item) == "全局尺寸")
                    isCreatedGlobalDim = true;
            }

            if (isCreatedGlobalDim)   // 已存在
                return;                                  // 直接退出
            else
            {
                auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), MeshChildItem, "全局尺寸", -1);
                _treeModel->appendChildren(_globalDimItem, childitem);
                int id = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getIDByMeshPanelName("全局尺寸");
                getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id)->setStaticsPic(Tree_Yes);
                selectedItem(childitem);
            }
        }
        else
        {

        }
    }
    void mTreeView::slot_updateLocalSizeTree(bool isCreate, int meshpanelID)
    {
        mMeshPanelBase* meshPanel = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshpanelID);
        if (meshPanel == nullptr)
            return;
        mTreeItem* _localSizeItem = _treeModel->getParentItem(MeshParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), MeshChildItem, meshPanel->getMeshPanelName(), meshpanelID);
            _treeModel->appendChildren(_localSizeItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
    }
    void mTreeView::slot_updateMeshTree(bool isCreate, int meshpanelID)
    {
        mMeshPanelBase* meshPanel = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshpanelID);
        if (meshPanel == nullptr)
            return;

        mTreeItem* _meshPanelItem = _treeModel->getParentItem(MeshParentItem);
        if (isCreate)
        {
            BrowserTreeStatus status = Tree_Yes;
            if (meshPanel->getMeshPanelType() != CustomEncryption)
            {
                status = Tree_Failure;
                setTreeItemStatus(_meshPanelItem, status);
            }
            else
            {
                getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshpanelID)->setStaticsPic(status);
            }

            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), MeshChildItem, meshPanel->getMeshPanelName(), meshpanelID, status);
            _treeModel->appendChildren(_meshPanelItem, childitem);
            selectedItem(childitem);
        }
        else
        {

        }
    }

    void mTreeView::slot_updateMeshOptTree(bool isCreate, int meshpanelID)
    {
        mMeshOptPanelBase* meshOptPanel = getCVM->getModelData()->getManager<MeshPanelData::mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshpanelID);
        if (meshOptPanel == nullptr)
            return;

        mTreeItem* _meshPanelItem = _treeModel->getParentItem(MeshOptParentItem);
        auto rootItem = _treeModel->getParentItem(ModelRootItem);
        if (_meshPanelItem == nullptr)
        {
            _meshPanelItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), MeshOptParentItem, "网格操作");
            _treeModel->insertChildren(rootItem, _meshPanelItem, getInsertLocation(MeshOptParentItem));
        }
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), MeshOptChildItem, meshOptPanel->getMeshPanelName(), meshpanelID, Tree_Yes);
            getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshpanelID)->setStaticsPic(Tree_Yes);
            _treeModel->appendChildren(_meshPanelItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            //auto childitem = _treeModel->getChildrenItem(MeshOptChildItem, meshpanelID, getCVM->getModelData()->getModelDataID());
            //selectedItem(childitem);
        }
    }

    void mTreeView::slot_updateMeshOperationTree(bool isCreate, int meshPanelID)
    {

    }

    void mTreeView::slot_menuMeshPanelChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        mTreeItem* _meshItem = _treeModel->getParentItem(MeshParentItem);
        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == MeshChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MeshPanel, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MeshPanel, phyIDs);
        selectedItem(_meshItem);

        emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
    }

    void mTreeView::slot_menuCustomPanelChildDel()
    {
        slot_menuBoxDelete();
        slot_menuMeshPanelChildDel();
    }

    void mTreeView::slot_menuMeshOptPanelChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        mTreeItem* _meshOptItem = _treeModel->getParentItem(MeshOptParentItem);
        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == MeshOptChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MeshOptPanel, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        //判断集合个数
        int meshOptId = getCVM->getModelData()->getAllPhyID(Physical_MeshOptPanel).size();
        if (meshOptId == 0 && _meshOptItem != nullptr)
        {
            removeTreeItem(_meshOptItem);
            selectedItem(_treeModel->getParentItem(ModelRootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_meshOptItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MeshOptPanel, phyIDs);

    }

    void mTreeView::slot_menuMeshPanelDelete()
    {
        //删除数据
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MeshPanel);
        mTreeItem* _meshItem = _treeModel->getParentItem(MeshParentItem);
        removeTreeChildrenItem(_meshItem);
        selectedItem(_meshItem);
    }
    void mTreeView::slot_menuMeshOptPanelDelete()
    {
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MeshOptPanel);
        mTreeItem* _meshOptItem = _treeModel->getParentItem(MeshOptParentItem);
        removeTreeItem(_meshOptItem);
        selectedItem(_treeModel->getParentItem(ModelRootItem));
    }
    void mTreeView::slot_createMeshByPartId(vector<int> partIds)
    {
        mInfoWindow::getInstance()->createMeshByPartId(partIds);

    }
    void mTreeView::slot_createMesh()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        mInfoWindow::getInstance()->createMesh(getPhyIdByModelIndex(idxs[0]));
    }
    void mTreeView::slot_createMeshAuto()
    {
        mInfoWindow::getInstance()->createMeshAuto();

    }
    void mTreeView::slot_deleteMeshAuto()
    {
        mInfoWindow::getInstance()->deleteMeshAuto();
    }
    void mTreeView::slot_createMeshBySetting()
    {
        mInfoWindow::getInstance()->createMeshBySetting();
    }
    void mTreeView::slot_createMesh1()
    {
        mInfoWindow::getInstance()->createMesh1();

    }
    void mTreeView::slot_deleteGlobalMesh()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        mTreeItem* _meshItem = _treeModel->getParentItem(MeshParentItem);
        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == MeshChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MeshPanel, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        //判断集合个数
        int meshId = getCVM->getModelData()->getAllPhyID(Physical_MeshPanel).size();
        if (meshId == 0 && _meshItem != nullptr)
        {
            removeTreeItem(_meshItem);
            selectedItem(_treeModel->getParentItem(ModelRootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_meshItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MeshPanel, phyIDs);
        getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel)._globalSize = 10e10;
    }
    void mTreeView::slot_createMeshAutoAdapt()
    {
        mInfoWindow::getInstance()->createMeshAutoAdapt();

    }
    void mTreeView::slot_createMeshCustomEncryption()
    {
        mInfoWindow::getInstance()->createMeshCustomEncryption();
    }
    void mTreeView::slot_createMeshLocalSize()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        mInfoWindow::getInstance()->createMeshLocalSize(getPhyIdByModelIndex(idxs[0]));
    }
    void mTreeView::slot_createMeshEdgeSuppress(bool isSuppress)
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        mInfoWindow::getInstance()->createMeshEdgeSuppress(getPhyIdByModelIndex(idxs[0]), isSuppress);
    }

    void mTreeView::slot_updateSectionShapeTree(bool isCreate, int sectionshapeID)
    {
        mSectionShapeBase* sec = getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(sectionshapeID);

        if (sec == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        mTreeItem* _sectionShapeItem = _treeModel->getParentItem(SectionShapeParentItem);
        if (_sectionShapeItem == nullptr)
        {
            _sectionShapeItem = new mTreeItem(modelID, SectionShapeParentItem, "梁截面");
            _treeModel->insertChildren(_treeModel->getParentItem(PropertyParentItem), _sectionShapeItem, getInsertLocation(SectionShapeParentItem));
        }

        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, SectionShapeChildItem, sec->getSectionShapeName(), sectionshapeID);
            _treeModel->appendChildren(_sectionShapeItem, childitem);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PropertyShape, sectionshapeID);
    }

    void mTreeView::slot_createSectionShape()
    {
    }

    void mTreeView::slot_menuSectionShapeDelete()
    {
        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_PropertyShape);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_PropertyShape);
        mTreeItem* _secItem = _treeModel->getParentItem(SectionShapeParentItem);
        removeTreeItem(_secItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PropertyShape, phyIDs);
        selectedItem(_treeModel->getParentItem(ModelRootItem));
        emit MechCommon::mMechGlobalSignals::getInstance()->clearGeoWidget();
    }
    void mTreeView::slot_menuSectionShapeChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _secItem = _treeModel->getParentItem(SectionShapeParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == SectionShapeChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_PropertyShape, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        removeProxyIndexs(deleteItems);
        selectedItem(_secItem);

        vector<int> IDs = getCVM->getModelData()->getAllPhyID(Physical_PropertyShape);
        if (IDs.size() == 0) //无梁截面 删除父节点
        {
            QModelIndex idx = _treeModel->indexFromItem(_treeModel->getParentItem(SectionShapeParentItem));
            QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
            removeProxyIndexs({ proxyIndex });
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PropertyShape, phyIDs);
        emit MechCommon::mMechGlobalSignals::getInstance()->clearGeoWidget();
    }
    void mTreeView::slot_menuCopySectionShape()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _secItem = _treeModel->getParentItem(SectionShapeParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == SectionShapeChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_PropertyShape, getPhyIdByModelIndex(idx));
                MechSectionShapeData::mSectionShapeBase* mat = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(copyId);
                if (mat == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), SectionShapeChildItem, mat->getSectionShapeName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_secItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_PropertyShape, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updateCaseTree(bool isCreate, int caseID)
    {
        if (getCVM->isPostModel()) return;
        mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
        if (caseBase == nullptr)
            return;
        auto caseType = caseBase->getCaseType();
        BrowserTreeType bt = CaseImplicitLinearStaticParentItem;
        bt = BrowserTreeType(int(bt) + caseType - 1);
        QString name = caseBase->getCaseName();
        QString iconPath = getCaseIconPath(caseType);

        int modelID = getCVM->getModelData()->getModelDataID();
        mTreeItem* parentItem = new mTreeItem(modelID, bt, name, caseID, Tree_Yes, iconPath);
        auto rootItem = _treeModel->getParentItem(ModelRootItem);
        _treeModel->insertChildren(rootItem, parentItem, getInsertLocation(CaseParentItem));

        // 统一创建工况下一级菜单节点（已内部处理各工况的显示规则，并包含结果节点）
        createCaseFirstLevelItems(modelID, caseType, caseID, parentItem);

        selectedItem(_treeModel->getParentItem(CaseSettingChildItem, caseID));
    }

    void mTreeView::slot_updateRotorParentItem(int caseID)
    {
        if (getCVM->isPostModel()) return;
        mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
        if (caseBase == nullptr || caseBase->getCaseType() != ImplicitModal)
            return;

        mCaseImplicitModal* modalData = dynamic_cast<mCaseImplicitModal*>(caseBase);
        bool showRotational = modalData && (modalData->_modalType == "复模态分析") && modalData->_rotationalChecked;

        int modelID = getCVM->getModelData()->getModelDataID();
        BrowserTreeType bt = BrowserTreeType(int(CaseImplicitLinearStaticParentItem) + ImplicitModal - 1);

        mTreeItem* parentItem = _treeModel->getParentItem(bt, caseID);
        if (parentItem == nullptr)
            return;

        mTreeItem* rotateItem = _treeModel->getParentItem(CaseRotateParentItem, caseID);

        if (showRotational)
        {
            if (rotateItem != nullptr)
                return;

            rotateItem = new mTreeItem(modelID, CaseRotateParentItem, "转子分析", caseID);
            _treeModel->appendChildren(parentItem, rotateItem);

            std::set<int> rotateIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>(
                { Physical_Case, caseID }, Physical_RotateAnalyse);
            if (!rotateIDs.empty())
            {
                QVector<mTreeItem*> rotateChildItems;
                rotateChildItems.reserve(rotateIDs.size());
                for (auto rotateID : rotateIDs)
                {
                    QString phyName = getCVM->getModelData()->getNameByPhyID(Physical_RotateAnalyse, rotateID);
                    QVector<QPair<QString, QString>> info;
                    int status = getCVM->getModelData()->getDataStatus(info, Physical_RotateAnalyse, rotateID);
                    auto childitem = new mTreeItem(modelID, CaseRotateChildItem, phyName, rotateID, (BrowserTreeStatus)status);
                    childitem->setCaseID(caseID);
                    rotateChildItems.push_back(childitem);
                }
                _treeModel->appendChildren(rotateItem, std::move(rotateChildItems));
            }
        }
        else
        {
            if (rotateItem == nullptr)
                return;
            _treeModel->removeTreeItem(rotateItem);
        }
    }

    void mTreeView::slot_updateLoadOnCaseTree(bool isCreate, int loadID)
    {
        //定位分析工况节点
        QModelIndexList idxs = getSelectIndex();
        QModelIndex porxyIndex;
        for (auto& idx : idxs)
        {
            auto seltype = getTreeTypeByModelIndex(idx);
            auto parenttype = getTreeTypeByModelIndex(idx.parent());
            if(isCaseParentItem(idx))
            {
                porxyIndex = idx;
                break;
            }
            //分析工况下的节点
            else if (seltype == CaseLoadParentItem || seltype == CaseBCParentItem || seltype == CaseRotateParentItem || seltype == CaseSettingChildItem || seltype == ResultParentItem)
            {
                porxyIndex = idx.parent();
                break;
            }
            //载荷节点
            else if ((seltype == CaseLoadChildItem || seltype == CaseBCChildItem || seltype == CaseRotateChildItem) && (parenttype != LoadManagerParentItem && parenttype != BoundaryManagerParentItem))
            {
                porxyIndex = idx.parent().parent();
                break;
            }
        }

        //挂载到载荷管理器中
        mTreeItem* _loadManagerItem = _treeModel->getParentItem(LoadManagerParentItem);
        if (isCreate && _loadManagerItem != nullptr)
        {
            auto loadData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadID);
            if (loadData == nullptr)
            {
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, loadID);
                return;
            }
            auto loadType = loadData->getLoadBoundaryType();
            mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseLoadChildItem, loadData->getLoadBoundaryName(), loadID);
            _treeModel->appendChildren(_loadManagerItem, childitem);

            if (porxyIndex.isValid())
            {
                int caseID = getPhyIdByModelIndex(porxyIndex);
                //找到分析节点下的载荷父节点
                mTreeItem* _loadItem = _treeModel->getParentItem(CaseLoadParentItem, caseID);
                //若工况下不存在载荷父节点（该工况类型不支持载荷），不再挂载到工况
                //TODO:声学载荷不再挂载在分析节点下的载荷父节点
                if (_loadItem != nullptr && loadType != LOAD_IM_ACOCONCFORCE)
                {
                    //载荷数据挂载
                    auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseLoadChildItem, loadData->getLoadBoundaryName(), loadID);
                    childitem->setCaseID(caseID);
                    _treeModel->appendChildren(_loadItem, childitem);
                    getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary , loadID }, { Physical_Case ,caseID });
                }
            }

            if (childitem) selectedItem(childitem);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, loadID);
    }


    void mTreeView::slot_updateRotateAnalyseOnCaseTree(bool isCreate, int rotateAnalyseID)
    {
        //Locate case parent node
        QModelIndexList idxs = getSelectIndex();
        QModelIndex porxyIndex;
        for (auto& idx : idxs)
        {
            auto seltype = getTreeTypeByModelIndex(idx);
            auto parenttype = getTreeTypeByModelIndex(idx.parent());
            if (isCaseParentItem(idx))
            {
                porxyIndex = idx;
                break;
            }
            //child node under case
            else if (seltype == CaseRotateParentItem || seltype == CaseSettingChildItem || seltype == ResultParentItem)
            {
                porxyIndex = idx.parent();
                break;
            }
            //rotate analysis child
            else if (seltype == CaseRotateChildItem && (parenttype != LoadManagerParentItem && parenttype != BoundaryManagerParentItem))
            {
                porxyIndex = idx.parent().parent();
                break;
            }
        }

        if (isCreate && porxyIndex.isValid())
        {
            int caseID = getPhyIdByModelIndex(porxyIndex);
            //FIX:为什么这里的rotateData是空的
            auto rotateData = getCVM->getModelData()->getManager<MechCaseData::mRotateAnalyseManager>(Physical_RotateAnalyse).getRotateAnalyseData(rotateAnalyseID);
            if (rotateData == nullptr)
            {
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_RotateAnalyse, rotateAnalyseID);
                return;
            }

            mTreeItem* _rotateItem = _treeModel->getParentItem(CaseRotateParentItem, caseID);
            if (_rotateItem == nullptr)
            {
                // 若转子分析父节点缺失，但该工况支持转子动力学，则自动创建父节点
                mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
                if (caseBase == nullptr || caseBase->getCaseType() != ImplicitModal)
                    return;

                mCaseImplicitModal* modalData = dynamic_cast<mCaseImplicitModal*>(caseBase);
                bool showRotational = modalData && (modalData->_modalType == "复模态分析") && modalData->_rotationalChecked;
                if (!showRotational)
                    return;

                int modelID = getCVM->getModelData()->getModelDataID();
                BrowserTreeType bt = BrowserTreeType(int(CaseImplicitLinearStaticParentItem) + ImplicitModal - 1);
                mTreeItem* parentItem = _treeModel->getParentItem(bt, caseID);
                if (parentItem == nullptr)
                    return;

                _rotateItem = new mTreeItem(modelID, CaseRotateParentItem, "转子分析", caseID);
                _treeModel->appendChildren(parentItem, _rotateItem);
            }

            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseRotateChildItem, rotateData->getRotateAnalyseName(), rotateAnalyseID);
            childitem->setCaseID(caseID);
            _treeModel->appendChildren(_rotateItem, childitem);
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_RotateAnalyse, rotateAnalyseID }, { Physical_Case, caseID });
            selectedItem(childitem);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_RotateAnalyse, rotateAnalyseID);
    }

    void mTreeView::slot_updateBoundaryOnCaseTree(bool isCreate, int boundaryID)
    {
        //定位分析工况节点
        QModelIndexList idxs = getSelectIndex();
        QModelIndex porxyIndex;
        for (auto& idx : idxs)
        {
            auto seltype = getTreeTypeByModelIndex(idx);
            auto parenttype = getTreeTypeByModelIndex(idx.parent());
            if (isCaseParentItem(idx))
            {
                porxyIndex = idx;
                break;
            }
            //分析工况下的节点
            else if (seltype == CaseLoadParentItem || seltype == CaseBCParentItem || seltype == CaseSettingChildItem || seltype == ResultParentItem)
            {
                porxyIndex = idx.parent();
                break;
            }
            //载荷节点
            else if ((seltype == CaseLoadChildItem || seltype == CaseBCChildItem) && (parenttype != LoadManagerParentItem && parenttype != BoundaryManagerParentItem))
            {
                porxyIndex = idx.parent().parent();
                break;
            }
        }

        //挂载到边界管理器中
        mTreeItem* _boundaryManagerItem = _treeModel->getParentItem(BoundaryManagerParentItem);
        if (isCreate && _boundaryManagerItem != nullptr)
        {
            auto boundaryData = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(boundaryID);
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseBCChildItem, boundaryData->getLoadBoundaryName(), boundaryID);
            _treeModel->appendChildren(_boundaryManagerItem, childitem);

            if (porxyIndex.isValid())
            {
                int caseID = getPhyIdByModelIndex(porxyIndex);
                //找到分析节点下的边界父节点
                mTreeItem* _bcItem = _treeModel->getParentItem(CaseBCParentItem, caseID);
                //若工况下不存在边界父节点（该工况类型不支持边界），不再挂载到工况
                if (_bcItem != nullptr)
                {
                    //边界数据挂载
                    childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseBCChildItem, boundaryData->getLoadBoundaryName(), boundaryID);
                    childitem->setCaseID(caseID);
                    _treeModel->appendChildren(_bcItem, childitem);
                    getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary , boundaryID }, { Physical_Case ,caseID });
                }
            }
            if (childitem) selectedItem(childitem);
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, boundaryID);
    }

    void mTreeView::slot_updatePredefinedFieldOnCaseTree(bool isCreate, int fieldID)
    {
        //定位分析工况节点
        QModelIndexList idxs = getSelectIndex();
        QModelIndex porxyIndex;
        for (auto& idx : idxs)
        {
            auto seltype = getTreeTypeByModelIndex(idx);
            auto parenttype = getTreeTypeByModelIndex(idx.parent());
            if(isCaseParentItem(idx))
            {
                porxyIndex = idx;
                break;
            }
            //分析工况下的节点
            else if (seltype == CaseLoadParentItem || seltype == CaseBCParentItem || seltype == CasePredefinedFieldParentItem || seltype == CaseSettingChildItem || seltype == ResultParentItem)
            {
                porxyIndex = idx.parent();
                break;
            }
            //载荷/边界/预定义场节点
            else if ((seltype == CaseLoadChildItem || seltype == CaseBCChildItem || seltype == CasePredefinedFieldChildItem) && (parenttype != LoadManagerParentItem && parenttype != BoundaryManagerParentItem && parenttype != PredefinedFieldManagerParentItem))
            {
                porxyIndex = idx.parent().parent();
                break;
            }
        }

        int caseID = 0;
        if (porxyIndex.isValid())
        {
            caseID = getPhyIdByModelIndex(porxyIndex);
        }

        if (isCreate)
        {
            //挂载到预定义场管理器中
            mTreeItem* _predefinedFieldManagerItem = _treeModel->getParentItem(PredefinedFieldManagerParentItem);
            auto rootItem = _treeModel->getParentItem(ModelRootItem);

            if (_predefinedFieldManagerItem == nullptr)
            {
                _predefinedFieldManagerItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), PredefinedFieldManagerParentItem, "预定义场");
                _treeModel->insertChildren(rootItem, _predefinedFieldManagerItem, getInsertLocation(PredefinedFieldManagerParentItem));
            }
            auto fieldData = getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(fieldID);
            if (fieldData)
            {
                auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CasePredefinedFieldChildItem, fieldData->getPredefinedFieldName(), fieldID);
                _treeModel->appendChildren(_predefinedFieldManagerItem, childitem);

                if (caseID != 0)
                {
                    //找到分析节点下的预定义场父节点
                    mTreeItem* _predefinedFieldItem = _treeModel->getParentItem(CasePredefinedFieldParentItem, caseID);
                    //若工况下不存在预定义场父节点（该工况类型不支持预定义场），不再挂载到工况
                    if (_predefinedFieldItem != nullptr)
                    {
                        //预定义场数据挂载
                        childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CasePredefinedFieldChildItem, fieldData->getPredefinedFieldName(), fieldID);
                        childitem->setCaseID(caseID);
                        _treeModel->appendChildren(_predefinedFieldItem, childitem);
                        getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_PredefinedField , fieldID }, { Physical_Case ,caseID });
                    }
                }
                if (childitem) selectedItem(childitem);
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PredefinedField, fieldID);
    }

    void mTreeView::slot_menuCopyCase()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        mTreeItem* lastItem = nullptr;
        int modelID = getCVM->getModelData()->getModelDataID();
        for (auto& idx : idxs)
        {
            int copyId = -1;
            int caseId = -1;
            BrowserTreeType treeType = getTreeTypeByModelIndex(idx);
            std::set<int> copyLoadBoundaryIds;
            std::set<int> copyPredefinedFieldIds;
            std::set<int> copyguoyingjiechuIds;
            QList<int> copyNsmIds;
            mMapRelationManager& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
            if (int(treeType) >= CaseImplicitLinearStaticParentItem && int(treeType) < CaseSettingChildItem)
            {
                //原始工况Id
                caseId = getPhyIdByModelIndex(idx);
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Case, caseId);
                //获取原始工况下的载荷边界映射关系,载荷边界融合，获取一次即可
                copyLoadBoundaryIds = mapRelationManager.getMapRelation<std::set<int>>({ Physical_Case, caseId }, Physical_LoadBoundary);
                //预定义场
                copyPredefinedFieldIds = mapRelationManager.getMapRelation<std::set<int>>({ Physical_Case, caseId }, Physical_PredefinedField);
                //相互作用(过盈接触)
                copyguoyingjiechuIds= mapRelationManager.getMapRelation<std::set<int>>({ Physical_Case, caseId }, Physical_Contact);

            }
            if (copyId == -1)
            {
                continue;
            }
            int id = copyId;
            mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(id);
            if (caseBase == nullptr)
                continue;

            mTreeItem* caseitem = new mTreeItem(modelID, treeType, caseBase->getCaseName(), id, Tree_Yes, getCaseIconPath(caseBase->getCaseType()));
            _treeModel->insertChildren(_treeModel->getParentItem(ModelRootItem), caseitem, getInsertLocation(CaseParentItem));
            lastItem = caseitem;

            //分析设置
            auto caseSettingItem = new mTreeItem(modelID, CaseSettingChildItem, "分析设置", id);
            _treeModel->appendChildren(caseitem, caseSettingItem);

            //复制子节点及其数据（边界、载荷等等）
            //载荷父节点
            auto caseLoadItem = new mTreeItem(modelID, CaseLoadParentItem, "载荷", id);
            _treeModel->appendChildren(caseitem, caseLoadItem);
            //边界父节点
            auto BCItem = new mTreeItem(modelID, CaseBCParentItem, "边界", id);
            _treeModel->appendChildren(caseitem, BCItem);
            for (auto loadBoundaryId : copyLoadBoundaryIds)
            {
                mapRelationManager.appendMapRelation({ Physical_LoadBoundary ,loadBoundaryId }, { Physical_Case,id });
                std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBoundary = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadBoundaryId);
                if (loadBoundary == nullptr)
                    continue;
                //判断类型
                if (loadBoundary->isLoadOrBoundary() == 0) //载荷
                {
                    mTreeItem* childitem = new mTreeItem(modelID, CaseLoadChildItem, loadBoundary->getLoadBoundaryName(), loadBoundaryId);
                    _treeModel->appendChildren(caseLoadItem, childitem);
                    getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, loadBoundaryId);
                    lastItem = childitem;
                }
                else if (loadBoundary->isLoadOrBoundary() == 1) //边界
                {
                    mTreeItem* childitem = new mTreeItem(modelID, CaseBCChildItem, loadBoundary->getLoadBoundaryName(), loadBoundaryId);
                    _treeModel->appendChildren(BCItem, childitem);
                    getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, loadBoundaryId);
                    lastItem = childitem;
                }
            }

            //预定义场父节点
            auto casePredefinedFieldItem = new mTreeItem(modelID, CasePredefinedFieldParentItem, "预定义场", id);
            _treeModel->appendChildren(caseitem, casePredefinedFieldItem);
            for (auto copyPredefinedFieldId : copyPredefinedFieldIds)
            {
                mapRelationManager.appendMapRelation({ Physical_LoadBoundary ,copyPredefinedFieldId }, { Physical_Case,id });
                QString name = getCVM->getModelData()->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldNameByID(copyPredefinedFieldId);
                mTreeItem* childitem = new mTreeItem(modelID, CasePredefinedFieldChildItem, name, copyPredefinedFieldId);
                _treeModel->appendChildren(casePredefinedFieldItem, childitem);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PredefinedField, copyPredefinedFieldId);
                lastItem = childitem;
            }

            //非结构质量父节点
            //TO DO 需要把对应的关系数据也拷贝过去
            auto nsmItem = new mTreeItem(modelID, CaseNSMManagerItem, "非结构质量", id);
            _treeModel->appendChildren(caseitem, nsmItem);

            // 频响和声固耦合：复制诊断优化及其子节点
            if (caseBase->getCaseType() == ImplicitFrequency || caseBase->getCaseType() == ASCoupling)
            {
                auto diagOptItem = new mTreeItem(modelID, CaseDiagOptParentItem, "诊断优化", id);
                _treeModel->appendChildren(caseitem, diagOptItem);
                //获取原始分析工况数据中是否存在贡献量峰值筛选等，有的话同步复制过来
                mCaseBase* oldCaseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseId);
                if (oldCaseBase->getCaseType() == ImplicitFrequency || oldCaseBase->getCaseType() == ASCoupling)
                {
                    if (oldCaseBase->getCaseType() == ImplicitFrequency)
                    {
                        mCaseImplicitFrequency* freqCase = dynamic_cast<mCaseImplicitFrequency*>(oldCaseBase);
                        mCaseImplicitFrequency* newFreqCase = dynamic_cast<mCaseImplicitFrequency*>(caseBase);

                        if (freqCase->getModalContriData()) //旧的有模态贡献量
                        {
                            auto childitem = new mTreeItem(modelID, CaseModalContriChildItem, "模态贡献量", id);
                            childitem->setCaseID(id);
                            _treeModel->appendChildren(diagOptItem, childitem);

                            newFreqCase->createModalContriData();
                            newFreqCase->getModalContriData()->copyData(freqCase->getModalContriData()->getId(), freqCase->getModalContriData()->getName());
                            newFreqCase->getModalContriData()->setId(newFreqCase->getCaseId());

                            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModalContri, newFreqCase->getCaseId());
                        }
                    }
                    else if (oldCaseBase->getCaseType() == ASCoupling)
                    {
                        mCaseImplASCoupling* asCase = dynamic_cast<mCaseImplASCoupling*>(oldCaseBase);
                        mCaseImplASCoupling* newAsCase = dynamic_cast<mCaseImplASCoupling*>(caseBase);

                        if (asCase->getModalContriData()) //旧的有模态贡献量
                        {
                            auto childitem = new mTreeItem(modelID, CaseModalContriChildItem, "模态贡献量", id);
                            childitem->setCaseID(id);
                            _treeModel->appendChildren(diagOptItem, childitem);

                            newAsCase->createModalContriData();
                            newAsCase->getModalContriData()->copyData(asCase->getModalContriData()->getId(), asCase->getModalContriData()->getName());
                            newAsCase->getModalContriData()->setId(newAsCase->getCaseId());

                            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModalContri, newAsCase->getCaseId());
                        }

                        if (asCase->getNodeContriData()) //旧的有节点贡献量
                        {
                            auto childitem = new mTreeItem(modelID, CaseNodeContriChildItem, "节点贡献量", id);
                            childitem->setCaseID(id);
                            _treeModel->appendChildren(diagOptItem, childitem);

                            newAsCase->createNodeContriData();
                            newAsCase->getNodeContriData()->copyData(asCase->getNodeContriData()->getId(), asCase->getNodeContriData()->getName());
                            newAsCase->getNodeContriData()->setId(newAsCase->getCaseId());

                            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_NodeContri, newAsCase->getCaseId());
                        }

                        if (asCase->getPanelContriData()) //旧的有面板贡献量
                        {
                            auto childitem = new mTreeItem(modelID, CasePanelContriChildItem, "面板贡献量", id);
                            childitem->setCaseID(id);
                            _treeModel->appendChildren(diagOptItem, childitem);

                            newAsCase->createPanelContriData();
                            newAsCase->getPanelContriData()->copyData(asCase->getPanelContriData()->getId(), asCase->getPanelContriData()->getName());
                            newAsCase->getPanelContriData()->setId(newAsCase->getCaseId());

                            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PanelContri, newAsCase->getCaseId());
                        }
                    }

                    // 获取原始工况的诊断优化节点，复制其子节点
                    mTreeItem* origDiagOptItem = _treeModel->getParentItem(CaseDiagOptParentItem, caseId);
                    if (origDiagOptItem != nullptr)
                    {
                        for (auto* child : origDiagOptItem->getChildrenItem())
                        {
                            QModelIndex childIdx = _treeModel->indexFromItem(child);
                            BrowserTreeType childType = (BrowserTreeType)_treeModel->data(childIdx, Roles::TypeRole).toInt();
                            if (childType == CasePeakFilterChildItem)
                            {
                                auto childitem = new mTreeItem(modelID, CasePeakFilterChildItem, "峰值筛选", id);
                                childitem->setCaseID(id);
                                _treeModel->appendChildren(diagOptItem, childitem);
                            }
                        }
                    }
                }
            }

            //相互作用
            auto xianghuItem = new mTreeItem(modelID, CaseInteractionParentItem, "相互作用", id);
            _treeModel->appendChildren(caseitem, xianghuItem);
            for (auto jiechuid : copyguoyingjiechuIds)
            {
                mapRelationManager.appendMapRelation({ Physical_Contact ,jiechuid }, { Physical_Case,id });
                QString name = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getContactNameByID(jiechuid);
                mTreeItem* childitem = new mTreeItem(modelID, CaseInteractionChildItem, name, jiechuid);
                _treeModel->appendChildren(xianghuItem, childitem);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, jiechuid);
                lastItem = childitem;
            }

            removeCaseParentItemsByCaseType(caseBase->getCaseType(), caseitem);
        }

        selectedItem(lastItem);
    }

    void mTreeView::slot_menuCaseDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            BrowserTreeType type = getTreeTypeByModelIndex(idx);
            if (int(type) >= CaseImplicitLinearStaticParentItem && int(type) < CaseSettingChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);

                // 根据工况类型删除对应的贡献量数据
                mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(phyID);
                if (caseBase != nullptr)
                {
                    MxFunctions::CaseType caseType = caseBase->getCaseType();
                    if (caseType == MxFunctions::ImplicitFrequency)
                    {
                        mCaseImplicitFrequency* freqCase = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
                        if (freqCase != nullptr)
                            freqCase->deleteModalContriData();
                    }
                    else if (caseType == MxFunctions::ASCoupling)
                    {
                        mCaseImplASCoupling* asCouplingCase = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                        if (asCouplingCase != nullptr)
                        {
                            asCouplingCase->deleteModalContriData();
                            asCouplingCase->deleteNodeContriData();
                            asCouplingCase->deletePanelContriData();
                        }
                    }
                }

                //删除面板
                mInfoWindow::getInstance()->deleteCase(phyID);
                //删除数据
                getCVM->getModelData()->deleteByPhysicalType(Physical_Case, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Case, phyIDs);

        selectedItem(_treeModel->getParentItem(ModelRootItem));
    }

    void mTreeView::slot_createLoad(MxFunctions::LoadBoundaryType type)
    {
        mInfoWindow::getInstance()->createLoad(type);
    }

    void mTreeView::slot_createRotateAnalyse(MxFunctions::RotateAnalysyType type)
    {
        mInfoWindow::getInstance()->createRotateAnalyse(type);
    }

    void mTreeView::slot_menuRotateChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        mTreeItem* _rotateItem = nullptr;
        vector<int> phyIDs;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            int caseID = getPhyIdByModelIndex(idx.parent());
            int rotateID = getPhyIdByModelIndex(idx);

            deleteItems.push_back(idx);
            // 转子分析子节点只存在于工况下，删除时同步删除实际数据及所有映射
            getCVM->getModelData()->deleteByPhysicalType(Physical_RotateAnalyse, rotateID);
            _rotateItem = _treeModel->getParentItem(CaseRotateParentItem, caseID);
            phyIDs.push_back(rotateID);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RotateAnalyse, phyIDs);
        removeProxyIndexs(deleteItems);
        if (_rotateItem) selectedItem(_rotateItem);
    }

    void mTreeView::slot_menuRotateDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        QModelIndex idx = idxs.back();
        auto seltype = getTreeTypeByModelIndex(idx);

        mTreeItem* _rotateItem = _treeModel->getParentItem(seltype, getPhyIdByModelIndex(idx));
        if (seltype == CaseRotateParentItem)
        {
            int caseID = getPhyIdByItem(_rotateItem);
            vector<int> phyIDs;
            QModelIndexList deleteItems;
            for (auto& item : _rotateItem->getChildrenItem())
            {
                int rotateID = getPhyIdByItem(item);
                // 删除实际数据，会同步删除 Physical_Case 映射及 Physical_Set 映射
                getCVM->getModelData()->deleteByPhysicalType(Physical_RotateAnalyse, rotateID);
                QModelIndex itemIdx = _treeModel->indexFromItem(item);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(itemIdx);
                deleteItems.push_back(proxyIndex);
                phyIDs.push_back(rotateID);
            }
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_RotateAnalyse, phyIDs);
            removeProxyIndexs(deleteItems);
            selectedItem(_rotateItem);
        }
    }

    void mTreeView::slot_menuCopyRotate()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        int copyId, caseID, rotateID;
        mTreeItem* lastItem = nullptr;
        QHash<mTreeItem*, QVector<mTreeItem*>> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            caseID = getPhyIdByModelIndex(idx.parent());
            rotateID = getPhyIdByModelIndex(idx);
            copyId = getCVM->getModelData()->copyPhysicalField(Physical_RotateAnalyse, rotateID, caseID);
            if (copyId == -1) continue;

            copyIDs.push_back(copyId);

            std::shared_ptr<MechCaseData::mRoatateAnalyseBase> rotate =
                getCVM->getModelData()->getManager<MechCaseData::mRotateAnalyseManager>(Physical_RotateAnalyse).getRotateAnalyseData(copyId);
            if (rotate == nullptr) continue;

            lastItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseRotateChildItem, rotate->getRotateAnalyseName(), copyId);
            addItems[_treeModel->getParentItem(CaseRotateParentItem, caseID)].push_back(lastItem);
        }

        for (auto it = addItems.begin(); it != addItems.cend(); ++it)
        {
            _treeModel->appendChildren(it.key(), std::move(it.value()));
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_RotateAnalyse, copyIDs } });

        // 高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_createBoundary(MxFunctions::LoadBoundaryType type)
    {
        mInfoWindow::getInstance()->createBoundary(type);
    }

    void mTreeView::slot_menuLoadChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        mTreeItem* _loadItem;
        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
           int caseID = getPhyIdByModelIndex(idx.parent());
           int loadID = getPhyIdByModelIndex(idx);

           deleteItems.push_back(idx);
           if(caseID == 0)
           {
                //从管理器删除会既删除树节点又删除数据，同时删除所有调用此载荷的工况下的树节点
                _loadItem = _treeModel->getParentItem(LoadManagerParentItem);

                //根据载荷id获取与其映射的工况id
                std::unordered_map<int, double> caseMap =
                    getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, loadID }, Physical_Case);

                for (auto caseIds : caseMap)
                {
                    mTreeItem* load= _treeModel->getChildrenItem(CaseLoadChildItem, loadID, caseIds.first);
                    if (load == nullptr) continue;
                    QModelIndex idx1 = _treeModel->indexFromItem(load);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx1);
                    deleteItems.push_back(proxyIndex);
                }
                getCVM->getModelData()->deleteByPhysicalType(Physical_LoadBoundary, loadID);
           }
           else
           {
               //不删除数据，只删除树节点上
               getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_LoadBoundary, loadID });
               _loadItem = _treeModel->getParentItem(CaseLoadParentItem, caseID);
           }
           phyIDs.push_back(loadID);
        }


        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_LoadBoundary, phyIDs);
        removeProxyIndexs(deleteItems);
        if(_loadItem) selectedItem(_loadItem);
    }

    void mTreeView::slot_menuLoadDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        QModelIndex idx = idxs.back();
        auto seltype = getTreeTypeByModelIndex(idx);

        mTreeItem* _loadItem = _treeModel->getParentItem(seltype, getPhyIdByModelIndex(idx));
        if (seltype == CaseLoadParentItem)
        {
            int caseID = getPhyIdByItem(_loadItem);
            for (auto& item : _loadItem->getChildrenItem())
            {
                int loadID = getPhyIdByItem(item);
                //不删除数据，只删除树节点
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_LoadBoundary, loadID });
            }
            removeTreeChildrenItem(_loadItem);
            selectedItem(_loadItem);
        }
        else if (seltype == LoadManagerParentItem)
        {
            QModelIndexList deleteItems; //删除容器
            vector<int> phyIDs;
            //获取所有载荷item
            QVector<mTreeItem*> loadItems = _loadItem->getChildrenItem();

            //根据item获取idx，放入删除容器
            for (auto loaditem : loadItems)
            {
                QModelIndex idx = _treeModel->indexFromItem(loaditem);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                //根据id删除数据
                int loadId = getPhyIdByModelIndex(proxyIndex);
                deleteItems.push_back(proxyIndex);
                phyIDs.push_back(loadId);

                //根据载荷id获取与其映射的工况id
                std::unordered_map<int, double> caseMap =
                    getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, loadId }, Physical_Case);
                for (auto caseIds : caseMap)
                {
                    mTreeItem* load = _treeModel->getChildrenItem(CaseLoadChildItem, loadId, caseIds.first);
                    if (load == nullptr) continue;
                    QModelIndex idx1 = _treeModel->indexFromItem(load);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx1);
                    deleteItems.push_back(proxyIndex);
                }

                getCVM->getModelData()->deleteByPhysicalType(Physical_LoadBoundary, loadId);
            }

            removeProxyIndexs(deleteItems);
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_LoadBoundary, phyIDs);
            selectedItem(_loadItem);
        }
    }

    void mTreeView::slot_menuCopyLoad()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        int copyId, caseID, loadID;
        mTreeItem* lastItem = nullptr;
        QHash<mTreeItem*, QVector<mTreeItem*>> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            caseID = getPhyIdByModelIndex(idx.parent());
            loadID = getPhyIdByModelIndex(idx);
            copyId = getCVM->getModelData()->copyPhysicalField(Physical_LoadBoundary, loadID, caseID);
            if (copyId == -1) continue;

            copyIDs.push_back(copyId);

            std::shared_ptr<MechCaseData::mLoadBoundaryBase> load = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(copyId);
            lastItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseLoadChildItem, load->getLoadBoundaryName(), copyId);
            addItems[_treeModel->getParentItem(LoadManagerParentItem)].push_back(lastItem);

            if (getTreeTypeByModelIndex(idx.parent()) == CaseLoadParentItem)
            {
                //此处复制需要工况下载荷节点添加
                lastItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseLoadChildItem, load->getLoadBoundaryName(), copyId);
                lastItem->setCaseID(caseID);
                addItems[_treeModel->getParentItem(CaseLoadParentItem, caseID)].push_back(lastItem);
            }
        }

        for (auto it = addItems.begin(); it != addItems.cend(); ++it)
        {
            _treeModel->appendChildren(it.key(), std::move(it.value()));
        }
       
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_LoadBoundary, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_menuBoundaryChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        mTreeItem* _boundaryItem;
        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            int caseID = getPhyIdByModelIndex(idx.parent());
            int bcID = getPhyIdByModelIndex(idx);

            deleteItems.push_back(idx);
            if (caseID == 0)
            {
                //从管理器删除会既删除树节点又删除数据，同时删除所有调用此载荷的工况下的树节点
                _boundaryItem = _treeModel->getParentItem(LoadManagerParentItem);

                //根据载荷id获取与其映射的工况id
                std::unordered_map<int, double> caseMap =
                    getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, bcID }, Physical_Case);

                for (auto caseIds : caseMap)
                {
                    mTreeItem* bc = _treeModel->getChildrenItem(CaseBCChildItem, bcID, caseIds.first);
                    if (bc == nullptr) continue;
                    QModelIndex idx1 = _treeModel->indexFromItem(bc);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx1);
                    deleteItems.push_back(proxyIndex);
                }
                getCVM->getModelData()->deleteByPhysicalType(Physical_LoadBoundary, bcID);
            }
            else
            {
                //不删除数据，只删除节点和映射
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_LoadBoundary, bcID });
                _boundaryItem = _treeModel->getParentItem(CaseBCParentItem, caseID);
            }
            phyIDs.push_back(bcID);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_LoadBoundary, phyIDs);
        removeProxyIndexs(deleteItems);
        if (_boundaryItem) selectedItem(_boundaryItem);
    }

    void mTreeView::slot_menuBoundaryDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        QModelIndex idx = idxs.back();
        auto seltype = getTreeTypeByModelIndex(idx);

        mTreeItem* _boundaryItem = _treeModel->getParentItem(seltype, getPhyIdByModelIndex(idx));
        if (seltype == CaseBCParentItem)
        {
            int caseID = getPhyIdByItem(_boundaryItem);
            for (auto& item : _boundaryItem->getChildrenItem())
            {
                int bcID = getPhyIdByItem(item);
                //不删除数据，只删除节点和映射
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_LoadBoundary, bcID });
            }
            removeTreeChildrenItem(_boundaryItem);
            selectedItem(_boundaryItem);
        }
        else if (seltype == BoundaryManagerParentItem)
        {
            QModelIndexList deleteItems; //删除容器
            vector<int> phyIDs;
            //获取所有边界item
            QVector<mTreeItem*> boundaryItems = _boundaryItem->getChildrenItem();

            //根据item获取idx，放入删除容器
            for (auto boundaryitem : boundaryItems)
            {
                QModelIndex idx = _treeModel->indexFromItem(boundaryitem);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                //根据id删除数据
                int boundaryId = getPhyIdByModelIndex(proxyIndex);
                deleteItems.push_back(proxyIndex);
                phyIDs.push_back(boundaryId);

                //根据载荷id获取与其映射的工况id
                std::unordered_map<int, double> caseMap =
                    getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, boundaryId }, Physical_Case);

                for (auto caseIds : caseMap)
                {
                    mTreeItem* boundary = _treeModel->getChildrenItem(CaseBCChildItem, boundaryId, caseIds.first);
                    QModelIndex idx1 = _treeModel->indexFromItem(boundary);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx1);
                    deleteItems.push_back(proxyIndex);
                }
                getCVM->getModelData()->deleteByPhysicalType(Physical_LoadBoundary, boundaryId);
            }

            removeProxyIndexs(deleteItems);
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_LoadBoundary, phyIDs);
            selectedItem(_boundaryItem);
        }
    }

    void mTreeView::slot_menuCopyBoundary()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        int copyId, caseID, bcID;
        mTreeItem* lastItem = nullptr;
        QHash<mTreeItem*, QVector<mTreeItem*>> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            caseID = getPhyIdByModelIndex(idx.parent());
            bcID = getPhyIdByModelIndex(idx);
            copyId = getCVM->getModelData()->copyPhysicalField(Physical_LoadBoundary, bcID, caseID);
            if (copyId == -1) continue;

            copyIDs.push_back(copyId);

            std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundary = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(copyId);
            lastItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseBCChildItem, boundary->getLoadBoundaryName(), copyId);
            addItems[_treeModel->getParentItem(BoundaryManagerParentItem)].push_back(lastItem);

            if (getTreeTypeByModelIndex(idx.parent()) == CaseBCParentItem)
            {
                //此处复制需要工况下载荷节点与载荷管理器都添加
                std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundary = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(copyId);
                lastItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseBCChildItem, boundary->getLoadBoundaryName(), copyId);
                lastItem->setCaseID(caseID);
                addItems[_treeModel->getParentItem(CaseBCParentItem, caseID)].push_back(lastItem);
            }
        }

        for (auto it = addItems.begin(); it != addItems.cend(); ++it)
        {
            _treeModel->appendChildren(it.key(), std::move(it.value()));
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_LoadBoundary, copyIDs } });
        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_createPredefinedField(MxFunctions::PredefinedFieldType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createPredefinedField(type);
    }

    void mTreeView::slot_menuPredefinedFieldChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        mTreeItem* _fieldItem;
        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            int caseID = getPhyIdByModelIndex(idx.parent());
            int fieldID = getPhyIdByModelIndex(idx);

            deleteItems.push_back(idx);
            if (caseID == 0)
            {
                // 从管理器删除会既删除树节点又删除数据，同时删除所有调用此预定义场的工况下的树节点
                _fieldItem = _treeModel->getParentItem(PredefinedFieldManagerParentItem);

                // 根据预定义场id获取与其映射的工况id
                std::unordered_map<int, double> caseMap =
                    getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_PredefinedField, fieldID }, Physical_Case);

                for (auto caseIds : caseMap)
                {
                    mTreeItem* field = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldID, caseIds.first);
                    if (field == nullptr) continue;
                    QModelIndex idx1 = _treeModel->indexFromItem(field);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx1);
                    deleteItems.push_back(proxyIndex);
                }
                getCVM->getModelData()->deleteByPhysicalType(Physical_PredefinedField, fieldID);
                phyIDs.push_back(fieldID);
            }
            else
            {
                // 不删除数据，只删除树节点和映射
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_PredefinedField, fieldID });
                _fieldItem = _treeModel->getParentItem(CasePredefinedFieldParentItem, caseID);
            }
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PredefinedField, phyIDs);
        removeProxyIndexs(deleteItems);
        if (_fieldItem) selectedItem(_fieldItem);
    }

    void mTreeView::slot_menuPredefinedFieldDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        QModelIndex idx = idxs.back();
        auto seltype = getTreeTypeByModelIndex(idx);

        mTreeItem* _fieldItem = _treeModel->getParentItem(seltype, getPhyIdByModelIndex(idx));
        if (seltype == CasePredefinedFieldParentItem)
        {
            int caseID = getPhyIdByItem(_fieldItem);
            for (auto& item : _fieldItem->getChildrenItem())
            {
                int fieldID = getPhyIdByItem(item);
                // 不删除数据，只删除节点和映射
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_PredefinedField, fieldID });
            }
            removeTreeChildrenItem(_fieldItem);
            selectedItem(_fieldItem);
        }
        else if (seltype == PredefinedFieldManagerParentItem)
        {
            QModelIndexList deleteItems; // 删除容器
            vector<int> phyIDs;
            // 获取所有预定义场item
            QVector<mTreeItem*> fieldItems = _fieldItem->getChildrenItem();

            // 根据item获取idx，放入删除容器
            for (auto fielditem : fieldItems)
            {
                QModelIndex idx = _treeModel->indexFromItem(fielditem);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                // 根据id删除数据
                int fieldId = getPhyIdByModelIndex(proxyIndex);
                deleteItems.push_back(proxyIndex);
                phyIDs.push_back(fieldId);

                // 根据预定义场id获取与其映射的工况id
                std::unordered_map<int, double> caseMap =
                    getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_PredefinedField, fieldId }, Physical_Case);

                for (auto caseIds : caseMap)
                {
                    mTreeItem* field = _treeModel->getChildrenItem(CasePredefinedFieldChildItem, fieldId, caseIds.first);
                    QModelIndex idx1 = _treeModel->indexFromItem(field);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx1);
                    deleteItems.push_back(proxyIndex);
                }
                getCVM->getModelData()->deleteByPhysicalType(Physical_PredefinedField, fieldId);
            }

            removeProxyIndexs(deleteItems);
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PredefinedField, phyIDs);
            selectedItem(_fieldItem);
        }
    }

    void mTreeView::slot_menuCopyPredefinedField()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        int copyId, caseID, fieldID;
        mTreeItem* lastItem = nullptr;
        QHash<mTreeItem*, QVector<mTreeItem*>> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            caseID = getPhyIdByModelIndex(idx.parent());
            fieldID = getPhyIdByModelIndex(idx);
            copyId = getCVM->getModelData()->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).copyPredefinedFieldData(fieldID);
            if (copyId == -1) continue;

            copyIDs.push_back(copyId);

            std::shared_ptr<MechCaseData::mPredefinedFieldBase> field = getCVM->getModelData()->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(copyId);
            lastItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CasePredefinedFieldChildItem, field->getPredefinedFieldName(), copyId);
            addItems[_treeModel->getParentItem(PredefinedFieldManagerParentItem)].push_back(lastItem);

            if (getTreeTypeByModelIndex(idx.parent()) == CasePredefinedFieldParentItem)
            {
                // 此处复制需要工况下预定义场节点与预定义场管理器都添加
                std::shared_ptr<MechCaseData::mPredefinedFieldBase> field = getCVM->getModelData()->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(copyId);
                lastItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CasePredefinedFieldChildItem, field->getPredefinedFieldName(), copyId);
                lastItem->setCaseID(caseID);
                addItems[_treeModel->getParentItem(CasePredefinedFieldParentItem, caseID)].push_back(lastItem);
            }
        }

        for (auto it = addItems.begin(); it != addItems.cend(); ++it)
        {
            _treeModel->appendChildren(it.key(), std::move(it.value()));
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_PredefinedField, copyIDs } });
        // 高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updateMatTree(bool isCreate, int matID)
    {
        std::shared_ptr<MechMaterialData::mMaterialBase> mat = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(matID);
        if (mat == nullptr)
            return;

        mTreeItem* _matItem = _treeModel->getParentItem(MaterialParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), MaterialChildItem, mat->getMaterialName(), matID, Tree_Failure);
            _treeModel->appendChildren(_matItem, childitem);
            selectedItem(childitem);
        }
        else
        {
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Material, matID);
    }

    void mTreeView::slot_menuMaterialDelete()
    {
        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_Material);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_Material);
        auto _matItem = _treeModel->getParentItem(MaterialParentItem);
        removeTreeChildrenItem(_matItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Material, phyIDs);
        getCVM->updatePartShowColor(PartColorType::MatColor, phyIDs);
        selectedItem(_matItem);
    }

    void mTreeView::slot_menuMaterialChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _matItem = _treeModel->getParentItem(MaterialParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == MaterialChildItem)
            {
                {
                    auto matData = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(phyID);
                    mCommondMaterial* commond = new mCommondMaterial(mCommondBase::CommondType::MAT_DELUNDO);
                    commond->storeSnap(matData);
                    mCommondSystem::getInstance()->addCommond(commond);
                }
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_Material, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Material, phyIDs);
        getCVM->updatePartShowColor(PartColorType::MatColor, phyIDs);
        selectedItem(_matItem);
    }

    void mTreeView::slot_menuCopyMaterial()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _matItem = _treeModel->getParentItem(MaterialParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == MaterialChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Material, getPhyIdByModelIndex(idx));
                std::shared_ptr<MechMaterialData::mMaterialBase> mat = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(copyId);
                if (mat == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), MaterialChildItem, mat->getMaterialName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_matItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Material, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_createMaterial()
    {
        mInfoWindow::getInstance()->createMaterial();
    }

    void mTreeView::slot_menuShowHideMaterial(bool isShow)
    {
        if (getCVM == nullptr || getCVM->getModelData() == nullptr) return;

        int selectMatID = -1;

        const auto indexes = getSelectIndex();   // 先保存，避免多次调用
        if (!indexes.isEmpty()) {
            const auto& idx = indexes.first();
            selectMatID = getPhyIdByModelIndex(idx);
        }

        //通过均质三维实体截面属性找部件：材料-截面属性-部件映射
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto& propManager = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property);
        std::set<int> partIDs;
        for (int propID : propManager.getAllProperyID())
        {
            mPropertyBase* property = propManager.getPropertyData(propID);
            if (property == nullptr || property->getPropertyType() != SolidProperty) continue;

            int matID = mapRelation.getMapRelation<int>({ Physical_Property, propID }, Physical_Material);
            if (selectMatID == matID)
            {
                QList<int> partList = mapRelation.getMapRelation<QList<int>>({ Physical_Property, propID }, Physical_Part);
                for (int partID : partList)
                    partIDs.insert(partID);
            }
        }
        if (partIDs.empty()) return;

        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        if (meshData == nullptr) return;

        ////目标部件全部可见则统一隐藏，任一隐藏则统一显示；setSelectPartMask会同时处理几何与网格
        //bool allVisible = true;
        //for (int partID : partIDs)
        //{
        //    if (meshData->getPartMask(partID)) { allVisible = false; break; }
        //}
        meshData->setSelectPartMask(partIDs, isShow);
        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), partIDs);
    }

    void mTreeView::slot_menuAddToMaterialDepot()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        QVector<int> matIds;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != MaterialChildItem)
            {
                continue;
            }
            int matID = getPhyIdByModelIndex(idx);
            matIds.push_back(matID);
        }
        emit MechCommon::mMechGlobalSignals::getInstance()->addToMaterialDepot(matIds);
    }

    void mTreeView::slot_updateOptimizationTree(bool isCreate)
    {

        int modelID = getCVM->getModelData()->getModelDataID();
        //树上查找连接父节点
        auto _optItem = _treeModel->getParentItem(OptimizationParentItem);

        if (_optItem == nullptr)
        {
            mTreeItem* parentItem = new mTreeItem(modelID, OptimizationParentItem, "优化");
            _treeModel->insertChildren(_treeModel->getParentItem(ModelRootItem), parentItem, GlobalParamParentItem);

            QVector<mTreeItem*> allChildItem;
            //设计域
            mTreeItem* item = new mTreeItem(modelID, OptimizationDesignParentItem, "设计域");
            allChildItem.push_back(item);
            //优化目标
            item = new mTreeItem(modelID, OptimizationGoalParentItem, "优化目标");
            allChildItem.push_back(item);
            //优化约束
            item = new mTreeItem(modelID, OptimizationConstraintsParentItem, "优化约束");
            allChildItem.push_back(item);

            _treeModel->appendChildren(parentItem, std::move(allChildItem));

            selectedItem(parentItem);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationGoal, modelID);
        }
        else
        {
            

        }

           
    }
    void mTreeView::slot_updateConnectorTree(bool isCreate, int connID)
    {
        std::shared_ptr<MechConnectorData::mConnectorBase> conn = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(connID);
        if (conn == nullptr)
            return;
        int modelID = getCVM->getModelData()->getModelDataID();
        //树上查找连接父节点
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        //找连接节点没有就在相互作用节点创建一个
        mTreeItem* _connItem = nullptr;
        auto childItems = _interItem->getChildrenItem();
        for (int i = 0; i < childItems.size(); ++i)
        {
            if (getTreeTypeByItem(childItems[i]) == ConnectorParentItem)
            {
                _connItem = childItems[i];
                break;
            }
        }
        if (_connItem == nullptr)
        {
            _connItem = new mTreeItem(modelID, ConnectorParentItem, "连接");
            _treeModel->appendChildren(_interItem, _connItem);
        }

        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, ConnectorChildItem, conn->getConnectorName(), connID, Tree_Failure);
            _treeModel->appendChildren(_connItem, childitem);
            selectedItem(childitem);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Connector, connID);
    }

    void mTreeView::slot_updateDeputyTree(bool isCreate, int deputyID)
    {
        std::shared_ptr<MechConnectorData::mDeputyBase> deputy = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyData(deputyID);
        if (deputy == nullptr)
            return;
        int modelID = getCVM->getModelData()->getModelDataID();
        //树上查找连接副父节点
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        //找连接副节点没有就在相互作用节点创建一个
        mTreeItem* _deputyItem = nullptr;
        auto childItems = _interItem->getChildrenItem();
        for (int i = 0; i < childItems.size(); ++i)
        {
            if (getTreeTypeByItem(childItems[i]) == DeputyParentItem)
            {
                _deputyItem = childItems[i];
                break;
            }
        }
        if (_deputyItem == nullptr)
        {
            _deputyItem = new mTreeItem(modelID, DeputyParentItem, "连接副");
            _treeModel->appendChildren(_interItem, _deputyItem);
        }

        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, DeputyChildItem, deputy->getDeputyName(), deputyID, Tree_Failure);
            _treeModel->appendChildren(_deputyItem, childitem);
            selectedItem(childitem);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Deputy, deputyID);
    }

    void mTreeView::slot_menuConnectorDelete()
    {
        deleteSymbol();

        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_Connector);
        for (auto phyID : phyIDs) {
            auto _loadData = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(phyID);
            auto nids = _loadData->getAllNodesInConnector();
            MechMainWindow::mDisplayMenuFunction::getInstance()->slot_showFreeNode(nids, false);
        }
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_Connector);
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        auto _connItem = _treeModel->getParentItem(ConnectorParentItem);
        removeTreeItem(_connItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Connector, phyIDs);
        selectedItem(_interItem);
    }

    void mTreeView::slot_menuDeputyDelete()
    {
        deleteSymbol();

        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_Deputy);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_Deputy);
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        auto _deputyItem = _treeModel->getParentItem(DeputyParentItem);
        removeTreeItem(_deputyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Deputy, phyIDs);
        selectedItem(_interItem);
    }

    void mTreeView::slot_menuConnectorChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _connItem = _treeModel->getParentItem(ConnectorParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == ConnectorChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                {
                    auto _loadData = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(phyID);
                    auto nids = _loadData->getAllNodesInConnector();
                    MechMainWindow::mDisplayMenuFunction::getInstance()->slot_showFreeNode(nids, false);
                    mCommondConnector* commond = new mCommondConnector(mCommondBase::CommondType::CONNECTOR_DELUNDO);
                    commond->_connectorData = _loadData;
                    mCommondSystem::getInstance()->addCommond(commond);
                }
                getCVM->getModelData()->deleteByPhysicalType(Physical_Connector, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //判断集合个数
        int conId = getCVM->getModelData()->getAllPhyID(Physical_Connector).size();
        if (conId == 0 && _connItem != nullptr)
        {
            removeTreeItem(_connItem);
            selectedItem(_treeModel->getParentItem(InteractionParentItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_connItem);
        }

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Connector, phyIDs);
        
    }

    void mTreeView::slot_menuDeputyChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _deputyItem = _treeModel->getParentItem(DeputyParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == DeputyChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                {
                    auto _deputyData = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyData(phyID);
                }
                getCVM->getModelData()->deleteByPhysicalType(Physical_Deputy, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //判断集合个数
        int deputyId = getCVM->getModelData()->getAllPhyID(Physical_Deputy).size();
        if (deputyId == 0 && _deputyItem != nullptr)
        {
            removeTreeItem(_deputyItem);
            selectedItem(_treeModel->getParentItem(InteractionParentItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_deputyItem);
        }

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Deputy, phyIDs);
    }

    void mTreeView::slot_menuCopyConnector()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _connItem = _treeModel->getParentItem(ConnectorParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == ConnectorChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Connector, getPhyIdByModelIndex(idx));
                std::shared_ptr<MechConnectorData::mConnectorBase> conn = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(copyId);
                if (conn == nullptr)
                    continue;
                conn->setSuppressed(false);

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), ConnectorChildItem, conn->getConnectorName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_connItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Connector, copyIDs } });
        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_menuCopyDeputy()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _deputyItem = _treeModel->getParentItem(DeputyParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == DeputyChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Deputy, getPhyIdByModelIndex(idx));
                std::shared_ptr<MechConnectorData::mDeputyBase> deputy = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyData(copyId);
                if (deputy == nullptr)
                    continue;
                deputy->setSuppressed(false);

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), DeputyChildItem, deputy->getDeputyName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_deputyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Deputy, copyIDs } });
        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    bool mTreeView::isCurrentSelectedConnectorSuppressed()
    {
        QModelIndexList idxs = getSelectIndex();
        bool allSuppressed = true;
        bool hasItem = false;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == ConnectorChildItem)
            {
                hasItem = true;
                int phyID = getPhyIdByModelIndex(idx);
                auto conn = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(phyID);
                if (conn == nullptr || !conn->isSuppressed())
                {
                    allSuppressed = false;
                    break;
                }
            }
        }
        return hasItem && allSuppressed;
    }

    bool mTreeView::isCurrentSelectedDeputySuppressed()
    {
        QModelIndexList idxs = getSelectIndex();
        bool allSuppressed = true;
        bool hasItem = false;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == DeputyChildItem)
            {
                hasItem = true;
                int phyID = getPhyIdByModelIndex(idx);
                auto deputy = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyData(phyID);
                if (deputy == nullptr || !deputy->isSuppressed())
                {
                    allSuppressed = false;
                    break;
                }
            }
        }
        return hasItem && allSuppressed;
    }

    bool mTreeView::isCurrentSelectedContactSuppressed()
    {
        QModelIndexList idxs = getSelectIndex();
        bool allSuppressed = true;
        bool hasItem = false;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == ContactChildItem)
            {
                hasItem = true;
                int phyID = getPhyIdByModelIndex(idx);
                auto contact = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(phyID);
                if (contact == nullptr || !contact->isSuppressed())
                {
                    allSuppressed = false;
                    break;
                }
            }
        }
        return hasItem && allSuppressed;
    }

    bool mTreeView::isCurrentSelectedLoadBoundarySuppressed()
    {
        QModelIndexList idxs = getSelectIndex();
        bool allSuppressed = true;
        bool hasItem = false;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == CaseLoadChildItem || getTreeTypeByModelIndex(idx) == CaseBCChildItem)
            {
                hasItem = true;
                int phyID = getPhyIdByModelIndex(idx);
                auto lb = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(phyID);
                if (lb == nullptr || !lb->isSuppressed())
                {
                    allSuppressed = false;
                    break;
                }
            }
        }
        return hasItem && allSuppressed;
    }

    void mTreeView::slot_menuSuppressConnector()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        bool toSuppress = !isCurrentSelectedConnectorSuppressed();
        std::vector<int> phyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != ConnectorChildItem) continue;
            int phyID = getPhyIdByModelIndex(idx);
            auto conn = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(phyID);
            if (conn == nullptr) continue;
            conn->setSuppressed(toSuppress);
            phyIDs.push_back(phyID);
        }
        if (!phyIDs.empty())
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Connector, phyIDs } });
        }
    }

    void mTreeView::slot_menuSuppressDeputy()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        bool toSuppress = !isCurrentSelectedDeputySuppressed();
        std::vector<int> phyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != DeputyChildItem) continue;
            int phyID = getPhyIdByModelIndex(idx);
            auto deputy = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyData(phyID);
            if (deputy == nullptr) continue;
            deputy->setSuppressed(toSuppress);
            phyIDs.push_back(phyID);
        }
        if (!phyIDs.empty())
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Deputy, phyIDs } });
        }
    }

    void mTreeView::slot_menuSuppressContact()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        bool toSuppress = !isCurrentSelectedContactSuppressed();
        std::vector<int> phyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != ContactChildItem) continue;
            int phyID = getPhyIdByModelIndex(idx);
            auto contact = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(phyID);
            if (contact == nullptr) continue;
            contact->setSuppressed(toSuppress);
            phyIDs.push_back(phyID);
        }
        if (!phyIDs.empty())
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Contact, phyIDs } });
        }
    }

    void mTreeView::slot_menuSuppressLoadBoundary()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        bool toSuppress = !isCurrentSelectedLoadBoundarySuppressed();
        std::vector<int> phyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != CaseLoadChildItem && getTreeTypeByModelIndex(idx) != CaseBCChildItem) continue;
            int phyID = getPhyIdByModelIndex(idx);
            auto lb = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(phyID);
            if (lb == nullptr) continue;
            lb->setSuppressed(toSuppress);

            if (toSuppress)
            {
                // 删除该载荷边界与所有工况的映射，并立即从分析工况树中移除
                auto caseIds = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getAllCaseID();
                for (int caseId : caseIds)
                {
                    getCVM->getModelData()->getMapRelationManager().deleteMapRelation(
                        { Physical_Case, caseId },
                        { Physical_LoadBoundary, phyID }
                    );
                    emit MechCommon::mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseId, phyID, 1);
                }
            }
            phyIDs.push_back(phyID);
        }

        if (!phyIDs.empty())
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_LoadBoundary, phyIDs } });
        }
    }

    QString mTreeView::slot_connTypePic(MxFunctions::ConnectorType type)
    {
        switch (type)
        {
        case MxFunctions::Connector_Spring:
            return QString(":/mMainWidgets/Icon/BrowserTree/tanhuang.png");
            break;
        case MxFunctions::Connector_Damping:
            return QString(":/mMainWidgets/Icon/BrowserTree/zuni.png");
            break;
        case MxFunctions::Connector_Rigid:
            return QString(":/mMainWidgets/Icon/BrowserTree/gangxinglianjie.png");
            break;
        case MxFunctions::Connector_Flexible:
            return QString(":/mMainWidgets/Icon/BrowserTree/rouxinglianjie.png");
            break;
        case MxFunctions::Connector_MPC:
            return QString(":/mMainWidgets/Icon/BrowserTree/duodianyueshu.png");
            break;
        case MxFunctions::Connector_Disperse:
            return QString(":/mMainWidgets/Icon/BrowserTree/lisangangti.png");
            break;
        case MxFunctions::Connector_DistributedCoupling:
            return QString(":/mMainWidgets/Icon/BrowserTree/fenbusiouhelianjie.png");
            break;
        case MxFunctions::Connector_BearingUnits:
            return QString(":/mMainWidgets/Icon/BrowserTree/zhouchengdanyuan.png");
            break;
        default:
            break;
        }

        return QString(":/mMainWidgets/Icon/BrowserTree/lianjie.png");
    }

    QString mTreeView::slot_deputyTypePic(MxFunctions::DeputyType type)
    {
        switch (type)
        {
        case MxFunctions::Deputy_Connector:
            return QString(":/mMainWidgets/Icon/BrowserTree/lianjie.png");
            break;
        case MxFunctions::Deputy_Fastener:
            return QString(":/mMainWidgets/Icon/BrowserTree/fastener.png");
            break;
        default:
            break;
        }

        return QString(":/mMainWidgets/Icon/BrowserTree/lianjie.png");
    }

    void mTreeView::slot_updateConnChildItemName(const int phyID, const QString& OldName, const QString& NewName, const QString& tarName)
    {
        auto _interactItem = _treeModel->getParentItem(InteractionParentItem);
        if (_interactItem == nullptr) return;

        QVector<mTreeItem*> connParentItems = _interactItem->getChildrenItem();

        mTreeItem* connParentItem = nullptr;
        for (int i = 0; i < connParentItems.size(); ++i)
        {
            mTreeItem* item = connParentItems[i];
            if (_treeModel->indexFromItem(item).data(Qt::DisplayRole).toString() == tarName)
            {
                connParentItem = item;
                break;
            }
        } 

        if (connParentItem == nullptr)
            return;

        QVector<mTreeItem*> childrenItems = connParentItem->getChildrenItem();
        for (int i = 0; i < childrenItems.size(); i++)
        {
            mTreeItem* item = childrenItems[i];
            if (_treeModel->indexFromItem(item).data(Roles::PhyidRole).toInt() == phyID && _treeModel->indexFromItem(item).data(Qt::DisplayRole).toString() == OldName)
            {
                setTreeItemText(item, NewName);
                return;
            }
        }

    }

    void mTreeView::slot_batchCreateContactTree(std::vector<int> ids)
    {
        if (ids.size() == 0)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        mContactManager& contactManager = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact);
        //树上查找连接父节点
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        //找接触节点没有就在相互作用节点创建一个
        mTreeItem* _contactItem = _treeModel->getParentItem(ContactParentItem);
        if (_contactItem == nullptr)
        {
            _contactItem = new mTreeItem(modelID, ContactParentItem, "接触");
            _treeModel->appendChildren(_interItem, _contactItem);
        }

        mTreeItem* lastItem;
        QVector<mTreeItem*> addItems;
        for (int i = 0; i < ids.size(); i++)
        {
            auto contact = contactManager.getContactData(ids[i]);
            lastItem = new mTreeItem(modelID, ContactChildItem, contact->getContactName(), ids[i], Tree_Yes);
            addItems.push_back(lastItem);
        }

        _treeModel->appendChildren(_contactItem, std::move(addItems));
        selectedItem(lastItem);
    }

    void mTreeView::slot_updateContactTree(bool isCreate, int contactID)
    {
        auto contact = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getContactData(contactID);
        if (contact == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        //树上查找连接父节点
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        //找接触节点没有就在相互作用节点创建一个
        mTreeItem* _contactItem = _treeModel->getParentItem(ContactParentItem);
        if (_contactItem == nullptr)
        {
            _contactItem = new mTreeItem(modelID, ContactParentItem, "接触");
            _treeModel->appendChildren(_interItem, _contactItem);
        }

        // 先检测是否从工况树下创建，保存caseID（参考载荷边界逻辑）
        int caseID = -1;
        if (isCreate)
        {
            QModelIndexList idxs = getSelectIndex();
            for (auto& idx : idxs)
            {
                auto seltype = getTreeTypeByModelIndex(idx);
                if (isCaseParentItem(idx))
                {
                    caseID = getPhyIdByModelIndex(idx);
                    break;
                }
                else if (seltype == CaseInteractionParentItem || seltype == CaseSettingChildItem || seltype == ResultParentItem)
                {
                    caseID = getPhyIdByModelIndex(idx.parent());
                    break;
                }
                else if (seltype == CaseInteractionChildItem)
                {
                    caseID = getPhyIdByModelIndex(idx.parent().parent());
                    break;
                }
            }
        }

        // 脚本文件批量执行期间不改变模型树选中节点；界面操作/命令行单条执行正常选中
        const bool autoSelect = !MechPreData::mPythonAgent::getInstance()->isScriptFileRunning();

        if (isCreate)
        {
            auto childitem = new mTreeItem(modelID, ContactChildItem, contact->getContactName(), contactID, Tree_Failure);
            _treeModel->appendChildren(_contactItem, childitem);
            if (autoSelect)
                selectedItem(childitem);
        }
        else
        {
        }

        // 如果从工况树下创建，同时挂载到工况树,只有过盈接触需要挂载
        if (isCreate && caseID != -1 && contact->getContactType() == InterferenceContact)
        {
            mTreeItem* _interactionItem = _treeModel->getParentItem(CaseInteractionParentItem, caseID);
            if (_interactionItem != nullptr)
            {
                auto childitem = new mTreeItem(modelID, CaseInteractionChildItem, contact->getContactName(), contactID, Tree_Failure);
                childitem->setCaseID(caseID);
                _treeModel->appendChildren(_interactionItem, childitem);
                // 统一映射方向为 Case -> Contact，与管理器表格的矩阵映射保持一致
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_Contact, contactID }, { Physical_Case, caseID });
                    selectedItem(childitem);
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Contact, contactID);
    }

    void mTreeView::slot_updateModelFixTree(bool isCreate, int modelFixID)
    {
        auto modelFix = getCVM->getModelData()
            ->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(modelFixID);
        if (modelFix == nullptr)
            return;

        int modelID = getCVM->getModelData()->getModelDataID();
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        if (_interItem == nullptr)
            return;

        mTreeItem* _modelFixItem = _treeModel->getParentItem(ModelFixParentItem);
        if (_modelFixItem == nullptr)
        {
            _modelFixItem = new mTreeItem(modelID, ModelFixParentItem, "模型修改");
            _treeModel->appendChildren(_interItem, _modelFixItem);
        }

        if (isCreate)
        {
            // 统一取当前数据状态作为初始状态，避免创建后显示状态与真实状态不一致
            QVector<QPair<QString, QString>> info;
            BrowserTreeStatus initStatus = getCVM->getModelData()->getDataStatus(info, Physical_ModelFix, modelFixID);
            auto childitem = new mTreeItem(modelID, ModelFixChildItem, modelFix->getModelFixName(), modelFixID, initStatus);
            _treeModel->appendChildren(_modelFixItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto childitem = _treeModel->getChildrenItem(ModelFixChildItem, modelFixID);
            setTreeItemText(childitem, modelFix->getModelFixName());
        }
    }

    void mTreeView::slot_menuModelFixDelete()
    {
        //删除渲染
        deleteSymbol();

        // 同步删除各工况树下的模型修改子节点及 Case->ModelFix 映射
        vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
        QModelIndexList deleteItems;
        for (int caseID : caseIDs)
        {
            mTreeItem* _caseModelFixItem = _treeModel->getParentItem(CaseModelFixManagerItem, caseID);
            if (_caseModelFixItem == nullptr)
            {
                continue;
            }

            for (auto* child : _caseModelFixItem->getChildrenItem())
            {
                QModelIndex childIdx = _treeModel->indexFromItem(child);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(childIdx);
                deleteItems.push_back(proxyIndex);
            }

            QList<int> modelFixIds = getCVM->getModelData()->getMapRelationManager()
                .getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
            for (int i = modelFixIds.size() - 1; i >= 0; --i)
            {
                getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation(
                    { Physical_Case, caseID },
                    { Physical_ModelFix, -1 },
                    i
                );
            }
        }

        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_ModelFix);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_ModelFix);
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        auto _modelFixItem = _treeModel->getParentItem(ModelFixParentItem);
        removeTreeItem(_modelFixItem);
        removeProxyIndexs(deleteItems);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModelFix, phyIDs);
        selectedItem(_interItem);
    }

    void mTreeView::slot_menuModelFixChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();
        mTreeItem* _modelFixItem;
        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        for (auto& idx : idxs)
        {
            int caseID = getPhyIdByModelIndex(idx.parent());
            int modelFixID = getPhyIdByModelIndex(idx);

            deleteItems.push_back(idx);
            if (caseID == 0)
            {
                // 从管理器删除：既删除树节点又删除数据，同时删除所有调用此模型修改的工况下的树节点
                _modelFixItem = _treeModel->getParentItem(ModelFixParentItem);

                // 根据 ModelFix id 获取与其映射的工况 id
                std::unordered_map<int, double> caseMap =
                    getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>
                    ({ Physical_ModelFix, modelFixID }, Physical_Case);

                for (auto caseIds : caseMap)
                {
                    mTreeItem* modelFix = _treeModel->getChildrenItem(CaseModelFixChildItem, modelFixID, caseIds.first);
                    if (modelFix == nullptr) continue;
                    QModelIndex idx1 = _treeModel->indexFromItem(modelFix);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx1);
                    deleteItems.push_back(proxyIndex);
                }
                getCVM->getModelData()->deleteByPhysicalType(Physical_ModelFix, modelFixID);
            }
            else
            {
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation(
                    { Physical_Case, caseID },
                    { Physical_ModelFix, modelFixID }
                );
                
                _modelFixItem = _treeModel->getParentItem(CaseModelFixManagerItem, caseID);
            }
            phyIDs.push_back(modelFixID);
        }

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModelFix, phyIDs);
        removeProxyIndexs(deleteItems);
        if (_modelFixItem) selectedItem(_modelFixItem);
    }

    void mTreeView::slot_menuCopyModelFix()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _modelFixItem = _treeModel->getParentItem(ModelFixParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == ModelFixChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_ModelFix, getPhyIdByModelIndex(idx));
                auto modelFix = getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(copyId);
                if (modelFix == nullptr)
                    continue;
                modelFix->setSuppressed(false);

                QVector<QPair<QString, QString>> info;
                BrowserTreeStatus initStatus = getCVM->getModelData()->getDataStatus(info, Physical_ModelFix, copyId);
                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), ModelFixChildItem, modelFix->getModelFixName(), copyId, initStatus);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_modelFixItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_ModelFix, copyIDs } });
        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_menuCaseModelFixDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)
        {
            return;
        }

        int caseID = getPhyIdByModelIndex(idxs[0]);
        mTreeItem* _caseModelFixItem = _treeModel->getParentItem(CaseModelFixManagerItem, caseID);
        if (_caseModelFixItem == nullptr)
        {
            return;
        }

        QList<int> modelFixIds = getCVM->getModelData()->getMapRelationManager()
            .getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
        for (int modelFixID : modelFixIds)
        {
            // 删除当前工况与该模型修改的表格映射
            QList<int> currentIds = getCVM->getModelData()->getMapRelationManager()
                .getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
            int idxRelation = currentIds.indexOf(modelFixID);
            if (idxRelation >= 0)
            {
                getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation(
                    { Physical_Case, caseID },
                    { Physical_ModelFix, -1 },
                    idxRelation
                );
            }

        }
        removeTreeChildrenItem(_caseModelFixItem);

        // 清理动态信息
        std::vector<int> phyIDs(modelFixIds.begin(), modelFixIds.end());
        if (!phyIDs.empty())
        {
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModelFix, phyIDs);
        }

        selectedItem(_caseModelFixItem);
    }

    void mTreeView::slot_menuCaseModelFixChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();

        vector<int> phyIDs;
        QModelIndexList deleteItems;
        mTreeItem* _caseModelFixItem = nullptr;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != CaseModelFixChildItem)
            {
                continue;
            }

            int caseID = getCaseIdByModelIndex(idx);
            int modelFixID = getPhyIdByModelIndex(idx);

            QList<int> modelFixIds = getCVM->getModelData()->getMapRelationManager()
                .getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
            int idxRelation = modelFixIds.indexOf(modelFixID);
            if (idxRelation >= 0)
            {
                getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation(
                    { Physical_Case, caseID },
                    { Physical_ModelFix, -1 },
                    idxRelation
                );
            }

            deleteItems.push_back(idx);
            phyIDs.push_back(modelFixID);

            if (_caseModelFixItem == nullptr)
            {
                _caseModelFixItem = _treeModel->getParentItem(CaseModelFixManagerItem, caseID);
            }
        }

        removeProxyIndexs(deleteItems);
        if (_caseModelFixItem)
        {
            selectedItem(_caseModelFixItem);
        }
        if (!phyIDs.empty())
        {
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModelFix, phyIDs);
        }
    }

    void mTreeView::slot_menuCopyCaseModelFix()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        int caseID = -1;
        mTreeItem* _caseModelFixItem = nullptr;
        mTreeItem* lastItem = nullptr;
        QHash<mTreeItem*, QVector<mTreeItem*>> addItems;
        vector<int> copyIDs;

        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != CaseModelFixChildItem)
            {
                continue;
            }

            caseID = getCaseIdByModelIndex(idx);
            int modelFixID = getPhyIdByModelIndex(idx);
            int copyId = getCVM->getModelData()->copyPhysicalField(Physical_ModelFix, modelFixID);
            auto modelFix = getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(copyId);
            if (modelFix == nullptr)
            {
                continue;
            }
            modelFix->setSuppressed(false);

            copyIDs.push_back(copyId);

            if (_caseModelFixItem == nullptr)
            {
                _caseModelFixItem = _treeModel->getParentItem(CaseModelFixManagerItem, caseID);
            }

            QVector<QPair<QString, QString>> info;
            BrowserTreeStatus initStatus = getCVM->getModelData()->getDataStatus(info, Physical_ModelFix, copyId);

            mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), ModelFixChildItem, modelFix->getModelFixName(), copyId, initStatus);
            addItems[_treeModel->getParentItem(ModelFixParentItem)].push_back(childitem);

            if (_caseModelFixItem != nullptr)
            {
                childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), CaseModelFixChildItem, modelFix->getModelFixName(), copyId, initStatus);
                childitem->setCaseID(caseID);
                addItems[_caseModelFixItem].push_back(childitem);
                lastItem = childitem;

                QList<int> modelFixIds = getCVM->getModelData()->getMapRelationManager()
                    .getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
                getCVM->getModelData()->getMapRelationManager().appendTableMapRelation(
                    { Physical_Case, caseID },
                    { Physical_ModelFix, copyId },
                    modelFixIds.size()
                );
            }
        }

        for (auto it = addItems.begin(); it != addItems.cend(); ++it)
        {
            _treeModel->appendChildren(it.key(), std::move(it.value()));
        }

        if (!copyIDs.empty())
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_ModelFix, copyIDs } });
        }
        if (lastItem) selectedItem(lastItem);
    }

    bool mTreeView::isCurrentSelectedModelFixSuppressed()
    {
        QModelIndexList idxs = getSelectIndex();
        bool allSuppressed = true;
        bool hasItem = false;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == ModelFixChildItem || getTreeTypeByModelIndex(idx) == CaseModelFixChildItem)
            {
                hasItem = true;
                int phyID = getPhyIdByModelIndex(idx);
                auto modelFix = getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(phyID);
                if (modelFix == nullptr || !modelFix->isSuppressed())
                {
                    allSuppressed = false;
                    break;
                }
            }
        }
        return hasItem && allSuppressed;
    }

    void mTreeView::slot_menuSuppressModelFix()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        bool toSuppress = !isCurrentSelectedModelFixSuppressed();
        std::vector<int> phyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != ModelFixChildItem && getTreeTypeByModelIndex(idx) != CaseModelFixChildItem) continue;
            int phyID = getPhyIdByModelIndex(idx);
            auto modelFix = getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(phyID);
            if (modelFix == nullptr) continue;
            modelFix->setSuppressed(toSuppress);
            phyIDs.push_back(phyID);
        }
        if (!phyIDs.empty())
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_ModelFix, phyIDs } });
        }
    }

    void mTreeView::slot_menuContactDelete()
    {
        //删除渲染
        deleteSymbol();
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() > 0)
        {
            auto seltype = getTreeTypeByModelIndex(idxs.back());
            if (seltype == CaseInteractionParentItem)
            {
                mTreeItem* _interactionItem = _treeModel->getParentItem(CaseInteractionParentItem, getPhyIdByModelIndex(idxs.back()));
                if (_interactionItem)
                {
                    int caseID = getPhyIdByItem(_interactionItem);
                    for (auto& item : _interactionItem->getChildrenItem())
                    {
                        int contactID = getPhyIdByItem(item);
                        getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_Contact, contactID });
                        emit mMechGlobalSignals::getInstance()->updateContactTreeView(caseID, contactID, 1);
                    }
                    removeTreeChildrenItem(_interactionItem);
                    selectedItem(_interactionItem);
                    return;
                }
            }
        }
        // 遍历所有工况，删除工况树下的接触节点和映射关系
        QModelIndexList deleteItems;
        vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
        for (int caseID : caseIDs)
        {
            mTreeItem* _interactionItem = _treeModel->getParentItem(CaseInteractionParentItem, caseID);
            if (_interactionItem == nullptr) continue;

            for (auto* item : _interactionItem->getChildrenItem())
            {
                int contactID = getPhyIdByItem(item);
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_Contact, contactID });
                QModelIndex idx = _treeModel->indexFromItem(item);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                deleteItems.push_back(proxyIndex);
            }
        }
        if (!deleteItems.isEmpty())
            removeProxyIndexs(deleteItems);

        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_Contact);
        //先通知面板丢弃悬垂指针（此时接触对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyContactDataAboutToBeDestroyed(-1);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_Contact);
        mTreeItem* _interItem = _treeModel->getParentItem(InteractionParentItem);
        auto _contactItem = _treeModel->getParentItem(ContactParentItem);
        removeTreeItem(_contactItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Contact, phyIDs);
        selectedItem(_interItem);
    }

    void mTreeView::slot_menuContactChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _contactItem = _treeModel->getParentItem(ContactParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == ContactChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);

                // 遍历所有工况，查找并删除该接触在所有工况树下的节点
                // 使用遍历父节点子节点的方式，与载荷边界逻辑一致，更可靠
                vector<int> caseIDs = getCVM->getModelData()->getAllPhyID(Physical_Case);
                for (int caseID : caseIDs)
                {
                    mTreeItem* _interactionItem = _treeModel->getParentItem(CaseInteractionParentItem, caseID);
                    if (_interactionItem == nullptr) continue;
                    
                    for (auto* child : _interactionItem->getChildrenItem())
                    {
                        if (getPhyIdByItem(child) == phyID)
                        {
                            QModelIndex idx1 = _treeModel->indexFromItem(child);
                            QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx1);
                            deleteItems.push_back(proxyIndex);
                            // 删除映射关系
                            getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_Contact, phyID });
                            break;
                        }
                    }
                }

                //先通知面板丢弃悬垂指针（此时接触对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyContactDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_Contact, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Contact, phyIDs);

        int conId = getCVM->getModelData()->getAllPhyID(Physical_Contact).size();
        if (conId == 0 && _contactItem != nullptr)
        {
            removeProxyIndexs(deleteItems);
            removeTreeItem(_contactItem);
            selectedItem(_treeModel->getParentItem(InteractionParentItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_contactItem);
        }
    }

    void mTreeView::slot_menuCopyContact()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _contactItem = _treeModel->getParentItem(ContactParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == ContactChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_Contact, getPhyIdByModelIndex(idx));
                auto contact = getCVM->getModelData()->getManager<mContactManager>(Physical_Contact).getContactData(copyId);
                if (contact == nullptr)
                    continue;
                contact->setSuppressed(false);

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), ContactChildItem, contact->getContactName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_contactItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Contact, copyIDs } });
        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_menuCaseInteractionChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        deleteSymbol();

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        mTreeItem* _interactionItem = nullptr;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == CaseInteractionChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                // 获取所属工况ID
                int caseId = getCaseIdByModelIndex(idx);
                // 删除映射关系（统一方向为 Case -> Contact）
                getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseId }, { Physical_Contact, phyID });
                // 发送删除信号更新工况树
                emit mMechGlobalSignals::getInstance()->updateContactTreeView(caseId, phyID, 1);
                deleteItems.push_back(idx);
                _interactionItem = _treeModel->getParentItem(CaseInteractionParentItem, caseId);
            }
        }
        removeProxyIndexs(deleteItems);
        if (_interactionItem) selectedItem(_interactionItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Contact, phyIDs);
    }

    void mTreeView::slot_menuCopyCaseInteraction()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == CaseInteractionChildItem)
            {
                int phyID = getPhyIdByModelIndex(idx);
                int caseId = getCaseIdByModelIndex(idx);
                //复制接触数据
                int copyId = getCVM->getModelData()->copyPhysicalField(Physical_Contact, phyID, -1);
                if (copyId < 0) continue;

                auto& contactMgr = getCVM->getModelData()->getManager<MechContactData::mContactManager>(Physical_Contact);
                auto contactType = contactMgr.getContactData(copyId)->getContactType();

                if (contactType == InterferenceContact)
                {
                    //添加到当前工况
                    getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                        { Physical_Contact, copyId },
                        { Physical_Case, caseId }
                    );
                    emit mMechGlobalSignals::getInstance()->updateContactTreeView(caseId, copyId, 0);
                }
                emit mMechGlobalSignals::getInstance()->updateContactTreeView(-1, copyId, 0);
            }
        }
    }









    void mTreeView::slot_createNote()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)  return;
        if (getTreeTypeByModelIndex(idxs[0]) != ResultParentItem) return;

        int caseId = getPhyIdByModelIndex(idxs[0]);
        mTreeItem* parentItem = _treeModel->getParentItem(ResultParentItem, caseId);

        // 如果已存在注释节点，则不再重复创建
        for (auto& item : parentItem->getChildrenItem())
        {
            if (getTreeTypeByItem(item) == NoteParentItem)
            {
                return;
            }
        }

        // 创建新的注释节点
        QString treeName = "注释";
        mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), NoteParentItem, treeName, 1,
            Tree_Yes, ":/mMainWidgets/Icon/post/XY chart.png");
        _treeModel->appendChildren(parentItem, childitem);
        selectedItem(childitem);

    }

    void mTreeView::slot_createLabel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)  return;
        if (getTreeTypeByModelIndex(idxs[0]) != ResultParentItem) return;
        int caseId = getPhyIdByModelIndex(idxs[0]);
        //注释面板已迁PostWindow::mPostInfoHub，经总线解耦
        emit MechCommon::mMechGlobalSignals::getInstance()->clickCreateNoteSig(caseId);
    }















    void mTreeView::slot_updateResultTree()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)  return;

        BrowserTreeType selectType = getTreeTypeByModelIndex(idxs[0]);
        //右键结果创建云图
        if (selectType == ResultParentItem)
        {
            //工况ID
            int caseID = getPhyIdByModelIndex(idxs[0]);
            mTreeItem* parentItem = _treeModel->getParentItem(ResultParentItem, caseID);

            QString treeName;
            int maxIndex = 0;
            for (auto& item : parentItem->getChildrenItem())
            {
                treeName = getTextByItem(item);
                QString viewport = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByItem(item));
                if (treeName.contains("_") && viewport.contains("Cloud"))
                {
                    QStringList list = treeName.split("_");
                    if (list.size() > 1 && list[1].toInt() > maxIndex)
                    {
                        maxIndex = list[1].toInt();
                    }
                }
            }
            treeName = QString("云图_%1").arg(maxIndex + 1);

            if (getTreeTypeByModelIndex(idxs[0].parent()) == PostTreeRootItem)
            {
                //创建渲染视图
                int modelID = getCVM->getModelData()->getModelDataID();
                int resultID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemId(ItemType::CloudItem);
                QString viewport = QString("Post_%1_Cloud%2").arg(modelID).arg(resultID);
                MechCommon::mMechGlobalSignals::getInstance()->create3DResultViewport(viewport);

                //存储节点数信息
                CloudInfo info;
                info.caseId = -1;
                info.itemName = treeName;
                info.viewName = viewport;
                getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addItem(ItemType::CloudItem, info);

                mTreeItem* item = _treeModel->getParentItem(ResultParentItem, -1);
                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), threeDResultParentItem, treeName, resultID);
                _treeModel->appendChildren(item, childitem);
                selectedItem(childitem);
            }
            //前处理模型树
            else
            {
                int caseID = getPhyIdByModelIndex(idxs[0]);
                //创建渲染视图
                int resultID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemId(ItemType::CloudItem);
                QString viewport = QString("Case%1_Cloud%2").arg(caseID).arg(resultID);
                MechCommon::mMechGlobalSignals::getInstance()->create3DResultViewport(viewport);

                //存储树节点信息
                CloudInfo info;
                info.caseId = caseID;
                info.itemName = treeName;
                info.viewName = viewport;
                getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addItem(ItemType::CloudItem, info);

                mTreeItem* item = _treeModel->getParentItem(ResultParentItem, caseID);
                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), threeDResultParentItem, treeName, resultID);
                _treeModel->appendChildren(item, childitem);
                selectedItem(childitem);
            }
        }
        //打开后处理
        else if (selectType == PostTreeRootItem)
        {
            mTreeItem* resultItem = _treeModel->getParentItem(ResultParentItem, -1);

            QString treeName;
            int maxIndex = 0;

            for (auto& item : resultItem->getChildrenItem())
            {
                treeName = getTextByItem(item);
                QString viewport = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByItem(item));
                if (treeName.contains("_") && viewport.contains("Cloud"))
                {
                    QStringList list = treeName.split("_");
                    if (list.size() > 1 && list[1].toInt() > maxIndex)
                    {
                        maxIndex = list[1].toInt();
                    }
                }
            }

            treeName = QString("云图_%1").arg(maxIndex + 1);

            int resultID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemId(ItemType::CloudItem);

            //打开读文件之前创建的视口
            int modelID = getCVM->getModelData()->getModelDataID();
            QString viewport = QString("Post_Cloud%1").arg(modelID);
            MechCommon::mMechGlobalSignals::getInstance()->create3DResultViewport(viewport);

            //存储节点信息
            CloudInfo info;
            info.caseId = -1;
            info.itemName = treeName;
            info.viewName = viewport;
            info.resultId = resultID;
            getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addItem(ItemType::CloudItem, info);

            mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), threeDResultParentItem, treeName, resultID);
            _treeModel->appendChildren(resultItem, childitem);
            selectedItem(childitem);
        }
        //计算完成后打开后处理
        //else if (selectType == ModelRootItem || (int(selectType) >= CaseImplicitLinearStaticParentItem && int(selectType) < CaseSettingChildItem))
        else  //支持命令行创建云图节点
        {
            mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(_resultCaseID);
            if (caseBase == nullptr)
                return;

            mTreeItem* parentItem = _treeModel->getParentItem(ResultParentItem, _resultCaseID);
            if (parentItem == nullptr)
                return;

            QString treeName;
            int maxIndex = 0;
            for (auto& item : parentItem->getChildrenItem())
            {
                treeName = getTextByItem(item);
                QString viewport = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByItem(item));
                if (treeName.contains("_") && viewport.contains("Cloud"))
                {
                    QStringList list = treeName.split("_");
                    if (list.size() > 1 && list[1].toInt() > maxIndex)
                    {
                        maxIndex = list[1].toInt();
                    }
                }
            }
            treeName = QString("云图_%1").arg(maxIndex + 1);

            //创建渲染视图
            int resultID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemId(ItemType::CloudItem);
            QString viewport = QString("Case%1_Cloud%2").arg(_resultCaseID).arg(resultID);
            MechCommon::mMechGlobalSignals::getInstance()->create3DResultViewport(viewport);

            //存储树节点信息
            CloudInfo info;
            info.caseId = _resultCaseID;
            info.itemName = treeName;
            info.viewName = viewport;
            getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addItem(ItemType::CloudItem, info);

            mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), threeDResultParentItem, treeName, resultID);
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
    }



    void mTreeView::slot_updateStatusPostResult(int modelID, int caseID)
    {
        //树只展示当前模型：结果数据已由执行中心写入所属模型，
        //非当前模型不刷树（切换模型时树会按数据重建）
        if (getCVM == nullptr || getCVM->getModelData()->getModelDataID() != modelID)
            return;

        mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
        if (caseBase == nullptr)
            return;

        mTreeItem* parentItem = _treeModel->getParentItem(ResultParentItem, caseID);
        if (parentItem == nullptr)
            return;

        if (caseBase->getSolveDir().isEmpty())
            setTreeItemStatus(parentItem, Tree_Failure);
        else
            setTreeItemStatus(parentItem, Tree_Yes);
    }

    void mTreeView::slot_importPostResult(int modelID, int caseID)
    {
        //后处理导入链路（信息窗口/结果面板）深度依赖当前模型，
        //结果所属模型非当前模型时不自动导入，避免把结果导进错误的模型
        if (getCVM == nullptr || getCVM->getModelData()->getModelDataID() != modelID)
        {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString("模型（ID:%1）的计算结果已就绪，切换到该模型后可在树上打开结果。").arg(modelID));
            return;
        }
        _resultCaseID = caseID;
        //3D结果面板已迁PostWindow::mPostInfoHub，经总线解耦
        emit MechCommon::mMechGlobalSignals::getInstance()->clickCreate3DResultSig(true, caseID);
    }

    void mTreeView::slot_menuFreqSamplPoint()
    {
        mInfoWindow::getInstance()->creatFREQ();
    }

    void mTreeView::slot_menuFreqSamplPointDelete()
    {
        //删除所有数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_FreqSamplPoint);
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_FreqSamplPoint);
        auto _parentItem = _treeModel->getParentItem(FrequencySamplingPointParentItem);
        removeTreeItem(_parentItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_FreqSamplPoint, phyIDs);
    }

    void mTreeView::slot_menuFreqSamplPointChilDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs;
        int phyID;
        QModelIndexList deleteItems;
        auto _parentItem = _treeModel->getParentItem(FrequencySamplingPointParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == FrequencySamplingPointChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                getCVM->getModelData()->deleteByPhysicalType(Physical_FreqSamplPoint, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        int setId = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getAllFreqSamplPointID().size();
        if (setId == 0 && _parentItem != nullptr)
        {
            removeTreeItem(_parentItem);
            selectedItem(_treeModel->getParentItem(ModelRootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_parentItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_FreqSamplPoint, phyIDs);
    }

    void mTreeView::slot_menuCopyFreqSamplPoint()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _parentItem = _treeModel->getParentItem(FrequencySamplingPointParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == FrequencySamplingPointChildItem)
            {
                copyId = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).copyFreqSamplPointData(getPhyIdByModelIndex(idx));
                mFreqSamplPointBase* freq = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getFreqSamplPointData(copyId);
                if (freq == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), FrequencySamplingPointChildItem, freq->getFreqSamplPointName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_parentItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_FreqSamplPoint, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)  selectedItem(lastItem);
    }

    void mTreeView::slot_updateFreqSamplPointTree(bool isCreate, int freqSamplPointTreeID)
    {
        mFreqSamplPointBase* _freqSamplPointData = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getFreqSamplPointData(freqSamplPointTreeID);
        if (_freqSamplPointData == nullptr)
            return;
        int modelId = getCVM->getModelData()->getModelDataID();
        auto* _FreqSamplPointItem = _treeModel->getParentItem(FrequencySamplingPointParentItem);
        if (_FreqSamplPointItem == nullptr) //无父节点，则创建父节点
        {
            _FreqSamplPointItem = new mTreeItem(modelId, FrequencySamplingPointParentItem, "采样频率点");
            _treeModel->insertChildren(_treeModel->getParentItem(ModelRootItem), _FreqSamplPointItem, getInsertLocation(FrequencySamplingPointParentItem));
        }
        vector<int> IDs;
        if (isCreate)
        {
            auto* childitem = new mTreeItem(modelId, FrequencySamplingPointChildItem, _freqSamplPointData->getFreqSamplPointName(), freqSamplPointTreeID);
            _treeModel->appendChildren(_FreqSamplPointItem, childitem);
            IDs.push_back(freqSamplPointTreeID);
            selectedItem(childitem);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_FreqSamplPoint, freqSamplPointTreeID );
    }

    void mTreeView::slot_menuSglLayerPlate()
    {
        mInfoWindow::getInstance()->creatLaminate(SglLayerPlate);
    }

    void mTreeView::slot_menuSglLayerPlateDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        QModelIndex idx = idxs.back();

        auto seltype = getTreeTypeByModelIndex(idx);

        if (seltype == SglLayerPlateParentItem)//选中节点为单层板父节点才进行删除
        {
            //获取单层板父节点item
            mTreeItem* _parentItem = _treeModel->getParentItem(SglLayerPlateParentItem);
            vector<int> phyIDs;
            QModelIndexList deleteItems; //删除容器

            //循环所有单层板子节点（同时删除单层板和全局单层板，因为两者都在单层板父节点下）
            for (auto& item : _parentItem->getChildrenItem())
            {
                auto itemType = getTreeTypeByItem(item);
                if (itemType != SglLayerPlateChildItem)
                    continue;
                QModelIndex idx = _treeModel->indexFromItem(item);
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);

                //获取子节点id
                int ID = getPhyIdByItem(item);
                //根据id获取数据
                auto _loadData = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getLaminateData(ID);
                //删除数据及映射
                getCVM->getModelData()->deleteByPhysicalType(Physical_Laminate, ID);
                phyIDs.push_back(ID);

                deleteItems.push_back(proxyIndex);
            }
            removeProxyIndexs(deleteItems);
            QModelIndex idx = _treeModel->indexFromItem(_treeModel->getParentItem(SglLayerPlateParentItem));
            QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
            removeProxyIndexs({ proxyIndex });
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Laminate, phyIDs);
        }
    }

    void mTreeView::slot_menuGlobalSglLam()
    {
        mInfoWindow::getInstance()->creatLaminate(GlobalSglLam);
    }

        void mTreeView::slot_menuLamPlate()
        {
            mInfoWindow::getInstance()->creatLaminate(LamPlate);
        }
    
        void mTreeView::slot_menuLamPlateDelete()
        {
            QModelIndexList idxs = getSelectIndex();
            if (idxs.size() == 0)
            {
                return;
            }
    
            QModelIndex idx = idxs.back();
    
            auto seltype = getTreeTypeByModelIndex(idx);
    
            if (seltype == LamPlateParentItem)//选中节点为层合板父节点才进行删除
            {
                //获取层合板父节点item
                mTreeItem* _parentItem = _treeModel->getParentItem(LamPlateParentItem);
                vector<int> phyIDs;
                QModelIndexList deleteItems; //删除容器
    
                //循环所有层合板子节点
                for (auto& item : _parentItem->getChildrenItem())
                {
                    QModelIndex idx = _treeModel->indexFromItem(item);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
    
                    //获取子节点id
                    int ID = getPhyIdByItem(item);
                    //根据id获取数据
                    auto _loadData = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getLaminateData(ID);
                    //删除数据及映射
                    getCVM->getModelData()->deleteByPhysicalType(Physical_Laminate, ID);
                    phyIDs.push_back(ID);
    
                    deleteItems.push_back(proxyIndex);
                }
                removeProxyIndexs(deleteItems);
                QModelIndex idx = _treeModel->indexFromItem(_treeModel->getParentItem(LamPlateParentItem));
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                removeProxyIndexs({ proxyIndex });
                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Laminate, phyIDs);
            }
        }
    
        void mTreeView::slot_menuDeleteAllLaminate()
        {
            // 直接删除所有层合结构，不依赖当前选中节点的 seltype（因为可能在 PropertyParentItem 上触发）
            // 1. 删除单层板父节点下的所有单层板和全局单层板
            mTreeItem* _sglParentItem = _treeModel->getParentItem(SglLayerPlateParentItem);
            if (_sglParentItem)
            {
                vector<int> phyIDs;
                QModelIndexList deleteItems;
                for (auto& item : _sglParentItem->getChildrenItem())
                {
                    auto itemType = getTreeTypeByItem(item);
                    if (itemType != SglLayerPlateChildItem)
                        continue;
                    QModelIndex idx = _treeModel->indexFromItem(item);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                    int ID = getPhyIdByItem(item);
                    getCVM->getModelData()->deleteByPhysicalType(Physical_Laminate, ID);
                    phyIDs.push_back(ID);
                    deleteItems.push_back(proxyIndex);
                }
                if (!deleteItems.isEmpty())
                {
                    removeProxyIndexs(deleteItems);
                    getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Laminate, phyIDs);
                }
            }

            // 2. 删除层合板父节点下的所有层合板
            mTreeItem* _lamParentItem = _treeModel->getParentItem(LamPlateParentItem);
            if (_lamParentItem)
            {
                vector<int> phyIDs;
                QModelIndexList deleteItems;
                for (auto& item : _lamParentItem->getChildrenItem())
                {
                    QModelIndex idx = _treeModel->indexFromItem(item);
                    QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                    int ID = getPhyIdByItem(item);
                    getCVM->getModelData()->deleteByPhysicalType(Physical_Laminate, ID);
                    phyIDs.push_back(ID);
                    deleteItems.push_back(proxyIndex);
                }
                if (!deleteItems.isEmpty())
                {
                    removeProxyIndexs(deleteItems);
                    getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Laminate, phyIDs);
                }
            }

            //删除两个父节点
            QModelIndex idx1 = _treeModel->indexFromItem(_treeModel->getParentItem(SglLayerPlateParentItem));
            QModelIndex proxyIndex1 = _treeProxyModel->mapFromSource(idx1);
            removeProxyIndexs({ proxyIndex1 });

            QModelIndex idx2 = _treeModel->indexFromItem(_treeModel->getParentItem(LamPlateParentItem));
            QModelIndex proxyIndex2 = _treeProxyModel->mapFromSource(idx2);
            removeProxyIndexs({ proxyIndex2 });
        }
    
        void mTreeView::slot_menuLaminateChildDelete()
        {
            QModelIndexList idxs = getSelectIndex();
            if (idxs.size() == 0)
            {
                return;
            }
    
            vector<int> phyIDs;
            int phyID;
            QModelIndexList deleteItems;
    
            for (auto& idx : idxs)
            {
                if (getTreeTypeByModelIndex(idx) == SglLayerPlateChildItem|| getTreeTypeByModelIndex(idx) == LamPlateChildItem)
                {
                    //获取id
                    phyID = getPhyIdByModelIndex(idx);
                    //删除数据及映射
                    getCVM->getModelData()->deleteByPhysicalType(Physical_Laminate, phyID);
                    //放入删除容器
                    deleteItems.push_back(idx);
                    phyIDs.push_back(phyID);
                }
            }
            removeProxyIndexs(deleteItems);

            QStringList  num1 =  getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getAllLaminateByTypes(SglLayerPlate);
            QStringList  num2 =  getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getAllLaminateByTypes(GlobalSglLam);
            QStringList  num3 =  getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getAllLaminateByTypes(LamPlate);

            if (num1.size() == 0 && num2.size() == 0) //删除单层板父节点
            {
                QModelIndex idx = _treeModel->indexFromItem(_treeModel->getParentItem(SglLayerPlateParentItem));
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                removeProxyIndexs({ proxyIndex });
            }

            if (num3.size() == 0) //删除层合板父节点
            {
                QModelIndex idx = _treeModel->indexFromItem(_treeModel->getParentItem(LamPlateParentItem));
                QModelIndex proxyIndex = _treeProxyModel->mapFromSource(idx);
                removeProxyIndexs({ proxyIndex });
            }

            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Laminate, phyIDs);
        }
    
        void mTreeView::slot_menuLaminateChildCopy()
        {
            QModelIndexList idxs = getSelectIndex();
            if (idxs.size() == 0)
            {
                return;
            }
    
            int copyId;
            mTreeItem* lastItem = nullptr;
            vector<int> copyIDs;
            for (auto& idx : idxs)
            {
                if (getTreeTypeByModelIndex(idx) == SglLayerPlateChildItem)
                {
                    copyId = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).copyLaminateData(getPhyIdByModelIndex(idx));
                    mLaminateBase* lamin = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getLaminateData(copyId);
                    if (lamin == nullptr)
                        continue;
                    mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), SglLayerPlateChildItem, lamin->getLaminateName(), copyId);
                    lastItem = childitem;
                    copyIDs.push_back(copyId);
    
                    auto _parentItem = _treeModel->getParentItem(SglLayerPlateParentItem);
                    _treeModel->appendChildren(_parentItem, childitem);
                }
                else if (getTreeTypeByModelIndex(idx) == LamPlateChildItem)
                {
                    copyId = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).copyLaminateData(getPhyIdByModelIndex(idx));
                    mLaminateBase* lamin = getCVM->getModelData()->getManager<mLaminateManager>(Physical_Laminate).getLaminateData(copyId);
                    if (lamin == nullptr)
                        continue;
                    mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), LamPlateChildItem, lamin->getLaminateName(), copyId);
                    lastItem = childitem;
                    copyIDs.push_back(copyId);
    
                    auto _parentItem = _treeModel->getParentItem(LamPlateParentItem);
                    _treeModel->appendChildren(_parentItem, childitem);
                }
            }
    
            getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_Laminate, copyIDs } });
    
            //高亮且点击复制的最后一个
            if (lastItem)  selectedItem(lastItem);
        }
    
        void mTreeView::isDeleteLaminateManagerItem()
        {
            // 父节点现在常驻在截面属性下，不需要删除
        }

    void mTreeView::slot_updateLaminateTree(bool isCreate, int laminateID)
    {
        mLaminateBase* laminate = getCVM->getModelData()->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getLaminateData(laminateID);
        if (laminate == nullptr)
            return;

        //判断类型，单层板，全局单层板，层合板放在截面属性树下的常驻父节点中
        MxFunctions::LaminateType type = laminate->getLaminateType();
        if (type == SglLayerPlate || type == GlobalSglLam || type == LamPlate)
        {
            int modelID = getCVM->getModelData()->getModelDataID();

            if (isCreate)
            {
                switch (type)
                {
                case MxFunctions::SglLayerPlate:
                case MxFunctions::GlobalSglLam:
                {
                    auto childitem = new mTreeItem(modelID, SglLayerPlateChildItem, laminate->getLaminateName(), laminate->getLaminateId());
                    auto _parentItem = _treeModel->getParentItem(SglLayerPlateParentItem);
                    if (!_parentItem) //如果不存在则创建
                    {
                        _parentItem= new mTreeItem(modelID, SglLayerPlateParentItem, "单层板");
                        auto temp = _treeModel->getParentItem(PropertyParentItem);
                        _treeModel->insertChildren(_treeModel->getParentItem(PropertyParentItem), _parentItem, getInsertLocation(SglLayerPlateParentItem));
                    }
                    if (childitem && _parentItem)
                    {
                        _treeModel->appendChildren(_parentItem, childitem);
                    }
                    selectedItem(childitem);
                }
                break;
                case MxFunctions::LamPlate:
                {
                    auto childitem = new mTreeItem(modelID, LamPlateChildItem, laminate->getLaminateName(), laminate->getLaminateId());
                    auto _parentItem = _treeModel->getParentItem(LamPlateParentItem);
                    if (!_parentItem) //如果不存在则创建
                    {
                        _parentItem = new mTreeItem(modelID, LamPlateParentItem, "层合板");
                        auto temp = _treeModel->getParentItem(PropertyParentItem);
                        _treeModel->insertChildren(_treeModel->getParentItem(PropertyParentItem), _parentItem, getInsertLocation(LamPlateParentItem));
                    }

                    if (childitem && _parentItem)
                    {
                        _treeModel->appendChildren(_parentItem, childitem);
                    }
                    selectedItem(childitem);
                }
                break;
                default:
                    break;
                }
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Laminate, laminateID);
    }

    void mTreeView::slot_updateContriTree(bool isCreate, int ID, QString name)
    {
        int modelID = getCVM->getModelData()->getModelDataID();

        auto typePhy = Physical_ModalContri;

        if (ID == 0) //id==0则添加到模型树上
        {
            auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
            auto _contriItem = _treeModel->getParentItem(ContriParentItem);
            if (_contriItem == nullptr && name != "等效辐射功率")
            {
                    _contriItem = new mTreeItem(modelID, ContriParentItem, "贡献量");
                    _treeModel->appendChildren(_treeModel->getParentItem(GlobalParamParentItem), _contriItem);
             }
            if (isCreate)
            {
                BrowserTreeType type;
                if (name == "模态贡献量")
                {
                    type = ModalContriChildItem;
                    typePhy = Physical_ModalContri;
                }
                else if (name == "节点贡献量")
                {
                    type = NodeContriChildItem;
                    typePhy = Physical_NodeContri;
                }
                else if (name == "面板贡献量")
                {
                    type = PanelContriChildItem;
                    typePhy = Physical_PanelContri;
                }
                else if (name == "传递路径贡献量")
                {
                    type = TPContriChildItem;
                    typePhy = Physical_TPContri;
                }
                else if (name == "等效辐射功率")
                {
                    type = ERPParentItem;
                    typePhy = Physical_ERP;
                }
                auto childitem = new mTreeItem(modelID, type, name);
                if (name == "等效辐射功率")
                {
                    _treeModel->appendChildren(_globalItem, childitem);
                    typePhy = Physical_ERP;
                }
                else
                {
                    if (_contriItem == nullptr)
                    {
                        _contriItem = new mTreeItem(modelID, ContriParentItem, "贡献量");
                        _treeModel->appendChildren(_globalItem, _contriItem);
                    }
                    _treeModel->appendChildren(_contriItem, childitem);
                }
                selectedItem(childitem);
            }
        }
        else //否则添加到分析工况下 id既是分析工况id又是模态贡献量id
        {
            //获取分析工况下诊断优化父节点
            auto parentItem = _treeModel->getParentItem(CaseDiagOptParentItem, ID);
            if (parentItem != nullptr)
            {
                BrowserTreeType type;
                auto typePhy = Physical_ModalContri;
                if (name == "模态贡献量")
                {
                    type = CaseModalContriChildItem;
                    typePhy = Physical_ModalContri;
                }
                else if (name == "节点贡献量")
                {
                    type = CaseNodeContriChildItem;
                    typePhy = Physical_NodeContri;
                }
                else if (name == "面板贡献量")
                {
                    type = CasePanelContriChildItem;
                    typePhy = Physical_PanelContri;
                }
                auto childitem = new mTreeItem(modelID, type, name, ID);
                childitem->setCaseID(ID);
                _treeModel->appendChildren(parentItem, childitem);
                selectedItem(childitem);
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(typePhy, ID);
    }

    void mTreeView::slot_updateCasePeakFilterTree(bool isCreate, int caseID)
    {
        if (getCVM->isPostModel()) return;
        if (!isCreate) return;

        int modelID = getCVM->getModelData()->getModelDataID();
        auto parentItem = _treeModel->getParentItem(CaseDiagOptParentItem, caseID);
        if (parentItem == nullptr)
            return;

        // 去重：已存在则不再创建
        for (auto* child : parentItem->getChildrenItem())
        {
            if (getTreeTypeByItem(child) == CasePeakFilterChildItem)
                return;
        }

        auto childitem = new mTreeItem(modelID, CasePeakFilterChildItem, "峰值筛选");
        childitem->setCaseID(caseID);
        _treeModel->appendChildren(parentItem, childitem);
        selectedItem(childitem);
    }

    void mTreeView::slot_menuContriParentDelete()
    {
        std::vector<int> phyIDs;
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        globalParam->deleteModalContriData();
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModalContri, { 0 });
        globalParam->deleteNodeContriData();
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_NodeContri, { 0 });
        globalParam->deletePanelContriData();
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PanelContri, { 0 });
        globalParam->deleteTPContriData();
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_TPContri, { 0 });

        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        auto _contriItem = _treeModel->getParentItem(ContriParentItem);
        removeTreeItem(_contriItem);

        selectedItem(_globalItem);
    }

    void mTreeView::slot_menuContriChildDelete()
    {
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        QModelIndexList deleteItems;
        bool hasModelTreeContri = false;
        for (auto& idx : idxs)
        {
            BrowserTreeType type = getTreeTypeByModelIndex(idx);
            if (type == ModalContriChildItem)
            {
                globalParam->deleteModalContriData();
                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModalContri, { 0 });
                deleteItems.push_back(idx);
                hasModelTreeContri = true;
            }
            else if (type == NodeContriChildItem)
            {
                globalParam->deleteNodeContriData();
                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_NodeContri, { 0 });
                deleteItems.push_back(idx);
                hasModelTreeContri = true;
            }
            else if (type == PanelContriChildItem)
            {
                globalParam->deletePanelContriData();
                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PanelContri, { 0 });
                deleteItems.push_back(idx);
                hasModelTreeContri = true;
            }
            else if (type == TPContriChildItem)
            {
                globalParam->deleteTPContriData();
                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_TPContri, { 0 });
                deleteItems.push_back(idx);
                hasModelTreeContri = true;
            }
            else if (type == CaseModalContriChildItem || type == CaseNodeContriChildItem ||
                     type == CasePanelContriChildItem || type == CaseTPContriChildItem)
            {
                // 工况下的贡献量子节点：获取工况ID并删除对应数据
                int caseId = getCaseIdByModelIndex(idx);
                if (caseId > 0)
                {
                    mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseId);
                    if (caseBase != nullptr)
                    {
                        if (type == CaseModalContriChildItem)
                        {
                            mCaseImplicitFrequency* freqCase = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
                            if (freqCase != nullptr)
                            {
                                freqCase->deleteModalContriData();
                                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModalContri, { freqCase->getCaseId() });
                            }
                            mCaseImplASCoupling* asCouplingCase = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                            if (asCouplingCase != nullptr)
                            {
                                asCouplingCase->deleteModalContriData();
                                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_ModalContri, { asCouplingCase->getCaseId() });
                            }
                        }
                        else if (type == CaseNodeContriChildItem)
                        {
                            mCaseImplASCoupling* asCouplingCase = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                            if (asCouplingCase != nullptr)
                            {
                                asCouplingCase->deleteNodeContriData();
                                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_NodeContri, { asCouplingCase->getCaseId() });
                            }
                        }
                        else if (type == CasePanelContriChildItem)
                        {
                            mCaseImplASCoupling* asCouplingCase = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                            if (asCouplingCase != nullptr)
                            {
                                asCouplingCase->deletePanelContriData();
                                getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_PanelContri, { asCouplingCase->getCaseId() });
                            }
                        }
                    }
                }
                deleteItems.push_back(idx);
            }
        }

        removeProxyIndexs(deleteItems);

        // 仅当删除的是模型树下的贡献量子节点时，才检查并清理父节点
        if (hasModelTreeContri)
        {
            auto _contriItem = _treeModel->getParentItem(ContriParentItem);
            if (_contriItem != nullptr && _contriItem->getChildrenItem().isEmpty())
            {
                removeTreeItem(_contriItem);
            }
        }
        auto _globalItem = _treeModel->getParentItem(GlobalParamParentItem);
        selectedItem(_globalItem);
    }

    void mTreeView::slot_menuDiagOptDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        // 获取诊断优化父节点
        mTreeItem* diagOptItem = getTreeItemByIndex(idxs[0]);
        if (diagOptItem == nullptr)
        {
            return;
        }
        // 获取工况ID（存储在诊断优化节点的phyId中）
        int caseId = getPhyIdByItem(diagOptItem);

        // 删除数据层贡献量
        mCaseBase* caseBase = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseId);
        if (caseBase != nullptr)
        {
            mCaseImplicitFrequency* freqCase = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
            if (freqCase != nullptr)
            {
                freqCase->deleteModalContriData();
            }
            mCaseImplASCoupling* asCouplingCase = dynamic_cast<mCaseImplASCoupling*>(caseBase);
            if (asCouplingCase != nullptr)
            {
                asCouplingCase->deleteModalContriData();
                asCouplingCase->deleteNodeContriData();
                asCouplingCase->deletePanelContriData();
            }
        }

        // 删除工况下峰值筛选映射
        getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseId }, { Physical_PeakFilter ,-1 });

        // 删除所有子节点，保留父节点
        removeTreeChildrenItem(diagOptItem);
        selectedItem(diagOptItem);
    }

    void mTreeView::slot_casePeakFilterDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }
        // 获取选择的工况下峰值筛选节点
        mTreeItem* Item = getTreeItemByIndex(idxs[0]);
        if (Item == nullptr)
        {
            return;
        }
        // 获取工况ID
        int caseId = getPhyIdByItem(Item);
        removeTreeItem(Item);
        // 删除工况下峰值筛选映射
        getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseId }, { Physical_PeakFilter ,-1 });
    }

    void mTreeView::slot_menuBoxHide()
    {
        mInfoWindow::getInstance()->boxHide();
    }

    void mTreeView::slot_menuBoxPreview()
    {
        mInfoWindow::getInstance()->boxPreview();
    }

    void mTreeView::slot_menuBoxShow()
    {
        mInfoWindow::getInstance()->boxShow();
    }

    void mTreeView::slot_menuLocalSizePreview()
    {
        // 查看种子节点前，清空已拾取的几何边高亮，避免残留红边影响预览
        CCSMRendWidget* preRend = getCVM->getPreRenderWidget();
        if (preRend)
        {
            ClearPreWidgetGeoPick(preRend);
            MechPreData::UpdatePreWidgetHighlight(preRend);
            preRend->updatePaint();
        }
        mInfoWindow::getInstance()->localSizePreview();
    }

    mItemKey mTreeView::getItemKeyByIndex(const QModelIndex& proxyIdx)
    {
        if (!proxyIdx.isValid()) return mItemKey();
        QModelIndex idx = toSourceIndex(proxyIdx);
        return  _treeModel->itemKeyFromIndex(idx);
    }

    QList<mItemKey> mTreeView::getItemKeyByIndex(const QModelIndexList& proxyIdx)
    {
        QList<mItemKey> itemKeys;
        itemKeys.reserve(proxyIdx.size());
        for (auto idx : proxyIdx)
        {
            itemKeys.append(getItemKeyByIndex(idx));
        }
        return itemKeys;
    }

    void mTreeView::deSelectItem(MechPreData::mTreeItem* item)
    {
        QModelIndex idx = _treeModel->indexFromItem(item);
        QModelIndex proxyIdx = _treeProxyModel->mapFromSource(idx);
        if (proxyIdx.isValid())
        {
            selectionModel()->select(proxyIdx, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
        }
    }

    void mTreeView::deSelectItem(QVector<MechPreData::mTreeItem*>& items)
    {
        if (items.size() == 0)return;
        QItemSelection deselectSelection;
        QModelIndex idx, proxyIdx;
        for (auto& item : items)
        {
            idx = _treeModel->indexFromItem(item);
            proxyIdx = _treeProxyModel->mapFromSource(idx);
            deselectSelection.select(proxyIdx, proxyIdx);
        }

        if (deselectSelection.size() > 0)
        {
            selectionModel()->select(deselectSelection, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
        }
    }

    bool mTreeView::isItemSelect(MechPreData::mTreeItem* item)
    {
        QModelIndex idx = _treeModel->indexFromItem(item);
        QModelIndex proxyIdx = _treeProxyModel->mapFromSource(idx);
        return getSelectIndex().contains(proxyIdx);
    }

    void mTreeView::expandItem(MechPreData::mTreeItem* item)
    {
        QModelIndex idx = _treeModel->indexFromItem(item);
        QModelIndex proxyIdx = _treeProxyModel->mapFromSource(idx);

        if (proxyIdx.isValid())
        {
            expandItem(proxyIdx);
            this->scrollTo(proxyIdx, QAbstractItemView::PositionAtCenter);
        }
    }

    void mTreeView::expandItem(QModelIndex proxyIdx)
    {
        QModelIndex idx = proxyIdx.parent();
        if (idx.isValid())
        {
            expandItem(idx);
        }
        this->expand(proxyIdx);
    }

    QModelIndex mTreeView::toSourceIndex(const QModelIndex& idx) const
    {
        if (!idx.isValid()) return QModelIndex();
        if (idx.model() == _treeModel) return idx;     //已是源模型索引（后处理树外部派发）
        return _treeProxyModel->mapToSource(idx);      //本视图代理索引
    }

    int mTreeView::getPhyIdByModelIndex(QModelIndex idx)
    {
        if (!idx.isValid()) return -1;
        return _treeModel->data(toSourceIndex(idx), Roles::PhyidRole).toInt();
    }

    int mTreeView::getPhyIdByItem(MechPreData::mTreeItem* item)
    {
        if (item == nullptr) return -1;
        return _treeModel->data(_treeModel->indexFromItem(item), Roles::PhyidRole).toInt();
    }

    int mTreeView::getModelIdByModelIndex(QModelIndex idx)
    {
        if (!idx.isValid()) return -1;
        return _treeModel->data(toSourceIndex(idx), Roles::ModelRole).toInt();
    }

    int mTreeView::getModelIdByItem(MechPreData::mTreeItem* item)
    {
        if (item == nullptr) return -1;
        return _treeModel->data(_treeModel->indexFromItem(item), Roles::ModelRole).toInt();
    }

    int mTreeView::getCaseIdByModelIndex(QModelIndex proxyIdx)
    {
        if (!proxyIdx.isValid()) return -1;
        return _treeModel->data(toSourceIndex(proxyIdx), Roles::CaseidRole).toInt();
    }

    int mTreeView::getCaseIdByItem(MechPreData::mTreeItem* item)
    {
        if (item == nullptr) return -1;
        return _treeModel->data(_treeModel->indexFromItem(item), Roles::CaseidRole).toInt();
    }

    QString mTreeView::getTextByModelIndex(QModelIndex idx)
    {
        if (!idx.isValid()) return "";
        return _treeModel->data(toSourceIndex(idx), Qt::DisplayRole).toString();
    }

    QString mTreeView::getTextByItem(MechPreData::mTreeItem* item)
    {
        if (item == nullptr)  return "";
        return _treeModel->data(_treeModel->indexFromItem(item), Qt::DisplayRole).toString();
    }

    MxFunctions::BrowserTreeType mTreeView::getTreeTypeByModelIndex(QModelIndex idx)
    {
        if (!idx.isValid()) return ModelRootItem;
        return (BrowserTreeType)_treeModel->data(toSourceIndex(idx), Roles::TypeRole).toInt();
    }

    MxFunctions::BrowserTreeType mTreeView::getTreeTypeByItem(MechPreData::mTreeItem* item)
    {
        if (item == nullptr)  return ModelRootItem;
        return (BrowserTreeType)_treeModel->data(_treeModel->indexFromItem(item), Roles::TypeRole).toInt();
    }

    MxFunctions::BrowserTreeStatus mTreeView::getStatusByModelIndex(QModelIndex idx)
    {
        if (!idx.isValid()) return Tree_Yes;
        return (BrowserTreeStatus)_treeModel->data(toSourceIndex(idx), Roles::StatusRole).toInt();
    }


    MxFunctions::BrowserTreeStatus mTreeView::getStatusByItem(MechPreData::mTreeItem* item)
    {
        if (item == nullptr) return Tree_Yes;
        return (BrowserTreeStatus)_treeModel->data(_treeModel->indexFromItem(item), Roles::StatusRole).toInt();
    }

    QString mTreeView::getIconStringByModelIndex(QModelIndex idx)
    {
        if (!idx.isValid()) return "";
        return _treeModel->data(toSourceIndex(idx), Roles::Icon2Role).toString();
    }

    QString mTreeView::getIconStringByItem(MechPreData::mTreeItem* item)
    {
        if (item == nullptr) return "";
        return _treeModel->data(_treeModel->indexFromItem(item), Roles::Icon2Role).toString();
    }

    bool mTreeView::getHiddenByModelIndex(QModelIndex idx)
    {
        if (!idx.isValid()) return false;
        return _treeModel->data(toSourceIndex(idx), Roles::HiddenRole).toBool();
    }

    bool mTreeView::getHiddenByItem(MechPreData::mTreeItem* item)
    {
        if (item == nullptr) return false;
        return _treeModel->data(_treeModel->indexFromItem(item), Roles::HiddenRole).toBool();
    }

    MechPreData::mTreeItem* mTreeView::getTreeItemByIndex(QModelIndex proxyIdx)
    {
        if (!proxyIdx.isValid()) return nullptr;
        return _treeModel->itemFromIndex(toSourceIndex(proxyIdx));
    }

    void mTreeView::setTreeItemStatus(MechPreData::mTreeItem* item, MxFunctions::BrowserTreeStatus stauts)
    {
        QModelIndex idx = _treeModel->indexFromItem(item);
        _treeModel->setData(idx, (int)stauts, Roles::StatusRole);
    }

    void mTreeView::setTreeItemPic(MechPreData::mTreeItem* item, QString picString)
    {
        QModelIndex idx = _treeModel->indexFromItem(item);
        _treeModel->setData(idx, picString, Roles::Icon2Role);
    }

    void mTreeView::setTreeItemText(MechPreData::mTreeItem* item, QString text)
    {
        QModelIndex idx = _treeModel->indexFromItem(item);
        _treeModel->setData(idx, text, Qt::DisplayRole);
    }

    void mTreeView::setTreeItemHidden(MechPreData::mTreeItem* item, bool hidden)
    {
        QModelIndex idx = _treeModel->indexFromItem(item);
        _treeModel->setData(idx, hidden, Roles::HiddenRole);
    }

    void mTreeView::removeProxyIndexs(QModelIndexList proxyIndexs)
    {
        if (proxyIndexs.isEmpty()) return;

        QModelIndex index;
        QModelIndexList modelIndexs;
        for (int i = 0; i < proxyIndexs.size(); i++)
        {
            index =  toSourceIndex(proxyIndexs[i]);
            modelIndexs.push_back(index);
        }

        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
        _treeModel->removeIndexs(modelIndexs);
        connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);

    }

    void mTreeView::removeTreeItem(mTreeItem* item)
    {
        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
        _treeModel->removeTreeItem(item);
        connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
    }

    void mTreeView::removeTreeChildrenItem(mTreeItem* item)
    {
        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
        _treeModel->removeTreeChildrenItem(item);
        connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &mTreeView::slot_itemSelectionChanged);
    }

    void mTreeView::removeModelRootItem(int modelDataID)
    {
        //前处理模型根存在于本树（前处理模型实例）；后处理模型根在后处理树的独立模型中，
        //经总线请求后处理树移除（隔离约束：本树不得直接操作后处理树模型）
        mItemKey key;
        key._modelID = modelDataID;
        key._treeType = ModelRootItem;
        if (mTreeItem* rootItem = _treeModel->treeItemFromKey(key))
        {
            removeTreeItem(rootItem);
        }
        //顶层多体动力学根与模型根同生命周期：随模型一并移除，避免残留隐根
        mItemKey motionKey;
        motionKey._modelID = modelDataID;
        motionKey._treeType = Motion_RootItem;
        if (mTreeItem* motionRootItem = _treeModel->treeItemFromKey(motionKey))
        {
            removeTreeItem(motionRootItem);
        }
        //后处理模型在前处理树也有同名根（slot_updateModelTree 按当前模型无条件
        //AddModelTree 建过），移除后不得提前返回：仍须通知后处理树移除其后处理根，
        //否则二次导入会话（清空重建）时旧后处理根残留，与新根并列成双根
        if (VMManager->isPostModel(modelDataID))
        {
            emit mMechGlobalSignals::getInstance()->removePostModelTreeSig(modelDataID);
        }
    }

    int mTreeView::getChildrenSize(QModelIndex idx)
    {
        if (!idx.isValid()) return 0;
        return _treeModel->rowCount(toSourceIndex(idx));
    }

    bool mTreeView::isCaseParentItem(QModelIndex proxyIdx)
    {
        auto seltype = getTreeTypeByModelIndex(proxyIdx);
        //分析工况节点
        if (seltype >= CaseImplicitLinearStaticParentItem && seltype < CaseSettingChildItem)
        {
            return true;
        }

        return false;
    }

    QItemSelectionModel::SelectionFlags mTreeView::selectionCommand(const QModelIndex& index, const QEvent* event) const
    {
        auto flags = QTreeView::selectionCommand(index, event);
        if (!event || !index.isValid()) return flags;

        if (event->type() != QEvent::MouseButtonPress &&
            event->type() != QEvent::MouseButtonDblClick &&
            event->type() != QEvent::MouseButtonRelease &&
            event->type() != QEvent::MouseMove)
            return flags;

        const auto* me = static_cast<const QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton && me->button() != Qt::RightButton && me->button() != Qt::NoButton)
            return flags;

        const QPoint vpPos = me->pos();

        // 1) 点箭头：放行（叶子节点时 branchHitRect() 应返回空矩形）
        const QRect br = branchHitRect(index);
        if (br.isValid() && br.contains(vpPos))
            return flags;

        // 2) 只允许点到 delegate 内容区域才选中
        QStyleOptionViewItem opt = viewOptions();
        opt.rect = visualRect(index);
        if (!opt.rect.contains(vpPos))
            return QItemSelectionModel::NoUpdate;

        const auto* del = qobject_cast<const mTreeDelegate*>(itemDelegate(index));
        if (!del) del = qobject_cast<const mTreeDelegate*>(itemDelegate());
        if (!del) return flags; // 没有自定义delegate就退化为默认整行可选

        if (!del->hitTest(opt, index, vpPos))
            return QItemSelectionModel::NoUpdate;

        if (me->button() == Qt::LeftButton &&
            (me->modifiers() & Qt::ControlModifier))
        {
            return QItemSelectionModel::Toggle | QItemSelectionModel::Rows;
        }
        else if (me->button() == Qt::LeftButton &&
            (me->modifiers() & Qt::ShiftModifier))
        {
            return QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows | QItemSelectionModel::Current;
        }
        else if (me->button() == Qt::RightButton)
        {
            MechCommon::mMechGlobalSignals::getInstance()->_isTreeRight = true;
            if (selectionModel()->isSelected(index))
            {
                return QItemSelectionModel::NoUpdate;
            }
        }

        return flags;
    }

    void mTreeView::mousePressEvent(QMouseEvent* event)
    {
        // 部件子节点：点击几何/网格图标直接切换显隐，不触发选中
        if (event->type() == QEvent::MouseButtonPress && event->button() == Qt::LeftButton)
        {
            QModelIndex index = indexAt(event->pos());
            if (index.isValid() && getTreeTypeByModelIndex(index) == ModelChildItem)
            {
                auto* del = qobject_cast<mTreeDelegate*>(itemDelegate(index));
                if (!del) del = qobject_cast<mTreeDelegate*>(itemDelegate());
                if (del)
                {
                    QStyleOptionViewItem opt = viewOptions();
                    opt.rect = visualRect(index);
                    const auto area = del->hitTestArea(opt, index, event->pos());
                    if (area == mTreeDelegate::HitArea::IconGeo || area == mTreeDelegate::HitArea::IconMesh)
                    {
                        togglePartDisplayByIcon(index, area == mTreeDelegate::HitArea::IconGeo);
                        return;
                    }
                }
            }
        }
        QTreeView::mousePressEvent(event);
    }

    void mTreeView::leaveEvent(QEvent* event)
    {
        viewport()->unsetCursor();
        QTreeView::leaveEvent(event);
    }

    void mTreeView::updatePartDisplayIcon(int modelID, const std::set<int>& partIDs)
    {
        for (int partID : partIDs)
        {
            mTreeItem* item = _treeModel->getChildrenItem(ModelChildItem, partID, -1, modelID);
            if (item == nullptr) continue;
            QModelIndex idx = _treeModel->indexFromItem(item);
            QModelIndex proxyIdx = _treeProxyModel->mapFromSource(idx);
            if (proxyIdx.isValid())
                viewport()->update(visualRect(proxyIdx)); // 仅重绘该节点行
        }
    }

    void mTreeView::togglePartDisplayByIcon(const QModelIndex& clickedProxyIdx, bool isGeoIcon)
    {
        if (getCVM == nullptr || getCVM->getModelData() == nullptr) return;

        // 点击节点属于其他模型时，先切换到该模型（与普通点击节点的行为一致）
        if (getModelIdByModelIndex(clickedProxyIdx) != getCVM->getModelData()->getModelDataID())
        {
            updateModelView(clickedProxyIdx);
            if (getCVM == nullptr || getCVM->getModelData() == nullptr) return;
        }

        // 收集目标部件：点击项在选中集合内则对所有选中的部件子节点生效，否则仅点击项
        std::set<int> partIDs;
        bool inSelection = false;
        QModelIndexList selIdxs = getSelectIndex();
        for (auto& idx : selIdxs)
        {
            if (idx == clickedProxyIdx) { inSelection = true; break; }
        }
        if (inSelection)
        {
            for (auto& idx : selIdxs)
            {
                if (getTreeTypeByModelIndex(idx) == ModelChildItem)
                    partIDs.insert(getPhyIdByModelIndex(idx));
            }
        }
        else
        {
            partIDs.insert(getPhyIdByModelIndex(clickedProxyIdx));
        }
        if (partIDs.empty()) return;

        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        if (meshData == nullptr) return;

        // 目标部件全部可见则统一隐藏，任一隐藏则统一显示
        bool allVisible = true;
        for (int partID : partIDs)
        {
            bool hidden = isGeoIcon ? meshData->getGeoPartMask(partID) : meshData->getMeshPartMask(partID);
            if (hidden) { allVisible = false; break; }
        }
        const bool toHide = allVisible;

        for (int partID : partIDs)
        {
            if (isGeoIcon) meshData->setGeoPartMask(partID, toHide);
            else meshData->setMeshPartMask(partID, toHide);
        }

        const mxr::ModelOperateEnum operateType = toHide ? mxr::HideOnePart : mxr::ShowOnePart;
        if (isGeoIcon)
            getCVM->updateAllGeoModelOperate(operateType, partIDs);
        else
            getCVM->updateAllMeshModelOperate(operateType);

        if (toHide)
        {
            // 仅移除被隐藏部件的选中/包围盒，保留其他仍可见部件的高亮（含3D视图中的拾取）
            std::vector<int> partIDsVec(partIDs.begin(), partIDs.end());
            MechPreData::reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), partIDsVec);
            UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->updateRender();
        }
        else
        {
            hightPart();
        }

        updatePartDisplayIcon(getCVM->getModelData()->getModelDataID(), partIDs);
    }

    void mTreeView::slot_deleteGeoTree(std::set<int> partIds)
    {
        int modelId = getCVM->getModelData()->getModelDataID();
        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet(partIds);
        getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
        MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);

        for (int id : partIds)
        {
            auto item = _treeModel->getChildrenItem(ModelChildItem, id, -1, modelId);
            if (item)
            {
                _treeModel->removeTreeItem(item);
            }
        }
    }
    void mTreeView::createMotionRootMenu()
    {
        QAction* action = nullptr;
        //新建运动体
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_movingBodyParentItem.png"), "新建运动体");
        connect(action, &QAction::triggered, this, [this]() {
            slot_menuMotionBodyNew();
            });

        //新建运动副
        auto actionProperty = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_connectorParentItem.png"), "新建运动副");
        QMenu* menuCreateProperty = new QMenu();
        actionProperty->setMenu(menuCreateProperty);
        auto addPropertyAction = [&](const QString& icon, const QString& text, int type) {
            action = menuCreateProperty->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionJointNew(JointType(type));
                });
            };
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_revolute.png", "旋转副", 0);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_prismatic.png", "移动副", 1);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_cylindrical.png", "柱面副", 2);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_spherical.png", "球面副", 3);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_fixed.png", "固定副", 4);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_universal.png", "万向节", 5);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_pointPlane.png", "点面副", 6);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_parallel.png", "平行约束", 8);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_perpendicular.png", "垂直约束", 9);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_direction.png", "方向约束", 10);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_cv.png", "恒速副", 12);
        //        //胡克副(13)/关联副(16)/齿轮副(17)：遗留枚举或已由耦合副体系承接，入口移除
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_screw.png", "螺纹副", 14);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_planar.png", "平面副", 15);
        
        
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_pointLine.png", "点线副", 18);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_pointOnCurve.png", "点在线上副", 19);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_pointOnSurface.png", "点在面上副", 20);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_curveOnCurve.png", "线在线上副", 21);

        //创建多体耦合副
        auto actionMotionCoupling = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_couplingParentItem.png"), "新建耦合副");
        QMenu* menuCreateMotionCoupling = new QMenu();
        actionMotionCoupling->setMenu(menuCreateMotionCoupling);
        auto addMotionCouplingAction = [&](const QString& icon, const QString& text, MxFunctions::CouplingType type) {
            action = menuCreateMotionCoupling->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionCouplingNew(type);
                });
            };
        addMotionCouplingAction(":/mMainWidgets/Icon/Motion/coupling_gearRack.png", "齿轮齿条副", CouplingType::GearRack);
        addMotionCouplingAction(":/mMainWidgets/Icon/Motion/joint_gear.png", "齿轮副", CouplingType::Gear);

        //新建驱动
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_driveParentItem.png"), "新建驱动");
        connect(action, &QAction::triggered, this, [this]() {
            slot_menuMotionDriveNew(DriveTargetType::Joint);
            });

        //新建标记点
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/caotu/dian.png"), "新建标记点");
        connect(action, &QAction::triggered, this, [this]() {
            slot_menuMotionMarkerNew();
            });

        //新建传感器
        auto actionMotionSensor = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/sensor.png"), "新建传感器");
        QMenu* menuCreateMotionSensor = new QMenu();
        actionMotionSensor->setMenu(menuCreateMotionSensor);
        auto addMotionSensorAction = [&](const QString& icon, const QString& text, MechMotionData::SensorType type) {
            action = menuCreateMotionSensor->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createMotionSensor(type);
                });
            };
        addMotionSensorAction(":/mMainWidgets/Icon/Motion/sensor_displacement.png", "体位移传感器", MechMotionData::SensorType::BodyDisplacement);
        addMotionSensorAction(":/mMainWidgets/Icon/Motion/sensor_velocity.png", "体速度传感器", MechMotionData::SensorType::BodyVelocity);
        addMotionSensorAction(":/mMainWidgets/Icon/Motion/sensor_acceleration.png", "体加速度传感器", MechMotionData::SensorType::BodyAcceleration);
        addMotionSensorAction(":/mMainWidgets/Icon/Motion/sensor_jointReaction.png", "关节反力传感器", MechMotionData::SensorType::JointReaction);
        addMotionSensorAction(":/mMainWidgets/Icon/Motion/sensor_driveActuation.png", "驱动输出传感器", MechMotionData::SensorType::DriveActuation);
        addMotionSensorAction(":/mMainWidgets/Icon/Motion/sensor_distance.png", "距离传感器", MechMotionData::SensorType::Distance);
        addMotionSensorAction(":/mMainWidgets/Icon/Motion/sensor_angle.png", "角度传感器", MechMotionData::SensorType::Angle);

        //新建控制器
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/controlCalculation.png"), "新建控制器");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMotionController();
            });

        //新建力元
        auto actionLiYuan = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_liYuanParentItem.png"), "新建力元");
        QMenu* menuCreateLiYuan = new QMenu();
        actionLiYuan->setMenu(menuCreateLiYuan);
        auto addLiYuanAction = [&](const QString& icon, const QString& text, MechMotionData::VectorLoadType type) {
            action = menuCreateLiYuan->addAction(QIcon(icon), text);
            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createMotionVectorForce(type);
                });
            };
        addLiYuanAction(":/mMainWidgets/Icon/Motion/force_vector.png", "矢量力", MechMotionData::VectorLoadType::Force);
        addLiYuanAction(":/mMainWidgets/Icon/Motion/force_torque.png", "矢量力矩", MechMotionData::VectorLoadType::Torque);
        addLiYuanAction(":/mMainWidgets/Icon/Motion/bush.png", "衬套", MechMotionData::VectorLoadType::Bushing);
        //单向力/单向力矩/一般作用力/重力/力场/线性弹簧阻尼/轮胎力：求解装配不传输，子菜单移除
        addLiYuanAction(":/mMainWidgets/Icon/Motion/force_tri.png", "三点力", MechMotionData::VectorLoadType::TriForce);
        addLiYuanAction(":/mMainWidgets/Icon/Motion/force_tri_torque.png", "三点力矩", MechMotionData::VectorLoadType::TriTorque);
        menuCreateLiYuan->addSeparator();
        action = menuCreateLiYuan->addAction(QIcon(":/mMainWidgets/Icon/Motion/force.png"), "标量力");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::ScalarForce);
            });

        action = menuCreateLiYuan->addAction(QIcon(":/mMainWidgets/Icon/Motion/moment.png"), "标量力矩");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::ScalarTorque);
            });

        // 新建弹簧（拉压/扭转）
        auto actionSpring = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/force_spring.png"), "新建弹簧");
        QMenu* menuCreateSpring = new QMenu();
        actionSpring->setMenu(menuCreateSpring);
        auto addSpringAction = [&](const QString& icon, const QString& text, MechMotionData::SpringType type) {
            action = menuCreateSpring->addAction(QIcon(icon), text);
            connect(action, &QAction::triggered, this, [this, type]() {
                mInfoWindow::getInstance()->unEditInfoWindow();
                mInfoWindow::getInstance()->createMotionSpring(static_cast<int>(type));
                });
            };
        addSpringAction(":/mMainWidgets/Icon/Motion/tcSpring.png", "拉压弹簧", MechMotionData::SpringType::Translational);
        addSpringAction(":/mMainWidgets/Icon/Motion/torsionSpring.png", "扭转弹簧", MechMotionData::SpringType::Torsional);
        //体点弹簧补入（与弹簧分组右键/菜单栏"柔性连接"组对齐：图标同体点弹簧入口）
        addSpringAction(":/mMainWidgets/Icon/Motion/force_spring.png", "体点弹簧", MechMotionData::SpringType::BodyPoint);

        //求解设置
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_solveParentItem.png"), "新建求解方案");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMotionScenario();
            });

        //求解
        //action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_solveParentItem.png"), "求解");
        //connect(action, &QAction::triggered, this, [this]() {
        //    mInfoWindow::getInstance()->unEditInfoWindow();
        //    //mInfoWindow::getInstance()->createCoordinate();
        //    });

        //删除运动数据：多体分支整树清空（部件/几何/网格保留）——同一 STP 换 JSON
        //工况重导时免去逐父分组删除；各类型删除链复用对应"删除所有X"槽
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除运动数据");
        connect(action, &QAction::triggered, this, [this]() {
            slot_menuMotionRootDeleteAll();
            });

        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionBodyParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建运动体");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionBodyNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有运动体");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionBodyDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionBodyChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionBodyChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionBody()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionJointParentMenu()
    {
        QAction* action = nullptr;
        auto actionProperty = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_connectorParentItem.png"), "新建运动副");
        QMenu* menuCreateProperty = new QMenu();
        actionProperty->setMenu(menuCreateProperty);
        auto addPropertyAction = [&](const QString& icon, const QString& text, int type) {
            action = menuCreateProperty->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionJointNew(JointType(type));
                });
            };
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_revolute.png", "旋转副", 0);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_prismatic.png", "移动副", 1);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_cylindrical.png", "柱面副", 2);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_spherical.png", "球面副", 3);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_fixed.png", "固定副", 4);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_universal.png", "万向节", 5);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_pointPlane.png", "点面副", 6);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_parallel.png", "平行约束", 8);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_perpendicular.png", "垂直约束", 9);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_direction.png", "方向约束", 10);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_cv.png", "恒速副", 12);
        //addPropertyAction(":/mMainWidgets/Icon/Motion/joint_hooke.png", "胡克副", 13);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_screw.png", "螺纹副", 14);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_planar.png", "平面副", 15);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_pointLine.png", "点线副", 18);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_pointOnCurve.png", "点在线上副", 19);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_pointOnSurface.png", "点在面上副", 20);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_curveOnCurve.png", "线在线上副", 21);

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有运动副");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionJointDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionJointChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionJointChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionJoint()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionCouplingParentMenu()
    {
        QAction* action = nullptr;
        auto actionProperty = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_connectorParentItem.png"), "新建耦合副");
        QMenu* menuCreateProperty = new QMenu();
        actionProperty->setMenu(menuCreateProperty);
        auto addPropertyAction = [&](const QString& icon, const QString& text, int type) {
            action = menuCreateProperty->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionCouplingNew(MxFunctions::CouplingType(type));
                });
            };
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_gear.png", "齿轮齿条副", 0);
        addPropertyAction(":/mMainWidgets/Icon/Motion/joint_gear.png", "齿轮副", 1);
        addPropertyAction(":/mMainWidgets/Icon/Motion/coupling_cable.png", "线缆副", 2);

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有耦合副");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionCouplingDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionCouplingChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionCouplingChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionCoupling()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionDriveParentMenu()
    {
        QAction* action = nullptr;
        auto actionProperty = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/motion_driveParentItem.png"), "新建驱动");
        QMenu* menuCreateProperty = new QMenu();
        actionProperty->setMenu(menuCreateProperty);
        auto addPropertyAction = [&](const QString& icon, const QString& text, int type) {
            action = menuCreateProperty->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionDriveNew(DriveTargetType(type));
                });
            };
        addPropertyAction(":/mMainWidgets/Icon/Motion/rotaryJoint.png", "运动副驱动", 0);
        //addPropertyAction(":/mMainWidgets/Icon/Motion/parallel.png", "运动体驱动", 1);
        //addPropertyAction(":/mMainWidgets/Icon/Motion/FixedDeputy.png", "固定副", 4);

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有驱动");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionDriveDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionDriveChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionDriveChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionDrive()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionMarkerParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/caotu/dian.png"), "新建标记点");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionMarkerNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有标记点");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionMarkerDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionMarkerChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionMarkerChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionMarker()));
        m_Menu->exec(QCursor::pos());    }

    void mTreeView::createMotionVectorForceParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/force_vector.png"), "新建矢量力");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionVectorForceNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/force_torque.png"), "新建矢量力矩");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionVectorTorqueNew()));
        m_Menu->addSeparator();
        //衬套（轴套）已归柔性连接族，创建入口在"弹簧"分组菜单
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/force_tri.png"), "新建三点力");
        connect(action, &QAction::triggered, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::TriForce); });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/force_tri_torque.png"), "新建三点力矩");
        connect(action, &QAction::triggered, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::TriTorque); });

        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/force.png"), "新建标量力");
        connect(action, &QAction::triggered, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::ScalarForce); });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/moment.png"), "新建标量力矩");
        connect(action, &QAction::triggered, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::ScalarTorque); });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/bush.png"), "新建衬套");
        connect(action, &QAction::triggered, this, [this]() { mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::Bushing); });
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有力元");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionVectorForceDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionVectorForceChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionVectorForceChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionVectorForce()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionScenarioParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionScenarioDelete()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "求解");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_solveMotionScenario()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionControlInputParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建控制输入");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionControlInputNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有控制输入");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionControlInputDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionControlInputChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionControlInputChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionControlInput()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionElectroImportParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建机电导入");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionElectroImportNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有机电导入");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionElectroImportDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionElectroImportChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionElectroImportChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionElectroImport()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionElectroExportParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建机电导出");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionElectroExportNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有机电导出");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionElectroExportDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionElectroExportChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionElectroExportChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionElectroExport()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionHydraulicParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建液压协同");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionHydraulicNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有液压协同");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionHydraulicDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionHydraulicChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionHydraulicChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionHydraulic()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionRLParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建强化学习");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionRLNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有强化学习");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionRLDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionRLChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionRLChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionRL()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionSensorParentMenu()
    {
        QAction* action = nullptr;
        //新建传感器：按类型铺子菜单（全类型直建，同运动副子菜单模式）
        auto actionSensorNew = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建传感器");
        QMenu* menuCreateSensor = new QMenu();
        actionSensorNew->setMenu(menuCreateSensor);
        auto addSensorAction = [&](const QString& icon, const QString& text, MechMotionData::SensorType type) {
            action = menuCreateSensor->addAction(QIcon(icon), text);

            connect(action, &QAction::triggered, this, [this, type]() {
                slot_menuMotionSensorNew(type);
                });
            };
        addSensorAction(":/mMainWidgets/Icon/Motion/sensor_displacement.png", "体位移传感器", MechMotionData::SensorType::BodyDisplacement);
        addSensorAction(":/mMainWidgets/Icon/Motion/sensor_velocity.png", "体速度传感器", MechMotionData::SensorType::BodyVelocity);
        addSensorAction(":/mMainWidgets/Icon/Motion/sensor_acceleration.png", "体加速度传感器", MechMotionData::SensorType::BodyAcceleration);
        addSensorAction(":/mMainWidgets/Icon/Motion/sensor_jointReaction.png", "关节反力传感器", MechMotionData::SensorType::JointReaction);
        addSensorAction(":/mMainWidgets/Icon/Motion/sensor_driveActuation.png", "驱动输出传感器", MechMotionData::SensorType::DriveActuation);
        addSensorAction(":/mMainWidgets/Icon/Motion/sensor_distance.png", "距离传感器", MechMotionData::SensorType::Distance);
        addSensorAction(":/mMainWidgets/Icon/Motion/sensor_angle.png", "角度传感器", MechMotionData::SensorType::Angle);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有传感器");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionSensorDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionSensorChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionSensorChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionSensor()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionControllerParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建控制器");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionControllerNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有控制器");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionControllerDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionControllerChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionControllerChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionController()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::slot_menuMotionBodyNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionBody();
    }

    //多体根"删除运动数据"：清空多体动力学分支全部对象（含求解方案及其结果），
    //部件/几何/网格保留——同一 STP 换 JSON 工况重导的整树复位入口。
    //各类型删除链复用对应"删除所有X"槽（符号/映射/信息表口径不另写一份）；
    //求解方案无清空式槽（既有为选中单删），此处补齐
    void mTreeView::slot_menuMotionRootDeleteAll()
    {
        const int btn = MechMainWindow::mFramelessMessageBox::warning(this, QString::fromUtf8("警告"),
            QString::fromUtf8("将删除多体动力学下的全部运动数据（含求解方案及其结果），部件/几何/网格保留。\n是否继续？"),
            QString::fromUtf8("是"),
            QString::fromUtf8("否"));
        if (btn != 0)
            return;

        //先退出运动编辑会话：编辑中的面板持数据指针，先删数据会悬垂
        mInfoWindow::getInstance()->unEditInfoWindow();

        //求解方案（浮动挂多体根下）：数据 + 浮动节点一并清。
        //节点类型走模型索引 TypeRole（mTreeItem::key 私有仅 mTreeModel 友元可及）
        {
            vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionScenario);
            //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
            mInfoWindow::getInstance()->notifyMotionScenarioDataAboutToBeDestroyed();
            getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionScenario);
            mTreeItem* rootItem = _treeModel->getParentItem(Motion_RootItem);
            const QModelIndex motionRootIdx = _treeModel->indexFromItem(rootItem);
            QModelIndexList deleteItems;
            for (int r = 0; r < _treeModel->rowCount(motionRootIdx); ++r)
            {
                const QModelIndex srcIdx = _treeModel->index(r, 0, motionRootIdx);
                if (srcIdx.data(Roles::TypeRole).toInt() == Motion_ScenarioParentItem)
                {
                    const QModelIndex proxyIdx = _treeProxyModel->mapFromSource(srcIdx);
                    if (proxyIdx.isValid())
                        deleteItems.push_back(proxyIdx);
                }
            }
            removeProxyIndexs(deleteItems);
            getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionScenario, phyIDs);
        }

        //其余按依赖序复用各分组"删除所有X"槽：传感/控制 → 接触/弹簧/力元/耦合副/
        //驱动/运动副 → 标记点 → 柔性体 → 运动体（末位，运动副等引用它）；
        //机电导入导出/液压/强化学习/控制输入为独立数据，末尾清
        slot_menuMotionSensorDelete();
        slot_menuMotionControllerDelete();
        slot_menuMotionContactDelete();
        slot_menuMotionSpringDelete();
        slot_menuMotionVectorForceDelete();
        slot_menuMotionCouplingDelete();
        slot_menuMotionDriveDelete();
        slot_menuMotionJointDelete();
        slot_menuMotionMarkerDelete();
        slot_menuMotionAutoFlexDelete();
        slot_menuMotionBodyDelete();
        slot_menuMotionElectroImportDelete();
        slot_menuMotionElectroExportDelete();
        slot_menuMotionHydraulicDelete();
        slot_menuMotionRLDelete();
        slot_menuMotionControlInputDelete();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString::fromUtf8("已删除全部运动数据（部件/几何/网格保留）"));
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionBodyDelete()
    {
        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionBody);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionBodyDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionBody);
        mTreeItem* item = _treeModel->getParentItem(Motion_MovingBodyParentItem);
        //固定分组常驻：清空分组下对象节点，保留分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionBody, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionBodyChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_MovingBodyParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_MovingBodyChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionBodyDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionBody, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionBody, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionBody()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(Motion_MovingBodyParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_MovingBodyChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionBody, getPhyIdByModelIndex(idx));
                mMotionBody* property = getCVM->getModelData()->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody).getData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_MovingBodyChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionBody, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionBodyTree(bool isCreate, int ID)
    {
        mMotionBody* motionBody = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).getData(ID);
        if (motionBody == nullptr)
            return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* motionBodyRootItem = _treeModel->getParentItem(Motion_MovingBodyParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_MovingBodyChildItem, motionBody->getName(), ID, MxFunctions::Tree_Yes, getMotionBodyPic(motionBody->getIsFixed()));
            _treeModel->appendChildren(motionBodyRootItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_MovingBodyChildItem, ID);
            if (cusItem != nullptr)            {
                QString pic;
                pic = getMotionBodyPic(motionBody->getIsFixed());
                if (getIconStringByItem(cusItem) != pic)
                {
                    setTreeItemPic(cusItem, pic);
                }
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionBody, ID);
    }

    void mTreeView::slot_updateMotionBodyTreeBatch(QVector<int> IDs)
    {
        if (IDs.isEmpty())
            return;
        auto& manager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);

        //节点一次建齐（appendChildren 批量口径），不再逐体 appendChildren+selectedItem：
        //单个路径每次选中伴随展开/滚动/面板刷新与信息表重建，N 个体 N 次即为批量卡顿主源
        mTreeItem* motionBodyRootItem = _treeModel->getParentItem(Motion_MovingBodyParentItem);
        QVector<mTreeItem*> childItems;
        childItems.reserve(IDs.size());
        mTreeItem* lastItem = nullptr;
        for (int id : IDs)
        {
            mMotionBody* motionBody = manager.getData(id);
            if (motionBody == nullptr)
                continue;
            auto* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_MovingBodyChildItem,
                motionBody->getName(), id, MxFunctions::Tree_Yes, getMotionBodyPic(motionBody->getIsFixed()));
            childItems.push_back(childitem);
            lastItem = childitem;
        }
        if (childItems.isEmpty())
            return;
        _treeModel->appendChildren(motionBodyRootItem, std::move(childItems));
        //选中/滚动仅末尾一次
        selectedItem(lastItem);
        //状态+输出信息表批量一次（updateDynamicInfoData 的 QHash 批量口径）
        getCVM->getDataStatusManager()->updateDynamicInfoData(
            { { Physical_MotionBody, IDs.toStdVector() } });
    }

    QString mTreeView::getMotionBodyPic(bool isFixed)
    {
        QString pic = ":/mMainWidgets/Icon/Motion/movingBody.png";
        if (isFixed)
        {
            pic = ":/mMainWidgets/Icon/Motion/FixedDeputy.png";
        }
        return pic;
    }

    void mTreeView::slot_menuMotionJointNew(MxFunctions::JointType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionJoint(type);
    }

    void mTreeView::slot_menuMotionJointDelete()
    {
        //删除数据
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionJoint);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionJointDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionJoint);
        //固定分组常驻：清空分组下对象节点，保留"运动副"分组本身
        mTreeItem* item = _treeModel->getParentItem(Motion_ConnectorParentItem);
        _treeModel->removeTreeChildrenItem(item);
        //原实现误删运动体的动态信息数据（复制粘贴笔误），此处一并纠正
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionJoint, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionJointChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_ConnectorParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ConnectorChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionJointDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionJoint, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionJoint, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionJoint()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(Motion_ConnectorParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ConnectorChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionJoint, getPhyIdByModelIndex(idx));
                mMotionJointBase* property = getCVM->getModelData()->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint).getData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ConnectorChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionJoint, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionJointTree(bool isCreate, int ID)
    {
        mMotionJointBase* motionJoint = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(ID);
        if (motionJoint == nullptr)
            return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* motionJointRootItem = _treeModel->getParentItem(Motion_ConnectorParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ConnectorChildItem, motionJoint->getName(), ID, MxFunctions::Tree_Yes, getMotionJointPic(motionJoint->getType()));
            _treeModel->appendChildren(motionJointRootItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_ConnectorChildItem, ID);
            if (cusItem != nullptr)
            {
                QString pic;
                pic = getMotionJointPic(motionJoint->getType());
                if (getIconStringByItem(cusItem) != pic)
                {
                    setTreeItemPic(cusItem, pic);
                }
                setTreeItemText(cusItem, motionJoint->getName());
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionJoint, ID);
    }

    void mTreeView::slot_menuMotionCouplingNew(MxFunctions::CouplingType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionCoupling(type);
    }

    void mTreeView::slot_menuMotionCouplingDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionCoupling);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionCouplingDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionCoupling);
        //固定分组常驻：清空分组下对象节点，保留"耦合副"分组本身
        mTreeItem* item = _treeModel->getParentItem(Motion_CouplingParentItem);
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionCoupling, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionCouplingChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_CouplingParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_CouplingChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionCouplingDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionCoupling, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionCoupling, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionCoupling()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(Motion_CouplingParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_CouplingChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionCoupling, getPhyIdByModelIndex(idx));
                mMotionCouplingBase* property = getCVM->getModelData()->getManager<MechMotionData::mMotionCouplingManager>(Physical_MotionCoupling).getData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_CouplingChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionCoupling, copyIDs } });

        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionCouplingTree(bool isCreate, int ID)
    {
        mMotionCouplingBase* motionCoupling = getCVM->getModelData()->getManager<mMotionCouplingManager>(Physical_MotionCoupling).getData(ID);
        if (motionCoupling == nullptr)
            return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* motionCouplingRootItem = _treeModel->getParentItem(Motion_CouplingParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_CouplingChildItem, motionCoupling->getName(), ID, MxFunctions::Tree_Yes, getMotionCouplingPic(motionCoupling->getType()));
            _treeModel->appendChildren(motionCouplingRootItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_CouplingChildItem, ID);
            if (cusItem != nullptr)
            {
                setTreeItemText(cusItem, motionCoupling->getName());
                QString pic = getMotionCouplingPic(motionCoupling->getType());
                if (getIconStringByItem(cusItem) != pic)
                {
                    setTreeItemPic(cusItem, pic);
                }
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionCoupling, ID);
    }

    QString mTreeView::getMotionJointPic(MxFunctions::JointType type)
    {
        QString pic;
        switch (type)
        {
        case MxFunctions::JointType::Revolute:
            pic = ":/mMainWidgets/Icon/Motion/joint_revolute.png"; break;
        case MxFunctions::JointType::Translational:
            pic = ":/mMainWidgets/Icon/Motion/joint_prismatic.png"; break;
        case MxFunctions::JointType::Cylindrical:
            pic = ":/mMainWidgets/Icon/Motion/joint_cylindrical.png"; break;
        case MxFunctions::JointType::Spherical:
            pic = ":/mMainWidgets/Icon/Motion/joint_spherical.png"; break;
        case MxFunctions::JointType::Fixed:
            pic = ":/mMainWidgets/Icon/Motion/joint_fixed.png"; break;
        case MxFunctions::JointType::Universal:
            pic = ":/mMainWidgets/Icon/Motion/joint_universal.png"; break;
        case MxFunctions::JointType::PointPlane:
            pic = ":/mMainWidgets/Icon/Motion/joint_pointPlane.png"; break;
        case MxFunctions::JointType::Line:
            pic = ":/mMainWidgets/Icon/Motion/joint_line.png"; break;
        case MxFunctions::JointType::Parallel:
            pic = ":/mMainWidgets/Icon/Motion/joint_parallel.png"; break;
        case MxFunctions::JointType::Perpendicular:
            pic = ":/mMainWidgets/Icon/Motion/joint_perpendicular.png"; break;
        case MxFunctions::JointType::Direction:
            pic = ":/mMainWidgets/Icon/Motion/joint_direction.png"; break;
        case MxFunctions::JointType::Centripetal:
            pic = ":/mMainWidgets/Icon/Motion/joint_centripetal.png"; break;
        case MxFunctions::JointType::ConstantVelocity:
            pic = ":/mMainWidgets/Icon/Motion/joint_cv.png"; break;
        case MxFunctions::JointType::Hooke:
            pic = ":/mMainWidgets/Icon/Motion/joint_hooke.png"; break;
        case MxFunctions::JointType::Screw:
            pic = ":/mMainWidgets/Icon/Motion/joint_screw.png"; break;
        case MxFunctions::JointType::Planar:
            pic = ":/mMainWidgets/Icon/Motion/joint_planar.png"; break;
        case MxFunctions::JointType::Coupling:
            pic = ":/mMainWidgets/Icon/Motion/joint_coupling.png"; break;
        case MxFunctions::JointType::Gear:
            pic = ":/mMainWidgets/Icon/Motion/joint_gear.png"; break;
        case MxFunctions::JointType::PointLine:
            pic = ":/mMainWidgets/Icon/Motion/joint_pointLine.png"; break;
        case MxFunctions::JointType::PointOnCurve:
            pic = ":/mMainWidgets/Icon/Motion/joint_pointLine.png"; break;
        case MxFunctions::JointType::PointOnSurface:
            pic = ":/mMainWidgets/Icon/Motion/joint_pointPlane.png"; break;
        case MxFunctions::JointType::CurveOnCurve:
            pic = ":/mMainWidgets/Icon/Motion/joint_pointLine.png"; break;
        }
        return pic;
    }

    QString mTreeView::getMotionCouplingPic(MxFunctions::CouplingType type)
    {
        QString pic;
        switch (type)
        {
        case MxFunctions::CouplingType::GearRack:
            pic = ":/mMainWidgets/Icon/Motion/coupling_gearRack.png"; break;
        case MxFunctions::CouplingType::Gear:
            pic = ":/mMainWidgets/Icon/Motion/coupling_gear.png"; break;
        case MxFunctions::CouplingType::Cable:
            pic = ":/mMainWidgets/Icon/Motion/coupling_cable.png"; break;
        default:
            pic = ":/mMainWidgets/Icon/BrowserTree/motion_couplingParentItem.png"; break;
        }
        return pic;
    }

    QString mTreeView::getMotionSensorPic(MechMotionData::SensorType type)
    {
        QString pic;
        switch (type)
        {
        case MechMotionData::SensorType::BodyDisplacement:
            pic = ":/mMainWidgets/Icon/Motion/sensor_displacement.png"; break;
        case MechMotionData::SensorType::BodyVelocity:
            pic = ":/mMainWidgets/Icon/Motion/sensor_velocity.png"; break;
        case MechMotionData::SensorType::BodyAcceleration:
            pic = ":/mMainWidgets/Icon/Motion/sensor_acceleration.png"; break;
        case MechMotionData::SensorType::JointReaction:
            pic = ":/mMainWidgets/Icon/Motion/sensor_jointReaction.png"; break;
        case MechMotionData::SensorType::DriveActuation:
            pic = ":/mMainWidgets/Icon/Motion/sensor_driveActuation.png"; break;
        case MechMotionData::SensorType::Distance:
            pic = ":/mMainWidgets/Icon/Motion/sensor_distance.png"; break;
        case MechMotionData::SensorType::Angle:
            pic = ":/mMainWidgets/Icon/Motion/sensor_angle.png"; break;
        default:
            pic = ":/mMainWidgets/Icon/Motion/sensor.png"; break;
        }
        return pic;
    }

    void mTreeView::slot_menuMotionDriveNew(MxFunctions::DriveTargetType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionDrive(type);
    }

    void mTreeView::slot_menuMotionDriveDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionDrive);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionDriveDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionDrive);
        mTreeItem* item = _treeModel->getParentItem(Motion_DriveParentItem);
        //固定分组常驻：清空分组下对象节点，保留"驱动"分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionDrive, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionDriveChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_DriveParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_DriveChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionDriveDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionDrive, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionDrive, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionDrive()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        auto _propertyItem = _treeModel->getParentItem(Motion_DriveParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_DriveChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionDrive, getPhyIdByModelIndex(idx));
                mMotionDriveBase* property = getCVM->getModelData()->getManager<MechMotionData::mMotionDriveManager>(Physical_MotionDrive).getData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_DriveChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionDrive, copyIDs } });

        //高亮且点击复制的最后一个
        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionDriveTree(bool isCreate, int ID)
    {
        mMotionDriveBase* motionDrive = getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive).getData(ID);
        if (motionDrive == nullptr)
            return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* motionDriveRootItem = _treeModel->getParentItem(Motion_DriveParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_DriveChildItem, motionDrive->getName(), ID, MxFunctions::Tree_Yes, getMotionDrivePic(motionDrive->getTargetType()));
            _treeModel->appendChildren(motionDriveRootItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_DriveChildItem, ID);
            if (cusItem != nullptr)
            {
                QString pic;
                pic = getMotionDrivePic(motionDrive->getTargetType());
                if (getIconStringByItem(cusItem) != pic)
                {
                    setTreeItemPic(cusItem, pic);
                }
                setTreeItemText(cusItem, motionDrive->getName());
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionDrive, ID);
    }

    QString mTreeView::getMotionDrivePic(MxFunctions::DriveTargetType type)
    {
        QString pic;
        switch (type)
        {
        case MxFunctions::DriveTargetType::Joint:
            pic = ":/mMainWidgets/Icon/Motion/drive.png"; break;
        case MxFunctions::DriveTargetType::Body:
            pic = ":/mMainWidgets/Icon/Motion/drive.png"; break;
        }
        return pic;
    }

    void mTreeView::slot_menuMotionScenarioNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionScenario();
    }

    void mTreeView::slot_menuMotionScenarioDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
        {
            return;
        }

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_RootItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ScenarioParentItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionScenarioDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionScenario, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);

        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionScenario, phyIDs);
    }

    void mTreeView::slot_menuMotionScenarioChildDel()
    {

    }

    void mTreeView::slot_updateMotionScenarioTree(bool isCreate, int ID)
    {
        mMotionScenario* motionScenario = getCVM->getModelData()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(ID);
        if (motionScenario == nullptr)
            return;

        //求解方案对象节点直接浮动挂多体根下（不设固定父分组）
        mTreeItem* motionRootItem = _treeModel->getParentItem(Motion_RootItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ScenarioParentItem, motionScenario->getName(), ID, MxFunctions::Tree_Yes);
            _treeModel->appendChildren(motionRootItem, childitem);

            auto animationitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_AnimationChildItem, QString("动画"), ID, MxFunctions::Tree_Yes);
            _treeModel->appendChildren(childitem, animationitem);

            auto loadExportItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_LoadExportChildItem, QString("结果导出"), ID, MxFunctions::Tree_Yes);
            _treeModel->appendChildren(childitem, loadExportItem);

            auto curveSelItem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_CurveChildItem, QString("运动曲线"), ID, MxFunctions::Tree_Yes);
            _treeModel->appendChildren(childitem, curveSelItem);

            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_AnimationChildItem, ID);
            if (cusItem != nullptr)
            {
                //QString pic;
                //pic = getMotionJointPic(motionDrive->getTargetType());
                //if (getIconStringByItem(cusItem) != pic)
                //{
                //    setTreeItemPic(cusItem, pic);
                //}
                //setTreeItemText(cusItem, motionDrive->getName());
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionScenario, ID);
    }

    void mTreeView::slot_menuMotionMarkerNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionMarker();
    }

    void mTreeView::slot_menuMotionMarkerDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionMarker);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionMarkerDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionMarker);
        mTreeItem* item = _treeModel->getParentItem(Motion_MarkerParentItem);
        //固定分组常驻：清空分组下对象节点，保留"标记点"分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionMarker, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionMarkerChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
            return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_MarkerParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_MarkerChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionMarkerDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionMarker, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionMarker, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionMarker()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
            return;

        auto _propertyItem = _treeModel->getParentItem(Motion_MarkerParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_MarkerChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionMarker, getPhyIdByModelIndex(idx));
                mMotionMarker* property = getCVM->getModelData()->getManager<MechMotionData::mMotionMarkerManager>(Physical_MotionMarker).getData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_MarkerChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionMarker, copyIDs } });

        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionMarkerTree(bool isCreate, int ID)
    {
        mMotionMarker* motionMarker = getCVM->getModelData()->getManager<mMotionMarkerManager>(Physical_MotionMarker).getData(ID);
        if (motionMarker == nullptr)
            return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* motionMarkerRootItem = _treeModel->getParentItem(Motion_MarkerParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_MarkerChildItem, motionMarker->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/caotu/dian.png");
            _treeModel->appendChildren(motionMarkerRootItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_MarkerChildItem, ID);
            if (cusItem != nullptr)
            {
                setTreeItemText(cusItem, motionMarker->getName());
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionMarker, ID);
    }

    void mTreeView::slot_menuMotionVectorForceNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::Force);
    }

    void mTreeView::slot_menuMotionVectorTorqueNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionVectorForce(VectorLoadType::Torque);
    }

    void mTreeView::slot_menuMotionVectorForceDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionVectorForce);
        //先通知面板丢弃悬垂指针（此时力元对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyVectorForceDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionVectorForce);
        mTreeItem* item = _treeModel->getParentItem(Motion_LiYuanParentItem);
        //固定分组常驻：清空分组下对象节点，保留"力元"分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionVectorForce, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionVectorForceChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
            return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_LiYuanParentItem);
        for (auto& idx : idxs)
        {
            int treeType = getTreeTypeByModelIndex(idx);
            if (treeType == Motion_LiYuanChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时力元对象仍有效），再真正删除数据对象
                //——否则面板刷新链 dynamic_cast 读已释放对象抛 no-RTTI 崩溃（衬套实测）
                mInfoWindow::getInstance()->notifyVectorForceDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionVectorForce, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionVectorForce, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionVectorForce()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)
            return;

        auto _propertyItem = _treeModel->getParentItem(Motion_LiYuanParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            int treeType = getTreeTypeByModelIndex(idx);
            if (treeType == Motion_LiYuanChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionVectorForce, getPhyIdByModelIndex(idx));
                MechMotionData::mMotionForceElement* property = getCVM->getModelData()->getManager<MechMotionData::mMotionForceElementManager>(Physical_MotionVectorForce).getData(copyId);
                if (property == nullptr)
                    continue;

                QString icon = getForceElementPic(property->getLoadType());
                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_LiYuanChildItem, property->getName(), copyId, MxFunctions::Tree_Yes, icon);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionVectorForce, copyIDs } });

        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionVectorForceTree(bool isCreate, int ID)
    {
        MechMotionData::mMotionForceElement* forceElement = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getData(ID);
        if (forceElement == nullptr)
            return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* vectorForceRootItem = _treeModel->getParentItem(Motion_LiYuanParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_LiYuanChildItem, forceElement->getName(), ID, MxFunctions::Tree_Yes, getForceElementPic(forceElement->getLoadType()));
            _treeModel->appendChildren(vectorForceRootItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_LiYuanChildItem, ID);
            if (cusItem != nullptr)
            {
                // 参照运动副策略：类型切换后图标随载荷类型刷新，名称同步
                QString pic = getForceElementPic(forceElement->getLoadType());
                if (getIconStringByItem(cusItem) != pic)
                {
                    setTreeItemPic(cusItem, pic);
                }
                setTreeItemText(cusItem, forceElement->getName());
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionVectorForce, ID);
    }

    void mTreeView::createMotionContactParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(""), "新建 3D 接触");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionContactNew()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有接触");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionContactDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionContactChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionContactChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionItemChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionContact()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::slot_menuMotionContactNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionContact(0);
    }

    void mTreeView::slot_menuMotionContactDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionContact);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionContactDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionContact);
        mTreeItem* item = _treeModel->getParentItem(Motion_ContactParentItem);
        //固定分组常驻：清空分组下对象节点，保留分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionContact, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionContactChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        int phyID = -1;
        vector<int> phyIDs;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_ContactParentItem);
        for (const auto& idx : idxs)
        {
            if (idx.isValid())
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionContactDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionContact, phyID);
                deleteItems.push_back(idx);
            }
        }
        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionContact, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionContact()
    {
        QModelIndexList idxs = getSelectIndex();
        auto _propertyItem = _treeModel->getParentItem(Motion_ContactParentItem);
        int copyId = -1;
        vector<int> copyIDs;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        for (const auto& idx : idxs)
        {
            if (idx.isValid())
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionContact, getPhyIdByModelIndex(idx));
                mMotionContact* property = getCVM->getModelData()->getManager<MechMotionData::mMotionContactManager>(Physical_MotionContact).getData(copyId);
                if (property == nullptr)
                    continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ContactChildItem, property->getName(), copyId);

                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionContact, copyIDs } });

        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::createMotionSpringParentMenu()
    {
        QAction* action = nullptr;
        //图标与菜单栏"柔性连接"组/多体根"新建弹簧"子菜单同源（tcSpring/torsionSpring）
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/tcSpring.png"), "新建拉压弹簧");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionSpringNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/torsionSpring.png"), "新建扭转弹簧");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionSpringNewTorsion()));
        //体点弹簧（与菜单栏"柔性连接"组对齐）
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/Motion/force_spring.png"), "新建体点弹簧");
        connect(action, &QAction::triggered, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createMotionSpring(static_cast<int>(MechMotionData::SpringType::BodyPoint));
            });
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有弹簧");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionSpringDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionSpringChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionSpringChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionItemChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionSpring()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::slot_menuMotionSpringNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionSpring(static_cast<int>(MechMotionData::SpringType::Translational));
    }

    void mTreeView::slot_menuMotionSpringNewTorsion()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionSpring(static_cast<int>(MechMotionData::SpringType::Torsional));
    }

    void mTreeView::slot_menuMotionSpringDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionSpring);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionSpringDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionSpring);
        mTreeItem* item = _treeModel->getParentItem(Motion_SpringParentItem);
        //固定分组常驻：仅移除弹簧对象节点，保留分组本身；
        //衬套节点（柔性连接族）与弹簧混挂本组，不在弹簧删除范围
        for (int id : phyIDs)
        {
            if (mTreeItem* springChild = _treeModel->getChildrenItem(Motion_SpringChildItem, id))
            {
                _treeModel->removeTreeItem(springChild);
            }
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionSpring, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionSpringChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        int phyID = -1;
        vector<int> phyIDs;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_SpringParentItem);
        for (const auto& idx : idxs)
        {
            if (idx.isValid())
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionSpringDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionSpring, phyID);
                deleteItems.push_back(idx);
            }
        }
        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionSpring, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionSpring()
    {
        QModelIndexList idxs = getSelectIndex();
        auto _propertyItem = _treeModel->getParentItem(Motion_SpringParentItem);
        int copyId = -1;
        vector<int> copyIDs;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        for (const auto& idx : idxs)
        {
            if (idx.isValid())
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionSpring, getPhyIdByModelIndex(idx));
                mMotionSpringBase* property = getCVM->getModelData()->getManager<MechMotionData::mMotionSpringManager>(Physical_MotionSpring).getData(copyId);
                if (property == nullptr)
                    continue;
                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_SpringChildItem, property->getName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }
        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionSpring, copyIDs } });
        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionSpringTree(bool isCreate, int ID)
    {
        mMotionSpringBase* spring = getCVM->getModelData()->getManager<mMotionSpringManager>(Physical_MotionSpring).getData(ID);
        if (spring == nullptr)
            return;
        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* springRootItem = _treeModel->getParentItem(Motion_SpringParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_SpringChildItem, spring->getName(), ID, MxFunctions::Tree_Yes, getMotionSpringPic(spring->getType()));
            _treeModel->appendChildren(springRootItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_SpringChildItem, ID);
            if (cusItem != nullptr)
            {
                QString pic;
                pic = getMotionSpringPic(spring->getType());
                if (getIconStringByItem(cusItem) != pic)
                {
                    setTreeItemPic(cusItem, pic);
                }
                setTreeItemText(cusItem, spring->getName());
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionSpring, ID);
    }

    QString mTreeView::getMotionSpringPic(MechMotionData::SpringType type)
    {
        QString pic;
        switch (type)
        {
        case MechMotionData::SpringType::Translational:
            pic = ":/mMainWidgets/Icon/Motion/tcSpring.png"; break;
        case MechMotionData::SpringType::Torsional:
            pic = ":/mMainWidgets/Icon/Motion/torsionSpring.png"; break;
        case MechMotionData::SpringType::BodyPoint:
            pic = ":/mMainWidgets/Icon/Motion/force_spring.png"; break;
        }
        return pic;
    }

    void mTreeView::slot_updateMotionContactTree(bool isCreate, int ID)
    {
        mMotionContact* contact = getCVM->getModelData()->getManager<mMotionContactManager>(Physical_MotionContact).getData(ID);
        if (contact == nullptr)
            return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* contactRootItem = _treeModel->getParentItem(Motion_ContactParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ContactChildItem, contact->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/Motion/contact.png");
            _treeModel->appendChildren(contactRootItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_ContactChildItem, ID);
            if (cusItem != nullptr)
            {
                setTreeItemText(cusItem, contact->getName());
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionContact, ID);
    }

    void mTreeView::slot_menuMotionControlInputNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionControlInput();
    }

    void mTreeView::slot_menuMotionControlInputDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionControlInput);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionControlInputDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionControlInput);
        mTreeItem* item = _treeModel->getParentItem(Motion_ControlInputParentItem);
        //固定分组常驻：清空分组下对象节点，保留分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionControlInput, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionControlInputChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_ControlInputParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ControlInputChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionControlInputDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionControlInput, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionControlInput, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionControlInput()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        auto _propertyItem = _treeModel->getParentItem(Motion_ControlInputParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ControlInputChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionControlInput, getPhyIdByModelIndex(idx));
                mMotionControlInput* property = getCVM->getModelData()->getManager<MechMotionData::mMotionControlInputManager>(Physical_MotionControlInput).getData(copyId);
                if (property == nullptr) continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ControlInputChildItem, property->getName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionControlInput, copyIDs } });
        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionControlInputTree(bool isCreate, int ID)
    {
        mMotionControlInput* data = getCVM->getModelData()->getManager<mMotionControlInputManager>(Physical_MotionControlInput).getData(ID);
        if (data == nullptr) return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* parentItem = _treeModel->getParentItem(Motion_ControlInputParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ControlInputChildItem, data->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/BrowserTree/motion_driveParentItem.png");
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_ControlInputChildItem, ID);
            if (cusItem != nullptr)
                setTreeItemText(cusItem, data->getName());
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionControlInput, ID);
    }

    void mTreeView::slot_menuMotionElectroImportNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionElectroImport();
    }

    void mTreeView::slot_menuMotionElectroImportDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionElectroImport);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionElectroImportDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionElectroImport);
        //动态分组：随全部对象一并移除分组本身
        mTreeItem* item = _treeModel->getParentItem(Motion_ElectroImportParentItem);
        _treeModel->removeTreeItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionElectroImport, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionElectroImportChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_ElectroImportParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ElectroImportChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionElectroImportDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionElectroImport, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //动态分组：删空后拆除父分组
        int num = getCVM->getModelData()->getManager<MechMotionData::mMotionElectroImportManager>(Physical_MotionElectroImport).getAllIDs().size();
        if (num == 0 && _propertyItem != nullptr)
        {
            _treeModel->removeTreeItem(_propertyItem);
            selectedItem(_treeModel->getParentItem(Motion_RootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_propertyItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionElectroImport, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionElectroImport()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        auto _propertyItem = _treeModel->getParentItem(Motion_ElectroImportParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ElectroImportChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionElectroImport, getPhyIdByModelIndex(idx));
                mMotionElectroImport* property = getCVM->getModelData()->getManager<MechMotionData::mMotionElectroImportManager>(Physical_MotionElectroImport).getData(copyId);
                if (property == nullptr) continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ElectroImportChildItem, property->getName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionElectroImport, copyIDs } });
        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionElectroImportTree(bool isCreate, int ID)
    {
        mMotionElectroImport* data = getCVM->getModelData()->getManager<mMotionElectroImportManager>(Physical_MotionElectroImport).getData(ID);
        if (data == nullptr) return;

        int modelID = getCVM->getModelData()->getModelDataID();
        mTreeItem* parentItem = _treeModel->getParentItem(Motion_ElectroImportParentItem);
        //动态分组：首对象到达时懒建父分组（删空由 ChildDel 菜单拆除）
        if (parentItem == nullptr)
        {
            parentItem = new mTreeItem(modelID, Motion_ElectroImportParentItem, "机电导入");
            _treeModel->appendChildren(_treeModel->getParentItem(Motion_RootItem), parentItem);
        }
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ElectroImportChildItem, data->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/BrowserTree/motion.png");
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_ElectroImportChildItem, ID);
            if (cusItem != nullptr)
                setTreeItemText(cusItem, data->getName());
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionElectroImport, ID);
    }

    void mTreeView::slot_menuMotionElectroExportNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionElectroExport();
    }

    void mTreeView::slot_menuMotionElectroExportDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionElectroExport);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionElectroExportDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionElectroExport);
        //动态分组：随全部对象一并移除分组本身
        mTreeItem* item = _treeModel->getParentItem(Motion_ElectroExportParentItem);
        _treeModel->removeTreeItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionElectroExport, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionElectroExportChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_ElectroExportParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ElectroExportChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionElectroExportDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionElectroExport, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //动态分组：删空后拆除父分组
        int num = getCVM->getModelData()->getManager<MechMotionData::mMotionElectroExportManager>(Physical_MotionElectroExport).getAllIDs().size();
        if (num == 0 && _propertyItem != nullptr)
        {
            _treeModel->removeTreeItem(_propertyItem);
            selectedItem(_treeModel->getParentItem(Motion_RootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_propertyItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionElectroExport, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionElectroExport()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        auto _propertyItem = _treeModel->getParentItem(Motion_ElectroExportParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ElectroExportChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionElectroExport, getPhyIdByModelIndex(idx));
                mMotionElectroExport* property = getCVM->getModelData()->getManager<MechMotionData::mMotionElectroExportManager>(Physical_MotionElectroExport).getData(copyId);
                if (property == nullptr) continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ElectroExportChildItem, property->getName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionElectroExport, copyIDs } });
        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionElectroExportTree(bool isCreate, int ID)
    {
        mMotionElectroExport* data = getCVM->getModelData()->getManager<mMotionElectroExportManager>(Physical_MotionElectroExport).getData(ID);
        if (data == nullptr) return;

        int modelID = getCVM->getModelData()->getModelDataID();
        mTreeItem* parentItem = _treeModel->getParentItem(Motion_ElectroExportParentItem);
        //动态分组：首对象到达时懒建父分组（删空由 ChildDel 菜单拆除）
        if (parentItem == nullptr)
        {
            parentItem = new mTreeItem(modelID, Motion_ElectroExportParentItem, "机电导出");
            _treeModel->appendChildren(_treeModel->getParentItem(Motion_RootItem), parentItem);
        }
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ElectroExportChildItem, data->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/BrowserTree/motion.png");
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_ElectroExportChildItem, ID);
            if (cusItem != nullptr)
                setTreeItemText(cusItem, data->getName());
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionElectroExport, ID);
    }

    void mTreeView::slot_menuMotionHydraulicNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionHydraulic();
    }

    void mTreeView::slot_menuMotionHydraulicDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionHydraulic);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionHydraulicDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionHydraulic);
        //动态分组：随全部对象一并移除分组本身
        mTreeItem* item = _treeModel->getParentItem(Motion_HydraulicParentItem);
        _treeModel->removeTreeItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionHydraulic, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionHydraulicChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_HydraulicParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_HydraulicChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionHydraulicDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionHydraulic, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //动态分组：删空后拆除父分组
        int num = getCVM->getModelData()->getManager<MechMotionData::mMotionHydraulicManager>(Physical_MotionHydraulic).getAllIDs().size();
        if (num == 0 && _propertyItem != nullptr)
        {
            _treeModel->removeTreeItem(_propertyItem);
            selectedItem(_treeModel->getParentItem(Motion_RootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_propertyItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionHydraulic, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionHydraulic()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        auto _propertyItem = _treeModel->getParentItem(Motion_HydraulicParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_HydraulicChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionHydraulic, getPhyIdByModelIndex(idx));
                mMotionHydraulic* property = getCVM->getModelData()->getManager<MechMotionData::mMotionHydraulicManager>(Physical_MotionHydraulic).getData(copyId);
                if (property == nullptr) continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_HydraulicChildItem, property->getName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionHydraulic, copyIDs } });
        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionHydraulicTree(bool isCreate, int ID)
    {
        mMotionHydraulic* data = getCVM->getModelData()->getManager<mMotionHydraulicManager>(Physical_MotionHydraulic).getData(ID);
        if (data == nullptr) return;

        int modelID = getCVM->getModelData()->getModelDataID();
        mTreeItem* parentItem = _treeModel->getParentItem(Motion_HydraulicParentItem);
        //动态分组：首对象到达时懒建父分组（删空由 ChildDel 菜单拆除）
        if (parentItem == nullptr)
        {
            parentItem = new mTreeItem(modelID, Motion_HydraulicParentItem, "液压协同");
            _treeModel->appendChildren(_treeModel->getParentItem(Motion_RootItem), parentItem);
        }
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_HydraulicChildItem, data->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/BrowserTree/motion.png");
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_HydraulicChildItem, ID);
            if (cusItem != nullptr)
                setTreeItemText(cusItem, data->getName());
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionHydraulic, ID);
    }

    void mTreeView::slot_menuMotionRLNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionRL();
    }

    void mTreeView::slot_menuMotionRLDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionRL);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionRLDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionRL);
        //动态分组：随全部对象一并移除分组本身
        mTreeItem* item = _treeModel->getParentItem(Motion_RLParentItem);
        _treeModel->removeTreeItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionRL, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionRLChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_RLParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_RLChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionRLDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionRL, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //动态分组：删空后拆除父分组
        int num = getCVM->getModelData()->getManager<MechMotionData::mMotionRLManager>(Physical_MotionRL).getAllIDs().size();
        if (num == 0 && _propertyItem != nullptr)
        {
            _treeModel->removeTreeItem(_propertyItem);
            selectedItem(_treeModel->getParentItem(Motion_RootItem));
        }
        else
        {
            removeProxyIndexs(deleteItems);
            selectedItem(_propertyItem);
        }
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionRL, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionRL()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        auto _propertyItem = _treeModel->getParentItem(Motion_RLParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_RLChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionRL, getPhyIdByModelIndex(idx));
                mMotionRL* property = getCVM->getModelData()->getManager<MechMotionData::mMotionRLManager>(Physical_MotionRL).getData(copyId);
                if (property == nullptr) continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_RLChildItem, property->getName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionRL, copyIDs } });
        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionRLTree(bool isCreate, int ID)
    {
        mMotionRL* data = getCVM->getModelData()->getManager<mMotionRLManager>(Physical_MotionRL).getData(ID);
        if (data == nullptr) return;

        int modelID = getCVM->getModelData()->getModelDataID();
        mTreeItem* parentItem = _treeModel->getParentItem(Motion_RLParentItem);
        //动态分组：首对象到达时懒建父分组（删空由 ChildDel 菜单拆除）
        if (parentItem == nullptr)
        {
            parentItem = new mTreeItem(modelID, Motion_RLParentItem, "强化学习");
            _treeModel->appendChildren(_treeModel->getParentItem(Motion_RootItem), parentItem);
        }
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_RLChildItem, data->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/BrowserTree/motion.png");
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_RLChildItem, ID);
            if (cusItem != nullptr)
                setTreeItemText(cusItem, data->getName());
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionRL, ID);
    }

    void mTreeView::slot_menuMotionSensorNew(MechMotionData::SensorType type)
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionSensor(type);
    }

    void mTreeView::slot_menuMotionSensorDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionSensor);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionSensorDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionSensor);
        mTreeItem* item = _treeModel->getParentItem(Motion_SensorParentItem);
        //固定分组常驻：清空分组下对象节点，保留分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionSensor, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionSensorChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_SensorParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_SensorChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionSensorDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionSensor, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionSensor, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionSensor()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        auto _propertyItem = _treeModel->getParentItem(Motion_SensorParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_SensorChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionSensor, getPhyIdByModelIndex(idx));
                mMotionSensorBase* property = getCVM->getModelData()->getManager<MechMotionData::mMotionSensorManager>(Physical_MotionSensor).getData(copyId);
                if (property == nullptr) continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_SensorChildItem, property->getName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionSensor, copyIDs } });
        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionSensorTree(bool isCreate, int ID)
    {
        mMotionSensorBase* data = getCVM->getModelData()->getManager<mMotionSensorManager>(Physical_MotionSensor).getData(ID);
        if (data == nullptr) return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* parentItem = _treeModel->getParentItem(Motion_SensorParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_SensorChildItem, data->getName(), ID, MxFunctions::Tree_Yes, getMotionSensorPic(data->getType()));
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_SensorChildItem, ID);
            if (cusItem != nullptr)
            {
                setTreeItemText(cusItem, data->getName());
                //编辑可能改类型：图标跟随刷新（同耦合副更新口径）
                QString pic = getMotionSensorPic(data->getType());
                if (getIconStringByItem(cusItem) != pic)
                {
                    setTreeItemPic(cusItem, pic);
                }
            }
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionSensor, ID);
    }

    void mTreeView::slot_menuMotionControllerNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionController();
    }

    void mTreeView::slot_menuMotionControllerDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionController);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionControllerDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionController);
        mTreeItem* item = _treeModel->getParentItem(Motion_ControllerParentItem);
        //固定分组常驻：清空分组下对象节点，保留分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionController, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionControllerChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_ControllerParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ControllerChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionControllerDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionController, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionController, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionController()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        auto _propertyItem = _treeModel->getParentItem(Motion_ControllerParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_ControllerChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionController, getPhyIdByModelIndex(idx));
                mMotionControllerBase* property = getCVM->getModelData()->getManager<MechMotionData::mMotionControllerManager>(Physical_MotionController).getData(copyId);
                if (property == nullptr) continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ControllerChildItem, property->getName(), copyId, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/Motion/controlCalculation.png");
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
                lastItem = childitem;
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionController, copyIDs } });

        if (lastItem)
            selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionControllerTree(bool isCreate, int ID)
    {
        mMotionControllerBase* data = getCVM->getModelData()->getManager<mMotionControllerManager>(Physical_MotionController).getData(ID);
        if (data == nullptr) return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* parentItem = _treeModel->getParentItem(Motion_ControllerParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_ControllerChildItem, data->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/Motion/controlCalculation.png");
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_ControllerChildItem, ID);
            if (cusItem != nullptr)
                setTreeItemText(cusItem, data->getName());
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionController, ID);
    }

    void mTreeView::createMotionAutoFlexParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "新建自动柔性体");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionAutoFlexNew()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除所有自动柔性体");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionAutoFlexDelete()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::createMotionAutoFlexChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuMotionAutoFlexChildDel()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), "复制");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuCopyMotionAutoFlex()));
        m_Menu->exec(QCursor::pos());
    }

    void mTreeView::slot_menuMotionAutoFlexNew()
    {
        mInfoWindow::getInstance()->unEditInfoWindow();
        mInfoWindow::getInstance()->createMotionAutoFlex();
    }

    void mTreeView::slot_menuMotionAutoFlexDelete()
    {
        vector<int> phyIDs = getCVM->getModelData()->getAllPhyID(Physical_MotionAutoFlex);
        //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
        mInfoWindow::getInstance()->notifyMotionAutoFlexDataAboutToBeDestroyed();
        getCVM->getModelData()->deleteAllByPhysicalType(Physical_MotionAutoFlex);
        mTreeItem* item = _treeModel->getParentItem(Motion_AutoFlexParentItem);
        //固定分组常驻：清空分组下对象节点，保留分组本身
        _treeModel->removeTreeChildrenItem(item);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionAutoFlex, phyIDs);
        selectedItem(_treeModel->getParentItem(Motion_RootItem));
    }

    void mTreeView::slot_menuMotionAutoFlexChildDel()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        vector<int> phyIDs; int phyID;
        QModelIndexList deleteItems;
        auto _propertyItem = _treeModel->getParentItem(Motion_AutoFlexParentItem);
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_AutoFlexChildItem)
            {
                phyID = getPhyIdByModelIndex(idx);
                //先通知面板丢弃悬垂指针（此时对象仍有效），再真正删除数据对象
                mInfoWindow::getInstance()->notifyMotionAutoFlexDataAboutToBeDestroyed(phyID);
                getCVM->getModelData()->deleteByPhysicalType(Physical_MotionAutoFlex, phyID);
                deleteItems.push_back(idx);
                phyIDs.push_back(phyID);
            }
        }

        //固定分组常驻：删空后不再移除分组，仅移除对象节点
        removeProxyIndexs(deleteItems);
        selectedItem(_propertyItem);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_MotionAutoFlex, phyIDs);
    }

    void mTreeView::slot_menuCopyMotionAutoFlex()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        auto _propertyItem = _treeModel->getParentItem(Motion_AutoFlexParentItem);
        int copyId;
        mTreeItem* lastItem = nullptr;
        QVector<mTreeItem*> addItems;
        vector<int> copyIDs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == Motion_AutoFlexChildItem)
            {
                copyId = getCVM->getModelData()->copyPhysicalField(Physical_MotionAutoFlex, getPhyIdByModelIndex(idx));
                mMotionAutoFlex* property = getCVM->getModelData()->getManager<MechMotionData::mMotionAutoFlexManager>(Physical_MotionAutoFlex).getData(copyId);
                if (property == nullptr) continue;

                mTreeItem* childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_AutoFlexChildItem, property->getName(), copyId);
                lastItem = childitem;
                addItems.push_back(childitem);
                copyIDs.push_back(copyId);
            }
        }

        _treeModel->appendChildren(_propertyItem, std::move(addItems));
        getCVM->getDataStatusManager()->updateDynamicInfoData({ { Physical_MotionAutoFlex, copyIDs } });
        if (lastItem) selectedItem(lastItem);
    }

    void mTreeView::slot_updateMotionAutoFlexTree(bool isCreate, int ID)
    {
        mMotionAutoFlex* data = getCVM->getModelData()->getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex).getData(ID);
        if (data == nullptr) return;

        //固定分组常驻（addMotionRootTree 创建），无需懒建
        mTreeItem* parentItem = _treeModel->getParentItem(Motion_AutoFlexParentItem);
        if (isCreate)
        {
            auto childitem = new mTreeItem(getCVM->getModelData()->getModelDataID(), Motion_AutoFlexChildItem, data->getName(), ID, MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/Motion/mnf.png");
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
        }
        else
        {
            auto cusItem = _treeModel->getChildrenItem(Motion_AutoFlexChildItem, ID);
            if (cusItem != nullptr)
                setTreeItemText(cusItem, data->getName());
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_MotionAutoFlex, ID);
    }

}

