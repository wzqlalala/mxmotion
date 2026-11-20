#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 模板曲线应用管线（追加/叠加/替换）实现，接口说明见 mTemplateApplyPipeline.h
//
// 回放骨架（一个撤销宏内逐页执行，全部结构变更走控制器命令栈）：
//   追加   addPage -> 页属性回放 -> 逐模板图表 addChart -> 环境回放 -> 曲线/基准线/注释
//   替换   目标组按位对位：目标图表内既有曲线/基准线/注释删除（骨架保留）
//          -> 环境回放（kChartType 先行 + 图表/页眉页脚/轴/图例）-> 曲线/基准线/注释
//   叠加   目标组按位对位：目标图表环境不动，仅曲线/基准线/注释重放
//          （叠加模式同名自动加" (N)"后缀为兼容保留，R23 起重名允许；基准线受 skipBaselines 门控）
//   曲线：装配/直写/求值先行，失败即跳过不建节点
//        （addCurve/addComplexCurve/addBarCurve -> setSeriesSource（映射后实际通道）
//        -> setSeriesData/setComplexSeriesData -> 样式与壳属性回放）
//   应用成功后向 mCurveLayerManager 登记层（层名=模板名，任务 5.1）
// =============================================================================

#include "mTemplateApplyPipeline.h"
#include "mCurveTemplateManager.h"
#include "mCurveDataSourceStore.h"
#include "mCurveExpressionEvaluator.h"
#include "mChannelMapCore.h"
#include "mCurveLayerManager.h"
#include "CurveWidgets/mCurveChannelSelector.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"

//QT
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QRegularExpression>
#include <QUndoStack>
#include <QVector>

namespace PostCurve
{
	namespace
	{
		//--- 应用模式（对外接口的私有分派） ---
		enum ApplyMode { ModeAppend = 0, ModeOverlay = 1, ModeReplace = 2 };

		//--- 属性回放表（不含需特殊处置的键：曲线 kColor 走配色选项、
		//轴 kAutoScale/kMin/kMax 走自动缩放选项，均单独回放） ---
		const QVector<QString>& pageProps()
		{
			static const QVector<QString> props = {
				CurveProp::kWidth, CurveProp::kFont, CurveProp::kLayout, CurveProp::kPageTitle,
				CurveProp::kPublishLayout, CurveProp::kPublishPageTitle, CurveProp::kPublishContentPos,
			};
			return props;
		}
		const QVector<QString>& chartProps()
		{
			static const QVector<QString> props = {
				CurveProp::kName, CurveProp::kChartVisible, CurveProp::kBackgroundColor,
				CurveProp::kGridColor, CurveProp::kComplexDisplayFormat, CurveProp::kComplexSwapped,
			};
			return props;
		}
		const QVector<QString>& headerFooterProps()
		{
			static const QVector<QString> props = {
				CurveProp::kHeaderVisible, CurveProp::kHeaderText, CurveProp::kHeaderAlign,
				CurveProp::kHeaderFont, CurveProp::kHeaderFont2, CurveProp::kHeaderFont3,
				CurveProp::kHeaderColor,
				CurveProp::kFooterVisible, CurveProp::kFooterText, CurveProp::kFooterAlign,
				CurveProp::kFooterFont, CurveProp::kFooterFont2, CurveProp::kFooterFont3,
				CurveProp::kFooterColor,
			};
			return props;
		}
		const QVector<QString>& axisProps()
		{
			static const QVector<QString> props = {
				CurveProp::kName, CurveProp::kTitle, CurveProp::kTitleVisible, CurveProp::kTitleColor,
				CurveProp::kVisible, CurveProp::kScaleMode, CurveProp::kWeightMode,
				CurveProp::kTitleFont, CurveProp::kLabelFont, CurveProp::kUnit, CurveProp::kAxisOffset,
				CurveProp::kMajorStep, CurveProp::kMinorCount, CurveProp::kDbRef, CurveProp::kLabelFormat,
				CurveProp::kTickGenMode, CurveProp::kTickCount, CurveProp::kDecadeTickCount,
				CurveProp::kDecadeMinorCount, CurveProp::kPrecision, CurveProp::kCategoryLabelAlign,
			};
			return props;
		}
		const QVector<QString>& legendProps()
		{
			static const QVector<QString> props = {
				CurveProp::kVisible, CurveProp::kAutoPosition, CurveProp::kPosition,
				CurveProp::kPosX, CurveProp::kPosY, CurveProp::kBorderStyle,
				CurveProp::kLegendBorderWidth, CurveProp::kBorderColor, CurveProp::kLegendFont,
				CurveProp::kTextColorMode, CurveProp::kTextColor, CurveProp::kBgColor,
				CurveProp::kLeaderPos,
			};
			return props;
		}
		const QVector<QString>& curveProps()
		{
			static const QVector<QString> props = {
				CurveProp::kName, CurveProp::kVisible, CurveProp::kLineStyle, CurveProp::kLineWidth,
				CurveProp::kShowSymbol, CurveProp::kSymbolSize, CurveProp::kSymbolColor,
				CurveProp::kSymbolStyle, CurveProp::kSymbolDensity, CurveProp::kSymbolScale,
				CurveProp::kLabelVisible, CurveProp::kLabelContent, CurveProp::kLabelText,
				CurveProp::kLabelPrefix, CurveProp::kLabelSuffix, CurveProp::kLabelPrefixVisible,
				CurveProp::kLabelSuffixVisible, CurveProp::kAreaFill,
			};
			return props;
		}
		const QVector<QString>& curveShellProps()
		{
			static const QVector<QString> props = {
				CurveProp::kScale, CurveProp::kOffset, CurveProp::kPhase, CurveProp::kClipEnabled,
				CurveProp::kClipMin, CurveProp::kClipMax, CurveProp::kExpression,
				CurveProp::kAxisAssign, CurveProp::kUnit,
			};
			return props;
		}
		const QVector<QString>& baselineProps()
		{
			static const QVector<QString> props = {
				CurveProp::kName, CurveProp::kLabel, CurveProp::kOrientation, CurveProp::kValue,
				CurveProp::kColor, CurveProp::kLineStyle, CurveProp::kLineWidth, CurveProp::kVisible,
				CurveProp::kLayer, CurveProp::kLabelFont, CurveProp::kLabelPos, CurveProp::kLink,
			};
			return props;
		}
		const QVector<QString>& annotationProps()
		{
			static const QVector<QString> props = {
				CurveProp::kName, CurveProp::kText, CurveProp::kPosX, CurveProp::kPosY,
				CurveProp::kVisible, CurveProp::kAlign, CurveProp::kFont, CurveProp::kBorderMode,
				CurveProp::kBorderColor, CurveProp::kBorderWidth, CurveProp::kBgColor,
				CurveProp::kAutoPos, CurveProp::kAutoAnchor, CurveProp::kAutoOffset,
				CurveProp::kAttachMode, CurveProp::kAttachCurve, CurveProp::kAttachIndex,
				CurveProp::kMinimized, CurveProp::kMinimizeRadius, CurveProp::kMinimizeColor,
				CurveProp::kScalarFmtOn, CurveProp::kScalarFmtMode, CurveProp::kScalarFmtPrecision,
			};
			return props;
		}

