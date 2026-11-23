#include "MotionWindow.h"
#include "OccViewport.h"
#include "OriginalSelection.h"
#include "NativeMotionSymbols.h"
#include "compat/picking/LegacyPickContext.h"
#include "compat/picking/OccPickController.h"
#include "mWorkingDirPanel.h"

#include <DockManager.h>
#include <DockWidget.h>
#include <DockAreaWidget.h>
#include <QAction>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QSignalBlocker>
#include <QToolButton>

namespace MotionOcc {
namespace {
QString settingsPath()
{
    return QCoreApplication::applicationDirPath()+QStringLiteral("/../Setting/Settings.ini");
}

void rememberFile(const QString& file)
{
    QSettings settings(settingsPath(),QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("recent"));
    for(const auto& key:settings.childKeys())if(settings.value(key).toString()==file)settings.remove(key);
    settings.setValue(QString::number(QDateTime::currentMSecsSinceEpoch()),QFileInfo(file).absoluteFilePath());
}

QVector<PickResult> currentPicks(OccViewport* viewport)
{
    if(auto* controller=MechMainWindow::mPickWidgetController::activeController())
        if(auto* context=controller->context())return context->resolve(controller->getAllData().pickedData);
    return viewport->pickSelection();
}

bool samePick(const PickResult& a,const PickResult& b)
{
    return a.partId==b.partId&&a.kind==b.kind&&a.index==b.index;
}
}

bool MotionWindow::dispatchOriginalWorkspaceAction(const QString& action,const QVariant& argument)
{
    const std::map<QString,Command> direct{
        {"slot_importGeoFile",Command::Import},{"slot_exportGeoFile",Command::Export},
        {"slot_importMotionJsonFile",Command::ImportMotionJson},{"slot_exportMotionJsonFile",Command::ExportMotionJson},
        {"slot_reverseMaskModel",Command::InvertVisibility},{"slot_showAllModel",Command::ShowAll}};
    const auto route=direct.find(action);
    if(route!=direct.end()){command(route->second)->trigger();return true;}
    if(action==QLatin1String("slot_maskModel") || action==QLatin1String("slot_maskOtherModel")) {
        const auto picks=currentPicks(m_viewport);
        std::set<int> parts;
        for(const auto& pick:picks) {
            // The original operation is typed: hiding one face/edge/element
            // must never accidentally hide its whole owning CAD component.
            // Only route the part operation until topology masks are supplied.
            if(pick.kind!=PickKind::Part)return false;
            if(m_document.part(pick.partId))parts.insert(pick.partId);
        }
        if(parts.empty() && picks.empty() && m_document.part(m_partId))parts.insert(m_partId);
        if(parts.empty())return true;
        if(action==QLatin1String("slot_maskModel"))m_hidden.insert(parts.begin(),parts.end());
        else {
            m_hidden.clear();
            for(const auto& part:m_document.parts())if(!parts.count(part.id))m_hidden.insert(part.id);
        }
        syncViewport();updateTree();updateParameters();updateActions();
        return true;
    }
    if(action==QLatin1String("createGeometry")) {
        const std::map<QString,Command> primitives{{"CUBE",Command::Box},{"CYLINDER",Command::Cylinder},{"SPHERE",Command::Sphere}};
        auto it=primitives.find(argument.toString());
        if(it!=primitives.end()){command(it->second)->trigger();return true;}
        return false;
    }
    if(action==QLatin1String("slot_openRecentFile")) {
        const QString file=argument.toString();
        if(!QFileInfo::exists(file)){setNotice(tr("文件不存在：%1").arg(file),true);return true;}
        bool opened=false;
        if(file.endsWith(QLatin1String(".mxmotion"),Qt::CaseInsensitive))opened=openRigidProject(file);
        else if(file.endsWith(QLatin1String(".json"),Qt::CaseInsensitive))opened=importMotionJson(file);
        else opened=importCad(file); // Original recent-geometry entries import into the current model.
        if(opened)rememberFile(file);
        return true;
    }
    if(action==QLatin1String("createWorkingDir")) {
        // Reuse the uploaded fold/editor/folder-button panel and its real
        // Settings.ini handler, including AutoFlex's external export path.
        auto* panel=new MechMainWindow::mWorkingDirPanel;
        panel->setObjectName(QStringLiteral("originalWorkingDirPanel"));
        showEditor(panel);
        return true;
    }
    if(action==QLatin1String("applyPreset1")||action==QLatin1String("applyPreset2")) {
        resetLayout(true);
        if(action==QLatin1String("applyPreset2")&&m_treeArea&&m_parameterDock) {
            // Original preset 2 places Parameters below the model/view tree;
            // preset 1 keeps them side by side to the left of the viewport.
            const auto area=m_docks->addDockWidget(ads::BottomDockWidgetArea,m_parameterDock,m_treeArea);
            if(area){auto sizes=m_docks->splitterSizes(area);if(sizes.size()>=2){sizes[0]=1;sizes[1]=1;m_docks->setSplitterSizes(area,sizes);}}
            m_parameterDock->toggleView(true);
        }
        return true;
    }
    if(action==QLatin1String("setModeMenuVisible")||action==QLatin1String("setTreeMenuLinkMode")) {
        // The original ribbon persists the mode-specific checklists and applies
        // the active pre-processing tabs. Tree selection reads this same mode.
        if(action==QLatin1String("setTreeMenuLinkMode"))setProperty("treeMenuLinkMode",argument.toInt());
        return true;
    }
    if(action==QLatin1String("slot_changeViewList")) {
        for(auto* button:findChildren<QToolButton*>())if(button->property("mxOriginalAction").toString()==QLatin1String("slot_autoRotation")) {
            QSignalBlocker blocked(button);button->setChecked(false);
        }
        m_viewport->stepViewHistory(argument.toBool());return true;
    }
    if(action==QLatin1String("slot_angleRotation")) {
        const auto args=argument.toList();if(args.size()!=2)return true;
        const QString axis=args[0].toString();
        m_viewport->rotateView(axis,(axis.contains(QLatin1Char('-'))?-1:1)*args[1].toDouble());return true;
    }
    if(action==QLatin1String("slot_directionPan")) {
        const auto args=argument.toList();if(args.size()==2)m_viewport->panView(args[0].toString(),args[1].toDouble());return true;
    }
    if(action==QLatin1String("slot_valueZoom")) {
        const auto args=argument.toList();if(args.size()==2)m_viewport->zoomView(args[0].toBool(),args[1].toDouble());return true;
    }
    if(action==QLatin1String("slot_autoRotation")) {
        const auto args=argument.toList();if(args.size()==2)m_viewport->setAutoRotation(args[0].toString(),args[1].toBool());return true;
    }
    if(action==QLatin1String("slot_showNodeEleLabel")||action==QLatin1String("slot_hideNodeEleLabel")||action==QLatin1String("slot_hideAllNodeEleLabel")) {
        auto labels=property("originalNumberLabels").value<QVector<PickResult>>();
        const auto picks=currentPicks(m_viewport);
        if(action==QLatin1String("slot_hideAllNodeEleLabel"))labels.clear();
        else if(action==QLatin1String("slot_hideNodeEleLabel"))labels.erase(std::remove_if(labels.begin(),labels.end(),[&](const auto& p){return std::any_of(picks.begin(),picks.end(),[&](const auto& selected){return samePick(p,selected);});}),labels.end());
        else for(const auto& pick:picks)if((pick.kind==PickKind::Node||pick.kind==PickKind::Element)&&std::none_of(labels.begin(),labels.end(),[&](const auto& p){return samePick(p,pick);}))labels.append(pick);
        setProperty("originalNumberLabels",QVariant::fromValue(labels));
        QVector<MotionSymbol> symbols;
        for(const auto& pick:labels) {
            MotionSymbol symbol;symbol.origin=pick.position;symbol.partId=pick.partId;
            symbol.drawOrigin=false;symbol.color=QColor(35,44,55);symbol.textColor=symbol.color;
            symbol.texts.push_back({QString::number(pick.kind==PickKind::Node?pick.nodeId:pick.index),pick.position});symbols.append(symbol);
        }
        m_viewport->setAuxiliaryMotionSymbols(QStringLiteral("original-number-labels"),symbols);
        return true;
    }
    if(action==QLatin1String("slot_showFreeNode")||action==QLatin1String("slot_hideFreeNode")
        ||action==QLatin1String("slot_showAllFreeNode")||action==QLatin1String("slot_hideAllFreeNode")) {
        auto shown=property("originalFreeNodeMarkers").value<QVector<PickResult>>();
        auto selected=currentPicks(m_viewport);QVector<PickResult> free;
        auto* context=LegacyPickContext::forObject(this);
        for(const auto& [partId,mesh]:m_meshes) {
            if(m_hidden.count(partId))continue;
            std::vector<bool> used(mesh.nodes.size(),false);
            for(const auto& cell:mesh.tetrahedra)for(auto node:cell)if(node<used.size())used[node]=true;
            for(const auto& face:mesh.surface)for(auto node:face)if(node<used.size())used[node]=true;
            const auto ids=context?context->nodeIds(partId):QVector<int>{};
            for(int i=0;i<int(used.size());++i)if(!used[std::size_t(i)]) {
                PickResult pick;pick.kind=PickKind::Node;pick.partId=partId;pick.index=i;pick.nodeId=ids.value(i,i+1);pick.position=mesh.nodes[std::size_t(i)];free.append(pick);
            }
        }
        if(action==QLatin1String("slot_hideAllFreeNode"))shown.clear();
        else if(action==QLatin1String("slot_hideFreeNode"))shown.erase(std::remove_if(shown.begin(),shown.end(),[&](const auto& p){return std::any_of(selected.begin(),selected.end(),[&](const auto& pick){return samePick(p,pick);});}),shown.end());
        else for(const auto& pick:free)if((action==QLatin1String("slot_showAllFreeNode")||std::any_of(selected.begin(),selected.end(),[&](const auto& p){return samePick(p,pick);}))&&std::none_of(shown.begin(),shown.end(),[&](const auto& p){return samePick(p,pick);}))shown.append(pick);
        setProperty("originalFreeNodeMarkers",QVariant::fromValue(shown));
        QVector<QVector3D> points;for(const auto& pick:shown)points.append(QVector3D(float(pick.position[0]),float(pick.position[1]),float(pick.position[2])));
        m_viewport->setAuxiliaryPoints(QStringLiteral("original-free-nodes"),points,QColor(229,113,35));
        return true;
    }
    QString message;
    if(dispatchOriginalSelection(*this,*m_viewport,m_document,m_meshes,m_hidden,m_partId,action,argument,message)) {
        if(!message.isEmpty())setNotice(message);
        return true;
    }
    return false;
}
}
