#include "mCurveMathExpression.h"
#include "mCurveMathFunctions.h"

//QT
#include <QChar>

//STL
#include <cmath>
#include <limits>
#include <utility>

namespace PostCurve
{
	//AST 节点：编译期把变量/常量/函数全部解析成直读指针，求值零查表
	struct mCurveMathExpression::Node
	{
		enum class Type { Number, Constant, Variable, Unary, Binary, Call };

		Type type = Type::Number;
		double number = 0.0;                       //Number/Constant
		const double* varPtr = nullptr;            //Variable
		QChar op;                                  //Unary('-','+') / Binary('+','-','*','/','^')
		mCurveMathExpression::Func1 fn1 = nullptr; //Call（1 参）
		mCurveMathExpression::Func2 fn2 = nullptr; //Call（2 参）
		QString funcName;                          //Call（仅报错时定位用）
		std::unique_ptr<Node> a, b;                //Unary: a；Binary/Call: a(+b)

		double eval() const
		{
			switch (type)
			{
			case Type::Number:
			case Type::Constant:
				return number;
			case Type::Variable:
				return varPtr ? *varPtr : std::numeric_limits<double>::quiet_NaN();
			case Type::Unary:
			{
				const double v = a->eval();
				return op == '-' ? -v : v;
			}
			case Type::Binary:
			{
				const double l = a->eval();
				const double r = b->eval();
				switch (op.toLatin1())
				{
				case '+': return l + r;
				case '-': return l - r;
				case '*': return l * r;
				case '/': return l / r;
				case '^': return std::pow(l, r);
				default:  return std::numeric_limits<double>::quiet_NaN();
				}
			}
			case Type::Call:
				if (fn1 && a)
					return fn1(a->eval());
				if (fn2 && a && b)
					return fn2(a->eval(), b->eval());
				return std::numeric_limits<double>::quiet_NaN();
			default:
				return std::numeric_limits<double>::quiet_NaN();
			}
		}
	};

	class mCurveMathExpression::Parser
	{
	public:
		Parser(const QString& text, const QHash<QString, const double*>& variables,
			const QHash<QString, double>& constants)
			: _text(text), _variables(variables), _constants(constants)
		{
		}

		std::unique_ptr<Node> parse(QString& errMsg)
		{
			_pos = 0;
			auto root = parseExpression(errMsg);
			if (root != nullptr && _pos < _text.size())
			{
				//表达式尾部还有残余 token（如 "1+1 2"）
				errMsg = QStringLiteral("表达式语法错误：位置 %1 存在多余内容“%2”")
					.arg(_pos + 1).arg(_text.mid(_pos));
				return nullptr;
			}
			return root;
		}

	private:
		bool atEnd() const { return _pos >= _text.size(); }
		QChar peek() const { return atEnd() ? QChar() : _text.at(_pos); }

		//跳过空白后向前取字符（不消费）
		QChar peekSkip()
		{
			while (!atEnd() && _text.at(_pos).isSpace())
				++_pos;
			return peek();
		}

		bool match(QChar c)
		{
			if (peekSkip() == c)
			{
				++_pos;
				return true;
			}
			return false;
		}

		std::unique_ptr<Node> parseExpression(QString& errMsg)
		{
			if (++_depth > kMaxDepth)
			{
				errMsg = QStringLiteral("表达式嵌套过深（超过 %1 层）").arg(kMaxDepth);
				return nullptr;
			}
			auto left = parseTerm(errMsg);
			if (!(left))
				return nullptr;
			for (;;)
			{
				const QChar c = peekSkip();
				if (c != '+' && c != '-')
					break;
				++_pos;
				auto right = parseTerm(errMsg);
				if (!(right))
					return nullptr;
				auto node = std::make_unique<Node>();
				node->type = Node::Type::Binary;
				node->op = c;
				node->a = std::move(left);
				node->b = std::move(right);
				left = std::move(node);
			}
			--_depth;
			return left;
		}

		std::unique_ptr<Node> parseTerm(QString& errMsg)
		{
			if (++_depth > kMaxDepth)
			{
				errMsg = QStringLiteral("表达式嵌套过深（超过 %1 层）").arg(kMaxDepth);
				return nullptr;
			}
			auto left = parseUnary(errMsg);
			if (!(left))
				return nullptr;
			for (;;)
			{
				const QChar c = peekSkip();
				if (c != '*' && c != '/')
					break;
				++_pos;
				auto right = parseUnary(errMsg);
				if (!(right))
					return nullptr;
				auto node = std::make_unique<Node>();
				node->type = Node::Type::Binary;
				node->op = c;
				node->a = std::move(left);
				node->b = std::move(right);
				left = std::move(node);
			}
			--_depth;
			return left;
		}