		//从 scratch 读属性 -> 命令栈写入活动文档（无效键/等值由文档与控制器兜底）
		void replayProps(const CurveDocument* from, int fromId, mCurveDocController* to,
			int toId, const QVector<QString>& props)
		{
			for (const QString& p : props)
			{
				const QVariant v = from->nodeProperty(fromId, p);
				if (v.isValid())
					to->setNodeProperty(toId, p, v);
			}
		}

		//曲线是否复数（数据保存时剥除，isComplex 不可靠）：侧来源定义或复数侧
		//表达式在即复数（complexFormat 跟随复数创建必写）
		bool isComplexCandidate(const CurveSeriesData& s)
		{
			return !s.complexSideSchema.isEmpty() || !s.complexSideComponent.isEmpty()
				|| !s.complexRef.expression.isEmpty();
		}

		//引用向量语法 p(\d+)w(\d+)c(\d+)\.([xy])（大小写不敏感，与求值器同源）
		bool parseRefToken(const QString& token, int* page, int* chart, int* curve, QChar* side)
		{
			static const QRegularExpression re(QStringLiteral(
				"^p(\\d+)w(\\d+)c(\\d+)\\.([xy])$"),
				QRegularExpression::CaseInsensitiveOption);
			const QRegularExpressionMatch m = re.match(token.trimmed());
			if (!m.hasMatch())
				return false;
			*page = m.captured(1).toInt();
			*chart = m.captured(2).toInt();
			*curve = m.captured(3).toInt();
			*side = m.captured(4).at(0).toLower();
			return true;
		}

		//模板位置表键（模板页序/视口序/曲线序均为 1 基位置序）
		QString chartKey(int tplPage, int tplChart) { return QStringLiteral("p%1w%2").arg(tplPage).arg(tplChart); }
		QString curveKey(int tplPage, int tplChart, int tplCurve)
		{
			return QStringLiteral("p%1w%2c%3").arg(tplPage).arg(tplChart).arg(tplCurve);
		}

		//目标图表内曲线叠加同名自动加 " (N)" 后缀（兼容保留：R23 起重名允许、
		//渲染键由视图层唯一化；追加/替换目标图表无同名，走通不改名）
		QString uniqueCurveName(const CurveDocument* doc, int chartId, const QString& name)
		{
			QSet<QString> taken;
			for (int cid : doc->childNodeIds(chartId))
			{
				if (doc->nodeType(cid) == CurveNodeType::Curve)
					taken.insert(doc->nodeName(cid));
			}
			if (!taken.contains(name))
				return name;
			for (int n = 2; ; ++n)
			{
				const QString cand = QStringLiteral("%1 (%2)").arg(name).arg(n);
				if (!taken.contains(cand))
					return cand;
			}
		}

