//---------------------------------------
// 临时功能：后处理线段-网格求交薄适配器
// 算法本体在渲染库 mxr::postalg::SegmentMeshIntersection（金样本单测锁定行为，
// 边界判定看似怪异但为有意行为，勿在任一侧"顺手修正"）。
// 本文件只做三件事：取数组装中性快照、端点真实 ID 换算连续 ID、QVector3D
// 与 glm::vec3 的出入参边界转换。
//---------------------------------------
#include "ZsTempRender.h"
#include "mxr/postalg/SegmentMeshIntersection.h"

IOFILE_NAMESPACE_BEGIN

ZsTempRender::~ZsTempRender()
{
}

namespace {

// QVector3D 与 glm::vec3 的边界转换（本文件唯一转换点：逐分量直拷，不做任何数值运算）
glm::vec3 toGlm(const QVector3D& v)
{
	return glm::vec3(v.x(), v.y(), v.z());
}

QVector3D toQt(const glm::vec3& v)
{
	return QVector3D(v.x, v.y, v.z);
}

// 取数段：组装中性快照。快照契约详见 mxr/postalg/SegmentMeshIntersection.h 头注释：
//   - 节点坐标在此烘焙形变（原始坐标 + 形变比例 × 位移；位移缺失视为零位移），
//     算法侧不再消费位移场与形变比例
//   - 面表下标 = 部件内局部面编号（从 0 起），与 elem_faces 里的面 ID 同域
//   - faces_by_rid / faces_by_cid 双面表承载两类查表口径：前者用真实单元 ID 直查
//     以连续单元 ID 为键的面表（未命中得空表，该单元零命中），后者先换算连续 ID
//     再查——真实数据两表通常相同，rid≠cid 的数据上行为真实不同，必须分别填
//   - dim2/dim3 单元各按单元表序存放（旧接口每次调用先清空再按维重填，维度间
//     无跨态，两个列表等价承载）
mxr::postalg::SegmentMeshSnapshot buildSegmentMeshSnapshot(
	int frame_id,
	PostData::mPostModelData* modeldata,
	PostData::mPostDataBase* database,
	mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
	mxr::IPostModelRenderDataProxy* postModelRenderDataProxy
)
{
	const auto& dis_scale = postRendQueryDataProxy->getDeformationScale();	// 形变缩放（按轴分量）
	const auto& dis = database->getDisplacementDataByFrameID(frame_id);		// 形变位移，键=节点连续ID（缺失视为零位移）
	const auto& part_ids = modeldata->getAllPartsIDs();

	mxr::postalg::SegmentMeshSnapshot snapshot;

	// 节点表：连续节点 ID → 烘焙后坐标 / 真实节点 ID
	const int node_count = modeldata->_nodedata.nodeamount;
	snapshot.node_positions.resize(node_count);
	snapshot.node_real_ids.resize(node_count);
	for (int cid = 0; cid < node_count; ++cid)
	{
		const QVector3D pos = modeldata->getVector(cid);	// 原始坐标（双精度源在函数内收窄为 float）
		const QVector3D d = dis.value(cid);					// 未命中得默认构造的零向量
		snapshot.node_positions[cid] = toGlm(QVector3D(
			pos.x() + dis_scale.x() * d.x(),
			pos.y() + dis_scale.y() * d.y(),
			pos.z() + dis_scale.z() * d.z()));
		snapshot.node_real_ids[cid] = modeldata->_nodedata.ids[cid];
	}

	snapshot.parts.reserve(part_ids.size());
	for (int part_id : part_ids)
	{
		const auto& part_data = modeldata->getPartData(part_id);
		if (part_data == nullptr)
			continue;

		mxr::postalg::SegmentMeshPart part;
		part.part_id = part_id;
		part.visual = part_data->getPartVisual();
		mxr::AABB loc_aabb;
		postModelRenderDataProxy->getBoundingBox(part_id, loc_aabb);
		part.aabb_min = toGlm(loc_aabb.minEdge);
		part.aabb_max = toGlm(loc_aabb.maxEdge);

		// 面表：face_node_starts/counts/ids 三数组直读（与 getContinueNodeIdsByRenderId 同值同序）
		const auto& face_data = part_data->_partfacedata;
		const auto& face_node_start_pos = face_data.face_node_starts;
		const int face_count = (int)face_data.face_node_counts.size();
		part.faces.resize(face_count);
		for (int face_id = 0; face_id < face_count; ++face_id)
		{
			const int start = face_node_start_pos[face_id];
			const int count = face_data.face_node_counts[face_id];
			part.faces[face_id].node_ids.assign(
				face_data.face_node_ids.begin() + start,
				face_data.face_node_ids.begin() + start + count);
		}

		// 单元表：按维筛选（遍历序=单元表序）
		for (int dim = 2; dim <= 3; ++dim)
		{
			std::vector<int> mesh_ids;
			modeldata->getElementIDsByPartIDAndDim(dim, part_id, mesh_ids);
			std::vector<mxr::postalg::SegmentMeshElement>& elements =
				(dim == 2) ? part.dim2_elements : part.dim3_elements;
			elements.resize(mesh_ids.size());
			for (int i = 0; i < (int)mesh_ids.size(); ++i)
			{
				const int mesh_rid = mesh_ids[i];
				mxr::postalg::SegmentMeshElement& ele = elements[i];
				ele.element_id = mesh_rid;
				ele.visual = part_data->getElementVisual(mesh_rid);
				// 真实单元 ID 直查（divide 版入口的查表口径：未命中=空表）
				for (int face_id : modeldata->getElemFace(part_id, mesh_rid))
					ele.faces_by_rid.push_back(face_id);
				// 先换算连续单元 ID 再查（排序交点版入口的查表口径；
				// cell_rid_to_cid 是 std::map，未命中按旧实现 operator[] 的读值得 0）
				const auto cid_iter = part_data->_parteledata.cell_rid_to_cid.find(mesh_rid);
				const int mesh_cid = (cid_iter != part_data->_parteledata.cell_rid_to_cid.end()) ? cid_iter->second : 0;
				for (int face_id : modeldata->getElemFace(part_id, mesh_cid))
					ele.faces_by_cid.push_back(face_id);
				// 单元全部节点（连续节点 ID / 全局点索引；getIntersectedNodeIDs 的输出内容，
				// 单元 ID 不在表内时得空表）
				const QList<int> elem_nodes = modeldata->getElementNodeId(part_id, mesh_rid);
				ele.node_ids.assign(elem_nodes.begin(), elem_nodes.end());
			}
		}
		snapshot.parts.push_back(std::move(part));
	}
	return snapshot;
}

// 每次调用装配算法实例并同步容差：setEpsilon 之后继续调本类其他入口，容差仍生效
mxr::postalg::SegmentMeshIntersection makeAlgorithm(float epsilon)
{
	mxr::postalg::SegmentMeshIntersection alg;
	alg.setEpsilon(epsilon);
	return alg;
}

} // namespace

