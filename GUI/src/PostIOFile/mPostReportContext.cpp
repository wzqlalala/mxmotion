#include "mPostReportContext.h"

namespace PostData
{
    void mPostReportContext::clear()
    {
        meta = ReportMeta();
        sections.clear();
        resourceImages.clear();
        animationPaths.clear();
        templateMode = false;
        templatePath.clear();
        imagePerChart = false;
    }

    void mPostReportContext::addSection(const ReportSection& section)
    {
        sections.append(section);
    }

    int mPostReportContext::findSectionIndex(const QString& sectionId) const
    {
        for (int i = 0; i < sections.size(); ++i)
        {
            if (sections[i].id == sectionId)
                return i;
        }
        return -1;
    }

    void mPostReportContext::addText(const QString& sectionId, const QString& text)
    {
        int idx = findSectionIndex(sectionId);
        if (idx < 0)
        {
            ReportSection sec;
            sec.id = sectionId;
            sec.title = sectionId;
            addSection(sec);
            idx = sections.size() - 1;
        }
        ReportSectionItem item;
        item.type = ReportSectionItem::Text;
        item.text.content = text;
        sections[idx].items.append(item);
    }

    void mPostReportContext::addImage(const QString& sectionId, const QString& path, const QString& caption)
    {
        int idx = findSectionIndex(sectionId);
        if (idx < 0)
        {
            ReportSection sec;
            sec.id = sectionId;
            sec.title = sectionId;
            addSection(sec);
            idx = sections.size() - 1;
        }
        ReportSectionItem item;
        item.type = ReportSectionItem::Image;
        item.image.imagePath = path;
        item.image.caption = caption;
        sections[idx].items.append(item);

        if (!resourceImages.contains(path))
            resourceImages.append(path);
    }

    void mPostReportContext::addTable(const QString& sectionId, const ReportTable& table)
    {
        int idx = findSectionIndex(sectionId);
        if (idx < 0)
        {
            ReportSection sec;
            sec.id = sectionId;
            sec.title = sectionId;
            addSection(sec);
            idx = sections.size() - 1;
        }
        ReportSectionItem item;
        item.type = ReportSectionItem::Table;
        item.table = table;
        sections[idx].items.append(item);
    }
}
