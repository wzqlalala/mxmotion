#include "mStrengthReportConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

namespace StrengthReport
{
	//绑定键人话字典（00-绑定键整体方案 §4；键清单以 tdr_digest.py 键类型表与 Config
	//契约为准，说明措辞=UI-05 B 类模板原词——不新造术语）。case.results.* 兼容带
	//工况段的逐槽绑定形态（case.results.<工况>.<键>）按尾段查表；查不到返回空串
	namespace
	{
		//绑定键人话字典本体（bindingKeyDescription/bindingKeyKeys 同一数据源，勿复制）
		const QMap<QString, QString>& bindingKeyDict()
		{
			static const QMap<QString, QString> dict = {
			{ QStringLiteral("model_stats.software_name"), QStringLiteral("软件名称") },
			{ QStringLiteral("model_stats.node_count"), QStringLiteral("节点数") },
			{ QStringLiteral("model_stats.element_count"), QStringLiteral("单元数") },
			{ QStringLiteral("case.load_type"), QStringLiteral("载荷类型") },
			{ QStringLiteral("case.results.stress_peak"), QStringLiteral("应力峰值") },
			{ QStringLiteral("case.results.displacement_max"), QStringLiteral("位移最大值") },
			{ QStringLiteral("case.results.buckling_mode"), QStringLiteral("失稳模态阶次") },
			{ QStringLiteral("case.results.reserve_factor"), QStringLiteral("剩余系数") },
			{ QStringLiteral("manual_fields.element_type"), QStringLiteral("单元类型") },
			{ QStringLiteral("manual_fields.constraint_desc"), QStringLiteral("约束形式描述") },
			{ QStringLiteral("manual_fields.load_case_desc"), QStringLiteral("加载形式描述") },
			{ QStringLiteral("manual_fields.calc_method"), QStringLiteral("计算方法") },
			{ QStringLiteral("manual_fields.project_subject"), QStringLiteral("对象名/报告标题") },
			};
			return dict;
		}
	}

	//绑定键人话字典（00-绑定键整体方案 §4；键清单以 tdr_digest.py 键类型表与 Config
	//契约为准，说明措辞=UI-05 B 类模板原词——不新造术语）。case.results.* 兼容带
	//工况段的逐槽绑定形态（case.results.<工况>.<键>）按尾段查表；查不到返回空串
	QString bindingKeyDescription(const QString& bindKey)
	{
		const QMap<QString, QString>& dict = bindingKeyDict();
		if (dict.contains(bindKey))
			return dict.value(bindKey);
		if (bindKey.startsWith(QStringLiteral("case.results.")))
		{
			const QStringList segs = bindKey.split(QLatin1Char('.'));
			if (segs.size() > 3)
				return dict.value(QStringLiteral("case.results.") + segs.last());
		}
		return QString();
	}

	//字典全部键清单（与 bindingKeyDescription 同源；步③建议键名格下拉候选）
	QStringList bindingKeyKeys()
	{
		return bindingKeyDict().keys();
	}

	namespace
	{
		//E2.2 录入值合法性判据（与页签②逐字段校验同口径，08 §2.3/09 §4.5）：
		//应力/位移=非负数值；剩余系数=正数；失稳模态阶次=正整数字符串（数据字典 string）
		bool saneNonNegative(const QJsonValue& v)
		{
			return v.isDouble() && v.toDouble() >= 0.0;
		}

		bool sanePositive(const QJsonValue& v)
		{
			return v.isDouble() && v.toDouble() > 0.0;
		}

		bool saneBuckling(const QJsonValue& v)
		{
			if (v.isNull())
				return true;
			if (!v.isString())
				return false;
			const QString s = v.toString().trimmed();
			if (s.isEmpty())
				return true;   //空串按未录清洗为 null
			static const QRegularExpression re(QStringLiteral("^[0-9]+$"));
			if (!re.match(s).hasMatch())
				return false;
			return s.toLongLong() > 0;
		}

