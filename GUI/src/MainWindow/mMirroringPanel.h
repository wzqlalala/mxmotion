#pragma once
#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

namespace MechMainWindow
{
    //线网格面板
    class MMAINWINDOW_EXPORT mMirroringPanel :public mMeshPanel
    {
    public:
        mMirroringPanel(QWidget* parent = Q_NULLPTR);

        ~mMirroringPanel();

        //应用
        void apply();
        //点1
        void pointChanged();

        //更新面板
        void editMeshPanel(int meshId);
        //根据数据初始化面板
        void updatePanel() override;
    private:
        QLabel* method_label;
        QComboBox* method;
        QLabel* point;

        vector<RenderPickResultData> MeshIDs;

        PosPickWidget1 posPick;     // 对象下拉框
        VecPickWidget vecPick;      // 方向拾取

        QLineEdit* pointXEdit;
        QLineEdit* pointYEdit;
        QLineEdit* pointZEdit;

        QCheckBox* newMeshPart;
    };
}