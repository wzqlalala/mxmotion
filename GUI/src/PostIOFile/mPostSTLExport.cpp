#include "mPostSTLExport.h"
#include "mSTLWriter.h"

// 云图-过滤 导出STL：核心计算
// 移植自参考工具 PostSTLGenerate/src/main.cpp：
//   按单元类型分块调用 UCP 的 stitched 拼接算法，各块表面按点偏移追加合并后写出。
// 扩展：
//   1. 大于/小于规则改用单值拼接模型 stitched_isosurface_boundary_model；
//   2. 合并后跨块共点焊接 + 跨单元类型内部重面去重（保证表面拓扑连通、水密）；
//   3. 剔除界面不可见的“悬空内部分量”（纯 cap 封闭薄壳），保证导出与过滤显示一致。

#include "ucp/ucp.hpp"

#include <algorithm>
#include <numeric>
#include <unordered_map>

namespace MIOFile
{
	namespace
	{
		// 合并后的表面：点表共享，三角形引用点表下标（STL 落盘前的中间结果）
		struct MergedSurface
		{
			std::vector<ucp::Point3> points;
			std::vector<std::array<std::size_t, 3>> triangles;
			// 1 表示该三角形来自原始网格外边界（界面过滤可见的必然是这类区域）
			std::vector<unsigned char> triangleIsBoundary;
		};

		void appendBlockRange(MergedSurface& merged, const ucp::StitchedRangeBoundaryModel& block)
		{
			const std::size_t pointOffset = merged.points.size();
			merged.points.insert(merged.points.end(), block.points.begin(), block.points.end());

			merged.triangles.reserve(merged.triangles.size() + block.triangles.size());
			merged.triangleIsBoundary.reserve(merged.triangleIsBoundary.size() + block.triangles.size());
			for (std::size_t i = 0; i < block.triangles.size(); ++i) {
				const auto& tri = block.triangles[i];
				merged.triangles.push_back({ {
					pointOffset + tri[0],
					pointOffset + tri[1],
					pointOffset + tri[2]} });
				merged.triangleIsBoundary.push_back(
					block.triangle_kinds[i] == ucp::StitchedRangeSurfaceTriangleKind::boundary_surface ? 1 : 0);
			}
		}

		void appendBlockIso(MergedSurface& merged, const ucp::StitchedIsosurfaceBoundaryModel& block)
		{
			const std::size_t pointOffset = merged.points.size();
			merged.points.insert(merged.points.end(), block.points.begin(), block.points.end());

			merged.triangles.reserve(merged.triangles.size() + block.triangles.size());
			merged.triangleIsBoundary.reserve(merged.triangleIsBoundary.size() + block.triangles.size());
			for (std::size_t i = 0; i < block.triangles.size(); ++i) {
				const auto& tri = block.triangles[i];
				merged.triangles.push_back({ {
					pointOffset + tri[0],
					pointOffset + tri[1],
					pointOffset + tri[2]} });
				merged.triangleIsBoundary.push_back(
					block.triangle_kinds[i] == ucp::StitchedSurfaceTriangleKind::boundary_surface ? 1 : 0);
			}
		}

		// 之间/等于：生成 [vmin, vmax] 区间外壳（lower cap + upper cap + 区间内原始边界）
		template <class Cell>
		bool processRangeBlock(const PostSTLExportInput& in, const PostSTLCellBlock& block, MergedSurface& merged, QString& err)
		{
			const ucp::PointView points = ucp::make_points(in.x.data(), in.y.data(), in.z.data(), in.x.size());
			const ucp::MeshView<Cell> mesh = ucp::make_mesh<Cell>(points, block.connectivity.data(), block.cellCount());
			const ucp::ScalarFieldView field = ucp::make_scalar_field(in.field.data(), in.field.size());

			const auto result = ucp::stitched_range_boundary_model_checked(mesh, field, ucp::ThresholdRange{ in.vmin, in.vmax });
			if (!result.status.ok()) {
				err = QString::fromStdString(result.status.message);
				return false;
			}
			appendBlockRange(merged, result.value);
			return true;
		}

