//#include "mViewportManager.h"
//
////Render
//#include <mBaseRend.h>
//#include <CCSMPreRender.h>
//#include <mBackGroundRender.h>
//#include <mPlotView.h>
//#include <mViewBase.h>
//#include <mLableDataController_pre.h>
//#include <mLableRendController_pre.h>
//#include <mGeoModelData1.h>
//#include "mGeoPartData1.h"
//#include "mGeoFaceData1.h"
//#include "mGeoPointData1.h"
//#include "mGeoSolidData1.h"
//#include "mGeoLineData1.h"
//#include "mGeoPickData1.h"
//#include "IRenderDataProxy.h"
//#include "mShapeRender.h"
//
//#include "CCSMRendWidget.h"
//#include "mFontRender.h"
//#include <QVector3D>
//#include <QHBoxLayout>
//#include <QSplitter>
//#include <iostream>
//#include "mLableRendController_common.h"
//#include "mShapeRender.h"
////#include "mLableDataController_common.h"
//#include "mLableDataController_common.cpp"
//#include "mGlobalSignals.h"
//#include "CCSMRendWidget.h"
//#include "mGeoPartData1.h"
//#include "mPreMeshPickData1.h"
//#include "CCSMPreRender.h"
//#include "mPreMeshModelRender.h"
//#include "mPreGeoModelRender.h"
//#include "mScaleBar.h"
//#include "mAppConfiguration.h"
//using namespace MxFunctions;
//using namespace MDataGeo;
//namespace MechCommon
//{
//	mViewportManager* mViewportManager::_instance = nullptr;
//
//	mViewportManager* mViewportManager::getInstance()
//	{
//		if (_instance == nullptr)
//		{
//			_instance = new mViewportManager;
//
//		}
//		return _instance;
//	}
//
//	void mViewportManager::connectSignal()
//	{
//		connect(_preport.first->getPreRender().get(), &CCSMPreRender::finishedPickSig, this, &mViewportManager::FinishPickedSig);
//		connect(this, &mViewportManager::deleteGeoPartSig, this, &mViewportManager::deleteGeoPart);//根据名字删除模型，	
//		connect(this, &mViewportManager::deleteAllGeoPartSig, this, &mViewportManager::deleteAllGeoPart);//删除所有几何
//
//	}
//
//
//	mViewportManager::mViewportManager()
//	{
//		//设置窗口背景
//		this->setBackground(QBrush(Qt::NoBrush));
//		this->setStyleSheet("QMdiArea{"
//			" background-image: url(:/mMainWidgets/Icon/bg.png);"
//			" background-repeat: no-repeat;"
//			" background-position: center;"
//			" background-color: rgba(0, 0, 0, 0);"
//			"}");
//
//		//初始化
//		_geoData = nullptr;
//		_geoPickData = nullptr;
//		_meshPickData = nullptr;
//
//		buttonFlag = false;
//		pickType = 0;
//		_pickMode = MultiplyPickMode::QuadPick;
//	}
//
//	mViewportManager::~mViewportManager()
//	{
//		this->closeViewports();
//	}
//
//
//	void mViewportManager::openViewport3D(MRendView::CCSMRendWidget* pview)
//	{
//		if (pview == nullptr)
//		{
//			return;
//		}
//
//		//获取视口名称
//		QString viewName = pview->getViewportName();
//
//		//创建MDI子窗口
//		QMdiSubWindow* subWindow = nullptr;
//		if (_preport.first == pview)
//		{
//			subWindow = _preport.second;
//		}
//		else
//		{
//			subWindow = this->addSubWindow(pview);
//			_preport.first = pview;
//			_preport.second = subWindow;
//		}
//
//		//显示MDI子窗口
//		if (subWindow != nullptr)
//		{
//			subWindow->activateWindow();
//			subWindow->setWindowFlags(Qt::FramelessWindowHint);
//			subWindow->showMaximized();
//			subWindow->setWindowTitle(viewName);
//
//		}
//
//		//获取几何数据指针,获取几何拾取数据指针
//		shared_ptr<CCSMPreRender> render = pview->getPreRender();
//		if (render != nullptr)
//		{
//			_geoData = render->getGeoModelData();
//			_geoPickData = render->getGeoPickData();
//			_meshPickData = render->getMeshPickData();
//			_meshData_temp = render->getMeshPickData_temp();
//		}
//
//
//		connectSignal();
//
//	}
//
//	void mViewportManager::closeViewport3D()
//	{
//		CCSMRendWidget* pview = _preport.first;
//		QMdiSubWindow* subwin = _preport.second;
//		if (pview != nullptr)
//		{
//			// TODO: black screen on old mesa
//#ifdef __linux__
//			pview->setParent(nullptr);
//#else
//			delete pview;
//#endif
//			pview = nullptr;
//			_meshPickData = nullptr;
//		}
//		if (subwin != nullptr)
//		{
//			delete subwin;
//			subwin = nullptr;
//		}
//		_preport.first = nullptr;
//		_preport.second = nullptr;
//	}
//
//	MPlotView::mPlotView* mViewportManager::getTimeCurveView()
//	{
//		return nullptr;
//	}
//
//	MPlotView::mPlotView* mViewportManager::getTimeCurveView(const QString& name)
//	{
//		return nullptr;
//	}
//
//	void mViewportManager::closeViewports()
//	{
//		//关闭前处理3D窗口
//		this->closeViewport3D();
//	}
//
//	CCSMRendWidget* mViewportManager::getPreViewport()
//	{
//		return _preport.first;
//	}
//
//	void mViewportManager::clearCommonFont()
//	{
//		_preport.first->getFontRender()->clearCommonFont("adjustNum");
//		_preport.first->getFontRender()->clearCommonFont("biastNum");
//	}
//
//	void mViewportManager::appendCommonFontInt(const QString& name, QVector<QVector3D> pos1, QVector<int> txt)
//	{
//		_preport.first->getFontRender()->appendCommonFont(name, pos1, txt);
//	}
//
//	void mViewportManager::appendCommonFontdouble(const QString& name, QVector<QVector3D> pos1, QVector<double> txt)
//	{
//		_preport.first->getFontRender()->appendCommonFont(name, pos1, txt);
//	}
//
//
//	QMdiSubWindow* mViewportManager::getPreSubViewport()
//	{
//		return _preport.second;
//	}
//
//	void mViewportManager::setSubWindowLayout(const QString& objectName, const QStringList& names)
//	{
//		QList<QString> List1;
//		QList<QMdiSubWindow*> windows;
//		windows.push_back(mViewportManager::getInstance()->getPreSubViewport());
//		QList<QMdiSubWindow*> List;
//		for (int i = 0; i < windows.size(); ++i) {
//			QMdiSubWindow* mdiSubWindow = windows.at(i);
//			CCSMRendWidget* child = qobject_cast<CCSMRendWidget*>(mdiSubWindow->widget());
//
//			if (child == nullptr)
//			{
//				continue;
//			}
//			if (names.contains(child->getViewportName()))
//			{
//				List.push_back(windows[i]);
//			}
//			windows[i]->hide();
//		}
//
//		if (objectName == "layout1")
//		{
//			//pushButton_3
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			splitterMain->addWidget(List[0]);
//			splitterMain->addWidget(List[1]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout2")
//		{
//			//pushButton_4
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			splitterMain->addWidget(List[0]);
//			splitterMain->addWidget(List[1]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//
//		}
//		else if (objectName == "layout3")
//		{
//			//pushButton_5
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			splitterMain->addWidget(List[0]);
//			splitterMain->addWidget(List[1]);
//			splitterMain->addWidget(List[2]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout4")
//		{
//			//pushButton_6
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			splitterMain->addWidget(List[0]);
//			QSplitter* splitter1 = new QSplitter(Qt::Vertical, splitterMain);
//			splitter1->addWidget(List[1]);
//			splitter1->addWidget(List[2]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout5")
//		{
//			//pushButton_7
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Vertical, splitterMain);;
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			mLayout1->addWidget(List[0]);
//			mLayout1->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout6")
//		{
//			//pushButton_8
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			splitterMain->addWidget(List[0]);
//			splitterMain->addWidget(List[1]);
//			splitterMain->addWidget(List[2]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout7")
//		{
//			//pushButton_9
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			mLayout1->addWidget(List[0]);
//			mLayout2->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout8")
//		{
//			//pushButton_10
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//			mLayout1->addWidget(List[0]);
//			mLayout1->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout9")
//		{
//			//pushButton_11
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			splitterMain->addWidget(List[0]);
//			splitterMain->addWidget(List[1]);
//			splitterMain->addWidget(List[2]);
//			splitterMain->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout10")
//		{
//			//pushButton_12
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout1->addWidget(List[0]);
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//			QSplitter* mLayout3 = new QSplitter(Qt::Horizontal, mLayout2);
//			QSplitter* mLayout4 = new QSplitter(Qt::Horizontal, mLayout2);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			mLayout3->addWidget(List[1]);
//			mLayout3->addWidget(List[2]);
//			mLayout4->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout11")
//		{
//			//pushButton_13
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			mLayout1->addWidget(List[0]);
//			mLayout1->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout2->addWidget(List[3]);
//
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout12")
//		{
//			//pushButton_14
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			mLayout1->addWidget(List[0]);
//			mLayout1->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout13")
//		{
//			//pushButton_15
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Vertical, splitterMain);;
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//
//
//			QSplitter* mLayout3 = new QSplitter(Qt::Horizontal, mLayout2);
//			QSplitter* mLayout4 = new QSplitter(Qt::Horizontal, mLayout2);
//
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			mLayout1->addWidget(List[0]);
//			mLayout3->addWidget(List[1]);
//			mLayout4->addWidget(List[2]);
//			mLayout4->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout14")
//		{
//			//pushButton_16
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Vertical, splitterMain);;
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//			mLayout1->addWidget(List[0]);
//
//			mLayout2->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout15")
//		{
//			//pushButton_17
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Vertical, splitterMain);;
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//			QSplitter* mLayout3 = new QSplitter(Qt::Vertical, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//			mLayout1->addWidget(List[0]);
//			mLayout2->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout3->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout16")
//		{
//			//pushButton_19
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//			splitterMain->addWidget(List[0]);
//			splitterMain->addWidget(List[1]);
//			splitterMain->addWidget(List[2]);
//			splitterMain->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout17")
//		{
//			//pushButton_20
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Vertical, splitterMain);;
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//			QSplitter* mLayout3 = new QSplitter(Qt::Vertical, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//
//			mLayout1->addWidget(List[0]);
//			mLayout1->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout3->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout18")
//		{
//			//pushButton_21
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout3 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//
//			mLayout1->addWidget(List[0]);
//			mLayout1->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout3->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout19")
//		{
//			//pushButton_22
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout3 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//
//			mLayout1->addWidget(List[0]);
//			mLayout2->addWidget(List[1]);
//			mLayout3->addWidget(List[2]);
//			mLayout3->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout20")
//		{
//			//pushButton_23
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout3 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//			mLayout1->addWidget(List[0]);
//			mLayout2->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout3->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout21")
//		{
//			//pushButton_24
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//
//
//			QSplitter* mLayout3 = new QSplitter(Qt::Vertical, mLayout1);
//			QSplitter* mLayout4 = new QSplitter(Qt::Horizontal, mLayout1);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//
//			mLayout3->addWidget(List[0]);
//			mLayout3->addWidget(List[1]);
//			mLayout4->addWidget(List[2]);
//
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout22")
//		{
//			//pushButton_25
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//
//
//			QSplitter* mLayout3 = new QSplitter(Qt::Horizontal, mLayout1);
//			QSplitter* mLayout4 = new QSplitter(Qt::Vertical, mLayout1);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//			mLayout3->addWidget(List[0]);
//
//			mLayout4->addWidget(List[1]);
//			mLayout4->addWidget(List[2]);
//
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout23")
//		{
//			//pushButton_26
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//
//
//			QSplitter* mLayout3 = new QSplitter(Qt::Vertical, mLayout2);
//			QSplitter* mLayout4 = new QSplitter(Qt::Horizontal, mLayout2);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//			mLayout1->addWidget(List[0]);
//
//			mLayout3->addWidget(List[1]);
//			mLayout3->addWidget(List[2]);
//			mLayout4->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout24")
//		{
//			//pushButton_27
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//
//
//			QSplitter* mLayout3 = new QSplitter(Qt::Vertical, mLayout2);
//			QSplitter* mLayout4 = new QSplitter(Qt::Vertical, mLayout2);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//
//			mLayout1->addWidget(List[0]);
//
//			mLayout3->addWidget(List[1]);
//			mLayout4->addWidget(List[2]);
//			mLayout4->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout25")
//		{
//			//pushButton_28
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Vertical, splitterMain);;
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//
//
//
//			QSplitter* mLayout3 = new QSplitter(Qt::Horizontal, mLayout1);
//			QSplitter* mLayout4 = new QSplitter(Qt::Horizontal, mLayout1);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//
//
//			mLayout3->addWidget(List[0]);
//			mLayout3->addWidget(List[1]);
//			mLayout4->addWidget(List[2]);
//
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout26")
//		{
//			//pushButton_29
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Vertical, splitterMain);;
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//
//
//			QSplitter* mLayout3 = new QSplitter(Qt::Horizontal, mLayout1);
//			QSplitter* mLayout4 = new QSplitter(Qt::Horizontal, mLayout1);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//
//
//			mLayout3->addWidget(List[0]);
//			mLayout4->addWidget(List[1]);
//			mLayout4->addWidget(List[2]);
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout27")
//		{
//			//pushButton_30
//			delete this->layout();
//			QHBoxLayout* mLayout = new QHBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Horizontal);
//			QSplitter* mLayout1 = new QSplitter(Qt::Vertical, splitterMain);;
//			QSplitter* mLayout2 = new QSplitter(Qt::Vertical, splitterMain);
//
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//			mLayout1->addWidget(List[0]);
//			mLayout1->addWidget(List[1]);
//			mLayout1->addWidget(List[2]);
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//
//
//		}
//		else if (objectName == "layout28")
//		{
//			//pushButton_31
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//
//
//
//			mLayout1->addWidget(List[0]);
//			mLayout2->addWidget(List[1]);
//			mLayout2->addWidget(List[2]);
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//		else if (objectName == "layout29")
//		{
//			//pushButton_32
//			delete this->layout();
//			QVBoxLayout* mLayout = new QVBoxLayout(this);
//			QSplitter* splitterMain = new QSplitter(Qt::Vertical);
//			QSplitter* mLayout1 = new QSplitter(Qt::Horizontal, splitterMain);
//			QSplitter* mLayout2 = new QSplitter(Qt::Horizontal, splitterMain);
//			mLayout->setContentsMargins(0, 0, 0, 0);
//			mLayout->setSpacing(0);
//
//			mLayout1->addWidget(List[0]);
//			mLayout1->addWidget(List[1]);
//			mLayout1->addWidget(List[2]);
//			mLayout2->addWidget(List[3]);
//			mLayout->addWidget(splitterMain);
//			this->setLayout(mLayout);
//		}
//
//		for (int i = 0; i < List.size(); ++i) {
//			List[i]->showNormal();
//		}
//	}
//
//	bool mViewportManager::isLinePick(const std::vector<QVector3D>& lines, const QVector2D& mouse) {
//		std::vector<glm::vec2> mouseVec2;
//		auto camera = getPreViewport()->getCamera();
//		auto pvm = camera->getPVMValue();
//		float offset = 5.0f;
//		mouseVec2.push_back({ mouse[0] - offset,mouse[1] - offset });
//		mouseVec2.push_back({ mouse[0] + offset,mouse[1] + offset });
//		for (int i = 0; i < lines.size() / 2; i++) {
//			std::vector<glm::vec2> line;
//			for (int j = 0; j < 2; j++) {
//				auto l_vec2 = mPickToolClass::WorldVertexToScreenVertex({ lines[i * 2 + j][0], lines[i * 2 + j][1], lines[i * 2 + j][2] }, pvm, camera->SCR_WIDTH(), camera->SCR_HEIGHT());
//				line.push_back({ l_vec2[0],l_vec2[1] });
//			}
//			if (mPickToolClass::IsLineIntersectionWithQuad(line, mouseVec2, MxFunctions::MeshType::MeshBeam)) return true;
//		}
//		return false;
//	}
//
//	bool mViewportManager::isFacePick(const std::vector<std::vector<QVector3D>>& faces, const QVector2D& mouse)
//	{
//		std::vector<glm::vec2> mouseVec2;
//		auto camera = getPreViewport()->getCamera();
//		auto pvm = camera->getPVMValue();
//		float offset = 0.5f;
//		mouseVec2.push_back({ mouse[0],mouse[1] });
//		std::vector<QVector3D> face_points(3);
//		QVector<QVector2D> face_vec2(3);
//		for (const auto& face : faces) {
//			// 构建面
//			face_vec2.clear();
//			face_points.clear();
//			for (int i = 0; i < face.size() - 2; i++) {
//				face_points.push_back(face[0]);
//				face_points.push_back(face[i + 1]);
//				face_points.push_back(face[i + 2]);
//			}
//			for (int j = 0; j < 3; j++) {
//				auto l_vec2 = mPickToolClass::WorldVertexToScreenVertex({ face_points[j][0], face_points[j][1], face_points[j][2] }, pvm, camera->SCR_WIDTH(), camera->SCR_HEIGHT());
//				face_vec2.push_back({ l_vec2[0],l_vec2[1] });
//			}
//			if (mPickToolClass::IsTriIntersectionWithCircle(face_vec2, mouse, offset)) return true;
//		}
//		return false;
//	}
//
//
//	bool mViewportManager::getGeoData()
//	{
//		if (_geoData == nullptr)
//		{
//			//获取几何数据指针
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				_geoData = render->getGeoModelData();
//			}
//			if (_geoData == nullptr)
//			{
//				return false;
//			}
//		}
//		return true;
//	}
//
//	void mViewportManager::deletePartData(int partName)
//	{
//		if (!getGeoData())
//		{
//			return;
//		}
//		_geoData->deleteGeoPart(partName);
//	}
//
//	void mViewportManager::setGeoPartVisible(const int& name, bool isshow)
//	{
//		if (_geoData == nullptr)
//		{
//			return;
//		}
//		_geoData->setGeoPartVisual(name, isshow);
//	}
//
//	void mViewportManager::setGeoPartColor(const int& name, const QColor& color)
//	{
//		if (_geoData == nullptr)
//		{
//			return;
//		}
//		_geoData->setGeoPartColor(name, QVector3D(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0));
//	}
//
//	void mViewportManager::setGeoHardPointVisual(int partName, bool isShow)
//	{
//		if (_geoData == nullptr)
//		{
//			return;
//		}
//		_geoData->setGeoPartVisual(partName, isShow);
//	}
//
//	void mViewportManager::setAllPartGeoHardPointVisual(bool isShow)
//	{
//		_geoData->setAllGeoHardPointVisual(isShow);
//		updateRender();
//	}
//
//	void mViewportManager::setGeoPartVertexArray(int PartID, int PartType, const std::vector<QVector3D>& xyz)
//	{
//		if (!getGeoData())
//		{
//			return;
//		}
//		mGeoPartData1* partData = _geoData->getGeoPartDataByPartName(PartID);
//
//		if (partData == nullptr)
//		{
//			partData = new mGeoPartData1(_geoData, PartID);
//			partData->setGeoShapeType(PartType);
//		}
//
//		std::vector<glm::vec3f> vertexArray;
//		for (int i = 0; i < xyz.size(); i++)
//		{
//			vertexArray.push_back(glm::vec3f(xyz[i][0], xyz[i][1], xyz[i][2]));
//		}
//
//		partData->setVertexArray(vertexArray);
//	}
//
//	void mViewportManager::appendGeoPointData(int PartID, int PartType, int VerId, int vertexIndex, int type)
//	{
//		if (!getGeoData())
//		{
//			return;
//		}
//		mGeoPartData1* partData = _geoData->getGeoPartDataByPartName(PartID);
//		mGeoPointData1* geoPointData;
//		if (partData == nullptr)
//		{
//			partData = new mGeoPartData1(_geoData, PartID);
//			partData->setGeoShapeType(PartType);
//		}
//
//		/*****************点***********/
//
//		geoPointData = new mGeoPointData1(_geoData, PartID, VerId);
//		geoPointData->setGeoPointProperty(PointProperty(type));
//		geoPointData->setPointData(VerId, vertexIndex);
//		partData->setGeoShapeType(PartType);
//
//		//[todo]计算包围盒,后续需要封装到数据里
//		Space::AABB aabb;
//		const std::vector<glm::vec3f>& vertexArray = partData->getVertexArray();
//		if (vertexIndex < vertexArray.size())
//		{
//			auto& vertex = vertexArray[vertexIndex];
//			aabb.expandBy(QVector3D(vertex[0], vertex[1], vertex[2]));
//		}
//		geoPointData->setGeoPointAABB(aabb);
//
//		partData->appendGeoPointID(VerId);
//	}
//
//	void mViewportManager::appendGeoLineData(int PartID, int PartType, int EdgeId,
//		const std::vector<int>& vertexsIndex, int Type)
//	{
//		if (!getGeoData())
//		{
//			return;
//		}
//		mGeoPartData1* partData = _geoData->getGeoPartDataByPartName(PartID);
//		mGeoLineData1* geoLineData;
//		if (partData == nullptr)
//		{
//			partData = new mGeoPartData1(_geoData, PartID);
//			partData->setGeoShapeType(PartType);
//		}
//
//		/*****************线***********/
//		geoLineData = new mGeoLineData1(_geoData, PartID, EdgeId);
//		geoLineData->setGeoLineProperty(GeoLineProperty(Type));
//		geoLineData->appendGeoLineData(EdgeId, vertexsIndex);
//
//
//		//[todo]计算包围盒,后续需要封装到数据里
//		Space::AABB aabb;
//		const std::vector<glm::vec3f>& vertexArray = partData->getVertexArray();
//		for (auto index : vertexsIndex)
//		{
//			if (index < vertexArray.size())
//			{
//				auto& vertex = vertexArray[index];
//				aabb.expandBy(QVector3D(vertex[0], vertex[1], vertex[2]));
//			}
//		}
//		geoLineData->setGeoLineAABB(aabb);
//
//		partData->appendGeoLineID(EdgeId);
//	}
//
//	void mViewportManager::appendGeoFaceData(int PartID, int PartType, int FaceId,
//		const std::vector<int>& vertexsIndex)
//	{
//		if (!getGeoData())
//		{
//			return;
//		}
//		mGeoPartData1* partData = _geoData->getGeoPartDataByPartName(PartID);
//		mGeoFaceData1* geoFaceData;
//		if (partData == nullptr)
//		{
//			partData = new mGeoPartData1(_geoData, PartID);
//			partData->setGeoShapeType(PartType);
//		}
//
//		/*****************面***********/
//		geoFaceData = new mGeoFaceData1(_geoData, PartID, FaceId);
//		geoFaceData->appendGeoFaceData(FaceId, vertexsIndex);
//
//		//[todo]计算包围盒,后续需要封装到数据里
//		Space::AABB aabb;
//		const std::vector<glm::vec3f>& vertexArray = partData->getVertexArray();
//		for (auto index : vertexsIndex)
//		{
//			if (index < vertexArray.size())
//			{
//				auto& vertex = vertexArray[index];
//				aabb.expandBy(QVector3D(vertex[0], vertex[1], vertex[2]));
//			}
//		}
//		geoFaceData->setGeoFaceAABB(aabb);
//
//		partData->appendGeoFaceID(FaceId);
//	}
//
//	//void mViewportManager::appendGeoSolidData(int PartID, int PartType, int SolidId,
//	//	const std::vector<int>& faceIds)
//	//{
//	//	if (!getGeoData())
//	//	{
//	//		return;
//	//	}
//	//	mGeoPartData1* partData = _geoData->getGeoPartDataByPartName(PartID);
//	//	mGeoSolidData1* geoSolidData;
//	//	if (partData == nullptr)
//	//	{
//	//		partData = new mGeoPartData1(_geoData, PartID);
//	//		partData->setGeoShapeType(PartType);
//	//	}
//
//	//	/*****************面***********/
//	//	geoSolidData = new mGeoSolidData1(_geoData, PartID, SolidId);
//	//	for (int id : faceIds)
//	//	{
//	//		auto faceData = _geoData->getGeoFaceDataByID(id);
//	//		faceData->setSolidID(SolidId);
//	//		geoSolidData->appendGeoSolidFace(id);
//	//	}
//
//	//	//[todo]计算包围盒,后续需要封装到数据里
//	//	Space::AABB aabb;
//	//	const std::vector<glm::vec3f>& vertexArray = partData->getVertexArray();
//	//	for (int id : faceIds)
//	//	{
//	//		aabb.expandBy(_geoData->getGeoFaceAABBByID(id));
//	//	}
//	//	geoSolidData->setGeoSolidAABB(aabb);
//
//	//	partData->appendGeoSolidID(SolidId);
//	//}
//
//	void mViewportManager::SetRotateCenterToModelCenter()
//	{
//		if (_preport.first)
//		{
//			_preport.first->SetRotateCenterToModelCenter();
//		}
//	}
//
//	void mViewportManager::setShowFunction(int index)
//	{
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->setShowFuntion(ShowFuntion(index));
//			}
//			this->updateRender();
//		}
//	}
//	void mViewportManager::setNodeSize(int size)
//	{
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				//render->setNodeSize(size);
//			}
//			this->updateRender();
//		}
//	}
//	void mViewportManager::setNodeColor(QVector4D color)
//	{
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				//render->setNodeColor(color);
//			}
//			this->updateRender();
//		}
//	}
//	void mViewportManager::setPointSize(int size)
//	{
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->setPointSize(size);
//			}
//			this->updateRender();
//		}
//	}
//
//	void mViewportManager::setEdgeLineWidth(float size)
//	{
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->setEdgeLineWidth(size);
//			}
//			this->updateRender();
//		}
//	}
//
//	void mViewportManager::setFaceLineColor(QVector4D color)
//	{
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->setFaceLineColor(color);
//			}
//			this->updateRender();
//		}
//	}
//	void mViewportManager::setBackGroundMode(BackGroundMode backgroundmode)
//	{
//		if (_preport.first)
//		{
//			_preport.first->getBackGroundRender()->setBackGroundMode(backgroundmode);
//		}
//	}
//	BackGroundMode mViewportManager::getBackGroundMode()
//	{
//		if (_preport.first)
//		{
//			_preport.first->getBackGroundRender()->getBackGroundMode();
//		}
//		return BackGroundMode();
//	}
//	void mViewportManager::setGradientColor(QVector3D color1, QVector3D color2)
//	{
//		if (_preport.first)
//		{
//			_preport.first->getBackGroundRender()->setGradientColor(color1, color2);
//		}
//	}
//	void mViewportManager::getGradientColor(QVector3D& color1, QVector3D& color2)
//	{
//		if (_preport.first)
//		{
//			_preport.first->getBackGroundRender()->getGradientColor(color1, color2);
//		}
//	}
//	void mViewportManager::setPureColor(QVector3D color)
//	{
//		if (_preport.first)
//		{
//			_preport.first->getBackGroundRender()->setPureColor(color);
//			MAppConfiguration::mAppConfiguration::getInstance()->_pureColor = QString("%1,%2,%3").arg(color.x()).arg(color.y()).arg(color.z());
//		}
//	}
//	void mViewportManager::getPureColor(QVector3D& color)
//	{
//		if (_preport.first)
//		{
//			_preport.first->getBackGroundRender()->getPureColor(color);
//		}
//	}
//	void mViewportManager::resetCameraCenter()
//	{
//		if (_geoData)
//		{
//			_geoData->setResetCamera(true);
//		}
//	}
//	shared_ptr<mLableRendController_pre> mViewportManager::getCurrentLableRendController_pre()
//	{
//		CCSMRendWidget* preview = _preport.first;
//		if (preview != nullptr)
//		{
//			return preview->getLableRendController_pre();
//		}
//		return nullptr;
//	}
//
//	shared_ptr<mLableRendController_common> mViewportManager::getCurrentLableRendController_common()
//	{
//		CCSMRendWidget* preview = _preport.first;
//		if (preview != nullptr)
//		{
//			return preview->getLableRendController_common();
//		}
//		return nullptr;
//	}
//
//	shared_ptr<mLableDataController_pre> mViewportManager::getCurrentLableDataController_pre()
//	{
//		CCSMRendWidget* preview = _preport.first;
//		if (preview != nullptr)
//		{
//			return preview->getLableDataController_pre();
//		}
//		return nullptr;
//	}
//
//	void mViewportManager::appendLableRendData(const QString& Name, QVector<QVector3D> pos, float size, QVector3D color)
//	{
//		CCSMRendWidget* preview = _preport.first;
//		if (preview != nullptr)
//		{
//			shared_ptr<mLableRendController_common> lableRendController_common = preview->getLableRendController_common();
//			if (lableRendController_common != nullptr)
//			{
//				lableRendController_common->appendLableRendData<TempPoint_Common>(Name, pos, size, color, true, false);
//			}
//		}
//	}
//
//	void mViewportManager::deleteLableRendData(const QString& Name)
//	{
//		CCSMRendWidget* preview = _preport.first;
//		if (preview != nullptr)
//		{
//			shared_ptr<mLableRendController_common> lableRendController_common = preview->getLableRendController_common();
//			if (lableRendController_common != nullptr)
//			{
//				lableRendController_common->deleteLableRendData<TempPoint_Common>(Name);
//			}
//		}
//	}
//
//	void mViewportManager::setAllPickGeoPointData(const set<int>& Pointids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAllPickGeoPointData(Pointids);
//		}
//	}
//
//	void mViewportManager::setAllPickGeoLineData(set<int> Lineids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAllPickGeoLineData(Lineids);
//		}
//	}
//
//	void mViewportManager::setAllPickGeoFaceData(set<int> Faceids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAllPickGeoFaceData(Faceids);
//		}
//	}
//
//	void mViewportManager::setAllPickGeoSolidData(set<int> Solidids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAllPickGeoSolidData(Solidids);
//		}
//	}
//
//	void mViewportManager::setAllPickGeoPartData(set<int> partNames)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAllPickGeoPartData(partNames);
//		}
//	}
//
//	void mViewportManager::setAddPickGeoPointData(set<int> Pointids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAddPickGeoPointData(Pointids);
//		}
//
//		//更新Point所在Part的选中信息
//		for (auto iter = Pointids.begin(); iter != Pointids.end(); ++iter)
//		{
//			mGeoPointData1* geoPointData = _geoData->getGeoPointDataByID(*iter);
//			if (geoPointData != nullptr)
//			{
//				mGeoPartData1* geoPartData = _geoData->getGeoPartDataByPartName(geoPointData->getPartID());
//				if (geoPartData != nullptr)
//				{
//					geoPartData->appendPickGeoPointID(*iter);
//				}
//			}
//		}
//	}
//
//	void mViewportManager::setAddPickGeoLineData(set<int> Lineids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAddPickGeoLineData(Lineids);
//		}
//
//		//更新Line所在Part的选中信息
//		for (auto iter = Lineids.begin(); iter != Lineids.end(); ++iter)
//		{
//			mGeoLineData1* geoLineData = _geoData->getGeoLineDataByID(*iter);
//			if (geoLineData != nullptr)
//			{
//				mGeoPartData1* geoPartData = _geoData->getGeoPartDataByPartName(geoLineData->getPartID());
//				if (geoPartData != nullptr)
//				{
//					geoPartData->appendPickGeoLineID(*iter);
//				}
//			}
//		}
//	}
//
//	void mViewportManager::setAddPickGeoFaceData(set<int> Faceids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAddPickGeoFaceData(Faceids);
//		}
//
//		//更新Face所在Part的选中信息
//		for (auto iter = Faceids.begin(); iter != Faceids.end(); ++iter)
//		{
//			mGeoFaceData1* geoFaceData = _geoData->getGeoFaceDataByID(*iter);
//			if (geoFaceData != nullptr)
//			{
//				mGeoPartData1* geoPartData = _geoData->getGeoPartDataByPartName(geoFaceData->getPartID());
//				if (geoPartData != nullptr)
//				{
//					geoPartData->appendPickGeoFaceID(*iter);
//				}
//			}
//		}
//	}
//
//	void mViewportManager::setAddPickGeoSolidData(set<int> Solidids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAddPickGeoSolidData(Solidids);
//		}
//
//		//更新Solid所在Part的选中信息
//		for (auto iter = Solidids.begin(); iter != Solidids.end(); ++iter)
//		{
//			mGeoSolidData1* geoSolidData = _geoData->getGeoSolidDataByID(*iter);
//			if (geoSolidData != nullptr)
//			{
//				mGeoPartData1* geoPartData = _geoData->getGeoPartDataByPartName(geoSolidData->getPartID());
//				if (geoPartData != nullptr)
//				{
//					geoPartData->appendPickGeoSolidID(*iter);
//				}
//			}
//		}
//	}
//
//	void mViewportManager::setAddPickGeoPartData(set<int> partNames)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAddPickGeoPartData(partNames);
//		}
//	}
//
//	void mViewportManager::setReducePickGeoPointData(set<int> Pointids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setReducePickGeoPointData(Pointids);
//		}
//
//		//更新Point所在Part的选中信息
//		for (auto iter = Pointids.begin(); iter != Pointids.end(); ++iter)
//		{
//			mGeoPointData1* geoPointData = _geoData->getGeoPointDataByID(*iter);
//			if (geoPointData != nullptr)
//			{
//				mGeoPartData1* geoPartData = _geoData->getGeoPartDataByPartName(geoPointData->getPartID());
//				if (geoPartData != nullptr)
//				{
//					geoPartData->removePickGeoPointID(*iter);
//				}
//			}
//		}
//	}
//
//	void mViewportManager::setReducePickGeoLineData(set<int> Lineids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setReducePickGeoLineData(Lineids);
//		}
//
//		//更新Line所在Part的选中信息
//		for (auto iter = Lineids.begin(); iter != Lineids.end(); ++iter)
//		{
//			mGeoLineData1* geoLineData = _geoData->getGeoLineDataByID(*iter);
//			if (geoLineData != nullptr)
//			{
//				mGeoPartData1* geoPartData = _geoData->getGeoPartDataByPartName(geoLineData->getPartID());
//				if (geoPartData != nullptr)
//				{
//					geoPartData->removePickGeoLineID(*iter);
//				}
//			}
//		}
//	}
//
//	void mViewportManager::setReducePickGeoFaceData(set<int> Faceids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setReducePickGeoFaceData(Faceids);
//		}
//
//		//更新Face所在Part的选中信息
//		for (auto iter = Faceids.begin(); iter != Faceids.end(); ++iter)
//		{
//			mGeoFaceData1* geoFaceData = _geoData->getGeoFaceDataByID(*iter);
//			if (geoFaceData != nullptr)
//			{
//				mGeoPartData1* geoPartData = _geoData->getGeoPartDataByPartName(geoFaceData->getPartID());
//				if (geoPartData != nullptr)
//				{
//					geoPartData->removePickGeoFaceID(*iter);
//				}
//			}
//		}
//	}
//
//	void mViewportManager::setReducePickGeoSolidData(set<int> Solidids)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setReducePickGeoSolidData(Solidids);
//		}
//
//		//更新Solid所在Part的选中信息
//		for (auto iter = Solidids.begin(); iter != Solidids.end(); ++iter)
//		{
//			mGeoSolidData1* geoSolidData = _geoData->getGeoSolidDataByID(*iter);
//			if (geoSolidData != nullptr)
//			{
//				mGeoPartData1* geoPartData = _geoData->getGeoPartDataByPartName(geoSolidData->getPartID());
//				if (geoPartData != nullptr)
//				{
//					geoPartData->removePickGeoSolidID(*iter);
//				}
//			}
//		}
//	}
//
//	//void mViewportManager::setAllPickNodeData_Temp(const set<MXMeshVertex*>& vs)
//	//{
//	//	if (_meshData_temp) {
//	//		_meshData_temp->setAllPickNodeData(vs);
//	//	}
//	//}
//
//	//void mViewportManager::setAllPickMeshData_Temp(const set<MXMeshElement*>& mesh)
//	//{
//	//	if (_meshData_temp) {
//	//		_meshData_temp->setAllPickMeshData(mesh);
//	//	}
//	//	//getRendMeshData_Temp(mesh,QVector4D(0,0,1,1));
//	//	//updateRend_Temp();
//	//}
//
//	void mViewportManager::setAllPickLineData_Temp(const set<MEdge*>& lines)
//	{
//		if (_meshData_temp) {
//			_meshData_temp->setAllPickMeshLineData(lines);
//		}
//	}
//
//	void mViewportManager::setAllPickFaceData_Temp(const set<MFace*>& faces)
//	{
//		if (_meshData_temp) {
//			_meshData_temp->setAllPickMeshFaceData(faces);
//		}
//	}
//
//	void mViewportManager::updateRend_Temp()
//	{
//		shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//		render->getMxDataTempRender()->updateRender();
//		//修改面颜色
//		render->getMxDataTempRender()->setFaceDrawableColor(0, 0, 1);//111
//		//修改线颜色
//		render->getMxDataTempRender()->setLineDrawableColor(0, 0, 0);
//	}
//
//
//	void mViewportManager::setReducePickGeoPartData(set<int> partNames)
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setAllPickGeoPartData(partNames);
//		}
//	}
//
//	set<int> mViewportManager::getPickPointIDs()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return  _geoPickData->getPickPointIDs();
//		}
//		return  set<int>();
//	}
//
//	set<int> mViewportManager::getPickLineIDs()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return  _geoPickData->getPickLineIDs();
//		}
//		return  set<int>();
//	}
//
//	int mViewportManager::getOncePickPartData()
//	{
//		if (GetPickFilter() == (int)PickFilter::PickGeoPart) {
//			if (_geoPickData != nullptr)
//			{
//				auto ids = _geoPickData->getCurrentPickGeoPartIDs();
//				if (!ids.empty())
//					return *ids.begin();
//			}
//		}
//		else if (GetPickFilter() == (int)PickFilter::PickMeshPart) {
//			if (_meshPickData != nullptr)
//			{
//				auto ids = _meshPickData->getOncePickResultDataByType(MRendData::IMeshPickResultBase::PickInfoType::PartID);
//				if (!ids.empty())
//					return ids[0].part_id;
//			}
//		}
//		return -1;
//	}
//
//	set<int> mViewportManager::getPickFaceIDs()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return  _geoPickData->getPickFaceIDs();
//		}
//		return  set<int>();
//	}
//
//	set<int> mViewportManager::getPickSolidIDs()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return  _geoPickData->getPickSolidIDs();
//		}
//		return  set<int>();
//	}
//
//	std::set<int> mViewportManager::getPickGeoPartIDs()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return  _geoPickData->getPickGeoPartIDs();
//		}
//		return  set<int>();
//	}
//
//	QVector<int> mViewportManager::getPickPointIdsOrder()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return _geoPickData->getPickPointIDsOrder();
//		}
//		return  QVector<int>();
//	}
//
//	std::set<int> mViewportManager::getCurrentPickGeoLineIDs()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return _geoPickData->getCurrentPickGeoLineIDs();
//		}
//		return set<int>();
//	}
//	std::set<int> mViewportManager::getCurrentPickGeoPartIDs()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return _geoPickData->getCurrentPickGeoPartIDs();
//		}
//		return set<int>();
//	}
//	int mViewportManager::getGeoPickFunction()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return _geoPickData->getGeoPickFunction();
//		}
//		return 0;
//	}
//
//	QVector<QVector3D> mViewportManager::getPickVertexOnScreenDatas()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return  _geoPickData->getPickVertexOnScreenDatas();
//		}
//		return  QVector<QVector3D>();
//	}
//	QHash<int, QVector<QVector3D>> mViewportManager::getPickVertexOnGeoFaceDatas()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return  _geoPickData->getPickVertexOnGeoFaceDatas();
//		}
//		return  QHash<int, QVector<QVector3D>>();
//	}
//	QHash<int, QVector<QVector3D>> mViewportManager::getPickVertexOnGeoLineDatas()
//	{
//		if (_geoPickData != nullptr)
//		{
//			return  _geoPickData->getPickVertexOnGeoLineDatas();
//		}
//		return  QHash<int, QVector<QVector3D>>();
//	}
//	set<int> mViewportManager::getAllGeoLineIDs()
//	{
//		if (_geoData != nullptr)
//		{
//			return _geoData->getAllGeoLineIDs();
//		}
//		return set<int>();
//	}
//	set<int> mViewportManager::getAllGeoFaceIDs()
//	{
//		if (_geoData != nullptr)
//		{
//			return _geoData->getAllGeoFaceIDs();
//		}
//		return set<int>();
//	}
//	set<int> mViewportManager::getAllGeoSolidIDs()
//	{
//		if (_geoData != nullptr)
//		{
//			return _geoData->getAllGeoSolidIDs();
//		}
//		return set<int>();
//	}
//
//	QVector3D mViewportManager::getPointVertex(int id) const
//	{
//		auto pointData = _geoData->getGeoPointDataByID(id);
//		if (pointData)
//		{
//			auto partdata = _geoData->getGeoPartDataByPartName(pointData->getPartID());
//			auto& vertexArray = partdata->getVertexArray();
//			glm::vec3f vertex = vertexArray[pointData->getGeoPointVertexIndex()];
//			return QVector3D(vertex[0], vertex[1], vertex[2]);
//		}
//		return QVector3D();
//	}
//
//	//QVector<QVector3D> mViewportManager::getLinePointVertex(int id) const
//	//{
//	//	return _geoData->getGeoLineDataByID(id)->getGeoLineVertex();
//	//}
//
//	//QVector<QVector3D> mViewportManager::getFacePointVertex(int id) const
//	//{
//	//	return _geoData->getGeoFaceDataByID(id)->getGeoFaceVertex();
//	//}
//
//	std::set<int> mViewportManager::getSolidFaceIds(int id) const
//	{
//		return _geoData->getGeoSolidDataByID(id)->getGeoSolidFaceIDs();
//	}
//	void mViewportManager::setAllPickNodeData(const std::vector<RenderPickResultData>& nodes)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : nodes)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到节点对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked node objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->setPickResultDataByType(IMeshPickResultBase::PickInfoType::NodeID, result_data);
//		}
//	}
//	void mViewportManager::setAllPickMeshData(const std::vector<RenderPickResultData>& meshs)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : meshs)
//			{
//				if (itr_data.part_id == -1) continue;
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元对象或单元边对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked mesh or mesh face objects is:%1").arg(result_data.size()));
//			}
//
//			_meshPickData->setPickResultDataByType(IMeshPickResultBase::PickInfoType::MeshID, result_data);
//		}
//	}
//	void mViewportManager::setAllPickMeshLineData(const std::vector<RenderPickResultData>& meshlines)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : meshlines)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				data.result_id_2 = itr_data.result_id2;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元边对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked  mesh edge objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->setPickResultDataByType(IMeshPickResultBase::PickInfoType::MeshLineID, result_data);
//		}
//	}
//	void mViewportManager::setAllPickMeshFaceData(const std::vector<RenderPickResultData>& meshfaces)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : meshfaces)
//			{
//				if (itr_data.part_id == -1) continue;
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到二维单元对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked Two-dimensional mesh objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->setPickResultDataByType(IMeshPickResultBase::PickInfoType::FaceID, result_data);
//		}
//	}
//	void mViewportManager::setAllPickMeshPartData(const std::vector<RenderPickResultData>& partNames)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : partNames)
//			{
//				if (itr_data.part_id == -1) continue;
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到零件对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked part objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->setPickResultDataByType(IMeshPickResultBase::PickInfoType::PartID, result_data);
//
//			//MxFunctions::mGlobalSignals::getInstance()->setSelectNamasToBrowserTreeSig(partNames);
//		}
//	}
//	void mViewportManager::setAddPickNodeData(const std::vector<RenderPickResultData>& partNames)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : partNames)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到节点对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked node objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->addPickResultDataByType(IMeshPickResultBase::PickInfoType::NodeID, result_data);
//		}
//	}
//	void mViewportManager::setAddPickMeshData(const std::vector<RenderPickResultData>& partNames)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : partNames)
//			{
//				if (itr_data.part_id == -1) continue;
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元或者二维单元表面对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked mesh or two-dimensional face objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->addPickResultDataByType(IMeshPickResultBase::PickInfoType::MeshID, result_data);
//		}
//	}
//	void mViewportManager::setReducePickPartData(const std::vector<int>& partIDs)
//	{
//		std::set<int> partNames;
//		// 取消网格部件高亮
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto id : partIDs)
//			{
//				partNames.insert(id);
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = id;
//				data.part_id = id;
//				result_data.push_back(data);
//			}
//			_meshPickData->reducePickResultDataByType(IMeshPickResultBase::PickInfoType::PartID, result_data);
//		}
//
//		// 取消几何部件高亮
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->setReducePickGeoPartData(partNames);
//		}
//	}
//	void mViewportManager::setAddPickMeshLineData(const std::vector<RenderPickResultData>& partNames)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : partNames)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				data.result_id_2 = itr_data.result_id2;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元边对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked mesh edge objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->addPickResultDataByType(IMeshPickResultBase::PickInfoType::MeshLineID, result_data);
//		}
//	}
//	bool mViewportManager::isPickedPart(int partID)
//	{
//		bool rel = false;
//		// 取消网格部件高亮
//		if (_meshPickData != nullptr)
//		{
//			const auto& results = _meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::PartID);
//			for (const auto& result : results)
//			{
//				if (partID == result.part_id || partID == result.result_id || partID == result.result_id_2)
//				{
//					rel |= 1;
//					break;
//				}
//			}
//		}
//
//		// 取消几何部件高亮
//		if (_geoPickData != nullptr)
//		{
//			const auto& partIDs = _geoPickData->getPickGeoPartIDs();
//			for (const auto& part : partIDs)
//			{
//				if (partID == part)
//				{
//					rel |= 1;
//					break;
//				}
//			}
//		}
//		return rel;
//	}
//	void mViewportManager::setAddPickMeshFaceData(const std::vector<RenderPickResultData>& partNames)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : partNames)
//			{
//				if (itr_data.part_id == -1) continue;
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元表面对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked mesh  face objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->addPickResultDataByType(IMeshPickResultBase::PickInfoType::FaceID, result_data);
//		}
//	}
//	void mViewportManager::setAddPickMeshPartData(const std::vector<RenderPickResultData>& partNames)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : partNames)
//			{
//				if (itr_data.part_id == -1) continue;
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到零件对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked part objects is:%1").arg(result_data.size()));
//			}
//			_meshPickData->addPickResultDataByType(IMeshPickResultBase::PickInfoType::PartID, result_data);
//		}
//	}
//	void mViewportManager::setReducePickNodeData(const std::vector<RenderPickResultData>& nodes)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : nodes)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			
//			_meshPickData->reducePickResultDataByType(IMeshPickResultBase::PickInfoType::NodeID, result_data);
//		}
//	}
//
//	void mViewportManager::setReducePickNodeOrderData(const std::vector<RenderPickResultData>& nodes)
//	{
//		setReducePickNodeData(nodes);
//	}
//
//	void mViewportManager::setReducePickMeshData(const std::vector<RenderPickResultData>& meshs)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : meshs)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			
//			_meshPickData->reducePickResultDataByType(IMeshPickResultBase::PickInfoType::MeshID, result_data);
//		}
//	}
//	void mViewportManager::setReducePickMeshLineData(const std::vector<RenderPickResultData>& meshlines)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : meshlines)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				data.result_id_2 = itr_data.result_id2;
//				result_data.push_back(data);
//			}
//			
//			_meshPickData->reducePickResultDataByType(IMeshPickResultBase::PickInfoType::MeshLineID, result_data);
//		}
//	}
//	void mViewportManager::setReducePickMeshFaceData(const std::vector<RenderPickResultData>& meshfaces)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : meshfaces)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			
//			_meshPickData->reducePickResultDataByType(IMeshPickResultBase::PickInfoType::FaceID, result_data);
//		}
//	}
//
//	void mViewportManager::setReducePickMeshPartData(const std::vector<RenderPickResultData>& partNames)
//	{
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> result_data;
//			for (auto itr_data : partNames)
//			{
//				IMeshPickResultBase::PickResultData data;
//				data.result_id = itr_data.result_id;
//				data.part_id = itr_data.part_id;
//				result_data.push_back(data);
//			}
//			_meshPickData->reducePickResultDataByType(IMeshPickResultBase::PickInfoType::PartID, result_data);
//		}
//	}
//
//	void mViewportManager::setPickFilter(PickFilter pickfilter)
//	{
//		if (buttonFlag)
//		{
//			emit buttonType();
//		}
//		if (_preport.first)
//		{
//			_preport.first->setPickFilter(pickfilter);
//		}
//	}
//	void mViewportManager::SetPickFilter(int Index)
//	{
//		if (_preport.first != nullptr)
//		{
//			_preport.first->setPickFilter((PickFilter)Index);
//		}
//	}
//
//	PickFilter mViewportManager::getPickFilter()
//	{
//		return _preport.first->getPickFilter();
//	}
//
//	intptr_t mViewportManager::GetPickFilter()
//	{
//		return (intptr_t)_preport.first->getPickFilter();
//
//	}
//
//	void mViewportManager::SetMutiplyPickStatus(bool M)
//	{
//		if (_preport.first == nullptr)
//		{
//			return;
//		}
//		if (M)
//		{
//			_preport.first->setMultiplyPickMode(_pickMode);
//		}
//		else
//		{
//			_preport.first->setMultiplyPickMode(MultiplyPickMode::NoPick);
//		}
//		//MultiplyPickMode mult = *_preport.first->getMultiplyPickMode();
//
//	}
//
//	void mViewportManager::setMultiplyPickMode(MultiplyPickMode multiplyPickMode)
//	{
//		if (_preport.first == nullptr)
//		{
//			return;
//		}
//
//		_pickMode = multiplyPickMode;
//		_preport.first->setMultiplyPickMode(multiplyPickMode);
//
//	}
//
//	void mViewportManager::SetMultiplyPickMode(int Index)
//	{
//		if (_preport.first != nullptr)
//		{
//			_pickMode = (MultiplyPickMode)Index;
//			_preport.first->setMultiplyPickMode((MultiplyPickMode)Index);
//		}
//	}
//
//	MultiplyPickMode mViewportManager::getMultiplyPickMode()
//	{
//		return _preport.first->getMultiplyPickMode();
//	}
//
//	intptr_t mViewportManager::GetMultiplyPickMode()
//	{
//		return (intptr_t)_preport.first->getMultiplyPickMode();
//	}
//
//	void mViewportManager::SetPickAngle(double angle)
//	{
//		if (_preport.first)
//		{
//			_preport.first->setPickAngle(angle);
//		}
//	}
//
//	void mViewportManager::updateGeoPickRender()
//	{
//		if (_preport.first)
//		{
//
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->updateHighLightRender();
//			}
//		}
//
//
//	}
//
//	void mViewportManager::updatePickRender()
//	{
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->updateHighLightRender();
//			}
//		}
//	}
//
//	void mViewportManager::clearAllPickData()
//	{
//		clearAllGeoPickData();
//		clearAllMeshPickData();
//	}
//
//	void mViewportManager::clearAllGeoPickData()
//	{
//		if (_geoPickData != nullptr)
//		{
//			_geoPickData->clearAllPickData();
//		}
//		if (_geoData != nullptr)
//		{
//			_geoData->clearAllPartPickData();
//		}
//	}
//
//	void mViewportManager::clearAllMeshPickData()
//	{
//		if (_meshPickData != nullptr)
//		{
//			_meshPickData->clearAllPickData();
//		}
//	}
//
//	void mViewportManager::clearPickNodeData()
//	{
//		if (_meshPickData != nullptr)
//		{
//			_meshPickData->clearPickResultDataByType(IMeshPickResultBase::PickInfoType::NodeID);
//		}
//	}
//
//	void mViewportManager::clearPickMeshData()
//	{
//		if (_meshPickData != nullptr)
//		{
//			_meshPickData->clearPickResultDataByType(IMeshPickResultBase::PickInfoType::MeshID);
//		}
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getPickNodeIDs()
//	{
//		std::vector<RenderPickResultData> result_data;
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> pick_datas =
//				_meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::NodeID);
//			pick_datas.reserve(pick_datas.size());
//			for (auto pick_data : pick_datas)
//			{
//				RenderPickResultData data;
//				data.part_id = pick_data.part_id;
//				data.result_id = pick_data.result_id;
//				data.pick_time_stamp = pick_data.pick_time_stamp;
//				result_data.push_back(data);
//			}
//		}
//		if ((getPickFilter() == PickFilter::PickNode) || (getPickFilter() == PickFilter::PickNodeByFace) || (getPickFilter() == PickFilter::PickNodeByLine) || (getPickFilter() == PickFilter::PickNodeByPart) || (getPickFilter() == PickFilter::PickNodeOrder) || (getPickFilter() == PickFilter::PickNodeByLineAngle) || (getPickFilter() == PickFilter::PickNodeByFaceAngle))
//		{
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到节点对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked node objects is:%1").arg(result_data.size()));
//			}
//		}
//		
//
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getPickNodeIDsOrder()
//	{
//		std::vector<RenderPickResultData> result_data;
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> pick_datas =
//				_meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::NodeID);
//			pick_datas.reserve(pick_datas.size());
//			for (auto pick_data : pick_datas)
//			{
//				RenderPickResultData data;
//				data.part_id = pick_data.part_id;
//				data.result_id = pick_data.result_id;
//				data.pick_time_stamp = pick_data.pick_time_stamp;
//				result_data.push_back(data);
//			}
//		}
//		std::sort(result_data.begin(), result_data.end(), [](const RenderPickResultData& p1, const RenderPickResultData& p2) {
//			return p1.pick_time_stamp < p2.pick_time_stamp;
//		});
//		if ((getPickFilter() == PickFilter::PickNodeOrder))
//		{
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到节点对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked node objects is:%1").arg(result_data.size()));
//			}
//		}			
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getPickMeshIDs()
//	{
//		std::vector<RenderPickResultData> result_data;
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> pick_datas =
//				_meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::MeshID);
//			pick_datas.reserve(pick_datas.size());
//			for (auto pick_data : pick_datas)
//			{
//				RenderPickResultData data;
//				data.part_id = pick_data.part_id;
//				data.result_id = pick_data.result_id;
//				data.pick_time_stamp = pick_data.pick_time_stamp;
//				result_data.push_back(data);
//			}
//		}
//		if ((getPickFilter() == PickFilter::PickMeshElement) || (getPickFilter() == PickFilter::PickMeshByFace) || (getPickFilter() == PickFilter::PickMeshByLine) || (getPickFilter() == PickFilter::PickAnyMeshByPart) || (getPickFilter() == PickFilter::PickMeshByFace) || (getPickFilter() == PickFilter::PickMeshByLine))
//		{
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked mech objects is:%1").arg(result_data.size()));
//			}
//		}
//		
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getPickPartIDs()
//	{
//		std::vector<RenderPickResultData> result_data;
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> pick_datas =
//				_meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::PartID);
//			pick_datas.reserve(pick_datas.size());
//			for (auto pick_data : pick_datas)
//			{
//				RenderPickResultData data;
//				data.part_id = pick_data.part_id;
//				data.result_id = pick_data.result_id;
//				data.pick_time_stamp = pick_data.pick_time_stamp;
//				result_data.push_back(data);
//			}
//		}
//		if ((getPickFilter() == PickFilter::PickMeshPart) || (getPickFilter() == PickFilter::PickMeshLineByPart))
//		{
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到部件对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked part objects is:%1").arg(result_data.size()));
//			}
//		}
//		
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getPickMeshLineIDs()
//	{
//		std::vector<RenderPickResultData> result_data;
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> pick_datas =
//				_meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::MeshLineID);
//			pick_datas.reserve(pick_datas.size());
//			for (auto pick_data : pick_datas)
//			{
//				RenderPickResultData data;
//				data.part_id = pick_data.part_id;
//				data.result_id = pick_data.result_id;
//				data.pick_time_stamp = pick_data.pick_time_stamp;
//				data.result_id2 = pick_data.result_id_2;
//				result_data.push_back(data);
//			}
//		}
//		if ((getPickFilter() == PickFilter::PickMeshLine) || (getPickFilter() == PickFilter::PickMeshLineByLine) || (getPickFilter() == PickFilter::PickMeshLineByPart))
//		{
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元线对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked mesh line objects is:%1").arg(result_data.size()));
//			}
//		}
//		
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getPickMeshFaceIDs()
//	{
//		std::vector<RenderPickResultData> result_data;
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> pick_datas =
//				_meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::FaceID);
//			pick_datas.reserve(pick_datas.size());
//			for (auto pick_data : pick_datas)
//			{
//				RenderPickResultData data;
//				data.part_id = pick_data.part_id;
//				data.result_id = pick_data.result_id;
//				data.pick_time_stamp = pick_data.pick_time_stamp;
//				result_data.push_back(data);
//			}
//		}
//		if ((getPickFilter() == PickFilter::PickMeshFace) || (getPickFilter() == PickFilter::PickMeshFaceByFace) || (getPickFilter() == PickFilter::PickMeshFaceByPart))
//		{
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元面对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked mesh face objects is:%1").arg(result_data.size()));
//			}
//		}
//		
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getPickMeshShellIDs()
//	{
//		std::vector<RenderPickResultData> result_data;
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> pick_datas =
//				_meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::ShellID);
//			pick_datas.reserve(pick_datas.size());
//			for (auto pick_data : pick_datas)
//			{
//				RenderPickResultData data;
//				data.part_id = pick_data.part_id;
//				data.result_id = pick_data.result_id;
//				data.pick_time_stamp = pick_data.pick_time_stamp;
//				result_data.push_back(data);
//			}
//		}
//		if ((getPickFilter() == PickFilter::Pick2DMesh) || (getPickFilter() == PickFilter::PickAll2DMeshByGeoFace)||(getPickFilter() == PickFilter::PickAll2DMeshByPart))
//		{
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到二维单元对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked Two-dimensional mesh objects is:%1").arg(result_data.size()));
//			}
//		}
//		
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getPickMeshPartNames()
//	{
//		std::vector<RenderPickResultData> result_data;
//		if (_meshPickData != nullptr)
//		{
//			std::vector<IMeshPickResultBase::PickResultData> pick_datas =
//				_meshPickData->getPickResultDataByType(IMeshPickResultBase::PickInfoType::PartID);
//			pick_datas.reserve(pick_datas.size());
//			for (auto pick_data : pick_datas)
//			{
//				RenderPickResultData data;
//				data.part_id = pick_data.part_id;
//				data.result_id = pick_data.result_id;
//				data.pick_time_stamp = pick_data.pick_time_stamp;
//				result_data.push_back(data);
//			}
//		}
//		if (getPickFilter() == PickFilter::PickMeshPart)
//		{
//			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("拾取到单元部件对象的个数为：%1").arg(result_data.size()));
//			}
//			else
//			{
//				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("The number of picked mesh part objects is:%1").arg(result_data.size()));
//			}
//		}
//		
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getCurrentOnceAddPickNodeIDs()
//	{
//		auto results = _meshPickData->getOncePickResultDataByType(MRendData::IMeshPickResultBase::PickInfoType::NodeID);
//		std::vector< RenderPickResultData> result_data;
//		for (const auto& r : results) {
//			RenderPickResultData a;
//			a.part_id = r.part_id;
//			a.result_id = r.result_id;
//			a.pick_time_stamp = r.pick_time_stamp;
//			result_data.push_back(a);
//		}
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getCurrentOnceAddMeshIDs()
//	{
//		auto results = _meshPickData->getOncePickResultDataByType(MRendData::IMeshPickResultBase::PickInfoType::MeshID);
//		std::vector< RenderPickResultData> result_data;
//		for (const auto& r : results) {
//			RenderPickResultData a;
//			a.part_id = r.part_id;
//			a.result_id = r.result_id;
//			a.pick_time_stamp = r.pick_time_stamp;
//			result_data.push_back(a);
//		}
//		return result_data;
//	}
//
//	std::vector<RenderPickResultData> mViewportManager::getCurrentOnceAddPartIDs()
//	{
//		auto results = _meshPickData->getOncePickResultDataByType(MRendData::IMeshPickResultBase::PickInfoType::PartID);
//		std::vector< RenderPickResultData> result_data;
//		for (const auto& r : results) {
//			RenderPickResultData a;
//			a.part_id = r.part_id;
//			a.result_id = r.result_id;
//			a.pick_time_stamp = r.pick_time_stamp;
//			result_data.push_back(a);
//		}
//		return result_data;
//	}
//
//	void mViewportManager::updateGeoRender()
//	{
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->update();
//			}
//		}
//	}
//
//	QVector3D mViewportManager::getScreenNormal()
//	{
//		QVector3D V3D;
//		if (_preport.first)
//		{
//			V3D = _preport.first->getCamera()->Eye() - _preport.first->getCamera()->Center();
//		}
//		return V3D;
//	}
//
//	QVector3D mViewportManager::getScreenParellel()
//	{
//		QVector3D V3D;
//		if (_preport.first)
//		{
//			QVector3D V3DEye = _preport.first->getCamera()->Eye() - _preport.first->getCamera()->Center();
//			QVector3D V3DUp = _preport.first->getCamera()->Up();
//			V3D = QVector3D::crossProduct(V3DEye, V3DUp);
//		}
//		return V3D;
//
//	}
//
//
//	void mViewportManager::setGeoPartColor(int partName, QVector3D partColor)
//	{
//		if (_geoData != nullptr)
//		{
//			_geoData->setGeoPartColor(partName, partColor / 255.0);
//		}
//	}
//
//	bool mViewportManager::getGeoPartVisual(int partName)
//	{
//		if (_geoData != nullptr)
//		{
//			return _geoData->getGeoPartVisual(partName) * 255;
//		}
//		return true;
//	}
//
//	void mViewportManager::deleteGeoPart(int partName)
//	{
//		if (_geoData != nullptr)
//		{
//			_geoData->deleteGeoPart(partName);
//		}
//	}
//
//	void mViewportManager::deleteAllGeoPart()
//	{
//		if (_geoData != nullptr)
//		{
//			_geoData->deleteAllGeoPart();
//		}
//	}
//
//	void mViewportManager::setAllGeoVisiable(bool is)
//	{
//		if (_geoData != nullptr)
//		{
//			_geoData->setAllGeoPartVisual(is);
//		}
//	}
//
//
//	void mViewportManager::setGeoLineProperty(set<int> geoLineIDs, int t)
//	{
//		_geoData->setGeoLineProperty(geoLineIDs, (MViewBasic::GeoLineProperty)t);
//	}
//
//
//
//	void mViewportManager::updateRender()
//	{
//		if (_preport.first)
//		{
//			_preport.first->update();
//		}
//	}
//
//	void mViewportManager::setGeoHidden(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID)
//	{
//		for (const auto& pID : pointID)
//			_geoData->getGeoPointDataByID(pID)->setIsHidden(true);
//		for (const auto& pID : lineID)
//			_geoData->getGeoLineDataByID(pID)->setIsHidden(true);
//		for (const auto& pID : faceID)
//			_geoData->getGeoFaceDataByID(pID)->setIsHidden(true);
//		for (const auto& pID : solidID)
//			_geoData->getGeoSolidDataByID(pID)->setIsHidden(true);
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->getGeoModelRender()->rebuildModelGeometry();
//			}
//			this->updateRender();
//		}
//	}
//
//	void mViewportManager::setGeoShow(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID)
//	{
//		for (const auto& pID : pointID)
//			_geoData->getGeoPointDataByID(pID)->setIsHidden(false);
//		for (const auto& pID : lineID)
//			_geoData->getGeoLineDataByID(pID)->setIsHidden(false);
//		for (const auto& pID : faceID)
//			_geoData->getGeoFaceDataByID(pID)->setIsHidden(false);
//		for (const auto& pID : solidID)
//			_geoData->getGeoSolidDataByID(pID)->setIsHidden(false);
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->getGeoModelRender()->rebuildModelGeometry();
//			}
//			this->updateRender();
//		}
//	}
//
//	void mViewportManager::setGeoShowAll()
//	{
//		if (_geoData == nullptr)
//			return;
//		QList<int> faceID = _geoData->getAllGeoFaceID();
//		QList<int> pointID = _geoData->getAllGeoPointID();
//		QList<int> lineID = _geoData->getAllGeoLineID();
//		QList<int> solidID = _geoData->getAllGeoSolidID();
//
//		for (const auto& pID : pointID)
//			_geoData->getGeoPointDataByID(pID)->setIsHidden(false);
//		for (const auto& pID : lineID)
//			_geoData->getGeoLineDataByID(pID)->setIsHidden(false);
//		for (const auto& pID : faceID)
//			_geoData->getGeoFaceDataByID(pID)->setIsHidden(false);
//		for (const auto& pID : solidID)
//			_geoData->getGeoSolidDataByID(pID)->setIsHidden(false);
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->getGeoModelRender()->rebuildModelGeometry();
//			}
//			this->updateRender();
//		}
//	}
//
//	void mViewportManager::setGeoShowReserve()
//	{
//		if (_geoData == nullptr)
//			return;
//		QList<int> faceID = _geoData->getAllGeoFaceID();
//		QList<int> pointID = _geoData->getAllGeoPointID();
//		QList<int> lineID = _geoData->getAllGeoLineID();
//		QList<int> solidID = _geoData->getAllGeoSolidID();
//
//		for (const auto& pID : pointID)
//		{
//			auto ptr = _geoData->getGeoPointDataByID(pID);
//			ptr->setIsHidden(!ptr->getIsHidden());
//		}
//		for (const auto& pID : lineID)
//		{
//			auto ptr = _geoData->getGeoLineDataByID(pID);
//			ptr->setIsHidden(!ptr->getIsHidden());
//		}
//		for (const auto& pID : faceID)
//		{
//			auto ptr = _geoData->getGeoFaceDataByID(pID);
//			ptr->setIsHidden(!ptr->getIsHidden());
//		}
//		for (const auto& pID : solidID)
//		{
//			auto ptr = _geoData->getGeoSolidDataByID(pID);
//			ptr->setIsHidden(!ptr->getIsHidden());
//		}
//
//		if (_preport.first)
//		{
//			shared_ptr<CCSMPreRender> render = _preport.first->getPreRender();
//			if (render != nullptr)
//			{
//				render->getGeoModelRender()->rebuildModelGeometry();
//			}
//			this->updateRender();
//		}
//	}
//
//	void mViewportManager::ResetOrthorCamera()
//	{
//		if (_preport.first)
//		{
//			this->updateRender();
//			_preport.first->slotResetOrthoAndCamera();
//			this->updateRender();
//		}
//	}
//
//	void mViewportManager::UpdateOrthorCamera()
//	{
//		if (_preport.first)
//		{
//			this->updateRender();
//			_preport.first->slotUpdateOrthoAndCamera();
//			this->updateRender();
//		}
//	}
//
//	void mViewportManager::renderOCtreeBoundingBox(QString boxname, double min[3], double max[3])
//	{
//		QVector3D pos = QVector3D(min[0], min[1], min[2]);
//		QVector3D normalx = QVector3D(1, 0, 0);
//		QVector3D normaly = QVector3D(0, 1, 0);
//		QVector3D normalz = QVector3D(0, 0, 1);
//		float lx = max[0] - min[0];
//		float ly = max[1] - min[1];
//		float lz = max[2] - min[2];
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		if (lableRendController_pre != nullptr)
//		{
//			lableRendController_pre->appendLableRendData_Box(boxname, pos, normalx, normaly, normalz, lx, ly, lz, QVector3D(229.0, 73.0, 73.0), true);
//		}
//	}
//
//	void mViewportManager::renderBoundingBox(QString boxname, double min[3], double max[3])
//	{
//		QVector3D pos = QVector3D(min[0], min[1], min[2]);
//		QVector3D normalx = QVector3D(1, 0, 0);
//		QVector3D normaly = QVector3D(0, 1, 0);
//		QVector3D normalz = QVector3D(0, 0, 1);
//		float lx = max[0] - min[0];
//		float ly = max[1] - min[1];
//		float lz = max[2] - min[2];
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		if (lableRendController_pre != nullptr)
//		{
//			lableRendController_pre->appendLableRendData_Box(boxname, pos, normalx, normaly, normalz, lx, ly, lz, QVector3D(74.0, 217.0, 91.0), true);
//		}
//	}
//
//	void mViewportManager::deleteRenderBoundingBox(QString& boxname)
//	{
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		if (lableRendController_pre != nullptr)
//		{
//			lableRendController_pre->deleteLableRendData_Box(boxname, true);
//		}
//	}
//
//	void mViewportManager::updateRenderBoundingBox(QString& boxname, bool state)
//	{
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		if (lableRendController_pre != nullptr)
//		{
//			lableRendController_pre->setLableRendShowState_Box(boxname, state, true);
//		}
//	}
//
//	void mViewportManager::renderSphere(QString name, QVector3D center, float radius)
//	{
//		shared_ptr<mCommonShapeRender> ShapeRender = mViewportManager::getInstance()->getPreViewport()->getCommonShapeRender();
//		if (ShapeRender != nullptr)
//		{
//			QString searchString = "thick";
//			// 找到 '-' 的位置
//			int dashIndex = name.indexOf('-');
//			if (dashIndex != -1) {
//				// 提取 '-' 后面的内容
//				QString afterDash = name.mid(dashIndex + 1);
//				// 检查提取的子字符串是否包含 "tihck"
//				if (afterDash.contains(searchString)) {
//					QVector4D rgb(74, 217, 91, 0.5 * 255);
//					ShapeRender->appendSphere(name, center, radius, rgb);
//					ShapeRender->setSphereIsShow(name, true);
//				}
//				else {
//					QVector4D rgb(0, 255, 0, 0.5 * 255);
//					ShapeRender->appendSphere(name, center, radius, rgb);
//					ShapeRender->setSphereIsShow(name, true);
//				}
//			}
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//	void mViewportManager::deleteRenderSphere(QString& name)
//	{
//		shared_ptr<mCommonShapeRender> ShapeRender = mViewportManager::getInstance()->getPreViewport()->getCommonShapeRender();
//		if (ShapeRender != nullptr)
//		{
//			QString searchString = "thick";
//			// 找到 '-' 的位置
//			int dashIndex = name.indexOf('-');
//			if (dashIndex != -1) {
//				// 提取 '-' 后面的内容
//				QString afterDash = name.mid(dashIndex + 1);
//				// 检查提取的子字符串是否包含 "tihck"
//				ShapeRender->deleteSphere(name);
//			}
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//	void mViewportManager::deleteRenderCylinder(QString& name)
//	{
//		shared_ptr<mCommonShapeRender> ShapeRender = mViewportManager::getInstance()->getPreViewport()->getCommonShapeRender();
//		if (ShapeRender != nullptr)
//		{
//			QString searchString = "thick";
//			// 找到 '-' 的位置
//			int dashIndex = name.indexOf('-');
//			if (dashIndex != -1) {
//				// 提取 '-' 后面的内容
//				QString afterDash = name.mid(dashIndex + 1);
//				// 检查提取的子字符串是否包含 "tihck"
//				ShapeRender->deleteCylinder(name);
//			}
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//	void mViewportManager::updateRenderSphere(QString& name, bool state)
//	{
//		shared_ptr<mCommonShapeRender> ShapeRender = mViewportManager::getInstance()->getPreViewport()->getCommonShapeRender();
//		if (ShapeRender != nullptr)
//		{
//			QString searchString = "thick";
//			// 找到 '-' 的位置
//			int dashIndex = name.indexOf('-');
//			if (dashIndex != -1) {
//				// 提取 '-' 后面的内容
//				QString afterDash = name.mid(dashIndex + 1);
//				// 检查提取的子字符串是否包含 "tihck"
//				ShapeRender->setSphereIsShow(name, state);
//			}
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//
//	void mViewportManager::renderSphere(QString name, QVector3D center, double maxlc, double minlc, float lc)
//	{
//		int R = 0, G = 0;
//		auto num = lc / maxlc;
//		R = num * 255;
//		G = 255 - R;
//		QVector4D color(R, G, 0.0, 255 * 1);
//
//		shared_ptr<mCommonShapeRender> ShapeRender = mViewportManager::getInstance()->getPreViewport()->getCommonShapeRender();
//		if (ShapeRender != nullptr)
//		{
//			ShapeRender->appendSphere(name, center, 0.2, color);
//			ShapeRender->setSphereIsShow(name, true);
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//	void mViewportManager::renderCylinder(QString name, QVector3D center, QVector3D height_half_dir, float radius)
//	{
//		shared_ptr<mCommonShapeRender> ShapeRender = mViewportManager::getInstance()->getPreViewport()->getCommonShapeRender();
//		if (ShapeRender != nullptr)
//		{
//			QString searchString = "thick";
//
//			// 找到 '-' 的位置
//			int dashIndex = name.indexOf('-');
//			if (dashIndex != -1) {
//				// 提取 '-' 后面的内容
//				QString afterDash = name.mid(dashIndex + 1);
//
//				// 检查提取的子字符串是否包含 "thick"
//				if (afterDash.contains(searchString)) {
//					QVector4D rgb(74, 217, 91, 0.5 * 255);
//					ShapeRender->appendCylinder(name, center, height_half_dir, radius, rgb);
//					ShapeRender->setCylinderIsShow(name, true);
//				}
//				else {
//					QVector4D rgb(0, 255, 0, 0.5 * 255);
//					ShapeRender->appendCylinder(name, center, height_half_dir, radius, rgb);
//					ShapeRender->setCylinderIsShow(name, true);
//				}
//			}
//
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//	void mViewportManager::updateRenderCylinder(QString& name, bool state)
//	{
//		shared_ptr<mCommonShapeRender> ShapeRender = mViewportManager::getInstance()->getPreViewport()->getCommonShapeRender();
//		if (ShapeRender != nullptr)
//		{
//			QString searchString = "thick";
//			// 找到 '-' 的位置
//			int dashIndex = name.indexOf('-');
//			if (dashIndex != -1) {
//				// 提取 '-' 后面的内容
//				QString afterDash = name.mid(dashIndex + 1);
//				// 检查提取的子字符串是否包含 "tihck"
//				ShapeRender->setCylinderIsShow(name, state);
//			}
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//	void mViewportManager::renderArrows(QString name, QVector3D pos, QVector3D dir)
//	{
//
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		if (lableRendController_pre != nullptr)
//		{
//			QVector<POS_DIR>pos_dir_vec;
//			POS_DIR pos_dir(pos, dir);
//			pos_dir_vec.push_back(pos_dir);
//			QVector3D color(255, 255, 0);
//			lableRendController_pre->appendLableRendData<MDataLable::Force>(name, pos_dir_vec, "x", 0.5, color, true, false, true);
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//	void mViewportManager::renderArrows(QString name, QVector3D pos, QVector3D dir, double lc, double maxlc, double minlc)
//	{
//		//green->red
//		int R = 0, G = 0;
//		auto num = lc / maxlc;
//		R = num * 255;
//		G = 255 - R;
//
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		if (lableRendController_pre != nullptr)
//		{
//			//QVector3D dir(0, 0, 1);
//			QVector<POS_DIR>pos_dir_vec;
//			POS_DIR pos_dir(pos, dir);
//			pos_dir_vec.push_back(pos_dir);
//			QVector3D color(R, G, 0.0);
//			const QString string = QString::number(lc, 'f', 3);
//			lableRendController_pre->appendLableRendData<MDataLable::Force>(name, pos_dir_vec, string, 1, color, true, false, true);
//		}
//		mViewportManager::getInstance()->getPreViewport()->update();
//	}
//
//	std::vector<QString> mViewportManager::getConnectPick(float x, float y)
//	{
//		// 拾取检测
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		auto _data = mViewportManager::getInstance()->getCurrentLableDataController_pre()->getLableData<Line_Connect_Implicit>();
//		auto _cam = mViewportManager::getInstance()->getPreViewport()->getCamera();
//		auto connectName = lableRendController_pre->GetLine_Connect_Explicit_ByMouse1<Line_Connect_Implicit>(
//			{ _cam->SCR_WIDTH(),_cam->SCR_HEIGHT() },
//			{ x,y },
//			_cam->Center() - _cam->Eye(), _cam->ProjectionMatrix(), _cam->ViewMatrix()
//			);
//
//		std::vector<QString> result;
//		for (auto name : connectName)
//		{
//			if (mViewportManager::getInstance()->getCurrentLableDataController_pre()->getLableData_ShowState<Line_Connect_Implicit>(name, Lable_Lable()))
//			{
//				result.push_back(name);
//			}
//		}
//		return result;
//	}
//	std::vector<QString> mViewportManager::getBindPick(float x, float y)
//	{
//		// 拾取检测
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		auto _data = mViewportManager::getInstance()->getCurrentLableDataController_pre()->getLableData<FaceToFace_Connect_Implicit>();
//		auto _cam = mViewportManager::getInstance()->getPreViewport()->getCamera();
//		auto connectName = lableRendController_pre->GetLine_Connect_Explicit_ByMouse2<FaceToFace_Connect_Implicit>(
//			{ _cam->SCR_WIDTH(),_cam->SCR_HEIGHT() },
//			{ x,y },
//			_cam->Center() - _cam->Eye(), _cam->ProjectionMatrix(), _cam->ViewMatrix()
//			);
//
//		std::vector<QString> result;
//		for (auto name : connectName)
//		{
//			if (mViewportManager::getInstance()->getCurrentLableDataController_pre()->getLableData_ShowState<FaceToFace_Connect_Implicit>(name, Lable_Lable()))
//			{
//				result.push_back(name);
//			}
//		}
//		return result;
//	}
//	std::vector<QString> mViewportManager::getContactPick(float x, float y)
//	{
//		// 拾取检测
//		shared_ptr<mLableRendController_pre> lableRendController_pre = mViewportManager::getInstance()->getCurrentLableRendController_pre();
//		auto _data = mViewportManager::getInstance()->getCurrentLableDataController_pre()->getLableData<Contact_i>();
//		auto _cam = mViewportManager::getInstance()->getPreViewport()->getCamera();
//		auto contactName = lableRendController_pre->GetLine_Connect_Explicit_ByMouse2<Contact_i>(
//			{ _cam->SCR_WIDTH(),_cam->SCR_HEIGHT() },
//			{ x,y },
//			_cam->Center() - _cam->Eye(), _cam->ProjectionMatrix(), _cam->ViewMatrix()
//			);
//
//		std::vector<QString> result;
//		for (auto name : contactName)
//		{
//			if (mViewportManager::getInstance()->getCurrentLableDataController_pre()->getLableData_ShowState<Contact_i>(name, Lable_Lable()))
//			{
//				result.push_back(name);
//			}
//		}
//		return result;
//	}
//	void mViewportManager::appendCommonFontQString(const QString& name, QVector<QVector3D> pos1, QVector<QString> txt, QVector3D mColor, float size)
//	{
//		_preport.first->getFontRender()->appendCommonFont(name, pos1, txt, mColor, size);
//	}
//
//	void mViewportManager::clearGeoCommonFont(QString name)
//	{
//		_preport.first->getFontRender()->clearCommonFont(name);
//	}
//
//	void mViewportManager::setGeoCommonFontIsShow(QString name, bool isShow)
//	{
//		_preport.first->getFontRender()->setCommonFontIsShow(name, isShow);
//	}
//	void mViewportManager::addScaleBar()
//	{
//		auto scaleBar = new mScaleBar(_preport.first);
//		//scaleBar->initScaleBar(1);
//		scaleBar->showScaleBar(true);//s设置显隐
//		scaleBar->setScaleBarUnit(2);//设置单位
//	}
//}
