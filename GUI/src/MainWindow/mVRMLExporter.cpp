#include <vector>
#include <array>
#include <fstream> //【OCC摘除·S10/P0-1】显式自含（原经 mModelData.h 头网的 OCC 链传递）
#include <iomanip> //同上：setprecision
#include "mmainwindow_global.h"
#include "mModelData.h"
#include <mxr/view/mBaseRend.h>
#include "mVRMLExporter.h"

using MechModelData::mModelData;

namespace MechMainWindow
{
    mVRMLExporter::mVRMLExporter(mxr::mBaseRend* pWnd) :_pWnd(pWnd) {}
    void mVRMLExporter::exportAllModel(const std::string& fileName)
    {
        if (_pWnd == nullptr)
        {
            return;
        }
        // 导出所有渲染内容
        std::vector<glm::vec3> faceVertices;
        std::vector<glm::vec3> colorVertices;
        std::vector<int> faceIndices;
        std::vector<int> faceLineIndices;
        _pWnd->exportAllModel(faceVertices, colorVertices, faceIndices, faceLineIndices);

        // 输出设置
        mVrmlExportOptions opt;

        // 输出数据
        mMeshData data;
        data.name = "HelloWorld";
        data.positions = faceVertices;
        for (int i = 0; i < faceIndices.size() / 3; i += 3)
        {
            data.faces.push_back({
                faceIndices[i * 3 + 0],
                faceIndices[i * 3 + 1],
                faceIndices[i * 3 + 2],
                });
        }

        writeVRML(fileName, faceVertices, colorVertices, faceIndices, faceLineIndices);
    }

    void mVRMLExporter::writeVRML(const std::string& fileName,
        const std::vector<glm::vec3>& faceVertices,
        const std::vector<glm::vec3>& faceColors,
        const std::vector<int>& faceIndices,
        const std::vector<int>& faceLineIndices)
    {
        std::ofstream out(fileName);
        out << "#VRML V2.0 utf8\n\n"
            << "Group {\n"                    // 把两个 Shape 包在一起
            << "  children [\n";

        /* ========== 1. 彩色三角面 ========== */
        out << "    Shape {\n"
            << "      appearance Appearance {\n"
            << "        material Material { }\n"
            << "      }\n"
            << "      geometry IndexedFaceSet {\n"
            << "        coord Coordinate {\n"
            << "          point [\n";
        // ---- 顶点表 ----
        out << std::fixed << std::setprecision(6);
        for (size_t i = 0; i < faceVertices.size(); ++i)
        {
            const auto& v = faceVertices[i];
            out << "            " << v.x << ' ' << v.y << ' ' << v.z;
            if (i + 1 < faceVertices.size()) out << ',';
            out << '\n';
        }
        out << "          ]\n"
            << "        }\n"
            << "        coordIndex [\n";
        // ---- 三角面索引（每3+ -1）----
        for (size_t base = 0; base + 2 < faceIndices.size(); base += 3)
        {
            int a = faceIndices[base],
                b = faceIndices[base + 1],
                c = faceIndices[base + 2];
            out << "            " << a << ' ' << b << ' ' << c << " -1";
            if (base + 3 < faceIndices.size()) out << ',';
            out << '\n';
        }
        out << "        ]\n"
            << "        colorPerVertex TRUE\n"
            << "        color Color {\n"
            << "          color [\n";
        // ---- 颜色表 ----
        for (size_t i = 0; i < faceColors.size(); ++i)
        {
            const auto& c = faceColors[i];
            out << "            " << c.x << ' ' << c.y << ' ' << c.z;
            if (i + 1 < faceColors.size()) out << ',';
            out << '\n';
        }
        out << "          ]\n"
            << "        }\n"
            << "      }\n"
            << "    },\n";      // 三角 Shape 结束，注意逗号

        /* ========== 2. 线框（IndexedLineSet） ========== */
        out << "    Shape {\n"
            << "      appearance Appearance {\n"
            << "        material Material { emissiveColor 0 0 0 }" // 黑线，自发光
            << "      }\n"
            << "      geometry IndexedLineSet {\n"
            << "        coord Coordinate {\n"
            << "          point [\n";
        // ---- 同一份顶点表复用 ----
        for (size_t i = 0; i < faceVertices.size(); ++i)
        {
            const auto& v = faceVertices[i];
            out << "            " << v.x << ' ' << v.y << ' ' << v.z;
            if (i + 1 < faceVertices.size()) out << ',';
            out << '\n';
        }
        out << "          ]\n"
            << "        }\n"
            << "        coordIndex [\n";
        // ---- 边索引：每2个索引+ -1 ----
        for (size_t base = 0; base + 1 < faceLineIndices.size(); base += 2)
        {
            int i1 = faceLineIndices[base],
                i2 = faceLineIndices[base + 1];
            out << "            " << i1 << ' ' << i2 << " -1";
            if (base + 2 < faceLineIndices.size()) out << ',';
            out << '\n';
        }
        out << "        ]\n"
            << "      }\n"
            << "    }\n";      // 线 Shape 结束

        out << "  ]\n"         // Group.children 结束
            << "}\n";
    }
}