		// 大于/小于：单值等值面封盖 + 裁剪后原始边界
		template <class Cell>
		bool processIsoBlock(const PostSTLExportInput& in, const PostSTLCellBlock& block, double iso, bool keepGreaterOrEqual, MergedSurface& merged, QString& err)
		{
			const ucp::PointView points = ucp::make_points(in.x.data(), in.y.data(), in.z.data(), in.x.size());
			const ucp::MeshView<Cell> mesh = ucp::make_mesh<Cell>(points, block.connectivity.data(), block.cellCount());
			const ucp::ScalarFieldView field = ucp::make_scalar_field(in.field.data(), in.field.size());

			ucp::StitchedIsosurfaceBoundaryOptions options;
			options.keep_greater_or_equal = keepGreaterOrEqual;

			const auto result = ucp::stitched_isosurface_boundary_model_checked(mesh, field, iso, options);
			if (!result.status.ok()) {
				err = QString::fromStdString(result.status.message);
				return false;
			}
			appendBlockIso(merged, result.value);
			return true;
		}

		bool dispatchBlock(const PostSTLExportInput& in, const PostSTLCellBlock& block, MergedSurface& merged, QString& err)
		{
			const bool rangeMode = (in.rule == STLFilterRule::Between || in.rule == STLFilterRule::Equal);
			// 大于取下界、小于取上界作为等值面值
			const double iso = (in.rule == STLFilterRule::Above) ? in.vmin : in.vmax;
			switch (block.kind) {
			case PostSTLCellBlock::Tetra:
				return rangeMode
					? processRangeBlock<ucp::tetra>(in, block, merged, err)
					: processIsoBlock<ucp::tetra>(in, block, iso, in.rule == STLFilterRule::Above, merged, err);
			case PostSTLCellBlock::Hexa:
				return rangeMode
					? processRangeBlock<ucp::hexahedron>(in, block, merged, err)
					: processIsoBlock<ucp::hexahedron>(in, block, iso, in.rule == STLFilterRule::Above, merged, err);
			case PostSTLCellBlock::Wedge:
				return rangeMode
					? processRangeBlock<ucp::wedge>(in, block, merged, err)
					: processIsoBlock<ucp::wedge>(in, block, iso, in.rule == STLFilterRule::Above, merged, err);
			case PostSTLCellBlock::Pyramid:
				return rangeMode
					? processRangeBlock<ucp::pyramid>(in, block, merged, err)
					: processIsoBlock<ucp::pyramid>(in, block, iso, in.rule == STLFilterRule::Above, merged, err);
			}
			err = QStringLiteral("unknown cell block kind");
			return false;
		}

		std::size_t ufFindRoot(std::vector<std::size_t>& parent, std::size_t x)
		{
			while (parent[x] != x) {
				parent[x] = parent[parent[x]];
				x = parent[x];
			}
			return x;
		}

		// 跨块共点焊接：四种单元类型块各自生成点表，同一条边上的交点在不同块里
		// 重复计算（数值上来自同一对节点值的同一插值，坐标几乎逐位一致）。
		// 按 1e-6 量化合并公共顶点，把跨块的表面在拓扑上接起来；
		// mm 级网格尺度下不会误并真实不同的点（STL 本身按 float32 落盘）。
		struct WeldKey
		{
			long long x = 0, y = 0, z = 0;
			bool operator==(const WeldKey& o) const { return x == o.x && y == o.y && z == o.z; }
		};
		struct WeldKeyHash
		{
			std::size_t operator()(const WeldKey& k) const
			{
				std::size_t h = std::hash<long long>()(k.x);
				h ^= std::hash<long long>()(k.y) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
				h ^= std::hash<long long>()(k.z) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
				return h;
			}
		};
		void weldCrossBlockPoints(MergedSurface& surface)
		{
			if (surface.points.empty())
				return;
			auto quantize = [](double v) { return static_cast<long long>(std::llround(v * 1e6)); };
			std::unordered_map<WeldKey, std::size_t, WeldKeyHash> table;
			table.reserve(surface.points.size() * 2);
			std::vector<std::size_t> remap(surface.points.size());
			std::vector<ucp::Point3> kept;
			kept.reserve(surface.points.size());
			for (std::size_t i = 0; i < surface.points.size(); ++i) {
				const WeldKey key{ quantize(surface.points[i].x), quantize(surface.points[i].y), quantize(surface.points[i].z) };
				const auto found = table.find(key);
				if (found == table.end()) {
					remap[i] = kept.size();
					table.emplace(key, kept.size());
					kept.push_back(surface.points[i]);
				}
				else {
					remap[i] = found->second;
				}
			}
			// 焊接后顶点重合的三角形面积为零（跨块共点误差所致），直接丢弃
			std::vector<std::array<std::size_t, 3>> triangles;
			std::vector<unsigned char> isBoundary;
			triangles.reserve(surface.triangles.size());
			isBoundary.reserve(surface.triangles.size());
			for (std::size_t i = 0; i < surface.triangles.size(); ++i) {
				const std::size_t a = remap[surface.triangles[i][0]];
				const std::size_t b = remap[surface.triangles[i][1]];
				const std::size_t c = remap[surface.triangles[i][2]];
				if (a == b || b == c || a == c)
					continue;
				triangles.push_back({ { a, b, c } });
				isBoundary.push_back(surface.triangleIsBoundary[i]);
			}
			surface.points = std::move(kept);
			surface.triangles = std::move(triangles);
			surface.triangleIsBoundary = std::move(isBoundary);
		}

