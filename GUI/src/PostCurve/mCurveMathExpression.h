#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 自研表达式求值引擎（"定义曲线-数学运算"求值内核，替代 muParser）
//
// 语法：四则 + - * /、幂 ^（右结合，-2^2 = -(2^2)）、一元 +/-、括号、
//   函数调用（按 mCurveMathFunctions 目录校验名称/参数个数/实现状态）、
//   变量（求值前经 setVariable 指针绑定）、常量（预置 pi/e，可追加）。
//   不支持隐式乘法、字符串、布尔/比较运算（现模块未用到）。
// 用法：setVariable 全部绑定 -> compile（语法/函数/变量静态校验）-> 循环内
//   改 *ptr 后逐点 eval()（对齐原 mu::Parser 指针绑定采样模式）。
// =============================================================================

#include <QHash>
#include <QString>

#include <memory>

namespace PostCurve
{
	//模块内私有（仅 mCurveExpressionEvaluator 与自测程序使用，不导出符号）
	class mCurveMathExpression
	{
	public:
		using Func1 = double (*)(double);
		using Func2 = double (*)(double, double);

		mCurveMathExpression();
		~mCurveMathExpression();
		mCurveMathExpression(const mCurveMathExpression&) = delete;
		mCurveMathExpression& operator=(const mCurveMathExpression&) = delete;

		//变量指针绑定：外部改 *ptr 后 eval() 即用新值；须在 compile 前注册
		void setVariable(const QString& name, const double* ptr);
		//常量注册（预置 pi/e；重名覆盖）。与变量的区别：值固定、无需指针
		void setConstant(const QString& name, double value);

		//编译表达式（词法+语法+静态校验：未知变量/未知函数/未实现函数/
		//参数个数不匹配）。失败返回 false，errMsg 带中文描述（含出错位置）
		bool compile(const QString& expression, QString& errMsg);

		//单点求值（须已 compile 成功）；除零等按 IEEE 浮点语义得 inf/nan，
		//不视为求值错误
		double eval() const;

	private:
		struct Node;
		std::unique_ptr<Node> _root;                       //编译产物 AST 根
		QHash<QString, const double*> _variables;          //变量槽（名字大小写敏感）
		QHash<QString, double> _constants;                 //常量表

		//解析器（词法+递归下降+校验）实现置于 cpp，隐藏 AST 细节
		class Parser;
	};
}
