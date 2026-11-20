#include "mPostAutoTestAPIRegistrar.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>
#include <QSet>
#include <QThread>

#include <algorithm>
#include <limits>

#include "mAPIServer.h"
#include "mAPISchedulingServer.h"

#include "mPostInfoHub.h"
#include "mPostValueProvider.h"
#include "mPost3DResult.h"
#include "mPostBrowserWindow.h"
#include "mPostResultDataStruct.h"
#include "mPanelHostProvider.h"

// 结果读取四分支协议与 _postDB 解析同 mPostQueryPanel 口径（readVarCompones/updateQueryPanel）
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPhysicalFieldEnum.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostModelData.h"
#include "mIMxdbFile1.h"
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"
#include "mMxdbThread.h"
#include "mxr/view/REBUILD_PostRend.h"

using namespace PostData;

namespace
{
	//脚本进程内截图前的稳定等待：processEvents 循环（同 mUiShotController::sleepInLoop 思路），
	//避免 QThread::msleep 阻塞主线程导致布局/样式收敛信号丢失
	void settleInLoop(int ms)
	{
		const int step = 50;
		for (int done = 0; done < ms; done += step)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, step);
			QThread::msleep(step);
		}
	}

	//当前后处理数据库：活动视口渲染部件的数据名 → PostData 管理器（updateQueryPanel 同链）
	QPointer<mPostDataBase> activePostDatabase(QString* dataNameOut = nullptr)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr || postRend->getPostRender() == nullptr)
			return nullptr;
		const QString postName = postRend->getPostDataName();
		if (dataNameOut)
			*dataNameOut = postName;
		if (postName.isEmpty())
			return nullptr;
		return getCVM->getModelData()->getManager<mPostDataManager>(MxFunctions::Physical_Post).getPostDatabase(postName);
	}
}

namespace PostWindow
{
	mPostAutoTestAPIRegistrar* mPostAutoTestAPIRegistrar::getInstance()
	{
		static mPostAutoTestAPIRegistrar inst;
		return &inst;
	}

	mPostAutoTestAPIRegistrar::mPostAutoTestAPIRegistrar(QObject* parent)
		: QObject(parent)
	{
	}

	bool mPostAutoTestAPIRegistrar::ensureRegistered()
	{
		if (_registered)
			return true;
		auto* scheduler = getAPISchedulingServer;
		if (scheduler == nullptr)
			return false;

		scheduler->registerAPI("API_Result_OpenFiles", this,
			&mPostAutoTestAPIRegistrar::onResultOpenFiles,
			QString::fromUtf8("打开结果文件并建后处理树（自动化测试）"),
			QStringList{ "files" });
		scheduler->registerAPI("API_Result_ListVariables", this,
			&mPostAutoTestAPIRegistrar::onResultListVariables,
			QString::fromUtf8("列出当前后处理数据库的帧号/变量名（自动化测试）"));
		scheduler->registerAPI("API_Result_QueryValues", this,
			&mPostAutoTestAPIRegistrar::onResultQueryValues,
			QString::fromUtf8("按变量取节点/单元数值并返回JSON摘要（金样本对比）"),
			QStringList{ "variable" },
			QStringList{ "frame", "component", "layer", "complex", "nodes", "maxDump" });
		scheduler->registerAPI("API_Scene_Shot", this,
			&mPostAutoTestAPIRegistrar::onSceneShot,
			QString::fromUtf8("截当前参数面板/整窗PNG（自动化测试）"),
			QStringList{ "out" },
			QStringList{ "target", "settleMs" });

		_registered = true;
		return true;
	}

	bool mPostAutoTestAPIRegistrar::importSettled() const
	{
		return _importStartMs > 0 && _importFinishMs >= _importStartMs;
	}

	QVariant mPostAutoTestAPIRegistrar::onResultOpenFiles(
		const APIServer::APIParams& params, QString& error)
	{
		const QStringList files = APIServer::mAPISchedulingServer::getParamValue(
			params, "files", QStringList{});
		QStringList existing;
		for (const QString& f : files)
		{
			const QString t = f.trimmed();
			if (!t.isEmpty() && QFileInfo::exists(t))
				existing.append(t);
		}
		if (existing.isEmpty())
		{
			error = QString::fromUtf8("files 中没有存在的结果文件: %1").arg(files.join(QLatin1String(", ")));
			return false;
		}

		//先接线再触发导入（快速完成不漏信号）；多文件首个信号即置完成（v1 约定）
		if (auto cloud = mPostInfoHub::getInstance()->cloudResultPanel())
		{
			connect(cloud.get(), &mPost3DResultPanel::finishedReadPostFileSig,
				this, [this](const QString&) {
					_importFinishMs = QDateTime::currentMSecsSinceEpoch();
				}, Qt::UniqueConnection);
		}
		_importStartMs = QDateTime::currentMSecsSinceEpoch();
		_importFinishMs = 0;

		mPostBrowserWindow::getInstance()->openPostResultFiles(existing);

		QJsonObject ret;
		ret.insert("opened", QJsonArray::fromStringList(existing));
		ret.insert(QString::fromUtf8("note"), QString::fromUtf8("已触发导入；完成等待由 postImportIdle 条件承担"));
		return QString::fromUtf8(QJsonDocument(ret).toJson(QJsonDocument::Compact));
	}

