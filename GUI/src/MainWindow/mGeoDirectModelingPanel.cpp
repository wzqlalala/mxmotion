#include <mGeoDataBase.h>
#include <mGeoManager.h>
#include "mGeoDirectModelingPanel.h"
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"
#include "mMechGlobalSignals.h"
#include "mUiMenu.h"
#include "mViewportModelManager.h"
#include "ViewportContainer.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
#include "mDirectModelingController.h"
#include <mMeshData.h>
#include "mGeoDevPreviewGate.h" //【GUI 门控】直模 V2
#include "mGeoManager.h" //【GUI适配】GeoManager 完整类型（getGeoModelBase）
//#include <mGeoDataModelSet.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include "mAPIServer.h"

// 渲染层依赖
#include "mxr/view/mBaseRend.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "InteractionCore/Controller/InteractionToolManager.h"
#include "InteractionCore/Tool/DirectGeoModelingTool.h"
#include "PreViewportWidget.h"
#include "LowCPlusDefine.h"

#include <QComboBox>
#include <QCheckBox>
#include <QColor>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QSignalBlocker>
#include <QTimer>
#include <QPointer>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <utility>

#include "PickCommonFunction.h"


using namespace mxr;
namespace MechMainWindow
{
    /*
     * Panel 的数据流：
     *   SelectionDataContainer -> processPickResult -> Controller 建立几何会话
     *   移动面：Controller 源面离散 -> 适配器缓存 -> 新版渲染矩阵扫掠预览
     *   实体平移：Controller 完整实体离散 -> 适配器缓存 -> 累计距离平移全部面/边
     *   DirectTool 本段拖动增量 -> Panel 累加 -> 更新缓存预览或后台精确预览
     *   当前折叠块“确定” -> Controller 提交 -> 数据层刷新 -> Panel 清理
     *
     * Panel 不保存 TopoDS_Shape，也不直接调用 OCC；它只保存一次 UI 会话所需的
     * 业务元素 ID。提交后这些 ID 会失效，必须由 resetPanelAfterCommit 清空。
     */
    namespace
    {
        constexpr int kReplaceFaceFoldIndex = 0;
        constexpr int kRotateFaceFoldIndex = 1;
        constexpr int kOffsetFaceFoldIndex = 2;
        constexpr int kResizeFilletFoldIndex = 3;
        constexpr int kGlobalParameterFoldIndex = 4;

        //【参数显示】最多保留 6 位小数（去尾零；整数不带小数点）。
        QString formatParameterValue(double value)
        {
            if (!std::isfinite(value))
            {
                return QString();
            }
            QString text = QString::number(value, 'f', 6);
            if (text.contains(QLatin1Char('.')))
            {
                while (text.endsWith(QLatin1Char('0')))
                {
                    text.chop(1);
                }
                if (text.endsWith(QLatin1Char('.')))
                {
                    text.chop(1);
                }
            }
            return text;
        }
        constexpr int kParameterNameColumn = 0;
        constexpr int kParameterExpressionColumn = 1;
        constexpr int kParameterValueColumn = 2;
        constexpr int kParameterStatusColumn = 3;

        /**
         * 只改写完整的“{参数名}”引用。一次扫描可正确处理 A/B 名称互换，也不会
         * 把 {A} 的改名误应用到 {A1}；不完整引用原样保留，交给表达式解析器报错。
         */
        QString rewriteBracedParameterNames(
            const QString& expression,
            const QHash<QString, QString>& newNameByOldNormalizedName)
        {
            if (newNameByOldNormalizedName.isEmpty())
                return expression;

            QString rewritten;
            rewritten.reserve(expression.size());
            for (int position = 0; position < expression.size();)
            {
                if (expression.at(position) != QLatin1Char('{'))
                {
                    rewritten += expression.at(position++);
                    continue;
                }

                const int close = expression.indexOf(QLatin1Char('}'), position + 1);
                if (close < 0)
                {
                    rewritten += expression.mid(position);
                    break;
                }
                const QString oldName = expression.mid(
                    position + 1, close - position - 1).trimmed();
                const auto renamed = newNameByOldNormalizedName.constFind(
                    oldName.toCaseFolded());
                if (renamed != newNameByOldNormalizedName.constEnd())
                    rewritten += QLatin1Char('{') + renamed.value() + QLatin1Char('}');
                else
                    rewritten += expression.mid(position, close - position + 1);
                position = close + 1;
            }
            return rewritten;
        }
    }

    /**
     * 普通操作使用 DirectTool 的拾取/清理/显隐接口，按 Panel 累计参数绘制。
     * 旋转通过独立接口配置源面与旋转轴，圆环拾取和即时旋转预览由 Tool 负责。
     * 面移动缓存目标面及扫掠侧壁，实体平移只缓存完整实体；均不逐帧重建 OCC。
     */
    class DirectModelingRenderAdapter
    {
    public:
        enum class PreviewKind
        {
            ExactOnly,       ///< 替换和圆角：等待 Controller 的精确结果。
            FaceTranslation, ///< 单面移动：目标面 + 源边到目标边之间的扫掠侧壁。
            SolidTranslation, ///< 整体平移：全部顶点使用同一个矩阵，不生成扫掠侧壁。
            Rotation          ///< 独立圆环交互与 Tool 即时旋转；释放后覆盖精确结果。
        };

        DirectModelingRenderAdapter()
        {
            // 线性 Tool 在信号后仍会绘制，需延后覆盖；旋转分支则由 Tool 先画
            // 再发信号。这里同时负责合并精确结果，不能反过来清除旋转近似。
            m_frameTimer.setSingleShot(true);
            m_frameTimer.setInterval(0);
            QObject::connect(&m_frameTimer, &QTimer::timeout,
                &m_frameTimer, [this]() { drawPendingPreview(); });
        }

        bool isRotationDragging() const
        {
            if (!m_configured || m_kind != PreviewKind::Rotation
                || m_previewWidget.isNull() || m_previewWidget.data() != activeRenderWidget())
                return false;
            auto* tool = activeTool();
            return tool && tool->isRotationDragging();
        }

        bool requestPreview(double accumulatedValue)
        {
            if (!m_configured || !std::isfinite(accumulatedValue)
                || !std::isfinite(static_cast<float>(accumulatedValue)))
                return false;
            if (m_kind == PreviewKind::Rotation)
            {
                // 旋转没有延迟回调兜底，直接调用前也必须核对视口和工具生命周期。
                auto* widget = m_previewWidget.data();
                auto* tool = activeTool();
                if (!widget || widget != activeRenderWidget() || !tool)
                    return false;
                m_frameTimer.stop();
                m_drawExact = false;
                m_value = (std::max)(-90.0, (std::min)(90.0, accumulatedValue));
                // 同步输入和圆环拖动的累计角；不要再排队 ExactOnly 清屏覆盖圆环预览。
                return tool->setRotationPreviewAngle(m_value);
            }
            if (m_kind == PreviewKind::FaceTranslation
                || m_kind == PreviewKind::SolidTranslation)
            {
                //【X线绕过】平移族预览已由本适配器接管（configureOperation 里
                // setPairedOffsetExternalPreview(true)，工具跳过自画块——其
                // 硬编码黑色线色在渲染 drawLine 绑定错绑下会把端点1 钉死在
                // 源位画出对角线）。此处必须排队重绘，适配器是唯一绘制通道。
                m_value = accumulatedValue;
                m_drawExact = false;
                scheduleFrame();
                return true;
            }
            m_value = accumulatedValue;
            // 新一段拖动不能保留上一参数的精确结果。ExactOnly 暂时清屏，
            // 防止圆角面被 Tool 的默认平移扫掠显示成拉伸，后台完成后再显示圆角。
            m_drawExact = false;
            scheduleFrame();
            return true;
        }

        void updatePreview(
            std::vector<glm::vec3> addedFacePoints,
            std::vector<glm::vec3> addedEdgePoints,
            std::vector<glm::vec3> removedFacePoints,
            std::vector<glm::vec3> removedEdgePoints)
        {
            if (!m_configured || m_kind == PreviewKind::SolidTranslation)
                return;
            appendPoints(addedFacePoints, std::move(removedFacePoints));
            appendPoints(addedEdgePoints, std::move(removedEdgePoints));
            m_exactFaces = std::move(addedFacePoints);
            m_exactEdges = std::move(addedEdgePoints);
            m_exactFaceFlags.assign(m_exactFaces.size(), 1.0f);
            m_exactEdgeFlags.assign(m_exactEdges.size(), 1.0f);
            m_drawExact = true;
            scheduleFrame();
        }

        void clearPreview()
        {
            resetPreviewState();
            if (auto* tool = activeTool())
            {
                tool->clearRotationInteraction();
                tool->clearPreview();
                //【X线绕过】退出适配器接管，工具恢复默认自画路径。
                tool->setPairedOffsetExternalPreview(false);
            }
            clearRenderBuffers(activeRenderWidget());
        }

        void finishSession()
        {
            clearPreview();
            if (auto* tool = activeTool())
                tool->showOrHiddleArrow(false);
        }

        /**
         * 普通操作只调用原有拾取接口；旋转单独传入源面和轴，不借用线性箭头。
         * 每次切换先清理旧圆环状态，不把旋转模式或角度泄漏到普通操作中。
         */
        bool configureOperation(
            int partId, int elementId, int elementDim,
            PreviewKind kind,
            std::vector<glm::vec3> sourceFacePoints,
            std::vector<glm::vec3> sourceEdgePoints,
            const QVector3D& center, const QVector3D& direction,
            bool showManipulator, double initialValue = 0.0)
        {
            finishSession();
            if (kind != PreviewKind::ExactOnly
                && (sourceFacePoints.empty() || sourceFacePoints.size() % 3 != 0
                    || sourceEdgePoints.size() % 2 != 0
                    || !std::isfinite(direction.lengthSquared())
                    || direction.lengthSquared() <= 1.0e-14f))
                return false;
            if (!getCVM || !getCVM->getPreViewportWidget()
                || !getCVM->getPreViewportWidget()->interaction())
                return false;
            auto* manager = getCVM->getPreViewportWidget()
                ->interaction()->getToolManager();
            if (!manager)
                return false;
            manager->setActive(mxr::InteractionToolType::DirectGeoModeling);
            auto* tool = manager->active<mxr::InteractionDirectGeoModelingTool>();
            auto* widget = activeRenderWidget();
            if (!tool || !widget || !widget->getGeoEditPreviewRender())
                return false;

            tool->clearPreview();
            const glm::vec3 origin(center.x(), center.y(), center.z());
            const QVector3D unitDirection = direction.normalized();
            const glm::vec3 axis(unitDirection.x(), unitDirection.y(), unitDirection.z());
            if (kind == PreviewKind::Rotation)
            {
                if (!tool->configureRotationPreview(
                    std::move(sourceFacePoints), std::move(sourceEdgePoints),
                    origin, axis, initialValue))
                {
                    tool->clearRotationInteraction();
                    return false;
                }
            }
            else if (showManipulator)
                tool->pickedGeoEleID(partId, elementId, elementDim);
            tool->showOrHiddleArrow(showManipulator);
            //【X线绕过·临时】渲染 drawLine 绑定错绑（详见 drawPendingPreview 注释）：
            // 平移族预览由本适配器接管绘制，工具不再用硬编码黑色线色调用
            // DrawLine（黑色 .x=0 使端点1 钉死在源位，画成侧墙对角线）。
            // 接管后工具的箭头/拖拽信号照常工作，仅跳过其自画块。
            // 渲染组修复 drawLine 绑定后可回退本接管。
            if (showManipulator && (kind == PreviewKind::FaceTranslation
                || kind == PreviewKind::SolidTranslation))
            {
                tool->setPairedOffsetExternalPreview(true);
            }

            m_previewWidget = widget;
            m_configured = true;
            m_kind = kind;
            m_partId = partId;
            m_showManipulator = showManipulator;
            m_value = initialValue;
            m_origin = origin;
            m_direction = axis;
            m_sourceFaces = std::move(sourceFacePoints);
            m_faceFlags.assign(m_sourceFaces.size(), 1.0f);
            if (kind == PreviewKind::SolidTranslation)
            {
                m_sourceEdges = std::move(sourceEdgePoints);
                m_edgeFlags.assign(m_sourceEdges.size(), 1.0f);
            }
            else if (kind == PreviewKind::FaceTranslation)
            {
                buildFaceSweep(sourceEdgePoints);
            }
            return true;
        }

    private:
        void scheduleFrame()
        {
            if (!m_frameTimer.isActive())
                m_frameTimer.start();
        }

        void resetPreviewState()
        {
            m_frameTimer.stop();
            m_configured = false;
            m_showManipulator = false;
            m_drawExact = false;
            m_value = 0.0;
            m_partId = -1;
            m_previewWidget.clear();
            m_sourceFaces.clear();
            m_sourceEdges.clear();
            m_sweepFaces.clear();
            m_faceFlags.clear();
            m_edgeFlags.clear();
            m_sweepFlags.clear();
            m_exactFaces.clear();
            m_exactEdges.clear();
            m_exactFaceFlags.clear();
            m_exactEdgeFlags.clear();
        }

