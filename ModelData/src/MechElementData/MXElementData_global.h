#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(ELEMENTDATA_LIB)
#  define MXELEMENTDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXELEMENTDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXELEMENTDATA_EXPORT
#endif
