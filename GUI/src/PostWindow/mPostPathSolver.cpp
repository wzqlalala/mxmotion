#include "mPostPathSolver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace PostWindow
{
	void PathSolver::reserve(std::size_t edgeHint)
	{
		m_packedEdges.reserve(edgeHint);
	}

	// 添加无向边：退化边（两端相同）忽略；(min,max) 规范化打包，允许重复（solve 时去重）
	void PathSolver::addEdge(int cidA, int cidB)
	{
		if (cidA < 0 || cidB < 0 || cidA == cidB)
			return;

		int lo = cidA;
		int hi = cidB;
		if (lo > hi)
			std::swap(lo, hi);
		const std::int64_t packed = (static_cast<std::int64_t>(lo) << 32) | static_cast<std::uint32_t>(hi);
		m_packedEdges.push_back(packed);
	}

	void PathSolver::clear()
	{
		std::vector<std::int64_t>().swap(m_packedEdges);
	}

	PathSolveResult PathSolver::solve(int nodeCount, int startCId, int endCId,
		const CoordOf& coordOf,
		double tolerance,
		int maxPaths,
		int maxCount)
	{
		PathSolveResult result;

		// ---- 参数校验 ----
		if (nodeCount <= 0 || coordOf == nullptr ||
			startCId < 0 || startCId >= nodeCount ||
			endCId < 0 || endCId >= nodeCount ||
			startCId == endCId)
		{
			result.status = PathSolveResult::Status::InvalidInput;
			return result;
		}
		if (maxPaths <= 0)
			maxPaths = 1;
		if (maxCount <= 0)
			maxCount = 1;

		// ---- 边表排序去重 + CSR 邻接构建（03 §4：打包 int64 一次 sort+unique，避免哈希集常数） ----
		// 就地排序去重、不清空：同一求解器实例可重复 solve（NFR-2 确定性）
		std::sort(m_packedEdges.begin(), m_packedEdges.end());
		m_packedEdges.erase(std::unique(m_packedEdges.begin(), m_packedEdges.end()), m_packedEdges.end());
		const int edgeCount = static_cast<int>(m_packedEdges.size());

		std::vector<int> head(nodeCount + 1, 0);
		for (int i = 0; i < edgeCount; ++i)
		{
			const int a = static_cast<int>(m_packedEdges[i] >> 32);
			const int b = static_cast<int>(m_packedEdges[i] & 0xffffffffULL);
			// 防御：越界节点 id 的边不入图（正常建图流程不会出现）
			if (a < 0 || a >= nodeCount || b < 0 || b >= nodeCount)
				continue;
			++head[a + 1];
			++head[b + 1];
		}
		for (int i = 0; i < nodeCount; ++i)
			head[i + 1] += head[i];

		const int directedCount = head[nodeCount];
		std::vector<int> adjTo(directedCount, 0);
		std::vector<double> adjW(directedCount, 0.0);
		std::vector<int> cursor(head.begin(), head.end() - 1);
		for (int i = 0; i < edgeCount; ++i)
		{
			const int a = static_cast<int>(m_packedEdges[i] >> 32);
			const int b = static_cast<int>(m_packedEdges[i] & 0xffffffffULL);
			if (a < 0 || a >= nodeCount || b < 0 || b >= nodeCount)
				continue;

			double ax = 0.0, ay = 0.0, az = 0.0;
			double bx = 0.0, by = 0.0, bz = 0.0;
			coordOf(a, ax, ay, az);
			coordOf(b, bx, by, bz);
			const double dx = ax - bx;
			const double dy = ay - by;
			const double dz = az - bz;
			const double w = std::sqrt(dx * dx + dy * dy + dz * dz);

			adjTo[cursor[a]] = b;
			adjW[cursor[a]] = w;
			++cursor[a];
			adjTo[cursor[b]] = a;
			adjW[cursor[b]] = w;
			++cursor[b];
		}
		// 边表已排序去重完毕，收缩冗余容量（大模型下先还内存再求解；保留数据以支持重复求解，
		// GUI 侧求解器为函数局部对象，随作用域析构整体释放）
		m_packedEdges.shrink_to_fit();

		// 邻接表内按邻居 ID 升序（枚举顺序确定性的一部分，03 §5.4；网格节点度数小，插入排序足够）
		for (int u = 0; u < nodeCount; ++u)
		{
			for (int i = head[u] + 1; i < head[u + 1]; ++i)
			{
				const int tv = adjTo[i];
				const double tw = adjW[i];
				int j = i - 1;
				while (j >= head[u] && adjTo[j] > tv)
				{
					adjTo[j + 1] = adjTo[j];
					adjW[j + 1] = adjW[j];
					--j;
				}
				adjTo[j + 1] = tv;
				adjW[j + 1] = tw;
			}
		}

		// ---- 两次 Dijkstra（二叉堆 + 惰性删除）：dist_s 给等长判定基准，dist_t 给枚举剪枝界 ----
		const double INF = std::numeric_limits<double>::infinity();
		using HeapItem = std::pair<double, int>;
		std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> pq;
		auto dijkstra = [&](int src, std::vector<double>& dist)
		{
			dist.assign(nodeCount, INF);
			dist[src] = 0.0;
			pq.push(HeapItem(0.0, src));
			while (!pq.empty())
			{
				const HeapItem top = pq.top();
				pq.pop();
				const int u = top.second;
				if (top.first > dist[u])
					continue;   // 过期堆项
				for (int i = head[u]; i < head[u + 1]; ++i)
				{
					const int v = adjTo[i];
					const double nd = top.first + adjW[i];
					if (nd < dist[v])
					{
						dist[v] = nd;
						pq.push(HeapItem(nd, v));
					}
				}
			}
		};

		std::vector<double> distS;
		std::vector<double> distT(nodeCount, INF);
		dijkstra(startCId, distS);
		dijkstra(endCId, distT);

		const double D = distS[endCId];
		if (D == INF)
		{
			result.status = PathSolveResult::Status::Disconnected;
			return result;
		}

		// 等长判定容差（03 §5.3）：默认 eps = max(1e-9, 1e-6 * D)，覆盖单精度坐标浮点噪声
		const double eps = (tolerance > 0.0) ? tolerance : std::max(1e-9, 1e-6 * D);
		result.shortestLength = D;

		// ---- 紧边 DAG 上迭代式 DFS 枚举（显式栈，路径深度千级无递归栈溢出风险） ----
		// 剪枝①：curLen + w + dist_t(v) > D + eps 的边不可达等长，跳过；
		// 剪枝②：产出满 maxPaths 后切换纯计数模式；计数超 maxCount 封顶终止（R4）
		struct DfsFrame
		{
			int node;
			int edgeIdx;
			double inLen;   // 进入该节点所走边的权重（回溯时扣减）
		};

		std::vector<DfsFrame> stack;
		std::vector<int> pathNodes;
		std::vector<char> onPath(nodeCount, 0);
		stack.reserve(64);
		pathNodes.reserve(64);

		int totalCount = 0;
		double curLen = 0.0;
		bool countingOnly = false;

		pathNodes.push_back(startCId);
		onPath[startCId] = 1;
		stack.push_back(DfsFrame{ startCId, head[startCId], 0.0 });

		while (!stack.empty())
		{
			DfsFrame& fr = stack.back();

			if (fr.node == endCId)
			{
				// 完成一条等长最短路径（endCId 不再向外探索）
				++totalCount;
				if (!countingOnly)
				{
					PathEntry entry;
					entry.nodeCIds = pathNodes;
					entry.length = curLen;
					result.paths.push_back(std::move(entry));
					if (static_cast<int>(result.paths.size()) >= maxPaths)
						countingOnly = true;   // 已够列表上限，剩余路径仅计数
				}
				if (totalCount > maxCount)
				{
					result.countCapped = true;   // 计数封顶：实际条数超过 maxCount
					break;
				}
				onPath[fr.node] = 0;
				curLen -= fr.inLen;
				pathNodes.pop_back();
				stack.pop_back();
				continue;
			}

			if (fr.edgeIdx < head[fr.node + 1])
			{
				const int v = adjTo[fr.edgeIdx];
				const double w = adjW[fr.edgeIdx];
				++fr.edgeIdx;
				// 剪枝①（紧边判定）+ 当前路径防环（eps 容差下等距节点间可能成环，DAG 分层不绝对）
				if (onPath[v])
					continue;
				if (curLen + w + distT[v] > D + eps)
					continue;
				stack.push_back(DfsFrame{ v, head[v], w });
				pathNodes.push_back(v);
				onPath[v] = 1;
				curLen += w;
			}
			else
			{
				onPath[fr.node] = 0;
				curLen -= fr.inLen;
				pathNodes.pop_back();
				stack.pop_back();
			}
		}

		result.totalEqualCount = result.countCapped ? maxCount : totalCount;

		// ---- 确定性排序（FR-04/NFR-2）：长度升序、节点连续 ID 序列字典序升序；
		// 比较器为全序（节点序列互异），同输入重复计算结果逐条一致 ----
		std::sort(result.paths.begin(), result.paths.end(),
			[](const PathEntry& a, const PathEntry& b)
			{
				if (a.length != b.length)
					return a.length < b.length;
				return a.nodeCIds < b.nodeCIds;
			});

		result.truncatedToListCap = (totalCount > static_cast<int>(result.paths.size()));
		result.status = PathSolveResult::Status::Ok;
		return result;
	}
}
