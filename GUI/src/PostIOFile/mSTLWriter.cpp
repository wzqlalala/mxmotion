#include "mSTLWriter.h"

// 云图-过滤 导出STL：STL 文件写出
// 移植自参考工具 PostSTLGenerate/src/stl_writer.cpp

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>

namespace MIOFile
{
	namespace
	{
		PostSTLSurface::Point triangleNormal(
			const PostSTLSurface::Point& a,
			const PostSTLSurface::Point& b,
			const PostSTLSurface::Point& c)
		{
			PostSTLSurface::Point ab{ b.x - a.x, b.y - a.y, b.z - a.z };
			PostSTLSurface::Point ac{ c.x - a.x, c.y - a.y, c.z - a.z };
			PostSTLSurface::Point n{
				ab.y * ac.z - ab.z * ac.y,
				ab.z * ac.x - ab.x * ac.z,
				ab.x * ac.y - ab.y * ac.x };
			const double len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
			if (len > 0.0) {
				n.x /= len;
				n.y /= len;
				n.z /= len;
			}
			return n;
		}
	}

	bool writeAsciiSTL(const QString& path, const PostSTLSurface& surface)
	{
		std::ofstream out(path.toLocal8Bit().toStdString(), std::ios::out);
		if (!out) {
			return false;
		}

		out << std::fixed << std::setprecision(6);
		out << "solid filter_surface\n";
		for (const auto& tri : surface.triangles) {
			const auto& a = surface.points[tri[0]];
			const auto& b = surface.points[tri[1]];
			const auto& c = surface.points[tri[2]];
			const auto n = triangleNormal(a, b, c);
			out << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n";
			out << "    outer loop\n";
			out << "      vertex " << a.x << " " << a.y << " " << a.z << "\n";
			out << "      vertex " << b.x << " " << b.y << " " << b.z << "\n";
			out << "      vertex " << c.x << " " << c.y << " " << c.z << "\n";
			out << "    endloop\n";
			out << "  endfacet\n";
		}
		out << "endsolid filter_surface\n";
		return static_cast<bool>(out);
	}

	bool writeBinarySTL(const QString& path, const PostSTLSurface& surface)
	{
		std::ofstream out(path.toLocal8Bit().toStdString(), std::ios::out | std::ios::binary);
		if (!out) {
			return false;
		}

		// 80 字节文件头
		char header[80] = {};
		std::memcpy(header, "MxSim post filter binary STL", 28);
		out.write(header, 80);

		const std::uint32_t triangleCount = static_cast<std::uint32_t>(surface.triangles.size());
		out.write(reinterpret_cast<const char*>(&triangleCount), sizeof(triangleCount));

		for (const auto& tri : surface.triangles) {
			const auto& a = surface.points[tri[0]];
			const auto& b = surface.points[tri[1]];
			const auto& c = surface.points[tri[2]];
			const auto n = triangleNormal(a, b, c);

			const float v[12] = {
				static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z),
				static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z),
				static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z),
				static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z) };
			const std::uint16_t attribute = 0;

			out.write(reinterpret_cast<const char*>(v), sizeof(v));
			out.write(reinterpret_cast<const char*>(&attribute), sizeof(attribute));
		}

		return static_cast<bool>(out);
	}
}
