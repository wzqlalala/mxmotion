#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(PROPERTYDATA_LIB)
#  define MXPROPERTYDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXPROPERTYDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXPROPERTYDATA_EXPORT
#endif