		//单条历史录入清洗（E2.2：非法值→null；未知键剥离；输出恒为完整契约键位，
		//缺失键补 null——未录属正常形态不计清洗）。*changed=是否存在非法值/未知键
		QJsonObject sanitizeEntry(const QJsonObject& in,
			const QStringList& numericNonNegKeys, const QStringList& numericPosKeys,
			const QStringList& textKeys, bool* changed)
		{
			QStringList known;
			for (const QString& k : numericNonNegKeys)
				known.append(k);
			for (const QString& k : numericPosKeys)
				known.append(k);
			for (const QString& k : textKeys)
				known.append(k);
			bool dirty = false;
			for (auto it = in.begin(); it != in.end(); ++it)
			{
				if (!known.contains(it.key()))
					dirty = true;   //未知键剥离
			}
			QJsonObject out;
			for (const QString& key : numericNonNegKeys)
			{
				const QJsonValue v = in.value(key);
				const bool sane = saneNonNegative(v);
				out.insert(key, sane ? QJsonValue(v.toDouble()) : QJsonValue(QJsonValue::Null));
				dirty = dirty || (!sane && !v.isNull() && !v.isUndefined());
			}
			for (const QString& key : numericPosKeys)
			{
				const QJsonValue v = in.value(key);
				const bool sane = sanePositive(v);
				out.insert(key, sane ? QJsonValue(v.toDouble()) : QJsonValue(QJsonValue::Null));
				dirty = dirty || (!sane && !v.isNull() && !v.isUndefined());
			}
			for (const QString& key : textKeys)
			{
				const QJsonValue v = in.value(key);
				const bool sane = saneBuckling(v);
				const QString s = v.isString() ? v.toString().trimmed() : QString();
				//空/全空白串归一 null（空串会被引擎当绑定值清槽——CB-08/CB-05 同口径）；
				//非法形态→null
				out.insert(key, (sane && !s.isEmpty())
					? QJsonValue(s) : QJsonValue(QJsonValue::Null));
				dirty = dirty || (!sane && !v.isNull() && !v.isUndefined());
			}
			if (changed != nullptr)
				*changed = dirty;
			return out;
		}
	}
QString mStrengthReportProjectConfig::sanitizeProjectId(const QString& raw)
{
	//CB-13：净化规则唯一落点（Collector::currentProjectId 与本函数共用——目录名必须一致）。
	//非法字符替换 '_'；结尾点剥离；段内点保留（与既有截图目录 <项目标识> 形态对齐，
	//旧 Config 把段内点也替换 '_' 的口径作废——以截图目录 v1.2 形态为准统一）
	QString id = raw;
	static const QString illegal = "\\/:*?\"<>|";
	for (QChar& ch : id)
	{
		if (illegal.contains(ch))
			ch = QChar::fromLatin1('_');
	}
	//L6-04：trim 与结尾点剥离循环至不动点（trim 先于剥点时 "A ." → "A " 残留尾空格，
	//Win32 目录创建剥尾空格后与项目 "A" 互串——恰为 L5-04 要堵的共撞面）
	while (true)
	{
		const QString prev = id;
		id = id.trimmed();   //L5-04：首尾空白剥离（Win32 文件系统剥尾空格——"A "与"A"目录共撞致配置互串，与结尾点剥离同口径合并）
		while (id.endsWith(QLatin1Char('.')))
			id.chop(1);
		if (id == prev)
			break;
	}
	return id;
}

QString mStrengthReportProjectConfig::configFilePath() const
{
	//项目标识做文件名净化（sanitizeProjectId 唯一实现——CB-13 两处一致）
	QString id = sanitizeProjectId(_projectId);
	if (id.isEmpty())
		id = QStringLiteral("default");
		//B10：appDir/Report/ 既有习惯区（同 post-images 目录习惯）；D-12：路径带项目标识
		return QCoreApplication::applicationDirPath()
			+ QStringLiteral("/Report/strength-report/%1/project_config.json").arg(id);
	}

