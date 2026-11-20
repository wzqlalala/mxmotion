#include "mCurveExpressionEvaluator.h"
#include "mCurveMathExpression.h"
#include "mCurveMathFunctions.h"

//QT
#include <QRegularExpression>

//STL
#include <algorithm>
#include <cmath>
#include <limits>

namespace PostCurve
{
	namespace
	{
		//曲线向量引用模式：p页w视口c曲线.向量（x/y），大小写不敏感
		//（含向量字母——20261104 修正：原 [xy] 仅小写，大写 .Y 提取不到）
		const QRegularExpression& refPattern()
		{
			static const QRegularExpression re(
				QStringLiteral("\\b[pP](\\d+)[wW](\\d+)[cC](\\d+)\\.([XxYy])\\b"));
			return re;
		}

		//引用名转合法变量名（'.' 不在变量名字符集内，用双下划线代替）
		QString mangleRef(const QString& ref)
		{
			return QString(ref).toLower().replace(QLatin1Char('.'), QStringLiteral("__"));
		}

		//线性插值：xs 升序；采样点在区间外取端点值
		double interpolate(const QVector<double>& xs, const QVector<double>& ys, double x)
		{
			const int n = xs.size();
			if (n == 0)
				return 0.0;
			if (n == 1 || x <= xs.first())
				return ys.first();
			if (x >= xs.last())
				return ys.last();
			const auto it = std::lower_bound(xs.cbegin(), xs.cend(), x);
			const int i = int(it - xs.cbegin());
			const double x0 = xs[i - 1], x1 = xs[i];
			const double y0 = ys[i - 1], y1 = ys[i];
			if (x1 == x0)
				return y1;
			return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
		}

		//预替换实参形态：曲线引用（pNwMcK.[XxYy]）| 虚拟向量名（__vxN）| 数值字面量
		//（polyfit 阶数/kaiser beta 等）。注意全部写成非捕获组——嵌进外层调用模式
		//时不得改变分组编号（captured(1)=函数名，captured(2..4)=实参）
		QString vecArgPattern()
		{
			return QStringLiteral("(?:\\b[pP]\\d+[wW]\\d+[cC]\\d+\\.[XxYy]\\b|__vx\\d+"
				"|-?(?:\\d+\\.?\\d*|\\.\\d+)(?:[eE][+-]?\\d+)?)");
		}

		//目录中已实现、可预替换的整向量函数名（正则备选串）
		const QStringList& implementedVecNames()
		{
			static const QStringList names = []
			{
				QStringList out;
				for (const mCurveMathFunctions::FuncInfo& f : mCurveMathFunctions::catalog())
					if (f.implemented && (f.aggU || f.vecFn))
						out << QRegularExpression::escape(f.name);
				return out;
			}();
			return names;
		}

		//整向量函数调用模式：func(arg[, arg]... 最多 8 个)，实参形态见
		//vecArgPattern，实参个数/种类由目录 paramKinds 在替换时校验
		//（必填数=params.size()，可选数=paramKinds.size()-params.size()，
		//mstep 型变参按 3 个标量一组无限追加）
		const QRegularExpression& vecCallPattern()
		{
			static const QRegularExpression re = []
			{
				const QString names = implementedVecNames().join(QLatin1Char('|'));
				const QString pattern = names.isEmpty()
					? QStringLiteral("(?!x)x")   //永不匹配占位
					: QStringLiteral("\\b(%1)\\s*\\(\\s*(%2)"
						"(?:\\s*,\\s*(%2))?(?:\\s*,\\s*(%2))?(?:\\s*,\\s*(%2))?"
						"(?:\\s*,\\s*(%2))?(?:\\s*,\\s*(%2))?(?:\\s*,\\s*(%2))?"
						"(?:\\s*,\\s*(%2))?(?:\\s*,\\s*(%2))?(?:\\s*,\\s*(%2))?"
						"(?:\\s*,\\s*(%2))?(?:\\s*,\\s*(%2))?(?:\\s*,\\s*(%2))?"
						"(?:\\s*,\\s*(%2))?\\s*\\)")
						.arg(names, vecArgPattern());
				return QRegularExpression(pattern);
			}();
			return re;
		}

