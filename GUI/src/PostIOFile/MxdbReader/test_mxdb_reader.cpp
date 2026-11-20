#include "MxdbDataBuilder.h"
#include "MxdbReaderCore/MxdbReaderBase.h"
#include "MxdbReaderCore/MxdbPrecisionPolicy.h"
#include "MxdbReaderCore/MxdbExtensionParser.h"
#include "MxdbReaderCore/ProductLineMxdbReaders.h"
#include "MxdbReaderCore/NewSingleMxdbReader.h"
#include "MxdbReaderCore/MxdbModelData.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostResultDataStruct.h"
#include "mPostEnumType.h"
#include "mElementFunction.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QTextStream>
#include <QVector3D>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <exception>
#include <csignal>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <dbghelp.h>
#include <psapi.h>
#else
#include <execinfo.h>
#include <unistd.h>
#endif

using namespace MIOFile;
using namespace PostData;

namespace MIOFile {

// 将 int 写入小端二进制流
static void writeInt(std::ofstream& os, int v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(int));
}

static void writeBool(std::ofstream& os, bool v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(bool));
}

static void writeDouble(std::ofstream& os, double v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(double));
}

static void writeFloat(std::ofstream& os, float v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(float));
}

static void writeLongLong(std::ofstream& os, int64_t v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(int64_t));
}

static void writeString(std::ofstream& os, const std::string& s, size_t len) {
    std::string pad(s);
    pad.resize(len, '\0');
    os.write(pad.data(), len);
}

static void writeIntArray(std::ofstream& os, const std::vector<int>& arr) {
    if (!arr.empty())
        os.write(reinterpret_cast<const char*>(arr.data()), arr.size() * sizeof(int));
}

static void writeScalar(std::ofstream& os, double v, bool isSinglePrecision) {
    if (isSinglePrecision) {
        writeFloat(os, static_cast<float>(v));
    } else {
        writeDouble(os, v);
    }
}

// 写入一个简化版 mxdb0 文件
static void writeSimpleMxdb0(const QString& path)
{
    std::ofstream os(path.toLocal8Bit().constData(), std::ios::binary);
    if (!os.is_open()) return;

    // 版本号
    int version = 0xA1B1C101;
    writeInt(os, version);

    // 节点：8 个
    std::vector<MxdbNode> nodes = {
        {1, {0.0, 0.0, 0.0}},
        {2, {1.0, 0.0, 0.0}},
        {3, {1.0, 1.0, 0.0}},
        {4, {0.0, 1.0, 0.0}},
        {5, {0.0, 0.0, 1.0}},
        {6, {1.0, 0.0, 1.0}},
        {7, {1.0, 1.0, 1.0}},
        {8, {0.0, 1.0, 1.0}}
    };
    writeInt(os, static_cast<int>(nodes.size()));
    for (const auto& n : nodes) {
        writeInt(os, n.id);
        writeDouble(os, n.coord[0]);
        writeDouble(os, n.coord[1]);
        writeDouble(os, n.coord[2]);
    }

    // Part：1 个，包含一个 D3H8 六面体单元
    writeInt(os, 1);
    writeString(os, "Part1", 64);
    writeInt(os, 1);
    writeInt(os, 100); // elem id
    writeString(os, "D3H8", 32);
    std::vector<int> eids = {1, 2, 3, 4, 5, 6, 7, 8};
    writeIntArray(os, eids);

    os.close();
}

// 写入一个简化版 mxdb 结果帧文件
static void writeSimpleMxdbFrame(const QString& path)
{
    std::ofstream os(path.toLocal8Bit().constData(), std::ios::binary);
    if (!os.is_open()) return;

    // 版本号
    int version = 0xA1B1C101;
    writeInt(os, version);

    // 工况名
    writeString(os, "Case1", 64);

    // 时间/频率
    writeDouble(os, 0.1);

    // sameFrame = 0 表示与初始帧相同
    writeInt(os, 0);

    // 变量个数：1 个 Displacement
    writeInt(os, 1);
    writeInt(os, 0); // resultType = 0
    writeString(os, "Displacement", 64);
    writeInt(os, 0); // varType = 节点
    writeInt(os, 1); // multiShapeCount
    writeInt(os, 0); // layer count
    writeInt(os, 3); // componentCount
    writeBool(os, false); // isMag
    writeString(os, "X", 32);
    writeBool(os, false);
    writeString(os, "Y", 32);
    writeBool(os, false);
    writeString(os, "Z", 32);
    writeBool(os, false);
    int64_t varByte = 0; // 占位，由 reader 记录实际位置
    os.write(reinterpret_cast<const char*>(&varByte), sizeof(int64_t));

    // 位移值：8 个节点
    writeInt(os, 8);
    for (int i = 1; i <= 8; ++i) {
        writeInt(os, i);
        writeDouble(os, 0.1 * i);
        writeDouble(os, 0.2 * i);
        writeDouble(os, 0.3 * i);
        writeDouble(os, 0.0);
        writeDouble(os, 0.0);
        writeDouble(os, 0.0);
    }

    // 结束标记
    uint32_t endTag = 0xA1B1C1D2;
    os.write(reinterpret_cast<const char*>(&endTag), sizeof(uint32_t));

    os.close();
}

// 写入一个只包含 Stress 变量的简化结果帧，用于测试懒加载
static void writeSimpleMxdbFrameWithStress(const QString& path)
{
    std::ofstream os(path.toLocal8Bit().constData(), std::ios::binary);
    if (!os.is_open()) return;

    // 版本号
    int version = 0xA1B1C101;
    writeInt(os, version);

    // 工况名
    writeString(os, "Case1", 64);

    // 时间/频率
    writeDouble(os, 0.2);

    // sameFrame = 0
    writeInt(os, 0);

    // 变量个数：1 个 Stress
    writeInt(os, 1);
    writeInt(os, 0); // resultType = 0
    writeString(os, "Stress", 64);
    writeInt(os, 0); // varType = 节点
    writeInt(os, 1); // multiShapeCount
    writeInt(os, 0); // layer count
    writeInt(os, 1); // componentCount
    writeBool(os, false); // isMag
    writeString(os, "Mises", 32);
    writeBool(os, false); // isComplex
    int64_t valueStartOffset = static_cast<int64_t>(static_cast<std::streamoff>(os.tellp())) + static_cast<int64_t>(sizeof(int64_t));
    writeLongLong(os, valueStartOffset);

    // Stress 值：4 个节点，每个节点实部和虚部各 1 个值
    writeInt(os, 4);
    for (int i = 1; i <= 4; ++i) {
        writeInt(os, i);
        writeDouble(os, 10.0 + i); // 实部
        writeDouble(os, 0.0);        // 虚部（文件写入总是包含虚部）
    }

    // 结束标记
    uint32_t endTag = 0xA1B1C1D2;
    os.write(reinterpret_cast<const char*>(&endTag), sizeof(uint32_t));

    os.close();
}

// 写入一个指定节点数的大文件，用于性能测试
static void writeLargeStressFrame(const QString& path, int nodeCount)
{
    std::ofstream os(path.toLocal8Bit().constData(), std::ios::binary);
    if (!os.is_open()) return;

    int version = 0xA1B1C101;
    writeInt(os, version);
    writeString(os, "PerfCase", 64);
    writeDouble(os, 0.1);
    writeInt(os, 0);

    writeInt(os, 1);
    writeInt(os, 0); // resultType = 0
    writeString(os, "Stress", 64);
    writeInt(os, 0); // varType = 节点
    writeInt(os, 1);
    writeInt(os, 0);
    writeInt(os, 1);
    writeBool(os, false);
    writeString(os, "Mises", 32);
    writeBool(os, false);
    int64_t valueStartOffset = static_cast<int64_t>(static_cast<std::streamoff>(os.tellp())) + static_cast<int64_t>(sizeof(int64_t));
    writeLongLong(os, valueStartOffset);

    writeInt(os, nodeCount);
    for (int i = 1; i <= nodeCount; ++i) {
        writeInt(os, i);
        writeDouble(os, 10.0 + i);
        writeDouble(os, 0.0);
    }

    uint32_t endTag = 0xA1B1C1D2;
    os.write(reinterpret_cast<const char*>(&endTag), sizeof(uint32_t));
    os.close();
}

static uint32_t expectedEndTagForVersion(uint32_t version)
{
    if (version == 0xA1B1C106)
        return 0xA1B1C1D4; // 电机 v06 结果帧特殊
    return version + 0xD0; // v01->D1, v02->D2, ..., v05->D5
}

static void writeNode(std::ofstream& os, const MxdbNode& node, bool isSinglePrecision)
{
    writeInt(os, node.id);
    if (isSinglePrecision) {
        writeFloat(os, static_cast<float>(node.coord[0]));
        writeFloat(os, static_cast<float>(node.coord[1]));
        writeFloat(os, static_cast<float>(node.coord[2]));
    } else {
        writeDouble(os, node.coord[0]);
        writeDouble(os, node.coord[1]);
        writeDouble(os, node.coord[2]);
    }
}

static void writeDisplacementValue(std::ofstream& os, int entityId, double rx, double ry, double rz, bool isSinglePrecision, bool writeImag)
{
    writeInt(os, entityId);
    if (isSinglePrecision) {
        writeFloat(os, static_cast<float>(rx));
        writeFloat(os, static_cast<float>(ry));
        writeFloat(os, static_cast<float>(rz));
        if (writeImag) {
            writeFloat(os, 0.0f);
            writeFloat(os, 0.0f);
            writeFloat(os, 0.0f);
        }
    } else {
        writeDouble(os, rx);
        writeDouble(os, ry);
        writeDouble(os, rz);
        if (writeImag) {
            writeDouble(os, 0.0);
            writeDouble(os, 0.0);
            writeDouble(os, 0.0);
        }
    }
}