	bool mStrengthReportProjectConfig::load(QString& err)
	{
		err.clear();
		const QString path = configFilePath();
		QFile file(path);
		if (!file.exists())
		{
			//PA-01：无配置文件（首次使用/无配置新项目）≠"保持现状"——显式重置为默认
			//空配置再成功返回。单例对话框跨项目切换（CB-01）落到无文件分支时，残留旧
			//项目字段会：containerPath 触发自动加载 A 的容器、页签①回显 A 的录入值、
			//导出经 collectUiToConfig 把 A 的值持久化写入 B 的 project_config.json
			//（"A 串 B"并入持久化）。与下方损坏回退分支同构：重置前暂存项目标识
			//（BUG-05 同款——重置后 setProjectId(_projectId) 会取到空标识落 default/ 目录）
			const QString keepProjectId = _projectId;
			*this = mStrengthReportProjectConfig();
			setProjectId(keepProjectId);   //回退空配置后仍保留当前项目标识（D-12）
			return true;   //首次使用：空配置成功返回
		}
		if (!file.open(QIODevice::ReadOnly))
		{
			//R3P-02：读取失败分支与无文件/损坏分支同构重置——切项目（CB-01）时 B 的
			//存量配置被独占锁/ACL 拒读场景下，A 项目字段若残留会经回显/自动重校验/
			//导出落盘三链串入 B 会话（PA-01 同类"A 串 B"，后果同级）。keepProjectId
			//暂存（BUG-05 同款：重置后 setProjectId(_projectId) 取到空标识落 default/）
			const QString keepProjectId = _projectId;
			*this = mStrengthReportProjectConfig();
			setProjectId(keepProjectId);
			err = QStringLiteral("项目配置读取失败：%1（%2），已回退空配置")
					  .arg(path, file.errorString());
			return false;
		}
		const QByteArray data = file.readAll();
		file.close();

		QJsonParseError parseErr;
		const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
		//H-15：配置损坏→回退空配置+err 告警，不阻断对话框打开（调用方仅提示不阻止）
		if (parseErr.error != QJsonParseError::NoError || !doc.isObject())
		{
			err = QStringLiteral("项目配置损坏（%1：%2），已回退空配置")
					  .arg(path, parseErr.errorString());
			//BUG-05：重置前暂存项目标识——*this 重置后 _projectId 已为空值，
			//旧写法 setProjectId(_projectId) 取到空标识，后续 save 落 default/
			//目录，D-12 项目隔离在该损坏回退场景失效
			const QString keepProjectId = _projectId;
			*this = mStrengthReportProjectConfig();
			setProjectId(keepProjectId);   //回退空配置后仍保留当前项目标识（D-12）
			return false;
		}
		const QJsonObject obj = doc.object();
		lastSanitizedCount = 0;
		containerPath = obj.value(QStringLiteral("containerPath")).toString();
		outputPath = obj.value(QStringLiteral("outputPath")).toString();
		softwareName = obj.value(QStringLiteral("softwareName")).toString();
		constraintDesc = obj.value(QStringLiteral("constraintDesc")).toString();
		loadCaseDesc = obj.value(QStringLiteral("loadCaseDesc")).toString();
		loadType = obj.value(QStringLiteral("loadType")).toString();
		calcMethod = obj.value(QStringLiteral("calcMethod")).toString();
		elementType = obj.value(QStringLiteral("elementType")).toString();
		projectSubject = obj.value(QStringLiteral("projectSubject")).toString();
		missingPolicy = obj.value(QStringLiteral("missingPolicy")).toString();
		const QJsonObject paths = obj.value(QStringLiteral("imagePaths")).toObject();
		imagePaths.clear();
		for (auto it = paths.begin(); it != paths.end(); ++it)
			imagePaths.insert(it.key(), it.value().toString());
		const QJsonObject captions = obj.value(QStringLiteral("imageCaptions")).toObject();
		imageCaptions.clear();
		for (auto it = captions.begin(); it != captions.end(); ++it)
			imageCaptions.insert(it.key(), it.value().toString());
		//二期批次 B：两组录入面装载（FR-241）——E2.2 非法历史值清洗为 null+计数
		//（黄字提示经页签②消息行呈现，不阻断）
		const QStringList caseNumKeys{ QStringLiteral("stress_peak"),
			QStringLiteral("displacement_max") };
		const QStringList casePosKeys{ QStringLiteral("reserve_factor") };
		const QStringList caseTextKeys{ QStringLiteral("buckling_mode") };
		const QJsonObject caseRes = obj.value(QStringLiteral("caseResults")).toObject();
		caseResults.clear();
		for (auto it = caseRes.begin(); it != caseRes.end(); ++it)
		{
			bool changed = false;
			caseResults.insert(it.key(), sanitizeEntry(it.value().toObject(),
				caseNumKeys, casePosKeys, caseTextKeys, &changed));
			if (changed)
				++lastSanitizedCount;
		}
		const QStringList matNumKeys{ QStringLiteral("max_stress") };
		const QStringList matPosKeys{ QStringLiteral("reserve_factor") };
		const QJsonObject matVals = obj.value(QStringLiteral("materialValues")).toObject();
		materialValues.clear();
		for (auto it = matVals.begin(); it != matVals.end(); ++it)
		{
			bool changed = false;
			materialValues.insert(it.key(), sanitizeEntry(it.value().toObject(),
				matNumKeys, matPosKeys, QStringList(), &changed));
			if (changed)
				++lastSanitizedCount;
		}
		//二期批次 C：去水印/试点/槽位映射/逐槽水印追溯装载（FR-231/FR-221/D3-3）。
		//映射帧标识非正整数的历史脏值直接丢弃（防御非法值入对齐抓取链——E2.2 同精神）
		watermarkOff = obj.value(QStringLiteral("watermarkOff")).toBool(false);
		semiAutoGrab = obj.value(QStringLiteral("semiAutoGrab")).toBool(false);
		slotFrameMap.clear();
		const QJsonObject frames = obj.value(QStringLiteral("slotFrameMap")).toObject();
		for (auto it = frames.begin(); it != frames.end(); ++it)
		{
			const int f = it.value().toInt(-1);
			if (f > 0)
				slotFrameMap.insert(it.key(), f);
		}
		imageWatermarks.clear();
		const QJsonObject wmMarks = obj.value(QStringLiteral("imageWatermarks")).toObject();
		for (auto it = wmMarks.begin(); it != wmMarks.end(); ++it)
		{
			if (it.value().isBool())
				imageWatermarks.insert(it.key(), it.value().toBool());
		}
		//三期批次 B（13 §4.2）：slotMappings 四元组装载（批次 C 消费面；批次 B 先落配置面）。
		//条目须为对象且 frame>0（缺 var/compo=帧维映射合法——批次 C 帧维应用语义）；
		//非法条目丢弃不阻断（E2.2 同精神）。O3-2 迁移：旧 semiAutoGrab=true 且无 autoCapture
		//键→迁移 true（开关合一，R3-9 行为语义变化点登记）
		slotMappings.clear();
		lastMappingDroppedCount = 0;   //LG-07：丢弃计数随本次 load 重置
		const QJsonObject mappings = obj.value(QStringLiteral("slotMappings")).toObject();
		for (auto it = mappings.begin(); it != mappings.end(); ++it)
		{
			const QJsonObject quad = it.value().toObject();
			if (quad.isEmpty())
				continue;   //空对象=该槽未映射语义（非丢弃，不计数）
			//LG-01（UB-03 save↔load 往返不对称）：合法条目两类——
			//①frame>0（任意四元组，原有口径）；②var=SolidColor 网格图伪变量无帧 quad
			//（采纳建议写入形态，帧维缺省=抓当前帧——消费面 mappingsFromConfig 的
			//frameOnly/frameId<=0 语义既有定义 Cap:106-107/325-332）。"采纳才落、落了
			//就在"——合法采纳映射不得跨会话被 frame>0 过滤静默丢弃；非法帧号仍清洗丢弃
			//（E2.2 同精神，防御非法值入切帧链）+丢弃计数黄字提示（LG-07）
			const bool frameOk = quad.value(QStringLiteral("frame")).toInt(-1) > 0;
			const bool solidNoFrame = quad.value(QStringLiteral("var")).toString()
				.compare(QStringLiteral("SolidColor"), Qt::CaseInsensitive) == 0;
			if (frameOk || solidNoFrame)
				slotMappings.insert(it.key(), quad);
			else
				++lastMappingDroppedCount;
		}
		const QJsonValue autoCaptureVal = obj.value(QStringLiteral("autoCapture"));
		if (autoCaptureVal.isBool() || autoCaptureVal.isDouble())
			autoCapture = autoCaptureVal.toBool(false);
		else if (semiAutoGrab)
			autoCapture = true;   //O3-2：旧试点开关迁移（一次性；save 起新键为准）。L5-03：键存在但为 null/非数值布尔（手编配置）按类型门视为未提供——归一化默认值，迁移不被压制
		return true;
	}

