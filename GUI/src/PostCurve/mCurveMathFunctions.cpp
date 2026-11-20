#include "mCurveMathFunctions.h"

//QT
#include <QHash>

//STL
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

namespace PostCurve
{
	namespace
	{
		constexpr double kPi = 3.14159265358979323846;
		constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

		//目录分类名（对齐《函数类型.xlsx》10 类；末项为本引擎补充项专用类）
		const QString kCatBasic    = QStringLiteral("基础数学函数");
		const QString kCatStat     = QStringLiteral("统计与特征值函数");
		const QString kCatInterp   = QStringLiteral("插值拟合与曲线处理");
		const QString kCatSpectrum = QStringLiteral("频谱与信号处理");
		const QString kCatMatrix   = QStringLiteral("矩阵/向量数组函数");
		const QString kCatString   = QStringLiteral("字符串与字符转换");
		const QString kCatFileIO   = QStringLiteral("文件 IO 与结果读取");
		const QString kCatSystem   = QStringLiteral("系统与 Tcl 交互");
		const QString kCatEngEval  = QStringLiteral("工程评价专用函数");
		const QString kCatCustom   = QStringLiteral("自定义扩展函数");
		const QString kCatExtra    = QStringLiteral("本引擎补充（未在参考目录）");

		// =============================================================================
		// 标量函数实现（逐点）
		// =============================================================================
		double fnAbs(double v)   { return std::fabs(v); }
		double fnAcos(double v)  { return std::acos(v); }
		double fnAsin(double v)  { return std::asin(v); }
		double fnAtan(double v)  { return std::atan(v); }
		double fnAtan2(double y, double x) { return std::atan2(y, x); }
		double fnCos(double v)   { return std::cos(v); }
		double fnSin(double v)   { return std::sin(v); }
		double fnTan(double v)   { return std::tan(v); }
		double fnCosh(double v)  { return std::cosh(v); }
		double fnSinh(double v)  { return std::sinh(v); }
		double fnTanh(double v)  { return std::tanh(v); }
		double fnAsinh(double v) { return std::asinh(v); }
		double fnAcosh(double v) { return std::acosh(v); }
		double fnAtanh(double v) { return std::atanh(v); }
		double fnExp(double v)   { return std::exp(v); }
		double fnLn(double v)    { return std::log(v); }
		double fnLog(double v)   { return std::log(v); }   //参考语义：log = ln 别名
		double fnLog10(double v) { return std::log10(v); }
		double fnLog2(double v)  { return std::log2(v); }
		double fnSqrt(double v)  { return std::sqrt(v); }
		double fnCint(double v)  { return std::trunc(v); }         //C 风格向零截断
		double fnInt(double v)   { return std::floor(v); }         //向下取整
		double fnFloor(double v) { return std::floor(v); }
		double fnCeil(double v)  { return std::ceil(v); }
		double fnRound(double v)
		{
			//四舍五入（.5 向 +∞ 进，对齐原 rint 行为）；先 floor 再比较余数，
			//避免 v+0.5 的浮点吞没（0.49999999999999994 会被误进位为 1）
			const double f = std::floor(v);
			return (v - f < 0.5) ? f : f + 1.0;
		}
		double fnRint(double v) { return fnRound(v); }
		double fnSign(double v)  { return v > 0.0 ? 1.0 : (v < 0.0 ? -1.0 : 0.0); }
		double fnDtor(double v)  { return v * kPi / 180.0; }
		double fnRtod(double v)  { return v * 180.0 / kPi; }
		double fnDeg2Rad(double v) { return v * kPi / 180.0; }
		double fnRad2Deg(double v) { return v * 180.0 / kPi; }
		//伪随机：线程局部 mt19937（会话内随机种子），每次调用取下一个 [0,1) 数
		double fnRand(double)
		{
			thread_local std::mt19937 gen{ std::random_device{}() };
			thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
			return dist(gen);
		}
		double fnMin(double a, double b) { return a < b ? a : b; }
		double fnMax(double a, double b) { return a > b ? a : b; }

		//标准正态累积分布 Φ(x)=0.5·(1+erf(x/√2))；官方 cdf(0)=0.5
		double fnCdf(double x)
		{
			return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
		}

		//标准正态分位（probit）：牛顿迭代求 Φ(x)=p 的根（Φ 单调、收敛域全局，
		//种子 0 起步收敛到机器精度）
		double fnInvcdf(double p)
		{
			if (p <= 0.0)
				return -std::numeric_limits<double>::infinity();
			if (p >= 1.0)
				return std::numeric_limits<double>::infinity();
			double x = 0.0;
			for (int it = 0; it < 100; ++it)
			{
				const double e = fnCdf(x) - p;
				const double pdf = std::exp(-0.5 * x * x) / std::sqrt(2.0 * kPi);
				double dx = e / std::max(pdf, 1e-300);
				if (x - dx > 8.0)          //极端尾部防过冲
					dx = e > 0.0 ? -(8.0 - x) : dx;
				if (x - dx < -8.0)
					dx = e < 0.0 ? -(x + 8.0) : dx;
				x -= dx;
				if (std::fabs(dx) < 4e-16 * (1.0 + std::fabs(x)))
					break;
			}
			return x;
		}

		// =============================================================================
		// 复数与 DSP 基础设施
		// =============================================================================
		struct Cx
		{
			double re = 0.0;
			double im = 0.0;
			Cx() = default;
			Cx(double r, double i = 0.0) : re(r), im(i) {}
			double abs() const { return std::sqrt(re * re + im * im); }
			Cx conj() const { return Cx(re, -im); }
		};
		inline Cx operator+(Cx a, Cx b) { return Cx(a.re + b.re, a.im + b.im); }
		inline Cx operator-(Cx a, Cx b) { return Cx(a.re - b.re, a.im - b.im); }
		inline Cx operator*(Cx a, Cx b) { return Cx(a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re); }
		inline Cx operator/(Cx a, double d) { return Cx(a.re / d, a.im / d); }

		//基-2 迭代 FFT（n 须为 2 的幂；inverse=true 时共轭旋转因子，结果未归一化）
		void fftRadix2(std::vector<Cx>& a, bool inverse)
		{
			const size_t n = a.size();
			for (size_t i = 1, j = 0; i < n; ++i)   //位反转重排
			{
				size_t bit = n >> 1;
				for (; j & bit; bit >>= 1)
					j ^= bit;
				j ^= bit;
				if (i < j)
					std::swap(a[i], a[j]);
			}
			const double sign = inverse ? 1.0 : -1.0;
			for (size_t len = 2; len <= n; len <<= 1)
			{
				for (size_t i = 0; i < n; i += len)
				{
					for (size_t k = 0; k < len / 2; ++k)
					{
						//旋转因子按角度直接计算（避免逐次递推的累积误差）
						const double ang = sign * 2.0 * kPi * double(k) / double(len);
						const Cx w(std::cos(ang), std::sin(ang));
						const Cx u = a[i + k];
						const Cx v = a[i + k + len / 2] * w;
						a[i + k] = u + v;
						a[i + k + len / 2] = u - v;
					}
				}
			}
		}

		//任意长度 FFT：2 的幂走基-2，其余 Bluestein（chirp-z，线性卷积补零到 2 的幂）
		bool fftAny(std::vector<Cx>& a, bool inverse, QString* err)
		{
			if (a.empty())
			{
				if (err)
					*err = QStringLiteral("数据为空");
				return false;
			}
			if (inverse)
			{
				//ifft = conj(fft(conj(x)))/n（正变换路径已支持任意长度）；
				//末尾再取共轭——FFT(conj(X))/N 实为 conj(IFFT(X))，须翻回
				for (Cx& v : a)
					v = v.conj();
				if (!fftAny(a, false, err))
					return false;
				const double inv = 1.0 / double(a.size());
				for (Cx& v : a)
				{
					v = v * inv;
					v = v.conj();
				}
				return true;
			}
			const int n = int(a.size());
			if ((n & (n - 1)) == 0)
			{
				fftRadix2(a, false);
				return true;
			}
			//Bluestein：X[k] = conj(c[k])·Σ_j (x[j]·conj(c[j]))·b[(k-j) mod m]，
			//c[l]=exp(+iπl²/n)（l² 偶函数 => 负滞后核 b[-l]=b[l]=c[l]）
			std::vector<Cx> c(n);
			for (int k = 0; k < n; ++k)
			{
				const long long kk = (long long(k) * k) % (2LL * n);   //k² 取模 2n 防大 n 精度损失
				const double ang = kPi * double(kk) / double(n);
				c[k] = Cx(std::cos(ang), std::sin(ang));
			}
			int m = 1;
			while (m < 2 * n - 1)
				m <<= 1;
			std::vector<Cx> sig(m, Cx());
			std::vector<Cx> ker(m, Cx());
			for (int k = 0; k < n; ++k)
			{
				sig[k] = a[k] * c[k].conj();
				ker[k] = c[k];
				if (k > 0)
					ker[m - k] = c[k];   //负滞后（核为偶函数）
			}
			fftRadix2(sig, false);
			fftRadix2(ker, false);
			for (int k = 0; k < m; ++k)
				sig[k] = sig[k] * ker[k];
			fftRadix2(sig, true);
			for (int k = 0; k < m; ++k)
				sig[k] = sig[k] / double(m);
			for (int k = 0; k < n; ++k)
				a[k] = c[k].conj() * sig[k];
			return true;
		}

		//修正贝塞尔 I0（Kaiser 窗用）：级数求和
		double besselI0(double x)
		{
			double sum = 1.0, term = 1.0;
			for (int k = 1; k < 40; ++k)
			{
				term *= (x * x / 4.0) / (double(k) * double(k));
				sum += term;
				if (term < 1e-12 * sum)
					break;
			}
			return sum;
		}

		// =============================================================================
		// 插值/拟合基础（自由函数，实参为数据序列）
		// =============================================================================
		bool checkVec(const QVector<double>& v, QString* err, const char* what = "向量")
		{
			if (v.isEmpty())
			{
				if (err)
					*err = QStringLiteral("%1数据为空").arg(QString::fromUtf8(what));
				return false;
			}
			return true;
		}

		//单调性检查（线性插值/样条前提）；返回 true=严格递增
		bool checkAscending(const QVector<double>& x, QString* err, const char* what = "x")
		{
			for (int i = 1; i < x.size(); ++i)
				if (x.at(i) <= x.at(i - 1))
				{
					if (err)
						*err = QStringLiteral("%1 序列须严格递增").arg(QString::fromUtf8(what));
					return false;
				}
			return true;
		}

		//线性插值单点（x 须升序，区间外取端点值）——与求值器 interpolate 同规则
		double interpAt(const QVector<double>& xs, const QVector<double>& ys, double x)
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
			if (x1 == x0)
				return ys[i];
			return ys[i - 1] + (ys[i] - ys[i - 1]) * (x - x0) / (x1 - x0);
		}

		//自然三次样条：解三对角得二阶导，返回后经 splineEval 求值
		QVector<double> splineSecondDeriv(const QVector<double>& x, const QVector<double>& y)
		{
			const int n = x.size();
			QVector<double> d2(n, 0.0), u(n, 0.0);
			for (int i = 1; i + 1 < n; ++i)
			{
				const double sig = (x.at(i) - x.at(i - 1)) / (x.at(i + 1) - x.at(i - 1));
				const double p = sig * d2.at(i - 1) + 2.0;
				d2[i] = (sig - 1.0) / p;
				u[i] = (y.at(i + 1) - y.at(i)) / (x.at(i + 1) - x.at(i))
					- (y.at(i) - y.at(i - 1)) / (x.at(i) - x.at(i - 1));
				u[i] = (6.0 * u[i] / (x.at(i + 1) - x.at(i - 1)) - sig * u.at(i - 1)) / p;
			}
			for (int k = n - 2; k >= 1; --k)
				d2[k] = d2.at(k) * d2.at(k + 1) + u.at(k);
			return d2;
		}

		double splineEval(const QVector<double>& x, const QVector<double>& y,
			const QVector<double>& d2, double q)
		{
			const int n = x.size();
			if (n == 1 || q <= x.first())
				return y.first();
			if (q >= x.last())
				return y.last();
			//二分定位区间
			int lo = 0, hi = n - 1;
			while (hi - lo > 1)
			{
				const int mid = (lo + hi) / 2;
				if (x.at(mid) > q)
					hi = mid;
				else
					lo = mid;
			}
			const double h = x.at(hi) - x.at(lo);
			const double a = (x.at(hi) - q) / h;
			const double b = (q - x.at(lo)) / h;
			return a * y.at(lo) + b * y.at(hi)
				+ ((a * a * a - a) * d2.at(lo) + (b * b * b - b) * d2.at(hi)) * (h * h) / 6.0;
		}

		//Akima 插值：返回每点斜率 m[i]（经典 5 点式，端点斜率线性外推）
		QVector<double> akimaSlopes(const QVector<double>& x, const QVector<double>& y)
		{
			const int n = x.size();
			QVector<double> s(n - 1, 0.0);
			for (int i = 0; i + 1 < n; ++i)
				s[i] = (y.at(i + 1) - y.at(i)) / (x.at(i + 1) - x.at(i));
			QVector<double> m(n, 0.0);
			for (int i = 0; i < n; ++i)
			{
				//s[-1..n+1] 线性外推：s[-1]=2s0-s1, s[n]=2s[n-1]-s[n-2]...
				const double sm2 = i >= 2 ? s.at(i - 2) : 2.0 * s.at(0) - s.at(1);
				const double sm1 = i >= 1 ? s.at(i - 1) : 2.0 * s.at(0) - s.at(1);
				const double sp0 = i <= n - 2 ? s.at(i) : 2.0 * s.at(n - 2) - s.at(n - 3);
				const double sp1 = i <= n - 3 ? s.at(i + 1) : 2.0 * s.at(n - 2) - s.at(n - 3);
				const double w1 = std::fabs(sp1 - sp0);
				const double w2 = std::fabs(sm1 - sm2);
				if (w1 + w2 > 0.0)
					m[i] = (w1 * sm1 + w2 * sp0) / (w1 + w2);
				else
					m[i] = (sm1 + sp0) / 2.0;
			}
			return m;
		}

		//Akima 区间三次式系数：t = q - x[i]，p(t)=c0+c1·t+c2·t²+c3·t³（Hermite 基）
		void akimaCoeffs(const QVector<double>& x, const QVector<double>& y,
			const QVector<double>& m, int i, double c[4])
		{
			const double dx = x.at(i + 1) - x.at(i);
			const double t0 = y.at(i), t1 = m.at(i), t2 = m.at(i + 1), t3 = y.at(i + 1);
			c[0] = t0;
			c[1] = t1;
			c[2] = (3.0 * (t3 - t0) / dx - 2.0 * t1 - t2) / dx;
			c[3] = ((t1 + t2) / dx - 2.0 * (t3 - t0) / (dx * dx)) / dx;
		}

		//稠密线性方程组高斯消元（列主元）；成功返回 true，sol 就地写入
		bool gaussSolve(std::vector<std::vector<double>> A, std::vector<double> b,
			std::vector<double>& sol, QString* err)
		{
			const int n = int(A.size());
			for (int col = 0; col < n; ++col)
			{
				int piv = col;
				for (int r = col + 1; r < n; ++r)
					if (std::fabs(A[r][col]) > std::fabs(A[piv][col]))
						piv = r;
				if (std::fabs(A[piv][col]) < 1e-300)
				{
					if (err)
						*err = QStringLiteral("线性方程组奇异（主元为零）");
					return false;
				}
				if (piv != col)
				{
					std::swap(A[piv], A[col]);
					std::swap(b[piv], b[col]);
				}
				for (int r = col + 1; r < n; ++r)
				{
					const double f = A[r][col] / A[col][col];
					for (int c = col; c < n; ++c)
						A[r][c] -= f * A[col][c];
					b[r] -= f * b[col];
				}
			}
			sol.assign(n, 0.0);
			for (int r = n - 1; r >= 0; --r)
			{
				double s = b[r];
				for (int c = r + 1; c < n; ++c)
					s -= A[r][c] * sol[c];
				sol[r] = s / A[r][r];
			}
			return true;
		}

		//Durand-Kerner 复多项式求根（coef 升幂、已剥离高位零）；输出根（与 coef 同序无保证）
		bool polyRoots(const QVector<double>& coef, std::vector<Cx>& roots, QString* err)
		{
			int deg = coef.size() - 1;
			while (deg > 0 && std::fabs(coef.at(deg)) < 1e-14)
				--deg;
			if (deg <= 0)
			{
				if (err)
					*err = QStringLiteral("多项式无常根（常数或零多项式）");
				return false;
			}
			//归一化为首一多项式（升幂存 coef[0..deg]）
			std::vector<double> a(deg + 1);
			for (int k = 0; k <= deg; ++k)
				a[k] = coef.at(k) / coef.at(deg);
			//初值 (0.4+0.9i)^k 缩放
			double scale = 1.0 + std::fabs(a[deg - 1]);
			for (int k = 0; k < deg; ++k)
				scale = std::max(scale, std::fabs(a[k]));
			std::vector<Cx> r(deg);
			Cx seed(0.4, 0.9);
			for (int k = 0; k < deg; ++k)
			{
				r[k] = seed;
				seed = seed * Cx(0.4, 0.9);
				r[k] = Cx(r[k].re * scale, r[k].im * scale);
			}
			for (int iter = 0; iter < 200; ++iter)
			{
				double maxDelta = 0.0;
				for (int k = 0; k < deg; ++k)
				{
					//p(r_k)（Horner 升幂）
					Cx p(0.0);
					for (int j = deg; j >= 0; --j)
						p = p * r[k] + Cx(a[j]);
					Cx denom(1.0);
					for (int j = 0; j < deg; ++j)
						if (j != k)
							denom = denom * (r[k] - r[j]);
					const double dr = denom.re * denom.re + denom.im * denom.im;
					if (dr < 1e-300)
						continue;
					const Cx step((p.re * denom.re + p.im * denom.im) / dr,
						(p.im * denom.re - p.re * denom.im) / dr);
					r[k] = r[k] - step;
					maxDelta = std::max(maxDelta, std::fabs(step.re) + std::fabs(step.im));
				}
				if (maxDelta < 1e-14 * scale)
					break;
			}
			roots = r;
			return true;
		}

		//方阵提取：m 行主序展平 -> n；非完全平方数报错
		bool squareMatrix(const QVector<double>& m, int& n,
			std::vector<std::vector<double>>& A, QString* err)
		{
			const int len = m.size();
			const int side = int(std::lround(std::sqrt(double(len))));
			if (side * side != len || side <= 0)
			{
				if (err)
					*err = QStringLiteral("矩阵数据长度 %1 不是完全平方数（行主序方阵约定）").arg(len);
				return false;
			}
			n = side;
			A.assign(n, std::vector<double>(n, 0.0));
			for (int r = 0; r < n; ++r)
				for (int c = 0; c < n; ++c)
					A[r][c] = m.at(r * n + c);
			return true;
		}

		//Faddeev–LeVerrier 特征多项式：p(λ)=λ^n+c1·λ^(n-1)+...+cn（返回 c1..cn）
		std::vector<double> charPoly(const std::vector<std::vector<double>>& A)
		{
			const int n = int(A.size());
			std::vector<double> c(n, 0.0);
			//M 初始为 I（存扁平），逐步 M_k = A·M_{k-1} + c_k·I
			std::vector<double> M(n * n, 0.0);
			for (int i = 0; i < n; ++i)
				M[i * n + i] = 1.0;
			for (int k = 1; k <= n; ++k)
			{
				//AM = A·M
				std::vector<double> AM(n * n, 0.0);
				for (int r = 0; r < n; ++r)
					for (int c2 = 0; c2 < n; ++c2)
					{
						double s = 0.0;
						for (int j = 0; j < n; ++j)
							s += A[r][j] * M[j * n + c2];
						AM[r * n + c2] = s;
					}
				double tr = 0.0;
				for (int i = 0; i < n; ++i)
					tr += AM[i * n + i];
				c[k - 1] = -tr / double(k);
				for (int i = 0; i < n; ++i)
					M[i * n + i] = AM[i * n + i] + c[k - 1];
				for (int r = 0; r < n; ++r)
					for (int c2 = 0; c2 < n; ++c2)
						if (r != c2)
							M[r * n + c2] = AM[r * n + c2];
			}
			return c;
		}