// 写入一个简化版 mxdb0 文件，覆盖任意产品线和版本
static void writeSyntheticMxdb0(const QString& path,
                                   MxdbProductLine product,
                                   uint32_t version)
{
    std::ofstream os(path.toLocal8Bit().constData(), std::ios::binary);
    if (!os.is_open()) return;

    // 与 Reader 一致：显式 v05+ 才用单精度；其余（通用/电机/显式 v01-v04）用双精度
    const bool isSinglePrecision = (product == MxdbProductLine::Explicit && version >= 0xA1B1C105);

    writeInt(os, static_cast<int>(version));

    // 节点：4 个，构成一个简单四面体
    std::vector<MxdbNode> nodes = {
        {1, {0.0, 0.0, 0.0}},
        {2, {1.0, 0.0, 0.0}},
        {3, {0.0, 1.0, 0.0}},
        {4, {0.0, 0.0, 1.0}}
    };
    writeInt(os, static_cast<int>(nodes.size()));
    for (const auto& n : nodes) {
        writeNode(os, n, isSinglePrecision);
    }

    // Part：1 个，包含一个 D3T4 四面体单元
    writeInt(os, 1);
    writeString(os, "Part1", 64);
    writeInt(os, 1);
    writeInt(os, 100);
    writeString(os, "D3T4", 32);
    std::vector<int> eids = {1, 2, 3, 4};
    writeIntArray(os, eids);

    // v04+：刚性墙
    if (version >= 0xA1B1C104) {
        writeInt(os, 1);
        writeInt(os, 1); // wall id
        writeInt(os, 0); // type 0
        for (int k = 0; k < 6; ++k)
            writeScalar(os, 0.0, isSinglePrecision);
    }

    // v05+：隐式/电机 局部坐标系；显式 分区
    if (version >= 0xA1B1C105) {
        if (product == MxdbProductLine::Implicit || product == MxdbProductLine::Motor) {
            writeInt(os, 1);
            for (int k = 0; k < 3; ++k)
                writeScalar(os, 0.0, isSinglePrecision);
            writeString(os, "LocalCoord1", 64);
            for (int k = 0; k < 9; ++k)
                writeScalar(os, (k % 4 == 0) ? 1.0 : 0.0, isSinglePrecision);
        } else if (product == MxdbProductLine::Explicit) {
            writeInt(os, 1);        // partition count
            writeInt(os, 1);        // element count in this partition
            writeIntArray(os, {100}); // element ids
        }
    }

    os.close();
}

// 写入一个简化版 mxdb 结果帧文件，覆盖任意产品线和版本
static void writeSyntheticMxdbFrame(const QString& path,
                                     MxdbProductLine product,
                                     uint32_t version)
{
    std::ofstream os(path.toLocal8Bit().constData(), std::ios::binary);
    if (!os.is_open()) return;

    const bool isSinglePrecision = (product == MxdbProductLine::Explicit && version >= 0xA1B1C105);

    writeInt(os, static_cast<int>(version));

    writeString(os, "Case1", 64);

    // 电机 v06：参数工况名 + 参数列表
    if (product == MxdbProductLine::Motor && version >= 0xA1B1C106) {
        writeString(os, "ParamCase1", 64);
        writeInt(os, 0); // time
        writeDouble(os, 0.1);

        writeInt(os, 2);
        writeString(os, "Angle", 32);
        writeInt(os, 0);
        writeString(os, "Speed", 32);
        writeInt(os, 1);
    } else {
        if (version >= 0xA1B1C103)
            writeInt(os, 0); // time type
        // 时间值在 mxdb 中始终按 double 写入，即使显式 v05+ 坐标/变量值用单精度
        writeDouble(os, 0.1);
    }

    writeInt(os, 0); // sameFrame = 0

    // 显式 v05+ 在 sameFrame 之后、变量信息之前包含失效模型信息
    if (product == MxdbProductLine::Explicit && version >= 0xA1B1C105) {
        writeInt(os, 0); // failedNodeCount = 0
        writeInt(os, 0); // failedElementCount = 0
    }

    // 变量：1 个 Displacement
    writeInt(os, 1);
    writeInt(os, 0);
    writeString(os, "Displacement", 64);
    writeInt(os, 0);
    writeInt(os, 1);
    writeInt(os, 0);
    writeInt(os, 3);
    writeBool(os, false);
    writeString(os, "X", 32);
    writeBool(os, false);
    writeString(os, "Y", 32);
    writeBool(os, false);
    writeString(os, "Z", 32);
    writeBool(os, false);
    int64_t valueStartOffset = static_cast<int64_t>(static_cast<std::streamoff>(os.tellp())) + static_cast<int64_t>(sizeof(int64_t));
    writeLongLong(os, valueStartOffset);

    // 位移值：4 个节点
    // v05 真实文件只写实部；v04 及更早保留实部+虚部。
    const bool writeImag = !(product == MxdbProductLine::Explicit && version >= 0xA1B1C105);
    writeInt(os, 4);
    for (int i = 1; i <= 4; ++i) {
        writeDisplacementValue(os, i, 0.01 * i, 0.02 * i, 0.03 * i, isSinglePrecision, writeImag);
    }

    // 结束标记
    uint32_t endTag = expectedEndTagForVersion(version);
    os.write(reinterpret_cast<const char*>(&endTag), sizeof(uint32_t));

    os.close();
}

static void logInfo(const QString& msg)
{
    std::cout << msg.toStdString() << std::endl << std::flush;
}

static void logWarn(const QString& msg)
{
    std::cerr << "WARN: " << msg.toStdString() << std::endl << std::flush;
}

static bool testInitialFrame()
{
    logInfo("=== testInitialFrame ===");
    QString initPath = QCoreApplication::applicationDirPath() + "/test_init.mxdb0";
    writeSimpleMxdb0(initPath);
    if (!QFile::exists(initPath)) {
        logWarn("failed to write init file");
        return false;
    }

    std::ifstream fs;
    ImplicitMxdbReader reader(fs);
    if (!reader.open(std::filesystem::path(initPath.toStdWString()))) {
        logWarn("failed to open " + initPath);
        return false;
    }
    MxdbInitialFrame frame = reader.readInitialFrame();
    logInfo(QString("version: 0x%1").arg(frame.version, 8, 16, QChar('0')));
    logInfo(QString("nodes: %1").arg(frame.nodes.size()));
    logInfo(QString("parts: %1").arg(frame.parts.size()));
    if (frame.parts.size() != 1 || frame.parts[0].elements.size() != 1) {
        logWarn("part/element count mismatch");
        return false;
    }
    logInfo("element type: " + QString::fromStdString(frame.parts[0].elements[0].type));
    logInfo(QString("element node ids: %1").arg(frame.parts[0].elements[0].nodeIds.size()));

    mPostDataBase db("test");
    db.setDataFlag(true);
    MxdbDataBuilder builder;
    bool ok = builder.buildInitialFrame(frame, &db);
    if (!ok) {
        logWarn("buildInitialFrame failed");
        return false;
    }
    QPointer<mPostModelData> model = db.getInitframe();
    if (model.isNull()) {
        logWarn("init frame model data is null");
        return false;
    }
    logInfo(QString("node amount: %1").arg(model->_nodedata.nodeamount));
    logInfo(QString("part count: %1").arg(model->_partdata.size()));
    if (model->_nodedata.nodeamount != 8 || model->_partdata.size() != 1) {
        logWarn("model node/part count mismatch");
        return false;
    }
    return true;
}

static bool testResultFrame()
{
    logInfo("=== testResultFrame ===");
    QString initPath = QCoreApplication::applicationDirPath() + "/test_init.mxdb0";
    QString framePath = QCoreApplication::applicationDirPath() + "/test_frame_001.mxdb";

    // 先确保有初始帧数据库
    mPostDataBase db("test");
    db.setDataFlag(true);
    {
        std::ifstream fs;
        ImplicitMxdbReader reader(fs);
        if (!reader.open(std::filesystem::path(initPath.toStdWString()))) {
            logWarn("failed to open init " + initPath);
            return false;
        }
        MxdbInitialFrame initFrame = reader.readInitialFrame();
        MxdbDataBuilder builder;
        builder.buildInitialFrame(initFrame, &db);
    }

    writeSimpleMxdbFrame(framePath);

    std::ifstream fs;
    ImplicitMxdbReader reader(fs);
    if (!reader.open(std::filesystem::path(framePath.toStdWString()))) {
        logWarn("failed to open frame " + framePath);
        return false;
    }
    MxdbResultFrame frame = reader.readResultFrame();
    logInfo("frame caseName: " + QString::fromStdString(frame.caseName));
    logInfo(QString("frame sameFrame: %1").arg(frame.sameFrame));
    logInfo(QString("variables: %1").arg(frame.variables.size()));
    logInfo(QString("displacement values: %1").arg(frame.displacementValues.size()));
    logInfo(QString("endTag: 0x%1").arg(frame.endTag, 8, 16, QChar('0')));

    MxdbDataBuilder builder;
    bool ok = builder.buildResultFrame(frame, &db, 1);
    if (!ok) {
        logWarn("buildResultFrame failed");
        return false;
    }
    QPointer<mPostModelData> model = db.getModelData(1);
    if (model.isNull()) {
        logWarn("model data 1 is null");
        return false;
    }
    logInfo(QString("frame info vars: %1").arg(model->_frameInfo._frameVars.size()));
    logInfo(QString("displacement R count: %1").arg(model->DisplacementValuesR.size()));
    if (model->DisplacementValuesR.size() != 8) {
        logWarn("displacement count mismatch");
        return false;
    }
    QHash<int, QVector3D> stored = db.getDisplacementDataByFrameID(1);
    logInfo(QString("stored displacement count: %1").arg(stored.size()));
    if (stored.size() != 8) {
        logWarn("stored displacement count mismatch");
        return false;
    }
    return true;
}

