#include "mTransFileInfoDialog.h"
#include "ui_mTransFileInfoDialog.h"

//Qt
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <mAppConfiguration.h>


//MxFunction
#include "mGlobalSignals.h"

#include "mViewportModel.h"
#include "mViewportModelManager.h"
#include "mIOFileManager.h"
#include "mDataStatusManager.h"

using namespace MAppConfiguration;
using namespace MxFunctions;
//using namespace MechPreData;
namespace MechMainWindow
{
	mTransFileInfoDialog::mTransFileInfoDialog(QWidget *parent): mFramelessDialog(parent)
	{
		_ui = new Ui::mTransFileInfoDialog;
		//.ui 顶层布局装进内容容器再交给基类（基类 this 的布局已被标题栏占用，
		//直接 setupUi(this) 会被 Qt 拒绝挂载）
		QWidget* content = new QWidget(this);
		_ui->setupUi(content);
		setContentWidget(content);

		_solverProcess = new QProcess;
		connect(_solverProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slot_processFinished(int, QProcess::ExitStatus)));

		//连接信号与槽
		connect(_ui->pushButton, SIGNAL(clicked()), this, SLOT(slot_okButton()));
		connect(_ui->pushButton_2, SIGNAL(clicked()), this, SLOT(slot_noButton()));
		connect(this, SIGNAL(readMxFileSig(const QString&)), MechIOFile::mIOFileManager::getInstance(), SLOT(slot_importSolveFile(const QString&)));
		connect(this, SIGNAL(readMeshFileSig(const QString&)), MechIOFile::mIOFileManager::getInstance(), SLOT(slot_importMeshFile(const QString&)));
	}

	mTransFileInfoDialog::~mTransFileInfoDialog()
	{
		if (_solverProcess != nullptr)
		{
			_solverProcess->terminate();
			_solverProcess->waitForFinished();
			delete _solverProcess;
			_solverProcess = nullptr;
		}

	}

	void mTransFileInfoDialog::transMxFile(const QString& fileName, bool isMeshFile)
	{
		//判断文件是否存在
		QFileInfo finfo(fileName);
		if (!finfo.exists())
		{
			mGlobalSignals::getInstance()->outputMessageSig("文件不存在!");
			return;
		}
		_isMeshFile = isMeshFile;
		mGlobalSignals::getInstance()->outputMessageSig("开始转换文件...");
#if defined(_WIN32)
        QString solverpath = QDir(qApp->applicationDirPath() + "/mxlibi/MxSim.exe").absolutePath();
#else
        QString solverpath = QDir(qApp->applicationDirPath() + "/mxlibi/MxSim").absolutePath();
#endif
		_fname = QDir::toNativeSeparators(fileName);
		QStringList cmdparas = QStringList{ "-i", _fname, "-trans" };
		if (_solverProcess->state() == QProcess::NotRunning)
		{
			_solverProcess->start(solverpath, cmdparas);
		}
	}

	void mTransFileInfoDialog::mousePressEvent(QMouseEvent * event)
	{
		if (event->button() == Qt::MiddleButton)
		{
			//确定
			this->slot_okButton();
		}
	}

	void mTransFileInfoDialog::sendReadMxFileSig()
	{
		if (_isMeshFile)
		{
			emit readMeshFileSig(_mxFileName);
		}
		else
		{
			emit readMxFileSig(_mxFileName);
		}
	}

	void mTransFileInfoDialog::slot_okButton()
	{
		sendReadMxFileSig();
		this->close();
	}

	void mTransFileInfoDialog::slot_noButton()
	{
		this->close();
	}

	void mTransFileInfoDialog::slot_processFinished(int exitCode, QProcess::ExitStatus exitStatus)
	{
		if (exitStatus == QProcess::NormalExit)
		{
			mGlobalSignals::getInstance()->outputMessageSig("文件转换完成");
			int index = _fname.lastIndexOf(".");
			QString fileName = _fname.left(index) + ".mx";
			_mxFileName = QDir::fromNativeSeparators(fileName);

			QFileInfo finfo(_fname);
			//判断是否有错误信息文件生成
			QString infofile = finfo.baseName() + "_tranform.msg";
			QFileInfoList list = finfo.absoluteDir().entryInfoList(QStringList{ infofile });
			if (list.size() > 0)
			{
				QString listfile = list.at(0).absoluteFilePath();
				QFile ifn(listfile);
				if (listfile.size() > 0)
				{
					getCVM->getDataStatusManager()->readFileErrorInfo(listfile);
				}
			}

			sendReadMxFileSig();
		}
		else
		{
			mGlobalSignals::getInstance()->outputMessageSig("文件转换失败");
		}
	}
}