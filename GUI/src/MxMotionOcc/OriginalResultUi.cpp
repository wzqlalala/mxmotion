#include "MotionWindow.h"
#include "RigidPlayback.h"
#include "RigidPlot.h"
#include "RigidResultSeries.h"
#include "NativeMotionLoadExport.h"
#include "compat/model/LegacyMotionContext.h"
#include "mCurveWindow.h"
#include "mDockLayoutManager.h"
#include "mMotionAnimationPanel.h"
#include "mMotionCurvePanel.h"
#include "mMotionLoadExportPanel.h"
#include "mMotionLoadImportPanel.h"
#include "mPlotView.h"
#include "mViewportModel.h"

#include <DockWidget.h>
#include <DockAreaWidget.h>
#include <QCheckBox>
#include <QComboBox>
#include <QPointer>
#include <QScrollArea>
#include <QSignalBlocker>

namespace MotionOcc {
using namespace MechMainWindow;
namespace {
void openDock(ads::CDockWidget* dock) {
    if(!dock)return;
    dock->toggleView(true);
    if(auto* area=dock->dockAreaWidget())area->setCurrentDockWidget(dock);
}
}

void MotionWindow::ensureOriginalCurveHost()
{
    if (!m_curveDock || !m_projectActive) return;
    if (m_workspace) {
        if (!m_projectCurveWindow) m_projectCurveWindow = mCurveWindow::createForProject(this);
        mCurveWindow::setActiveProjectWindow(m_projectCurveWindow);
    }
    auto* window = m_projectCurveWindow ? m_projectCurveWindow : mCurveWindow::getInstance();
    if (m_curveDock->widget() != window) {
        // ADS releases its old child without deleting it. Keep the native
        // playback backend alive while the uploaded CurveWindow owns the UI.
        if (auto* previous = m_curveDock->takeWidget()) {
            previous->setParent(this);
            previous->hide();
        }
        m_curveDock->setWidget(window, ads::CDockWidget::ForceNoScrollArea);
    }
    const QPointer<MotionWindow> owner(this);
    mDockLayoutManager::getInstance()->setActivationHandler([owner](const QString& key) {
        if (!owner) return;
        if (key == QStringLiteral("Curve")) openDock(owner->m_curveDock);
    });
    auto* plot = window->getPlotView();
    if (plot && !plot->property("mxOriginalCurveHostConnected").toBool()) {
        plot->setProperty("mxOriginalCurveHostConnected", true);
        connect(plot, &PlotView::mPlotView::seriesUpdated, this, [this, plot] {
            // Expression and drive-table previews use the same original
            // CurveWindow API, so their real data opens the existing dock too.
            if (!plot->getCurveNames().isEmpty()) openDock(m_curveDock);
        });
    }
}

void MotionWindow::deactivateOriginalCurveHost()
{
    if (!m_workspace || !m_projectCurveWindow) return;
    mCurveWindow::setActiveProjectWindow(nullptr);
    mDockLayoutManager::getInstance()->setActivationHandler({});
}

void MotionWindow::restoreProjectEditor()
{
    const bool resumeAnimation = m_selectionNode.kind == ModelNode::Animation
        && m_animationActive && m_resultScenario == m_selectionNode.id;
    const double seconds = m_playbackTime;
    const bool elastic = m_flexContour->currentData().toInt() == int(FlexContourKind::ElasticDisplacement);
    const bool adaptive = m_flexRange->currentData().toInt() == 0;
    selectRigidNode(m_selectionNode);

    if (!resumeAnimation || !m_editor) return;
    auto* panel = m_editor->findChild<mMotionAnimationPanel*>();
    if (!panel) return;
    if (auto* contour = panel->findChild<QComboBox*>(QStringLiteral("originalMotionFlexContour"))) {
        const QSignalBlocker blocked(contour);
        contour->setCurrentIndex(elastic ? 1 : 0);
    }
    if (auto* scale = panel->findChild<QCheckBox*>(QStringLiteral("originalMotionFlexAdaptiveScale"))) {
        const QSignalBlocker blocked(scale);
        scale->setChecked(adaptive);
    }
    panel->setCurrentTime(seconds);
}

bool MotionWindow::showOriginalMotionResult(ModelNode node)
{
    if (node.kind != ModelNode::Animation && node.kind != ModelNode::Curves && node.kind != ModelNode::Export)
        return false;
    if (!m_model.nativeDocument) return false;
    if (!m_projectActive) return true;
    const auto found = m_results.find(node.id);
    if (motionBusy() || m_jobPart >= 0 || found == m_results.end()
        || found->second->result.frames.empty()) return true;
    const auto run = found->second;
    if (node.kind == ModelNode::Animation) {
        QString error;
        if (!validateRigidPlayback(run->input, run->result, error)) {
            setNotice(error, true);
            return true;
        }
    }

    closeEditor();
    ensureOriginalMotionContext();
    if (!m_original) return true;
    ensureOriginalCurveHost();
    syncOriginalMotionResults(node.id);

    auto* bridge = m_original->viewportModel();
    bridge->motionCurveChannels = [this](int scenarioId) {
        QJsonArray channels;const auto found=m_results.find(scenarioId);
        if(found==m_results.end())return channels;
        const auto& run=*found->second;
        for(const auto& channel:run.result.channels)channels.append(QJsonObject{
            {"sourceType",rigidChannelSourceType(channel.sourceType)}, {"sourceId",channel.sourceId},
            {"sourceName",rigidChannelObjectName(run.input,channel.sourceType,channel.sourceId)},
            {"variable",channel.variable}, {"label",rigidChannelVariableLabel(channel.variable)},
            {"unit",channel.unit}, {"components",channel.components}});
        return channels;
    };
    bridge->queryMotionCurve = [this](int scenarioId,const QString& source,int sourceId,
                                    const QString& variable,int component,QVector<double>& times,
                                    QVector<double>& values,QString& error) {
        times.clear();values.clear();
        const auto found=m_results.find(scenarioId);
        if(found==m_results.end()) { error=tr("当前方案没有结果采样。");return false; }
        RigidCurveRequest request;request.metric=RigidCurveMetric::NativeChannel;
        request.sourceType=source;request.sourceId=sourceId;request.variable=variable;request.component=component;
        const auto series=rigidCurveSeries(found->second->input,found->second->result,request);
        if(!series.isValid()) { error=series.error;return false; }
        times=series.times;values=series.values;error.clear();return true;
    };
    bridge->displayMotionTime = [this](int scenarioId, double seconds, int reference, bool adaptive) {
        if (!m_projectActive || m_results.find(scenarioId) == m_results.end()) return;
        m_resultScenario = scenarioId;
        m_animationActive = true;
        const int contour = int(reference == 0 ? FlexContourKind::TotalDisplacement
                                               : FlexContourKind::ElasticDisplacement);
        const QSignalBlocker contourBlock(m_flexContour), rangeBlock(m_flexRange);
        m_flexContour->setCurrentIndex(m_flexContour->findData(contour));
        m_flexRange->setCurrentIndex(m_flexRange->findData(adaptive ? 0 : 1));
        seekRigidTime(seconds);
    };
    bridge->resetMotionDisplay = [this] {
        if (m_animationDock) m_animationDock->setProperty("mxOriginalAnimationControllerActive", false);
        // Deactivating a project closes its original editor while its viewport
        // can remain visible in a split layout. Keep its last rendered frame.
        if (m_projectActive) leaveRigidAnimation();
    };
    bridge->exportMotionLoads = [this](int id,const QString& path,const NativeLoadSource& source,
                                       const NativeLoadExportOptions& options,QString& error) {
        const auto it=m_results.find(id);
        if (it==m_results.end() || !it->second->input.nativeInput) {
            error=tr("当前方案没有可导出的多体求解结果。");
            return false;
        }
        return exportNativeLoads(path,*it->second->input.nativeInput,it->second->result,source,options,error);
    };
    bridge->buildMotionLoadCases = [this](int id,const NativeLoadCaseOptions& options,QJsonObject& data,QString& error) {
        const auto it=m_results.find(id);
        if (it==m_results.end() || !it->second->input.nativeInput) {
            error=tr("当前方案没有可导出的多体求解结果。");
            return false;
        }
        return buildNativeLoadCases(*it->second->input.nativeInput,it->second->result,options,data,error);
    };

    QWidget* panel = nullptr;
    std::function<void()> enter, leave;
    if (node.kind == ModelNode::Animation) {
        auto* animation = new mMotionAnimationPanel;
        animation->setObjectName(QStringLiteral("originalMotionAnimationPanel"));
        panel = animation;
        const QPointer<mMotionAnimationPanel> pointer(animation);
        enter = [this, animation, id = node.id] {
            m_animationDock->setProperty("mxOriginalAnimationControllerActive", true);
            m_resultScenario = id;
            m_animationActive = true;
            animation->editAnimationPanel(id);
        };
        leave = [pointer] { if (pointer) pointer->uneditAnimationPanel(); };
    } else if (node.kind == ModelNode::Curves) {
        auto* curves = new mMotionCurvePanel;
        curves->setObjectName(QStringLiteral("originalMotionCurvePanel"));
        panel = curves;
        const QPointer<mMotionCurvePanel> pointer(curves);
        enter = [curves, id = node.id] { curves->editMotionCurvePanel(id); };
        leave = [pointer] { if (pointer) pointer->uneditMotionCurvePanel(); };
    } else {
        auto* loads = new mMotionLoadExportPanel;
        loads->setObjectName(QStringLiteral("originalMotionLoadExportPanel"));
        panel = loads;
        const QPointer<mMotionLoadExportPanel> pointer(loads);
        enter = [loads, id = node.id] { loads->editLoadExportPanel(id); };
        leave = [pointer] { if (pointer) pointer->uneditLoadExportPanel(); };
    }
    showOriginalParameterPanel(panel, panel->objectName() + QStringLiteral("Scroll"));
    m_original->unedit = std::move(leave);
    enter();
    // The uploaded result editors create their curve rows/marker checkboxes
    // on entry, so observe those actual controls after they have been built.
    m_original->watch(panel);
    m_selectionNode = node;
    return true;
}

bool MotionWindow::showOriginalMotionLoadImport(const QString& path)
{
    if (motionBusy() || m_jobPart >= 0) return true;
    if (!ensureNativeModel()) return true;
    closeEditor();
    ensureOriginalMotionContext();
    if (!m_original) return true;
    auto* panel=new mMotionLoadImportPanel;
    panel->setObjectName(QStringLiteral("originalMotionLoadImportPanel"));
    auto* scroll=new QScrollArea;
    scroll->setObjectName(QStringLiteral("originalMotionLoadImportScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(panel);
    scroll->setProperty("mxOriginalMotionPanel",true);
    showEditor(scroll);
    const QPointer<mMotionLoadImportPanel> pointer(panel);
    m_original->unedit=[pointer] { if(pointer)pointer->deactivatePickAndCommit(); };
    panel->openWithFile(path);
    m_original->watch(panel);
    return true;
}
}
