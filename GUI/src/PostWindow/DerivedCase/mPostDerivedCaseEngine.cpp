#include "mPostDerivedCaseEngine.h"
#include "mPostDerivedCaseStore.h"

#include <QCoreApplication>
#include <QHash>
#include <QProgressDialog>

#include <utility>
#include <vector>

#include "mGlobalSignals.h"
#include "mAppConfiguration.h"

//BasicData
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
// 须在 mIMxdbFile1.h 之后：mIONativeFile.h 的裸 include "mIOEnum.h" 靠 include 栈
// （mIMxdbFile1.h 所在的 PostIOFile 目录）命中 MIOFile 版枚举，先于本行会被
// Common 版遮蔽（与 mPostLinearSuperposition.cpp 同款顺序约束）
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"
#include "mMxdbThread.h"
#include "mPostInfoHub.h"
#include "mPost3DResult.h"
#include "../mPostValueProvider.h"

using namespace MxFunctions;
using namespace PostData;
using namespace MIOFile;

namespace
{
	//读数失败统一落输出窗口（文案口径对齐线性叠加 reportLinearReadFailure；
	//"派生工况"通用于步序列/包络）
	void reportDerivedReadFailure(int frame, const QString& varstr, const QString& compostr,
		const QString& postPathName)
	{
		const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
		mGlobalSignals::getInstance()->outputMessageSig(2,
			isChinese ? QStringLiteral("派生工况读取结果分量失败：帧%1，变量%2，分量%3（%4）")
								.arg(frame).arg(varstr).arg(compostr).arg(postPathName)
					  : QStringLiteral("Failed to read result component for the derived case: frame %1, variable %2, component %3 (%4)")
								.arg(frame).arg(varstr).arg(compostr).arg(postPathName),
			true);
	}
}

namespace PostWindow
{
	QString mPostDerivedCaseEngine::resultVarIdentity(const QString& varstr, const QString& compostr)
	{
		const QString varName = varstr.trimmed();
		const QString compoName = compostr.trimmed();
		QString resultName = varName;
		if (!varName.isEmpty() && !compoName.isEmpty()
			&& !compoName.contains(varName, Qt::CaseInsensitive))
			resultName = varName + QStringLiteral(" ") + compoName;
		return resultName;
	}

	bool mPostDerivedCaseEngine::readSourceFrame(const GenerateContext& ctx, int frameId)
	{
		//统一取值 provider（阶段2）：四格式发布式分支收敛（ElementNodalForce 特判
		//与坐标系参数由 MxdbLegacy 适配器内处理，失败报告口径不变）
		bool readOk = false;
		if (IPostValueProvider* provider = valueProviderFor(ctx.postDB))
		{
			ValueRequest req;
			req.frame = frameId;
			req.var = ctx.var;
			req.compo = ctx.compo;
			req.layer = ctx.layer;
			req.complexMode = ctx.complexMode;
			readOk = provider->publish(req);
			delete provider;
		}
		if (!readOk)
			reportDerivedReadFailure(frameId, ctx.var, ctx.compo, ctx.postPathName);
		return readOk;
	}

