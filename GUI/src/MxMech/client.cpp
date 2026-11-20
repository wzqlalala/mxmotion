#define NOMINMAX
#include "client.h"
#include <iostream>
#include <cerrno>
#include <limits.h>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <iomanip>
#include <fstream>
#include <thread>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <fstream>
#include <iostream>
#include <QCoreApplication>
#include <QDir>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QString>
#include <iostream>
#include <string>
#include "ip_utils.h"

#include <string>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QtCore>
#include <QMetaObject>
#include <QApplication>
#include <QObject>
#pragma execution_character_set("utf-8")

// 线程安全的 UUID 生成函数
std::string generate_uuid() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<uint32_t> dis(0, 0xffffffff);

	// 生成随机数
	uint32_t a = dis(gen);
	uint32_t b = dis(gen);
	uint32_t c = dis(gen);
	uint32_t d = dis(gen);

	// 设置版本号和变体
	b = (b & 0x0fff) | 0x4000; // 设置版本号 4
	d = (d & 0x3fffff) | 0x8000; // 设置变体 1

	// 转换为字符串
	std::ostringstream oss;
	oss << std::hex << std::setw(8) << std::setfill('0') << a << '-'
		<< std::setw(4) << std::setfill('0') << (b >> 16) << '-'
		<< std::setw(4) << std::setfill('0') << (b & 0xffff) << '-'
		<< std::setw(4) << std::setfill('0') << (c >> 16) << '-'
		<< std::setw(8) << std::setfill('0') << c;

	return oss.str();
}

std::string token = generate_uuid();


Client::Client(const std::string& user_name, const std::string& user_ip,  int user_pool)
	: user_name_(user_name),user_ip_(user_ip), user_pool_(user_pool)
{

#ifdef _WIN32
	socket_ = INVALID_SOCKET;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		throw std::runtime_error("WSAStartup failed");
	}
#else
	socket_ = -1;
#endif
}

\

Client::~Client() {
	if (
#ifdef _WIN32
		socket_ != INVALID_SOCKET
#else
		socket_ != -1
#endif
		) {
#ifdef _WIN32
		closesocket(socket_);
		WSACleanup();
#else
		close(socket_);
#endif
	}
}

bool Client::connectToServer(const std::string& host, int port) {
#ifdef _WIN32
	socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_ == INVALID_SOCKET) {
		std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
		return false;
	}
#else
	socket_ = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_ == -1) {
		std::cerr << "Socket creation failed" << std::endl;
		return false;
	}
#endif

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
		std::cerr << "Invalid address" << std::endl;
		return false;
	}

	if (connect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
#ifdef _WIN32
		closesocket(socket_);
		socket_ = INVALID_SOCKET;
#else
		close(socket_);
		socket_ = -1;
#endif
		return false;
	}
	return true;
}

std::pair<bool, std::string> Client::authenticate() {
	QString exeFilePath = QCoreApplication::applicationFilePath();
	QString exeDirPath = QFileInfo(exeFilePath).absolutePath();

	// 构造文件路径（确保使用路径分隔符）
	QString filePath = exeDirPath + QDir::separator() + "uuid.txt";
	std::string stdFilePath = filePath.toStdString();
	std::ofstream file(stdFilePath, std::ios::trunc); // trun模式覆盖写入
	if (file.is_open()) {
		file << token;
			file.close();
	}
	else {
		throw std::runtime_error("无法写入token文件: " + stdFilePath);
	}
	std::ostringstream oss;
	oss << token << " " << user_pool_ <<  " " << user_ip_ << " " << user_name_ << " " <<  "R" << " " << "MxSim.Mechanical.GUI";
	std::string authData = oss.str();

	auto connect_time = std::chrono::system_clock::now();

	// 发送认证数据
	if (send(socket_, authData.c_str(), authData.size(), 0) <= 0) {
		return std::make_pair(false, "发送认证数据失败");
	}

	// 接收响应
	char buffer[1024] = { 0 };
	int bytesReceived = recv(socket_, buffer, sizeof(buffer) - 1, 0);
	if (bytesReceived <= 0) {
		return std::make_pair(false, "接收响应失败");
	}

	// 解析响应
	std::istringstream iss(buffer);


	bool authSuccess;
	std::string message;
	std::string product_name;
	int resource;
	int time;

	iss >> authSuccess >> message >> resource >> time >> product_name;

	if (authSuccess) {
		// 认证成功，保持连接
		startHeartbeat();
		QString message = QString("\n服务端许可证剩余时间： %1 天\n服务端剩余资源数量: %2 个")
			.arg(time)
			.arg(resource);

		// 弹出提示框
		MechMainWindow::mFramelessMessageBox::information(new QWidget(), "提示", message);

	}
	else {
		MechMainWindow::mFramelessMessageBox::information(new QWidget(), "提示", " 此产品为：MxSim.Dyna.GUI");
		MechMainWindow::mFramelessMessageBox::information(new QWidget(),
			QString("提示"),
			QString("浮动许可服务端对应的产品为： %1").arg(QString::fromStdString(product_name)));
	}
	return std::make_pair(authSuccess, message);
}

