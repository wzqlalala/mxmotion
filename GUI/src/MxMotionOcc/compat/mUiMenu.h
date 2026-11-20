#pragma once

#include <QMenu>
#include <QPointer>

class QStyle;

namespace MxFunctions {

class mUiMenu : public QMenu
{
    Q_OBJECT

public:
    explicit mUiMenu(QWidget* parent = nullptr);
    explicit mUiMenu(const QString& title, QWidget* parent = nullptr);

    static void setSharedStyle(QStyle* style);
    void applyMetrics();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void init();

    QPointer<QStyle> m_appliedStyle;
    int m_metricsLevel = 0;
};

} // namespace MxFunctions

namespace MechMainWindow {
using MxFunctions::mUiMenu;
}
