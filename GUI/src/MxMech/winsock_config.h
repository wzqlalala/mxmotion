// winsock_config.h
#ifndef WINSOCK_CONFIG_H
#define WINSOCK_CONFIG_H

// 防止 Windows.h 包含不必要的组件
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

// 必须先包含的 Windows Socket 头文件
#include <winsock2.h>
#include <ws2tcpip.h>

// 然后是 Windows 系统头文件
#include <windows.h>

#endif // WINSOCK_CONFIG_H