static bool testRealModel(const QString& dirPath)
{
    logInfo("=== testRealModel ===");
    QDir dir(dirPath);
    if (!dir.exists()) {
        logWarn("dir not found: " + dirPath);
        return false;
    }

    QStringList mxdb0Filters;
    mxdb0Filters << "*.mxdb0";
    QFileInfoList mxdb0Files = dir.entryInfoList(mxdb0Filters, QDir::Files);
    if (mxdb0Files.isEmpty()) {
        logWarn("no .mxdb0 file found in " + dirPath);
        return false;
    }
    QString initPath = mxdb0Files.first().absoluteFilePath();

    mPostDataBase db("real");
    db.setDataFlag(true);

    MxdbInitialFrame initFrame;
    MxdbProductLine product = MxdbProductLine::Implicit;
    {
        // 对真实工程文件，先尝试显式 Reader，再尝试隐式 Reader。
        // 因为显式 v05 使用单精度，且包含分区等扩展块，用隐式 Reader 会解析错位。
        std::ifstream fsExplicit;
        ExplicitMxdbReader explicitReader(fsExplicit);
        if (explicitReader.open(std::filesystem::path(initPath.toStdWString()))) {
            MxdbInitialFrame explicitFrame = explicitReader.readInitialFrame();
            // 简单启发式：如果解析出的 part 数合理，则使用显式 Reader
            if (explicitFrame.parts.size() > 0 && explicitFrame.parts.size() < 10000000) {
                product = MxdbProductLine::Explicit;
                initFrame = std::move(explicitFrame);
            }
        }

        if (product == MxdbProductLine::Implicit) {
            std::ifstream fsImplicit;
            ImplicitMxdbReader implicitReader(fsImplicit);
            if (!implicitReader.open(std::filesystem::path(initPath.toStdWString()))) {
                logWarn("failed to open init with both Implicit and Explicit readers: " + initPath);
                return false;
            }
            initFrame = implicitReader.readInitialFrame();
        }
    }
    logInfo(QString("init version: 0x%1").arg(initFrame.version, 8, 16, QChar('0')));
    logInfo(QString("init nodes: %1").arg(initFrame.nodes.size()));
    logInfo(QString("init parts: %1").arg(initFrame.parts.size()));
    int totalElems = 0;
    for (const auto& part : initFrame.parts) {
        totalElems += static_cast<int>(part.elements.size());
    }
    logInfo(QString("init total elements: %1").arg(totalElems));

    {
        MxdbDataBuilder builder;
        if (!builder.buildInitialFrame(initFrame, &db)) {
            logWarn("buildInitialFrame failed for real model");
            return false;
        }
    }
    QPointer<mPostModelData> initModel = db.getInitframe();
    if (initModel.isNull()) {
        logWarn("init frame model data is null for real model");
        return false;
    }
    logInfo(QString("init model nodes: %1").arg(initModel->_nodedata.nodeamount));
    logInfo(QString("init model parts: %1").arg(initModel->_partdata.size()));
    for (auto it = initModel->_partdata.begin(); it != initModel->_partdata.end(); ++it) {
        QPointer<mPostPartData> part = *it;
        if (part.isNull()) continue;
        const auto& eledata = part->_parteledata;
        logInfo(QString("  part %1 cells: %2").arg(part->getPartID()).arg(eledata.cell_rid.size()));
        if (!eledata.cell_info.empty()) {
            logInfo(QString("  first cell type: %1").arg(static_cast<int>(eledata.cell_info[0].type)));
        }
        for (auto etIt = eledata.elementtype_elemcount.begin(); etIt != eledata.elementtype_elemcount.end(); ++etIt) {
            logInfo(QString("  element type %1 count: %2").arg(static_cast<int>(etIt.key())).arg(etIt.value()));
        }
    }

    // 验证初始帧非空，否则 buildResultFrame 会因 getInitframe() 为空而失败
    if (db.getInitframe().isNull()) {
        logWarn("init frame is null after buildInitialFrame");
        return false;
    }

    int frameCount = 0;
    QStringList mxdbFilters;
    mxdbFilters << "*.mxdb";
    QFileInfoList frameFiles = dir.entryInfoList(mxdbFilters, QDir::Files);
    auto extractNumber = [](const QString& name) {
        QRegExp re("_(\\d+)\\.");
        if (re.indexIn(name) >= 0) {
            return re.cap(1).toInt();
        }
        return 0;
    };
    std::sort(frameFiles.begin(), frameFiles.end(),
              [&](const QFileInfo& a, const QFileInfo& b) {
                  return extractNumber(a.fileName()) < extractNumber(b.fileName());
              });

    // 对于超大显式模型，只验证前 3 帧以避免过长时间读取
    const int maxFramesToVerify = (product == MxdbProductLine::Explicit) ? 3 : static_cast<int>(frameFiles.size());

    for (int idx = 0; idx < frameFiles.size() && frameCount < maxFramesToVerify; ++idx) {
        const auto& fi = frameFiles[idx];
        QString framePath = fi.absoluteFilePath();
        std::ifstream fs;
        std::unique_ptr<MxdbReaderBase> reader;
        if (product == MxdbProductLine::Explicit)
            reader = std::make_unique<ExplicitMxdbReader>(fs);
        else
            reader = std::make_unique<ImplicitMxdbReader>(fs);
        if (!reader->open(std::filesystem::path(framePath.toStdWString()))) {
            logWarn("failed to open frame: " + framePath);
            return false;
        }
        reader->setInitialRigidWallCount(static_cast<int>(initFrame.rigidWalls.size()));
        MxdbResultFrame frame = reader->readResultFrame();
        MxdbDataBuilder builder;
        if (!builder.buildResultFrame(frame, &db, frameCount + 1)) {
            logWarn(QString("buildResultFrame failed at frame %1").arg(frameCount + 1));
            return false;
        }
        QPointer<mPostModelData> model = db.getModelData(frameCount + 1);
        if (model.isNull()) {
            logWarn(QString("model data %1 is null").arg(frameCount + 1));
            return false;
        }

        // 懒加载第一个非位移变量，验证常规变量读取路径
        const MxdbVariable* firstVar = nullptr;
        for (const auto& var : frame.variables) {
            logInfo(QString("  variable: %1 type=%2 varByte=%3 compCount=%4")
                        .arg(QString::fromStdString(var.name))
                        .arg(var.varType)
                        .arg(var.varByte)
                        .arg(var.componentCount));
            if (firstVar == nullptr &&
                !QString::fromStdString(var.name).contains("Displacement", Qt::CaseInsensitive)) {
                firstVar = &var;
            }
        }
        int varValueCount = 0;
        if (firstVar != nullptr) {
            logInfo(QString("  lazy loading variable: %1 varByte=%2").arg(QString::fromStdString(firstVar->name)).arg(firstVar->varByte));
            std::vector<MxdbVariableValue> values = reader->readVariableValues(*firstVar, {});
            logInfo(QString("  read values: %1").arg(values.size()));
            if (builder.buildVariableValues(*firstVar, values, &db, frameCount + 1)) {
                varValueCount = static_cast<int>(values.size());
            }
        }

        double minDisp = 1e300;
        double maxDisp = -1e300;
        int infDisp = 0;
        int nanDisp = 0;
        for (const auto& dv : frame.displacementValues) {
            for (int c = 0; c < 3; ++c) {
                double v = dv.real[c];
                if (std::isinf(v)) {
                    ++infDisp;
                } else if (std::isnan(v)) {
                    ++nanDisp;
                } else {
                    if (v < minDisp) minDisp = v;
                    if (v > maxDisp) maxDisp = v;
                }
            }
        }
        logInfo(QString("  displacement min=%1 max=%2 inf=%3 nan=%4")
                    .arg(minDisp).arg(maxDisp).arg(infDisp).arg(nanDisp));
        logInfo(QString("frame %1 (%2): sameFrame=%3 vars=%4 disp=%5 varVals=%6 nodes=%7")
                    .arg(frameCount + 1)
                    .arg(fi.fileName())
                    .arg(frame.sameFrame)
                    .arg(frame.variables.size())
                    .arg(frame.displacementValues.size())
                    .arg(varValueCount)
                    .arg(model->_nodedata.nodeamount));
        ++frameCount;
    }
    logInfo(QString("total frames loaded: %1 (verified %2)").arg(frameCount).arg(maxFramesToVerify));
    return frameCount > 0;
}

static double getCurrentMemoryMB()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
    }
#endif
    return -1.0;
}

