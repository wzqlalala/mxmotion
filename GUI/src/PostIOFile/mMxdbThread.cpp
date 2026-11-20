#include "mMxdbThread.h"

#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mAppConfiguration.h"
#include "mPostParseMesh.h"
#include "mPostModelData.h"
#include "mPostMeshSignature.h"
#include "mModelData.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGlobalSignals.h"

#include "MxdbReaderCore/NewSingleMxdbReader.h"
#include "MxdbReader/MxdbDataBuilder.h"
#include "LowCPlusDefine.h"

#include <QFileInfo>
#include <QDebug>
#include <QVector3D>
#include <QFile>
#include <QApplication>
#include <QDateTime>
#include <QMatrix4x4>
#include <QSet>
#include <climits>
#include <cfloat>
#include "mOtherFunction.h"
//#define TESTCODE

using namespace PostData;
namespace MIOFile
{
	using namespace MxdbReader;

	// 全局按文件路径缓存 varByte -> MxdbVariable，避免按需读取时每个新 mMxdbThread
	// 实例都重新 readInitialFrame() 重建该映射。导入时填充一次，后续共享。
	static QHash<QString, QHash<long long, MxdbReader::MxdbVariable>> g_varByteCacheByFile;

	// 与 mIMxdbFile1.cpp:162 相同的静态文案翻译函数（内部链接，同名不冲突）
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


	//迁移辅助：calcu* 统一返回 double 容器后，float 输出容器在此转换
	static QHash<int, float> toFloatHashF(const QHash<int, double>& src)
	{
		QHash<int, float> dst;
		dst.reserve(src.size());
		for (auto it = src.constBegin(); it != src.constEnd(); ++it)
			dst.insert(it.key(), static_cast<float>(it.value()));
		return dst;
	}

	static QHash<int, QHash<int, float>> toFloatHashFF(const QHash<int, QHash<int, double>>& src)
	{
		QHash<int, QHash<int, float>> dst;
		dst.reserve(src.size());
		for (auto it = src.constBegin(); it != src.constEnd(); ++it)
			dst.insert(it.key(), toFloatHashF(it.value()));
		return dst;
	}

	// 通用矢量变量 Mag 推断：包含 X/Y/Z 三矢量分量且不含 vonMises 等量时应启用 Mag
	static bool inferMagFromComponents(const QString& varName, const QStringList& compoNames)
	{
		if (compoNames.size() < 2)
			return false;
		static const QSet<QString> vectorCompos = { QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z") };
		static const QSet<QString> scalarMarkers = { QStringLiteral("vonMises"), QStringLiteral("mises"),
			QStringLiteral("Tresca"), QStringLiteral("tresca"), QStringLiteral("MaxPrincipal"),
			QStringLiteral("MidPrincipal"), QStringLiteral("MinPrincipal"), QStringLiteral("MaxShear"),
			QStringLiteral("Intensity"), QStringLiteral("Equivalent") };
		for (const QString& marker : scalarMarkers)
		{
			if (varName.contains(marker, Qt::CaseInsensitive))
				return false;
		}
		int vectorHits = 0;
		for (const QString& c : compoNames)
		{
			if (vectorCompos.contains(c.trimmed()))
				++vectorHits;
		}
		return vectorHits >= 3;
	}

	mMxdbThread::mMxdbThread(const QString& inp_file, QPointer<mPostDataBase> modeldb, QObject* parent)
		: QThread(parent)
	{
		_inpFlag = false;

		mPathName = inp_file;
		QFileInfo file(mPathName);
		mModelName = file.baseName();
        _fileSize = file.size();
		_modeldb = modeldb;
		connect(this, SIGNAL(finished()), this, SLOT(slot_readVarValue()));

	}

	mMxdbThread::~mMxdbThread()
	{
	}

	bool mMxdbThread::isReadSuccess()
	{
		return _inpFlag;
	}

	QString mMxdbThread::getPathName()
	{
		return mPathName;
	}

	QString mMxdbThread::getModelName()
	{
		return mModelName;
	}

	bool mMxdbThread::readModelAndFrameInfo_TestData()
	{
		QPointer<mPostModelData> modelData = new mPostModelData();
        int frameCount_index = 0;

        PostFrameInfo frameInfo;
        frameInfo._frameCase = QString("grav");
        frameInfo._frameFile = mPathName;
        frameInfo._frameNum = frameCount_index + 1;
        frameInfo.type = 0;
        frameInfo._frameTime = 1.f;

        QString partName = "1111";
        int partid = 0;
        QPointer<mPostPartData> partData = new mPostPartData(partid);
        partData->setPartName(partName);


        QHash<int, double> Node_Cid_Value;

        // 1. 构造节点数据 (共需22个节点)
        postnodedata& _nodedata = modelData->_nodedata;
        postelementdata& _parteledata = partData->_parteledata;


        // ============ 可配置测试区域 ============
        // 固定单元类型开关（设为true启用，false禁用）
        bool testTetra = true;   // 四面体 (4节点)
        bool testHexa = true;   // 六面体 (8节点)  
        bool testQuad = true;   // 四边形/壳单元 (4节点)

        // 多面体棱柱配置（注释掉不需要的边数）
        std::vector<int> testPolygons = {
            5,    // 五棱柱 (10节点, 7面)
            6,    // 六棱柱 (12节点, 8面)
            7,    // 七棱柱 (14节点, 9面)
            8,    // 八棱柱 (16节点, 10面)
            9,    // 九棱柱 (18节点, 11面)
            10    // 十棱柱 (20节点, 12面)
        };

        // 布局参数
        double fixedSpacing = 2.0;   // 固定单元之间的间距
        double polySpacing = 3.0;   // 多面体之间的间距
        double polyRadius = 1.0;   // 多边形底面半径
        double polyHeight = 2.0;   // 棱柱高度
        // =======================================

        // 1. 计算总节点数
        int fixedNodes = 0;
        if (testTetra) fixedNodes += 4;    // 0-3
        if (testHexa)  fixedNodes += 8;      // 4-11
        if (testQuad)  fixedNodes += 4;    // 12-15

        int polyNodes = 0;
        for (int n : testPolygons) {
            polyNodes += n * 2;  // 每个n边形棱柱2n个节点
        }

        int totalNodes = fixedNodes + polyNodes;
        int totalCells = (testTetra ? 1 : 0) + (testHexa ? 1 : 0) + (testQuad ? 1 : 0) + testPolygons.size();

        // 2. 初始化节点数据
        _nodedata.isDle = true;
        _nodedata.nodeamount = totalNodes;
        _nodedata.ids = new int[totalNodes];
        _nodedata.nodeposD = new double[totalNodes * 3];

        for (int i = 0; i < totalNodes; i++) {
            _nodedata.ids[i] = 1000 + i;
            _nodedata.nodeidmap[1000 + i] = i;
        }

        // 3. 生成固定单元节点坐标
        int nodeIdx = 0;

        // 四面体：位于原点，底面在xy平面，顶点在z方向
        if (testTetra) {
            _nodedata.nodeposD[nodeIdx * 3 + 0] = 0.0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 0.0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0.0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 0
            _nodedata.nodeposD[nodeIdx * 3 + 0] = 1.0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 0.0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0.0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 1
            _nodedata.nodeposD[nodeIdx * 3 + 0] = 0.0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 1.0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0.0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 2
            _nodedata.nodeposD[nodeIdx * 3 + 0] = 0.0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 0.0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 1.0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 3
            Node_Cid_Value.insert(nodeIdx, 4);
        }

        // 六面体：位于x=fixedSpacing，1x1x1立方体
        if (testHexa) {
            double x = fixedSpacing;
            _nodedata.nodeposD[nodeIdx * 3 + 0] = x + 0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 4
            _nodedata.nodeposD[nodeIdx * 3 + 0] = x + 1;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 5
            _nodedata.nodeposD[nodeIdx * 3 + 0] = x + 1;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 1;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 6
            _nodedata.nodeposD[nodeIdx * 3 + 0] = x + 0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 1;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 7
            _nodedata.nodeposD[nodeIdx * 3 + 0] = x + 0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 1; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 8
            _nodedata.nodeposD[nodeIdx * 3 + 0] = x + 1;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 1; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 9
            _nodedata.nodeposD[nodeIdx * 3 + 0] = x + 1;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 1;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 1; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 10
            _nodedata.nodeposD[nodeIdx * 3 + 0] = x + 0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = 1;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 1; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 11
        }

        // 四边形：位于y=fixedSpacing*2，xy平面上的正方形（作为壳单元）
        if (testQuad) {
            double y = fixedSpacing * 2;
            _nodedata.nodeposD[nodeIdx * 3 + 0] = 0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = y + 0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 12
            _nodedata.nodeposD[nodeIdx * 3 + 0] = 1;  _nodedata.nodeposD[nodeIdx * 3 + 1] = y + 0;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 13
            _nodedata.nodeposD[nodeIdx * 3 + 0] = 1;  _nodedata.nodeposD[nodeIdx * 3 + 1] = y + 1;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 14
            _nodedata.nodeposD[nodeIdx * 3 + 0] = 0;  _nodedata.nodeposD[nodeIdx * 3 + 1] = y + 1;  _nodedata.nodeposD[nodeIdx * 3 + 2] = 0; Node_Cid_Value.insert(nodeIdx, nodeIdx); nodeIdx++;  // 15
        }

        int value_index = nodeIdx;
        // 多面体棱柱：沿x方向排列，从x=fixedSpacing*3开始
        double currentX = fixedSpacing * 3;
        for (int n : testPolygons) {
            // 底面 (z=0)
            for (int i = 0; i < n; i++) {
                double angle = 2.0 * 3.1415926 * i / n;
                _nodedata.nodeposD[nodeIdx * 3 + 0] = currentX + polyRadius * cos(angle);
                _nodedata.nodeposD[nodeIdx * 3 + 1] = polyRadius * sin(angle);
                _nodedata.nodeposD[nodeIdx * 3 + 2] = 0.0;
                Node_Cid_Value.insert(nodeIdx, 0);
                nodeIdx++;
            }
            // 顶面 (z=polyHeight)
            for (int i = 0; i < n; i++) {
                double angle = 2.0 * 3.1415926 * i / n;
                _nodedata.nodeposD[nodeIdx * 3 + 0] = currentX + polyRadius * cos(angle);
                _nodedata.nodeposD[nodeIdx * 3 + 1] = polyRadius * sin(angle);
                _nodedata.nodeposD[nodeIdx * 3 + 2] = polyHeight;
                Node_Cid_Value.insert(nodeIdx, value_index);
                nodeIdx++;
            }
            currentX += polySpacing;
        }

        // 4. 构造单元数据
        _parteledata.cell_types.resize(totalCells);
        _parteledata.cell_rid.resize(totalCells);
        _parteledata.cell_node_counts.resize(totalCells);
        _parteledata.cell_node_offsets.resize(totalCells);
        _parteledata.cell_info.resize(totalCells);
        _parteledata.cell_node_ids.resize(totalNodes);

        int offset = 0;
        int cellId = 0;
        nodeIdx = 0;  // 重置用于节点连接

        // ---- Cell 0: 四面体 ----
        if (testTetra) {
            _parteledata.cell_types[cellId] = PostCelllType::MX_TETRA;
            _parteledata.cell_rid[cellId] = 2001;
            _parteledata.cell_rid_to_cid[2001] = cellId;
            _parteledata.cell_node_counts[cellId] = 4;
            _parteledata.cell_node_offsets[cellId] = offset;
            _parteledata.cell_info[cellId].type = ElementType::D3T4;
            _parteledata.cell_info[cellId].is_show = true;
            _parteledata.cell_info[cellId].is_select = true;
            _parteledata.cell_info[cellId].property = nullptr;

            for (int i = 0; i < 4; i++) _parteledata.cell_node_ids[offset + i] = nodeIdx++;
            offset += 4;
            cellId++;
        }

        // ---- Cell 1: 六面体 ----
        if (testHexa) {
            _parteledata.cell_types[cellId] = PostCelllType::MX_HEXAHEDRON;
            _parteledata.cell_rid[cellId] = 2002;
            _parteledata.cell_rid_to_cid[2002] = cellId;
            _parteledata.cell_node_counts[cellId] = 8;
            _parteledata.cell_node_offsets[cellId] = offset;
            _parteledata.cell_info[cellId].type = ElementType::D3H8;
            _parteledata.cell_info[cellId].is_show = true;
            _parteledata.cell_info[cellId].is_select = true;
            _parteledata.cell_info[cellId].property = nullptr;

            for (int i = 0; i < 8; i++) _parteledata.cell_node_ids[offset + i] = nodeIdx++;
            offset += 8;
            cellId++;
        }

        // ---- Cell 2: 四边形 ----
        if (testQuad) {
            _parteledata.cell_types[cellId] = PostCelllType::MX_QUAD;
            _parteledata.cell_rid[cellId] = 2003;
            _parteledata.cell_rid_to_cid[2003] = cellId;
            _parteledata.cell_node_counts[cellId] = 4;
            _parteledata.cell_node_offsets[cellId] = offset;
            _parteledata.cell_info[cellId].type = ElementType::Q4;
            _parteledata.cell_info[cellId].is_show = true;
            _parteledata.cell_info[cellId].is_select = true;

            // 壳单元属性
            EleProperty* prop = new EleProperty();
            prop->flag = 1;  // 壳单元
            prop->valCount = 1;
            prop->thicknessVals = new float[1] {0.5f};
            _parteledata.cell_info[cellId].property = prop;

            for (int i = 0; i < 4; i++) _parteledata.cell_node_ids[offset + i] = nodeIdx++;
            offset += 4;
            cellId++;
        }

        // ---- 多面体棱柱单元 ----
        for (int n : testPolygons) {
            int nNodes = n * 2;
            int nFaces = 2 + n;

            _parteledata.cell_types[cellId] = PostCelllType::MX_POLYHEDRON;
            _parteledata.cell_rid[cellId] = 3000 + cellId;
            _parteledata.cell_rid_to_cid[3000 + cellId] = cellId;
            _parteledata.cell_node_counts[cellId] = nNodes;
            _parteledata.cell_node_offsets[cellId] = offset;
            _parteledata.cell_info[cellId].type = ElementType::NoneElement;
            _parteledata.cell_info[cellId].is_show = true;
            _parteledata.cell_info[cellId].is_select = true;
            _parteledata.cell_info[cellId].property = nullptr;

            // 节点连接
            for (int i = 0; i < nNodes; i++) {
                _parteledata.cell_node_ids[offset + i] = nodeIdx + i;
            }

            // 多面体面数据
            _parteledata.poly_cell_indices.push_back(cellId);
            _parteledata.poly_face_counts.push_back(nFaces);
            _parteledata.poly_face_offsets.push_back(_parteledata.poly_face_node_counts.size());
            _parteledata.poly_face_node_offsets.push_back(_parteledata.poly_face_node_ids.size());

            // 底面n边形
            _parteledata.poly_face_node_counts.push_back(n);
            for (int i = 0; i < n; i++) _parteledata.poly_face_node_ids.push_back(nodeIdx + i);

            // 顶面n边形
            _parteledata.poly_face_node_counts.push_back(n);
            for (int i = 0; i < n; i++) _parteledata.poly_face_node_ids.push_back(nodeIdx + n + i);

            // n个侧面四边形
            for (int i = 0; i < n; i++) {
                _parteledata.poly_face_node_counts.push_back(4);
                int next = (i + 1) % n;
                _parteledata.poly_face_node_ids.push_back(nodeIdx + i);        // 底面当前
                _parteledata.poly_face_node_ids.push_back(nodeIdx + next);      // 底面下一个
                _parteledata.poly_face_node_ids.push_back(nodeIdx + n + next);  // 顶面下一个
                _parteledata.poly_face_node_ids.push_back(nodeIdx + n + i);     // 顶面当前
            }

            offset += nNodes;
            nodeIdx += nNodes;
            cellId++;
        }

        // 5. 统计节点-单元关联
        for (int cid = 0; cid < totalCells; cid++) {
            int off = _parteledata.cell_node_offsets[cid];
            int cnt = _parteledata.cell_node_counts[cid];
            for (int i = 0; i < cnt; i++) {
                int idx = _parteledata.cell_node_ids[off + i];
                _nodedata.nodeEleCountmap[idx]++;
            }
        }

        // ============ 使用示例 ============
        // 只测固定单元：testTetra=true, testHexa=true, testQuad=true, testPolygons={}
        // 只测五棱柱：testTetra=false, testHexa=false, testQuad=false, testPolygons={5}
        // 测全部：全部设为true，testPolygons={5,6,7,8,9,10}
        // 测四面体+五棱柱+十棱柱：testTetra=true, testHexa=false, testQuad=false, testPolygons={5,10}

        modelData->_partdataForFind.insert(partData->getPartID(), partData);
        modelData->_partdata.push_back(partData);
        _modeldb->appendModelData(frameCount_index + 1, modelData);
        _modeldb->setFile(mPathName);
        modelData->_frameInfo = frameInfo;
        _modeldb->setDisplacementDataByFrameID(frameCount_index + 1, {});
        _modeldb->distributeFrameIdAndResult(frameCount_index + 1, Node_Cid_Value);
        _modeldb->distributeFrameIdAndResultType(frameCount_index + 1, 0);
        //获取所有帧数据指针
        QList<PostFrameInfo*> allFramept = _modeldb->getAllFrameInfoPointer();
        //根据帧号顺序设置不同工况的帧名称
        _modeldb->setCaseFrameName(allFramept);

		mPostParseMesh* parseMesh = new mPostParseMesh(this);
		//存储部件面的信息
		QHash<int, QPointer<mPostModelData>> models = _modeldb->getAllFrameData();
        mGlobalSignals::getInstance()->setProgressBarValue(93);
		auto it = models.begin();
		while (it != models.end())
		{
			parseMesh->ParseMesh(*it);
			it++;
		}
        mGlobalSignals::getInstance()->setProgressBarValue(94);
		delete parseMesh;
		parseMesh = nullptr;

		return true;
	}

    bool mMxdbThread::readModelAndFrameInfo()
    {
        // 新版记录式格式（601/602）open() 需全文件扫描建帧索引（记录无长度字段、
        // 单元变量值区逐单元推进，大文件耗时较长），先读文件头给一条提示，
        // 避免"正在读取模型数据"出现前界面长时间静默；仅在导入链路发一次，
        // 按需读取的重复 open 不提示
        {
            unsigned int headVersion = 0;
            std::ifstream headStream;
#ifdef _WIN32
            headStream.open(mPathName.trimmed().toStdWString(), std::ios::in | std::ios::binary);
#else
            headStream.open(mPathName.trimmed().toLocal8Bit().constData(), std::ios::in | std::ios::binary);
#endif
            if (headStream)
                headStream.read(reinterpret_cast<char*>(&headVersion), sizeof(headVersion));
            if (headVersion == 0xA1B1C601 || headVersion == 0xA1B1C602)
            {
                //仅大文件提示（小文件扫描毫秒级完成，提示反而是噪音）
                if (_notifyScanHint && QFileInfo(mPathName).size() > 50LL * 1024 * 1024)
                {
                    const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
                    mGlobalSignals::getInstance()->outputMessageSig(0,
                        isChinese ? QStringLiteral("正在扫描新版格式文件建立帧索引（仅首次，文件较大时约需半分钟），请稍候…")
                                  : QStringLiteral("Scanning new-format file to build frame index (first open only; may take up to half a minute for large files), please wait..."),
                        true);
                }
            }
        }
        if (!openFileStatus(mPathName))
            return false;

        // 601/602（新版记录式格式）走通用基类接口；201/202 需要 legacy 专有接口（装配树等）
        auto* newFormatReader = isNewFormatFile()
            ? dynamic_cast<MxdbReader::NewFormatMxdbReader*>(_reader.get()) : nullptr;
        auto* legacyReader = newFormatReader != nullptr
            ? nullptr : dynamic_cast<NewSingleMxdbReader*>(_reader.get());
        if (newFormatReader == nullptr && legacyReader == nullptr)
            return false;
        MxdbReader::MxdbReaderBase* reader = _reader.get();

        // 装配信息（仅 201/202；新格式无装配记录，reader 在 open 时已解析）
        if (legacyReader != nullptr)
        {
            QList<QPair<QString, int>> assemblyInfos;
            QStringList partNames;
            for (const auto& node : legacyReader->assemblyTree())
                assemblyInfos.append({ QString::fromStdString(node.first), node.second });
            for (const auto& n : legacyReader->assemblyPartNames())
                partNames.append(QString::fromStdString(n));
            _modeldb->setAssemblyInfos(assemblyInfos, partNames, legacyReader->assemblyRootCount());
        }

        MxdbDataBuilder builder;
        const int totalFrames = newFormatReader != nullptr
            ? static_cast<int>(newFormatReader->frameCount())
            : static_cast<int>(legacyReader->frameCount());

        // 第 0 帧：初始帧（模型 + 变量信息）
        {
            MxdbReader::MxdbInitialFrame initFrame;
            try {
                initFrame = reader->readInitialFrame();
            } catch (const std::exception& e) {
                // 损坏文件可抛 bad_alloc 等（垃圾数量字段触发超大 resize）
                qWarning() << "mMxdbThread: legacy initial frame read exception:" << e.what();
                mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("初始帧读取异常（%1），文件可能已损坏！").arg(e.what()), true);
                return false;
            }
            if (!builder.buildInitialFrame(initFrame, _modeldb))
                return false;
            // 新格式初始帧无变量（readVariableInfos 返回空）；201/202 初始帧可带变量
            convertFrameVariables(legacyReader != nullptr
                ? legacyReader->currentFrameVariables() : reader->readVariableInfos(), 1);
            // 201/202 的 Displacement 是普通变量，import 时预读到 DisplacementValuesR
            // （渲染 proxy 的颜色表 max/min 依赖它；buildResultFrame 对 201/202 不填）。
            // 新格式 readResultFrame 已自动带 displacementValues（buildResultFrame 落库），无需预读
            if (legacyReader != nullptr)
                fillLegacyDisplacement(legacyReader, 1);
            mGlobalSignals::getInstance()->setProgressBarValue(0);
        }

        // 结果帧：1 .. totalFrames-1（初始帧占 [1]，结果帧从 [2] 开始，与旧版一致）
        for (int fi = 1; fi < totalFrames; ++fi)
        {
            MxdbReader::MxdbResultFrame frame;
            try {
                frame = reader->readResultFrame();
            } catch (const std::exception& e) {
                // 损坏文件可抛 bad_alloc 等；跳过该帧而非让线程崩溃
                qWarning() << "mMxdbThread: legacy result frame" << (fi + 1) << "read exception:" << e.what();
                mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("结果帧%1读取异常（%2），该帧已跳过，文件可能已损坏！").arg(fi + 1).arg(e.what()), true);
                continue;
            }
            if (!builder.buildResultFrame(frame, _modeldb, fi + 1))
            {
                // S6 帧装载回归诊断（2026-11-09）：此前失败仅 qWarning+continue，
                // 导入整体仍报成功——结果帧静默丢失后，切帧 seekFrame 无数据可绑，
                // GUI 停在"帧 N 装载中"死等 frameReady。这里显式透出到输出窗口，
                // 让"导入期丢帧"可被用户/诊断直接观测（帧号与原因见调试日志）
                mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("结果帧%1构建失败，该帧已跳过（详见调试日志 buildResultFrame 告警）").arg(fi + 1), true);
                continue;
            }

            QPointer<mPostModelData> modelData = _modeldb->getModelData(fi + 1);
            PostFrameInfo frameInfo = modelData->_frameInfo;
            frameInfo._frameFile = mPathName;
            // 帧名：201/202 与 mIMxdbFile1 规则一致（文件名后缀或帧号）；
            // 新格式（601/602）单文件多帧，文件名尾部（如 _ST2）是模型名不是帧号——
            // 所有帧会取到同名帧名导致帧下拉排序/默认帧错乱（t=0 帧被排到末尾），
            // 统一用全局帧号命名
            if (isNewFormatFile())
            {
                // 帧名对齐文件帧号（结果帧 1..N，初始帧不进帧表）
                frameInfo._frameName = MxFunctions::mNumberToString(fi);
            }
            else
            {
                QStringList lis = mPathName.split("/");
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

            // 引用帧（sameFrame=0/n）的拓扑与被引用帧完全一致（cloneModelData 克隆而来），
            // 统一记 0/n，不在导入期单独做网格面解析；显示期经跨帧面缓存复用
            _flag.insert(fi + 1, frame.sameFrame);

            convertFrameVariables(frame.variables, fi + 1);
            if (legacyReader != nullptr)
                fillLegacyDisplacement(legacyReader, fi + 1);
            mGlobalSignals::getInstance()->setProgressBarValue(totalFrames > 0 ? 90 * fi / totalFrames : 0);
        }

        // 网格面解析（仅对新模型帧；初始帧不在 _flag 中，默认 -1 同样解析）。
        // 逐部件 buildUniqueFaceTopology + classifyAndWriteFaces（等价 ParseMesh 内部
        // 的 buildMeshFace），同时把拓扑与签名注册进 mPostDataBase 跨帧面缓存，
        // 显示期 rebuildFacesForModelDataWithCache 对引用帧直接命中，无需重建。
        mGlobalSignals::getInstance()->setProgressBarValue(92);
        mPostParseMesh* parseMesh = new mPostParseMesh(this);
        QHash<int, QPointer<mPostModelData>> models = _modeldb->getAllFrameData();
        auto it = models.begin();
        while (it != models.end())
        {
            int flag = _flag.value(it.key(), -1);
            if (flag != -1) { ++it; continue; }

            QPointer<mPostModelData> modelData = *it;
            if (modelData != nullptr)
            {
                std::shared_ptr<mPostModelData::MeshFaceCache> cache =
                    std::make_shared<mPostModelData::MeshFaceCache>();
                for (auto part : modelData->_partdata)
                {
                    if (part == nullptr)
                    {
                        cache->partTopologies.emplace_back();
                        continue;
                    }
                    UniqueFaceTopology topology;
                    if (mPostParseMesh::buildUniqueFaceTopology(part, topology))
                        mPostParseMesh::classifyAndWriteFaces(part, topology);
                    cache->partTopologies.push_back(std::move(topology));
                    parseMesh->findPartNodeSet(part);
                }
                cache->signature = mPostMeshSignature::computeFullSignature(modelData);
                cache->failureSignature = mPostMeshSignature::computeFailureSignature(modelData);
                _modeldb->setMeshFaceCacheForFrame(it.key(), cache);
            }
            ++it;
        }
        mGlobalSignals::getInstance()->setProgressBarValue(93);
        delete parseMesh;
        return true;
    }

	bool mMxdbThread::readVarDatas(const QString& filePath, int f, const QString& varName, const QString& compoName, const QString& layer, const QString& complex, Compones& componeData, QVector<double> local)
	{
		bool isSuccess = openFileStatus(filePath);
		if (!isSuccess)
			return false;

        bool isD = DataPrecisionIsDouble();

		//拿到第f帧的帧信息
		QFileInfo file(filePath);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(filePath);
		if (outPutData == nullptr)
			return false;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		QPointer<mPostModelData> modelData = modelDatabase.value(f);
		if (modelData == nullptr && f == 0)
			modelData = modelDatabase.value(1); // 初始帧注册在 [1]
		if (modelData == nullptr)
			return false;
		PostFrameInfo frameInfo = modelData->_frameInfo;

		//拿到变量信息
		PostVariableInfo curVarInfo;
		QList<PostVariableInfo> varInfos = frameInfo._frameVars;
		auto it = varInfos.begin();
		unsigned long long varPos = 0;
		while (it != varInfos.end())
		{
			PostVariableInfo varInfo = *it;
			if (varInfo._varName == varName)
			{
				curVarInfo = varInfo;
				break;
			}
			it++;
		}
		if (it == varInfos.end())
		{
			//该帧不存在此变量（如SolidColor伪变量），直接返回，避免用无效的_varByte偏移解析文件导致越界
			return false;
		}
		int componeIndex = curVarInfo._compoNames.indexOf(compoName);
		componeData.isD = true; // readLegacyVarValues 已将 float 提升为 double，value1 恒为 double 容器
		componeData.frameID = f;

		// 新格式（601/602）：统一按分量名/截面点层/复数方式取标量（值布局由 reader 返回结构携带）
		if (isNewFormatFile())
		{
			QHash<int, double> vals;
			int valueType = -1;
			if (!readNewFormatScalarValues(curVarInfo, compoName, layer, complex, vals, valueType, modelData))
				return false;
			componeData.isD = true;
			componeData.frameID = f;
			componeData.flag = valueType; // 0 节点 / 1 单元
			componeData.isNodeFace = false;
			componeData.value1 = vals;    // addResultValue 合并 value1/value2，实体/壳槽无区分语义
			return true;
		}
		std::vector<int> lbIds1, lbIds2;
		std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
		int varType = -1; // 0-常规变量, 1-复数变量（由 readLegacyVarHead 读出）
		std::string lbVarName;
		// 预读：块头（varType + varName），替代旧代码的 _readf.read(varType) + _readf.read(varName)
		if (!readLegacyVarHead(curVarInfo._varByte, varType, lbVarName))
			return false;
		// readLegacyVarValues 已将 float 提升为 double，下游统一走 double 路径
		isD = true;
		if (varType == 0 || varType == 1)
		{
			const char* varName = lbVarName.c_str();


			if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
				|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
				|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
			{
				// 预读：实体块 + 壳块全部值（不 seekg，从块头之后继续读）
				if (!readLegacyVarValues(20, 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				componeData.isNodeFace = false;
				componeData.flag = 1;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count1 = 20 * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds1.data();
					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						valRD = calcuComponeValues(valRealD1, nums1, componeName, ids, componeIndex, local);

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							valVD = calcuComponeValues(valVirD1, nums1, componeName, ids, componeIndex, local);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								componeData.value1 = valRD;
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								componeData.value1 = valVD;
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									int id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);
									componeData.value1.insert(id, sqrt(v0 * v0 + v1 * v1));
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									int id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);

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
									componeData.value1.insert(id, phase_deg);
								}
							}

					}
					else
					{
						componeData.value1 = valRD;

					}

				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
				int count2 = 16 * nums2;
				if (nums2 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds2.data();
					//实部
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
						valRD = calcuComponeValues1(valRealD2, nums2, componeName, layer, ids, componeIndex);


					//虚部
					if (varType == 1)
					{
						double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							valVD = calcuComponeValues1(valVirD2, nums2, componeName, layer, ids, componeIndex);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								componeData.value2 = valRD;
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								componeData.value2 = valVD;
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									int id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);
									componeData.value2.insert(id, sqrt(v0 * v0 + v1 * v1));
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									int id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);

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
									componeData.value2.insert(id, phase_deg);
								}
							}

					}
					else
					{
						componeData.value2 = valRD;

					}
				}
			}
			else if (QString(varName) == "Stress(N)")
			{
				// 预读：实体块头（nums1/ids1），值在两段式读取
				if (!readLegacyVarValues(0, 0, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				componeData.isNodeFace = true;
				componeData.flag = 2;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();

				if (nums1 > 0)
				{
					QHash<int, QHash<int, double>> valRD;
					QHash<int, QHash<int, float>>  valRF;
					QHash<int, QHash<int, double>> valVD;
					QHash<int, QHash<int, float>>  valVF;
					int* ids = lbIds1.data();

					//获取单元节点总数
					int count1 = 0;
					for (int p = 0; p < nums1; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count1 += elenodeNums * componeNums;
					}

					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();

						valRD = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, ids, componeIndex, modelData, local);

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							valVD = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, ids, componeIndex, modelData, local);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								componeData.dRValue = valRD;
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								componeData.dRValue = valVD;
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										int nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);
										nodeHash.insert(nid, sqrt(v0 * v0 + v1 * v1));
									}
									componeData.dRValue.insert(eid, nodeHash);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										int nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);

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
										nodeHash.insert(nid, phase_deg);
									}
									componeData.dRValue.insert(eid, nodeHash);
								}
							}

					}
					else
					{
						componeData.dRValue = valRD;

					}

				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());

				if (nums2 > 0)
				{
					QHash<int, QHash<int, double>> valRD;
					QHash<int, QHash<int, float>> valRF;
					QHash<int, QHash<int, double>> valVD;
					QHash<int, QHash<int, float>> valVF;
					int* ids = lbIds2.data();

					//获取单元节点总数
					int count2 = 0;
					for (int p = 0; p < nums2; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = PostData::postElementFunction::elementNodeCount((EleType)eleType);
						count2 += elenodeNums * componeNums * 2;
					}

					//实部
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
						valRD = calcuEleNodeComponeValuesQ(valRealD2, nums2, componeName, layer, ids, componeIndex, modelData, local);


					//虚部
					if (varType == 1)
					{
						double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							valVD = calcuEleNodeComponeValuesQ(valVirD2, nums2, componeName, layer, ids, componeIndex, modelData, local);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								componeData.dQValue = valRD;
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								componeData.dQValue = valVD;
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										int nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);
										nodeHash.insert(nid, sqrt(v0 * v0 + v1 * v1));
									}
									componeData.dQValue.insert(eid, nodeHash);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										int nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);
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
										nodeHash.insert(nid, phase_deg);
									}
									componeData.dQValue.insert(eid, nodeHash);
								}
							}

					}
					else
					{
						componeData.dQValue = valRD;

					}
				}
			}
			else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress(N)")
			{
				// 预读：实体块头（nums1/ids1），值在两段式读取
				if (!readLegacyVarValues(0, 0, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				componeData.isNodeFace = true;
				componeData.flag = 2;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();

				if (nums1 > 0)
				{
					QHash<int, QHash<int, double>> valRD;
					QHash<int, QHash<int, float>>  valRF;
					QHash<int, QHash<int, double>> valVD;
					QHash<int, QHash<int, float>>  valVF;
					int* ids = lbIds1.data();

					//获取单元节点总数
					int count1 = 0;
					for (int p = 0; p < nums1; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count1 += elenodeNums * componeNums;
					}

					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();

						valRD = calcuEleNodeComponeValues(valRealD1, nums1, componeName, ids, componeIndex, modelData);

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							valVD = calcuEleNodeComponeValues(valVirD1, nums1, componeName, ids, componeIndex, modelData);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								componeData.dRValue = valRD;
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								componeData.dRValue = valVD;
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										int nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);
										nodeHash.insert(nid, sqrt(v0 * v0 + v1 * v1));
									}
									componeData.dRValue.insert(eid, nodeHash);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										int nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);
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
										nodeHash.insert(nid, phase_deg);
									}
									componeData.dRValue.insert(eid, nodeHash);
								}
							}

					}
					else
					{
						componeData.dRValue = valRD;

					}

				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
			else
			{
				// 普通变量（Displacement 等）：预读实体块 + 壳块全部值
				if (!readLegacyVarValues(_varStringList1.value(varName).size(), 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				componeData.isNodeFace = false;
				componeData.flag = 0;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count = componeNums * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds1.data();
					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						for (int p = 0; p < nums1; ++p)
						{
							int rId = ids[p];
							int cId = modelData->_nodedata.nodeidmap.value(rId);							// 三个分量，且分量值没有在存储当中则为mag
							if (componeNums == 3 && componeName.indexOf(compoName) == -1) {
								componeIndex = 0;
								double val = valRealD1[p * componeNums + componeIndex];
								int indexX = p * componeNums + componeIndex;
								double x = valRealD1[p * componeNums + componeIndex];

								int indexY = p * componeNums + componeIndex + 1;
								double y = valRealD1[p * componeNums + componeIndex + 1];

								int indexZ = p * componeNums + componeIndex + 2;
								double z = valRealD1[p * componeNums + componeIndex + 2];
								valRD.insert(cId, sqrt(x * x + y * y + z * z));
							}
							else
							{
								if (local.size() > 0)
								{
									if (local.size() % 3 != 0)
									{
										return false;
									}
									int index = 0;
									double x = valRealD1[p * componeNums + componeIndex];
									double y = valRealD1[p * componeNums + componeIndex + 1];
									double z = valRealD1[p * componeNums + componeIndex + 2];

									QVector3D oldVec;
									oldVec.setX(x);
									oldVec.setY(y);
									oldVec.setZ(z);

									QVector<QVector3D> vector3Ds;
									int numVectors = local.size() / 3;
									for (int i = 0; i < numVectors; ++i)
									{
										int index = i * 3;
										double x = local[index];
										double y = local[index + 1];
										double z = local[index + 2];
										vector3Ds.append(QVector3D(x, y, z));
									}
									QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

									if (componeIndex == 0)
									{
										double val = newVec[0];
										valRD.insert(cId, val);
									}
									else if (componeIndex == 1)
									{
										double val = newVec[1];
										valRD.insert(cId, val);
									}
									else if (componeIndex == 2)
									{
										double val = newVec[2];
										valRD.insert(cId, val);
									}
								}
								else
								{
									double val = valRealD1[p * componeNums + componeIndex];
									valRD.insert(cId, val);
								}
							}

						}

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							for (int p = 0; p < nums1; ++p)
							{
								int rId = ids[p];
								int cId = modelData->_nodedata.nodeidmap.value(rId);
								double val = valVirD1[p * componeNums + componeIndex];
								valVD.insert(cId, val);
							}

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								componeData.value1 = valRD;
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								componeData.value1 = valVD;
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									int id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);
									componeData.value1.insert(id, sqrt(v0 * v0 + v1 * v1));
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									int id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);
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
									componeData.value1.insert(id, phase_deg);
								}
							}

					}
					else
					{
						componeData.value1 = valRD;

					}
				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
		}
		return true;
	}

	bool mMxdbThread::readVarDatas(const QString& filePath, int f, const QString& varName, const QString& compoName, const QString& layer, const QString& complex, QVector<double> local)
	{
		bool isSuccess = openFileStatus(filePath);
		if (!isSuccess)
			return false;

        bool isD = DataPrecisionIsDouble();

		//拿到第f帧的帧信息
		QFileInfo file(filePath);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(filePath);
		if (outPutData == nullptr)
			return false;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		QPointer<mPostModelData> modelData = modelDatabase.value(f);
		if (modelData == nullptr && f == 0)
			modelData = modelDatabase.value(1); // 初始帧注册在 [1]
		if (modelData == nullptr)
			return false;
		PostFrameInfo frameInfo = modelData->_frameInfo;

		//拿到变量信息
		PostVariableInfo curVarInfo;
		QList<PostVariableInfo> varInfos = frameInfo._frameVars;
		auto it = varInfos.begin();
		unsigned long long varPos = 0;
		while (it != varInfos.end())
		{
			PostVariableInfo varInfo = *it;
			if (varInfo._varName == varName)
			{
				curVarInfo = varInfo;
				break;
			}
			it++;
		}
		if (it == varInfos.end())
		{
			//该帧不存在此变量（如SolidColor伪变量），直接返回，避免用无效的_varByte偏移解析文件导致越界
			return false;
		}
		int componeIndex = curVarInfo._compoNames.indexOf(compoName);
		Compones* compones = new Compones();
		compones->isD = true; // readLegacyVarValues 已将 float 提升为 double，value1 恒为 double 容器
		compones->frameID = f;
		std::vector<int> lbIds1, lbIds2;
		std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
		int varType = -1; // 0-常规变量, 1-复数变量（由 readLegacyVarHead 读出）
		std::string lbVarName;

		//存储值
		double maxval = -DBL_MAX, minval = DBL_MAX;
		float maxv = -FLT_MAX, minv = FLT_MAX;
		//存储ID值
		int maxID = 0, minID = 0;

		// 新格式（601/602）：值布局由 reader 返回结构携带（单元变量按结果分组），
		// 无旧格式 20/16 槽位假设，统一按分量名/截面点层/复数方式取标量后直接落库
		//（addResultValue 将 value1/value2 合并处理，新格式单元值统一进 value1）
		if (isNewFormatFile())
		{
			QHash<int, double> vals;
			int valueType = -1;
			if (!readNewFormatScalarValues(curVarInfo, compoName, layer, complex, vals, valueType, modelData))
			{
				delete compones;
				return false;
			}
			compones->varName = varName;
			compones->compoName = compoName;
			compones->isD = true;
			compones->frameID = f;
			compones->flag = valueType; // 0 节点 / 1 单元
			compones->isNodeFace = false;
			compones->value1 = vals;
			_modeldb->setComponeValues(*compones);
			delete compones;
			return true;
		}

		// 预读：块头（varType + varName），替代旧代码的 _readf.read(varType) + _readf.read(varName)
		if (!readLegacyVarHead(curVarInfo._varByte, varType, lbVarName))
			return false;
		// readLegacyVarValues 已将 float 提升为 double，下游统一走 double 路径
		isD = true;
		if (varType == 0 || varType == 1)
		{
			const char* varName = lbVarName.c_str();


			if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" /*|| QString(varName) == "PSD Stress(E)" */|| QString(varName) == "PSD Strain(E)"
				|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
				|| QString(varName) == "Fluid Density(Unaveraged)" || QString(varName) == "Pressure (Unaveraged)"
				|| QString(varName) == "Effective Stress (v-m) (Unaveraged)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
			{
				// 预读：实体块 + 壳块全部值（不 seekg，从块头之后继续读）
				if (!readLegacyVarValues(20, 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				compones->flag = 1;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count1 = 20 * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds1.data();
					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						valRD = calcuComponeValues(valRealD1, nums1, componeName, ids, componeIndex,local);

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							valVD = calcuComponeValues(valVirD1, nums1, componeName, ids, componeIndex,local);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								double tempval;
								int id;
								compones->value1 = valRD;
								auto it = valRD.begin();
								while (it != valRD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								double tempval;
								int id;
								compones->value1 = valVD;
								auto it = valRD.begin();
								while (it != valRD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								int id;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);
									val = sqrt(v0 * v0 + v1 * v1);

									if (val > maxval)
									{
										maxval = val;
										maxID = id;
									}
									if (val < minval)
									{
										minval = val;
										minID = id;
									}

									compones->value1.insert(id, val);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								int id;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);
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
									val = phase_deg;

									if (val > maxval)
									{
										maxval = val;
										maxID = id;
									}
									if (val < minval)
									{
										minval = val;
										minID = id;
									}

									compones->value1.insert(id, val);
								}
							}

					}
					else
					{
						int id;
							double tempval;
							compones->value1 = valRD;

							auto it = valRD.begin();
							while (it != valRD.end())
							{
								tempval = it.value();
								id = it.key();
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
								++it;
							}

					}

				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
				int count2 = 16 * nums2;
				if (nums2 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds2.data();
					//实部
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
						valRD = calcuComponeValues1(valRealD2, nums2, componeName, layer, ids, componeIndex);


					//虚部
					if (varType == 1)
					{
						double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							valVD = calcuComponeValues1(valVirD2, nums2, componeName, layer, ids, componeIndex);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								int id;
								double tempval;
								compones->value2 = valRD;

								auto it = valRD.begin();
								while (it != valRD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								int id;
								double tempval;
								compones->value2 = valVD;

								auto it = valVD.begin();
								while (it != valVD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								int id;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);

									val = sqrt(v0 * v0 + v1 * v1);

									if (val > maxval)
									{
										maxval = val;
										maxID = id;
									}
									if (val < minval)
									{
										minval = val;
										minID = id;
									}

									compones->value2.insert(id, val);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								int id;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);

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
									val = phase_deg;

									if (val > maxval)
									{
										maxval = val;
										maxID = id;
									}
									if (val < minval)
									{
										minval = val;
										minID = id;
									}

									compones->value2.insert(id, val);
								}
							}

					}
					else
					{
						int id;
							double tempval;
							compones->value2 = valRD;
							auto it = valRD.begin();
							while (it != valRD.end())
							{
								tempval = it.value();
								id = it.key();
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
								++it;
							}

					}
				}
			}
			else if (QString(varName) == "Stress(N)")
			{
				// 预读：实体块头（nums1/ids1），值在两段式读取
				if (!readLegacyVarValues(0, 0, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				compones->flag = 2;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();

				if (nums1 > 0)
				{
					QHash<int, QHash<int, double>> valRD;
					QHash<int, QHash<int, float>>  valRF;
					QHash<int, QHash<int, double>> valVD;
					QHash<int, QHash<int, float>>  valVF;
					int* ids = lbIds1.data();

					//获取单元节点总数
					int count1 = 0;
					for (int p = 0; p < nums1; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count1 += elenodeNums * componeNums;
					}

					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();

						valRD = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, ids, componeIndex, modelData,local);

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							valVD = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, ids, componeIndex, modelData,local);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								int id;
								double tempval;
								compones->dRValue = valRD;

								auto it = valRD.begin();
								while (it != valRD.end())
								{
									auto itIt = it.value().begin();
									while (itIt != it.value().end())
									{
										tempval = itIt.value();
										id = itIt.key();
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
										++itIt;
									}
									++it;
								}
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								int id;
								double tempval;
								compones->dRValue = valVD;

								auto it = valVD.begin();
								while (it != valVD.end())
								{
									auto itIt = it.value().begin();
									while (itIt != it.value().end())
									{
										tempval = itIt.value();
										id = itIt.key();
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
										++itIt;
									}
									++it;
								}
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								int nid;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);

										val = sqrt(v0* v0 + v1 * v1);

										if (val > maxval)
										{
											maxval = val;
											maxID = nid;
										}
										if (val < minval)
										{
											minval = val;
											minID = nid;
										}

										nodeHash.insert(nid, val);
									}
									compones->dRValue.insert(eid , nodeHash);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								int nid;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);
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
										val = phase_deg;

										if (val > maxval)
										{
											maxval = val;
											maxID = nid;
										}
										if (val < minval)
										{
											minval = val;
											minID = nid;
										}

										nodeHash.insert(nid, val);
									}
									compones->dRValue.insert(eid, nodeHash);
								}
							}

					}
					else
					{
						int id;
							double tempval;
							compones->dRValue = valRD;

							auto it = valRD.begin();
							while (it != valRD.end())
							{
								auto itIt = it.value().begin();
								while (itIt != it.value().end())
								{
									tempval = itIt.value();
									id = itIt.key();
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
									++itIt;
								}
								++it;
							}

					}

				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());

				if (nums2 > 0)
				{
					QHash<int, QHash<int, double>> valRD;
					QHash<int, QHash<int, float>> valRF;
					QHash<int, QHash<int, double>> valVD;
					QHash<int, QHash<int, float>> valVF;
					int* ids = lbIds2.data();

					//获取单元节点总数
					int count2 = 0;
					for (int p = 0; p < nums2; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count2 += elenodeNums * componeNums * 2;
					}

					//实部
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
						valRD = calcuEleNodeComponeValuesQ(valRealD2, nums2, componeName, layer, ids, componeIndex,modelData,local);


					//虚部
					if (varType == 1)
					{
						double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							valVD = calcuEleNodeComponeValuesQ(valVirD2, nums2, componeName, layer, ids, componeIndex,modelData,local);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								int id;
								double tempval;
								compones->dQValue = valRD;

								auto it = valRD.begin();
								while (it != valRD.end())
								{
									auto itIt = it.value().begin();
									while (itIt != it.value().end())
									{
										tempval = itIt.value();
										id = itIt.key();
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
										++itIt;
									}
									++it;
								}
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								int id;
								double tempval;
								compones->dQValue = valVD;

								auto it = valVD.begin();
								while (it != valVD.end())
								{
									auto itIt = it.value().begin();
									while (itIt != it.value().end())
									{
										tempval = itIt.value();
										id = itIt.key();
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
										++itIt;
									}
									++it;
								}
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								int nid;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);

										val = sqrt(v0 * v0 + v1 * v1);

										if (val > maxval)
										{
											maxval = val;
											maxID = nid;
										}
										if (val < minval)
										{
											minval = val;
											minID = nid;
										}

										nodeHash.insert(nid, val);
									}
									compones->dQValue.insert(eid, nodeHash);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								int nid;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);
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
                                        val = phase_deg;

										if (val > maxval)
										{
											maxval = val;
											maxID = nid;
										}
										if (val < minval)
										{
											minval = val;
											minID = nid;
										}

										nodeHash.insert(nid, val);
									}
									compones->dQValue.insert(eid, nodeHash);
								}
							}

					}
					else
					{
						int id;
							double tempval;
							compones->dQValue = valRD;

							auto it = valRD.begin();
							while (it != valRD.end())
							{
								auto itIt = it.value().begin();
								while (itIt != it.value().end())
								{
									tempval = itIt.value();
									id = itIt.key();
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
									++itIt;
								}
								++it;
							}

					}
				}
			}
			else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress(N)")
			{
				// 预读：实体块头（nums1/ids1），值在两段式读取
				if (!readLegacyVarValues(0, 0, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				compones->flag = 2;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();

				if (nums1 > 0)
				{
					QHash<int, QHash<int, double>> valRD;
					QHash<int, QHash<int, float>>  valRF;
					QHash<int, QHash<int, double>> valVD;
					QHash<int, QHash<int, float>>  valVF;
					int* ids = lbIds1.data();

					//获取单元节点总数
					int count1 = 0;
					for (int p = 0; p < nums1; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count1 += elenodeNums * componeNums;
					}

					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();

						valRD = calcuEleNodeComponeValues(valRealD1, nums1, componeName, ids, componeIndex, modelData);

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							valVD = calcuEleNodeComponeValues(valVirD1, nums1, componeName, ids, componeIndex, modelData);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								int id;
								double tempval;
								compones->dRValue = valRD;

								auto it = valRD.begin();
								while (it != valRD.end())
								{
									auto itIt = it.value().begin();
									while (itIt != it.value().end())
									{
										tempval = itIt.value();
										id = itIt.key();
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
										++itIt;
									}
									++it;
								}
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								int id;
								double tempval;
								compones->dRValue = valVD;

								auto it = valVD.begin();
								while (it != valVD.end())
								{
									auto itIt = it.value().begin();
									while (itIt != it.value().end())
									{
										tempval = itIt.value();
										id = itIt.key();
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
										++itIt;
									}
									++it;
								}
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								int nid;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);

										val = sqrt(v0 * v0 + v1 * v1);

										if (val > maxval)
										{
											maxval = val;
											maxID = nid;
										}
										if (val < minval)
										{
											minval = val;
											minID = nid;
										}

										nodeHash.insert(nid, val);
									}
									compones->dRValue.insert(eid, nodeHash);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								int nid;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									QHash<int, double> nodeHash;
									int eid = ids[k];
									QHash<int, double> rVal = valRD[eid];
									QHash<int, double> vVal = valVD[eid];
									QList<int> nids = rVal.keys();
									for (int n = 0; n < nids.size(); ++n)
									{
										nid = nids[n];
										double v0 = rVal.value(nid);
										double v1 = vVal.value(nid);
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
                                        val = phase_deg;

										if (val > maxval)
										{
											maxval = val;
											maxID = nid;
										}
										if (val < minval)
										{
											minval = val;
											minID = nid;
										}

										nodeHash.insert(nid, val);
									}
									compones->dRValue.insert(eid, nodeHash);
								}
							}

					}
					else
					{
						int id;
							double tempval;
							compones->dRValue = valRD;

							auto it = valRD.begin();
							while (it != valRD.end())
							{
								auto itIt = it.value().begin();
								while (itIt != it.value().end())
								{
									tempval = itIt.value();
									id = itIt.key();
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
									++itIt;
								}
								++it;
							}

					}

				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
            else if (QString(varName) == "secforce")
            {
				// 预读：实体块 + 壳块全部值（不 seekg，从块头之后继续读）
				if (!readLegacyVarValues(_varStringList1.value(varName).size(), 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
                compones->flag = 1;
                //变量具体信息
                //实体个数
int nums1 = static_cast<int>(lbIds1.size());
                QStringList componeName = _varStringList1.value(varName);
                int componeNums = componeName.size();
                int count = componeNums * nums1;

                if (nums1 > 0)
                {
                    QHash<int, double> valRD;
                    QHash<int, float> valRF;
                    QHash<int, double> valVD;
                    QHash<int, float> valVF;
int* ids = lbIds1.data();

                    //实部
                    double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();

                        for (int p = 0; p < nums1; ++p)
                        {
                            int eleId = ids[p];
                            double val = valRealD1[p * componeNums + componeIndex];
                            valRD.insert(eleId, val);
                        }

                    //虚部
                    if (varType == 1)
                    {
                        double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();

                            for (int p = 0; p < nums1; ++p)
                            {
                                int eleId = ids[p];
                                double val = valVirD1[p * componeNums + componeIndex];
                                valVD.insert(eleId, val);
                            }

                    }
                    if (varType == 1)
                    {
                        if (complex == QString("实部") || complex == QString("Real"))
                            {
                                double tempval;
                                int id;
                                compones->value1 = valRD;
                                auto it = valRD.begin();
                                while (it != valRD.end())
                                {
                                    tempval = it.value();
                                    id = it.key();
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
                                    ++it;
                                }
                            }
                            else if (complex == QString("虚部") || complex == QString("Img"))
                            {
                                double tempval;
                                int id;
                                compones->value1 = valVD;
                                auto it = valRD.begin();
                                while (it != valRD.end())
                                {
                                    tempval = it.value();
                                    id = it.key();
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
                                    ++it;
                                }
                            }
                            else if (complex == QString("幅值") || complex == QString("Amplitude"))
                            {
                                int id;
                                double val;
                                QList<int> ids = valRD.keys();
                                for (int k = 0; k < ids.size(); ++k)
                                {
                                    id = ids[k];
                                    double v0 = valRD.value(id);
                                    double v1 = valRF.value(id);
                                    val = sqrt(v0 * v0 + v1 * v1);

                                    if (val > maxv)
                                    {
                                        maxv = val;
                                        maxID = id;
                                    }
                                    if (val < minv)
                                    {
                                        minv = val;
                                        minID = id;
                                    }

                                    compones->value3.insert(id, val);
                                }
                            }
                            else if (complex == QString("相位") || complex == QString("Phase"))
                            {
                                int id;
                                double val;
                                QList<int> ids = valRD.keys();
                                for (int k = 0; k < ids.size(); ++k)
                                {
                                    id = ids[k];
                                    double v0 = valRD.value(id);
                                    double v1 = valRF.value(id);
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
                                    val = phase_deg;

                                    if (val > maxv)
                                    {
                                        maxv = val;
                                        maxID = id;
                                    }
                                    if (val < minv)
                                    {
                                        minv = val;
                                        minID = id;
                                    }

                                    compones->value3.insert(id, val);
                                }
                            }

                    }
                    else
                    {
                        int id;
                            double tempval;
                            compones->value1 = valRD;

                            auto it = valRD.begin();
                            while (it != valRD.end())
                            {
                                tempval = it.value();
                                id = it.key();
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
                                ++it;
                            }

                    }
                }
                //壳单元
int nums2 = static_cast<int>(lbIds2.size());
            }
			else if (QString(varName) == "STRESS" || QString(varName) == "STRAIN" || QString(varName) == "PSD Stress(E)")
			{
				// 预读：实体块 + 壳块全部值（不 seekg，从块头之后继续读）
				if (!readLegacyVarValues(20, 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				compones->flag = 1;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count1 = componeName.size() * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds1.data();
					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						valRD = calcuComponeValues(valRealD1, nums1, componeName, ids, componeIndex, local);

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							valVD = calcuComponeValues(valVirD1, nums1, componeName, ids, componeIndex, local);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								double tempval;
								int id;
								compones->value1 = valRD;
								auto it = valRD.begin();
								while (it != valRD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								double tempval;
								int id;
								compones->value1 = valVD;
								auto it = valRD.begin();
								while (it != valRD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								int id;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);
									val = sqrt(v0 * v0 + v1 * v1);

									if (val > maxval)
									{
										maxval = val;
										maxID = id;
									}
									if (val < minval)
									{
										minval = val;
										minID = id;
									}

									compones->value1.insert(id, val);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								int id;
								double val;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);
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
                                    val = phase_deg;

									if (val > maxval)
									{
										maxval = val;
										maxID = id;
									}
									if (val < minval)
									{
										minval = val;
										minID = id;
									}

									compones->value1.insert(id, val);
								}
							}

					}
					else
					{
						int id;
							double tempval;
							compones->value1 = valRD;

							auto it = valRD.begin();
							while (it != valRD.end())
							{
								tempval = it.value();
								id = it.key();
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
								++it;
							}

					}

				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
			else
			{
				// 普通变量（Displacement 等）：预读实体块 + 壳块全部值
				if (!readLegacyVarValues(_varStringList1.value(varName).size(), 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				compones->flag = 0;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count = componeNums * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds1.data();
					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						for (int p = 0; p < nums1; ++p)
						{
							int rId = ids[p];
							int cId = modelData->_nodedata.nodeidmap.value(rId);

							// 三个分量，且分量值没有在存储当中则为mag
							if (componeNums == 3 && componeName.indexOf(compoName) == -1) {
								componeIndex = 0;
								double val = valRealD1[p * componeNums + componeIndex];
								int indexX = p * componeNums + componeIndex;
								double x = valRealD1[p * componeNums + componeIndex];

								int indexY = p * componeNums + componeIndex + 1;
								double y = valRealD1[p * componeNums + componeIndex + 1];

								int indexZ = p * componeNums + componeIndex + 2;
								double z = valRealD1[p * componeNums + componeIndex + 2];
								valRD.insert(cId, x * x + y * y + z * z);
							}
							else 
							{
								if (local.size() > 0)
								{
									if (local.size() % 3 != 0)
									{
										return false;
									}
									int index = 0;
									double x = valRealD1[p * componeNums + componeIndex];
									double y = valRealD1[p * componeNums + componeIndex + 1];
									double z = valRealD1[p * componeNums + componeIndex + 2];

									QVector3D oldVec;
									oldVec.setX(x);
									oldVec.setY(y);
									oldVec.setZ(z);

									QVector<QVector3D> vector3Ds;
									int numVectors = local.size() / 3;
									for (int i = 0; i < numVectors; ++i)
									{
										int index = i * 3;
										double x = local[index];
										double y = local[index + 1];
										double z = local[index + 2];
										vector3Ds.append(QVector3D(x, y, z));
									}
									QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
								
									if (componeIndex == 0)
									{
										double val = newVec[0];
										valRD.insert(cId, val);
									}
									else if (componeIndex == 1)
									{
										double val = newVec[1];
										valRD.insert(cId, val);
									}
									else if (componeIndex == 2)
									{
										double val = newVec[2];
										valRD.insert(cId, val);
									}
								}
								else
								{
									double val = valRealD1[p * componeNums + componeIndex];
									valRD.insert(cId, val);
								}
							}

						}

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							for (int p = 0; p < nums1; ++p)
							{
								int rId = ids[p];
								int cId = modelData->_nodedata.nodeidmap.value(rId);

								// 三个分量，且分量值没有在存储当中则为mag
								if (componeNums == 3 && componeName.indexOf(compoName) == -1) {
									componeIndex = 0;
									double val = valVirD1[p * componeNums + componeIndex];
									int indexX = p * componeNums + componeIndex;
									double x = valVirD1[p * componeNums + componeIndex];

									int indexY = p * componeNums + componeIndex + 1;
									double y = valVirD1[p * componeNums + componeIndex + 1];

									int indexZ = p * componeNums + componeIndex + 2;
									double z = valVirD1[p * componeNums + componeIndex + 2];
									valRD.insert(cId, x * x + y * y + z * z + valRD.value(cId));
								}
								else
								{
									if (local.size() > 0)
									{
										if (local.size() % 3 != 0)
										{
											return false;
										}
										int index = 0;
										double x = valVirD1[p * componeNums + componeIndex];
										double y = valVirD1[p * componeNums + componeIndex + 1];
										double z = valVirD1[p * componeNums + componeIndex + 2];

										QVector3D oldVec;
										oldVec.setX(x);
										oldVec.setY(y);
										oldVec.setZ(z);

										QVector<QVector3D> vector3Ds;
										int numVectors = local.size() / 3;
										for (int i = 0; i < numVectors; ++i)
										{
											int index = i * 3;
											double x = local[index];
											double y = local[index + 1];
											double z = local[index + 2];
											vector3Ds.append(QVector3D(x, y, z));
										}
										QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

										if (componeIndex == 0)
										{
											double val = newVec[0];
											valVD.insert(cId, val);
										}
										else if (componeIndex == 1)
										{
											double val = newVec[1];
											valVD.insert(cId, val);
										}
										else if (componeIndex == 2)
										{
											double val = newVec[2];
											valVD.insert(cId, val);
										}
									}
									else
									{
										double val = valVirD1[p * componeNums + componeIndex];
										valVD.insert(cId, val);
									}
								}

							}
							/*for (int p = 0; p < nums1; ++p)
							{
								int rId = ids[p];
								int cId = modelData->_nodedata.nodeidmap.value(rId);
								double val = valVirD1[p * componeNums + componeIndex];
								valVD.insert(cId, val);
							}*/

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								int id;
								double tempval;
								compones->value1 = valRD;

								auto it = valRD.begin();
								while (it != valRD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								int id;
								double tempval;
								compones->value1 = valVD;

								auto it = valVD.begin();
								while (it != valVD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								double val;
								int id;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);

									val = sqrt(v0 * v0 + v1 * v1);

									if (val > maxval)
									{
										maxval = val;
										maxID = id;
									}
									if (val < minval)
									{
										minval = val;
										minID = id;
									}

									compones->value1.insert(id, val);
								}
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								double val;
								int id;
								QList<int> ids = valRD.keys();
								for (int k = 0; k < ids.size(); ++k)
								{
									id = ids[k];
									double v0 = valRD.value(id);
									double v1 = valVD.value(id);

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
                                    val = phase_deg;

									if (val > maxval)
									{
										maxval = val;
										maxID = id;
									}
									if (val < minval)
									{
										minval = val;
										minID = id;
									}

									compones->value1.insert(id, val);
								}
							}
							else
							{
								if (componeNums == 3 && componeName.indexOf(compoName) == -1)
								{
									int id;
									double tempval;

									auto it = valRD.begin();
									while (it != valRD.end())
									{
										tempval = sqrt(it.value());
										id = it.key();
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
										compones->value1.insert(it.key(), tempval);
										++it;
									}
								}
							}

					}
					else
					{
						if (componeNums == 3 && componeName.indexOf(compoName) == -1)
							{
								int id;
								double tempval;
							
								auto it = valRD.begin();
								while (it != valRD.end())
								{
									tempval = sqrt(it.value());
									id = it.key();
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
									compones->value1.insert(it.key(), tempval);
									++it;
								}
							}
							else
							{
								int id;
								double tempval;
								compones->value1 = valRD;

								auto it = valRD.begin();
								while (it != valRD.end())
								{
									tempval = it.value();
									id = it.key();
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
									++it;
								}
							}

					}
				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
		}
		QList<QPair<int, double>> temp{};
		temp.append(QPair<int, double>(maxID, maxval));
			temp.append(QPair<int, double>(minID, minval));

		compones->varName = varName;
		compones->compoName = compoName;
		//_modeldb->setResultMaxMin(f, temp);
		_modeldb->setComponeValues(*compones);
		delete compones;
		return true;
	}

	bool mMxdbThread::readVarDatas(const QString & filePath, int f, const QString & varName, const QStringList & compos, const QString & layer, const QString & complex, QVector<double> local, std::set<int> selids, QHash<QString, QHash<int, double>>& allVals, int & flag)
	{
		bool isSuccess = openFileStatus(filePath);
		if (!isSuccess)
			return false;

        bool isD = DataPrecisionIsDouble();

		//拿到第f帧的帧信息
		QFileInfo file(filePath);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(filePath);
		if (outPutData == nullptr)
			return false;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		QPointer<mPostModelData> modelData = modelDatabase.value(f);
		if (modelData == nullptr && f == 0)
			modelData = modelDatabase.value(1); // 初始帧注册在 [1]
		if (modelData == nullptr)
			return false;
		PostFrameInfo frameInfo = modelData->_frameInfo;

		//拿到变量信息
		PostVariableInfo curVarInfo;
		QList<PostVariableInfo> varInfos = frameInfo._frameVars;
		auto it = varInfos.begin();
		unsigned long long varPos = 0;
		while (it != varInfos.end())
		{
			PostVariableInfo varInfo = *it;
			if (varInfo._varName == varName)
			{
				curVarInfo = varInfo;
				break;
			}
			it++;
		}
		if (it == varInfos.end())
		{
			//该帧不存在此变量（如SolidColor伪变量），直接返回，避免用无效的_varByte偏移解析文件导致越界
			return false;
		}
		
		std::vector<int> lbIds1, lbIds2;
		std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
		int varType = -1; // 0-常规变量, 1-复数变量（由 readLegacyVarHead 读出）
		std::string lbVarName;

		// 预读：块头（varType + varName），替代旧代码的 _readf.read(varType) + _readf.read(varName)
		if (!readLegacyVarHead(curVarInfo._varByte, varType, lbVarName))
			return false;
		if (varType == 0 || varType == 1)
		{
			const char* varName = lbVarName.c_str();


			if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
				|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
				|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
			{
				// 预读：实体块 + 壳块全部值（不 seekg，从块头之后继续读）
				if (!readLegacyVarValues(20, 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 1;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count1 = 20 * nums1;
				std::set<int> qids{};
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds1.data();
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();

					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}
					
					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							valRD = calcuComponeValues(valRealD1, nums1, componeName, ids, componeIndex, local);
						}
						else
						{
							valRF = toFloatHashF(calcuComponeValues(valRealF1, nums1, componeName, ids, componeIndex, local));
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuComponeValues(valVirD1, nums1, componeName, ids, componeIndex, local);
							}
							else
							{
								valVF = toFloatHashF(calcuComponeValues(valVirF1, nums1, componeName, ids, componeIndex, local));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											vals.insert(id, valRD.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVD.constFind(id) != valVD.cend())
										{
											vals.insert(id, valVD.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
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
                                            val = phase_deg;

											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											vals.insert(id, valRF.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVF.constFind(id) != valVF.cend())
										{
											vals.insert(id, valVF.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRD.constFind(id) != valRD.cend())
									{
										vals.insert(id, valRD.value(id));
									}
									else
									{
										qids.insert(id);
									}
								}
								allVals[compo] = vals;
							}
							else
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRF.constFind(id) != valRF.cend())
									{
										vals.insert(id, valRF.value(id));
									}
									else
									{
										qids.insert(id);
									}
								}
								allVals[compo] = vals;
							}
						}
					}
				}

				if (nums1 == 0)
				{
					qids = selids;
				}

				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
				int count2 = 16 * nums2;
				if (nums2 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds2.data();
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
					double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
					double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
					double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();

					//实部
					if (isD)
					{
					}
					else
					{
					}

					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}
					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);

						//实部
						if (isD)
						{
							valRD = calcuComponeValues1(valRealD2, nums2, componeName, layer, ids, componeIndex);
						}
						else
						{
							valRF = toFloatHashF(calcuComponeValues1(valRealF2, nums2, componeName, layer, ids, componeIndex));
						}

						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuComponeValues1(valVirD2, nums2, componeName, layer, ids, componeIndex);
							}
							else
							{
								valVF = toFloatHashF(calcuComponeValues1(valVirF2, nums2, componeName, layer, ids, componeIndex));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											vals.insert(id, valRD.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVD.constFind(id) != valVD.cend())
										{
											vals.insert(id, valVD.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											vals.insert(id, valRF.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVF.constFind(id) != valVF.cend())
										{
											vals.insert(id, valVF.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRD.constFind(id) != valRD.cend())
									{
										vals.insert(id, valRD.value(id));
									}
								}
								allVals[compo] = vals;
							}
							else
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRF.constFind(id) != valRF.cend())
									{
										vals.insert(id, valRF.value(id));
									}
								}
								allVals[compo] = vals;
							}
						}
					}
				}
			}
			else if (QString(varName) == "Stress(N)")
			{
				// 预读：实体块头（nums1/ids1），值在两段式读取
				if (!readLegacyVarValues(0, 0, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 2;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				std::set<int> qids{};
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float>  valRF;
					QHash<int, double> valVD;
					QHash<int, float>  valVF;
					int* ids = lbIds1.data();
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();

					//获取单元节点总数
					int count1 = 0;
					for (int p = 0; p < nums1; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count1 += elenodeNums * componeNums;
					}

					//实部
					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							valRD = calcuEleNodeComponeValR(valRealD1, nums1, componeName, ids, componeIndex, modelData, local);
						}
						else
						{
							valRF = toFloatHashF(calcuEleNodeComponeValR(valRealF1, nums1, componeName, ids, componeIndex, modelData, local));
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuEleNodeComponeValR(valVirD1, nums1, componeName, ids, componeIndex, modelData, local);
							}
							else
							{
								valVF = toFloatHashF(calcuEleNodeComponeValR(valVirF1, nums1, componeName, ids, componeIndex, modelData, local));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											vals.insert(id, valRD.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVD.constFind(id) != valVD.cend())
										{
											vals.insert(id, valVD.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											vals.insert(id, valRF.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVF.constFind(id) != valVF.cend())
										{
											vals.insert(id, valVF.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRD.constFind(id) != valRD.cend())
									{
										vals.insert(id, valRD.value(id));
									}
									else
									{
										qids.insert(id);
									}
								}
								allVals[compo] = vals;
							}
							else
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRF.constFind(id) != valRF.cend())
									{
										vals.insert(id, valRF.value(id));
									}
									else
									{
										qids.insert(id);
									}
								}
								allVals[compo] = vals;
							}
						}
					}
				}

				if (nums1 == 0)
					qids = selids;

				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());

				if (nums2 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
					double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
					double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
					double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();

					int* ids = lbIds2.data();

					//获取单元节点总数
					int count2 = 0;
					for (int p = 0; p < nums2; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count2 += elenodeNums * componeNums * 2;
					}

					//实部
					if (isD)
					{
					}
					else
					{
					}

					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							valRD = calcuEleNodeComponeValQ(valRealD2, nums2, componeName, layer, ids, componeIndex, modelData, local);
						}
						else
						{
							valRF = toFloatHashF(calcuEleNodeComponeValQ(valRealF2, nums2, componeName, layer, ids, componeIndex, modelData, local));
						}

						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuEleNodeComponeValQ(valVirD2, nums2, componeName, layer, ids, componeIndex, modelData, local);
							}
							else
							{
								valVF = toFloatHashF(calcuEleNodeComponeValQ(valVirF2, nums2, componeName, layer, ids, componeIndex, modelData, local));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											vals.insert(id, valRD.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVD.constFind(id) != valVD.cend())
										{
											vals.insert(id, valVD.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											vals.insert(id, valRF.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVF.constFind(id) != valVF.cend())
										{
											vals.insert(id, valVF.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRD.constFind(id) != valRD.cend())
									{
										vals.insert(id, valRD.value(id));
									}
								}
								allVals[compo] = vals;
							}
							else
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRF.constFind(id) != valRF.cend())
									{
										vals.insert(id, valRF.value(id));
									}
								}
								allVals[compo] = vals;
							}
						}
					}
				}
			}
			else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress(N)")
			{
				// 预读：实体块头（nums1/ids1），值在两段式读取
				if (!readLegacyVarValues(0, 0, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 2;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				std::set<int> qids{};
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float>  valRF;
					QHash<int, double> valVD;
					QHash<int, float>  valVF;
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
					int* ids = lbIds1.data();

					//获取单元节点总数
					int count1 = 0;
					for (int p = 0; p < nums1; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count1 += elenodeNums * componeNums;
					}

					//实部
					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							valRD = calcuEleNodeComponeVal(valRealD1, nums1, componeName, ids, componeIndex, modelData);
						}
						else
						{
							valRF = toFloatHashF(calcuEleNodeComponeVal(valRealF1, nums1, componeName, ids, componeIndex, modelData));
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuEleNodeComponeVal(valVirD1, nums1, componeName, ids, componeIndex, modelData);
							}
							else
							{
								valVF = toFloatHashF(calcuEleNodeComponeVal(valVirF1, nums1, componeName, ids, componeIndex, modelData));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											vals.insert(id, valRD.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVD.constFind(id) != valVD.cend())
										{
											vals.insert(id, valVD.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											vals.insert(id, valRF.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVF.constFind(id) != valVF.cend())
										{
											vals.insert(id, valVF.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRD.constFind(id) != valRD.cend())
									{
										vals.insert(id, valRD.value(id));
									}
									else
									{
										qids.insert(id);
									}
								}
								allVals[compo] = vals;
							}
							else
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRF.constFind(id) != valRF.cend())
									{
										vals.insert(id, valRF.value(id));
									}
									else
									{
										qids.insert(id);
									}
								}
								allVals[compo] = vals;
							}
						}
					}
				}

				if (nums1 == 0)
					qids = selids;

				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
			else if (QString(varName) == "secforce")
			{
				// 预读：实体块 + 壳块全部值（不 seekg，从块头之后继续读）
				if (!readLegacyVarValues(_varStringList1.value(varName).size(), 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 1;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count = componeNums * nums1;
				std::set<int> qids{};
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;

					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
					int* ids = lbIds1.data();

					//实部
					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							for (int p = 0; p < nums1; ++p)
							{
								int eleId = ids[p];
								double val = valRealD1[p * componeNums + componeIndex];
								valRD.insert(eleId, val);
							}
						}
						else
						{
							for (int p = 0; p < nums1; ++p)
							{
								int eleId = ids[p];
								float val = valRealF1[p * componeNums + componeIndex];
								valRF.insert(eleId, val);
							}
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								for (int p = 0; p < nums1; ++p)
								{
									int eleId = ids[p];
									double val = valVirD1[p * componeNums + componeIndex];
									valVD.insert(eleId, val);
								}
							}
							else
							{
								for (int p = 0; p < nums1; ++p)
								{
									int eleId = ids[p];
									float val = valVirF1[p * componeNums + componeIndex];
									valVF.insert(eleId, val);
								}
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											vals.insert(id, valRD.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVD.constFind(id) != valVD.cend())
										{
											vals.insert(id, valVD.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valRF.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valRF.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											vals.insert(id, valRF.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVF.constFind(id) != valVF.cend())
										{
											vals.insert(id, valVF.value(id));
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
										else
										{
											qids.insert(id);
										}
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRD.constFind(id) != valRD.cend())
									{
										vals.insert(id, valRD.value(id));
									}
									else
									{
										qids.insert(id);
									}
								}
								allVals[compo] = vals;
							}
							else
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRF.constFind(id) != valRF.cend())
									{
										vals.insert(id, valRF.value(id));
									}
									else
									{
										qids.insert(id);
									}
								}
								allVals[compo] = vals;
							}
						}
					}
				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
			else
			{
				// 普通变量（Displacement 等）：预读实体块 + 壳块全部值
				if (!readLegacyVarValues(_varStringList1.value(varName).size(), 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 0;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count = componeNums * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
					int* ids = lbIds1.data();

					//实部
					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}
					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							for (int p = 0; p < nums1; ++p)
							{
								int rId = ids[p];

								// 三个分量，且分量值没有在存储当中则为mag
								if (componeNums == 3 && componeName.indexOf(compo) == -1) {
									componeIndex = 0;
									double val = valRealD1[p * componeNums + componeIndex];
									int indexX = p * componeNums + componeIndex;
									double x = valRealD1[p * componeNums + componeIndex];

									int indexY = p * componeNums + componeIndex + 1;
									double y = valRealD1[p * componeNums + componeIndex + 1];

									int indexZ = p * componeNums + componeIndex + 2;
									double z = valRealD1[p * componeNums + componeIndex + 2];
									valRD.insert(rId, sqrt(x * x + y * y + z * z));
								}
								else
								{
									if (local.size() > 0)
									{
										if (local.size() % 3 != 0)
										{
											return false;
										}
										int index = 0;
										double x = valRealD1[p * componeNums + componeIndex];
										double y = valRealD1[p * componeNums + componeIndex + 1];
										double z = valRealD1[p * componeNums + componeIndex + 2];

										QVector3D oldVec;
										oldVec.setX(x);
										oldVec.setY(y);
										oldVec.setZ(z);

										QVector<QVector3D> vector3Ds;
										int numVectors = local.size() / 3;
										for (int i = 0; i < numVectors; ++i)
										{
											int index = i * 3;
											double x = local[index];
											double y = local[index + 1];
											double z = local[index + 2];
											vector3Ds.append(QVector3D(x, y, z));
										}
										QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

										if (componeIndex == 0)
										{
											double val = newVec[0];
											valRD.insert(rId, val);
										}
										else if (componeIndex == 1)
										{
											double val = newVec[1];
											valRD.insert(rId, val);
										}
										else if (componeIndex == 2)
										{
											double val = newVec[2];
											valRD.insert(rId, val);
										}
									}
									else
									{
										double val = valRealD1[p * componeNums + componeIndex];
										valRD.insert(rId, val);
									}
								}

							}
						}
						else
						{
							for (int p = 0; p < nums1; ++p)
							{
								int rId = ids[p];
								// 三个分量，且分量值没有在存储当中则为mag
								if (componeNums == 3 && componeName.indexOf(compo) == -1) {
									componeIndex = 0;
									float val = valRealF1[p * componeNums + componeIndex];
									int indexX = p * componeNums + componeIndex;
									float x = valRealF1[p * componeNums + componeIndex];

									int indexY = p * componeNums + componeIndex + 1;
									float y = valRealF1[p * componeNums + componeIndex + 1];

									int indexZ = p * componeNums + componeIndex + 2;
									float z = valRealF1[p * componeNums + componeIndex + 2];
									valRF.insert(rId, sqrt(x * x + y * y + z * z));
								}
								else
								{
									if (local.size() > 0)
									{
										if (local.size() % 3 != 0)
										{
											return false;
										}
										int index = 0;
										float x = valRealF1[p * componeNums + componeIndex];
										float y = valRealF1[p * componeNums + componeIndex + 1];
										float z = valRealF1[p * componeNums + componeIndex + 2];

										QVector3D oldVec;
										oldVec.setX(x);
										oldVec.setY(y);
										oldVec.setZ(z);

										QVector<QVector3D> vector3Ds;
										int numVectors = local.size() / 3;
										for (int i = 0; i < numVectors; ++i)
										{
											int index = i * 3;
											double x = local[index];
											double y = local[index + 1];
											double z = local[index + 2];
											vector3Ds.append(QVector3D(x, y, z));
										}
										QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

										if (componeIndex == 0)
										{
											double val = newVec[0];
											valRF.insert(rId, val);
										}
										else if (componeIndex == 1)
										{
											double val = newVec[1];
											valRF.insert(rId, val);
										}
										else if (componeIndex == 2)
										{
											double val = newVec[2];
											valRF.insert(rId, val);
										}
									}
									else
									{
										float val = valRealF1[p * componeNums + componeIndex];
										valRF.insert(rId, val);
									}
								}
							}
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								for (int p = 0; p < nums1; ++p)
								{
									int rId = ids[p];
									double val = valVirD1[p * componeNums + componeIndex];
									valVD.insert(rId, val);
								}
							}
							else
							{
								for (int p = 0; p < nums1; ++p)
								{
									int rId = ids[p];
									float val = valVirF1[p * componeNums + componeIndex];
									valVF.insert(rId, val);
								}
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											vals.insert(id, valRD.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVD.constFind(id) != valVD.cend())
										{
											vals.insert(id, valVD.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRD.constFind(id) != valRD.cend())
										{
											double v0 = valRD.value(id);
											double v1 = valVD.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											vals.insert(id, valRF.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals = allVals.value(compo);
									for (auto id : selids)
									{
										if (valVF.constFind(id) != valVF.cend())
										{
											vals.insert(id, valVF.value(id));
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
											val = sqrt(v0 * v0 + v1 * v1);
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (auto id : selids)
									{
										if (valRF.constFind(id) != valRF.cend())
										{
											double v0 = valRF.value(id);
											double v1 = valVF.value(id);
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
                                            val = phase_deg;
											vals.insert(id, val);
										}
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRD.constFind(id) != valRD.cend())
									{
										vals.insert(id, valRD.value(id));
									}
								}
								allVals[compo] = vals;
							}
							else
							{
								QHash<int, double> vals = allVals.value(compo);
								for (auto id : selids)
								{
									if (valRF.constFind(id) != valRF.cend())
									{
										float  val = valRF.value(id);
										vals.insert(id, val);
									}
								}
								allVals[compo] = vals;
							}
						}
					}
				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
		}
		
		return true;
	}

	bool mMxdbThread::readVarDatas(const QString & filePath, int f, const QString & varName, const QStringList & compos, const QString & layer, const QString & complex, QVector<double> local, QHash<QString, QHash<int, double>>& allVals, int & flag)
	{
		bool isSuccess = openFileStatus(filePath);
		if (!isSuccess)
			return false;

        bool isD = DataPrecisionIsDouble();

		//拿到第f帧的帧信息
		QFileInfo file(filePath);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(filePath);
		if (outPutData == nullptr)
			return false;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		QPointer<mPostModelData> modelData = modelDatabase.value(f);
		if (modelData == nullptr && f == 0)
			modelData = modelDatabase.value(1); // 初始帧注册在 [1]
		if (modelData == nullptr)
			return false;
		PostFrameInfo frameInfo = modelData->_frameInfo;

		//拿到变量信息
		PostVariableInfo curVarInfo;
		QList<PostVariableInfo> varInfos = frameInfo._frameVars;
		auto it = varInfos.begin();
		unsigned long long varPos = 0;
		while (it != varInfos.end())
		{
			PostVariableInfo varInfo = *it;
			if (varInfo._varName == varName)
			{
				curVarInfo = varInfo;
				break;
			}
			it++;
		}
		if (it == varInfos.end())
		{
			//该帧不存在此变量（如SolidColor伪变量），直接返回，避免用无效的_varByte偏移解析文件导致越界
			return false;
		}

		std::vector<int> lbIds1, lbIds2;
		std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
		int varType = -1; // 0-常规变量, 1-复数变量（由 readLegacyVarHead 读出）
		std::string lbVarName;

		// 预读：块头（varType + varName），替代旧代码的 _readf.read(varType) + _readf.read(varName)
		if (!readLegacyVarHead(curVarInfo._varByte, varType, lbVarName))
			return false;
		if (varType == 0 || varType == 1)
		{
			const char* varName = lbVarName.c_str();


			if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
				|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
				|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
			{
				// 预读：实体块 + 壳块全部值（不 seekg，从块头之后继续读）
				if (!readLegacyVarValues(20, 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 1;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count1 = 20 * nums1;
				//std::set<int> qids{};
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds1.data();
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();

					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							valRD = calcuComponeValues(valRealD1, nums1, componeName, ids, componeIndex, local);
						}
						else
						{
							valRF = toFloatHashF(calcuComponeValues(valRealF1, nums1, componeName, ids, componeIndex, local));
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuComponeValues(valVirD1, nums1, componeName, ids, componeIndex, local);
							}
							else
							{
								valVF = toFloatHashF(calcuComponeValues(valVirF1, nums1, componeName, ids, componeIndex, local));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									allVals[compo] = valRD;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									allVals[compo] = valVD;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double val;
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
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
                                        val = phase_deg;

										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals{};
									auto it = valRF.begin();
									while (it != valRF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals{};
									auto it = valVF.begin();
									while (it != valVF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								allVals[compo] = valRD;
							}
							else
							{
								QHash<int, double> vals{};
								auto it = valRF.begin();
								while (it != valRF.end())
								{
									vals.insert(it.key(), it.value());
									++it;
								}
								allVals[compo] = vals;
							}
						}
					}
				}

			

				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
				int count2 = 16 * nums2;
				if (nums2 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds2.data();
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
					double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
					double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
					double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();

					//实部
					if (isD)
					{
					}
					else
					{
					}

					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}
					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);

						//实部
						if (isD)
						{
							valRD = calcuComponeValues1(valRealD2, nums2, componeName, layer, ids, componeIndex);
						}
						else
						{
							valRF = toFloatHashF(calcuComponeValues1(valRealF2, nums2, componeName, layer, ids, componeIndex));
						}

						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuComponeValues1(valVirD2, nums2, componeName, layer, ids, componeIndex);
							}
							else
							{
								valVF = toFloatHashF(calcuComponeValues1(valVirF2, nums2, componeName, layer, ids, componeIndex));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									allVals[compo] = valRD;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									allVals[compo] = valVD;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums2; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums2; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals{};
									auto it = valRF.begin();
									while (it != valRF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals{};
									auto it = valVF.begin();
									while (it != valVF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums2; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums2; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								allVals[compo] = valRD;
							}
							else
							{
								QHash<int, double> vals{};

								auto it = valRF.begin();
								while (it != valRF.end())
								{
									vals.insert(it.key(), it.value());
									++it;
								}
								allVals[compo] = vals;
							}
						}
					}
				}
			}
			else if (QString(varName) == "Stress(N)")
			{
				// 预读：实体块头（nums1/ids1），值在两段式读取
				if (!readLegacyVarValues(0, 0, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 2;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float>  valRF;
					QHash<int, double> valVD;
					QHash<int, float>  valVF;
					int* ids = lbIds1.data();
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();

					//获取单元节点总数
					int count1 = 0;
					for (int p = 0; p < nums1; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count1 += elenodeNums * componeNums;
					}

					//实部
					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							valRD = calcuEleNodeComponeValR(valRealD1, nums1, componeName, ids, componeIndex, modelData, local);
						}
						else
						{
							valRF = toFloatHashF(calcuEleNodeComponeValR(valRealF1, nums1, componeName, ids, componeIndex, modelData, local));
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuEleNodeComponeValR(valVirD1, nums1, componeName, ids, componeIndex, modelData, local);
							}
							else
							{
								valVF = toFloatHashF(calcuEleNodeComponeValR(valVirF1, nums1, componeName, ids, componeIndex, modelData, local));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									allVals[compo] = valRD;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									allVals[compo] = valVD;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals{};
									auto it = valRF.begin();
									while (it != valRF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}

									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals{};
									auto it = valVF.begin();
									while (it != valVF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								allVals[compo] = valRD;
							}
							else
							{
								QHash<int, double> vals{};
								auto it = valRF.begin();
								while (it != valRF.end())
								{
									vals.insert(it.key(), it.value());
									++it;
								}
								allVals[compo] = vals;
							}
						}
					}
				}

				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());

				if (nums2 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
					double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
					double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
					double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();

					int* ids = lbIds2.data();

					//获取单元节点总数
					int count2 = 0;
					for (int p = 0; p < nums2; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count2 += elenodeNums * componeNums * 2;
					}

					//实部
					if (isD)
					{
					}
					else
					{
					}

					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							valRD = calcuEleNodeComponeValQ(valRealD2, nums2, componeName, layer, ids, componeIndex, modelData, local);
						}
						else
						{
							valRF = toFloatHashF(calcuEleNodeComponeValQ(valRealF2, nums2, componeName, layer, ids, componeIndex, modelData, local));
						}

						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuEleNodeComponeValQ(valVirD2, nums2, componeName, layer, ids, componeIndex, modelData, local);
							}
							else
							{
								valVF = toFloatHashF(calcuEleNodeComponeValQ(valVirF2, nums2, componeName, layer, ids, componeIndex, modelData, local));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									allVals[compo] = valRD;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									allVals[compo] = valVD;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums2; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums2; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals{};
									auto it = valRF.begin();
									while (it != valRF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals{};
									auto it = valVF.begin();
									while (it != valVF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums2; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums2; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								allVals[compo] = valRD;
							}
							else
							{
								QHash<int, double> vals{};
								auto it = valRF.begin();
								while (it != valRF.end())
								{
									vals.insert(it.key(), it.value());
									++it;
								}
								allVals[compo] = vals;
							}
						}
					}
				}
			}
			else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress(N)")
			{
				// 预读：实体块头（nums1/ids1），值在两段式读取
				if (!readLegacyVarValues(0, 0, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 2;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float>  valRF;
					QHash<int, double> valVD;
					QHash<int, float>  valVF;
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
					int* ids = lbIds1.data();

					//获取单元节点总数
					int count1 = 0;
					for (int p = 0; p < nums1; ++p)
					{
						int rId = ids[p];
						int partID = modelData->getPartIDByEleID(rId);
						QPointer<mPostPartData> partData = modelData->getPartData(partID);
                        int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                        int eleType = partData->_parteledata.cell_info[ele_cid].type;
						int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
						count1 += elenodeNums * componeNums;
					}

					//实部
					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							valRD = calcuEleNodeComponeVal(valRealD1, nums1, componeName, ids, componeIndex, modelData);
						}
						else
						{
							valRF = toFloatHashF(calcuEleNodeComponeVal(valRealF1, nums1, componeName, ids, componeIndex, modelData));
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								valVD = calcuEleNodeComponeVal(valVirD1, nums1, componeName, ids, componeIndex, modelData);
							}
							else
							{
								valVF = toFloatHashF(calcuEleNodeComponeVal(valVirF1, nums1, componeName, ids, componeIndex, modelData));
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									allVals[compo] = valRD;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									allVals[compo] = valVD;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals{};
									auto it = valRF.begin();
									while (it != valRF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals{};
									auto it = valVF.begin();
									while (it != valVF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								allVals[compo] = valRD;
							}
							else
							{
								QHash<int, double> vals{};

								auto it = valRF.begin();
								while (it != valRF.end())
								{
									vals.insert(it.key(), it.value());
									++it;
								}
								allVals[compo] = vals;
							}
						}
					}
				}

				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
			else if (QString(varName) == "secforce")
			{
				// 预读：实体块 + 壳块全部值（不 seekg，从块头之后继续读）
				if (!readLegacyVarValues(_varStringList1.value(varName).size(), 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 1;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count = componeNums * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;

					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
					int* ids = lbIds1.data();

					//实部
					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}

					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							for (int p = 0; p < nums1; ++p)
							{
								int eleId = ids[p];
								double val = valRealD1[p * componeNums + componeIndex];
								valRD.insert(eleId, val);
							}
						}
						else
						{
							for (int p = 0; p < nums1; ++p)
							{
								int eleId = ids[p];
								float val = valRealF1[p * componeNums + componeIndex];
								valRF.insert(eleId, val);
							}
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								for (int p = 0; p < nums1; ++p)
								{
									int eleId = ids[p];
									double val = valVirD1[p * componeNums + componeIndex];
									valVD.insert(eleId, val);
								}
							}
							else
							{
								for (int p = 0; p < nums1; ++p)
								{
									int eleId = ids[p];
									float val = valVirF1[p * componeNums + componeIndex];
									valVF.insert(eleId, val);
								}
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									allVals[compo] = valRD;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									allVals[compo] = valVD;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valRF.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals = allVals.value(compo);
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valRF.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals{};
									auto it = valRF.begin();
									while (it != valRF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals{};
									auto it = valVF.begin();
									while (it != valVF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; k++)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; k++)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								allVals[compo] = valRD;
							}
							else
							{
								QHash<int, double> vals{};
								auto it = valRF.begin();
								while (it != valRF.end())
								{
									vals.insert(it.key(), it.value());
									++it;
								}
								allVals[compo] = vals;
							}
						}
					}
				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
			else
			{
				// 普通变量（Displacement 等）：预读实体块 + 壳块全部值
				if (!readLegacyVarValues(_varStringList1.value(varName).size(), 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
					return false;
				flag = 0;
				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count = componeNums * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
					double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
					int* ids = lbIds1.data();

					//实部
					if (isD)
					{
					}
					else
					{
					}
					//虚部
					if (varType == 1)
					{
						if (isD)
						{
						}
						else
						{
						}
					}
					for (auto compo : compos)
					{
						int componeIndex = curVarInfo._compoNames.indexOf(compo);
						//实部
						if (isD)
						{
							for (int p = 0; p < nums1; ++p)
							{
								int rId = ids[p];

								// 三个分量，且分量值没有在存储当中则为mag
								if (componeNums == 3 && componeName.indexOf(compo) == -1) {
									componeIndex = 0;
									double val = valRealD1[p * componeNums + componeIndex];
									int indexX = p * componeNums + componeIndex;
									double x = valRealD1[p * componeNums + componeIndex];

									int indexY = p * componeNums + componeIndex + 1;
									double y = valRealD1[p * componeNums + componeIndex + 1];

									int indexZ = p * componeNums + componeIndex + 2;
									double z = valRealD1[p * componeNums + componeIndex + 2];
									valRD.insert(rId, sqrt(x * x + y * y + z * z));
								}
								else
								{
									if (local.size() > 0)
									{
										if (local.size() % 3 != 0)
										{
											return false;
										}
										int index = 0;
										double x = valRealD1[p * componeNums + componeIndex];
										double y = valRealD1[p * componeNums + componeIndex + 1];
										double z = valRealD1[p * componeNums + componeIndex + 2];

										QVector3D oldVec;
										oldVec.setX(x);
										oldVec.setY(y);
										oldVec.setZ(z);

										QVector<QVector3D> vector3Ds;
										int numVectors = local.size() / 3;
										for (int i = 0; i < numVectors; ++i)
										{
											int index = i * 3;
											double x = local[index];
											double y = local[index + 1];
											double z = local[index + 2];
											vector3Ds.append(QVector3D(x, y, z));
										}
										QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

										if (componeIndex == 0)
										{
											double val = newVec[0];
											valRD.insert(rId, val);
										}
										else if (componeIndex == 1)
										{
											double val = newVec[1];
											valRD.insert(rId, val);
										}
										else if (componeIndex == 2)
										{
											double val = newVec[2];
											valRD.insert(rId, val);
										}
									}
									else
									{
										double val = valRealD1[p * componeNums + componeIndex];
										valRD.insert(rId, val);
									}
								}

							}
						}
						else
						{
							for (int p = 0; p < nums1; ++p)
							{
								int rId = ids[p];
								// 三个分量，且分量值没有在存储当中则为mag
								if (componeNums == 3 && componeName.indexOf(compo) == -1) {
									componeIndex = 0;
									float val = valRealF1[p * componeNums + componeIndex];
									int indexX = p * componeNums + componeIndex;
									float x = valRealF1[p * componeNums + componeIndex];

									int indexY = p * componeNums + componeIndex + 1;
									float y = valRealF1[p * componeNums + componeIndex + 1];

									int indexZ = p * componeNums + componeIndex + 2;
									float z = valRealF1[p * componeNums + componeIndex + 2];
									valRF.insert(rId, sqrt(x * x + y * y + z * z));
								}
								else
								{
									if (local.size() > 0)
									{
										if (local.size() % 3 != 0)
										{
											return false;
										}
										int index = 0;
										float x = valRealF1[p * componeNums + componeIndex];
										float y = valRealF1[p * componeNums + componeIndex + 1];
										float z = valRealF1[p * componeNums + componeIndex + 2];

										QVector3D oldVec;
										oldVec.setX(x);
										oldVec.setY(y);
										oldVec.setZ(z);

										QVector<QVector3D> vector3Ds;
										int numVectors = local.size() / 3;
										for (int i = 0; i < numVectors; ++i)
										{
											int index = i * 3;
											double x = local[index];
											double y = local[index + 1];
											double z = local[index + 2];
											vector3Ds.append(QVector3D(x, y, z));
										}
										QVector3D newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);

										if (componeIndex == 0)
										{
											double val = newVec[0];
											valRF.insert(rId, val);
										}
										else if (componeIndex == 1)
										{
											double val = newVec[1];
											valRF.insert(rId, val);
										}
										else if (componeIndex == 2)
										{
											double val = newVec[2];
											valRF.insert(rId, val);
										}
									}
									else
									{
										float val = valRealF1[p * componeNums + componeIndex];
										valRF.insert(rId, val);
									}
								}
							}
						}
						//虚部
						if (varType == 1)
						{
							if (isD)
							{
								for (int p = 0; p < nums1; ++p)
								{
									int rId = ids[p];
									double val = valVirD1[p * componeNums + componeIndex];
									valVD.insert(rId, val);
								}
							}
							else
							{
								for (int p = 0; p < nums1; ++p)
								{
									int rId = ids[p];
									float val = valVirF1[p * componeNums + componeIndex];
									valVF.insert(rId, val);
								}
							}
						}
						if (varType == 1)
						{
							if (isD)
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									allVals[compo] = valRD;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									allVals[compo] = valVD;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRD.value(id);
										double v1 = valVD.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
							else
							{
								if (complex == QString("实部") || complex == QString("Real"))
								{
									QHash<int, double> vals{};
									auto it = valRF.begin();
									while (it != valRF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("虚部") || complex == QString("Img"))
								{
									QHash<int, double> vals{};
									auto it = valVF.begin();
									while (it != valVF.end())
									{
										vals.insert(it.key(), it.value());
										++it;
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("幅值") || complex == QString("Amplitude"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
										val = sqrt(v0 * v0 + v1 * v1);
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
								else if (complex == QString("相位") || complex == QString("Phase"))
								{
									QHash<int, double> vals{};
									double val;
									for (int k = 0; k < nums1; ++k)
									{
										int id = ids[k];
										double v0 = valRF.value(id);
										double v1 = valVF.value(id);
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
                                        val = phase_deg;
										vals.insert(id, val);
									}
									allVals[compo] = vals;
								}
							}
						}
						else
						{
							if (isD)
							{
								allVals[compo] = valRD;
							}
							else
							{
								QHash<int, double> vals{};
								auto it = valRF.begin();
								while (it != valRF.end())
								{
									vals.insert(it.key(), it.value());
									++it;
								}
								allVals[compo] = vals;
							}
						}
					}
				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
			}
		}

		return true;
	}

	bool mMxdbThread::readElemData(const QString & filePath, int f, const QString & varName, const QString & compoName, const QString & layer, const QString & complex, QVector<double> local, std::set<int> eleids)
	{
		bool isSuccess = openFileStatus(filePath);
		if (!isSuccess)
			return false;

        bool isD = DataPrecisionIsDouble();


		//拿到第f帧的帧信息
		QFileInfo file(filePath);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(filePath);
		if (outPutData == nullptr)
			return false;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		QPointer<mPostModelData> modelData = modelDatabase.value(f);
		if (modelData == nullptr && f == 0)
			modelData = modelDatabase.value(1); // 初始帧注册在 [1]
		if (modelData == nullptr)
			return false;
		PostFrameInfo frameInfo = modelData->_frameInfo;

		//拿到变量信息
		PostVariableInfo curVarInfo;
		QList<PostVariableInfo> varInfos = frameInfo._frameVars;
		auto it = varInfos.begin();
		unsigned long long varPos = 0;
		while (it != varInfos.end())
		{
			PostVariableInfo varInfo = *it;
			if (varInfo._varName == varName)
			{
				curVarInfo = varInfo;
				break;
			}
			it++;
		}
		if (it == varInfos.end())
		{
			//该帧不存在此变量（如SolidColor伪变量），直接返回，避免用无效的_varByte偏移解析文件导致越界
			return false;
		}
		int componeIndex = curVarInfo._compoNames.indexOf(compoName);
		// 预读：块头（varType + varName），替代旧代码的 _readf.read(varType) + _readf.read(varName)
		std::vector<int> lbIds1, lbIds2;
		std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
		int varType = -1;
		std::string lbVarName;
		if (!readLegacyVarHead(curVarInfo._varByte, varType, lbVarName))
			return false;
		// readLegacyVarValues 已将 float 提升为 double，下游统一走 double 路径
		isD = true;

		QHash<int, double> tempvalues{};
		QHash<int, float> tempvaluesf{};
		if (varType == 0 || varType == 1)
		{
			const char* varName = lbVarName.c_str();
			QStringList componeName = _varStringList1.value(varName);
			int componeNums = componeName.size();
			// 按变量类型决定每实体值数：Stress/Strain 20，普通变量 componeNums
			const bool isStressVar = (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
				|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
				|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)");
			const int vpE = isStressVar ? 20 : componeNums;
			if (!readLegacyVarValues(vpE, 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
				return false;

			//变量具体信息
			//实体个数
			int nums1 = static_cast<int>(lbIds1.size());
			int count1 = vpE * nums1;
			if (nums1 > 0)
			{
				QHash<int, double> valRD;
				QHash<int, float> valRF;
				QHash<int, double> valVD;
				QHash<int, float> valVF;
				int* ids = lbIds1.data();
				//实部
				double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
					valRD = calcuComponeValues(valRealD1, nums1, componeName, ids, componeIndex, local);

				//虚部
				if (varType == 1)
				{
					double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
						valVD = calcuComponeValues(valVirD1, nums1, componeName, ids, componeIndex, local);

				}
				if (varType == 1)
				{
					if (complex == QString("实部") || complex == QString("Real"))
						{
							tempvalues = valRD;
						}
						else if (complex == QString("虚部") || complex == QString("Img"))
						{
							tempvalues = valVD;
						}
						else if (complex == QString("幅值") || complex == QString("Amplitude"))
						{
							int id;
							double val;
							QList<int> ids = valRD.keys();
							for (int k = 0; k < ids.size(); ++k)
							{
								id = ids[k];
								double v0 = valRD.value(id);
								double v1 = valVD.value(id);
								val = sqrt(v0 * v0 + v1 * v1);

								tempvalues.insert(id, val);
							}
						}
						else if (complex == QString("相位") || complex == QString("Phase"))
						{
							int id;
							double val;
							QList<int> ids = valRD.keys();
							for (int k = 0; k < ids.size(); ++k)
							{
								id = ids[k];
								double v0 = valRD.value(id);
								double v1 = valVD.value(id);
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
                                val = phase_deg;

								tempvalues.insert(id, val);
							}
						}

				}
				else
				{
					tempvalues = valRD;

				}

			}
			//壳单元
			int nums2 = static_cast<int>(lbIds2.size());
			int count2 = 16 * nums2;
			if (nums2 > 0)
			{
				QHash<int, double> valRD;
				QHash<int, float> valRF;
				QHash<int, double> valVD;
				QHash<int, float> valVF;
				int* ids = lbIds2.data();
				//实部
				double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
					valRD = calcuComponeValues1(valRealD2, nums2, componeName, layer, ids, componeIndex);


				//虚部
				if (varType == 1)
				{
					double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
						valVD = calcuComponeValues1(valVirD2, nums2, componeName, layer, ids, componeIndex);

				}
				if (varType == 1)
				{
					if (complex == QString("实部") || complex == QString("Real"))
						{
							tempvalues = valRD;
						}
						else if (complex == QString("虚部") || complex == QString("Img"))
						{
							tempvalues = valVD;
						}
						else if (complex == QString("幅值") || complex == QString("Amplitude"))
						{
							int id;
							double val;
							QList<int> ids = valRD.keys();
							for (int k = 0; k < ids.size(); ++k)
							{
								id = ids[k];
								double v0 = valRD.value(id);
								double v1 = valVD.value(id);

								val = sqrt(v0 * v0 + v1 * v1);

								tempvalues.insert(id, val);
							}
						}
						else if (complex == QString("相位") || complex == QString("Phase"))
						{
							int id;
							double val;
							QList<int> ids = valRD.keys();
							for (int k = 0; k < ids.size(); ++k)
							{
								id = ids[k];
								double v0 = valRD.value(id);
								double v1 = valVD.value(id);

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
                                val = phase_deg;

								tempvalues.insert(id, val);
							}
						}

				}
				else
				{
					tempvalues = valRD;

				}
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
					double val = tempvalues.value(nodeid);
					int rid = modelData->_nodedata.ids[nodeid];
					values.insert({ rid, val });
				}
				elesdata.insert({ eleid, values });
			}

		
		outPutData->setElemsData(elesdata);
		return true;
	}

	bool mMxdbThread::readFlattenStress(const QString& filePath, int f, const QString& varName, const QString& compoName, const QString& layer, const QString& complex, QVector<double> local)
	{
		bool isSuccess = openFileStatus(filePath);
		if (!isSuccess)
			return false;

        bool isD = DataPrecisionIsDouble();

		//拿到第f帧的帧信息
		QFileInfo file(filePath);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(filePath);
		if (outPutData == nullptr)
			return false;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		QPointer<mPostModelData> modelData = modelDatabase.value(f);
		if (modelData == nullptr && f == 0)
			modelData = modelDatabase.value(1); // 初始帧注册在 [1]
		if (modelData == nullptr)
			return false;
		PostFrameInfo frameInfo = modelData->_frameInfo;

		//拿到变量信息
		PostVariableInfo curVarInfo;
		QList<PostVariableInfo> varInfos = frameInfo._frameVars;
		auto it = varInfos.begin();
		unsigned long long varPos = 0;
		while (it != varInfos.end())
		{
			PostVariableInfo varInfo = *it;
			if (varInfo._varName == varName)
			{
				curVarInfo = varInfo;
				break;
			}
			it++;
		}
		if (it == varInfos.end())
		{
			//该帧不存在此变量（如SolidColor伪变量），直接返回，避免用无效的_varByte偏移解析文件导致越界
			return false;
		}
		int componeIndex = curVarInfo._compoNames.indexOf(compoName);
		std::vector<int> lbIds1, lbIds2;
		std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
		int varType = -1;
		std::string lbVarName;
		// 预读：块头 + 实体/壳块全部值（20/16 槽 Stress 布局）
		if (!readLegacyVarHead(curVarInfo._varByte, varType, lbVarName))
			return false;
		isD = true;
		if (!readLegacyVarValues(20, 16, varType == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
			return false;
		Compones* compones = new Compones();
		compones->isD = true; // readLegacyVarValues 已将 float 提升为 double，value1 恒为 double 容器
		compones->frameID = f;
		if (varType == 0 || varType == 1)
		{
			const char* varName = lbVarName.c_str();


			if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
				|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
				|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
			{
				compones->flag = 0;
				compones->isNodeFace = false;

				std::list<QPointer<mPostPartData>> parts = modelData->getAllPartData();

				//变量具体信息
				//实体个数
				int nums1 = static_cast<int>(lbIds1.size());
				QStringList componeName = _varStringList1.value(varName);
				int componeNums = componeName.size();
				int count1 = 20 * nums1;
				if (nums1 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds1.data();
					//实部
					double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						valRD = calcuComponeValues(valRealD1, nums1, componeName, ids, componeIndex, local);

					//虚部
					if (varType == 1)
					{
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							valVD = calcuComponeValues(valVirD1, nums1, componeName, ids, componeIndex, local);

					}
					
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								QHash<int, double> vals{};

								for (auto part : parts)
								{
									if (part == nullptr)
										continue;

                                    for(int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                    {
                                        int ele_rid = part->_parteledata.cell_rid[ele_cid];
                                        double val = 0.0;
                                        if (valRD.constFind(ele_rid) == valRD.cend())
                                            continue;
                                        val = valRD.value(ele_rid);
                                        int type = part->_parteledata.cell_info[ele_cid].type;
                                        int pos = part->_parteledata.cell_node_offsets[ele_cid];
                                        int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
                                        if (nodeCount > 0)
                                        {
                                            for (int j = 0; j < nodeCount; ++j)
                                            {
                                                int nodeId = part->_parteledata.cell_node_ids[pos];
                                                if (vals.constFind(nodeId) == vals.cend())
                                                {
                                                    vals.insert(nodeId, val);
                                                }
                                                else
                                                {
                                                    vals[nodeId] += val;
                                                }
                                                ++pos;
                                            }
                                        }

                                    }
								}

								for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
								{
									if (vals.constFind(id) != vals.cend())
									{
										double val = vals[id];
										vals[id] = val / modelData->_nodedata.nodeEleCountmap.value(id);
									}
								}

								compones->value1 = vals;
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								QHash<int, double> vals{};

								for (auto part : parts)
								{
									if (part == nullptr)
										continue;

                                    for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                    {
                                        int ele_rid = part->_parteledata.cell_rid[ele_cid];
										double val = 0.0;
										if (valVD.constFind(ele_rid) == valVD.cend())
											continue;
										val = valVD.value(ele_rid);
										int type = part->_parteledata.cell_info[ele_cid].type;
										int pos = part->_parteledata.cell_node_offsets[ele_cid];
										int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
										if (nodeCount > 0)
										{
											for (int j = 0; j < nodeCount; ++j)
											{
												int nodeId = part->_parteledata.cell_node_ids[pos];
												if (vals.constFind(nodeId) == vals.cend())
												{
													vals.insert(nodeId, val);
												}
												else
												{
													vals[nodeId] += val;
												}
												++pos;
											}
										}
									}
								}

								for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
								{
									if (vals.constFind(id) != vals.cend())
									{
										double val = vals[id];
										vals[id] = val / modelData->_nodedata.nodeEleCountmap.value(id);
									}
								}

								compones->value1 = vals;
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								QHash<int, double> vals{};

								for (auto part : parts)
								{
									if (part == nullptr)
										continue;

                                    for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                    {
                                        int ele_rid = part->_parteledata.cell_rid[ele_cid];
										double val = 0.0;
										if (valRD.constFind(ele_rid) == valRD.cend())
											continue;
										double v0 = valRD.value(ele_rid);
										double v1 = valVD.value(ele_rid);
										val = sqrt(v0 * v0 + v1 * v1);
										int type = part->_parteledata.cell_info[ele_cid].type;
										int pos = part->_parteledata.cell_node_offsets[ele_cid];
										int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
										if (nodeCount > 0)
										{
											for (int j = 0; j < nodeCount; ++j)
											{
												int nodeId = part->_parteledata.cell_node_ids[pos];
												if (vals.constFind(nodeId) == vals.cend())
												{
													vals.insert(nodeId, val);
												}
												else
												{
													vals[nodeId] += val;
												}
												++pos;
											}
										}
									}
								}

								for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
								{
									if (vals.constFind(id) != vals.cend())
									{
										double val = vals[id];
										vals[id] = val / modelData->_nodedata.nodeEleCountmap.value(id);
									}
								}

								compones->value1 = vals;
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								QHash<int, double> vals{};

								for (auto part : parts)
								{
									if (part == nullptr)
										continue;

                                    for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                    {
                                        int ele_rid = part->_parteledata.cell_rid[ele_cid];
										double val = 0.0;
										if (valRD.constFind(ele_rid) == valRD.cend())
											continue;
										double v0 = valRD.value(ele_rid);
										double v1 = valVD.value(ele_rid);
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
                                        val = phase_deg;
										int type = part->_parteledata.cell_info[ele_cid].type;
										int pos = part->_parteledata.cell_node_offsets[ele_cid];
										int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
										if (nodeCount > 0)
										{
											for (int j = 0; j < nodeCount; ++j)
											{
												int nodeId = part->_parteledata.cell_node_ids[pos];
												if (vals.constFind(nodeId) == vals.cend())
												{
													vals.insert(nodeId, val);
												}
												else
												{
													vals[nodeId] += val;
												}
												++pos;
											}
										}
									}
								}

								for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
								{
									if (vals.constFind(id) != vals.cend())
									{
										double val = vals[id];
										vals[id] = val / modelData->_nodedata.nodeEleCountmap.value(id);
									}
								}
								compones->value1 = vals;
							}

					}
					else
					{
						QHash<int, double> vals{};

							for (auto part : parts)
							{
								if (part == nullptr)
									continue;

                                for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                {
                                    int ele_rid = part->_parteledata.cell_rid[ele_cid];
									double val = 0.0;
									if (valRD.constFind(ele_rid) == valRD.cend())
										continue;
									val = valRD.value(ele_rid);
									int type = part->_parteledata.cell_info[ele_cid].type;
									int pos = part->_parteledata.cell_node_offsets[ele_cid];
									int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
									if (nodeCount > 0)
									{
										for (int j = 0; j < nodeCount; ++j)
										{
											int nodeId = part->_parteledata.cell_node_ids[pos];
											if (vals.constFind(nodeId) == vals.cend())
											{
												vals.insert(nodeId, val);
											}
											else
											{
												vals[nodeId] += val;
											}
											++pos;
										}
									}
								}
							}

							for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
							{
								if (vals.constFind(id) != vals.cend())
								{
									double val = vals[id];
									vals[id] = val / modelData->_nodedata.nodeEleCountmap.value(id);
								}
							}
							compones->value1 = vals;

					}

				}
				//壳单元
				int nums2 = static_cast<int>(lbIds2.size());
				int count2 = 16 * nums2;
				if (nums2 > 0)
				{
					QHash<int, double> valRD;
					QHash<int, float> valRF;
					QHash<int, double> valVD;
					QHash<int, float> valVF;
					int* ids = lbIds2.data();
					//实部
					double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
						valRD = calcuComponeValues1(valRealD2, nums2, componeName, layer, ids, componeIndex);


					//虚部
					if (varType == 1)
					{
						double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							valVD = calcuComponeValues1(valVirD2, nums2, componeName, layer, ids, componeIndex);

					}
					if (varType == 1)
					{
						if (complex == QString("实部") || complex == QString("Real"))
							{
								QHash<int, double> vals{};

								for (auto part : parts)
								{
									if (part == nullptr)
										continue;

                                    for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                    {
                                        int ele_rid = part->_parteledata.cell_rid[ele_cid];
										double val = 0.0;
										if (valRD.constFind(ele_rid) == valRD.cend())
											continue;
										val = valRD.value(ele_rid);
										int type = part->_parteledata.cell_info[ele_cid].type;
										int pos = part->_parteledata.cell_node_offsets[ele_cid];
										int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
										if (nodeCount > 0)
										{
											for (int j = 0; j < nodeCount; ++j)
											{
												int nodeId = part->_parteledata.cell_node_ids[pos];
												if (vals.constFind(nodeId) == vals.cend())
												{
													vals.insert(nodeId, val);
												}
												else
												{
													vals[nodeId] += val;
												}
												++pos;
											}
										}
									}
								}
								QList<int> ms = modelData->_nodedata.nodeEleCountmap.keys();
								for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
								{
									double val = vals[id];
									val = val / modelData->_nodedata.nodeEleCountmap.value(id);
									vals[id] = val;
								}

								compones->value2 = vals;
							}
							else if (complex == QString("虚部") || complex == QString("Img"))
							{
								QHash<int, double> vals{};

								for (auto part : parts)
								{
									if (part == nullptr)
										continue;

                                    for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                    {
                                        int ele_rid = part->_parteledata.cell_rid[ele_cid];
										double val = 0.0;
										if (valVD.constFind(ele_rid) == valVD.cend())
											continue;
										val = valVD.value(ele_rid);
										int type = part->_parteledata.cell_info[ele_cid].type;
										int pos = part->_parteledata.cell_node_offsets[ele_cid];
										int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
										if (nodeCount > 0)
										{
											for (int j = 0; j < nodeCount; ++j)
											{
												int nodeId = part->_parteledata.cell_node_ids[pos];
												if (vals.constFind(nodeId) == vals.cend())
												{
													vals.insert(nodeId, val);
												}
												else
												{
													vals[nodeId] += val;
												}
												++pos;
											}
										}
									}
								}
								QList<int> ms = modelData->_nodedata.nodeEleCountmap.keys();
								for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
								{
									double val = vals[id];
									val = val / modelData->_nodedata.nodeEleCountmap.value(id);
									vals[id] = val;
								}

								compones->value2 = vals;
							}
							else if (complex == QString("幅值") || complex == QString("Amplitude"))
							{
								QHash<int, double> vals{};

								for (auto part : parts)
								{
									if (part == nullptr)
										continue;

                                    for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                    {
                                        int ele_rid = part->_parteledata.cell_rid[ele_cid];
										double val = 0.0;
										if (valRD.constFind(ele_rid) == valRD.cend())
											continue;
										double v0 = valRD.value(ele_rid);
										double v1 = valVD.value(ele_rid);
										val = sqrt(v0 * v0 + v1 * v1);
										int type = part->_parteledata.cell_info[ele_cid].type;
										int pos = part->_parteledata.cell_node_offsets[ele_cid];
										int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
										if (nodeCount > 0)
										{
											for (int j = 0; j < nodeCount; ++j)
											{
												int nodeId = part->_parteledata.cell_node_ids[pos];
												if (vals.constFind(nodeId) == vals.cend())
												{
													vals.insert(nodeId, val);
												}
												else
												{
													vals[nodeId] += val;
												}
												++pos;
											}
										}
									}
								}
								QList<int> ms = modelData->_nodedata.nodeEleCountmap.keys();
								for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
								{
									double val = vals[id];
									val = val / modelData->_nodedata.nodeEleCountmap.value(id);
									vals[id] = val;
								}

								compones->value2 = vals;
							}
							else if (complex == QString("相位") || complex == QString("Phase"))
							{
								QHash<int, double> vals{};

								for (auto part : parts)
								{
									if (part == nullptr)
										continue;

                                    for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                    {
                                        int ele_rid = part->_parteledata.cell_rid[ele_cid];
										double val = 0.0;
										if (valRD.constFind(ele_rid) == valRD.cend())
											continue;
										double v0 = valRD.value(ele_rid);
										double v1 = valVD.value(ele_rid);
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
                                        val = phase_deg;
										int type = part->_parteledata.cell_info[ele_cid].type;
										int pos = part->_parteledata.cell_node_offsets[ele_cid];
										int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
										if (nodeCount > 0)
										{
											for (int j = 0; j < nodeCount; ++j)
											{
												int nodeId = part->_parteledata.cell_node_ids[pos];
												if (vals.constFind(nodeId) == vals.cend())
												{
													vals.insert(nodeId, val);
												}
												else
												{
													vals[nodeId] += val;
												}
												++pos;
											}
										}
									}
								}
								QList<int> ms = modelData->_nodedata.nodeEleCountmap.keys();
								for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
								{
									double val = vals[id];
									val = val / modelData->_nodedata.nodeEleCountmap.value(id);
									vals[id] = val;
								}

								compones->value2 = vals;
							}

					}
					else
					{
						QHash<int, double> vals{};

							for (auto part : parts)
							{
								if (part == nullptr)
									continue;

                                for (int ele_cid = 0; ele_cid < part->_parteledata.cell_rid.size(); ++ele_cid)
                                {
                                    int ele_rid = part->_parteledata.cell_rid[ele_cid];
									double val = 0.0;
									if (valRD.constFind(ele_rid) == valRD.cend())
										continue;
									val = valRD.value(ele_rid);
									int type = part->_parteledata.cell_info[ele_cid].type;
									int pos = part->_parteledata.cell_node_offsets[ele_cid];
									int nodeCount = MxFunctions::mElementFunction::elementNodeCount((ElementType)type);
									if (nodeCount > 0)
									{
										for (int j = 0; j < nodeCount; ++j)
										{
											int nodeId = part->_parteledata.cell_node_ids[pos];
											if (vals.constFind(nodeId) == vals.cend())
											{
												vals.insert(nodeId, val);
											}
											else
											{
												vals[nodeId] += val;
											}
											++pos;
										}
									}
								}
							}
							QList<int> ms = modelData->_nodedata.nodeEleCountmap.keys();
							for (auto id : modelData->_nodedata.nodeEleCountmap.keys())
							{
								double val = vals[id];
								val = val / modelData->_nodedata.nodeEleCountmap.value(id);
								vals[id] = val;
							}

							compones->value2 = vals;

					}
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		int i = modelData->_nodedata.nodeamount;
		_modeldb->setComponeValues(*compones);
		delete compones;
		return true;
	}

	bool mMxdbThread::readVectorData(const QString& filePath, int f, const QString& varName)
	{
		bool isSuccess = openFileStatus(filePath);
		if (!isSuccess)
			return false;

        bool isD = DataPrecisionIsDouble();

		//拿到第f帧的帧信息
		QFileInfo file(filePath);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(filePath);
		if (outPutData == nullptr)
			return false;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		QPointer<mPostModelData> modelData = modelDatabase.value(f);
		if (modelData == nullptr && f == 0)
			modelData = modelDatabase.value(1); // 初始帧注册在 [1]
		if (modelData == nullptr)
			return false;
		PostFrameInfo frameInfo = modelData->_frameInfo;

		//拿到变量信息
		PostVariableInfo curVarInfo;
		QList<PostVariableInfo> varInfos = frameInfo._frameVars;
		auto it = varInfos.begin();
		unsigned long long varPos = 0;
		while (it != varInfos.end())
		{
			PostVariableInfo varInfo = *it;
			if (varInfo._varName == varName)
			{
				curVarInfo = varInfo;
				break;
			}
			it++;
		}
		if (it == varInfos.end())
		{
			//该帧不存在此变量（如SolidColor伪变量），直接返回，避免用无效的_varByte偏移解析文件导致越界
			return false;
		}

		QStringList compoentNames = curVarInfo._compoNames;
		if (compoentNames.size() == 3) {
			// 经 MxdbReaderCore 预读全部实体值（旧版矢量变量只会是节点变量，3 分量）
			std::vector<int> ids;
			std::vector<double> real, imag;
			if (!readVarValuesToArrays(curVarInfo, {}, ids, real, imag))
				return false;

			const int nums1 = static_cast<int>(ids.size());
			const int componeNums = 3;

			if (nums1 > 1) {
				// 实部
				QHash<int, QVector3D> vectorData{};// 这里是要读取的数据存储的地方
				for (int p = 0; p < nums1; p++)
				{
					int rId = ids[p];
					int cId = modelData->_nodedata.nodeidmap.value(rId);
					double x = real[p * componeNums + 0];
					double y = real[p * componeNums + 1];
					double z = real[p * componeNums + 2];

					QVector3D vector{ (float)x,(float)y,(float)z };
					vectorData.insert(cId, vector);// 存到数据
				}
				_modeldb->setVectorDataByFrameID(f, vectorData);
				// 虚部（复数矢量）：旧版读出后即丢弃（注释掉的代码），迁移后同样不使用
			}
		}
		return true;

	}

	void mMxdbThread::readCurveData(const QString& pname, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QVector<int>& ids, const std::set<int>& frameIds, QStringList& cnames, QHash<QString, QHash<int, double >> &yvaluesD, QHash<QString, QHash<int, float>>& yvaluesF, QString caseName)
	{
		QFileInfo file(pname);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname); 
		if (outPutData == nullptr)
			return;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		if (modelDatabase.size() == 0)
			return;
		int nframe = modelDatabase.size();
		for (int i = 1; i <= nframe; ++i)
		{
			QPointer<mPostModelData> modelData = modelDatabase.value(i);
			if (modelData == nullptr)	return;
			if (frameIds.find(i) != frameIds.end())
			{
				PostFrameInfo info = modelDatabase.value(i)->_frameInfo;
				if (caseName.compare("All") == 0 || caseName.compare(info._frameCase) == 0)
				{
					if (frameIds.find(info._frameNum) != frameIds.end())
					{
						int nvar = info._frameVars.size();
						for (int p = 0; p < nvar; ++p)
						{
							PostVariableInfo varInfo = info._frameVars.at(p);
							if (var == varInfo._varName)
							{
								bool isSuccess = openFileStatus(info._frameFile);
								if (!isSuccess)
									return;

                                bool isD = DataPrecisionIsDouble();

								// 新格式（601/602）：值区偏移绝对定位（与帧无关），按分量/层/复数取标量
								if (isNewFormatFile())
								{
									for (const QString& compo : compos)
									{
										QHash<int, double> vals;
										int valueType = -1;
										if (!readNewFormatScalarValues(varInfo, compo, layer, complex, vals, valueType))
											continue;
										QString cname = QString("ID-%1_%2_%3_%4-f%5").arg(var).arg(compo).arg(layer).arg(complex).arg(info._frameNum);
										if (!cnames.contains(cname))
											cnames.append(cname);
										QHash<int, double> tempvec;
										if (yvaluesD.contains(cname))
											tempvec = yvaluesD.value(cname);
										for (int id : ids)
										{
											if (vals.contains(id))
												tempvec.insert(id, vals.value(id));
										}
										yvaluesD.insert(cname, tempvec);
									}
									continue;
								}

								//获取层索引
								int layerIndex = varInfo._varLayerNames.indexOf(layer);
								//获取当前变量的字节位置
								unsigned long long pos = varInfo._varByte;
								// 预读：块头（type+varName），值在各分支内按需读
								std::vector<int> lbIds1, lbIds2;
								std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
								int type = -1;
								std::string lbVarName;
								if (!readLegacyVarHead(pos, type, lbVarName))
									continue;

								const char* varName = lbVarName.c_str();
								if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
									|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
									|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
								{
									// 预读：实体块 + 壳块全部值
									if (!readLegacyVarValues(20, 16, type == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
										continue;
									//变量具体信息
									//实体个数
									int nums1 = static_cast<int>(lbIds1.size());
									QStringList componeName = _varStringList1.value(varName);
									int componeNums = componeName.size();
									int count1 = 20 * nums1;
									int* idsR = lbIds1.data();
									double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
									double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
									if (nums1 > 0)
									{
										int* idsR = lbIds1.data();
										//实部
										if (isD)
										{

										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									//壳单元
									int nums2 = static_cast<int>(lbIds2.size());
									int count2 = 16 * nums2;
									int* idsQ = lbIds2.data();
									double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
									double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
									double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
									double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
									if (nums2 > 0)
									{
										int* idsQ = lbIds2.data();
										//实部
										if (isD)
										{
										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									//
									for (QString compo : compos)
									{
										//实体
										for (int i = 0; i < nums1; ++i)
										{
											if (ids.contains(idsR[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2_%3_%4-f%5").arg(var).arg(compo).arg(layer).arg(complex).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(compo);
												QHash<int, double> valD1{};
												QHash<int, double> valD2{};
												QHash<int, float> valD3{};
												QHash<int, float> valD4{};

												if (isD)
												{
													valD1 = calcuComponeValues(valRealD1, nums1, componeName, idsR, compoIndex);
													valD2 = calcuComponeValues(valVirD1, nums1, componeName, idsR, compoIndex);
												}
												else
												{
													valD3 = toFloatHashF(calcuComponeValues(valRealF1, nums1, componeName, idsR, compoIndex));
													valD4 = toFloatHashF(calcuComponeValues(valVirF1, nums1, componeName, idsR, compoIndex));
												}

												if (complex == QString("虚部") || complex == QString("Img"))
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], valD2.value(idsR[i]));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], valD4.value(idsR[i]));
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (complex == QString("幅值") || complex == QString("Amplitude"))
												{
													if (isD)
													{
														double v0 = valD1.value(idsR[i]);
														double v1 = valD2.value(idsR[i]);
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valD3.value(idsR[i]);
														float v1 = valD4.value(idsR[i]);
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (complex == QString("相位") || complex == QString("Phase"))
												{
													if (isD)
													{
														double v0 = valD1.value(idsR[i]);
														double v1 = valD2.value(idsR[i]);
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
                                                       
														tempvec.insert(idsR[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valD3.value(idsR[i]);
														float v1 = valD4.value(idsR[i]);
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], valD1.value(idsR[i]));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], valD3.value(idsR[i]));
														yvaluesF.insert(cname, tempvec);
													}
												}

											}
										}
										//壳
										for (int i = 0; i < nums2; ++i)
										{
											if (ids.contains(idsQ[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2_%3_%4-f%5").arg(var).arg(compo).arg(layer).arg(complex).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(compo);
												QHash<int, double> valD1{};
												QHash<int, double> valD2{};
												QHash<int, float> valD3{};
												QHash<int, float> valD4{};

												if (isD)
												{
													valD1 = calcuComponeValues1(valRealD2, nums2, componeName, layer, idsQ, compoIndex);
													valD2 = calcuComponeValues1(valVirD2, nums2, componeName, layer, idsQ, compoIndex);
												}
												else
												{
													valD3 = toFloatHashF(calcuComponeValues1(valRealF2, nums2, componeName, layer, idsQ, compoIndex));
													valD4 = toFloatHashF(calcuComponeValues1(valVirF2, nums2, componeName, layer, idsQ, compoIndex));
												}

												if (complex == QString("虚部") || complex == QString("Img"))
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], valD2.value(idsQ[i]));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], valD4.value(idsQ[i]));
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (complex == QString("幅值") || complex == QString("Amplitude"))
												{
													if (isD)
													{
														double v0 = valD1.value(idsQ[i]);
														double v1 = valD2.value(idsQ[i]);
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valD3.value(idsQ[i]);
														float v1 = valD4.value(idsQ[i]);
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (complex == QString("相位") || complex == QString("Phase"))
												{
													if (isD)
													{
														double v0 = valD1.value(idsQ[i]);
														double v1 = valD2.value(idsQ[i]);
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsQ[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valD3.value(idsQ[i]);
														float v1 = valD4.value(idsQ[i]);
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], valD1.value(idsQ[i]));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], valD3.value(idsQ[i]));
														yvaluesF.insert(cname, tempvec);
													}
												}

											}
										}
									}

									if (idsR != nullptr)
									{
									}
									if (valRealD1 != nullptr)
									{
									}
									if (valRealF1 != nullptr)
									{
									}
									if (valVirD1 != nullptr)
									{
									}
									if (valVirF1 != nullptr)
									{
									}

									if (idsQ != nullptr)
									{
									}
									if (valRealD2 != nullptr)
									{
									}
									if (valRealF2 != nullptr)
									{
									}
									if (valVirD2 != nullptr)
									{
									}
									if (valVirF2 != nullptr)
									{
									}
								}
								else if (QString(varName) == "Stress(N)")
								{
									//变量具体信息
									//实体个数
									int nums1 = static_cast<int>(lbIds1.size());
									QStringList componeName = _varStringList1.value(varName);
									int componeNums = componeName.size();

									int* idsR = lbIds1.data();
									double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
									double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
									if (nums1 > 0)
									{
										int* idsR = lbIds1.data();

										//获取单元节点总数
										int count1 = 0;
										for (int p = 0; p < nums1; ++p)
										{
											int rId = idsR[p];
											int partID = modelData->getPartIDByEleID(rId);
											QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                            int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                            int eleType = partData->_parteledata.cell_info[ele_cid].type;
											int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
											count1 += elenodeNums * componeNums;
										}

										//实部
										if (isD)
										{

										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									//壳单元
									int nums2 = static_cast<int>(lbIds2.size());

									int* idsQ = lbIds2.data();
									double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
									double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
									double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
									double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
									if (nums2 > 0)
									{
										int* idsQ = lbIds2.data();

										//获取单元节点总数
										int count2 = 0;
										for (int p = 0; p < nums2; ++p)
										{
											int rId = idsQ[p];
											int partID = modelData->getPartIDByEleID(rId);
											QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                            int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                            int eleType = partData->_parteledata.cell_info[ele_cid].type;
											int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
											count2 += elenodeNums * componeNums * 2;
										}

										//实部
										if (isD)
										{
										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									for (QString compo : compos)
									{
										//实体
										for (int i = 0; i < nums1; ++i)
										{
											if (ids.contains(idsR[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2_%3_%4-f%5").arg(var).arg(compo).arg(layer).arg(complex).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(compo);
												QHash<int, QHash<int, double>> valD1{};
												QHash<int, QHash<int, double>> valD2{};
												QHash<int, QHash<int, float>> valD3{};
												QHash<int, QHash<int, float>> valD4{};
												QVector<double> local{};

												if (isD)
												{
													valD1 = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, idsR, compoIndex, modelData, local);
													valD2 = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, idsR, compoIndex, modelData, local);
												}
												else
												{
													valD3 = toFloatHashFF(calcuEleNodeComponeValuesR(valRealF1, nums1, componeName, idsR, compoIndex, modelData, local));
													valD4 = toFloatHashFF(calcuEleNodeComponeValuesR(valVirF1, nums1, componeName, idsR, compoIndex, modelData, local));
												}

												if (complex == QString("虚部") || complex == QString("Img"))
												{
													if (isD)
													{
														QHash<int, double> ndata = valD2[idsR[i]];
														int nsize = ndata.size();
														double sum = 0;
														QHash<int, double>::const_iterator it;
														for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
														{
															sum += it.value();
														}
														double value = sum / nsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> ndata = valD4[idsR[i]];
														int nsize = ndata.size();
														float sum = 0;
														QHash<int, float>::const_iterator it;
														for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
														{
															sum += it.value();
														}
														float value = sum / nsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (complex == QString("幅值") || complex == QString("Amplitude"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsR[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;

														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsR[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (complex == QString("相位") || complex == QString("Phase"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsR[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsR[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsR[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														double value = rsum / rsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														float value = rsum / rsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}

											}
										}
										//壳
										for (int i = 0; i < nums2; ++i)
										{
											if (ids.contains(idsQ[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2_%3_%4-f%5").arg(var).arg(compo).arg(layer).arg(complex).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(compo);
												QHash<int, QHash<int, double>> valD1{};
												QHash<int, QHash<int, double>> valD2{};
												QHash<int, QHash<int, float>> valD3{};
												QHash<int, QHash<int, float>> valD4{};
												QVector<double> local;

												if (isD)
												{
													valD1 = calcuEleNodeComponeValuesQ(valRealD2, nums1, componeName, layer, idsQ, compoIndex, modelData, local);
													valD2 = calcuEleNodeComponeValuesQ(valVirD2, nums1, componeName, layer, idsQ, compoIndex, modelData, local);
												}
												else
												{
													valD3 = toFloatHashFF(calcuEleNodeComponeValuesQ(valRealF2, nums1, componeName, layer, idsQ, compoIndex, modelData, local));
													valD4 = toFloatHashFF(calcuEleNodeComponeValuesQ(valVirF2, nums1, componeName, layer, idsQ, compoIndex, modelData, local));
												}

												if (complex == QString("虚部") || complex == QString("Img"))
												{
													if (isD)
													{
														QHash<int, double> vdata = valD2[idsQ[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator it;
														for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
														{
															vsum += it.value();
														}
														double value = vsum / vsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> vdata = valD4[idsQ[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator it;
														for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
														{
															vsum += it.value();
														}
														float value = vsum / vsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (complex == QString("幅值") || complex == QString("Amplitude"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsQ[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsQ[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;

														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsQ[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsQ[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (complex == QString("相位") || complex == QString("Phase"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsQ[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsQ[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;

														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsQ[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsQ[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsQ[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														QHash<int, double> rdata = valD1[idsQ[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														double value = rsum / rsize;

														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsQ[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														float value = rsum / rsize;

														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}
											}
										}
									}

									if (idsR != nullptr)
									{
									}
									if (valRealD1 != nullptr)
									{
									}
									if (valRealF1 != nullptr)
									{
									}
									if (valVirD1 != nullptr)
									{
									}
									if (valVirF1 != nullptr)
									{
									}

									if (idsQ != nullptr)
									{
									}
									if (valRealD2 != nullptr)
									{
									}
									if (valRealF2 != nullptr)
									{
									}
									if (valVirD2 != nullptr)
									{
									}
									if (valVirF2 != nullptr)
									{
									}
								}
								else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress(N)")
								{
									//变量具体信息
									//实体个数
									int nums1 = static_cast<int>(lbIds1.size());
									QStringList componeName = _varStringList1.value(varName);
									int componeNums = componeName.size();

									int* idsR = lbIds1.data();
									double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
									double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
									if (nums1 > 0)
									{
										int* idsR = lbIds1.data();

										//获取单元节点总数
										int count1 = 0;
										for (int p = 0; p < nums1; ++p)
										{
											int rId = idsR[p];
											int partID = modelData->getPartIDByEleID(rId);
											QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                            int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                            int eleType = partData->_parteledata.cell_info[ele_cid].type;
											int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
											count1 += elenodeNums * componeNums;
										}

										//实部
										if (isD)
										{

										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									//壳单元
									int nums2 = static_cast<int>(lbIds2.size());

									for (QString compo : compos)
									{
										//实体
										for (int i = 0; i < nums1; ++i)
										{
											if (ids.contains(idsR[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2_%3_%4-f%5").arg(var).arg(compo).arg(layer).arg(complex).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(compo);
												QHash<int, QHash<int, double>> valD1{};
												QHash<int, QHash<int, double>> valD2{};
												QHash<int, QHash<int, float>> valD3{};
												QHash<int, QHash<int, float>> valD4{};
												QVector<double> local{};

												if (isD)
												{
													valD1 = calcuEleNodeComponeValues(valRealD1, nums1, componeName, idsR, compoIndex, modelData);
													valD2 = calcuEleNodeComponeValues(valVirD1, nums1, componeName, idsR, compoIndex, modelData);
												}
												else
												{
													valD3 = toFloatHashFF(calcuEleNodeComponeValues(valRealF1, nums1, componeName, idsR, compoIndex, modelData));
													valD4 = toFloatHashFF(calcuEleNodeComponeValues(valVirF1, nums1, componeName, idsR, compoIndex, modelData));
												}

												if (complex == QString("虚部") || complex == QString("Img"))
												{
													if (isD)
													{
														QHash<int, double> ndata = valD2[idsR[i]];
														int nsize = ndata.size();
														double sum = 0;
														QHash<int, double>::const_iterator it;
														for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
														{
															sum += it.value();
														}
														double value = sum / nsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> ndata = valD4[idsR[i]];
														int nsize = ndata.size();
														float sum = 0;
														QHash<int, float>::const_iterator it;
														for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
														{
															sum += it.value();
														}
														float value = sum / nsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (complex == QString("幅值") || complex == QString("Amplitude"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsR[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;

														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsR[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (complex == QString("相位") || complex == QString("Phase"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsR[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsR[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsR[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														double value = rsum / rsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														float value = rsum / rsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}

											}
										}
									}

									if (idsR != nullptr)
									{
									}
									if (valRealD1 != nullptr)
									{
									}
									if (valRealF1 != nullptr)
									{
									}
									if (valVirD1 != nullptr)
									{
									}
									if (valVirF1 != nullptr)
									{
									}
								}
								else
								{
									//变量具体信息
									//实体个数
									int nums1 = static_cast<int>(lbIds1.size());
									QStringList componeName = _varStringList1.value(varName);
									int componeNums = componeName.size();
									int count = componeNums * nums1;
									int* idsR = lbIds1.data();
									double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
									double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
									if (nums1 > 0)
									{
										int* idsR = lbIds1.data();
										//实部
										if (isD)
										{
										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									for (QString compo : compos)
									{
										//实体
										for (int i = 0; i < nums1; ++i)
										{
											if (ids.contains(idsR[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2_%3_%4-f%5").arg(var).arg(compo).arg(layer).arg(complex).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(compo);
												if (complex == QString("虚部") || complex == QString("Img"))
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], valVirD1[i * componeName.size() + compoIndex]);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], valVirF1[i * componeName.size() + compoIndex]);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (complex == QString("幅值") || complex == QString("Amplitude"))
												{
													if (isD)
													{
														double v0 = valRealD1[i * componeName.size() + compoIndex];
														double v1 = valVirD1[i * componeName.size() + compoIndex];
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valRealF1[i * componeName.size() + compoIndex];
														float v1 = valVirF1[i * componeName.size() + compoIndex];
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (complex == QString("相位") || complex == QString("Phase"))
												{
													if (isD)
													{
														double v0 = valRealD1[i * componeName.size() + compoIndex];
														double v1 = valVirD1[i * componeName.size() + compoIndex];
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
                                                            tempvec = yvaluesD.value(cname);  
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
														tempvec.insert(idsR[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valRealF1[i * componeName.size() + compoIndex];
														float v1 = valVirF1[i * componeName.size() + compoIndex];
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
                                                        const float PI = std::acos(-1);
                                                        float phase_rad = std::atan2(v1, v0);
                                                        float phase_deg = phase_rad * 180.0 / PI;
                                                        phase_deg = std::fmod(phase_deg, 360.0);
                                                        if (phase_deg < 0) {
                                                            phase_deg += 360.0;
                                                        }
                                                        if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                            phase_deg = 0.0;
                                                        }
														tempvec.insert(idsR[i], phase_deg);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
                                                        // 三个分量，且分量值没有在存储当中则为mag
                                                        if (componeNums == 3 && compoIndex == -1) 
                                                        {
                                                            int componeIndex = 0;
                                                            double x = valRealD1[i * componeNums + componeIndex];
                                                            double y = valRealD1[i * componeNums + componeIndex + 1];
                                                            double z = valRealD1[i * componeNums + componeIndex + 2];
                                                            if (yvaluesD.contains(cname))
                                                                tempvec = yvaluesD.value(cname);
                                                            tempvec.insert(idsR[i], sqrt(x * x + y * y + z * z));
                                                            yvaluesD.insert(cname, tempvec);
                                                        }
                                                        else
                                                        {
                                                            if (yvaluesD.contains(cname))
                                                                tempvec = yvaluesD.value(cname);
                                                            tempvec.insert(idsR[i], valRealD1[i * componeName.size() + compoIndex]);
                                                            yvaluesD.insert(cname, tempvec);
                                                        }
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
                                                        // 三个分量，且分量值没有在存储当中则为mag
                                                        if (componeNums == 3 && compoIndex == -1)
                                                        {
                                                            int componeIndex = 0;
                                                            double x = valRealF1[i * componeNums + componeIndex];
                                                            double y = valRealF1[i * componeNums + componeIndex + 1];
                                                            double z = valRealF1[i * componeNums + componeIndex + 2];
                                                            if (yvaluesF.contains(cname))
                                                                tempvec = yvaluesF.value(cname);
                                                            tempvec.insert(idsR[i], sqrt(x * x + y * y + z * z));
                                                            yvaluesF.insert(cname, tempvec);
                                                        }
                                                        else
                                                        {
                                                            if (yvaluesF.contains(cname))
                                                                tempvec = yvaluesF.value(cname);
                                                            tempvec.insert(idsR[i], valRealF1[i * componeName.size() + compoIndex]);
                                                            yvaluesF.insert(cname, tempvec);
                                                        }
													}
												}
											}
										}
									}
									if (idsR != nullptr)
									{
									}
									if (valRealD1 != nullptr)
									{
									}
									if (valRealF1 != nullptr)
									{
									}
									if (valVirD1 != nullptr)
									{
									}
									if (valVirF1 != nullptr)
									{
									}
									//壳单元
									int nums2 = static_cast<int>(lbIds2.size());
									/*int count2 = 16 * nums2;
									if (nums2 > 0)
									{
										int* ids = new int[nums2];
										//实部
										if (isD)
										{
											double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
										}
										else
										{
											double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
										}

										//虚部
										if (varType == 1)
										{
											if (isD)
											{
												double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
											}
											else
											{
												double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
											}
										}
									}*/
								}
							}
						}
					}
				}
			}
		}
	}

	void mMxdbThread::readTimeVarCurveData(const QString& pname, const QString& var, const QStringList& compos, const QString& layer, const QString& complex, const QVector<int>& ids, QStringList& cnames, QVector<double>& xvalues, QHash<QString, QVector<double>>& yvaluesD, QHash<QString, QVector<float>>& yvaluesF, QString caseName)
	{
		QFileInfo file(pname);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname); 
		if (outPutData == nullptr)
			return;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		if (modelDatabase.size() == 0)
			return;
		int nframe = modelDatabase.size();
		for (int i = 1; i <= nframe; ++i)
		{
			QPointer<mPostModelData> modelData = modelDatabase.value(i);
			if (modelData == nullptr)	return;
			PostFrameInfo info = modelDatabase.value(i)->_frameInfo;
			if (caseName.compare("All") == 0 || caseName.compare(info._frameCase) == 0)
			{
				int nvar = info._frameVars.size();
				for (int p = 0; p < nvar; ++p)
				{
					PostVariableInfo varInfo = info._frameVars.at(p);
					if (var == varInfo._varName)
					{
						bool isSuccess = openFileStatus(info._frameFile);
						if (!isSuccess)
							return;

                        bool isD = DataPrecisionIsDouble();

						//生成X数据
						xvalues.append(info._frameTime);

						//获取层索引
						int layerIndex = varInfo._varLayerNames.indexOf(layer);
						//获取当前变量的字节位置
						unsigned long long pos = varInfo._varByte;
						// 预读：块头（type+varName），值在各分支内按需读
						std::vector<int> lbIds1, lbIds2;
						std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
						int type = -1;
						std::string lbVarName;
						if (!readLegacyVarHead(pos, type, lbVarName))
							continue;

						const char* varName = lbVarName.c_str();
						if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
							|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
							|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
						{
							// 预读：实体块 + 壳块全部值
							if (!readLegacyVarValues(20, 16, type == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
								continue;
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();
							int count1 = 20 * nums1;
							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();
								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());
							int count2 = 16 * nums2;
							int* idsQ = lbIds2.data();
							double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
							if (nums2 > 0)
							{
								//idsQ = new int[nums2];
								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}

							for (QString compo : compos)
							{
								//实体
								for (int i = 0; i < nums1; ++i)
								{
									if (ids.contains(idsR[i]))
									{
										//曲线名称
										QString cname = QString("Time-%1_%2_%3_%4-ID%5").arg(var).arg(compo).arg(layer).arg(complex).arg(idsR[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, double> valD1{};
										QHash<int, double> valD2{};
										QHash<int, float> valD3{};
										QHash<int, float> valD4{};

										if (isD)
										{
											valD1 = calcuComponeValues(valRealD1, nums1, componeName, idsR, compoIndex);
											valD2 = calcuComponeValues(valVirD1, nums1, componeName, idsR, compoIndex);
										}
										else
										{
											valD3 = toFloatHashF(calcuComponeValues(valRealF1, nums1, componeName, idsR, compoIndex));
											valD4 = toFloatHashF(calcuComponeValues(valVirF1, nums1, componeName, idsR, compoIndex));
										}

										if (complex == QString("虚部") || complex == QString("Img"))
										{
											if (isD)
											{
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valD2.value(idsR[i]));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valD4.value(idsR[i]));
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complex == QString("幅值") || complex == QString("Amplitude"))
										{
											if (isD)
											{
												double v0 = valD1.value(idsR[i]);
												double v1 = valD2.value(idsR[i]);
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valD3.value(idsR[i]);
												float v1 = valD4.value(idsR[i]);
												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complex == QString("相位") || complex == QString("Phase"))
										{
											if (isD)
											{
												double v0 = valD1.value(idsR[i]);
												double v1 = valD2.value(idsR[i]);
												//存储

												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);

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
												tempvec.append(phase_deg);

												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valD3.value(idsR[i]);
												float v1 = valD4.value(idsR[i]);
												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);

                                                const float PI = std::acos(-1);

                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }

												tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valD1.value(idsR[i]));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valD3.value(idsR[i]));
												yvaluesF.insert(cname, tempvec);
											}
										}

									}
								}
								//壳
								for (int i = 0; i < nums2; ++i)
								{
									if (ids.contains(idsQ[i]))
									{
										//曲线名称
										QString cname = QString("Time-%1_%2_%3_%4-ID%5").arg(var).arg(compo).arg(layer).arg(complex).arg(idsQ[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, double> valD1{};
										QHash<int, double> valD2{};
										QHash<int, float> valD3{};
										QHash<int, float> valD4{};

										if (isD)
										{
											valD1 = calcuComponeValues1(valRealD2, nums2, componeName, layer, idsQ, compoIndex);
											valD2 = calcuComponeValues1(valVirD2, nums2, componeName, layer, idsQ, compoIndex);
										}
										else
										{
											valD3 = toFloatHashF(calcuComponeValues1(valRealF2, nums2, componeName, layer, idsQ, compoIndex));
											valD4 = toFloatHashF(calcuComponeValues1(valVirF2, nums2, componeName, layer, idsQ, compoIndex));
										}

										if (complex == QString("虚部") || complex == QString("Img"))
										{
											if (isD)
											{
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valD2.value(idsQ[i]));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valD4.value(idsQ[i]));
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complex == QString("幅值") || complex == QString("Amplitude"))
										{
											if (isD)
											{
												double v0 = valD1.value(idsQ[i]);
												double v1 = valD2.value(idsQ[i]);
												//存储
												QVector<double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valD3.value(idsQ[i]);
												float v1 = valD4.value(idsQ[i]);
												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complex == QString("相位") || complex == QString("Phase"))
										{
											if (isD)
											{
												double v0 = valD1.value(idsQ[i]);
												double v1 = valD2.value(idsQ[i]);
												//存储
												QVector<double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);

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

												tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valD3.value(idsQ[i]);
												float v1 = valD4.value(idsQ[i]);
												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);

                                                const float PI = std::acos(-1);

                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }
												tempvec.append(phase_deg);

												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valD1.value(idsQ[i]));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valD3.value(idsQ[i]));
												yvaluesF.insert(cname, tempvec);
											}
										}

									}
								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}

							if (idsQ != nullptr)
							{
							}
							if (valRealD2 != nullptr)
							{
							}
							if (valRealF2 != nullptr)
							{
							}
							if (valVirD2 != nullptr)
							{
							}
							if (valVirF2 != nullptr)
							{
							}
						}
						else if (QString(varName) == "Stress(N)")
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();

							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();

								//获取单元节点总数
								int count1 = 0;
								for (int p = 0; p < nums1; ++p)
								{
									int rId = idsR[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count1 += elenodeNums * componeNums;
								}

								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());

							int* idsQ = lbIds2.data();
							double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
							if (nums2 > 0)
							{
								int* idsQ = lbIds2.data();

								//获取单元节点总数
								int count2 = 0;
								for (int p = 0; p < nums2; ++p)
								{
									int rId = idsQ[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count2 += elenodeNums * componeNums * 2;
								}

								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							
							for (QString compo : compos)
							{
								//实体
								for (int i = 0; i < nums1; ++i)
								{
									if (ids.contains(idsR[i]))
									{
										//曲线名称
										QString cname = QString("Time-%1_%2_%3_%4-ID%5").arg(var).arg(compo).arg(layer).arg(complex).arg(idsR[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, QHash<int, double>> valD1{};
										QHash<int, QHash<int, double>> valD2{};
										QHash<int, QHash<int, float>> valD3{};
										QHash<int, QHash<int, float>> valD4{};
										QVector<double> local{};

										if (isD)
										{
											valD1 = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, idsR, compoIndex, modelData, local);
											valD2 = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, idsR, compoIndex, modelData, local);
										}
										else
										{
											valD3 = toFloatHashFF(calcuEleNodeComponeValuesR(valRealF1, nums1, componeName, idsR, compoIndex, modelData, local));
											valD4 = toFloatHashFF(calcuEleNodeComponeValuesR(valVirF1, nums1, componeName, idsR, compoIndex, modelData, local));
										}

										if (complex == QString("虚部") || complex == QString("Img"))
										{
											if (isD)
											{
												QHash<int, double> ndata = valD2[idsR[i]];
												int nsize = ndata.size();
												double sum = 0;
												QHash<int, double>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												double value = sum / nsize;

												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> ndata = valD4[idsR[i]];
												int nsize = ndata.size();
												double sum = 0;
												QHash<int, float>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												double value = sum / nsize;
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complex == QString("幅值") || complex == QString("Amplitude"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsR[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;
												
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsR[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;

												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complex == QString("相位") || complex == QString("Phase"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsR[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);

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

                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsR[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;

												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);

                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }

                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator it;
												for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
												{
													rsum += it.value();
												}
												double value = rsum / rsize;

												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator it;
												for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
												{
													rsum += it.value();
												}
												float value = rsum / rsize;

												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}

									}
								}
								//壳
								for (int i = 0; i < nums2; ++i)
								{
									if (ids.contains(idsQ[i]))
									{
										//曲线名称
										QString cname = QString("Time-%1_%2_%3_%4-ID%5").arg(var).arg(compo).arg(layer).arg(complex).arg(idsQ[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, QHash<int, double>> valD1{};
										QHash<int, QHash<int, double>> valD2{};
										QHash<int, QHash<int, float>> valD3{};
										QHash<int, QHash<int, float>> valD4{};
										QVector<double> local;

										if (isD)
										{
											valD1 = calcuEleNodeComponeValuesQ(valRealD2, nums2, componeName, layer, idsQ, compoIndex, modelData, local);
											valD2 = calcuEleNodeComponeValuesQ(valVirD2, nums2, componeName, layer, idsQ, compoIndex, modelData, local);
										}
										else
										{
											valD3 = toFloatHashFF(calcuEleNodeComponeValuesQ(valRealF2, nums2, componeName, layer, idsQ, compoIndex, modelData, local));
											valD4 = toFloatHashFF(calcuEleNodeComponeValuesQ(valVirF2, nums2, componeName, layer, idsQ, compoIndex, modelData, local));
										}

										if (complex == QString("虚部") || complex == QString("Img"))
										{
											if (isD)
											{
												QHash<int, double> vdata = valD2[idsQ[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator it;
												for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
												{
													vsum += it.value();
												}
												double value = vsum / vsize;
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> vdata = valD4[idsQ[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator it;
												for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
												{
													vsum += it.value();
												}

												float value = vsum / vsize;
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complex == QString("幅值") || complex == QString("Amplitude"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsQ[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsQ[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector<double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsQ[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsQ[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;
												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complex == QString("相位") || complex == QString("Phase"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsQ[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsQ[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;
												//存储
												QVector<double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);

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

                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsQ[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsQ[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;
												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);

                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }

                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												QHash<int, double> rdata = valD1[idsQ[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator it;
												for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
												{
													rsum += it.value();
												}
												double value = rsum / rsize;
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsQ[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator it;
												for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
												{
													rsum += it.value();
												}
												float value = rsum / rsize;
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}

									}
								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}

							if (idsQ != nullptr)
							{
							}
							if (valRealD2 != nullptr)
							{
							}
							if (valRealF2 != nullptr)
							{
							}
							if (valVirD2 != nullptr)
							{
							}
							if (valVirF2 != nullptr)
							{
							}
						}
						else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress")
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();

							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();

								//获取单元节点总数
								int count1 = 0;
								for (int p = 0; p < nums1; ++p)
								{
									int rId = idsR[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count1 += elenodeNums * componeNums;
								}

								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());

							for (QString compo : compos)
							{
								//实体
								for (int i = 0; i < nums1; ++i)
								{
									if (ids.contains(idsR[i]))
									{
										//曲线名称
										QString cname = QString("Time-%1_%2_%3_%4-ID%5").arg(var).arg(compo).arg(layer).arg(complex).arg(idsR[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, QHash<int, double>> valD1{};
										QHash<int, QHash<int, double>> valD2{};
										QHash<int, QHash<int, float>> valD3{};
										QHash<int, QHash<int, float>> valD4{};
										QVector<double> local{};

										if (isD)
										{
											valD1 = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, idsR, compoIndex, modelData, local);
											valD2 = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, idsR, compoIndex, modelData, local);
										}
										else
										{
											valD3 = toFloatHashFF(calcuEleNodeComponeValuesR(valRealF1, nums1, componeName, idsR, compoIndex, modelData, local));
											valD4 = toFloatHashFF(calcuEleNodeComponeValuesR(valVirF1, nums1, componeName, idsR, compoIndex, modelData, local));
										}

										if (complex == QString("虚部") || complex == QString("Img"))
										{
											if (isD)
											{
												QHash<int, double> ndata = valD2[idsR[i]];
												int nsize = ndata.size();
												double sum = 0;
												QHash<int, double>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												double value = sum / nsize;

												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> ndata = valD4[idsR[i]];
												int nsize = ndata.size();
												double sum = 0;
												QHash<int, float>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												double value = sum / nsize;
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complex == QString("幅值") || complex == QString("Amplitude"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsR[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsR[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;

												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complex == QString("相位") || complex == QString("Phase"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsR[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
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

                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsR[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;

												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);

                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }

                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator it;
												for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
												{
													rsum += it.value();
												}
												double value = rsum / rsize;

												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator it;
												for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
												{
													rsum += it.value();
												}
												float value = rsum / rsize;

												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}

									}
								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}
						}
						else
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();
							int count = componeNums * nums1;
							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();
								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							for (QString compo : compos)
							{
								//实体
								for (int i = 0; i < nums1; ++i)
								{
									if (ids.contains(idsR[i]))
									{
										//曲线名称
										QString cname = QString("Time-%1_%2_%3_%4-ID%5").arg(var).arg(compo).arg(layer).arg(complex).arg(idsR[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										if (complex == QString("虚部") || complex == QString("Img"))
										{
											if (isD)
											{
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valVirD1[i * componeName.size() + compoIndex]);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valVirF1[i * componeName.size() + compoIndex]);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complex == QString("幅值") || complex == QString("Amplitude"))
										{
											if (isD)
											{
												double v0 = valRealD1[i * componeName.size() + compoIndex];
												double v1 = valVirD1[i * componeName.size() + compoIndex];
												//存储
												QVector<double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valRealF1[i * componeName.size() + compoIndex];
												float v1 = valVirF1[i * componeName.size() + compoIndex];
												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complex == QString("相位") || complex == QString("Phase"))
										{
											if (isD)
											{
												double v0 = valRealD1[i * componeName.size() + compoIndex];
												double v1 = valVirD1[i * componeName.size() + compoIndex];
												//存储
												QVector<double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
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

                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valRealF1[i * componeName.size() + compoIndex];
												float v1 = valVirF1[i * componeName.size() + compoIndex];
												//存储
												QVector<float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);

                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }

                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valRealD1[i * componeName.size() + compoIndex]);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valRealF1[i * componeName.size() + compoIndex]);
												yvaluesF.insert(cname, tempvec);
											}
										}
									}
								}
							}
							if (idsR != nullptr)
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}
						}
					}
				}
			}
		}
	}

	void mMxdbThread::readVarVarCurveData(const QString& pname, const QString& varx, const QString& compox, const QString& layerx, const QString& complexx, const QString& vary, const QStringList& composy, const QString& layery, const QString& complexy, const QVector<int>& ids, QStringList& cnames, QVector<double>& xvaluesD, QVector<float>& xvaluesF, QHash<QString, QVector<double>>& yvaluesD, QHash<QString, QVector<float>>& yvaluesF, QString caseName)
	{
		QFileInfo file(pname);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
		if (outPutData == nullptr)
			return;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		if (modelDatabase.size() == 0)
			return;
		int nframe = modelDatabase.size();
		for (int i = 1; i <= nframe; ++i)
		{
			QPointer<mPostModelData> modelData = modelDatabase.value(i);
			if (modelData == nullptr)	return;
			PostFrameInfo info = modelDatabase.value(i)->_frameInfo;
			if (caseName.compare("All") == 0 || caseName.compare(info._frameCase) == 0)
			{
				int nvar = info._frameVars.size();
				for (int p = 0; p < nvar; ++p)
				{
					PostVariableInfo varInfo = info._frameVars.at(p);

					//X值
					if (varx == varInfo._varName)
					{
						bool isSuccess = openFileStatus(info._frameFile);
						if (!isSuccess)
							return;

                        bool isD = DataPrecisionIsDouble();

						//获取层索引
						int layerIndex = varInfo._varLayerNames.indexOf(layerx);
						//获取当前变量的字节位置
						unsigned long long pos = varInfo._varByte;
						// 预读：块头（type+varName），值在各分支内按需读
						std::vector<int> lbIds1, lbIds2;
						std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
						int type = -1;
						std::string lbVarName;
						if (!readLegacyVarHead(pos, type, lbVarName))
							continue;

						const char* varName = lbVarName.c_str();
						if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
							|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
							|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
						{
							// 预读：实体块 + 壳块全部值
							if (!readLegacyVarValues(20, 16, type == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
								continue;
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();
							int count1 = 20 * nums1;
							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();
								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());
							int count2 = 16 * nums2;
							int* idsQ = lbIds2.data();
							double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
							if (nums2 > 0)
							{
								int* idsQ = lbIds2.data();
								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}

							//实体
							for (int i = 0; i < nums1; ++i)
							{
								if (ids.contains(idsR[i]))
								{
									//获取当前分量索引
									int compoIndex = componeName.indexOf(compox);
									QHash<int, double> valD1{};
									QHash<int, double> valD2{};
									QHash<int, float> valD3{};
									QHash<int, float> valD4{};

									if (isD)
									{
										valD1 = calcuComponeValues(valRealD1, nums1, componeName, idsR, compoIndex);
										valD2 = calcuComponeValues(valVirD1, nums1, componeName, idsR, compoIndex);
									}
									else
									{
										valD3 = toFloatHashF(calcuComponeValues(valRealF1, nums1, componeName, idsR, compoIndex));
										valD4 = toFloatHashF(calcuComponeValues(valVirF1, nums1, componeName, idsR, compoIndex));
									}

									if (complexx == QString("虚部") || complexx == QString("Img"))
									{
										if (isD)
										{
											xvaluesD.append(valD2.value(idsR[i]));
										}
										else
										{
											xvaluesF.append(valD4.value(idsR[i]));
										}
									}
									else if (complexx == QString("幅值") || complexx == QString("Amplitude"))
									{
										if (isD)
										{
											double v0 = valD1.value(idsR[i]);
											double v1 = valD2.value(idsR[i]);
											xvaluesD.append(sqrt(v0* v0 + v1 * v1));
										}
										else
										{
											float v0 = valD3.value(idsR[i]);
											float v1 = valD4.value(idsR[i]);
											xvaluesF.append(sqrt(v0* v0 + v1 * v1));
										}

									}
									else if (complexx == QString("相位") || complexx == QString("Phase"))
									{
										if (isD)
										{
											double v0 = valD1.value(idsR[i]);
											double v1 = valD2.value(idsR[i]);
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
											xvaluesD.append(phase_deg);
										}
										else
										{
											float v0 = valD3.value(idsR[i]);
											float v1 = valD4.value(idsR[i]);
                                            const float PI = std::acos(-1);

                                            float phase_rad = std::atan2(v1, v0);
                                            float phase_deg = phase_rad * 180.0 / PI;
                                            phase_deg = std::fmod(phase_deg, 360.0);
                                            if (phase_deg < 0) {
                                                phase_deg += 360.0;
                                            }
                                            if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                phase_deg = 0.0;
                                            }
											xvaluesF.append(phase_deg);
										}
									}
									else
									{
										if (isD)
										{
											xvaluesD.append(valD1.value(idsR[i]));
										}
										else
										{
											xvaluesF.append(valD3.value(idsR[i]));
										}
									}

								}
							}
							//壳
							for (int i = 0; i < nums2; ++i)
							{
								if (ids.contains(idsQ[i]))
								{
									//获取当前分量索引
									int compoIndex = componeName.indexOf(compox);
									QHash<int, double> valD1{};
									QHash<int, double> valD2{};
									QHash<int, float> valD3{};
									QHash<int, float> valD4{};

									if (isD)
									{
										valD1 = calcuComponeValues1(valRealD2, nums1, componeName, layerx, idsQ, compoIndex);
										valD2 = calcuComponeValues1(valVirD2, nums1, componeName, layerx, idsQ, compoIndex);
									}
									else
									{
										valD3 = toFloatHashF(calcuComponeValues1(valRealF2, nums1, componeName, layerx, idsQ, compoIndex));
										valD4 = toFloatHashF(calcuComponeValues1(valVirF2, nums1, componeName, layerx, idsQ, compoIndex));
									}

									if (complexx == QString("虚部") || complexx == QString("Img"))
									{
										if (isD)
										{
											xvaluesD.append(valD2.value(idsQ[i]));
										}
										else
										{
											xvaluesF.append(valD4.value(idsQ[i]));
										}
									}
									else if (complexx == QString("幅值") || complexx == QString("Amplitude"))
									{
										if (isD)
										{
											double v0 = valD1.value(idsQ[i]);
											double v1 = valD2.value(idsQ[i]);
											xvaluesD.append(sqrt(v0* v0 + v1 * v1));
										}
										else
										{
											float v0 = valD3.value(idsQ[i]);
											float v1 = valD4.value(idsQ[i]);
											xvaluesF.append(sqrt(v0* v0 + v1 * v1));
										}

									}
									else if (complexx == QString("相位") || complexx == QString("Phase"))
									{
										if (isD)
										{
											double v0 = valD1.value(idsQ[i]);
											double v1 = valD2.value(idsQ[i]);

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
											xvaluesD.append(phase_deg);
										}
										else
										{
											float v0 = valD3.value(idsQ[i]);
											float v1 = valD4.value(idsQ[i]);

                                            const float PI = std::acos(-1);

                                            float phase_rad = std::atan2(v1, v0);
                                            float phase_deg = phase_rad * 180.0 / PI;
                                            phase_deg = std::fmod(phase_deg, 360.0);
                                            if (phase_deg < 0) {
                                                phase_deg += 360.0;
                                            }
                                            if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                phase_deg = 0.0;
                                            }
											xvaluesF.append(phase_deg);
										}
									}
									else
									{
										if (isD)
										{
											xvaluesD.append(valD1.value(idsQ[i]));
										}
										else
										{
											xvaluesF.append(valD3.value(idsQ[i]));
										}
									}

								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}

							if (idsQ != nullptr)
							{
							}
							if (valRealD2 != nullptr)
							{
							}
							if (valRealF2 != nullptr)
							{
							}
							if (valVirD2 != nullptr)
							{
							}
							if (valVirF2 != nullptr)
							{
							}
						}
						else if (QString(varName) == "Stress(N)")
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();

							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();

								//获取单元节点总数
								int count1 = 0;
								for (int p = 0; p < nums1; ++p)
								{
									int rId = idsR[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count1 += elenodeNums * componeNums;
								}

								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());

							int* idsQ = lbIds2.data();
							double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
							if (nums2 > 0)
							{
								int* idsQ = lbIds2.data();

								//获取单元节点总数
								int count2 = 0;
								for (int p = 0; p < nums2; ++p)
								{
									int rId = idsQ[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count2 += elenodeNums * componeNums * 2;
								}

								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}

							//实体
							for (int i = 0; i < nums1; ++i)
							{
								if (ids.contains(idsR[i]))
								{
									//获取当前分量索引
									int compoIndex = componeName.indexOf(compox);
									QHash<int, QHash<int, double>> valD1{};
									QHash<int, QHash<int, double>> valD2{};
									QHash<int, QHash<int, float>> valD3{};
									QHash<int, QHash<int, float>> valD4{};
									QVector<double> local{};

									if (isD)
									{
										valD1 = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, idsR, compoIndex, modelData, local);
										valD2 = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, idsR, compoIndex, modelData, local);
									}
									else
									{
										valD3 = toFloatHashFF(calcuEleNodeComponeValuesR(valRealF1, nums1, componeName, idsR, compoIndex, modelData, local));
										valD4 = toFloatHashFF(calcuEleNodeComponeValuesR(valVirF1, nums1, componeName, idsR, compoIndex, modelData, local));
									}

									if (complexx == QString("虚部") || complexx == QString("Img"))
									{
										if (isD)
										{
											QHash<int, double> ndata = valD2[idsR[i]];
											int nsize = ndata.size();
											double sum = 0;
											QHash<int, double>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											double xvalue = sum / nsize;
											xvaluesD.append(xvalue);
										}
										else
										{
											QHash<int, float> ndata = valD4[idsR[i]];
											int nsize = ndata.size();
											float sum = 0;
											QHash<int, float>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											float xvalue = sum / nsize;
											xvaluesF.append(xvalue);
										}
									}
									else if (complexx == QString("幅值") || complexx == QString("Amplitude"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsR[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;
											xvaluesD.append(sqrt(v0 * v0 + v1 * v1));
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsR[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;
											xvaluesF.append(sqrt(v0 * v0 + v1 * v1));
										}

									}
									else if (complexx == QString("相位") || complexx == QString("Phase"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsR[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;

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
											xvaluesD.append(phase_deg);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsR[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;

                                            const float PI = std::acos(-1);

                                            float phase_rad = std::atan2(v1, v0);
                                            float phase_deg = phase_rad * 180.0 / PI;
                                            phase_deg = std::fmod(phase_deg, 360.0);
                                            if (phase_deg < 0) {
                                                phase_deg += 360.0;
                                            }
                                            if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                phase_deg = 0.0;
                                            }
											xvaluesF.append(phase_deg);
										}
									}
									else
									{
										if (isD)
										{
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											double value = rsum / rsize;
											xvaluesD.append(value);
										}
										else
										{
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											float value = rsum / rsize;
											xvaluesF.append(value);
										}
									}

								}
							}
							//壳
							for (int i = 0; i < nums2; ++i)
							{
								if (ids.contains(idsQ[i]))
								{
									//获取当前分量索引
									int compoIndex = componeName.indexOf(compox);
									QHash<int, QHash<int, double>> valD1{};
									QHash<int, QHash<int, double>> valD2{};
									QHash<int, QHash<int, float>> valD3{};
									QHash<int, QHash<int, float>> valD4{};

									QVector<double> local;
									if (isD)
									{
										valD1 = calcuEleNodeComponeValuesQ(valRealD2, nums1, componeName, layerx, idsQ, compoIndex, modelData, local);
										valD2 = calcuEleNodeComponeValuesQ(valVirD2, nums1, componeName, layerx, idsQ, compoIndex, modelData, local);
									}
									else
									{
										valD3 = toFloatHashFF(calcuEleNodeComponeValuesQ(valRealF2, nums1, componeName, layerx, idsQ, compoIndex, modelData, local));
										valD4 = toFloatHashFF(calcuEleNodeComponeValuesQ(valVirF2, nums1, componeName, layerx, idsQ, compoIndex, modelData, local));
									}

									if (complexx == QString("虚部") || complexx == QString("Img"))
									{
										if (isD)
										{
											QHash<int, double> vdata = valD2[idsQ[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator it;
											for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
											{
												vsum += it.value();
											}
											double value = vsum / vsize;
											xvaluesD.append(value);
										}
										else
										{
											QHash<int, float> vdata = valD4[idsQ[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator it;
											for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
											{
												vsum += it.value();
											}
											float value = vsum / vsize;
											xvaluesF.append(value);
										}
									}
									else if (complexx == QString("幅值") || complexx == QString("Amplitude"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsQ[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsQ[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;
											xvaluesD.append(sqrt(v0 * v0 + v1 * v1));
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsQ[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsQ[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;
											xvaluesF.append(sqrt(v0 * v0 + v1 * v1));
										}

									}
									else if (complexx == QString("相位") || complexx == QString("Phase"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsQ[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsQ[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;

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
											xvaluesD.append(phase_deg);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsQ[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsQ[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;

                                            const float PI = std::acos(-1);

                                            float phase_rad = std::atan2(v1, v0);
                                            float phase_deg = phase_rad * 180.0 / PI;
                                            phase_deg = std::fmod(phase_deg, 360.0);
                                            if (phase_deg < 0) {
                                                phase_deg += 360.0;
                                            }
                                            if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                phase_deg = 0.0;
                                            }
											xvaluesF.append(phase_deg);
										}
									}
									else
									{
										if (isD)
										{
											QHash<int, double> rdata = valD1[idsQ[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											double value = rsum / rsize;
											xvaluesD.append(value);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsQ[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											float value = rsum / rsize;
											xvaluesF.append(value);
										}
									}

								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}

							if (idsQ != nullptr)
							{
							}
							if (valRealD2 != nullptr)
							{
							}
							if (valRealF2 != nullptr)
							{
							}
							if (valVirD2 != nullptr)
							{
							}
							if (valVirF2 != nullptr)
							{
							}
						}
						else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress(N)")
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();

							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();

								//获取单元节点总数
								int count1 = 0;
								for (int p = 0; p < nums1; ++p)
								{
									int rId = idsR[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count1 += elenodeNums * componeNums;
								}

								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());

							//实体
							for (int i = 0; i < nums1; ++i)
							{
								if (ids.contains(idsR[i]))
								{
									//获取当前分量索引
									int compoIndex = componeName.indexOf(compox);
									QHash<int, QHash<int, double>> valD1{};
									QHash<int, QHash<int, double>> valD2{};
									QHash<int, QHash<int, float>> valD3{};
									QHash<int, QHash<int, float>> valD4{};
									QVector<double> local{};

									if (isD)
									{
										valD1 = calcuEleNodeComponeValues(valRealD1, nums1, componeName, idsR, compoIndex, modelData);
										valD2 = calcuEleNodeComponeValues(valVirD1, nums1, componeName, idsR, compoIndex, modelData);
									}
									else
									{
										valD3 = toFloatHashFF(calcuEleNodeComponeValues(valRealF1, nums1, componeName, idsR, compoIndex, modelData));
										valD4 = toFloatHashFF(calcuEleNodeComponeValues(valVirF1, nums1, componeName, idsR, compoIndex, modelData));
									}

									if (complexx == QString("虚部") || complexx == QString("Img"))
									{
										if (isD)
										{
											QHash<int, double> ndata = valD2[idsR[i]];
											int nsize = ndata.size();
											double sum = 0;
											QHash<int, double>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											double xvalue = sum / nsize;
											xvaluesD.append(xvalue);
										}
										else
										{
											QHash<int, float> ndata = valD4[idsR[i]];
											int nsize = ndata.size();
											float sum = 0;
											QHash<int, float>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											float xvalue = sum / nsize;
											xvaluesF.append(xvalue);
										}
									}
									else if (complexx == QString("幅值") || complexx == QString("Amplitude"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsR[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;
											xvaluesD.append(sqrt(v0 * v0 + v1 * v1));
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsR[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;
											xvaluesF.append(sqrt(v0 * v0 + v1 * v1));
										}

									}
									else if (complexx == QString("相位") || complexx == QString("Phase"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsR[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;

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
											xvaluesD.append(phase_deg);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsR[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;

                                            const float PI = std::acos(-1);
                                            float phase_rad = std::atan2(v1, v0);
                                            float phase_deg = phase_rad * 180.0 / PI;
                                            phase_deg = std::fmod(phase_deg, 360.0);
                                            if (phase_deg < 0) {
                                                phase_deg += 360.0;
                                            }
                                            if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                phase_deg = 0.0;
                                            }
											xvaluesF.append(phase_deg);
										}
									}
									else
									{
										if (isD)
										{
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											double value = rsum / rsize;
											xvaluesD.append(value);
										}
										else
										{
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											float value = rsum / rsize;
											xvaluesF.append(value);
										}
									}

								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}
						}
						else
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();
							int count = componeNums * nums1;
							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();
								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//实体
							for (int i = 0; i < nums1; ++i)
							{
								if (ids.contains(idsR[i]))
								{
									//获取当前分量索引
									int compoIndex = componeName.indexOf(compox);
									if (complexx == QString("虚部") || complexx == QString("Img"))
									{
										if (isD)
										{
											xvaluesD.append(valVirD1[i * componeName.size() + compoIndex]);
										}
										else
										{
											xvaluesF.append(valVirF1[i * componeName.size() + compoIndex]);
										}
									}
									else if (complexx == QString("幅值") || complexx == QString("Amplitude"))
									{
										if (isD)
										{
											double v0 = valRealD1[i * componeName.size() + compoIndex];
											double v1 = valVirD1[i * componeName.size() + compoIndex];
											xvaluesD.append(sqrt(v0* v0 + v1 * v1));
										}
										else
										{
											float v0 = valRealF1[i * componeName.size() + compoIndex];
											float v1 = valVirF1[i * componeName.size() + compoIndex];
											xvaluesF.append(sqrt(v0* v0 + v1 * v1));
										}

									}
									else if (complexx == QString("相位") || complexx == QString("Phase"))
									{
										if (isD)
										{
											double v0 = valRealD1[i * componeName.size() + compoIndex];
											double v1 = valVirD1[i * componeName.size() + compoIndex];
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
											xvaluesD.append(phase_deg);
										}
										else
										{
											float v0 = valRealF1[i * componeName.size() + compoIndex];
											float v1 = valVirF1[i * componeName.size() + compoIndex];
                                            const float PI = std::acos(-1);
                                            float phase_rad = std::atan2(v1, v0);
                                            float phase_deg = phase_rad * 180.0 / PI;
                                            phase_deg = std::fmod(phase_deg, 360.0);
                                            if (phase_deg < 0) {
                                                phase_deg += 360.0;
                                            }
                                            if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                phase_deg = 0.0;
                                            }
											xvaluesF.append(phase_deg);
										}
									}
									else
									{
										if (isD)
										{
											xvaluesD.append(valRealD1[i * componeName.size() + compoIndex]);
										}
										else
										{
											xvaluesF.append(valRealF1[i * componeName.size() + compoIndex]);
										}
									}
								}
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}
						}
					}
					if (vary == varInfo._varName)
					{
						bool isSuccess = openFileStatus(info._frameFile);
						if (!isSuccess)
							return;

                        bool isD = DataPrecisionIsDouble();

						//获取层索引
						int layerIndex = varInfo._varLayerNames.indexOf(layery);
						//获取当前变量的字节位置
						unsigned long long pos = varInfo._varByte;
						// 预读：块头（type+varName），值在各分支内按需读
						std::vector<int> lbIds1, lbIds2;
						std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
						int type = -1;
						std::string lbVarName;
						if (!readLegacyVarHead(pos, type, lbVarName))
							continue;

						const char* varName = lbVarName.c_str();
						if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
							|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
							|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
						{
							// 预读：实体块 + 壳块全部值
							if (!readLegacyVarValues(20, 16, type == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
								continue;
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();
							int count1 = 20 * nums1;
							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();
								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());
							int count2 = 16 * nums2;
							int* idsQ = lbIds2.data();
							double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
							if (nums2 > 0)
							{
								int* idsQ = lbIds2.data();
								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}

							for (QString compo : composy)
							{
								//实体
								for (int i = 0; i < nums1; ++i)
								{
									if (ids.contains(idsR[i]))
									{
										//曲线名称
										QString cname = QString("%1_%2_%3_%4-%5_%6_%7_%8-ID%9").arg(varx).arg(compox).arg(layerx).arg(complexx)
											.arg(vary).arg(compo).arg(layery).arg(complexy).arg(idsR[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, double> valD1{};
										QHash<int, double> valD2{};
										QHash<int, float> valD3{};
										QHash<int, float> valD4{};

										if (isD)
										{
											valD1 = calcuComponeValues(valRealD1, nums1, componeName, idsR, compoIndex);
											valD2 = calcuComponeValues(valVirD1, nums1, componeName, idsR, compoIndex);
										}
										else
										{
											valD3 = toFloatHashF(calcuComponeValues(valRealF1, nums1, componeName, idsR, compoIndex));
											valD4 = toFloatHashF(calcuComponeValues(valVirF1, nums1, componeName, idsR, compoIndex));
										}

										if (complexy == QString("虚部") || complexy == QString("Img"))
										{
											if (isD)
											{
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valD2.value(idsR[i]));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valD4.value(idsR[i]));
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complexy == QString("幅值") || complexy == QString("Amplitude"))
										{
											if (isD)
											{
												double v0 = valD1.value(idsR[i]);
												double v1 = valD2.value(idsR[i]);
												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valD3.value(idsR[i]);
												float v1 = valD4.value(idsR[i]);
												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complexy == QString("相位") || complexy == QString("Phase"))
										{
											if (isD)
											{
												double v0 = valD1.value(idsR[i]);
												double v1 = valD2.value(idsR[i]);
												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
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
												tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valD3.value(idsR[i]);
												float v1 = valD4.value(idsR[i]);
												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);
                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }
                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valD1.value(idsR[i]));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valD3.value(idsR[i]));
												yvaluesF.insert(cname, tempvec);
											}
										}

									}
								}
								//壳
								for (int i = 0; i < nums2; ++i)
								{
									if (ids.contains(idsQ[i]))
									{
										//曲线名称
										QString cname = QString("%1_%2_%3_%4-%5_%6_%7_%8-ID%9").arg(varx).arg(compox).arg(layerx).arg(complexx)
											.arg(vary).arg(compo).arg(layery).arg(complexy).arg(idsQ[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, double> valD1{};
										QHash<int, double> valD2{};
										QHash<int, float> valD3{};
										QHash<int, float> valD4{};

										if (isD)
										{
											valD1 = calcuComponeValues1(valRealD2, nums1, componeName, layery, idsQ, compoIndex);
											valD2 = calcuComponeValues1(valVirD2, nums1, componeName, layery, idsQ, compoIndex);
										}
										else
										{
											valD3 = toFloatHashF(calcuComponeValues1(valRealF2, nums1, componeName, layery, idsQ, compoIndex));
											valD4 = toFloatHashF(calcuComponeValues1(valVirF2, nums1, componeName, layery, idsQ, compoIndex));
										}

										if (complexy == QString("虚部") || complexy == QString("Img"))
										{
											if (isD)
											{
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valD2.value(idsQ[i]));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valD4.value(idsQ[i]));
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complexy == QString("幅值") || complexy == QString("Amplitude"))
										{
											if (isD)
											{
												double v0 = valD1.value(idsQ[i]);
												double v1 = valD2.value(idsQ[i]);
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valD3.value(idsQ[i]);
												float v1 = valD4.value(idsQ[i]);
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complexy == QString("相位") || complexy == QString("Phase"))
										{
											if (isD)
											{
												double v0 = valD1.value(idsQ[i]);
												double v1 = valD2.value(idsQ[i]);
												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
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
                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valD3.value(idsQ[i]);
												float v1 = valD4.value(idsQ[i]);
												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);
                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }
                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valD1.value(idsQ[i]));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valD3.value(idsQ[i]));
												yvaluesF.insert(cname, tempvec);
											}
										}

									}
								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}

							if (idsQ != nullptr)
							{
							}
							if (valRealD2 != nullptr)
							{
							}
							if (valRealF2 != nullptr)
							{
							}
							if (valVirD2 != nullptr)
							{
							}
							if (valVirF2 != nullptr)
							{
							}
						}
						else if (QString(varName) == "Stress(N)")
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();

							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();

								//获取单元节点总数
								int count1 = 0;
								for (int p = 0; p < nums1; ++p)
								{
									int rId = idsR[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count1 += elenodeNums * componeNums;
								}

								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());
							int* idsQ = lbIds2.data();
							double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
							double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
							double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
							if (nums2 > 0)
							{
								int* idsQ = lbIds2.data();

								//获取单元节点总数
								int count2 = 0;
								for (int p = 0; p < nums2; ++p)
								{
									int rId = idsQ[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count2 += elenodeNums * componeNums * 2;
								}

								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}

							for (QString compo : composy)
							{
								//实体
								for (int i = 0; i < nums1; ++i)
								{
									if (ids.contains(idsR[i]))
									{
										//曲线名称
										QString cname = QString("%1_%2_%3_%4-%5_%6_%7_%8-ID%9").arg(varx).arg(compox).arg(layerx).arg(complexx)
											.arg(vary).arg(compo).arg(layery).arg(complexy).arg(idsR[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, QHash<int, double>> valD1{};
										QHash<int, QHash<int, double>> valD2{};
										QHash<int, QHash<int, float>> valD3{};
										QHash<int, QHash<int, float>> valD4{};
										QVector<double> local{};

										if (isD)
										{
											valD1 = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, idsR, compoIndex, modelData, local);
											valD2 = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, idsR, compoIndex, modelData, local);
										}
										else
										{
											valD3 = toFloatHashFF(calcuEleNodeComponeValuesR(valRealF1, nums1, componeName, idsR, compoIndex, modelData, local));
											valD4 = toFloatHashFF(calcuEleNodeComponeValuesR(valVirF1, nums1, componeName, idsR, compoIndex, modelData, local));
										}

										if (complexy == QString("虚部") || complexy == QString("Img"))
										{
											if (isD)
											{
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
											QHash<int, double> ndata = valD2[idsR[i]];
											int nsize = ndata.size();
											double sum = 0;
											QHash<int, double>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											double value = sum / nsize;
											tempvec.append(value);
											yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);

												QHash<int, float> ndata = valD4[idsR[i]];
												int nsize = ndata.size();
												float sum = 0;
												QHash<int, float>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												float value = sum / nsize;

												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complexy == QString("幅值") || complexy == QString("Amplitude"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsR[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsR[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;
;
												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complexy == QString("相位") || complexy == QString("Phase"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsR[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
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
                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsR[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;

												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);
                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }
                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												QHash<int, double> ndata = valD1[idsR[i]];
												int nsize = ndata.size();
												double sum = 0;
												QHash<int, double>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												double value = sum / nsize;
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> ndata = valD3[idsR[i]];
												int nsize = ndata.size();
												float sum = 0;
												QHash<int, float>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												float value = sum / nsize;
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
									}
								}
								//壳
								for (int i = 0; i < nums2; ++i)
								{
									if (ids.contains(idsQ[i]))
									{
										//曲线名称
										QString cname = QString("%1_%2_%3_%4-%5_%6_%7_%8-ID%9").arg(varx).arg(compox).arg(layerx).arg(complexx)
											.arg(vary).arg(compo).arg(layery).arg(complexy).arg(idsQ[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, QHash<int, double>> valD1{};
										QHash<int, QHash<int, double>> valD2{};
										QHash<int, QHash<int, float>> valD3{};
										QHash<int, QHash<int, float>> valD4{};
										QVector<double> local;

										if (isD)
										{
											valD1 = calcuEleNodeComponeValuesQ(valRealD2, nums1, componeName, layery, idsQ, compoIndex, modelData, local);
											valD2 = calcuEleNodeComponeValuesQ(valVirD2, nums1, componeName, layery, idsQ, compoIndex, modelData, local);
										}
										else
										{
											valD3 = toFloatHashFF(calcuEleNodeComponeValuesQ(valRealF2, nums1, componeName, layery, idsQ, compoIndex, modelData, local));
											valD4 = toFloatHashFF(calcuEleNodeComponeValuesQ(valVirF2, nums1, componeName, layery, idsQ, compoIndex, modelData, local));
										}

										if (complexy == QString("虚部") || complexy == QString("Img"))
										{
											if (isD)
											{
												QHash<int, double> vdata = valD2[idsQ[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator it;
												for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
												{
													vsum += it.value();
												}
												double value = vsum / vsize;
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> vdata = valD4[idsQ[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator it;
												for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
												{
													vsum += it.value();
												}
												float value = vsum / vsize;
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complexy == QString("幅值") || complexy == QString("Amplitude"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsQ[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsQ[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsQ[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsQ[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;

												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complexy == QString("相位") || complexy == QString("Phase"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsQ[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsQ[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
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
                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsQ[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsQ[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;

												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);
                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }
                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												QHash<int, double> rdata = valD1[idsQ[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator it;
												for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
												{
													rsum += it.value();
												}
												double value = rsum / rsize;

												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> rdata = valD3[idsQ[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator it;
												for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
												{
													rsum += it.value();
												}
												float value = rsum / rsize;
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
									}
								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}

							if (idsQ != nullptr)
							{
							}
							if (valRealD2 != nullptr)
							{
							}
							if (valRealF2 != nullptr)
							{
							}
							if (valVirD2 != nullptr)
							{
							}
							if (valVirF2 != nullptr)
							{
							}
						}
						else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress(N)")
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();

							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();

								//获取单元节点总数
								int count1 = 0;
								for (int p = 0; p < nums1; ++p)
								{
									int rId = idsR[p];
									int partID = modelData->getPartIDByEleID(rId);
									QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                    int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                    int eleType = partData->_parteledata.cell_info[ele_cid].type;
									int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
									count1 += elenodeNums * componeNums;
								}

								//实部
								if (isD)
								{

								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							//壳单元
							int nums2 = static_cast<int>(lbIds2.size());

							for (QString compo : composy)
							{
								//实体
								for (int i = 0; i < nums1; ++i)
								{
									if (ids.contains(idsR[i]))
									{
										//曲线名称
										QString cname = QString("%1_%2_%3_%4-%5_%6_%7_%8-ID%9").arg(varx).arg(compox).arg(layerx).arg(complexx)
											.arg(vary).arg(compo).arg(layery).arg(complexy).arg(idsR[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										QHash<int, QHash<int, double>> valD1{};
										QHash<int, QHash<int, double>> valD2{};
										QHash<int, QHash<int, float>> valD3{};
										QHash<int, QHash<int, float>> valD4{};
										QVector<double> local{};

										if (isD)
										{
											valD1 = calcuEleNodeComponeValues(valRealD1, nums1, componeName, idsR, compoIndex, modelData);
											valD2 = calcuEleNodeComponeValues(valVirD1, nums1, componeName, idsR, compoIndex, modelData);
										}
										else
										{
											valD3 = toFloatHashFF(calcuEleNodeComponeValues(valRealF1, nums1, componeName, idsR, compoIndex, modelData));
											valD4 = toFloatHashFF(calcuEleNodeComponeValues(valVirF1, nums1, componeName, idsR, compoIndex, modelData));
										}

										if (complexy == QString("虚部") || complexy == QString("Img"))
										{
											if (isD)
											{
												//存储
												QVector<double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												QHash<int, double> ndata = valD2[idsR[i]];
												int nsize = ndata.size();
												double sum = 0;
												QHash<int, double>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												double value = sum / nsize;
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector<float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);

												QHash<int, float> ndata = valD4[idsR[i]];
												int nsize = ndata.size();
												float sum = 0;
												QHash<int, float>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												float value = sum / nsize;

												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complexy == QString("幅值") || complexy == QString("Amplitude"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsR[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsR[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;
												;
												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complexy == QString("相位") || complexy == QString("Phase"))
										{
											if (isD)
											{
												//实部
												QHash<int, double> rdata = valD1[idsR[i]];
												int rsize = rdata.size();
												double rsum = 0;
												QHash<int, double>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												double v0 = rsum / rsize;

												//虚部
												QHash<int, double> vdata = valD2[idsR[i]];
												int vsize = vdata.size();
												double vsum = 0;
												QHash<int, double>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												double v1 = vsum / vsize;

												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);

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
                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//实部
												QHash<int, float> rdata = valD3[idsR[i]];
												int rsize = rdata.size();
												float rsum = 0;
												QHash<int, float>::const_iterator rit;
												for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
												{
													rsum += rit.value();
												}
												float v0 = rsum / rsize;

												//虚部
												QHash<int, float> vdata = valD4[idsR[i]];
												int vsize = vdata.size();
												float vsum = 0;
												QHash<int, float>::const_iterator vit;
												for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
												{
													vsum += vit.value();
												}
												float v1 = vsum / vsize;

												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);
                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }
                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												QHash<int, double> ndata = valD1[idsR[i]];
												int nsize = ndata.size();
												double sum = 0;
												QHash<int, double>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												double value = sum / nsize;
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(value);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												QHash<int, float> ndata = valD3[idsR[i]];
												int nsize = ndata.size();
												float sum = 0;
												QHash<int, float>::const_iterator it;
												for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
												{
													sum += it.value();
												}
												float value = sum / nsize;
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(value);
												yvaluesF.insert(cname, tempvec);
											}
										}
									}
								}
							}

							if (idsR != nullptr)
							{
							}
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}
						}
						else
						{
							//变量具体信息
							//实体个数
							int nums1 = static_cast<int>(lbIds1.size());
							QStringList componeName = _varStringList1.value(varName);
							int componeNums = componeName.size();
							int count = componeNums * nums1;
							int* idsR = lbIds1.data();
							double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
							double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
							double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
							if (nums1 > 0)
							{
								int* idsR = lbIds1.data();
								//实部
								if (isD)
								{
								}
								else
								{
								}
								//虚部
								if (type == 1)
								{
									if (isD)
									{
									}
									else
									{
									}
								}
							}
							for (QString compo : composy)
							{
								//实体
								for (int i = 0; i < nums1; ++i)
								{
									if (ids.contains(idsR[i]))
									{
										//曲线名称
										QString cname = QString("%1_%2_%3_%4-%5_%6_%7_%8-ID%9").arg(varx).arg(compox).arg(layerx).arg(complexx)
											.arg(vary).arg(compo).arg(layery).arg(complexy).arg(idsR[i]);
										if (!cnames.contains(cname))
											cnames.append(cname);
										//获取当前分量索引
										int compoIndex = componeName.indexOf(compo);
										if (complexy == QString("虚部") || complexy == QString("Img"))
										{
											if (isD)
											{
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valVirD1[i * componeName.size() + compoIndex]);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valVirF1[i * componeName.size() + compoIndex]);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else if (complexy == QString("幅值") || complexy == QString("Amplitude"))
										{
											if (isD)
											{
												double v0 = valRealD1[i * componeName.size() + compoIndex];
												double v1 = valVirD1[i * componeName.size() + compoIndex];
												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valRealF1[i * componeName.size() + compoIndex];
												float v1 = valVirF1[i * componeName.size() + compoIndex];
												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(sqrt(v0 * v0 + v1 * v1));
												yvaluesF.insert(cname, tempvec);
											}

										}
										else if (complexy == QString("相位") || complexy == QString("Phase"))
										{
											if (isD)
											{
												double v0 = valRealD1[i * componeName.size() + compoIndex];
												double v1 = valVirD1[i * componeName.size() + compoIndex];
												//存储
												QVector< double > tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
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
                                                tempvec.append(phase_deg);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												float v0 = valRealF1[i * componeName.size() + compoIndex];
												float v1 = valVirF1[i * componeName.size() + compoIndex];
												//存储
												QVector< float > tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
                                                const float PI = std::acos(-1);
                                                float phase_rad = std::atan2(v1, v0);
                                                float phase_deg = phase_rad * 180.0 / PI;
                                                phase_deg = std::fmod(phase_deg, 360.0);
                                                if (phase_deg < 0) {
                                                    phase_deg += 360.0;
                                                }
                                                if (std::abs(phase_deg - 360.0) < 1e-10) {
                                                    phase_deg = 0.0;
                                                }
                                                tempvec.append(phase_deg);
												yvaluesF.insert(cname, tempvec);
											}
										}
										else
										{
											if (isD)
											{
												//存储
												QVector< double> tempvec;
												if (yvaluesD.contains(cname))
													tempvec = yvaluesD.value(cname);
												tempvec.append(valRealD1[i * componeName.size() + compoIndex]);
												yvaluesD.insert(cname, tempvec);
											}
											else
											{
												//存储
												QVector< float> tempvec;
												if (yvaluesF.contains(cname))
													tempvec = yvaluesF.value(cname);
												tempvec.append(valRealF1[i * componeName.size() + compoIndex]);
												yvaluesF.insert(cname, tempvec);
											}
										}
									}
								}
							}
							if (idsR != nullptr)
							if (valRealD1 != nullptr)
							{
							}
							if (valRealF1 != nullptr)
							{
							}
							if (valVirD1 != nullptr)
							{
							}
							if (valVirF1 != nullptr)
							{
							}
						}
					}
				}
			}
		}
	}

	void mMxdbThread::readTimeVarCurve(const QString& pname, const QString& var, QList<CurveKey> timeCurves, const QVector<int>& ids, QStringList& cnames, QVector<double>& xvalues, QHash<QString, QVector<double>>& yvaluesD, QHash<QString, QVector<float>>& yvaluesF)
	{
		QFileInfo file(pname);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
		if (outPutData == nullptr)
			return;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		if (modelDatabase.size() == 0)
			return;
		int nframe = modelDatabase.size();
		for (int i = 1; i <= nframe; ++i)
		{
			QPointer<mPostModelData> modelData = modelDatabase.value(i);
			if (modelData == nullptr)	return;
			PostFrameInfo info = modelDatabase.value(i)->_frameInfo;
			int nvar = info._frameVars.size();
			for (int p = 0; p < nvar; ++p)
			{
				PostVariableInfo varInfo = info._frameVars.at(p);
				if (var == varInfo._varName)
				{
					bool isSuccess = openFileStatus(info._frameFile);
					if (!isSuccess)
						return;

                    bool isD = DataPrecisionIsDouble();

					//生成X数据
					xvalues.append(info._frameTime);

					//获取当前变量的字节位置
					unsigned long long pos = varInfo._varByte;
					// 预读：块头（type+varName），值在各分支内按需读
					std::vector<int> lbIds1, lbIds2;
					std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
					int type = -1;
					std::string lbVarName;
					if (!readLegacyVarHead(pos, type, lbVarName))
						continue;

					const char* varName = lbVarName.c_str();
					if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
						|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
						|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
					{
						// 预读：实体块 + 壳块全部值
						if (!readLegacyVarValues(20, 16, type == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
							continue;
						//变量具体信息
						//实体个数
						int nums1 = static_cast<int>(lbIds1.size());
						QStringList componeName = _varStringList1.value(varName);
						int componeNums = componeName.size();
						int count1 = 20 * nums1;
						int* idsR = lbIds1.data();
						double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
						double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
						if (nums1 > 0)
						{
							int* idsR = lbIds1.data();
							//实部
							if (isD)
							{

							}
							else
							{
							}
							//虚部
							if (type == 1)
							{
								if (isD)
								{
								}
								else
								{
								}
							}
						}
						//壳单元
						int nums2 = static_cast<int>(lbIds2.size());
						int count2 = 16 * nums2;
						int* idsQ = lbIds2.data();
						double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
						double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
						double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
						double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
						if (nums2 > 0)
						{
							//idsQ = new int[nums2];
							//实部
							if (isD)
							{
							}
							else
							{
							}
							//虚部
							if (type == 1)
							{
								if (isD)
								{
								}
								else
								{
								}
							}
						}

						for (CurveKey timeCurve : timeCurves)
						{
							//实体
							for (int i = 0; i < nums1; ++i)
							{
								if (ids.contains(idsR[i]))
								{
									//曲线名称
									QString cname = QString("Time-%1_%2-ID%3").arg(var).arg(timeCurve.toString()).arg(idsR[i]);
									if (!cnames.contains(cname))
										cnames.append(cname);
									//获取当前分量索引
									int compoIndex = componeName.indexOf(timeCurve.component);
									QHash<int, double> valD1{};
									QHash<int, double> valD2{};
									QHash<int, float> valD3{};
									QHash<int, float> valD4{};

									if (isD)
									{
										valD1 = calcuComponeValues(valRealD1, nums1, componeName, idsR, compoIndex);
										valD2 = calcuComponeValues(valVirD1, nums1, componeName, idsR, compoIndex);
									}
									else
									{
										valD3 = toFloatHashF(calcuComponeValues(valRealF1, nums1, componeName, idsR, compoIndex));
										valD4 = toFloatHashF(calcuComponeValues(valVirF1, nums1, componeName, idsR, compoIndex));
									}

									if (timeCurve.cplx == QString("虚部") || timeCurve.cplx == QString("Img"))
									{
										if (isD)
										{
											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(valD2.value(idsR[i]));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(valD4.value(idsR[i]));
											yvaluesF.insert(cname, tempvec);
										}
									}
									else if (timeCurve.cplx == QString("幅值") || timeCurve.cplx == QString("Amplitude"))
									{
										if (isD)
										{
											double v0 = valD1.value(idsR[i]);
											double v1 = valD2.value(idsR[i]);
											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											float v0 = valD3.value(idsR[i]);
											float v1 = valD4.value(idsR[i]);
											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesF.insert(cname, tempvec);
										}

									}
									else if (timeCurve.cplx == QString("相位") || timeCurve.cplx == QString("Phase"))
									{
										if (isD)
										{
											double v0 = valD1.value(idsR[i]);
											double v1 = valD2.value(idsR[i]);
											//存储

											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);

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
											tempvec.append(phase_deg);

											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											float v0 = valD3.value(idsR[i]);
											float v1 = valD4.value(idsR[i]);
											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);

											const float PI = std::acos(-1);

											float phase_rad = std::atan2(v1, v0);
											float phase_deg = phase_rad * 180.0 / PI;
											phase_deg = std::fmod(phase_deg, 360.0);
											if (phase_deg < 0) {
												phase_deg += 360.0;
											}
											if (std::abs(phase_deg - 360.0) < 1e-10) {
												phase_deg = 0.0;
											}

											tempvec.append(phase_deg);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else
									{
										if (isD)
										{
											//存储
											QVector< double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(valD1.value(idsR[i]));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(valD3.value(idsR[i]));
											yvaluesF.insert(cname, tempvec);
										}
									}

								}
							}
							//壳
							for (int i = 0; i < nums2; ++i)
							{
								if (ids.contains(idsQ[i]))
								{
									//曲线名称
									QString cname = QString("Time-%1_%2-ID%3").arg(var).arg(timeCurve.toString()).arg(idsQ[i]);
									if (!cnames.contains(cname))
										cnames.append(cname);
									//获取当前分量索引
									int compoIndex = componeName.indexOf(timeCurve.component);
									QHash<int, double> valD1{};
									QHash<int, double> valD2{};
									QHash<int, float> valD3{};
									QHash<int, float> valD4{};

									if (isD)
									{
										valD1 = calcuComponeValues1(valRealD2, nums2, componeName, timeCurve.layer, idsQ, compoIndex);
										valD2 = calcuComponeValues1(valVirD2, nums2, componeName, timeCurve.layer, idsQ, compoIndex);
									}
									else
									{
										valD3 = toFloatHashF(calcuComponeValues1(valRealF2, nums2, componeName, timeCurve.layer, idsQ, compoIndex));
										valD4 = toFloatHashF(calcuComponeValues1(valVirF2, nums2, componeName, timeCurve.layer, idsQ, compoIndex));
									}

									if (timeCurve.cplx == QString("虚部") || timeCurve.cplx == QString("Img"))
									{
										if (isD)
										{
											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(valD2.value(idsQ[i]));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(valD4.value(idsQ[i]));
											yvaluesF.insert(cname, tempvec);
										}
									}
									else if (timeCurve.cplx == QString("幅值") || timeCurve.cplx == QString("Amplitude"))
									{
										if (isD)
										{
											double v0 = valD1.value(idsQ[i]);
											double v1 = valD2.value(idsQ[i]);
											//存储
											QVector<double > tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											float v0 = valD3.value(idsQ[i]);
											float v1 = valD4.value(idsQ[i]);
											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesF.insert(cname, tempvec);
										}

									}
									else if (timeCurve.cplx == QString("相位") || timeCurve.cplx == QString("Phase"))
									{
										if (isD)
										{
											double v0 = valD1.value(idsQ[i]);
											double v1 = valD2.value(idsQ[i]);
											//存储
											QVector<double > tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);

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

											tempvec.append(phase_deg);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											float v0 = valD3.value(idsQ[i]);
											float v1 = valD4.value(idsQ[i]);
											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);

											const float PI = std::acos(-1);

											float phase_rad = std::atan2(v1, v0);
											float phase_deg = phase_rad * 180.0 / PI;
											phase_deg = std::fmod(phase_deg, 360.0);
											if (phase_deg < 0) {
												phase_deg += 360.0;
											}
											if (std::abs(phase_deg - 360.0) < 1e-10) {
												phase_deg = 0.0;
											}
											tempvec.append(phase_deg);

											yvaluesF.insert(cname, tempvec);
										}
									}
									else
									{
										if (isD)
										{
											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(valD1.value(idsQ[i]));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(valD3.value(idsQ[i]));
											yvaluesF.insert(cname, tempvec);
										}
									}

								}
							}
						}

						if (idsR != nullptr)
						{
						}
						if (valRealD1 != nullptr)
						{
						}
						if (valRealF1 != nullptr)
						{
						}
						if (valVirD1 != nullptr)
						{
						}
						if (valVirF1 != nullptr)
						{
						}

						if (idsQ != nullptr)
						{
						}
						if (valRealD2 != nullptr)
						{
						}
						if (valRealF2 != nullptr)
						{
						}
						if (valVirD2 != nullptr)
						{
						}
						if (valVirF2 != nullptr)
						{
						}
					}
					else if (QString(varName) == "Stress(N)")
					{
						//变量具体信息
						//实体个数
						int nums1 = static_cast<int>(lbIds1.size());
						QStringList componeName = _varStringList1.value(varName);
						int componeNums = componeName.size();

						int* idsR = lbIds1.data();
						double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
						double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
						if (nums1 > 0)
						{
							int* idsR = lbIds1.data();

							//获取单元节点总数
							int count1 = 0;
							for (int p = 0; p < nums1; ++p)
							{
								int rId = idsR[p];
								int partID = modelData->getPartIDByEleID(rId);
								QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                int eleType = partData->_parteledata.cell_info[ele_cid].type;
								int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
								count1 += elenodeNums * componeNums;
							}

							//实部
							if (isD)
							{

							}
							else
							{
							}
							//虚部
							if (type == 1)
							{
								if (isD)
								{
								}
								else
								{
								}
							}
						}
						//壳单元
						int nums2 = static_cast<int>(lbIds2.size());

						int* idsQ = lbIds2.data();
						double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
						double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
						double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
						double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
						if (nums2 > 0)
						{
							int* idsQ = lbIds2.data();

							//获取单元节点总数
							int count2 = 0;
							for (int p = 0; p < nums2; ++p)
							{
								int rId = idsQ[p];
								int partID = modelData->getPartIDByEleID(rId);
								QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                int eleType = partData->_parteledata.cell_info[ele_cid].type;
								int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
								count2 += elenodeNums * componeNums * 2;
							}

							//实部
							if (isD)
							{
							}
							else
							{
							}
							//虚部
							if (type == 1)
							{
								if (isD)
								{
								}
								else
								{
								}
							}
						}

						for (CurveKey timeCurve : timeCurves)
						{
							//实体
							for (int i = 0; i < nums1; ++i)
							{
								if (ids.contains(idsR[i]))
								{
									//曲线名称
									QString cname = QString("Time-%1_%2-ID%3").arg(var).arg(timeCurve.toString()).arg(idsR[i]);
									if (!cnames.contains(cname))
										cnames.append(cname);
									//获取当前分量索引
									int compoIndex = componeName.indexOf(timeCurve.component);
									QHash<int, QHash<int, double>> valD1{};
									QHash<int, QHash<int, double>> valD2{};
									QHash<int, QHash<int, float>> valD3{};
									QHash<int, QHash<int, float>> valD4{};
									QVector<double> local{};

									if (isD)
									{
										valD1 = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, idsR, compoIndex, modelData, local);
										valD2 = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, idsR, compoIndex, modelData, local);
									}
									else
									{
										valD3 = toFloatHashFF(calcuEleNodeComponeValuesR(valRealF1, nums1, componeName, idsR, compoIndex, modelData, local));
										valD4 = toFloatHashFF(calcuEleNodeComponeValuesR(valVirF1, nums1, componeName, idsR, compoIndex, modelData, local));
									}

									if (timeCurve.cplx == QString("虚部") || timeCurve.cplx == QString("Img"))
									{
										if (isD)
										{
											QHash<int, double> ndata = valD2[idsR[i]];
											int nsize = ndata.size();
											double sum = 0;
											QHash<int, double>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											double value = sum / nsize;

											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(value);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											QHash<int, float> ndata = valD4[idsR[i]];
											int nsize = ndata.size();
											double sum = 0;
											QHash<int, float>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											double value = sum / nsize;
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(value);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else if (timeCurve.cplx == QString("幅值") || timeCurve.cplx == QString("Amplitude"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsR[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;

											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsR[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;

											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesF.insert(cname, tempvec);
										}

									}
									else if (timeCurve.cplx == QString("相位") || timeCurve.cplx == QString("Phase"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsR[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;

											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);

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

											tempvec.append(phase_deg);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsR[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;

											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											const float PI = std::acos(-1);

											float phase_rad = std::atan2(v1, v0);
											float phase_deg = phase_rad * 180.0 / PI;
											phase_deg = std::fmod(phase_deg, 360.0);
											if (phase_deg < 0) {
												phase_deg += 360.0;
											}
											if (std::abs(phase_deg - 360.0) < 1e-10) {
												phase_deg = 0.0;
											}

											tempvec.append(phase_deg);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else
									{
										if (isD)
										{
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											double value = rsum / rsize;

											//存储
											QVector< double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(value);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											float value = rsum / rsize;

											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(value);
											yvaluesF.insert(cname, tempvec);
										}
									}

								}
							}
							//壳
							for (int i = 0; i < nums2; ++i)
							{
								if (ids.contains(idsQ[i]))
								{
									//曲线名称
									QString cname = QString("Time-%1_%2-ID%3").arg(var).arg(timeCurve.toString()).arg(idsQ[i]);
									if (!cnames.contains(cname))
										cnames.append(cname);
									//获取当前分量索引
									int compoIndex = componeName.indexOf(timeCurve.component);
									QHash<int, QHash<int, double>> valD1{};
									QHash<int, QHash<int, double>> valD2{};
									QHash<int, QHash<int, float>> valD3{};
									QHash<int, QHash<int, float>> valD4{};
									QVector<double> local;

									if (isD)
									{
										valD1 = calcuEleNodeComponeValuesQ(valRealD2, nums2, componeName, timeCurve.layer, idsQ, compoIndex, modelData, local);
										valD2 = calcuEleNodeComponeValuesQ(valVirD2, nums2, componeName, timeCurve.layer, idsQ, compoIndex, modelData, local);
									}
									else
									{
										valD3 = toFloatHashFF(calcuEleNodeComponeValuesQ(valRealF2, nums2, componeName, timeCurve.layer, idsQ, compoIndex, modelData, local));
										valD4 = toFloatHashFF(calcuEleNodeComponeValuesQ(valVirF2, nums2, componeName, timeCurve.layer, idsQ, compoIndex, modelData, local));
									}

									if (timeCurve.cplx == QString("虚部") || timeCurve.cplx == QString("Img"))
									{
										if (isD)
										{
											QHash<int, double> vdata = valD2[idsQ[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator it;
											for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
											{
												vsum += it.value();
											}
											double value = vsum / vsize;
											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(value);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											QHash<int, float> vdata = valD4[idsQ[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator it;
											for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
											{
												vsum += it.value();
											}

											float value = vsum / vsize;
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(value);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else if (timeCurve.cplx == QString("幅值") || timeCurve.cplx == QString("Amplitude"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsQ[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsQ[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;

											//存储
											QVector<double > tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsQ[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsQ[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;
											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesF.insert(cname, tempvec);
										}

									}
									else if (timeCurve.cplx == QString("相位") || timeCurve.cplx == QString("Phase"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsQ[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsQ[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;
											//存储
											QVector<double > tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);

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

											tempvec.append(phase_deg);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsQ[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsQ[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;
											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											const float PI = std::acos(-1);

											float phase_rad = std::atan2(v1, v0);
											float phase_deg = phase_rad * 180.0 / PI;
											phase_deg = std::fmod(phase_deg, 360.0);
											if (phase_deg < 0) {
												phase_deg += 360.0;
											}
											if (std::abs(phase_deg - 360.0) < 1e-10) {
												phase_deg = 0.0;
											}

											tempvec.append(phase_deg);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else
									{
										if (isD)
										{
											QHash<int, double> rdata = valD1[idsQ[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											double value = rsum / rsize;
											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(value);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsQ[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											float value = rsum / rsize;
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(value);
											yvaluesF.insert(cname, tempvec);
										}
									}

								}
							}
						}

						if (idsR != nullptr)
						{
						}
						if (valRealD1 != nullptr)
						{
						}
						if (valRealF1 != nullptr)
						{
						}
						if (valVirD1 != nullptr)
						{
						}
						if (valVirF1 != nullptr)
						{
						}

						if (idsQ != nullptr)
						{
						}
						if (valRealD2 != nullptr)
						{
						}
						if (valRealF2 != nullptr)
						{
						}
						if (valVirD2 != nullptr)
						{
						}
						if (valVirF2 != nullptr)
						{
						}
					}
					else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress")
					{
						//变量具体信息
						//实体个数
						int nums1 = static_cast<int>(lbIds1.size());
						QStringList componeName = _varStringList1.value(varName);
						int componeNums = componeName.size();

						int* idsR = lbIds1.data();
						double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
						double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
						if (nums1 > 0)
						{
							int* idsR = lbIds1.data();

							//获取单元节点总数
							int count1 = 0;
							for (int p = 0; p < nums1; ++p)
							{
								int rId = idsR[p];
								int partID = modelData->getPartIDByEleID(rId);
								QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                int eleType = partData->_parteledata.cell_info[ele_cid].type;
								int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
								count1 += elenodeNums * componeNums;
							}

							//实部
							if (isD)
							{

							}
							else
							{
							}
							//虚部
							if (type == 1)
							{
								if (isD)
								{
								}
								else
								{
								}
							}
						}
						//壳单元
						int nums2 = static_cast<int>(lbIds2.size());

						for (CurveKey timeCurve : timeCurves)
						{
							//实体
							for (int i = 0; i < nums1; ++i)
							{
								if (ids.contains(idsR[i]))
								{
									//曲线名称
									QString cname = QString("Time-%1_%2-ID%3").arg(var).arg(timeCurve.toString()).arg(idsR[i]);
									if (!cnames.contains(cname))
										cnames.append(cname);
									//获取当前分量索引
									int compoIndex = componeName.indexOf(timeCurve.component);
									QHash<int, QHash<int, double>> valD1{};
									QHash<int, QHash<int, double>> valD2{};
									QHash<int, QHash<int, float>> valD3{};
									QHash<int, QHash<int, float>> valD4{};
									QVector<double> local{};

									if (isD)
									{
										valD1 = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, idsR, compoIndex, modelData, local);
										valD2 = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, idsR, compoIndex, modelData, local);
									}
									else
									{
										valD3 = toFloatHashFF(calcuEleNodeComponeValuesR(valRealF1, nums1, componeName, idsR, compoIndex, modelData, local));
										valD4 = toFloatHashFF(calcuEleNodeComponeValuesR(valVirF1, nums1, componeName, idsR, compoIndex, modelData, local));
									}

									if (timeCurve.cplx == QString("虚部") || timeCurve.cplx == QString("Img"))
									{
										if (isD)
										{
											QHash<int, double> ndata = valD2[idsR[i]];
											int nsize = ndata.size();
											double sum = 0;
											QHash<int, double>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											double value = sum / nsize;

											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(value);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											QHash<int, float> ndata = valD4[idsR[i]];
											int nsize = ndata.size();
											double sum = 0;
											QHash<int, float>::const_iterator it;
											for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
											{
												sum += it.value();
											}
											double value = sum / nsize;
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(value);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else if (timeCurve.cplx == QString("幅值") || timeCurve.cplx == QString("Amplitude"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsR[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;

											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsR[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;

											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesF.insert(cname, tempvec);
										}

									}
									else if (timeCurve.cplx == QString("相位") || timeCurve.cplx == QString("Phase"))
									{
										if (isD)
										{
											//实部
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											double v0 = rsum / rsize;

											//虚部
											QHash<int, double> vdata = valD2[idsR[i]];
											int vsize = vdata.size();
											double vsum = 0;
											QHash<int, double>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											double v1 = vsum / vsize;

											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
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

											tempvec.append(phase_deg);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//实部
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator rit;
											for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
											{
												rsum += rit.value();
											}
											float v0 = rsum / rsize;

											//虚部
											QHash<int, float> vdata = valD4[idsR[i]];
											int vsize = vdata.size();
											float vsum = 0;
											QHash<int, float>::const_iterator vit;
											for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
											{
												vsum += vit.value();
											}
											float v1 = vsum / vsize;

											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											const float PI = std::acos(-1);

											float phase_rad = std::atan2(v1, v0);
											float phase_deg = phase_rad * 180.0 / PI;
											phase_deg = std::fmod(phase_deg, 360.0);
											if (phase_deg < 0) {
												phase_deg += 360.0;
											}
											if (std::abs(phase_deg - 360.0) < 1e-10) {
												phase_deg = 0.0;
											}

											tempvec.append(phase_deg);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else
									{
										if (isD)
										{
											QHash<int, double> rdata = valD1[idsR[i]];
											int rsize = rdata.size();
											double rsum = 0;
											QHash<int, double>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											double value = rsum / rsize;

											//存储
											QVector< double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(value);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											QHash<int, float> rdata = valD3[idsR[i]];
											int rsize = rdata.size();
											float rsum = 0;
											QHash<int, float>::const_iterator it;
											for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
											{
												rsum += it.value();
											}
											float value = rsum / rsize;

											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(value);
											yvaluesF.insert(cname, tempvec);
										}
									}

								}
							}
						}

						if (idsR != nullptr)
						{
						}
						if (valRealD1 != nullptr)
						{
						}
						if (valRealF1 != nullptr)
						{
						}
						if (valVirD1 != nullptr)
						{
						}
						if (valVirF1 != nullptr)
						{
						}
					}
					else
					{
						//变量具体信息
						//实体个数
						int nums1 = static_cast<int>(lbIds1.size());
						QStringList componeName = _varStringList1.value(varName);
						int componeNums = componeName.size();
						int count = componeNums * nums1;
						int* idsR = lbIds1.data();
						double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
						double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
						double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
						double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
						if (nums1 > 0)
						{
							int* idsR = lbIds1.data();
							//实部
							if (isD)
							{
							}
							else
							{
							}
							//虚部
							if (type == 1)
							{
								if (isD)
								{
								}
								else
								{
								}
							}
						}
						for (CurveKey timeCurve : timeCurves)
						{
							//实体
							for (int i = 0; i < nums1; ++i)
							{
								if (ids.contains(idsR[i]))
								{
									//曲线名称
									QString cname = QString("Time-%1_%2-ID%3").arg(var).arg(timeCurve.toString()).arg(idsR[i]);
									if (!cnames.contains(cname))
										cnames.append(cname);
									//获取当前分量索引
									int compoIndex = componeName.indexOf(timeCurve.component);
									if (timeCurve.cplx == QString("虚部") || timeCurve.cplx == QString("Img"))
									{
										if (isD)
										{
											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(valVirD1[i * componeName.size() + compoIndex]);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(valVirF1[i * componeName.size() + compoIndex]);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else if (timeCurve.cplx == QString("幅值") || timeCurve.cplx == QString("Amplitude"))
									{
										if (isD)
										{
											double v0 = valRealD1[i * componeName.size() + compoIndex];
											double v1 = valVirD1[i * componeName.size() + compoIndex];
											//存储
											QVector<double > tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											float v0 = valRealF1[i * componeName.size() + compoIndex];
											float v1 = valVirF1[i * componeName.size() + compoIndex];
											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(sqrt(v0 * v0 + v1 * v1));
											yvaluesF.insert(cname, tempvec);
										}

									}
									else if (timeCurve.cplx == QString("相位") || timeCurve.cplx == QString("Phase"))
									{
										if (isD)
										{
											double v0 = valRealD1[i * componeName.size() + compoIndex];
											double v1 = valVirD1[i * componeName.size() + compoIndex];
											//存储
											QVector<double > tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
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

											tempvec.append(phase_deg);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											float v0 = valRealF1[i * componeName.size() + compoIndex];
											float v1 = valVirF1[i * componeName.size() + compoIndex];
											//存储
											QVector<float > tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											const float PI = std::acos(-1);

											float phase_rad = std::atan2(v1, v0);
											float phase_deg = phase_rad * 180.0 / PI;
											phase_deg = std::fmod(phase_deg, 360.0);
											if (phase_deg < 0) {
												phase_deg += 360.0;
											}
											if (std::abs(phase_deg - 360.0) < 1e-10) {
												phase_deg = 0.0;
											}

											tempvec.append(phase_deg);
											yvaluesF.insert(cname, tempvec);
										}
									}
									else
									{
										if (isD)
										{
											//存储
											QVector<double> tempvec;
											if (yvaluesD.contains(cname))
												tempvec = yvaluesD.value(cname);
											tempvec.append(valRealD1[i * componeName.size() + compoIndex]);
											yvaluesD.insert(cname, tempvec);
										}
										else
										{
											//存储
											QVector<float> tempvec;
											if (yvaluesF.contains(cname))
												tempvec = yvaluesF.value(cname);
											tempvec.append(valRealF1[i * componeName.size() + compoIndex]);
											yvaluesF.insert(cname, tempvec);
										}
									}
								}
							}
						}
						if (idsR != nullptr)
						if (valRealD1 != nullptr)
						{
						}
						if (valRealF1 != nullptr)
						{
						}
						if (valVirD1 != nullptr)
						{
						}
						if (valVirF1 != nullptr)
						{
						}
					}
				}
			}
		}
	}

	void mMxdbThread::readIDVarCurve(const QString& pname, const QString& var, QList<CurveKey> idCurves, const QVector<int>& ids, const std::set<int>& frameIds, QStringList& cnames, QHash<QString, QHash<int, double>>& yvaluesD, QHash<QString, QHash<int, float>>& yvaluesF, QString caseName)
	{
		QFileInfo file(pname);
		QPointer<mPostDataBase> outPutData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(pname);
		if (outPutData == nullptr)
			return;
		QHash<int, QPointer<mPostModelData>> modelDatabase = outPutData->getAllFrameData();
		if (modelDatabase.size() == 0)
			return;
		int nframe = modelDatabase.size();
		for (int i = 1; i <= nframe; ++i)
		{
			QPointer<mPostModelData> modelData = modelDatabase.value(i);
			if (modelData == nullptr)	return;
			if (frameIds.find(i) != frameIds.end())
			{
				PostFrameInfo info = modelDatabase.value(i)->_frameInfo;
				if (caseName.compare(info._frameCase) == 0)
				{
					if (frameIds.find(info._frameNum) != frameIds.end())
					{
						int nvar = info._frameVars.size();
						for (int p = 0; p < nvar; ++p)
						{
							PostVariableInfo varInfo = info._frameVars.at(p);
							if (var == varInfo._varName)
							{
								bool isSuccess = openFileStatus(info._frameFile);
								if (!isSuccess)
									return;

                                bool isD = DataPrecisionIsDouble();

								//获取当前变量的字节位置
								unsigned long long pos = varInfo._varByte;
								// 预读：块头（type+varName），值在各分支内按需读
								std::vector<int> lbIds1, lbIds2;
								std::vector<double> lbReal1, lbImag1, lbReal2, lbImag2;
								int type = -1;
								std::string lbVarName;
								if (!readLegacyVarHead(pos, type, lbVarName))
									continue;

								const char* varName = lbVarName.c_str();
								if (QString(varName) == "Stress(E)" || QString(varName) == "Strain(E)" || QString(varName) == "PSD Stress(E)" || QString(varName) == "PSD Strain(E)"
									|| QString(varName) == "RMS Stress(E)" || QString(varName) == "1 Segalman RMS Stress(E)" || QString(varName) == "2 Segalman RMS Stress(E)" || QString(varName) == "3 Segalman RMS Stress(E)"
									|| QString(varName) == "PSD Stress(E)" || QString(varName) == "1 Segalman RMS Element Stress(E, Simple Average)(68.269%)")
								{
									// 预读：实体块 + 壳块全部值
									if (!readLegacyVarValues(20, 16, type == 1, lbIds1, lbReal1, lbImag1, lbIds2, lbReal2, lbImag2))
										continue;
									//变量具体信息
									//实体个数
									int nums1 = static_cast<int>(lbIds1.size());
									QStringList componeName = _varStringList1.value(varName);
									int componeNums = componeName.size();
									int count1 = 20 * nums1;
									int* idsR = lbIds1.data();
									double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
									double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
									if (nums1 > 0)
									{
										int* idsR = lbIds1.data();
										//实部
										if (isD)
										{

										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									//壳单元
									int nums2 = static_cast<int>(lbIds2.size());
									int count2 = 16 * nums2;
									int* idsQ = lbIds2.data();
									double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
									double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
									double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
									double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
									if (nums2 > 0)
									{
										int* idsQ = lbIds2.data();
										//实部
										if (isD)
										{
										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									//
									for (CurveKey idCurve : idCurves)
									{
										//实体
										for (int i = 0; i < nums1; ++i)
										{
											if (ids.contains(idsR[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2-f%3").arg(var).arg(idCurve.toString()).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(idCurve.component);
												QHash<int, double> valD1{};
												QHash<int, double> valD2{};
												QHash<int, float> valD3{};
												QHash<int, float> valD4{};

												if (isD)
												{
													valD1 = calcuComponeValues(valRealD1, nums1, componeName, idsR, compoIndex);
													valD2 = calcuComponeValues(valVirD1, nums1, componeName, idsR, compoIndex);
												}
												else
												{
													valD3 = toFloatHashF(calcuComponeValues(valRealF1, nums1, componeName, idsR, compoIndex));
													valD4 = toFloatHashF(calcuComponeValues(valVirF1, nums1, componeName, idsR, compoIndex));
												}

												if (idCurve.cplx == QString("虚部") || idCurve.cplx == QString("Img"))
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], valD2.value(idsR[i]));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], valD4.value(idsR[i]));
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (idCurve.cplx == QString("幅值") || idCurve.cplx == QString("Amplitude"))
												{
													if (isD)
													{
														double v0 = valD1.value(idsR[i]);
														double v1 = valD2.value(idsR[i]);
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valD3.value(idsR[i]);
														float v1 = valD4.value(idsR[i]);
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (idCurve.cplx == QString("相位") || idCurve.cplx == QString("Phase"))
												{
													if (isD)
													{
														double v0 = valD1.value(idsR[i]);
														double v1 = valD2.value(idsR[i]);
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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

														tempvec.insert(idsR[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valD3.value(idsR[i]);
														float v1 = valD4.value(idsR[i]);
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], valD1.value(idsR[i]));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], valD3.value(idsR[i]));
														yvaluesF.insert(cname, tempvec);
													}
												}

											}
										}
										//壳
										for (int i = 0; i < nums2; ++i)
										{
											if (ids.contains(idsQ[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2-f%3").arg(var).arg(idCurve.toString()).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(idCurve.component);
												QHash<int, double> valD1{};
												QHash<int, double> valD2{};
												QHash<int, float> valD3{};
												QHash<int, float> valD4{};

												if (isD)
												{
													valD1 = calcuComponeValues1(valRealD2, nums2, componeName, idCurve.layer, idsQ, compoIndex);
													valD2 = calcuComponeValues1(valVirD2, nums2, componeName, idCurve.layer, idsQ, compoIndex);
												}
												else
												{
													valD3 = toFloatHashF(calcuComponeValues1(valRealF2, nums2, componeName, idCurve.layer, idsQ, compoIndex));
													valD4 = toFloatHashF(calcuComponeValues1(valVirF2, nums2, componeName, idCurve.layer, idsQ, compoIndex));
												}

												if (idCurve.cplx == QString("虚部") || idCurve.cplx == QString("Img"))
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], valD2.value(idsQ[i]));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], valD4.value(idsQ[i]));
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (idCurve.cplx == QString("幅值") || idCurve.cplx == QString("Amplitude"))
												{
													if (isD)
													{
														double v0 = valD1.value(idsQ[i]);
														double v1 = valD2.value(idsQ[i]);
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valD3.value(idsQ[i]);
														float v1 = valD4.value(idsQ[i]);
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (idCurve.cplx == QString("相位") || idCurve.cplx == QString("Phase"))
												{
													if (isD)
													{
														double v0 = valD1.value(idsQ[i]);
														double v1 = valD2.value(idsQ[i]);
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsQ[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valD3.value(idsQ[i]);
														float v1 = valD4.value(idsQ[i]);
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], valD1.value(idsQ[i]));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], valD3.value(idsQ[i]));
														yvaluesF.insert(cname, tempvec);
													}
												}

											}
										}
									}

									if (idsR != nullptr)
									{
									}
									if (valRealD1 != nullptr)
									{
									}
									if (valRealF1 != nullptr)
									{
									}
									if (valVirD1 != nullptr)
									{
									}
									if (valVirF1 != nullptr)
									{
									}

									if (idsQ != nullptr)
									{
									}
									if (valRealD2 != nullptr)
									{
									}
									if (valRealF2 != nullptr)
									{
									}
									if (valVirD2 != nullptr)
									{
									}
									if (valVirF2 != nullptr)
									{
									}
								}
								else if (QString(varName) == "Stress(N)")
								{
									//变量具体信息
									//实体个数
									int nums1 = static_cast<int>(lbIds1.size());
									QStringList componeName = _varStringList1.value(varName);
									int componeNums = componeName.size();

									int* idsR = lbIds1.data();
									double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
									double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
									if (nums1 > 0)
									{
										int* idsR = lbIds1.data();

										//获取单元节点总数
										int count1 = 0;
										for (int p = 0; p < nums1; ++p)
										{
											int rId = idsR[p];
											int partID = modelData->getPartIDByEleID(rId);
											QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                            int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                            int eleType = partData->_parteledata.cell_info[ele_cid].type;
											int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
											count1 += elenodeNums * componeNums;
										}

										//实部
										if (isD)
										{

										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									//壳单元
									int nums2 = static_cast<int>(lbIds2.size());

									int* idsQ = lbIds2.data();
									double* valRealD2 = lbReal2.empty() ? nullptr : lbReal2.data();
									double* valRealF2 = lbReal2.empty() ? nullptr : lbReal2.data();
									double* valVirD2 = lbImag2.empty() ? nullptr : lbImag2.data();
									double* valVirF2 = lbImag2.empty() ? nullptr : lbImag2.data();
									if (nums2 > 0)
									{
										int* idsQ = lbIds2.data();

										//获取单元节点总数
										int count2 = 0;
										for (int p = 0; p < nums2; ++p)
										{
											int rId = idsQ[p];
											int partID = modelData->getPartIDByEleID(rId);
											QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                            int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                            int eleType = partData->_parteledata.cell_info[ele_cid].type;
											int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
											count2 += elenodeNums * componeNums * 2;
										}

										//实部
										if (isD)
										{
										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									for (CurveKey idCurve : idCurves)
									{
										//实体
										for (int i = 0; i < nums1; ++i)
										{
											if (ids.contains(idsR[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2-f%3").arg(var).arg(idCurve.toString()).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(idCurve.component);
												QHash<int, QHash<int, double>> valD1{};
												QHash<int, QHash<int, double>> valD2{};
												QHash<int, QHash<int, float>> valD3{};
												QHash<int, QHash<int, float>> valD4{};
												QVector<double> local{};

												if (isD)
												{
													valD1 = calcuEleNodeComponeValuesR(valRealD1, nums1, componeName, idsR, compoIndex, modelData, local);
													valD2 = calcuEleNodeComponeValuesR(valVirD1, nums1, componeName, idsR, compoIndex, modelData, local);
												}
												else
												{
													valD3 = toFloatHashFF(calcuEleNodeComponeValuesR(valRealF1, nums1, componeName, idsR, compoIndex, modelData, local));
													valD4 = toFloatHashFF(calcuEleNodeComponeValuesR(valVirF1, nums1, componeName, idsR, compoIndex, modelData, local));
												}

												if (idCurve.cplx == QString("虚部") || idCurve.cplx == QString("Img"))
												{
													if (isD)
													{
														QHash<int, double> ndata = valD2[idsR[i]];
														int nsize = ndata.size();
														double sum = 0;
														QHash<int, double>::const_iterator it;
														for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
														{
															sum += it.value();
														}
														double value = sum / nsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> ndata = valD4[idsR[i]];
														int nsize = ndata.size();
														float sum = 0;
														QHash<int, float>::const_iterator it;
														for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
														{
															sum += it.value();
														}
														float value = sum / nsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (idCurve.cplx == QString("幅值") || idCurve.cplx == QString("Amplitude"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsR[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;

														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsR[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (idCurve.cplx == QString("相位") || idCurve.cplx == QString("Phase"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsR[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsR[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsR[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														double value = rsum / rsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														float value = rsum / rsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}

											}
										}
										//壳
										for (int i = 0; i < nums2; ++i)
										{
											if (ids.contains(idsQ[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2-f%3").arg(var).arg(idCurve.toString()).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(idCurve.component);
												QHash<int, QHash<int, double>> valD1{};
												QHash<int, QHash<int, double>> valD2{};
												QHash<int, QHash<int, float>> valD3{};
												QHash<int, QHash<int, float>> valD4{};
												QVector<double> local;

												if (isD)
												{
													valD1 = calcuEleNodeComponeValuesQ(valRealD2, nums1, componeName, idCurve.layer, idsQ, compoIndex, modelData, local);
													valD2 = calcuEleNodeComponeValuesQ(valVirD2, nums1, componeName, idCurve.layer, idsQ, compoIndex, modelData, local);
												}
												else
												{
													valD3 = toFloatHashFF(calcuEleNodeComponeValuesQ(valRealF2, nums1, componeName, idCurve.layer, idsQ, compoIndex, modelData, local));
													valD4 = toFloatHashFF(calcuEleNodeComponeValuesQ(valVirF2, nums1, componeName, idCurve.layer, idsQ, compoIndex, modelData, local));
												}

												if (idCurve.cplx == QString("虚部") || idCurve.cplx == QString("Img"))
												{
													if (isD)
													{
														QHash<int, double> vdata = valD2[idsQ[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator it;
														for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
														{
															vsum += it.value();
														}
														double value = vsum / vsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> vdata = valD4[idsQ[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator it;
														for (it = vdata.constBegin(); it != vdata.constEnd(); ++it)
														{
															vsum += it.value();
														}
														float value = vsum / vsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (idCurve.cplx == QString("幅值") || idCurve.cplx == QString("Amplitude"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsQ[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsQ[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;

														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsQ[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsQ[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (idCurve.cplx == QString("相位") || idCurve.cplx == QString("Phase"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsQ[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsQ[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;

														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsQ[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsQ[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsQ[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														QHash<int, double> rdata = valD1[idsQ[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														double value = rsum / rsize;

														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsQ[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsQ[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														float value = rsum / rsize;

														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsQ[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}
											}
										}
									}

									if (idsR != nullptr)
									{
									}
									if (valRealD1 != nullptr)
									{
									}
									if (valRealF1 != nullptr)
									{
									}
									if (valVirD1 != nullptr)
									{
									}
									if (valVirF1 != nullptr)
									{
									}

									if (idsQ != nullptr)
									{
									}
									if (valRealD2 != nullptr)
									{
									}
									if (valRealF2 != nullptr)
									{
									}
									if (valVirD2 != nullptr)
									{
									}
									if (valVirF2 != nullptr)
									{
									}
								}
								else if (QString(varName) == "Beam Stress(N)" || QString(varName) == "Rod Stress(N)")
								{
									//变量具体信息
									//实体个数
									int nums1 = static_cast<int>(lbIds1.size());
									QStringList componeName = _varStringList1.value(varName);
									int componeNums = componeName.size();

									int* idsR = lbIds1.data();
									double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
									double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
									if (nums1 > 0)
									{
										int* idsR = lbIds1.data();

										//获取单元节点总数
										int count1 = 0;
										for (int p = 0; p < nums1; ++p)
										{
											int rId = idsR[p];
											int partID = modelData->getPartIDByEleID(rId);
											QPointer<mPostPartData> partData = modelData->getPartData(partID);
                                            int ele_cid = partData->_parteledata.cell_rid_to_cid[rId];
                                            int eleType = partData->_parteledata.cell_info[ele_cid].type;
											int elenodeNums = MxFunctions::mElementFunction::elementNodeCount((ElementType)eleType);
											count1 += elenodeNums * componeNums;
										}

										//实部
										if (isD)
										{

										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									//壳单元
									int nums2 = static_cast<int>(lbIds2.size());

									for (CurveKey idCurve : idCurves)
									{
										//实体
										for (int i = 0; i < nums1; ++i)
										{
											if (ids.contains(idsR[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2-f%3").arg(var).arg(idCurve.toString()).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(idCurve.component);
												QHash<int, QHash<int, double>> valD1{};
												QHash<int, QHash<int, double>> valD2{};
												QHash<int, QHash<int, float>> valD3{};
												QHash<int, QHash<int, float>> valD4{};
												QVector<double> local{};

												if (isD)
												{
													valD1 = calcuEleNodeComponeValues(valRealD1, nums1, componeName, idsR, compoIndex, modelData);
													valD2 = calcuEleNodeComponeValues(valVirD1, nums1, componeName, idsR, compoIndex, modelData);
												}
												else
												{
													valD3 = toFloatHashFF(calcuEleNodeComponeValues(valRealF1, nums1, componeName, idsR, compoIndex, modelData));
													valD4 = toFloatHashFF(calcuEleNodeComponeValues(valVirF1, nums1, componeName, idsR, compoIndex, modelData));
												}

												if (idCurve.cplx == QString("虚部") || idCurve.cplx == QString("Img"))
												{
													if (isD)
													{
														QHash<int, double> ndata = valD2[idsR[i]];
														int nsize = ndata.size();
														double sum = 0;
														QHash<int, double>::const_iterator it;
														for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
														{
															sum += it.value();
														}
														double value = sum / nsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> ndata = valD4[idsR[i]];
														int nsize = ndata.size();
														float sum = 0;
														QHash<int, float>::const_iterator it;
														for (it = ndata.constBegin(); it != ndata.constEnd(); ++it)
														{
															sum += it.value();
														}
														float value = sum / nsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (idCurve.cplx == QString("幅值") || idCurve.cplx == QString("Amplitude"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsR[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;

														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsR[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (idCurve.cplx == QString("相位") || idCurve.cplx == QString("Phase"))
												{
													if (isD)
													{
														//实部
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														double v0 = rsum / rsize;

														//虚部
														QHash<int, double> vdata = valD2[idsR[i]];
														int vsize = vdata.size();
														double vsum = 0;
														QHash<int, double>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														double v1 = vsum / vsize;
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsR[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//实部
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator rit;
														for (rit = rdata.constBegin(); rit != rdata.constEnd(); ++rit)
														{
															rsum += rit.value();
														}
														float v0 = rsum / rsize;

														//虚部
														QHash<int, float> vdata = valD4[idsR[i]];
														int vsize = vdata.size();
														float vsum = 0;
														QHash<int, float>::const_iterator vit;
														for (vit = vdata.constBegin(); vit != vdata.constEnd(); ++vit)
														{
															vsum += vit.value();
														}
														float v1 = vsum / vsize;

														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], atan(v1 / v0) * 180 / 3.1415926 + 180);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														QHash<int, double> rdata = valD1[idsR[i]];
														int rsize = rdata.size();
														double rsum = 0;
														QHash<int, double>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														double value = rsum / rsize;
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														QHash<int, float> rdata = valD3[idsR[i]];
														int rsize = rdata.size();
														float rsum = 0;
														QHash<int, float>::const_iterator it;
														for (it = rdata.constBegin(); it != rdata.constEnd(); ++it)
														{
															rsum += it.value();
														}
														float value = rsum / rsize;
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], value);
														yvaluesF.insert(cname, tempvec);
													}
												}

											}
										}
									}

									if (idsR != nullptr)
									{
									}
									if (valRealD1 != nullptr)
									{
									}
									if (valRealF1 != nullptr)
									{
									}
									if (valVirD1 != nullptr)
									{
									}
									if (valVirF1 != nullptr)
									{
									}
								}
								else
								{
									//变量具体信息
									//实体个数
									int nums1 = static_cast<int>(lbIds1.size());
									QStringList componeName = _varStringList1.value(varName);
									int componeNums = componeName.size();
									int count = componeNums * nums1;
									int* idsR = lbIds1.data();
									double* valRealD1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valRealF1 = lbReal1.empty() ? nullptr : lbReal1.data();
									double* valVirD1 = lbImag1.empty() ? nullptr : lbImag1.data();
									double* valVirF1 = lbImag1.empty() ? nullptr : lbImag1.data();
									if (nums1 > 0)
									{
										int* idsR = lbIds1.data();
										//实部
										if (isD)
										{
										}
										else
										{
										}
										//虚部
										if (type == 1)
										{
											if (isD)
											{
											}
											else
											{
											}
										}
									}
									for (CurveKey idCurve : idCurves)
									{
										//实体
										for (int i = 0; i < nums1; ++i)
										{
											if (ids.contains(idsR[i]))
											{
												//曲线名称
												QString cname = QString("ID-%1_%2-f%3").arg(var).arg(idCurve.toString()).arg(info._frameNum);
												if (!cnames.contains(cname))
													cnames.append(cname);
												//获取当前分量索引
												int compoIndex = componeName.indexOf(idCurve.component);
												if (idCurve.cplx == QString("虚部") || idCurve.cplx == QString("Img"))
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], valVirD1[i * componeName.size() + compoIndex]);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], valVirF1[i * componeName.size() + compoIndex]);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else if (idCurve.cplx == QString("幅值") || idCurve.cplx == QString("Amplitude"))
												{
													if (isD)
													{
														double v0 = valRealD1[i * componeName.size() + compoIndex];
														double v1 = valVirD1[i * componeName.size() + compoIndex];
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valRealF1[i * componeName.size() + compoIndex];
														float v1 = valVirF1[i * componeName.size() + compoIndex];
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														tempvec.insert(idsR[i], sqrt(v0 * v0 + v1 * v1));
														yvaluesF.insert(cname, tempvec);
													}

												}
												else if (idCurve.cplx == QString("相位") || idCurve.cplx == QString("Phase"))
												{
													if (isD)
													{
														double v0 = valRealD1[i * componeName.size() + compoIndex];
														double v1 = valVirD1[i * componeName.size() + compoIndex];
														//存储
														QHash<int, double > tempvec;
														if (yvaluesD.contains(cname))
															tempvec = yvaluesD.value(cname);
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
														tempvec.insert(idsR[i], phase_deg);
														yvaluesD.insert(cname, tempvec);
													}
													else
													{
														float v0 = valRealF1[i * componeName.size() + compoIndex];
														float v1 = valVirF1[i * componeName.size() + compoIndex];
														//存储
														QHash<int, float > tempvec;
														if (yvaluesF.contains(cname))
															tempvec = yvaluesF.value(cname);
														const float PI = std::acos(-1);
														float phase_rad = std::atan2(v1, v0);
														float phase_deg = phase_rad * 180.0 / PI;
														phase_deg = std::fmod(phase_deg, 360.0);
														if (phase_deg < 0) {
															phase_deg += 360.0;
														}
														if (std::abs(phase_deg - 360.0) < 1e-10) {
															phase_deg = 0.0;
														}
														tempvec.insert(idsR[i], phase_deg);
														yvaluesF.insert(cname, tempvec);
													}
												}
												else
												{
													if (isD)
													{
														//存储
														QHash<int, double> tempvec;
														// 三个分量，且分量值没有在存储当中则为mag
														if (componeNums == 3 && compoIndex == -1)
														{
															int componeIndex = 0;
															double x = valRealD1[i * componeNums + componeIndex];
															double y = valRealD1[i * componeNums + componeIndex + 1];
															double z = valRealD1[i * componeNums + componeIndex + 2];
															if (yvaluesD.contains(cname))
																tempvec = yvaluesD.value(cname);
															tempvec.insert(idsR[i], sqrt(x * x + y * y + z * z));
															yvaluesD.insert(cname, tempvec);
														}
														else
														{
															if (yvaluesD.contains(cname))
																tempvec = yvaluesD.value(cname);
															tempvec.insert(idsR[i], valRealD1[i * componeName.size() + compoIndex]);
															yvaluesD.insert(cname, tempvec);
														}
													}
													else
													{
														//存储
														QHash<int, float> tempvec;
														// 三个分量，且分量值没有在存储当中则为mag
														if (componeNums == 3 && compoIndex == -1)
														{
															int componeIndex = 0;
															double x = valRealF1[i * componeNums + componeIndex];
															double y = valRealF1[i * componeNums + componeIndex + 1];
															double z = valRealF1[i * componeNums + componeIndex + 2];
															if (yvaluesF.contains(cname))
																tempvec = yvaluesF.value(cname);
															tempvec.insert(idsR[i], sqrt(x * x + y * y + z * z));
															yvaluesF.insert(cname, tempvec);
														}
														else
														{
															if (yvaluesF.contains(cname))
																tempvec = yvaluesF.value(cname);
															tempvec.insert(idsR[i], valRealF1[i * componeName.size() + compoIndex]);
															yvaluesF.insert(cname, tempvec);
														}
													}
												}
											}
										}
									}
									if (idsR != nullptr)
									{
									}
									if (valRealD1 != nullptr)
									{
									}
									if (valRealF1 != nullptr)
									{
									}
									if (valVirD1 != nullptr)
									{
									}
									if (valVirF1 != nullptr)
									{
									}
									//壳单元
									int nums2 = static_cast<int>(lbIds2.size());
								}
							}
						}
					}
				}
			}
		}
	}

	void mMxdbThread::slot_readVarValue()
	{
		//readVarDatas(mPathName, 1, "Displacement", "X", "");
		//_modeldb;
	}

	QVector3D mMxdbThread::transVector(const QVector3D& x, const QVector3D& a, const QVector3D& b, const QVector3D& c)
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

    bool mMxdbThread::DataPrecisionIsDouble()
    {
		return _version == 0xA1B1C202 || _version == 0xA1B1C602;
    }

    void mMxdbThread::setProgressBarValue(std::streampos pos)
    {
        double num = (double)pos / (double)_fileSize;
        quint64 tempNum = num * 90;
        int value = tempNum;
        mGlobalSignals::getInstance()->setProgressBarValue(value);
    }

    QHash<int, double> mMxdbThread::calcuComponeValues(const double* values, int nums, const QStringList& lis, int* ids, int varIndex)
	{
		QHash<int, double> comPoneval{};
		if (lis.size() != 14)
			return comPoneval;
		for (int p = 0; p < nums; ++p)
		{
			switch (varIndex)
			{
			case 0:
			{
				double XX = values[p * 20];
				comPoneval.insert(ids[p], XX);
			}
			break;
			case 1:
			{
				double YY = values[p * 20 + 8];
				comPoneval.insert(ids[p], YY);
			}
			break;
			case 2:
			{
				double ZZ = values[p * 20 + 14];
				comPoneval.insert(ids[p], ZZ);
			}
			break;
			case 3:
			{
				double YZ = values[p * 20 + 9];
				comPoneval.insert(ids[p], YZ);
			}
			break;
			case 4:
			{
				double XZ = values[p * 20 + 15];
				comPoneval.insert(ids[p], XZ);
			}
			break;
			case 5:
			{
				double XY = values[p * 20 + 1];
				comPoneval.insert(ids[p], XY);
			}
			break;
			case 6:
			{
				double vonMises = values[p * 20 + 7];
				comPoneval.insert(ids[p], vonMises);
			}
			break;
			case 7:
			{
				double MaxPrincipal = values[p * 20 + 2];
				comPoneval.insert(ids[p], MaxPrincipal);
			}
			break;
			case 8:
			{
				double MidPrincipal = values[p * 20 + 10];
				comPoneval.insert(ids[p], MidPrincipal);
			}
			break;
			case 9:
			{
				double MinPrincipal = values[p * 20 + 16];
				comPoneval.insert(ids[p], MinPrincipal);
			}
			break;
			case 10:
			{
				double Max = values[p * 20 + 2];
				double mid = values[p * 20 + 10];
				double min = values[p * 20 + 16];
				double val = abs(Max) > abs(mid) ? Max : mid;
				double MaxPrincipalabs = abs(val) > abs(min) ? val : min;
				comPoneval.insert(ids[p], MaxPrincipalabs);
			}
			break;
			case 11:
			{
				double Max = values[p * 20 + 2];
				double min = values[p * 20 + 16];
				double MaxShear = (Max - min) / 2;
				comPoneval.insert(ids[p], MaxShear);
			}
			break;
			case 12:
			{
				double Max = values[p * 20 + 2];
				double min = values[p * 20 + 16];
				double Tresca = Max - min;
				comPoneval.insert(ids[p], Tresca);
			}
			break;
			case 13:
			{
				double XX = values[p * 20];
				double YY = values[p * 20 + 8];
				double ZZ = values[p * 20 + 14];
				double Pressure = -(XX + YY + ZZ) / 3;
				comPoneval.insert(ids[p], Pressure);
			}
			break;
			default:
				break;
			}
		}
		return comPoneval;
	}

	QHash<int, float> mMxdbThread::calcuComponeValues(float* values, int nums, const QStringList& lis, int* ids, int varIndex)
	{
		QHash<int, float> comPoneval{};
		if (lis.size() != 14)
			return comPoneval;
		for (int p = 0; p < nums; ++p)
		{
			switch (varIndex)
			{
			case 0:
			{
				float XX = values[p * 20];
				comPoneval.insert(ids[p], XX);
			}
			break;
			case 1:
			{
				float YY = values[p * 20 + 8];
				comPoneval.insert(ids[p], YY);
			}
			break;
			case 2:
			{
				float ZZ = values[p * 20 + 14];
				comPoneval.insert(ids[p], ZZ);
			}
			break;
			case 3:
			{
				float YZ = values[p * 20 + 9];
				comPoneval.insert(ids[p], YZ);
			}
			break;
			case 4:
			{
				float XZ = values[p * 20 + 15];
				comPoneval.insert(ids[p], XZ);
			}
			break;
			case 5:
			{
				float XY = values[p * 20 + 1];
				comPoneval.insert(ids[p], XY);
			}
			break;
			case 6:
			{
				float vonMises = values[p * 20 + 7];
				comPoneval.insert(ids[p], vonMises);
			}
			break;
			case 7:
			{
				float MaxPrincipal = values[p * 20 + 2];
				comPoneval.insert(ids[p], MaxPrincipal);
			}
			break;
			case 8:
			{
				float MidPrincipal = values[p * 20 + 10];
				comPoneval.insert(ids[p], MidPrincipal);
			}
			break;
			case 9:
			{
				float MinPrincipal = values[p * 20 + 16];
				comPoneval.insert(ids[p], MinPrincipal);
			}
			break;
			case 10:
			{
				float Max = values[p * 20 + 2];
				float mid = values[p * 20 + 10];
				float min = values[p * 20 + 16];
				float val = abs(Max) > abs(mid) ? Max : mid;
				float MaxPrincipalabs = abs(val) > abs(min) ? val : min;
				comPoneval.insert(ids[p], MaxPrincipalabs);
			}
			break;
			case 11:
			{
				float Max = values[p * 20 + 2];
				float min = values[p * 20 + 16];
				float MaxShear = (Max - min) / 2;
				comPoneval.insert(ids[p], MaxShear);
			}
			break;
			case 12:
			{
				float Max = values[p * 20 + 2];
				float min = values[p * 20 + 16];
				float Tresca = Max - min;
				comPoneval.insert(ids[p], Tresca);
			}
			break;
			case 13:
			{
				float XX = values[p * 20];
				float YY = values[p * 20 + 8];
				float ZZ = values[p * 20 + 14];
				float Pressure = -(XX + YY + ZZ) / 3;
				comPoneval.insert(ids[p], Pressure);
			}
			break;
			default:
				break;
			}
		}
		return comPoneval;
	}

	QHash<int, double> mMxdbThread::calcuComponeValues(const double* values, int nums, const QStringList& lis, int* ids, int varIndex, QVector<double> local)
	{
		QHash<int, double> comPoneval{};

		if (lis.size() >0 && lis.size() != 14)
		{
			int num = lis.size();
			for (int p = 0; p < nums; ++p)
			{
				comPoneval.insert(ids[p], values[p * num + varIndex]);
			}
			return comPoneval;
		}

		if (lis.size() != 14)
			return comPoneval;

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneval;
		}

		QVector3D newVec;
		for (int p = 0; p < nums; ++p)
		{
			if (local.size() > 0)
			{
				QVector3D oldVec;
				oldVec.setX(values[p * 20]);
				oldVec.setY(values[p * 20 + 8]);
				oldVec.setZ(values[p * 20 + 14]);

				QVector<QVector3D> vector3Ds;
				int numVectors = local.size() / 3;
				for (int i = 0; i < numVectors; ++i)
				{
					int index = i * 3;
					double x = local[index];
					double y = local[index + 1];
					double z = local[index + 2];
					vector3Ds.append(QVector3D(x, y, z));
				}

				newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
			}

			switch (varIndex)
			{
			case 0:
			{
				double XX;
				if (local.size() > 0)
				{
					XX = newVec[0];
				}
				else
				{
					XX = values[p * 20];
				}
				comPoneval.insert(ids[p], XX);
			}
			break;
			case 1:
			{
				double YY;
				if (local.size() > 0)
				{
					YY = newVec[1];
				}
				else
				{
					YY = values[p * 20 + 8];
				}
				comPoneval.insert(ids[p], YY);
			}
			break;
			case 2:
			{
				double ZZ;
				if (local.size() > 0)
				{
					ZZ = newVec[2];
				}
				else
				{
					ZZ = values[p * 20 + 14];
				}
				comPoneval.insert(ids[p], ZZ);
			}
			break;
			case 3:
			{
				double YZ;
				if (local.size() > 0)
				{
					YZ = sqrt(newVec[1] * newVec[1] + newVec[2] * newVec[2]);
				}
				else
				{
					YZ = values[p * 20 + 9];
				}
				comPoneval.insert(ids[p], YZ);
			}
			break;
			case 4:
			{
				double XZ;
				if (local.size() > 0)
				{
					XZ = sqrt(newVec[0] * newVec[0] + newVec[2] * newVec[2]);
				}
				else
				{
					XZ = values[p * 20 + 15];
				}
				comPoneval.insert(ids[p], XZ);
			}
			break;
			case 5:
			{
				double XY;
				if (local.size() > 0)
				{
					XY = sqrt(newVec[0] * newVec[0] + newVec[1] * newVec[1]);
				}
				else
				{
					XY = values[p * 20 + 1];
				}
				comPoneval.insert(ids[p], XY);
			}
			break;
			case 6:
			{
				double vonMises = values[p * 20 + 7];
				comPoneval.insert(ids[p], vonMises);
			}
			break;
			case 7:
			{
				double MaxPrincipal = values[p * 20 + 2];
				comPoneval.insert(ids[p], MaxPrincipal);
			}
			break;
			case 8:
			{
				double MidPrincipal = values[p * 20 + 10];
				comPoneval.insert(ids[p], MidPrincipal);
			}
			break;
			case 9:
			{
				double MinPrincipal = values[p * 20 + 16];
				comPoneval.insert(ids[p], MinPrincipal);
			}
			break;
			case 10:
			{
				double Max = values[p * 20 + 2];
				double mid = values[p * 20 + 10];
				double min = values[p * 20 + 16];
				double val = abs(Max) > abs(mid) ? Max : mid;
				double MaxPrincipalabs = abs(val) > abs(min) ? val : min;
				comPoneval.insert(ids[p], MaxPrincipalabs);
			}
			break;
			case 11:
			{
				double Max = values[p * 20 + 2];
				double min = values[p * 20 + 16];
				double MaxShear = (Max - min) / 2;
				comPoneval.insert(ids[p], MaxShear);
			}
			break;
			case 12:
			{
				double Max = values[p * 20 + 2];
				double min = values[p * 20 + 16];
				double Tresca = Max - min;
				comPoneval.insert(ids[p], Tresca);
			}
			break;
			case 13:
			{
				double XX = values[p * 20];
				double YY = values[p * 20 + 8];
				double ZZ = values[p * 20 + 14];
				double Pressure = -(XX + YY + ZZ) / 3;
				comPoneval.insert(ids[p], Pressure);
			}
			break;
			default:
				break;
			}
		}
		return comPoneval;
	}

	QHash<int, float> mMxdbThread::calcuComponeValues(float* values, int nums, const QStringList& lis, int* ids, int varIndex, QVector<double> local)
	{
		QHash<int, float> comPoneval{};

		if (lis.size() > 0 && lis.size() != 14)
		{
			int num = lis.size();
			for (int p = 0; p < nums; ++p)
			{
				comPoneval.insert(ids[p], values[p * num + varIndex]);
			}
			return comPoneval;
		}

		if (lis.size() != 14)
			return comPoneval;

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneval;
		}

		QVector3D newVec;
		for (int p = 0; p < nums; ++p)
		{
			if (local.size() > 0)
			{
				QVector3D oldVec;
				oldVec.setX(values[p * 20]);
				oldVec.setY(values[p * 20 + 8]);
				oldVec.setZ(values[p * 20 + 14]);

				QVector<QVector3D> vector3Ds;
				int numVectors = local.size() / 3;
				for (int i = 0; i < numVectors; ++i)
				{
					int index = i * 3;
					double x = local[index];
					double y = local[index + 1];
					double z = local[index + 2];
					vector3Ds.append(QVector3D(x, y, z));
				}

				newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
			}

			switch (varIndex)
			{
			case 0:
			{
				float XX;
				if (local.size() > 0)
				{
					XX = newVec[0];
				}
				else
				{
					XX = values[p * 20];
				}
				comPoneval.insert(ids[p], XX);
			}
			break;
			case 1:
			{
				float YY;
				if (local.size() > 0)
				{
					YY = newVec[1];
				}
				else
				{
					YY = values[p * 20 + 8];
				}
				comPoneval.insert(ids[p], YY);
			}
			break;
			case 2:
			{
				float ZZ;
				if (local.size() > 0)
				{
					ZZ = newVec[2];
				}
				else
				{
					ZZ = values[p * 20 + 14];
				}
				comPoneval.insert(ids[p], ZZ);
			}
			break;
			case 3:
			{
				float YZ;
				if (local.size() > 0)
				{
					YZ = sqrt(newVec[1] * newVec[1] + newVec[2] * newVec[2]);
				}
				else
				{
					YZ = values[p * 20 + 9];
				}
				comPoneval.insert(ids[p], YZ);
			}
			break;
			case 4:
			{
				float XZ;
				if (local.size() > 0)
				{
					XZ = sqrt(newVec[0] * newVec[0] + newVec[2] * newVec[2]);
				}
				else
				{
					XZ = values[p * 20 + 15];
				}
				comPoneval.insert(ids[p], XZ);
			}
			break;
			case 5:
			{
				float XY;
				if (local.size() > 0)
				{
					XY = sqrt(newVec[0] * newVec[0] + newVec[1] * newVec[1]);
				}
				else
				{
					XY = values[p * 20 + 1];
				}
				comPoneval.insert(ids[p], XY);
			}
			break;
			case 6:
			{
				float vonMises = values[p * 20 + 7];
				comPoneval.insert(ids[p], vonMises);
			}
			break;
			case 7:
			{
				float MaxPrincipal = values[p * 20 + 2];
				comPoneval.insert(ids[p], MaxPrincipal);
			}
			break;
			case 8:
			{
				float MidPrincipal = values[p * 20 + 10];
				comPoneval.insert(ids[p], MidPrincipal);
			}
			break;
			case 9:
			{
				float MinPrincipal = values[p * 20 + 16];
				comPoneval.insert(ids[p], MinPrincipal);
			}
			break;
			case 10:
			{
				float Max = values[p * 20 + 2];
				float mid = values[p * 20 + 10];
				float min = values[p * 20 + 16];
				float val = abs(Max) > abs(mid) ? Max : mid;
				float MaxPrincipalabs = abs(val) > abs(min) ? val : min;
				comPoneval.insert(ids[p], MaxPrincipalabs);
			}
			break;
			case 11:
			{
				float Max = values[p * 20 + 2];
				float min = values[p * 20 + 16];
				float MaxShear = (Max - min) / 2;
				comPoneval.insert(ids[p], MaxShear);
			}
			break;
			case 12:
			{
				float Max = values[p * 20 + 2];
				float min = values[p * 20 + 16];
				float Tresca = Max - min;
				comPoneval.insert(ids[p], Tresca);
			}
			break;
			case 13:
			{
				float XX = values[p * 20];
				float YY = values[p * 20 + 8];
				float ZZ = values[p * 20 + 14];
				float Pressure = -(XX + YY + ZZ) / 3;
				comPoneval.insert(ids[p], Pressure);
			}
			break;
			default:
				break;
			}
		}
		return comPoneval;
	}

	QHash<int, double> mMxdbThread::calcuComponeValues1(double* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex)
	{
		QHash<int, double> comPoneVals{};
		if (lis.size() != 14 || values == nullptr)
			return comPoneVals;
		QVector<double> valuesTop{};
		QVector<double> valuesBottom{};
		valuesTop.reserve(nums);
		valuesBottom.reserve(nums);
		//计算top层分量值
		for (int p = 0; p < nums; ++p)
		{
			switch (varIndex)
			{
			case 0:
			{
				double XX = values[p * 16 + 1];
				valuesTop.push_back(XX);
			}
			break;
			case 1:
			{
				double YY = values[p * 16 + 2];
				valuesTop.push_back(YY);
			}
			break;
			case 2:
			case 3:
			case 4:
				valuesTop.push_back(0);
				break;
			case 5:
			{
				double XY = values[p * 16 + 3];
				valuesTop.push_back(XY);
			}
			break;
			case 6:
			{
				double vonMises = values[p * 16 + 7];
				valuesTop.push_back(vonMises);
			}
			break;
			case 7:
			{
				double MaxPrincipal = values[p * 16 + 5];
				valuesTop.push_back(MaxPrincipal);
			}
			break;
			case 8:
				valuesTop.push_back(0);
				break;
			case 9:
			{
				double MinPrincipal = values[p * 16 + 6];
				valuesTop.push_back(MinPrincipal);
			}
			break;
			case 10:
			{
				//double MaxPrincipalabs = max(abs(values[p * 16 + 5]), abs(values[p * 16 + 6]));
				double MaxPrincipalabs = abs(values[p * 16 + 5]) > abs(values[p * 16 + 6]) ? values[p * 16 + 5] : values[p * 16 + 6];
				valuesTop.push_back(MaxPrincipalabs);
			}
			break;
			case 11:
			{
				double MaxShear = sqrt((values[p * 16 + 1] - values[p * 16 + 2]) * (values[p * 16 + 1] - values[p * 16 + 2]) / 4.0 + values[p * 16 + 3] * values[p * 16 + 3]);
				valuesTop.push_back(MaxShear);
			}
			break;
			case 12:
			{
				double val = max(abs(values[p * 16 + 5] - 0), abs(values[p * 16 + 5] - values[p * 16 + 6]));
				double Tresca = max(val, abs(0 - values[p * 16 + 6]));
				valuesTop.push_back(Tresca);
			}
			break;
			case 13:
			{
				double Pressure = -(values[p * 16 + 5] + values[p * 16 + 6]) / 3.0;
				valuesTop.push_back(Pressure);
			}
			break;
			default:
				break;
			}
		}

		//计算bottom层分量值
		for (int p = 0; p < nums; ++p)
		{
			switch (varIndex)
			{
			case 0:
			{
				double XX = values[p * 16 + 9];
				valuesBottom.push_back(XX);
			}
			break;
			case 1:
			{
				double YY = values[p * 16 + 10];
				valuesBottom.push_back(YY);
			}
			break;
			case 2:
			case 3:
			case 4:
				valuesBottom.push_back(0);
				break;
			case 5:
			{
				double XY = values[p * 16 + 11];
				valuesBottom.push_back(XY);
			}
			break;
			case 6:
			{
				double vonMises = values[p * 16 + 15];
				valuesBottom.push_back(vonMises);
			}
			break;
			case 7:
			{
				double MaxPrincipal = values[p * 16 + 13];
				valuesBottom.push_back(MaxPrincipal);
			}
			break;
			case 8:
				valuesBottom.push_back(0);
				break;
			case 9:
			{
				double MinPrincipal = values[p * 16 + 14];
				valuesBottom.push_back(MinPrincipal);
			}
			break;
			case 10:
			{
				//double MaxPrincipalabs = max(abs(values[p * 16 + 13]), abs(values[p * 16 + 14]));
				double MaxPrincipalabs = abs(values[p * 16 + 13]) > abs(values[p * 16 + 14]) ? values[p * 16 + 13] : abs(values[p * 16 + 14]);
				valuesBottom.push_back(MaxPrincipalabs);
			}
			break;
			case 11:
			{
				double MaxShear = sqrt((values[p * 16 + 9] - values[p * 16 + 10]) * (values[p * 16 + 9] - values[p * 16 + 10]) / 4.0 + values[p * 16 + 11] * values[p * 16 + 11]);
				valuesBottom.push_back(MaxShear);
			}
			break;
			case 12:
			{
				double val = max(abs(values[p * 16 + 13] - 0), abs(values[p * 16 + 13] - values[p * 16 + 14]));
				double Tresca = max(val, abs(0 - values[p * 16 + 14]));
				valuesBottom.push_back(Tresca);
			}
			break;
			case 13:
			{
				double Pressure = -(values[p * 16 + 13] + values[p * 16 + 14]) / 3.0;
				valuesBottom.push_back(Pressure);
			}
			break;
			default:
				break;
			}
		}

		if (layer == "Top")
		{
			for (int p = 0; p < nums; ++p)
			{
				comPoneVals.insert(ids[p], valuesTop[p]);
			}
			return comPoneVals;
		}
		else if (layer == "Bottom")
		{
			for (int p = 0; p < nums; ++p)
			{
				comPoneVals.insert(ids[p], valuesBottom[p]);
			}
			return comPoneVals;
		}
		else if (layer == "Middle")
		{
			for (int p = 0; p < nums; ++p)
			{
				//double val = (valuesTop[p] + valuesBottom[p]) / 2;
				comPoneVals.insert(ids[p], 0);
			}
			return comPoneVals;
		}
		else if (layer == "Max")
		{
			for (int p = 0; p < nums; ++p)
			{
				double val = max(valuesTop[p], valuesBottom[p]);
				comPoneVals.insert(ids[p], val);
			}
			return comPoneVals;
		}

		return QHash<int, double>();
	}

	QHash<int, float> mMxdbThread::calcuComponeValues1(float* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex)
	{
		QHash<int, float> comPoneVals{};
		if (lis.size() != 14 || values == nullptr)
			return comPoneVals;
		QVector<float> valuesTop{};
		QVector<float> valuesBottom{};
		//计算top层分量值
		valuesTop.reserve(nums);
		valuesBottom.reserve(nums);
		for (int p = 0; p < nums; ++p)
		{
			switch (varIndex)
			{
			case 0:
			{
				float XX = values[p * 16 + 1];
				valuesTop.push_back(XX);
			}
			break;
			case 1:
			{
				float YY = values[p * 16 + 2];
				valuesTop.push_back(YY);
			}
			break;
			case 2:
			case 3:
			case 4:
				valuesTop.push_back(0);
				break;
			case 5:
			{
				float XY = values[p * 16 + 3];
				valuesTop.push_back(XY);
			}
			break;
			case 6:
			{
				float vonMises = values[p * 16 + 7];
				valuesTop.push_back(vonMises);
			}
			break;
			case 7:
			{
				float MaxPrincipal = values[p * 16 + 5];
				valuesTop.push_back(MaxPrincipal);
			}
			break;
			case 8:
				valuesTop.push_back(0);
				break;
			case 9:
			{
				float MinPrincipal = values[p * 16 + 6];
				valuesTop.push_back(MinPrincipal);
			}
			break;
			case 10:
			{
				//float MaxPrincipalabs = max(abs(values[p * 16 + 5]), abs(values[p * 16 + 6]));
				float MaxPrincipalabs = abs(values[p * 16 + 5]) > abs(values[p * 16 + 6]) ? values[p * 16 + 5] : values[p * 16 + 6];
				valuesTop.push_back(MaxPrincipalabs);
			}
			break;
			case 11:
			{
				float MaxShear = sqrt((values[p * 16 + 1] - values[p * 16 + 2]) * (values[p * 16 + 1] - values[p * 16 + 2]) / 4.0 + values[p * 16 + 3] * values[p * 16 + 3]);
				valuesTop.push_back(MaxShear);
			}
			break;
			case 12:
			{
				float val = max(abs(values[p * 16 + 5] - 0), abs(values[p * 16 + 5] - values[p * 16 + 6]));
				float Tresca = max(val, abs(0 - values[p * 16 + 6]));
				valuesTop.push_back(Tresca);
			}
			break;
			case 13:
			{
				float Pressure = -(values[p * 16 + 5] + values[p * 16 + 6]) / 3.0;
				valuesTop.push_back(Pressure);
			}
			break;
			default:
				break;
			}
		}

		//计算bottom层分量值
		for (int p = 0; p < nums; ++p)
		{
			switch (varIndex)
			{
			case 0:
			{
				float XX = values[p * 16 + 9];
				valuesBottom.push_back(XX);
			}
			break;
			case 1:
			{
				float YY = values[p * 16 + 10];
				valuesBottom.push_back(YY);
			}
			break;
			case 2:
			case 3:
			case 4:
				valuesBottom.push_back(0);
				break;
			case 5:
			{
				float XY = values[p * 16 + 11];
				valuesBottom.push_back(XY);
			}
			break;
			case 6:
			{
				float vonMises = values[p * 16 + 15];
				valuesBottom.push_back(vonMises);
			}
			break;
			case 7:
			{
				float MaxPrincipal = values[p * 16 + 13];
				valuesBottom.push_back(MaxPrincipal);
			}
			break;
			case 8:
				valuesBottom.push_back(0);
				break;
			case 9:
			{
				float MinPrincipal = values[p * 16 + 14];
				valuesBottom.push_back(MinPrincipal);
			}
			break;
			case 10:
			{
				//float MaxPrincipalabs = max(abs(values[p * 16 + 13]), abs(values[p * 16 + 14]));
				float MaxPrincipalabs = abs(values[p * 16 + 13]) > abs(values[p * 16 + 14]) ? values[p * 16 + 13] : values[p * 16 + 14];
				valuesBottom.push_back(MaxPrincipalabs);
			}
			break;
			case 11:
			{
				float MaxShear = sqrt((values[p * 16 + 9] - values[p * 16 + 10]) * (values[p * 16 + 9] - values[p * 16 + 10]) / 4.0 + values[p * 16 + 11] * values[p * 16 + 11]);
				valuesBottom.push_back(MaxShear);
			}
			break;
			case 12:
			{
				float val = max(abs(values[p * 16 + 13] - 0), abs(values[p * 16 + 13] - values[p * 16 + 14]));
				float Tresca = max(val, abs(0 - values[p * 16 + 14]));
				valuesBottom.push_back(Tresca);
			}
			break;
			case 13:
			{
				float Pressure = -(values[p * 16 + 13] + values[p * 16 + 14]) / 3.0;
				valuesBottom.push_back(Pressure);
			}
			break;
			default:
				break;
			}
		}

		if (layer == "Top")
		{
			for (int p = 0; p < nums; ++p)
			{
				comPoneVals.insert(ids[p], valuesTop[p]);
			}
			return comPoneVals;
		}
		else if (layer == "Bottom")
		{
			for (int p = 0; p < nums; ++p)
			{
				comPoneVals.insert(ids[p], valuesBottom[p]);
			}
			return comPoneVals;
		}
		else if (layer == "Middle")
		{
			for (int p = 0; p < nums; ++p)
			{
				//float val = (valuesTop[p] + valuesBottom[p]) / 2;
				comPoneVals.insert(ids[p], 0);
			}
			return comPoneVals;
		}
		else if (layer == "Max")
		{
			for (int p = 0; p < nums; ++p)
			{
				float val = max(valuesTop[p], valuesBottom[p]);
				comPoneVals.insert(ids[p], val);
			}
			return comPoneVals;
		}

		return QHash<int, float>();
	}

	QHash<int, QHash<int, double>> mMxdbThread::calcuEleNodeComponeValuesR(const double* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local)
	{
		QHash<int, QHash<int, double>> comPoneval{};
		if (lis.size() != 14)
			return comPoneval;

		if (values == nullptr)
		{
			return comPoneval;
		}

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneval;
		}

		int index = 0;
		for (int p = 0; p < nums; ++p)
		{
			QHash<int, double> nodeValue;
			int eleID = ids[p];

			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID,eleID);

			QVector3D newVec;
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[index * 14]);
					oldVec.setY(values[index * 14 + 1]);
					oldVec.setZ(values[index * 14 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = c * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}

					newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}


				switch (varIndex)
				{
					case 0:
					{
						double XX;
						if (local.size() > 0)
						{
							XX = newVec[0];
						}
						else
						{
							XX = values[index * 14];
						}
						nodeValue.insert(nodeIDs[i], XX);
					}
					break;
					case 1:
					{
						double YY ;
						if (local.size() > 0)
						{
							YY = newVec[1];
						}
						else
						{
							YY = values[index * 14 + 1];
						}
						nodeValue.insert(nodeIDs[i], YY);
					}
					break;
					case 2:
					{
						double ZZ;
						if (local.size() > 0)
						{
							ZZ = newVec[2];
						}
						else
						{
							ZZ = values[index * 14 + 2];
						}
						nodeValue.insert(nodeIDs[i], ZZ);
					}
					break;
					case 3:
					{
						float YZ ;
						if (local.size() > 0)
						{
							YZ = sqrt(newVec[1] * newVec[1] + newVec[2] * newVec[2]);
						}
						else
						{
							YZ = values[index * 14 + 3];
						}
						nodeValue.insert(nodeIDs[i], YZ);
					}
					break;
					case 4:
					{
						double XZ;
						if (local.size() > 0)
						{
							XZ = sqrt(newVec[0] * newVec[0] + newVec[2] * newVec[2]);
						}
						else
						{
							XZ = values[index * 14 + 4];
						}
						nodeValue.insert(nodeIDs[i], XZ);
					}
					break;
					case 5:
					{
						double XY ;
						if (local.size() > 0)
						{
							XY = sqrt(newVec[0] * newVec[0] + newVec[1] * newVec[1]);
						}
						else
						{
							XY = values[index * 14 + 5];
						}
						nodeValue.insert(nodeIDs[i], XY);
					}
					break;
					case 6:
					{
						double vonMises = values[index * 14 + 6];
						nodeValue.insert(nodeIDs[i], vonMises);
					}
					break;
					case 7:
					{
						double MaxPrincipal = values[index * 14 + 7];
						nodeValue.insert(nodeIDs[i], MaxPrincipal);
					}
					break;
					case 8:
					{
						double MidPrincipal = values[index * 14 + 8];
						nodeValue.insert(nodeIDs[i], MidPrincipal);
					}
					break;
					case 9:
					{
						double MinPrincipal = values[index * 14 + 9];
						nodeValue.insert(nodeIDs[i], MinPrincipal);
					}
					break;
					case 10:
					{
						double MaxPrincipalabs = values[index * 14 + 10];
						nodeValue.insert(nodeIDs[i], MaxPrincipalabs);
					}
					break;
					case 11:
					{
						double MaxShear = values[index * 14 + 11];
						nodeValue.insert(nodeIDs[i], MaxShear);
					}
					break;
					case 12:
					{
						double Tresca = values[index * 14 + 12];
						nodeValue.insert(nodeIDs[i], Tresca);
					}
					break;
					case 13:
					{
						double Pressure = values[index * 14 + 13];
						nodeValue.insert(nodeIDs[i], Pressure);
					}
					break;
					default:
						break;
				}
				index++;
			}
			comPoneval.insert(ids[p],nodeValue);
		}
		return comPoneval;
	}

	QHash<int, QHash<int, float>> mMxdbThread::calcuEleNodeComponeValuesR(float* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local)
	{
		QHash<int, QHash<int, float>> comPoneval{};
		if (lis.size() != 14)
			return comPoneval;

		if (values == nullptr)
		{
			return comPoneval;
		}

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneval;
		}

		QVector3D newVec;
		int index = 0;
		for (int p = 0; p < nums; ++p)
		{
			QHash<int, float> nodeValue;
			int eleID = ids[p];

			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[index * 14]);
					oldVec.setY(values[index * 14 + 1]);
					oldVec.setZ(values[index * 14 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = i * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}

					newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}

				switch (varIndex)
				{
					case 0:
					{
						float XX;
						if (local.size() > 0)
						{
							XX = newVec[0];
						}
						else
						{
							XX = values[index * 14];
						}
						nodeValue.insert(nodeIDs[i], XX);
					}
					break;
					case 1:
					{
						float YY;
						if (local.size() > 0)
						{
							YY = newVec[1];
						}
						else
						{
							YY = values[index * 14 + 1];
						}
						nodeValue.insert(nodeIDs[i], YY);
					}
					break;
					case 2:
					{
						float ZZ;
						if (local.size() > 0)
						{
							ZZ = newVec[2];
						}
						else
						{
							ZZ = values[index * 14 + 2];
						}
						nodeValue.insert(nodeIDs[i], ZZ);
					}
					break;
					case 3:
					{
						float YZ;
						if (local.size() > 0)
						{
							YZ = sqrt(newVec[1] * newVec[1] + newVec[2] * newVec[2]);
						}
						else
						{
							YZ = values[index * 14 + 3];
						}
						nodeValue.insert(nodeIDs[i], YZ);
					}
					break;
					case 4:
					{
						float XZ;
						if (local.size() > 0)
						{
							XZ = sqrt(newVec[0] * newVec[0] + newVec[2] * newVec[2]);
						}
						else
						{
							XZ = values[index * 14 + 4];
						}
						nodeValue.insert(nodeIDs[i], XZ);
					}
					break;
					case 5:
					{
						float XY;
						if (local.size() > 0)
						{
							XY = sqrt(newVec[0] * newVec[0] + newVec[1] * newVec[1]);
						}
						else
						{
							XY = values[index * 14 + 5];
						}
						nodeValue.insert(nodeIDs[i], XY);
					}
					break;
					case 6:
					{
						float vonMises = values[index * 14 + 6];
						nodeValue.insert(nodeIDs[i], vonMises);
					}
					break;
					case 7:
					{
						float MaxPrincipal = values[index * 14 + 7];
						nodeValue.insert(nodeIDs[i], MaxPrincipal);
					}
					break;
					case 8:
					{
						float MidPrincipal = values[index * 14 + 8];
						nodeValue.insert(nodeIDs[i], MidPrincipal);
					}
					break;
					case 9:
					{
						float MinPrincipal = values[index * 14 + 9];
						nodeValue.insert(nodeIDs[i], MinPrincipal);
					}
					break;
					case 10:
					{
						float MaxPrincipalabs = values[index * 14 + 10];
						nodeValue.insert(nodeIDs[i], MaxPrincipalabs);
					}
					break;
					case 11:
					{
						float MaxShear = values[index * 14 + 11];
						nodeValue.insert(nodeIDs[i], MaxShear);
					}
					break;
					case 12:
					{
						float Tresca = values[index * 14 + 12];
						nodeValue.insert(nodeIDs[i], Tresca);
					}
					break;
					case 13:
					{
						float Pressure = values[index * 14 + 13];
						nodeValue.insert(nodeIDs[i], Pressure);
					}
					break;
					default:
						break;
				}
				index++;
			}
			comPoneval.insert(ids[p],nodeValue);
		}
		return comPoneval;
	}

	QHash<int, double> mMxdbThread::calcuEleNodeComponeValR(const double * values, int nums, const QStringList & lis, int * ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local)
	{
		QHash<int, double> comPoneval{};
		if (lis.size() != 14)
			return comPoneval;

		if (values == nullptr)
		{
			return comPoneval;
		}

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneval;
		}

		int index = 0;
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];

			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> tempNodeids = modelData->getElementNodeId(partID, eleID);
			QList<int> nodeIDs{};
			for (auto it : tempNodeids)
			{
				nodeIDs.append(modelData->_nodedata.ids[it]);
			}
			QVector3D newVec;
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[index * 14]);
					oldVec.setY(values[index * 14 + 1]);
					oldVec.setZ(values[index * 14 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = c * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}

					newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}


				switch (varIndex)
				{
				case 0:
				{
					double XX;
					if (local.size() > 0)
					{
						XX = newVec[0];
					}
					else
					{
						XX = values[index * 14];
					}
					comPoneval.insert(nodeIDs[i], XX);
				}
				break;
				case 1:
				{
					double YY;
					if (local.size() > 0)
					{
						YY = newVec[1];
					}
					else
					{
						YY = values[index * 14 + 1];
					}
					comPoneval.insert(nodeIDs[i], YY);
				}
				break;
				case 2:
				{
					double ZZ;
					if (local.size() > 0)
					{
						ZZ = newVec[2];
					}
					else
					{
						ZZ = values[index * 14 + 2];
					}
					comPoneval.insert(nodeIDs[i], ZZ);
				}
				break;
				case 3:
				{
					float YZ;
					if (local.size() > 0)
					{
						YZ = sqrt(newVec[1] * newVec[1] + newVec[2] * newVec[2]);
					}
					else
					{
						YZ = values[index * 14 + 3];
					}
					comPoneval.insert(nodeIDs[i], YZ);
				}
				break;
				case 4:
				{
					double XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newVec[0] * newVec[0] + newVec[2] * newVec[2]);
					}
					else
					{
						XZ = values[index * 14 + 4];
					}
					comPoneval.insert(nodeIDs[i], XZ);
				}
				break;
				case 5:
				{
					double XY;
					if (local.size() > 0)
					{
						XY = sqrt(newVec[0] * newVec[0] + newVec[1] * newVec[1]);
					}
					else
					{
						XY = values[index * 14 + 5];
					}
					comPoneval.insert(nodeIDs[i], XY);
				}
				break;
				case 6:
				{
					double vonMises = values[index * 14 + 6];
					comPoneval.insert(nodeIDs[i], vonMises);
				}
				break;
				case 7:
				{
					double MaxPrincipal = values[index * 14 + 7];
					comPoneval.insert(nodeIDs[i], MaxPrincipal);
				}
				break;
				case 8:
				{
					double MidPrincipal = values[index * 14 + 8];
					comPoneval.insert(nodeIDs[i], MidPrincipal);
				}
				break;
				case 9:
				{
					double MinPrincipal = values[index * 14 + 9];
					comPoneval.insert(nodeIDs[i], MinPrincipal);
				}
				break;
				case 10:
				{
					double MaxPrincipalabs = values[index * 14 + 10];
					comPoneval.insert(nodeIDs[i], MaxPrincipalabs);
				}
				break;
				case 11:
				{
					double MaxShear = values[index * 14 + 11];
					comPoneval.insert(nodeIDs[i], MaxShear);
				}
				break;
				case 12:
				{
					double Tresca = values[index * 14 + 12];
					comPoneval.insert(nodeIDs[i], Tresca);
				}
				break;
				case 13:
				{
					double Pressure = values[index * 14 + 13];
					comPoneval.insert(nodeIDs[i], Pressure);
				}
				break;
				default:
					break;
				}
				index++;
			}
		}
		return comPoneval;
	}

	QHash<int, float> mMxdbThread::calcuEleNodeComponeValR(float * values, int nums, const QStringList & lis, int * ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local)
	{
		QHash<int, float> comPoneval{};
		if (lis.size() != 14)
			return comPoneval;

		if (values == nullptr)
		{
			return comPoneval;
		}

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneval;
		}

		QVector3D newVec;
		int index = 0;
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];

			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> tempNodeids = modelData->getElementNodeId(partID, eleID);
			QList<int> nodeIDs{};
			for (auto it : tempNodeids)
			{
				nodeIDs.append(modelData->_nodedata.ids[it]);
			}
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[index * 14]);
					oldVec.setY(values[index * 14 + 1]);
					oldVec.setZ(values[index * 14 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = i * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}

					newVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}

				switch (varIndex)
				{
				case 0:
				{
					float XX;
					if (local.size() > 0)
					{
						XX = newVec[0];
					}
					else
					{
						XX = values[index * 14];
					}
					comPoneval.insert(nodeIDs[i], XX);
				}
				break;
				case 1:
				{
					float YY;
					if (local.size() > 0)
					{
						YY = newVec[1];
					}
					else
					{
						YY = values[index * 14 + 1];
					}
					comPoneval.insert(nodeIDs[i], YY);
				}
				break;
				case 2:
				{
					float ZZ;
					if (local.size() > 0)
					{
						ZZ = newVec[2];
					}
					else
					{
						ZZ = values[index * 14 + 2];
					}
					comPoneval.insert(nodeIDs[i], ZZ);
				}
				break;
				case 3:
				{
					float YZ;
					if (local.size() > 0)
					{
						YZ = sqrt(newVec[1] * newVec[1] + newVec[2] * newVec[2]);
					}
					else
					{
						YZ = values[index * 14 + 3];
					}
					comPoneval.insert(nodeIDs[i], YZ);
				}
				break;
				case 4:
				{
					float XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newVec[0] * newVec[0] + newVec[2] * newVec[2]);
					}
					else
					{
						XZ = values[index * 14 + 4];
					}
					comPoneval.insert(nodeIDs[i], XZ);
				}
				break;
				case 5:
				{
					float XY;
					if (local.size() > 0)
					{
						XY = sqrt(newVec[0] * newVec[0] + newVec[1] * newVec[1]);
					}
					else
					{
						XY = values[index * 14 + 5];
					}
					comPoneval.insert(nodeIDs[i], XY);
				}
				break;
				case 6:
				{
					float vonMises = values[index * 14 + 6];
					comPoneval.insert(nodeIDs[i], vonMises);
				}
				break;
				case 7:
				{
					float MaxPrincipal = values[index * 14 + 7];
					comPoneval.insert(nodeIDs[i], MaxPrincipal);
				}
				break;
				case 8:
				{
					float MidPrincipal = values[index * 14 + 8];
					comPoneval.insert(nodeIDs[i], MidPrincipal);
				}
				break;
				case 9:
				{
					float MinPrincipal = values[index * 14 + 9];
					comPoneval.insert(nodeIDs[i], MinPrincipal);
				}
				break;
				case 10:
				{
					float MaxPrincipalabs = values[index * 14 + 10];
					comPoneval.insert(nodeIDs[i], MaxPrincipalabs);
				}
				break;
				case 11:
				{
					float MaxShear = values[index * 14 + 11];
					comPoneval.insert(nodeIDs[i], MaxShear);
				}
				break;
				case 12:
				{
					float Tresca = values[index * 14 + 12];
					comPoneval.insert(nodeIDs[i], Tresca);
				}
				break;
				case 13:
				{
					float Pressure = values[index * 14 + 13];
					comPoneval.insert(nodeIDs[i], Pressure);
				}
				break;
				default:
					break;
				}
				index++;
			}
		}
		return comPoneval;
	}

	QHash<int, QHash<int, double>> mMxdbThread::calcuEleNodeComponeValuesQ(double* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local)
	{
		QHash<int, QHash<int, double>> comPoneVals{};
		if (lis.size() != 14)
			return comPoneVals;

		if (values == nullptr)
		{
			return comPoneVals;
		}

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneVals;
		}

		QList<double> valuesTop{};
		QList<double> valuesBottom{};

		//计算top层分量值
		QVector3D newtVec;
		int tIndex = 0;
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];
			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[tIndex * 28]);
					oldVec.setY(values[tIndex * 28 + 1]);
					oldVec.setZ(values[tIndex * 28 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = i * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}

					newtVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}

				switch (varIndex)
				{
				case 0:
				{
					double XX;
					if (local.size() > 0)
					{
						XX = newtVec[0];
					}
					else
					{
						XX = values[tIndex * 28];
					}
					valuesTop << XX;
				}
				break;
				case 1:
				{
					double YY;
					if (local.size() > 0)
					{
						YY = newtVec[1];
					}
					else
					{
						YY = values[tIndex * 28 + 1];
					}
					valuesTop << YY;
				}
				break;
				case 2:
				{
					double ZZ;
					if (local.size() > 0)
					{
						ZZ = newtVec[2];
					}
					else
					{
						ZZ = values[tIndex * 28 + 2];
					}
					valuesTop << ZZ;
				}
				break;
				case 3:
				{
					double YZ;
					if (local.size() > 0)
					{
						YZ = sqrt(newtVec[1] * newtVec[1] + newtVec[2] * newtVec[2]);
					}
					else
					{
						YZ = values[tIndex * 28 + 3];
					}
					valuesTop << YZ;
				}
				break;
				case 4:
				{
					double XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newtVec[0] * newtVec[0] + newtVec[2] * newtVec[2]);
					}
					else
					{
						XZ = values[tIndex * 28 + 4];
					}
					valuesTop << XZ;
				}
				break;
				case 5:
				{
					double XY;
					if (local.size() > 0)
					{
						XY = sqrt(newtVec[0] * newtVec[0] + newtVec[1] * newtVec[1]);
					}
					else
					{
						XY = values[tIndex * 28 + 5];
					}
					valuesTop << XY;
				}
				break;
				case 6:
				{
					double vonMises = values[tIndex * 28 + 6];
					valuesTop << vonMises;
				}
				break;
				case 7:
				{
					double MaxPrincipal = values[tIndex * 28 + 7];
					valuesTop << MaxPrincipal;
				}
				break;
				case 8:
				{
					double MidPrincipal = values[tIndex * 28 + 8];
					valuesTop << MidPrincipal;
				}
					break;
				case 9:
				{
					double MinPrincipal = values[tIndex * 28 + 9];
					valuesTop << MinPrincipal;
				}
				break;
				case 10:
				{
					double MaxPrincipalabs = values[tIndex * 28 + 10];
					valuesTop << MaxPrincipalabs;
				}
				break;
				case 11:
				{
					double MaxShear = values[tIndex * 28 + 11];
					valuesTop << MaxShear;
				}
				break;
				case 12:
				{
					double Pressure = values[tIndex * 28 + 12];
					valuesTop << Pressure;
				}
				break;
				case 13:
				{
					double Tresca = values[tIndex * 28 + 13];
					valuesTop << Tresca;
				}
				break;
				default:
					break;
				}
				tIndex++;
			}
		}

		int bIndex = 0;
		QVector3D newbVec;
		//计算bottom层分量值
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];
			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[bIndex * 28]);
					oldVec.setY(values[bIndex * 28 + 1]);
					oldVec.setZ(values[bIndex * 28 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = i * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}

					newbVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}

				switch (varIndex)
				{
				case 0:
				{
					double XX;
					if (local.size() > 0)
					{
						XX = newbVec[0];
					}
					else
					{
						XX = values[bIndex * 28 + 14];
					}
					valuesBottom << XX;
				}
				break;
				case 1:
				{
					double YY;
					if (local.size() > 0)
					{
						YY = newbVec[1];
					}
					else
					{
						YY = values[bIndex * 28 + 15];
					}
					valuesBottom << YY;
				}
				break;
				case 2:
				{
					double ZZ ;
					if (local.size() > 0)
					{
						ZZ = newbVec[2];
					}
					else
					{
						ZZ = values[bIndex * 28 + 16];
					}
					valuesBottom << ZZ;
				}
				break;
				case 3:
				{
					double YZ ;
					if (local.size() > 0)
					{
						YZ = sqrt(newbVec[1] * newbVec[1] + newbVec[2] * newbVec[2]);
					}
					else
					{
						YZ = values[bIndex * 28 + 17];
					}
					valuesBottom << YZ;
				}
				break;
				case 4:
				{
					double XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newbVec[0] * newbVec[0] + newbVec[2] * newbVec[2]);
					}
					else
					{
						XZ = values[bIndex * 28 + 18];
					}
					valuesBottom << XZ;
				}
				break;
				case 5:
				{
					double XY ;
					if (local.size() > 0)
					{
						XY = sqrt(newbVec[0] * newbVec[0] + newbVec[1] * newbVec[1]);
					}
					else
					{
						XY = values[bIndex * 28 + 19];
					}
					valuesBottom << XY;
				}
				break;
				case 6:
				{
					double vonMises = values[bIndex * 28 + 20];
					valuesBottom << vonMises;
				}
				break;
				case 7:
				{
					double MaxPrincipal = values[bIndex * 28 + 21];
					valuesBottom << MaxPrincipal;
				}
				break;
				case 8:
				{
					double MidPrincipal = values[bIndex * 28 + 22];
					valuesBottom << MidPrincipal;
				}
				break;
				case 9:
				{
					double MinPrincipal = values[bIndex * 28 + 23];
					valuesBottom << MinPrincipal;
				}
				break;
				case 10:
				{
					double MaxPrincipalabs = values[bIndex * 28 + 24];
					valuesBottom << MaxPrincipalabs;
				}
				break;
				case 11:
				{
					double MaxShear = values[bIndex * 28 + 25];
					valuesBottom << MaxShear;
				}
				break;
				case 12:
				{
					double Pressure = values[bIndex * 28 + 26];
					valuesBottom << Pressure;
				}
				break;
				case 13:
				{
					double Tresca = values[bIndex * 28 + 27];
					valuesBottom << Tresca;
				}
				break;
				default:
					break;
				}
				bIndex++;
			}
		}

		if (layer == "Top")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, double> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					nodeValue.insert(nodeIDs[i],valuesTop[index]);
				}
				comPoneVals.insert(eleID, nodeValue);
				index++;
			}
			return comPoneVals;
		}
		else if (layer == "Bottom")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, double> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					nodeValue.insert(nodeIDs[i], valuesBottom[index]);
				}
				comPoneVals.insert(eleID, nodeValue);
				index++;
			}
			return comPoneVals;
		}
		else if (layer == "Middle")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, double> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					double val = (valuesTop[index] + valuesBottom[index]) / 2;
					nodeValue.insert(nodeIDs[i], val);
				}
				comPoneVals.insert(eleID, nodeValue);
				index++;
			}
			return comPoneVals;
		}
		else if (layer == "Max")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, double> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					double val = max(valuesTop[index], valuesBottom[index]);
					nodeValue.insert(nodeIDs[i], val);
				}
				comPoneVals.insert(eleID, nodeValue);
				index++;
			}
			return comPoneVals;
		}
		return QHash<int, QHash<int, double>>();
	}

	QHash<int, QHash<int, float>> mMxdbThread::calcuEleNodeComponeValuesQ(float* values, int nums, const QStringList& lis, const QString& layer, int* ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local)
	{
		QHash<int, QHash<int, float>> comPoneVals{};
		if (lis.size() != 14)
			return comPoneVals;

		if (values == nullptr)
		{
			return comPoneVals;
		}
		
		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneVals;
		}

		QList<float> valuesTop{};
		QList<float> valuesBottom{};

		//计算top层分量值
		QVector3D newtVec;
		int tIndex = 0;
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];
			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[tIndex * 28]);
					oldVec.setY(values[tIndex * 28 + 1]);
					oldVec.setZ(values[tIndex * 28 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = i * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}
					newtVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}

				switch (varIndex)
				{
				case 0:
				{
					float XX;
					if (local.size() > 0)
					{
						XX = newtVec[0];
					}
					else
					{
						XX = values[tIndex * 28];
					}
					valuesTop << XX;
				}
				break;
				case 1:
				{
					float YY;
					if (local.size() > 0)
					{
						YY = newtVec[1];
					}
					else
					{
						YY = values[tIndex * 28 + 1];
					}
					valuesTop << YY;
				}
				break;
				case 2:
				{
					float ZZ;
					if (local.size() > 0)
					{
						ZZ = newtVec[2];
					}
					else
					{
						ZZ = values[tIndex * 28 + 2];
					}
					valuesTop << ZZ;
				}
				break;
				case 3:
				{
					float YZ = values[tIndex * 28 + 3];
					if (local.size() > 0)
					{
						YZ = sqrt(newtVec[1] * newtVec[1] + newtVec[2] * newtVec[2]);
					}
					else
					{
						YZ = values[tIndex * 28 + 3];
					}
					valuesTop << YZ;
				}
				break;
				case 4:
				{
					float XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newtVec[0] * newtVec[0] + newtVec[2] * newtVec[2]);
					}
					else
					{
						XZ = values[tIndex * 28 + 4];
					}
					valuesTop << XZ;
				}
				break;
				case 5:
				{
					float XY;
					if (local.size() > 0)
					{
						XY = sqrt(newtVec[0] * newtVec[0] + newtVec[1] * newtVec[1]);
					}
					else
					{
						XY = values[tIndex * 28 + 5];
					}
					valuesTop << XY;
				}
				break;
				case 6:
				{
					float vonMises = values[tIndex * 28 + 6];
					valuesTop << vonMises;
				}
				break;
				case 7:
				{
					float MaxPrincipal = values[tIndex * 28 + 7];
					valuesTop << MaxPrincipal;
				}
				break;
				case 8:
				{
					float MidPrincipal = values[tIndex * 28 + 8];
					valuesTop << MidPrincipal;
				}
				break;
				case 9:
				{
					double MinPrincipal = values[tIndex * 28 + 9];
					valuesTop << MinPrincipal;
				}
				break;
				case 10:
				{
					float MaxPrincipalabs = values[tIndex * 28 + 10];
					valuesTop << MaxPrincipalabs;
				}
				break;
				case 11:
				{
					float MaxShear = values[tIndex * 28 + 11];
					valuesTop << MaxShear;
				}
				break;
				case 12:
				{
					float Pressure = values[tIndex * 28 + 12];
					valuesTop << Pressure;
				}
				break;
				case 13:
				{
					float Tresca = values[tIndex * 28 + 13];
					valuesTop << Tresca;
				}
				break;
				default:
					break;
				}
				tIndex++;
			}
		}

		int bIndex = 0;
		QVector3D newbVec;
		//计算bottom层分量值
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];
			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			if (local.size() > 0)
			{
				QVector3D oldVec;
				oldVec.setX(values[bIndex * 28 + 14]);
				oldVec.setY(values[bIndex * 28 + 15]);
				oldVec.setZ(values[bIndex * 28 + 16]);

				QVector<QVector3D> vector3Ds;
				int numVectors = local.size() / 3;
				for (int c = 0; c < numVectors; ++c)
				{
					int order = i * 3;
					double x = local[order];
					double y = local[order + 1];
					double z = local[order + 2];
					vector3Ds.append(QVector3D(x, y, z));
				}

				newbVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
			}

			for (int i = 0; i < nodeIDs.size(); i++)
			{
				switch (varIndex)
				{
				case 0:
				{
					float XX;
					if (local.size() > 0)
					{
						XX = newbVec[0];
					}
					else
					{
						XX = values[bIndex * 28 + 14];
					}
					valuesBottom << XX;
				}
				break;
				case 1:
				{
					float YY;
					if (local.size() > 0)
					{
						YY = newbVec[1];
					}
					else
					{
						YY = values[bIndex * 28 + 15];
					}
					valuesBottom << YY;
				}
				break;
				case 2:
				{
					float ZZ;
					if (local.size() > 0)
					{
						ZZ = newbVec[2];
					}
					else
					{
						ZZ = values[bIndex * 28 + 16];
					}
					valuesBottom << ZZ;
				}
				break;
				case 3:
				{
					float YZ;
					if (local.size() > 0)
					{
						YZ = sqrt(newbVec[1] * newbVec[1] + newbVec[2] * newbVec[2]);
					}
					else
					{
						YZ = values[bIndex * 28 + 17];
					}
					valuesBottom << YZ;
				}
				break;
				case 4:
				{
					float XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newbVec[0] * newbVec[0] + newbVec[2] * newbVec[2]);
					}
					else
					{
						XZ = values[bIndex * 28 + 18];
					}
					valuesBottom << XZ;
				}
				break;
				case 5:
				{
					float XY;
					if (local.size() > 0)
					{
						XY = sqrt(newbVec[0] * newbVec[0] + newbVec[1] * newbVec[1]);
					}
					else
					{
						XY = values[bIndex * 28 + 19];
					}
					valuesBottom << XY;
				}
				break;
				case 6:
				{
					float vonMises = values[bIndex * 28 + 20];
					valuesBottom << vonMises;
				}
				break;
				case 7:
				{
					float MaxPrincipal = values[bIndex * 28 + 21];
					valuesBottom << MaxPrincipal;
				}
				break;
				case 8:
				{
					float MidPrincipal = values[bIndex * 28 + 22];
					valuesBottom << MidPrincipal;
				}
				break;
				case 9:
				{
					float MinPrincipal = values[bIndex * 28 + 23];
					valuesBottom << MinPrincipal;
				}
				break;
				case 10:
				{
					float MaxPrincipalabs = values[bIndex * 28 + 24];
					valuesBottom << MaxPrincipalabs;
				}
				break;
				case 11:
				{
					float MaxShear = values[bIndex * 28 + 25];
					valuesBottom << MaxShear;
				}
				break;
				case 12:
				{
					float Pressure = values[bIndex * 28 + 26];
					valuesBottom << Pressure;
				}
				break;
				case 13:
				{
					float Tresca = values[bIndex * 28 + 27];
					valuesBottom << Tresca;
				}
				break;
				default:
					break;
				}
				bIndex++;
			}
		}

		if (layer == "Top")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, float> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					nodeValue.insert(nodeIDs[i], valuesTop[index]);
					index++;
				}
				comPoneVals.insert(eleID, nodeValue);
			}
			return comPoneVals;
		}
		else if (layer == "Bottom")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, float> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					nodeValue.insert(nodeIDs[i], valuesBottom[index]);
					index++;
				}
				comPoneVals.insert(eleID, nodeValue);
			}
			return comPoneVals;
		}
		else if (layer == "Middle")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, float> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					double val = (valuesTop[index] + valuesBottom[index]) / 2;
					nodeValue.insert(nodeIDs[i], val);
					index++;
				}
				comPoneVals.insert(eleID, nodeValue);
			}
			return comPoneVals;
		}
		else if (layer == "Max")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, float> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					double val = max(valuesTop[index], valuesBottom[index]);
					nodeValue.insert(nodeIDs[i], val);
					index++;
				}
				comPoneVals.insert(eleID, nodeValue);
			}
			return comPoneVals;
		}
		return QHash<int, QHash<int, float>>();
	}

	QHash<int, double> mMxdbThread::calcuEleNodeComponeValQ(double * values, int nums, const QStringList & lis, const QString & layer, int * ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local)
	{
		QHash<int, double> comPoneVals{};
		if (lis.size() != 14)
			return comPoneVals;

		if (values == nullptr)
		{
			return comPoneVals;
		}

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneVals;
		}

		QList<double> valuesTop{};
		QList<double> valuesBottom{};

		//计算top层分量值
		QVector3D newtVec;
		int tIndex = 0;
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];
			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> tempNodeids = modelData->getElementNodeId(partID, eleID);
			QList<int> nodeIDs{};
			for (auto it : tempNodeids)
			{
				nodeIDs.append(modelData->_nodedata.ids[it]);
			}
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[tIndex * 28]);
					oldVec.setY(values[tIndex * 28 + 1]);
					oldVec.setZ(values[tIndex * 28 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = i * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}

					newtVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}

				switch (varIndex)
				{
				case 0:
				{
					double XX;
					if (local.size() > 0)
					{
						XX = newtVec[0];
					}
					else
					{
						XX = values[tIndex * 28];
					}
					valuesTop << XX;
				}
				break;
				case 1:
				{
					double YY;
					if (local.size() > 0)
					{
						YY = newtVec[1];
					}
					else
					{
						YY = values[tIndex * 28 + 1];
					}
					valuesTop << YY;
				}
				break;
				case 2:
				{
					double ZZ;
					if (local.size() > 0)
					{
						ZZ = newtVec[2];
					}
					else
					{
						ZZ = values[tIndex * 28 + 2];
					}
					valuesTop << ZZ;
				}
				break;
				case 3:
				{
					double YZ;
					if (local.size() > 0)
					{
						YZ = sqrt(newtVec[1] * newtVec[1] + newtVec[2] * newtVec[2]);
					}
					else
					{
						YZ = values[tIndex * 28 + 3];
					}
					valuesTop << YZ;
				}
				break;
				case 4:
				{
					double XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newtVec[0] * newtVec[0] + newtVec[2] * newtVec[2]);
					}
					else
					{
						XZ = values[tIndex * 28 + 4];
					}
					valuesTop << XZ;
				}
				break;
				case 5:
				{
					double XY;
					if (local.size() > 0)
					{
						XY = sqrt(newtVec[0] * newtVec[0] + newtVec[1] * newtVec[1]);
					}
					else
					{
						XY = values[tIndex * 28 + 5];
					}
					valuesTop << XY;
				}
				break;
				case 6:
				{
					double vonMises = values[tIndex * 28 + 6];
					valuesTop << vonMises;
				}
				break;
				case 7:
				{
					double MaxPrincipal = values[tIndex * 28 + 7];
					valuesTop << MaxPrincipal;
				}
				break;
				case 8:
				{
					double MidPrincipal = values[tIndex * 28 + 8];
					valuesTop << MidPrincipal;
				}
				break;
				case 9:
				{
					double MinPrincipal = values[tIndex * 28 + 9];
					valuesTop << MinPrincipal;
				}
				break;
				case 10:
				{
					double MaxPrincipalabs = values[tIndex * 28 + 10];
					valuesTop << MaxPrincipalabs;
				}
				break;
				case 11:
				{
					double MaxShear = values[tIndex * 28 + 11];
					valuesTop << MaxShear;
				}
				break;
				case 12:
				{
					double Pressure = values[tIndex * 28 + 12];
					valuesTop << Pressure;
				}
				break;
				case 13:
				{
					double Tresca = values[tIndex * 28 + 13];
					valuesTop << Tresca;
				}
				break;
				default:
					break;
				}
				tIndex++;
			}
		}

		int bIndex = 0;
		QVector3D newbVec;
		//计算bottom层分量值
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];
			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[bIndex * 28]);
					oldVec.setY(values[bIndex * 28 + 1]);
					oldVec.setZ(values[bIndex * 28 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = i * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}

					newbVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}

				switch (varIndex)
				{
				case 0:
				{
					double XX;
					if (local.size() > 0)
					{
						XX = newbVec[0];
					}
					else
					{
						XX = values[bIndex * 28 + 14];
					}
					valuesBottom << XX;
				}
				break;
				case 1:
				{
					double YY;
					if (local.size() > 0)
					{
						YY = newbVec[1];
					}
					else
					{
						YY = values[bIndex * 28 + 15];
					}
					valuesBottom << YY;
				}
				break;
				case 2:
				{
					double ZZ;
					if (local.size() > 0)
					{
						ZZ = newbVec[2];
					}
					else
					{
						ZZ = values[bIndex * 28 + 16];
					}
					valuesBottom << ZZ;
				}
				break;
				case 3:
				{
					double YZ;
					if (local.size() > 0)
					{
						YZ = sqrt(newbVec[1] * newbVec[1] + newbVec[2] * newbVec[2]);
					}
					else
					{
						YZ = values[bIndex * 28 + 17];
					}
					valuesBottom << YZ;
				}
				break;
				case 4:
				{
					double XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newbVec[0] * newbVec[0] + newbVec[2] * newbVec[2]);
					}
					else
					{
						XZ = values[bIndex * 28 + 18];
					}
					valuesBottom << XZ;
				}
				break;
				case 5:
				{
					double XY;
					if (local.size() > 0)
					{
						XY = sqrt(newbVec[0] * newbVec[0] + newbVec[1] * newbVec[1]);
					}
					else
					{
						XY = values[bIndex * 28 + 19];
					}
					valuesBottom << XY;
				}
				break;
				case 6:
				{
					double vonMises = values[bIndex * 28 + 20];
					valuesBottom << vonMises;
				}
				break;
				case 7:
				{
					double MaxPrincipal = values[bIndex * 28 + 21];
					valuesBottom << MaxPrincipal;
				}
				break;
				case 8:
				{
					double MidPrincipal = values[bIndex * 28 + 22];
					valuesBottom << MidPrincipal;
				}
				break;
				case 9:
				{
					double MinPrincipal = values[bIndex * 28 + 23];
					valuesBottom << MinPrincipal;
				}
				break;
				case 10:
				{
					double MaxPrincipalabs = values[bIndex * 28 + 24];
					valuesBottom << MaxPrincipalabs;
				}
				break;
				case 11:
				{
					double MaxShear = values[bIndex * 28 + 25];
					valuesBottom << MaxShear;
				}
				break;
				case 12:
				{
					double Pressure = values[bIndex * 28 + 26];
					valuesBottom << Pressure;
				}
				break;
				case 13:
				{
					double Tresca = values[bIndex * 28 + 27];
					valuesBottom << Tresca;
				}
				break;
				default:
					break;
				}
				bIndex++;
			}
		}

		if (layer == "Top")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					comPoneVals.insert(nodeIDs[i], valuesTop[index]);
				}
				index++;
			}
			return comPoneVals;
		}
		else if (layer == "Bottom")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					comPoneVals.insert(nodeIDs[i], valuesBottom[index]);
				}
				index++;
			}
			return comPoneVals;
		}
		else if (layer == "Middle")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					double val = (valuesTop[index] + valuesBottom[index]) / 2;
					comPoneVals.insert(nodeIDs[i], val);
				}
				index++;
			}
			return comPoneVals;
		}
		else if (layer == "Max")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					double val = max(valuesTop[index], valuesBottom[index]);
					comPoneVals.insert(nodeIDs[i], val);
				}
				index++;
			}
			return comPoneVals;
		}
		return QHash<int, double>();
	}

	QHash<int, float> mMxdbThread::calcuEleNodeComponeValQ(float * values, int nums, const QStringList & lis, const QString & layer, int * ids, int varIndex, QPointer<mPostModelData> modelData, QVector<double> local)
	{
		QHash<int, float> comPoneVals{};
		if (lis.size() != 14)
			return comPoneVals;

		if (values == nullptr)
		{
			return comPoneVals;
		}

		if (local.size() > 0 && local.size() % 3 != 0)
		{
			return comPoneVals;
		}

		QList<float> valuesTop{};
		QList<float> valuesBottom{};

		//计算top层分量值
		QVector3D newtVec;
		int tIndex = 0;
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];
			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> tempNodeids = modelData->getElementNodeId(partID, eleID);
			QList<int> nodeIDs{};
			for (auto it : tempNodeids)
			{
				nodeIDs.append(modelData->_nodedata.ids[it]);
			}
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (local.size() > 0)
				{
					QVector3D oldVec;
					oldVec.setX(values[tIndex * 28]);
					oldVec.setY(values[tIndex * 28 + 1]);
					oldVec.setZ(values[tIndex * 28 + 2]);

					QVector<QVector3D> vector3Ds;
					int numVectors = local.size() / 3;
					for (int c = 0; c < numVectors; ++c)
					{
						int order = i * 3;
						double x = local[order];
						double y = local[order + 1];
						double z = local[order + 2];
						vector3Ds.append(QVector3D(x, y, z));
					}
					newtVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
				}

				switch (varIndex)
				{
				case 0:
				{
					float XX;
					if (local.size() > 0)
					{
						XX = newtVec[0];
					}
					else
					{
						XX = values[tIndex * 28];
					}
					valuesTop << XX;
				}
				break;
				case 1:
				{
					float YY;
					if (local.size() > 0)
					{
						YY = newtVec[1];
					}
					else
					{
						YY = values[tIndex * 28 + 1];
					}
					valuesTop << YY;
				}
				break;
				case 2:
				{
					float ZZ;
					if (local.size() > 0)
					{
						ZZ = newtVec[2];
					}
					else
					{
						ZZ = values[tIndex * 28 + 2];
					}
					valuesTop << ZZ;
				}
				break;
				case 3:
				{
					float YZ = values[tIndex * 28 + 3];
					if (local.size() > 0)
					{
						YZ = sqrt(newtVec[1] * newtVec[1] + newtVec[2] * newtVec[2]);
					}
					else
					{
						YZ = values[tIndex * 28 + 3];
					}
					valuesTop << YZ;
				}
				break;
				case 4:
				{
					float XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newtVec[0] * newtVec[0] + newtVec[2] * newtVec[2]);
					}
					else
					{
						XZ = values[tIndex * 28 + 4];
					}
					valuesTop << XZ;
				}
				break;
				case 5:
				{
					float XY;
					if (local.size() > 0)
					{
						XY = sqrt(newtVec[0] * newtVec[0] + newtVec[1] * newtVec[1]);
					}
					else
					{
						XY = values[tIndex * 28 + 5];
					}
					valuesTop << XY;
				}
				break;
				case 6:
				{
					float vonMises = values[tIndex * 28 + 6];
					valuesTop << vonMises;
				}
				break;
				case 7:
				{
					float MaxPrincipal = values[tIndex * 28 + 7];
					valuesTop << MaxPrincipal;
				}
				break;
				case 8:
				{
					float MidPrincipal = values[tIndex * 28 + 8];
					valuesTop << MidPrincipal;
				}
				break;
				case 9:
				{
					double MinPrincipal = values[tIndex * 28 + 9];
					valuesTop << MinPrincipal;
				}
				break;
				case 10:
				{
					float MaxPrincipalabs = values[tIndex * 28 + 10];
					valuesTop << MaxPrincipalabs;
				}
				break;
				case 11:
				{
					float MaxShear = values[tIndex * 28 + 11];
					valuesTop << MaxShear;
				}
				break;
				case 12:
				{
					float Pressure = values[tIndex * 28 + 12];
					valuesTop << Pressure;
				}
				break;
				case 13:
				{
					float Tresca = values[tIndex * 28 + 13];
					valuesTop << Tresca;
				}
				break;
				default:
					break;
				}
				tIndex++;
			}
		}

		int bIndex = 0;
		QVector3D newbVec;
		//计算bottom层分量值
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];
			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			if (local.size() > 0)
			{
				QVector3D oldVec;
				oldVec.setX(values[bIndex * 28 + 14]);
				oldVec.setY(values[bIndex * 28 + 15]);
				oldVec.setZ(values[bIndex * 28 + 16]);

				QVector<QVector3D> vector3Ds;
				int numVectors = local.size() / 3;
				for (int c = 0; c < numVectors; ++c)
				{
					int order = i * 3;
					double x = local[order];
					double y = local[order + 1];
					double z = local[order + 2];
					vector3Ds.append(QVector3D(x, y, z));
				}

				newbVec = transVector(oldVec, vector3Ds[0], vector3Ds[1], vector3Ds[2]);
			}

			for (int i = 0; i < nodeIDs.size(); i++)
			{
				switch (varIndex)
				{
				case 0:
				{
					float XX;
					if (local.size() > 0)
					{
						XX = newbVec[0];
					}
					else
					{
						XX = values[bIndex * 28 + 14];
					}
					valuesBottom << XX;
				}
				break;
				case 1:
				{
					float YY;
					if (local.size() > 0)
					{
						YY = newbVec[1];
					}
					else
					{
						YY = values[bIndex * 28 + 15];
					}
					valuesBottom << YY;
				}
				break;
				case 2:
				{
					float ZZ;
					if (local.size() > 0)
					{
						ZZ = newbVec[2];
					}
					else
					{
						ZZ = values[bIndex * 28 + 16];
					}
					valuesBottom << ZZ;
				}
				break;
				case 3:
				{
					float YZ;
					if (local.size() > 0)
					{
						YZ = sqrt(newbVec[1] * newbVec[1] + newbVec[2] * newbVec[2]);
					}
					else
					{
						YZ = values[bIndex * 28 + 17];
					}
					valuesBottom << YZ;
				}
				break;
				case 4:
				{
					float XZ;
					if (local.size() > 0)
					{
						XZ = sqrt(newbVec[0] * newbVec[0] + newbVec[2] * newbVec[2]);
					}
					else
					{
						XZ = values[bIndex * 28 + 18];
					}
					valuesBottom << XZ;
				}
				break;
				case 5:
				{
					float XY;
					if (local.size() > 0)
					{
						XY = sqrt(newbVec[0] * newbVec[0] + newbVec[1] * newbVec[1]);
					}
					else
					{
						XY = values[bIndex * 28 + 19];
					}
					valuesBottom << XY;
				}
				break;
				case 6:
				{
					float vonMises = values[bIndex * 28 + 20];
					valuesBottom << vonMises;
				}
				break;
				case 7:
				{
					float MaxPrincipal = values[bIndex * 28 + 21];
					valuesBottom << MaxPrincipal;
				}
				break;
				case 8:
				{
					float MidPrincipal = values[bIndex * 28 + 22];
					valuesBottom << MidPrincipal;
				}
				break;
				case 9:
				{
					float MinPrincipal = values[bIndex * 28 + 23];
					valuesBottom << MinPrincipal;
				}
				break;
				case 10:
				{
					float MaxPrincipalabs = values[bIndex * 28 + 24];
					valuesBottom << MaxPrincipalabs;
				}
				break;
				case 11:
				{
					float MaxShear = values[bIndex * 28 + 25];
					valuesBottom << MaxShear;
				}
				break;
				case 12:
				{
					float Pressure = values[bIndex * 28 + 26];
					valuesBottom << Pressure;
				}
				break;
				case 13:
				{
					float Tresca = values[bIndex * 28 + 27];
					valuesBottom << Tresca;
				}
				break;
				default:
					break;
				}
				bIndex++;
			}
		}

		if (layer == "Top")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					comPoneVals.insert(nodeIDs[i], valuesTop[index]);
					index++;
				}
			}
			return comPoneVals;
		}
		else if (layer == "Bottom")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					comPoneVals.insert(nodeIDs[i], valuesBottom[index]);
					index++;
				}
			}
			return comPoneVals;
		}
		else if (layer == "Middle")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					double val = (valuesTop[index] + valuesBottom[index]) / 2;
					comPoneVals.insert(nodeIDs[i], val);
					index++;
				}
			}
			return comPoneVals;
		}
		else if (layer == "Max")
		{
			int index = 0;
			for (int p = 0; p < nums; ++p)
			{
				QHash<int, float> nodeValue;
				int eleID = ids[p];
				//获取单元包含的所有节点ID
				int partID = modelData->getPartIDByEleID(eleID);
				QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

				for (int i = 0; i < nodeIDs.size(); i++)
				{
					double val = max(valuesTop[index], valuesBottom[index]);
					comPoneVals.insert(nodeIDs[i], val);
					index++;
				}
			}
			return comPoneVals;
		}
		return QHash<int, float>();
	}

	QHash<int, QHash<int, double>> mMxdbThread::calcuEleNodeComponeValues(double* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData)
	{
		QHash<int, QHash<int, double>> comPoneval{};
		if (lis.size() != 13 && lis.size() != 1)
			return comPoneval;

		if (values == nullptr)
		{
			return comPoneval;
		}

		int index = 0;
		for (int p = 0; p < nums; ++p)
		{
			QHash<int, double> nodeValue;
			int eleID = ids[p];

			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			QVector3D newVec;
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (lis.size() == 13)
				{
					switch (varIndex)
					{
					case 0:
					{
						double Axial = values[index * 13];
						nodeValue.insert(nodeIDs[i], Axial);
					}
					break;
					case 1:
					{
						double SAC = values[index * 13 + 1];
						nodeValue.insert(nodeIDs[i], SAC);
					}
					break;
					case 2:
					{
						double SAD = values[index * 13 + 2];
						nodeValue.insert(nodeIDs[i], SAD);
					}
					break;
					case 3:
					{
						double SAE = values[index * 13 + 3];
						nodeValue.insert(nodeIDs[i], SAE);
					}
					break;
					case 4:
					{
						double SAF = values[index * 13 + 4];
						nodeValue.insert(nodeIDs[i], SAF);
					}
					break;
					case 5:
					{
						double SAMIN = values[index * 13 + 5];
						nodeValue.insert(nodeIDs[i], SAMIN);
					}
					break;
					case 6:
					{
						double SAMAX = values[index * 13 + 6];
						nodeValue.insert(nodeIDs[i], SAMAX);
					}
					break;
					case 7:
					{
						double SBC = values[index * 13 + 7];
						nodeValue.insert(nodeIDs[i], SBC);
					}
					break;
					case 8:
					{
						double SBD = values[index * 13 + 8];
						nodeValue.insert(nodeIDs[i], SBD);
					}
					break;
					case 9:
					{
						double SBE = values[index * 13 + 9];
						nodeValue.insert(nodeIDs[i], SBE);
					}
					break;
					case 10:
					{
						double SBF = values[index * 13 + 10];
						nodeValue.insert(nodeIDs[i], SBF);
					}
					break;
					case 11:
					{
						double SBMIN = values[index * 13 + 11];
						nodeValue.insert(nodeIDs[i], SBMIN);
					}
					break;
					case 12:
					{
						double SBMAX = values[index * 13 + 12];
						nodeValue.insert(nodeIDs[i], SBMAX);
					}
					break;
					default:
						break;
					}
				}
				else
				{
					double Axial = values[index];
					nodeValue.insert(nodeIDs[i], Axial);
				}	
				index++;
			}
			comPoneval.insert(ids[p], nodeValue);
		}
		return comPoneval;
	}

	QHash<int, QHash<int, float>> mMxdbThread::calcuEleNodeComponeValues(float* values, int nums, const QStringList& lis, int* ids, int varIndex, QPointer<mPostModelData> modelData)
	{
		QHash<int, QHash<int, float>> comPoneval{};
		if (lis.size() != 13 && lis.size() != 1)
			return comPoneval;

		if (values == nullptr)
		{
			return comPoneval;
		}

		int index = 0;
		for (int p = 0; p < nums; ++p)
		{
			QHash<int, float> nodeValue;
			int eleID = ids[p];

			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> nodeIDs = modelData->getElementNodeId(partID, eleID);

			QVector3D newVec;
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (lis.size() == 13)
				{
					switch (varIndex)
					{
						case 0:
						{
							float Axial = values[index * 13];
							nodeValue.insert(nodeIDs[i], Axial);
						}
						break;
						case 1:
						{
							float SAC = values[index * 13 + 1];
							nodeValue.insert(nodeIDs[i], SAC);
						}
						break;
						case 2:
						{
							float SAD = values[index * 13 + 2];
							nodeValue.insert(nodeIDs[i], SAD);
						}
						break;
						case 3:
						{
							float SAE = values[index * 13 + 3];
							nodeValue.insert(nodeIDs[i], SAE);
						}
						break;
						case 4:
						{
							float SAF = values[index * 13 + 4];
							nodeValue.insert(nodeIDs[i], SAF);
						}
						break;
						case 5:
						{
							float SAMIN = values[index * 13 + 5];
							nodeValue.insert(nodeIDs[i], SAMIN);
						}
						break;
						case 6:
						{
							float SAMAX = values[index * 13 + 6];
							nodeValue.insert(nodeIDs[i], SAMAX);
						}
						break;
						case 7:
						{
							float SBC = values[index * 13 + 7];
							nodeValue.insert(nodeIDs[i], SBC);
						}
						break;
						case 8:
						{
							float SBD = values[index * 13 + 8];
							nodeValue.insert(nodeIDs[i], SBD);
						}
						break;
						case 9:
						{
							float SBE = values[index * 13 + 9];
							nodeValue.insert(nodeIDs[i], SBE);
						}
						break;
						case 10:
						{
							float SBF = values[index * 13 + 10];
							nodeValue.insert(nodeIDs[i], SBF);
						}
						break;
						case 11:
						{
							float SBMIN = values[index * 13 + 11];
							nodeValue.insert(nodeIDs[i], SBMIN);
						}
						break;
						case 12:
						{
							float SBMAX = values[index * 13 + 12];
							nodeValue.insert(nodeIDs[i], SBMAX);
						}
						break;
						default:
							break;
					}
				}
				else
				{
					float Axial = values[index];
					nodeValue.insert(nodeIDs[i], Axial);
				}
				index++;
			}
			comPoneval.insert(ids[p], nodeValue);
		}
		return comPoneval;
	}

	QHash<int, double> mMxdbThread::calcuEleNodeComponeVal(double * values, int nums, const QStringList & lis, int * ids, int varIndex, QPointer<mPostModelData> modelData)
	{
		QHash<int, double> comPoneval{};
		if (lis.size() != 13 && lis.size() != 1)
			return comPoneval;

		if (values == nullptr)
		{
			return comPoneval;
		}

		int index = 0;
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];

			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> tempNodeids = modelData->getElementNodeId(partID, eleID);
			QList<int> nodeIDs{};
			for (auto it : tempNodeids)
			{
				nodeIDs.append(modelData->_nodedata.ids[it]);
			}

			QVector3D newVec;
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (lis.size() == 13)
				{
					switch (varIndex)
					{
					case 0:
					{
						double Axial = values[index * 13];
						comPoneval.insert(nodeIDs[i], Axial);
					}
					break;
					case 1:
					{
						double SAC = values[index * 13 + 1];
						comPoneval.insert(nodeIDs[i], SAC);
					}
					break;
					case 2:
					{
						double SAD = values[index * 13 + 2];
						comPoneval.insert(nodeIDs[i], SAD);
					}
					break;
					case 3:
					{
						double SAE = values[index * 13 + 3];
						comPoneval.insert(nodeIDs[i], SAE);
					}
					break;
					case 4:
					{
						double SAF = values[index * 13 + 4];
						comPoneval.insert(nodeIDs[i], SAF);
					}
					break;
					case 5:
					{
						double SAMIN = values[index * 13 + 5];
						comPoneval.insert(nodeIDs[i], SAMIN);
					}
					break;
					case 6:
					{
						double SAMAX = values[index * 13 + 6];
						comPoneval.insert(nodeIDs[i], SAMAX);
					}
					break;
					case 7:
					{
						double SBC = values[index * 13 + 7];
						comPoneval.insert(nodeIDs[i], SBC);
					}
					break;
					case 8:
					{
						double SBD = values[index * 13 + 8];
						comPoneval.insert(nodeIDs[i], SBD);
					}
					break;
					case 9:
					{
						double SBE = values[index * 13 + 9];
						comPoneval.insert(nodeIDs[i], SBE);
					}
					break;
					case 10:
					{
						double SBF = values[index * 13 + 10];
						comPoneval.insert(nodeIDs[i], SBF);
					}
					break;
					case 11:
					{
						double SBMIN = values[index * 13 + 11];
						comPoneval.insert(nodeIDs[i], SBMIN);
					}
					break;
					case 12:
					{
						double SBMAX = values[index * 13 + 12];
						comPoneval.insert(nodeIDs[i], SBMAX);
					}
					break;
					default:
						break;
					}
				}
				else
				{
					double Axial = values[index];
					comPoneval.insert(nodeIDs[i], Axial);
				}
				index++;
			}
		}
		return comPoneval;
	}

	QHash<int, float> mMxdbThread::calcuEleNodeComponeVal(float * values, int nums, const QStringList & lis, int * ids, int varIndex, QPointer<mPostModelData> modelData)
	{
		QHash<int, float> comPoneval{};
		if (lis.size() != 13 && lis.size() != 1)
			return comPoneval;

		if (values == nullptr)
		{
			return comPoneval;
		}

		int index = 0;
		for (int p = 0; p < nums; ++p)
		{
			int eleID = ids[p];

			//获取单元包含的所有节点ID
			int partID = modelData->getPartIDByEleID(eleID);
			QList<int> tempNodeids = modelData->getElementNodeId(partID, eleID);
			QList<int> nodeIDs{};
			for (auto it : tempNodeids)
			{
				nodeIDs.append(modelData->_nodedata.ids[it]);
			}
			QVector3D newVec;
			for (int i = 0; i < nodeIDs.size(); i++)
			{
				if (lis.size() == 13)
				{
					switch (varIndex)
					{
					case 0:
					{
						float Axial = values[index * 13];
						comPoneval.insert(nodeIDs[i], Axial);
					}
					break;
					case 1:
					{
						float SAC = values[index * 13 + 1];
						comPoneval.insert(nodeIDs[i], SAC);
					}
					break;
					case 2:
					{
						float SAD = values[index * 13 + 2];
						comPoneval.insert(nodeIDs[i], SAD);
					}
					break;
					case 3:
					{
						float SAE = values[index * 13 + 3];
						comPoneval.insert(nodeIDs[i], SAE);
					}
					break;
					case 4:
					{
						float SAF = values[index * 13 + 4];
						comPoneval.insert(nodeIDs[i], SAF);
					}
					break;
					case 5:
					{
						float SAMIN = values[index * 13 + 5];
						comPoneval.insert(nodeIDs[i], SAMIN);
					}
					break;
					case 6:
					{
						float SAMAX = values[index * 13 + 6];
						comPoneval.insert(nodeIDs[i], SAMAX);
					}
					break;
					case 7:
					{
						float SBC = values[index * 13 + 7];
						comPoneval.insert(nodeIDs[i], SBC);
					}
					break;
					case 8:
					{
						float SBD = values[index * 13 + 8];
						comPoneval.insert(nodeIDs[i], SBD);
					}
					break;
					case 9:
					{
						float SBE = values[index * 13 + 9];
						comPoneval.insert(nodeIDs[i], SBE);
					}
					break;
					case 10:
					{
						float SBF = values[index * 13 + 10];
						comPoneval.insert(nodeIDs[i], SBF);
					}
					break;
					case 11:
					{
						float SBMIN = values[index * 13 + 11];
						comPoneval.insert(nodeIDs[i], SBMIN);
					}
					break;
					case 12:
					{
						float SBMAX = values[index * 13 + 12];
						comPoneval.insert(nodeIDs[i], SBMAX);
					}
					break;
					default:
						break;
					}
				}
				else
				{
					float Axial = values[index];
					comPoneval.insert(nodeIDs[i], Axial);
				}
				index++;
			}
		}
		return comPoneval;
	}

	bool mMxdbThread::ensureReaderOpen(const QString& filename)
	{
		if (filename.trimmed().isEmpty())
			return false;
		//同文件已打开：直接复用（帧索引/版本/消息回调均有效；重复 open 会全文件重扫建索引）
		if (_reader && _openedReaderFile == filename.trimmed())
			return true;
		if (!_reader) {
			// 先读文件头版本号决定 reader：601/602 为新版记录式格式（v07.mxdb 规范，
			// 与产品线无关），其余（201/202）为旧版单文件格式
			unsigned int headVersion = 0;
			{
				std::ifstream headStream;
#ifdef _WIN32
				headStream.open(filename.trimmed().toStdWString(), std::ios::in | std::ios::binary);
#else
				headStream.open(filename.trimmed().toLocal8Bit().constData(), std::ios::in | std::ios::binary);
#endif
				if (headStream)
					headStream.read(reinterpret_cast<char*>(&headVersion), sizeof(headVersion));
			}
			if (headVersion == 0xA1B1C601 || headVersion == 0xA1B1C602)
				_reader = std::make_unique<MxdbReader::NewFormatMxdbReader>(_mxdbStream);
			else
				_reader = std::make_unique<NewSingleMxdbReader>(_mxdbStream);
		}
		if (!_reader->open(filename.trimmed().toStdWString()))
		{
			qWarning() << "mMxdbThread::ensureReaderOpen: failed to open" << filename;
			_openedReaderFile.clear();
			return false;
		}
		_version = _reader->version();
		_openedReaderFile = filename.trimmed();

		_reader->setMessageCallback([](MxdbMessageLevel level, MxdbMessageCode code, const std::vector<std::string>& args) {
			if (level == MxdbMessageLevel::Info && code == MxdbMessageCode::ReadingPartInfo) return;
			const QString msg = translateMxdbMessage(code, args);
			if (msg.isEmpty()) return;
			int messageType = 0;
			if (level == MxdbMessageLevel::Warning) messageType = 1;
			else if (level == MxdbMessageLevel::Error) messageType = 2;
			mGlobalSignals::getInstance()->outputMessageSig(messageType, msg, true);
		});
		return true;
	}

	void mMxdbThread::convertFrameVariables(const std::vector<MxdbReader::MxdbVariable>& vars, int frameId)
	{
		QPointer<mPostModelData> modelData = nullptr;
		if (frameId == 0) {
			// 初始帧注册在 _modelDatabase[1]
			modelData = _modeldb->getModelData(1);
		} else {
			modelData = _modeldb->getModelData(frameId);
		}
		if (modelData == nullptr)
			return;
		PostFrameInfo frameInfo = modelData->_frameInfo;
		auto& fileCache = g_varByteCacheByFile[mPathName];
		const bool newFormat = isNewFormatFile();
		for (const auto& v : vars)
		{
			if (v.resultType != 0)   // 特殊/载荷变量不在 _frameVars 中
				continue;
			// 新格式整体变量（target=2，如总能量）首版不注册显示，仅提示一次
			if (newFormat && v.varType == 2) {
				if (!_newFormatWholeVarWarned) {
					_newFormatWholeVarWarned = true;
					mGlobalSignals::getInstance()->outputMessageSig(1,
						tr("整体变量（如%1）暂不支持显示，已跳过。").arg(QString::fromStdString(v.name)), true);
				}
				continue;
			}
			// 定位键：节点变量为值区偏移（varByte）；新格式单元变量 varByte=0，
			// 借首分组值区偏移（dataOffset，全文件唯一）做键，供 readLegacyVarHead
			// 按需读取时查回完整变量头（含 resultGroups）
			long long varKey = static_cast<long long>(v.varByte);
			if (newFormat && v.varType == 1 && !v.resultGroups.empty() && varKey <= 0)
				varKey = static_cast<long long>(v.resultGroups.front().dataOffset);
			// 填充 readLegacyVarHead/Values 委托缓存（跨帧 varByte 全局唯一）
			fileCache.insert(varKey, v);
			PostVariableInfo info;
			info._resultType = v.resultType;
			info._varName = QString::fromStdString(v.name);
			info._varType = v.varType;
			info._varByte = varKey;
			QStringList compoNames;
			if (newFormat) {
				// 新格式变量头带真实分量名；单元变量分量在各分组内，取并集保持信息完整
				// （某分组无该分量时该组单元显示 0，首版策略）
				if (v.varType == 1 && !v.resultGroups.empty()) {
					for (const auto& grp : v.resultGroups)
						for (const auto& comp : grp.components) {
							const QString cn = QString::fromStdString(comp.name);
							if (!cn.isEmpty() && !compoNames.contains(cn))
								compoNames << cn;
						}
				} else {
					for (const auto& comp : v.components) {
						const QString cn = QString::fromStdString(comp.name);
						if (!cn.isEmpty())
							compoNames << cn;
					}
				}
				if (compoNames.isEmpty())
					compoNames << info._varName;
			}
			else if (_varStringList1.contains(info._varName))
				compoNames = _varStringList1.value(info._varName);
			else if (v.componentCount > 1)
				for (int c = 0; c < v.componentCount; ++c)
					compoNames << QString("C%1").arg(c);
			else
				compoNames << info._varName;
			info._compoNames = compoNames;
			info._nCompo = compoNames.size();
			// 旧版文件变量头中没有 isMag 字段，按分量名推断；新版保留文件中的 isMag
			info._isMag = v.isMag || inferMagFromComponents(info._varName, compoNames);
			QHash<QString, bool> compoplex;
			for (const auto& cn : compoNames)
				compoplex.insert(cn, false);
			if (newFormat) {
				// 新格式分量带逐分量复数标记（读值时实虚部都在）
				for (const auto& grp : v.resultGroups)
					for (const auto& comp : grp.components) {
						const QString cn = QString::fromStdString(comp.name);
						if (comp.isComplex && compoplex.contains(cn))
							compoplex[cn] = true;
					}
				for (const auto& comp : v.components) {
					const QString cn = QString::fromStdString(comp.name);
					if (comp.isComplex && compoplex.contains(cn))
						compoplex[cn] = true;
				}
			}
			info._compoComplex = compoplex;
			if (newFormat && v.varType == 1 && !v.resultGroups.empty()) {
				// 新格式单元变量层 = 结果分组截面点名并集（规范：名为空按编号生成）；
				// 值读取取第一结果位置的该截面点分量
				QStringList layerNames;
				for (const auto& grp : v.resultGroups) {
					int idx = 0;
					for (const auto& sp : grp.sectionPointNames) {
						QString name = QString::fromStdString(sp);
						if (name.isEmpty())
							name = tr("截面点%1").arg(idx + 1);
						if (!layerNames.contains(name))
							layerNames << name;
						++idx;
					}
				}
				if (layerNames.isEmpty())
					layerNames << QStringLiteral("1");
				info._varCount = 1;
				info._varLayer = { layerNames.size() };
				info._varLayerNames1 = { layerNames };
				info._varLayerNames = layerNames;
			}
			// 旧版壳单元变量：1 shape / Top-Bottom 2 层（16 槽 = 8 分量 × 2 层）
			else if (v.varType == 1 && info._nCompo >= 6) {
				info._varCount = 1;
				info._varLayer = { 2 };
				info._varLayerNames1 = { QStringList{"Top", "Bottom"} };
				info._varLayerNames = QStringList{"Top", "Bottom"};
			} else {
				info._varCount = 1;
				info._varLayer = { 0 };
				info._varLayerNames1 = { QStringList{} };
			}
			frameInfo._frameVars.append(info);
		}
		modelData->_frameInfo = frameInfo;
	}

	void mMxdbThread::fillLegacyDisplacement(MxdbReader::NewSingleMxdbReader* legacyReader, int frameId)
	{
		if (legacyReader == nullptr)
			return;
		// 找当前帧的 Displacement 变量（名字兼容两种拼写）
		const MxdbReader::MxdbVariable* dispVar = nullptr;
		for (const auto& v : legacyReader->currentFrameVariables()) {
			if (v.name == "Displacement" || v.name == "Displacment") {
				dispVar = &v;
				break;
			}
		}
		if (dispVar == nullptr)
			return;
		std::vector<MxdbReader::MxdbVariableValue> values;
		try {
			values = legacyReader->readVariableValues(*dispVar, {});
		} catch (const std::exception& e) {
			// 损坏文件可抛 bad_alloc 等（垃圾 entityCount 触发超大分配）；跳过位移预读而非中断导入
			qWarning() << "mMxdbThread::fillLegacyDisplacement: readVariableValues exception:" << e.what();
			mGlobalSignals::getInstance()->outputMessageSig(2,
				tr("位移数据读取异常（%1），文件可能已损坏！").arg(e.what()), true);
			return;
		}
		if (values.empty())
			return;
		QPointer<mPostModelData> modelData = _modeldb->getModelData(frameId);
		if (modelData == nullptr)
			return;
		QHash<int, QVector3D> displacementR;
		displacementR.reserve(static_cast<int>(values.size()));
		for (const auto& val : values) {
			if (val.realValues.size() < 3)
				continue;
			int cid = modelData->_nodedata.nodeidmap.value(val.entityId, -1);
			if (cid < 0)
				continue;
			displacementR.insert(cid, QVector3D(val.realValues[0], val.realValues[1], val.realValues[2]));
		}
		modelData->DisplacementValuesR = displacementR;
		_modeldb->setDisplacementDataByFrameID(frameId, displacementR);
	}

	MxdbReader::MxdbVariable mMxdbThread::convertToMxdbVariable(const PostData::PostVariableInfo& info) const
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

	bool mMxdbThread::readVariableValuesCore(const PostData::PostVariableInfo& varInfo,
		const std::vector<int>& entityIds,
		std::vector<MxdbReader::MxdbVariableValue>& outValues)
	{
		if (!_reader)
			return false;
		// 新格式：convertToMxdbVariable 反构会丢单元变量 resultGroups（值定位必需），
		// 优先用导入期缓存的完整变量头（键 = _varByte，单元变量为首分组 dataOffset）
		MxdbReader::MxdbVariable var;
		const auto& fileCache = g_varByteCacheByFile[mPathName];
		const auto cached = fileCache.constFind(varInfo._varByte);
		if (cached != fileCache.constEnd() &&
			cached.value().name == varInfo._varName.toStdString())
		{
			var = cached.value();
		}
		else
		{
			var = convertToMxdbVariable(varInfo);
		}
		try {
			outValues = _reader->readVariableValues(var, entityIds);
		} catch (const std::exception& e) {
			// 损坏文件可抛 bad_alloc 等（垃圾 entityCount 触发超大分配）；判损坏报错而非让按需读取崩溃
			qWarning() << "mMxdbThread::readVariableValuesCore: readVariableValues exception:" << e.what();
			mGlobalSignals::getInstance()->outputMessageSig(2,
				tr("变量%1数值段读取异常（%2），文件可能已损坏！").arg(varInfo._varName).arg(e.what()), true);
			return false;
		}
		return true;
	}

	bool mMxdbThread::readNewFormatScalarValues(const PostData::PostVariableInfo& varInfo,
		const QString& compoName, const QString& layer, const QString& complex,
		QHash<int, double>& outVals, int& valueType,
		PostData::mPostModelData* modelData)
	{
		outVals.clear();
		valueType = varInfo._varType; // 0 节点 / 1 单元
		if (!_reader)
			return false;
		const auto& fileCache = g_varByteCacheByFile[mPathName];
		const auto it = fileCache.constFind(varInfo._varByte);
		if (it == fileCache.constEnd())
		{
			qWarning().noquote() << QString::fromUtf8(
				"readNewFormatScalarValues: 缓存未命中 var=%1 key=%2 桶大小=%3 全局键数=%4 file=%5")
				.arg(varInfo._varName).arg(varInfo._varByte).arg(fileCache.size())
				.arg(g_varByteCacheByFile.size()).arg(mPathName);
			return false;
		}
		const MxdbReader::MxdbVariable& var = it.value();
		const int cIdx = varInfo._compoNames.indexOf(compoName);
		// "Mag" 为 UI 派生分量（getVariableCompos 对 isMag 变量自动追加），按矢量模合成
		const bool wantMag = (cIdx < 0 && compoName == QString("Mag"));
		if (cIdx < 0 && !wantMag)
			return false;
		std::vector<MxdbReader::MxdbVariableValue> values;
		try {
			values = _reader->readVariableValues(var, {});
		} catch (const std::exception& e) {
			qWarning() << "mMxdbThread::readNewFormatScalarValues: readVariableValues exception:" << e.what();
			mGlobalSignals::getInstance()->outputMessageSig(2,
				tr("变量%1数值段读取异常（%2），文件可能已损坏！").arg(varInfo._varName).arg(e.what()), true);
			return false;
		}
		// 复数合成方式（口径与旧链路一致：相位 = atan(虚/实)*180/π + 180）
		enum ComplexMode { CReal, CImag, CMag, CPhase };
		ComplexMode mode = CReal;
		if (complex == QString("虚部") || complex == QString("Img"))
			mode = CImag;
		else if (complex == QString("幅值") || complex == QString("Amplitude"))
			mode = CMag;
		else if (complex == QString("相位") || complex == QString("Phase"))
			mode = CPhase;
		const bool isCellVar = var.varType == 1 && !var.resultGroups.empty();
		for (const auto& val : values) {
			// 取值区间 [base, base+count)：
			//   节点变量：每实体平铺 [ncomp]，Mag 时聚合全部分量
			//   单元变量：值区 = 结果位置 × [截面点数×分组分量数]；首版取第一结果位置，
			//             本截面点段为 [sectionIdx*ncomp, (sectionIdx+1)*ncomp)，
			//             组内无该分量（分量并集超出本组）时填 0
			int base = -1;
			int count = 1;
			if (isCellVar) {
				const auto& grp = (val.blockIndex >= 0 &&
					val.blockIndex < static_cast<int>(var.resultGroups.size()))
					? var.resultGroups[static_cast<size_t>(val.blockIndex)]
					: var.resultGroups.front();
				int inGroupIdx = -1;
				for (size_t c = 0; c < grp.components.size(); ++c) {
					if (QString::fromStdString(grp.components[c].name) == compoName) {
						inGroupIdx = static_cast<int>(c);
						break;
					}
				}
				if (inGroupIdx < 0 && !wantMag) {
					outVals.insert(val.entityId, 0.0);
					continue;
				}
				int sectionIdx = 0;
				if (!grp.sectionPointNames.empty()) {
					int idx = 0;
					sectionIdx = -1;
					for (const auto& sp : grp.sectionPointNames) {
						QString name = QString::fromStdString(sp);
						if (name.isEmpty())
							name = tr("截面点%1").arg(idx + 1);
						if (name == layer) {
							sectionIdx = idx;
							break;
						}
						++idx;
					}
					if (sectionIdx < 0)
						sectionIdx = 0; // 层名不匹配取第一截面点
				}
				const int ncomp = static_cast<int>(grp.components.size());
				base = sectionIdx * ncomp;
				if (wantMag)
					count = ncomp;
				else
					base += inGroupIdx;
			} else {
				// Mag（矢量模）从第 0 分量起聚合全部分量；普通分量用分量序
				base = wantMag ? 0 : cIdx;
				if (wantMag)
					count = std::max(1, static_cast<int>(var.components.size()));
			}
			const int nReal = static_cast<int>(val.realValues.size());
			const bool isComplex = !val.imagValues.empty();
			const int nImag = isComplex ? static_cast<int>(val.imagValues.size()) : 0;
			double out = 0.0;
			if (wantMag) {
				// 矢量模（口径与旧链路一致：复数时实虚部共同计入模）
				double sum = 0.0;
				for (int k = base; k >= 0 && k < base + count && k < nReal; ++k) {
					const double rv = val.realValues[static_cast<size_t>(k)];
					sum += rv * rv;
					if (isComplex && k < nImag) {
						const double iv = val.imagValues[static_cast<size_t>(k)];
						sum += iv * iv;
					}
				}
				out = std::sqrt(sum);
			} else {
				const double rv = (base >= 0 && base < nReal)
					? val.realValues[static_cast<size_t>(base)] : 0.0;
				const double iv = (isComplex && base >= 0 && base < nImag)
					? val.imagValues[static_cast<size_t>(base)] : 0.0;
				switch (mode) {
				case CImag:  out = iv; break;
				case CMag:   out = isComplex ? std::sqrt(rv * rv + iv * iv) : rv; break;
				case CPhase: out = isComplex ? std::atan(iv / rv) * 180.0 / 3.1415926 + 180.0 : 0.0; break;
				default:     out = rv; break;
				}
			}
			// 键契约：节点值用连续 cid（v07 节点 ID 可稀疏，渲染端 _raw_value 按
			// cid 下标直取，文件 ID 必须经 nodeidmap 转换）；单元值用文件单元 ID
			// （消费端按 cell_rid 哈希查找）
			if (!isCellVar && modelData != nullptr) {
				const int cid = modelData->_nodedata.nodeidmap.value(val.entityId, -1);
				if (cid < 0)
					continue;
				outVals.insert(cid, out);
			} else {
				outVals.insert(val.entityId, out);
			}
		}
		return !outVals.isEmpty();
	}

	bool mMxdbThread::readVarValuesToArrays(const PostData::PostVariableInfo& curVarInfo,
		const std::vector<int>& entityIds,
		std::vector<int>& ids,
		std::vector<double>& real,
		std::vector<double>& imag)
	{
		std::vector<MxdbReader::MxdbVariableValue> values;
		if (!readVariableValuesCore(curVarInfo, entityIds, values))
			return false;
		const int ncompo = curVarInfo._nCompo > 0 ? curVarInfo._nCompo : 1;
		ids.reserve(values.size());
		real.reserve(values.size() * ncompo);
		imag.reserve(values.size() * ncompo);
		for (const auto& v : values) {
			ids.push_back(v.entityId);
			for (int c = 0; c < ncompo; ++c) {
				real.push_back(c < (int)v.realValues.size() ? v.realValues[c] : 0.0);
				if (!v.imagValues.empty())
					imag.push_back(c < (int)v.imagValues.size() ? v.imagValues[c] : 0.0);
			}
		}
		return true;
	}



	// 损坏文件防御：GUI 直读的计数字段（如两段式块头的实体数）超出文件剩余字节
	// 能容纳的上限时判文件损坏并报错，避免按垃圾计数 assign/分配内存抛 bad_alloc
	// 使按需读取（曲线/云图等）直接崩溃。minBytesPerRecord 为每条记录的最小字节数。
	// 文件大小用流自身 seek 探测（保存/恢复当前位置），与实际打开的文件严格一致。
	static bool countFitsRemainingBytes(std::ifstream& stream,
		const QString& what, long long count, unsigned long long minBytesPerRecord)
	{
		if (count < 0 || minBytesPerRecord == 0)
			return false;
		const std::streampos cur = stream.tellg();
		if (cur < 0)
			return true; // 流位置不可用时放行，维持旧路径行为
		stream.seekg(0, std::ios::end);
		const std::streampos endPos = stream.tellg();
		stream.seekg(cur);
		if (endPos < 0)
		{
			stream.clear();
			stream.seekg(cur);
			return true;
		}
		const unsigned long long curOff = static_cast<unsigned long long>(static_cast<std::streamoff>(cur));
		const unsigned long long endOff = static_cast<unsigned long long>(static_cast<std::streamoff>(endPos));
		const unsigned long long remaining = endOff > curOff ? endOff - curOff : 0ull;
		if (static_cast<unsigned long long>(count) <= remaining / minBytesPerRecord)
			return true;
		const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
		mGlobalSignals::getInstance()->outputMessageSig(2,
			isChinese ? QString("结果文件数据块%1的数量%2非法（超出文件剩余字节），文件可能已损坏！").arg(what).arg(count)
				: QString("Result file block %1 count %2 is invalid (exceeds remaining file bytes); the file may be corrupted!").arg(what).arg(count),
			true);
		return false;
	}

	bool mMxdbThread::readLegacyVarHead(long long varByte, int& typeOut, std::string& nameOut)
	{
		typeOut = -1;
		nameOut.clear();
		// 新格式（601/602）不走旧版两段式块头委托（值布局不同，直接套用会错位取值）；
		// 已分流的新格式入口统一走 readNewFormatScalarValues，未分流路径在此安全失败
		if (isNewFormatFile())
			return false;
		auto& fileCache = g_varByteCacheByFile[mPathName];
		// 按需读取时 MainWindow 新建 mMxdbThread，全局缓存可能尚未填充。
		// 仅当按文件路径的缓存也为空时才回退到读初始帧（导入后不会走到这里）。
		if (fileCache.isEmpty() && _reader) {
			auto* legacyReader = dynamic_cast<MxdbReader::NewSingleMxdbReader*>(_reader.get());
			if (legacyReader != nullptr) {
				try {
					legacyReader->readInitialFrame();
				} catch (const std::exception& e) {
					// 损坏文件可抛 bad_alloc 等（垃圾数量字段触发超大 resize）；判损坏返回空而非崩溃
					qWarning() << "mMxdbThread::readLegacyVarHead: readInitialFrame exception:" << e.what();
					return false;
				}
				for (const auto& v : legacyReader->currentFrameVariables()) {
					if (v.resultType == 0)
						fileCache.insert(static_cast<long long>(v.varByte), v);
				}
			}
		}
		// 委托：从全局按文件路径缓存查询（跨帧 varByte 全局唯一）
		const auto it = fileCache.constFind(varByte);
		if (it == fileCache.constEnd())
			return false;
		nameOut = it->name;
		bool isComplex = false;
		for (const auto& comp : it->components) {
			if (comp.isComplex) { isComplex = true; break; }
		}
		typeOut = isComplex ? 1 : 0;
		// 记录当前变量供 readLegacyVarValues 使用
		_lastHeadVarByte = varByte;
		return true;
	}

	bool mMxdbThread::readLegacyVarValues(int vpE1, int vpE2, bool isComplex,
		std::vector<int>& ids1, std::vector<double>& real1, std::vector<double>& imag1,
		std::vector<int>& ids2, std::vector<double>& real2, std::vector<double>& imag2)
	{
		// (0,0) 两段式（Stress(N)/Beam Stress(N)）：201/202 该布局无样本无法确认，
		// 保持旧的流读取（readLegacyVarBlockHead/Values 保留至有样本后迁移）
		if (vpE1 == 0 && vpE2 == 0) {
			// readLegacyVarHead 委托后不再 seekg，此处补定位（跳过 varType + varName 块头）
			_mxdbStream.clear();
			_mxdbStream.seekg(_lastHeadVarByte, std::ios::beg);
			int headType = 0;
			_mxdbStream.read(reinterpret_cast<char*>(&headType), sizeof(int));
			int headNameLen = 0;
			_mxdbStream.read(reinterpret_cast<char*>(&headNameLen), sizeof(int));
			if (headNameLen > 0 && headNameLen <= 4096)
				_mxdbStream.seekg(headNameLen, std::ios::cur);
			int nums1 = 0;
			if (!readLegacyVarBlockHead(false, ids1, nums1))
				return false;
			int nums2 = 0;
			if (!readLegacyVarBlockHead(true, ids2, nums2))
				return false;
			return true;
		}
		// 委托：经 _reader 读取当前变量（readLegacyVarHead 记录），按 blockIndex 分实体/壳段摊平
		const auto& fileCache = g_varByteCacheByFile[mPathName];
		const auto it = fileCache.constFind(_lastHeadVarByte);
		if (it == fileCache.constEnd() || !_reader)
			return false;
		std::vector<MxdbReader::MxdbVariableValue> values;
		try {
			values = _reader->readVariableValues(*it, {});
		} catch (const std::exception& e) {
			// 损坏文件可抛 bad_alloc 等（垃圾 entityCount 触发超大分配）；判损坏报错而非让按需读取崩溃
			qWarning() << "mMxdbThread::readLegacyVarValues: readVariableValues exception:" << e.what();
			mGlobalSignals::getInstance()->outputMessageSig(2,
				tr("变量%1数值段读取异常（%2），文件可能已损坏！").arg(QString::fromStdString(it->name)).arg(e.what()), true);
			return false;
		}
		ids1.clear(); real1.clear(); imag1.clear();
		ids2.clear(); real2.clear(); imag2.clear();
		for (const auto& v : values) {
			const int vpE = (v.blockIndex == 0) ? vpE1 : vpE2;
			std::vector<int>& ids = (v.blockIndex == 0) ? ids1 : ids2;
			std::vector<double>& real = (v.blockIndex == 0) ? real1 : real2;
			std::vector<double>& imag = (v.blockIndex == 0) ? imag1 : imag2;
			ids.push_back(v.entityId);
			for (int c = 0; c < vpE; ++c) {
				real.push_back(c < static_cast<int>(v.realValues.size()) ? v.realValues[c] : 0.0);
			}
			if (isComplex) {
				for (int c = 0; c < vpE; ++c) {
					imag.push_back(c < static_cast<int>(v.imagValues.size()) ? v.imagValues[c] : 0.0);
				}
			}
		}
		return true;
	}

	bool mMxdbThread::readLegacyVarBlockHead(bool secondBlock, std::vector<int>& ids, int& numsOut)
	{
		numsOut = 0;
		_mxdbStream.clear();
		int nums = 0;
		_mxdbStream.read(reinterpret_cast<char*>(&nums), sizeof(int));
		if (!_mxdbStream || nums < 0)
			return false;
		// 损坏文件防御：实体数超出剩余字节可容纳上限（每实体至少 id 4 字节）时判损坏报错
		if (!countFitsRemainingBytes(_mxdbStream, QStringLiteral("entityCount"), nums, sizeof(int)))
			return false;
		numsOut = nums;
		ids.assign(nums, 0);
		if (nums > 0)
			_mxdbStream.read(reinterpret_cast<char*>(ids.data()), nums * sizeof(int));
		return static_cast<bool>(_mxdbStream);
	}

	bool mMxdbThread::readLegacyVarBlockValues(int count, bool isComplex, std::vector<double>& real, std::vector<double>& imag)
	{
		if (count < 0)
			return false;
		// 损坏文件防御：数量超出剩余字节可容纳上限（每值至少 4 字节）时判损坏报错
		if (!countFitsRemainingBytes(_mxdbStream, QStringLiteral("entityCount"), count, sizeof(float)))
			return false;
		real.assign(count, 0.0);
		const bool isD = DataPrecisionIsDouble();
		for (int i = 0; i < count; ++i) {
			if (isD) {
				double v = 0.0;
				_mxdbStream.read(reinterpret_cast<char*>(&v), sizeof(double));
				real[i] = v;
			} else {
				float v = 0.0f;
				_mxdbStream.read(reinterpret_cast<char*>(&v), sizeof(float));
				real[i] = static_cast<double>(v);
			}
		}
		if (isComplex) {
			imag.assign(count, 0.0);
			for (int i = 0; i < count; ++i) {
				if (isD) {
					double v = 0.0;
					_mxdbStream.read(reinterpret_cast<char*>(&v), sizeof(double));
					imag[i] = v;
				} else {
					float v = 0.0f;
					_mxdbStream.read(reinterpret_cast<char*>(&v), sizeof(float));
					imag[i] = static_cast<double>(v);
				}
			}
		} else {
			imag.clear();
		}
		return static_cast<bool>(_mxdbStream);
	}

	bool mMxdbThread::readElementNodeFaceValuesCore(const PostData::PostVariableInfo& varInfo,
		PostData::mPostModelData* modelData,
		const std::vector<int>& elementIds,
		std::vector<MxdbReader::MxdbElementNodeFaceValue>& outValues)
	{
		if (!_reader || modelData == nullptr)
			return false;
		MxdbReader::MxdbVariable var = convertToMxdbVariable(varInfo);
		auto elementInfoProvider = [modelData](int elementId) {
			MxdbReader::MxdbElementNodeFaceInfo info;
			MxFunctions::ElementType eleType = modelData->getEleTypeByEleId(elementId);
			QString str = mElementFunction::getElementMainType(eleType);
			if (str == QString("梁") || str == QString("杆") || str == QString("壳"))
				info.comVarCount = 6;
			else if (str == QString("平面"))
				info.comVarCount = 2;
			else if (str == QString("实体") || str == QString("实体壳") || str == QString("膜") || str == QString("三维实体"))
				info.comVarCount = 3;
			else if (str == QString("剪切板"))
				info.comVarCount = 5;
			QVector<int> nodeIds = modelData->getNodeIdsByEleId(elementId);
			info.nodeCount = nodeIds.size();
			info.nodeIds.reserve(static_cast<size_t>(nodeIds.size()));
			for (int id : nodeIds)
				info.nodeIds.push_back(id);
			return info;
		};
		try {
			outValues = _reader->readElementNodeFaceValues(var, elementInfoProvider, elementIds);
		} catch (const std::exception& e) {
			// 损坏文件可抛 bad_alloc 等（垃圾数量字段触发超大分配）；判损坏报错而非让按需读取崩溃
			qWarning() << "mMxdbThread::readElementNodeFaceValuesCore: readElementNodeFaceValues exception:" << e.what();
			mGlobalSignals::getInstance()->outputMessageSig(2,
				tr("变量%1单元节点面数值段读取异常（%2），文件可能已损坏！").arg(varInfo._varName).arg(e.what()), true);
			return false;
		}
		return true;
	}

	void mMxdbThread::run()
	{
		mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);

		//[pws]接入测试数据的地方
		bool isSuccess = false;
		try {
			isSuccess = readModelAndFrameInfo();
		} catch (const std::exception& e) {
			fprintf(stderr, "mMxdbThread::run: exception in readModelAndFrameInfo: %s\n", e.what());
			fflush(stderr);
		} catch (...) {
			fprintf(stderr, "mMxdbThread::run: unknown exception in readModelAndFrameInfo\n");
			fflush(stderr);
		}
		if (!isSuccess)
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("后处理文件%1读取失败！").arg(mPathName), true);
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("Failed to read post-processing file %1!").arg(mPathName), true);
			}
			mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
			return;
		}
		_inpFlag = true;

		QString currentTime{};

		currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,tr("成功读取文件%1！").arg(mPathName));
		}
		else
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,tr("File %1 was successfully read!").arg(mPathName));
		}
		emit mxdbReadFinished(mModelName, mPathName);

		mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
	}


	bool mMxdbThread::isMatchedVersion()
	{
		return _reader != nullptr && (_version == 0xA1B1C201 || _version == 0xA1B1C202
			|| _version == 0xA1B1C601 || _version == 0xA1B1C602);
	}

	void mMxdbThread::moveReader(int f)
	{
		Q_UNUSED(f);
	}
	bool mMxdbThread::openFileStatus(const QString& filename)
	{
		return ensureReaderOpen(filename);
	}
}
