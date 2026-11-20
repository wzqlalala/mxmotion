#include "mPostPptReportRenderer.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QCoreApplication>
#include "mCommonFunctions.h"   //resolveBundledPythonExe：python 可执行文件回退探测
#include "mGlobalSignals.h"     //outputMessageSig：自定义模板回退提示

namespace PostData
{
    //模板化导出：从 qrc 释放默认报告母版到临时目录（python 需要真实磁盘文件）。
    //经内容校验释放：已存在副本若来自旧版程序（陈旧母版）会被覆盖，
    //保证默认模板与本次构建发布内容一致
    static bool releaseDefaultPptTemplate(const QString& tempDir, QString& outPath, QString& error)
    {
        const QString tplTmp = tempDir + QStringLiteral("/报告母版.pptx");
        if (!PostData::releaseQrcResource(
                QStringLiteral(":/mMainWidgets/templates/报告母版.pptx"), tplTmp, error))
            return false;
        outPath = tplTmp;
        return true;
    }

    bool mPostPptReportRenderer::render(const mPostReportContext& context,
                                         const QString& outputPath,
                                         QString& error)
    {
        QString tempDir = QDir::temp().absoluteFilePath("mx_ppt_export_tmp");
        QDir(tempDir).mkpath(".");

        QString jsonPath = QDir(tempDir).absoluteFilePath("report_ctx.json");
        if (!saveContextToJson(context, jsonPath, error))
            return false;

        // 生成前先清理已存在的同名文件，避免旧文件残留导致误判导出成功
        if (QFile::exists(outputPath) && !QFile::remove(outputPath))
        {
            error = QString("failed to remove existing file: %1 (文件可能正被打开)").arg(outputPath);
            return false;
        }

        //python 可执行文件解析带回退：Debug 部署的 Conan python 包只有 python_d.exe，
        //依次探测 python.exe/python_d.exe 取第一个存在的——与曲线统计导出 mCurveStatisticsDlg 同一机制
        QStringList probedPythons;
        QString pythonExe = PostCommon::mCommonFunctions::resolveBundledPythonExe(&probedPythons);
        if (pythonExe.isEmpty())
        {
            error = QString("no usable python executable found (probed: %1)")
                        .arg(probedPythons.join("; "));
            return false;
        }

        QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts/export_pptx.py";

        //磁盘脚本须与本次构建 qrc 内容一致才直接使用；否则按内容校验释放到
        //临时目录后使用。老逻辑"存在即复用"曾让跑过旧版程序的机器拿到陈旧
        //脚本，模板导出误走旧版式（每节多出空标题页+页图偏左上，20261030-B01）
        {
            QFile diskScript(scriptPath);
            QFile qrcScript(QStringLiteral(":/mMainWidgets/scripts/export_pptx.py"));
            const bool freshOnDisk = diskScript.open(QIODevice::ReadOnly)
                && qrcScript.open(QIODevice::ReadOnly)
                && diskScript.readAll() == qrcScript.readAll();
            if (!freshOnDisk)
            {
                scriptPath = tempDir + "/export_pptx.py";
                if (!PostData::releaseQrcResource(
                        QStringLiteral(":/mMainWidgets/scripts/export_pptx.py"),
                        scriptPath, error))
                    return false;
            }
        }

        // 每页右上角的 logo 图片，找不到时脚本会自动跳过。
        // 优先从 qrc 资源释放到临时目录（脚本需要真实磁盘文件），经
        // releaseQrcResource 内容校验（陈旧/损坏即替换、幂等），与脚本/母版
        // 释放同一机制；临时释放不可用时再退回到磁盘上的候选路径。
        QString logoPath;
        QString logoTmp = tempDir + "/ppt_logo.png";
        QString logoError;   //logo 可选：释放失败不判导出失败，走磁盘候选/跳过
        if (PostData::releaseQrcResource(
                QStringLiteral(":/mMainWidgets/Icon/ppt_logo.png"), logoTmp, logoError))
        {
            logoPath = logoTmp;
        }
        else
        {
            const QStringList logoCandidates = QStringList()
                << QCoreApplication::applicationDirPath() + "/resource/Icon/ppt_logo.png"
                << QCoreApplication::applicationDirPath() + "/../resource/Icon/ppt_logo.png"
                << QCoreApplication::applicationDirPath() + "/Icon/ppt_logo.png";
            for (const QString& candidate : logoCandidates)
            {
                if (QFile::exists(candidate))
                {
                    logoPath = candidate;
                    break;
                }
            }
        }

        //--- 模板化分支（曲线报告导出，templateMode=true）：解析本次实际使用的
        //模板文件；老链路（templateMode=false）不传模板参数，行为零变化 ---
        QString templateFile;        //本次传给脚本的模板（空=脚本走旧版式）
        bool usingCustom = false;    //本次是否使用自定义模板（失败需回退重导）
        bool customInvalid = false;  //自定义模板校验不过：直接按默认模板导出并提示
        if (context.templateMode)
        {
            if (!context.templatePath.isEmpty())
            {
                //自定义模板先校验：真实可读（QIODevice）且 .pptx 后缀
                const QFileInfo tplInfo(context.templatePath);
                QFile tplProbe(context.templatePath);
                if (tplInfo.suffix().compare(QLatin1String("pptx"), Qt::CaseInsensitive) == 0
                    && tplProbe.open(QIODevice::ReadOnly))
                {
                    templateFile = context.templatePath;
                    usingCustom = true;
                }
                else
                {
                    customInvalid = true;
                }
            }
            if (templateFile.isEmpty() && !releaseDefaultPptTemplate(tempDir, templateFile, error))
                return false;
        }

        //执行导出（lambda 收口便于自定义模板失败后回退默认模板重导一次）
        auto runExport = [pythonExe, scriptPath, logoPath, outputPath]
                         (const QString& jsonFile, const QString& tplFile, QString& err) -> bool {
            QStringList args;
            args << scriptPath << jsonFile << outputPath;
            if (!logoPath.isEmpty())
                args << logoPath;
            if (!tplFile.isEmpty())
                args << tplFile;
            QProcess process;
            process.start(pythonExe, args);
            if (!process.waitForFinished(-1))
            {
                err = QString("python-pptx process failed: %1").arg(process.errorString());
                return false;
            }
            const QString stderrOutput = QString::fromLocal8Bit(process.readAllStandardError());
            if (!stderrOutput.isEmpty())
            {
                err = stderrOutput;
                return false;
            }
            if (!QFile::exists(outputPath))
            {
                err = "pptx file was not generated";
                return false;
            }
            return true;
        };

        QString firstError;
        if (runExport(jsonPath, templateFile, firstError))
        {
            if (customInvalid)
            {
                //自定义模板不可读/后缀不符：已按默认模板导出（提示不判失败）
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("自定义模板不可用，已按默认模板导出"), true);
            }
            return true;
        }
        if (!usingCustom)
        {
            //老链路/默认模板：失败即失败（v1 语义）
            error = firstError;
            return false;
        }

