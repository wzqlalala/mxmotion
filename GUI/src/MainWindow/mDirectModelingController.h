#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mGeoFeatureParameterCalculator.h"

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>
#include <QTimer>

//【OCC摘除·S10/P0-1 终局】退役头 mGeoCommandDirectModelingGeometry.h（含 50 处 OCC 引用）
// 不再引入；DirectModelingOperation 枚举以中性形态就地落位（成员照抄退役头实测定义）。
// 直模引擎化迁移时本枚举即现成契约。原 OCC include 同步摘除：
//#include "mGeoCommandDirectModelingGeometry.h"
//#include <BRepTools_History.hxx>
//#include <TopoDS_Shape.hxx>
namespace MGeoCommand {
    /** 单次直接建模操作类型（中性副本：源定义在退役的 mGeoCommandDirectModelingGeometry.h）。 */
    enum class DirectModelingOperation
    {
        None,
        MoveFace,
        DeleteFace,
        ReplaceFace,
        OffsetFace,
        ResizeFillet,
        EdgeFillet,
        ExtrudeVertex,
        ExtrudeEdge,
        ResizeCylinder,
        RotateFace  //【S11·旋转面】绕指定轴旋转面（角度 [-90,90]，轴=面中轴线或直线边）
    };
}

#include <glm/vec3.hpp>

#include <memory>
#include <vector>

// previewDataReady 可能通过 QueuedConnection 跨线程投递。注册容器类型后，Qt
// 才能安全复制/移动这些顶点数据；信号中不传 TopoDS_Shape，避免 OCC 对象进入 UI 线程。
Q_DECLARE_METATYPE(std::vector<glm::vec3>)

// 前向声明，避免在头文件中引入 RebuildGUI 内部模块头文件
namespace MGeoData
{
    class GeoDataBase;
    class GeoDataModelSet;
}
namespace MechModelData
{
    class mModelData;
}
namespace MechPreData
{
    class mViewportModel;
}
namespace MechMainWindow
{
    class DirectModelingPreviewWorker;

    /**
     * @brief 一次可由界面或脚本提交的直接建模命令
     *
     * 该结构只包含稳定的业务参数，不包含 Panel、DirectTool 或 TopoDS 对象。
     * primaryElementIds/referenceElementIds 用于 Python/API 批量调用；旧版单数成员
     * 继续供面板和已有脚本使用。Controller 会把两种表示归一化，并把多对象作为
     * 一次原子几何事务提交。elementDim 使用现有约定：1=边，2=面。
     */
    struct MMAINWINDOW_EXPORT DirectModelingCommand
    {
        MGeoCommand::DirectModelingOperation operation =
            MGeoCommand::DirectModelingOperation::None;
        // API 批量调用优先使用复数列表；为空时继续读取下面的单数 ID，保证旧脚本
        // 和面板调用兼容。替换面 referenceElementIds 可为 1 个（所有源面共用目标面）
        // 或与 primaryElementIds 等长；旋转面始终只允许一条公共轴边。
        QVector<int> primaryElementIds;
        QVector<int> referenceElementIds;
        int primaryElementId = -1;
        int referenceElementId = -1;
        int elementDim = 2;
        double value = 0.0;
        QString drivingExpression;
        // 仅供命令行 JSON 执行器内部使用。非空时，primaryElementIds 表示脚本
        // 启动时记录的初始面 ID。字段名为兼容旧协议而保留；Controller 会在该
        // context 内用 OCC History 维护“初始点/线/面/体 ID -> 当前 ID”的完整链。
        QString originalFaceIdContext;
        // true 仅对单面 MoveFace/OffsetFace 有效：选中面只用于确定方向，
        // 实际几何操作改为刚体平移该面唯一所属的 Solid/CompSolid 实体。追加在结构体末尾，
        // 使旧的位置聚合初始化仍按原字段顺序工作；缺省 false 保持原有面移动。
        bool moveSolid = false;
    };

