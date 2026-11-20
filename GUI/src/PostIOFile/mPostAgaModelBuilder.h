#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

// 后处理数据库 → Altair Generic ASCII 数据模型 适配器。
// 从 mPostDataBase 抽取基准网格与逐帧结果数组，填充 altairresultio::AgaModel，
// 供 AgaWriter 写出后经 HvTrans 转换为 H3D（方案见 AltairResultIO/doc）。

#include "miofile_global.h"

#include <QString>
#include <QStringList>

namespace altairresultio
{
    struct AgaModel;    // 完整定义见 AltairResultIO/aga_data.h
}

namespace PostData
{
    class mPostDataBase;
}

namespace MxPost
{
    // 从后处理数据库按帧抽取网格 + 位移/标量数组。
    // frameIds: 要导出的数据库帧号列表（如动画面板当前帧范围对应的帧号）。
    // 网格取自第一帧；位移数据集名固定为 Displacement；标量数据集名取帧内
    // 首个变量名（自动区分节点/单元位置），全部成功返回 true，失败填充 error。
    // skipped: 非空时返回不支持单元的跳过统计（"类型: 数量" 逗号分隔）。
    bool MIOFILE_EXPORT buildAgaModelFromPost(PostData::mPostDataBase* postDB,
                               const QList<int>& frameIds,
                               altairresultio::AgaModel& out,
                               QString& error,
                               QString* skipped = nullptr);

    // 查找本机 hvtrans.exe：依次检查 %ALTAIR_HOME%、Program Files 下 Altair 版本目录。
    // 找不到返回空串。
    QString MIOFILE_EXPORT findHvTransExe();
}
