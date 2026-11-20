#include "mPostInfoHub.h"
#include "DerivedCase/mPostDerivedCaseStore.h"
#include "mpostwindow_global.h"
#include "mFramelessDialog.h"	//信息标签弹窗宿主（自绘标题栏与首选项同链路）

//QT
#include <QString>
#include <QApplication>
#include <QTimer>
#include <QFile>
#include <QMainWindow>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QAbstractSpinBox>

#include "mPostCurve.h"
#include "mPostExportWord.h"
#include "mPostParrayMimage.h"
#include "mPostCompVideo.h"
#include "mPostResultPath.h"
#include "mPostPanelParticipation.h"
#include "mPostTransferPathAnalysis.h"
#include "mPostNVHContributionPanel.h"	//NVH工具集面板（模态贡献量分析入参数栏）
#include "mPanelHostProvider.h"	//Info dock容器经IPanelHost接口解耦（原直调mInfoWindow）
#include "mDockLayoutManager.h"
#include "mViewportModelManager.h"
#include "mModelData.h"
#include "mPostDataManager.h"
#include "mViewTreeFunction.h"
#include "mParamPanelBase.h"   //mPanelBase 注册表分发（视口改名面板层跟随）
#include "mPost3DResult.h"
#include "mPostAnimation.h"
#include "mPostGenerateAnimation.h"
#include "mPostCustomScreenshot.h"
#include "mPostCustomVideo.h"
#include "mPostAdvancedQuery.h"
#include "mPostCurveExtract.h"
#include "mPostQueryPanel.h"
#include "mPostSectionCut.h"
#include "mPostStepSequence.h"
#include "mPostEnvelope.h"
#include "mPostVectorPlot.h"
#include "mPostLinearSuperposition.h"
#include "mPostScalarOperation.h"
#include "mPostIsosurface.h"
#include "mPostDeformation.h"
#include "mPostSection.h"
#include "mPostVector.h"
#include "mPostStreamLinePanel.h"
#include "mPostSetPanel.h"
#include "mPostNotesPanel.h"
#include "mPostExplodePanel.h"
#include "mPostMeasureTool.h"
#include "mPostTagPanel.h"
#include "mPostSectionForce.h"
#include "mPostCoordinatePanel.h"
#include "mPostPath.h"
#include "mPostLine.h"
#include "mPostArrow.h"
#include "mPostText.h"
#include "mPostLog.h"
#include "mMechGlobalSignals.h"
#include "mPostDataManager.h"


namespace PostWindow
{
	void mPostInfoHub::mountToHost(QWidget* panel)
	{
		if (panel == nullptr)
			return;
		//面板若在信息标签弹窗宿主里（openNotesAsDialog 弹出过），回 dock 前收起弹窗
		//——面板本体随后被 setMainWidget 重挂进 dock 布局，空壳弹窗不得残留在屏
		if (_notesDlg != nullptr && _notesDlg->isAncestorOf(panel))
			_notesDlg->hide();
		//窄参数栏通用适配（只降不升，见头文件注释）。下限取 min(规范值, 当前有效
		//最小值 max(显式最小宽, minimumSizeHint))：受全局 QSS width:80 支配的按钮/
		//最长条目即最小宽的下拉得到释放（82/90+ → 规范值），自带紧凑样式的原型
		//面板（measure/explode/note/sectionforce 实测 40-60px）保持原样不受扰动
		const auto effectiveMin = [](QWidget* w) { return qMax(w->minimumWidth(), w->minimumSizeHint().width()); };
		for (QComboBox* combo : panel->findChildren<QComboBox*>())
			combo->setMinimumWidth(qMin(50, effectiveMin(combo)));
		for (QPushButton* btn : panel->findChildren<QPushButton*>())
			//上限受 maximumWidth 钳制：显式限宽的紧凑钮（如曲线提取绘制钮 40、
			//拾取辅助钮 26）不得被规范值反向抬升（setMinimumWidth 超 max 时 Qt 会
			//连带抬 max，固定宽被静默放大，违背"只降不升"）
			btn->setMinimumWidth(qMin(qMin(52, effectiveMin(btn)), btn->maximumWidth()));
		for (QLineEdit* edit : panel->findChildren<QLineEdit*>())
		{
			QWidget* pw = edit->parentWidget();
			if (qobject_cast<QAbstractSpinBox*>(pw) != nullptr || qobject_cast<QComboBox*>(pw) != nullptr)
				continue;   //微调框/下拉内部输入框由父控件自管几何
			edit->setMinimumWidth(qMin(40, effectiveMin(edit)));
		}
		//复选/单选：窄栏可压缩 + 自身文字宽下限。不能用 Ignored——Qt 对 Ignored
		//在布局定位时把 hint/minimum 全按 0 算，同排多个选择控件会被依次贴着排，
		//之后 QWidget::setGeometry 才按显式 minimumWidth 钳宽，位置已定 → 同行
		//控件互相叠压（变形控制"系数定义"两单选重叠 49px 实测）。Preferred 保留
		//真实 hint 供定位，压缩只到 minimumWidth 下限，按序排布永不重叠
		for (QCheckBox* check : panel->findChildren<QCheckBox*>())
		{
			check->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
			check->setMinimumWidth(check->minimumSizeHint().width());
		}
		for (QRadioButton* radio : panel->findChildren<QRadioButton*>())
		{
			radio->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
			radio->setMinimumWidth(radio->minimumSizeHint().width());
		}
		MechCommon::panelHostRef()->setMainWidget(panel);
	}

