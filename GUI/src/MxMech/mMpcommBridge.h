#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <cstdint>

#include <mpcomm/simple.hpp>

namespace mpcomm::simple
{
    class SoftwareAdapter;
}

namespace MxMech
{

    /**
     * @brief 结构软件与多物理平台的 mpcomm 通信桥
     * @details GUI 软件（SoftwareAdapter）角色，
     *          节点状态机 start->opened->committed/changed->closed，
     *          revision 自增；committed/changed 只发 editableCells 内的角色。
     *          结构单元格映射：geometry=几何、mesh+setup=计算文件(.mx，导入计算
     *          文件时触发；给平台存储时同一文件挂 mesh+setup 两个角色)、
     *          solution+results=求解结果。
     *          OPEN_NODE 经 data.context.node.templateId 判定分析类型并创建
     *          对应工况（静态结构/瞬态/模态/谐响应/响应谱/随机振动/屈曲）。
     */
    class MpcommBridge : public QObject
    {
        Q_OBJECT

    public:
        explicit MpcommBridge(QObject* parent = nullptr);

        // 必须在 SoftwareAdapter::start() 之后调用，内部注册命令表与回调。
        void setAdapter(mpcomm::simple::SoftwareAdapter* adapter);

        // 向平台发送一条应用日志（topic=application.log）。
        void sendLog(const QString& text,
            const QString& level = QStringLiteral("info"));

    public slots:
        // CLOSE 命令到达：在主线程执行关闭流程。
        void onClose();

        // 几何导入完成：回传 committed(geometry)。
        void onImportGeometryFinished(const QString& geometryPath);

        // 计算文件（.mx）导入完成：回传 committed(mesh+setup)。
        void onImportCalcFileFinished(bool success, const QString& filePath);

        // 计算文件（.mx）手动导出完成：回传 committed(mesh+setup)。
        // 仅手动导出走此入口；求解流程的自动导出不回报（求解完成会单独
        // 回报 solution+results，避免重复刷新 mesh/setup 行状态）。
        void onExportCalcFileFinished(bool success, const QString& filePath);

        // 几何模型手动导出完成（stp/step/iges）：回传 committed(geometry)。
        void onExportGeometryFinished(const QString& filePath);

        // 求解状态变化：started/finished/failed。
        void onSolverStatusChanged(const QString& state,
            const QVariantMap& outputs);

        // 模型被删除：通知平台清空所有单元格状态。
        void onModelCleared();

    private:
        friend mpcomm::simple::Reply handleMpcommCommand(
            const mpcomm::simple::Command& command,
            void* userData);

        mpcomm::simple::Reply handleCommand(
            const mpcomm::simple::Command& command);

        void sendApplicationLog(const QString& topic,
            const QString& text,
            const QString& level) const;

        // OPEN_NODE/REFRESH_INPUTS 的 context 解析：editableCells、
        // exchangeId、contextRevision、node.templateId、data.files[] 中的文件路径。
        bool readContext(const mpcomm::StructuredValue& context,
            QString& error,
            QStringList* outSharedCells = nullptr);

        // OPEN_NODE 主线程处理。
        void onOpenNode(const QString& operation,
            const mpcomm::StructuredValue& context);

        // mxsim.node 事件：opened/failed/closed。
        bool sendNodeEvent(const QString& event,
            const QString& text,
            const QString& level = QStringLiteral("info"));

        // mxsim.node 事件：changed（changed_cells 过滤 editableCells）。
        bool sendChanged(const QStringList& cells, const QString& text);

        // mxsim.node 事件：committed（committed_cells 过滤 editableCells，
        // changes 携带文件引用）。
        bool sendCommitted(const QStringList& committedCells,
            const mpcomm::StructuredValue& changes,
            const QString& text);

        // mxsim.node 事件：清空角色（cleared_cells 过滤 editableCells）。
        // 删除模型/输入时通知平台对应单元格清空。
        bool sendCleared(const QStringList& clearedCells,
            const QString& text);

        // 上游共享输入已就绪：按收到的角色回报 committed。
        void markSharedInputsCommitted(const QStringList& sharedRoles);

        mpcomm::StructuredValue makeFileRef(const QString& path) const;

        mpcomm::simple::SoftwareAdapter* adapter_ = nullptr;
        std::int64_t revision_ = 0;
        bool hasSession_ = false;

        QString contextRevision_;
        QString templateId_;     // context.node.templateId（分析类型判定）
        QStringList editableCells_;

        QString geometryPath_;
        QString calcFilePath_;   // role=mesh+setup（计算文件 .mx）
        QStringList resultPaths_;   // context 带入的结果文件列表（role=result/results）
        QStringList resultFiles_; // 求解结果文件列表（*.mxdb/*.mxdb0/*.op2）
        QStringList mpifPaths_;   // context 带入的上游场数据文件列表（role=mpif）
        QStringList couplingPaths_; // context 带入的通用耦合文件列表（role=coupling）
    };

    // SoftwareAdapter 的命令入口（自由函数，签名匹配 CommandHandler）。
    mpcomm::simple::Reply handleMpcommCommand(
        const mpcomm::simple::Command& command,
        void* userData);

} // namespace MxMech
