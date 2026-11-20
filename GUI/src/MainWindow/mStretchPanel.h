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

	//线网格面板
	class MMAINWINDOW_EXPORT mStretchPanel :public mMeshPanel
	{
	public:

		mStretchPanel(QWidget* parent = Q_NULLPTR);

		void applyStretch();

		~mStretchPanel();

		void newMeshPanel() override;
		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;
		//初始化默认尺寸
		void setMeshSize();
		//初始化默认参数
		void initSize() override;
	private:

		FoldSection* m_ObjectFold = nullptr;            // 对象下拉框
		FoldSection* m_paramFold = nullptr;             // 参数下拉框

		QComboBox* methodBox;                           // 拉伸方式
		QLabel* distance; mCalcLineEdit* distanceEdit;  // 拉伸距离
		QLabel* layers; mCalcLineEdit* layersEdit;      // 拉伸层数


		vector<RenderPickResultData> MeshIDs;           // 拾取的网格数据

		vector<int> edgeIds;
		QComboBox* discretizationBox;
		mCalcLineEdit* startSizeEdit;

		PosPickWidget1 posPick;
		PosPickWidget1 curvePick;
		VecPickWidget vecPick;
	};
}