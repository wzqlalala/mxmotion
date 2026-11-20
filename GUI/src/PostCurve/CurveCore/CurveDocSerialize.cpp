#include "CurveDocSerialize.h"
#include "ChartTypeProfile.h"
#include "CurveDocument.h"

//Qt
#include <QSet>

namespace PostCurve
{
	namespace
	{
		//快照标识与版本（独立于 mxses 文件版本：曲线块自身的演进开关）。
		//V2：Curve 节点 series 追加 curveKind（曲线类别，曲线统计按类别过滤；
		//V1 快照无该字段，读入保持缺省 Normal）
		const quint32 CURVE_DOC_SNAPSHOT_MAGIC = 0x43445331;   //'CDS1'
		//V3：Chart 追加三维图轴组 axis3d（R28 入树）、Curve 追加 zData/plot3dType
		//（V2 起有 curveCategory；V1 快照无追加字段，读入保持缺省）
		//V4：Curve 追加三维曲线外观与着色三键 areaFillOpacity/contour/contourVar
		//（R36；V3 及更早快照无此段，读入保持缺省——旧 .mxses 读取零损伤兼容红线）
		//V5：Curve 追加色带类显示模式 drawStyle（R38，仅曲面消费；V4 及更早快照
		//无此段读入保持缺省 0=引擎默认，旧 .mxses 读取零损伤兼容红线）
		//V6：Chart 追加注释图表级总开关 annotationsVisible、Annotation 追加
		//显示边框 borderShow（R42 二期；写侧在既有字段落串之后恒追写 quint8，
		//读侧按版本门控——V5 及更早快照无此段读入保持缺省 true，旧 .mxses
		//读取零损伤兼容红线。共享 operator<<（Annotation 整结构体）被模板路径
		//复用，字段序冻结不得追加，故走追写段；模板路径（V2 字段序冻结）不含
		//新字段，模板读入新字段保持缺省 true）
		//V7：Chart 追加三维基准面图表级总开关 datum3DVisible、新增 DatumPlane3D
		//节点类型分支（R41 三维基准面，全字段无损往返）。datum3DVisible 写侧恒
		//追写 quint8、读侧版本门控（V6 及更早快照无此段读入保持缺省 true）；新增
		//节点类型分支仅 V7+ 快照可能含该类型节点（旧写入端无法产生，无需在分支
		//内再做版本判定）；类型上界校验（readNode/readTemplateNode 两处）同步更新。
		//模板路径本期不纳入基准面：writeTemplateNode 无 DatumPlane3D 分支（落到
		//default 只写 id/type/name/子树），readTemplateNode 仅放行类型不读字段
		//（恢复出厂默认）——模板 V2 字段序冻结零波及
		//V8：Annotation 追加三维锚点 posZ（R42 修订——注释锚定三维数据空间，
		//{X}{Y}{Z}=注释自身位置；写侧在 V6 borderShow 之后恒追写 double，读侧
		//版本门控——V7 及更早快照无此段读入保持缺省 0，旧 .mxses 读取零损伤
		//兼容红线；模板路径不含新字段，模板读入保持缺省 0）
		//V9：Chart 三维图轴组逐轴追加刻度类型 scaleType（R43 三维轴参数栏翻活，
		//0 Linear/1 log10/2 dB20/3 dB10 值表同二维；写侧 writeChart3DAxis 末尾
		//逐轴恒追写 qint32，读侧版本门控——V8 及更早快照无此段读入保持缺省 0
		//线性零回归，旧 .mxses 读取零损伤兼容红线；模板路径 V3 同款门控，
		//V2 及更早模板读入保持缺省 0）
		//V10：Chart 三维图轴组追加图表级三元组 gridStyle/backdropColor/
		//backdropGridColor（R44 三维图网格样式翻活：0 三维框/1 线框+背景面
		//底填充/网格线两色，默认=现观感等效 0/(240,240,240)/(191,191,191)；
		//写侧 writeChart3DAxis 逐轴段后恒追写，读侧在 R43 双参门控机制内
		//再套阈值——V9 及更早快照无此段读入保持默认零回归，旧 .mxses 读取
		//零损伤兼容红线；模板路径 V4 同款门控，V3 及更早模板读入保持默认）
		//V11：Chart 节点 header/footer 追加 emphLine（表头强调行，需求02渲染层级；
		//V10 及更早快照无该字段，读入保持缺省 -1=无强调）。并档注记：emphLine 在
		//dev_base 侧曾占 V3 槽位（wb 侧旧口径，与上游 axis3d 口径相撞）、其并档
		//起挪 V5，又与本侧 drawStyle 的 V5 槽位相撞——本并档再挪 V11，旧口径
		//（V3/V5）快照按本并档代码解析会失败，仅涉开发机本地会话
		//V12：Axis 节点尾部追加 categoryLabelAlign（类别轴标签对齐 qint32，
		//2026-11-12 D25/L14；V11 及更早快照无此段，读入保持缺省 0=水平，越界值
		//钳 0..3）。并档注记：dev_base 侧原占 V6 槽位，与本侧 annotationsVisible/
		//borderShow 的 V6 槽位相撞，本并档挪 V12——旧口径快照仅涉开发机本地会话
		//V13：Curve 追加三维曲线定义溯源段（z3DExpression + 三侧文件分量列下标
		//x3DComponent/y3DComponent/z3DComponent，"定义曲线"三维弹窗打开载入既有
		//曲线的往返字段；写侧在 V5 drawStyle 之后恒追写，读侧版本门控——V12 及
		//更早快照无此段读入保持缺省，旧 .mxses 读取零损伤兼容红线；模板路径
		//（V2 字段序冻结）不含新字段，模板读入保持缺省）
		//V14：Annotation 追加三维框位置三键 boxPosX/boxPosY/boxPosZ（R46——内容
		//锚定注释拖拽框体提交框位、锚点三键不动=牵引线拉长且 {X}{Y}{Z} 稳定对齐
		//二维；NaN=未手动摆位框=锚点+固定偏移；写侧在 V8 posZ 之后恒追写三 double，
		//读侧版本门控——V13 及更早快照无此段读入保持缺省 NaN，旧 .mxses 读取零
		//损伤兼容红线；共享 operator<<（Annotation 整结构体）字段序冻结不追加，
		//模板路径不含新字段，模板读入保持缺省 NaN）
		const quint32 CURVE_DOC_SNAPSHOT_VERSION = 14;

