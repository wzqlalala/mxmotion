//【OCC摘除·S10/P0-1 终局】直模控制器实现整体保档下线：
// 原 WIP 机构（TopoDS 择取/OCC History 脚本上下文/预览工作线程/stub 几何会话）
// 功能上自 S9 起恒经 stub 返回失败——本中性壳与其真实行为一致，零功能损失；
// 引擎化迁移时移除下方 #if 0/#endif 恢复原实现，按
// Basic/RebuildGeometry/doc/2026-10-07/05-直模中性化与MainWindow摘链方案.md §三
// 改走引擎编辑族（editSurfaceOfOffsetOCC 等）。
#include "mDirectModelingController.h"
#include "mGeoFeatureParameterCalculator.h"
#include "mGlobalSignals.h"
#include "mViewportModelManager.h" //【直模引擎化·S11】getCVM/getGEOServer（引擎提交链）
#include "mViewportModel.h" //【直模引擎化·S11】getCVM->getModelData() 完整类型
#include "mGeoManager.h" //【ModelID分离·命令直调】getGeoManager()->syncAfterShapeEdit() 完整类型
#include "mGeoDirectModelingTypes.h" //【直模引擎化·S11】直模引擎契约
#include "mGeoNeutralQuery.h" //【直模引擎化·S11·GUI 预览源】GN 部件离散网格检索

#include <QHash>
#include <QDebug> //【X线取证】neutralPreviewSource 数据诊断日志

#if 0 // ===================== 原 WIP 实现（保档，勿删） =====================
#include "mDirectModelingController.h"
#include "mGeoDevPreviewGate.h" //【GUI 门控】直模 V2（RotateFace/瞬态预览/驱动重建扩展）依赖未推送 API

// RebuildGUI 内部模块
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mGeoManager.h"
#include "mGeoDataBase.h"
#include "mGeoDataModelSet.h"
#include "mGeoGlobalSignals.h"
#include "mGlobalSignals.h"
#include "LowCPlusDefine.h"

// OCC
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Builder.hxx>
#include <GeomAbs_CurveType.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopTools_DataMapIteratorOfDataMapOfIntegerShape.hxx>
#include <TopTools_DataMapOfIntegerShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <gp_Lin.hxx>
#include <gp_Vec.hxx>

#include <QMetaObject>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QWaitCondition>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>

/*
 * Controller 是 UI/业务数据与纯 OCC 几何层之间的协调器：
 * - UI 线程：解析拾取 ID、维护会话、接收拖动参数、执行最终提交；
 * - 几何线程：对只读 DirectModelingGeometry 快照生成预览和离散数据；
 * - 渲染线程边界：只通过 std::vector<glm::vec3> 信号传递顶点，不传 OCC 对象。
 *
 * “预览成功”不等于“正式几何已修改”。只有 onCommit -> finish -> updatePart
 * 全部成功后，才替换 GeoDataModelSet 并发出 commitFinished(true)。
 */
namespace
{
    /**
     * 数据层的不同版本不一定都继承 QObject。仅在支持时记录弱生命周期，
     * 不拥有数据库，也不把不支持弱指针的版本强制转换成 QObject。
     */
    template<typename T>
    QPointer<QObject> DirectParameterDataOwner(T* data)
    {
        if constexpr (std::is_convertible<T*, QObject*>::value)
            return QPointer<QObject>(data);
        else
            return QPointer<QObject>();
    }

    /**
     * 参数历史不能保存 GeoDataModelSet 中会被重新分配的面/边 ID，因此保存目标
     * 子形状在“该操作输入 Shape”同类型拓扑表中的位置和数量。参数回放时只有
     * 数量未变化才按该位置重新定位；无法可靠定位时宁可判为失效，也不修改错误面。
     */
    struct TopologyLocator
    {
        TopAbs_ShapeEnum shapeType = TopAbs_SHAPE;
        int index = 0;
        int sourceCount = 0;
        bool external = false;
        TopoDS_Shape externalShape;
    };

    bool CaptureTopologyLocator(
        const TopoDS_Shape& parentShape,
        const TopoDS_Shape& targetShape,
        bool allowExternal,
        TopologyLocator& locator,
        QString* errorMessage)
    {
        locator = TopologyLocator();
        if (targetShape.IsNull())
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("参数操作引用的几何对象为空");
            return false;
        }

        locator.shapeType = targetShape.ShapeType();
        TopTools_IndexedMapOfShape subShapes;
        TopExp::MapShapes(parentShape, locator.shapeType, subShapes);
        locator.index = subShapes.FindIndex(targetShape);
        locator.sourceCount = subShapes.Extent();
        if (locator.index > 0)
            return true;

        if (allowExternal)
        {
            locator.external = true;
            locator.externalShape = targetShape;
            return true;
        }

        if (errorMessage)
            *errorMessage = QStringLiteral("参数操作的选择对象不属于其输入几何");
        return false;
    }

    TopoDS_Shape ResolveTopologyLocator(
        const TopoDS_Shape& currentShape,
        const TopologyLocator& locator,
        QString* errorMessage)
    {
        if (locator.external)
        {
            if (!locator.externalShape.IsNull())
                return locator.externalShape;
            if (errorMessage)
                *errorMessage = QStringLiteral("外部参照几何已经失效");
            return TopoDS_Shape();
        }

        TopTools_IndexedMapOfShape subShapes;
        TopExp::MapShapes(currentShape, locator.shapeType, subShapes);
        if (subShapes.Extent() != locator.sourceCount)
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral(
                    "前序参数改变了拓扑数量，无法可靠定位原选择对象（原 %1，现 %2）")
                    .arg(locator.sourceCount)
                    .arg(subShapes.Extent());
            }
            return TopoDS_Shape();
        }
        if (locator.index <= 0 || locator.index > subShapes.Extent())
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("原选择对象的拓扑位置已经失效");
            return TopoDS_Shape();
        }
        return subShapes.FindKey(locator.index);
    }

    // Qt 与渲染层使用不同向量类型，在工作线程计算结束后一次性转换并 move 给 UI。
    std::vector<glm::vec3> ToRenderPoints(
        const QVector<QVector3D>& previewPoints)
    {
        std::vector<glm::vec3> renderPoints;
        renderPoints.reserve(static_cast<std::size_t>(previewPoints.size()));
        for (const QVector3D& point : previewPoints)
            renderPoints.emplace_back(point.x(), point.y(), point.z());
        return renderPoints;
    }

}

namespace MechMainWindow
{
    /**
     * 一次 JSON 脚本运行中的稳定拓扑身份表。
     *
     * initialId/initialShape 永远指向该脚本第一次接触部件时的状态；
     * currentId/currentShape 则在每次成功提交前通过 BRepTools_History 原子推进。
     * 四个维度分别保存，避免数值相同的点 ID 与面 ID 互相覆盖。
     */
    struct DirectModelingController::ScriptInitialTopologyIdContext
    {
        struct TrackedTopology
        {
            int initialId = -1;
            int currentId = -1;
            int partId = -1;
            TopoDS_Shape initialShape;
            TopoDS_Shape currentShape;
        };

        struct TrackedPart
        {
            int partId = -1;
            // GeoDataModelSet 是 QObject。脚本步骤之间若有其他编辑路径删除部件，
            // QPointer 会自动归零，避免初始 ID 查询解引用悬空裸指针。
            QPointer<MGeoData::GeoDataModelSet> currentSet;
            TopoDS_Shape initialPartShape;
            std::array<int, 4> topologyCounts{ { 0, 0, 0, 0 } };
        };

        QString contextId;
        MechPreData::mViewportModel* viewportModel = nullptr; // 非拥有指针
        MGeoData::GeoDataBase* geoData = nullptr;             // 非拥有指针
        bool valid = true;
        QString failureReason;
        std::array<QHash<int, TrackedTopology>, 4> topologies;
        QHash<int, TrackedPart> parts;
    };

    /**
     * 单消费者、只保留最新请求的预览线程。
     *
     * submit() 不建立无界队列：新请求直接覆盖尚未开始的旧请求。若计算期间又收到
     * 新请求，旧结果完成后也不会向 UI 发布，从而避免拖动时堆积过时网格。
     */
    class DirectModelingPreviewWorker final : public QThread
    {
    public:
        explicit DirectModelingPreviewWorker(DirectModelingController* receiver)
            : m_receiver(receiver)
        {
        }

        ~DirectModelingPreviewWorker() override
        {
            shutdown();
        }

        void submit(
            quint64 sessionId,
            quint64 sequence,
            //std::shared_ptr<const MGeoCommand::DirectModelingGeometry> geometry, //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
            double value,
            bool exact)
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopping)
                return;

            // pending 只有一个槽位；连续拖动天然合并为最后一个参数。
            m_pendingRequest.sessionId = sessionId;
            m_pendingRequest.sequence = sequence;
            m_pendingRequest.geometry = std::move(geometry);
            m_pendingRequest.value = value;
            m_pendingRequest.exact = exact;
            m_hasPendingRequest = true;
            m_requestCondition.wakeOne();
        }

        void cancelPendingAndWait()
        {
            // 提交前等待正在运行的 OCC 计算结束，避免正式提交与后台预览同时
            // 遍历共享 TShape/缓存而进入 OCC 的非线程安全路径。
            QMutexLocker locker(&m_mutex);
            m_hasPendingRequest = false;
            m_pendingRequest = Request();
            while (m_busy)
                m_idleCondition.wait(&m_mutex);
        }

        void shutdown()
        {
            {
                QMutexLocker locker(&m_mutex);
                if (m_stopping)
                    return;
                m_stopping = true;
                m_hasPendingRequest = false;
                m_pendingRequest = Request();
                m_requestCondition.wakeOne();
            }
            wait();
        }

    protected:
        void run() override
        {
            for (;;)
            {
                Request request;
                {
                    QMutexLocker locker(&m_mutex);
                    while (!m_stopping && !m_hasPendingRequest)
                        m_requestCondition.wait(&m_mutex);
                    if (m_stopping)
                        return;

                    request = std::move(m_pendingRequest);
                    m_pendingRequest = Request();
                    m_hasPendingRequest = false;
                    m_busy = true;
                }

                // 锁只保护请求槽和状态；耗时 OCC 计算期间必须释放锁，否则 UI
                // 无法投递更新请求或安全关闭线程。
                //MGeoCommand::GeoPreviewData added; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                //MGeoCommand::GeoPreviewData removed; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                QString errorMessage;
                std::vector<glm::vec3> addedFacePoints;
                std::vector<glm::vec3> addedEdgePoints;
                std::vector<glm::vec3> removedFacePoints;
                std::vector<glm::vec3> removedEdgePoints;
                bool success = false;
                bool resultReady = false;
                // 异常只终止本次请求，不能逃出 QThread::run。点转换同样可能因
                // 内存分配等原因抛出异常，必须与 OCC 计算共用这一请求级边界。
                try
                {
                    //【OCC摘除·S9·直模下线】旧命令预览离散链（previewDiscretized 族）随
                    // 冻结岛退役——预览恒失败（走 errorMessage 分支）；迁移引擎后恢复
                    success = false;
                    errorMessage = QStringLiteral("direct modeling preview migrating to engine (S9)");
                    if (!request.geometry)
                        errorMessage = tr("后台预览会话无效");

                    bool shouldConvert = false;
                    {
                        QMutexLocker locker(&m_mutex);
                        shouldConvert = !m_stopping && !m_hasPendingRequest;
                    }
                    if (success && shouldConvert && m_receiver)
                    {
                        addedFacePoints = ToRenderPoints(added.facePoint);
                        addedEdgePoints = ToRenderPoints(added.edgePoint);
                        removedFacePoints = ToRenderPoints(removed.facePoint);
                        removedEdgePoints = ToRenderPoints(removed.edgePoint);
                        resultReady = true;
                    }
                    else if (!success)
                        resultReady = true;
                }
                catch (const Standard_Failure& failure)
                {
                    success = false;
                    resultReady = true;
                    const char* detail = failure.GetMessageString();
                    errorMessage = tr("后台预览几何计算异常：%1")
                        .arg(detail && *detail ? QString::fromUtf8(detail) : tr("未知 OCC 异常"));
                }
                catch (const std::exception& failure)
                {
                    success = false;
                    resultReady = true;
                    errorMessage = tr("后台预览计算或数据转换异常：%1")
                        .arg(QString::fromUtf8(failure.what()));
                }
                catch (...)
                {
                    success = false;
                    resultReady = true;
                    errorMessage = tr("后台预览发生未知异常，本次预览已取消");
                }
                if (!success)
                {
                    // 不允许把算法/转换中途留下的半份数据作为失败结果传给渲染。
                    added.facePoint.clear();
                    added.edgePoint.clear();
                    removed.facePoint.clear();
                    removed.edgePoint.clear();
                    addedFacePoints.clear();
                    addedEdgePoints.clear();
                    removedFacePoints.clear();
                    removedEdgePoints.clear();
                }

                bool shouldPublish = false;
                {
                    QMutexLocker locker(&m_mutex);
                    // 无论几何计算或点转换是否失败都释放等待者；转换结束之前仍算
                    // busy，避免正式提交/关闭线程与后台数据处理交错。
                    m_busy = false;
                    // 忙碌期间若已有更新请求，当前结果已经过时，不再传输大块网格数据。
                    // resultReady 防止跳过过时转换后，取消请求又清空 pending 时误发空成功结果。
                    shouldPublish = resultReady && !m_stopping && !m_hasPendingRequest;
                    m_idleCondition.wakeAll();
                }

                if (!shouldPublish || !m_receiver)
                    continue;

                // QPointer 防止 Controller 在队列回调执行前析构。invokeMethod
                // 明确回到 receiver 所在线程，信号发布仍发生在 UI 线程。
                QPointer<DirectModelingController> receiver = m_receiver;
                QMetaObject::invokeMethod(
                    receiver.data(),
                    [receiver,
                    sessionId = request.sessionId,
                    sequence = request.sequence,
                    success,
                    errorMessage = std::move(errorMessage),
                    addedFacePoints = std::move(addedFacePoints),
                    addedEdgePoints = std::move(addedEdgePoints),
                    removedFacePoints = std::move(removedFacePoints),
                    removedEdgePoints = std::move(removedEdgePoints)]() mutable {
                        if (!receiver)
                            return;
                        receiver->handlePreviewResult(
                            sessionId,
                            sequence,
                            success,
                            std::move(errorMessage),
                            std::move(addedFacePoints),
                            std::move(addedEdgePoints),
                            std::move(removedFacePoints),
                            std::move(removedEdgePoints));
                    },
                    Qt::QueuedConnection);
            }
        }

    private:
        struct Request
        {
            quint64 sessionId = 0;
            quint64 sequence = 0;
            //std::shared_ptr<const MGeoCommand::DirectModelingGeometry> geometry; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
            double value = 0.0;
            bool exact = false;
        };

        QPointer<DirectModelingController> m_receiver;
        QMutex m_mutex;
        QWaitCondition m_requestCondition;
        QWaitCondition m_idleCondition;
        Request m_pendingRequest;
        bool m_hasPendingRequest = false;
        bool m_busy = false;
        bool m_stopping = false;
    };

    /**
     * 一次已确认的参数驱动直接建模历史节点。
     *
     * sourcePartShape 是该节点第一次确认时的输入，只用于记录拓扑定位信息；真正
     * 重建时从同一部件第一条活动历史的基准 Shape 开始，将活动节点依次应用到上一条
     * 输出。currentPartSet 会在数据层创建新部件后统一重定向，避免保留悬空指针。
     */
    struct DirectModelingController::DrivenOperation
    {
        QString parameterId;
        // 仅用于消息显示。真正的参数身份和回放取值始终使用不可变 parameterId，
        // 因此用户修改显示名称不会断开已确认的几何历史。
        QString parameterName;
        MGeoCommand::DirectModelingOperation operation =
            MGeoCommand::DirectModelingOperation::None;
        // 面操作的作用域必须进入历史节点；否则修改参数后回放会把实体平移误做成
        // 普通移动面操作，得到与首次确定不同的几何。
        bool moveSolid = false;
        MechPreData::mViewportModel* viewportModel = nullptr;
        // 参数属于几何数据库，而不属于“当前活动视口”。有弱生命周期保护时，
        // 即使用户切换视口也可安全核对原数据库；数据库销毁后绝不解引用裸指针。
        MGeoData::GeoDataBase* geoData = nullptr;
        QPointer<QObject> geoDataOwner;
        bool tracksGeoDataOwner = false;
        // 历史缺失后，以当前 Shape 建立新的回放基准。基准之前的效果已烘焙到
        // Shape，旧节点仅保留参数名称/公式/归属，不可重新回放，否则会丢失断链
        // 处的几何贡献。普通 retarget/refresh 不得把这些节点恢复为活动历史。
        bool historyRetired = false;
        // 参数历史可能跨越多次 UI/脚本操作；部件若在 Controller 外被替换或删除，
        // QPointer 会自动归零，回放时即可把该参数标记为失效，而不会解引用悬空对象。
        QPointer<MGeoData::GeoDataModelSet> currentPartSet;
        int partId = -1;
        TopoDS_Shape sourcePartShape;
        // 一个历史节点可以代表单选择或批量选择。所有定位器都相对于
        // sourcePartShape 捕获，回放时先全部解析，再一次性执行组合算法。
        std::vector<TopologyLocator> selectedLocators;
        std::vector<TopologyLocator> referenceLocators;
        double currentValue = 0.0;
    };

    DirectModelingController* DirectModelingController::s_instance = nullptr;

    DirectModelingController* DirectModelingController::getInstance()
    {
        if (!s_instance)
        {
            s_instance = new DirectModelingController();
        }
        return s_instance;
    }

    DirectModelingController::DirectModelingController(QObject* parent)
        : QObject(parent)
    {
        // 注册跨线程信号参数类型。GeoPreviewData 仅在 Controller 内部使用。
        qRegisterMetaType<std::vector<glm::vec3>>("std::vector<glm::vec3>");
        // 参数计算器不依赖 QObject；由 Controller 单独创建并在析构时自动释放。
        m_parameterCalculator =
            std::make_unique<mGeoFeatureParameterCalculator>();

        m_previewWorker = std::make_unique<DirectModelingPreviewWorker>(this);
        m_previewWorker->start();

        // 圆角等拓扑操作只在连续拖动暂停后计算一次预览。
        m_previewConsumeTimer = new QTimer(this);
        m_previewConsumeTimer->setSingleShot(true);
        m_previewConsumeTimer->setInterval(kTopologyPreviewDebounceMs);
        connect(m_previewConsumeTimer, &QTimer::timeout, this, [this]() {
            if (m_previewPending && m_geometry.hasActiveOperation())
            {
                m_previewPending = false;
                doPreview(m_pendingPreviewDistance, false);
            }
            });
    }

    DirectModelingController::~DirectModelingController()
    {
        invalidatePreviewSession();
        if (m_previewWorker)
            m_previewWorker->shutdown();
        exitDirectModelingMode();
    }

    bool DirectModelingController::updateDirectModelingParameterExpressions(
        const QHash<QString, QString>& expressionById,
        QString* errorMessage)
    {
        return updateDirectModelingParameters(
            QHash<QString, QString>(), expressionById, errorMessage);
    }

    bool DirectModelingController::updateDirectModelingParameters(
        const QHash<QString, QString>& nameById,
        const QHash<QString, QString>& expressionById,
        QString* errorMessage)
    {
        if (!m_parameterCalculator)
        {
            if (errorMessage)
                *errorMessage = tr("直接建模参数计算器未初始化");
            return false;
        }

        // Panel 每次会提交整张表，因此先按稳定 id 比较用户实际修改的公式。
        // 纯改名只会让计算器精确重写 {旧名} 引用，数值和几何均不改变，不应清除
        // 或重新计算既有几何校验状态；公式内容确有修改时仍沿用原来的回放逻辑。
        QHash<QString, QString> oldExpressionById;
        for (const GeoFeatureParameter& parameter : m_parameterCalculator->parameters())
            oldExpressionById.insert(parameter.id, parameter.expression);
        bool expressionChanged = false;
        for (auto it = expressionById.constBegin(); it != expressionById.constEnd(); ++it)
        {
            const auto oldExpression = oldExpressionById.constFind(it.key());
            if (oldExpression == oldExpressionById.constEnd()
                || oldExpression.value() != it.value())
            {
                expressionChanged = true;
                break;
            }
        }

        QString firstError;
        bool success = m_parameterCalculator->updateDefinitions(
            nameById, expressionById, &firstError);

        // 名称是可编辑的显示属性。历史节点只以稳定 id 绑定参数；这里同步最新名称
        // 仅用于错误消息，不把可变名称重新当成公式或历史键。
        const QVector<GeoFeatureParameter> parameters =
            m_parameterCalculator->parameters();
        QHash<QString, QString> parameterNameById;
        parameterNameById.reserve(parameters.size());
        for (const GeoFeatureParameter& parameter : parameters)
            parameterNameById.insert(parameter.id, parameter.name);
        for (const std::unique_ptr<DrivenOperation>& operation : m_drivenOperations)
        {
            if (operation && parameterNameById.contains(operation->parameterId))
                operation->parameterName = parameterNameById.value(operation->parameterId);
        }
        if (success && expressionChanged)
        {
            QString rebuildError;
            if (!rebuildDrivenOperations(&rebuildError))
            {
                success = false;
                firstError = rebuildError;
            }
        }
        if (errorMessage)
            *errorMessage = firstError;
        emit directModelingParametersChanged();
        return success;
    }

    bool DirectModelingController::evaluateDirectModelingExpression(
        const QString& expression,
        double& value,
        QString* errorMessage) const
    {
        if (!m_parameterCalculator)
        {
            if (errorMessage)
                *errorMessage = tr("直接建模参数计算器未初始化");
            return false;
        }
        return m_parameterCalculator->evaluateExpression(
            expression, value, errorMessage);
    }
    // ------------------------------------------------------------------
// 模式开关
// ------------------------------------------------------------------

    void DirectModelingController::enterDirectModelingMode()
    {
        // Panel 切换功能时即使模式已经激活，也可能有部件在外部被删除。先核对
        // 数据层，再判断是否需要启动模式，使每次打开直接建模都显示最新参数表。
        refreshDirectModelingParameters();
        if (m_isActive)
            return;

        if (!currentViewportModel() || !currentModelData())
        {
            outputMessage(2, tr("无法获取当前模型，进入直接建模模式失败"));
            return;
        }

        m_isActive = true;
        emit modeChanged(true);
        outputMessage(0, tr("已进入直接建模模式，请拾取几何面或几何边"));
    }

    void DirectModelingController::exitDirectModelingMode()
    {
        exitDirectModelingMode(true);
    }

    void DirectModelingController::exitDirectModelingMode(bool rebuildDrivenHistory)
    {
        if (!m_isActive)
            return;

        resetSession();
        if (rebuildDrivenHistory)
        {
            QString pendingDrivenError;
            if (!rebuildDrivenOperations(&pendingDrivenError))
                outputMessage(2, pendingDrivenError);
        }
        emit directModelingParametersChanged();
        emit arrowHidden();
        m_isActive = false;
        emit modeChanged(false);
        outputMessage(0, tr("已退出直接建模模式"));
    }

    bool DirectModelingController::beginOperation(int elementDim, int elementId, bool isOffset)
    {
        return beginOperationInternal(
            elementDim,
            elementId,
            MGeoCommand::DirectModelingOperation::None,
            isOffset,
            false);
    }

    bool DirectModelingController::beginOperation(
        int elementDim,
        int elementId,
        MGeoCommand::DirectModelingOperation operation)
    {
        return beginOperationInternal(elementDim, elementId, operation, false, false);
    }

    bool DirectModelingController::beginOperation(
        int elementDim,
        int elementId,
        MGeoCommand::DirectModelingOperation operation,
        bool moveSolid)
    {
        return beginOperationInternal(
            elementDim, elementId, operation, false, moveSolid);
    }

    bool DirectModelingController::beginOperationInternal(
        int elementDim,
        int elementId,
        MGeoCommand::DirectModelingOperation requestedOperation,
        bool isOffset,
        bool moveSolid)
    {
        if (!m_isActive)
        {
            outputMessage(1, tr("请先进入直接建模模式"), true);
            return false;
        }
        // 切换拾取前先使旧结果失效并等待旧计算；否则旧面 ID 对应的网格可能在
        // 新会话建立后才返回，造成预览串台。
        invalidatePreviewSession();
        if (m_previewWorker)
            m_previewWorker->cancelPendingAndWait();
        m_referenceShape = TopoDS_Shape();
        m_referenceShapeBelongsToCurrentPart = false;

        // 新拾取会替换旧会话，渲染侧应先清除上一份预览。
        if (m_geometry.hasActiveOperation())
        {
            emit previewClearRequested();
        }

        m_selectedElementDim = elementDim;
        m_selectedElementId = elementId;
        if (!startDirectModelingOperation(requestedOperation, isOffset, moveSolid))
            return false;

        preparePreviewSession();
#if MXGUI_HAS_DIRECTMODELING_V2
        if (m_currentOperation == MGeoCommand::DirectModelingOperation::RotateFace)
            emit arrowHidden();
        else
#endif
            emit arrowAvailable(m_arrowCenter, m_arrowDirection);
        return true;
    }

    bool DirectModelingController::beginReplaceFaceOperation(
        int sourceFaceId,
        int targetFaceId)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模功能后续迁移 RebuildGeometry（引擎编辑族）。
    return false;
    //【S9 原体（逐行注释保留）】
//        int sourceFaceId,
//        int targetFaceId)
//    {
//        if (!m_isActive)
//            return false;
//        m_moveSolid = false;
//        invalidatePreviewSession();
//        if (m_previewWorker)
//            m_previewWorker->cancelPendingAndWait();
//        m_referenceShape = TopoDS_Shape();
//        m_referenceShapeBelongsToCurrentPart = false;

//        TopoDS_Shape sourceShape;
//        TopoDS_Shape targetShape;
//        MGeoData::GeoDataModelSet* sourcePart = nullptr;
//        MGeoData::GeoDataModelSet* targetPart = nullptr;
//        if (!resolveSelection(2, sourceFaceId, sourceShape, sourcePart)
//            || !resolveSelection(2, targetFaceId, targetShape, targetPart))
//        {
//            return false;
//        }
//        // 目标面可来自另一部件，但正式修改对象始终是 sourcePart。
//        if (!sourcePart || !targetPart)
//        {
//            outputMessage(2, tr("无法获取替换面的源面或目标面所属部件"));
//            return false;
//        }
//        if (sourceShape.IsSame(targetShape))
//        {
//            outputMessage(2, tr("替换面的源面和目标面不能相同"));
//            return false;
//        }

//        if (m_geometry.hasActiveOperation())
//            emit previewClearRequested();
//        m_geometry.cancel();
//        m_geometry.setPartShape(sourcePart->getShape());

//        QString errorMessage;
//        if (!m_geometry.beginOperation(
//            sourceShape,
//            MGeoCommand::DirectModelingOperation::ReplaceFace,
//            MGeoCommand::DirectModelingResultMode::Update,
//            &errorMessage)
//#if MXGUI_HAS_DIRECTMODELING_V2
//            || !m_geometry.setReplacementFace(TopoDS::Face(targetShape))
//#endif
//            )
//        {
//            outputMessage(2, tr("启动替换面失败：%1").arg(errorMessage));
//            m_geometry.cancel();
//            return false;
//        }
//        m_selectedElementDim = 2;
//        m_selectedElementId = sourceFaceId;
//        m_currentPartSet = sourcePart;
//        m_currentOperation = MGeoCommand::DirectModelingOperation::ReplaceFace;
//        m_referenceShape = targetShape;
//        m_referenceShapeBelongsToCurrentPart = sourcePart == targetPart;
//        m_currentParameter = 0.0;
//        const MGeoCommand::DirectSelectionInfo info = m_geometry.activeSelection();
//        m_arrowCenter = info.arrowCenter;
//        m_arrowDirection = info.arrowDirection;
//        preparePreviewSession();
//        emit arrowHidden();
//        return true;
//    }
}

    bool DirectModelingController::beginRotateFaceOperation(
        int sourceFaceId)
    {
#if !MXGUI_HAS_DIRECTMODELING_V2
        return false; //【GUI 门控】RotateFace 依赖未推送
#else
        return beginOperationInternal(
            2,
            sourceFaceId,
            MGeoCommand::DirectModelingOperation::RotateFace,
            false,
            false);
    #endif
}

    bool DirectModelingController::beginRotateFaceOperation(
        int sourceFaceId,
        int axisEdgeId)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模功能后续迁移 RebuildGeometry（引擎编辑族）。
    return false;
    //【S9 原体（逐行注释保留）】
//        int sourceFaceId,
//        int axisEdgeId)
//    {
//#if !MXGUI_HAS_DIRECTMODELING_V2
//        return false; //【GUI 门控】RotateFace 依赖未推送
//#else
//        if (!m_isActive)
//            return false;
//        m_moveSolid = false;
//        invalidatePreviewSession();
//        if (m_previewWorker)
//            m_previewWorker->cancelPendingAndWait();
//        m_referenceShape = TopoDS_Shape();
//        m_referenceShapeBelongsToCurrentPart = false;

//        TopoDS_Shape sourceShape;
//        TopoDS_Shape axisShape;
//        MGeoData::GeoDataModelSet* sourcePart = nullptr;
//        MGeoData::GeoDataModelSet* axisPart = nullptr;
//        if (!resolveSelection(2, sourceFaceId, sourceShape, sourcePart)
//            || !resolveSelection(1, axisEdgeId, axisShape, axisPart))
//        {
//            return false;
//        }
//        if (!sourcePart || sourcePart != axisPart)
//        {
//            outputMessage(2, tr("旋转面和旋转轴必须属于同一部件"));
//            return false;
//        }

