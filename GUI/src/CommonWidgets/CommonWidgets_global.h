#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(COMMONWIDGETS_LIB)
#  define COMMONWIDGETS_EXPORT Q_DECL_EXPORT
# else
#  define COMMONWIDGETS_EXPORT Q_DECL_IMPORT
# endif
#else
# define COMMONWIDGETS_EXPORT
#endif
