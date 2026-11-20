#pragma once
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include <native_readers/result.hpp>
#include <native_readers/d3plot_stream.hpp>

#include <QHash>
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
* 将 native_readers::Dataset（OP2/ANSYS RST·RTH/LS-DYNA d3plot/H3D/ODB 原生读取结果）
* 写入现有 PostData 数据库。协议对齐 Op2PostDataBuilder：
*  - 帧号 1 = 首个结果帧（复用几何帧对象）；帧 2..N = 几何克隆帧；
*    原始几何经 appendInitframe 注册为初始构型（变形显示基准）。
*  - 导入期只注册帧/变量元信息并发布首帧首分量；变量值按需经 readVarDatas
*    从常驻内存的 Dataset 摊平发布（同步调用，结果写入 mPostDataBase）。
*  - element_nodal 场（MxNativeReaders 0.3.12 起 OP2/RST/H3D/ODB 实际产出，
*    ids=单元、element_node_counts/ids=显式单元-节点映射）按单元变量注册，
*    读取时对每单元的节点样本等权平均后发布单元值。
*/
class MIOFILE_EXPORT NativeReadersDataBuilder
{
public:
    // 导入主流程：几何 + 全部结果帧元信息 + 首帧首分量发布。
    // 失败时若 error 非空则回填英文技术原因（首因优先，面向输出窗口/排障），返回 false；
    // 成功但存在部分降级（如跳过不支持类型的单元）时向 notes 追加英文说明条目
    bool buildAll(const native_readers::Dataset& dataset, QPointer<PostData::mPostDataBase> db,
                  QString* error = nullptr, QStringList* notes = nullptr);

    // d3plot 流式会话路径的导入主流程（对齐 buildAll 语义）：几何 + 全部保存状态
    // 的帧/变量元信息注册。frameDirectory/fieldCatalog 来自
    // D3plotReaderSession::frames()/fields()（会话构造期已解码首帧并建立目录）；
    // model 为 session->model()（不含 frames）。首帧首分量不在本函数发布——调用方
    // read_frame(0) 后经 readVarDatasFrame 发布（选择口径见
    // selectFirstDisplayableVariable）
    bool buildAllFromStream(const native_readers::Dataset& model,
                            const std::vector<native_readers::D3plotFrameInfo>& frameDirectory,
                            const std::vector<native_readers::D3plotFieldInfo>& fieldCatalog,
                            QPointer<PostData::mPostDataBase> db,
                            QString* error = nullptr,
                            QStringList* notes = nullptr);

    // 云图初显分量选择（与 GUI 分量选择优先级一致：Mag > vonMises > 首分量）。
    // 全量路径在 buildResultFrames 内联同口径逻辑；流式路径经目录调用本入口
    static bool selectFirstDisplayableVariable(const std::vector<native_readers::D3plotFieldInfo>& catalog,
                                               QString* variableName,
                                               QString* componentName);

    // 懒读：发布 frameIndex（1 起的 GUI 帧号）下 variableName 的 componentName 分量。
    // complexMode 兼容旧协议保留参数；当前 Dataset 仅实数，忽略该参数。
    // layerName 为 GUI 层下拉框选中的层（复合材料铺层/壳 Z1·Z2/角点/ODB facet）：
    // 命中 field.layers 标签时只取该层切片；为空时跨层取平均（旧 OP2 链路口径）。
    bool readVarDatas(const native_readers::Dataset& dataset,
                      QPointer<PostData::mPostDataBase> db,
                      int frameIndex,
                      const QString& variableName,
                      const QString& componentName,
                      const QString& layerName = QString());

    // 帧级懒读入口（d3plot 流式会话路径）：frame 为会话 read_frame 取得的当帧
    // 对象，model 提供几何/单元表（session->model()，不含 frames）；frameIndex 仍为
    // 1 起的 GUI 帧号。分量/层语义与 readVarDatas 完全一致。
    bool readVarDatasFrame(const native_readers::Dataset& model,
                           const native_readers::Frame& frame,
                           QPointer<PostData::mPostDataBase> db,
                           int frameIndex,
                           const QString& variableName,
                           const QString& componentName,
                           const QString& layerName = QString());

private:
    bool buildGeometry(const native_readers::Dataset& dataset, QPointer<PostData::mPostDataBase> db,
                       QString* error, QStringList* notes);
    bool buildResultFrames(const native_readers::Dataset& dataset, QPointer<PostData::mPostDataBase> db,
                           QString* error, QStringList* notes);
    // 流式路径的帧元信息注册（buildAllFromStream 的第二段）：目录转临时 Field
    // 复用全量注册入口，帧值不发布
    bool buildResultFramesFromStream(const native_readers::Dataset& model,
                                     const std::vector<native_readers::D3plotFrameInfo>& frameDirectory,
                                     const std::vector<native_readers::D3plotFieldInfo>& fieldCatalog,
                                     QPointer<PostData::mPostDataBase> db,
                                     QString* error, QStringList* notes);

    // 角点→节点平均伪变量（"<base> (nodal averaged)"）的读取实现：把角点源场
    // （OP2 Corner Samples / RST at corners）按显式 GRID 或连接槽位反演到节点，
    // 共享节点等权平均后按节点位发布；model 提供单元表（角点槽位反演用）
    bool readNodalAverageDataFrame(const native_readers::Dataset& model,
                                   const native_readers::Frame& frame,
                                   QPointer<PostData::mPostDataBase> db,
                                   int frameIndex,
                                   const QString& variableName,
                                   const QString& baseName,
                                   const QString& componentName);

    QPointer<PostData::mPostModelData> cloneGeometry(QPointer<PostData::mPostModelData> source) const;
};

} // namespace MIOFile
