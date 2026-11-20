#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mPostReportContext.h"

#include <QString>

namespace PostData
{
    class mPostExportWordData;
}

namespace MechMainWindow
{
    struct PartInfo;
    struct ResultInfo;
}

namespace PostData
{
    // 报告数据收集器：将现有 UI/数据模型转换为与格式无关的 mPostReportContext
    class MIOFILE_EXPORT mPostReportDataCollector
    {
    public:
        explicit mPostReportDataCollector(PostData::mPostExportWordData* wordData,
                                          const QString& curViewName);

        // 主入口
        bool collect(mPostReportContext& outContext, QString& error);

    private:
        bool collectMeta(mPostReportContext& ctx);
        bool collectSummary(mPostReportContext& ctx);
        bool collectDescription(mPostReportContext& ctx);
        bool collectModel(mPostReportContext& ctx);
        bool collectMaterial(mPostReportContext& ctx);
        bool collectProperty(mPostReportContext& ctx);
        bool collectCase(mPostReportContext& ctx);
        bool collectLoad(mPostReportContext& ctx);
        bool collectBoundary(mPostReportContext& ctx);
        bool collectConnector(mPostReportContext& ctx);
        bool collectContact(mPostReportContext& ctx);
        bool collectResult(mPostReportContext& ctx);

        // 辅助
        void appendText(mPostReportContext& ctx, const QString& sectionId, const QString& text);
        void appendImage(mPostReportContext& ctx, const QString& sectionId,
                         const QString& path, const QString& caption);
        void appendPartInfos(mPostReportContext& ctx, const QString& sectionId,
                             const QString& tableTitle, const QList<MechMainWindow::PartInfo>& infos);
        void appendUserPictures(mPostReportContext& ctx, const QString& sectionId, const QString& type);

        PostData::mPostExportWordData* _wordData = nullptr;
        // B1b(M12 同款收编注记)：本类非 mPanelBase 系（无继承遮蔽），_curViewName
        // 为自有会话态；视口改名跟随若需覆盖本对象，须由持有方显式同步（B2/B3 批次评估）
        QString _curViewName;
        QString _imageDir;
    };
}