		//--- 数学曲线重放求值：引用按模板位置表解析到新建曲线（跳过曲线引用失败
		//整体跳过），表达式保持模板形态原样求值。镜像 mDefineCurveDlg::buildMathData
		//的取值语义（x 向量=自变量取采样索引；X 采样=xText 优先，其次首个引用） ---
		class MathReplayContext
		{
		public:
			const CurveDocument* liveDoc = nullptr;
			QHash<QString, int> chartByTpl;   //chartKey -> 目标图表 id（新建或复用）
			QHash<QString, int> curveByTpl;   //curveKey -> 新曲线 id（0=跳过/未创建）

			bool buildRefData(const QString& token, CurveRefData* out, QString* err) const
			{
				int p = 0, w = 0, c = 0;
				QChar side;
				if (!parseRefToken(token, &p, &w, &c, &side) || !liveDoc)
				{
					*err = QStringLiteral("引用向量无效：%1").arg(token);
					return false;
				}
				const int chartId = chartByTpl.value(chartKey(p, w), 0);
				const int curveId = chartId > 0 ? curveByTpl.value(curveKey(p, w, c), 0) : 0;
				const CurveNode* node = curveId > 0 ? liveDoc->node(curveId) : nullptr;
				if (!node)
				{
					*err = QStringLiteral("引用曲线未生成或已跳过：%1").arg(token);
					return false;
				}
				const CurveSeriesData& s = node->series;
				if (side == QLatin1Char('x'))
				{
					out->x.resize(s.xData.size());          //自变量=采样索引
					for (int i = 0; i < s.xData.size(); ++i)
						out->x[i] = double(i);
					out->y = s.xData;
				}
				else
				{
					out->x = s.xData;
					out->y = s.yData;
				}
				if (out->x.isEmpty() || out->x.size() != out->y.size())
				{
					*err = QStringLiteral("引用向量无有效数据：%1").arg(token);
					return false;
				}
				out->ref = token.toLower();
				return true;
			}

			bool evaluate(const CurveSeriesData& s, QVector<double>* xs, QVector<double>* ys,
				QString* err) const
			{
				const QString expr = s.yRef.expression.trimmed();
				if (expr.isEmpty())
				{
					*err = QStringLiteral("表达式为空");
					return false;
				}
				const QStringList refs = mCurveExpressionEvaluator::extractRefs(expr);

				//X 采样：xText（落模约定存 x 侧 expression 字段）优先，其次首个引用
				if (!s.xRef.expression.trimmed().isEmpty())
				{
					int p = 0, w = 0, c = 0;
					QChar side;
					if (!parseRefToken(s.xRef.expression, &p, &w, &c, &side))
					{
						*err = QStringLiteral("X 向量引用无效：%1").arg(s.xRef.expression.trimmed());
						return false;
					}
					CurveRefData d;
					if (!buildRefData(s.xRef.expression, &d, err))
						return false;
					*xs = (side == QLatin1Char('x')) ? d.y : d.x;   //x 向量引用取其值序列
				}
				else if (!refs.isEmpty())
				{
					CurveRefData d;
					if (!buildRefData(refs.first(), &d, err))
						return false;
					*xs = d.x;
				}
				else
				{
					*err = QStringLiteral("表达式无曲线向量引用且未指定 X 向量");
					return false;
				}
				if (xs->isEmpty())
				{
					*err = QStringLiteral("X 向量无数据");
					return false;
				}

				QList<CurveRefData> refData;
				for (const QString& ref : refs)
				{
					CurveRefData d;
					if (!buildRefData(ref, &d, err))
						return false;
					refData.push_back(d);
				}
				return mCurveExpressionEvaluator::evaluate(expr, refData, *xs, *ys, *err);
			}
		};

