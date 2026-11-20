#include "mPostWordReportRenderer.h"
#include "mPostHtmlReportRenderer.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QCoreApplication>
#include "mCommonFunctions.h"   //resolveBundledPythonExe：python 可执行文件回退探测
#include "mGlobalSignals.h"     //outputMessageSig：自定义模板回退提示

namespace
{
    //--- JSON 键契约与 mPostPptReportRenderer 的同名私有方法逐字段一致（就地
    //复制实现，避免为复用而改动 PPT 渲染器头文件；两处需同步维护） ---
    QJsonObject reportMetaToJson(const PostData::ReportMeta& meta)
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

    QJsonObject reportTableToJson(const PostData::ReportTable& table)
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

    QJsonObject reportSectionItemToJson(const PostData::ReportSectionItem& item)
    {
        QJsonObject obj;
        obj.insert("type", static_cast<int>(item.type));
        switch (item.type)
        {
        case PostData::ReportSectionItem::Text:
            obj.insert("content", item.text.content);
            obj.insert("isBold", item.text.isBold);
            break;
        case PostData::ReportSectionItem::Image:
            obj.insert("imagePath", item.image.imagePath);
            obj.insert("caption", item.image.caption);
            obj.insert("embedBase64", item.image.embedBase64);
            break;
        case PostData::ReportSectionItem::Table:
            obj.insert("table", reportTableToJson(item.table));
            break;
        default:
            break;
        }
        return obj;
    }

    QJsonArray reportSectionsToJson(const QList<PostData::ReportSection>& sections)
    {
        QJsonArray arr;
        for (const auto& section : sections)
        {
            QJsonObject obj;
            obj.insert("id", section.id);
            obj.insert("title", section.title);
            //节级布局网格（R07 报告导出跟随页面布局；0=自动按 3 列）。本转换器
            //仅模板分支 saveContextToJson 调用，老链路不写 JSON 不受影响
            obj.insert("gridCols", section.gridCols);
            obj.insert("gridRows", section.gridRows);
            QJsonArray items;
            for (const auto& item : section.items)
            {
                items.append(reportSectionItemToJson(item));
            }
            obj.insert("items", items);
            arr.append(obj);
        }
        return arr;
    }
}

namespace PostData
{
    //模板化导出：从 qrc 释放默认报告母版到临时目录（python 需要真实磁盘文件）。
    //经内容校验释放：已存在副本若来自旧版程序（陈旧母版）会被覆盖，
    //同 PPT 渲染器（20261030-B01）
    static bool releaseDefaultDocxTemplate(const QString& tempDir, QString& outPath, QString& error)
    {
        const QString tplTmp = tempDir + QStringLiteral("/报告母版.docx");
        if (!PostData::releaseQrcResource(
                QStringLiteral(":/mMainWidgets/templates/报告母版.docx"), tplTmp, error))
            return false;
        outPath = tplTmp;
        return true;
    }

