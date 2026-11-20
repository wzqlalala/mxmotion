#pragma once
#include <QObject>
#include <QStringList>

namespace APIServer
{
    class mAPIScriptExecutor;
}

namespace MechMainWindow
{
    class mMechMainWindow;
}

namespace MxMech
{
    class mStartPage;

    class mAppInitializer : public QObject
    {
        Q_OBJECT
    public:
        /*
        * 重载构造函数，通过控制台命令或者修改属性启动传入参数
        */
        mAppInitializer(int argc, char* argv[], int ramainDay);

        // 在 Qt 事件循环启动后显示主窗，避免原生外框早于内容首帧出现。
        void start();

        /*
        * 析构函数
        */
        ~mAppInitializer();

    private:
        /*
        * 读取配置文件
        */
        void readConfigFile();

        /*
        * 初始化命令行 JSON 脚本执行器。
        * 必须在主窗口及业务 API 注册完成后调用。
        */
        void initScriptExecutor(const QStringList& arguments);

        // 首帧绘制后执行同步 Python 与命令行初始化。
        void finishStartup();

    private:
        //启动窗口
        mStartPage* _startPage;

        //主窗口
        MechMainWindow::mMechMainWindow* _mainWindow;

        //失效日期
        int _remainDay;

        //main函数入口参数
        QStringList _fileNames;

        //命令行脚本执行器，由主窗口负责 QObject 生命周期管理
        APIServer::mAPIScriptExecutor* _scriptExecutor = nullptr;

        //预留的几何数据线程状态，切换回异步策略时用于判断业务是否完成
        bool _modelBusy = false;
    };
}
