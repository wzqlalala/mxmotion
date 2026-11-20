#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"

#include <QString>
#include <QStringList>
#include <QList>
#include <QVariant>

namespace PostData
{
    // 表格单元格
    struct MIOFILE_EXPORT ReportCell
    {
        QString text;
        int rowSpan = 1;
        int colSpan = 1;
        bool isHeader = false;
        QString hAlign = "center";   // left / center / right
    };

    // 表格行
    struct MIOFILE_EXPORT ReportRow
    {
        QList<ReportCell> cells;
    };

    // 表格
    struct MIOFILE_EXPORT ReportTable
    {
        QString title;
        QList<ReportRow> rows;
        int columnCount = 0;
    };

    // 图片
    struct MIOFILE_EXPORT ReportImage
    {
        QString imagePath;           // 本地绝对路径或相对路径
        QString caption;             // 图题
        bool embedBase64 = false;    // 是否内嵌 Base64（HTML 单文件场景）
    };

    // 段落/文本
    struct MIOFILE_EXPORT ReportText
    {
        QString content;
        bool isBold = false;
    };

    // 章节内元素
    struct MIOFILE_EXPORT ReportSectionItem
    {
        enum Type { Text, Image, Table } type;
        ReportText  text;
        ReportImage image;
        ReportTable table;

        ReportSectionItem() : type(Text) {}
    };

    // 章节
    struct MIOFILE_EXPORT ReportSection
    {
        QString id;                  // "description", "model", "material", ...
        QString title;               // 本地化标题，如 "1  模型"
        QList<ReportSectionItem> items;
        //节级布局网格（20261102-R07 报告导出跟随页面布局；模板分支契约字段）：
        //来源=页面 kLayout 预设派生（PostCurve::curvePageLayoutGrid）。
        //0=自动（kLayout 未选/越界/不可解析，渲染侧维持各自现状排布——
        //曲线报告即 R06 固定 3 列）。gridRows 仅为预设名义行数的透传，
        //渲染侧行数一律按 ⌈图窗数/gridCols⌉ 现算（空格留白/同列数续行）
        int gridCols = 0;
        int gridRows = 0;
    };

    // 报告元数据
    struct MIOFILE_EXPORT ReportMeta
    {
        QString mainTitle;           // "MxSim仿真分析报告"
        QString subTitle;
        QString author;              // 分析员
        QString company;
        QString address;
        QString phone;
        QString dateTime;
        QString mxFileName;
        bool showSubTitle = false;
        bool showAuthor   = false;
        bool showCompany  = false;
        bool showAddress  = false;
        bool showPhone    = false;
    };

    // 完整报告上下文：一份报告的临时快照，与 UI 和格式无关
    class MIOFILE_EXPORT mPostReportContext
    {
    public:
        ReportMeta meta;
        QList<ReportSection> sections;
        QStringList resourceImages;  // 所有图片资源路径（用于打包或复制）
        QStringList animationPaths;  // 已导出的动画文件（AVI/MP4/GIF 等），逐个追加到报告末尾；为空表示不追加
        bool windowPerSlide = true;  // PPT 专用：true 每个窗口图占一页幻灯片，false 本节全部图片合并为一页网格

        //--- 曲线报告模板化分支（20261029 报告输出模板化）---
        //新字段全部带默认值：3D 后处理报告等老调用方不感知、不设置，渲染器走原逻辑
        bool templateMode = false;   // 曲线报告分支开关：true 时渲染器按模板/新版式导出
        QString templatePath;        // 模板文件绝对路径（.pptx/.docx）；空 = 使用随程序发布的默认母版
        bool imagePerChart = false;  // 图片粒度：true 每个图窗一张图片，false 每个视图页合成一张大图

        void clear();
        void addSection(const ReportSection& section);

        // 便捷函数：在指定 id 的章节中追加元素，章节不存在则自动创建
        void addText(const QString& sectionId, const QString& text);
        void addImage(const QString& sectionId, const QString& path, const QString& caption);
        void addTable(const QString& sectionId, const ReportTable& table);

        // 查找章节索引，-1 表示不存在
        int findSectionIndex(const QString& sectionId) const;
    };
}
