#pragma once

#include "OccViewport.h"
#include <QJsonObject>
#include <QPointer>
#include <functional>

class QFormLayout;
class QGroupBox;
class QPushButton;

namespace MotionOcc {
class CadDocument;
class LegacyPickContext;

// One selection session per property editor. CAD coordinates stay in the
// original placed world frame, matching the original motion picker contract.
class NativePickSession final : public QObject {
public:
    using Changed = std::function<void(const QVector<PickResult>&)>;
    NativePickSession(OccViewport*, const CadDocument&, const QJsonObject&, QObject* parent);
    ~NativePickSession() override;
    void begin(const QString& key, QPushButton*, PickKind, bool multiple,
               const std::set<int>& allowedParts, const QVector<PickResult>& initial,
               Changed changed, std::function<void()> cancelled = {});
    void finish(bool cancel = false);
    QWidget* decorateVector(QFormLayout*, QGroupBox*, const QString& key, const QString& path);
    QVector<PickResult> saved(const QString& key) const;
    QJsonObject references() const { return m_references; }

private:
    void connectViewport(OccViewport* viewport);
    bool ownsSession() const;
    void record(const QString&, const QVector<PickResult>&);
    QPointer<OccViewport> m_viewport;
    QPointer<LegacyPickContext> m_legacyContext;
    const CadDocument& m_cad;
    QJsonObject m_references;
    QPointer<QPushButton> m_button;
    QString m_buttonText;
    QString m_key;
    QVector<PickResult> m_initial;
    Changed m_changed;
    std::function<void()> m_cancelled;
};
} // namespace MotionOcc
