#include "mGeoInfoPanel.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
#include "mGeoManager.h"
#include <mMeshData.h>
#include <mMenuWidget.h>

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/basic/PublicStruct.h"
#include "mxr/view/CCSMPreRender.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include <QMenu>
#include <QtCore/qglobal.h>
#include <QColorDialog>
#include <QSlider>
#include <QSpinBox>
#include <qlist.h>
#include <qmap.h>
#include <mPropertyBase.h>
#include "PickCommonFunction.h"

//#include "mGeoInfoCalculate.h" //【OCC摘除·S10/P0-1】死 include（S9 已退役，零使用）
#include "mDataStatusManager.h"
#include "mOtherFunction.h"

using namespace mxr;
using namespace MechPropertyData;
using namespace MechCommon;
using namespace MechPreData
;
namespace MechMainWindow
{
	mGeoInfoPanel::mGeoInfoPanel(QWidget* parent)
		: mPanelBase(parent)
	{
		//this->setStyleSheet(
		//	"mCoordinatePanel { background-color: white; }"

		//	/* 关键：把 QComboBox 的下拉列表也刷成白底黑字 */
		//	"QComboBox QAbstractItemView {"
		//	"    background-color: white;"     /* 白底   */
		//	"    color: black;"                /* 黑字   */
		//	"    selection-background-color: #3399ff;" /* 悬停高亮，可自定 */
		//	"    selection-color: black;"      /* 悬停时文字保持黑色 */
		//	"}");

		_mainLayout = new QVBoxLayout(this);

		initParentNodeFold();
		initChildNodeFold();

		_mainLayout->addStretch();

        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitLenX, lenX));
        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitLenY, lenY));
        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitLenZ, lenZ));
        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitChdX, childlenX));
        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitChdY, childlenY));
        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitChdZ, childlenZ));
        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitCenterX, barycenterX));
        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitCenterY, barycenterY));
        m_unitLabels.append(QPair<QLabel*, QLineEdit*>(unitCenterZ, barycenterZ));
	}

	mGeoInfoPanel::~mGeoInfoPanel()
	{

	}

    void mGeoInfoPanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mGeoInfoPanel::slot_unitSystemResponse, Qt::UniqueConnection);
        connectUnitChange = true;
    }

    void mGeoInfoPanel::disconnectUnitSystemResponse()
    {
        if (connectUnitChange)
        {
            disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mGeoInfoPanel::slot_unitSystemResponse);
            connectUnitChange = false;
        } 
    }

    void mGeoInfoPanel::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & UnitSystem)) return;
        if (getCVM == nullptr)
        {
            return;
        }

        QString unit = UnitSystems[idx[0]][0];

        double ratio = getCVM->getModelData()->getUnitSystemManager()->getSwitchUnitSystemRatio(Unit::Length);
        for (auto& unitLabel : m_unitLabels)
        {
            unitLabel.first->setText(unit);
            QString valuestr = unitLabel.second->text().trimmed();
            if (!valuestr.isEmpty())
            {
                double value = valuestr.toDouble();
                value *= ratio;
                unitLabel.second->setText(MxFunctions::mNumberToString(value));
            }
        }
    }

	void mGeoInfoPanel::showPanel(const BrowserTreeType type)
	{
		for (auto it : m_foldWidgets)
		{
			it->setVisible(false);
		}

		switch (type)
		{
		case ModelParentItem:
			m_foldWidgets[0]->setVisible(true);
			m_foldWidgets[1]->setVisible(true);
			break;
		case ModelChildItem:
			m_foldWidgets[2]->setVisible(true);
			m_foldWidgets[3]->setVisible(true);
			m_foldWidgets[4]->setVisible(true);
			break;
		default:
			break;
		}


	}

	void mGeoInfoPanel::initParentNodeFold()
	{
		lenX = new mCalcLineEdit(this);
		lenY = new mCalcLineEdit(this);
		lenZ = new mCalcLineEdit(this);
		volumeLine = new mCalcLineEdit(this);
		prtCountBtn = new mCalcLineEdit(this);
		elementCountBtn = new mCalcLineEdit(this);
		nodeCountBtn = new mCalcLineEdit(this);

        
        unitLenX = new QLabel("mm", this);
        unitLenY = new QLabel("mm", this);
        unitLenZ = new QLabel("mm", this);

		lenX->setDisabled(true);
		lenY->setDisabled(true);
		lenZ->setDisabled(true);
		volumeLine->setDisabled(true);
		prtCountBtn->setDisabled(true);
		elementCountBtn->setDisabled(true);
		nodeCountBtn->setDisabled(true);

		FoldSection* parentNodeFold_1 = new FoldSection("属性", this, QColor(0, 0, 0));
		parentNodeFold_1->addRow(new QLabel("长度X"), lenX, unitLenX);
		parentNodeFold_1->addRow(new QLabel("长度Y"), lenY, unitLenY);
		parentNodeFold_1->addRow(new QLabel("长度Z"), lenZ, unitLenZ);
		parentNodeFold_1->addRow(new QLabel("体积"), volumeLine, new QLabel("mm^3"));

		m_foldWidgets.append(parentNodeFold_1);
		_mainLayout->addWidget(parentNodeFold_1);

		FoldSection* parentNodeFold_2 = new FoldSection("统计", this);
		parentNodeFold_2->addRow(new PanelLabel("部件数量"), prtCountBtn);
		parentNodeFold_2->addRow(new PanelLabel("单元数量"), elementCountBtn);
		parentNodeFold_2->addRow(new PanelLabel("节点数量"), nodeCountBtn);

		m_foldWidgets.append(parentNodeFold_2);
		_mainLayout->addWidget(parentNodeFold_2);

	}

	void mGeoInfoPanel::initChildNodeFold()
	{
        unitChdX = new QLabel("mm", this);
        unitChdY = new QLabel("mm", this);
        unitChdZ = new QLabel("mm", this);
        unitCenterX = new QLabel("mm", this);
        unitCenterY = new QLabel("mm", this);
        unitCenterZ = new QLabel("mm", this);

		visibleCombo = new QComboBox(this);
		visibleCombo->addItems({ "是","否" });
		connect(visibleCombo, QOverload<int>::of(&QComboBox::activated), [this](int index) {
			setPartsVisible(selPartIds);
			});


		diaphaneitySlider = new QSlider(Qt::Horizontal, this);
		diaphaneitySpinBox = new QDoubleSpinBox(this);
		colorBtn = new QPushButton(this);
		propertieCombo = new QComboBox(this);
		createProperty = new QToolButton(this);
		createProperty->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/jiemianshuxing.png"));
		createProperty->setToolTip("新建截面属性");
		connect(createProperty, &QToolButton::clicked, this, [this]() {
			MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("截面属性");
			});

		connect(propertieCombo, QOverload<int>::of(&QComboBox::activated), [this](int index) {
			setPartsProperty(selPartIds);
			});

		diaphaneitySlider->setRange(0, 100);
		diaphaneitySpinBox->setRange(0, 1.00);
		diaphaneitySpinBox->setSingleStep(0.01);

		// 信号槽连接（带转换）
		connect(diaphaneitySpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			diaphaneitySlider, [=](double value) { 
				diaphaneitySlider->setValue(value * 100); 
				if (diaphaneitySpinBox->hasFocus())
				{
					setPartAlpha(selPartIds, diaphaneitySpinBox->text().toDouble());
				}
			});

		connect(diaphaneitySlider, &QSlider::valueChanged,
			diaphaneitySpinBox, [=](int value) { diaphaneitySpinBox->setValue(value / 100.0); });

		connect(diaphaneitySlider, &QSlider::sliderReleased,
			diaphaneitySpinBox, [=]() { 
				setPartAlpha(selPartIds, diaphaneitySpinBox->text().toDouble());
			});

		// 安装事件过滤器
		diaphaneitySlider->installEventFilter(this);

		propertyLayout = new QHBoxLayout(this);

		materialLine = new QLineEdit(this);
		childlenX = new QLineEdit(this);
		childlenY = new QLineEdit(this);
		childlenZ = new QLineEdit(this);
		barycenterX = new QLineEdit(this);
		barycenterY = new QLineEdit(this);
		barycenterZ = new QLineEdit(this);
		childVolume = new QLineEdit(this);
		childElementCount = new QLineEdit(this);
		childNoteCount = new QLineEdit(this);

		materialLine->setDisabled(true);
		childlenX->setDisabled(true);
		childlenY->setDisabled(true);
		childlenZ->setDisabled(true);
		barycenterX->setDisabled(true);
		barycenterY->setDisabled(true);
		barycenterZ->setDisabled(true);
		childVolume->setDisabled(true);
		childElementCount->setDisabled(true);
		childNoteCount->setDisabled(true);

		connect(colorBtn, &QPushButton::clicked, this, &mGeoInfoPanel::slot_setPartColor);

		FoldSection* childNodeFold_1 = new FoldSection("设置", this);
		childNodeFold_1->addRow(new QLabel("可见"), visibleCombo);
		childNodeFold_1->addRow(new QLabel("透明度"), diaphaneitySlider, diaphaneitySpinBox);
		childNodeFold_1->addRow(new QLabel("颜色"), colorBtn);
		childNodeFold_1->addRow(new QLabel("截面属性"), propertieCombo, createProperty);
		childNodeFold_1->addRow(new QLabel("材料"), materialLine);

		m_foldWidgets.append(childNodeFold_1);
		_mainLayout->addWidget(childNodeFold_1);

		FoldSection* childNodeFold_2 = new FoldSection("属性", this, QColor(50, 50, 50), false);
		childNodeFold_2->addRow(new QLabel("长度X"), childlenX, unitChdX);
		childNodeFold_2->addRow(new QLabel("长度Y"), childlenY, unitChdY);
		childNodeFold_2->addRow(new QLabel("长度Z"), childlenZ, unitChdZ);
		childNodeFold_2->addRow(new QLabel("质心X"), barycenterX, unitCenterX);
		childNodeFold_2->addRow(new QLabel("质心Y"), barycenterY, unitCenterY);
		childNodeFold_2->addRow(new QLabel("质心Z"), barycenterZ, unitCenterZ);
		childNodeFold_2->addRow(new QLabel("体积"), childVolume, new QLabel("mm^3"));

		m_foldWidgets.append(childNodeFold_2);
		_mainLayout->addWidget(childNodeFold_2);

		FoldSection* childNodeFold_3 = new FoldSection("统计", this, QColor(50, 50, 50), false);
		childNodeFold_3->addRow(new QLabel("单元数量"), childElementCount);
		childNodeFold_3->addRow(new QLabel("节点数量"), childNoteCount);

		m_foldWidgets.append(childNodeFold_3);
		_mainLayout->addWidget(childNodeFold_3);

	}

	

	// 实现事件过滤器
	bool mGeoInfoPanel::eventFilter(QObject* obj, QEvent* event)
	{
		if (obj == diaphaneitySlider && event->type() == QEvent::Wheel) {
			// 拦截滚轮事件，不传递给 QSlider
			return true;
		}
		return QObject::eventFilter(obj, event);
	}

	//显示几何结构父节点信息
    void mGeoInfoPanel::showGeoParentNodeInfo()
    {
        //视口模型已删除或未就绪时直接返回
        if (getCVM == nullptr)
        {
            return;
        }

        vector<int> ids = getCVM->getModelData()->getMeshData()->getAllPartIds();

        if (ids.size() <= 0)
        {
            return;
        }

        //连接单位转换响应信号
        connectUnitSystemResponse();
        //获取当前单位进行转换
        QVector<int> unitIndex = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitSystems[unitIndex[0]][0];
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, { 3,0,0,0 }, unitIndex);
        unitLenX->setText(unit);
        unitLenY->setText(unit);
        unitLenZ->setText(unit);
   
		//包围盒
		QVector3D bndBox = getPartsBndBox(ids);
		lenX->setText(MxFunctions::mNumberToString(bndBox.x() * ratio, 'f', 5));
		lenY->setText(MxFunctions::mNumberToString(bndBox.y() * ratio, 'f', 5));
		lenZ->setText(MxFunctions::mNumberToString(bndBox.z() * ratio, 'f', 5));

		//体积
		double dVolume = getPartsVolume(ids);
		volumeLine->setText(MxFunctions::mNumberToString(dVolume, 'f', 5));

		//部件数量
		int nPartCount = getCVM->getModelData()->getMeshData()->getPartCount();
		prtCountBtn->setText(MxFunctions::mNumberToString(nPartCount));

		//单元、节点数
		QPair<int, int> pElementNodes = getPartsElementNodeCount(ids);
		nodeCountBtn->setText(MxFunctions::mNumberToString(pElementNodes.first));
		elementCountBtn->setText(MxFunctions::mNumberToString(pElementNodes.second));

	}


	//显示几何结构子节点信息
	void mGeoInfoPanel::showGeoChildNodeInfo(const std::vector<int> partIds)
	{
		//视口模型已删除或未就绪时直接返回：getGEOServer 链会对空指针解引用导致崩溃
		if (getCVM == nullptr)
		{
			return;
		}

		//可见性
		int nVisible = getPartsVisible(partIds);
		switch (nVisible)
		{
		case -1:
			visibleCombo->setCurrentIndex(-1);
			break;
		case 0:
			visibleCombo->setCurrentText("是");
			break;
		case 1:
			visibleCombo->setCurrentText("否");
			break;

		default:
			break;
		}

		//颜色
		QColor partColor = getPartsColor(partIds);
		QString styleSheet =
			"QPushButton{"
			"background-color: " + partColor.name() + ";"
			"color: " + partColor.name() + ";"
			"}";
		colorBtn->setStyleSheet(styleSheet);

		//透明度
		double dAlphaScale = 0.00;
		int nAlpha = getPartAlpha(partIds);
		diaphaneitySpinBox->blockSignals(true);
		diaphaneitySlider->blockSignals(true);
		dAlphaScale = 1.00 - nAlpha / 65535.00;
		diaphaneitySpinBox->setValue(dAlphaScale);
		diaphaneitySlider->setValue(dAlphaScale * 100);
		diaphaneitySpinBox->blockSignals(false);
		diaphaneitySlider->blockSignals(false);

		//属性与统计汇总范围：选中父节点时包含全部子节点
		std::vector<int> aggregateIds = collectPartsWithDescendants(partIds);

		//体积
		double dVolume = getPartsVolume(aggregateIds);
		childVolume->setText(MxFunctions::mNumberToString(dVolume, 'f', 5));

        //连接单位转换响应信号
        connectUnitSystemResponse();
        //获取当前单位进行转换
        QVector<int> unitIndex = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitSystems[unitIndex[0]][0];
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, { 3,0,0,0 }, unitIndex);
        unitChdX->setText(unit);
        unitChdY->setText(unit);
        unitChdZ->setText(unit);
        unitCenterX->setText(unit);
        unitCenterY->setText(unit);
        unitCenterZ->setText(unit);

		//包围盒
		QVector3D bndBox = getPartsBndBox(aggregateIds);
		childlenX->setText(MxFunctions::mNumberToString(bndBox.x() * ratio, 'f', 5));
		childlenY->setText(MxFunctions::mNumberToString(bndBox.y() * ratio, 'f', 5));
		childlenZ->setText(MxFunctions::mNumberToString(bndBox.z() * ratio, 'f', 5));


		//质心
		QVector3D weightCent = getPartsBarycenter(aggregateIds);
		barycenterX->setText(MxFunctions::mNumberToString(weightCent.x() * ratio, 'f', 5));
		barycenterY->setText(MxFunctions::mNumberToString(weightCent.y() * ratio, 'f', 5));
		barycenterZ->setText(MxFunctions::mNumberToString(weightCent.z() * ratio, 'f', 5));

		//单元、节点数
		QPair<int, int> pElementNodes = getPartsElementNodeCount(aggregateIds);
		childNoteCount->setText(MxFunctions::mNumberToString(pElementNodes.first));
		childElementCount->setText(MxFunctions::mNumberToString(pElementNodes.second));


		//截面属性
		propertieCombo->clear();
		propertieCombo->addItems(getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property).getAllPropertyNames());
		std::vector<QString> vecPropName = getPartsProperty(partIds);
		if (vecPropName.size() == 1)
		{
			propertieCombo->setCurrentText(vecPropName[0]);
		}
		else
		{
			propertieCombo->setCurrentIndex(-1);
		}

		//材料
		propMatMap.clear();
		propMatMap = getPartsPropertyMat();

		//if (propMatMap.isEmpty())
		//	propMatMap = getPartsPropertyMat(partIds);

	    //显示材料
		QString matName = "";
		for (auto name : propMatMap[propertieCombo->currentText()])
		{
			matName += name + ";";
		}
		matName.chop(1);
		materialLine->setText(matName);
	}

