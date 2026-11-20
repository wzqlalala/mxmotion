#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(POSTCOMMON_LIB)
#  define POSTCOMMON_EXPORT Q_DECL_EXPORT
# else
#  define POSTCOMMON_EXPORT Q_DECL_IMPORT
# endif
#else
# define POSTCOMMON_EXPORT
#endif
