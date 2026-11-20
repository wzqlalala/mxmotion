#pragma once

#include "CadDocument.h"
#include "FemMesh.h"

#include <QMainWindow>
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
class QCloseEvent;

namespace ads {
class CDockManager;
class CDockWidget;
class CDockAreaWidget;
}

namespace MotionOcc {

class OccViewport;

enum class Command {
    New, Open, Save, SaveAs, Import, Export, Remove,
    Box, Cylinder, Sphere,
    Fit, Front, Top, Right, Isometric,
    ShadedEdges, Shaded, Wireframe, MeshDisplay,
    PickPart, ClearSelection, ShowAll, HideSelected, HideOthers, InvertVisibility,
    GenerateMesh, RemoveMesh, ExportMesh,
    ResetLayout, ManageLayout, QuickShot, About, Exit
};

class MotionWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MotionWindow(QWidget* parent = nullptr);
    ~MotionWindow() override;

    QAction* command(Command id) const;
    bool importCad(const QString& path);

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void closeEvent(QCloseEvent* event) override;

private:
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
    void saveCad(bool all, bool choosePath);
    void newModel();
    bool confirmReplace();
    void beginMeshSetup();
    void beginMesh();
    void endMesh(int exitCode, const QString& processError = {});
    void cancelMesh();
    void saveMesh();

    CadDocument m_document;
    std::map<Command, QAction*> m_commands;
    std::map<int, FemMesh> m_meshes;
    std::set<int> m_hidden;
    QString m_savePath;
    int m_partId = -1;
    int m_modelNumber = 1;
    bool m_modified = false;
    bool m_syncingTree = false;

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
};

QWidget* createMotionRibbon(MotionWindow& window, QWidget* parent);

} // namespace MotionOcc
