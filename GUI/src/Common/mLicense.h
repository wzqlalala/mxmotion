#pragma once
#if !defined(AFX_14BEC153_17B9_47BE_845F_71A27BF26B59__INCLUDED_)
#define AFX_14BEC153_17B9_47BE_845F_71A27BF26B59__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include<vector>
#include <QObject>
#include <string>
#include <time.h>
#include <fstream>
#include <iostream>
#include <math.h>
#if defined _WIN32|| defined _WIN64||defined WIN32
//#include <winSock2.h>
#pragma comment(lib , "ws2_32.lib")
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include<unistd.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<net/if_arp.h>
#include<netinet/ip.h>
#include<string.h>
#endif
#include "mCommon_global.h"
#include"mDES.h"
#ifndef NDEBUG
#define NDEBUG
#endif // !NDEBUG
using namespace std;

namespace Common
{
    class COMMON_EXPORT mLicense : public QObject
    {
        Q_OBJECT

    public:
        mLicense(QString workdir);
        ~mLicense();

    public:
        //开始效验许可证
        int checkLicense();
        //获取许可证剩余天数
        int getRemainDay();
        //获取mac地址
        bool getMacByCmd(vector<string>& mac);
    private:
        string  licenseFile;   //激活码文件位置
        string  configureFile;	//生成的本机信息文件的位置

        int remainDay = -1;//  免注册默认无天数

        //一年的天数
        int year_alldays(int year);
        //一个月的天数
        int month_days(int month1);
        int days_gap(int year1, int month1, int day1, int year2, int month2, int day2);

        //检查激活码文件是否存在
        bool check_localfile();
        //检验激活码的有效性
        int check_code();
        //检验激活码是否过期
        bool time_check(string lincese, int usetime, int year, int month, int day);

        //写配置信息(本机主机名、网卡mac地址);
        bool writeConfigureInfor();
        //读配置信息
        bool readConfigureInfor(vector<string>& sysinfor);

    };

    class Copyright
    {
    public:
        /*string localFile;
        string systemFile;*/
        bool WriteFile1(const char* file, const LPBYTE buffer, long size);
        bool ReadFile(const char* file, LPBYTE buffer, long size);
        long GetFileSize(const char* file);
    };
}
#endif // !defined(AFX_14BEC153_17B9_47BE_845F_71A27BF26B59__INCLUDED_)
