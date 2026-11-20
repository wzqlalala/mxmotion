#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(GUIFOUNDATION_LIB)
#  define GUIFOUNDATION_EXPORT Q_DECL_EXPORT
# else
#  define GUIFOUNDATION_EXPORT Q_DECL_IMPORT
# endif
#else
# define GUIFOUNDATION_EXPORT
#endif
