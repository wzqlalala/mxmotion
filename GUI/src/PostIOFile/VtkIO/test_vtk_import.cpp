#include "mIOVtkFile.h"
#include "VtkDataBuilder.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mElementFunction.h"

#include <QCoreApplication>
#include <QDebug>
#include <QPointer>
#include <QString>
#include <QStringList>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>

namespace
{
void consoleMessageHandler(QtMsgType, const QMessageLogContext&, const QString& message)
{
    std::fprintf(stderr, "%s\n", qPrintable(message));
}

vfr::DataArray makeArray(const std::string& name, std::size_t componentCount, const std::vector<float>& values)
{
    auto result = vfr::DataArray::from_vector<float>(name, vfr::ScalarType::Float32,
                                                     componentCount, values);
    if (!result)
    {
        std::cerr << "makeArray failed for " << name << "\n";
        std::exit(2);
    }
    return std::move(result.value());
}

vfr::DataArray makeIntArray(const std::string& name, const std::vector<std::int32_t>& values)
{
    auto result = vfr::DataArray::from_vector<std::int32_t>(name, vfr::ScalarType::Int32, 1, values);
    if (!result)
    {
        std::cerr << "makeIntArray failed for " << name << "\n";
        std::exit(2);
    }
    return std::move(result.value());
}

// 单帧合成数据：8 节点，2 个四面体 + 1 个六面体（General block），
// temperature（节点标量）/Displacement（节点矢量）/Stress（单元对称张量）
vfr::DataSet makeFrame(const QString&, double temperatureBase)
{
    vfr::DataSet dataSet;
    dataSet.kind = vfr::DataSetKind::UnstructuredGrid;
    dataSet.points = makeArray("Points", 3, {
        0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f,
        0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f});

    vfr::CellBlock general;
    general.kind = vfr::CellBlockKind::General;
    general.connectivity = makeIntArray("connectivity", {0, 1, 2, 4, 2, 1, 3, 5, 4, 5, 6, 7, 0, 1, 3, 2});
    general.offsets = makeIntArray("offsets", {4, 8, 16});
    general.types = makeIntArray("types", {10, 10, 12});
    dataSet.cell_blocks = {general};

    std::vector<float> temperature;
    for (int index = 0; index < 8; ++index)
        temperature.push_back(static_cast<float>(temperatureBase + index));
    dataSet.point_data.arrays = {
        makeArray("temperature", 1, temperature),
        makeArray("Displacement", 3, {
            0.1f, 0.f, 0.f, 0.2f, 0.f, 0.f, 0.3f, 0.f, 0.f, 0.4f, 0.f, 0.f,
            0.5f, 0.f, 0.f, 0.6f, 0.f, 0.f, 0.7f, 0.f, 0.f, 0.8f, 0.f, 0.f}),
    };
    dataSet.cell_data.arrays = {
        makeArray("Stress", 6, {
            10.f, 0.f, 0.f, 0.f, 0.f, 0.f,
            20.f, 0.f, 0.f, 0.f, 0.f, 0.f,
            30.f, 0.f, 0.f, 0.f, 0.f, 0.f}),
    };
    return dataSet;
}

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition)
    {
        std::cerr << "CHECK FAILED: " << what << "\n";
        ++failures;
    }
}