//        BRepAdaptor_Curve curve(TopoDS::Edge(axisShape));
//        if (curve.GetType() != GeomAbs_Line)
//        {
//            outputMessage(2, tr("旋转轴必须选择直线边"));
//            return false;
//        }
//        const gp_Lin line = curve.Line();
//        const MGeoCommand::DirectModelingGeometry sourceGeometry(
//            sourcePart->getShape());
//        const MGeoCommand::DirectSelectionInfo sourceInfo =
//            sourceGeometry.analyzeSelection(sourceShape);
//        const gp_Pnt sourceCenter(
//            sourceInfo.arrowCenter.x(),
//            sourceInfo.arrowCenter.y(),
//            sourceInfo.arrowCenter.z());
//        const gp_Vec axisVector(line.Direction());
//        const gp_Vec locationToCenter(line.Location(), sourceCenter);
//        // 用户选择的是有限边，而旋转算法需要无限直线轴。保持边方向，并把轴平移到
//        // 离源面中心最近的位置；几何层随后还会验证该轴确实位于源面平面内。
//        const gp_Pnt axisCenter = line.Location().Translated(
//            axisVector.Multiplied(locationToCenter.Dot(axisVector)));
//        const gp_Ax1 axis(axisCenter, line.Direction());

//        if (m_geometry.hasActiveOperation())
//            emit previewClearRequested();
//        m_geometry.cancel();
//        m_geometry.setPartShape(sourcePart->getShape());

//        QString errorMessage;
//        if (!m_geometry.beginOperation(
//            sourceShape,
//            MGeoCommand::DirectModelingOperation::RotateFace,
//            MGeoCommand::DirectModelingResultMode::Update,
//            &errorMessage))
//        {
//            outputMessage(2, tr("启动旋转面失败：%1").arg(errorMessage));
//            m_geometry.cancel();
//            return false;
//        }
//        if (!m_geometry.setRotationAxis(axis))
//        {
//            outputMessage(2, tr("旋转轴必须是位于所选面内的直线"));
//            m_geometry.cancel();
//            return false;
//        }
//        m_selectedElementDim = 2;
//        m_selectedElementId = sourceFaceId;
//        m_currentPartSet = sourcePart;
//        m_currentOperation = MGeoCommand::DirectModelingOperation::RotateFace;
//        m_referenceShape = axisShape;
//        m_referenceShapeBelongsToCurrentPart = true;
//        m_currentParameter = 0.0;
//        m_arrowCenter = QVector3D(
//            static_cast<float>(axis.Location().X()),
//            static_cast<float>(axis.Location().Y()),
//            static_cast<float>(axis.Location().Z()));
//        m_arrowDirection = QVector3D(
//            static_cast<float>(axis.Direction().X()),
//            static_cast<float>(axis.Direction().Y()),
//            static_cast<float>(axis.Direction().Z()));
//        preparePreviewSession();
//        emit arrowHidden();
//        return true;
//    #endif
//}
}

    bool DirectModelingController::executeCommand(
        const DirectModelingCommand& command,
        QString* errorMessage)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。
    return false;
//【S9 原体（逐行注释保留）】
//        const DirectModelingCommand& command,
//        QString* errorMessage)
//    {
//        if (errorMessage)
//            errorMessage->clear();

//        auto fail = [errorMessage](const QString& message) {
//            if (errorMessage)
//                *errorMessage = message;
//            return false;
//            };

//        if (!std::isfinite(command.value))
//            return fail(tr("直接建模参数必须是有限数值"));
//        // 增量类 API 的 0 表示几何没有变化。作为成功的空操作立即返回，既不解析
//        // 几何 ID，也不进入模式、创建参数或触发部件重建，脚本执行器可继续下一步。
//        // ReplaceFace 的 0 仍表示“替换后不附加偏置”；圆角 API 的 0
//        // 同样按空操作处理，只有正值才表示绝对目标半径。
//        if (std::abs(command.value) <= 1.0e-12
//            && (command.operation == MGeoCommand::DirectModelingOperation::MoveFace
//                || command.operation == MGeoCommand::DirectModelingOperation::OffsetFace
//#if MXGUI_HAS_DIRECTMODELING_V2
//                || command.operation == MGeoCommand::DirectModelingOperation::RotateFace
//#endif // MXGUI_HAS_DIRECTMODELING_V2
//                || command.operation == MGeoCommand::DirectModelingOperation::ResizeFillet
//                || command.operation == MGeoCommand::DirectModelingOperation::EdgeFillet)
//            && command.drivingExpression.trimmed().isEmpty())
//        {
//            return true;
//        }

//        // 复数列表是新 API 的统一表示；旧面板和旧脚本仍写单数成员。控制器在入口
//        // 归一化一次，后续代码不再区分 JSON 标量和数组。
//        if (!command.primaryElementIds.isEmpty()
//            && command.primaryElementId >= 0
//            && (command.primaryElementIds.size() != 1
//                || command.primaryElementIds.front() != command.primaryElementId))
//        {
//            return fail(tr("不能同时传入不一致的主对象单数 ID 和复数 ID 列表"));
//        }
//        if (!command.referenceElementIds.isEmpty()
//            && command.referenceElementId >= 0
//            && (command.referenceElementIds.size() != 1
//                || command.referenceElementIds.front() != command.referenceElementId))
//        {
//            return fail(tr("不能同时传入不一致的参照对象单数 ID 和复数 ID 列表"));
//        }
//        QVector<int> primaryElementIds = command.primaryElementIds;
//        QVector<int> referenceElementIds = command.referenceElementIds;
//        if (primaryElementIds.isEmpty() && command.primaryElementId >= 0)
//            primaryElementIds.push_back(command.primaryElementId);
//        if (referenceElementIds.isEmpty() && command.referenceElementId >= 0)
//            referenceElementIds.push_back(command.referenceElementId);
//        if (primaryElementIds.isEmpty())
//            return fail(tr("直接建模主对象 ID 列表为空"));

//        auto containsInvalidOrDuplicate = [](const QVector<int>& ids) {
//            QSet<int> seen;
//            for (int id : ids)
//            {
//                if (id < 0 || seen.contains(id))
//                    return true;
//                seen.insert(id);
//            }
//            return false;
//            };
//        if (containsInvalidOrDuplicate(primaryElementIds)
//            || containsInvalidOrDuplicate(referenceElementIds))
//        {
//            return fail(tr("几何 ID 必须为非负整数，且同一列表中不能重复"));
//        }
//        if (command.moveSolid
//            && (command.elementDim != 2
//                || (command.operation != MGeoCommand::DirectModelingOperation::MoveFace
//                    && command.operation
//                    != MGeoCommand::DirectModelingOperation::OffsetFace)
//                || !referenceElementIds.isEmpty()))
//        {
//            return fail(tr("移动实体模式只能用于不带参照对象、且仅选择一个面的移动面命令"));
//        }
//        const bool usesOriginalFaceIds =
//            !command.originalFaceIdContext.trimmed().isEmpty();
//        // 保留脚本传入的初始 ID 用于参数身份；几何选择则在下面通过同一次
//        // script context 的 OCC History 链转换为当前 GeoDataModelSet 业务 ID。
//        const QVector<int> scriptOriginalFaceIds = usesOriginalFaceIds
//            ? primaryElementIds : QVector<int>();

//        // originalFaceIdContext 只由 JSON 脚本执行器注入。普通 API 和 Panel 仍按
//        // 当前业务 ID 工作。当前 original 模式仅允许连续移动面；每次命令
//        // 都把最初面 ID 精确转换成最新 ID，不按拓扑序号或几何相似度猜测。
//        if (usesOriginalFaceIds)
//        {
//            if (command.operation != MGeoCommand::DirectModelingOperation::OffsetFace
//                || command.elementDim != 2
//                || !referenceElementIds.isEmpty())
//            {
//                return fail(tr("原始几何 ID 模式当前只支持移动面命令"));
//            }

//            QVector<int> currentFaceIds;
//            if (!resolveScriptOriginalFaceIds(
//                command.originalFaceIdContext,
//                primaryElementIds,
//                currentFaceIds,
//                errorMessage))
//            {
//                return false;
//            }
//            primaryElementIds = std::move(currentFaceIds);
//        }

//        // 命令行调用时通常没有打开直接建模面板，因此由统一命令入口负责激活
//        // Controller；Panel 已激活时该调用是幂等的。
//        if (!m_isActive)
//            enterDirectModelingMode();
//        if (!m_isActive)
//            return fail(tr("当前模型未就绪，无法进入直接建模模式"));

//        // 多对象必须走组合几何路径。它不会逐个调用 onCommit，因此中途失败时既不
//        // 创建参数，也不替换部件；全部成功后只更新一次正式部件。
//        if (primaryElementIds.size() > 1 || referenceElementIds.size() > 1)
//        {
//            return executeBatchCommand(
//                command, primaryElementIds, referenceElementIds, errorMessage);
//        }

//        const int primaryElementId = primaryElementIds.front();
//        const int referenceElementId = referenceElementIds.isEmpty()
//            ? -1 : referenceElementIds.front();

//        bool operationStarted = false;
//        switch (command.operation)
//        {
//        case MGeoCommand::DirectModelingOperation::ReplaceFace:
//            if (referenceElementId < 0)
//                return fail(tr("替换面 API 缺少有效的目标面 ID"));
//            operationStarted = beginReplaceFaceOperation(
//                primaryElementId, referenceElementId);
//            break;

//#if MXGUI_HAS_DIRECTMODELING_V2
//        case MGeoCommand::DirectModelingOperation::RotateFace:
//            operationStarted = referenceElementId >= 0
//                ? beginRotateFaceOperation(
//                    primaryElementId, referenceElementId)
//                : beginRotateFaceOperation(primaryElementId);
//            break;
//#endif // MXGUI_HAS_DIRECTMODELING_V2

//        case MGeoCommand::DirectModelingOperation::OffsetFace:
//            if (command.elementDim != 2)
//                return fail(tr("移动面 API 只能使用几何面 ID"));
//            operationStarted = beginOperation(
//                2,
//                primaryElementId,
//                MGeoCommand::DirectModelingOperation::OffsetFace,
//                command.moveSolid);
//            break;

//        case MGeoCommand::DirectModelingOperation::ResizeFillet:
//            if (command.elementDim != 2)
//                return fail(tr("调整圆角面时 elementType 必须为 face"));
//            operationStarted = beginOperation(
//                2,
//                primaryElementId,
//                MGeoCommand::DirectModelingOperation::ResizeFillet);
//            break;

//        case MGeoCommand::DirectModelingOperation::EdgeFillet:
//            if (command.elementDim != 1)
//                return fail(tr("边倒圆角时 elementType 必须为 edge"));
//            operationStarted = beginOperation(
//                1,
//                primaryElementId,
//                MGeoCommand::DirectModelingOperation::EdgeFillet);
//            break;

//        default:
//            return fail(tr("API 不支持该直接建模操作类型"));
//        }

//        if (!operationStarted)
//            return fail(tr("直接建模操作启动失败，请检查几何 ID 和选择类型"));

//        if (usesOriginalFaceIds && m_currentPartSet)
//        {
//            const int inputFaceId = scriptOriginalFaceIds.front();
//            QString parameterKey = QStringLiteral(
//                "part:%1/dim:%2/%3:%4/operation:%5")
//                .arg(m_currentPartSet->getPartID())
//                .arg(command.elementDim)
//                .arg(QStringLiteral("originalFace"))
//                .arg(inputFaceId)
//                .arg(static_cast<int>(command.operation));
//            // 不勾选时保持历史键完全不变；实体移动只在末尾增加作用域，避免与同一
//            // 面已有的移动参数互相覆盖。
//            if (command.moveSolid)
//                parameterKey += QStringLiteral("/scope:solid");
//            m_commandParameterObjectKeyOverride = parameterKey;
//        }
//        const bool succeeded = onCommit(
//            command.value,
//            command.drivingExpression,
//            errorMessage,
//            command.originalFaceIdContext);
//        m_commandParameterObjectKeyOverride.clear();
//        return succeeded;
//    }
}

    void DirectModelingController::clearScriptOriginalFaceIdContext(
        const QString& contextId)
    {
        const QString normalizedContextId = contextId.trimmed();
        if (normalizedContextId.isEmpty())
            return;

        // 结束通知可能晚于下一次脚本启动，因此只删除 ID 完全匹配的上下文，绝不
        // 用“清空全部”的方式误删另一个仍在运行的脚本映射。
        m_scriptInitialTopologyIdContexts.erase(
            std::remove_if(
                m_scriptInitialTopologyIdContexts.begin(),
                m_scriptInitialTopologyIdContexts.end(),
                [&normalizedContextId](
                    const std::unique_ptr<ScriptInitialTopologyIdContext>& context) {
                        return context && context->contextId == normalizedContextId;
                }),
            m_scriptInitialTopologyIdContexts.end());
    }

    bool DirectModelingController::resolveScriptOriginalGeometryIds(
        const QString& contextId,
        int elementDim,
        const QVector<int>& originalIds,
        QVector<int>& currentIds,
        QString* errorMessage)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。
    return false;
//【S9 原体（逐行注释保留）】
//        const QString& contextId,
//        int elementDim,
//        const QVector<int>& originalIds,
//        QVector<int>& currentIds,
//        QString* errorMessage)
//    {
//        currentIds.clear();
//        auto fail = [errorMessage](const QString& message) {
//            if (errorMessage)
//                *errorMessage = message;
//            return false;
//            };

//        const QString normalizedContextId = contextId.trimmed();
//        if (normalizedContextId.isEmpty())
//            return fail(tr("初始几何 ID 模式缺少有效的脚本上下文"));
//        if (elementDim < 0 || elementDim > 3)
//            return fail(tr("几何维度必须为 0（点）、1（线）、2（面）或 3（体）"));
//        if (originalIds.isEmpty())
//            return true;

//        ScriptInitialTopologyIdContext* context = nullptr;
//        for (const std::unique_ptr<ScriptInitialTopologyIdContext>& candidate
//            : m_scriptInitialTopologyIdContexts)
//        {
//            if (candidate && candidate->contextId == normalizedContextId)
//            {
//                context = candidate.get();
//                break;
//            }
//        }

//        // 在第一条非零直接建模命令执行前没有必要创建上下文。此时几何尚未改变，
//        // 约束/边界步骤中的初始 ID 就是当前 ID；仍要确认每个 ID 在当前模型中
//        // 唯一存在，不能让 original 模式把无效编号静默传给物理场对象。
//        if (!context)
//        {
//            MGeoData::GeoDataBase* geoData = currentGeoData();
//            if (!geoData)
//                return fail(tr("当前模型未就绪，无法解析最初几何 ID"));
//            const auto partSets = geoData->getPartSet();
//            for (int originalId : originalIds)
//            {
//                if (originalId < 0)
//                    return fail(tr("几何 ID 必须为非负整数"));
//                MGeoData::GeoDataModelSet* owner = nullptr;
//                for (MGeoData::GeoDataModelSet* partSet : partSets)
//                {
//                    if (!partSet
//                        || partSet->getShapeByDimTag(elementDim, originalId).IsNull())
//                    {
//                        continue;
//                    }
//                    if (owner && owner != partSet)
//                    {
//                        return fail(tr("维度 %1 的几何 ID %2 在多个当前部件中重复")
//                            .arg(elementDim)
//                            .arg(originalId));
//                    }
//                    owner = partSet;
//                }
//                if (!owner)
//                {
//                    return fail(tr("最初维度 %1 的几何 ID %2 不存在")
//                        .arg(elementDim)
//                        .arg(originalId));
//                }
//                currentIds.push_back(originalId);
//            }
//            return true;
//        }

//        if (!context->valid)
//        {
//            return fail(context->failureReason.isEmpty()
//                ? tr("本次脚本的初始几何 ID 映射已经失效")
//                : context->failureReason);
//        }
//        if (context->viewportModel != currentViewportModel()
//            || context->geoData != currentGeoData())
//        {
//            return fail(tr("脚本执行期间活动模型已经切换，初始几何 ID 无法继续使用"));
//        }

//        const QHash<int, ScriptInitialTopologyIdContext::TrackedTopology>& topologyMap =
//            context->topologies[static_cast<std::size_t>(elementDim)];
//        // 对未被本脚本直接建模修改的部件，最初 ID 与当前 ID 相同，可以原样使用；
//        // 但必须先在当前 GeoDataBase 中证明该 ID 唯一存在且确实属于未跟踪部件。
//        // 不能仅凭“没有出现在 topologyMap 中”就放行，否则一个不存在的 ID，或调用方
//        // 误传的建模后 current ID，都可能在后续约束/边界接口中静默绑定错误对象。
//        auto findUntrackedCurrentOwner = [&](int geometryId,
//            MGeoData::GeoDataModelSet*& owner, QString& ownerError) {
//                owner = nullptr;
//                const auto partSets = context->geoData->getPartSet();
//                for (MGeoData::GeoDataModelSet* partSet : partSets)
//                {
//                    if (!partSet)
//                        continue;
//                    const TopoDS_Shape shape = partSet->getShapeByDimTag(
//                        elementDim, geometryId);
//                    if (shape.IsNull())
//                        continue;
//                    if (owner && owner != partSet)
//                    {
//                        ownerError = tr("维度 %1 的几何 ID %2 在多个当前部件中重复")
//                            .arg(elementDim)
//                            .arg(geometryId);
//                        return false;
//                    }
//                    owner = partSet;
//                }
//                if (!owner)
//                {
//                    ownerError = tr("最初维度 %1 的几何 ID %2 不存在")
//                        .arg(elementDim)
//                        .arg(geometryId);
//                    return false;
//                }
//                if (context->parts.contains(owner->getPartID()))
//                {
//                    ownerError = tr(
//                        "几何 ID %1 属于已被直接建模修改的部件，但不是该部件的最初维度 %2 ID")
//                        .arg(geometryId)
//                        .arg(elementDim);
//                    return false;
//                }
//                return true;
//            };
//        currentIds.reserve(originalIds.size());
//        for (int originalId : originalIds)
//        {
//            if (originalId < 0)
//                return fail(tr("几何 ID 必须为非负整数"));

//            const auto topologyIt = topologyMap.constFind(originalId);
//            if (topologyIt == topologyMap.constEnd())
//            {
//                // original 模式不接受调用方偷换成建模后新分配的 current ID。
//                // 这类 ID 不会是 key，但会出现在某条已跟踪谱系的 currentId 中；
//                // 若原样放行，恰好存在的数字会把边界静默绑到错误对象。
//                for (auto trackedIt = topologyMap.constBegin();
//                    trackedIt != topologyMap.constEnd(); ++trackedIt)
//                {
//                    if (trackedIt->currentId == originalId)
//                    {
//                        return fail(tr(
//                            "几何 ID %1 是直接建模后的当前 ID；original 模式必须传入最初 ID")
//                            .arg(originalId));
//                    }
//                }
//                MGeoData::GeoDataModelSet* untrackedOwner = nullptr;
//                QString ownerError;
//                if (!findUntrackedCurrentOwner(
//                    originalId, untrackedOwner, ownerError))
//                {
//                    return fail(ownerError);
//                }
//                // 第一次修改某个部件时会一次性记录它的全部拓扑；已经确认该 ID
//                // 唯一属于尚未修改的其他部件，所以其最初 ID 仍等于当前 ID。
//                currentIds.push_back(originalId);
//                continue;
//            }

//            const ScriptInitialTopologyIdContext::TrackedTopology& topology =
//                topologyIt.value();
//            const auto partIt = context->parts.constFind(topology.partId);
//            if (partIt == context->parts.constEnd() || !partIt->currentSet)
//                return fail(tr("初始几何 ID %1 所属部件已经失效").arg(originalId));

//            const TopoDS_Shape currentShape = partIt->currentSet->getShapeByDimTag(
//                elementDim, topology.currentId);
//            if (currentShape.IsNull()
//                || topology.currentShape.IsNull()
//                || !currentShape.IsSame(topology.currentShape))
//            {
//                return fail(tr(
//                    "初始几何 ID %1 的当前 ID %2 与 History 链不一致，已拒绝猜测")
//                    .arg(originalId)
//                    .arg(topology.currentId));
//            }
//            currentIds.push_back(topology.currentId);
//        }
//        return true;
//    }
}

    bool DirectModelingController::captureScriptInitialTopologyPart(
        ScriptInitialTopologyIdContext& context,
        MGeoData::GeoDataModelSet* partSet,
        QString* errorMessage)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。
    return false;
//【S9 原体（逐行注释保留）】
//        ScriptInitialTopologyIdContext& context,
//        MGeoData::GeoDataModelSet* partSet,
//        QString* errorMessage)
//    {
//        auto fail = [&context, errorMessage](const QString& message) {
//            context.valid = false;
//            context.failureReason = message;
//            if (errorMessage)
//                *errorMessage = message;
//            return false;
//            };

//        if (!context.valid)
//            return fail(context.failureReason);
//        if (!partSet || partSet->getShape().IsNull())
//            return fail(tr("无法为初始 ID 模式快照空部件"));
//        if (context.viewportModel != currentViewportModel()
//            || context.geoData != currentGeoData())
//        {
//            return fail(tr("脚本执行期间活动模型已经切换，无法建立初始拓扑快照"));
//        }

//        const int partId = partSet->getPartID();
//        if (partId < 0)
//            return fail(tr("部件 ID 无效，无法建立初始拓扑快照"));
//        const auto existingPartIt = context.parts.constFind(partId);
//        if (existingPartIt != context.parts.constEnd())
//        {
//            if (existingPartIt->currentSet == partSet)
//                return true;
//            return fail(tr("部件 %1 已在脚本外被替换，初始 ID 上下文失效")
//                .arg(partId));
//        }

//        using TrackedTopology =
//            ScriptInitialTopologyIdContext::TrackedTopology;
//        std::array<QVector<TrackedTopology>, 4> pendingTopologies;
//        std::array<int, 4> topologyCounts{ { 0, 0, 0, 0 } };
//        const std::array<TopAbs_ShapeEnum, 4> shapeTypes{
//            TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE, TopAbs_SOLID };

//        auto collectMap = [&](int dimension,
//            const TopTools_DataMapOfIntegerShape& idToShape) {
//                TopTools_IndexedMapOfShape actualShapes;
//                TopExp::MapShapes(
//                    partSet->getShape(), shapeTypes[static_cast<std::size_t>(dimension)],
//                    actualShapes);
//                if (actualShapes.Extent() != idToShape.Extent())
//                {
//                    return fail(tr(
//                        "部件 %1 的维度 %2 拓扑数量与 ID 表不一致（Shape=%3，ID=%4）")
//                        .arg(partId)
//                        .arg(dimension)
//                        .arg(actualShapes.Extent())
//                        .arg(idToShape.Extent()));
//                }

//                for (TopTools_DataMapIteratorOfDataMapOfIntegerShape iterator(idToShape);
//                    iterator.More(); iterator.Next())
//                {
//                    const int id = iterator.Key();
//                    const TopoDS_Shape& shape = iterator.Value();
//                    if (id < 0 || shape.IsNull()
//                        || shape.ShapeType()
//                        != shapeTypes[static_cast<std::size_t>(dimension)]
//                        || actualShapes.FindIndex(shape) <= 0)
//                    {
//                        return fail(tr("部件 %1 的维度 %2 包含无效 ID 或拓扑")
//                            .arg(partId)
//                            .arg(dimension));
//                    }
//                    if (context.topologies[static_cast<std::size_t>(dimension)]
//                        .contains(id))
//                    {
//                        return fail(tr("初始维度 %1 的几何 ID %2 在多个部件中重复")
//                            .arg(dimension)
//                            .arg(id));
//                    }

//                    TrackedTopology topology;
//                    topology.initialId = id;
//                    topology.currentId = id;
//                    topology.partId = partId;
//                    topology.initialShape = shape;
//                    topology.currentShape = shape;
//                    pendingTopologies[static_cast<std::size_t>(dimension)]
//                        .push_back(topology);
//                }
//                topologyCounts[static_cast<std::size_t>(dimension)] =
//                    idToShape.Extent();
//                return true;
//            };

//        const TopTools_DataMapOfIntegerShape vertexMap =
//            partSet->getIntegerVretexMap();
//        const TopTools_DataMapOfIntegerShape edgeMap =
//            partSet->getIntegerEdgeMap();
//        const TopTools_DataMapOfIntegerShape faceMap =
//            partSet->getIntegerFaceMap();
//        const TopTools_DataMapOfIntegerShape solidMap =
//            partSet->getIntegerSolidMap();
//        if (!collectMap(0, vertexMap)
//            || !collectMap(1, edgeMap)
//            || !collectMap(2, faceMap)
//            || !collectMap(3, solidMap))
//        {
//            return false;
//        }

//        // 只有四类拓扑全部验证通过才写入 context，避免留下半份初始快照。
//        for (int dimension = 0; dimension < 4; ++dimension)
//        {
//            QHash<int, TrackedTopology>& destination =
//                context.topologies[static_cast<std::size_t>(dimension)];
//            for (const TrackedTopology& topology
//                : pendingTopologies[static_cast<std::size_t>(dimension)])
//            {
//                destination.insert(topology.initialId, topology);
//            }
//        }

//        ScriptInitialTopologyIdContext::TrackedPart trackedPart;
//        trackedPart.partId = partId;
//        trackedPart.currentSet = partSet;
//        trackedPart.initialPartShape = partSet->getShape();
//        trackedPart.topologyCounts = topologyCounts;
//        context.parts.insert(partId, trackedPart);
//        return true;
//    }
}

    bool DirectModelingController::resolveScriptOriginalFaceIds(
        const QString& contextId,
        const QVector<int>& originalFaceIds,
        QVector<int>& currentFaceIds,
        QString* errorMessage)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模功能后续迁移 RebuildGeometry（引擎编辑族）。
    return false;
    //【S9 原体（逐行注释保留）】
//        const QString& contextId,
//        const QVector<int>& originalFaceIds,
//        QVector<int>& currentFaceIds,
//        QString* errorMessage)
//    {
//        currentFaceIds.clear();
//        auto fail = [errorMessage](const QString& message) {
//            if (errorMessage)
//                *errorMessage = message;
//            return false;
//            };

//        const QString normalizedContextId = contextId.trimmed();
//        if (normalizedContextId.isEmpty())
//            return fail(tr("原始几何 ID 模式缺少有效的脚本上下文"));
//        if (originalFaceIds.isEmpty())
//            return fail(tr("原始面 ID 列表为空"));

//        MechPreData::mViewportModel* viewportModel = currentViewportModel();
//        MGeoData::GeoDataBase* geoData = currentGeoData();
//        if (!viewportModel || !geoData)
//            return fail(tr("当前模型未就绪，无法解析原始面 ID"));

//        ScriptInitialTopologyIdContext* context = nullptr;
//        for (const std::unique_ptr<ScriptInitialTopologyIdContext>& candidate
//            : m_scriptInitialTopologyIdContexts)
//        {
//            if (candidate && candidate->contextId == normalizedContextId)
//            {
//                context = candidate.get();
//                break;
//            }
//        }
//        if (!context)
//        {
//            auto newContext = std::make_unique<ScriptInitialTopologyIdContext>();
//            newContext->contextId = normalizedContextId;
//            newContext->viewportModel = viewportModel;
//            newContext->geoData = geoData;
//            context = newContext.get();
//            m_scriptInitialTopologyIdContexts.push_back(std::move(newContext));
//        }
//        if (!context->valid)
//            return fail(context->failureReason);
//        if (context->viewportModel != viewportModel || context->geoData != geoData)
//            return fail(tr("脚本执行期间活动模型已经切换，原始面 ID 无法继续使用"));

//        // 第一次选择某个部件时必须在修改前一次性记录该部件的全部点、线、面、体。
//        // 不能等某个面以后第一次被使用时才记录，否则它可能早已被前序移动间接改变。
//        QSet<int> validatedIds;
//        for (int originalFaceId : originalFaceIds)
//        {
//            if (validatedIds.contains(originalFaceId))
//                return fail(tr("同一个面 ID %1 被重复使用")
//                    .arg(originalFaceId));
//            if (!context->topologies[2].contains(originalFaceId))
//            {
//                MGeoData::GeoDataModelSet* partSet =
//                    geoData->getPartSetByGeoFaceID(originalFaceId);
//                if (!partSet || partSet->getShape().IsNull())
//                {
//                    return fail(tr(
//                        "初始面 ID %1 不存在；不能把建模后新分配的 ID 当作初始 ID")
//                        .arg(originalFaceId));
//                }
//                if (!captureScriptInitialTopologyPart(
//                    *context, partSet, errorMessage))
//                {
//                    return false;
//                }
//                if (!context->topologies[2].contains(originalFaceId))
//                    return fail(tr("几何 ID %1 不是该部件的有效初始面 ID")
//                        .arg(originalFaceId));
//            }

//            validatedIds.insert(originalFaceId);
//        }

//        return resolveScriptOriginalGeometryIds(
//            normalizedContextId, 2, originalFaceIds, currentFaceIds, errorMessage);
//    }
}

    bool DirectModelingController::advanceScriptInitialTopologyIds(
        const QString& contextId,
        MGeoData::GeoDataModelSet* oldSet,
        const TopoDS_Shape& newShape,
        MGeoData::GeoDataModelSet* newSet,
        const Handle(BRepTools_History)& trackingHistory,
        bool commitMapping,
        QString* errorMessage,
        Handle(BRepTools_History)* stableDataHistoryOutput)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。
    return false;
