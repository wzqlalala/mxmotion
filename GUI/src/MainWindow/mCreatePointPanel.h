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
	class MMAINWINDOW_EXPORT mCreatePointPanel :public mMeshPanel
	{
		Q_OBJECT
	public:

        mCreatePointPanel(QWidget* parent = Q_NULLPTR);

		~mCreatePointPanel();
		void newMeshPanel() override;

		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;

		void apply();
        //基于所选对象平均-更新节点坐标
		void updateNodePos();

	private:
		//多线程
		mMeshThread* _meshthread = nullptr;

		FoldSection* m_rangeFold = nullptr;
		QLabel* _meshControlLabel;
		QComboBox* _meshControlBox;
		QPushButton* _comfirmButton;

		FoldSection* m_ParamFold = nullptr;

        PosPickWidget1 posPick;
        PosPickWidget1 posPick1;
        QStackedWidget* stackWidget;
        VecPickWidget vecPick;

        QLineEdit* xEdit;
        QLineEdit* yEdit;
        QLineEdit* zEdit;
        //【镜像六件套摘除·连带】渲染头不再泄漏 using namespace mxr，类型改为显式限定
        vector<mxr::MeshSelectInfoData> nodes;

        QLineEdit* coorEdit;

        QLineEdit* disEdit;
	};
}