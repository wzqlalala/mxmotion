#pragma once
#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
#  ifdef APISERVER_LIB          
#    define APISERVER_EXPORT Q_DECL_EXPORT
#  else
#    define APISERVER_EXPORT Q_DECL_IMPORT 
#  endif
#else
#  define APISERVER_EXPORT
#endif