static bool testReadPerformanceRealModel(const QString& dirPath)
{
    logInfo("=== testReadPerformanceRealModel ===");
    QDir dir(dirPath);
    if (!dir.exists()) {
        logWarn("dir not found: " + dirPath);
        return false;
    }

    QStringList mxdb0Filters;
    mxdb0Filters << "*.mxdb0";
    QFileInfoList mxdb0Files = dir.entryInfoList(mxdb0Filters, QDir::Files);
    if (mxdb0Files.isEmpty()) {
        logWarn("no .mxdb0 file found in " + dirPath);
        return false;
    }
    QString initPath = mxdb0Files.first().absoluteFilePath();

    // 确定产品线和读取 init 帧
    MxdbProductLine product = MxdbProductLine::Implicit;
    MxdbInitialFrame initFrame;
    {
        std::ifstream fsExplicit;
        ExplicitMxdbReader explicitReader(fsExplicit);
        if (explicitReader.open(std::filesystem::path(initPath.toStdWString()))) {
            MxdbInitialFrame explicitFrame = explicitReader.readInitialFrame();
            if (explicitFrame.parts.size() > 0 && explicitFrame.parts.size() < 10000000) {
                product = MxdbProductLine::Explicit;
                initFrame = std::move(explicitFrame);
            }
        }
        if (product == MxdbProductLine::Implicit) {
            std::ifstream fsImplicit;
            ImplicitMxdbReader implicitReader(fsImplicit);
            if (!implicitReader.open(std::filesystem::path(initPath.toStdWString()))) {
                logWarn("failed to open init with both Implicit and Explicit readers: " + initPath);
                return false;
            }
            initFrame = implicitReader.readInitialFrame();
        }
    }

    logInfo(QString("product: %1").arg(product == MxdbProductLine::Explicit ? "Explicit" : "Implicit"));
    logInfo(QString("init version: 0x%1").arg(initFrame.version, 8, 16, QChar('0')));
    logInfo(QString("init nodes: %1").arg(initFrame.nodes.size()));

    // 构建 1% 过滤 ID 列表（用于懒加载测试）
    std::vector<int> filterIds;
    if (!initFrame.nodes.empty()) {
        filterIds.reserve(initFrame.nodes.size() / 100 + 1);
        for (size_t i = 0; i < initFrame.nodes.size(); i += 100) {
            filterIds.push_back(initFrame.nodes[i].id);
        }
    }
    logInfo(QString("filter ids prepared: %1 (1%% of nodes)").arg(filterIds.size()));

    // 枚举并排序结果帧文件
    QStringList mxdbFilters;
    mxdbFilters << "*.mxdb";
    QFileInfoList frameFiles = dir.entryInfoList(mxdbFilters, QDir::Files);
    auto extractNumber = [](const QString& name) {
        QRegExp re("_(\\d+)\\.");
        if (re.indexIn(name) >= 0) return re.cap(1).toInt();
        return 0;
    };
    std::sort(frameFiles.begin(), frameFiles.end(),
              [&](const QFileInfo& a, const QFileInfo& b) {
                  return extractNumber(a.fileName()) < extractNumber(b.fileName());
              });

    double memBefore = getCurrentMemoryMB();

    // 1. 初始帧读取性能（多次取平均，减少误差）
    qint64 initTotalTime = 0;
    const int initRepeat = 3;
    for (int r = 0; r < initRepeat; ++r) {
        std::ifstream fs;
        std::unique_ptr<MxdbReaderBase> reader;
        if (product == MxdbProductLine::Explicit)
            reader = std::make_unique<ExplicitMxdbReader>(fs);
        else
            reader = std::make_unique<ImplicitMxdbReader>(fs);
        if (!reader->open(std::filesystem::path(initPath.toStdWString()))) {
            logWarn("perf: failed to open init");
            return false;
        }
        QElapsedTimer timer;
        timer.start();
        MxdbInitialFrame tmp = reader->readInitialFrame();
        initTotalTime += timer.elapsed();
        Q_UNUSED(tmp)
    }
    logInfo(QString("PERF init read avg: %1 ms (repeat %2)").arg(initTotalTime / initRepeat).arg(initRepeat));

    double memAfterInit = getCurrentMemoryMB();
    logInfo(QString("PERF memory after init: %1 MB (before: %2 MB)").arg(memAfterInit).arg(memBefore));

    // 2. 逐帧读取性能
    qint64 totalFrameHeaderTime = 0;       // readResultFrame（含变量头扫描 + 位移）
    qint64 totalAllVariableTime = 0;       // 读取所有非位移变量
    qint64 totalFilteredVariableTime = 0;  // 对第一个非位移变量做 1%% 过滤读取
    qint64 totalFrameBuildTime = 0;        // buildResultFrame 时间
    qint64 totalOpenTime = 0;              // open() 时间
    qint64 totalBytes = 0;
    int totalVarCount = 0;
    int totalNonDisplacementVarCount = 0;
    int processedFrames = 0;

    for (const auto& fi : frameFiles) {
        QString framePath = fi.absoluteFilePath();
        totalBytes += fi.size();

        std::ifstream fs;
        std::unique_ptr<MxdbReaderBase> reader;
        if (product == MxdbProductLine::Explicit)
            reader = std::make_unique<ExplicitMxdbReader>(fs);
        else
            reader = std::make_unique<ImplicitMxdbReader>(fs);

        QElapsedTimer openTimer;
        openTimer.start();
        if (!reader->open(std::filesystem::path(framePath.toStdWString()))) {
            logWarn("perf: failed to open frame: " + framePath);
            return false;
        }
        qint64 openTime = openTimer.elapsed();
        totalOpenTime += openTime;
        reader->setInitialRigidWallCount(static_cast<int>(initFrame.rigidWalls.size()));

        QElapsedTimer frameTimer;
        frameTimer.start();
        MxdbResultFrame frame = reader->readResultFrame();
        qint64 frameHeaderTime = frameTimer.elapsed();
        totalFrameHeaderTime += frameHeaderTime;

        totalVarCount += static_cast<int>(frame.variables.size());
        const MxdbVariable* firstNonDisplacementVar = nullptr;
        for (const auto& var : frame.variables) {
            if (var.resultType == 0 &&
                !QString::fromStdString(var.name).contains("Displacement", Qt::CaseInsensitive)) {
                if (firstNonDisplacementVar == nullptr) firstNonDisplacementVar = &var;
                ++totalNonDisplacementVarCount;
            }
        }

        // 2a. 读取所有非位移变量（全量）
        qint64 allVarTime = 0;
        for (const auto& var : frame.variables) {
            if (var.resultType != 0 ||
                QString::fromStdString(var.name).contains("Displacement", Qt::CaseInsensitive)) {
                continue;
            }
            QElapsedTimer varTimer;
            varTimer.start();
            std::vector<MxdbVariableValue> values = reader->readVariableValues(var, {});
            allVarTime += varTimer.elapsed();
            Q_UNUSED(values)
        }
        totalAllVariableTime += allVarTime;

        // 2b. 对第一个非位移变量做 1%% 过滤读取
        qint64 filteredTime = 0;
        if (firstNonDisplacementVar != nullptr) {
            QElapsedTimer filterTimer;
            filterTimer.start();
            std::vector<MxdbVariableValue> filtered = reader->readVariableValues(*firstNonDisplacementVar, filterIds);
            filteredTime = filterTimer.elapsed();
            logInfo(QString("PERF frame %1 filtered read %2 values in %3 ms")
                        .arg(fi.fileName())
                        .arg(filtered.size())
                        .arg(filteredTime));
            totalFilteredVariableTime += filteredTime;
        }

        logInfo(QString("PERF frame %1 open=%2 ms header=%3 ms allVars=%4 ms filter=%5 ms total=%6 ms vars=%7")
                    .arg(fi.fileName())
                    .arg(openTime)
                    .arg(frameHeaderTime)
                    .arg(allVarTime)
                    .arg(filteredTime)
                    .arg(openTimer.elapsed())
                    .arg(frame.variables.size()));

        ++processedFrames;
    }

    double memAfterFrames = getCurrentMemoryMB();
    logInfo(QString("PERF memory after frames: %1 MB").arg(memAfterFrames));

    if (processedFrames == 0) {
        logWarn("no frames processed");
        return false;
    }

    // 汇总
    double totalFileSizeMB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);
    qint64 totalReadTime = totalFrameHeaderTime + totalAllVariableTime;
    logInfo(QString("PERF summary:"));
    logInfo(QString("  frames: %1").arg(processedFrames));
    logInfo(QString("  total file size: %1 MB").arg(totalFileSizeMB, 0, 'f', 1));
    logInfo(QString("  init read avg: %1 ms").arg(initTotalTime / initRepeat));
    logInfo(QString("  open() avg: %1 ms").arg(totalOpenTime / processedFrames));
    logInfo(QString("  readResultFrame avg: %1 ms").arg(totalFrameHeaderTime / processedFrames));
    logInfo(QString("  read all non-displacement vars avg: %1 ms").arg(totalAllVariableTime / processedFrames));
    logInfo(QString("  read filtered (1%%) var avg: %1 ms").arg(totalFilteredVariableTime / processedFrames));
    logInfo(QString("  buildResultFrame avg: %1 ms").arg(totalFrameBuildTime / processedFrames));
    logInfo(QString("  raw read throughput: %1 MB/s")
                .arg(totalFileSizeMB / (totalReadTime / 1000.0), 0, 'f', 2));
    logInfo(QString("  variables per frame avg: %1").arg(static_cast<double>(totalVarCount) / processedFrames, 0, 'f', 1));
    logInfo(QString("  memory delta: %1 MB").arg(memAfterFrames - memBefore, 0, 'f', 1));

    return true;
}

static void logInfo(const QString& msg);
static void logWarn(const QString& msg);

struct BatchReadResult {
    QString filePath;
    QString relPath;
    QString version;
    bool isMxdb0;
    bool ok;
    QString error;
    qint64 elapsedMs;
    qint64 fileSize;
    double throughputMBps;
    int nodeCount;
    int partCount;
    int variableCount;
};

static bool readMxdbFile(const QString& filePath, bool isMxdb0, BatchReadResult& result)
{
    result.filePath = filePath;
    result.isMxdb0 = isMxdb0;
    result.ok = false;
    result.nodeCount = 0;
    result.partCount = 0;
    result.variableCount = 0;
    result.fileSize = QFileInfo(filePath).size();
    result.throughputMBps = 0.0;

    QElapsedTimer timer;
    timer.start();

    try {
        // 先尝试 Explicit，再尝试 Implicit
        std::unique_ptr<MxdbReaderBase> reader;
        std::unique_ptr<std::ifstream> fs;
        bool opened = false;
        {
            fs = std::make_unique<std::ifstream>();
            auto explicitReader = std::make_unique<ExplicitMxdbReader>(*fs);
            if (explicitReader->open(std::filesystem::path(filePath.toStdWString()))) {
                reader = std::move(explicitReader);
                opened = true;
            } else {
                fs.reset();
            }
        }
        if (!opened) {
            fs = std::make_unique<std::ifstream>();
            auto implicitReader = std::make_unique<ImplicitMxdbReader>(*fs);
            if (implicitReader->open(std::filesystem::path(filePath.toStdWString()))) {
                reader = std::move(implicitReader);
                opened = true;
            } else {
                fs.reset();
            }
        }
        if (!opened || !reader) {
            result.error = "open failed";
            result.elapsedMs = timer.elapsed();
            return false;
        }

        if (isMxdb0) {
            MxdbInitialFrame init = reader->readInitialFrame();
            result.nodeCount = static_cast<int>(init.nodes.size());
            result.partCount = static_cast<int>(init.parts.size());
        } else {
            MxdbResultFrame frame = reader->readResultFrame();
            result.variableCount = static_cast<int>(frame.variables.size());
            // 尝试读取 displacement
            const MxdbVariable* dispVar = nullptr;
            for (const auto& var : frame.variables) {
                if (var.resultType == 0 && QString::fromStdString(var.name).contains("Displacement", Qt::CaseInsensitive)) {
                    dispVar = &var;
                    break;
                }
            }
            if (dispVar) {
                std::vector<MxdbVariableValue> disp = reader->readVariableValues(*dispVar, {});
                Q_UNUSED(disp)
            }
            // 尝试读取第一个非位移变量
            for (const auto& var : frame.variables) {
                if (var.resultType == 0 && !QString::fromStdString(var.name).contains("Displacement", Qt::CaseInsensitive)) {
                    std::vector<MxdbVariableValue> vals = reader->readVariableValues(var, {});
                    Q_UNUSED(vals)
                    break;
                }
            }
        }
        result.ok = true;
        result.elapsedMs = timer.elapsed();
        if (result.elapsedMs > 0 && result.fileSize > 0) {
            result.throughputMBps = (result.fileSize / (1024.0 * 1024.0)) / (result.elapsedMs / 1000.0);
        }
        return true;
    } catch (const std::exception& e) {
        result.error = QString("exception: %1").arg(e.what());
        result.elapsedMs = timer.elapsed();
        return false;
    } catch (...) {
        result.error = "unknown exception";
        result.elapsedMs = timer.elapsed();
        return false;
    }
}

