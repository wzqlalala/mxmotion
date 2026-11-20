#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QLabel>

#include "mPanelBase.h"
#include "mMeshPanelBase.h"

#include "mPhysicalFieldEnum.h"

#include "mMeshThread.h"
#include "mMeshPanel.h"

#include "mxr/view/HighlightController.h"
using namespace MxFunctions;
using namespace MGUIMesh;
namespace MechPreData
{
	class mViewportModel;
}
class MXMesh;
namespace MechMainWindow
{



	//局部尺寸面板
	class MMAINWINDOW_EXPORT mMeshEditPanel :public mMeshPanel
	{
		Q_OBJECT
	public:

        mMeshEditPanel(QWidget* parent = Q_NULLPTR);

		~mMeshEditPanel();

		void newMeshPanel() override;

		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;
        //
        void clearCheck();
    private slots:
        //交换边
        void slot_swapedge();
        //移动节点
        void slot_placenode();
        //合并边
        void slot_collapseedge();
        //分割边
        void slot_splitedge();
        //三角形单元合并
        void slot_combinetri();
        //四边形单元分割
        void slot_splitquad();
        //处理拾取
        void slot_dealPickedFinished();

	private:
		FoldSection* m_rangeFold = nullptr;

        // 按钮容器
        vector<QToolButton*>buttons;

        //功能选择，默认为没有功能
        int function_flag = 0;

        QToolButton* btnPlaceNode;
        QToolButton* btnSwapEdge;
        QToolButton* btnCollpaseEdge;
        QToolButton* btnSplitEdge;
        QToolButton* btnCombineTri;
        QToolButton* btnSplitQuad;
	};
} 