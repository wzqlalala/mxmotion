#include "mPostIsosurface.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QDoubleValidator>
#include <QTimer>
#include <algorithm>
#include <cmath>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"

//MxFunctions
#include "mGlobalSignals.h"
#include "mOtherFunction.h"
//共享 UI 库控件（V1.3 等值面面板收敛，差异表 2026-11-14）
#include "mPrimaryButton.h"
#include "mChoiceGroup.h"
#include "mRangeSliderRow.h"
#include "mStepAdjustRow.h"
#include "mSlider.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "mxr/data/IPostRenderDataProxy.h"

//窗口
#include "mModelData.h"
#include "mPostInfoHub.h"
#include "mPost3DResult.h"
#include "mCommonFunctions.h"   // S5：G5 台账寻址助手（PostCommon）

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;

namespace
{
    //值域直显文本（同原型数字直显：整数不带小数，其余最多 6 位并去尾零，0→"0"、1→"1"）
    QString isoBoundText(double v)
    {
        QString s = QString::number(v, 'f', 6);
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
        return s;
    }
}

namespace PostWindow
{
	mPostIsosurfacePanel::IsoUIState& mPostIsosurfacePanel::uiState(const QString& viewName)
	{
		return _uiStates[viewName];
	}

	REBUILD_PostRend* mPostIsosurfacePanel::boundPostRend() const
	{
		//按绑定视口寻址（动画链同款getPostRenderByViewName）：激活视口≠绑定视口时
		//面板操作仍作用到绑定视口，不再误入当前视口的渲染器
		if (_curViewName.isEmpty()) return nullptr;
		return getCVM->getPostRenderByViewName(_curViewName);
	}

	MechPreData::mViewportPrivate* mPostIsosurfacePanel::boundViewportPrivate() const
	{
		if (_curViewName.isEmpty()) return nullptr;
		return getCVM->getViewportPrivateByName(_curViewName);
	}

	void mPostIsosurfacePanel::scheduleRegen()
	{
		//拖动/输入类实时重生成统一走 40ms 尾沿去抖（见构造函数注释）
		_regenTimer->start();
	}

	void mPostIsosurfacePanel::restoreBoundShowMode(mxr::REBUILD_PostRender* postRender, const IsoUIState& state)
	{
		if (postRender == nullptr) return;
		//优先生成时捕获的 savedShowMode；未捕获（异常/旧状态）回退绑定视口当前勾选态
		int restoreMode = state.savedShowMode;
		if (restoreMode < 0 || restoreMode > 3)
		{
			if (MechPreData::mViewportPrivate* priv = boundViewportPrivate())
				restoreMode = priv->_showViewCheckedStatus;
		}
		switch (restoreMode)
		{
		case 0: postRender->setShowFuntion(ShowFuntion::WireEdge);
			break;
		case 1: postRender->setShowFuntion(ShowFuntion::WireFrame);
			break;
		case 2: postRender->setShowFuntion(ShowFuntion::ElementFace);
			break;
		case 3: postRender->setShowFuntion(ShowFuntion::SmoothShaded);
			break;
		}
	}

