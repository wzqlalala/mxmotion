#pragma once
#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

namespace MechMainWindow
{
	// 自由边面板
	class MMAINWINDOW_EXPORT mFreeEdgePanel :public mMeshPanel
	{
	public:

        mFreeEdgePanel(QWidget* parent = Q_NULLPTR);

        ~mFreeEdgePanel();

        // 编辑面板
        void editMeshPanel(int meshId);
        // 更新面板
        void updatePanel() override;

        // 查找
        void search();
        // 提取
        void extract();
       
	private:
		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
        QLineEdit* partEdit;
	};
}