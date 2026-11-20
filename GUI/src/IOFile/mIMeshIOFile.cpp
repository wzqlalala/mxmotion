#include "mIMxCaeFile.h"
#include "mIMeshIOFile.h"

//QT
#include <mGlobalSignals.h>
#include <QFileInfo>
#include <QFile>
#include <QSet>
#include <QPair>
#include <QVector2D>
#include <QPointer>
#include "mGlobalVarible.h"
#include <QTextCodec>
#include <set>
#include <QMutexLocker>

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
#include "mPartDataStruct.h"
#include "mParseMeshData.h"
//MechCommon
#include "mGlobalSignals.h"
#include "mOutputMessage.h"
#include "mWidgetOperation.h"
#include "mLanguageVarible.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include "mModelInOutSteam.h"

//PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"

using namespace MxFunctions;
using namespace MechPreData;
using namespace MechCommon;
using namespace MechPreData;
namespace MechIOFile
{
    mIMeshIOFile::mIMeshIOFile(const QString& name) :_filename(name)
	{
		_isSuccessRead = true;
	}

    mIMeshIOFile::~mIMeshIOFile()
	{
	}

	QString mIMeshIOFile::getFileName()
	{
		return _filename;
	}
    QString mIMeshIOFile::getFileType()
    {
        return _fileType;
    }

	bool mIMeshIOFile::isSuccessReadFile()
	{
		return _isSuccessRead;
	}

    QVector<meshReadPart> mIMeshIOFile::getMeshParts() {
        return _parts;
    }
    bool mIMeshIOFile::praseParts() {
        MechModelData::mModelData* modeldata = getCVM->getModelData();
        if (modeldata == nullptr)
        {
            return false;
        }
        mMeshData* meshData = modeldata->getMeshData();
        if (meshData == nullptr)
        {
            return false;
        }

        int lastMaxNodeID = meshData->getMaxNodeId();
        int lastMaxEleID = meshData->getMaxEleId();
        int curnodenum = meshData->_nodedata.nodeamount;

        //扩容全局节点池（方式与scanMxFile一致）
        int totalnodes = 0;
        for (auto& part : _parts)
        {
            totalnodes += part.nodes.size();
        }
        int newamount = curnodenum + totalnodes;
        double* xyz = (double*)malloc(newamount * 3 * sizeof(double));
        int* realids = (int*)malloc(newamount * sizeof(int));
        if (curnodenum > 0)
        {
            memcpy(xyz, meshData->_nodedata.xyz, curnodenum * 3 * sizeof(double));
            memcpy(realids, meshData->_nodedata.realids, curnodenum * sizeof(int));
            free(meshData->_nodedata.xyz);
            free(meshData->_nodedata.realids);
        }
        meshData->_nodedata.xyz = xyz;
        meshData->_nodedata.realids = realids;
        meshData->_nodedata.nodeamount = newamount;

        //文件内节点id → 全局真实id（OBJ多part共享顶点池，按文件id去重合并）
        QHash<int, int> nodeIdMap;
        for (auto& part : _parts)
        {
            for (auto& node : part.nodes)
            {
                if (nodeIdMap.contains(node.id))
                {
                    continue;
                }
                lastMaxNodeID++;
                nodeIdMap.insert(node.id, lastMaxNodeID);
                meshData->_nodedata.xyz[curnodenum * 3] = node.positions.x();
                meshData->_nodedata.xyz[curnodenum * 3 + 1] = node.positions.y();
                meshData->_nodedata.xyz[curnodenum * 3 + 2] = node.positions.z();
                meshData->_nodedata.realids[curnodenum] = lastMaxNodeID;
                meshData->_nodedata.nodeidmap.insert(lastMaxNodeID, curnodenum);
                curnodenum++;
            }
        }

        bool hasPart = false;
        for (auto& part : _parts)
        {
            //剔除节点缺失或节点数与类型不符的非法单元（保证分组布局不出现空洞）
            QVector<meshReadElement> eles;
            for (auto& ele : part.elements)
            {
                ElementType etenum = mElementFunction::getDefaultElementType(ImplicitAnalysis, ele.type);
                bool valid = (mElementFunction::elementNodeCount(etenum) == ele.nodeIDs.size());
                if (valid)
                {
                    for (int nid : ele.nodeIDs)
                    {
                        if (!nodeIdMap.contains(nid))
                        {
                            valid = false;
                            break;
                        }
                    }
                }
                if (valid)
                {
                    eles.push_back(ele);
                }
            }
            if (eles.isEmpty())
            {
                continue;
            }

            //部件重名规避
            QString pname = part.name;
            while (meshData->getPartData(pname) != nullptr)
            {
                pname += "_1";
            }

            //按类型分组统计（保持首次出现顺序；渲染按elementtype顺序读取分组区间）
            QVector<MeshType> types;
            QHash<int, int> typeIndex;
            QHash<int, int> typeCount;
            for (auto& ele : eles)
            {
                if (!typeIndex.contains((int)ele.type))
                {
                    typeIndex.insert((int)ele.type, types.size());
                    types.push_back(ele.type);
                    typeCount.insert((int)ele.type, 0);
                }
                typeCount[(int)ele.type]++;
            }

            int pid = meshData->getMaxPartID() + 1;
            mPartData* pd = new mPartData(pname, pid);
            meshData->_partdata.insert(pid, pd);

            //类型汇总信息，并计算各类型的单元起始连续ID与nodeids起始位置
            int& etn = pd->_parteledata.elementtypenum;
            int* et = pd->_parteledata.elementtype;
            int* enc = pd->_parteledata.elementnodecount;
            QVector<int> typeEleStart(types.size());
            QVector<int> typeNodeCount(types.size());
            QVector<int> typeNodeStart(types.size());
            int eleStart = 0;
            int nodeStart = 0;
            for (int i = 0; i < types.size(); ++i)
            {
                ElementType etenum = mElementFunction::getDefaultElementType(ImplicitAnalysis, types[i]);
                int encount = mElementFunction::elementNodeCount(etenum);
                et[etn] = (int)etenum;
                enc[etn * 2] = encount;
                enc[etn * 2 + 1] = typeCount[(int)types[i]];
                etn++;

                typeEleStart[i] = eleStart;
                typeNodeCount[i] = encount;
                typeNodeStart[i] = nodeStart;
                eleStart += typeCount[(int)types[i]];
                nodeStart += encount * typeCount[(int)types[i]];
            }

            pd->_parteledata.nodeids = (int*)malloc(nodeStart * sizeof(int));
            pd->_parteledata.realids = (int*)malloc(eles.size() * sizeof(int));

            //按类型分组填充
            QVector<int> eleCursor(types.size(), 0);
            QVector<int> nodeCursor = typeNodeStart;
            for (auto& ele : eles)
            {
                int i = typeIndex[(int)ele.type];
                int consec = typeEleStart[i] + eleCursor[i];
                int pos = nodeCursor[i];

                lastMaxEleID++;
                int eid = lastMaxEleID;
                pd->_parteledata.elementidmap.insert(eid, consec);
                pd->_parteledata.realids[consec] = eid;
                pd->_parteledata.elenodeposmap.insert(eid, QPair<int, int>(pos, ele.nodeIDs.size()));
                for (int nid : ele.nodeIDs)
                {
                    int gid = nodeIdMap.value(nid);
                    pd->_parteledata.nodeids[pos] = gid;
                    pos++;
                    meshData->addNodeFindElementData(pd, gid, eid);
                }
                eleCursor[i]++;
                nodeCursor[i] = pos;
            }
            pd->_parteledata.currentelenum = eles.size();
            pd->_parteledata._ele_node_count = nodeStart;
            hasPart = true;
        }
        //提取渲染所需的边界面/边数据（与.mx流程一致，ParseMesh会对全部部件重算）
        mParseMeshData* pmd = new mParseMeshData(meshData);
        pmd->ParseMesh();
        delete pmd;
        return hasPart;
    }

