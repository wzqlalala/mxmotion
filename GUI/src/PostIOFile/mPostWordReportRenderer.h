#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mPostReportRenderer.h"

#include <QJsonObject>

namespace PostData
{
    // Word 报告渲染器：
    //  - 老分支（3D 后处理报告，templateMode=false，行为零变化）：把 HTML 内容
    //    （图片 Base64 内嵌）写入 .docx（兼容模式）；
    //  - 模板分支（曲线报告，templateMode=true）：写 JSON 上下文后经 QProcess
    //    调用 export_docx.py 生成真 OOXML .docx（报告母版封面/摘要/目录原样，
    //    示例章节替换为真实章节），机制与 PPT 渲染器同款。
    class MIOFILE_EXPORT mPostWordReportRenderer : public mPostReportRenderer
    {
    public:
        QString name() const override { return "Word"; }
        QString fileFilter() const override { return "DOCX Files(*.docx)"; }

        bool render(const mPostReportContext& context,
                    const QString& outputPath,
                    QString& error) override;

    private:
        // 复用 mPostHtmlReportRenderer 生成图片内嵌的 HTML 内容（老分支专用）
        bool generateHtml(const mPostReportContext& context, QString& html, QString& error);

        // 模板分支：上下文写 JSON（键契约与 PPT 渲染器一致；就地复制实现，
        // 避免为复用而改动 PPT 渲染器头文件）
        bool saveContextToJson(const mPostReportContext& context, const QString& jsonPath, QString& error);
    };
}