	void mPostInfoHub::createCurve(int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postCurvePanel).get());
		//视口绑定模式（用户定案 2026-11-04）：点击只唤出曲线参数栏并跟随当前激活
		//视口，不按次生成实例
		ensurePanel(_postCurvePanel)->editCurveForViewport(caseid);
	}

	void mPostInfoHub::createExportWord(int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postExportWordPanel).get());

		//视口绑定模式（用户定案 2026-11-04）：点击只唤出报告参数栏并跟随当前激活
		//视口，不按次生成报告实例
		ensurePanel(_postExportWordPanel)->editExportWordForViewport(caseid);
	}

	void mPostInfoHub::createParrayMimage(int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postParrayMimagePanel).get());
		ensurePanel(_postParrayMimagePanel)->newParrayMimagePanel(caseid);
	}

	void mPostInfoHub::createCompVideo(int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postCompVideoPanel).get());
		ensurePanel(_postCompVideoPanel)->newCompVideoPanel(caseid);
	}

	void mPostInfoHub::editCompVideo()
	{
		//只重挂面板不动状态（叠加形态切换后拉回对比视频上下文），不调
		//newCompVideoPanel——新建语义留给"结果"页入口
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postCompVideoPanel).get());
	}

	void mPostInfoHub::editCurve(const QString& viewname)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();

		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postCurvePanel).get());

		//更新节点名称方便获取曲线数据
		ensurePanel(_postCurvePanel)->setCurViewName(viewname);

		ensurePanel(_postCurvePanel)->updateCurveGUI();
	}

	void mPostInfoHub::editExportWord(const QString& viewname)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();

		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postExportWordPanel).get());

		//更新节点名称方便获取曲线数据
		ensurePanel(_postExportWordPanel)->setCurViewName(viewname);

		ensurePanel(_postExportWordPanel)->updateExportWordGUI();
	}

	void mPostInfoHub::updateDisplayCurve()
	{
		ensurePanel(_postCurvePanel)->updateDisplayCurve();
	}

	void mPostInfoHub::showTableData()
	{
		ensurePanel(_postCurvePanel)->showTableData();
	}
	void mPostInfoHub::createResultPath(int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postResultPathPanel).get());
		ensurePanel(_postResultPathPanel)->newPathResult(caseid);
	}

	void mPostInfoHub::panelContributionAmount()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postPanelContributionAmount).get());
	}

	void mPostInfoHub::pathContributionAmount()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postPathContributionAmount).get());
	}

	//堆常驻单例：QObject 无父 + 面板 shared_ptr，规避进程退出期静态析构顺序问题
	mPostInfoHub* mPostInfoHub::getInstance()
	{
		static mPostInfoHub* inst = new mPostInfoHub;
		return inst;
	}

	mPostInfoHub::mPostInfoHub(QObject* parent) : QObject(parent)
	{
		connectMenuSignals();

		//视口改名面板层跟随桥（批次3）：改名信号 → mPanelBase 注册表统一分发。
		//hub 在 PostWindow，依赖方向合法（CommonWidgets 基类不依赖 PreData）
		connect(MechPreData::mViewportModelManager::getInstance(),
			&MechPreData::mViewportModelManager::sig_viewportRenamed, this,
			[](int, const QString& oldName, const QString& newName)
		{
			MechMainWindow::mPanelBase::dispatchViewportRenamed(oldName, newName);
		});
	}

	/*
	* 菜单/主窗信号接线（自 mInfoWindow 构造函数迁入，行为原样；新增第2批
	* 接线：主页截屏双钮/主窗resize·move重绘/前处理树注释·3D结果·快照入口）
	*/
	void mPostInfoHub::connectMenuSignals()
	{
		//结果页/动画页菜单动作（发送方为mPostMenuContributor，经全局信号总线解耦）
		auto* postBus = MechCommon::mMechGlobalSignals::getInstance();
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniFirstFrameSig, this, &mPostInfoHub::slot_firstFrame);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniPrevFrameSig, this, &mPostInfoHub::slot_lastFrame);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniNextFrameSig, this, &mPostInfoHub::slot_nextFrame);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniLastFrameSig, this, &mPostInfoHub::slot_endFrame);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniPlaySig, this, &mPostInfoHub::slot_PlayAnimation);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniSetStatusSig, this, &mPostInfoHub::slot_setStatus);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniSetRenderRangeSig, this, &mPostInfoHub::slot_setRenderRange);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniSetLoopPlaySig, this, &mPostInfoHub::slot_setLoopPlay);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniSetHideFailedElementSig, this, &mPostInfoHub::slot_setHideFailedElement);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniSetFrameRateSig, this, &mPostInfoHub::slot_setFrameRate);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniExportVideoSig, this, &mPostInfoHub::slot_aniExportVideo);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCreateCustomVideoSig, this, &mPostInfoHub::createCustomVideo);

		//云图页菜单动作（查询/过滤/导出STL，发送方为mPostMenuContributor）
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCloudShowValueSig, this, &mPostInfoHub::slot_showValue);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCloudHideValueSig, this, &mPostInfoHub::slot_hideValue);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCloudHideAllSig, this, &mPostInfoHub::slot_hideAll);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAdvancedQuerySig, this, &mPostInfoHub::createAdvancedQuery);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickPostCurveExtractSig, this, &mPostInfoHub::createCurveExtract);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickPostQuerySig, this, &mPostInfoHub::createQuery);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickPostSectionCutSig, this, &mPostInfoHub::createSectionCut);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickPostStepSequenceSig, this, &mPostInfoHub::createStepSequence);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickPostEnvelopeSig, this, &mPostInfoHub::createEnvelope);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickPostVectorPlotSig, this, &mPostInfoHub::createVectorPlot);
		//曲线页诊断组：模态贡献量分析入口（唤醒 NVHToolSet 独立窗口页，唯一入口；
		//门控=「函数与曲线」dock 开，add-nvh-toolset-window-page）
		connect(postBus, &MechCommon::mMechGlobalSignals::clickPostModalContributionSig, this, &mPostInfoHub::createModalContribution);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCloudStartFilterSig, this, &mPostInfoHub::slot_startFilter);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCloudClearFilterSig, this, &mPostInfoHub::slot_clearFilter);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickExportFilterSTLSig, this, &mPostInfoHub::slot_exportFilterSTL);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickDeformationSig, this, &mPostInfoHub::createDeformation);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickIsosurfacePanelSig, this, &mPostInfoHub::openIsosurfacePanel);

		//MainWindow侧入口（主页截屏双钮/主窗resize·move重绘/前处理树注释·3D结果·快照）
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniQuickScreenshotSig, this, &mPostInfoHub::slot_aniQuickScreenshot);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCustomScreenshotSig, this, &mPostInfoHub::createCustomScreenshot);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickAniQuickScreenshotPathSig, this, &mPostInfoHub::aniQuickScreenshot);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCreateNoteSig, this, &mPostInfoHub::createNote);
		connect(postBus, &MechCommon::mMechGlobalSignals::clickCreate3DResultSig, this, &mPostInfoHub::create3DResult);
		connect(postBus, &MechCommon::mMechGlobalSignals::requestRepaintRecordWindowSig, this, &mPostInfoHub::slot_rePainterRecordWindow);
		connect(postBus, &MechCommon::mMechGlobalSignals::requestUneditPostPanelsSig, this, &mPostInfoHub::uneditPath);
	}


	std::shared_ptr<mPost3DResultPanel> mPostInfoHub::cloudResultPanel()
	{
		return ensurePanel(_3DResultPanel);
	}

	void mPostInfoHub::create3DResult(bool isRead, int caseid)
	{
		bool res = ensurePanel(_3DResultPanel)->new3DResultPanel(isRead, caseid);
		if (res)
		{
			mDockLayoutManager::getInstance()->activateDock("Info");
			mountToHost(ensurePanel(_3DResultPanel).get());
		}
		else
		{
			MechCommon::panelHostRef()->unEditInfoWindow();
		}
	}

	bool mPostInfoHub::import3DResult(QStringList paths, int caseid, const QString& modelFilePath,
		bool notifyScanHint)
	{
		bool res =  ensurePanel(_3DResultPanel)->new3DResultPanel(true, caseid, paths, modelFilePath, notifyScanHint);
		if (res)
		{
			mDockLayoutManager::getInstance()->activateDock("Info");
			mountToHost(ensurePanel(_3DResultPanel).get());
		}
		else
		{
			MechCommon::panelHostRef()->unEditInfoWindow();
		}
		return res;
	}

	void mPostInfoHub::update3DResult(int caseid, QStringList viewports)
	{
		ensurePanel(_3DResultPanel)->update3DResultViewport(caseid, viewports);
	}

	void mPostInfoHub::applyResultToViewports(const QStringList& viewports, const QString& postname)
	{
		if (viewports.isEmpty() || postname.isEmpty())
			return;
		ensurePanel(_3DResultPanel)->applyResultToViewports(viewports, postname);
	}

	void mPostInfoHub::edit3DResult(QString viewname)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_3DResultPanel).get());

		//设置当前显示视口
		mViewTreeFunction::activatePostViewport(viewname);

		//更新视口名称
		MechCommon::mMechGlobalSignals::getInstance()->updatePostViewName(viewname);
			
		//更新参数页、菜单栏界面显示
		MechCommon::mMechGlobalSignals::getInstance()->updateResultInfoGUI();
		MechCommon::mMechGlobalSignals::getInstance()->updateResultMenuGUI();
	}

	void mPostInfoHub::slot_startFilter(QString min, QString max, QString type)
	{
		ensurePanel(_3DResultPanel)->startFilter(min, max, type);
	}

	void mPostInfoHub::slot_clearFilter()
	{
		ensurePanel(_3DResultPanel)->clearFilter();
	}

	void mPostInfoHub::slot_exportFilterSTL(const QString& fileName)
	{
		ensurePanel(_3DResultPanel)->exportFilterSTL(fileName);
	}

	void mPostInfoHub::slot_showValue()
	{
		ensurePanel(_3DResultPanel)->showValue();
	}

	void mPostInfoHub::slot_hideValue()
	{
		ensurePanel(_3DResultPanel)->hideValue();
	}

	void mPostInfoHub::slot_hideAll()
	{
		ensurePanel(_3DResultPanel)->hideAll();
	}

	void mPostInfoHub::slot_switchToMaxFrame()
	{
		ensurePanel(_3DResultPanel)->switchToMaxFrame();
	}

    void mPostInfoHub::createNote(int caseid /*= -1*/)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postNotesPanel).get());
        ensurePanel(_postNotesPanel)->newNotesPanel(caseid);
    }

    void mPostInfoHub::autoApplyNotes(int caseid /*= -1*/)
    {
        //导入完成即应用标注（dev_base面板启动即建故始终存在；此处懒创建，未建时先建再应用）。
        //仅创建与应用、不走setMainWidget，不把Info面板切到标注页
        ensurePanel(_postNotesPanel)->newNotesPanel(caseid);
    }

    void mPostInfoHub::openNotesAsDialog(int caseid /*= -1*/)
    {
        //信息标签配置弹窗：共享注释面板装进 mFramelessDialog 宿主弹出。
        //不 unEditInfoWindow/不 activateDock/不 setMainWidget——Info dock 内当前面板
        //（如云图）原地不动；点 X 仅隐藏弹窗不销毁面板。后续任何 setMainWidget
        //(注释面板) 会把它 re-parent 回 dock 布局（mountToHost 先收起弹窗），双向无残留。
        auto panel = ensurePanel(_postNotesPanel);
        panel->newNotesPanel(caseid);
        //主窗引用（弹窗父挂与居中共用；topLevelWidgets 扫描与 mUiShotController
        //同款——隔离规约禁 include MainWindow 头）
        QWidget* mw = nullptr;
        for (QWidget* w : QApplication::topLevelWidgets())
        {
            if (qobject_cast<QMainWindow*>(w) != nullptr && w->isVisible())
            {
                mw = w;
                break;
            }
        }
        //宿主弹窗懒创建：自绘标题栏走 mFramelessDialog 类级 nativeEvent（与首选项
        //同链路）。不再用"面板 reparent 成顶层窗 + installTitleBarChrome 应用级
        //filter 清零非客户区"旧方案——WM_NCCALCSIZE 在 CreateWindowEx 期间同步
        //发出，彼时 HWND 尚未进入 QWidget::find 的映射表，filter 全部落空，
        //原生标题栏残留与自绘栏并存成"双标题栏/双最小化关闭键"（2026-11-14
        //最小复现实验钉因：filter 收到 6 条 NCCALCSIZE、resolved=0）
        if (_notesDlg == nullptr)
        {
            _notesDlg = new MechMainWindow::mFramelessDialog(mw);
            _notesDlg->setWindowTitle(QString("信息标签"));
        }
        //面板从 dock/上次宿主挂进弹窗内容区（setContentWidget=addWidget 重挂，
        //弹窗开着重复点"自定义"再进一次也安全）
        _notesDlg->setContentWidget(panel.get());
        if (!_notesDlg->isVisible())
            _notesDlg->resize(560, 640);
        _notesDlg->show();
        _notesDlg->raise();
        _notesDlg->activateWindow();
        //弹出位置居中主窗：无显式定位时系统把新顶层窗放在屏幕级联位（偏左上），
        //恰好压住左侧 Info dock 的区标题条（ADS 页签条自带最小化/关闭），视觉上
        //与弹窗自绘标题栏粘连成"两行按钮/两个关闭键"。居中后与 dock 列完全错开
        if (mw != nullptr)
        {
            _notesDlg->move(mw->geometry().center().x() - _notesDlg->width() / 2,
                mw->geometry().center().y() - _notesDlg->height() / 2);
        }
    }

    void mPostInfoHub::createExplode(int caseid /*= -1*/)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postExplodePanel).get());
        ensurePanel(_postExplodePanel)->newExplodePanel(caseid);
        ensurePanel(_postExplodePanel)->updatePickFilterAndSymbol(true);
    }

    void mPostInfoHub::createMeasureTool(int caseid /*= -1*/)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postMeasureToolPanel).get());
        ensurePanel(_postMeasureToolPanel)->newMeasureToolPanel(caseid);
        ensurePanel(_postMeasureToolPanel)->setPanelPickAndSignal(true);
    }

    void mPostInfoHub::editMeasure(int recordId)
    {
        //树"测量"子节点点击（V1.3 原型测量树入口）：打开面板 + 定位记录进编辑。
        //面板选中内建统一编辑契约——dirty 弹三选一，被"继续编辑"拦截时编辑对象不变，
        //调用方（树单击）按 currentEditingMeasureId 还原高亮（延迟切换）
        createMeasureTool();
        if (recordId >= 0)
            ensurePanel(_postMeasureToolPanel)->editMeasureRecord(recordId);
    }

    mPostMeasureTool* mPostInfoHub::existingMeasureToolPanel() const
    {
        //不懒创建：树分支重建后的高亮查询用（未开过面板即无编辑对象）
        return _postMeasureToolPanel.get();
    }

    void mPostInfoHub::createTagPanel(int caseid /*= -1*/)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postTagPanel).get());
        ensurePanel(_postTagPanel)->newTagPanel(caseid);
        ensurePanel(_postTagPanel)->setPanelPickAndSignal(true);
    }

    QVector<QPair<int, QString>> mPostInfoHub::tagTreeRecords() const
    {
        //面板懒加载：未开过标签面板即无会话记录，"标签"分支保持空
        if (_postTagPanel == nullptr)
            return {};
        return _postTagPanel->tagTreeRecords();
    }

    void mPostInfoHub::selectTagRecord(int recordId)
    {
        //树"标签"子节点点击：面板已由调用方经 createTagPanel 打开，此处仅选中记录
        if (_postTagPanel == nullptr)
            return;
        _postTagPanel->selectRecord(recordId);
    }

    void mPostInfoHub::createSectionForce(int caseid /*= -1*/)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postSectionForcePanel).get());
        ensurePanel(_postSectionForcePanel)->newSectionForcePanel(caseid);
    }

    void mPostInfoHub::createCoordinate(int coordType)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postCoordinatePanel).get());
        ensurePanel(_postCoordinatePanel)->newCoordinatePanel(coordType);
    }

    void mPostInfoHub::editCoordinate(int coordinateId)
    {
        //树"坐标系"子节点点击/右键"编辑"：面板已由面板自身状态承接，编辑对象切换在
        //editCoordinatePanel 内完成（数据回填+只读守卫，全局坐标系 id=-1 只读展示）
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postCoordinatePanel).get());
        ensurePanel(_postCoordinatePanel)->editCoordinatePanel(coordinateId);
    }

    mPostCoordinatePanel* mPostInfoHub::existingCoordinatePanel() const
    {
        //不懒创建：树侧重命名同步查询用（未开过面板即无编辑对象）
        return _postCoordinatePanel.get();
    }

    void mPostInfoHub::notifyCoordinateRenamed(int coordinateId, const QString& newName)
    {
        //树"坐标系"子节点内联重命名：面板正编辑该坐标系时名称行跟随
        if (_postCoordinatePanel == nullptr)
            return;
        _postCoordinatePanel->onCoordinateRenamedExternally(coordinateId, newName);
    }

    QVector<QPair<int, QString>> mPostInfoHub::coordinateTreeRecords() const
    {
        //面板懒加载：未开过坐标系面板即无会话记录，"坐标系"分支仅剩全局坐标系节点
        if (_postCoordinatePanel == nullptr)
            return {};
        return _postCoordinatePanel->coordinateTreeRecords();
    }

    bool mPostInfoHub::deleteCoordinateRecords(const QList<int>& ids)
    {
        if (_postCoordinatePanel == nullptr)
            return false;
        return _postCoordinatePanel->deleteCoordinateRecords(ids);
    }

    int mPostInfoHub::copyCoordinateRecord(int id)
    {
        if (_postCoordinatePanel == nullptr)
            return -1;
        return _postCoordinatePanel->copyCoordinateRecord(id);
    }

    void mPostInfoHub::clearCoordinateRecords()
    {
        if (_postCoordinatePanel == nullptr)
            return;
        _postCoordinatePanel->clearCoordinateRecords();
    }

    void mPostInfoHub::renameCoordinateRecord(int coordinateId, const QString& newName)
    {
        if (_postCoordinatePanel == nullptr)
            return;
        _postCoordinatePanel->renameCoordinateRecord(coordinateId, newName);
    }

    void mPostInfoHub::createLinearSuperposition(int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postLinearSuperpositionPanel).get());

		ensurePanel(_postLinearSuperpositionPanel)->newLinearPanel(caseid);
	}

	void mPostInfoHub::createScalarOperation(int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postScalarOperationPanel).get());
		ensurePanel(_postScalarOperationPanel)->newScalarPanel(caseid);
	}

	void mPostInfoHub::editLinear(QString viewname)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postLinearSuperpositionPanel).get());

		//线性叠加去派生视口（2026-11-04）：viewname=源云图视口（通常已是激活视口），
		//仅绑定面板不再激活视口——激活链会把面板切回云图面板（edit3DResult）
		ensurePanel(_postLinearSuperpositionPanel)->setCurViewName(viewname);
	}

	void mPostInfoHub::editDerivedCase(int definitionId)
	{
		//按 kind 路由到对应面板装载编辑（store 桶键=当前模型实例 modelDataID）
		const int storeKey = getCVM->getModelData()->getModelDataID();
		DerivedCaseDefinition def;
		if (!mPostDerivedCaseStore::instance().definition(storeKey, definitionId, &def))
			return;
		mDockLayoutManager::getInstance()->activateDock("Info");
		switch (def.kind)
		{
		case DerivedCaseKind::Steps:
			mountToHost(ensurePanel(_postStepSequencePanel).get());
			ensurePanel(_postStepSequencePanel)->editDefinition(definitionId);
			break;
		case DerivedCaseKind::Envelope:
			mountToHost(ensurePanel(_postEnvelopePanel).get());
			ensurePanel(_postEnvelopePanel)->editDefinition(definitionId);
			break;
		case DerivedCaseKind::Superposition:
			mountToHost(ensurePanel(_postLinearSuperpositionPanel).get());
			ensurePanel(_postLinearSuperpositionPanel)->editDefinitionById(definitionId);
			break;
		}
	}

	void mPostInfoHub::editScalarOperation(QString viewname)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postScalarOperationPanel).get());

		//标量运算去派生视口（2026-11-04）：viewname=源云图视口（通常已是激活视口），
		//仅绑定面板不再激活视口——激活链会把面板切回云图面板（edit3DResult）
		ensurePanel(_postScalarOperationPanel)->setCurViewName(viewname);
	}

	void mPostInfoHub::createAnimation(int type, int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		//动画功能启动：请求菜单栏切到“动画”页（该页未勾选时以临时页签显示），
		//保证播放/帧数控制在动画创建后立即可达
		MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow(QString::fromUtf8("动画"));
		//M13 侧栏整合已回退：动画流程恢复独立面板挂载（与云图面板单面板交换）
		mDockLayoutManager::getInstance()->activateDock("Info");
		if (type == 1)
		{
			mountToHost(ensurePanel(_animationPanel).get());
			ensurePanel(_animationPanel)->newAniPanel(caseid);
		}
		else if (type == 2)
		{
			ensurePanel(_generateAnimationPanel)->newGenerateAniPanel(caseid);
			//自定义入口的最终态必须是"生成参数页"：newGenerateAniPanel 内部
			//createAnimationSig(2) 的绑定链会派发 editAnimation，在已生成过动画的
			//视口上（_generateAnimate=1，如刚一键生成后接着点自定义）它改挂播放
			//控制页，把刚挂的生成参数页顶掉——用户报障"自定义没调出参数面板"；
			//绑定链的 setCurViewName 副作用仍需保留（后续 slot_generateAnimation
			//按 _curViewName 作用视口），故在其后重挂收尾
			mountToHost(ensurePanel(_generateAnimationPanel).get());
		}
	}

	void mPostInfoHub::editAnimation(QString viewname)
	{
		//根据动画是否生成显示不同参数页
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(viewname);
		if (postRend == nullptr) return;
		//存储GUI数据
		MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		mDockLayoutManager::getInstance()->activateDock("Info");
		//根据动画是否生成显示不同参数页（播放控制页/生成参数页）
		if (guidata->_generateAnimate)
		{
			mountToHost(ensurePanel(_animationPanel).get());
		}
		else
		{
			mountToHost(ensurePanel(_generateAnimationPanel).get());
		}

		//去派生视口（2026-11-04）：viewname=源云图视口（通常已是激活视口），不再激活
		//视口——激活链会把面板切回云图面板；参数页按源视口 guidata 判定逻辑不变

		//更新动画视口名称
		ensurePanel(_animationPanel)->setCurViewName(viewname);

		//更新参数页、菜单栏界面显示
		MechCommon::mMechGlobalSignals::getInstance()->updateAnimationInfoGUI();
	}

	void mPostInfoHub::slot_firstFrame()
	{
		ensurePanel(_animationPanel)->slot_firstFrame();
	}

	void mPostInfoHub::slot_lastFrame()
	{
		ensurePanel(_animationPanel)->slot_lastFrame();
	}

	void mPostInfoHub::slot_PlayAnimation(bool isrun)
	{
		ensurePanel(_animationPanel)->slot_PlayAnimation(isrun);
	}

	//FR-01/FR-05 单视口形态：树侧删除动画的转发入口——stop 回退停止时帧+状态复位，
	//视口本体不关闭（原树侧 deletePostViewport(动画视口) 随第二视口链拆除）
	void mPostInfoHub::deleteAnimation(QString viewname)
	{
		ensurePanel(_animationPanel)->slot_deleteAnimateFor(viewname);
	}

	//录制×实时动画互斥（实时动画 v1.1）：录制发起广播。调用序约定（录制入口侧）：
	//先广播（实时浮层同步让路停播放，期间录制尚未置位、反向闸不拦让路切帧），
	//再置录制簿记、再调引擎 RecordAnimation/RecordScreen（广播点在引擎调用之前，
	//让路切帧不会落在录制逐帧走查中途）
	void mPostInfoHub::notifyRecordingStarted()
	{
		emit recordingStarted();
	}

	//录制进行中权威查询：聚合录制面板 _isStart 簿记（引擎链发起成功处置位，
	//signal_EndAnimation/CaptureFinished 回执复位）。面板懒创建未触达=未录制约为空
	bool mPostInfoHub::isRecordingActive() const
	{
		if (_postCustomVideoPanel != nullptr && _postCustomVideoPanel->isRecording())
			return true;
		if (_postCustomScreenshotPanel != nullptr && _postCustomScreenshotPanel->isRecording())
			return true;
		return false;
	}

	void mPostInfoHub::slot_nextFrame()
	{
		ensurePanel(_animationPanel)->slot_nextFrame();
	}

	void mPostInfoHub::slot_endFrame()
	{
		ensurePanel(_animationPanel)->slot_endFrame();
	}

	void mPostInfoHub::slot_setRenderRange(const QString& type)
	{
		ensurePanel(_animationPanel)->slot_setRenderRange(type);
	}

	void mPostInfoHub::slot_setHideFailedElement(bool checked)
	{
		ensurePanel(_animationPanel)->slot_setHideFailedElement(checked);
	}

	void mPostInfoHub::slot_setLoopPlay(bool istrue)
	{
		ensurePanel(_animationPanel)->slot_setLoopPlay(istrue);
	}

	void mPostInfoHub::slot_setFrameRate(int fr)
	{
		ensurePanel(_animationPanel)->slot_setFrameRate(fr);
	}

	void mPostInfoHub::slot_setStatus(bool isEnable)
	{
		ensurePanel(_animationPanel)->slot_setStatus(isEnable);
	}

	void mPostInfoHub::slot_generateAnimation(QString ani, QString var, QString start, QString end)
	{
		const bool ok = ensurePanel(_animationPanel)->customGenerateAnimation(ani, var, start, end);
		//生成失败（如云图停在默认伪变量 SolidColor）：customGenerateAnimation 入口已先行
		//启用动画菜单，失败不回滚会留下"开始可点却无动画可播"的空态，与一键生成失败同收口
		if (!ok)
			MechCommon::mMechGlobalSignals::getInstance()->setAnimationMenuStatus(false);
	}

	void mPostInfoHub::slot_updateGenerateAnimationGUI(QString ani, QString var, QString start, QString end)
	{
		ensurePanel(_generateAnimationPanel)->updateGenerateAnimationGUI(ani, var, start, end);
	}

	int mPostInfoHub::slot_aniCurFrame()
	{
		return ensurePanel(_animationPanel)->getCurrentFrameID();
	}

	int mPostInfoHub::slot_aniStartFrame()
	{
		return ensurePanel(_animationPanel)->getStartFrameID();
	}

	int mPostInfoHub::slot_aniEndFrame()
	{
		return ensurePanel(_animationPanel)->getEndFrameID();
	}

	void mPostInfoHub::createCustomScreenshot()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postCustomScreenshotPanel).get());
		ensurePanel(_postCustomScreenshotPanel)->updateScreenshotPanel();
	}

	void mPostInfoHub::createCustomVideo()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postCustomVideoPanel).get());
		ensurePanel(_postCustomVideoPanel)->updateVideoPanel();
	}

	void mPostInfoHub::slot_aniQuickScreenshot()
	{
		ensurePanel(_postCustomScreenshotPanel)->slot_aniQuickScreenshot();
	}

	void mPostInfoHub::aniQuickScreenshot(QString path)
	{
        ensurePanel(_postCustomScreenshotPanel)->aniQuickScreenshot(path);
	}

	void mPostInfoHub::slot_aniExportVideo()
	{
		ensurePanel(_postCustomVideoPanel)->slot_aniExportVideo();
	}

	void mPostInfoHub::slot_videoFrameRange(int min, int max)
	{
		ensurePanel(_postCustomVideoPanel)->setFrameRange(min, max);
	}

	void mPostInfoHub::slot_videoEndFrame(int val)
	{
		ensurePanel(_postCustomVideoPanel)->setEndFrame(val);
	}

	void mPostInfoHub::slot_rePainterRecordWindow()
	{
		ensurePanel(_postCustomScreenshotPanel)->rePainterRecordWindow();
		ensurePanel(_postCustomVideoPanel)->rePainterRecordWindow();
	}

	void mPostInfoHub::createAdvancedQuery()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postAdvancedQueryPanel).get());
		ensurePanel(_postAdvancedQueryPanel)->updateAdvancedQueryPanel();
	}

	void mPostInfoHub::createCurveExtract()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postCurveExtractPanel).get());
		ensurePanel(_postCurveExtractPanel)->updateCurveExtractPanel();
	}

	//模态贡献量分析入口（曲线页诊断组按钮，唯一入口；add-nvh-toolset-window-page
	//2026-11-16 裁定，参数栏嵌入链下线）：唤醒 NVHToolSet 独立窗口页。
	//门控=出现层单层（2026-11-16 二次裁定"曲线模式即可点"）：按钮所在「曲线」
	//Ribbon 页为曲线工作模式专属页签，信号只在曲线模式可达，此处无附加 dock 判定
	//（activateDock 语义=关→开+置前，已开仅置前；面板单例常驻，切走切回状态保留）
	void mPostInfoHub::createModalContribution()
	{
		mDockLayoutManager::getInstance()->activateDock("NVHToolSet");
	}

	void mPostInfoHub::createQuery()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postQueryPanel).get());
		ensurePanel(_postQueryPanel)->updateQueryPanel();
	}

	void mPostInfoHub::createSectionCut()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postSectionCutPanel).get());
		ensurePanel(_postSectionCutPanel)->updateSectionCutPanel();
	}

	void mPostInfoHub::createStepSequence()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postStepSequencePanel).get());
		//入口点击=新建一条定义进入编辑（原型 createDerivedCase 语义，含上下文解析）
		ensurePanel(_postStepSequencePanel)->activateFromMenu();
	}

	void mPostInfoHub::createEnvelope()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postEnvelopePanel).get());
		//入口点击=新建一条定义进入编辑（原型 createDerivedCase 语义，含上下文解析）
		ensurePanel(_postEnvelopePanel)->activateFromMenu();
	}

	void mPostInfoHub::createVectorPlot()
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postVectorPlotPanel).get());
		ensurePanel(_postVectorPlotPanel)->updateVectorPlotPanel();
	}

    void mPostInfoHub::createSection(int caseid)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postSectionPanel).get());

        ensurePanel(_postSectionPanel)->newSectionPanel(caseid);
    }

    void mPostInfoHub::editSection(QString viewname)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postSectionPanel).get());

        //去派生视口（2026-11-04）：仅绑定面板不激活视口（源视口是云图视口，激活链会切走面板）

        //更新切面图视口名称
        ensurePanel(_postSectionPanel)->setCurViewName(viewname);

        // 恢复显示切面
        ensurePanel(_postSectionPanel)->setAllClipingPlaneVisible(true);
    }

	void mPostInfoHub::createIsosurface(int caseid)
	{
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postIsosurfacePanel).get());

        ensurePanel(_postIsosurfacePanel)->newIsosurfacePanel(caseid);
	}

    void mPostInfoHub::editIsosurface(QString viewname)
	{
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postIsosurfacePanel).get());

        //去派生视口（2026-11-04）：仅绑定面板不激活视口（源视口是云图视口，激活链会切走面板）

        //更新等值面图视口名称
        ensurePanel(_postIsosurfacePanel)->setCurViewName(viewname);
	}

    void mPostInfoHub::openIsosurfacePanel()
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postIsosurfacePanel).get());

        //绑定当前激活云图视口（同变形控制 newDeformationPanel；不切视口、不建树节点）。
        //无激活视口时仅显示面板：共享折叠组待 setCurViewName 有绑定视口时才接管
        mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        ensurePanel(_postIsosurfacePanel)->setCurViewName(postRend->getViewportName());
    }

    void mPostInfoHub::restoreIsosurfaceAfterAnimation(const QString& viewname)
    {
        //面板未创建＝该视口从未生成过等值面（生成必经面板），无操作
        if (_postIsosurfacePanel == nullptr || viewname.isEmpty())
            return;
        _postIsosurfacePanel->restoreIsosurfaceIfGenerated(viewname);
    }

	void mPostInfoHub::createDeformation()
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postDeformationPanel).get());

		ensurePanel(_postDeformationPanel)->newDeformationPanel();
	}

	void mPostInfoHub::editDeformation(QString viewname)
	{
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postDeformationPanel).get());

		ensurePanel(_postDeformationPanel)->setCurViewName(viewname);
	}

    void mPostInfoHub::createVector(int caseid)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postVectorPanel).get());

        ensurePanel(_postVectorPanel)->newVectorPanel(caseid);
    }

    void mPostInfoHub::editVector(QString viewname)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postVectorPanel).get());

        //去派生视口（2026-11-04）：仅绑定面板不激活视口（源视口是云图视口，激活链会切走面板）

        //更新矢量图视口名称
        ensurePanel(_postVectorPanel)->setCurViewName(viewname);
    }

    bool mPostInfoHub::haveVector(QString cloudView)
    {
        return _postVectorPanel && _postVectorPanel->haveVector(cloudView);
    }

    void mPostInfoHub::createStreamLine(int caseid)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postStreamLinePanel).get());

        ensurePanel(_postStreamLinePanel)->newStreamLinePanel(caseid);
    }

    void mPostInfoHub::editStreamLine(QString viewname)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postStreamLinePanel).get());

        //去派生视口（2026-11-04）：仅绑定面板不激活视口（源视口是云图视口，激活链会切走面板）

        //更新流线图视口名称
        ensurePanel(_postStreamLinePanel)->setCurViewName(viewname);
    }

    bool mPostInfoHub::haveStreamLine(QString cloudView)
    {
        return _postStreamLinePanel && _postStreamLinePanel->haveStreamLine(cloudView);
    }

	void mPostInfoHub::createSet(int caseid, int panelType)
	{
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postSetPanel).get());

        ensurePanel(_postSetPanel)->newSetPanel(caseid, panelType);
    }

    void mPostInfoHub::editSet(QString viewname)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postSetPanel).get());

        //去派生视口（2026-11-04）：仅绑定面板不激活视口（源视口是云图视口，激活链会切走面板）

        //更新流线图视口名称
        ensurePanel(_postSetPanel)->setCurViewName(viewname);
    }

    void mPostInfoHub::editSetByName(const QString& name)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postSetPanel).get());

        ensurePanel(_postSetPanel)->editSetByName(name);
    }

    void mPostInfoHub::notifySetRenamed(const QString& oldName, const QString& newName)
    {
        if (_postSetPanel)
            (*_postSetPanel).onSetRenamed(oldName, newName);
    }

    void mPostInfoHub::notifySetsDeleted(const QStringList& names)
    {
        if (_postSetPanel)
            (*_postSetPanel).onSetsDeleted(names);
    }

 void mPostInfoHub::createLine(int caseid)
	{
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postLinePanel).get());

        ensurePanel(_postLinePanel)->newLinePanel(caseid);
	}

    void mPostInfoHub::editLine(QString viewname)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postLinePanel).get());

        ensurePanel(_postLinePanel)->setCurViewName(viewname);
    }

    void mPostInfoHub::createArrow(int caseid)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postArrowPanel).get());

        ensurePanel(_postArrowPanel)->newArrowPanel(caseid);
    }

    void mPostInfoHub::editArrow(QString viewname)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postArrowPanel).get());

        ensurePanel(_postArrowPanel)->setCurViewName(viewname);
    }

    void mPostInfoHub::createText(int caseid)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postTextPanel).get());

        ensurePanel(_postTextPanel)->newTextPanel(caseid);
    }

    void mPostInfoHub::editText(QString viewname)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postTextPanel).get());

        ensurePanel(_postTextPanel)->setCurViewName(viewname);
    }

    void mPostInfoHub::createLog(int caseid)
    {
        MechCommon::panelHostRef()->unEditInfoWindow();
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postLogPanel).get());

        ensurePanel(_postLogPanel)->newLogPanel(caseid);
    }

    void mPostInfoHub::editLog(QString viewname)
    {
        mDockLayoutManager::getInstance()->activateDock("Info");
        mountToHost(ensurePanel(_postLogPanel).get());

        ensurePanel(_postLogPanel)->setCurViewName(viewname);
    }

 void mPostInfoHub::createPath(int caseid)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postPathPanel).get());
		ensurePanel(_postPathPanel)->editPathForViewport(caseid);
	}

    void mPostInfoHub::editPath(QString viewname)
	{
		MechCommon::panelHostRef()->unEditInfoWindow();

		mDockLayoutManager::getInstance()->activateDock("Info");
		mountToHost(ensurePanel(_postPathPanel).get());

		//更新节点名称方便获取曲线数据
		ensurePanel(_postPathPanel)->setCurViewName(viewname);

		ensurePanel(_postPathPanel)->updatePathGUI();
	}

	void mPostInfoHub::uneditPath()
	{
		if (_postPathPanel)
			_postPathPanel->uneditPathPanel();
	}
}