		//按通道五元组（模型显示串）解析回 id 形态并装配 X/Y 序列。通道策略
		//（任务 4.5）按侧独立收口于此（add-complex-side-channel-parity）：
		//   跳过验证(0)   = 模板五元组直装（既有"块级判定+无 secondary 告警跳过"防线兜底）；
		//   选择最佳匹配(2) = 精确失配时按归一化相似度自动配对（无命中告警跳过）；
		//   校验通道(1)   = 按 channelMap 映射表装配（未配置行跳过不产假数据）。
		//ym 锚 / yp / x（通道形态时）三路各按自身四元组键解析：
		//   - yp 侧：complexSide 结果类型非空 = 有独立四元组，按同策略独立解析
		//    （校验通道下键缺配置即跳过）；complexSide 结果类型为空（仅分量令牌
		//    的旧模板形态）= 回退镜像，b 跟随锚通道+令牌互换（effSelB.isValid()
		//    =false 标记回退态，来源落模按镜像回写）；
		//   - x 侧：仅通道形态（isChannelSchema）参与，横坐标形态恒取锚块横坐标
		//    （effSelX.isValid()=false，xSide 来源不落模）。
		//effSel 带回 ym 实际通道，effSelB/effSelX 带回 yp/x 实际通道（回退态
		//isValid()=false），compBOut 带回复数配对分量
		bool assembleFileCurve(const CurveSeriesData& s, const QSet<QString>& brokenFiles,
			const mTemplateApplyPipeline::Options& opt, ChannelSelection* effSel,
			ChannelSelection* effSelB, ChannelSelection* effSelX,
			QString* compBOut, QVector<double>* x, QVector<double>* ya,
			QVector<double>* yb, QString* err)
		{
			mCurveDataSourceStore* store = mCurveDataSourceStore::getInstance();
			const QString normFile = mCurveDataSourceStore::normalizePath(s.sourceFile);
			if (brokenFiles.contains(normFile))
			{
				*err = QStringLiteral("数据文件不可用：%1").arg(QFileInfo(s.sourceFile).fileName());
				return false;
			}
			const PchReader::PchFile* pf = store->file(normFile);
			if (!pf)
			{
				*err = QStringLiteral("数据文件不在仓库：%1").arg(QFileInfo(s.sourceFile).fileName());
				return false;
			}

			//单侧通道解析（策略分派，锚/yp/x 三路共用）：校验通道查映射表
			//（键=侧四元组+分量，未配置返回 false 态由调用方定语义）；直装/最佳
			//匹配按 fromModelTexts 定位（策略 2 精确失配时按相似度自动配对）
			const auto resolveSide = [&](const QString& subcaseText, const QString& schema,
				const QString& objectText, const QString& component,
				ChannelSelection* out, QString* sideErr) -> bool
			{
				if (opt.channelStrategy == 1)
				{
					ChannelSelection mapped = opt.channelMap.value(
						mChannelMapCore::channelKey(subcaseText, schema, objectText, component));
					if (!mapped.isValid())
					{
						*sideErr = QStringLiteral("通道映射未配置");
						return false;
					}
					mapped.filePath = normFile;   //文件以本次装配目标为准（映射行防路径漂移）
					*out = mapped;
					return true;
				}
				ChannelSelection sel = mCurveChannelSelector::fromModelTexts(
					s.sourceFile, subcaseText, schema, objectText, component);
				if (opt.channelStrategy == 2)
				{
					mCurveDataSourceStore::ChannelDataResult probe;
					QString perr;
					if (!mCurveDataSourceStore::assembleChannelData(*pf, sel.subcaseId, sel.schema,
						sel.pointId, sel.component, &probe, &perr))
					{
						const ChannelSelection best = mChannelMapCore::bestMatchChannel(
							normFile, subcaseText, schema, objectText, component);
						if (!best.isValid())
						{
							*sideErr = QStringLiteral("最佳匹配无命中：%1").arg(perr);
							return false;
						}
						sel = best;
					}
				}
				*out = sel;
				return true;
			};

			//ym 锚侧解析 + a 路装配
			ChannelSelection sel;
			{
				QString selErr;
				if (!resolveSide(s.subcase, s.schema, s.object, s.component, &sel, &selErr))
				{
					*err = selErr;
					return false;
				}
			}
			mCurveDataSourceStore::ChannelDataResult a;
			if (!mCurveDataSourceStore::assembleChannelData(*pf, sel.subcaseId, sel.schema,
				sel.pointId, sel.component, &a, err))
				return false;

			//yp 侧解析 + b 路装配（complexRef 表达式的复数数学曲线不走本函数）
			if (isComplexCandidate(s))
			{
				//下面板分量：模板侧定义优先，缺省按同方向令牌互换配对
				const QString compB = !s.complexSideComponent.isEmpty()
					? s.complexSideComponent
					: mCurveDataSourceStore::siblingComplexComponent(sel.component);
				if (compB.isEmpty())
				{
					*err = QStringLiteral("复数配对分量缺失：%1").arg(sel.component);
					return false;
				}
				ChannelSelection selB;
				QString effCompB = compB;
				const bool perSideB = !s.complexSideSchema.isEmpty();
				if (perSideB)
				{
					QString selErr;
					//键用模板令牌 compB 定位；解析后的分量 = 映射/匹配结果分量
					//（校验通道下用户改配的 yp 分量由此生效）
					if (!resolveSide(s.complexSideSubcase, s.complexSideSchema,
						s.complexSideObject, compB, &selB, &selErr))
					{
						*err = QStringLiteral("yp 侧%1").arg(selErr);
						return false;
					}
					if (!selB.component.isEmpty())
						effCompB = selB.component;
				}
				else
				{
					selB = sel;   //回退镜像：b 跟随锚通道（effSelB 保持无效态）
				}
				mCurveDataSourceStore::ChannelDataResult b;
				if (!mCurveDataSourceStore::assembleChannelData(*pf, selB.subcaseId, selB.schema,
					selB.pointId, effCompB, &b, err))
					return false;
				*yb = b.yData;
				if (compBOut)
					*compBOut = effCompB;
				if (perSideB && effSelB)
					*effSelB = selB;
			}

			//x 侧通道形态重装配横坐标（自选 X 通道模板；setSeriesSource 会把 X 侧
			//复位为横坐标形态，故创建后须 setCurveSideSource 回写；点数与 Y 不配对
			//按"更改曲线"同规跳过）。横坐标形态恒=锚块横坐标（effSelX 保持无效态）
			if (!s.xSideSchema.isEmpty()
				&& mCurveDataSourceStore::isChannelSchema(s.xSideSchema))
			{
				ChannelSelection selX;
				QString selErr;
				if (!resolveSide(s.xSideSubcase, s.xSideSchema, s.xSideObject,
					s.xSideComponent, &selX, &selErr))
				{
					*err = QStringLiteral("x 侧%1").arg(selErr);
					return false;
				}
				mCurveDataSourceStore::ChannelDataResult rx;
				QString xerr;
				if (!mCurveDataSourceStore::assembleChannelData(*pf, selX.subcaseId, selX.schema,
					selX.pointId, selX.component, &rx, &xerr)
					|| rx.yData.size() != a.yData.size())
				{
					*err = QStringLiteral("X侧通道“%1”装配失败或与Y点数不配对")
						.arg(s.xSideComponent);
					return false;
				}
				*x = rx.yData;
				if (effSelX)
					*effSelX = selX;
			}
			*ya = a.yData;
			if (x->isEmpty())
				*x = a.xData;
			*effSel = sel;
			return true;
		}
	}

