#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(CASEDATA_LIB)
#  define MXCASEDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXCASEDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXCASEDATA_EXPORT
#endif