		//对 expr 应用一轮整向量函数替换：聚合现算成括号常量代回，向量变换生成
		//"虚拟向量"（__vxN，随引用同样逐点插值）后以名字代回。可嵌套（如
		//mean(delta(p1w1c1.y))）：外层模式随后续轮次再命中。实参按目录
		//paramKinds 解析（"v"=曲线引用/虚拟向量，"s"=数值字面量）；形参不匹配
		//或实参形态不符（如 mean(x*2)）则跳过本轮（引擎报"整向量函数"错误）。
		//出错（引用缺数据/函数报错）写 errMsg 并返回空串
		QString applyVecSubstitution(const QString& expr,
			const QHash<QString, const CurveRefData*>& refMap,
			QList<CurveRefData>& virtuals, bool& changed, QString& errMsg)
		{
			const QRegularExpression& re = vecCallPattern();
			auto it = re.globalMatch(expr);
			if (!it.hasNext())
				return expr;
			QString out;
			int last = 0;
			while (it.hasNext())
			{
				const QRegularExpressionMatch m = it.next();
				const mCurveMathFunctions::FuncInfo* info =
					mCurveMathFunctions::find(m.captured(1));
				if (!info || !(info->aggU || info->vecFn))
					continue;   //模式由目录生成，理论上不发生

				//实参数量（captured(2..9)，未取到的组为空）
				int argc = 0;
				for (int gi = 2; gi <= 13; ++gi)
					if (!m.captured(gi).isEmpty())
						argc = gi - 1;
				//形参匹配：必填数=params.size()，可选至 paramKinds.size()；
				//mstep 型变参在必填数以上按 3 个标量一组追加
				bool arityOk;
				if (info->variadicTripleScalars)
					arityOk = argc >= info->params.size()
						&& (argc - info->params.size()) % 3 == 0;
				else
					arityOk = argc >= info->params.size()
						&& argc <= info->paramKinds.size();
				if (!arityOk || argc == 0)
					continue;   //形参不匹配：留给引擎给出明确错误

				//按参数种类解析：'v' 收 (y, x) 序列，'s' 收数值字面量
				mCurveMathFunctions::VecArgs va;
				QVector<double> scalars;
				bool ok = true;
				bool kindMismatch = false;
				for (int i = 0; i < argc && ok; ++i)
				{
					const QString arg = m.captured(2 + i);
					//变参追加组均为标量（超出 paramKinds 的部分按 "s" 处理）
					const QString kind = i < info->paramKinds.size()
						? info->paramKinds.at(i) : QStringLiteral("s");
					if (kind == QLatin1String("s"))
					{
						bool numOk = false;
						const double v = arg.toDouble(&numOk);
						if (!numOk)
						{
							kindMismatch = true;   //数值参数不是字面量（引用/表达式）
							break;
						}
						scalars.push_back(v);
						continue;
					}
					if (arg.startsWith(QLatin1String("__vx")))
					{
						bool idxOk = false;
						const int idx = arg.mid(4).toInt(&idxOk);
						if (idxOk && idx >= 0 && idx < virtuals.size())
						{
							va.ys.push_back(&virtuals[idx].y);
							va.xs.push_back(&virtuals[idx].x);
						}
						else
						{
							errMsg = QStringLiteral("引用向量 “%1” 无有效数据").arg(arg);
							ok = false;
						}
						continue;
					}
					const CurveRefData* d = refMap.value(arg.toLower());
					if (!d)
					{
						errMsg = QStringLiteral("引用向量 “%1” 无有效数据").arg(arg);
						ok = false;
						continue;
					}
					va.ys.push_back(&d->y);
					va.xs.push_back(&d->x);
				}
				if (!ok)
					return QString();
				if (kindMismatch)
				{
					//预替换不适用（如 mean(x*2)/polyfit 传引用作阶数）：整段原样
					//保留，交由引擎给出"整向量函数"明确报错
					out += expr.mid(last, m.capturedEnd() - last);
					last = m.capturedEnd();
					continue;
				}

				QString replacement;
				QString err;
				if (info->kind == mCurveMathFunctions::Kind::Aggregate && info->aggU)
				{
					const double v = info->aggU(va, scalars, &err);
					if (!err.isEmpty())
					{
						errMsg = err;
						return QString();
					}
					//非有限值（inf/nan，如质心退化）直接报错：替换文本会被引擎
					//当未知变量名，报错误导
					if (!std::isfinite(v))
					{
						errMsg = QStringLiteral("函数 %1 计算结果为非有限值（inf/nan）")
							.arg(info->name);
						return QString();
					}
					replacement = QStringLiteral("(%1)").arg(QString::number(v, 'g', 17));
				}
				else if (info->kind == mCurveMathFunctions::Kind::VectorTransform && info->vecFn)
				{
					const QVector<double> vec = info->vecFn(va, scalars, &err);
					if (!err.isEmpty())
					{
						errMsg = err;
						return QString();
					}
					CurveRefData v;
					v.ref = QStringLiteral("__vx%1").arg(virtuals.size());
					v.x = va.xs.isEmpty() ? QVector<double>() : *va.xs[0];
					v.y = vec;
					virtuals.push_back(v);
					replacement = v.ref;
				}
				else
					continue;   //目录不一致，跳过
				out += expr.mid(last, m.capturedStart() - last);
				out += replacement;
				last = m.capturedEnd();
				changed = true;
			}
			out += expr.mid(last);
			return out;
		}

