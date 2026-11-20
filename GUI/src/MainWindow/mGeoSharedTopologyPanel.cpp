#include "mGeoSharedTopologyPanel.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "ViewportContainer.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
#include "mGeoManager.h"
//#include "mGeoDataOfTie.h" //【OCC摘除·S10/P0-1 终局3】死 include（零使用；mGeoDataModelSet OCC 链）

//#include "mGeoSketchAlgorithm.h" //【OCC摘除·S10/P0-1 终局2】死 include（零使用；OCC 链头）
#include "GlobalMeshMessage.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"
//#include "mGeoDataOfShare.h" //【OCC摘除·S10/P0-1 终局3】死 include（零使用；mGeoDataModelSet OCC 链）

#include <QMenu>
#include <QtConcurrent/qtconcurrentrun.h>
//【OCC迁移·§8零成本清理】以下 11 条 OCC include 零调用（本体仅含 include 行），按不删习惯注释
//#include <BRepOffsetAPI_NormalProjection.hxx>
//#include <BRepBuilderAPI_MakeWire.hxx>
//#include <BRepBuilderAPI_MakeFace.hxx>
//#include <BRepFeat_SplitShape.hxx>
//#include <BRepGProp.hxx>
//#include <GProp_GProps.hxx>
//#include <BRepAlgoAPI_Splitter.hxx>
//#include <BRepOffset_Tool.hxx>
//#include <BRepBuilderAPI_MakeEdge.hxx>
//#include <BRepOffsetAPI_FindContigousEdges.hxx>
//#include <BRepBuilderAPI_MakeVertex.hxx>

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
using namespace mxr;

