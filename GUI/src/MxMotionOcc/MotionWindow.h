#pragma once

#include "CadDocument.h"
#include "FemMesh.h"
#include "RigidMotion.h"
#include "NativeMotionDocument.h"

#include <QElapsedTimer>
#include <QMainWindow>
#include <QVariant>
#include <atomic>
#include <functional>
#include <thread>
#include <map>
#include <memory>
#include <set>

class QAction;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QProgressBar;
class QPushButton;
class QTemporaryDir;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class QComboBox;
class QCheckBox;
class QCloseEvent;
class QSlider;
class QVBoxLayout;

namespace ads {
class CDockManager;
class CDockWidget;
class CDockAreaWidget;
}

namespace MotionOcc {

class OccViewport;
class RigidPlot;
class LegacyMotionContext;

struct ModelNode {
    enum Kind { None, Part, Body, Joint, Drive, Scenario, Animation, Export, Curves, Native } kind = None;
    int id = 0;
    QString collection;
};

enum class Command {
    New, Open, Save, SaveAs, Import, Export, Remove,
    Box, Cylinder, Sphere,
    Fit, Front, Top, Right, Isometric,
    NavigationClassic, NavigationNx,
    ShadedEdges, Shaded, Wireframe, MeshDisplay,
    PickPart, ClearSelection, ShowAll, HideSelected, HideOthers, InvertVisibility,
    GenerateMesh, RemoveMesh, ExportMesh,
    AddBody, BatchBodies, AddRevolute, AddPrismatic, AddFixed, AddSpeedDrive, AddPositionDrive,
    NewScenario, Solve, StopSolve, LoadRigidExample,
    ImportMotionJson, ExportMotionJson, AddNativeJoint, AddCoupling, AddSpring,
    AddVectorForce, AddScalarForce, AddScalarTorque, AddThreePointForce, AddThreePointTorque,
    AddContact, AddMarker, AddSensor, AddController, AddControlInput, AddAutoFlex,
    AddSoftStop, AddTensionCable,
    ResetLayout, ManageLayout, QuickShot, About, Exit
};

class MotionWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MotionWindow(QWidget* parent = nullptr);
    ~MotionWindow() override;

    QAction* command(Command id) const;
    bool importCad(const QString& path);
    bool openRigidProject(const QString& path);
    bool importMotionJson(const QString& path, bool interactiveBinding = true);
    void dispatchOriginalMotionAction(const QString& action, const QVariant& argument = {});

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void closeEvent(QCloseEvent* event) override;

private:
    bool dispatchOriginalWorkspaceAction(const QString&, const QVariant&);
    void ensureOriginalMotionContext();
    void ensureOriginalCurveHost();
    bool editOriginalMotionEntity(const QString&, int, const QString&);
    void commitOriginalMotionDocument(const NativeMotionDocument&);
    void publishOriginalSolveProgress(int scenarioId,int percent);
    void publishOriginalSolveFinished(int scenarioId,bool success,const QString& message);
    void syncOriginalMotionResults(int scenarioId);
    bool showOriginalMotionResult(ModelNode);
    bool showOriginalDynamicTreeMenu(QTreeWidgetItem*,const QPoint&);
    bool renameOriginalDynamicTreeItem(QTreeWidgetItem*);
    void deleteOriginalTreeEntities(const QString& collection,int id);
    bool showOriginalMotionLoadImport(const QString& path);
    void createCommands();
    void createWorkspace();
    void createStatusBar();
    QWidget* createModelBrowser();
    QWidget* createParameterPage();
    QWidget* createViewportFrame();
    ads::CDockWidget* addPane(const QString& key, const QString& title, QWidget* content);
    void resetLayout(bool resetVisibility);
    void applyDockTheme();
    void updateTree();
    void updateParameters();
    void updateActions();
    void syncViewport(bool fit = false);
    void selectPart(int id);
    void filterTree();
    void showPartMenu(const QPoint& position);
    void setNotice(const QString& message, bool error = false);
    void addPrimitive(Primitive primitive);
    void openCadDialog(bool addToModel);
    void saveCad(bool all);
    void newModel();
    bool confirmReplace();
    void beginMeshSetup();
    void beginMesh();
    bool beginPartMesh(int partId, double elementSize, MeshKind kind);
    void endMesh(int exitCode, const QString& processError = {});
    void cancelMesh();
    void saveMesh();
    void saveProject(bool choosePath);
    void createRigidControls();
    void createRigidDocks(ads::CDockAreaWidget* bottom);
    void createRigidCommands();
    void updateRigidActions();
    void appendRigidNodes(QTreeWidgetItem* root, const std::map<QString, QTreeWidgetItem*>& groups);
    void selectRigidNode(ModelNode node);
    void showRigidMenu(ModelNode node, const QPoint& globalPosition);
    void editBody(int id = -1);
    void batchBodies();
    void editJoint(JointKind kind, int id = -1);
    void editDrive(DriveKind kind, int id = -1);
    void editScenario(int id = -1);
    void showEditor(QWidget* editor);
    void closeEditor();
    void removeRigidNode(ModelNode node);
    void commitRigid(RigidModel model, ModelNode selection);
    void clearResults();
    void startRigidSolve(int scenarioId);
    void pollRigidSolve();
    void showRigidResult(ModelNode node);
    void seekRigidTime(double seconds);
    void setPlayback(bool play);
    void stopRigidPlayback();
    void leaveRigidAnimation();
    void updatePlaybackControls();
    void loadRigidExample();
    bool motionBusy() const;
    bool canRemovePart(int id) const;
    void createNativeCommands();
    void appendNativeNodes(QTreeWidgetItem* root, const std::map<QString, QTreeWidgetItem*>& groups);
    void editNativeEntity(const QString& collection, int id = -1, const QString& type = {});
    void removeNativeEntity(const QString& collection, int id);
    bool ensureNativeModel();
    void batchNativeBodies();
    void editNativeLoadExport(int scenarioId);
    bool commitNativeDocument(NativeMotionDocument document, ModelNode selection);
    bool resolveNativeBindings(NativeMotionDocument& document, bool interactive);
    void ensureNativeFlexMesh(int flexId);
    void inspectFlexModes(int flexId);
    void snapshotNativeFlexMeshes();
    void updateNativeFlexMeshForPart(int partId);
    void restoreNativeFlexMeshes();

