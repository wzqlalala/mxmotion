//Functions
#include "mOtherFunction.h"
//BasicData
#include "mMeshData.h"
//Mesh
#include <MeSurfaceWrap/Foundation/Mesh/FaceIndexedMesh.h>
#include <MeSurfaceWrap/Foundation/Mesh/RadialHalfedgeMesh.h>
#include <MeSurfaceWrap/Foundation/Surface/SurfaceModel.h>
#include <MeSurfaceWrap/Foundation/Surface/SurfaceFeatureState.h>
#include <MeSurfaceWrap/PostProcess/Remeshing/DenseRemesher.h>
#include <MeSurfaceWrap/PostProcess/Remeshing/SurfaceRemesher.h>
#include <MeSurfaceWrap/PreProcess/Build/SurfaceBuild.h>
#include <MeSurfaceWrap/PreProcess/Repair/SurfaceRepair.h>
#include <MeSurfaceWrap/PreProcess/Repair/SurfaceRepairOptions.h>
//Mech
#include "mMeshPanel.h"
#include "mSTLRemeshPanel.h"
#include "PickCommonFunction.h"
//std
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>
//Qt
#include <QSet>
#include <QSlider>
#include <QSpinBox>

using namespace me::surface;
using namespace MechPreData;
using namespace MeshPanelData;

namespace MechMainWindow
{

    mSTLRemeshPanel::mSTLRemeshPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

        FoldSection* m_ObjectFold = new FoldSection("对象", this);
        posPick.initWidget("对象");
        posPick.setState(false);
        m_ObjectFold->addRow(posPick.propwid);

        FoldSection* m_ParaFold = new FoldSection("参数", this);
        QLabel* method = new PanelLabel("形状");
        meshtype = new QComboBox;
        meshtype->addItems({"三角形"});
        QLabel* size = new PanelLabel("尺寸");
        meshsize = new mCalcLineEdit("2");
        m_ParaFold->addRow(method, meshtype);
        m_ParaFold->addRow(size, meshsize);

        // 二面角阈值：滑块(0~180°) 与输入框双向联动，重剖分时 ≥ 该角度的折痕保持为锐边
        QLabel* dihedralLabel = new PanelLabel("二面角");
        dihedralSlider = new QSlider(Qt::Horizontal, this);
        dihedralSlider->setRange(0, 180);
        dihedralSlider->setValue(20);
        dihedralSpin = new QSpinBox(this);
        dihedralSpin->setRange(0, 180);
        dihedralSpin->setSuffix(" 度");
        dihedralSpin->setValue(20);
        dihedralSpin->setMinimumWidth(70);
        auto* dihedralRow = new QWidget(this);
        auto* dihedralLay = new QHBoxLayout(dihedralRow);
        dihedralLay->setContentsMargins(0, 0, 0, 0);
        dihedralLay->addWidget(dihedralSlider, 1);
        dihedralLay->addWidget(dihedralSpin);
        m_ParaFold->addRow(dihedralLabel, dihedralRow);

        QFrame* horizontalLine = createHorizontalLine();

        QPushButton* btn_feature = new QPushButton("显示锐利边");
        QPushButton* btn_remesh = new QPushButton("重剖分");
        QHBoxLayout* HLayout = new QHBoxLayout;
        HLayout->addStretch();
        HLayout->addWidget(btn_feature);
        HLayout->addWidget(btn_remesh);

