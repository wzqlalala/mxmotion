#include "mPostHtmlReportRenderer.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QBuffer>
#include <QTextStream>
#include <QProcess>
#include <QCoreApplication>

namespace PostData
{
    bool mPostHtmlReportRenderer::render(const mPostReportContext& context,
                                          const QString& outputPath,
                                          QString& error)
    {
        //曲线报告新版式分支（templateMode=true）：对齐 HTML 样张的独立实现，
        //老分支（3D 报告）走下方原逻辑零变化
        if (context.templateMode)
            return renderTemplateReport(context, outputPath, error);

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

        // 图片输出目录（Base64 内嵌模式不需要）
        QString imagesDir;
        if (!_embedImages)
        {
            imagesDir = outDir.absoluteFilePath(outInfo.baseName() + "_images");
            QDir imgDir(imagesDir);
            if (!imgDir.exists())
                imgDir.mkpath(".");
        }

        QString html;
        html.append("<html>\n<head>\n");
        html.append("<meta charset=\"utf-8\">\n");
        html.append(QString("<title>%1</title>\n").arg(escapeHtml(context.meta.mainTitle)));
        html.append("<style>\n");
        html.append("body { font-family: Microsoft YaHei, SimSun, sans-serif; margin: 40px; }\n");
        html.append("h1 { text-align:center; font-size:36px; color:rgb(59,87,125); }\n");
        html.append("h2 { text-align:center; font-size:24px; color:rgb(59,87,125); }\n");
        html.append("h3 { text-align:left; font-size:24px; color:rgb(59,87,125); }\n");
        html.append("table { width:100%; border-collapse:collapse; margin:20px 0; }\n");
        html.append("td, th { border:1px solid rgb(122,153,192); padding:8px; font-size:16px; vertical-align:middle; }\n");
        html.append(".header-cell { background-color:rgb(209,220,234); text-align:right; }\n");
        html.append(".title-cell { background-color:rgb(165,186,213); text-align:center; font-weight:bold; font-size:24px; }\n");
        html.append(".content-cell { text-align:center; }\n");
        html.append("p { font-size:16px; text-indent:2em; }\n");
        html.append(".img-title { text-align:center; font-size:16px; }\n");
        html.append(".img-wrap { text-align:center; margin:20px 0; }\n");
        html.append("</style>\n");
        html.append("</head>\n<body>\n");

        renderMeta(context.meta, html);

        for (const auto& section : context.sections)
        {
            renderSection(section, html, imagesDir);
        }

        for (const QString& animPath : context.animationPaths)
        {
            renderAnimation(animPath, html, outDir, imagesDir);
        }

        html.append("</body>\n</html>");

        QFile file(outputPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            error = QString("failed to open file: %1").arg(outputPath);
            return false;
        }
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << html;
        file.close();

        return true;
    }

    void mPostHtmlReportRenderer::renderMeta(const ReportMeta& meta, QString& html)
    {
        html.append(QString("<h1>%1</h1>\n").arg(escapeHtml(meta.mainTitle)));
        if (meta.showSubTitle && !meta.subTitle.isEmpty())
        {
            html.append(QString("<h2>%1</h2>\n").arg(escapeHtml(meta.subTitle)));
        }

        html.append("<table>\n");
        html.append("<tr><td colspan=\"2\" class=\"title-cell\">用例概述</td></tr>\n");

        auto addRow = [&](const QString& name, const QString& value) {
            html.append("<tr>");
            html.append(QString("<td class=\"header-cell\" width=\"50%\">%1</td>").arg(escapeHtml(name)));
            html.append(QString("<td class=\"content-cell\" width=\"50%\">%1</td>").arg(escapeHtml(value)));
            html.append("</tr>\n");
        };

        addRow("计算文件", meta.mxFileName);
        addRow("测试时间", meta.dateTime);
        if (meta.showAuthor)
            addRow("分析员", meta.author);
        if (meta.showCompany)
            addRow("公司", meta.company);
        if (meta.showAddress)
            addRow("地址", meta.address);
        if (meta.showPhone)
            addRow("电话", meta.phone);
        html.append("</table>\n");
    }

