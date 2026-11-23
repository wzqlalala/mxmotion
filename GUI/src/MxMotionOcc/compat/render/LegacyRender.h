#pragma once

#include "picking/LegacyPickContext.h"
#include "mMeshData.h"
#include "NativeMotionSymbols.h"
#include <QColor>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QVector3D>
#include <QVector4D>
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace mxr {
enum class SelectionInfoType { GeoPoint,GeoEdge,GeoFace,GeoSolid,GeoPart,MeshNode,MeshEdge,MeshFace,MeshElement,MeshPart };
enum class SelectionOperate { ADD,REMOVE,CLEAR };
enum class SymbolSelectionInfoType { Symbol_All };
enum class HighStyle { PartALLFeatures };
struct MeshSelectInfoData {
    int part_id=-1;
    struct { int node_id=-1; int element_id=-1; int face_id=-1; int edge_id=-1; } data_id;
};
struct AABB {
    QVector3D minEdge,maxEdge;
    bool populated=false;
    bool valid() const { return populated; }
    float getDiagonal() const { return (maxEdge-minEdge).length(); }
};
template<class T> T* refData(T* value) { return value; }

class mGeoSelectData {
public:
    std::map<SelectionInfoType,std::vector<int>> values;
    void UpdatePickData(SelectionOperate op,SelectionInfoType type,const std::vector<int>& ids) {
        auto& target=values[type];
        if(op==SelectionOperate::CLEAR) { target.clear(); return; }
        for(int id:ids) {
            auto it=std::find(target.begin(),target.end(),id);
            if(op==SelectionOperate::REMOVE) { if(it!=target.end()) target.erase(it); }
            else if(it==target.end()) target.push_back(id);
        }
    }
    std::vector<int> getSelectDataByType(SelectionInfoType type) const {
        auto it=values.find(type);return it==values.end() ? std::vector<int>{} : it->second;
    }
};
class MeshSelectMemoryData {
public:
    std::map<SelectionInfoType,std::vector<MeshSelectInfoData>> values;
    void UpdateSelectDataByType(SelectionOperate op,SelectionInfoType type,const std::vector<MeshSelectInfoData>& ids) {
        auto& target=values[type];
        if(op==SelectionOperate::CLEAR) target.clear();
        else if(op==SelectionOperate::ADD) target.insert(target.end(),ids.begin(),ids.end());
        else for(const auto& id:ids) target.erase(std::remove_if(target.begin(),target.end(),[&](const auto& x) {
            return x.part_id==id.part_id && x.data_id.node_id==id.data_id.node_id && x.data_id.element_id==id.data_id.element_id;
        }),target.end());
    }
    std::vector<MeshSelectInfoData> getSelectDataByType(SelectionInfoType type) const {
        auto it=values.find(type);return it==values.end() ? std::vector<MeshSelectInfoData>{} : it->second;
    }
};
class SymbolSelectData {
public:
    QSet<QString> values;
    std::function<void()> changed;
    void UpdatePickData(SelectionOperate operation,SymbolSelectionInfoType,const std::vector<QString>& keys) {
        if(operation==SelectionOperate::CLEAR)values.clear();
        else for(const auto& key:keys) {
            if(operation==SelectionOperate::ADD)values.insert(key);
            else values.remove(key);
        }
        if(changed)changed();
    }
};
class SelectionDataContainer : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    mGeoSelectData* getGeoSelectData() { return &geometry; }
    MeshSelectMemoryData* getMeshSelectData() { return &mesh; }
    SymbolSelectData* getSymbolSelectData() { return &symbols; }
    mGeoSelectData geometry;
    MeshSelectMemoryData mesh;
    SymbolSelectData symbols;
signals:
    void ReadyToReadContainer();
};