    /**
     * @brief 直接建模几何会话控制器
     *
     * 职责：
     * 1. 管理直接建模模式与几何操作会话；
     * 2. 接收面板传入的拾取结果，调用 DirectModelingGeometry 分析并启动操作；
     * 3. 刚体拖动仅维护参数，释放后才向后台 OCC 线程请求精确预览；
     *    圆角等拓扑型操作在拖动停顿后防抖预览；
     * 4. 接收提交/取消，完成几何修改并更新部件数据模型；
     * 5. 不负责拾取、箭头交互或绘制；DirectPanel 使用 DirectTool 的拾取/距离接口，
     *    由适配器将本类返回的缓存/精确顶点交给当前视口预览渲染对象。
     *
     * 生命周期：应用级单例（QObject 派生），DirectModelingGeometry 为其成员，
     * 生命周期与直接建模模式绑定；模式退出时自动取消会话。
     *
     * 线程约束：正式 m_geometry 只在界面线程建立/提交；后台线程只读取
     * m_previewGeometrySnapshot 的独立副本。每次换选择都会增加 sessionId，
     * 每次换参数都会增加 sequence，过期结果即使晚到也不会覆盖新预览。
     */
    class MMAINWINDOW_EXPORT DirectModelingController : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief 获取应用级唯一实例
         */
        static DirectModelingController* getInstance();

        /**
         * @brief 析构（自动退出模式并释放资源）
         */
        ~DirectModelingController() override;

        // ------------------------------------------------------------------
        // 模式开关
        // ------------------------------------------------------------------

        /**
         * @brief 进入直接建模模式
         *
         * 激活会话控制器；几何拾取由 DirectPanel 负责并通过 beginOperation 转入。
         * 每次调用均核对当前模型并刷新参数表；已激活时只刷新，不重复启动模式。
         */
        void enterDirectModelingMode();

        /**
         * @brief 退出直接建模模式
         *
         * 取消当前会话并通知外部清理箭头/预览，恢复到正常交互状态。
         * 保留无参入口以兼容已经链接该导出类的其他模块。
         */
        void exitDirectModelingMode();

        /**
         * @brief 退出直接建模模式，并指定是否回放参数驱动历史
         *
         * @param rebuildDrivenHistory true 时在退出前回放参数历史；命令行 API 已在
         *        提交过程中完成回放，可传 false 避免重复几何计算。
         */
        void exitDirectModelingMode(bool rebuildDrivenHistory);

        /**
         * @brief 当前是否处于直接建模模式
         */
        bool isActive() const { return m_isActive; }

        /**
         * @brief 使用面板传入的拾取结果启动直接建模操作
         * @param elementDim 几何维度：0 点、1 边、2 面、3 体
         * @param elementId 几何元素 ID
         * @param isOffset 是否优先使用 Offset 操作
         * @return 操作成功启动返回 true
         */
        bool beginOperation(int elementDim, int elementId, bool isOffset = false);

        /**
         * @brief 使用面板明确选择的操作类型启动直接建模
         *
         * 选择 ReplaceFace 时还需要目标替换面；几何层未取得目标面前会拒绝启动，
         * 不会退回到其他自动操作。
         */
        bool beginOperation(int elementDim, int elementId,
            MGeoCommand::DirectModelingOperation operation);

        /**
         * @brief 启动面操作，并指定是否改为平移所选面所属实体
         *
         * moveSolid 只允许用于单个几何面的 MoveFace/OffsetFace。false 与三参数
         * 重载完全一致，保证现有调用方行为不变；true 时仍沿所选面的操作方向拖动，
         * 但预览和确定都作用于该面唯一所属的 Solid/CompSolid 实体。
         */
        bool beginOperation(
            int elementDim,
            int elementId,
            MGeoCommand::DirectModelingOperation operation,
            bool moveSolid);

        /** 使用源面和目标面启动替换面会话。目标面只作为参照，可以来自其他部件。 */
        bool beginReplaceFaceOperation(int sourceFaceId, int targetFaceId);

        /** 使用源面的面心中轴线启动旋转面会话。旋转参数单位为度，范围为 [-90, 90]。 */
        bool beginRotateFaceOperation(int sourceFaceId);

        /** 使用源面和直线边覆盖默认中轴线并启动旋转面会话。 */
        bool beginRotateFaceOperation(int sourceFaceId, int axisEdgeId);

        /**
         * @brief 统一执行一次直接建模命令
         *
         * Python/命令行 API 和 Panel 的普通面操作通过本函数执行。Panel 勾选
         * 实体平移时直接 onCommit 当前交互会话，保留已选择的完整实体作用域；
         * 两条路径最终共用几何提交、自动参数创建和部件更新实现。
         */
        bool executeCommand(
            const DirectModelingCommand& command,
            QString* errorMessage = nullptr);

