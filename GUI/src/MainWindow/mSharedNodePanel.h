#pragma once
#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

namespace MechMainWindow
{
	// 共节点面板
	class MMAINWINDOW_EXPORT mSharedNodePanel :public mMeshPanel
	{
	public:

        mSharedNodePanel(QWidget* parent = Q_NULLPTR);

        ~mSharedNodePanel();

        // 查找
        void search();

        // 合并
        void combine();
		
		//更新面板
		void editMeshPanel(int meshId);

		//根据数据初始化面板
		void updatePanel() override;
	private:
		// 参数折叠框
		FoldSection* m_paramFold = nullptr;
        // 拾取器
        PosPickWidget1 posPick;
        // 拾取的网格单元
		vector<RenderPickResultData> MeshIDs;
        // 容差
        QLineEdit* tolEdit;
        // 查询的共节点
        vector<vector<int>> nodeIds;
	};
}