//【S9 原体（逐行注释保留）】
//        const QString& contextId,
//        MGeoData::GeoDataModelSet* oldSet,
//        const TopoDS_Shape& newShape,
//        MGeoData::GeoDataModelSet* newSet,
//        const Handle(BRepTools_History)& trackingHistory,
//        bool commitMapping,
//        QString* errorMessage,
//        Handle(BRepTools_History)* stableDataHistoryOutput)
//    {
//        if (stableDataHistoryOutput)
//            stableDataHistoryOutput->Nullify();

//        const QString normalizedContextId = contextId.trimmed();
//        if (normalizedContextId.isEmpty())
//            return true;

//        ScriptInitialTopologyIdContext* context = nullptr;
//        for (const std::unique_ptr<ScriptInitialTopologyIdContext>& candidate
//            : m_scriptInitialTopologyIdContexts)
//        {
//            if (candidate && candidate->contextId == normalizedContextId)
//            {
//                context = candidate.get();
//                break;
//            }
//        }
//        if (!context)
//        {
//            if (errorMessage)
//                *errorMessage = tr("找不到脚本初始几何 ID 上下文");
//            return false;
//        }

//        auto rejectCandidate = [errorMessage](const QString& message) {
//            if (errorMessage)
//                *errorMessage = message;
//            return false;
//            };
//        auto invalidateContext = [context, errorMessage](const QString& message) {
//            context->valid = false;
//            context->failureReason = message;
//            if (errorMessage)
//                *errorMessage = message;
//            return false;
//            };
//        if (!context->valid)
//            return rejectCandidate(context->failureReason);
//        if (!oldSet || oldSet->getShape().IsNull() || newShape.IsNull()
//            || (commitMapping && !newSet))
//        {
//            return rejectCandidate(tr("推进初始几何 ID 时部件或 Shape 为空"));
//        }
//        if (context->viewportModel != currentViewportModel()
//            || context->geoData != currentGeoData())
//        {
//            return invalidateContext(tr(
//                "脚本执行期间活动模型已经切换，初始几何 ID 映射失效"));
//        }

//        const int partId = oldSet->getPartID();
//        auto trackedPartIt = context->parts.find(partId);
//        if (trackedPartIt == context->parts.end())
//        {
//            // rebuildDrivenOperations 可能同时回放其他部件。它们没有在本脚本中通过
//            // 初始 ID 直接建模，不能把当前状态误当成脚本初始状态。
//            return true;
//        }
//        if (trackedPartIt->currentSet != oldSet)
//        {
//            return invalidateContext(tr("部件 %1 的当前对象与脚本 History 链不一致")
//                .arg(partId));
//        }
//        if (commitMapping && newSet->getPartID() != partId)
//            return rejectCandidate(tr("直接建模前后的部件 ID 不一致"));

//        const std::array<TopAbs_ShapeEnum, 4> shapeTypes{
//            TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE, TopAbs_SOLID };
//        std::array<TopTools_IndexedMapOfShape, 4> oldShapeMaps;
//        std::array<TopTools_IndexedMapOfShape, 4> newShapeMaps;
//        for (int dimension = 0; dimension < 4; ++dimension)
//        {
//            TopExp::MapShapes(
//                oldSet->getShape(), shapeTypes[static_cast<std::size_t>(dimension)],
//                oldShapeMaps[static_cast<std::size_t>(dimension)]);
//            TopExp::MapShapes(
//                newShape, shapeTypes[static_cast<std::size_t>(dimension)],
//                newShapeMaps[static_cast<std::size_t>(dimension)]);

//            const int expectedCount = trackedPartIt->topologyCounts[
//                static_cast<std::size_t>(dimension)];
//            if (oldShapeMaps[static_cast<std::size_t>(dimension)].Extent()
//                != expectedCount)
//            {
//                return invalidateContext(tr(
//                    "部件 %1 的维度 %2 当前拓扑数量已偏离初始快照（初始=%3，当前=%4）")
//                    .arg(partId)
//                    .arg(dimension)
//                    .arg(expectedCount)
//                    .arg(oldShapeMaps[static_cast<std::size_t>(dimension)].Extent()));
//            }
//            if (newShapeMaps[static_cast<std::size_t>(dimension)].Extent()
//                != expectedCount)
//            {
//                return rejectCandidate(tr(
//                    "部件 %1 的维度 %2 拓扑数量发生变化（初始=%3，修改后=%4）")
//                    .arg(partId)
//                    .arg(dimension)
//                    .arg(expectedCount)
//                    .arg(newShapeMaps[static_cast<std::size_t>(dimension)].Extent()));
//            }
//        }

//        struct PendingTopologyUpdate
//        {
//            int dimension = -1;
//            int initialId = -1;
//            int newId = -1;
//            TopoDS_Shape oldShape;
//            TopoDS_Shape newShape;
//        };
//        QVector<PendingTopologyUpdate> pendingUpdates;
//        std::array<TopTools_IndexedMapOfShape, 4> usedNewShapes;

//        for (int dimension = 0; dimension < 4; ++dimension)
//        {
//            const std::size_t dimensionIndex =
//                static_cast<std::size_t>(dimension);
//            const TopAbs_ShapeEnum expectedType = shapeTypes[dimensionIndex];
//            const QHash<int, ScriptInitialTopologyIdContext::TrackedTopology>&
//                trackedTopologies = context->topologies[dimensionIndex];
//            int trackedCount = 0;

//            for (auto topologyIt = trackedTopologies.constBegin();
//                topologyIt != trackedTopologies.constEnd(); ++topologyIt)
//            {
//                const ScriptInitialTopologyIdContext::TrackedTopology& topology =
//                    topologyIt.value();
//                if (topology.partId != partId)
//                    continue;
//                ++trackedCount;

//                if (topology.currentShape.IsNull()
//                    || oldShapeMaps[dimensionIndex].FindIndex(
//                        topology.currentShape) <= 0)
//                {
//                    return invalidateContext(tr(
//                        "初始维度 %1 ID %2 的当前拓扑不属于修改前部件")
//                        .arg(dimension)
//                        .arg(topology.initialId));
//                }

//                TopTools_IndexedMapOfShape candidates;
//                auto addCandidate = [&](const TopoDS_Shape& candidateShape) {
//                    if (candidateShape.IsNull()
//                        || candidateShape.ShapeType() != expectedType)
//                    {
//                        return;
//                    }
//                    const int index = newShapeMaps[dimensionIndex].FindIndex(
//                        candidateShape);
//                    if (index > 0)
//                        candidates.Add(newShapeMaps[dimensionIndex].FindKey(index));
//                    };
//                auto collectCandidates = [&](const TopoDS_Shape& sourceShape) {
//                    if (sourceShape.IsNull())
//                        return;
//                    // 未改变的拓扑通常保持同一 TShape；但 History 明确标记 Removed
//                    // 时不能把另一 owner 中碰巧共享的 TShape 当成该谱系的恒等结果。
//                    if (trackingHistory.IsNull()
//                        || !trackingHistory->IsRemoved(sourceShape))
//                    {
//                        addCandidate(sourceShape);
//                    }
//                    if (trackingHistory.IsNull())
//                        return;
//                    const TopTools_ListOfShape& modified =
//                        trackingHistory->Modified(sourceShape);
//                    for (TopTools_ListIteratorOfListOfShape iterator(modified);
//                        iterator.More(); iterator.Next())
//                    {
//                        addCandidate(iterator.Value());
//                    }
//                    const TopTools_ListOfShape& generated =
//                        trackingHistory->Generated(sourceShape);
//                    for (TopTools_ListIteratorOfListOfShape iterator(generated);
//                        iterator.More(); iterator.Next())
//                    {
//                        addCandidate(iterator.Value());
//                    }
//                    };

//                // 普通确认的 History 是 current -> next；参数整链回放的复合 History
//                // 则是 initial -> final。先试当前拓扑，完全无候选时再试初始拓扑。
//                collectCandidates(topology.currentShape);
//                if (candidates.IsEmpty()
//                    && !topology.initialShape.IsNull()
//                    && !topology.initialShape.IsSame(topology.currentShape))
//                {
//                    collectCandidates(topology.initialShape);
//                }

//                if (candidates.Extent() != 1)
//                {
//                    return rejectCandidate(tr(
//                        "初始维度 %1 ID %2 通过 OCC History 得到 %3 个当前候选，无法唯一映射")
//                        .arg(dimension)
//                        .arg(topology.initialId)
//                        .arg(candidates.Extent()));
//                }

//                const TopoDS_Shape canonicalShape = candidates.FindKey(1);
//                const int usedBefore = usedNewShapes[dimensionIndex].Extent();
//                usedNewShapes[dimensionIndex].Add(canonicalShape);
//                if (usedNewShapes[dimensionIndex].Extent() == usedBefore)
//                {
//                    return rejectCandidate(tr(
//                        "两个初始维度 %1 ID 被 History 映射到了同一当前拓扑")
//                        .arg(dimension));
//                }

//                PendingTopologyUpdate update;
//                update.dimension = dimension;
//                update.initialId = topology.initialId;
//                update.oldShape = topology.currentShape;
//                update.newShape = canonicalShape;
//                pendingUpdates.push_back(update);
//            }

//            const int expectedCount = trackedPartIt->topologyCounts[dimensionIndex];
//            if (trackedCount != expectedCount
//                || usedNewShapes[dimensionIndex].Extent() != expectedCount)
//            {
//                return invalidateContext(tr(
//                    "部件 %1 的维度 %2 没有形成完整的一一 History 映射")
//                    .arg(partId)
//                    .arg(dimension));
//            }
//        }

//        if (stableDataHistoryOutput)
//        {
//            // trackingHistory 在普通提交时是 current -> next，在参数整链回放时则可能是
//            // initial -> final。上面的候选解析已经把二者统一成了经完整双射验证的
//            // current oldShape -> final newShape；据此重新创建独立 History，不能把输入域
//            // 不同的 replayHistory 直接交给 GeoDataModelSet。
//            Handle(BRepTools_History) stableHistory = new BRepTools_History();
//            for (const PendingTopologyUpdate& update : pendingUpdates)
//            {
//                // IsSame 的未变拓扑采用 OCC 的隐式恒等关系。只为真正替换的拓扑写
//                // Modified，不写 Removed/Generated，从结构上杜绝 Debug History 冲突。
//                if (!update.oldShape.IsSame(update.newShape))
//                    stableHistory->AddModified(update.oldShape, update.newShape);
//            }
//            *stableDataHistoryOutput = stableHistory;
//        }

//        // updatePart 在构造 GeoDataModelSet 之前先以 commitMapping=false 调用本函数。
//        // 到这里已证明 Shape 数量和全部 History 谱系严格一一对应，可以避免在历史
//        // 本身无效时触发 setShape/updateMeshEntity 等数据层副作用。
//        if (!commitMapping)
//            return true;

//        // newSet 已按数据层原有流程建立业务 ID。这里只读取 ID 并做双向校验，
//        // 不修改 GeoDataModelSet 的任何全局编号或绑定，因此其他功能完全不受影响。
//        std::array<QSet<int>, 4> usedNewIds;
//        for (PendingTopologyUpdate& update : pendingUpdates)
//        {
//            TopoDS_Shape canonicalShape = update.newShape;
//            update.newId = newSet->getTagByShape(&canonicalShape);
//            if (update.newId < 0
//                || usedNewIds[static_cast<std::size_t>(update.dimension)]
//                .contains(update.newId))
//            {
//                return rejectCandidate(tr("数据层没有为 History 结果建立唯一的当前几何 ID"));
//            }
//            const TopoDS_Shape idShape = newSet->getShapeByDimTag(
//                update.dimension, update.newId);
//            if (idShape.IsNull() || !idShape.IsSame(update.newShape))
//                return rejectCandidate(tr("当前几何 ID 的双向绑定与 History 结果不一致"));
//            usedNewIds[static_cast<std::size_t>(update.dimension)]
//                .insert(update.newId);
//        }

//        // 所有维度、所有 Shape 和所有业务 ID 均验证完成后一次性提交映射更新。
//        for (const PendingTopologyUpdate& update : pendingUpdates)
//        {
//            auto topologyIt = context->topologies[
//                static_cast<std::size_t>(update.dimension)].find(update.initialId);
//                if (topologyIt == context->topologies[
//                    static_cast<std::size_t>(update.dimension)].end())
//                {
//                    return invalidateContext(tr("提交初始几何 ID 映射时内部状态丢失"));
//                }
//                topologyIt->currentId = update.newId;
//                topologyIt->currentShape = update.newShape;
//        }
//        trackedPartIt->currentSet = newSet;
//        return true;
//    }
}

    bool DirectModelingController::executeBatchCommand(
        const DirectModelingCommand& command,
        const QVector<int>& primaryElementIds,
        const QVector<int>& referenceElementIds,
        QString* errorMessage)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模功能后续迁移 RebuildGeometry（引擎编辑族 executeEdit*）。
    return false;
    //【S9 原体（逐行注释保留）】
//        const DirectModelingCommand& command,
//        const QVector<int>& primaryElementIds,
//        const QVector<int>& referenceElementIds,
//        QString* errorMessage)
//    {
//        if (errorMessage)
//            errorMessage->clear();
//        auto fail = [this, errorMessage](const QString& message) {
//            if (errorMessage)
//                *errorMessage = message;
//            outputMessage(2, message);
//            emit commitFinished(false);
//            return false;
//            };

//        if (!m_parameterCalculator)
//            return fail(tr("直接建模参数计算器未初始化"));
//        if (primaryElementIds.size() < 2)
//            return fail(tr("批量直接建模至少需要两个主对象 ID"));
//        // 一个面已经能够唯一确定所属实体。批量实体移动既可能把同一 Solid 重复
//        // 平移，也可能跨多个 Solid，当前事务语义无法消除这种歧义，因此明确拒绝。
//        if (command.moveSolid)
//            return fail(tr("移动实体模式只允许选择一个几何面，不支持批量执行"));

//        // 当前批处理按一个业务部件内的一组拓扑对象定义。若跨部件执行，每个部件都
//        // 会有独立生命周期和几何 ID，无法再保证“一次成功或全部不改”的事务语义。
//        std::vector<TopoDS_Shape> selectedShapes;
//        selectedShapes.reserve(static_cast<std::size_t>(primaryElementIds.size()));
//        MGeoData::GeoDataModelSet* sourcePartSet = nullptr;
//        for (int id : primaryElementIds)
//        {
//            TopoDS_Shape shape;
//            MGeoData::GeoDataModelSet* partSet = nullptr;
//            if (!resolveSelection(command.elementDim, id, shape, partSet))
//                return fail(tr("无法解析批量选择中的几何 ID %1").arg(id));
//            if (!sourcePartSet)
//                sourcePartSet = partSet;
//            else if (partSet != sourcePartSet)
//                return fail(tr("一次直接建模的所有主对象必须属于同一部件"));
//            selectedShapes.push_back(shape);
//        }
//        if (!sourcePartSet || sourcePartSet->getShape().IsNull())
//            return fail(tr("批量直接建模的源部件无效"));

//        std::vector<TopoDS_Shape> referenceShapes;
//        std::vector<bool> referenceBelongsToSourcePart;
//        if (command.operation == MGeoCommand::DirectModelingOperation::ReplaceFace)
//        {
//            if (command.elementDim != 2)
//                return fail(tr("替换面只能传入几何面 ID"));
//            if (referenceElementIds.isEmpty()
//                || (referenceElementIds.size() != 1
//                    && referenceElementIds.size() != primaryElementIds.size()))
//            {
//                return fail(tr(
//                    "替换面的目标面 ID 数量必须为 1，或与源面 ID 数量一致"));
//            }
//            for (int id : referenceElementIds)
//            {
//                TopoDS_Shape shape;
//                MGeoData::GeoDataModelSet* partSet = nullptr;
//                if (!resolveSelection(2, id, shape, partSet))
//                    return fail(tr("无法解析替换目标面 ID %1").arg(id));
//                referenceShapes.push_back(shape);
//                referenceBelongsToSourcePart.push_back(partSet == sourcePartSet);
//            }
//            for (std::size_t index = 0; index < selectedShapes.size(); ++index)
//            {
//                const std::size_t referenceIndex = referenceShapes.size() == 1
//                    ? 0 : index;
//                if (selectedShapes[index].IsSame(referenceShapes[referenceIndex]))
//                    return fail(tr("替换面的源面和对应目标面不能相同"));
//            }
//        }
//#if MXGUI_HAS_DIRECTMODELING_V2
//        else if (command.operation == MGeoCommand::DirectModelingOperation::RotateFace)
//#endif
//        if (false) //【GUI 门控】RotateFace 批量校验暂闭（V2 依赖未推送）
//        {
//            if (command.elementDim != 2)
//                return fail(tr("旋转面只能传入几何面 ID"));
//            if (referenceElementIds.size() != 1)
//                return fail(tr("多个面同步旋转时必须传入一条公共 axisEdgeId"));

//            TopoDS_Shape axisShape;
//            MGeoData::GeoDataModelSet* axisPartSet = nullptr;
//            if (!resolveSelection(
//                1, referenceElementIds.front(), axisShape, axisPartSet))
//            {
//                return fail(tr("无法解析公共旋转轴边 ID"));
//            }
//            if (axisPartSet != sourcePartSet)
//                return fail(tr("旋转面和公共旋转轴必须属于同一部件"));
//            BRepAdaptor_Curve curve(TopoDS::Edge(axisShape));
//            if (curve.GetType() != GeomAbs_Line)
//                return fail(tr("公共旋转轴必须选择直线边"));
//            referenceShapes.push_back(axisShape);
//            referenceBelongsToSourcePart.push_back(true);
//        }
//        else if (command.operation == MGeoCommand::DirectModelingOperation::OffsetFace)
//        {
//            if (command.elementDim != 2 || !referenceElementIds.isEmpty())
//                return fail(tr("批量移动面只能传入面 ID，不需要参照对象"));
//        }
//        else if (command.operation == MGeoCommand::DirectModelingOperation::ResizeFillet)
//        {
//            if (command.elementDim != 2 || !referenceElementIds.isEmpty())
//                return fail(tr("批量调整圆角面只能传入面 ID"));
//        }
//        else if (command.operation == MGeoCommand::DirectModelingOperation::EdgeFillet)
//        {
//            if (command.elementDim != 1 || !referenceElementIds.isEmpty())
//                return fail(tr("批量边倒圆只能传入边 ID"));
//        }
//        else
//        {
//            return fail(tr("当前 API 操作不支持多个几何 ID"));
//        }

//        // updatePart 会重新生成几何 ID。普通调用使用同类型拓扑表位置建立参数键；
//        // 原始 ID 脚本模式在下面改用脚本 ID。除替换面配对外，排序后使 [A,B] 与
//        // [B,A] 表示同一组对象。
//        TopTools_IndexedMapOfShape topologyMap;
//        TopExp::MapShapes(
//            sourcePartSet->getShape(), selectedShapes.front().ShapeType(), topologyMap);
//        QVector<int> keyTopologyIndices;
//        keyTopologyIndices.reserve(primaryElementIds.size());
//        for (const TopoDS_Shape& shape : selectedShapes)
//        {
//            const int index = topologyMap.FindIndex(shape);
//            if (index <= 0)
//                return fail(tr("无法建立批量参数的稳定拓扑标识"));
//            keyTopologyIndices.push_back(index);
//        }
//        if (command.operation != MGeoCommand::DirectModelingOperation::ReplaceFace)
//            std::sort(keyTopologyIndices.begin(), keyTopologyIndices.end());
//        QStringList topologyTexts;
//        for (int index : keyTopologyIndices)
//            topologyTexts.push_back(QString::number(index));
//        QString automaticParameterId;
//        if (!command.originalFaceIdContext.trimmed().isEmpty())
//        {
//            // 原始 ID 模式不能用当前 TopExp 序号作为参数身份。序号重排后，另一个
//            // 原始面可能占用旧序号，从而被误判为更新已有参数并回放错误的选择。
//            QVector<int> inputIds = command.primaryElementIds;
//            if (inputIds.isEmpty() && command.primaryElementId >= 0)
//                inputIds.push_back(command.primaryElementId);
//            QStringList identityTokens;
//            for (int id : inputIds)
//            {
//                identityTokens.push_back(QStringLiteral("original:%1").arg(id));
//            }
//            std::sort(identityTokens.begin(), identityTokens.end());
//            automaticParameterId =
//                QStringLiteral("part:%1/dim:%2/scriptFaces:%3/operation:%4")
//                .arg(sourcePartSet->getPartID())
//                .arg(command.elementDim)
//                .arg(identityTokens.join(QLatin1Char(',')))
//                .arg(static_cast<int>(command.operation));
//        }
//        else
//        {
//            automaticParameterId =
//                QStringLiteral("part:%1/dim:%2/topologies:%3/operation:%4")
//                .arg(sourcePartSet->getPartID())
//                .arg(command.elementDim)
//                .arg(topologyTexts.join(QLatin1Char(',')))
//                .arg(static_cast<int>(command.operation));
//        }

//        auto operationDisplayName = [this, &command]() {
//            switch (command.operation)
//            {
//            case MGeoCommand::DirectModelingOperation::ReplaceFace:
//                return tr("替换面");
//#if MXGUI_HAS_DIRECTMODELING_V2
//            case MGeoCommand::DirectModelingOperation::RotateFace:
//                return tr("旋转面");
//#endif // MXGUI_HAS_DIRECTMODELING_V2
//            case MGeoCommand::DirectModelingOperation::OffsetFace:
//                return tr("移动面");
//            case MGeoCommand::DirectModelingOperation::ResizeFillet:
//            case MGeoCommand::DirectModelingOperation::EdgeFillet:
//                return tr("调整圆角大小");
//            default:
//                return tr("直接建模");
//            }
//            };

//        bool parameterAlreadyExists = false;
//        for (const GeoFeatureParameter& parameter : m_parameterCalculator->parameters())
//        {
//            if (parameter.id == automaticParameterId)
//            {
//                parameterAlreadyExists = true;
//                break;
//            }
//        }
//        auto pendingCalculator =
//            std::make_unique<mGeoFeatureParameterCalculator>(*m_parameterCalculator);
//        QString automaticParameterName;
//        QString parameterError;
//        if (!pendingCalculator->upsertOperationParameter(
//            automaticParameterId,
//            operationDisplayName(),
//            command.drivingExpression,
//            command.value,
//            &automaticParameterName,
//            &parameterError))
//        {
//            return fail(tr("批量操作自动参数创建或更新失败：%1")
//                .arg(parameterError));
//        }

//        double effectiveValue = command.value;
//        if (!command.drivingExpression.trimmed().isEmpty())
//        {
//            bool found = false;
//            for (const GeoFeatureParameter& parameter : pendingCalculator->parameters())
//            {
//                if (parameter.id == automaticParameterId)
//                {
//                    found = true;
//                    effectiveValue = parameter.value;
//                    break;
//                }
//            }
//            if (!found || !std::isfinite(effectiveValue))
//                return fail(tr("无法取得批量直接建模表达式的有效计算结果"));
//        }
//#if MXGUI_HAS_DIRECTMODELING_V2
//        if (command.operation == MGeoCommand::DirectModelingOperation::RotateFace
//            && std::abs(effectiveValue) > 90.0)
//        {
//            return fail(tr("旋转面角度必须在 -90° 到 90° 之间"));
//        }
//#endif // MXGUI_HAS_DIRECTMODELING_V2
//        if (std::abs(effectiveValue) <= 1.0e-12
//            && command.operation != MGeoCommand::DirectModelingOperation::ReplaceFace)
//        {
//            // 表达式的当前计算结果也可能为 0。API 约定 0 是成功空操作：不创建
//            // 参数、不重建部件，调用方可以继续执行后续脚本步骤。
//            return true;
//        }

//        DrivenOperation* existingOperation = nullptr;
//        if (parameterAlreadyExists)
//        {
//            const int partId = sourcePartSet->getPartID();
//            MechPreData::mViewportModel* viewportModel = currentViewportModel();
//            MGeoData::GeoDataBase* geoData = currentGeoData();
//            for (const std::unique_ptr<DrivenOperation>& operation : m_drivenOperations)
//            {
//                if (!operation || operation->parameterId != automaticParameterId)
//                    continue;
//                // 相同拓扑键可能出现在其他模型中，不能把“当前模型未找到节点”误当作
//                // 孤立参数而覆盖。弱 owner 失效时也不使用可能已复用的数据库地址。
//                if (operation->partId != partId
//                    || operation->viewportModel != viewportModel
//                    || (operation->geoData && operation->geoData != geoData)
//                    || (operation->tracksGeoDataOwner && operation->geoDataOwner.isNull()))
//                {
//                    return fail(tr("批量参数的历史属于其他模型或已失效的数据源，不能覆盖"));
//                }
//                if (operation->historyRetired)
//                    continue;
//                if (existingOperation)
//                    return fail(tr("批量参数对应多个历史节点，不能唯一确定要更新的操作"));
//                existingOperation = operation.get();
//            }
//            if (!existingOperation && !command.originalFaceIdContext.trimmed().isEmpty())
//            {
//                return fail(tr(
//                    "参数历史缺失，无法保证 original-ID 脚本的初始拓扑 History；"
//                    "本次操作未提交，请先退出原始 ID 上下文并在当前几何上恢复参数"));
//            }
//            // 节点确实缺失时，下面按当前几何重新建立历史。候选计算器已通过 upsert
//            // 保留旧 parameterId 和用户名称/公式引用；几何写入成功前不覆盖正式参数。
//            // 原历史基准无法恢复，因此本次当前 Shape 是新节点的基准，不猜测旧操作。
//        }

//        if (existingOperation)
//        {
//            // 拓扑序号只用于查找候选参数，不能单独证明它仍是同一个物理对象。把历史
//            // locator 在当前 Shape 上重新解析，并与本次 API 选择逐个 IsSame 校验，防止
//            // 布尔重排后相同序号碰撞到另一个面。非替换操作的选择组按集合比较。
//            std::vector<TopoDS_Shape> historicalSelections;
//            QString locatorError;
//            for (const TopologyLocator& locator : existingOperation->selectedLocators)
//            {
//                const TopoDS_Shape resolved = ResolveTopologyLocator(
//                    sourcePartSet->getShape(), locator, &locatorError);
//                if (resolved.IsNull())
//                    return fail(tr("已有批量参数的选择对象无法重新定位：%1")
//                        .arg(locatorError));
//                historicalSelections.push_back(resolved);
//            }
//            auto sameShapeList = [](const std::vector<TopoDS_Shape>& left,
//                const std::vector<TopoDS_Shape>& right,
//                bool ordered) {
//                    if (left.size() != right.size())
//                        return false;
//                    if (ordered)
//                    {
//                        for (std::size_t index = 0; index < left.size(); ++index)
//                            if (!left[index].IsSame(right[index]))
//                                return false;
//                        return true;
//                    }
//                    std::vector<bool> used(right.size(), false);
//                    for (const TopoDS_Shape& shape : left)
//                    {
//                        bool found = false;
//                        for (std::size_t index = 0; index < right.size(); ++index)
//                        {
//                            if (!used[index] && shape.IsSame(right[index]))
//                            {
//                                used[index] = true;
//                                found = true;
//                                break;
//                            }
//                        }
//                        if (!found)
//                            return false;
//                    }
//                    return true;
//                };
//            if (!sameShapeList(
//                historicalSelections,
//                selectedShapes,
//                command.operation == MGeoCommand::DirectModelingOperation::ReplaceFace))
//            {
//                return fail(tr(
//                    "当前选择与同序号历史对象不一致，已拒绝更新以避免修改错误几何"));
//            }

//            std::vector<TopoDS_Shape> historicalReferences;
//            for (const TopologyLocator& locator : existingOperation->referenceLocators)
//            {
//                const TopoDS_Shape resolved = ResolveTopologyLocator(
//                    sourcePartSet->getShape(), locator, &locatorError);
//                if (resolved.IsNull())
//                    return fail(tr("已有批量参数的参照对象无法重新定位：%1")
//                        .arg(locatorError));
//                historicalReferences.push_back(resolved);
//            }
//            if (!sameShapeList(historicalReferences, referenceShapes, true))
//            {
//                return fail(tr(
//                    "已有参数只允许更新数值；替换目标面或公共旋转轴已发生变化"));
//            }

//            // 回放成功前只临时安装候选计算器。若几何或初始 ID History 校验失败，
//            // 恢复旧参数表，避免出现“参数值已变但 Shape 仍是旧结果”的半提交状态。
//            auto previousCalculator = std::move(m_parameterCalculator);
//            m_parameterCalculator = std::move(pendingCalculator);
//            resetSession();
//            QString rebuildError;
//            const bool rebuilt = rebuildDrivenOperations(
//                &rebuildError, command.originalFaceIdContext);
//            emit arrowHidden();
//            if (!rebuilt)
//            {
//                m_parameterCalculator = std::move(previousCalculator);
//                emit directModelingParametersChanged();
//                return fail(tr("批量参数更新未提交，几何回放失败：%1")
//                    .arg(rebuildError));
//            }
//            emit directModelingParametersChanged();

//            outputMessage(0, tr("已更新批量直接建模参数：%1")
//                .arg(automaticParameterName));
//            emit commitFinished(true);
//            return true;
//        }

//        const TopoDS_Shape sourcePartShape = sourcePartSet->getShape();
//        std::vector<TopologyLocator> selectedLocators;
//        selectedLocators.reserve(selectedShapes.size());
//        for (const TopoDS_Shape& shape : selectedShapes)
//        {
//            TopologyLocator locator;
//            QString locatorError;
//            if (!CaptureTopologyLocator(
//                sourcePartShape, shape, false, locator, &locatorError))
//            {
//                return fail(tr("无法记录批量选择：%1").arg(locatorError));
//            }
//            selectedLocators.push_back(locator);
//        }
//        std::vector<TopologyLocator> referenceLocators;
//        referenceLocators.reserve(referenceShapes.size());
//        for (std::size_t index = 0; index < referenceShapes.size(); ++index)
//        {
//            TopologyLocator locator;
//            QString locatorError;
//            const bool allowExternal = !referenceBelongsToSourcePart[index];
//            if (!CaptureTopologyLocator(
//                sourcePartShape,
//                referenceShapes[index],
//                allowExternal,
//                locator,
//                &locatorError))
//            {
//                return fail(tr("无法记录批量操作参照：%1").arg(locatorError));
//            }
//            referenceLocators.push_back(locator);
//        }

