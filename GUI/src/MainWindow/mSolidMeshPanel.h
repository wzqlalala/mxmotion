#include "mmainwindow_global.h"
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

	//体网格面板
	class MMAINWINDOW_EXPORT mSolidMeshPanel :public mMeshPanel
	{
	public:

		mSolidMeshPanel(QWidget* parent = Q_NULLPTR);

		~mSolidMeshPanel();

		void newMeshPanel() override;

		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;
		//初始化网格尺寸
		void setMeshSize();
		//初始化默认参数
		void initSize() override;


	private:
		// 刷新实体集合下拉框：加载模型树中所有部件名
		void refreshGeoComboBox();
		//全局尺寸
		FoldSection* m_rangeFold = nullptr;
		QLabel* _meshShapeLabel;
		QComboBox* _meshShapeBox;
		QLabel* _meshSizeLabel;
		QLineEdit* _meshSizeEdit;
		QToolButton* _meshSizeButton;
		QPushButton* _comfirmButton;
		//MeshData::mFunctionBase* _funcData = nullptr;

		FoldSection* m_GeoFold = nullptr;
		QLabel* _geoLabel;
		QComboBox* _geoComboBox;

		FoldSection* m_OrderFold = nullptr;
		QComboBox* _orderComboBox;
		QToolButton* allBtn;
		QToolButton* addBtn;
		QToolButton* reduceBtn;
		QToolButton* deleteBtn;
		//用户手动输入过尺寸值标志：手动输入后添加/删减剖分几何时不再自动更新尺寸
		bool _userSizeManuallyEdited = false;
		//程序化更新尺寸文本标志：区分程序 setText 与用户手动输入
		bool _programmaticSizeUpdate = false;
	};


}