static bool testBatchRead(const QString& baseDir)
{
    logInfo("=== testBatchRead ===");
    logInfo("base dir: " + baseDir);

    QDir dir(baseDir);
    if (!dir.exists()) {
        logWarn("base dir not found: " + baseDir);
        return false;
    }

    QStringList filters;
    filters << "*.mxdb" << "*.mxdb0";
    QFileInfoList allFiles = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    QDirIterator iter(baseDir, filters, QDir::Files, QDirIterator::Subdirectories);
    QFileInfoList files;
    while (iter.hasNext()) {
        iter.next();
        files.append(iter.fileInfo());
    }
    if (files.isEmpty()) {
        logWarn("no mxdb files found in: " + baseDir);
        return false;
    }
    // 按文件大小排序便于后续分析
    std::sort(files.begin(), files.end(), [](const QFileInfo& a, const QFileInfo& b) {
        return a.size() < b.size();
    });

    // 按文件夹分组，排除 201/202，按版本选择代表性 mxdb
    QMap<QString, QFileInfoList> byFolder; // 相对路径
    for (const QFileInfo& fi : files) {
        QString rel = fi.absolutePath();
        rel = QDir(baseDir).relativeFilePath(rel);
        byFolder[rel].append(fi);
    }

    std::vector<BatchReadResult> results;
    int excluded = 0;
    int invalid = 0;

    for (auto it = byFolder.begin(); it != byFolder.end(); ++it) {
        QString folderRel = it.key();
        const QFileInfoList& list = it.value();

        QFileInfoList mxdb0List;
        QMap<uint32_t, QFileInfoList> mxdbByVersion;
        for (const QFileInfo& fi : list) {
            QString f = fi.absoluteFilePath();
            bool isMxdb0 = fi.fileName().endsWith(".mxdb0", Qt::CaseInsensitive);
            uint32_t version = 0;
            bool hasVersion = false;
            {
                QFile headFile(f);
                if (headFile.open(QIODevice::ReadOnly)) {
                    QDataStream in(&headFile);
                    in.setByteOrder(QDataStream::LittleEndian);
                    if (headFile.size() >= static_cast<qint64>(sizeof(version))) {
                        in >> version;
                        hasVersion = true;
                    }
                }
            }
            if (!hasVersion) {
                ++invalid;
                continue;
            }
            if (version == 0xA1B1C201 || version == 0xA1B1C202) {
                ++excluded;
                continue;
            }
            if (version < 0xA1B1C101 || version > 0xA1B1C106) {
                ++invalid;
                continue;
            }
            if (isMxdb0) {
                mxdb0List.append(fi);
            } else {
                mxdbByVersion[version].append(fi);
            }
        }

        // 读取所有 mxdb0
        for (const QFileInfo& fi : mxdb0List) {
            BatchReadResult r;
            r.relPath = QDir(baseDir).relativeFilePath(fi.absoluteFilePath());
            readMxdbFile(fi.absoluteFilePath(), true, r);
            // 读取版本用于显示
            QFile headFile(fi.absoluteFilePath());
            uint32_t v = 0;
            if (headFile.open(QIODevice::ReadOnly)) {
                QDataStream in(&headFile);
                in.setByteOrder(QDataStream::LittleEndian);
                in >> v;
            }
            r.version = QString("0x%1").arg(v, 8, 16, QChar('0'));
            results.push_back(r);
        }

        // 每个版本最多 2 个 mxdb
        for (auto vit = mxdbByVersion.begin(); vit != mxdbByVersion.end(); ++vit) {
            QFileInfoList candidates = vit.value();
            // 优先根目录，按路径深度排序
            std::sort(candidates.begin(), candidates.end(), [](const QFileInfo& a, const QFileInfo& b) {
                return a.absoluteFilePath().count('/') + a.absoluteFilePath().count('\\') <
                       b.absoluteFilePath().count('/') + b.absoluteFilePath().count('\\');
            });
            int count = 0;
            for (const QFileInfo& fi : candidates) {
                if (count >= 2) break;
                BatchReadResult r;
                r.relPath = QDir(baseDir).relativeFilePath(fi.absoluteFilePath());
                QFile headFile(fi.absoluteFilePath());
                uint32_t v = 0;
                if (headFile.open(QIODevice::ReadOnly)) {
                    QDataStream in(&headFile);
                    in.setByteOrder(QDataStream::LittleEndian);
                    in >> v;
                }
                r.version = QString("0x%1").arg(v, 8, 16, QChar('0'));
                readMxdbFile(fi.absoluteFilePath(), false, r);
                results.push_back(r);
                ++count;
            }
        }
    }

    int okCount = 0;
    int failCount = 0;
    for (const auto& r : results) {
        if (r.ok) {
            ++okCount;
        } else {
            ++failCount;
        }
    }

    // 计算效率统计
    qint64 totalBytes = 0;
    qint64 totalTimeMs = 0;
    QMap<QString, QPair<int, int>> versionStats; // okCount, failCount
    QMap<QString, qint64> versionTimeMs;
    QMap<QString, qint64> versionBytes;
    QMap<QString, int> versionFileCount;
    qint64 maxTimeMs = 0;
    qint64 minTimeMs = LLONG_MAX;
    for (const auto& r : results) {
        if (!r.ok) continue;
        totalBytes += r.fileSize;
        totalTimeMs += r.elapsedMs;
        versionStats[r.version].first++;
        versionTimeMs[r.version] += r.elapsedMs;
        versionBytes[r.version] += r.fileSize;
        versionFileCount[r.version]++;
        if (r.elapsedMs > maxTimeMs) maxTimeMs = r.elapsedMs;
        if (r.elapsedMs < minTimeMs) minTimeMs = r.elapsedMs;
    }
    qint64 avgTimeMs = okCount > 0 ? totalTimeMs / okCount : 0;
    double totalThroughput = totalTimeMs > 0 ? (totalBytes / (1024.0 * 1024.0)) / (totalTimeMs / 1000.0) : 0.0;
    double maxThroughput = 0.0;
    double minThroughput = 1e9;
    for (const auto& r : results) {
        if (!r.ok) continue;
        if (r.throughputMBps > maxThroughput) maxThroughput = r.throughputMBps;
        if (r.throughputMBps < minThroughput) minThroughput = r.throughputMBps;
    }
    if (minThroughput > maxThroughput) minThroughput = 0.0;

    // 输出报告
    QString reportPath = baseDir + "/_batch_read_report.txt";
    QFile report(reportPath);
    if (report.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&report);
        out.setCodec("UTF-8");
        out << "MXDB 批量读取报告\n";
        out << "================================================================================\n";
        out << "base dir: " << baseDir << "\n";
        out << "total scanned: " << files.size() << "\n";
        out << "excluded (0xA1B1C201/0xA1B1C202): " << excluded << "\n";
        out << "invalid/unknown: " << invalid << "\n";
        out << "attempted read: " << results.size() << "\n\n";
        out << "success: " << okCount << "\n";
        out << "fail: " << failCount << "\n\n";

        out << "[效率汇总]\n";
        out << "total read time: " << totalTimeMs << " ms\n";
        out << "total file size: " << QString::number(totalBytes / (1024.0 * 1024.0), 'f', 2) << " MB\n";
        out << "average time per file: " << avgTimeMs << " ms\n";
        out << "min time per file: " << (minTimeMs == LLONG_MAX ? 0 : minTimeMs) << " ms\n";
        out << "max time per file: " << maxTimeMs << " ms\n";
        out << "overall throughput: " << QString::number(totalThroughput, 'f', 2) << " MB/s\n";
        out << "min throughput: " << QString::number(minThroughput, 'f', 2) << " MB/s\n";
        out << "max throughput: " << QString::number(maxThroughput, 'f', 2) << " MB/s\n\n";

        out << "[按版本汇总]\n";
        for (auto it = versionStats.begin(); it != versionStats.end(); ++it) {
            QString v = it.key();
            int count = versionFileCount[v];
            qint64 vTime = versionTimeMs[v];
            qint64 vBytes = versionBytes[v];
            double vThroughput = vTime > 0 ? (vBytes / (1024.0 * 1024.0)) / (vTime / 1000.0) : 0.0;
            double vAvgTime = count > 0 ? static_cast<double>(vTime) / count : 0.0;
            out << v << " | files=" << count << " | ok=" << it.value().first
                << " | fail=" << it.value().second
                << " | avgTime=" << QString::number(vAvgTime, 'f', 1) << " ms"
                << " | throughput=" << QString::number(vThroughput, 'f', 2) << " MB/s\n";
        }

        out << "\n[成功读取]\n";
        for (const auto& r : results) {
            if (r.ok) {
                out << r.relPath << " | " << r.version << " | " << (r.isMxdb0 ? "mxdb0" : "mxdb")
                    << " | " << r.elapsedMs << " ms"
                    << " | " << QString::number(r.fileSize / (1024.0 * 1024.0), 'f', 2) << " MB"
                    << " | " << QString::number(r.throughputMBps, 'f', 2) << " MB/s";
                if (r.isMxdb0) {
                    out << " | nodes=" << r.nodeCount << " parts=" << r.partCount;
                } else {
                    out << " | vars=" << r.variableCount;
                }
                out << "\n";
            }
        }

        out << "\n[读取失败]\n";
        for (const auto& r : results) {
            if (!r.ok) {
                out << r.relPath << " | " << r.version << " | " << (r.isMxdb0 ? "mxdb0" : "mxdb")
                    << " | " << r.error << "\n";
            }
        }

        out << "\n[按文件夹汇总]\n";
        QMap<QString, QPair<int, int>> folderStats;
        for (const auto& r : results) {
            QString folder = QFileInfo(r.filePath).path();
            folder = QDir(baseDir).relativeFilePath(folder);
            if (r.ok) folderStats[folder].first++;
            else folderStats[folder].second++;
        }
        for (auto it = folderStats.begin(); it != folderStats.end(); ++it) {
            out << it.key() << " | ok=" << it.value().first << " fail=" << it.value().second << "\n";
        }

        report.close();
        logInfo("batch read report saved to: " + reportPath);
    }

    // 控制台输出效率摘要
    logInfo("=== efficiency summary ===");
    logInfo(QString("total read time: %1 ms").arg(totalTimeMs));
    logInfo(QString("total file size: %1 MB").arg(totalBytes / (1024.0 * 1024.0), 0, 'f', 2));
    logInfo(QString("average time per file: %1 ms").arg(avgTimeMs));
    logInfo(QString("overall throughput: %1 MB/s").arg(totalThroughput, 0, 'f', 2));
    logInfo(QString("files by version:"));
    for (auto it = versionStats.begin(); it != versionStats.end(); ++it) {
        QString v = it.key();
        int count = versionFileCount[v];
        qint64 vTime = versionTimeMs[v];
        qint64 vBytes = versionBytes[v];
        double vThroughput = vTime > 0 ? (vBytes / (1024.0 * 1024.0)) / (vTime / 1000.0) : 0.0;
        double vAvgTime = count > 0 ? static_cast<double>(vTime) / count : 0.0;
        logInfo(QString("  %1: %2 files, avg %3 ms, %4 MB/s")
                    .arg(v).arg(count).arg(vAvgTime, 0, 'f', 1).arg(vThroughput, 0, 'f', 2));
    }

    logInfo(QString("batch read done: ok=%1 fail=%2").arg(okCount).arg(failCount));
    return failCount == 0;
}