		//复数高斯消元（逆迭代求特征向量用）
		bool gaussSolveCx(std::vector<std::vector<Cx>> A, std::vector<Cx> b, std::vector<Cx>& sol)
		{
			const int n = int(A.size());
			for (int col = 0; col < n; ++col)
			{
				int piv = col;
				for (int r = col + 1; r < n; ++r)
					if (A[r][col].abs() > A[piv][col].abs())
						piv = r;
				if (A[piv][col].abs() < 1e-300)
					return false;
				if (piv != col)
				{
					std::swap(A[piv], A[col]);
					std::swap(b[piv], b[col]);
				}
				for (int r = col + 1; r < n; ++r)
				{
					const double dr = A[col][col].re * A[col][col].re + A[col][col].im * A[col][col].im;
					const Cx f((A[r][col].re * A[col][col].re + A[r][col].im * A[col][col].im) / dr,
						(A[r][col].im * A[col][col].re - A[r][col].re * A[col][col].im) / dr);
					for (int c = col; c < n; ++c)
						A[r][c] = A[r][c] - A[col][c] * f;
					b[r] = b[r] - b[col] * f;
				}
			}
			sol.assign(n, Cx());
			for (int r = n - 1; r >= 0; --r)
			{
				Cx s = b[r];
				for (int c = r + 1; c < n; ++c)
					s = s - A[r][c] * sol[c];
				const double dr = A[r][r].re * A[r][r].re + A[r][r].im * A[r][r].im;
				sol[r] = Cx((s.re * A[r][r].re + s.im * A[r][r].im) / dr,
					(s.im * A[r][r].re - s.re * A[r][r].im) / dr);
			}
			return true;
		}

		// =============================================================================
		// 聚合函数实现（vector -> scalar）
		// =============================================================================
		double aggMean(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			double s = 0.0;
			for (double v : y)
				s += v;
			return s / y.size();
		}

		double aggMedian(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			QVector<double> s = y;
			std::sort(s.begin(), s.end());
			const int n = s.size();
			return (n % 2 == 1) ? s.at(n / 2)
			                    : 0.5 * (s.at(n / 2 - 1) + s.at(n / 2));
		}

		double aggRms(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			double s = 0.0;
			for (double v : y)
				s += v * v;
			return std::sqrt(s / y.size());
		}

		double aggSum(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			double s = 0.0;
			for (double v : y)
				s += v;
			return s;
		}

		//总体方差（除 n）；单点视为无离散，返回 0
		double aggVariance(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			const int n = y.size();
			if (n == 1)
				return 0.0;
			double mean = 0.0;
			for (double v : y)
				mean += v;
			mean /= n;
			double s = 0.0;
			for (double v : y)
				s += (v - mean) * (v - mean);
			return s / n;
		}

		//偏度 g1 = m3/m2^1.5（矩比式）；无离散度或点数不足时报错
		double aggSkewness(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			const int n = y.size();
			if (n < 3)
			{
				if (err)
					*err = QStringLiteral("skewness 至少需要 3 个数据点");
				return kNaN;
			}
			double mean = 0.0;
			for (double v : y)
				mean += v;
			mean /= n;
			double m2 = 0.0, m3 = 0.0;
			for (double v : y)
			{
				const double d = v - mean;
				m2 += d * d;
				m3 += d * d * d;
			}
			m2 /= n;
			m3 /= n;
			if (m2 <= 0.0)
			{
				if (err)
					*err = QStringLiteral("skewness 需要非零离散度");
				return kNaN;
			}
			return m3 / std::pow(m2, 1.5);
		}

		double aggAvgdev(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			double mean = 0.0;
			for (double v : y)
				mean += v;
			mean /= y.size();
			double s = 0.0;
			for (double v : y)
				s += std::fabs(v - mean);
			return s / y.size();
		}