bool Client::connectAndAuthenticate() {
	QString exeFilePath = QCoreApplication::applicationFilePath();
	std::string exeFilepath = QFileInfo(exeFilePath).absolutePath().toStdString();
	QDir dir(QString::fromStdString(exeFilepath));
	dir.cdUp();  // 进入上一级目录

	// 构造 Server_ip 的完整路径（std::string）
	std::string floatpath = dir.absoluteFilePath("Server_ip").toStdString();
	// 检查目录是否存在，如果不存在则创建
	QDir serverIpDir(floatpath.c_str());
	if (!serverIpDir.exists()) {
		if (!serverIpDir.mkpath(".")) {
			std::cerr << "无法创建目录：" << floatpath << std::endl;
			return false;
		}
	}

	// 构造文件路径
	std::string floatpath01 = floatpath + "/server_info.txt";
	QString absolutePath = QDir(exeFilePath).absoluteFilePath(QString::fromStdString(floatpath));
	// 检查文件是否存在
	if (!QFile::exists(floatpath01.c_str())) {
        printf("12345\n");
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), "警告", "许可验证存在异常");
		//MechMainWindow::mFramelessMessageBox::information(nullptr, "提示",
		//	QString("请将 server_info.txt 文件放入以下路径:\n%1").arg(absolutePath));
		exit(0);
	}

	// 2. 读取文件内容
	std::ifstream file(floatpath01);
	if (!file.is_open()) {
		exit(0);
	}

	std::string line;
	if (!std::getline(file, line)) {
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), "警告", "文件为空或读取失败");
		exit(0);
	}

	file.close();

	// 3. 解析 IP 和端口（格式：IP;端口）
	size_t delimiter_pos = line.find(':');
	if (delimiter_pos == std::string::npos) {
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), "警告", "文件格式不正确");
		exit(0);
	}

	std::string server_ip = line.substr(0, delimiter_pos);
	std::string port_str = line.substr(delimiter_pos + 1);

	// 4. 验证 IP 和端口
	if (server_ip.empty() || port_str.empty()) {
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), "警告", "IP 或端口不能为空");
		exit(0);
	}
	int port = std::stoi(port_str);
	
	if (!this->connectToServer(server_ip, port)) {
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), "警告", "服务端连接失败");
		exit(0);
	}

	auto authResult = this->authenticate();
	if (authResult.first) {
		if (this->requestResources(1)) {
			return true;
		}
		else {
			exit(0);
		}
	}
	else {
		exit(0);
	}

}

std::unique_ptr<Client> initializeClient() {
	auto ip = getFirstLocalIPAddress();  // 假设返回 std::pair<std::string, std::string>
	return std::unique_ptr<Client>(new Client(ip.first, ip.second, 0));  // C++11 的 make_unique
}


//Client client = initializeClient();
//client.connectAndAuthenticate();

bool Client::requestResources(int requested_resources) {
	std::ostringstream oss;
	oss << token << " " << "1" << " " << user_ip_ << " " << user_name_ << " " << "REQUEST" << " " << "MxSim.Mechanical.GUI";
	std::string authData = oss.str();

	// 发送资源请求
	if (send(socket_, authData.c_str(), authData.size(), 0) <= 0) {
		std::cerr << "发送申请资源请求失败" << std::endl;
		return false;
	}

	// 接收响应
	char buffer[1024] = { 0 };
	int bytesReceived = recv(socket_, buffer, sizeof(buffer) - 1, 0);

	if (bytesReceived <= 0) {
		std::cerr << "未能收到申请资源的响应" << std::endl;
		return false;
	}

	// 解析响应
	std::istringstream iss(buffer);
	bool auth_success;
	std::string message;
	bool days_left;
	iss >> auth_success >> message >> days_left;

	if (auth_success) {
		return true;
	}
	else {
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), "警告", "资源申请失败!");
		return false;
	}

}

