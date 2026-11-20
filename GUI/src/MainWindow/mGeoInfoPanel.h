#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QComboBox>
#include <qvector3d.h>

#include <set>

#include "mPanelBase.h"
#include "mModelEnum.h"
#include "mUnitSystemManager.h"

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mGeoInfoPanel : public mPanelBase
	{
		Q_OBJECT

	public:

		/*
		* 构造函数
		*/
		mGeoInfoPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mGeoInfoPanel();

		void showPanel(const BrowserTreeType type);

		void getGeoInfoById(const std::vector<int> ids);

		void updateInfo(const std::vector<int> ids);

        /*
        * 连接单位制响应信号
        */
        void connectUnitSystemResponse();

        /*
        * 断开单位制响应信号
        */
        void disconnectUnitSystemResponse();

	private:

		void initParentNodeFold();

		void initChildNodeFold();

    public:
		//设置部件透明度
		void setPartAlpha(const std::vector<int> partIds, double scale);

    private:
		//设置部件可见性
		void setPartsVisible(const std::vector<int> ids);

		//获取可见性-- 0:可见 ， 1:不可见，-1:空白（存在不同结果的部件）
		int getPartsVisible(const std::vector<int> partIds);

		//获取部件颜色-存在不同颜色时返回白色
		QColor getPartsColor(const std::vector<int> partIds);

		//获取部件透明度-存在不同透明度时返回最大值65535
		int getPartAlpha(const std::vector<int> partIds);

		//获取体积
		double getPartsVolume(const std::vector<int> partIds);

		//获取包围盒
		QVector3D getPartsBndBox(const std::vector<int> partIds);

		//获取质心
		QVector3D getPartsBarycenter(const std::vector<int> partIds);

		//获取单元数和节点数
		QPair<int, int> getPartsElementNodeCount(const std::vector<int> partIds);

		//获取截面属性
		std::vector<QString> getPartsProperty(const std::vector<int> partIds);

		//获取所有截面属性对应的材料
		QMap<QString, QList<QString>> getPartsPropertyMat();

		//设置当所选部件的截面属性并显示截面属性对应的材料
		void setPartsProperty(const std::vector<int> ids);

		//显示几何结构父节点信息
		void showGeoParentNodeInfo();

		//显示几何结构子节点信息
		void showGeoChildNodeInfo(const std::vector<int> partIds);

		//收集部件及其全部递归子节点；选中父节点时属性值汇总所有子节点
		std::vector<int> collectPartsWithDescendants(const std::vector<int> partIds);

		//事件过滤
		bool eventFilter(QObject* obj, QEvent* event);

		//已选中的部件id
		std::vector<int> selPartIds;

		//所有截面属性以及对应的材料
		QMap<QString, QList<QString>> propMatMap;

		QVBoxLayout* _mainLayout;

		//几何结构父节点控件
		QLineEdit* lenX;
		QLineEdit* lenY;
		QLineEdit* lenZ;
		QLineEdit* volumeLine;
		QLineEdit* prtCountBtn;
		QLineEdit* elementCountBtn;
		QLineEdit* nodeCountBtn;
        QLabel* unitLenX;
        QLabel* unitLenY;
        QLabel* unitLenZ;
		
		//几何子节点控件
		QComboBox* visibleCombo;
		QSlider* diaphaneitySlider;
		QDoubleSpinBox* diaphaneitySpinBox;
		QPushButton* colorBtn;
		QComboBox* propertieCombo;
		QToolButton* createProperty;
		QHBoxLayout* propertyLayout;
		QLineEdit* materialLine;
		QLineEdit* childlenX;
		QLineEdit* childlenY;
		QLineEdit* childlenZ;
		QLineEdit* barycenterX;
		QLineEdit* barycenterY;
		QLineEdit* barycenterZ;
		QLineEdit* childVolume;
		QLineEdit* childElementCount;
		QLineEdit* childNoteCount;

        QLabel* unitChdX;
        QLabel* unitChdY;
        QLabel* unitChdZ;
        QLabel* unitCenterX;
        QLabel* unitCenterY;
        QLabel* unitCenterZ;

        bool connectUnitChange = false;
        QList< QPair<QLabel*,QLineEdit*>> m_unitLabels;
	private slots:

		//设置部件颜色
		void slot_setPartColor();

        //切换单位制
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);
		
	};
}