        /**
         * @brief 释放一次脚本执行所使用的初始几何 ID 上下文
         *
         * contextId 由脚本执行器生成，并贯穿同一次脚本的全部步骤。脚本成功、
         * 失败或取消后都必须调用本函数，避免保留已经失效的部件指针。
         */
        void clearScriptOriginalFaceIdContext(const QString& contextId);

        /**
         * @brief 把脚本中的初始几何 ID 转换为当前数据模型中的业务 ID
         *
         * 已被本脚本修改的部件只接受最初 ID，并沿严格 OCC History 链返回当前
         * 业务 ID；未进入 History 链的 ID 只有在当前模型中唯一存在、且属于尚未
         * 修改的其他部件时才按原值返回。elementDim 约定为 0=点、1=线、2=面、
         * 3=体。该接口供后续约束、边界、接触等脚本 API 复用。
         */
        bool resolveScriptOriginalGeometryIds(
            const QString& contextId,
            int elementDim,
            const QVector<int>& originalIds,
            QVector<int>& currentIds,
            QString* errorMessage = nullptr);

        // ------------------------------------------------------------------
        // 拖拽交互（供箭头操作器调用）
        // ------------------------------------------------------------------

        /**
         * @brief 拖拽参数更新
         *
         * 移动面由适配器调用渲染矩阵即时预览，本函数不调用 OCC；
         * 圆角等不能通过刚体变换表达的操作使用空闲防抖后台预览。
         * 普通拖动结束调用精确 previewDiscretized，实体平移无需重复离散。
         * 圆环发送本段角度增量（度）；角度输入和释放圆环均通过 onDragFinished 精确预览。
         * @param accumulatedValue Panel 根据 DirectTool 的本段增量换算出的累计值；
         * 移动面时单位为 mm，旋转面时单位为度
         */
        void onDragValue(double accumulatedValue);

        /** 拖动结束：使用 Panel 换算后的累计值生成精确后台预览，但仍不修改几何数据。 */
        void onDragFinished(double accumulatedValue);

        /**
         * @brief ExactOnly 族（替换面/圆角调整/边倒圆）引擎"编辑不落盘"预览
         *
         * 按当前会话（对象/引用/参数）请求引擎跑同一编辑链但不提交部件，
         * 结果体直接离散为一次性网格经 previewDataReady 下发渲染；
         * 失败时下发空数据清除旧预览。移动/旋转族由渲染矩阵通道即时预览，不走本接口。
         */
        void requestExactPreview(double value);

        /**
         * @brief 删除一个参数及其驱动步骤（含同部件后续依赖步骤）并基线重放
         *
         * 参数表右键删除入口：按提交顺序，被删步骤之后同部件的步骤视为依赖
         * 其几何前缀，一并删除；删除后从基线重放剩余步骤并刷新参数表。
         */
        bool deleteDrivenParameter(const QString& parameterId, QString* errorMessage);

        /**
         * @brief 部件校验：被删除/不存在的部件对应的参数自动移除
         *
         * 迭代全部驱动节点检查 partId 在部件管理器中的存在性；
         * 不存在的连同参数和驱动节点一并移除，并通知面板刷新。
         * 在参数表刷新和参数回放前自动调用。
         */
        void validateDrivenParameters();

        /**
         * @brief 提交当前直接建模操作
         *
         * 提交前根据当前“部件 + 几何对象 + 操作类型”自动创建或更新参数；
         * 公式无效时不会修改正式几何。随后调用 finish 生成最终 shape，执行
         * updatePart 更新数据模型，并通知渲染层清理预览、隐藏操纵器。
         * @param distance 移动面/替换面时为距离（mm），旋转面时为角度（度）；
         * drivingExpression 非空时仅作为表达式无法创建常量时的回退值
         * @param drivingExpression 用户输入的原始公式；为空表示使用 distance 建立常量参数，
         * 非空时以公式计算结果作为真正提交值
         * @param errorMessage 可选的同步错误信息，供 API 调用方返回到脚本执行器
         * @return 正式几何和数据模型均更新成功时返回 true
         */
        bool onCommit(
            double distance,
            const QString& drivingExpression = QString(),
            QString* errorMessage = nullptr,
            const QString& scriptContextId = QString());

