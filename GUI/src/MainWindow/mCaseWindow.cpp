 
#include "mCaseWindow.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mTitleBar.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QFont>
#include <QSplitter>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QLibraryInfo>
#include <QFile>
#include <QtGlobal>

#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

using namespace MechPreData;
using namespace MechCommon;
namespace MechMainWindow
{
    mCaseWindow* mCaseWindow::_instance = nullptr;
    mCaseWindow* mCaseWindow::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mCaseWindow;
            _instance->setObjectName("OutputDock");
        }
        return _instance;
    }

    mCaseWindow::~mCaseWindow()
    {
        if (view)
            delete view;
    }

    QWebEngineView* mCaseWindow::getView()
    {
        return view;
    }

    bool mCaseWindow::checkQtWebEngineProcessExists()
    {
#ifdef Q_OS_WIN
        QString processPath = QCoreApplication::applicationDirPath() + "/QtWebEngineProcess.exe";
        bool isExist = QFile::exists(processPath);
        if (!isExist)
        {
            processPath = QLibraryInfo::location(QLibraryInfo::LibraryExecutablesPath) + "/QtWebEngineProcess.exe";
            isExist = QFile::exists(processPath);
        }
        return isExist;
#else
        QString processPath = QCoreApplication::applicationDirPath() + "/QtWebEngineProcess";
        bool isExist = QFile::exists(processPath);
        if (!isExist)
        {
            processPath = QLibraryInfo::location(QLibraryInfo::LibraryExecutablesPath) + "/QtWebEngineProcess";
            isExist = QFile::exists(processPath);
        }
        return isExist;
#endif
    }

    mCaseWindow::mCaseWindow(QWidget* parent) :QWidget(parent)
    {
        //设置允许的位置
        this->setStyleSheet("QTextEdit { font-size: 14px; }");
        //QString styleSheet = "QPushButton:checked { background: #E5ECF0; }";

        QLabel* stack_label = new QLabel("案例文档");
        QVBoxLayout* v = new QVBoxLayout;
        this->setLayout(v);

        if (checkQtWebEngineProcessExists())
        {
	    view = new QWebEngineView(this);
		QWebEngineSettings* settings = view->page()->settings();
		settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
#endif
		view->setWindowState(Qt::WindowMaximized);

            QString file = QStringLiteral("案例操作手册.pdf");
            QString pdf = QApplication::applicationDirPath() + "/../案例操作手册.pdf";
            QString pdfDir = QCoreApplication::applicationDirPath() + "/..";
            QDir().mkpath(pdfDir);                 // 确保目录存在
            view->show();
            //view->load(QUrl(pdf));
            v->addWidget(stack_label);
            v->addWidget(view, 1);
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("QtWebEngineProcess.exe 不存在，无法加载 PDF 文档。"));
            view = nullptr;
            v->addWidget(stack_label);
        }
    }
}
