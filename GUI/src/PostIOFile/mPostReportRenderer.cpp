#include "mPostReportRenderer.h"

#include <QFile>
#include <QSharedPointer>

namespace PostData
{
    mPostReportRendererFactory* mPostReportRendererFactory::instance()
    {
        static mPostReportRendererFactory factory;
        return &factory;
    }

    void mPostReportRendererFactory::registerRenderer(QSharedPointer<mPostReportRenderer> renderer)
    {
        if (renderer.isNull())
            return;

        const QString name = renderer->name();
        for (const auto& r : _renderers)
        {
            if (!r.isNull() && r->name() == name)
                return;
        }
        _renderers.append(renderer);
    }

    QStringList mPostReportRendererFactory::availableNames() const
    {
        QStringList names;
        for (const auto& r : _renderers)
        {
            if (!r.isNull())
                names.append(r->name());
        }
        return names;
    }

    QString mPostReportRendererFactory::fileFilterForName(const QString& name) const
    {
        for (const auto& r : _renderers)
        {
            if (!r.isNull() && r->name() == name)
                return r->fileFilter();
        }
        return QString();
    }

    QSharedPointer<mPostReportRenderer> mPostReportRendererFactory::createRenderer(const QString& name) const
    {
        for (const auto& r : _renderers)
        {
            if (!r.isNull() && r->name() == name)
                return r;
        }
        return QSharedPointer<mPostReportRenderer>();
    }

    bool releaseQrcResource(const QString& qrcPath,
                            const QString& targetPath,
                            QString& error)
    {
        QFile src(qrcPath);
        if (!src.open(QIODevice::ReadOnly))
        {
            error = QString("qrc resource not found: %1").arg(qrcPath);
            return false;
        }
        const QByteArray fresh = src.readAll();

        //已有副本内容一致：复用（避免每导出一次就重写一遍临时文件）
        QFile existing(targetPath);
        if (existing.exists())
        {
            //先读完即关（QFile 不以 FILE_SHARE_DELETE 打开，握着句柄会让
            //后面的 setPermissions/remove 失败）
            const bool same = existing.open(QIODevice::ReadOnly)
                && existing.readAll() == fresh;
            existing.close();
            if (same)
                return true;
            //陈旧副本（旧版程序释放）：覆盖，保证执行的是随本次构建发布的内容。
            //旧副本常带只读属性（本机实测 R），直接 remove 会失败——先恢复写权限
            existing.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                | QFile::ReadUser | QFile::WriteUser);
            if (!QFile::remove(targetPath))
            {
                error = QString("stale copy remove failed: %1 (文件可能正被占用)")
                            .arg(targetPath);
                return false;
            }
        }

        QFile dst(targetPath);
        if (!dst.open(QIODevice::WriteOnly) || dst.write(fresh) != fresh.size())
        {
            error = QString("qrc resource release failed: %1").arg(targetPath);
            return false;
        }
        return true;
    }
}
