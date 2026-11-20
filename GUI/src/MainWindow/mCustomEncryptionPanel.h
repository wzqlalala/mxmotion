#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

class SizeField; //【OCC摘除·S10/P0-1 终局2】前置声明（原经 MeshEntity.h 传递；cpp 本地自含完整类型）

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
	//自定义加密面板
	class MMAINWINDOW_EXPORT mCustomEncryptionPanel :public mMeshPanel
	{
		Q_OBJECT
	public:

		mCustomEncryptionPanel(QWidget* parent = Q_NULLPTR);

		~mCustomEncryptionPanel();
		void newMeshPanel() override;
		void newCustomEncryptionPanel();

		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//更新数据初始化面板
		void updatePanel() override;

        //基于几何
        void createEncryptionByGeo(MeshEntity* Entity);
        //几何拾取已清空时，基于已保存的原始包围盒重建加密区域（套用当前偏移/拉伸）
        void recreateBySavedBox(MeshEntity* Entity);
        //基于参数
        void createEncryptionByParam(MeshEntity* Entity);
        //非规则
        void createEncryptionByIrregular(MeshEntity* Entity);
        //根据参数创建圆柱体
        void createRenderCylinderBySetting(SizeField* CylinderField);
        //根据参数创建球
        void createRenderBallBySetting(SizeField* BallField);
        //根据参数创建箱型
        void createRenderBoxBySetting(SizeField* BoxField);
        //根据几何创建箱型
        void createRenderBox(SBoundingBox3d& bd, double  min[3], double  max[3], SizeField* BoxField);
        //设置BoxField的Option(根据几何)
        void setValueBox(SBoundingBox3d& bd, double  min[3], double  max[3], SizeField* BoxField);
        //设置BoxField的Option(根据参数)
        void setValueBoxBySetting(SizeField* BoxField);
        //设置BallField的Option(根据参数)
        void setValueBallBySetting(SizeField* BallField);
        //设置CylinderField的Option(根据参数)
        void setValueCylinderBySetting(SizeField* CylinderField);
        //对箱形包围盒施加拉伸(以中心为基准，上下边分别缩放)与偏移
        void applyStretchOffset(double min[3], double max[3]);

        //重置(删除)该节点的包围盒
		void slot_boxDelete() override;
        //显示所有节点的包围盒
		void slot_boxShow() override;
        //隐藏所有节点的包围盒
		void slot_boxHide() override;
        //预览并保存包围盒，
		void slot_boxPreview() override;
        //清空预览
		void cancelPreview();
        //修改包围盒
		void boxChange();
        //基于参数包围盒
        void boxChangeByParam(MeshEntity* Entity);
	public slots:
        //偏移修改
		void offsetChanged();
        //拉伸修改
		void stretchChanged();
	private:
		//包围盒
		FoldSection* m_boundingBoxFold = nullptr;
		QLabel* shape;
		QLabel* method;
		QLabel* num;
		QLabel* thickness;
		QComboBox* shapeBox;
		QComboBox* methodBox;
		QComboBox* numBox;
        mCalcLineEdit* thicknessEdit; 

		FoldSection* m_meshSizeFold = nullptr;
		QLabel* inside;
		QLabel* outside;
        mCalcLineEdit* insideEdit;
        mCalcLineEdit* outsideEdit; 

		FoldSection* m_offsetFold = nullptr;
		QLabel* x;
		QLabel* y;
		QLabel* z;
        mCalcLineEdit* xEdit;
        mCalcLineEdit* yEdit; 
        mCalcLineEdit* zEdit; 

		FoldSection* m_stretchFold = nullptr;
		QLabel* xup;
		QLabel* xdown;
		QLabel* yup;
		QLabel* ydown;
		QLabel* zup;
		QLabel* zdown;
        mCalcLineEdit* xupEdit; 
        mCalcLineEdit* xdownEdit;
        mCalcLineEdit* yupEdit; 
        mCalcLineEdit* ydownEdit;
        mCalcLineEdit* zupEdit;
        mCalcLineEdit* zdownEdit;

		//箱形
		QLabel* vertex1;
		QLabel* vertex2;
		QLineEdit* vertex1Edit; 
		QLineEdit* vertex2Edit;
		//球形
		QLabel* center;
		QLabel* radius;
		QLineEdit* centerEdit;
        mCalcLineEdit* radiusEdit;
		//底面圆心
		QLabel* downCenter;
		QLabel* downRadius;
		QLabel* height;
		QLabel* axial;
		QLineEdit* downCenterEdit; 
        mCalcLineEdit* downRadiusEdit;
        mCalcLineEdit* heightEdit;
		QLineEdit* axialEdit;

		int currentId = 0;
	};
}