		//梯形积分 ∫y dx（分段求和，x 降序自然得负值）；两序列须等长
		double trapz(const QVector<double>& y, const QVector<double>& x, QString* err)
		{
			if (!checkVec(y, err, "y向量"))
				return kNaN;
			if (!checkVec(x, err, "x向量"))
				return kNaN;
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("y 与 x 数据长度不一致（%1 ≠ %2）").arg(y.size()).arg(x.size());
				return kNaN;
			}
			double s = 0.0;
			for (int i = 1; i < y.size(); ++i)
				s += (x.at(i) - x.at(i - 1)) * (y.at(i) + y.at(i - 1)) * 0.5;
			return s;
		}

		//Pearson 相关系数；两路数据须等长且离散度非零
		double aggCorrel(const QVector<double>& a, const QVector<double>& b, QString* err)
		{
			if (!checkVec(a, err, "向量1") || !checkVec(b, err, "向量2"))
				return kNaN;
			if (a.size() != b.size())
			{
				if (err)
					*err = QStringLiteral("两路数据长度不一致（%1 ≠ %2）").arg(a.size()).arg(b.size());
				return kNaN;
			}
			const int n = a.size();
			if (n < 2)
			{
				if (err)
					*err = QStringLiteral("correl 至少需要 2 个数据点");
				return kNaN;
			}
			double ma = 0.0, mb = 0.0;
			for (int i = 0; i < n; ++i)
			{
				ma += a.at(i);
				mb += b.at(i);
			}
			ma /= n;
			mb /= n;
			double sab = 0.0, sa = 0.0, sb = 0.0;
			for (int i = 0; i < n; ++i)
			{
				const double da = a.at(i) - ma;
				const double db = b.at(i) - mb;
				sab += da * db;
				sa += da * da;
				sb += db * db;
			}
			if (sa <= 0.0 || sb <= 0.0)
			{
				if (err)
					*err = QStringLiteral("correl 需要两路数据均有非零离散度");
				return kNaN;
			}
			return sab / std::sqrt(sa * sb);
		}

		double aggIndexofmax(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			int imax = 0;
			for (int i = 1; i < y.size(); ++i)
				if (y.at(i) > y.at(imax))
					imax = i;
			return double(imax);   //首个最大值点索引
		}

		double aggIndexofmin(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			int imin = 0;
			for (int i = 1; i < y.size(); ++i)
				if (y.at(i) < y.at(imin))
					imin = i;
			return double(imin);
		}

		//过零计数：以 0 为正负分界（0 归非负侧），相邻点异号记一次
		double aggZerocrossings(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return kNaN;
			int count = 0;
			for (int i = 1; i < y.size(); ++i)
				if ((y.at(i - 1) < 0.0) != (y.at(i) < 0.0))
					++count;
			return double(count);
		}

		// =============================================================================
		// 向量变换实现（vector -> vector）
		// =============================================================================
		QVector<double> xfmDelta(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return {};
			QVector<double> out(y.size(), 0.0);
			for (int i = 1; i < y.size(); ++i)
				out[i] = y.at(i) - y.at(i - 1);
			return out;
		}

		//峰点提取：局部极大值处保留幅值，其余置 0（端点不计）；保长
		QVector<double> xfmPeaks(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return {};
			QVector<double> out(y.size(), 0.0);
			for (int i = 1; i + 1 < y.size(); ++i)
				if (y.at(i) > y.at(i - 1) && y.at(i) > y.at(i + 1))
					out[i] = y.at(i);
			return out;
		}

		// =============================================================================
		// 新增：插值拟合（统一签名包装见目录构建）
		// =============================================================================
		QVector<double> xfLininterp(const QVector<double>& x, const QVector<double>& y,
			const QVector<double>& xq, QString* err)
		{
			if (!checkVec(y, err, "y") || !checkVec(x, err, "x") || !checkVec(xq, err, "xq"))
				return {};
			if (!checkAscending(x, err, "x"))
				return {};
			QVector<double> out;
			out.reserve(xq.size());
			for (double q : xq)
				out.push_back(interpAt(x, y, q));
			return out;
		}

		//对数域插值：x 与 xq 取 log10 后线性插值（要求 x、xq 均 > 0）
		QVector<double> xfLogint(const QVector<double>& x, const QVector<double>& y,
			const QVector<double>& xq, QString* err)
		{
			for (double v : x)
				if (v <= 0.0)
				{
					if (err)
						*err = QStringLiteral("logint 要求 x 序列均为正数");
					return {};
				}
			for (double v : xq)
				if (v <= 0.0)
				{
					if (err)
						*err = QStringLiteral("logint 要求查询点均为正数");
					return {};
				}
			QVector<double> lx;
			lx.reserve(x.size());
			for (double v : x)
				lx.push_back(std::log10(v));
			QVector<double> lq;
			lq.reserve(xq.size());
			for (double v : xq)
				lq.push_back(std::log10(v));
			return xfLininterp(lx, y, lq, err);
		}

		QVector<double> xfSpline(const QVector<double>& x, const QVector<double>& y,
			const QVector<double>& xq, QString* err)
		{
			if (!checkVec(y, err, "y") || !checkVec(x, err, "x") || !checkVec(xq, err, "xq"))
				return {};
			if (x.size() < 2)
			{
				if (err)
					*err = QStringLiteral("spline 至少需要 2 个数据点");
				return {};
			}
			if (!checkAscending(x, err, "x"))
				return {};
			const QVector<double> d2 = splineSecondDeriv(x, y);
			QVector<double> out;
			out.reserve(xq.size());
			for (double q : xq)
				out.push_back(splineEval(x, y, d2, q));
			return out;
		}

		QVector<double> akimaCommon(const QVector<double>& x, const QVector<double>& y,
			const QVector<double>& xq, bool derivative, QString* err)
		{
			if (!checkVec(y, err, "y") || !checkVec(x, err, "x") || !checkVec(xq, err, "xq"))
				return {};
			if (x.size() < 5)
			{
				if (err)
					*err = QStringLiteral("akima 至少需要 5 个数据点");
				return {};
			}
			if (!checkAscending(x, err, "x"))
				return {};
			const QVector<double> m = akimaSlopes(x, y);
			QVector<double> out;
			out.reserve(xq.size());
			for (double q : xq)
			{
				//定位区间（夹取到端点区间，端值直接取/求导）
				if (q <= x.first())
				{
					double c[4];
					akimaCoeffs(x, y, m, 0, c);
					const double t = q - x.first();
					out.push_back(derivative ? c[1] : (c[0] + t * (c[1] + t * (c[2] + t * c[3]))));
					continue;
				}
				if (q >= x.last())
				{
					const int i = x.size() - 2;
					double c[4];
					akimaCoeffs(x, y, m, i, c);
					const double t = q - x.at(i);
					out.push_back(derivative ? c[1] : (c[0] + t * (c[1] + t * (c[2] + t * c[3]))));
					continue;
				}
				const auto it = std::lower_bound(x.cbegin(), x.cend(), q);
				const int i = int(it - x.cbegin()) - 1;
				double c[4];
				akimaCoeffs(x, y, m, i, c);
				const double t = q - x.at(i);
				out.push_back(derivative ? (c[1] + t * (2.0 * c[2] + 3.0 * t * c[3]))
				                         : (c[0] + t * (c[1] + t * (c[2] + t * c[3]))));
			}
			return out;
		}

		QVector<double> xfAkima(const QVector<double>& x, const QVector<double>& y,
			const QVector<double>& xq, QString* err)
		{
			return akimaCommon(x, y, xq, false, err);
		}

		QVector<double> xfAkimaDeriv(const QVector<double>& x, const QVector<double>& y,
			const QVector<double>& xq, QString* err)
		{
			return akimaCommon(x, y, xq, true, err);
		}

		//多项式最小二乘拟合（法方程 + 高斯消元）；返回升幂系数 [c0..cn]
		QVector<double> xfPolyfit(const QVector<double>& x, const QVector<double>& y,
			double degree, QString* err)
		{
			if (!checkVec(y, err, "y") || !checkVec(x, err, "x"))
				return {};
			const int n = int(degree);
			if (degree < 0.0 || double(n) != degree)
			{
				if (err)
					*err = QStringLiteral("polyfit 阶数须为非负整数");
				return {};
			}
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("x 与 y 数据长度不一致");
				return {};
			}
			if (x.size() < n + 1)
			{
				if (err)
					*err = QStringLiteral("数据点数 %1 不足拟合阶数+1（%2）").arg(x.size()).arg(n + 1);
				return {};
			}
			//法方程 AᵀA·c = Aᵀy
			std::vector<std::vector<double>> ata(n + 1, std::vector<double>(n + 1, 0.0));
			std::vector<double> aty(n + 1, 0.0);
			for (int r = 0; r <= n; ++r)
				for (int c = 0; c <= n; ++c)
					for (int i = 0; i < x.size(); ++i)
						ata[r][c] += std::pow(x.at(i), r + c);
			for (int r = 0; r <= n; ++r)
				for (int i = 0; i < x.size(); ++i)
					aty[r] += std::pow(x.at(i), r) * y.at(i);
			std::vector<double> sol;
			if (!gaussSolve(ata, aty, sol, err))
				return {};
			QVector<double> out;
			for (double v : sol)
				out.push_back(v);
			return out;
		}

		//多项式求值（coef 升幂，Horner）在 xq 各点取值
		QVector<double> xfPolyval(const QVector<double>& coef, const QVector<double>& xq, QString* err)
		{
			if (!checkVec(coef, err, "coef") || !checkVec(xq, err, "xq"))
				return {};
			QVector<double> out;
			out.reserve(xq.size());
			for (double q : xq)
			{
				double acc = 0.0;
				for (int k = coef.size() - 1; k >= 0; --k)
					acc = acc * q + coef.at(k);
				out.push_back(acc);
			}
			return out;
		}

		//多项式实根（Durand-Kerner；返回所有根的实部，顺序与 polyrootsimag 一致）
		QVector<double> xfPolyrootsReal(const QVector<double>& coef, QString* err)
		{
			std::vector<Cx> roots;
			if (!polyRoots(coef, roots, err))
				return {};
			QVector<double> out;
			for (const Cx& r : roots)
				out.push_back(r.re);
			return out;
		}

		QVector<double> xfPolyrootsImag(const QVector<double>& coef, QString* err)
		{
			std::vector<Cx> roots;
			if (!polyRoots(coef, roots, err))
				return {};
			QVector<double> out;
			for (const Cx& r : roots)
				out.push_back(r.im);
			return out;
		}

		//指数拟合 y ≈ a·e^(bx)：仅取正值样本线性化 ln y = ln a + bx，返回 x 上的拟合曲线
		QVector<double> xfExpfit(const QVector<double>& x, const QVector<double>& y, QString* err)
		{
			if (!checkVec(y, err, "y") || !checkVec(x, err, "x"))
				return {};
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("x 与 y 数据长度不一致");
				return {};
			}
			double sx = 0.0, sxx = 0.0, sl = 0.0, sxl = 0.0;
			int m = 0;
			for (int i = 0; i < y.size(); ++i)
			{
				if (y.at(i) <= 0.0)
					continue;   //负值/零值样本不参与（ln 无定义）
				const double ly = std::log(y.at(i));
				sx += x.at(i);
				sxx += x.at(i) * x.at(i);
				sl += ly;
				sxl += x.at(i) * ly;
				++m;
			}
			if (m < 2)
			{
				if (err)
					*err = QStringLiteral("expfit 至少需要 2 个正值样本（y>0）");
				return {};
			}
			const double den = double(m) * sxx - sx * sx;
			if (std::fabs(den) < 1e-300)
			{
				if (err)
					*err = QStringLiteral("expfit 需要 x 具有非零离散度");
				return {};
			}
			const double b = (double(m) * sxl - sx * sl) / den;
			const double lna = (sl - b * sx) / double(m);
			const double a = std::exp(lna);
			QVector<double> out;
			out.reserve(x.size());
			for (double v : x)
				out.push_back(a * std::exp(b * v));
			return out;
		}

		//曲线同步：把曲线 a（ys[0]）按参考横坐标（ys[1]）线性重采样
		QVector<double> xfCurvesync(const QVector<double>& ya, const QVector<double>& xa,
			const QVector<double>& yref, QString* err)
		{
			if (!checkVec(ya, err, "曲线") || !checkVec(xa, err, "曲线x") || !checkVec(yref, err, "参考x"))
				return {};
			if (!checkAscending(xa, err, "曲线x"))
				return {};
			QVector<double> out;
			out.reserve(yref.size());
			for (double q : yref)
				out.push_back(interpAt(xa, ya, q));
			return out;
		}

		// =============================================================================
		// 新增：频谱与信号处理
		// =============================================================================
		//单边谱长度：N/2+1（N 偶）或 (N+1)/2（N 奇）
		int oneSidedLen(int n) { return n / 2 + 1; }

		//实信号 -> 复谱（完整谱，未归一化）；失败返回空
		bool realFft(const QVector<double>& y, std::vector<Cx>& spec, QString* err)
		{
			if (!checkVec(y, err))
				return false;
			spec.assign(y.size(), Cx());
			for (int i = 0; i < y.size(); ++i)
				spec[i] = Cx(y.at(i));
			return fftAny(spec, false, err);
		}

		QVector<double> xfFftComponent(const QVector<double>& y, int comp, QString* err)
		{
			//comp: 0=幅值 1=实部 2=虚部 3=相位
			std::vector<Cx> spec;
			if (!realFft(y, spec, err))
				return {};
			const int n = int(spec.size());
			const int m = oneSidedLen(n);
			QVector<double> out(m, 0.0);
			for (int k = 0; k < m; ++k)
			{
				const bool edge = (k == 0) || (k == n - k);   //DC 与 Nyquist 不加倍
				const double scale = edge ? 1.0 / double(n) : 2.0 / double(n);
				const double re = spec[k].re * scale;
				const double im = spec[k].im * scale;
				switch (comp)
				{
				case 0: out[k] = std::sqrt(re * re + im * im); break;
				case 1: out[k] = re; break;
				case 2: out[k] = im; break;
				case 3: out[k] = std::atan2(spec[k].im, spec[k].re); break;
				}
			}
			return out;
		}

		//单边幅值谱 -> 时域重构（共轭对称、零相位；输出 2(M-1) 点）
		QVector<double> xfIfft(const QVector<double>& mag, QString* err)
		{
			if (!checkVec(mag, err))
				return {};
			const int m = mag.size();
			const int n = 2 * (m - 1);
			if (n <= 0)
			{
				if (err)
					*err = QStringLiteral("ifft 输入至少需要 2 个谱线");
				return {};
			}
			std::vector<Cx> spec(n, Cx());
			spec[0] = Cx(mag.at(0) * double(n));
			for (int k = 1; k < m; ++k)
			{
				const bool nyq = (k == n - k);
				const Cx v = mag.at(k) * (nyq ? double(n) : double(n) / 2.0);
				spec[k] = v;
				if (k != n - k)
					spec[n - k] = v.conj();
			}
			if (!fftAny(spec, true, err))
				return {};
			QVector<double> out;
			out.reserve(n);
			for (const Cx& v : spec)
				out.push_back(v.re);
			return out;
		}

		QVector<double> xfDftComponent(const QVector<double>& y, int comp, QString* err)
		{
			if (!checkVec(y, err))
				return {};
			const int n = y.size();
			const int m = oneSidedLen(n);
			QVector<double> out(m, 0.0);
			for (int k = 0; k < m; ++k)
			{
				double re = 0.0, im = 0.0;
				for (int t = 0; t < n; ++t)
				{
					const double ang = -2.0 * kPi * double(k) * double(t) / double(n);
					re += y.at(t) * std::cos(ang);
					im += y.at(t) * std::sin(ang);
				}
				const bool edge = (k == 0) || (k == n - k);
				const double scale = edge ? 1.0 / double(n) : 2.0 / double(n);
				switch (comp)
				{
				case 0: out[k] = std::sqrt(re * re + im * im) * scale; break;
				case 1: out[k] = re * scale; break;
				case 2: out[k] = im * scale; break;
				case 3: out[k] = std::atan2(im, re); break;
				}
			}
			return out;
		}

		QVector<double> xfIdft(const QVector<double>& mag, QString* err)
		{
			//与 ifft 同语义，直接 DFT 逆（O(n²)）
			return xfIfft(mag, err);
		}

		//Welch 分段参数：默认 4 段不重叠；段长不足时自动减段
		int welchSegments(int n)
		{
			int k = 4;
			while (k > 1 && n / k < 8)
				--k;
			return std::max(1, k);
		}

		QVector<double> xfPsd(const QVector<double>& y, QString* err)
		{
			if (!checkVec(y, err))
				return {};
			const int n = y.size();
			const int K = welchSegments(n);
			const int L = n / K;               //段长（截去尾段不足）
			if (L < 2)
			{
				if (err)
					*err = QStringLiteral("psd 数据点数不足（至少 2 点/段）");
				return {};
			}
			//Hann 窗及能量归一因子
			std::vector<double> w(L);
			double u = 0.0;
			for (int i = 0; i < L; ++i)
			{
				w[i] = 0.5 * (1.0 - std::cos(2.0 * kPi * double(i) / double(L - 1)));
				u += w[i] * w[i];
			}
			const int m = L / 2 + 1;
			QVector<double> out(m, 0.0);
			for (int s = 0; s < K; ++s)
			{
				std::vector<Cx> seg(L, Cx());
				for (int i = 0; i < L; ++i)
					seg[i] = Cx(y.at(s * L + i) * w[i]);
				if (!fftAny(seg, false, err))
					return {};
				for (int k = 0; k < m; ++k)
				{
					const bool edge = (k == 0) || (k == L - k);
					const double scale = edge ? 1.0 / u : 2.0 / u;
					out[k] += (seg[k].re * seg[k].re + seg[k].im * seg[k].im) * scale;
				}
			}
			for (int k = 0; k < m; ++k)
				out[k] /= double(K);
			return out;
		}

		//相干函数（标准 H1 型估计）：coh² = |E[Sxy]|²/(E[Sxx]·E[Syy])，单边；
		//Welch 分段 + Hann 窗（与 psd 一致），E 为分段平均
		QVector<double> xfCoherence(const QVector<double>& a, const QVector<double>& b, QString* err)
		{
			if (!checkVec(a, err, "向量1") || !checkVec(b, err, "向量2"))
				return {};
			if (a.size() != b.size())
			{
				if (err)
					*err = QStringLiteral("两路数据长度不一致");
				return {};
			}
			const int n = a.size();
			const int K = welchSegments(n);
			const int L = n / K;
			if (L < 2)
			{
				if (err)
					*err = QStringLiteral("coherence 数据点数不足");
				return {};
			}
			const int m = L / 2 + 1;
			//Hann 窗（能量归一不参与相干比值，只保证与 psd 同一约定）
			std::vector<double> w(L);
			for (int i = 0; i < L; ++i)
				w[i] = 0.5 * (1.0 - std::cos(2.0 * kPi * double(i) / double(L - 1)));
			std::vector<Cx> sxy(m, Cx());
			std::vector<double> sxx(m, 0.0), syy(m, 0.0);
			for (int s = 0; s < K; ++s)
			{
				std::vector<Cx> fa(L), fb(L);
				for (int i = 0; i < L; ++i)
				{
					fa[i] = Cx(a.at(s * L + i) * w[i]);
					fb[i] = Cx(b.at(s * L + i) * w[i]);
				}
				if (!fftAny(fa, false, err) || !fftAny(fb, false, err))
					return {};
				for (int k = 0; k < m; ++k)
				{
					sxy[k] = sxy[k] + fa[k].conj() * fb[k];
					sxx[k] += fa[k].re * fa[k].re + fa[k].im * fa[k].im;
					syy[k] += fb[k].re * fb[k].re + fb[k].im * fb[k].im;
				}
			}
			QVector<double> out(m, 0.0);
			for (int k = 0; k < m; ++k)
			{
				const double num = sxy[k].re * sxy[k].re + sxy[k].im * sxy[k].im;
				const double den = sxx[k] * syy[k];
				out[k] = den > 0.0 ? std::min(1.0, num / den) : 0.0;
			}
			return out;
		}

		//循环互相关（FFT 实现）：c[k] = Σ y1[t]·y2[(t+k) mod n]
		QVector<double> xfFcorr(const QVector<double>& a, const QVector<double>& b, QString* err)
		{
			if (!checkVec(a, err, "向量1") || !checkVec(b, err, "向量2"))
				return {};
			if (a.size() != b.size())
			{
				if (err)
					*err = QStringLiteral("两路数据长度不一致");
				return {};
			}
			std::vector<Cx> fa, fb;
			if (!realFft(a, fa, err) || !realFft(b, fb, err))
				return {};
			//c[k] = Σ y1[t]·y2[(t+k) mod n] ⇔ IDFT(conj(A)·B)；取 conj(A)·B
			//（此前 A·conj(B) 给出的是 -k 滞后方向，与文档式/dcorr 相反）
			std::vector<Cx> prod(a.size(), Cx());
			for (size_t k = 0; k < prod.size(); ++k)
				prod[k] = fa[k].conj() * fb[k];
			if (!fftAny(prod, true, err))
				return {};
			QVector<double> out;
			for (const Cx& v : prod)
				out.push_back(v.re);
			return out;
		}

		//循环互相关（直接求和，与 fcorr 同式）
		QVector<double> xfDcorr(const QVector<double>& a, const QVector<double>& b, QString* err)
		{
			if (!checkVec(a, err, "向量1") || !checkVec(b, err, "向量2"))
				return {};
			if (a.size() != b.size())
			{
				if (err)
					*err = QStringLiteral("两路数据长度不一致");
				return {};
			}
			const int n = a.size();
			QVector<double> out(n, 0.0);
			for (int lag = 0; lag < n; ++lag)
			{
				double s = 0.0;
				for (int t = 0; t < n; ++t)
					s += a.at(t) * b.at((t + lag) % n);
				out[lag] = s;
			}
			return out;
		}

		//Hilbert 变换（正交分量）：解析信号构造后取虚部
		QVector<double> xfHilbert(const QVector<double>& y, QString* err)
		{
			std::vector<Cx> spec;
			if (!realFft(y, spec, err))
				return {};
			const int n = int(spec.size());
			for (int k = 1; k < n; ++k)
			{
				if (k < n - k)
					spec[k] = spec[k] * 2.0;
				else if (k > n - k)
					spec[k] = Cx();
			}
			if (!fftAny(spec, true, err))
				return {};
			QVector<double> out;
			for (const Cx& v : spec)
				out.push_back(v.im);
			return out;
		}

		// =============================================================================
		// 新增（官方 Templex 参考文档语义）：直方图/尖峰抑制/求根/重采样/分块/FRF
		// =============================================================================
		//pdf 直方图计数：按 [min,max] 均分 num_bins 区间，输出各区间数据点数
		QVector<double> xfPdf(const QVector<double>& y, double numBins, QString* err)
		{
			if (!checkVec(y, err))
				return {};
			const int nb = int(numBins);
			if (nb < 1 || double(nb) != numBins)
			{
				if (err)
					*err = QStringLiteral("pdf 区间数须为正整数");
				return {};
			}
			const double lo = *std::min_element(y.cbegin(), y.cend());
			const double hi = *std::max_element(y.cbegin(), y.cend());
			const double range = hi - lo;
			QVector<double> out(nb, 0.0);
			if (range <= 0.0)
			{
				out[0] = double(y.size());   //全部相同：落入第一区间
				return out;
			}
			for (double v : y)
			{
				int idx = int((v - lo) / range * double(nb));
				out[std::clamp(idx, 0, nb - 1)] += 1.0;
			}
			return out;
		}

		//bins 区间中点：与 pdf 同边界，输出各区间中点（概率密度图的 x 向量）
		QVector<double> xfBins(const QVector<double>& y, double numBins, QString* err)
		{
			if (!checkVec(y, err))
				return {};
			const int nb = int(numBins);
			if (nb < 1 || double(nb) != numBins)
			{
				if (err)
					*err = QStringLiteral("bins 区间数须为正整数");
				return {};
			}
			const double lo = *std::min_element(y.cbegin(), y.cend());
			const double hi = *std::max_element(y.cbegin(), y.cend());
			const double step = (hi - lo) / double(nb);
			QVector<double> out(nb, 0.0);
			if (step <= 0.0)
			{
				for (int k = 0; k < nb; ++k)
					out[k] = lo;
				return out;
			}
			for (int k = 0; k < nb; ++k)
				out[k] = lo + (double(k) + 0.5) * step;
			return out;
		}

		//suppress 尖峰抑制：i 点尖峰度 = 与两邻点夹角的余弦；大于阈值时该点
		//替换为两邻点均值（首末点不受影响）。官方示例：{1,1,1E6,2,2} 阈值 0.9
		//→ {1,1,1.5,2,2}
		QVector<double> xfSuppress(const QVector<double>& x, const QVector<double>& y,
			double sharpness, QString* err)
		{
			if (!checkVec(y, err) || !checkVec(x, err))
				return {};
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("x 与 y 数据长度不一致");
				return {};
			}
			if (sharpness <= 0.0 || sharpness >= 1.0)
			{
				if (err)
					*err = QStringLiteral("suppress sharpness 须在 (0,1) 开区间内");
				return {};
			}
			QVector<double> out = y;
			for (int i = 1; i + 1 < y.size(); ++i)
			{
				const double ax = x.at(i - 1) - x.at(i);
				const double ay = y.at(i - 1) - y.at(i);
				const double bx = x.at(i + 1) - x.at(i);
				const double by = y.at(i + 1) - y.at(i);
				const double la = std::sqrt(ax * ax + ay * ay);
				const double lb = std::sqrt(bx * bx + by * by);
				if (la < 1e-300 || lb < 1e-300)
					continue;
				const double cosAngle = (ax * bx + ay * by) / (la * lb);
				if (cosAngle > sharpness)
					out[i] = (y.at(i - 1) + y.at(i + 1)) / 2.0;
			}
			return out;
		}

		//mstep 多级阶跃生成（MotionSolve STEP/STEP5/HAVSIN 语义）：
		//interp_key 1=STEP(三次平滑 3s²-2s³) 2=STEP5(五次 6s⁵-15s⁴+10s³)
		//3=HAVSIN(sin²)；use_abs 1=绝对 y / 0=相对前级；步进组 (x起,x止,y止)，
		//y 起为前级 y 止；x<x0 取 y0，x>末级 x1 取末级 y
		QVector<double> xfMstep(const QVector<double>& x, double useAbs, double key,
			const QVector<double>& steps, QString* err)
		{
			if (!checkVec(x, err))
				return {};
			//步进布局（官方）：首级四元组 (x0,y0,x1,y1)，后续每级三元组 (x起,x止,y止)，
			//总长 = 4 + 3·级数
			if (steps.size() < 4 || (steps.size() - 4) % 3 != 0)
			{
				if (err)
					*err = QStringLiteral("mstep 步进参数须为 (x0,y0,x1,y1)+3 点/级追加");
				return {};
			}
			const int interpKey = int(key);
			if (interpKey < 1 || interpKey > 3 || double(interpKey) != key)
			{
				if (err)
					*err = QStringLiteral("mstep interp_key 须为 1(step)/2(step5)/3(havsin)");
				return {};
			}
			auto stepInterp = [interpKey](double q, double y0, double y1)
			{
				const double s = std::clamp(q, 0.0, 1.0);
				if (interpKey == 1)
					return y0 + (y1 - y0) * s * s * (3.0 - 2.0 * s);
				if (interpKey == 2)
					return y0 + (y1 - y0) * s * s * s * (10.0 + s * (-15.0 + 6.0 * s));
				const double th = kPi * s / 2.0;
				return y0 + (y1 - y0) * std::sin(th) * std::sin(th);
			};
			//解析步级序列：每级 (xs, yStart, xe, ye)
			const int nSteps = 1 + int(steps.size() - 4) / 3;
			struct StepRun { double xs, ys, xe, ye; };
			QVector<StepRun> runs;
			runs.reserve(nSteps);
			runs.push_back({ steps.at(0), steps.at(1), steps.at(2), steps.at(3) });
			double prevEnd = steps.at(3);
			for (int i = 4; i + 3 <= int(steps.size()); i += 3)
			{
				const double xs2 = steps.at(i);
				const double xe2 = steps.at(i + 1);
				const double ye2 = (useAbs != 0.0) ? steps.at(i + 2) : prevEnd + steps.at(i + 2);
				runs.push_back({ xs2, prevEnd, xe2, ye2 });
				prevEnd = ye2;
			}
			QVector<double> out(x.size(), 0.0);
			for (int i = 0; i < x.size(); ++i)
			{
				const double q = x.at(i);
				//定位当前步级：最后一级 x起 ≤ q 的步级（首级之前取 y0）
				int cur = -1;
				for (int st = 0; st < runs.size(); ++st)
					if (q >= runs.at(st).xs)
						cur = st;
				if (cur < 0)
				{
					out[i] = steps.at(1);   //首级 y0
					continue;
				}
				const StepRun& run = runs.at(cur);
				if (q >= run.xe)
				{
					out[i] = run.ye;   //本级已完成
					continue;
				}
				out[i] = stepInterp((q - run.xs) / (run.xe - run.xs), run.ys, run.ye);
			}
			return out;
		}

		//resamp 族公共：rev(t) = rps 对时间的累积积分（梯形，从 0 起）
		QVector<double> revCumtrapz(const QVector<double>& rpsTime, const QVector<double>& rpsMag)
		{
			QVector<double> rev(rpsTime.size(), 0.0);
			for (int i = 1; i < rpsTime.size(); ++i)
				rev[i] = rev.at(i - 1) + (rpsTime.at(i) - rpsTime.at(i - 1))
					* (rpsMag.at(i) + rpsMag.at(i - 1)) * 0.5;
			return rev;
		}

		//resamprevs：等转角增量网格（点数与转速测量点数一致，0..总转角）
		QVector<double> xfResamprevs(const QVector<double>& rpsTime, const QVector<double>& rpsMag,
			QString* err)
		{
			if (!checkVec(rpsTime, err, "rps_time") || !checkVec(rpsMag, err, "rps_mag"))
				return {};
			if (rpsTime.size() != rpsMag.size())
			{
				if (err)
					*err = QStringLiteral("rps_time 与 rps_mag 长度不一致");
				return {};
			}
			const QVector<double> rev = revCumtrapz(rpsTime, rpsMag);
			const int n = rev.size();
			QVector<double> out(n, 0.0);
			for (int i = 0; i < n; ++i)
				out[i] = rev.first() + (rev.last() - rev.first()) * double(i) / double(n - 1);
			return out;
		}

		//resamptime：等转角增量对应的时间点（rev(t) 反插值）
		QVector<double> xfResamptime(const QVector<double>& rpsTime, const QVector<double>& rpsMag,
			QString* err)
		{
			if (!checkVec(rpsTime, err, "rps_time") || !checkVec(rpsMag, err, "rps_mag"))
				return {};
			const QVector<double> rev = revCumtrapz(rpsTime, rpsMag);
			const QVector<double> grid = xfResamprevs(rpsTime, rpsMag, err);
			if (!grid.isEmpty())
			{
				//rev(t) 须单调（rps 非负），可直接反插值
				QVector<double> out;
				out.reserve(grid.size());
				for (double r : grid)
					out.push_back(interpAt(rev, rpsTime, r));
				return out;
			}
			return {};
		}

		//resampresp：等转角增量对应的响应幅值（先求时间点，再插值响应曲线）
		QVector<double> xfResampresp(const QVector<double>& respTime, const QVector<double>& respMag,
			const QVector<double>& rpsTime, const QVector<double>& rpsMag, QString* err)
		{
			if (!checkVec(respTime, err, "resp_time") || !checkVec(respMag, err, "resp_mag"))
				return {};
			if (respTime.size() != respMag.size())
			{
				if (err)
					*err = QStringLiteral("resp_time 与 resp_mag 长度不一致");
				return {};
			}
			const QVector<double> times = xfResamptime(rpsTime, rpsMag, err);
			if (!times.isEmpty())
			{
				QVector<double> out;
				out.reserve(times.size());
				for (double t : times)
					out.push_back(interpAt(respTime, respMag, t));
				return out;
			}
			return {};
		}

		//FRF（H = FFT(输出)/FFT(输入)）：长度 = ≥N 的最小 2 的幂；分母为零的
		//频点不计入平均（置 0）
		bool frfSpectrum(const QVector<double>& in, const QVector<double>& out, int comp,
			QVector<double>& result, QString* err)
		{
			if (!checkVec(in, err, "输入") || !checkVec(out, err, "输出"))
				return false;
			if (in.size() != out.size())
			{
				if (err)
					*err = QStringLiteral("输入与输出长度不一致");
				return false;
			}
			const int n = in.size();
			int m = 1;
			while (m < n)
				m <<= 1;
			std::vector<Cx> fa(m, Cx()), fb(m, Cx());
			for (int i = 0; i < n; ++i)
			{
				fa[i] = Cx(in.at(i));
				fb[i] = Cx(out.at(i));
			}
			if (!fftAny(fa, false, err) || !fftAny(fb, false, err))
				return false;
			result = QVector<double>(m, 0.0);
			for (int k = 0; k < m; ++k)
			{
				const double dr = fa[k].re * fa[k].re + fa[k].im * fa[k].im;
				if (dr < 1e-300)
					continue;   //输入谱为零的频点不计
				const Cx h((fb[k].re * fa[k].re + fb[k].im * fa[k].im) / dr,
					(fb[k].im * fa[k].re - fb[k].re * fa[k].im) / dr);
				switch (comp)
				{
				case 0: result[k] = h.abs(); break;
				case 1: result[k] = h.re; break;
				case 2: result[k] = h.im; break;
				case 3: result[k] = std::atan2(h.im, h.re); break;
				}
			}
			return true;
		}

		//分块谱公共：块步进 = block_size - overlap；块数据补零到 ≥bs 的 2 的幂
		//后 FFT，各块复谱平均；输出长度 = 该 2 的幂（与官方一致，未做单边折叠）
		bool blockSpectrum(const QVector<double>& y, double blockSize, double overlap,
			int comp, QVector<double>& result, QString* err)
		{
			if (!checkVec(y, err))
				return false;
			const int bs = int(blockSize);
			if (bs < 1 || double(bs) != blockSize)
			{
				if (err)
					*err = QStringLiteral("块长须为正整数");
				return false;
			}
			int step = bs - int(overlap);
			if (step <= 0)
			{
				if (err)
					*err = QStringLiteral("overlap 须小于块长");
				return false;
			}
			int m = 1;
			while (m < bs)
				m <<= 1;
			std::vector<Cx> acc(m, Cx());
			int blocks = 0;
			for (int start = 0; start + bs <= y.size(); start += step)
			{
				std::vector<Cx> blk(m, Cx());
				for (int i = 0; i < bs; ++i)
					blk[i] = Cx(y.at(start + i));
				if (!fftAny(blk, false, err))
					return false;
				for (int k = 0; k < m; ++k)
					acc[k] = acc[k] + blk[k];
				++blocks;
			}
			if (blocks == 0)
			{
				if (err)
					*err = QStringLiteral("数据点数 %1 不足一个块（%2）").arg(y.size()).arg(bs);
				return false;
			}
			result = QVector<double>(m, 0.0);
			for (int k = 0; k < m; ++k)
			{
				const Cx avg = acc[k] / double(blocks);
				switch (comp)
				{
				case 0: result[k] = avg.abs(); break;
				case 1: result[k] = avg.re; break;
				case 2: result[k] = avg.im; break;
				case 3: result[k] = std::atan2(avg.im, avg.re); break;
				}
			}
			return true;
		}

		//分块 FRF：每块 H = FFT(输出块)/FFT(输入块)（输入谱零频点不计），H 块平均
		bool blockFrf(const QVector<double>& in, const QVector<double>& out,
			double blockSize, double overlap, int comp,
			QVector<double>& result, QString* err)
		{
			if (!checkVec(in, err, "输入") || !checkVec(out, err, "输出"))
				return false;
			if (in.size() != out.size())
			{
				if (err)
					*err = QStringLiteral("输入与输出长度不一致");
				return false;
			}
			const int bs = int(blockSize);
			if (bs < 1 || double(bs) != blockSize)
			{
				if (err)
					*err = QStringLiteral("块长须为正整数");
				return false;
			}
			const int step = bs - int(overlap);
			if (step <= 0)
			{
				if (err)
					*err = QStringLiteral("overlap 须小于块长");
				return false;
			}
			int m = 1;
			while (m < bs)
				m <<= 1;
			std::vector<Cx> acc(m, Cx());
			std::vector<int> cnt(m, 0);
			int blocks = 0;
			for (int start = 0; start + bs <= in.size(); start += step)
			{
				std::vector<Cx> fa(m, Cx()), fb(m, Cx());
				for (int i = 0; i < bs; ++i)
				{
					fa[i] = Cx(in.at(start + i));
					fb[i] = Cx(out.at(start + i));
				}
				if (!fftAny(fa, false, err) || !fftAny(fb, false, err))
					return false;
				++blocks;
				for (int k = 0; k < m; ++k)
				{
					const double dr = fa[k].re * fa[k].re + fa[k].im * fa[k].im;
					if (dr < 1e-300)
						continue;
					acc[k] = acc[k] + Cx((fb[k].re * fa[k].re + fb[k].im * fa[k].im) / dr,
						(fb[k].im * fa[k].re - fb[k].re * fa[k].im) / dr);
					++cnt[k];
				}
			}
			if (blocks == 0)
			{
				if (err)
					*err = QStringLiteral("数据点数 %1 不足一个块（%2）").arg(in.size()).arg(bs);
				return false;
			}
			result = QVector<double>(m, 0.0);
			for (int k = 0; k < m; ++k)
			{
				if (cnt[k] == 0)
					continue;
				const Cx h = acc[k] / double(cnt[k]);
				switch (comp)
				{
				case 0: result[k] = h.abs(); break;
				case 1: result[k] = h.re; break;
				case 2: result[k] = h.im; break;
				case 3: result[k] = std::atan2(h.im, h.re); break;
				}
			}
			return true;
		}

		//psd5（修正 PSD）：逐块去均值 + Hann 窗 + 幅值补偿(1/mean(w))，
		//PSD[k] = |X[k]|²/(2N²)（双边密度，官方算例：100·sin(2πk/16) 类正弦
		//幅值 10@20Hz → PSD(20)=8.3333、PSD(16/24)=2.0833）；逐块平均
		bool psd5Block(const QVector<double>& y, QString* err, QVector<double>& out)
		{
			const int n = y.size();
			std::vector<double> w(n);
			double meanW = 0.0;
			for (int i = 0; i < n; ++i)
			{
				w[i] = 0.5 * (1.0 - std::cos(2.0 * kPi * double(i) / double(n)));
				meanW += w[i];
			}
			meanW /= double(n);
			if (meanW < 1e-300)
			{
				if (err)
					*err = QStringLiteral("psd5 窗均值异常");
				return false;
			}
			double mean = 0.0;
			for (double v : y)
				mean += v;
			mean /= double(n);
			std::vector<Cx> spec(n, Cx());
			for (int i = 0; i < n; ++i)
				spec[i] = Cx((y.at(i) - mean) * w[i] / meanW);
			if (!fftAny(spec, false, err))
				return false;
			out = QVector<double>(n, 0.0);
			for (int k = 0; k < n; ++k)
				out[k] = (spec[k].re * spec[k].re + spec[k].im * spec[k].im) / (2.0 * double(n) * double(n));
			return true;
		}

		QVector<double> xfPsd5(const QVector<double>& time, const QVector<double>& amp,
			double blockSize, double overlap, QString* err)
		{
			if (!checkVec(time, err, "time") || !checkVec(amp, err, "amplitude"))
				return {};
			if (time.size() != amp.size())
			{
				if (err)
					*err = QStringLiteral("time 与 amplitude 长度不一致");
				return {};
			}
			const int bs = int(blockSize);
			if (bs < 2 || double(bs) != blockSize)
			{
				if (err)
					*err = QStringLiteral("psd5 块长须为 ≥2 的整数");
				return {};
			}
			const int step = bs - int(overlap);
			if (step <= 0)
			{
				if (err)
					*err = QStringLiteral("overlap 须小于块长");
				return {};
			}
			QVector<double> acc(bs, 0.0);
			int blocks = 0;
			for (int start = 0; start + bs <= amp.size(); start += step)
			{
				QVector<double> blk(bs, 0.0);
				for (int i = 0; i < bs; ++i)
					blk[i] = amp.at(start + i);
				QVector<double> psd;
				if (!psd5Block(blk, err, psd))
					return {};
				for (int k = 0; k < bs; ++k)
					acc[k] += psd.at(k);
				++blocks;
			}
			if (blocks == 0)
			{
				if (err)
					*err = QStringLiteral("数据点数 %1 不足一个块（%2）").arg(amp.size()).arg(bs);
				return {};
			}
			const double olNorm = 1.0 + double(overlap) / double(bs);   //重叠归一
			for (int k = 0; k < bs; ++k)
				acc[k] /= (double(blocks) * olNorm);
			return acc;
		}

		//asdcal（幅值谱密度，支持分块/窗/重叠/平均/去均值）：
		//type 0=真值幅值谱 1=RMS 谱（=type0/√2） 3=PSD；overlap 为百分数；
		//mean_correction 1=逐块去均值；window 0=矩形 1=Hann；averaging 0=max 1=avg；
		//输出为块长度的双边谱
		QVector<double> xfAsdcal(const QVector<double>& time, const QVector<double>& mag,
			double type, double blockSize, double overlapPct, double meanCorrection,
			double window, double averaging, QString* err)
		{
			if (!checkVec(time, err, "time") || !checkVec(mag, err, "magnitude"))
				return {};
			if (time.size() != mag.size())
			{
				if (err)
					*err = QStringLiteral("time 与 magnitude 长度不一致");
				return {};
			}
			const int bs = int(blockSize);
			if (bs < 32 || bs > 131072 || double(bs) != blockSize)
			{
				if (err)
					*err = QStringLiteral("asdcal 块长须为 32..131072 的 2 的幂整数");
				return {};
			}
			const int step = std::max(1, int(std::lround(double(bs) * (1.0 - overlapPct / 100.0))));
			if (step <= 0)
			{
				if (err)
					*err = QStringLiteral("asdcal overlap 百分数无效");
				return {};
			}
			std::vector<double> w(bs, 1.0);
			if (int(window) == 1)
			{
				for (int i = 0; i < bs; ++i)
					w[i] = 0.5 * (1.0 - std::cos(2.0 * kPi * double(i) / double(bs)));
			}
			const double dt = (time.last() - time.first()) / double(time.size() - 1);
			const double fs = dt > 0.0 ? 1.0 / dt : 1.0;
			QVector<double> acc(bs, 0.0);
			QVector<double> accMax(bs, 0.0);
			int blocks = 0;
			for (int start = 0; start + bs <= mag.size(); start += step)
			{
				std::vector<Cx> spec(bs, Cx());
				double mean = 0.0;
				if (int(meanCorrection) == 1)
				{
					for (int i = 0; i < bs; ++i)
						mean += mag.at(start + i);
					mean /= double(bs);
				}
				for (int i = 0; i < bs; ++i)
					spec[i] = Cx((mag.at(start + i) - mean) * w[i]);
				if (!fftAny(spec, false, err))
					return {};
				const int half = bs / 2;
				for (int k = 0; k < bs; ++k)
				{
					const bool edge = (k == 0) || (k == bs - k);
					const double amp = (edge ? 1.0 : 2.0) * spec[k].abs() / double(bs);
					double val = 0.0;
					if (int(type) == 0)
						val = amp;
					else if (int(type) == 1)
						val = amp / std::sqrt(2.0);
					else if (int(type) == 3)
						val = amp * amp / 2.0 / fs;
					acc[k] += val;
					accMax[k] = std::max(accMax[k], val);
				}
				++blocks;
			}
			if (blocks == 0)
			{
				if (err)
					*err = QStringLiteral("数据点数 %1 不足一个块（%2）").arg(mag.size()).arg(bs);
				return {};
			}
			QVector<double> out(bs, 0.0);
			for (int k = 0; k < bs; ++k)
				out[k] = (int(averaging) == 0) ? accMax[k] : acc[k] / double(blocks);
			return out;
		}

		//batch（FFT 理想带通滤波）：频域砖墙 mask（low=0 低通 / high=0 高通 /
		//high<low 带阻 / 其他带通），截止频率超过 Nyquist 的部分不滤波；
		//可选 extension 点前后延拓（滤波后去除）
		QVector<double> xfBatch(const QVector<double>& x, const QVector<double>& y,
			double low, double high, double extension, QString* err)
		{
			if (!checkVec(x, err) || !checkVec(y, err))
				return {};
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("x 与 y 数据长度不一致");
				return {};
			}
			if (low < 0.0 || high < 0.0)
			{
				if (err)
					*err = QStringLiteral("截止频率须为非负数");
				return {};
			}
			const int n = y.size();
			if (n < 2)
			{
				if (err)
					*err = QStringLiteral("batch 至少需要 2 个数据点");
				return {};
			}
			const double dt = (x.last() - x.first()) / double(n - 1);
			if (dt <= 0.0)
			{
				if (err)
					*err = QStringLiteral("时间轴无效（步长非正）");
				return {};
			}
			const double fnyq = 0.5 / dt;
			//官方语义：截止频率均大于 Nyquist 时曲线不滤波
			const bool noFilter = (low > fnyq && high > fnyq);
			const int ext = std::max(0, int(extension));
			std::vector<double> ye(n + 2 * ext);
			std::vector<double> xe(n + 2 * ext);
			const double dxEdge = x.at(1) - x.at(0);
			const double dxTail = x.last() - x.at(n - 2);
			for (int i = 0; i < n; ++i)
			{
				xe[ext + i] = x.at(i);
				ye[ext + i] = y.at(i);
			}
			for (int i = 0; i < ext; ++i)
			{
				//按首末段趋势线性延拓
				ye[i] = y.first() + (y.first() - y.at(std::min(n - 1, i + 1)));
				ye[n + ext + i] = y.last() + (y.last() - y.at(std::max(0, n - 2 - i)));
				xe[i] = x.first() - double(i + 1) * dxEdge;
				xe[n + ext + i] = x.last() + double(i + 1) * dxTail;
			}
			std::vector<Cx> spec;
			QVector<double> yVec(int(ye.size()), 0.0);
			for (int i = 0; i < int(ye.size()); ++i)
				yVec[i] = ye[i];
			if (!noFilter && !realFft(yVec, spec, err))
				return {};
			if (!noFilter)
			{
				const int m = int(spec.size());
				for (int k = 0; k < m; ++k)
				{
					const double f = double(std::min(k, m - k)) / double(m) / dt;
					bool pass;
					if (high == 0.0)
						pass = f >= low - 1e-12;                       //高通
					else if (low == 0.0)
						pass = f <= high + 1e-12;                      //低通
					else if (high > low)
						pass = f >= low - 1e-12 && f <= high + 1e-12;  //带通
					else
						pass = !(f >= high - 1e-12 && f <= low + 1e-12);   //带阻
					if (!pass)
						spec[k] = Cx();
				}
				if (!fftAny(spec, true, err))
					return {};
			}
			QVector<double> out;
			out.reserve(n);
			for (int i = 0; i < n; ++i)
				out.push_back(noFilter ? y.at(i) : spec[ext + i].re);
			return out;
		}

		//batchpolyfit：先拟合 order 阶多项式并扣除，残差 batch 滤波后加回
		QVector<double> xfBatchpolyfit(const QVector<double>& x, const QVector<double>& y,
			double low, double high, double order, QString* err)
		{
			QVector<double> coef = xfPolyfit(x, y, order, err);
			if (!coef.isEmpty())
			{
				QVector<double> trend = xfPolyval(coef, x, err);
				if (!err || err->isEmpty())
				{
					QVector<double> resid(y.size());
					for (int i = 0; i < y.size(); ++i)
						resid[i] = y.at(i) - trend.at(i);
					QVector<double> filtered = xfBatch(x, resid, low, high, 0.0, err);
					if (!err || err->isEmpty())
					{
						QVector<double> out(y.size());
						for (int i = 0; i < y.size(); ++i)
							out[i] = trend.at(i) + filtered.at(i);
						return out;
					}
				}
			}
			return {};
		}

		// =============================================================================
		// 新增：矩阵/向量数组
		// =============================================================================
		QVector<double> xfCat(const QVector<double>& a, const QVector<double>& b, QString* err)
		{
			if (!checkVec(a, err) || !checkVec(b, err))
				return {};
			QVector<double> out = a;
			out += b;
			return out;
		}

		QVector<double> xfSort(const QVector<double>& a, QString* err)
		{
			if (!checkVec(a, err))
				return {};
			QVector<double> out = a;
			std::sort(out.begin(), out.end());
			return out;
		}

		QVector<double> xfSubrange(const QVector<double>& a, double i, double j, QString* err)
		{
			if (!checkVec(a, err))
				return {};
			const int lo = std::max(0, std::min(int(a.size() - 1), int(i)));
			const int hi = std::max(0, std::min(int(a.size() - 1), int(j)));
			if (lo > hi)
			{
				if (err)
					*err = QStringLiteral("subrange 区间无效（%1 > %2）").arg(int(i)).arg(int(j));
				return {};
			}
			QVector<double> out;
			for (int k = lo; k <= hi; ++k)
				out.push_back(a.at(k));
			return out;
		}

		QVector<double> xfRemoveSuccDup(const QVector<double>& a, QString* err)
		{
			if (!checkVec(a, err))
				return {};
			QVector<double> out;
			for (int i = 1; i < a.size(); ++i)
				if (a.at(i) == a.at(i - 1))
					out.push_back(double(i));
			return out;
		}

		double aggSize(const QVector<double>& a, const QVector<double>&, QString* err)
		{
			if (!checkVec(a, err))
				return kNaN;
			return double(a.size());
		}

		QVector<double> xfDetOrInv(const QVector<double>& m, bool inverse, QString* err)
		{
			int n = 0;
			std::vector<std::vector<double>> A;
			if (!squareMatrix(m, n, A, err))
				return {};
			if (!inverse)
			{
				//行列式：列主元消元，对角乘积 × 交换符号
				double det = 1.0;
				auto B = A;
				int swaps = 0;
				for (int col = 0; col < n; ++col)
				{
					int piv = col;
					for (int r = col + 1; r < n; ++r)
						if (std::fabs(B[r][col]) > std::fabs(B[piv][col]))
							piv = r;
					if (std::fabs(B[piv][col]) < 1e-300)
						return { 0.0 };
					if (piv != col)
					{
						std::swap(B[piv], B[col]);
						++swaps;
					}
					det *= B[col][col];
					for (int r = col + 1; r < n; ++r)
					{
						const double f = B[r][col] / B[col][col];
						for (int c = col; c < n; ++c)
							B[r][c] -= f * B[col][c];
					}
				}
				return { swaps % 2 == 1 ? -det : det };
			}
			//求逆：高斯-约当，增广 [A | I]
			std::vector<std::vector<double>> aug(n, std::vector<double>(2 * n, 0.0));
			for (int r = 0; r < n; ++r)
			{
				for (int c = 0; c < n; ++c)
					aug[r][c] = A[r][c];
				aug[r][n + r] = 1.0;
			}
			for (int col = 0; col < n; ++col)
			{
				int piv = col;
				for (int r = col + 1; r < n; ++r)
					if (std::fabs(aug[r][col]) > std::fabs(aug[piv][col]))
						piv = r;
				if (std::fabs(aug[piv][col]) < 1e-300)
				{
					if (err)
						*err = QStringLiteral("矩阵奇异，无法求逆");
					return {};
				}
				if (piv != col)
					std::swap(aug[piv], aug[col]);
				const double diag = aug[col][col];
				for (int c = 0; c < 2 * n; ++c)
					aug[col][c] /= diag;
				for (int r = 0; r < n; ++r)
				{
					if (r == col)
						continue;
					const double f = aug[r][col];
					if (f == 0.0)
						continue;
					for (int c = 0; c < 2 * n; ++c)
						aug[r][c] -= f * aug[col][c];
				}
			}
			QVector<double> out;
			out.reserve(n * n);
			for (int r = 0; r < n; ++r)
				for (int c = 0; c < n; ++c)
					out.push_back(aug[r][n + c]);
			return out;
		}

		//特征值（Faddeev–LeVerrier 特征多项式 + Durand-Kerner 求根）
		QVector<double> eigValues(const QVector<double>& m, bool realPart, QString* err)
		{
			int n = 0;
			std::vector<std::vector<double>> A;
			if (!squareMatrix(m, n, A, err))
				return {};
			const std::vector<double> c = charPoly(A);
			//p(λ)=λ^n + c1 λ^(n-1) + ... + cn -> 升幂系数 [cn, ..., c1, 1]
			QVector<double> coef(n + 1, 0.0);
			coef[n] = 1.0;
			for (int k = 1; k <= n; ++k)
				coef[n - k] = c[k - 1];
			std::vector<Cx> roots;
			if (!polyRoots(coef, roots, err))
				return {};
			QVector<double> out;
			for (const Cx& r : roots)
				out.push_back(realPart ? r.re : r.im);
			return out;
		}

		//特征向量（逐个特征值逆迭代）：输出 n×n 行主序展平，第 k 列为第 k 个特征向量
		QVector<double> eigVectors(const QVector<double>& m, bool realPart, QString* err)
		{
			int n = 0;
			std::vector<std::vector<double>> A;
			if (!squareMatrix(m, n, A, err))
				return {};
			const std::vector<double> c = charPoly(A);
			QVector<double> coef(n + 1, 0.0);
			coef[n] = 1.0;
			for (int k = 1; k <= n; ++k)
				coef[n - k] = c[k - 1];
			std::vector<Cx> roots;
			if (!polyRoots(coef, roots, err))
				return {};
			QVector<double> out(n * n, 0.0);
			for (int k = 0; k < n; ++k)
			{
				//位移矩阵 B = A - λI（奇异时加微扰）
				std::vector<std::vector<Cx>> B(n, std::vector<Cx>(n));
				for (int r = 0; r < n; ++r)
					for (int cc = 0; cc < n; ++cc)
						B[r][cc] = Cx(A[r][cc] - (r == cc ? roots[k].re : 0.0),
						              (r == cc ? -roots[k].im : 0.0));
				B[k][k] = B[k][k] + Cx(1e-10, 0.0);
				std::vector<Cx> v(n, Cx(1.0, k * 0.1));   //初值
				for (int iter = 0; iter < 8; ++iter)
				{
					std::vector<Cx> nv;
					if (!gaussSolveCx(B, v, nv))
						break;
					double norm = 0.0;
					for (const Cx& z : nv)
						norm += z.re * z.re + z.im * z.im;
					norm = std::sqrt(norm);
					if (norm < 1e-300)
						break;
					for (int i = 0; i < n; ++i)
						nv[i] = nv[i] / norm;
					v = nv;
				}
				//归一化按模最大分量为实正（相位规整）
				int imax = 0;
				for (int i = 1; i < n; ++i)
					if (v[i].abs() > v[imax].abs())
						imax = i;
				if (v[imax].abs() > 1e-300)
				{
					const double dr = v[imax].re * v[imax].re + v[imax].im * v[imax].im;
					const Cx phase(v[imax].re / dr, -v[imax].im / dr);   //conj(v_imax)/|v_imax|²
					for (int i = 0; i < n; ++i)
						v[i] = v[i] * phase;
				}
				for (int r = 0; r < n; ++r)
					out[r * n + k] = realPart ? v[r].re : v[r].im;
			}
			return out;
		}

		// =============================================================================
		// 新增：文件读取映射与工程评价
		// =============================================================================
		QVector<double> xfCurvex(const QVector<double>&, const QVector<double>& x, QString* err)
		{
			if (!checkVec(x, err))
				return {};
			return x;   //引用的横坐标序列
		}

		QVector<double> xfCurvey(const QVector<double>& y, const QVector<double>&, QString* err)
		{
			if (!checkVec(y, err))
				return {};
			return y;
		}

		QVector<double> xfFold(const QVector<double>& y, QString* err)
		{
			if (!checkVec(y, err))
				return {};
			QVector<double> out;
			out.reserve(y.size());
			for (double v : y)
				out.push_back(std::fabs(v));
			return out;
		}

		//与零线交点：相邻点异号线性插值，端点恰为 0 亦计入；返回横坐标序列
		QVector<double> zeroCrossPoints(const QVector<double>& y, const QVector<double>& x)
		{
			QVector<double> out;
			for (int i = 0; i < y.size(); ++i)
			{
				if (y.at(i) == 0.0)
					out.push_back(x.at(i));
				else if (i > 0 && ((y.at(i - 1) < 0.0) != (y.at(i) < 0.0)) && y.at(i - 1) != 0.0)
					out.push_back(x.at(i - 1)
						+ (0.0 - y.at(i - 1)) * (x.at(i) - x.at(i - 1)) / (y.at(i) - y.at(i - 1)));
			}
			return out;
		}

		QVector<double> xfIntercepts(const QVector<double>& y, const QVector<double>& x, QString* err)
		{
			if (!checkVec(y, err, "y") || !checkVec(x, err, "x"))
				return {};
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("y 与 x 数据长度不一致");
				return {};
			}
			return zeroCrossPoints(y, x);
		}

		//两曲线交点：b 线性插值到 a 的横坐标后求差曲线与零线交点（同 x 约定，
		//两路点数须一致——缺校验时 interpAt 会以 b 的短序列越界读，QA BUG-1）
		QVector<double> xfIntersect(const QVector<double>& ya, const QVector<double>& xa,
			const QVector<double>& yb, QString* err)
		{
			if (!checkVec(ya, err, "曲线1") || !checkVec(xa, err, "曲线1x") || !checkVec(yb, err, "曲线2"))
				return {};
			if (!checkAscending(xa, err, "曲线1x"))
				return {};
			if (ya.size() != yb.size())
			{
				if (err)
					*err = QStringLiteral("两曲线点数须一致（同 x 约定：%1 ≠ %2）")
						.arg(ya.size()).arg(yb.size());
				return {};
			}
			QVector<double> d(ya.size());
			for (int i = 0; i < ya.size(); ++i)
				d[i] = ya.at(i) - interpAt(xa, yb, xa.at(i));
			return zeroCrossPoints(d, xa);
		}

		// =============================================================================
		// 新增（官方 Templex 参考文档语义）II：SAE/IRF 滤波、求根、条件取值、数组
		// =============================================================================
		//saefilter（SAE class filter，SAE 810813）：二阶 Butterworth 低通
		//（双线性离散），零相位（前向+时间反转）；fL=2.5·class（60/180/600/1000）；
		//官方规则：采样频率 fs < 3.2·class 时曲线不滤波（原样返回）
		QVector<double> xfSaefilter(const QVector<double>& x, const QVector<double>& y,
			double cfc, QString* err)
		{
			if (!checkVec(x, err) || !checkVec(y, err))
				return {};
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("x 与 y 数据长度不一致");
				return {};
			}
			if (x.size() < 2)
			{
				if (err)
					*err = QStringLiteral("saefilter 至少需要 2 个数据点");
				return {};
			}
			if (cfc <= 0.0)
			{
				if (err)
					*err = QStringLiteral("saefilter class 须为非负标量（典型 60/180/600/1000）");
				return {};
			}
			const double dt = (x.last() - x.first()) / double(x.size() - 1);
			const double fs = dt > 0.0 ? 1.0 / dt : 0.0;
			if (fs <= 0.0)
			{
				if (err)
					*err = QStringLiteral("saefilter 时间轴无效（步长非正）");
				return {};
			}
			if (fs < 3.2 * cfc)
				return y;   //官方规则：采样频率不足 3.2·class，不滤波
			const double fL = 2.5 * cfc;
			const double wn = 2.0 * kPi * fL;
			const double K = 2.0 / dt;
			const double B = std::sqrt(2.0) * wn;
			const double C = wn * wn;
			double d0 = 1.0 / (K * K + B * K + C);
			double d1 = (-2.0 * K * K + 2.0 * C) * d0;
			double d2 = (K * K - B * K + C) * d0;
			double b0 = C * d0;
			double b1 = 2.0 * C * d0;
			double b2 = C * d0;
			auto biquad = [b0, b1, b2, d1, d2](const QVector<double>& in)
			{
				QVector<double> o(in.size(), 0.0);
				for (int k = 0; k < in.size(); ++k)
				{
					double s = b0 * in.at(k);
					if (k >= 1)
						s += b1 * in.at(k - 1) - d1 * o.at(k - 1);
					if (k >= 2)
						s += b2 * in.at(k - 2) - d2 * o.at(k - 2);
					o[k] = s;
				}
				return o;
			};
			QVector<double> fwd = biquad(y);
			std::reverse(fwd.begin(), fwd.end());
			QVector<double> rev = biquad(fwd);
			std::reverse(rev.begin(), rev.end());
			return rev;
		}

		//irfilter（脉冲响应滤波，官方式）：y[k] = Σ b[i]·x[k-i] − Σ a[i]·y[k-i-1]，
		//a 向量省略恒为 1 的首项 a0（等价差分方程中 a0=1）；仅给 FIR 时为纯卷积
		QVector<double> xfIrfilter(const QVector<double>& y, const QVector<double>& b,
			const QVector<double>& a, bool hasIir, QString* err)
		{
			if (!checkVec(y, err, "y") || !checkVec(b, err, "FIR") )
				return {};
			if (hasIir && !checkVec(a, err, "IIR"))
				return {};
			QVector<double> out(y.size(), 0.0);
			for (int k = 0; k < y.size(); ++k)
			{
				double acc = 0.0;
				for (int i = 0; i < b.size() && k - i >= 0; ++i)
					acc += b.at(i) * y.at(k - i);
				if (hasIir)
					for (int i = 0; i < a.size() && k - i - 1 >= 0; ++i)
						acc -= a.at(i) * out.at(k - i - 1);
				out[k] = acc;
			}
			return out;
		}

		//nr（Newton-Raphson 求根）：在曲线 (xvect,yvect) 上求 y=preload 的根。
		//曲线按 interpAt 为分段线性——段内牛顿一步即线性插值精确解；返回首个根
		//的 x 值（聚合→标量；官方示例 xVal = nr(p1w1c1.x, p1w1c1.y, 100)）
		double aggNr(const QVector<double>& x, const QVector<double>& y,
			double preload, QString* err)
		{
			if (!checkVec(y, err, "yvect") || !checkVec(x, err, "xvect"))
				return std::numeric_limits<double>::quiet_NaN();
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("xvect 与 yvect 长度不一致");
				return std::numeric_limits<double>::quiet_NaN();
			}
			for (int i = 0; i < y.size(); ++i)
			{
				const double d0 = y.at(i) - preload;
				if (d0 == 0.0)
					return x.at(i);
				if (i + 1 >= y.size())
					break;
				const double d1 = y.at(i + 1) - preload;
				if (d0 == d1)
					continue;
				if (d0 * d1 <= 0.0)
					return x.at(i) + (x.at(i + 1) - x.at(i)) * d0 / (d0 - d1);
			}
			if (err)
				*err = QStringLiteral("nr 未找到 y=preload 的根（数据范围内无符号变化）");
			return std::numeric_limits<double>::quiet_NaN();
		}

		//getval2：index_vec 中首个等于 value 的位置 i，返回 data_vec[i]；
		//未找到/越界报错
		double aggGetval2(const QVector<double>& dataVec, const QVector<double>& indexVec,
			double value, QString* err)
		{
			if (!checkVec(indexVec, err, "index_vec") || !checkVec(dataVec, err, "data_vec"))
				return std::numeric_limits<double>::quiet_NaN();
			for (int i = 0; i < indexVec.size(); ++i)
				if (indexVec.at(i) == value)
				{
					if (i >= dataVec.size())
					{
						if (err)
							*err = QStringLiteral("命中索引 %1 超出 data_vec 长度 %2")
								.arg(i).arg(dataVec.size());
						return std::numeric_limits<double>::quiet_NaN();
					}
					return dataVec.at(i);
				}
			if (err)
				*err = QStringLiteral("index_vec 中不存在值 %1").arg(value);
			return std::numeric_limits<double>::quiet_NaN();
		}

		//iffunc：expr1 非零返回 a，否则返回 b
		double aggIffunc(double cond, double a, double b)
		{
			return (cond != 0.0) ? a : b;
		}

		//array(n[,m])：全零行向量（矩阵形式 m 返回展平 n·m 零向量，说明栏注明）
		QVector<double> aggArray(double size1, double size2, bool hasSize2, QString* err)
		{
			const int n = int(size1);
			if (n < 1 || double(n) != size1)
			{
				if (err)
					*err = QStringLiteral("array 元素数须为正整数");
				return {};
			}
			const int total = hasSize2 ? n * std::max(0, int(size2)) : n;
			if (total < 1)
			{
				if (err)
					*err = QStringLiteral("array 元素数须为正整数");
				return {};
			}
			return QVector<double>(total, 0.0);
		}

		//分段线性曲线在 [lo,hi]（夹取到数据范围）内的梯形积分
		double trapzOverRange(const QVector<double>& x, const QVector<double>& y,
			double lo, double hi)
		{
			lo = std::max(lo, x.first());
			hi = std::min(hi, x.last());
			if (hi <= lo)
				return 0.0;
			const auto itLo = std::lower_bound(x.cbegin(), x.cend(), lo);
			int i0 = int(itLo - x.cbegin());
			if (i0 > 0)
				--i0;
			double s = 0.0;
			double prevX = lo, prevY = interpAt(x, y, lo);
			for (int i = i0; i < x.size() && x.at(i) < hi; ++i)
			{
				const double cx = std::min(x.at(i), hi);
				const double cy = (x.at(i) <= hi) ? y.at(i) : interpAt(x, y, hi);
				s += (cx - prevX) * (cy + prevY) * 0.5;
				prevX = cx;
				prevY = cy;
			}
			const double lastY = interpAt(x, y, hi);
			s += (hi - prevX) * (lastY + prevY) * 0.5;
			return s;
		}

		//smean（滑动均值积分，官方 5 参）：窗口宽 winsize（x 单位），
		//out(t_i) = 窗口 [t_i-W/2, t_i+W/2] 内梯形积分 / 窗口实际宽度；
		//boundarywinsize：1/2=窗口夹取到数据范围（边界差异以文档图示为准，
		//实现按实际宽度平均）；3=端点镜像补长窗口；numrep 为重复次数（≥1）
		QVector<double> xfSmean5(const QVector<double>& x, const QVector<double>& y,
			double winSize, double boundaryMode, double numRep, QString* err)
		{
			if (!checkVec(x, err, "x_in") || !checkVec(y, err, "y_in"))
				return {};
			if (x.size() != y.size())
			{
				if (err)
					*err = QStringLiteral("x_in 与 y_in 长度不一致");
				return {};
			}
			if (winSize <= 0.0)
			{
				if (err)
					*err = QStringLiteral("smean 窗口宽度须为正数");
				return {};
			}
			const int reps = std::max(1, int(numRep));
			const double half = winSize / 2.0;
			QVector<double> cur = y;
			for (int rep = 0; rep < reps; ++rep)
			{
				QVector<double> out(cur.size(), 0.0);
				for (int i = 0; i < cur.size(); ++i)
				{
					const double ti = x.at(i);
					double lo = ti - half, hi = ti + half;
					if (int(boundaryMode) == 3)
					{
						//镜像补长：窗口越界部分以端点镜像取值，按全窗口平均
						auto reflected = [&cur, &x](double t)
						{
							const int n = int(x.size());
							if (t <= x.first())
								return cur.at(std::min(n - 1, int(std::round((x.first() - t) / std::max(1e-300, x.at(1) - x.first())))));
							if (t >= x.last())
								return cur.at(std::max(0, int(std::round(n - 1 - (t - x.last()) / std::max(1e-300, x.last() - x.at(n - 2))))));
							return interpAt(x, cur, t);
						};
						//全窗口等距采样积分（21 点）
						double s = 0.0;
						for (int ss = 0; ss <= 20; ++ss)
							s += reflected(lo + (hi - lo) * double(ss) / 20.0);
						out[i] = s * (hi - lo) / 20.0 / winSize;
						continue;
					}
					const double clipped = std::min(hi, x.last()) - std::max(lo, x.first());
					if (clipped <= 0.0)
					{
						out[i] = cur.at(i);
						continue;
					}
					out[i] = trapzOverRange(x, cur, lo, hi) / clipped;
				}
				cur = out;
			}
			return cur;
		}

		// =============================================================================
		// 目录构建辅助：旧式自由函数包装为统一签名
		// =============================================================================
		using AggFnU = mCurveMathFunctions::AggFnU;
		using VecFnU = mCurveMathFunctions::VecFnU;
		using VecArgs = mCurveMathFunctions::VecArgs;
		using Agg1 = double (*)(const QVector<double>&, const QVector<double>&, QString*);
		using Agg2 = double (*)(const QVector<double>&, const QVector<double>&, QString*);
		using Xfm1 = QVector<double> (*)(const QVector<double>&, const QVector<double>&, QString*);

		//VecArgs 第 i 个引用的 y 数据（缺省空向量）
		QVector<double> yAt(const mCurveMathFunctions::VecArgs& a, int i)
		{
			return (i >= 0 && i < a.ys.size()) ? *a.ys[i] : QVector<double>();
		}

		AggFnU wrapAggY(Agg1 fn)   //单引用聚合（只用 y；x 传空防御）
		{
			return [fn](const VecArgs& a, const QVector<double>&, QString* e)
			{ return fn(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], QVector<double>(), e); };
		}
		AggFnU wrapAgg2(Agg2 fn)   //双引用聚合：a=ys[0], b=ys[1]
		{
			return [fn](const VecArgs& a, const QVector<double>&, QString* e)
			{
				const QVector<double> v1 = a.ys.size() > 0 ? *a.ys[0] : QVector<double>();
				const QVector<double> v2 = a.ys.size() > 1 ? *a.ys[1] : QVector<double>();
				return fn(v1, v2, e);
			};
		}
		VecFnU wrapXfm(Xfm1 fn)    //单引用变换：y=ys[0]，x=xs[0]
		{
			return [fn](const VecArgs& a, const QVector<double>&, QString* e)
			{ return fn(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
			            a.xs.isEmpty() ? QVector<double>() : *a.xs[0], e); };
		}
		VecFnU wrapXfm(QVector<double> (*fn)(const QVector<double>&, QString*))   //2 参版（无需 x）
		{
			return [fn](const VecArgs& a, const QVector<double>&, QString* e)
			{ return fn(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], e); };
		}
		VecFnU wrapXfm2(std::function<QVector<double>(const QVector<double>&,
			const QVector<double>&, const QVector<double>&, QString*)> fn) //双引用变换（a 的 x 为坐标轴）
		{
			return [fn](const VecArgs& a, const QVector<double>&, QString* e)
			{
				const QVector<double> v1 = a.ys.size() > 0 ? *a.ys[0] : QVector<double>();
				const QVector<double> x1 = a.xs.size() > 0 ? *a.xs[0] : QVector<double>();
				const QVector<double> v2 = a.ys.size() > 1 ? *a.ys[1] : QVector<double>();
				return fn(v1, x1, v2, e);
			};
		}
		VecFnU wrapXfm3(std::function<QVector<double>(const QVector<double>&,
			const QVector<double>&, const QVector<double>&, QString*)> fn) //三引用变换（x,y,xq）
		{
			return [fn](const VecArgs& a, const QVector<double>&, QString* e)
			{
				const QVector<double> v1 = a.ys.size() > 0 ? *a.ys[0] : QVector<double>();
				const QVector<double> v2 = a.ys.size() > 1 ? *a.ys[1] : QVector<double>();
				const QVector<double> v3 = a.ys.size() > 2 ? *a.ys[2] : QVector<double>();
				return fn(v1, v2, v3, e);
			};
		}

		//目录条目构造辅助：f=已实现项（黑色），r=未实现项（红色，仅展示）
		mCurveMathFunctions::FuncInfo mkEntry(const QString& name, const QString& signature,
			std::initializer_list<const char*> params, std::initializer_list<const char*> kinds,
			const QString& description, const QString& category, bool implemented,
			mCurveMathFunctions::Kind kind)
		{
			mCurveMathFunctions::FuncInfo f;
			f.name = name;
			f.signature = signature;
			for (const char* p : params)
				f.params.push_back(QString::fromUtf8(p));
			for (const char* k : kinds)
				f.paramKinds.push_back(QString::fromUtf8(k));
			f.description = description;
			f.category = category;
			f.implemented = implemented;
			f.kind = kind;
			return f;
		}
	}

	const QList<mCurveMathFunctions::FuncInfo>& mCurveMathFunctions::catalog()
	{
		auto fs  = [&](const QString& n, const QString& s, std::initializer_list<const char*> p,
			const QString& d, const QString& c, double (*fn)(double))
		{
			auto f = mkEntry(n, s, p, p, d, c, true, Kind::Scalar);
			f.fn1 = fn;
			return f;
		};
		auto fs2 = [&](const QString& n, const QString& s, std::initializer_list<const char*> p,
			const QString& d, const QString& c, double (*fn)(double, double))
		{
			auto f = mkEntry(n, s, p, p, d, c, true, Kind::Scalar);
			f.fn2 = fn;
			return f;
		};
		auto fc  = [&](const QString& n, const QString& s, const QString& d, const QString& c)
		{ return mkEntry(n, s, {}, {}, d, c, true, Kind::Constant); };
		//整向量聚合：kinds 省略=全 "v"；fn 直接给 AggFnU 或经包装
		auto faK = [&](const QString& n, const QString& s, std::initializer_list<const char*> p,
			std::initializer_list<const char*> kinds, const QString& d, const QString& c, AggFnU fn)
		{
			auto f = mkEntry(n, s, p, kinds, d, c, true, Kind::Aggregate);
			f.aggU = fn;
			return f;
		};
		auto fxv = [&](const QString& n, const QString& s, std::initializer_list<const char*> p,
			std::initializer_list<const char*> kinds, const QString& d, const QString& c, VecFnU fn)
		{
			auto f = mkEntry(n, s, p, kinds, d, c, true, Kind::VectorTransform);
			f.variadicTripleScalars = true;
			f.vecFn = fn;
			return f;
		};
		auto fa  = [&](const QString& n, const QString& s, std::initializer_list<const char*> p,
			const QString& d, const QString& c, AggFnU fn)
		{
			std::initializer_list<const char*> kinds = p.size() == 2
				? std::initializer_list<const char*>{ "v", "v" }
				: std::initializer_list<const char*>{ "v" };
			auto f = mkEntry(n, s, p, kinds, d, c, true, Kind::Aggregate);
			f.aggU = fn;
			return f;
		};
		//向量变换：kinds 显式给（支持数值参数混合签名）
		auto fx  = [&](const QString& n, const QString& s, std::initializer_list<const char*> p,
			std::initializer_list<const char*> kinds, const QString& d, const QString& c, VecFnU fn)
		{
			auto f = mkEntry(n, s, p, kinds, d, c, true, Kind::VectorTransform);
			f.vecFn = fn;
			return f;
		};
		auto r   = [&](const QString& n, const QString& s, std::initializer_list<const char*> p,
			const QString& d, const QString& c, Kind kind = Kind::Scalar)
		{ return mkEntry(n, s, p, {}, d, c, false, kind); };

		static const QList<FuncInfo> table = {
			//======== 1 基础数学函数（标量三角/双曲/指数对数/取整/角度转换）
			fs (QStringLiteral("abs"),    QStringLiteral("abs(vector)"),    { "vector" }, QStringLiteral("绝对值"), kCatBasic, &fnAbs),
			fs (QStringLiteral("acos"),   QStringLiteral("acos(vector)"),   { "vector" }, QStringLiteral("反余弦"), kCatBasic, &fnAcos),
			fs (QStringLiteral("asin"),   QStringLiteral("asin(vector)"),   { "vector" }, QStringLiteral("反正弦"), kCatBasic, &fnAsin),
			fs (QStringLiteral("atan"),   QStringLiteral("atan(vector)"),   { "vector" }, QStringLiteral("反正切"), kCatBasic, &fnAtan),
			fs2(QStringLiteral("atan2"),  QStringLiteral("atan2(y, x)"),    { "y", "x" }, QStringLiteral("y/x 的反正切（四象限）"), kCatBasic, &fnAtan2),
			fs (QStringLiteral("cos"),    QStringLiteral("cos(vector)"),    { "vector" }, QStringLiteral("余弦（弧度）"), kCatBasic, &fnCos),
			fs (QStringLiteral("sin"),    QStringLiteral("sin(vector)"),    { "vector" }, QStringLiteral("正弦（弧度）"), kCatBasic, &fnSin),
			fs (QStringLiteral("tan"),    QStringLiteral("tan(vector)"),    { "vector" }, QStringLiteral("正切（弧度）"), kCatBasic, &fnTan),
			fs (QStringLiteral("cosh"),   QStringLiteral("cosh(vector)"),   { "vector" }, QStringLiteral("双曲余弦"), kCatBasic, &fnCosh),
			fs (QStringLiteral("sinh"),   QStringLiteral("sinh(vector)"),   { "vector" }, QStringLiteral("双曲正弦"), kCatBasic, &fnSinh),
			fs (QStringLiteral("tanh"),   QStringLiteral("tanh(vector)"),   { "vector" }, QStringLiteral("双曲正切"), kCatBasic, &fnTanh),
			fs (QStringLiteral("exp"),    QStringLiteral("exp(vector)"),    { "vector" }, QStringLiteral("e 的 x 次幂"), kCatBasic, &fnExp),
			fs (QStringLiteral("ln"),     QStringLiteral("ln(vector)"),     { "vector" }, QStringLiteral("自然对数（e 为底）"), kCatBasic, &fnLn),
			fs (QStringLiteral("log"),    QStringLiteral("log(vector)"),    { "vector" }, QStringLiteral("对数（自然对数，= ln 的别名）"), kCatBasic, &fnLog),
			fs (QStringLiteral("log10"),  QStringLiteral("log10(vector)"),  { "vector" }, QStringLiteral("常用对数（10 为底）"), kCatBasic, &fnLog10),
			fs (QStringLiteral("sqrt"),   QStringLiteral("sqrt(vector)"),   { "vector" }, QStringLiteral("平方根"), kCatBasic, &fnSqrt),
			fs (QStringLiteral("cint"),   QStringLiteral("cint(vector)"),   { "vector" }, QStringLiteral("截断取整（C 风格向零取整）"), kCatBasic, &fnCint),
			fs (QStringLiteral("int"),    QStringLiteral("int(vector)"),    { "vector" }, QStringLiteral("取整（向下取整）"), kCatBasic, &fnInt),
			fs (QStringLiteral("rand"),   QStringLiteral("rand(vector)"),   { "vector" }, QStringLiteral("伪随机数（每次调用取下一个 [0,1) 值，忽略入参）"), kCatBasic, &fnRand),
			fs (QStringLiteral("dtor"),   QStringLiteral("dtor(vector)"),   { "vector" }, QStringLiteral("角度转弧度"), kCatBasic, &fnDtor),
			fs (QStringLiteral("rtod"),   QStringLiteral("rtod(vector)"),   { "vector" }, QStringLiteral("弧度转角度"), kCatBasic, &fnRtod),
			fs2(QStringLiteral("min"),    QStringLiteral("min(a, b)"),      { "a", "b" }, QStringLiteral("较小值（当前为标量双参；参考的向量聚合语义未实现）"), kCatBasic, &fnMin),
			fs2(QStringLiteral("max"),    QStringLiteral("max(a, b)"),      { "a", "b" }, QStringLiteral("较大值（当前为标量双参；参考的向量聚合语义未实现）"), kCatBasic, &fnMax),
			fs (QStringLiteral("floor"),  QStringLiteral("floor(vector)"),  { "vector" }, QStringLiteral("向下取整（本引擎补充）"), kCatBasic, &fnFloor),
			fs (QStringLiteral("ceil"),   QStringLiteral("ceil(vector)"),   { "vector" }, QStringLiteral("向上取整（本引擎补充）"), kCatBasic, &fnCeil),
			fs (QStringLiteral("round"),  QStringLiteral("round(vector)"),  { "vector" }, QStringLiteral("四舍五入到最近整数（本引擎补充）"), kCatBasic, &fnRound),
			//======== 2 统计与特征值函数
			fa (QStringLiteral("mean"),   QStringLiteral("mean(vector)"),   { "vector" }, QStringLiteral("算术平均值"), kCatStat, wrapAggY(&aggMean)),
			fa (QStringLiteral("median"), QStringLiteral("median(vector)"),  { "vector" }, QStringLiteral("中位数"), kCatStat, wrapAggY(&aggMedian)),
			fa (QStringLiteral("rms"),    QStringLiteral("rms(vector)"),    { "vector" }, QStringLiteral("均方根"), kCatStat, wrapAggY(&aggRms)),
			fa (QStringLiteral("sum"),    QStringLiteral("sum(vector)"),    { "vector" }, QStringLiteral("向量求和"), kCatStat, wrapAggY(&aggSum)),
			fa (QStringLiteral("variance"), QStringLiteral("variance(vector)"), { "vector" }, QStringLiteral("方差（总体方差，除 n）"), kCatStat, wrapAggY(&aggVariance)),
			fa (QStringLiteral("skewness"), QStringLiteral("skewness(vector)"), { "vector" }, QStringLiteral("偏度（g1 = m3/m2^1.5）"), kCatStat, wrapAggY(&aggSkewness)),
			fa (QStringLiteral("avgdev"), QStringLiteral("avgdev(vector)"), { "vector" }, QStringLiteral("平均绝对偏差"), kCatStat, wrapAggY(&aggAvgdev)),
			fa (QStringLiteral("integral"), QStringLiteral("integral(y, x)"), { "y", "x" }, QStringLiteral("数值积分（梯形法 ∫y dx）"), kCatStat, wrapAgg2(&trapz)),
			fa (QStringLiteral("signedarea"), QStringLiteral("signedarea(y, x)"), { "y", "x" }, QStringLiteral("有符号面积（与 integral 同义）"), kCatStat, wrapAgg2(&trapz)),
			fa (QStringLiteral("absarea"), QStringLiteral("absarea(y, x)"),  { "y", "x" }, QStringLiteral("绝对面积（∫|y| dx）"), kCatStat,
				wrapAgg2([](const QVector<double>& y, const QVector<double>& x, QString* e)
				{
					QVector<double> a;
					a.reserve(y.size());
					for (double v : y)
						a.push_back(std::fabs(v));
					return trapz(a, x, e);
				})),
			fa (QStringLiteral("centroid"), QStringLiteral("centroid(y, x)"), { "y", "x" }, QStringLiteral("面积质心横坐标（∫x·y dx / ∫y dx）"), kCatStat,
				wrapAgg2([](const QVector<double>& y, const QVector<double>& x, QString* e)
				{
					const double den = trapz(y, x, e);
					if (e && !e->isEmpty())
						return kNaN;
					if (std::fabs(den) < 1e-300)
					{
						if (e)
							*e = QStringLiteral("y 积分为零，质心无定义");
						return kNaN;
					}
					QVector<double> xy;
					xy.reserve(y.size());
					for (int i = 0; i < y.size(); ++i)
						xy.push_back(x.at(i) * y.at(i));
					return trapz(xy, x, e) / den;
				})),
			fa (QStringLiteral("correl"), QStringLiteral("correl(y1, y2)"), { "y1", "y2" }, QStringLiteral("两曲线 Pearson 相关系数"), kCatStat, wrapAgg2(&aggCorrel)),
			fa (QStringLiteral("indexofmax"), QStringLiteral("indexofmax(vector)"), { "vector" }, QStringLiteral("最大值所在点索引（0 起）"), kCatStat, wrapAggY(&aggIndexofmax)),
			fa (QStringLiteral("indexofmin"), QStringLiteral("indexofmin(vector)"), { "vector" }, QStringLiteral("最小值所在点索引（0 起）"), kCatStat, wrapAggY(&aggIndexofmin)),
			fa (QStringLiteral("zerocrossings"), QStringLiteral("zerocrossings(vector)"), { "vector" }, QStringLiteral("过零点计数（0 归非负侧）"), kCatStat, wrapAggY(&aggZerocrossings)),
			fs (QStringLiteral("cdf"),     QStringLiteral("cdf(x)"),              { "x" }, QStringLiteral("标准正态累积分布（官方示例 cdf(0)=0.5）"), kCatStat, &fnCdf),
			fs (QStringLiteral("invcdf"),  QStringLiteral("invcdf(x)"),           { "x" }, QStringLiteral("标准正态分位（官方示例 invcdf(0.5)=0）"), kCatStat, &fnInvcdf),
			fx (QStringLiteral("pdf"),     QStringLiteral("pdf(vec, num_bins)"),  { "vec", "num_bins" }, { "v", "s" }, QStringLiteral("概率密度直方图计数（[min,max] 均分 num_bins，输出各区间数据点数）"), kCatStat,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return xfPdf(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
					sc.isEmpty() ? 0.0 : sc.at(0), e); }),
			fx (QStringLiteral("bins"),    QStringLiteral("bins(vec, num_bins)"), { "vec", "num_bins" }, { "v", "s" }, QStringLiteral("概率密度直方图区间中点（pdf 的 x 向量）"), kCatStat,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return xfBins(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
					sc.isEmpty() ? 0.0 : sc.at(0), e); }),
			fx (QStringLiteral("peaks"),  QStringLiteral("peaks(vector)"),  { "vector" }, { "v" }, QStringLiteral("极值点提取（局部极大值保留幅值，其余置 0）"), kCatStat, wrapXfm(&xfmPeaks)),
			fx (QStringLiteral("delta"),  QStringLiteral("delta(vector)"),  { "vector" }, { "v" }, QStringLiteral("逐点差分（out[0]=0，out[i]=y[i]-y[i-1]）"), kCatStat, wrapXfm(&xfmDelta)),
			//======== 3 插值拟合与曲线处理
			fx (QStringLiteral("lininterp"), QStringLiteral("lininterp(x, y, xq)"), { "x", "y", "xq" }, { "v", "v", "v" }, QStringLiteral("线性插值查询（x 须严格递增）"), kCatInterp, wrapXfm3(&xfLininterp)),
			fx (QStringLiteral("logint"), QStringLiteral("logint(x, y, xq)"), { "x", "y", "xq" }, { "v", "v", "v" }, QStringLiteral("对数域插值查询（x/xq 均须为正）"), kCatInterp, wrapXfm3(&xfLogint)),
			fx (QStringLiteral("spline"), QStringLiteral("spline(x, y, xq)"), { "x", "y", "xq" }, { "v", "v", "v" }, QStringLiteral("自然三次样条插值"), kCatInterp, wrapXfm3(&xfSpline)),
			fx (QStringLiteral("akima"), QStringLiteral("akima(x, y, xq)"), { "x", "y", "xq" }, { "v", "v", "v" }, QStringLiteral("Akima 插值（至少 5 点）"), kCatInterp, wrapXfm3(&xfAkima)),
			fx (QStringLiteral("akimaderivative"), QStringLiteral("akimaderivative(x, y, xq)"), { "x", "y", "xq" }, { "v", "v", "v" }, QStringLiteral("Akima 插值导数"), kCatInterp, wrapXfm3(&xfAkimaDeriv)),
			fx (QStringLiteral("polyfit"), QStringLiteral("polyfit(x, y, n)"), { "x", "y", "n" }, { "v", "v", "s" }, QStringLiteral("多项式最小二乘拟合（返回升幂系数 [c0..cn]）"), kCatInterp,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return xfPolyfit(yAt(a, 0), yAt(a, 1),
					sc.isEmpty() ? 0.0 : sc.at(0), e); }),
			fx (QStringLiteral("polyval"), QStringLiteral("polyval(coef, xq)"), { "coef", "xq" }, { "v", "v" }, QStringLiteral("按升幂系数向量求多项式值"), kCatInterp,
				wrapXfm2([](const QVector<double>& coef, const QVector<double>&, const QVector<double>& xq, QString* e)
				{ return xfPolyval(coef, xq, e); })),
			fx (QStringLiteral("polyrootsreal"), QStringLiteral("polyrootsreal(coef)"), { "coef" }, { "v" }, QStringLiteral("多项式实根（全部根的实部，Durand-Kerner）"), kCatInterp, wrapXfm(&xfPolyrootsReal)),
			fx (QStringLiteral("polyrootsimag"), QStringLiteral("polyrootsimag(coef)"), { "coef" }, { "v" }, QStringLiteral("多项式根的虚部（顺序与 polyrootsreal 一致）"), kCatInterp, wrapXfm(&xfPolyrootsImag)),
			fx (QStringLiteral("expfit"), QStringLiteral("expfit(x, y)"), { "x", "y" }, { "v", "v" }, QStringLiteral("指数拟合 a·e^(bx)（仅正值样本，返回 x 上拟合曲线）"), kCatInterp,
				wrapXfm2([](const QVector<double>& x, const QVector<double>&, const QVector<double>& y, QString* e)
				{ return xfExpfit(x, y, e); })),
			r  (QStringLiteral("nonlinfit"), QStringLiteral("nonlinfit(x, y, model, params)"), { "x", "y", "model", "params" }, QStringLiteral("非线性拟合（官方：方程为双引号字符串 \"c1+c2*x1+c3*x2\" 形式 + 初始条件向量 c，需字符串/表达式参数机制）"), kCatInterp),
			faK(QStringLiteral("nr"), QStringLiteral("nr(xvect, yvect, preload[, debug])"), { "xvect", "yvect", "preload" }, { "v", "v", "s", "s" }, QStringLiteral("Newton-Raphson 求根（曲线上求 y=preload 的根，返回首个根的 x 值；官方示例 nr(p1w1c1.x,p1w1c1.y,100)）"), kCatInterp,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return aggNr(yAt(a, 0), yAt(a, 1), sc.isEmpty() ? 0.0 : sc.at(0), e); }),
			fx (QStringLiteral("curvesync"), QStringLiteral("curvesync(curve, xref)"), { "curve", "xref" }, { "v", "v" }, QStringLiteral("曲线同步（按参考横坐标线性重采样）"), kCatInterp,
				wrapXfm2([](const QVector<double>& ya, const QVector<double>& xa, const QVector<double>& xref, QString* e)
				{ return xfCurvesync(ya, xa, xref, e); })),
			fx (QStringLiteral("curveenv"), QStringLiteral("curveenv(a, b)"), { "a", "b" }, { "v", "v" }, QStringLiteral("上包络（b 插值到 a 的横坐标后逐点取大，约定 b 与 a 同 x）"), kCatInterp,
				wrapXfm2([](const QVector<double>& ya, const QVector<double>&, const QVector<double>& yb, QString* e)
				{
					if (!checkVec(ya, e) || !checkVec(yb, e))
						return QVector<double>();
					if (ya.size() != yb.size())
					{
						if (e)
							*e = QStringLiteral("两曲线点数须一致（同 x 约定）");
						return QVector<double>();
					}
					QVector<double> out(ya.size());
					for (int i = 0; i < ya.size(); ++i)
						out[i] = std::max(ya.at(i), yb.at(i));
					return out;
				})),
			fx (QStringLiteral("curveproduct"), QStringLiteral("curveproduct(a, b)"), { "a", "b" }, { "v", "v" }, QStringLiteral("曲线逐点相乘（同 x 约定）"), kCatInterp,
				wrapXfm2([](const QVector<double>& ya, const QVector<double>&, const QVector<double>& yb, QString* e)
				{
					if (!checkVec(ya, e) || !checkVec(yb, e) || ya.size() != yb.size())
					{
						if (e && ya.size() != yb.size())
							*e = QStringLiteral("两曲线点数须一致（同 x 约定）");
						return QVector<double>();
					}
					QVector<double> out(ya.size());
					for (int i = 0; i < ya.size(); ++i)
						out[i] = ya.at(i) * yb.at(i);
					return out;
				})),
			fx (QStringLiteral("curvesum"), QStringLiteral("curvesum(a, b)"), { "a", "b" }, { "v", "v" }, QStringLiteral("曲线逐点相加（同 x 约定）"), kCatInterp,
				wrapXfm2([](const QVector<double>& ya, const QVector<double>&, const QVector<double>& yb, QString* e)
				{
					if (!checkVec(ya, e) || !checkVec(yb, e) || ya.size() != yb.size())
					{
						if (e && ya.size() != yb.size())
							*e = QStringLiteral("两曲线点数须一致（同 x 约定）");
						return QVector<double>();
					}
					QVector<double> out(ya.size());
					for (int i = 0; i < ya.size(); ++i)
						out[i] = ya.at(i) + yb.at(i);
					return out;
				})),
			fx (QStringLiteral("suppress"), QStringLiteral("suppress(indep, dep, sharpness)"), { "indep", "dep", "sharpness" }, { "v", "v", "s" }, QStringLiteral("尖峰抑制（与两邻点夹角余弦>阈值时该点取两邻点均值，官方示例 {1,1,1E6,2,2}@0.9→{1,1,1.5,2,2}）"), kCatInterp,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return xfSuppress(yAt(a, 0), yAt(a, 1), sc.isEmpty() ? 0.0 : sc.at(0), e); }),
			fxv(QStringLiteral("mstep"), QStringLiteral("mstep(x_vector, use_abs_steps, interp_key, x0, y0, x1, y1[, x2, x3, y3]...)"), { "x_vector", "use_abs_steps", "interp_key", "x0", "y0", "x1", "y1" }, { "v", "s", "s", "s", "s", "s", "s" }, QStringLiteral("多级阶跃生成（官方：interp_key 1=STEP 2=STEP5 3=HAVSIN；use_abs 1=绝对/0=相对前级；步进组 (x起,x止,y止) 每级 3 参追加）"), kCatInterp,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> steps(sc.size() > 2 ? sc.size() - 2 : 0, 0.0);
					for (int i = 2; i < sc.size(); ++i)
						steps[i - 2] = sc.at(i);
					return xfMstep(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						sc.size() > 0 ? sc.at(0) : 0.0,
						sc.size() > 1 ? sc.at(1) : 0.0, steps, e);
				}),
			fx (QStringLiteral("resampresp"), QStringLiteral("resampresp(resp_time, resp_mag, rps_time, rps_mag[, rps_reorder])"), { "resp_time", "resp_mag", "rps_time", "rps_mag" }, { "v", "v", "v", "v", "s" }, QStringLiteral("等转角增量的响应幅值（rps 积分得转角轴，官方瀑布图重采样三件套之一；末参数为占位，传 0）"), kCatInterp,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfResampresp(yAt(a, 0), yAt(a, 1), yAt(a, 2), yAt(a, 3), e); }),
			fx (QStringLiteral("resamprevs"), QStringLiteral("resamprevs(resp_time, rps_time, rps_mag[, rps_reorder])"), { "resp_time", "rps_time", "rps_mag" }, { "v", "v", "v", "s" }, QStringLiteral("等转角增量网格（rps 积分均布，点数与转速测量一致；末参数为占位，传 0）"), kCatInterp,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfResamprevs(yAt(a, 1), yAt(a, 2), e); }),
			fx (QStringLiteral("resamptime"), QStringLiteral("resamptime(resp_time, rps_time, rps_mag[, rps_reorder])"), { "resp_time", "rps_time", "rps_mag" }, { "v", "v", "v", "s" }, QStringLiteral("等转角增量对应的时间点（rev(t) 反插值；末参数为占位，传 0）"), kCatInterp,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfResamptime(yAt(a, 1), yAt(a, 2), e); }),
			//======== 4 频谱与信号处理
			fx (QStringLiteral("fftmag"), QStringLiteral("fftmag(vector)"), { "vector" }, { "v" }, QStringLiteral("FFT 单边幅值谱（内部×2/长度，DC/Nyquist 不加倍）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfFftComponent(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], 0, e); }),
			fx (QStringLiteral("fftreal"), QStringLiteral("fftreal(vector)"), { "vector" }, { "v" }, QStringLiteral("FFT 单边实部谱"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfFftComponent(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], 1, e); }),
			fx (QStringLiteral("fftimag"), QStringLiteral("fftimag(vector)"), { "vector" }, { "v" }, QStringLiteral("FFT 单边虚部谱"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfFftComponent(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], 2, e); }),
			fx (QStringLiteral("fftphase"), QStringLiteral("fftphase(vector)"), { "vector" }, { "v" }, QStringLiteral("FFT 相位谱（弧度）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfFftComponent(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], 3, e); }),
			fx (QStringLiteral("ifft"), QStringLiteral("ifft(mag)"), { "mag" }, { "v" }, QStringLiteral("单边幅值谱逆变换（零相位重构，输出 2(M-1) 点）"), kCatSpectrum, wrapXfm(&xfIfft)),
			fx (QStringLiteral("dftmag"), QStringLiteral("dftmag(vector)"), { "vector" }, { "v" }, QStringLiteral("DFT 单边幅值谱（直接法）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfDftComponent(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], 0, e); }),
			fx (QStringLiteral("dftreal"), QStringLiteral("dftreal(vector)"), { "vector" }, { "v" }, QStringLiteral("DFT 单边实部谱"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfDftComponent(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], 1, e); }),
			fx (QStringLiteral("dftimag"), QStringLiteral("dftimag(vector)"), { "vector" }, { "v" }, QStringLiteral("DFT 单边虚部谱"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfDftComponent(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], 2, e); }),
			fx (QStringLiteral("dftphase"), QStringLiteral("dftphase(vector)"), { "vector" }, { "v" }, QStringLiteral("DFT 相位谱（弧度）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfDftComponent(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], 3, e); }),
			fx (QStringLiteral("idft"), QStringLiteral("idft(mag)"), { "mag" }, { "v" }, QStringLiteral("单边幅值谱逆 DFT（与 ifft 同语义）"), kCatSpectrum, wrapXfm(&xfIdft)),
			fx (QStringLiteral("blockfftmag"), QStringLiteral("blockfftmag(vec, block_size, overlap)"), { "vec", "block_size", "overlap" }, { "v", "s", "s" }, QStringLiteral("分块 FFT 幅值（块步进=块长-重叠，块平均；输出长度=≥块长的最小 2 的幂，双边未折叠）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> out;
					blockSpectrum(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						0, out, e);
					return out;
				}),
			fx (QStringLiteral("blockfftreal"), QStringLiteral("blockfftreal(vec, block_size, overlap)"), { "vec", "block_size", "overlap" }, { "v", "s", "s" }, QStringLiteral("分块 FFT 实部（同 blockfftmag 块平均）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> out;
					blockSpectrum(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						1, out, e);
					return out;
				}),
			fx (QStringLiteral("blockfftimag"), QStringLiteral("blockfftimag(vec, block_size, overlap)"), { "vec", "block_size", "overlap" }, { "v", "s", "s" }, QStringLiteral("分块 FFT 虚部（同 blockfftmag 块平均）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> out;
					blockSpectrum(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						2, out, e);
					return out;
				}),
			fx (QStringLiteral("blockfftphase"), QStringLiteral("blockfftphase(vec, block_size, overlap)"), { "vec", "block_size", "overlap" }, { "v", "s", "s" }, QStringLiteral("分块 FFT 相位（同 blockfftmag 块平均）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> out;
					blockSpectrum(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						3, out, e);
					return out;
				}),
			fx (QStringLiteral("blockfrfmag"), QStringLiteral("blockfrfmag(input, output, block_size, overlap)"), { "input", "output", "block_size", "overlap" }, { "v", "v", "s", "s" }, QStringLiteral("分块 FRF 幅值（每块 H=FFT(输出)/FFT(输入) 块平均，输入谱零频点不计）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> out;
					blockFrf(yAt(a, 0), yAt(a, 1),
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						0, out, e);
					return out;
				}),
			fx (QStringLiteral("blockfrfreal"), QStringLiteral("blockfrfreal(input, output, block_size, overlap)"), { "input", "output", "block_size", "overlap" }, { "v", "v", "s", "s" }, QStringLiteral("分块 FRF 实部（同 blockfrfmag 块平均）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> out;
					blockFrf(yAt(a, 0), yAt(a, 1),
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						1, out, e);
					return out;
				}),
			fx (QStringLiteral("blockfrfimag"), QStringLiteral("blockfrfimag(input, output, block_size, overlap)"), { "input", "output", "block_size", "overlap" }, { "v", "v", "s", "s" }, QStringLiteral("分块 FRF 虚部（同 blockfrfmag 块平均）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> out;
					blockFrf(yAt(a, 0), yAt(a, 1),
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						2, out, e);
					return out;
				}),
			fx (QStringLiteral("blockfrfphase"), QStringLiteral("blockfrfphase(input, output, block_size, overlap)"), { "input", "output", "block_size", "overlap" }, { "v", "v", "s", "s" }, QStringLiteral("分块 FRF 相位（同 blockfrfmag 块平均）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					QVector<double> out;
					blockFrf(yAt(a, 0), yAt(a, 1),
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						3, out, e);
					return out;
				}),
			fx (QStringLiteral("frfmag"), QStringLiteral("frfmag(input, output)"), { "input", "output" }, { "v", "v" }, QStringLiteral("FRF 幅值（H=FFT(输出)/FFT(输入)，输出长度=≥N 最小 2 的幂；输入谱零频点置 0）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{
					QVector<double> out;
					frfSpectrum(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						a.ys.size() > 1 ? *a.ys[1] : QVector<double>(), 0, out, e);
					return out;
				}),
			fx (QStringLiteral("frfreal"), QStringLiteral("frfreal(input, output)"), { "input", "output" }, { "v", "v" }, QStringLiteral("FRF 实部"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{
					QVector<double> out;
					frfSpectrum(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						a.ys.size() > 1 ? *a.ys[1] : QVector<double>(), 1, out, e);
					return out;
				}),
			fx (QStringLiteral("frfimag"), QStringLiteral("frfimag(input, output)"), { "input", "output" }, { "v", "v" }, QStringLiteral("FRF 虚部"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{
					QVector<double> out;
					frfSpectrum(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						a.ys.size() > 1 ? *a.ys[1] : QVector<double>(), 2, out, e);
					return out;
				}),
			fx (QStringLiteral("frfphase"), QStringLiteral("frfphase(input, output)"), { "input", "output" }, { "v", "v" }, QStringLiteral("FRF 相位（弧度）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{
					QVector<double> out;
					frfSpectrum(a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
						a.ys.size() > 1 ? *a.ys[1] : QVector<double>(), 3, out, e);
					return out;
				}),
			fx (QStringLiteral("coherence"), QStringLiteral("coherence(y1, y2)"), { "y1", "y2" }, { "v", "v" }, QStringLiteral("相干函数（Welch 分段 H1 型估计，单边）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{
					const QVector<double> v1 = yAt(a, 0);
					const QVector<double> v2 = yAt(a, 1);
					return xfCoherence(v1, v2, e);
				}),
			fx (QStringLiteral("fcorr"), QStringLiteral("fcorr(y1, y2)"), { "y1", "y2" }, { "v", "v" }, QStringLiteral("循环互相关（FFT 实现，滞后 0..n-1）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfFcorr(yAt(a, 0), yAt(a, 1), e); }),
			fx (QStringLiteral("dcorr"), QStringLiteral("dcorr(y1, y2)"), { "y1", "y2" }, { "v", "v" }, QStringLiteral("循环互相关（直接求和，与 fcorr 同式）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfDcorr(yAt(a, 0), yAt(a, 1), e); }),
			fx (QStringLiteral("psd"), QStringLiteral("psd(vector)"), { "vector" }, { "v" }, QStringLiteral("功率谱密度（Welch 4 段 Hann，单边，fs 归一化）"), kCatSpectrum, wrapXfm(&xfPsd)),
			fx (QStringLiteral("psd5"), QStringLiteral("psd5(time, amplitude, block, overlap)"), { "time", "amplitude", "block", "overlap" }, { "v", "v", "s", "s" }, QStringLiteral("修正功率谱密度（逐块去均值+Hann+幅值补偿，双边密度；官方算例幅值 10@20Hz→PSD(20)=8.3333）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return xfPsd5(yAt(a, 0), yAt(a, 1),
					sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0, e); }),
			fx (QStringLiteral("asdcal"), QStringLiteral("asdcal(time, mag, type, block_size, overlap, mean_corr, window, averaging)"), { "time", "mag", "type", "block_size", "overlap", "mean_corr", "window", "averaging" }, { "v", "v", "s", "s", "s", "s", "s", "s" }, QStringLiteral("幅值谱密度（type 0=真值幅值 1=RMS 3=PSD；overlap 百分数；去均值/窗/平均方式可配；输出双边块长谱）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					return xfAsdcal(yAt(a, 0), yAt(a, 1),
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						sc.size() > 2 ? sc.at(2) : 0.0, sc.size() > 3 ? sc.at(3) : 0.0,
						sc.size() > 4 ? sc.at(4) : 0.0, sc.size() > 5 ? sc.at(5) : 0.0, e);
				}),
			fx (QStringLiteral("hilbert"), QStringLiteral("hilbert(vector)"), { "vector" }, { "v" }, QStringLiteral("Hilbert 变换（解析信号正交分量）"), kCatSpectrum, wrapXfm(&xfHilbert)),
			r  (QStringLiteral("srs"), QStringLiteral("srs(vector)"), { "vector" }, QStringLiteral("冲击响应谱（参考专用语义）"), kCatSpectrum, Kind::VectorTransform),
			r  (QStringLiteral("rrs"), QStringLiteral("rrs(vector)"), { "vector" }, QStringLiteral("rrs 谱（参考专用语义）"), kCatSpectrum, Kind::VectorTransform),
			r  (QStringLiteral("sgcm"), QStringLiteral("sgcm(...)"), { "vector" }, QStringLiteral("Sprague-Geers 组合度量（官方：返回 [C,SG,幅值比,相位差]；度量公式以图片呈现无法提取，需公式文本）"), kCatSpectrum),
			r  (QStringLiteral("wifac"), QStringLiteral("wifac(...)"), { "vector" }, QStringLiteral("Weighted Integrated Factor（官方：返回 [峰值,峰值时间,WIFac,总分]；Peak/PeakTime 公式已提取，WIFac 分数公式为图片，需公式文本）"), kCatSpectrum),
			r  (QStringLiteral("OLC"), QStringLiteral("OLC(...)"), { "vector" }, QStringLiteral("Occupant Load Criteria（官方：flag 0=|斜率| 1=OLC++ 2/3=OLC 曲线 4=Tv0 5=SM25 6=V0 7=全部；算法图为图片，需公式文本后实现 flag 0/2/3）"), kCatSpectrum),
			fx (QStringLiteral("batch"), QStringLiteral("batch(indep, dep, low, high[, extension])"), { "indep", "dep", "low", "high" }, { "v", "v", "s", "s", "s" }, QStringLiteral("FFT 理想滤波（low=0 低通/high=0 高通/high>low 带通/high<low 带阻/截止>Nyquist 不滤波；可选延拓点数）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return xfBatch(yAt(a, 0), yAt(a, 1),
					sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
					sc.size() > 2 ? sc.at(2) : 0.0, e); }),
			fx (QStringLiteral("batchpolyfit"), QStringLiteral("batchpolyfit(x, y, low, high, poly_order)"), { "x", "y", "low", "high", "poly_order" }, { "v", "v", "s", "s", "s" }, QStringLiteral("多项式扣除+batch 滤波残差+加回（消除趋势项的带通滤波）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return xfBatchpolyfit(yAt(a, 0), yAt(a, 1),
					sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
					sc.size() > 2 ? sc.at(2) : 0.0, e); }),
			fx (QStringLiteral("irfilter"), QStringLiteral("irfilter(dep_vec, FIR_coeffs[, IIR_coeffs])"), { "dep_vec", "FIR_coeffs" }, { "v", "v", "v" }, QStringLiteral("脉冲响应滤波（FIR 卷积；IIR 系数向量省略恒为 1 的 a0，差分反馈）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{
					const bool hasIir = a.ys.size() > 2;
					const QVector<double> aVec = hasIir ? *a.ys[2] : QVector<double>();
					return xfIrfilter(yAt(a, 0), yAt(a, 1), aVec, hasIir, e);
				}),
			fx (QStringLiteral("saefilter"), QStringLiteral("saefilter(indep, dep, class)"), { "indep", "dep", "class" }, { "v", "v", "s" }, QStringLiteral("SAE class 低通（零相位双向二阶 Butterworth，fL=2.5·class；官方规则 fs<3.2·class 不滤波）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return xfSaefilter(yAt(a, 0), yAt(a, 1), sc.isEmpty() ? 0.0 : sc.at(0), e); }),
			fx (QStringLiteral("smean"), QStringLiteral("smean(x_in, y_in, winsize, boundarywinsize, numrep)"), { "x_in", "y_in", "winsize", "boundarywinsize", "numrep" }, { "v", "v", "s", "s", "s" }, QStringLiteral("滑动均值积分（窗口宽 winsize 为 x 单位，梯形积分/窗口宽；边界模式 1/2 夹取 3=镜像；numrep 重复次数）"), kCatSpectrum,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					return xfSmean5(yAt(a, 0), yAt(a, 1),
						sc.size() > 0 ? sc.at(0) : 0.0, sc.size() > 1 ? sc.at(1) : 0.0,
						sc.size() > 2 ? sc.at(2) : 1.0, e);
				}),
			fx (QStringLiteral("blackman"), QStringLiteral("blackman(n)"), { "n" }, { "s" }, QStringLiteral("Blackman 窗系数（长度 n）"), kCatSpectrum,
				[](const VecArgs&, const QVector<double>& sc, QString* e)
				{
					const int n = sc.isEmpty() ? 0 : int(sc.at(0));
					if (n <= 0) { if (e) *e = QStringLiteral("窗长度须为正整数"); return QVector<double>(); }
					QVector<double> out(n);
					if (n == 1) { out[0] = 1.0; return out; }
					for (int k = 0; k < n; ++k)
					{
						const double t = 2.0 * kPi * double(k) / double(n - 1);
						out[k] = 0.42 - 0.5 * std::cos(t) + 0.08 * std::cos(2.0 * t);
					}
					return out;
				}),
			fx (QStringLiteral("hamming"), QStringLiteral("hamming(n)"), { "n" }, { "s" }, QStringLiteral("Hamming 窗系数（长度 n）"), kCatSpectrum,
				[](const VecArgs&, const QVector<double>& sc, QString* e)
				{
					const int n = sc.isEmpty() ? 0 : int(sc.at(0));
					if (n <= 0) { if (e) *e = QStringLiteral("窗长度须为正整数"); return QVector<double>(); }
					QVector<double> out(n);
					if (n == 1) { out[0] = 1.0; return out; }
					for (int k = 0; k < n; ++k)
						out[k] = 0.54 - 0.46 * std::cos(2.0 * kPi * double(k) / double(n - 1));
					return out;
				}),
			fx (QStringLiteral("hanning"), QStringLiteral("hanning(n)"), { "n" }, { "s" }, QStringLiteral("Hanning 窗系数（长度 n）"), kCatSpectrum,
				[](const VecArgs&, const QVector<double>& sc, QString* e)
				{
					const int n = sc.isEmpty() ? 0 : int(sc.at(0));
					if (n <= 0) { if (e) *e = QStringLiteral("窗长度须为正整数"); return QVector<double>(); }
					QVector<double> out(n);
					if (n == 1) { out[0] = 1.0; return out; }
					for (int k = 0; k < n; ++k)
						out[k] = 0.5 * (1.0 - std::cos(2.0 * kPi * double(k) / double(n - 1)));
					return out;
				}),
			fx (QStringLiteral("kaiser"), QStringLiteral("kaiser(n, beta)"), { "n", "beta" }, { "s", "s" }, QStringLiteral("Kaiser 窗系数（长度 n，参数 beta）"), kCatSpectrum,
				[](const VecArgs&, const QVector<double>& sc, QString* e)
				{
					const int n = sc.isEmpty() ? 0 : int(sc.at(0));
					const double beta = sc.size() > 1 ? sc.at(1) : 0.0;
					if (n <= 0) { if (e) *e = QStringLiteral("窗长度须为正整数"); return QVector<double>(); }
					const double den = besselI0(beta);
					QVector<double> out(n);
					if (n == 1) { out[0] = 1.0; return out; }
					for (int k = 0; k < n; ++k)
					{
						const double r = 2.0 * double(k) / double(n - 1) - 1.0;
						out[k] = besselI0(beta * std::sqrt(std::max(0.0, 1.0 - r * r))) / den;
					}
					return out;
				}),
			fx (QStringLiteral("parzen"), QStringLiteral("parzen(n)"), { "n" }, { "s" }, QStringLiteral("Parzen 窗系数（长度 n）"), kCatSpectrum,
				[](const VecArgs&, const QVector<double>& sc, QString* e)
				{
					const int n = sc.isEmpty() ? 0 : int(sc.at(0));
					if (n <= 0) { if (e) *e = QStringLiteral("窗长度须为正整数"); return QVector<double>(); }
					QVector<double> out(n);
					if (n == 1) { out[0] = 1.0; return out; }
					const double half = double(n - 1) / 2.0;
					for (int k = 0; k < n; ++k)
					{
						const double d = std::fabs(double(k) - half) / half;
						out[k] = d <= 0.5 ? 1.0 - 6.0 * d * d + 6.0 * d * d * d
						                  : 2.0 * std::pow(1.0 - d, 3);
					}
					return out;
				}),
			fx (QStringLiteral("welch"), QStringLiteral("welch(n)"), { "n" }, { "s" }, QStringLiteral("Welch 窗系数（长度 n）"), kCatSpectrum,
				[](const VecArgs&, const QVector<double>& sc, QString* e)
				{
					const int n = sc.isEmpty() ? 0 : int(sc.at(0));
					if (n <= 0) { if (e) *e = QStringLiteral("窗长度须为正整数"); return QVector<double>(); }
					QVector<double> out(n);
					if (n == 1) { out[0] = 1.0; return out; }
					const double half = double(n - 1) / 2.0;
					for (int k = 0; k < n; ++k)
					{
						const double r = (double(k) - half) / half;
						out[k] = 1.0 - r * r;
					}
					return out;
				}),
			r  (QStringLiteral("stft"), QStringLiteral("stft(vector)"), { "vector" }, QStringLiteral("短时傅里叶变换（官方：瀑布图 3D 面板辅助，amp_type/amp_scale 为字符串参数，数值引擎不支持）"), kCatSpectrum, Kind::VectorTransform),
			r  (QStringLiteral("stfreq"), QStringLiteral("stfreq(...)"), { "vector" }, QStringLiteral("瀑布图频率向量（官方：seg_locs 为 \"slices\"/\"steps\" 字符串参数，数值引擎不支持）"), kCatSpectrum, Kind::VectorTransform),
			r  (QStringLiteral("storder"), QStringLiteral("storder(...)"), { "vector" }, QStringLiteral("瀑布图阶次向量（官方：瀑布图 3D 面板辅助，字符串参数，数值引擎不支持）"), kCatSpectrum, Kind::VectorTransform),
			r  (QStringLiteral("vtdft"), QStringLiteral("vtdft(...)"), { "vector" }, QStringLiteral("变时间 DFT（官方：瀑布图 3D 面板辅助，窗型/量纲为字符串参数，数值引擎不支持）"), kCatSpectrum, Kind::VectorTransform),
			r  (QStringLiteral("vtfreq"), QStringLiteral("vtfreq(...)"), { "vector" }, QStringLiteral("变时间瀑布图频率（官方：瀑布图辅助，字符串参数，数值引擎不支持）"), kCatSpectrum, Kind::VectorTransform),
			r  (QStringLiteral("vtorder"), QStringLiteral("vtorder(...)"), { "vector" }, QStringLiteral("变时间瀑布图阶次（官方：瀑布图辅助，字符串参数，数值引擎不支持）"), kCatSpectrum, Kind::VectorTransform),
			//======== 5 矩阵/向量数组函数
			fx (QStringLiteral("array"), QStringLiteral("array(size1[, size2])"), { "size1" }, { "s", "s" }, QStringLiteral("全零数组（array(3)={0,0,0}；两参矩阵形式返回展平 n·m 零向量，官方注明曲线编辑场景不适用矩阵）"), kCatMatrix,
				[](const VecArgs&, const QVector<double>& sc, QString* e)
				{ return aggArray(sc.size() > 0 ? sc.at(0) : 0.0,
					sc.size() > 1 ? sc.at(1) : 0.0, sc.size() > 1, e); }),
			fx (QStringLiteral("cat"), QStringLiteral("cat(a, b)"), { "a", "b" }, { "v", "v" }, QStringLiteral("数组拼接"), kCatMatrix,
				wrapXfm2([](const QVector<double>& a, const QVector<double>&, const QVector<double>& b, QString* e)
				{ return xfCat(a, b, e); })),
			fx (QStringLiteral("catoffset"), QStringLiteral("catoffset(a, b, off)"), { "a", "b", "off" }, { "v", "v", "s" }, QStringLiteral("带偏移拼接（后半段逐点加 off）"), kCatMatrix,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					const QVector<double> va = yAt(a, 0);
					const QVector<double> vb = yAt(a, 1);
					const double off = sc.isEmpty() ? 0.0 : sc.at(0);
					if (!checkVec(va, e) || !checkVec(vb, e))
						return QVector<double>();
					QVector<double> out = va;
					for (double v : vb)
						out.push_back(v + off);
					return out;
				}),
			fx (QStringLiteral("determinant"), QStringLiteral("determinant(m)"), { "m" }, { "v" }, QStringLiteral("行列式（m 为行主序展平方阵，长度须为完全平方数）"), kCatMatrix,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfDetOrInv(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], false, e); }),
			fx (QStringLiteral("inverse"), QStringLiteral("inverse(m)"), { "m" }, { "v" }, QStringLiteral("矩阵求逆（行主序展平方阵，返回行主序展平）"), kCatMatrix,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfDetOrInv(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], true, e); }),
			fx (QStringLiteral("eigvalreal"), QStringLiteral("eigvalreal(m)"), { "m" }, { "v" }, QStringLiteral("特征值实部（LeVerrier 特征多项式+DK 求根，n≤8 内可靠）"), kCatMatrix,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return eigValues(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], true, e); }),
			fx (QStringLiteral("eigvalimag"), QStringLiteral("eigvalimag(m)"), { "m" }, { "v" }, QStringLiteral("特征值虚部（顺序与 eigvalreal 一致）"), kCatMatrix,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return eigValues(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], false, e); }),
			fx (QStringLiteral("eigvecreal"), QStringLiteral("eigvecreal(m)"), { "m" }, { "v" }, QStringLiteral("特征向量实部（第 k 列对应第 k 个特征值，行主序展平，逆迭代）"), kCatMatrix,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return eigVectors(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], true, e); }),
			fx (QStringLiteral("eigvecimag"), QStringLiteral("eigvecimag(m)"), { "m" }, { "v" }, QStringLiteral("特征向量虚部（顺序与 eigvecreal 一致）"), kCatMatrix,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return eigVectors(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], false, e); }),
			fa (QStringLiteral("size"), QStringLiteral("size(a)"), { "a" }, QStringLiteral("数组长度"), kCatMatrix, wrapAggY(&aggSize)),
			fx (QStringLiteral("sort"), QStringLiteral("sort(a)"), { "a" }, { "v" }, QStringLiteral("数组排序（升序）"), kCatMatrix, wrapXfm(&xfSort)),
			fx (QStringLiteral("subrange"), QStringLiteral("subrange(a, i, j)"), { "a", "i", "j" }, { "v", "s", "s" }, QStringLiteral("子区间提取（闭区间 [i,j]，越界夹取）"), kCatMatrix,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{
					const double i = sc.size() > 0 ? sc.at(0) : 0.0;
					const double j = sc.size() > 1 ? sc.at(1) : 0.0;
					return xfSubrange(yAt(a, 0), i, j, e);
				}),
			fa (QStringLiteral("numpts"), QStringLiteral("numpts(a)"), { "a" }, QStringLiteral("数据点数"), kCatMatrix, wrapAggY(&aggSize)),
			fx (QStringLiteral("remove_succ_dup_indx"), QStringLiteral("remove_succ_dup_indx(a)"), { "a" }, { "v" }, QStringLiteral("连续重复点索引（相邻等值点的后续者，保留每组首个）"), kCatMatrix, wrapXfm(&xfRemoveSuccDup)),
			//======== 6 字符串与字符转换（数值引擎无字符串类型，整体保持占位）
			r  (QStringLiteral("asc"), QStringLiteral("asc(s)"), { "s" }, QStringLiteral("字符转 ASCII 码（引擎为纯数值表达式，不支持字符串参数）"), kCatString),
			r  (QStringLiteral("chr"), QStringLiteral("chr(n)"), { "n" }, QStringLiteral("ASCII 码转字符（返回字符串，数值引擎不支持）"), kCatString),
			r  (QStringLiteral("itoa"), QStringLiteral("itoa(n)"), { "n" }, QStringLiteral("整数转字符串（返回字符串，数值引擎不支持）"), kCatString),
			r  (QStringLiteral("atoi"), QStringLiteral("atoi(s)"), { "s" }, QStringLiteral("字符串转整数（不支持字符串参数）"), kCatString),
			r  (QStringLiteral("tolower"), QStringLiteral("tolower(s)"), { "s" }, QStringLiteral("转小写（返回字符串，数值引擎不支持）"), kCatString),
			r  (QStringLiteral("toupper"), QStringLiteral("toupper(s)"), { "s" }, QStringLiteral("转大写（返回字符串，数值引擎不支持）"), kCatString),
			r  (QStringLiteral("mid"), QStringLiteral("mid(s, i, n)"), { "s", "i", "n" }, QStringLiteral("截取子串（返回字符串，数值引擎不支持）"), kCatString),
			r  (QStringLiteral("strlen"), QStringLiteral("strlen(s)"), { "s" }, QStringLiteral("字符串长度（不支持字符串参数）"), kCatString),
			r  (QStringLiteral("strchr"), QStringLiteral("strchr(s, c)"), { "s", "c" }, QStringLiteral("查找字符（不支持字符串参数）"), kCatString),
			r  (QStringLiteral("strrchr"), QStringLiteral("strrchr(s, c)"), { "s", "c" }, QStringLiteral("查找字符（不支持字符串参数）"), kCatString),
			r  (QStringLiteral("strstr"), QStringLiteral("strstr(s, sub)"), { "s", "sub" }, QStringLiteral("查找子串（不支持字符串参数）"), kCatString),
			r  (QStringLiteral("replace"), QStringLiteral("replace(s, old, new)"), { "s", "old", "new" }, QStringLiteral("替换子串（返回字符串，数值引擎不支持）"), kCatString),
			r  (QStringLiteral("stringarray"), QStringLiteral("stringarray(...)"), { "items" }, QStringLiteral("字符串数组拼接（不支持字符串参数）"), kCatString),
			r  (QStringLiteral("cr"), QStringLiteral("cr"), { }, QStringLiteral("换行符常量（字符串，数值引擎不支持）"), kCatString, Kind::Constant),
			//======== 7 文件 IO 与结果读取（多数需文档/文件上下文；curvex/curvey 映射引用）
			fx (QStringLiteral("curvex"), QStringLiteral("curvex(ref)"), { "ref" }, { "v" }, QStringLiteral("曲线 X 向量查询（= 引用的横坐标序列）"), kCatFileIO,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfCurvex(QVector<double>(), a.xs.isEmpty() ? QVector<double>() : *a.xs[0], e); }),
			fx (QStringLiteral("curvey"), QStringLiteral("curvey(ref)"), { "ref" }, { "v" }, QStringLiteral("曲线 Y 向量查询（= 引用本身）"), kCatFileIO,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfCurvey(a.ys.isEmpty() ? QVector<double>() : *a.ys[0], QVector<double>(), e); }),
			r  (QStringLiteral("read"), QStringLiteral("read(...)"), { "file" }, QStringLiteral("读结果文件（需文件路径与文档上下文，超出表达式引擎）"), kCatFileIO),
			r  (QStringLiteral("readsim"), QStringLiteral("readsim(...)"), { "file" }, QStringLiteral("读仿真结果（需文件上下文）"), kCatFileIO),
			r  (QStringLiteral("readsubcase"), QStringLiteral("readsubcase(...)"), { "file" }, QStringLiteral("读子工况（需文件上下文）"), kCatFileIO),
			r  (QStringLiteral("fileopen"), QStringLiteral("fileopen(...)"), { "file" }, QStringLiteral("打开文件（需文件上下文）"), kCatFileIO),
			r  (QStringLiteral("fileclose"), QStringLiteral("fileclose(...)"), { "file" }, QStringLiteral("关闭文件（需文件上下文）"), kCatFileIO),
			r  (QStringLiteral("fileexists"), QStringLiteral("fileexists(path)"), { "path" }, QStringLiteral("文件存在性判断（需路径字符串参数）"), kCatFileIO),
			r  (QStringLiteral("filefindstring"), QStringLiteral("filefindstring(...)"), { "file" }, QStringLiteral("文件内查找字符串（不支持字符串参数）"), kCatFileIO),
			r  (QStringLiteral("fileinfo"), QStringLiteral("fileinfo(path)"), { "path" }, QStringLiteral("文件信息（需路径字符串参数）"), kCatFileIO),
			r  (QStringLiteral("abstorelative"), QStringLiteral("abstorelative(path)"), { "path" }, QStringLiteral("绝对路径转相对（返回字符串）"), kCatFileIO),
			r  (QStringLiteral("relativetoabs"), QStringLiteral("relativetoabs(path)"), { "path" }, QStringLiteral("相对路径转绝对（返回字符串）"), kCatFileIO),
			r  (QStringLiteral("getvalue"), QStringLiteral("getvalue(...)"), { "file" }, QStringLiteral("取结果值（需文档上下文）"), kCatFileIO),
			faK(QStringLiteral("getval2"), QStringLiteral("getval2(index_vec, data_vec, value)"), { "index_vec", "data_vec", "value" }, { "v", "v", "s" }, QStringLiteral("index_vec 中首个等于 value 的位置 i，返回 data_vec[i]（未找到/越界报错；官方示例 getval2({1,2,3,4,5},{4,5,6,7,8},2)=5）"), kCatFileIO,
				[](const VecArgs& a, const QVector<double>& sc, QString* e)
				{ return aggGetval2(a.ys.size() > 1 ? *a.ys[1] : QVector<double>(),
					a.ys.isEmpty() ? QVector<double>() : *a.ys[0],
					sc.isEmpty() ? 0.0 : sc.at(0), e); }),
			r  (QStringLiteral("curvelabel"), QStringLiteral("curvelabel(ref)"), { "ref" }, QStringLiteral("曲线标签查询（需文档上下文）"), kCatFileIO),
			r  (QStringLiteral("numcurves"), QStringLiteral("numcurves(...)"), { }, QStringLiteral("曲线数量查询（需文档上下文）"), kCatFileIO, Kind::Aggregate),
			r  (QStringLiteral("projectcurvetosurface"), QStringLiteral("projectcurvetosurface(...)"), { "ref" }, QStringLiteral("曲线投影到曲面（需曲面上下文）"), kCatFileIO),
			//======== 8 系统与 Tcl 交互（参考软件内嵌 Tcl；Qt 应用不移植，整体占位）
			r  (QStringLiteral("eval"), QStringLiteral("eval(expr)"), { "expr" }, QStringLiteral("表达式求值（参数为表达式，待表达式参数机制）"), kCatSystem),
			r  (QStringLiteral("fill"), QStringLiteral("fill(...)"), { "value" }, QStringLiteral("填充生成（参考语义待核对）"), kCatSystem),
			r  (QStringLiteral("getenv"), QStringLiteral("getenv(name)"), { "name" }, QStringLiteral("环境变量读取（返回字符串）"), kCatSystem),
			r  (QStringLiteral("tcleval"), QStringLiteral("tcleval(script)"), { "script" }, QStringLiteral("Tcl 表达式求值（不移植 Tcl 解释器）"), kCatSystem),
			r  (QStringLiteral("tclevalfile"), QStringLiteral("tclevalfile(path)"), { "path" }, QStringLiteral("执行 Tcl 脚本文件（不移植 Tcl 解释器）"), kCatSystem),
			r  (QStringLiteral("tclgetvar"), QStringLiteral("tclgetvar(name)"), { "name" }, QStringLiteral("读 Tcl 变量（不移植 Tcl 解释器）"), kCatSystem),
			r  (QStringLiteral("date"), QStringLiteral("date"), { }, QStringLiteral("当前日期（返回字符串）"), kCatSystem, Kind::Constant),
			r  (QStringLiteral("time"), QStringLiteral("time"), { }, QStringLiteral("当前时间（返回字符串）"), kCatSystem, Kind::Constant),
			r  (QStringLiteral("timer"), QStringLiteral("timer"), { }, QStringLiteral("计时（语义待核对）"), kCatSystem, Kind::Constant),
			r  (QStringLiteral("sysid"), QStringLiteral("sysid"), { }, QStringLiteral("系统标识（返回字符串）"), kCatSystem, Kind::Constant),
			r  (QStringLiteral("system"), QStringLiteral("system(cmd)"), { "cmd" }, QStringLiteral("执行系统命令（安全性不适用表达式求值）"), kCatSystem),
			r  (QStringLiteral("freeze"), QStringLiteral("freeze(...)"), { "var" }, QStringLiteral("冻结变量（语义待核对）"), kCatSystem),
			r  (QStringLiteral("thaw"), QStringLiteral("thaw(...)"), { "var" }, QStringLiteral("解冻变量（语义待核对）"), kCatSystem),
			r  (QStringLiteral("typeof"), QStringLiteral("typeof(v)"), { "v" }, QStringLiteral("变量类型判断（返回字符串）"), kCatSystem),
			faK(QStringLiteral("iffunc"), QStringLiteral("iffunc(expr1, expr2, expr3)"), { "expr1", "expr2", "expr3" }, { "s", "s", "s" }, QStringLiteral("条件取值（expr1 非零返回 expr2，否则 expr3；官方示例 iffunc(1,2,3)=2；官方 expr2/expr3 可为向量，本引擎为标量版）"), kCatSystem,
				[](const VecArgs&, const QVector<double>& sc, QString*)
				{ return aggIffunc(sc.size() > 0 ? sc.at(0) : 0.0,
					sc.size() > 1 ? sc.at(1) : 0.0, sc.size() > 2 ? sc.at(2) : 0.0); }),
			//======== 9 工程评价专用函数（deviation_metric/reldisp 参考专用语义待核对）
			r  (QStringLiteral("deviation_metric"), QStringLiteral("deviation_metric(...)"), { "ref" }, QStringLiteral("动刚度/损耗角偏差度量（官方：deviation_metric(v1, v2, flag) flag 1=动刚度偏差 2=损耗角偏差；官方页未给出公式，需内部算法定义）"), kCatEngEval),
			r  (QStringLiteral("reldisp"), QStringLiteral("reldisp(...)"), { "ref" }, QStringLiteral("相对位移变换（官方：reldisp 需 LS-DYNA/PAM-CRASH 结果文件读取与字符串参数，数值引擎不支持）"), kCatEngEval),
			fx (QStringLiteral("fold"), QStringLiteral("fold(vector)"), { "vector" }, { "v" }, QStringLiteral("幅值折叠（负幅值镜像为正 |y|）"), kCatEngEval, wrapXfm(&xfFold)),
			fx (QStringLiteral("intercepts"), QStringLiteral("intercepts(y, x)"), { "y", "x" }, { "v", "v" }, QStringLiteral("与零线交点横坐标（线性插值，含端点恰为 0）"), kCatEngEval,
				[](const VecArgs& a, const QVector<double>&, QString* e)
				{ return xfIntercepts(yAt(a, 0), yAt(a, 1), e); }),
			fx (QStringLiteral("intersect"), QStringLiteral("intersect(a, b)"), { "a", "b" }, { "v", "v" }, QStringLiteral("两曲线交点横坐标（b 插值到 a 的 x 后求差曲线零点）"), kCatEngEval, wrapXfm2(&xfIntersect)),
			//======== 10 自定义扩展函数
			r  (QStringLiteral("extern"), QStringLiteral("extern(name, ...)"), { "name" }, QStringLiteral("调用外部注册的 Templex 函数（.tfn，不适用本引擎）"), kCatCustom),
			//======== 本引擎补充（当前已实现但《函数类型.xlsx》目录未列的项）
			fs (QStringLiteral("asinh"),  QStringLiteral("asinh(vector)"),  { "vector" }, QStringLiteral("反双曲正弦"), kCatExtra, &fnAsinh),
			fs (QStringLiteral("acosh"),  QStringLiteral("acosh(vector)"),  { "vector" }, QStringLiteral("反双曲余弦"), kCatExtra, &fnAcosh),
			fs (QStringLiteral("atanh"),  QStringLiteral("atanh(vector)"),  { "vector" }, QStringLiteral("反双曲正切"), kCatExtra, &fnAtanh),
			fs (QStringLiteral("sign"),   QStringLiteral("sign(vector)"),   { "vector" }, QStringLiteral("符号（-1/0/1）"), kCatExtra, &fnSign),
			fs (QStringLiteral("rint"),   QStringLiteral("rint(vector)"),   { "vector" }, QStringLiteral("四舍五入到最近整数"), kCatExtra, &fnRint),
			fs (QStringLiteral("log2"),   QStringLiteral("log2(vector)"),   { "vector" }, QStringLiteral("以 2 为底的对数"), kCatExtra, &fnLog2),
			fs (QStringLiteral("deg2rad"), QStringLiteral("deg2rad(vector)"), { "vector" }, QStringLiteral("角度转弧度"), kCatExtra, &fnDeg2Rad),
			fs (QStringLiteral("rad2deg"), QStringLiteral("rad2deg(vector)"), { "vector" }, QStringLiteral("弧度转角度"), kCatExtra, &fnRad2Deg),
			fc (QStringLiteral("pi"),     QStringLiteral("pi"),             QStringLiteral("圆周率 π"), kCatExtra),
			fc (QStringLiteral("e"),      QStringLiteral("e"),              QStringLiteral("自然常数 e"), kCatExtra),
		};
		return table;
	}

	const mCurveMathFunctions::FuncInfo* mCurveMathFunctions::find(const QString& name)
	{
		static const QHash<QString, const FuncInfo*> index = []()
		{
			QHash<QString, const FuncInfo*> map;
			for (const FuncInfo& f : catalog())
				map.insert(f.name, &f);
			return map;
		}();
		return index.value(name, nullptr);
	}

	bool mCurveMathFunctions::constantValue(const QString& name, double& out)
	{
		const FuncInfo* info = find(name);
		if (!info || info->kind != Kind::Constant || !info->implemented)
			return false;
		if (name == QLatin1String("pi"))
		{
			out = 3.14159265358979323846;
			return true;
		}
		if (name == QLatin1String("e"))
		{
			out = 2.71828182845904523536;
			return true;
		}
		return false;
	}
}