	bool mPostDerivedCaseEngine::generateSteps(const DerivedCaseDefinition& def,
		const QList<DerivedCaseDefinition>& allDefs, const GenerateContext& ctx,
		QString* errMsg)
	{
		const auto fail = [errMsg](const QString& message)
		{
			if (errMsg != nullptr)
				*errMsg = message;
			return false;
		};
		//失败/取消统一回滚本次登记的派生帧（旧帧在入口已清，回滚只影响本次）
		const auto rollback = [&ctx, &def]()
		{
			ctx.postDB->removeDerivedCase(def.name);
		};

		if (ctx.postDB == nullptr || def.name.trimmed().isEmpty())
			return fail(QString::fromUtf8("生成上下文无效，请确认已打开后处理结果。"));

		//预检：SolidColor 为伪变量无结果数据，无可物化内容
		if (ctx.var.trimmed().isEmpty() || ctx.var == QStringLiteral("SolidColor"))
			return fail(QString::fromUtf8("请先在云图中选择结果变量。"));

		//预检：派生工况以定义名为工况名物化，不得与分析工况重名
		if (ctx.postDB->getAllCaseNames().contains(def.name))
			return fail(QString::fromUtf8("派生工况名称与分析工况重名，请修改名称。"));

		QList<mPostDerivedCaseStore::ExpandedSource> expanded;
		mPostDerivedCaseStore::ReferenceError expandError = mPostDerivedCaseStore::ReferenceError::None;
		if (!mPostDerivedCaseStore::expandDefinition(allDefs, def.id, &expanded, &expandError)
			|| expanded.isEmpty())
			return fail(QString::fromUtf8("该派生工况待配置，请先配置有效来源。"));

		const QString varIdentity = resultVarIdentity(ctx.var, ctx.compo);

		//再生成语义：先清同名旧派生帧（墓碑、帧号不复用）
		ctx.postDB->removeDerivedCase(def.name);

		QProgressDialog progress(QString::fromUtf8("正在生成派生工况：%1").arg(def.name),
			QString::fromUtf8("取消"), 0, expanded.size(), ctx.progressParent);
		progress.setWindowTitle(QString::fromUtf8("步序列生成"));
		progress.setWindowModality(Qt::WindowModal);
		progress.setMinimumDuration(300);	//短任务不闪框（一键生成动画先例）

		//工况 → 帧显示串 → 帧号（逐工况懒建；来源行帧列是 getCaseFrameNames 的
		//显示串"帧名:Time = …"，getFrameNumByCaseAndFrameName 匹配原始 _frameName，
		//须经 getCaseFrameNamesAndIDs 映射换算）
		QHash<QString, QHash<QString, int>> frameIdByDisplay;

		for (int i = 0; i < expanded.size(); ++i)
		{
			progress.setValue(i);
			QCoreApplication::processEvents();
			if (progress.wasCanceled())
			{
				rollback();
				return fail(QString::fromUtf8("已取消生成。"));
			}

			const mPostDerivedCaseStore::ExpandedSource& src = expanded.at(i);

			QHash<QString, int>& byDisplay = frameIdByDisplay[src.caseName];
			if (byDisplay.isEmpty())
			{
				std::vector<QPair<QString, int>> namesAndIds;
				ctx.postDB->getCaseFrameNamesAndIDs(src.caseName, namesAndIds);
				for (const auto& nameAndId : namesAndIds)
					byDisplay.insert(nameAndId.first, nameAndId.second);
			}
			const int frameId = byDisplay.value(src.frameName, -1);
			if (frameId <= 0)
			{
				rollback();
				return fail(QString::fromUtf8("来源不可用：%1 / %2。")
					.arg(src.caseName).arg(src.frameName));
			}

			//四分支读数（引擎统一读数器，三生成链共用）
			if (!readSourceFrame(ctx, frameId))
			{
				rollback();
				return fail(QString::fromUtf8("读取结果分量失败：帧%1。").arg(frameId));
			}

			const QHash<int, double> source = ctx.postDB->getResultDataByFrameID(frameId);
			if (source.isEmpty())
			{
				rollback();
				return fail(QString::fromUtf8("来源帧无结果数据：帧%1。").arg(frameId));
			}

			QHash<int, double> scaled;
			if (src.scale == 1.0)
				scaled = source;
			else
			{
				for (auto it = source.constBegin(); it != source.constEnd(); ++it)
					scaled[it.key()] = it.value() * src.scale;
			}

			const QString shortName = ctx.postDB->getFrameNameByFrameNum(frameId);
			const QString frameName = QString::fromUtf8("%1: %2/%3").arg(i + 1)
				.arg(src.caseName).arg(shortName.isEmpty() ? src.frameName : shortName);
			const int appended = ctx.postDB->appendDerivedFrame(def.name, frameName,
				ctx.postDB->getFrameTime(frameId), frameId, scaled, varIdentity,
				ctx.postDB->getFrameResultType(frameId));
			if (appended < 0)
			{
				rollback();
				return fail(QString::fromUtf8("物化派生帧失败：帧%1。").arg(frameId));
			}
		}
		progress.setValue(expanded.size());
		return true;
	}

