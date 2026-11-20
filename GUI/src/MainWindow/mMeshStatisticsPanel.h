#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

namespace MechMainWindow
{

	//线网格面板
	class MMAINWINDOW_EXPORT mMeshStatisticsPanel :public mMeshPanel
	{
	public:

        mMeshStatisticsPanel(QWidget* parent = Q_NULLPTR);

		~mMeshStatisticsPanel();

		//更新面板
		void editMeshPanel(int meshId);
        //清空表格
        void StatisticsClear();
        //更新表格
        void StatisticsUpdate();
	private:
		//全局尺寸
		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
        QTableWidget* tab = new QTableWidget();
	};
}