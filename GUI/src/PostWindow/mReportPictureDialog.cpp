#include "mReportPictureDialog.h"

#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QIcon>

#include "mOutputMessage.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mPostExportWord.h"
#include "mCommonFunctions.h"
namespace Common
{
	class mViewportManager;
}
using namespace PostCommon;
using namespace MAppConfiguration;
namespace PostWindow
{
	mReportPictureDialog::mReportPictureDialog(QWidget *parent)
		: MechMainWindow::mFramelessDialog(parent)
	{
		if (objectName().isEmpty())
			setObjectName(QString::fromUtf8("mReportPictureDialog"));
		resize(530, 260);

		//内容装进中转容器再交给基类（基类 this 的布局已被标题栏占用）
		QWidget* content = new QWidget(this);
		QGridLayout* gridLayout = new QGridLayout(content);
		gridLayout->setSpacing(6);
		gridLayout->setContentsMargins(11, 11, 11, 11);
		gridLayout->setObjectName(QString::fromUtf8("gridLayout"));

		//--------- 左列：标题行 + 截图列表 + 工具按钮行 ---------
		_listWidget = new QListWidget(this);
		_listWidget->setObjectName(QString::fromUtf8("listWidget"));
		gridLayout->addWidget(_listWidget, 1, 0, 1, 1);

		QHBoxLayout* horizontalLayout = new QHBoxLayout();
		horizontalLayout->setSpacing(6);
		horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
		_label = new QLabel(this);
		_label->setObjectName(QString::fromUtf8("label"));
		horizontalLayout->addWidget(_label);
		horizontalLayout->addItem(new QSpacerItem(188, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
		gridLayout->addLayout(horizontalLayout, 0, 0, 1, 1);

		QHBoxLayout* horizontalLayout_4 = new QHBoxLayout();
		horizontalLayout_4->setSpacing(6);
		horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
		_toolButton = new QToolButton(this);
		_toolButton->setObjectName(QString::fromUtf8("toolButton"));
		QIcon icon;
		icon.addFile(QString::fromUtf8(":/mMainWidgets/Icon/up.png"), QSize(), QIcon::Normal, QIcon::Off);
		_toolButton->setIcon(icon);
		horizontalLayout_4->addWidget(_toolButton);

		_toolButton_2 = new QToolButton(this);
		_toolButton_2->setObjectName(QString::fromUtf8("toolButton_2"));
		QIcon icon1;
		icon1.addFile(QString::fromUtf8(":/mMainWidgets/Icon/down.png"), QSize(), QIcon::Normal, QIcon::Off);
		_toolButton_2->setIcon(icon1);
		horizontalLayout_4->addWidget(_toolButton_2);

		_toolButton_3 = new QToolButton(this);
		_toolButton_3->setObjectName(QString::fromUtf8("toolButton_3"));
		QIcon icon2;
		icon2.addFile(QString::fromUtf8(":/mMainWidgets/Icon/minus.png"), QSize(), QIcon::Normal, QIcon::Off);
		_toolButton_3->setIcon(icon2);
		horizontalLayout_4->addWidget(_toolButton_3);

		horizontalLayout_4->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

		_pushButton = new QPushButton(this);
		_pushButton->setObjectName(QString::fromUtf8("pushButton"));
		_pushButton->setMinimumSize(QSize(80, 0));
		horizontalLayout_4->addWidget(_pushButton);

		gridLayout->addLayout(horizontalLayout_4, 2, 0, 1, 2);

		//--------- 右列：标题输入 + 预览 ---------
		QVBoxLayout* verticalLayout = new QVBoxLayout();
		verticalLayout->setSpacing(6);
		verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

		QHBoxLayout* horizontalLayout_3 = new QHBoxLayout();
		horizontalLayout_3->setSpacing(6);
		horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
		_label_4 = new QLabel(this);
		_label_4->setObjectName(QString::fromUtf8("label_4"));
		horizontalLayout_3->addWidget(_label_4);
		_lineEdit = new QLineEdit(this);
		_lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
		horizontalLayout_3->addWidget(_lineEdit);
		verticalLayout->addLayout(horizontalLayout_3);

		QHBoxLayout* horizontalLayout_2 = new QHBoxLayout();
		horizontalLayout_2->setSpacing(6);
		horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
		_label_2 = new QLabel(this);
		_label_2->setObjectName(QString::fromUtf8("label_2"));
		horizontalLayout_2->addWidget(_label_2);
		horizontalLayout_2->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
		verticalLayout->addLayout(horizontalLayout_2);

		_label_3 = new QLabel(this);
		_label_3->setObjectName(QString::fromUtf8("label_3"));
		verticalLayout->addWidget(_label_3);

		verticalLayout->setStretch(2, 1);
		gridLayout->addLayout(verticalLayout, 0, 1, 2, 1);

		gridLayout->setColumnStretch(1, 1);

		setContentWidget(content);

		_wordReport = dynamic_cast<mPostExportWordPanel*>(this->parent());

		//连接信号与槽
		connect(_listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slot_clickPictureList(QListWidgetItem*)));
		connect(_lineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(slot_titleEdited(const QString&)));
		connect(_pushButton, SIGNAL(clicked()), this, SLOT(close()));
		connect(_toolButton, SIGNAL(clicked()), this, SLOT(slot_moveUp()));
		connect(_toolButton_2, SIGNAL(clicked()), this, SLOT(slot_moveDown()));
		connect(_toolButton_3, SIGNAL(clicked()), this, SLOT(slot_delete()));

		//切换界面语言
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
		slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
	}

	void mReportPictureDialog::updatePictureList(const QStringList& names)
	{
		PostCommon::mCommonFunctions::clearListWidget(_listWidget);
		_listWidget->addItems(names);
	       if (names.count() == 0)
	       {
	           _label_3->clear();
	           _lineEdit->clear();
	           return;
	       }
	       _listWidget->setCurrentRow(0);
	       QListWidgetItem* item = _listWidget->item(0);
	       slot_clickPictureList(item);
	}

	void mReportPictureDialog::setImageDir(const QString& dir)
	{
		_imageDir = dir;
	}

	QString mReportPictureDialog::legendImageDir() const
	{
		return _imageDir.isEmpty() ? QCoreApplication::applicationDirPath() + "/Report/post-images/" : _imageDir + "/";
	}

	QString mReportPictureDialog::getPictureLegendName(const QString & name)
	{
		//兼容全路径与纯文件名两种入参，统一按文件名取题注
		const QString fileName = QFileInfo(name).fileName();
		//优先取运行时属性；属性不持久化（重启/重存图片后会丢），回退读取图片旁的题注文件
		QString legend = _listWidget->property(fileName.toStdString().c_str()).toString();
		if (!legend.isEmpty())
			return legend;

		QString sidecar = legendImageDir() + fileName + ".txt";
		QFile file(sidecar);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			legend = QString::fromUtf8(file.readAll());
			file.close();
			if (!legend.isEmpty())
				_listWidget->setProperty(fileName.toStdString().c_str(), legend);
		}
		return legend;
	}

