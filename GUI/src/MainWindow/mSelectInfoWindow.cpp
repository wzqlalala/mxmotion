#include "mSelectInfoWindow.h"
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"
#include "mGlobalSignals.h"
#include "mTitleBar.h"
#include "mMechGlobalSignals.h"
#include "mUnitEnum.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QMenu>
#include "mUiMenu.h"
#include <QApplication>
#include <QClipboard>
#include <qmath.h>
#include <QCheckBox>
#include <QDateTime>

//分段计时调试日志，排查完 grep "DEBUG-perf" 清理
#include <chrono>
#include <iostream>
#include <string>
inline void GUI_PERF_LOG(const char* tag, double ms, const std::string& extra)
{
	std::cout << "[DEBUG-perf] " << tag << ": total=" << ms << "ms " << extra << std::endl;
}

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"

//Post
#include "mPostDataBase.h"
#include "mPostModelData.h"
 
//Geo
#include "mGeoManager.h"
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
//#include <mGeometricFeatureAttribute.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
#include "mDataElement.h"
#include "mOtherFunction.h"
using namespace mxr;
using namespace MechElementData;
using namespace MechPreData;
using namespace MxFunctions;
using namespace PostData;
namespace MechMainWindow
{
	mSelectInfoWindow* mSelectInfoWindow::_instance = nullptr;
	mSelectInfoWindow* mSelectInfoWindow::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mSelectInfoWindow;
		}
		return _instance;
	}

	mSelectInfoWindow::mSelectInfoWindow(QWidget* parent) :QWidget(parent)
	{
		//设置允许的位置
		//this->setAllowedAreas(Qt::AllDockWidgetAreas);
		this->setStyleSheet("background-color: rgb(255,255,255)");

		_currentViewportPrivate = nullptr;

		QVBoxLayout* mainLayout = new QVBoxLayout(this);

		// 滚动条
		_scrollArea = new QScrollArea(this);
		_scrollArea->setWidgetResizable(true);
		_scrollArea->setFrameShape(QFrame::NoFrame);
		_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // 只要垂直条
		_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		//setWidget(_scrollArea);
		QVBoxLayout* lay = new QVBoxLayout();
		lay->setContentsMargins(0, 0, 0, 0);
		lay->addWidget(_scrollArea);

		_table = new QTableWidget(_scrollArea);
		_table->setItemDelegate(new CenterAlignDelegate(_table));
		_scrollArea->setWidget(_table);
		_table->setColumnCount(1);
		_table->setRowCount(1);
		_table->setEditTriggers(QTableWidget::EditTrigger::NoEditTriggers);
		_table->horizontalHeader()->setVisible(false);
		_table->setSelectionBehavior(QAbstractItemView::SelectItems);//设置整列选中
		_table->setContextMenuPolicy(Qt::CustomContextMenu);//右键菜单
		_copyAction = new QAction("复制");
		_flushedAction = new QAction("刷新");
		_reselectAction = new QAction("重新选择");


		//显示后处理控制复选框
		_controlsWidget = new QWidget(this);
		QHBoxLayout* controlsLayout = new QHBoxLayout(_controlsWidget);
		controlsLayout->setContentsMargins(0, 0, 0, 0);

		QCheckBox* chkOrder = new QCheckBox(tr("序号"), _controlsWidget);
		QCheckBox* chkID = new QCheckBox(tr("ID"), _controlsWidget);
		QCheckBox* chkNodeCoord = new QCheckBox(tr("节点坐标"), _controlsWidget);
		QCheckBox* chkElementNode = new QCheckBox(tr("单元包含的节点"), _controlsWidget);
		QCheckBox* chkResultValue = new QCheckBox(tr("结果值"), _controlsWidget);
		QCheckBox* chkPartName = new QCheckBox(tr("部件名称"), _controlsWidget);
		QCheckBox* chkCaseName = new QCheckBox(tr("工况名称"), _controlsWidget);

		chkOrder->setChecked(true);
		chkID->setChecked(true);
		chkNodeCoord->setChecked(true);
		chkElementNode->setChecked(true);
		chkResultValue->setChecked(true);
		chkPartName->setChecked(true);
		chkCaseName->setChecked(true);

		controlsLayout->addWidget(chkOrder);
		controlsLayout->addWidget(chkID);
		controlsLayout->addWidget(chkNodeCoord);
		controlsLayout->addWidget(chkElementNode);
		controlsLayout->addWidget(chkResultValue);
		controlsLayout->addWidget(chkPartName);
		controlsLayout->addWidget(chkCaseName);
		controlsLayout->addStretch();
		auto connectColCheck = [=](QCheckBox* chk, int col) {
			QObject::connect(chk, &QCheckBox::toggled,
				_table, [=](bool checked) {
				_table->setColumnHidden(col, !checked);
				_postCheckedState[col] = checked;
			});
		};
		connectColCheck(chkOrder, 0);
		connectColCheck(chkID, 1);
		connectColCheck(chkNodeCoord, 2);
		connectColCheck(chkElementNode, 3);
		connectColCheck(chkResultValue, 4);
		connectColCheck(chkPartName, 5);
		connectColCheck(chkCaseName, 6);

		for (int i = 0; i < 7; i++)
		{
			_postCheckedState[i] = true;
		}

		//布局
		mainLayout->addWidget(_controlsWidget);
		mainLayout->addLayout(lay);
		_controlsWidget->setHidden(true);


		//切换界面语言
		//connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
		connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mSelectInfoWindow::slot_subWindowActivated);
		connect(_table, &QTableWidget::customContextMenuRequested, this, &mSelectInfoWindow::slot_contextMenuTableData);
		connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::unitSystemChanged, this, &mSelectInfoWindow::slot_unitSystemChanged);
		//connect(_dialog, &TestDialog::ok, this, &mSelectInfoWindow::slot_highlightGeoData);
		//connect(_table, &QTableWidget::itemSelectionChanged, _dialog, &TestDialog::slot_showDialog);

		//面板构造可能晚于首个视口激活（布局恢复/默认布局都在之后），
		//此时不会再发出 currentViewportChanged，拾取连接永远不会建立，
		//导致新建第一个模型后直接拾取无响应、切一次视口才生效，此处补建初始绑定
		if (getCVM)
		{
			_currentViewportPrivate = VMManager->getCurrentViewportPrivate();
			if (getCVM->getPreRenderWidget())
				conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mSelectInfoWindow::slot_highlightGeoData);

			if (getCVM->getPostRenderWidget())
				conn_post = connect(getCVM->getPostRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mSelectInfoWindow::slot_highlightPostData);
		}

	}

	void mSelectInfoWindow::setTableHorizontalHeader(QStringList header)
	{
		for (int i = 0; i < header.size(); i++)
		{
			QLabel* label = new QLabel(_table);
			label->setAlignment(Qt::AlignCenter);
			label->setText(header.at(i));
			_table->setCellWidget(0, i, label);
		}
		//label->setText("<html>m<sup>2</sup></html>");
	}

	void mSelectInfoWindow::setTableVerticalHeader(QStringList header)
	{
		for (int i = 0; i < header.size(); i++)
		{
			_table->setItem(i + 1, 0, new QTableWidgetItem(header.at(i)));
		}
	}

	void mSelectInfoWindow::setTableRowQVector3DData(int row, int col, QVector3D data)
	{
		for (int i = 0; i < 3; i++)
		{
			_table->setItem(row, i + col, new QTableWidgetItem(QString("%1").arg(data[i])));
		}
	}

	void mSelectInfoWindow::addRow(QTableWidget* table, const QStringList& rowData)
	{
		int row = table->rowCount();
		table->insertRow(row);
		for (int col = 0; col < rowData.size(); ++col)
		{
			QTableWidgetItem* item = new QTableWidgetItem(rowData[col]);
			table->setItem(row, col, item);
		}
	}

	void mSelectInfoWindow::slot_highlightGeoData()
	{
		QTime time;
		time.start();
		_controlsWidget->setHidden(true);
		QString type = "无";//假设为几何点
		int num = 0;//假设数据个数为4
		vector<int> res;
		//获取拾取模式
		PickDataType pickDataType = getCVM->getCurrentPickDataType(num);
		auto meshData = getCVM->getModelData()->getMeshData();
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDataBase = getCVM->getModelData()->getGeoData();
		if (pickDataType > PickDataType::GeoSolid)
		{
			slot_highlightMeshData();
			return;
		}
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			type = "几何体";
			break;
		}
		case PickDataType::GeoFace:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			type = "几何面";
			break;
		}
		case PickDataType::GeoEdge:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			type = "几何线";
			break;
		}
		case PickDataType::GeoPoint:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			type = "几何点";
			break;
		}
		default:
			break;
		}
		num = res.size();
		//type = _dialog->_typeCombox->currentText();
		//num = _dialog->_numCombox->currentIndex() + 1;

		QString m = _length;
		QString m2 = QString("<html>%1<sup>2</sup></html>").arg(_length);
		QString m3 = QString("<html>%1<sup>3</sup></html>").arg(_length);
		QString statusInfo = "无";
		_table->clear();
		_table->setColumnCount(1);
		_table->setRowCount(1);
		if (num == 0)
		{
			updateStatusBarSelectInfo(statusInfo);
			return;
		}
		statusInfo = QString("%1 %2").arg(num).arg(type);
		QStringList verticalHeaderLabels;
		//几何点
		if (type == "几何点")
		{
			//QVector<QVector3D> points = QVector<QVector3D>{ QVector3D(0,0,0), QVector3D(1,0,1), QVector3D(1,1,1),QVector3D(2,1,1) };
			//auto points = getGEOServer.geoCalculate().CalculateVertexAttr(res);
			auto points = getGEOServer.geoCalculate().getVertexAttr(res);
			num = points.size();
			if (num == 0)
			{
				//属性查找失败(如缓存未计算)时直接返回,避免按0个点求形心产生NaN
				updateStatusBarSelectInfo(statusInfo);
				return;
			}
			if (num == 2)
			{
				verticalHeaderLabels = QStringList{ QString("距离(%1)").arg(m), QString("%1, 概要").arg(statusInfo)};
				_table->setRowCount(5);
				_table->setColumnCount(5);
				setTableHorizontalHeader(QStringList{ "实体", QString("距离(%1)").arg(m),QString("X(%1)").arg(m),QString("Y(%1)").arg(m), QString("Z(%1)").arg(m)});
				auto disvec = points[1].m_coor - points[0].m_coor;
				auto center = (points[1].m_coor + points[0].m_coor) / 2.0;
				auto distance = disvec.length();
				_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(distance * _scale)));
				setTableRowQVector3DData(1, 2, disvec * _scale);
				setTableRowQVector3DData(2, 2, center * _scale);
				setTableRowQVector3DData(3, 2, points[0].m_coor * _scale);
				setTableRowQVector3DData(4, 2, points[1].m_coor * _scale);
				statusInfo += QString(": 距离 = %1%2").arg(distance * _scale).arg(m);
			}
			else
			{
				verticalHeaderLabels = QStringList{ QString("%1, 概要").arg(statusInfo) };
				_table->setRowCount(num + 2);
				_table->setColumnCount(4);
				setTableHorizontalHeader(QStringList{ "实体",QString("X(%1)").arg(m),QString("Y(%1)").arg(m), QString("Z(%1)").arg(m) });
				if (num == 1)
				{
					QString location = QString(": 位置 =（%1, %2, %3）%4").arg(points[0].m_coor.x() * _scale).arg(points[0].m_coor.y() * _scale).arg(points[0].m_coor.z() * _scale).arg(m);
					statusInfo += location;
				}
				else if (num == 3)
				{
					// 计算角度
					QVector3D ba = points[0].m_coor - points[1].m_coor;  // 向量BA
					QVector3D bc = points[2].m_coor - points[1].m_coor;  // 向量BC
					float angleRad = acos(QVector3D::dotProduct(ba.normalized(), bc.normalized()));
					float angleDeg = qRadiansToDegrees(angleRad);
					statusInfo += QString(": 角度 = %1°").arg(angleDeg);
				}
				//计算形心
				auto centroid = QVector3D(0, 0, 0);
				for (int i = 0; i < num; i++)
				{
					centroid += points[i].m_coor;
					setTableRowQVector3DData(i + 2, 1, points[i].m_coor * _scale);
				}
				setTableRowQVector3DData(1, 1, centroid/ float(num) * _scale);
			}
			for (int i = 0; i < num; i++)
			{
				verticalHeaderLabels.append(QString("几何点%1").arg(i + 1));
			}
		}
		else if(type == "几何线")
		{
			//QVector<QPair<QVector3D, QVector3D>> lines = QVector<QPair<QVector3D, QVector3D>>{
			//											{QVector3D(0,0,0), QVector3D(1,0,0)},
			//											{QVector3D(1,0,0), QVector3D(1,1,0)},
			//											{QVector3D(1,1,0), QVector3D(0,1,0)},
			//											{QVector3D(0,1,0), QVector3D(0,0,0)}
			//};
			//auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
			auto lines = getGEOServer.geoCalculate().getEdgeAttr(res);
			num = lines.size();
			if (num == 0)
			{
				//属性查找失败(如缓存未计算)时直接返回,避免总长度除0产生NaN
				updateStatusBarSelectInfo(statusInfo);
				return;
			}
			if (num == 2)
			{
				verticalHeaderLabels = QStringList{
					QString("距离(%1)").arg(m),
					QString("%1, 概要").arg(statusInfo)
				};
				_table->setRowCount(5);
				_table->setColumnCount(6);
                QStringList headers({ "实体",
                    QString("质心之间的距离(%1)").arg(m),
                    "长度",
                    QString("质心X(%1)").arg(m),
                    QString("质心Y(%1)").arg(m),
                    QString("质心Z(%1)").arg(m)
                    });

				
                for (const auto& line : lines)
                {
                    if (line.m_radius > 0)
                    {
                        _table->setColumnCount(7);
                        headers.append("半径");
                        break;
                    }
                }

                setTableHorizontalHeader(headers);

				// 计算两条线的质心
				QVector3D centroid1 = lines[0].m_coor;
				QVector3D centroid2 = lines[1].m_coor;
				QVector3D vec1 = lines[0].m_dir;
				QVector3D vec2 = lines[1].m_dir;
				// 计算角度（弧度）
				float angleRad = acos(QVector3D::dotProduct(vec1.normalized(), vec2.normalized()));
				// 转换为角度
				float angleDeg = qRadiansToDegrees(angleRad);
				auto disvec = centroid2 - centroid1;
				auto center = (centroid2 + centroid1) / 2.0;
				// 计算质心间距离
				auto centroidDis = disvec.length();

				// 计算线段长度
				float length1 = lines[0].m_length;
				float length2 = lines[1].m_length;

				_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(center.length()* _scale)));
				//_table->setItem(0, 2, new QTableWidgetItem(QString("%1").arg(disvec.length())));
				_table->setItem(2, 2, new QTableWidgetItem(QString("%1").arg((length1 + length2)* _scale)));
				_table->setItem(3, 2, new QTableWidgetItem(QString("%1").arg(length1* _scale)));
				_table->setItem(4, 2, new QTableWidgetItem(QString("%1").arg(length2* _scale)));
				setTableRowQVector3DData(1, 3, disvec* _scale);
				setTableRowQVector3DData(2, 3, _scale * (centroid1 + centroid2) / 2.0f);
				setTableRowQVector3DData(3, 3, centroid1 * _scale);
				setTableRowQVector3DData(4, 3, centroid2 * _scale);

                if (lines[0].m_radius > 0)
                    _table->setItem(3, 6, new QTableWidgetItem(QString("%1").arg(lines[0].m_radius)));
                if (lines[1].m_radius > 0)
                    _table->setItem(4, 6, new QTableWidgetItem(QString("%1").arg(lines[1].m_radius)));


				statusInfo += QString(": 角度 = %1, 长度 = %3%4")
					.arg(angleDeg).arg(length1 * _scale + length2 * _scale).arg(m);
			}
			else
			{
				verticalHeaderLabels = QStringList{ QString("%1, 概要").arg(statusInfo) };
				_table->setRowCount(num + 2);
				_table->setColumnCount(5);
				setTableHorizontalHeader(QStringList{
					"实体",
					QString("长度(%1)").arg(m),
					QString("质心X(%1)").arg(m),
					QString("质心Y(%1)").arg(m),
					QString("质心Z(%1)").arg(m)
					});

				// 计算总长度和整体质心
				double totalLength = 0;
				QVector3D weightedCentroid(0, 0, 0);
				QVector3D centroidSum(0, 0, 0);

				for (int i = 0; i < num; i++)
				{
					double length = lines[i].m_length;
					QVector3D centroid = lines[i].m_coor;

					// 无效长度(NaN/非正值,结构体默认-1)只原样显示,不参与汇总,避免概要行被污染成NaN
					if (qIsFinite(length) && length > 0)
					{
						totalLength += length;
						weightedCentroid += centroid * static_cast<float>(length);
					}
					centroidSum += centroid;

					_table->setItem(i + 2, 1, new QTableWidgetItem(QString("%1").arg(length* _scale)));
					setTableRowQVector3DData(i + 2, 2, centroid* _scale);

                    if (lines[i].m_radius > 0 )
                    {
                        if (_table->columnCount() < 6)
                        {
                            _table->setColumnCount(6);
                            QLabel* label = new QLabel(_table);
                            label->setAlignment(Qt::AlignCenter);
                            label->setText(QString("半径"));
                            _table->setCellWidget(0, 5, label);
                        }
                        _table->setItem(i + 2, 5, new QTableWidgetItem(QString("%1").arg(lines[i].m_radius)));
                    }
                    
				}

				// 整体质心:长度加权;没有有效长度时退化为算术平均,避免除0产生NaN
				QVector3D overallCentroid(0, 0, 0);
				if (totalLength > 0)
					overallCentroid = weightedCentroid / static_cast<float>(totalLength);
				else if (num > 0)
					overallCentroid = centroidSum / num;
				_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(totalLength* _scale)));
				setTableRowQVector3DData(1, 2, overallCentroid* _scale);

				statusInfo += QString(": 长度 = %1%2").arg(totalLength * _scale).arg(m);
			}

			for (int i = 0; i < num; i++)
			{
				verticalHeaderLabels.append(QString("几何线%1").arg(i + 1));
			}
		}
		else if (type == "几何面")
		{
			// 假设有4个几何面，每个面的 {面积, 质心}
			//QVector<QPair<float, QVector3D>> faces = {
			//	{1.0f, QVector3D(0.5f, 0.5f, 0.0f)},  // 面1: 面积=1m², 质心=(0.5,0.5,0)
			//	{2.0f, QVector3D(1.5f, 0.5f, 0.0f)},  // 面2: 面积=2m², 质心=(1.5,0.5,0)
			//	{1.5f, QVector3D(1.0f, 1.5f, 0.0f)},  // 面3: 面积=1.5m², 质心=(1.0,1.5,0)
			//	{3.0f, QVector3D(2.0f, 2.0f, 0.0f)}   // 面4: 面积=3m², 质心=(2.0,2.0,0)
			//};
			//auto faces = getGEOServer.geoCalculate().CalculateFaceAttr(res);
			auto faces = getGEOServer.geoCalculate().getFaceAttr(res);
			num = faces.size();
			if (num == 0)
			{
				//属性查找失败(如缓存未计算)时直接返回,避免总面积除0产生NaN
				updateStatusBarSelectInfo(statusInfo);
				return;
			}
			if (num == 2)
			{
				verticalHeaderLabels = QStringList{
					QString("距离(%1)").arg(m),
					QString("%1, 概要").arg(statusInfo)
				};
				_table->setRowCount(5);
				_table->setColumnCount(6);

                QStringList headers({ "实体",
                    QString("质心之间的距离(%1)").arg(m),
                    QString("表面积(%1)").arg(m2),
                    QString("质心X(%1)").arg(m),
                    QString("质心Y(%1)").arg(m),
                    QString("质心Z(%1)").arg(m)
                    });
				
                for (const auto& face : faces)
                {
                    if (face.m_radius > 0)
                    {
                        _table->setColumnCount(7);
                        headers.append("半径");
                        break;
                    }
                }

                setTableHorizontalHeader(headers);

				// 取前两个面计算
				float area1 = faces[0].m_area;
				float area2 = faces[1].m_area;
				QVector3D centroid1 = faces[0].m_coor;
				QVector3D centroid2 = faces[1].m_coor;

				// 计算质心距离
				float centroidDistance = (centroid2 - centroid1).length();
				float totalArea = area1 + area2;

				// 填充表格数据
				_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(centroidDistance* _scale)));
				setTableRowQVector3DData(1, 3, (centroid2 - centroid1)* _scale);  // 距离向量
				_table->setItem(2, 2, new QTableWidgetItem(QString("%1").arg(totalArea* _scale* _scale)));
				_table->setItem(3, 2, new QTableWidgetItem(QString("%1").arg(area1* _scale* _scale)));
				setTableRowQVector3DData(3, 3, centroid1* _scale);

				_table->setItem(4, 2, new QTableWidgetItem(QString("%1").arg(area2* _scale* _scale)));
				setTableRowQVector3DData(4, 3, centroid2* _scale);

				// 计算整体质心（面积加权平均,总面积为0时退化为中点,避免NaN）
				QVector3D overallCentroid = (totalArea > 0)
					? (centroid1 * area1 + centroid2 * area2) / totalArea
					: (centroid1 + centroid2) / 2.0f;
				setTableRowQVector3DData(2, 3, overallCentroid* _scale);

				//计算面角度
				QVector3D vec1 = faces[0].m_dir;
				QVector3D vec2 = faces[1].m_dir;
				// 计算角度（弧度）
				float angleRad = acos(QVector3D::dotProduct(vec1.normalized(), vec2.normalized()));
				// 转换为角度
				float angleDeg = qRadiansToDegrees(angleRad);

				// 更新状态栏信息
				statusInfo += QString(": 角度 = %1, 面积 = %2%3")
					.arg(angleDeg).arg(totalArea * _scale * _scale).arg(m2);

                if (faces[0].m_radius > 0)
                    _table->setItem(3, 6, new QTableWidgetItem(QString("%1").arg(faces[0].m_radius)));
                if (faces[1].m_radius > 0)
                    _table->setItem(4, 6, new QTableWidgetItem(QString("%1").arg(faces[1].m_radius)));

			}
			else
			{
				verticalHeaderLabels = QStringList{ QString("%1, 概要").arg(statusInfo) };
				_table->setRowCount(num + 2);
				_table->setColumnCount(5);
				setTableHorizontalHeader(QStringList{
					"实体",
					QString("表面积(%1)").arg(m2),
					QString("质心X(%1)").arg(m),
					QString("质心Y(%1)").arg(m),
					QString("质心Z(%1)").arg(m)
					});

				// 计算总面积和加权质心
				double totalArea = 0.0;
				QVector3D weightedCentroid(0, 0, 0);
				QVector3D centroidSum(0, 0, 0);

				for (int i = 0; i < num; i++)
				{
					double area = faces[i].m_area;
					QVector3D centroid = faces[i].m_coor;

					// 无效面积(NaN/非正值,结构体默认-1)只原样显示,不参与汇总
					if (qIsFinite(area) && area > 0)
					{
						totalArea += area;
						weightedCentroid += centroid * static_cast<float>(area);
					}
					centroidSum += centroid;

					_table->setItem(i + 2, 1, new QTableWidgetItem(QString("%1").arg(area* _scale* _scale)));
					setTableRowQVector3DData(i + 2, 2, centroid* _scale);

                    if (faces[i].m_radius > 0)
                    {
                        if (_table->columnCount() < 6)
                        {
                            _table->setColumnCount(6);
                            QLabel* label = new QLabel(_table);
                            label->setAlignment(Qt::AlignCenter);
                            label->setText(QString("半径"));
                            _table->setCellWidget(0, 5, label);
                        }
                        _table->setItem(i + 2, 5, new QTableWidgetItem(QString("%1").arg(faces[i].m_radius)));
                    }
				}

				// 整体质心：面积加权；没有有效面积时退化为算术平均,避免除0产生NaN
				QVector3D overallCentroid(0, 0, 0);
				if (totalArea > 0)
					overallCentroid = weightedCentroid / static_cast<float>(totalArea);
				else if (num > 0)
					overallCentroid = centroidSum / num;
				_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(totalArea* _scale* _scale)));
				setTableRowQVector3DData(1, 2, overallCentroid* _scale);

				// 更新状态栏信息
				statusInfo += QString(": 面积 = %1%2").arg(totalArea * _scale * _scale).arg(m2);
			}

			// 添加行标签
			for (int i = 0; i < num; i++)
			{
				verticalHeaderLabels.append(QString("几何面%1").arg(i + 1));
			}
		}
		else if (type == "几何体")
		{
			// 定义几何体结构体
			struct GeometryBody {
				float volume;    // 体积(m³)
				int nodes;      // 节点数
				int elements;   // 单元数
				QVector3D centroid; // 质心坐标
			};
			double density = 7.890;

			// 示例数据（4个几何体）
			//QVector<GeometryBody> bodies = {
			//	{1.0f, 2.5f, 100, 50, QVector3D(0.5f, 0.5f, 0.5f)},
			//	{2.0f, 1.8f, 200, 80, QVector3D(1.5f, 0.5f, 0.5f)},
			//	{1.5f, 3.0f, 150, 60, QVector3D(1.0f, 1.5f, 0.5f)},
			//	{3.0f, 2.0f, 300, 120, QVector3D(2.0f, 2.0f, 1.0f)}
			//};
			//auto bodies = geoManager->CalculateSolidAttr(res);
			auto bodies = getGEOServer.geoCalculate().getSolidAttr(res);
			num = bodies.size();
			if (num == 0)
			{
				//属性查找失败(如缓存未计算)时直接返回,避免总体积除0产生NaN
				updateStatusBarSelectInfo(statusInfo);
				return;
			}
			// 公共列定义（除距离列外）
			QStringList commonColumns = {
				"实体",
				QString("体积(%1)").arg(m3),
				QString("质心X(%1)").arg(m),
				QString("质心Y(%1)").arg(m),
				QString("质心Z(%1)").arg(m),
				"节点数",
				"单元数"
			};
			std::vector<GeometryBody> geometryBodies;
			//按实际查到的属性个数构建,缓存查不到时bodies可能少于选中数,按res.size()访问bodies会越界
			geometryBodies.resize(bodies.size());
			for (int i = 0; i < (int)bodies.size(); i++)
			{
				geometryBodies[i].centroid = bodies[i].m_coor;
				geometryBodies[i].volume = bodies[i].m_volume;
				geometryBodies[i].elements = 0;
				geometryBodies[i].nodes = 0;
				//【OCC摘除·S9】旧栈 set 反查退役，改 GN 部件 ID
				const int partId = MGeoNeutralQuery::partIdByGeoId(
					(getCVM && getCVM->getModelData()) ? getCVM->getModelData()->getGeoManager() : nullptr, 3, bodies[i].id);
				auto partData = meshData->getPartData(partId);
				if (!partData)
				{
					continue;
				}
				if (partData->_partGeoToMeshdata.solidToMesh.contains(bodies[i].id))
				{
					geometryBodies[i].elements = partData->_partGeoToMeshdata.solidToMesh.value(bodies[i].id).size();
				}
				if (partData->_partGeoToMeshdata.solidToNode.contains(bodies[i].id))
				{
					geometryBodies[i].nodes = partData->_partGeoToMeshdata.solidToNode.value(bodies[i].id).size();
				}
			}

			if (num == 2)
			{
				// num=2时的特殊表格结构
				verticalHeaderLabels = QStringList{
					QString("距离(%1)").arg(m),
					QString("%1, 概要").arg(statusInfo),
					QString("几何体1"),
					QString("几何体2")
				};

				_table->setRowCount(5);
				_table->setColumnCount(commonColumns.size() + 1); // 增加距离列

				// 设置表头（距离列插入到第二列）
				QStringList headers = commonColumns;
				headers.insert(1, QString("质心之间的距离(%1)").arg(m));
				setTableHorizontalHeader(headers);

				// 取前两个几何体
				//const GeometryBody& body1 = bodies[0];
				//const GeometryBody& body2 = bodies[1];

				// 计算衍生数据
				float centroidDistance = (geometryBodies[0].centroid - geometryBodies[1].centroid).length();
				float totalVolume = geometryBodies[0].volume + geometryBodies[1].volume;
				//总体积为0时退化为质心中点,避免除0产生NaN
				QVector3D overallCentroid = (totalVolume > 0)
					? (geometryBodies[0].centroid * geometryBodies[0].volume + geometryBodies[1].centroid * geometryBodies[1].volume) / totalVolume
					: (geometryBodies[0].centroid + geometryBodies[1].centroid) / 2.0f;
				// 填充表格数据
				// 第一行：距离信息
				_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(centroidDistance* _scale)));
				setTableRowQVector3DData(1, 4, (geometryBodies[1].centroid - geometryBodies[0].centroid)* _scale);

				// 第二行：概要信息
				_table->setItem(2, 2, new QTableWidgetItem(QString("%1").arg(totalVolume* _scale* _scale* _scale)));
				setTableRowQVector3DData(2, 3, overallCentroid* _scale);
				_table->setItem(2, 6, new QTableWidgetItem(QString("%1").arg(geometryBodies[0].nodes + geometryBodies[1].nodes)));
				_table->setItem(2, 7, new QTableWidgetItem(QString("%1").arg(geometryBodies[0].elements + geometryBodies[1].elements)));

				// 第三行：几何体1
				_table->setItem(3, 2, new QTableWidgetItem(QString("%1").arg(geometryBodies[0].volume)));
				setTableRowQVector3DData(3, 3, geometryBodies[0].centroid* _scale);
				_table->setItem(3, 6, new QTableWidgetItem(QString("%1").arg(geometryBodies[0].nodes)));
				_table->setItem(3, 7, new QTableWidgetItem(QString("%1").arg(geometryBodies[0].elements)));

				// 第四行：几何体2
				_table->setItem(4, 2, new QTableWidgetItem(QString("%1").arg(geometryBodies[1].volume)));
				setTableRowQVector3DData(4, 3, geometryBodies[1].centroid* _scale);
				_table->setItem(4, 6, new QTableWidgetItem(QString("%1").arg(geometryBodies[1].nodes)));
				_table->setItem(4, 7, new QTableWidgetItem(QString("%1").arg(geometryBodies[1].elements)));

				// 更新状态栏
				statusInfo += QString(": 体积 = %1%2").arg(totalVolume).arg(m3);
			}
			else
			{
				// 其他情况的标准表格结构
				verticalHeaderLabels = QStringList{ QString("%1, 概要").arg(statusInfo) };
				for (int i = 0; i < num; i++) {
					verticalHeaderLabels.append(QString("几何体%1").arg(i + 1));
				}
				_table->setRowCount(num + 2);
				_table->setColumnCount(commonColumns.size());
				setTableHorizontalHeader(commonColumns);

				// 计算汇总数据
				double totalVolume = 0.0;
				float totalMass = 0.0f;
				QVector3D weightedCentroid(0, 0, 0);
				QVector3D centroidSum(0, 0, 0);
				int totalNodes = 0;
				int totalElements = 0;


				for (int i = 0; i < num; i++) {
					auto body = geometryBodies[i];

					// 无效体积(NaN/非正值,结构体默认-1)只原样显示,不参与汇总
					if (qIsFinite(body.volume) && body.volume > 0)
					{
						totalVolume += body.volume;
						weightedCentroid += body.centroid * body.volume;
					}
					centroidSum += body.centroid;
					totalNodes += body.nodes;
					totalElements += body.elements;

					// 填充各几何体数据
					_table->setItem(i + 2, 1, new QTableWidgetItem(QString("%1").arg(body.volume* _scale* _scale* _scale)));
					setTableRowQVector3DData(i + 2, 2, body.centroid* _scale);
					_table->setItem(i + 2, 5, new QTableWidgetItem(QString("%1").arg(body.nodes)));
					_table->setItem(i + 2, 6, new QTableWidgetItem(QString("%1").arg(body.elements)));
				}
				// 整体质心:体积加权;没有有效体积时退化为算术平均,避免除0产生NaN
				QVector3D overallCentroid(0, 0, 0);
				if (totalVolume > 0)
					overallCentroid = weightedCentroid / static_cast<float>(totalVolume);
				else if (num > 0)
					overallCentroid = centroidSum / num;

				// 第一行：概要信息
				_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(totalVolume* _scale* _scale* _scale)));
				setTableRowQVector3DData(1, 2, overallCentroid* _scale);
				_table->setItem(1, 5, new QTableWidgetItem(QString("%1").arg(totalNodes)));
				_table->setItem(1, 6, new QTableWidgetItem(QString("%1").arg(totalElements)));

				// 更新状态栏
				statusInfo += QString(": 体积 = %1%2").arg(totalVolume * _scale * _scale * _scale).arg(m3);
			}
		}
		setTableVerticalHeader(verticalHeaderLabels);
		updateStatusBarSelectInfo(statusInfo);
		_table->resizeColumnsToContents();

		qDebug() << "拾取信息显示耗时" << time.elapsed();
	}

	void mSelectInfoWindow::slot_highlightMeshData()
	{
		_controlsWidget->setHidden(true);
		QString type = "节点";//假设为节点
		int num = 0;//假设数据个数为4
		std::vector<MeshSelectInfoData> res;
		//获取拾取模式
		PickDataType pickDataType = getCVM->getCurrentPickDataType(num);
		if (pickDataType < PickDataType::Node || pickDataType > PickDataType::Element)
		{
			slot_highlightPartData();
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		auto elementData = getCVM->getModelData()->getElementData();
		//获取几何点
		switch (pickDataType)
		{
		case PickDataType::Node:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
			type = "节点";
			break;
		}
		case PickDataType::ElementEdge:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);
			type = "单元线";
			break;
		}
		case PickDataType::ElementFace:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
			type = "单元面";
			break;
		}
		case PickDataType::Element:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
			type = "单元";
			break;
		}
		default:
			break;
		}
		num = res.size();
		//type = _dialog->_typeCombox->currentText();
		//num = _dialog->_numCombox->currentIndex() + 1;

		auto getNodeVertexByID = [&](mMeshData *meshData, int id, QVector3D & vertex3D) {
			if (!meshData->_nodedata.nodeidmap.contains(id))
			{
				vertex3D = QVector3D();
				return;
			}
			double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap.value(id) * 3;

			vertex3D = QVector3D(vertex[0], vertex[1], vertex[2]);
			};

		QString m = _length;
		QString m2 = QString("<html>%1<sup>2</sup></html>").arg(_length);
		QString m3 = QString("<html>%1<sup>3</sup></html>").arg(_length);
		QString statusInfo = "无";
		_table->clear();
		_table->setColumnCount(1);
		_table->setRowCount(1);
		if (num == 0)
		{
			updateStatusBarSelectInfo(statusInfo);
			return;
		}
		statusInfo = QString("%1 %2").arg(num).arg(type);
		QStringList verticalHeaderLabels;
		if (type == "节点")
		{
			//QVector<QPair<int, QVector3D>> nodes = {
			//	{1, QVector3D(0.5f, 0.5f, 0.0f)},  // 节点1: ID=1, 位置=(0.5,0.5,0)
			//	{2, QVector3D(1.5f, 0.5f, 0.0f)},  // 节点2: ID=2, 位置=(1.5,0.5,0)
			//	{3, QVector3D(1.0f, 1.5f, 0.0f)},  // 节点3: ID=3, 位置=(1.0,1.5,0)
			//	{4, QVector3D(2.0f, 2.0f, 0.0f)}   // 节点4: ID=4, 位置=(2.0,2.0,0)
			//};
			QVector<QPair<int, QVector3D>> nodes;
			for (auto r : res)
			{
				QVector3D vertex;
				getNodeVertexByID(meshData, r.data_id.element_id, vertex);
				nodes.append({ r.data_id.element_id, vertex });
			}
			if (num == 2)
			{
				verticalHeaderLabels = QStringList{ QString("距离(%1)").arg(m), QString("%1, 概要").arg(statusInfo) };
				_table->setRowCount(5);
				_table->setColumnCount(6);
				setTableHorizontalHeader(QStringList{ "实体", QString("距离(%1)").arg(m),QString("X(%1)").arg(m),QString("Y(%1)").arg(m), QString("Z(%1)").arg(m),"节点ID" });
				auto disvec = nodes[1].second - nodes[0].second;
				auto center = (nodes[1].second + nodes[0].second) / 2.0;
				auto distance = disvec.length();
				_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(distance * _scale)));
				setTableRowQVector3DData(1, 2, disvec * _scale);
				setTableRowQVector3DData(2, 2, center * _scale);
				setTableRowQVector3DData(3, 2, nodes[0].second * _scale);
				setTableRowQVector3DData(4, 2, nodes[1].second * _scale);
				_table->setItem(3, 5, new QTableWidgetItem(QString("%1").arg(nodes[0].first)));
				_table->setItem(4, 5, new QTableWidgetItem(QString("%1").arg(nodes[1].first)));
				statusInfo += QString(": 距离 = %1%2").arg(distance * _scale).arg(m);
			}
			else
			{
				verticalHeaderLabels = QStringList{ QString("%1, 概要").arg(statusInfo) };
				num = std::min(num, 5000);//限制显示范围
				_table->setRowCount(num + 2);
				_table->setColumnCount(5);
				setTableHorizontalHeader(QStringList{ "实体",QString("X(%1)").arg(m),QString("Y(%1)").arg(m), QString("Z(%1)").arg(m),"节点ID" });
				if (num == 1)
				{
					QString location = QString("：ID = %1, 位置 =（%2, %3, %4）%5").arg(nodes[0].first).arg(nodes[0].second.x() * _scale).arg(nodes[0].second.y() * _scale).arg(nodes[0].second.z() * _scale).arg(m);
					statusInfo += location;
				}
				else if (num == 3)
				{
					// 计算角度
					QVector3D ba = nodes[0].second - nodes[1].second;  // 向量BA
					QVector3D bc = nodes[2].second - nodes[1].second;  // 向量BC
					float angleRad = acos(QVector3D::dotProduct(ba.normalized(), bc.normalized()));
					float angleDeg = qRadiansToDegrees(angleRad);
					statusInfo += QString(": 角度 = %1°").arg(angleDeg);
				}
				//计算形心
				auto centroid = QVector3D(0, 0, 0);
				for (int i = 0; i < num; i++)
				{
					centroid += nodes[i].second;
					setTableRowQVector3DData(i + 2, 1, nodes[i].second* _scale);
					_table->setItem(i + 2, 4, new QTableWidgetItem(QString("%1").arg(nodes[i].first)));
				}
				setTableRowQVector3DData(1, 1, _scale * centroid / float(num));
			}
			for (int i = 0; i < num; i++)
			{
				verticalHeaderLabels.append(QString("节点%1").arg(i + 1));
			}
		}
		else if (type == "单元线")
		{
			// 定义单元面结构体
			struct ElementLine {
				int nodeID1;
				int nodeID2;
				int eleID;      // 单元ID
				int order; //单元面在单元的第几个线
				ElementType type;//单元类型
			};

			// 示例数据（4个单元面）
			//QVector<ElementFace> elefaces = {
			//	{1, 1, D3T4},
			//	{1, 2, D3T4},
			//	{1, 3, D3T4},
			//	{1, 4, D3T4}
			//};
			QVector<ElementLine> eleLines;
			eleLines.reserve(num);
			for (auto r : res)
			{
				//meshData->getModelAllEleNum();
				//meshData->getMeshIdAndOrderByeleFaceId(r.result_id, r.result_id_2, meshID, order);
				int nodeId1 = r.data_id.element_id;
				int nodeId2 = r.data_index.line_index;
				mPartData* part = meshData->getPartData(r.part_id);
				if (part == nullptr)
					continue;

				auto eles1 = part->_parteledata.nodeelements.value(nodeId1);
				auto eles2 = part->_parteledata.nodeelements.value(nodeId2);
                QSet<int> eleSet1;
                for (int e : eles1)
                {
                    eleSet1.insert(e);
                }
                QSet<int> eleSet2;
                for (int e : eles2)
                {
                    eleSet2.insert(e);
                }
                QSet<int> intersectEles = eleSet1 & eleSet2;
				for (auto ele : intersectEles)
				{
					auto lines = meshData->getMeshEdgeLineByMeshID(r.part_id, ele);
					std::pair<int, int> line{ nodeId1 , nodeId2 };

					auto it = std::find(lines.begin(), lines.end(), line);
					int index = 0;
					if (it != lines.end()) {
						index = std::distance(lines.begin(), it) + 1;		
					}
					ElementType elementType = elementData->getElementTypeByMeshID(ele);
					if (elementType == NoneElement)
					{
						elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, meshData->getMeshTypeByEleId(ele));
					}
					eleLines.append({ r.data_id.node_id, r.data_index.line_index, ele, index, elementType });
				}
			}

			// 公共列定义（除距离列外）
			QStringList commonColumns = {
				"实体",
				"节点ID1",
				"节点ID2",
				"单元ID",
				"线ID",
				"单元类型"
			};
			num = std::min(num, 5000);//限制显示范围
			_table->setRowCount(num + 1);
			_table->setColumnCount(commonColumns.size());
			setTableHorizontalHeader(commonColumns);

			for (int i = 0; i < num; i++) {
				const ElementLine& eleline = eleLines[i];
				verticalHeaderLabels.append(QString("单元线%1").arg(i + 1));

				// 填充各几何体数据
				_table->setItem(i + 1, 1, new QTableWidgetItem(QString("%1").arg(eleline.nodeID1)));
				_table->setItem(i + 1, 2, new QTableWidgetItem(QString("%1").arg(eleline.nodeID2)));
				_table->setItem(i + 1, 3, new QTableWidgetItem(QString("%1").arg(eleline.eleID)));
				_table->setItem(i + 1, 4, new QTableWidgetItem(QString("%1").arg(eleline.order)));
				_table->setItem(i + 1, 5, new QTableWidgetItem(QString("%1").arg(mElementFunction::getElementString(eleline.type))));
			}
			if (num == 1)
			{
				// 更新状态栏
				statusInfo += QString(": 单元ID = %1，线索引%2").arg(eleLines[0].eleID).arg(eleLines[0].order);
			}

		}
		else if (type == "单元面")
		{
			// 定义单元面结构体
			struct ElementFace {
				int eleID;      // 单元ID
				int order; //单元面在单元的第几个面
				ElementType type;//单元类型
			};

			// 示例数据（4个单元面）
			//QVector<ElementFace> elefaces = {
			//	{1, 1, D3T4},
			//	{1, 2, D3T4},
			//	{1, 3, D3T4},
			//	{1, 4, D3T4}
			//};
			QVector<ElementFace> elefaces;
			for (auto r : res)
			{
				int meshID = r.data_id.element_id;
				ElementType elementType = elementData->getElementTypeByMeshID(meshID);
				if (elementType == NoneElement)
				{
					elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, meshData->getMeshTypeByEleId(meshID));
				}
				elefaces.append({ meshID,  r.data_index.face_index, elementType });
			}

			// 公共列定义（除距离列外）
			QStringList commonColumns = {
				"实体",
				"单元ID",
				"面ID",
				"单元类型"
			};
			num = std::min(num, 5000);//限制显示范围
			_table->setRowCount(num + 1);
			_table->setColumnCount(commonColumns.size());
			setTableHorizontalHeader(commonColumns);

			for (int i = 0; i < num; i++) {
				const ElementFace& eleface = elefaces[i];
				verticalHeaderLabels.append(QString("单元面%1").arg(i + 1));

				// 填充各几何体数据
				_table->setItem(i + 1, 1, new QTableWidgetItem(QString("%1").arg(eleface.eleID)));
				_table->setItem(i + 1, 2, new QTableWidgetItem(QString("%1").arg(eleface.order)));
				_table->setItem(i + 1, 3, new QTableWidgetItem(QString("%1").arg(mElementFunction::getElementString(eleface.type))));
			}
			if (num == 1)
			{
				// 更新状态栏
				statusInfo += QString(": 单元ID = %1，面索引%2").arg(elefaces[0].eleID).arg(elefaces[0].order);
			}

			}
		else if (type == "单元")
		{
			// 定义单元结构体
			struct Element {
				int eleID;      // 单元ID
				ElementType type;//单元类型
			};

			// 示例数据（4个单元面）
			//QVector<Element> elefaces = {
			//	{ 1, D3T4},
			//	{ 2, D3T4},
			//	{ 3, D3T4},
			//	{ 4, D3T4}
			//};
			QVector<Element> elefaces;
			for (auto r : res)
			{
				int meshID = r.data_id.element_id;
				ElementType elementType = elementData->getElementTypeByMeshID(meshID);
				if (elementType == NoneElement)
				{
					elementType = mElementFunction::getDefaultElementType(ImplicitAnalysis, meshData->getMeshTypeByEleId(meshID));
				}
				elefaces.append({ meshID, elementType });
			}

			// 公共列定义（除距离列外）
			QStringList commonColumns = {
				"实体",
				"单元ID",
				"单元类型"
			};
			num = std::min(num, 5000);//限制显示范围
			_table->setRowCount(num + 1);
			_table->setColumnCount(commonColumns.size());
			setTableHorizontalHeader(commonColumns);

			for (int i = 0; i < num; i++) {
				const Element& ele = elefaces[i];
				verticalHeaderLabels.append(QString("单元%1").arg(i + 1));

				// 填充各几何体数据
				_table->setItem(i + 1, 1, new QTableWidgetItem(QString("%1").arg(ele.eleID)));
				_table->setItem(i + 1, 2, new QTableWidgetItem(QString("%1").arg(mElementFunction::getElementString(ele.type))));
			}
			if (num == 1)
			{
				// 更新状态栏
				statusInfo += QString(": 单元ID = %1").arg(elefaces[0].eleID);
			}
			}
			setTableVerticalHeader(verticalHeaderLabels);
			updateStatusBarSelectInfo(statusInfo);
			_table->resizeColumnsToContents();
	}

	void mSelectInfoWindow::slot_highlightPartData()
	{
		_controlsWidget->setHidden(true);
		QString type = "部件";
		int num = 0;
		vector<int> res;
		//获取拾取模式
		PickDataType pickDataType = getCVM->getCurrentPickDataType(num);
		//mGeoModelData1 *geoModelData = getCVM->getModelData()->get
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		if (pickDataType != PickDataType::Part)
		{
			return;
		}

		auto meshData = getCVM->getModelData()->getMeshData();
		res = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());

		// 定义几何体结构体
		struct GeometryBody {
			double volume = 0;    // 体积(m³)
			int nodes = 0;      // 节点数
			int elements = 0;   // 单元数
			QVector3D centroid; // 质心坐标
		};
		std::vector<GeometryBody> parts;
		for (auto partId : res)
		{
			auto partData = meshData->getPartData(partId);
			if (partData == nullptr)
			{
				continue;
			}
			GeometryBody part;
			int partType = partData->getPartType();
			if (partType == 2)//网格部件
			{
				part.elements = partData->_parteledata.currentelenum;
				part.nodes = partData->_parteledata.nodeelements.size();
				//meshData->getPartWeight(partId, part.volume, part.centroid);
				//part.centroid = part.centroid * part.volume;
			}
			else
			{
				//按实体体积加权计算质心;只含面/线的部件(如草图直线)没有实体,
				//体积为0时直接除会得到NaN质心,需逐级退化到面面积/边长度加权
				auto solids = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 3);
				auto solidsRes = getGEOServer.geoCalculate().getSolidAttr(solids);
				for (auto solid : solidsRes)
				{
					if (qIsFinite(solid.m_volume) && solid.m_volume > 0)
					{
						part.volume += solid.m_volume;
						part.centroid += solid.m_coor * static_cast<float>(solid.m_volume);
					}
				}

				if (part.volume > 0)
				{
					part.centroid = part.centroid / static_cast<float>(part.volume);
				}
				else
				{
					part.centroid = QVector3D(0, 0, 0);
					double totalArea = 0;
					auto faces = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, partId);
					//【OCC摘除·S9】mGeoInfoCalculate 已退役（恒 null）——改服务端属性查询
			auto facesRes = getGEOServer.geoCalculate().getFaceAttr(faces);
					for (auto face : facesRes)
					{
						if (qIsFinite(face.m_area) && face.m_area > 0)
						{
							totalArea += face.m_area;
							part.centroid += face.m_coor * static_cast<float>(face.m_area);
						}
					}

					if (totalArea > 0)
					{
						part.centroid = part.centroid / static_cast<float>(totalArea);
					}
					else
					{
						part.centroid = QVector3D(0, 0, 0);
						double totalLength = 0;
						auto edges = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, partId);
						//【OCC摘除·S9】mGeoInfoCalculate 已退役（恒 null）——改服务端属性查询
			auto edgesRes = getGEOServer.geoCalculate().getEdgeAttr(edges);
						for (auto edge : edgesRes)
						{
							if (qIsFinite(edge.m_length) && edge.m_length > 0)
							{
								totalLength += edge.m_length;
								part.centroid += edge.m_coor * static_cast<float>(edge.m_length);
							}
						}

						if (totalLength > 0)
						{
							part.centroid = part.centroid / static_cast<float>(totalLength);
						}
					}
				}
				if (partType == 3)
				{
					part.elements = partData->_parteledata.currentelenum;
					part.nodes = partData->_parteledata.nodeelements.size();
				}
				else
				{
					part.elements = 0;
					part.nodes = 0;
				}
			}
			parts.push_back(part);
		}
		num = parts.size();


		QString m = _length;
		QString m2 = QString("<html>%1<sup>2</sup></html>").arg(_length);
		QString m3 = QString("<html>%1<sup>3</sup></html>").arg(_length);
		QString statusInfo = "无";
		_table->clear();
		_table->setColumnCount(1);
		_table->setRowCount(1);
		if (num == 0)
		{
			updateStatusBarSelectInfo(statusInfo);
			return;
		}
		statusInfo = QString("%1 %2").arg(num).arg(type);
		QStringList verticalHeaderLabels;

		// 公共列定义（除距离列外）
		QStringList commonColumns = {
			"部件",
			QString("体积(%1)").arg(m3),
			QString("质心X(%1)").arg(m),
			QString("质心Y(%1)").arg(m),
			QString("质心Z(%1)").arg(m),
			"节点数",
			"单元数"
		};

		if (num == 2)
		{
			// num=2时的特殊表格结构
			verticalHeaderLabels = QStringList{
				QString("距离(%1)").arg(m),
				QString("%1, 概要").arg(statusInfo),
				QString("部件1"),
				QString("部件2")
			};

			_table->setRowCount(5);
			_table->setColumnCount(commonColumns.size() + 1); // 增加距离列

			// 设置表头（距离列插入到第二列）
			QStringList headers = commonColumns;
			headers.insert(1, QString("质心之间的距离(%1)").arg(m));
			setTableHorizontalHeader(headers);

			// 取前两个几何体
			//const GeometryBody& body1 = bodies[0];
			//const GeometryBody& body2 = bodies[1];

			// 计算衍生数据
			float centroidDistance = (parts[0].centroid - parts[1].centroid).length();
			float totalVolume = parts[0].volume + parts[1].volume;
			//总体积为0(如只含线的部件)时退化为质心中点,避免除0产生NaN
			QVector3D overallCentroid = (totalVolume > 0)
				? (parts[0].centroid * parts[0].volume + parts[1].centroid * parts[1].volume) / totalVolume
				: (parts[0].centroid + parts[1].centroid) / 2.0f;
			// 填充表格数据
			// 第一行：距离信息
			_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(centroidDistance* _scale)));
			setTableRowQVector3DData(1, 3, (parts[1].centroid - parts[0].centroid) * _scale);

			// 第二行：概要信息
			_table->setItem(2, 2, new QTableWidgetItem(QString("%1").arg(totalVolume* _scale* _scale* _scale)));
			setTableRowQVector3DData(2, 3, overallCentroid* _scale);
			_table->setItem(2, 6, new QTableWidgetItem(QString("%1").arg(parts[0].nodes + parts[1].nodes)));
			_table->setItem(2, 7, new QTableWidgetItem(QString("%1").arg(parts[0].elements + parts[1].elements)));

			// 第三行：几何体1
			_table->setItem(3, 2, new QTableWidgetItem(QString("%1").arg(parts[0].volume)));
			setTableRowQVector3DData(3, 3, parts[0].centroid* _scale);
			_table->setItem(3, 6, new QTableWidgetItem(QString("%1").arg(parts[0].nodes)));
			_table->setItem(3, 7, new QTableWidgetItem(QString("%1").arg(parts[0].elements)));

			// 第四行：几何体2
			_table->setItem(4, 2, new QTableWidgetItem(QString("%1").arg(parts[1].volume)));
			setTableRowQVector3DData(4, 3, parts[1].centroid* _scale);
			_table->setItem(4, 6, new QTableWidgetItem(QString("%1").arg(parts[1].nodes)));
			_table->setItem(4, 7, new QTableWidgetItem(QString("%1").arg(parts[1].elements)));

			// 更新状态栏
			statusInfo += QString(": 体积 = %1%2").arg(totalVolume).arg(m3);
		}
		else
		{
			// 其他情况的标准表格结构
			verticalHeaderLabels = QStringList{ QString("%1, 概要").arg(statusInfo) };
			for (int i = 0; i < num; i++) {
				verticalHeaderLabels.append(QString("部件%1").arg(i + 1));
			}
			_table->setRowCount(num + 2);
			_table->setColumnCount(commonColumns.size());
			setTableHorizontalHeader(commonColumns);

			// 计算汇总数据
			double totalVolume = 0.0;
			double totalMass = 0.0;
			QVector3D weightedCentroid(0, 0, 0);
			QVector3D centroidSum(0, 0, 0);
			int totalNodes = 0;
			int totalElements = 0;


			for (int i = 0; i < num; i++) {
				auto body = parts[i];

				//体积为0或无效的部件(如只含线的草图部件)不参与加权,只做算术平均
				if (qIsFinite(body.volume) && body.volume > 0)
				{
					totalVolume += body.volume;
					weightedCentroid += body.centroid * static_cast<float>(body.volume);
				}
				centroidSum += body.centroid;
				totalNodes += body.nodes;
				totalElements += body.elements;

				// 填充各几何体数据
				_table->setItem(i + 2, 1, new QTableWidgetItem(QString("%1").arg(body.volume * _scale * _scale * _scale)));
				setTableRowQVector3DData(i + 2, 2, body.centroid * _scale);
				_table->setItem(i + 2, 5, new QTableWidgetItem(QString("%1").arg(body.nodes)));
				_table->setItem(i + 2, 6, new QTableWidgetItem(QString("%1").arg(body.elements)));
			}
			// 整体质心:体积加权;没有有效体积时退化为算术平均,避免除0产生NaN
			QVector3D overallCentroid(0, 0, 0);
			if (totalVolume > 0)
				overallCentroid = weightedCentroid / static_cast<float>(totalVolume);
			else if (num > 0)
				overallCentroid = centroidSum / num;

			// 第一行：概要信息
			_table->setItem(1, 1, new QTableWidgetItem(QString("%1").arg(totalVolume * _scale * _scale * _scale)));
			setTableRowQVector3DData(1, 2, overallCentroid * _scale);
			_table->setItem(1, 5, new QTableWidgetItem(QString("%1").arg(totalNodes)));
			_table->setItem(1, 6, new QTableWidgetItem(QString("%1").arg(totalElements)));

			// 更新状态栏
			statusInfo += QString(": 体积 = %1%2").arg(totalVolume * _scale * _scale * _scale).arg(m3);
		}

		setTableVerticalHeader(verticalHeaderLabels);
		updateStatusBarSelectInfo(statusInfo);
		_table->resizeColumnsToContents();
	}

	void mSelectInfoWindow::slot_highlightPostData()
	{
		const auto perfT0 = std::chrono::steady_clock::now();
		_controlsWidget->setVisible(true);
		QString type = "节点";
		int num = 0;
		std::vector<MeshSelectInfoData> res;
		//获取拾取模式
		PickDataType pickDataType = getCVM->getPostPickDataType(num);
		if (pickDataType != PickDataType::Node && pickDataType != PickDataType::Element)
		{
			_table->clear();
			_table->setRowCount(0);
			_table->setColumnCount(0);
			updateStatusBarSelectInfo("无");
			return;
		}

		switch (pickDataType)
		{
		case PickDataType::Node:
		{
			res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
			type = "节点";
			break;
		}
		case PickDataType::Element:
		{
			res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
			type = "单元";
			break;
		}
		default:
			break;
		}
		
		num = res.size();

		QString statusInfo = "无";
		_table->clear();
		_table->setColumnCount(1);
		_table->setRowCount(1);
		if (num == 0)
		{
			updateStatusBarSelectInfo(statusInfo);
			GUI_PERF_LOG("gui-post-select-info",
				std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - perfT0).count(),
				"num=0");
			return;
		}

		QStringList tableHeaderList;
		tableHeaderList << "序号" << "ID" << "节点坐标" << "单元包含的节点ID" << "结果值" << "部件名称" << "工况名称";
		_table->setColumnCount(7);
		setTableHorizontalHeader(tableHeaderList);

		statusInfo = QString("%1 %2").arg(num).arg(type);

		std::vector<int> ids;
		ids.reserve(res.size());
		for (auto r : res)
		{
			ids.push_back(r.data_id.element_id);
		}
		//与前处理一致限制明细行数，避免几十万行QTableWidgetItem卡死界面；状态栏仍显示真实总数
		constexpr std::size_t kMaxDetailRows = 1000;
		if (ids.size() > kMaxDetailRows)
		{
			ids.resize(kMaxDetailRows);
		}
		if (type == "节点")
		{
			slot_setPostData(ids, 1);
		}
		else if (type == "单元")
		{
			slot_setPostData(ids, 2);
		}

		for (auto it = _postCheckedState.begin(); it != _postCheckedState.end(); ++it) 
		{
			int col = it.key();
			bool checked = it.value();
			_table->setColumnHidden(col, !checked);
		}
		updateStatusBarSelectInfo(statusInfo);
		_table->resizeColumnsToContents();
		int colCount = _table->columnCount();
		for (int col = 0; col < colCount; ++col) {
			int width = _table->columnWidth(col);
			width = qBound(100, width, 200); // 这里80是最小，200是最大，可以按实际调整
			_table->setColumnWidth(col, width);
		}

		GUI_PERF_LOG("gui-post-select-info",
			std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - perfT0).count(),
			("num=" + std::to_string(num)).c_str());
	}

	void mSelectInfoWindow::slot_setPostData(std::vector<int> ids, int type)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		QString postname = postRend->getPostDataName();

		//获取后处理数据
		QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr)
			return;

		mPostModelData* modeldata = postDB->getModelData(guidata->_cloudFrame);
		if (modeldata == nullptr) return;

		//整帧结果与选中数量无关，只取一次供所有行使用；逐行调用会按行数放大整帧取值开销
		const auto perfT0 = std::chrono::steady_clock::now();
		int resultFlag = 0;
		const QHash<int, double> frameValues = postDB->getResultDataByFrameID(guidata->_cloudFrame, resultFlag);

		//批量填充：一次性分配行并暂停重绘，避免逐行insertRow+setItem触发布局刷新
		int writeRow = 0;
		_table->setUpdatesEnabled(false);
		_table->setRowCount(static_cast<int>(ids.size()));

		//节点
		if (type == 1)
		{
			for (int i = 0; i < ids.size(); i++)
			{
				QStringList itemList{};

				//序号
				itemList << QString("节点%1").arg(i + 1);

				//ID
				itemList << MxFunctions::mNumberToString(ids[i]);

				//节点坐标
				int cId = modeldata->_nodedata.nodeidmap.value(ids[i]);
				if (cId == -1)
				{
					continue;
				}

				QVector3D vertex = modeldata->getCoordinates(cId);
				QString coord = QString("%1 %2 %3").arg(MxFunctions::mNumberToString(vertex.x(), 'f', 2))
					.arg(MxFunctions::mNumberToString(vertex.y(), 'f', 2)).arg(MxFunctions::mNumberToString(vertex.z(), 'f', 2));
				itemList << coord;

				//单元包含的节点
				itemList << " ";

				//结果值
				//flag结果类型 0：节点 1：单元 2：单元节点
				if (resultFlag == 0)
				{
				double value = frameValues.value(cId);
				QString noderes = MxFunctions::mNumberToString(value, 'E', 3);
					itemList << noderes;
				}
				else
				{
					itemList << " ";
				}

				//部件名称
				int cid = modeldata->_nodedata.nodeidmap.value(ids[i]);
				if (cId == -1)
				{
					continue;
				}
				int nodepartid = modeldata->getPartIDByNodeID(cid);
				QString nodepartname = modeldata->getPartNameByPartId(nodepartid);
				itemList << nodepartname;

				//工况名称
				QString casename = modeldata->_frameInfo._frameCase;
				itemList << casename;

				for (int col = 0; col < itemList.size(); ++col)
				{
					_table->setItem(writeRow, col, new QTableWidgetItem(itemList[col]));
			}
				++writeRow;
		}
		}
		//单元
		else
		{
			//单元节点结果（flag==2）同样与行无关，只取一次
			QHash<int, QHash<int, double>> eleNodeVals;
			if (resultFlag == 2)
			{
				eleNodeVals = postDB->getEleNodeResultDataByFrameID(guidata->_cloudFrame);
			}
			for (int i = 0; i < ids.size(); i++)
			{
				QStringList itemList{};

				//序号
				itemList << QString("单元%1").arg(i + 1);

				//ID
				itemList << MxFunctions::mNumberToString(ids[i]);

				//节点坐标
				itemList << "  ";

				//单元包含的节点
				int element = ids[i];
				int partid = modeldata->getPartIDByEleID(element);
				//连续id
				QList<int> nodeid = modeldata->getElementNodeId(partid, element);

				QStringList stringList;
				for (int num : nodeid)
				{
					//真实id
					int id = modeldata->_nodedata.ids[num];
					stringList.append(MxFunctions::mNumberToString(id));
				}
				QString result = stringList.join("  ");
				itemList << result;

				//结果值
				//flag结果类型 0：节点 1：单元 2：单元节点

				//单元
				if (resultFlag == 1)
				{
					int partid = modeldata->getPartIDByEleID(ids[i]);
					double value = frameValues.value(ids[i]);
					QString eleres = MxFunctions::mNumberToString(value, 'E', 3);
					itemList << eleres;
				}
				//单元节点
				else if (resultFlag == 2)
				{
					int element = ids[i];
					int partid = modeldata->getPartIDByEleID(element);
					QList<int> nodeid = modeldata->getElementNodeId(partid, element);

					double sum = 0;
					QHash<int, double> nodeVals = eleNodeVals[element];
					for (int n = 0; n < nodeid.size(); ++n)
					{
						sum += nodeVals[nodeid[n]];
					}
					double value = sum / nodeid.size();
					QString elenoderes = MxFunctions::mNumberToString(value, 'E', 3);
					itemList << elenoderes;
				}
				else
				{
					itemList << " ";
				}

				//部件名称
				int elepartid = modeldata->getPartIDByEleID(ids[i]);
				QString elepartname = modeldata->getPartNameByPartId(elepartid);
				itemList << elepartname;

				//工况名称
				QString casename = modeldata->_frameInfo._frameCase;
				itemList << casename;

				for (int col = 0; col < itemList.size(); ++col)
				{
					_table->setItem(writeRow, col, new QTableWidgetItem(itemList[col]));
			}
				++writeRow;
		}

		_table->setRowCount(writeRow);
		_table->setUpdatesEnabled(true);

		GUI_PERF_LOG("gui-post-table-fill",
			std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - perfT0).count(),
			("rows=" + std::to_string(writeRow)).c_str());
	}

		_table->setRowCount(writeRow);
		_table->setUpdatesEnabled(true);

		GUI_PERF_LOG("gui-post-table-fill",
			std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - perfT0).count(),
			("rows=" + std::to_string(writeRow)).c_str());
	}

	void mSelectInfoWindow::slot_contextMenuTableData(const QPoint& pos)
	{
		if (getCVM->getPostRenderWidget()) return;

		mUiMenu menu(_table);
		menu.addActions({ _copyAction, _flushedAction, _reselectAction });

		QAction* selected = menu.exec(_table->viewport()->mapToGlobal(pos));
		if (selected == _copyAction) {
			QList<QTableWidgetItem*> items = _table->selectedItems();
			if (!items.isEmpty()) {
				// 取第一行的 row
				int firstRow = items.first()->row();

				// 挑出第一行中被选中的单元格
				QList<int> selectedCols;
				for (QTableWidgetItem* item : items) {
					if (item->row() == firstRow)
						selectedCols.append(item->column());
				}
				std::sort(selectedCols.begin(), selectedCols.end());
				selectedCols.erase(std::unique(selectedCols.begin(), selectedCols.end()), selectedCols.end());

				// 拼接文本
				QStringList rowData;
				for (int col : selectedCols) {
					QTableWidgetItem* item = _table->item(firstRow, col);
					rowData << (item ? item->text() : "");
				}
				QString text = rowData.join(" ");
				QApplication::clipboard()->setText(text);
			}
		}
		else if (selected == _flushedAction) {
			_table->clearSelection();
		}
		else if (selected == _reselectAction) {
			// 获取选中的行
			QList<QTableWidgetItem*> items = _table->selectedItems();
			QList<int> selectedRows;
			for (QTableWidgetItem* item : items) {
				if (!selectedRows.contains(item->row())) {
					selectedRows.append(item->row());
				}
			}
			std::sort(selectedRows.begin(), selectedRows.end());

			// ✅ 这里你就拿到了 QList<int> selectedRows
			qDebug() << "选中的行:" << selectedRows;

			//将拾取高亮的值更新为选中的行的内容
			//通过行ID拿到真实的ID，然后传递到渲染拾取中
		}
	}

	void mSelectInfoWindow::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
		if (_currentViewportPrivate == VMManager->getCurrentViewportPrivate())
		{
			return;
		}
		_currentViewportPrivate = VMManager->getCurrentViewportPrivate();
		if(conn_geo)
			disconnect(conn_geo);

		if (conn_post)
			disconnect(conn_post);

		if (!getCVM)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
			conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mSelectInfoWindow::slot_highlightGeoData);

		if (getCVM->getPostRenderWidget())
			conn_post = connect(getCVM->getPostRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mSelectInfoWindow::slot_highlightPostData);

		slot_highlightGeoData();
	}

	void mSelectInfoWindow::slot_unitSystemChanged(int index)
	{
		if (getCVM == nullptr)
		{
			return;
		}
		auto unit = getCVM->getModelData()->getUnitSystemManager();
		if (unit)
		{
			//unit->getSwitchUnitSystemRatio(Unit::Length);
			_length = UnitsLibrary[1].at(index);
			_scale = unit->getUnitSystemFromToRatio(Unit::Length,{ 3, 0, 0, 0 }, unit->getCurUnitsIndex());
		}
		slot_highlightGeoData();
	}

	void mSelectInfoWindow::slot_changeLanguage(int language)
	{
		//if (language == 0)
		//{
		//	//设置Dock名称
		//	this->setDockWindowTitle(QString("选择信息"));
		//}
		//else if (language == 1)
		//{
		//	//设置Dock名称
		//	this->setDockWindowTitle(QString("SelectInfo"));
		//}
	}

	mSelectInfoWindow::~mSelectInfoWindow()
	{

	}
}
