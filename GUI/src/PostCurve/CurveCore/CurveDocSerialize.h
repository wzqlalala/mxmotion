#pragma once
#pragma execution_character_set("utf-8")
// =============================================================================
// 曲线模块 - 文档快照序列化（.mxses 会话接入用）
//
// 把 CurveDocument 整棵节点树（结构 + 每节点按类型生效的数据 + 默认编号计数器）
// 序列化为自包含 QByteArray（curveDocChunk），及从快照整体恢复。
// 设计对齐 mModelData 的 chunk 模式（mMxsesRestore::collectSessionData）：
//   - 快照内嵌曲线 xData/yData 原始值，恢复后不依赖 pch 文件即可显示；
//     pch 数据源仓库（mCurveDataSourceStore）按路径另行重登记，仅服务
//     "更改曲线/刷新"等溯源功能，文件缺失不影响显示。
//   - 恢复保留原节点 id（页结构引用键）与计数器（保证恢复后新建节点
//     默认命名/编号与保存时会话连续）。
//   - 快照自带 magic+版本，独立于 mxses 文件版本演进。
//
// 使用规约（与 CurveDocument 修改纪律一致）：
//   - save/restore 均为直写路径，不进命令栈；调用方（mMxsesRestore）负责
//     恢复前后清空文档（mCurveDocController::clearDocument）；
//   - restoreCurveDocument 原子性：先完整读出并校验快照再插入，快照损坏时
//     文档保持原状并返回 false。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "CurveDefine.h"

//Qt
#include <QByteArray>
#include <QDataStream>

namespace PostCurve
{
	class CurveDocument;

	//--- CurveDefine.h 各数据结构的流算子（快照格式的字段契约） ---
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveDataRefData& ref);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveDataRefData& ref);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveSeriesData& s);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveSeriesData& s);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveAxisData& a);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveAxisData& a);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveLegendData& l);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveLegendData& l);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveTextData& t);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveTextData& t);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveBaselineData& b);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveBaselineData& b);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveAnnotationData& a);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveAnnotationData& a);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurvePageData& p);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurvePageData& p);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveChartData& c);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveChartData& c);
	POSTCURVE_EXPORT QDataStream& operator<<(QDataStream& out, const CurveDatumPlane3DData& d);
	POSTCURVE_EXPORT QDataStream& operator>>(QDataStream& in, CurveDatumPlane3DData& d);

	//--- 文档快照 ---
	//整树序列化（根节点本身不存：恢复时文档必已有根，只写根下的页子树序列
	//+ 七个计数器，R41 起含三维基准面计数）。空文档（无页）返回空 QByteArray，
	//调用方按"无曲线"跳过
	POSTCURVE_EXPORT QByteArray saveCurveDocument(const CurveDocument* doc);

	//从快照整体恢复：先完整读出校验（页类型/id 唯一且不与现存冲突）再插入，
	//失败返回 false 且文档保持原状。要求调用方已 clearDocument（防御性兼容
	//非空文档：现存非根节点会造成 id 冲突，直接判失败）
	POSTCURVE_EXPORT bool restoreCurveDocument(CurveDocument* doc, const QByteArray& chunk);

	//--- 模板子树快照（.mxtpl 页子树块，add-template-curve 任务 1.1） ---
	//把指定页子树序列化为模板块。与整树快照（saveCurveDocument）同构，差异有四：
	// 1) 文件/数学/复数来源曲线的数据体（xData/yData/complexYData 与 xTickLabels）
	//    不写入——模板只存"怎么画"，应用时重新装配/求值；
	//    数值（定义式）来源曲线（无文件无三侧表达式）定义即数据，原样保留；
	// 2) 文件来源 sourceFile 参数化为 "@param{N}"（N=参数清单下标，按首次出现
	//    序去重），清单随块写出；
	// 3) 不写计数器——恢复端按目标文档计数器续接（合并语义），见下；
	// 4) 页 viewPageId 归 -1（模板应用生成新页，不继承保存会话的视图页绑定）。
	//pageIds 须全为页节点；空表/入参非法返回空 QByteArray。
	//paramFiles 回传实际参数清单（可传 nullptr 不回传）
	POSTCURVE_EXPORT QByteArray saveTemplateSubtree(const CurveDocument* doc,
		const QList<int>& pageIds, QStringList* paramFiles = nullptr);

	//从模板块恢复页子树并以追加语义挂入文档（不要求文档为空）。
	//paramFiles=用户替换的参数文件表，按行对应模板参数清单：第 N 行覆盖
	//"@param{N}"；行缺省回填块内保存的原始路径（"不替换"语义）。
	//节点 id 全部经 reassignNodeTreeIds 重编（块内旧 id 易与现存节点冲突；
	//树内跨节点引用按名锚定——注释附着曲线名/基准线关联轴名，无需映射），
	//命名计数器按插入子树推进（后续手动新建页/曲线的默认名不与模板页撞名）。
	//先完整读出校验再动文档（原子性）；失败返回 false 且文档保持原状。
	//restoredPageIds 回传新插入页 id（可传 nullptr）
	POSTCURVE_EXPORT bool restoreTemplateSubtree(CurveDocument* doc, const QByteArray& chunk,
		const QStringList& paramFiles, QList<int>* restoredPageIds = nullptr);
}