		//三维图轴组落串/读取（会话快照 V3 起追加、模板 V2 起追加；写侧恒当前
		//版本，读侧按版本门控——V2 及更早快照无此段，保持缺省轴状态）。
		//scaleTypeSince=刻度类型追写段（R43）的门控版本阈值——会话快照与模板
		//两路径版本空间独立（会话 V9/模板 V3 起），由调用点按各自语义传入：
		//version 达标才读 scaleType，否则保持缺省 0（旧文件缺省线性零回归）。
		//backdropSince=图表级三元组追写段（R44 gridStyle+背景两色）的门控
		//阈值（会话 V10/模板 V4 起）——同一双参门控机制内再套一层阈值，
		//不达标保持三字段默认值（旧文件缺省=现观感零回归）
		void writeChart3DAxis(QDataStream& out, const CurveChart3DData& d)
		{
			for (int i = 0; i < 3; ++i)
			{
				const CurveAxis3DState& ax = d.axes[i];
				out << ax.label << ax.min << ax.max
					<< qint32(ax.tickCount) << ax.gridVisible << ax.autoFit;
				//V9/模板 V3：刻度类型（R43；写侧恒追写占位保持格式一致）
				out << qint32(ax.scaleType);
			}
			//V10/模板 V4：图表级三元组（R44；写侧恒追写占位保持格式一致；
			//图表级一次非逐轴——gridStyle+背景两色作用全图）
			out << qint32(d.gridStyle) << d.backdropColor << d.backdropGridColor;
		}

		bool readChart3DAxis(QDataStream& in, CurveChart3DData& d,
			quint32 version, quint32 scaleTypeSince, quint32 backdropSince)
		{
			for (int i = 0; i < 3; ++i)
			{
				CurveAxis3DState& ax = d.axes[i];
				qint32 tickCount = 0;
				in >> ax.label >> ax.min >> ax.max >> tickCount
					>> ax.gridVisible >> ax.autoFit;
				ax.tickCount = tickCount;
				//V9/模板 V3 追写字段：越界值钳 0..3 防脏数据进模型（写侧不钳制
				//=页侧保证口径，读侧防御对齐 V5 drawStyle 先例）
				if (version >= scaleTypeSince)
				{
					qint32 scaleType = 0;
					in >> scaleType;
					ax.scaleType = qBound(0, scaleType, 3);
				}
			}
			//V10/模板 V4 追写段（R44 图表级三元组）：阈值不达标（旧文件）整段
			//跳过，三字段保持默认（0/(240,240,240)/(191,191,191)=现观感零回归）；
			//达标读入，gridStyle 越界值钳 0..1、QColor 流算子原样回读（无效色
			//语义=引擎 setter 默认兜底，与属性路径同口径）
			if (version >= backdropSince)
			{
				qint32 gridStyle = 0;
				in >> gridStyle >> d.backdropColor >> d.backdropGridColor;
				d.gridStyle = qBound(0, gridStyle, 1);
			}
			return in.status() == QDataStream::Ok;
		}

		//递归写节点：id/type/name + 按类型生效的数据结构 + 子节点序。
		//Header/Footer 节点的数据实际挂在图表节点上（成员备用），节点侧不写
		void writeNode(QDataStream& out, const CurveNode* n)
		{
			out << qint32(n->id) << qint32(n->type) << n->name;
			switch (n->type)
			{
			case CurveNodeType::Page:
				out << n->page;
				break;
			case CurveNodeType::Chart:
				out << n->chart << n->header << n->footer;
				//V3：三维图轴组（chartType=5 有效，其余类型写默认值占位保持格式一致）
				writeChart3DAxis(out, n->chart.axis3d);
				//V6：注释图表级总开关（R42；写侧恒写 quint8 bool 占位保持格式一致）
				out << quint8(n->chart.annotationsVisible);
				//V7：三维基准面图表级总开关（R41；写侧恒写 quint8 bool 占位保持格式一致）
				out << quint8(n->chart.datum3DVisible);
				//V11：表头/页脚强调行（需求02渲染层级；dev_base 侧原占 V5 槽位与
				//drawStyle 相撞，本并档挪 V11；写侧恒写占位保持格式一致）
				out << qint32(n->header.emphLine);
				out << qint32(n->footer.emphLine);
				break;
			case CurveNodeType::Curve:
				out << n->series;
				//V2：曲线类别（曲线统计的目标过滤字段）
				out << qint32(n->series.curveCategory);
				//V3：三维曲线数据（zData + 渲染序列类型 + 网格行列；非三维曲线写默认值）
				out << n->series.zData << qint32(n->series.plot3dType)
					<< qint32(n->series.plot3dRows) << qint32(n->series.plot3dCols);
				//V4：三维曲线外观与着色（R36 三新键；非三维曲线写出厂默认值，
				//占位保持格式一致——线型/符号/区域填充复用 series 既有字段不走此段）
				out << qint32(n->series.areaFillOpacity) << n->series.contour
					<< qint32(n->series.contourVar);
				//V5：色带类显示模式（R38；非色带曲线写出厂默认值占位保持格式一致）
				out << qint32(n->series.drawStyle);
				//V13：三维曲线定义溯源（"定义曲线"三维弹窗往返；非三维曲线写
				//缺省占位保持格式一致；模板路径不含本段）
				out << n->series.z3DExpression
					<< qint32(n->series.x3DComponent) << qint32(n->series.y3DComponent)
					<< qint32(n->series.z3DComponent);
				break;
			case CurveNodeType::XAxis:
			case CurveNodeType::YAxis:
			case CurveNodeType::YAxis2:
			case CurveNodeType::XAxis2:
				out << n->axis;
				//V12：类别轴标签对齐（dev_base 侧原占 V6 槽位与本侧注释开关相撞，
				//本并档挪 V12；写侧恒写占位保持格式一致）
				out << qint32(n->axis.categoryLabelAlign);
				//V13：列表标注法主刻度值列表（2d-chart-split-align；写侧恒写占位
				//保持格式一致——QVector<double> 逐元素序列化；模板路径不含本段）
				out << n->axis.tickValues;
				break;
			case CurveNodeType::Legend:
				out << n->legend;
				break;
			case CurveNodeType::Baseline:
				out << n->baseline;
				break;
			case CurveNodeType::Annotation:
				out << n->annotation;
				//V6：显示边框（R42 二期；写侧恒写 quint8 bool。共享 operator<<
				//被模板路径复用字段序冻结，新字段只走会话路径追写段）
				out << quint8(n->annotation.borderShow);
				//V8：三维锚点 posZ（R42 修订；写侧恒写 double）
				out << n->annotation.posZ;
				//V14：三维框位置三键（R46；写侧恒写三 double，NaN=未手动摆位）
				out << n->annotation.boxPosX << n->annotation.boxPosY
					<< n->annotation.boxPosZ;
				break;
			case CurveNodeType::DatumPlane3D:
				//V7：三维基准面（R41）全字段整结构落串。新节点类型仅 V7+ 写端产生，
				//无需版本内判定；模板路径无本分支（writeTemplateNode default 只写
				//id/type/name/子树，基准面不入模板）
				out << n->datumPlane3D;
				break;
			default:
				break;
			}
			out << qint32(n->children.size());
			for (const CurveNode* c : n->children)
				writeNode(out, c);
		}

