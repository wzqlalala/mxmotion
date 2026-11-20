#ifndef MY_CLIENT_H
#define MY_CLIENT_H
#if defined(_WIN32) || defined(_WIN64)
#include "winsock_config.h"
#include "mLicense.h"
#else
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#endif
#include <array>
#include <string>
#include <cstring>
#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

#ifdef __linux__
#include <fstream>
#endif

#include <vector>
#include <string>
#include <system_error>
#include "client.h"

// 声明获取本机所有IP地址的函数
std::pair<std::string, std::string> getFirstLocalIPAddress();

// 获取CPU型号
std::string getCPUModel();

std::unique_ptr<Client> initializeClient();  // 只声明，不实现

#endif // MY_CLIENT_H