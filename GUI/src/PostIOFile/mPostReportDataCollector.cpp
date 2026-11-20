#include "mPostReportDataCollector.h"
#include "mPostExportWordData.h"

#include <QDir>
#include <QFileInfo>

namespace PostData
{
    mPostReportDataCollector::mPostReportDataCollector(PostData::mPostExportWordData* wordData,
                                                       const QString& curViewName)
        : _wordData(wordData), _curViewName(curViewName)
    {
        QString exePath;
        // 由于 PostIOFile 不直接访问 QCoreApplication，imageDir 由调用方传入或后续补充
        _imageDir = QString("Report/post-images/%1").arg(curViewName);
    }

    bool mPostReportDataCollector::collect(mPostReportContext& outContext, QString& error)
    {
        if (_wordData == nullptr)
        {
            error = "export word data is null";
            return false;
        }

        collectMeta(outContext);
        collectSummary(outContext);
        collectDescription(outContext);
        collectModel(outContext);
        collectMaterial(outContext);
        collectProperty(outContext);
        collectCase(outContext);
        collectLoad(outContext);
        collectBoundary(outContext);
        collectConnector(outContext);
        collectContact(outContext);
        collectResult(outContext);

        return true;
    }

    bool mPostReportDataCollector::collectMeta(mPostReportContext& ctx)
    {
        if (_wordData == nullptr)
            return false;
        ctx.meta.mainTitle = "MxSim仿真分析报告";
        ctx.meta.showSubTitle = _wordData->_reportShowSubTitle;
        ctx.meta.subTitle = _wordData->_reportSubTitle;
        ctx.meta.showAuthor = _wordData->_reportShowUser;
        ctx.meta.author = _wordData->_reportUser;
        ctx.meta.showCompany = _wordData->_reportShowCompany;
        ctx.meta.company = _wordData->_reportCompany;
        ctx.meta.showAddress = _wordData->_reportShowAddress;
        ctx.meta.address = _wordData->_reportAddress;
        ctx.meta.showPhone = _wordData->_reportShowPhone;
        ctx.meta.phone = _wordData->_reportPhone;
        return true;
    }

    bool mPostReportDataCollector::collectSummary(mPostReportContext& ctx)
    {
        // 用例概述表格由调用方在生成时补充（需要 mxFileName / testDate）
        return true;
    }

    bool mPostReportDataCollector::collectDescription(mPostReportContext& ctx)
    {
        // 说明内容目前为空，由 UI 层在生成时填充
        return true;
    }

    bool mPostReportDataCollector::collectModel(mPostReportContext& ctx)
    {
        // 模型数据由 UI 层在生成时填充
        return true;
    }

    bool mPostReportDataCollector::collectMaterial(mPostReportContext& ctx)
    {
        return true;
    }

    bool mPostReportDataCollector::collectProperty(mPostReportContext& ctx)
    {
        return true;
    }

    bool mPostReportDataCollector::collectCase(mPostReportContext& ctx)
    {
        return true;
    }

    bool mPostReportDataCollector::collectLoad(mPostReportContext& ctx)
    {
        return true;
    }

    bool mPostReportDataCollector::collectBoundary(mPostReportContext& ctx)
    {
        return true;
    }

    bool mPostReportDataCollector::collectConnector(mPostReportContext& ctx)
    {
        return true;
    }

    bool mPostReportDataCollector::collectContact(mPostReportContext& ctx)
    {
        return true;
    }

    bool mPostReportDataCollector::collectResult(mPostReportContext& ctx)
    {
        return true;
    }
}
