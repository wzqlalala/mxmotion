#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(MODELDATA_LIB)
#  define MXMODELDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXMODELDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXMODELDATA_EXPORT
#endif
