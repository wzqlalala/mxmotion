#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <ctime>
#include <atomic>
#include <utility>
#include <thread>
#include <QObject>
// Windows 平台需要包含 Winsock2 头文件
#ifdef _WIN32
typedef intptr_t ssize_t;
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // 自动链接 Winsock 库
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class Client {
public:
	Client(const std::string& username, const std::string& user_ip, int user_pool = 0);
	~Client();  // 添加析构函数

	bool connectToServer(const std::string& host, int port);
	bool requestResources(int requested_resources);
	bool releaseResources(int released_resources);
	std::pair<bool, std::string> authenticate();
	void startHeartbeat();
	std::string getExePath();
	bool connectAndAuthenticate();

private:
	void cleanupAndExit();      // 清理资源并退出
	std::atomic<bool> keepRunning_{ true };
	std::string user_ip_;
	std::string user_name_;
	int user_pool_;
	std::thread heartbeat_thread_;
	std::atomic<bool> keep_running_{ false };
#ifdef _WIN32
	SOCKET socket_;
	WSADATA wsa_data_;
#else
	int socket_;
#endif
};


#endif // CLIENT_H