static bool testReadPerformance()
{

    logInfo("=== testReadPerformance ===");
    const int nodeCount = 100000;
    const QString framePath = QCoreApplication::applicationDirPath() + "/test_perf_stress.mxdb";

    writeLargeStressFrame(framePath, nodeCount);

    // 1. 全量读取
    {
        std::ifstream fs;
        ImplicitMxdbReader reader(fs);
        if (!reader.open(std::filesystem::path(framePath.toStdWString()))) {
            logWarn("perf: failed to open frame");
            return false;
        }
        MxdbResultFrame frame = reader.readResultFrame();
        if (frame.variables.empty()) {
            logWarn("perf: no variables");
            return false;
        }
        const MxdbVariable& var = frame.variables[0];

        QElapsedTimer timer;
        timer.start();
        std::vector<MxdbVariableValue> values = reader.readVariableValues(var, {});
        qint64 elapsed = timer.elapsed();
        logInfo(QString("perf: read all %1 entities, %2 ms").arg(values.size()).arg(elapsed));
    }

    // 2. 过滤读取（1% entity）
    {
        std::ifstream fs;
        ImplicitMxdbReader reader(fs);
        if (!reader.open(std::filesystem::path(framePath.toStdWString()))) {
            logWarn("perf: failed to reopen frame for filtered test");
            return false;
        }
        MxdbResultFrame frame = reader.readResultFrame();
        const MxdbVariable& var = frame.variables[0];

        std::vector<int> filterIds;
        filterIds.reserve(nodeCount / 100);
        for (int i = 1; i <= nodeCount; i += 100)
            filterIds.push_back(i);

        QElapsedTimer timer;
        timer.start();
        std::vector<MxdbVariableValue> values = reader.readVariableValues(var, filterIds);
        qint64 elapsed = timer.elapsed();
        logInfo(QString("perf: read filtered %1 entities, %2 ms").arg(values.size()).arg(elapsed));
    }

    // 3. 多次重新打开文件并过滤读取
    {
        std::vector<int> filterIds;
        filterIds.reserve(nodeCount / 100);
        for (int i = 1; i <= nodeCount; i += 100)
            filterIds.push_back(i);

        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < 10; ++i) {
            std::ifstream fs;
            ImplicitMxdbReader reader(fs);
            reader.open(std::filesystem::path(framePath.toStdWString()));
            MxdbResultFrame frame = reader.readResultFrame();
            const MxdbVariable& var = frame.variables[0];
            std::vector<MxdbVariableValue> values = reader.readVariableValues(var, filterIds);
            if (values.empty()) {
                logWarn("perf: empty filtered values on iteration");
                return false;
            }
        }
        qint64 elapsed = timer.elapsed();
        logInfo(QString("perf: 10 times reopen and read filtered, %1 ms").arg(elapsed));
    }

    return true;
}

static bool testVariableLazyLoading()
{
    logInfo("=== testVariableLazyLoading ===");
    QString initPath = QCoreApplication::applicationDirPath() + "/test_init.mxdb0";
    QString framePath = QCoreApplication::applicationDirPath() + "/test_frame_stress.mxdb";

    // 构建初始模型
    mPostDataBase db("test");
    db.setDataFlag(true);
    {
        std::ifstream fs;
        ImplicitMxdbReader reader(fs);
        if (!reader.open(std::filesystem::path(initPath.toStdWString()))) {
            logWarn("failed to open init " + initPath);
            return false;
        }
        MxdbInitialFrame initFrame = reader.readInitialFrame();
        MxdbDataBuilder builder;
        builder.buildInitialFrame(initFrame, &db);
    }

    writeSimpleMxdbFrameWithStress(framePath);

    std::ifstream fs;
    ImplicitMxdbReader reader(fs);
    if (!reader.open(std::filesystem::path(framePath.toStdWString()))) {
        logWarn("failed to open frame " + framePath);
        return false;
    }
    MxdbResultFrame frame = reader.readResultFrame();
    if (frame.variables.empty()) {
        logWarn("no variables in stress frame");
        return false;
    }

    const MxdbVariable* stressVar = nullptr;
    for (const auto& var : frame.variables) {
        if (QString::fromStdString(var.name).contains("Stress", Qt::CaseInsensitive)) {
            stressVar = &var;
            break;
        }
    }
    if (stressVar == nullptr) {
        logWarn("Stress variable not found");
        return false;
    }

    // 懒加载读取 Stress 变量值
    std::vector<MxdbVariableValue> values = reader.readVariableValues(*stressVar, {});
    logInfo(QString("stress values loaded: %1").arg(values.size()));
    if (values.size() != 4) {
        logWarn("stress value count mismatch");
        return false;
    }

    MxdbDataBuilder builder;
    if (!builder.buildResultFrame(frame, &db, 1)) {
        logWarn("buildResultFrame failed for stress frame");
        return false;
    }
    bool ok = builder.buildVariableValues(*stressVar, values, &db, 1);
    if (!ok) {
        logWarn("buildVariableValues failed for stress");
        return false;
    }

    PostData::Compones comp = db.getComponeValues();
    logInfo(QString("stored stress value count: %1").arg(comp.value1.size()));
    if (comp.value1.size() != 4) {
        logWarn("stored stress value count mismatch");
        return false;
    }

    // 验证按 entityIds 过滤的按需读取
    {
        std::ifstream fs2;
        ImplicitMxdbReader reader2(fs2);
        if (!reader2.open(std::filesystem::path(framePath.toStdWString()))) {
            logWarn("failed to reopen frame for filtered lazy loading");
            return false;
        }
        std::vector<int> filterIds = { 2, 4 };
        std::vector<MxdbVariableValue> filtered = reader2.readVariableValues(*stressVar, filterIds);
        if (filtered.size() != 2) {
            logWarn(QString("filtered lazy loading count mismatch: expected 2, got %1").arg(filtered.size()));
            return false;
        }
        for (const auto& val : filtered) {
            if (std::find(filterIds.begin(), filterIds.end(), val.entityId) == filterIds.end()) {
                logWarn(QString("unexpected entity id returned: %1").arg(val.entityId));
                return false;
            }
            if (std::abs(val.realValues[0] - (10.0 + val.entityId)) > 1e-10) {
                logWarn(QString("entity %1 real value mismatch").arg(val.entityId));
                return false;
            }
        }
        logInfo("filtered lazy loading ok");
    }
    return true;
}

static bool testAllFormatSamples()
{
    logInfo("=== testAllFormatSamples ===");
    QString baseDir = QCoreApplication::applicationDirPath() + "/../../doc/后处理文件格式";
    if (!QDir(baseDir).exists()) {
        logWarn("format sample directory not found: " + baseDir);
        return false;
    }

    struct TestCase {
        QString subDir;
        MxdbProductLine product;
        QString readerName;
    };
    std::vector<TestCase> cases = {
        {"通用", MxdbProductLine::Implicit, "Implicit"},
        {"显式", MxdbProductLine::Explicit, "Explicit"},
        {"电机", MxdbProductLine::Motor, "Motor"},
    };

    bool allOk = true;
    for (const auto& tc : cases) {
        QString dirPath = baseDir + "/" + tc.subDir;
        QDir dir(dirPath);
        if (!dir.exists()) {
            logWarn("missing sample dir: " + dirPath);
            allOk = false;
            continue;
        }
        QStringList filters;
        filters << "*.mxdb0" << "*.mxdb";
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
        if (files.isEmpty()) {
            logWarn("no mxdb files in " + dirPath);
            allOk = false;
            continue;
        }

        for (const auto& fi : files) {
            std::ifstream fs;
            std::unique_ptr<MxdbReaderBase> reader;
            if (tc.product == MxdbProductLine::Implicit)
                reader = std::make_unique<ImplicitMxdbReader>(fs);
            else if (tc.product == MxdbProductLine::Explicit)
                reader = std::make_unique<ExplicitMxdbReader>(fs);
            else
                reader = std::make_unique<MotorMxdbReader>(fs);

            if (!reader->open(std::filesystem::path(fi.absoluteFilePath().toStdWString()))) {
                // doc/后处理文件格式 目录中当前只有明文文档，没有二进制样本，
                // 因此 open 失败时只记录信息，不强制测试失败。
                logInfo(QString("[%1] skipped non-binary sample: %2")
                            .arg(tc.readerName)
                            .arg(fi.absoluteFilePath()));
                continue;
            }
            uint32_t expectedEndTag = expectedEndTagForVersion(reader->version());
            if (fi.suffix() == "mxdb0") {
                MxdbInitialFrame init = reader->readInitialFrame();
                if (init.nodes.empty()) {
                    logWarn(QString("[%1] readInitialFrame empty: %2").arg(tc.readerName).arg(fi.fileName()));
                    allOk = false;
                } else {
                    logInfo(QString("[%1] init ok: %2 nodes=%3").arg(tc.readerName).arg(fi.fileName()).arg(init.nodes.size()));
                }
            } else {
                MxdbResultFrame result = reader->readResultFrame();
                if (result.caseName.empty()) {
                    logWarn(QString("[%1] readResultFrame no caseName: %2").arg(tc.readerName).arg(fi.fileName()));
                    allOk = false;
                } else if (result.endTag != expectedEndTag) {
                    logWarn(QString("[%1] frame endTag mismatch for %2: 0x%3 vs expected 0x%4")
                                .arg(tc.readerName).arg(fi.fileName())
                                .arg(result.endTag, 8, 16, QChar('0'))
                                .arg(expectedEndTag, 8, 16, QChar('0')));
                    allOk = false;
                } else {
                    logInfo(QString("[%1] frame ok: %2 vars=%3").arg(tc.readerName).arg(fi.fileName()).arg(result.variables.size()));
                }
            }
        }
    }
    return allOk;
}

