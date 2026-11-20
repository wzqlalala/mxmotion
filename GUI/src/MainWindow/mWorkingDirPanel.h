#pragma once
#pragma execution_character_set("utf-8")

#include "mPanelBase.h"
#include <QtWidgets>
namespace MechMainWindow
{
    class mWorkingDirPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mWorkingDirPanel(QWidget* parent = nullptr);
        ~mWorkingDirPanel();

    public slots:
        
    private slots:
       

    private:
        QLabel* workingDir;
        QLineEdit* workingDirEdit;
        QToolButton* workingDirOpen;
        FoldSection* workingDirFold;
    };
}