        /**
         * @brief 取消当前会话
         *
         * 清理预览、隐藏箭头、重置会话状态，但保持直接建模模式激活。
         */
        void onCancel();
        // ------------------------------------------------------------------
        // 供箭头操作器读取的会话数据
        // ------------------------------------------------------------------

        /**
         * @brief 当前会话箭头中心（部件局部坐标）
         */
        QVector3D arrowCenter() const { return m_arrowCenter; }

        /**
         * @brief 当前会话箭头方向（部件局部坐标）
         */
        QVector3D arrowDirection() const { return m_arrowDirection; }

        /**
         * @brief 当前是否有已启动的操作会话
         */
        bool hasActiveOperation() const;

        /** 当前操作类型，供渲染工具选择平移或旋转即时预览。 */
        MGeoCommand::DirectModelingOperation currentOperation() const
        {
            return m_currentOperation;
        }
        /** 当前累计参数；圆角面为检测到的初始半径，其他操作通常从 0 开始。 */
        double currentParameter() const { return m_currentParameter; }
        /** 当前面移动会话是否改为平移所选面所属实体。 */
        bool moveSolidEnabled() const { return m_moveSolid; }

        // ------------------------------------------------------------------
        // 全局参数驱动
        // ------------------------------------------------------------------

        /**
         * 根据当前几何管理器及可安全访问的历史数据库清理已删除模型的参数/历史。
         * 部件替换但 partId 仍存在时只重定向绑定；不根据旧 QPointer 归零误删参数。
         * 数据库已销毁或部件清空时删除对应参数，其他存活模型及其公式仍予保留。
         * 始终发送参数表刷新通知，不执行几何回放；丢失部件的活动预览会被取消。
         */
        void refreshDirectModelingParameters();

        /**
         * 按稳定参数 id 更新名称和公式，不允许创建或删除参数。
         * 公式中的参数引用必须写成“{参数名}”；函数和内置常量语法保持不变。
         */
        bool updateDirectModelingParameters(
            const QHash<QString, QString>& nameById,
            const QHash<QString, QString>& expressionById,
            QString* errorMessage = nullptr);

        /**
         * 兼容原有调用方：只更新公式，内部转发到 updateDirectModelingParameters。
         */
        bool updateDirectModelingParameterExpressions(
            const QHash<QString, QString>& expressionById,
            QString* errorMessage = nullptr);

        /** 应用级参数表的只读快照；Controller 单例使其不依赖某个 Panel 的生命周期。 */
        QVector<GeoFeatureParameter> directModelingParameters() const
        {
            return m_parameterCalculator
                ? m_parameterCalculator->parameters()
                : QVector<GeoFeatureParameter>();
        }

        /**
         * 使用当前有效参数计算任意表达式。表达式可带可选的前导 '='，
         * 参数引用必须使用“{参数名}”；
         * 成功结果保证为有限 double，可直接传给预览或提交接口。
         */
        bool evaluateDirectModelingExpression(
            const QString& expression,
            double& value,
            QString* errorMessage = nullptr) const;

        /**
         * 将当前选择只离散一次。适配器缓存顶点并传矩阵给渲染，不再逐帧调用 OCC。
         */
        bool buildRenderPreviewSource(
            std::vector<glm::vec3>& facePoints,
            std::vector<glm::vec3>& edgePoints) const;

    signals:
        /**
         * @brief 预览三角形和线段顶点已就绪
         * @param addedFacePoints 增料区顶点，每三个点组成一个三角形
         * @param addedEdgePoints 增料区顶点，每两个点组成一条线段
         * @param removedFacePoints 减料区顶点，每三个点组成一个三角形
         * @param removedEdgePoints 减料区顶点，每两个点组成一条线段
         *
         //* 信号只暴露标准顶点容器，渲染侧无需依赖 MGeoCommand::GeoPreviewData。 //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
         */
        void previewDataReady(std::vector<glm::vec3> addedFacePoints,
            std::vector<glm::vec3> addedEdgePoints,
            std::vector<glm::vec3> removedFacePoints,
            std::vector<glm::vec3> removedEdgePoints);

