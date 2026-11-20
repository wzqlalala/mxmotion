#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(DISCRETEFIELDDATA_LIB)
#  define MXDISCRETEFIELDDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXDISCRETEFIELDDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXDISCRETEFIELDDATA_EXPORT
#endif
