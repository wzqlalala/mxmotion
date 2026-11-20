#include "mIONativeFile.h"

#include "NativeReadersDataBuilder.h"
#include "mPostDataBase.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <native_readers/d3plot_stream.hpp>

#include <filesystem>
#include <stdexcept>

namespace MIOFile
{
    namespace
    {
        std::filesystem::path toFileSystemPath(const QString& value)
        {
            return std::filesystem::path(value.toStdWString());
        }

        // d3plot 族续文件判定：d3plot + 2~3 位纯数字后缀（d3plot01..d3plotNN，
        // 与库内 d3plot_reader 的族扫描规则一致）；主文件“d3plot”本身不含在内
        bool isD3plotFamilyMember(const QString& fileName)
        {
            const QString name = fileName.trimmed();
            if (name.size() < 7 || name.size() > 9)
                return false;
            if (name.left(6).compare(QStringLiteral("d3plot"), Qt::CaseInsensitive) != 0)
                return false;
            const QString digits = name.mid(6);
            for (const QChar& character : digits)
            {
                if (!character.isDigit())
                    return false;
            }
            return true;
        }

        QString translateNativeError(MIOFile::IOFileErrorType type, bool isChinese)
        {
            switch (type)
            {
            case DatabaseNull:
                return isChinese ? QStringLiteral("数据库不存在！") : QStringLiteral("The database does not exist!");
            case NativeFileNotExist:
                return isChinese ? QStringLiteral("结果文件不存在！") : QStringLiteral("The result file does not exist!");
            case NativeParseFailed:
                return isChinese ? QStringLiteral("结果文件解析失败！") : QStringLiteral("Failed to parse the result file!");
            case NativeUnsupportedLayout:
                return isChinese ? QStringLiteral("文件格式布局不受支持或无有效结果数据！") : QStringLiteral("The file layout is not supported or contains no valid results!");
            default:
                return isChinese ? QStringLiteral("未知错误！") : QStringLiteral("Unknown error!");
            }
        }
    }

    mIONativeFile::mIONativeFile(const QString& resultFile,
                                 const QString& modelFile,
                                 QPointer<PostData::mPostDataBase> modeldb,
                                 QObject* parent)
        : QThread(parent), _resultFile(resultFile), _modelFile(modelFile), _modeldb(modeldb)
    {
    }

    mIONativeFile::~mIONativeFile()
    {
        if (isRunning())
            wait();
    }

    QString mIONativeFile::findCompanionBdf(const QString& op2File)
    {
        const QFileInfo op2Info(op2File);
        if (op2Info.fileName().isEmpty())
            return QString();

        const QDir directory = op2Info.absoluteDir();
        const QString baseName = op2Info.completeBaseName();

        QStringList candidates;
        candidates << baseName + QStringLiteral(".bdf")
                   << baseName + QStringLiteral("_MODEL.bdf");

        if (baseName.endsWith(QStringLiteral("_MODEL"), Qt::CaseInsensitive))
        {
            const QString suffix = QStringLiteral("_MODEL");
            const QString stripped = baseName.left(baseName.size() - suffix.size());
            candidates << stripped + QStringLiteral(".bdf");
        }

        for (const QString& candidate : candidates)
        {
            const QString path = directory.filePath(candidate);
            if (QFile::exists(path))
                return QDir::cleanPath(path);
        }
        return QString();
    }