//        std::vector<MGeoCommand::DirectModelingRequest> requests;
//        requests.reserve(selectedShapes.size());
//#if MXGUI_HAS_DIRECTMODELING_V2
//        gp_Ax1 rotationAxis;
//        if (command.operation == MGeoCommand::DirectModelingOperation::RotateFace)
//        {
//            const BRepAdaptor_Curve curve(TopoDS::Edge(referenceShapes.front()));
//            rotationAxis = curve.Line().Position();
//        }
//#endif // MXGUI_HAS_DIRECTMODELING_V2
//        for (std::size_t index = 0; index < selectedShapes.size(); ++index)
//        {
//            MGeoCommand::DirectModelingRequest request;
//            request.operation = command.operation;
//            request.selectedShape = selectedShapes[index];
//            request.value = effectiveValue;
//            request.mode = MGeoCommand::DirectModelingResultMode::Update;
//#if MXGUI_HAS_DIRECTMODELING_V2
//            request.simplifyResultOnCommit =
//                command.operation != MGeoCommand::DirectModelingOperation::ResizeFillet
//                && command.operation != MGeoCommand::DirectModelingOperation::EdgeFillet;
//#endif // MXGUI_HAS_DIRECTMODELING_V2
//#if MXGUI_HAS_DIRECTMODELING_V2
//            if (command.operation == MGeoCommand::DirectModelingOperation::ReplaceFace)
//            {
//                const std::size_t referenceIndex = referenceShapes.size() == 1
//                    ? 0 : index;
//                request.replacementFace = TopoDS::Face(
//                    referenceShapes[referenceIndex]);
//            }
//            else if (command.operation == MGeoCommand::DirectModelingOperation::RotateFace)
//            {
//                request.rotationAxisPoint = rotationAxis.Location();
//                request.rotationAxisDirection = rotationAxis.Direction();
//                request.hasRotationAxis = true;
//            }
//#endif // MXGUI_HAS_DIRECTMODELING_V2
//            requests.push_back(request);
//        }

//        // 到这里才停止交互预览。几何计算和数据写入前的所有校验均已完成；组合算法
//        // 返回失败时不调用 updatePart，因此不会出现前几个面已改、后一个面失败的状态。
//        invalidatePreviewSession();
//        if (m_previewWorker)
//            m_previewWorker->cancelPendingAndWait();

//        MGeoCommand::DirectModelingGeometry batchGeometry(sourcePartShape);
//        MGeoCommand::DirectModelingResult result;
//        QString geometryError;
//        const bool batchGenerated =
//#if MXGUI_HAS_DIRECTMODELING_V2
//            batchGeometry.generateBatchResult(requests, result, &geometryError);
//#else
//            false; //【GUI 门控】generateBatchResult 未推送，批量直模暂闭
//#endif
//        if (!batchGenerated
//            || result.shape.IsNull())
//        {
//            if (m_geometry.hasActiveOperation())
//                preparePreviewSession();
//            return fail(tr("批量直接建模提交失败：%1").arg(geometryError));
//        }

//        const int drivenPartId = sourcePartSet->getPartID();
//        MechPreData::mViewportModel* drivenViewport = currentViewportModel();
//        MGeoData::GeoDataBase* drivenGeoData = currentGeoData();
//        MGeoData::GeoDataModelSet* updatedPartSet = nullptr;
//        if (!updatePart(
//            sourcePartSet,
//            result.shape,
//            &updatedPartSet,
//#if MXGUI_HAS_DIRECTMODELING_V2
//            result.history,
//#else
//            nullptr, //【GUI 门控】history 字段未推送
//#endif
//            command.originalFaceIdContext)
//            || !updatedPartSet)
//        {
//            if (m_geometry.hasActiveOperation())
//                preparePreviewSession();
//            return fail(tr("批量直接建模结果无法写入部件"));
//        }

//        m_parameterCalculator = std::move(pendingCalculator);
//        retargetDrivenOperations(drivenPartId, drivenViewport, updatedPartSet);
//        if (parameterAlreadyExists && !existingOperation)
//        {
//            // 当前结果包含无法再回放的旧操作，恢复节点必须成为新的 checkpoint。
//            // 只有几何写入成功后才退役旧历史；失败时原参数、旧历史和 Shape 均保留。
//            retireDrivenHistoryForRecovery(
//                automaticParameterId, drivenPartId, drivenViewport, drivenGeoData);
//        }

//        auto drivenOperation = std::make_unique<DrivenOperation>();
//        drivenOperation->parameterId = automaticParameterId;
//        drivenOperation->parameterName = automaticParameterName;
//        drivenOperation->operation = command.operation;
//        drivenOperation->moveSolid = command.moveSolid;
//        drivenOperation->viewportModel = drivenViewport;
//        drivenOperation->geoData = drivenGeoData;
//        drivenOperation->geoDataOwner = DirectParameterDataOwner(drivenOperation->geoData);
//        drivenOperation->tracksGeoDataOwner = !drivenOperation->geoDataOwner.isNull();
//        drivenOperation->currentPartSet = updatedPartSet;
//        drivenOperation->partId = drivenPartId;
//        drivenOperation->sourcePartShape = sourcePartShape;
//        drivenOperation->selectedLocators = std::move(selectedLocators);
//        drivenOperation->referenceLocators = std::move(referenceLocators);
//        drivenOperation->currentValue = effectiveValue;
//        m_drivenOperations.push_back(std::move(drivenOperation));

//        outputMessage(0, tr("已创建或更新批量直接建模参数：%1")
//            .arg(automaticParameterName));
//        resetSession();
//        // 新参数可能被后续公式引用；与单对象提交一致，在退出拾取会话后回放一次
//        // 受影响的参数历史。回放失败只会把相应参数标记为几何失效，不覆盖刚提交的
//        // 最后有效 Shape。
//        QString pendingDrivenError;
//        if (!rebuildDrivenOperations(
//            &pendingDrivenError, command.originalFaceIdContext))
//            outputMessage(2, pendingDrivenError);
//        emit directModelingParametersChanged();
//        emit arrowHidden();
//        emit commitFinished(true);
//        return true;
//    }
}

    // ------------------------------------------------------------------
    // 拖拽交互
    // ------------------------------------------------------------------

    void DirectModelingController::onDragValue(double accumulatedValue)
    {
        if (!m_isActive || !m_geometry.hasActiveOperation())
            return;
        if (!std::isfinite(accumulatedValue))
            return;
#if MXGUI_HAS_DIRECTMODELING_V2
        if (m_currentOperation == MGeoCommand::DirectModelingOperation::RotateFace)
            accumulatedValue = (std::max)(-90.0, (std::min)(90.0, accumulatedValue));
#endif // MXGUI_HAS_DIRECTMODELING_V2

        // DirectTool 的信号值是单次按下后的鼠标增量；Panel 已完成跨多段拖动的
        // 累加。Controller 只接受累计业务参数，避免渲染实现细节进入几何会话。
        m_currentParameter = accumulatedValue;

        // 移动面由 Panel 适配器调用渲染矩阵复用缓存，不逐帧启动 OCC。
        // 旋转由 Tool 的独立圆环分支复用源面显示；编辑框和释放圆环均调用 onDragFinished。
        // 这里仅使上一参数尚在计算的精确结果失效。
        if (m_currentOperation == MGeoCommand::DirectModelingOperation::MoveFace
            || m_currentOperation == MGeoCommand::DirectModelingOperation::OffsetFace
#if MXGUI_HAS_DIRECTMODELING_V2
            || m_currentOperation == MGeoCommand::DirectModelingOperation::RotateFace
#endif // MXGUI_HAS_DIRECTMODELING_V2
            )
        {
            ++m_previewSequence;
            m_previewPending = false;
            if (m_previewConsumeTimer)
                m_previewConsumeTimer->stop();
            return;
        }

        m_pendingPreviewDistance = accumulatedValue;
        m_previewPending = true;
        // 使正在返回的旧结果失效；每一帧重启计时器，拖动停顿后只消费最终值。
        ++m_previewSequence;
        if (m_previewConsumeTimer)
            m_previewConsumeTimer->start();
    }

    void DirectModelingController::onDragFinished(double accumulatedValue)
    {
        if (!m_isActive || !m_geometry.hasActiveOperation())
            return;
        if (!std::isfinite(accumulatedValue))
            return;
#if MXGUI_HAS_DIRECTMODELING_V2
        if (m_currentOperation == MGeoCommand::DirectModelingOperation::RotateFace)
            accumulatedValue = (std::max)(-90.0, (std::min)(90.0, accumulatedValue));
#endif // MXGUI_HAS_DIRECTMODELING_V2

        m_currentParameter = accumulatedValue;
        m_previewPending = false;
        m_pendingPreviewDistance = accumulatedValue;
        if (m_previewConsumeTimer)
            m_previewConsumeTimer->stop();
        // 释放时用完整 OCC 结果覆盖渲染层的刚体近似；仍然只是预览，不修改数据模型。
        doPreview(accumulatedValue, true);
    }

    bool DirectModelingController::onCommit(
        double distance,
        const QString& drivingExpression,
        QString* errorMessage,
        const QString& scriptContextId)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。
    return false;
//【S9 原体（逐行注释保留）】
//        double distance,
//        const QString& drivingExpression,
//        QString* errorMessage,
//        const QString& scriptContextId)
//    {
//        if (errorMessage)
//            errorMessage->clear();
//        auto fail = [this, errorMessage](
//            const QString& message,
//            int messageType,
//            bool restorePreview = false) {
//                if (restorePreview)
//                    preparePreviewSession();
//                if (errorMessage)
//                    *errorMessage = message;
//                outputMessage(messageType, message);
//                emit commitFinished(false);
//                return false;
//            };

//        if (!m_isActive || !m_geometry.hasActiveOperation())
//            return fail(tr("当前没有可提交的直接建模操作"), 2);
//        if (!std::isfinite(distance))
//            return fail(tr("直接建模参数无效"), 2);

//        if (!m_currentPartSet || !m_parameterCalculator)
//            return fail(tr("未找到当前部件或参数计算器，无法提交直接建模结果"), 2);

//        // 每次确定都自动为“部件 + 几何对象 + 操作类型”创建或更新一个参数。
//        // 先在计算器副本中验证，避免几何已经写入后才发现公式循环或未知引用。
//        auto pendingCalculator =
//            std::make_unique<mGeoFeatureParameterCalculator>(*m_parameterCalculator);
//        const QString automaticParameterId = currentParameterObjectKey();
//        if (automaticParameterId.isEmpty())
//            return fail(tr("无法建立直接建模参数的稳定拓扑标识"), 2);
//        bool parameterAlreadyExists = false;
//        for (const GeoFeatureParameter& parameter : m_parameterCalculator->parameters())
//        {
//            if (parameter.id == automaticParameterId)
//            {
//                parameterAlreadyExists = true;
//                break;
//            }
//        }
//        QString automaticParameterName;
//        QString parameterError;
//        if (!pendingCalculator->upsertOperationParameter(
//            automaticParameterId,
//            currentOperationDisplayName(),
//            drivingExpression,
//            distance,
//            &automaticParameterName,
//            &parameterError))
//            return fail(
//                tr("自动参数创建或更新失败：%1").arg(parameterError), 2);

//        // API 可以同时提供一个数值和一个参数表达式。存在表达式时，以计算器得到的
//        // 当前值作为真正的几何参数；传入数值仅作为创建常量参数时的回退值。Panel
//        // 原本已经提前计算公式，因此两条调用路径在这里会得到完全相同的结果。
//        double effectiveDistance = distance;
//        if (!drivingExpression.trimmed().isEmpty())
//        {
//            bool parameterFound = false;
//            for (const GeoFeatureParameter& parameter : pendingCalculator->parameters())
//            {
//                if (parameter.id == automaticParameterId)
//                {
//                    parameterFound = true;
//                    effectiveDistance = parameter.value;
//                    break;
//                }
//            }
//            if (!parameterFound || !std::isfinite(effectiveDistance))
//                return fail(tr("无法取得直接建模表达式的有效计算结果"), 2);
//        }

//#if MXGUI_HAS_DIRECTMODELING_V2
//        if (m_currentOperation == MGeoCommand::DirectModelingOperation::RotateFace
//            && std::abs(effectiveDistance) > 90.0)
//            return fail(tr("旋转面角度必须在 -90° 到 90° 之间"), 2);
//#endif // MXGUI_HAS_DIRECTMODELING_V2

//        // 零值不是一次有效的几何生成，保留当前会话供用户继续输入或拖动。
//        if (std::abs(effectiveDistance) <= 1e-12
//            && m_currentOperation != MGeoCommand::DirectModelingOperation::ReplaceFace)
//            return fail(tr("请输入非零参数后再确定"), 1);
//        m_currentParameter = effectiveDistance;

//        // 已有参数表示“修改同一个历史操作的驱动值”，不能在当前结果上再执行一次
//        // 几何算法，否则回放链会同时包含新旧两个同名节点并产生双重移动/旋转。
//        // 这里接纳新公式后直接从该部件的基准 Shape 回放整条历史。
//        DrivenOperation* existingOperation = nullptr;
//        if (parameterAlreadyExists)
//        {
//            const int partId = m_currentPartSet->getPartID();
//            MechPreData::mViewportModel* viewportModel = currentViewportModel();
//            MGeoData::GeoDataBase* geoData = currentGeoData();
//            for (const std::unique_ptr<DrivenOperation>& operation : m_drivenOperations)
//            {
//                if (!operation || operation->parameterId != automaticParameterId)
//                    continue;
//                // 参数键不含模型身份。其他视口/数据库中的同键节点不是本模型丢失的
//                // 历史，必须拒绝覆盖；已销毁数据库的 raw 地址也不能重新认作当前库。
//                if (operation->partId != partId
//                    || operation->viewportModel != viewportModel
//                    || (operation->geoData && operation->geoData != geoData)
//                    || (operation->tracksGeoDataOwner && operation->geoDataOwner.isNull()))
//                {
//                    return fail(tr("参数的历史属于其他模型或已失效的数据源，不能覆盖"), 2);
//                }
//                if (operation->historyRetired)
//                    continue;
//                if (existingOperation)
//                    return fail(tr("参数对应多个历史节点，不能唯一确定要更新的操作"), 2);
//                existingOperation = operation.get();
//            }
//            if (!existingOperation && !scriptContextId.trimmed().isEmpty())
//            {
//                return fail(tr(
//                    "参数历史缺失，无法保证 original-ID 脚本的初始拓扑 History；"
//                    "本次操作未提交，请先退出原始 ID 上下文并在当前几何上恢复参数"), 2);
//            }
//            // 没有任何活动同键历史时允许修复孤立/已退役参数：复用 ID 和名称，以当前 Shape
//            // 重新记录一次操作；复用下面的新节点提交流程保证失败不写正式参数/历史。
//        }

//        if (existingOperation)
//        {
//            if (existingOperation->selectedLocators.size() != 1)
//                return fail(tr("已有参数的选择历史与当前单对象操作不一致"), 2);
//            if (existingOperation->moveSolid != m_moveSolid)
//            {
//                return fail(tr(
//                    "已有参数的作用域与当前操作不一致，不能在面移动和实体移动之间切换"), 2);
//            }

//            QString locatorError;
//            const TopoDS_Shape historicalSelection = ResolveTopologyLocator(
//                m_currentPartSet->getShape(),
//                existingOperation->selectedLocators.front(),
//                &locatorError);
//            if (historicalSelection.IsNull())
//                return fail(tr("已有参数的选择对象无法重新定位：%1")
//                    .arg(locatorError), 2);
//            if (!historicalSelection.IsSame(
//                m_geometry.activeSelection().selectedShape))
//            {
//                return fail(tr(
//                    "当前选择与同序号历史对象不一致，已拒绝更新以避免修改错误几何"), 2);
//            }

//            const bool currentHasReference = !m_referenceShape.IsNull();
//            if (existingOperation->referenceLocators.size()
//                != static_cast<std::size_t>(currentHasReference ? 1 : 0))
//            {
//                return fail(tr("已有参数只允许更新数值，不能更换操作参照"), 2);
//            }
//            if (currentHasReference)
//            {
//                const TopoDS_Shape historicalReference = ResolveTopologyLocator(
//                    m_currentPartSet->getShape(),
//                    existingOperation->referenceLocators.front(),
//                    &locatorError);
//                if (historicalReference.IsNull())
//                    return fail(tr("已有参数的参照对象无法重新定位：%1")
//                        .arg(locatorError), 2);
//                if (!historicalReference.IsSame(m_referenceShape))
//                    return fail(tr("已有参数只允许更新数值，不能更换目标面或旋转轴"), 2);
//            }

//            // 与批量路径一致：候选参数仅在整条几何历史成功回放后才正式接纳。
//            auto previousCalculator = std::move(m_parameterCalculator);
//            m_parameterCalculator = std::move(pendingCalculator);
//            resetSession();
//            QString rebuildError;
//            const bool rebuilt = rebuildDrivenOperations(
//                &rebuildError, scriptContextId);
//            emit arrowHidden();
//            if (!rebuilt)
//            {
//                m_parameterCalculator = std::move(previousCalculator);
//                emit directModelingParametersChanged();
//                return fail(tr("参数更新未提交，几何回放失败：%1").arg(rebuildError), 2);
//            }
//            emit directModelingParametersChanged();

//            outputMessage(0, tr("已更新直接建模参数：%1")
//                .arg(automaticParameterName));
//            emit commitFinished(true);
//            return true;
//        }

//        // 为参数历史保存与 GeoModelSet 子几何 ID 无关的拓扑定位信息。源选择必须属于
//        // 当前输入部件；替换面的外部目标允许保留为外部 Shape。
//        const TopoDS_Shape drivenSourceShape = m_currentPartSet->getShape();
//        TopologyLocator selectedLocator;
//        QString locatorError;
//        if (!CaptureTopologyLocator(
//            drivenSourceShape,
//            m_geometry.activeSelection().selectedShape,
//            false,
//            selectedLocator,
//            &locatorError))
//            return fail(
//                tr("无法建立参数驱动历史：%1").arg(locatorError), 2);

//        const bool hasReference = !m_referenceShape.IsNull();
//        TopologyLocator referenceLocator;
//        if (hasReference
//            && !CaptureTopologyLocator(
//                drivenSourceShape,
//                m_referenceShape,
//                !m_referenceShapeBelongsToCurrentPart,
//                referenceLocator,
//                &locatorError))
//            return fail(tr("无法记录操作参照：%1").arg(locatorError), 2);
//        const int drivenPartId = m_currentPartSet->getPartID();
//        MechPreData::mViewportModel* drivenViewport = currentViewportModel();
//        MGeoData::GeoDataBase* drivenGeoData = currentGeoData();

//        // 确定操作前停止后台预览，避免 OCC 同时读取同一份拓扑数据。
//        invalidatePreviewSession();
//        if (m_previewWorker)
//            m_previewWorker->cancelPendingAndWait();

//        QString geometryError;
//        MGeoCommand::DirectModelingResult result;
//        // finish 会结束几何对象内部会话，因此在副本上生成最终 Shape。只有数据层
//        // updatePart 成功后才重置正式会话；失败时仍可恢复预览并保留用户选择。
//        MGeoCommand::DirectModelingGeometry commitGeometry(m_geometry);
//        if (!commitGeometry.finish(effectiveDistance, result, &geometryError))
//            return fail(
//                tr("直接建模提交失败：%1").arg(geometryError), 2, true);

//        if (!m_currentPartSet)
//            return fail(
//                tr("未找到当前部件，无法提交直接建模结果"), 2, true);

//        TopoDS_Shape shapeToUpdate = result.shape;
//        Handle(BRepTools_History) historyToUpdate =
//#if MXGUI_HAS_DIRECTMODELING_V2
//            result.history;
//#else
//            nullptr; //【GUI 门控】history 字段未推送
//#endif // MXGUI_HAS_DIRECTMODELING_V2
//        if (result.mode == MGeoCommand::DirectModelingResultMode::Copy)
//        {
//            // 复制模式：将结果 shape 合并到原部件 Compound 中
//            const TopoDS_Shape originalShape = m_currentPartSet->getShape();
//            if (originalShape.IsNull())
//                return fail(
//                    tr("原部件几何为空，无法合并复制结果"), 2, true);

//            TopoDS_Compound mergedShape;
//            BRep_Builder builder;
//            builder.MakeCompound(mergedShape);
//            builder.Add(mergedShape, originalShape);
//            builder.Add(mergedShape, result.shape);
//            shapeToUpdate = mergedShape;
//            // Copy 模式同时保留旧拓扑并加入新拓扑，不是一对一替换。
//            // 将 Update 模式的历史用在这里会导致同一旧 ID 有两个候选，因此不传。
//            historyToUpdate.Nullify();
//        }

//        MGeoData::GeoDataModelSet* updatedPartSet = nullptr;
//        if (!updatePart(
//            m_currentPartSet,
//            shapeToUpdate,
//            &updatedPartSet,
//            historyToUpdate,
//            scriptContextId))
//            return fail(tr("更新直接建模部件失败"), 2, true);
//        if (updatedPartSet)
//        {
//            // 到此几何已成功写入，正式接纳预校验后的参数计算器状态。
//            m_parameterCalculator = std::move(pendingCalculator);

//            // updatePart 会移除旧 GeoModelSet；必须先把同一部件的全部历史节点重定向
//            // 到新对象，旧参数才能在后续公式修改时继续参与完整历史回放。
//            retargetDrivenOperations(drivenPartId, drivenViewport, updatedPartSet);
//            if (parameterAlreadyExists && !existingOperation)
//            {
//                retireDrivenHistoryForRecovery(
//                    automaticParameterId, drivenPartId, drivenViewport, drivenGeoData);
//            }

//            auto drivenOperation = std::make_unique<DrivenOperation>();
//            drivenOperation->parameterId = automaticParameterId;
//            drivenOperation->parameterName = automaticParameterName;
//            drivenOperation->operation = m_currentOperation;
//            drivenOperation->moveSolid = m_moveSolid;
//            drivenOperation->viewportModel = drivenViewport;
//            drivenOperation->geoData = drivenGeoData;
//            drivenOperation->geoDataOwner = DirectParameterDataOwner(drivenOperation->geoData);
//            drivenOperation->tracksGeoDataOwner = !drivenOperation->geoDataOwner.isNull();
//            drivenOperation->currentPartSet = updatedPartSet;
//            drivenOperation->partId = drivenPartId;
//            drivenOperation->sourcePartShape = drivenSourceShape;
//            drivenOperation->selectedLocators.push_back(selectedLocator);
//            if (hasReference)
//                drivenOperation->referenceLocators.push_back(referenceLocator);
//            drivenOperation->currentValue = effectiveDistance;
//            m_drivenOperations.push_back(std::move(drivenOperation));
//            outputMessage(0, tr("已创建或更新直接建模参数：%1").arg(automaticParameterName));
//        }

//        // 提交后面/边 ID 会由数据层重新编号，旧选择和箭头都不可继续复用。
//        resetSession();
//        QString pendingDrivenError;
//        if (!rebuildDrivenOperations(&pendingDrivenError, scriptContextId))
//            outputMessage(2, pendingDrivenError);
//        emit directModelingParametersChanged();
//        emit arrowHidden();
//        emit commitFinished(true);
//        return true;
//    }
}

    void DirectModelingController::onCancel()
    {
        if (!m_isActive)
            return;

        resetSession();
        QString pendingDrivenError;
        if (!rebuildDrivenOperations(&pendingDrivenError))
            outputMessage(2, pendingDrivenError);
        emit directModelingParametersChanged();
        emit arrowHidden();
    }

    // ------------------------------------------------------------------
    // 会话状态查询
    // ------------------------------------------------------------------

    bool DirectModelingController::hasActiveOperation() const
    {
        return m_geometry.hasActiveOperation();
    }

    bool DirectModelingController::buildRenderPreviewSource(
        std::vector<glm::vec3>& facePoints,
        std::vector<glm::vec3>& edgePoints) const
    {
        facePoints.clear();
        edgePoints.clear();

        if (!m_isActive || !m_geometry.hasActiveOperation()
            || !m_previewGeometrySnapshot)
        {
            return false;
        }
#if MXGUI_HAS_DIRECTMODELING_V2
        const bool geometryMoveSolid = m_geometry.moveSolidMode();
        const bool snapshotMoveSolid = m_previewGeometrySnapshot->moveSolidMode();
        if (geometryMoveSolid != m_moveSolid
            || snapshotMoveSolid != m_moveSolid)
        {
            outputMessage(
                2,
                tr("实体平移预览状态不同步，已拒绝使用旧的单面预览数据"));
            return false;
        }
#endif // MXGUI_HAS_DIRECTMODELING_V2

        if (m_currentOperation != MGeoCommand::DirectModelingOperation::MoveFace
            && m_currentOperation != MGeoCommand::DirectModelingOperation::OffsetFace
#if MXGUI_HAS_DIRECTMODELING_V2
            && m_currentOperation != MGeoCommand::DirectModelingOperation::RotateFace
#endif // MXGUI_HAS_DIRECTMODELING_V2
            )
        {
            return false;
        }

        // 只在操作启动/选择变化时调用一次。平移源面/实体由适配器缓存；旋转源面
        // 经独立接口交给 Tool。两者均不逐帧访问 OCC。
        //MGeoCommand::GeoPreviewData source; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
        QString errorMessage;
        const bool sourceReady =
#if MXGUI_HAS_DIRECTMODELING_V2
            m_previewGeometrySnapshot->sourceDiscretized(source, &errorMessage);
#else
            false; //【GUI 门控】sourceDiscretized 未推送
#endif
        if (!sourceReady)
        {
            outputMessage(1, tr("无法创建渲染侧即时预览：%1").arg(errorMessage));
            return false;
        }

        facePoints = ToRenderPoints(source.facePoint);
        // 实体模式需要完整轮廓线来显示整个实体的移动；适配器不会将这些边
        // 交给 DirectTool 补扫掠侧面，而是与三角面一样只平移一次。
        edgePoints = ToRenderPoints(source.edgePoint);
        return !facePoints.empty() || !edgePoints.empty();
    }

    bool DirectModelingController::resolveSelection(int dim, int id, TopoDS_Shape& shape,
        MGeoData::GeoDataModelSet*& partSet)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。
    return false;
//【S9 原体（逐行注释保留）】
//        MGeoData::GeoDataModelSet*& partSet)
//    {
//        // 渲染拾取得到的是业务 ID，不是稳定的 TopoDS_Shape。每次启动操作都必须从
//        // 当前 GeoDataBase 重新解析，以适应上一次提交后的拓扑 ID 重建。
//        partSet = nullptr;
//        shape = TopoDS_Shape();

//        MGeoData::GeoDataBase* geoData = currentGeoData();
//        if (!geoData)
//        {
//            outputMessage(2, tr("无法获取几何数据"));
//            return false;
//        }

//        if (dim == 2)
//        {
//            partSet = geoData->getPartSetByGeoFaceID(id);
//            if (!partSet)
//            {
//                outputMessage(2, tr("所选面不属于任何部件"));
//                return false;
//            }
//            shape = partSet->getFaceByTag(id);
//        }
//        else if (dim == 1)
//        {
//            partSet = geoData->getPartSetByGeoLineID(id);
//            if (!partSet)
//            {
//                outputMessage(2, tr("所选边不属于任何部件"));
//                return false;
//            }
//            shape = partSet->getEdgeByTag(id);
//        }
//        else
//        {
//            outputMessage(1, tr("当前仅支持几何面或几何边进行直接建模"), true);
//            return false;
//        }

//        if (shape.IsNull())
//        {
//            outputMessage(2, tr("无法获取所选几何形状"));
//            return false;
//        }

//        return true;
//    }
}

    // ------------------------------------------------------------------
    // 会话管理
    // ------------------------------------------------------------------

    bool DirectModelingController::startDirectModelingOperation(
        MGeoCommand::DirectModelingOperation requestedOperation,
        bool isOffset,
        bool moveSolid)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模功能后续迁移 RebuildGeometry（引擎编辑族）。
    return false;
    //【S9 原体（逐行注释保留）】
//        MGeoCommand::DirectModelingOperation requestedOperation,
//        bool isOffset,
//        bool moveSolid)
//    {
//        m_currentPartSet = nullptr;
//        m_currentOperation = MGeoCommand::DirectModelingOperation::None;
//        m_moveSolid = false;
//        m_geometry.cancel();

//        if (m_selectedElementId < 0 || m_selectedElementDim < 0)
//        {
//            outputMessage(1, tr("请先选择一个几何面或几何边"), true);
//            return false;
//        }

//        TopoDS_Shape selectedShape;
//        if (!resolveSelection(m_selectedElementDim, m_selectedElementId,
//            selectedShape, m_currentPartSet))
//        {
//            return false;
//        }

//        // 几何层必须持有完整部件作为基准，不能只把被选面/边当作 partShape。
//        m_geometry.setPartShape(m_currentPartSet->getShape());
//        MGeoCommand::DirectSelectionInfo info = m_geometry.analyzeSelection(selectedShape);
//        if (!info.isValid())
//        {
//            outputMessage(2, tr("所选几何无法用于直接建模"));
//            return false;
//        }

