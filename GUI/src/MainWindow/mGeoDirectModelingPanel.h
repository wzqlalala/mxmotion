#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mGeoPanelBase.h"

#include <QLineEdit>
#include <QMetaObject>

#include <memory>
#include <vector>

class QComboBox;
class QCheckBox;
class QTableWidget;
class QTimer;

namespace MechMainWindow
{
    class DirectModelingController;
    class DirectModelingRenderAdapter;

    /**
     * @brief 直接建模面板（可选精确数值输入视图）
     *
     * 改造说明：
     * DirectModelingController 负责几何会话、精确预览与提交；DirectTool 负责箭头/圆环交互。
         * 单面/偏置双面平移由适配器调用矩阵绘制；旋转源面通过独立接口配置到 Tool，拖动时
     * 即时旋转、释放后更新精确预览。替换和圆角显示后台结果，不访问 Tool 私有状态。
     *
     * 重要语义：编辑框回车、箭头拖动和拖动结束都只更新预览；只有当前功能块
     * 自己的“确定”按钮会调用 Controller::onCommit 修改正式几何。提交成功后才
     * 清空预览、操作器、拾取 ID 和全部高亮，失败时保留现场供用户修改参数重试。
     */
    class MMAINWINDOW_EXPORT mGeoDirectModelingPanel : public mGeoPanelBase
    {
        Q_OBJECT

    public:
        enum class DirectModelingType
        {
            ReplaceFace, ///< 主面 + 目标面 + 可选偏置距离。
            RotateFace,  ///< 旋转面 + 可选直线轴 + [-90°, 90°] 角度。
            OffsetFace,  ///< 单面 + 有符号移动距离；勾选偏置后自动联动相对平面。
            ResizeFillet ///< 单个圆角面或共享边 + 目标绝对半径。
        };
        Q_ENUM(DirectModelingType)

            /*
            * 构造函数
            */
            mGeoDirectModelingPanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mGeoDirectModelingPanel() override;

        /** 连接当前活动视口的拖动/拖动结束信号；调用前会断开旧视口连接。 */
        void connectSignal();

        /** 断开由 connectSignal 保存的两个渲染拖动连接。 */
        void disConnectSignal();

        /**
         * 根据工具栏命令显示单个直接建模功能块，并始终附带全局参数表折叠块。
         * GeoOperationType 需提供 GeoDirectModelingReplaceFace、
         * GeoDirectModelingRotateFace、GeoDirectModelingOffsetFace 和
         * GeoDirectModelingResizeFillet；旧 GeoDirectModeling 兼容为 OffsetFace。
         */
        void showFold(GeoOperationType type);

        /** 允许已有调用方直接切换类型；常规入口应通过 showFold(GeoOperationType)。 */
        void setDirectModelingType(DirectModelingType type);
        DirectModelingType directModelingType() const { return m_currentModelingType; }

        /**
        * 外部统一退出入口（ESC 清拾取等）：结束当前会话，清预览与箭头，
        * 面板回到待拾取状态；与"取消"按钮同语义。面板被切换/隐藏的退出
        * 走 hideEvent，不经过本接口。
        */
        void exitDirectModelingSession();

        /*
        * 进入面板时连接拾取、单位制、控制器等全局信号（mInfoWindow::createGeoDirectModeling 调用）
        */
        void connectGeoSignals() override;

        /*
        * 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
        */
        void disconnectGeoSignals() override;

    protected:
        /*
        * 面板切换（setMainWidget 换主件会显式 hide 旧面板）与 Info 停靠窗关闭
        * 都以 hide 为统一边界：结束直接建模模式并断开全局信号，防止箭头/预览
        * 残留到其他面板会话。
        */
        void hideEvent(QHideEvent* event) override;

    private:
        enum class PickTarget
        {
            None,               ///< 暂停处理选择容器通知。
            ReplaceSourceFace,  ///< 替换面第一步：待修改主面。
            ReplaceTargetFace,  ///< 替换面第二步：定位参照面。
            RotateSourceFace,   ///< 旋转面主选择。
            RotateAxisEdge,     ///< 可选的直线旋转轴。
            OperationSelection  ///< 移动面/圆角的单对象选择。
        };

