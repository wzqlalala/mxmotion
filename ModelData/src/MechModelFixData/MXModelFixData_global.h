#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(MODELFIXDATA_LIB)
#  define MXMODELFIXDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXMODELFIXDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXMODELFIXDATA_EXPORT
#endif