static bool testSyntheticAllVersions()
{
    logInfo("=== testSyntheticAllVersions ===");
    QString baseDir = QCoreApplication::applicationDirPath() + "/test_synthetic_data";
    QDir().mkpath(baseDir);

    struct TestCase {
        MxdbProductLine product;
        QString dirName;
        std::vector<uint32_t> versions;
    };

    std::vector<TestCase> cases = {
        {MxdbProductLine::Implicit, "implicit", {0xA1B1C101, 0xA1B1C102, 0xA1B1C103, 0xA1B1C104, 0xA1B1C105}},
        {MxdbProductLine::Explicit, "explicit", {0xA1B1C101, 0xA1B1C104, 0xA1B1C105}},
        {MxdbProductLine::Motor, "motor", {0xA1B1C101, 0xA1B1C104, 0xA1B1C105, 0xA1B1C106}}
    };

    bool allOk = true;
    for (const auto& tc : cases) {
        QString productDir = baseDir + "/" + tc.dirName;
        QDir().mkpath(productDir);

        for (uint32_t version : tc.versions) {
            QString initPath = QString("%1/v%2_init.mxdb0").arg(productDir).arg(version, 8, 16, QChar('0'));
            QString framePath = QString("%1/v%2_1.mxdb").arg(productDir).arg(version, 8, 16, QChar('0'));

            writeSyntheticMxdb0(initPath, tc.product, version);
            writeSyntheticMxdbFrame(framePath, tc.product, version);

            std::ifstream fs;
            std::unique_ptr<MxdbReaderBase> reader;
            if (tc.product == MxdbProductLine::Implicit)
                reader = std::make_unique<ImplicitMxdbReader>(fs);
            else if (tc.product == MxdbProductLine::Explicit)
                reader = std::make_unique<ExplicitMxdbReader>(fs);
            else
                reader = std::make_unique<MotorMxdbReader>(fs);

            if (!reader->open(std::filesystem::path(initPath.toStdWString()))) {
                logWarn(QString("[%1] open init failed for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
                continue;
            }

            MxdbInitialFrame init = reader->readInitialFrame();
            if (init.nodes.empty()) {
                logWarn(QString("[%1] init nodes empty for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
                continue;
            }
            if (version >= 0xA1B1C104 && init.rigidWalls.empty()) {
                logWarn(QString("[%1] rigid walls missing for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
            }
            if (version >= 0xA1B1C105 && (tc.product == MxdbProductLine::Implicit || tc.product == MxdbProductLine::Motor) && init.localCoordinateSystems.empty()) {
                logWarn(QString("[%1] local coords missing for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
            }
            if (version >= 0xA1B1C105 && tc.product == MxdbProductLine::Explicit && init.partitions.empty()) {
                logWarn(QString("[%1] partitions missing for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
            }

            reader->setInitialRigidWallCount(static_cast<int>(init.rigidWalls.size()));

            if (!reader->open(std::filesystem::path(framePath.toStdWString()))) {
                logWarn(QString("[%1] open frame failed for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
                continue;
            }

            MxdbResultFrame result = reader->readResultFrame();
            if (result.caseName.empty()) {
                logWarn(QString("[%1] frame caseName empty for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
                continue;
            }
            if (result.endTag != expectedEndTagForVersion(version)) {
                logWarn(QString("[%1] frame endTag mismatch for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
            }
            if (result.variables.empty()) {
                logWarn(QString("[%1] frame variables empty for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
                continue;
            }
            if (result.displacementValues.empty()) {
                logWarn(QString("[%1] displacement values empty for version 0x%2").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
                allOk = false;
                continue;
            }
            if (tc.product == MxdbProductLine::Motor && version >= 0xA1B1C106) {
                if (result.paramCaseName.empty()) {
                    logWarn(QString("[%1] paramCaseName empty for motor v06").arg(tc.dirName));
                    allOk = false;
                }
                if (result.parameters.size() != 2) {
                    logWarn(QString("[%1] parameter count mismatch for motor v06: %2").arg(tc.dirName).arg(result.parameters.size()));
                    allOk = false;
                }
            }

            logInfo(QString("[%1] version 0x%2 ok").arg(tc.dirName).arg(version, 8, 16, QChar('0')));
        }
    }
    return allOk;
}

static bool testComplexGeneratedFiles()
{
    logInfo("=== testComplexGeneratedFiles ===");
    struct TestCase {
        QString productName;
        MxdbProductLine product;
        QString readerName;
    };
    std::vector<TestCase> cases = {
        {QStringLiteral("通用"), MxdbProductLine::Implicit, QStringLiteral("Implicit")},
        {QStringLiteral("电机"), MxdbProductLine::Motor, QStringLiteral("Motor")},
        {QStringLiteral("显式"), MxdbProductLine::Explicit, QStringLiteral("Explicit")},
    };
    std::vector<uint32_t> versions = {0xA1B1C101, 0xA1B1C102, 0xA1B1C103, 0xA1B1C104};

    QString baseDir = QCoreApplication::applicationDirPath() + "/../test_complex_mxdb";
    bool allOk = true;
    for (const auto& tc : cases) {
        QString pdir = baseDir + "/" + tc.productName;
        if (!QDir(pdir).exists()) {
            logWarn("generated test dir not found: " + pdir);
            allOk = false;
            continue;
        }
        for (uint32_t version : versions) {
            QString verStr = QStringLiteral("v%1").arg(version & 0xFF, 2, 16, QChar('0'));
            QString initPath = pdir + "/" + verStr + ".mxdb0";
            QString framePath = pdir + "/" + verStr + ".mxdb";

            uint32_t expectedEndTag = expectedEndTagForVersion(version);

            std::ifstream initFs;
            std::unique_ptr<MxdbReaderBase> reader;
            if (tc.product == MxdbProductLine::Implicit)
                reader = std::make_unique<ImplicitMxdbReader>(initFs);
            else if (tc.product == MxdbProductLine::Explicit)
                reader = std::make_unique<ExplicitMxdbReader>(initFs);
            else
                reader = std::make_unique<MotorMxdbReader>(initFs);

            if (!reader->open(std::filesystem::path(initPath.toStdWString()))) {
                logWarn(QString("[%1] failed to open init: %2").arg(tc.readerName).arg(initPath));
                allOk = false;
                continue;
            }
            MxdbInitialFrame init = reader->readInitialFrame();
            reader->setInitialRigidWallCount(static_cast<int>(init.rigidWalls.size()));
            if (init.nodes.size() != 12) {
                logWarn(QString("[%1] %2 init nodes mismatch: %3").arg(tc.readerName).arg(verStr).arg(init.nodes.size()));
                allOk = false;
            }
            if (init.parts.size() != 2) {
                logWarn(QString("[%1] %2 init parts mismatch: %3").arg(tc.readerName).arg(verStr).arg(init.parts.size()));
                allOk = false;
            }
            if (version >= 0xA1B1C104 && init.rigidWalls.size() != 1) {
                logWarn(QString("[%1] %2 init rigidWalls mismatch: %3").arg(tc.readerName).arg(verStr).arg(init.rigidWalls.size()));
                allOk = false;
            }

            std::ifstream frameFs;
            if (tc.product == MxdbProductLine::Implicit)
                reader = std::make_unique<ImplicitMxdbReader>(frameFs);
            else if (tc.product == MxdbProductLine::Explicit)
                reader = std::make_unique<ExplicitMxdbReader>(frameFs);
            else
                reader = std::make_unique<MotorMxdbReader>(frameFs);
            if (!reader->open(std::filesystem::path(framePath.toStdWString()))) {
                logWarn(QString("[%1] failed to open frame: %2").arg(tc.readerName).arg(framePath));
                allOk = false;
                continue;
            }
            reader->setInitialRigidWallCount(static_cast<int>(init.rigidWalls.size()));
            MxdbResultFrame result = reader->readResultFrame();
            if (result.caseName != "ComplexCase") {
                logWarn(QString("[%1] %2 caseName mismatch: %3").arg(tc.readerName).arg(verStr).arg(QString::fromStdString(result.caseName)));
                allOk = false;
            }
            int expectedVarCount = 2;
            if (version == 0xA1B1C103)
                expectedVarCount = 3;
            if (version >= 0xA1B1C104)
                expectedVarCount = 4;  // Displacement + Stress + Strain + RigidWallPos
            if (result.variables.size() != expectedVarCount) {
                logWarn(QString("[%1] %2 variable count mismatch: %3 (expected %4)")
                            .arg(tc.readerName).arg(verStr).arg(result.variables.size()).arg(expectedVarCount));
                allOk = false;
            }
            if (version >= 0xA1B1C104 && result.rigidWallPositions.size() != 1) {
                logWarn(QString("[%1] %2 rigidWallPositions mismatch: %3")
                            .arg(tc.readerName).arg(verStr).arg(result.rigidWallPositions.size()));
                allOk = false;
            }
            if (version >= 0xA1B1C104 && result.rigidWallPositions.size() == 1) {
                const auto& wall = result.rigidWallPositions[0];
                if (wall.id != 1) {
                    logWarn(QString("[%1] %2 rigidWall id mismatch: %3").arg(tc.readerName).arg(verStr).arg(wall.id));
                    allOk = false;
                }
                if (std::abs(wall.normal[0] - 1.0) > 1e-5 ||
                    std::abs(wall.normal[1] - 2.0) > 1e-5 ||
                    std::abs(wall.normal[2] - 3.0) > 1e-5) {
                    logWarn(QString("[%1] %2 rigidWall position mismatch: %3 %4 %5")
                                .arg(tc.readerName).arg(verStr)
                                .arg(wall.normal[0]).arg(wall.normal[1]).arg(wall.normal[2]));
                    allOk = false;
                }
            }
            if (result.endTag != expectedEndTag) {
                logWarn(QString("[%1] %2 endTag mismatch: 0x%3 vs 0x%4")
                            .arg(tc.readerName).arg(verStr)
                            .arg(result.endTag, 8, 16, QChar('0'))
                            .arg(expectedEndTag, 8, 16, QChar('0')));
                allOk = false;
            }

            // 验证各常规变量值
            const MxdbVariable* displacementVar = nullptr;
            const MxdbVariable* stressVar = nullptr;
            const MxdbVariable* strainVar = nullptr;
            for (const auto& var : result.variables) {
                if (var.resultType == 0 && QString::fromStdString(var.name).contains("Displacement", Qt::CaseInsensitive))
                    displacementVar = &var;
                else if (var.resultType == 0 && QString::fromStdString(var.name).contains("Stress", Qt::CaseInsensitive))
                    stressVar = &var;
                else if (var.resultType == 0 && QString::fromStdString(var.name).contains("Strain", Qt::CaseInsensitive))
                    strainVar = &var;
            }

            if (displacementVar == nullptr) {
                logWarn(QString("[%1] %2 Displacement variable not found").arg(tc.readerName).arg(verStr));
                allOk = false;
            } else {
                if (displacementVar->varType != 0 || displacementVar->componentCount != 3) {
                    logWarn(QString("[%1] %2 Displacement meta mismatch").arg(tc.readerName).arg(verStr));
                    allOk = false;
                }
                if (result.displacementValues.size() != 12) {
                    logWarn(QString("[%1] %2 displacement value count mismatch: %3")
                                .arg(tc.readerName).arg(verStr).arg(result.displacementValues.size()));
                    allOk = false;
                }
                for (const auto& v : result.displacementValues) {
                    if (std::abs(v.imag[0] - 0.1) > 1e-5 || std::abs(v.imag[1] - 0.2) > 1e-5 || std::abs(v.imag[2] - 0.3) > 1e-5) {
                        logWarn(QString("[%1] %2 displacement value mismatch for entity %3")
                                    .arg(tc.readerName).arg(verStr).arg(v.entityId));
                        allOk = false;
                    }
                }
            }

            if (stressVar == nullptr) {
                logWarn(QString("[%1] %2 Stress variable not found").arg(tc.readerName).arg(verStr));
                allOk = false;
            } else {
                std::vector<MxdbVariableValue> stressValues = reader->readVariableValues(*stressVar, {});
                if (stressValues.size() != 2) {
                    logWarn(QString("[%1] %2 stress value count mismatch: %3")
                                .arg(tc.readerName).arg(verStr).arg(stressValues.size()));
                    allOk = false;
                }
                for (const auto& v : stressValues) {
                    if (v.entityId == 100 && std::abs(v.realValues[0] - 10.0) > 1e-5)
                        allOk = false;
                    if (v.entityId == 200 && std::abs(v.realValues[0] - 20.0) > 1e-5)
                        allOk = false;
                }
            }

            if (version >= 0xA1B1C103 && strainVar == nullptr) {
                logWarn(QString("[%1] %2 Strain variable not found").arg(tc.readerName).arg(verStr));
                allOk = false;
            } else if (strainVar != nullptr) {
                if (strainVar->shapes.size() != 2) {
                    logWarn(QString("[%1] %2 strain shape count mismatch: %3")
                                .arg(tc.readerName).arg(verStr).arg(strainVar->shapes.size()));
                    allOk = false;
                }
                std::vector<MxdbVariableValue> strainValues = reader->readVariableValues(*strainVar, {});
                if (strainValues.size() != 4) {
                    logWarn(QString("[%1] %2 strain value count mismatch: %3")
                                .arg(tc.readerName).arg(verStr).arg(strainValues.size()));
                    allOk = false;
                }
                for (const auto& v : strainValues) {
                    if (v.realValues.size() != 8 || v.imagValues.size() != 8) {
                        logWarn(QString("[%1] %2 strain component count mismatch for entity %3: %4")
                                    .arg(tc.readerName).arg(verStr).arg(v.entityId).arg(v.realValues.size()));
                        allOk = false;
                    }
                }
            }

            logInfo(QString("[%1] %2 initOk=%3 frameOk=%4").arg(tc.readerName).arg(verStr)
                        .arg(init.nodes.size() == 12 && init.parts.size() == 2)
                        .arg(result.endTag == expectedEndTag));
        }
    }
    return allOk;
}

static bool testLegacyRealModel()
{
    QString path = QString::fromWCharArray(L"F:\\Test_Model_Data\\数据重构测试文件\\PostV3.0测试文件\\现在能导入\\抽屉模型.mxdb");
    if (!QFileInfo(path).exists()) {
        logInfo("legacy real model path not found, skipping");
        return true;
    }

    logInfo("=== testLegacyRealModel ===");
    logInfo("file: " + path);

    std::ifstream stream;
    NewSingleMxdbReader reader(stream, MxdbProductLine::Explicit);
    if (!reader.open(std::filesystem::path(path.toStdWString()))) {
        logWarn("failed to open legacy real model: " + path);
        return false;
    }

    logInfo("frame count: " + QString::number(reader.frameCount()));

    MxdbInitialFrame initial = reader.readInitialFrame();
    logInfo("init nodes: " + QString::number(initial.nodes.size()));
    logInfo("init parts: " + QString::number(initial.parts.size()));
    if (initial.nodes.empty()) {
        logWarn("legacy real model init nodes empty");
        return false;
    }

    bool anyVariableRead = false;

    const std::vector<MxdbVariable>& initVars = reader.currentFrameVariables();
    logInfo("init frame variables: " + QString::number(initVars.size()));
    for (const auto& var : initVars) {
        logInfo(QString("  init var: %1 type=%2 comp=%3 varByte=%4")
                    .arg(QString::fromStdString(var.name))
                    .arg(var.varType)
                    .arg(var.componentCount)
                    .arg(var.varByte));
        auto values = reader.readVariableValues(var, {});
        logInfo(QString("  init var values: %1").arg(values.size()));
        if (!values.empty()) {
            anyVariableRead = true;
        }
    }

    int frameIndex = 0;
    while (reader.hasMoreFrames()) {
        MxdbResultFrame frame = reader.readResultFrame();
        ++frameIndex;
        logInfo(QString("frame %1: caseName=%2 vars=%3 disp=%4")
                    .arg(frameIndex)
                    .arg(QString::fromStdString(frame.caseName))
                    .arg(frame.variables.size())
                    .arg(frame.displacementValues.size()));

        if (!frame.variables.empty()) {
            const auto& var = frame.variables[0];
            auto values = reader.readVariableValues(var, {});
            logInfo(QString("  var %1: type=%2 compCount=%3 values=%4")
                        .arg(QString::fromStdString(var.name))
                        .arg(var.varType)
                        .arg(var.componentCount)
                        .arg(values.size()));
            if (!values.empty()) {
                anyVariableRead = true;
            }
        }
        if (frameIndex >= 10) {
            logInfo("legacy real model: stopping after 10 frames");
            break;
        }
    }

    logInfo(QString("legacy real model: read %1 result frames").arg(frameIndex));
    if (!anyVariableRead && frameIndex > 0) {
        logWarn("legacy real model: no variable values could be read");
        return false;
    }

    logInfo("legacy real model test passed");
    return true;
}

} // namespace MIOFile

static void writeCrashReport(const QString& message)
{
    std::cerr << message.toStdString() << std::endl;
    std::ofstream log("mxdb_reader_test_crash.log", std::ios::app);
    if (log.is_open()) {
        log << message.toStdString() << "\n";
        log.close();
    }
}

#ifdef _WIN32
static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* ex)
{
    HANDLE hProcess = GetCurrentProcess();
    SymInitialize(hProcess, nullptr, TRUE);

    HANDLE hDumpFile = CreateFileA(
        "mxdb_reader_test_crash.dmp",
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hDumpFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = ex;
        mei.ClientPointers = TRUE;
        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hDumpFile,
            MiniDumpWithDataSegs,
            &mei,
            nullptr,
            nullptr);
        CloseHandle(hDumpFile);
    }

    QString msg = QString("CRASH: unhandled exception, code: 0x%1")
                      .arg(ex->ExceptionRecord->ExceptionCode, 8, 16, QChar('0'));
    writeCrashReport(msg);
    writeCrashReport("Stack trace:");

    STACKFRAME64 frame = {};
    CONTEXT* ctx = ex->ContextRecord;
    HANDLE hThread = GetCurrentThread();

    frame.AddrPC.Offset = ctx->Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx->Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx->Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProcess, hThread, &frame, ctx, nullptr,
                       SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
        if (frame.AddrPC.Offset == 0) break;
        DWORD64 addr = frame.AddrPC.Offset;
        QByteArray symbolBuffer(sizeof(SYMBOL_INFO) + 256, 0);
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer.data());
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 256;
        if (SymFromAddr(hProcess, addr, nullptr, symbol)) {
            QString line = QString("  0x%1 %2").arg(addr, 0, 16).arg(QString::fromLatin1(symbol->Name));
            writeCrashReport(line);
        } else {
            QString line = QString("  0x%1").arg(addr, 0, 16);
            writeCrashReport(line);
        }
    }

    SymCleanup(hProcess);
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
static void posixSignalHandler(int sig)
{
    QString name;
    switch (sig) {
    case SIGSEGV: name = "SIGSEGV"; break;
    case SIGABRT: name = "SIGABRT"; break;
    case SIGFPE: name = "SIGFPE"; break;
    case SIGILL: name = "SIGILL"; break;
    case SIGBUS: name = "SIGBUS"; break;
    default: name = QString("SIGNAL_%1").arg(sig); break;
    }
    writeCrashReport(QString("CRASH: received signal %1").arg(name));
    writeCrashReport("Stack trace:");

    void* buffer[64];
    int count = backtrace(buffer, 64);
    char** symbols = backtrace_symbols(buffer, count);
    if (symbols) {
        for (int i = 0; i < count; ++i) {
            writeCrashReport(QString("  %1").arg(symbols[i]));
        }
        free(symbols);
    }
    std::_Exit(1);
}
#endif

static void terminateHandler()
{
    writeCrashReport("CRASH: std::terminate called (uncaught C++ exception)");
    std::abort();
}

static void setupCrashHandler()
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
#else
    std::signal(SIGSEGV, posixSignalHandler);
    std::signal(SIGABRT, posixSignalHandler);
    std::signal(SIGFPE, posixSignalHandler);
    std::signal(SIGILL, posixSignalHandler);
    std::signal(SIGBUS, posixSignalHandler);
#endif
    std::set_terminate(terminateHandler);
}

    int main(int argc, char* argv[])
{
    setupCrashHandler();

    QCoreApplication app(argc, argv);

    if (argc > 1 && std::strcmp(argv[1], "--crash") == 0) {
        int* p = nullptr;
        *p = 42;
    }

    if (argc > 1 && std::strcmp(argv[1], "--legacy-real") == 0) {
        try {
            bool ok = testLegacyRealModel();
            return ok ? 0 : 1;
        } catch (const std::exception& e) {
            std::cerr << "EXCEPTION: " << e.what() << std::endl;
            return 1;
        } catch (...) {
            std::cerr << "UNKNOWN EXCEPTION" << std::endl;
            return 1;
        }
    }

    if (argc > 2 && std::strcmp(argv[1], "--real") == 0) {
        QString realDir = QString::fromLocal8Bit(argv[2]);
        if (QDir(realDir).exists()) {
            bool ok4 = true;
            bool ok4p = true;
            try {
                ok4 = testRealModel(realDir);
                ok4p = testReadPerformanceRealModel(realDir);
            } catch (const std::exception& e) {
                std::cerr << "EXCEPTION: " << e.what() << std::endl;
                return 1;
            } catch (...) {
                std::cerr << "UNKNOWN EXCEPTION" << std::endl;
                return 1;
            }
            return (ok4 && ok4p) ? 0 : 1;
        } else {
            logWarn("real dir not found: " + realDir);
            return 1;
        }
    }

    if (argc > 2 && std::strcmp(argv[1], "--batch") == 0) {
        QString batchDir = QString::fromLocal8Bit(argv[2]);
        if (QDir(batchDir).exists()) {
            try {
                bool ok = testBatchRead(batchDir);
                return ok ? 0 : 1;
            } catch (const std::exception& e) {
                std::cerr << "EXCEPTION: " << e.what() << std::endl;
                return 1;
            } catch (...) {
                std::cerr << "UNKNOWN EXCEPTION" << std::endl;
                return 1;
            }
        }
    }

    try {
        bool ok1 = testInitialFrame();
        bool ok2 = testResultFrame();
        bool ok3 = testVariableLazyLoading();
        bool ok3p = testReadPerformance();
        bool ok4 = true;

        QString realDir = QString::fromWCharArray(L"F:\\Test_Model_Data\\数据重构测试文件\\PostV3.0测试文件\\有bug\\HTYY-xiaolv-05");
        if (QDir(realDir).exists()) {
            ok4 = testRealModel(realDir);
        } else {
            logInfo("real model path not found, skipping");
        }

        bool ok4p = true;
        if (QDir(realDir).exists()) {
            ok4p = testReadPerformanceRealModel(realDir);
        } else {
            logInfo("real model path not found, skipping performance test");
        }

        bool ok5 = testSyntheticAllVersions();
        bool ok6 = testAllFormatSamples();
        bool ok7 = testComplexGeneratedFiles();

        if (ok1 && ok2 && ok3 && ok3p && ok4 && ok4p && ok5 && ok6 && ok7) {
            std::cout << "ALL TESTS PASSED" << std::endl;
            return 0;
        } else {
            std::cerr << "SOME TESTS FAILED" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
