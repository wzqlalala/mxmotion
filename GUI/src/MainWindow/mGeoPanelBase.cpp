#include "mGeoPanelBase.h"
#include "UiMetrics.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
//#include "mGeoCommandSolidOfDragVecOCC.h" //【OCC摘除·S10/P0-1 终局2】死 include（零使用；几何面板基类）
#include "mGeoManager.h"
#include "PickCommonFunction.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
 

#include "mViewportModelManager.h"
#include "mViewportModel.h"

#include <QMenu>
#include "mUiMenu.h"
#include "mOtherFunction.h"

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】随死 include 下线
using namespace mxr;
using namespace MechPreData;
using namespace MxFunctions;
namespace MechMainWindow
{
	mGeoPanelBase::mGeoPanelBase(QWidget* parent)
		: mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		pickObjTable = new QTableWidget(this);
		pickObjTable->setMaximumHeight(100);
		pickObjTable->setMinimumHeight(80);
		pickObjTable->verticalHeader()->setHidden(true);
		pickObjTable->setColumnCount(1);
		QStringList horizontalHeader = { "几何ID" };
		pickObjTable->setHorizontalHeaderLabels(horizontalHeader);
		pickObjTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		pickObjTable->horizontalHeader()->setHighlightSections(false);
		pickObjTable->setFont(QFont(MxFunctions::UiMetrics::uiFontFamily(), 10));
		pickObjTable->verticalHeader()->setDefaultSectionSize(20);
		pickObjTable->setContextMenuPolicy(Qt::CustomContextMenu);//右键菜单
		pickObjTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		delAction = new QAction("删除");
		



		geoPickWidget = new GeoPickWidget(this);
		geoPickWidget->_featPick = None;

