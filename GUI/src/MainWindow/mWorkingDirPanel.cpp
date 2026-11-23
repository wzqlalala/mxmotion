#include "mWorkingDirPanel.h"
#include "mAppConfiguration.h"
MechMainWindow::mWorkingDirPanel::mWorkingDirPanel(QWidget* parent)
{
	workingDirFold = new FoldSection("工作目录");
	workingDir = new QLabel("工作目录");
	// Always create the original editor, including the first run with no
	// Settings.ini. The previous branch wrote through a second QSettings and
	// then dereferenced an uninitialised workingDirEdit in that case.
	workingDirEdit = new QLineEdit(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
	workingDirEdit->setReadOnly(true);
	workingDirOpen = new QToolButton();
	workingDirOpen->setIcon(QIcon(":/mMainWidgets/Icon/homepage/dakai.png"));
	workingDirFold->addRow(workingDir, workingDirEdit, workingDirOpen);
	//打开按钮只在构造时连接一次：面板经 mInfoWindow::ensurePanel 复用同一实例，
	//若在 createWorkingDir 的每次触发路径里连接，会随菜单点击累积重复连接、弹窗多次弹出
	connect(workingDirOpen, &QToolButton::clicked, this, [=]() {
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;

		QFileInfo info(defaultDir);
		if (defaultDir.isEmpty() || !info.exists())
		{
			defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
		}
		QString openFile = QFileDialog::getExistingDirectory(
			this, tr("选择文件夹"), defaultDir,
			QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

		if (openFile.isEmpty())
		{
			return;
		}
		MAppConfiguration::mAppConfiguration::getInstance()->_workingDir = openFile;
		workingDirEdit->setText(openFile);
		auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
		QSettings Settings(path, QSettings::IniFormat);
		// 添加一条“最近打开”
		QString value1 = openFile;
		QString key1 = "WorkingDir/WorkingDir";
		Settings.setValue(key1, value1);
		});
	QVBoxLayout* mainLay = new QVBoxLayout;
	mainLay->addWidget(workingDirFold);
	mainLay->addStretch();
	this->setLayout(mainLay);
}

MechMainWindow::mWorkingDirPanel::~mWorkingDirPanel()
{
}