        mainLay->addWidget(m_ObjectFold);
        mainLay->addWidget(m_ParaFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(HLayout);
        mainLay->addStretch();

        connect(posPick.btnok, &QToolButton::clicked, this, [=]() { syncPickFromViewport(false); });
        connect(posPick.btnclear, &QToolButton::clicked, this, [=]() { clearPickState(); });
        connect(posPick.btnadd, &QToolButton::clicked, this, [=]() { syncPickFromViewport(true); });
        connect(meshtype, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=]() {_meshOptPanelData->setMeshType(meshtype->currentText()); });
        connect(meshsize, &QLineEdit::textChanged, this, [=]() {_meshOptPanelData->setMeshSize(meshsize->text().toDouble()); });
        // 滑块与输入框双向联动：两边范围一致（0~180），setValue 互相触发对方 valueChanged，
        // QSlider/QSpinBox 内部对相同值不再发信号，天然防循环
        connect(dihedralSlider, &QSlider::valueChanged, this, [=](int v) {
            m_dihedralAngle = v;
            dihedralSpin->setValue(v);
            });
        connect(dihedralSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int v) {
            m_dihedralAngle = v;
            dihedralSlider->setValue(v);
            });
        connect(btn_feature, &QPushButton::clicked, this, &mSTLRemeshPanel::showFeatureEdges);
        connect(btn_remesh,&QPushButton::clicked,this,&mSTLRemeshPanel::remesh);
    }

    mSTLRemeshPanel::~mSTLRemeshPanel()
    {

    }

    void mSTLRemeshPanel::syncPickFromViewport(bool append)
    {
        // append 为 true 时保留历史拾取再追加去重，否则以当前视口选择覆盖
        std::vector<RenderPickResultData> ids = append ? MeshIDs : std::vector<RenderPickResultData>{};
        std::vector<MeshSelectInfoData> picked = getCVM->getPreRenderWidget()->getSelectionContainer()
            ->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
        for (const auto& ele : picked)
            pushBackUnique(ids, SelectData2ResultData(SelectionInfoType::MeshElement, ele));

        MeshIDs = ids;
        posPick.setState(!MeshIDs.empty());
        if (!MeshIDs.empty())
            posPick.labelpicked->setText(QString("已选择:%1个单元").arg(MeshIDs.size()));
        // MeshIDs 后续 remesh() 还要用，这里只能拷贝不能 move
        _meshOptPanelData->setMeshIDs(MeshIDs);
        _meshOptPanelData->setPickType(QString("拾取"));
    }

    void mSTLRemeshPanel::clearPickState()
    {
        MeshIDs.clear();
        posPick.setState(false);
        _meshOptPanelData->setMeshIDs({});
        _meshOptPanelData->setPickType(QString("拾取"));
    }

    void mSTLRemeshPanel::newMeshPanel()
    {
        // 面板复用，每次新建时重置为未选择状态
        MeshIDs.clear();
        if (_meshOptPanelData)
        {
            _meshOptPanelData->setPickType(QString("拾取"));
            _meshOptPanelData->setMeshIDs({});
        }
        posPick.setState(false);

        // 清空视口中残留的拾取，避免第一次点击添加按钮把历史选择全部加入
        if (getCVM && getCVM->getPreRenderWidget())
        {
            getCVM->clearPick();
            getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        }
    }

    void mSTLRemeshPanel::editMeshPanel(int meshId)
    {
        _meshOptPanelData = getCVM->getModelData()->getManager<mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        updatePanel();
    }

    void mSTLRemeshPanel::updatePanel()
    {
        MeshIDs = _meshOptPanelData->getMeshIDs();
        if (MeshIDs.empty())
        {
            posPick.setState(false);
        }
        else
        {
            posPick.setState(true);
            posPick.labelpicked->setText(QString("已选择:%1个单元").arg(MeshIDs.size()));
        }
        auto type = _meshOptPanelData->getMeshType();
        auto size = _meshOptPanelData->getMeshSize();
        meshtype->setCurrentText(type);
        meshsize->setText(MxFunctions::mNumberToString(size));
    }

    void mSTLRemeshPanel::remesh()
    {
        // 获取拾取的网格单元
        if (MeshIDs.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "请选择需要重剖分的网格对象！", true);
            return;
        }

        // 获取网格数据
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        if (!meshData)
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "未获取到网格数据！", true);
            return;
        }

        // STL 重剖分仅支持三角形单元，拾取到其他类型时明确报错（而非静默跳过）
        for (const auto& ele : MeshIDs)
        {
            if (meshData->getMeshTypeByEleId(ele.result_id) != MeshTri)
            {
                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "选取网格类型错误：STL 重剖分仅支持三角形网格！", true);
                return;
            }
        }

        // 拾取单元涉及的去重节点（真实节点ID）
        std::set<int> nodeIds = meshData->getNodeIdsByEleIdsDeWeight(MeshIDs);
        if (nodeIds.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "拾取单元未包含有效节点！", true);
            return;
        }

        // 真实节点ID -> FaceIndexedMesh 顶点索引（0-based）
        std::unordered_map<int, std::uint32_t> nodeIdToIndex;
        nodeIdToIndex.reserve(nodeIds.size());

        // 原始数据：扁平顶点坐标 [x,y,z]×N + 0-based 三角形连接（每面 3 顶点，法向由绕序决定）
        std::vector<double> points;
        points.reserve(nodeIds.size() * 3);
        for (int nodeId : nodeIds)
        {
            QVector3D coor = meshData->getVectorById(nodeId);
            nodeIdToIndex[nodeId] = static_cast<std::uint32_t>(points.size() / 3);
            points.push_back(coor.x());
            points.push_back(coor.y());
            points.push_back(coor.z());
        }

        // 获取三角形单元的节点索引（类型已在上文统一校验，此处均为 3 节点）
        std::vector<std::uint32_t> tris;
        tris.reserve(MeshIDs.size() * 3);
        for (const auto& ele : MeshIDs)
        {
            std::vector<int> nodeIDs = meshData->getNodeIndexByID(ele.part_id, ele.result_id);
            for (int nodeId : nodeIDs)
            {
                tris.push_back(nodeIdToIndex[nodeId]);
            }
        }

        // 创建面索引网格（无哨兵 CSR：每个面记录起始角点，末面结束位置由 indices.size() 隐式给出）
        FaceIndexedMesh indexedMesh;
        indexedMesh.dimension = 3;
        indexedMesh.positions = std::move(points);
        const std::uint32_t faceCount = static_cast<std::uint32_t>(tris.size() / 3);
        indexedMesh.offsets.resize(faceCount);
        for (std::uint32_t f = 0; f < faceCount; ++f)
        {
            indexedMesh.offsets[f] = f * 3U;
        }
        indexedMesh.indices = std::move(tris);

        if (indexedMesh.num_faces() == 0 || !indexedMesh.valid_topology())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "拾取单元无法构建有效的三角网格！", true);
            return;
        }

        // 创建预处理工作区（MxMech 直接读入的 STL 常含重复顶点、退化面、非流形边等缺陷，
        // 而 DenseRemesher 要求输入为"可信流形三角网格"。先经过 build_surface_mesh + 确定性清理，
        // 把脏输入修复到满足 Dense 前置条件，与 MeGrid 的加载流程保持一致。）
        SurfacePreprocessWorkspace model;
        SurfaceDiagnostics diagnostics;
        SurfaceStatus status;

        if (!build_surface_mesh(indexedMesh, model, &diagnostics, &status) || !status.ok())
        {
            QString reason = QString::fromStdString(status.message);
            if (reason.isEmpty())
            {
                reason = QStringLiteral("未知原因");
            }
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, QString("网格拓扑构建失败：%1").arg(reason), true);

            for (const auto& issue : diagnostics.issues)
            {
                if (issue.severity == SurfaceIssueSeverity::Info)continue;

                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage,
                    QString("  [%1] %2").arg(QString::fromStdString(issue.module), QString::fromStdString(issue.message)), true);
            }
            return;
        }

        // 确定性清理：焊接重复顶点、删除退化/重复/零面积面、删除孤立点。
        // 保守档位下这些操作都是低风险且可回滚的，能显著提升 DenseRemesher 的成功率。
        {
            SurfaceDeterministicCleanupRepairOptions cleanupOpts;
            cleanupOpts.enabled                  = true;
            cleanupOpts.weld_duplicate_vertices  = true;
            cleanupOpts.merge_close_vertices     = false;
            cleanupOpts.remove_degenerate_faces  = true;
            cleanupOpts.remove_zero_area_faces   = true;
            cleanupOpts.remove_duplicate_faces   = true;
            cleanupOpts.remove_isolated_vertices = true;
            repair_deterministic_cleanup(model, cleanupOpts, &diagnostics);
        }

        // 重剖分参数：目标边长取界面输入的网格尺寸
        RemeshOptions options;
        options.target_edge_length = meshsize->text().toDouble();   // 目标边长：重剖分后三角形边的期望长度，必填且 >0

        // 尺寸非法（非数字、0、负数、无穷）时报错并退出；toDouble 对非法输入返回 0
        if (!(options.target_edge_length > 0.0) || !std::isfinite(options.target_edge_length))
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "网格尺寸必须为正数！", true);
            return;
        }
        // 与 MeGrid 对齐：均匀重网格不设置 approximation_error（0 表示关闭自适应曲率尺寸场）
        options.approximation_error = 0.0;
        options.allow_feature_rediscretization = true;  // 允许沿特征线重新离散：默认 false；Dense 路径强制要求为 true，漏设直接报 InvalidParameter
        options.iterations = 10U;                       // 迭代轮数：每轮依次执行 split/collapse/flip/relaxation，轮数越多尺寸越均匀、耗时越长
        // 与 MeGrid 对齐：松弛步数 1，投影启用，特征/边界/约束保持，拒绝非流形
        options.relaxation_steps = 1U;
        options.use_projection                   = true;
        options.constraints.preserve_boundary    = true;
        options.constraints.preserve_features    = true;
        options.constraints.preserve_constrained = true;
        options.constraints.reject_non_manifold  = true;

        DenseBatchFeatureRequest features;
        features.dihedral_threshold_degrees = m_dihedralAngle;     // 二面角阈值（面板滑块输入）

        DenseBatchRemeshResult result = DenseRemesher{}.remesh_batch(model, options, features); // 进行重剖分

        // 失败时回退到 SurfaceRemesher（普通 PMP 风格重网格），对脏输入更宽容
        if (!result.ok())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage,
                "Dense 重剖分未通过门禁，尝试普通重剖分路径...", true);

            // 确定性清理选项需要与前面保持一致
            SurfaceDeterministicCleanupRepairOptions cleanupOpts;
            cleanupOpts.enabled                  = true;
            cleanupOpts.weld_duplicate_vertices  = true;
            cleanupOpts.merge_close_vertices     = false;
            cleanupOpts.remove_degenerate_faces  = true;
            cleanupOpts.remove_zero_area_faces   = true;
            cleanupOpts.remove_duplicate_faces   = true;
            cleanupOpts.remove_isolated_vertices = true;

            // SurfaceRemesher 直接操作拓扑，需要重新构建工作模型
            SurfacePreprocessWorkspace fallbackModel;
            SurfaceDiagnostics fallbackDiag;
            SurfaceStatus fallbackStatus;
            if (!build_surface_mesh(indexedMesh, fallbackModel, &fallbackDiag, &fallbackStatus) || !fallbackStatus.ok())
            {
                QString reason = QString::fromStdString(fallbackStatus.message);
                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, QString("网格拓扑构建失败：%1").arg(reason), true);
                return;
            }
            repair_deterministic_cleanup(fallbackModel, cleanupOpts, &fallbackDiag);

            // 普通重网格使用相同的参数，但不强制 allow_feature_rediscretization
            RemeshOptions fallbackOptions = options;
            fallbackOptions.allow_feature_rediscretization = false;

            const SurfaceRemesher fallbackRemesher;
            auto fallbackResult = fallbackRemesher.remesh(fallbackModel.topology, fallbackOptions);
            if (!fallbackResult.ok())
            {
                QString reason = QString::fromStdString(fallbackResult.status.message);
                if (reason.isEmpty())
                {
                    reason = QStringLiteral("未知原因");
                }
                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, QString("网格重剖分失败：%1").arg(reason), true);
                return;
            }

            // 将结果写入与 Dense 路径一致的输出结构
            result.disposition = DenseRemeshDisposition::CandidateReady;
            result.algorithm   = std::move(fallbackResult);
            DenseRemeshCandidate candidate;
            candidate.topology      = std::move(fallbackModel.topology);
            candidate.features      = std::move(fallbackModel.features);
            candidate.feature_state = SurfaceFeatureState{};
            result.output           = std::move(candidate);
        }

        // 失败原因检测（Dense 和回退路径共用）
        if (!result.ok())
        {
            QString reason = QString::fromStdString(result.algorithm.status.message);
            if (reason.isEmpty())
            {
                reason = QStringLiteral("未知原因");
            }
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, QString("网格重剖分失败：%1").arg(reason), true);
            return;
        }
        if (!result.changed())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, "拾取区域网格已满足目标尺寸要求，无需重剖分。", true);
            return;
        }

        // 获取结果网格
        const RadialHalfedgeMesh& result_mesh = result.output->topology;

        // 将结果网格转换为一个新部件
        FaceIndexedMesh out;
        SurfaceStatus outStatus;
        if (!result_mesh.to_mesh(out, nullptr, &outStatus) || !outStatus.ok())
        {
            QString reason = QString::fromStdString(outStatus.message);
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, QString("重剖分结果导出失败：%1").arg(reason), true);
            return;
        }

        // 结果顶点批量注册为真实节点（局部顶点索引 -> 真实节点ID）
        const std::size_t addCount = out.num_vertices();
        std::vector<QVector3D> nodeCoors;
        nodeCoors.reserve(addCount);
        for (std::size_t i = 0; i < addCount; ++i)
        {
            nodeCoors.emplace_back(out.positions[3 * i], out.positions[3 * i + 1], out.positions[3 * i + 2]);
        }
        const std::vector<int> realNodeIds = meshData->createNodes(nodeCoors);

        // 三角形连接关系换成真实节点ID
        QVector<int> index;
        index.reserve(static_cast<int>(out.indices.size()));
        for (std::uint32_t idx : out.indices)
        {
            index.push_back(realNodeIds[idx]);
        }
            
        // 批量建成新部件（CTRIA3 三角形壳单元）；每次执行生成独立部件 STLRemesh_1、STLRemesh_2…
        QString partName;
        {
            const QStringList existNames = meshData->getAllPartNames();
            // Qt5 的 QSet 没有迭代器区间构造，逐个 insert 建哈希集，避免循环里反复扫列表
            QSet<QString> nameSet;
            for (const QString& name : existNames)
                nameSet.insert(name);
            for (int i = 1; ; ++i)
            {
                partName = QString("STLRemesh_%1").arg(i);
                if (!nameSet.contains(partName))break;
            }
        }
        meshData->createMeshToPart(CTRIA3, index, partName, 3);
        mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage, QString("重剖分完成：生成 %1 个节点、%2 个三角形单元，已写入部件 %3。").arg(out.num_vertices()).arg(out.num_faces()).arg(partName), true);

        // 更新模型树
        vector<int> partIds = getCVM->getModelData()->getAllPhyID(Physical_Part);
        emit mGlobalSignals::getInstance()->createUpdatePartSig(true, set<int>{partIds.end() - 1, partIds.end()});
        // 清除拾取与高亮
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightMesh();
        MeshIDs.clear();
    }

    void mSTLRemeshPanel::showFeatureEdges()
    {
        // 获取拾取的网格单元
        if (MeshIDs.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "请先拾取需要检测锐利边的网格对象！", true);
            return;
        }
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        if (!meshData)
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "未获取到网格数据！", true);
            return;
        }

        // 拾取单元 -> 面索引网格（与 remesh() 同构，但保留 真实节点ID 供回映射）
        std::set<int> nodeIds = meshData->getNodeIdsByEleIdsDeWeight(MeshIDs);
        if (nodeIds.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "拾取单元未包含有效节点！", true);
            return;
        }
        std::unordered_map<int, std::uint32_t> nodeIdToIndex;
        nodeIdToIndex.reserve(nodeIds.size());
        std::vector<int> indexToNodeId;   // 局部顶点索引 -> 真实节点ID
        indexToNodeId.reserve(nodeIds.size());
        std::vector<double> points;
        points.reserve(nodeIds.size() * 3);
        for (int nodeId : nodeIds)
        {
            QVector3D coor = meshData->getVectorById(nodeId);
            nodeIdToIndex[nodeId] = static_cast<std::uint32_t>(indexToNodeId.size());
            indexToNodeId.push_back(nodeId);
            points.push_back(coor.x());
            points.push_back(coor.y());
            points.push_back(coor.z());
        }
        std::vector<std::uint32_t> tris;
        tris.reserve(MeshIDs.size() * 3);
        for (const auto& ele : MeshIDs)
        {
            std::vector<int> nodeIDs = meshData->getNodeIndexByID(ele.part_id, ele.result_id);
            if (nodeIDs.size() != 3)
                continue;
            for (int nodeId : nodeIDs)
                tris.push_back(nodeIdToIndex[nodeId]);
        }
        FaceIndexedMesh indexedMesh;
        indexedMesh.dimension = 3;
        indexedMesh.positions = std::move(points);
        const std::uint32_t faceCount = static_cast<std::uint32_t>(tris.size() / 3);
        indexedMesh.offsets.resize(faceCount);
        for (std::uint32_t f = 0; f < faceCount; ++f)
            indexedMesh.offsets[f] = f * 3U;
        indexedMesh.indices = std::move(tris);

        if (indexedMesh.num_faces() == 0 || !indexedMesh.valid_topology())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "拾取单元无法构建有效的三角网格！", true);
            return;
        }

        // 半边拓扑 + 二面角特征检测
        SurfaceModel model;
        SurfaceDiagnostics diagnostics;
        SurfaceStatus status;
        if (!RadialHalfedgeMesh::from_mesh(indexedMesh, model.topology, &diagnostics, &status) || !status.ok())
        {
            QString reason = QString::fromStdString(status.message);
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, QString("网格拓扑构建失败：%1").arg(reason), true);
            return;
        }
        SurfaceFeatureState featureState;
        featureState.reset(model);
        auto detectResult = featureState.detect_dihedral(model, m_dihedralAngle);
        if (!detectResult.ok())
        {
            QString reason = QString::fromStdString(detectResult.message);
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::ErrorMessage, QString("锐利边检测失败：%1").arg(reason), true);
            return;
        }

        // 特征边（局部顶点索引对）-> 真实节点ID对
        const auto& edges = model.topology.edges();
        std::vector<MeshSelectInfoData> selectData;
        for (SurfaceEdgeId edgeId : model.features.feature_edges)
        {
            const auto& edge = edges[edgeId];
            if (!edge.alive)
                continue;
            const int n0 = indexToNodeId[edge.v0];
            const int n1 = indexToNodeId[edge.v1];
            selectData.push_back({ 2, 0, std::min(n0, n1), std::max(n0, n1), 0 });
        }

        if (selectData.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage,
                QString("在二面角阈值 %1° 下未检测到锐利边。").arg(m_dihedralAngle), true);
            return;
        }

        // 高亮显示
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()
            ->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshLine, selectData);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
        mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::NormalMessage,
            QString("检测到 %1 条锐利边（二面角 ≥ %2°），已在视口中高亮。").arg(selectData.size()).arg(m_dihedralAngle), true);
    }

    bool mSTLRemeshPanel::computeMeanCurvature(std::vector<double>& curvatures,
        std::vector<QVector3D>& normals,
        std::vector<double>& areas,
        std::vector<int>& indexToNodeId)
    {
        curvatures.clear();
        normals.clear();
        areas.clear();
        indexToNodeId.clear();

        // 获取拾取的网格单元
        if (MeshIDs.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "请先拾取需要计算曲率的网格对象！", true);
            return false;
        }
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
        if (!meshData)
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "未获取到网格数据！", true);
            return false;
        }

        // 仅支持三角形单元
        for (const auto& ele : MeshIDs)
        {
            if (meshData->getMeshTypeByEleId(ele.result_id) != MeshTri)
            {
                mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "平均曲率计算仅支持三角形网格！", true);
                return false;
            }
        }

        // 拾取单元涉及的去重节点（真实节点ID -> 局部顶点索引）
        std::set<int> nodeIds = meshData->getNodeIdsByEleIdsDeWeight(MeshIDs);
        if (nodeIds.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "拾取单元未包含有效节点！", true);
            return false;
        }
        std::unordered_map<int, std::uint32_t> nodeIdToIndex;
        nodeIdToIndex.reserve(nodeIds.size());
        indexToNodeId.reserve(nodeIds.size());
        std::vector<QVector3D> pts;
        pts.reserve(nodeIds.size());
        for (int nodeId : nodeIds)
        {
            nodeIdToIndex[nodeId] = static_cast<std::uint32_t>(pts.size());
            indexToNodeId.push_back(nodeId);
            pts.push_back(meshData->getVectorById(nodeId));
        }

        // 三角形单元（局部顶点索引）
        struct LocalTri { std::uint32_t a, b, c; };
        std::vector<LocalTri> tris;
        tris.reserve(MeshIDs.size());
        for (const auto& ele : MeshIDs)
        {
            std::vector<int> nodeIDs = meshData->getNodeIndexByID(ele.part_id, ele.result_id);
            if (nodeIDs.size() != 3)
                continue;
            tris.push_back({ nodeIdToIndex[nodeIDs[0]], nodeIdToIndex[nodeIDs[1]], nodeIdToIndex[nodeIDs[2]] });
        }
        if (tris.empty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(MxFunctions::WarningMessage, "拾取单元无法构建有效的三角网格！", true);
            return false;
        }

        const std::size_t n = pts.size();

        // 顶点邻接
        std::vector<std::vector<std::uint32_t>> adj(n);
        for (const auto& t : tris)
        {
            auto addEdge = [&adj](std::uint32_t u, std::uint32_t v) {
                auto& vec = adj[u];
                if (std::find(vec.begin(), vec.end(), v) == vec.end())
                    vec.push_back(v);
                };
            addEdge(t.a, t.b); addEdge(t.b, t.a);
            addEdge(t.b, t.c); addEdge(t.c, t.b);
            addEdge(t.c, t.a); addEdge(t.a, t.c);
        }

        // 余切权重累加器（按无向边）、重心面积、面法向累加器
        std::map<std::pair<std::uint32_t, std::uint32_t>, double> cotSum;
        std::vector<double> baryArea(n, 0.0);
        std::vector<QVector3D> faceNormalSum(n, QVector3D(0, 0, 0));

        auto cotOf = [](const QVector3D& u, const QVector3D& v) -> double {
            double lenU = u.length(), lenV = v.length();
            if (lenU < 1e-30 || lenV < 1e-30)
                return 0.0;
            double cosA = QVector3D::dotProduct(u, v) / (lenU * lenV);
            cosA = std::max(-1.0, std::min(1.0, cosA));
            double sinA = std::sqrt(std::max(0.0, 1.0 - cosA * cosA));
            if (sinA < 1e-12)
                return 0.0;
            return cosA / sinA;
            };

        for (const auto& t : tris)
        {
            const QVector3D& A = pts[t.a];
            const QVector3D& B = pts[t.b];
            const QVector3D& C = pts[t.c];
            QVector3D cross = QVector3D::crossProduct(B - A, C - A);
            double doubleArea = cross.length();
            if (doubleArea < 1e-30)
                continue;
            double triArea = 0.5 * doubleArea;
            baryArea[t.a] += triArea / 3.0;
            baryArea[t.b] += triArea / 3.0;
            baryArea[t.c] += triArea / 3.0;
            QVector3D nrm = cross / doubleArea;
            faceNormalSum[t.a] += nrm;
            faceNormalSum[t.b] += nrm;
            faceNormalSum[t.c] += nrm;

            // 每条边对面那个角贡献余切权重
            auto key = [](std::uint32_t u, std::uint32_t v) { return std::minmax(u, v); };
            cotSum[key(t.a, t.b)] += cotOf(A - C, B - C);
            cotSum[key(t.b, t.c)] += cotOf(B - A, C - A);
            cotSum[key(t.c, t.a)] += cotOf(C - B, A - B);
        }

        // Meyer 混合 Voronoi 面积
        std::vector<double> mixedArea(n, 0.0);
        for (const auto& t : tris)
        {
            const QVector3D& A = pts[t.a];
            const QVector3D& B = pts[t.b];
            const QVector3D& C = pts[t.c];
            QVector3D cross = QVector3D::crossProduct(B - A, C - A);
            double doubleArea = cross.length();
            if (doubleArea < 1e-30)
                continue;
            double triArea = 0.5 * doubleArea;

            // 三个角（弧度）
            auto angleAt = [&cotOf](const QVector3D& u, const QVector3D& v) -> double {
                double c = cotOf(u, v);
                return std::atan2(1.0, c);  // atan2(sin,cos)，sin>0 时等价 acot(c)
                };
            double angA = angleAt(B - A, C - A);
            double angB = angleAt(C - B, A - B);
            double angC = angleAt(A - C, B - C);
            const double PI = 3.14159265358979323846;

            if (angA >= PI / 2.0)
            {
                mixedArea[t.a] += triArea / 2.0;
                mixedArea[t.b] += triArea / 4.0;
                mixedArea[t.c] += triArea / 4.0;
            }
            else if (angB >= PI / 2.0)
            {
                mixedArea[t.b] += triArea / 2.0;
                mixedArea[t.a] += triArea / 4.0;
                mixedArea[t.c] += triArea / 4.0;
            }
            else if (angC >= PI / 2.0)
            {
                mixedArea[t.c] += triArea / 2.0;
                mixedArea[t.a] += triArea / 4.0;
                mixedArea[t.b] += triArea / 4.0;
            }
            else
            {
                // 锐角三角形：Voronoi 面积 = (|a|²·cotA' + |b|²·cotB')/8
                double ab2 = (B - A).lengthSquared();
                double bc2 = (C - B).lengthSquared();
                double ca2 = (A - C).lengthSquared();
                mixedArea[t.a] += (ab2 * cotOf(A - C, B - C) + ca2 * cotOf(C - B, A - B)) / 8.0;
                mixedArea[t.b] += (ab2 * cotOf(A - C, B - C) + bc2 * cotOf(C - B, A - B)) / 8.0;
                mixedArea[t.c] += (ca2 * cotOf(C - B, A - B) + bc2 * cotOf(A - C, B - C)) / 8.0;
            }
        }
        for (std::size_t i = 0; i < n; ++i)
        {
            // 边界/退化情况下 mixed 面积可能为 0，回退到重心面积
            areas.push_back(mixedArea[i] > 1e-30 ? mixedArea[i] : baryArea[i]);
        }

        // 平均曲率向量 K = 1/(2A) * Σ (cotα+cotβ)(vi-vj)
        curvatures.resize(n, 0.0);
        normals.resize(n, QVector3D(0, 0, 1));
        for (std::size_t i = 0; i < n; ++i)
        {
            QVector3D K(0, 0, 0);
            for (std::uint32_t j : adj[i])
            {
                auto it = cotSum.find(std::minmax(static_cast<std::uint32_t>(i), j));
                if (it == cotSum.end())
                    continue;
                K += (pts[i] - pts[j]) * static_cast<float>(it->second);
            }
            if (areas[i] > 1e-30)
                K /= static_cast<float>(2.0 * areas[i]);

            // 顶点法向（面积加权面法向平均）
            QVector3D nrm = faceNormalSum[i];
            if (nrm.lengthSquared() > 1e-24)
                nrm.normalize();
            normals[i] = nrm;

            // 带符号平均曲率：H = |K|/2，符号取 K 与法向同向为正
            double sign = QVector3D::dotProduct(K, nrm) >= 0.0 ? 1.0 : -1.0;
            curvatures[i] = sign * K.length() / 2.0;
        }

        return true;
    }
}