    bool mIONativeFile::tryGetNativeSelection(const QStringList& fileNames,
                                              QString* nativeFile,
                                              QString* bdfFile)
    {
        QString selectedNativeFile;
        QString selectedBdfFile;
        for (const QString& fileName : fileNames)
        {
            if (fileName.trimmed().isEmpty())
                continue;

            const QFileInfo fileInfo(fileName.trimmed());
            const QString suffix = fileInfo.suffix();
            if (suffix.compare(QStringLiteral("op2"), Qt::CaseInsensitive) == 0 ||
                suffix.compare(QStringLiteral("rst"), Qt::CaseInsensitive) == 0 ||
                suffix.compare(QStringLiteral("rth"), Qt::CaseInsensitive) == 0 ||
                suffix.compare(QStringLiteral("odb"), Qt::CaseInsensitive) == 0 ||
                suffix.compare(QStringLiteral("h3d"), Qt::CaseInsensitive) == 0)
            {
                if (!selectedNativeFile.isEmpty())
                    return false;
                selectedNativeFile = QDir::cleanPath(fileInfo.absoluteFilePath());
            }
            else if (fileInfo.fileName().compare(QStringLiteral("d3plot"), Qt::CaseInsensitive) == 0)
            {
                // d3plot 主文件（d3plot01..NN 族续文件由库在同目录自动扫描）
                if (!selectedNativeFile.isEmpty())
                    return false;
                selectedNativeFile = QDir::cleanPath(fileInfo.absoluteFilePath());
            }
            else if (isD3plotFamilyMember(fileInfo.fileName()))
            {
                // 用户在对话框多选了族续文件（d3plot01..NN）：忽略即可，
                // 库读取主文件时会按同目录扫描收编，避免整组选择被拒后落入 mxdb 分支
            }
            else if (suffix.compare(QStringLiteral("bdf"), Qt::CaseInsensitive) == 0)
            {
                if (!selectedBdfFile.isEmpty())
                    return false;
                selectedBdfFile = QDir::cleanPath(fileInfo.absoluteFilePath());
            }
            else
            {
                return false;
            }
        }

        if (selectedNativeFile.isEmpty())
            return false;
        if (nativeFile != nullptr)
            *nativeFile = selectedNativeFile;
        if (bdfFile != nullptr)
            *bdfFile = selectedBdfFile;
        return true;
    }

    QString mIONativeFile::getPostDataName() const
    {
        return _modeldb ? _modeldb->getName() : QString();
    }

    const native_readers::Dataset* mIONativeFile::activeDataset() const
    {
        if (!_datasetReady)
            return nullptr;
        if (_d3plotSession != nullptr)
            return &_d3plotSession->model();
        return &_dataset;
    }

    QList<mIONativeFile::SourceMetadataEntry> mIONativeFile::sourceMetadata() const
    {
        QList<SourceMetadataEntry> entries;
        const native_readers::Dataset* dataset = activeDataset();
        if (dataset == nullptr)
            return entries;
        entries.reserve(static_cast<int>(dataset->source_metadata.size()));
        for (const auto& record : dataset->source_metadata)
        {
            SourceMetadataEntry entry;
            entry.format = QString::fromStdString(record.format);
            entry.category = QString::fromStdString(record.category);
            entry.name = QString::fromStdString(record.name);
            entry.id = record.id;
            entry.decoded = record.decoded;
            for (const auto& text : record.texts)
                entry.texts.append(QString::fromStdString(text));
            for (const auto& integer : record.integers)
                entry.integers.append(integer);
            for (const auto& real : record.reals)
                entry.reals.append(real);
            entries.append(entry);
        }
        return entries;
    }

    bool mIONativeFile::sourceMetadataTruncated() const
    {
        const native_readers::Dataset* dataset = activeDataset();
        return dataset != nullptr && dataset->source_metadata_truncated;
    }

    quint64 mIONativeFile::sourceMetadataRetainedBytes() const
    {
        const native_readers::Dataset* dataset = activeDataset();
        return dataset != nullptr ? dataset->retained_metadata_bytes : 0;
    }

    bool mIONativeFile::exportSourceMetadata(const QString& targetJsonPath, QString* error) const
    {
        const native_readers::Dataset* dataset = activeDataset();
        if (dataset == nullptr)
        {
            if (error != nullptr)
                *error = QStringLiteral("result data is not ready");
            return false;
        }
        try
        {
            native_readers::write_source_metadata(*dataset, toFileSystemPath(targetJsonPath));
            return true;
        }
        catch (const std::exception& failure)
        {
            if (error != nullptr && error->isEmpty())
                *error = QString::fromUtf8(failure.what());
            return false;
        }
    }

