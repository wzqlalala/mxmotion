#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(MAPRELATION_LIB)
#  define MXMAPRELATION_EXPORT Q_DECL_EXPORT
# else
#  define MXMAPRELATION_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXMAPRELATION_EXPORT
#endif
