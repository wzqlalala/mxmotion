#pragma once
#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"


using namespace MxFunctions;
using namespace MGUIMesh;
namespace MechPreData
{
	class mViewportModel;
}

class MXMesh;
namespace MechMainWindow
{
	//线网格面板
	class MMAINWINDOW_EXPORT mTEdgePanel :public mMeshPanel
	{
	public:
        mTEdgePanel(QWidget* parent = Q_NULLPTR);

		~mTEdgePanel();

		//更新面板
		void editMeshPanel(int meshId);
		//根据数据初始化面板
		void updatePanel() override;

        //查找
        void search();
        //提取
        void extract();

	private:
		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;

        QLineEdit* partEdit;
	};
}