		// 跨块内部界面去重：焊接后，相邻的不同类型单元之间的内部面会以相同的
		// 三个顶点出现两次（两侧绕向相反）。这类面位于导出体内部，剔除后表面
		// 才是水密的。同顶点同绕向的重复（异常数据）保留一份。
		bool sameWinding(const std::array<std::size_t, 3>& a, const std::array<std::size_t, 3>& b)
		{
			for (int shift = 0; shift < 3; ++shift) {
				if (b[0] == a[shift] && b[1] == a[(shift + 1) % 3] && b[2] == a[(shift + 2) % 3])
					return true;
			}
			return false;
		}
		void dropDuplicateTriangles(MergedSurface& surface)
		{
			struct TriKey
			{
				std::size_t a = 0, b = 0, c = 0;
				bool operator==(const TriKey& o) const { return a == o.a && b == o.b && c == o.c; }
			};
			struct TriKeyHash
			{
				std::size_t operator()(const TriKey& k) const
				{
					std::size_t h = std::hash<std::size_t>()(k.a);
					h ^= std::hash<std::size_t>()(k.b) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
					h ^= std::hash<std::size_t>()(k.c) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
					return h;
				}
			};
			std::unordered_map<TriKey, std::vector<std::size_t>, TriKeyHash> groups;
			groups.reserve(surface.triangles.size() * 2);
			for (std::size_t i = 0; i < surface.triangles.size(); ++i) {
				TriKey key{ surface.triangles[i][0], surface.triangles[i][1], surface.triangles[i][2] };
				if (key.b < key.a) std::swap(key.a, key.b);
				if (key.c < key.a) std::swap(key.a, key.c);
				if (key.c < key.b) std::swap(key.b, key.c);
				groups[key].push_back(i);
			}
			std::vector<char> drop(surface.triangles.size(), 0);
			for (const auto& kv : groups) {
				const std::vector<std::size_t>& idxs = kv.second;
				if (idxs.size() < 2)
					continue;
				bool hasOpposite = false;
				for (std::size_t i = 0; i < idxs.size() && !hasOpposite; ++i)
					for (std::size_t j = i + 1; j < idxs.size(); ++j)
						if (!sameWinding(surface.triangles[idxs[i]], surface.triangles[idxs[j]])) {
							hasOpposite = true;
							break;
						}
				if (hasOpposite) {
					for (std::size_t i : idxs)
						drop[i] = 1; // 内部界面：两侧都剔除
				}
				else {
					for (std::size_t k = 1; k < idxs.size(); ++k)
						drop[idxs[k]] = 1; // 同向重复：保留一份
				}
			}
			std::vector<std::array<std::size_t, 3>> triangles;
			std::vector<unsigned char> isBoundary;
			triangles.reserve(surface.triangles.size());
			isBoundary.reserve(surface.triangles.size());
			for (std::size_t i = 0; i < surface.triangles.size(); ++i) {
				if (drop[i])
					continue;
				triangles.push_back(surface.triangles[i]);
				isBoundary.push_back(surface.triangleIsBoundary[i]);
			}
			surface.triangles = std::move(triangles);
			surface.triangleIsBoundary = std::move(isBoundary);
		}

