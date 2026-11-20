#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 通道映射核心（纯逻辑，无 GUI 依赖，可无头单测）
//
// 模板通道（曲线来源五元组）与新数据文件通道的匹配/预配/候选枚举/CSV 存取，
// 供通道映射对话框（mChannelMapDlg，任务 4.6/4.7/4.8）与应用管线（任务 4.5，
// "选择最佳匹配"策略）共用：
//   - 匹配判定 = mCurveChannelSelector::fromModelTexts 定位 + assembleChannelData
//     试装配（能装配出数据即"已匹配"，与曲线生成同一防线）；
//   - 相似度 = 归一化（去分隔符/小写）后 Levenshtein，v1 细则（design Open
//     Questions：通道映射对话框人工兜底，误配影响可控）。
// =============================================================================

#include "PostCurveGlobal.h"
#include "CurveWidgets/mCurveChannelSelector.h"

//QT
#include <QString>
#include <QList>

namespace PostCurve
{
	class POSTCURVE_EXPORT mChannelMapCore
	{
	public:
		//"自动填充"预配模式（应用选项，任务 4.9；控制相似通道预配的字段侧重）
		enum AutoFillMode
		{
			AutoFillNone = 0,     //无：不预配（默认，仅精确匹配+人工调整）
			AutoFillFileName = 1, //文件名：全字段归一化相似度
			AutoFillSuffix = 2,   //后缀名：分量/结果类型后缀字段侧重
		};

		//通道映射行（表格一行 = 一条模板文件来源曲线的"一侧"；复数曲线按侧展开）
		struct Row
		{
			//侧别（复数曲线按侧出行：锚行+yp 行+x 行，各自独立映射）
			enum Side
			{
				SideAnchor = 0,   //ym 锚侧（五元组 = series.subcase/schema/object/component）
				SideYp = 1,       //yp 侧（complexSide* 四元组 + 分量令牌）
				SideX = 2,        //x 侧（xSide* 四元组，仅通道形态）
			};
			QString curveName;                       //模板内曲线名；侧行存限定名
			                                         //"曲线名 (yp)"/"曲线名 (x)"（CSV 对齐键）
			int side = SideAnchor;                   //侧别（复数曲线锚行外按侧出行）
			QString subcase;                         //模板通道显示串（工况）
			QString schema;                          //模板通道显示串（结果类型）
			QString object;                          //模板通道显示串（对象）
			QString component;                       //模板通道显示串（分量）
			ChannelSelection mapped;                 //当前映射（complete=已匹配；
			                                         //filePath=目标数据文件）
			bool exactHit = false;                   //初始即精确命中（重置回到此状态）
			bool confirmed = false;                  //已确认（绿勾，确认后不回退）
		};

		//模板五元组 -> 映射表键（管线 channelMap 与 CSV 的对齐键同源）
		static QString channelKey(const QString& subcase, const QString& schema,
			const QString& object, const QString& component);

		//从模板文件提取通道行（文件来源曲线逐条一行；effectiveParams 为参数表
		//替换后的当前参数值，与管线同规：空行回退模板保存时原始路径）。
		//模板读取/恢复失败返回空表（err 带原因）
		static QList<Row> rowsFromTemplate(const QString& templatePath,
			const QStringList& effectiveParams, QString* err = nullptr);

		//数据文件预登记（首次打开即可匹配的前提）：行集引用的文件来源路径全部
	//同步入共享仓库——否则（本会话尚无任何入库路径）hasChannelData"数据文件
	//不在仓库"、候选索引空，精确匹配/相似预配/行内下拉三者全哑。幂等
	//（contains 跳过已登记不重复解析）；单文件失败不中断（该文件各行保持
	//未配红叉，语义同管线 brokenFiles）。须在 initExactMatches/相似预配/
	//候选枚举之前调用
	static void ensureFilesRegistered(const QList<Row>& rows);

	//精确匹配：逐行 fromModelTexts 定位 + 试装配，成功置 mapped/exactHit
		//（目标文件取各行 mapped.filePath——模板可引用多个参数文件）
		static void initExactMatches(QList<Row>* rows);

		//自动连接相似通道（对话框初始预配）：仅未匹配行按相似度预配
		static void autoConnectSimilar(QList<Row>* rows, int autoFillMode);

		//选择最佳匹配：仅未匹配行（精确命中的行不动），无命中保持未配
		static void bestMatchUnmatched(QList<Row>* rows);

		//按模板通道文本在目标文件找最佳匹配通道（管线"选择最佳匹配"策略用）。
		//无命中（低于门槛）返回 complete=false 的 selection
		static ChannelSelection bestMatchChannel(const QString& filePath,
			const QString& subcaseText, const QString& schema,
			const QString& objectText, const QString& component);

		//目标文件四级候选（行内下拉现算，显示串与创建链路同源）：
		//工况列=该文件全部 Subcase、结果类型列=该工况全部结果类型、
		//对象列=该结果类型下全部对象、分量列=该对象全部分量
		static QStringList subcaseTexts(const QString& filePath);
		static QStringList schemaTexts(const QString& filePath, int subcaseId);
		static QStringList objectTexts(const QString& filePath, int subcaseId, const QString& schema);
		static QStringList componentTexts(const QString& filePath, int subcaseId,
			const QString& schema, int pointId);

		//映射配置 CSV 保存/导入（按曲线名对齐；UTF-8 BOM；列头
		//曲线名/工况/结果类型/对象/分量，值为映射后实际通道显示串）。
		//loadCsv 按曲线名回填 rows 中同名列（导入映射须先经 initExactMatches
		//建立行集），并复验可装配（装配失败该行保持未配）
		static bool saveCsv(const QList<Row>& rows, const QString& filePath,
			QString* err = nullptr);
		static bool loadCsv(const QString& filePath, QList<Row>* rows,
			QString* err = nullptr);

		//归一化相似度（去分隔符/小写后 Levenshtein；1-距离/最大长度）。
		//threshold() 为匹配门槛（v1=0.6）
		static double similarity(const QString& a, const QString& b);
		static double threshold() { return 0.6; }
	};
}
