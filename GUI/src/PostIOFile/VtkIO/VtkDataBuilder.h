#pragma once
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mIOEnum.h"

#include <vfr/dataset.hpp>

#include <QPointer>
#include <QString>
#include <QStringList>
#include <vector>

namespace PostData
{
    class mPostDataBase;
    class mPostModelData;
}

namespace MIOFile
{

/*
* 将 vfr::DataSet（VTK legacy/XML/parallel 后处理读取结果）写入现有 PostData 数据库。
* 协议对齐 NativeReadersDataBuilder：
*  - 多选成组多帧：每个文件一帧（顺序即帧序），帧 1 复用几何帧对象，帧 2..N 为
*    几何克隆帧；原始几何经 appendInitframe 注册为初始构型。各帧拓扑必须一致。
*  - 导入期只注册帧/变量元信息并发布首帧首分量；变量值按需经 readVarDatas
*    从常驻内存的 DataSet 列表摊平发布（同步调用，结果写入 mPostDataBase）。
*  - 仅支持非结构网格（PolyData/UnstructuredGrid，含 .vtm 多 block/parallel 聚合），
*    结构化数据集（ImageData/StructuredGrid/RectilinearGrid/StructuredPoints）与
*    .pvd 集合明确不支持，导入时报错。
*/
class MIOFILE_EXPORT VtkDataBuilder
{
public:
    // 导入主流程：frames 与 fileNames 等长（一文件一帧，帧序 = 列表顺序）。
    // 失败时若 error 非空则回填中文技术原因（面向输出窗口/排障），返回 false；
    // 成功但存在部分降级（如跳过多边形/三角带单元）时向 notes 追加说明条目
    bool buildAll(const std::vector<vfr::DataSet>& frames, const QStringList& fileNames,
                  QPointer<PostData::mPostDataBase> db,
                  QString* error = nullptr, QStringList* notes = nullptr);

    // 懒读：发布 frameIndex（1 起的 GUI 帧号）下 variableName 的 componentName 分量。
    // 分量命名约定：1 分量=变量名，2=X/Y，3=X/Y/Z（注册 Mag 派生），6=XX..ZX
    // （注册 vonMises 派生），其余 C1..Cn；complexMode 兼容旧协议保留参数（忽略）
    bool readVarDatas(const std::vector<vfr::DataSet>& frames,
                      QPointer<PostData::mPostDataBase> db,
                      int frameIndex,
                      const QString& variableName,
                      const QString& componentName);

private:
    bool buildGeometry(const std::vector<vfr::DataSet>& frames,
                       QPointer<PostData::mPostDataBase> db,
                       QString* error, QStringList* notes);
    bool buildResultFrames(const std::vector<vfr::DataSet>& frames,
                           const QStringList& fileNames,
                           QPointer<PostData::mPostDataBase> db,
                           QString* error, QStringList* notes);

    QPointer<PostData::mPostModelData> cloneGeometry(QPointer<PostData::mPostModelData> source) const;
};

} // namespace MIOFile
