#include "mStrengthReportAutoCapture.h"

#include "mStrengthReportAutoPreview.h"   //AutoAssembler::compositeResultName（T3-3① 复合格式唯一镜像）
#include "mStrengthReportCollector.h"     //saveViewportScreenshot（LogoFlipGuard RAII 零改动复用）
#include "mStrengthReportConfig.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QMetaObject>
#include <QTimer>

#include "mViewportModelManager.h"        //getCVM（只读调用——Collector::currentPostDb 同源链）
#include "mViewportModel.h"               //getGUIDataByViewName（guidata 公共字段写面）
#include "mxr/view/REBUILD_PostRend.h"    //getPostRenderWidget/getViewportName/getPostRender
#include "mGlobalSignals.h"               //outputMessageSig（进度/失败分类消息）
#include "mMechGlobalSignals.h"           //MechCommon::mMechGlobalSignals::changedFrame（仅帧维
                                          //应用切帧链——与用户手动操作完全同链，试点同源）
#include "mPostDataBase.h"                //getAllCaseNames/getAllVariableNames/getVariableCompos
                                          //+getResultVarNameByFrameID（缓存旁证只读）
#include "mPostDataManager.h"             //Physical_Post 管理器→getPostDatabase（只读）
#include "mPostGUIData.h"                 //guidata 公共字段（_cloudCase/_cloudFrame/…——13 §4.4）
#include "mPostInfoHub.h"                 //cloudResultPanel()（mPostInfoHub.h:137 公开）
#include "mPost3DResult.h"                //mPost3DResultPanel 完整类型（reloadCloudFromGUIData
                                          //public 入口 mPost3DResult.h:183-190——只读调用）
#include "mModelData.h"
#include "mPhysicalFieldEnum.h"

#include <cmath>

using namespace MechPreData;
using namespace PostData;
using namespace MxFunctions;

namespace StrengthReport
{
	namespace
	{
		//单槽预算（NFR-3.1：单槽 ≤5s，批量 ≤N×5s；三道闸全部在预算内——回显校验闸早退
		//即跳不贴上限、缓存旁证宽限 share 同一预算）
		const int kSlotBudgetMs = 5000;
		//缓存旁证单次轮询间隔（回显校验判据 b 的宽限轮询步长）
		const int kCachePollMs = 100;
		//上屏防抖（D3-4 初值 200ms，S6 真机实测回填——二期试点同源定值）
		const int kDebounceMs = 200;