		std::unique_ptr<Node> parseUnary(QString& errMsg)
		{
			if (++_depth > kMaxDepth)
			{
				errMsg = QStringLiteral("表达式嵌套过深（超过 %1 层）").arg(kMaxDepth);
				return nullptr;
			}
			const QChar c = peekSkip();
			if (c == '+' || c == '-')
			{
				++_pos;
				auto operand = parseUnary(errMsg);
				if (!(operand))
					return nullptr;
				if (c == '-')
				{
					auto node = std::make_unique<Node>();
					node->type = Node::Type::Unary;
					node->op = c;
					node->a = std::move(operand);
					--_depth;
					return node;
				}
				--_depth;
				return operand;   //一元 + 直接透传
			}
			auto node = parsePower(errMsg);
			--_depth;
			return node;
		}

		std::unique_ptr<Node> parsePower(QString& errMsg)
		{
			if (++_depth > kMaxDepth)
			{
				errMsg = QStringLiteral("表达式嵌套过深（超过 %1 层）").arg(kMaxDepth);
				return nullptr;
			}
			auto base = parsePrimary(errMsg);
			if (!(base))
				return nullptr;
			if (match('^'))
			{
				//右结合：指数侧允许一元负号（2^-3）
				auto exponent = parseUnary(errMsg);
				if (!(exponent))
					return nullptr;
				auto node = std::make_unique<Node>();
				node->type = Node::Type::Binary;
				node->op = '^';
				node->a = std::move(base);
				node->b = std::move(exponent);
				--_depth;
				return node;
			}
			--_depth;
			return base;
		}

		std::unique_ptr<Node> parsePrimary(QString& errMsg)
		{
			if (++_depth > kMaxDepth)
			{
				errMsg = QStringLiteral("表达式嵌套过深（超过 %1 层）").arg(kMaxDepth);
				return nullptr;
			}
			const QChar c = peekSkip();
			if (atEnd())
			{
				errMsg = QStringLiteral("表达式意外结束");
				return nullptr;
			}

			//数字（含科学计数法；指数后必须跟数字，否则按普通数字截断）
			if (c.isDigit() || c == '.')
			{
				const int start = _pos;
				while (!atEnd() && (peek().isDigit() || peek() == '.'))
					++_pos;
				if (!atEnd() && (peek() == 'e' || peek() == 'E'))
				{
					int look = _pos + 1;
					if (look < _text.size() && (_text.at(look) == '+' || _text.at(look) == '-'))
						++look;
					if (look < _text.size() && _text.at(look).isDigit())
					{
						++_pos;                          //e/E
						if (!atEnd() && (peek() == '+' || peek() == '-'))
							++_pos;
						while (!atEnd() && peek().isDigit())
							++_pos;
					}
				}
				bool ok = false;
				const double value = _text.mid(start, _pos - start).toDouble(&ok);
				if (!ok)
				{
					errMsg = QStringLiteral("非法数字字面量“%1”（位置 %2）")
						.arg(_text.mid(start, _pos - start)).arg(start + 1);
					return nullptr;
				}
				auto node = std::make_unique<Node>();
				node->type = Node::Type::Number;
				node->number = value;
				--_depth;
				return node;
			}

			//标识符：变量 / 常量 / 函数调用
			if (isIdentStart(c))
			{
				const int start = _pos;
				while (!atEnd() && isIdentPart(peek()))
					++_pos;
				const QString name = _text.mid(start, _pos - start);

				if (match('('))
				{
					return parseCall(name, errMsg);
				}

				auto node = std::make_unique<Node>();
				const auto cit = _constants.constFind(name);
				if (cit != _constants.constEnd())
				{
					node->type = Node::Type::Constant;
					node->number = cit.value();
				}
				else
				{
					const auto vit = _variables.constFind(name);
					if (vit == _variables.constEnd())
					{
						errMsg = QStringLiteral("未知变量或常量：“%1”").arg(name);
						return nullptr;
					}
					node->type = Node::Type::Variable;
					node->varPtr = vit.value();
				}
				--_depth;
				return node;
			}

			if (c == '(')
			{
				++_pos;
				auto inner = parseExpression(errMsg);
				if (!(inner))
					return nullptr;
				if (!match(')'))
				{
					errMsg = QStringLiteral("表达式语法错误：位置 %1 缺少右括号") .arg(_pos + 1);
					return nullptr;
				}
				--_depth;
				return inner;
			}

			errMsg = QStringLiteral("表达式语法错误：位置 %1 出现意外字符“%2”")
				.arg(_pos + 1).arg(QString(c));
			return nullptr;
		}