	bool mPostDerivedCaseEngine::envelopeFeed(EnvelopeAccumulator& acc,
		const QHash<int, double>& frameValues)
	{
		if (frameValues.isEmpty())
			return false;	//空帧：交集必空
		if (acc.fedFrames == 0)
		{
			for (auto it = frameValues.constBegin(); it != frameValues.constEnd(); ++it)
			{
				acc.minByKey[it.key()] = it.value();
				acc.maxByKey[it.key()] = it.value();
				acc.minSourceByKey[it.key()] = 0;
				acc.maxSourceByKey[it.key()] = 0;
			}
			acc.fedFrames = 1;
			return true;
		}
		//键域交集收缩：运行域中不在本帧的键剔除（keys() 取拷贝，移除安全）
		const QList<int> runningKeys = acc.minByKey.keys();
		for (int key : runningKeys)
		{
			if (!frameValues.contains(key))
			{
				acc.minByKey.remove(key);
				acc.maxByKey.remove(key);
				acc.minSourceByKey.remove(key);
				acc.maxSourceByKey.remove(key);
				acc.keysIntersected = true;
			}
		}
		//域内键更新运行极值（本帧独有键不在交集域，跳过）
		const int sourceIndex = acc.fedFrames;
		for (auto it = frameValues.constBegin(); it != frameValues.constEnd(); ++it)
		{
			const int key = it.key();
			if (!acc.minByKey.contains(key))
				continue;
			if (it.value() < acc.minByKey[key])
			{
				acc.minByKey[key] = it.value();
				acc.minSourceByKey[key] = sourceIndex;
			}
			if (it.value() > acc.maxByKey[key])
			{
				acc.maxByKey[key] = it.value();
				acc.maxSourceByKey[key] = sourceIndex;
			}
		}
		++acc.fedFrames;
		return !acc.minByKey.isEmpty();
	}

	QHash<int, double> mPostDerivedCaseEngine::envelopeReduce(
		const EnvelopeAccumulator& acc, int envelopeType)
	{
		QHash<int, double> out;
		switch (envelopeType)
		{
		case 0:	//Min
			out = acc.minByKey;
			break;
		case 2:	//Extreme：绝对值最大者取原值（|max|==|min| 并列取 max 侧）
			for (auto it = acc.minByKey.constBegin(); it != acc.minByKey.constEnd(); ++it)
			{
				const double vmin = it.value();
				const double vmax = acc.maxByKey.value(it.key());
				out[it.key()] = (qAbs(vmax) >= qAbs(vmin)) ? vmax : vmin;
			}
			break;
		case 3:	//Range：极差
			for (auto it = acc.minByKey.constBegin(); it != acc.minByKey.constEnd(); ++it)
				out[it.key()] = acc.maxByKey.value(it.key()) - it.value();
			break;
		case 1:	//Max（默认）
		default:
			out = acc.maxByKey;
			break;
		}
		return out;
	}

