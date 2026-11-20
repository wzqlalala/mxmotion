#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QStringList>

namespace MGUIMesh
{
    /**
     * @brief 命令行批量剖分配置参数
     */
    struct BatchCLIConfig
    {
        QString batchConfigPath;    // --batch 批量任务配置文件路径
        QString reportPath;         // --report 结果报告输出路径
        QString logPath;            // --log 日志文件路径
        int concurrency = -1;       // --concurrency 覆盖并发度，-1 表示使用配置文件
        int timeoutMs = 0;          // --timeout 总超时时间（毫秒），0 表示不超时
        bool headless = false;      // --headless 是否不创建主窗口
        bool batchMode = false;     // 是否进入批量模式

        bool isValid() const
        {
            return batchMode && !batchConfigPath.isEmpty();
        }
    };

    /**
     * @brief 命令行退出码定义
     */
    enum class BatchCLIExitCode
    {
        Success = 0,                // 全部成功
        InvalidArguments = 1,       // 命令行参数错误
        ConfigParseFailed = 2,      // 配置文件解析失败
        ModelLoadFailed = 3,        // 模型加载失败
        TaskFailed = 4,             // 批量剖分存在失败任务
        Timeout = 5,                // 超时
        SaveExportFailed = 6,       // 保存/导出失败
        UnknownException = 7        // 未知异常
    };
}