        /**
         * @brief 请求渲染侧清空当前预览
         *
         * Controller 只发出请求，不执行任何界面侧或渲染侧绘制操作。
         */
        void previewClearRequested();

        /**
         * @brief 一次提交已经结束
         * @param success true 表示新几何已成功写入数据模型
         *
         * 界面只能在 success 为 true 时清空拾取状态和全部高亮；失败时保留现场供重试。
         */
        void commitFinished(bool success);

        /**
         * @brief 出现可拖拽箭头
         * @param center 箭头中心（部件局部坐标）
         * @param direction 箭头方向（部件局部坐标）
         */
        void arrowAvailable(QVector3D center, QVector3D direction);

        /**
         * @brief 箭头应被隐藏（会话取消/提交完成/退出模式）
         */
        void arrowHidden();

        /**
         * @brief 直接建模模式状态改变
         * @param active true=进入，false=退出
         */
        void modeChanged(bool active);

        /** 参数定义或其计算状态发生变化；用于同步所有直接建模面板。 */
        void directModelingParametersChanged();

    private:
        friend class DirectModelingPreviewWorker;
        //【S11·参数驱动】一次"确定"的回放记录：拓扑身份 = 提交时的原始 GN ID，
        // 回放经引擎 GN 编辑历史（old→new 链）解析到当前 ID；断链（历史清空/
        // 部件删除）后 historyRetired，仅保留参数记录不可回放。
        struct DrivenOperation
        {
            QString parameterId;    // 不可变参数身份（= 计算器 objectKey）
            QString parameterName;  // 仅显示用
            MGeoCommand::DirectModelingOperation operation =
                MGeoCommand::DirectModelingOperation::None;
            bool moveSolid = false;
            int partId = -1;
            int elementDim = 2;
            std::vector<int> selectedIds;    // 原始操作面/边 ID
            std::vector<int> referenceIds;   // 替换目标面/旋转轴边原始 ID
            int referencePartId = -1;        // 引用对象所在部件（替换面常跨部件）
            QVector3D axisPoint;             // 旋转面回放轴
            QVector3D axisDirection{ 0.0f, 0.0f, 1.0f };
            double currentValue = 0.0;
            bool historyRetired = false;
            //【基线重放·拓扑定位】提交时操作面在其部件拓扑表（getTopologyByDimension）
            // 中的序号与表长（对齐旧栈 TopologyLocator：数量不变时序号即身份，
            // 数量变化=前序参数改变拓扑 → 该参数失效而非认错面）。
            int locatorIndex = -1;
            int locatorCount = -1;
            //【锚点验证】提交前面锚点（质心/法向/面积）：回放时 locator 命中后
            // 校验是否同一面——前序参数值改变会使拓扑表顺序变化，序号可能指
            // 到别的面；锚点不匹配时回退到几何搜索。
            QVector3D preEditAnchorCenter;
            QVector3D preEditAnchorNormal{ 0.0f, 0.0f, 1.0f };
            double preEditAnchorArea = -1.0;
            //【边倒圆回放】几何锚定：提交时被倒圆边的中点 + 所在面 ID。
            // 回放时在当前面边界中找中点最近的边（边 ID 因布尔重建不稳定）。
            QVector3D filletEdgeMidpoint;
            int filletHostFaceId = -1;
            //【几何锚点身份】操作面提交后的质心/法向/面积（网格离散导出）。
            // 多操作多次改号时按几何匹配找回面（编号无关），回放后滚动更新。
            QVector3D anchorCenter;
            QVector3D anchorNormal{ 0.0f, 0.0f, 1.0f };
            double anchorArea = -1.0;
        };
        struct ScriptInitialTopologyIdContext;

        /**
         * @brief 私有构造（单例）
         */
        explicit DirectModelingController(QObject* parent = nullptr);

        // 禁用拷贝
        DirectModelingController(const DirectModelingController&) = delete;
        DirectModelingController& operator=(const DirectModelingController&) = delete;

        /**
         * @brief 根据拾取维度与 ID 提取 TopoDS_Shape 与所属部件
         * @param dim 拾取维度：0 点 1 边 2 面 3 体
         * @param id 几何元素 ID
         * @param shape 输出所选形状
         * @param partSet 输出所属部件
         * @return 成功返回 true
         */
        //bool resolveSelection(int dim, int id, TopoDS_Shape& shape,
        //    MGeoData::GeoDataModelSet*& partSet); //【OCC摘除·S10/P0-1 终局】（TopoDS 出参签名随直模机构下线）