    bool mPostWordReportRenderer::render(const mPostReportContext& context,
                                          const QString& outputPath,
                                          QString& error)
    {
        //老分支（3D 报告，templateMode=false）：HTML 内容冒充 .docx，一行不动
        if (!context.templateMode)
        {
            QString html;
            if (!generateHtml(context, html, error))
                return false;

            QFileInfo outInfo(outputPath);
            QDir outDir = outInfo.dir();
            if (!outDir.exists())
            {
                if (!outDir.mkpath("."))
                {
                    error = QString("failed to create output directory: %1").arg(outDir.absolutePath());
                    return false;
                }
            }

            // 兼容模式：把图片 Base64 内嵌的 HTML 内容直接写入 .docx
            QString targetPath = outputPath;
            if (QFileInfo(targetPath).suffix().isEmpty())
                targetPath.append(".docx");

            QFile file(targetPath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                error = QString("failed to open file: %1").arg(targetPath);
                return false;
            }
            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            stream << html;
            file.close();

            return true;
        }

        //--- 模板分支（曲线报告，templateMode=true）：JSON + export_docx.py 生成
        //真 OOXML .docx（机制与 PPT 渲染器同款：python 回退探测 + qrc 释放） ---
        QString tempDir = QDir::temp().absoluteFilePath("mx_word_export_tmp");
        QDir(tempDir).mkpath(".");

        // 导出前清理同名旧文件，避免旧文件残留导致误判导出成功
        if (QFile::exists(outputPath) && !QFile::remove(outputPath))
        {
            error = QString("failed to remove existing file: %1 (文件可能正被打开)").arg(outputPath);
            return false;
        }

        QString jsonPath = QDir(tempDir).absoluteFilePath("report_ctx.json");
        if (!saveContextToJson(context, jsonPath, error))
            return false;

        QStringList probedPythons;
        QString pythonExe = PostCommon::mCommonFunctions::resolveBundledPythonExe(&probedPythons);
        if (pythonExe.isEmpty())
        {
            error = QString("no usable python executable found (probed: %1)")
                        .arg(probedPythons.join("; "));
            return false;
        }

        QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts/export_docx.py";

        //磁盘脚本须与本次构建 qrc 内容一致才直接使用；否则按内容校验释放到
        //临时目录后使用（同 PPT 渲染器，20261030-B01：陈旧副本会误走旧版式）
        {
            QFile diskScript(scriptPath);
            QFile qrcScript(QStringLiteral(":/mMainWidgets/scripts/export_docx.py"));
            const bool freshOnDisk = diskScript.open(QIODevice::ReadOnly)
                && qrcScript.open(QIODevice::ReadOnly)
                && diskScript.readAll() == qrcScript.readAll();
            if (!freshOnDisk)
            {
                scriptPath = tempDir + "/export_docx.py";
                if (!PostData::releaseQrcResource(
                        QStringLiteral(":/mMainWidgets/scripts/export_docx.py"),
                        scriptPath, error))
                    return false;
            }
        }

        //模板解析：自定义模板先校验真实可读（QIODevice）且 .docx 后缀；
        //不合约定不判失败——直接按默认模板导出并提示（同 PPT 渲染器约定）
        QString templateFile;
        bool usingCustom = false;
        bool customInvalid = false;
        if (!context.templatePath.isEmpty())
        {
            const QFileInfo tplInfo(context.templatePath);
            QFile tplProbe(context.templatePath);
            if (tplInfo.suffix().compare(QLatin1String("docx"), Qt::CaseInsensitive) == 0
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
        if (templateFile.isEmpty() && !releaseDefaultDocxTemplate(tempDir, templateFile, error))
            return false;

        //templateKind=脚本 CLI 第 4 参数（模板类别旗标，20261105-R17）：类别不能
        //走 JSON——JSON 只写一次、自定义失败回退默认模板重导时不重写，旗标必须
        //随每次调用区分首导（custom/default）与回退重导（恒 default）
        //masterFile=脚本 CLI 第 5 参数（合并源母版路径，20261105-R18）：仅
        //kind=custom 时传（脚本必需），default 不传/传空串均可（脚本忽略）
        auto runExport = [pythonExe, scriptPath, outputPath]
                         (const QString& jsonFile, const QString& tplFile,
                          const QString& templateKind, const QString& masterFile,
                          QString& err) -> bool {
            QStringList args;
            args << scriptPath << jsonFile << outputPath << tplFile << templateKind;
            if (!masterFile.isEmpty())
                args << masterFile;
            QProcess process;
            process.start(pythonExe, args);
            if (!process.waitForFinished(-1))
            {
                err = QString("docx export process failed: %1").arg(process.errorString());
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
                err = "docx file was not generated";
                return false;
            }
            return true;
        };

        //自定义首导前置：释放内置报告母版到临时目录（内容校验,幂等）作为合并
        //源（20261105-R18:合并产物=用户模板内容在前+母版形式追加在后,页眉页脚
        //整篇用用户的——母版仅移植骨架/样式/编号/媒体,其页眉页脚部件零引入）
        QString masterTemplate;
        if (usingCustom && !releaseDefaultDocxTemplate(tempDir, masterTemplate, error))
            return false;

        QString firstError;
        if (runExport(jsonPath, templateFile,
                      usingCustom ? QStringLiteral("custom") : QStringLiteral("default"),
                      masterTemplate, firstError))
        {
            if (customInvalid)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
                    QStringLiteral("自定义模板不可用，已按默认模板导出"), true);
            }
            return true;
        }
        if (!usingCustom)
        {
            //默认模板导出失败：报错（脚本问题随 stderr 透出）
            error = firstError;
            return false;
        }

        //自定义模板导出失败：回退默认模板重导一次，仍失败才报错（旗标=default,
        //不传母版——合并分支不参与回退语义）
        QString defaultTemplate;
        if (!releaseDefaultDocxTemplate(tempDir, defaultTemplate, error))
            return false;
        if (QFile::exists(outputPath))
            QFile::remove(outputPath);   //清掉失败尝试的半成品（若有）
        QString fallbackError;
        if (!runExport(jsonPath, defaultTemplate, QStringLiteral("default"),
                       QString(), fallbackError))
        {
            error = fallbackError;
            return false;
        }
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
            QStringLiteral("自定义模板不可用，已按默认模板导出"), true);
        return true;
    }

    bool mPostWordReportRenderer::generateHtml(const mPostReportContext& context, QString& html, QString& error)
    {
        // 图片以 Base64 内嵌，单个 .docx 文件自包含，无外置图片依赖
        mPostHtmlReportRenderer htmlRenderer;
        htmlRenderer.setImageEmbedMode(true);

        // 临时文件仅用于接收 HTML 内容，随后读回字符串
        QString tempDir = QDir::temp().absoluteFilePath("mx_word_export_tmp");
        QDir(tempDir).mkpath(".");
        QString tempHtmlPath = QDir(tempDir).absoluteFilePath("temp_report.html");
        if (!htmlRenderer.render(context, tempHtmlPath, error))
            return false;

        QFile file(tempHtmlPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            error = QString("failed to read temporary html: %1").arg(tempHtmlPath);
            return false;
        }
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        html = stream.readAll();
        file.close();
        QFile::remove(tempHtmlPath);
        return true;
    }

    bool mPostWordReportRenderer::saveContextToJson(const mPostReportContext& context,
                                                     const QString& jsonPath,
                                                     QString& error)
    {
        //键契约与 mPostPptReportRenderer::saveContextToJson 一致（就地复制实现，
        //避免为复用而改动 PPT 渲染器头文件；两处需同步维护）
        QJsonObject root;
        root.insert("meta", reportMetaToJson(context.meta));
        root.insert("sections", reportSectionsToJson(context.sections));
        root.insert("resourceImages", QJsonArray::fromStringList(context.resourceImages));
        root.insert("animations", QJsonArray::fromStringList(context.animationPaths));
        root.insert("windowPerSlide", context.windowPerSlide);
        root.insert("templateMode", context.templateMode);
        root.insert("templatePath", context.templatePath);
        root.insert("imagePerChart", context.imagePerChart);

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
}