// ---- 公有入口：组装快照 → 端点真实 ID 换算连续 ID → 转调算法 → 出参 glm→QVector3D ----

void ZsTempRender::chooseNode(
	int realID0, int realID1, int divide, int frame_id,
	PostData::mPostModelData* modeldata,
	PostData::mPostDataBase* database,
	mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
	mxr::IPostModelRenderDataProxy* postModelRenderDataProxy,
	std::vector<std::pair<int, int>>& out_ele_ids,	// <partID,meshID>
	std::vector<QVector3D>& out_point_poses,
	std::vector<std::set<int>>& out_point_in_ele
)
{
	const mxr::postalg::SegmentMeshSnapshot snapshot = buildSegmentMeshSnapshot(
		frame_id, modeldata, database, postRendQueryDataProxy, postModelRenderDataProxy);
	const int cid0 = modeldata->_nodedata.nodeidmap.value(realID0);
	const int cid1 = modeldata->_nodedata.nodeidmap.value(realID1);

	std::vector<glm::vec3> point_poses;
	auto alg = makeAlgorithm(EPSILON);
	alg.chooseNode(cid0, cid1, divide, snapshot, out_ele_ids, point_poses, out_point_in_ele);
	out_point_poses.reserve(point_poses.size());
	for (const auto& p : point_poses)
		out_point_poses.push_back(toQt(p));
}

void ZsTempRender::chooseNode(int realID0, int realID1, int frame_id, PostData::mPostModelData* modeldata, PostData::mPostDataBase* database, mxr::IPostRendQueryDataProxy* postRendQueryDataProxy, mxr::IPostModelRenderDataProxy* postModelRenderDataProxy, std::vector<std::pair<int, int>>& out_ele_ids, std::vector<QVector3D>& out_point_poses)
{
	const mxr::postalg::SegmentMeshSnapshot snapshot = buildSegmentMeshSnapshot(
		frame_id, modeldata, database, postRendQueryDataProxy, postModelRenderDataProxy);
	const int cid0 = modeldata->_nodedata.nodeidmap.value(realID0);
	const int cid1 = modeldata->_nodedata.nodeidmap.value(realID1);

	std::vector<glm::vec3> point_poses;
	auto alg = makeAlgorithm(EPSILON);
	alg.chooseNode(cid0, cid1, snapshot, out_ele_ids, point_poses);
	out_point_poses.reserve(point_poses.size());
	for (const auto& p : point_poses)
		out_point_poses.push_back(toQt(p));
}

