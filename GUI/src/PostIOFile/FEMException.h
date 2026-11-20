#ifndef FEMException_H_
#define FEMException_H_
#include <string>
#include "Core_API.h"
/*
* 异常的基类函数
*
* 命名说明（第三方代码引入审计 2026-11-11）：类名沿用求解器侧 FE* 异常命名
* 惯例，与 FEBio 的异常命名风格相近，但实现为本仓库自研（自有 Core_API 导出、
* 中文消息文案，未拷贝 FEBio 源码）。类名本身不受版权保护，保留现名以兼容
* 既有调用点。
*/
namespace MIOFile
{
	class CORE_API FEException
	{
	public:
		FEException(const char* msg = nullptr);

		const char* what();

		void what(const char* msg, ...);
	private:
		std::string m_what;
	};

	/*
	* 线性方程组的异常
	*
	*/
	class CORE_API LinearSolver_SolveFailed : public FEException
	{
	public:
		LinearSolver_SolveFailed(const char* msg = nullptr) : FEException(msg) {}

	};

	class CORE_API LinearSolver_FactorizationError {};
	/*
	* 产生异常不正常数
	*/
	class CORE_API NANDetected {};
	/*
	* 最大的刚度计算次数
	*/
	class CORE_API MaxStiffnessRecomputations {};
	/*
	* 最大余量误差
	*/
	class CORE_API ExceedMaxResidual {};

	class CORE_API ZeroOrNegativeDENSITY
	{
	public:
		std::string nMat;
		ZeroOrNegativeDENSITY(std::string name) : nMat(name) {}
	};
	/*
	* 单元负雅克比矩阵的错误
	*/
	class CORE_API NegativeJacobian : public FEException
	{
	public:
		int iel;
		int ng;
		double vol;
		NegativeJacobian(int _iel, int _ng, double _vol);
	};
	class NegativeDeformationGradient
	{
	public:
		int iel;
		int ng;
		double vol;
		NegativeDeformationGradient(int _iel, int _ng, double _vol) : iel(_iel), ng(_ng), vol(_vol) {}
	};

	class CORE_API ArcLengthError
	{

	};

	class CORE_API DoRestart {};
	/*
	 * 材料计算的异常
	 */
	class CORE_API Material_ComputeFailed
	{
	};
}

#endif