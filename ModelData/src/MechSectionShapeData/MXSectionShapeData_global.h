#pragma once
#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(SECTIONSHAPE_LIB)
#  define MXSECTIONSHAPEDATA_EXPORT Q_DECL_EXPORT
# else
#  define MXSECTIONSHAPEDATA_EXPORT Q_DECL_IMPORT
# endif
#else
# define MXSECTIONSHAPEDATA_EXPORT
#endif