        // ------------------------------------------------------------------
        // 会话管理
        // ------------------------------------------------------------------

        /**
         * @brief 使用指定操作启动当前拾取的直接建模会话
         * @param requestedOperation 指定操作；None 表示继续使用自动分析结果
         * @param isOffset 自动分析时是否优先使用 Offset 模式
         * @return 成功返回 true
         */
        bool startDirectModelingOperation(
            MGeoCommand::DirectModelingOperation requestedOperation,
            bool isOffset,
            bool moveSolid);

        bool beginOperationInternal(
            int elementDim,
            int elementId,
            MGeoCommand::DirectModelingOperation requestedOperation,
            bool isOffset,
            bool moveSolid);

        /**
         * @brief 取消会话并清理内部状态
         */
        void resetSession();

        /**
         * @brief 根据选择信息确定默认操作（复用现有面板优先级逻辑）
         */
        //MGeoCommand::DirectModelingOperation operationFromSelection(
        //    const MGeoCommand::DirectSelectionInfo& info, bool isOffset) const; //【OCC摘除·S10/P0-1 终局】（DirectSelectionInfo 属退役头）

        // ------------------------------------------------------------------
        // 预览与提交
        // ------------------------------------------------------------------

        /**
         * @brief 将一次轻量或精确预览请求投递给后台工作线程
         */
        void doPreview(double distance, bool exact);

        /**
         * 为已启动的几何操作创建后台预览快照。
         * 快照复制完成后，后台线程不再读取可被 UI 修改的正式 m_geometry。
         */
        void preparePreviewSession();

        /**
         * 增加会话/序列号并清除 pending，使所有在途结果失效；
         * 不修改正式几何会话，因此提交失败后仍可恢复预览。
         */
        void invalidatePreviewSession();

        /**
         * 接收工作线程结果；只有 sessionId 和 sequence 同时匹配当前值才发布。
         * 该函数由 QMetaObject::invokeMethod 回到 Controller 所在线程执行。
         */
        void handlePreviewResult(
            quint64 sessionId,
            quint64 sequence,
            bool success,
            QString errorMessage,
            std::vector<glm::vec3> addedFacePoints,
            std::vector<glm::vec3> addedEdgePoints,
            std::vector<glm::vec3> removedFacePoints,
            std::vector<glm::vec3> removedEdgePoints);