		//递归读节点（失败返回 nullptr 并由调用方统一释放已读部分）。
		//ids 收集快照内已用 id，用于唯一性校验；遇到重复/非法 id 判失败。
		//snapshotVersion 决定版本追加字段的读取（V2 起 Curve 追加 curveKind）
		CurveNode* readNode(QDataStream& in, QSet<qint32>& ids, int* maxId, quint32 snapshotVersion)
		{
			qint32 id = -1;
			qint32 type = 0;
			QString name;
			in >> id >> type >> name;
			if (in.status() != QDataStream::Ok || id <= 0 || ids.contains(id))
				return nullptr;
			//类型上界=词表最后一个枚举（新增节点类型后缀接在尾部，同步更新此处；
			//R28 起上界为 AxisZ3DData——原停在 CurveY2Data 的上界曾误拒新类型；
			//R41 起上界为 DatumPlane3D）
			if (type < static_cast<int>(CurveNodeType::Project)
				|| type > static_cast<int>(CurveNodeType::DatumPlane3D))
				return nullptr;
			ids.insert(id);
			if (id > *maxId)
				*maxId = id;

			CurveNode* n = new CurveNode;
			n->id = id;
			n->type = static_cast<CurveNodeType>(type);
			n->name = name;
			switch (n->type)
			{
			case CurveNodeType::Page:
				in >> n->page;
				break;
			case CurveNodeType::Chart:
				in >> n->chart >> n->header >> n->footer;
				//V3 追加字段：三维图轴组（V2 及更早快照无此段，保持缺省轴状态）；
				//刻度类型段门控阈值=9（R43 V9 起，V8 及更早快照无此段缺省 0）；
				//图表级三元组段门控阈值=10（R44 V10 起，V9 及更早快照三字段缺省）
				if (snapshotVersion >= 3 && !readChart3DAxis(in, n->chart.axis3d, snapshotVersion, 9, 10))
					return nullptr;
				//V6 追加字段：注释图表级总开关（R42；V5 及更早快照无此段保持
				//缺省 true 零行为变化）
				if (snapshotVersion >= 6)
				{
					quint8 annoVis = 1;
					in >> annoVis;
					n->chart.annotationsVisible = (annoVis != 0);
				}
				//V7 追加字段：三维基准面图表级总开关（R41；V6 及更早快照无此段
				//保持缺省 true 零行为变化）
				if (snapshotVersion >= 7)
				{
					quint8 datumVis = 1;
					in >> datumVis;
					n->chart.datum3DVisible = (datumVis != 0);
				}
				//V11 追加字段：表头/页脚强调行（需求02；dev_base 侧原占 V5 槽位
				//与 drawStyle 相撞，本并档挪 V11；V10 及更早快照无此段保持缺省
				//-1=无强调）
				if (snapshotVersion >= 11)
				{
					qint32 eh = -1, ef = -1;
					in >> eh >> ef;
					n->header.emphLine = eh;
					n->footer.emphLine = ef;
				}
				break;
			case CurveNodeType::Curve:
				in >> n->series;
				//V2 追加字段：曲线类别（越界值防御性归为普通，不让脏数据进模型）
				if (snapshotVersion >= 2)
				{
					qint32 kind = 0;
					in >> kind;
					n->series.curveCategory = (kind >= static_cast<qint32>(CurveCategory::Normal)
						&& kind <= static_cast<qint32>(CurveCategory::TargetCurve))
						? static_cast<CurveCategory>(kind) : CurveCategory::Normal;
				}
				//V3 追加字段：三维曲线数据（zData + 渲染序列类型 + 网格行列）
				if (snapshotVersion >= 3)
				{
					qint32 plot3dType = 0, plot3dRows = 0, plot3dCols = 0;
					in >> n->series.zData >> plot3dType >> plot3dRows >> plot3dCols;
					n->series.plot3dType = (plot3dType >= 0 && plot3dType <= 3)
						? plot3dType : 1;
					n->series.plot3dRows = plot3dRows;
					n->series.plot3dCols = plot3dCols;
				}
				//V4 追加字段：三维曲线外观与着色（R36；V3 及更早快照无此段保持
				//缺省——旧 .mxses 读取零损伤兼容红线；越界值钳制回缺省防脏数据进模型）
				if (snapshotVersion >= 4)
				{
					qint32 fillOpacity = 0, contourVar = 0;
					bool contour = false;
					in >> fillOpacity >> contour >> contourVar;
					n->series.areaFillOpacity = qBound(0, fillOpacity, 100);
					n->series.contour = contour;
					n->series.contourVar = (contourVar >= 0 && contourVar <= 2)
						? contourVar : 0;
				}
				//V5 追加字段：色带类显示模式（R38；V4 及更早快照无此段保持缺省 0
				//=引擎默认 StyleSurface 零行为变化；越界值钳 0..2 防脏数据进模型）
				if (snapshotVersion >= 5)
				{
					qint32 drawStyle = 0;
					in >> drawStyle;
					n->series.drawStyle = qBound(0, drawStyle, 2);
				}
				//V13 追加字段：三维曲线定义溯源（V12 及更早快照无此段读入保持
				//缺省——旧 .mxses 读取零损伤兼容红线；分量列下标负值钳 -1 防
				//脏数据进模型，上界不钳——列数随文件形态，载入时对解析缓存界内
				//判定）
				if (snapshotVersion >= 13)
				{
					qint32 xComp = -1, yComp = -1, zComp = -1;
					in >> n->series.z3DExpression >> xComp >> yComp >> zComp;
					n->series.x3DComponent = qMax(xComp, -1);
					n->series.y3DComponent = qMax(yComp, -1);
					n->series.z3DComponent = qMax(zComp, -1);
				}
				break;
			case CurveNodeType::XAxis:
			case CurveNodeType::YAxis:
			case CurveNodeType::YAxis2:
			case CurveNodeType::XAxis2:
				in >> n->axis;
				//V12 追加字段：类别轴标签对齐（dev_base 侧原占 V6 槽位与本侧注释
				//开关相撞，本并档挪 V12；V11 及更早快照无此段保持缺省 0=水平；
				//越界值钳 0..3）
				if (snapshotVersion >= 12)
				{
					qint32 labelAlign = 0;
					in >> labelAlign;
					n->axis.categoryLabelAlign = qBound(0, labelAlign, 3);
				}
				//V13 追加字段：列表标注法主刻度值列表（2d-chart-split-align；
				//V12 及更早快照无此段保持缺省空表=回落自动划分零回归）
				if (snapshotVersion >= 13)
				{
					in >> n->axis.tickValues;
				}
				break;
			case CurveNodeType::Legend:
				in >> n->legend;
				break;
			case CurveNodeType::Baseline:
				in >> n->baseline;
				break;
			case CurveNodeType::Annotation:
				in >> n->annotation;
				//V6 追加字段：显示边框（R42 二期；V5 及更早快照无此段保持缺省
				//true 零行为变化；模板路径不含本段，模板读入同样保持缺省 true）
				if (snapshotVersion >= 6)
				{
					quint8 borderShow = 1;
					in >> borderShow;
					n->annotation.borderShow = (borderShow != 0);
				}
				//V8 追加字段：三维锚点 posZ（R42 修订；V7 及更早快照无此段保持
				//缺省 0 零行为变化；模板路径不含本段，模板读入同样保持缺省 0）
				if (snapshotVersion >= 8)
					in >> n->annotation.posZ;
				//V14 追加字段：三维框位置三键（R46；V13 及更早快照无此段保持
				//缺省 NaN=未手动摆位框=锚点+固定偏移，零行为变化；模板路径不含
				//本段，模板读入同样保持缺省 NaN）
				if (snapshotVersion >= 14)
					in >> n->annotation.boxPosX >> n->annotation.boxPosY
						>> n->annotation.boxPosZ;
				break;
			case CurveNodeType::DatumPlane3D:
				//V7：三维基准面（R41）整结构读入。仅 V7+ 写端会产生本类型节点，
				//无需版本内判定；越界值经 operator>> 钳制（防脏数据进模型）
				in >> n->datumPlane3D;
				break;
			default:
				break;
			}
			qint32 childCount = 0;
			in >> childCount;
			if (in.status() != QDataStream::Ok || childCount < 0 || childCount > 1000000)
			{
				delete n;
				return nullptr;
			}
			for (int i = 0; i < childCount; ++i)
			{
				CurveNode* c = readNode(in, ids, maxId, snapshotVersion);
				if (!c)
				{
					CurveDocument::freeNodeTree(n);
					return nullptr;
				}
				c->parent = n;
				n->children.append(c);
			}
			return n;
		}

