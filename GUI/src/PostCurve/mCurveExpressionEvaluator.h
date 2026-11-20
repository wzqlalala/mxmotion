#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 表达式求值器（"定义曲线-数学运算"来源的求值内核，设计图12）
//
// 求值链：自研引擎 mCurveMathExpression（词法/递归下降/AST，替代原 muParser）
//   + 函数目录 mCurveMathFunctions（《函数类型.xlsx》目录与实现唯一样）。
//   y = f(x)：自变量以标识符 "x" 直接出现在表达式中（采样点横坐标）；
//   曲线向量引用形如 "p1w1c1.y"（页p1 视口w1 第c1条曲线的 y 向量，大小写
//   不敏感），求值时按该曲线 x 数据线性插值到当前采样点（区间外取端点值）。
//   实现：引用名含 '.'，不在引擎变量名字符集内，求值前把每个引用
//   整体替换为下划线形式（p1w1c1.y -> p1w1c1__y）并按引用注册变量。
//
// 向量函数：聚合（mean/rms/integral/... 现算成常量）与向量变换（delta/peaks
//   生成"虚拟向量"按引用同样插值）在求值前经 applyVecSubstitution 预替换，
//   支持嵌套（如 mean(delta(p1w1c1.y))）；未实现项（红色占位）求值直接报错。
// functionTable() 提供表达式构建器"函数"页的展示表（名称/签名/参数/说明/
// 分类/实现状态），适配自函数目录。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

namespace PostCurve
{
	//引用向量数据（调用方按文档结构解析 "pNwMcK.[x|y]" 后传入求值器）：
	//x 为该向量的自变量序列（升序），y 为取值序列（与 x 等长）
	struct CurveRefData
	{
		QString ref;            //引用名（小写规范形式，如 "p1w1c1.y"）
		QVector<double> x;      //自变量序列
		QVector<double> y;      //取值序列
	};

	class POSTCURVE_EXPORT mCurveExpressionEvaluator
	{
	public:
		//表达式构建器"函数"页的展示项
		struct MathFuncInfo
		{
			QString name;         //函数名（如 log10）
			QString signature;    //签名（如 "log10(x)"）
			QStringList params;   //参数名（按序）
			QString description;  //功能说明
			QString category;     //分类
			bool implemented = true;   //是否当前可求值：false=未实现项（《函数类型.xlsx》
			                           //目录中尚不支持者），UI 红色标注、禁用插入、求值报"未实现"
		};
		static const QList<MathFuncInfo>& functionTable();
		static QStringList functionCategories();   //去重分类（按表序）

		//提取表达式中的曲线向量引用（p\d+w\d+c\d+\.[xy]，统一小写、去重保序）
		static QStringList extractRefs(const QString& expression);

		//采样求值：outY 与 xSamples 等长。refs 须覆盖表达式中的全部引用，
		//缺项/空数据/解析错误经 errMsg 带出（返回 false，outY 清空）。
		//结果逐值清洗：±inf→NaN（数学域 NaN 保留），非有限点数经
		//nonFiniteCount 带出（默认 nullptr 兼容既有调用方，R40）
		static bool evaluate(const QString& expression, const QList<CurveRefData>& refs,
			const QVector<double>& xSamples, QVector<double>& outY, QString& errMsg,
			int* nonFiniteCount = nullptr);

		//二元网格求值（曲面 Z 侧数学来源）：rows=ys.size()（纵轴行）、
		//cols=xs.size()（横轴列）；outZ 行主序 outZ[r*cols+c]=f(xs[c], ys[r])
		//（行=Y、列=X，与"从文件新建三维图"矩阵表/Plot3D Row-Major 口径一致）。
		//表达式仅自变量 x/y（网格场景无曲线引用，refs 恒空——含引用直接报错）；
		//未实现函数预检/表达式为空/解析错误/异常经 errMsg 带出（返回 false，
		//outZ 清空、rows/cols 置 0）。常量表达式得到均匀网格，自然成立。
		//结果逐值清洗：±inf→NaN（数学域 NaN 保留），非有限单元数经
		//nonFiniteCount 带出（默认 nullptr 兼容既有调用方，R40）
		static bool evaluateGrid(const QString& expression, const QVector<double>& xs,
			const QVector<double>& ys, QVector<double>& outZ, int& rows, int& cols,
			QString& errMsg, int* nonFiniteCount = nullptr);

		//逐点配对二元求值（曲线 Z 侧数学来源）：调用方保证 xs.size()==ys.size()
		//（弹窗装配前已按行位配对截齐到较少者），逐点 z_i=f(xs[i], ys[i])，
		//out 与输入等长。表达式仅自变量 x/y（与 evaluateGrid 同口径：含曲线
		//引用直接报错）；未实现函数预检/空序列/空表达式/解析错误/异常经
		//errMsg 带出（返回 false，out 清空）。结果逐值清洗：±inf→NaN
		//（数学域 NaN 保留），非有限点数经 nonFiniteCount 带出（默认 nullptr）
		static bool evaluatePointwiseXY(const QString& expression,
			const QVector<double>& xs, const QVector<double>& ys,
			QVector<double>& outZ, QString& errMsg, int* nonFiniteCount = nullptr);

		//校验：取首个引用的起始横坐标（无引用取 0）试算一个点
		static bool validate(const QString& expression, const QList<CurveRefData>& refs,
			QString& errMsg);
	};
}
