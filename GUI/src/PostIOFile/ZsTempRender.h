//---------------------------------------
// 临时功能存放处
//---------------------------------------
// 后处理线段-网格求交的薄适配层：公开签名与出参形态保持不变（消费者零改动），
// 数学本体在渲染库 mxr::postalg::SegmentMeshIntersection（金样本单测锁定行为，
// 边界判定看似怪异但为有意行为，勿在任一侧"顺手修正"）。本类只负责：
//   1. 从 modeldata/database/两个代理取数，组装中性快照 SegmentMeshSnapshot
//      （坐标在此处烘焙形变：原始坐标 + 形变比例 × 位移，位移缺失视为零位移）
//   2. 端点真实节点 ID 换算成连续节点 ID（快照下标）
//   3. QVector3D 与 glm::vec3 的出入参边界转换（集中在 ZsTempRender.cpp）
#pragma once
#include "mPostModelData.h"
#include "miofile_global.h"
#include "mPostDataBase.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include <array>


namespace PostData
{
    class mPostModelData;
    class mPostDataBase;
}

IOFILE_NAMESPACE_BEGIN

struct CompareVectors {
	bool operator()(const QVector3D& lhs, const QVector3D& rhs) const {
		if (lhs.x() != rhs.x()) {
			return lhs.x() < rhs.x();
		}
		else if (lhs.y() != rhs.y()) {
			return lhs.y() < rhs.y();
		}
		else {
			return lhs.z() < rhs.z();
		}
	}
};

// 交点处的三角形命中信息：三个面节点连续ID、真实ID、对应重心坐标权重
struct HitPointInfo {
	std::array<int, 3> triNodeCIds;      // 命中小三角形三个面节点的连续ID
	std::array<int, 3> triNodeRIds;      // 命中小三角形三个面节点的真实ID
	std::array<float, 3> bary;           // 重心坐标权重 (u, v, w)，满足 u+v+w=1
};

class MIOFILE_EXPORT ZsTempRender {
public:
	ZsTempRender() = default;
	~ZsTempRender();

	// 选点功能， 选中两个点，返回两个点构成线段穿过的所有面 体 单元，以及显示出等分的点, 返回等分点所在单元
	void chooseNode(
		int realID0,
		int realID1,
		int divide,
		int frame_id,
		PostData::mPostModelData* modeldata,
		PostData::mPostDataBase* database,
		mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
		mxr::IPostModelRenderDataProxy* postModelRenderDataProxy,
		std::vector<std::pair<int, int>>& out_ele_ids,	// <partID,meshID>
		std::vector<QVector3D>& out_point_poses,
		std::vector<std::set<int>>& out_point_in_ele
	);

	void chooseNode(
		int realID0,
		int realID1,
		int frame_id,
		PostData::mPostModelData* modeldata,
		PostData::mPostDataBase* database,
		mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
		mxr::IPostModelRenderDataProxy* postModelRenderDataProxy,
		std::vector<std::pair<int, int>>& out_ele_ids,	// <partID,meshID>, 击中单元
		std::vector<QVector3D>& out_point_poses			// 击中单元的交点坐标
	);

    void chooseNode(
        int realID0,
        int realID1,
        int frame_id,
        PostData::mPostModelData* modeldata,
        PostData::mPostDataBase* database,
        mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
        mxr::IPostModelRenderDataProxy* postModelRenderDataProxy,
        std::vector<std::pair<int, int>>& out_ele_ids,	// <partID,meshID>, 击中单元
        std::vector<QVector3D>& out_point_poses,			// 击中单元的交点坐标
        std::map<QVector3D, std::pair<int, int>, CompareVectors>& out_point_to_element//交点对应的单元
    );

    // 在点-单元映射的基础上，额外输出每个交点对应的命中面三角形的节点信息和重心坐标
    void chooseNode(
        int realID0,
        int realID1,
        int frame_id,
        PostData::mPostModelData* modeldata,
        PostData::mPostDataBase* database,
        mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
        mxr::IPostModelRenderDataProxy* postModelRenderDataProxy,
        std::vector<std::pair<int, int>>& out_ele_ids,	// <partID,meshID>, 击中单元
        std::vector<QVector3D>& out_point_poses,			// 击中单元的交点坐标
        std::map<QVector3D, std::pair<int, int>, CompareVectors>& out_point_to_element,
        std::map<QVector3D, HitPointInfo, CompareVectors>& out_point_hit_info
    );

	// 选中两个点，返回线段穿过的所有单元的节点ID集合
	std::set<int> getIntersectedNodeIDs(
		int realID0,
		int realID1,
		int frame_id,
		PostData::mPostModelData* modeldata,
		PostData::mPostDataBase* database,
		mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
		mxr::IPostModelRenderDataProxy* postModelRenderDataProxy
	);

	// 选中两个点，返回线段与所有单元面的交点坐标（已排序）
	std::vector<QVector3D> getIntersectedPoints(
		int realID0,
		int realID1,
		int frame_id,
		PostData::mPostModelData* modeldata,
		PostData::mPostDataBase* database,
		mxr::IPostRendQueryDataProxy* postRendQueryDataProxy,
		mxr::IPostModelRenderDataProxy* postModelRenderDataProxy
	);

	float getEpsilon() { return EPSILON; }
	void setEpsilon(float epsilon) { EPSILON = epsilon; }
private:
	float EPSILON = 1e-6f;
};


IOFILE_NAMESPACE_END