// VtkDataBuilder 数据模型适配回归（自包含，无需样本文件）：多帧成组/首帧几何
// 注册、变量元信息（分量命名、Mag/vonMises 派生注册）、懒读（帧路由、节点/
// 单元键域、派生分量、Displacement 位移发布）、结构化数据集与拓扑不一致拒绝
int runSyntheticChecks()
{
    std::vector<vfr::DataSet> frames;
    QStringList fileNames;
    for (int index = 0; index < 3; ++index)
    {
        frames.push_back(makeFrame(QStringLiteral("f%1").arg(index), 10.0 * index));
        fileNames.append(QStringLiteral("frame_%1.vtk").arg(index));
    }

    QPointer<PostData::mPostDataBase> database =
        new PostData::mPostDataBase(QStringLiteral("synthetic_vtk_dataset"));
    MIOFile::VtkDataBuilder builder;
    QString buildError;
    QStringList buildNotes;
    if (!builder.buildAll(frames, fileNames, database, &buildError, &buildNotes))
    {
        std::cerr << "buildAll failed: " << qPrintable(buildError) << "\n";
        return 10;
    }

    PostData::mPostModelData* model = database->getModelData(1);
    check(model != nullptr, "frame 1 model exists");
    check(model->_nodedata.nodeamount == 8, "node amount is 8");
    int elementCount = 0;
    for (QPointer<PostData::mPostPartData> part : model->_partdata)
    {
        if (part != nullptr)
            elementCount += static_cast<int>(part->_parteledata.cell_rid.size());
    }
    check(elementCount == 3, "3 elements imported (2 tetra + 1 hexa)");
    check(database->getModelData(3) != nullptr, "frame 3 registered");

    const QList<PostData::PostVariableInfo>& variables = model->_frameInfo._frameVars;
    const PostData::PostVariableInfo* temperatureInfo = nullptr;
    const PostData::PostVariableInfo* displacementInfo = nullptr;
    const PostData::PostVariableInfo* stressInfo = nullptr;
    for (const auto& variable : variables)
    {
        if (variable._varName == QStringLiteral("temperature"))
            temperatureInfo = &variable;
        else if (variable._varName == QStringLiteral("Displacement"))
            displacementInfo = &variable;
        else if (variable._varName == QStringLiteral("Stress"))
            stressInfo = &variable;
    }
    check(temperatureInfo != nullptr && temperatureInfo->_varType == 0 &&
          temperatureInfo->_compoNames == QStringList{QStringLiteral("temperature")},
          "temperature registered as nodal with self-named component");
    check(displacementInfo != nullptr && displacementInfo->_isMag,
          "Displacement registered as magnitude variable");
    check(stressInfo != nullptr && stressInfo->_varType == 1 &&
          stressInfo->_compoNames.contains(QStringLiteral("vonMises")),
          "Stress registered as element variable with vonMises");

    // 懒读：帧路由（帧 1 与帧 3 数值不同）与节点键域（连续 id 0..7）
    check(builder.readVarDatas(frames, database, 1, QStringLiteral("temperature"),
                               QStringLiteral("temperature")), "read frame1 temperature");
    {
        const QHash<int, double>& values = database->getComponeValues().value1;
        check(values.size() == 8, "temperature has 8 nodal values");
        check(std::abs(values.value(0) - 0.0) < 1e-4 &&
              std::abs(values.value(7) - 7.0) < 1e-4, "frame1 temperature values routed");
    }
    check(builder.readVarDatas(frames, database, 3, QStringLiteral("temperature"),
                               QStringLiteral("temperature")), "read frame3 temperature");
    {
        const QHash<int, double>& values = database->getComponeValues().value1;
        check(std::abs(values.value(0) - 20.0) < 1e-4, "frame3 temperature values routed");
    }

    // 矢量模派生 + Displacement 位移哈希发布
    check(builder.readVarDatas(frames, database, 1, QStringLiteral("Displacement"),
                               QStringLiteral("Mag")), "read Displacement Mag");
    {
        const QHash<int, double>& values = database->getComponeValues().value1;
        check(std::abs(values.value(0) - 0.1) < 1e-4 &&
              std::abs(values.value(7) - 0.8) < 1e-4, "Displacement Mag values");
        check(database->getModelData(1)->DisplacementValuesR.size() == 8,
              "displacement hash published");
    }

    // 单元变量 vonMises 派生（纯 XX 张量 → vonMises=|XX|），键域为单元真实 id 1..3
    check(builder.readVarDatas(frames, database, 1, QStringLiteral("Stress"),
                               QStringLiteral("vonMises")), "read Stress vonMises");
    {
        const QHash<int, double>& values = database->getComponeValues().value1;
        check(values.size() == 3 &&
              std::abs(values.value(1) - 10.0) < 1e-3 &&
              std::abs(values.value(2) - 20.0) < 1e-3 &&
              std::abs(values.value(3) - 30.0) < 1e-3, "Stress vonMises keyed by element id");
    }

    // 结构化数据集拒绝
    {
        vfr::DataSet structured = makeFrame(QString(), 0.0);
        structured.kind = vfr::DataSetKind::ImageData;
        std::vector<vfr::DataSet> structuredFrames = {structured};
        QPointer<PostData::mPostDataBase> rejectDb =
            new PostData::mPostDataBase(QStringLiteral("reject_structured"));
        QString error;
        check(!builder.buildAll(structuredFrames, {QStringLiteral("a.vti")}, rejectDb, &error, nullptr),
              "structured dataset rejected");
        check(error.contains(QStringLiteral("结构化")), "structured rejection message");
    }

    // 拓扑不一致拒绝（帧 2 节点数不同）
    {
        vfr::DataSet mismatch = makeFrame(QString(), 0.0);
        mismatch.points = makeArray("Points", 3, {0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f});
        std::vector<vfr::DataSet> mismatchFrames = {makeFrame(QString(), 0.0), mismatch};
        QPointer<PostData::mPostDataBase> rejectDb =
            new PostData::mPostDataBase(QStringLiteral("reject_mismatch"));
        QString error;
        check(!builder.buildAll(mismatchFrames, {QStringLiteral("a.vtk"), QStringLiteral("b.vtk")},
                               rejectDb, &error, nullptr),
              "topology mismatch rejected");
        check(error.contains(QStringLiteral("拓扑不一致")), "mismatch rejection message");
    }

    // MultiBlock 多 part：节点池合并（id 连续）与单元 id 跨 part 唯一
    {
        vfr::DataSet blockA = makeFrame(QString(), 0.0);
        blockA.kind = vfr::DataSetKind::PolyData;
        vfr::DataSet blockB = makeFrame(QString(), 0.0);
        blockB.kind = vfr::DataSetKind::PolyData;
        // 裁成 4 节点小 part：只留第一个四面体
        blockB.points = makeArray("Points", 3, {0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f});
        blockB.cell_blocks.front().connectivity = makeIntArray("connectivity", {0, 1, 2, 3});
        blockB.cell_blocks.front().offsets = makeIntArray("offsets", {4});
        blockB.cell_blocks.front().types = makeIntArray("types", {10});
        vfr::DataSet multiBlock;
        multiBlock.kind = vfr::DataSetKind::MultiBlock;
        multiBlock.blocks = {blockA, blockB};
        vfr::BlockMetadata metadataA;
        metadataA.name = "Wheel";
        multiBlock.block_metadata = {metadataA, vfr::BlockMetadata{}};

        std::vector<vfr::DataSet> multiFrames = {multiBlock};
        QPointer<PostData::mPostDataBase> multiDb =
            new PostData::mPostDataBase(QStringLiteral("multiblock"));
        QString error;
        if (!builder.buildAll(multiFrames, {QStringLiteral("scene.vtm")}, multiDb, &error, nullptr))
        {
            std::cerr << "multiblock buildAll failed: " << qPrintable(error) << "\n";
            ++failures;
        }
        else
        {
            PostData::mPostModelData* multiModel = multiDb->getModelData(1);
            check(multiModel->_nodedata.nodeamount == 12, "merged node pool 8+4");
            check(multiModel->_partdata.size() == 2, "two parts from vtm blocks");
            auto partIt = multiModel->_partdata.begin();
            PostData::mPostPartData* partA = *partIt;
            PostData::mPostPartData* partB = *++partIt;
            check(partA->getPartName() == QStringLiteral("Wheel"),
                  "part name from block_metadata");
            const auto& ridA = partA->_parteledata.cell_rid;
            const auto& ridB = partB->_parteledata.cell_rid;
            check(!ridA.empty() && !ridB.empty() && ridA.back() != ridB.front(),
                  "element ids unique across parts");
        }
    }

    // 后缀识别/成组收集/自然排序
    check(MIOFile::mIOVtkFile::isVtkFile(QStringLiteral("a.VTU")), "isVtkFile case-insensitive");
    check(!MIOFile::mIOVtkFile::isVtkFile(QStringLiteral("a.pvd")), "pvd not in suffix set");
    {
        QStringList collected;
        check(MIOFile::mIOVtkFile::tryGetVtkSelection(
                  {QStringLiteral("x.vtk"), QStringLiteral("y.mxdb"), QStringLiteral("z.vtu")}, &collected),
              "tryGetVtkSelection collects vtk files");
        check(collected.size() == 2, "non-vtk files excluded");
        const QStringList sorted = MIOFile::mIOVtkFile::sortAsSeries(
            {QStringLiteral("frame_10.vtu"), QStringLiteral("frame_2.vtu"), QStringLiteral("frame_1.vtu")});
        check(sorted.first() == QStringLiteral("frame_1.vtu") && sorted.last() == QStringLiteral("frame_10.vtu"),
              "numeric sort orders frame_2 before frame_10");
    }

    if (failures != 0)
    {
        std::cerr << failures << " check(s) failed\n";
        return 11;
    }
    std::cout << "VtkImportTest: all checks passed\n";
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MxSim"));
    qInstallMessageHandler(consoleMessageHandler);

    // 提供真实 VTK 文件参数时走 vfr::read_file + builder 全链路（后缀支持面内任一）
    if (argc > 1)
    {
        QStringList files;
        for (int index = 1; index < argc; ++index)
            files.append(QString::fromLocal8Bit(argv[index]));
        std::vector<vfr::DataSet> frames;
        for (const QString& file : files)
        {
            auto result = vfr::read_file(file.toStdWString());
            if (!result)
            {
                std::cerr << "read_file failed for " << qPrintable(file) << ": "
                          << result.error().message << "\n";
                return 20;
            }
            frames.push_back(std::move(result.value()));
        }
        QPointer<PostData::mPostDataBase> database =
            new PostData::mPostDataBase(QStringLiteral("file_dataset"));
        MIOFile::VtkDataBuilder builder;
        QString error;
        if (!builder.buildAll(frames, MIOFile::mIOVtkFile::sortAsSeries(files), database, &error, nullptr))
        {
            std::cerr << "file buildAll failed: " << qPrintable(error) << "\n";
            return 21;
        }
        std::cout << "VtkImportTest: file import ok (" << frames.size() << " frame(s))\n";
    }

    return runSyntheticChecks();
}
