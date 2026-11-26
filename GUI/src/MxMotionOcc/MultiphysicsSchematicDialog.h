#pragma once

#include "NativeMotionDiagnostics.h"
#include "NativeMotionDocument.h"
#include "RigidMotion.h"

#include <QDialog>
#include <functional>

namespace MotionOcc {

class MultiphysicsSchematicDialog final : public QDialog {
    Q_OBJECT
public:
    struct Callbacks {
        std::function<bool(NativeMotionDocument&&)> commitDocument;
        std::function<void(const QString& collection, int id)> openEntityEditor;
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

protected:
    void showEvent(QShowEvent* event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace MotionOcc
