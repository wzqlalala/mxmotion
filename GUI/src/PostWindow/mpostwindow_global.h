#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(MPOSTWINDOW_LIB)
#  define MPOSTWINDOW_EXPORT Q_DECL_EXPORT
# else
#  define MPOSTWINDOW_EXPORT Q_DECL_IMPORT
# endif
#else
# define MPOSTWINDOW_EXPORT
#endif