    void mPostHtmlReportRenderer::renderSection(const ReportSection& section, QString& html, const QString& outputDir)
    {
        html.append(QString("<h3>%1</h3>\n").arg(escapeHtml(section.title)));

        for (const auto& item : section.items)
        {
            switch (item.type)
            {
            case ReportSectionItem::Text:
                renderText(item.text, html);
                break;
            case ReportSectionItem::Image:
                renderImage(item.image, html, outputDir);
                break;
            case ReportSectionItem::Table:
                renderTable(item.table, html);
                break;
            default:
                break;
            }
        }
    }

    void mPostHtmlReportRenderer::renderTable(const ReportTable& table, QString& html)
    {
        html.append("<table>\n");
        if (!table.title.isEmpty())
        {
            int col = table.columnCount > 0 ? table.columnCount : 2;
            html.append(QString("<tr><td colspan=\"%1\" class=\"title-cell\">%2</td></tr>\n")
                .arg(col).arg(escapeHtml(table.title)));
        }

        for (const auto& row : table.rows)
        {
            html.append("<tr>\n");
            for (const auto& cell : row.cells)
            {
                QString tag = cell.isHeader ? "th" : "td";
                QString cls = cell.isHeader ? "header-cell" : "content-cell";
                QString align = cell.hAlign;
                QString style = QString(" class=\"%1\" align=\"%2\"").arg(cls).arg(align);
                if (cell.colSpan > 1)
                    style += QString(" colspan=\"%1\"").arg(cell.colSpan);
                if (cell.rowSpan > 1)
                    style += QString(" rowspan=\"%1\"").arg(cell.rowSpan);
                html.append(QString("<%1%2>%3</%4>\n").arg(tag).arg(style).arg(escapeHtml(cell.text)).arg(tag));
            }
            html.append("</tr>\n");
        }
        html.append("</table>\n");
    }

    void mPostHtmlReportRenderer::renderImage(const ReportImage& image, QString& html, const QString& outputDir)
    {
        QFileInfo srcInfo(image.imagePath);
        if (!srcInfo.exists())
        {
            html.append(QString("<p>图片丢失: %1</p>\n").arg(escapeHtml(image.imagePath)));
            return;
        }

        QString src;
        if (_embedImages || image.embedBase64)
        {
            // Base64 内嵌：单文件自包含，Word 兼容导出使用该模式
            QFile imgFile(image.imagePath);
            if (!imgFile.open(QIODevice::ReadOnly))
            {
                html.append(QString("<p>图片读取失败: %1</p>\n").arg(escapeHtml(image.imagePath)));
                return;
            }
            QByteArray base64 = imgFile.readAll().toBase64();
            imgFile.close();
            src = QString("data:image/%1;base64,%2").arg(srcInfo.suffix().toLower()).arg(QString::fromLatin1(base64));
        }
        else
        {
            QString dstPath = QDir(outputDir).absoluteFilePath(srcInfo.fileName());
            QFile::remove(dstPath);
            QFile::copy(image.imagePath, dstPath);

            // 相对路径基于 html 文件所在目录：图片目录名/文件名
            src = QFileInfo(outputDir).fileName() + "/" + srcInfo.fileName();
        }

        html.append("<div class=\"img-wrap\">\n");
        html.append(QString("<img src=\"%1\" width=\"550\" height=\"350\">\n").arg(src));
        if (!image.caption.isEmpty())
        {
            html.append(QString("<p class=\"img-title\">%1</p>\n").arg(escapeHtml(image.caption)));
        }
        html.append("</div>\n");
    }

    void mPostHtmlReportRenderer::renderText(const ReportText& text, QString& html)
    {
        if (text.isBold)
            html.append(QString("<p><b>%1</b></p>\n").arg(escapeHtml(text.content)));
        else
            html.append(QString("<p>%1</p>\n").arg(escapeHtml(text.content)));
    }