		// 剔除“悬空内部分量”：与界面过滤显示保持一致。
		// 某个单元的节点值同时跨过区间下界与上界时，其内部会生成一层与主体不相连的
		// 封闭等值面薄壳——它属于区间实体但界面不可见，导出会表现为多余碎块。
		// 判据：分量完全由 cap（等值面封盖）组成、不含任何原始边界面片——即完全
		// 封闭在材料内部、界面上不可见的薄壳；含原始边界面片的分量对应界面可见
		// 区域，一律保留。
		// 注意：不能用“边界面片面积占比”做阈值。跨块共点焊接之前，同一张连续表面
		// 会被按单元类型拆成多个分量，较小类型的表面区域（如 hexa 网格区相对 tetra
		// 主体）面积占比常低于阈值而被整块误删（实测数据曾误删两个约 1.8 万三角形的
		// 区域）。焊接之后按“是否接触原始边界”判定与界面可见性严格一致。
		// 返回被剔除的三角形数。
		std::size_t dropInteriorComponents(MergedSurface& surface)
		{
			const std::size_t triCount = surface.triangles.size();
			if (triCount == 0)
				return 0;

			std::vector<std::size_t> parent(surface.points.size());
			std::iota(parent.begin(), parent.end(), std::size_t(0));
			for (const auto& tri : surface.triangles) {
				const std::size_t r0 = ufFindRoot(parent, tri[0]);
				const std::size_t r1 = ufFindRoot(parent, tri[1]);
				const std::size_t r2 = ufFindRoot(parent, tri[2]);
				if (r0 != r1) parent[r1] = r0;
				const std::size_t r02 = ufFindRoot(parent, tri[0]);
				if (r02 != r2) parent[r2] = r02;
			}

			// 每个连通分量是否含有原始边界面片（界面可见区域必然含有）
			std::unordered_map<std::size_t, bool> hasBoundary;
			for (std::size_t i = 0; i < triCount; ++i) {
				if (!surface.triangleIsBoundary[i])
					continue;
				hasBoundary[ufFindRoot(parent, surface.triangles[i][0])] = true;
			}

			std::size_t dropped = 0;
			std::vector<std::array<std::size_t, 3>> keptTriangles;
			std::vector<unsigned char> keptIsBoundary;
			keptTriangles.reserve(triCount);
			keptIsBoundary.reserve(triCount);
			for (std::size_t i = 0; i < triCount; ++i) {
				const std::size_t root = ufFindRoot(parent, surface.triangles[i][0]);
				const auto it = hasBoundary.find(root);
				const bool keep = (it != hasBoundary.end() && it->second);
				if (keep) {
					keptTriangles.push_back(surface.triangles[i]);
					keptIsBoundary.push_back(surface.triangleIsBoundary[i]);
				}
				else {
					++dropped;
				}
			}
			if (dropped == 0 || keptTriangles.empty())
				return 0;

			// 压缩点表
			const std::size_t invalid = static_cast<std::size_t>(-1);
			std::vector<std::size_t> remap(surface.points.size(), invalid);
			std::vector<ucp::Point3> newPoints;
			for (auto& tri : keptTriangles) {
				for (std::size_t& idx : tri) {
					if (remap[idx] == invalid) {
						remap[idx] = newPoints.size();
						newPoints.push_back(surface.points[idx]);
					}
					idx = remap[idx];
				}
			}
			surface.points = std::move(newPoints);
			surface.triangles = std::move(keptTriangles);
			surface.triangleIsBoundary = std::move(keptIsBoundary);
			return dropped;
		}
	}

	mPostSTLExportThread::mPostSTLExportThread(PostSTLExportInput input, QObject* parent)
		: QThread(parent)
		, _input(std::move(input))
	{
	}

	void mPostSTLExportThread::run()
	{
		MergedSurface merged;
		QString err;

		for (const auto& block : _input.blocks) {
			if (!dispatchBlock(_input, block, merged, err)) {
				_result.ok = false;
				_result.errorDetail = err;
				emit exportSTLFinishedSig(false, _result.errorDetail);
				return;
			}
		}

		//各块点表独立生成，先把跨块的公共顶点焊接起来、剔除跨单元类型的内部
		//重面，再做分量剔除——否则同一张连续表面会按单元类型被拆成多个分量
		weldCrossBlockPoints(merged);
		dropDuplicateTriangles(merged);

		//剔除界面不可见的悬空内部分量，保持与过滤显示一致
		_result.droppedTriangleCount = dropInteriorComponents(merged);

		_result.pointCount = merged.points.size();
		_result.triangleCount = merged.triangles.size();
		if (merged.triangles.empty()) {
			_result.ok = false;
			_result.errorDetail = QStringLiteral("no triangles generated");
			emit exportSTLFinishedSig(false, _result.errorDetail);
			return;
		}

		PostSTLSurface surface;
		surface.points.resize(merged.points.size());
		for (std::size_t i = 0; i < merged.points.size(); ++i) {
			surface.points[i] = { merged.points[i].x, merged.points[i].y, merged.points[i].z };
		}
		surface.triangles = std::move(merged.triangles);

		if (!writeBinarySTL(_input.outputPath, surface)) {
			_result.ok = false;
			_result.errorDetail = QStringLiteral("failed to write stl file");
			emit exportSTLFinishedSig(false, _result.errorDetail);
			return;
		}

		_result.ok = true;
		emit exportSTLFinishedSig(true, QString());
	}
}
