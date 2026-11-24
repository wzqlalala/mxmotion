#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

class QWidget;
namespace MechMainWindow { class ViewportContainer; }

namespace MotionOcc {

class MotionWindow;
class MotionWorkspace;
class OccViewport;

class MotionViewPages final : public QObject {
public:
    struct Page {
        QString name;
        QList<int> viewports;
        QList<int> visibleViewports;
        int layout = 0;
        int active = -1;
    };

    MotionViewPages(MotionWorkspace* workspace, QWidget* parking);
    ~MotionViewPages() override;

    QWidget* createHost(MotionWindow& project, QWidget* parent);
    void registerProject(MotionWindow& project);
    void activateProject(MotionWindow& project);
    void removeProject(MotionWindow& project);
    void projectNameChanged(const MotionWindow& project);
    void mount(MotionWindow& project);
    void beginShutdown();

    int currentPage() const { return m_currentPage; }
    int pageCount() const { return m_pages.size(); }
    const Page& page(int index) const { return m_pages.at(index); }
    int activeViewport() const { return m_activeViewport; }
    QString viewportDisplayTitle(int id) const;
    void setLayout(int layout);
    void selectPage(int index);
    void selectViewport(int id);
    void addLayout(int preset);
    void addViewport(int pageIndex, MotionWindow& project);
    bool canRemoveViewport(int id) const;
    void removeViewport(int id);
    void removeViewportModel(int id);
    void removePage(int index);
    void renamePage(int index, const QString& name);
    void renameViewport(int id, const QString& name);
    void tile(int pageIndex, const QList<int>& chosen);
    bool linked() const { return m_linked; }
    void setLinked(bool linked);

private:
    struct View {
        MotionWindow* project = nullptr;
        OccViewport* widget = nullptr;
        QString customName;
        int projectViewNumber = 0;
    };

    Page newPage();
    int pageOf(int id) const;
    int createViewport(MotionWindow& project);
    void eraseViewport(int id);
    void eraseEmptyPage(int index);
    QList<int> visibleViewports(const Page& page) const;
    bool changeSelection(int pageIndex, int id);
    void applyLayout();
    void repairLayout();
    void refreshControls(bool rebuildTree);
    void synchronizeCameras(int source);

    MotionWorkspace* m_workspace;
    QWidget* m_parking;
    MotionWindow* m_standaloneProject = nullptr;
    std::unique_ptr<MechMainWindow::ViewportContainer> m_container;
    QVector<Page> m_pages;
    QMap<int, View> m_views;
    int m_currentPage = 0;
    int m_activeViewport = -1;
    int m_nextPageNumber = 1;
    int m_nextViewportId = 1;
    bool m_linked = false;
    bool m_changingSelection = false;
    bool m_shuttingDown = false;
};

} // namespace MotionOcc
