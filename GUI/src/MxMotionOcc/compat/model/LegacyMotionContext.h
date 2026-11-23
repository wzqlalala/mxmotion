#pragma once
#include "NativeMotionDocument.h"
#include "mViewportModel.h"
#include "mGeoEngineData.h"
#include "mUnitSystemManager.h"
#include <QPointer>
#include <QTimer>
#include <memory>

namespace MotionOcc {
// Owns the original GUI's data/services. The document remains the exact input
// plus edited fields: merely opening a float-based legacy widget must never
// round doubles, inject defaults, or discard later engine extensions.
class LegacyMotionContext final : public QObject {
    Q_OBJECT
public:
    LegacyMotionContext(QWidget* owner,OccViewport*,const CadDocument&,
                        const std::map<int,FemMesh>&,const NativeMotionDocument&);
    ~LegacyMotionContext() override;
    MechModelData::mModelData* model() const { return model_.get(); }
    MechPreData::mViewportModel* viewportModel() const { return viewport_.get(); }
    LegacyPickContext* picks() const { return picks_.get(); }
    void watch(QWidget* panel);
    void endEdit();
    void refreshMeshes();
    void adoptExternalDocument(const NativeMotionDocument& document);
    void setExtensionFields(const QString& collection,int id,const QJsonObject& fields);
    const NativeMotionDocument& document() const { return document_; }
    std::function<void()> unedit;
    static QJsonValue applyFieldChanges(const QJsonValue& before,const QJsonValue& after,const QJsonValue& original);
public slots:
    void scheduleFlush();
    void flush();
signals:
    void documentChanged(const MotionOcc::NativeMotionDocument& document);
    void message(const QString& text,bool error);
private:
    QJsonObject inputForOriginalGui() const;
    void bindTopologyEdits(const QString& collection,const QJsonObject& before,
                           const QJsonObject& after,QJsonObject& merged);
    const CadDocument& cad_;
    const std::map<int,FemMesh>& meshes_;
    NativeMotionDocument document_;
    std::unique_ptr<mMeshData> mesh_;
    std::unique_ptr<GeoNeutralData::GeoEngineData> geometry_;
    std::unique_ptr<CommonData::mUnitSystemManager> units_;
    std::unique_ptr<LegacyPickContext> picks_;
    std::unique_ptr<mxr::CCSMRendWidget> render_;
    std::unique_ptr<MechModelData::mModelData> model_;
    std::unique_ptr<MechPreData::mViewportModel> viewport_;
    QJsonObject baseline_;
    QPointer<QWidget> panel_;
    QTimer flushTimer_;
    QSet<int> flexAttachmentCommits_,confirmedFlexAttachments_;
    std::map<int,QVector<int>> nodeIdSnapshot_;
    bool meshSnapshotInitialized_=false,meshSnapshotDirty_=false;
    bool flushing_=false,ending_=false;
};
}
