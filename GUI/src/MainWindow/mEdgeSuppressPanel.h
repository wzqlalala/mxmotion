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
	//边抑制面板：仅包含几何线拾取模块
	class MMAINWINDOW_EXPORT mEdgeSuppressPanel :public mMeshPanel
	{
		Q_OBJECT
	public:
		mEdgeSuppressPanel(QWidget* parent = Q_NULLPTR);
		~mEdgeSuppressPanel();

		void newMeshPanel() override;
		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;
		//初始化默认参数
		void initSize() override;

	private:
		FoldSection* m_GeoFold = nullptr;
		QLabel* _geoLabel = nullptr;
		QComboBox* _geoComboBox = nullptr;

		QListWidget* _listAssignPart = nullptr;
		QToolButton* addBtn = nullptr;
		QToolButton* reduceBtn = nullptr;
		QToolButton* deleteBtn = nullptr;
	};
}
