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
	class MMAINWINDOW_EXPORT mRemeshingPanel :public mMeshPanel
	{
	public:

        mRemeshingPanel(QWidget* parent = Q_NULLPTR);

		void apply();

		~mRemeshingPanel();

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
		//全局尺寸
		FoldSection* m_TranslationFold = nullptr;
		QLabel* size;
		//增长率（仅形状为四面体时显示；勾选框默认关闭，控制输入框启停）
		QCheckBox* growthRateBox;
		QLabel* growthRate;
		mCalcLineEdit* growthRateEdit;
		QWidget* growthRateRow;

		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
        QComboBox* methodBox;
        mCalcLineEdit* sizeEdit;
	};
}