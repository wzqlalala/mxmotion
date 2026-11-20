#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(COORDINATEDATA_LIB)
#  define MXCOORDINATEDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXCOORDINATEDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXCOORDINATEDATA_EXPORT
#endif
