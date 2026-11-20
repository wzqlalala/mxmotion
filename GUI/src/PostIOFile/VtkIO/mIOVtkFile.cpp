#include "mIOVtkFile.h"

#include "VtkDataBuilder.h"
#include "mPostDataBase.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include <QCollator>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <filesystem>

namespace MIOFile
{
    namespace
    {
        std::filesystem::path toFileSystemPath(const QString& value)
        {
            return std::filesystem::path(value.toStdWString());
        }

        QString translateVtkError(MIOFile::IOFileErrorType type, bool isChinese)
        {
            switch (type)
            {
            case DatabaseNull:
                return isChinese ? QStringLiteral("数据库不存在！") : QStringLiteral("The database does not exist!");
            case FileNotExist:
                return isChinese ? QStringLiteral("结果文件不存在！") : QStringLiteral("The result file does not exist!");
            case FileNotOpen:
                return isChinese ? QStringLiteral("VTK 文件解析失败！") : QStringLiteral("Failed to parse the VTK file!");
            case ModelMismatching:
                return isChinese ? QStringLiteral("VTK 文件数据不受支持（结构化网格/.pvd 集合/拓扑不一致）！")
                                 : QStringLiteral("The VTK file contains unsupported data (structured grid, .pvd collection or inconsistent topology)!");
            default:
                return isChinese ? QStringLiteral("未知错误！") : QStringLiteral("Unknown error!");
            }
        }
    }

    mIOVtkFile::mIOVtkFile(const QStringList& resultFiles,
                           QPointer<PostData::mPostDataBase> modeldb,
                           QObject* parent)
        : QThread(parent), _resultFiles(sortAsSeries(resultFiles)), _modeldb(modeldb)
    {
    }

    mIOVtkFile::~mIOVtkFile()
    {
        if (isRunning())
            wait();
    }

    bool mIOVtkFile::isVtkFile(const QString& fileName)
    {
        const QString suffix = QFileInfo(fileName.trimmed()).suffix().toLower();
        return suffix == QStringLiteral("vtk") ||
               suffix == QStringLiteral("vtp") ||
               suffix == QStringLiteral("vtu") ||
               suffix == QStringLiteral("vtm") ||
               suffix == QStringLiteral("pvtp") ||
               suffix == QStringLiteral("pvtu");
    }

    bool mIOVtkFile::tryGetVtkSelection(const QStringList& fileNames, QStringList* vtkFiles)
    {
        QStringList collected;
        for (const QString& fileName : fileNames)
        {
            if (fileName.trimmed().isEmpty())
                continue;
            if (isVtkFile(fileName))
                collected.append(QDir::cleanPath(QFileInfo(fileName.trimmed()).absoluteFilePath()));
        }
        if (collected.isEmpty())
            return false;
        if (vtkFiles != nullptr)
            *vtkFiles = collected;
        return true;
    }

    QStringList mIOVtkFile::sortAsSeries(const QStringList& fileNames)
    {
        QStringList sorted = fileNames;
        QCollator collator;
        collator.setNumericMode(true);
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(sorted.begin(), sorted.end(), [&collator](const QString& left, const QString& right)
        {
            const int byName = collator.compare(QFileInfo(left).fileName(), QFileInfo(right).fileName());
            if (byName != 0)
                return byName < 0;
            return collator.compare(left, right) < 0;
        });
        return sorted;
    }

    QString mIOVtkFile::getPostDataName() const
    {
        return _modeldb ? _modeldb->getName() : QString();
    }

    bool mIOVtkFile::readVarDatas(int frameIndex,
                                  const QString& variableName,
                                  const QString& componentName,
                                  const QString& complexMode,
                                  const QString& layerName) const
    {
        if (!_datasetReady)
        {
            qWarning() << "mIOVtkFile::readVarDatas: dataset is not ready";
            return false;
        }
        Q_UNUSED(complexMode);
        Q_UNUSED(layerName);

        VtkDataBuilder builder;
        return builder.readVarDatas(_datasets, _modeldb, frameIndex, variableName, componentName);
    }

