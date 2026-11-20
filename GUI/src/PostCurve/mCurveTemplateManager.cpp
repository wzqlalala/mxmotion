#pragma execution_character_set("utf-8")

// =============================================================================
// mCurveTemplateManager —— 模板文件读写 + templateRecents（实现）
// 格式与语义见 mCurveTemplateManager.h 头注（openspec add-template-curve D1/D2）
// =============================================================================

#include "mCurveTemplateManager.h"
#include "CurveCore/CurveDocSerialize.h"
#include "CurveCore/CurveDocument.h"

//QT
#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QSettings>

namespace PostCurve
{
	namespace
	{
		const char TEMPLATE_FILE_MAGIC[] = "MXTPL";
		const quint32 TEMPLATE_FILE_VERSION = 1;
		const int TEMPLATE_RECENTS_MAX = 20;
		const char RECENTS_KEY[] = "PostCurve/templateRecents";

		//recents 所在 ini：默认对齐 mMenuWidget 最近打开的既有约定
		//（applicationDirPath()/../Setting/Settings.ini）
		QString recentsSettingsFile(const QString& injected)
		{
			return injected.isEmpty()
				? QCoreApplication::applicationDirPath() + "/../Setting/Settings.ini"
				: injected;
		}
	}

	mCurveTemplateManager* mCurveTemplateManager::_instance = nullptr;

	mCurveTemplateManager* mCurveTemplateManager::getInstance()
	{
		if (!_instance)
			_instance = new mCurveTemplateManager;
		return _instance;
	}

	mCurveTemplateManager::mCurveTemplateManager(QObject* parent)
		: QObject(parent)
	{
	}

	/*************************** 模板文件（D1） ***************************/

