#include "CadDocument.h"
#include "FemMesh.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mViewportModelManager.h"
#include "model/LegacyMaterialCodec.h"
#include "model/LegacyMotionJsonAdapter.h"
#include "model/LegacyFlexAttachmentCodec.h"
#include "solver/LegacyMotionSolverThread.h"
#include "mMotionFlexBridge.h"
#include "mMotionFlexMxExport.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTextStream>
#include <cmath>
#include <stdexcept>

using namespace MotionOcc;
using namespace MxFunctions;
using namespace MechMotionData;
namespace {
void require(bool condition,const char* message) { if(!condition)throw std::runtime_error(message); }
void events() { for(int i=0;i<3;++i)QCoreApplication::processEvents(); }
FemMesh tetra(double offset) {
    FemMesh mesh;mesh.nodes={{{offset,0,0}},{{offset+1,0,0}},{{offset,1,0}},{{offset,0,1}}};
    mesh.tetrahedra={{{0,1,2,3}}};mesh.surface={{{0,2,1}},{{0,1,3}},{{0,3,2}},{{1,2,3}}};return mesh;
}
}
int runMotionOccOriginalBackendSmoke() {
    try {
        MechModelData::mModelData model;
        QJsonObject material{{"id",7},{"name","steel"},{"youngModulus",210000.0},{"poissonRatio",0.3},
            {"density",7.85e-6},{"vendorExtension",QJsonObject{{"preserve",true}}}};
        LegacyMotionJsonAdapter::load(QJsonObject{{"materials",QJsonArray{material}}},model);
        auto& materials=model.getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        auto steel=materials.getMaterialData(7);require(bool(steel),"material load failed");
        steel->addProperty(MAT_Isotropy,MatProp_YoungsModulus,205000.125);
        const auto saved=LegacyMotionJsonAdapter::save(model).value("materials").toArray().first().toObject();
        require(saved.value("youngModulus").toDouble()==205000.125,"material edit lost precision");
        require(saved.value("density").toDouble()==7.85e-6,"native density changed units");
        require(saved.value("vendorExtension")==material.value("vendorExtension"),"material extension lost");
        const int copy=model.copyPhysicalField(Physical_Material,7);
        require(copy>7 && materials.getMaterialData(copy)->getAllProps()==steel->getAllProps(),"material copy failed");
        model.deleteByPhysicalType(Physical_Material,copy);
        require(!materials.getMaterialData(copy),"material delete failed");
        { MechModelData::mModelData zeroModel;auto zeroMaterial=material;zeroMaterial.insert("id",0);
          LegacyMotionJsonAdapter::load(QJsonObject{{"materials",QJsonArray{zeroMaterial}}},zeroModel);
          require(bool(zeroModel.getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(0)),"valid zero-based imported material ID was rejected"); }

        CadDocument cad;QString error;int part1=-1,part2=-1;
        require(cad.addPrimitive(Primitive::Box,1,1,1,part1,error),"first OCC part failed");
        require(cad.addPrimitive(Primitive::Box,1,1,1,part2,error),"second OCC part failed");
        std::map<int,FemMesh> meshes{{part1,tetra(0)},{part2,tetra(10)}};
        mMeshData mesh(cad,meshes);model.bindBackends(&mesh,nullptr,nullptr);
        auto* body=new mMotionBody(11,"multi-part flex");model.addPhysicalField(body);
        auto& relations=model.getMapRelationManager();
        relations.appendMapRelation({Physical_MotionBody,11},{Physical_Part,part1});
        relations.appendMapRelation({Physical_MotionBody,11},{Physical_Part,part2});
        auto* flex=new mMotionAutoFlex(12,"flex");flex->setMaterialId(7);flex->setAttachPoints({1,2,3,5,6,7});
        model.addPhysicalField(flex);
        relations.appendMapRelation({Physical_MotionAutoFlex,12},{Physical_MotionBody,11});
        std::vector<double> coords;std::vector<int> tets;std::map<int,int> ids;
        require(mMotionFlexBridge::collectBodyTetraMesh(&model,{part2},1,coords,tets,ids,{}),"second part mesh collection failed");
        require(coords.size()==12 && tets.size()==4 && coords[0]==10 && ids.count(5) && !ids.count(1),"part filtering or global IDs wrong");
        auto inputs=mMotionFlexBridge::collectFlexInputs(nullptr,&model,{});
        require(inputs.size()==1 && inputs.front().valid,"legacy flex input not valid");
        const auto& input=inputs.front();
        require(input.nodeCoords.size()==24 && input.tetIds.size()==8 && input.attachGroups.size()==2,"multi-part interfaces merged or omitted");
        require(std::abs(input.rho-7.85e-9)<1e-20 && input.E==205000.125,"MX unit conversion wrong");
        QTemporaryDir output;require(output.isValid(),"temporary output unavailable");
        const auto path=output.filePath("original-flex.mx");
        require(mMotionFlexMxExport::exportMxForMnf(input,path,&error),"original MX exporter failed");
        QFile mx(path);require(mx.open(QIODevice::ReadOnly)&&mx.size()>500,"MX file missing");
        flex->setAttachPoints({999});
        inputs=mMotionFlexBridge::collectFlexInputs(nullptr,&model,{});
        require(inputs.size()==1&&!inputs.front().valid&&!inputs.front().error.isEmpty(),"stale node ID silently snapped");

        const QVector<int> parts{part1,part2};
        const auto snapshot=legacyFlexMeshSnapshot(mesh,parts);
        require(snapshot.value("nodeIds").toArray()==QJsonArray({1,2,3,4,5,6,7,8})
            &&snapshot.value("partRanges").toArray().size()==2,"native flex snapshot lacks real global mapping");
        auto previousMesh=snapshot;previousMesh.insert("nodeIds",QJsonArray{701,702,703,704,705,706,707,708});
        QJsonObject previousFlex{{"id",12},{"targetBodyId",11},{"mesh",previousMesh},
            {"attachPoints",QJsonArray{701,702,703,705,706,707}}};
        const auto guiIds=legacyFlexGuiAttachmentIds(previousFlex,mesh,parts);
        require(guiIds==QJsonArray({1,2,3,5,6,7}),"restored global IDs did not map through real mesh identity");
        auto grouped=previousFlex;grouped.remove("attachPoints");
        grouped.insert("attachGroups",QJsonArray{QJsonArray{0,1,2},QJsonArray{4,5,6}});
        require(legacyFlexGuiAttachmentIds(grouped,mesh,parts)==guiIds,"native explicit interface groups disappeared from original picker");
        NativeMotionDocument previous;previous.root=QJsonObject{{"motionData",QJsonObject{{"autoFlexBodies",QJsonArray{previousFlex}}}}};
        previous.bodyPartBindings.insert(11,parts);auto current=previous;
        QJsonObject oldTyped{{"id",12},{"targetBodyId",11},{"attachPoints",guiIds}};
        auto newTyped=oldTyped;newTyped.insert("attachPoints",QJsonArray{1,2,3});
        auto edited=previousFlex;edited.insert("attachPoints",newTyped.value("attachPoints"));edited.insert("attachmentMeshChanged",true);
        QJsonObject editedData{{"autoFlexBodies",QJsonArray{edited}}};
        const QJsonObject before{{"autoFlexBodies",QJsonArray{oldTyped}}},after{{"autoFlexBodies",QJsonArray{newTyped}}};
        synchronizeLegacyFlexAttachments(previous,current,editedData,before,after,mesh,{12},{12});
        auto repaired=editedData.value("autoFlexBodies").toArray().first().toObject();
        require(!repaired.value("attachmentMeshChanged").toBool()
            &&repaired.value("mesh").toObject().value("nodeIds")==snapshot.value("nodeIds")
            &&repaired.value("attachPoints")==newTyped.value("attachPoints"),"explicit reselection failed to establish native node IDs");
        auto materialBefore=oldTyped,materialAfter=oldTyped;
        materialBefore.insert("material",material);auto changedMaterial=material;changedMaterial.insert("youngModulus",200000);
        materialAfter.insert("material",changedMaterial);
        auto partialMaterial=previousFlex;partialMaterial.insert("material",QJsonObject{{"youngModulus",200000}});
        QJsonObject materialData{{"autoFlexBodies",QJsonArray{partialMaterial}}};
        synchronizeLegacyFlexAttachments(previous,current,materialData,
            QJsonObject{{"autoFlexBodies",QJsonArray{materialBefore}}},QJsonObject{{"autoFlexBodies",QJsonArray{materialAfter}}},mesh,{},{});
        const auto completeMaterial=materialData.value("autoFlexBodies").toArray().first().toObject().value("material").toObject();
        require(completeMaterial.value("youngModulus").toDouble()==200000&&completeMaterial.value("poissonRatio").toDouble()==.3
            &&completeMaterial.value("density").toDouble()==7.85e-6,"editing one material field left a partial inline material");
        current.bodyPartBindings.insert(11,{part2});editedData=previous.root.value("motionData").toObject();
        synchronizeLegacyFlexAttachments(previous,current,editedData,before,before,mesh,{},{});
        auto rebound=editedData.value("autoFlexBodies").toArray().first().toObject();
        require(rebound.value("attachmentMeshChanged").toBool()
            &&rebound.value("mesh").toObject().value("nodes").toArray().size()==4,"body binding edit silently reused old attachment mesh");

        std::map<int,FemMesh> delayedMeshes{{part2,tetra(10)}};mMeshData stable(cad,delayedMeshes);
        const auto preserved=stable.nodeIds().at(part2);delayedMeshes.emplace(part1,tetra(0));stable.rebuildNodeIndex();
        require(stable.nodeIds().at(part2)==preserved,"adding an earlier CAD part renumbered existing nodes");
        delayedMeshes[part2].nodes[0][0]+=.25;stable.rebuildNodeIndex();
        require(stable.nodeIds().at(part2)!=preserved&&!stable._nodedata.nodeidmap.count(preserved.front()),"changed mesh reused stale global IDs");

        MechPreData::mViewportModel viewport(&model,nullptr);
        MechPreData::mViewportModelManager::getInstance()->setCurrentViewportModel(&viewport);
        int starts=0,stops=0,flushes=0,outputs=0,finished=0;bool success=true;
        viewport.validateMotion=[](int) { return QStringList{}; };
        viewport.solveMotion=[&](int id) { require(id==42,"wrong scenario ID");++starts; };
        viewport.stopMotion=[&] { ++stops; };
        viewport.flushMotion=[&] { ++flushes; };
        viewport.syncMotionResults=[&](int) { ++outputs; };
        {
            MechMainWindow::mMotionSolverThread solver(42,&viewport);
            QObject::connect(&solver,&MechMainWindow::mMotionSolverThread::solveFinished,[&](int,bool ok) { ++finished;success=ok; });
            solver.start();solver.requestStopSolve();events();
            require(starts==0&&stops==0&&finished==1&&!success&&!solver.isRunning(),"cancel before asynchronous start raced");
            solver.start();events();require(starts==1&&flushes==1&&solver.isRunning(),"solver did not dispatch native backend");
            emit viewport.motionSolveProgress(41,77);require(solver.m_currentProgress.load()==0,"foreign scenario progress leaked");
            emit viewport.motionSolveProgress(42,51);require(solver.m_currentProgress.load()==51,"native progress was not forwarded");
            emit viewport.motionSolveFinished(42,true);
            require(finished==2&&success&&outputs==1&&!solver.isRunning()&&solver.m_currentProgress.load()==100,"native result completion lost");
        }
        {
            viewport.validateMotion=[](int) { return QStringList{QStringLiteral("invalid model")}; };
            MechMainWindow::mMotionSolverThread solver(42,&viewport);solver.start();events();
            require(starts==1&&!solver.isRunning(),"invalid model still entered solver");
        }
        MechPreData::mViewportModelManager::getInstance()->setCurrentViewportModel(nullptr);
        QTextStream(stdout)<<"Original backend compatibility PASS: material round-trip, real OCC parts / filtered tetra IDs, MX export, native attachment mapping / body rebinding / stable IDs, solver cancellation/progress/completion.\n";
        return 0;
    } catch(const std::exception& exception) { QTextStream(stderr)<<exception.what()<<'\n';return 1; }
}
#ifndef MX_OCC_DESKTOP_SMOKE
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);return runMotionOccOriginalBackendSmoke();
}
#endif