	void mReportPictureDialog::resizeEvent(QResizeEvent * event)
	{
		//获取名称
		QString name = this->property("截图").toString();
		if (name.isEmpty())
			return;

		//预览
	       QString imagesdir = _imageDir.isEmpty() ? QCoreApplication::applicationDirPath() + "/Report/post-images/" : _imageDir + "/";
	       QPixmap pixmap(imagesdir + name);
		pixmap = pixmap.scaled(_label_3->width(), pixmap.height()*((double)_label_3->width() / pixmap.width()), Qt::KeepAspectRatio);
		_label_3->setPixmap(pixmap);
	}

	void mReportPictureDialog::slot_clickPictureList(QListWidgetItem * item)
	{
		if (item == nullptr)
		{
			return;
		}
		QString name = item->text();
		this->setProperty("截图", name);

		//显示图例名称
		QString legend = getPictureLegendName(name);
		_lineEdit->setText(legend);

		//预览
		      QString imagesdir = _imageDir.isEmpty() ? QCoreApplication::applicationDirPath() + "/Report/post-images/" : _imageDir + "/";
		QPixmap pixmap(imagesdir + name);
		pixmap = pixmap.scaled(_label_3->width(), pixmap.height()*((double)_label_3->width() / pixmap.width()), Qt::KeepAspectRatio);
		_label_3->setPixmap(pixmap);
	}

	void mReportPictureDialog::slot_titleEdited(const QString & text)
	{
		QString picname = this->property("截图").toString();
		if (picname.isEmpty())
			return;
		//运行时属性 + 题注文件双写，保证题注跨会话/跨图片重存不丢失
		_listWidget->setProperty(picname.toStdString().c_str(), text);

		QString sidecar = legendImageDir() + picname + ".txt";
		QFile file(sidecar);
		if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			file.write(text.toUtf8());
			file.close();
		}
	}

    void mReportPictureDialog::slot_moveUp()
    {
        int index = _listWidget->currentRow();
        if (index == 0 || index == -1)
            return;
        _listWidget->setCurrentRow(index - 1);

        slot_clickPictureList(_listWidget->currentItem());
    }

    void mReportPictureDialog::slot_moveDown()
    {
        int index = _listWidget->currentRow();
        int count = _listWidget->count();
        count = count - 1;
        if (index == count)
            return;
        _listWidget->setCurrentRow(index + 1);

        slot_clickPictureList(_listWidget->currentItem());
    }

    void mReportPictureDialog::slot_delete()
    {
        QListWidgetItem *item = _listWidget->currentItem();
        if (item == nullptr)
            return;

        QString itemName = item->text();
        int index = _listWidget->currentRow();
        _listWidget->takeItem(index);
        _wordReport->updateUserImages(itemName);

        if (_listWidget->count() == 0)
        {
            _label_3->clear();
            _lineEdit->clear();
        }

        slot_clickPictureList(_listWidget->currentItem());
    }

    void mReportPictureDialog::slot_changeLanguage(int language)
    {
        if (language == 0)
        {
            this->setWindowTitle("截图列表");
            _label->setText("截图列表");
            _label_4->setText("标题");
            _label_2->setText("预览");
            _pushButton->setText("关闭");
        }
        if (language == 1)
        {
            this->setWindowTitle("Screenshot List");
            _label->setText("Screenshot List");
            _label_4->setText("Title");
            _label_2->setText("Preview");
            _pushButton->setText("Close");
        }
    }
}