        /*
        * 初始化替换面/旋转面/移动面/圆角调整四个操作折叠块及全局参数表
        */
        void initDirectModelingPanel();
        /** 切换当前拾取步骤，并同步底层 PickFilter；可按需清除旧步骤状态。 */
        void activatePickTarget(PickTarget target, bool resetSelection = true);
        /** 断开拖动与当前视口拾取的全部直接建模会话级连接。 */
        void disconnectRuntimeSignals();
        /** 修改隐藏的基础拾取控件，以复用项目已有的面/边拾取过滤逻辑。 */
        void setPickType(const QString& type);
        /** 清空所有功能块的拾取 ID、只读框、参数框和防抖计时器。 */
        void resetSelectionState();
        /** 仅在 commitFinished(true) 后调用，完成渲染、选择和界面状态清理。 */
        void resetPanelAfterCommit();
        /** Controller 退出时断开面板运行期信号并结束拾取会话。 */
        void handleDirectModelingModeChanged(bool active);
        /** 从 SelectionDataContainer 移除面/边/点/体全部高亮。 */
        void clearAllHighlights();
        PickTarget defaultPickTarget() const;

        /*
        * 参数输入框回车：仅更新预览，不修改几何数据
        */
        void slot_distanceEditingFinished();
        /** 统一预览入口：累计值先排队渲染；实体仅平移，其他操作按需请求精确结果。 */
        void updateCurrentPreview(double accumulatedValue, bool finished);
        /** 仅供移动面界面使用；偏置启用时建立双面会话，不进入脚本 API。 */
        bool beginMoveFaceUiOperation(int faceId, bool pairedOffset);
        /** 已拾面后切换偏置：重新识别联动面并重建预览，失败恢复原会话。 */
        void slot_pairedOffsetToggled(bool checked);
        /** 显示主面和自动联动面的 ID，不向全局拾取容器追加自动选择。 */
        void updateMoveFacePickDisplay();

        /*
        * 取消按钮：转发到 Controller 取消当前会话
        */
        void slot_cancelClicked();
        void slot_replaceFaceConfirmClicked();
        void slot_rotateFaceConfirmClicked();
        void slot_offsetFaceConfirmClicked();
        void slot_resizeFilletConfirmClicked();
        bool commitParameterOperation(DirectModelingType type, QLineEdit* editor);
        QLineEdit* currentParameterEdit() const;
        /** 将数值文本或公式（可带 '='）统一计算为几何接口需要的有限 double。 */
        bool evaluateParameterEditor(
            QLineEdit* editor, double& value, QString* errorMessage = nullptr) const;
        /**
         * 从表格收集已有参数的名称和公式，并让 Controller 原子更新依赖图；
         * 参数仍只能由直接建模“确定”自动创建，本接口不创建或删除参数。
         */
        bool applyParameterTable(bool showMessage);
        /** 使用 Controller 中的计算结果刷新“当前值/状态”两列。 */
        void refreshParameterTable();
        /** 参数表变化后，若当前操作输入的是公式，则立即请求新的精确预览。 */
        void updatePreviewFromDrivenExpression();

        /*
        * 捕获窗口激活（保留基础拾取高亮连接）
        */
        void slot_subWindowActivated();

        /*
        * 高亮几何数据变化（保留基础拾取类型识别与输出）
        */
        void slot_highlightGeoData();

        /*
        * 根据当前拾取结果输出几何类型（保留信息提示，不做会话记录）
        */
        /** 将严格单选结果按当前 PickTarget 分发给替换、旋转、移动面或圆角流程。 */
        void processPickResult(const QString& type, const std::vector<int>& ids);
        /**
         * 普通操作通过 pickedGeoEleID 启动 Tool，源顶点缓存于适配器。
         * 旋转单独配置源面/旋转轴并显示圆环；替换面仍隐藏操作器。
         */
        bool configureCurrentRenderOperation(
            int partId, int elementId, int elementDim, bool showManipulator);

