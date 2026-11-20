#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mPostReportRenderer.h"

#include <QJsonObject>
#include <QJsonArray>

namespace PostData
{
    // PowerPoint 报告渲染器：通过 QProcess 调用 python-pptx 脚本生成 .pptx
    class MIOFILE_EXPORT mPostPptReportRenderer : public mPostReportRenderer
    {
    public:
        QString name() const override { return "PowerPoint"; }
        QString fileFilter() const override { return "PPTX Files(*.pptx)"; }

        bool render(const mPostReportContext& context,
                    const QString& outputPath,
                    QString& error) override;

    private:
        bool saveContextToJson(const mPostReportContext& context, const QString& jsonPath, QString& error);
        QJsonObject metaToJson(const ReportMeta& meta) const;
        //includeLayout：节级布局字段（gridCols/gridRows）是否写入 JSON——
        //仅模板分支写入（true），老链路 false 保持 JSON 与改前逐字节一致
        QJsonArray sectionsToJson(const QList<ReportSection>& sections, bool includeLayout) const;
        QJsonObject sectionItemToJson(const ReportSectionItem& item) const;
        QJsonObject tableToJson(const ReportTable& table) const;
    };
}