		connect(pickObjTable, &QTableWidget::customContextMenuRequested, this, &mGeoPanelBase::slot_contextMenuTableData);
	}

	mGeoPanelBase::~mGeoPanelBase()
	{

	}

	void mGeoPanelBase::hideAllFold()
	{
		geoPickWidget->_featPick = FeatureObj;

		for (int i = 0; i < m_foldWidgets.size(); i++)
		{
			_mainLayout->removeWidget(m_foldWidgets[i]);
			m_foldWidgets[i]->hide();
		}

		geoPickWidget->clearHighlight();
		geoPickWidget->_featPick = None;
	}

	vector<int> mGeoPanelBase::GetTableIdsUseVector()
	{
		vector<int> ids;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			//行已插入但单元格还没 setItem 时 item 为空，跳过避免空指针
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			strId = strId.mid(strId.lastIndexOf(":") + 1);
			int id = strId.toInt();
			ids.push_back(id);
		}
		return ids;
	}

	std::set<int> mGeoPanelBase::GetTableIdsUseSet()
	{
		std::set<int> ids;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			//行已插入但单元格还没 setItem 时 item 为空，跳过避免空指针
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			strId = strId.mid(strId.lastIndexOf(":") + 1);
			int id = strId.toInt();
			ids.insert(id);
		}
		return ids;
	}

	//创建水平分割线
	QFrame* mGeoPanelBase::createHorizontalLine(QWidget* parent)
	{
		QFrame* line = new QFrame(parent);
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken);
		line->setLineWidth(1);
		line->setMidLineWidth(0);
		return line;
	}

	void mGeoPanelBase::clearTableItems(QTableWidget* table)
	{
		for (int i = table->rowCount() - 1; i >= 0 ; i--)
		{
			table->removeRow(i);
		}
		getCVM->clearPick();
	}

	void mGeoPanelBase::slot_contextMenuTableData(const QPoint& pos)
	{
		mUiMenu menu(pickObjTable);
		menu.addActions({ delAction });

		QAction* selected = menu.exec(pickObjTable->viewport()->mapToGlobal(pos));
		if (selected == delAction)
		{
			
			QList<QTableWidgetItem*> items = pickObjTable->selectedItems();
			if (!items.isEmpty())
			{
				vector<int> ids;
				for (auto item : items)
				{
					ids.push_back(item->row());
				}

				sort(ids.begin(), ids.end());

				for (int i = ids.size() - 1; i >= 0; --i)
				{
					QString strId = pickObjTable->item(ids[i], 0)->text();
					QString strGeoType = strId;
					strId = strId.mid(strId.lastIndexOf(":") + 1);
					int id = strId.toInt();
					
					std::vector<int> Ids;
					Ids.push_back(id);
                    if (strGeoType.indexOf("几何点") == 0)
                    {
                        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoPoint, Ids);
                    }
                    else if (strGeoType.indexOf("几何线") == 0)
					{
						getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoEdge, Ids);
					}
					else if(strGeoType.indexOf("几何面") == 0)
					{
						getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoFace, Ids);
					}
					else if (strGeoType.indexOf("几何体") == 0)
					{
						getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoSolid, Ids);
					}
					pickObjTable->removeRow(ids[i]);
				}
				MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
				getCVM->getPreRenderWidget()->updatePaint();
			}
		}
	}

	//高亮列表中几何
	void mGeoPanelBase::setGeoHighlight(QTableWidget* table ,QString type)
	{
        if (type.contains("面"))
        {
            type = "几何面";
        }
        else if (type.contains("边"))
        {
            type = "几何线";
        }

		std::vector<int> Ids;
		for (int i = 0; i < table->rowCount(); ++i)
		{
			QString strId = table->item(i, 0)->text();
			if (strId.indexOf(type) == 0)
			{
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				Ids.push_back(id);
			}
			
		}
		if (Ids.size() > 0)
		{
			if (type == "几何面" )
			{
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, Ids);
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, Ids);
			}
			if (type == "几何线" )
			{
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, Ids);
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, Ids);
			}
			if (type == "几何体")
			{
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoSolid, Ids);
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, Ids);
			}
			MechPreData::UpdatePreWidgetHighlightWithoutRTRC(getCVM->getPreRenderWidget());
			getCVM->getPreRenderWidget()->updatePaint();
		}

	}

	//设置坐标
	void mGeoPanelBase::setCoord(QVector3D vec, QLineEdit* x, QLineEdit* y, QLineEdit* z)
	{
		if (vec[0] == 0 && vec[1] == 0 && vec[2] == 0)
		{
			x->setFocus();
			x->setText(MxFunctions::mNumberToString(vec[0]));
			y->setFocus();
			y->setText(MxFunctions::mNumberToString(vec[1]));
			z->setFocus();
			z->setText(MxFunctions::mNumberToString(vec[2]));
		}
		else if (vec.isNull())
		{
			x->setFocus();
			x->setText(0);
			y->setFocus();
			y->setText(0);
			z->setFocus();
			z->setText(0);
		}
		else
		{
			x->setFocus();
			x->setText(MxFunctions::mNumberToString(vec[0]));
			y->setFocus();
			y->setText(MxFunctions::mNumberToString(vec[1]));
			z->setFocus();
			z->setText(MxFunctions::mNumberToString(vec[2]));
		}
	}

	void mGeoPanelBase::reverseVecAction(QLineEdit* Vx, QLineEdit* Vy, QLineEdit* Vz)
	{
		double dVx, dVy, dVz;

		Vx->text().isEmpty() ? dVx = 0.0 : dVx = Vx->text().toDouble();
		Vy->text().isEmpty() ? dVy = 0.0 : dVy = Vy->text().toDouble();
		Vz->text().isEmpty() ? dVz = 0.0 : dVz = Vz->text().toDouble();

		QVector3D vec(dVx, dVy, dVz);
		vec = QVector3D(0, 0, 0) - vec;

		Vx->setFocus();
		Vx->setText(MxFunctions::mNumberToString(vec[0]));
		Vy->setFocus();
		Vy->setText(MxFunctions::mNumberToString(vec[1]));
		Vz->setFocus();
		Vz->setText(MxFunctions::mNumberToString(vec[2]));
	}

	QVector3D mGeoPanelBase::extractVectorFromLineEdit(QString text,const QVector3D& defaultValue ,bool* ok)
	{
		if (ok) *ok = false;

		// 1.清理前后空格
		text = text.trimmed();
		if (text.isEmpty()) {
			return defaultValue;
		}

		// 2. 使用正则表达式验证格式（允许数字、负号、小数点、科学计数法）
		QRegularExpression regExp("^\\s*([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*,\\s*([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*,\\s*([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*$");
		QRegularExpressionMatch match = regExp.match(text);

		if (match.hasMatch()) {
			bool xOk, yOk, zOk;
			float x = match.captured(1).toFloat(&xOk);
			float y = match.captured(3).toFloat(&yOk);
			float z = match.captured(5).toFloat(&zOk);

			if (xOk && yOk && zOk) {
				if (ok) *ok = true;
				return QVector3D(x, y, z);
			}
		}
		else {
			// 尝试简单的逗号分割方法作为备选
			QStringList parts = text.split(',', QString::SkipEmptyParts); //跳过空字符
			if (parts.size() == 3) {
				bool xOk, yOk, zOk;
				float x = parts[0].trimmed().toFloat(&xOk);
				float y = parts[1].trimmed().toFloat(&yOk);
				float z = parts[2].trimmed().toFloat(&zOk);

				if (xOk && yOk && zOk) {
					if (ok) *ok = true;
					return QVector3D(x, y, z);
				}
			}
		}

		return defaultValue;
	}

	void mGeoPanelBase::reverseVecAction(QLineEdit* LineEditVec)
	{
		QVector3D vec = extractVectorFromLineEdit(LineEditVec->text());
		vec = QVector3D(0, 0, 0) - vec;
		setVectorToLineEdit(LineEditVec, vec);
	}

	void mGeoPanelBase::setVectorToLineEdit(QLineEdit* lineEdit, const QVector3D& vector, int precision)
	{
		lineEdit->setText(QString("%1,%2,%3")
			.arg(vector.x(), 0, 'f', precision)
			.arg(vector.y(), 0, 'f', precision)
			.arg(vector.z(), 0, 'f', precision));
	}

	void FindParallelVectorsRobust(const float N[3], double vec1[3])
	{
		float nx = N[0], ny = N[1], nz = N[2];

		// 标准化法向量
		float length = sqrt(nx * nx + ny * ny + nz * nz);
		if (length < 1e-6f) {
			vec1[0] = vec1[1] = vec1[2] = 0.0f;
			return;
		}

		nx /= length;
		ny /= length;
		nz /= length;

		// 选择与法向量最不平行的轴,绝对值最小的分量对应的轴
		if (fabsf(nx) <= fabsf(ny) && fabsf(nx) <= fabsf(nz)) {
			// x分量最小，使用y轴或z轴
			vec1[0] = 0.0f;
			vec1[1] = 1.0f;
			vec1[2] = 0.0f;
		}
		else if (fabsf(ny) <= fabsf(nz)) {
			// y分量最小，使用x轴或z轴
			vec1[0] = 1.0f;
			vec1[1] = 0.0f;
			vec1[2] = 0.0f;
		}
		else {
			// z分量最小，使用x轴或y轴
			vec1[0] = 1.0f;
			vec1[1] = 0.0f;
			vec1[2] = 0.0f;
		}

		// 确保结果是正确的平行向量
		double temp[3] = { vec1[0], vec1[1], vec1[2] };
		vec1[0] = ny * temp[2] - nz * temp[1];
		vec1[1] = nz * temp[0] - nx * temp[2];
		vec1[2] = nx * temp[1] - ny * temp[0];

		// 标准化
		float vec1Length = sqrt(vec1[0] * vec1[0] + vec1[1] * vec1[1] + vec1[2] * vec1[2]);
		if (vec1Length > 1e-6f) {
			vec1[0] /= vec1Length;
			vec1[1] /= vec1Length;
			vec1[2] /= vec1Length;
		}
	}
	void mGeoPanelBase::getDir(double* md[3], int index, QLineEdit* lineEdit/*= nullptr*/)
	{
		for (int i = 0; i < 3; i++)
		{
			md[i] = new double[1];

		}

		double m_dir[3] = { 1,0,1 };
		if (0 == index)
		{
			m_dir[0] = 1;
			m_dir[1] = 0;
			m_dir[2] = 0;
		}
		if (1 == index)
		{
			m_dir[0] = 0;
			m_dir[1] = 1;
			m_dir[2] = 0;
		}
		if (2 == index)
		{
			m_dir[0] = 0;
			m_dir[1] = 0;
			m_dir[2] = 1;
		}
		if (3 == index)
		{
			glm::vec3 screenVec = getCVM->getPreRenderWidget()->getCameraController()->getCameraState().eye_dir;
			m_dir[0] = screenVec[0];
			m_dir[1] = screenVec[1];
			m_dir[2] = screenVec[2];
		}
		if (4 == index)
		{
			glm::vec3 screenVec = getCVM->getPreRenderWidget()->getCameraController()->getCameraState().eye_dir;
			float sVec[3];
			sVec[0] = screenVec[0];
			sVec[1] = screenVec[1];
			sVec[2] = screenVec[2];
			FindParallelVectorsRobust(sVec, m_dir);
		}
		if (5 == index && lineEdit != nullptr)
		{
			QVector3D customVec = extractVectorFromLineEdit(lineEdit->text());
			m_dir[0] = customVec[0];
			m_dir[1] = customVec[1];
			m_dir[2] = customVec[2];
		}

		*md[0] = m_dir[0];
		*md[1] = m_dir[1];
		*md[2] = m_dir[2];
	}


	//取消高亮
	void mGeoPanelBase::disHighlight(int &id, QString type)
	{
		if (id <= 0)
		{
            id = -1;
			return;
		}

		std::vector<int> Ids;
		Ids.push_back(id);
		if (type == "几何线")
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoEdge, Ids);
		}
		if (type == "几何点")
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoPoint, Ids);
		}
		if (type == "几何面")
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoFace, Ids);
		}
		if (type == "几何体")
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoSolid, Ids);
		}
       
		id = -1;
        MechPreData::UpdatePreWidgetHighlightWithoutRTRC(getCVM->getPreRenderWidget());
		getCVM->getPreRenderWidget()->updatePaint();
	}

    void mGeoPanelBase::disHighlightSymbol(SymbolSelectInfoData data)
    {
        std::vector<SymbolSelectInfoData>symbolDatas;
        symbolDatas.push_back(data);
        getCVM->getPreRenderWidget()->getSelectionContainer()->getSymbolSelectData()->UpdatePickData(SelectionOperate::REDUCE, SymbolSelectionInfoType::Symbol_All, symbolDatas);
        MechPreData::UpdatePreWidgetHighlightWithoutRTRC(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

	//高亮
	void mGeoPanelBase::Highlight(int id, QString type)
	{
		geoPickWidget->strPickType = type;

		if (id <= 0)
		{
			return;
		}
		std::vector<int> Ids;
		Ids.push_back(id);
		if (type == "几何线")
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, Ids);
		}
		if (type == "几何点")
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPoint, Ids);
		}
		if (type == "几何面")
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, Ids);
		}
        MechPreData::UpdatePreWidgetHighlightWithoutRTRC(getCVM->getPreRenderWidget());
		getCVM->getPreRenderWidget()->updatePaint();
	}

    void mGeoPanelBase::HighlightSymbol(int id, QString SymbolStr)
    {
        QString symbolName = QString("%1_%2").arg(SymbolStr).arg(id);
        SymbolSelectInfoData symbolData;
        symbolData.name = symbolName;
        std::vector<SymbolSelectInfoData>symbolDatas;
        symbolDatas.push_back(symbolData);
        getCVM->getPreRenderWidget()->getSelectionContainer()->getSymbolSelectData()->UpdatePickData(SelectionOperate::ADD, SymbolSelectionInfoType::Symbol_All, symbolDatas);
        MechPreData::UpdatePreWidgetHighlightWithoutRTRC(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

	void mGeoPanelBase::displayAfterInsertFolds(QList<int> index)
	{
		for (int i = 0; i < index.size(); ++i)
		{
			_mainLayout->insertWidget(i, m_foldWidgets[index[i]]);
			m_foldWidgets[index[i]]->show();
		}
	}

    void mGeoPanelBase::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mGeoPanelBase::slot_unitSystemResponse,Qt::UniqueConnection);
        //connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_AngleUnitChanged, this, &mGeoPanelBase::slot_angleUnitChanged, Qt::UniqueConnection);
        isConnectedUnit = true;
    }

    void mGeoPanelBase::disconnectUnitSystemResponse()
    {
        if (isConnectedUnit)
        {
            disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mGeoPanelBase::slot_unitSystemResponse);
            //disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_AngleUnitChanged, this, &mGeoPanelBase::slot_angleUnitChanged);
            isConnectedUnit = false;
        }
    }

    void mGeoPanelBase::connectGeoSignals()
    {
        //默认无全局信号，由各面板按需重写（拾取、单位制等）
    }

    void mGeoPanelBase::disconnectGeoSignals()
    {
        //默认无全局信号，由各面板按需重写（拾取、单位制等）
    }

    double mGeoPanelBase::getDefaultValue(const Unit& unit, double val)
    {
        //获取当前单位进行转换
        QVector<int> unitIndex = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio;
        if (unit == Unit::Length)
        {
            ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(unit, unitIndex, { 3,0,0,0 });
        }
        else if (unit == Unit::Angle)
        {
            ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(unit, unitIndex, { 3,0,0,0 });
        }

        return val * ratio;
    }

    void mGeoPanelBase::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | AngleUnit))) return;
        if(type & UnitSystem)
        {
            QString unit = UnitSystems[idx[0]][0];
            double ratio = getCVM->getModelData()->getUnitSystemManager()->getSwitchUnitSystemRatio(Unit::Length);
            for (auto& unitLabel : m_LengthLabels)
            {
                unitLabel.first->setText(unit);
                QString valuestr = unitLabel.second->text().trimmed();
                if (!valuestr.isEmpty())
                {
                    double value = valuestr.toDouble();
                    value *= ratio;
                    unitLabel.second->setExpression(MxFunctions::mNumberToString(value));
                }
            }
        }else if(type & AngleUnit)
        {
            QString unit = AngleSystem[idx[1]];
            if (unit == "degree")
            {
                unit = "°";
            }

            for (auto& unitLabel : m_AngleLabels)
            {
                unitLabel.first->setText(unit);
                QString valuestr = unitLabel.second->text().trimmed();
                if (!valuestr.isEmpty())
                {
                    double value = valuestr.toDouble();
                    getCVM->getModelData()->getUnitSystemManager()->respSwitchUnitSystem(Unit::Angle, value);
                    unitLabel.second->setExpression(MxFunctions::mNumberToString(value));
                }
            }
        }
    }

    void mGeoPanelBase::slot_angleUnitChanged(int idx)
    {
        QString unit = AngleSystem[idx];
        if (unit == "degree")
        {
            unit = "°";
        }
        
        for (auto& unitLabel : m_AngleLabels)
        {
            unitLabel.first->setText(unit);
            QString valuestr = unitLabel.second->text().trimmed();
            if (!valuestr.isEmpty())
            {
                double value = valuestr.toDouble();
                getCVM->getModelData()->getUnitSystemManager()->respSwitchUnitSystem(Unit::Angle, value);
                unitLabel.second->setExpression(MxFunctions::mNumberToString(value));
            }
        }
    }

    void mGeoPanelBase::updateLengthUnitWidget()
    {
        if (m_LengthLabels.size() <= 0)
        {
            return;
        }

        //获取当前单位进行转换
        QVector<int> unitIndex = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitSystems[unitIndex[0]][0];

        if (m_LengthLabels[0].first->text() == unit)
        {
            return;
        }

        //double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, { 2,0,0,0 }, unitIndex);
        for (auto& unitLabel : m_LengthLabels)
        {
            unitLabel.first->setText(unit);
            /*QString valuestr = unitLabel.second->text().trimmed();
            if (!valuestr.isEmpty())
            {
                double value = valuestr.toDouble();
                value *= ratio;
                unitLabel.second->setText(MxFunctions::mNumberToString(value));
            }*/
        }
    }

    void mGeoPanelBase::updateAngleUnitWidget()
    {
        if (m_AngleLabels.size() <= 0)
        {
            return;
        }

        //获取当前单位进行转换
        QVector<int> unitIndex = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = AngleSystem[unitIndex[1]];

        if (m_AngleLabels[0].first->text() == unit)
        {
            return;
        }

        if (unit == "degree")
        {
            unit = "°";
        }


        for (auto& unitLabel : m_AngleLabels)
        {
            unitLabel.first->setText(unit);
        }
    }

    

	GeoPickWidget::GeoPickWidget( QWidget* parent)
        : QWidget(parent)
	{
		strPickType = "";
		pickLabel = new PanelLabel("拾取", this);
		pickObjTypeCombo = new QComboBox(this);
		pickObjTypeCombo->addItems({ "几何面" , "几何线" });
		pickObjTypeCombo->setCurrentIndex(0);

		pickBtn = new QToolButton(this);
		pickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		

		pickLayout = new QHBoxLayout(this);
		pickLayout->setContentsMargins(0, 0, 0, 0);
		pickLayout->addWidget(pickLabel);
		pickLayout->addWidget(pickObjTypeCombo,1);
		pickLayout->addWidget(pickBtn);
		
	}
	GeoPickWidget::~GeoPickWidget()
	{

	}

	//设置拾取模式对应当前下拉选项
	void GeoPickWidget::setPickModeByCombo(QComboBox* combo)
	{
		//切换拾取模式 用发信号的方式
		//主动发送切换拾取模式(0://智能拾取;1:// 几何点 2://几何边 3:几何面;
		// 4:// 几何体;5:// 网格点;6:// 网格线;7:// 网格面; 8:// 网格单元

		QString partSetName = combo->currentText();
		strPickType = partSetName;
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
		if (partSetName.compare("几何面") == 0 || partSetName.contains("面")) {
            strPickType = "几何面";
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
		}
		if (partSetName.compare("几何线") == 0 || partSetName.contains("边")) {
            strPickType = "几何线";
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
		}
		if (partSetName.compare("几何点") == 0 ) {
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
		}
		if (partSetName.compare("几何体") == 0 ) {
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(4);
		}
	}
	
	void GeoPickWidget::clearHighlight()
	{
		if (!getCVM)
		{
			return;
		}
		if (getCVM->getPreRenderWidget())
		{
			//清掉带交点信息的补充拾取数据，否则硬点/孔功能遗留的点击点标记会在其它面板拾取时再次出现
			auto geoSelectData = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData();
			geoSelectData->UpdateMorePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, std::vector<GeoSelectInfoData>{});
			geoSelectData->UpdateMorePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, std::vector<GeoSelectInfoData>{});
		}
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        ClearPreWidgetSymbolPick(getCVM->getPreRenderWidget());
        MechPreData::UpdatePreWidgetHighlightWithoutRTRC(getCVM->getPreRenderWidget());
		getCVM->getPreRenderWidget()->updatePaint();
	}
}