        //自定义模板导出失败（脚本 stderr 非空等）：回退默认模板重导一次，
        //回退仍失败才报错
        QString defaultTemplate;
        if (!releaseDefaultPptTemplate(tempDir, defaultTemplate, error))
            return false;
        if (QFile::exists(outputPath))
            QFile::remove(outputPath);   //清掉失败尝试的半成品（若有）
        QString fallbackError;
        if (!runExport(jsonPath, defaultTemplate, fallbackError))
        {
            error = fallbackError;
            return false;
        }
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
            QStringLiteral("自定义模板不可用，已按默认模板导出"), true);
        return true;
    }

    bool mPostPptReportRenderer::saveContextToJson(const mPostReportContext& context,
                                                    const QString& jsonPath,
                                                    QString& error)
    {
        QJsonObject root;
        root.insert("meta", metaToJson(context.meta));
        root.insert("sections", sectionsToJson(context.sections, context.templateMode));
        root.insert("resourceImages", QJsonArray::fromStringList(context.resourceImages));
        root.insert("animations", QJsonArray::fromStringList(context.animationPaths));
        //版式开关：false 时脚本把各节图片合并为一页网格（"每个窗口导出至单独幻灯片"未勾选）
        root.insert("windowPerSlide", context.windowPerSlide);
        //曲线报告模板化契约键：仅在模板分支写入，3D 老链路的 JSON 与改前逐字节一致
        if (context.templateMode)
        {
            root.insert("templateMode", true);
            root.insert("templatePath", context.templatePath);
            root.insert("imagePerChart", context.imagePerChart);
        }

        QJsonDocument doc(root);
        QFile file(jsonPath);
        if (!file.open(QIODevice::WriteOnly))
        {
            error = QString("failed to write json: %1").arg(jsonPath);
            return false;
        }
        file.write(doc.toJson());
        file.close();
        return true;
    }

    QJsonObject mPostPptReportRenderer::metaToJson(const ReportMeta& meta) const
    {
        QJsonObject obj;
        obj.insert("mainTitle", meta.mainTitle);
        obj.insert("subTitle", meta.subTitle);
        obj.insert("author", meta.author);
        obj.insert("company", meta.company);
        obj.insert("address", meta.address);
        obj.insert("phone", meta.phone);
        obj.insert("dateTime", meta.dateTime);
        obj.insert("mxFileName", meta.mxFileName);
        obj.insert("showSubTitle", meta.showSubTitle);
        obj.insert("showAuthor", meta.showAuthor);
        obj.insert("showCompany", meta.showCompany);
        obj.insert("showAddress", meta.showAddress);
        obj.insert("showPhone", meta.showPhone);
        return obj;
    }

    QJsonArray mPostPptReportRenderer::sectionsToJson(const QList<ReportSection>& sections,
                                                      bool includeLayout) const
    {
        QJsonArray arr;
        for (const auto& section : sections)
        {
            QJsonObject obj;
            obj.insert("id", section.id);
            obj.insert("title", section.title);
            //节级布局网格（R07 报告导出跟随页面布局）：仅模板分支写入——
            //老链路 JSON 与改前逐字节一致（红线）；0=自动（脚本侧按 3 列）
            if (includeLayout)
            {
                obj.insert("gridCols", section.gridCols);
                obj.insert("gridRows", section.gridRows);
            }
            QJsonArray items;
            for (const auto& item : section.items)
            {
                items.append(sectionItemToJson(item));
            }
            obj.insert("items", items);
            arr.append(obj);
        }
        return arr;
    }

    QJsonObject mPostPptReportRenderer::sectionItemToJson(const ReportSectionItem& item) const
    {
        QJsonObject obj;
        obj.insert("type", static_cast<int>(item.type));
        switch (item.type)
        {
        case ReportSectionItem::Text:
            obj.insert("content", item.text.content);
            obj.insert("isBold", item.text.isBold);
            break;
        case ReportSectionItem::Image:
            obj.insert("imagePath", item.image.imagePath);
            obj.insert("caption", item.image.caption);
            obj.insert("embedBase64", item.image.embedBase64);
            break;
        case ReportSectionItem::Table:
            obj.insert("table", tableToJson(item.table));
            break;
        default:
            break;
        }
        return obj;
    }

    QJsonObject mPostPptReportRenderer::tableToJson(const ReportTable& table) const
    {
        QJsonObject obj;
        obj.insert("title", table.title);
        obj.insert("columnCount", table.columnCount);
        QJsonArray rows;
        for (const auto& row : table.rows)
        {
            QJsonArray cells;
            for (const auto& cell : row.cells)
            {
                QJsonObject c;
                c.insert("text", cell.text);
                c.insert("rowSpan", cell.rowSpan);
                c.insert("colSpan", cell.colSpan);
                c.insert("isHeader", cell.isHeader);
                c.insert("hAlign", cell.hAlign);
                cells.append(c);
            }
            rows.append(cells);
        }
        obj.insert("rows", rows);
        return obj;
    }
}
