#include "mStrengthReportAutoPreview.h"
#include "mStrengthReportConfig.h"
#include "mStrengthReportCollector.h"    //PA-05/PB-02：finishReserve 材料名集与发射面同源

#include <QAbstractButton>
#include <QBrush>
#include <QColor>
#include <QDate>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QLocale>
#include <QPair>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVector>
#include <QVector3D>

#include <cmath>
#include <vector>

#include "mViewportModelManager.h"       //getCVM（只读调用——Collector::currentPostDb 同源链）
#include "mViewportModel.h"
#include "mxr/view/REBUILD_PostRend.h"   //REBUILD_PostRend::getPostDataName（FR-301 主通道）
#include "mPostDataBase.h"               //批次 B（FR-321/322）：帧/工况枚举+出参只读重载宿主
#include "mPostDataManager.h"            //Physical_Post 管理器→getPostDatabase（只读）
#include "mPostGUIData.h"                //guidata->_cloudFrame（单元类型帧号同源）
#include "mModelData.h"                  //MechModelData::mModelData::getManager
#include "mPhysicalFieldEnum.h"          //Physical_Post/ElementType/NoneElement
#include "mElementFunction.h"            //elementMetaCount/getElementString（FR-311）
#include "mMxdbThread.h"                 //T3-1 只读读文件通道（出参重载；零写面实证 t31 8/8）

using namespace MechPreData;
using namespace PostData;
using namespace MxFunctions;
using namespace MIOFile;                 //批次 B：mMxdbThread（T3-1 只读读文件通道宿主）

namespace StrengthReport
{
	namespace
	{
		//六态局部子状态机取值（13 §2.1；与既有 LoadState 五态/probe 异步/_exporting 正交组合）
		enum Phase
		{
			PhaseIdle = 0,        //S0：空态（对话框打开/项目切换会话重置）
			PhaseAssembling,      //S1：自动装配中（只读通道逐项取值，不抓图不切视口）
			PhaseReady,           //S2：装配完成（一键导出使能）
			PhaseAutoCapture,     //S3：自动截图批量（三期批次 C 交付，本批占位不进入——Q3-3 默认关）
			PhaseAssemble,        //S4：数据集装配（既有 slot_export 链内，正交透传）
			PhaseEngine,          //S5：引擎导出中（阶段消息透传）
			PhaseDone             //S6：完成/失败汇总
		};

		//UI-01：全局 QSS（mApplicationStart.qss）将 QPushButton 宽度钉为 80px（width:80px），
		//QSS width 会钳制 sizeHint——长文案按钮（跳转按钮等）文本被截断。按字体实际宽度
		//抬高最小宽度下限（+左右内边距 24px），布局遵守显式 minimumWidth，完整文本可见
		void fitBtnMinWidth(QPushButton* btn)
		{
			if (btn == nullptr)
				return;
			const int textW = btn->fontMetrics().horizontalAdvance(btn->text());
			btn->setMinimumWidth(textW + 24);
		}

		//FR-01/WO-1：QAbstractButton 通用重载（01/02 所述"QWidget 重载"的落形态）——
		//QToolButton 被 qss:5 钉宽（width:@size.controlIn）且默认 (Fixed,Fixed) 策略
		//sizeHint 恒 37px，文字截断（"高级设置 ▸/▾"）。显式 setMinimumWidth 为布局
		//硬约束（qss 未写 maximumSize），实证必赢。QPushButton* 实参仍精确匹配上方
		//原重载（恒等匹配优先于派生→基转换），既有调用点行为零变化
		void fitBtnMinWidth(QAbstractButton* btn)
		{
			if (btn == nullptr)
				return;
			const int textW = btn->fontMetrics().horizontalAdvance(btn->text());
			btn->setMinimumWidth(textW + 24);
		}

		//只读单元格（预览表整表只读——消双真源：覆盖编辑仍在页签①④原控件，13 §2.1）
		QTableWidgetItem* readOnlyItem(const QString& text)
		{
			QTableWidgetItem* item = new QTableWidgetItem(text);
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			return item;
		}

		//FR-343 三态告警分级配色（黄字=需注意/红字=失败，复用既有预警色系——页签②E2.2 同款）
		const QColor kWarnColor(0xB8, 0x86, 0x0B);
		const QColor kFailColor(0xC0, 0x00, 0x00);
		const QColor kNullColor(0x80, 0x80, 0x80);