	mPostIsosurfacePanel::mPostIsosurfacePanel(QWidget* parent) : mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);

		/********************操作条********************/
		QWidget* actionBar = new QWidget(this);
		QHBoxLayout* actionBarLayout = new QHBoxLayout(actionBar);
		actionBarLayout->setContentsMargins(4, 4, 4, 2);
		actionBarLayout->setSpacing(6);
		_btnReuse = new QPushButton("复用云图设置");
		_btnClear = new QPushButton("清除等值面");
		_btnClear->setEnabled(false);
		//V1.3：生成等值面=原型 symmetry-execute 恒主色（共享库 mPrimaryButton，
		//样式统一取令牌，不再本地 QSS；云图"应用渲染"同款）
		_btnGenerate = new MxFunctions::mPrimaryButton("生成等值面");
		_btnGenerate->setObjectName("btnIsoGenerate");
		//三按钮等宽且允许随窄栏压缩（全局 QSS 的 width:80 与文字宽不再是硬下限）
		for (QPushButton* btn : { static_cast<QPushButton*>(_btnReuse), static_cast<QPushButton*>(_btnClear),
			static_cast<QPushButton*>(_btnGenerate) })
		{
			btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
			actionBarLayout->addWidget(btn, 1);
		}
		_mainLayout->addWidget(actionBar);

		/* 云图变量/对象及坐标系/数据处理：共享折叠组，面板激活时自云图面板物理接管
		   （takeSharedSections 插入到操作条与等值面样式之间），此处不重复构造 */

	/********************等值面样式********************/
	//组头恒显示（原型 details 恒在）：字段行初始隐藏，点击"生成等值面"成功后
	//显示（setStyleRowsVisible 行级显隐），清除等值面/切到未生成视口时再隐藏
	_styleFold = new FoldSection("等值面样式", this);

	//范围行（滑块与上下限）=库 mRangeSliderRow 共框拼接行（原型 range-composite：
	//只读下限|滑块|只读上限 1:2:1）。初始留空——点击"生成等值面"后才回填当前值域。
	//滑条 0-1000=归一化位置（QSlider int 精度必需；原型滑条值域为实际值，功能
	//等价，差异表 #10 记口径）
	_rangeRow = new MxFunctions::mRangeSliderRow(_styleFold);
	_sliderThreshold = _rangeRow->slider();
	_sliderThreshold->setRange(0, 1000);
	_sliderThreshold->setValue(500);
	_styleFold->addRow(_rangeRow);

	//当前阈值（归一化 0-1，实际值=下限+(上限-下限)*阈值）
	_editThreshold = new QLineEdit();
	_editThreshold->setObjectName("isoThresholdEdit");
	_editThreshold->setText("0.5");
	//当前阈值为实际值（与范围栏同单位，量级随变量/数据而变）：不设固定上下限，
	//越界输入由 thresholdActual 钳到值域内；默认 0.5 对应未生成时默认值域 0-1 的中点
	_editThreshold->setValidator(new QDoubleValidator(-1e300, 1e300, 6, this));
	_styleFold->addRow(new PanelLabel("当前阈值"), _editThreshold);

	//阈值调整行=库 mStepAdjustRow 共框 [‹ | 步长 | ›]（原型 isosurface-adjustment 1:2:1）。
	//步长=实际值增量（原型 adjustIsosurfaceThreshold：threshold ± step）：单次移动
	//幅度与范围栏同单位；>0 任意有限值，默认 0.01。±触界禁用见 updateAdjustButtons
	_adjustRow = new MxFunctions::mStepAdjustRow(_styleFold);
	_adjustRow->setDecTooltip(QString::fromUtf8("按步长减小阈值"));
	_adjustRow->setIncTooltip(QString::fromUtf8("按步长增大阈值"));
	_editStep = _adjustRow->stepEdit();
	_editStep->setObjectName("isoStepEdit");
	_editStep->setText("0.01");
	_editStep->setValidator(new QDoubleValidator(0.000001, 1e9, 6, this));
	_styleFold->addRow(new PanelLabel("阈值调整"), _adjustRow);

	//生成形式=库 mChoiceGroup 共框圆点单选（原型 mirror-planes）：小于(0)/等于(1)/
	//大于(2)，默认等于。等于=阈值处等值面；大于/小于=保留≥/≤阈值一侧的封闭区域壳，
	//经 createContourGraph 比较模式参数下传渲染端
	_choiceComparison = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("小于"), QString::fromUtf8("等于"), QString::fromUtf8("大于") }, _styleFold);
	_choiceComparison->setObjectName("isoComparisonGroup");
	_choiceComparison->setCurrentIndex(1);
	_styleFold->addRow(new PanelLabel("生成形式"), _choiceComparison);

	//窄参数栏自适应：生成形式单选/范围行的字段最小宽度超出可用列宽时字段自动换行
	//到标签下方，宽栏仍为两列——否则这些行把面板最小宽度顶高，与云图面板同宽对比
	//时表现为整面板不压缩、右侧裁切
	_styleFold->layout()->setRowWrapPolicy(QFormLayout::WrapLongRows);

	//字段行初始隐藏（生成后经 setStyleRowsVisible(true) 逐行显示）
	setStyleRowsVisible(false);
	_styleFold->refreshLayout();
	m_foldWidgets.append(_styleFold);
	_mainLayout->addWidget(_styleFold);
	_mainLayout->addStretch();

	//连接信号槽
	connect(_btnReuse, &QPushButton::clicked, this, &mPostIsosurfacePanel::slot_reuseContourSettings);
	connect(_btnClear, &QPushButton::clicked, this, &mPostIsosurfacePanel::slot_clearIsosurface);
	connect(_btnGenerate, &QPushButton::clicked, this, &mPostIsosurfacePanel::slot_generateIsosurface);
	connect(_sliderThreshold, &QSlider::valueChanged, this, &mPostIsosurfacePanel::slot_thresholdSliderMoved);
	connect(_editThreshold, &QLineEdit::textEdited, this, &mPostIsosurfacePanel::slot_thresholdEdited);
	connect(_adjustRow, &MxFunctions::mStepAdjustRow::decClicked, this, &mPostIsosurfacePanel::slot_decreaseThreshold);
	connect(_adjustRow, &MxFunctions::mStepAdjustRow::incClicked, this, &mPostIsosurfacePanel::slot_increaseThreshold);
	connect(_choiceComparison, &MxFunctions::mChoiceGroup::currentIndexChanged,
		this, &mPostIsosurfacePanel::slot_comparisonChanged);