		//--- 模板子树（.mxtpl，saveTemplateSubtree/restoreTemplateSubtree 专用） ---
		//magic 5 字节 + formatVersion（对齐 design D1：独立于会话快照版本演进）
		const char TEMPLATE_SUBTREE_MAGIC[] = "MXTPL";
		//V2：Chart 追加三维图轴组、Curve 追加 zData/plot3dType（R28 入树；
		//V1 及更早模板无此段，读入保持缺省——旧模板不存在三维节点天然兼容）
		//V3：Chart 三维图轴组逐轴追加刻度类型 scaleType（R43；写侧恒追写，
		//读侧版本门控——V2 及更早模板无此段读入保持缺省 0 线性零回归，旧
		//模板读取零损伤兼容红线）
		//V4：Chart 三维图轴组追加图表级三元组 gridStyle/backdropColor/
		//backdropGridColor（R44；读侧版本门控——V3 及更早模板无此段读入
		//保持默认 0/(240,240,240)/(191,191,191) 现观感零回归，旧模板读取
		//零损伤兼容红线）
		//V5：Axis 节点尾部追加 categoryLabelAlign（对齐会话快照 V12，2026-11-12
		//D25/L14；V4 及更早模板无此段，读入保持缺省 0=水平）。并档注记：
		//dev_base 侧原占模板 V3 槽位，与本侧 scaleType 的模板 V3 槽位相撞，
		//本并档挪 V5——旧口径模板仅涉开发机本地会话
		const quint32 TEMPLATE_SUBTREE_VERSION = 5;
		//sourceFile 参数化占位符前缀（真实路径含此前缀的概率可忽略）
		const char TEMPLATE_PARAM_PREFIX[] = "@param{";

		//数值（定义式）来源判定：定义即数据，模板原样保留数据体；
		//文件/数学/复数来源数据体置空不写（判定含"定义曲线"落模的
		//"数学运算"/"数值"标记串，见 curveSeriesSourceKind）
		bool isDefinitionCarriedSeries(const CurveSeriesData& s)
		{
			return curveSeriesSourceKind(s) == CurveSourceKind::Numeric;
		}

		//递归写模板节点：与会话快照 writeNode 同构，差异——
		//页 viewPageId 归 -1；Curve 数据体置空（数值来源除外）、sourceFile 参数化
		void writeTemplateNode(QDataStream& out, const CurveNode* n, QStringList* params)
		{
			out << qint32(n->id) << qint32(n->type) << n->name;
			switch (n->type)
			{
			case CurveNodeType::Page:
			{
				CurvePageData p = n->page;
				p.viewPageId = -1;
				out << p;
				break;
			}
			case CurveNodeType::Chart:
				out << n->chart << n->header << n->footer;
				//V2 模板扩展：三维图轴组（非三维图表写默认值占位保持格式一致）
				writeChart3DAxis(out, n->chart.axis3d);
				break;
			case CurveNodeType::Curve:
			{
				//临时拷贝上做参数化与置空，再经既有流算子落串（字段清单单一来源：
				//operator<< 增字段时模板块自动跟随，无需双份维护）。
				//参数化仅对文件来源（"数学运算"/"数值"标记串不是路径，不入参数清单）
				CurveSeriesData s = n->series;
				const CurveSourceKind sourceKind = curveSeriesSourceKind(n->series);
				if (sourceKind == CurveSourceKind::File)
				{
					int idx = params->indexOf(s.sourceFile);
					if (idx < 0)
					{
						idx = params->size();
						params->append(s.sourceFile);
					}
					s.sourceFile = QStringLiteral("@param{%1}").arg(idx);
				}
				if (sourceKind != CurveSourceKind::Numeric)
				{
					s.xData.clear();
					s.yData.clear();
					s.complexYData.clear();
					s.xTickLabels.clear();
				}
				out << s;
				//模板扩展块（CurveSeriesData 流算子未覆盖、模板必带的字段）：
				//类目刻度 + 复数侧数据/定义（复数通道应用时重取数，定义须随模板走）
				out << s.xTickLabels
					<< s.complexYData << qint32(s.complexFormat) << s.complexRef
					<< s.complexSideSubcase << s.complexSideSchema
					<< s.complexSideObject << s.complexSideComponent;
				out << qint32(s.curveCategory);   //曲线类别（对齐会话快照 V2 追加字段）
				//V2 模板扩展：三维曲线数据（zData + 渲染序列类型 + 网格行列；三维
				//曲线数据体随模板走——数值来源定义即数据，文件/数学来源应用时重装配）
				if (sourceKind != CurveSourceKind::Numeric)
					s.zData.clear();
				out << s.zData << qint32(s.plot3dType)
					<< qint32(s.plot3dRows) << qint32(s.plot3dCols);
				break;
			}
			case CurveNodeType::XAxis:
			case CurveNodeType::YAxis:
			case CurveNodeType::YAxis2:
			case CurveNodeType::XAxis2:
				out << n->axis;
				//V5 模板扩展：类别轴标签对齐（dev_base 侧原占模板 V3 槽位与
				//scaleType 相撞，本并档挪 V5；缺省 0=水平）
				out << qint32(n->axis.categoryLabelAlign);
				break;
			case CurveNodeType::Legend:
				out << n->legend;
				break;
			case CurveNodeType::Baseline:
				out << n->baseline;
				break;
			case CurveNodeType::Annotation:
				out << n->annotation;
				break;
			default:
				break;
			}
			out << qint32(n->children.size());
			for (const CurveNode* c : n->children)
				writeTemplateNode(out, c, params);
		}