	bool mCurveTemplateManager::saveTemplateFile(const CurveDocument* doc,
		const QList<int>& pageIds, const QString& templateName,
		const QString& filePath, QString* err)
	{
		//页子树块先序列化（参数清单边写边收集，文件级清单与块内同源）
		QStringList params;
		const QByteArray chunk = saveTemplateSubtree(doc, pageIds, &params);
		if (chunk.isEmpty())
		{
			if (err)
				*err = QStringLiteral("模板子树序列化失败（文档为空或页不存在）");
			return false;
		}

		QFile file(filePath);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			if (err)
				*err = QStringLiteral("模板文件无法写入: %1").arg(file.errorString());
			return false;
		}
		QDataStream out(&file);
		out.setVersion(QDataStream::Qt_5_13);
		out.writeRawData(TEMPLATE_FILE_MAGIC, 5);
		out << TEMPLATE_FILE_VERSION;
		out << templateName << QDateTime::currentMSecsSinceEpoch()
			<< QCoreApplication::applicationVersion();
		out << qint32(params.size());
		for (const QString& p : params)
			out << p;
		out << chunk;
		return true;
	}

	bool mCurveTemplateManager::readTemplateMeta(const QString& filePath,
		TemplateMeta* meta, QStringList* paramFiles, QString* err)
	{
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly))
		{
			if (err)
				*err = QStringLiteral("模板文件无法读取: %1").arg(file.errorString());
			return false;
		}
		QDataStream in(&file);
		in.setVersion(QDataStream::Qt_5_13);

		char magic[5] = {};
		in.readRawData(magic, 5);
		quint32 version = 0;
		in >> version;
		if (qstrncmp(magic, TEMPLATE_FILE_MAGIC, 5) != 0 || version > TEMPLATE_FILE_VERSION
			|| in.status() != QDataStream::Ok)
		{
			if (err)
				*err = QStringLiteral("不是有效的模板文件（magic/版本不符）");
			return false;
		}

		TemplateMeta m;
		in >> m.name >> m.savedMsecs >> m.appVersion;
		if (in.status() != QDataStream::Ok)
		{
			if (err)
				*err = QStringLiteral("模板元信息读取失败（文件截断）");
			return false;
		}

		qint32 paramCount = 0;
		in >> paramCount;
		if (in.status() != QDataStream::Ok || paramCount < 0 || paramCount > 100000)
		{
			if (err)
				*err = QStringLiteral("模板参数清单读取失败");
			return false;
		}
		QStringList params;
		for (int i = 0; i < paramCount; ++i)
		{
			QString p;
			in >> p;
			params.append(p);
		}
		if (in.status() != QDataStream::Ok)
		{
			if (err)
				*err = QStringLiteral("模板参数清单读取失败");
			return false;
		}

		if (meta)
			*meta = m;
		if (paramFiles)
			*paramFiles = params;
		return true;
	}

	bool mCurveTemplateManager::restoreTemplateFile(const QString& filePath,
		CurveDocument* doc, const QStringList& paramOverride,
		QList<int>* restoredPageIds, QString* err)
	{
		//文件头校验 + 整块读出页子树块（块自身再验 magic/版本，双保险同源）
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly))
		{
			if (err)
				*err = QStringLiteral("模板文件无法读取: %1").arg(file.errorString());
			return false;
		}
		QDataStream in(&file);
		in.setVersion(QDataStream::Qt_5_13);

		char magic[5] = {};
		in.readRawData(magic, 5);
		quint32 version = 0;
		in >> version;
		if (qstrncmp(magic, TEMPLATE_FILE_MAGIC, 5) != 0 || version > TEMPLATE_FILE_VERSION
			|| in.status() != QDataStream::Ok)
		{
			if (err)
				*err = QStringLiteral("不是有效的模板文件（magic/版本不符）");
			return false;
		}

		//越过元信息与参数清单（恢复语义只依赖页子树块；参数清单供 UI 预览，
		//见 readTemplateMeta）
		TemplateMeta m;
		in >> m.name >> m.savedMsecs >> m.appVersion;
		qint32 paramCount = 0;
		in >> paramCount;
		QString dummy;
		for (int i = 0; i < paramCount && in.status() == QDataStream::Ok; ++i)
			in >> dummy;
		QByteArray chunk;
		in >> chunk;
		if (in.status() != QDataStream::Ok)
		{
			if (err)
				*err = QStringLiteral("模板内容读取失败（文件截断）");
			return false;
		}

		if (!restoreTemplateSubtree(doc, chunk, paramOverride, restoredPageIds))
		{
			if (err)
				*err = QStringLiteral("模板页子树恢复失败（版本或内容不兼容）");
			return false;
		}
		return true;
	}

	/*************************** templateRecents（D2） ***************************/

	QStringList mCurveTemplateManager::templateRecents() const
	{
		QSettings settings(recentsSettingsFile(_settingsFile), QSettings::IniFormat);
		return settings.value(QLatin1String(RECENTS_KEY)).toStringList();
	}

	void mCurveTemplateManager::addTemplateRecent(const QString& filePath)
	{
		if (filePath.isEmpty())
			return;
		{
			QSettings settings(recentsSettingsFile(_settingsFile), QSettings::IniFormat);
			QStringList list = settings.value(QLatin1String(RECENTS_KEY)).toStringList();
			list.removeAll(filePath);
			list.append(filePath);            //最新在尾（时间正序）
			while (list.size() > TEMPLATE_RECENTS_MAX)
				list.removeFirst();           //上限 20 截最旧
			settings.setValue(QLatin1String(RECENTS_KEY), list);
			settings.sync();                  //先落盘再广播（观察者重读必见新清单）
		}
		emit sig_recentsChanged();
	}

	void mCurveTemplateManager::removeTemplateRecent(const QString& filePath)
	{
		{
			QSettings settings(recentsSettingsFile(_settingsFile), QSettings::IniFormat);
			QStringList list = settings.value(QLatin1String(RECENTS_KEY)).toStringList();
			if (!list.contains(filePath))
				return;
			list.removeAll(filePath);
			settings.setValue(QLatin1String(RECENTS_KEY), list);
			settings.sync();
		}
		emit sig_recentsChanged();
	}

	void mCurveTemplateManager::setSettingsFile(const QString& iniPath)
	{
		_settingsFile = iniPath;
	}

	/*************************** 保存链路（任务 2.1） ***************************/

	QString mCurveTemplateManager::saveCurrentAsTemplate(const CurveDocument* doc,
		QWidget* parent)
	{
		//另存为：过滤器对齐设计图（"模板文件 (*.mxtpl)"），自动补扩展名
		const QString filePath = QFileDialog::getSaveFileName(parent,
			QStringLiteral("保存模板"), QString(),
			QStringLiteral("模板文件 (*.mxtpl)"));
		if (filePath.isEmpty())
			return QString();

		const QString name = QFileInfo(filePath).completeBaseName();
		QString err;
		if (!saveTemplateFile(doc, doc ? doc->childNodeIds(doc->rootNodeId())
			: QList<int>(), name, filePath, &err))
		{
			MechMainWindow::mFramelessMessageBox::warning(parent, QStringLiteral("保存模板"),
				err.isEmpty() ? QStringLiteral("模板保存失败") : err);
			return QString();
		}
		addTemplateRecent(QDir::toNativeSeparators(filePath));
		return filePath;
	}
}