//实时重生成去抖（40ms 尾沿）：滑条拖动/阈值输入逐刻度全量重提取（会话扇出
//静态帧+各动画帧）大模型上卡顿；sliderReleased/按钮类入口立即生成收尾
_regenTimer = new QTimer(this);
	_regenTimer->setSingleShot(true);
	_regenTimer->setInterval(40);
	connect(_regenTimer, &QTimer::timeout, this, [this]() {
		if (_hasGenerated)
			generateIsosurface();
	});
	connect(_sliderThreshold, &QSlider::sliderReleased, this, [this]() {
		if (!_hasGenerated) return;
		_regenTimer->stop();
		generateIsosurface();
	});
}

	void mPostIsosurfacePanel::newIsosurfacePanel(int caseid)
	{
		QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
		if (postDB == nullptr) return;

		//绑定链（createIsosurfaceSig→slot_updateIsosurfaceTree→editIsosurface→setCurViewName）；
		//不登记等值面数据——派生去节点化后管理器不再登记 IsosurfaceInfo 条目，
		//addIsosurfaceData 对无条目视口是静默空操作（生成态/阈值改存面板 _uiStates）
		MechCommon::mMechGlobalSignals::getInstance()->createIsosurfaceSig();
	}

	QString mPostIsosurfacePanel::getCloudViewName(QString view)
	{
		//源云图定位：优先管理器关系（IsosurfaceInfo.viewName → resultId → CloudItem.viewName）
		//——显示名接管后视口名不再携带 "_Isosurface<N>" 后缀，名字解析只作兜底
		//（派生视口创建信号先于 addItem 登记的窗口期，彼时视口名仍带后缀）
		const QString byInfo = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post)
			.getCloudViewNameByView(PostData::ItemType::IsosurfaceItem, view);
		if (!byInfo.isEmpty())
			return byInfo;
		QRegularExpression re("^(.*)(_Isosurface\\d+)$");
		QRegularExpressionMatch match = re.match(view);
		if (match.hasMatch()) {
			return match.captured(1);
		}
		return view;
	}

	double mPostIsosurfacePanel::thresholdActual() const
	{
		//阈值框内容为实际值（与范围栏同单位）：解析后钳到值域内；解析失败取值域中点
		bool ok = false;
		double v = _editThreshold->text().trimmed().toDouble(&ok);
		if (!ok)
			v = _rangeMin + 0.5 * (_rangeMax - _rangeMin);
		if (_rangeMax > _rangeMin)
			return std::clamp(v, _rangeMin, _rangeMax);
		return std::clamp(v, 0.0, 1.0);
	}

	void mPostIsosurfacePanel::setThresholdActual(double actual)
	{
		double t = 0.5;
		if (_rangeMax > _rangeMin)
			t = std::clamp((actual - _rangeMin) / (_rangeMax - _rangeMin), 0.0, 1.0);
		else
			t = std::clamp(actual, 0.0, 1.0);

		_isSyncing = true;
		_editThreshold->setText(isoBoundText(actual));
		_sliderThreshold->setValue(int(t * 1000 + 0.5));
		_isSyncing = false;
		updateAdjustButtons();
	}

	void mPostIsosurfacePanel::generateIsosurface(bool reanchorBasis)
	{
		//挂起的去抖重生成失效（防双发；防视口切换后旧视口的挂起请求串到新视口）
		if (_regenTimer != nullptr)
			_regenTimer->stop();

		//按绑定视口寻址（激活视口≠绑定视口时仍作用到绑定视口）
		REBUILD_PostRend* postRend = boundPostRend();
		if (postRend == nullptr) return;

		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		//值域恢复/重锚并回填范围栏（用户逻辑：点击"生成等值面"后范围栏才有内容），
		//reanchorBasis 时强制按当前云图图例重锚（复用云图设置入口）；
		//同时等值面样式字段行显示、清除按钮解除禁用
		updateIsoBounds(reanchorBasis);
		fillIsoBoundsView();
		setStyleRowsVisible(true);

		//首次生成（含清除后再生成）阈值取值域中点：对齐旧版归一化默认 0.5 的语义。
		//实际值语义下默认文本"0.5"在小量级数据上越界、被 thresholdActual 钳到值域
		//端点，"等于"模式在端点上无跨越单元、提取为空（表现为"点生成没等值面，
		//拖一下滑块才有"）
		IsoUIState& state = uiState(_curViewName);
		const bool firstGenerate = !state.generated;
		if (firstGenerate)
			setThresholdActual(0.5 * (_rangeMin + _rangeMax));

		double actual = thresholdActual();

		//生成态与阈值（实际值+归一化位置）存面板进程态（按视口名）：管理器 IsosurfaceInfo 条目
		//已随派生去节点化不再登记，guidata 通道（addIsosurfaceData）对无条目视口是
		//静默空操作，无法作数据源——重开面板/切面板回来据此恢复。
		//thresholdNorm 供切变量重锚值域时按归一化位置重定位实际值（slot_cloudRedrawn），
		//避免旧变量实际值钳到新值域端点、"等于"模式端点无跨越单元空提取
		state.generated = true;
		state.threshold = actual;
		state.thresholdNorm = (_rangeMax > _rangeMin)
			? std::clamp((actual - _rangeMin) / (_rangeMax - _rangeMin), 0.0, 1.0)
			: 0.5;

		//生成形式映射渲染端比较模式：等于=阈值处等值面；大于/小于=保留
		//≥/≤阈值一侧的封闭区域壳（等值面+模型边界封盖，按场值渐变上色）
		mxr::ContourCompareMode mode = mxr::ContourCompareMode::Equal;
		if (state.comparison == QString("大于"))
			mode = mxr::ContourCompareMode::GreaterOrEqual;
		else if (state.comparison == QString("小于"))
			mode = mxr::ContourCompareMode::LessOrEqual;

		//【契约补齐 2026-11-10】比较模式经 ContourParams.compareMode（int，随 value
		//更新一并生效）→会话 createContourGraphInternal(i,value,isShow,mode)→单帧
		//渲染器透传，"大于/小于"区域壳正式接通（2026-11-06 曾因 ContourParams 无该
		//字段、会话层不带 mode 降级为恒 Equal）。备注：单帧渲染器
		//recreateContourGraphIfActive 走 num 重建路径不携带 mode（重置默认 Equal），
		//num 型等值线若将来需要生成形式须在该路径另行持久化。
		//重复 create 为替换语义（同旧版表格实时生成）——S5 G5（D9 载体）：先寻/登
		//等值线台账条目（count=1→extIndex=0），再 updateObject(index/value/isShow/
		//compareMode) 按值创建（旧 createContourGraph(0,actual,true) 语义承接；重复
		//生成为同 id 覆盖）
		mxr::ContourParams contourCreate;
		contourCreate.count = 1;
		const int contourId = PostCommon::ensureSceneObjectId(postRender.get(), contourCreate);
		mxr::ContourParams contourUpdate;
		contourUpdate.index = 0;   // 寻址必填（登记条目 extIndex=0 同值）
		contourUpdate.value = actual;
		contourUpdate.isShow = true;
		contourUpdate.compareMode = static_cast<int>(mode);
		contourUpdate.updateMask = mxr::ContourParams::Mask_Value | mxr::ContourParams::Mask_IsShow
			| mxr::ContourParams::Mask_CompareMode;
		postRender->updateObject(contourId, contourUpdate);

		//首次生成时捕获绑定视口的用户显示模式（清除等值面时恢复依据，见
		//restoreBoundShowMode）。每次生成都重申 WireEdge：云图重绘链 setShowMode 会
		//把渲染器显示模式恢复为用户勾选态，重提取后不重申等值面会被不透明着色挡住
		//（2026-11-05 立方体模型实测即此）
		if (firstGenerate)
		{
			if (MechPreData::mViewportPrivate* priv = boundViewportPrivate())
				state.savedShowMode = priv->_showViewCheckedStatus;
		}
		postRender->setShowFuntion(ShowFuntion::WireEdge);
		postRend->updatePaint();

		_hasGenerated = true;
		_btnClear->setEnabled(true);
		updateAdjustButtons();
	}

	void mPostIsosurfacePanel::updateIsoBounds(bool forceCapture)
	{
		//按绑定视口寻址：图例取绑定视口渲染器的（激活视口≠绑定时不再锚错值域）
		REBUILD_PostRend* postRend = boundPostRend();
		if (postRend == nullptr) return;

		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		//范围取关联云图视口的当前图例最值（仅内部计算，界面回填由 fillIsoBoundsView 在生成后执行）
		MechPreData::mPostGUIData* cloudGui = getCVM->getGUIDataByViewName(getCloudViewName(_curViewName));
		if (cloudGui == nullptr) return;

		//变量身份（不含帧）。已生成且身份未变＝纯切帧：恢复生成时刻锚定的基准值域，
		//阈值（实际值）/滑条/范围栏随切帧保持不动（HyperView 语义——图例值域跨帧稳定、
		//同一实际值逐帧提取；本云图图例逐帧变化，以生成时刻锚定近似该稳定性）
		IsoUIState& state = uiState(_curViewName);
		QString basisKey = cloudGui->_cloudCase + QChar('\x1f') + cloudGui->_cloudVarName + QChar('\x1f') +
			cloudGui->_cloudCompo + QChar('\x1f') + cloudGui->_cloudLayerName + QChar('\x1f') + cloudGui->_cloudComplex;
		if (!forceCapture && state.generated && state.basisKey == basisKey && state.basisMax > state.basisMin)
		{
			_rangeMin = state.basisMin;
			_rangeMax = state.basisMax;
			return;
		}

		//首帧锚定/切换变量/复用云图设置强制重锚：优先取渲染端图例"当前显示帧"的
		//值域。动画链不写 guidata 最值——动画生成后视图停靠在动画帧、图例随播放
		//SetFrameID 逐帧切换，guidata 却仍是动画前静态帧的最值，直接取 guidata 会
		//锚到与显示脱节的帧值域（如动画停在末帧时锚成首帧值域，范围栏与图例相差
		//数倍）；静态路径 slot_showCloud 末尾把图例同步为当前帧最值，故图例读取在
		//两种模式下都恒等于用户所见。图例无效（该帧未写入/被哨兵拦截为 0-0）时
		//回退 guidata，再无效保持默认 0-1
		double minval = 0;
		double maxval = 1;
		bool fromLegend = false;
		if (auto* legend = postRender->getColorTableWindow())
		{
			float legendMin = 0.f;
			float legendMax = 0.f;
			legend->getValueMinMax(legend->GetFrameID(), legendMin, legendMax);
			if (legendMax > legendMin && std::isfinite(legendMin) && std::isfinite(legendMax))
			{
				minval = legendMin;
				maxval = legendMax;
				fromLegend = true;
			}
		}
		if (!fromLegend)
		{
			minval = cloudGui->_cloudMinValue;
			maxval = cloudGui->_cloudMaxValue;
			if (maxval <= minval)
			{
				//尚未取得有效图例范围时保持默认 0-1
				minval = 0;
				maxval = 1;
			}
		}

		_rangeMin = minval;
		_rangeMax = maxval;
		state.basisKey = basisKey;
		state.basisMin = minval;
		state.basisMax = maxval;
	}

	void mPostIsosurfacePanel::fillIsoBoundsView()
	{
		_rangeRow->setLowerText(isoBoundText(_rangeMin));
		_rangeRow->setUpperText(isoBoundText(_rangeMax));
	}

	void mPostIsosurfacePanel::clearIsoBoundsView()
	{
		//未生成等值面时范围栏不显示内容（用户逻辑：点击"生成等值面"后才有值）
		_rangeRow->setLowerText(QString());
		_rangeRow->setUpperText(QString());
	}

	void mPostIsosurfacePanel::takeSharedSections()
	{
		if (_sectionsTaken) return;

		auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel();
		if (cloudPanel == nullptr) return;

		QList<FoldSection*> sections = cloudPanel->sharedSections();
		if (sections.isEmpty()) return;

		//先从云图面板布局移除，再插入本面板（操作条之后、等值面样式之前），
		//insertWidget 自动 reparent——单一实例单一数据源，变量切换仍走云图面板既有槽
		cloudPanel->detachSharedSections();
		//等值面模式下隐藏云图变量组内的"变形系数"行（同原型共享参数块行为）
		cloudPanel->setDeformationRowVisible(false);
		int insertRow = 1;   //_mainLayout: 0=操作条
		for (FoldSection* section : sections)
		{
			if (section == nullptr) continue;
			_mainLayout->insertWidget(insertRow++, section);
			section->refreshLayout();
		}
		_sectionsTaken = true;

		//接管后恢复共享拾取条可见：云图面板 hideEvent 的 setPickWidgetVisible(false)
		//已把拾取条显式隐藏（挂载时序为先切面板后接管，隐藏发生在搬入本面板之前），
		//显式隐藏跨 reparent 保持、refreshLayout 不恢复——不在此恢复则"对象选择"行
		//只剩类型下拉，与云图不一致；恢复入口必须在接管方（云图面板在等值面激活
		//期间恒隐藏，其 showEvent 走不到）
		cloudPanel->setSharedObjectPickVisible(true);

		//云图重绘联动（首次接管时建立，常驻不随归还断开——云图面板独立编辑时
		//切变量/切帧同样要跟随）：见 slot_cloudRedrawn
		connect(cloudPanel.get(), &mPost3DResultPanel::cloudRedrawnSig, this, &mPostIsosurfacePanel::slot_cloudRedrawn);
	}

	void mPostIsosurfacePanel::returnSharedSections()
	{
		if (!_sectionsTaken) return;

		auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel();
		if (cloudPanel == nullptr) return;

		//cloudRedrawnSig 连接保留（等值面随云图重绘自动重提取，与折叠组归属无关）

		//先从本面板布局移除，再由云图面板插回原位（避免一 widget 同时在两个布局）
		QList<FoldSection*> sections = cloudPanel->sharedSections();
		for (FoldSection* section : sections)
		{
			if (section == nullptr) continue;
			_mainLayout->removeWidget(section);
		}
		//归还云图面板时恢复"变形系数"行显示
		cloudPanel->setDeformationRowVisible(true);
		cloudPanel->reattachSharedSections();
		_sectionsTaken = false;
	}

	void mPostIsosurfacePanel::slot_cloudRedrawn(const QString& viewName)
	{
		//仅跟随本面板绑定视口的重绘（云图面板可能正在编辑其它视口）
		if (viewName != _curViewName)
			return;

		//云图变量/帧变化重绘已销毁等值面（渲染器整体重建），已生成时按存储阈值
		//自动重提取（HyperView 语义）：纯切帧时基准值域保持——阈值（实际值）/
		//滑条/范围栏不动，同一实际值在新帧提取，新帧数据未跨阈值的帧无等值面；
		//切变量时基准重锚新变量值域，阈值按生成时的归一化位置重定位——旧变量
		//实际值直接钳制可能落在新值域端点，"等于"模式端点无跨越单元、提取为空
		//（表现为"切了变量等值面消失"）
		IsoUIState& state = uiState(_curViewName);
		const QString basisBefore = state.basisKey;
		updateIsoBounds();
		if (_hasGenerated)
		{
			fillIsoBoundsView();
			if (state.basisKey != basisBefore)
			{
				//切变量：新值域下按归一化位置重定位实际值（HyperView 归一化保持语义）
				const double repositioned = _rangeMin + (_rangeMax - _rangeMin) * state.thresholdNorm;
				state.threshold = repositioned;
				setThresholdActual(repositioned);
			}
			else
			{
				setThresholdActual(state.threshold);
			}
			generateIsosurface();
		}
	}

	void mPostIsosurfacePanel::applyComparisonFromUI()
	{
		IsoUIState& state = uiState(_curViewName);
		switch (_choiceComparison->currentIndex())
		{
		case 0:  state.comparison = QString::fromUtf8("小于"); break;
		case 2:  state.comparison = QString::fromUtf8("大于"); break;
		default: state.comparison = QString::fromUtf8("等于"); break;
		}

		//步进值随界面输入更新（实际值增量语义：>0 任意有限值，默认 0.01——
		//原型 isosurfaceAdjustmentStep，"请输入大于 0 的调整步长"）
		bool ok = false;
		double step = _editStep->text().trimmed().toDouble(&ok);
		if (ok && step > 0)
			state.step = std::clamp(step, 0.000001, 1e9);
	}

	void mPostIsosurfacePanel::nudgeHostResize()
	{
		//面板最小尺寸变化（共享折叠组迁入/等值面样式组显隐）后同步 Info dock 滚动区：
		//宿主被 ADS 包在 QScrollArea(widgetResizable) 里，Qt 只在宿主自身 resize 时
		//重算滚动范围（eventFilter 监听 Resize），不感知内容最小尺寸变化——显式触发
		//一次宿主 resize(+1px)，滚动区按 max(视口,最小尺寸) 收放，多出 1px 自动收回
		QTimer::singleShot(0, this, [this]() {
			QWidget* host = parentWidget();
			if (host != nullptr)
				host->resize(host->size() + QSize(0, 1));
			});
	}

	void mPostIsosurfacePanel::setStyleRowsVisible(bool show)
	{
		//原型 isosurfaceGeneratedFields：组头"等值面样式"恒在，仅字段行按生成态
		//显隐；行级 takeRow/insertRow 由 FoldSection::setRowVisible 承担（含高度帽
		//同步——QFormLayout 行 hide 仍摊行距，不能直接 setVisible）
		for (QWidget* editor : { static_cast<QWidget*>(_rangeRow), static_cast<QWidget*>(_editThreshold),
			static_cast<QWidget*>(_adjustRow), static_cast<QWidget*>(_choiceComparison) })
		{
			if (editor != nullptr)
				_styleFold->setRowVisible(editor, show);
		}
		nudgeHostResize();
	}

	void mPostIsosurfacePanel::updateAdjustButtons()
	{
		//±触界禁用（原型 updateIsosurfaceGeneration）：未生成双禁；阈值到值域端点
		//禁对应侧。以滑条归一化位置判定（int 精确）——文本框 6 位舍入可能落在端点
		//上方一丝，直接比实际值会漏禁
		if (!_hasGenerated || _adjustRow == nullptr)
		{
			if (_adjustRow != nullptr)
			{
				_adjustRow->setDecEnabled(false);
				_adjustRow->setIncEnabled(false);
			}
			return;
		}
		_adjustRow->setDecEnabled(_sliderThreshold->value() > 0);
		_adjustRow->setIncEnabled(_sliderThreshold->value() < 1000);
	}

	void mPostIsosurfacePanel::setCurViewName(const QString& viewName)
	{
		_curViewName = viewName;

		//视口切换：挂起的去抖重生成属于旧视口，取消（generateIsosurface 内亦有 stop 双保险）
		if (_regenTimer != nullptr)
			_regenTimer->stop();

		//接管前先把云图面板编辑上下文同步到本面板绑定的源云图视口：共享折叠组
		//（对象及坐标系/云图变量/数据处理）的显示回填与写入槽全部锚在云图面板
		//_curViewName 上（updateResultInfoGUI/slot_chooseObjectType/slot_showCloud 等），
		//上下文漂移时（多云图视口先后编辑后再开等值面）接管期间的编辑会落到别的
		//视口——须与云图面板自身编辑同语义（轻量同步不重绘不回收折叠组，见其实现）
		if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
			cloudPanel->syncEditContextToView(getCloudViewName(_curViewName));

		//接管云图面板共享折叠组（云图变量/对象及坐标系/数据处理）。
		//必须先于下方数据判空执行：首次创建等值面时共享分组接管只依赖视口名
		//与云图面板，不依赖等值面数据
		takeSharedSections();

		//共享折叠组运行时迁入使面板最小尺寸增大，滚动范围同步见 nudgeHostResize
		nudgeHostResize();

		//按绑定视口寻址：下方清理/重建都作用到绑定视口自身的渲染器（多视口各自
		//的等值面互不影响；激活≠绑定时不再误清当前视口的渲染对象）
		REBUILD_PostRend* postRend = boundPostRend();
		if (postRend == nullptr) { clearIsoBoundsView(); setStyleRowsVisible(false); return; }

		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) { clearIsoBoundsView(); setStyleRowsVisible(false); return; }

		PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::Contour);   // S5 G5：deleteContourGraph() 全删→台账家族摘除（内部承担渲染删除+清台账）
		postRend->updatePaint();

		//回填进程内状态（生成形式/步进）。跨视口生成形式不同时 setCurrentIndex 会
		//发射 currentIndexChanged→slot_comparisonChanged，而此刻 _hasGenerated 尚为
		//旧视口的 true（下方才复位），会提前对新视口 generateIsosurface——回填必须
		//blockSignals（同旧 radio 回填口径；下方 generated 分支的重建是唯一入口）
		IsoUIState& state = uiState(_curViewName);
		_choiceComparison->blockSignals(true);
		if (state.comparison == QString("小于"))
			_choiceComparison->setCurrentIndex(0);
		else if (state.comparison == QString("大于"))
			_choiceComparison->setCurrentIndex(2);
		else
			_choiceComparison->setCurrentIndex(1);
		_choiceComparison->blockSignals(false);
		_editStep->setText(isoBoundText(state.step));

		//回填阈值并恢复已生成的等值面（生成态/阈值存面板进程态 _uiStates，按视口名——
		//见 generateIsosurface 注释）；已生成时值域按存储基准恢复（变量已换则重锚）并
		//回填范围栏、等值面样式字段行显示，未生成时范围栏留空、字段行隐藏、阈值复位
		//默认 0.5（用户逻辑：点击"生成等值面"后范围栏与样式组才有内容）
		_hasGenerated = false;
		_btnClear->setEnabled(false);
		updateAdjustButtons();
		if (state.generated)
		{
			updateIsoBounds();
			setThresholdActual(state.threshold);
			//generateIsosurface 内部含 createContourGraph/setShowFuntion(WireEdge)/updatePaint，
			//按存储阈值确定性重建（渲染态与面板态保持一致）
			generateIsosurface();
		}
		else
		{
			clearIsoBoundsView();
			setStyleRowsVisible(false);
			//阈值复位默认 0.5（同 setCurViewName 未生成分支的"用户逻辑"）：同时值域
			//复位默认 0-1，0.5 才是与实际值语义一致的中点
			_rangeMin = 0;
			_rangeMax = 1;
			_isSyncing = true;
			_editThreshold->setText("0.5");
			_sliderThreshold->setValue(500);
			_isSyncing = false;
			updateAdjustButtons();
		}
	}

	void mPostIsosurfacePanel::restoreIsosurfaceIfGenerated(const QString& viewName)
	{
		if (viewName.isEmpty()) return;
		IsoUIState& state = uiState(viewName);
		if (!state.generated) return;

		//动画生成后恢复（面板此时通常隐藏——动画面板挂载；界面重绑由后续
		//setCurViewName 确定性收口，这里只管渲染端）。与 setCurViewName 重建
		//分支同口径：值域→按存储阈值回填→确定性重建（contour 广播落静态帧+各动画帧）
		_curViewName = viewName;
		updateIsoBounds();
		setThresholdActual(state.threshold);
		generateIsosurface();
	}

	void mPostIsosurfacePanel::slot_reuseContourSettings()
	{
		//共享折叠组显示的即云图当前设置：已生成过等值面时按当前云图图例值域重锚
		//基准并重生成——也是切帧后用户主动把基准对齐到当前帧图例的显式入口。
		//先重锚并按存储实际阈值同步显示（换基准后滑条位置需在新值域下重定位，
		//generateIsosurface(true) 内部的再次重锚为幂等重取）；未生成时范围栏维持留空
		if (_hasGenerated)
		{
			updateIsoBounds(true);
			setThresholdActual(uiState(_curViewName).threshold);
			generateIsosurface(true);
		}
	}

	void mPostIsosurfacePanel::slot_clearIsosurface()
	{
		//清除唯一入口（用户定案 2026-11-05）：面板切换/重开不再清等值面，只有本按钮。
		//渲染清理按绑定视口寻址（激活视口≠绑定视口时清绑定视口），上下文可得时执行
		//（渲染缺失则跳过渲染清理，避免静默失败连累界面复位）；生成态复位与界面复位
		//（清除按钮禁用+范围栏留空+样式字段行隐藏）无条件执行
		if (_regenTimer != nullptr)
			_regenTimer->stop();
		IsoUIState& state = uiState(_curViewName);
		state.generated = false;
		if (REBUILD_PostRend* postRend = boundPostRend())
		{
			if (auto postRender = postRend->getPostRender())
			{
				//渲染清理（S5 G5）：deleteContourGraph() 已随旧接口面消亡，改台账家族
				//整族摘除（会话扇出等价旧全删，静态帧+动画帧全覆盖）
				PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::Contour);

				//等值面已清：恢复用户显示模式（生成期间强制的 WireEdge 随清除还原；
				//面板切换不再还原——见 hideEvent，保持线框的依凭随生成态一并消失）。
				//恢复依据生成时捕获的 savedShowMode（绑定视口），未捕获回退其当前勾选态
				restoreBoundShowMode(postRender.get(), state);
				postRend->updatePaint();
			}
		}

		_hasGenerated = false;
		_btnClear->setEnabled(false);
		clearIsoBoundsView();
		setStyleRowsVisible(false);

		//阈值复位默认 0.5（同 setCurViewName 未生成分支的"用户逻辑"）：清除=回到
		//未生成初始态，所有随生成出现的界面一并复位；不复位则清除前拖到极端位置的
		//阈值会带进下一次生成（如小于模式拖到 1.0 顶格后清除再生成，阈值仍顶格、
		//保留侧壳覆盖整个模型，看起来像"拖动条范围坏了"）。值域一并复位默认 0-1，
		//0.5 才是与实际值语义一致的中点
		_rangeMin = 0;
		_rangeMax = 1;
		_isSyncing = true;
		_editThreshold->setText("0.5");
		_sliderThreshold->setValue(500);
		_isSyncing = false;
		updateAdjustButtons();
	}

	void mPostIsosurfacePanel::slot_generateIsosurface()
	{
		applyComparisonFromUI();
		generateIsosurface();
	}

	void mPostIsosurfacePanel::slot_thresholdSliderMoved(int value)
	{
		if (_isSyncing) return;

		_isSyncing = true;
		double t = std::clamp(value / 1000.0, 0.0, 1.0);
		//滑条是值域内归一化位置：拖动按位置换算实际值回填阈值框
		double actual = _rangeMin + (_rangeMax - _rangeMin) * t;
		_editThreshold->setText(isoBoundText(actual));
		_isSyncing = false;

		//已生成时滑块拖动实时重生成（原型"实时生成"语义；逐刻度全量重提取经
		//40ms 去抖合并，sliderReleased 立即收尾）
		if (_hasGenerated)
			scheduleRegen();
	}

	void mPostIsosurfacePanel::slot_thresholdEdited()
	{
		if (_isSyncing) return;

		bool ok = false;
		double v = _editThreshold->text().trimmed().toDouble(&ok);
		if (!ok) return;

		//输入为实际值：换算归一化位置同步滑条（不回写文本，保留用户输入；
		//越界钳制发生在 generateIsosurface→thresholdActual）
		double t = 0.5;
		if (_rangeMax > _rangeMin)
			t = std::clamp((v - _rangeMin) / (_rangeMax - _rangeMin), 0.0, 1.0);
		else
			t = std::clamp(v, 0.0, 1.0);

		_isSyncing = true;
		_sliderThreshold->setValue(int(t * 1000 + 0.5));
		_isSyncing = false;

		//已生成时输入实时重生成（与滑块拖动同语义，经 40ms 去抖合并连续击键）
		if (_hasGenerated)
			scheduleRegen();
	}

	void mPostIsosurfacePanel::slot_decreaseThreshold()
	{
		applyComparisonFromUI();
		const IsoUIState& state = uiState(_curViewName);

		//步进=实际值增量（原型 adjustIsosurfaceThreshold：threshold ± step）：
		//直接在实际值上减步长，钳到值域内；同步回填由 setThresholdActual 承担
		//（含±触界禁用刷新）
		const double v = std::clamp(thresholdActual() - state.step, _rangeMin, _rangeMax);
		setThresholdActual(v);

		if (_hasGenerated)
			generateIsosurface();
	}

	void mPostIsosurfacePanel::slot_increaseThreshold()
	{
		applyComparisonFromUI();
		const IsoUIState& state = uiState(_curViewName);

		//步进=实际值增量（同 slot_decreaseThreshold）
		const double v = std::clamp(thresholdActual() + state.step, _rangeMin, _rangeMax);
		setThresholdActual(v);

		if (_hasGenerated)
			generateIsosurface();
	}

	void mPostIsosurfacePanel::slot_comparisonChanged()
	{
		applyComparisonFromUI();

		//已生成时切换生成形式实时重生成（与滑块/阈值输入/±步进同一实时语义：
		//大于/小于改变的是区域壳的保留侧，属生成参数而非纯界面态）
		if (_hasGenerated)
			generateIsosurface();
	}

	void mPostIsosurfacePanel::showEvent(QShowEvent* event)
	{
		mPanelBase::showEvent(event);

		//拖动浮动/拖回停靠时 ADS 先隐藏再重显面板：hideEvent 已把共享折叠组归还
		//云图面板，重显时须重新接管，否则窗口里只剩操作条（takeSharedSections 有
		//_sectionsTaken 幂等保护，与 hide/show 循环严格配对）。
		//未绑定视口时维持"仅显示面板"语义（见 openIsosurfacePanel 注释），待 setCurViewName 再接管
		if (_curViewName.isEmpty()) return;
		takeSharedSections();
		nudgeHostResize();
	}

	void mPostIsosurfacePanel::hideEvent(QHideEvent* event)
	{
		//接管期间武装中的拾取随归还收口（清视口过滤器/高亮，防残留在视口上）：
		//只失活不改可见性——拾取条归还后随云图面板自身显隐呈现，与云图 showEvent
		//的恢复调用先后顺序无关（幂等）
		if (_sectionsTaken)
		{
			if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
				cloudPanel->deactivateSharedObjectPick();
		}

		//归还共享折叠组给云图面板（面板切走时，云图面板重新编辑也会主动回收）
		returnSharedSections();

		//等值面渲染随面板切换保持（用户定案 2026-11-05）：切到云图参数栏等其它面板
		//不再 deleteContourGraph——等值面直接作用于源云图视口，清除唯一入口是
		//"清除等值面"按钮（重开本面板经 setCurViewName 按存储阈值确定性重建）。
		//显示模式：本视口仍有已生成等值面时不恢复用户设定——生成时强制 WireEdge，
		//体模型的等值面多在模型内部，恢复不透明着色会把等值面整个挡住（"保持"
		//落空，用户实测立方体模型切云图后等值面看不见即此）；保持现有显示模式
		//直到清除等值面（slot_clearIsosurface 内恢复）或用户手动切换显示模式
		//
		//空绑定无面板态可依（也避免 uiState("") 落脏键），直接走基类
		if (!_curViewName.isEmpty() && !uiState(_curViewName).generated)
		{
			//未生成（或已清除）时按绑定视口归还显示模式（savedShowMode 优先，见
			//restoreBoundShowMode；渲染缺失时跳过归还，不连累基类收尾）
			if (REBUILD_PostRend* postRend = boundPostRend())
			{
				if (auto postRender = postRend->getPostRender())
				{
					restoreBoundShowMode(postRender.get(), uiState(_curViewName));
					postRend->updatePaint();
				}
			}
		}

		mPanelBase::hideEvent(event);
	}
}