		//表达式中出现的"目录内未实现"函数名（《函数类型.xlsx》红色标注项）。
		//预检给出可定位的明确报错（引擎对未知名字也会报"未知函数"，此处消息更聚合）
		QStringList unimplementedFuncs(const QString& expression)
		{
			static const QRegularExpression funcCall(
				QStringLiteral("\\b([A-Za-z_][A-Za-z0-9_]*)\\s*\\("));
			QStringList out;
			const QList<mCurveExpressionEvaluator::MathFuncInfo>& table =
				mCurveExpressionEvaluator::functionTable();
			auto it = funcCall.globalMatch(expression);
			while (it.hasNext())
			{
				const QString name = it.next().captured(1).toLower();
				for (const mCurveExpressionEvaluator::MathFuncInfo& info : table)
				{
					if (info.name.compare(name, Qt::CaseInsensitive) == 0)
					{
						if (!info.implemented && !out.contains(info.name))
							out.push_back(info.name);
						break;
					}
				}
			}
			return out;
		}
	}

	const QList<mCurveExpressionEvaluator::MathFuncInfo>& mCurveExpressionEvaluator::functionTable()
	{
		//适配自 mCurveMathFunctions 目录（唯一样）：UI"函数"页展示 + 红黑标记
		static const QList<MathFuncInfo> table = []
		{
			QList<MathFuncInfo> out;
			out.reserve(mCurveMathFunctions::catalog().size());
			for (const mCurveMathFunctions::FuncInfo& f : mCurveMathFunctions::catalog())
			{
				MathFuncInfo info;
				info.name = f.name;
				info.signature = f.signature;
				info.params = f.params;
				info.description = f.description;
				info.category = f.category;
				info.implemented = f.implemented;
				out.push_back(info);
			}
			return out;
		}();
		return table;
	}

	QStringList mCurveExpressionEvaluator::functionCategories()
	{
		//仅返回含已实现函数的分类（未实现占位项不上"函数"页展示）
		QStringList out;
		for (const MathFuncInfo& f : functionTable())
		{
			if (!f.implemented)
				continue;
			if (!out.contains(f.category))
				out.push_back(f.category);
		}
		return out;
	}

