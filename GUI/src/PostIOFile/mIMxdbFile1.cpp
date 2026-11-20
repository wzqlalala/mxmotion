#include "mIMxdbFile1.h"

#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QVector>
#include <QVector3D>
#include <QMutex>
#include <QtConcurrent/QtConcurrent>
#include <math.h>
#include <QSet>
#include <QHash>
#include <QApplication>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "MxdbReaderCore/MxdbReaderBase.h"
#include "MxdbReaderCore/ProductLineMxdbReaders.h"
#include "MxdbReaderCore/NewSingleMxdbReader.h"
#include "MxdbReaderCore/NewFormatMxdbReader.h"
#include "MxdbReader/MxdbDataBuilder.h"
#include "MxdbReaderCore/MxdbModelData.h"

#include "mElementFunction.h"
#include "mIOFileManager.h"
#include "mBasicEnum.h"
#include "mGlobalSignals.h"
#include "mElementFunction.h"
#include "mAppConfiguration.h"
#include "mPostParseMesh.h"
#include "mPostRigidWallData.h"
#include "LowCPlusDefine.h"

#include "mDataPost1.h"
#include "mOneFrameData1.h"
#include "mPostMeshPartData1.h"
#include "mPostMeshData1.h"
#include "mPostMeshFaceData1.h"
#include "mPostMeshLineData1.h"
#include "mPostMeshNodeData1.h"
#include "mPostOneFrameRendData.h"
#include "mPostAnimationRendData.h"

#define optimazation false

#include <QDateTime>
#include <QMatrix4x4>
#include "mOtherFunction.h"

//解决中文乱码
#pragma execution_character_set("utf-8")

using namespace MxFunctions;
using namespace std;
using namespace MIOFile;
using namespace mxr;
using namespace PostData;
using namespace MechPreData;

QMutex postmutex;
namespace MIOFile
{
    using namespace MxdbReader;

    mIMxdbFile1::mIMxdbFile1()
    {
        _oneFrameRender = new mPostOneFrameRendData;
        _oneFrameRender = nullptr;
        _productLine = 0;  // 初始化为0，确保createMxdbReaderForFile中重新检测
        //setAutoDelete(true);
    }

    mIMxdbFile1::mIMxdbFile1(const QString& initfile, const QStringList& filenames, QPointer<mPostDataBase> modeldb, QObject* parent) :QThread(parent)
    {
        _oneFrameRender = new mPostOneFrameRendData;
        _initFileName = initfile;
        _frameFiles = filenames;
        _modeldb = modeldb;
        _oneFrameRender = nullptr;

        QFileInfo finfo(initfile);
        if (finfo.exists())
        {
            _fileSize = finfo.size();
        }

        //setAutoDelete(true);
    }

    mIMxdbFile1::~mIMxdbFile1() {}

    QString mIMxdbFile1::getPostDataName()
    {
        return _postDataName;
    }

    void mIMxdbFile1::setOneFrameRenderData(mPostOneFrameRendData* oneframe)
    {
        _oneFrameRender = oneframe;
    }

    mPostOneFrameRendData* mIMxdbFile1::getOneFrameRenderData()
    {
        return _oneFrameRender;
    }

    void mIMxdbFile1::setOutputFile(bool flag)
    {
        _outputFile = flag;
    }

    void mIMxdbFile1::setAIStatus(bool flag)
    {
        _isAI = flag;
    }

    QVector3D mIMxdbFile1::transVector(const QVector3D& x, const QVector3D& a, const QVector3D& b, const QVector3D& c)
    {
        QVector3D i = b - a;
        QVector3D j = c - a;
        QVector3D k = i.crossProduct(i, j);        // 右手坐标系，如果是左手调换i、j位置

        QMatrix4x4 A(i.x(), j.x(), k.x(), 0.0,
            i.y(), j.y(), k.y(), 0.0,
            i.z(), j.z(), k.z(), 0.0,
            0.0, 0.0, 0.0, 1.0);
        A = A.inverted();

        QVector4D x4(x, 1.0);
        x4 = A * x4;

        return x4.toVector3D();
    }

    struct MessageThrottler {
        QHash<int, int> count;
        QHash<int, qint64> lastSentMs;

        bool shouldSend(MxdbMessageCode code) {
            const int key = static_cast<int>(code);
            ++count[key];
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (code == MxdbMessageCode::ElementTypeUnknown || code == MxdbMessageCode::ElementTypeEmpty) {
                if (count[key] == 1 || count[key] % 100 == 0 || (now - lastSentMs.value(key, 0)) > 500) {
                    lastSentMs[key] = now;
                    return true;
                }
                return false;
            }
            lastSentMs[key] = now;
            return true;
        }

        QString appendCount(MxdbMessageCode code, const QString& msg) {
            if (code == MxdbMessageCode::ElementTypeUnknown || code == MxdbMessageCode::ElementTypeEmpty) {
                const int total = count.value(static_cast<int>(code), 0);
                if (total > 1) {
                    return msg + QStringLiteral(" (已累计%1次)").arg(total);
                }
            }
            return msg;
        }
    };

    static QString translateMxdbMessage(MxdbMessageCode code, const std::vector<std::string>& args)
    {
        const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
        switch (code) {
        case MxdbMessageCode::ReadingModelData:
            return isChinese ? QStringLiteral("正在读取模型数据...") : QStringLiteral("Reading model data...");
        case MxdbMessageCode::ReadingResultFrameVariables:
            return isChinese ? QStringLiteral("正在读取结果帧变量信息...") : QStringLiteral("Reading variable information from result frame...");
        case MxdbMessageCode::ResultFrameReadSuccess:
            return isChinese ? QStringLiteral("结果帧文件数据读取成功！") : QStringLiteral("The result frame file data is read successfully!");
        case MxdbMessageCode::FileNotFound:
            return isChinese ? QString("文件%1不存在！").arg(QString::fromStdString(args.empty() ? std::string() : args[0])) : QString("File %1 does not exist!").arg(QString::fromStdString(args.empty() ? std::string() : args[0]));
        case MxdbMessageCode::FileOpenFailed:
            return isChinese ? QString("文件%1打开失败！").arg(QString::fromStdString(args.empty() ? std::string() : args[0])) : QString("File %1 open failed!").arg(QString::fromStdString(args.empty() ? std::string() : args[0]));
        case MxdbMessageCode::FileVersionInvalid:
            return isChinese ? QString("文件%1版本错误！").arg(QString::fromStdString(args.empty() ? std::string() : args[0])) : QString("File %1 version error!").arg(QString::fromStdString(args.empty() ? std::string() : args[0]));
        case MxdbMessageCode::NodeCountZero:
            return isChinese ? QStringLiteral("节点总数为0！") : QStringLiteral("The total number of nodes is 0!");
        case MxdbMessageCode::PartCountZero:
            return isChinese ? QStringLiteral("部件总数为0！") : QStringLiteral("The total part number of nodes is 0!");
        case MxdbMessageCode::PartCount:
            return isChinese ? QString("部件数：%1！").arg(QString::fromStdString(args.empty() ? std::string() : args[0])) : QString("Part Number:%1").arg(QString::fromStdString(args.empty() ? std::string() : args[0]));
        case MxdbMessageCode::ReadingPartInfo:
            return isChinese ? QString("正在读取零件%1信息！").arg(QString::fromStdString(args.empty() ? std::string() : args[0])) : QString("Reading information for part %1!").arg(QString::fromStdString(args.empty() ? std::string() : args[0]));
        case MxdbMessageCode::ElementTypeUnknown:
            return isChinese ? QString("不存在的单元类型为 (%1); 第%2个零件(%3)的第%4个单元")
                .arg(QString::fromStdString(args.size() > 0 ? args[0] : std::string()))
                .arg(QString::fromStdString(args.size() > 1 ? args[1] : std::string()))
                .arg(QString::fromStdString(args.size() > 2 ? args[2] : std::string()))
                .arg(QString::fromStdString(args.size() > 3 ? args[3] : std::string()))
                : QString("Unknown element type (%1); part %2 (%3), element %4")
                .arg(QString::fromStdString(args.size() > 0 ? args[0] : std::string()))
                .arg(QString::fromStdString(args.size() > 1 ? args[1] : std::string()))
                .arg(QString::fromStdString(args.size() > 2 ? args[2] : std::string()))
                .arg(QString::fromStdString(args.size() > 3 ? args[3] : std::string()));
        case MxdbMessageCode::ElementTypeEmpty:
            return isChinese ? QString("单元类型为空; 第%1个零件(%2)的第%3个单元")
                .arg(QString::fromStdString(args.size() > 0 ? args[0] : std::string()))
                .arg(QString::fromStdString(args.size() > 1 ? args[1] : std::string()))
                .arg(QString::fromStdString(args.size() > 2 ? args[2] : std::string()))
                : QString("Element type is empty; part %1 (%2), element %3")
                .arg(QString::fromStdString(args.size() > 0 ? args[0] : std::string()))
                .arg(QString::fromStdString(args.size() > 1 ? args[1] : std::string()))
                .arg(QString::fromStdString(args.size() > 2 ? args[2] : std::string()));
        case MxdbMessageCode::CountOutOfRange:
            return isChinese ? QString("结果文件数据块%1的数量%2非法（超出文件剩余字节），文件可能已损坏！")
                .arg(QString::fromStdString(args.size() > 0 ? args[0] : std::string()))
                .arg(QString::fromStdString(args.size() > 1 ? args[1] : std::string()))
                : QString("Result file block %1 count %2 is invalid (exceeds remaining file bytes); the file may be corrupted!")
                .arg(QString::fromStdString(args.size() > 0 ? args[0] : std::string()))
                .arg(QString::fromStdString(args.size() > 1 ? args[1] : std::string()));
        default:
            // 库内通用告警（code=None）的文案放在 args[0]：透出给输出窗口，避免
            // 集合块错位、objectCount 语义兼容等告警被静默吞掉
            if (!args.empty() && !args[0].empty())
                return QString::fromStdString(args[0]);
            return QString();
        }
    }

    // 损坏文件防御类报错：按当前界面语言输出到输出窗口（错误级别）
    static void reportCorruptFileReadMessage(const QString& chinese, const QString& english)
    {
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            mGlobalSignals::getInstance()->outputMessageSig(2, chinese, true);
        else
            mGlobalSignals::getInstance()->outputMessageSig(2, english, true);
    }