    void mPostHtmlReportRenderer::renderAnimation(const QString& animationPath,
                                                  QString& html,
                                                  const QDir& outDir,
                                                  const QString& imagesDir)
    {
        //无动画文件则不追加
        if (animationPath.isEmpty() || !QFileInfo(animationPath).exists())
            return;

        QFileInfo videoInfo(animationPath);
        QString ext = videoInfo.suffix().toLower();

        html.append(QString("<h3>%1</h3>\n").arg(escapeHtml(QString::fromUtf8("结果动画"))));

        if (ext == "gif")
        {
            //GIF 是图片,浏览器与 Word(内嵌模式)均可直接显示
            QString src;
            if (_embedImages)
            {
                QFile gifFile(animationPath);
                if (!gifFile.open(QIODevice::ReadOnly))
                    return;
                src = QString("data:image/gif;base64,%1").arg(QString::fromLatin1(gifFile.readAll().toBase64()));
                gifFile.close();
            }
            else
            {
                QString dstPath = QDir(imagesDir).absoluteFilePath(videoInfo.fileName());
                QFile::remove(dstPath);
                QFile::copy(animationPath, dstPath);
                src = QFileInfo(imagesDir).fileName() + "/" + videoInfo.fileName();
            }
            html.append("<div class=\"img-wrap\">\n");
            html.append(QString("<img src=\"%1\" width=\"640\">\n").arg(src));
            html.append("</div>\n");
            return;
        }

        if (_embedImages)
        {
            //Word 兼容模式(.docx 本质是 HTML):视频无法内嵌,给出文件引用
            html.append(QString("<p><b>%1：</b>%2</p>\n")
                .arg(escapeHtml(QString::fromUtf8("动画文件"))).arg(escapeHtml(videoInfo.absoluteFilePath())));
            return;
        }

        //HTML 导出：视频复制到输出目录后用 video 标签内嵌
        QString dstPath = outDir.absoluteFilePath(videoInfo.fileName());
        QFile::remove(dstPath);
        QFile::copy(animationPath, dstPath);

        //浏览器仅支持 H.264/VP8/VP9 等编码；引擎录制的 AVI/MPEG/MP4 均为
        //MPEG-4 Part 2 编码（即使容器是 MP4），浏览器无法解码，统一转码为
        //Windows 系统编码器(h264_mf)输出的 H.264 MP4
        QString refName = videoInfo.fileName();
        QString mime = "video/mp4";
        if (ext == "webm")
        {
            mime = "video/webm";
        }
        else
        {
            QString h5Path = outDir.absoluteFilePath(videoInfo.completeBaseName() + "_h5.mp4");
            if (transcodeToH264(animationPath, h5Path))
            {
                refName = QFileInfo(h5Path).fileName();
            }
        }

        html.append("<div class=\"img-wrap\">\n");
        html.append(QString("<video controls width=\"640\"><source src=\"%1\" type=\"%2\">%3</video>\n")
            .arg(escapeHtml(refName)).arg(mime)
            .arg(escapeHtml(QString::fromUtf8("当前浏览器不支持播放该视频格式，请使用本地播放器打开"))));
        html.append("</div>\n");
    }

    bool mPostHtmlReportRenderer::transcodeToH264(const QString& srcVideo,
                                                  const QString& dstMp4)
    {
        QString ffmpeg = QCoreApplication::applicationDirPath() + "/ffmpeg/ffmpeg.exe";
        if (!QFile::exists(ffmpeg))
            return false;

        QFile::remove(dstMp4);
        // 用 Windows 系统自带的 MediaFoundation H.264 编码器（h264_mf），
        // 不依赖 libx264 等外部库
        QProcess process;
        process.start(ffmpeg, QStringList()
            << "-y" << "-loglevel" << "error"
            << "-i" << srcVideo
            << "-c:v" << "h264_mf"
            << "-an" << "-movflags" << "+faststart"
            << dstMp4);
        if (!process.waitForStarted(5000))
            return false;
        if (!process.waitForFinished(10 * 60 * 1000))
        {
            process.kill();
            return false;
        }
        return process.exitCode() == 0 && QFile::exists(dstMp4);
    }

    QString mPostHtmlReportRenderer::imageToDataUrl(const QString& path)
    {
        QFile imgFile(path);
        if (!imgFile.open(QIODevice::ReadOnly))
            return QString();
        const QByteArray base64 = imgFile.readAll().toBase64();
        imgFile.close();
        return QString("data:image/%1;base64,%2")
            .arg(QFileInfo(path).suffix().toLower(), QString::fromLatin1(base64));
    }

