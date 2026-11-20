#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(CONTACTDATA_LIB)
#  define MXCONTACTDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXCONTACTDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXCONTACTDATA_EXPORT
#endif
