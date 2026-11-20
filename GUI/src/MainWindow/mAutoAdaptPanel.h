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
#include "mOtherFunction.h"
using namespace MxFunctions;
using namespace MGUIMesh;
namespace MechPreData
{
	class mViewportModel;
}
class MXMesh;
namespace MechMainWindow
{
	struct mAutoAdaptParam
	{
		QCheckBox* checkBox = nullptr;
		QLabel* label = nullptr;
		QLineEdit* lineEdit = nullptr;
		QHBoxLayout* layout = nullptr;
		QWidget* widget = nullptr;
		void initWidget(QCheckBox* checkBox1, QLabel* label1, QLineEdit* lineEdit1, QString key, double value)
		{
			checkBox = checkBox1;
			label = label1;
			lineEdit = lineEdit1;
			lineEdit->setText(MxFunctions::mNumberToString(value));
			layout = new QHBoxLayout;
			widget = new QWidget;
			layout->setMargin(0);
			layout->setSpacing(0);
			if (checkBox)
				layout->addWidget(checkBox);
			else
			{
				// 保留 checkbox 占位，使无 checkbox 时编辑框与有 checkbox 的行对齐
				auto* placeholder = new QWidget(widget);
				placeholder->setFixedWidth(20);
				layout->addWidget(placeholder);
			}
			layout->addWidget(label);
			layout->addWidget(lineEdit, 1);
			widget->setWindowFlags(Qt::Widget);
			widget->setContentsMargins(0, 0, 0, 0);
			widget->setLayout(layout);
		}
	};
	//网格自适应面板
	class MMAINWINDOW_EXPORT mAutoAdaptPanel :public mMeshPanel
	{
	public:

		mAutoAdaptPanel(QWidget* parent = Q_NULLPTR);

		~mAutoAdaptPanel();
		void newMeshPanel() override;
		void newAutoAdaptPanel();

		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;

		void slot_createMeshAutoAdapt() override;
		//设置最大尺寸
		void setMaxSize();
		//设置最小尺寸
		void setMinSize();
		//初始化默认尺寸
		void initSize() override;
	private:
		// 刷新几何特征集合下拉框：加载模型树中所有部件名
		void refreshGeoComboBox();
		//全局尺寸
		FoldSection* m_paramFold = nullptr;
		QVector<mAutoAdaptParam> inputWidgets{ 3 };
		QCheckBox* checkBox1;
		QCheckBox* checkBox2;
		QCheckBox* checkBox3;
		QLabel* dimSmoothnessRatio;
		QLabel* curvatureEigenValue;
		QLabel* geoProximityValue;
		QLabel* maxSize;
		QLabel* minSize;
		QLabel* geoFeature;
		QComboBox* geoFeatureBox;
		QLineEdit* dimSmoothnessRatioEdit;
		QLineEdit* curvatureEigenValueEdit;
		QLineEdit* geoProximityValueEdit;
		QLineEdit* maxSizeEdit;
		QToolButton* maxSizeButton;
		QLineEdit* minSizeEdit;
		QToolButton* minSizeButton;
		FoldSection* m_GeoFold = nullptr;
		QComboBox* _geoComboBox;
		QLabel* _geoLabel;


		int dim = 1;
		QToolButton* allBtn;
		QToolButton* addBtn;
		QToolButton* reduceBtn;
		QToolButton* deleteBtn;


	};
}
