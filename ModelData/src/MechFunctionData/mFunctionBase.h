#pragma once
//解决中文乱码
#pragma execution_character_Function("utf-8")

#include <QString>
#include <vector>

#include "MXFunctionData_global.h"
#include "mPhysicalFieldEnum.h"

#include <QVector3D>
#include <QVector>
#include <QDataStream>


namespace MechFunctionData
{
	struct AnalyticCol
	{
		double xMin = 0;   // 变量范围
		double xMax = 0;
		QString expr;      // 表达式
		int simple = 0;

		AnalyticCol() = default;
		AnalyticCol(double xi, double xa, const QString& e, int sim) {
			xMin = xi;
			xMax = xa;
			expr = e;
			simple = sim;
		}
	};
	inline QDataStream& operator<<(QDataStream& out, const AnalyticCol& c)
	{
		out << c.xMin << c.xMax << c.expr << c.simple;
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, AnalyticCol& c)
	{
		in >> c.xMin >> c.xMax >> c.expr >> c.simple;
		return in;
	}

	struct DiscreteCol
	{
		std::vector<int>    idx;   // 序号
		std::vector<double> x;
		std::vector<double> y;
	};
	inline QDataStream& operator<<(QDataStream& out, const DiscreteCol& c)
	{
		out << static_cast<int>(c.idx.size());
		for (size_t i = 0; i < c.idx.size(); ++i)
		{
			out << c.idx[i] << c.x[i] << c.y[i];
		}
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, DiscreteCol& c)
	{
		int count;
		in >> count;
		c.idx.resize(count);
		c.x.resize(count);
		c.y.resize(count);
		for (int i = 0; i < count; ++i)
		{
			in >> c.idx[i] >> c.x[i] >> c.y[i];
		}
		return in;
	}

	struct InterpCol
	{
		std::vector<double> x;          // 原始 x
		std::vector<double> y;          // 原始 y
		int interpMode = 1;             // 0 临近点 / 1 线性 / 2 分段三次 / 3 三次样条 / 4 对数插值
		int extrapMode = 0;             // 0 常数 / 1 线性
	};
	inline QDataStream& operator<<(QDataStream& out, const InterpCol& c)
	{
		out << static_cast<int>(c.x.size()) << c.interpMode << c.extrapMode;
		for (size_t i = 0; i < c.x.size(); ++i)
		{
			out << c.x[i] << c.y[i];
		}
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, InterpCol& c)
	{
		int count;
		in >> count >> c.interpMode >> c.extrapMode;
		c.x.resize(count);
		c.y.resize(count);
		for (int i = 0; i < count; ++i)
		{
			in >> c.x[i] >> c.y[i];
		}
		return in;
	}

	struct PiecewiseCol
	{
		std::vector<int> sampleCount ;              // 采样点数
		std::vector<int>    idx;            // 序号
		std::vector<double> start;          // 起始
		std::vector<double> end;            // 结束
		std::vector<QString> expr;          // 表达式
	};
	inline QDataStream& operator<<(QDataStream& out, const PiecewiseCol& c)
	{
		out << static_cast<int>(c.idx.size());
		for (size_t i = 0; i < c.idx.size(); ++i)
		{
			out << c.sampleCount[i] << c.idx[i] << c.start[i] << c.end[i] << c.expr[i];
		}
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, PiecewiseCol& c)
	{
		int count;
		in >> count;
		c.sampleCount.resize(count);
		c.idx.resize(count);
		c.start.resize(count);
		c.end.resize(count);
		c.expr.resize(count);
		for (int i = 0; i < count; ++i)
		{
			in >> c.sampleCount[i] >> c.idx[i] >> c.start[i] >> c.end[i] >> c.expr[i];
		}
		return in;
	}

	struct StepCol
	{
		double xMin = -10;   // X 区间
		double xMax = 10;
		double pos = 0;   // 阶跃位置
		double v0 = 0;   // 起始值
		double v1 = 1;   // 终止值
	};
	inline QDataStream& operator<<(QDataStream& out, const StepCol& c)
	{
		out << c.xMin << c.xMax << c.pos << c.v0 << c.v1;
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, StepCol& c)
	{
		in >> c.xMin >> c.xMax >> c.pos >> c.v0 >> c.v1;
		return in;
	}

