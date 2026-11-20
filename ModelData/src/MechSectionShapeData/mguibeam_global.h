#pragma once
#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(SECTIONSHAPEDATA_LIB)      // ← 改成 CMake 里真正给的那个
#  define MGUIBEAM_EXPORT Q_DECL_EXPORT
# else
#  define MGUIBEAM_EXPORT Q_DECL_IMPORT
# endif
#else
# define MGUIBEAM_EXPORT
#endif