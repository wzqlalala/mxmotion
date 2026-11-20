#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(GLOBALPARAMDATA_LIB)
#  define MXGLOBALPARAMDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXGLOBALPARAMDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXGLOBALPARAMDATA_EXPORT
#endif
