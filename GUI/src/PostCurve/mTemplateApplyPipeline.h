#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 模板曲线应用管线（追加/叠加/替换三模式，add-template-curve 4.1/4.3/4.4/4.5）
//
// 把 .mxtpl 模板页子树应用到当前会话（mCurveDocController 单例文档）：
//   - 追加：既有页一动不动，模板页整体追加在后（4.1）；
//   - 替换（4.3）：目标=选定模板页面组——组内既有曲线/基准线/注释删除，
//     图表骨架保留并回放新环境与曲线（整组包单个撤销宏）；模板页数多于
//     目标组时多出部分按追加建新页；目标组多出的页/图表不动；
//   - 叠加（4.4）：目标=选定组的既有图表——曲线重放到既有图表（同名自动
//     改名保证视口内唯一），不回放页面/图表环境；"不叠加基准线/参考曲线"
//     勾=基准线独立成层（D10：应用前对目标组判重，命中=本次不生成；未命中
//     =照常生成并登记独立基准线层），不勾=逐条重放进曲线同层不做查重；
//   - 一个撤销宏：beginMacro("模板曲线-<模板名>") 包全部结构命令，
//     Ctrl+Z 一次整体撤销（数据直写不随 redo 重放，与pch导入同源限制）；
//   - 来源分派（curveSeriesSourceKind）：文件来源按五元组重新装配
//     （assembleChannelData，通道失配跳过不产假数据）、数值来源定义即数据
//     原样直写、数学来源按模板结构重映射引用后重新求值；
//   - 选项语义：自动缩放X/Y 勾=强制轴自动范围，不勾=回放模板保存范围；
//     使用报告颜色 勾=模板颜色回放，不勾=默认六色轮换（按目标图表既有曲线
//     数续起）；自动注释 不勾=不回放注释节点；
//   - 通道策略（4.5，收口在 assembleFileCurve 缝隙）：跳过验证=按模板五元组
//     直装（现状防线兜底）；选择最佳匹配=精确失配时按归一化相似度自动配对
//     （无命中告警跳过）；校验通道=按对话框映射表（channelMap）装配，
//     未配置行跳过；
//   - 应用成功后向 mCurveLayerManager 登记层（层名=模板名，任务 5.1）；
//     叠加勾选独立成层时另行登记基准线层（D10）。
//
// 无 GUI 依赖（不触碰 mCurveViewportFactory 懒加载单例），可无头单测；
// ChannelSelection 为值类型（通道映射表的值）。
// =============================================================================

#include "PostCurveGlobal.h"
#include "CurveWidgets/mCurveChannelSelector.h"

//QT
#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QVector>

namespace PostCurve
{
	class POSTCURVE_EXPORT mTemplateApplyPipeline
	{
	public:
		//应用选项（mTemplateCurveDlg::ApplyOptions 的管线侧映射）
		struct Options
		{
			bool autoScaleX = true;      //自动缩放X：勾=按数据适配，不勾=模板保存范围
			bool autoScaleY = true;      //自动缩放Y：同上
			bool useReportColors = true; //使用报告颜色：勾=模板颜色，不勾=默认六色轮换
			bool autoAnnotation = true;  //自动注释：不勾=不回放注释节点
			bool skipBaselines = false;  //叠加子项"不叠加基准线/参考曲线"（仅叠加生效；勾=基准线独立成层+判重，D10）
			int channelStrategy = 0;     //0跳过验证(默认)/1校验通道/2选择最佳匹配
			//校验通道映射表（策略 1）：模板五元组键（mChannelMapCore::channelKey）
			//-> 实际通道；未配置行跳过不产假数据
			QHash<QString, ChannelSelection> channelMap;
		};

		//应用结果（部分跳过不算失败：页骨架仍生成，曲线缺数据）
		struct Result
		{
			bool ok = false;
			QString err;              //整体失败原因（ok=false 时有效）
			QList<int> pageIds;       //本次涉及页（追加=新建页；替换/叠加=作用页）
			QList<int> createdCurveIds;    //本次新建曲线 id（图层登记）
			QList<int> createdBaselineIds; //本次新建基准线 id（图层登记）
			int createdCurves = 0;    //成功创建（含装配/直写/求值落数据）的曲线数
			int skippedCurves = 0;    //跳过的曲线数（通道失配/文件不可用/求值失败）
			QStringList warnings;     //跳过与告警明细（已同步经控制器上报）
			int registeredLayerId = 0; //应用完成登记的层 id（任务 5.1）
		};

		//追加应用：模板页整体追加到当前会话（paramOverride 第 N 行覆盖参数 N，
		//空串/缺行=回退模板保存时的原始路径；下同）
		static Result appendTemplate(const QString& templatePath,
			const QStringList& paramOverride, const Options& opt);

		//叠加应用：曲线/基准线重放到 targetPageIds 各页的既有图表（页序/图表序
		//按模板位置对位，缺位新建；环境不回放）
		static Result overlayTemplate(const QString& templatePath,
			const QStringList& paramOverride, const Options& opt,
			const QVector<int>& targetPageIds);

		//替换应用：目标组既有曲线/基准线/注释删除，图表骨架保留回放新环境与
		//曲线（缺位新建）；无组场景由调用方（对话框）提示终止
		static Result replaceTemplate(const QString& templatePath,
			const QStringList& paramOverride, const Options& opt,
			const QVector<int>& targetPageIds);
	};
}
