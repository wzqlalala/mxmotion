#include "mWorkingDirPanel.h"
#include "mAppConfiguration.h"
MechMainWindow::mWorkingDirPanel::mWorkingDirPanel(QWidget* parent)
{
	workingDirFold = new FoldSection("工作目录");
	workingDir = new QLabel("工作目录");
	auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
	QSettings Settings(path, QSettings::IniFormat);
	Settings.beginGroup("WorkingDir");
	auto map = Settings.childKeys();
	QStringList keys;
	QStringList values;
	if (Settings.childKeys().isEmpty())
	{
		auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
		QSettings Settings(path, QSettings::IniFormat);
		// 添加一条“最近打开”
		QString value1 = QCoreApplication::applicationDirPath()+"/../WorkingDir";
		QString key1 = "WorkingDir/WorkingDir";
		Settings.setValue(key1, value1);
	}
	for (const QString& k : Settings.childKeys()) {
		workingDirEdit = new QLineEdit(Settings.value(k).toString());
	}
	Settings.endGroup();
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
