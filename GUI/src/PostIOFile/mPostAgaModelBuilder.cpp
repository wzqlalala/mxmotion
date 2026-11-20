#include "mPostAgaModelBuilder.h"

#include "AltairResultIO/aga_data.h"

#include "mPostDataBase.h"
#include "mPostModelData.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSet>

#include <algorithm>
#include <map>

namespace PostData
{
    class mPostDataBase;
}

namespace MxPost
{
    namespace
    {
        // (维度 flag, 顶点数) → Generic ASCII 单元类型
        bool mapElementType(int dim, int nodeCount, altairresultio::AgaElemType& out)
        {
            using ET = altairresultio::AgaElemType;
            if (dim == 2)
            {
                switch (nodeCount)
                {
                case 3:  out = ET::Tri3;  return true;
                case 4:  out = ET::Quad4; return true;
                case 6:  out = ET::Tri3;  return true;    // 高阶壳：P2 精确映射 TRI6，先按顶点导出
                case 8:  out = ET::Quad4; return true;
                default: return false;
                }
            }
            if (dim == 3)
            {
                switch (nodeCount)
                {
                case 4:  out = ET::Tet4;  return true;
                case 10: out = ET::Tet10; return true;
                case 8:  out = ET::Hex8;  return true;
                case 20: out = ET::Hex20; return true;
                default: return false;
                }
            }
            if (dim == 1)
            {
                if (nodeCount == 2) { out = ET::Rod; return true; }
                return false;
            }
            return false;
        }

        QString dimName(int dim)
        {
            switch (dim)
            {
            case 0: return QStringLiteral("0维");
            case 1: return QStringLiteral("1维");
            case 2: return QStringLiteral("2维");
            case 3: return QStringLiteral("3维");
            }
            return QStringLiteral("未知");
        }
    }