        /**
         * @brief 将最终 shape 更新到数据模型并刷新视图
         */
//        bool updatePart(
//            MGeoData::GeoDataModelSet* oldSet,
//            const TopoDS_Shape& newShape,
//            MGeoData::GeoDataModelSet** updatedSet = nullptr,
//            Handle(BRepTools_History) history = Handle(BRepTools_History)(),
//            const QString& scriptContextId = QString(),
//            Handle(BRepTools_History) scriptTrackingHistory =
//            Handle(BRepTools_History)()); //【OCC摘除·S10/P0-1 终局】（TopoDS/History 签名随直模机构下线）
        /** 多对象命令的原子提交实现：全部几何成功后只更新一次部件。 */
        bool executeBatchCommand(
            const DirectModelingCommand& command,
            const QVector<int>& primaryElementIds,
            const QVector<int>& referenceElementIds,
            QString* errorMessage);
        /**
         * 解析直接建模命令中的初始面 ID。第一次访问某个部件时会在修改前一次性
         * 快照该部件的全部点、线、面、体；之后只能沿 OCC History 精确推进。
         */
        bool resolveScriptOriginalFaceIds(
            const QString& contextId,
            const QVector<int>& originalFaceIds,
            QVector<int>& currentFaceIds,
            QString* errorMessage);
        /** 为指定脚本上下文快照一个尚未修改的部件及其全部四类拓扑。 */
        bool captureScriptInitialTopologyPart(
            ScriptInitialTopologyIdContext& context,
            MGeoData::GeoDataModelSet* partSet,
            QString* errorMessage);
        /**
         * 校验或提交一次直接建模的拓扑历史。commitMapping=false 时仅使用纯 Shape
         * 预检完整一一映射；true 时再读取 newSet 的业务 ID，并把该部件全部
         * initialId -> currentShape/currentId 原子推进。任何缺失或一对多都会失败。
         * stableDataHistoryOutput 仅供 original-ID 脚本预检使用：它输出一份严格的
         * “当前 oldSet -> newShape”双射 History，供数据层保留业务 ID；参数整链回放
         * 的 trackingHistory 可能是“初始 Shape -> 最终 Shape”，不能直接交给数据层。
         */
//        bool advanceScriptInitialTopologyIds(
//            const QString& contextId,
//            MGeoData::GeoDataModelSet* oldSet,
//            const TopoDS_Shape& newShape,
//            MGeoData::GeoDataModelSet* newSet,
//            const Handle(BRepTools_History)& trackingHistory,
//            bool commitMapping,
//            QString* errorMessage,
//            Handle(BRepTools_History)* stableDataHistoryOutput = nullptr); //【OCC摘除·S10/P0-1 终局】
        /**
         * 按部件和确认顺序回放全部参数驱动操作。任一操作无法重新定位或生成时，
         * 保持该部件上一次有效几何，并把该历史链参数标记为几何失效。
         */
        bool rebuildDrivenOperations(
            QString* errorMessage = nullptr,
            const QString& scriptContextId = QString());
        /**【S11·参数驱动】提交成功后建参/更新参数并记录回放节点（onCommit 与 executeCommand 共用）。 */
        void recordDrivenOperation(
            MGeoCommand::DirectModelingOperation operation,
            const QVector<int>& selectedIds, int elementDim,
            const QVector<int>& referenceIds,
            double value, const QString& drivingExpression,
            bool moveSolid, const QVector3D& axisPoint,
            const QVector3D& axisDirection,
            int partIdHint = -1,
            const QVector3D& preAnchorCenter = QVector3D(),
            bool hasPreAnchor = false,
            int locatorIndex = -1,
            int locatorCount = -1,
            const QVector3D& preEditNormal = QVector3D(0.0f, 0.0f, 1.0f),
            double preEditArea = -1.0);
        /**【S11·参数驱动】原始 GN ID 经引擎编辑历史链解析为当前 ID（断链返回 -1）。 */
        int resolveCurrentId(int partId, int dim, int originalId) const;

        /** 新部件替换旧部件后，将该部件全部历史记录重定向到新的数据对象。 */
        void retargetDrivenOperations(
            int partId,
            MechPreData::mViewportModel* viewportModel,
            MGeoData::GeoDataModelSet* updatedPartSet);
        /** 恢复缺失历史成功后，旧效果保留在当前 Shape 中，旧节点仅保留参数不再回放。 */
        void retireDrivenHistoryForRecovery(
            const QString& recoveredParameterId,
            int partId,
            MechPreData::mViewportModel* viewportModel,
            MGeoData::GeoDataBase* geoData);
        /** 每次清理几何校验后恢复退役节点的失效状态，避免改名/重绑定使旧历史复活。 */
        void markRetiredDrivenParametersInvalid();
        /** 根据当前部件、选中对象和操作类型生成自动参数的内部稳定键。 */
        QString currentParameterObjectKey() const;
        /** 返回用于自动参数命名和表格显示的中文操作名称。 */
        QString currentOperationDisplayName() const;

        /**
         * @brief 获取当前视口模型（封装 getCVM，便于判空）
         */
        MechPreData::mViewportModel* currentViewportModel() const;

        /**
         * @brief 获取当前模型数据
         */
        MechModelData::mModelData* currentModelData() const;

        /**
         * @brief 获取几何数据
         */
        MGeoData::GeoDataBase* currentGeoData() const;

        /**
         * @brief 输出全局消息
         */
        void outputMessage(int type, const QString& msg, bool statusbar = false) const;

    private:
        // 单例只负责协调当前活动视口；它不拥有部件数据和渲染窗口。
        static DirectModelingController* s_instance;

        // 模式开关与“是否已经选择几何并启动算法会话”是两个独立状态。
        bool m_isActive = false;

        // 业务层元素 ID 会在每次启动操作时解析为当前 GeoDataModelSet 内的 OCC 子形状。
        int m_selectedElementId = -1;
        int m_selectedElementDim = -1; // 0 点 1 边 2 面 3 体