void ZsTempRender::chooseNode(int realID0, int realID1, int frame_id, PostData::mPostModelData* modeldata, PostData::mPostDataBase* database,
    mxr::IPostRendQueryDataProxy* postRendQueryDataProxy, mxr::IPostModelRenderDataProxy* postModelRenderDataProxy,
    std::vector<std::pair<int, int>>& out_ele_ids, std::vector<QVector3D>& out_point_poses, std::map<QVector3D, std::pair<int, int>, CompareVectors>& out_point_to_element)
{
    std::map<QVector3D, HitPointInfo, CompareVectors> unused;
    chooseNode(realID0, realID1, frame_id, modeldata, database, postRendQueryDataProxy, postModelRenderDataProxy,
        out_ele_ids, out_point_poses, out_point_to_element, unused);
}

void ZsTempRender::chooseNode(int realID0, int realID1, int frame_id, PostData::mPostModelData* modeldata, PostData::mPostDataBase* database,
    mxr::IPostRendQueryDataProxy* postRendQueryDataProxy, mxr::IPostModelRenderDataProxy* postModelRenderDataProxy,
    std::vector<std::pair<int, int>>& out_ele_ids, std::vector<QVector3D>& out_point_poses,
    std::map<QVector3D, std::pair<int, int>, CompareVectors>& out_point_to_element,
    std::map<QVector3D, HitPointInfo, CompareVectors>& out_point_hit_info)
{
	const mxr::postalg::SegmentMeshSnapshot snapshot = buildSegmentMeshSnapshot(
		frame_id, modeldata, database, postRendQueryDataProxy, postModelRenderDataProxy);
	const int cid0 = modeldata->_nodedata.nodeidmap.value(realID0);
	const int cid1 = modeldata->_nodedata.nodeidmap.value(realID1);

	std::vector<glm::vec3> point_poses;
	std::map<glm::vec3, std::pair<int, int>, mxr::postalg::CompareVec3> point_to_element;
	std::map<glm::vec3, mxr::postalg::SegmentHitPointInfo, mxr::postalg::CompareVec3> point_hit_info;
	auto alg = makeAlgorithm(EPSILON);
	alg.chooseNode(cid0, cid1, snapshot, out_ele_ids, point_poses, point_to_element, point_hit_info);
	out_point_poses.reserve(point_poses.size());
	for (const auto& p : point_poses)
		out_point_poses.push_back(toQt(p));
	// 键转换不改变字典序：两个比较器都是 x→y→z 逐分量比大小
	for (const auto& kv : point_to_element)
		out_point_to_element[toQt(kv.first)] = kv.second;
	for (const auto& kv : point_hit_info)
	{
		HitPointInfo info;
		info.triNodeCIds = kv.second.triNodeCIds;
		info.triNodeRIds = kv.second.triNodeRIds;
		info.bary = kv.second.bary;
		out_point_hit_info[toQt(kv.first)] = info;
	}
}

std::set<int> ZsTempRender::getIntersectedNodeIDs(
	int realID0,
	int realID1,
	int frame_id,
	PostData::mPostModelData* modeldata,
	PostData::mPostDataBase* database,
	mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
	mxr::IPostModelRenderDataProxy* postModelRenderDataProxy
)
{
	std::set<int> result;
	if (!modeldata || !database || !postRendQueryDataProxy || !postModelRenderDataProxy)
		return result;

	const mxr::postalg::SegmentMeshSnapshot snapshot = buildSegmentMeshSnapshot(
		frame_id, modeldata, database, postRendQueryDataProxy, postModelRenderDataProxy);
	const int cid0 = modeldata->_nodedata.nodeidmap.value(realID0);
	const int cid1 = modeldata->_nodedata.nodeidmap.value(realID1);

	auto alg = makeAlgorithm(EPSILON);
	return alg.getIntersectedNodeIDs(cid0, cid1, snapshot);
}

std::vector<QVector3D> ZsTempRender::getIntersectedPoints(
	int realID0,
	int realID1,
	int frame_id,
	PostData::mPostModelData* modeldata,
	PostData::mPostDataBase* database,
	mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
	mxr::IPostModelRenderDataProxy* postModelRenderDataProxy
)
{
	std::vector<QVector3D> result;
	if (!modeldata || !database || !postRendQueryDataProxy || !postModelRenderDataProxy)
		return result;

	std::vector<std::pair<int, int>> ele_ids;
	chooseNode(realID0, realID1, frame_id, modeldata, database,
		postRendQueryDataProxy, postModelRenderDataProxy, ele_ids, result);
	return result;
}

IOFILE_NAMESPACE_END