	QVariant mPostAutoTestAPIRegistrar::onResultListVariables(
		const APIServer::APIParams& params, QString& error)
	{
		Q_UNUSED(params);
		QPointer<mPostDataBase> db = activePostDatabase();
		if (db == nullptr)
		{
			error = QString::fromUtf8("当前无活动后处理结果（先执行 API_Result_OpenFiles）");
			return false;
		}
		QJsonObject ret;
		ret.insert("file", db->getFile());
		ret.insert("caseId", db->getCaseId());
		QJsonArray frames;
		const QList<int> frameIds = db->getFrameIds();
		for (int f : frameIds)
			frames.append(f);
		ret.insert("frames", frames);
		ret.insert("variables", QJsonArray::fromStringList(db->getAllVariableNames()));
		return QString::fromUtf8(QJsonDocument(ret).toJson(QJsonDocument::Compact));
	}

	QVariant mPostAutoTestAPIRegistrar::onResultQueryValues(
		const APIServer::APIParams& params, QString& error)
	{
		const QStringList variable = APIServer::mAPISchedulingServer::getParamValue(
			params, "variable", QStringList{});
		if (variable.isEmpty() || variable.first().trimmed().isEmpty())
		{
			error = QString::fromUtf8("缺少必需参数 variable");
			return false;
		}
		const QString varstr = variable.first().trimmed();
		const QString compostr = APIServer::mAPISchedulingServer::getParamValue(
			params, "component", QStringList{ QString() }).first();
		const QString layerstr = APIServer::mAPISchedulingServer::getParamValue(
			params, "layer", QStringList{ QString() }).first();
		const QString complexStr = APIServer::mAPISchedulingServer::getParamValue(
			params, "complex", QStringList{ QString() }).first();
		const QStringList nodesParam = APIServer::mAPISchedulingServer::getParamValue(
			params, "nodes", QStringList{});
		bool maxDumpOk = false;
		//并发会话 WIP 编译错最小修复（qBound 三参+解析值原义保留：失败取默认 50，钳 0..100000）
		const int maxDumpRequested = APIServer::mAPISchedulingServer::getParamValue(
			params, "maxDump", QStringList{ "50" }).first().toInt(&maxDumpOk);
		const int maxDump = qBound(0, maxDumpOk ? maxDumpRequested : 50, 100000);

		QPointer<mPostDataBase> db = activePostDatabase();
		if (db == nullptr)
		{
			error = QString::fromUtf8("当前无活动后处理结果（先执行 API_Result_OpenFiles）");
			return false;
		}

		//帧号缺省取首帧
		const QList<int> frameIds = db->getFrameIds();
		if (frameIds.isEmpty())
		{
			error = QString::fromUtf8("后处理数据库无帧数据");
			return false;
		}
		bool frameOk = false;
		const int frameParam = APIServer::mAPISchedulingServer::getParamValue(
			params, "frame", QStringList{}).value(0).toInt(&frameOk);
		const int frame = frameOk ? frameParam : frameIds.first();

		//----统一取值 provider（阶段2）：四分支收敛为出参式单口（键域/失败语义逐点保持）----
		const QString postPathName = db->getFile();
		auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel();
		if (cloudPanel == nullptr)
		{
			error = QString::fromUtf8("云图结果面板不可用");
			return false;
		}
		Compones compone;
		bool ok = false;
		if (IPostValueProvider* provider = valueProviderFor(db))
		{
			ValueRequest req;
			req.frame = frame;
			req.var = varstr;
			req.compo = compostr;
			req.layer = layerstr;
			req.complexMode = complexStr;
			ok = provider->fetch(req, compone);
			delete provider;
		}
		if (ok && db->getDataFlag())
		{
			//mxdb 老链路：readCloudData 只填 value1/value2 且不置 isNodeFace，
			//节点位（flag==0）键是连续 id——换算真实 id（云图 STL 导出同款坑）
			if (QPointer<mPostModelData> md = db->getModelData(frame))
			{
				if (compone.flag == 0)
				{
					QHash<int, double> real;
					for (auto it = compone.value1.cbegin(); it != compone.value1.cend(); ++it)
					{
						const int k = it.key();
						real.insert(k >= 0 && k < md->_nodedata.nodeamount
							? md->_nodedata.ids[k] : k, it.value());
					}
					compone.value1 = real;
					compone.isNodeFace = true;
				}
				else
					compone.isNodeFace = false;
			}
		}
		if (!ok)
		{
			error = QString::fromUtf8("变量读取失败: frame=%1 variable=%2 component=%3 file=%4")
				.arg(frame).arg(varstr, compostr, postPathName);
			return false;
		}

		//----统计摘要 + 指定节点值（金样本对比口径：值排序取前 maxDump 个）----
		QList<QPair<int, double>> solid;
		solid.reserve(compone.value1.size());
		for (auto it = compone.value1.cbegin(); it != compone.value1.cend(); ++it)
			solid.append(qMakePair(it.key(), it.value()));
		QList<QPair<int, double>> shell;
		shell.reserve(compone.value2.size());
		for (auto it = compone.value2.cbegin(); it != compone.value2.cend(); ++it)
			shell.append(qMakePair(it.key(), it.value()));
		std::sort(solid.begin(), solid.end());
		std::sort(shell.begin(), shell.end());

		QSet<int> wanted;
		for (const QString& n : nodesParam)
		{
			bool numOk = false;
			const int id = n.trimmed().toInt(&numOk);
			if (numOk)
				wanted.insert(id);
		}

		auto dumpValues = [&wanted, maxDump](const QList<QPair<int, double>>& values) {
			QJsonObject obj;
			int dumped = 0;
			double vmin = std::numeric_limits<double>::max();
			double vmax = -std::numeric_limits<double>::max();
			double sum = 0;
			for (const auto& kv : values)
			{
				vmin = qMin(vmin, kv.second);
				vmax = qMax(vmax, kv.second);
				sum += kv.second;
				const bool pick = wanted.isEmpty() ? dumped < maxDump : wanted.contains(kv.first);
				if (pick && dumped < (wanted.isEmpty() ? maxDump : 100000))
				{
					obj.insert(QString::number(kv.first), kv.second);
					++dumped;
				}
			}
			QJsonObject stat;
			stat.insert("count", values.size());
			if (!values.isEmpty())
			{
				stat.insert("min", vmin);
				stat.insert("max", vmax);
				stat.insert("mean", sum / values.size());
			}
			stat.insert("dumped", dumped);
			QJsonObject out;
			out.insert("stat", stat);
			out.insert("values", obj);
			return out;
		};

		QJsonObject ret;
		ret.insert("variable", varstr);
		ret.insert("component", compostr);
		ret.insert("frame", frame);
		ret.insert("file", postPathName);
		ret.insert("flag", compone.flag);
		ret.insert("isNodeFace", compone.isNodeFace);
		ret.insert("solid", dumpValues(solid));
		ret.insert("shell", dumpValues(shell));
		return QString::fromUtf8(QJsonDocument(ret).toJson(QJsonDocument::Compact));
	}