	bool mStrengthReportProjectConfig::save(QString& err)
	{
		err.clear();
		const QString path = configFilePath();
		const QString dir = QFileInfo(path).absolutePath();
		if (!QDir().mkpath(dir))
		{
			err = QStringLiteral("项目配置目录创建失败：%1").arg(dir);
			return false;
		}
		QJsonObject obj;
		obj.insert(QStringLiteral("containerPath"), containerPath);
		obj.insert(QStringLiteral("outputPath"), outputPath);
		obj.insert(QStringLiteral("softwareName"), softwareName);
		obj.insert(QStringLiteral("constraintDesc"), constraintDesc);
		obj.insert(QStringLiteral("loadCaseDesc"), loadCaseDesc);
		obj.insert(QStringLiteral("loadType"), loadType);
		obj.insert(QStringLiteral("calcMethod"), calcMethod);
		obj.insert(QStringLiteral("elementType"), elementType);
		obj.insert(QStringLiteral("projectSubject"), projectSubject);
		obj.insert(QStringLiteral("missingPolicy"), missingPolicy);
		QJsonObject paths;
		for (auto it = imagePaths.begin(); it != imagePaths.end(); ++it)
			paths.insert(it.key(), it.value());
		obj.insert(QStringLiteral("imagePaths"), paths);
		QJsonObject captions;
		for (auto it = imageCaptions.begin(); it != imageCaptions.end(); ++it)
			captions.insert(it.key(), it.value());
		obj.insert(QStringLiteral("imageCaptions"), captions);
		//二期批次 B：两组录入面持久化（FR-241/D-12——按项目隔离落盘）
		QJsonObject caseRes;
		for (auto it = caseResults.begin(); it != caseResults.end(); ++it)
			caseRes.insert(it.key(), it.value());
		obj.insert(QStringLiteral("caseResults"), caseRes);
		QJsonObject matVals;
		for (auto it = materialValues.begin(); it != materialValues.end(); ++it)
			matVals.insert(it.key(), it.value());
		obj.insert(QStringLiteral("materialValues"), matVals);
		//二期批次 C：去水印/试点/槽位映射/逐槽水印追溯持久化（FR-231/FR-221）
		obj.insert(QStringLiteral("watermarkOff"), watermarkOff);
		obj.insert(QStringLiteral("semiAutoGrab"), semiAutoGrab);
		QJsonObject frames;
		for (auto it = slotFrameMap.begin(); it != slotFrameMap.end(); ++it)
			frames.insert(it.key(), it.value());
		obj.insert(QStringLiteral("slotFrameMap"), frames);
		QJsonObject wmMarks;
		for (auto it = imageWatermarks.begin(); it != imageWatermarks.end(); ++it)
			wmMarks.insert(it.key(), it.value());
		obj.insert(QStringLiteral("imageWatermarks"), wmMarks);
		//三期批次 B：slotMappings 四元组+autoCapture 总开关持久化（13 §4.2；autoCapture
		//以新键为准——O3-2 开关合一迁移后旧 semiAutoGrab 仅作读兼容保留）
		QJsonObject mappings;
		for (auto it = slotMappings.begin(); it != slotMappings.end(); ++it)
			mappings.insert(it.key(), it.value());
		obj.insert(QStringLiteral("slotMappings"), mappings);
		obj.insert(QStringLiteral("autoCapture"), autoCapture);

		const QJsonDocument doc(obj);
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			err = QStringLiteral("项目配置写入失败：%1（%2）").arg(path, file.errorString());
			return false;
		}
		file.write(doc.toJson(QJsonDocument::Indented));
		file.close();
		return true;
	}
}