	QStringList mCurveExpressionEvaluator::extractRefs(const QString& expression)
	{
		QStringList out;
		auto it = refPattern().globalMatch(expression);
		while (it.hasNext())
		{
			const QRegularExpressionMatch m = it.next();
			const QString ref = m.captured(0).toLower();
			if (!out.contains(ref))
				out.push_back(ref);
		}
		return out;
	}

	bool mCurveExpressionEvaluator::evaluate(const QString& expression,
		const QList<CurveRefData>& refs, const QVector<double>& xSamples,
		QVector<double>& outY, QString& errMsg, int* nonFiniteCount)
	{
		outY.clear();
		errMsg.clear();
		if (nonFiniteCount)
			*nonFiniteCount = 0;
		if (xSamples.isEmpty())
		{
			errMsg = QStringLiteral("X 采样序列为空");
			return false;
		}

		//未实现函数预检（函数页红色标注项），命中即报错
		const QStringList notImpl = unimplementedFuncs(expression);
		if (!notImpl.isEmpty())
		{
			errMsg = QStringLiteral("函数尚未实现（函数页红色标注项）：%1")
				.arg(notImpl.join(QStringLiteral(", ")));
			return false;
		}

		//引用收集与数据校验；缺项直接报错（不再猜测替代）
		const QStringList refNames = extractRefs(expression);
		QHash<QString, const CurveRefData*> refMap;
		for (const CurveRefData& r : refs)
			refMap.insert(r.ref.toLower(), &r);
		QStringList missing;
		for (const QString& ref : refNames)
			if (!refMap.contains(ref))
				missing.push_back(ref);
		if (!missing.isEmpty())
		{
			errMsg = QStringLiteral("引用向量无数据：%1").arg(missing.join(QStringLiteral(", ")));
			return false;
		}
		for (const QString& ref : refNames)
		{
			const CurveRefData* d = refMap.value(ref);
			if (d->x.isEmpty() || d->x.size() != d->y.size())
			{
				errMsg = QStringLiteral("引用向量 “%1” 无有效数据").arg(ref);
				return false;
			}
		}

		//聚合/向量变换预替换（可嵌套，迭代至不动点）：mean(p1w1c1.y) 现算成
		//常量、delta(p1w1c1.y) 生成虚拟向量；剩余表达式仍为逐点标量形态
		QString expr = expression;
		QList<CurveRefData> virtuals;
		for (int pass = 0; pass < 10; ++pass)
		{
			bool changed = false;
			expr = applyVecSubstitution(expr, refMap, virtuals, changed, errMsg);
			if (expr.isEmpty())
			{
				outY.clear();
				return false;
			}
			if (!changed)
				break;
		}

		//引用名转合法变量名后重组表达式（虚拟向量名无 '.'，不受影响）
		QString mangled = expr;
		for (const QString& ref : refNames)
			mangled.replace(ref, mangleRef(ref), Qt::CaseInsensitive);

		QVector<double> refVals(refNames.size(), 0.0);
		QVector<double> virtualVals(virtuals.size(), 0.0);
		double xv = 0.0;
		try
		{
			mCurveMathExpression engine;
			engine.setVariable(QStringLiteral("x"), &xv);
			for (int i = 0; i < refNames.size(); ++i)
				engine.setVariable(mangleRef(refNames.at(i)), &refVals[i]);
			for (int i = 0; i < virtuals.size(); ++i)
				engine.setVariable(virtuals.at(i).ref, &virtualVals[i]);
			//pi/e 常量由引擎预置；deg2rad/rad2deg 等已在函数目录
			if (!engine.compile(mangled, errMsg))
			{
				outY.clear();
				return false;
			}

			outY.resize(xSamples.size());
			for (int s = 0; s < xSamples.size(); ++s)
			{
				xv = xSamples.at(s);
				for (int i = 0; i < refNames.size(); ++i)
				{
					const CurveRefData* d = refMap.value(refNames.at(i));
					refVals[i] = interpolate(d->x, d->y, xv);
				}
				for (int i = 0; i < virtuals.size(); ++i)
				{
					const QVector<double>& vx = virtuals.at(i).x;
					const QVector<double>& vy = virtuals.at(i).y;
					//虚拟向量采样：无横坐标（纯数值参数型函数如窗函数）或长度与
					//横坐标不一致（拼接/求根/交点等变长结果）→ 按采样索引取值，
					//越界取末值；否则随引用同样线性插值
					virtualVals[i] = (vx.isEmpty() || vy.size() != vx.size())
						? (s < vy.size() ? vy.at(s) : (vy.isEmpty() ? 0.0 : vy.last()))
						: interpolate(vx, vy, xv);
				}
				outY[s] = engine.eval();
			}

			//结果逐值清洗（R40）：±inf→NaN（数学域 NaN 保留），非有限点数
			//带出——2D 侧对齐"NaN=断口"既有约定，3D 弹窗统计提示消费该计数
			int nonFinite = 0;
			constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
			for (double& v : outY)
			{
				if (!std::isfinite(v))
				{
					v = kNaN;
					++nonFinite;
				}
			}
			if (nonFiniteCount)
				*nonFiniteCount = nonFinite;
			return true;
		}
		catch (...)
		{
			errMsg = QStringLiteral("表达式求值发生未知异常");
			outY.clear();
			return false;
		}
	}