    bool mIONativeFile::readVarDatas(int frameIndex,
                                     const QString& variableName,
                                     const QString& componentName,
                                     const QString& complexMode,
                                     const QString& layerName) const
    {
        if (!_datasetReady)
        {
            qWarning() << "mIONativeFile::readVarDatas: dataset is not ready";
            return false;
        }
        Q_UNUSED(complexMode);

        NativeReadersDataBuilder builder;

        // d3plot 流式路径：当帧按需解码（会话 visit 已结束，read_frame 可安全随机
        // 访问；帧值不缓存，与全量路径"常驻内存摊平"相比多一次解码 IO）
        if (_d3plotSession)
        {
            const auto& directory = _d3plotSession->frames();
            const int datasetFrameIndex = frameIndex - 1;
            if (datasetFrameIndex < 0 ||
                datasetFrameIndex >= static_cast<int>(directory.size()))
            {
                qWarning() << "mIONativeFile::readVarDatas: frame" << frameIndex
                           << "is out of range";
                return false;
            }
            try
            {
                native_readers::Frame frame =
                    _d3plotSession->read_frame(static_cast<std::size_t>(datasetFrameIndex));
                return builder.readVarDatasFrame(_d3plotSession->model(), frame, _modeldb,
                                                 frameIndex, variableName, componentName,
                                                 layerName);
            }
            catch (const std::exception& error)
            {
                qWarning() << "mIONativeFile::readVarDatas: stream frame decode failed:"
                           << error.what();
                return false;
            }
        }

        return builder.readVarDatas(_dataset, _modeldb, frameIndex, variableName,
                                    componentName, layerName);
    }