//        MGeoCommand::DirectModelingOperation operation = requestedOperation;
//        if (operation == MGeoCommand::DirectModelingOperation::None)
//        {
//            operation = operationFromSelection(info, isOffset);
//        }
//        if (operation == MGeoCommand::DirectModelingOperation::None || !info.supports(operation))
//        {
//            if (operation == MGeoCommand::DirectModelingOperation::EdgeFillet)
//                outputMessage(2, tr("倒圆角需要选择由两个面共享的几何边"));
//            else
//                outputMessage(2, tr("当前选择不支持该操作"));
//            return false;
//        }
//        if (moveSolid
//            && (m_selectedElementDim != 2
//                || (operation != MGeoCommand::DirectModelingOperation::MoveFace
//                    && operation != MGeoCommand::DirectModelingOperation::OffsetFace)))
//        {
//            outputMessage(2, tr("移动实体只能在移动面操作中使用，且必须只选择一个几何面"));
//            return false;
//        }

//        QString errorMessage;
//        if (!m_geometry.beginOperation(selectedShape, operation,
//            MGeoCommand::DirectModelingResultMode::Update,
//            &errorMessage))
//        {
//            outputMessage(2, tr("启动直接建模失败：%1").arg(errorMessage));
//            return false;
//        }
//        // 几何会话先完成普通选择分析，再切换作用域。这样箭头仍使用选中面的
//        // 外法向，而几何层可以把预览、正式提交和 History 统一改为所属实体。
//        if (
//#if MXGUI_HAS_DIRECTMODELING_V2
//            !m_geometry.setMoveSolidMode(moveSolid)
//            || m_geometry.moveSolidMode() != moveSolid
//#endif // MXGUI_HAS_DIRECTMODELING_V2
//            false) //【GUI 门控】V2 关闭时跳过作用域校验
//        {
//            outputMessage(
//                2,
//                moveSolid
//                ? tr("所选面无法解析出唯一的所属实体，不能启用偏置模式")
//                : tr("直接建模作用域状态初始化失败"));
//            m_geometry.cancel();
//            m_currentPartSet = nullptr;
//            m_currentOperation = MGeoCommand::DirectModelingOperation::None;
//            m_moveSolid = false;
//            m_selectedElementId = -1;
//            m_selectedElementDim = -1;
//            emit previewClearRequested();
//            emit arrowHidden();
//            return false;
//        }
//        m_currentOperation = operation;
//        m_moveSolid = moveSolid;
//        const MGeoCommand::DirectSelectionInfo& activeInfo =
//            m_geometry.activeSelection();
//        m_arrowCenter = activeInfo.arrowCenter;
//        m_arrowDirection = activeInfo.arrowDirection;
//        m_currentParameter = operation
//            == MGeoCommand::DirectModelingOperation::ResizeFillet
//            ? activeInfo.detectedRadius : 0.0;
//        return true;
//    }
}

    void DirectModelingController::resetSession()
    {
        // 顺序上先使后台结果失效，再清几何会话和非拥有指针，最后通知渲染清屏。
        invalidatePreviewSession();
        m_geometry.cancel();
        m_currentOperation = MGeoCommand::DirectModelingOperation::None;
        m_moveSolid = false;
        m_currentParameter = 0.0;
        m_currentPartSet = nullptr;
        m_selectedElementId = -1;
        m_selectedElementDim = -1;
        m_referenceShape = TopoDS_Shape();
        m_referenceShapeBelongsToCurrentPart = false;
        m_arrowCenter = QVector3D(0.0f, 0.0f, 0.0f);
        m_arrowDirection = QVector3D(0.0f, 0.0f, 1.0f);
        emit previewClearRequested();
    }
    MGeoCommand::DirectModelingOperation DirectModelingController::operationFromSelection(
        //const MGeoCommand::DirectSelectionInfo& info, bool isOffset) const //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
    {
        if (!info.isValid())
            return MGeoCommand::DirectModelingOperation::None;

        //if (info.selectionType == MGeoCommand::DirectSelectionType::Face) //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
        {
            // 完整圆柱面优先修改半径
            if (info.supports(MGeoCommand::DirectModelingOperation::ResizeCylinder))
            {
                return MGeoCommand::DirectModelingOperation::ResizeCylinder;
            }

            // 圆角面优先调整圆角半径
            if (info.supports(MGeoCommand::DirectModelingOperation::ResizeFillet))
            {
                return MGeoCommand::DirectModelingOperation::ResizeFillet;
            }

            if (isOffset && info.supports(MGeoCommand::DirectModelingOperation::OffsetFace))
            {
                return MGeoCommand::DirectModelingOperation::OffsetFace;
            }
            if (info.supports(MGeoCommand::DirectModelingOperation::MoveFace))
            {
                return MGeoCommand::DirectModelingOperation::MoveFace;
            }
            return MGeoCommand::DirectModelingOperation::None;
        }

        //if (info.selectionType == MGeoCommand::DirectSelectionType::Edge) //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
        {
            //if (info.edgeType == MGeoCommand::DirectEdgeType::Shared //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                && info.supports(MGeoCommand::DirectModelingOperation::EdgeFillet))
            {
                return MGeoCommand::DirectModelingOperation::EdgeFillet;
            }
            //if ((info.edgeType == MGeoCommand::DirectEdgeType::Free //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                //|| info.edgeType == MGeoCommand::DirectEdgeType::Independent) //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                && info.supports(MGeoCommand::DirectModelingOperation::ExtrudeEdge))
            {
                return MGeoCommand::DirectModelingOperation::ExtrudeEdge;
            }
        }

        return MGeoCommand::DirectModelingOperation::None;
    }

    // ------------------------------------------------------------------
    // 预览与提交
    // ------------------------------------------------------------------

    void DirectModelingController::doPreview(double distance, bool exact)
    {
        if (!m_geometry.hasActiveOperation()
            || !m_previewGeometrySnapshot
            || !m_previewWorker)
            return;

        // sequence 是同一 session 内的单调版本号；handlePreviewResult 只接受最新值。
        const quint64 sequence = ++m_previewSequence;
        m_previewWorker->submit(
            m_previewSessionId,
            sequence,
            m_previewGeometrySnapshot,
            distance,
            exact);
    }

    void DirectModelingController::preparePreviewSession()
    {
        ++m_previewSessionId;
        m_previewSequence = 0;
        m_previewPending = false;
        m_pendingPreviewDistance = 0.0;
        // DirectModelingGeometry 的拷贝拥有独立 Impl，会话参数固定为当前选择。
        m_previewGeometrySnapshot =
            //std::make_shared<const MGeoCommand::DirectModelingGeometry>(m_geometry); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
    }

    void DirectModelingController::invalidatePreviewSession()
    {
        ++m_previewSessionId;
        m_previewSequence = 0;
        m_previewGeometrySnapshot.reset();
        m_previewPending = false;
        m_pendingPreviewDistance = 0.0;
        if (m_previewConsumeTimer)
            m_previewConsumeTimer->stop();
    }

    void DirectModelingController::handlePreviewResult(
        quint64 sessionId,
        quint64 sequence,
        bool success,
        QString errorMessage,
        std::vector<glm::vec3> addedFacePoints,
        std::vector<glm::vec3> addedEdgePoints,
        std::vector<glm::vec3> removedFacePoints,
        std::vector<glm::vec3> removedEdgePoints)
    {
        if (sessionId != m_previewSessionId
            || sequence != m_previewSequence
            || !m_geometry.hasActiveOperation())
        {
            return;
        }

        if (!success)
        {
            // 清除本参数的失败预览，不销毁旋转圆环/有效会话；用户仍可修改角度重试。
            // 空数据只清显示缓存，避免继续把上一角度或半份扫掠当作成功的几何结果。
            emit previewDataReady({}, {}, {}, {});
            outputMessage(2, tr("预览失败：%1").arg(errorMessage));
            return;
        }

        emit previewDataReady(
            std::move(addedFacePoints),
            std::move(addedEdgePoints),
            std::move(removedFacePoints),
            std::move(removedEdgePoints));
    }

    QString DirectModelingController::currentParameterObjectKey() const
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。
    return QString();
//【S9 原体（逐行注释保留）】
//    {
//        if (!m_currentPartSet || m_selectedElementId < 0 || m_selectedElementDim < 0)
//            return QString();
//        // 业务面/边 ID 会在 updatePart 后重新分配，不能作为参数身份。这里改用被选
//        // 子形状在当前部件同类型拓扑表中的位置；参数历史回放本来就使用相同定位规则。
//        // operation 仍参与键值，避免同一面上的距离和角度参数互相覆盖。
//        const TopoDS_Shape& selectedShape = m_geometry.activeSelection().selectedShape;
//        if (selectedShape.IsNull())
//            return QString();
//        if (!m_commandParameterObjectKeyOverride.isEmpty())
//            return m_commandParameterObjectKeyOverride;
//        TopTools_IndexedMapOfShape subShapes;
//        TopExp::MapShapes(
//            m_currentPartSet->getShape(), selectedShape.ShapeType(), subShapes);
//        const int topologyIndex = subShapes.FindIndex(selectedShape);
//        if (topologyIndex <= 0)
//            return QString();
//        QString parameterKey = QStringLiteral(
//            "part:%1/dim:%2/topology:%3/operation:%4")
//            .arg(m_currentPartSet->getPartID())
//            .arg(m_selectedElementDim)
//            .arg(topologyIndex)
//            .arg(static_cast<int>(m_currentOperation));
//        // 原有移动面参数键不能改变，否则已有参数将无法继续更新。只有实体移动追加作用域。
//        if (m_moveSolid)
//            parameterKey += QStringLiteral("/scope:solid");
//        return parameterKey;
//    }
}

    QString DirectModelingController::currentOperationDisplayName() const
    {
        switch (m_currentOperation)
        {
        case MGeoCommand::DirectModelingOperation::ReplaceFace:
            return tr("替换面");
#if MXGUI_HAS_DIRECTMODELING_V2
        case MGeoCommand::DirectModelingOperation::RotateFace:
            return tr("旋转面");
#endif // MXGUI_HAS_DIRECTMODELING_V2
        case MGeoCommand::DirectModelingOperation::MoveFace:
        case MGeoCommand::DirectModelingOperation::OffsetFace:
            return m_moveSolid ? tr("移动实体") : tr("移动面");
        case MGeoCommand::DirectModelingOperation::ResizeFillet:
        case MGeoCommand::DirectModelingOperation::EdgeFillet:
            return tr("调整圆角大小");
        case MGeoCommand::DirectModelingOperation::ResizeCylinder:
            return tr("调整圆柱大小");
        case MGeoCommand::DirectModelingOperation::ExtrudeEdge:
        case MGeoCommand::DirectModelingOperation::ExtrudeVertex:
            return tr("拉伸");
        case MGeoCommand::DirectModelingOperation::DeleteFace:
            return tr("删除面");
        default:
            return tr("直接建模");
        }
    }

    void DirectModelingController::refreshDirectModelingParameters()
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。

//【S9 原体（逐行注释保留）】
//    {
//        MechPreData::mViewportModel* viewportModel = currentViewportModel();
//        MGeoData::GeoDataBase* geoData = currentGeoData();

//        struct PartSnapshot
//        {
//            QHash<int, MGeoData::GeoDataModelSet*> byId;
//            QSet<int> ambiguousIds;
//            QSet<MGeoData::GeoDataModelSet*> liveParts;
//        };
//        // 每个数据库只遍历一次。参数是应用级数据，不能用当前视口的部件表清理
//        // 其他仍打开模型，也不能因当前视口为空而跳过已确认销毁的旧模型。
//        QHash<MGeoData::GeoDataBase*, PartSnapshot> snapshots;
//        auto snapshotFor = [&](MGeoData::GeoDataBase* data) -> const PartSnapshot& {
//            auto it = snapshots.find(data);
//            if (it != snapshots.end())
//                return it.value();
//            PartSnapshot snapshot;
//            for (MGeoData::GeoDataModelSet* partSet : data->getPartSet())
//            {
//                if (!partSet || partSet->getShape().IsNull())
//                    continue;
//                // clearSetData() 会清空全部拓扑编号表，但可能保留旧 _Mshape/partId。
//                // 因此只检查 Shape 非空会把已删除的空壳当作仍然存在的几何部件。
//                if (partSet->getIntegerVretexMap().IsEmpty()
//                    && partSet->getIntegerEdgeMap().IsEmpty()
//                    && partSet->getIntegerFaceMap().IsEmpty()
//                    && partSet->getIntegerSolidMap().IsEmpty())
//                {
//                    continue;
//                }
//                snapshot.liveParts.insert(partSet);
//                const int partId = partSet->getPartID();
//                if (snapshot.byId.contains(partId)
//                    && snapshot.byId.value(partId) != partSet)
//                    snapshot.ambiguousIds.insert(partId);
//                else
//                    snapshot.byId.insert(partId, partSet);
//            }
//            return snapshots.insert(data, snapshot).value();
//            };

//        if (m_parameterCalculator)
//        {
//            m_drivenOperations.erase(
//                std::remove_if(m_drivenOperations.begin(), m_drivenOperations.end(),
//                    [&](const std::unique_ptr<DrivenOperation>& operation) {
//                        if (!operation)
//                            return true;
//                        if (operation->tracksGeoDataOwner
//                            && operation->geoDataOwner.isNull())
//                        {
//                            // 数据库确已销毁：即使没有当前视口，也应移除其全部历史。
//                            return true;
//                        }
//                        MGeoData::GeoDataBase* operationData = nullptr;
//                        if (operation->tracksGeoDataOwner)
//                        {
//                            // 只有弱生命周期仍有效，才允许查询非当前数据库。
//                            operationData = operation->geoData;
//                        }
//                        else if (viewportModel && operation->viewportModel == viewportModel
//                            && (!operation->geoData || operation->geoData == geoData))
//                        {
//                            // 兼容非 QObject 数据库：只查询调用方提供的当前有效库，
//                            // 不根据失效部件弱指针猜测其他模型是否被删除。
//                            operationData = geoData;
//                        }
//                        if (!operationData)
//                            return false;

//                        const PartSnapshot& parts = snapshotFor(operationData);
//                        if (!parts.byId.contains(operation->partId))
//                            return true;
//                        if (parts.ambiguousIds.contains(operation->partId))
//                        {
//                            // 回放会重新计算几何校验状态；必须同时断开有歧义的绑定，
//                            // 避免清除错误标记后仍沿旧指针修改其中一个同 ID 部件。
//                            operation->currentPartSet.clear();
//                            m_parameterCalculator->setGeometryInvalid(
//                                operation->parameterId,
//                                tr("当前模型包含重复部件 ID，无法唯一绑定参数历史"));
//                            return false;
//                        }
//                        // 合法编辑会销毁旧部件并创建同 partId 新部件；必须先查现有
//                        // 部件表再重定向，不能仅因 currentPartSet 归零就删除参数。
//                        operation->currentPartSet = parts.byId.value(operation->partId);
//                        operation->geoData = operationData;
//                        operation->geoDataOwner = DirectParameterDataOwner(operationData);
//                        operation->tracksGeoDataOwner = !operation->geoDataOwner.isNull();
//                        return false;
//                    }),
//                m_drivenOperations.end());

//            QSet<QString> retainedParameterIds;
//            for (const std::unique_ptr<DrivenOperation>& operation : m_drivenOperations)
//            {
//                if (operation)
//                    retainedParameterIds.insert(operation->parameterId);
//            }
//            QSet<QString> removedParameterIds;
//            for (const GeoFeatureParameter& parameter : m_parameterCalculator->parameters())
//            {
//                if (!retainedParameterIds.contains(parameter.id))
//                    removedParameterIds.insert(parameter.id);
//            }
//            // 被删参数的直接/传递依赖保留公式并标为失效，不连带删除其他部件参数。
//            m_parameterCalculator->removeParameters(removedParameterIds);
//        }