    bool mPostHtmlReportRenderer::renderTemplateReport(const mPostReportContext& context,
                                                        const QString& outputPath,
                                                        QString& error)
    {
        //顶栏/导航/内容区版式对齐 design/report-export/HTML模板-*.png 两张样张；
        //图片含 logo 全部 Base64 内嵌，单文件自包含，双击浏览器可开（导出后仅
        //落盘，由用户自行打开——需求文档已拍板）。定位栏数值沿革：R09 为两张
        //@1.5x 样张像素量取÷1.5 反推（225px/14px/行高 24px）；20261103-R11 用
        //户实机（宽屏+大图内容）反馈定位栏过小且随页滚走，**有意偏离 R09"设计
        //稿量取值"基线（用户实机体验优先）**：侧栏宽 300px、条目 15px/行高
        //32px、sticky 固定可见（height:100vh+overflow-y:auto，条目多时侧栏自
        //身滚动）、scrollspy 滚动高亮（文末内联原生脚本）；logo 格随侧栏
        //300px 同宽同线（R11 评审跟进项，恢复 R09 对齐验收要点），顶栏其余
        //数值维持 R09 样张量取值。其余沿革：分隔线
        //2px #303133（样张 3px）、条目左缩进 16px、logo 高 40px、"报告"26px
        //#FFA444、版权行 12px #88898C；悬停/当前态样张为静态图不可量取，保留
        //原样式
        QString logoUrl = imageToDataUrl(QStringLiteral(":/mMainWidgets/Icon/logo.png"));

        QString html;
        html.append("<!DOCTYPE html>\n<html>\n<head>\n");
        html.append("<meta charset=\"utf-8\">\n");
        html.append(QString("<title>%1</title>\n").arg(escapeHtml(context.meta.mainTitle)));
        html.append("<style>\n");
        html.append("html, body { margin:0; padding:0; font-family:'Microsoft YaHei',Arial,sans-serif;"
                    " color:#333333; background:#FFFFFF; }\n");
        //顶栏：logo(左)+报告(主题橙)+版权行(右)，高 60px 含 2px 底线（样张
        //实测）；logo 格宽随 R11 侧栏 300px 同宽同线
        html.append(".topbar { display:flex; align-items:center; height:60px; box-sizing:border-box;"
                    " border-bottom:2px solid #303133; }\n");
        html.append(".logo-cell { width:300px; box-sizing:border-box; align-self:stretch; display:flex;"
                    " align-items:center; justify-content:flex-start; padding-left:16px;"
                    " border-right:2px solid #303133; flex:none; }\n");
        html.append(".logo-cell img { height:40px; }\n");
        html.append(".topbar-title { color:#FFA444; font-size:26px; font-weight:600; margin-left:16px; }\n");
        html.append(".topbar-copy { margin-left:auto; margin-right:16px; color:#88898C; font-size:12px; }\n");
        //主体：左侧视图页导航 + 内容区。R11 定位栏加大（300px/条目 15px/行高
        //32px，内边距保持水平 16px 与 logo/内容缩进同线，加大体现在行高）+
        //sticky 固定可见（height:100vh 常驻左侧全高，内容滚动时不随页滚走）+
        //条目超视口时侧栏自身滚动（overflow-y:auto，细滚动条样式与整体一致）
        html.append(".layout { display:flex; align-items:stretch; }\n");
        html.append(".sidebar { width:300px; flex:none; border-right:2px solid #303133; padding:12px 0;"
                    " box-sizing:border-box; position:sticky; top:0; height:100vh; overflow-y:auto;"
                    " scrollbar-width:thin; }\n");
        html.append(".sidebar::-webkit-scrollbar { width:6px; }\n");
        html.append(".sidebar::-webkit-scrollbar-track { background:transparent; }\n");
        html.append(".sidebar::-webkit-scrollbar-thumb { background:#C9CCD0; border-radius:3px; }\n");
        html.append(".sidebar::-webkit-scrollbar-thumb:hover { background:#A9AEB8; }\n");
        html.append(".sidebar a { display:block; padding:0 16px; line-height:32px; color:#303133;"
                    " font-size:15px; text-decoration:none; white-space:nowrap; overflow:hidden;"
                    " text-overflow:ellipsis; }\n");
        html.append(".sidebar a:hover { background:#F7F8FA; color:#1385F6; }\n");
        //scrollspy 当前视图页高亮（文末脚本切换）：主题蓝+加粗+左侧 3px 指示条
        //（inset 阴影实现，不挤占文字宽度、无布局位移）
        html.append(".sidebar a.active { color:#1385F6; font-weight:bold; background:#F7F8FA;"
                    " box-shadow:inset 3px 0 0 0 #1385F6; }\n");
        html.append(".content { flex:1; min-width:0; padding:8px 16px 40px; box-sizing:border-box; }\n");
        html.append(".page-title { font-size:26px; font-weight:600; color:#1D2129; margin:8px 0 16px; }\n");
        html.append(".page-box { border:1px solid #E5E6EB; padding:16px; }\n");
        html.append(".page-box img.page-img { display:block; width:100%; height:auto; }\n");
        //Figure 宫格（20261102-R08 拉伸充满）：整网格保持 PPT 插入框宽高比
        //（7200900:3632200 EMU）——格子图片区高宽比经 .imgbox 的 padding-top
        //百分比内联写入（经典响应式定比盒：图片绝对定位铺满格内），每图
        //width/height 100% + object-fit:fill 非等比拉伸充满格子，图注居中
        html.append(".fig-grid { display:flex; flex-wrap:wrap; margin:-8px; }\n");
        html.append(".fig { width:33.333%; box-sizing:border-box; padding:8px; text-align:center; }\n");
        html.append(".fig .imgbox { position:relative; width:100%; }\n");
        html.append(".fig .imgbox img { position:absolute; left:0; top:0;"
                    " width:100%; height:100%; object-fit:fill; }\n");
        html.append(".fig .cap { text-align:center; color:#4E5969; font-size:13px; margin-top:6px;"
                    " overflow:hidden; text-overflow:ellipsis; }\n");
        html.append("</style>\n</head>\n<body>\n");

        //顶栏
        html.append("<div class=\"topbar\">\n");
        html.append("<div class=\"logo-cell\">");
        if (!logoUrl.isEmpty())
            html.append(QString("<img src=\"%1\" alt=\"logo\">").arg(logoUrl));
        html.append("</div>\n");
        html.append("<div class=\"topbar-title\">报告</div>\n");
        html.append("<div class=\"topbar-copy\">"
                    "© Hunan Maixi Software Co., Ltd. Proprietary and Confidential. All rights reserved."
                    "</div>\n");
        html.append("</div>\n");

        //主体：左侧导航（条目文本=页名，锚点跳转）+ 内容区（页标题+图片）
        html.append("<div class=\"layout\">\n");
        html.append("<div class=\"sidebar\">\n");
        for (const auto& section : context.sections)
        {
            html.append(QString("<a href=\"#%1\">%2</a>\n")
                .arg(escapeHtml(section.id), escapeHtml(section.title)));
        }
        html.append("</div>\n");
        html.append("<div class=\"content\">\n");
        for (const auto& section : context.sections)
        {
            html.append(QString("<div id=\"%1\">\n").arg(escapeHtml(section.id)));
            html.append(QString("<h2 class=\"page-title\">%1</h2>\n").arg(escapeHtml(section.title)));
            html.append("<div class=\"page-box\">\n");
            if (context.imagePerChart)
            {
                //勾选：Figure 宫格（20261102-R08 拉伸充满）：整网格保持 PPT
                //插入框宽高比（7200900x3632200 EMU）——行数=⌈N/列数⌉、格子
                //图片区高宽比=框高宽比×列数/行数（padding-top 百分比），每图
                //object-fit:fill 拉伸充满格子。列数=节级布局 gridCols（R07
                //跟随页面布局；0=自动 3 列，gridCols 语义不变）；单图 N=1
                //独占整框（R08 §1"单视图页该图直接拉伸充满插入框"三格式一致
                //语义）；末行不满留白；图注=宿主组装的完整图注
                int imgCount = 0;
                for (const auto& item : section.items)
                {
                    if (item.type == ReportSectionItem::Image)
                        ++imgCount;
                }
                int cols = section.gridCols > 0 ? section.gridCols : 3;
                if (imgCount == 1)
                    cols = 1;
                const int rows = (imgCount + cols - 1) / cols;
                //格子高/宽比（%）= (3632200×cols)/(7200900×rows)×100
                const double cellRatioPct =
                    3632200.0 * cols / (7200900.0 * rows) * 100.0;
                html.append("<div class=\"fig-grid\">\n");
                for (const auto& item : section.items)
                {
                    if (item.type != ReportSectionItem::Image)
                        continue;
                    const QString url = imageToDataUrl(item.image.imagePath);
                    if (url.isEmpty())
                    {
                        html.append(QString("<div class=\"fig\">图片丢失: %1</div>\n")
                            .arg(escapeHtml(item.image.imagePath)));
                        continue;
                    }
                    html.append(QString("<div class=\"fig\" style=\"width:%1%\">\n")
                        .arg(100.0 / cols, 0, 'g', 10));
                    html.append(QString("<div class=\"imgbox\" style=\"padding-top:%1%\">\n")
                        .arg(cellRatioPct, 0, 'g', 10));
                    html.append(QString("<img src=\"%1\">\n").arg(url));
                    html.append("</div>\n");
                    if (!item.image.caption.isEmpty())
                        html.append(QString("<div class=\"cap\">%1</div>\n").arg(escapeHtml(item.image.caption)));
                    html.append("</div>\n");
                }
                html.append("</div>\n");
            }
            else
            {
                //不勾选：单张页大图（页内全部图窗拼合图）
                for (const auto& item : section.items)
                {
                    if (item.type != ReportSectionItem::Image)
                        continue;
                    const QString url = imageToDataUrl(item.image.imagePath);
                    if (url.isEmpty())
                    {
                        html.append(QString("<p>图片丢失: %1</p>\n").arg(escapeHtml(item.image.imagePath)));
                        continue;
                    }
                    html.append(QString("<img class=\"page-img\" src=\"%1\">\n").arg(url));
                }
            }
            html.append("</div>\n</div>\n");
        }
        html.append("</div>\n</div>\n");

        //scrollspy 滚动定位（20261103-R11）：原生 JS 内联零依赖——监听窗口滚
        //动/resize/加载完成，按各 section 的 offsetTop 判定当前视图页，给侧栏
        //对应条目切换 .active 高亮（主题蓝/加粗/左侧 3px 指示条）；点击锚点跳
        //转后显式同步高亮（防无滚动场景脱节）。与既有 <a href="#pageN"> /
        //<div id="pageN"> 结构直接兼容，不改任何锚点标记
        html.append("<script>\n"
                    "(function () {\n"
                    "    var links = document.querySelectorAll('.sidebar a');\n"
                    "    var sections = [];\n"
                    "    for (var i = 0; i < links.length; ++i) {\n"
                    "        var id = (links[i].getAttribute('href') || '').slice(1);\n"
                    "        var sec = id ? document.getElementById(id) : null;\n"
                    "        if (sec) sections.push({ link: links[i], sec: sec });\n"
                    "    }\n"
                    "    if (sections.length === 0) return;\n"
                    "    var current = -1;\n"
                    "    function setActive(idx) {\n"
                    "        if (idx === current) return;\n"
                    "        current = idx;\n"
                    "        for (var i = 0; i < sections.length; ++i) {\n"
                    "            if (i === idx)\n"
                    "                sections[i].link.classList.add('active');\n"
                    "            else\n"
                    "                sections[i].link.classList.remove('active');\n"
                    "        }\n"
                    "    }\n"
                    "    function onScroll() {\n"
                    "        var pos = window.pageYOffset || document.documentElement.scrollTop || 0;\n"
                    "        var idx = 0;\n"
                    "        for (var i = 0; i < sections.length; ++i) {\n"
                    "            if (sections[i].sec.offsetTop <= pos + 80) idx = i;\n"
                    "        }\n"
                    "        setActive(idx);\n"
                    "    }\n"
                    "    for (var i = 0; i < sections.length; ++i) {\n"
                    "        (function (idx) {\n"
                    "            sections[idx].link.addEventListener('click', function () { setActive(idx); });\n"
                    "        })(i);\n"
                    "    }\n"
                    "    window.addEventListener('scroll', onScroll);\n"
                    "    window.addEventListener('resize', function () { onScroll(); });\n"
                    "    window.addEventListener('load', function () { onScroll(); });\n"
                    "    onScroll();\n"
                    "})();\n"
                    "</script>\n");

        html.append("</body>\n</html>");

        QFile file(outputPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            error = QString("failed to open file: %1").arg(outputPath);
            return false;
        }
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << html;
        file.close();
        return true;
    }

    QString mPostHtmlReportRenderer::escapeHtml(const QString& text) const
    {
        QString out = text;
        out.replace("&", "&amp;");
        out.replace("<", "&lt;");
        out.replace(">", "&gt;");
        out.replace("\"", "&quot;");
        return out;
    }
}