namespace MechMainWindow
{
    mGeoSharedTopologyPanel::mGeoSharedTopologyPanel(QWidget* parent)
        : mGeoPanelBase(parent)
    {
        connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            clearTableItems(pickObjTable);
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = FeatureObj;
            });

        initShareFold();
        initUnshareFold();
        initForceShareFold();

        _mainLayout->addStretch();

        //全局信号（拾取/视口切换）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接
    }

    void mGeoSharedTopologyPanel::connectGeoSignals()
    {
        //拾取信号
        slot_subWindowActivated();

        // 新架构：直接挂容器激活信号（带 prevID，可区分真正的视口切换）
        if (auto* container = MechPreData::mViewportModelManager::getInstance()->getViewportContainer()) {
            conn_viewportChanged = connect(container, &MechMainWindow::ViewportContainer::sig_activeViewportChanged,
                this, [this](int viewportID, int prevID) {
                    Q_UNUSED(viewportID); Q_UNUSED(prevID);
                    slot_subWindowActivated();
                });
        } else {
            conn_viewportChanged = connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, [this](int) { slot_subWindowActivated(); });
        }
    }

    void mGeoSharedTopologyPanel::disconnectGeoSignals()
    {
        if (conn_geo)
        {
            disconnect(conn_geo);
            conn_geo = QMetaObject::Connection();
        }
        if (conn_viewportChanged)
        {
            disconnect(conn_viewportChanged);
            conn_viewportChanged = QMetaObject::Connection();
        }
    }

    mGeoSharedTopologyPanel::~mGeoSharedTopologyPanel()
    {

    }

    void mGeoSharedTopologyPanel::showFold(GeoOperationType type)
    {
        hideAllFold();
        currentFeature = type;
        geoPickWidget->pickLabel->setText("对象");

        geoPickWidget->pickObjTypeCombo->clear();
        geoPickWidget->_featPick = FeatureObj;
        pickObjTable->setVisible(true);
        geoPickWidget->setVisible(true);

        if (GeoSharedTopology == type)
        {
            geoPickWidget->pickObjTypeCombo->addItems({ "几何线", "几何面" });
            m_FoldSections[0]->insertRow(0, pickObjTable);
            m_FoldSections[0]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 0 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoUnSharedTopology == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何体");
            m_FoldSections[1]->insertRow(0, pickObjTable);
            m_FoldSections[1]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 1 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoForceSharedTopology == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何体");
            m_FoldSections[2]->insertRow(0, pickObjTable);
            m_FoldSections[2]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 2 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
    }

    //初始化共享拓扑界面
    void mGeoSharedTopologyPanel::initShareFold()
    {
        shareTolerance = new MxFunctions::mCalcLineEdit();
        shareTolerance->setExpression("0.1");
        shareToleranceUnit = new QLabel("mm", this);

        QPushButton* shareCancelBtn = new QPushButton("取消", this);
        QPushButton* shareOkBtn = new QPushButton("确认", this);

        connect(shareCancelBtn, &QPushButton::clicked, this, &mGeoSharedTopologyPanel::slot_onCancel);
        connect(shareOkBtn, &QPushButton::clicked, this, &mGeoSharedTopologyPanel::slot_shareAction);
        FoldSection* shareFold_1 = new FoldSection("共享拓扑", this);

        shareFold_1->addRow(new PanelLabel("重合公差"), shareTolerance, shareToleranceUnit);

        QFrame* horizontalLine = createHorizontalLine(shareFold_1);
        shareFold_1->addRow(horizontalLine);
        shareFold_1->addRow(shareCancelBtn, shareOkBtn, "Left");

        m_FoldSections.append(shareFold_1);
        m_foldWidgets.append(shareFold_1);
        _mainLayout->addWidget(shareFold_1);
    }

    //初始化取消共享拓扑界面
    void mGeoSharedTopologyPanel::initUnshareFold()
    {
        unshareTolerance = new MxFunctions::mCalcLineEdit();
        unshareTolerance->setExpression("0.1");
        unshareToleranceUnit = new QLabel("mm", this);

        QPushButton* unshareCancelBtn = new QPushButton("取消", this);
        QPushButton* unshareOkBtn = new QPushButton("确认", this);

        connect(unshareCancelBtn, &QPushButton::clicked, this, &mGeoSharedTopologyPanel::slot_onCancel);
        connect(unshareOkBtn, &QPushButton::clicked, this, &mGeoSharedTopologyPanel::slot_unshareAction);
        FoldSection* unshareFold_1 = new FoldSection("取消共享拓扑", this);

        unshareFold_1->addRow(new PanelLabel("重合公差"), unshareTolerance, unshareToleranceUnit);

        QFrame* horizontalLine = createHorizontalLine(unshareFold_1);
        unshareFold_1->addRow(horizontalLine);
        unshareFold_1->addRow(unshareCancelBtn, unshareOkBtn, "Left");

        m_FoldSections.append(unshareFold_1);
        m_foldWidgets.append(unshareFold_1);
        _mainLayout->addWidget(unshareFold_1);
    }

    //初始化强制共享拓扑界面
    void mGeoSharedTopologyPanel::initForceShareFold()
    {
        forceShareTolerance = new MxFunctions::mCalcLineEdit();
        forceShareTolerance->setExpression("0.1");
        forceShareToleranceUnit = new QLabel("mm", this);

        QPushButton* forceShareCancelBtn = new QPushButton("取消", this);
        QPushButton* forceShareOkBtn = new QPushButton("确认", this);

        connect(forceShareCancelBtn, &QPushButton::clicked, this, &mGeoSharedTopologyPanel::slot_onCancel);
        connect(forceShareOkBtn, &QPushButton::clicked, this, &mGeoSharedTopologyPanel::slot_forceShareAction);
        FoldSection* forceShareFold_1 = new FoldSection("强制共享拓扑", this);

        forceShareFold_1->addRow(new PanelLabel("重合公差"), forceShareTolerance, forceShareToleranceUnit);

        QFrame* horizontalLine = createHorizontalLine(forceShareFold_1);
        forceShareFold_1->addRow(horizontalLine);
        forceShareFold_1->addRow(forceShareCancelBtn, forceShareOkBtn, "Left");

        m_FoldSections.append(forceShareFold_1);
        m_foldWidgets.append(forceShareFold_1);
        _mainLayout->addWidget(forceShareFold_1);
    }

    void mGeoSharedTopologyPanel::slot_highlightGeoData()
    {
        QString type = "几何线";

        int num = 0;
        vector<int> res;
        //获取拾取模式
        PickFilter pickFilter = getCVM->getPreRenderWidget()->getPickFilter();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        switch (pickFilter)
        {
        case mxr::PickFilter::PickGeoSolid:
        {
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
            type = "几何体";
            break;
        }
        case mxr::PickFilter::PickGeoFace:
        {
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
            type = "几何面";
            break;
        }
        case mxr::PickFilter::PickGeoLine:
        {
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            type = "几何线";
            break;
        }
        case mxr::PickFilter::PickGeoPoint:
        {
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
            type = "几何点";
            break;
        }
        default:
            break;
        }
        num = res.size();
        if (geoPickWidget->_featPick == FeatureObj)
        {
            for (int i = pickObjTable->rowCount() - 1; i >= 0; i--)
            {
                QString strId = pickObjTable->item(i, 0)->text();
                if (strId.indexOf(type) == 0)
                {
                    pickObjTable->removeRow(i);
                }
            }

            for (auto id : res)
            {
                pickObjTable->insertRow(pickObjTable->rowCount());
                pickObjTable->setItem(pickObjTable->rowCount() - 1, 0, new QTableWidgetItem(QString(type + ": %1").arg(id)));
            }
        }
    }
    void mGeoSharedTopologyPanel::slot_subWindowActivated()
    {
        if (conn_geo)
            disconnect(conn_geo);
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }
        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoSharedTopologyPanel::slot_highlightGeoData);
    }


    void mGeoSharedTopologyPanel::slot_onCancel()
    {
        hideAllFold();
    }


    void mGeoSharedTopologyPanel::slot_shareAction()
    {
        try
        {
            EventDispatcher::mGeoSharedTopologyRequest req;
            for (int i = 0; i < pickObjTable->rowCount(); ++i)
            {
                QString strId = pickObjTable->item(i, 0)->text();
                if (strId.indexOf("几何线") == 0)
                {
                    strId = strId.mid(strId.lastIndexOf(":") + 1);
                    req.lineIds.insert(strId.toInt());
                }
                else if (strId.indexOf("几何面") == 0)
                {
                    strId = strId.mid(strId.lastIndexOf(":") + 1);
                    req.faceIds.insert(strId.toInt());
                }
            }

            req.tolerance = getDefaultValue(Unit::Length, shareTolerance->text().toDouble());

            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().sharedTopologyOCC(req);
            if (res.succeeded())
            {
                //【共享拓扑·断链修复】适配器回传的是 modifiedObjectIds（generatedObjects
                // 恒空），按其登记编辑部件；否则 syncAfterShapeEdit 拿不到部件，
                // MeshEntity 不重建，压印结果传不到剖分侧
                for (const auto& modified : res.modifiedObjectIds)
                {
                    if (modified.type == GeoNeutralData::GN_ShapeType::Part)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(modified.id);
                    }
                }
                //【共享拓扑·登记】引擎判定/压印出的共享面对、边对落 GN 对账表
                //（旧命令 setCommonFace/setCommonEdge 同款登记），syncAfterShapeEdit
                // 内 rebuildSharedTopologyFromGN 回填共享面/共边/共点组到剖分数据源
                auto* geoManager = getCVM->getModelData()->getGeoManager();
                geoManager->registerSharedTopologyPairs(res.sharedFacePairs, res.sharedEdgePairs);
                geoManager->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("共享拓扑成功!"));
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("共享拓扑失败!"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("共享拓扑操作出错！"));
        }
    }

    void mGeoSharedTopologyPanel::slot_unshareAction()
    {
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何体!"), true);
                return;
            }

            double tolerance = getDefaultValue(Unit::Length, unshareTolerance->text().toDouble());
            // TODO: 接入实际取消共享拓扑命令，tolerance 为重合公差
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("取消共享拓扑操作已触发。"));
            geoPickWidget->clearHighlight();
            clearTableItems(pickObjTable);
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("取消共享拓扑操作出错！"));
        }
    }

    void mGeoSharedTopologyPanel::slot_forceShareAction()
    {
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何体!"), true);
                return;
            }

            double tolerance = getDefaultValue(Unit::Length, forceShareTolerance->text().toDouble());
            // TODO: 接入实际强制共享拓扑命令，tolerance 为重合公差
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("强制共享拓扑操作已触发。"));
            geoPickWidget->clearHighlight();
            clearTableItems(pickObjTable);
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("强制共享拓扑操作出错！"));
        }
    }

}
