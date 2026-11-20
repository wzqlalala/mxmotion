#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 自研数学函数库
//
// 唯一样：mCurveMathExpression 按此查函数/常量定义，mCurveExpressionEvaluator
// 按此出"函数"页展示表（红黑、说明、签名）并做整向量函数的预替换求值。
//
// 条目四类（Kind）：
//   Scalar          逐点标量函数（fn1/fn2，引擎直接调用）
//   Aggregate       整向量聚合 vector->scalar（aggU，求值器预替换现算成常量）
//   VectorTransform 向量变换 vector->vector（vecFn，结果经"虚拟向量"插值）
//   Constant        常量名（pi/e，表达式中直接以名字出现）
//
// 整向量函数参数种类（paramKinds，与 params 一一同序）：
//   "v" = 曲线向量引用（pNwMcK.[xy] 或虚拟向量），传入 VecArgs::ys/xs；
//   "s" = 数值字面量（如 polyfit 的阶数、kaiser 的 beta），传入 scalars。
//
// implemented=false 为目录占位项（UI 红色、求值报"尚未实现"），仅收录元数据——
// 集中在三类：字符串函数（数值引擎无字符串类型）、文件 IO/Tcl/系统（需文档与
// 系统上下文）、参考专用算法（srs/OLC/Vold-Kalman 等，语义待与参考软件核对）。
// =============================================================================

//QT
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

//STL
#include <functional>

namespace PostCurve
{
	class mCurveMathFunctions
	{
	public:
		enum class Kind { Scalar, Aggregate, VectorTransform, Constant };

		//整向量函数实参：ys[i]/xs[i] = 第 i 个 "v" 引用的取值/横坐标序列
		struct VecArgs
		{
			QVector<const QVector<double>*> ys;
			QVector<const QVector<double>*> xs;
		};

		using AggFnU = std::function<double(const VecArgs&, const QVector<double>& scalars, QString* err)>;
		using VecFnU = std::function<QVector<double>(const VecArgs&, const QVector<double>& scalars, QString* err)>;

		struct FuncInfo
		{
			QString name;              //函数名（大小写敏感）
			QString signature;         //签名（"函数"页展示）
			QStringList params;        //必填参数名（按序；常量为空）
			QStringList paramKinds;    //参数种类（"v"/"s"，含可选参数，与实参位置对应）
			bool variadicTripleScalars = false;   //mstep 型变参：末尾按 3 个标量一组
			                                      //无限追加（(argc-必填数)%3==0）
			QString description;       //功能说明
			QString category;          //分类（《函数类型.xlsx》10 类 + 本引擎补充）
			bool implemented = false;  //是否当前可求值（false=红色占位）
			Kind kind = Kind::Scalar;

			//实现（按 kind 取用；implemented=true 时非空）
			double (*fn1)(double) = nullptr;
			double (*fn2)(double, double) = nullptr;
			AggFnU aggU;
			VecFnU vecFn;
		};

		//全量目录（构建期一次性生成，调用方只读）
		static const QList<FuncInfo>& catalog();

		//按名精确查找（未收录返回 nullptr）
		static const FuncInfo* find(const QString& name);

		//常量取值（pi/e；未收录返回 false）
		static bool constantValue(const QString& name, double& out);
	};
}