	bool mCurveExpressionEvaluator::evaluateGrid(const QString& expression,
		const QVector<double>& xs, const QVector<double>& ys,
		QVector<double>& outZ, int& rows, int& cols, QString& errMsg,
		int* nonFiniteCount)
	{
		outZ.clear();
		rows = 0;
		cols = 0;
		errMsg.clear();
		if (nonFiniteCount)
			*nonFiniteCount = 0;
		if (xs.isEmpty() || ys.isEmpty())
		{
			errMsg = QStringLiteral("网格轴序列为空");
			return false;
		}
		if (expression.trimmed().isEmpty())
		{
			errMsg = QStringLiteral("表达式为空");
			return false;
		}

		//未实现函数预检（与单变量 evaluate 同口径：函数页红色标注项，命中即报错）
		const QStringList notImpl = unimplementedFuncs(expression);
		if (!notImpl.isEmpty())
		{
			errMsg = QStringLiteral("函数尚未实现（函数页红色标注项）：%1")
				.arg(notImpl.join(QStringLiteral(", ")));
			return false;
		}

		//网格场景无曲线引用（refs 恒空）：表达式含引用直接报错（与 evaluate
		//缺项报错同文案口径，不静默）
		const QStringList refNames = extractRefs(expression);
		if (!refNames.isEmpty())
		{
			errMsg = QStringLiteral("引用向量无数据：%1")
				.arg(refNames.join(QStringLiteral(", ")));
			return false;
		}

		//rows=纵轴行（Y 值个数）、cols=横轴列（X 值个数）；compile 一次、双层
		//循环内改 *xv/*yv 后逐单元 eval（外层行=y、内层列=x，单元公式
		//Z(r,c)=f(xs[c], ys[r]) 与样例矩阵口径一致）
		const int nRows = ys.size();
		const int nCols = xs.size();
		double xv = 0.0;
		double yv = 0.0;
		try
		{
			mCurveMathExpression engine;
			engine.setVariable(QStringLiteral("x"), &xv);
			engine.setVariable(QStringLiteral("y"), &yv);
			//pi/e 常量由引擎预置
			if (!engine.compile(expression, errMsg))
			{
				outZ.clear();
				return false;
			}

			outZ.resize(nRows * nCols);
			for (int r = 0; r < nRows; ++r)
			{
				yv = ys.at(r);
				for (int c = 0; c < nCols; ++c)
				{
					xv = xs.at(c);
					outZ[r * nCols + c] = engine.eval();
				}
			}

			//结果逐值清洗（R40）：±inf→NaN（数学域 NaN 保留），非有限单元数
			//带出——引擎按断口渲染 NaN 单元，3D 弹窗"部分非法建图+统计提示 /
			//全部非法报错不建图"消费该计数
			int nonFinite = 0;
			constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
			for (double& v : outZ)
			{
				if (!std::isfinite(v))
				{
					v = kNaN;
					++nonFinite;
				}
			}
			if (nonFiniteCount)
				*nonFiniteCount = nonFinite;
			rows = nRows;
			cols = nCols;
			return true;
		}
		catch (...)
		{
			errMsg = QStringLiteral("表达式求值发生未知异常");
			outZ.clear();
			rows = 0;
			cols = 0;
			return false;
		}
	}

