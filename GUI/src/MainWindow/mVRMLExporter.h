#pragma once


#include "mmainwindow_global.h"
namespace MechModelData
{
    class mModelData;
}

namespace MechMainWindow
{
    struct mMeshData
    {
        std::string name;
        std::vector<glm::vec3> positions;
        std::vector<std::vector<int>> faces;
    };

    struct mVrmlExportOptions {
        // 统一白模颜色（0~1）
        double diffuse = 0.85;

        // 单位缩放：比如你的单位是 mm，想导出 m：scale=0.001
        double scale = 1.0;

        // 坐标变换（可选）
        // - swap_yz: 把 (x,y,z) -> (x,z,y)  常用于 Y-up/Z-up 切换
        bool swap_yz = false;

        // - flip_z: 把 z 取反，常用于左手/右手切换
        bool flip_z = false;

        // VRML IndexedFaceSet 参数
        bool solid = true;       // TRUE：背面不渲染（封闭实体更合理）
        bool ccw = true;         // 顶点是否逆时针定义为正面
        double creaseAngle = 0.5; // 平滑角（不写法线时，有些查看器会参考它）
    };

	class MMAINWINDOW_EXPORT mVRMLExporter
	{
	public:
        mVRMLExporter(mxr::mBaseRend* pWnd);

        // 导出后处理中的所有模型
        void exportAllModel(const std::string& fileName);
    private:
        void writeVRML(const std::string& fileName,
            const std::vector<glm::vec3>& faceVertices,
            const std::vector<glm::vec3>& faceColors,
            const std::vector<int>& faceIndices,
            const std::vector<int>& faceLineIndices);
	private:
        mxr::mBaseRend* _pWnd;
        // 导出状态
        mVrmlExportOptions _exportOptions;
	};


}