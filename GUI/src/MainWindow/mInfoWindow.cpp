#include "mInfoWindow.h"
#include "mMenuWidget.h"
#include "mGlobalSignals.h"
#include "UiPalette.h"
#include "mTitleBar.h"
#include "mDockLayoutManager.h"
#include "ThemeManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include <QMessageBox>
//多体动力学面板
#include "mMotionControlInputPanel.h"
#include "mMotionElectroImportPanel.h"
#include "mMotionElectroExportPanel.h"
#include "mMotionHydraulicPanel.h"
#include "mMotionRLPanel.h"
#include "mMotionSensorPanel.h"
#include "mMotionControllerPanel.h"
#include "mMotionAutoFlexPanel.h"
#include "mMotionMechanismPanel.h"
#include "mMotionTransmissionPanel.h"
#include "mMotionMechElecHydPanel.h"
#include "mMotionBodyPanel.h"
#include "mMotionBodyBatchPanel.h"
#include "mMotionJointPanel.h"
#include "mMotionCouplingPanel.h"
#include "mMotionDrivePanel.h"
#include "mMotionScenarioPanel.h"
#include "mMotionAnimationPanel.h"
#include "mMotionLoadExportPanel.h"
#include "mMotionMarkerPanel.h"
#include "mMotionVectorForcePanel.h"
#include "mMotionContactPanel.h"
#include "mMotionSpringPanel.h"
#include "mMotionCurvePanel.h"

//曲线模块属性页注册表（editCurveNode 使用）
#include "CurvePages/mCurvePropPageManager.h"
using namespace PostCurve;

//曲线模块属性页注册表（editCurveNode 使用）
#include "CurvePages/mCurvePropPageManager.h"
using namespace PostCurve;

#include <QVBoxLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPointer>

#include "mCasePanel.h"
#include "mCaseSettingPanel.h"
#include "mCaseNonLinearStaticPanel.h"
#include "mCaseNonLinearDynamicPanel.h"
#include "mCaseNonLinearArcPanel.h"
#include "mCaseLinearStaticPanel.h"
#include "mCaseLinearBucklingPanel.h"
#include "mCaseSteadyHeatPanel.h"
#include "mCaseStructuralCouplingHeatPanel.h"
#include "mCaseTransientHeatPanel.h"
#include "mCaseTransientPanel.h"
#include "mCaseResponseSpectrumPanel.h"
#include "mCaseGenericPanel.h"
#include "mCaseRandomVibrationPanel.h"
#include "mCaseFrequencyPanel.h"
#include "mCaseASCouplingPanel.h"
#include "mCaseModalPanel.h"
#include "mSetPanel.h"
#include "mCoordinatePanel.h"
#include "mNodeCoordinatePanel.h"
#include "mStructuralOptimizationPanel.h"
#include "mNonStructuralQualityPanel.h"
#include "mOptimizationGoalPanel.h"
#include "mOptimizationDesignPanel.h"
#include "mOptimizationConstraintPanel.h"
#include "mRunnerOptimizationPanel.h"
#include "mRunnerOptimizationGoalPanel.h"
#include "mRunnerOptimizationDesignPanel.h"
#include "mRunnerOptimizationConstraintPanel.h"
#include "mfunctionpanel.h"
#include "mSectionShapePanel.h"
#include "mMaterialPanel.h"
#include "mPropertyPanel.h"
#include "mElementPanel.h"
#include "mPropertyCheckPanel.h"
#include "mSetManagerPanel.h"
#include "mGlobalDimPanel.h"
#include "mContactPairPanel.h"
#include "mContactPanel.h"
#include "mPeriodicBoundaryPanel.h"
#include "mASCouplingSearchPanel.h"
#include "mNsmManagerUnderCasePanel.h"
#include "mModelFixManaUnderCasePanel.h"
#include "mDiscreteFieldPanel.h"
#include "mAnalysisFieldPanel.h"

#include "mGenericParamPanel.h"
#include "mGenericAssignPanel.h"
#include "mGenericMatrixPanel.h"
#include "mGenericPartPanel.h"
#include "mSolveControlPanel.h"
#include "mSolveTaskPanel.h"
#include "mGlobalOutputControlPanel.h"


#include "mGeoFeaturePanel.h"
#include "mGeoTransformPanel.h"
#include "mGeoBoolOperationPanel.h"
#include "mWorkingConditionPanel.h"
#include "mGeoInfoPanel.h"
#include "mGeoEditPanel.h"
#include "mGeoRepairPanel.h"
#include "mGeoSharedTopologyPanel.h"
#include "mGeoSolidifyPanel.h"
#include "mGeoRemovalPanel.h"
#include "mGeoDirectModelingPanel.h"
#include "mGeoExtractPanel.h"
#include "mViewTreeFunction.h"


#include "mModelData.h"
#include "mCaseManager.h"
#include "mCaseBase.h"
#include "mWorkingConditions.h"

#include "mMaterialProps.h"
#include "mConnectorPannel.h"
#include "mMotionLoadImportPanel.h"
#include "mDeputyPannel.h"
#include "mConnectorSearchPannel.h"
#include "mWorkingDirPanel.h"


//#include "ControlPanel.h"
#include "mMeshPanel.h"
#include "mAutoAdaptPanel.h"
#include "mCustomEncryptionPanel.h"
#include "mEdgeSuppressPanel.h"
#include "mFaceMeshPanel.h"
#include "mLineMeshPanel.h"
#include "mLocalSizePanel.h"
#include "mMeshEditPanel.h"
#include "mCreateMeshPanel.h"
#include "mCreateMetaMeshPanel.h"
#include "mRotateMeshPanel.h"
#include "mExtractSurfacePanel.h"
#include "mSolidMeshPanel.h"
#include "mTranslationPanel.h"
#include "mRotationPanel.h"
#include "mStretchPanel.h"
#include "mMirroringPanel.h"
#include "mMatrixPanel.h"
#include "mZoomPanel.h"
#include "mChangeOrderPanel.h"
#include "mSharedNodePanel.h"

#include "mFreeEdgePanel.h"
#include "mTEdgePanel.h"
#include "mMeshIntersectPanel.h"
#include "mMeshOverlapPanel.h"
#include "mMeshInterferePanel.h"
#include "mMeshStatisticsPanel.h"
#include "mMoveMeshPanel.h"
#include "mQualityPanel.h"
#include "mSmoothPanel.h"
#include "mRemeshingPanel.h"
#include "mSTLRemeshPanel.h"
#include "mRenumberPanel.h"
#include "mMeshNormalPanel.h"
#include "mCreatePointPanel.h"
#include "mFreqSamplPointPanel.h"
#include "mLaminatePanel.h"
#include "mModalContriPanel.h"
#include "mNodeContriPanel.h"
#include "mPanelContriPanel.h"
#include "mTPContriPanel.h"
#include "mERPPanel.h"
#include "mPeakFilterPanel.h"
#include "mCasePeakFilterPanel.h"

#include "mEleSetCtrlPanel.h"

#include <mPBeam.h>
#include "mSectionShapeManager.h"
#include "mGeoSketchSignalManager.h"
#include "mBrowserWindow.h"
#include "mTreeView.h"
#include "mManagerPanelTpl.h"
#include "mReferencePanelTpl.h"
#include "mPanelTraitsImpl.h"

using MechSectionShapeData::mSectionShapeManager;   // 放在 include 下面

#include "mMeshPanelManager.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mDataElement.h"
#include "mElementBase.h"
using namespace MechSectionShapeData;
using namespace MechElementData;

namespace
{
	//管理器/引用面板成员以 shared_ptr<void> 存储（避免mInfoWindow.h包含模板定义），
	//此处在cpp内完成具体类型的懒加载创建与类型恢复
	template<typename PanelT>
	std::shared_ptr<PanelT> ensureTypedPanel(std::shared_ptr<void>& panel)
	{
		if (!panel)
			panel = std::make_shared<PanelT>();
		return std::static_pointer_cast<PanelT>(panel);
	}
}

namespace MechMainWindow
{
	mInfoWindow* mInfoWindow::_instance = nullptr;
    mInfoWindow* mInfoWindow::getInstance()
    {

        if (_instance == nullptr)
        {
            _instance = new mInfoWindow;
            //_instance->setTitleBarWidget(new TitleBarWidget(_instance, "参数"));
            _instance->setObjectName("InfoDock");
            //_instance->setMinimumWidth(150);

            // 平台经 mpcomm 下发的求解请求 -> 触发多体动力学求解
            QObject::connect(
                MechCommon::mMechGlobalSignals::getInstance(),
                &MechCommon::mMechGlobalSignals::mpcommStartSolveRequested,
                _instance,
                []()
                {
                    // 标记为平台触发的求解：求解面板据此不弹模态进度框，
                    // 避免后台求解弹窗与主界面状态冲突导致崩溃。
                    qApp->setProperty("mpcomm_solve_requested", true);
                    mInfoWindow::getInstance()->solveMotionScenario(0);
                });
        }
        return _instance;
    }

    mGeoInfoPanel* mInfoWindow::getGeoInfoPanel()
    {
        //外部调用方（几何修复/实体化面板）直接解引用，懒加载创建保证非空
        return ensurePanel(_geoInfoPanel).get();
    }

	//宿主底色 surface：objectName 规则只命中自身。不能用 QWidget 类型选择器
	//级联子孙——那会把所有无背景规则的普通容器（含折叠内容层）刷成写死白色，
	//盖住主题底色；子孙无规则时自然透底，透出本层 surface
	void mInfoWindow::applyHostTheme()
	{
		if (objectName().isEmpty())
			setObjectName(QStringLiteral("mInfoWindowHost"));
		setStyleSheet(QStringLiteral("#%1{ background-color:%2; }")
			.arg(objectName(), MxFunctions::UiPalette::surfaceHex()));
	}

	void mInfoWindow::connectEnable(QToolButton* btn, QToolButton* btn1)
	{
		createBtn = btn;
		deleteBtn = btn1;
	}

	void mInfoWindow::setMenuWidget(mMenuWidget* menuWidget)
	{
		//懒加载：面板可能尚未创建，先缓存，创建时再注入
		_menuWidget = menuWidget;
	}

    mInfoWindow::mInfoWindow(QWidget* parent) :QWidget(parent)
    {
        //注册参数面板宿主（PostWindow::mPostInfoHub经MechCommon::panelHostRef()取用）
        MechCommon::panelHostRef() = this;
		//设置允许的位置
		//this->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
		applyHostTheme();

		//切主题/切档随 ThemeManager 的 dock 补发链重发：宿主底色 applyHostTheme
		//原本只在构造执行一次，运行期切主题会残留旧色；内部折叠表格（FoldTable）
		//标题按钮的悬停/选中色同理。QPointer 防悬挂（本类随主窗存活，正常只注册一次）
		QPointer<mInfoWindow> hostGuard(this);
		ThemeManager::addDockThemeExtraApplier([hostGuard]()
		{
			if (!hostGuard)
				return;
			hostGuard->applyHostTheme();
			const auto tables = hostGuard->findChildren<FoldTable*>();
			for (FoldTable* t : tables)
				t->applyTheme();
		});

		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(0, 0, 0, 0);
		//layout->addWidget(_scrollArea);
		//_scrollArea->setWidget(new QWidget());
		this->setLayout(_mainLayout);
		//this->setMainWidget(new QWidget);

        //切换界面语言
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
        _actions = new QList<QAction*>;
        connect(mGlobalSignals::getInstance(), &mGlobalSignals::assignElemnetTypeBymeshEdit, this, [=](QHash<MeshType, vector<int>> eleTypeToeleId) {
            _eleTypeToeleId = eleTypeToeleId;
            });

        //连接写文件进程 写文件之前保存当前面板未保存的拾取数据
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::requestPrepareWriteFile,this, &mInfoWindow::commitActivePickWidget, Qt::BlockingQueuedConnection);