	bool mCurveExpressionEvaluator::evaluatePointwiseXY(const QString& expression,
		const QVector<double>& xs, const QVector<double>& ys,
		QVector<double>& outZ, QString& errMsg, int* nonFiniteCount)
	{
		outZ.clear();
		errMsg.clear();
		if (nonFiniteCount)
			*nonFiniteCount = 0;
		if (xs.isEmpty() || ys.isEmpty())
		{
			errMsg = QStringLiteral("配对序列为空");
			return false;
		}
		if (expression.trimmed().isEmpty())
		{
			errMsg = QStringLiteral("表达式为空");
			return false;
		}

		//未实现函数预检（与 evaluate/evaluateGrid 同口径：函数页红色标注项，
		//命中即报错）
		const QStringList notImpl = unimplementedFuncs(expression);
		if (!notImpl.isEmpty())
		{
			errMsg = QStringLiteral("函数尚未实现（函数页红色标注项）：%1")
				.arg(notImpl.join(QStringLiteral(", ")));
			return false;
		}

		//逐点场景无曲线引用（与 evaluateGrid 同口径）：表达式含引用直接报错
		//（与 evaluate 缺项报错同文案口径，不静默）
		const QStringList refNames = extractRefs(expression);
		if (!refNames.isEmpty())
		{
			errMsg = QStringLiteral("引用向量无数据：%1")
				.arg(refNames.join(QStringLiteral(", ")));
			return false;
		}

		//compile 一次、单层循环内改 *xv/*yv 后逐点 eval（z_i=f(xs[i], ys[i])，
		//调用方保证两序列等长——弹窗装配前已按行位配对截齐）
		const int n = qMin(xs.size(), ys.size());
		double xv = 0.0;
		double yv = 0.0;
		try
		{
			mCurveMathExpression engine;
			engine.setVariable(QStringLiteral("x"), &xv);
			engine.setVariable(QStringLiteral("y"), &yv);
			//pi/e 常量由引擎预置
			if (!engine.compile(expression, errMsg))
			{
				outZ.clear();
				return false;
			}

			outZ.resize(n);
			for (int i = 0; i < n; ++i)
			{
				xv = xs.at(i);
				yv = ys.at(i);
				outZ[i] = engine.eval();
			}

			//结果逐值清洗（R40）：±inf→NaN（数学域 NaN 保留），非有限点数
			//带出——弹窗按"部分非法建图+统计提示/全部非法报错不建图"消费该计数
			int nonFinite = 0;
			constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
			for (double& v : outZ)
			{
				if (!std::isfinite(v))
				{
					v = kNaN;
					++nonFinite;
				}
			}
			if (nonFiniteCount)
				*nonFiniteCount = nonFinite;
			return true;
		}
		catch (...)
		{
			errMsg = QStringLiteral("表达式求值发生未知异常");
			outZ.clear();
			return false;
		}
	}

	bool mCurveExpressionEvaluator::validate(const QString& expression,
		const QList<CurveRefData>& refs, QString& errMsg)
	{
		double x0 = 0.0;
		for (const CurveRefData& r : refs)
			if (!r.x.isEmpty())
			{
				x0 = r.x.first();
				break;
			}
		QVector<double> out;
		return evaluate(expression, refs, QVector<double>{ x0 }, out, errMsg);
	}
}