		//递归读模板节点：与会话快照 readNode 同构（无 maxId——计数器不走块回填），
		//差异——Curve 的 sourceFile 占位符回填（用户参数表第 N 行覆盖，行缺省
		//回填块内保存的原始路径；异常形态保留原文）
		CurveNode* readTemplateNode(QDataStream& in, QSet<qint32>& ids, quint32 version,
			const QStringList& userParams, const QStringList& chunkParams)
		{
			qint32 id = -1;
			qint32 type = 0;
			QString name;
			in >> id >> type >> name;
			if (in.status() != QDataStream::Ok || id <= 0 || ids.contains(id))
				return nullptr;
			//类型上界与会话路径同步（R41 起为 DatumPlane3D）：模板含三维基准面时
			//放行类型但不读字段（下方 switch 无分支，属性恢复出厂默认、仅名称随
			//模板走）——模板路径本期不纳入基准面，字段序冻结零波及（备案）
			if (type < static_cast<int>(CurveNodeType::Project)
				|| type > static_cast<int>(CurveNodeType::DatumPlane3D))
				return nullptr;
			ids.insert(id);

			CurveNode* n = new CurveNode;
			n->id = id;
			n->type = static_cast<CurveNodeType>(type);
			n->name = name;
			switch (n->type)
			{
			case CurveNodeType::Page:
				in >> n->page;
				n->page.viewPageId = -1;   //防御性归位（save 侧已归 -1）
				break;
			case CurveNodeType::Chart:
				in >> n->chart >> n->header >> n->footer;
				//V2 模板扩展：三维图轴组（V1 模板无此段保持缺省）；
				//刻度类型段门控阈值=3（R43 模板 V3 起，V2 及更早模板缺省 0）；
				//图表级三元组段门控阈值=4（R44 模板 V4 起，V3 及更早模板三字段缺省）
				if (version >= 2 && !readChart3DAxis(in, n->chart.axis3d, version, 3, 4))
					return nullptr;
				break;
			case CurveNodeType::Curve:
			{
				in >> n->series;
				if (version >= 1)
				{
					//模板扩展块（与 writeTemplateNode Curve 分支一一对应）
					in >> n->series.xTickLabels
						>> n->series.complexYData;
					qint32 cplxFormat = 0;
					in >> cplxFormat;
					n->series.complexFormat = cplxFormat;
					in >> n->series.complexRef
						>> n->series.complexSideSubcase >> n->series.complexSideSchema
						>> n->series.complexSideObject >> n->series.complexSideComponent;
					qint32 kind = 0;
					in >> kind;
					n->series.curveCategory = (kind >= static_cast<qint32>(CurveCategory::Normal)
						&& kind <= static_cast<qint32>(CurveCategory::TargetCurve))
						? static_cast<CurveCategory>(kind) : CurveCategory::Normal;
				}
				//V2 模板扩展：三维曲线数据（zData + 渲染序列类型 + 网格行列）
				if (version >= 2)
				{
					qint32 plot3dType = 0, plot3dRows = 0, plot3dCols = 0;
					in >> n->series.zData >> plot3dType >> plot3dRows >> plot3dCols;
					n->series.plot3dType = (plot3dType >= 0 && plot3dType <= 3)
						? plot3dType : 1;
					n->series.plot3dRows = plot3dRows;
					n->series.plot3dCols = plot3dCols;
				}
				const QString f = n->series.sourceFile;
				const int prefixLen = int(qstrlen(TEMPLATE_PARAM_PREFIX));
				if (f.startsWith(QLatin1String(TEMPLATE_PARAM_PREFIX)) && f.endsWith(QLatin1Char('}')))
				{
					bool ok = false;
					const int idx = f.mid(prefixLen, f.size() - prefixLen - 1).toInt(&ok);
					if (ok && idx >= 0)
						n->series.sourceFile = (idx < userParams.size())
							? userParams.at(idx)
							: chunkParams.value(idx, f);
				}
				break;
			}
			case CurveNodeType::XAxis:
			case CurveNodeType::YAxis:
			case CurveNodeType::YAxis2:
			case CurveNodeType::XAxis2:
				in >> n->axis;
				//V5 模板扩展：类别轴标签对齐（dev_base 侧原占模板 V3 槽位与
				//scaleType 相撞，本并档挪 V5；V4 及更早模板无此段保持缺省
				//0=水平；越界值钳 0..3）
				if (version >= 5)
				{
					qint32 labelAlign = 0;
					in >> labelAlign;
					n->axis.categoryLabelAlign = qBound(0, labelAlign, 3);
				}
				break;
			case CurveNodeType::Legend:
				in >> n->legend;
				break;
			case CurveNodeType::Baseline:
				in >> n->baseline;
				break;
			case CurveNodeType::Annotation:
				in >> n->annotation;
				break;
			default:
				break;
			}
			qint32 childCount = 0;
			in >> childCount;
			if (in.status() != QDataStream::Ok || childCount < 0 || childCount > 1000000)
			{
				delete n;
				return nullptr;
			}
			for (int i = 0; i < childCount; ++i)
			{
				CurveNode* c = readTemplateNode(in, ids, version, userParams, chunkParams);
				if (!c)
				{
					CurveDocument::freeNodeTree(n);
					return nullptr;
				}
				c->parent = n;
				n->children.append(c);
			}
			return n;
		}

		//统计子树内各命名类型节点数（恢复后命名计数器推进用；数据壳等其余类型不计）
		void countNamingTypes(const CurveNode* n, int* pageCnt, int* chartCnt, int* curveCnt,
			int* baselineCnt, int* annotationCnt, int* datumPlane3DCnt)
		{
			switch (n->type)
			{
			case CurveNodeType::Page: ++(*pageCnt); break;
			case CurveNodeType::Chart: ++(*chartCnt); break;
			case CurveNodeType::Curve: ++(*curveCnt); break;
			case CurveNodeType::Baseline: ++(*baselineCnt); break;
			case CurveNodeType::Annotation: ++(*annotationCnt); break;
			case CurveNodeType::DatumPlane3D: ++(*datumPlane3DCnt); break;
			default: break;
			}
			for (const CurveNode* c : n->children)
				countNamingTypes(c, pageCnt, chartCnt, curveCnt, baselineCnt, annotationCnt, datumPlane3DCnt);
		}

		//--- R29 化石自愈（restoreCurveDocument 恢复后执行；restore 本为直写路径，
		//    自愈直写节点树不进命令栈） ---