    struct RigidRun {
        RigidInput input;
        RigidResult result;
        QString error;
        SolveStatus status = SolveStatus::Failed;
        int inspectionFlexId = -1; // Standalone basis analysis, not a time history.
        std::atomic_bool stop{false};
        std::atomic_bool done{false};
        std::atomic<double> progress{0};
    };

    CadDocument m_document;
    RigidModel m_model;
    ModelNode m_selectionNode;
    std::map<int, std::shared_ptr<RigidRun>> m_results;
    std::shared_ptr<RigidRun> m_running;
    bool m_runningInputChanged = false;
    std::thread m_solverThread;
    int m_resultScenario = -1;
    bool m_animationActive = false;
    double m_playbackTime = 0.0;
    QElapsedTimer m_playbackClock;
    std::map<Command, QAction*> m_commands;
    std::map<int, FemMesh> m_meshes;
    std::set<int> m_hidden;
    QString m_projectPath;
    int m_partId = -1;
    int m_modelNumber = 1;
    bool m_modified = false;
    bool m_syncingTree = false;
    std::unique_ptr<LegacyMotionContext> m_original;

    OccViewport* m_viewport = nullptr;
    ads::CDockManager* m_docks = nullptr;
    ads::CDockAreaWidget* m_treeArea = nullptr;
    ads::CDockWidget* m_modelDock = nullptr;
    ads::CDockWidget* m_parameterDock = nullptr;
    ads::CDockWidget* m_outputDock = nullptr;
    ads::CDockWidget* m_selectionDock = nullptr;
    QTreeWidget* m_tree = nullptr;
    QTreeWidgetItem* m_partsItem = nullptr;
    QLineEdit* m_filter = nullptr;
    QComboBox* m_visibilityFilter = nullptr;
    QComboBox* m_sortType = nullptr;
    QLabel* m_parameterTitle = nullptr;
    QLabel* m_parameterDetails = nullptr;
    QVBoxLayout* m_parameterLayout = nullptr;
    QWidget* m_editor = nullptr;
    ads::CDockWidget* m_curveDock = nullptr;
    ads::CDockWidget* m_animationDock = nullptr;
    RigidPlot* m_rigidPlot = nullptr;
    QSlider* m_timeline = nullptr;
    QPushButton* m_playToggle = nullptr;
    QPushButton* m_pausePlayback = nullptr;
    QPushButton* m_stopPlayback = nullptr;
    QLabel* m_timeLabel = nullptr;
    QCheckBox* m_loopPlayback = nullptr;
    QComboBox* m_playbackSpeed = nullptr;
    QComboBox* m_flexContour = nullptr;
    QComboBox* m_flexRange = nullptr;
    QPushButton* m_modeInspectButton = nullptr;
    std::map<std::pair<int,int>,double> m_flexGlobalMax;
    QCheckBox* m_followCurveMarker = nullptr;
    QCheckBox* m_clickCurveJump = nullptr;
    QTimer* m_rigidPoll = nullptr;
    QTimer* m_playbackTimer = nullptr;
    QByteArray m_defaultLayout;
    QWidget* m_meshForm = nullptr;
    QComboBox* m_meshKind = nullptr;
    QDoubleSpinBox* m_meshSize = nullptr;
    QPushButton* m_meshGenerateButton = nullptr;
    QPushButton* m_meshCancelButton = nullptr;
    QLabel* m_notice = nullptr;
    QLabel* m_selection = nullptr;
    QLabel* m_selectionDetails = nullptr;
    QPlainTextEdit* m_output = nullptr;
    QProgressBar* m_progress = nullptr;
    QProcess* m_worker = nullptr;
    QTimer* m_meshTimeout = nullptr;
    std::unique_ptr<QTemporaryDir> m_jobDir;
    int m_jobPart = -1;
    MeshKind m_jobKind = MeshKind::Volume;
    QString m_jobError;
    std::function<void(bool)> m_meshCompletion;
};

QWidget* createMotionRibbon(MotionWindow& window, QWidget* parent);

} // namespace MotionOcc
