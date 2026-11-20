#pragma once

#include "mmainwindow_global.h"
#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(MMAINWINDOW_LIB)
#  define MMAINWINDOW_EXPORT Q_DECL_EXPORT
# else
#  define MMAINWINDOW_EXPORT Q_DECL_IMPORT
# endif
#else
# define MMAINWINDOW_EXPORT
#endif
