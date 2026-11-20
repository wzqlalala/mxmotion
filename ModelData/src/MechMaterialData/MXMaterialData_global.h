#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(MATERIALDATA_LIB)
#  define MXMATERIALDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXMATERIALDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXMATERIALDATA_EXPORT
#endif