	//三模式共用核心（前置声明：对外包装在其上，定义在后）
	mTemplateApplyPipeline::Result runApply(int mode, const QString& templatePath,
		const QStringList& paramOverride, const mTemplateApplyPipeline::Options& opt,
		const QVector<int>& targetPageIds);

	mTemplateApplyPipeline::Result mTemplateApplyPipeline::appendTemplate(
		const QString& templatePath, const QStringList& paramOverride, const Options& opt)
	{
		return runApply(ModeAppend, templatePath, paramOverride, opt, QVector<int>());
	}

	mTemplateApplyPipeline::Result mTemplateApplyPipeline::overlayTemplate(
		const QString& templatePath, const QStringList& paramOverride, const Options& opt,
		const QVector<int>& targetPageIds)
	{
		return runApply(ModeOverlay, templatePath, paramOverride, opt, targetPageIds);
	}

	mTemplateApplyPipeline::Result mTemplateApplyPipeline::replaceTemplate(
		const QString& templatePath, const QStringList& paramOverride, const Options& opt,
		const QVector<int>& targetPageIds)
	{
		return runApply(ModeReplace, templatePath, paramOverride, opt, targetPageIds);
	}

	//三模式共用核心（追加/叠加/替换只差目标页/图表解析与环境回放范围）
	mTemplateApplyPipeline::Result runApply(int mode, const QString& templatePath,
		const QStringList& paramOverride, const mTemplateApplyPipeline::Options& opt,
		const QVector<int>& targetPageIds)
	{
		mTemplateApplyPipeline::Result r;
		mCurveDocController* controller = mCurveDocController::getInstance();
		CurveDocument* doc = controller->document();

		//文件头（元信息 + 参数清单）；失败保持会话原状
		mCurveTemplateManager::TemplateMeta meta;
		QStringList paramDefaults;
		QString err;
		if (!mCurveTemplateManager::readTemplateMeta(templatePath, &meta, &paramDefaults, &err))
		{
			r.err = QStringLiteral("模板读取失败：%1").arg(err);
			return r;
		}

		//参数行对齐：弹窗空行（未替换哨兵）回退模板保存时的原始路径
		QStringList effective;
		effective.reserve(paramDefaults.size());
		for (int i = 0; i < paramDefaults.size(); ++i)
		{
			const QString ov = (i < paramOverride.size()) ? paramOverride.at(i) : QString();
			effective << (ov.isEmpty() ? paramDefaults.at(i) : ov);
		}

		//模板子树恢复到暂存文档（不动会话；成功后再进宏回放）
		CurveDocument scratch;
		QList<int> scratchPageIds;
		if (!mCurveTemplateManager::restoreTemplateFile(templatePath, &scratch, effective,
			&scratchPageIds, &err))
		{
			r.err = QStringLiteral("模板恢复失败：%1").arg(err);
			return r;
		}

		//数据文件预登记：模板涉及的文件来源路径全部入共享仓库（幂等；失败记录，
		//曲线装配时整组跳过——环境重建语义，页骨架仍生成）
		mCurveDataSourceStore* store = mCurveDataSourceStore::getInstance();
		QSet<QString> brokenFiles;
		for (int pageId : scratchPageIds)
		{
			for (int chartId : scratch.childNodeIds(pageId))
			{
				if (scratch.nodeType(chartId) != CurveNodeType::Chart)
					continue;
				for (int curveId : scratch.childNodeIds(chartId))
				{
					if (scratch.nodeType(curveId) != CurveNodeType::Curve)
						continue;
					const CurveSeriesData& s = scratch.node(curveId)->series;
					if (curveSeriesSourceKind(s) != CurveSourceKind::File)
						continue;
					const QString norm = mCurveDataSourceStore::normalizePath(s.sourceFile);
					if (store->contains(norm) || brokenFiles.contains(norm))
						continue;
					QString ferr;
					if (!store->addFile(s.sourceFile, &ferr))
						brokenFiles.insert(norm);
				}
			}
		}

		//--- 宏内回放：一个撤销粒度，Ctrl+Z 一次整体撤销 ---
		controller->undoStack()->beginMacro(QStringLiteral("模板曲线-%1").arg(meta.name));
		MathReplayContext mathCtx;
		mathCtx.liveDoc = doc;

		//D10 v2（2026-10-30 需求修订）：叠加勾选"不叠加基准线/参考曲线"→ 基准线
		//独立成层：生成时逐条判重（同数据基准线已在本组页面上=本次不生成），
		//收尾并层——把上一应用层（追加/叠加/替换皆可）落在本次页面上的基准线
		//成员迁出，与本次新建基准线并成一个独立基准线层。追加/叠加不勾选维持
		//基准线随曲线同层
		const bool datumLayerMode = (mode == ModeOverlay && opt.skipBaselines);

		for (int tp = 0; tp < scratchPageIds.size(); ++tp)
		{
			const int scratchPageId = scratchPageIds.at(tp);

			//目标页解析：追加恒新建；替换/叠加按位对位（越界/失效新建）
			int pageId = -1;
			const bool reusePage = (mode != ModeAppend) && tp < targetPageIds.size()
				&& targetPageIds.at(tp) > 0 && doc->node(targetPageIds.at(tp)) != nullptr
				&& doc->nodeType(targetPageIds.at(tp)) == CurveNodeType::Page;
			if (reusePage)
				pageId = targetPageIds.at(tp);
			else
				pageId = controller->addPage(scratch.nodeName(scratchPageId));
			r.pageIds << pageId;

			//页属性回放（叠加不回放环境：目标页属性不动）
			if (mode != ModeOverlay)
				replayProps(&scratch, scratchPageId, controller, pageId, pageProps());

			//目标页既有图表（替换/叠加按位对位；页新建时为空恒走新建）
			QList<int> targetCharts;
			if (mode != ModeAppend)
			{
				for (int cid : doc->childNodeIds(pageId))
				{
					if (doc->nodeType(cid) == CurveNodeType::Chart)
						targetCharts << cid;
				}
			}

			int tplChart = 0;
			for (int scratchChartId : scratch.childNodeIds(scratchPageId))
			{
				if (scratch.nodeType(scratchChartId) != CurveNodeType::Chart)
					continue;
				++tplChart;
				int chartId = -1;
				const bool reuseChart = (mode != ModeAppend) && (tplChart - 1) < targetCharts.size();
				if (reuseChart)
					chartId = targetCharts.at(tplChart - 1);
				else
					chartId = controller->addChart(pageId);
				mathCtx.chartByTpl.insert(chartKey(tp + 1, tplChart), chartId);

				//图表类型：叠加取目标图表实况（曲线建法跟随目标图表），
				//追加/替换取模板
				const int scratchChartType = scratch.nodeProperty(scratchChartId,
					CurveProp::kChartType).toInt();
				const int chartType = (mode == ModeOverlay)
					? doc->nodeProperty(chartId, CurveProp::kChartType).toInt()
					: scratchChartType;

				//替换：目标图表既有曲线/基准线/注释删除（骨架与轴/图例保留，
				//"把原有的数据清除，替换成新生成的曲线数据"）
				if (mode == ModeReplace)
				{
					const QList<int> stale = doc->childNodeIds(chartId);
					for (int cid : stale)
					{
						const CurveNodeType nt = doc->nodeType(cid);
						if (nt == CurveNodeType::Curve || nt == CurveNodeType::Baseline
							|| nt == CurveNodeType::Annotation)
							controller->removeNode(cid);
					}
				}

				if (mode != ModeOverlay)
				{
					//环境回放（追加=新图表；替换=骨架保留、环境重放）
					//kChartType 先行：复数图骨架协调先补 YAxis2，副轴对位才有可复用骨架
					controller->setNodeProperty(chartId, CurveProp::kChartType, scratchChartType);
					replayProps(&scratch, scratchChartId, controller, chartId, chartProps());
					replayProps(&scratch, scratchChartId, controller, chartId, headerFooterProps());

					//轴回放：主轴对位（骨架固定子）；副轴优先复用目标图表已有同侧
					//副轴（复数骨架自带首枚 YAxis2），不足再 addAxisNode 补齐
					QList<int> newX2, newY2;
					for (int cid : doc->childNodeIds(chartId))
					{
						if (doc->nodeType(cid) == CurveNodeType::XAxis2)
							newX2 << cid;
						else if (doc->nodeType(cid) == CurveNodeType::YAxis2)
							newY2 << cid;
					}
					int consumedX2 = 0, consumedY2 = 0;
					for (int scratchAxisId : scratch.childNodeIds(scratchChartId))
					{
						const CurveNodeType at = scratch.nodeType(scratchAxisId);
						const bool isXSide = (at == CurveNodeType::XAxis || at == CurveNodeType::XAxis2);
						const bool isYSide = (at == CurveNodeType::YAxis || at == CurveNodeType::YAxis2);
						if (!isXSide && !isYSide)
							continue;
						int axisId = -1;
						if (at == CurveNodeType::XAxis || at == CurveNodeType::YAxis)
						{
							for (int cid : doc->childNodeIds(chartId))
							{
								if (doc->nodeType(cid) == at)
								{
									axisId = cid;
									break;
								}
							}
						}
						else
						{
							QList<int>& pool = (at == CurveNodeType::XAxis2) ? newX2 : newY2;
							int& consumed = (at == CurveNodeType::XAxis2) ? consumedX2 : consumedY2;
							if (consumed < pool.size())
								axisId = pool.at(consumed++);
							else
								axisId = controller->addAxisNode(chartId, at == CurveNodeType::YAxis2);
						}
						if (axisId < 0)
							continue;

						//常规轴属性回放后处置范围三键：勾自动缩放=强制按数据适配
						//（忽略模板保存范围），不勾=回放模板范围
						replayProps(&scratch, scratchAxisId, controller, axisId, axisProps());
						const bool forceAuto = (isXSide && opt.autoScaleX) || (isYSide && opt.autoScaleY);
						if (forceAuto)
						{
							controller->setNodeProperty(axisId, CurveProp::kAutoScale, true);
						}
						else
						{
							for (const QString& p : { QString(CurveProp::kAutoScale), QString(CurveProp::kMin), QString(CurveProp::kMax) })
							{
								const QVariant v = scratch.nodeProperty(scratchAxisId, p);
								if (v.isValid())
									controller->setNodeProperty(axisId, p, v);
							}
						}
					}

					//图例
					for (int cid : scratch.childNodeIds(scratchChartId))
					{
						if (scratch.nodeType(cid) == CurveNodeType::Legend)
						{
							for (int nid : doc->childNodeIds(chartId))
							{
								if (doc->nodeType(nid) == CurveNodeType::Legend)
								{
									replayProps(&scratch, cid, controller, nid, legendProps());
									break;
								}
							}
							break;
						}
					}
				}

				//曲线逐条：数据先行（装配/直写/求值），失败即跳过不建节点。
				//不勾"使用报告颜色"时的轮换取色序按目标图表既有曲线数续起
				//（追加/替换目标图表新建或刚清空=0，叠加=既有曲线数）
				int tplCurve = 0;
				int colorIdx = 0;
				if (mode == ModeOverlay)
				{
					for (int cid : doc->childNodeIds(chartId))
					{
						if (doc->nodeType(cid) == CurveNodeType::Curve)
							++colorIdx;
					}
				}
				for (int scratchCurveId : scratch.childNodeIds(scratchChartId))
				{
					if (scratch.nodeType(scratchCurveId) != CurveNodeType::Curve)
						continue;
					++tplCurve;
					const CurveSeriesData& s = scratch.node(scratchCurveId)->series;
					const CurveSourceKind kind = curveSeriesSourceKind(s);
					const bool complexCand = chartType != 1 && isComplexCandidate(s);
					const bool isBar = (chartType == 1);

					QString skipReason;
					QVector<double> x, ya, yb;
					ChannelSelection effSel, effSelB, effSelX;   //effSelB/X 回退态 isValid()=false
					QString effCompB;
					if (kind == CurveSourceKind::File)
					{
						if (!assembleFileCurve(s, brokenFiles, opt, &effSel, &effSelB,
							&effSelX, &effCompB, &x, &ya, &yb, &skipReason))
							skipReason = QStringLiteral("通道装配失败：%1").arg(skipReason);
					}
					else if (kind == CurveSourceKind::Numeric)
					{
						x = s.xData;   //定义即数据，原样直写
						ya = s.yData;
					}
					else if (complexCand)
					{
						skipReason = QStringLiteral("复数数学曲线暂不支持模板应用");   //回放面收敛：v1 跳过
					}
					else if (!mathCtx.evaluate(s, &x, &ya, &skipReason))
					{
						skipReason = QStringLiteral("表达式求值失败：%1").arg(skipReason);
					}

					if (!skipReason.isEmpty())
					{
						++r.skippedCurves;
						const QString msg = QStringLiteral("[模板曲线] 曲线“%1”跳过：%2")
							.arg(scratch.nodeName(scratchCurveId), skipReason);
						r.warnings << msg;
						controller->reportWarning(msg);
						continue;
					}

					//建节点（柱状单壳/复数三壳/普通双壳）；叠加模式同名自动加后缀
					//（兼容保留，R23 起重名允许）
					const QString name = (mode == ModeOverlay)
						? uniqueCurveName(doc, chartId, scratch.nodeName(scratchCurveId))
						: scratch.nodeName(scratchCurveId);
					const int curveId = isBar ? controller->addBarCurve(chartId, name)
						: complexCand ? controller->addComplexCurve(chartId, name)
						: controller->addCurve(chartId, name);
					if (curveId < 0)
					{
						++r.skippedCurves;
						continue;
					}
					mathCtx.curveByTpl.insert(curveKey(tp + 1, tplChart, tplCurve), curveId);

					if (kind == CurveSourceKind::File)
					{
						//来源按实际装配通道落模（策略 0 精确命中=模板五元组原样回环，
						//策略 1/2=映射/匹配后的真实通道，"更改曲线"溯源可用）。
						//yp/x 侧带实际通道回写；b 侧回退态（仅令牌旧模板）传空参
						//=镜像 a 侧四元组（setComplexSeriesSource 缺省语义）；
						//x 侧回退态（横坐标形态）不写来源
						if (complexCand)
							controller->setComplexSeriesSource(curveId, effSel.filePath,
								mCurveChannelSelector::subcaseText(effSel.subcaseId),
								effSel.schema,
								mCurveChannelSelector::objectText(effSel.pointId),
								effSel.component,
								effCompB.isEmpty() ? s.complexSideComponent : effCompB,
								s.complexFormat,
								effSelB.isValid()
									? mCurveChannelSelector::subcaseText(effSelB.subcaseId)
									: QString(),
								effSelB.isValid() ? effSelB.schema : QString(),
								effSelB.isValid()
									? mCurveChannelSelector::objectText(effSelB.pointId)
									: QString());
						else
							controller->setSeriesSource(curveId, effSel.filePath,
								mCurveChannelSelector::subcaseText(effSel.subcaseId),
								effSel.schema,
								mCurveChannelSelector::objectText(effSel.pointId),
								effSel.component);
						if (effSelX.isValid())
						{
							controller->setCurveSideSource(curveId, true,
								mCurveChannelSelector::subcaseText(effSelX.subcaseId),
								effSelX.schema,
								mCurveChannelSelector::objectText(effSelX.pointId),
								effSelX.component);
						}
					}
					else
					{
						//数学/数值来源：标记串与四元组按模板原样回放（component 兜底
						//承载表达式/数值文本的 60 字符回显，语义同"定义曲线"创建）
						const QString tag = (kind == CurveSourceKind::Math)
							? kMathSourceTag : kNumericSourceTag;
						controller->setSeriesSource(curveId, tag, s.subcase, s.schema,
							s.object, s.component);
					}

					//数据直写（不进命令栈，宏 redo 不重放——与 pch 导入同源限制）
					if (complexCand && kind != CurveSourceKind::Numeric)
						controller->setComplexSeriesData(curveId, x, ya, yb);
					else
						controller->setSeriesData(curveId, x, ya);
					if (!s.xTickLabels.isEmpty())
						controller->setSeriesTickLabels(curveId, s.xTickLabels);

					//样式回放：配色按选项（模板色 or 六色轮换续起），其余一律模板回放
					if (opt.useReportColors)
						controller->setNodeProperty(curveId, CurveProp::kColor, s.color);
					else
					{
						const QVector<QColor> wheel = curveDefaultColorWheel();
						controller->setNodeProperty(curveId, CurveProp::kColor,
							wheel.at(colorIdx % wheel.size()));
					}
					++colorIdx;
					replayProps(&scratch, scratchCurveId, controller, curveId, curveProps());

					//x/y(/yp) 壳属性回放（缩放/偏移/表达式/坐标轴指派等；按类型对位）
					for (int shellId : scratch.childNodeIds(scratchCurveId))
					{
						const CurveNodeType st = scratch.nodeType(shellId);
						if (st != CurveNodeType::CurveXData && st != CurveNodeType::CurveYData
							&& st != CurveNodeType::CurveY2Data)
							continue;
						for (int nshellId : doc->childNodeIds(curveId))
						{
							if (doc->nodeType(nshellId) == st)
							{
								replayProps(&scratch, shellId, controller, nshellId, curveShellProps());
								break;
							}
						}
					}
					r.createdCurveIds << curveId;
					++r.createdCurves;
				}

				//基准线/注释回放（D10 v3：叠加勾选→逐条判重，同数据基准线已在
				//本组页面上=本次不生成，存量由收尾按数据归置进基准线层；未勾选=
				//逐条重放随曲线同层；注释受自动注释门控）
				for (int cid : scratch.childNodeIds(scratchChartId))
				{
					const CurveNodeType nt = scratch.nodeType(cid);
					if (nt == CurveNodeType::Baseline)
					{
						if (datumLayerMode)
						{
							const CurveBaselineData spec = scratch.node(cid)->baseline;
							if (mCurveLayerManager::getInstance()->findEquivalentBaseline(
									targetPageIds, spec) > 0)
								continue;
						}
						const int nid = controller->addBaseline(chartId);
						if (nid > 0)
						{
							replayProps(&scratch, cid, controller, nid, baselineProps());
							r.createdBaselineIds << nid;
						}
					}
					else if (nt == CurveNodeType::Annotation && opt.autoAnnotation)
					{
						const int nid = controller->addAnnotation(chartId);
						if (nid > 0)
							replayProps(&scratch, cid, controller, nid, annotationProps());
					}
				}
			}
		}
		controller->undoStack()->endMacro();

		//层登记（任务 5.1/D10 v3）：普通层名=模板名、成员=本次新建曲线/基准线；
		//叠加勾选独立成层时基准线不进曲线层，改归置登记——上一次应用普通层的
		//基准线成员（落在本次页面上）迁出，与本次新建基准线逐条按数据等价类
		//找层/建层（同数据并入/异数据新建，同组可多个基准线层；跨组命中并入
		//他组基准线层并升独立页面节点；无成员可归不建层）
		const QVector<int> pageVec = QVector<int>::fromList(r.pageIds);
		QList<int> layerBaselines = r.createdBaselineIds;
		if (datumLayerMode)
		{
			layerBaselines.clear();
			mCurveLayerManager::getInstance()->mergeDatumLayer(
				r.createdBaselineIds, pageVec);
		}
		r.registeredLayerId = mCurveLayerManager::getInstance()->registerLayer(
			meta.name, r.createdCurveIds, layerBaselines, pageVec);

		r.ok = true;
		return r;
	}
}
