#include "mIONativeFile.h"
#include "NativeReadersDataBuilder.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mElementFunction.h"
#include "mGlobalSignals.h"

#include <native_readers/d3plot_writer.hpp>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>

namespace
{
void consoleMessageHandler(QtMsgType, const QMessageLogContext&, const QString& message)
{
    std::fprintf(stderr, "%s\n", qPrintable(message));
}

// MxNativeReaders 0.3.0 数据模型适配回归（自包含，无需样本文件）：合成 Dataset
// 直接驱动 builder，覆盖新固定拓扑编码 110/231/241（缺映射则整类单元被跳过）、
// d3plot 的 global/part 位场（不可注册为节点/单元场）与首显分量选择顺序
int runSyntheticDatasetChecks()
{
    native_readers::Dataset dataset;
    dataset.name = "synthetic";
    dataset.source_format = "SYNTHETIC";
    dataset.nodes = {
        {1, {0.0, 0.0, 0.0}}, {2, {1.0, 0.0, 0.0}}, {3, {0.0, 1.0, 0.0}}, {4, {1.0, 1.0, 0.0}},
        {5, {0.5, 0.0, 0.0}}, {6, {0.5, 1.0, 0.0}}, {7, {0.0, 0.5, 0.0}}, {8, {1.0, 0.5, 0.0}},
    };

    native_readers::Element point;
    point.id = 1; point.part_id = 1; point.type = "RNODE3D";
    point.mxdb_type = 110; point.nodes = {1};
    point.source_node_slots = {1, 0};

    native_readers::Element tri6;
    tri6.id = 2; tri6.part_id = 2; tri6.type = "TRIA6";
    tri6.mxdb_type = 231; tri6.nodes = {1, 2, 3, 5, 6, 7};

    native_readers::Element quad8;
    quad8.id = 3; quad8.part_id = 3; quad8.type = "QUAD8";
    quad8.mxdb_type = 241; quad8.nodes = {1, 2, 4, 3, 5, 6, 7, 8};

    dataset.elements = {point, tri6, quad8};
    dataset.source_element_ids = {{1, 42, "reference"}};
    // 0.3.3 Dataset.parts：part 1 带实名、part 2 显式空名（源标题为空）、
    // part 3 未登记，覆盖部件命名"实名优先、猜测回退"三分支
    dataset.parts = {{1, "FrameRail"}, {2, ""}};

    native_readers::Frame frame;
    frame.name = "time 0";
    frame.subcase = 1;
    frame.time = 0.0;
    frame.fields = {
        {"Global energy", "global", {"Kinetic", "Internal", "Total"}, {}, {1}, {1.5, 2.5, 4.0}},
        {"Part mass", "part", {"Mass"}, {}, {1}, {9.5}},
        {"Displacement", "nodal", {"X", "Y", "Z"}, {}, {1, 2, 3, 4, 5, 6, 7, 8},
         {0.1, 0.0, 0.0, 0.2, 0.0, 0.0, 0.3, 0.0, 0.0, 0.4, 0.0, 0.0,
          0.5, 0.0, 0.0, 0.6, 0.0, 0.0, 0.7, 0.0, 0.0, 0.8, 0.0, 0.0}},
        {"Stress", "element", {"XX", "YY", "ZZ", "XY", "YZ", "ZX"}, {}, {2, 3},
         {1.0, 0.1, 0.2, 0.0, 0.0, 0.0, 2.0, 0.4, 0.3, 0.0, 0.0, 0.0}},
        // 带层场（复合材料铺层形态）：行主序 entity → layer → component，
        // 层值刻意拉开（1/3 与 5/7）以便断言单层切片与跨层平均互不混淆
        {"Composite Stress", "element", {"XX", "YY", "ZZ", "XY", "YZ", "ZX"},
         {"Layer_1", "Layer_2"}, {2, 3},
         {1.0, 0.1, 0.2, 0.0, 0.0, 0.0, 3.0, 0.3, 0.6, 0.0, 0.0, 0.0,
          5.0, 0.5, 1.0, 0.0, 0.0, 0.0, 7.0, 0.7, 1.4, 0.0, 0.0, 0.0}},
        // 角点源场两种形态（0.3.0 实际发射格式）：OP2 Corner Samples（GRID 前导，
        // 每 (单元,角点) 显式带节点号）与 RST at corners（角点层序号=连接槽位；
        // 单元1 带 source_node_slots {1,0} 验证零槽跳过，单元3 用 nodes 前 4 槽）
        {"Solid Stress(t) / CTETRA Corner Samples (source record)", "element",
         {"GRID", "XX", "YY", "ZZ", "XY", "YZ", "ZX"},
         {"Corner_1", "Corner_2", "Corner_3", "Corner_4"}, {2},
         {1, 10, 0, 0, 0, 0, 0,
          2, 20, 0, 0, 0, 0, 0,
          3, 30, 0, 0, 0, 0, 0,
          5, 40, 0, 0, 0, 0, 0}},
        {"Stress at corners (ANSYS 185)", "element",
         {"XX", "YY", "ZZ", "XY", "YZ", "ZX"},
         {"Corner 1", "Corner 2", "Corner 3", "Corner 4"}, {1, 3},
         {100, 0, 0, 0, 0, 0, 999, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          2, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0}},
    };
    dataset.frames = {frame};

    QPointer<PostData::mPostDataBase> database = new PostData::mPostDataBase(
        QStringLiteral("synthetic_native_dataset"));
    MIOFile::NativeReadersDataBuilder builder;
    QString buildError;
    QStringList buildNotes;
    if (!builder.buildAll(dataset, database, &buildError, &buildNotes))
    {
        std::cerr << "synthetic buildAll failed: " << qPrintable(buildError) << "\n";
        return 10;
    }

    PostData::mPostModelData* model = database->getModelData(1);
    if (model == nullptr)
    {
        std::cerr << "synthetic: frame 1 model is null\n";
        return 11;
    }
    int elementCount = 0;
    for (QPointer<PostData::mPostPartData> part : model->_partdata)
    {
        if (part != nullptr)
            elementCount += static_cast<int>(part->_parteledata.cell_rid.size());
    }
    if (elementCount != 3)
    {
        std::cerr << "synthetic: expected 3 imported elements, got " << elementCount << "\n";
        return 12;
    }
    if (model->getEleTypeByEleId(1) != MxFunctions::ElementType::Mass1)
    {
        std::cerr << "synthetic: mxdb_type 110 did not map to Mass1\n";
        return 13;
    }
    if (model->getEleTypeByEleId(2) != MxFunctions::ElementType::TRIA6)
    {
        std::cerr << "synthetic: mxdb_type 231 did not map to TRIA6\n";
        return 14;
    }
    if (model->getEleTypeByEleId(3) != MxFunctions::ElementType::SSQ8)
    {
        std::cerr << "synthetic: mxdb_type 241 did not map to SSQ8\n";
        return 15;
    }

    // 部件实名（0.3.3 Dataset.parts）：parts 命中且非空 → 真名（FrameRail）；
    // 显式空名与未命中 → 回退猜测命名 Part_N（分组按 part_id 升序对应 3 个部件）
    if (model->_partdata.size() != 3)
    {
        std::cerr << "synthetic: expected 3 parts, got " << model->_partdata.size() << "\n";
        return 36;
    }
    QStringList importedPartNames;
    for (const QPointer<PostData::mPostPartData>& part : model->_partdata)
        importedPartNames.append(part->getPartName());
    if (importedPartNames != QStringList{QStringLiteral("FrameRail"),
                                         QStringLiteral("Part_2"),
                                         QStringLiteral("Part_3")})
    {
        std::cerr << "synthetic: part names wrong: "
                  << qPrintable(importedPartNames.join(QStringLiteral(", "))) << "\n";
        return 37;
    }

    const QStringList variables = database->getVariableByFrameID(1);
    if (variables.contains(QStringLiteral("Global energy")) ||
        variables.contains(QStringLiteral("Part mass")))
    {
        std::cerr << "synthetic: non-mesh field was registered as GUI variable\n";
        return 16;
    }
    if (!variables.contains(QStringLiteral("Displacement")) ||
        !variables.contains(QStringLiteral("Stress")))
    {
        std::cerr << "synthetic: mesh fields missing from variables: "
                  << qPrintable(variables.join(QStringLiteral(", "))) << "\n";
        return 17;
    }
    const bool hasNonMeshNote = std::any_of(
        buildNotes.cbegin(), buildNotes.cend(),
        [](const QString& note) { return note.contains(QStringLiteral("non-mesh")); });
    if (!hasNonMeshNote)
    {
        std::cerr << "synthetic: skipping non-mesh fields produced no note\n";
        return 18;
    }
    if (builder.readVarDatas(dataset, database, 1,
                             QStringLiteral("Global energy"), QStringLiteral("Kinetic")))
    {
        std::cerr << "synthetic: readVarDatas accepted a global-location field\n";
        return 19;
    }
    if (!builder.readVarDatas(dataset, database, 1,
                              QStringLiteral("Displacement"), QStringLiteral("Mag")))
    {
        std::cerr << "synthetic: readVarDatas rejected Displacement Mag\n";
        return 20;
    }
    if (!builder.readVarDatas(dataset, database, 1,
                              QStringLiteral("Stress"), QStringLiteral("vonMises")))
    {
        std::cerr << "synthetic: readVarDatas rejected derived vonMises\n";
        return 21;
    }
    // 首显分量必须来自首个可显示场（Displacement），而非排在字段列表最前的 global 场
    if (database->getResultDataByFrameID(1).isEmpty())
    {
        std::cerr << "synthetic: first displayable component was not published\n";
        return 22;
    }

    // 层机制回归：层名注册进数据库（UI 层下拉框数据源）、单层切片取精确值、
    // 空层跨层平均、未知层/无层场带层请求明确拒绝
    const QStringList layerNames = database->getVariableLayer1(QStringLiteral("Composite Stress"));
    if (!layerNames.contains(QStringLiteral("Layer_1")) ||
        !layerNames.contains(QStringLiteral("Layer_2")))
    {
        std::cerr << "synthetic: layered field did not register layer names: "
                  << qPrintable(layerNames.join(QStringLiteral(", "))) << "\n";
        return 23;
    }
    if (!builder.readVarDatas(dataset, database, 1, QStringLiteral("Composite Stress"),
                              QStringLiteral("XX"), QStringLiteral("Layer_1")))
    {
        std::cerr << "synthetic: readVarDatas rejected a layer selection\n";
        return 24;
    }
    {
        const QHash<int, double> slice = database->getResultDataByFrameID(1);
        if (std::abs(slice.value(2) - 1.0) > 1e-12 || std::abs(slice.value(3) - 5.0) > 1e-12)
        {
            std::cerr << "synthetic: layer slice values wrong: " << slice.value(2) << ", "
                      << slice.value(3) << "\n";
            return 25;
        }
    }
    if (!builder.readVarDatas(dataset, database, 1, QStringLiteral("Composite Stress"),
                              QStringLiteral("XX")))
    {
        std::cerr << "synthetic: readVarDatas rejected empty-layer average read\n";
        return 26;
    }
    {
        const QHash<int, double> average = database->getResultDataByFrameID(1);
        if (std::abs(average.value(2) - 2.0) > 1e-12 || std::abs(average.value(3) - 6.0) > 1e-12)
        {
            std::cerr << "synthetic: layer average values wrong: " << average.value(2) << ", "
                      << average.value(3) << "\n";
            return 27;
        }
    }
    if (builder.readVarDatas(dataset, database, 1, QStringLiteral("Composite Stress"),
                             QStringLiteral("XX"), QStringLiteral("Layer_9")))
    {
        std::cerr << "synthetic: readVarDatas accepted an unknown layer\n";
        return 28;
    }
    if (builder.readVarDatas(dataset, database, 1, QStringLiteral("Stress"),
                             QStringLiteral("XX"), QStringLiteral("Layer_1")))
    {
        std::cerr << "synthetic: readVarDatas accepted a layer request on an unlayered field\n";
        return 29;
    }

    // 角点→节点平均伪变量回归：注册为普通变量、OP2 形态（显式 GRID）与 RST
    // 形态（槽位映射 + 零槽/越层跳过）各自反演，共享节点等权平均
    if (!variables.contains(QStringLiteral("Solid Stress(t) (nodal averaged)")) ||
        !variables.contains(QStringLiteral("Stress (nodal averaged)")))
    {
        std::cerr << "synthetic: corner nodal-average pseudo variables not registered\n";
        return 30;
    }
    if (!builder.readVarDatas(dataset, database, 1,
                              QStringLiteral("Solid Stress(t) (nodal averaged)"),
                              QStringLiteral("XX")))
    {
        std::cerr << "synthetic: nodal averaged read failed (OP2 GRID shape)\n";
        return 31;
    }
    {
        // 节点值按连续索引发布（nodeidmap：节点1→0、2→1、3→2、5→4），与渲染管线一致
        const QHash<int, double> nodal = database->getResultDataByFrameID(1);
        if (nodal.size() != 4 ||
            std::abs(nodal.value(0) - 10.0) > 1e-12 || std::abs(nodal.value(1) - 20.0) > 1e-12 ||
            std::abs(nodal.value(2) - 30.0) > 1e-12 || std::abs(nodal.value(4) - 40.0) > 1e-12)
        {
            std::cerr << "synthetic: OP2-shape nodal values wrong (size=" << nodal.size()
                      << " c0=" << nodal.value(0) << " c1=" << nodal.value(1)
                      << " c2=" << nodal.value(2) << " c4=" << nodal.value(4) << ")\n";
            return 32;
        }
    }
    if (!builder.readVarDatas(dataset, database, 1,
                              QStringLiteral("Stress (nodal averaged)"),
                              QStringLiteral("XX")))
    {
        std::cerr << "synthetic: nodal averaged read failed (RST slot shape)\n";
        return 33;
    }
    {
        // 连续索引键：节点1→0、2→1、3→2、4→3；节点1 = (100+2)/2（两单元角点
        // 共享），单元1 的 Corner2（零槽）与 Corner3/4（越层）被跳过
        const QHash<int, double> nodal = database->getResultDataByFrameID(1);
        if (nodal.size() != 4 ||
            std::abs(nodal.value(0) - 51.0) > 1e-12 || std::abs(nodal.value(1) - 4.0) > 1e-12 ||
            std::abs(nodal.value(3) - 6.0) > 1e-12 || std::abs(nodal.value(2) - 8.0) > 1e-12)
        {
            std::cerr << "synthetic: RST-shape nodal values wrong (size=" << nodal.size()
                      << " c0=" << nodal.value(0) << " c1=" << nodal.value(1)
                      << " c3=" << nodal.value(3) << " c2=" << nodal.value(2) << ")\n";
            return 34;
        }
    }
    if (!builder.readVarDatas(dataset, database, 1,
                              QStringLiteral("Stress (nodal averaged)"),
                              QStringLiteral("vonMises")))
    {
        std::cerr << "synthetic: nodal averaged vonMises derivation failed\n";
        return 35;
    }

    std::cout << "synthetic 0.3.0 checks passed\n";
    return 0;
}

// MxNativeReaders 0.3.3 数据模型契约回归（自包含，无需样本文件）：Diagnostic
// 默认值与 format_diagnostic 输出、emit_diagnostic 回调分发、ReadOptions 新
// 成员可赋值、Dataset.parts 聚合初始化（尾部追加保持成员顺序）
int runContractChecks033()
{
    native_readers::Diagnostic diagnostic;
    if (diagnostic.severity != "warning" || diagnostic.frame != -1 ||
        diagnostic.byte_offset != (std::numeric_limits<std::uint64_t>::max)())
    {
        std::cerr << "0.3.3 contract: Diagnostic defaults changed\n";
        return 40;
    }
    diagnostic.code = "SKIPPED_RESULT";
    diagnostic.format = "OP2";
    diagnostic.variable = "STRESS";
    diagnostic.message = "unsupported result table skipped";
    const std::string text = native_readers::format_diagnostic(diagnostic);
    // 0.3.10 起 format_diagnostic 的标签中文化（如 [警告]/格式=），库契约要求
    // 宿主只锚定稳定部分：code 原样保留、message 原文透传，不得匹配本地化句子
    if (text.find("[SKIPPED_RESULT]") == std::string::npos ||
        text.find("unsupported result table skipped") == std::string::npos)
    {
        std::cerr << "0.3.3 contract: format_diagnostic output unexpected: " << text << "\n";
        return 41;
    }

    native_readers::ReadOptions options;
    options.strict = true;
    options.odb_first_frame = 1;
    options.odb_fields = {"U", "S"};
    options.max_frames = 2;
    int sinkCalls = 0;
    options.diagnostic_sink = [&sinkCalls](const native_readers::Diagnostic&) { ++sinkCalls; };
    native_readers::emit_diagnostic(options, diagnostic);
    if (sinkCalls != 1)
    {
        std::cerr << "0.3.3 contract: diagnostic_sink was not invoked\n";
        return 42;
    }

    native_readers::Dataset dataset;
    dataset.parts = {{1, "FrameRail"}, {2, ""}};
    if (dataset.parts.size() != 2 || dataset.parts.front().id != 1 ||
        dataset.parts.front().name != "FrameRail" || !dataset.parts.back().name.empty())
    {
        std::cerr << "0.3.3 contract: Dataset.parts aggregate init changed\n";
        return 43;
    }

    std::cout << "0.3.3 contract checks passed\n";
    return 0;
}

// tryGetNativeSelection 分发规则（自包含，无需样本文件）
int runSelectionChecks()
{
    QString nativeFile;
    QString bdfFile;
    if (!MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/model/model.op2")}, &nativeFile, &bdfFile) ||
        nativeFile != QStringLiteral("F:/model/model.op2") || !bdfFile.isEmpty())
        return 2;
    if (!MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/models/other/model.op2"),
                        QStringLiteral("F:/mesh/model.bdf")}, &nativeFile, &bdfFile) ||
        bdfFile != QStringLiteral("F:/mesh/model.bdf"))
        return 2;
    if (!MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/ansys/job.rst")}, &nativeFile, &bdfFile) ||
        nativeFile != QStringLiteral("F:/ansys/job.rst"))
        return 2;
    if (!MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/ansys/job.rth")}, &nativeFile, &bdfFile))
        return 2;
    if (!MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/abaqus/job.odb")}, &nativeFile, &bdfFile))
        return 2;
    if (!MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/altair/result.h3d")}, &nativeFile, &bdfFile))
        return 2;
    if (!MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/dyna/d3plot")}, &nativeFile, &bdfFile) ||
        nativeFile != QStringLiteral("F:/dyna/d3plot"))
        return 2;

    // 族续文件 d3plot01 不视为主文件
    if (MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/dyna/d3plot01")}, &nativeFile, &bdfFile))
        return 2;
    // 多选整族（主文件+续文件）→ 以主文件导入，续文件忽略
    if (!MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/dyna/d3plot"),
                        QStringLiteral("F:/dyna/d3plot01"),
                        QStringLiteral("F:/dyna/d3plot02")}, &nativeFile, &bdfFile) ||
        nativeFile != QStringLiteral("F:/dyna/d3plot") || !bdfFile.isEmpty())
        return 2;
    // 只选续文件（无主文件）→ 拒绝
    if (MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/dyna/d3plot01"),
                        QStringLiteral("F:/dyna/d3plot02")}, &nativeFile, &bdfFile))
        return 2;
    // 混入 mxdb 等其他后缀 → 交回 mxdb 分支
    if (MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/model/model.op2"),
                        QStringLiteral("F:/model/model.mxdb")}, &nativeFile, &bdfFile))
        return 2;
    // 两个原生结果文件 → 拒绝
    if (MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/a/model.op2"),
                        QStringLiteral("F:/b/model.rst")}, &nativeFile, &bdfFile))
        return 2;
    // 两个 BDF → 拒绝
    if (MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/a/model.op2"),
                        QStringLiteral("F:/a/model.bdf"),
                        QStringLiteral("F:/b/model.bdf")}, &nativeFile, &bdfFile))
        return 2;
    // 纯 mxdb 选择 → 不是原生链路
    if (MIOFile::mIONativeFile::tryGetNativeSelection(
            QStringList{QStringLiteral("F:/a/b.mxdb"),
                        QStringLiteral("F:/a/c.mxdb0")}, &nativeFile, &bdfFile))
        return 2;
    return 0;
}

