#pragma once
#pragma execution_character_set("utf-8")

// 云图-过滤 导出STL：把当前过滤区间内的模型区域生成为封闭表面 STL
// 算法参考 PostSTLGenerate（UnstructureCellPost 的 stitched 拼接算法）

#include "miofile_global.h"

#include <QThread>
#include <QString>
#include <vector>
#include <cstdint>

namespace MIOFile
{
	// 过滤规则（与云图-过滤下拉框对应）
	enum class STLFilterRule
	{
		Between,    // 之间：[vmin, vmax] 区间外壳
		Above,      // 大于：保留 value >= vmin 一侧
		Below,      // 小于：保留 value <= vmax 一侧
		Equal       // 等于：调用方展开为 [v-δ, v+δ] 窄区间后按 Between 处理
	};

	// 按单元类型分块的连接关系（节点下标指向 PostSTLExportInput 的节点表）
	// connectivity 用 uint64：与 ucp::index_t 一致
	struct MIOFILE_EXPORT PostSTLCellBlock
	{
		enum Kind { Tetra, Hexa, Wedge, Pyramid };
		Kind kind = Tetra;
		std::vector<std::uint64_t> connectivity;  // 每个单元 nodeCount 个节点下标

		static std::size_t nodeCountOf(Kind k)
		{
			switch (k) {
			case Tetra:    return 4;
			case Hexa:     return 8;
			case Wedge:    return 6;
			case Pyramid:  return 5;
			}
			return 0;
		}
		std::size_t nodeCount() const { return nodeCountOf(kind); }
		std::size_t cellCount() const
		{
			const std::size_t n = nodeCount();
			return n == 0 ? 0 : connectivity.size() / n;
		}
	};

	// 导出输入快照：GUI 线程上一次性收集的纯数据，后台线程只读，
	// 不携带任何 QObject/指针，规避跨线程访问界面数据
	struct MIOFILE_EXPORT PostSTLExportInput
	{
		std::vector<double> x, y, z;          // 节点坐标（已按界面显示构型叠加变形）
		std::vector<PostSTLCellBlock> blocks; // 三维实体单元分块（全量，不随界面显隐）
		std::vector<double> field;            // 节点标量场（单元变量已平均到节点）
		STLFilterRule rule = STLFilterRule::Between;
		double vmin = 0.0;
		double vmax = 0.0;
		QString outputPath;
	};

	// 导出结果统计（errorDetail 仅放技术细节，面向用户的提示由调用方按语言组织）
	struct MIOFILE_EXPORT PostSTLExportResult
	{
		bool ok = false;
		QString errorDetail;
		std::size_t pointCount = 0;
		std::size_t triangleCount = 0;
		std::size_t droppedTriangleCount = 0; // 剔除的界面不可见内部悬空分量三角形数
	};

	// STL 导出后台线程：run() 内只使用快照数据，不触碰任何 QObject/单例
	class MIOFILE_EXPORT mPostSTLExportThread : public QThread
	{
		Q_OBJECT
	public:
		explicit mPostSTLExportThread(PostSTLExportInput input, QObject* parent = nullptr);

		/** 线程结束后在 GUI 线程读取（finished 之后） */
		const PostSTLExportResult& result() const { return _result; }

	protected:
		void run() override;

	signals:
		void exportSTLFinishedSig(bool ok, QString errorDetail);

	private:
		PostSTLExportInput _input;
		PostSTLExportResult _result;
	};
}
