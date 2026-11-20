#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(FRPDATA_LIB)
#  define MXFRPDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXFRPDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXFRPDATA_EXPORT
#endif
