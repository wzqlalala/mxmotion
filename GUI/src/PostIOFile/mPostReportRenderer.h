#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mPostReportContext.h"

#include <QString>
#include <QSharedPointer>

namespace PostData
{
    // 报告格式渲染器抽象基类
    class MIOFILE_EXPORT mPostReportRenderer
    {
    public:
        virtual ~mPostReportRenderer() = default;

        // 渲染器名称（HTML / Word / PowerPoint / ...）
        virtual QString name() const = 0;

        // 文件对话框过滤字符串
        virtual QString fileFilter() const = 0;

        // 主渲染入口
        virtual bool render(const mPostReportContext& context,
                            const QString& outputPath,
                            QString& error) = 0;

        // 是否支持内嵌图片（单文件 HTML/PPT）
        virtual bool supportsEmbeddedImages() const { return false; }
    };

    // 渲染器工厂：集中管理所有可用格式，新增格式不改 UI
    class MIOFILE_EXPORT mPostReportRendererFactory
    {
    public:
        static mPostReportRendererFactory* instance();

        // 注册渲染器
        void registerRenderer(QSharedPointer<mPostReportRenderer> renderer);

        // 获取所有可用格式名称
        QStringList availableNames() const;

        // 根据名称获取 fileFilter
        QString fileFilterForName(const QString& name) const;

        // 创建渲染器实例（找不到返回 nullptr）
        QSharedPointer<mPostReportRenderer> createRenderer(const QString& name) const;

    private:
        mPostReportRendererFactory() = default;
        QList<QSharedPointer<mPostReportRenderer>> _renderers;
    };

    // qrc 资源按内容释放到磁盘（python 进程需真实文件）：目标已存在且内容与
    // qrc 一致时复用；不一致（旧版程序留下的陈旧副本）时删除重释放。
    // "存在即复用"会让跑过旧版程序的机器永远用陈旧脚本/母版——模板导出曾因此
    // 误走旧版式（20261030-B01 缺陷1：每视图页多出空标题幻灯片+页图偏左上）
    bool MIOFILE_EXPORT releaseQrcResource(const QString& qrcPath,
                                           const QString& targetPath,
                                           QString& error);
}