	struct TableCol
	{
		std::vector<double> x;          // 原始 x（幂函数时存幂指数 Ai）
		std::vector<double> y;          // 原始 y
		int xAxisType = 0;              // 0 线性 / 1 对数（仅普通表格使用）
		int yAxisType = 0;              // 0 线性 / 1 对数（仅普通表格使用）
		int extrapMode = 0;             // 0 常数外推 / 1 线性外推
		double x1 = 0.0;                // 偏移量（可平移/可缩放/幂函数使用）
		double x2 = 1.0;                // 缩放值（可缩放/幂函数使用）
		double x3 = 0.0;                // 横坐标下限（仅幂函数使用）
		double x4 = 10.0;               // 横坐标上限（仅幂函数使用）
	};
	inline QDataStream& operator<<(QDataStream& out, const TableCol& c)
	{
		out << static_cast<int>(c.x.size()) << c.xAxisType << c.yAxisType << c.extrapMode
		    << c.x1 << c.x2 << c.x3 << c.x4;
		for (size_t i = 0; i < c.x.size(); ++i)
		{
			out << c.x[i] << c.y[i];
		}
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, TableCol& c)
	{
		int count;
		in >> count >> c.xAxisType >> c.yAxisType >> c.extrapMode
		   >> c.x1 >> c.x2 >> c.x3 >> c.x4;
		c.x.resize(count);
		c.y.resize(count);
		for (int i = 0; i < count; ++i)
		{
			in >> c.x[i] >> c.y[i];
		}
		return in;
	}

	struct PeriodicCol
	{
		int order = 0;                  // 阶数 N
		double freq = 0.0;              // 频率 f
		double t0 = 0.0;                // 起始时间 t0
		double A0 = 0.0;                // 初始幅值 A0
		std::vector<double> A;          // 幅值 An
		std::vector<double> B;          // 幅值 Bn
	};
	inline QDataStream& operator<<(QDataStream& out, const PeriodicCol& c)
	{
		out << c.order << c.freq << c.t0 << c.A0;
		out << static_cast<int>(c.A.size());
		for (size_t i = 0; i < c.A.size(); ++i)
		{
			out << c.A[i] << c.B[i];
		}
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, PeriodicCol& c)
	{
		int count = 0;
		in >> c.order >> c.freq >> c.t0 >> c.A0 >> count;
		c.A.resize(count);
		c.B.resize(count);
		for (int i = 0; i < count; ++i)
		{
			in >> c.A[i] >> c.B[i];
		}
		return in;
	}
	struct ModulatedCol
	{
		double A0 = 0.0;    // 初始幅值
		double A = 0.0;     // 幅值
		double t0 = 0.0;    // 起始时间
		double f1 = 0.0;    // 频率1
		double f2 = 0.0;    // 频率2
	};
	struct ExpDecayCol
	{
		double A0 = 0.0;    // 初始幅值
		double Am = 0.0;    // 最大幅值
		double t0 = 0.0;    // 起始时间
		double td = 0.0;    // 衰减时间
	};
	inline QDataStream& operator<<(QDataStream& out, const ExpDecayCol& c)
	{
		out << c.A0 << c.Am << c.t0 << c.td;
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, ExpDecayCol& c)
	{
		in >> c.A0 >> c.Am >> c.t0 >> c.td;
		return in;
	}
	inline QDataStream& operator<<(QDataStream& out, const ModulatedCol& c)
	{
		out << c.A0 << c.A << c.t0 << c.f1 << c.f2;
		return out;
	}
	inline QDataStream& operator>>(QDataStream& in, ModulatedCol& c)
	{
		in >> c.A0 >> c.A >> c.t0 >> c.f1 >> c.f2;
		return in;
	}

