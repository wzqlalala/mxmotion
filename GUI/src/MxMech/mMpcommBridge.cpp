//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMpcommBridge.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>

#include "mMechGlobalSignals.h"

// 调试日志开关：默认不向平台发 application.log（排障时在编译定义里加
// MPCOMM_ENABLE_DEBUG_LOG 打开，例如 target_compile_definitions 或手动
// 临时取消下一行注释）
// #define MPCOMM_ENABLE_DEBUG_LOG

#ifndef MPCOMM_ENABLE_DEBUG_LOG
#define MPCOMM_SEND_LOG(bridge, ...)  do {} while (0)
#else
#define MPCOMM_SEND_LOG(bridge, ...)  (bridge)->sendLog(__VA_ARGS__)
#endif

namespace
{
    QString fromUtf8(const std::string& text)
    {
        return QString::fromUtf8(
            text.data(),
            static_cast<int>(text.size()));
    }

    std::string toUtf8(const QString& text)
    {
        const QByteArray bytes = text.toUtf8();
        return std::string(bytes.constData(),
            static_cast<std::size_t>(bytes.size()));
    }

    // 单元格角色白名单（与平台 editable_cells 一致）
    bool isKnownCell(const QString& cell)
    {
        return cell == QStringLiteral("geometry")
            || cell == QStringLiteral("mesh")
            || cell == QStringLiteral("setup")
            || cell == QStringLiteral("solution")
            || cell == QStringLiteral("results")
            || cell == QStringLiteral("mpif");
    }
}

namespace MxMech
{

    MpcommBridge::MpcommBridge(QObject* parent)
        : QObject(parent)
    {
        auto* sigs = MechCommon::mMechGlobalSignals::getInstance();

        connect(
            sigs,
            &MechCommon::mMechGlobalSignals::mpcommImportGeometryFinished,
            this,
            &MpcommBridge::onImportGeometryFinished);

        connect(
            sigs,
            &MechCommon::mMechGlobalSignals::mpcommImportCalcFileFinished,
            this,
            &MpcommBridge::onImportCalcFileFinished);

        // 手动导出计算文件完成：回传 committed(mesh+setup)
        connect(
            sigs,
            &MechCommon::mMechGlobalSignals::mpcommExportCalcFileFinished,
            this,
            &MpcommBridge::onExportCalcFileFinished);

        // 手动导出几何完成：回传 committed(geometry)
        connect(
            sigs,
            &MechCommon::mMechGlobalSignals::mpcommExportGeometryFinished,
            this,
            &MpcommBridge::onExportGeometryFinished);

        connect(
            sigs,
            &MechCommon::mMechGlobalSignals::mpcommSolverStatusChanged,
            this,
            &MpcommBridge::onSolverStatusChanged);

        // 删除模型：通知平台清空单元格
        connect(
            sigs,
            &MechCommon::mMechGlobalSignals::mpcommModelCleared,
            this,
            &MpcommBridge::onModelCleared);
    }

    void MpcommBridge::setAdapter(mpcomm::simple::SoftwareAdapter* adapter)
    {
        adapter_ = adapter;

        if (!adapter_) {
            return;
        }

        adapter_->set_commands({
            "OPEN_NODE",
            "REFRESH_INPUTS",
            "GET_STATE",
            "CLOSE",
            "EXECUTE_ACTION"
            });

        adapter_->on_command(
            &MxMech::handleMpcommCommand,
            this);
    }

    void MpcommBridge::sendLog(const QString& text, const QString& level)
    {
        sendApplicationLog(
            QStringLiteral("application.log"), text, level);
    }

    void MpcommBridge::sendApplicationLog(
        const QString& topic,
        const QString& text,
        const QString& level) const
    {
        if (!adapter_ || text.isEmpty()) {
            return;
        }

        mpcomm::simple::Message message;
        message.topic = toUtf8(topic);
        message.text = toUtf8(text);
        message.level = toUtf8(level);

        if (!adapter_->send_message(message)) {
            qWarning()
                << "[mpcomm] log send failed:"
                << fromUtf8(adapter_->error());
        }
    }

    // ------------------------------------------------------------------
    // 会话与状态机
    // ------------------------------------------------------------------

    bool MpcommBridge::sendNodeEvent(
        const QString& event,
        const QString& text,
        const QString& level)
    {
        if (!adapter_ || !hasSession_) {
            return false;
        }

        const std::int64_t nextRevision = revision_ + 1;

        mpcomm::simple::Message message;
        message.topic = "mxsim.node";
        message.level = toUtf8(level);
        message.text = toUtf8(text.isEmpty()
            ? QStringLiteral("event: %1").arg(event)
            : text);
        message.data = mpcomm::StructuredValue::object({
            {"event", mpcomm::StructuredValue::string(toUtf8(event))},
            {"revision", mpcomm::StructuredValue::integer(nextRevision)}
            });

        if (!adapter_->send_message(message)) {
            qWarning()
                << "[mpcomm] event send failed:"
                << event
                << fromUtf8(adapter_->error());
            return false;
        }

        revision_ = nextRevision;
        return true;
    }