    // 当调用者没有设置产品线时，根据文件内容自动判断。
    // 对于 v05/v06，显式使用单精度坐标，隐式/电机使用双精度坐标，
    // 通过比较两种精度下解析出的 partCount 与第一个部件名的有效性来决策。
    static int detectProductLine(const QString& fname)
    {
        QFile file(fname);
        if (!file.open(QIODevice::ReadOnly))
            return 0;

        uint32_t version = 0;
        file.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));
        // 只有 v05/v06 需要通过精度区分产品线；更早版本统一走隐式（双精度）
        if (version < 0xA1B1C105 || version > 0xA1B1C106)
            return 0;

        const qint64 fileSize = file.size();
        file.seek(4); // skip version

        int nodeCount = 0;
        file.read(reinterpret_cast<char*>(&nodeCount), sizeof(int));
        if (nodeCount <= 0 || nodeCount > 100000000) return 0;

        const qint64 floatPos = 8 + static_cast<qint64>(nodeCount) * (4 + 3 * 4);
        const qint64 doublePos = 8 + static_cast<qint64>(nodeCount) * (4 + 3 * 8);

        auto isValidPartName = [](QFile& stream, qint64 pos, qint64 fileSize) -> bool {
            if (pos < 0 || pos + 4 + 64 > fileSize) return false;
            stream.seek(pos);
            int partCount = 0;
            stream.read(reinterpret_cast<char*>(&partCount), sizeof(int));
            if (partCount <= 0 || partCount > 100000) return false;
            char name[64] = {};
            stream.read(name, 64);
            // 要求前几个字节为可见 ASCII/GBK 或英文，且不全为 0
            bool hasNonZero = false;
            bool allPrintable = true;
            for (int i = 0; i < 64; ++i) {
                if (name[i] != '\0') hasNonZero = true;
                // 允许可见 ASCII 和中文字节（>127）；空字节只出现在末尾
                if (name[i] != '\0' && static_cast<unsigned char>(name[i]) < 32 &&
                    static_cast<unsigned char>(name[i]) != 0x09) {
                    allPrintable = false;
                    break;
                }
            }
            return hasNonZero && allPrintable;
            };

        const bool floatValid = isValidPartName(file, floatPos, fileSize);
        const bool doubleValid = isValidPartName(file, doublePos, fileSize);

        if (floatValid && !doubleValid) return 1;      // 显式
        if (!floatValid && doubleValid) return 0;      // 隐式/电机（保持默认）
        if (floatValid && doubleValid) {
            // 两种精度都解析出有效结构，取 partCount 较小/更合理的那个
            file.seek(floatPos);
            int floatPartCount = 0;
            file.read(reinterpret_cast<char*>(&floatPartCount), sizeof(int));
            file.seek(doublePos);
            int doublePartCount = 0;
            file.read(reinterpret_cast<char*>(&doublePartCount), sizeof(int));
            return floatPartCount <= doublePartCount ? 1 : 0;
        }
        return 0;
    }

    // detectProductLine 需开文件按两种坐标精度试算；按需读取（点击云图节点等）每次
    // 新建 mIMxdbFile1 实例，实例级 _productLine 防重不生效，故按文件路径做进程级缓存，
    // 并以 size/mtime 校验兜底同名文件被覆盖的情况。
    struct ProductLineCacheEntry
    {
        int productLine = 0;
        qint64 size = -1;
        qint64 mtimeMSecs = 0;
    };
    QHash<QString, ProductLineCacheEntry> g_productLineCacheByFile;
    QMutex g_productLineCacheMutex;

    static bool lookupProductLineCache(const QString& fname, int& productLineOut)
    {
        QFileInfo fi(fname);
        if (!fi.exists())
            return false;
        QMutexLocker lock(&g_productLineCacheMutex);
        const auto it = g_productLineCacheByFile.constFind(fname);
        if (it == g_productLineCacheByFile.constEnd())
            return false;
        if (it->size != fi.size() || it->mtimeMSecs != fi.lastModified().toMSecsSinceEpoch())
            return false;
        productLineOut = it->productLine;
        return true;
    }

    static void storeProductLineCache(const QString& fname, int productLine)
    {
        QFileInfo fi(fname);
        if (!fi.exists())
            return;
        QMutexLocker lock(&g_productLineCacheMutex);
        g_productLineCacheByFile[fname] = { productLine, fi.size(), fi.lastModified().toMSecsSinceEpoch() };
    }

    static int detectProductLineCached(const QString& fname)
    {
        if (fname.isEmpty())
            return 0;
        int cached = 0;
        if (lookupProductLineCache(fname, cached))
            return cached;
        const int productLine = detectProductLine(fname);
        storeProductLineCache(fname, productLine);
        return productLine;
    }

    std::unique_ptr<MxdbReaderBase> mIMxdbFile1::createMxdbReader(std::ifstream& fs, uint32_t version) const
    {
        if (version == 0xA1B1C201 || version == 0xA1B1C202)
            return std::make_unique<NewSingleMxdbReader>(fs);
        // 新版记录式格式（601/602）与产品线无关，直接构造（否则 fileName 为空时
        // version 透传断链会误入产品线 reader）
        if (version == 0xA1B1C601 || version == 0xA1B1C602)
            return std::make_unique<NewFormatMxdbReader>(fs);
        return createMxdbReaderForFile(fs, QString());
    }

    std::unique_ptr<MxdbReaderBase> mIMxdbFile1::createMxdbReaderForFile(std::ifstream& fs, const QString& fileName) const
    {
        int productLine = _productLine;
        // 按需读取时若未设置产品线，根据初始帧自动判断（显式 v05+ 为单精度）。
        if (productLine == 0)
        {
            // 优先使用导入时一起传入的初始帧文件名。
            QString initFileName = _initFileName;
            if (!initFileName.isEmpty())
            {
                productLine = detectProductLineCached(initFileName);
            }

            // 兜底：仍用当前文件做一次启发式判断（对.mxdb0有效）
            if (productLine == 0 && !fileName.isEmpty())
            {
                productLine = detectProductLineCached(fileName);
            }
        }

        uint32_t version = 0;
        if (!fileName.isEmpty())
        {
            std::ifstream versionStream;
#ifdef _WIN32
            versionStream.open(fileName.toStdWString(), std::ios::in | std::ios::binary);
#else
            versionStream.open(fileName.toLocal8Bit().constData(), std::ios::in | std::ios::binary);
#endif
            if (versionStream)
            {
                int v = 0;
                versionStream.read(reinterpret_cast<char*>(&v), sizeof(int));
                version = static_cast<uint32_t>(v);
            }
        }

        std::unique_ptr<MxdbReaderBase> reader;
        if (version == 0xA1B1C201 || version == 0xA1B1C202) {
            reader = std::make_unique<NewSingleMxdbReader>(fs);
        }
        else {
            MxdbProductLine line = MxdbProductLine::Implicit;
            if (productLine == 1)
                line = MxdbProductLine::Explicit;
            else if (productLine == 2)
                line = MxdbProductLine::Motor;
            reader = MxdbReader::createMxdbReader(fs, line, version);
        }

        static MessageThrottler throttler;
        reader->setMessageCallback([](MxdbMessageLevel level, MxdbMessageCode code, const std::vector<std::string>& args) {
            // 不输出“正在读取每个部件信息”的提示，避免部件多时刷屏
            if (level == MxdbMessageLevel::Info && code == MxdbMessageCode::ReadingPartInfo) return;

            const QString msg = translateMxdbMessage(code, args);
            if (msg.isEmpty()) return;

            // 对按单元触发的错误消息进行限流/聚合
            if (level == MxdbMessageLevel::Error &&
                (code == MxdbMessageCode::ElementTypeUnknown || code == MxdbMessageCode::ElementTypeEmpty)) {
                if (!throttler.shouldSend(code)) return;
                const QString formatted = throttler.appendCount(code, msg);
                mGlobalSignals::getInstance()->outputMessageSig(2, formatted, true);
                return;
            }

            int messageType = 0;
            if (level == MxdbMessageLevel::Warning)
                messageType = 1;
            else if (level == MxdbMessageLevel::Error)
                messageType = 2;
            mGlobalSignals::getInstance()->outputMessageSig(messageType, msg, true);
            });

        return reader;
    }

    MxdbReader::MxdbVariable mIMxdbFile1::convertToMxdbVariable(const PostData::PostVariableInfo& info) const
    {
        MxdbReader::MxdbVariable var;
        var.resultType = 0;
        var.name = info._varName.toStdString();
        var.varType = info._varType;
        var.componentCount = info._nCompo;
        var.isMag = info._isMag;
        var.varByte = info._varByte;

        const int shapeCount = info._varCount;
        var.shapes.resize(shapeCount);
        for (int s = 0; s < shapeCount; ++s) {
            const int layerCount = info._varLayer.at(s);
            var.shapes[s].layers.resize(layerCount);
            for (int l = 0; l < layerCount; ++l) {
                if (l < info._varLayerNames1.at(s).size())
                    var.shapes[s].layers[l].name = info._varLayerNames1.at(s).at(l).toStdString();
            }
        }

        const int compoCount = info._compoNames.size();
        var.components.resize(compoCount);
        for (int c = 0; c < compoCount; ++c) {
            var.components[c].name = info._compoNames.at(c).toStdString();
            var.components[c].isComplex = info._compoComplex.value(info._compoNames.at(c), false);
        }

        return var;
    }

    MxdbReader::MxdbReaderBase* mIMxdbFile1::ensureFrameReader(const QString& frameFile)
    {
        if (_frameReader && _frameReaderFile == frameFile)
            return _frameReader.get();
        std::unique_ptr<MxdbReaderBase> reader = createMxdbReaderForFile(_frameStream, frameFile);
        if (!reader || !reader->open(frameFile.toStdWString()))
        {
            _frameReader.reset();
            _frameReaderFile.clear();
            return nullptr;
        }
        _frameReader = std::move(reader);
        _frameReaderFile = frameFile;
        return _frameReader.get();
    }

    bool mIMxdbFile1::readVariableValuesHelper(
        const QString& frameFile,
        const PostData::PostVariableInfo& varInfo,
        const std::vector<int>& entityIds,
        std::vector<MxdbReader::MxdbVariableValue>& outValues)
    {
        MxdbReaderBase* reader = ensureFrameReader(frameFile);
        if (reader == nullptr) {
            qWarning() << "mIMxdbFile1::readVariableValuesHelper: failed to open" << frameFile;
            return false;
        }

        // 导入时解析的元信息已含值区偏移（_varByte），按偏移直接读值，
        // 避免每次按需读取都 readResultFrame 全帧重解析（与 readElementNodeFaceValuesHelper 同一模式）
        if (varInfo._varByte > 0) {
            MxdbReader::MxdbVariable var = convertToMxdbVariable(varInfo);
            try {
                outValues = reader->readVariableValues(var, entityIds);
            } catch (const std::exception& e) {
                // 损坏文件可抛 bad_alloc 等（垃圾 entityCount 触发超大分配）；判损坏报错而非让按需读取崩溃
                qWarning() << "mIMxdbFile1::readVariableValuesHelper: readVariableValues exception:" << e.what();
                reportCorruptFileReadMessage(
                    QString("变量%1数值段读取异常（%2），文件可能已损坏！").arg(varInfo._varName).arg(e.what()),
                    QString("Variable %1 value block read exception (%2); the file may be corrupted!").arg(varInfo._varName).arg(e.what()));
                return false;
            }
            if (!outValues.empty())
                return true;
            outValues.clear();
        }

        // 兜底：元信息缺失或按偏移读取未得到数据时，回退全帧解析定位变量
        MxdbResultFrame frame;
        try {
            frame = reader->readResultFrame();
        } catch (const std::exception& e) {
            qWarning() << "mIMxdbFile1::readVariableValuesHelper: readResultFrame exception:" << e.what();
            return false;
        }
        const MxdbReader::MxdbVariable* matchedVar = nullptr;
        for (const auto& v : frame.variables) {
            if (QString::fromStdString(v.name) == varInfo._varName) {
                matchedVar = &v;
                break;
            }
        }

        if (!matchedVar) {
            qWarning() << "[readVariableValuesHelper] variable not found in frame:" << varInfo._varName;
            return false;
        }

        try {
            outValues = reader->readVariableValues(*matchedVar, entityIds);
        } catch (const std::exception& e) {
            // 损坏文件可抛 bad_alloc 等（垃圾 entityCount 触发超大分配）；判损坏报错而非让按需读取崩溃
            qWarning() << "mIMxdbFile1::readVariableValuesHelper: readVariableValues fallback exception:" << e.what();
            reportCorruptFileReadMessage(
                QString("变量%1数值段读取异常（%2），文件可能已损坏！").arg(varInfo._varName).arg(e.what()),
                QString("Variable %1 value block read exception (%2); the file may be corrupted!").arg(varInfo._varName).arg(e.what()));
            return false;
        }
        return true;
    }

    int mIMxdbFile1::elementTypeToComVarCount(MxFunctions::ElementType eleType) const
    {
        QString str = mElementFunction::getElementMainType(eleType);
        if (str == QString("梁") || str == QString("杆") || str == QString("壳"))
            return 6;
        if (str == QString("平面"))
            return 2;
        if (str == QString("实体") || str == QString("实体壳") || str == QString("膜") || str == QString("三维实体"))
            return 3;
        if (str == QString("剪切板"))
            return 5;
        return 0;
    }

    bool mIMxdbFile1::readElementNodeFaceValuesHelper(
        const QString& frameFile,
        const PostData::PostVariableInfo& varInfo,
        PostData::mPostModelData* modelData,
        const std::vector<int>& elementIds,
        std::vector<MxdbReader::MxdbElementNodeFaceValue>& outValues)
    {
        if (modelData == nullptr)
            return false;

        MxdbReaderBase* reader = ensureFrameReader(frameFile);
        if (reader == nullptr) {
            return false;
        }

        MxdbReader::MxdbVariable var = convertToMxdbVariable(varInfo);
        auto elementInfoProvider = [modelData, this](int elementId) {
            MxdbReader::MxdbElementNodeFaceInfo info;
            MxFunctions::ElementType eleType = modelData->getEleTypeByEleId(elementId);
            info.comVarCount = elementTypeToComVarCount(eleType);
            QVector<int> nodeIds = modelData->getNodeIdsByEleId(elementId);
            info.nodeCount = nodeIds.size();
            info.nodeIds.reserve(static_cast<size_t>(nodeIds.size()));
            for (int id : nodeIds)
                info.nodeIds.push_back(id);
            return info;
            };
        try {
            outValues = reader->readElementNodeFaceValues(var, elementInfoProvider, elementIds);
        } catch (const std::exception& e) {
            // 损坏文件可抛 bad_alloc 等（垃圾数量字段触发超大分配）；判损坏报错而非让按需读取崩溃
            qWarning() << "mIMxdbFile1::readElementNodeFaceValuesHelper: readElementNodeFaceValues exception:" << e.what();
            reportCorruptFileReadMessage(
                QString("变量%1单元节点面数值段读取异常（%2），文件可能已损坏！").arg(varInfo._varName).arg(e.what()),
                QString("Variable %1 element-node-face value block read exception (%2); the file may be corrupted!").arg(varInfo._varName).arg(e.what()));
            return false;
        }
        return true;
    }


    void mIMxdbFile1::slot_importAllMxDbFile()
    {
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);

        QFileInfo inifinfo(_initFileName);
        QString inifname = inifinfo.completeBaseName();
        if (inifname.isEmpty())
            inifname = inifinfo.dir().dirName();
        _postDataName = inifname;
        bool openinit = this->openFileStatus(_initFileName);
        if (openinit)
        {
            bool flag = this->readInitModelDatas(_modeldb);
            if (flag == false)
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("初始构型文件%1读取失败，导入中止！").arg(_initFileName), true);
                }
                else
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("Failed to read initial configuration file %1, import aborted!").arg(_initFileName), true);
                }
                mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
                return;
            }
            emit readFileSuccessSig(_initFileName);
        }
        else
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("没有初始帧文件..."), true);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("No initial frame file available..."));
            }
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
            return;
        }

        //读取帧文件
        //读取后面的mxdb文件
        const int nframe = _frameFiles.size();

        ////对文件进行分类
        QString str = "";
        QHash<QString, QVector<QString>> names{};
        for (int fi = 0; fi < nframe; ++fi)
        {
            QString framefile = _frameFiles.at(fi);
            QStringList lis = framefile.split('/');
            QString fileName = lis[lis.size() - 1];
            fileName.remove(".mxdb");
            int index = fileName.lastIndexOf('_');
            QString temp = fileName.mid(0, index);
            if (names.constFind(temp) != names.cend())
            {
                names[temp].push_back(framefile);
            }
            else
            {
                QVector<QString> vec{ framefile };
                names.insert(temp, vec);
            }
        }

        //给文件排序
        QMap<QDateTime, QVector<QString>> fileMap{};
        for (auto vec : names)
        {
            QString fileName = *(vec.begin());
            QFileInfo info(fileName);
            QDateTime time = info.lastModified();
            fileMap.insert(time, vec);
        }
        QList<QString> fileNames{};
        //按时间排序
        fileNames.reserve(nframe);
        for (auto it : fileMap)
        {
            QMap<double, QString> temp{};
            for (int i = 0; i < it.size(); ++i)
            {
                QString fileName = it.at(i);
                bool isopen = this->openFileStatus(fileName);
                if (!isopen)
                {
                    continue;
                }

                std::ifstream fs;
                fs.open(fileName.toLocal8Bit(), ios::in | ios::binary);
                if (!fs)
                {
                    continue;
                }

                //跳过版本号
                int v = 0;
                fs.read((char*)&v, sizeof(int));

                //读取工况名
                char caseName[64];
                fs.read(caseName, 64 * sizeof(char));

                //读取时间/频率类型，仅在0xA1B1C103及其之后的版本中
                if (_version >= 0xA1B1C103)
                {
                    int typeval;
                    fs.read((char*)&typeval, sizeof(int));
                }

                //读取并存储时间/频率
                double timeval;
                fs.read((char*)&timeval, sizeof(double));
                temp.insert(timeval, fileName);
            }
            if (temp.size() < it.size())
            {
                for (int i = 0; i < it.size(); ++i)
                {
                    fileNames.append(it.at(i));
                }
            }
            else
            {
                for (auto tep : temp)
                {
                    fileNames.append(tep);
                }
            }
        }

        bool isFirst = false;
        for (int fi = 0; fi < nframe; ++fi)
        {
            QString framefile = fileNames.at(fi);
            QPointer<mPostModelData> modelData = new mPostModelData();

            std::ifstream fs;
            std::unique_ptr<MxdbReaderBase> reader = createMxdbReader(fs, _version);

            if (!reader->open(framefile.toStdWString()))
            {
                emit readMxdbErrorSig(FileNotOpen);
                continue;
            }

            MxdbResultFrame frame;
            try {
                frame = reader->readResultFrame();
            } catch (const std::exception& e) {
                // 损坏文件可抛 bad_alloc 等（如垃圾数量字段触发超大 resize），
                // 跳过该帧而非让导入线程崩溃
                qWarning() << "mIMxdbFile1: result frame" << (fi + 1) << "read exception:" << e.what();
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("结果帧%1读取异常（%2），该帧已跳过，文件可能已损坏！").arg(fi + 1).arg(e.what()), true);
                }
                else
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("Result frame %1 read exception (%2), frame skipped; the file may be corrupted!").arg(fi + 1).arg(e.what()), true);
                }
                emit readMxdbErrorSig(UnknownError);
                continue;
            }
            MxdbDataBuilder builder;
            bool ok = builder.buildResultFrame(frame, _modeldb, fi + 1);
            if (!ok)
            {
                // S6 帧装载回归诊断（2026-11-09）：失败仅发 sig+continue，帧静默丢失，
                // 切帧时 GUI 死等"帧 N 装载中"。补帧号透出，使导入期丢帧可观测
                qWarning() << "mIMxdbFile1: result frame" << (fi + 1) << "build failed, skipped:"
                           << QString::fromStdString(frame.caseName);
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("结果帧%1构建失败，该帧已跳过（详见输出信息与调试日志）！").arg(fi + 1), true);
                }
                else
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("Result frame %1 build failed, frame skipped (see output window and debug log)!").arg(fi + 1), true);
                }
                emit readMxdbErrorSig(UnknownError);
                continue;
            }

            modelData = _modeldb->getModelData(fi + 1);
            PostFrameInfo frameInfo = modelData->_frameInfo;
            frameInfo._frameFile = framefile;
            // 与旧版 mIMxdbFile1 保持一致：帧名从结果文件名后缀提取（如 xxx_1.mxdb -> "1"）。
            {
                QStringList lis = framefile.split("/");
                QString fname = lis[lis.size() - 1];
                fname.remove(".mxdb");
                int nIndex = fname.lastIndexOf('_');
                if (nIndex >= 0)
                    frameInfo._frameName = fname.mid(nIndex + 1);
                else
                    frameInfo._frameName = MxFunctions::mNumberToString(fi + 1);
            }
            modelData->_frameInfo = frameInfo;
            _modeldb->appendModelData(fi + 1, modelData);

            if (frame.sameFrame == 0)
            {
                if (!isFirst)
                {
                    _flag.insert(fi + 1, -1);
                    isFirst = true;
                }
                else
                {
                    _flag.insert(fi + 1, 0);
                }
            }
            else
            {
                _flag.insert(fi + 1, frame.sameFrame);
            }

            int endtag = frame.endTag;
            if (endtag == 0xA1B1C1D2)
            {
                emit readFileSuccessSig(framefile);
            }
            continue;
        }

        mGlobalSignals::getInstance()->setProgressBarValue(90);
        _modeldb->freeInitframe();

        mPostParseMesh* parseMesh = new mPostParseMesh(this);
        mGlobalSignals::getInstance()->setProgressBarValue(92);

        //存储部件面的信息
        QHash<int, QPointer<mPostModelData>> models = _modeldb->getAllFrameData();
        auto it = models.begin();
        while (it != models.end())
        {
            int flag = _flag.value(it.key());

            if (flag != -1)
            {
                it++;
                continue;
            }
            parseMesh->ParseMesh(*it);
            it++;
        }
        mGlobalSignals::getInstance()->setProgressBarValue(93);

        delete parseMesh;
        parseMesh = nullptr;

        mGlobalSignals::getInstance()->setProgressBarValue(95);
        emit finishedImportAllMxDbFileSig();

        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
    }

    bool mIMxdbFile1::readElemData(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, const QString& global, QVector<double> local, std::set<int> eleids, bool isexport)
    {
        if (eleids.size() == 0)
            return false;
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;
        //获取局部坐标系
        //坐标系
        QVector<double> coordinate = {
            1,0,0,
            0,1,0,
            0,0,1
        };
        if (global == QString("Global"))
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else if (global == "")
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else
        {
            coordinate = outputDatabase->getAllcoordinate(global);
        }

        QVector<double> userCoord = local;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;

        auto modelData = modelDatas.value(f);

        if (modelData == nullptr)
            return false;

        PostFrameInfo frameinfo = modelData->_frameInfo;

        Compones* compones = new Compones();
        compones->isD = true;
        compones->frameID = f;
        QHash<int, double> tempvalues{};
        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                //获取分量数
                const int ncompo = varinfo._nCompo;

                //获取当前分量的索引
                const int compoi = varinfo._compoNames.indexOf(compo);

                compones->flag = varinfo._varType;

                //收集目标 entity ids
                std::vector<int> targetIds;
                targetIds.reserve(eleids.size());
                for (int id : eleids)
                {
                    targetIds.push_back(id);
                }

                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, targetIds, values))
                {
                    delete compones;
                    return false;
                }

                //获取当前变量中的变量分量个数
                const int nresult = varinfo._varCount;
                for (const auto& val : values)
                {
                    int id = val.entityId;
                    int cid = modelData->_nodedata.nodeidmap.value(id);

                    int offset = 0;
                    for (int k = 0; k < nresult; ++k)
                    {
                        //获取层数及层索引
                        const int nlayer = varinfo._varLayer.at(k);
                        const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);

                        //计算分量值个数及所需值的索引
                        int nval, valindex;
                        if (nlayer == 0)
                        {
                            if (ncompo == 0)
                            {
                                nval = 1;
                                valindex = 0;
                            }
                            else
                            {
                                nval = ncompo;
                                valindex = compoi;
                            }
                        }
                        else
                        {
                            if (ncompo == 0)
                            {
                                nval = nlayer;
                                valindex = layerindex + compoi;
                            }
                            else
                            {
                                nval = ncompo * nlayer;
                                valindex = ncompo * layerindex + compoi;
                            }
                        }

                        if (offset + nval > static_cast<int>(val.realValues.size()))
                        {
                            offset += nval;
                            continue;
                        }

                        //值（实部）
                        double* realVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                        {
                            realVals[i] = val.realValues[offset + i];
                        }
                        QVector<double> dis;
                        for (int i = 0; i < nval; i++)
                        {
                            dis.push_back(realVals[i]);
                        }
                        if (var == QString("Displacement"))
                        {
                            QVector<double> res = multiplyMatrixVector(coordinate, dis);
                            delete[]realVals;
                            realVals = new double[res.size()];
                            std::copy(res.constBegin(), res.constEnd(), realVals);
                        }

                        //场变量局部柱坐标系转化
                        if (userCoord.size() > 0 && (compo == "X" || compo == "Y" || compo == "Z" ||
                            compo == "XX" || compo == "YY" || compo == "ZZ" || compo == "XY" ||
                            compo == "YZ" || compo == "XZ"))
                        {
                            QVector3D oldVec;
                            oldVec.setX(dis[0]);
                            oldVec.setY(dis[1]);
                            oldVec.setZ(dis[2]);

                            if (userCoord.size() % 3 != 0) return false;
                            QVector<QVector3D> vector3Ds;
                            int numVectors = userCoord.size() / 3;
                            for (int i = 0; i < numVectors; ++i)
                            {
                                int index = i * 3;
                                double x = userCoord[index];
                                double y = userCoord[index + 1];
                                double z = userCoord[index + 2];
                                vector3Ds.append(QVector3D(x, y, z));
                            }

                            QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

                            if (var == QString("Displacement"))
                            {
                                dis[0] = newVec[0];
                                dis[1] = newVec[1];
                                dis[2] = newVec[2];

                                delete[]realVals;
                                realVals = new double[dis.size()];
                                std::copy(dis.constBegin(), dis.constEnd(), realVals);
                            }
                            else if (var == QString("Stress(N)"))
                            {
                                dis[0] = newVec[0];
                                dis[1] = newVec[1];
                                dis[2] = newVec[2];

                                dis[3] = sqrt(dis[1] * dis[1] + dis[2] * dis[2]);
                                dis[4] = sqrt(dis[0] * dis[0] + dis[2] * dis[2]);
                                dis[5] = sqrt(dis[0] * dis[0] + dis[1] * dis[1]);

                                delete[]realVals;
                                realVals = new double[dis.size()];
                                std::copy(dis.constBegin(), dis.constEnd(), realVals);
                            }
                        }

                        //值（虚部）
                        double* imagVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                        {
                            imagVals[i] = val.imagValues[offset + i];
                        }

                        double tempval;
                        if (complex == QString("实部") || complex == QString("Real"))
                        {
                            tempval = realVals[valindex];
                        }
                        else if (complex == QString("虚部") || complex == QString("Img"))
                        {
                            tempval = imagVals[valindex];
                        }
                        else if (complex == QString("幅值") || complex == QString("Amplitude"))
                        {
                            double v0 = realVals[valindex];
                            double v1 = imagVals[valindex];
                            tempval = sqrt(v0 * v0 + v1 * v1);
                        }
                        else if (complex == QString("相位") || complex == QString("Phase"))
                        {
                            // 实部
                            double v0 = realVals[valindex];
                            // 虚部
                            double v1 = imagVals[valindex];
                            tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                            //cout <<"id = "<< id <<"\t"<< "real:v0 = " << v0 << "\t" << "imag:v1 = " << v1 << "\t" << "tempval = " << tempval << "\t" << "\n";
                        }
                        else
                        {
                            // 这里计算mag合量值
                            if (valindex == -1 && nval == 3 && compo == "Mag")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2] +
                                    imagVals[0] * imagVals[0] + imagVals[1] * imagVals[1] + imagVals[2] * imagVals[2]);
                            }
                            else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2]);
                            }
                            else
                                tempval = realVals[valindex];
                        }
                        tempvalues.insert(cid, tempval);

                        delete[]realVals;
                        delete[]imagVals;
                        offset += nval;
                    }
                }

                //存储指定单元的节点数据
                std::map<int, std::map<int, double>> elesdata{};
                for (auto eleid : eleids)
                {
                    QVector<int> cell_node_ids = modelData->getNodeIdsByEleId(eleid);
                    std::map<int, double> values{};
                    for (auto nodeid : cell_node_ids)
                    {
                        int rid = modelData->_nodedata.ids[nodeid];
                        double val = tempvalues.value(nodeid);
                        values.insert({ rid, val });
                    }
                    elesdata.insert({ eleid, values });
                }
                outputDatabase->setElemsData(elesdata);
                return true;
            }
        }
        return false;
    }

    void mIMxdbFile1::slot_importOneFrameData(PostData::mPostAnimationRendData* animationData, int f, int threadNum, const QString& postname, const QString& var, const QString& compo, const QString& layer, const QString& complex, bool hasstreamLine, QString streamLineVectorName, bool hasvectorArrowVector, QString vectorArrowVectorName)
    {
        _animationData = animationData;
        _threadNum = threadNum;
        _frame = f;
        _pname = postname, _var = var, _compo = compo, _layer = layer, _complex = complex; _hasstreamLine = hasstreamLine; _streamLineVectorName = streamLineVectorName;  _hasvectorArrowVector = hasvectorArrowVector; _vectorArrowVectorName = vectorArrowVectorName;
    }

    bool mIMxdbFile1::readCloudData(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, bool isexport)
    {
        auto db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (db == nullptr)
            return false;
        QHash<int, QPointer<mPostModelData>> modelDatabase = db->getAllFrameData();
        if (f < 1 || f > modelDatabase.size())
            return false;
        QPointer<mPostModelData> modelData = modelDatabase.value(f);
        PostFrameInfo frameinfo = modelData->_frameInfo;
        const int nvar = frameinfo._frameVars.size();
        Compones* compones = new Compones();
        compones->isD = true;
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                //获取分量数
                const int ncompo = varinfo._nCompo;

                //获取当前分量的索引
                const int compoi = varinfo._compoNames.indexOf(compo);

                //存储值
                double maxval = -DBL_MAX, minval = DBL_MAX;
                //存储ID值
                //int maxID = 0, minID = 0;
                //QHash<int, double> compovalues;
                compones->frameID = f;
                compones->flag = varinfo._varType;

                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                {
                    delete compones;
                    return false;
                }

                //获取当前变量中的变量分量个数
                const int nresult = varinfo._nCompo;
                for (const auto& val : values)
                {
                    int id = val.entityId;

                    int offset = 0;
                    for (int k = 0; k < nresult; ++k)
                    {
                        //获取层数及层索引
                        const int nlayer = varinfo._varLayer.at(k);
                        const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);
                        //计算分量值个数及所需值的索引
                        int nval, valindex;
                        if (nlayer == 0)
                        {
                            if (ncompo == 0)
                            {
                                nval = 1;
                                valindex = 0;
                            }
                            else
                            {
                                nval = ncompo;
                                valindex = compoi;
                            }
                        }
                        else
                        {
                            if (ncompo == 0)
                            {
                                nval = nlayer;
                                valindex = layerindex + compoi;
                            }
                            else
                            {
                                nval = ncompo * nlayer;
                                valindex = ncompo * layerindex + compoi;
                            }
                        }

                        if (offset + nval > static_cast<int>(val.realValues.size()))
                        {
                            offset += nval;
                            continue;
                        }

                        //值（实部）
                        double* realVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                            realVals[i] = val.realValues[offset + i];
                        vector<double> stress;
                        for (int i = 0; i < nval; i++)
                            stress.push_back(realVals[i]);

                        //值（虚部）
                        double* imagVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                            imagVals[i] = val.imagValues[offset + i];

                        double tempval;
                        if (complex == QString("实部"))
                            tempval = realVals[valindex];
                        else if (complex == QString("虚部"))
                            tempval = imagVals[valindex];
                        else if (complex == QString("幅值"))
                        {
                            double v0 = realVals[valindex];
                            double v1 = imagVals[valindex];
                            tempval = sqrt(v0 * v0 + v1 * v1);
                        }
                        else if (complex == QString("相位"))
                        {
                            double v0 = realVals[valindex];
                            double v1 = imagVals[valindex];
                            tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                        }
                        else
                        {
                            if (valindex == -1 && nval == 3 && compo == "Mag")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2] +
                                    imagVals[0] * imagVals[0] + imagVals[1] * imagVals[1] + imagVals[2] * imagVals[2]);
                            }
                            else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2]);
                            }
                            else
                            {
                                tempval = realVals[valindex];
                            }
                        }
                        compones->value1.insert(id, tempval);
                        delete[]realVals;
                        delete[]imagVals;
                        offset += nval;
                    }
                }
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    //输出信息
                    mOutputMessage tepmsg(NormalMessage, QString("第%1帧文件数据读取成功！").arg(f));
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("第%1帧文件数据读取成功！").arg(f), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    //输出信息
                    mOutputMessage tepmsg(NormalMessage, QString("The frame %1 file data is read successfully！").arg(f));
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("The frame %1 file data is read successfully！").arg(f), true);
                }

                db->setComponeValues(*compones);
                return true;
            }
        }
        return false;
    }

    bool mIMxdbFile1::readCloudDataResult(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, bool isexport)
    {
        auto db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (db == nullptr)
            return false;
        QHash<int, QPointer<mPostModelData>> modelDatabase = db->getAllFrameData();

        if (f < 1 || f > modelDatabase.size())
            return false;

        PostFrameInfo frameinfo = modelDatabase.value(f)->_frameInfo;
        Compones* compones = new Compones();
        compones->isD = true;
        compones->frameID = f;
        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                //获取分量数
                const int ncompo = varinfo._nCompo;

                //获取当前分量的索引
                const int compoi = varinfo._compoNames.indexOf(compo);

                //存储值
                double maxval = -DBL_MAX, minval = DBL_MAX;
                //存储ID值
                int maxID = 0, minID = 0;

                compones->flag = varinfo._varType;

                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                {
                    delete compones;
                    return false;
                }

                //获取当前变量中的变量个数
                const int nresult = varinfo._nCompo;
                for (const auto& val : values)
                {
                    int id = val.entityId;

                    int offset = 0;
                    for (int k = 0; k < nresult; ++k)
                    {
                        //获取层数及层索引
                        const int nlayer = varinfo._varLayer.at(k);
                        const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);
                        //计算分量值个数及所需值的索引
                        int nval, valindex;
                        if (nlayer == 0)
                        {
                            if (ncompo == 0)
                            {
                                nval = 1;
                                valindex = 0;
                            }
                            else
                            {
                                nval = ncompo;
                                valindex = compoi;
                            }
                        }
                        else
                        {
                            if (ncompo == 0)
                            {
                                nval = nlayer;
                                valindex = layerindex + compoi;
                            }
                            else
                            {
                                nval = ncompo * nlayer;
                                valindex = ncompo * layerindex + compoi;
                            }
                        }

                        if (offset + nval > static_cast<int>(val.realValues.size()))
                        {
                            offset += nval;
                            continue;
                        }

                        //值（实部）
                        double* realVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                            realVals[i] = val.realValues[offset + i];
                        vector<double> stress;
                        for (int i = 0; i < nval; i++)
                            stress.push_back(realVals[i]);

                        //值（虚部）
                        double* imagVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                            imagVals[i] = val.imagValues[offset + i];

                        double tempval;
                        if (complex == QString("实部"))
                            tempval = realVals[valindex];
                        else if (complex == QString("虚部"))
                            tempval = imagVals[valindex];
                        else if (complex == QString("幅值"))
                        {
                            double v0 = realVals[valindex];
                            double v1 = imagVals[valindex];
                            tempval = sqrt(v0 * v0 + v1 * v1);
                        }
                        else if (complex == QString("相位"))
                        {
                            double v0 = realVals[valindex];
                            double v1 = imagVals[valindex];
                            tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                        }
                        else
                        {
                            if (valindex == -1 && nval == 3 && compo == "Mag")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2] +
                                    imagVals[0] * imagVals[0] + imagVals[1] * imagVals[1] + imagVals[2] * imagVals[2]);
                            }
                            else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2]);
                            }
                            else
                            {
                                tempval = realVals[valindex];
                            }
                        }
                        compones->value1.insert(id, tempval);
                        delete[]realVals;
                        delete[]imagVals;
                        offset += nval;
                    }
                }
                db->setComponeValues(*compones);
                return true;
            }
        }
        return false;
    }

    bool mIMxdbFile1::readCloudData(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, const QString& global, QVector<double> local, bool isexport)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;
        //获取局部坐标系
        //坐标系
        QVector<double> coordinate = {
            1,0,0,
            0,1,0,
            0,0,1
        };
        if (global == QString("Global"))
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else if (global == "")
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else
        {
            coordinate = outputDatabase->getAllcoordinate(global);
        }

        QVector<double> userCoord = local;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;

        auto modelData = modelDatas.value(f);

        if (modelData == nullptr)
            return false;

        PostFrameInfo frameinfo = modelData->_frameInfo;

        Compones* compones = new Compones();
        compones->isD = true;
        compones->frameID = f;

        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                //获取分量数
                const int ncompo = varinfo._nCompo;

                //获取当前分量的索引
                const int compoi = varinfo._compoNames.indexOf(compo);

                //存储值
                double maxval = -DBL_MAX, minval = DBL_MAX;
                //存储ID值
                int maxID = 0, minID = 0;

                compones->flag = varinfo._varType;

                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                {
                    delete compones;
                    return false;
                }

                //获取当前变量中的变量分量个数
                const int nresult = varinfo._varCount;
                for (const auto& val : values)
                {
                    int id = val.entityId;
                    int cid = -1;
                    if (varinfo._varType == 0)
                        cid = modelData->_nodedata.nodeidmap.value(id);

                    int offset = 0;
                    for (int k = 0; k < nresult; ++k)
                    {
                        //获取层数及层索引
                        const int nlayer = varinfo._varLayer.at(k);
                        const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);
                        //计算分量值个数及所需值的索引
                        int nval, valindex;
                        if (nlayer == 0)
                        {
                            if (ncompo == 0)
                            {
                                nval = 1;
                                valindex = 0;
                            }
                            else
                            {
                                nval = ncompo;
                                valindex = compoi;
                            }
                        }
                        else
                        {
                            if (ncompo == 0)
                            {
                                nval = nlayer;
                                valindex = layerindex + compoi;
                            }
                            else
                            {
                                nval = ncompo * nlayer;
                                valindex = ncompo * layerindex + compoi;
                            }
                        }

                        if (offset + nval > static_cast<int>(val.realValues.size()))
                        {
                            offset += nval;
                            continue;
                        }

                        //值（实部）
                        double* realVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                            realVals[i] = val.realValues[offset + i];
                        QVector<double> dis;
                        for (int i = 0; i < nval; i++)
                            dis.push_back(realVals[i]);
                        if (var == QString("Displacement"))
                        {
                            QVector<double> res = multiplyMatrixVector(coordinate, dis);
                            delete[]realVals;
                            realVals = new double[res.size()];
                            std::copy(res.constBegin(), res.constEnd(), realVals);
                        }

                        //场变量局部柱坐标系转化
                        if (userCoord.size() > 0 && (compo == "X" || compo == "Y" || compo == "Z" ||
                            compo == "XX" || compo == "YY" || compo == "ZZ" || compo == "XY" ||
                            compo == "YZ" || compo == "XZ"))
                        {
                            QVector3D oldVec;
                            oldVec.setX(dis[0]);
                            oldVec.setY(dis[1]);
                            oldVec.setZ(dis[2]);

                            if (userCoord.size() % 3 != 0) return false;
                            QVector<QVector3D> vector3Ds;
                            int numVectors = userCoord.size() / 3;
                            for (int i = 0; i < numVectors; ++i)
                            {
                                int index = i * 3;
                                double x = userCoord[index];
                                double y = userCoord[index + 1];
                                double z = userCoord[index + 2];
                                vector3Ds.append(QVector3D(x, y, z));
                            }

                            QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

                            if (var == QString("Displacement"))
                            {
                                dis[0] = newVec[0];
                                dis[1] = newVec[1];
                                dis[2] = newVec[2];

                                delete[]realVals;
                                realVals = new double[dis.size()];
                                std::copy(dis.constBegin(), dis.constEnd(), realVals);
                            }
                            else if (var == QString("Stress(N)"))
                            {
                                dis[0] = newVec[0];
                                dis[1] = newVec[1];
                                dis[2] = newVec[2];

                                dis[3] = sqrt(dis[1] * dis[1] + dis[2] * dis[2]);
                                dis[4] = sqrt(dis[0] * dis[0] + dis[2] * dis[2]);
                                dis[5] = sqrt(dis[0] * dis[0] + dis[1] * dis[1]);

                                delete[]realVals;
                                realVals = new double[dis.size()];
                                std::copy(dis.constBegin(), dis.constEnd(), realVals);
                            }
                        }

                        //值（虚部）
                        double* imagVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                            imagVals[i] = val.imagValues[offset + i];

                        double tempval;
                        // 防止valindex为-1时数组越界（当compo="Mag"且complex="实部"时）
                        if (valindex < 0 || valindex >= nval)
                        {
                            // 计算mag合量值
                            if (nval == 3 && compo == "Mag")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2] +
                                    imagVals[0] * imagVals[0] + imagVals[1] * imagVals[1] + imagVals[2] * imagVals[2]);
                            }
                            else if (nval == 3 && compo == "Mag cos(wt+phase)")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2]);
                            }
                            else
                            {
                                tempval = 0.0;
                            }
                        }
                        else if (complex == QString("实部") || complex == QString("Real"))
                        {
                            tempval = realVals[valindex];
                        }
                        else if (complex == QString("虚部") || complex == QString("Img"))
                        {
                            tempval = imagVals[valindex];
                        }
                        else if (complex == QString("幅值") || complex == QString("Amplitude"))
                        {
                            double v0 = realVals[valindex];
                            double v1 = imagVals[valindex];
                            tempval = sqrt(v0 * v0 + v1 * v1);
                        }
                        else if (complex == QString("相位") || complex == QString("Phase"))
                        {
                            // 实部
                            double v0 = realVals[valindex];
                            // 虚部
                            double v1 = imagVals[valindex];
                            tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                            //cout <<"id = "<< id <<"\t"<< "real:v0 = " << v0 << "\t" << "imag:v1 = " << v1 << "\t" << "tempval = " << tempval << "\t" << "\n";
                        }
                        else
                        {
                            // 这里计算mag合量值
                            if (valindex == -1 && nval == 3 && compo == "Mag")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2] +
                                    imagVals[0] * imagVals[0] + imagVals[1] * imagVals[1] + imagVals[2] * imagVals[2]);
                            }
                            else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                            {
                                tempval = sqrt(realVals[0] * realVals[0] + realVals[1] * realVals[1] + realVals[2] * realVals[2]);
                            }
                            else
                                tempval = realVals[valindex];
                        }

                        //防止输出无关的节点的值
                        if (varinfo._varType == 0)
                        {
                            if (modelData->_nodedata.nodeidmap.constFind(id) != modelData->_nodedata.nodeidmap.cend())
                            {
                                compones->value1.insert(cid, tempval);
                                if (tempval > maxval)
                                {
                                    maxval = tempval;
                                    maxID = id;
                                }
                                if (tempval < minval)
                                {
                                    minval = tempval;
                                    minID = id;
                                }
                            }
                        }
                        else
                        {
                            compones->value1.insert(id, tempval);
                            if (tempval > maxval)
                            {
                                maxval = tempval;
                                maxID = id;
                            }
                            if (tempval < minval)
                            {
                                minval = tempval;
                                minID = id;
                            }
                        }

                        delete[]realVals;
                        delete[]imagVals;
                        offset += nval;
                    }
                }

                QList<QPair<int, double>> temp{};
                temp.append(QPair<int, double>(maxID, maxval));
                temp.append(QPair<int, double>(minID, minval));
                compones->varName = var;
                compones->compoName = compo;
                //outputDatabase->setResultMaxMin(f, temp);
                outputDatabase->setComponeValues(*compones);
                delete compones;
                return true;
            }
        }
        return false;
    }

    bool mIMxdbFile1::readResultData(const QString& pname, int f, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QString& global, QVector<double> local, std::set<int>& ids, QHash<QString, QHash<int, double>>& allVals, int& flag)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;
        //获取局部坐标系
        //坐标系
        QVector<double> coordinate = {
            1,0,0,
            0,1,0,
            0,0,1
        };
        if (global == QString("Global"))
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else if (global == "")
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else
        {
            coordinate = outputDatabase->getAllcoordinate(global);
        }

        QVector<double> userCoord = local;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;

        auto modelData = modelDatas.value(f);

        if (modelData == nullptr)
            return false;

        PostFrameInfo frameinfo = modelData->_frameInfo;

        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                //获取分量数
                const int ncompo = varinfo._nCompo;

                //存储ID值
                int maxID = 0, minID = 0;

                flag = varinfo._varType;

                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                    return false;

                //获取当前变量中的变量分量个数
                const int nresult = varinfo._varCount;
                for (const auto& val : values)
                {
                    int id = val.entityId;

                    if (ids.count(id) <= 0)
                        continue;

                    int offset = 0;
                    for (int k = 0; k < nresult; ++k)
                    {
                        //获取层数及层索引
                        const int nlayer = varinfo._varLayer.at(k);
                        const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);

                        //计算分量值个数
                        int nval;
                        if (nlayer == 0)
                        {
                            if (ncompo == 0)
                                nval = 1;
                            else
                                nval = ncompo;
                        }
                        else
                        {
                            if (ncompo == 0)
                                nval = nlayer;
                            else
                                nval = ncompo * nlayer;
                        }

                        if (offset + nval > static_cast<int>(val.realValues.size()))
                        {
                            offset += nval;
                            continue;
                        }

                        //值（实部/虚部）
                        double* realVals = new double[nval];
                        double* imagVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                        {
                            realVals[i] = val.realValues[offset + i];
                            imagVals[i] = val.imagValues[offset + i];
                        }

                        for (auto compo : compos)
                        {
                            QVector<double> dis;
                            for (int i = 0; i < nval; i++)
                                dis.push_back(realVals[i]);

                            // 使用局部可修改副本进行坐标转换
                            double* val = new double[nval];
                            double* val1 = new double[nval];
                            for (int i = 0; i < nval; i++)
                            {
                                val[i] = realVals[i];
                                val1[i] = imagVals[i];
                            }

                            if (var == QString("Displacement"))
                            {
                                QVector<double> res = multiplyMatrixVector(coordinate, dis);
                                delete[]val;
                                val = new double[res.size()];
                                std::copy(res.constBegin(), res.constEnd(), val);
                            }

                            //场变量局部柱坐标系转化
                            if (userCoord.size() > 0 && (compo == "X" || compo == "Y" || compo == "Z" ||
                                compo == "XX" || compo == "YY" || compo == "ZZ" || compo == "XY" ||
                                compo == "YZ" || compo == "XZ"))
                            {
                                QVector3D oldVec;
                                oldVec.setX(dis[0]);
                                oldVec.setY(dis[1]);
                                oldVec.setZ(dis[2]);

                                if (userCoord.size() % 3 != 0) return false;
                                QVector<QVector3D> vector3Ds;
                                int numVectors = userCoord.size() / 3;
                                for (int i = 0; i < numVectors; ++i)
                                {
                                    int index = i * 3;
                                    double x = userCoord[index];
                                    double y = userCoord[index + 1];
                                    double z = userCoord[index + 2];
                                    vector3Ds.append(QVector3D(x, y, z));
                                }

                                QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

                                if (var == QString("Displacement"))
                                {
                                    dis[0] = newVec[0];
                                    dis[1] = newVec[1];
                                    dis[2] = newVec[2];

                                    delete[]val;
                                    val = new double[dis.size()];
                                    std::copy(dis.constBegin(), dis.constEnd(), val);
                                }
                                else if (var == QString("Stress(N)"))
                                {
                                    dis[0] = newVec[0];
                                    dis[1] = newVec[1];
                                    dis[2] = newVec[2];

                                    dis[3] = sqrt(dis[1] * dis[1] + dis[2] * dis[2]);
                                    dis[4] = sqrt(dis[0] * dis[0] + dis[2] * dis[2]);
                                    dis[5] = sqrt(dis[0] * dis[0] + dis[1] * dis[1]);

                                    delete[]val;
                                    val = new double[dis.size()];
                                    std::copy(dis.constBegin(), dis.constEnd(), val);
                                }
                            }
                            //获取当前分量的索引
                            const int compoi = varinfo._compoNames.indexOf(compo);

                            //计算分量的索引
                            int valindex;
                            if (nlayer == 0)
                            {
                                if (ncompo == 0)
                                    valindex = 0;
                                else
                                    valindex = compoi;
                            }
                            else
                            {
                                if (ncompo == 0)
                                    valindex = layerindex + compoi;
                                else
                                    valindex = ncompo * layerindex + compoi;
                            }

                            double tempval;

                            if ((complex == QString("实部") || complex == QString("Real")) && compo != QString("Mag"))
                            {
                                tempval = val[valindex];
                            }
                            else if ((complex == QString("虚部") || complex == QString("Img")) && compo != QString("Mag"))
                            {
                                tempval = val1[valindex];
                            }
                            else if ((complex == QString("幅值") || complex == QString("Amplitude")) && compo != QString("Mag"))
                            {
                                double v0 = val[valindex];
                                double v1 = val1[valindex];
                                tempval = sqrt(v0 * v0 + v1 * v1);
                            }
                            else if ((complex == QString("相位") || complex == QString("Phase")) && compo != QString("Mag"))
                            {
                                // 实部
                                double v0 = val[valindex];
                                // 虚部
                                double v1 = val1[valindex];
                                tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                                //cout <<"id = "<< id <<"\t"<< "real:v0 = " << v0 << "\t" << "imag:v1 = " << v1 << "\t" << "tempval = " << tempval << "\t" << "\n";
                            }
                            else
                            {
                                // 这里计算mag合量值
                                if (valindex == -1 && nval == 3 && compo == "Mag")
                                {
                                    tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2] +
                                        val1[0] * val1[0] + val1[1] * val1[1] + val1[2] * val1[2]);
                                }
                                else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                                {
                                    tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
                                }
                                else
                                    tempval = val[valindex];
                            }

                            QHash<int, double> curVals = allVals.value(compo);
                            //防止输出无关的节点的值
                            if (varinfo._varType == 0)
                            {
                                if (modelData->_nodedata.nodeidmap.constFind(id) != modelData->_nodedata.nodeidmap.cend())
                                {
                                    curVals.insert(id, tempval);
                                }
                            }
                            else
                            {
                                curVals.insert(id, tempval);
                            }
                            allVals.insert(compo, curVals);

                            delete[]val;
                            delete[]val1;
                        }
                        delete[]realVals;
                        delete[]imagVals;
                        offset += nval;
                    }
                }
                return true;
            }
        }
        return false;
    }

    bool mIMxdbFile1::readResultData(const QString& pname, int f, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QString& global, QVector<double> local, QHash<QString, QHash<int, double>>& allVals, int& flag)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;
        //获取局部坐标系
        //坐标系
        QVector<double> coordinate = {
            1,0,0,
            0,1,0,
            0,0,1
        };
        if (global == QString("Global"))
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else if (global == "")
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else
        {
            coordinate = outputDatabase->getAllcoordinate(global);
        }

        QVector<double> userCoord = local;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;

        auto modelData = modelDatas.value(f);

        if (modelData == nullptr)
            return false;

        PostFrameInfo frameinfo = modelData->_frameInfo;

        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                //获取分量数
                const int ncompo = varinfo._nCompo;

                //存储ID值
                int maxID = 0, minID = 0;

                flag = varinfo._varType;
                QHash<int, double> curVals{};

                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                    return false;

                //获取当前变量中的变量分量个数
                const int nresult = varinfo._varCount;
                for (const auto& val : values)
                {
                    int id = val.entityId;

                    int offset = 0;
                    for (int k = 0; k < nresult; ++k)
                    {
                        //获取层数及层索引
                        const int nlayer = varinfo._varLayer.at(k);
                        const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);

                        //计算分量值个数
                        int nval;
                        if (nlayer == 0)
                        {
                            if (ncompo == 0)
                                nval = 1;
                            else
                                nval = ncompo;
                        }
                        else
                        {
                            if (ncompo == 0)
                                nval = nlayer;
                            else
                                nval = ncompo * nlayer;
                        }

                        if (offset + nval > static_cast<int>(val.realValues.size()))
                        {
                            offset += nval;
                            continue;
                        }

                        //值（实部/虚部）
                        double* realVals = new double[nval];
                        double* imagVals = new double[nval];
                        for (int i = 0; i < nval; i++)
                        {
                            realVals[i] = val.realValues[offset + i];
                            imagVals[i] = val.imagValues[offset + i];
                        }

                        for (auto compo : compos)
                        {
                            QVector<double> dis;
                            for (int i = 0; i < nval; i++)
                                dis.push_back(realVals[i]);

                            // 使用局部可修改副本
                            double* val = new double[nval];
                            double* val1 = new double[nval];
                            for (int i = 0; i < nval; i++)
                            {
                                val[i] = realVals[i];
                                val1[i] = imagVals[i];
                            }

                            if (var == QString("Displacement"))
                            {
                                QVector<double> res = multiplyMatrixVector(coordinate, dis);
                                delete[]val;
                                val = new double[res.size()];
                                std::copy(res.constBegin(), res.constEnd(), val);
                            }

                            //场变量局部柱坐标系转化
                            if (userCoord.size() > 0 && (compo == "X" || compo == "Y" || compo == "Z" ||
                                compo == "XX" || compo == "YY" || compo == "ZZ" || compo == "XY" ||
                                compo == "YZ" || compo == "XZ"))
                            {
                                QVector3D oldVec;
                                oldVec.setX(dis[0]);
                                oldVec.setY(dis[1]);
                                oldVec.setZ(dis[2]);

                                if (userCoord.size() % 3 != 0) return false;
                                QVector<QVector3D> vector3Ds;
                                int numVectors = userCoord.size() / 3;
                                for (int i = 0; i < numVectors; ++i)
                                {
                                    int index = i * 3;
                                    double x = userCoord[index];
                                    double y = userCoord[index + 1];
                                    double z = userCoord[index + 2];
                                    vector3Ds.append(QVector3D(x, y, z));
                                }

                                QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

                                if (var == QString("Displacement"))
                                {
                                    dis[0] = newVec[0];
                                    dis[1] = newVec[1];
                                    dis[2] = newVec[2];

                                    delete[]val;
                                    val = new double[dis.size()];
                                    std::copy(dis.constBegin(), dis.constEnd(), val);
                                }
                                else if (var == QString("Stress(N)"))
                                {
                                    dis[0] = newVec[0];
                                    dis[1] = newVec[1];
                                    dis[2] = newVec[2];

                                    dis[3] = sqrt(dis[1] * dis[1] + dis[2] * dis[2]);
                                    dis[4] = sqrt(dis[0] * dis[0] + dis[2] * dis[2]);
                                    dis[5] = sqrt(dis[0] * dis[0] + dis[1] * dis[1]);

                                    delete[]val;
                                    val = new double[dis.size()];
                                    std::copy(dis.constBegin(), dis.constEnd(), val);
                                }
                            }
                            //获取当前分量的索引
                            const int compoi = varinfo._compoNames.indexOf(compo);

                            //计算分量的索引
                            int valindex;
                            if (nlayer == 0)
                            {
                                if (ncompo == 0)
                                    valindex = 0;
                                else
                                    valindex = compoi;
                            }
                            else
                            {
                                if (ncompo == 0)
                                    valindex = layerindex + compoi;
                                else
                                    valindex = ncompo * layerindex + compoi;
                            }

                            double tempval;

                            if ((complex == QString("实部") || complex == QString("Real")) && compo != QString("Mag"))
                            {
                                tempval = val[valindex];
                            }
                            else if ((complex == QString("虚部") || complex == QString("Img")) && compo != QString("Mag"))
                            {
                                tempval = val1[valindex];
                            }
                            else if ((complex == QString("幅值") || complex == QString("Amplitude")) && compo != QString("Mag"))
                            {
                                double v0 = val[valindex];
                                double v1 = val1[valindex];
                                tempval = sqrt(v0 * v0 + v1 * v1);
                            }
                            else if ((complex == QString("相位") || complex == QString("Phase")) && compo != QString("Mag"))
                            {
                                // 实部
                                double v0 = val[valindex];
                                // 虚部
                                double v1 = val1[valindex];
                                tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                                //cout <<"id = "<< id <<"\t"<< "real:v0 = " << v0 << "\t" << "imag:v1 = " << v1 << "\t" << "tempval = " << tempval << "\t" << "\n";
                            }
                            else
                            {
                                // 这里计算mag合量值
                                if (valindex == -1 && nval == 3 && compo == "Mag")
                                {
                                    tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2] +
                                        val1[0] * val1[0] + val1[1] * val1[1] + val1[2] * val1[2]);
                                }
                                else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                                {
                                    tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
                                }
                                else
                                    tempval = val[valindex];
                            }
                            //防止输出无关的节点的值
                            if (varinfo._varType == 0)
                            {
                                if (modelData->_nodedata.nodeidmap.constFind(id) != modelData->_nodedata.nodeidmap.cend())
                                {
                                    curVals.insert(id, tempval);
                                }
                            }
                            else
                            {
                                curVals.insert(id, tempval);
                            }

                            delete[]val;
                            delete[]val1;
                        }
                        delete[]realVals;
                        delete[]imagVals;
                        offset += nval;
                    }
                }
                allVals[compos[0]] = curVals;
                return true;
            }
        }
        return false;
    }

    bool mIMxdbFile1::readCloudData(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, const QString& global, QVector<double> local, Compones& componeData, bool isexport)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;
        //获取局部坐标系
        //坐标系
        QVector<double> coordinate = {
            1,0,0,
            0,1,0,
            0,0,1
        };
        if (global == QString("Global"))
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else if (global == "")
        {
            coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };

        }
        else
        {
            coordinate = outputDatabase->getAllcoordinate(global);
        }

        QVector<double> userCoord = local;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;

        auto modelData = modelDatas.value(f);

        if (modelData == nullptr)
            return false;

        PostFrameInfo frameinfo = modelData->_frameInfo;

        componeData.isD = true;
        componeData.frameID = f;

        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                // 使用 MxdbReader 读取变量
                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                    return false;

                //获取分量数
                const int ncompo = varinfo._nCompo;

                //获取当前分量的索引
                const int compoi = varinfo._compoNames.indexOf(compo);

                //存储值
                double maxval = -DBL_MAX, minval = DBL_MAX;
                //存储ID值
                int maxID = 0, minID = 0;

                componeData.flag = varinfo._varType;

                //获取当前变量中的变量分量个数
                const int nresult = varinfo._varCount;
                for (int k = 0; k < nresult; ++k)
                {
                    //获取层数及层索引
                    const int nlayer = varinfo._varLayer.at(k);

                    const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);

                    //计算分量值个数及所需值的索引
                    int nval, valindex;
                    if (nlayer == 0)
                    {
                        if (ncompo == 0)
                        {
                            nval = 1;
                            valindex = 0;
                        }
                        else
                        {
                            nval = ncompo;
                            valindex = compoi;
                        }
                    }
                    else
                    {
                        if (ncompo == 0)
                        {
                            nval = nlayer;
                            valindex = layerindex + compoi;
                        }
                        else
                        {
                            nval = ncompo * nlayer;
                            valindex = ncompo * layerindex + compoi;
                        }
                    }

                    // 计算当前 shape 在 value 数组中的偏移
                    int offset = 0;
                    for (int kk = 0; kk < k; ++kk)
                    {
                        const int prevLayer = varinfo._varLayer.at(kk);
                        const int prevNval = (prevLayer == 0) ? (ncompo == 0 ? 1 : ncompo) : (ncompo == 0 ? prevLayer : ncompo * prevLayer);
                        offset += prevNval;
                    }

                    for (const auto& val : values)
                    {
                        int id = val.entityId;
                        int cid = -1;
                        if (varinfo._varType == 0)
                            cid = modelData->_nodedata.nodeidmap.value(id);

                        if (offset + nval > static_cast<int>(val.realValues.size()))
                            continue;

                        const double* value = val.realValues.data() + offset;
                        const double* value1 = val.imagValues.data() + offset;
                        QVector<double> dis;
                        for (int i = 0; i < nval; i++)
                            dis.push_back(value[i]);

                        //场变量局部柱坐标系转化
                        if (userCoord.size() > 0 && (compo == "X" || compo == "Y" || compo == "Z" ||
                            compo == "XX" || compo == "YY" || compo == "ZZ" || compo == "XY" ||
                            compo == "YZ" || compo == "XZ"))
                        {
                            QVector3D oldVec;
                            oldVec.setX(dis[0]);
                            oldVec.setY(dis[1]);
                            oldVec.setZ(dis[2]);

                            if (userCoord.size() % 3 != 0) return false;
                            QVector<QVector3D> vector3Ds;
                            int numVectors = userCoord.size() / 3;
                            for (int i = 0; i < numVectors; ++i)
                            {
                                int index = i * 3;
                                double x = userCoord[index];
                                double y = userCoord[index + 1];
                                double z = userCoord[index + 2];
                                vector3Ds.append(QVector3D(x, y, z));
                            }

                            QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

                            if (var == QString("Displacement"))
                            {
                                dis[0] = newVec[0];
                                dis[1] = newVec[1];
                                dis[2] = newVec[2];
                            }
                            else if (var == QString("Stress(N)"))
                            {
                                dis[0] = newVec[0];
                                dis[1] = newVec[1];
                                dis[2] = newVec[2];

                                dis[3] = sqrt(dis[1] * dis[1] + dis[2] * dis[2]);
                                dis[4] = sqrt(dis[0] * dis[0] + dis[2] * dis[2]);
                                dis[5] = sqrt(dis[0] * dis[0] + dis[1] * dis[1]);
                            }
                        }

                        double tempval;
                        if (complex == QString("实部") || complex == QString("Real"))
                        {
                            tempval = value[valindex];
                        }
                        else if (complex == QString("虚部") || complex == QString("Img"))
                        {
                            tempval = value1[valindex];
                        }
                        else if (complex == QString("幅值") || complex == QString("Amplitude"))
                        {
                            double v0 = value[valindex];
                            double v1 = value1[valindex];
                            tempval = sqrt(v0 * v0 + v1 * v1);
                        }
                        else if (complex == QString("相位") || complex == QString("Phase"))
                        {
                            double v0 = value[valindex];
                            double v1 = value1[valindex];
                            tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                        }
                        else
                        {
                            if (valindex == -1 && nval == 3 && compo == "Mag")
                            {
                                tempval = sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2] +
                                    value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2]);
                            }
                            else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                            {
                                tempval = sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
                            }
                            else
                                tempval = value[valindex];
                        }
                        if (varinfo._varType == 0)
                        {
                            componeData.value1.insert(cid, tempval);
                        }
                        else
                        {
                            componeData.value1.insert(id, tempval);
                        }

                        if (tempval > maxval)
                        {
                            maxval = tempval;
                            maxID = id;
                        }
                        if (tempval < minval)
                        {
                            minval = tempval;
                            minID = id;
                        }
                    }
                }

                /*QList<QPair<int, double>> temp{};
                temp.append(QPair<int, double>(maxID, maxval));
                temp.append(QPair<int, double>(minID, minval));*/
                componeData.varName = var;
                componeData.compoName = compo;
                /*outputDatabase->setFrameMaxMin(f, temp);
                outputDatabase->setComponeValues(*compones);
                outputDatabase->setTempData(*compones);*/
                //QHash<int,QVector3D> dis = outputDatabase->getDisplacementDataByFrameID(1);
                return true;
            }
        }
        return false;
    }

    // 已迁移到 MxdbReader::readElementNodeFaceValues
    bool mIMxdbFile1::readCloudData(const QString& pname, int f, const QString& var, const QString& compo, bool isexport)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;
        PostFrameInfo frameinfo = modelDatas.value(f)->_frameInfo;

        Compones* compones = new Compones();
        compones->isD = true;
        compones->frameID = f;

        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                const int compoi = varinfo._compoNames.indexOf(compo);

                double maxval = -DBL_MAX, minval = DBL_MAX;
                int maxID = 0, minID = 0;
                QHash<int, QHash<int, double>> compovalues;

                auto modelData = modelDatas.value(f);
                if (modelData == nullptr)
                    return false;

                compones->flag = 2;
                compones->isNodeFace = true;

                std::vector<MxdbReader::MxdbElementNodeFaceValue> values;
                if (!readElementNodeFaceValuesHelper(frameinfo._frameFile, varinfo, modelData, {}, values))
                {
                    delete compones;
                    return false;
                }

                for (const auto& val : values)
                {
                    int eleId = val.elementId;
                    const int nodeCount = static_cast<int>(val.nodeIds.size());
                    if (nodeCount == 0 || val.realValuesPerNode.empty())
                        continue;
                    const int comVarCount = static_cast<int>(val.realValuesPerNode[0].size());
                    if (comVarCount == 0)
                        continue;

                    QHash<int, double> values;
                    if (compoi + 1 > comVarCount)
                    {
                        for (int k = 0; k < nodeCount; ++k)
                        {
                            int nodeId = val.nodeIds[k];
                            double tempVal = 0.0;
                            values[nodeId] = tempVal;
                            if (tempVal > maxval) { maxval = tempVal; maxID = nodeId; }
                            if (tempVal < minval) { minval = tempVal; minID = nodeId; }
                        }
                    }
                    else
                    {
                        for (int k = 0; k < nodeCount; ++k)
                        {
                            int nodeId = val.nodeIds[k];
                            double tempVal = val.realValuesPerNode[k][compoi];
                            values[nodeId] = tempVal;
                            if (tempVal > maxval) { maxval = tempVal; maxID = nodeId; }
                            if (tempVal < minval) { minval = tempVal; minID = nodeId; }
                        }
                    }
                    compovalues[eleId] = values;
                }

                QList<QPair<int, double>> temp{};
                temp.append(QPair<int, double>(maxID, maxval));
                temp.append(QPair<int, double>(minID, minval));
                compones->dRValue = compovalues;
                outputDatabase->setComponeValues(*compones);
                compovalues.clear();
                return true;
            }
        }
        return false;
    }


    // 已迁移到 MxdbReader::readElementNodeFaceValues
    bool mIMxdbFile1::readCloudData(const QString& pname, int f, const QString& var, const QStringList& compo, std::set<int>& ids, QHash<QString, QHash<int, double>>& allVals, int& flag)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;
        PostFrameInfo frameinfo = modelDatas.value(f)->_frameInfo;

        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                auto modelData = modelDatas.value(f);
                if (modelData == nullptr)
                    return false;

                flag = 2;

                std::vector<MxdbReader::MxdbElementNodeFaceValue> values;
                if (!readElementNodeFaceValuesHelper(frameinfo._frameFile, varinfo, modelData, {}, values))
                    return false;

                for (const auto& val : values)
                {
                    const int nodeCount = static_cast<int>(val.nodeIds.size());
                    if (nodeCount == 0 || val.realValuesPerNode.empty())
                        continue;
                    const int comVarCount = static_cast<int>(val.realValuesPerNode[0].size());
                    if (comVarCount == 0)
                        continue;

                    for (const auto& comName : compo)
                    {
                        const int compoi = varinfo._compoNames.indexOf(comName);
                        QHash<int, double> values = allVals[comName];
                        for (int k = 0; k < nodeCount; ++k)
                        {
                            int internalId = val.nodeIds[k];
                            int realId = modelData->_nodedata.ids[internalId];
                            if (ids.count(realId) <= 0)
                                continue;
                            double tempVal = (compoi + 1 > comVarCount) ? 0.0 : val.realValuesPerNode[k][compoi];
                            values[realId] = tempVal;
                        }
                        allVals[comName] = values;
                    }
                }
                return true;
            }
        }
        return false;
    }


    // 已迁移到 MxdbReader::readElementNodeFaceValues
    bool mIMxdbFile1::readCloudData(const QString& pname, int f, const QString& var, const QStringList& compo, QHash<QString, QHash<int, double>>& allVals, int& flag)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;
        PostFrameInfo frameinfo = modelDatas.value(f)->_frameInfo;

        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return false;

                auto modelData = modelDatas.value(f);
                if (modelData == nullptr)
                    return false;

                flag = 2;

                std::vector<MxdbReader::MxdbElementNodeFaceValue> values;
                if (!readElementNodeFaceValuesHelper(frameinfo._frameFile, varinfo, modelData, {}, values))
                    return false;

                for (const auto& val : values)
                {
                    const int nodeCount = static_cast<int>(val.nodeIds.size());
                    if (nodeCount == 0 || val.realValuesPerNode.empty())
                        continue;
                    const int comVarCount = static_cast<int>(val.realValuesPerNode[0].size());
                    if (comVarCount == 0)
                        continue;

                    for (const auto& comName : compo)
                    {
                        const int compoi = varinfo._compoNames.indexOf(comName);
                        QHash<int, double> values = allVals[comName];
                        for (int k = 0; k < nodeCount; ++k)
                        {
                            int internalId = val.nodeIds[k];
                            int realId = modelData->_nodedata.ids[internalId];
                            double tempVal = (compoi + 1 > comVarCount) ? 0.0 : val.realValuesPerNode[k][compoi];
                            values[realId] = tempVal;
                        }
                        allVals[comName] = values;
                    }
                }
                return true;
            }
        }
        return false;
    }

    bool mIMxdbFile1::readElementNodeCompValues(const QString& pname, int f, const QString& var, QHash<int, QHash<int, QVector<double>>>& eleNodeCompVals, const std::vector<int>& elementIds)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return false;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return false;
        auto modelData = modelDatas.value(f);
        if (modelData == nullptr)
            return false;
        PostFrameInfo frameinfo = modelData->_frameInfo;

        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName != var)
                continue;

            bool isopen = openFileStatus(frameinfo._frameFile);
            if (!isopen)
                return false;

            std::vector<MxdbReader::MxdbElementNodeFaceValue> values;
            //elementIds非空时reader仅解码所选单元，其余单元seekg跳过（免全模型解码拷贝）
            if (!readElementNodeFaceValuesHelper(frameinfo._frameFile, varinfo, modelData, elementIds, values))
                return false;

            for (const auto& val : values)
            {
                const int nodeCount = static_cast<int>(val.nodeIds.size());
                if (nodeCount == 0 || val.realValuesPerNode.empty())
                    continue;

                QHash<int, QVector<double>> nodeVals;
                for (int k = 0; k < nodeCount; ++k)
                {
                    //连续id转真实节点id（与多分量出参重载同口径）
                    const int realId = modelData->_nodedata.ids[val.nodeIds[k]];
                    nodeVals.insert(realId, QVector<double>::fromStdVector(val.realValuesPerNode[k]));
                }
                eleNodeCompVals.insert(val.elementId, nodeVals);
            }
            return true;
        }
        return false;
    }


    void mIMxdbFile1::readDisplacementData(const QString& pname, int f, const QString& var)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return;
        QPointer<mPostModelData> modelData = modelDatas.value(f);
        if (modelData == nullptr)
            return;
        PostFrameInfo frameinfo = modelData->_frameInfo;
        const int nvar = frameinfo._frameVars.size();

        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == var)
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return;

                // 使用 MxdbReader 读取位移变量
                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                    return;

                QHash<int, QVector3D> dispvals;
                dispvals.reserve(values.size());
                for (const auto& v : values)
                {
                    int cid = modelData->_nodedata.nodeidmap.value(v.entityId);
                    if (v.realValues.size() >= 3)
                        dispvals.insert(cid, QVector3D(v.realValues[0], v.realValues[1], v.realValues[2]));
                }

                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    //输出信息
                    mOutputMessage tepmsg(NormalMessage, QString("第%1帧文件位移数据读取成功！").arg(f));
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("第%1帧文件位移数据读取成功！").arg(f), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    //输出信息
                    mOutputMessage tepmsg(NormalMessage, QString("The file displacement data of frame %1 was read successfully！").arg(f));
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("The file displacement data of frame %1 was read successfully！").arg(f), true);
                }


                outputDatabase->setVectorDataByFrameID(f, dispvals);
                dispvals.clear();
                return;
            }
        }
    }

    void mIMxdbFile1::readDisplacementDataResult(const QString& pname, int f, const QString& complexstr)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return;

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        if (f < 1 || f > modelDatas.size())
            return;
        auto modelData = modelDatas.value(f);
        if (modelData == nullptr)
            return;
        PostFrameInfo frameinfo = modelData->_frameInfo;
        const int nvar = frameinfo._frameVars.size();
        for (int i = 0; i < nvar; ++i)
        {
            PostVariableInfo varinfo = frameinfo._frameVars.at(i);
            if (varinfo._varName == QString("Displacement"))
            {
                //打开文件
                bool isopen = openFileStatus(frameinfo._frameFile);
                if (!isopen)
                    return;

                // 使用 MxdbReader 读取位移变量
                std::vector<MxdbReader::MxdbVariableValue> values;
                if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                    return;

                QHash<int, QVector3D> dispvals;
                dispvals.reserve(values.size());
                for (const auto& v : values)
                {
                    if (v.realValues.size() < 3 || v.imagValues.size() < 3)
                        continue;
                    const double* val = v.realValues.data();
                    const double* val1 = v.imagValues.data();
                    QVector3D valvec(val[0], val[1], val[2]);
                    if (complexstr == QString("实部"))
                    {
                        valvec = QVector3D(val[0], val[1], val[2]);
                    }
                    else if (complexstr == QString("虚部"))
                    {
                        valvec = QVector3D(val1[0], val1[1], val1[2]);
                    }
                    else if (complexstr == QString("幅值"))
                    {
                        double magx = sqrt(val[0] * val[0] + val1[0] * val1[0]);
                        double magy = sqrt(val[1] * val[1] + val1[1] * val1[1]);
                        double magz = sqrt(val[2] * val[2] + val1[2] * val1[2]);
                        valvec = QVector3D(magx, magy, magz);
                    }
                    else if (complexstr == QString("相位"))
                    {
                        double magx = atan(val[0] / val1[0]);
                        double magy = atan(val[1] / val1[1]);
                        double magz = atan(val[2] / val1[2]);
                        valvec = QVector3D(magx, magy, magz);
                    }

                    dispvals.insert(v.entityId, valvec);
                }
                modelData->DisplacementValuesR = dispvals;

                outputDatabase->setDisplacementDataByFrameID(f, dispvals);
                dispvals.clear();



                return;
            }
        }
    }

    void mIMxdbFile1::readCurveData1(const QString& pname, const QString& var, const QStringList& compos, const QString& layer, const QString& complex,
        const QVector<int>& ids, QStringList& cnames, QVector<double>& xvalues, QHash<QString, QVector<double>>& yvalues, QString caseName)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return;

        if (ids.isEmpty())
            return;

        QHash<int, QVector<QString>> idCurNames{};

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        //QList<FrameInfo> frameInfo = dp->getFrameInfos();
        const int nframe = modelDatas.size();
        for (int i = 0; i < nframe; ++i)
        {
            auto modelData = modelDatas.value(i + 1);
            if (modelData == nullptr)
                continue;
            PostFrameInfo finfo = modelData->_frameInfo;
            if (caseName.compare("All") == 0 || finfo._frameCase.compare(caseName) == 0)
            {
                const int nvar = finfo._frameVars.size();
                for (int ii = 0; ii < nvar; ++ii)
                {
                    PostVariableInfo varinfo = finfo._frameVars.at(ii);
                    if (varinfo._varName == var)
                    {
                        //打开文件
                        bool isopen = openFileStatus(finfo._frameFile);
                        if (!isopen)
                            return;

                        std::vector<int> targetIds;
                        targetIds.reserve(ids.size());
                        for (int id : ids)
                            targetIds.push_back(id);

                        std::vector<MxdbReader::MxdbVariableValue> values;
                        if (!readVariableValuesHelper(finfo._frameFile, varinfo, targetIds, values))
                            return;

                        //获取分量数
                        const int ncompo = varinfo._nCompo;

                        //生成X数据
                        xvalues.append(finfo._frameTime);

                        //获取当前变量中的变量个数
                        const int nresult = varinfo._varCount;
                        for (int k = 0; k < nresult; ++k)
                        {
                            //获取层数及层索引
                            const int nlayer = varinfo._varLayer.at(k);
                            const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);
                            //计算分量值个数
                            int nval;
                            if (nlayer == 0)
                            {
                                if (ncompo == 0)
                                    nval = 1;
                                else
                                    nval = ncompo;
                            }
                            else
                            {
                                if (ncompo == 0)
                                    nval = nlayer;
                                else
                                    nval = ncompo * nlayer;
                            }

                            // 计算当前 shape 在 value 数组中的偏移
                            int offset = 0;
                            for (int kk = 0; kk < k; ++kk)
                            {
                                const int prevLayer = varinfo._varLayer.at(kk);
                                const int prevNval = (prevLayer == 0) ? (ncompo == 0 ? 1 : ncompo) : (ncompo == 0 ? prevLayer : ncompo * prevLayer);
                                offset += prevNval;
                            }

                            //读取变量值
                            for (const auto& v : values)
                            {
                                if (offset + nval > static_cast<int>(v.realValues.size()))
                                    continue;

                                const int id = v.entityId;
                                const double* val = v.realValues.data() + offset;
                                const double* val1 = v.imagValues.data() + offset;

                                for (QString compo : compos)
                                {
                                    //曲线名称
                                    QString cname = QString("Time-%1_%2_%3_%4-ID%5").arg(var).arg(compo).arg(layer).arg(complex).arg(id);
                                    QVector<QString> vec = idCurNames.value(id);
                                    if (!vec.contains(cname))
                                    {
                                        vec.push_back(cname);
                                        idCurNames.insert(id, vec);
                                    }


                                    //获取当前分量的索引
                                    const int compoi = varinfo._compoNames.indexOf(compo);
                                    //计算分量的索引
                                    int valindex;
                                    if (nlayer == 0)
                                    {
                                        if (ncompo == 0)
                                            valindex = 0;
                                        else
                                            valindex = compoi;
                                    }
                                    else
                                    {
                                        if (ncompo == 0)
                                            valindex = layerindex + compoi;
                                        else
                                            valindex = ncompo * layerindex + compoi;
                                    }
                                    //取值
                                    double tempval;
                                    if (complex == QString("实部"))
                                        tempval = val[valindex];
                                    else if (complex == QString("虚部"))
                                        tempval = val1[valindex];
                                    else if (complex == QString("幅值"))
                                    {
                                        double v0 = val[valindex];
                                        double v1 = val1[valindex];
                                        tempval = sqrt(v0 * v0 + v1 * v1);
                                    }
                                    else if (complex == QString("相位"))
                                    {
                                        double v0 = val[valindex];
                                        double v1 = val1[valindex];

                                        const double PI = std::acos(-1);

                                        double phase_rad = std::atan2(v1, v0);
                                        double phase_deg = phase_rad * 180.0 / PI;
                                        phase_deg = std::fmod(phase_deg, 360.0);
                                        if (phase_deg < 0) {
                                            phase_deg += 360.0;
                                        }
                                        if (std::abs(phase_deg - 360.0) < 1e-10) {
                                            phase_deg = 0.0;
                                        }

                                        tempval = phase_deg;
                                    }
                                    else
                                    {
                                        if (valindex == -1 && nval == 3 && compo == "Mag")
                                        {
                                            tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2] +
                                                val1[0] * val1[0] + val1[1] * val1[1] + val1[2] * val1[2]);
                                        }
                                        else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                                        {
                                            tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
                                        }
                                        else
                                        {
                                            tempval = val[valindex];
                                        }
                                    }
                                    //存储
                                    QVector<double> tempvec;
                                    if (yvalues.contains(cname))
                                        tempvec = yvalues.value(cname);
                                    tempvec.append(tempval);
                                    yvalues.insert(cname, tempvec);
                                }

                                if (compos.size() == 0)
                                {
                                    //曲线名称
                                    QString cname = QString("Time-%1_%2_%3-ID%4").arg(var).arg(layer).arg(complex).arg(id);
                                    QVector<QString> vec = idCurNames.value(id);
                                    if (!vec.contains(cname))
                                    {
                                        vec.push_back(cname);
                                        idCurNames.insert(id, vec);
                                    }

                                    /** 该变量没有分量 */
                                    //存储
                                    QVector<double> tempvec;
                                    if (yvalues.contains(cname))
                                        tempvec = yvalues.value(cname);
                                    tempvec.append(val[0]);
                                    yvalues.insert(cname, tempvec);
                                }
                            }
                        }

                        for (int id : ids)
                        {
                            QVector<QString> vec = idCurNames.value(id);
                            for (auto it : vec)
                            {
                                cnames.push_back(it);
                            }
                        }
                    }
                }
            }
        }
    }

    void mIMxdbFile1::readCurveData2(const QString& pname, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QVector<int>& ids,
        const set<int>& frameIds, QStringList& cnames, QHash<QString, QHash<int, double>>& yvalues, QString caseName)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return;

        if (ids.isEmpty())
            return;

        QHash<int, QVector<QString>> idCurNames{};

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        const int nframe = modelDatas.size();
        for (int i = 0; i < nframe; ++i)
        {
            PostFrameInfo finfo = modelDatas.value(i + 1)->_frameInfo;
            if (caseName.compare("All") == 0 || finfo._frameCase.compare(caseName) == 0)
            {
                if (frameIds.find(finfo._frameNum) != frameIds.end())
                {
                    const int nvar = finfo._frameVars.size();
                    for (int ii = 0; ii < nvar; ++ii)
                    {
                        PostVariableInfo varinfo = finfo._frameVars.at(ii);
                        if (varinfo._varName == var)
                        {
                            //打开文件
                            bool isopen = openFileStatus(finfo._frameFile);
                            if (!isopen)
                                return;

                            std::vector<int> targetIds;
                            targetIds.reserve(ids.size());
                            for (int id : ids)
                                targetIds.push_back(id);

                            std::vector<MxdbReader::MxdbVariableValue> values;
                            if (!readVariableValuesHelper(finfo._frameFile, varinfo, targetIds, values))
                                return;

                            //获取分量数
                            const int ncompo = varinfo._nCompo;

                            //获取当前变量中的变量个数
                            const int nresult = varinfo._varCount;
                            for (int k = 0; k < nresult; ++k)
                            {
                                //获取层数及层索引
                                const int nlayer = varinfo._varLayer.at(k);
                                const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);
                                //计算分量值个数
                                int nval;
                                if (nlayer == 0)
                                {
                                    if (ncompo == 0)
                                        nval = 1;
                                    else
                                        nval = ncompo;
                                }
                                else
                                {
                                    if (ncompo == 0)
                                        nval = nlayer;
                                    else
                                        nval = ncompo * nlayer;
                                }

                                // 计算当前 shape 在 value 数组中的偏移
                                int offset = 0;
                                for (int kk = 0; kk < k; ++kk)
                                {
                                    const int prevLayer = varinfo._varLayer.at(kk);
                                    const int prevNval = (prevLayer == 0) ? (ncompo == 0 ? 1 : ncompo) : (ncompo == 0 ? prevLayer : ncompo * prevLayer);
                                    offset += prevNval;
                                }

                                //读取变量值
                                for (const auto& v : values)
                                {
                                    const int id = v.entityId;
                                    if (ids.contains(id))
                                    {
                                        if (offset + nval > static_cast<int>(v.realValues.size()))
                                            continue;

                                        const double* val = v.realValues.data() + offset;
                                        const double* val1 = v.imagValues.data() + offset;

                                        for (QString compo : compos)
                                        {
                                            //曲线名称
                                            QString cname = QString("ID-%1_%2_%3_%4-f%5").arg(var).arg(compo).arg(layer).arg(complex).arg(finfo._frameNum);
                                            QVector<QString> vec = idCurNames.value(id);
                                            if (!vec.contains(cname))
                                            {
                                                vec.push_back(cname);
                                                idCurNames.insert(id, vec);
                                            }

                                            //获取当前分量的索引
                                            const int compoi = varinfo._compoNames.indexOf(compo);
                                            //计算分量的索引
                                            int valindex;
                                            if (nlayer == 0)
                                            {
                                                if (ncompo == 0)
                                                    valindex = 0;
                                                else
                                                    valindex = compoi;
                                            }
                                            else
                                            {
                                                if (ncompo == 0)
                                                    valindex = layerindex + compoi;
                                                else
                                                    valindex = ncompo * layerindex + compoi;
                                            }
                                            //取值
                                            double tempval;
                                            if (complex == QString("实部"))
                                                tempval = val[valindex];
                                            else if (complex == QString("虚部"))
                                                tempval = val1[valindex];
                                            else if (complex == QString("幅值"))
                                            {
                                                double v0 = val[valindex];
                                                double v1 = val1[valindex];
                                                tempval = sqrt(v0 * v0 + v1 * v1);
                                            }
                                            else if (complex == QString("相位"))
                                            {
                                                double v0 = val[valindex];
                                                double v1 = val1[valindex];
                                                tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                                            }
                                            else
                                            {
                                                if (valindex == -1 && nval == 3 && compo == "Mag")
                                                {
                                                    tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2] +
                                                        val1[0] * val1[0] + val1[1] * val1[1] + val1[2] * val1[2]);
                                                }
                                                else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                                                {
                                                    tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
                                                }
                                                else
                                                {
                                                    tempval = val[valindex];
                                                }
                                            }
                                            //存储
                                            QHash<int, double> tempvec;
                                            if (yvalues.contains(cname))
                                                tempvec = yvalues.value(cname);
                                            tempvec.insert(id, tempval);
                                            yvalues.insert(cname, tempvec);
                                        }

                                        if (compos.size() == 0)
                                        {
                                            //曲线名称
                                            QString cname = QString("ID-%1_%2_%3-f%4").arg(id).arg(var).arg(complex).arg(finfo._frameNum);
                                            QVector<QString> vec = idCurNames.value(id);
                                            if (!vec.contains(cname))
                                            {
                                                vec.push_back(cname);
                                                idCurNames.insert(id, vec);
                                            }
                                            QHash<int, double> tempvec;
                                            if (yvalues.contains(cname))
                                                tempvec = yvalues.value(cname);
                                            tempvec.insert(id, val[0]);
                                            yvalues.insert(cname, tempvec);
                                        }
                                    }
                                }
                            }

                            for (int id : ids)
                            {
                                QVector<QString> vec = idCurNames.value(id);
                                for (auto it : vec)
                                {
                                    cnames.push_back(it);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    void mIMxdbFile1::readCurveData3(const QString& pname, const QString& varx, const QString& compox, const QString& layerx, const QString& complexx,
        const QString& vary, const QStringList& composy, const QString& layery, const QString& complexy,
        const QVector<int>& ids, QStringList& cnames, QVector<double>& xvalues, QHash<QString, QVector<double>>& yvalues, QString caseName)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return;

        if (ids.isEmpty())
            return;

        QHash<int, QVector<QString>> idCurNames{};
        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        const int nframe = modelDatas.size();
        for (int i = 0; i < nframe; ++i)
        {
            PostFrameInfo finfo = modelDatas.value(i + 1)->_frameInfo;
            if (caseName.compare("All") == 0 || finfo._frameCase.compare(caseName) == 0)
            {
                const int nvar = finfo._frameVars.size();
                for (int ii = 0; ii < nvar; ++ii)
                {
                    PostVariableInfo varinfo = finfo._frameVars.at(ii);

                    //X值
                    if (varinfo._varName == varx)
                    {
                        //打开文件
                        bool isopen = openFileStatus(finfo._frameFile);
                        if (!isopen)
                            return;

                        std::vector<int> targetIds;
                        targetIds.reserve(ids.size());
                        for (int id : ids)
                            targetIds.push_back(id);

                        std::vector<MxdbReader::MxdbVariableValue> xvaluesData;
                        if (!readVariableValuesHelper(finfo._frameFile, varinfo, targetIds, xvaluesData))
                            return;

                        //获取分量数
                        const int ncompo = varinfo._nCompo;

                        //获取当前变量中的变量个数
                        const int nresult = varinfo._varCount;
                        for (int k = 0; k < nresult; ++k)
                        {
                            //获取层数及层索引
                            const int nlayer = varinfo._varLayer.at(k);
                            const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layerx);
                            //计算分量值个数及所需值的索引
                            int nval;
                            if (nlayer == 0)
                            {
                                if (ncompo == 0)
                                    nval = 1;
                                else
                                    nval = ncompo;
                            }
                            else
                            {
                                if (ncompo == 0)
                                    nval = nlayer;
                                else
                                    nval = ncompo * nlayer;
                            }

                            // 计算当前 shape 在 value 数组中的偏移
                            int offset = 0;
                            for (int kk = 0; kk < k; ++kk)
                            {
                                const int prevLayer = varinfo._varLayer.at(kk);
                                const int prevNval = (prevLayer == 0) ? (ncompo == 0 ? 1 : ncompo) : (ncompo == 0 ? prevLayer : ncompo * prevLayer);
                                offset += prevNval;
                            }

                            //读取变量值
                            for (const auto& v : xvaluesData)
                            {
                                const int id = v.entityId;
                                if (ids.contains(id))
                                {
                                    if (offset + nval > static_cast<int>(v.realValues.size()))
                                        continue;

                                    const double* val = v.realValues.data() + offset;
                                    const double* val1 = v.imagValues.data() + offset;

                                    //获取当前分量的索引
                                    const int compoi = varinfo._compoNames.indexOf(compox);
                                    //计算分量的索引
                                    int valindex;
                                    if (nlayer == 0)
                                    {
                                        if (ncompo == 0)
                                            valindex = 0;
                                        else
                                            valindex = compoi;
                                    }
                                    else
                                    {
                                        if (ncompo == 0)
                                            valindex = layerindex + compoi;
                                        else
                                            valindex = ncompo * layerindex + compoi;
                                    }
                                    //取值
                                    double tempval;
                                    if (complexx == QString("实部"))
                                        tempval = val[valindex];
                                    else if (complexx == QString("虚部"))
                                        tempval = val1[valindex];
                                    else if (complexx == QString("幅值"))
                                    {
                                        double v0 = val[valindex];
                                        double v1 = val1[valindex];
                                        tempval = sqrt(v0 * v0 + v1 * v1);
                                    }
                                    else if (complexx == QString("相位"))
                                    {
                                        double v0 = val[valindex];
                                        double v1 = val1[valindex];
                                        tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                                    }
                                    else
                                    {
                                        if (valindex == -1 && nval == 3 && compox == "Mag")
                                        {
                                            tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2] +
                                                val1[0] * val1[0] + val1[1] * val1[1] + val1[2] * val1[2]);
                                        }
                                        else if (valindex == -1 && nval == 3 && compox == "Mag cos(wt+phase)")
                                        {
                                            tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
                                        }
                                        else
                                        {
                                            tempval = val[valindex];
                                        }
                                    }
                                    //存储
                                    xvalues.append(tempval);
                                }
                            }
                        }
                    }

                    //Y值
                    if (varinfo._varName == vary)
                    {
                        //打开文件
                        bool isopen = openFileStatus(finfo._frameFile);
                        if (!isopen)
                            return;

                        std::vector<int> targetIds;
                        targetIds.reserve(ids.size());
                        for (int id : ids)
                            targetIds.push_back(id);

                        std::vector<MxdbReader::MxdbVariableValue> yvaluesData;
                        if (!readVariableValuesHelper(finfo._frameFile, varinfo, targetIds, yvaluesData))
                            return;

                        //获取分量数
                        const int ncompo = varinfo._nCompo;

                        //获取当前变量中的变量个数
                        const int nresult = varinfo._varCount;
                        for (int k = 0; k < nresult; ++k)
                        {
                            //获取层数及层索引
                            const int nlayer = varinfo._varLayer.at(k);
                            const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layery);
                            //计算分量值个数及所需值的索引
                            int nval;
                            if (nlayer == 0)
                            {
                                if (ncompo == 0)
                                    nval = 1;
                                else
                                    nval = ncompo;
                            }
                            else
                            {
                                if (ncompo == 0)
                                    nval = nlayer;
                                else
                                    nval = ncompo * nlayer;
                            }

                            // 计算当前 shape 在 value 数组中的偏移
                            int offset = 0;
                            for (int kk = 0; kk < k; ++kk)
                            {
                                const int prevLayer = varinfo._varLayer.at(kk);
                                const int prevNval = (prevLayer == 0) ? (ncompo == 0 ? 1 : ncompo) : (ncompo == 0 ? prevLayer : ncompo * prevLayer);
                                offset += prevNval;
                            }

                            //读取变量值
                            for (const auto& v : yvaluesData)
                            {
                                const int id = v.entityId;
                                if (ids.contains(id))
                                {
                                    if (offset + nval > static_cast<int>(v.realValues.size()))
                                        continue;

                                    const double* val = v.realValues.data() + offset;
                                    const double* val1 = v.imagValues.data() + offset;

                                    for (QString compo : composy)
                                    {
                                        //曲线名称
                                        QString cname = QString("%1_%2_%3_%4-%5_%6_%7_%8-ID%9").arg(varx).arg(compox).arg(layerx).arg(complexx)
                                            .arg(vary).arg(compo).arg(layery).arg(complexy).arg(id);
                                        QVector<QString> vec = idCurNames.value(id);
                                        if (!vec.contains(cname))
                                        {
                                            vec.push_back(cname);
                                            idCurNames.insert(id, vec);
                                        }

                                        //获取当前分量的索引
                                        const int compoi = varinfo._compoNames.indexOf(compo);
                                        //计算分量的索引
                                        int valindex;
                                        if (nlayer == 0)
                                        {
                                            if (ncompo == 0)
                                                valindex = 0;
                                            else
                                                valindex = compoi;
                                        }
                                        else
                                        {
                                            if (ncompo == 0)
                                                valindex = layerindex + compoi;
                                            else
                                                valindex = ncompo * layerindex + compoi;
                                        }
                                        //取值
                                        double tempval;
                                        if (complexy == QString("实部"))
                                            tempval = val[valindex];
                                        else if (complexy == QString("虚部"))
                                            tempval = val1[valindex];
                                        else if (complexy == QString("幅值"))
                                        {
                                            double v0 = val[valindex];
                                            double v1 = val1[valindex];
                                            tempval = sqrt(v0 * v0 + v1 * v1);
                                        }
                                        else if (complexy == QString("相位"))
                                        {
                                            double v0 = val[valindex];
                                            double v1 = val1[valindex];
                                            tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                                        }
                                        else
                                        {
                                            if (valindex == -1 && nval == 3 && compo == "Mag")
                                            {
                                                tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2] +
                                                    val1[0] * val1[0] + val1[1] * val1[1] + val1[2] * val1[2]);
                                            }
                                            else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                                            {
                                                tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
                                            }
                                            else
                                            {
                                                tempval = val[valindex];
                                            }
                                        }
                                        //存储
                                        QVector<double> tempvec;
                                        if (yvalues.contains(cname))
                                            tempvec = yvalues.value(cname);
                                        tempvec.append(tempval);
                                        yvalues.insert(cname, tempvec);
                                    }
                                }
                            }
                        }

                        for (int id : ids)
                        {
                            QVector<QString> vec = idCurNames.value(id);
                            for (auto it : vec)
                            {
                                cnames.push_back(it);
                            }
                        }
                    }
                }
            }
        }
    }
    bool mIMxdbFile1::readFlattenStress(const QString& pname, int f, const QString& var, const QString& compo, const QString& layer, const QString& complex, const QString& global, QVector<double> local, bool isexport)
    {
        //if (QString(var) == "Stress(E)" || QString(var) == "Strain(E)" || QString(var) == "PSD Stress(E)" || QString(var) == "PSD Strain(E)"
        //    || QString(var) == "RMS Stress(E)" || QString(var) == "1 Segalman RMS Stress(E)" || QString(var) == "2 Segalman RMS Stress(E)" || QString(var) == "3 Segalman RMS Stress(E)"
        //    || QString(var) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
        {
            auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
            if (outputDatabase == nullptr)
                return false;
            //获取局部坐标系
            //坐标系
            QVector<double> coordinate = {
                1,0,0,
                0,1,0,
                0,0,1
            };
            if (global == QString("Global"))
            {
                coordinate = {
                    1,0,0,
                    0,1,0,
                    0,0,1
                };

            }
            else if (global == "")
            {
                coordinate = {
                    1,0,0,
                    0,1,0,
                    0,0,1
                };

            }
            else
            {
                coordinate = outputDatabase->getAllcoordinate(global);
            }

            QVector<double> userCoord = local;

            QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
            if (f < 1 || f > modelDatas.size())
                return false;

            auto modelData = modelDatas.value(f);

            if (modelData == nullptr)
                return false;

            PostFrameInfo frameinfo = modelData->_frameInfo;

            Compones* compones = new Compones();
            compones->isD = true;
            compones->frameID = f;
            QHash<int, double> nodeValues{};
            const int nvar = frameinfo._frameVars.size();
            for (int i = 0; i < nvar; ++i)
            {
                PostVariableInfo varinfo = frameinfo._frameVars.at(i);
                if (varinfo._varName == var)
                {
                    //打开文件
                    bool isopen = openFileStatus(frameinfo._frameFile);
                    if (!isopen)
                        return false;

                    // 使用 MxdbReader 读取变量
                    std::vector<MxdbReader::MxdbVariableValue> values;
                    if (!readVariableValuesHelper(frameinfo._frameFile, varinfo, {}, values))
                        return false;

                    //获取分量数
                    const int ncompo = varinfo._nCompo;

                    //获取当前分量的索引
                    const int compoi = varinfo._compoNames.indexOf(compo);

                    //存储值
                    double maxval = -DBL_MAX, minval = DBL_MAX;
                    //存储ID值
                    int maxID = 0, minID = 0;

                    //compones->flag = varinfo._varType;
                    compones->flag = 0;

                    //获取当前变量中的变量分量个数
                    const int nresult = varinfo._varCount;
                    for (int k = 0; k < nresult; ++k)
                    {
                        //获取层数及层索引
                        const int nlayer = varinfo._varLayer.at(k);
                        const int layerindex = varinfo._varLayerNames1.at(k).indexOf(layer);

                        //计算分量值个数及所需值的索引
                        int nval, valindex;
                        if (nlayer == 0)
                        {
                            if (ncompo == 0)
                            {
                                nval = 1;
                                valindex = 0;
                            }
                            else
                            {
                                nval = ncompo;
                                valindex = compoi;
                            }
                        }
                        else
                        {
                            if (ncompo == 0)
                            {
                                nval = nlayer;
                                valindex = layerindex + compoi;
                            }
                            else
                            {
                                nval = ncompo * nlayer;
                                valindex = ncompo * layerindex + compoi;
                            }
                        }

                        // 计算当前 shape 在 value 数组中的偏移
                        int offset = 0;
                        for (int kk = 0; kk < k; ++kk)
                        {
                            const int prevLayer = varinfo._varLayer.at(kk);
                            const int prevNval = (prevLayer == 0) ? (ncompo == 0 ? 1 : ncompo) : (ncompo == 0 ? prevLayer : ncompo * prevLayer);
                            offset += prevNval;
                        }

                        for (const auto& val : values)
                        {
                            int id = val.entityId;
                            if (offset + nval > static_cast<int>(val.realValues.size()))
                                continue;

                            const double* value = val.realValues.data() + offset;
                            const double* value1 = val.imagValues.data() + offset;
                            QVector<double> dis;
                            for (int i = 0; i < nval; i++)
                                dis.push_back(value[i]);

                            //场变量局部柱坐标系转化
                            if (userCoord.size() > 0 && (compo == "X" || compo == "Y" || compo == "Z" ||
                                compo == "XX" || compo == "YY" || compo == "ZZ" || compo == "XY" ||
                                compo == "YZ" || compo == "XZ"))
                            {
                                QVector3D oldVec;
                                oldVec.setX(dis[0]);
                                oldVec.setY(dis[1]);
                                oldVec.setZ(dis[2]);

                                if (userCoord.size() % 3 != 0) return false;
                                QVector<QVector3D> vector3Ds;
                                int numVectors = userCoord.size() / 3;
                                for (int i = 0; i < numVectors; ++i)
                                {
                                    int index = i * 3;
                                    double x = userCoord[index];
                                    double y = userCoord[index + 1];
                                    double z = userCoord[index + 2];
                                    vector3Ds.append(QVector3D(x, y, z));
                                }

                                QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
                                dis[0] = newVec[0];
                                dis[1] = newVec[1];
                                dis[2] = newVec[2];
                            }

                            double tempval;
                            if (complex == QString("实部") || complex == QString("Real"))
                            {
                                tempval = value[valindex];
                            }
                            else if (complex == QString("虚部") || complex == QString("Img"))
                            {
                                tempval = value1[valindex];
                            }
                            else if (complex == QString("幅值") || complex == QString("Amplitude"))
                            {
                                double v0 = value[valindex];
                                double v1 = value1[valindex];
                                tempval = sqrt(v0 * v0 + v1 * v1);
                            }
                            else if (complex == QString("相位") || complex == QString("Phase"))
                            {
                                double v0 = value[valindex];
                                double v1 = value1[valindex];
                                tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                            }
                            else
                            {
                                // 这里计算mag合量值
                                if (valindex == -1 && nval == 3 && compo == "Mag")
                                {
                                    tempval = sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2] +
                                        value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2]);
                                }
                                else if (valindex == -1 && nval == 3 && compo == "Mag cos(wt+phase)")
                                {
                                    tempval = sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
                                }
                                else
                                    tempval = value[valindex];
                            }

                            if (varinfo._varType == 0)
                            {
                                //节点实体变量（如求解器已磨平的 Element Stress(E, Simple Average)）：
                                //实体即节点，rid→cid 直接落值——按单元散布会把节点值错配到
                                //同号单元的节点上，空间分布错乱成噪点
                                const int nodeCid = modelData->_nodedata.nodeidmap.value(id, -1);
                                if (nodeCid >= 0)
                                    nodeValues.insert(nodeCid, tempval);
                            }
                            else
                            {
                            //使用part->_parteledata.cell_node_ids获取节点索引（cid），与nodeEleCountmap的key类型保持一致
                            std::list<QPointer<mPostPartData>> parts = modelData->getAllPartData();
                            for (auto part : parts)
                            {
                                if (part == nullptr)
                                    continue;

                                //查找当前单元在部件中的索引
                                for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                {
                                    int ele_rid = part->_parteledata.cell_rid[ele_cid];
                                    if (ele_rid != id)
                                        continue;

                                    int type = part->_parteledata.cell_info[ele_cid].type;
                                    int pos = part->_parteledata.cell_node_offsets[ele_cid];
                                    int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
                                    if (nodeCount > 0)
                                    {
                                        for (int j = 0; j < nodeCount; ++j)
                                        {
                                            int nodeId = part->_parteledata.cell_node_ids[pos];
                                            if (nodeValues.constFind(nodeId) == nodeValues.cend())
                                            {
                                                nodeValues.insert(nodeId, tempval);
                                            }
                                            else
                                            {
                                                nodeValues[nodeId] += tempval;
                                            }
                                            ++pos;
                                        }
                                    }
                                    break;
                                }
                            }
                            }
                        }
                    }

                    //节点实体变量的值已是每节点一份（多为求解器已磨平结果），不做单元数平均；
                    //单元变量才需要"累加值/节点单元数"的简单磨平
                    if (varinfo._varType != 0)
                    {
                        for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
                        {
                            double val = nodeValues.value(id, 0.0);
                            nodeValues[id] = val / modelData->_nodedata.nodeEleCountmap.value(id);
                        }
                    }
                    compones->value1 = nodeValues;

                    outputDatabase->setComponeValues(*compones);
                    return true;
                }
            }
            return false;
        }
        return false;
    }

    QVector<double> mIMxdbFile1::multiplyMatrixVector(const QVector<double>& matrix, const QVector<double>& vector)
    {
        QVector<double> result;
        if (matrix.size() != 9)
        {
            return result;
        }

        //向量不足3个分量时按0补齐，避免异常数据越界（返回值恒为3个，保持调用方约定）
        const double v0 = vector.size() > 0 ? vector[0] : 0.0;
        const double v1 = vector.size() > 1 ? vector[1] : 0.0;
        const double v2 = vector.size() > 2 ? vector[2] : 0.0;

        result.append(matrix[0] * v0 + matrix[1] * v1 + matrix[2] * v2);
        result.append(matrix[3] * v0 + matrix[4] * v1 + matrix[5] * v2);
        result.append(matrix[6] * v0 + matrix[7] * v1 + matrix[8] * v2);

        return result;
    }

    QHash<int, float> mIMxdbFile1::getRendDataValue() const
    {
        return _rendDataValue;
    }

    QHash<int, QVector3D> mIMxdbFile1::getDisRendDataValue() const
    {
        return _disDataValue;
    }

    void mIMxdbFile1::setProgressBarValue(std::streampos pos)
    {
        if (_isAI) return;
        double num = (double)pos / (double)_fileSize;
        quint64 tempNum = num * 80;
        int value = tempNum;
        mGlobalSignals::getInstance()->setProgressBarValue(value);
    }

    void mIMxdbFile1::readTimeVarCurve(const QString& pname, const QString& var, QList<CurveKey> timeCurves, const QVector<int>& ids, QStringList& cnames, QVector<double>& xvalues, QHash<QString, QVector<double>>& yvalues)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return;

        if (ids.isEmpty())
            return;

        QHash<int, QVector<QString>> idCurNames{};

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();

        const int nframe = modelDatas.size();
        for (int i = 0; i < nframe; ++i)
        {
            auto modelData = modelDatas.value(i + 1);
            if (modelData == nullptr)
                continue;
            PostFrameInfo finfo = modelData->_frameInfo;
            const int nvar = finfo._frameVars.size();
            for (int ii = 0; ii < nvar; ++ii)
            {
                PostVariableInfo varinfo = finfo._frameVars.at(ii);
                if (varinfo._varName == var)
                {
                    //打开文件
                    bool isopen = openFileStatus(finfo._frameFile);
                    if (!isopen)
                        return;

                    std::vector<int> targetIds;
                    targetIds.reserve(ids.size());
                    for (int id : ids)
                        targetIds.push_back(id);

                    std::vector<MxdbReader::MxdbVariableValue> values;
                    if (!readVariableValuesHelper(finfo._frameFile, varinfo, targetIds, values))
                        return;

                    // 如果按目标 ID 过滤后为空，尝试读取全部，看看有效 entity 集合与 ids 的交集
                    if (values.empty() && !ids.isEmpty()) {
                        std::vector<MxdbReader::MxdbVariableValue> allValues;
                        if (readVariableValuesHelper(finfo._frameFile, varinfo, {}, allValues)) {
                            QSet<int> validIds;
                            for (const auto& v : allValues) validIds.insert(v.entityId);
                            QSet<int> pickIds;
                            for (int id : ids) pickIds.insert(id);
                            QSet<int> inter = validIds & pickIds;

                            if (inter.isEmpty()) {
                                QString objType = (varinfo._varType == 0) ? QString("节点") : QString("单元");
                                mGlobalSignals::getInstance()->outputMessageSig(
                                    2,
                                    QString("当前拾取的%1不在变量[%2]的有效范围内，请重新拾取。")
                                    .arg(objType).arg(var),
                                    true);
                            }
                        }
                    }

                    //获取分量数
                    const int ncompo = varinfo._nCompo;

                    //生成X数据
                    xvalues.append(finfo._frameTime);

                    //获取当前变量中的变量个数
                    const int nresult = varinfo._varCount;
                    for (int k = 0; k < nresult; ++k)
                    {
                        //获取层数及层索引
                        const int nlayer = varinfo._varLayer.at(k);
                        //计算分量值个数
                        int nval;
                        if (nlayer == 0)
                        {
                            if (ncompo == 0)
                                nval = 1;
                            else
                                nval = ncompo;
                        }
                        else
                        {
                            if (ncompo == 0)
                                nval = nlayer;
                            else
                                nval = ncompo * nlayer;
                        }

                        // 计算当前 shape 在 value 数组中的偏移
                        int offset = 0;
                        for (int kk = 0; kk < k; ++kk)
                        {
                            const int prevLayer = varinfo._varLayer.at(kk);
                            const int prevNval = (prevLayer == 0) ? (ncompo == 0 ? 1 : ncompo) : (ncompo == 0 ? prevLayer : ncompo * prevLayer);
                            offset += prevNval;
                        }

                        //读取变量值
                        for (const auto& v : values)
                        {
                            const int id = v.entityId;
                            if (ids.contains(id))
                            {
                                if (offset + nval > static_cast<int>(v.realValues.size()))
                                    continue;

                                const double* val = v.realValues.data() + offset;
                                const double* val1 = v.imagValues.data() + offset;

                                for (CurveKey timeCurve : timeCurves)
                                {
                                    const int layerindex = varinfo._varLayerNames1.at(k).indexOf(timeCurve.layer);
                                    if (nlayer > 0 && layerindex < 0)
                                        continue;

                                    //曲线名称
                                    QString cname = QString("Time-%1_%2-ID%3").arg(var).arg(timeCurve.toString()).arg(id);
                                    QVector<QString> vec = idCurNames.value(id);
                                    if (!vec.contains(cname))
                                    {
                                        vec.push_back(cname);
                                        idCurNames.insert(id, vec);
                                    }

                                    //获取当前分量的索引
                                    const int compoi = varinfo._compoNames.indexOf(timeCurve.component);
                                    bool isMag = (timeCurve.component == "Mag" || timeCurve.component == "Mag cos(wt+phase)");
                                    if (ncompo > 0 && compoi < 0 && !(isMag && nval == 3))
                                        continue;

                                    //计算分量的索引
                                    int valindex;
                                    if (isMag && nval == 3)
                                    {
                                        valindex = -1;  // Mag 合量走特殊分支
                                    }
                                    else if (nlayer == 0)
                                    {
                                        if (ncompo == 0)
                                            valindex = 0;
                                        else
                                            valindex = compoi;
                                    }
                                    else
                                    {
                                        if (ncompo == 0)
                                            valindex = layerindex;
                                        else
                                            valindex = ncompo * layerindex + compoi;
                                    }

                                    if (valindex < -1 || valindex >= nval)
                                        continue;

                                    //取值
                                    double tempval;
                                    if (timeCurve.cplx == QString("实部"))
                                        tempval = val[valindex];
                                    else if (timeCurve.cplx == QString("虚部"))
                                        tempval = val1[valindex];
                                    else if (timeCurve.cplx == QString("幅值"))
                                    {
                                        double v0 = val[valindex];
                                        double v1 = val1[valindex];
                                        tempval = sqrt(v0 * v0 + v1 * v1);
                                    }
                                    else if (timeCurve.cplx == QString("相位"))
                                    {
                                        double v0 = val[valindex];
                                        double v1 = val1[valindex];

                                        const double PI = std::acos(-1);

                                        double phase_rad = std::atan2(v1, v0);
                                        double phase_deg = phase_rad * 180.0 / PI;
                                        phase_deg = std::fmod(phase_deg, 360.0);
                                        if (phase_deg < 0) {
                                            phase_deg += 360.0;
                                        }
                                        if (std::abs(phase_deg - 360.0) < 1e-10) {
                                            phase_deg = 0.0;
                                        }

                                        tempval = phase_deg;
                                    }
                                    else
                                    {
                                        if (valindex == -1 && nval == 3 && timeCurve.component == "Mag")
                                        {
                                            tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2] +
                                                val1[0] * val1[0] + val1[1] * val1[1] + val1[2] * val1[2]);
                                        }
                                        else if (valindex == -1 && nval == 3 && timeCurve.component == "Mag cos(wt+phase)")
                                        {
                                            tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
                                        }
                                        else
                                        {
                                            tempval = val[valindex];
                                        }
                                    }
                                    //存储
                                    QVector<double> tempvec;
                                    if (yvalues.contains(cname))
                                        tempvec = yvalues.value(cname);
                                    tempvec.append(tempval);
                                    yvalues.insert(cname, tempvec);
                                }
                            }
                        }
                    }

                    for (int id : ids)
                    {
                        QVector<QString> vec = idCurNames.value(id);
                        for (auto it : vec)
                        {
                            cnames.push_back(it);
                        }
                    }
                }
            }
        }
    }
    void mIMxdbFile1::readIDVarCurve(const QString& pname, const QString& var, QList<CurveKey> idCurves, const QVector<int>& ids, const set<int>& frameIds, QStringList& cnames, QHash<QString, QHash<int, double>>& yvalues, QString caseName)
    {
        auto outputDatabase = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
        if (outputDatabase == nullptr)
            return;

        if (ids.isEmpty())
            return;

        QHash<int, QVector<QString>> idCurNames{};

        QHash<int, QPointer<mPostModelData>> modelDatas = outputDatabase->getAllFrameData();
        const int nframe = modelDatas.size();
        for (int i = 0; i < nframe; ++i)
        {
            PostFrameInfo finfo = modelDatas.value(i + 1)->_frameInfo;
            if (finfo._frameCase.compare(caseName) == 0)
            {
                if (frameIds.find(finfo._frameNum) != frameIds.end())
                {
                    const int nvar = finfo._frameVars.size();
                    for (int ii = 0; ii < nvar; ++ii)
                    {
                        PostVariableInfo varinfo = finfo._frameVars.at(ii);
                        if (varinfo._varName == var)
                        {
                            //打开文件
                            bool isopen = openFileStatus(finfo._frameFile);
                            if (!isopen)
                                return;

                            std::vector<int> targetIds;
                            targetIds.reserve(ids.size());
                            for (int id : ids)
                                targetIds.push_back(id);

                            std::vector<MxdbReader::MxdbVariableValue> values;
                            if (!readVariableValuesHelper(finfo._frameFile, varinfo, targetIds, values))
                                return;

                            //获取分量数
                            const int ncompo = varinfo._nCompo;

                            //获取当前变量中的变量个数
                            const int nresult = varinfo._varCount;
                            for (int k = 0; k < nresult; ++k)
                            {
                                //获取层数及层索引
                                const int nlayer = varinfo._varLayer.at(k);
                                //计算分量值个数及所需值的索引
                                int nval;
                                if (nlayer == 0)
                                {
                                    if (ncompo == 0)
                                        nval = 1;
                                    else
                                        nval = ncompo;
                                }
                                else
                                {
                                    if (ncompo == 0)
                                        nval = nlayer;
                                    else
                                        nval = ncompo * nlayer;
                                }

                                // 计算当前 shape 在 value 数组中的偏移
                                int offset = 0;
                                for (int kk = 0; kk < k; ++kk)
                                {
                                    const int prevLayer = varinfo._varLayer.at(kk);
                                    const int prevNval = (prevLayer == 0) ? (ncompo == 0 ? 1 : ncompo) : (ncompo == 0 ? prevLayer : ncompo * prevLayer);
                                    offset += prevNval;
                                }

                                //读取变量值
                                for (const auto& v : values)
                                {
                                    const int id = v.entityId;
                                    if (ids.contains(id))
                                    {
                                        if (offset + nval > static_cast<int>(v.realValues.size()))
                                            continue;

                                        const double* val = v.realValues.data() + offset;
                                        const double* val1 = v.imagValues.data() + offset;

                                        for (CurveKey idCurve : idCurves)
                                        {
                                            const int layerindex = varinfo._varLayerNames1.at(k).indexOf(idCurve.layer);

                                            //曲线名称
                                            QString cname = QString("ID-%1_%2-f%3").arg(var).arg(idCurve.toString()).arg(finfo._frameNum);
                                            QVector<QString> vec = idCurNames.value(id);
                                            if (!vec.contains(cname))
                                            {
                                                vec.push_back(cname);
                                                idCurNames.insert(id, vec);
                                            }

                                            //获取当前分量的索引
                                            const int compoi = varinfo._compoNames.indexOf(idCurve.component);
                                            //计算分量的索引
                                            int valindex;
                                            if (nlayer == 0)
                                            {
                                                if (ncompo == 0)
                                                    valindex = 0;
                                                else
                                                    valindex = compoi;
                                            }
                                            else
                                            {
                                                if (ncompo == 0)
                                                    valindex = layerindex + compoi;
                                                else
                                                    valindex = ncompo * layerindex + compoi;
                                            }
                                            //取值
                                            double tempval;
                                            if (idCurve.cplx == QString("实部"))
                                                tempval = val[valindex];
                                            else if (idCurve.cplx == QString("虚部"))
                                                tempval = val1[valindex];
                                            else if (idCurve.cplx == QString("幅值"))
                                            {
                                                double v0 = val[valindex];
                                                double v1 = val1[valindex];
                                                tempval = sqrt(v0 * v0 + v1 * v1);
                                            }
                                            else if (idCurve.cplx == QString("相位"))
                                            {
                                                double v0 = val[valindex];
                                                double v1 = val1[valindex];
                                                tempval = atan(v1 / v0) * 180 / 3.1415926 + 180;
                                            }
                                            else
                                            {
                                                if (valindex == -1 && nval == 3 && idCurve.component == "Mag")
                                                {
                                                    tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2] +
                                                        val1[0] * val1[0] + val1[1] * val1[1] + val1[2] * val1[2]);
                                                }
                                                else if (valindex == -1 && nval == 3 && idCurve.component == "Mag cos(wt+phase)")
                                                {
                                                    tempval = sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
                                                }
                                                else
                                                {
                                                    tempval = val[valindex];
                                                }
                                            }
                                            //存储

                                            QHash<int, double> tempvec;
                                            if (yvalues.contains(cname))
                                                tempvec = yvalues.value(cname);
                                            tempvec.insert(id, tempval);
                                            yvalues.insert(cname, tempvec);

                                            if (idCurve.component.isEmpty())
                                            {
                                                //曲线名称
                                                QString cname = QString("ID-%1_%2-f%3").arg(id).arg(var).arg(idCurve.toString()).arg(finfo._frameNum);
                                                QVector<QString> vec = idCurNames.value(id);
                                                if (!vec.contains(cname))
                                                {
                                                    vec.push_back(cname);
                                                    idCurNames.insert(id, vec);
                                                }
                                                QHash<int, double> tempvec;
                                                if (yvalues.contains(cname))
                                                    tempvec = yvalues.value(cname);
                                                tempvec.insert(id, val[0]);
                                                yvalues.insert(cname, tempvec);
                                            }
                                        }
                                    }
                                }
                            }

                            for (int id : ids)
                            {
                                QVector<QString> vec = idCurNames.value(id);
                                for (auto it : vec)
                                {
                                    cnames.push_back(it);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    void mIMxdbFile1::run()
    {
        slot_importAllMxDbFile();
    }

    bool mIMxdbFile1::openFileStatus(const QString& fname)
    {
        _mutex_for_mulitfile_read.lock();

        QFile dbFile(fname);
        if (!dbFile.exists())
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("文件%1不存在！").arg(fname));
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("File %1 does not exist!").arg(fname));
            }
            _mutex_for_mulitfile_read.unlock();
            return false;
        }

        //同帧文件已缓存打开：版本随缓存 reader 取，跳过版本嗅探/产品线探测与重复 open
        if (_frameReader && _frameReaderFile == fname)
        {
            _version = _frameReader->version();
            _mutex_for_mulitfile_read.unlock();
            return true;
        }

        // 先读取文件版本号，用于选择正确的 MxdbReader
        uint32_t fileVersion = 0;
        {
            std::ifstream versionStream;
#ifdef _WIN32
            versionStream.open(fname.toStdWString(), std::ios::in | std::ios::binary);
#else
            versionStream.open(fname.toLocal8Bit().constData(), std::ios::in | std::ios::binary);
#endif
            if (versionStream) {
                int v = 0;
                versionStream.read(reinterpret_cast<char*>(&v), sizeof(int));
                fileVersion = static_cast<uint32_t>(v);
            }
        }

        // 旧版 201/202 不依赖坐标精度区分产品线，直接使用 NewSingleMxdbReader；
        // 新版 601/602 记录式格式与产品线无关，同样跳过探测（避免误导性 productLine 日志）
        if (fileVersion != 0xA1B1C201 && fileVersion != 0xA1B1C202
            && fileVersion != 0xA1B1C601 && fileVersion != 0xA1B1C602)
        {
            // 若调用者未设置产品线，先根据文件内容自动判断。
            // 显式 v05/v06 与隐式/电机使用不同坐标精度，必须选对 Reader。
            if (_productLine == 0)
            {
                // 缓存命中时直接复用探测结果，不试算也不重复输出日志
                if (!lookupProductLineCache(fname, _productLine))
                {
                    _productLine = detectProductLine(fname);
                    storeProductLineCache(fname, _productLine);
                    mGlobalSignals::getInstance()->outputMessageSig(
                        0,
                        QString("[detectProductLine] %1 productLine=%2")
                        .arg(fname)
                        .arg(_productLine));
                }
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(
                    0,
                    QString("[detectProductLine] skipped, productLine already %1: %2")
                    .arg(_productLine)
                    .arg(fname));
            }
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(
                0,
                QString("[detectProductLine] %1 legacy version 0x%2, use NewSingleMxdbReader")
                .arg(fname)
                .arg(fileVersion, 8, 16, QChar('0')));
        }

        std::ifstream fs;
        std::unique_ptr<MxdbReaderBase> reader = createMxdbReader(fs, fileVersion);
        if (!reader->open(fname.toStdWString()))
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("文件%1打开失败！").arg(fname));
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("File %1 open failed!").arg(fname));
            }
            _mutex_for_mulitfile_read.unlock();
            return false;
        }

        _version = reader->version();

        // 经典格式支持到 v06（0xA1B1C106，含部件材料/截面名与集合块），201/202 为旧版单文件格式，
        // 601/602 为新版记录式格式（v07.mxdb 规范，主链路在 mMxdbThread，此处放行防其它入口误报版本错误）
        if (_version > 0xA1B1C106 && _version != 0xA1B1C201 && _version != 0xA1B1C202
            && _version != 0xA1B1C601 && _version != 0xA1B1C602)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("文件%1版本错误！").arg(fname));
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("File %1 version error!").arg(fname));
            }
            _mutex_for_mulitfile_read.unlock();
            return false;
        }

        _mutex_for_mulitfile_read.unlock();
        return true;
    }

    bool mIMxdbFile1::readInitModelDatas(QPointer<mPostDataBase> modeldb)
    {
        std::ifstream fs;
        std::unique_ptr<MxdbReaderBase> reader = createMxdbReader(fs, _version);

        if (!reader->open(_initFileName.toStdWString()))
            return false;

        MxdbInitialFrame frame;
        try {
            frame = reader->readInitialFrame();
        } catch (const std::exception& e) {
            // 损坏文件可抛 bad_alloc 等；交由上游"初始构型读取失败"提示中止导入
            qWarning() << "mIMxdbFile1::readInitModelDatas: readInitialFrame exception:" << e.what();
            return false;
        }
        MxdbDataBuilder builder;
        return builder.buildInitialFrame(frame, modeldb);
    }


    bool mIMxdbFile1::isSameNode(int* ids, std::vector<int> poss)
    {
        if (ids == nullptr)
            return false;

        std::set<int> temp{};
        for (auto pos : poss)
        {
            temp.insert(ids[pos]);
        }

        if (temp.size() == 1)
            return true;

        return false;
    }
    void mIMxdbFile1::d3h20ToD3p15(int* ids, std::vector<int>& nodeIds)
    {
        if (ids == nullptr)
            return;

        nodeIds.resize(15);
        nodeIds[0] = ids[3];
        nodeIds[1] = ids[2];
        nodeIds[2] = ids[6];

        nodeIds[3] = ids[0];
        nodeIds[4] = ids[1];
        nodeIds[5] = ids[5];

        nodeIds[6] = ids[10];
        nodeIds[7] = ids[18];
        nodeIds[8] = ids[19];

        nodeIds[9] = ids[8];
        nodeIds[10] = ids[17];
        nodeIds[11] = ids[16];

        nodeIds[12] = ids[11];
        nodeIds[13] = ids[9];
        nodeIds[14] = ids[13];
    }
}
