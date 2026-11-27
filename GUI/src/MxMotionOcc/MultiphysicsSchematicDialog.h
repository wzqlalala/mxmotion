#pragma once

#include "NativeMotionDiagnostics.h"
#include "NativeMotionDocument.h"
#include "RigidMotion.h"
#include "mFramelessDialog.h"

#include <functional>
#include <memory>

namespace MotionOcc {

class MultiphysicsSchematicDialog final : public MechMainWindow::mFramelessDialog {
    Q_OBJECT
public:
    enum class PickTarget {
        None,
        HydraulicNode,         // 拾取液压节点（油箱 / 压力源 / 容腔）
        HydraulicActuatorPort, // 拾取液压执行器油口（液压缸或液压马达的 A / B 口）
        PowerSupply,           // 拾取直流电源（DC+）
        Inverter               // 拾取伺服逆变器（UVW）
    };

    struct PickRequest {
        PickTarget target = PickTarget::None;
        QString promptTitle;    // 例如“端口连接-1 · 目标执行器油口”
        QString ownerCollection;
        int ownerId = -1;
        QString ownerPinKey;
        std::function<void(const QString& collection, int id, const QString& pinKey)> onPicked;
        std::function<void()> onCancelled;
    };

    struct Callbacks {
        std::function<bool(NativeMotionDocument&&)> commitDocument;
        std::function<void(const QString& collection, int id)> openEntityEditor;
        std::function<void(const QString& collection, int id)> onEntitySelected;
        std::function<void(int frameIndex)> onFrameScrubbed;
        std::function<MotionDiagnosticReport(const NativeMotionDocument&)> runDiagnostics;
        std::function<QJsonObject(const NativeMotionDocument&, const QString& collection, const QString& type)> createDefaultEntity;
    };

    MultiphysicsSchematicDialog(const NativeMotionDocument& initialDocument,
                                const RigidResult* solvedResult,
                                int initialFrameIndex,
                                Callbacks callbacks,
                                QWidget* parent = nullptr);
    ~MultiphysicsSchematicDialog() override;

    const NativeMotionDocument& currentDocument() const;
    bool exportPng(const QString& filePath) const;

    // 与主窗口模型树、参数面板和动画时间轴实时同步
    void syncDocument(const NativeMotionDocument& document, const RigidResult* solvedResult, int frameIndex);
    void syncSelection(const QString& collection, int id, bool centerInView = true);
    void setPlaybackFrame(int frameIndex);

    // 可视化端口/节点拾取引导模式
    void beginPortPick(PickRequest request);
    void cancelPortPick(bool notifyCancelled = true);
    bool isPortPicking() const;

    // 供交互点击与无头回归测试统一触发图元/端口选择
    QString selectedCollection() const;
    int selectedEntityId() const;
    bool activateEntityOrPin(const QString& collection, int id, const QString& pinKey = {});

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace MotionOcc