		//单个 chartType=5 Chart 节点的化石自愈。R28 中间开发版曾以"直写 chartType
		//绕过 reconcile"产生化石树（chartType=5 + XY 固定骨架 X1/Y1/Header/Footer
		//+ 旧默认图名），被 .mxses 忠实保留。自愈只动两类内容：
		//  1) 删除三维图声明固定骨架之外的图表级固定子类型节点（保留集=
		//     chartTypeProfile(5).fixedChildren 类型表，勿另行硬编码；图表直属子节点
		//     中的固定子只可能是图表级骨架类型——x/y/yp/三维轴壳等数据壳挂在曲线
		//     节点下，故以 isFixedChildNode 判定骨架集合是安全的）；
		//  2) 图表名仍为旧类型默认名（其余注册类型的 defaultChartName 集合）时改为
		//     本类型默认名（对齐 kChartType 骨架协调第 4 项口径）；用户改过名不动。
		//曲线节点（含 x/y/z 轴壳）/Legend/用户节点（基准线/注释）/axis3d/数据一律
		//原样。删除走文档既有直写删除口 removeNodeTree（行信号 + idMap 摘除 +
		//内存释放，与命令栈底层同一机制）；正常三维图两类内容均不命中，零变化（幂等）
		void heal3DFossilChart(CurveDocument* doc, CurveNode* chart)
		{
			const ChartTypeProfile& prof = chartTypeProfile(chart->chart.chartType);
			QVector<CurveNodeType> keepTypes;
			keepTypes.reserve(prof.fixedChildren.size());
			for (const ChartFixedChild& fc : prof.fixedChildren)
				keepTypes.append(fc.type);
			//childNodeIds 返回副本，边遍历边删安全
			for (int childId : doc->childNodeIds(chart->id))
			{
				const CurveNode* c = doc->node(childId);
				if (c && isFixedChildNode(c->type) && !keepTypes.contains(c->type))
					doc->removeNodeTree(childId);
			}
			//旧类型默认名集合：除本类型外各注册类型的 defaultChartName（按类型词表
			//枚举，不硬编码名字符串；含空值防御性跳过）
			QSet<QString> legacyNames;
			const int typeCount = chartTypeDisplayNames().size();
			for (int t = 0; t < typeCount; ++t)
			{
				if (t == chart->chart.chartType)
					continue;
				const QString& legacy = chartTypeProfile(t).defaultChartName;
				if (!legacy.isEmpty())
					legacyNames.insert(legacy);
			}
			if (legacyNames.contains(chart->name))
				chart->name = prof.defaultChartName;
		}

		//遍历恢复子树，对全部 chartType=5 的 Chart 节点做化石自愈（先自愈本图表
		//再下行：heal3DFossilChart 只删固定子不新增图表，图表下也不会再有图表）
		void heal3DFossilCharts(CurveDocument* doc, CurveNode* n)
		{
			if (!n)
				return;
			if (n->type == CurveNodeType::Chart && n->chart.chartType == 5)
				heal3DFossilChart(doc, n);
			for (CurveNode* c : n->children)
				heal3DFossilCharts(doc, c);
		}
	}

	/*************************** 数据结构流算子 ***************************/

	QDataStream& operator<<(QDataStream& out, const CurveDataRefData& ref)
	{
		out << ref.scale << ref.offset << ref.phase << ref.clipEnabled
			<< ref.clipMin << ref.clipMax << ref.expression << qint32(ref.axisAssign) << ref.unit;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveDataRefData& ref)
	{
		qint32 axisAssign = 0;
		in >> ref.scale >> ref.offset >> ref.phase >> ref.clipEnabled
			>> ref.clipMin >> ref.clipMax >> ref.expression >> axisAssign >> ref.unit;
		ref.axisAssign = axisAssign;
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const CurveSeriesData& s)
	{
		out << s.name << s.visible << s.color << qint32(s.lineStyle) << s.lineWidth
			<< s.showSymbol << qint32(s.symbolSize) << s.symbolColor << qint32(s.symbolDensity)
			<< s.symbolScale << s.xData << s.yData << qint32(s.symbolStyle)
			<< s.labelVisible << qint32(s.labelContent) << s.labelText << s.labelPrefix << s.labelSuffix
			<< s.labelPrefixVisible << s.labelSuffixVisible << s.areaFill
			<< s.sourceFile << s.subcase << s.schema << s.object << s.component
			<< s.xSideSubcase << s.xSideSchema << s.xSideObject << s.xSideComponent
			<< s.ySideSubcase << s.ySideSchema << s.ySideObject << s.ySideComponent
			<< s.xRef << s.yRef << s.hiddenRanges;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveSeriesData& s)
	{
		qint32 lineStyle = 0, symbolSize = 0, symbolDensity = 0, symbolStyle = 0, labelContent = 0;
		in >> s.name >> s.visible >> s.color >> lineStyle >> s.lineWidth
			>> s.showSymbol >> symbolSize >> s.symbolColor >> symbolDensity
			>> s.symbolScale >> s.xData >> s.yData >> symbolStyle
			>> s.labelVisible >> labelContent >> s.labelText >> s.labelPrefix >> s.labelSuffix
			>> s.labelPrefixVisible >> s.labelSuffixVisible >> s.areaFill
			>> s.sourceFile >> s.subcase >> s.schema >> s.object >> s.component
			>> s.xSideSubcase >> s.xSideSchema >> s.xSideObject >> s.xSideComponent
			>> s.ySideSubcase >> s.ySideSchema >> s.ySideObject >> s.ySideComponent
			>> s.xRef >> s.yRef >> s.hiddenRanges;
		s.lineStyle = lineStyle;
		s.symbolSize = symbolSize;
		s.symbolDensity = symbolDensity;
		s.symbolStyle = symbolStyle;
		s.labelContent = labelContent;
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const CurveAxisData& a)
	{
		out << a.title << a.titleVisible << a.titleColor << a.visible
			<< qint32(a.scaleMode) << qint32(a.weightMode) << a.autoScale << a.min << a.max
			<< a.titleFont << a.labelFont << a.unit << a.offset
			<< a.majorStep << qint32(a.minorCount) << a.dbRef
			<< qint32(a.labelFormat) << qint32(a.tickGenMode) << a.tickCount
			<< qint32(a.decadeTickCount) << qint32(a.decadeMinorCount) << qint32(a.precision);
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveAxisData& a)
	{
		qint32 scaleMode = 0, weightMode = 0, minorCount = 0, labelFormat = 0, tickGenMode = 0;
		qint32 decadeTickCount = 0, decadeMinorCount = 0, precision = 0;
		in >> a.title >> a.titleVisible >> a.titleColor >> a.visible
			>> scaleMode >> weightMode >> a.autoScale >> a.min >> a.max
			>> a.titleFont >> a.labelFont >> a.unit >> a.offset
			>> a.majorStep >> minorCount >> a.dbRef
			>> labelFormat >> tickGenMode >> a.tickCount
			>> decadeTickCount >> decadeMinorCount >> precision;
		a.scaleMode = scaleMode;
		a.weightMode = weightMode;
		a.minorCount = minorCount;
		a.labelFormat = labelFormat;
		a.tickGenMode = tickGenMode;
		a.decadeTickCount = decadeTickCount;
		a.decadeMinorCount = decadeMinorCount;
		a.precision = precision;
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const CurveLegendData& l)
	{
		out << l.visible << l.autoPosition << qint32(l.position) << l.posX << l.posY
			<< qint32(l.borderStyle) << l.borderWidth << l.borderColor << l.font
			<< qint32(l.textColorMode) << l.textColor << l.bgColor << qint32(l.leaderPos);
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveLegendData& l)
	{
		qint32 position = 0, borderStyle = 0, textColorMode = 0, leaderPos = 0;
		in >> l.visible >> l.autoPosition >> position >> l.posX >> l.posY
			>> borderStyle >> l.borderWidth >> l.borderColor >> l.font
			>> textColorMode >> l.textColor >> l.bgColor >> leaderPos;
		l.position = position;
		l.borderStyle = borderStyle;
		l.textColorMode = textColorMode;
		l.leaderPos = leaderPos;
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const CurveTextData& t)
	{
		out << t.visible << t.text << qint32(t.align) << t.font << t.font2 << t.font3 << t.color;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveTextData& t)
	{
		qint32 align = 0;
		in >> t.visible >> t.text >> align >> t.font >> t.font2 >> t.font3 >> t.color;
		t.align = align;
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const CurveBaselineData& b)
	{
		out << b.label << qint32(b.orientation) << b.value << b.color
			<< qint32(b.lineStyle) << b.lineWidth << b.visible
			<< b.layer << b.labelFont << qint32(b.labelPos) << b.link;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveBaselineData& b)
	{
		qint32 orientation = 0, lineStyle = 0, labelPos = 0;
		in >> b.label >> orientation >> b.value >> b.color
			>> lineStyle >> b.lineWidth >> b.visible
			>> b.layer >> b.labelFont >> labelPos >> b.link;
		b.orientation = orientation;
		b.lineStyle = lineStyle;
		b.labelPos = labelPos;
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const CurveAnnotationData& a)
	{
		out << a.text << a.posX << a.posY << a.color << a.visible
			<< qint32(a.align) << a.font << qint32(a.borderMode) << a.borderColor << a.borderWidth << a.bgColor
			<< a.autoPos << qint32(a.autoAnchor) << a.autoOffset
			<< qint32(a.attachMode) << a.attachCurve << qint32(a.attachIndex)
			<< a.minimized << a.minimizeRadius << a.minimizeColor
			<< a.scalarFmtOn << qint32(a.scalarFmtMode) << qint32(a.scalarFmtPrecision);
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveAnnotationData& a)
	{
		qint32 align = 0, borderMode = 0, autoAnchor = 0, attachMode = 0, attachIndex = 0;
		qint32 scalarFmtMode = 0, scalarFmtPrecision = 0;
		in >> a.text >> a.posX >> a.posY >> a.color >> a.visible
			>> align >> a.font >> borderMode >> a.borderColor >> a.borderWidth >> a.bgColor
			>> a.autoPos >> autoAnchor >> a.autoOffset
			>> attachMode >> a.attachCurve >> attachIndex
			>> a.minimized >> a.minimizeRadius >> a.minimizeColor
			>> a.scalarFmtOn >> scalarFmtMode >> scalarFmtPrecision;
		a.align = align;
		a.borderMode = borderMode;
		a.autoAnchor = autoAnchor;
		a.attachMode = attachMode;
		a.attachIndex = attachIndex;
		a.scalarFmtMode = scalarFmtMode;
		a.scalarFmtPrecision = scalarFmtPrecision;
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const CurvePageData& p)
	{
		out << p.width << p.font << qint32(p.layout) << p.pageTitle
			<< p.publishLayout << p.publishPageTitle << qint32(p.publishContentPos)
			<< qint32(p.viewPageId);
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurvePageData& p)
	{
		qint32 layout = 0, publishContentPos = 0, viewPageId = -1;
		in >> p.width >> p.font >> layout >> p.pageTitle
			>> p.publishLayout >> p.publishPageTitle >> publishContentPos
			>> viewPageId;
		p.layout = layout;
		p.publishContentPos = publishContentPos;
		p.viewPageId = viewPageId;
		return in;
	}

