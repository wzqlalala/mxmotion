#pragma once
#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

namespace MechMainWindow
{
	// STL重剖分面板
	class MMAINWINDOW_EXPORT mSTLRemeshPanel :public mMeshPanel
	{
	public:

        mSTLRemeshPanel(QWidget* parent = Q_NULLPTR);

        ~mSTLRemeshPanel();

        // 新建面板
        void newMeshPanel() override;
        // 编辑面板
        void editMeshPanel(int meshId);
        // 更新面板
        void updatePanel() override;
        // 重剖分
		void remesh();
        // 显示锐利边（按二面角阈值检测特征边并在视口中高亮）
		void showFeatureEdges();

		// 计算拾取三角网格的顶点平均曲率（cotangent 离散化 + Meyer 混合 Voronoi 面积）
		// 返回 true 表示成功；curvatures/normals/areas 按内部顶点索引输出，indexToNodeId 用于回映射真实节点ID
		bool computeMeanCurvature(std::vector<double>& curvatures,
			std::vector<QVector3D>& normals,
			std::vector<double>& areas,
			std::vector<int>& indexToNodeId);

	private:

		// 从视口选择集同步拾取单元到 MeshIDs 并刷新面板状态；append 为 true 时在已有拾取上追加去重
		void syncPickFromViewport(bool append);
		// 清空拾取并复位面板状态
		void clearPickState();

		vector<RenderPickResultData> MeshIDs;   // 拾取的网格单元
		PosPickWidget1 posPick;                 // 拾取器
        QComboBox* meshtype;                    // 单元类型
        mCalcLineEdit* meshsize;                // 网格尺寸
        QSlider* dihedralSlider = nullptr;      // 二面角阈值滑块（0~180 度）
        QSpinBox* dihedralSpin = nullptr;       // 二面角输入框（与滑块双向联动）
        double m_dihedralAngle = 25.0;          // 二面角阈值（重剖分时 ≥ 该角度的折痕保持为锐边）
	};
}