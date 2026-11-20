#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(SOLVETASKDATA_LIB)
#  define MXSOLVETASKDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXSOLVETASKDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXSOLVETASKDATA_EXPORT
#endif