	//三维基准面（R41，会话快照 V7 起 DatumPlane3D 节点专用；模板路径不含本结构
	//——模板 V2 字段序冻结，基准面不入模板）。读侧越界值钳制防脏数据进模型：
	//type 本期唯一合法值 0（下一期扩类型时同步放开上界）、normalAxis 0..2、
	//labelPos 0..8、opacity 0..1
	QDataStream& operator<<(QDataStream& out, const CurveDatumPlane3DData& d)
	{
		out << qint32(d.type) << qint32(d.normalAxis)
			<< d.posX << d.posY << d.posZ << d.text
			<< qint32(d.labelPos) << d.color << d.opacity
			<< d.reverseClip << d.visible;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveDatumPlane3DData& d)
	{
		qint32 type = 0, normalAxis = 0, labelPos = 0;
		in >> type >> normalAxis
			>> d.posX >> d.posY >> d.posZ >> d.text
			>> labelPos >> d.color >> d.opacity
			>> d.reverseClip >> d.visible;
		d.type = (type >= 0 && type <= 0) ? type : 0;
		d.normalAxis = qBound(0, normalAxis, 2);
		d.labelPos = qBound(0, labelPos, 8);
		d.opacity = qBound(0.0, d.opacity, 1.0);
		return in;
	}

	QDataStream& operator<<(QDataStream& out, const CurveChartData& c)
	{
		out << c.visible << qint32(c.chartType) << c.backgroundColor << c.gridColor;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, CurveChartData& c)
	{
		qint32 chartType = 0;
		in >> c.visible >> chartType >> c.backgroundColor >> c.gridColor;
		c.chartType = chartType;
		return in;
	}

	/*************************** 文档快照 ***************************/

	QByteArray saveCurveDocument(const CurveDocument* doc)
	{
		if (!doc)
			return QByteArray();
		const QList<int> pageIds = doc->childNodeIds(doc->rootNodeId());
		if (pageIds.isEmpty())
			return QByteArray();

		QByteArray buffer;
		QDataStream out(&buffer, QIODevice::WriteOnly);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		out.setVersion(QDataStream::Qt_5_13);
#else
		out.setVersion(QDataStream::Qt_5_12);
#endif
		out << CURVE_DOC_SNAPSHOT_MAGIC << CURVE_DOC_SNAPSHOT_VERSION;
		out << qint32(pageIds.size());
		for (int pageId : pageIds)
			writeNode(out, doc->node(pageId));
		//默认编号计数器（节点 id 计数由 maxId 兜底，仍写快照值保持一致）
		const CurveDocument::CurveDocCounters counters = doc->snapshotCounters();
		out << counters.nextId << counters.pageCount << counters.chartCount << counters.curveCount
			<< counters.baselineCount << counters.annotationCount
			<< counters.datumPlane3DCount;   //V7：三维基准面计数（R41；旧版本读端在版本
			                                 //校验处整体拒绝 V7+ 快照，尾部追加零风险）
		return buffer;
	}

