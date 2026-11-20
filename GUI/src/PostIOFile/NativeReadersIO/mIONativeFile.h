#pragma once
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mIOEnum.h"
#include "NativeReadersDataBuilder.h"

#include <native_readers/result.hpp>

#include <QPointer>
#include <QString>
#include <QStringList>
#include <QThread>

#include <memory>

namespace native_readers
{
    class D3plotReaderSession;
}

namespace PostData
{
    class mPostDataBase;
}

namespace MIOFile
{
/*
* MxNativeReaders 原生结果文件读取线程（OP2/ANSYS RST·RTH/LS-DYNA d3plot/H3D/ODB）。
* 协议对齐 mIOP2File：导入在工作线程执行；数据源常驻内存（d3plot 走 0.3.10 流式
* 会话——会话持有归一化模型+目录+单帧解码窗口，全帧值不驻留；其余格式 Dataset
* 常驻），云图/变形按需经 readVarDatas 同步摊平发布；失败约定 finishedNativeFileSig
* 携带空 postName。
*/
class MIOFILE_EXPORT mIONativeFile : public QThread
{
    Q_OBJECT
public:
    mIONativeFile(const QString& resultFile,
                  const QString& modelFile,
                  QPointer<PostData::mPostDataBase> modeldb,
                  QObject* parent = nullptr);

    ~mIONativeFile();

    // OP2 伴随 BDF 探测（同名 / _MODEL 后缀变体）；找不到返回空串，
    // 空串时交由库自行探测（OP2 内嵌几何或同目录 .bdf/.nas）
    static QString findCompanionBdf(const QString& op2File);

    // 从候选文件列表识别原生读取格式：
    //  op2/rst/rth/odb/h3d 后缀各至多一个 + d3plot 主文件（族续文件自动扫描）
    //  + 至多一个 bdf 伴随；混入其他后缀返回 false（交回 mxdb 分支兜底）
    static bool tryGetNativeSelection(const QStringList& fileNames,
                                      QString* nativeFile = nullptr,
                                      QString* bdfFile = nullptr);

    QString getPostDataName() const;

    // 来源元信息条目（MxNativeReaders 0.3.4+，求解器原始记录的结构化视图：
    // 材料/载荷/截面等）。decoded=false 表示仅结构视图、未完全解码——UI 不得
    // 将其当作完整定义展示（库契约）；数值数组仅取摘要（前若干个）
    struct SourceMetadataEntry
    {
        QString format;
        QString category;
        QString name;
        qlonglong id = 0;
        bool decoded = false;
        QStringList texts;
        QList<qlonglong> integers;
        QList<double> reals;
    };

    // 来源元信息全量拷贝（导入成功后有效；UI 冷路径按需调用）。
    // 流式 d3plot 与全量路径同源（model()/Dataset 均随带保留记录）
    QList<SourceMetadataEntry> sourceMetadata() const;

    // 保留的来源元信息是否超出库预算被截断（截断时 sourceMetadata() 仅含
    // 保留部分，界面应提示"不完整"）
    bool sourceMetadataTruncated() const;

    // 保留的来源元信息字节数（截断提示展示用）
    quint64 sourceMetadataRetainedBytes() const;

    // 导出来源元信息 JSON 侧车（库原子写：拒绝覆盖已存在目标、拒绝写入源
    // 文件别名——调用方先做存在性预检并给出提示）。成功返回 true
    bool exportSourceMetadata(const QString& targetJsonPath, QString* error = nullptr) const;

    // 伴随结果库是否仍存活（所属后处理模型删除后 QPointer 置空）：置空后懒读会
    // 静默失败、dataset 属已删模型——再次导入同名文件前应先废弃本 reader 走全新导入
    bool isModelAlive() const { return _modeldb != nullptr; }

    // 懒读：发布 frameIndex 帧指定变量分量（complexMode 兼容旧协议保留，当前仅实数）。
    // layerName 为层下拉框选中层（复合材料铺层/壳 Z1·Z2/角点/ODB facet）：命中取
    // 该层切片，为空时跨层平均。d3plot 流式路径当帧按需 read_frame 解码（不缓存）
    bool readVarDatas(int frameIndex,
                      const QString& variableName,
                      const QString& componentName,
                      const QString& complexMode,
                      const QString& layerName = QString()) const;

protected:
    void run() override;

signals:
    void readFileSuccessSig(const QString& postName);
    void readNativeErrorSig(MIOFile::IOFileErrorType type);
    void finishedNativeFileSig(const QString& postName);

private:
    QString _resultFile;
    QString _modelFile;
    QPointer<PostData::mPostDataBase> _modeldb;

    // 数据源（二选一，_datasetReady 标记导入成功前均不可用）：
    //  - d3plot：流式会话（0.3.10 起，未压缩已证布局；构造失败回退 _dataset 全量路径）
    //  - 其余格式：Dataset 常驻内存
    native_readers::Dataset _dataset;
    std::unique_ptr<native_readers::D3plotReaderSession> _d3plotSession;
    bool _datasetReady = false;

    // 当前数据源（流式会话 model() 或全量 Dataset），导入成功前返回空
    const native_readers::Dataset* activeDataset() const;
};

} // namespace MIOFile
