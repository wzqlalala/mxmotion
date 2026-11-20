#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

namespace MechMainWindow
{

    // 删除网格面板
    class MMAINWINDOW_EXPORT mDeleteMeshPanel :public mMeshPanel
    {
    public:

        mDeleteMeshPanel(QWidget* parent = Q_NULLPTR);

        ~mDeleteMeshPanel();

        void deletemesh();

        // 编辑面板
        void editMeshPanel(int meshId);
        // 更新面板
        void updatePanel() override;

    private:
        //全局尺寸
        vector<RenderPickResultData> MeshIDs;
        vector<RenderPickResultData> PartIDs;
        QComboBox* partBox;
        PosPickWidget1 posPick;
    };
}