    void mIONativeFile::run()
    {
        MxFunctions::mGlobalSignals* globalSignals = MxFunctions::mGlobalSignals::getInstance();
        const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
        globalSignals->modelDataThreadStatusChangedSig(true);
        globalSignals->setProgressBarValue(5);
        globalSignals->outputMessageSig(0,
            isChinese ? tr("正在读取结果文件%1...").arg(_resultFile)
                      : tr("Reading result file %1...").arg(_resultFile),
            true);

        //统一失败收尾：输出错误文案并复位界面置灰状态
        const auto emitNativeError = [this, globalSignals, isChinese](MIOFile::IOFileErrorType type, const QString& detail = QString())
        {
            const QString reason = translateNativeError(type, isChinese);
            QString adjustedDetail = detail;
            // OP2 伴随模型类错误的 GUI 行动指引：库的报错文案面向 CLI（supply --model），
            // 翻译成 GUI 语境的可操作提示，避免用户按 CLI 参数自行排障
            if (type == NativeParseFailed && !adjustedDetail.isEmpty() &&
                QFileInfo(_resultFile).suffix().compare(QStringLiteral("op2"), Qt::CaseInsensitive) == 0 &&
                adjustedDetail.contains(QStringLiteral("BDF"), Qt::CaseInsensitive))
            {
                adjustedDetail += isChinese
                    ? QStringLiteral("；提示：OP2 依赖伴随网格模型，请在同目录放置同名 .bdf/.nas（需拍平、不含 INCLUDE），或在导入时将 OP2 与 .bdf 一起多选")
                    : QStringLiteral("; hint: OP2 needs a companion mesh - place a same-named flattened .bdf/.nas next to it (no INCLUDE), or multi-select the OP2 together with its .bdf when importing");
            }
            QString message;
            if (adjustedDetail.isEmpty())
            {
                message = isChinese ? tr("读取结果文件%1失败：%2").arg(_resultFile, reason)
                                    : tr("Failed to read result file %1: %2").arg(_resultFile, reason);
            }
            else
            {
                message = isChinese ? tr("读取结果文件%1失败：%2（%3）").arg(_resultFile, reason, adjustedDetail)
                                    : tr("Failed to read result file %1: %2 (%3)").arg(_resultFile, reason, adjustedDetail);
            }
            globalSignals->outputMessageSig(2, message, true);
            globalSignals->modelDataThreadStatusChangedSig(false);
            emit readNativeErrorSig(type);
            emit finishedNativeFileSig(QString());
        };

        if (_modeldb == nullptr)
        {
            emitNativeError(DatabaseNull);
            return;
        }

        if (!QFile::exists(_resultFile))
        {
            emitNativeError(NativeFileNotExist);
            return;
        }

        try
        {
            native_readers::ReadOptions options;
            if (!_modelFile.isEmpty())
                options.model_path = toFileSystemPath(_modelFile);
            // 库默认文件尺寸上限（0.3.0 起为 16GiB，此前 4GiB）对长时程
            // d3plot/RST/ODB 仍偏小（超限抛 ReadError，界面只能报笼统的
            // "解析失败"），放宽到 1TiB；保留有限上限以维持对损坏文件的基本
            // 防护（库内各 reader 的记录级结构校验不受影响）
            options.max_file_bytes = 1ull << 40;

            // d3plot 主文件优先走 0.3.10 流式会话：会话仅常驻归一化模型 + 全状态
            // 目录 + 单帧解码窗口，长时程多状态文件的全部帧值不再驻留内存；流式
            // 仅支持未压缩已证布局，构造失败（ReadError）回退下方全量路径，行为
            // 不倒退。MIO_NATIVE_FORCE_FULL_D3PLOT=1 可强制全量（排障对照用）
            const bool isD3plotMainFile =
                QFileInfo(_resultFile).fileName().compare(QStringLiteral("d3plot"),
                                                          Qt::CaseInsensitive) == 0 &&
                qEnvironmentVariableIsEmpty("MIO_NATIVE_FORCE_FULL_D3PLOT");
            if (isD3plotMainFile)
            {
                native_readers::D3plotStreamOptions streamOptions;
                streamOptions.read = options;
                // 会话构造/逐帧解码的诊断与全量路径 diagnostics 同格式同通道
                streamOptions.read.diagnostic_sink =
                    [globalSignals](const native_readers::Diagnostic& diagnostic) {
                        globalSignals->outputMessageSig(
                            1,
                            QString::fromUtf8(
                                native_readers::format_diagnostic(diagnostic).c_str()),
                            false);
                    };
                // 取消源唯一：导入期间后处理模型被删（与全量路径中段复查同语义）
                streamOptions.should_cancel = [this] { return _modeldb.isNull(); };
                try
                {
                    _d3plotSession = std::make_unique<native_readers::D3plotReaderSession>(
                        toFileSystemPath(_resultFile), streamOptions);
                }
                catch (const native_readers::D3plotCancelled&)
                {
                    emitNativeError(DatabaseNull);
                    return;
                }
                catch (const std::exception& error)
                {
                    qWarning() << "mIONativeFile::run: d3plot stream unavailable, "
                                  "falling back to full read:"
                               << error.what();
                    _d3plotSession.reset();
                }
            }

            const bool streamed = _d3plotSession != nullptr;
            if (!streamed)
                _dataset = native_readers::read_result(toFileSystemPath(_resultFile), options);
            const native_readers::Dataset& dataset =
                streamed ? _d3plotSession->model() : _dataset;
            _datasetReady = true;
            globalSignals->setProgressBarValue(50);

            // 0.3.3 起库以统一诊断接口上报读取告警/错误（format_diagnostic 输出
            // "[severity][code] ...: message"）；warnings 为兼容保留、内容与
            // diagnostics 重复，仅在无结构化诊断时兜底输出，避免同一条目刷屏。
            // 流式会话的诊断已在构造/解码期经 diagnostic_sink 实时输出，此处
            // 再遍历 model().diagnostics 会整段重复，跳过
            if (streamed)
            {
                if (dataset.diagnostics.empty())
                {
                    for (const auto& warning : dataset.warnings)
                    {
                        globalSignals->outputMessageSig(1,
                            QString::fromUtf8(warning.c_str()), false);
                    }
                }
            }
            else if (!dataset.diagnostics.empty())
            {
                for (const auto& diagnostic : dataset.diagnostics)
                {
                    globalSignals->outputMessageSig(1,
                        QString::fromUtf8(native_readers::format_diagnostic(diagnostic).c_str()),
                        false);
                }
            }
            else
            {
                for (const auto& warning : dataset.warnings)
                {
                    globalSignals->outputMessageSig(1,
                        QString::fromUtf8(warning.c_str()), false);
                }
            }

            // 0.3.4 来源元信息超保留预算截断提醒：静默截断会让"来源信息"查看
            // 缺记录且无解释，与诊断同通道透出（保留条数/字节可对上右侧面板）
            if (dataset.source_metadata_truncated)
            {
                globalSignals->outputMessageSig(1,
                    isChinese
                        ? tr("来源元信息超出保留预算已截断（保留 %1 条 / %2 字节）")
                              .arg(dataset.source_metadata.size())
                              .arg(dataset.retained_metadata_bytes)
                        : tr("Source metadata exceeded the retention budget and was "
                             "truncated (kept %1 records / %2 bytes)")
                              .arg(dataset.source_metadata.size())
                              .arg(dataset.retained_metadata_bytes),
                    false);
            }

            //读取线程运行期间模型被删（如mtime重建删旧模型撞上未完成的导入）：
            //dataset 不再落库，按失败收尾（失败分支会移除reader并通知恢复协调器跳过）
            if (_modeldb == nullptr)
            {
                emitNativeError(DatabaseNull);
                return;
            }

            _modeldb->setFile(_resultFile);
            _modeldb->setDataFlag(false);

            NativeReadersDataBuilder builder;
            QString buildError;
            QStringList buildNotes;
            bool buildOk = false;
            if (streamed)
            {
                buildOk = builder.buildAllFromStream(dataset, _d3plotSession->frames(),
                                                     _d3plotSession->fields(), _modeldb,
                                                     &buildError, &buildNotes);
                if (buildOk)
                {
                    // 首帧首分量（云图初显）：read_frame(0) 取值发布，选择口径与
                    // 全量路径 buildResultFrames 尾部一致（Mag > vonMises > 首分量）
                    QString firstName;
                    QString firstComponent;
                    if (NativeReadersDataBuilder::selectFirstDisplayableVariable(
                            _d3plotSession->fields(), &firstName, &firstComponent))
                    {
                        try
                        {
                            native_readers::Frame firstFrame = _d3plotSession->read_frame(0);
                            if (!builder.readVarDatasFrame(dataset, firstFrame, _modeldb, 1,
                                                           firstName, firstComponent))
                            {
                                buildOk = false;
                                buildError = QStringLiteral(
                                    "first result publish failed (variable '%1', "
                                    "component '%2')").arg(firstName, firstComponent);
                            }
                        }
                        catch (const native_readers::D3plotCancelled&)
                        {
                            emitNativeError(DatabaseNull);
                            return;
                        }
                        catch (const std::exception& error)
                        {
                            buildOk = false;
                            buildError = QString::fromUtf8(error.what());
                        }
                    }
                }
                if (buildOk)
                {
                    // 逐帧轻校验全部保存状态（对齐旧全量路径：坏帧在导入期暴露，
                    // 而非用户翻到该帧才报错）；帧值即弃，进度按帧线性推进。
                    // buildOk 成立意味着目录非空（空目录已在注册阶段失败）
                    const std::size_t frameCount = _d3plotSession->frames().size();
                    std::size_t visited = 0;
                    native_readers::D3plotStreamCallbacks callbacks;
                    callbacks.on_frame = [&](std::size_t, const native_readers::Frame&) {
                        ++visited;
                        globalSignals->setProgressBarValue(
                            50 + static_cast<int>(40 * visited / frameCount));
                        return true;
                    };
                    try
                    {
                        const auto visitResult = _d3plotSession->visit(callbacks);
                        if (visitResult.status != native_readers::D3plotStreamStatus::complete)
                        {
                            emitNativeError(DatabaseNull);
                            return;
                        }
                    }
                    catch (const native_readers::D3plotCancelled&)
                    {
                        emitNativeError(DatabaseNull);
                        return;
                    }
                    catch (const std::exception& error)
                    {
                        emitNativeError(NativeParseFailed, QString::fromUtf8(error.what()));
                        return;
                    }
                }
            }
            else
            {
                buildOk = builder.buildAll(dataset, _modeldb, &buildError, &buildNotes);
            }
            if (!buildOk)
            {
                emitNativeError(NativeUnsupportedLayout, buildError);
                return;
            }
            // 部分降级（如跳过不支持类型的单元）：与库 warnings 同通道提醒，
            // 用户可见"模型看起来缺单元"的原因
            for (const QString& note : buildNotes)
            {
                globalSignals->outputMessageSig(1,
                    isChinese ? tr("部分数据不支持已跳过：%1").arg(note)
                              : tr("Some unsupported data was skipped: %1").arg(note),
                    false);
            }
            globalSignals->setProgressBarValue(90);

            const QString postName = _modeldb->getName();
            if (isChinese)
                globalSignals->outputMessageSig(0, tr("成功读取结果文件%1！").arg(_resultFile));
            else
                globalSignals->outputMessageSig(0, tr("The result file %1 was successfully read!").arg(_resultFile));
            emit readFileSuccessSig(postName);
            globalSignals->modelDataThreadStatusChangedSig(false);
            emit finishedNativeFileSig(postName);
        }
        catch (const std::exception& error)
        {
            qWarning() << "mIONativeFile::run: parse failed:" << error.what();
            emitNativeError(NativeParseFailed, QString::fromUtf8(error.what()));
        }
    }
}