//        // 参数通知会同步触发 Panel 的公式预览。先取消已失去实际部件的活动会话，
//        // 防止沿用删除前的缓存；只清交互，不调用会回放几何的 onCancel()。
//        if (m_geometry.hasActiveOperation()
//            && (!geoData || !snapshotFor(geoData).liveParts.contains(m_currentPartSet)))
//        {
//            resetSession();
//            emit arrowHidden();
//        }
//        // 数据暂不可读时保留无法确认删除的参数，但始终让表格重新读取当前快照。
//        emit directModelingParametersChanged();
//    }
}

    void DirectModelingController::retargetDrivenOperations(
        int partId,
        MechPreData::mViewportModel* viewportModel,
        MGeoData::GeoDataModelSet* updatedPartSet)
    {
        if (partId < 0 || !viewportModel || !updatedPartSet)
            return;
        MGeoData::GeoDataBase* geoData = currentGeoData();
        for (const std::unique_ptr<DrivenOperation>& operation : m_drivenOperations)
        {
            if (operation
                && operation->partId == partId
                && operation->viewportModel == viewportModel
                && (!operation->geoData || operation->geoData == geoData)
                && (!operation->tracksGeoDataOwner || !operation->geoDataOwner.isNull()))
            {
                operation->currentPartSet = updatedPartSet;
                operation->geoData = geoData;
                operation->geoDataOwner = DirectParameterDataOwner(operation->geoData);
                operation->tracksGeoDataOwner = !operation->geoDataOwner.isNull();
            }
        }
    }

    void DirectModelingController::retireDrivenHistoryForRecovery(
        const QString& recoveredParameterId,
        int partId,
        MechPreData::mViewportModel* viewportModel,
        MGeoData::GeoDataBase* geoData)
    {
        // 仅在当前几何已经成功写入且候选计算器正式接纳后调用。缺失的中间操作
        // 无法还原，其贡献只能保留在恢复节点的 sourcePartShape 中；旧节点若再次
        // 从更早 Shape 回放，就会静默抹掉这些已烘焙的几何，因此整段旧历史退役。
        m_drivenOperations.erase(
            std::remove_if(m_drivenOperations.begin(), m_drivenOperations.end(),
                [&](const std::unique_ptr<DrivenOperation>& operation) {
                    if (!operation || operation->partId != partId
                        || operation->viewportModel != viewportModel
                        || (operation->geoData && operation->geoData != geoData)
                        || (operation->tracksGeoDataOwner && operation->geoDataOwner.isNull()))
                    {
                        return false;
                    }
                    // 用户可再次选中旧对象确认以逐步恢复参数。只替换同一个稳定 ID 的
                    // 退役记录，参数行/名称/公式不删；其他旧参数保留为几何失效状态。
                    if (operation->parameterId == recoveredParameterId)
                        return true;
                    operation->historyRetired = true;
                    return false;
                }),
            m_drivenOperations.end());
        markRetiredDrivenParametersInvalid();
    }

    void DirectModelingController::markRetiredDrivenParametersInvalid()
    {
        if (!m_parameterCalculator)
            return;
        for (const std::unique_ptr<DrivenOperation>& operation : m_drivenOperations)
        {
            if (operation && operation->historyRetired)
            {
                m_parameterCalculator->setGeometryInvalid(
                    operation->parameterId,
                    tr("历史断链：旧几何效果已保留在新基准中，此参数不再驱动旧操作；"
                        "可重新选择对应对象并确定以恢复参数"));
            }
        }
    }

    bool DirectModelingController::rebuildDrivenOperations(
        QString* errorMessage,
        const QString& scriptContextId)
    {
#if !MXGUI_HAS_DIRECTMODELING_V2
        //【GUI 门控】驱动重建链整体依赖未推送 API（simplify/rotationAxis/history/batch）
        return true; // 视为无驱动项，保持既有操作可用
#else
        if (errorMessage)
            errorMessage->clear();
        // 参数表在活动拾取期间只更新当前预览；此时替换正式部件会使现有面/边 ID 失效。
        if (m_geometry.hasActiveOperation())
            return true;
        if (!m_parameterCalculator)
        {
            if (errorMessage)
                *errorMessage = tr("直接建模参数计算器未初始化");
            return false;
        }

        const QString normalizedScriptContextId = scriptContextId.trimmed();
        if (!normalizedScriptContextId.isEmpty())
        {
            // 脚本已有参数更新会暂时安装一个候选计算器。为保证失败时能够完整
            // 回滚，任何 GeoDataModelSet 被替换前先确认本轮至多改变一个部件。
            // 跨部件公式若同时驱动多个 Shape，需要数据层级的多部件事务支持；当前
            // original-ID 脚本明确拒绝，避免前一组已提交、后一组失败的半完成状态。
            QSet<int> changedPartIds;
            for (const std::unique_ptr<DrivenOperation>& operation
                : m_drivenOperations)
            {
                if (!operation || operation->historyRetired
                    || operation->viewportModel != currentViewportModel()
                    || (operation->geoData && operation->geoData != currentGeoData())
                    || (operation->tracksGeoDataOwner && operation->geoDataOwner.isNull()))
                {
                    continue;
                }

                double value = 0.0;
                QString expressionError;
                QString resolvedParameterName;
                if (!m_parameterCalculator->parameterValue(
                    operation->parameterId,
                    value,
                    &resolvedParameterName,
                    &expressionError))
                {
                    if (!resolvedParameterName.isEmpty())
                        operation->parameterName = resolvedParameterName;
                    if (errorMessage)
                    {
                        *errorMessage = tr("参数 %1 计算失败：%2")
                            .arg(operation->parameterName, expressionError);
                    }
                    return false;
                }
                operation->parameterName = resolvedParameterName;
                if (false //【GUI 门控】RotateFace 依赖未推送
#if MXGUI_HAS_DIRECTMODELING_V2
                    || (operation->operation
                    == MGeoCommand::DirectModelingOperation::RotateFace
                    && std::abs(value) > 90.0)
#endif // MXGUI_HAS_DIRECTMODELING_V2
                    )
                {
                    if (errorMessage)
                        *errorMessage = tr("参数 %1 的旋转结果超出 [-90°, 90°]")
                        .arg(operation->parameterName);
                    return false;
                }
                if (std::abs(value) <= 1.0e-12
                    && operation->operation
                    != MGeoCommand::DirectModelingOperation::ReplaceFace)
                {
                    if (errorMessage)
                        *errorMessage = tr("参数 %1 的结果为 0，无法生成有效几何")
                        .arg(operation->parameterName);
                    return false;
                }
                if (std::abs(value - operation->currentValue) > 1.0e-12)
                    changedPartIds.insert(operation->partId);
            }
            if (changedPartIds.size() > 1)
            {
                if (errorMessage)
                {
                    *errorMessage = tr(
                        "original-ID 脚本的一次参数更新不能同时重建多个部件");
                }
                return false;
            }
        }

        m_parameterCalculator->clearGeometryValidation();
        markRetiredDrivenParametersInvalid();
        bool allSucceeded = true;
        std::vector<bool> processed(m_drivenOperations.size(), false);

        for (std::size_t firstIndex = 0;
            firstIndex < m_drivenOperations.size(); ++firstIndex)
        {
            if (processed[firstIndex] || !m_drivenOperations[firstIndex]
                || m_drivenOperations[firstIndex]->historyRetired)
                continue;

            DrivenOperation* first = m_drivenOperations[firstIndex].get();
            if (first->viewportModel != currentViewportModel()
                || (first->geoData && first->geoData != currentGeoData())
                || (first->tracksGeoDataOwner && first->geoDataOwner.isNull()))
                continue;

            std::vector<DrivenOperation*> group;
            for (std::size_t index = firstIndex;
                index < m_drivenOperations.size(); ++index)
            {
                DrivenOperation* candidate = m_drivenOperations[index].get();
                if (candidate && !candidate->historyRetired
                    && candidate->partId == first->partId
                    && candidate->viewportModel == first->viewportModel
                    && (!candidate->geoData || candidate->geoData == currentGeoData())
                    && (!candidate->tracksGeoDataOwner || !candidate->geoDataOwner.isNull()))
                {
                    processed[index] = true;
                    group.push_back(candidate);
                }
            }
            if (group.empty())
                continue;

            auto invalidateGroup = [&](const QString& reason) {
                allSucceeded = false;
                for (DrivenOperation* operation : group)
                {
                    m_parameterCalculator->setGeometryInvalid(
                        operation->parameterId,
                        tr("几何失效：%1").arg(reason));
                }
                if (errorMessage && errorMessage->isEmpty())
                    *errorMessage = reason;
                };

            // 只有本脚本已经快照过的部件才需要同步初始 ID 链。参数公式可能同时
            // 回放其他部件；对那些部件保持原有 updatePart 行为，避免扩大影响范围。
            bool trackScriptTopology = false;
            TopoDS_Shape scriptInitialPartShape;
            if (!normalizedScriptContextId.isEmpty())
            {
                for (const std::unique_ptr<ScriptInitialTopologyIdContext>& context
                    : m_scriptInitialTopologyIdContexts)
                {
                    if (context && context->contextId == normalizedScriptContextId)
                    {
                        const auto trackedPartIt = context->parts.constFind(
                            group.front()->partId);
                        trackScriptTopology = trackedPartIt != context->parts.constEnd();
                        if (trackScriptTopology)
                            scriptInitialPartShape = trackedPartIt->initialPartShape;
                        break;
                    }
                }
            }

            std::vector<double> values;
            values.reserve(group.size());
            bool needsRebuild = false;
            QString groupError;
            for (DrivenOperation* operation : group)
            {
                double value = 0.0;
                QString expressionError;
                QString resolvedParameterName;
                if (!m_parameterCalculator->parameterValue(
                    operation->parameterId,
                    value,
                    &resolvedParameterName,
                    &expressionError))
                {
                    if (!resolvedParameterName.isEmpty())
                        operation->parameterName = resolvedParameterName;
                    groupError = tr("参数 %1 计算失败：%2")
                        .arg(operation->parameterName, expressionError);
                    break;
                }
                operation->parameterName = resolvedParameterName;
                if (operation->operation
                    == MGeoCommand::DirectModelingOperation::RotateFace
                    && std::abs(value) > 90.0)
                {
                    groupError = tr("参数 %1 的旋转结果超出 [-90°, 90°]")
                        .arg(operation->parameterName);
                    break;
                }
                if (std::abs(value) <= 1.0e-12
                    && operation->operation
                    != MGeoCommand::DirectModelingOperation::ReplaceFace)
                {
                    groupError = tr("参数 %1 的结果为 0，无法生成有效几何")
                        .arg(operation->parameterName);
                    break;
                }
                values.push_back(value);
                needsRebuild = needsRebuild
                    || std::abs(value - operation->currentValue) > 1.0e-12;
            }
            if (!groupError.isEmpty())
            {
                invalidateGroup(groupError);
                continue;
            }
            if (!needsRebuild)
                continue;

            // 没有数值变化的组不会参与本轮事务，也不应因一份与本轮无关的旧历史
            // 缺失而把已经提交的唯一目标部件标记为失败。仅对确实需要重建的组检查
            // 当前 PartSet 和回放起点。
            MGeoData::GeoDataModelSet* currentPartSet = nullptr;
            for (DrivenOperation* operation : group)
            {
                if (operation->currentPartSet)
                    currentPartSet = operation->currentPartSet;
            }
            if (!currentPartSet || group.front()->sourcePartShape.IsNull())
            {
                invalidateGroup(tr("参数历史缺少当前部件或初始 Shape"));
                continue;
            }

            // 普通参数编辑从该部件第一条活动历史开始回放。恢复断链时，之前的节点已
            // 退役且不进入 group，恢复节点的 sourcePartShape 就是保留旧效果的新基准。
            // original 脚本可能在
            // Panel 已经创建过参数后才启动；此时 context 的初始 Shape 位于历史中段。
            // 从第一个 sourcePartShape 与该快照相同的节点开始，才能让合成 History
            // 的输入域严格等于脚本 initialShape，而不是错误地从更早的模型开始。
            std::size_t rebuildStartIndex = 0;
            TopoDS_Shape rebuiltShape = group.front()->sourcePartShape;
            if (trackScriptTopology)
            {
                if (scriptInitialPartShape.IsNull())
                {
                    invalidateGroup(tr("脚本初始拓扑快照为空"));
                    continue;
                }
                bool boundaryFound = false;
                for (std::size_t index = 0; index < group.size(); ++index)
                {
                    if (group[index]->sourcePartShape.IsSame(scriptInitialPartShape))
                    {
                        rebuildStartIndex = index;
                        boundaryFound = true;
                        break;
                    }
                }
                if (!boundaryFound)
                {
                    invalidateGroup(tr(
                        "参数历史中找不到脚本初始 Shape 对应的回放边界"));
                    continue;
                }
                // 快照以前的操作已经烘焙进 scriptInitialPartShape，不能在本次
                // original-ID 回放中改变；否则必须先建立更早期的持久历史。
                bool prefixChanged = false;
                for (std::size_t index = 0; index < rebuildStartIndex; ++index)
                {
                    if (std::abs(values[index] - group[index]->currentValue) > 1.0e-12)
                    {
                        prefixChanged = true;
                        break;
                    }
                }
                if (prefixChanged)
                {
                    invalidateGroup(tr(
                        "脚本初始快照以前的参数发生变化，无法保持初始 ID 语义"));
                    continue;
                }
                rebuiltShape = scriptInitialPartShape;
            }

            // 任何一步失败都不会调用 updatePart，正式模型保持上一次有效结果。
            Handle(BRepTools_History) replayHistory;
            for (std::size_t index = rebuildStartIndex;
                index < group.size(); ++index)
            {
                DrivenOperation* operation = group[index];
                QString locatorError;
                if (operation->selectedLocators.empty())
                {
                    groupError = tr("参数 %1 缺少选择对象定位信息")
                        .arg(operation->parameterName);
                    break;
                }

                std::vector<TopoDS_Shape> selectedShapes;
                selectedShapes.reserve(operation->selectedLocators.size());
                for (const TopologyLocator& locator : operation->selectedLocators)
                {
                    const TopoDS_Shape selectedShape = ResolveTopologyLocator(
                        rebuiltShape, locator, &locatorError);
                    if (selectedShape.IsNull())
                    {
                        groupError = tr("参数 %1 的选择对象失效：%2")
                            .arg(operation->parameterName, locatorError);
                        break;
                    }
                    selectedShapes.push_back(selectedShape);
                }
                if (!groupError.isEmpty())
                    break;

                std::vector<TopoDS_Shape> referenceShapes;
                referenceShapes.reserve(operation->referenceLocators.size());
                for (const TopologyLocator& locator : operation->referenceLocators)
                {
                    const TopoDS_Shape referenceShape = ResolveTopologyLocator(
                        rebuiltShape, locator, &locatorError);
                    if (referenceShape.IsNull())
                    {
                        groupError = tr("参数 %1 的参照对象失效：%2")
                            .arg(operation->parameterName, locatorError);
                        break;
                    }
                    referenceShapes.push_back(referenceShape);
                }
                if (!groupError.isEmpty())
                    break;

                //MGeoCommand::DirectModelingGeometry rebuildGeometry(rebuiltShape); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                QString geometryError;
                //MGeoCommand::DirectModelingResult result; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                if (selectedShapes.size() == 1)
                {
                    if (!rebuildGeometry.beginOperation(
                        selectedShapes.front(),
                        operation->operation,
                        //MGeoCommand::DirectModelingResultMode::Update, //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                        &geometryError))
                    {
                        groupError = tr("参数 %1 无法重新启动：%2")
                            .arg(operation->parameterName, geometryError);
                        break;
                    }
#if MXGUI_HAS_DIRECTMODELING_V2
                    if (!rebuildGeometry.setMoveSolidMode(operation->moveSolid))
                    {
                        groupError = operation->moveSolid
                            ? tr("参数 %1 无法恢复实体移动作用域")
                            .arg(operation->parameterName)
                            : tr("参数 %1 无法恢复普通面操作作用域")
                            .arg(operation->parameterName);
                        break;
                    }
#endif // MXGUI_HAS_DIRECTMODELING_V2

                    //【OCC摘除·S9·直模下线】旧替换面重建链（setReplacementFace）随冻结岛退役
//                    if (!referenceShapes.empty()
//                        && operation->operation
//                        == MGeoCommand::DirectModelingOperation::ReplaceFace)
//                    {
//                        if (referenceShapes.front().ShapeType() != TopAbs_FACE
//                            || !rebuildGeometry.setReplacementFace(
//                                TopoDS::Face(referenceShapes.front())))
//                        {
//                            groupError = tr("参数 %1 的替换目标面已经失效")
//                                .arg(operation->parameterName);
//                            break;
//                        }
//                    }
                    else if (!referenceShapes.empty()
                        && operation->operation
                        == MGeoCommand::DirectModelingOperation::RotateFace)
                    {
                        if (referenceShapes.front().ShapeType() != TopAbs_EDGE)
                        {
                            groupError = tr("参数 %1 的旋转轴边已经失效")
                                .arg(operation->parameterName);
                            break;
                        }
                        BRepAdaptor_Curve curve(TopoDS::Edge(referenceShapes.front()));
                        if (curve.GetType() != GeomAbs_Line)
                        {
                            groupError = tr("参数 %1 的旋转轴不再是直线")
                                .arg(operation->parameterName);
                            break;
                        }
                        const gp_Lin line = curve.Line();
                        const QVector3D sourceCenter =
                            rebuildGeometry.activeSelection().arrowCenter;
                        const gp_Pnt center(
                            sourceCenter.x(), sourceCenter.y(), sourceCenter.z());
                        const gp_Vec axisVector(line.Direction());
                        const gp_Vec locationToCenter(line.Location(), center);
                        const gp_Pnt axisCenter = line.Location().Translated(
                            axisVector.Multiplied(locationToCenter.Dot(axisVector)));
                        if (!rebuildGeometry.setRotationAxis(
                            gp_Ax1(axisCenter, line.Direction())))
                        {
                            groupError = tr("参数 %1 的旋转轴不再位于选择面内")
                                .arg(operation->parameterName);
                            break;
                        }
                    }

                    if (!rebuildGeometry.finish(values[index], result, &geometryError)
                        || result.shape.IsNull())
                    {
                        groupError = tr("参数 %1 几何重建失败：%2")
                            .arg(operation->parameterName, geometryError);
                        break;
                    }
                }
                else
                {
                    //std::vector<MGeoCommand::DirectModelingRequest> requests; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                    requests.reserve(selectedShapes.size());
                    gp_Ax1 rotationAxis;
                    if (operation->operation
                        == MGeoCommand::DirectModelingOperation::RotateFace)
                    {
                        if (referenceShapes.size() != 1
                            || referenceShapes.front().ShapeType() != TopAbs_EDGE)
                        {
                            groupError = tr("参数 %1 的批量旋转缺少公共轴边")
                                .arg(operation->parameterName);
                            break;
                        }
                        BRepAdaptor_Curve curve(TopoDS::Edge(referenceShapes.front()));
                        if (curve.GetType() != GeomAbs_Line)
                        {
                            groupError = tr("参数 %1 的公共旋转轴不再是直线")
                                .arg(operation->parameterName);
                            break;
                        }
                        rotationAxis = curve.Line().Position();
                    }

                    for (std::size_t selectedIndex = 0;
                        selectedIndex < selectedShapes.size(); ++selectedIndex)
                    {
                        //MGeoCommand::DirectModelingRequest request; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                        request.operation = operation->operation;
                        request.selectedShape = selectedShapes[selectedIndex];
                        request.value = values[index];
                        //request.mode = MGeoCommand::DirectModelingResultMode::Update; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
                        request.simplifyResultOnCommit =
                            operation->operation
                            != MGeoCommand::DirectModelingOperation::ResizeFillet
                            && operation->operation
                            != MGeoCommand::DirectModelingOperation::EdgeFillet;
                        if (operation->operation
                            == MGeoCommand::DirectModelingOperation::ReplaceFace)
                        {
                            if (referenceShapes.empty()
                                || (referenceShapes.size() != 1
                                    && referenceShapes.size() != selectedShapes.size()))
                            {
                                groupError = tr("参数 %1 的替换面配对已经失效")
                                    .arg(operation->parameterName);
                                break;
                            }
                            const std::size_t referenceIndex =
                                referenceShapes.size() == 1 ? 0 : selectedIndex;
                            if (referenceShapes[referenceIndex].ShapeType() != TopAbs_FACE)
                            {
                                groupError = tr("参数 %1 的替换目标不再是面")
                                    .arg(operation->parameterName);
                                break;
                            }
                            request.replacementFace = TopoDS::Face(
                                referenceShapes[referenceIndex]);
                        }
                        else if (operation->operation
                            == MGeoCommand::DirectModelingOperation::RotateFace)
                        {
                            request.rotationAxisPoint = rotationAxis.Location();
                            request.rotationAxisDirection = rotationAxis.Direction();
                            request.hasRotationAxis = true;
                        }
                        requests.push_back(request);
                    }
                    if (!groupError.isEmpty())
                        break;

                    if (!rebuildGeometry.generateBatchResult(
                        requests, result, &geometryError)
                        || result.shape.IsNull())
                    {
                        groupError = tr("参数 %1 批量几何重建失败：%2")
                            .arg(operation->parameterName, geometryError);
                        break;
                    }
                }
                if (trackScriptTopology)
                {
                    if (result.history.IsNull())
                    {
                        groupError = tr(
                            "参数 %1 没有返回 OCC History，无法维护脚本初始 ID")
                            .arg(operation->parameterName);
                        break;
                    }
                    if (replayHistory.IsNull())
                        replayHistory = result.history;
                    else
                        replayHistory->Merge(result.history);
                }
                rebuiltShape = result.shape;
            }

            if (!groupError.isEmpty())
            {
                invalidateGroup(groupError);
                continue;
            }

            MGeoData::GeoDataModelSet* updatedSet = nullptr;
            if (!updatePart(
                currentPartSet,
                rebuiltShape,
                &updatedSet,
                Handle(BRepTools_History)(),
                trackScriptTopology ? normalizedScriptContextId : QString(),
                replayHistory)
                || !updatedSet)
            {
                invalidateGroup(tr("参数历史结果无法写入模型"));
                continue;
            }
            retargetDrivenOperations(
                group.front()->partId, group.front()->viewportModel, updatedSet);
            for (std::size_t index = 0; index < group.size(); ++index)
                group[index]->currentValue = values[index];
        }
        return allSucceeded;
    #endif
}

    bool DirectModelingController::updatePart(
        MGeoData::GeoDataModelSet* oldSet,
        const TopoDS_Shape& newShape,
        MGeoData::GeoDataModelSet** updatedSet,
        Handle(BRepTools_History) history,
        const QString& scriptContextId,
        Handle(BRepTools_History) scriptTrackingHistory)
{
    //【OCC摘除·S9·直模下线】旧栈 OCC 链随冻结岛退役，函数体保壳；
    // 直模正式承接为引擎编辑族（OCCAdapter executeEdit*），接入后恢复。
    return false;
//【S9 原体（逐行注释保留）】
//        MGeoData::GeoDataModelSet* oldSet,
//        const TopoDS_Shape& newShape,
//        MGeoData::GeoDataModelSet** updatedSet,
//        Handle(BRepTools_History) history,
//        const QString& scriptContextId,
//        Handle(BRepTools_History) scriptTrackingHistory)
//    {
//        if (updatedSet)
//            *updatedSet = nullptr;
//        if (!oldSet || newShape.IsNull())
//        {
//            outputMessage(2, tr("直接建模结果为空，更新部件失败"));
//            return false;
//        }

//        MGeoData::GeoDataBase* geoData = currentGeoData();
//        if (!geoData)
//        {
//            outputMessage(2, tr("无法获取几何数据，更新部件失败"));
//            return false;
//        }

//        // 不在 oldSet 上就地改 Shape：创建编辑结果集、复制编辑参数、加入数据库后
//        // 再移除旧集，使数据层撤销/刷新流程与其他几何编辑功能保持一致。
//        const int oldPartId = oldSet->getPartID();
//        const QString normalizedScriptContextId = scriptContextId.trimmed();
//        const Handle(BRepTools_History) trackingHistory =
//            scriptTrackingHistory.IsNull() ? history : scriptTrackingHistory;
//        Handle(BRepTools_History) stableScriptDataHistory;
//        if (!normalizedScriptContextId.isEmpty())
//        {
//            QString trackingError;
//            if (!advanceScriptInitialTopologyIds(
//                normalizedScriptContextId,
//                oldSet,
//                newShape,
//                nullptr,
//                trackingHistory,
//                false,
//                &trackingError,
//                &stableScriptDataHistory))
//            {
//                outputMessage(2, tr("直接建模初始 ID History 预检失败：%1")
//                    .arg(trackingError));
//                return false;
//            }
//            if (stableScriptDataHistory.IsNull())
//            {
//                outputMessage(2, tr(
//                    "直接建模初始 ID History 预检没有生成数据层稳定历史"));
//                return false;
//            }
//        }

//        MGeoData::GeoDataModelSet* newSet = new MGeoData::GeoDataModelSet(geoData);
//        // 普通 Panel/普通 API 继续使用几何算法的原始 History，行为保持不变。
//        // original-ID 脚本改用预检产生的 current oldSet -> newShape 严格双射历史：
//        // 即使参数回放的 trackingHistory 是 initial -> final，也不会把错误输入域传入
//        // GeoDataModelSet。两份 History 使用不同 Handle，数据层消费不会污染脚本映射链。
//        const Handle(BRepTools_History) dataHistory =
//            normalizedScriptContextId.isEmpty() ? history : stableScriptDataHistory;
//        newSet->updateEditPara(oldSet, newShape, dataHistory);

//        // 脚本原始-ID模式额外维护一条与数据层解耦的初始拓扑链。普通提交的
//        // history 是 oldSet -> newSet；参数整链回放会通过 scriptTrackingHistory
//        // 单独传入 initialShape -> newSet，避免把语义不同的历史交给数据层。
//        if (!normalizedScriptContextId.isEmpty())
//        {
//            QString trackingError;
//            if (!advanceScriptInitialTopologyIds(
//                normalizedScriptContextId,
//                oldSet,
//                newShape,
//                newSet,
//                trackingHistory,
//                true,
//                &trackingError))
//            {
//                outputMessage(2, tr("直接建模初始 ID 映射失败：%1")
//                    .arg(trackingError));
//                delete newSet;
//                return false;
//            }
//        }

//        // 一个 Controller 理论上只会串行执行一份 JSON 脚本，但这里仍不能让其他
//        // context 保留即将被 removeSet() 删除的裸指针。例如脚本运行期间用户通过
//        // 面板或参数表改了同一部件，该修改没有对应脚本 History，旧映射必须明确
//        // 失效。当前提交所属 context 已在上面把 currentSet 原子推进到了 newSet，
//        // 因此不会命中 oldSet；其余命中的 context 只标记失败，不再允许后续解引用。
//        for (const std::unique_ptr<ScriptInitialTopologyIdContext>& context
//            : m_scriptInitialTopologyIdContexts)
//        {
//            if (!context || !context->valid)
//                continue;
//            for (auto partIt = context->parts.constBegin();
//                partIt != context->parts.constEnd(); ++partIt)
//            {
//                if (partIt->currentSet == oldSet)
//                {
//                    context->valid = false;
//                    context->failureReason = tr(
//                        "部件 %1 在脚本 History 链之外被修改，初始几何 ID 映射已失效")
//                        .arg(oldPartId);
//                    break;
//                }
//            }
//        }
//        geoData->appendEditSet(newSet);
//        // 刷新信号可能使用 DirectConnection 立即重入 Controller，因此必须在删除旧集、
//        // 发出任何信号之前重定向全部非拥有指针，不能让槽函数看到悬空 oldSet。
//        retargetDrivenOperations(oldPartId, currentViewportModel(), newSet);
//        if (m_currentPartSet == oldSet)
//            m_currentPartSet = newSet;
//        geoData->removeSet(oldSet);
//        if (updatedSet)
//            *updatedSet = newSet;

//        // 两个信号分别触发几何缓存/拓扑 ID 重建和模型视图刷新；提交成功后 Panel
//        // 才清除全部高亮，避免失败时丢失用户现场。
//        MechModelData::mModelData* modelData = currentModelData();
//        if (modelData)
//        {
//            int modelDataID = modelData->getModelDataID();
//            MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
//            MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
//        }

//        outputMessage(0, tr("直接建模完成"));
//        return true;
//    }
}
    // ------------------------------------------------------------------
    // 辅助函数
    // ------------------------------------------------------------------

    MechPreData::mViewportModel* DirectModelingController::currentViewportModel() const
    {
        return getCVM;
    }

    MechModelData::mModelData* DirectModelingController::currentModelData() const
    {
        return getCVM->getModelData();
    }

    MGeoData::GeoDataBase* DirectModelingController::currentGeoData() const
    {
        return getCVM->getModelData()->getGeoData();
    }

    void DirectModelingController::outputMessage(int type, const QString& msg, bool statusbar) const
    {
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(type, msg, statusbar);
    }
}
#endif // ===================== 原 WIP 实现结束 =====================

//【OCC摘除·S10/P0-1 终局】中性壳实现：签名与头一致，语义=恒无活动会话。
namespace MechMainWindow
{
    namespace
    {
        //【直模引擎化·S11·GUI 预览源】GN 部件离散网格 → DirectTool 渲染源
        //（后端无关，OCC/CGM 同构；旧 OCC 源面采样链已随 S9 退役）：
        //   facePoints = 选中面三角形顶点（或偏置模式 wholePart 下的全部件
        //   面三角形），edgePoints = 面三角 soup 的单邻接边（边界线段）或
        //   wholePart 下的全部件离散边折线段；
        //   箭头 = 选中面面积加权质心 + 面积加权法向（箭头始终锚在被拾面上）。
        struct NeutralPreviewSource
        {
            std::vector<glm::vec3> facePoints;   // 三角形顶点表（数量为 3 的倍数）
            std::vector<glm::vec3> edgePoints;   // 线段端点表（数量为 2 的倍数）
            QVector3D center;
            QVector3D direction;
            bool valid = false;
        };

        NeutralPreviewSource neutralPreviewSource(int faceId, bool wholePart)
        {
            NeutralPreviewSource out;
            do {
                MGeoManager::GeoManager* manager =
                    (getCVM != nullptr && getCVM->getModelData() != nullptr)
                    ? getCVM->getModelData()->getGeoManager()
                    : nullptr;
                if (manager == nullptr)
                {
                    break;
                }
                const int partId = MGeoNeutralQuery::partIdByGeoId(
                    manager, 2, faceId);
                GeoNeutralData::mGeoPartManager* partManager =
                    partId >= 0 ? MGeoNeutralQuery::partMgr(manager) : nullptr;
                const GeoNeutralData::GN_Part* part =
                    partManager != nullptr ? partManager->getPartPtrById(partId)
                    : nullptr;
                if (part == nullptr)
                {
                    break;
                }
                const GeoNeutralData::GN_TessellationMesh& mesh =
                    part->getGeoDiscreteData();
                const auto selectedFace = mesh.facePoints.find(faceId);
                if (selectedFace == mesh.facePoints.end()
                        || selectedFace->second.size() % 3 != 0)
                {
                    break;
                }

                auto nodeAt = [&mesh](int index) -> const GeoNeutralData::GN_Point* {
                    const auto found = mesh.globalPoints.find(index);
                    return found == mesh.globalPoints.end() ? nullptr : &found->second;
                };

                // 选中面三角形：输出顶点表，同时按 Newell 法累积面积/质心/法向。
                double normal[3] = { 0.0, 0.0, 0.0 };
                double centroid[3] = { 0.0, 0.0, 0.0 };
                double totalArea = 0.0;
                const std::vector<int>& triangles = selectedFace->second;
                for (size_t triangle = 0; triangle + 2 < triangles.size();
                    triangle += 3)
                {
                    const GeoNeutralData::GN_Point* a =
                        nodeAt(triangles[triangle]);
                    const GeoNeutralData::GN_Point* b =
                        nodeAt(triangles[triangle + 1]);
                    const GeoNeutralData::GN_Point* c =
                        nodeAt(triangles[triangle + 2]);
                    if (a == nullptr || b == nullptr || c == nullptr)
                    {
                        break;
                    }
                    const double pa[3] = { a->x(), a->y(), a->z() };
                    const double pb[3] = { b->x(), b->y(), b->z() };
                    const double pc[3] = { c->x(), c->y(), c->z() };
                    out.facePoints.push_back({ (float)pa[0], (float)pa[1], (float)pa[2] });
                    out.facePoints.push_back({ (float)pb[0], (float)pb[1], (float)pb[2] });
                    out.facePoints.push_back({ (float)pc[0], (float)pc[1], (float)pc[2] });
                    // Newell：n += pa×pb + pb×pc + pc×pa；面积 = |n|/2；
                    // 质心权重 = 三角形面积。
                    const double cross[3] = {
                        (pb[1] - pa[1]) * (pc[2] - pa[2])
                            - (pb[2] - pa[2]) * (pc[1] - pa[1]),
                        (pb[2] - pa[2]) * (pc[0] - pa[0])
                            - (pb[0] - pa[0]) * (pc[2] - pa[2]),
                        (pb[0] - pa[0]) * (pc[1] - pa[1])
                            - (pb[1] - pa[1]) * (pc[0] - pa[0])
                    };
                    normal[0] += cross[0];
                    normal[1] += cross[1];
                    normal[2] += cross[2];
                    const double area = 0.5 * std::sqrt(
                        cross[0] * cross[0] + cross[1] * cross[1]
                        + cross[2] * cross[2]);
                    centroid[0] += area * (pa[0] + pb[0] + pc[0]) / 3.0;
                    centroid[1] += area * (pa[1] + pb[1] + pc[1]) / 3.0;
                    centroid[2] += area * (pa[2] + pb[2] + pc[2]) / 3.0;
                    totalArea += area;
                }
                if (!(totalArea > 1.0e-12))
                {
                    break;
                }
                const double normalLength = std::sqrt(
                    normal[0] * normal[0] + normal[1] * normal[1]
                    + normal[2] * normal[2]);
                if (!(normalLength > 1.0e-12))
                {
                    break;
                }
                out.center = QVector3D(
                    (float)(centroid[0] / totalArea),
                    (float)(centroid[1] / totalArea),
                    (float)(centroid[2] / totalArea));
                out.direction = QVector3D(
                    (float)(normal[0] / normalLength),
                    (float)(normal[1] / normalLength),
                    (float)(normal[2] / normalLength));

                if (wholePart)
                {
                    // 偏置模式（移动实体）：预览源 = 全部件面三角形 + 全部件
                    // 离散边折线段（箭头仍锚在被拾面上）。
                    for (const auto& faceEntry : mesh.facePoints)
                    {
                        if (faceEntry.first == faceId
                                || faceEntry.second.size() % 3 != 0)
                        {
                            continue;
                        }
                        for (int pointId : faceEntry.second)
                        {
                            const GeoNeutralData::GN_Point* point = nodeAt(pointId);
                            if (point != nullptr)
                            {
                                out.facePoints.push_back(
                                    { (float)point->x(), (float)point->y(),
                                      (float)point->z() });
                            }
                        }
                    }
                    for (const auto& edgeEntry : mesh.edgePoints)
                    {
                        const std::vector<int>& polyline = edgeEntry.second;
                        for (size_t index = 1; index < polyline.size(); ++index)
                        {
                            const GeoNeutralData::GN_Point* from =
                                nodeAt(polyline[index - 1]);
                            const GeoNeutralData::GN_Point* to =
                                nodeAt(polyline[index]);
                            if (from != nullptr && to != nullptr)
                            {
                                out.edgePoints.push_back(
                                    { (float)from->x(), (float)from->y(),
                                      (float)from->z() });
                                out.edgePoints.push_back(
                                    { (float)to->x(), (float)to->y(),
                                      (float)to->z() });
                            }
                        }
                    }
                }
                else
                {
                    // 单面模式：边界线段 = 面三角 soup 中只被一个三角形使用的边。
                    // 网格全局点 ID 不保证跨三角形焊接（同一位置可持有不同 ID），
                    // 按 ID 对统计会把四边形对角线误判为边界（预览出 X 交叉线）；
                    // 改按量化坐标焊接计数。
                    using QKey = std::array<long long, 3>;
                    auto quantize = [](double value) -> long long {
                        return static_cast<long long>(
                            std::llround(value * 10000.0));    // 1e-4 mm 分辨率
                    };
                    auto keyOf = [&quantize](const GeoNeutralData::GN_Point* point) {
                        return QKey{ quantize(point->x()), quantize(point->y()),
                            quantize(point->z()) };
                    };
                    struct BoundaryEdge
                    {
                        const GeoNeutralData::GN_Point* from = nullptr;
                        const GeoNeutralData::GN_Point* to = nullptr;
                    };
                    std::map<std::pair<QKey, QKey>, int> edgeUse;
                    std::map<std::pair<QKey, QKey>, BoundaryEdge> edgeNodes;
                    auto appendEdgeUse = [&](int fromId, int toId) {
                        const GeoNeutralData::GN_Point* from = nodeAt(fromId);
                        const GeoNeutralData::GN_Point* to = nodeAt(toId);
                        if (from == nullptr || to == nullptr)
                        {
                            return;
                        }
                        const QKey fromKey = keyOf(from);
                        const QKey toKey = keyOf(to);
                        if (fromKey == toKey)
                        {
                            return;
                        }
                        const std::pair<QKey, QKey> key =
                            fromKey < toKey
                            ? std::make_pair(fromKey, toKey)
                            : std::make_pair(toKey, fromKey);
                        if (++edgeUse[key] == 1)
                        {
                            edgeNodes[key] = { from, to };
                        }
                    };
                    for (size_t triangle = 0; triangle + 2 < triangles.size();
                        triangle += 3)
                    {
                        appendEdgeUse(triangles[triangle], triangles[triangle + 1]);
                        appendEdgeUse(triangles[triangle + 1], triangles[triangle + 2]);
                        appendEdgeUse(triangles[triangle + 2], triangles[triangle]);
                    }
                    for (const auto& edge : edgeUse)
                    {
                        if (edge.second != 1)
                        {
                            continue;
                        }
                        const BoundaryEdge& nodes = edgeNodes[edge.first];
                        out.edgePoints.push_back(
                            { (float)nodes.from->x(), (float)nodes.from->y(),
                              (float)nodes.from->z() });
                        out.edgePoints.push_back(
                            { (float)nodes.to->x(), (float)nodes.to->y(),
                              (float)nodes.to->z() });
                    }
                    //【X线取证·临时】拾取面数据快照：判定预览 X/斜线归属。
                    // 焊接边界若含对角线段 → 网格三角化未按位置焊接（几何数据
                    // 问题）；边界干净仍现斜线 → 渲染侧问题。排查完成后删除。
                    {
                        QString segmentsLog;
                        for (std::size_t s = 0; s + 1 < out.edgePoints.size();
                            s += 2)
                        {
                            if (s > 0) segmentsLog += QStringLiteral(", ");
                            segmentsLog += QStringLiteral(
                                "(%1,%2,%3)-(%4,%5,%6)")
                                .arg(out.edgePoints[s].x, 0, 'f', 2)
                                .arg(out.edgePoints[s].y, 0, 'f', 2)
                                .arg(out.edgePoints[s].z, 0, 'f', 2)
                                .arg(out.edgePoints[s + 1].x, 0, 'f', 2)
                                .arg(out.edgePoints[s + 1].y, 0, 'f', 2)
                                .arg(out.edgePoints[s + 1].z, 0, 'f', 2);
                        }
                        std::vector<int> topoEdges;
                        for (const auto& kid : part->getChildren(
                                 GeoNeutralData::GN_ObjectId(
                                     GeoNeutralData::GN_ShapeType::Face,
                                     faceId)))
                        {
                            if (kid.type == GeoNeutralData::GN_ShapeType::Edge)
                            {
                                topoEdges.push_back(kid.id);
                            }
                            else if (kid.type
                                == GeoNeutralData::GN_ShapeType::Wire)
                            {
                                for (const auto& ekid : part->getChildren(kid))
                                {
                                    if (ekid.type
                                        == GeoNeutralData::GN_ShapeType::Edge)
                                    {
                                        topoEdges.push_back(ekid.id);
                                    }
                                }
                            }
                        }
                        QString edgesLog;
                        for (int eid : topoEdges)
                        {
                            const auto entry = mesh.edgePoints.find(eid);
                            edgesLog += QStringLiteral(" e%1:%2pts")
                                .arg(eid)
                                .arg(entry == mesh.edgePoints.end()
                                    ? -1 : (int)entry->second.size());
                        }
                        qWarning() << "[DM-Forensic] face" << faceId
                            << "part" << partId
                            << "tris" << (triangles.size() / 3)
                            << "weldedBoundarySegs"
                            << (out.edgePoints.size() / 2) << edgesLog
                            << "\n  segments:" << segmentsLog;
                        //【X线取证·跨部件 ID 冲突】GN 拓扑 ID 按部件独立编号，
                        // GeoSketchCalc::edgeDiscretePoints/faceDiscretePoints
                        // 跨部件遍历首个命中即返回；其他部件存在同号边/面时
                        // DirectTool 会拿到别的部件的离散点（墙底错位=楔形斜线）。
                        QString partsLog;
                        QString collisionLog;
                        for (int pid : partManager->getPartList())
                        {
                            partsLog += QStringLiteral(" %1").arg(pid);
                            if (pid == partId)
                            {
                                continue;
                            }
                            const GeoNeutralData::GN_Part* other =
                                partManager->getPartPtrById(pid);
                            if (other == nullptr
                                || other->dataManager() == nullptr)
                            {
                                continue;
                            }
                            const GeoNeutralData::GN_TessellationMesh&
                                otherMesh = other->getGeoDiscreteData();
                            QString hits;
                            if (otherMesh.facePoints.find(faceId)
                                != otherMesh.facePoints.end())
                            {
                                hits += QStringLiteral(" f%1").arg(faceId);
                            }
                            for (int eid : topoEdges)
                            {
                                if (otherMesh.edgePoints.find(eid)
                                    != otherMesh.edgePoints.end())
                                {
                                    hits += QStringLiteral(" e%1").arg(eid);
                                }
                            }
                            if (!hits.isEmpty())
                            {
                                collisionLog += QStringLiteral(
                                    "  [COLLISION] part %1 also has:%2\n")
                                    .arg(pid).arg(hits);
                            }
                        }
                        qWarning() << "[DM-Forensic] parts(all):" << partsLog;
                        if (!collisionLog.isEmpty())
                        {
                            qWarning() << "[DM-Forensic] cross-part ID collisions:\n"
                                << collisionLog;
                        }
                    }
                }
                out.valid = out.facePoints.size() % 3 == 0
                    && !out.facePoints.empty()
                    && out.edgePoints.size() % 2 == 0
                    && out.direction.lengthSquared() > 1.0e-14f;
            } while (false);
            return out;
        }

        //【S11·旋转面】面中轴线：过面质心、位于面平面内、沿边界较长主方向
        //（旋转面默认轴；拾取直线边时由 rotationAxisFromEdge 覆盖）。
        bool faceMedianAxis(int faceId, QVector3D& axisPoint, QVector3D& axisDirection)
        {
            const NeutralPreviewSource source = neutralPreviewSource(faceId, false);
            if (!source.valid)
            {
                return false;
            }
            const QVector3D normal = source.direction.normalized();
            QVector3D seed(0.0f, 0.0f, 1.0f);
            if (std::abs(QVector3D::dotProduct(normal, seed)) > 0.9f)
            {
                seed = QVector3D(1.0f, 0.0f, 0.0f);
            }
            const QVector3D u = QVector3D::crossProduct(normal, seed).normalized();
            const QVector3D v = QVector3D::crossProduct(normal, u);
            float minU = 1.0e30f, maxU = -1.0e30f, minV = 1.0e30f, maxV = -1.0e30f;
            for (const glm::vec3& p : source.facePoints)
            {
                const QVector3D q(p.x, p.y, p.z);
                const float du = QVector3D::dotProduct(q, u);
                const float dv = QVector3D::dotProduct(q, v);
                minU = std::min(minU, du);
                maxU = std::max(maxU, du);
                minV = std::min(minV, dv);
                maxV = std::max(maxV, dv);
            }
            axisDirection = (maxU - minU >= maxV - minV) ? u : v;
            axisPoint = source.center;
            return axisDirection.lengthSquared() > 1.0e-14f;
        }

        //【S11·旋转面】直线边定轴：GN 边离散点首尾（edgePoints 为链式段对
        // 格式，首点=front、末点=back）。
        bool rotationAxisFromEdge(int edgeId, QVector3D& axisPoint,
            QVector3D& axisDirection)
        {
            MGeoManager::GeoManager* manager =
                (getCVM != nullptr && getCVM->getModelData() != nullptr)
                ? getCVM->getModelData()->getGeoManager()
                : nullptr;
            if (manager == nullptr)
            {
                return false;
            }
            const int partId = MGeoNeutralQuery::partIdByGeoId(manager, 1, edgeId);
            GeoNeutralData::mGeoPartManager* partManager =
                partId >= 0 ? MGeoNeutralQuery::partMgr(manager) : nullptr;
            const GeoNeutralData::GN_Part* part =
                partManager != nullptr ? partManager->getPartPtrById(partId)
                : nullptr;
            if (part == nullptr)
            {
                return false;
            }
            const GeoNeutralData::GN_TessellationMesh& mesh =
                part->getGeoDiscreteData();
            const auto entry = mesh.edgePoints.find(edgeId);
            if (entry == mesh.edgePoints.end() || entry->second.size() < 2)
            {
                return false;
            }
            auto pointAt = [&mesh](int pid) -> const GeoNeutralData::GN_Point* {
                const auto found = mesh.globalPoints.find(pid);
                return found == mesh.globalPoints.end() ? nullptr : &found->second;
            };
            const GeoNeutralData::GN_Point* first = pointAt(entry->second.front());
            const GeoNeutralData::GN_Point* last = pointAt(entry->second.back());
            if (first == nullptr || last == nullptr)
            {
                return false;
            }
            const QVector3D a(first->x(), first->y(), first->z());
            const QVector3D b(last->x(), last->y(), last->z());
            const QVector3D dir = b - a;
            if (dir.lengthSquared() < 1.0e-14f)
            {
                return false;
            }
            axisPoint = a;
            axisDirection = dir.normalized();
            return true;
        }

        //【参数驱动·几何锚点身份】面锚点：离散网格三角形 Newell 质心/法向/面积。
        bool faceAnchorOfPart(const GeoNeutralData::GN_Part* part, int faceId,
            QVector3D& center, QVector3D& normal, double& area)
        {
            if (part == nullptr || faceId <= 0)
            {
                return false;
            }
            const GeoNeutralData::GN_TessellationMesh& mesh =
                part->getGeoDiscreteData();
            const auto entry = mesh.facePoints.find(faceId);
            if (entry == mesh.facePoints.end() || entry->second.size() < 3)
            {
                return false;
            }
            double n[3] = { 0.0, 0.0, 0.0 };
            double c[3] = { 0.0, 0.0, 0.0 };
            double totalArea = 0.0;
            auto pointAt = [&mesh](int pid) -> const GeoNeutralData::GN_Point* {
                const auto found = mesh.globalPoints.find(pid);
                return found == mesh.globalPoints.end() ? nullptr : &found->second;
            };
            const std::vector<int>& tri = entry->second;
            for (size_t t = 0; t + 2 < tri.size(); t += 3)
            {
                const GeoNeutralData::GN_Point* a = pointAt(tri[t]);
                const GeoNeutralData::GN_Point* b = pointAt(tri[t + 1]);
                const GeoNeutralData::GN_Point* cPoint = pointAt(tri[t + 2]);
                if (a == nullptr || b == nullptr || cPoint == nullptr)
                {
                    continue;
                }
                const double pa[3] = { a->x(), a->y(), a->z() };
                const double pb[3] = { b->x(), b->y(), b->z() };
                const double pc[3] = { cPoint->x(), cPoint->y(), cPoint->z() };
                const double cross[3] = {
                    (pb[1] - pa[1]) * (pc[2] - pa[2])
                        - (pb[2] - pa[2]) * (pc[1] - pa[1]),
                    (pb[2] - pa[2]) * (pc[0] - pa[0])
                        - (pb[0] - pa[0]) * (pc[2] - pa[2]),
                    (pb[0] - pa[0]) * (pc[1] - pa[1])
                        - (pb[1] - pa[1]) * (pc[0] - pa[0])
                };
                n[0] += cross[0];
                n[1] += cross[1];
                n[2] += cross[2];
                const double triArea = 0.5 * std::sqrt(
                    cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
                c[0] += triArea * (pa[0] + pb[0] + pc[0]) / 3.0;
                c[1] += triArea * (pa[1] + pb[1] + pc[1]) / 3.0;
                c[2] += triArea * (pa[2] + pb[2] + pc[2]) / 3.0;
                totalArea += triArea;
            }
            const double len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
            if (totalArea <= 1.0e-12 || len <= 1.0e-12)
            {
                return false;
            }
            center = QVector3D(
                (float)(c[0] / totalArea), (float)(c[1] / totalArea),
                (float)(c[2] / totalArea));
            normal = QVector3D(
                (float)(n[0] / len), (float)(n[1] / len), (float)(n[2] / len));
            area = totalArea;
            return true;
        }

        // 编辑后选面：ID 存活且面内偏移小 → 沿用；否则从本次编辑 created 清单
        // 里选"面内位置离编辑前锚点最近"的面（面沿法向移动不改变面内坐标，
        // 多面同时改号时据此区分，避免张冠李戴）。
        int selectFaceAfterEdit(MGeoManager::GeoManager* manager, int partId,
            int previousId, const QVector3D& preCenter, bool hasPre)
        {
            GeoNeutralData::mGeoPartManager* partManager =
                manager != nullptr ? MGeoNeutralQuery::partMgr(manager) : nullptr;
            const GeoNeutralData::GN_Part* part =
                partManager != nullptr ? partManager->getPartPtrById(partId)
                : nullptr;
            if (part == nullptr)
            {
                return -1;
            }
            QVector3D c, n;
            double a;
            if (previousId > 0
                && faceAnchorOfPart(part, previousId, c, n, a) && hasPre)
            {
                const QVector3D delta = c - preCenter;
                const float axial = QVector3D::dotProduct(delta, n);
                const QVector3D inPlane = delta - n * axial;
                if (inPlane.lengthSquared() < 1.0e-4f)
                {
                    return previousId;
                }
            }
            else if (previousId > 0
                && faceAnchorOfPart(part, previousId, c, n, a) && !hasPre)
            {
                return previousId;
            }
            if (!hasPre)
            {
                // 无锚点参照（如边倒圆只知边）：退回 created 首个面。
                const auto& table = part->getGeoDiscreteData().facePoints;
                for (const GeoNeutralData::GN_ObjectId& created : part->createdIds())
                {
                    if (created.type == GeoNeutralData::GN_ShapeType::Face
                        && created.id > 0 && table.find(created.id) != table.end())
                    {
                        return created.id;
                    }
                }
                return -1;
            }
            int bestId = -1;
            float bestInPlane = 1.0e30f;
            for (const GeoNeutralData::GN_ObjectId& created : part->createdIds())
            {
                if (created.type != GeoNeutralData::GN_ShapeType::Face
                    || created.id <= 0)
                {
                    continue;
                }
                if (!faceAnchorOfPart(part, created.id, c, n, a))
                {
                    continue;
                }
                const QVector3D delta = c - preCenter;
                const float axial = QVector3D::dotProduct(delta, n);
                const float inPlane =
                    (delta - n * axial).lengthSquared();
                if (inPlane < bestInPlane)
                {
                    bestInPlane = inPlane;
                    bestId = created.id;
                }
            }
            return bestInPlane < 1.0e-2f ? bestId : -1;
        }

        // 回放前按锚点找回面：编号无关（质心重合 + 法向平行 + 面积相当）。
        int matchFaceByAnchor(MGeoManager::GeoManager* manager, int partId,
            const QVector3D& center, const QVector3D& normal, double area)
        {
            GeoNeutralData::mGeoPartManager* partManager =
                manager != nullptr ? MGeoNeutralQuery::partMgr(manager) : nullptr;
            const GeoNeutralData::GN_Part* part =
                partManager != nullptr ? partManager->getPartPtrById(partId)
                : nullptr;
            if (part == nullptr || area <= 0.0)
            {
                return -1;
            }
            int bestId = -1;
            float bestDist = 1.0e30f;
            for (const auto& face : part->getGeoDiscreteData().facePoints)
            {
                QVector3D c, n;
                double a;
                if (!faceAnchorOfPart(part, face.first, c, n, a))
                {
                    continue;
                }
                const float dist = (c - center).lengthSquared();
                //【容差按面尺寸缩放】引擎布尔会把移动面分裂成共面碎片、后续
                // 编辑又可能合并：面积比放宽到 [0.25, 4]，距离容差 = 0.6×特征
                // 边长（sqrt(面积)），配合法向平行即可在多参数面间精确区分。
                if (dist < bestDist
                    && std::abs(QVector3D::dotProduct(n, normal)) > 0.98f
                    && a >= area * 0.25 && a <= area * 4.0
                    && dist < 0.36f * static_cast<float>(area))
                {
                    bestDist = dist;
                    bestId = face.first;
                }
            }
            return bestId;
        }
    } // namespace

    DirectModelingController* DirectModelingController::getInstance()
    {
        static DirectModelingController instance;
        return &instance;
    }

    DirectModelingController::DirectModelingController(QObject* parent) : QObject(parent)
    {
        //【S11·参数驱动】参数计算器随单例创建（面板参数表/公式求值/回放取值共用）。
        m_parameterCalculator = std::make_unique<mGeoFeatureParameterCalculator>();
    }
    DirectModelingController::~DirectModelingController() = default;

    //【直模引擎化·S11·GUI 复活】直模会话=轻量记录（拾取→确认两段式，无预览线程）。
    void DirectModelingController::enterDirectModelingMode()
    {
        m_isActive = true;
        emit modeChanged(true);
    }

    void DirectModelingController::exitDirectModelingMode()
    {
        resetSession();
        emit arrowHidden();
        m_isActive = false;
        emit modeChanged(false);
    }

    void DirectModelingController::exitDirectModelingMode(bool) { exitDirectModelingMode(); }

    //【直模引擎化·S11】begin 族=记录拾取（dim：0 点 1 边 2 面 3 体）。
    bool DirectModelingController::beginOperation(int elementDim, int elementId, bool)
    {
        return beginOperation(elementDim, elementId,
            elementDim == 1 ? MGeoCommand::DirectModelingOperation::EdgeFillet
                            : MGeoCommand::DirectModelingOperation::MoveFace);
    }

    bool DirectModelingController::beginOperation(int elementDim, int elementId,
        MGeoCommand::DirectModelingOperation operation)
    {
        return beginOperation(elementDim, elementId, operation, false);
    }

    bool DirectModelingController::beginOperation(int elementDim, int elementId,
        MGeoCommand::DirectModelingOperation operation, bool moveSolid)
    {
        if (elementId <= 0
            || operation == MGeoCommand::DirectModelingOperation::None)
        {
            return false;
        }
        resetSession();
        m_selectedElementId = elementId;
        m_selectedElementDim = elementDim;
        m_currentOperation = operation;
        m_moveSolid = moveSolid
            && operation == MGeoCommand::DirectModelingOperation::OffsetFace;
        //【直模引擎化·S11·GUI 预览源】面拾取时从 GN 离散网格解析箭头锚点
        //（面积加权质心 + 法向）；解析失败不阻断拾取（ExactOnly 族无需箭头，
        // 移动/偏置族在 configureCurrentRenderOperation 再取完整源数据）。
        if (elementDim == 2)
        {
            const NeutralPreviewSource source = neutralPreviewSource(
                elementId, false);
            if (source.valid)
            {
                m_arrowCenter = source.center;
                m_arrowDirection = source.direction;
            }
        }
        return true;
    }

    bool DirectModelingController::beginReplaceFaceOperation(int sourceFaceId, int targetFaceId)
    {
        if (!beginOperation(2, sourceFaceId,
                MGeoCommand::DirectModelingOperation::ReplaceFace))
        {
            return false;
        }
        m_engineReferenceId = targetFaceId;
        return true;
    }

    //【直模引擎化·S11】RotateFace 未入引擎枚举（与旧引擎能力一致），保持迁移提示。
    bool DirectModelingController::beginRotateFaceOperation(int sourceFaceId)
    {
        //【S11·旋转面】默认轴 = 面中轴线（过面质心、面平面内、沿边界较长主方向）。
        QVector3D axisPoint, axisDirection;
        if (!faceMedianAxis(sourceFaceId, axisPoint, axisDirection))
        {
            return false;
        }
        if (!beginOperation(2, sourceFaceId,
                MGeoCommand::DirectModelingOperation::RotateFace, false))
        {
            return false;
        }
        m_rotationAxisPoint = axisPoint;
        m_rotationAxisDirection = axisDirection;
        // 旋转圆环锚定在旋转轴上（覆盖 beginOperation 按面法向写入的箭头数据）。
        m_arrowCenter = axisPoint;
        m_arrowDirection = axisDirection;
        return true;
    }

    bool DirectModelingController::beginRotateFaceOperation(int sourceFaceId,
        int axisEdgeId)
    {
        //【S11·旋转面】直线边覆盖旋转轴：轴 = 边离散点首尾连线。
        QVector3D axisPoint, axisDirection;
        if (!rotationAxisFromEdge(axisEdgeId, axisPoint, axisDirection))
        {
            return false;
        }
        //【DraftAngle 契约】引擎（OCC DraftAngle 语义）要求轴位于面平面内：
        // 方向去掉面法向分量，锚点投影到面平面（面质心所在平面）。
        const NeutralPreviewSource source = neutralPreviewSource(sourceFaceId, false);
        if (!source.valid)
        {
            return false;
        }
        const QVector3D normal = source.direction.normalized();
        const QVector3D projected = axisDirection
            - normal * QVector3D::dotProduct(axisDirection, normal);
        if (projected.lengthSquared() < 1.0e-10f)
        {
            return false;    // 边与面法向平行，无法定义面内旋转轴。
        }
        axisDirection = projected.normalized();
        axisPoint = axisPoint
            - normal * QVector3D::dotProduct(axisPoint - source.center, normal);
        if (!beginOperation(2, sourceFaceId,
                MGeoCommand::DirectModelingOperation::RotateFace, false))
        {
            return false;
        }
        m_rotationAxisPoint = axisPoint;
        m_rotationAxisDirection = axisDirection;
        m_arrowCenter = axisPoint;
        m_arrowDirection = axisDirection;
        return true;
    }

    //【直模引擎化·S11】脚本/面板统一提交：映射引擎 directFaceEdit。
    namespace {
        // 控制器中性枚举 → 引擎枚举（成员名一一对应）。
        EventDispatcher::mGeoDirectModelingOperation MapToEngineOperation(
            MGeoCommand::DirectModelingOperation operation)
        {
            using Src = MGeoCommand::DirectModelingOperation;
            using Dst = EventDispatcher::mGeoDirectModelingOperation;
            switch (operation)
            {
            case Src::MoveFace: return Dst::MoveFace;
            case Src::DeleteFace: return Dst::DeleteFace;
            case Src::ReplaceFace: return Dst::ReplaceFace;
            case Src::OffsetFace: return Dst::OffsetFace;
            case Src::ResizeFillet: return Dst::ResizeFillet;
            case Src::EdgeFillet: return Dst::EdgeFillet;
            case Src::RotateFace: return Dst::RotateFace;
            default: return Dst::None;
            }
        }

        QString CommitDirectEdit(
            EventDispatcher::mGeoDirectModelingOperation operation,
            const QVector<int>& primaryFaceIds, int referenceFaceId, int referenceEdgeId,
            double value,
            const QVector3D& axisPoint = QVector3D(),
            const QVector3D& axisDirection = QVector3D(0.0f, 0.0f, 1.0f),
            bool hasAxis = false,
            bool moveSolid = false)
        {
            EventDispatcher::mGeoDirectFaceEditRequest req;
            req.operation = operation;
            req.faceIds = primaryFaceIds;
            req.referenceFaceId = referenceFaceId;
            req.referenceEdgeId = referenceEdgeId;
            req.value = value;
            //【偏置实体】整体平移面所属实体（引擎侧刚体变换），而非单面移动。
            req.moveSolid = moveSolid;
            if (hasAxis)
            {
                //【S11·旋转面】轴 = axisPoint + direction（角度单位：度）。
                req.direction[0] = axisDirection.x();
                req.direction[1] = axisDirection.y();
                req.direction[2] = axisDirection.z();
                req.hasCustomDirection = true;
                req.axisPoint[0] = axisPoint.x();
                req.axisPoint[1] = axisPoint.y();
                req.axisPoint[2] = axisPoint.z();
            }
            req.name = "DirectModel";
            EventDispatcher::mGeoOperationResult res =
                getGEOServer.geoEdit().directFaceEdit(req);
            if (!res.succeeded())
            {
                return QString::fromStdString(res.message);
            }
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
            return QString();
        }
    } // namespace

    bool DirectModelingController::executeCommand(const DirectModelingCommand& command,
        QString* errorMessage)
    {
        const EventDispatcher::mGeoDirectModelingOperation operation =
            MapToEngineOperation(command.operation);
        if (operation == EventDispatcher::mGeoDirectModelingOperation::None)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("direct modeling operation unsupported (RotateFace pending)");
            return false;
        }
        QVector<int> faceIds = command.primaryElementIds;
        if (faceIds.isEmpty() && command.primaryElementId > 0)
        {
            faceIds.append(command.primaryElementId);
        }
        int referenceFaceId = -1;
        int referenceEdgeId = -1;
        if (command.operation == MGeoCommand::DirectModelingOperation::ReplaceFace)
        {
            referenceFaceId = command.referenceElementIds.isEmpty()
                ? command.referenceElementId
                : command.referenceElementIds.first();
        }
        else if (command.operation == MGeoCommand::DirectModelingOperation::EdgeFillet)
        {
            referenceEdgeId = command.referenceElementIds.isEmpty()
                ? command.referenceElementId
                : command.referenceElementIds.first();
            //【圆角修复】Python API 的 ResizeFillet(elementType=edge) 把边 ID 放在
            // primaryElementIds（非 referenceElementId），此处兼容迁移，否则边 ID
            // 恒 -1 → 引擎报"边倒圆需要共享边 ID"。
            if (referenceEdgeId <= 0 && !faceIds.isEmpty())
            {
                referenceEdgeId = faceIds.first();
                faceIds.removeFirst();
            }
        }
        //【S11·旋转面】脚本通道同用会话期记录的旋转轴；无会话（纯脚本调用）
        // 或命令显式带 axisEdgeId 时现算（边定轴优先，否则面中轴线）。
        const bool isRotateFaceCmd = command.operation
            == MGeoCommand::DirectModelingOperation::RotateFace;
        //【断链修复】提交前捕获操作面部件（同 onCommit：提交后 ID 可能改号）。
        // 同时捕获提交前面锚点供编辑后精确选面。
        MGeoManager::GeoManager* commitManagerCmd =
            (getCVM != nullptr && getCVM->getModelData() != nullptr)
            ? getCVM->getModelData()->getGeoManager() : nullptr;
        //【边倒圆修复】EdgeFillet 的边 ID 在 referenceEdgeId 不在 faceIds，
        // 查 partId 时需用边 ID；faceIds 为空且 referenceEdgeId 有效时用它。
        int partIdLookupId = faceIds.isEmpty() ? -1 : faceIds.first();
        if (partIdLookupId < 0 && referenceEdgeId > 0)
        {
            partIdLookupId = referenceEdgeId;
        }
        const int sessionPartIdCmd = MGeoNeutralQuery::partIdByGeoId(
            commitManagerCmd, command.elementDim, partIdLookupId);
        //【参数化基线】首个直模编辑提交前捕获（同 onCommit）。
        if (m_drivenOperations.empty())
        {
            getGEOServer.history().beginParametricBaseline();
        }
        QVector3D preAnchorCenterCmd;
        QVector3D preEditNormalCmd(0.0f, 0.0f, 1.0f);
        double preEditAreaCmd = -1.0;
        bool hasPreAnchorCmd = false;
        if (command.elementDim == 2 && !faceIds.isEmpty())
        {
            GeoNeutralData::mGeoPartManager* prePartMgrCmd =
                MGeoNeutralQuery::partMgr(commitManagerCmd);
            const GeoNeutralData::GN_Part* prePartCmd = prePartMgrCmd
                ? prePartMgrCmd->getPartPtrById(sessionPartIdCmd) : nullptr;
            hasPreAnchorCmd = prePartCmd != nullptr
                && faceAnchorOfPart(prePartCmd, faceIds.first(),
                    preAnchorCenterCmd, preEditNormalCmd, preEditAreaCmd);
        }
        //【基线重放·拓扑定位】提交前记录操作面在部件拓扑表中的序号与表长。
        int sessionLocatorIndexCmd = -1;
        int sessionLocatorCountCmd = -1;
        if (!faceIds.isEmpty() && sessionPartIdCmd >= 0)
        {
            GeoNeutralData::mGeoPartManager* locPartMgrCmd =
                MGeoNeutralQuery::partMgr(commitManagerCmd);
            GeoNeutralData::GN_Part* locPartCmd = locPartMgrCmd
                ? locPartMgrCmd->getPartPtrById(sessionPartIdCmd) : nullptr;
            if (locPartCmd != nullptr && locPartCmd->dataManager() != nullptr)
            {
                const std::vector<int>& topologyCmd =
                    locPartCmd->getTopologyByDimension(command.elementDim);
                sessionLocatorCountCmd = static_cast<int>(topologyCmd.size());
                for (size_t i = 0; i < topologyCmd.size(); ++i)
                {
                    if (topologyCmd[i] == faceIds.first())
                    {
                        sessionLocatorIndexCmd = static_cast<int>(i);
                        break;
                    }
                }
            }
        }
        if (isRotateFaceCmd)
        {
            bool axisReady = m_currentOperation
                == MGeoCommand::DirectModelingOperation::RotateFace
                && m_selectedElementId > 0;
            if (command.referenceElementId > 0)
            {
                axisReady = rotationAxisFromEdge(command.referenceElementId,
                    m_rotationAxisPoint, m_rotationAxisDirection);
            }
            else if (!axisReady && !faceIds.isEmpty())
            {
                axisReady = faceMedianAxis(faceIds.first(),
                    m_rotationAxisPoint, m_rotationAxisDirection);
            }
            if (!axisReady)
            {
                if (errorMessage)
                    *errorMessage = QStringLiteral("旋转面无法确定旋转轴");
                emit commitFinished(false);
                return false;
            }
        }
        const QString error = CommitDirectEdit(operation, faceIds,
            referenceFaceId, referenceEdgeId, command.value,
            m_rotationAxisPoint, m_rotationAxisDirection, isRotateFaceCmd);
        // 提交后拓扑 ID 重编号，旧选择/箭头不可复用；对齐旧栈 executeCommand
        // 成功路径：resetSession + arrowHidden + commitFinished（S11 壳漏发
        // 导致确定后预览/箭头残留）。
        if (error.isEmpty())
        {
            //【S11·参数驱动】脚本/API 提交路径同记参数（无公式，纯数值建参）。
            const bool isRotateCmd = command.operation
                == MGeoCommand::DirectModelingOperation::RotateFace;
            QVector<int> recordFaces = faceIds;
            if (command.operation
                    == MGeoCommand::DirectModelingOperation::EdgeFillet
                && recordFaces.isEmpty() && referenceEdgeId > 0)
            {
                recordFaces = QVector<int>{ referenceEdgeId };
            }
            QVector<int> recordReferences;
            if (referenceFaceId > 0)
                recordReferences.append(referenceFaceId);
            if (isRotateCmd && command.referenceElementId > 0)
                recordReferences.append(command.referenceElementId);
            recordDrivenOperation(command.operation, recordFaces,
                command.operation
                        == MGeoCommand::DirectModelingOperation::EdgeFillet
                    ? 1 : command.elementDim,
                recordReferences, command.value, QString(), false,
                m_rotationAxisPoint, m_rotationAxisDirection, sessionPartIdCmd,
                preAnchorCenterCmd, hasPreAnchorCmd,
                sessionLocatorIndexCmd, sessionLocatorCountCmd,
                preEditNormalCmd, preEditAreaCmd);
        }
        resetSession();
        emit arrowHidden();
        emit commitFinished(error.isEmpty());
        if (!error.isEmpty())
        {
            if (errorMessage)
                *errorMessage = error;
            return false;
        }
        return true;
    }

    void DirectModelingController::clearScriptOriginalFaceIdContext(const QString&) {}

    bool DirectModelingController::resolveScriptOriginalGeometryIds(
        const QString&, int, const QVector<int>&, QVector<int>&, QString*)
    { return false; }

    void DirectModelingController::onDragValue(double) {}
    void DirectModelingController::onDragFinished(double) {}

    //【直模引擎化·S11】面板确认：以 begin 记录的拾取直接提交引擎（无拖拽预览）。
    bool DirectModelingController::onCommit(double distance,
        const QString& drivingExpression, QString* errorMessage, const QString&)
    {
        if (m_currentOperation == MGeoCommand::DirectModelingOperation::None
            || m_selectedElementId <= 0)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("no active direct modeling selection");
            return false;
        }
        QVector<int> faceIds;
        int referenceFaceId = -1;
        int referenceEdgeId = -1;
        if (m_currentOperation == MGeoCommand::DirectModelingOperation::EdgeFillet)
        {
            referenceEdgeId = m_selectedElementId;    // dim=1：共享边
        }
        else
        {
            faceIds.append(m_selectedElementId);     // dim=2：操作面
            if (m_currentOperation == MGeoCommand::DirectModelingOperation::ReplaceFace)
            {
                referenceFaceId = m_engineReferenceId;
            }
        }
        //【S11·旋转面】轴在 beginRotateFaceOperation 会话期记录，随提交下发。
        const bool isRotateFace = m_currentOperation
            == MGeoCommand::DirectModelingOperation::RotateFace;
        //【偏置实体】勾选"偏置"时整体平移实体（对齐旧栈 ApplyOwnerEntityTranslation）。
        const bool isMoveSolidScope = m_moveSolid
            && (m_currentOperation
                == MGeoCommand::DirectModelingOperation::OffsetFace
                || m_currentOperation
                    == MGeoCommand::DirectModelingOperation::MoveFace);
        //【断链修复】提交前捕获操作面部件：编辑可能改号面 ID，提交后反查失效。
        // 同时捕获提交前面锚点（质心），供编辑后按"面内偏移"精确选面。
        MGeoManager::GeoManager* commitManager =
            (getCVM != nullptr && getCVM->getModelData() != nullptr)
            ? getCVM->getModelData()->getGeoManager() : nullptr;
        const int sessionPartId = MGeoNeutralQuery::partIdByGeoId(
            commitManager, m_selectedElementDim, m_selectedElementId);
        //【参数化基线】首个直模编辑提交前捕获基线（未编辑状态），后续参数
        // 变更恢复到此状态再按序重放。必须在 CommitDirectEdit 之前——基线
        // 在编辑后捕获会导致恢复状态与驱动记录的面 ID 不匹配。
        if (m_drivenOperations.empty())
        {
            getGEOServer.history().beginParametricBaseline();
        }
        QVector3D preAnchorCenter;
        QVector3D preEditNormal(0.0f, 0.0f, 1.0f);
        double preEditArea = -1.0;
        bool hasPreAnchor = false;
        if (m_selectedElementDim == 2 && m_selectedElementId > 0)
        {
            GeoNeutralData::mGeoPartManager* prePartMgr =
                MGeoNeutralQuery::partMgr(commitManager);
            const GeoNeutralData::GN_Part* prePart = prePartMgr
                ? prePartMgr->getPartPtrById(sessionPartId) : nullptr;
            hasPreAnchor = prePart != nullptr
                && faceAnchorOfPart(prePart, m_selectedElementId,
                    preAnchorCenter, preEditNormal, preEditArea);
        }
        //【基线重放·拓扑定位】提交前记录操作面在部件拓扑表中的序号与表长。
        int sessionLocatorIndex = -1;
        int sessionLocatorCount = -1;
        if (m_selectedElementId > 0 && sessionPartId >= 0)
        {
            GeoNeutralData::mGeoPartManager* locPartMgr =
                MGeoNeutralQuery::partMgr(commitManager);
            GeoNeutralData::GN_Part* locPart = locPartMgr
                ? locPartMgr->getPartPtrById(sessionPartId) : nullptr;
            if (locPart != nullptr && locPart->dataManager() != nullptr)
            {
                const std::vector<int>& topology =
                    locPart->getTopologyByDimension(m_selectedElementDim);
                sessionLocatorCount = static_cast<int>(topology.size());
                for (size_t i = 0; i < topology.size(); ++i)
                {
                    if (topology[i] == m_selectedElementId)
                    {
                        sessionLocatorIndex = static_cast<int>(i);
                        break;
                    }
                }
            }
        }
        const QString error = CommitDirectEdit(MapToEngineOperation(m_currentOperation),
            faceIds, referenceFaceId, referenceEdgeId, distance,
            m_rotationAxisPoint, m_rotationAxisDirection, isRotateFace,
            isMoveSolidScope);
        if (error.isEmpty())
        {
            //【S11·参数驱动】提交成功建参/更新参数并记录回放节点（必须在
            // resetSession 前——会话对象/引用/轴此刻仍是有效输入）。
            QVector<int> recordReferences;
            if (referenceFaceId > 0)
                recordReferences.append(referenceFaceId);
            if (referenceEdgeId > 0)
                recordReferences.append(referenceEdgeId);
            recordDrivenOperation(m_currentOperation, faceIds,
                m_selectedElementDim, recordReferences, distance,
                drivingExpression, isMoveSolidScope,
                m_rotationAxisPoint, m_rotationAxisDirection, sessionPartId,
                preAnchorCenter, hasPreAnchor,
                sessionLocatorIndex, sessionLocatorCount,
                preEditNormal, preEditArea);
        }
        resetSession();
        // 对齐旧栈提交语义：commitFinished 通知面板统一清预览/箭头/高亮
        //（S11 壳漏发导致确定后预览残留）。
        emit commitFinished(error.isEmpty());
        if (!error.isEmpty())
        {
            if (errorMessage)
                *errorMessage = error;
            return false;
        }
        return true;
    }

    void DirectModelingController::onCancel()
    {
        // resetSession 清预览；arrowHidden 收起 DirectTool 箭头并结束会话。
        resetSession();
        emit arrowHidden();
    }

    void DirectModelingController::validateDrivenParameters()
    {
        //【部件校验】部件被删除/不存在时，对应的参数和驱动节点自动移除。
        // 在参数表刷新和参数回放前调用，确保参数表不残留已失效部件的参数。
        if (m_drivenOperations.empty() || !m_parameterCalculator)
        {
            return;
        }
        MGeoManager::GeoManager* manager =
            (getCVM != nullptr && getCVM->getModelData() != nullptr)
            ? getCVM->getModelData()->getGeoManager() : nullptr;
        GeoNeutralData::mGeoPartManager* partManager =
            manager != nullptr ? MGeoNeutralQuery::partMgr(manager) : nullptr;
        if (partManager == nullptr)
        {
            return;    // 管理器不可用（文档重置等），不清理
        }
        // 先探测是否存在失效节点：无失效时直接返回，绝不能先把节点 move
        // 进局部容器再判空提前返回——move 后 m_drivenOperations 内全部是
        // 空 unique_ptr，下一次 recordDrivenOperation 遍历即空指针崩溃。
        bool hasStaleNode = false;
        for (const auto& driven : m_drivenOperations)
        {
            if (driven == nullptr
                || driven->partId < 0
                || partManager->getPartPtrById(driven->partId) == nullptr)
            {
                hasStaleNode = true;
                break;
            }
        }
        if (!hasStaleNode)
        {
            return;
        }
        QStringList removedNames;
        std::vector<std::unique_ptr<DrivenOperation>> kept;
        kept.reserve(m_drivenOperations.size());
        for (auto& driven : m_drivenOperations)
        {
            const bool partExists = driven != nullptr
                && driven->partId >= 0
                && partManager->getPartPtrById(driven->partId) != nullptr;
            if (!partExists)
            {
                if (driven != nullptr)
                {
                    removedNames << driven->parameterName;
                    m_parameterCalculator->removeParameters(
                        { driven->parameterId });
                }
                continue;
            }
            kept.push_back(std::move(driven));
        }
        m_drivenOperations = std::move(kept);
        // 剩余参数重放（部件删除后基线可能不一致，失败只标失效不动几何）。
        QString rebuildError;
        rebuildDrivenOperations(&rebuildError);
        outputMessage(0, tr("部件已删除，自动移除参数：%1")
            .arg(removedNames.join(QStringLiteral("、"))));
        emit directModelingParametersChanged();
    }

    bool DirectModelingController::deleteDrivenParameter(const QString& parameterId,
        QString* errorMessage)
    {
        if (!m_parameterCalculator)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("参数计算器未初始化");
            return false;
        }
        // 定位节点：删除它 + 之后同部件全部节点（依赖其几何前缀）。
        int targetIndex = -1;
        int targetPartId = -1;
        for (size_t i = 0; i < m_drivenOperations.size(); ++i)
        {
            if (m_drivenOperations[i]
                && m_drivenOperations[i]->parameterId == parameterId)
            {
                targetIndex = static_cast<int>(i);
                targetPartId = m_drivenOperations[i]->partId;
                break;
            }
        }
        if (targetIndex < 0)
        {
            // 非驱动参数（手动建参等）：仅删参数本身。
            m_parameterCalculator->removeParameters({ parameterId });
            emit directModelingParametersChanged();
            return true;
        }
        QStringList removedNames;
        std::vector<std::unique_ptr<DrivenOperation>> kept;
        kept.reserve(m_drivenOperations.size());
        for (size_t i = 0; i < m_drivenOperations.size(); ++i)
        {
            DrivenOperation* node = m_drivenOperations[i].get();
            const bool removed = static_cast<int>(i) >= targetIndex
                && node != nullptr && node->partId == targetPartId;
            if (removed)
            {
                removedNames << node->parameterName;
                m_parameterCalculator->removeParameters({ node->parameterId });
            }
            else
            {
                kept.push_back(std::move(m_drivenOperations[i]));
            }
        }
        m_drivenOperations = std::move(kept);
        // 剩余步骤从基线重放（失败则标失效，几何保持现状）。
        QString rebuildError;
        rebuildDrivenOperations(&rebuildError);
        if (errorMessage && !rebuildError.isEmpty())
        {
            *errorMessage = rebuildError;
        }
        outputMessage(0, tr("已删除参数及依赖步骤：%1")
            .arg(removedNames.join(QStringLiteral("、"))));
        emit directModelingParametersChanged();
        return true;
    }

    void DirectModelingController::requestExactPreview(double value)
    {        //【ExactOnly 预览】替换面/圆角族：引擎"编辑不落盘"预览——跑同一编辑
        // 链但不提交部件，结果体离散为一次性网格直发渲染（updatePreview 以
        // identity 矩阵直画）；失败下发空数据清除旧预览，不动会话。
        if (!m_isActive || !hasActiveOperation())
        {
            return;
        }
        const EventDispatcher::mGeoDirectModelingOperation operation =
            MapToEngineOperation(m_currentOperation);
        if (operation != EventDispatcher::mGeoDirectModelingOperation::ReplaceFace
            && operation != EventDispatcher::mGeoDirectModelingOperation::ResizeFillet
            && operation != EventDispatcher::mGeoDirectModelingOperation::EdgeFillet)
        {
            return;
        }
        if (!std::isfinite(value))
        {
            return;
        }
        EventDispatcher::mGeoDirectFaceEditRequest req;
        req.operation = operation;
        req.value = value;
        req.name = "DirectModelPreview";
        if (m_currentOperation == MGeoCommand::DirectModelingOperation::EdgeFillet)
        {
            req.referenceEdgeId = m_selectedElementId;    // dim=1：共享边
        }
        else
        {
            req.faceIds.append(m_selectedElementId);      // dim=2：操作面
            if (m_currentOperation
                == MGeoCommand::DirectModelingOperation::ReplaceFace)
            {
                req.referenceFaceId = m_engineReferenceId;
            }
        }
        const EventDispatcher::mGeoPreviewResult result =
            getGEOServer.geoEdit().directFaceEditPreview(req);
        if (!result.succeeded())
        {
            emit previewDataReady({}, {}, {}, {});
            return;
        }
        std::vector<glm::vec3> facePoints;
        facePoints.reserve(result.mesh.faceTriangles.size());
        for (const GeoNeutralData::GN_Point& p : result.mesh.faceTriangles)
        {
            facePoints.push_back({ (float)p.x(), (float)p.y(), (float)p.z() });
        }
        std::vector<glm::vec3> edgePoints;
        edgePoints.reserve(result.mesh.lines.size());
        for (const GeoNeutralData::GN_Point& p : result.mesh.lines)
        {
            edgePoints.push_back({ (float)p.x(), (float)p.y(), (float)p.z() });
        }
        emit previewDataReady(std::move(facePoints), std::move(edgePoints), {}, {});
    }

    bool DirectModelingController::hasActiveOperation() const
    {
        return m_currentOperation != MGeoCommand::DirectModelingOperation::None;
    }
    void DirectModelingController::refreshDirectModelingParameters()
    {
        //【S11·参数驱动】参数集变化统一通知（面板刷新参数表 + 公式预览）。
        emit directModelingParametersChanged();
    }

    bool DirectModelingController::updateDirectModelingParameters(
        const QHash<QString, QString>& nameById,
        const QHash<QString, QString>& expressionById, QString* errorMessage)
    {
        if (!m_parameterCalculator)
        {
            if (errorMessage) *errorMessage = QStringLiteral("参数计算器未初始化");
            return false;
        }
        // 1) 名称/公式先入计算器（改名会同步迁移公式内 {} 引用）。
        if (!nameById.isEmpty()
            && !m_parameterCalculator->updateDefinitions(nameById, {}, errorMessage))
        {
            return false;
        }
        if (!expressionById.isEmpty()
            && !m_parameterCalculator->updateExpressions(expressionById, errorMessage))
        {
            return false;
        }
        // 2) 全部驱动操作按新值回放（失败参数标记几何失效，不中断其他回放）。
        m_parameterCalculator->clearGeometryValidation();
        QString rebuildError;
        if (!rebuildDrivenOperations(&rebuildError) && errorMessage
            && errorMessage->isEmpty())
        {
            *errorMessage = rebuildError;
        }
        emit directModelingParametersChanged();
        return true;
    }

    bool DirectModelingController::updateDirectModelingParameterExpressions(
        const QHash<QString, QString>& expressionById, QString* errorMessage)
    {
        if (!m_parameterCalculator)
        {
            if (errorMessage) *errorMessage = QStringLiteral("参数计算器未初始化");
            return false;
        }
        if (!m_parameterCalculator->updateExpressions(expressionById, errorMessage))
        {
            return false;
        }
        m_parameterCalculator->clearGeometryValidation();
        QString rebuildError;
        if (!rebuildDrivenOperations(&rebuildError) && errorMessage
            && errorMessage->isEmpty())
        {
            *errorMessage = rebuildError;
        }
        emit directModelingParametersChanged();
        return true;
    }

    bool DirectModelingController::evaluateDirectModelingExpression(
        const QString& expression, double& value, QString* errorMessage) const
    {
        if (!m_parameterCalculator)
        {
            if (errorMessage) *errorMessage = QStringLiteral("参数计算器未初始化");
            return false;
        }
        return m_parameterCalculator->evaluateExpression(
            expression, value, errorMessage);
    }

    //【直模引擎化·S11·GUI 预览源】移动/偏置面的 DirectTool 渲染源数据：
    // 从 GN 部件离散网格取选中面三角形与边界线段（偏置模式=全部件面与边；
    // 箭头锚点/方向由 beginOperation 预先写入）。后端无关，替代 S9 退役的
    // OCC 源面采样链。

    bool DirectModelingController::buildRenderPreviewSource(
        std::vector<glm::vec3>& facePoints, std::vector<glm::vec3>& edgePoints) const
    {
        if (m_selectedElementDim != 2 || m_selectedElementId <= 0) return false;
        const NeutralPreviewSource source = neutralPreviewSource(m_selectedElementId, m_moveSolid);
        if (!source.valid) return false;
        facePoints = source.facePoints;
        edgePoints = source.edgePoints;
        return true;
    }

    bool DirectModelingController::startDirectModelingOperation(MGeoCommand::DirectModelingOperation, bool, bool) { return false; }
    bool DirectModelingController::beginOperationInternal(int, int, MGeoCommand::DirectModelingOperation, bool, bool) { return false; }
    void DirectModelingController::resetSession()
    {
        m_selectedElementId = -1;
        m_selectedElementDim = -1;
        m_currentOperation = MGeoCommand::DirectModelingOperation::None;
        m_engineReferenceId = -1;
        m_currentParameter = 0.0;
        m_moveSolid = false;
        m_arrowCenter = QVector3D(0.0f, 0.0f, 0.0f);
        m_arrowDirection = QVector3D(0.0f, 0.0f, 1.0f);
        m_rotationAxisPoint = QVector3D(0.0f, 0.0f, 0.0f);
        m_rotationAxisDirection = QVector3D(0.0f, 0.0f, 1.0f);
        emit previewClearRequested();
    }
    void DirectModelingController::doPreview(double, bool) {}
    void DirectModelingController::preparePreviewSession() {}
    void DirectModelingController::invalidatePreviewSession() {}
    void DirectModelingController::handlePreviewResult(quint64, quint64, bool, QString,
        std::vector<glm::vec3>, std::vector<glm::vec3>,
        std::vector<glm::vec3>, std::vector<glm::vec3>) {}
    bool DirectModelingController::executeBatchCommand(const DirectModelingCommand&,
        const QVector<int>&, const QVector<int>&, QString*) { return false; }
    bool DirectModelingController::resolveScriptOriginalFaceIds(
        const QString&, const QVector<int>&, QVector<int>&, QString*) { return false; }
    bool DirectModelingController::captureScriptInitialTopologyPart(
        ScriptInitialTopologyIdContext&, MGeoData::GeoDataModelSet*, QString*) { return false; }
    QString DirectModelingController::currentParameterObjectKey() const { return QString(); }

    QString DirectModelingController::currentOperationDisplayName() const
    {
        switch (m_currentOperation)
        {
        case MGeoCommand::DirectModelingOperation::ReplaceFace: return tr("替换面");
        case MGeoCommand::DirectModelingOperation::RotateFace: return tr("旋转面");
        case MGeoCommand::DirectModelingOperation::MoveFace:
        case MGeoCommand::DirectModelingOperation::OffsetFace:
            return m_moveSolid ? tr("移动实体") : tr("移动面");
        case MGeoCommand::DirectModelingOperation::ResizeFillet:
        case MGeoCommand::DirectModelingOperation::EdgeFillet: return tr("调整圆角大小");
        case MGeoCommand::DirectModelingOperation::ResizeCylinder: return tr("调整圆柱大小");
        case MGeoCommand::DirectModelingOperation::ExtrudeEdge:
        case MGeoCommand::DirectModelingOperation::ExtrudeVertex: return tr("拉伸");
        case MGeoCommand::DirectModelingOperation::DeleteFace: return tr("删除面");
        default: return tr("直接建模");
        }
    }

    void DirectModelingController::outputMessage(int type, const QString& msg, bool statusbar) const
    {
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(type, msg, statusbar);
    }


    void DirectModelingController::recordDrivenOperation(
        MGeoCommand::DirectModelingOperation operation,
        const QVector<int>& selectedIds, int elementDim,
        const QVector<int>& referenceIds,
        double value, const QString& drivingExpression,
        bool moveSolid, const QVector3D& axisPoint,
        const QVector3D& axisDirection, int partIdHint,
        const QVector3D& preAnchorCenter, bool hasPreAnchor,
        int locatorIndex, int locatorCount,
        const QVector3D& preEditNormal, double preEditArea)
    {
        if (!m_parameterCalculator || selectedIds.isEmpty()) return;
        MGeoManager::GeoManager* manager =
            (getCVM != nullptr && getCVM->getModelData() != nullptr)
            ? getCVM->getModelData()->getGeoManager() : nullptr;
        int partId = partIdHint;
        if (partId < 0 && manager != nullptr)
        {
            partId = MGeoNeutralQuery::partIdByGeoId(manager, elementDim, selectedIds.first());
        }
        int referencePartId = -1;
        if (manager != nullptr && !referenceIds.isEmpty())
        {
            referencePartId = MGeoNeutralQuery::partIdByGeoId(manager, 2, referenceIds.first());
        }
        const int originalId = selectedIds.first();
        const QString objectKey = QStringLiteral("part:%1/dim:%2/topology:%3/op:%4%5")
            .arg(partId).arg(elementDim).arg(originalId)
            .arg(static_cast<int>(operation))
            .arg(moveSolid ? QStringLiteral("/scope:solid") : QString());
        QString parameterName;
        QString upsertError;
        if (!m_parameterCalculator->upsertOperationParameter(
                objectKey, currentOperationDisplayName(),
                drivingExpression, value, &parameterName, &upsertError))
        {
            outputMessage(2, tr("直接建模参数创建失败：%1").arg(upsertError));
            return;
        }
        for (auto& driven : m_drivenOperations)
        {
            if (!driven)
                continue;    // 防御：历史遗留空节点不再让提交路径崩溃
            if (driven->parameterId == objectKey)
            {
                driven->parameterName = parameterName;
                driven->operation = operation;
                driven->moveSolid = moveSolid;
                driven->partId = partId;
                driven->elementDim = elementDim;
                driven->selectedIds = { originalId };
                driven->referenceIds = referenceIds.toStdVector();
                driven->referencePartId = referencePartId;
                driven->axisPoint = axisPoint;
                driven->axisDirection = axisDirection;
                driven->currentValue = value;
                driven->historyRetired = false;
                emit directModelingParametersChanged();
                return;
            }
        }
        auto driven = std::make_unique<DrivenOperation>();
        driven->parameterId = objectKey;
        driven->parameterName = parameterName;
        driven->operation = operation;
        driven->moveSolid = moveSolid;
        driven->partId = partId;
        driven->elementDim = elementDim;
        //【边倒圆·面收养】边 ID 在上游参数改变后不稳定（布尔重建可销毁边），
        // 但面 ID 因历史映射而稳定：提交后从 createdIds 收养新生圆角面作为
        // 回放身份，回放走 ResizeFillet（面驱动）而非 EdgeFillet（边驱动）。
        if (operation == MGeoCommand::DirectModelingOperation::EdgeFillet
            && manager != nullptr)
        {
            //【边倒圆·几何锚定】记录被倒圆边的中点 + 所在面，回放时用几何
            // 匹配在当前面边界找回对应边（边 ID 因布尔重建不稳定）。回放
            // 走 EdgeFillet（从边创建圆角），不走 ResizeFillet（要求已有
            // 圆角面，但基线重放序列中圆角还没创建）。
            GeoNeutralData::mGeoPartManager* fpm =
                MGeoNeutralQuery::partMgr(manager);
            GeoNeutralData::GN_Part* fpart = nullptr;
            if (partId >= 0 && fpm != nullptr)
            {
                fpart = fpm->getPartPtrById(partId);
            }
            if (fpart == nullptr && fpm != nullptr)
            {
                for (int pid : fpm->getPartList())
                {
                    GeoNeutralData::GN_Part* cand = fpm->getPartPtrById(pid);
                    if (cand == nullptr || cand->dataManager() == nullptr)
                        continue;
                    const std::vector<int>& eids =
                        cand->getTopologyByDimension(1);
                    if (std::find(eids.begin(), eids.end(), originalId)
                        != eids.end())
                    {
                        fpart = cand;
                        partId = pid;
                        break;
                    }
                }
            }
            if (fpart != nullptr)
            {
                // 边中点
                const auto& edgePts =
                    fpart->getGeoDiscreteData().edgePoints;
                const auto eIt = edgePts.find(originalId);
                if (eIt != edgePts.end() && eIt->second.size() >= 2)
                {
                    const auto& pts = eIt->second;
                    const auto pIt0 =
                        fpart->getGeoDiscreteData().globalPoints.find(pts.front());
                    const auto pIt1 =
                        fpart->getGeoDiscreteData().globalPoints.find(pts.back());
                    if (pIt0 != fpart->getGeoDiscreteData().globalPoints.end()
                        && pIt1 != fpart->getGeoDiscreteData().globalPoints.end())
                    {
                        driven->filletEdgeMidpoint = QVector3D(
                            (float)(pIt0->second.x() + pIt1->second.x()) * 0.5f,
                            (float)(pIt0->second.y() + pIt1->second.y()) * 0.5f,
                            (float)(pIt0->second.z() + pIt1->second.z()) * 0.5f);
                    }
                }
                // 所在面：边的邻接面（取第一个）
                for (const auto& faceEntry :
                    fpart->getGeoDiscreteData().facePoints)
                {
                    const auto& kids = fpart->getChildren(
                        GeoNeutralData::GN_ObjectId(
                            GeoNeutralData::GN_ShapeType::Face,
                            faceEntry.first));
                    for (const auto& kid : kids)
                    {
                        if (kid.type == GeoNeutralData::GN_ShapeType::Wire)
                        {
                            for (const auto& ekid : fpart->getChildren(kid))
                            {
                                if (ekid.type
                                        == GeoNeutralData::GN_ShapeType::Edge
                                    && ekid.id == originalId)
                                {
                                    driven->filletHostFaceId = faceEntry.first;
                                    break;
                                }
                            }
                        }
                        else if (kid.type == GeoNeutralData::GN_ShapeType::Edge
                            && kid.id == originalId)
                        {
                            driven->filletHostFaceId = faceEntry.first;
                        }
                        if (driven->filletHostFaceId > 0) break;
                    }
                    if (driven->filletHostFaceId > 0) break;
                }
                driven->partId = partId;
                qWarning() << "[DM-Param] EdgeFillet anchored edge" << originalId
                    << "mid=" << driven->filletEdgeMidpoint
                    << "hostFace=" << driven->filletHostFaceId;
            }
        }
        if (driven->selectedIds.empty())
        {
            driven->selectedIds = { originalId };
        }
        driven->referenceIds = referenceIds.toStdVector();
        driven->referencePartId = referencePartId;
        driven->axisPoint = axisPoint;
        driven->axisDirection = axisDirection;
        driven->currentValue = value;
        m_drivenOperations.push_back(std::move(driven));
        qWarning() << "[DM-Param] recorded #" << m_drivenOperations.size()
            << "key=" << objectKey << "name=" << parameterName
            << "faceId=" << originalId << "refIds=" << referenceIds
            << "value=" << value;
        emit directModelingParametersChanged();
    }

    bool DirectModelingController::rebuildDrivenOperations(QString* errorMessage,
        const QString&)
    {
        if (!m_parameterCalculator || m_drivenOperations.empty()) return true;
        //【部件校验】回放前清理已删除部件的参数。
        validateDrivenParameters();
        if (m_drivenOperations.empty()) return true;
        bool anyChange = false;
        for (auto& driven : m_drivenOperations)
        {
            if (!driven || driven->historyRetired) continue;
            double value = 0.0;
            QString valueError;
            if (!m_parameterCalculator->parameterValue(driven->parameterId, value, nullptr, &valueError))
            {
                m_parameterCalculator->setGeometryInvalid(driven->parameterId, valueError);
                continue;
            }
            if (std::abs(value - driven->currentValue) > 1.0e-9) anyChange = true;
        }
        if (!anyChange) return true;
        auto invalidateAll = [this, errorMessage](const QString& reason) {
            for (auto& driven : m_drivenOperations)
            {
                if (driven && !driven->historyRetired)
                    m_parameterCalculator->setGeometryInvalid(driven->parameterId,
                        QStringLiteral("几何失效：%1").arg(reason));
            }
            if (errorMessage) *errorMessage = reason;
        };
        auto& baselineHistory = getGEOServer.history();
        if (!baselineHistory.restoreParametricBaseline().succeeded())
        {
            invalidateAll(QStringLiteral("参数基线恢复失败"));
            return false;
        }
        bool allSucceeded = true;
        for (auto& driven : m_drivenOperations)
        {
            if (!driven || driven->historyRetired) continue;
            double value = 0.0;
            QString valueError;
            if (!m_parameterCalculator->parameterValue(driven->parameterId, value, nullptr, &valueError))
            {
                m_parameterCalculator->setGeometryInvalid(driven->parameterId, valueError);
                allSucceeded = false;
                continue;
            }
            const int currentId = driven->selectedIds.empty() ? -1 : driven->selectedIds.front();
            if (currentId <= 0)
            {
                m_parameterCalculator->setGeometryInvalid(driven->parameterId,
                    QStringLiteral("几何失效：缺少操作面"));
                allSucceeded = false;
                continue;
            }
            int referenceId = -1;
            if (!driven->referenceIds.empty()) referenceId = driven->referenceIds.front();
            MGeoCommand::DirectModelingOperation replayOp = driven->operation;
            const bool isRotate = replayOp
                == MGeoCommand::DirectModelingOperation::RotateFace;
            QVector<int> replayFaces;
            int replayRefFace = -1;
            int replayRefEdge = -1;
            if (replayOp == MGeoCommand::DirectModelingOperation::EdgeFillet)
            {
                //【边倒圆·几何找边回放】基线重放序列中圆角还没创建——必须走
                // EdgeFillet（从边创建），不能走 ResizeFillet（要求已有圆角）。
                // 边 ID 因布尔重建不稳定：用提交时边中点在所在面边界中找最近边。
                MGeoManager::GeoManager* filletManager =
                    (getCVM != nullptr && getCVM->getModelData() != nullptr)
                    ? getCVM->getModelData()->getGeoManager() : nullptr;
                if (driven->filletHostFaceId > 0
                    && driven->filletEdgeMidpoint.lengthSquared() > 1.0e-14f
                    && filletManager != nullptr)
                {
                    GeoNeutralData::mGeoPartManager* rpm =
                        MGeoNeutralQuery::partMgr(filletManager);
                    const GeoNeutralData::GN_Part* rpart = rpm
                        ? rpm->getPartPtrById(driven->partId) : nullptr;
                    if (rpart != nullptr)
                    {
                        // 面边界的边 ID 列表
                        std::vector<int> hostEdges;
                        for (const auto& kid : rpart->getChildren(
                                 GeoNeutralData::GN_ObjectId(
                                     GeoNeutralData::GN_ShapeType::Face,
                                     driven->filletHostFaceId)))
                        {
                            if (kid.type == GeoNeutralData::GN_ShapeType::Edge)
                                hostEdges.push_back(kid.id);
                            else if (kid.type
                                == GeoNeutralData::GN_ShapeType::Wire)
                                for (const auto& ek : rpart->getChildren(kid))
                                    if (ek.type
                                        == GeoNeutralData::GN_ShapeType::Edge)
                                        hostEdges.push_back(ek.id);
                        }
                        // 找中点最近的边
                        int bestEdge = -1;
                        float bestDist = 1.0e30f;
                        const auto& ep = rpart->getGeoDiscreteData().edgePoints;
                        const auto& gp =
                            rpart->getGeoDiscreteData().globalPoints;
                        for (int eid : hostEdges)
                        {
                            const auto eIt2 = ep.find(eid);
                            if (eIt2 == ep.end() || eIt2->second.size() < 2)
                                continue;
                            const auto a = gp.find(eIt2->second.front());
                            const auto b = gp.find(eIt2->second.back());
                            if (a == gp.end() || b == gp.end()) continue;
                            const QVector3D mid(
                                (float)(a->second.x() + b->second.x()) * 0.5f,
                                (float)(a->second.y() + b->second.y()) * 0.5f,
                                (float)(a->second.z() + b->second.z()) * 0.5f);
                            const float dist =
                                (mid - driven->filletEdgeMidpoint)
                                    .lengthSquared();
                            if (dist < bestDist)
                            {
                                bestDist = dist;
                                bestEdge = eid;
                            }
                        }
                        if (bestEdge > 0 && bestDist < 100.0f)
                        {
                            replayRefEdge = bestEdge;
                            qWarning() << "[DM-Param] EdgeFillet replay found"
                                << "edge" << bestEdge << "dist" << bestDist;
                        }
                    }
                }
                if (replayRefEdge <= 0)
                {
                    // 兜底：直接用记录的边 ID（可能因布尔改号失效）
                    replayRefEdge = currentId;
                }
            }
            else if (replayOp
                == MGeoCommand::DirectModelingOperation::ResizeFillet)
            {
                replayFaces = QVector<int>{ currentId };
            }
            else if (replayOp
                == MGeoCommand::DirectModelingOperation::ReplaceFace)
            {
                replayFaces = QVector<int>{ currentId };
                replayRefFace = referenceId;
            }
            else
            {
                replayFaces = QVector<int>{ currentId };
            }
            qWarning() << "[DM-Param] replay key=" << driven->parameterId
                << "faceId=" << currentId << "refFace=" << replayRefFace
                << "value=" << value << "op=" << static_cast<int>(replayOp);
            //【修复】RotateFace 回放时用面中轴线重算轴：上游参数值改变后
            // 面位置变化，提交时的轴可能不在当前面平面内（DraftAngle 校验
            // 会拒绝"axis must lie in the selected face plane"）。
            QVector3D replayAxisPoint = driven->axisPoint;
            QVector3D replayAxisDirection = driven->axisDirection;
            if (isRotate)
            {
                QVector3D recomputedPoint, recomputedDirection;
                if (faceMedianAxis(currentId, recomputedPoint, recomputedDirection))
                {
                    replayAxisPoint = recomputedPoint;
                    replayAxisDirection = recomputedDirection;
                }
            }
            const QString error = CommitDirectEdit(
                MapToEngineOperation(replayOp),
                replayFaces, replayRefFace, replayRefEdge,
                value, replayAxisPoint, replayAxisDirection, isRotate,
                driven->moveSolid);
            if (!error.isEmpty())
            {
                qWarning() << "[DM-Param] replay FAILED key=" << driven->parameterId
                    << "error=" << error;
                m_parameterCalculator->setGeometryInvalid(driven->parameterId, error);
                allSucceeded = false;
                continue;    //【级联保护】单个操作失败不中断后续回放
            }
            driven->currentValue = value;
            qWarning() << "[DM-Param] replay OK key=" << driven->parameterId;
        }
        emit directModelingParametersChanged();
        if (!allSucceeded && errorMessage)
            *errorMessage = QStringLiteral("部分参数回放失败");
        return allSucceeded;
    }

} // namespace MechMainWindow
