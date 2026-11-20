#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <memory>

//QT
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QLabel>
#include <QRadioButton>

#include "mPanelBase.h"
#include "mMeshPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mMeshPanel.h"

using namespace MGUIMesh;
namespace MechPreData
{
    class mViewportModel;
}
class MXMesh;
namespace MechMainWindow
{

    // 改变阶数面板
    class MMAINWINDOW_EXPORT mChangeOrderPanel :public mMeshPanel
    {
    public:

        mChangeOrderPanel(QWidget* parent = Q_NULLPTR);

        ~mChangeOrderPanel();

        void apply();

        void newMeshPanel() override;
        //更新面板
        void editMeshPanel(int meshId);
        //根据数据初始化面板
        void updatePanel() override;

    private:
        // 全局尺寸
        FoldSection* m_paramFold = nullptr;
        // 一阶
        QRadioButton* orderBox1;
        // 二阶
        QRadioButton* orderBox2;

        // 拾取的网格单元
        vector<RenderPickResultData> MeshIDs;

        PosPickWidget1 posPick;
    };
}
