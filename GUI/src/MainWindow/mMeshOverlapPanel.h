#pragma once
#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

namespace MechMainWindow
{
	//线网格面板
	class MMAINWINDOW_EXPORT mMeshOverlapPanel :public mMeshPanel
	{
	public:

        mMeshOverlapPanel(QWidget* parent = Q_NULLPTR);
        ~mMeshOverlapPanel();

        //应用
        void apply();
        //清除
        void clear();

		//更新面板
		void editMeshPanel(int meshId);

		//根据数据初始化面板
		void updatePanel() override;

	private:
		//全局尺寸
		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
        QLineEdit* overlapEdit;
	};
}