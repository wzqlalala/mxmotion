#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(CONNECTORDATA_LIB)
#  define MXCONNECTORDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXCONNECTORDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXCONNECTORDATA_EXPORT
#endif
