#include "mGeoSketchSignalManager.h"
#include <QToolButton>
#include <QMetaObject>
#include <QRegularExpression>
#include <QShortcut>
#include <functional>
#include <mViewportModelManager.h>
#include "mViewportModel.h"
#include "mGeoManager.h"
#include "mModelData.h"
#include "mBrowserWindow.h"
#include "mMechGlobalSignals.h"
#include "mTreeView.h"
#include "mMeshData.h"
//#include "mGeoCommandSolidOfSketchOCC.h" //【OCC摘除·S10/P0-1】退役头死 include（零引用）
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/CCSMPreRender.h"
#include "PreViewportWidget.h"
#include "InteractionCore/Controller/InteractionToolManager.h"
#include <InteractionCore/Tool/CreateSketch2DGeometryTool.h>
#include <InteractionCore/Tool/CreateSketchGeometryTool.h>
#include <InteractionCore/Tool/CreateSketchPlaneTool.h>
#include "mxr/basic/CameraController.h"
#include "mCenterMainWindow.h"
#include "mShortcutManager.h"
#include "mShortcutActionList.h"
#include "mInfoWindow.h"
#include "mGeoPanelBase.h"

#include "mGeoEventDispatcher.h"
#include "mGeoEvent.h"
#include "mGeoServerTypes.h"
#include "mGeoPrimitiveTypes.h"
#include "mGeoServer.h"
//#include "mGeoSketchAlgorithm.h" //【OCC摘除·S10/P0-1 终局2】死 include（零使用；OCC 链头）

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
using namespace MechCommon;
namespace MechMainWindow
{
    // 全局单例指针
    static mGeoSketchSignalManager* g_signalManager = nullptr;

    //解析"x,y,z"格式的点坐标文本，格式错误返回false
    static bool parsePointText(const QString& text, QVector3D& point)
    {
        static const QRegularExpression regExp(
            "^\\s*([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*,\\s*"
            "([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*,\\s*"
            "([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*$");
        QRegularExpressionMatch match = regExp.match(text);
        if (!match.hasMatch())
            return false;

        bool xOk = false, yOk = false, zOk = false;
        const float x = match.captured(1).toFloat(&xOk);
        const float y = match.captured(3).toFloat(&yOk);
        const float z = match.captured(5).toFloat(&zOk);
        if (!xOk || !yOk || !zOk)
            return false;

        point = QVector3D(x, y, z);
        return true;
    }

    mGeoSketchSignalManager* mGeoSketchSignalManager::instance()
    {
        if (!g_signalManager) {
            g_signalManager = new mGeoSketchSignalManager();
        }
        return g_signalManager;
    }

    mGeoSketchSignalManager::mGeoSketchSignalManager(QObject* parent)
        : QObject(parent)
    {
    }

    mGeoSketchSignalManager::~mGeoSketchSignalManager()
    {
        // 清理所有连接
        clearAllConnections();
    }

    void mGeoSketchSignalManager::enterSketch(SketchPlane plane)
    {
        // 重复入口只切换交互状态，不能覆盖当前会话或留下未收尾的草图部件。
        if (isSketch)
            return;

        //注销ESC快捷键
        ShortcutManager::instance()->unregisterShortcut(ShortcutModule::Global, "clearPick");
        /************************************************************************** */
        //进入 2D 草图前清理 3D 创建会话遗留的连接：2D 草图与 3D 创建共用
        // signal_GeometrySketchPlaneFinish，遗留的 3D 回调会在草图选平面时
        //再连一条几何创建信号（UniqueConnection 对 lambda 无效，连接会累积），
        //之后一次创建信号会被两个处理器各建一个部件（同参数重叠、视口只见一个）
        cleanupPreviousState(mxr::SketchGModeType::CUBE); //参数仅占位，函数内全量清理
        //上一次草图的平面完成连接同样会随成员覆盖而累积，先断开
        if (sketchPlaneFinishConnection)
        {
            disconnect(sketchPlaneFinishConnection);
            sketchPlaneFinishConnection = QMetaObject::Connection();
        }
        /************************************************************************** */
        isSketch = true;
        hasCompound = false;
        mBrowserWindow::getInstance()->getBrowserTree()->setSketch(true);
        /*m_SketchOCC = mGeoCommandFactory::create2DGeoSketch();
        m_SketchOCC->setGeoDataBase(getCVM->getModelData()->getGeoData());*/
        //空部件不再预建，改为首个 2D 图形创建时惰性建部件（ensureSketchPart）：
        //预建的空部件有 ID、无几何、视口不可见，每进一次草图就多占一个部件 ID，
        //破坏"部件 ID 数量 == 视口可见部件数量"的对应关系。
       //创建一个空部件
       // EventDispatcher::mGeoPartRequest req;
       // req.name = "Sketch";
      //  EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createEmptyPart(req);

       // if (res.succeeded())
       // {
       //    int modelDataID = getCVM->getModelData()->getModelDataID();
       //    curPartID = res.generatedObjects[0].id.id;
       //    getCVM->getModelData()->getGeoManager()->registerCreatedPart(curPartID);
       // }
        //临时代码：注册约束测试入口（F9），渲染组交互交付后移除
        setupTestConstraintHook();
        //if (m_SketchOCC->createCompound())
        //{
        //    int modelDataID = getCVM->getModelData()->getModelDataID();
        //    MxFunctions::mGlobalSignals::getInstance()->createGeoModelSignal(m_SketchOCC->getGeoDataSet()->getPartID(), modelDataID);
        //    m_SketchOCC->sketchCompound.PartId = m_SketchOCC->getGeoDataSet()->getPartID();
        //    //getCVM->getModelData()->getMeshData()->getPartData(m_SketchOCC->sketchCompound.PartId)->setPartColor(QColor("#000000"));
        //}

        //启用创建平面模式
        getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::CreateSketchPlane);

