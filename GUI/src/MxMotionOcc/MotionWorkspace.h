#pragma once

#include <QMainWindow>
#include <memory>

class QStackedWidget;

namespace MotionOcc {

class MotionWindow;
class MotionViewPages;

class MotionWorkspace final : public QMainWindow {
    Q_OBJECT
public:
    explicit MotionWorkspace(const QString& initialPath = {});
    ~MotionWorkspace() override;

    MotionWindow* activeProject() const;
    MotionWindow* newProject();
    bool openProject(const QString& path);

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void closeEvent(QCloseEvent* event) override;

private:
    friend class MotionWindow;
    friend class MotionViewPages;
    MotionWindow* projectAt(int index) const;
    QString projectName(const MotionWindow& project) const;
    void addProject(MotionWindow* project);
    void showProjectForViewport(MotionWindow& project, int viewId);
    void closeProject(MotionWindow& project);
    void removeProject(MotionWindow& project);

    QStackedWidget* m_projects = nullptr;
    MotionWindow* m_active = nullptr;
    int m_nextModelNumber = 1;
    std::unique_ptr<MotionViewPages> m_viewManager;
};

} // namespace MotionOcc
