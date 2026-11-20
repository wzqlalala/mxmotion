#pragma once
#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

namespace MechPreData
{
	class mViewportModel;
}
class MXMesh;
namespace MechMainWindow
{
	//线网格面板
	class MMAINWINDOW_EXPORT mMeshIntersectPanel :public mMeshPanel
	{
	public:

        mMeshIntersectPanel(QWidget* parent = Q_NULLPTR);

        ~mMeshIntersectPanel();

        //应用
        void apply();
        //清除高亮
        void clear();

		//更新面板
		void editMeshPanel(int meshId);

		//根据数据初始化面板
		void updatePanel() override;

	private:
		//全局尺寸
		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
	};
}