        // 参数输入框：用户输入的是累计/绝对参数，不是单帧拖动增量。
        QLineEdit* m_offsetDistanceEdit = nullptr;
        // 保留原控件成员名以减少内网界面合并改动；“偏置”现在是双面同向联动，
        // 不再代表整个实体平移。API 的 moveSolid 字段及其语义不受该控件影响。
        QCheckBox* m_moveSolidCheckBox = nullptr;
        QLineEdit* m_filletRadiusEdit = nullptr;
        QLineEdit* m_rotateAngleEdit = nullptr;
        QTimer* m_rotatePreviewTimer = nullptr;
        QLineEdit* m_replaceOffsetEdit = nullptr;
        QTimer* m_replacePreviewTimer = nullptr;
        // 只读拾取显示框。真实选择以对应 ID 成员和数据层解析结果为准。
        QLineEdit* m_replaceSourcePickEdit = nullptr;
        QLineEdit* m_replaceTargetPickEdit = nullptr;
        QLineEdit* m_rotateSourcePickEdit = nullptr;
        QLineEdit* m_rotateAxisPickEdit = nullptr;
        QLineEdit* m_offsetPickEdit = nullptr;
        QLineEdit* m_filletPickEdit = nullptr;
        QComboBox* m_filletPickTypeCombo = nullptr;

        // 全局参数表：参数由“确定”自动创建；名称和公式可编辑，当前值和状态只读。
        // 公式中的参数引用必须使用 {参数名}，函数与内置常量仍沿用原语法。
        QTableWidget* m_parameterTable = nullptr;
        QTimer* m_parameterApplyTimer = nullptr;
        bool m_refreshingParameterTable = false;

        // 当前活动视口的拾取容器连接；退出直接建模时必须断开。
        QMetaObject::Connection conn_geo;

        // 视口切换信号的连接（connectGeoSignals 建立）
        QMetaObject::Connection conn_viewportChanged;

        // 几何会话控制器。渲染/UI 接线由本 Panel 统一持有。
        DirectModelingController* m_controller = nullptr;
        std::unique_ptr<DirectModelingRenderAdapter> m_renderAdapter;

        // 当前活动渲染视口的拖动信号连接，避免重复 showFold 累积连接。
        QMetaObject::Connection m_stretchConnection;
        QMetaObject::Connection m_stretchFinishConnection;
        // 标识已经完成运行期接线的实际视口。同一 QMdi 子窗口被重复激活时，
        // 不得再次清空选择和“偏置”复选框；真正切换视口时才重建会话。
        const void* m_runtimeViewportKey = nullptr;

        DirectModelingType m_currentModelingType = DirectModelingType::OffsetFace;
        PickTarget m_activePickTarget = PickTarget::OperationSelection;
        // 这些 ID 只在本次未提交会话内有效；几何提交后拓扑 ID 会重建。
        int m_replaceSourceFaceId = -1;
        int m_replaceTargetFaceId = -1;
        int m_rotateSourceFaceId = -1;
        int m_rotateAxisEdgeId = -1;
        // 移动面/圆角折叠块当前的单选对象，提交 API 时使用。
        int m_operationElementId = -1;
        int m_operationElementDim = -1;

        // 最后一次有效的累计参数；只有对应折叠块的“确定”按钮才提交。
        double m_lastDistance = 0.0;
        // 当前这一段鼠标拖动开始前的累计参数。当前 DirectTool 的两个信号传出的是
        // “本段增量”（旋转为度、其余为距离），Panel 用 base + delta 还原累计参数。
        double m_dragBaseParameter = 0.0;
        bool m_dragInProgress = false;

    private slots:
        /** 拖动中：把 DirectTool 的本段增量换算为累计值并同步到 Controller。 */
        void slot_DirectGeoModelingStretch(double distance);
        /** 拖动结束：以累计值请求一次精确预览，仍不提交正式几何。 */
        void slot_DirectGeoModelingStretchFinish(double distance);
    };
}