	QVariant mPostAutoTestAPIRegistrar::onSceneShot(
		const APIServer::APIParams& params, QString& error)
	{
		const QString out = APIServer::mAPISchedulingServer::getParamValue(
			params, "out", QStringList{}).value(0);
		if (out.trimmed().isEmpty())
		{
			error = QString::fromUtf8("缺少必需参数 out（PNG 输出路径）");
			return false;
		}
		const QString target = APIServer::mAPISchedulingServer::getParamValue(
			params, "target", QStringList{ "panel" }).first().toLower();
		bool settleOk = false;
		const int settleMs = APIServer::mAPISchedulingServer::getParamValue(
			params, "settleMs", QStringList{ "400" }).first().toInt(&settleOk) ? 400 : 400;

		QWidget* panel = MechCommon::panelHostRef() ? MechCommon::panelHostRef()->mainWidget() : nullptr;
		if (panel == nullptr)
		{
			error = QString::fromUtf8("未取得面板部件（宿主未注册或面板未挂载）");
			return false;
		}
		settleInLoop(settleMs);

		QWidget* subject = panel;
		if (target == QLatin1String("window"))
			subject = panel->window();   //Info dock 主部件的顶层即主窗口（含 3D 视口）
		else if (target != QLatin1String("panel"))
		{
			error = QString::fromUtf8("target 只支持 panel/window，收到: %1").arg(target);
			return false;
		}

		const QPixmap pix = subject->grab();
		if (!pix.save(out))
		{
			error = QString::fromUtf8("截图保存失败: %1").arg(out);
			return false;
		}
		QJsonObject ret;
		ret.insert("out", QDir::toNativeSeparators(out));
		ret.insert("target", target);
		ret.insert("width", pix.width());
		ret.insert("height", pix.height());
		return QString::fromUtf8(QJsonDocument(ret).toJson(QJsonDocument::Compact));
	}
}
