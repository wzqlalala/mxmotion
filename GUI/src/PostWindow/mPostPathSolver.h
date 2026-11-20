#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

// 后处理路径结果：拓扑最短路径求解器（纯逻辑类，零 GUI/PostData 依赖）
// 选型（03-技术路线 §2.1/§2.2-A）：两次 Dijkstra（二叉堆）得 dist_s/dist_t，
// 紧边（dist_s(u)+w(u,v)+dist_t(v) ≤ D+ε）子图必为 DAG，其上 DFS 枚举全部等长
// 最短路径；枚举序沿 CSR 邻接构建序，最终按 (长度, 节点序列字典序) 确定性排序。
// 只依赖 std，可脱离 PostData 独立构造算例验证。

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace PostWindow
{
	// 一条等长最短路径条目：节点为连续 ID（调用方负责与真实 ID 互转）
	struct PathEntry
	{
		std::vector<int> nodeCIds;   // 相邻节点对均共享至少一条入图边（FR-01）
		double length = 0.0;         // 参考坐标累计欧氏长度（Q2=A，double 累加）
	};

	struct PathSolveResult
	{
		enum class Status
		{
			Ok,             // 求解成功（paths 可能被截断，见 truncatedToListCap/countCapped）
			Disconnected,   // 起终点不连通（含孤立节点，FR-06/边界 11）
			InvalidInput    // 参数非法（起终点越界/相同、坐标访问器为空等）
		};
		Status status = Status::Disconnected;
		double shortestLength = 0.0;        // 最短路径总长 D（Ok 时有效）
		std::vector<PathEntry> paths;       // 等长最短路径，已按 (长度, 节点序列字典序) 确定性排序
		int totalEqualCount = 0;            // 等长路径总数（受计数上限保护，超限见 countCapped）
		bool countCapped = false;           // true: totalEqualCount 为计数上限值，实际更多（R4 裁决：1 万封顶）
		bool truncatedToListCap = false;    // true: paths 超过列表上限被截断（FR-04/边界 3）
	};

	// 最短路径求解器：点击时建图、用后即释（03-技术路线 §2.3-A），不持任何模型数据指针
	class PathSolver
	{
	public:
		// 坐标访问器：连续 ID -> 参考坐标 (x,y,z)，用于边权与等长判定（Q2=A）
		using CoordOf = std::function<void(int cid, double& x, double& y, double& z)>;

		// 预留边容量（大模型建图减少 vector 扩容拷贝）
		void reserve(std::size_t edgeHint);

		// 添加无向边；两端相同的退化边内部忽略；允许重复边（solve 时排序去重）
		void addEdge(int cidA, int cidB);

		// 求解 startCId -> endCId 的全部等长最短路径
		// nodeCount：图节点数（连续 ID 取值 [0, nodeCount)）
		// tolerance：等长判定容差；<=0 时取默认相对容差 eps = max(1e-9, 1e-6 * D)（03 §5.3）
		// maxPaths：列表保留上限（FR-04/边界 3：100）；maxCount：等长路径计数上限（R4 裁决：1 万）
		PathSolveResult solve(int nodeCount, int startCId, int endCId,
			const CoordOf& coordOf,
			double tolerance = 0.0,
			int maxPaths = 100,
			int maxCount = 10000);

		// 释放图内存。注意 solve 非破坏性：调用后 m_packedEdges 保留（已排序去重），
		// 同一实例可重复 solve（NFR-2 确定性）；GUI 侧求解器为函数局部对象随作用域释放
		void clear();

	private:
		// 打包临时边表：(min,max) 规范化打包为 int64，solve 入口一次 sort+unique 建 CSR
		std::vector<std::int64_t> m_packedEdges;
	};
}
