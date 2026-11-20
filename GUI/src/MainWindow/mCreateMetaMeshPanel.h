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



	//创建元网格面板
	class MMAINWINDOW_EXPORT mCreateMetaMeshPanel :public mMeshPanel
	{
		Q_OBJECT
	public:

        mCreateMetaMeshPanel(QWidget* parent = Q_NULLPTR);

		~mCreateMetaMeshPanel();
		void newMeshPanel() override;

		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;



 


	private:
		//多线程
		mMeshThread* _meshthread = nullptr;

        //部件部分
		FoldSection* m_rangeFold = nullptr;
		QLabel* _meshControlLabel;
		QComboBox* _meshControlBox;
		QLineEdit* _meshControlEdit;
		QPushButton* _comfirmButton;

        //拓扑信息部分
        FoldSection* m_TopologyFold = nullptr;

        //网格数量部分
        FoldSection* m_ParamFold = nullptr;

        //网格尺寸部分
		FoldSection* m_SizeFold = nullptr;

        PosPickWidget1 posPick;
        PosPickWidget1 posPick1;
        QStackedWidget* stackWidget;
        VecPickWidget vecPick;
        VecPickWidget vecPick1;
	};
}