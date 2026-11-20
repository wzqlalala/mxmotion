#include "mStrengthReportCollector.h"
#include "mStrengthReportConfig.h"
#include "mStrengthReportAutoPreview.h"   //三期批次 B：AssemblyResult 装配合并口（前向声明实体）

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QScreen>
#include <QTimer>

#include <algorithm>
#include <limits>

#include "mViewportModelManager.h"   //getCVM/VMManager 宏（PreData，只读调用）
#include "mViewportModel.h"
#include "mPostGUIData.h"            //getGUIDataByPostRender→_cloudFrame 当前帧号
#include "mModelData.h"              //MechModelData::mModelData::getManager/getAllNames
#include "mPostDataManager.h"        //Physical_Post 管理器→getPostDatabase（只读）
#include "mPostDataBase.h"
#include "mxr/view/REBUILD_PostRend.h"   //REBUILD_PostRend：getPostRenderWidget/getPostDataName
#include "mPhysicalFieldEnum.h"      //MxFunctions::Physical_Post/Physical_Material

using namespace MechPreData;
using namespace PostData;
using namespace MxFunctions;

namespace StrengthReport
{
	namespace
	{
		//与 postDB 取数样板（mPostExportWord.cpp:1351-1393）同链路的轻量封装：
		//getCVM→postRend→postname→getManager<mPostDataManager>(Physical_Post)→getPostDatabase
		QPointer<mPostDataBase> currentPostDb()
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr)
				return nullptr;
			const QString postname = postRend->getPostDataName();
			QPointer<mPostDataBase> postDB = getCVM->getModelData()
				->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			return postDB;
		}

		//首期 software_name 默认常量（D-24① 单键单值；页签①可录入覆盖并随项目配置记忆）。
		//产品名采集通道首期未定义，取批次 1 样例数据集同值保持引擎链路一致
		const QString& defaultSoftwareName()
		{
			static const QString name = QStringLiteral("迈曦CAE");
			return name;
		}

		//D3-1（C-4 裁定）：v06+ 无材料部件产生的默认材料占位名——不过滤逐字进清单
		const QString& defaultMaterialName()
		{
			static const QString name = QStringLiteral("default");
			return name;
		}

		//缺项策略覆盖键枚举（FR-211；与 tdrrules.MISSING_POLICY_ENUM/页签④三选一同集）
		bool isMissingPolicyEnumValue(const QString& policy)
		{
			static const QStringList enumValues
			{ QStringLiteral("dash_and_warn"), QStringLiteral("keep_highlight"),
			  QStringLiteral("block_export") };
			return enumValues.contains(policy);
		}

		//数据集键值取数（undefined→显式 null）：QJsonObject::insert 对 Undefined 值按
		//删除键处理，配置值对象缺失键必须归一为显式 null 才能保住"键位在、值 null"
		//的契约形状（02 §4.2/09 §4.1——null 才走缺项策略，键缺失同效但形状漂移）
		QJsonValue jsonValueOrNull(const QJsonObject& obj, const QString& key)
		{
			const QJsonValue v = obj.value(key);
			return v.isUndefined() ? QJsonValue(QJsonValue::Null) : v;
		}

		//——二期批次 C（C-5，09 §0.2/R2-5）：logo 翻转 RAII 栈守卫——
		//构造=按水印开关期望态强制复位（E2.5：防上次异常残留；带水印档=复位可见、
		//无水印档=临时隐藏）；析构=恢复可见+请求重绘（异步，不阻塞返回）。
		//栈对象生命周期覆盖抓空/落盘失败/异常全部早退路径（一期 :282-285 isNull
		//早退即漏恢复点；C++ 无 finally）。仅调用 mBaseRend::setLogoVisible 既有
		//透传接口（mBaseRend.h:188，渲染库零修改）；rend 为空=守卫不激活（调用方告警）
		class LogoFlipGuard
		{
		public:
			LogoFlipGuard(bool hideDuringCapture, REBUILD_PostRend* rend, QWidget* repaintTarget)
				: _rend(rend), _target(repaintTarget)
			{
				if (_rend == nullptr)
					return;   //门面不可达：按带水印抓（调用方经 warn 告警，开关失效）
				//进入抓取前强制复位为当前开关期望态（E2.5）
				_rend->setLogoVisible(!hideDuringCapture);
			}
			~LogoFlipGuard()
			{
				if (_rend == nullptr)
					return;
				_rend->setLogoVisible(true);   //无水印仅限抓取瞬间——恢复产品水印交付观感
				if (_target != nullptr)
					_target->update();   //恢复像素（异步，不阻塞返回——09 步10）
			}
			LogoFlipGuard(const LogoFlipGuard&) = delete;
			LogoFlipGuard& operator=(const LogoFlipGuard&) = delete;

		private:
			REBUILD_PostRend* _rend = nullptr;
			QWidget* _target = nullptr;
		};
	}

	QString mStrengthReportCollector::currentProjectId()
	{
		//项目标识=模型级（D-12/V-05）：当前后处理数据名（postRend→getPostDataName）。
		//做文件名净化（Windows 非法字符替换 '_'），空视口返回空串由调用方兜底
	REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
	if (postRend == nullptr)
		return QString();
	const QString name = postRend->getPostDataName();
	if (name.isEmpty())
		return QString();
	//CB-13：净化规则与 Config::configFilePath 共用 sanitizeProjectId 唯一实现——
	//配置目录与截图目录两处项目标识形态必须一致（非法字符替换 '_'、结尾点剥离、段内点保留）
	return mStrengthReportProjectConfig::sanitizeProjectId(name);
}

	QJsonObject mStrengthReportCollector::collectModelStats(const QString& softwareName, QString& err)
	{
		err.clear();
		QJsonObject stats;
		stats.insert(QStringLiteral("software_name"),
			softwareName.isEmpty() ? defaultSoftwareName() : softwareName);
		QPointer<mPostDataBase> postDB = currentPostDb();
		if (postDB == nullptr)
		{
			stats.insert(QStringLiteral("node_count"), QJsonValue());
			stats.insert(QStringLiteral("element_count"), QJsonValue());
			err = QStringLiteral("模型统计不可用（无激活后处理视口或后处理数据）");
			return stats;
		}
		//当前帧模型数据（样板 mPostExportWord.cpp:1351-1391：guidata->_cloudFrame→
		//getModelData(framei)→_nodedata.nodeamount/getElementAmount）
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		int framei = 1;
		if (postRend != nullptr)
		{
			//GUI 数据（当前云图帧号）；取不到退回第 1 帧（节点/单元总数与帧无关）
			if (mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend))
				framei = guidata->_cloudFrame;
		}
		if (framei <= 0)
			framei = 1;
		mPostModelData* modeldata = postDB->getModelData(framei);
		if (modeldata == nullptr)
		{
			stats.insert(QStringLiteral("node_count"), QJsonValue());
			stats.insert(QStringLiteral("element_count"), QJsonValue());
			err = QStringLiteral("模型统计不可用（帧模型数据未就绪）");
			return stats;
		}
		stats.insert(QStringLiteral("node_count"), modeldata->_nodedata.nodeamount);
		stats.insert(QStringLiteral("element_count"), modeldata->getElementAmount());
		return stats;
	}

	QStringList mStrengthReportCollector::collectCaseNames()
	{
		QPointer<mPostDataBase> postDB = currentPostDb();
		if (postDB == nullptr)
			return QStringList();
		return postDB->getAllCaseNames();
	}

		QJsonObject mStrengthReportCollector::collectCaseLoads()
	{
		//T-09 实测列集键位（表7：工况/端面载荷[gridSpan=2]/外压(MPa)/温度）。
		//首期通道未核实→全 null（03 §4.5：通道缺失键=null，E1 标缺，不静默填错）
		QJsonObject loads;
		loads.insert(QStringLiteral("position"), QJsonValue());
		loads.insert(QStringLiteral("end_load"), QJsonValue());
		loads.insert(QStringLiteral("pressure"), QJsonValue());
		loads.insert(QStringLiteral("temperature"), QJsonValue());
		return loads;
	}

	mStrengthReportCollector::MaterialList mStrengthReportCollector::collectMaterialNames(
		QString& err)
	{
		err.clear();
		MaterialList list;
		//①主通道（FR-201，09 §0.1）：postDB 材料→零件映射（MxdbDataBuilder 读结果文件
		//初始帧回填 ：189-204/:294 注入——纯后处理态即有值，与 getAllCaseNames 同 postDB）
		QPointer<mPostDataBase> postDB = currentPostDb();
		if (postDB != nullptr)
		{
			const QHash<QString, QVector<int>> materialParts = postDB->getMaterialParts();
			if (!materialParts.isEmpty())
			{
				//D3-2 定序：QHash 无稳定遍历序→各材料映射零件 min(partId) 升序、
				//同序名称字典序（确定性发射——R2-4 数据集/报告 diff 防护）
				QList<QPair<int, QString>> keyed;
				keyed.reserve(materialParts.size());
				for (auto it = materialParts.constBegin(); it != materialParts.constEnd(); ++it)
				{
					int minId = std::numeric_limits<int>::max();
					const QVector<int>& ids = it.value();
					for (const int id : ids)
					{
						if (id < minId)
							minId = id;
					}
					keyed.append(qMakePair(minId, it.key()));
				}
				std::sort(keyed.begin(), keyed.end(),
					[](const QPair<int, QString>& a, const QPair<int, QString>& b)
					{
						if (a.first != b.first)
							return a.first < b.first;
						return a.second < b.second;
					});
				for (const auto& pair : keyed)
					list.names.append(pair.second);
				list.source = QStringLiteral("结果文件");
				list.ok = true;
				return list;   //D3-1："default"占位名不过滤（C-4 裁定）
			}
		}
		//②兜底通道：模型侧统一入口（mModelData.cpp:154-163→getAllMatNames）——仅建模
		//模型在役时有值（纯后处理态不可达=登记语义降级，09 §0.1）；有序清单保持返回序
		if (MechModelData::mModelData* modelData = getCVM->getModelData())
		{
			const QStringList names = modelData->getAllNames(Physical_Material);
			for (const QString& name : names)
			{
				if (!name.isEmpty() && !list.names.contains(name))
					list.names.append(name);   //保序去重（清单语义防御）
			}
			if (!list.names.isEmpty())
			{
				list.source = QStringLiteral("模型数据");
				list.ok = true;
				return list;
			}
		}
		//③双空降级（FR-202，E2.1）：materials=[]+聚合告警 1 条（调用方并入 buildDataset
		//errs 汇总），导出继续、表8=表头+0 行组（一期降级形态）
		err = QStringLiteral("材料数据不可得（结果文件无材料信息且模型侧无材料清单），"
			"表8 按无数据行组输出");
		return list;
	}

	//二期形状入口（兼容锚）：nullptr=不并装配结果，发射形状与二期逐字节一致（R3-2 锚）
	QJsonDocument mStrengthReportCollector::buildDataset(const mStrengthReportProjectConfig& cfg,
		QString& err)
	{
		return buildDataset(cfg, err, nullptr);
	}

	QJsonDocument mStrengthReportCollector::buildDataset(const mStrengthReportProjectConfig& cfg,
		QString& err, const AssemblyResult* autoValues)
	{
		err.clear();
		QStringList errs;

		QJsonObject meta;
		//五键首期未被模板短语消费（schema 预留——H-05/C3-05）；date 取当前日期。
		//CB-08：空值发 null 不发空串——与 manual_fields 同口径（空串会被引擎当作
		//绑定值执行替换清槽，null 才走缺项策略；meta.* 未来被容器绑定时同此语义）
		meta.insert(QStringLiteral("title"),
			cfg.projectSubject.isEmpty() ? QJsonValue() : QJsonValue(cfg.projectSubject));
		meta.insert(QStringLiteral("author"), QJsonValue());
		meta.insert(QStringLiteral("company"), QJsonValue());
		meta.insert(QStringLiteral("date"),
			QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
		meta.insert(QStringLiteral("calc_file"), QJsonValue());

		//三期批次 B 装配合并口（FR-325/13 §3）：meta.title/author/calc_file——仅"自动"来源
		//且非 null 注入（录入值已由上方 config 发射=E3-7 优先级；null=走缺项策略 CB-08）。
		//autoValues==nullptr 时本段零执行=二期形状逐字节兼容锚（R3-2 退化一致性）
		const auto assemblyItem = [autoValues](const QString& key) -> AutoValue
		{
			return autoValues != nullptr
				? autoValues->items.value(key) : AutoValue();
		};
		{
			const AutoValue title = assemblyItem(QStringLiteral("meta.title"));
			const AutoValue author = assemblyItem(QStringLiteral("meta.author"));
			const AutoValue calcFile = assemblyItem(QStringLiteral("meta.calc_file"));
			if (title.source.startsWith(QStringLiteral("自动")) && !title.value.isNull())
				meta.insert(QStringLiteral("title"), title.value);
			if (author.source.startsWith(QStringLiteral("自动")) && !author.value.isNull())
				meta.insert(QStringLiteral("author"), author.value);
			if (calcFile.source.startsWith(QStringLiteral("自动")) && !calcFile.value.isNull())
				meta.insert(QStringLiteral("calc_file"), calcFile.value);
		}

		QString statsErr;
		const QJsonObject modelStats = collectModelStats(cfg.softwareName, statsErr);
		if (!statsErr.isEmpty())
			errs.append(statsErr);

		//cases[]：自动采集工况名+人工载荷类型；loads 首期全 null；results=页签②录入值
		//或 null（二期 FR-241 录入版 Q2-3=A：四键从"恒 null"转录入值——键位在、未录值
		//null 走缺项策略标缺+按工况聚合告警，C7/D-4"绝不静默填错值"口径不变；
		//当前模型不存在的历史录入条目不发射——08 §2.3 持久化口径）
		//批次 B：逐工况扫描结果索引（按工况名匹配——CaseScan.caseName 与 collectCaseNames
		//同源 getAllCaseNames）
		QMap<QString, const AssemblyResult::CaseScan*> scanByCase;
		if (autoValues != nullptr)
		{
			for (const AssemblyResult::CaseScan& scan : autoValues->cases)
				scanByCase.insert(scan.caseName, &scan);
		}
		const AutoValue casesLoadType = assemblyItem(QStringLiteral("cases.load_type"));

		const QStringList caseNames = collectCaseNames();
		QJsonArray cases;
		int seq = 1;
		for (const QString& name : caseNames)
		{
			QJsonObject one;
			one.insert(QStringLiteral("seq"), seq++);
			one.insert(QStringLiteral("name"), name);
			//load_type：录入＞默认句（"规则层默认句"来源）＞null（E3-7/E3-11）
			{
				const AutoValue lt = casesLoadType;
				if (!cfg.loadType.isEmpty())
					one.insert(QStringLiteral("load_type"), cfg.loadType);
				else if (lt.source == QStringLiteral("规则层默认句") && !lt.value.isNull())
					one.insert(QStringLiteral("load_type"), lt.value);
				else
					one.insert(QStringLiteral("load_type"), QJsonValue());
			}
			one.insert(QStringLiteral("loads"), collectCaseLoads());
			const QJsonObject entered = cfg.caseResults.value(name);
			const AssemblyResult::CaseScan* scan = scanByCase.value(name, nullptr);
			//results 四键取值序（FR-325）：录入＞自动（扫描值非 null）＞null——键位恒在、
			//未录值 null 走缺项策略（C7/D-4 口径不变）
			auto pick = [&entered, scan](const char* key, const AutoValue& scanVal) -> QJsonValue
			{
				const QJsonValue enteredVal = jsonValueOrNull(entered, QLatin1String(key));
				if (!enteredVal.isNull())
					return enteredVal;   //录入＞自动（E3-7）
				//PA-04：Undefined 判定补全——AutoValue 空值虽经 NSDMI 归一 Null（消根），
				//此处仍双判 isUndefined 防御：QJsonObject::insert(Undefined)=删键，会破坏
				//"键位恒在、未录值 null=二期锚（R3-2）"契约形状
				if (!scanVal.value.isNull() && !scanVal.value.isUndefined())
					return scanVal.value;
				return QJsonValue(QJsonValue::Null);
			};
			static const AutoValue emptyAuto;
			const AutoValue& stress = scan != nullptr ? scan->stressPeak : emptyAuto;
			const AutoValue& disp = scan != nullptr ? scan->dispMax : emptyAuto;
			const AutoValue& buck = scan != nullptr ? scan->buckling : emptyAuto;
			QJsonObject results;
			results.insert(QStringLiteral("stress_peak"), pick("stress_peak", stress));
			results.insert(QStringLiteral("displacement_max"), pick("displacement_max", disp));
			results.insert(QStringLiteral("buckling_mode"), pick("buckling_mode", buck));
			//reserve_factor 工况位：本批不落逐工况扫描值（发射口径=材料级保守侧，
			//materials[].reserve_factor；工况位维持录入值语义——12 §2.3 口径）
			results.insert(QStringLiteral("reserve_factor"),
				jsonValueOrNull(entered, QStringLiteral("reserve_factor")));
			one.insert(QStringLiteral("results"), results);
			cases.append(one);
		}

		//materials[]：二期 FR-201/202 材料通道——name=采集值（D3-1 "default"不过滤、
		//D3-2 min(partId) 定序）、allowable_stress=schema 预留键 null（02 §4.2 不展示）、
		//max_stress/reserve_factor=页签②录入值或 null（引擎行组 grow/shrink 一期已备，
		//materials=[]→表8 表头+0 行组一期行为零变化，08 §2.1 兼容口径）
		QJsonArray materials;
		QString matErr;
		const MaterialList matList = collectMaterialNames(matErr);
		if (!matList.ok && !matErr.isEmpty())
			errs.append(matErr);   //FR-202：双空降级聚合告警恰 1 条，导出继续
		for (const QString& matName : matList.names)
		{
			const QJsonObject vals = cfg.materialValues.value(matName);
			QJsonObject one;
			one.insert(QStringLiteral("name"), matName);
			one.insert(QStringLiteral("allowable_stress"), QJsonValue(QJsonValue::Null));
			one.insert(QStringLiteral("max_stress"),
				jsonValueOrNull(vals, QStringLiteral("max_stress")));
			//reserve_factor：录入＞自动（FR-324 自动计算值=保守侧 limit÷全工况最大峰值；
			//材料级发射口径，12 §2.3）
			{
				const QJsonValue enteredRf =
					jsonValueOrNull(vals, QStringLiteral("reserve_factor"));
				if (!enteredRf.isNull())
					one.insert(QStringLiteral("reserve_factor"), enteredRf);
				else if (autoValues != nullptr
					&& autoValues->materialReserves.contains(matName)
					&& !autoValues->materialReserves.value(matName).value.isNull())
				{
					one.insert(QStringLiteral("reserve_factor"),
						autoValues->materialReserves.value(matName).value);
				}
				else
					one.insert(QStringLiteral("reserve_factor"), QJsonValue());
			}
			materials.append(one);
		}

		//images[]：仅收录有图片路径的槽（key/path/caption 形状完整性——tdrrules 数组元素
		//形状存在性校验）；无图槽不出现在数组→引擎按槽级 missing_policy（U-04 允许缺项）
		QJsonArray images;
		for (auto it = cfg.imagePaths.begin(); it != cfg.imagePaths.end(); ++it)
		{
			const QString path = it.value();
			if (path.isEmpty() || !QFile::exists(path))
				continue;
			QJsonObject img;
			img.insert(QStringLiteral("key"), it.key());
			img.insert(QStringLiteral("path"), path);
			//CB-05：题注空值发 null 不发空串——空串会把模板既有题注文字清空，
			//null 才按该槽 missing_policy 走缺项（与 manual_fields 同口径）
			const QString caption = cfg.imageCaptions.value(it.key());
			img.insert(QStringLiteral("caption"),
				caption.isEmpty() ? QJsonValue() : QJsonValue(caption));
			images.append(img);
		}

		//manual_fields（U-06 单元类型补录/D-24② 对象名）：空值一律填 null 而非空串——
		//空串会被引擎当作绑定值执行替换（清空槽位），null 才走缺项策略（D-24② XXX 保留原文+告警）
		QJsonObject manual;
		//批次 B 优先级合并（E3-7/E3-11）：录入（cfg）＞自动/默认句（装配结果，仅"自动"或
		//"规则层默认句"来源且非 null）＞null。element_type 对象类型=FR-311 自动值
		auto mergedManual = [&manual, &assemblyItem](const char* key,
			const QString& entered, const QJsonValue& autoVal)
		{
			QJsonValue v = entered.isEmpty() ? QJsonValue(QJsonValue::Null)
				: QJsonValue(entered);
			if (v.isNull() && !autoVal.isNull() && !autoVal.isUndefined())
				v = autoVal;
			manual.insert(QLatin1String(key), v);
		};
		{
			const AutoValue constraint = assemblyItem(QStringLiteral("manual_fields.constraint_desc"));
			const AutoValue loadCase = assemblyItem(QStringLiteral("manual_fields.load_case_desc"));
			const AutoValue loadType = assemblyItem(QStringLiteral("manual_fields.load_type"));
			const AutoValue calcMethod = assemblyItem(QStringLiteral("manual_fields.calc_method"));
			const AutoValue elementType = assemblyItem(QStringLiteral("manual_fields.element_type"));
			const AutoValue subject = assemblyItem(QStringLiteral("manual_fields.project_subject"));
			mergedManual("constraint_desc", cfg.constraintDesc, constraint.value);
			mergedManual("load_case_desc", cfg.loadCaseDesc, loadCase.value);
			mergedManual("load_type", cfg.loadType, loadType.value);
			mergedManual("calc_method", cfg.calcMethod, calcMethod.value);
			mergedManual("element_type", cfg.elementType, elementType.value);
			mergedManual("project_subject", cfg.projectSubject, subject.value);
		}

		//conclusion.verdict：D2-2/C-2 材料级 min 口径（09 §0.4/08 §2.3）——本次发射
		//materials[] 中已录 reserve_factor 的最小值（数值，引擎 render_conditional/
		//_eval_when 分支即用，引擎零改动）；无任何录入→null（结论句整句走槽级缺省
		//文案 D-4）。仅统计发射集合（当前模型材料）——历史失配条目不参与推导
		QJsonObject conclusion;
		{
			bool hasRf = false;
			double minRf = 0.0;
			for (const QJsonValue& v : materials)
			{
				const QJsonValue rf = v.toObject().value(QStringLiteral("reserve_factor"));
				if (rf.isDouble())
				{
					const double d = rf.toDouble();
					if (!hasRf || d < minRf)
					{
						minRf = d;
						hasRf = true;
					}
				}
			}
			conclusion.insert(QStringLiteral("verdict"),
				hasRf ? QJsonValue(minRf) : QJsonValue(QJsonValue::Null));
			//conclusion.pass/fail 分支文案（FR-26 措辞激活配套，08 §2.3 三态语义）：
			//容器条件措辞槽 branches use=conclusion.pass/fail 自数据集取文案（BUG-01
			//校验器同口径）——无此键则 verdict 分支命中后仍落槽级 missing_text，三态
			//不可见。文案=一期样例数据集权威字面（htsy 模板原句派生，{manual_fields.
			//project_subject} 全限定引用由引擎 H-14 展开）；对象名未录时句子无法成文
			//→不发射（verdict 分支落 missing_text=一期 D-4 形态，缺项预警已提示补录）。
			//未声明该键族的容器：多余键被引擎忽略（O-7 未知键无消费路径），无害
			//批次 A 遗留占位接线（批次 B 收口，任务项⑤）：conclusion.pass/fail 激活门随
			//自动对象名扩展——对象名=录入（cfg）或装配"自动"值非 null 时均可成文
			//（{manual_fields.project_subject} 引用由上方 mergedManual 注入值解析，引擎
			//H-14 展开链零改动）；对象名完全不可得=句子无法成文→不发射（分支落
			//missing_text=一期 D-4 形态）。未声明该键族的容器：多余键被忽略无害（O-7）
			const AutoValue subjectItem = assemblyItem(QStringLiteral("manual_fields.project_subject"));
			const bool subjectAvailable = !cfg.projectSubject.isEmpty()
				|| (!subjectItem.value.isNull() && subjectItem.value.isString());
			if (subjectAvailable)
			{
				conclusion.insert(QStringLiteral("pass"),
					QStringLiteral("通过有限元计算分析，{manual_fields.project_subject}"
						"在设计载荷下结构稳定性和强度均满足设计要求，计算结果总结如下："));
				conclusion.insert(QStringLiteral("fail"),
					QStringLiteral("通过有限元计算分析，{manual_fields.project_subject}"
						"在设计载荷下强度剩余系数低于 1.0，结构不满足设计要求，需进一步评估。"));
			}
		}

		QJsonObject dataset;
		dataset.insert(QStringLiteral("meta"), meta);
		dataset.insert(QStringLiteral("model_stats"), modelStats);
		dataset.insert(QStringLiteral("cases"), cases);
		dataset.insert(QStringLiteral("materials"), materials);
		dataset.insert(QStringLiteral("images"), images);
		dataset.insert(QStringLiteral("manual_fields"), manual);
		dataset.insert(QStringLiteral("conclusion"), conclusion);
		//missing_policy_override（FR-211/212，Q2-7=A 映射）：页签④三选一值直发射
		//（08 §2.2 映射表：dash/keep 照常导出真覆盖；block=预检拒绝在 GUI 侧、此处
		//发射 block_export 为导出中新缺项的引擎兜底）；空值/未知值不发射=容器语义
		//不变（一期数据集兼容锚，09 §4.1）
		if (isMissingPolicyEnumValue(cfg.missingPolicy))
			dataset.insert(QStringLiteral("missing_policy_override"), cfg.missingPolicy);

		if (!errs.isEmpty())
			err = errs.join(QStringLiteral("\n"));
		return QJsonDocument(dataset);
	}

	bool mStrengthReportCollector::saveViewportScreenshot(const QString& slotKey,
		QWidget* hideDuringGrab, bool watermarkOff, QString& outPath, QString& err, QString* warn)
	{
		outPath.clear();
		err.clear();
		if (warn != nullptr)
			warn->clear();
		//获取当前激活视口（样板 mPostExportWord.cpp:2849 slot_savePictureButton）——09 步1
		QWidget* subwin = getCVM->activatePostViewport();
		if (subwin == nullptr)
		{
			err = QStringLiteral("无激活后处理视口，无法抓取");
			return false;
		}
		if (slotKey.isEmpty())
		{
			err = QStringLiteral("截图槽键为空");
			return false;
		}
		//渲染门面（logo 翻转只调用既有透传接口——渲染库零修改）；不可达=按带水印抓并告警
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (watermarkOff && rend == nullptr && warn != nullptr)
		{
			//09 §0.2 步3 失败（rend 为空）→直接按带水印抓（开关失效告警，不阻断）
			*warn = QStringLiteral("渲染组件不可用，本次按带水印抓取（本次未去除水印）");
		}
		//T-21-③ 首图预热（09 步2）：离屏抓取强制视口完成一次 OpenGL 渲染合成（帧缓冲/DWM
		//后备存储有内容），防首图发黑；不触碰屏幕、不调用 winId（winId 强制本地化致界面
		//闪黑——mPostExportWord.cpp:366-375 先例）
		subwin->grab();
		//C-5：RAII 守卫包络（09 步3/9/10）——构造=按开关期望态复位（无水印档即翻转隐藏），
		//析构=恢复可见+update()；守卫声明于全部早退路径之前，任何 return 均经析构收口
		const LogoFlipGuard guard(watermarkOff, rend, subwin);
		if (watermarkOff && rend != nullptr)
		{
			//09 步5-6：翻转后显式 update+processEvents 保险一轮——保证至少一次含翻转的
			//paintGL 合成进 DWM（setLogoVisible 生效条件：翻转后必须发生一次 paintGL）
			subwin->update();
			QApplication::processEvents();
		}
		//T-21-② 防遮挡时序（09 步4/8，B14）：grabWindow 抓屏幕合成像素，对话框与视口矩形
		//重叠必入图——先隐对话框→处理一轮事件循环（等真正从屏幕消失）→抓→恢复
		const bool needRestore = hideDuringGrab != nullptr && hideDuringGrab->isVisible();
		if (needRestore)
		{
			hideDuringGrab->hide();
			QApplication::processEvents();
			//DWM 合成滞后（D-4 同源）：hide 指令到合成画面真正消失有时差，
			//单轮 processEvents 后立即抓屏会把本弹窗截入画面（2026-11-16 用户实测）——
			//等待一拍再抓；抓图路径总时长增量可忽略（单槽一次性操作）
			QEventLoop hideLoop;
			QTimer::singleShot(150, &hideLoop, &QEventLoop::quit);
			hideLoop.exec();
		}
		//裁剪基准=渲染控件本体：REBUILD_PostRend 继承 mBaseRend(QWidget)，其全局几何才是
		//可见 GL 视口；activatePostViewport 返回的容器在单视口重构后几何不代表可见视口
		//（2026-11-16 用户实测裁剪偏移实证，crop 尺寸恒 640×480）。rend 为空回退容器
		QWidget* grabHost = (rend != nullptr) ? rend : subwin;
		//截图（09 步7）：抓取桌面整屏后裁剪出视口区域（回移 c4426b875 方案）。grabWindow(0)
		//拿到的是 DWM 合成后的最终画面（带水印档含两层 logo 水印，与运行时显示一致；无水印
		//档此刻屏幕为 logo 已隐藏的合成帧），且不需要目标窗口原生句柄，不会触发 winId()
		//本地化致界面闪黑。B20：QScreen 取目标窗口所在屏（Qt 5.13 无 QWidget::screen()，
		//用 screenAt 按窗口全局坐标定位，双屏不同缩放场景与目标窗口所在屏一致；取不到退回主屏兜底）
		QScreen* screen = QGuiApplication::screenAt(grabHost->mapToGlobal(QPoint(0, 0)));
		if (screen == nullptr)
			screen = QGuiApplication::primaryScreen();
		QPixmap fullScreen = screen->grabWindow(0);
		QPoint topLeft = grabHost->mapToGlobal(QPoint(0, 0));
		qreal dpr = fullScreen.devicePixelRatio();
		QRect pixelRect(static_cast<int>(topLeft.x() * dpr), static_cast<int>(topLeft.y() * dpr),
			static_cast<int>(grabHost->width() * dpr), static_cast<int>(grabHost->height() * dpr));
		QPixmap picture = fullScreen.copy(pixelRect);
		if (needRestore)
		{
			hideDuringGrab->show();
			QApplication::processEvents();
		}
		if (picture.isNull())
		{
			//C-5 早退路径：守卫析构已完成 logo 恢复+update()
			err = QStringLiteral("视口抓取失败（画面为空）");
			return false;
		}
		//落盘目录（B17 层级）：appDir/Report/strength-report-images/<项目标识>/<slotKey>/
		//（独立于既有 post-images，E10 防互染；项目标识空=视口已关，落 default 兜底）
		QString projectId = currentProjectId();
		if (projectId.isEmpty())
			projectId = QStringLiteral("default");
		const QString baseDir = QCoreApplication::applicationDirPath()
			+ QStringLiteral("/Report/strength-report-images/%1/%2").arg(projectId, slotKey);
		QDir dir(baseDir);
		if (!dir.exists())
			dir.mkpath(baseDir);
		//PNG 固定格式（A-07）+跨重名计数（样板 mPostExportWord.cpp:513-522）
		const QStringList namelist = dir.entryList(QStringList{ QStringLiteral("*.png") }, QDir::Files);
		int namei = namelist.size() + 1;
		QString filename = QStringLiteral("pic_%1.png").arg(namei);
		while (namelist.contains(filename))
		{
			++namei;
			filename = QStringLiteral("pic_%1.png").arg(namei);
		}
		outPath = baseDir + QStringLiteral("/") + filename;
		if (!picture.save(outPath, "PNG"))
		{
			//C-5 早退路径：守卫析构已完成 logo 恢复+update()
			err = QStringLiteral("截图保存失败：%1").arg(outPath);
			outPath.clear();
			return false;
		}
		return true;
		//函数尾=LogoFlipGuard 析构收口（09 步9/10：恢复 logo 可见+update 恢复像素）——
		//正常/早退路径统一经此，无遗漏恢复点（C-5）
	}

	void mStrengthReportCollector::restoreViewportLogo()
	{
		//C-5 兜底复位（E2.5）：主对话框关闭/导出完成调用一次——防异常残留把产品水印
		//长期丢失（水印是对外交付观感承诺）。渲染门面不在位=无事可做
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr)
			return;
		rend->setLogoVisible(true);
		rend->update();   //请求重绘恢复屏幕合成像素（异步，不阻塞调用方）
	}
}
