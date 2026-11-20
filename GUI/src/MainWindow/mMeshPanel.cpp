
#include "mMeshPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mDataStatusManager.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QHBoxLayout>
#include <QMetaType>      // 元类型注册函数
#include <QVector3D>
#include "mCalcLineEdit.h"

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitModal.h"
#include "mCaseImplicitNonLinearStatic.h"
#include "mCaseImplicitLinearBuckling.h"
#include "mCaseImplicitTransient.h"
#include "mModelData.h"
#include "mMeshData.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/mFontRender.h"
//PreData
#include "mGeoManager.h"
//Mesh
//#include "MXMeshAPI.h"
#include "MXMesh.h"
#include "mxr/view/mPreMeshHighLightRender.h"
#include "mxr/view/HighlightController.h"
#include "mxr/basic/CameraController.h"
#include "mxr/basic/Camera.h"

#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "PickCommonFunction.h"

#include "mInfoWindow.h"
#include <set>
#include <QAction>
#include "mDataElement.h"
#include "mElementBase.h"
#include "mDataStatusManager.h"
#include "mInfoWindow.h"
#include "mOtherFunction.h"
#include"mCFDMeshData.h"
using namespace MechPreData;
using namespace MeshPanelData;
using namespace MxFunctions;
using namespace MechElementData;

Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{

    mMeshPanel::mMeshPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        qRegisterMetaType<MeshParam>();         // main 或初始化里注册一次
        qRegisterMetaType<std::set<int>>();   // 只写一次
        m_meshThread = new mMeshThread(this);
        m_thread = new QThread(this);
      
        m_meshThread->moveToThread(m_thread);
        connect(this, &mMeshPanel::startSlot_createMeshAuto, m_meshThread, &mMeshThread::slot_createMeshAuto, Qt::AutoConnection);
        connect(this, &mMeshPanel::startSlot_createMeshByPartId, m_meshThread, &mMeshThread::slot_createMeshByPartId, Qt::QueuedConnection);
        connect(this, &mMeshPanel::startSlot_createMesh, m_meshThread, &mMeshThread::slot_createMesh, Qt::QueuedConnection);
        connect(this, &mMeshPanel::startSlot_createMeshBySetting, m_meshThread, &mMeshThread::slot_createMeshBySetting, Qt::QueuedConnection);
        connect(this, &mMeshPanel::startSlot_createMesh1, m_meshThread, &mMeshThread::slot_createMesh1, Qt::QueuedConnection);
        connect(this, &mMeshPanel::startSlot_createMeshAutoAdapt, m_meshThread, &mMeshThread::slot_createMeshAutoAdapt, Qt::QueuedConnection);
        connect(this, &mMeshPanel::startSlot_createMeshCustomEncryption, m_meshThread, &mMeshThread::slot_createMeshCustomEncryption, Qt::QueuedConnection);
        connect(this, &mMeshPanel::startSlot_createMeshLocalSize, m_meshThread, &mMeshThread::slot_createMeshLocalSize, Qt::QueuedConnection);
        connect(this, &mMeshPanel::startSlot_createMeshEdgeSuppress, m_meshThread, &mMeshThread::slot_createMeshEdgeSuppress, Qt::QueuedConnection);
        connect(m_meshThread, &mMeshThread::meshSutureRequested, this, [=](bool isSuppress, std::set<int> geoIds) {
            emit MxFunctions::mGlobalSignals::getInstance()->MeshSutureSig(isSuppress, geoIds);
        }, Qt::QueuedConnection);
        // 工作线程在 mMeshThread 中 emit meshSutureRequested，由主线程的 mMeshPanel 接收并转发
        connect(this, &mMeshPanel::setBtnEnabled, [=]() {
            mInfoWindow::getInstance()->createBtn->setEnabled(true);
            mInfoWindow::getInstance()->deleteBtn->setEnabled(true);
            });
      
        connect(m_meshThread, &mMeshThread::done, this, [=](const QString& result,vector<int> partIds)
            {
            
                mInfoWindow::getInstance()->isMeshing = false;//剖分完毕
                mInfoWindow::getInstance()->_actions->at(5)->setEnabled(true);
                mInfoWindow::getInstance()->_actions->at(6)->setEnabled(true);
                mInfoWindow::getInstance()->_actions->at(7)->setEnabled(true);
                mInfoWindow::getInstance()->_actions->at(8)->setEnabled(true);
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(mInfoWindow::getInstance()->_pickFilter);
                emit setBtnEnabled();
                if (result == "Error")
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, tr("面网格类型冲突"), true);
                    return;
                }
                if (result == "NoGeometry")
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, tr("模型无几何部件，可能未导入"), true);
                    return;
                }
                MechModelData::mModelData* modelData = getCVM->getModelData();
                mMeshData* meshdata = modelData->getMeshData();          
                meshdata->ensureMesh();//网格剖分后里存底层数据的接口

                // 若当前网格面板设为二阶，对本次生成的 part 统一升阶
                if (_meshPanelData && _meshPanelData->getMeshOrder() == "二阶" && !partIds.empty())
                {
                    std::set<QString> partNames;
                    for (int pid : partIds)
                    {
                        partNames.insert(meshdata->getPartNameByPartId(pid));
                    }

                    std::set<int> oldIds, newIds;
                    meshdata->setMeshSecond(partNames, oldIds, newIds);

                    if (!oldIds.empty() || !newIds.empty())
                    {
                        mGlobalSignals::getInstance()->MeshOrderChangeSig(oldIds, newIds);
                    }
                }

                //mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分完成"), true);
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
                getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
                getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("biastNum");
                getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("adjustNum");
                /*修改几何部件状态*/
                if (!partIds.empty())
                {
                    std::set<int> set(partIds.begin(), partIds.end());
                    emit mGlobalSignals::getInstance()->createUpdatePartSig(false, set);
                }
                // 网格生成完成后刷新模型树网格剖分节点状态（子节点及父节点图标）
                emit MechCommon::mMechGlobalSignals::getInstance()->updateMeshStatus();
    
        
                // 默认赋予单元：直接从 meshdata 获取所有元素并分类
                // 避免依赖未发射的 assignElemnetTypeBymeshEdit 信号
                std::vector<int> beamIds, triIds, quadIds, tetIds, wedgeIds, hexIds;
                std::vector<RenderPickResultData> allElements = meshdata->getAllElements();
                for (const auto& ele : allElements)
                {
                    int eleId = ele.result_id;
                    MeshType meshType = meshdata->getMeshTypeByEleId(eleId);
                    switch (meshType)
                    {
                        case MeshBeam: beamIds.push_back(eleId); break;
                        case MeshTri:  triIds.push_back(eleId);  break;
                        case MeshQuad: quadIds.push_back(eleId); break;
                        case MeshTet:  tetIds.push_back(eleId);  break;
                        case MeshWedge: wedgeIds.push_back(eleId); break;
                        case MeshHex:  hexIds.push_back(eleId);  break;
                        default: break;
                    }
                }

                // 单元阶数：用发起剖分时经管理器重查后的快照。
                // 期间面板节点可能已被删除（缓存的_meshPanelData悬空），不能在这里解引用
                int meshOrder = _meshOrderSnapshot;

                //赋予默认单元
                mDataElement* elementData = getCVM->getModelData()->getElementData();
                ElementType elementType;
                for (auto elementsid : tetIds)
                {
                    elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshTet, meshOrder);
                    elementData->assignElementTypeToMesh(elementsid, elementType);
                }
                for (auto elementsid : hexIds)
                {
                    elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshHex, meshOrder);
                    elementData->assignElementTypeToMesh(elementsid, elementType);
                }
                for (auto elementsid : wedgeIds)
                {
                    elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshWedge, meshOrder);
                    elementData->assignElementTypeToMesh(elementsid, elementType);
                }
                for (auto elementsid : triIds)
                {
                    elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshTri, meshOrder);
                    mElementBase* elementBase = elementData->getElementDataByMeshID(elementsid);
                    mShellElement* element = nullptr;
                    if (elementBase != nullptr && elementBase->getElementType() == elementType)
                    {
                        element = dynamic_cast<mShellElement*>(elementBase);
                        element->setElementType(elementType);
                    }
                    if (elementBase == nullptr || element == nullptr || elementBase->getElementType() != elementType)
                    {
                        auto element = new mShellElement(elementType);
                        elementData->assignElementToMesh(elementsid, element);
                    }
                    else
                    {
                        element;//修改参数
                    }

                }
                for (auto elementsid : quadIds)
                {
                    elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshQuad, meshOrder);
                    mElementBase* elementBase = elementData->getElementDataByMeshID(elementsid);
                    mShellElement* element = nullptr;
                    if (elementBase != nullptr && elementBase->getElementType() == elementType)
                    {
                        element = dynamic_cast<mShellElement*>(elementBase);
                        element->setElementType(elementType);
                    }
                    if (elementBase == nullptr || element == nullptr || elementBase->getElementType() != elementType)
                    {
                        auto element = new mShellElement(elementType);
                        elementData->assignElementToMesh(elementsid, element);
                    }
                    else
                    {
                        element;//修改参数
                    }

                }

                for (auto elementsid : beamIds)
                {
                    elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, MeshBeam, meshOrder);
                    mElementBase* elementBase = elementData->getElementDataByMeshID(elementsid);
                    mBeamElement* element = nullptr;
                    if (elementBase != nullptr && elementBase->getElementType() == elementType)
                    {
                        element = dynamic_cast<mBeamElement*>(elementBase);
                        element->setElementType(elementType);
                    }
                    if (elementBase == nullptr || element == nullptr || elementBase->getElementType() != elementType)
                    {
                        auto element = new mBeamElement(elementType);
                        elementData->assignElementToMesh(elementsid, element);
                    }
                    else
                    {
                        element;//修改参数
                    }
                }



            }, Qt::QueuedConnection
        );
       
        m_thread->start();
    }

    mMeshPanel::~mMeshPanel()
    {
      
        m_thread->quit();
        m_thread->wait();   // 安全退出
    }
    QFrame* mMeshPanel::createHorizontalLine(QWidget* parent)
    {
        QFrame* line = new QFrame(parent);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        return line;
    }
    int mMeshPanel::getMeshID()
    {
       /* if (_caseData)
            return _caseData->getCaseId();
      */
        return 1;
    }
    bool pushBackUnique(std::vector<RenderPickResultData>& vec, const RenderPickResultData& data)
    {
        // 查找是否已存在相同 result_id
        auto it = std::find_if(vec.begin(), vec.end(),
            [&data](const RenderPickResultData& item) {
                return item.result_id == data.result_id;
            });

        if (it == vec.end()) {
            vec.push_back(data);  // 不存在，插入
            return true;          // 插入成功
        }
        return false;             // 已存在，跳过
    }
    //【镜像六件套摘除·连带】与头文件声明同步：mxr:: 显式限定
    bool pushBackUnique(std::vector<mxr::MeshSelectInfoData>& vec, const mxr::MeshSelectInfoData& data)
    {
        // 查找是否已存在相同 result_id
        auto it = std::find_if(vec.begin(), vec.end(),
            [&data](const mxr::MeshSelectInfoData& item) {
                return item.data_id.node_id == data.data_id.node_id;
            });

        if (it == vec.end()) {
            vec.push_back(data);  // 不存在，插入
            return true;          // 插入成功
        }
        return false;             // 已存在，跳过
    }
    void mMeshPanel::newMeshPanel(int type)
    {

        mMeshPanelManager& manager = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel);


        int meshId = manager.createID();
        _meshPanelId = meshId;
        if (type == SolidMesh)
        {
            _meshPanelData = new mMeshPanelBase(SolidMesh, getCVM->getModelData()->createNameByType(Physical_MeshPanel, "体网格"), meshId);
        }
        else if (type == FaceMesh)
        {
            _meshPanelData = new mMeshPanelBase(FaceMesh, getCVM->getModelData()->createNameByType(Physical_MeshPanel, "面网格"), meshId);
        }
        else if (type == LineMesh)
        {
            _meshPanelData = new mMeshPanelBase(LineMesh, getCVM->getModelData()->createNameByType(Physical_MeshPanel, "线网格"), meshId);
        }
        else if (type == AutoAdapt)
        {
            _meshPanelData = new mMeshPanelBase(AutoAdapt, getCVM->getModelData()->createNameByType(Physical_MeshPanel, "网格自适应"), meshId);
        }
        else if (type == CustomEncryption)
        {
            _meshPanelData = new mMeshPanelBase(CustomEncryption, getCVM->getModelData()->createNameByType(Physical_MeshPanel, "自定义加密"), meshId);
        }
        else if (type == MeshPanelType::LocalSize)
        {
        	_meshPanelData = new mMeshPanelBase(MeshPanelType::LocalSize, getCVM->getModelData()->createNameByType(Physical_MeshPanel, "局部尺寸"), meshId);
        }
        else if (type == EdgeSuppress)
        {
        	_meshPanelData = new mMeshPanelBase(EdgeSuppress, getCVM->getModelData()->createNameByType(Physical_MeshPanel, "边抑制"), meshId);
        }
      
        if (type == SolidMesh || type == FaceMesh || type == LineMesh
           || type == AutoAdapt || type == CustomEncryption || type == MeshPanelType::LocalSize || type == EdgeSuppress)
        {
            //创建成功发送消息
            mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建网格 %1").arg(_meshPanelData->getMeshPanelName()), true);
            getCVM->getModelData()->addPhysicalField<mMeshPanelBase>(_meshPanelData);

            //添加默认映射关系
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            newMeshPanel();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMeshSig(true, meshId);
        }
      
        initSize();



    }

    void mMeshPanel::newMeshOptPanel(int type)
    {
        mMeshOptPanelManager& manager = getCVM->getModelData()->getManager<MeshPanelData::mMeshOptPanelManager>(Physical_MeshOptPanel);
        int meshId = manager.createID();
        if (type == Translation)
        {
            _meshOptPanelData = new mMeshOptPanelBase(Translation, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "平移"), meshId);
            }
        else if (type == Rotation)
        {
            _meshOptPanelData = new mMeshOptPanelBase(Rotation, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "旋转"), meshId);
            }
        else if (type == Mirroring)
        {
            _meshOptPanelData = new mMeshOptPanelBase(Mirroring, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "镜像"), meshId);
            }
        else if (type == Matrix)
        {
            _meshOptPanelData = new mMeshOptPanelBase(Matrix, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "阵列"), meshId);
            }
        else if (type == MeshZoom)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshZoom, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "缩放"), meshId);
            }
        else if (type == MovePoint)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MovePoint, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "移动点"), meshId);
        }
        else if (type == MeshStretch)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshStretch, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "拉伸"), meshId);
            }
        else if (type == MeshSmooth)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshSmooth, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "网格光滑"), meshId);
        }
        else if (type == Remeshing)
        {
            _meshOptPanelData = new mMeshOptPanelBase(Remeshing, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "重剖分"), meshId);
        }
        else if (type == STLRemesh)
        {
            _meshOptPanelData = new mMeshOptPanelBase(STLRemesh, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "STL重剖分"), meshId);
        }
        else if (type == DeleteMesh)
        {
            _meshOptPanelData = new mMeshOptPanelBase(DeleteMesh, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "网格删除"), meshId);
        }
        else if (type == Renumber)
        {
            _meshOptPanelData = new mMeshOptPanelBase(Renumber, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "重编号"), meshId);
        }
        else if (type == MeshNormal)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshNormal, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "法向"), meshId);
        }
        else if (type == CreatePoint)
        {
            _meshOptPanelData = new mMeshOptPanelBase(CreatePoint, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "创建节点"), meshId);
        }
        else if (type == CreateMesh)
        {
            _meshOptPanelData = new mMeshOptPanelBase(CreateMesh, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "创建网格"), meshId);
        }
        else if (type == CreateMetaMesh)
        {
            _meshOptPanelData = new mMeshOptPanelBase(CreateMetaMesh, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "创建元网格"), meshId);
        }
        else if (type == ExtractSurface)
        {
            _meshOptPanelData = new mMeshOptPanelBase(ExtractSurface, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "提取表面网格"), meshId);
        }
        else if (type == RotateMesh)
        {
            _meshOptPanelData = new mMeshOptPanelBase(RotateMesh, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "旋转网格"), meshId);
        }
        else if (type == ChangeOrder)
        {
            _meshOptPanelData = new mMeshOptPanelBase(ChangeOrder, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "改变阶数"), meshId);
        }
        else if (type == SharedNode)
        {
            _meshOptPanelData = new mMeshOptPanelBase(SharedNode, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "共节点"), meshId);
        }
        else if (type == MeshPanelType::MeshFreeEdge)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshPanelType::MeshFreeEdge, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "自由边"), meshId);
            updatePanel();
        }
        else if (type == TEdge)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshPanelType::TEdge, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "T形边"), meshId);
            updatePanel();
        }
        else if (type == MeshIntersect)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshIntersect, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "相交"), meshId);
            updatePanel();
        }
        else if (type == MeshOverlap)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshOverlap, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "重叠"), meshId);
            updatePanel();
        }
        else if (type == MeshInterfere)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshInterfere, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "干涉"), meshId);
            updatePanel();
        }
        else if (type == MeshStatistics)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MeshStatistics, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "统计"), meshId);
            }
        else if (type == MoveMesh)
        {
            _meshOptPanelData = new mMeshOptPanelBase(MoveMesh, getCVM->getModelData()->createNameByType(Physical_MeshOptPanel, "移动网格"), meshId);
            }
        if (type != MeshQuality&&
            type != MeshPanelType::MeshFreeEdge &&
            type != TEdge&&
            type != MeshIntersect&&
            type != MeshOverlap&&
            type != MeshInterfere&&
            type != MeshStatistics&&
            type != MeshEdit
            )
        {
            //创建成功发送消息
            mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建网格 %1").arg(_meshOptPanelData->getMeshPanelName()), true);
            getCVM->getModelData()->addPhysicalField<mMeshOptPanelBase>(_meshOptPanelData);
            newMeshPanel();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMeshOptSig(true, meshId);
        }
  
    }
    void mMeshPanel::initSize()
    {

    }
    void mMeshPanel::newMeshPanel()
    {

    }
    void mMeshPanel::updatePanel()
    {
        
    }
    void mMeshPanel::editMeshPanel(int meshId)
    {
        _meshPanelId = meshId;
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);

        updatePanel();

    }

    void mMeshPanel::slot_createMeshByPartId(vector<int> partIds)
    {
        emit startSlot_createMeshByPartId(partIds);
    }

    void mMeshPanel::slot_createMeshAuto()
    {
        // 发起时刻经管理器按id重查（节点可能已被删除，缓存的_meshPanelData会悬空），
        // 查不到按一阶兜底；快照成值供完成回调使用
        _meshOrderSnapshot = 1;
        if (_meshPanelId >= 0)
        {
            mMeshPanelBase* data = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(_meshPanelId);
            if (data != nullptr && data->getMeshOrder() == "二阶")
            {
                _meshOrderSnapshot = 2;
            }
        }
        emit startSlot_createMeshAuto();
    }

    void mMeshPanel::slot_deleteMeshAuto()
    {
        //MxFunctions::mGlobalVarible::delPartName(citem->text(0));
                    //mGlobalSignals::getInstance()->DeleteUnitAssociatedPart(citem->text(0));
                    //MeshMessage::getInstance()->deleteMeshPart(citem->text(0));
                    //meshSectionDelete(citem);
                    //mUndoRedo::getInstance()->clear();
                    /*if (db != nullptr)
                    {
                        db->DeletePartUpSectionData(citem->text(0));

                    }*/
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();

        MeshEntity* entity = meshdata->getMeshEntity();
        getCVM->clearPick();
        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll(); 
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("biastNum");
        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("adjustNum");

        // 获取当前的网格部件
        std::vector<int> partIds_before_del = getCVM->getModelData()->getMeshData()->getAllPartIds();

        meshdata->deleteAllMeshPart();
        
        //mModelTree* modelTree = mInfoWindow::getInstance()->getModelTreeManager()->getCurrentModelTree();
        //auto partItem = modelTree->getItem(MeshParentItem);
        //partItem->setPic1Status(Tree_Update);
        getCVM->getDataStatusManager()->deleteDynamicInfoData(Physical_Part, meshdata->getAllPartIds());
        //meshdata->ensureMesh();//网格剖分后里存底层数据的接口
        //MxFunctions::_repeatPartName.clear();
        //更新模型树
        //updateBrowserTree(MxFunctions::ModelParentItem);
        //清空信息窗口
        //mInfoWindow::getInstance()->getInfoTable()->clearTable();
        //发送信号更新网格零件
        //mGlobalSignals::getInstance()->updatePartInfoSig();
        //mGlobalSignals::getInstance()->UpDateView();

        // 获取删除之后剩余的网格部件
        std::vector<int> partIds_after_del = getCVM->getModelData()->getMeshData()->getAllPartIds();
        std::sort(partIds_before_del.begin(), partIds_before_del.end());
        std::sort(partIds_after_del.begin(), partIds_after_del.end());

        std::set<int> deleted_ids;
        std::set_difference(
            partIds_before_del.begin(), partIds_before_del.end(),
            partIds_after_del.begin(), partIds_after_del.end(),
            std::inserter(deleted_ids, deleted_ids.begin())
        );

        if (deleted_ids.size() != 0)
        {
            mGlobalSignals::getInstance()->deletePartSig(deleted_ids);
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData({ {Physical_Part,  partIds_after_del} });
    }
    void mMeshPanel::slot_createMeshBySetting()
    {
        emit startSlot_createMeshBySetting();
    }
    void mMeshPanel::slot_createMesh(int id)
    {
        QStringList list=_meshPanelData->_list;
        if (list.empty())
            return;
        emit startSlot_createMesh(id,list);
    }

    void mMeshPanel::slot_createMesh1() {
        emit startSlot_createMesh1();
    }
    void mMeshPanel::slot_createMeshAutoAdapt()
    {

    }
    void mMeshPanel::slot_createMeshCustomEncryption()
    {
        emit startSlot_createMeshCustomEncryption();
    }
    void mMeshPanel::slot_createMeshLocalSize(int id)
    {
        QStringList list = _meshPanelData->_list;
        std::map<int,double> map = _meshPanelData->_ids2SizesMap;
        if (list.empty()||map.empty())
            return;
        emit startSlot_createMeshLocalSize(id, list, map);
    }
    void mMeshPanel::slot_createMeshEdgeSuppress(int id, bool isSuppress)
    {
        // 边抑制/取消抑制功能待实现
        QStringList list = _meshPanelData->_list;
        if (list.empty())
            return;
        emit startSlot_createMeshEdgeSuppress(id, list, isSuppress);
    }
    void mMeshPanel::slot_boxSave()
    {

    }
    void mMeshPanel::slot_boxDelete()
    {

    }
    void mMeshPanel::slot_boxHide()
    {

    }
    void mMeshPanel::slot_boxPreview()
    {

    }
    void mMeshPanel::slot_boxShow()
    {

    }
    void mMeshPanel::slot_localSizePreview()
    {

    }
    void mMeshPanel::slot_updateListWidget()
    {
        _listAssignPart->clear();
        _listAssignPart->addItems(_meshPanelData->_list);
    }
    void mMeshPanel::recalculateSizeFromList()
    {
        if (!_meshPanelData || _meshPanelData->_list.isEmpty())
            return;

        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData) return;

        mMeshData* meshdata = modelData->getMeshData();
        if (!meshdata) return;

        auto* meshEntity = meshdata->getMeshEntity();
        if (!meshEntity) return;

        // 从 _list 中提取几何ID
        vector<int> geoIds;
        for (const QString& item : _meshPanelData->_list)
        {
            QStringList parts = item.split("_");
            if (parts.size() >= 2)
                geoIds.push_back(parts[1].toInt());
        }

        if (geoIds.empty())
            return;

        // 根据面板类型计算默认尺寸
        double defaultMeshSize = 2.0;
        int type = _meshPanelData->getMeshPanelType();
        switch (type)
        {
        case SolidMesh:
        {
            vector<double> defaultsizes;
            bool sizeDiff = false;
            defaultMeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForSolidMesh(geoIds, defaultsizes, sizeDiff);
            if (sizeDiff)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("实体剖分选取的剖分对象大小差距较大，造成计算的预定义尺寸不同")), true);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, QString("%1").arg(QString("选取最小实体的计算剖分尺寸作为当前体剖分的预定义尺寸")), true);
            }
            break;
        }
        case FaceMesh:
        {
            double defaultsizes = 2.0;
            defaultMeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForFaceMesh(geoIds, defaultsizes);
            break;
        }
        case LineMesh:
        {
            // 线网格使用固定默认尺寸或从实体计算
            double defaultsizes = 2.0;
            defaultMeshSize = meshEntity->calculateDefaultMeshsizeBaseAttributeForFaceMesh(geoIds, defaultsizes);
            break;
        }
        case MeshPanelType::LocalSize:
        {
            // 局部尺寸面板按几何线在 _ids2SizesMap/_ids2NumsMap 中分线记录尺寸，
            // 复制作用对象时尺寸已随 _list 一并拷贝，不应被全局默认尺寸覆盖
            updatePanel();
            return;
        }
        default:
            return;
        }

        // 更新数据
        _meshPanelData->setMeshSize(defaultMeshSize);
        _meshPanelData->setPreDefineMeshSize(defaultMeshSize);

        // 更新面板显示
        updatePanel();
    }
    void mMeshPanel::unEditMeshPanel()
    {
        if (getCVM->getPreRenderWidget())
        {
            getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("biastNum");
            getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("adjustNum");
        }
            
    }

    void mMeshPanel::getMeshInfoById(const std::vector<int> ids)
    {

    }

    MeshPanelData::mMeshPanelBase* mMeshPanel::getMeshPanelBase()
    {
        return _meshPanelData;
    }

    QFrame* mMeshPanel::createHorizontalLine()
    {
        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        return line;
    }
    

    void VecPickWidget::initWidget(const QString& name, bool hasadd, bool hasThreePoint)
    {
        _hasThreePoint = hasThreePoint;
        coor = new PanelLabel();
        coorBox = new QComboBox();
        coorBtn = new QToolButton();
        dir = new PanelLabel();
        dirBox = new QComboBox;
        dirBtn = new QToolButton;
        vec = new PanelLabel();
        vecEdit = new QLineEdit;
        vecBtn = new QToolButton;
        propwid = new QWidget();

        coor->setText("参考系");
        coorBox->addItems({ "全局坐标系" });
        coorBtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/zuobiaoxi.png"));
        dir->setText(name);
        if (hasThreePoint)
            dirBox->addItems({ "X轴", "Y轴", "Z轴", "输入参数", "三点确定", "垂直于屏幕", "平行于屏幕" });
        else
            dirBox->addItems({ "X轴", "Y轴", "Z轴", "输入参数", "垂直于屏幕", "平行于屏幕" });
        dirBtn->setIcon(QIcon(":/mMainWidgets/Icon/homepage/gengxin.png")); dirBtn->setToolTip("反向");
        vec->setText("");
        vecEdit->setText("1,0,0"); vecBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        vecEdit->setDisabled(true); vecBtn->setDisabled(true);
        QVBoxLayout* v = new QVBoxLayout;
        QHBoxLayout* h0 = new QHBoxLayout;
        QHBoxLayout* h1 = new QHBoxLayout;
        h0->addWidget(coor);
        h0->addWidget(coorBox);
        h0->addWidget(coorBtn);
        h1->addWidget(dir);
        h1->addWidget(dirBox);
        h1->addWidget(dirBtn);

        vecLayout = new QWidget();
        QHBoxLayout* h2Layout = new QHBoxLayout();
        h2Layout->setMargin(0);
        h2Layout->setSpacing(10);
        h2Layout->addWidget(vec);
        h2Layout->addWidget(vecEdit);
        h2Layout->addWidget(vecBtn);
        vecLayout->setLayout(h2Layout);
        vecLayout->setMinimumHeight(26);

        v->addLayout(h0);
        v->addLayout(h1);
        v->addWidget(vecLayout);

        // 三点确定 UI
        threePointWidget = new QWidget();
        QVBoxLayout* tpV = new QVBoxLayout();
        tpV->setMargin(0);
        tpV->setSpacing(10);

        auto createPointRow = [&](const QString& label, mCalcLineEdit*& x, mCalcLineEdit*& y, mCalcLineEdit*& z, QToolButton*& btn) {
            QLabel* lbl = new PanelLabel(label);
            lbl->setFixedWidth(30);
            lbl->setFixedHeight(24);
            lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            x = new mCalcLineEdit();
            y = new mCalcLineEdit();
            z = new mCalcLineEdit();
            btn = new QToolButton();
            btn->setFixedWidth(26);
            btn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
            btn->setToolTip("拾取节点");
            QHBoxLayout* h = new QHBoxLayout();
            h->setMargin(0);
            h->setSpacing(10);
            h->addWidget(lbl);
            h->addWidget(x);
            h->addWidget(y);
            h->addWidget(z);
            h->addWidget(btn);
            QWidget* rowWidget = new QWidget();
            rowWidget->setLayout(h);
            rowWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            return rowWidget;
        };

        tpV->addWidget(createPointRow("点1", tpX1, tpY1, tpZ1, tpBtn1));
        tpV->addWidget(createPointRow("点2", tpX2, tpY2, tpZ2, tpBtn2));
        tpV->addWidget(createPointRow("点3", tpX3, tpY3, tpZ3, tpBtn3));
        threePointWidget->setLayout(tpV);
        threePointWidget->setVisible(false);
        threePointWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        v->addWidget(threePointWidget);

        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(2);
        h0->setSpacing(10);
        h1->setSpacing(10);
        propwid->setMinimumWidth(250);
        propwid->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        propwid->setWindowFlags(Qt::Widget);
        propwid->setContentsMargins(0, 0, 0, 0);
        propwid->setLayout(v);

        connect(coorBtn, &QToolButton::clicked, this, &VecPickWidget::createCoor);

        connect(dirBtn, &QToolButton::clicked, this, [=]() {
            QStringList vector = vecEdit->text().split(",");
            if (vector.size() != 3)
                return;
            double vec0 = -vector[0].toDouble();
            double vec1 = -vector[1].toDouble();
            double vec2 = -vector[2].toDouble();
            QString vec = MxFunctions::mNumberToString(vec0) + "," + MxFunctions::mNumberToString(vec1) + "," + MxFunctions::mNumberToString(vec2);
            vecEdit->setText(vec);
            });

        // 根据当前相机视线计算屏幕方向并填入向量框：isNormal=true 取屏幕法向（垂直于屏幕），否则取屏幕平面内方向（平行于屏幕）
        auto applyScreenDir = [=](bool isNormal) {
            auto camCtrl = getCVM->getPreRenderWidget()->getCameraController();
            if (!camCtrl) return;
            glm::dvec3 eyeDir = camCtrl->getCameraState().eye_dir;
            QVector3D n(eyeDir.x, eyeDir.y, eyeDir.z);
            if (n.lengthSquared() < 1e-12) return;
            n.normalize();

            QVector3D dirVec = n;
            if (!isNormal)
            {
                // 平行于屏幕：取与法向最不平行的坐标轴做叉积，得到一个垂直于法向的单位向量
                QVector3D axis;
                double ax = qAbs(n.x()), ay = qAbs(n.y()), az = qAbs(n.z());
                if (ax <= ay && ax <= az)      axis = QVector3D(1, 0, 0);
                else if (ay <= az)             axis = QVector3D(0, 1, 0);
                else                           axis = QVector3D(0, 0, 1);
                dirVec = QVector3D::crossProduct(n, axis);
                if (dirVec.lengthSquared() < 1e-12) return;
                dirVec.normalize();
            }
            vecEdit->setText(QString("%1,%2,%3").arg(dirVec.x()).arg(dirVec.y()).arg(dirVec.z()));
        };

        connect(dirBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {
            int idx = dirBox->currentIndex();
            int threePointIdx = hasThreePoint ? 4 : -1;
            int screenNormalIdx = hasThreePoint ? 5 : 4;
            int screenParallelIdx = hasThreePoint ? 6 : 5;
            if (idx == 3)
            {
                if (hasThreePoint)
                    threePointWidget->setVisible(false);
                vecLayout->setVisible(true);
                vecEdit->setDisabled(false);
                vecBtn->setDisabled(false);
            }
            else if (idx == threePointIdx)
            {
                vecLayout->setVisible(false);
                vecEdit->setDisabled(true);
                vecBtn->setDisabled(true);
                threePointWidget->setVisible(true);
            }
            else if (idx == 0)
            {
                if (hasThreePoint)
                    threePointWidget->setVisible(false);
                vecLayout->setVisible(true);
                vecEdit->setText("1,0,0");
                vecEdit->setDisabled(true);
                vecBtn->setDisabled(true);
            }
            else if (idx == 1)
            {
                if (hasThreePoint)
                    threePointWidget->setVisible(false);
                vecLayout->setVisible(true);
                vecEdit->setText("0,1,0");
                vecEdit->setDisabled(true);
                vecBtn->setDisabled(true);
            }
            else if (idx == 2)
            {
                if (hasThreePoint)
                    threePointWidget->setVisible(false);
                vecLayout->setVisible(true);
                vecEdit->setText("0,0,1");
                vecEdit->setDisabled(true);
                vecBtn->setDisabled(true);
            }
            else if (idx == screenNormalIdx || idx == screenParallelIdx)
            {
                // 垂直/平行于屏幕：从当前相机视线方向取屏幕法向，再构造平行方向
                // 向量框只读，但保留加号按钮可用，允许改从拾取的几何边/面取方向
                if (hasThreePoint)
                    threePointWidget->setVisible(false);
                vecLayout->setVisible(true);
                vecEdit->setDisabled(true);
                vecBtn->setDisabled(false);

                applyScreenDir(idx == screenNormalIdx);
            }
            });
        connect(vecBtn, &QToolButton::clicked, this, [=]()
            {
                int idx = dirBox->currentIndex();
                int screenNormalIdx = hasThreePoint ? 5 : 4;
                int screenParallelIdx = hasThreePoint ? 6 : 5;
                if (idx == screenNormalIdx || idx == screenParallelIdx)
                {
                    // 屏幕方向模式下，加号按钮按当前相机视角刷新向量
                    applyScreenDir(idx == screenNormalIdx);
                    return;
                }
                QVector3D res;
                if (getPickVecData(res))
                {
                    QString result = QString::asprintf("%.1f,%.1f,%.1f", res.x(), res.y(), res.z());
                    vecEdit->setText(result);
                }
            });

        if (hasThreePoint)
        {
            setupThreePointPicker(tpBtn1, tpX1, tpY1, tpZ1);
            setupThreePointPicker(tpBtn2, tpX2, tpY2, tpZ2);
            setupThreePointPicker(tpBtn3, tpX3, tpY3, tpZ3);

            auto updateVecFromThreePoints = [=]() {
                QVector3D p1(tpX1->text().toDouble(), tpY1->text().toDouble(), tpZ1->text().toDouble());
                QVector3D p2(tpX2->text().toDouble(), tpY2->text().toDouble(), tpZ2->text().toDouble());
                QVector3D p3(tpX3->text().toDouble(), tpY3->text().toDouble(), tpZ3->text().toDouble());

                // 三点确定平面法向作为旋转轴方向
                QVector3D v1 = p2 - p1;
                QVector3D v2 = p3 - p1;
                QVector3D normal = QVector3D::crossProduct(v1, v2);
                qDebug() << "[VecPickWidget::三点确定] p1=" << p1 << "p2=" << p2 << "p3=" << p3
                         << "v1=" << v1 << "v2=" << v2 << "normal=" << normal;
                if (normal.lengthSquared() < 1e-12)
                {
                    qDebug() << "[VecPickWidget::三点确定] 三点共线/法向过小，无法确定旋转轴";
                    return;
                }
                normal.normalize();
                qDebug() << "[VecPickWidget::三点确定] 归一化旋转轴方向=" << normal;
                vecEdit->setText(QString("%1,%2,%3").arg(normal.x()).arg(normal.y()).arg(normal.z()));
            };

            connect(tpX1, &QLineEdit::textChanged, this, updateVecFromThreePoints);
            connect(tpY1, &QLineEdit::textChanged, this, updateVecFromThreePoints);
            connect(tpZ1, &QLineEdit::textChanged, this, updateVecFromThreePoints);
            connect(tpX2, &QLineEdit::textChanged, this, updateVecFromThreePoints);
            connect(tpY2, &QLineEdit::textChanged, this, updateVecFromThreePoints);
            connect(tpZ2, &QLineEdit::textChanged, this, updateVecFromThreePoints);
            connect(tpX3, &QLineEdit::textChanged, this, updateVecFromThreePoints);
            connect(tpY3, &QLineEdit::textChanged, this, updateVecFromThreePoints);
            connect(tpZ3, &QLineEdit::textChanged, this, updateVecFromThreePoints);
        }
    }

    void VecPickWidget::setupThreePointPicker(QToolButton* tool, mCalcLineEdit* editx, mCalcLineEdit* edity, mCalcLineEdit* editz)
    {
        connect(tool, &QToolButton::clicked, this, [=]() {
            auto pickMeshIDs = getCVM->getPreRenderWidget()
                ->getSelectionContainer()
                ->getMeshSelectData()
                ->getSelectDataByType(SelectionInfoType::MeshNode);

            if (pickMeshIDs.empty())
                return;

            QVector3D point = getCVM->getModelData()->getMeshData()->getVectorById(pickMeshIDs.back().data_id.node_id);
            editx->setText(MxFunctions::mNumberToString(point.x()));
            edity->setText(MxFunctions::mNumberToString(point.y()));
            editz->setText(MxFunctions::mNumberToString(point.z()));
            });
    }
    void VecPickWidget::createCoor()
    {

    }


}