        //连接平面创建完成信号
        sketchPlaneFinishConnection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
            &mxr::SignalEmitter::signal_GeometrySketchPlaneFinish,
            this, [this]() {

                if (!hasCompound)
                {
                    hasCompound = true;
                    //启用创建平面模式
                    getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::CreateSketch2DGeometry);

                    //设置模式类型
                    getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                        active<mxr::InteractionCreateSketch2DGeometryTool>()->setModeType(mxr::Sketch2DGeoType::NOTHING);

                    //进入草图模式时，根据对象捕捉复选框勾选状态设置吸附类型
                    auto centerWindow = mCenterMainWindow::instance();
                    if (centerWindow)
                    {
                        static const std::vector<mxr::AdsorbPointType> kIndexToAdsorb = {
                            mxr::AdsorbPointType::arcOrCircle,
                            mxr::AdsorbPointType::edgeMiddle,
                            mxr::AdsorbPointType::quarterEdge,
                            mxr::AdsorbPointType::faceCenter,
                            mxr::AdsorbPointType::geoPoint,
                            mxr::AdsorbPointType::intersection,
                            mxr::AdsorbPointType::planeGrid
                        };
                        auto checkedIndexes = centerWindow->getCheckedSnapTypes();
                        std::vector<mxr::AdsorbPointType> adsorbTypes;
                        for (int idx : checkedIndexes)
                        {
                            if (idx >= 0 && idx < static_cast<int>(kIndexToAdsorb.size()))
                            {
                                adsorbTypes.push_back(kIndexToAdsorb[idx]);
                            }
                        }
                        auto sketchTool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                            getTool<mxr::InteractionCreateSketch2DGeometryTool>(mxr::InteractionToolType::CreateSketch2DGeometry);
                        if (sketchTool)
                        {
                            if (adsorbTypes.empty())
                            {
                                sketchTool->enableAdsorbTypes(false);
                            }
                            else
                            {
                                sketchTool->enableAdsorbTypes(true, adsorbTypes);
                            }
                        }
                    }
                }
            }, Qt::UniqueConnection);


        //获取选择的平面
        std::vector<float> planeNormal;
        if (plane == XOY)
        {
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                active<mxr::InteractionCreateSketchPlaneTool>()->setSketchPlaneType(mxr::SketchPlaneType::SketchPlaneXOY_2D);
        }
        else if (plane == XOZ)
        {
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                active<mxr::InteractionCreateSketchPlaneTool>()->setSketchPlaneType(mxr::SketchPlaneType::SketchPlaneXOZ_2D);
        }
        else if (plane == YOZ)
        {
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                active<mxr::InteractionCreateSketchPlaneTool>()->setSketchPlaneType(mxr::SketchPlaneType::SketchPlaneYOZ_2D);
        }
        else if (plane == CUSTOM)
        {
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                active<mxr::InteractionCreateSketchPlaneTool>()->setSketchPlaneType(mxr::SketchPlaneType::SketchPlaneSelect_2D);
        }
        else if (plane == THREEPOINT)
        {
            //借用XOY平面类型进入草图以触发平面完成信号（自动切换到草绘工具），
            //随后由enterSketchByThreePoints调用setSketchPlane覆盖为三点确定的平面
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                active<mxr::InteractionCreateSketchPlaneTool>()->setSketchPlaneType(mxr::SketchPlaneType::SketchPlaneXOY_2D);
        }


    }
    /************************************************************************** */
    //惰性建草图部件：会话内首个 2D 图形创建时才分配部件 ID 并注册
    //（网格/模型树/渲染经 createGeoModelSignal_2 同轨，与原 enterSketch
    //预建路径完全一致）。curPartID 已有效则直接复用。
    bool mGeoSketchSignalManager::ensureSketchPart()
    {
        if (curPartID > 0)
            return true;

        EventDispatcher::mGeoPartRequest req;
        req.name = "Sketch";
        EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createEmptyPart(req);
        if (!res.succeeded() || res.generatedObjects.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("草图部件创建失败！"));
            return false;
        }
        curPartID = res.generatedObjects[0].id.id;
        getCVM->getModelData()->getGeoManager()->registerCreatedPart(curPartID);
        return true;
    }
    /************************************************************************** */
    void mGeoSketchSignalManager::exitSketch()
    {
        //幂等保护：部分调用路径会先后两次触发退出
        //（如"结束草图绘制"先切页签经 slot_CurrentTabChanged 退出一次，再显式调用一次），
        //重复执行会导致 ESC 快捷键重复注册等副作用
        if (!isSketch)
            return;

        //重新注册ESC快捷键
        ShortcutManager::instance()->registerShortcut(ShortcutModule::Global, "clearPick",
            { QKeySequence(Qt::Key_Escape),"清空全部拾取",
              [] { ShortcutActionList::instance()->clearPick(); } });

        isSketch = false;
        mBrowserWindow::getInstance()->getBrowserTree()->setSketch(false);

        //临时代码：注销约束测试入口，渲染组交互交付后移除
        teardownTestConstraintHook();

        clearButtonSelection();

        //关闭创建平面模式
        //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreateSketch2DGeomtry(false);
        if (getCVM != nullptr && getCVM->getPreViewportWidget() != nullptr)
        {
            auto tool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->active<mxr::InteractionCreateSketch2DGeometryTool>();
            if (tool != nullptr)
                tool->setModeType(mxr::Sketch2DGeoType::NOTHING);
            //设置模式类型
      //getCVM->getPreRenderWidget()->getStateMachineManager()->set2DGeoType(mxr::Sketch2DGeoType::NOTHING);
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);

        }
      
        GeoNeutralData::GN_Part* part = getGEOServer.context().dataManager().findPart(curPartID);
        if (part != nullptr)
        {
            if (part->getShape() == 0 || (curPlaneNVec[0] == 0.0 && curPlaneNVec[1] == 0.0 && curPlaneNVec[2] == 0.0))
            {
                std::set<int> ids = { curPartID };
                // 撤消接入：空草图部件清理直写中性库（removePart），手工三段式包裹；
                // undo 后部件从快照恢复，树/渲染/_partdata 镜像由全量刷新重建。
                void* historyToken = getGEOServer.historyOperationBegin();
                getCVM->getModelData()->getGeoManager()->deletePart(ids);
                getCVM->getModelData()->getMeshData()->deleteGeoPart(ids);
                MxFunctions::mGlobalSignals::getInstance()->deletePartSig(ids);
                getGEOServer.context().dataManager().removePart(curPartID);
                getGEOServer.historyOperationSucceeded(
                    historyToken, EventDispatcher::mGeoOperationKind::Delete);
            }
            else
            {
                //创建面请求
                mGeoSketchCreateFacesRequest req;
                req.curPlaneNVec[0] = curPlaneNVec[0];
                req.curPlaneNVec[1] = curPlaneNVec[1];
                req.curPlaneNVec[2] = curPlaneNVec[2];
                req.curPlanePoint[0] = curPlanePoint[0];
                req.curPlanePoint[1] = curPlanePoint[1];
                req.curPlanePoint[2] = curPlanePoint[2];
                req.partID = curPartID;
                req.name = "createSketchFaces";

                EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createFaces(req);
                if (res.succeeded())
                {
                    int modelDataID = getCVM->getModelData()->getModelDataID();
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                }
            }
            curPartID = 0;
            curPlaneNVec[0] = 0.0;
            curPlaneNVec[1] = 0.0;
            curPlaneNVec[2] = 0.0;
            curPlanePoint[0] = 0.0;
            curPlanePoint[1] = 0.0;
            curPlanePoint[2] = 0.0;
        }

        //if (m_SketchOCC)
                //{
        //    if (m_SketchOCC->sketchCompound.isEmpty())
        //    {
        //        std::set<int> ids = { m_SketchOCC->sketchCompound.PartId };
        //        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet(ids);
        //        getCVM->getModelData()->getMeshData()->deleteGeoPart(ids);
        //        MxFunctions::mGlobalSignals::getInstance()->deletePartSig(ids);
        //    }
        //    else
        //    {
        //        ////自交分割
        //        //if (m_SketchOCC->SplitEdge())
        //        //{
        //        //    int modelDataID = getCVM->getModelData()->getModelDataID();
        //        //    MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
        //        //    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
        //        //}

        //        ////线线分割
        //        //if (m_SketchOCC->splitLines())
        //        //{
        //        //    int modelDataID = getCVM->getModelData()->getModelDataID();
        //        //    MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
        //        //    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
        //        //}

        //        ////创建封闭线框
        //        //if (m_SketchOCC->CreateFaceByWire())
        //        //{
        //        //    int modelDataID = getCVM->getModelData()->getModelDataID();
        //        //    MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
        //        //    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
        //        //}

        //        if (m_SketchOCC->CreateSketchFaces())
        //        {
                //    int modelDataID = getCVM->getModelData()->getModelDataID();
                //    MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
                //    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
        //        }

        //        delete m_SketchOCC;
        //        m_SketchOCC = nullptr;
        //    }
                //}

        hasCompound = false;
        disconnect(sketchPlaneFinishConnection);

        //m_SketchOCC 已随【OCC摘除·S10/P0-1】退役（成员注释保档于头文件），此处不再释放

        //状态清理完成后通知：菜单栏移除"草图"动态页签（此时 currentStatus 已为 false，
        //移除页签引发的页签切换不会再次进入 exitSketch）
        emit sketchExited();
    }

    void mGeoSketchSignalManager::showThreePointPlanePanel()
    {
        if (!m_threePointPanel)
            m_threePointPanel = createThreePointPlanePanel();

        mInfoWindow::getInstance()->showCustomPanel(m_threePointPanel);
    }

    QWidget* mGeoSketchSignalManager::createThreePointPlanePanel()
    {
        QWidget* panel = new QWidget();
        QVBoxLayout* panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(0, 0, 0, 0);

        QLineEdit* point1Edit = new QLineEdit(panel);
        point1Edit->setPlaceholderText("0,0,0");
        MxFunctions::mEditDiagnosis::setOnlyVec3(point1Edit);

        QLineEdit* point2Edit = new QLineEdit(panel);
        point2Edit->setPlaceholderText("100,0,0");
        MxFunctions::mEditDiagnosis::setOnlyVec3(point2Edit);

        QLineEdit* point3Edit = new QLineEdit(panel);
        point3Edit->setPlaceholderText("0,100,0");
        MxFunctions::mEditDiagnosis::setOnlyVec3(point3Edit);

        QPushButton* cancelButton = new QPushButton("取消", panel);
        QPushButton* okButton = new QPushButton("确认", panel);
        connect(cancelButton, &QPushButton::clicked, this, []() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            });
        connect(okButton, &QPushButton::clicked, this, [this, point1Edit, point2Edit, point3Edit]() {
            QVector3D p1, p2, p3;
            if (!parsePointText(point1Edit->text(), p1) || !parsePointText(point2Edit->text(), p2) ||
                !parsePointText(point3Edit->text(), p3))
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("坐标格式错误，请按\"x,y,z\"格式输入，如 1,0,0！"));
                return;
            }

            QVector3D point, normal;
            if (!threePointsToPlane(p1, p2, p3, point, normal))
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("三点共线或重合，无法确定草图平面！"));
                return;
            }

            //关闭面板并进入草图模式
            mInfoWindow::getInstance()->unEditInfoWindow();
            mMechGlobalSignals::getInstance()->switchMenuBarWindow("草图");
            enterSketchByThreePoints(p1, p2, p3);
            });

        FoldSection* threePointFold = new FoldSection("三点确定草图平面", panel);
        threePointFold->addRow(new QLabel("点1坐标", panel), point1Edit);
        threePointFold->addRow(new QLabel("点2坐标", panel), point2Edit);
        threePointFold->addRow(new QLabel("点3坐标", panel), point3Edit);
        threePointFold->addRow(cancelButton, okButton, "Left");

        panelLayout->addWidget(threePointFold);
        panelLayout->addStretch();
        return panel;
    }

    bool mGeoSketchSignalManager::threePointsToPlane(const QVector3D& p1, const QVector3D& p2, const QVector3D& p3,
        QVector3D& point, QVector3D& normal)
    {
        //法向为三点构成的三角形两条边的叉积
        QVector3D n = QVector3D::crossProduct(p2 - p1, p3 - p1);
        if (n.length() < 1e-6f) //三点共线或重合
            return false;

        normal = n.normalized();

        //平面经过三点，取质心作为平面上的点
        point = (p1 + p2 + p3) / 3.0f;
        return true;
    }

    void mGeoSketchSignalManager::enterSketchByThreePoints(const QVector3D& p1, const QVector3D& p2, const QVector3D& p3)
    {
        QVector3D point, normal;
        if (!threePointsToPlane(p1, p2, p3, point, normal))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("三点共线或重合，无法确定草图平面！"));
            return;
        }

        enterSketch(THREEPOINT);

        //用三点确定的平面覆盖默认平面
        auto tool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->
            active<mxr::InteractionCreateSketchPlaneTool>();
        if (tool)
            tool->setSketchPlane(point, normal);

        ////将相机对准草图平面（与手动选择平面进入草图的行为一致）。
        ////否则倾斜平面上屏幕xy方向与平面UV方向混合，鼠标无法独立控制矩形等图元的长宽
        //auto rendWidget = getCVM->getPreViewportWidget()->renderWidget();
        //if (rendWidget)
        //{
        //    auto camera = rendWidget->getCameraController();
        //    if (camera)
        //    {
        //        camera->setView(glm::vec3(point.x(), point.y(), point.z()),
        //            glm::vec3(normal.x(), normal.y(), normal.z()),
        //            mxr::SketchPlaneType::SketchPlane_Solid);
        //    }
        //}
    }

    void mGeoSketchSignalManager::connectGeometryButton(QToolButton* button, mxr::SketchGModeType modeType)
    {
        //断开该按钮之前的连接
        auto buttonIt = m_buttonConnections.find(button);
        if (buttonIt != m_buttonConnections.end()) {
            disconnect(*buttonIt);
            m_buttonConnections.erase(buttonIt);
        }

        //断开该模式类型之前的平面完成连接
        auto planeIt = m_planeFinishConnections.find(modeType);
        if (planeIt != m_planeFinishConnections.end()) {
            disconnect(*planeIt);
            m_planeFinishConnections.erase(planeIt);
        }

        //断开该模式类型之前的几何创建连接
        auto geoIt = m_geometryCreateConnections.find(modeType);
        if (geoIt != m_geometryCreateConnections.end()) {
            disconnect(*geoIt);
            m_geometryCreateConnections.erase(geoIt);
        }

        //存储按钮和模式的映射关系
        m_buttonModeMap[button] = modeType;

        //连接按钮点击事件
        auto buttonConnection = connect(button, &QToolButton::clicked, this, [this, modeType, button]() {
            //设置按钮选中状态
            selectButton(button);

            //清理之前可能存在的状态
            cleanupPreviousState(modeType);

            //启用创建平面模式
            //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreateSketchPlane(true);
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::CreateSketchPlane);

            //连接平面创建完成信号
            auto planeFinishConnection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_GeometrySketchPlaneFinish,
                this, [this, modeType]() {

                    ////关闭草绘创建模式
                    //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreateSketch2DGeomtry(false);
                    ////设置模式类型
                    //getCVM->getPreRenderWidget()->getStateMachineManager()->setModeType(modeType);
                    ////启用几何体创建模式
                    //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreateSketchGeomtry(true);

                    getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::CreateSketchGeometry);
                    getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                        active<mxr::InteractionCreateSketchGeometryTool>()->setModeType(modeType);

                    //连接几何创建信号
                    connectGeometryCreationSignal(modeType);
                }, Qt::UniqueConnection);

            //存储平面完成连接
            m_planeFinishConnections[modeType] = planeFinishConnection;

            }, Qt::UniqueConnection);

        //存储按钮连接
        m_buttonConnections[button] = buttonConnection;
    }

    void mGeoSketchSignalManager::selectButton(QToolButton* button)
    {
        clearButtonSelection();
        if (button) {
            button->setStyleSheet("QToolButton { background-color: #D0E7FD; }");
            m_selectedButton = button;
        }
    }

    void mGeoSketchSignalManager::clearButtonSelection()
    {
        if (m_selectedButton) {
            m_selectedButton->setStyleSheet("");
            m_selectedButton = nullptr;
        }
    }

    void mGeoSketchSignalManager::cleanupPreviousState(mxr::SketchGModeType modeType)
    {
        // 断开所有平面完成连接
        for (auto& conn : m_planeFinishConnections)
        {
            disconnect(conn);
        }
        m_planeFinishConnections.clear();

        // 断开所有几何创建连接
        for (auto& conn : m_geometryCreateConnections)
        {
            disconnect(conn);
        }
        m_geometryCreateConnections.clear();

        // 断开所有草图创建连接
        for (auto& conn : m_SketchCreateConnections)
        {
            disconnect(conn);
        }
        m_SketchCreateConnections.clear();
    }

    void mGeoSketchSignalManager::connectSketchGeoButton(QToolButton* button, mxr::Sketch2DGeoType modeType)
    {
        //断开该按钮之前的连接
        auto buttonIt = m_buttonConnections.find(button);
        if (buttonIt != m_buttonConnections.end()) {
            disconnect(*buttonIt);
            m_buttonConnections.erase(buttonIt);
        }

        //断开该模式类型之前的几何创建连接
        auto geoIt = m_SketchCreateConnections.find(modeType);
        if (geoIt != m_SketchCreateConnections.end()) {
            disconnect(*geoIt);
            m_SketchCreateConnections.erase(geoIt);
        }

        //存储按钮和模式的映射关系
        m_buttonSketchMap[button] = modeType;

        //连接按钮点击事件
        auto buttonConnection = connect(button, &QToolButton::clicked, this, [this, modeType, button]() {

            if (!hasCompound)
                return;

            //设置按钮选中状态
            selectButton(button);

            //清理之前可能存在的状态
            cleanupSketchPreviousState(modeType);

            //关闭几何体创建模式
            //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreateSketchGeomtry(false);


            //设置模式类型
            //getCVM->getPreRenderWidget()->getStateMachineManager()->set2DGeoType(modeType);
            //getCVM->getPreRenderWidget()->getStateMachineManager()->set2DGeoType(modeType);
            auto tool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                getTool<mxr::InteractionCreateSketch2DGeometryTool>(mxr::InteractionToolType::CreateSketch2DGeometry);
            tool->setModeType(modeType);
            //启用草绘创建模式
            //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreateSketch2DGeomtry(true);

            //连接草绘创建信号
            connectSketchCreationSignal(modeType);

            }, Qt::UniqueConnection);

        //存储按钮连接
        m_buttonConnections[button] = buttonConnection;
    }

    void mGeoSketchSignalManager::connectSketchConstraintButton(QToolButton* button)
    {
        //断开该按钮之前的连接
        auto buttonIt = m_buttonConnections.find(button);
        if (buttonIt != m_buttonConnections.end()) {
            disconnect(*buttonIt);
            m_buttonConnections.erase(buttonIt);
        }

        auto buttonConnection = connect(button, &QToolButton::clicked, this, [this, button]() {

            //设置按钮选中状态
            selectButton(button);

            //退出绘制模式，等待约束拾取
            auto tool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                getTool<mxr::InteractionCreateSketch2DGeometryTool>(mxr::InteractionToolType::CreateSketch2DGeometry);
            if (tool) tool->setModeType(mxr::Sketch2DGeoType::NOTHING);

            //约束拾取与尺寸值键盘输入由渲染组在
            // InteractionCreateSketch2DGeometryTool 中实现后生效：
            //   setConstraintType(...) -> 拾取 -> Enter 后 emit
            //   signal_SketchDimensionConstraint(mGeoSketchDimensionInput)
            // 界面侧已具备完整的求解与刷新链路。
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                tr("尺寸约束已就绪：拾取与值输入交互待渲染组工具支持。"));

            }, Qt::UniqueConnection);

        //存储按钮连接
        m_buttonConnections[button] = buttonConnection;
    }

    void mGeoSketchSignalManager::submitDimensionConstraint(const EventDispatcher::mGeoSketchDimensionInput& input)
    {
        EventDispatcher::mGeoSketchAddConstraintRequest req;
        req.kind = input.kind;
        req.ref1 = input.ref1;
        req.ref2 = input.ref2;
        req.value = input.value;
        req.driving = true;
        req.partID = curPartID;

        //平面优先取输入携带（工具从草图网格获取），否则兜底当前草图记录
        bool hasInputPlane = input.NVec[0] != 0.f || input.NVec[1] != 0.f || input.NVec[2] != 0.f;
        for (int i = 0; i < 3; ++i)
        {
            req.NVec[i] = hasInputPlane ? input.NVec[i] : curPlaneNVec[i];
            req.planePoint[i] = hasInputPlane ? input.planePoint[i] : curPlanePoint[i];
        }

        EventDispatcher::mGeoQueryResult<EventDispatcher::mGeoSketchSolveResult> res =
            getGEOServer.sketches().addConstraint(req);

        if (res.succeeded())
        {
            //求解后草图 Part 的 shape 已重建，记录并触发刷新
            getGEOServer.context().PartManager()._editPart.insert(curPartID);
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();

            QString msg = tr("尺寸约束求解成功，剩余自由度: %1").arg(res.value.dof);
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(msg);

            //TODO(渲染组): 就地更新交互工具显示态与持久尺寸标注，接口由渲染组实现后启用：
            //auto tool = ...getTool<mxr::InteractionCreateSketch2DGeometryTool>(...);
            //tool->applySketchSolveResult(res.value);
        }
        else
        {
            QString msg = tr("尺寸约束求解失败");
            if (!res.value.conflictingConstraintIds.empty())
            {
                QStringList ids;
                for (int id : res.value.conflictingConstraintIds) ids << QString::number(id);
                msg += tr("，冲突约束ID: %1").arg(ids.join(","));
            }
            if (!res.value.redundantConstraintIds.empty())
            {
                QStringList ids;
                for (int id : res.value.redundantConstraintIds) ids << QString::number(id);
                msg += tr("，冗余约束ID: %1").arg(ids.join(","));
            }
            if (!res.message.empty())
            {
                msg += QStringLiteral(" (") + QString::fromStdString(res.message) + QStringLiteral(")");
            }
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(msg);
        }
    }

    void mGeoSketchSignalManager::slot_addSketchDimensionConstraint(const EventDispatcher::mGeoSketchDimensionInput& input)
    {
        submitDimensionConstraint(input);
    }

    //临时代码：应用内约束测试入口。进入草图后按 F9：
    //  - 第 1 次：对会话内首个线元素(geoId=0)提交长度=100 的距离约束，
    //             期望：线长变为 100、3D 刷新、消息栏显示剩余自由度；
    //  - 第 2 次：同一线再提交一个约束，期望：求解失败、几何不变、消息栏报告冲突约束ID（验证回滚）。
    // 依赖前提：进入草图后先画一条线（线是会话的第 0 个元素）。
    //渲染组交互交付后整段移除。
    void mGeoSketchSignalManager::setupTestConstraintHook()
    {
        if (m_testConstraintShortcut != nullptr)
            return;
        QWidget* viewport = getCVM->getPreViewportWidget();
        if (viewport == nullptr)
            return;
        m_testConstraintShortcut = new QShortcut(QKeySequence(Qt::Key_F9), viewport);
        connect(m_testConstraintShortcut, &QShortcut::activated, this, [this]() {
            if (!isSketch || curPartID <= 0)
                return;
            EventDispatcher::mGeoSketchDimensionInput input;
            input.kind = EventDispatcher::mGeoSketchConstraintKind::Distance;
            input.ref1.geoId = 0;
            input.ref1.pointPos = EventDispatcher::mGeoSketchPointPos::Start;
            input.ref2.geoId = 0;
            input.ref2.pointPos = EventDispatcher::mGeoSketchPointPos::End;
            input.value = 100.0;
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                tr("[测试] F9：对首条线提交长度=100 的尺寸约束..."));
            submitDimensionConstraint(input);
            });
    }

    void mGeoSketchSignalManager::teardownTestConstraintHook()
    {
        if (m_testConstraintShortcut != nullptr)
        {
            m_testConstraintShortcut->deleteLater();
            m_testConstraintShortcut = nullptr;
        }
    }

    void mGeoSketchSignalManager::clearAllConnections()
    {
        // 断开所有平面完成连接
        for (auto& conn : m_planeFinishConnections)
        {
            disconnect(conn);
        }
        m_planeFinishConnections.clear();

        // 断开所有几何创建连接
        for (auto& conn : m_geometryCreateConnections)
        {
            disconnect(conn);
        }
        m_geometryCreateConnections.clear();

        // 断开所有草图创建连接
        for (auto& conn : m_SketchCreateConnections)
        {
            disconnect(conn);
        }
        m_SketchCreateConnections.clear();

        // 断开所有按钮连接
        for (auto& conn : m_buttonConnections)
        {
            disconnect(conn);
        }
        m_buttonConnections.clear();

        // 清空映射
        m_buttonModeMap.clear();
        m_buttonSketchMap.clear();

        // 清除按钮选中状态
        clearButtonSelection();
    }

    void mGeoSketchSignalManager::connectGeometryCreationSignal(mxr::SketchGModeType modeType)
    {
        /************************************************************************** */
        //同模式先断旧连接再连（Qt::UniqueConnection 对 lambda 无效）：平面完成
        // lambda 每次选平面都会重连一次，不去重会累积连接——一次创建信号被
        //多个处理器各建一个部件（同参数重叠不可见，部件 ID 数多于可见部件）
        auto geoIt = m_geometryCreateConnections.find(modeType);
        if (geoIt != m_geometryCreateConnections.end()) {
            disconnect(geoIt.value());
            m_geometryCreateConnections.erase(geoIt);
        }
        /************************************************************************** */
        switch (modeType)
        {
        case mxr::SketchGModeType::CUBE:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_GeometryCreateBox,
                this, [this](const float NVec[3], const float vertexA[3], const float vertexC[3], const float distance) {
                    clearButtonSelection();
                    if (distance > 1e+8)
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("立方体创建失败,输入尺寸过大！"));
                        return;
                    }

                    //创建请求
                    EventDispatcher::mGeoBoxRequest makeBoxReq;
                    makeBoxReq.way = createBoxWay::DiagonalAndExtrusion;
                    makeBoxReq.vec[0] = NVec[0];
                    makeBoxReq.vec[1] = NVec[1];
                    makeBoxReq.vec[2] = NVec[2];
                    makeBoxReq.vA[0] = vertexA[0];
                    makeBoxReq.vA[1] = vertexA[1];
                    makeBoxReq.vA[2] = vertexA[2];
                    makeBoxReq.vC[0] = vertexC[0];
                    makeBoxReq.vC[1] = vertexC[1];
                    makeBoxReq.vC[2] = vertexC[2];
                    makeBoxReq.distance = distance;

                    EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createBox(makeBoxReq);
                    //确保执行成功并且生成了Compound和Part
                    if (res.succeeded() && res.generatedObjects.size() == 2)
                    {
                        //拿出生成的部件ID

                        getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    }
                    else
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("立方体创建失败！"));
                    }

                }, Qt::UniqueConnection);
            m_geometryCreateConnections[mxr::SketchGModeType::CUBE] = connection;
        }
        break;

        case mxr::SketchGModeType::CYLINDER:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_GeometryCreateCylinder,
                this, [this](const float NVec[3], const float circlePnt[3], const float radius, const float distance) {
                    clearButtonSelection();
                    if (distance > 1e+8 || radius > 1e+8)
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆柱创建失败,输入尺寸过大！"));
                        return;
                    }

                    //创建请求
                    EventDispatcher::mGeoCylinderRequest Req;
                    Req.NVec[0] = NVec[0];
                    Req.NVec[1] = NVec[1];
                    Req.NVec[2] = NVec[2];
                    Req.circlePnt[0] = circlePnt[0];
                    Req.circlePnt[1] = circlePnt[1];
                    Req.circlePnt[2] = circlePnt[2];
                    Req.radius = radius;
                    Req.distance = distance;

                    EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createCylinder(Req);
                    //确保执行成功并且生成了Compound和Part
                    if (res.succeeded() && res.generatedObjects.size() == 2)
                    {
                        //拿出生成的部件ID

                        getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    }
                    else
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆柱创建失败！"));
                    }
                }, Qt::UniqueConnection);
            m_geometryCreateConnections[mxr::SketchGModeType::CYLINDER] = connection;
        }
        break;

        case mxr::SketchGModeType::SPHERE:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_GeometryCreateSphere,
                this, [this](const float NVec[3], const float circlePnt[3], const float radius) {
                    clearButtonSelection();
                    if (radius > 1e+8)
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("球体创建失败,输入尺寸过大！"));
                        return;
                    }

                    //创建请求
                    EventDispatcher::mGeoSphereRequest Req;
                    Req.NVec[0] = NVec[0];
                    Req.NVec[1] = NVec[1];
                    Req.NVec[2] = NVec[2];
                    Req.circlePnt[0] = circlePnt[0];
                    Req.circlePnt[1] = circlePnt[1];
                    Req.circlePnt[2] = circlePnt[2];
                    Req.radius = radius;

                    EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createSphere(Req);
                    //确保执行成功并且生成了Compound和Part
                    if (res.succeeded() && res.generatedObjects.size() == 2)
                    {
                        //拿出生成的部件ID

                        getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    }
                    else
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("球体创建失败！"));
                    }
                }, Qt::UniqueConnection);
            m_geometryCreateConnections[mxr::SketchGModeType::SPHERE] = connection;
        }
        break;

        case mxr::SketchGModeType::CONOID:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_GeometryCreateConoid,
                this, [this](const float NVec[3], const float circlePnt[3], const float radius, const float distance) {
                    clearButtonSelection();
                    if (distance > 1e+8 || radius > 1e+8)
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆锥创建失败,输入尺寸过大！"));
                        return;
                    }

                    //创建请求
                    EventDispatcher::mGeoConeRequest Req;
                    Req.NVec[0] = NVec[0];
                    Req.NVec[1] = NVec[1];
                    Req.NVec[2] = NVec[2];
                    Req.circlePnt[0] = circlePnt[0];
                    Req.circlePnt[1] = circlePnt[1];
                    Req.circlePnt[2] = circlePnt[2];
                    Req.radius = radius;
                    Req.distance = distance;

                    EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createCone(Req);
                    //确保执行成功并且生成了Compound和Part
                    if (res.succeeded() && res.generatedObjects.size() == 2)
                    {
                        //拿出生成的部件ID

                        getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    }
                    else
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆锥创建失败！"));
                    }
                }, Qt::UniqueConnection);
            m_geometryCreateConnections[mxr::SketchGModeType::CONOID] = connection;
        }
        break;

        case mxr::SketchGModeType::TORUS:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_GeometryCreateTorus,
                this, [this](const float NVec[3], const float circlePnt[3], const float BRadius, const float SRadius) {
                    clearButtonSelection();
                    if (BRadius > 1e+8 || SRadius > 1e+8)
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆环创建失败,输入尺寸过大！"));
                        return;
                    }

                    //创建请求
                    EventDispatcher::mGeoTorusRequest Req;
                    Req.NVec[0] = NVec[0];
                    Req.NVec[1] = NVec[1];
                    Req.NVec[2] = NVec[2];
                    Req.circlePnt[0] = circlePnt[0];
                    Req.circlePnt[1] = circlePnt[1];
                    Req.circlePnt[2] = circlePnt[2];
                    Req.BRadius = BRadius;
                    Req.SRadius = SRadius;

                    EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createTorus(Req);
                    //确保执行成功并且生成了Compound和Part
                    if (res.succeeded() && res.generatedObjects.size() == 2)
                    {
                        //拿出生成的部件ID

                        getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    }
                    else
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆环创建失败！"));
                    }
                }, Qt::UniqueConnection);
            m_geometryCreateConnections[mxr::SketchGModeType::TORUS] = connection;
        }
        break;

        case mxr::SketchGModeType::FRUSTUM:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_GeometryCreateFrustum,
                this, [this](const float NVec[3], const float BCircle[3], const float BRadius, const float SCircle[3], const float SRadius, const float distance) {
                    clearButtonSelection();
                    if (distance > 1e+8 || BRadius > 1e+8)
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆台创建失败,输入尺寸过大！"));
                        return;
                    }

                    //创建请求
                    EventDispatcher::mGeoFrustumRequest Req;
                    Req.NVec[0] = NVec[0];
                    Req.NVec[1] = NVec[1];
                    Req.NVec[2] = NVec[2];
                    Req.BCircle[0] = BCircle[0];
                    Req.BCircle[1] = BCircle[1];
                    Req.BCircle[2] = BCircle[2];
                    Req.BRadius = BRadius;
                    Req.SCircle[0] = SCircle[0];
                    Req.SCircle[1] = SCircle[1];
                    Req.SCircle[2] = SCircle[2];
                    Req.SRadius = SRadius;
                    Req.distance = distance;

                    EventDispatcher::mGeoOperationResult res = getGEOServer.primitives().createFrustum(Req);
                    //确保执行成功并且生成了Compound和Part
                    if (res.succeeded() && res.generatedObjects.size() == 2)
                    {
                        //拿出生成的部件ID

                        getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    }
                    else
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆台创建失败！"));
                    }
                }, Qt::UniqueConnection);
            m_geometryCreateConnections[mxr::SketchGModeType::FRUSTUM] = connection;
        }
        break;
        default:
            break;
        }
    }

    void mGeoSketchSignalManager::cleanupSketchPreviousState(mxr::Sketch2DGeoType modeType)
    {
        // 断开所有平面完成连接
        for (auto& conn : m_planeFinishConnections)
        {
            disconnect(conn);
        }
        m_planeFinishConnections.clear();

        // 断开所有几何创建连接
        for (auto& conn : m_geometryCreateConnections)
        {
            disconnect(conn);
        }
        m_geometryCreateConnections.clear();

        // 断开所有草图创建连接
        for (auto& conn : m_SketchCreateConnections)
        {
            disconnect(conn);
        }
        m_SketchCreateConnections.clear();
    }

    void mGeoSketchSignalManager::connectSketchCreationSignal(mxr::Sketch2DGeoType modeType)
    {
        switch (modeType)
        {
        case mxr::Sketch2DGeoType::LINE:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometryLine,
                this, &mGeoSketchSignalManager::slot_create2DGeometryLine, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::LINE] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::POINT:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometryPoint,
                this, &mGeoSketchSignalManager::slot_create2DGeometryPoint, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::POINT] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::CIRCLE:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometryCircle,
                this, &mGeoSketchSignalManager::slot_create2DGeometryCircle, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::CIRCLE] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::RECTANGLE:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometryRectangle,
                this, &mGeoSketchSignalManager::slot_create2DGeometryRectangle, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::RECTANGLE] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::SPLINE:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometrySpline,
                this, &mGeoSketchSignalManager::slot_create2DGeometrySPLine, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::SPLINE] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::THREEPOINTCIRCLE:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometry3PointCircle,
                this, &mGeoSketchSignalManager::slot_create2DGeometryCircle, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::THREEPOINTCIRCLE] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::THREEPOINTARC:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometry3PointArc,
                this, &mGeoSketchSignalManager::slot_create2DGeometry3PointArc, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::THREEPOINTARC] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::ELLIPSE:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometryEllipse,
                this, &mGeoSketchSignalManager::slot_create2DGeometryEllipse, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::ELLIPSE] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::POLYGON:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometryPolygon,
                this, &mGeoSketchSignalManager::slot_create2DGeometryPolygon, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::POLYGON] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::PARALLELOGRAM:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometryParallelogram,
                this, &mGeoSketchSignalManager::slot_create2DGeometryParallelogram, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::PARALLELOGRAM] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::ELLIPSEARC:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometryEllipseArc,
                this, &mGeoSketchSignalManager::slot_create2DGeometryEllipseArc, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::ELLIPSEARC] = connection;
            break;
        }
        case mxr::Sketch2DGeoType::SWEEPARC:
        {
            auto connection = connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
                &mxr::SignalEmitter::signal_Create2DGeometrySweepArc,
                this, &mGeoSketchSignalManager::slot_create2DGeometrySweepArc, Qt::UniqueConnection);
            m_SketchCreateConnections[mxr::Sketch2DGeoType::SWEEPARC] = connection;
            break;
        }

        default:
            break;
        }
    }

    void mGeoSketchSignalManager::slot_create2DGeometryLine(const float NVec[3], const std::vector <std::pair<std::vector<float>, std::vector<float>>>& linePnte)
    {
        curPlaneNVec[0] = NVec[0];
        curPlaneNVec[1] = NVec[1];
        curPlaneNVec[2] = NVec[2];
        curPlanePoint[0] = linePnte[0].first[0];
        curPlanePoint[1] = linePnte[0].first[1];
        curPlanePoint[2] = linePnte[0].first[2];
        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreateLineRequest req;
        req.linePnte = linePnte;
        req.partID = curPartID;
        //线/点请求本身不含平面，补充法向供约束会话建立平面
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createLine(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("线段创建失败！"));
        }
        /*m_SketchOCC->setGeoDataBase(getCVM->getModelData()->getGeoData());
        if (m_SketchOCC->Create2DGeometryLine(NVec, linePnte))
        {
            int modelDataID = getCVM->getModelData()->getModelDataID();
            MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
            MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("线段创建失败！"));
        }*/
    }

    void mGeoSketchSignalManager::slot_create2DGeometryPoint(const float NVec[3], const float pointPnt[3])
    {
        curPlaneNVec[0] = NVec[0];
        curPlaneNVec[1] = NVec[1];
        curPlaneNVec[2] = NVec[2];
        curPlanePoint[0] = pointPnt[0];
        curPlanePoint[1] = pointPnt[1];
        curPlanePoint[2] = pointPnt[2];

        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreatePointRequest req;
        req.pointPnt[0] = pointPnt[0];
        req.pointPnt[1] = pointPnt[1];
        req.pointPnt[2] = pointPnt[2];
        req.partID = curPartID;
        //线/点请求本身不含平面，补充法向供约束会话建立平面
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createPoint(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("点创建失败！"));
        }
        /*m_SketchOCC->setGeoDataBase(getCVM->getModelData()->getGeoData());
        if (m_SketchOCC->Create2DGeometryPoint(pointPnt))
        {
            int modelDataID = getCVM->getModelData()->getModelDataID();
            MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
            MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("点创建失败！"));
        }*/
    }

    void mGeoSketchSignalManager::slot_create2DGeometryCircle(const float NVec[3], const float circlePnt[3], const float radius)
    {
        curPlaneNVec[0] = NVec[0];
        curPlaneNVec[1] = NVec[1];
        curPlaneNVec[2] = NVec[2];
        curPlanePoint[0] = circlePnt[0];
        curPlanePoint[1] = circlePnt[1];
        curPlanePoint[2] = circlePnt[2];

        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreateCircleRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.circlePnt[0] = circlePnt[0];
        req.circlePnt[1] = circlePnt[1];
        req.circlePnt[2] = circlePnt[2];
        req.radius = radius;
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createCircle(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆创建失败！"));
        }
        /*m_SketchOCC->setGeoDataBase(getCVM->getModelData()->getGeoData());
        if (m_SketchOCC->Create2DGeometryCircle(NVec, circlePnt, radius))
        {
            int modelDataID = getCVM->getModelData()->getModelDataID();
            MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
            MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("圆创建失败！"));
        }*/
    }

    void mGeoSketchSignalManager::slot_create2DGeometryRectangle(const float NVec[3], const float vertexA[3], const float vertexB[3], const float vertexC[3], const float vertexD[3])
    {
        curPlaneNVec[0] = NVec[0];
        curPlaneNVec[1] = NVec[1];
        curPlaneNVec[2] = NVec[2];
        curPlanePoint[0] = vertexA[0];
        curPlanePoint[1] = vertexA[1];
        curPlanePoint[2] = vertexA[2];

        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreateRectangleRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.vertexA[0] = vertexA[0];
        req.vertexA[1] = vertexA[1];
        req.vertexA[2] = vertexA[2];
        req.vertexB[0] = vertexB[0];
        req.vertexB[1] = vertexB[1];
        req.vertexB[2] = vertexB[2];
        req.vertexC[0] = vertexC[0];
        req.vertexC[1] = vertexC[1];
        req.vertexC[2] = vertexC[2];
        req.vertexD[0] = vertexD[0];
        req.vertexD[1] = vertexD[1];
        req.vertexD[2] = vertexD[2];
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createRectangle(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("矩形创建失败！"));
        }
    }

    void mGeoSketchSignalManager::slot_create2DGeometrySPLine(const float NVec[3], const std::vector <std::vector<float>>& vertexs)
    {
        if (!vertexs.empty())
        {
            curPlaneNVec[0] = NVec[0];
            curPlaneNVec[1] = NVec[1];
            curPlaneNVec[2] = NVec[2];
            curPlanePoint[0] = vertexs[0][0];
            curPlanePoint[1] = vertexs[0][1];
            curPlanePoint[2] = vertexs[0][2];
        }
        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreateSPLineRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.vertexs = vertexs;
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createSPLine(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("样条曲线创建失败！"));
        }
    }

    void mGeoSketchSignalManager::slot_create2DGeometry3PointArc(const float NVec[3], const float circlePnt[3], const std::vector<std::vector<float>>& vertexs)
    {
        if (!vertexs.empty())
        {
            curPlaneNVec[0] = NVec[0];
            curPlaneNVec[1] = NVec[1];
            curPlaneNVec[2] = NVec[2];
            curPlanePoint[0] = vertexs[0][0];
            curPlanePoint[1] = vertexs[0][1];
            curPlanePoint[2] = vertexs[0][2];
        }
        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreate3PointArcRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.vertexs = vertexs;
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().create3PointArc(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("三点圆弧创建失败！"));
        }
    }

    void mGeoSketchSignalManager::slot_create2DGeometryEllipse(const float NVec[3], const float centerPnt[3], const float vertexA[3], const float vertexB[3])
    {
        curPlaneNVec[0] = NVec[0];
        curPlaneNVec[1] = NVec[1];
        curPlaneNVec[2] = NVec[2];
        curPlanePoint[0] = centerPnt[0];
        curPlanePoint[1] = centerPnt[1];
        curPlanePoint[2] = centerPnt[2];
        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreateEllipseRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.centerPnt[0] = centerPnt[0];
        req.centerPnt[1] = centerPnt[1];
        req.centerPnt[2] = centerPnt[2];
        req.vertexA[0] = vertexA[0];
        req.vertexA[1] = vertexA[1];
        req.vertexA[2] = vertexA[2];
        req.vertexB[0] = vertexB[0];
        req.vertexB[1] = vertexB[1];
        req.vertexB[2] = vertexB[2];
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createEllipse(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("椭圆创建失败！"));
        }
    }

    void mGeoSketchSignalManager::slot_create2DGeometryPolygon(const float NVec[3], const float centerPnt[3], const std::vector<std::vector<float>>& vertexs)
    {
        if (!vertexs.empty())
        {
            curPlaneNVec[0] = NVec[0];
            curPlaneNVec[1] = NVec[1];
            curPlaneNVec[2] = NVec[2];
            curPlanePoint[0] = vertexs[0][0];
            curPlanePoint[1] = vertexs[0][1];
            curPlanePoint[2] = vertexs[0][2];
        }
        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreatePolygonRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.centerPnt[0] = centerPnt[0];
        req.centerPnt[1] = centerPnt[1];
        req.centerPnt[2] = centerPnt[2];
        req.vertexs = vertexs;
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createPolygon(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("正多边形创建失败！"));
        }
    }

    void mGeoSketchSignalManager::slot_create2DGeometryParallelogram(const float NVec[3], const float vertexA[3], const float vertexB[3], const float vertexC[3], const float vertexD[3])
    {
        curPlaneNVec[0] = NVec[0];
        curPlaneNVec[1] = NVec[1];
        curPlaneNVec[2] = NVec[2];
        curPlanePoint[0] = vertexA[0];
        curPlanePoint[1] = vertexA[1];
        curPlanePoint[2] = vertexA[2];
        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreateParallelogramRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.vertexA[0] = vertexA[0];
        req.vertexA[1] = vertexA[1];
        req.vertexA[2] = vertexA[2];
        req.vertexB[0] = vertexB[0];
        req.vertexB[1] = vertexB[1];
        req.vertexB[2] = vertexB[2];
        req.vertexC[0] = vertexC[0];
        req.vertexC[1] = vertexC[1];
        req.vertexC[2] = vertexC[2];
        req.vertexD[0] = vertexD[0];
        req.vertexD[1] = vertexD[1];
        req.vertexD[2] = vertexD[2];
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createParallelogram(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("平行四边形创建失败！"));
        }
    }

    void mGeoSketchSignalManager::slot_create2DGeometryEllipseArc(const float NVec[3], const float centerPnt[3], const float vertexA[3], const float vertexB[3], const std::vector<std::vector<float>>& vertexs)
    {
        curPlaneNVec[0] = NVec[0];
        curPlaneNVec[1] = NVec[1];
        curPlaneNVec[2] = NVec[2];
        curPlanePoint[0] = centerPnt[0];
        curPlanePoint[1] = centerPnt[1];
        curPlanePoint[2] = centerPnt[2];
        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreateEllipseArcRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.centerPnt[0] = centerPnt[0];
        req.centerPnt[1] = centerPnt[1];
        req.centerPnt[2] = centerPnt[2];
        req.vertexA[0] = vertexA[0];
        req.vertexA[1] = vertexA[1];
        req.vertexA[2] = vertexA[2];
        req.vertexB[0] = vertexB[0];
        req.vertexB[1] = vertexB[1];
        req.vertexB[2] = vertexB[2];
        req.vertexs = vertexs;
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createEllipseArc(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("椭圆弧创建失败！"));
        }
    }

    void mGeoSketchSignalManager::slot_create2DGeometrySweepArc(const float NVec[3], const float centerPnt[3], const float vertexStart[3], const float vertexEnd[3], const std::vector<std::vector<float>>& vertexs)
    {
        curPlaneNVec[0] = NVec[0];
        curPlaneNVec[1] = NVec[1];
        curPlaneNVec[2] = NVec[2];
        curPlanePoint[0] = centerPnt[0];
        curPlanePoint[1] = centerPnt[1];
        curPlanePoint[2] = centerPnt[2];
        /************************************************************************** */
        if (!ensureSketchPart())
            return;
        /************************************************************************** */
        EventDispatcher::mGeoSketchCreateSweepArcRequest req;
        req.NVec[0] = NVec[0];
        req.NVec[1] = NVec[1];
        req.NVec[2] = NVec[2];
        req.centerPnt[0] = centerPnt[0];
        req.centerPnt[1] = centerPnt[1];
        req.centerPnt[2] = centerPnt[2];
        req.vertexStart[0] = vertexStart[0];
        req.vertexStart[1] = vertexStart[1];
        req.vertexStart[2] = vertexStart[2];
        req.vertexEnd[0] = vertexEnd[0];
        req.vertexEnd[1] = vertexEnd[1];
        req.vertexEnd[2] = vertexEnd[2];
        req.vertexs = vertexs;
        req.partID = curPartID;

        EventDispatcher::mGeoOperationResult res = getGEOServer.sketches().createSweepArc(req);

        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("扫掠弧创建失败！"));
        }
    }
}