	class MXFUNCTIONDATA_EXPORT mFunctionBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mFunctionBase(int id, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mFunctionBase() {}

		/*
		* 析构函数
		*/
		~mFunctionBase();

		/*
		* 获取函数ID
		*/
		int getFunctionId();

		/*
		* 设置函数名称
		*/
		void setFunctionName(const QString& name);

		/*
		* 获取函数名称
		*/
		QString getFunctionName();

		/*
		* 拷贝材料
		*/
		mFunctionBase* copyFunction(const int& id, const QString& name);

	public:
		/* 新增三行即可 */
		void setXRange(double min, double max) { _xRangeMin = min; _xRangeMax = max; }
		double xMin() const { return _xRangeMin; }
		double xMax() const { return _xRangeMax; }
		void setVarName(const QString& v) { _varName = v; }
		QString varName() const { return _varName; }

		// FunctionType 的 getter 和 setter
		int getFunctionType() const;
		void setFunctionType(const int type);

		// varRangeMin 的 getter 和 setter
		double getVarRangeMin() const { return _xRangeMin; }
		void   setVarRangeMin(double v) { _xRangeMin = v; }
		
		// varRangeMax 的 getter 和 setter
		double getVarRangeMax() const { return _xRangeMax; }
		void   setVarRangeMax(double v) { _xRangeMax = v; }

		// expression 的 getter 和 setter
		QString getExpression() const { return analytic().expr; }
		void    setExpression(const QString& e) { analytic().expr = e; }

		/* ====================== 解析函数数据 ====================== */

		AnalyticCol& analytic() { return _analyticCol; }

		const AnalyticCol& analytic() const { return _analyticCol; }   // 专门给 const 用

		void setAnalytic(const AnalyticCol& ana);
		/* 离散函数三列：序号、x、y */

		DiscreteCol& discrete() { return _discreteCol; }

		void setDiscrete(const DiscreteCol& dis);
		/* ====================== 插值函数数据 ====================== */

		InterpCol& interp() { return _interpCol; }
		void setInterp(const InterpCol& inte);

		/* ====================== 分段函数数据 ====================== */

		PiecewiseCol& piecewise() { return _piecewiseCol; }
		void setPiecewise(const PiecewiseCol& pie);
		/* ====================== 阶跃函数数据 ====================== */

		StepCol& step() { return _stepCol; }
		void setStep(const StepCol& step);

		/* ====================== 表格函数数据 ====================== */
		TableCol& table() { return _tableCol; }
		void setTable(const TableCol& tbl);

		/* ====================== 周期函数数据 ====================== */
		PeriodicCol& periodic() { return _periodicCol; }
		void setPeriodic(const PeriodicCol& per);

		/* ====================== 调制函数数据 ====================== */
		ModulatedCol& modulated() { return _modulatedCol; }
		void setModulated(const ModulatedCol& mod);

		/* ====================== 指数衰减函数数据 ====================== */
		ExpDecayCol& expDecay() { return _expDecayCol; }
		void setExpDecay(const ExpDecayCol& ed);

		friend QDataStream& operator>>(QDataStream& in, mFunctionBase& obj);
		friend QDataStream& InStreammFunctionBaseV3(QDataStream& in, mFunctionBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mFunctionBase& obj);

	private:
		//函数ID
		int _id;

		//函数名字
		QString _functionName;

		int  FunctionType = 0;          // 已有
		double _xRangeMin = -10.0;      // 新增：全局变量范围
		double _xRangeMax = 10.0;
		QString _varName = "x";         // 新增：变量名
		/*
		* 1.解析函数
		*/
		double varRangeMin = 0;
		double varRangeMax = 0;

		QString expression;

		AnalyticCol  _analyticCol;   // 解析 0
		DiscreteCol _discreteCol;   // 离散  1
		InterpCol   _interpCol;     // 插值  2
		PiecewiseCol _piecewiseCol; // 分段  3
		StepCol      _stepCol;      // 阶跃  4
		TableCol     _tableCol;     // 表格  5
		PeriodicCol  _periodicCol;  // 周期  9
		ModulatedCol _modulatedCol; // 调制 10
		ExpDecayCol  _expDecayCol;  // 指数衰减 11

	};
}