		//批次 B 通道面（只读：Collector::currentPostDb 同源链的文件内副本——Collector 匿名
		//空间实现不导出，既有类零修改纪律下的等价取链）
		QPointer<mPostDataBase> scanPostDb()
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr)
				return nullptr;
			const QString postname = postRend->getPostDataName();
			return getCVM->getModelData()
				->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		}

		//容器内最大有限值（T3-1 实测发现：真实文件 Stress(E) 可含 NaN/Inf——reardoorsum/
		//grave 样例实证；峰值统计必须过滤非有限值，全无效=false 由调用方按 null+告警出）
		bool maxFiniteValue(const QHash<int, double>& vals, double& out)
		{
			bool has = false;
			for (auto it = vals.constBegin(); it != vals.constEnd(); ++it)
			{
				const double v = it.value();
				if (!std::isfinite(v))
					continue;
				if (!has || v > out)
				{
					out = v;
					has = true;
				}
			}
			return has;
		}

		//数值→预览/记录文本（与页签②entryDisplayText 同格式 'g' 15）
		QString scanNumberText(double v)
		{
			return QLocale::c().toString(v, 'g', 15);
		}

		//QJsonValue 类型名（T3-4③ 错型告警定位文案用；Qt 5.13 无 QJsonValue::typeString）
		QString jsonTypeName(const QJsonValue& v)
		{
			switch (v.type())
			{
			case QJsonValue::Null: return QStringLiteral("null");
			case QJsonValue::Bool: return QStringLiteral("bool");
			case QJsonValue::Double: return QStringLiteral("number");
			case QJsonValue::String: return QStringLiteral("string");
			case QJsonValue::Array: return QStringLiteral("array");
			case QJsonValue::Object: return QStringLiteral("object");
			case QJsonValue::Undefined: break;
			}
			return QStringLiteral("undefined");
		}

		//缓存旁证双容器探测（E3-3/13 §2.4 第 3/4 条；T3-3①② 落字）
		//①同源旁证 gate：getResultVarNameByFrameID==复合格式"变量名 分量名"（单分量裸名
		//  特例同构）——该帧"当前装载变量"对照旁证，不作主校验（读文件路径来源=实参本身）
		//②回退门判据：getFrameResultType(frame)>=0 且结果容器非空（空 pair=={0,0} 与合法
		//  全零不可区分，禁作判据——T3-3②/B-5）
		//③主容器=getResultDataByFrameID（_resultDatas）。（备容器 getResultNodeDataByFrameID
		//  电机 partNode 值转存已废弃——全工作区零生产者，GUI 侧消费 2026-11-18 移除）
		bool cacheProbePeak(const QPointer<mPostDataBase>& postDB, int frameId,
			const QString& var, const QString& compo, double& out)
		{
			if (postDB == nullptr)
				return false;
			if (postDB->getResultVarNameByFrameID(frameId)
				!= AutoAssembler::compositeResultName(var, compo))
				return false;
			if (postDB->getFrameResultType(frameId) < 0)
				return false;
			const QHash<int, double> main = postDB->getResultDataByFrameID(frameId);
			if (!main.isEmpty())
				return maxFiniteValue(main, out);
			return std::isfinite(out);
		}

		//运行最大值并入 CaseScan（峰值帧号副产品：ties 保留首帧——FR-333 同源建议依据）
		void mergeFramePeak(AutoValue& target, int& peakFrameId, QString& peakFrameName,
			int frameId, const QString& frameName, double peak, bool warnTainted,
			const QString& warn)
		{
			const double prev = target.value.isDouble()
				? target.value.toDouble() : -std::numeric_limits<double>::infinity();
			if (!(peak > prev) && target.value.isDouble())
				return;   //非更优帧：仅值保留（ties 首帧优先）
			target.value = QJsonValue(peak);
			peakFrameId = frameId;
			peakFrameName = frameName;
			if (warnTainted && target.warn.isEmpty())
			{
				target.warn = warn;
				target.confidence = QStringLiteral("warn");
			}
			else if (!warnTainted)
			{
				//PA-11：更优帧来自读文件通道（非 warn 通道）——过时的缓存旁证注记撤回
				//并还原置信（出处标注与实际取值通道一致）；含首次命中自 initScanPlan
				//"fail"占位到"high"的翻转（无此翻转扫描成功行的工况行置信恒误显失败）
				target.warn.clear();
				target.confidence = QStringLiteral("high");
			}
		}
	}

	//——automation 节解析（T3-4③ 逐键错型守卫：错型条目剔除→该键 null+逐键定位告警，
	//不崩溃不静默失实；合法条目照常消费——R3-11/D3-05/D3-06 兜底前提）——
	AutomationConfig AutoAssembler::parseAutomation(const QJsonObject& automation)
	{
		AutomationConfig cfg;
		if (automation.isEmpty())
			return cfg;   //无节/空对象=E3-11（全回退缺项，一期行为；缺节不告噪；Qt 5.13 QJsonObject 无 isObject 成员——空对象判据等价）
		cfg.present = true;

		//default_sentences：必须为对象；条目必须为字符串（错型条目剔除+逐键定位）
		const QJsonValue sentencesVal = automation.value(QStringLiteral("default_sentences"));
		if (!sentencesVal.isUndefined() && !sentencesVal.isNull())
		{
			if (!sentencesVal.isObject())
			{
				cfg.invalidKeys.append(QStringLiteral("default_sentences"));
				cfg.typeWarns.append(QStringLiteral(
					"模板自动化配置有误：default_sentences 应为对象（实际 %1），该键按缺项策略留空")
					.arg(jsonTypeName(sentencesVal)));
			}
			else
			{
				const QJsonObject sentences = sentencesVal.toObject();
				for (auto it = sentences.begin(); it != sentences.end(); ++it)
				{
					//PB-01：空串/纯空白默认句剔除+逐键告警——空白串原样发射会被引擎
					//当作绑定值执行替换（静默清槽）且计入"采用默认句 N 处"披露，绕过
					//C7"绝不静默填错值"；与 VarSpec names 非空守卫（:236）同精神
					if (it.value().isString() && !it.value().toString().trimmed().isEmpty())
						cfg.defaultSentences.insert(it.key(), it.value().toString());
					else
					{
						cfg.invalidKeys.append(
							QStringLiteral("default_sentences.%1").arg(it.key()));
						cfg.typeWarns.append(QStringLiteral(
							"模板自动化配置有误：default_sentences.%1 应为非空字符串"
							"（实际 %2），该键按缺项策略留空").arg(it.key(),
							it.value().isString()
								? QStringLiteral("空白字符串") : jsonTypeName(it.value())));
					}
				}
			}
		}

		//stress_variable_map 条目解析器（BUG-303 双形态：13 §4.3 嵌套契约优先+顶层平铺
		//向后兼容——names=非空字符串数组 且 compo=字符串才合法）
		auto parseVarSpec = [&cfg](const QJsonObject& src, const QString& key,
			AutomationConfig::VarSpec& spec, bool* magnitude)
		{
			const QJsonValue v = src.value(key);
			if (v.isUndefined() || v.isNull())
				return false;   //未声明=该键 null（E3-2/E3-11 聚合告警在会话元信息步出）
			if (!v.isObject())
			{
				cfg.invalidKeys.append(key);
				cfg.typeWarns.append(QStringLiteral(
					"模板自动化配置有误：%1 应为对象（实际 %2），该键按缺项策略留空")
					.arg(key, jsonTypeName(v)));
				return false;
			}
			const QJsonObject obj = v.toObject();
			const QJsonValue namesVal = obj.value(QStringLiteral("names"));
			const QJsonValue compoVal = obj.value(QStringLiteral("compo"));
			if (namesVal.isArray() && compoVal.isString())
			{
				const QJsonArray names = namesVal.toArray();
				for (int i = 0; i < names.size(); ++i)
				{
					const QJsonValue n = names.at(i);
					if (n.isString() && !n.toString().isEmpty())
						spec.names.append(n.toString());
					else
					{
						//PA-10：数组内错型条目剔除不再静默——逐键定位告警
						//（T3-4③"逐键定位"补全；合法条目照常消费不误伤）
						cfg.invalidKeys.append(
							QStringLiteral("%1.names[%2]").arg(key).arg(i));
						cfg.typeWarns.append(QStringLiteral(
							"模板自动化配置有误：%1.names[%2] 应为非空字符串"
							"（实际 %3），该条目已剔除")
							.arg(key).arg(i).arg(jsonTypeName(n)));
					}
				}
				spec.compo = compoVal.toString();
				spec.valid = !spec.names.isEmpty() && !spec.compo.isEmpty();
			}
			if (!spec.valid)
			{
				cfg.invalidKeys.append(key);
				cfg.typeWarns.append(QStringLiteral(
					"模板自动化配置有误：%1.names 应为非空字符串数组且 %1.compo 应为字符串，"
					"该键按缺项策略留空").arg(key));
				return false;
			}
			if (magnitude != nullptr)
			{
				*magnitude = obj.value(QStringLiteral("metric")).toString()
					== QLatin1String("magnitude");
			}
			return true;
		};
		//BUG-303 形态双兼容：嵌套（stress_variable_map 包装，13 §4.3 文档契约）优先；
		//嵌套缺席=顶层平铺键（初稿兼容形态，13 §4.3 补注登记）；嵌套在位=平铺键忽略
		//（嵌套优先，文档=契约）。形态识别失败（嵌套/平铺键在位而均非法）→varMapShapeFailure=
		//预览 ⚠ 行+导出汇总披露（不留静默——静默降级治理）
		const QJsonValue nestedVal = automation.value(QStringLiteral("stress_variable_map"));
		const bool nestedPresent = !nestedVal.isUndefined() && !nestedVal.isNull();
		const bool flatPresent = !automation.value(QStringLiteral("stress_peak")).isUndefined()
			|| !automation.value(QStringLiteral("displacement")).isUndefined();
		if (nestedPresent && nestedVal.isObject())
		{
			//嵌套契约形态（13 §4.3）
			const QJsonObject nested = nestedVal.toObject();
			parseVarSpec(nested, QStringLiteral("stress_peak"), cfg.stressPeak, nullptr);
			parseVarSpec(nested, QStringLiteral("displacement"), cfg.displacement,
				&cfg.displacementMagnitude);
		}
		else
		{
			//平铺兼容形态（初稿；嵌套在位但非对象=错型告警+平铺回退——降级不静默）
			if (nestedPresent)
			{
				cfg.invalidKeys.append(QStringLiteral("stress_variable_map"));
				cfg.typeWarns.append(QStringLiteral(
					"自动取值配置非法：stress_variable_map 应为对象（实际 %1），"
					"已按兼容格式解析").arg(jsonTypeName(nestedVal)));
			}
			parseVarSpec(automation, QStringLiteral("stress_peak"), cfg.stressPeak, nullptr);
			parseVarSpec(automation, QStringLiteral("displacement"), cfg.displacement,
				&cfg.displacementMagnitude);
		}
		//形态识别失败判定：automation 在位而嵌套/平铺键至少一处声明、但两 spec 均未解析合法
		//→既非嵌套也非平铺合法形态（预览 ⚠+导出披露，不留静默）
		cfg.varMapShapeFailure = cfg.present && (nestedPresent || flatPresent)
			&& !cfg.stressPeak.valid && !cfg.displacement.valid;

		//buckling_case_map：对象；条目=正整数（错型剔除+逐键定位）
		const QJsonValue buckVal = automation.value(QStringLiteral("buckling_case_map"));
		if (!buckVal.isUndefined() && !buckVal.isNull())
		{
			if (!buckVal.isObject())
			{
				cfg.invalidKeys.append(QStringLiteral("buckling_case_map"));
				cfg.typeWarns.append(QStringLiteral(
					"模板自动化配置有误：buckling_case_map 应为对象（实际 %1），该键按缺项策略留空")
					.arg(jsonTypeName(buckVal)));
			}
			else
			{
				const QJsonObject buck = buckVal.toObject();
				for (auto it = buck.begin(); it != buck.end(); ++it)
				{
					if (it.value().isDouble() && it.value().toDouble() > 0.0
						&& it.value().toDouble() == static_cast<double>(it.value().toInt()))
						cfg.bucklingMap.insert(it.key(), it.value().toInt());
					else
					{
						cfg.invalidKeys.append(
							QStringLiteral("buckling_case_map.%1").arg(it.key()));
						cfg.typeWarns.append(QStringLiteral(
							"模板自动化配置有误：buckling_case_map.%1 应为正整数（实际 %2），"
							"该条目按缺项策略留空").arg(it.key(), jsonTypeName(it.value())));
					}
				}
			}
		}

		//material_library：数组；条目=对象且 name=非空字符串+limit_stress=正数
		//（Q3-6=规则层材料极限配置表；12 §2.3"枚举不进首版代码路径"——错型条目剔除）
		const QJsonValue matVal = automation.value(QStringLiteral("material_library"));
		if (!matVal.isUndefined() && !matVal.isNull())
		{
			if (!matVal.isArray())
			{
				cfg.invalidKeys.append(QStringLiteral("material_library"));
				cfg.typeWarns.append(QStringLiteral(
					"模板自动化配置有误：material_library 应为数组（实际 %1），该键按缺项策略留空")
					.arg(jsonTypeName(matVal)));
			}
			else
			{
				const QJsonArray mats = matVal.toArray();
				for (int i = 0; i < mats.size(); ++i)
				{
					const QJsonObject entry = mats.at(i).toObject();
					const QString name = entry.value(QStringLiteral("name")).toString();
					const QJsonValue limit = entry.value(QStringLiteral("limit_stress"));
					if (name.isEmpty() || !limit.isDouble() || limit.toDouble() <= 0.0)
					{
						cfg.invalidKeys.append(QStringLiteral("material_library[%1]").arg(i));
						cfg.typeWarns.append(QStringLiteral(
							"模板自动化配置有误：material_library[%1] 应含 name（非空字符串）+"
							"limit_stress（正数），该条目按缺项策略留空").arg(i));
						continue;
					}
					//PB-03：同名多极限非静默——QMap::insert 覆盖语义（末条胜出）加告警定位，
					//"同名不同极限=末条静默覆盖"破坏出处可追溯性（宁提示不静默）
					if (cfg.materialLimits.contains(name))
					{
						cfg.typeWarns.append(QStringLiteral(
							"模板自动化配置告警：material_library 同名条目重复（%1）——"
							"已取末条极限值 %2（前值 %3 被替换）")
							.arg(name).arg(limit.toDouble())
							.arg(cfg.materialLimits.value(name)));
					}
					cfg.materialLimits.insert(name, limit.toDouble());
				}
			}
		}
		return cfg;
	}

	//同源注册名复合格式（T3-3①/B-4：mPostDataBase.cpp:1939-1955 resultName 拼接语义镜像）
	QString AutoAssembler::compositeResultName(const QString& var, const QString& compo)
	{
		if (!var.isEmpty() && !compo.isEmpty()
			&& !compo.contains(var, Qt::CaseInsensitive))
			return var + QStringLiteral(" ") + compo;
		return var;
	}

	//单元类型自动采集（FR-311/E3-1；framei 取法=collectModelStats 同源，13 §3 批次 B）
	AutoValue AutoAssembler::collectElementType(const mStrengthReportProjectConfig* cfg)
	{
		AutoValue v;
		const QString entered = cfg != nullptr ? cfg->elementType : QString();
		if (!entered.isEmpty())
		{
			v.value = QJsonValue(entered);
			v.source = QStringLiteral("录入");
			v.confidence = QStringLiteral("high");
			return v;   //录入＞自动（E3-7）
		}
		QPointer<mPostDataBase> postDB = scanPostDb();
		if (postDB == nullptr)
		{
			v.confidence = QStringLiteral("fail");
			v.warn = QStringLiteral("单元类型不可得（无激活后处理视口或后处理数据，按缺项策略留空）");
			return v;
		}
		//帧号取法同 collectModelStats（guidata->_cloudFrame fallback 1——单元计数与帧无关；
		//与节点/单元总数同一帧模型数据=13 §3"单元类型计数同源"）
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		int framei = 1;
		if (postRend != nullptr)
		{
			if (mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend))
				framei = guidata->_cloudFrame;
		}
		if (framei <= 0)
			framei = 1;
		mPostModelData* modeldata = postDB->getModelData(framei);
		if (modeldata == nullptr)
		{
			v.confidence = QStringLiteral("fail");
			v.warn = QStringLiteral("单元类型不可得（帧模型数据未就绪，按缺项策略留空）");
			return v;
		}
		//逐型计数（0..elementMetaCount() 整型遍历；NoneElement 跳过；并列取枚举序首个）
		ElementType best = NoneElement;
		long long bestCount = 0;
		const int metaCount = elementMetaCount();
		for (int t = 0; t < metaCount; ++t)
		{
			const ElementType type = static_cast<ElementType>(t);
			if (type == NoneElement)
				continue;
			const long long count = modeldata->getNumByEleType(type);
			if (count > bestCount)
			{
				bestCount = count;
				best = type;
			}
		}
		if (best == NoneElement || bestCount <= 0)
		{
			//E3-1：全零计数=聚合告警+null（绝不猜型）
			v.confidence = QStringLiteral("fail");
			v.warn = QStringLiteral("单元类型不可得（全部单元类型计数为零，按缺项策略留空）");
			return v;
		}
		v.value = QJsonValue(mElementFunction::getElementString(best));
		v.source = QStringLiteral("自动·单元计数");
		v.confidence = QStringLiteral("high");
		return v;
	}

	//剩余系数纯计算核（FR-324/E3-4：除零/任一缺失/非有限值=null+告警，绝不静默计算）
	AutoValue AutoAssembler::computeReserveFactor(double limit, double peak)
	{
		AutoValue v;
		if (!(limit > 0.0) || !std::isfinite(limit))
		{
			v.confidence = QStringLiteral("warn");
			v.warn = QStringLiteral("材料极限不可得（材料库配置缺失或非法），剩余系数按缺项策略留空");
			return v;
		}
		if (!(peak > 0.0) || !std::isfinite(peak))
		{
			v.confidence = QStringLiteral("warn");
			v.warn = QStringLiteral("应力峰值不可得（扫描无值），剩余系数按缺项策略留空");
			return v;
		}
		const double rf = limit / peak;
		if (!std::isfinite(rf))
		{
			v.confidence = QStringLiteral("warn");
			v.warn = QStringLiteral("剩余系数计算结果非法（除零/溢出），按缺项策略留空");
			return v;
		}
		v.value = QJsonValue(rf);
		v.source = QStringLiteral("自动·自动计算");
		v.confidence = QStringLiteral("high");
		return v;
	}

	//——分步装配会话（批次 B：读文件通道逐帧 IO——QTimer 逐帧分步，R3-5/O3-6/O3-7）——
	AutoAssemblySession::AutoAssemblySession(const AssemblyInput& input)
		: _input(input)
		, _automation(AutoAssembler::parseAutomation(
			input.automation != nullptr && !input.automation->isEmpty()
			? *input.automation : QJsonObject()))
	{
		initMeta();
		initScanPlan();
		_total = _done + _tasks.size() + 1;   //元信息步+扫描步+收尾步
	}

	void AutoAssemblySession::initMeta()
	{
		//元信息 5 键（批次 A 通道族零改动复用——全为轻通道内存查询，单步内完成无 IO）
		const mStrengthReportProjectConfig* cfg = _input.config;
		const mStrengthReportProjectConfig emptyCfg;

		//通道①（FR-301）：计算文件名（Collector::currentPostDb 同源只读链）
		QString postName;
		if (REBUILD_PostRend* postRend = getCVM->getPostRenderWidget())
			postName = postRend->getPostDataName();
		const QString nameSansExt = QFileInfo(postName).completeBaseName();
		const bool needsSanitize = !postName.isEmpty()
			&& postName != mStrengthReportProjectConfig::sanitizeProjectId(postName);
		const QString sanitizeWarn = QStringLiteral(
			"名称含文件名非法字符——界面显示已净化，报告内使用原始文件名");

		//对象名（manual_fields.project_subject）：录入＞自动（E3-7）
		AutoValue subject;
		const QString enteredSubject = cfg != nullptr ? cfg->projectSubject : emptyCfg.projectSubject;
		if (!enteredSubject.isEmpty())
		{
			subject.value = QJsonValue(enteredSubject);
			subject.source = QStringLiteral("录入");
			subject.confidence = QStringLiteral("high");
		}
		else if (!nameSansExt.isEmpty())
		{
			subject.value = QJsonValue(nameSansExt);
			subject.source = QStringLiteral("自动·计算文件");
			subject.confidence = needsSanitize ? QStringLiteral("warn") : QStringLiteral("high");
			subject.warn = needsSanitize ? sanitizeWarn : QString();
		}
		else
		{
			subject.confidence = QStringLiteral("fail");
			subject.warn = QStringLiteral("对象名不可得（无激活后处理视口或计算文件名，按缺项策略留空）");
		}
		_result.items.insert(QStringLiteral("manual_fields.project_subject"), subject);
		_result.items.insert(QStringLiteral("meta.title"), subject);

		//计算文件（meta.calc_file）：同 #1 通道原样（不去扩展名）
		AutoValue calcFile;
		if (!postName.isEmpty())
		{
			calcFile.value = QJsonValue(postName);
			calcFile.source = QStringLiteral("自动·计算文件");
			calcFile.confidence = needsSanitize ? QStringLiteral("warn") : QStringLiteral("high");
			calcFile.warn = needsSanitize ? sanitizeWarn : QString();
		}
		else
		{
			calcFile.confidence = QStringLiteral("fail");
			calcFile.warn = QStringLiteral("计算文件名不可得（无激活后处理视口，按缺项策略留空）");
		}
		_result.items.insert(QStringLiteral("meta.calc_file"), calcFile);

		//编写（meta.author，FR-302/Q3-4）：系统登录名（空环境变量=null 不发空串，CB-08）
		AutoValue author;
		const QString userName = QString::fromLocal8Bit(qgetenv("USERNAME")).trimmed();
		if (!userName.isEmpty())
		{
			author.value = QJsonValue(userName);
			author.source = QStringLiteral("自动·登录名");
			author.confidence = QStringLiteral("high");
		}
		else
		{
			author.confidence = QStringLiteral("fail");
			author.warn = QStringLiteral("系统登录名不可得（环境变量 USERNAME 为空，按缺项策略留空）");
		}
		_result.items.insert(QStringLiteral("meta.author"), author);

		//日期（meta.date）：当天（与 buildDataset 既有发射同源同格式 yyyy-MM-dd）
		AutoValue date;
		date.value = QJsonValue(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
		date.source = QStringLiteral("自动·自动计算");
		date.confidence = QStringLiteral("high");
		_result.items.insert(QStringLiteral("meta.date"), date);

		//软件名（model_stats.software_name）：既有单键默认常量链镜像（D-24①）
		AutoValue software;
		const QString enteredSoftware = cfg != nullptr ? cfg->softwareName : emptyCfg.softwareName;
		software.value = QJsonValue(enteredSoftware.isEmpty()
			? QStringLiteral("迈曦CAE") : enteredSoftware);
		software.source = enteredSoftware.isEmpty()
			? QStringLiteral("自动·自动计算") : QStringLiteral("录入");
		software.confidence = QStringLiteral("high");
		_result.items.insert(QStringLiteral("model_stats.software_name"), software);

		//单元类型（FR-311，批次 B 新通道；录入＞自动在函数内合并）
		_result.items.insert(QStringLiteral("manual_fields.element_type"),
			AutoAssembler::collectElementType(cfg));

		//默认句四键+cases.load_type（FR-312/313；Q3-2 约束描述宁缺勿错——无配置即 null
		//绝不生成；E3-7 录入＞默认句；未命中=现状零变化）
		struct SentenceKey { const char* dsKey; const char* itemKey; int manualField; };
		enum ManualField { MfNone = 0, MfConstraint, MfLoadCase, MfLoadType, MfCalcMethod };
		static const QVector<SentenceKey> sentenceKeys
		{
			{ "manual_fields.constraint_desc", "manual_fields.constraint_desc", MfConstraint },
			{ "manual_fields.load_case_desc",  "manual_fields.load_case_desc",  MfLoadCase },
			{ "manual_fields.load_type",       "manual_fields.load_type",       MfLoadType },
			{ "manual_fields.calc_method",     "manual_fields.calc_method",     MfCalcMethod },
			{ "cases.load_type",               "cases.load_type",               MfNone },
		};
		for (const SentenceKey& sk : sentenceKeys)
		{
			const QString dsKey = QString::fromLatin1(sk.dsKey);
			const QString itemKey = QString::fromLatin1(sk.itemKey);
			QString entered;
			if (cfg != nullptr)
			{
				switch (sk.manualField)
				{
				case MfConstraint: entered = cfg->constraintDesc; break;
				case MfLoadCase: entered = cfg->loadCaseDesc; break;
				case MfLoadType: entered = cfg->loadType; break;
				case MfCalcMethod: entered = cfg->calcMethod; break;
				default: break;
				}
			}
			AutoValue item;
			if (!entered.isEmpty())
			{
				item.value = QJsonValue(entered);
				item.source = QStringLiteral("录入");
				item.confidence = QStringLiteral("high");
			}
			else if (_automation.defaultSentences.contains(dsKey))
			{
				item.value = QJsonValue(_automation.defaultSentences.value(dsKey));
				item.source = QStringLiteral("规则层默认句");
				item.confidence = QStringLiteral("high");
				++_result.defaultSentenceCount;
			}
			else
			{
				//无默认句配置=null 走缺项（E3-11/Q3-2 宁缺勿错）；预览中性呈现不告噪
				item.confidence = QStringLiteral("info");
			}
			_result.items.insert(itemKey, item);
		}

		//规则层未配置标注（E3-11 预览呈现面：无 automation 节=无默认句）。
		//PA-14：区分"规则模板未加载"（视口事件先于容器加载/校验失败的瞬态）与
		//"已加载但无 automation 节"两态归因——前者中性提示，防用户据"规则层未配置"
		//误判模板缺节（装配随后由 probe 完成链重跑纠正）
		const bool rulesLoaded = _input.rulesDocument != nullptr
			&& _input.rulesDocument->isObject();
		if (!_automation.present)
		{
			AutoValue missing;
			missing.value = QJsonValue(rulesLoaded
				? QStringLiteral("模板未配置自动化内容（无默认句/对应关系/材料库——按缺项策略导出）")
				: QStringLiteral("规则模板未加载——加载规则模板后自动读取自动化配置（相关键当前按缺项策略导出）"));
			missing.source = QStringLiteral("规则层未配置");
			missing.confidence = QStringLiteral("info");
			_result.items.insert(QStringLiteral("__automation__"), missing);
			_result.warns.append(rulesLoaded ? QStringLiteral(
				"模板未配置自动化内容——默认句/变量对应/失稳对应/材料库不可用，"
				"相关键按缺项策略导出（兼容旧版容器）") : QStringLiteral(
				"规则模板未加载——加载规则模板后自动读取自动化配置"
				"（默认句/变量对应/失稳对应/材料库相关键当前按缺项策略导出）"));
		}
		//错型守卫告警聚合（T3-4③/R3-11：逐键定位，不静默）
		_result.warns.append(_automation.typeWarns);

		//BUG-303 静默降级治理：形态识别失败（嵌套/平铺键在位而均非法）→预览 ⚠ 行
		//（__automation_shape__ 项）+聚合告警+导出汇总披露（Dlg 披露段），不留静默
		if (_automation.varMapShapeFailure)
		{
			const QString shapeWarn = QStringLiteral(
				"模板自动化配置无法识别：stress_variable_map 格式非法"
				"——应力峰值/位移最大值按缺项策略留空（请核对规则模板的自动化配置）");
			AutoValue shape;
			shape.source = QStringLiteral("规则层未配置");
			shape.confidence = QStringLiteral("warn");
			shape.warn = shapeWarn;
			_result.items.insert(QStringLiteral("__automation_shape__"), shape);
			_result.warns.append(shapeWarn);
		}

		//扫描段映射缺失聚合告警（E3-2：映射未配置=该键 null+告警，绝不猜源）
		if (_automation.present && !_automation.stressPeak.valid)
			_result.warns.append(QStringLiteral(
				"模板自动化配置缺少有效的 stress_variable_map.stress_peak（names/compo）——"
				"应力峰值按缺项策略留空"));
		if (_automation.present && !_automation.displacement.valid)
			_result.warns.append(QStringLiteral(
				"模板自动化配置缺少有效的 stress_variable_map.displacement（names/compo）——"
				"位移最大值按缺项策略留空"));
		if (_automation.present && _automation.bucklingMap.isEmpty())
			_result.warns.append(QStringLiteral(
				"模板自动化配置缺少 buckling_case_map——失稳模态阶次按缺项策略留空"));
		if (_automation.present && _automation.materialLimits.isEmpty())
			_result.warns.append(QStringLiteral(
				"模板自动化配置缺少 material_library——剩余系数按缺项策略留空"));

		++_done;   //元信息步完成
	}

	void AutoAssemblySession::initScanPlan()
	{
		//扫描计划（13 §3 scanCaseResults：逐工况（getAllCaseNames）→逐帧
		//（getCaseFrameNamesAndIDs）；仅映射涉及变量读=T3-1③/B-8 降级口径）
		QPointer<mPostDataBase> postDB = scanPostDb();
		QStringList caseNames;
		QString resultFile;
		if (postDB != nullptr)
		{
			caseNames = postDB->getAllCaseNames();
			resultFile = postDB->getFile();   //slot_showCloud :4673 同源（文件路径=读文件通道实参）
		}
		const bool hasStressTask = _automation.stressPeak.valid;
		const bool hasDispTask = _automation.displacement.valid;
		if (postDB == nullptr)
		{
			//无 postDB：E3 退化（扫描通道全 null）——预览逐键 fail 标注已含
			_result.warns.append(QStringLiteral(
				"结果扫描不可用（无激活后处理视口或后处理数据）——工况结果按缺项策略留空"));
			_done += 0;
			return;
		}
		for (const QString& caseName : caseNames)
		{
			AssemblyResult::CaseScan scan;
			scan.caseName = caseName;
			scan.stressPeak.confidence = QStringLiteral("fail");
			scan.dispMax.confidence = QStringLiteral("fail");
			scan.buckling.confidence = QStringLiteral("fail");

			//失稳模态阶次（FR-323）：工况→失稳映射命中=指定阶次帧短名
			//（getCaseFrameShortNames"只含阶数"通道）；无映射=null（映射缺失已在元信息步聚合）
			if (_automation.bucklingMap.contains(caseName))
			{
				const int order = _automation.bucklingMap.value(caseName);
				const QStringList shortNames = postDB->getCaseFrameShortNames(caseName);
				if (order >= 1 && order <= shortNames.size())
				{
					scan.buckling.value = QJsonValue(shortNames.at(order - 1));
					scan.buckling.source = QStringLiteral("自动·结果扫描");
					scan.buckling.confidence = QStringLiteral("high");
				}
				else
				{
					scan.buckling.warn = QStringLiteral(
						"失稳阶次 %1 超出工况帧范围（帧短名数 %2），按缺项策略留空")
						.arg(order).arg(shortNames.size());
					scan.buckling.confidence = QStringLiteral("warn");
				}
			}
			_result.cases.append(scan);

			if (hasStressTask || hasDispTask)
			{
				std::vector<QPair<QString, int>> frames;
				postDB->getCaseFrameNamesAndIDs(caseName, frames);
				for (const QPair<QString, int>& fr : frames)
				{
					FrameTask task;
					task.caseIndex = _result.cases.size() - 1;
					task.caseName = caseName;
					task.frameId = fr.second;
					task.frameName = fr.first;
					_tasks.append(task);
				}
			}
		}
		//E3-3/B-7 登记清单（T3-1②③ 确认结论：只读出参通道=NewSingleMxdbReader 201/202
		//专链；native 五格式与 mxdb v105/v106（电机产品线）结构不可达——缓存旁证兜底；
		//登记清单仅收长尾、不吞主格式（mxdb v201/202 已实调实证 t31 8/8 PASS））
		if ((hasStressTask || hasDispTask) && !resultFile.isEmpty()
			&& !resultFile.endsWith(QStringLiteral(".mxdb"), Qt::CaseInsensitive))
		{
			_result.warns.append(QStringLiteral(
				"该结果文件格式不支持直接读取（OP2/RST/d3plot/H3D/ODB/VTK 及部分旧版 "
				"mxdb），文件 %1 已改用当前已加载的结果数据取值").arg(QFileInfo(resultFile).fileName()));
		}
	}

	void AutoAssemblySession::stepScanFrame()
	{
		if (_taskIdx >= _tasks.size())
			return;
		const FrameTask task = _tasks.at(_taskIdx++);
		++_done;
		AssemblyResult::CaseScan& scan = _result.cases[task.caseIndex];
		QPointer<mPostDataBase> postDB = scanPostDb();
		if (postDB == nullptr)
			return;   //扫描中途视口丢失：该帧静默跳过（收尾按无值出 null+告警）
		const QString resultFile = postDB->getFile();
		//T3-1 只读出参重载宿主（构造含 openFileStatus=文件句柄惰性打开；不触发分发链）。
		//A 观察项7 锁死：reader=QThread 子类栈构造，禁 start()——出参只读重载零写面，
		//finished→slot_readVarValue 分发连接存在但永不触发（t31 运行时快照+静态 grep 双证）
		mMxdbThread reader(resultFile, postDB);
		const bool readChannelReachable = !resultFile.isEmpty()
			&& resultFile.endsWith(QStringLiteral(".mxdb"), Qt::CaseInsensitive);
		const bool frameHasResult = postDB->getFrameResultType(task.frameId) >= 0;
		//PA-08：分量成员性预检（读文件通道前置闸）——错配 compo 名直入 reader 时，
		//calcuComponeValues 家族（分量数∉{0,14}）对 indexOf 失败=-1 越界读，读出的内存
		//垃圾会被 maxFiniteValue 采纳为有限值静默入报告（C7"绝不静默填错值"击穿）。
		//分量表=postDB 注册面 getVariableCompos（与导入期同源）；表空=变量未注册，
		//读通道自会失败走既有 null+告警路径（不误伤）；位移链三分量表+未命中=Q3-5
		//模长语义放行（reader 对 componeNums==3 有安全模长分支）
		auto compoReadAllowed = [&postDB](const QString& varName, const QString& compo,
			bool magnitudeAllowed) -> bool
		{
			const QStringList regCompos = postDB->getVariableCompos(varName);
			if (regCompos.isEmpty())
				return true;   //变量未注册/无分量表：既有失败语义兜底，不在此误伤
			if (regCompos.contains(compo, Qt::CaseInsensitive))
				return true;
			return magnitudeAllowed && regCompos.size() == 3;
		};

		//应力峰值（FR-321）：主通道=T3-1 只读读文件通道（来源=实参本身，无猜测面）；
		//失败回退=缓存旁证双容器探测（T3-3①复合名比对+T3-3②回退门判据）
		if (_automation.stressPeak.valid)
		{
			for (const QString& varName : _automation.stressPeak.names)
			{
				//PA-08：应力链无模长语义——错配 compo 不入读文件通道，判读失败走告警
				//（模长回退仅位移链 Q3-5 冻结口径）
				if (!compoReadAllowed(varName, _automation.stressPeak.compo, false))
				{
					if (scan.stressPeak.warn.isEmpty())
					{
						scan.stressPeak.warn = QStringLiteral(
							"应力分量名 \"%1\" 不在变量 %2 的可用分量列表内，"
							"按缺项策略留空，请核对自动取值配置")
							.arg(_automation.stressPeak.compo, varName);
						scan.stressPeak.confidence = QStringLiteral("fail");
					}
					continue;
				}
				const QStringList compos{ _automation.stressPeak.compo };
				QHash<QString, QHash<int, double>> allVals;
				int flag = -1;
				double peak = 0.0;
				//T3-1 只读出参重载（mMxdbThread.h:70；禁用分发变体与 mIMxdbFile1 读链）
				if (readChannelReachable
					&& reader.readVarDatas(resultFile, task.frameId, varName, compos,
						QString(), QString(), QVector<double>(), allVals, flag)
					&& maxFiniteValue(allVals.value(_automation.stressPeak.compo), peak))
				{
					mergeFramePeak(scan.stressPeak, scan.stressPeakFrameId,
						scan.stressPeakFrameName,
						task.frameId, task.frameName, peak, false, QString());
					break;
				}
				//缓存旁证回退（自算+双容器探测；E3-3——warn 标注缓存口径来源）
				if (cacheProbePeak(postDB, task.frameId, varName,
					_automation.stressPeak.compo, peak))
				{
					mergeFramePeak(scan.stressPeak, scan.stressPeakFrameId,
						scan.stressPeakFrameName,
						task.frameId, task.frameName, peak, true,
						QStringLiteral("未能直接读取结果文件（格式或变量名不匹配），改用当前已加载的结果数据取值"
							"（两者比对一致）"));
					break;
				}
			}
			if (scan.stressPeak.value.isNull() && frameHasResult
				&& scan.stressPeak.warn.isEmpty())
			{
				//变量在帧不命中：逐工况聚合一次候选清单告警（E3-2——绝不猜源）
				scan.stressPeak.warn = QStringLiteral(
					"应力峰值取值失败（候选变量 %1 在本工况帧不命中或读取失败）")
					.arg(_automation.stressPeak.names.join(QStringLiteral("/")));
				scan.stressPeak.confidence = QStringLiteral("fail");
			}
		}

		//位移最大值（FR-322/T3-5 格式分治）：同通道；compo 不在三分量变量分量表内时读文件
		//通道返回模长（Q3-5 冻结=模长口径）；缓存回退=位移容器模长 max（native 导入期逐帧
		//填充可达；mxdb 第 1 帧导入期有值——懒填充帧由读文件通道承载）
		if (_automation.displacement.valid)
		{
			for (const QString& varName : _automation.displacement.names)
			{
				//PA-08：位移链三分量族放行模长语义（Q3-5）；其余错配 compo 拒入读通道
				if (!compoReadAllowed(varName, _automation.displacement.compo, true))
				{
					if (scan.dispMax.warn.isEmpty())
					{
						scan.dispMax.warn = QStringLiteral(
							"位移分量名 \"%1\" 不在变量 %2 的可用分量列表内——"
							"按缺项策略留空，请核对自动取值配置").arg(_automation.displacement.compo, varName);
						scan.dispMax.confidence = QStringLiteral("fail");
					}
					continue;
				}
				const QStringList compos{ _automation.displacement.compo };
				QHash<QString, QHash<int, double>> allVals;
				int flag = -1;
				double peak = 0.0;
				if (readChannelReachable
					&& reader.readVarDatas(resultFile, task.frameId, varName, compos,
						QString(), QString(), QVector<double>(), allVals, flag)
					&& maxFiniteValue(allVals.value(_automation.displacement.compo), peak))
				{
					mergeFramePeak(scan.dispMax, scan.dispMaxFrameId,
						scan.dispMaxFrameName,
						task.frameId, task.frameName, peak, false, QString());
					break;
				}
				//缓存回退：位移容器（T3-5——native 导入期逐帧填充路径）
				const QHash<int, QVector3D> disp =
					postDB->getDisplacementDataByFrameID(task.frameId);
				if (!disp.isEmpty())
				{
					double mag = 0.0;
					bool has = false;
					for (auto it = disp.constBegin(); it != disp.constEnd(); ++it)
					{
						const double len = it.value().length();
						if (std::isfinite(len) && (!has || len > mag))
						{
							mag = len;
							has = true;
						}
					}
					if (has)
					{
						mergeFramePeak(scan.dispMax, scan.dispMaxFrameId,
							scan.dispMaxFrameName,
							task.frameId, task.frameName, mag, true,
							QStringLiteral("未能直接读取结果文件，取值=当前已加载位移数据的模长最大值"));
						break;
					}
				}
			}
			if (scan.dispMax.value.isNull() && frameHasResult
				&& scan.dispMax.warn.isEmpty())
			{
				scan.dispMax.warn = QStringLiteral(
					"位移最大值取值失败（候选变量 %1 在本工况帧不命中或读取失败）")
					.arg(_automation.displacement.names.join(QStringLiteral("/")));
				scan.dispMax.confidence = QStringLiteral("fail");
			}
		}
	}

	void AutoAssemblySession::finishReserve()
	{
		//剩余系数自动计算（FR-324/Q3-6=规则层 material_library 配置表）：分子=材料极限
		//（按 collectMaterialNames 名匹配——与 buildDataset 材料行集同源）；分母=关联应力
		//峰值（默认=全工况扫描最大值=安全保守侧，12 §2.3）；除零/缺边=null+告警；
		//verdict 链零改动（buildDataset 材料级 min 自动衔接 FR-326）
		//PA-05/PB-02：材料名集与发射面同源（Collector::collectMaterialNames 三级链：
		//①postDB 材料映射→②模型侧兜底→③双空降级）——原实现仅取①级键集，材料名走
		//②级兜底通道时（09 §0.1 登记降级路径，chouti 实测形态）materialReserves 为空：
		//materials[].reserve_factor×M 键漏计 failCount、verdict 桶在 Tab0 误报失败而
		//发射集合正常（计数与发射集合分裂）。此处与 buildDataset/页签②/verdict 推导
		//共用同一实现（D3-2 定序在 Collector 内已做，QMap 插入序无关发射）
		QString matChainErr;
		const QStringList materialNames =
			mStrengthReportCollector::collectMaterialNames(matChainErr).names;
		if (!matChainErr.isEmpty())
		{
			_result.warns.append(QStringLiteral(
				"材料数据不可得（结果文件无材料信息且模型侧无材料清单）——"
				"材料剩余系数按缺项策略留空"));
		}
		//全工况扫描最大应力峰值（保守侧分母）
		double globalPeak = 0.0;
		bool hasPeak = false;
		for (const AssemblyResult::CaseScan& scan : _result.cases)
		{
			if (scan.stressPeak.value.isDouble())
			{
				const double p = scan.stressPeak.value.toDouble();
				if (!hasPeak || p > globalPeak)
				{
					globalPeak = p;
					hasPeak = true;
				}
			}
		}
		if (_automation.present && !_automation.materialLimits.isEmpty() && !hasPeak)
		{
			_result.warns.append(QStringLiteral(
				"剩余系数未计算（材料库已配置但应力峰值扫描无值）——按缺项策略留空"));
		}
		QStringList unmatchedMaterials;
		for (const QString& matName : materialNames)
		{
			if (!_automation.materialLimits.contains(matName))
			{
				unmatchedMaterials.append(matName);
				continue;
			}
			const double limit = _automation.materialLimits.value(matName);
			//材料级保守侧（发射口径=全工况最大峰值）
			AutoValue rf = AutoAssembler::computeReserveFactor(limit, hasPeak ? globalPeak : 0.0);
			if (rf.value.isNull())
				rf.warn = QStringLiteral("%1（材料=%2）").arg(rf.warn, matName);
			_result.materialReserves.insert(matName, rf);
		}
		if (!unmatchedMaterials.isEmpty())
		{
			_result.warns.append(QStringLiteral(
				"材料库未配置以下材料极限强度（material_library）： %1——"
				"对应剩余系数按缺项策略留空").arg(unmatchedMaterials.join(QStringLiteral("、"))));
		}
		//逐工况关联峰值口径的参考剩余系数（预览呈现；发射口径恒为保守侧全局值——12 §2.3）。
		//分子取首条材料库条目（多材料歧义场景=材料级保守侧发射值已覆盖，参考值标首材料口径）
		if (!_automation.materialLimits.isEmpty())
		{
			const double firstLimit = _automation.materialLimits.first();
			const QString firstMaterial = _automation.materialLimits.firstKey();
			for (AssemblyResult::CaseScan& scan : _result.cases)
			{
				if (!scan.stressPeak.value.isDouble())
					continue;
				AutoValue rf = AutoAssembler::computeReserveFactor(firstLimit,
					scan.stressPeak.value.toDouble());
				if (!rf.value.isNull())
				{
					rf.warn = QStringLiteral("按首材料 %1 极限计算的工况参考值"
						"（取全工况最大峰值，偏保守）").arg(firstMaterial);
				}
				scan.reserve = rf;
			}
		}

		//BUG-301（C14①/B-301，11 §8 C14）：Tab0 汇总行三计数自增（键粒度唯一归属）——
		//统计对象=三期自动化范围内数据集键位（meta.title/author/calc_file、manual_fields
		//五键、cases[].results 四键×N 工况逐键、materials[].reserve_factor×M 材料逐键、
		//conclusion.verdict）；**排除清单**=设计性留空键（meta.company/loads 四键/allowable_
		//stress）、录入值键（用户显式负责）、默认句键（归 defaultSentenceCount 既有计数，
		//initMeta 自增）、images 截图键（批次 C）、meta.date/software_name（既有发射链
		//锁定锚非三期自动化范围）；逐键唯一归属：autoCount=来源自动∧值非 null∧无⚠、
		//warnCount=值非 null∧带⚠（降级/占位名/需核对）、failCount=值=null（配置缺失与
		//通道失败均计，告警区分原因——null 键走缺项策略标缺=红字依据）。键粒度计数与
		//告警"按工况聚合"两口径并存不冲突（计数=键、预览=工况聚合行）
		const mStrengthReportProjectConfig* countCfg = _input.config;
		int autoN = 0, warnN = 0, failN = 0;
		QStringList& failKeysRef = _result.failKeys;   //成员禁入 lambda 捕获——局部引用桥接
		//BUG-304（fix6）：failKeys 逐键明细与 failCount 同源自增（键粒度唯一归属不变）——
		//键位标签=数据集键位（逐工况键带工况名前缀），原因=item.warn 短文案或缺项标缺注记
		auto bucketItem = [&autoN, &warnN, &failN, &failKeysRef](const AutoValue& item,
			bool isDefaultSentenceKey, const QString& keyLabel)
		{
			if (isDefaultSentenceKey || item.source == QStringLiteral("录入"))
				return;   //默认句键/录入键排除（C14① 排除清单）
			if (item.value.isNull() || item.value.isUndefined())
			{
				++failN;   //值=null（无论来源——null 走缺项策略标缺=红字依据）
				failKeysRef.append(keyLabel
					+ (item.warn.isEmpty()
						? QStringLiteral("——按缺项策略留空（配置缺失/自动取值失败）")
						: QStringLiteral("——%1").arg(item.warn)));
				return;
			}
			if (item.confidence == QStringLiteral("warn") || !item.warn.isEmpty())
				++warnN;   //值非 null∧带⚠（降级/占位名/需核对）
			else
				++autoN;   //值非 null∧无⚠（自动来源）
		};
		//① meta 三键+manual_fields 五键（items 键位）
		bucketItem(_result.items.value(QStringLiteral("meta.title")), false,
			QStringLiteral("meta.title"));
		bucketItem(_result.items.value(QStringLiteral("meta.author")), false,
			QStringLiteral("meta.author"));
		bucketItem(_result.items.value(QStringLiteral("meta.calc_file")), false,
			QStringLiteral("meta.calc_file"));
		{
			const AutoValue constraint = _result.items.value(QStringLiteral("manual_fields.constraint_desc"));
			const AutoValue loadCase = _result.items.value(QStringLiteral("manual_fields.load_case_desc"));
			const AutoValue loadType = _result.items.value(QStringLiteral("manual_fields.load_type"));
			const AutoValue calcMethod = _result.items.value(QStringLiteral("manual_fields.calc_method"));
			bucketItem(constraint, _automation.defaultSentences.contains(
				QStringLiteral("manual_fields.constraint_desc")),
				QStringLiteral("manual_fields.constraint_desc"));
			bucketItem(loadCase, _automation.defaultSentences.contains(
				QStringLiteral("manual_fields.load_case_desc")),
				QStringLiteral("manual_fields.load_case_desc"));
			bucketItem(loadType, _automation.defaultSentences.contains(
				QStringLiteral("manual_fields.load_type")),
				QStringLiteral("manual_fields.load_type"));
			bucketItem(calcMethod, _automation.defaultSentences.contains(
				QStringLiteral("manual_fields.calc_method")),
				QStringLiteral("manual_fields.calc_method"));
			bucketItem(_result.items.value(QStringLiteral("manual_fields.element_type")), false,
				QStringLiteral("manual_fields.element_type"));
			bucketItem(_result.items.value(QStringLiteral("manual_fields.project_subject")), false,
				QStringLiteral("manual_fields.project_subject"));
		}
		//② cases[].results 四键×N 工况逐键（录入值经 config 判定排除；reserve_factor=
		//逐工况参考值口径——扫描无值=null 计 fail）
		for (const AssemblyResult::CaseScan& scan : _result.cases)
		{
			const QJsonObject entered = countCfg != nullptr
				? countCfg->caseResults.value(scan.caseName) : QJsonObject();
			auto enteredHas = [&entered](const char* key) -> bool
			{
				const QJsonValue v = entered.value(QLatin1String(key));
				return !v.isNull() && !v.isUndefined();
			};
			if (!enteredHas("stress_peak"))
				bucketItem(scan.stressPeak, false,
					QStringLiteral("%1.results.stress_peak").arg(scan.caseName));
			if (!enteredHas("displacement_max"))
				bucketItem(scan.dispMax, false,
					QStringLiteral("%1.results.displacement_max").arg(scan.caseName));
			if (!enteredHas("buckling_mode"))
				bucketItem(scan.buckling, false,
					QStringLiteral("%1.results.buckling_mode").arg(scan.caseName));
			if (!enteredHas("reserve_factor"))
				bucketItem(scan.reserve, false,
					QStringLiteral("%1.results.reserve_factor").arg(scan.caseName));   //逐工况 reserve=参考值（warn 污染口径如实归属）
		}
		//③ materials[].reserve_factor×M 材料逐键（录入排除）
		for (auto it = _result.materialReserves.constBegin();
			it != _result.materialReserves.constEnd(); ++it)
		{
			if (countCfg != nullptr)
			{
				const QJsonValue enteredRf = countCfg->materialValues.value(it.key())
					.value(QStringLiteral("reserve_factor"));
				if (!enteredRf.isNull() && !enteredRf.isUndefined())
					continue;   //录入键排除
			}
			bucketItem(it.value(), false,
				QStringLiteral("materials[%1].reserve_factor").arg(it.key()));
		}
		//④ conclusion.verdict（材料级 min 同 buildDataset 口径：发射集合内 reserve 任一非
		//null→verdict 非null=auto；全空=null=fail）。
		//PB-02：verdict 桶遍历集与发射面同源（三级链材料清单=buildDataset materials[]
		//行集；推导序同 Collector=录入 reserve＞自动保守侧）——原实现仅查 materialReserves
		//键集（①级通道子集），材料名走②级兜底通道+录入 reserve 时发射 verdict 正常
		//而 Tab0 误报失败（呈现面"失败 N"与文档结论句分裂）
		{
			bool verdictHas = false;
			for (const QString& matName : materialNames)
			{
				if (countCfg != nullptr)
				{
					const QJsonValue enteredRf = countCfg->materialValues.value(matName)
						.value(QStringLiteral("reserve_factor"));
					if (!enteredRf.isNull() && !enteredRf.isUndefined())
					{
						verdictHas = true;
						break;
					}
				}
				const auto rfIt = _result.materialReserves.constFind(matName);
				if (rfIt != _result.materialReserves.constEnd()
					&& !rfIt.value().value.isNull())
				{
					verdictHas = true;
					break;
				}
			}
			if (verdictHas)
				++autoN;
			else
			{
				++failN;
				failKeysRef.append(QStringLiteral("conclusion.verdict")
					+ QStringLiteral("——全部剩余系数均无值（材料库未配置/应力峰值无值/未录入），"
						"结论句整句按缺项文案"));
			}
		}
		_result.autoCount = autoN;
		_result.warnCount = warnN;
		_result.failCount = failN;
		++_done;   //收尾步完成
	}

	bool AutoAssemblySession::step()
	{
		if (finished())
			return false;
		if (_done == 0)
		{
			initMeta();   //防御（构造已调）：幂等
		}
		else if (_taskIdx < _tasks.size())
		{
			stepScanFrame();   //一个（工况,帧）扫描单元（读文件通道 IO 在此步内）
		}
		else if (!_reserveDone)
		{
			_reserveDone = true;
			finishReserve();
		}
		return !finished();
	}

	//同步全量装配（批次 A 形态保留：镜像自测/退化断言用；Dlg 走会话分步形态）
	AssemblyResult AutoAssembler::assemble(const AssemblyInput& input)
	{
		AutoAssemblySession session(input);
		while (session.step())
		{
		}
		return session.result();
	}

	//——Tab0 主视图部件（批次 A 交互面零改动保留 + 批次 B 预览行扩充）——
	mStrengthReportAutoPreview::mStrengthReportAutoPreview(QWidget* parent)
		: QWidget(parent)
	{
		buildUi();
		setPhase(PhaseIdle);
		rebuildTable();
	}

	void mStrengthReportAutoPreview::buildUi()
	{
		auto* layout = new QVBoxLayout(this);
		layout->setContentsMargins(6, 6, 6, 6);
		layout->setSpacing(4);

		_phaseLabel = new QLabel(this);
		_phaseLabel->setWordWrap(true);
		layout->addWidget(_phaseLabel);

		//装配预览一览表（12 §3.1）：输入项/预览值/来源/置信/操作 五列，整表只读
		//（覆盖编辑仍在页签原控件——Tab0 预览可跳页签，Tab0 内不再提供编辑控件=消双真源）
		_table = new QTableWidget(0, 5, this);
		_table->setHorizontalHeaderLabels(QStringList
		{ QStringLiteral("输入项"), QStringLiteral("当前值"), QStringLiteral("来源"),
		  QStringLiteral("可信度"), QStringLiteral("操作") });
		_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
		_table->setColumnWidth(0, 120);
		_table->setColumnWidth(3, 90);
		//UI-01：操作列宽自适应"修改…"按钮完整文本（全局 QSS 钉按钮宽 80px，列宽需容纳
		//按钮实际宽+单元格内边距——"修改…"完整显示，悬停 tooltip 有完整动作说明）
		const int opBtnW = _table->fontMetrics().horizontalAdvance(QStringLiteral("修改…")) + 24;
		_table->setColumnWidth(4, qMax(80, opBtnW));
		_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_table->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
		_table->setMinimumHeight(120);   //WO-3 改法③：整页硬下限 -30px，装载+展开态预算 436<456~474，不再挤压无 minH 的 _captureTable
		layout->addWidget(_table, 1);

		//汇总行（FR-343 三态告警分级落主视图：黄字需注意/红字失败，不阻断导出）
		_summaryLabel = new QLabel(this);
		_summaryLabel->setTextFormat(Qt::RichText);
		_summaryLabel->setWordWrap(true);
		layout->addWidget(_summaryLabel);

		//自动截图提示行（批次 C 填充：开关态+批量结果呈现面——开关本体在页签③"自动截图"
		//（O3-2 开关合一，13 §3 批次 C 字面），Tab0 不设第二开关=消双真源；Q3-3 默认关=
		//一键导出默认不含截图批量）
		_captureHint = new QLabel(QStringLiteral(
			"自动截图：开关关闭（默认）。开启与逐槽对应请在「截图槽管理」页签设置"
			"（页签③\"自动截图\"开关+[对应设置…]）；开启后一键导出将按对应设置批量截图。"), this);
		_captureHint->setWordWrap(true);
		layout->addWidget(_captureHint);

		//截图槽建议列表（FR-333）：槽/当前映射/建议三列+行内[采纳建议]/[清除]两动作钮；
		//无槽声明或全未装配建议时整表隐藏（占零布局高度）
		_captureTable = new QTableWidget(0, 4, this);
		_captureTable->setHorizontalHeaderLabels(QStringList
		{ QStringLiteral("截图槽"), QStringLiteral("当前对应"), QStringLiteral("建议"),
		  QStringLiteral("操作") });
		_captureTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		_captureTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
		_captureTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_captureTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
		//WO-2/P-03：cellWidget 行内按钮几何由列宽裁定、setMinimumWidth 失守（88+56 被压到 55），
		//列宽是唯一杠杆。按 :1255-1256"修改…"列同型先例显式配宽：
		//两钮 fit 值(16n+24)之和 + 按钮间距2 + 余量16 = 64+32+48+2+16 = 162 ≥ 工单"需146"
		const QFontMetrics capFm = _captureTable->fontMetrics();
		const int capOpW = capFm.horizontalAdvance(QStringLiteral("采纳建议"))
			+ capFm.horizontalAdvance(QStringLiteral("清除")) + 24 * 2 + 2 + 16;
		_captureTable->setColumnWidth(3, qMax(100, capOpW));   //S-4：与先例同型 qMax 兜底（纯风格项）
		_captureTable->setMaximumHeight(120);
		_captureTable->hide();
		layout->addWidget(_captureTable);
		_captureEditBtn = new QPushButton(QStringLiteral("对应设置…→页签③"), this);
		fitBtnMinWidth(_captureEditBtn);
		{
			auto* capRow = new QHBoxLayout();
			capRow->addStretch(1);
			capRow->addWidget(_captureEditBtn);
			layout->addLayout(capRow);
		}
		connect(_captureEditBtn, &QPushButton::clicked, this,
			[this]() { emit editMappingsRequested(); });

		//签栏说明行（FR-303/12 §2.6：折叠区旁固定说明行——签栏非数据槽，不进缺项统计，
		//零代码变化的呈现面。BUG-304（fix6）补"编写"与 meta.author 落域披露：htsy 等容器
		//"编写/校对/审核…"签栏在页脚（规则层扫描域=主文档流，正文无 meta.author 槽位），
		//编写自动值仅入数据集不落文档——防用户据预览行"编写=登录名"误判导出缺填）
		QLabel* signNote = new QLabel(QStringLiteral(
			"说明：编写/校对/审核/批准等签栏（页脚）由线下签署，导出不填写；若模板未声明"
			"\"编写\"槽位，\"编写\"的自动值不会写入文档。"), this);
		signNote->setWordWrap(true);
		layout->addWidget(signNote);

		//高级设置折叠区入口（12 §3.4：控件本体仍在页签①④，折叠区=入口不改布局——
		//FR-342"骨架不破坏"锚；默认折叠）
		_advancedToggle = new QToolButton(this);
		_advancedToggle->setText(QStringLiteral("高级设置 ▸"));
		_advancedToggle->setToolButtonStyle(Qt::ToolButtonTextOnly);
		_advancedToggle->setCheckable(true);
		_advancedToggle->setStyleSheet(QStringLiteral("QToolButton { border: none; }"));
		fitBtnMinWidth(_advancedToggle);   //UI-01：qss 钉 QToolButton 宽 37px 致"高级设置 ▸/▾"截断，显式最小宽必赢
		layout->addWidget(_advancedToggle);
		_advancedPanel = new QWidget(this);
		auto* advLayout = new QVBoxLayout(_advancedPanel);
		advLayout->setContentsMargins(16, 0, 0, 0);
		advLayout->setSpacing(2);
		QLabel* advNote = new QLabel(QStringLiteral(
			"预填值为自动取到的默认值，可直接使用；上表只读，如需修改请点对应行的"
			"「修改…」或到对应页签编辑。缺项策略/无水印截图/输出路径在页签④；"
			"标题/对象名/软件名/单元类型/默认句四键在页签①。"), _advancedPanel);
		advNote->setWordWrap(true);
		advLayout->addWidget(advNote);
		auto* advBtnRow = new QHBoxLayout();
		QPushButton* subjectBtn = new QPushButton(QStringLiteral("改标题/对象名→页签①"), _advancedPanel);
		QPushButton* exportTabBtn = new QPushButton(QStringLiteral("缺项策略/输出/水印→页签④"), _advancedPanel);
		//UI-01：跳转按钮宽度按完整文本自适应（全局 QSS 钉宽 80px 导致文本截断；
		//两钮同排由布局等高对齐，不改高度）
		fitBtnMinWidth(subjectBtn);
		fitBtnMinWidth(exportTabBtn);
		advBtnRow->addWidget(subjectBtn);
		advBtnRow->addWidget(exportTabBtn);
		advBtnRow->addStretch(1);
		advLayout->addLayout(advBtnRow);
		_advancedPanel->hide();
		layout->addWidget(_advancedPanel);
		connect(_advancedToggle, &QToolButton::toggled, this, [this](bool on)
			{
				_advancedPanel->setVisible(on);
				_advancedToggle->setText(on
					? QStringLiteral("高级设置 ▾") : QStringLiteral("高级设置 ▸"));
			});
		connect(subjectBtn, &QPushButton::clicked, this,
			[this]() { emit jumpRequested(JumpProjectSubject); });
		connect(exportTabBtn, &QPushButton::clicked, this,
			[this]() { emit jumpRequested(JumpExportTab); });

		//主按钮行（12 §3.1：S2 后使能；运行中=既有进度条+阶段消息，全页签禁用——setExporting）
		auto* btnRow = new QHBoxLayout();
		btnRow->addStretch(1);
		_mainBtn = new QPushButton(QStringLiteral("一键导出"), this);
		_mainBtn->setToolTip(QStringLiteral(
			"零配置一键导出：自动取到的默认值直接写入报告（可修改，不修改也可以）；"
			"与底部[一键导出]使用同一导出流程。"));
		_mainBtn->setEnabled(false);
		fitBtnMinWidth(_mainBtn);   //UI-01：同跳转按钮，宽度按完整文本自适应
		btnRow->addWidget(_mainBtn);
		layout->addLayout(btnRow);
		connect(_mainBtn, &QPushButton::clicked, this, [this]() { emit oneClickRequested(); });
	}

	void mStrengthReportAutoPreview::setPhase(int phase)
	{
		_phase = phase;
		if (_phaseLabel == nullptr)
			return;
		switch (phase)
		{
		case PhaseIdle:
			_phaseLabel->setText(QStringLiteral(
				"<span style='color:#606060;'>待取值——加载规则模板/打开计算文件后自动取值。</span>"));
			break;
		case PhaseAssembling:
			_phaseLabel->setText(QStringLiteral(
				"<span style='color:#606060;'>正在自动取值…（逐项进行，不抓图不切视口）</span>"));
			break;
		case PhaseReady:
			_phaseLabel->setText(QStringLiteral(
				"<span style='color:#008000;'>取值完成——可直接一键导出；预填值可在对应页签修改。</span>"));
			break;
		case PhaseAutoCapture:
			_phaseLabel->setText(QStringLiteral(
				"<span style='color:#606060;'>自动截图进行中，请勿操作 3D 视口…</span>"));
			break;
		case PhaseAssemble:
			_phaseLabel->setText(QStringLiteral(
				"<span style='color:#606060;'>正在汇总导出数据并预检…</span>"));
			break;
		case PhaseEngine:
			_phaseLabel->setText(QStringLiteral(
				"<span style='color:#606060;'>引擎导出中：%1</span>").arg(_lastHeadline));
			break;
		case PhaseDone:
			_phaseLabel->setText(_lastHeadline);
			break;
		default:
			break;
		}
	}

	void mStrengthReportAutoPreview::beginAssembly()
	{
		setPhase(PhaseAssembling);
	}

	void mStrengthReportAutoPreview::applyResult(const AssemblyResult& result)
	{
		_result = result;
		rebuildTable();
		updateSummary();
		setPhase(PhaseReady);
	}

	void mStrengthReportAutoPreview::setExporting(bool on)
	{
		_exporting = on;
		if (on)
			setPhase(PhaseAssemble);
		else if (_phase == PhaseAssemble || _phase == PhaseEngine
			|| _phase == PhaseAutoCapture)   //LG-08/LG-02：S3 批量段收尾同口径回 S2
			setPhase(PhaseReady);   //导出链中止（取消/失败收尾前的兜底回 S2——完成态由 finishExport 置）
		//三期批次 C：导出中（含截图批量段）禁用建议采纳/清除/映射入口动作（防并发改映射）
		if (_captureTable != nullptr)
			_captureTable->setEnabled(!on);
		if (_captureEditBtn != nullptr)
			_captureEditBtn->setEnabled(!on);
	}

	//——三期批次 C（FR-333，13 §3 批次 C Tab0 行）：截图槽建议列表+提示行呈现面——
	void mStrengthReportAutoPreview::enterCapturePhase()
	{
		//LG-08：批量截图段（S3）落 phase 状态机——此前 phaseLabel 滞留 S2 文案，
		//与 12 §2.1 六态图不完全对齐（行为无歧义纯呈现缺口）
		setPhase(PhaseAutoCapture);
	}

	void mStrengthReportAutoPreview::updateManualEntryRow(const QString& itemKey,
		const QString& value, const QString& source)
	{
		//LG-05（UB-05）：页签①非空改写=录入语义——预览对应行值列/来源列即时同步，
		//预览与 collectUiToConfig+buildDataset 实际发射一致（录入优先口径镜像）；
		//行定位=rebuildTable 写入标签项 UserRole 的数据集键位（下方重建处同源写入）
		if (_table == nullptr)
			return;
		for (int row = 0; row < _table->rowCount(); ++row)
		{
			const QTableWidgetItem* label = _table->item(row, 0);
			if (label == nullptr || label->data(Qt::UserRole).toString() != itemKey)
				continue;
			if (QTableWidgetItem* valueItem = _table->item(row, 1))
			{
				valueItem->setText(value.isEmpty()
					? QStringLiteral("—（按缺项策略留空）") : value);
				valueItem->setForeground(QBrush(QColor(0x20, 0x20, 0x20)));
			}
			if (QTableWidgetItem* sourceItem = _table->item(row, 2))
				sourceItem->setText(source);
			return;
		}
	}

	void mStrengthReportAutoPreview::setCaptureHint(const QString& html)
	{
		if (_captureHint != nullptr)
			_captureHint->setText(html);
	}

	void mStrengthReportAutoPreview::setCaptureSlots(const QList<CaptureSlotRow>& rows)
	{
		if (_captureTable == nullptr)
			return;
		_captureTable->setRowCount(rows.size());
		for (int i = 0; i < rows.size(); ++i)
		{
			const CaptureSlotRow& row = rows.at(i);
			auto* keyItem = new QTableWidgetItem(row.key);
			keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
			_captureTable->setItem(i, 0, keyItem);
			auto* mapItem = new QTableWidgetItem(row.mappingText);
			mapItem->setFlags(mapItem->flags() & ~Qt::ItemIsEditable);
			_captureTable->setItem(i, 1, mapItem);
			auto* sugItem = new QTableWidgetItem(row.suggestionText.isEmpty()
				? QStringLiteral("-") : row.suggestionText);
			sugItem->setFlags(sugItem->flags() & ~Qt::ItemIsEditable);
			if (!row.suggestionText.isEmpty())
			{
				//FR-333：建议黄字"建议·来自自动扫描"标注（建议≠配置）
				sugItem->setForeground(QBrush(QColor(0xB8, 0x86, 0x0B)));
				sugItem->setToolTip(QStringLiteral("建议值来自自动扫描（峰值帧号）；"
					"点击[采纳建议]才写入对应设置，[清除]恢复手动状态。"));
			}
			_captureTable->setItem(i, 2, sugItem);
			//行内动作钮：[采纳建议]（无建议=禁用）+[清除]（无映射=禁用）
			auto* actions = new QWidget(_captureTable);
			auto* btnRow = new QHBoxLayout(actions);
			btnRow->setContentsMargins(0, 0, 0, 0);
			btnRow->setSpacing(2);
			auto* adoptBtn = new QPushButton(QStringLiteral("采纳建议"), actions);
			auto* clearBtn = new QPushButton(QStringLiteral("清除"), actions);
			fitBtnMinWidth(adoptBtn);
			fitBtnMinWidth(clearBtn);
			btnRow->addWidget(adoptBtn);
			btnRow->addWidget(clearBtn);
			btnRow->addStretch(1);
			_captureTable->setCellWidget(i, 3, actions);
			adoptBtn->setEnabled(!row.suggestionText.isEmpty() && !_exporting);
			clearBtn->setEnabled(row.mappingText != QStringLiteral("-") && !_exporting);
			const QString key = row.key;
			connect(adoptBtn, &QPushButton::clicked, this,
				[this, key]() { emit adoptSuggestion(key); });
			connect(clearBtn, &QPushButton::clicked, this,
				[this, key]() { emit clearSlotMapping(key); });
		}
		_captureTable->setVisible(!rows.isEmpty());
	}

	void mStrengthReportAutoPreview::setStageText(const QString& text)
	{
		if (_phase == PhaseAssemble || _phase == PhaseEngine || _exporting)
		{
			_lastHeadline = text;
			setPhase(PhaseEngine);
		}
	}

	void mStrengthReportAutoPreview::finishExport(bool ok, const QString& headline)
	{
		_lastHeadline = QStringLiteral("<span style='color:%1;'>%2</span>")
			.arg(ok ? QStringLiteral("#008000") : QStringLiteral("#C00000"),
				ok ? QStringLiteral("导出完成——%1").arg(headline)
				   : QStringLiteral("导出失败——%1").arg(headline));
		setPhase(PhaseDone);
	}

	void mStrengthReportAutoPreview::resetSession()
	{
		_result = AssemblyResult();
		_lastHeadline.clear();
		_exporting = false;
		rebuildTable();
		updateSummary();
		setPhase(PhaseIdle);
	}

	void mStrengthReportAutoPreview::setExportEnabled(bool on)
	{
		_exportEnabled = on;
		if (_mainBtn != nullptr)
		{
			//S2/S6 使能（12 §2.1：主按钮使能面=装配就绪+既有加载态/probe/导出旗标正交组合；
			//S6 完成态支持重复一键导出——NFR-3.5 幂等）
			_mainBtn->setEnabled(on && (_phase == PhaseReady || _phase == PhaseDone));
		}
	}

	void mStrengthReportAutoPreview::rebuildTable()
	{
		if (_table == nullptr)
			return;
		_table->setRowCount(0);
		//批次 A 呈现行集（键=数据集键位）+批次 B 扩充行（单元类型/默认句四键/cases.load_type；
		//未知键自动尾部呈现——前向兼容）
		struct RowSpec
		{
			QString key;
			QString label;
			int jumpRole;
			bool displaySanitized;
		};
		const QVector<RowSpec> rows
		{
			{ QStringLiteral("meta.title"),                    QStringLiteral("标题"), JumpProjectSubject, true },
			{ QStringLiteral("manual_fields.project_subject"), QStringLiteral("对象名"), JumpProjectSubject, true },
			{ QStringLiteral("meta.calc_file"),               QStringLiteral("计算文件"), -1, true },
			{ QStringLiteral("meta.date"),                    QStringLiteral("日期"), -1, false },
			{ QStringLiteral("model_stats.software_name"),    QStringLiteral("软件名"), JumpSoftwareName, false },
			{ QStringLiteral("meta.author"),                  QStringLiteral("编写"), -1, false },
			//批次 B（FR-311/312/313）：单元类型+默认句四键+逐工况载荷句
			{ QStringLiteral("manual_fields.element_type"),   QStringLiteral("单元类型"), JumpElementType, false },
			{ QStringLiteral("manual_fields.constraint_desc"), QStringLiteral("约束描述"), JumpConstraintDesc, false },
			{ QStringLiteral("manual_fields.load_case_desc"),  QStringLiteral("加载工况"), JumpLoadCaseDesc, false },
			{ QStringLiteral("manual_fields.load_type"),      QStringLiteral("载荷类型"), JumpLoadType, false },
			{ QStringLiteral("manual_fields.calc_method"),    QStringLiteral("计算方法"), JumpCalcMethod, false },
			{ QStringLiteral("cases.load_type"),              QStringLiteral("载荷句（逐工况）"), -1, false },
		};
		QStringList presentKeys;
		for (const RowSpec& spec : rows)
		{
			if (_result.items.contains(spec.key))
				presentKeys.append(spec.key);
		}
		for (auto it = _result.items.constBegin(); it != _result.items.constEnd(); ++it)
		{
			if (!presentKeys.contains(it.key()))
				presentKeys.append(it.key());   //批次 B/C 新增键（前向兼容）
		}

		auto appendRow = [this](const QString& label, const AutoValue& item, int jumpRole,
			bool displaySanitized)
		{
			const int row = _table->rowCount();
			_table->insertRow(row);
			_table->setItem(row, 0, readOnlyItem(label));

			//预览值：null="—"灰字（走缺项策略）；字符串=显示净化后文本（E3-5）；数值='g'15
			QString display;
			if (item.value.isString())
				display = displaySanitized
					? mStrengthReportProjectConfig::sanitizeProjectId(item.value.toString())
					: item.value.toString();
			else if (item.value.isDouble())
				display = scanNumberText(item.value.toDouble());
			QTableWidgetItem* valueItem = readOnlyItem(item.value.isNull()
				? QStringLiteral("—（按缺项策略留空）") : display);
			if (item.value.isNull())
				valueItem->setForeground(QBrush(kNullColor));
			if (!item.warn.isEmpty())
				valueItem->setToolTip(item.warn);   //12 §3.1：⚠ 悬停→原因文案
			_table->setItem(row, 1, valueItem);

			_table->setItem(row, 2, readOnlyItem(item.source.isEmpty()
				? QStringLiteral("—") : item.source));

			//置信列：●高/⚠ 需注意/✘ 失败/— 中性（FR-343 分级）；自动来源可覆盖项加"·可修改"
			QString confidenceText;
			QColor confidenceColor(0x20, 0x20, 0x20);
			if (item.confidence == QLatin1String("warn"))
			{
				confidenceText = QStringLiteral("⚠ 需注意");
				confidenceColor = kWarnColor;
			}
			else if (item.confidence == QLatin1String("fail"))
			{
				confidenceText = QStringLiteral("✘ 失败");
				confidenceColor = kFailColor;
			}
			else if (item.confidence == QLatin1String("info"))
			{
				confidenceText = QStringLiteral("—");
				confidenceColor = kNullColor;
			}
			else
			{
				confidenceText = QStringLiteral("●高");
				if (!item.value.isNull() && item.source.startsWith(QStringLiteral("自动"))
					&& jumpRole >= 0)
					confidenceText += QStringLiteral("·可修改");
			}
			QTableWidgetItem* confItem = readOnlyItem(confidenceText);
			confItem->setForeground(QBrush(confidenceColor));
			_table->setItem(row, 3, confItem);

			//操作列：可覆盖项=跳转覆盖按钮（12 §3.1"○可覆盖 点击→跳对应页签定位控件"）
			if (jumpRole >= 0 && !item.value.isNull())
			{
				QPushButton* jumpBtn = new QPushButton(QStringLiteral("修改…"), _table);
				jumpBtn->setToolTip(QStringLiteral(
					"跳转到对应页签定位并修改（上表只读）；修改后以新值写入，不修改则用默认值"));
				_table->setCellWidget(row, 4, jumpBtn);
				const int role = jumpRole;
				connect(jumpBtn, &QPushButton::clicked, this,
					[this, role]() { emit jumpRequested(role); });
			}
			else
			{
				_table->setItem(row, 4, readOnlyItem(QStringLiteral("-")));
			}
		};

		for (const QString& key : presentKeys)
		{
			QString label = key;
			int jumpRole = -1;
			bool displaySanitized = false;   //E3-5：文件名派生键显示侧净化（数据集发射原始名）
			for (const RowSpec& spec : rows)
			{
				if (spec.key == key)
				{
					label = spec.label;
					jumpRole = spec.jumpRole;
					displaySanitized = spec.displaySanitized;
					break;
				}
			}
			//内部标注键（E3-11 规则层未配置呈现行）特殊文案。
			//PA-14：装配侧已按"模板未加载/未配置"两态写入中性文案（value 字符串在位
			//则原样呈现）；历史会话值缺位时回退既有缺省文案
			const AutoValue item = _result.items.value(key);
			if (key == QStringLiteral("__automation__"))
			{
				AutoValue shown = item;
				if (!shown.value.isString())
					shown.value = QJsonValue(QStringLiteral(
						"模板未配置自动化内容（无默认句/对应关系/材料库——按缺项策略导出）"));
				appendRow(QStringLiteral("自动化配置"), shown, -1, false);
				continue;
			}
			if (key == QStringLiteral("__automation_shape__"))
			{
				//BUG-303：形态识别失败 ⚠ 行（预览留痕——不留静默）
				appendRow(QStringLiteral("自动取值配置（stress_variable_map）"), item, -1, false);
				continue;
			}
			appendRow(label, item, jumpRole, displaySanitized);
			//LG-05：标签项写入数据集键位（updateManualEntryRow 行定位数据面）
			if (QTableWidgetItem* labelItem = _table->item(_table->rowCount() - 1, 0))
				labelItem->setData(Qt::UserRole, key);
		}

		//批次 B 逐工况扫描行（12 §3.1"工况结果"行集：工况名/四键值/来源/置信/⚠ 聚合 tooltip）
		for (const AssemblyResult::CaseScan& scan : _result.cases)
		{
			QStringList parts;
			int worst = 0;   //0=high 1=warn 2=fail
			QString warnTexts;
			auto absorb = [&parts, &worst, &warnTexts](const AutoValue& v)
			{
				if (v.value.isDouble())
					parts.append(scanNumberText(v.value.toDouble()));
				else if (v.value.isString())
					parts.append(v.value.toString());
				if (v.confidence == QLatin1String("fail"))
					worst = qMax(worst, 2);
				else if (v.confidence == QLatin1String("warn"))
					worst = qMax(worst, 1);
				if (!v.warn.isEmpty())
					warnTexts += (warnTexts.isEmpty() ? QString() : QStringLiteral("\n")) + v.warn;
			};
			if (scan.stressPeak.value.isDouble() || !scan.stressPeak.warn.isEmpty())
			{
				parts.append(QStringLiteral("应力峰值=") + (scan.stressPeak.value.isDouble()
					? scanNumberText(scan.stressPeak.value.toDouble()) : QStringLiteral("-")));
				absorb(scan.stressPeak);
			}
			if (scan.dispMax.value.isDouble() || !scan.dispMax.warn.isEmpty())
			{
				parts.append(QStringLiteral("位移=") + (scan.dispMax.value.isDouble()
					? scanNumberText(scan.dispMax.value.toDouble()) : QStringLiteral("-")));
				absorb(scan.dispMax);
			}
			//PA-09：阶次越界（值 null+warn）同样上浮工况行——与应力/位移一致
			//（"键名=-"+tooltip），失败原因不再只有计数面可见
			if (scan.buckling.value.isString() || !scan.buckling.warn.isEmpty())
			{
				parts.append(QStringLiteral("失稳=") + (scan.buckling.value.isString()
					? scan.buckling.value.toString() + QStringLiteral(" 阶")
					: QStringLiteral("-")));
				absorb(scan.buckling);
			}
			if (scan.reserve.value.isDouble())
			{
				parts.append(QStringLiteral("剩余系数=")
					+ scanNumberText(scan.reserve.value.toDouble()) + QStringLiteral("（参考）"));
				absorb(scan.reserve);
			}
			//PA-03：峰值帧副产品按链分口径呈现（应力峰值帧/位移帧各自归属，
			//不再共用单一槽位互写覆没）
			if (!scan.stressPeakFrameName.isEmpty())
				parts.append(QStringLiteral("应力峰值帧=%1").arg(scan.stressPeakFrameName));
			if (!scan.dispMaxFrameName.isEmpty())
				parts.append(QStringLiteral("位移帧=%1").arg(scan.dispMaxFrameName));

			AutoValue row;
			row.value = parts.isEmpty()
				? QJsonValue() : QJsonValue(parts.join(QStringLiteral("，")));
			row.source = parts.isEmpty()
				? QStringLiteral("—") : QStringLiteral("自动·结果扫描");
			row.confidence = worst == 2 ? QStringLiteral("fail")
				: worst == 1 ? QStringLiteral("warn") : QStringLiteral("high");
			row.warn = warnTexts;
			appendRow(scan.caseName, row, -1, false);
		}

		//批次 B 材料级剩余系数行（FR-324：保守侧发射口径=全工况最大峰值分母）
		for (auto it = _result.materialReserves.constBegin();
			it != _result.materialReserves.constEnd(); ++it)
		{
			appendRow(QStringLiteral("剩余系数·%1").arg(it.key()), it.value(), -1, false);
		}
	}

	void mStrengthReportAutoPreview::updateSummary()
	{
		if (_summaryLabel == nullptr)
			return;
		//汇总行（12 §3.1："自动值 N·默认句 N·需注意 N·失败 N"）+FR-343 三态分级：
		//需注意 N>0 黄字/失败 N>0 红字（提示性不阻断——导出继续，缺项策略语义）
		QString html = QStringLiteral("自动值 %1 · 默认句 %2 · ")
			.arg(_result.autoCount).arg(_result.defaultSentenceCount);
		html += QStringLiteral("<span style='color:%1;'>需注意 %2</span> · ")
			.arg(_result.warnCount > 0 ? QStringLiteral("#B8860B") : QStringLiteral("#606060"))
			.arg(_result.warnCount);
		html += QStringLiteral("<span style='color:%1;'>失败 %2</span>")
			.arg(_result.failCount > 0 ? QStringLiteral("#C00000") : QStringLiteral("#606060"))
			.arg(_result.failCount);
		//R3P-03：聚合告警呈现面（warns 全链零消费治理——PB-01/PA-10/PB-03/E3-2/E3-3/
		//材料双空降级的定位文本经此可见）：汇总行下追加 ⚠ 计数行（黄字），完整清单
		//悬停查看（上限 10 条折叠同 BUG-302 惯例；N=0 零呈现=无 automation 容器不告噪）。
		//BUG-304（fix6）：失败键逐键明细同面呈现（tooltip"失败键 N 项"块）——消"失败 N
		//只有计数无明细"UX 缺口；明细与 failCount 同源自增恒一致（键粒度唯一归属不变）
		QString tip;
		if (!_result.warns.isEmpty())
		{
			const int total = _result.warns.size();
			html += QStringLiteral("<br><span style='color:#B8860B;'>⚠ 取值告警 %1 条"
				"（悬停查看明细）</span>").arg(total);
			QStringList shown = _result.warns.mid(0, qMin(total, 10));
			if (total > 10)
				shown.append(QStringLiteral("…等 %1 条").arg(total));
			tip += QStringLiteral("取值告警汇总 %1 条：\n%2").arg(total)
				.arg(shown.join(QStringLiteral("\n")));
		}
		if (!_result.failKeys.isEmpty())
		{
			const int fkTotal = _result.failKeys.size();
			html += QStringLiteral("<br><span style='color:#C00000;'>失败键明细 %1 项"
				"（悬停查看逐键）</span>").arg(fkTotal);
			QStringList fkShown = _result.failKeys.mid(0, qMin(fkTotal, 10));
			if (fkTotal > 10)
				fkShown.append(QStringLiteral("…等 %1 项").arg(fkTotal));
			if (!tip.isEmpty())
				tip += QStringLiteral("\n\n");
			tip += QStringLiteral("失败键 %1 项明细（与\"失败 %2\"计数一致）：\n%3")
				.arg(fkTotal).arg(_result.failCount)
				.arg(fkShown.join(QStringLiteral("\n")));
		}
		_summaryLabel->setText(html);
		_summaryLabel->setToolTip(tip);
	}
}
