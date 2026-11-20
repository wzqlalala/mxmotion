#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(ASSEMBLYDATA_LIB)
#  define MXASSEMBLYDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXASSEMBLYDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXASSEMBLYDATA_EXPORT
#endif