//收集部件及其全部递归子节点；选中父节点时属性值汇总所有子节点
std::vector<int> mGeoInfoPanel::collectPartsWithDescendants(const std::vector<int> partIds)
{
	if (getCVM == nullptr)
	{
		return partIds;
	}

	std::set<int> collected(partIds.begin(), partIds.end());
	std::vector<int> pending(partIds.begin(), partIds.end());

	while (!pending.empty())
	{
		const int partId = pending.back();
		pending.pop_back();

		GeoNeutralData::GN_Part part;
		try
		{
			part = getGEOServer.context().PartManager().getPartById(partId);
		}
		catch (...)
		{
			continue;
		}

		for (int childId : part.childIds())
		{
			if (childId > 0 && collected.insert(childId).second)
			{
				pending.push_back(childId);
			}
		}
	}

	return std::vector<int>(collected.begin(), collected.end());
}

//获取可见性-- 0:可见 ， 1:不可见，-1:空白（存在不同结果的部件）
int mGeoInfoPanel::getPartsVisible(const std::vector<int> partIds)
	{
		if (getCVM == nullptr)
		{
			return -1;
		}

		int n = -1;
		for (auto id : partIds)
		{
			bool isVisible = getCVM->getModelData()->getMeshData()->getPartMask(id);
			if (n != -1 && n != isVisible)
			{
				return -1;
			}

			isVisible ? n = 1 : n = 0;
		}
		return n;
	}

	//获取部件颜色-存在不同颜色时返回白色
	QColor mGeoInfoPanel::getPartsColor(const std::vector<int> partIds)
	{
		if (getCVM == nullptr)
		{
			return QColor();
		}

		QColor color;
		for (auto id : partIds)
		{
            mPartData* part = getCVM->getModelData()->getMeshData()->getPartData(id);
            if (part)
            {
                QColor partColor = part->getPartInnerColor();
                if (color.isValid() && color != partColor)
                {
                    return QColor(Qt::white);
                }
                color = partColor;
            }
		}
		
		return color;
	}

	//获取部件透明度-存在不同透明度时返回最大值65535
	int mGeoInfoPanel::getPartAlpha(const std::vector<int> partIds)
	{
		if (getCVM == nullptr)
		{
			return -1;
		}

		int alpha = -1;
		for (auto id : partIds)
		{
            mPartData* part = getCVM->getModelData()->getMeshData()->getPartData(id);
            if (part)
            {
                QColor partColor = part->getPartColor();
                QRgba64 rgba64 = partColor.rgba64();
                if (alpha >= 0 && alpha != rgba64.alpha())
                {
                    return 65535;
                }
                alpha = rgba64.alpha();
            }
		}
		return alpha;
	}


	//获取体积
	double mGeoInfoPanel::getPartsVolume(const std::vector<int> partIds)
	{
		if (getCVM == nullptr)
		{
			return 0.0;
		}

		double dVolume = 0;
		for (auto id : partIds)
		{
			double d = getGEOServer.geoCalculate().getVolume(id);
			dVolume += d;
		}
		return dVolume;
	}

	//获取包围盒
	QVector3D mGeoInfoPanel::getPartsBndBox(const std::vector<int> partIds)
	{
		if (getCVM == nullptr)
		{
			return QVector3D(0, 0, 0);
		}

		//包围盒
	    QPair<QVector3D, QVector3D> bndBox;
		for (auto id : partIds)
		{
			if (bndBox.first.isNull() && bndBox.second.isNull())
			{
				bndBox = getGEOServer.geoCalculate().getBoundingBox(id);
			}
			else
			{
				QPair<QVector3D, QVector3D> b = getGEOServer.geoCalculate().getBoundingBox(id);
				if (!b.first.isNull() && !b.second.isNull())
				{
					bndBox.first.setX(std::min(bndBox.first.x(), b.first.x()));
					bndBox.first.setY(std::min(bndBox.first.y(), b.first.y()));
					bndBox.first.setZ(std::min(bndBox.first.z(), b.first.z()));
					bndBox.second.setX(std::max(bndBox.second.x(), b.second.x()));
					bndBox.second.setY(std::max(bndBox.second.y(), b.second.y()));
					bndBox.second.setZ(std::max(bndBox.second.z(), b.second.z()));
				}
			}

		}

		if (bndBox.first.isNull() && bndBox.second.isNull())
		{
			bndBox.first = { 0,0,0 };
			bndBox.second = { 0,0,0 };
		}

		QVector3D Box(
			bndBox.second.x() - bndBox.first.x(),
			bndBox.second.y() - bndBox.first.y(),
			bndBox.second.z() - bndBox.first.z()
		);

		return Box;

	}

	//获取质心
	QVector3D mGeoInfoPanel::getPartsBarycenter(const std::vector<int> partIds)
	{
		if (getCVM == nullptr)
		{
			return QVector3D(0, 0, 0);
		}

		//质心
		QVector3D weightedCentroid(0, 0, 0);

		for (auto id : partIds)
		{
			weightedCentroid += getGEOServer.geoCalculate().getCentroid(id);
		}

		return weightedCentroid;
	}

	//获取单元数
	QPair<int, int> mGeoInfoPanel::getPartsElementNodeCount(const std::vector<int> partIds)
	{
		if (getCVM == nullptr)
		{
			return QPair<int, int>(0, 0);
		}

		QPair<int, int> pair;
		pair.first = 0;
		pair.second = 0;
		for (auto id : partIds)
		{
			QString partName = getCVM->getModelData()->getMeshData()->getPartNameByPartId(id);
			QPair<int, int> p = getCVM->getModelData()->getMeshData()->getNodeNumAndMeshNumInPart(partName);

			pair.first += p.first;
			pair.second += p.second;
		}
		
		return pair;
	}

	//获取截面属性
	std::vector<QString> mGeoInfoPanel::getPartsProperty(const std::vector<int> partIds)
	{
		if (getCVM == nullptr)
		{
			return std::vector<QString>();
		}

		bool isNull = false;
		std::vector<QString> propName;
		auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
		auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
		for (auto id : partIds)
		{
			int propertyId = mapRelation.getMapRelation<int>({ Physical_Part, id }, Physical_Property);
			mPropertyBase* propertyBase = propertyManager.getPropertyData(propertyId);
			if (propertyBase != nullptr)
			{
				QString propertyName = propertyBase->getPropertyName();
				if (std::find(propName.begin(), propName.end(), propertyName) == propName.end())
				{
					propName.push_back(propertyName);
					mapRelation.getMapRelation<int>({ Physical_Property, propertyBase->getPropertyId() }, Physical_Material);
				}
			}
			else if (propName.size() > 0 && isNull)
			{
				propName.clear();
				return propName;
			}
			else
			{
				isNull = true;
			}
		}

		if (propName.size() > 0 && isNull)
		{
			propName.clear();
		}

		return propName;
	}

	//获取所有截面属性的对应材料
	QMap<QString, QList<QString>> mGeoInfoPanel::getPartsPropertyMat()
	{
		if (getCVM == nullptr)
		{
			return QMap<QString, QList<QString>>();
		}

		QMap<QString, QList<QString>> propMatMap;
		auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
		auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
		std::vector<int> propIds = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property).getAllProperyID();
		for (auto pId : propIds)
		{
			mPropertyBase* propertyBase = propertyManager.getPropertyData(pId);
			if (propertyBase == nullptr)
				continue;

			QList<int> matIds{};
			if (propertyBase->getPropertyType() == LayeredShellProperty)
			{
				matIds = mapRelation.getMapRelation<QList<int>>({ Physical_Property, pId }, Physical_Material);
			}
			else
			{
				int matid = mapRelation.getMapRelation<int>({ Physical_Property, pId }, Physical_Material);
				matIds.append(matid);
			}
			
			QList<QString> matsName;
			for (auto mId : matIds)
			{
				QString matName = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMatNameByID(mId);
				matsName.append(matName);
			}
			
			QString propName = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property).getPropertyNameByID(pId);
			propMatMap[propName] = matsName;
		}
		return propMatMap;
	}

	void mGeoInfoPanel::getGeoInfoById(const std::vector<int> ids)
	{
		if (ids.size() <= 0 || getCVM == nullptr)
		{
			return;
		}

		selPartIds.clear();
		selPartIds.insert(selPartIds.end(), ids.begin(), ids.end());

		if (selPartIds.size() == 1 && selPartIds.at(0) <= 0)
		{
			showGeoParentNodeInfo();
		}
		else if (std::find(selPartIds.begin(), selPartIds.end(), 0) == selPartIds.end())
		{
            if (selPartIds.size() == 1)
            {
                if (!getCVM->getModelData()->getMeshData()->getPartData(selPartIds[0]))
                {
                    return;
                }
            }
			showGeoChildNodeInfo(selPartIds);
		}
	}

	void mGeoInfoPanel::updateInfo(const std::vector<int> ids)
	{
		if (ids.size() <= 0 || getCVM == nullptr)
		{
			return;
		}

		//可见性
		int nVisible = getPartsVisible(ids);
		switch (nVisible)
		{
		case -1:
			visibleCombo->setCurrentIndex(-1);
			break;
		case 0:
			visibleCombo->setCurrentText("是");
			break;
		case 1:
			visibleCombo->setCurrentText("否");
			break;

		default:
			break;
		}
	}

	//设置部件颜色
	void mGeoInfoPanel::slot_setPartColor()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		QColor mcolor = QColorDialog::getColor(Qt::white, this, "选择颜色");
        
		if (mcolor.isValid())
		{
			std::set<int> sId;
			for (auto id : selPartIds)
			{
				mPartData* part = getCVM->getModelData()->getMeshData()->getPartData(id);
                if (part)
                {
                    QRgba64 rgba64 = mcolor.rgba64();
                    QColor partColor = getCVM->getModelData()->getMeshData()->getPartData(id)->getPartInnerColor();
                    QRgba64 oldRgba64 = partColor.rgba64();

                    mcolor.setRgba64(qRgba64(rgba64.red(), rgba64.green(), rgba64.blue(), oldRgba64.alpha()));
                    part->setPartInnerColor(mcolor);
                    sId.insert(id);
                }
			}

			if (!sId.empty())
			{
				reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), selPartIds);
				getCVM->updatePartShowColor(PartColorType::partColor, selPartIds);
			}

			QString styleSheet =
				"QPushButton{"
				"background-color: " + mcolor.name() + ";"
				"color: " + mcolor.name() + ";"
				"}";
			colorBtn->setStyleSheet(styleSheet);
		}
	}

	//设置部件透明度
	void mGeoInfoPanel::setPartAlpha(const std::vector<int> partIds, double scale)
	{
		if (getCVM == nullptr)
		{
			return;
		}

		//获取部件颜色
		std::set<int> sId;
		for (auto id : partIds)
		{
            mPartData* part = getCVM->getModelData()->getMeshData()->getPartData(id);
            if (part)
            {
                int a =  255 * (1.00 - scale);
                part->setPartAlpha(a);
                sId.insert(id);
            }
		}

		if (!sId.empty())
		{
			reducePreWidgetSelectPartID(getCVM->getPreRenderWidget(), partIds);
            getCVM->updatePartShowColor(getCVM->getPartType(), partIds);
		}
	}

	//设置部件可见性
	void mGeoInfoPanel::setPartsVisible(const std::vector<int> ids)
	{
		if (visibleCombo->currentText().isEmpty() || getCVM == nullptr)
		{
			return;
		}

		QString partSetName = visibleCombo->currentText();

		int nVisible = visibleCombo->currentIndex();
		bool isVisible = false;
		nVisible == 0 ? isVisible = false : isVisible = true; //false是显示, true是隐藏

		set<int> hidePartIDs;
		for_each(ids.begin(), ids.end(), [&](int id) {hidePartIDs.insert(id); });
		getCVM->getModelData()->getMeshData()->setSelectPartMask(hidePartIDs, isVisible);
		MechPreData::clearPreWidgetSelectPartID(getCVM->getPreRenderWidget());
		UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		getCVM->updateRender();
		getCVM->clearPick();

	}

	//设置当所选部件的截面属性并显示截面属性对应的材料
	void mGeoInfoPanel::setPartsProperty(const std::vector<int> ids)
	{
		if (propertieCombo->currentText().isEmpty() || getCVM == nullptr)
		{
			return;
		}

		QString propSetName = propertieCombo->currentText();
		if (propSetName == " "|| propSetName == "\t")
		{
			return;
		}

		int propId = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property).getPropertyData(propSetName)->getPropertyId();

		auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
		for (auto id : ids)
		{
			mapRelation.appendMapRelation({ Physical_Part, id }, { Physical_Property, propId });
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Part, id);
		}
		
		//显示材料
		QString matName = "";
		for (auto name : propMatMap[propSetName])
		{
			matName += name + ";";
		}
		matName.chop(1);
		materialLine->setText(matName);

	}
}