        void buildFaceSweep(const std::vector<glm::vec3>& edges)
        {
            // 与新版 Tool 的着色器约定一致：+1 随矩阵移动，-1 保留源位置。
            // 只在选择变化时构造一次；输入累计值时只更新矩阵，不重算这些顶点。
            //【X线绕过·临时】渲染 drawLine 绑定错绑（loc3=颜色）使端点1 的
            // sketchflag 读到颜色 .x，线段按"端点1 恒为 +1"重排：预览位轮廓
            // 段 (+1,+1) 与角点连接段 (-1,+1)，不输出源位轮廓段 (-1,-1)——
            // 它的端点1 会被错绑强制移动画成对角线（源位轮廓由实体自身棱线
            // 提供视觉）。该结构在渲染修复后依然正确，无需回退本函数。
            m_sweepFaces.reserve(edges.size() * 3);
            m_sweepFlags.reserve(edges.size() * 3);
            m_sourceEdges.reserve(edges.size() * 3);
            m_edgeFlags.reserve(edges.size() * 3);
            for (std::size_t i = 0; i + 1 < edges.size(); i += 2)
            {
                const auto& first = edges[i];
                const auto& second = edges[i + 1];
                m_sweepFaces.insert(m_sweepFaces.end(),
                    { first, second, second, second, first, first });
                m_sweepFlags.insert(m_sweepFlags.end(),
                    { -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f });
                m_sourceEdges.insert(m_sourceEdges.end(),
                    { first, second, first, first, second, second });
                m_edgeFlags.insert(m_edgeFlags.end(),
                    { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f });
            }
        }

        static void clearRenderBuffers(mxr::CCSMRendWidget* widget)
        {
            if (!widget || !widget->getGeoEditPreviewRender())
                return;
            auto renderer = widget->getGeoEditPreviewRender();
            // DrawVectFace 是新版新增的扫掠侧面通道，只清面和线会残留旧侧壁。
            // 显式单位矩阵避免沿用 Tool 上一帧的 _moveM。
            const glm::mat4 identity(1.0f);
            renderer->DrawVectFace({}, {}, glm::vec4(0.8f, 0.7f, 0.6f, 0.3f), identity);
            renderer->DrawFace({}, {}, glm::vec4(0.8f, 0.7f, 0.6f, 0.3f), identity);
            renderer->DrawLine({}, {}, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, identity);
            widget->updatePaint();
        }

        glm::vec4 sourcePartPreviewColor() const
        {
            constexpr float kPreviewFaceAlpha = 0.3f;
            if (m_partId >= 0 && getCVM && getCVM->getModelData()
                && getCVM->getModelData()->getMeshData())
            {
                mPartData* part =
                    getCVM->getModelData()->getMeshData()->getPartData(
                        static_cast<uint64_t>(m_partId));
                if (part)
                {
                    const QColor color = part->getPartInnerColor();
                    if (color.isValid())
                    {
                        return glm::vec4(color.redF(), color.greenF(),
                            color.blueF(), kPreviewFaceAlpha);
                    }
                }
            }
            return glm::vec4(0.8f, 0.7f, 0.6f, kPreviewFaceAlpha);
        }

        void drawPendingPreview()
        {
            auto* widget = m_previewWidget.data();
            if (!m_configured || !widget || widget != activeRenderWidget()
                || !activeTool() || !widget->getGeoEditPreviewRender())
                return;
            if (m_kind == PreviewKind::Rotation && !m_drawExact)
                return; // Tool 已显示即时旋转，旧定时器不得清屏或按平移重画。

            //【X线绕过·临时】渲染 OsgGeoEditPreviewFx::drawLine 把颜色数组错绑
            // 到 loc3（着色器端点1 的 aSketch1），端点1 的 sketchflag 实际读到
            // 颜色 .x 分量：黑色线（.x=0）导致所有线段端点1 永不随矩阵移动，
            // 平移预览被画成每面侧墙一条对角线。绕过方式：线段数据按"端点1
            // 全为 +1"重排（buildFaceSweep/m_exactEdges 均满足），线色取 .x=1
            // 使端点1 恒随矩阵移动，与真实 flag 一致。渲染组修复 drawLine 的
            // 属性绑定后，把该色改回黑色 {0,0,0,1} 即可。
            const glm::vec4 workaroundLineColor(1.0f, 0.0f, 0.0f, 1.0f);
            auto renderer = widget->getGeoEditPreviewRender();
            const glm::mat4 identity(1.0f);
            const glm::vec4 faceColor = sourcePartPreviewColor();
            if (m_drawExact)
            {
                // OCC 结果已是最终世界坐标，不能再次叠加上一帧的平移矩阵。
                renderer->DrawVectFace({}, {}, faceColor, identity);
                renderer->DrawFace(m_exactFaces, m_exactFaceFlags, faceColor, identity);
                renderer->DrawLine(m_exactEdges, m_exactEdgeFlags, workaroundLineColor, 1.0f, identity);
            }
            else if (m_kind == PreviewKind::FaceTranslation || m_kind == PreviewKind::SolidTranslation)
            {
                //【X线绕过】平移族预览已由本适配器接管（configureOperation 里
                // setPairedOffsetExternalPreview(true)，工具不再自画），此处是
                // 唯一绘制通道：墙体（VectFace 通道不受绑定错绑影响）+ 预览位
                // 面 + 预览位轮廓/角点连接线。
                const glm::vec3 offset = m_direction * static_cast<float>(m_value);
                glm::mat4 translation(1.0f);
                translation[3] = glm::vec4(offset, 1.0f);
                // SolidTranslation 的 sweep 数组为空；整体平移只画半透明体与线框。
                renderer->DrawVectFace(m_sweepFaces, m_sweepFlags, faceColor, translation);
                renderer->DrawFace(m_sourceFaces, m_faceFlags, faceColor, translation);
                renderer->DrawLine(m_sourceEdges, m_edgeFlags, workaroundLineColor, 1.0f, translation);
            }
            else
            {
                clearRenderBuffers(widget);
            }

            if (m_showManipulator && (m_kind == PreviewKind::FaceTranslation
                || m_kind == PreviewKind::SolidTranslation))
            {
                // 新箭头类型/字段与 Tool 保持一致。这里只修正可见位置，不修改
                // _normal，也不声称能设置 Tool 私有的 _center/_nor/_recdis。
                // 拖动参数仍以 Panel 的 base + delta 为准，不依赖 Tool 累计缓存。
                auto* symbols = widget->GetSymbolController();
                const QString arrowName = QStringLiteral("DirectGeoModelingArrow");
                if (symbols && symbols->isExistSymbol(arrowName))
                {
                    const auto& arrow = symbols->getOrCreateSymbolData<mxr::CubCylSphBoxMoveAxis>(arrowName);
                    const glm::vec3 center = m_origin + m_direction * static_cast<float>(m_value);
                    arrow->_pos = { center.x, center.y, center.z };
                    symbols->UpdateAllSymbol();
                }
            }
            widget->updatePaint();
        }

        QTimer m_frameTimer;
        QPointer<mxr::CCSMRendWidget> m_previewWidget;
        PreviewKind m_kind = PreviewKind::ExactOnly;
        bool m_configured = false;
        bool m_showManipulator = false;
        bool m_drawExact = false;
        double m_value = 0.0;
        int m_partId = -1;
        glm::vec3 m_origin{ 0.0f };
        glm::vec3 m_direction{ 0.0f, 0.0f, 1.0f };
        std::vector<glm::vec3> m_sourceFaces, m_sourceEdges, m_sweepFaces;
        std::vector<float> m_faceFlags, m_edgeFlags, m_sweepFlags;
        std::vector<glm::vec3> m_exactFaces, m_exactEdges;
        std::vector<float> m_exactFaceFlags, m_exactEdgeFlags;

        static void appendPoints(
            std::vector<glm::vec3>& destination,
            std::vector<glm::vec3>&& source)
        {
            if (destination.empty())
                destination = std::move(source);
            else
                destination.insert(destination.end(),
                    std::make_move_iterator(source.begin()), std::make_move_iterator(source.end()));
        }

        static mxr::CCSMRendWidget* activeRenderWidget()
        {
            return getCVM ? getCVM->getPreRenderWidget() : nullptr;
        }

        static mxr::InteractionDirectGeoModelingTool* activeTool()
        {
            if (!getCVM || !getCVM->getPreViewportWidget()
                || !getCVM->getPreViewportWidget()->interaction())
                return nullptr;
            auto* manager = getCVM->getPreViewportWidget()->interaction()->getToolManager();
            return manager ? manager->active<mxr::InteractionDirectGeoModelingTool>() : nullptr;
        }
    };

