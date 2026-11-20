#include "ip_utils.h"
#include <iostream>
#include <thread>



// 获取CPU型号
std::string getCPUModel() {
	std::string cpuModel;

#ifdef _WIN32
	// Windows平台：使用__cpuid获取CPU型号
	std::array<int, 4> cpuInfo;

	// 获取CPU型号信息
	for (int i = 0x80000002; i <= 0x80000004; ++i) {
		__cpuid(cpuInfo.data(), i);
		cpuModel.append(reinterpret_cast<char*>(cpuInfo.data()), 16);
	}

#elif __linux__
	// Linux平台：读取/proc/cpuinfo文件获取CPU型号
	std::ifstream cpuinfo("/proc/cpuinfo");
	std::string line;

	while (std::getline(cpuinfo, line)) {
		if (line.find("model name") != std::string::npos) {
			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				cpuModel = line.substr(colonPos + 2); // 跳过冒号和空格
				break;
			}
		}
	}

#elif __APPLE__
	// macOS平台：使用sysctl获取CPU型号
	std::array<char, 256> buffer;
	size_t length = buffer.size();

	if (sysctlbyname("machdep.cpu.brand_string", buffer.data(), &length, nullptr, 0) == 0) {
		cpuModel = std::string(buffer.data());
	}
	else {
		cpuModel = "Unknown";
	}

#else
	// 其他平台：返回未知
	cpuModel = "Unknown";
#endif

	return cpuModel;
}

std::pair<std::string, std::string> getFirstLocalIPAddress() {
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		throw std::system_error(WSAGetLastError(), std::system_category(), "WSAStartup failed");
	}
	auto cleanup = []() { WSACleanup(); };
#else
	auto cleanup = []() {};
#endif

	try {
		// 获取主机名
		char hostName[256] = { 0 };
		if (gethostname(hostName, sizeof(hostName)) != 0) {
			throw std::system_error(
#ifdef _WIN32
				WSAGetLastError(),
#else
				errno,
#endif
				std::system_category(), "gethostname failed");
		}

		// 获取IP地址（改进版）
		std::string ipStr;

#ifdef _WIN32
		// Windows实现
		struct addrinfo hints = { 0 }, *result = nullptr;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		if (getaddrinfo(hostName, nullptr, &hints, &result) != 0) {
			throw std::system_error(WSAGetLastError(), std::system_category(), "getaddrinfo failed");
		}

		for (auto p = result; p != nullptr; p = p->ai_next) {
			if (p->ai_family == AF_INET) {
				char ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &((struct sockaddr_in*)p->ai_addr)->sin_addr, ip, sizeof(ip));
				if (strcmp(ip, "127.0.0.1") != 0) {  // 排除回环地址
					ipStr = ip;
					break;
				}
			}
		}
		freeaddrinfo(result);
#else
		// Linux/Unix实现（更准确的方式）
		struct ifaddrs *ifaddr, *ifa;
		if (getifaddrs(&ifaddr) == -1) {
			throw std::system_error(errno, std::system_category(), "getifaddrs failed");
		}

		for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) {
				continue;
			}

			auto addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
			char ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, addr, ip, sizeof(ip));

			if (strcmp(ip, "127.0.0.1") != 0 && strncmp(ip, "169.254", 7) != 0) {  // 排除回环和APIPA地址
				ipStr = ip;
				break;
			}
		}
		freeifaddrs(ifaddr);
#endif

		if (ipStr.empty()) {
			throw std::runtime_error("No valid IPv4 address found");
		}

		cleanup();
		return { hostName, ipStr };
	}
	catch (...) {
		cleanup();
		throw;  // 重新抛出异常
	}
}