void Client::startHeartbeat() {
	std::thread([this]() {
		int retryCount = 0;
		const int maxRetry = 3;  // 最大重试次数
		const std::chrono::seconds interval = std::chrono::seconds(1);  // 10秒一次心跳

		while (keepRunning_.load()) {
			char heartbeat = 0;
			ssize_t sendResult = send(socket_, &heartbeat, 1, 0);

			if (sendResult <= 0) {
				// 判断错误类型（区分临时错误和致命错误）
				bool isTemporaryError = false;
#ifdef _WIN32
				int error = WSAGetLastError();
				// Windows：WSAEWOULDBLOCK表示暂时无法发送，属于正常现象
				if (error == WSAEWOULDBLOCK || error == WSAETIMEDOUT) {
					isTemporaryError = true;
				}
#else
				// Linux：EAGAIN/EWOULDBLOCK表示暂时无法发送
				if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT) {
					isTemporaryError = true;
				}
#endif

				if (isTemporaryError) {
					// 临时错误，不计数，继续重试
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
				}

				// 致命错误，累计重试次数
				retryCount++;
				if (retryCount >= maxRetry) {
					// 多次失败后，判定为连接断开
					QMetaObject::invokeMethod(qApp, [this]() {
						QMessageBox::StandardButton reply;
						reply = MechMainWindow::mFramelessMessageBox::question(new QWidget(), "警告", "连接已断开，可能是服务端关闭或网络不稳定导致。\n是否尝试重新连接？！",
							QMessageBox::Retry | QMessageBox::Cancel);
						if (reply == QMessageBox::Retry) {
							// 用户选择重新连接
							this->connectAndAuthenticate();
						}
						else if (reply == QMessageBox::Cancel) {
							// 用户选择取消，直接关闭
							this->cleanupAndExit();
						}
					}, Qt::QueuedConnection);
					break;
				}
			}
			else {
				// 发送成功，重置重试计数
				retryCount = 0;
			}

			std::this_thread::sleep_for(interval);
		}
	}).detach();
}

void Client::cleanupAndExit() {
#ifdef _WIN32
	closesocket(socket_);
	WSACleanup();
	socket_ = INVALID_SOCKET;
#else
	close(socket_);
	socket_ = -1;
#endif

	// 安全退出（GUI程序应调用QApplication::exit）
	QCoreApplication::exit(-1);
}

bool Client::releaseResources(int released_resources) {
	std::ostringstream oss;
	oss << token << " " << "1" << " " << user_ip_ << " " << user_name_ << " " << "RELEASE" << " " << "MxSim.Mechanical.GUI"; // 发送负数表示释放资源
	std::string authData = oss.str();

	// 发送释放资源请求
	if (send(socket_, authData.c_str(), authData.size(), 0) <= 0) {
		MechMainWindow::mFramelessMessageBox::information(new QWidget(), "警告", "发送释放资源请求失败!");
		return false;
	}

	// 接收响应
	char buffer[1024] = { 0 };
	int bytesReceived = recv(socket_, buffer, sizeof(buffer) - 1, 0);
	if (bytesReceived <= 0) {
		MechMainWindow::mFramelessMessageBox::information(new QWidget(), "警告", "未能收到释放资源的响应!");
		return false;
	}

	// 解析响应
	std::istringstream iss(buffer);
	bool auth_success;
	std::string message;
	int days_left;
	if (!(iss >> auth_success >> message >> days_left)) {
		MechMainWindow::mFramelessMessageBox::information(new QWidget(), "警告", "收到的响应格式无效!");
		return false;
	}

	if (auth_success) {
		MechMainWindow::mFramelessMessageBox::information(new QWidget(),
			QString("提示"),
			QString("资源释放成功!\n用户名: %1\n释放资源: %2")
			.arg(QString::fromStdString(user_name_))
			.arg(released_resources));
		return true;
	}
	else {
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), "警告", "资源释放失败!");
		return false;
	}
}

std::string Client::getExePath() {
#if defined(_WIN32) || defined(_WIN64)
	// Windows
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
#elif defined(__linux__) || defined(__unix__)
	// Linux/Unix
//#include <limits.h>      // 标准 POSIX 定义
// 或者
//#include <linux/limits.h> // Linux 专用定义
	std::array<char, PATH_MAX> buffer;
	ssize_t len = ::readlink("/proc/self/exe", buffer.data(), buffer.size());
	if (len != -1) {
		std::string::size_type pos = std::string(buffer.data()).find_last_of("\\/");
		return std::string(buffer.data()).substr(0, pos);
	}
	else {
		throw std::runtime_error("Failed to get executable path");
	}
#elif defined(__APPLE__) || defined(__MACH__)
	// macOS
	char buffer[PATH_MAX];
	uint32_t size = sizeof(buffer);
	if (_NSGetExecutablePath(buffer, &size) == 0) {
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");
		return std::string(buffer).substr(0, pos);
	}
	else {
		throw std::runtime_error("Failed to get executable path");
	}
#else
	throw std::runtime_error("Unsupported platform");
#endif
}