	bool restoreCurveDocument(CurveDocument* doc, const QByteArray& chunk)
	{
		if (!doc || chunk.isEmpty())
			return false;
		//防御：文档非空（除根）时 id 空间重叠，整体判失败不动文档
		if (!doc->childNodeIds(doc->rootNodeId()).isEmpty())
			return false;

		QDataStream in(chunk);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		in.setVersion(QDataStream::Qt_5_13);
#else
		in.setVersion(QDataStream::Qt_5_12);
#endif
		quint32 magic = 0;
		quint32 version = 0;
		in >> magic >> version;
		if (magic != CURVE_DOC_SNAPSHOT_MAGIC || version > CURVE_DOC_SNAPSHOT_VERSION
			|| in.status() != QDataStream::Ok)
			return false;

		qint32 topLevel = 0;
		in >> topLevel;
		if (topLevel <= 0 || topLevel > 1000000)
			return false;

		//先完整读出并校验，失败时文档原状（原子性）
		QSet<qint32> ids;
		int maxId = 0;
		QList<CurveNode*> pages;
		for (int i = 0; i < topLevel; ++i)
		{
			CurveNode* page = readNode(in, ids, &maxId, version);
			if (!page)
			{
				for (CurveNode* p : pages)
					CurveDocument::freeNodeTree(p);
				return false;
			}
			if (page->type != CurveNodeType::Page)
			{
				//顶层只允许页子树（防误用其他节点当页挂根）
				CurveDocument::freeNodeTree(page);
				for (CurveNode* p : pages)
					CurveDocument::freeNodeTree(p);
				return false;
			}
			pages.append(page);
		}
		CurveDocument::CurveDocCounters counters;
		in >> counters.nextId >> counters.pageCount >> counters.chartCount >> counters.curveCount
			>> counters.baselineCount >> counters.annotationCount;
		//V7 追加字段：三维基准面默认编号计数（R41；V6 及更早快照无此段保持缺省
		//0——旧快照不可能含基准面节点，0 即语义正确值）
		counters.datumPlane3DCount = 0;
		if (version >= 7)
			in >> counters.datumPlane3DCount;
		if (in.status() != QDataStream::Ok)
		{
			for (CurveNode* p : pages)
				CurveDocument::freeNodeTree(p);
			return false;
		}

		//id 分配器不得回拨（防快照计数器异常导致后续新建节点 id 冲突）
		counters.nextId = qMax(counters.nextId, maxId + 1);
		for (CurveNode* page : pages)
			doc->insertNodeTree(page, doc->rootNodeId());
		doc->restoreCounters(counters);
		//R29 化石自愈：插入后对恢复树中全部 chartType=5 图表统一自愈（删除固定子
		//须走 idMap 既有摘除机制，故必须在 insertNodeTree 之后；此刻卡片尚未开
		//——mxses 恢复中卡片由页重建统一打开，树/视图随后按自愈后文档构建）。
		//正常三维图零变化（幂等），仅 R28 中间版化石树被修正
		for (CurveNode* page : pages)
			heal3DFossilCharts(doc, page);
		return true;
	}

	QByteArray saveTemplateSubtree(const CurveDocument* doc, const QList<int>& pageIds,
		QStringList* paramFiles)
	{
		if (paramFiles)
			paramFiles->clear();
		if (!doc || pageIds.isEmpty())
			return QByteArray();

		//先写节点体（参数清单边写边收集），再拼装头部：magic+版本+参数清单+页数+节点体
		//页数（topLevel）与节点体分开落串：restore 侧先读页数再取节点体整块校验
		QStringList params;
		QByteArray body;
		QDataStream bodyOut(&body, QIODevice::WriteOnly);
		bodyOut.setVersion(QDataStream::Qt_5_13);
		for (int pageId : pageIds)
		{
			const CurveNode* page = doc->node(pageId);
			if (!page || page->type != CurveNodeType::Page)
				return QByteArray();
			writeTemplateNode(bodyOut, page, &params);
		}

		QByteArray buffer;
		QDataStream out(&buffer, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_5_13);
		out.writeRawData(TEMPLATE_SUBTREE_MAGIC, 5);
		out << TEMPLATE_SUBTREE_VERSION;
		out << qint32(params.size());
		for (const QString& p : params)
			out << p;
		out << qint32(pageIds.size());
		out << body;
		if (paramFiles)
			*paramFiles = params;
		return buffer;
	}

	bool restoreTemplateSubtree(CurveDocument* doc, const QByteArray& chunk,
		const QStringList& paramFiles, QList<int>* restoredPageIds)
	{
		if (restoredPageIds)
			restoredPageIds->clear();
		if (!doc || chunk.isEmpty())
			return false;

		QDataStream in(chunk);
		in.setVersion(QDataStream::Qt_5_13);
		char magic[5] = {};
		in.readRawData(magic, 5);
		quint32 version = 0;
		in >> version;
		if (qstrncmp(magic, TEMPLATE_SUBTREE_MAGIC, 5) != 0 || version > TEMPLATE_SUBTREE_VERSION
			|| in.status() != QDataStream::Ok)
			return false;   //拒绝未知版本（兼容纪律对齐会话快照）

		//块内自带参数清单（保存时的原始路径），用户表按行覆盖
		qint32 chunkParamCount = 0;
		in >> chunkParamCount;
		if (chunkParamCount < 0 || chunkParamCount > 100000)
			return false;
		QStringList chunkParams;
		for (int i = 0; i < chunkParamCount; ++i)
		{
			QString p;
			in >> p;
			chunkParams.append(p);
		}

		qint32 topLevel = 0;
		in >> topLevel;
		QByteArray body;
		in >> body;
		if (in.status() != QDataStream::Ok || topLevel <= 0 || topLevel > 1000000)
			return false;

		//先从节点体完整读出并校验（原子性：失败不动文档、不占 id）
		QDataStream bin(body);
		bin.setVersion(QDataStream::Qt_5_13);
		QSet<qint32> ids;
		QList<CurveNode*> pages;
		for (int i = 0; i < topLevel; ++i)
		{
			CurveNode* page = readTemplateNode(bin, ids, version, paramFiles, chunkParams);
			if (!page || page->type != CurveNodeType::Page)
			{
				CurveDocument::freeNodeTree(page);
				for (CurveNode* p : pages)
					CurveDocument::freeNodeTree(p);
				return false;
			}
			pages.append(page);
		}
		if (bin.status() != QDataStream::Ok)
		{
			for (CurveNode* p : pages)
				CurveDocument::freeNodeTree(p);
			return false;
		}

		//插入前统计命名类型数（id 重编不改类型，先后无碍）
		int addPage = 0, addChart = 0, addCurve = 0, addBaseline = 0, addAnnotation = 0;
		int addDatumPlane3D = 0;
		for (CurveNode* page : pages)
			countNamingTypes(page, &addPage, &addChart, &addCurve, &addBaseline, &addAnnotation,
				&addDatumPlane3D);

		//id 重编（块内旧 id 易与现存节点冲突）→ 挂根 → 命名计数器推进
		QList<int> newPageIds;
		for (CurveNode* page : pages)
		{
			doc->reassignNodeTreeIds(page);
			newPageIds.append(page->id);
			doc->insertNodeTree(page, doc->rootNodeId());
		}
		CurveDocument::CurveDocCounters c = doc->snapshotCounters();
		c.pageCount += addPage;
		c.chartCount += addChart;
		c.curveCount += addCurve;
		c.baselineCount += addBaseline;
		c.annotationCount += addAnnotation;
		c.datumPlane3DCount += addDatumPlane3D;   //R41：基准面计数一并推进（模板含
		                                          //基准面时后续新建默认名不与之撞号）
		//nextId 已被 reassign 的 allocateNodeId 推进；restoreCounters 的 qMax 语义安全
		doc->restoreCounters(c);
		if (restoredPageIds)
			*restoredPageIds = newPageIds;
		return true;
	}
}
