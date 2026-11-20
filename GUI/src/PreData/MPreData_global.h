#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(PREDATA_LIB)
#  define MPREDATA_EXPORT Q_DECL_EXPORT
# else
#  define MPREDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MPREDATA_EXPORT
#endif
