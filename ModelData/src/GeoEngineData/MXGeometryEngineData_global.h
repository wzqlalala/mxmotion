#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(GEOENGINEDATA_LIB)
#  define GEOENGINEDATA_EXPORT Q_DECL_EXPORT
# else
#  define GEOENGINEDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define GEOENGINEDATA_EXPORT
#endif