	void mIMeshIOFile::run()
	{
		mGlobalSignals::getInstance()->outputMessageSig(QString("开始读取文件： %1").arg(_filename));
		QFileInfo fileInfo(_filename);
        if(!fileInfo.exists()){
			emit readMxcaeErrorSig(FileNotExist);
            _isSuccessRead = false;
			return;
		}
        QString fsuffix = fileInfo.suffix().toLower();
        this->_fileType = fsuffix;
        this->_parts.clear();
        if (fsuffix == QString("obj")) {
            meshReadOBJ(_filename, _parts);
        }
        else if (fsuffix == QString("ply")) {
            meshReadPLY(_filename, _parts);
        }
        else if (fsuffix == QString("off")) {
            meshReadOFF(_filename, _parts);
        }
        else if (fsuffix == QString("stl")) {
            meshReadSTL(_filename, _parts);
        }
        else {
            emit readMxcaeErrorSig(FileNotOpen);
            _isSuccessRead = false;
            return;
        }
        if (_parts.isEmpty())
        {
            emit readMxcaeErrorSig(FileNotOpen);
            _isSuccessRead = false;
            return;
        }

        //将面片写入中央网格数据（与.mx一致：IO线程写，UI经完成信号后再读）
        if (!praseParts())
        {
            emit readMxcaeErrorSig(FileNotOpen);
            _isSuccessRead = false;
            return;
        }
        _isSuccessRead = true;
        emit MxFunctions::mGlobalSignals::getInstance()->outputStatusBarSig("成功导入文件");
        emit readMeshSuccessSig(_filename);

	}

}
