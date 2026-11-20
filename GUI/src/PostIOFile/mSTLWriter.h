#pragma once
#pragma execution_character_set("utf-8")

// 云图-过滤 导出STL：STL 文件写出
// 移植自参考工具 PostSTLGenerate/src/stl_writer.{hpp,cpp}

#include "miofile_global.h"

#include <array>
#include <cstddef>
#include <QString>
#include <vector>

namespace MIOFile
{
	// STL 三角形网格的共享点表表示
	struct MIOFILE_EXPORT PostSTLSurface
	{
		struct Point { double x, y, z; };
		std::vector<Point> points;
		std::vector<std::array<std::size_t, 3>> triangles; // 索引指向 points
	};

	/** 写二进制 STL（默认导出格式） */
	MIOFILE_EXPORT bool writeBinarySTL(const QString& path, const PostSTLSurface& surface);

	/** 写 ASCII STL（备用） */
	MIOFILE_EXPORT bool writeAsciiSTL(const QString& path, const PostSTLSurface& surface);
}
