#include "mCommonLanguage.h"

//Qt
#include <QDir>
#include <QApplication>

#include "mWidgetOperation.h"

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>

#endif

using namespace MxFunctions;
namespace MechCommon
{
	QHash<QString, QString>CommonChinese;
	QHash<QString, QString>CommonEnglish;

	void mCommonLanguage::initLanguageVarible()
	{

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("不存在公钥文件，请联系管理员！", "不存在公钥文件，请联系管理员！");
		CommonEnglish.insert("不存在公钥文件，请联系管理员！", "Public key file does not exist, please contact administrator!");

		CommonChinese.insert("确定", "确定");
		CommonEnglish.insert("确定", "OK");

		CommonChinese.insert("公钥为空，请联系管理员！", "公钥为空，请联系管理员！");
		CommonEnglish.insert("公钥为空，请联系管理员！", "Public key is empty, please contact administrator!");

		CommonChinese.insert("许可证配置文件不存在，请联系管理员！", "许可证配置文件不存在，请联系管理员！");
		CommonEnglish.insert("许可证配置文件不存在，请联系管理员！", "License profile does not exist, please contact administrator!");

		CommonChinese.insert("未填写连接类型，请填写连接类型！", "未填写连接类型，请填写连接类型！");
		CommonEnglish.insert("未填写连接类型，请填写连接类型！", "No connection type, please fill in the connection type!");

		CommonChinese.insert("未填写服务器IP，请填写服务器IP！", "未填写服务器IP，请填写服务器IP！");
		CommonEnglish.insert("未填写服务器IP，请填写服务器IP！", "No server IP, please fill in the server IP!");

		CommonChinese.insert("未填写服务器端口，请填写服务器端口！", "未填写服务器端口，请填写服务器端口！");
		CommonEnglish.insert("未填写服务器端口，请填写服务器端口！", "No server port, please fill in the server port!");

		CommonChinese.insert("客户端创建失败，请联系管理员！", "客户端创建失败，请联系管理员！");
		CommonEnglish.insert("客户端创建失败，请联系管理员！", "Client creation failed, please contact the administrator!");

		CommonChinese.insert("文件client.crt不存在！", "文件client.crt不存在！");
		CommonEnglish.insert("文件client.crt不存在！", "The file client.crt does not exist!");

		CommonChinese.insert("文件client.key不存在！", "文件client.key不存在！");
		CommonEnglish.insert("文件client.key不存在！", "File client.key does not exist!");

		CommonChinese.insert("客户端初始化失败，请联系管理员！", "客户端初始化失败，请联系管理员！");
		CommonEnglish.insert("客户端初始化失败，请联系管理员！", "Client initialization failed, please contact the administrator!");

		CommonChinese.insert("连接服务器失败，请联系管理员！", "连接服务器失败，请联系管理员！");
		CommonEnglish.insert("连接服务器失败，请联系管理员！", "Failed to connect to the server, please contact the administrator!");

		CommonChinese.insert("查询失败，请联系管理员！错误信息:%1", "查询失败，请联系管理员！错误信息:%1");
		CommonEnglish.insert("查询失败，请联系管理员！错误信息:%1", "Query failed, please contact administrator! Error message :%1");

		CommonChinese.insert("检出失败，请联系管理员！错误信息:%1", "检出失败，请联系管理员！错误信息:%1");
		CommonEnglish.insert("检出失败，请联系管理员！错误信息:%1", "Check out failed, please contact administrator! Error message :%1");

		CommonChinese.insert("获取的Token:", "获取的Token:");
		CommonEnglish.insert("获取的Token:", "Acquired Token:");

		CommonChinese.insert("归还失败，错误信息:%1", "归还失败，错误信息:%1");
		CommonEnglish.insert("归还失败，错误信息:%1", "Return failed with error message: %1");

		CommonChinese.insert("更新失败，请联系管理员！错误信息:%1", "更新失败，请联系管理员！错误信息:%1");
		CommonEnglish.insert("更新失败，请联系管理员！错误信息:%1", "Update failed, please contact administrator! Error message :%1");

		CommonChinese.insert("提示信息！", "提示信息！");
		CommonEnglish.insert("提示信息！", "Prompt message!");

		CommonChinese.insert("未成功写入文件，请联系管理员！", "未成功写入文件，请联系管理员！");
		CommonEnglish.insert("未成功写入文件，请联系管理员！", "Failed to write to file, please contact administrator!");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");

		CommonChinese.insert("警告", "警告");
		CommonEnglish.insert("警告", "Warning");
	}


	QString mCommonLanguage::FindLanguageValue(const QString & key)
	{
		return mWidgetOperation::FindLanguageValue(CommonEnglish, CommonChinese, key);
	}

}