    void mIOVtkFile::run()
    {
        MxFunctions::mGlobalSignals* globalSignals = MxFunctions::mGlobalSignals::getInstance();
        const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
        globalSignals->modelDataThreadStatusChangedSig(true);
        globalSignals->setProgressBarValue(5);
        globalSignals->outputMessageSig(0,
            isChinese ? tr("正在读取VTK结果文件（共%1帧）...").arg(_resultFiles.size())
                      : tr("Reading VTK result files (%1 frames)...").arg(_resultFiles.size()),
            true);

        //统一失败收尾：输出错误文案并复位界面置灰状态
        const auto emitVtkError = [this, globalSignals, isChinese](MIOFile::IOFileErrorType type, const QString& detail = QString())
        {
            const QString reason = translateVtkError(type, isChinese);
            QString message;
            if (detail.isEmpty())
            {
                message = isChinese ? tr("读取VTK结果文件失败：%1").arg(reason)
                                    : tr("Failed to read VTK result files: %1").arg(reason);
            }
            else
            {
                message = isChinese ? tr("读取VTK结果文件失败：%1（%2）").arg(reason, detail)
                                    : tr("Failed to read VTK result files: %1 (%2)").arg(reason, detail);
            }
            globalSignals->outputMessageSig(2, message, true);
            globalSignals->modelDataThreadStatusChangedSig(false);
            emit readVtkErrorSig(type);
            emit finishedVtkFileSig(QString());
        };

        if (_modeldb == nullptr)
        {
            emitVtkError(DatabaseNull);
            return;
        }

        if (_resultFiles.isEmpty())
        {
            emitVtkError(FileNotExist);
            return;
        }
        for (const QString& file : _resultFiles)
        {
            if (!QFile::exists(file))
            {
                emitVtkError(FileNotExist, file);
                return;
            }
        }

        // .pvd 集合明确不支持（后缀即可判定，避免读入后才发现）
        for (const QString& file : _resultFiles)
        {
            if (QFileInfo(file).suffix().toLower() == QStringLiteral("pvd"))
            {
                emitVtkError(ModelMismatching,
                             isChinese ? QStringLiteral("%1：暂不支持 .pvd 集合文件，请多选各时间步文件导入").arg(file)
                                       : QStringLiteral("%1: .pvd collection files are not supported; multi-select the per-step files instead").arg(file));
                return;
            }
        }

        // 逐文件读取（一文件一帧，帧序 = 已排序列表顺序）；任一失败整组中止
        _datasets.clear();
        const int totalFiles = _resultFiles.size();
        for (int index = 0; index < totalFiles; ++index)
        {
            auto result = vfr::read_file(toFileSystemPath(_resultFiles.at(index)));
            if (!result)
            {
                const vfr::Error& error = result.error();
                emitVtkError(error.code == vfr::ErrorCode::UnsupportedFeature ? ModelMismatching : FileNotOpen,
                             QStringLiteral("%1: [%2] %3")
                                 .arg(_resultFiles.at(index))
                                 .arg(QString::fromLatin1(vfr::to_string(error.code)))
                                 .arg(QString::fromStdString(error.message)));
                return;
            }
            _datasets.push_back(std::move(result.value()));
            globalSignals->setProgressBarValue(5 + 45 * (index + 1) / totalFiles);
        }
        _datasetReady = true;

        // 读取线程运行期间模型被删：数据不再落库，按失败收尾
        if (_modeldb == nullptr)
        {
            emitVtkError(DatabaseNull);
            return;
        }

        _modeldb->setFile(_resultFiles.first());
        _modeldb->setDataFlag(false);

        VtkDataBuilder builder;
        QString buildError;
        QStringList buildNotes;
        if (!builder.buildAll(_datasets, _resultFiles, _modeldb, &buildError, &buildNotes))
        {
            emitVtkError(ModelMismatching, buildError);
            return;
        }
        // 部分降级（如跳过多边形/三角带单元）：与 native 链路同通道提醒
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
            globalSignals->outputMessageSig(0, tr("成功读取VTK结果文件%1！").arg(_resultFiles.first()));
        else
            globalSignals->outputMessageSig(0, tr("The VTK result files %1 were successfully read!").arg(_resultFiles.first()));
        emit readFileSuccessSig(postName);
        globalSignals->modelDataThreadStatusChangedSig(false);
        emit finishedVtkFileSig(postName);
    }
}