    mGeoDirectModelingPanel::mGeoDirectModelingPanel(QWidget* parent)
        : mGeoPanelBase(parent)
        , m_controller(DirectModelingController::getInstance())
        , m_renderAdapter(std::make_unique<DirectModelingRenderAdapter>())
    {
        connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::activated), [this](int) {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = DirectModel;
            });

        // 隐藏拾取列表，避免占用面板空间
        pickObjTable->setVisible(false);

        initDirectModelingPanel();
        _mainLayout->addStretch();

        // 全局信号（拾取/单位制/视口切换/控制器）统一改由 mInfoWindow
        // 进入面板后 connectGeoSignals 连接，uneditGeometry 统一断开。
    }

    mGeoDirectModelingPanel::~mGeoDirectModelingPanel()
    {

    }

    void mGeoDirectModelingPanel::connectSignal()
    {
        // getCVM 可能因子窗口切换而变化，不能长期持有旧 SignalEmitter 指针。
        // 这里只更换 DirectTool 的拖动连接；conn_geo 由
        // slot_subWindowActivated 绑定到同一个活动视口。
        disConnectSignal();
        // connectSignal 是公开的历史入口，即使外部在退出后误调，
        // 也不允许重新挂上 DirectTool 的拖动回调。
        if (!m_controller || !m_controller->isActive())
            return;

        if (!getCVM || !getCVM->getPreViewportWidget()
            || !getCVM->getPreViewportWidget()->interaction())
        {
            return;
        }

        auto* emitter = getCVM->getPreViewportWidget()->interaction()->getSignalEmitter();
        if (!emitter)
            return;

        m_stretchConnection = connect(
            emitter, &mxr::SignalEmitter::signal_DirectGeoModelingStretch,
            this, [this](double distance) {
                slot_DirectGeoModelingStretch(distance);
            });
        m_stretchFinishConnection = connect(
            emitter, &mxr::SignalEmitter::signal_DirectGeoModelingStretchFinish,
            this, [this](double distance) {
                slot_DirectGeoModelingStretchFinish(distance);
            });
    }

    void mGeoDirectModelingPanel::disConnectSignal()
    {
        if (m_stretchConnection)
        {
            disconnect(m_stretchConnection);
            m_stretchConnection = QMetaObject::Connection();
        }
        if (m_stretchFinishConnection)
        {
            disconnect(m_stretchFinishConnection);
            m_stretchFinishConnection = QMetaObject::Connection();
        }
    }

    void mGeoDirectModelingPanel::disconnectRuntimeSignals()
    {
        disConnectSignal();
        if (conn_geo)
        {
            disconnect(conn_geo);
            conn_geo = QMetaObject::Connection();
        }
        m_runtimeViewportKey = nullptr;
    }

    void mGeoDirectModelingPanel::connectGeoSignals()
    {
        //控制器信号（预览/提交/模式/参数表）带 lambda，无法按签名去重，
        //先整体断开再重连，避免未退出面板切换功能类型时叠加连接
        disconnect(m_controller, nullptr, this, nullptr);
        connect(m_controller, &DirectModelingController::previewDataReady,
            this,
            [this](
                std::vector<glm::vec3> addedFacePoints,
                std::vector<glm::vec3> addedEdgePoints,
                std::vector<glm::vec3> removedFacePoints,
                std::vector<glm::vec3> removedEdgePoints) {
                    m_renderAdapter->updatePreview(
                        std::move(addedFacePoints),
                        std::move(addedEdgePoints),
                        std::move(removedFacePoints),
                        std::move(removedEdgePoints));
            });
        connect(m_controller, &DirectModelingController::previewClearRequested,
            this, [this]() { m_renderAdapter->clearPreview(); });
        connect(m_controller, &DirectModelingController::arrowHidden,
            this, [this]() { m_renderAdapter->finishSession(); });
        connect(m_controller, &DirectModelingController::commitFinished,
            this, [this](bool success) {
                if (success)
                    resetPanelAfterCommit();
            });
        connect(m_controller, &DirectModelingController::modeChanged,
            this, &mGeoDirectModelingPanel::handleDirectModelingModeChanged);
        connect(m_controller, &DirectModelingController::directModelingParametersChanged,
            this, [this]() {
                refreshParameterTable();
                updatePreviewFromDrivenExpression();
            });

        // 新架构：直接挂容器激活信号（带 prevID，可区分真正的视口切换）。
        // currentViewportChanged 由容器信号驱动管理器同步发出，二者等价，
        // 这里用容器信号以便后续按 viewportID/prevID 做精细判断
        if (conn_viewportChanged)
            disconnect(conn_viewportChanged);
        if (auto* container = MechPreData::mViewportModelManager::getInstance()->getViewportContainer()) {
            conn_viewportChanged = connect(container, &MechMainWindow::ViewportContainer::sig_activeViewportChanged,
                this, [this](int viewportID, int prevID) {
                    Q_UNUSED(viewportID); Q_UNUSED(prevID);
                    slot_subWindowActivated();
                });
        }
        else {
            conn_viewportChanged = connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, [this](int) { slot_subWindowActivated(); });
        }

        //拾取信号（内部按活动视口/会话状态守卫，重复调用安全）
        slot_subWindowActivated();
        //单位制信号
        connectUnitSystemResponse();
    }

    void mGeoDirectModelingPanel::disconnectGeoSignals()
    {
        //拾取与拖动等运行期信号
        disconnectRuntimeSignals();
        //控制器信号（预览/提交/模式/参数表）
        disconnect(m_controller, nullptr, this, nullptr);
        //视口切换信号
        if (conn_viewportChanged)
        {
            disconnect(conn_viewportChanged);
            conn_viewportChanged = QMetaObject::Connection();
        }
        //单位制信号
        disconnectUnitSystemResponse();
    }


    void mGeoDirectModelingPanel::showFold(GeoOperationType type)
    {
        // 工具栏的一次功能选择展示一个操作块；全局参数表始终作为独立折叠块附带。
        hideAllFold();

        DirectModelingType modelingType = DirectModelingType::OffsetFace;
        int operationFoldIndex = -1;
        switch (type)
        {
        case GeoDirectModelingReplaceFace:
            modelingType = DirectModelingType::ReplaceFace;
            operationFoldIndex = kReplaceFaceFoldIndex;
            break;
        case GeoDirectModelingRotateFace:
            modelingType = DirectModelingType::RotateFace;
            operationFoldIndex = kRotateFaceFoldIndex;
            break;
        case GeoDirectModelingOffsetFace:
            // 兼容旧的统一入口，默认进入移动面。
            modelingType = DirectModelingType::OffsetFace;
            operationFoldIndex = kOffsetFaceFoldIndex;
            break;
        case GeoDirectModelingResizeFillet:
            modelingType = DirectModelingType::ResizeFillet;
            operationFoldIndex = kResizeFilletFoldIndex;
            break;
        default:
            break;
        }

        if (operationFoldIndex >= 0)
        {
            // 先在进入入口核对已删除部件并刷新参数，再取消旧选择。
            // onCancel 可能回放参数历史，不能让它先处理已经从模型中移除的部件。
            if (m_controller)
                m_controller->enterDirectModelingMode();
            setDirectModelingType(modelingType);
            displayAfterInsertFolds({ operationFoldIndex, kGlobalParameterFoldIndex });
            // showFold 可能发生在子窗口已经激活之后，需要在此主动绑定当前视口。
            slot_subWindowActivated();
            if (geoPickWidget)
            {
                geoPickWidget->_featPick =
                    m_controller && m_controller->isActive()
                    ? DirectModel : None;
            }
        }
        else
        {
            disconnectRuntimeSignals();
            if (m_controller)
                m_controller->exitDirectModelingMode();
            if (geoPickWidget)
                geoPickWidget->_featPick = None;
        }
    }

    void mGeoDirectModelingPanel::setDirectModelingType(DirectModelingType type)
    {
        if (m_controller && m_controller->hasActiveOperation())
        {
            // 激活任一操作都从新会话开始，旧预览和旧拓扑 ID 不能继续复用。
            m_controller->onCancel();
        }

        // 类型切换后旧参数的单位/语义可能不同，必须完整重置 UI 状态。
        m_currentModelingType = type;
        resetSelectionState();
        activatePickTarget(
            defaultPickTarget(),
            false);
    }

    mGeoDirectModelingPanel::PickTarget
        mGeoDirectModelingPanel::defaultPickTarget() const
    {
        if (m_currentModelingType == DirectModelingType::ReplaceFace)
            return PickTarget::ReplaceSourceFace;
        if (m_currentModelingType == DirectModelingType::RotateFace)
            return PickTarget::RotateSourceFace;
        return PickTarget::OperationSelection;
    }

    void mGeoDirectModelingPanel::setPickType(const QString& type)
    {
        if (!geoPickWidget || !geoPickWidget->pickObjTypeCombo)
            return;

        geoPickWidget->_featPick = None;
        geoPickWidget->pickObjTypeCombo->setCurrentText(type);
        geoPickWidget->strPickType = type;
        geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        geoPickWidget->_featPick = DirectModel;
    }

    void mGeoDirectModelingPanel::activatePickTarget(
        PickTarget target,
        bool resetSelection)
    {
        // resetSelection=true 表示用户主动重新点某个拾取按钮。已有几何会话先取消，
        // 防止主面已改变但后台仍使用旧目标面或旧旋转轴。
        if (resetSelection && m_controller && m_controller->hasActiveOperation())
            m_controller->onCancel();

        m_activePickTarget = target;
        if (target == PickTarget::ReplaceSourceFace)
        {
            if (resetSelection)
            {
                m_replaceSourceFaceId = -1;
                m_replaceTargetFaceId = -1;
                if (m_replaceSourcePickEdit)
                    m_replaceSourcePickEdit->clear();
                if (m_replaceTargetPickEdit)
                    m_replaceTargetPickEdit->clear();
                if (m_replaceOffsetEdit)
                    m_replaceOffsetEdit->setText(QStringLiteral("0"));
                m_lastDistance = 0.0;
                m_dragBaseParameter = 0.0;
                m_dragInProgress = false;
            }
            setPickType(tr("几何面"));
            return;
        }

        if (target == PickTarget::ReplaceTargetFace)
        {
            if (m_replaceSourceFaceId < 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("请先拾取替换面的主面"));
                m_activePickTarget = PickTarget::ReplaceSourceFace;
            }
            else if (resetSelection)
            {
                m_replaceTargetFaceId = -1;
                if (m_replaceTargetPickEdit)
                    m_replaceTargetPickEdit->clear();
            }
            setPickType(tr("几何面"));
            return;
        }

        if (target == PickTarget::RotateSourceFace)
        {
            if (resetSelection)
            {
                if (geoPickWidget)
                    geoPickWidget->_featPick = None;
                clearAllHighlights();
                m_rotateSourceFaceId = -1;
                m_rotateAxisEdgeId = -1;
                m_lastDistance = 0.0;
                m_dragBaseParameter = 0.0;
                m_dragInProgress = false;
                if (m_rotateSourcePickEdit)
                    m_rotateSourcePickEdit->clear();
                if (m_rotateAxisPickEdit)
                    m_rotateAxisPickEdit->clear();
                if (m_rotateAngleEdit)
                    m_rotateAngleEdit->setText(QStringLiteral("0"));
            }
            setPickType(tr("几何面"));
            return;
        }

        if (target == PickTarget::RotateAxisEdge)
        {
            if (m_rotateSourceFaceId < 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("请先拾取需要旋转的几何面"));
                m_activePickTarget = PickTarget::RotateSourceFace;
                setPickType(tr("几何面"));
                return;
            }
            if (resetSelection)
            {
                m_rotateAxisEdgeId = -1;
                if (m_rotateAxisPickEdit)
                    m_rotateAxisPickEdit->clear();
            }
            setPickType(tr("几何线"));
            return;
        }

        if (target == PickTarget::OperationSelection)
        {
            if (resetSelection)
            {
                m_rotateSourceFaceId = -1;
                m_rotateAxisEdgeId = -1;
                m_operationElementId = -1;
                m_operationElementDim = -1;
                m_lastDistance = 0.0;
                m_dragBaseParameter = 0.0;
                m_dragInProgress = false;
                if (QLineEdit* editor = currentParameterEdit())
                    editor->setText(QStringLiteral("0"));
                QLineEdit* pickEdit = nullptr;
                if (m_currentModelingType == DirectModelingType::OffsetFace)
                    pickEdit = m_offsetPickEdit;
                else if (m_currentModelingType == DirectModelingType::ResizeFillet)
                    pickEdit = m_filletPickEdit;
                if (pickEdit)
                    pickEdit->clear();
            }
            setPickType(
                (m_currentModelingType == DirectModelingType::ResizeFillet
                    && m_filletPickTypeCombo
                    && m_filletPickTypeCombo->currentIndex() == 1)
                ? tr("几何线") : tr("几何面"));
        }
    }

    void mGeoDirectModelingPanel::resetSelectionState()
    {
        if (m_replacePreviewTimer)
            m_replacePreviewTimer->stop();
        if (m_rotatePreviewTimer)
            m_rotatePreviewTimer->stop();
        m_replaceSourceFaceId = -1;
        m_replaceTargetFaceId = -1;
        m_rotateSourceFaceId = -1;
        m_rotateAxisEdgeId = -1;
        m_operationElementId = -1;
        m_operationElementDim = -1;
        m_lastDistance = 0.0;
        m_dragBaseParameter = 0.0;
        m_dragInProgress = false;

        for (QLineEdit* editor : {
            m_replaceSourcePickEdit, m_replaceTargetPickEdit,
            m_rotateSourcePickEdit, m_rotateAxisPickEdit,
            m_offsetPickEdit, m_filletPickEdit })
        {
            if (editor)
                editor->clear();
        }
        if (m_offsetDistanceEdit)
            m_offsetDistanceEdit->setText(QStringLiteral("0"));
        if (m_moveSolidCheckBox)
        {
            // resetSelectionState 用于功能切换、普通取消、成功提交和模式退出。
            // 阻断 toggled，避免清理过程中再次触发“请重新拾取”的交互逻辑。
            const QSignalBlocker blocker(m_moveSolidCheckBox);
            m_moveSolidCheckBox->setChecked(false);
        }
        if (m_replaceOffsetEdit)
            m_replaceOffsetEdit->setText(QStringLiteral("0"));
        if (m_filletRadiusEdit)
            m_filletRadiusEdit->setText(QStringLiteral("0"));
        if (m_rotateAngleEdit)
            m_rotateAngleEdit->setText(QStringLiteral("0"));
    }

    void mGeoDirectModelingPanel::initDirectModelingPanel()
    {
        // 基础拾取控件继续作为拾取过滤器使用，但不再作为独立折叠块显示。
        QToolButton* originalPickButton = geoPickWidget->findChild<QToolButton*>();
        geoPickWidget->setVisible(false);

        // 每个功能块使用独立只读框，但复用原拾取 ToolButton 的 icon、尺寸和样式，
        // 保持与项目其他几何面板一致；按钮只切换 PickTarget，不直接读取选择。
        auto addPickRow = [this, originalPickButton](
            FoldSection* fold,
            const QString& label,
            QLineEdit*& editor,
            QToolButton*& button) {
                editor = new QLineEdit(this);
                editor->setReadOnly(true);
                editor->setPlaceholderText(tr("未选择"));
                button = new QToolButton(this);
                button->setToolTip(tr("拾取"));
                button->setAccessibleName(tr("拾取"));
                button->setToolButtonStyle(Qt::ToolButtonIconOnly);
                if (originalPickButton)
                {
                    button->setIcon(originalPickButton->icon());
                    button->setIconSize(originalPickButton->iconSize());
                    button->setAutoRaise(originalPickButton->autoRaise());
                    button->setStyleSheet(originalPickButton->styleSheet());
                    button->setMinimumSize(originalPickButton->minimumSize());
                    button->setMaximumSize(originalPickButton->maximumSize());
                }
                QHBoxLayout* layout = new QHBoxLayout();
                layout->addWidget(new QLabel(label, this));
                layout->addWidget(editor, 1);
                layout->addWidget(button);
                fold->addRow(layout);
            };

        QToolButton* replaceSourcePickButton = nullptr;
        QToolButton* replaceTargetPickButton = nullptr;
        QToolButton* rotateSourcePickButton = nullptr;
        QToolButton* rotateAxisPickButton = nullptr;
        QToolButton* offsetPickButton = nullptr;
        QToolButton* filletPickButton = nullptr;

        // 替换面提供两个独立拾取入口，避免依赖不明确的选择先后顺序。
        FoldSection* replaceFaceFold = new FoldSection(tr("替换面"), this);
        addPickRow(replaceFaceFold, tr("主面"),
            m_replaceSourcePickEdit, replaceSourcePickButton);
        addPickRow(replaceFaceFold, tr("目标面"),
            m_replaceTargetPickEdit, replaceTargetPickButton);
        m_replaceOffsetEdit = new QLineEdit(QStringLiteral("0"), this);
        m_replaceOffsetEdit->setToolTip(
            tr("输入数值或公式（例如 ={高度}-{间隙}）；正值增加高度，负值减少高度。"));
        QHBoxLayout* replaceOffsetLayout = new QHBoxLayout();
        replaceOffsetLayout->addWidget(new QLabel(tr("偏置距离"), this));
        replaceOffsetLayout->addWidget(m_replaceOffsetEdit);
        replaceFaceFold->addRow(replaceOffsetLayout);
        QPushButton* replaceCancelButton = new QPushButton(tr("取消"), this);
        QPushButton* replaceConfirmButton = new QPushButton(tr("确定"), this);
        QHBoxLayout* replaceButtonLayout = new QHBoxLayout();
        replaceButtonLayout->addWidget(replaceCancelButton);
        replaceButtonLayout->addWidget(replaceConfirmButton);
        replaceFaceFold->addRow(replaceButtonLayout);
        m_FoldSections.append(replaceFaceFold);
        m_foldWidgets.append(replaceFaceFold);

        // 旋转面默认使用面中心轴；第二个拾取框用于可选的直线轴覆盖。
        FoldSection* rotateFaceFold = new FoldSection(tr("旋转面"), this);
        addPickRow(rotateFaceFold, tr("旋转面"),
            m_rotateSourcePickEdit, rotateSourcePickButton);
        addPickRow(rotateFaceFold, tr("旋转轴"),
            m_rotateAxisPickEdit, rotateAxisPickButton);
        m_rotateAngleEdit = new QLineEdit(QStringLiteral("0"), this);
        m_rotateAngleEdit->setToolTip(
            tr("拖动旋转圆环，或输入数值/公式；角度限制为 -90° 到 90°，仅点击确定修改几何。"));
        QHBoxLayout* rotateValueLayout = new QHBoxLayout();
        rotateValueLayout->addWidget(new QLabel(tr("旋转角度"), this));
        rotateValueLayout->addWidget(m_rotateAngleEdit);
        rotateFaceFold->addRow(rotateValueLayout);
        QPushButton* rotateCancelButton = new QPushButton(tr("取消"), this);
        QPushButton* rotateConfirmButton = new QPushButton(tr("确定"), this);
        QHBoxLayout* rotateButtonLayout = new QHBoxLayout();
        rotateButtonLayout->addWidget(rotateCancelButton);
        rotateButtonLayout->addWidget(rotateConfirmButton);
        rotateFaceFold->addRow(rotateButtonLayout);
        m_FoldSections.append(rotateFaceFold);
        m_foldWidgets.append(rotateFaceFold);

        // 移动面。“偏置”只改变移动对象的范围，方向仍由所选面的法向确定。
        FoldSection* offsetFaceFold = new FoldSection(tr("移动面"), this);
        addPickRow(offsetFaceFold, tr("移动面"), m_offsetPickEdit, offsetPickButton);
        m_offsetDistanceEdit = new QLineEdit(QStringLiteral("0"), this);
        m_offsetDistanceEdit->setToolTip(
            tr("输入数值或公式（例如 ={基础高度}+{增量}），按回车仅更新预览。"));
        QHBoxLayout* offsetValueLayout = new QHBoxLayout();
        offsetValueLayout->addWidget(new QLabel(tr("移动距离"), this));
        offsetValueLayout->addWidget(m_offsetDistanceEdit);
        offsetFaceFold->addRow(offsetValueLayout);
        // “偏置”作为移动距离的作用域选项，放在距离输入的下一行。
        m_moveSolidCheckBox = new QCheckBox(tr("偏置"), this);
        m_moveSolidCheckBox->setChecked(false);
        m_moveSolidCheckBox->setToolTip(
            tr("勾选后，沿所选面的方向平移包含该面的整个实体；默认只移动所选面。"));
        QHBoxLayout* moveSolidLayout = new QHBoxLayout();
        moveSolidLayout->addWidget(m_moveSolidCheckBox);
        moveSolidLayout->addStretch();
        offsetFaceFold->addRow(moveSolidLayout);
        QPushButton* offsetCancelButton = new QPushButton(tr("取消"), this);
        QPushButton* offsetConfirmButton = new QPushButton(tr("确定"), this);
        QHBoxLayout* offsetButtonLayout = new QHBoxLayout();
        offsetButtonLayout->addWidget(offsetCancelButton);
        offsetButtonLayout->addWidget(offsetConfirmButton);
        offsetFaceFold->addRow(offsetButtonLayout);
        m_FoldSections.append(offsetFaceFold);
        m_foldWidgets.append(offsetFaceFold);

        // 调整圆角大小。
        FoldSection* resizeFilletFold = new FoldSection(tr("调整圆角大小"), this);
        m_filletPickTypeCombo = new QComboBox(this);
        m_filletPickTypeCombo->addItems({ tr("几何面"), tr("几何线") });
        QHBoxLayout* filletPickTypeLayout = new QHBoxLayout();
        filletPickTypeLayout->addWidget(new QLabel(tr("拾取类型"), this));
        filletPickTypeLayout->addWidget(m_filletPickTypeCombo, 1);
        resizeFilletFold->addRow(filletPickTypeLayout);
        addPickRow(resizeFilletFold, tr("圆角对象"), m_filletPickEdit, filletPickButton);
        m_filletRadiusEdit = new QLineEdit(QStringLiteral("0"), this);
        m_filletRadiusEdit->setToolTip(
            tr("选择圆角面或共享边后输入目标半径数值/公式；回车仅更新预览。"));
        QHBoxLayout* filletValueLayout = new QHBoxLayout();
        filletValueLayout->addWidget(new QLabel(tr("目标半径"), this));
        filletValueLayout->addWidget(m_filletRadiusEdit);
        resizeFilletFold->addRow(filletValueLayout);
        QPushButton* filletCancelButton = new QPushButton(tr("取消"), this);
        QPushButton* filletConfirmButton = new QPushButton(tr("确定"), this);
        QHBoxLayout* filletButtonLayout = new QHBoxLayout();
        filletButtonLayout->addWidget(filletCancelButton);
        filletButtonLayout->addWidget(filletConfirmButton);
        resizeFilletFold->addRow(filletButtonLayout);
        m_FoldSections.append(resizeFilletFold);
        m_foldWidgets.append(resizeFilletFold);

        // 全局参数驱动表。参数仍由“确定”操作自动生成，但生成后的显示名称和公式
        // 都允许用户修改；参数的稳定身份由名称单元格 UserRole 中的内部 ID 保持。
        // 当前值和状态由 Controller 中的参数计算器统一计算后回填。
        // 参数表与具体拾取功能解耦，因此切换替换面/旋转面等操作时不会被清空。
        FoldSection* parameterFold = new FoldSection(tr("全局参数驱动"), this);
        m_parameterTable = new QTableWidget(0, 4, this);
        m_parameterTable->setHorizontalHeaderLabels({
            tr("参数名"), tr("公式"), tr("当前值"), tr("状态") });
        m_parameterTable->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_parameterTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_parameterTable->setAlternatingRowColors(true);
        m_parameterTable->verticalHeader()->setVisible(false);
        m_parameterTable->horizontalHeader()->setSectionResizeMode(
            kParameterNameColumn, QHeaderView::ResizeToContents);
        m_parameterTable->horizontalHeader()->setSectionResizeMode(
            kParameterExpressionColumn, QHeaderView::Stretch);
        m_parameterTable->horizontalHeader()->setSectionResizeMode(
            kParameterValueColumn, QHeaderView::ResizeToContents);
        m_parameterTable->horizontalHeader()->setSectionResizeMode(
            kParameterStatusColumn, QHeaderView::ResizeToContents);
        m_parameterTable->setMinimumHeight(180);
        m_parameterTable->setToolTip(tr(
            "公式引用参数时必须使用花括号，例如 {参数A}；支持 + - * / % ^、比较/逻辑运算、三目运算，以及 "
            "sin/cos/tan、sind/cosd/tand、sqrt、abs、min/max、pow、if、clamp 等函数。"));
        //【参数删除】右键删除参数及其驱动步骤（同部件后续依赖步骤一并删除）。
        m_parameterTable->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_parameterTable, &QTableWidget::customContextMenuRequested,
            this, [this](const QPoint& pos) {
                if (!m_controller || !m_parameterTable)
                {
                    return;
                }
                const QModelIndex index =
                    m_parameterTable->indexAt(pos);
                if (!index.isValid())
                {
                    return;
                }
                QTableWidgetItem* nameItem =
                    m_parameterTable->item(index.row(), kParameterNameColumn);
                if (nameItem == nullptr)
                {
                    return;
                }
                const QString parameterId =
                    nameItem->data(Qt::UserRole).toString();
                const QString parameterName = nameItem->text();
                if (parameterId.isEmpty())
                {
                    return;
                }
                mUiMenu menu(this);
                QAction* removeAction = menu.addAction(
                    tr("删除参数“%1”及依赖步骤").arg(parameterName));
                connect(removeAction, &QAction::triggered, this,
                    [this, parameterId, parameterName]() {
                        QString error;
                        if (!m_controller->deleteDrivenParameter(
                                parameterId, &error))
                        {
                            MxFunctions::mGlobalSignals::getInstance()
                                ->outputMessageSig(2,
                                    tr("删除参数失败：%1").arg(error));
                        }
                    });
                menu.exec(m_parameterTable->viewport()->mapToGlobal(pos));
            });

        QVBoxLayout* parameterTableLayout = new QVBoxLayout();
        /*QLabel* parameterHelp = new QLabel(
            tr("参数由每次直接建模“确定”自动创建，生成后可修改名称。公式引用其他参数时必须写成 {参数名}；函数和 pi/e/true/false 等内置常量仍保持原有写法。"),
            this);*/
        //parameterHelp->setWordWrap(true);
        //parameterTableLayout->addWidget(parameterHelp);
        parameterTableLayout->addWidget(m_parameterTable);
        parameterFold->addRow(parameterTableLayout);

        QPushButton* applyParameterButton = new QPushButton(tr("应用参数"), this);
        QHBoxLayout* parameterButtonLayout = new QHBoxLayout();
        parameterButtonLayout->addStretch();
        parameterButtonLayout->addWidget(applyParameterButton);
        parameterFold->addRow(parameterButtonLayout);
        m_FoldSections.append(parameterFold);
        m_foldWidgets.append(parameterFold);

        // 连续编辑参数名称或公式时只提交最后一次内容，避免短时间内重复计算依赖关系
        // 和触发几何精确预览。
        m_parameterApplyTimer = new QTimer(this);
        m_parameterApplyTimer->setSingleShot(true);
        m_parameterApplyTimer->setInterval(300);
        connect(m_parameterApplyTimer, &QTimer::timeout,
            this, [this]() { applyParameterTable(false); });
        connect(m_parameterTable, &QTableWidget::cellChanged,
            this, [this](int, int column) {
                if (!m_refreshingParameterTable
                    && (column == kParameterNameColumn
                        || column == kParameterExpressionColumn))
                {
                    m_parameterApplyTimer->start();
                }
            });
        connect(applyParameterButton, &QPushButton::clicked,
            this, [this]() { applyParameterTable(true); });
        // directModelingParametersChanged 由 connectGeoSignals 在进入面板时统一连接
        refreshParameterTable();

        m_replacePreviewTimer = new QTimer(this);
        m_replacePreviewTimer->setSingleShot(true);
        m_replacePreviewTimer->setInterval(180);
        connect(m_replacePreviewTimer, &QTimer::timeout,
            this, &mGeoDirectModelingPanel::slot_distanceEditingFinished);
        connect(m_replaceOffsetEdit, &QLineEdit::textEdited,
            this, [this](const QString&) {
                if (m_currentModelingType == DirectModelingType::ReplaceFace
                    && m_controller && m_controller->hasActiveOperation())
                {
                    m_replacePreviewTimer->start();
                }
            });
        connect(m_replaceOffsetEdit, &QLineEdit::editingFinished,
            this, [this]() {
                m_replacePreviewTimer->stop();
                slot_distanceEditingFinished();
            });
        m_rotatePreviewTimer = new QTimer(this);
        m_rotatePreviewTimer->setSingleShot(true);
        m_rotatePreviewTimer->setInterval(180);
        connect(m_rotatePreviewTimer, &QTimer::timeout,
            this, &mGeoDirectModelingPanel::slot_distanceEditingFinished);
        connect(m_rotateAngleEdit, &QLineEdit::textEdited,
            this, [this](const QString&) {
                if (m_currentModelingType == DirectModelingType::RotateFace
                    && m_controller && m_controller->hasActiveOperation())
                {
                    m_rotatePreviewTimer->start();
                }
            });
        connect(m_rotateAngleEdit, &QLineEdit::editingFinished,
            this, [this]() {
                m_rotatePreviewTimer->stop();
                slot_distanceEditingFinished();
            });
        connect(m_offsetDistanceEdit, &QLineEdit::returnPressed,
            this, &mGeoDirectModelingPanel::slot_distanceEditingFinished);
        connect(m_filletRadiusEdit, &QLineEdit::returnPressed,
            this, &mGeoDirectModelingPanel::slot_distanceEditingFinished);
        connect(replaceSourcePickButton, &QToolButton::clicked,
            this, [this]() { activatePickTarget(PickTarget::ReplaceSourceFace); });
        connect(replaceTargetPickButton, &QToolButton::clicked,
            this, [this]() { activatePickTarget(PickTarget::ReplaceTargetFace); });
        connect(rotateSourcePickButton, &QToolButton::clicked,
            this, [this]() { activatePickTarget(PickTarget::RotateSourceFace); });
        connect(rotateAxisPickButton, &QToolButton::clicked,
            this, [this]() { activatePickTarget(PickTarget::RotateAxisEdge); });
        connect(offsetPickButton, &QToolButton::clicked,
            this, [this]() { activatePickTarget(PickTarget::OperationSelection); });
        connect(m_moveSolidCheckBox, &QCheckBox::toggled,
            this, [this](bool checked) {
                if (m_currentModelingType != DirectModelingType::OffsetFace
                    || !m_controller || !m_controller->hasActiveOperation())
                {
                    return;
                }

                // 已经拾取面时，切换作用域必须用同一个面重新建立 Controller 快照，
                // 并重建适配器的源顶点。不能沿用单面扫掠缓存，否则无法显示实体平移。
                const int faceId = m_operationElementId;
                const int faceDim = m_operationElementDim;
                const bool previousMode = m_controller->moveSolidEnabled();
                if (faceId < 0 || faceDim != 2 || previousMode == checked)
                    return;

                auto* geoData = getCVM && getCVM->getModelData()
                    && getCVM->getModelData()->getGeoManager()
                    ? getCVM->getModelData()->getGeoManager()->getGeoModelBase()
                    : nullptr;
                //【OCC摘除·S9】旧栈 set 反查退役，改 GN 部件 ID
                const int partId0 = MGeoNeutralQuery::partIdByGeoId(
                    getCVM->getModelData()->getGeoManager(), 2, faceId);
                const bool switched = partId0 >= 0
                    && m_controller->beginOperation(
                        2,
                        faceId,
                        MGeoCommand::DirectModelingOperation::OffsetFace,
                        checked)
                    && configureCurrentRenderOperation(
                        partId0, faceId, 2, true);

                if (!switched)
                {
                    // 切换失败时尽量恢复原作用域，并阻断 setChecked 触发递归 toggled。
                    bool restored = false;
                    if (partId0 >= 0)
                    {
                        restored = m_controller->beginOperation(
                            2,
                            faceId,
                            MGeoCommand::DirectModelingOperation::OffsetFace,
                            previousMode)
                            && configureCurrentRenderOperation(
                                partId0, faceId, 2, true);
                    }
                    const QSignalBlocker blocker(m_moveSolidCheckBox);
                    m_moveSolidCheckBox->setChecked(previousMode);
                    if (!restored)
                    {
                        m_controller->onCancel();
                        m_operationElementId = -1;
                        m_operationElementDim = -1;
                        if (m_offsetPickEdit)
                            m_offsetPickEdit->clear();
                        activatePickTarget(PickTarget::OperationSelection, false);
                    }
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        2,
                        restored
                        ? tr("无法切换偏置作用域，已恢复原来的预览方式")
                        : tr("无法切换偏置作用域，请重新拾取几何面"));
                    return;
                }

                // 保留当前累计距离。常见的“先拾面再勾选”无需重新拾取，下一次拖动
                // 会直接使用完整实体（或单面）的新缓存。
                m_dragBaseParameter = m_lastDistance;
                m_dragInProgress = false;
                if (std::abs(m_lastDistance) > 1.0e-12)
                    updateCurrentPreview(m_lastDistance, true);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    0,
                    checked
                    ? tr("已启用偏置模式，拖动时将预览所选面所属的整个实体")
                    : tr("已关闭偏置模式，拖动时将只预览所选面"));
            });
        connect(filletPickButton, &QToolButton::clicked,
            this, [this]() { activatePickTarget(PickTarget::OperationSelection); });
        connect(m_filletPickTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (m_currentModelingType == DirectModelingType::ResizeFillet)
                {
                    if (m_controller && m_controller->hasActiveOperation())
                        m_controller->onCancel();
                    m_activePickTarget = PickTarget::None;
                    if (geoPickWidget)
                        geoPickWidget->_featPick = None;
                    clearAllHighlights();
                    activatePickTarget(PickTarget::OperationSelection);
                }
            });
        connect(replaceCancelButton, &QPushButton::clicked,
            this, &mGeoDirectModelingPanel::slot_cancelClicked);
        connect(offsetCancelButton, &QPushButton::clicked,
            this, &mGeoDirectModelingPanel::slot_cancelClicked);
        connect(rotateCancelButton, &QPushButton::clicked,
            this, &mGeoDirectModelingPanel::slot_cancelClicked);
        connect(filletCancelButton, &QPushButton::clicked,
            this, &mGeoDirectModelingPanel::slot_cancelClicked);
        connect(replaceConfirmButton, &QPushButton::clicked,
            this, &mGeoDirectModelingPanel::slot_replaceFaceConfirmClicked);
        connect(rotateConfirmButton, &QPushButton::clicked,
            this, &mGeoDirectModelingPanel::slot_rotateFaceConfirmClicked);
        connect(offsetConfirmButton, &QPushButton::clicked,
            this, &mGeoDirectModelingPanel::slot_offsetFaceConfirmClicked);
        connect(filletConfirmButton, &QPushButton::clicked,
            this, &mGeoDirectModelingPanel::slot_resizeFilletConfirmClicked);

        geoPickWidget->pickObjTypeCombo->clear();
        geoPickWidget->pickObjTypeCombo->addItems({ tr("几何面"), tr("几何线") });
        geoPickWidget->strPickType = tr("几何面");
        setDirectModelingType(DirectModelingType::OffsetFace);
    }

    void mGeoDirectModelingPanel::slot_distanceEditingFinished()
    {
        // editingFinished 可能由回车或失焦触发。无论来源都只同步累计值并请求预览，
        // 不能在这里调用 onCommit，否则用户切换输入框也会意外修改正式几何。
        if (!m_controller || !m_controller->isActive() || !m_controller->hasActiveOperation())
            return;

        QLineEdit* editor = currentParameterEdit();
        if (!editor)
            return;
        if (QLineEdit* sourceEditor = qobject_cast<QLineEdit*>(sender()))
        {
            if (sourceEditor != editor)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("请先激活该参数所属的同步建模操作"));
                return;
            }
        }

        double distance = 0.0;
        QString expressionError;
        if (!evaluateParameterEditor(editor, distance, &expressionError))
        {
            editor->setToolTip(expressionError);
            return;
        }
        if (m_currentModelingType == DirectModelingType::RotateFace)
        {
            if (std::abs(distance) > 90.0)
            {
                editor->setToolTip(tr("公式计算结果必须在 -90° 到 90° 之间"));
                return;
            }
        }

        // 参数输入和回车都只更新预览；几何修改只能由“确定”按钮触发。
        m_lastDistance = distance;
        m_dragBaseParameter = distance;
        m_dragInProgress = false;
        // 渲染侧先立即显示变换结果，随后只请求一次精确 OCC 预览。
        updateCurrentPreview(distance, true);
    }

    void mGeoDirectModelingPanel::updateCurrentPreview(double accumulatedValue, bool finished)
    {
        if (!m_controller || !m_controller->isActive() || !m_controller->hasActiveOperation())
            return;
        // 先排队显示当前累计值；如果 Controller 同步返回精确数据，后者具有更高优先级。
        // 普通面也使用该累计值覆盖 Tool 的内部累计缓存，支持手输距离后继续拖动。
        if (m_renderAdapter)
            m_renderAdapter->requestPreview(accumulatedValue);
        //【ExactOnly 预览】替换面/圆角族参数变化时请求引擎"编辑不落盘"预览
        //（结果体离散网格直发渲染；移动/旋转族由矩阵通道即时预览不走此路）。
        {
            const auto operation = m_controller->currentOperation();
            if (operation == MGeoCommand::DirectModelingOperation::ReplaceFace
                || operation == MGeoCommand::DirectModelingOperation::ResizeFillet
                || operation == MGeoCommand::DirectModelingOperation::EdgeFillet)
            {
                m_controller->requestExactPreview(accumulatedValue);
            }
        }
        if (m_controller->moveSolidEnabled())
        {
            // 刚体平移不需要后台布尔或重新离散。onDragValue 仅同步累计参数并使旧结果
            // 失效，真正绘制在 Tool 的当前鼠标回调结束后进行，避免被它随后清屏覆盖。
            m_controller->onDragValue(accumulatedValue);
            return;
        }
        if (finished)
            m_controller->onDragFinished(accumulatedValue);
        else
            m_controller->onDragValue(accumulatedValue);
    }

    void mGeoDirectModelingPanel::slot_cancelClicked()
    {
        if (m_controller && m_controller->isActive())
            m_controller->onCancel();

        m_activePickTarget = PickTarget::None;
        if (geoPickWidget)
            geoPickWidget->_featPick = None;
        clearAllHighlights();
        resetSelectionState();
        activatePickTarget(
            defaultPickTarget(),
            false);
    }

    void mGeoDirectModelingPanel::exitDirectModelingSession()
    {
        //【直模退出机制】ESC 清拾取等外部退出统一入口：与"取消"按钮同语义，
        // 面板保持打开、拾取过滤复位到当前功能块，可直接重新拾取。
        slot_cancelClicked();
    }

    void mGeoDirectModelingPanel::hideEvent(QHideEvent* event)
    {
        mGeoPanelBase::hideEvent(event);
        //【直模退出机制】mInfoWindow::setMainWidget 换主件（面板切换）与
        // Info 停靠窗关闭都以 hide 为边界（setMainWidget 显式 hide 旧面板，
        // 本工程以此替代独立的面板退出事件）。此处完整退出直模模式：
        // 清预览/箭头（exitDirectModelingMode → resetSession/arrowHidden）、
        // 经 modeChanged(false) 复位面板状态，再断开全部全局信号并卸下
        // DirectTool，避免箭头/预览/交互残留到其他面板会话。
        if (m_controller && m_controller->isActive())
        {
            m_controller->exitDirectModelingMode();
            disconnectGeoSignals();
            if (getCVM && getCVM->getPreViewportWidget()
                && getCVM->getPreViewportWidget()->interaction())
            {
                auto* manager = getCVM->getPreViewportWidget()
                    ->interaction()->getToolManager();
                if (manager && manager->active()
                    && manager->active()->type()
                        == mxr::InteractionToolType::DirectGeoModeling)
                {
                    manager->setActive(mxr::InteractionToolType::None);
                }
            }
        }
    }

    QLineEdit* mGeoDirectModelingPanel::currentParameterEdit() const
    {
        if (m_currentModelingType == DirectModelingType::ReplaceFace)
            return m_replaceOffsetEdit;
        if (m_currentModelingType == DirectModelingType::OffsetFace)
            return m_offsetDistanceEdit;
        if (m_currentModelingType == DirectModelingType::ResizeFillet)
            return m_filletRadiusEdit;
        if (m_currentModelingType == DirectModelingType::RotateFace)
            return m_rotateAngleEdit;
        return nullptr;
    }

    bool mGeoDirectModelingPanel::evaluateParameterEditor(
        QLineEdit* editor,
        double& value,
        QString* errorMessage) const
    {
        if (!editor)
        {
            if (errorMessage)
                *errorMessage = tr("参数输入框无效");
            return false;
        }

        const QString text = editor->text().trimmed();
        bool numeric = false;
        value = text.toDouble(&numeric);
        if (numeric && std::isfinite(value))
        {
            if (errorMessage)
                errorMessage->clear();
            return true;
        }

        if (!m_controller)
        {
            if (errorMessage)
                *errorMessage = tr("全局参数控制器不可用");
            return false;
        }
        return m_controller->evaluateDirectModelingExpression(text, value, errorMessage);
    }

    bool mGeoDirectModelingPanel::applyParameterTable(bool showMessage)
    {
        if (!m_controller || !m_parameterTable || m_refreshingParameterTable)
            return false;
        if (m_parameterApplyTimer)
            m_parameterApplyTimer->stop();

        // 记录提交前的名称，用稳定 id 与提交后的快照比较。若当前还有尚未确认的
        // 操作公式，也要同步其中的完整花括号引用，避免参数表改名后预览立即失效。
        const QVector<GeoFeatureParameter> parametersBefore =
            m_controller->directModelingParameters();
        QHash<QString, QString> oldNameById;
        QHash<QString, QString> oldExpressionById;
        for (const GeoFeatureParameter& parameter : parametersBefore)
        {
            oldNameById.insert(parameter.id, parameter.name);
            oldExpressionById.insert(parameter.id, parameter.expression);
        }

        QHash<QString, QString> nameById;
        QHash<QString, QString> expressionById;
        for (int row = 0; row < m_parameterTable->rowCount(); ++row)
        {
            QTableWidgetItem* nameItem =
                m_parameterTable->item(row, kParameterNameColumn);
            QTableWidgetItem* expressionItem =
                m_parameterTable->item(row, kParameterExpressionColumn);
            const QString parameterId = nameItem
                ? nameItem->data(Qt::UserRole).toString() : QString();
            if (parameterId.isEmpty() || !nameItem || !expressionItem)
                continue;
            nameById.insert(parameterId, nameItem->text());
            // 只提交用户实际改动的公式。这样同一次改名中的未编辑公式由计算器
            // 从旧名称空间迁移，而新输入公式可直接使用表格中的最终参数名。
            const auto oldExpression = oldExpressionById.constFind(parameterId);
            if (oldExpression == oldExpressionById.constEnd()
                || oldExpression.value() != expressionItem->text())
            {
                expressionById.insert(parameterId, expressionItem->text());
            }
        }

        QString errorMessage;
        const bool success = m_controller->updateDirectModelingParameters(
            nameById, expressionById, &errorMessage);

        QHash<QString, QString> newNameByOldNormalizedName;
        for (const GeoFeatureParameter& parameter
            : m_controller->directModelingParameters())
        {
            const auto oldName = oldNameById.constFind(parameter.id);
            if (oldName != oldNameById.constEnd()
                && oldName.value() != parameter.name)
            {
                newNameByOldNormalizedName.insert(
                    oldName.value().trimmed().toCaseFolded(), parameter.name);
            }
        }
        if (!newNameByOldNormalizedName.isEmpty())
        {
            for (QLineEdit* editor : {
                m_replaceOffsetEdit,
                m_rotateAngleEdit,
                m_offsetDistanceEdit,
                m_filletRadiusEdit })
            {
                if (!editor)
                    continue;
                const QString rewritten = rewriteBracedParameterNames(
                    editor->text(), newNameByOldNormalizedName);
                if (rewritten != editor->text())
                    editor->setText(rewritten);
            }
            updatePreviewFromDrivenExpression();
        }
        if (showMessage)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                success ? 0 : 2,
                success ? tr("全局参数更新完成")
                : tr("全局参数存在错误：%1").arg(errorMessage));
        }
        return success;
    }

    void mGeoDirectModelingPanel::refreshParameterTable()
    {
        if (!m_controller || !m_parameterTable)
            return;
        //【部件校验】刷新前清理已删除部件的参数（面板打开/参数变化时触发）。
        m_controller->validateDrivenParameters();

        m_refreshingParameterTable = true;
        QSignalBlocker blocker(m_parameterTable);
        const QVector<GeoFeatureParameter> parameters =
            m_controller->directModelingParameters();
        m_parameterTable->setRowCount(parameters.size());
        for (int row = 0; row < parameters.size(); ++row)
        {
            const GeoFeatureParameter& parameter = parameters.at(row);
            auto ensureItem = [&](int column) {
                QTableWidgetItem* item = m_parameterTable->item(row, column);
                if (!item)
                {
                    item = new QTableWidgetItem();
                    m_parameterTable->setItem(row, column, item);
                }
                return item;
                };
            QTableWidgetItem* nameItem = ensureItem(kParameterNameColumn);
            QTableWidgetItem* expressionItem = ensureItem(kParameterExpressionColumn);
            QTableWidgetItem* valueItem = ensureItem(kParameterValueColumn);
            QTableWidgetItem* statusItem = ensureItem(kParameterStatusColumn);
            nameItem->setText(parameter.name);
            nameItem->setData(Qt::UserRole, parameter.id);
            nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
            nameItem->setToolTip(tr(
                "由“%1”确定操作自动创建；可修改名称，公式中请使用 {%2} 引用")
                .arg(parameter.operationName, parameter.name));
            expressionItem->setText(parameter.expression);
            expressionItem->setToolTip(tr(
                "参数引用必须使用花括号，例如 {参数A}；函数和内置常量保持原有写法"));
            valueItem->setText(parameter.valid
                ? formatParameterValue(parameter.value) : QStringLiteral("—"));
            statusItem->setText(parameter.valid ? tr("正常") : parameter.errorMessage);
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
            statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
            statusItem->setForeground(parameter.valid
                ? QColor(35, 140, 75) : QColor(190, 55, 55));
            statusItem->setToolTip(statusItem->text());
        }
        m_refreshingParameterTable = false;
    }

    void mGeoDirectModelingPanel::updatePreviewFromDrivenExpression()
    {
        if (!m_controller || !m_controller->isActive()
            || !m_controller->hasActiveOperation())
        {
            return;
        }
        QLineEdit* editor = currentParameterEdit();
        if (!editor)
            return;
        const QString validToolTip = m_currentModelingType
            == DirectModelingType::RotateFace
            ? tr("输入数值或公式；参数引用必须使用 {参数名}，计算结果必须在 -90° 到 90°。")
            : tr("输入数值或公式；参数引用必须使用 {参数名}。");

        // 纯数值输入不依赖参数表，无需在其他参数变化时重复计算几何。
        bool isNumber = false;
        editor->text().trimmed().toDouble(&isNumber);
        if (isNumber)
        {
            editor->setToolTip(validToolTip);
            return;
        }

        double value = 0.0;
        QString errorMessage;
        if (!evaluateParameterEditor(editor, value, &errorMessage))
        {
            editor->setToolTip(errorMessage);
            return;
        }
        if (m_currentModelingType == DirectModelingType::RotateFace
            && std::abs(value) > 90.0)
        {
            editor->setToolTip(tr("公式计算结果必须在 -90° 到 90° 之间"));
            return;
        }
        editor->setToolTip(validToolTip);

        m_lastDistance = value;
        m_dragBaseParameter = value;
        m_dragInProgress = false;
        updateCurrentPreview(value, true);
    }

    bool mGeoDirectModelingPanel::commitParameterOperation(
        DirectModelingType type,
        QLineEdit* editor)
    {
        // 每个确定按钮带入自己的 type，防止隐藏折叠块的残留信号提交当前其他操作。
        if (m_currentModelingType != type)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("请先激活对应的同步建模操作并重新选择几何面"));
            return false;
        }
        if (!m_controller || !m_controller->isActive()
            || !m_controller->hasActiveOperation() || !editor)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("当前没有可确认的直接建模预览"));
            return false;
        }

        double value = 0.0;
        QString expressionError;
        if (!evaluateParameterEditor(editor, value, &expressionError))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, tr("建模参数或公式无效：%1").arg(expressionError));
            return false;
        }
        if (type == DirectModelingType::RotateFace && std::abs(value) > 90.0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, tr("旋转面角度必须在 -90° 到 90° 之间"));
            return false;
        }

        // 提交值来自编辑框，代表当前最终累计参数；Controller 会再次做有限值、
        // 零值和旋转角范围校验，形成 UI + 几何会话两层保护。
        m_lastDistance = value;
        m_dragBaseParameter = value;
        m_dragInProgress = false;
        bool isNumericInput = false;
        editor->text().trimmed().toDouble(&isNumericInput);
        if (type == DirectModelingType::OffsetFace)
        {
            const bool checkedMoveSolid = m_moveSolidCheckBox && m_moveSolidCheckBox->isChecked();
            if (checkedMoveSolid != m_controller->moveSolidEnabled())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    2, tr("偏置复选框与当前几何会话状态不一致，请重新拾取几何面"));
                return false;
            }
            if (checkedMoveSolid)
            {
                // 实体平移仅作为面板交互：直接确认正在预览的会话，不经 API 重建
                // 操作。提交期间暂停拾取，避免几何刷新同步通知重新进入旧会话。
                const PickTarget previousTarget = m_activePickTarget;
                m_activePickTarget = PickTarget::None;
                if (geoPickWidget)
                    geoPickWidget->_featPick = None;
                const bool success = m_controller->onCommit(value,
                    isNumericInput ? QString() : editor->text().trimmed());
                if (!success)
                {
                    if (m_controller->hasActiveOperation())
                    {
                        m_activePickTarget = previousTarget;
                        if (geoPickWidget)
                            geoPickWidget->_featPick = m_controller->isActive() ? DirectModel : None;
                    }
                    else
                    {
                        // 更新已有参数的回放失败时，Controller 可能已经结束旧会话。
                        // 此时不能恢复旧 ID 和“可确定”外观，必须返回可重新拾取状态。
                        m_renderAdapter->finishSession();
                        clearAllHighlights();
                        resetSelectionState();
                        if (m_controller->isActive())
                            activatePickTarget(defaultPickTarget(), false);
                    }
                }
                // 成功由 commitFinished 统一清除预览/箭头/高亮，继续保持面板可拾取。
                return success;
            }
        }
        // 普通面操作与命令行继续经过原 API 路径；实体平移已在上面直接提交当前会话。
        if (!getAPISchedulingServer)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, tr("直接建模 API 调度器未初始化"));
            return false;
        }

        APIServer::APIParams params;
        QString apiKey;
        const QString valueText = formatParameterValue(value);
        if (!isNumericInput)
            params.insert(QStringLiteral("expression"),
                QStringList({ editor->text().trimmed() }));

        if (type == DirectModelingType::ReplaceFace)
        {
            apiKey = QStringLiteral("directModelingReplaceFace");
            params.insert(QStringLiteral("sourceFaceId"),
                QStringList({ QString::number(m_replaceSourceFaceId) }));
            params.insert(QStringLiteral("targetFaceId"),
                QStringList({ QString::number(m_replaceTargetFaceId) }));
            params.insert(QStringLiteral("offset"), QStringList({ valueText }));
        }
        else if (type == DirectModelingType::RotateFace)
        {
            apiKey = QStringLiteral("directModelingRotateFace");
            params.insert(QStringLiteral("faceId"),
                QStringList({ QString::number(m_rotateSourceFaceId) }));
            params.insert(QStringLiteral("angle"), QStringList({ valueText }));
            if (m_rotateAxisEdgeId >= 0)
            {
                params.insert(QStringLiteral("axisEdgeId"),
                    QStringList({ QString::number(m_rotateAxisEdgeId) }));
            }
        }
        else if (type == DirectModelingType::OffsetFace)
        {
            apiKey = QStringLiteral("directModelingOffsetFace");
            params.insert(QStringLiteral("faceId"),
                QStringList({ QString::number(m_operationElementId) }));
            params.insert(QStringLiteral("distance"), QStringList({ valueText }));
        }
        else
        {
            apiKey = QStringLiteral("directModelingResizeFillet");
            params.insert(QStringLiteral("elementType"),
                QStringList({ m_operationElementDim == 1
                    ? QStringLiteral("edge") : QStringLiteral("face") }));
            params.insert(QStringLiteral("elementId"),
                QStringList({ QString::number(m_operationElementId) }));
            params.insert(QStringLiteral("radius"), QStringList({ valueText }));
        }

        // API 会重建 Controller 会话并清除预览配置。保留当前主选择用于失败恢复，
        // 提交期间暂停拾取，避免同步几何通知把本次选择替换成其他对象。
        const int previewElementId = type == DirectModelingType::ReplaceFace
            ? m_replaceSourceFaceId : (type == DirectModelingType::RotateFace
                ? m_rotateSourceFaceId : m_operationElementId);
        const int previewElementDim = type == DirectModelingType::ResizeFillet
            ? m_operationElementDim : 2;
        const PickTarget previousTarget = m_activePickTarget;
        m_activePickTarget = PickTarget::None;
        if (geoPickWidget)
            geoPickWidget->_featPick = None;
        const APIServer::APIExecutionResult result =
            getAPISchedulingServer->executeAPI(apiKey, params);
        if (!result._success)
        {
            bool previewRestored = false;
            if (m_controller->isActive() && m_controller->hasActiveOperation()
                && getCVM && getCVM->getModelData()
                && getCVM->getModelData()->getGeoManager())
            {
                //【OCC摘除·S9】旧栈 set 反查退役，改 GN 部件 ID
                const int partId0 = MGeoNeutralQuery::partIdByGeoId(
                    getCVM->getModelData()->getGeoManager(), previewElementDim, previewElementId);
                previewRestored = partId0 >= 0 && configureCurrentRenderOperation(
                    partId0, previewElementId, previewElementDim,
                    type != DirectModelingType::ReplaceFace && type != DirectModelingType::RotateFace);
            }
            if (previewRestored)
            {
                m_activePickTarget = previousTarget;
                if (geoPickWidget)
                    geoPickWidget->_featPick = DirectModel;
                updateCurrentPreview(value, true);
            }
            else
            {
                // 回放失败可能已经结束会话；不能保留旧 ID、箭头和可确认的外观。
                if (m_controller->hasActiveOperation())
                    m_controller->onCancel();
                m_renderAdapter->finishSession();
                clearAllHighlights();
                resetSelectionState();
                if (m_controller->isActive())
                    activatePickTarget(defaultPickTarget(), false);
            }
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, tr("直接建模 API 执行失败：%1").arg(result._errorMessage));
        }
        else if (m_controller->isActive() && m_activePickTarget == PickTarget::None)
        {
            // 零值 API 会成功跳过而不发 commitFinished；恢复临时暂停的拾取。
            m_activePickTarget = previousTarget;
            if (geoPickWidget)
                geoPickWidget->_featPick = DirectModel;
        }
        return result._success;
    }

    void mGeoDirectModelingPanel::slot_replaceFaceConfirmClicked()
    {
        if (m_currentModelingType != DirectModelingType::ReplaceFace)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("请先激活替换面操作"));
            return;
        }

        if (m_replaceSourceFaceId < 0 || m_replaceTargetFaceId < 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("请分别拾取替换面的主面和目标面"));
            return;
        }

        if (!m_controller || !m_controller->hasActiveOperation())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("当前没有可确认的替换面预览"));
            return;
        }

        // 目标面确定基准位置，偏置距离允许在该位置上继续增减高度。
        commitParameterOperation(DirectModelingType::ReplaceFace, m_replaceOffsetEdit);
    }

    void mGeoDirectModelingPanel::slot_rotateFaceConfirmClicked()
    {
        commitParameterOperation(DirectModelingType::RotateFace, m_rotateAngleEdit);
    }

    void mGeoDirectModelingPanel::slot_offsetFaceConfirmClicked()
    {
        commitParameterOperation(DirectModelingType::OffsetFace, m_offsetDistanceEdit);
    }

    void mGeoDirectModelingPanel::slot_resizeFilletConfirmClicked()
    {
        commitParameterOperation(DirectModelingType::ResizeFillet, m_filletRadiusEdit);
    }

    bool mGeoDirectModelingPanel::configureCurrentRenderOperation(
        int partId, int elementId, int elementDim, bool showManipulator)
    {
        if (!m_controller || !m_renderAdapter)
            return false;

        using PreviewKind = DirectModelingRenderAdapter::PreviewKind;
        const auto operation = m_controller->currentOperation();
        const bool translation =
            operation == MGeoCommand::DirectModelingOperation::MoveFace
            || operation == MGeoCommand::DirectModelingOperation::OffsetFace;
        //【S11·旋转面】枚举已补 RotateFace（RebuildBasicData），新引擎链路
        // 不再依赖旧栈 V2 门控；旋转预览 = Tool 圆环 + 源面即时旋转。
        const bool rotation = operation == MGeoCommand::DirectModelingOperation::RotateFace;
        PreviewKind kind = PreviewKind::ExactOnly;
        std::vector<glm::vec3> sourceFacePoints;
        std::vector<glm::vec3> sourceEdgePoints;
        if (translation || rotation)
        {
            // 选择/旋转轴变化时缓存一次源数据，拖动不重复调用 OCC。
            if (!m_controller->buildRenderPreviewSource(sourceFacePoints, sourceEdgePoints))
                return false;
            kind = rotation ? PreviewKind::Rotation : (m_controller->moveSolidEnabled()
                ? PreviewKind::SolidTranslation : PreviewKind::FaceTranslation);
        }
        if (rotation)
            showManipulator = true; // 旋转独立显示圆环，不显示线性箭头。
        else if (operation == MGeoCommand::DirectModelingOperation::ReplaceFace)
            showManipulator = false;
        const bool configured = m_renderAdapter->configureOperation(
            partId, elementId, elementDim, kind,
            std::move(sourceFacePoints), std::move(sourceEdgePoints),
            m_controller->arrowCenter(), m_controller->arrowDirection(), showManipulator, m_lastDistance);
        if (configured && (translation || rotation))
        {
            // 选择/切换后立即使用当前参数，清除 Tool 可能残留的上一选择累计距离。
            m_controller->onDragValue(m_lastDistance);
            m_renderAdapter->requestPreview(m_lastDistance);
        }
        return configured;
    }

    void mGeoDirectModelingPanel::processPickResult(const QString& type, const std::vector<int>& ids)
    {
        // SelectionDataContainer 可能暂时保存多个历史高亮。slot_highlightGeoData 已把它
        // 收敛为单选，这里仍只使用最后一个 ID，保证操作会话始终绑定单个对象。
        if (ids.empty())
            return;

        int partId;
        int dim = 1;
        int id = ids[0];
        MGeoData::GeoDataModelSet* m_currentPartSet = nullptr;
        QString message;
        if (type == "几何面")
        {
            dim = 2;
            message = tr("拾取到几何面，ID = %1").arg(ids[0]);
            m_currentPartSet = nullptr; //【OCC摘除·S9】旧栈 set 反查退役（拾取部件 ID 改经 GN，见下方使用）

        }
        else if (type == "几何线")
        {
            dim = 1;
            message = tr("拾取到几何线，ID = %1").arg(ids[0]);
            m_currentPartSet = nullptr; //【OCC摘除·S9】同上

        }
        else if (type == "几何点")
        {
            dim = 0;
            message = tr("拾取到几何点，ID = %1").arg(ids[0]);
        }
        else if (type == "几何体")
        {
            dim = 3;
            message = tr("拾取到几何体，ID = %1").arg(ids[0]);
        }
        else
        {
            message = tr("拾取到未知几何类型：%1，ID = %2").arg(type).arg(ids[0]);
        }

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, message);

        //【OCC摘除·S9】旧栈 set 反查退役（m_currentPartSet 仅保留占位恒 null）——部件 ID 改经 GN
        partId = MGeoNeutralQuery::partIdByGeoId(
            (getCVM != nullptr && getCVM->getModelData() != nullptr) ? getCVM->getModelData()->getGeoManager() : nullptr,
            dim, id);
        if (partId < 0)
            return;
        if (m_currentModelingType == DirectModelingType::ReplaceFace)
        {
            if (dim != 2)
                return;

            if (m_activePickTarget == PickTarget::ReplaceSourceFace)
            {
                m_replaceSourceFaceId = id;
                m_replaceTargetFaceId = -1;
                if (m_replaceSourcePickEdit)
                    m_replaceSourcePickEdit->setText(tr("面 ID = %1").arg(id));
                if (m_replaceTargetPickEdit)
                    m_replaceTargetPickEdit->clear();
                m_activePickTarget = PickTarget::ReplaceTargetFace;
                setPickType(tr("几何面"));
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    0, tr("已选择替换面主面 ID = %1，请继续选择目标面").arg(id));
            }
            else if (m_activePickTarget == PickTarget::ReplaceTargetFace)
            {
                if (m_replaceSourceFaceId < 0)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        1, tr("请先拾取替换面的主面"));
                    m_activePickTarget = PickTarget::ReplaceSourceFace;
                    return;
                }
                if (id == m_replaceSourceFaceId)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        1, tr("主面和目标面不能相同"));
                    return;
                }

                m_replaceTargetFaceId = id;
                if (!m_controller || !m_controller->beginReplaceFaceOperation(
                    m_replaceSourceFaceId, m_replaceTargetFaceId))
                {
                    m_replaceTargetFaceId = -1;
                    if (m_replaceTargetPickEdit)
                        m_replaceTargetPickEdit->clear();
                    return;
                }
                if (m_replaceTargetPickEdit)
                    m_replaceTargetPickEdit->setText(tr("面 ID = %1").arg(id));

                //【OCC摘除·S9】旧栈 set 反查退役，改 GN 部件 ID
                const int replacePartId0 = MGeoNeutralQuery::partIdByGeoId(
                    getCVM->getModelData()->getGeoManager(), 2, m_replaceSourceFaceId);
                if (replacePartId0 < 0 || !configureCurrentRenderOperation(
                    replacePartId0, m_replaceSourceFaceId, 2, false))
                {
                    m_controller->onCancel();
                    return;
                }

                // 目标面给出基准位置；偏置值在其基础上继续沿主面方向增减高度。
                double offset = 0.0;
                QString offsetError;
                const bool offsetValid = evaluateParameterEditor(
                    m_replaceOffsetEdit, offset, &offsetError);
                m_lastDistance = offsetValid ? offset : 0.0;
                m_dragBaseParameter = m_lastDistance;
                m_dragInProgress = false;
                if (m_replaceOffsetEdit && !offsetValid)
                    m_replaceOffsetEdit->setToolTip(offsetError);
                updateCurrentPreview(m_lastDistance, true);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    0, tr("已选择替换面目标面 ID = %1，可输入偏置距离后确定").arg(id));
            }
            return;
        }

        if (m_currentModelingType == DirectModelingType::RotateFace)
        {
            if (m_activePickTarget == PickTarget::RotateSourceFace)
            {
                if (dim != 2)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        1, tr("旋转面操作请先选择源面"));
                    return;
                }
                m_rotateSourceFaceId = id;
                m_rotateAxisEdgeId = -1;
                m_lastDistance = 0.0;
                m_dragBaseParameter = 0.0;
                m_dragInProgress = false;
                if (m_rotateSourcePickEdit)
                    m_rotateSourcePickEdit->setText(tr("面 ID = %1").arg(id));
                if (m_rotateAxisPickEdit)
                    m_rotateAxisPickEdit->setText(tr("面中轴线（默认）"));
                if (m_rotateAngleEdit)
                    m_rotateAngleEdit->setText(QStringLiteral("0"));

                if (!m_controller
                    || !m_controller->beginRotateFaceOperation(m_rotateSourceFaceId))
                {
                    m_rotateSourceFaceId = -1;
                    if (m_rotateSourcePickEdit)
                        m_rotateSourcePickEdit->clear();
                    if (m_rotateAxisPickEdit)
                        m_rotateAxisPickEdit->clear();
                    return;
                }

                if (!configureCurrentRenderOperation(partId, id, 2, false))
                {
                    m_controller->onCancel();
                    return;
                }

                m_activePickTarget = PickTarget::RotateSourceFace;
                setPickType(tr("几何面"));
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    0, tr("已选择旋转面 ID = %1，当前使用面中轴线；可直接输入角度，或拾取直线边覆盖旋转轴").arg(id));
                return;
            }
            if (m_activePickTarget != PickTarget::RotateAxisEdge)
                return;
            if (m_rotateSourceFaceId < 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("请先拾取需要旋转的几何面"));
                m_activePickTarget = PickTarget::RotateSourceFace;
                setPickType(tr("几何面"));
                return;
            }

            if (dim != 1)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("旋转轴必须选择几何直线"));
                return;
            }
            m_rotateAxisEdgeId = id;
            if (!m_controller || !m_controller->beginRotateFaceOperation(
                m_rotateSourceFaceId, m_rotateAxisEdgeId))
            {
                m_rotateAxisEdgeId = -1;
                if (m_rotateAxisPickEdit)
                    m_rotateAxisPickEdit->clear();
                return;
            }
            if (m_rotateAxisPickEdit)
                m_rotateAxisPickEdit->setText(tr("边 ID = %1").arg(m_rotateAxisEdgeId));

            //【OCC摘除·S9】旧栈 set 反查退役，改 GN 部件 ID
            const int rotatePartId0 = MGeoNeutralQuery::partIdByGeoId(
                getCVM->getModelData()->getGeoManager(), 2, m_rotateSourceFaceId);
            if (rotatePartId0 < 0 || !configureCurrentRenderOperation(
                rotatePartId0, m_rotateSourceFaceId, 2, false))
            {
                m_controller->onCancel();
                return;
            }

            double angle = 0.0;
            QString angleError;
            const bool angleValid = evaluateParameterEditor(
                m_rotateAngleEdit, angle, &angleError);
            if (angleValid
                && std::abs(angle) <= 90.0 && std::abs(angle) > 1.0e-12)
            {
                m_lastDistance = angle;
                m_dragBaseParameter = angle;
                m_dragInProgress = false;
                updateCurrentPreview(angle, true);
            }
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                0, tr("已选择旋转轴 ID = %1，请输入角度并预览后点击确定").arg(id));
            return;
        }

        MGeoCommand::DirectModelingOperation requestedOperation =
            MGeoCommand::DirectModelingOperation::None;
        if (m_currentModelingType == DirectModelingType::OffsetFace)
        {
            if (dim != 2)
                return;
            requestedOperation = MGeoCommand::DirectModelingOperation::OffsetFace;
        }
        else if (m_currentModelingType == DirectModelingType::ResizeFillet)
        {
            if (dim == 2)
                requestedOperation = MGeoCommand::DirectModelingOperation::ResizeFillet;
            else if (dim == 1)
                requestedOperation = MGeoCommand::DirectModelingOperation::EdgeFillet;
            else
                return;

            // 圆角拾取器是严格单选：清除其他维度的残留选择，只保留当前面或边。
            if (getCVM && getCVM->getPreRenderWidget()
                && getCVM->getPreRenderWidget()->getSelectionContainer())
            {
                auto* renderWidget = getCVM->getPreRenderWidget();
                auto* geoSelectData =
                    renderWidget->getSelectionContainer()->getGeoSelectData();
                if (geoSelectData)
                {
                    const SelectionInfoType keepType = dim == 1
                        ? SelectionInfoType::GeoEdge : SelectionInfoType::GeoFace;
                    geoPickWidget->_featPick = None;
                    for (SelectionInfoType selectionType : {
                        SelectionInfoType::GeoFace,
                            SelectionInfoType::GeoEdge,
                            SelectionInfoType::GeoPoint,
                            SelectionInfoType::GeoSolid })
                    {
                        if (selectionType == keepType)
                            continue;
                        std::vector<int> selectedIds =
                            geoSelectData->getSelectDataByType(selectionType);
                        if (!selectedIds.empty())
                        {
                            geoSelectData->UpdatePickData(
                                SelectionOperate::REDUCE,
                                selectionType,
                                selectedIds);
                        }
                    }
                    MechPreData::UpdatePreWidgetHighlight(renderWidget);
                    renderWidget->updatePaint();
                    geoPickWidget->_featPick = DirectModel;
                }
            }
        }

        // 由 Panel 将同一份拾取结果同时交给几何控制器与渲染交互工具。
        const bool moveSolid =
            requestedOperation == MGeoCommand::DirectModelingOperation::OffsetFace
            && m_moveSolidCheckBox && m_moveSolidCheckBox->isChecked();
        if (!m_controller || !m_controller->beginOperation(
            dim, id, requestedOperation, moveSolid))
        {
            // 启动失败时必须清掉 DirectTool 上一次会话缓存；否则旧的单面数据仍可
            // 被箭头绘制，用户会误以为本次勾选“偏置”后仍然只移动单面。
            if (m_renderAdapter)
                m_renderAdapter->finishSession();
            return;
        }
        m_operationElementId = id;
        m_operationElementDim = dim;

        QLineEdit* pickEdit = m_currentModelingType == DirectModelingType::OffsetFace
            ? m_offsetPickEdit : m_filletPickEdit;
        if (pickEdit)
            pickEdit->setText(
                dim == 1 ? tr("边 ID = %1").arg(id) : tr("面 ID = %1").arg(id));

        // 调整已有圆角时以检测半径作为绝对参数起点；其他新操作从零开始。
        m_lastDistance = requestedOperation
            == MGeoCommand::DirectModelingOperation::ResizeFillet
            ? m_controller->currentParameter() : 0.0;
        m_dragBaseParameter = m_lastDistance;
        m_dragInProgress = false;
        if (QLineEdit* editor = currentParameterEdit())
            editor->setText(formatParameterValue(m_lastDistance));

        if (!configureCurrentRenderOperation(partId, id, dim, true))
        {
            m_controller->onCancel();
            return;
        }

        //【ExactOnly 预览】圆角拾取完成后按当前参数值立即出首帧引擎预览
        //（替换面在目标面拾取处已有 updateCurrentPreview 触发）。
        if (requestedOperation == MGeoCommand::DirectModelingOperation::ResizeFillet
            || requestedOperation == MGeoCommand::DirectModelingOperation::EdgeFillet)
        {
            updateCurrentPreview(m_lastDistance, true);
        }
    }

    void mGeoDirectModelingPanel::slot_DirectGeoModelingStretch(double distance)
    {
        if (m_currentModelingType == DirectModelingType::ReplaceFace)
            return;
        if (!m_controller || !m_controller->isActive()
            || !m_controller->hasActiveOperation())
        {
            return;
        }

        if (!std::isfinite(distance))
            return;
        if (m_currentModelingType == DirectModelingType::RotateFace && m_rotatePreviewTimer)
            m_rotatePreviewTimer->stop(); // 开始圆环拖动后，不能由旧输入防抖重置拖动基准。
        // 当前 DirectTool 发出本段增量（旋转为度，其余为距离）；同一面多次拖动时，
        // 以这一段开始前的值为基准累加，避免第二次拖动覆盖第一次结果。
        if (!m_dragInProgress)
        {
            m_dragBaseParameter = m_lastDistance;
            m_dragInProgress = true;
        }
        double accumulatedValue = m_dragBaseParameter + distance;
        if (m_currentModelingType == DirectModelingType::RotateFace)
            accumulatedValue = (std::max)(-90.0, (std::min)(90.0, accumulatedValue));

        m_lastDistance = accumulatedValue;
        if (QLineEdit* editor = currentParameterEdit())
            editor->setText(formatParameterValue(m_lastDistance));

        // 旋转在 Tool 中即时绘制，线性操作按累计值延后覆盖；圆角等待后台结果。
        updateCurrentPreview(m_lastDistance, false);
    }

    void mGeoDirectModelingPanel::slot_DirectGeoModelingStretchFinish(double distance)
    {
        if (m_currentModelingType == DirectModelingType::ReplaceFace)
            return;
        if (!m_controller || !m_controller->isActive()
            || !m_controller->hasActiveOperation())
        {
            return;
        }

        // 非有限值通常来自退化的屏幕投影/射线求交。此时取消会话比提交异常
        // 参数更安全，并把编辑框恢复为 0。
        if (!std::isfinite(distance))
        {
            m_controller->onCancel();
            m_lastDistance = 0.0;
            m_dragBaseParameter = 0.0;
            m_dragInProgress = false;
            if (QLineEdit* editor = currentParameterEdit())
                editor->setText(QStringLiteral("0"));
            return;
        }
        // end 信号仍携带本段最终增量，不能直接覆盖累计值。即使一次拖动没有
        // 产生 update 回调，也能在这里用拖动前基准正确还原最终参数。
        if (!m_dragInProgress)
            m_dragBaseParameter = m_lastDistance;
        double accumulatedValue = m_dragBaseParameter + distance;
        if (m_currentModelingType == DirectModelingType::RotateFace)
            accumulatedValue = (std::max)(-90.0, (std::min)(90.0, accumulatedValue));

        m_lastDistance = accumulatedValue;
        m_dragBaseParameter = accumulatedValue;
        m_dragInProgress = false;
        if (QLineEdit* editor = currentParameterEdit())
            editor->setText(formatParameterValue(m_lastDistance));

        // 实体平移直接显示最终刚体位移；其他操作请求精确后台预览。
        updateCurrentPreview(m_lastDistance, true);
    }

    void mGeoDirectModelingPanel::clearAllHighlights()
    {
        if (!getCVM || !getCVM->getPreRenderWidget()
            || !getCVM->getPreRenderWidget()->getSelectionContainer())
        {
            return;
        }

        auto* renderWidget = getCVM->getPreRenderWidget();
        auto* geoSelectData = renderWidget->getSelectionContainer()->getGeoSelectData();
        if (!geoSelectData)
            return;

        // 同时清除所有维度，因为旋转面和替换面会在一个会话中先后选择面与边，
        // 只清当前 PickFilter 会留下旧高亮和无效拓扑 ID。
        for (SelectionInfoType type : {
            SelectionInfoType::GeoFace,
                SelectionInfoType::GeoEdge,
                SelectionInfoType::GeoPoint,
                SelectionInfoType::GeoSolid })
        {
            std::vector<int> ids = geoSelectData->getSelectDataByType(type);
            if (!ids.empty())
                geoSelectData->UpdatePickData(SelectionOperate::REDUCE, type, ids);
        }

        MechPreData::UpdatePreWidgetHighlight(renderWidget);
        renderWidget->updatePaint();
    }

    void mGeoDirectModelingPanel::resetPanelAfterCommit()
    {
        // 暂停选择处理，避免数据层重建拓扑和清高亮产生的同步通知误启动新会话。
        m_activePickTarget = PickTarget::None;
        if (geoPickWidget)
            geoPickWidget->_featPick = None;
        if (m_currentModelingType == DirectModelingType::ResizeFillet
            && m_filletPickTypeCombo)
        {
            // 边倒圆完成后最常见的下一步是拾取新圆角面调整半径。
            const QSignalBlocker blocker(m_filletPickTypeCombo);
            m_filletPickTypeCombo->setCurrentIndex(0);
        }

        m_renderAdapter->finishSession();
        clearAllHighlights();
        resetSelectionState();
        activatePickTarget(
            defaultPickTarget(),
            false);
    }

    void mGeoDirectModelingPanel::handleDirectModelingModeChanged(bool active)
    {
        if (active)
            return;

        // Controller 的 modeChanged(false) 是命令行自动退出、面板主动
        // 退出和 Controller 销毁的统一边界。先断开选择容器，再清高亮，
        // 避免 UpdatePickData 的同步通知在退出过程中重新启动操作。
        disconnectRuntimeSignals();
        if (m_parameterApplyTimer)
            m_parameterApplyTimer->stop();
        m_activePickTarget = PickTarget::None;
        if (geoPickWidget)
            geoPickWidget->_featPick = None;
        clearAllHighlights();
        resetSelectionState();
    }

    void mGeoDirectModelingPanel::slot_highlightGeoData()
    {
        if (!getCVM || !getCVM->getPreRenderWidget() || !getCVM->getModelData())
            return;
        // 鼠标按住圆环期间，渲染高亮通知不能再次选面并重建操作器。
        // Tool 状态覆盖首个 Stretch 信号前的按下阶段，Panel 状态覆盖信号处理阶段。
        if (m_currentModelingType == DirectModelingType::RotateFace
            && (m_dragInProgress || (m_renderAdapter && m_renderAdapter->isRotationDragging())))
            return;
        if (m_activePickTarget == PickTarget::None)
            return;

        QString type = "几何线";
        std::vector<int> res;
        PickFilter pickFilter = getCVM->getPreRenderWidget()->getPickFilter();
        switch (pickFilter)
        {
        case mxr::PickFilter::PickGeoSolid:
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
            type = "几何体";
            break;
        case mxr::PickFilter::PickGeoFace:
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
            type = "几何面";
            break;
        case mxr::PickFilter::PickGeoLine:
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
            type = "几何线";
            break;
        case mxr::PickFilter::PickGeoPoint:
            res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
            type = "几何点";
            break;
        default:
            break;
        }
        if (res.empty())
            return;

        if (geoPickWidget->_featPick == DirectModel)
        {
            if (res.size() == 1)
            {
                processPickResult(type, res);
            }
            else
            {
                // 直接建模严格单选：保留最后加入的 ID，并从选择容器主动移除旧 ID。
                // 临时把 _featPick 设为 None，避免 UpdatePickData 的同步信号递归进入本槽。
                const int newestId = res.back();
                std::vector<int> Ids(res.begin(), res.end() - 1);
                // 清理旧高亮时暂时关闭拾取处理，避免选择容器的同步通知重复启动操作。
                geoPickWidget->_featPick = None;
                if (type == "几何线")
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoEdge, Ids);
                if (type == "几何点")
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoPoint, Ids);
                if (type == "几何面")
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoFace, Ids);
                if (type == "几何体")
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoSolid, Ids);

                MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
                getCVM->getPreRenderWidget()->updatePaint();
                geoPickWidget->_featPick = DirectModel;
                processPickResult(type, std::vector<int>{ newestId });
            }
        }
    }

    void mGeoDirectModelingPanel::slot_subWindowActivated()
    {
        const void* viewportKey = getCVM && getCVM->getPreRenderWidget()
            ? static_cast<const void*>(getCVM->getPreRenderWidget())
            : nullptr;
        const void* previousViewportKey = m_runtimeViewportKey;
        const bool viewportActuallyChanged =
            previousViewportKey != nullptr
            && previousViewportKey != viewportKey;
        // QMdiArea 可能对同一个子窗口重复发出激活通知。它不代表模型发生变化，
        // 若仍按真正的视口切换处理，会把用户刚勾选的“偏置”恢复为 false。
        if (viewportKey && viewportKey == m_runtimeViewportKey && conn_geo)
            return;

        // 视口切换后旧 SignalEmitter、SelectionContainer 和业务 ID 均不再可靠。
        disconnectRuntimeSignals();
        // 退出直接建模后，子窗口切换不得偷偷把拾取信号接回。
        if (!m_controller || !m_controller->isActive())
            return;

        if (viewportActuallyChanged)
        {
            // 只有从一个已绑定视口真正切到另一个视口时，旧拾取 ID、累计距离和
            // 复选框才属于旧模型，必须完整清理。第一次建立连接时 previousViewportKey
            // 为空；此时用户可能已经先勾选“偏置”，不能在第一次点击视口时清掉它。
            m_activePickTarget = PickTarget::None;
            if (geoPickWidget)
                geoPickWidget->_featPick = None;
            m_controller->onCancel();
            resetSelectionState();
        }

        // 子窗口被关闭时当前视口可以暂时为空。旧会话仍已在上面完整清理；
        // 等下一次有效子窗口激活时再恢复拾取和连接。
        if (!getCVM || !getCVM->getPreRenderWidget()
            || !getCVM->getPreRenderWidget()->getSelectionContainer())
            return;

        if (viewportActuallyChanged)
            clearAllHighlights();
        activatePickTarget(defaultPickTarget(), false);

        conn_geo = connect(
            getCVM->getPreRenderWidget()->getSelectionContainer(),
            &mxr::SelectionDataContainer::ReadyToReadContainer,
            this,
            &mGeoDirectModelingPanel::slot_highlightGeoData);
        m_runtimeViewportKey = viewportKey;
        connectSignal();
    }
}