    bool MpcommBridge::sendChanged(
        const QStringList& cells,
        const QString& text)
    {
        if (!adapter_ || !hasSession_) {
            return false;
        }

        QStringList writable;
        for (const QString& cell : cells) {
            if (editableCells_.contains(cell)) {
                writable.append(cell);
            }
        }
        if (writable.isEmpty()) {
            return false;
        }

        const std::int64_t nextRevision = revision_ + 1;

        mpcomm::StructuredValue::Array cellArray;
        for (const QString& cell : writable) {
            cellArray.push_back(
                mpcomm::StructuredValue::string(toUtf8(cell)));
        }

        mpcomm::simple::Message message;
        message.topic = "mxsim.node";
        message.level = "info";
        message.text = toUtf8(text.isEmpty()
            ? QStringLiteral("changed")
            : text);
        message.data = mpcomm::StructuredValue::object({
            {"event", mpcomm::StructuredValue::string("changed")},
            {"revision", mpcomm::StructuredValue::integer(nextRevision)},
            {"changed_cells",
             mpcomm::StructuredValue::array(std::move(cellArray))}
            });

        if (!adapter_->send_message(message)) {
            qWarning()
                << "[mpcomm] changed send failed:"
                << fromUtf8(adapter_->error());
            return false;
        }

        revision_ = nextRevision;
        return true;
    }

    mpcomm::StructuredValue MpcommBridge::makeFileRef(
        const QString& path) const
    {
        return mpcomm::StructuredValue::object({
            {"path",
             mpcomm::StructuredValue::string(
                 toUtf8(QFileInfo(path).absoluteFilePath()))}
            });
    }

    bool MpcommBridge::sendCommitted(
        const QStringList& committedCells,
        const mpcomm::StructuredValue& changes,
        const QString& text)
    {
        if (!adapter_ || !hasSession_) {
            return false;
        }

        // 只提交有权限的角色；全部被过滤则整包不发。
        QStringList committed;
        for (const QString& cell : committedCells) {
            if (editableCells_.contains(cell)) {
                committed.append(cell);
            }
            else {
                qWarning()
                    << "[mpcomm] skip read-only cell:" << cell;
            }
        }
        if (committed.isEmpty()) {
            return false;
        }

        const std::int64_t nextRevision = revision_ + 1;

        mpcomm::StructuredValue::Array cellArray;
        for (const QString& cell : committed) {
            cellArray.push_back(
                mpcomm::StructuredValue::string(toUtf8(cell)));
        }

        mpcomm::StructuredValue::Object payload;
        payload.emplace_back("event",
            mpcomm::StructuredValue::string("committed"));
        payload.emplace_back("revision",
            mpcomm::StructuredValue::integer(nextRevision));
        payload.emplace_back("committed_cells",
            mpcomm::StructuredValue::array(std::move(cellArray)));
        if (changes.type == mpcomm::StructuredValue::Type::Object
            && !changes.object_value.empty()) {
            payload.emplace_back("changes", changes);
        }

        mpcomm::simple::Message message;
        message.topic = "mxsim.node";
        message.level = "info";
        message.text = toUtf8(text.isEmpty()
            ? QStringLiteral("committed")
            : text);
        message.data =
            mpcomm::StructuredValue::object(std::move(payload));

        if (!adapter_->send_message(message)) {
            qWarning()
                << "[mpcomm] committed send failed:"
                << fromUtf8(adapter_->error());
            return false;
        }

        revision_ = nextRevision;
        return true;
    }

    // ------------------------------------------------------------------
    // 主动回报入口
    // ------------------------------------------------------------------

    void MpcommBridge::onImportGeometryFinished(const QString& geometryPath)
    {
        // 所有导入（含 OPEN_NODE 平台触发的）都回报 committed；
        // 平台侧对同值重复提交有幂等处理。
        if (!hasSession_) {
            return;
        }
        if (geometryPath.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("geometry import finished without path"),
                QStringLiteral("warning"));
            return;
        }

        geometryPath_ = QFileInfo(geometryPath).absoluteFilePath();

        mpcomm::StructuredValue changes =
            mpcomm::StructuredValue::object({
                {"geometry",
                 mpcomm::StructuredValue::object({
                     {"files",
                      mpcomm::StructuredValue::array({
                          makeFileRef(geometryPath_)
                          })}
                     })}
                });