struct UniversalSymbolFont {
    QString key;
    QVector<QVector3D> _vexter;
    std::vector<unsigned int> _lineIndex,_faceIndex;
    QVector3D _lineColor{0,0,0},_faceColor{0,0,0},_textColor{0,0,0};
    float _opacity=1.f;
    bool _lineIsFixedPixel=false,_lineDepthState=false,_faceIsFixedPixel=false,_faceDepthState=false,_showState=true;
    QVector<QPair<QString,QVector3D>> _txt_pos;
};
class mSymbolController {
public:
    explicit mSymbolController(MotionOcc::OccViewport* viewport):viewport_(viewport) {}
    ~mSymbolController() { if(viewport_)viewport_->clearAuxiliaryMotionSymbols(QStringLiteral("original:%1").arg(quintptr(this))); }
    template<class T> std::shared_ptr<T> getOrCreateSymbolData(const QString& name) {
        static_assert(std::is_same_v<T,UniversalSymbolFont>);
        auto& value=symbols_[name];if(!value) { value=std::make_shared<T>();value->key=name; } return value;
    }
    bool isExistSymbol(const QString& name) const { return symbols_.contains(name); }
    void createSymbol(const std::shared_ptr<UniversalSymbolFont>& symbol) { if(symbol) symbols_[symbol->key]=symbol; }
    void deleteSymbol(const QStringList& names) { for(const auto& name:names) symbols_.remove(name); }
    void setSymbolShowState(const QStringList& names,bool show) { for(const auto& name:names) if(symbols_.contains(name)) symbols_[name]->_showState=show; }
    void setSymbolShowState(const std::vector<QString>& names,bool show) { for(const auto& name:names)if(symbols_.contains(name))symbols_[name]->_showState=show; }
    void setSymbolShowState(std::initializer_list<QString> names,bool show) { for(const auto& name:names)if(symbols_.contains(name))symbols_[name]->_showState=show; }
    void UpdateAllSymbol();
    void setSelectedSymbols(const QSet<QString>& selected) { selected_=selected;UpdateAllSymbol(); }
    void clear() { symbols_.clear();UpdateAllSymbol(); }
private:
    QPointer<MotionOcc::OccViewport> viewport_;
    QMap<QString,std::shared_ptr<UniversalSymbolFont>> symbols_;
    QSet<QString> selected_;
};
class LegacyHighLightRender {
public:
    LegacyHighLightRender(MotionOcc::LegacyPickContext* context,QString key):context_(context),key_(std::move(key)) {}
    ~LegacyHighLightRender();
    void setHighLightColor(int r,int g,int b) { color_=QColor(r,g,b);publish(); }
    // The OCC feedback pass is depth tested; cover/depth parameters are retained
    // as metadata so this interface cannot accidentally make hidden CAD visible.
    void setHighLightCover(bool cover,float depth) { cover_=cover;depth_=depth; }
    void updateHighLightRender(const mGeoSelectData* selection,HighStyle = HighStyle::PartALLFeatures);
    void updateHighLightRender(const MeshSelectMemoryData* selection);
    mGeoSelectData* getGeoSelectData() { return &geometry_; }
    void publish();
private:
    QPointer<MotionOcc::LegacyPickContext> context_;
    QString key_;
    QColor color_{255,128,0};
    bool cover_=true;
    float depth_=0;
    mGeoSelectData geometry_;
    MeshSelectMemoryData mesh_;
};
class LegacyHighlightController {
public:
    explicit LegacyHighlightController(MotionOcc::LegacyPickContext* context):context_(context) {}
    LegacyHighLightRender* getExtraPreGeoHighLightRender(const std::string& key) { return layer("geo:"+QString::fromStdString(key)); }
    LegacyHighLightRender* getExtraPreMeshHighLightRender(const std::string& key) { return layer("mesh:"+QString::fromStdString(key)); }
    void clear() { layers_.clear(); }
private:
    LegacyHighLightRender* layer(const QString& key) {
        auto& value=layers_[key];if(!value)value=std::make_unique<LegacyHighLightRender>(context_,key);return value.get();
    }
    QPointer<MotionOcc::LegacyPickContext> context_;
    std::map<QString,std::unique_ptr<LegacyHighLightRender>> layers_;
};
class LegacyCommonShapeRender {
public:
    LegacyCommonShapeRender(MotionOcc::LegacyPickContext* context,mSymbolController* symbols):context_(context),symbols_(symbols) {}
    void appendPoint(const QString& key,const QVector<QVector3D>& points,const QVector3D& color,float pointSize = 10.f);
    void deletePoint(const QString& key);
    void appendCylinder(const QString& key,const QVector3D& center,const QVector3D& halfAxis,float radius,const QVector3D& color);
    void appendCylinder(const QString& key,const QVector3D& center,const QVector3D& halfAxis,float radius,const QVector4D& color);
    void deleteCylinder(const QString& key);
private:
    QPointer<MotionOcc::LegacyPickContext> context_;
    mSymbolController* symbols_;
};
class LegacyGeoPartManager {
public:
    explicit LegacyGeoPartManager(const MotionOcc::CadDocument& cad):cad_(cad) {}
    bool containsPart(int id) const { return cad_.part(id)!=nullptr; }
private: const MotionOcc::CadDocument& cad_;
};
struct LegacyRenderModelData {
    mMeshData* _meshData=nullptr;
    LegacyGeoPartManager* geometry=nullptr;
    LegacyGeoPartManager* getGeoPartMgr() const { return geometry; }
};
struct CameraState {
    struct Vector { double x=0,y=0,z=0; };
    Vector eye,target,up;
    double ortho_zoom=1;
};
class LegacyCameraController {
public:
    using State=CameraState;
    explicit LegacyCameraController(MotionOcc::OccViewport* view):view_(view) {}
    State getCameraState() const;
private: QPointer<MotionOcc::OccViewport> view_;
};
class CCSMRendWidget : public QObject {
    Q_OBJECT
public:
    CCSMRendWidget(MotionOcc::LegacyPickContext*,mMeshData*,QObject* parent=nullptr);
    ~CCSMRendWidget() override;
    SelectionDataContainer* getSelectionContainer() { return &selection_; }
    LegacyHighlightController* getHighlightController() { return &highlight_; }
    LegacyCommonShapeRender* getCommonShapeRender() { return &common_; }
    mSymbolController* GetSymbolController() { return &symbols_; }
    LegacyCameraController* getCameraController() { return &camera_; }
    const LegacyRenderModelData& getModelData() const { return model_; }
    AABB getAABBOfView() const;
    void updatePaint();
    void beginPartPick();
    void setSelectedParts(const std::vector<int>& ids,bool notify);
    std::vector<int> selectedParts() const;
    MotionOcc::LegacyPickContext* context() const { return context_; }
    MotionOcc::LegacyPickContext* legacyPickContext() const { return context_; }
private:
    QPointer<MotionOcc::LegacyPickContext> context_;
    SelectionDataContainer selection_;
    LegacyHighlightController highlight_;
    mSymbolController symbols_;
    LegacyCommonShapeRender common_;
    LegacyGeoPartManager geoParts_;
    LegacyRenderModelData model_;
    LegacyCameraController camera_;
    QPointer<MechMainWindow::mPickWidgetController> partPicker_;
};
}
using mxr::MeshSelectInfoData;
using mxr::MeshSelectMemoryData;