		//Collector::currentPostDb 同源链的文件内副本（批次 B AutoPreview 同惯例——
		//Collector 匿名命名空间符号不可跨 TU 复用）
		QPointer<mPostDataBase> currentPostDbLocal()
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr)
				return nullptr;
			const QString postname = postRend->getPostDataName();
			QPointer<mPostDataBase> postDB = getCVM->getModelData()
				->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			return postDB;
		}

		QString reasonText(const QString& code, const QString& slotKey)
		{
			if (code == QStringLiteral("ViewportClosed"))
				return QStringLiteral("槽「%1」无激活后处理视口/视口已关闭，该槽留空（缺项策略），后续槽继续").arg(slotKey);
			if (code == QStringLiteral("RenderUnreachable"))
				return QStringLiteral("槽「%1」3D 渲染不可用，该槽留空（缺项策略）").arg(slotKey);
			if (code == QStringLiteral("Playing"))
				return QStringLiteral("槽「%1」动画播放中，该槽留空（请先停止动画再导出）").arg(slotKey);
			if (code == QStringLiteral("MappingMismatch"))
				return QStringLiteral("槽「%1」对应设置与当前工况/变量/分量不一致，请检查[对应设置…]；该槽留空（按缺项策略）").arg(slotKey);
			if (code == QStringLiteral("Timeout"))
				return QStringLiteral("槽「%1」画面就绪等待超时（等待 5 秒），该槽留空（缺项策略）").arg(slotKey);
			if (code == QStringLiteral("Aborted"))
				return QStringLiteral("槽「%1」批量截图被中止（视口/对话框关闭），该槽留空").arg(slotKey);
			if (code == QStringLiteral("GrabFailed"))
				return QStringLiteral("槽「%1」截图保存失败，该槽留空（缺项策略）").arg(slotKey);
			if (code == QStringLiteral("ImageUnreadable"))
				return QStringLiteral("槽「%1」截图文件不可读（疑为空图或写盘异常），该槽留空（请重新截图或核对显示）").arg(slotKey);
			return code;
		}
	}

	mStrengthReportAutoCapture::mStrengthReportAutoCapture(QObject* parent)
		: QObject(parent)
	{
	}

	QMap<QString, CaptureMapping> mStrengthReportAutoCapture::mappingsFromConfig(
		const mStrengthReportProjectConfig* cfg, const QStringList& slotKeys)
	{
		QMap<QString, CaptureMapping> out;
		if (cfg == nullptr)
			return out;
		for (const QString& key : slotKeys)
		{
			CaptureMapping m;
			const QJsonObject quad = cfg->slotMappings.value(key);
			if (!quad.isEmpty())
			{
				//四元组解析（批次 B Config load 同款校验：frame>0 才有效；case/var/compo
				//可空=仅帧维；var=="SolidColor"=网格图伪变量→仅帧维应用，13 §3 批次 C）
				m.caseName = quad.value(QStringLiteral("case")).toString();
				m.frameId = quad.value(QStringLiteral("frame")).toInt(-1);
				m.var = quad.value(QStringLiteral("var")).toString();
				m.compo = quad.value(QStringLiteral("compo")).toString();
				m.frameOnly = m.var.isEmpty()
					|| m.var.compare(QStringLiteral("SolidColor"), Qt::CaseInsensitive) == 0;
			}
			else
			{
				//R3-9：旧 slotFrameMap int 形态=四元组仅帧维特例（O3-2 兼容读取——
				//slotFrameMap/slotMappings 各自独立装载互不迁移，消费面按此优先序）
				const int legacyFrame = cfg->slotFrameMap.value(key, -1);
				if (legacyFrame <= 0)
					continue;   //两处皆无=该槽未映射（调用方=跳过槽保持现状，非失败）
				m.frameId = legacyFrame;
				m.frameOnly = true;
			}
			out.insert(key, m);
		}
		return out;
	}

	QString mStrengthReportAutoCapture::classifySlotFailure(bool viewportOk, bool renderOk,
		bool playing, bool mappingValid, bool preGuardOk, bool received, bool aborted,
		bool deadlineHit, bool cacheMismatch)
	{
		//分类优先级（T3-2：中止/不可达/失配先于超时——失败原因分类正确性=FR-334 验收项）
		if (aborted)
			return QStringLiteral("Aborted");
		if (!viewportOk)
			return QStringLiteral("ViewportClosed");
		if (!renderOk)
			return QStringLiteral("RenderUnreachable");
		if (playing)
			return QStringLiteral("Playing");
		if (!mappingValid)
			return QStringLiteral("MappingMismatch");
		if (!preGuardOk)
			return QStringLiteral("MappingMismatch");   //reloadCloudFromGUIData 早退守卫前置复刻
		if (cacheMismatch)
			return QStringLiteral("MappingMismatch");   //缓存旁证失配=E3-8，不落超时误报（T3-2）
		if (deadlineHit)
			return QStringLiteral("Timeout");
		if (!received)
			return QStringLiteral("Timeout");
		return QString();
	}

	bool mStrengthReportAutoCapture::isSuspectMonochrome(const QImage& img)
	{
		//O3-5 双判据（13 §4.4 步④）：64×64 降采样→亮度极差≤ε1 且 标准差≤ε2→可疑。
		//ε 初值保守（ε1=6/255、ε2=2.0——黑图/纯色/近纯色命中，网格图细线降采样混合后
		//极差远超 ε1 不误伤；真机矩阵定版）。图空=false（调用方按不可读分类，防假图）
		if (img.isNull() || img.width() <= 0 || img.height() <= 0)
			return false;
		const QImage small = img.scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
			.convertToFormat(QImage::Format_Grayscale8);
		if (small.isNull())
			return false;
		double sum = 0.0;
		double sumSq = 0.0;
		int minV = 255;
		int maxV = 0;
		const int n = small.width() * small.height();
		for (int y = 0; y < small.height(); ++y)
		{
			const uchar* line = small.constScanLine(y);
			for (int x = 0; x < small.width(); ++x)
			{
				const int v = line[x];
				sum += v;
				sumSq += double(v) * double(v);
				if (v < minV) minV = v;
				if (v > maxV) maxV = v;
			}
		}
		const double mean = sum / n;
		const double std = std::sqrt(sumSq / n - mean * mean);
		return (maxV - minV) <= 6 && std <= 2.0;
	}

	void mStrengthReportAutoCapture::abortBatch()
	{
		//R3-7/B-6① 同口径：等待期间视口关闭/移除/对话框关闭→立即中止（无落图）。
		//仅在批量活跃时生效；旗标由 runBatch 复位
		if (_batchActive)
		{
			_batchAborted = true;
			if (_batchLoop != nullptr)
				_batchLoop->quit();
		}
	}

	QList<CaptureSlotResult> mStrengthReportAutoCapture::runBatch(
		const QMap<QString, CaptureMapping>& mappings, const QStringList& slotKeys,
		bool watermarkOff, QWidget* hideDuringGrab,
		const std::function<void(int, int, const QString&)>& progress)
	{
		QList<CaptureSlotResult> results;
		_batchAborted = false;
		_batchActive = true;
		const int total = slotKeys.size();
		for (int i = 0; i < total; ++i)
		{
			const QString& key = slotKeys.at(i);
			if (progress != nullptr)
				progress(i + 1, total, key);
			CaptureSlotResult r;
			r.slotKey = key;
			if (!mappings.contains(key))
			{
				r.skipped = true;   //未映射=跳过（保持槽现状，非失败——12 §2.5 映射驱动口径）
				r.reason = QStringLiteral("槽「%1」未设置对应，跳过（保持现状）").arg(key);
				results.append(r);
				continue;
			}
			if (_batchAborted)
			{
				//上一槽中止后剩余槽全部留空（中止通道语义——不再逐槽尝试）
				r.reasonCode = QStringLiteral("Aborted");
				r.reason = reasonText(r.reasonCode, key);
				results.append(r);
				continue;
			}
			r = captureOne(mappings.value(key), key, watermarkOff, hideDuringGrab);
			if (r.ok)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
					QStringLiteral("[强度报告]槽「%1」自动截图已保存：%2%3")
						.arg(key, r.path, r.suspect ? QStringLiteral("（⚠ 可疑·近单色，请核对缩略图）")
							: QString()), true);
			}
			else
			{
				//E3-6：逐槽失败不阻塞——原因分类聚合，后续槽继续
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
					QStringLiteral("[强度报告]%1").arg(r.reason), true);
			}
			results.append(r);
		}
		_batchActive = false;
		_batchAborted = false;
		return results;
	}

	CaptureSlotResult mStrengthReportAutoCapture::captureOne(
		const CaptureMapping& mapping, const QString& slotKey, bool watermarkOff,
		QWidget* hideDuringGrab)
	{
		CaptureSlotResult r;
		r.slotKey = slotKey;
		QElapsedTimer budget;
		budget.start();

		//——入口闸（与试点 grabWithAlign 同序：可达性→播放态→映射有效性）——
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		std::shared_ptr<mxr::REBUILD_PostRender> postRender = rend != nullptr
			? rend->getPostRender() : nullptr;
		const bool playing = postRender != nullptr
			&& postRender->playbackState() == mxr::PlaybackState::Playing;
		QPointer<mPostDataBase> postDB = currentPostDbLocal();
		const bool fullApply = !mapping.frameOnly;   //全量应用（工况+帧+变量+分量）
		bool mappingValid = true;
		if (fullApply)
		{
			//工况有效性（绝不猜源——映射工况不在候选表=失配 E3-8，13 §5 R3-8）
			const QStringList cases = postDB != nullptr ? postDB->getAllCaseNames() : QStringList();
			mappingValid = !mapping.caseName.isEmpty() && cases.contains(mapping.caseName);
		}
		QString failCode = classifySlotFailure(rend != nullptr, postRender != nullptr, playing,
			mappingValid, /*preGuardOk*/true, /*received*/true, /*aborted*/false,
			/*deadlineHit*/false, /*cacheMismatch*/false);
		if (!failCode.isEmpty())
		{
			r.reasonCode = failCode;
			r.reason = reasonText(failCode, slotKey);
			return r;
		}

		//C-1 硬约束②：订阅 connect 必须先于切换指令——同线程 direct connection 在切换
		//调用栈内同步发射（reloadCloudFromGUIData 链尾 seekFrame(fi,UserSeek,emit=true)
		//mPost3DResult.cpp:4858），先切后连必漏收（09 §0.3/试点同源）
		const QString viewName = rend->getViewportName();
		const int targetFrame = mapping.frameId;
		bool received = false;
		const QMetaObject::Connection readyConn = connect(postRender.get(),
			&mxr::REBUILD_PostRender::frameChanged, this,
			[&received, targetFrame](int frameId, mxr::FrameChangeSource source)
			{
				//C-1 硬约束①：frameId==目标帧 且 source==UserSeek 双过滤——动画步进/
				//他人切帧不提前放行等待，防抓到错帧（09 §0.3）
				if (frameId == targetFrame && source == mxr::FrameChangeSource::UserSeek)
					received = true;
			});

		//——步① 映射应用（13 §4.4 调用序；T3-2/A-2：早退守卫前置复刻，不落超时误报）——
		bool preGuardOk = true;
		if (fullApply)
		{
			//reloadCloudFromGUIData 五条早退守卫（mPost3DResult.cpp:1953-1977）中四条可在
			//调用前公开复刻（postRend 空/postName 空/modeldb 空/guidata 空）——任一命中=
			//链必早退（收口写回不会发生），直接分类 E3-8 跳槽留空（第五条"帧模型数据空"
			//无公开预检入口→由等待后缓存旁证失配分类兜底，不误报超时）
			mPostGUIData* gui = getCVM->getGUIDataByViewName(viewName);
			const QString postname = rend->getPostDataName();
			preGuardOk = gui != nullptr && !postname.isEmpty() && postDB != nullptr;
			if (preGuardOk)
			{
				//调用序=变形面板生产链 mPostDeformation.cpp:710-721 同构（13 §4.4）：
				//候选列表就绪→四元组写 guidata 公共字段→public 回放入口全量重绘
				gui->_cloudCases = postDB->getAllCaseNames();
				gui->_cloudVarNames = postDB->getAllVariableNames();
				gui->_cloudCompos = postDB->getVariableCompos(mapping.var);
				if (!mapping.caseName.isEmpty())
					gui->_cloudCase = mapping.caseName;
				gui->_cloudFrame = targetFrame;
				gui->_cloudVarName = mapping.var;
				gui->_cloudCompo = mapping.compo;
				PostWindow::mPostInfoHub::getInstance()->cloudResultPanel()->reloadCloudFromGUIData(viewName);
				//链内（同步）：updateResultInfoGUI 回填五下拉→drawCloudMap→slot_showCloud→
				//seekFrame(fi,UserSeek,emit=true)→frameChanged(UserSeek)（上方 received 置位）
			}
		}
		else if (targetFrame > 0)
		{
			//仅帧维应用（SolidColor 伪变量槽/旧 int 映射）：与用户手动切帧完全同链——
			//MechCommon 全局信号 changedFrame→云图面板 slot_changedFrame（面板构造期连接，
			//mPost3DResult.cpp:631）→seekFrame(fi,UserSeek,emit=true)；零触碰面板文件
			//（调用先例 mPostAdvancedQuery.cpp:413/试点 grabWithAlign 同源）
			MechCommon::mMechGlobalSignals::getInstance()->changedFrame(targetFrame);
		}

		//——步② 就绪等待（direct 同步发射时 received 可能已置位；异步收敛/链路不可达走
		//等待循环——单槽 5s 预算封顶，中止通道可提前退出）——
		//LG-01 修法兑现：SolidColor 无帧 quad（网格图槽采纳建议形态，Config::load 放行）
		//=帧维缺省抓当前帧——无帧号判据可等（不切帧不等待），直接视为就绪（绝不因缺
		//帧号落入下方 !received 分类误报 Timeout）
		if (!fullApply && targetFrame <= 0)
			received = true;
		bool aborted = false;
		bool deadlineHit = false;
		if (!received && (fullApply || targetFrame > 0) && preGuardOk)
		{
			const QMetaObject::Connection loopConn = connect(postRender.get(),
				&mxr::REBUILD_PostRender::frameChanged, this,
				[this, &received, targetFrame](int frameId, mxr::FrameChangeSource source)
				{
					if (frameId == targetFrame && source == mxr::FrameChangeSource::UserSeek)
					{
						received = true;
						if (_batchLoop != nullptr)
							_batchLoop->quit();
					}
				});
			QEventLoop waitLoop;
			_batchLoop = &waitLoop;   //abortBatch→quit（视口关闭/对话框关闭中止通道）
			QTimer waitTimer;
			waitTimer.setSingleShot(true);
			connect(&waitTimer, &QTimer::timeout, &waitLoop, &QEventLoop::quit);
			const int remain = kSlotBudgetMs - int(budget.elapsed());
			waitTimer.start(qMax(100, remain));
			waitLoop.exec();
			_batchLoop = nullptr;
			disconnect(loopConn);
			deadlineHit = !received && budget.elapsed() >= kSlotBudgetMs;
		}
		disconnect(readyConn);
		if (_batchAborted)
			aborted = true;

		//——步②' 回显校验（T3-2/A-2 防自证修订：面板链侧独立判据，不依赖 guidata 收口）——
		//判据 a（重绘证明）=frameChanged(target,UserSeek) 已到（slot_showCloud 实际进入——
		//链尾 seekFrame 只在全量重绘链内发生）；判据 b（缓存旁证）=postDB 单槽结果缓存
		//getResultVarNameByFrameID==复合格式"变量名 分量名"（渲染链分发写面，独立于调用方
		//guidata 写入；mPost3DResult.cpp:4713 读数缓存门同款比对）。全量应用判据 b 必查
		//（received 仍可能对应异步收敛中的错误装载）；失配→宽限轮询（预算内）后仍失配=
		//分类 E3-8"映射应用失配"（不落超时误报）
		bool cacheMismatch = false;
		if (!aborted && preGuardOk && fullApply)
		{
			const QString expected = AutoAssembler::compositeResultName(mapping.var, mapping.compo);
			for (;;)
			{
				//BUG-305：中止即时打断旁证轮询——pollLoop 被 abortBatch quit 后必须直读
				//_batchAborted 归 Aborted 码并立即跳槽（不贴满 5s 预算、原因码不漂移为
				//MappingMismatch——分类正确性 FR-334；中止优先级最高 T3-2）
				if (_batchAborted)
				{
					aborted = true;
					break;
				}
				const QString actual = postDB != nullptr
					? postDB->getResultVarNameByFrameID(targetFrame) : QString();
				if (actual == expected)
					break;   //两判据齐备（received 已为重绘证明）
				if (aborted || budget.elapsed() >= kSlotBudgetMs)
				{
					cacheMismatch = true;   //预算内未证同源=失配（E3-8，绝不抓存疑画面 R3-8）
					break;
				}
				QEventLoop pollLoop;
				_batchLoop = &pollLoop;
				QTimer::singleShot(kCachePollMs, &pollLoop, &QEventLoop::quit);
				pollLoop.exec();
				_batchLoop = nullptr;
			}
		}
		QString failCode2 = classifySlotFailure(/*viewportOk*/true, /*renderOk*/true,
			/*playing*/false, /*mappingValid*/true, preGuardOk, received, aborted,
			deadlineHit, cacheMismatch);
		if (!failCode2.isEmpty())
		{
			r.reasonCode = failCode2;
			r.reason = reasonText(failCode2, slotKey);
			return r;
		}

		//——步③ 就绪后上屏同步+抓取（09 §0.3/D3-4 定版序，试点同源）：update+processEvents
		//一轮→同步 grab()→200ms 防抖→复用抓取链（LogoFlipGuard RAII 十步序，按水印开关）——
		QWidget* subwin = getCVM->activatePostViewport();
		if (subwin == nullptr)
		{
			r.reasonCode = QStringLiteral("ViewportClosed");
			r.reason = reasonText(r.reasonCode, slotKey);
			return r;
		}
		subwin->update();
		QApplication::processEvents();
		subwin->grab();
		QEventLoop debounceLoop;
		QTimer::singleShot(kDebounceMs, &debounceLoop, &QEventLoop::quit);
		debounceLoop.exec();

		QString outPath;
		QString err;
		QString warn;
		if (!mStrengthReportCollector::saveViewportScreenshot(slotKey, hideDuringGrab,
			watermarkOff, outPath, err, &warn))
		{
			r.reasonCode = QStringLiteral("GrabFailed");
			r.reason = err.isEmpty() ? reasonText(r.reasonCode, slotKey) : err;
			return r;
		}
		//——步④ 错图检测（防假图最后闸）：落盘 PNG 回读非空+单色检测（可疑=标"可疑"继续
		//不阻断——O3-5 误报无害偏保守；不可读=失败留空，绝不发假图）——
		const QImage grabbed(outPath);
		if (grabbed.isNull())
		{
			r.reasonCode = QStringLiteral("ImageUnreadable");
			r.reason = reasonText(r.reasonCode, slotKey);
			return r;
		}
		r.ok = true;
		r.path = outPath;
		r.suspect = isSuspectMonochrome(grabbed);
		return r;
	}
}
