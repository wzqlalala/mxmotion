#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(STRUCTOPTIONDATA_LIB)
#  define MXSTRUCTOPTIONDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXSTRUCTOPTIONDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXSTRUCTOPTIONDATA_EXPORT
#endif