        //【直模引擎化·S11】替换面目标/旋转参考（引擎提交用，中性 ID）
        int m_engineReferenceId = -1;
        // 非拥有指针；提交后旧 PartSet 被数据层移除，因此 resetSession 必须立即置空。
        MGeoData::GeoDataModelSet* m_currentPartSet = nullptr;

        // 直接建模几何会话（非全局静态，生命周期绑定 Controller）
        //MGeoCommand::DirectModelingGeometry m_geometry; //【OCC摘除·S10/P0-1 终局】（stub 类，随桩退役）

        // 当前操作类型
        MGeoCommand::DirectModelingOperation m_currentOperation =
            MGeoCommand::DirectModelingOperation::None;
        // true 时选中面只提供移动方向，实际预览/提交对象为该面唯一所属的 Solid/CompSolid 实体。
        // 这是单次会话状态，resetSession 后必须恢复 false，不能污染下一次普通移动面操作。
        bool m_moveSolid = false;

        // 当前累计参数（移动面为 mm，旋转面为度，圆角为目标绝对半径）。
        double m_currentParameter = 0.0;

        // 参数计算器由 Controller 创建和销毁；Panel 与几何层都不拥有它。
        std::unique_ptr<mGeoFeatureParameterCalculator> m_parameterCalculator;
        // 保存全部“确定”操作的有序历史；纯数值确认也会自动建参并进入重建链。
        std::vector<std::unique_ptr<DrivenOperation>> m_drivenOperations;
        // 只有 JSON 脚本 original-ID 模式会创建这些上下文。Panel、普通 API 和
        // 其他几何编辑路径从不读写该容器，因此不会改变现有 ID 行为。
        //std::vector<std::unique_ptr<ScriptInitialTopologyIdContext>>
        //    m_scriptInitialTopologyIdContexts; //【OCC摘除·S10/P0-1 终局】同上
        // executeCommand 的单对象脚本调用在 onCommit 期间临时设置。它把自动参数
        // 绑定到由 OCC History 保留的业务 ID，避免 TopExp 序号重排碰撞。
        QString m_commandParameterObjectKeyOverride;

        // 替换面的目标面或旋转面的显式轴边。仅在当前拾取会话内保存；历史记录会把
        // 同部件引用转换为拓扑序号，外部部件引用则保留其 TopoDS_Shape。
        //TopoDS_Shape m_referenceShape; //【OCC摘除·S10/P0-1 终局】
        //bool m_referenceShapeBelongsToCurrentPart = false;

        // 由几何层分析得到的零件局部坐标；Panel 原样交给渲染工具创建箭头/旋转环。
        QVector3D m_arrowCenter = QVector3D(0.0f, 0.0f, 0.0f);
        QVector3D m_arrowDirection = QVector3D(0.0f, 0.0f, 1.0f);
        //【S11·旋转面】当前会话旋转轴（面中轴线或直线边；提交时随请求下发，
        // 旋转预览圆环也锚定在该轴上——beginRotateFaceOperation 会同步写入
        // m_arrowCenter/m_arrowDirection）。
        QVector3D m_rotationAxisPoint = QVector3D(0.0f, 0.0f, 0.0f);
        QVector3D m_rotationAxisDirection = QVector3D(0.0f, 0.0f, 1.0f);

        // 圆角等拓扑型操作使用空闲防抖，避免连续输入期间反复启动昂贵 OCC 重建。
        double m_pendingPreviewDistance = 0.0;
        bool m_previewPending = false;
        static constexpr int kTopologyPreviewDebounceMs = 250;

        // 连续输入停止后消费最后一个 pending 参数；QObject 由 Controller 管理。
        QTimer* m_previewConsumeTimer = nullptr;

        // OCC 预览只在专用工作线程串行执行；同一操作内复用只读几何快照。
        //std::unique_ptr<DirectModelingPreviewWorker> m_previewWorker; //【OCC摘除·S10/P0-1 终局】类型定义随原实现保档（#if 0），壳无预览线程
        //std::shared_ptr<const MGeoCommand::DirectModelingGeometry>
        //    m_previewGeometrySnapshot; //【OCC摘除·S10/P0-1 终局】
        quint64 m_previewSessionId = 0; ///< 换选择/取消/提交时递增。
        quint64 m_previewSequence = 0;  ///< 同一选择下每次请求递增。

    };
}