		std::unique_ptr<Node> parseCall(const QString& name, QString& errMsg)
		{
			const mCurveMathFunctions::FuncInfo* info = mCurveMathFunctions::find(name);
			if (!info)
			{
				errMsg = QStringLiteral("未知函数：“%1”").arg(name);
				return nullptr;
			}
			if (!info->implemented)
			{
				errMsg = QStringLiteral("函数尚未实现（函数页红色标注项）：%1").arg(info->name);
				return nullptr;
			}
			if (info->kind == mCurveMathFunctions::Kind::Constant || info->params.isEmpty())
			{
				errMsg = QStringLiteral("“%1”是常量，不能作为函数调用").arg(info->name);
				return nullptr;
			}
			//整向量函数（聚合/向量变换）：仅接受曲线向量引用参数，由求值器
			//预替换承担（mean(p1w1c1.y)），标量求值内核不直接执行——此处拦截
			//也兜住预替换未命中的写法（如 mean(x*2)），给出明确报错而非 NaN
			if (info->kind == mCurveMathFunctions::Kind::Aggregate
				|| info->kind == mCurveMathFunctions::Kind::VectorTransform)
			{
				errMsg = QStringLiteral("函数“%1”为整向量函数：参数须为曲线向量引用（如 p1w1c1.y）")
					.arg(info->name);
				return nullptr;
			}
			const int want = int(info->params.size());

			//参数列表：expression (',' expression)*，允许零参（常量型调用不在此路径）
			std::unique_ptr<Node> arg1, arg2;
			int count = 0;
			if (peekSkip() != ')')
			{
				arg1 = parseExpression(errMsg);
				if (!(arg1))
					return nullptr;
				count = 1;
				while (match(','))
				{
					if (count >= 2)
					{
						errMsg = QStringLiteral("函数“%1”最多支持 2 个参数").arg(name);
						return nullptr;
					}
					arg2 = parseExpression(errMsg);
					if (!(arg2))
						return nullptr;
					++count;
				}
			}
			if (!match(')'))
			{
				errMsg = QStringLiteral("函数“%1”调用缺少右括号").arg(name);
				return nullptr;
			}
			if (count != want)
			{
				errMsg = QStringLiteral("函数“%1”需要 %2 个参数，实际 %3 个")
					.arg(name).arg(want).arg(count);
				return nullptr;
			}

			auto node = std::make_unique<Node>();
			node->type = Node::Type::Call;
			node->funcName = info->name;
			node->fn1 = info->fn1;
			node->fn2 = info->fn2;
			node->a = std::move(arg1);
			node->b = std::move(arg2);
			--_depth;
			return node;
		}

		static bool isIdentStart(QChar c)
		{
			return c.isLetter() || c == '_';
		}
		static bool isIdentPart(QChar c)
		{
			return c.isLetterOrNumber() || c == '_';
		}

		static constexpr int kMaxDepth = 100;   //嵌套深度上限（防用户超长表达式栈溢出）

		const QString& _text;
		const QHash<QString, const double*>& _variables;
		const QHash<QString, double>& _constants;
		int _pos = 0;
		int _depth = 0;
	};

	// =============================================================================
	// 公共接口
	// =============================================================================

	mCurveMathExpression::mCurveMathExpression()
	{
		setConstant(QStringLiteral("pi"), 3.14159265358979323846);
		setConstant(QStringLiteral("e"), 2.71828182845904523536);
	}

	mCurveMathExpression::~mCurveMathExpression() = default;

	void mCurveMathExpression::setVariable(const QString& name, const double* ptr)
	{
		_variables.insert(name, ptr);
	}

	void mCurveMathExpression::setConstant(const QString& name, double value)
	{
		_constants.insert(name, value);
	}

	bool mCurveMathExpression::compile(const QString& expression, QString& errMsg)
	{
		errMsg.clear();
		_root = nullptr;
		if (expression.trimmed().isEmpty())
		{
			errMsg = QStringLiteral("表达式为空");
			return false;
		}
		Parser parser(expression, _variables, _constants);
		_root = parser.parse(errMsg);
		return _root != nullptr;
	}

	double mCurveMathExpression::eval() const
	{
		return _root ? _root->eval()
		             : std::numeric_limits<double>::quiet_NaN();
	}
}