    bool buildAgaModelFromPost(PostData::mPostDataBase* postDB,
                               const QList<int>& frameIds,
                               altairresultio::AgaModel& out,
                               QString& error,
                               QString* skipped)
    {
        out = altairresultio::AgaModel();
        if (postDB == nullptr || frameIds.isEmpty())
        {
            error = QStringLiteral("后处理数据库为空或未选择帧");
            return false;
        }

        // 网格取自第一帧
        PostData::mPostModelData* modelData = postDB->getModelDataByFrameID(frameIds.first());
        if (modelData == nullptr)
        {
            error = QStringLiteral("帧 %1 的模型数据为空").arg(frameIds.first());
            return false;
        }

        out.modelName = "MxSim";

        // ---- 节点（真实 ID 去重）与单元 ----
        std::map<int, int> realIdToIndex;          // 真实节点 ID → 输出序号
        const QVector<int> partIds = modelData->getAllPartsIDs();
        int skippedCount = 0;
        std::map<QString, int> skippedByDim;

        for (int partId : partIds)
        {
            const QList<int> nodeRealIds = modelData->getNodeRealIds(partId);
            for (int realId : nodeRealIds)
            {
                if (realIdToIndex.find(realId) != realIdToIndex.end())
                    continue;
                realIdToIndex[realId] = static_cast<int>(out.nodes.size());
                const QVector3D coord = modelData->getVectorById(realId);
                altairresultio::AgaNode node;
                node.id = realId;
                node.x = coord.x();
                node.y = coord.y();
                node.z = coord.z();
                out.nodes.push_back(node);
            }

            const QList<int> eleIds = modelData->getElementIdsByPartId(partId);
            for (int eleId : eleIds)
            {
                int dim = -1;
                const std::vector<int> conn = modelData->getNodeIdsByEleIdWithoutBuild(partId, eleId, dim);
                altairresultio::AgaElemType type;
                if (!mapElementType(dim, static_cast<int>(conn.size()), type))
                {
                    ++skippedCount;
                    skippedByDim[QString("%1维%2节点").arg(dim).arg(conn.size())] += 1;
                    continue;
                }

                altairresultio::AgaElement element;
                element.id = eleId;
                element.type = type;
                element.connectivity.reserve(conn.size());
                for (int nid : conn)
                    element.connectivity.push_back(nid);
                out.elements.push_back(element);
            }
        }

        if (out.nodes.empty() || out.elements.empty())
        {
            error = QStringLiteral("网格抽取结果为空（节点 %1 / 单元 %2）")
                        .arg(out.nodes.size()).arg(out.elements.size());
            return false;
        }

        // ---- 帧数据集 ----
        for (int frameId : frameIds)
        {
            altairresultio::AgaFrame frame;
            frame.index = frameId;
            frame.name = QStringLiteral("Frame %1").arg(frameId).toStdString();

            // 位移：变形动画 / 变形图
            const QHash<int, QVector3D> disp = postDB->getDisplacementDataByFrameID(frameId);
            if (!disp.isEmpty())
            {
                altairresultio::AgaDataset ds;
                ds.name = "Displacement";
                ds.type = altairresultio::AgaDatasetType::Vector;
                ds.location = altairresultio::AgaDatasetLocation::Nodal;
                ds.values.reserve(out.nodes.size() * 3);
                for (const auto& node : out.nodes)
                {
                    const auto it = disp.constFind(node.id);
                    if (it != disp.constEnd())
                    {
                        ds.values.push_back(it->x());
                        ds.values.push_back(it->y());
                        ds.values.push_back(it->z());
                    }
                    else
                    {
                        ds.values.insert(ds.values.end(), {0.0, 0.0, 0.0});
                    }
                }
                frame.datasets.push_back(ds);
            }

            // 标量云图：当前变量（自动区分节点/单元位置）
            int flag = 0;
            const QHash<int, double> values = postDB->getResultDataByFrameID(frameId, flag);
            if (!values.isEmpty())
            {
                const QStringList vars = postDB->getVariableByFrameID(frameId);
                const int varType = vars.isEmpty()
                    ? 0 : postDB->getVariableType(vars.first());   // 0=节点 1=单元

                altairresultio::AgaDataset ds;
                ds.name = (vars.isEmpty() ? QStringLiteral("Scalar") : vars.first()).toStdString();
                ds.type = altairresultio::AgaDatasetType::Scalar;
                ds.location = (varType == 1)
                    ? altairresultio::AgaDatasetLocation::Elemental
                    : altairresultio::AgaDatasetLocation::Nodal;

                if (ds.location == altairresultio::AgaDatasetLocation::Nodal)
                {
                    ds.values.reserve(out.nodes.size());
                    for (const auto& node : out.nodes)
                    {
                        const auto it = values.constFind(node.id);
                        ds.values.push_back(it != values.constEnd() ? it.value() : 0.0);
                    }
                }
                else
                {
                    // 单元数据：哈希键为单元真实 ID，按输出单元表顺序取值
                    ds.values.reserve(out.elements.size());
                    for (const auto& element : out.elements)
                    {
                        const auto it = values.constFind(element.id);
                        ds.values.push_back(it != values.constEnd() ? it.value() : 0.0);
                    }
                }
                frame.datasets.push_back(ds);
            }

            out.frames.push_back(frame);
        }

        if (out.frames.empty())
        {
            error = QStringLiteral("未抽取到任何帧数据");
            return false;
        }

        if (skipped != nullptr && skippedCount > 0)
        {
            QStringList parts;
            for (const auto& it : skippedByDim)
                parts << QStringLiteral("%1 x%2").arg(it.first).arg(it.second);
            *skipped = parts.join(QStringLiteral(", "));
        }
        return true;
    }

    QString findHvTransExe()
    {
        // 1) %ALTAIR_HOME%/hwdesktop/hv/bin/win64/hvtrans.exe
        const QString altairHome = qEnvironmentVariable("ALTAIR_HOME");
        if (!altairHome.isEmpty())
        {
            const QString candidate = altairHome + "/hwdesktop/hv/bin/win64/hvtrans.exe";
            if (QFileInfo::exists(candidate))
                return QDir(candidate).absolutePath();
        }

        // 2) Program Files 下扫描 Altair 版本目录（取版本号最大的）
        const QString programFiles = qEnvironmentVariable("ProgramFiles", "C:/Program Files");
        QDir dir(programFiles + "/Altair");
        if (dir.exists())
        {
            const QFileInfoList versions =
                dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
            for (const QFileInfo& versionDir : versions)
            {
                const QString candidate = versionDir.absoluteFilePath()
                    + "/hwdesktop/hv/bin/win64/hvtrans.exe";
                if (QFileInfo::exists(candidate))
                    return candidate;
            }
        }
        return QString();
    }
}