        sendCommitted({QStringLiteral("geometry")},
            changes,
            QStringLiteral("geometry committed"));
    }

    void MpcommBridge::onImportCalcFileFinished(
        bool success,
        const QString& filePath)
    {
        // 不做回声抑制：所有导入完成（含平台触发的）都回报 committed，
        // 平台对同值重复提交有幂等处理。
        if (!hasSession_) {
            return;
        }
        if (!success || filePath.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("calc file import failed: %1")
                .arg(filePath),
                QStringLiteral("error"));
            return;
        }

        calcFilePath_ = QFileInfo(filePath).absoluteFilePath();

        // 平台 schema：mesh 角色用 path 字段（CFD 同款），setup 用
        // projectFiles 数组；mesh 不接受 files 数组。
        // 结构软件：计算文件(.mx)同时挂 mesh+setup 两个角色存储给平台。
        mpcomm::StructuredValue::Object meshChange;
        meshChange.emplace_back("path",
            mpcomm::StructuredValue::string(toUtf8(calcFilePath_)));

        mpcomm::StructuredValue::Object setupChange;
        setupChange.emplace_back("projectFiles",
            mpcomm::StructuredValue::array({
                makeFileRef(calcFilePath_)
                }));

        mpcomm::StructuredValue changes =
            mpcomm::StructuredValue::object({
                {"mesh",
                 mpcomm::StructuredValue::object(std::move(meshChange))},
                {"setup",
                 mpcomm::StructuredValue::object(std::move(setupChange))}
                });

        sendCommitted({QStringLiteral("mesh"), QStringLiteral("setup")},
            changes,
            QStringLiteral("calc file committed"));
    }

    void MpcommBridge::onExportCalcFileFinished(
        bool success,
        const QString& filePath)
    {
        if (!hasSession_) {
            return;
        }
        if (!success || filePath.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("calc file export failed: %1")
                .arg(filePath),
                QStringLiteral("error"));
            return;
        }

        const QString absPath = QFileInfo(filePath).absoluteFilePath();

        mpcomm::StructuredValue::Object meshChange;
        meshChange.emplace_back("path",
            mpcomm::StructuredValue::string(toUtf8(absPath)));

        mpcomm::StructuredValue::Object setupChange;
        setupChange.emplace_back("projectFiles",
            mpcomm::StructuredValue::array({
                makeFileRef(absPath)
                }));

        mpcomm::StructuredValue changes =
            mpcomm::StructuredValue::object({
                {"mesh",
                 mpcomm::StructuredValue::object(std::move(meshChange))},
                {"setup",
                 mpcomm::StructuredValue::object(std::move(setupChange))}
                });

        sendCommitted({QStringLiteral("mesh"), QStringLiteral("setup")},
            changes,
            QStringLiteral("calc file exported"));
    }

    void MpcommBridge::onExportGeometryFinished(const QString& filePath)
    {
        if (!hasSession_) {
            return;
        }
        if (filePath.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("geometry export finished without path"),
                QStringLiteral("warning"));
            return;
        }

        const QString absPath = QFileInfo(filePath).absoluteFilePath();

        mpcomm::StructuredValue changes =
            mpcomm::StructuredValue::object({
                {"geometry",
                 mpcomm::StructuredValue::object({
                     {"files",
                      mpcomm::StructuredValue::array({
                          makeFileRef(absPath)
                          })}
                     })}
                });

        sendCommitted({QStringLiteral("geometry")},
            changes,
            QStringLiteral("geometry exported"));
    }

    void MpcommBridge::onSolverStatusChanged(
        const QString& state,
        const QVariantMap& outputs)
    {
        if (!hasSession_) {
            return;
        }

        const QString normalized = state.trimmed().toLower();

        if (normalized == QStringLiteral("started")) {
            sendChanged({QStringLiteral("solution")},
                QStringLiteral("solver started"));
            return;
        }

        if (normalized == QStringLiteral("failed")) {
            sendChanged({QStringLiteral("solution")},
                QStringLiteral("solver failed"));
            return;
        }

        if (normalized == QStringLiteral("finished")) {
            // 结果文件列表（具体文件路径）；兼容旧调用方仍传单个 resultPath。
            const QStringList resultFiles =
                outputs.value(QStringLiteral("resultFiles")).toStringList();
            if (!resultFiles.isEmpty()) {
                resultFiles_ = resultFiles;
            }
            else {
                const QString resultPath =
                    outputs.value(QStringLiteral("resultPath")).toString();
                if (!resultPath.isEmpty()) {
                    resultFiles_ = QStringList{ QFileInfo(resultPath).absoluteFilePath() };
                }
            }

            mpcomm::StructuredValue::Object changesObj;
            if (!resultFiles_.isEmpty()) {
                // 平台 schema：solution/results 用 resultFiles（CFD 同款，
                // 不接受 files）。results 可附加 mpifFiles（后续耦合用）。
                // 多文件时平台要求每项提供唯一 id（"resultFiles 含多个文件时
                // 每项都必须提供唯一 id"），按文件名去重后生成 file-0/1/...。
                mpcomm::StructuredValue::Array fileArray;
                int fileIndex = 0;
                for (const QString& file : resultFiles_) {
                    mpcomm::StructuredValue::Object ref;
                    ref.emplace_back("id",
                        mpcomm::StructuredValue::string(
                            "file-" + std::to_string(fileIndex++)));
                    ref.emplace_back("path",
                        mpcomm::StructuredValue::string(
                            toUtf8(QFileInfo(file).absoluteFilePath())));
                    fileArray.push_back(
                        mpcomm::StructuredValue::object(std::move(ref)));
                }
                const mpcomm::StructuredValue filesValue =
                    mpcomm::StructuredValue::array(fileArray);
                changesObj.emplace_back(
                    "solution",
                    mpcomm::StructuredValue::object({
                        {"resultFiles", filesValue}
                        }));
                changesObj.emplace_back(
                    "results",
                    mpcomm::StructuredValue::object({
                        {"resultFiles", filesValue}
                        }));
            }

            sendCommitted(
                {QStringLiteral("solution"), QStringLiteral("results")},
                mpcomm::StructuredValue::object(std::move(changesObj)),
                QStringLiteral("solver finished"));
            return;
        }

        MPCOMM_SEND_LOG(this, QStringLiteral("solver status: %1").arg(state));
    }

    bool MpcommBridge::sendCleared(
        const QStringList& clearedCells,
        const QString& text)
    {
        if (!adapter_ || !hasSession_) {
            return false;
        }

        // 只清有权限的角色；全部被过滤则整包不发。
        QStringList cleared;
        for (const QString& cell : clearedCells) {
            if (editableCells_.contains(cell)) {
                cleared.append(cell);
            }
        }
        if (cleared.isEmpty()) {
            return false;
        }

        const std::int64_t nextRevision = revision_ + 1;

        mpcomm::StructuredValue::Array cellArray;
        for (const QString& cell : cleared) {
            cellArray.push_back(
                mpcomm::StructuredValue::string(toUtf8(cell)));
        }

        mpcomm::StructuredValue::Object payload;
        payload.emplace_back("event",
            mpcomm::StructuredValue::string("committed"));
        payload.emplace_back("revision",
            mpcomm::StructuredValue::integer(nextRevision));
        payload.emplace_back("committed_cells",
            mpcomm::StructuredValue::array({}));
        payload.emplace_back("cleared_cells",
            mpcomm::StructuredValue::array(std::move(cellArray)));

        mpcomm::simple::Message message;
        message.topic = "mxsim.node";
        message.level = "info";
        message.text = toUtf8(text.isEmpty()
            ? QStringLiteral("cells cleared")
            : text);
        message.data =
            mpcomm::StructuredValue::object(std::move(payload));

        if (!adapter_->send_message(message)) {
            qWarning()
                << "[mpcomm] cleared send failed:"
                << fromUtf8(adapter_->error());
            return false;
        }

        revision_ = nextRevision;
        return true;
    }

    void MpcommBridge::onModelCleared()
    {
        // 删除模型后，清空本地输入/结果路径并通知平台清空全部单元格。
        geometryPath_.clear();
        calcFilePath_.clear();
        resultPaths_.clear();
        resultFiles_.clear();
        mpifPaths_.clear();
        couplingPaths_.clear();
        sendCleared(
            {QStringLiteral("geometry"),
             QStringLiteral("mesh"),
             QStringLiteral("setup"),
             QStringLiteral("solution"),
             QStringLiteral("results")},
            QStringLiteral("model cleared"));
    }

    void MpcommBridge::markSharedInputsCommitted(const QStringList& sharedRoles)
    {
        if (!hasSession_ || sharedRoles.isEmpty()) {
            return;
        }

        // 上游共享输入已就绪：把收到的角色回报为 committed，让平台
        // 更新这些单元格状态（上游已共享的几何/计算文件要显示就绪）。
        mpcomm::StructuredValue::Object changesObj;
        if (sharedRoles.contains(QStringLiteral("geometry"))
            && !geometryPath_.isEmpty()) {
            changesObj.emplace_back("geometry",
                mpcomm::StructuredValue::object({
                    {"files", mpcomm::StructuredValue::array({
                        makeFileRef(geometryPath_) })}
                    }));
        }
        if (sharedRoles.contains(QStringLiteral("mesh"))
            && !calcFilePath_.isEmpty()) {
            changesObj.emplace_back("mesh",
                mpcomm::StructuredValue::object({
                    {"path", mpcomm::StructuredValue::string(
                        toUtf8(calcFilePath_))}
                    }));
        }
        if (sharedRoles.contains(QStringLiteral("setup"))
            && !calcFilePath_.isEmpty()) {
            changesObj.emplace_back("setup",
                mpcomm::StructuredValue::object({
                    {"projectFiles", mpcomm::StructuredValue::array({
                        makeFileRef(calcFilePath_) })}
                    }));
        }
        if ((sharedRoles.contains(QStringLiteral("result"))
            || sharedRoles.contains(QStringLiteral("results")))) {
            // 优先用求解回报的具体结果文件列表；context 带入的列表兜底。
            QStringList files = resultFiles_;
            if (files.isEmpty()) {
                files = resultPaths_;
            }
            if (!files.isEmpty()) {
                mpcomm::StructuredValue::Array fileArray;
                int fileIndex = 0;
                for (const QString& file : files) {
                    mpcomm::StructuredValue::Object ref;
                    ref.emplace_back("id",
                        mpcomm::StructuredValue::string(
                            "file-" + std::to_string(fileIndex++)));
                    ref.emplace_back("path",
                        mpcomm::StructuredValue::string(
                            toUtf8(QFileInfo(file).absoluteFilePath())));
                    fileArray.push_back(
                        mpcomm::StructuredValue::object(std::move(ref)));
                }
                changesObj.emplace_back("results",
                    mpcomm::StructuredValue::object({
                        {"resultFiles",
                         mpcomm::StructuredValue::array(std::move(fileArray))}
                        }));
            }
        }

        if (changesObj.empty()) {
            return;
        }

        QStringList cells;
        for (const auto& kv : changesObj) {
            cells.append(QString::fromStdString(kv.first));
        }
        sendCommitted(cells,
            mpcomm::StructuredValue::object(std::move(changesObj)),
            QStringLiteral("shared inputs applied"));
    }

    void MpcommBridge::onClose()
    {
        // 主线程执行正常退出流程。
        // aboutToQuit 和平台 CLOSE 命令都会走到这里，重入安全：会话已关闭
        // （hasSession_=false）时不再重复发 closed。
        if (hasSession_) {
            sendNodeEvent(QStringLiteral("closed"),
                QStringLiteral("MxMech node closed"));
            hasSession_ = false;
        }
        MPCOMM_SEND_LOG(this, QStringLiteral("platform requested close"));
        // 已在退出流程中（aboutToQuit）时再次调 quit 无副作用
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
    }

    // ------------------------------------------------------------------
    // 命令处理
    // ------------------------------------------------------------------

    bool MpcommBridge::readContext(
        const mpcomm::StructuredValue& context,
        QString& error,
        QStringList* outSharedCells)
    {
        std::string value;

        if (context.get_string("contextRevision", value)) {
            contextRevision_ = fromUtf8(value);
        }

        if (const auto* node = context.find("node")) {
            // 分析类型判定依据：analysis.static_structural /
            // analysis.transient_structural / analysis.modal /
            // analysis.harmonic_response / analysis.response_spectrum /
            // analysis.random_vibration / analysis.buckling
            if (node->get_string("templateId", value)) {
                templateId_ = fromUtf8(value).trimmed();
            }
        }

        if (const auto* editable = context.find("editableCells")) {
            if (editable->type
                == mpcomm::StructuredValue::Type::Array) {
                editableCells_.clear();
                for (const auto& cell : editable->array_value) {
                    if (cell.type
                        == mpcomm::StructuredValue::Type::String) {
                        const QString role =
                            fromUtf8(cell.string_value);
                        if (isKnownCell(role)) {
                            editableCells_.append(role);
                        }
                    }
                }
            }
        }

        // data.files[]：按 role 提取文件路径（结构认 geometry/mesh/setup/result）。
        // 返回实际收到的角色集合（含本节点 data 和上游 connectionInputs）。
        QStringList receivedRoles;
        const auto readDataFiles = [this, &receivedRoles](const mpcomm::StructuredValue* data, bool overwrite) {
            if (!data
                || data->type
                    != mpcomm::StructuredValue::Type::Object) {
                return;
            }
            const auto* files = data->find("files");
            if (!files
                || files->type
                    != mpcomm::StructuredValue::Type::Array) {
                return;
            }
            for (const auto& file : files->array_value) {
                std::string role;
                std::string path;
                const bool hasRole = file.get_string("role", role);
                const bool hasPath = file.get_string("path", path);
                if (!hasRole || !hasPath || path.empty()) {
                    continue;
                }
                const QString qrole =
                    fromUtf8(role).trimmed().toLower();
                const QString qpath = fromUtf8(path);
                if (!receivedRoles.contains(qrole)) {
                    receivedRoles.append(qrole);
                }
                if (qrole == QStringLiteral("geometry")) {
                    if (overwrite || geometryPath_.isEmpty()) {
                        geometryPath_ = qpath;
                    }
                }
                else if (qrole == QStringLiteral("mesh")
                    || qrole == QStringLiteral("setup")) {
                    // mesh/setup 同指计算文件(.mx)
                    if (overwrite || calcFilePath_.isEmpty()) {
                        calcFilePath_ = qpath;
                    }
                }
                else if (qrole == QStringLiteral("result")
                    || qrole == QStringLiteral("results")) {
                    // 求解结果可能有多个文件，全收集
                    if (!resultPaths_.contains(qpath)) {
                        resultPaths_.append(qpath);
                    }
                }
                else if (qrole == QStringLiteral("mpif")) {
                    // 上游场数据（其他场共享关联过来的），可能有多个，全收集
                    if (!mpifPaths_.contains(qpath)) {
                        mpifPaths_.append(qpath);
                    }
                }
                else if (qrole == QStringLiteral("coupling")) {
                    // 通用耦合文件（多体载荷工况 JSON 等），不走 MPIF 场映射器，
                    // 仅记录路径，由「多体载荷导入」面板按需读取。
                    if (!couplingPaths_.contains(qpath)) {
                        couplingPaths_.append(qpath);
                    }
                }
            }
        };

        // 节点当前数据（本节点已存储的文件，overwrite=false）。
        readDataFiles(context.find("data"), false);

        // 上游连接输入优先于节点旧输入（CFD 同款：connectionInputs[].data）。
        // 这里 overwrite=true，因为 connectionInputs 才是上游最新共享的文件。
        if (const auto* inputs = context.find("connectionInputs")) {
            if (inputs->type
                == mpcomm::StructuredValue::Type::Array) {
                for (const auto& input : inputs->array_value) {
                    if (input.type
                        != mpcomm::StructuredValue::Type::Object) {
                        continue;
                    }
                    readDataFiles(input.find("data"), true);
                }
            }
        }

        error.clear();
        if (outSharedCells) {
            *outSharedCells = receivedRoles;
        }
        return true;
    }

    void MpcommBridge::onOpenNode(
        const QString& operation,
        const mpcomm::StructuredValue& context)
    {
        if (hasSession_) {
            qWarning() << "[mpcomm] duplicate OPEN_NODE ignored";
            return;
        }

        revision_ = 0;
        hasSession_ = true;

        QString error;
        QStringList sharedRoles;
        if (!readContext(context, error, &sharedRoles)) {
            sendNodeEvent(QStringLiteral("failed"),
                error,
                QStringLiteral("error"));
            hasSession_ = false;
            return;
        }

        // 分析类型判定：把 templateId 挂到 qApp 属性，界面构造完成后
        // 主动取出并创建对应工况（OPEN_NODE 先于界面构造，信号投递不可靠）。
        if (!templateId_.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("analysis templateId: %1")
                .arg(templateId_));
            qApp->setProperty("mpcomm_pending_templateId", templateId_);
        }

        // 节点已带几何（上游提交过）：平台会话中主动导入该几何文件。
        // 界面可能尚未就绪（OPEN_NODE 先于主窗口构造），先把路径
        // 挂到 QCoreApplication 属性上；界面构造完成后主动来取
        // 并触发导入，不依赖信号投递时序。平台触发的导入经 fromPlatform
        // 标记不回传 committed。
        if (!geometryPath_.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("import geometry from context: %1")
                .arg(geometryPath_));

            qApp->setProperty("mpcomm_pending_geometry", geometryPath_);
        }
        else {
            MPCOMM_SEND_LOG(this, QStringLiteral("no geometry in context"));
        }

        // 节点已带计算文件（上游网格/设置已确认）：主动导入 .mx。
        if (!calcFilePath_.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("import calc file from context: %1")
                .arg(calcFilePath_));

            qApp->setProperty("mpcomm_pending_calcfile", calcFilePath_);
        }

        // 节点已带后处理结果（上游求解完成）：打开后自动导入（可能多个）。
        // 由界面侧在几何/计算文件导入完成后再触发（结果依赖工况与求解目录）。
        if (!resultPaths_.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("import post results from context: %1 个文件")
                .arg(resultPaths_.size()));

            qApp->setProperty("mpcomm_pending_results", resultPaths_);
        }

        // 节点已带上游场数据（其他场共享关联过来）：打开后触发 mpif 处理。
        if (!mpifPaths_.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("import mpif from context: %1 个文件")
                .arg(mpifPaths_.size()));

            qApp->setProperty("mpcomm_pending_mpif", mpifPaths_);
        }

        // 节点已带通用耦合文件（多体载荷工况 JSON 等）：仅记录路径，不自动
        // 触发导入，由用户经「多体载荷导入」面板主动选择加载。
        if (!couplingPaths_.isEmpty()) {
            MPCOMM_SEND_LOG(this, QStringLiteral("coupling files from context: %1 个文件")
                .arg(couplingPaths_.size()));

            qApp->setProperty("mpcomm_pending_coupling", couplingPaths_);
        }

        // 通知界面取回挂起数据。OPEN_NODE 经队列投递，晚于界面构造时
        // 属性设置后无人消费（构造时的一次性取回已经跑过），必须再通知一次。
        emit MechCommon::mMechGlobalSignals::getInstance()
            ->mpcommPendingContextChanged();

        MPCOMM_SEND_LOG(this, QStringLiteral("OPEN_NODE received, operation=%1")
            .arg(operation));

        // 平台状态机：start -> committed 无效，必须先 opened。
        if (!sendNodeEvent(QStringLiteral("opened"),
                QStringLiteral("MxMech node opened"))) {
            hasSession_ = false;
            return;
        }

        // 上游共享的输入（几何/网格/设置）：回报为 committed，让平台
        // 把对应单元格标记为就绪。
        markSharedInputsCommitted(sharedRoles);
    }

    mpcomm::simple::Reply MpcommBridge::handleCommand(
        const mpcomm::simple::Command& command)
    {
        if (command.name == "GET_STATE") {
            return mpcomm::simple::Reply::ok(
                "running");
        }

        if (command.name == "CLOSE") {
            const bool queued =
                QMetaObject::invokeMethod(
                    this,
                    [this]() {
                        onClose();
                    },
                    Qt::QueuedConnection);

            if (!queued) {
                return mpcomm::simple::Reply::reject(
                    "QUEUE_FAILED",
                    "cannot queue CLOSE");
            }

            return mpcomm::simple::Reply::pending(
                "closing");
        }

        if (command.name == "OPEN_NODE") {
            std::string operationValue;

            const mpcomm::StructuredValue* context =
                command.data.find("context");

            if (!command.data.get_string("operation", operationValue)
                || operationValue.empty()
                || !context
                || context->type
                    != mpcomm::StructuredValue::Type::Object) {
                return mpcomm::simple::Reply::reject(
                    "INVALID_ARGUMENT",
                    "OPEN_NODE requires operation and object context");
            }

            const QString operation = fromUtf8(operationValue);
            const mpcomm::StructuredValue contextCopy = *context;

            const bool queued =
                QMetaObject::invokeMethod(
                    this,
                    [this, operation, contextCopy]() {
                        onOpenNode(operation, contextCopy);
                    },
                    Qt::QueuedConnection);

            if (!queued) {
                return mpcomm::simple::Reply::reject(
                    "QUEUE_FAILED",
                    "cannot queue OPEN_NODE");
            }

            return mpcomm::simple::Reply::pending(
                "OPEN_NODE queued");
        }

        if (command.name == "REFRESH_INPUTS") {
            // 更新会话上下文；changed_cells 含 geometry/mesh 时重导对应文件。
            const mpcomm::StructuredValue* context =
                command.data.find("context");
            std::string baseRevision;
            command.data.get_string("baseContextRevision",
                baseRevision);

            QStringList changedCells;
            if (const auto* cells = command.data.find("changed_cells")) {
                if (cells->type
                    == mpcomm::StructuredValue::Type::Array) {
                    for (const auto& cell : cells->array_value) {
                        if (cell.type
                            == mpcomm::StructuredValue::Type::String) {
                            changedCells.append(
                                fromUtf8(cell.string_value));
                        }
                    }
                }
            }

            QStringList refreshSharedRoles;
            if (context
                && context->type
                    == mpcomm::StructuredValue::Type::Object) {
                if (!fromUtf8(baseRevision).isEmpty()
                    && !contextRevision_.isEmpty()
                    && fromUtf8(baseRevision) != contextRevision_) {
                    return mpcomm::simple::Reply::reject(
                        "REFRESH_REJECTED",
                        "baseContextRevision mismatch");
                }
                QString error;
                readContext(*context, error, &refreshSharedRoles);
            }

            // 上游共享输入更新：立即回报 committed（导入完成后的回报与之
            // 同值，平台幂等处理），让平台单元格状态跟上。
            markSharedInputsCommitted(refreshSharedRoles);

            // 重导平台更新的输入；导入完成后照常回报 committed（无回声抑制）。
            if (changedCells.contains(QStringLiteral("geometry"))
                && !geometryPath_.isEmpty()) {
                MPCOMM_SEND_LOG(this, QStringLiteral("refresh: import geometry %1")
                    .arg(geometryPath_));
                emit MechCommon::mMechGlobalSignals::getInstance()
                    ->mpcommImportGeometryRequested(geometryPath_);
            }
            if ((changedCells.contains(QStringLiteral("mesh"))
                || changedCells.contains(QStringLiteral("setup")))
                && !calcFilePath_.isEmpty()) {
                MPCOMM_SEND_LOG(this, QStringLiteral("refresh: import calc file %1")
                    .arg(calcFilePath_));
                emit MechCommon::mMechGlobalSignals::getInstance()
                    ->mpcommImportCalcFileRequested(calcFilePath_);
            }
            // mpif 来自 context 的 connectionInputs（上游结果连线到下游设置时，
            // 平台 changed_cells 给的是 setup 而不是 mpif），所以这里不依赖
            // changed_cells 标记：本次 refresh 解析到了 mpif 文件就触发导入。
            for (const QString& mpifPath : mpifPaths_) {
                MPCOMM_SEND_LOG(this, QStringLiteral("refresh: import mpif %1")
                    .arg(mpifPath));
                emit MechCommon::mMechGlobalSignals::getInstance()
                    ->mpcommImportMpifRequested(mpifPath);
            }
            // 通用耦合文件：仅更新 pending 属性供「多体载荷导入」面板读取，
            // 不自动触发导入。
            if (!couplingPaths_.isEmpty()) {
                MPCOMM_SEND_LOG(this, QStringLiteral("refresh: coupling files %1 个")
                    .arg(couplingPaths_.size()));
                qApp->setProperty("mpcomm_pending_coupling", couplingPaths_);
            }

            MPCOMM_SEND_LOG(this, QStringLiteral("REFRESH_INPUTS received"));
            return mpcomm::simple::Reply::ok(
                "input context applied");
        }

        if (command.name == "EXECUTE_ACTION") {
            std::string operationValue;
            std::string contextRevisionValue;
            std::string cellValue;

            command.data.get_string("operation", operationValue);
            command.data.get_string("contextRevision",
                contextRevisionValue);
            command.data.get_string("cell", cellValue);

            const QString operation =
                fromUtf8(operationValue).trimmed().toLower();
            const QString cell =
                fromUtf8(cellValue).trimmed().toLower();
            const QString requestedRevision =
                fromUtf8(contextRevisionValue);

            // 当前只支持求解动作。
            if (operation != QStringLiteral("solve")) {
                return mpcomm::simple::Reply::reject(
                    "UNSUPPORTED_ACTION",
                    "only solve action is supported");
            }
            if (!cell.isEmpty()
                && cell != QStringLiteral("solution")) {
                return mpcomm::simple::Reply::reject(
                    "UNSUPPORTED_ACTION",
                    "solve action requires solution cell");
            }
            if (!hasSession_) {
                return mpcomm::simple::Reply::reject(
                    "NO_ACTIVE_SESSION",
                    "no active node session");
            }
            // 平台携带 contextRevision 时校验一致性（过期上下文拒绝）。
            if (!requestedRevision.isEmpty()
                && !contextRevision_.isEmpty()
                && requestedRevision != contextRevision_) {
                return mpcomm::simple::Reply::reject(
                    "STALE_CONTEXT",
                    "contextRevision mismatch");
            }

            const bool queued =
                QMetaObject::invokeMethod(
                    this,
                    []() {
                        emit MechCommon::mMechGlobalSignals
                            ::getInstance()
                            ->mpcommStartSolveRequested();
                    },
                    Qt::QueuedConnection);

            if (!queued) {
                return mpcomm::simple::Reply::reject(
                    "QUEUE_FAILED",
                    "cannot queue EXECUTE_ACTION");
            }

            return mpcomm::simple::Reply::pending(
                "solve accepted");
        }

        return mpcomm::simple::Reply::reject(
            "UNSUPPORTED_COMMAND",
            "unsupported command: " + command.name);
    }

    mpcomm::simple::Reply handleMpcommCommand(
        const mpcomm::simple::Command& command,
        void* userData)
    {
        auto* bridge =
            static_cast<MpcommBridge*>(userData);

        if (!bridge) {
            return mpcomm::simple::Reply::reject(
                "INVALID_BRIDGE",
                "MpcommBridge is null");
        }

        return bridge->handleCommand(command);
    }

} // namespace MxMech
