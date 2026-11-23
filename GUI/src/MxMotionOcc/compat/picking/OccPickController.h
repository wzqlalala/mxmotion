#pragma once

#include "mPickWidgetViewBase.h"
#include "mPickWidgetDataModel.h"
#include <QPointer>
#include <functional>
#include <memory>
#include <set>

namespace MotionOcc { class LegacyPickContext; }
namespace mxr { class CCSMRendWidget; }

namespace MechMainWindow {
using OccPickMap = std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>;
class IPickPersistenceStrategy;

// The original service interface is implemented by LegacyPickContext layers;
// it is still injectable through mGeneralPickUp and controller constructors.
class mPickWidgetHighlightServiceBase : public QObject {
public:
    explicit mPickWidgetHighlightServiceBase(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~mPickWidgetHighlightServiceBase() = default;
    void setContext(MotionOcc::LegacyPickContext* context);
    void setRenderWidget(mxr::CCSMRendWidget* widget);
    void setColorKey(const QString& key) { m_key = key; }
    virtual void setColor(const QColor& color) { m_color = color; }
    virtual void highlightSet(const OccPickMap& data, bool on);
    virtual void highlightPicked(const OccPickMap& data, bool on);
    virtual void clearOwnTempHighlight();
    virtual void clearAllHighlight();
    void setHighlightSuppressed(bool value) { m_suppressed = value; if (value) clearAllHighlight(); }
    bool isHighlightSuppressed() const { return m_suppressed; }
    void flushHighlightNow();
protected:
    QPointer<MotionOcc::LegacyPickContext> m_context;
    QString m_key;
    QColor m_color{232,130,28};
    bool m_suppressed = false;
};
class mPickWidgetHighlightService : public mPickWidgetHighlightServiceBase {
public:
    using mPickWidgetHighlightServiceBase::mPickWidgetHighlightServiceBase;
};

class mPickWidgetController : public QObject {
    Q_OBJECT
public:
    explicit mPickWidgetController(std::shared_ptr<mPickWidgetHighlightServiceBase> service,
                                   mPickWidgetViewBase* view, QWidget* parent = nullptr);
    ~mPickWidgetController() override;
    mPickWidgetViewBase* widget() const { return m_view; }
    void setPickWidgetVisible(bool visible);
    bool isPickWidgetVisible() const { return m_visible; }
    void setDefultTip(const QString& text, const QString& top = {});
    void setSelectSetList(const QHash<QString, QString>& values = {});
    void setSelectSetBtnVisible(bool value);
    void displayData(const PickWidgetData& data, bool highlight = false);
    void replacePickedDataAndClearSets(const OccPickMap& data);
    const PickWidgetData& getAllData() const { return m_model.getAllData(); }
    void setPickActive(bool active);
    bool isPickActive() const { return m_active; }
    void setBindingGuard(std::function<bool()> guard) { m_guard = std::move(guard); }
    void detachActive();
    bool eventFilter(QObject*, QEvent*) override;
    void setPickMode(mPickMode mode, bool clearWhenSwitch = false);
    mPickMode pickMode() const { return m_view ? m_view->pickMode() : PickMore; }
    void setKeepHighlightOnDeactivate(bool keep) { m_keepHighlight = keep; }
    bool keepHighlightOnDeactivate() const { return m_keepHighlight; }
    void setCloudChartMode(bool enable);
    bool isCloudChartMode() const { return m_cloud; }
    void setAllowedTypes(std::function<QSet<MxFunctions::PickDataType>()> getTypes);
    QSet<MxFunctions::PickDataType> allowedTypes() const;
    void setPickFilter(QSet<int> filter) { m_elementFilter = std::move(filter); }
    void setEdgeLoadPickFilter(bool enable) { m_edgeLoadFilter = enable; }
    void setAllPickTypesDisabled(bool disabled);
    void refreshAll();
    void clearAllHighlight();
    void clearAllTempHighlight();
    void refreshHighlight();
    void setSetResolver(std::function<OccPickMap(const QString&)> resolver) { m_setResolver = std::move(resolver); }
    void setPersistenceStrategy(std::shared_ptr<IPickPersistenceStrategy> strategy) { m_strategy = std::move(strategy); }
    static void commitActivePickWidget();
    static bool hasActivePickController() { return !s_active.isNull(); }
    static mPickWidgetController* activeController() { return s_active; }
    static void setActivePickType(MxFunctions::PickDataType type);
    static QSet<MxFunctions::PickDataType> activeAllowedTypes();
    void setPickType();
    void setContext(MotionOcc::LegacyPickContext* context);
    MotionOcc::LegacyPickContext* context() const;
    void setAllowedParts(std::set<int> parts) { m_allowedParts = std::move(parts); if (m_active) setPickType(); }
    const std::set<int>& allowedParts() const { return m_allowedParts; }
    void receiveViewportSelection(MxFunctions::PickDataType type, const std::vector<RenderPickResultData>& picks);
signals:
    void pickActiveChanged(bool active);
    void dataChanged();
    void calcMainNodeSig();
protected:
    virtual void commit();
private:
    void refreshUI();
    void changed();
    void enforceSinglePickLimit();
    bool bindingAlive() const { return !m_guard || m_guard(); }
    OccPickMap resolvedData() const;
    QPointer<mPickWidgetViewBase> m_view;
    mPickWidgetDataModel m_model;
    QPointer<MotionOcc::LegacyPickContext> m_context;
    std::shared_ptr<mPickWidgetHighlightServiceBase> m_service;
    std::shared_ptr<IPickPersistenceStrategy> m_strategy;
    std::function<OccPickMap(const QString&)> m_setResolver;
    std::function<QSet<MxFunctions::PickDataType>()> m_types;
    std::function<bool()> m_guard;
    std::set<int> m_allowedParts;
    QSet<int> m_elementFilter;
    QString m_layer;
    bool m_active = false, m_visible = true, m_dirty = false;
    bool m_contextExplicit = false;
    bool m_keepHighlight = true, m_cloud = false, m_disabled = false, m_edgeLoadFilter = false;
    static QPointer<mPickWidgetController> s_active;
};

class mSetPickWidgetConntroller : public mPickWidgetController {
public:
    using mPickWidgetController::mPickWidgetController;
protected:
    void commit() override {}
};
} // namespace MechMainWindow