        //跨模型切换前统一收尾参数栏面板：信号发出时管理器尚未切换当前模型指针，
        //unEdit 链路（数据复核/拾取提交/折叠框卸载）全部按旧模型执行，语义正确。
        //DirectConnection 与拾取控件处理保持一致，保证在指针切换前同步完成
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::viewportModelAboutToChange,
            this, &mInfoWindow::slot_viewportModelAboutToChange, Qt::DirectConnection);

        //曲线页菜单动作（发送方为PostCurve模块的mCurveMenuContributor，经全局信号总线解耦）

    }

	void mInfoWindow::hideAll()
	{
	}

	QWidget* mInfoWindow::currentMainWidget() const
	{
		//setMainWidget 挂载时主部件总在第0项（其后仅跟一个弹簧项）
		QLayoutItem* item = (_mainLayout != nullptr && _mainLayout->count() > 0) ? _mainLayout->itemAt(0) : nullptr;
		return (item != nullptr) ? item->widget() : nullptr;
	}

	void mInfoWindow::setMainWidget(QWidget* w)
	{
		QLayoutItem* item;
		while ((item = _mainLayout->takeAt(0)) != nullptr)
		{
            if (item->widget())
            {
                //方案一
                item->widget()->hide();     // 这里必须加上隐藏，方便重写隐藏事件，用来替代没有的面板退出事件，方便信号断开
                item->widget()->setParent(nullptr);  // 移除旧 widget
            }
			delete item;
		}

		if (!w)
            return;

		// 设置 widget 的大小策略，确保充满整个布局
		w->setParent(this);  // 重要：父控件设为当前 mInfoWindow
		w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		w->show();

		// 添加到布局中
		_mainLayout->addWidget(w);
		_mainLayout->addSpacerItem(new QSpacerItem(10, 50, QSizePolicy::Minimum, QSizePolicy::Expanding));

		// 移除任何多余伸缩空间
		// _mainLayout->addStretch(0); // 不加伸缩，保证充满
		_mainLayout->setContentsMargins(0, 0, 0, 0);
		_mainLayout->setSpacing(0);

		// 更新布局
		this->updateGeometry();
		this->update();

        //方案二 强制立马刷新
        //{ QCoreApplication::processEvents(); }

        //方案三 "延迟刷新"
        //{
        //    if (m_refreshScheduled) return;
        //    m_refreshScheduled = true;

        //    QTimer::singleShot(0, infoWindow, [this] {
        //        m_refreshScheduled = false;
        //        infoWindow->setMainWidget(pageFor(tree->currentIndex()));
        //        });
        //}
	}

    void mInfoWindow::createOptimization()
    {
        ensurePanel(_mStructuralOptimization)->newStructuralOptimization();
    }

    void mInfoWindow::editOptimization()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mStructuralOptimization).get());
        ensurePanel(_mStructuralOptimization)->editStructuralOptimization();
    }

    void mInfoWindow::editOptimizationGoal(const QString& type)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_optimizationGoalPanel).get());
        ensurePanel(_optimizationGoalPanel)->editOptimizationGoalPanel(type);
    }

    void mInfoWindow::createOptimizationDesign(const QString& type)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_optimizationDesignPanel).get());
        ensurePanel(_optimizationDesignPanel)->createOptimizationDesign(type);
    }

    void mInfoWindow::editOptimizationDesign(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_optimizationDesignPanel).get());
        ensurePanel(_optimizationDesignPanel)->editOptimizationDesignPanel(id);
    }

    void mInfoWindow::createOptimizationConstraint(const QString& type)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_optimizationGoalPanel).get());
        ensurePanel(_optimizationConstraintPanel)->createOptimizationConstraint(type);
    }

    void mInfoWindow::editOptimizationConstraint(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_optimizationConstraintPanel).get());
        ensurePanel(_optimizationConstraintPanel)->editOptimizationConstraintPanel(id);
    }

    void mInfoWindow::unEditOptimization()
    {
        if (_optimizationGoalPanel && _optimizationGoalPanel->isEditing())
        	_optimizationGoalPanel->uneditOptimizationGoalPanel();
        if (_optimizationDesignPanel && _optimizationDesignPanel->isEditing())
        	_optimizationDesignPanel->uneditOptimizationDesignPanel();
        if (_optimizationConstraintPanel && _optimizationConstraintPanel->isEditing())
        	_optimizationConstraintPanel->uneditOptimizationConstraintPanel();
    }

    void mInfoWindow::createRunnerOptimization()
    {
        ensurePanel(_mRunnerOptimization)->newRunnerOptimization();
    }

    void mInfoWindow::editRunnerOptimization()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mRunnerOptimization).get());
        ensurePanel(_mRunnerOptimization)->editRunnerOptimization();
    }

    void mInfoWindow::editRunnerOptimizationGoal(const QString& type)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_runnerOptimizationGoalPanel).get());
        ensurePanel(_runnerOptimizationGoalPanel)->editRunnerOptimizationGoalPanel(type);
    }

    void mInfoWindow::createRunnerOptimizationDesign(const QString& type)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_runnerOptimizationDesignPanel).get());
        ensurePanel(_runnerOptimizationDesignPanel)->createRunnerOptimizationDesign(type);
    }

    void mInfoWindow::editRunnerOptimizationDesign(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_runnerOptimizationDesignPanel).get());
        ensurePanel(_runnerOptimizationDesignPanel)->editRunnerOptimizationDesignPanel(id);
    }

    void mInfoWindow::createRunnerOptimizationConstraint(const QString& type)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_runnerOptimizationConstraintPanel).get());
        ensurePanel(_runnerOptimizationConstraintPanel)->createRunnerOptimizationConstraint(type);
    }

    void mInfoWindow::editRunnerOptimizationConstraint(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_runnerOptimizationConstraintPanel).get());
        ensurePanel(_runnerOptimizationConstraintPanel)->editRunnerOptimizationConstraintPanel(id);
    }

    void mInfoWindow::unEditRunnerOptimization()
    {
        if (_runnerOptimizationGoalPanel && _runnerOptimizationGoalPanel->isEditing())
        	_runnerOptimizationGoalPanel->uneditRunnerOptimizationGoalPanel();
        if (_runnerOptimizationDesignPanel && _runnerOptimizationDesignPanel->isEditing())
        	_runnerOptimizationDesignPanel->uneditRunnerOptimizationDesignPanel();
        if (_runnerOptimizationConstraintPanel && _runnerOptimizationConstraintPanel->isEditing())
        	_runnerOptimizationConstraintPanel->uneditRunnerOptimizationConstraintPanel();
    }

    void mInfoWindow::createGenericParam()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mGenericParamPanel).get());
        ensurePanel(_mGenericParamPanel)->createGenericParam();
    }

    void mInfoWindow::editGenericParam(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mGenericParamPanel).get());
        ensurePanel(_mGenericParamPanel)->editGenericParamPanel(id);
    }

    void mInfoWindow::createGenericAssign()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mGenericAssignPanel)->createGenericAssign();
    }

    void mInfoWindow::editGenericAssign()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mGenericAssignPanel).get());
        ensurePanel(_mGenericAssignPanel)->editGenericAssignPanel();
    }

    void mInfoWindow::createASCouplingSearch()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mASCouplingSearchPanel).get());
        ensurePanel(_mASCouplingSearchPanel)->createASCouplingSearch();
    }

    void mInfoWindow::editASCouplingSearchPanel()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mASCouplingSearchPanel).get());
        ensurePanel(_mASCouplingSearchPanel)->editASCouplingSearchPanel();
    }

    void mInfoWindow::createGenericMatrix()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mGenericMatrixPanel)->createGenericMatrix();
    }

    void mInfoWindow::editGenericMatrix()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mGenericMatrixPanel).get());
        ensurePanel(_mGenericMatrixPanel)->editGenericMatrixPanel();
    }

    void mInfoWindow::createSolveControl()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mSolveControlPanel).get());
        ensurePanel(_mSolveControlPanel)->createSolveControl();
    }

    void mInfoWindow::editSolveControl()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mSolveControlPanel).get());
        ensurePanel(_mSolveControlPanel)->editSolveControlPanel();
    }

    void mInfoWindow::showSolveTaskPanel()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mSolveTaskPanel).get());
        ensurePanel(_mSolveTaskPanel)->refreshPanel();
    }

    void mInfoWindow::createGlobalOutputControl()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mGlobalOutputControlPanel).get());
        ensurePanel(_mGlobalOutputControlPanel)->createGlobalOutputControlPanel();
        ensurePanel(_mGlobalOutputControlPanel)->editGlobalOutputControlPanel();
   }

    void mInfoWindow::editGlobalOutputControl()
   {
       mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mGlobalOutputControlPanel).get());
        ensurePanel(_mGlobalOutputControlPanel)->editGlobalOutputControlPanel();
   }

 void mInfoWindow::createModalContri(int id)
	{
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mModalContriPanel)->createModalContri(id);
	}

    void mInfoWindow::editModalContri(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mModalContriPanel).get());
        ensurePanel(_mModalContriPanel)->editModalContri(id);
    }

    void mInfoWindow::createNodeContri(int id)
	{
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mNodeContriPanel)->createNodeContri(id);
	}

    void mInfoWindow::editNodeContri(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mNodeContriPanel).get());
        ensurePanel(_mNodeContriPanel)->editNodeContri(id);
    }

    void mInfoWindow::createPanelContri(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mPanelContriPanel)->createPanelContri(id);
    }

    void mInfoWindow::editPanelContri(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mPanelContriPanel).get());
        ensurePanel(_mPanelContriPanel)->editPanelContri(id);
    }

    void mInfoWindow::createTPContri(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mTPContriPanel)->createTPContri(id);
    }

    void mInfoWindow::editTPContri(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mTPContriPanel).get());
        ensurePanel(_mTPContriPanel)->editTPContri(id);
    }

    void mInfoWindow::createERP(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mERPPanel)->createERP(id);
    }

    void mInfoWindow::editERP(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mERPPanel).get());
        ensurePanel(_mERPPanel)->editERP(id);
    }

    void mInfoWindow::createPeakFilter()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mPeakFilterPanel)->createPeakFilter();
    }

    void mInfoWindow::editPeakFilter(int id)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mPeakFilterPanel).get());
        ensurePanel(_mPeakFilterPanel)->editPeakFilter(id);
    }

    void mInfoWindow::createCasePeakFilter(int caseId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mCasePeakFilterPanel).get());
        ensurePanel(_mCasePeakFilterPanel)->createCasePeakFilter(caseId);
    }

    void mInfoWindow::editCasePeakFilter(int caseId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mCasePeakFilterPanel).get());
        ensurePanel(_mCasePeakFilterPanel)->editCasePeakFilter(caseId);
    }

    void mInfoWindow::createGenericPart()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        ensurePanel(_mGenericPartPanel)->createGenericPart();
    }

    void mInfoWindow::editGenericPart()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mGenericPartPanel).get());
        ensurePanel(_mGenericPartPanel)->editGenericPartPanel();
    }

	void mInfoWindow::createCase(MxFunctions::CaseType type, int caseId)
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		std::shared_ptr<mCaseSettingPanel> caseSettingPanel = nullptr;
		switch (type)
		{
		case ImplicitLinearStatic:
			caseSettingPanel = std::make_shared<mCaseLinearStaticPanel>(); break;
		case ImplicitNonLinearStatic:
			caseSettingPanel = std::make_shared<mCaseNonLinearStaticPanel>(); break;
		case ImplicitModal:
			caseSettingPanel = std::make_shared<mCaseModalPanel>(); break;
        case ImplicitResponseSpectrum:
            caseSettingPanel = std::make_shared<mCaseResponseSpectrumPanel>(); break;
        case ImplicitRandomVibration:
            caseSettingPanel = std::make_shared<mCaseRandomVibrationPanel>(); break;
		case ImplicitFrequency:
			caseSettingPanel = std::make_shared<mCaseFrequencyPanel>(); break;
        case ImplicitTransient:
			caseSettingPanel = std::make_shared<mCaseTransientPanel>(); break;
		case ImplicitNonLinearDynamic:
			caseSettingPanel = std::make_shared<mCaseNonLinearDynamicPanel>(); break;
        case ASCoupling:
            caseSettingPanel = std::make_shared<mCaseASCouplingPanel>(); break;
        case ImplicitGeneric:
            caseSettingPanel = std::make_shared<mCaseGenericPanel>(); break;
		case ImplicitLinearBuckling:
			caseSettingPanel = std::make_shared<mCaseLinearBucklingPanel>(); break;
		case ImplicitPostBuckling:
			caseSettingPanel = std::make_shared<mCaseNonLinearArcPanel>(); break;
		case ImplicitSteadyHeat:
			caseSettingPanel = std::make_shared<mCaseSteadyHeatPanel>(); break;
		case ImplicitTransientHeat:
			caseSettingPanel = std::make_shared<mCaseTransientHeatPanel>(); break;
        case ImplicitStructuralCouplingHeat:
            caseSettingPanel = std::make_shared<mCaseStructuralCouplingHeatPanel>(); break;
		default:
			break;
		}

		if (caseSettingPanel == nullptr)
		{
			caseSettingPanel = std::make_shared<mCaseSettingPanel>();
		}
		ensurePanel(_casePanel)->setCaseData("xx分析", type);
		_caseSettingPanels.append(caseSettingPanel);
		//新建数据
		if (caseId == 0)
		{
			caseSettingPanel->newCasePanel();
		}
		else
		{
			caseSettingPanel->newCasePanel(caseId);
		}

		this->setMainWidget(caseSettingPanel.get());
		caseSettingPanel->updateMapRelationData();//从其他模块更新数据
	}

	void mInfoWindow::editCase(int caseId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		for (auto caseSettingPanel : _caseSettingPanels)
		{
			if (caseId == caseSettingPanel->getCaseID())
			{
				this->setMainWidget(caseSettingPanel.get());
				caseSettingPanel->updateMapRelationData();//从其他模块更新数据
				return;
			}
		}
		
		//面板不存在
		MechCaseData::mCaseBase* caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseId);
		if (caseBase == nullptr)
			return;
		auto caseType = caseBase->getCaseType();
		createCase(caseType, caseId);
	}

	void mInfoWindow::deleteCase(int caseId)
	{
		unEditInfoWindow();
		for (int i = 0;i < _caseSettingPanels.size(); ++i)
		{
			if (!_caseSettingPanels[i])
			{
				continue;
			}
			if (caseId == _caseSettingPanels[i]->getCaseID())
			{
				//面板为共享指针缓存，移除前先退出其编辑态，避免残留 isEditing 标志
				_caseSettingPanels[i]->uneditCasePanel();
				_caseSettingPanels.removeAt(i);
				return;
			}
		}
	}

	void mInfoWindow::createLoad(MxFunctions::LoadBoundaryType type)
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_workingConditionPanel).get());
		ensurePanel(_workingConditionPanel)->createLoad(type);
	}

	void mInfoWindow::createBoundary(MxFunctions::LoadBoundaryType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_workingConditionPanel).get());
		ensurePanel(_workingConditionPanel)->createBoundary(type);
	}

	void mInfoWindow::createPredefinedField(MxFunctions::PredefinedFieldType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_workingConditionPanel).get());
		ensurePanel(_workingConditionPanel)->createPredefinedField(type);
	}

	void mInfoWindow::createRotateAnalyse(MxFunctions::RotateAnalysyType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_workingConditionPanel).get());
		ensurePanel(_workingConditionPanel)->createRotateAnalyse(type);
	}

	void mInfoWindow::editLoad(int loadId, int caseId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_workingConditionPanel).get());
		ensurePanel(_workingConditionPanel)->editLoad(loadId,caseId);
	}

	void mInfoWindow::editBoundary(int boundaryId, int caseId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_workingConditionPanel).get());
		ensurePanel(_workingConditionPanel)->editBoundary(boundaryId,caseId);
	}

	void mInfoWindow::editPredefinedField(int fieldId, int caseId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_workingConditionPanel).get());
		ensurePanel(_workingConditionPanel)->editPredefinedField(fieldId, caseId);
	}

	void mInfoWindow::editRotateAnalyse(int rotateId, int caseId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_workingConditionPanel).get());
		ensurePanel(_workingConditionPanel)->editRotateAnalyse(rotateId, caseId);
	}

	void mInfoWindow::unEditLoad()
	{
		if (_workingConditionPanel && _workingConditionPanel->isEditing())
			_workingConditionPanel->unEditLoad();
	}

	void mInfoWindow::unEditBoundary()
	{
		if (_workingConditionPanel && _workingConditionPanel->isEditing())
			_workingConditionPanel->unEditBoundary();
	}

    void mInfoWindow::createDiscreteField()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mDiscreteFieldPanel).get());
        ensurePanel(_mDiscreteFieldPanel)->newDiscreteFieldPanel();
    }

    void mInfoWindow::createDiscreteField(MxFunctions::DiscreteSpace spaceType)
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mDiscreteFieldPanel).get());
        ensurePanel(_mDiscreteFieldPanel)->newDiscreteFieldPanel(spaceType);
    }

    void mInfoWindow::editDiscreteField(int discreteFieldId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mDiscreteFieldPanel).get());
        ensurePanel(_mDiscreteFieldPanel)->editDiscreteFieldPanel(discreteFieldId);
    }

    void mInfoWindow::createAnalysisField()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mAnalysisFieldPanel).get());
        ensurePanel(_mAnalysisFieldPanel)->newAnalysisFieldPanel();
    }

    void mInfoWindow::editAnalysisField(int analysisFieldId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mAnalysisFieldPanel).get());
        ensurePanel(_mAnalysisFieldPanel)->editAnalysisFieldPanel(analysisFieldId);
    }

    void mInfoWindow::uneditAnalysisField()
    {
        if (_mAnalysisFieldPanel && _mAnalysisFieldPanel->isEditing())
            _mAnalysisFieldPanel->uneditAnalysisFieldPanel();
    }

    void mInfoWindow::notifyFieldDataAboutToBeDestroyed(int fieldID)
    {
        if (_mAnalysisFieldPanel)
            _mAnalysisFieldPanel->onFieldDataAboutToBeDestroyed(fieldID);
    }

    void mInfoWindow::notifyContactDataAboutToBeDestroyed(int contactID)
    {
        if (_contactPanel)
            _contactPanel->onContactDataAboutToBeDestroyed(contactID);
    }

    void mInfoWindow::notifyPeriodicBoundaryDataAboutToBeDestroyed(int pbID)
    {
        if (_mPeriodicBoundaryPanel)
            _mPeriodicBoundaryPanel->onPeriodicBoundaryDataAboutToBeDestroyed(pbID);
    }

    //力元（含衬套）数据即将被删除：树删除前先置空面板编辑指针，防 dynamic_cast
    //读到已释放对象抛 __non_rtti_object（"no RTTI data"）未捕获崩溃
    void mInfoWindow::notifyVectorForceDataAboutToBeDestroyed(int feID)
    {
        if (_motionVectorForcePanel)
            _motionVectorForcePanel->onForceElementDataAboutToBeDestroyed(feID);
    }

    void mInfoWindow::notifyMotionBodyDataAboutToBeDestroyed(int id)
    {
        if (_motionBodyPanel)
            _motionBodyPanel->onMotionBodyDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionJointDataAboutToBeDestroyed(int id)
    {
        if (_motionJointPanel)
            _motionJointPanel->onMotionJointDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionCouplingDataAboutToBeDestroyed(int id)
    {
        if (_motionCouplingPanel)
            _motionCouplingPanel->onMotionCouplingDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionDriveDataAboutToBeDestroyed(int id)
    {
        if (_motionDrivePanel)
            _motionDrivePanel->onMotionDriveDataAboutToBeDestroyed(id);
    }

    //方案删除连带释放其结果对象：动画面板持结果指针，同步置空（面板对空结果有守卫）
    void mInfoWindow::notifyMotionScenarioDataAboutToBeDestroyed(int id)
    {
        if (_motionScenarioPanel)
            _motionScenarioPanel->onMotionScenarioDataAboutToBeDestroyed(id);
        if (_motionAnimationPanel)
            _motionAnimationPanel->onScenarioDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionMarkerDataAboutToBeDestroyed(int id)
    {
        if (_motionMarkerPanel)
            _motionMarkerPanel->onMotionMarkerDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionSpringDataAboutToBeDestroyed(int id)
    {
        if (_motionSpringPanel)
            _motionSpringPanel->onMotionSpringDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionContactDataAboutToBeDestroyed(int id)
    {
        if (_motionContactPanel)
            _motionContactPanel->onMotionContactDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionSensorDataAboutToBeDestroyed(int id)
    {
        if (_motionSensorPanel)
            _motionSensorPanel->onMotionSensorDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionControllerDataAboutToBeDestroyed(int id)
    {
        if (_motionControllerPanel)
            _motionControllerPanel->onMotionControllerDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionAutoFlexDataAboutToBeDestroyed(int id)
    {
        if (_motionAutoFlexPanel)
            _motionAutoFlexPanel->onMotionAutoFlexDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionControlInputDataAboutToBeDestroyed(int id)
    {
        if (_motionControlInputPanel)
            _motionControlInputPanel->onMotionControlInputDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionElectroImportDataAboutToBeDestroyed(int id)
    {
        if (_motionElectroImportPanel)
            _motionElectroImportPanel->onMotionElectroImportDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionElectroExportDataAboutToBeDestroyed(int id)
    {
        if (_motionElectroExportPanel)
            _motionElectroExportPanel->onMotionElectroExportDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionHydraulicDataAboutToBeDestroyed(int id)
    {
        if (_motionHydraulicPanel)
            _motionHydraulicPanel->onMotionHydraulicDataAboutToBeDestroyed(id);
    }

    void mInfoWindow::notifyMotionRLDataAboutToBeDestroyed(int id)
    {
        if (_motionRLPanel)
            _motionRLPanel->onMotionRLDataAboutToBeDestroyed(id);
    }

	void mInfoWindow::unEditPredefinedField()
	{
		if (_workingConditionPanel && _workingConditionPanel->isEditing())
			_workingConditionPanel->unEditPredefinedField();
	}

	void mInfoWindow::unEditMotionLoadImport()
	{
		//面板懒加载未创建时无编辑态可收尾
		if (_motionLoadImportPanel && _motionLoadImportPanel->isEditing())
			_motionLoadImportPanel->deactivatePickAndCommit();
	}

    void mInfoWindow::createNonStructuralQuality()
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mNonStructuralQualityPanel).get());
        ensurePanel(_mNonStructuralQualityPanel)->newNonStructuralQualityPanel();
    }

    void mInfoWindow::editNonStructuralQuality(int nonStructuralQualityId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mNonStructuralQualityPanel).get());
        ensurePanel(_mNonStructuralQualityPanel)->editPropertyPanel(nonStructuralQualityId);
    }

    void mInfoWindow::uneditNonStructuralQuality()
    {
        if (_mNonStructuralQualityPanel && _mNonStructuralQualityPanel->isEditing())
        	_mNonStructuralQualityPanel->uneditNonStructuralQualityPanel();
    }

    void mInfoWindow::editNsmManagerUnderCasePanel(int caseId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mNsmManagerUnderCasePanel).get());
        ensurePanel(_mNsmManagerUnderCasePanel)->editNsmManagerUnderCasePanel(caseId);
    }

    void mInfoWindow::editModelFixManagerUnderCasePanel(int caseId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_mModelFixManaUnderCasePanel).get());
        ensurePanel(_mModelFixManaUnderCasePanel)->editModelFixManagerUnderCasePanel(caseId);
    }

	void mInfoWindow::createProperty(PropertyType type)
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_propertyPanel).get());
		ensurePanel(_propertyPanel)->newPropertyPanel(type);
	}
	void mInfoWindow::editProperty(int propertyId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_propertyPanel).get());
		ensurePanel(_propertyPanel)->editPropertyPanel(propertyId);
	}

	void mInfoWindow::uneditProperty()
	{
		if (_propertyPanel && _propertyPanel->isEditing())
			_propertyPanel->uneditPropertyPanel();
	}
	void mInfoWindow::checkProperty(int type)
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_propertyCheckPanel).get());
		ensurePanel(_propertyCheckPanel)->checkPropertyPanel(type);
	}
	void mInfoWindow::uneditCheckProperty()
	{
		if (_propertyCheckPanel && _propertyCheckPanel->isEditing())
			_propertyCheckPanel->uneditPropertyCheckPanel();
	}

	void mInfoWindow::showSetManager()
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_setManagerPanel).get());
		ensurePanel(_setManagerPanel)->showSetManager();
	}

	void mInfoWindow::showLoadManager()
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		auto panel = ensureTypedPanel<mManagerPanelTpl<mLoadPanelTraits>>(_loadManagerPanel);
		//懒加载创建后补注入启动时缓存的菜单栏（内嵌矩阵按工况类型校验需要）
		if (_menuWidget)
			panel->setMenuWidget(_menuWidget);
		this->setMainWidget(panel.get());
		panel->showManager();
	}
	void mInfoWindow::showLoadReference(int caseID)
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensureTypedPanel<mReferencePanelTpl<mLoadPanelTraits, true>>(_loadReferencePanel).get());
		ensureTypedPanel<mReferencePanelTpl<mLoadPanelTraits, true>>(_loadReferencePanel)->showReference(caseID);
	}
	void mInfoWindow::showBoundaryManager()
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		auto panel = ensureTypedPanel<mManagerPanelTpl<mBoundaryPanelTraits>>(_boundaryManagerPanel);
		if (_menuWidget)
			panel->setMenuWidget(_menuWidget);
		this->setMainWidget(panel.get());
		panel->showManager();
	}
	void mInfoWindow::showBoundaryReference(int caseID)
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensureTypedPanel<mReferencePanelTpl<mBoundaryPanelTraits, true>>(_boundaryReferencePanel).get());
		ensureTypedPanel<mReferencePanelTpl<mBoundaryPanelTraits, true>>(_boundaryReferencePanel)->showReference(caseID);
	}
	void mInfoWindow::showPredefinedFieldManager()
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		auto panel = ensureTypedPanel<mManagerPanelTpl<mPredefinedFieldPanelTraits>>(_predefinedFieldManagerPanel);
		if (_menuWidget)
			panel->setMenuWidget(_menuWidget);
		this->setMainWidget(panel.get());
		panel->showManager();
	}
	void mInfoWindow::showPredefinedFieldReference(int caseID)
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensureTypedPanel<mReferencePanelTpl<mPredefinedFieldPanelTraits, false>>(_predefinedFieldReferencePanel).get());
		ensureTypedPanel<mReferencePanelTpl<mPredefinedFieldPanelTraits, false>>(_predefinedFieldReferencePanel)->showReference(caseID);
	}
	void mInfoWindow::showContactManager()
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		auto panel = ensureTypedPanel<mManagerPanelTpl<mContactPanelTraits>>(_contactManagerPanel);
		//懒加载创建后补注入启动时缓存的菜单栏（矩阵按工况类型校验需要）
		if (_menuWidget)
			panel->setMenuWidget(_menuWidget);
		this->setMainWidget(panel.get());
		panel->showManager();
	}
	void mInfoWindow::showContactReference(int caseID)
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensureTypedPanel<mReferencePanelTpl<mContactPanelTraits, false>>(_contactReferencePanel).get());
		ensureTypedPanel<mReferencePanelTpl<mContactPanelTraits, false>>(_contactReferencePanel)->showReference(caseID);
	}
	void mInfoWindow::showModelFixManager()
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		auto panel = ensureTypedPanel<mManagerPanelTpl<mModelFixPanelTraits>>(_modelFixManagerPanel);
		if (_menuWidget)
			panel->setMenuWidget(_menuWidget);
		this->setMainWidget(panel.get());
		panel->showManager();
	}
	void mInfoWindow::showModelFixReference(int caseID)
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensureTypedPanel<mReferencePanelTpl<mModelFixPanelTraits, false>>(_modelFixReferencePanel).get());
		ensureTypedPanel<mReferencePanelTpl<mModelFixPanelTraits, false>>(_modelFixReferencePanel)->showReference(caseID);
	}

	void mInfoWindow::editElement(QString type)
	{
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_elementPanel).get());
		ensurePanel(_elementPanel)->editElementPanel(type);
	}
	void mInfoWindow::unEditElement()
	{
		if (_elementPanel && _elementPanel->isEditing())
			_elementPanel->uneditElementPanel();
	}
	//集合
	void mInfoWindow::createSet()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_setPanel).get());
		ensurePanel(_setPanel)->newSetPanel();
	}

    void mInfoWindow::createSet(QString setName)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_setPanel).get());
        ensurePanel(_setPanel)->newSetPanel(setName);
    }

	void mInfoWindow::editSet(int setId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_setPanel).get());
		ensurePanel(_setPanel)->editSetPanel(setId);
	}

	void mInfoWindow::uneditSet()
	{
		if (_setPanel && _setPanel->isEditing())
			_setPanel->uneditSetPanel();
	}

	//坐标
	void mInfoWindow::createCoordinate(MxFunctions::CoordinateType type, QVector3D origin)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_coordinatePanel).get());
		ensurePanel(_coordinatePanel)->newCoordinatePanel(type, origin);
	}

	void mInfoWindow::editCoordinate(int coorId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_coordinatePanel).get());
		ensurePanel(_coordinatePanel)->editCoordinatePanel(coorId);
	}

	void mInfoWindow::uneditCoordinate()
	{
		if (_coordinatePanel && _coordinatePanel->isEditing())
			_coordinatePanel->uneditCoordinatePanel();
	}

	void mInfoWindow::createNodeCoordinate()
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_nodeCoordinatePanel).get());
		ensurePanel(_nodeCoordinatePanel)->newNodeCoordinatePanel();
	}

	void mInfoWindow::uneditNodeCoordinate()
	{
		if (_nodeCoordinatePanel && _nodeCoordinatePanel->isEditing())
			_nodeCoordinatePanel->uneditNodeCoordinatePanel();
	}

    void mInfoWindow::createMyPanel()
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
       
        this->setMainWidget(ensurePanel(_myPanel).get());
        ensurePanel(_myPanel)->newMyPanel();
      
    }

	void mInfoWindow::editCurveNode(int nodeId)
	{
		//关闭销毁阶段不再联动参数栏（激活 dock / 换主部件都是 widget 操作，
		//qApp 析构后执行会崩溃）
		if (QCoreApplication::closingDown())
			return;
		QWidget* page = mCurvePropPageManager::getInstance()->takeParamBarPage(nodeId);
		if (!page)
			return;
		mInfoWindow::getInstance()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(page);
	}


	//函数(曲线)
	void mInfoWindow::createFunction(int type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_FunctionPanel).get());
		ensurePanel(_FunctionPanel)->newFunctionPanel(type);
	}

	void mInfoWindow::editFunction(int setId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_FunctionPanel).get());
		ensurePanel(_FunctionPanel)->editFunctionPanel(setId);
	}

	void mInfoWindow::editSectionShape(int sectionShapeId)
	{
		getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).setCurrentID(sectionShapeId);
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_SectionShapePanel).get());
		/*if (getCVM->getPreRenderWidget() == nullptr)
			return;



		if (_SectionShapePanel == nullptr)
		{
			_SectionShapePanel = std::make_shared<mSectionShapePanel>();
		}

		
		_SectionShapePanel->newSectionShapePanel(getCVM->getModelData()->getManager<mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(sectionShapeId)->getSectionShapeType());*/
		

		ensurePanel(_SectionShapePanel)->editSectionShapePanel(sectionShapeId);
		
	}

	void mInfoWindow::uneditFunction()
	{
		//_FunctionPanel->uneditFunctionPanel();
	}
	//创建截面形状
	void mInfoWindow::createSectionShape(int type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_SectionShapePanel).get());
		ensurePanel(_SectionShapePanel)->newSectionShapePanel(type);
		
	
		

	
		

		//// 1. 默认 H 参数（类型 + 4 个值）
		//QList<double> para;
		//para << MxFunctions::Sec_H << 200 << 10 << 180 << 12;
		//QString name = QString("H-Section-%1").arg(QDateTime::currentMSecsSinceEpoch());

		/* 2. 写新数据层（替代老 mPBeam） */
		//_SectionShapePanel->getSectionShapeManager()->addSection(name, para);

		/* 3. 发信号（告诉全世界：新的 H 型来了） */
		//emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSectionShapeSig(true, /*create*/ /*MxFunctions::Sec_H*/0);


		// 3. 让面板一次性刷新（老函数里会 setRowCount 4 / 14）
		/*_SectionShapePanel->rebuildPanel(para, name);*/

		

		
	}
	void mInfoWindow::createCopyPanel(std::vector<int> copyId)
	{
		ensurePanel(_SectionShapePanel)->createCopyPanel(copyId);
	}
    void mInfoWindow::unEditInfoWindow(bool clearPicked)
    {
        // 树选中链路内（slot_itemSelectionChanged 已做过完整清理）由 show*Manager/
        // show*Reference 等触发的重复调用：短路一次，避免 clear/渲染器删除/表格复位双份执行
        if (_uneditSuppressed)
        {
            _uneditSuppressed = false;
            return;
        }
        this->setMainWidget(nullptr);
        uneditCoordinate();
        uneditNodeCoordinate();
        uneditSet();
        uneditMaterial();
        uneditProperty();
        uneditCheckProperty();
        unEditElement();
        unEditLoad();
        unEditBoundary();
        unEditConnector();
        unSearchConnector();
        uneditGeometry();
        unEditContact();
        unEditContactPairPanel();
        unEditPeriodicBoundary();
        unEditMeshPanel();
        //后处理面板复位执行体已迁PostWindow::mPostInfoHub，经总线解耦（直连同步）
        emit MechCommon::mMechGlobalSignals::getInstance()->requestUneditPostPanelsSig();
        uneditGeoInfo();
        unEditOptimization();
        unEditRunnerOptimization();
        unEditPredefinedField();
        unEditDeputy();
        uneditEleSetCtrl();
        uneditAnalysisField();
        unEditMotion();
        unEditMotionLoadImport();
        if (_mDiscreteFieldPanel && _mDiscreteFieldPanel->isEditing())
            _mDiscreteFieldPanel->uneditDiscreteFieldPanel();
        if (_workingConditionPanel && _workingConditionPanel->isEditing())
            _workingConditionPanel->unEditRotateAnalyse();
        if (getCVM && clearPicked)
        {
            getCVM->clearPick();
            if (getCVM->getPreRenderWidget())
            {
                getCVM->getPreRenderWidget()->getHighlightController()->clearTempHighlight();
                getCVM->getPreRenderWidget()->getHighlightController()->clearTempMeshHighlight();
                //getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont();//不能删除全部字体显示，这是一个通用接口，应该在退出面板时清空自己的键

                //删除临时高亮（后续做成接触、绑定等做成符号后就可删除）
                {
                    getCVM->getPreRenderWidget()->getHighlightController()->deleteExtraPreMeshHighLightRender(getCVM->getPreRenderWidget()->getHighlightController()->getExtraPreMeshHighLightRenderName());
                    getCVM->getPreRenderWidget()->getHighlightController()->deleteExtraPreMeshFaceHighLightRender(getCVM->getPreRenderWidget()->getHighlightController()->getExtraPreMeshFaceHighLightRenderName());
                    getCVM->getPreRenderWidget()->getHighlightController()->deleteExtraPreGeoHighLightRender(getCVM->getPreRenderWidget()->getHighlightController()->getExtraPreGeoHighLightRenderName());
                }
            }
        }
        mTableDataWindow::getInstance()->clearAndReset(0, 0);
        mTableDataWindow::getInstance()->resetInitialTable();
    }
	void mInfoWindow::commitActivePickWidget()
	{
		mPickWidgetController::commitActivePickWidget();
		//写文件前把正在编辑的分析场面板控件值（默认场值等 LineEdit 内容）写回数据对象，
		//否则编辑中未切出面板就保存时，LineEdit 里的修改不会进入 .mx 文件
		if (_mAnalysisFieldPanel && _mAnalysisFieldPanel->isEditing())
			_mAnalysisFieldPanel->commitEditingData();
	}

	void mInfoWindow::slot_viewportModelAboutToChange(int newViewportID)
	{
		if (!getCVM)
		{
			return;
		}

		//同模型视口切换：面板编辑态保持（拾取会话不中断，与拾取控件/模型树处理一致）
		if (newViewportID != -1)
		{
			int newModelID = -1;
			if (VMManager->getViewportPrivateByID(newViewportID, &newModelID)
				&& newModelID == getCVM->getModelData()->getModelDataID())
			{
				return;
			}
		}

		//跨模型切换/新建模型/删除当前模型：此时当前模型指针仍指向旧模型，
		//按退出面板语义统一收尾（数据复核、拾取提交、折叠框卸载均落在旧模型上），
		//避免共享面板带着旧模型编辑态进入新模型后与后续面板叠加显示
		unEditInfoWindow(true);
	}

	void mInfoWindow::searchConnector()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_connectorSearchPannel).get());
		ensurePanel(_connectorSearchPannel)->editSearchConnector();
	}

	//显示外部创建的面板(如三点确定草图平面面板)
	void mInfoWindow::showCustomPanel(QWidget* panel)
	{
		if (panel == nullptr)
			return;

		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		setMainWidget(panel);
	}
	void mInfoWindow::unSearchConnector()
	{
		if (_connectorSearchPannel && _connectorSearchPannel->isEditing())
			_connectorSearchPannel->unEditSearchConnector();
	}

	void mInfoWindow::createConnector(MxFunctions::ConnectorType type)
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_connectorPannel).get());
		ensurePanel(_connectorPannel)->createConnector(type);
	}

	void mInfoWindow::openMotionLoadImport(const QString& path)
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		//入口先统一退出其它面板编辑态（对齐 showSetManager 等入口约定）
		unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionLoadImportPanel).get());
		ensurePanel(_motionLoadImportPanel)->openWithFile(path);
	}

	void mInfoWindow::editConnector(int connId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_connectorPannel).get());
		ensurePanel(_connectorPannel)->editConnector(connId);
	}

	void mInfoWindow::unEditConnector()
	{
		if (_connectorPannel && _connectorPannel->isEditing())
			_connectorPannel->unEditConnector();
	}

	void mInfoWindow::rendConnFreeNodes()
	{
		ensurePanel(_connectorPannel)->rendFreeNodes();
	}

	void mInfoWindow::disRendConnFreeNodes()
	{
		ensurePanel(_connectorPannel)->disRendFreeNodes();
	}

	void mInfoWindow::createDeputy(MxFunctions::DeputyType type)
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_deputyPannel).get());
		ensurePanel(_deputyPannel)->createDeputy(type);
	}

	void mInfoWindow::editDeputy(int deputyId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_deputyPannel).get());
		ensurePanel(_deputyPannel)->editDeputy(deputyId);
	}

	void mInfoWindow::unEditDeputy()
	{
		if (_deputyPannel && _deputyPannel->isEditing())
			_deputyPannel->unEditDeputy();
	}

	void mInfoWindow::createContact(MxFunctions::ContactType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_contactPanel).get());
		ensurePanel(_contactPanel)->newContactPanel(type);
	}

    void mInfoWindow::createElementSetCtrl(int caseId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_elementSetCtrlPanel).get());
        int modelFixId = ensurePanel(_elementSetCtrlPanel)->newEleSetCtrlPanel();
        if (caseId > 0 && modelFixId > 0)
        {
            auto& relationMgr = getCVM->getModelData()->getMapRelationManager();
            QList<int> modelFixIds = relationMgr.getMapRelation<QList<int>>({ Physical_Case, caseId }, Physical_ModelFix);
            relationMgr.appendTableMapRelation(
                { Physical_Case, caseId },
                { Physical_ModelFix, modelFixId },
                modelFixIds.size()
            );
            emit MechCommon::mMechGlobalSignals::getInstance()->updateModelFixTreeView(caseId, modelFixId, 0);
        }
    }

    void mInfoWindow::editModelFix(int modelFixId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_elementSetCtrlPanel).get());
        ensurePanel(_elementSetCtrlPanel)->editEleSetCtrlPanel(modelFixId);
    }

    void mInfoWindow::uneditEleSetCtrl()
    {
        if (_elementSetCtrlPanel && _elementSetCtrlPanel->isEditing())
        	_elementSetCtrlPanel->uneditEleSetCtrlPanel();
    }

	void mInfoWindow::editContact(int contactId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_contactPanel).get());
		ensurePanel(_contactPanel)->editContactPanel(contactId);
	}

	void mInfoWindow::unEditContact()
	{
		if (_contactPanel && _contactPanel->isEditing())
			_contactPanel->uneditContactPanel();
	}

	void mInfoWindow::createPeriodicBoundary(MxFunctions::PeriodicBoundaryType type)
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_mPeriodicBoundaryPanel).get());
		ensurePanel(_mPeriodicBoundaryPanel)->newPeriodicBoundaryPanel(type);
	}

	void mInfoWindow::editPeriodicBoundary(int pbId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_mPeriodicBoundaryPanel).get());
		ensurePanel(_mPeriodicBoundaryPanel)->editPeriodicBoundaryPanel(pbId);
	}

	void mInfoWindow::unEditPeriodicBoundary()
	{
		if (_mPeriodicBoundaryPanel && _mPeriodicBoundaryPanel->isEditing())
			_mPeriodicBoundaryPanel->uneditPeriodicBoundaryPanel();
	}

	void mInfoWindow::editContactPairPanel()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_contactPairPanel).get());
		ensurePanel(_contactPairPanel)->editContactPairPanel();
	}

	void mInfoWindow::unEditContactPairPanel()
	{
		if (_contactPairPanel && _contactPairPanel->isEditing())
			_contactPairPanel->uneditContactPairPanel();
	}

	//材料
	void mInfoWindow::createMaterial(MxFunctions::MaterialType matType)
	{
        MxFunctions::MaterialType type = matType;
		if (getCVM->getPreRenderWidget() == nullptr)
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_materialPanel).get());
		ensurePanel(_materialPanel)->createMaterial(type);
	}

	void mInfoWindow::editMaterial(int matId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_materialPanel).get());
		ensurePanel(_materialPanel)->editMaterial(matId);
	}

    void mInfoWindow::updateMaterial(int matId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_materialPanel).get());
        //_materialPanel->slotUpdateMaterialMenu(matId);
    }

	void mInfoWindow::uneditMaterial()
	{
		if (_materialPanel && _materialPanel->isEditing())
			_materialPanel->unEditMaterial();
	}

	void mInfoWindow::storeMaterialProps()
	{
		if (_materialPanel)
			_materialPanel->updatePropsAndFindDiff();
	}

	//几何-信息
	void mInfoWindow::createGeoInfo(const BrowserTreeType type, const std::vector<int> ids)
	{
		if (getCVM == nullptr)
		{
			return;
		}

		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoInfoPanel).get());
		ensurePanel(_geoInfoPanel)->showPanel(type);
		ensurePanel(_geoInfoPanel)->getGeoInfoById(ids);
	}

	void mInfoWindow::updateGeoInfo(const std::vector<int> ids)
	{
		if (getCVM == nullptr)
		{
			return;
		}
		if (_geoInfoPanel)
			_geoInfoPanel->updateInfo(ids);
	}

    void mInfoWindow::uneditGeoInfo()
    {
        if (_geoInfoPanel)
        	_geoInfoPanel->disconnectUnitSystemResponse();
    }

	//几何-特征
	void mInfoWindow::createGeoFeature(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoFeaturePanel).get());
		ensurePanel(_geoFeaturePanel)->showFeatureFold(type);
        //进入面板后连接拾取、单位制等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoFeaturePanel)->connectGeoSignals();
	}

	void mInfoWindow::uneditGeometry()
	{
		//关闭屏幕绘制曲线
		if (_geoEditPanel)
			_geoEditPanel->closeDrawCurve();

        //清空按钮背景色
        mGeoSketchSignalManager::instance()->clearButtonSelection();

		//清空线上交点信息
		if (getCVM && getCVM->getPreRenderWidget())
		{
			auto vecSelInfos = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getMoreSelectDataByType(SelectionInfoType::GeoEdge);
            auto vecSelInfosFace = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getMoreSelectDataByType(SelectionInfoType::GeoFace);
            if (!vecSelInfos.empty())
            {
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdateMorePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, vecSelInfos);
            }
            if (!vecSelInfosFace.empty())
            {
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdateMorePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, vecSelInfosFace);
            }
		}

		if (_geoEditPanel)
			_geoEditPanel->geoPickWidget->_featPick = None;
        if (_geoDirectModelingPanel)
        	_geoDirectModelingPanel->geoPickWidget->_featPick = None;
	}

	void mInfoWindow::unEditMeshPanel()
	{
		//关闭屏幕绘制曲线
		if (_meshPanel)
			_meshPanel->unEditMeshPanel();
        if (_mMeshEditPanel)
        	_mMeshEditPanel->clearCheck();
	}

	void mInfoWindow::creatFREQ()
	{
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_FREQPanel).get());
        ensurePanel(_FREQPanel)->createFREQ();
	}

    void mInfoWindow::editFREQ(int freqId)
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_FREQPanel).get());
        ensurePanel(_FREQPanel)->editFREQ(freqId);
    }

	void mInfoWindow::creatLaminate(LaminateType type)
	{
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_LaminatePanel).get());
        ensurePanel(_LaminatePanel)->newLaminatePanel(type);
	}

    void mInfoWindow::editLaminate(int laminateId)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_LaminatePanel).get());
        ensurePanel(_LaminatePanel)->editLaminatePanel(laminateId);
    }

	//几何-变换
	void mInfoWindow::createGeoTransform(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoTransformPanel).get());
		ensurePanel(_geoTransformPanel)->showFold(type);
        //进入面板后连接拾取、单位制等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoTransformPanel)->connectGeoSignals();
	}

	//几何-布尔
	void mInfoWindow::createGeoBool(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoBoolPanel).get());
		ensurePanel(_geoBoolPanel)->showFold(type);
        //进入面板后连接拾取等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoBoolPanel)->connectGeoSignals();
	}

	//几何-编辑
	void mInfoWindow::createGeoEdit(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoEditPanel).get());
		ensurePanel(_geoEditPanel)->showFold(type);
        //进入面板后连接拾取、单位制等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoEditPanel)->connectGeoSignals();
	}

    //几何-修复
    void mInfoWindow::createGeoRepair(const GeoOperationType type)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        this->setMainWidget(ensurePanel(_geoRepairPanel).get());
        ensurePanel(_geoRepairPanel)->showFold(type);
        //进入面板后连接拾取等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoRepairPanel)->connectGeoSignals();
    }

	//几何-实体化
	void mInfoWindow::createGeoSolidify(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoSolidifyPanel).get());
		ensurePanel(_geoSolidifyPanel)->showFold(type);
        //进入面板后连接拾取、后台任务等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoSolidifyPanel)->connectGeoSignals();
	}

	//几何-共享拓扑
	void mInfoWindow::createGeoSharedTopology(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoSharedTopologyPanel).get());
		ensurePanel(_geoSharedTopologyPanel)->showFold(type);
        //进入面板后连接拾取、视口切换等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoSharedTopologyPanel)->connectGeoSignals();
	}

	//几何-去除
	void mInfoWindow::createGeoRemoval(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoRemovalPanel).get());
		ensurePanel(_geoRemovalPanel)->showFold(type);
        //进入面板后连接拾取、单位制等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoRemovalPanel)->connectGeoSignals();
	}

	//几何-直接建模
	void mInfoWindow::createGeoDirectModeling(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoDirectModelingPanel).get());
		ensurePanel(_geoDirectModelingPanel)->showFold(type);
        //进入面板后连接拾取、单位制、控制器等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoDirectModelingPanel)->connectGeoSignals();
	}

	//【直模退出机制】ESC 清拾取等外部入口的转发：面板未创建时为空操作；
	// 实际清理语义见 mGeoDirectModelingPanel::exitDirectModelingSession。
	void mInfoWindow::exitGeoDirectModeling()
	{
		if (_geoDirectModelingPanel)
			_geoDirectModelingPanel->exitDirectModelingSession();
	}

	//几何-提取
	void mInfoWindow::createGeoExtract(const GeoOperationType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_geoExtractPanel).get());
		ensurePanel(_geoExtractPanel)->showFold(type);
        //进入面板后连接拾取、单位制等全局信号（uneditGeometry 统一断开）
        ensurePanel(_geoExtractPanel)->connectGeoSignals();
	}


	void mInfoWindow::addMaterialProp(const MxFunctions::MaterialClass& propname)
	{
		//_materialPanel->addMaterialProp1(propname);
	}

	void mInfoWindow::clearMaterialProp(const MxFunctions::MaterialClass& propname)
	{
		if (_materialPanel)
			_materialPanel->clearPropData(propname);
	}

	void mInfoWindow::remMaterialProp(const MxFunctions::MaterialClass& propname)
	{
		if (_materialPanel)
			_materialPanel->removeMaterialProp(propname);
	}

	void mInfoWindow::storeMaterialProp()
	{
		if (_materialPanel)
			_materialPanel->slot_finishEdit();
		//_materialPanel->updatePropsAndFindDiff();
	}

	void mInfoWindow::addMaterialFromLib(const QString& name, const QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>>& props)
	{
		ensurePanel(_materialPanel)->addMaterialFromLib(name, props);
	}

	void mInfoWindow::slot_changeLanguage(int language)
	{
		//if (language == 0)
		//{
		//	//设置Dock名称
		//	this->setDockWindowTitle(QString("参数"));
		//}
		//else if (language == 1)
		//{
		//	//设置Dock名称
		//	this->setDockWindowTitle(QString("Parameter"));
		//}
	}

	void mInfoWindow::editGlobalDim(int meshId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");

		this->setMainWidget(ensurePanel(_globalDimPanel).get());
		ensurePanel(_globalDimPanel)->editGlobalDimPanel(meshId);
	}

	void mInfoWindow::editMeshPanel(int meshId)
	{
		
		mDockLayoutManager::getInstance()->activateDock("Info");
		//_meshPanel = std::make_shared<mSolidMeshPanel>();
		MeshPanelData::mMeshPanelBase* _meshPanelData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
		int type = _meshPanelData->getMeshPanelType();
		switch (type) {
		case SolidMesh:
			_meshPanel = ensurePanel(_mSolidMeshPanel);
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(4);
		
			break;
		case FaceMesh:
			_meshPanel = ensurePanel(_mFaceMeshPanel);
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
	
			break;
		case LineMesh:
			_meshPanel = ensurePanel(_mLineMeshPanel);
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			break;
		case AutoAdapt:
			_meshPanel = ensurePanel(_mAutoAdaptPanel);
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
			break;
		case CustomEncryption:
			_meshPanel = ensurePanel(_mCustomEncryptionPanel);
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(4);
			break;
		case MeshPanelType::LocalSize:
			_meshPanel = ensurePanel(_mLocalSizePanel);
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			break;
		case EdgeSuppress:
			_meshPanel = ensurePanel(_mEdgeSuppressPanel);
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			break;
		default:
			break;
		}
		this->setMainWidget(ensurePanel(_meshPanel).get());
		ensurePanel(_meshPanel)->editMeshPanel(meshId);
	}
    void mInfoWindow::editOptMeshPanel(int meshId) {
        mDockLayoutManager::getInstance()->activateDock("Info");
        //_meshPanel = std::make_shared<mSolidMeshPanel>();
        MeshPanelData::mMeshOptPanelBase* _meshOptPanelData = getCVM->getModelData()->getManager<MeshPanelData::mMeshOptPanelManager>(Physical_MeshOptPanel).getMeshPanelData(meshId);
        int type = _meshOptPanelData->getMeshPanelType();
        switch (type) {
        case MeshEdit:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(6);
            _meshPanel = ensurePanel(_mMeshEditPanel);
            break;
        case CreatePoint:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
            _meshPanel = ensurePanel(_mCreatePointPanel);
            break;
        case CreateMesh:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
            _meshPanel = ensurePanel(_mCreateMeshPanel);
            break;
        case CreateMetaMesh:
            _meshPanel = ensurePanel(_mCreateMetaMeshPanel);
            break;
        case RotateMesh:
            _meshPanel = ensurePanel(_mRotateMeshPanel);
            break;
        case ExtractSurface:
            _meshPanel = ensurePanel(_mExtractSurfacePanel);
            break;
        case Translation:
            _meshPanel = ensurePanel(_mTranslationPanel);
            break;
        case Rotation:
            _meshPanel = ensurePanel(_mRotationPanel);
            break;
        case Mirroring:
            _meshPanel = ensurePanel(_mMirroringPanel);
            break;
        case Matrix:
            _meshPanel = ensurePanel(_mMatrixPanel);
            break;
        case MeshZoom:
            _meshPanel = ensurePanel(_mZoomPanel);
            break;
        case MeshSmooth:
            _meshPanel = ensurePanel(_mSmoothPanel);
            break;
        case Remeshing:
            _meshPanel = ensurePanel(_mRemeshingPanel);
            break;
        case STLRemesh:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
            _meshPanel = ensurePanel(_mSTLRemeshPanel);
            break;
        case Renumber:
            _meshPanel = ensurePanel(_mRenumberPanel);
            break;
        case MeshNormal:
            _meshPanel = ensurePanel(_mMeshNormalPanel);
            break;
        case ChangeOrder:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
            _meshPanel = ensurePanel(_mChangeOrderPanel);
            break;
        case SharedNode:
            _meshPanel = ensurePanel(_mSharedNodePanel);
            break;
        case MeshPanelType::MeshFreeEdge:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
            _meshPanel = ensurePanel(_mFreeEdgePanel);
            break;
        case TEdge:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
            _meshPanel = ensurePanel(_mTEdgePanel);
            break;
        case MeshIntersect:
            _meshPanel = ensurePanel(_mMeshIntersectPanel);
            break;
        case MeshOverlap:
            _meshPanel = ensurePanel(_mMeshOverlapPanel);
            break;
        case MeshInterfere:
            _meshPanel = ensurePanel(_mMeshInterferePanel);
            break;
        case MeshStretch:
            _meshPanel = ensurePanel(_mStretchPanel);
            break;
        case MoveMesh:
            _meshPanel = ensurePanel(_mMoveMeshPanel);
            break;
        default:
            break;
        }
        this->setMainWidget(ensurePanel(_meshPanel).get());
        ensurePanel(_meshPanel)->editMeshPanel(meshId);
    }
	void mInfoWindow::createMeshPanelMultiSelect(const std::vector<int> ids)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		//_meshPanel = std::make_shared<mSolidMeshPanel>();
		if (ids.empty())
			return;
		MeshPanelData::mMeshPanelBase* _meshPanelData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(ids[0]);
		int meshType = _meshPanelData->getMeshPanelType();
		for (auto& it : ids)
		{
			MeshPanelData::mMeshPanelBase* _meshPanelData = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(it);
			int type_ = _meshPanelData->getMeshPanelType();
			if (type_ != meshType)
			{
				return;
			}
		}
		if (meshType == MxFunctions::SolidMesh)
		{
			_meshPanel = make_shared<mSolidMeshPanel>();
			this->setMainWidget(ensurePanel(_meshPanel).get());
			ensurePanel(_meshPanel)->getMeshInfoById(ids);
		}
		else if (meshType == MxFunctions::FaceMesh)
		{
			_meshPanel = make_shared<mFaceMeshPanel>();
			this->setMainWidget(ensurePanel(_meshPanel).get());
			ensurePanel(_meshPanel)->getMeshInfoById(ids);
		}
		else if (meshType == MxFunctions::LineMesh)
		{
			_meshPanel = make_shared<mLineMeshPanel>();
			this->setMainWidget(ensurePanel(_meshPanel).get());
			ensurePanel(_meshPanel)->getMeshInfoById(ids);
		}
		else if (meshType == MxFunctions::LocalSize)
		{
			/*_meshPanel = make_shared<mLocalSizePanel>();
			this->setMainWidget(_meshPanel.get());
			_meshPanel->getMeshInfoById(ids);*/
		}
		else if (meshType == MxFunctions::AutoAdapt)
		{
			_meshPanel = make_shared<mAutoAdaptPanel>();
			this->setMainWidget(ensurePanel(_meshPanel).get());
			ensurePanel(_meshPanel)->getMeshInfoById(ids);
		}
		else if (meshType == MxFunctions::CustomEncryption)
		{
			_meshPanel = make_shared<mCustomEncryptionPanel>();
			this->setMainWidget(ensurePanel(_meshPanel).get());
			ensurePanel(_meshPanel)->getMeshInfoById(ids);
		}

	}
	//网格:全局尺寸
	void mInfoWindow::createGlobalDimensions()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_globalDimPanel).get());
		//mCustomtreeWidgetItem* _globalDimItem = _modelTreeManager->getCurrentModelTree()->getItem(MeshParentItem);
		
		ensurePanel(_globalDimPanel)->newGlobalDimPanel();

	
	}


	
	//创建网格
	void mInfoWindow::createMeshPanel(int type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		std::shared_ptr<mMeshPanel> meshPanel = nullptr;
		switch (type) {
		case SolidMesh:
			_meshPanel = ensurePanel(_mSolidMeshPanel);
			break;
		case FaceMesh:
			_meshPanel = ensurePanel(_mFaceMeshPanel);
			break;
		case LineMesh:
			_meshPanel = ensurePanel(_mLineMeshPanel);
			break;
		case EdgeSuppress:
			_meshPanel = ensurePanel(_mEdgeSuppressPanel);
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			break;
		case AutoAdapt:
			_meshPanel = ensurePanel(_mAutoAdaptPanel);
			break;
		case CustomEncryption:
			_meshPanel = ensurePanel(_mCustomEncryptionPanel);			
			break;
		case MeshPanelType::LocalSize:
			_meshPanel = ensurePanel(_mLocalSizePanel);
			break;
		default:
			break;
		}
		if (_meshPanel == nullptr)
		{
			_meshPanel = std::make_shared<mMeshPanel>();
		}
		
		this->setMainWidget(ensurePanel(_meshPanel).get());
		ensurePanel(_meshPanel)->newMeshPanel(type);
		switch (type) {
		case SolidMesh:
			ensurePanel(_meshPanel)->getMeshPanelBase()->setMeshShape("四面体");
			break;
		case FaceMesh:
			ensurePanel(_meshPanel)->getMeshPanelBase()->setMeshShape("三角形");
			break;
		default:
			break;
		}
		//新建节点后刷新面板，使UI显示与新建数据一致（避免残留上一个节点的形状等显示状态）
		_meshPanel->updatePanel();

	}

    //创建网格操作
    void mInfoWindow::createMeshOptPanel(int type)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        std::shared_ptr<mMeshPanel> meshPanel = nullptr;
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
        switch (type) {
        case MeshEdit:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(6);
            _meshPanel = ensurePanel(_mMeshEditPanel);
            ensurePanel(_mMeshEditPanel)->updatePanel();
            break;
        case CreatePoint:
            _meshPanel = ensurePanel(_mCreatePointPanel);
            break;
        case CreateMesh:
            _meshPanel = ensurePanel(_mCreateMeshPanel);
            break;
        case CreateMetaMesh:
            _meshPanel = ensurePanel(_mCreateMetaMeshPanel);
            break;
        case RotateMesh:
            _meshPanel = ensurePanel(_mRotateMeshPanel);
            break;
        case ExtractSurface:
            _meshPanel = ensurePanel(_mExtractSurfacePanel);
            break;
        case Translation:
            _meshPanel = ensurePanel(_mTranslationPanel);
            break;
        case Rotation:
            _meshPanel = ensurePanel(_mRotationPanel);
            break;
        case Mirroring:
            _meshPanel = ensurePanel(_mMirroringPanel);
            break;
        case Matrix:
            _meshPanel = ensurePanel(_mMatrixPanel);
            break;
        case MeshStretch:
            _meshPanel = ensurePanel(_mStretchPanel);
            break;
        case MeshZoom:
            _meshPanel = ensurePanel(_mZoomPanel);
            break;
        case MeshQuality:
            _meshPanel = ensurePanel(_mQualityPanel);
            break;
        case MeshSmooth:
            _meshPanel = ensurePanel(_mSmoothPanel);
            break;
        case Remeshing:
            _meshPanel = ensurePanel(_mRemeshingPanel);
            break;
        case STLRemesh:
            _meshPanel = ensurePanel(_mSTLRemeshPanel);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
            break;
        case Renumber:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
            _meshPanel = ensurePanel(_mRenumberPanel);
            break;
        case MeshNormal:
            _meshPanel = ensurePanel(_mMeshNormalPanel);
            break;
        case ChangeOrder:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
            _meshPanel = ensurePanel(_mChangeOrderPanel);
            break;
        case SharedNode:
            _meshPanel = ensurePanel(_mSharedNodePanel);
            break;
        case MeshPanelType::MeshFreeEdge:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
            _meshPanel = ensurePanel(_mFreeEdgePanel);
            break;
        case TEdge:
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
            _meshPanel = ensurePanel(_mTEdgePanel);
            break;
        case MeshIntersect:
            _meshPanel = ensurePanel(_mMeshIntersectPanel);
            break;
        case MeshOverlap:
            _meshPanel = ensurePanel(_mMeshOverlapPanel);
            break;
        case MeshInterfere:
            _meshPanel = ensurePanel(_mMeshInterferePanel);
            break;
        case MeshStatistics:
            _meshPanel = ensurePanel(_mMeshStatisticsPanel);
            break;
        case MoveMesh:
            _meshPanel = ensurePanel(_mMoveMeshPanel);
            break;
        default:
            break;
        }
        if (_meshPanel == nullptr)
        {
            _meshPanel = std::make_shared<mMeshPanel>();
        }

        this->setMainWidget(ensurePanel(_meshPanel).get());
        ensurePanel(_meshPanel)->newMeshOptPanel(type);

        // 创建网格面板显示后，延迟切换到网格节点拾取模式
        if (type == CreateMesh)
        {
            QTimer::singleShot(0, this, []() {
                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
            });
        }

    }
	void mInfoWindow::createMeshByPartId(vector<int> partIds)
	{
		ensurePanel(_meshPanel)->slot_createMeshByPartId(partIds);
	}

	void mInfoWindow::createMeshAuto()
	{
		isMeshing = true;
		createBtn->setEnabled(false);
		deleteBtn->setEnabled(false);
		_actions->at(5)->setEnabled(false);
		_actions->at(6)->setEnabled(false);
		_actions->at(7)->setEnabled(false);
		_actions->at(8)->setEnabled(false);
		CCSMRendWidget* rend = getCVM->getPreRenderWidget();
		int binary = int(rend->getPickFilter());
		_pickFilter = BinaryToDecima[binary];
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
		//网格剖分父节点的"生成"可在未打开过任何网格面板时触发（如新建立方体后直接一键生成），
		//此时_meshPanel尚未构造，而剖分线程与信号连接都在面板构造函数里建立，须先确保其存在
		ensurePanel(_meshPanel)->slot_createMeshAuto();
	}

	void mInfoWindow::deleteMeshAuto()
	{
		ensurePanel(_meshPanel)->slot_deleteMeshAuto();
	}
	void mInfoWindow::createMeshBySetting()
	{
		isMeshing = true;
		_actions->at(5)->setEnabled(false);
		_actions->at(6)->setEnabled(false);
		_actions->at(7)->setEnabled(false);
		_actions->at(8)->setEnabled(false);
		mInfoWindow::getInstance()->createBtn->setEnabled(false);
		mInfoWindow::getInstance()->deleteBtn->setEnabled(false);
		CCSMRendWidget* rend = getCVM->getPreRenderWidget();
		int binary = int(rend->getPickFilter());
		_pickFilter = BinaryToDecima[binary];
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
		ensurePanel(_meshPanel)->slot_createMeshBySetting();
	}
	void mInfoWindow::createMesh(int id)
	{
		isMeshing = true;
		_actions->at(5)->setEnabled(false);
		_actions->at(6)->setEnabled(false);
		_actions->at(7)->setEnabled(false);
		_actions->at(8)->setEnabled(false);
		mInfoWindow::getInstance()->createBtn->setEnabled(false);
		mInfoWindow::getInstance()->deleteBtn->setEnabled(false);
		CCSMRendWidget* rend = getCVM->getPreRenderWidget();
		int binary = int(rend->getPickFilter());
		_pickFilter = BinaryToDecima[binary];
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
		ensurePanel(_meshPanel)->slot_createMesh(id);
	}
	void mInfoWindow::createMesh1()
	{
		isMeshing = true;
		_actions->at(5)->setEnabled(false);
		_actions->at(6)->setEnabled(false);
		_actions->at(7)->setEnabled(false);
		_actions->at(8)->setEnabled(false);
		mInfoWindow::getInstance()->createBtn->setEnabled(false);
		mInfoWindow::getInstance()->deleteBtn->setEnabled(false);
		CCSMRendWidget* rend = getCVM->getPreRenderWidget();
		int binary = int(rend->getPickFilter());
		_pickFilter = BinaryToDecima[binary];
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
		ensurePanel(_meshPanel)->slot_createMesh1();
	}
	void mInfoWindow::createMeshAutoAdapt()
	{
		isMeshing = true;
		_actions->at(5)->setEnabled(false);
		_actions->at(6)->setEnabled(false);
		_actions->at(7)->setEnabled(false);
		_actions->at(8)->setEnabled(false);
		mInfoWindow::getInstance()->createBtn->setEnabled(false);
		mInfoWindow::getInstance()->deleteBtn->setEnabled(false);
		CCSMRendWidget* rend = getCVM->getPreRenderWidget();
		int binary = int(rend->getPickFilter());
		_pickFilter = BinaryToDecima[binary];
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
		ensurePanel(_meshPanel)->slot_createMeshAutoAdapt();
	}
	void mInfoWindow::createMeshCustomEncryption()
	{
		isMeshing = true;
		_actions->at(5)->setEnabled(false);
		_actions->at(6)->setEnabled(false);
		_actions->at(7)->setEnabled(false);
		_actions->at(8)->setEnabled(false);
		mInfoWindow::getInstance()->createBtn->setEnabled(false);
		mInfoWindow::getInstance()->deleteBtn->setEnabled(false);
		CCSMRendWidget* rend = getCVM->getPreRenderWidget();
		int binary = int(rend->getPickFilter());
		_pickFilter = BinaryToDecima[binary];
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
		ensurePanel(_meshPanel)->slot_createMeshCustomEncryption();
	}
    void mInfoWindow::createMeshLocalSize(int id)
{
    isMeshing = true;
    _actions->at(5)->setEnabled(false);
    _actions->at(6)->setEnabled(false);
    _actions->at(7)->setEnabled(false);
    _actions->at(8)->setEnabled(false);
    mInfoWindow::getInstance()->createBtn->setEnabled(false);
    mInfoWindow::getInstance()->deleteBtn->setEnabled(false);
    CCSMRendWidget* rend = getCVM->getPreRenderWidget();
    int binary = int(rend->getPickFilter());
    _pickFilter = BinaryToDecima[binary];
    MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
    ensurePanel(_meshPanel)->slot_createMeshLocalSize(id);
}
void mInfoWindow::createMeshEdgeSuppress(int id, bool isSuppress)
{
	// 边抑制/取消抑制功能待实现
	ensurePanel(_meshPanel)->slot_createMeshEdgeSuppress(id, isSuppress);  
}
void mInfoWindow::copyMeshChild()
	{
		if (_meshPanel)
			_meshPanel->slot_updateListWidget();
	}
	void mInfoWindow::boxShow()
	{
		ensurePanel(_meshPanel)->slot_boxShow();
	}
	void mInfoWindow::boxDelete()
	{
		ensurePanel(_meshPanel)->slot_boxDelete();
	}
	void mInfoWindow::boxHide()
	{
		ensurePanel(_meshPanel)->slot_boxHide();
	}
	void mInfoWindow::boxPreview()
	{
		ensurePanel(_meshPanel)->slot_boxPreview();
	}
    void mInfoWindow::localSizePreview()
    {
        ensurePanel(_meshPanel)->slot_localSizePreview();
        ensurePanel(_meshPanel)->slot_deleteMeshAuto();
    }
	void mInfoWindow::createWorkingDir()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		//面板经 ensurePanel 仅构造一次（构造函数内完成打开按钮的信号连接），
		//此处重复调用只做激活/设为主部件，不再追加连接
		this->setMainWidget(ensurePanel(_WorkingDirPanel).get());
	}
	mInfoWindow::~mInfoWindow()
	{

	}

	void mInfoWindow::updateAllPanelUnits()
	{
		//材料
		if (_materialPanel)
			_materialPanel->updatePanelUnit();
	}

	/********************* 多体动力学：入口实现（面板经ensurePanel懒加载，迁移自RebuildMotion） **********************/
	void mInfoWindow::createMotionBody()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionBodyPanel).get());
		ensurePanel(_motionBodyPanel)->newMotionBodyPanel();
	}

	//批量定义运动体：进面板选部件子集（列表/视口拾取），确定逐部件 1:1 建体
	void mInfoWindow::createMotionBodyBatch()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionBodyBatchPanel).get());
		ensurePanel(_motionBodyBatchPanel)->newBatchPanel();
	}

	//一键生成运动体：当前模型全部部件直接建体，不进面板
	//（运动体名=部件名，重名自动 -x 后缀；已归属部件跳过）
	void mInfoWindow::createMotionBodyOneKey()
	{
		if (!getCVM || !getCVM->getModelData() || !getCVM->getModelData()->getMeshData())
			return;
		auto* meshData = getCVM->getModelData()->getMeshData();
		std::vector<int> partIds;
		for (const QString& name : meshData->getAllPartNames())
		{
			const int partId = meshData->getPartIdByPartName(name);
			if (partId != -1)
				partIds.push_back(partId);
		}
		if (partIds.empty())
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
				QString::fromUtf8("当前模型没有部件，无法生成运动体"));
			return;
		}
		QStringList skipped;
		const int created = mMotionBodyBatchPanel::batchCreateMotionBodies(partIds, &skipped);
		QString msg = QString::fromUtf8("一键生成 %1 个运动体（部件同名，重名自动加 -x 后缀）").arg(created);
		if (!skipped.isEmpty())
			msg += QString::fromUtf8("；跳过已归属部件：%1").arg(skipped.join(", "));
		MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(msg);
	}

	void mInfoWindow::editMotionBody(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionBodyPanel).get());
		ensurePanel(_motionBodyPanel)->editMotionBodyPanel(id);
	}

	void mInfoWindow::unEditMotion()
	{
		//与材料等面板一致的守卫：仅对处于编辑会话中的面板执行退出清理，
		//避免对从未进入编辑的面板做无谓的拾取失活/渲染清理
		if (_motionBodyPanel && _motionBodyPanel->isEditing()) _motionBodyPanel->uneditMotionBodyPanel();
		if (_motionBodyBatchPanel && _motionBodyBatchPanel->isEditing()) _motionBodyBatchPanel->uneditBatchPanel();
		if (_motionJointPanel && _motionJointPanel->isEditing()) _motionJointPanel->uneditMotionJointPanel();
		if (_motionCouplingPanel && _motionCouplingPanel->isEditing()) _motionCouplingPanel->uneditMotionCouplingPanel();
		if (_motionDrivePanel && _motionDrivePanel->isEditing()) _motionDrivePanel->uneditMotionDrivePanel();
		if (_motionScenarioPanel && _motionScenarioPanel->isEditing()) _motionScenarioPanel->uneditMotionScenarioPanel();
		if (_motionMarkerPanel && _motionMarkerPanel->isEditing()) _motionMarkerPanel->uneditMotionMarkerPanel();
		if (_motionVectorForcePanel && _motionVectorForcePanel->isEditing()) _motionVectorForcePanel->uneditMotionVectorForcePanel();
		if (_motionContactPanel && _motionContactPanel->isEditing()) _motionContactPanel->uneditMotionContactPanel();
		if (_motionSpringPanel && _motionSpringPanel->isEditing()) _motionSpringPanel->uneditMotionSpringPanel();
		if (_motionAnimationPanel && _motionAnimationPanel->isEditing()) _motionAnimationPanel->uneditAnimationPanel();
		if (_motionLoadExportPanel && _motionLoadExportPanel->isEditing()) _motionLoadExportPanel->uneditLoadExportPanel();
		if (_motionCurvePanel && _motionCurvePanel->isEditing()) _motionCurvePanel->uneditMotionCurvePanel();
		if (_motionControlInputPanel && _motionControlInputPanel->isEditing()) _motionControlInputPanel->uneditControlInputPanel();
		if (_motionElectroImportPanel && _motionElectroImportPanel->isEditing()) _motionElectroImportPanel->uneditElectroImportPanel();
		if (_motionElectroExportPanel && _motionElectroExportPanel->isEditing()) _motionElectroExportPanel->uneditElectroExportPanel();
		if (_motionHydraulicPanel && _motionHydraulicPanel->isEditing()) _motionHydraulicPanel->uneditHydraulicPanel();
		if (_motionRLPanel && _motionRLPanel->isEditing()) _motionRLPanel->uneditRLPanel();
		if (_motionSensorPanel && _motionSensorPanel->isEditing()) _motionSensorPanel->uneditMotionSensorPanel();
		if (_motionControllerPanel && _motionControllerPanel->isEditing()) _motionControllerPanel->uneditMotionControllerPanel();
		if (_motionAutoFlexPanel && _motionAutoFlexPanel->isEditing()) _motionAutoFlexPanel->uneditMotionAutoFlexPanel();
	}

	void mInfoWindow::createMotionJoint(MxFunctions::JointType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionJointPanel).get());
		ensurePanel(_motionJointPanel)->newMotionJointPanel(type);
	}

	void mInfoWindow::editMotionJoint(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionJointPanel).get());
		ensurePanel(_motionJointPanel)->editMotionJointPanel(id);
	}

	void mInfoWindow::createMotionCoupling(MxFunctions::CouplingType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionCouplingPanel).get());
		ensurePanel(_motionCouplingPanel)->newMotionCouplingPanel(type);
	}

	void mInfoWindow::editMotionCoupling(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionCouplingPanel).get());
		ensurePanel(_motionCouplingPanel)->editMotionCouplingPanel(id);
	}

	void mInfoWindow::createMotionDrive(MxFunctions::DriveTargetType type, MechMotionData::DriveControlMode mode)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionDrivePanel).get());
		ensurePanel(_motionDrivePanel)->newMotionDrivePanel(type, mode);
	}

	void mInfoWindow::editMotionDrive(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionDrivePanel).get());
		ensurePanel(_motionDrivePanel)->editMotionDrivePanel(id);
	}

	void mInfoWindow::createMotionMarker()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionMarkerPanel).get());
		ensurePanel(_motionMarkerPanel)->newMotionMarkerPanel();
	}

	void mInfoWindow::editMotionMarker(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionMarkerPanel).get());
		ensurePanel(_motionMarkerPanel)->editMotionMarkerPanel(id);
	}

	void mInfoWindow::createMotionVectorForce(MechMotionData::VectorLoadType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionVectorForcePanel).get());
		ensurePanel(_motionVectorForcePanel)->newMotionVectorForcePanel(type);
	}

	void mInfoWindow::editMotionVectorForce(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionVectorForcePanel).get());
		ensurePanel(_motionVectorForcePanel)->editMotionVectorForcePanel(id);
	}

	void mInfoWindow::createMotionContact(int type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionContactPanel).get());
		ensurePanel(_motionContactPanel)->newMotionContactPanel(static_cast<MechMotionData::MotionContactType>(type));
	}

	void mInfoWindow::editMotionContact(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionContactPanel).get());
		ensurePanel(_motionContactPanel)->editMotionContactPanel(id);
	}

	void mInfoWindow::createMotionSpring(int type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionSpringPanel).get());
		ensurePanel(_motionSpringPanel)->newMotionSpringPanel(static_cast<MechMotionData::SpringType>(type));
	}

	void mInfoWindow::editMotionSpring(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionSpringPanel).get());
		ensurePanel(_motionSpringPanel)->editMotionSpringPanel(id);
	}

	void mInfoWindow::createMotionControlInput()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionControlInputPanel).get());
		ensurePanel(_motionControlInputPanel)->newControlInputPanel();
	}

	void mInfoWindow::editMotionControlInput(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionControlInputPanel).get());
		ensurePanel(_motionControlInputPanel)->editControlInputPanel(id);
	}

	void mInfoWindow::createMotionElectroImport()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionElectroImportPanel).get());
		ensurePanel(_motionElectroImportPanel)->newElectroImportPanel();
	}

	void mInfoWindow::editMotionElectroImport(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionElectroImportPanel).get());
		ensurePanel(_motionElectroImportPanel)->editElectroImportPanel(id);
	}

	void mInfoWindow::createMotionElectroExport()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionElectroExportPanel).get());
		ensurePanel(_motionElectroExportPanel)->newElectroExportPanel();
	}

	void mInfoWindow::editMotionElectroExport(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionElectroExportPanel).get());
		ensurePanel(_motionElectroExportPanel)->editElectroExportPanel(id);
	}

	void mInfoWindow::createMotionHydraulic()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionHydraulicPanel).get());
		ensurePanel(_motionHydraulicPanel)->newHydraulicPanel();
	}

	void mInfoWindow::editMotionHydraulic(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionHydraulicPanel).get());
		ensurePanel(_motionHydraulicPanel)->editHydraulicPanel(id);
	}

	void mInfoWindow::createMotionRL()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionRLPanel).get());
		ensurePanel(_motionRLPanel)->newRLPanel();
	}

	void mInfoWindow::editMotionRL(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionRLPanel).get());
		ensurePanel(_motionRLPanel)->editRLPanel(id);
	}

	void mInfoWindow::createMotionSensor(MechMotionData::SensorType type)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionSensorPanel).get());
		ensurePanel(_motionSensorPanel)->newMotionSensorPanel(type);
	}

	void mInfoWindow::createMotionController()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionControllerPanel).get());
		ensurePanel(_motionControllerPanel)->newMotionControllerPanel();
	}

	void mInfoWindow::editMotionSensor(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionSensorPanel).get());
		ensurePanel(_motionSensorPanel)->editMotionSensorPanel(id);
	}

	void mInfoWindow::editMotionController(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionControllerPanel).get());
		ensurePanel(_motionControllerPanel)->editMotionControllerPanel(id);
	}

	void mInfoWindow::createMotionAutoFlex()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionAutoFlexPanel).get());
		ensurePanel(_motionAutoFlexPanel)->newMotionAutoFlexPanel();
	}

	void mInfoWindow::editMotionAutoFlex(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionAutoFlexPanel).get());
		ensurePanel(_motionAutoFlexPanel)->editMotionAutoFlexPanel(id);
	}

	void mInfoWindow::createMotionScenario()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionScenarioPanel).get());
		ensurePanel(_motionScenarioPanel)->newMotionScenarioPanel();
	}

	void mInfoWindow::editMotionScenario(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionScenarioPanel).get());
		ensurePanel(_motionScenarioPanel)->editMotionScenarioPanel(id);
	}

	void mInfoWindow::solveMotionScenario(int id)
	{
		ensurePanel(_motionScenarioPanel)->onStartSolve(id);
	}

	void mInfoWindow::editMotionAnimation(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionAnimationPanel).get());
		ensurePanel(_motionAnimationPanel)->editAnimationPanel(id);
	}

	void mInfoWindow::editMotionLoadExport(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionLoadExportPanel).get());
		ensurePanel(_motionLoadExportPanel)->editLoadExportPanel(id);
	}

	void mInfoWindow::createMotionCurveSelector(int scenarioId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		ensurePanel(_motionCurvePanel)->newMotionCurvePanel();
		ensurePanel(_motionCurvePanel)->editMotionCurvePanel(scenarioId);
		this->setMainWidget(ensurePanel(_motionCurvePanel).get());
	}

	void mInfoWindow::editMotionCurveSelector(int scenarioId)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		//先确保面板已创建再进入编辑：首次点击"曲线"节点时面板尚未懒加载，
		//若先调 edit 会被空指针守卫跳过，导致 m_scenarioId 失效、添加曲线静默无效
		this->setMainWidget(ensurePanel(_motionCurvePanel).get());
		ensurePanel(_motionCurvePanel)->editMotionCurvePanel(scenarioId);
	}

	//控制输入/机电耦合/液压/强化学习/系统建模工具（Ribbon调用，无Motion前缀历史命名）
	void mInfoWindow::createControlInput()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionControlInputPanel).get());
		ensurePanel(_motionControlInputPanel)->newControlInputPanel();
	}
	void mInfoWindow::editControlInput(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionControlInputPanel).get());
		ensurePanel(_motionControlInputPanel)->editControlInputPanel(id);
	}
	void mInfoWindow::uneditControlInput()
	{
		if (_motionControlInputPanel) _motionControlInputPanel->uneditControlInputPanel();
	}

	void mInfoWindow::createElectroImport()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionElectroImportPanel).get());
		ensurePanel(_motionElectroImportPanel)->newElectroImportPanel();
	}
	void mInfoWindow::editElectroImport(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionElectroImportPanel).get());
		ensurePanel(_motionElectroImportPanel)->editElectroImportPanel(id);
	}
	void mInfoWindow::uneditElectroImport()
	{
		if (_motionElectroImportPanel) _motionElectroImportPanel->uneditElectroImportPanel();
	}

	void mInfoWindow::createElectroExport()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionElectroExportPanel).get());
		ensurePanel(_motionElectroExportPanel)->newElectroExportPanel();
	}
	void mInfoWindow::editElectroExport(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionElectroExportPanel).get());
		ensurePanel(_motionElectroExportPanel)->editElectroExportPanel(id);
	}
	void mInfoWindow::uneditElectroExport()
	{
		if (_motionElectroExportPanel) _motionElectroExportPanel->uneditElectroExportPanel();
	}

	void mInfoWindow::createHydraulic()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionHydraulicPanel).get());
		ensurePanel(_motionHydraulicPanel)->newHydraulicPanel();
	}
	void mInfoWindow::editHydraulic(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionHydraulicPanel).get());
		ensurePanel(_motionHydraulicPanel)->editHydraulicPanel(id);
	}
	void mInfoWindow::uneditHydraulic()
	{
		if (_motionHydraulicPanel) _motionHydraulicPanel->uneditHydraulicPanel();
	}

	void mInfoWindow::createRLPanel()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionRLPanel).get());
		ensurePanel(_motionRLPanel)->newRLPanel();
	}
	void mInfoWindow::editRLPanel(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionRLPanel).get());
		ensurePanel(_motionRLPanel)->editRLPanel(id);
	}
	void mInfoWindow::uneditRLPanel()
	{
		if (_motionRLPanel) _motionRLPanel->uneditRLPanel();
	}

	void mInfoWindow::createMechanism()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionMechanismPanel).get());
		ensurePanel(_motionMechanismPanel)->newMechanismPanel();
	}
	void mInfoWindow::editMechanism(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionMechanismPanel).get());
		ensurePanel(_motionMechanismPanel)->editMechanismPanel(id);
	}
	void mInfoWindow::uneditMechanism()
	{
		if (_motionMechanismPanel) _motionMechanismPanel->uneditMechanismPanel();
	}

	void mInfoWindow::createTransmission()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionTransmissionPanel).get());
		ensurePanel(_motionTransmissionPanel)->newTransmissionPanel();
	}
	void mInfoWindow::editTransmission(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionTransmissionPanel).get());
		ensurePanel(_motionTransmissionPanel)->editTransmissionPanel(id);
	}
	void mInfoWindow::uneditTransmission()
	{
		if (_motionTransmissionPanel) _motionTransmissionPanel->uneditTransmissionPanel();
	}

	void mInfoWindow::createMechElecHyd()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionMechElecHydPanel).get());
		ensurePanel(_motionMechElecHydPanel)->newMechElecHydPanel();
	}
	void mInfoWindow::editMechElecHyd(int id)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		this->setMainWidget(ensurePanel(_motionMechElecHydPanel).get());
		ensurePanel(_motionMechElecHydPanel)->editMechElecHydPanel(id);
	}
	void mInfoWindow::uneditMechElecHyd()
	{
		if (_motionMechElecHydPanel) _motionMechElecHydPanel->uneditMechElecHydPanel();
	}

	/* ---------- FoldTable ---------- */
	FoldTable::FoldTable(const QString &title, QWidget *parent)
		: QWidget(parent)
	{
		// 折叠按钮：左侧箭头 + 文字
		m_btn = new QPushButton(title, this);
		m_btn->setCheckable(true);
		m_btn->setChecked(false);
		m_btn->setIcon(getFoldIcon(false));
		m_btn->setLayoutDirection(Qt::LeftToRight);    // 图标在左
		m_btn->setContentsMargins(0, 0, 0, 0);
		//悬停/选中色随主题（applyTheme 重发，构造与切主题共用同一条路径）
		applyTheme();
		connect(m_btn, &QPushButton::toggled, this, &FoldTable::onFoldClicked);

		// 表格
		m_table = new QTableWidget(this);
		m_table->setColumnCount(2);
		m_table->horizontalHeader()->setVisible(false);
		m_table->verticalHeader()->setVisible(false);
		m_table->horizontalHeader()->setStretchLastSection(true);
		m_table->setContentsMargins(0, 0, 0, 0);
		m_table->setFrameShape(QFrame::NoFrame);
		m_table->setVisible(false);
		/* 新增：去掉所有多余空白 */
		m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		m_table->setContentsMargins(0, 0, 0, 0);
		m_table->setMinimumHeight(0);
		updateTableHeight();            // 计算一次高度

		// 整体布局
		QVBoxLayout *lay = new QVBoxLayout(this);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(0);
		lay->addWidget(m_btn);
		lay->addWidget(m_table);
	}

	//标题按钮悬停/选中色取 UiPalette（原为硬编码浅色，深色/护眼主题下不跟随）；
	//mInfoWindow 的主题补发链在切主题/切档时逐 FoldTable 重发，构造亦走本函数
	void FoldTable::applyTheme()
	{
		if (!m_btn)
			return;
		m_btn->setStyleSheet(QString(
			"QPushButton{"
			"  text-align:left; padding:0 2px; border:none; margin:0; }"
			"QPushButton:hover{background:%1;}"
			"QPushButton:checked{background:%2;}")
			.arg(MxFunctions::UiPalette::hoverHex())
			.arg(MxFunctions::UiPalette::selectedHex()));
	}

	void FoldTable::addRow(const QString &name, const QString &value)
	{
		int row = m_table->rowCount();
		m_table->insertRow(row);

		QTableWidgetItem *item0 = new QTableWidgetItem(name);
		item0->setFlags(item0->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 0, item0);

		QTableWidgetItem *item1 = new QTableWidgetItem(value);
		m_table->setItem(row, 1, item1);

		updateTableHeight();
	}

	void FoldTable::addRow(QWidget* widget)
	{
		m_table->setColumnCount(1);
		m_table->insertRow(0);
		m_table->setCellWidget(0, 0, widget);
		m_table->setRowHeight(0,500);
		updateTableHeight();
	}
	void FoldTable::onFoldClicked(bool open)
	{
		// 更新图标
		m_btn->setIcon(getFoldIcon(open));

		// 带动画展开/收起
		int h = open ? m_table->sizeHint().height() : 0;
		QPropertyAnimation *anim = new QPropertyAnimation(m_table, "maximumHeight");
		anim->setDuration(200);
		anim->setStartValue(m_table->maximumHeight());
		anim->setEndValue(h);
		connect(anim, &QPropertyAnimation::finished, [=] {
			if (!open) m_table->setVisible(false);          // 完全隐藏
			m_table->setMaximumHeight(QWIDGETSIZE_MAX);     // 恢复默认
			});
		anim->start(QAbstractAnimation::DeleteWhenStopped);

		if (open) m_table->setVisible(true);
	}

	void FoldTable::updateTableHeight()
	{
		int h = 0;
		for (int i = 0; i < m_table->rowCount(); ++i)
		{
			m_table->resizeRowToContents(i);   // ← 关键：按内容设行高
			h += m_table->rowHeight(i);
		}
		m_table->setFixedHeight(h);
	}

	// 根据主题/资源返回箭头图标（这里用 Qt 自带）
	QIcon FoldTable::getFoldIcon(bool open)
	{
		QStyle *s = style();
		QIcon icon = open ? s->standardIcon(QStyle::SP_ArrowDown)
			: s->standardIcon(QStyle::SP_ArrowRight);
		return icon;
	}
}
