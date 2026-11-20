#include "FEMException.h"
#include <cstdarg>

namespace MIOFile
{
	FEException::FEException(const char* msg)
	{
		if (msg)
		{
			m_what = msg;
		}
		else
		{
			m_what = " ";
		}
	}

	const char* FEException::what()
	{
		return m_what.c_str();
	}

	void FEException::what(const char* msg, ...)
	{
		// get a pointer to the argument list
		va_list	args;

		// make the message
		char sztxt[1024] = { 0 };
		va_start(args, msg);
		vsprintf(sztxt, msg, args);
		va_end(args);

		m_what = sztxt;
	}

	NegativeJacobian::NegativeJacobian(int _iel, int _ng, double _vol) : iel(_iel), ng(_ng), vol(_vol)
	{
		what("单元%d第%d个积分点的雅克比为%lg！", iel, ng, vol);
	}
}