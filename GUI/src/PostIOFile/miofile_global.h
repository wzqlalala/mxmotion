#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(MIOFILE_LIB)
#  define MIOFILE_EXPORT Q_DECL_EXPORT
# else
#  define MIOFILE_EXPORT Q_DECL_IMPORT
# endif
#else
# define MIOFILE_EXPORT
#endif

#define IOFILE_NAMESPACE_BEGIN namespace MIOFile {
#define IOFILE_NAMESPACE_END }
