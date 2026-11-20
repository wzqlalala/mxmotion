#pragma once
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mIOEnum.h"
#include "VtkDataBuilder.h"

#include <vfr/dataset.hpp>
#include <vfr/reader.hpp>

#include <QPointer>
#include <QString>
#include <QStringList>
#include <QThread>
#include <vector>

namespace PostData
{
    class mPostDataBase;
}

namespace MIOFile
{
/*
* VTK 后处理文件读取线程（.vtk legacy / .vtp/.vtu XML / .vtm 多 block /
* .pvtp/.pvtu parallel 聚合，走 VTKFileReader/vfr）。协议对齐 mIONativeFile：
* 导入在工作线程执行；多选成组多帧（一文件一帧，文件名自然排序为帧序），
* DataSet 列表常驻内存，云图/变形按需经 readVarDatas 同步摊平发布；
* 失败约定 finishedVtkFileSig 携带空 postName。
* v1 不支持 .pvd 集合与结构化网格（ImageData 等），由读取/装配层明确报错。
*/
class MIOFILE_EXPORT mIOVtkFile : public QThread
{
    Q_OBJECT
public:
    mIOVtkFile(const QStringList& resultFiles,
               QPointer<PostData::mPostDataBase> modeldb,
               QObject* parent = nullptr);

    ~mIOVtkFile();

    // vtk 系后缀识别（.vtk/.vtp/.vtu/.vtm/.pvtp/.pvtu，大小写不敏感）
    static bool isVtkFile(const QString& fileName);

    // 从候选文件列表收集全部 vtk 系文件（成组多帧的组员）；至少一个才返回 true
    static bool tryGetVtkSelection(const QStringList& fileNames, QStringList* vtkFiles = nullptr);

    // 组内文件名自然排序（数字段按数值比较，如 frame_2 < frame_10），即帧序
    static QStringList sortAsSeries(const QStringList& fileNames);

    QString getPostDataName() const;

    // 伴随结果库是否仍存活（所属后处理模型删除后 QPointer 置空）：置空后懒读会
    // 静默失败——再次导入同名文件前应先废弃本 reader 走全新导入
    bool isModelAlive() const { return _modeldb != nullptr; }

    // 懒读：发布 frameIndex 帧指定变量分量（complexMode/layerName 兼容旧协议
    // 保留参数，VTK 链路当前均忽略）
    bool readVarDatas(int frameIndex,
                      const QString& variableName,
                      const QString& componentName,
                      const QString& complexMode,
                      const QString& layerName = QString()) const;

protected:
    void run() override;

signals:
    void readFileSuccessSig(const QString& postName);
    void readVtkErrorSig(MIOFile::IOFileErrorType type);
    void finishedVtkFileSig(const QString& postName);

private:
    QStringList _resultFiles;   // 已按帧序排序
    QPointer<PostData::mPostDataBase> _modeldb;

    // 各帧读取结果常驻内存（懒读数据源），_datasetReady 标记导入成功前不可用
    std::vector<vfr::DataSet> _datasets;
    bool _datasetReady = false;
};

} // namespace MIOFile