	bool mPostDerivedCaseEngine::generateEnvelope(const DerivedCaseDefinition& def,
		const QList<DerivedCaseDefinition>& allDefs, const GenerateContext& ctx,
		QString* errMsg)
	{
		const auto fail = [errMsg](const QString& message)
		{
			if (errMsg != nullptr)
				*errMsg = message;
			return false;
		};
		const auto rollback = [&ctx, &def]()
		{
			ctx.postDB->removeDerivedCase(def.name);
		};

		if (ctx.postDB == nullptr || def.name.trimmed().isEmpty())
			return fail(QString::fromUtf8("生成上下文无效，请确认已打开后处理结果。"));
		if (ctx.var.trimmed().isEmpty() || ctx.var == QStringLiteral("SolidColor"))
			return fail(QString::fromUtf8("请先在云图中选择结果变量。"));
		if (ctx.postDB->getAllCaseNames().contains(def.name))
			return fail(QString::fromUtf8("派生工况名称与分析工况重名，请修改名称。"));

		QList<mPostDerivedCaseStore::ExpandedSource> expanded;
		mPostDerivedCaseStore::ReferenceError expandError = mPostDerivedCaseStore::ReferenceError::None;
		if (!mPostDerivedCaseStore::expandDefinition(allDefs, def.id, &expanded, &expandError)
			|| expanded.isEmpty())
			return fail(QString::fromUtf8("该派生工况待配置，请先配置有效来源。"));

		const QString varIdentity = resultVarIdentity(ctx.var, ctx.compo);

		//再生成语义：先清同名旧派生帧（墓碑、帧号不复用）
		ctx.postDB->removeDerivedCase(def.name);

		QProgressDialog progress(QString::fromUtf8("正在生成派生工况：%1").arg(def.name),
			QString::fromUtf8("取消"), 0, expanded.size(), ctx.progressParent);
		progress.setWindowTitle(QString::fromUtf8("包络生成"));
		progress.setWindowModality(Qt::WindowModal);
		progress.setMinimumDuration(300);

		EnvelopeAccumulator acc;
		int geometrySourceFrame = -1;	//首个成功喂入的源帧（几何克隆源）
		double sourceTime = 0.0;
		int sourceResultType = -1;

		QHash<QString, QHash<QString, int>> frameIdByDisplay;	//同 generateSteps：显示串→帧号懒映射

		for (int i = 0; i < expanded.size(); ++i)
		{
			progress.setValue(i);
			QCoreApplication::processEvents();
			if (progress.wasCanceled())
			{
				rollback();
				return fail(QString::fromUtf8("已取消生成。"));
			}

			const mPostDerivedCaseStore::ExpandedSource& src = expanded.at(i);

			QHash<QString, int>& byDisplay = frameIdByDisplay[src.caseName];
			if (byDisplay.isEmpty())
			{
				std::vector<QPair<QString, int>> namesAndIds;
				ctx.postDB->getCaseFrameNamesAndIDs(src.caseName, namesAndIds);
				for (const auto& nameAndId : namesAndIds)
					byDisplay.insert(nameAndId.first, nameAndId.second);
			}
			const int frameId = byDisplay.value(src.frameName, -1);
			if (frameId <= 0)
			{
				rollback();
				return fail(QString::fromUtf8("来源不可用：%1 / %2。")
					.arg(src.caseName).arg(src.frameName));
			}

			//四分支读数（引擎统一读数器，三生成链共用）
			if (!readSourceFrame(ctx, frameId))
			{
				rollback();
				return fail(QString::fromUtf8("读取结果分量失败：帧%1。").arg(frameId));
			}

			const QHash<int, double> source = ctx.postDB->getResultDataByFrameID(frameId);
			if (source.isEmpty())
			{
				rollback();
				return fail(QString::fromUtf8("来源帧无结果数据：帧%1。").arg(frameId));
			}

			QHash<int, double> scaled;
			if (src.scale == 1.0)
				scaled = source;
			else
			{
				for (auto it = source.constBegin(); it != source.constEnd(); ++it)
					scaled[it.key()] = it.value() * src.scale;
			}

			if (!envelopeFeed(acc, scaled))
			{
				rollback();
				return fail(QString::fromUtf8("来源帧键域无交集，无法归约包络。"));
			}
			if (geometrySourceFrame < 0)
			{
				geometrySourceFrame = frameId;
				sourceTime = ctx.postDB->getFrameTime(frameId);
				sourceResultType = ctx.postDB->getFrameResultType(frameId);
			}
		}
		progress.setValue(expanded.size());

		//键域收缩非致命：输出窗提示（脏键剔除按交集归约，用户可感知）
		if (acc.keysIntersected)
		{
			const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
			mGlobalSignals::getInstance()->outputMessageSig(2,
				isChinese ? QStringLiteral("包络生成：来源帧键域不一致，已按交集 %1 个键归约。")
									.arg(acc.minByKey.size())
						  : QStringLiteral("Envelope generation: source frames have inconsistent key domains; reduced over the intersection of %1 keys.")
									.arg(acc.minByKey.size()),
				true);
		}

		const QHash<int, double> values = envelopeReduce(acc, def.envelopeType);
		const int appended = ctx.postDB->appendDerivedFrame(def.name,
			QString::fromUtf8("0: 包络"), sourceTime, geometrySourceFrame, values,
			varIdentity, sourceResultType);
		if (appended < 0)
		{
			rollback();
			return fail(QString::fromUtf8("物化派生帧失败。"));
		}
		return true;
	}
}