// d3plot 流式会话路径回归（自包含，0.3.10 接入契约）：write_d3plot 合成未压缩
// d3plot（4 节点壳模型 + 2 个保存状态 + global 位场），经 D3plotReaderSession 读回，
// 验证 buildAllFromStream 帧注册/跳过逻辑、selectFirstDisplayableVariable 选择
// 口径、readVarDatasFrame 帧级发布与 visit 全帧遍历
int runD3plotStreamChecks()
{
    native_readers::Dataset dataset;
    dataset.name = "stream synthetic";
    dataset.source_format = "LS-DYNA d3plot";
    dataset.nodes = {{10, {0.0, 0.0, 0.0}}, {20, {1.0, 0.0, 0.0}},
                     {30, {1.0, 1.0, 0.0}}, {40, {0.0, 1.0, 0.0}}};
    native_readers::Element shell;
    shell.id = 71;
    shell.part_id = 99;
    shell.type = "QUAD4";
    shell.mxdb_type = 240;
    shell.nodes = {10, 20, 30, 40};
    shell.thickness = 0.25;
    dataset.elements = {shell};
    dataset.parts = {{99, "Shell part"}};

    for (int state = 0; state < 2; ++state)
    {
        // 位移取 2 的负幂次（0.25/0.5）：绝对坐标往返（坐标+位移-坐标）在
        // 浮点下可逆，写出器按此校验拒绝不可逆表示
        const double offset = 0.25 * (state + 1);
        native_readers::Frame frame;
        frame.name = "Time = " + std::to_string(0.5 * state);
        frame.subcase = 1;
        frame.time = 0.5 * state;
        frame.fields = {
            {"Global energy", "global", {"Kinetic", "Internal", "Total"}, {}, {1}, {1.0, 2.0, 3.0}},
            {"Global velocity", "global", {"X", "Y", "Z"}, {}, {1}, {4.0, 5.0, 6.0}},
            {"Part Internal Energy", "part", {"value"}, {}, {99}, {7.0}},
            {"Part Kinetic Energy", "part", {"value"}, {}, {99}, {8.0}},
            {"Part Mass", "part", {"value"}, {}, {99}, {9.0}},
            {"Part Hourglass Energy", "part", {"value"}, {}, {99}, {10.0}},
            {"Part Velocity", "part", {"X", "Y", "Z"}, {}, {99}, {11.0, 12.0, 13.0}},
            {"Displacement", "nodal", {"X", "Y", "Z"}, {}, {10, 20, 30, 40},
             {offset, 0.0, 0.0, offset, 0.0, 0.0, offset, 0.0, 0.0, offset, 0.0, 0.0}},
            {"Thickness", "element", {"value"}, {}, {71}, {0.25}},
            {"Shell dependent variables", "element", {"Var 1", "Var 2"}, {}, {71}, {14.0, 15.0}},
            {"Internal energy", "element", {"value"}, {}, {71}, {16.0}},
        };
        dataset.frames.push_back(std::move(frame));
    }

    const QTemporaryDir directory;
    const std::filesystem::path mainFile =
        std::filesystem::path(QDir(directory.path()).filePath(QStringLiteral("d3plot")).toStdWString());
    try
    {
        const native_readers::D3plotWriteReport report = native_readers::write_d3plot(dataset, mainFile);
        if (report.files.empty())
        {
            std::cerr << "stream synthetic: write_d3plot produced no files\n";
            return 50;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "stream synthetic: write_d3plot failed: " << error.what() << "\n";
        return 50;
    }

    native_readers::D3plotStreamOptions streamOptions;
    streamOptions.read.diagnostic_sink = [](const native_readers::Diagnostic&) {};
    native_readers::D3plotReaderSession session(mainFile, streamOptions);
    if (session.frames().size() != 2)
    {
        std::cerr << "stream synthetic: expected 2 saved states, got " << session.frames().size()
                  << "\n";
        return 51;
    }
    if (std::abs(session.frames()[1].time - 0.5) > 1e-12)
    {
        std::cerr << "stream synthetic: frame 2 time mismatch\n";
        return 51;
    }
    bool catalogHasDisplacement = false;
    bool catalogHasGlobal = false;
    for (const auto& info : session.fields())
    {
        if (info.name == "Displacement" && info.location == "nodal" && info.components.size() == 3)
            catalogHasDisplacement = true;
        if (info.location == "global")
            catalogHasGlobal = true;
    }
    if (!catalogHasDisplacement || !catalogHasGlobal)
    {
        std::cerr << "stream synthetic: field catalog mismatch (displacement=" << catalogHasDisplacement
                  << " global=" << catalogHasGlobal << ")\n";
        return 51;
    }

    QPointer<PostData::mPostDataBase> database =
        new PostData::mPostDataBase(QStringLiteral("stream_synthetic_dataset"));
    MIOFile::NativeReadersDataBuilder builder;
    QString buildError;
    QStringList buildNotes;
    if (!builder.buildAllFromStream(session.model(), session.frames(), session.fields(),
                                    database, &buildError, &buildNotes))
    {
        std::cerr << "stream synthetic: buildAllFromStream failed: "
                  << qPrintable(buildError) << "\n";
        return 52;
    }
    QPointer<PostData::mPostModelData> frameOne = database->getModelData(1);
    QPointer<PostData::mPostModelData> frameTwo = database->getModelData(2);
    if (frameOne == nullptr || frameTwo == nullptr)
    {
        std::cerr << "stream synthetic: frame models missing after registration\n";
        return 52;
    }
    bool registeredDisplacement = false;
    bool skippedGlobal = true;
    for (const auto& variable : frameOne->_frameInfo._frameVars)
    {
        if (variable._varName == QStringLiteral("Displacement"))
            registeredDisplacement = true;
        if (variable._varName == QStringLiteral("Global energy"))
            skippedGlobal = false;
    }
    if (!registeredDisplacement || !skippedGlobal)
    {
        std::cerr << "stream synthetic: variable registration mismatch (displacement="
                  << registeredDisplacement << " globalSkipped=" << skippedGlobal << ")\n";
        return 52;
    }

    QString firstName;
    QString firstComponent;
    if (!MIOFile::NativeReadersDataBuilder::selectFirstDisplayableVariable(
            session.fields(), &firstName, &firstComponent) ||
        firstName != QStringLiteral("Displacement") || firstComponent != QStringLiteral("Mag"))
    {
        std::cerr << "stream synthetic: first displayable variable selection mismatch ("
                  << qPrintable(firstName) << "/" << qPrintable(firstComponent) << ")\n";
        return 53;
    }

    // 首帧首分量发布 + 第二帧 X 分量读取（帧级懒读协议）
    try
    {
        native_readers::Frame decoded = session.read_frame(0);
        if (!builder.readVarDatasFrame(session.model(), decoded, database, 1, firstName,
                                       firstComponent))
        {
            std::cerr << "stream synthetic: first frame publish failed\n";
            return 54;
        }
        decoded = session.read_frame(1);
        if (!builder.readVarDatasFrame(session.model(), decoded, database, 2,
                                       QStringLiteral("Displacement"), QStringLiteral("X")))
        {
            std::cerr << "stream synthetic: second frame publish failed\n";
            return 54;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "stream synthetic: read_frame failed: " << error.what() << "\n";
        return 54;
    }
    if (frameTwo->DisplacementValuesR.size() != 4)
    {
        std::cerr << "stream synthetic: frame 2 displacement count mismatch ("
                  << frameTwo->DisplacementValuesR.size() << ")\n";
        return 54;
    }

    // visit 全帧遍历（导入期轻校验协议）
    native_readers::D3plotStreamCallbacks callbacks;
    int visits = 0;
    callbacks.on_frame = [&](std::size_t, const native_readers::Frame&) {
        ++visits;
        return true;
    };
    const auto visitResult = session.visit(callbacks);
    if (visitResult.status != native_readers::D3plotStreamStatus::complete ||
        visitResult.frames_delivered != 2 || visits != 2)
    {
        std::cerr << "stream synthetic: visit mismatch (status="
                  << static_cast<int>(visitResult.status) << " delivered="
                  << visitResult.frames_delivered << " visits=" << visits << ")\n";
        return 55;
    }

    std::cout << "d3plot stream synthetic checks passed\n";
    return 0;
}

// 来源元信息契约回归（MxNativeReaders 0.3.4+，自包含）：write_source_metadata
// 侧车原子写、拒绝覆盖已存在目标；截断标记与保留字节计数的默认值
int runSourceMetadataChecks()
{
    native_readers::Dataset dataset;
    dataset.name = "metadata synthetic";
    dataset.source_format = "SYNTHETIC";

    native_readers::SourceMetadataRecord decoded;
    decoded.format = "OP2";
    decoded.category = "material";
    decoded.name = "MAT1 1";
    decoded.id = 1;
    decoded.decoded = true;
    decoded.texts = {"STEEL", "MID-1"};
    decoded.integers = {1000000, 3};
    decoded.reals = {0.3, 7.85e-9};

    native_readers::SourceMetadataRecord structural;
    structural.format = "OP2";
    structural.category = "load";
    structural.name = "FORCE";
    structural.id = 7;
    structural.decoded = false;
    structural.integers = {101, 102};

    dataset.source_metadata = {decoded, structural};
    if (dataset.source_metadata_truncated || dataset.retained_metadata_bytes != 0)
    {
        std::cerr << "source metadata: truncation defaults changed\n";
        return 60;
    }

    const QTemporaryDir directory;
    const std::filesystem::path sidecar =
        std::filesystem::path(QDir(directory.path())
                                  .filePath(QStringLiteral("metadata.json"))
                                  .toStdWString());
    try
    {
        native_readers::write_source_metadata(dataset, sidecar);
    }
    catch (const std::exception& error)
    {
        std::cerr << "source metadata: write failed: " << error.what() << "\n";
        return 61;
    }
    if (!std::filesystem::file_size(sidecar))
    {
        std::cerr << "source metadata: sidecar is empty\n";
        return 61;
    }
    bool refusedExisting = false;
    try
    {
        native_readers::write_source_metadata(dataset, sidecar);
    }
    catch (const native_readers::WriteError&)
    {
        refusedExisting = true;
    }
    if (!refusedExisting)
    {
        std::cerr << "source metadata: existing target was not refused\n";
        return 61;
    }

    std::cout << "source metadata checks passed\n";
    return 0;
}

// MxNativeReaders 0.3.12 element_nodal 显式单元-节点映射回归（自包含）：字段
// 注册为单元变量，读取按节点等权平均到单元值（ids=单元、每单元节点数可变，
// values 行主序 entity → node → layer → component）；层切片先取层再平均节点，
// 派生 vonMises 在平均后的分量上计算；布局漂移（值行数与节点目录不符）拒绝
int runElementNodalChecks()
{
    native_readers::Dataset dataset;
    dataset.name = "synthetic_element_nodal";
    dataset.source_format = "SYNTHETIC";
    dataset.nodes = {
        {1, {0.0, 0.0, 0.0}}, {2, {1.0, 0.0, 0.0}}, {3, {0.0, 1.0, 0.0}}, {4, {1.0, 1.0, 0.0}},
        {5, {0.5, 0.0, 0.0}}, {6, {0.5, 1.0, 0.0}}, {7, {0.0, 0.5, 0.0}}, {8, {1.0, 0.5, 0.0}},
    };
    native_readers::Element tri6;
    tri6.id = 2; tri6.part_id = 1; tri6.type = "TRIA6";
    tri6.mxdb_type = 231; tri6.nodes = {1, 2, 3, 5, 6, 7};
    native_readers::Element quad8;
    quad8.id = 3; quad8.part_id = 1; quad8.type = "QUAD8";
    quad8.mxdb_type = 241; quad8.nodes = {1, 2, 4, 3, 5, 6, 7, 8};
    dataset.elements = {tri6, quad8};

    native_readers::Frame frame;
    frame.subcase = 1;
    frame.time = 0.0;
    frame.fields = {
        // 单元 2 三节点（XX = 3/6/9 → 平均 6）、单元 3 四节点（XX = 4/8/12/16
        // → 平均 10），验证变节点数行进位与节点等权平均
        {"Stress at element nodes (ANSYS 185)", "element_nodal",
         {"XX", "YY", "ZZ", "XY", "YZ", "ZX"}, {}, {2, 3},
         {3, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0,
          4, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0},
         {3, 4}, {1, 2, 3, 1, 2, 4, 3}},
        // 带层映射场：单元 2 三节点 × Z1/Z2，层切片 Z1=4、Z2=6、跨层跨节点平均 5
        {"Plate stress at element nodes", "element_nodal",
         {"XX"}, {"Z1", "Z2"}, {2},
         {2, 4, 4, 6, 6, 8},
         {3}, {1, 2, 3}},
        // 布局漂移：节点目录声明 6 行、values 只有 2 行 → 读取明确拒绝
        {"Broken element nodes", "element_nodal",
         {"XX", "YY", "ZZ", "XY", "YZ", "ZX"}, {}, {2, 3},
         {1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0},
         {2, 4}, {1, 2, 1, 2, 4, 3}},
    };
    dataset.frames = {frame};

    QPointer<PostData::mPostDataBase> database = new PostData::mPostDataBase(
        QStringLiteral("synthetic_element_nodal"));
    MIOFile::NativeReadersDataBuilder builder;
    QString buildError;
    QStringList buildNotes;
    if (!builder.buildAll(dataset, database, &buildError, &buildNotes))
    {
        std::cerr << "element_nodal buildAll failed: " << qPrintable(buildError) << "\n";
        return 70;
    }

    const QStringList variables = database->getVariableByFrameID(1);
    if (!variables.contains(QStringLiteral("Stress at element nodes (ANSYS 185)")) ||
        !variables.contains(QStringLiteral("Plate stress at element nodes")))
    {
        std::cerr << "element_nodal: fields not registered: "
                  << qPrintable(variables.join(QStringLiteral(", "))) << "\n";
        return 71;
    }
    // 首显发布：首个可显示场为六分量张量 → vonMises（节点平均后 |XX|）
    {
        const QHash<int, double> first = database->getResultDataByFrameID(1);
        if (first.size() != 2 || std::abs(first.value(2) - 6.0) > 1e-12 ||
            std::abs(first.value(3) - 10.0) > 1e-12)
        {
            std::cerr << "element_nodal: first publish wrong (size=" << first.size()
                      << " e2=" << first.value(2) << " e3=" << first.value(3) << ")\n";
            return 72;
        }
    }
    if (!builder.readVarDatas(dataset, database, 1,
                              QStringLiteral("Stress at element nodes (ANSYS 185)"),
                              QStringLiteral("XX")))
    {
        std::cerr << "element_nodal: component read rejected\n";
        return 73;
    }
    {
        const QHash<int, double> values = database->getResultDataByFrameID(1);
        if (values.size() != 2 || std::abs(values.value(2) - 6.0) > 1e-12 ||
            std::abs(values.value(3) - 10.0) > 1e-12)
        {
            std::cerr << "element_nodal: node-averaged values wrong (e2=" << values.value(2)
                      << " e3=" << values.value(3) << ")\n";
            return 74;
        }
    }
    if (!builder.readVarDatas(dataset, database, 1,
                              QStringLiteral("Stress at element nodes (ANSYS 185)"),
                              QStringLiteral("vonMises")))
    {
        std::cerr << "element_nodal: derived vonMises rejected\n";
        return 75;
    }
    {
        const QHash<int, double> mises = database->getResultDataByFrameID(1);
        if (mises.size() != 2 || std::abs(mises.value(2) - 6.0) > 1e-12 ||
            std::abs(mises.value(3) - 10.0) > 1e-12)
        {
            std::cerr << "element_nodal: vonMises values wrong (e2=" << mises.value(2)
                      << " e3=" << mises.value(3) << ")\n";
            return 76;
        }
    }

    // 层机制：Z1/Z2 注册；层切片先取层再平均节点，空层跨层跨节点平均
    const QStringList layerNames = database->getVariableLayer1(
        QStringLiteral("Plate stress at element nodes"));
    if (!layerNames.contains(QStringLiteral("Z1")) || !layerNames.contains(QStringLiteral("Z2")))
    {
        std::cerr << "element_nodal: layered field did not register layer names\n";
        return 77;
    }
    const double expectedByLayer[] = {4.0, 6.0};
    const char* const layerLabels[] = {"Z1", "Z2"};
    for (int layer = 0; layer < 2; ++layer)
    {
        if (!builder.readVarDatas(dataset, database, 1,
                                  QStringLiteral("Plate stress at element nodes"),
                                  QStringLiteral("XX"),
                                  QString::fromLatin1(layerLabels[layer])))
        {
            std::cerr << "element_nodal: layer " << layerLabels[layer] << " read rejected\n";
            return 78;
        }
        const QHash<int, double> slice = database->getResultDataByFrameID(1);
        if (slice.size() != 1 || std::abs(slice.value(2) - expectedByLayer[layer]) > 1e-12)
        {
            std::cerr << "element_nodal: layer " << layerLabels[layer] << " value wrong ("
                      << slice.value(2) << ")\n";
            return 78;
        }
    }
    if (!builder.readVarDatas(dataset, database, 1,
                              QStringLiteral("Plate stress at element nodes"),
                              QStringLiteral("XX")))
    {
        std::cerr << "element_nodal: cross-layer node-averaged read rejected\n";
        return 79;
    }
    {
        const QHash<int, double> average = database->getResultDataByFrameID(1);
        if (average.size() != 1 || std::abs(average.value(2) - 5.0) > 1e-12)
        {
            std::cerr << "element_nodal: cross-layer average wrong (" << average.value(2)
                      << ")\n";
            return 79;
        }
    }
    if (builder.readVarDatas(dataset, database, 1,
                             QStringLiteral("Broken element nodes"),
                             QStringLiteral("XX")))
    {
        std::cerr << "element_nodal: inconsistent layout was accepted\n";
        return 79;
    }

    std::cout << "element_nodal 0.3.12 checks passed\n";
    return 0;
}
}

int main(int argc, char** argv)
{
    const int selectionResult = runSelectionChecks();
    if (selectionResult != 0)
        return selectionResult;

    const int contractResult = runContractChecks033();
    if (contractResult != 0)
        return contractResult;

    QCoreApplication application(argc, argv);
    qInstallMessageHandler(&consoleMessageHandler);

    const int syntheticResult = runSyntheticDatasetChecks();
    if (syntheticResult != 0)
        return syntheticResult;

    const int streamResult = runD3plotStreamChecks();
    if (streamResult != 0)
        return streamResult;

    const int metadataResult = runSourceMetadataChecks();
    if (metadataResult != 0)
        return metadataResult;

    const int elementNodalResult = runElementNodalChecks();
    if (elementNodalResult != 0)
        return elementNodalResult;

    if (argc < 2)
    {
        // 无样本文件时仅验证分发规则与合成数据回归
        std::cout << "selection and synthetic checks passed\n";
        return 0;
    }

    const QString resultFile = QString::fromLocal8Bit(argv[1]);
    QString bdfFile;
    if (QFileInfo(resultFile).suffix().compare(QStringLiteral("op2"), Qt::CaseInsensitive) == 0)
    {
        bdfFile = argc > 2 ? QString::fromLocal8Bit(argv[2])
                           : MIOFile::mIONativeFile::findCompanionBdf(resultFile);
    }

    QPointer<PostData::mPostDataBase> database = new PostData::mPostDataBase(resultFile);
    MIOFile::mIONativeFile reader(resultFile, bdfFile, database);
    bool readSucceeded = false;

    // 输出窗口通道镜像到 stdout：诊断/截断提醒在测试进程默认无接收者（GUI 里
    // 由输出窗口消费），此处显式连接便于实测核对导入期告警文案。工作线程 emit，
    // 用 Direct 连接绕开事件循环时序（exec 在 finished 后即退出，queued 事件可能
    // 不再被处理）
    MxFunctions::mGlobalSignals* outputChannel = MxFunctions::mGlobalSignals::getInstance();
    const bool outputMirrorConnected = QObject::connect(
                     outputChannel,
                     static_cast<void (MxFunctions::mGlobalSignals::*)(
                         int, const QString&, bool, bool)>(
                         &MxFunctions::mGlobalSignals::outputMessageSig),
                     outputChannel,
                     [](int type, const QString& message, bool, bool) {
                         std::cout << "[output:" << type << "] " << qPrintable(message)
                                   << std::endl;
                     },
                     Qt::DirectConnection);
    std::cout << "outputMirrorConnected=" << (outputMirrorConnected ? 1 : 0) << std::endl;

    QObject::connect(&reader, &MIOFile::mIONativeFile::readNativeErrorSig,
                     [](MIOFile::IOFileErrorType error) {
                         std::cerr << "Native import error: " << error << "\n";
                     });
    QObject::connect(&reader, &MIOFile::mIONativeFile::finishedNativeFileSig,
                     [&](const QString& postName) {
                         readSucceeded = !postName.isEmpty();
                         if (!readSucceeded)
                         {
                             application.exit(3);
                             return;
                         }

                         const QList<int> frames = database->getFrameIds();
                         std::cout << "postName=" << qPrintable(postName) << "\n";
                         std::cout << "frameCount=" << frames.size() << "\n";
                         std::cout << "caseNames=" << qPrintable(database->getAllCaseNames().join(", "))
                                   << "\n";
                         for (int frame : frames)
                         {
                             const QStringList variables = database->getVariableByFrameID(frame);
                             std::cout << "frame=" << frame
                                       << " variables=" << qPrintable(variables.join(", "))
                                       << " resultCount=" << database->getResultDataByFrameID(frame).size()
                                       << "\n";
                         }

                         // 懒读校验：切到首个含变量的帧，读其首变量首分量；
                         // 矢量变量再按 GUI 的 Mag 伪分量（getVariableCompos 对 _isMag 追加）
                         // 读一次——不识别 Mag 会让 slot_showCloud 早退（无图例无颜色变化）
                         const auto varFrame = std::find_if(frames.cbegin(), frames.cend(),
                                                           [&database](int value) {
                                                               return !database->getVariableByFrameID(value).isEmpty();
                                                           });
                         if (varFrame != frames.cend())
                         {
                             const int frame = *varFrame;
                             const QString variable = database->getVariableByFrameID(frame).value(0);
                             const QStringList compos = database->getVariableCompos(variable);
                             const QString component = compos.isEmpty() ? QStringLiteral("X") : compos.first();
                             const bool switched = reader.readVarDatas(frame, variable,
                                                                      component,
                                                                      QStringLiteral("Real"));
                             std::cout << "switchFrame=" << frame
                                       << " variable=" << qPrintable(variable)
                                       << " component=" << qPrintable(component)
                                       << " success=" << (switched ? 1 : 0)
                                       << " resultCount="
                                       << database->getResultDataByFrameID(frame).size()
                                       << "\n";
                             if (!switched || database->getResultDataByFrameID(frame).isEmpty())
                                 readSucceeded = false;

                             const QStringList magCompos = database->getVariableCompos(variable);
                             if (magCompos.contains(QStringLiteral("Mag")))
                             {
                                 const bool magSwitched = reader.readVarDatas(frame, variable,
                                                                            QStringLiteral("Mag"),
                                                                            QStringLiteral("Real"));
                                 std::cout << "switchMag=" << (magSwitched ? 1 : 0)
                                           << " resultCount="
                                           << database->getResultDataByFrameID(frame).size()
                                           << "\n";
                                 if (!magSwitched || database->getResultDataByFrameID(frame).isEmpty())
                                     readSucceeded = false;
                             }

                             // 派生 vonMises 回归：任一变量分量列表含 vonMises 即必须可读
                             // （覆盖 ODB 的 S11 系标签经规范名归一后的派生路径）
                             const QStringList frameVariables = database->getVariableByFrameID(frame);
                             for (const QString& misesVariable : frameVariables)
                             {
                                 const QStringList misesCompos = database->getVariableCompos(misesVariable);
                                 if (!misesCompos.contains(QStringLiteral("vonMises")))
                                     continue;
                                 const bool misesSwitched = reader.readVarDatas(
                                     frame, misesVariable, QStringLiteral("vonMises"),
                                     QStringLiteral("Real"));
                                 std::cout << "switchVonMises variable=" << qPrintable(misesVariable)
                                           << " compos=" << qPrintable(misesCompos.join(","))
                                           << " success=" << (misesSwitched ? 1 : 0)
                                           << " resultCount="
                                           << database->getResultDataByFrameID(frame).size()
                                           << "\n";
                                 if (!misesSwitched || database->getResultDataByFrameID(frame).isEmpty())
                                     readSucceeded = false;
                                 break;
                             }

                             // 角点→节点平均伪变量回归：变量列表存在即必须可读
                             //（XX 分量 + 派生 vonMises，值非空），覆盖真实
                             // RST/OP2 角点源场反演路径
                             const auto nodalIt = std::find_if(
                                 frameVariables.cbegin(), frameVariables.cend(),
                                 [](const QString& name) {
                                     return name.endsWith(QStringLiteral(" (nodal averaged)"));
                                 });
                             if (nodalIt != frameVariables.cend())
                             {
                                 const QString& nodalVariable = *nodalIt;
                                 const bool nodalXX = reader.readVarDatas(
                                     frame, nodalVariable, QStringLiteral("XX"),
                                     QStringLiteral("Real"));
                                 const bool nodalMises = reader.readVarDatas(
                                     frame, nodalVariable, QStringLiteral("vonMises"),
                                     QStringLiteral("Real"));
                                 std::cout << "switchNodalAverage variable=" << qPrintable(nodalVariable)
                                           << " xx=" << (nodalXX ? 1 : 0)
                                           << " vonMises=" << (nodalMises ? 1 : 0)
                                           << " resultCount="
                                           << database->getResultDataByFrameID(frame).size() << "\n";
                                 if (!nodalXX || !nodalMises ||
                                     database->getResultDataByFrameID(frame).isEmpty())
                                     readSucceeded = false;
                             }
                         }
                         else
                         {
                             readSucceeded = false;
                         }

                         application.exit(readSucceeded ? 0 : 4);
                     });

    reader.start();
    const int result = application.exec();
    reader.wait();

    // 部分降级提示探针：直连 builder 重跑一遍 buildAll，打印 notes 文案
    //（限小文件——重读会令大样本导入耗时翻倍）
    if (result == 0 && QFileInfo(resultFile).size() < 100 * 1024 * 1024)
    {
        try
        {
            native_readers::ReadOptions probeOptions;
            if (!bdfFile.isEmpty())
                probeOptions.model_path = std::filesystem::path(bdfFile.toStdWString());
            const native_readers::Dataset probeDataset = native_readers::read_result(
                std::filesystem::path(resultFile.toStdWString()), probeOptions);
            PostData::mPostDataBase probeDatabase(resultFile);
            QString probeError;
            QStringList probeNotes;
            MIOFile::NativeReadersDataBuilder probeBuilder;
            if (!probeBuilder.buildAll(probeDataset, &probeDatabase, &probeError, &probeNotes))
            {
                std::cout << "notesProbe buildAll failed: " << qPrintable(probeError) << "\n";
                return 5;
            }
            for (const QString& note : probeNotes)
                std::cout << "skipNote=" << qPrintable(note) << "\n";
        }
        catch (const std::exception& error)
        {
            std::cout << "notesProbe failed: " << error.what() << "\n";
            return 5;
        }
    }
    return result;
}
