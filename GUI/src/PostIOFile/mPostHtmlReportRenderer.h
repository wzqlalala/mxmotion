#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mPostReportRenderer.h"

#include <QDir>

namespace PostData
{
    // HTML 报告渲染器
    class MIOFILE_EXPORT mPostHtmlReportRenderer : public mPostReportRenderer
    {
    public:
        QString name() const override { return "HTML"; }
        QString fileFilter() const override { return "HTML Files(*.html)"; }
        bool supportsEmbeddedImages() const override { return true; }

        // 图片输出模式：false=拷贝到 html 旁的图片目录（默认），
        // true=Base64 内嵌为 data URL（单文件自包含，供 Word 兼容导出使用）
        void setImageEmbedMode(bool embed) { _embedImages = embed; }

        bool render(const mPostReportContext& context,
                    const QString& outputPath,
                    QString& error) override;

    private:
        // 曲线报告新版式（templateMode=true，对齐设计图03/HTML样张）：
        // 顶栏(logo+报告+版权行)/左侧视图页导航/内容区页标题+图片宫格，
        // 单文件自包含（图片含 logo 全部 Base64 内嵌）。
        // 老分支（3D 报告 HTML/Word 冒充链路）走 render() 原逻辑零变化
        bool renderTemplateReport(const mPostReportContext& context,
                                  const QString& outputPath,
                                  QString& error);

        // 图片读入并转为 data:image/...;base64 URL（模板新版式恒内嵌）；失败返回空串
        static QString imageToDataUrl(const QString& path);

        void renderMeta(const ReportMeta& meta, QString& html);
        void renderSection(const ReportSection& section, QString& html, const QString& outputDir);
        void renderTable(const ReportTable& table, QString& html);
        void renderImage(const ReportImage& image, QString& html, const QString& outputDir);
        void renderText(const ReportText& text, QString& html);
        void renderAnimation(const QString& animationPath, QString& html,
                             const QDir& outDir, const QString& imagesDir);

        /** 用自带 ffmpeg 将视频转码为浏览器可播的 H.264 MP4 */
        bool transcodeToH264(const QString& srcVideo, const QString& dstMp4);

        QString escapeHtml(const QString& text) const;

        bool _embedImages = false;
    };
}
