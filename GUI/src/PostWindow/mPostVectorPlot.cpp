#include "mPostVectorPlot.h"
#include "mFbdColorPicker.h"

#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QFrame>
#include <QHideEvent>
#include <QScrollArea>
#include <QScreen>
#include <QGuiApplication>
#include <QSpinBox>
#include <QtMath>
#include <algorithm>
#include <climits>
#include <functional>
#include <initializer_list>

#include "mChoiceGroup.h"
#include "mLegendBoundRow.h"
#include "mPostGeneralPickUp.h"      //对象选择拾取条工厂（createPickWidget；头文件已改前向声明，包含下沉至此）
#include "mPostPartScopeField.h"    //范围状态控制器（云图同款，对象及坐标系功能对齐）
#include "../PreData/PickCommonFunction.h"   //UpdatePostWidgetHighlight（拾取解除清缓冲高亮，云图同款）
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/mPostFrameText.h"   //信息标签三钮作用视口右上文本（云图同款）
#include <InteractionCore/InteractionSystem.h>                    //渲染图例自定义：视口交互系统（云图同款）
#include <InteractionCore/Dialog/Legend/CLegendEditorWidget.h>    //图例编辑器（与视口内双击图例同窗口）
#include "PostViewportWidget.h"                                   //interaction() 访问（同 mPost3DResult）
#include "mPostDeformation.h"     // readNodeVectorField（按变量读三分量矢量场）
#include "mCommonFunctions.h"     // PostCommon::removeAllSceneObjects（G5 台账家族删除）
#include "mPostInfoHub.h"         // cloudResultPanel（selectCaseByName 标准链）
#include "mPost3DResult.h"        // selectCaseByName 完整类型（hub 仅前向声明）
#include "mOtherFunction.h"       // mNumberToString（边界行回显 12 位小数，云图同口径）

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;

namespace
{
	//令牌（--std-* 标准档；出处 V1.3 原型 CSS 变量定义块）
	constexpr int kControl = 26;		//--std-control 控件外高
	constexpr int kInner = 24;			//连体内控件高（--std-control - 2）
	constexpr int kRadius = 4;			//--std-radius（V1.3：2→4）
	constexpr int kGap = 6;				//--std-gap
	constexpr int kColumnGap = 8;		//--std-column-gap
	constexpr int kChoiceGap = 4;		//--std-choice-gap
	constexpr int kGroupInset = 7;		//--std-group-inset
	constexpr int kHeader = 28;			//弹窗头高

	const char* kTextColor = "#303313";		//--std-text / --std-label
	const char* kMuted = "#788391";			//--std-muted
	const char* kLineBorder = "#DCE2E9";	//--std-border
	const char* kHoverBg = "#F0F7FF";		//--std-hover
	const char* kSelectedBg = "#E8F3FF";	//--std-selected
	const char* kPrimary = "#1890FF";		//--std-primary / --std-focus

	//主操作蓝按钮（应用渲染）：蓝底白字，悬停深蓝，禁用灰（--std-action/-hover）
	const char* kPrimaryBtnQss =
		"QPushButton{background:#1677C8;border:1px solid #1677C8;color:#FFFFFF;border-radius:4px;}"
		"QPushButton:hover{background:#1267B0;border:1px solid #1267B0;}"
		"QPushButton:disabled{background:#F5F7FA;border:1px solid #DCE2E9;color:#A6AFBB;}";

	//次级描边按钮（清除渲染/渲染图例三钮/重置）
	const char* kLineBtnQss =
		"QPushButton{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:4px;color:#303313;}"
		"QPushButton:hover{background:#F0F7FF;border-color:#1890FF;}"
		"QPushButton:disabled{background:#F5F7FA;color:#A6AFBB;}";

	//数字输入（白底描边圆角 4；QSS 占位符逐个写死防 arg 串位）
	QString numberEditQss()
	{
		return QString("QLineEdit{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:4px;"
			"padding:0 7px;color:#303313;}");
	}

	//帧名排序（云图面板同口径：按 "帧号:时间" 前缀数字升序）
	QStringList sortFrameNames(const QStringList& frameIds)
	{
		QStringList sortedList = frameIds;
		std::sort(sortedList.begin(), sortedList.end(), [](const QString& a, const QString& b)
			{
				return a.section(':', 0, 0).toInt() < b.section(':', 0, 0).toInt();
			});
		return sortedList;
	}

	//文本转有限数（失败返回 false）
	bool toFinite(const QString& text, double* value)
	{
		bool ok = false;
		const double v = text.trimmed().toDouble(&ok);
		if (!ok || !qIsFinite(v))
			return false;
		if (value)
			*value = v;
		return true;
	}

	//全程幅值域：全帧 readNodeVectorField 真读聚合（云图 allFrameRange 同款模式；
	//readNodeVectorField 自带懒加载链路，OP2/RST 等格式可逐帧读）。键控缓存
	//postName|var + 帧数失效——apply/切帧重建反复调用只首次真读；全帧读失败
	//返回 (0,0)（resolveVectorLegendRange 回退逐帧档）
	QPair<double, double> vectorAllFrameRange(PostData::mPostDataBase* postDB, const QString& postName,
		const QString& varName, const MechPreData::mPostGUIData* guidata)
	{
		static QHash<QString, QPair<double, double>> rangeCache;
		static QHash<QString, int> frameCountCache;
		if (postDB == nullptr)
			return { 0.0, 0.0 };
		const QList<int> frameIds = postDB->getFrameIds();
		if (frameIds.isEmpty())
			return { 0.0, 0.0 };
		const QString key = postName + QLatin1Char('|') + varName;
		const auto cached = frameCountCache.constFind(key);
		if (cached != frameCountCache.constEnd() && cached.value() == frameIds.count())
			return rangeCache.value(key);

		bool anyUsable = false;
		double vmin = 0.0, vmax = 0.0;
		for (int frame : frameIds)
		{
			const QHash<int, QVector3D> field = PostWindow::readNodeVectorField(postDB, postName, varName, frame, guidata);
			if (field.isEmpty())
				continue;
			for (auto it = field.cbegin(); it != field.cend(); ++it)
			{
				const double len = it.value().length();
				if (!anyUsable || len < vmin) vmin = len;
				if (!anyUsable || len > vmax) vmax = len;
				anyUsable = true;
			}
		}
		const QPair<double, double> result = anyUsable ? qMakePair(vmin, vmax) : qMakePair(0.0, 0.0);
		rangeCache.insert(key, result);
		frameCountCache.insert(key, frameIds.count());
		return result;
	}

	//整数文本校验随"图例自定义"子组一并删除（intInRange 无剩余调用方，2026-11-14）
}

namespace PostWindow
{
	/*
	* 向量图渲染三入口（面板 apply/clear 与云图切帧/重绘插桩共用）。
	* 参数存云图视口 guidata（源视口直作用模型：矢量图随云图视口存续，不建派生视口），
	* 渲染经 G5 台账 addObject(VectorGraphParams)——全帧实例扇出，每实例按自身帧号读
	* postDB 矢量哈希装配箭头（节点位置自动叠加变形/爆炸/切面裁剪）。
	*/
	bool applyVectorPlot(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata,
		QPair<double, double>* resolvedRange /*= nullptr*/)
	{
		if (postRend == nullptr || guidata == nullptr) return false;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return false;

		const QString postName = postRend->getPostDataName();
		PostData::mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (postDB == nullptr) return false;

		const int frame = guidata->_cloudFrame;
		const QString varName = guidata->_vectorPlotVarName;
		const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;

		//读场（readNodeVectorField：懒加载链路自带云图态保护；旧格式 mxdb 返回空）
		QHash<int, QVector3D> field = readNodeVectorField(postDB, postName, varName, frame, guidata);
		if (field.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, isChinese
				? QString("向量图变量%1在当前帧的数据读取失败，未生成矢量图。").arg(varName)
				: QString("Failed to read vector data of variable %1 at current frame. Vector plot not created.").arg(varName),
				true);
			return false;
		}
		postDB->setVectorDataByFrameID(frame, field);

		//B5 图例值域三档解析：逐帧=当前帧幅值现算；全程=全帧聚合（键控缓存）；
		//手动=guidata 锁定值（不可用回退逐帧）。解析结果写 guidata 供面板回显/复位
		const QPair<double, double> frameRange = detail::vectorFieldRange(field);
		QPair<double, double> allRange(0.0, 0.0);
		if (guidata->_vectorPlotLegendRange == QString::fromUtf8("全程"))
			allRange = vectorAllFrameRange(postDB, postName, varName, guidata);
		const QPair<double, double> range = detail::resolveVectorLegendRange(guidata->_vectorPlotLegendRange,
			guidata->_vectorPlotLegendMax, guidata->_vectorPlotLegendMin, frameRange, allRange);
		guidata->_vectorPlotResolvedMin = range.first;
		guidata->_vectorPlotResolvedMax = range.second;

		//先家族删后建（旧矢量图面板同款序列；色值 0-255 由渲染端 /255 归一）
		PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);

		mxr::VectorGraphParams params;
		params.source = mxr::VectorGraphParams::VectorSource::Labels;
		params.labels = guidata->_vectorPlotLabels;
		params.size = guidata->_vectorPlotScale;
		params.arrowType = guidata->_vectorPlotArrowType;
		params.isShowText = guidata->_vectorPlotShowText;
		//值域随参数下发：数值着色（MinMax 色表采样）才接管色条与采样域；自定义
		//通道色为纯色箭头，无值域语义（引擎侧 rangeValid=false 不接管）
		params.rangeValid = (guidata->_vectorPlotArrowType == 1);
		params.valueMin = static_cast<float>(range.first);
		params.valueMax = static_cast<float>(range.second);
		params.isEquivariance = guidata->_vectorPlotEquivariance;
		postRender->addObject(params);

		guidata->_vectorPlotActive = true;
		postRend->updatePaint();
		if (resolvedRange != nullptr)
			*resolvedRange = range;
		return true;
	}

	void removeVectorPlot(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata)
	{
		if (guidata != nullptr)
			guidata->_vectorPlotActive = false;
		if (postRend == nullptr) return;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;
		PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);
		postRend->updatePaint();
	}

	void refreshVectorPlotIfActive(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata)
	{
		if (postRend == nullptr || guidata == nullptr) return;
		if (!guidata->_vectorPlotActive) return;
		//读场失败时 applyVectorPlot 内部提示并保持既有箭头（切到无该变量数据的帧不闪断）
		applyVectorPlot(postRend, guidata);
	}

	namespace detail
	{
		//面板通道索引 → 引擎通道名（0=X,1=Y,2=Z,3=X+Y→XY,4=Y+Z→YZ,5=Z+X→XZ,6=X+Y+Z→XYZ；
		//引擎投影表 REBUILD_PostOneFrameRender::createVectorGraph 认这七个名字）
		const char* const kEngineChannelNames[7] = { "X", "Y", "Z", "XY", "YZ", "XZ", "XYZ" };

		int vectorPlotChannelIndex(const QString& channelName)
		{
			for (int i = 0; i < 7; ++i)
				if (channelName == QLatin1String(kEngineChannelNames[i]))
					return i;
			return -1;
		}

		QPair<double, double> vectorFieldRange(const QHash<int, QVector3D>& field)
		{
			if (field.isEmpty())
				return { 0.0, 0.0 };
			double vmin = qInf(), vmax = -qInf();
			for (auto it = field.cbegin(); it != field.cend(); ++it)
			{
				const double len = it.value().length();
				if (len < vmin) vmin = len;
				if (len > vmax) vmax = len;
			}
			return { vmin, vmax };
		}

		QPair<double, double> resolveVectorLegendRange(const QString& mode, double manualMax, double manualMin,
			const QPair<double, double>& frameRange, const QPair<double, double>& allRange)
		{
			//不可用=max<min（倒挂）或 (0,0)（空读/未算标记；云图 getMaxMinByFrameID 空帧同判）
			auto usable = [](const QPair<double, double>& r)
			{
				return r.second >= r.first && !(r.first == 0.0 && r.second == 0.0);
			};
			if (mode == QString::fromUtf8("手动"))
			{
				const QPair<double, double> manual(manualMin, manualMax);
				if (usable(manual))
					return manual;
				return frameRange;   //手动值不可用（校验前置通常已挡）回退逐帧
			}
			if (mode == QString::fromUtf8("全程") && usable(allRange))
				return allRange;
			return frameRange;      //逐帧/全程不可用回退
		}

		QVector<QPair<QString, QVector3D>> buildVectorPlotLabels(const bool channelChecked[7], const QColor channelColors[7])
		{
			QVector<QPair<QString, QVector3D>> labels;
			for (int i = 0; i < 7; ++i)
			{
				if (!channelChecked[i])
					continue;
				const QColor& c = channelColors[i];
				labels.append({ QString::fromLatin1(kEngineChannelNames[i]),
					QVector3D(c.red(), c.green(), c.blue()) });   //0-255 原值，渲染端 /255 归一
			}
			return labels;
		}

		double vectorPlotScale(int sizeMode, double uniformSize, double scaleBy)
		{
			switch (sizeMode)
			{
			case 1: return uniformSize;	//统一尺寸值
			case 2: return scaleBy;		//缩放倍率
			default: return 1.0;		//自动
			}
		}

		int vectorPlotArrowType(int colorModeIndex)
		{
			return colorModeIndex == 1 ? 0 : 1;	//自定义→Common 通道色；数值→MinMax 色表
		}
	}

	mPostVectorPlotPanel::mPostVectorPlotPanel(QWidget* parent) : mPanelBase(parent)
	{
		_langEn = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1);
		setObjectName("vectorParameters");
		setupUI();

		//语言切换全量刷新
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
			this, SLOT(slot_changeLanguage(int)));

		//Escape 结束拾取武装态并关显示设置弹窗（原型 keydown Escape）
		bindShortcut(QKeySequence(Qt::Key_Escape), [this]()
			{
				endPick();
				closeDisplayPopup();
			});

		refreshCaseList();
		this->setMinimumWidth(319);	//原型侧栏宽 319（与标签/截面剖切面板同族）
	}

	void mPostVectorPlotPanel::setupUI()
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);
		_mainLayout->setSpacing(kGap);
		setStyleSheet(
			"mPostVectorPlotPanel{background-color:#FFFFFF;}"
			"QComboBox QAbstractItemView{background-color:#FFFFFF;color:#303313;"
			"selection-background-color:#E8F3FF;selection-color:#303313;}");

		/******** 操作条（原型 #vectorActions 两列等分；panel.prepend 置顶） ********/
		{
			QWidget* actionsRow = new QWidget(this);
			QHBoxLayout* actionsLayout = new QHBoxLayout(actionsRow);
			actionsLayout->setContentsMargins(kGroupInset, 0, kGroupInset, 0);
			actionsLayout->setSpacing(kGap);
			_btnClear = new QPushButton(QString::fromUtf8("清除渲染"), actionsRow);
			_btnClear->setObjectName("clearVectorRendering");
			_btnClear->setFixedHeight(kControl);
			_btnClear->setStyleSheet(kLineBtnQss);
			_btnClear->setEnabled(false);	//未提交向量参数前禁用（原型 clear.disabled=true）
			_btnApply = new QPushButton(QString::fromUtf8("应用渲染"), actionsRow);
			_btnApply->setObjectName("applyVectorParameters");
			_btnApply->setFixedHeight(kControl);
			_btnApply->setStyleSheet(kPrimaryBtnQss);
			actionsLayout->addWidget(_btnClear, 1);
			actionsLayout->addWidget(_btnApply, 1);
			_mainLayout->addWidget(actionsRow);
		}

		/******** 折叠组：向量变量（原型克隆云图"云图变量"组改造） ********/
		_variableFold = new FoldSection(QString::fromUtf8("向量变量"), this);

		_comboCase = new QComboBox(_variableFold);
		_comboCase->setObjectName("vectorCase");
		_comboCase->setFixedHeight(kControl);
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "工况", "Case");
			_variableFold->addRow(label, _comboCase);
		}

		_comboFrame = new QComboBox(_variableFold);
		_comboFrame->setObjectName("vectorFrame");
		_comboFrame->setFixedHeight(kControl);
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "帧", "Frame");
			_variableFold->addRow(label, _comboFrame);
		}

		//帧导航四钮（原型克隆 frame-controls：首帧/上一帧/下一帧/末帧；均分填满字段列）
		{
			const char* icons[4] = { ":/mMainWidgets/Icon/post/diyizheng.png",
				":/mMainWidgets/Icon/post/shangyizheng.png",
				":/mMainWidgets/Icon/post/xiayizheng.png",
				":/mMainWidgets/Icon/post/zuihouyizheng.png" };
			QPushButton** buttons[4] = { &_btnFirstFrame, &_btnPreviousFrame, &_btnNextFrame, &_btnLastFrame };
			QWidget* frameNavRow = new QWidget(_variableFold);
			QHBoxLayout* frameNavLayout = new QHBoxLayout(frameNavRow);
			frameNavLayout->setContentsMargins(0, 0, 0, 0);
			frameNavLayout->setSpacing(kGap);
			for (int i = 0; i < 4; ++i)
			{
				*buttons[i] = new QPushButton(QString(), frameNavRow);
				(*buttons[i])->setIcon(QIcon(QString::fromUtf8(icons[i])));
				(*buttons[i])->setMinimumSize(QSize(26, 24));
				frameNavLayout->addWidget(*buttons[i], 1);
			}
			_variableFold->addRow(new QWidget(), frameNavRow);
		}

		//变量（原型 vectorResultType：位移/速度/转动速度/加速度/力，默认转动速度）
		_comboVariable = new QComboBox(_variableFold);
		_comboVariable->setObjectName("vectorResultType");
		_comboVariable->setFixedHeight(kControl);
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "变量", "Variable");
			_variableFold->addRow(label, _comboVariable);
		}

		//层（原型 contourLayer 禁用占位："当前未接入分层向量数据"）
		_comboLayer = new QComboBox(_variableFold);
		_comboLayer->setObjectName("vectorLayer");
		_comboLayer->setFixedHeight(kControl);
		_comboLayer->setEnabled(false);
		_comboLayer->setToolTip(QString::fromUtf8("当前未接入分层向量数据"));
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "层", "Layer");
			_variableFold->addRow(label, _comboLayer);
		}

		//使用角点数据（原型 contourCornerData 禁用占位）
		_checkCornerData = new QCheckBox(QString::fromUtf8("使用角点数据"), _variableFold);
		_checkCornerData->setObjectName("vectorCornerData");
		_checkCornerData->setEnabled(false);
		_checkCornerData->setToolTip(QString::fromUtf8("当前未接入角点向量数据"));
		_variableFold->addRow(new QWidget(), _checkCornerData);

		//变形系数（内联输入默认 1 + 齿轮跳转变形控制；原型 contour-deformation-composite）
		_editDeformCoef = new QLineEdit(QStringLiteral("1"), _variableFold);
		_editDeformCoef->setObjectName("vectorDeformationFactor");
		_editDeformCoef->setFixedHeight(kControl);
		_editDeformCoef->setStyleSheet(numberEditQss());
		_btnOpenDeformation = new QToolButton(_variableFold);
		_btnOpenDeformation->setIcon(QIcon(":/mMainWidgets/Icon/function/shezhi.svg"));
		_btnOpenDeformation->setToolTip(QString::fromUtf8("设置模型变形"));
		_btnOpenDeformation->setFixedSize(QSize(kControl, kControl));
		{
			QWidget* deformRow = new QWidget(_variableFold);
			QHBoxLayout* deformLayout = new QHBoxLayout(deformRow);
			deformLayout->setContentsMargins(0, 0, 0, 0);
			deformLayout->setSpacing(kGap);
			deformLayout->addWidget(_editDeformCoef, 1);
			deformLayout->addWidget(_btnOpenDeformation);
			PanelLabel* label = new PanelLabel;
			regLabel(label, "变形系数", "Deformation Factor");
			_variableFold->addRow(label, deformRow);
		}

		_variableFold->refreshLayout();
		_mainLayout->addWidget(_variableFold);

		/******** 折叠组：对象及坐标系（控件与功能对齐云图，2026-11-14：复合行/小钮/
		拾取文案/坐标系回填/尾双列区全同款；拾取真武装+范围弹窗+类型/范围持久化） ********/
		_objectFold = new FoldSection(QString::fromUtf8("对象及坐标系"), this);

		//对象选择 = mPost 拾取栈拾取条（2026-11-18 云图同款替换，mObjectPickComposite 下线）：
		//[类型下拉 | 拾取条] 单行并排。拾取条内建：点击条体激活拾取/再点停止（deactivatePick
		//开关语义）、详情弹窗（分组+逐项删/按类型整删）、清空钮（云图口径启用：空集=默认
		//"所有X"，原"范围模式不支持空集"禁用口径随范围弹窗下线一并废止）；激活/失活的视口
		//过滤器与工具栏白名单由 slot_objectPickActiveChanged 收口。CBBlue 颜色块=云图同款。
		//_btnObjectScope 仍为隐形文案控制器（提供"所有X"本地化默认文案+持久化口径）
		_objectTypeCombo = new QComboBox(this);
		_objectTypeCombo->setObjectName("vectorObjectType");	//ui-shot 文本驱动沿用旧名
		_objectTypeCombo->addItems(QStringList{ QString::fromUtf8("单元"), QString::fromUtf8("部件") });
		_objectTypeCombo->setCurrentIndex(1);	//默认部件（原型 renderObjectType value=part）
		_objectPickController = mPostGeneralPickUp::createPickWidget(CBBlue, this);
		_objectPickController->widget()->setObjectName("vectorObjectPick");	//ui-shot 沿用旧名
		_objectPickController->setSelectSetBtnVisible(false);         //后处理无集合语义
		_objectPickController->setKeepHighlightOnDeactivate(false);   //云图同口径：解除拾取即清视口高亮
		_objectPickController->setAllowedTypes([this]() {
			//激活时按当前类型发工具栏白名单（0=单元/1=部件）
			QSet<MxFunctions::PickDataType> types;
			types.insert(_objectTypeCombo && _objectTypeCombo->currentIndex() == 0
				? MxFunctions::PickDataType::Element : MxFunctions::PickDataType::Part);
			return types;
			});
		_btnObjectScope = new mPostPartScopeField(this);
		_btnObjectScope->hide();
		//类型与范围控制器同步（默认部件；构造期 scopeTextChanged 未连接，显式设默认文案）
		_btnObjectScope->setObjectType(_objectTypeCombo->currentIndex() == 0 ? 0 : 1);
		_objectPickController->setDefultTip(_btnObjectScope->scopeText());
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "对象选择", "Object");
			_objectFold->addRow(label, _objectTypeCombo, _objectPickController->widget());
		}

		//坐标系（全局坐标系+自定义坐标系按 guidata 回填，云图同款）+ 快速新建"＋"
		//（云图同款启用：接 clickPostCoordinateCreateSig 全局链）
		{
			QWidget* coordRow = new QWidget(_objectFold);
			QHBoxLayout* coordLayout = new QHBoxLayout(coordRow);
			coordLayout->setContentsMargins(0, 0, 0, 0);
			coordLayout->setSpacing(0);
			_comboCoordSystem = new QComboBox(coordRow);
			_comboCoordSystem->setObjectName("vectorCoordinateSystem");
			_btnCoordAdd = new QToolButton(coordRow);
			_btnCoordAdd->setText(QString::fromUtf8("＋"));
			_btnCoordAdd->setToolTip(QString::fromUtf8("快速新建坐标系"));
			_btnCoordAdd->setFixedSize(QSize(kControl, kControl));
			coordLayout->addWidget(_comboCoordSystem, 1);
			coordLayout->addWidget(_btnCoordAdd);
			PanelLabel* label = new PanelLabel;
			regLabel(label, "坐标系", "Coordinate System");
			_objectFold->addRow(label, coordRow);
		}

		//组尾双列区（云图 tail row 同款）：使用跟踪坐标（启用写 guidata）| 边中点值（默认勾+禁用）
		_checkTrackingCoord = new QCheckBox(QString::fromUtf8("使用跟踪坐标"));
		_checkTrackingCoord->setObjectName("vectorTrackingCoordinates");
		_checkMidsideValues = new QCheckBox(QString::fromUtf8("边中点值"));
		_checkMidsideValues->setObjectName("vectorMidsideValues");
		_checkMidsideValues->setChecked(true);
		_checkMidsideValues->setEnabled(false);
		_coordTailRow = new QWidget();
		QHBoxLayout* coordTailLayout = new QHBoxLayout(_coordTailRow);
		coordTailLayout->setContentsMargins(0, 0, 0, 0);
		coordTailLayout->setSpacing(kGap);
		coordTailLayout->addWidget(_checkTrackingCoord, 1);
		coordTailLayout->addWidget(_checkMidsideValues, 1);
		_objectFold->addRow(_coordTailRow, static_cast<QWidget*>(nullptr));

		_objectFold->refreshLayout();
		_mainLayout->addWidget(_objectFold);

		/******** 折叠组：绘图设置（原型 vectorPlotSection 动态构建） ********/
		_plotFold = new FoldSection(QString::fromUtf8("绘图设置"), this);

		//分量/合量连体复选（原型 mirror-planes 复选版：等宽+竖线分隔共框）
		{
			QWidget* componentRow = buildCheckStrip(
			{ QString::fromUtf8("X"), QString::fromUtf8("Y"), QString::fromUtf8("Z") }, _componentChecks);
			QWidget* resultantRow = buildCheckStrip(
			{ QString::fromUtf8("X+Y"), QString::fromUtf8("Y+Z"), QString::fromUtf8("Z+X") }, _resultantChecks);
			for (int i = 0; i < 3; ++i)
			{
				_componentChecks[i]->setObjectName(QString("vectorComponent%1").arg(i));
				_resultantChecks[i]->setObjectName(QString("vectorComponent%1").arg(i + 3));
			}
			PanelLabel* labelComponent = new PanelLabel;
			regLabel(labelComponent, "分量", "Components");
			_plotFold->addRow(labelComponent, componentRow);
			PanelLabel* labelResultant = new PanelLabel;
			regLabel(labelResultant, "合量", "Resultants");
			_plotFold->addRow(labelResultant, resultantRow);
		}

		//三列复选行：显示数值（默认不勾）｜标签前缀（默认勾）｜X+Y+Z（默认勾）
		{
			QWidget* flagsRow = new QWidget(_plotFold);
			flagsRow->setObjectName("vectorLabelOptions");
			QHBoxLayout* flagsLayout = new QHBoxLayout(flagsRow);
			flagsLayout->setContentsMargins(0, 0, 0, 0);
			flagsLayout->setSpacing(kGap);
			_checkShowValues = new QCheckBox(QString::fromUtf8("显示数值"), flagsRow);
			_checkShowValues->setObjectName("vectorShowValues");
			_checkLabelPrefix = new QCheckBox(QString::fromUtf8("标签前缀"), flagsRow);
			_checkLabelPrefix->setObjectName("vectorLabelPrefix");
			_checkLabelPrefix->setChecked(true);
			_checkTotal = new QCheckBox(QString::fromUtf8("X+Y+Z"), flagsRow);
			_checkTotal->setObjectName("vectorComponent6");
			_checkTotal->setChecked(true);
			flagsLayout->addWidget(_checkShowValues, 1);
			flagsLayout->addWidget(_checkLabelPrefix, 1);
			flagsLayout->addWidget(_checkTotal, 1);
			flagsRow->setFixedHeight(kControl);
			_plotFold->addRow(new QWidget(), flagsRow);
		}

		//显示设置入口（原型 vectorDisplayOpen"设置 ›"高级选项钮）
		_btnDisplayOpen = new QPushButton(_plotFold);
		_btnDisplayOpen->setObjectName("vectorDisplayOpen");
		_btnDisplayOpen->setFixedHeight(kControl);
		_btnDisplayOpen->setCursor(Qt::PointingHandCursor);
		_btnDisplayOpen->setStyleSheet(
			QString("QPushButton{background:#FFFFFF;border:1px solid %1;border-radius:%2px;color:%3;font-size:12px;}"
				"QPushButton:hover{background:%4;border-color:%5;}")
			.arg(kLineBorder).arg(kRadius).arg(kTextColor, kSelectedBg, kPrimary));
		{
			QHBoxLayout* openLayout = new QHBoxLayout(_btnDisplayOpen);
			openLayout->setContentsMargins(kGroupInset, 0, kGroupInset, 0);
			openLayout->setSpacing(kChoiceGap);
			QLabel* openText = new QLabel(QString::fromUtf8("设置"), _btnDisplayOpen);
			openText->setObjectName("vectorDisplayOpenText");
			openText->setStyleSheet("background:transparent;border:0;");
			openLayout->addWidget(openText, 1);
			QLabel* openArrow = new QLabel(QString::fromUtf8("›"), _btnDisplayOpen);
			openArrow->setObjectName("vectorDisplayOpenArrow");
			openArrow->setStyleSheet("background:transparent;border:0;");
			openLayout->addWidget(openArrow);
		}
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "显示设置", "Display Settings");
			_plotFold->addRow(label, _btnDisplayOpen);
		}

		_plotFold->refreshLayout();
		_mainLayout->addWidget(_plotFold);

		/******** 折叠组：图例（控件与交互对齐云图，2026-11-14：+信息标签行/删倍率行） ********/
		_legendFold = new FoldSection(QString::fromUtf8("图例"), this);

		//模式（所有部件/显示部件，云图同款）
		_comboLegendMode = new QComboBox(_legendFold);
		_comboLegendMode->setObjectName("vectorLegendMode");
		_comboLegendMode->setFixedHeight(kControl);
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "模式", "Mode");
			_legendFold->addRow(label, _comboLegendMode);
		}

		//图例范围：逐帧/全程/手动（云图同款带说明 tooltip，默认逐帧）
		_choiceLegendRange = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("逐帧"), QString::fromUtf8("全程"), QString::fromUtf8("手动") }, _legendFold);
		_choiceLegendRange->setObjectName("vectorLegendRangeControl");
		_choiceLegendRange->setOptionTooltips({
			QString::fromUtf8("当前帧的数值范围"), QString::fromUtf8("当前工况整个分析过程的数值范围"),
			QString::fromUtf8("手动设置最大值和最小值") });
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "图例范围", "Legend Range");
			_legendFold->addRow(label, _choiceLegendRange);
		}

		//最大值/最小值行=图例边界行（库 mLegendBoundRow，云图同款：输入+复位+显隐三件；
		//图标沿用云图口径：复位=reset.png、眼睛=xianshi(On)/quxiao(Off) 双态；
		//显隐钮独立不受编辑启停影响，值/占位仍由 refreshLegendRange 驱动）
		_legendMax = new MxFunctions::mLegendBoundRow(_legendFold);
		_legendMin = new MxFunctions::mLegendBoundRow(_legendFold);
		_legendMax->lineEdit()->setObjectName("vectorLegendMaximum");	//ui-shot 文本驱动沿用旧名
		_legendMin->lineEdit()->setObjectName("vectorLegendMinimum");
		{
			const QIcon resetIcon(QStringLiteral(":/mMainWidgets/Icon/post/reset.png"));
			QIcon eyeIcon;
			eyeIcon.addFile(QStringLiteral(":/mMainWidgets/Icon/post/xianshi.svg"), QSize(), QIcon::Normal, QIcon::On);   //选中态=显示
			eyeIcon.addFile(QStringLiteral(":/mMainWidgets/Icon/post/quxiao.svg"), QSize(), QIcon::Normal, QIcon::Off);  //未选中态=隐藏
			for (MxFunctions::mLegendBoundRow* row : { _legendMax, _legendMin })
			{
				row->setResetIcon(resetIcon);
				row->setEyeIcon(eyeIcon);
			}
		}
		{
			PanelLabel* labelMax = new PanelLabel;
			regLabel(labelMax, "最大值", "Maximum");
			_legendFold->addRow(labelMax, _legendMax);
			PanelLabel* labelMin = new PanelLabel;
			regLabel(labelMin, "最小值", "Minimum");
			_legendFold->addRow(labelMin, _legendMin);
		}

		//插值阶数（云图同款 QSpinBox 2..30；默认 9）
		_spinInterpOrder = new QSpinBox(_legendFold);
		_spinInterpOrder->setObjectName("vectorInterpolationOrder");
		_spinInterpOrder->setRange(2, 30);
		_spinInterpOrder->setValue(9);
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "插值阶数", "Interpolation Order");
			_legendFold->addRow(label, _spinInterpOrder);
		}

		//平滑方式（平滑/离散，默认平滑）
		_choiceSmoothMode = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("平滑"), QString::fromUtf8("离散") }, _legendFold);
		_choiceSmoothMode->setObjectName("vectorInterpolationMode");
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "平滑方式", "Smoothing");
			_legendFold->addRow(label, _choiceSmoothMode);
		}

		//分布方式（等差/等比，默认等差）
		_choiceSpacing = new MxFunctions::mChoiceGroup(
		{ QString::fromUtf8("等差"), QString::fromUtf8("等比") }, _legendFold);
		_choiceSpacing->setObjectName("vectorInterpolationSpacing");
		{
			PanelLabel* label = new PanelLabel;
			regLabel(label, "分布方式", "Distribution");
			_legendFold->addRow(label, _choiceSpacing);
		}

		//渲染图例三钮（隐藏↔显示/复位/自定义；云图同款：等分+随窄栏压缩，零间距）
		{
			QWidget* legendActionsRow = new QWidget(_legendFold);
			QHBoxLayout* legendActionsLayout = new QHBoxLayout(legendActionsRow);
			legendActionsLayout->setContentsMargins(0, 0, 0, 0);
			legendActionsLayout->setSpacing(0);
			_btnLegendHide = new QPushButton(QString::fromUtf8("隐藏"), legendActionsRow);
			_btnLegendHide->setObjectName("vectorLegendVisibility");
			_btnLegendReset = new QPushButton(QString::fromUtf8("复位"), legendActionsRow);
			_btnLegendReset->setObjectName("vectorLegendReset");
			_btnLegendCustom = new QPushButton(QString::fromUtf8("自定义"), legendActionsRow);
			_btnLegendCustom->setObjectName("vectorLegendCustom");
			//三按钮等分且允许随窄栏压缩（云图同款）：三字文案+边距硬下限 52 防文字裁切
			for (QPushButton* btn : { _btnLegendHide, _btnLegendReset, _btnLegendCustom })
			{
				btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
				btn->setMinimumWidth(52);
				legendActionsLayout->addWidget(btn, 1);
			}
			PanelLabel* label = new PanelLabel;
			regLabel(label, "渲染图例", "Rendered Legend");
			_legendFold->addRow(label, legendActionsRow);
		}

		//信息标签三钮（云图同款构造：等分+随窄栏压缩，零间距；隐藏↔显示/复位/自定义=
		//开注释面板弹窗；倍率行已删——2026-11-14 用户定案暂不需要该功能）
		{
			QWidget* labelActionsRow = new QWidget(_legendFold);
			QHBoxLayout* labelActionsLayout = new QHBoxLayout(labelActionsRow);
			labelActionsLayout->setContentsMargins(0, 0, 0, 0);
			labelActionsLayout->setSpacing(0);
			_btnLabelHide = new QPushButton(QString::fromUtf8("隐藏"), labelActionsRow);
			_btnLabelHide->setObjectName("vectorLabelVisibility");
			_btnLabelReset = new QPushButton(QString::fromUtf8("复位"), labelActionsRow);
			_btnLabelReset->setObjectName("vectorLabelReset");
			_btnLabelCustom = new QPushButton(QString::fromUtf8("自定义"), labelActionsRow);
			_btnLabelCustom->setObjectName("vectorLabelCustom");
			for (QPushButton* btn : { _btnLabelHide, _btnLabelReset, _btnLabelCustom })
			{
				btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
				btn->setMinimumWidth(52);
				labelActionsLayout->addWidget(btn, 1);
			}
			PanelLabel* label = new PanelLabel;
			regLabel(label, "信息标签", "Info Labels");
			_legendFold->addRow(label, labelActionsRow);
		}

		_legendFold->refreshLayout();
		_mainLayout->addWidget(_legendFold);

		//原型"图例自定义"子组（vectorLegendCustomSection）已删：渲染图例三钮对齐云图后，
		//自定义=打开渲染侧图例编辑器（slot_legendCustomClicked），子组无对应功能（2026-11-14）

		/******** 状态行（空文案整行收起；原型 #vectorStatus） ********/
		_statusLabel = new QLabel(this);
		_statusLabel->setObjectName("vectorStatus");
		_statusLabel->setContentsMargins(10, 0, 10, kGap);
		_statusLabel->setStyleSheet(
			QString("QLabel{color:%1;font-size:11px;border:0;background:transparent;}").arg(kMuted));
		_statusLabel->setVisible(false);
		_mainLayout->addWidget(_statusLabel);

		_mainLayout->addStretch();

		//分量及合量颜色默认（原型 colors 常量，索引 0..6 = X/Y/Z/X+Y/Y+Z/Z+X/X+Y+Z）
		const QColor defaults[7] = { QColor(0xc7, 0x35, 0x35), QColor(0x25, 0x83, 0x48),
			QColor(0x28, 0x5c, 0xbe), QColor(0xa6, 0x3a, 0xb8), QColor(0x98, 0x75, 0x1b),
			QColor(0x16, 0x8a, 0x99), QColor(0x9b, 0x60, 0x25) };
		for (int i = 0; i < 7; ++i)
			_componentColors[i] = defaults[i];

		//显示设置弹窗（构造期建壳，全部弹窗内控件随壳就位；语言/接线访问安全）
		ensureDisplayPopup();

		//选项与默认选择 → 语言文案
		initOptions();
		applyLanguageTexts();
		_objectPickController->setDefultTip(_btnObjectScope ? _btnObjectScope->scopeText()
			: QString::fromUtf8("所有部件"));	//空态默认文案随当前语言范围标签（原 refreshPickText 职责）
		refreshLegendRange();
		refreshDisplayState();
		refreshFrameButtons();

		/******** 级联与交互接线 ********/
		connect(_btnApply, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_applyClicked);
		connect(_btnClear, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_clearClicked);
		connect(_comboCase, &QComboBox::currentTextChanged, this, &mPostVectorPlotPanel::slot_chooseCase);
		connect(_btnFirstFrame, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_firstFrame);
		connect(_btnPreviousFrame, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_previousFrame);
		connect(_btnNextFrame, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_nextFrame);
		connect(_btnLastFrame, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_lastFrame);
		connect(_objectTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mPostVectorPlotPanel::slot_objectTypeChanged);
		//拾取激活/失活收口（云图同款）：视口拾取过滤器按对象类型直设（勿用 activePickSig
		//——该信号只同步前处理工具栏按钮组，对后处理视口不生效）；失活恢复工具栏全部拾取类型
		connect(_objectPickController, &mPostPickWidgetController::pickActiveChanged,
			this, &mPostVectorPlotPanel::slot_objectPickActiveChanged);
		//范围文本变化（回填/类型切换/语言）→ 拾取条空态默认文案（云图同款）
		connect(_btnObjectScope, &mPostPartScopeField::scopeTextChanged, this, [this](const QString& text)
			{
				if (_objectPickController != nullptr)
					_objectPickController->setDefultTip(text);
			});
		//快速新建坐标系：云图同款全局信号（树侧槽现阶段提示"功能开发中"，链路就绪自动升级）
		connect(_btnCoordAdd, &QToolButton::clicked, this, []()
			{
				MechCommon::mMechGlobalSignals::getInstance()->clickPostCoordinateCreateSig(0);   //0=直角
			});
		//使用跟踪坐标：写当前视口 guidata（云图同款）
		connect(_checkTrackingCoord, &QCheckBox::toggled, this, [this](bool checked)
			{
				mxr::REBUILD_PostRend* postRend = nullptr;
				MechPreData::mPostGUIData* guidata = nullptr;
				if (!currentCloudContext(&postRend, &guidata))
					return;
				guidata->_useTrackingCoord = checked;
			});
		connect(_btnOpenDeformation, &QToolButton::clicked, this, &mPostVectorPlotPanel::slot_openDeformation);
		connect(_btnDisplayOpen, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_displayOpenClicked);
		connect(_choiceSizeMode, &MxFunctions::mChoiceGroup::currentIndexChanged,
			this, &mPostVectorPlotPanel::slot_sizeModeChanged);
		connect(_choiceColorMode, &MxFunctions::mChoiceGroup::currentIndexChanged,
			this, [this](int) { refreshDisplayState(); });
		connect(_checkShowValues, &QCheckBox::toggled, this, [this](bool) { refreshDisplayState(); });
		connect(_choiceLegendRange, &MxFunctions::mChoiceGroup::currentIndexChanged,
			this, &mPostVectorPlotPanel::slot_legendRangeChanged);
		connect(_legendMax->lineEdit(), &QLineEdit::textChanged, this, [this](const QString& text) { _manualMax = text; });
		connect(_legendMin->lineEdit(), &QLineEdit::textChanged, this, [this](const QString& text) { _manualMin = text; });
		connect(_legendMax, &MxFunctions::mLegendBoundRow::resetClicked, this, &mPostVectorPlotPanel::slot_legendMaxReset);
		connect(_legendMin, &MxFunctions::mLegendBoundRow::resetClicked, this, &mPostVectorPlotPanel::slot_legendMinReset);
		//边界行眼睛显隐：真作用视口色表最大/最小值线（云图 slot_setPositionVisible 同款）
		connect(_legendMax, &MxFunctions::mLegendBoundRow::visibilityToggled, this, &mPostVectorPlotPanel::slot_legendBoundVisibilityToggled);
		connect(_legendMin, &MxFunctions::mLegendBoundRow::visibilityToggled, this, &mPostVectorPlotPanel::slot_legendBoundVisibilityToggled);
		connect(_btnLegendHide, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_legendVisibilityClicked);
		connect(_btnLegendReset, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_legendResetClicked);
		connect(_btnLegendCustom, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_legendCustomClicked);
		connect(_btnLabelHide, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_labelHide);
		connect(_btnLabelReset, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_labelReset);
		connect(_btnLabelCustom, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_labelCustom);
		connect(_btnFontColor, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_fontColorClicked);
		for (QPushButton* swatch : _componentSwatches)
		{
			if (swatch)
				connect(swatch, &QPushButton::clicked, this, &mPostVectorPlotPanel::slot_componentColorClicked);
		}

		//任意变更清状态行（原型 panel/popup change+input → status 清空）
		auto clearOnChange = [this](QWidget* widget)
		{
			if (auto* combo = qobject_cast<QComboBox*>(widget))
				connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { clearStatus(); });
			else if (auto* edit = qobject_cast<QLineEdit*>(widget))
				connect(edit, &QLineEdit::textChanged, this, [this](const QString&) { clearStatus(); });
			else if (auto* check = qobject_cast<QCheckBox*>(widget))
				connect(check, &QCheckBox::toggled, this, [this](bool) { clearStatus(); });
			else if (auto* group = qobject_cast<MxFunctions::mChoiceGroup*>(widget))
				connect(group, &MxFunctions::mChoiceGroup::currentIndexChanged, this, [this](int) { clearStatus(); });
			else if (auto* spin = qobject_cast<QSpinBox*>(widget))
				connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { clearStatus(); });
		};
		for (QWidget* widget : std::initializer_list<QWidget*>{ _comboCase, _comboFrame, _comboVariable,
			_objectTypeCombo, _comboCoordSystem, _editDeformCoef, _checkShowValues, _checkLabelPrefix,
			_checkTotal, _choiceLegendRange, _legendMax->lineEdit(), _legendMin->lineEdit(), _spinInterpOrder,
			_choiceSmoothMode, _choiceSpacing, _comboFontSize, _comboFontFamily,
			_choiceColorMode, _choiceSizeMode, _editSizeValue, _editScaleBy, _choiceSymbolPos,
			_comboHeadForm, _comboLegendMode })
		{
			clearOnChange(widget);
		}
		for (QCheckBox* check : _componentChecks)
			clearOnChange(check);
		for (QCheckBox* check : _resultantChecks)
			clearOnChange(check);

		//折叠组收起退出拾取武装态（面板互斥模式；对象组收起时拾取钮不可见）
		for (FoldSection* fold : { _variableFold, _objectFold, _plotFold, _legendFold })
		{
			connect(fold, &FoldSection::foldExpandChanged, this, [this](bool expand)
				{
					if (!expand)
						endPick();
				});
		}
	}

	QWidget* mPostVectorPlotPanel::buildCheckStrip(const QStringList& names, QList<QCheckBox*>& checks)
	{
		//连体复选组（原型 mirror-planes 复选版）：1px 圆角外框，项等宽、竖线分隔、内高 24
		QFrame* strip = new QFrame(this);
		strip->setObjectName("vectorCheckStrip");
		strip->setFixedHeight(kControl);
		strip->setStyleSheet(QString(
			"QFrame#vectorCheckStrip{background:#FFFFFF;border:1px solid %1;border-radius:%2px;}")
			.arg(kLineBorder).arg(kRadius));
		QHBoxLayout* lay = new QHBoxLayout(strip);
		lay->setContentsMargins(0, 1, 0, 1);
		lay->setSpacing(0);
		checks.clear();
		for (int i = 0; i < names.count(); ++i)
		{
			QCheckBox* check = new QCheckBox(names.at(i), strip);
			check->setFixedHeight(kInner);
			check->setStyleSheet(QString(
				"QCheckBox{background:transparent;color:%1;font-size:12px;padding:0 %2px;spacing:%3px;border:0;}"
				"QCheckBox::indicator{width:13px;height:13px;}%4")
				.arg(kTextColor).arg(kChoiceGap).arg(kChoiceGap)
				.arg(i > 0 ? QString("QCheckBox{border-left:1px solid %1;}").arg(kLineBorder) : QString()));
			lay->addWidget(check, 1);
			checks.append(check);
		}
		return strip;
	}

	QPushButton* mPostVectorPlotPanel::buildSwatch(const QColor& color)
	{
		//26×26 色块钮（原型 input.label-color 26；底色=当前色）
		QPushButton* swatch = new QPushButton(this);
		swatch->setFixedSize(QSize(kControl, kControl));
		swatch->setCursor(Qt::PointingHandCursor);
		applySwatchColor(swatch, color);
		return swatch;
	}

	void mPostVectorPlotPanel::applySwatchColor(QPushButton* swatch, const QColor& color)
	{
		swatch->setStyleSheet(QString(
			"QPushButton{background:%1;border:1px solid %2;border-radius:%3px;}"
			"QPushButton:disabled{border:1px solid %2;}")
			.arg(color.name(), QString(kLineBorder), QString::number(kRadius)));
	}

	void mPostVectorPlotPanel::popupUnder(QWidget* popup, QWidget* anchor)
	{
		//弹窗贴锚点控件下方显示（越界收回屏内；同标签/查询面板口径）
		popup->adjustSize();
		QPoint pos = anchor->mapToGlobal(QPoint(0, anchor->height() + 6));
		if (QScreen* screen = QGuiApplication::screenAt(anchor->mapToGlobal(QPoint(0, 0))))
		{
			const QRect avail = screen->availableGeometry();
			if (pos.x() + popup->width() > avail.right())
				pos.setX(avail.right() - popup->width());
			if (pos.y() + popup->height() > avail.bottom())
				pos.setY(anchor->mapToGlobal(QPoint(0, 0)).y() - popup->height() - 6);
		}
		popup->move(pos);
		popup->show();
		popup->raise();
	}

	PostData::mPostDataBase* mPostVectorPlotPanel::currentPostDB() const
	{
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
		if (rebuildPostRend == nullptr || rebuildPostRend->getPostRender() == nullptr)
			return nullptr;
		const QString postname = rebuildPostRend->getPostDataName();
		if (postname.isEmpty())
			return nullptr;
		return getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
	}

	void mPostVectorPlotPanel::updateVectorPlotPanel()
	{
		_postDB = currentPostDB();

		//面板工况/帧跟随视口当前态（guidata 权威；解析失败回退保持面板选择）
		QString preferCase;
		QString preferFramePrefix;
		mxr::REBUILD_PostRend* rend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (currentCloudContext(&rend, &guidata))
		{
			if (!guidata->_cloudCase.isEmpty())
				preferCase = guidata->_cloudCase;
			if (guidata->_cloudFrame > 0)
				preferFramePrefix = QString::number(guidata->_cloudFrame) + QStringLiteral(":");
		}
		refreshCaseList(preferCase, preferFramePrefix);
		restoreFromGUIData();
	}

	void mPostVectorPlotPanel::refreshCaseList(const QString& preferCase /*= QString()*/, const QString& preferFramePrefix /*= QString()*/)
	{
		//工况（优先 preferCase=视口当前工况；否则保持面板选择；无结果库清空）
		QString keepCase = preferCase.isEmpty() ? _comboCase->currentText() : preferCase;
		QStringList cases;
		if (_postDB)
			cases = _postDB->getAllCaseNames();
		_comboCase->blockSignals(true);
		_comboCase->clear();
		_comboCase->addItems(cases);
		if (cases.contains(keepCase))
			_comboCase->setCurrentText(keepCase);
		else if (!cases.isEmpty())
			_comboCase->setCurrentIndex(0);
		_comboCase->blockSignals(false);
		//blockSignals 下 setCurrentIndex 不发射 currentTextChanged，手动级联
		//（slot_chooseCase 为单参槽不可带 prefer 前缀，这里直调等价两步）
		clearStatus();
		refreshFrameList(_comboCase->currentText(), preferFramePrefix);
	}

	void mPostVectorPlotPanel::refreshFrameList(const QString& caseName, const QString& preferFramePrefix /*= QString()*/)
	{
		//帧（优先 preferFramePrefix=视口当前帧前缀"帧号:"；否则保持面板选择；云图同口径按帧号排序）
		QStringList frames;
		if (_postDB && !caseName.isEmpty())
			frames = sortFrameNames(_postDB->getCaseFrameNames(caseName));
		QString keepFrame = _comboFrame->currentText();
		if (!preferFramePrefix.isEmpty())
		{
			//视口当前帧优先（帧表显示串"帧号:Time = …"按前缀匹配）
			for (const QString& f : frames)
			{
				if (f.startsWith(preferFramePrefix))
				{
					keepFrame = f;
					break;
				}
			}
		}
		_comboFrame->blockSignals(true);
		_comboFrame->clear();
		_comboFrame->addItems(frames);
		if (frames.contains(keepFrame))
			_comboFrame->setCurrentText(keepFrame);
		else if (!frames.isEmpty())
			_comboFrame->setCurrentIndex(0);
		_comboFrame->blockSignals(false);
		refreshFrameButtons();
		//变量列表随帧刷新（数据驱动：当前帧节点矢量变量）
		refreshVariableList(_comboFrame->currentText().section(':', 0, 0).toInt());
	}

	void mPostVectorPlotPanel::refreshVariableList(int frameId)
	{
		//变量=当前帧节点矢量变量（filterDeformVariables 口径，同变形面板：前三分量
		//X/Y/Z 或 T1/T2/T3 的节点变量；张量/标量/伪变量剔除），默认首项（位移）
		QStringList vars;
		if (_postDB != nullptr && frameId > 0)
		{
			const QStringList frameVars = _postDB->getVariableByFrameID(frameId);
			QHash<QString, QStringList> composMap;
			QHash<QString, int> varTypes;
			for (const QString& name : frameVars)
			{
				composMap.insert(name, _postDB->getVariableCompos(name));
				varTypes.insert(name, _postDB->getVariableType(name));
			}
			vars = detail::filterDeformVariables(frameVars, composMap, varTypes);
		}
		const QString keepVar = _comboVariable->currentText();
		_comboVariable->blockSignals(true);
		_comboVariable->clear();
		_comboVariable->addItems(vars);
		if (vars.contains(keepVar))
			_comboVariable->setCurrentText(keepVar);
		else if (!vars.isEmpty())
			_comboVariable->setCurrentIndex(0);
		_comboVariable->blockSignals(false);
	}

	bool mPostVectorPlotPanel::currentCloudContext(mxr::REBUILD_PostRend** postRend, MechPreData::mPostGUIData** guidata)
	{
		if (postRend == nullptr || guidata == nullptr) return false;
		*postRend = nullptr;
		*guidata = nullptr;
		mxr::REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend == nullptr || rend->getPostRender() == nullptr)
			return false;
		MechPreData::mPostGUIData* data = getCVM->getGUIDataByPostRender(rend);
		if (data == nullptr)
			return false;
		*postRend = rend;
		*guidata = data;
		return true;
	}

	void mPostVectorPlotPanel::syncViewToPanelSelection(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata)
	{
		//工况不一致：云图面板 selectCaseByName 标准链（重灌工况下拉+级联首帧；
		//viewname 经 rend→子窗口→视口私有三步反查）
		const QString panelCase = _comboCase->currentText();
		if (!panelCase.isEmpty() && panelCase != guidata->_cloudCase)
		{
			QWidget* sub = getCVM->getSubWindowFromRend(postRend);
			MechPreData::mViewportPrivate* priv = sub ? getCVM->getViewportPrivate(sub) : nullptr;
			if (priv != nullptr)
				mPostInfoHub::getInstance()->cloudResultPanel()->selectCaseByName(priv->getViewportName(), panelCase);
		}

		//帧不一致：全局 changedFrame 信号驱动云图标准链（高级查询同款；显示串"帧号:…"取前缀）
		const int panelFrame = _comboFrame->currentText().section(':', 0, 0).toInt();
		if (panelFrame > 0 && panelFrame != guidata->_cloudFrame)
			MechCommon::mMechGlobalSignals::getInstance()->changedFrame(panelFrame);
	}

	void mPostVectorPlotPanel::restoreFromGUIData()
	{
		mxr::REBUILD_PostRend* rend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&rend, &guidata))
			return;

		//变量回填（列表已刷新；不在列表内不回填）
		if (!guidata->_vectorPlotVarName.isEmpty())
		{
			const int idx = _comboVariable->findText(guidata->_vectorPlotVarName);
			if (idx >= 0)
			{
				_comboVariable->blockSignals(true);
				_comboVariable->setCurrentIndex(idx);
				_comboVariable->blockSignals(false);
			}
		}

		//B5 图例回填：档位（空/非法回退逐帧；blockSignals 防 slot 写回 guidata）
		//+手动缓存+解析值域（随 _requested 态在 refreshLegendRange 内决定边界行
		//显示实际范围还是占位）
		{
			const QString range = guidata->_vectorPlotLegendRange.isEmpty()
				? QString::fromUtf8("逐帧") : guidata->_vectorPlotLegendRange;
			_choiceLegendRange->blockSignals(true);
			_choiceLegendRange->setCurrentText(range);
			_choiceLegendRange->blockSignals(false);
			_manualMax = MxFunctions::mNumberToString(guidata->_vectorPlotLegendMax);
			_manualMin = MxFunctions::mNumberToString(guidata->_vectorPlotLegendMin);
			_resolvedRange = qMakePair(guidata->_vectorPlotResolvedMin, guidata->_vectorPlotResolvedMax);
		}

		//对象及坐标系回填（云图 update3DResultPanel 同口径）：坐标系=全局+guidata 自定义名，
		//当前=渲染记录；使用跟踪坐标/边中点值复选同步（blockSignals 防回写）
		_comboCoordSystem->blockSignals(true);
		_comboCoordSystem->clear();
		_comboCoordSystem->addItem(QString::fromUtf8("全局坐标系"));
		for (const QString& name : guidata->_cloudCoordinateNames)
		{
			if (name != QString::fromUtf8("全局坐标系"))
				_comboCoordSystem->addItem(name);
		}
		if (!guidata->_renderCoordSystem.isEmpty())
			_comboCoordSystem->setCurrentText(guidata->_renderCoordSystem);
		_comboCoordSystem->blockSignals(false);
		_checkTrackingCoord->blockSignals(true);
		_checkTrackingCoord->setChecked(guidata->_useTrackingCoord);
		_checkTrackingCoord->blockSignals(false);
		_checkMidsideValues->blockSignals(true);
		_checkMidsideValues->setChecked(guidata->_midsideValues);
		_checkMidsideValues->blockSignals(false);

		//对象选择回填（云图 2026-11-18 口径同款）：类型下拉 blockSignals 防触发类型切换
		//联动；范围恒回默认"所有X"（不再 setScopeByText 回显旧范围文本）；拾取数据为
		//进程态，回填/切 case 不带旧上下文。guidata 存中文基准文本（云图契约），
		//EN 会话遗留英文值按索引兼容
		const int typeIndex = (guidata->_renderObjectType == QString::fromUtf8("单元")
			|| guidata->_renderObjectType == QLatin1String("Element")) ? 0 : 1;
		_objectTypeCombo->blockSignals(true);
		_objectTypeCombo->setCurrentIndex(typeIndex);
		_objectTypeCombo->blockSignals(false);
		if (_btnObjectScope != nullptr)
		{
			_btnObjectScope->setObjectType(typeIndex);
			_btnObjectScope->setScope(1);
		}
		if (_objectPickController != nullptr)
		{
			_objectPickController->displayData(PickWidgetData{});
			_objectPickController->setDefultTip(_btnObjectScope ? _btnObjectScope->scopeText()
				: QString::fromUtf8("所有部件"));
		}

		//信息标签按钮状态跟随当前视口（云图同款：隐藏态按钮变"显示"并禁用其余）
		setInfoLabelButtonsState(guidata->_cloudInfoLabelVisible);

		//渲染图例按钮态+边界行眼睛随当前视口（云图口径：隐藏态复位/自定义禁用、
		//眼睛=guidata._cloudShowMax/MinLine）
		_legendHidden = !guidata->_isColorTableShow;
		_btnLegendHide->setText(_legendHidden
			? (_langEn ? QString("Show") : QString::fromUtf8("显示"))
			: (_langEn ? QString("Hide") : QString::fromUtf8("隐藏")));
		_btnLegendHide->setToolTip(_legendHidden
			? (_langEn ? QString("Show rendered legend") : QString::fromUtf8("显示渲染图例"))
			: (_langEn ? QString("Hide rendered legend") : QString::fromUtf8("隐藏渲染图例")));
		_btnLegendReset->setDisabled(!guidata->_isColorTableShow);
		_btnLegendCustom->setDisabled(!guidata->_isColorTableShow);
		_legendMax->setValueVisible(guidata->_cloudShowMaxLine);
		_legendMin->setValueVisible(guidata->_cloudShowMinLine);

		//已应用态回填（渲染态存 guidata 随视口存续；通道/颜色细节不回填——
		//重应用以当前 UI 为准，见头文件注释）
		_requested = guidata->_vectorPlotActive;
		_btnClear->setEnabled(_requested);
		refreshLegendRange();   //边界行随已应用态回显解析值域/占位
	}

	void mPostVectorPlotPanel::refreshFrameButtons()
	{
		//单帧/无帧禁用四钮（原型克隆态 title"当前仅有一帧"）
		const bool enabled = _comboFrame->count() > 1;
		const QString tip = enabled ? QString() : (_langEn ? QString("Only one frame") : QString::fromUtf8("当前仅有一帧"));
		for (QPushButton* btn : { _btnFirstFrame, _btnPreviousFrame, _btnNextFrame, _btnLastFrame })
		{
			btn->setEnabled(enabled);
			btn->setToolTip(tip);
		}
	}

	void mPostVectorPlotPanel::slot_chooseCase(const QString& text)
	{
		clearStatus();
		refreshFrameList(text);
	}

	void mPostVectorPlotPanel::slot_firstFrame()
	{
		if (_comboFrame->count() > 0)
			_comboFrame->setCurrentIndex(0);
		refreshVariableList(_comboFrame->currentText().section(':', 0, 0).toInt());
	}

	void mPostVectorPlotPanel::slot_previousFrame()
	{
		if (_comboFrame->currentIndex() > 0)
			_comboFrame->setCurrentIndex(_comboFrame->currentIndex() - 1);
		refreshVariableList(_comboFrame->currentText().section(':', 0, 0).toInt());
	}

	void mPostVectorPlotPanel::slot_nextFrame()
	{
		if (_comboFrame->currentIndex() + 1 < _comboFrame->count())
			_comboFrame->setCurrentIndex(_comboFrame->currentIndex() + 1);
		refreshVariableList(_comboFrame->currentText().section(':', 0, 0).toInt());
	}

	void mPostVectorPlotPanel::slot_lastFrame()
	{
		if (_comboFrame->count() > 0)
			_comboFrame->setCurrentIndex(_comboFrame->count() - 1);
		refreshVariableList(_comboFrame->currentText().section(':', 0, 0).toInt());
	}

	void mPostVectorPlotPanel::initOptions()
	{
		//变量：位移/速度/转动速度/加速度/力（默认转动速度=索引2，原型 rotation-velocity）
		_comboVariable->addItems(QStringList{ QString::fromUtf8("位移"), QString::fromUtf8("速度"),
			QString::fromUtf8("转动速度"), QString::fromUtf8("加速度"), QString::fromUtf8("力") });
		_comboVariable->setCurrentIndex(2);

		//对象类型下拉项已在构造期随控件创建（单元/部件默认部件），此处不再装填

		//坐标系：全局坐标系（自定义坐标系随 guidata 回填，云图同款；默认全局）
		_comboCoordSystem->addItem(QString::fromUtf8("全局坐标系"));

		//图例模式（云图同款两项）
		_comboLegendMode->addItems(QStringList{ QString::fromUtf8("所有部件"), QString::fromUtf8("显示部件") });

		//字号（数字项；默认 12）/字体（默认微软雅黑）
		_comboFontSize->addItems(QStringList{ "8", "9", "10", "11", "12", "14", "16", "18",
			"20", "24", "28", "32", "36", "48", "72" });
		_comboFontSize->setCurrentText("12");
		_comboFontFamily->addItems(QStringList{ QString::fromUtf8("微软雅黑"), QString::fromUtf8("宋体"), QString("Arial") });
		_comboFontFamily->setCurrentIndex(0);

		//箭头样式：箭头/线条（默认箭头）
		_comboHeadForm->addItems(QStringList{ QString::fromUtf8("箭头"), QString::fromUtf8("线条") });
		_comboHeadForm->setCurrentIndex(0);

		//符号形式默认尾部（原型 position.value='tail'）
		_choiceSymbolPos->setCurrentIndex(2);
	}

	void mPostVectorPlotPanel::endPick()
	{
		//退出拾取（Esc/切类型/开弹窗/收折叠组/应用渲染共用）：失活拾取栈控件即可，
		//视口过滤器/高亮清空/工具栏恢复由 slot_objectPickActiveChanged(false) 链统一收口
		if (_objectPickController != nullptr && _objectPickController->isPickActive())
			_objectPickController->setPickActive(false);
	}

	void mPostVectorPlotPanel::slot_objectTypeChanged(int index)
	{
		//云图 slot_chooseObjectType 同口径：范围控制器随类型联动（单元=3 项/部件=5 项，
		//切换后范围重置"所有X"，原型 dataset.renderObjectScope='all'）。
		//索引判定（0=单元 1=部件），持久化写中文基准文本（guidata 契约，云图回填口径），
		//面板语言不影响
		if (_btnObjectScope != nullptr)
		{
			_btnObjectScope->setObjectType(index == 0 ? 0 : 1);
			_btnObjectScope->setScope(1);
		}

		//类型切换语义重置（云图同款）：清空拾取数据并解除拾取——旧类型数据不带入新类型
		//防空集混计；拾取过滤器按激活时类型设定，切换后旧过滤器失效，统一解除由用户重新拾取
		if (_objectPickController != nullptr)
		{
			if (_objectPickController->isPickActive())
				_objectPickController->setPickActive(false);
			_objectPickController->displayData(PickWidgetData{});
			_objectPickController->setDefultTip(_btnObjectScope ? _btnObjectScope->scopeText()
				: QString::fromUtf8("所有部件"));
		}
		clearStatus();

		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&postRend, &guidata))
			return;
		guidata->_renderObjectType = (index == 0) ? QString::fromUtf8("单元") : QString::fromUtf8("部件");
		guidata->_renderObjectScope = _btnObjectScope ? mPostPartScopeField::scopeText(
			_btnObjectScope->objectType(), _btnObjectScope->scope(), 0)
			: QString::fromUtf8("所有部件");
	}

	void mPostVectorPlotPanel::slot_objectPickActiveChanged(bool active)
	{
		//云图 slot_objectPickActiveChanged 同款收口
		if (_objectPickController == nullptr) return;
		mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (active)
		{
			//激活前置校验（原 slot_pickToggled 口径）：后处理上下文无效立即失活回退
			if (postRend == nullptr || postRend->getPostRender() == nullptr)
			{
				_objectPickController->setPickActive(false);
				return;
			}
			//按对象类型设过滤器（单元=单元拾取/部件=部件拾取）；收割（ReadyToReadContainer
			//连接）、高亮与重入回显（激活时历史数据写回选择容器）由拾取栈控制器自管
			postRend->setPickFilter(_objectTypeCombo->currentIndex() == 0
				? mxr::PickFilter::PickMeshElement : mxr::PickFilter::PickMeshPart);
		}
		else
		{
			//失活：清过滤器与拾取高亮（控制器已清共享选中容器），恢复工具栏全部拾取类型
			if (postRend != nullptr)
				postRend->setPickFilter(mxr::PickFilter::PickNothing);
			emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
		}
		clearStatus();
	}

	void mPostVectorPlotPanel::slot_openDeformation()
	{
		//齿轮跳转变形控制面板（原型 openContourDeformationSettings → 变形设置）
		emit MechCommon::mMechGlobalSignals::getInstance()->clickDeformationSig();
	}

	void mPostVectorPlotPanel::ensureDisplayPopup()
	{
		if (_displayPopup)
			return;

		//弹窗壳（原型 #vectorDisplayPopup fbd-popup：头 28"显示设置"+关闭钮）
		_displayPopup = new QFrame(this, Qt::Popup);
		_displayPopup->setObjectName("vectorDisplayPopup");
		_displayPopup->setFrameShape(QFrame::NoFrame);
		_displayPopup->setStyleSheet(QString(
			"QFrame#vectorDisplayPopup{background:#FFFFFF;border:1px solid %1;}").arg(kLineBorder));
		QVBoxLayout* popupLayout = new QVBoxLayout(_displayPopup);
		popupLayout->setContentsMargins(0, 0, 0, 0);
		popupLayout->setSpacing(0);

		QFrame* header = new QFrame(_displayPopup);
		header->setFixedHeight(kHeader);
		header->setStyleSheet("QFrame{background:#FFFFFF;border:0;border-bottom:1px solid #DCE2E9;}");
		QHBoxLayout* headerLayout = new QHBoxLayout(header);
		headerLayout->setContentsMargins(kGroupInset, 0, 0, 0);
		headerLayout->setSpacing(0);
		QLabel* title = new QLabel(QString::fromUtf8("显示设置"), header);
		title->setObjectName("vectorDisplayTitle");
		title->setStyleSheet("background:transparent;border:0;color:#303313;font-size:12px;");
		headerLayout->addWidget(title);
		headerLayout->addStretch();
		QToolButton* close = new QToolButton(header);
		close->setText(QString::fromUtf8("×"));
		close->setFixedSize(24, 24);
		close->setCursor(Qt::PointingHandCursor);
		close->setStyleSheet("QToolButton{background:transparent;border:0;font-size:14px;}"
			"QToolButton:hover{background:#F0F7FF;}");
		headerLayout->addWidget(close);
		popupLayout->addWidget(header);

		QWidget* body = new QWidget(_displayPopup);
		QVBoxLayout* bodyLayout = new QVBoxLayout(body);
		bodyLayout->setContentsMargins(kGroupInset, kGap, kGroupInset, kGap);
		bodyLayout->setSpacing(kGap);

		//字号字体行（原型 vector-font-color：连体字号+字体 1:1 + 26 色块=数值文字颜色）
		{
			QWidget* fontFull = new QWidget(body);
			QHBoxLayout* fontFullLayout = new QHBoxLayout(fontFull);
			fontFullLayout->setContentsMargins(0, 0, 0, 0);
			fontFullLayout->setSpacing(kColumnGap);
			PanelLabel* labelFont = new PanelLabel;
			regLabel(labelFont, "字号字体", "Font Size & Family");
			fontFullLayout->addWidget(labelFont);

			QWidget* fontRow = new QWidget(fontFull);
			QHBoxLayout* fontRowLayout = new QHBoxLayout(fontRow);
			fontRowLayout->setContentsMargins(0, 0, 0, 0);
			fontRowLayout->setSpacing(kGap);
			QFrame* pair = new QFrame(fontRow);
			pair->setObjectName("vectorFontPair");
			pair->setFixedHeight(kControl);
			pair->setStyleSheet(QString(
				"QFrame#vectorFontPair{background:#FFFFFF;border:1px solid %1;border-radius:%2px;}")
				.arg(kLineBorder).arg(kRadius));
			QHBoxLayout* pairLayout = new QHBoxLayout(pair);
			pairLayout->setContentsMargins(0, 0, 0, 0);
			pairLayout->setSpacing(0);
			_comboFontSize = new QComboBox(pair);
			_comboFontSize->setObjectName("vectorFontSize");
			_comboFontSize->setFixedHeight(kInner);
			_comboFontSize->setStyleSheet("QComboBox{border:0;border-radius:0;background:transparent;color:#303313;padding:0 4px;}");
			_comboFontFamily = new QComboBox(pair);
			_comboFontFamily->setObjectName("vectorFontFamily");
			_comboFontFamily->setFixedHeight(kInner);
			_comboFontFamily->setStyleSheet(QString(
				"QComboBox{border:0;border-radius:0;background:transparent;color:#303313;padding:0 4px;"
				"border-left:1px solid %1;}").arg(kLineBorder));
			pairLayout->addWidget(_comboFontSize, 1);
			pairLayout->addWidget(_comboFontFamily, 1);
			_btnFontColor = buildSwatch(_fontColor);
			_btnFontColor->setObjectName("vectorFontColor");
			_btnFontColor->setToolTip(QString::fromUtf8("数值文字颜色"));
			fontRowLayout->addWidget(pair, 1);
			fontRowLayout->addWidget(_btnFontColor);
			fontFullLayout->addWidget(fontRow, 1);
			bodyLayout->addWidget(fontFull);
		}

		//分量及合量颜色色盘（原型 4 列；显示顺序 X/Y/Z/X+Y+Z/X+Y/Y+Z/Z+X；着色=自定义才显示）
		{
			_paletteRow = new QWidget(body);
			_paletteRow->setObjectName("vectorComponentColors");
			QGridLayout* paletteLayout = new QGridLayout(_paletteRow);
			paletteLayout->setContentsMargins(0, 0, 0, 0);
			paletteLayout->setHorizontalSpacing(kChoiceGap);
			paletteLayout->setVerticalSpacing(kGap);
			const int order[7] = { 0, 1, 2, 6, 3, 4, 5 };
			static const char* names[7] = { "X", "Y", "Z", "X+Y", "Y+Z", "Z+X", "X+Y+Z" };
			for (int slot = 0; slot < 7; ++slot)
			{
				const int index = order[slot];
				QWidget* cell = new QWidget(_paletteRow);
				QHBoxLayout* cellLayout = new QHBoxLayout(cell);
				cellLayout->setContentsMargins(0, 0, 0, 0);
				cellLayout->setSpacing(kChoiceGap);
				QLabel* nameLabel = new QLabel(QString::fromUtf8(names[index]), cell);
				_componentSwatches[index] = buildSwatch(_componentColors[index]);
				_componentSwatches[index]->setObjectName(QString("vectorComponentColor%1").arg(index));
				_componentSwatches[index]->setToolTip(QString::fromUtf8("%1 颜色").arg(QString::fromUtf8(names[index])));
				_componentSwatches[index]->setEnabled(false);	//着色=数值时禁用（refreshDisplayState）
				cellLayout->addWidget(nameLabel, 1);
				cellLayout->addWidget(_componentSwatches[index]);
				paletteLayout->addWidget(cell, slot / 4, slot % 4);
			}
			_paletteRow->setVisible(false);	//着色方式默认"数值"
			bodyLayout->addWidget(_paletteRow);
		}

		//着色方式（数值/自定义，默认数值）
		{
			QWidget* colorRow = new QWidget(body);
			QHBoxLayout* colorRowLayout = new QHBoxLayout(colorRow);
			colorRowLayout->setContentsMargins(0, 0, 0, 0);
			colorRowLayout->setSpacing(kColumnGap);
			_choiceColorMode = new MxFunctions::mChoiceGroup(
			{ QString::fromUtf8("数值"), QString::fromUtf8("自定义") }, colorRow);
			_choiceColorMode->setObjectName("vectorColorModeChoices");
			PanelLabel* labelColor = new PanelLabel;
			regLabel(labelColor, "着色方式", "Color By");
			colorRowLayout->addWidget(labelColor);
			colorRowLayout->addWidget(_choiceColorMode, 1);
			bodyLayout->addWidget(colorRow);
		}

		//符号尺寸（自动/统一/缩放，默认自动；标题在上，三拼 3:控件 1——原型 fbd-symbol-field）
		{
			QWidget* field = new QWidget(body);
			QVBoxLayout* fieldLayout = new QVBoxLayout(field);
			fieldLayout->setContentsMargins(0, 0, 0, 0);
			fieldLayout->setSpacing(kGap);
			PanelLabel* labelSize = new PanelLabel;
			regLabel(labelSize, "符号尺寸", "Symbol Size");
			fieldLayout->addWidget(labelSize);
			QWidget* composite = new QWidget(field);
			QHBoxLayout* compositeLayout = new QHBoxLayout(composite);
			compositeLayout->setContentsMargins(0, 0, 0, 0);
			compositeLayout->setSpacing(kGap);
			_choiceSizeMode = new MxFunctions::mChoiceGroup(
			{ QString::fromUtf8("自动"), QString::fromUtf8("统一"), QString::fromUtf8("缩放") }, composite);
			_choiceSizeMode->setObjectName("vectorSizeChoices");
			compositeLayout->addWidget(_choiceSizeMode, 3);
			QWidget* controls = new QWidget(composite);
			QHBoxLayout* controlsLayout = new QHBoxLayout(controls);
			controlsLayout->setContentsMargins(0, 0, 0, 0);
			controlsLayout->setSpacing(0);
			_editSizeValue = new QLineEdit(QStringLiteral("1"), controls);		//符号统一尺寸
			_editSizeValue->setObjectName("vectorSizeValue");
			_editSizeValue->setFixedHeight(kControl);
			_editSizeValue->setStyleSheet(numberEditQss());
			_editScaleBy = new QLineEdit(QStringLiteral("1"), controls);		//符号缩放倍率
			_editScaleBy->setObjectName("vectorScaleBy");
			_editScaleBy->setFixedHeight(kControl);
			_editScaleBy->setStyleSheet(numberEditQss());
			controlsLayout->addWidget(_editSizeValue);
			controlsLayout->addWidget(_editScaleBy);
			compositeLayout->addWidget(controls, 1);
			fieldLayout->addWidget(composite);
			bodyLayout->addWidget(field);
		}

		//符号形式（头部/中点/尾部，默认尾部；三拼 3:箭头样式 1）
		{
			QWidget* field = new QWidget(body);
			QVBoxLayout* fieldLayout = new QVBoxLayout(field);
			fieldLayout->setContentsMargins(0, 0, 0, 0);
			fieldLayout->setSpacing(kGap);
			PanelLabel* labelPos = new PanelLabel;
			regLabel(labelPos, "符号形式", "Symbol Position");
			fieldLayout->addWidget(labelPos);
			QWidget* composite = new QWidget(field);
			QHBoxLayout* compositeLayout = new QHBoxLayout(composite);
			compositeLayout->setContentsMargins(0, 0, 0, 0);
			compositeLayout->setSpacing(kGap);
			_choiceSymbolPos = new MxFunctions::mChoiceGroup(
			{ QString::fromUtf8("头部"), QString::fromUtf8("中点"), QString::fromUtf8("尾部") }, composite);
			_choiceSymbolPos->setObjectName("vectorPositionChoices");
			compositeLayout->addWidget(_choiceSymbolPos, 3);
			_comboHeadForm = new QComboBox(composite);		//箭头样式
			_comboHeadForm->setObjectName("vectorHeadForm");
			_comboHeadForm->setFixedHeight(kControl);
			compositeLayout->addWidget(_comboHeadForm, 1);
			fieldLayout->addWidget(composite);
			bodyLayout->addWidget(field);
		}

		//内容超高时滚动（窄屏/低分辨率兜底；原型 fbd-popup-body 自适应）
		QScrollArea* scroll = new QScrollArea(_displayPopup);
		scroll->setWidgetResizable(true);
		scroll->setFrameShape(QFrame::NoFrame);
		scroll->setStyleSheet("QScrollArea{background:#FFFFFF;border:0;}");
		scroll->setWidget(body);
		popupLayout->addWidget(scroll);

		connect(close, &QToolButton::clicked, this, [this]() { closeDisplayPopup(); });
		//Qt::Popup 外点自动关闭（原型 document pointerdown 关闭语义由 Popup 窗口类型承接）
	}

	void mPostVectorPlotPanel::openDisplayPopup()
	{
		//开弹窗前退出拾取武装态（原型 openDisplay：deactivate）
		endPick();
		if (!_displayPopup->isVisible())
		{
			_displayPopup->setFixedWidth(qMax(this->width(), 320));	//原型弹窗宽=参数面板宽
			popupUnder(_displayPopup, _btnDisplayOpen);
		}
	}

	void mPostVectorPlotPanel::slot_displayOpenClicked()
	{
		ensureDisplayPopup();
		if (_displayPopup->isVisible())
			closeDisplayPopup();
		else
			openDisplayPopup();
	}

	void mPostVectorPlotPanel::closeDisplayPopup()
	{
		if (_displayPopup)
			_displayPopup->hide();
	}

	void mPostVectorPlotPanel::refreshDisplayState()
	{
		//原型 refreshDisplay：符号尺寸模式/着色方式/显示数值联动启停
		const int sizeMode = _choiceSizeMode ? _choiceSizeMode->currentIndex() : 0;
		const bool uniform = (sizeMode == 1);
		const bool scale = (sizeMode == 2);
		_editSizeValue->setVisible(!scale);
		_editSizeValue->setEnabled(uniform);
		_editScaleBy->setVisible(scale);
		_editScaleBy->setEnabled(scale);

		const bool byComponent = _choiceColorMode && _choiceColorMode->currentIndex() == 1;
		if (_paletteRow)
			_paletteRow->setVisible(byComponent);
		for (QPushButton* swatch : _componentSwatches)
		{
			if (swatch)
				swatch->setEnabled(byComponent);
		}

		const bool showValues = _checkShowValues->isChecked();
		_checkLabelPrefix->setEnabled(showValues);
		_comboFontSize->setEnabled(showValues);
		_comboFontFamily->setEnabled(showValues);
		_btnFontColor->setEnabled(showValues);
	}

	void mPostVectorPlotPanel::slot_sizeModeChanged(int index)
	{
		Q_UNUSED(index);
		refreshDisplayState();
	}

	void mPostVectorPlotPanel::refreshLegendRange()
	{
		//手动才可编辑并回填缓存（编辑+复位联动启停，眼睛独立——云图 refreshLegendBoundEditState
		//同口径）；非手动只读回显最近一次应用的解析值域（云图 12 位小数同口径），未应用保持
		//占位"暂无向量范围"
		const bool manual = _choiceLegendRange && _choiceLegendRange->currentIndex() == 2;
		const QPair<MxFunctions::mLegendBoundRow*, QString*> bounds[2] = {
			qMakePair(_legendMax, &_manualMax), qMakePair(_legendMin, &_manualMin) };
		const double resolved[2] = { _resolvedRange.second, _resolvedRange.first };   //0=最大值行 1=最小值行
		for (int i = 0; i < 2; ++i)
		{
			const auto& bound = bounds[i];
			bound.first->setEditEnabled(manual);
			bound.first->setResetEnabled(manual);
			QLineEdit* edit = bound.first->lineEdit();
			edit->blockSignals(true);
			if (manual)
			{
				edit->setText(*bound.second);
				edit->setPlaceholderText(QString());
			}
			else if (_requested)
			{
				edit->setText(MxFunctions::mNumberToString(resolved[i]));
				edit->setPlaceholderText(QString());
			}
			else
			{
				edit->clear();
				edit->setPlaceholderText(_langEn ? QString("No vector range") : QString::fromUtf8("暂无向量范围"));
			}
			edit->blockSignals(false);
		}
	}

	void mPostVectorPlotPanel::slot_legendRangeChanged(int index)
	{
		Q_UNUSED(index);
		//档位即时写 guidata（云图 slot_chooseLegendRange 同款；切帧重建链从此读档位）
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (currentCloudContext(&postRend, &guidata))
			guidata->_vectorPlotLegendRange = _choiceLegendRange->currentText();
		refreshLegendRange();
	}

	void mPostVectorPlotPanel::slot_legendMaxReset()
	{
		//复位=回最近一次应用的解析实际上限（云图 slot_resetMostValue 同语义）；
		//未应用/无解析回退原型演示值 1
		_manualMax = _requested ? MxFunctions::mNumberToString(_resolvedRange.second) : QStringLiteral("1");
		refreshLegendRange();
	}

	void mPostVectorPlotPanel::slot_legendMinReset()
	{
		_manualMin = _requested ? MxFunctions::mNumberToString(_resolvedRange.first) : QStringLiteral("0");
		refreshLegendRange();
	}

	void mPostVectorPlotPanel::slot_legendVisibilityClicked()
	{
		//云图 slot_showOrHideColorBar 同款：真作用当前视口色表（向量图源视口直作用，
		//与云图共享同一视口渲染图例）；隐藏态复位/自定义禁用
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&postRend, &guidata))
			return;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		guidata->_isColorTableShow = !guidata->_isColorTableShow;
		_legendHidden = !guidata->_isColorTableShow;
		_btnLegendHide->setText(_legendHidden
			? (_langEn ? QString("Show") : QString::fromUtf8("显示"))
			: (_langEn ? QString("Hide") : QString::fromUtf8("隐藏")));
		_btnLegendHide->setToolTip(_legendHidden
			? (_langEn ? QString("Show rendered legend") : QString::fromUtf8("显示渲染图例"))
			: (_langEn ? QString("Hide rendered legend") : QString::fromUtf8("隐藏渲染图例")));
		_btnLegendReset->setDisabled(!guidata->_isColorTableShow);
		_btnLegendCustom->setDisabled(!guidata->_isColorTableShow);
		postRender->getColorTableWindow()->setVisiable(guidata->_isColorTableShow);
		postRend->updatePaint();
	}

	void mPostVectorPlotPanel::slot_legendCustomClicked()
	{
		//云图 slot_customColorBar 同款：自定义=弹出渲染侧 Edit Legend（与视口内双击图例
		//同一窗口），配置直接作用于当前视口色带；原型"图例自定义"子组随功能对齐废除
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&postRend, &guidata))
			return;

		mxr::ZsEngine_NephogramWindow* colorTable = postRend->getColorTableRender();
		//空白视口（未显示结果，色带无内容）不弹窗——守卫条件同云图双击路径
		if (colorTable == nullptr || colorTable->GetColorCount() <= 0)
			return;

		MechPreData::PostViewportWidget* postWidget = getCVM->getPostWidget();
		if (postWidget == nullptr) return;
		mxr::CLegendEditorWidget* editor = postWidget->interaction()->getLegendEditor();
		if (editor == nullptr) return;

		editor->setColorTable(colorTable);
		editor->show();
		editor->raise();
		editor->activateWindow();
	}

	void mPostVectorPlotPanel::setInfoLabelButtonsState(bool visible)
	{
		//云图同款按钮态：可见分支=隐藏+复位/自定义可用；隐藏分支=显示+两钮禁用
		_infoLabelVisible = visible;
		_btnLabelHide->setText(visible ? (_langEn ? QString("Hide") : QString::fromUtf8("隐藏"))
			: (_langEn ? QString("Show") : QString::fromUtf8("显示")));
		_btnLabelReset->setEnabled(visible);
		_btnLabelCustom->setEnabled(visible);
	}

	void mPostVectorPlotPanel::slot_labelHide()
	{
		//信息标签显隐切换（云图 slot_labelHide 同款：作用视口右上文本）
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&postRend, &guidata))
			return;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;
		mxr::mPostFrameText* viewtext = postRender->getPostFrameText();
		if (viewtext == nullptr) return;

		guidata->_cloudInfoLabelVisible = !guidata->_cloudInfoLabelVisible;
		viewtext->setTopRightTextVisual(guidata->_cloudInfoLabelVisible);
		setInfoLabelButtonsState(guidata->_cloudInfoLabelVisible);
		postRend->updatePaint();
	}

	void mPostVectorPlotPanel::slot_labelReset()
	{
		//信息标签复位（云图 slot_labelReset 同款：恢复默认显示——可见+默认字体15px/黑色）
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&postRend, &guidata))
			return;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;
		mxr::mPostFrameText* viewtext = postRender->getPostFrameText();
		if (viewtext == nullptr) return;

		guidata->_cloudInfoLabelVisible = true;
		viewtext->setTopRightTextSize(15.0f);
		viewtext->setTopRightTextColor(QVector3D(0, 0, 0));
		viewtext->setTopRightTextVisual(true);
		setInfoLabelButtonsState(true);
		postRend->updatePaint();
	}

	void mPostVectorPlotPanel::slot_labelCustom()
	{
		//信息标签自定义=配置弹窗（共享注释面板顶层窗口化；云图 slot_labelCustom 同款）
		const int caseId = _postDB ? _postDB->getCaseId() : -1;
		mPostInfoHub::getInstance()->openNotesAsDialog(caseId);
	}

	void mPostVectorPlotPanel::slot_legendResetClicked()
	{
		//云图 slot_resetColorBar 同款：视口渲染图例位置复位（原型 reset 的组快照恢复为
		//演示级语义，功能对齐云图后废除；显隐/眼睛等真实态随 guidata 回填兜底）
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&postRend, &guidata))
			return;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		postRender->getColorTableWindow()->resetPos();
		postRend->updatePaint();
	}

	void mPostVectorPlotPanel::slot_legendBoundVisibilityToggled(bool isshow)
	{
		//云图 slot_setPositionVisible 同款：边界行眼睛=视口色表最大/最小值线显隐+持久化
		//（信号源区分行：两边界行各接本槽）
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&postRend, &guidata))
			return;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		MxFunctions::mLegendBoundRow* row = qobject_cast<MxFunctions::mLegendBoundRow*>(sender());
		if (row == _legendMax)
		{
			// 最值标注收编（unify-addobject-arch）：单侧掩码切换（首建=全量缺省隐藏=现状初始态）
			mxr::MinMaxLabelParams _mm;
			_mm.updateMask = mxr::MinMaxLabelParams::Mask_Max;
			_mm.maxVisible = isshow;
			PostCommon::ensureUpdateSceneObject(postRender.get(), _mm);
			guidata->_cloudShowMaxLine = isshow;
		}
		else if (row == _legendMin)
		{
			mxr::MinMaxLabelParams _mm;
			_mm.updateMask = mxr::MinMaxLabelParams::Mask_Min;
			_mm.minVisible = isshow;
			PostCommon::ensureUpdateSceneObject(postRender.get(), _mm);
			guidata->_cloudShowMinLine = isshow;
		}
		postRend->updatePaint();
	}

	void mPostVectorPlotPanel::slot_componentColorClicked()
	{
		//分量及合量色块取色（共用单实例取色弹窗，目标=点击的色块）
		QPushButton* swatch = qobject_cast<QPushButton*>(sender());
		if (!swatch)
			return;
		const int index = swatch->objectName().right(1).toInt();
		if (index < 0 || index > 6)
			return;
		if (!_colorPicker)
		{
			_colorPicker = new FbdColorPicker(this);
			connect(_colorPicker, &FbdColorPicker::colorChanged, this, [this](const QColor& color)
				{
					if (!_colorPickerTarget)
						return;
					if (_colorPickerTarget == _btnFontColor)
					{
						_fontColor = color;		//数值文字颜色
					}
					else
					{
						const int idx = _colorPickerTarget->objectName().right(1).toInt();
						if (idx >= 0 && idx <= 6)
							_componentColors[idx] = color;
					}
					applySwatchColor(_colorPickerTarget, color);
					clearStatus();	//原型 color input 事件清状态行
				});
		}
		_colorPickerTarget = swatch;
		_colorPicker->setCurrentColor(_componentColors[index]);
		popupUnder(_colorPicker, swatch);
	}

	void mPostVectorPlotPanel::slot_fontColorClicked()
	{
		//数值文字颜色取色（默认 --std-text #424b57）
		if (!_colorPicker)
		{
			_colorPicker = new FbdColorPicker(this);
			connect(_colorPicker, &FbdColorPicker::colorChanged, this, [this](const QColor& color)
				{
					if (!_colorPickerTarget)
						return;
					if (_colorPickerTarget == _btnFontColor)
					{
						_fontColor = color;
					}
					else
					{
						const int idx = _colorPickerTarget->objectName().right(1).toInt();
						if (idx >= 0 && idx <= 6)
							_componentColors[idx] = color;
					}
					applySwatchColor(_colorPickerTarget, color);
					clearStatus();
				});
		}
		_colorPickerTarget = _btnFontColor;
		_colorPicker->setCurrentColor(_fontColor);
		popupUnder(_colorPicker, _btnFontColor);
	}

	bool mPostVectorPlotPanel::validateForApply(QString* error)
	{
		//原型 apply 四步校验链（引擎未接入，提交仅置内存 requested 态）
		//①至少一个分量或合量
		bool anyComponent = _checkTotal->isChecked();
		for (QCheckBox* check : _componentChecks)
			anyComponent = anyComponent || check->isChecked();
		for (QCheckBox* check : _resultantChecks)
			anyComponent = anyComponent || check->isChecked();
		if (!anyComponent)
		{
			*error = _langEn ? QString("Select at least one component or resultant.")
				: QString::fromUtf8("请至少选择一个分量或合量。");
			return false;
		}

		const QString rangeError = _langEn ? QString("Please check parameter ranges.") : QString::fromUtf8("请检查参数范围。");

		//②全字段有效性（disabled 跳过；原型 checkValidity 含 min/required）。
		//返回 false 时 *error 已置；inPopup 字段无效先开弹窗（原型 openDisplay）
		auto numberFieldOk = [&](QLineEdit* edit, bool hasMin, double minValue, bool required, bool inPopup) -> bool
		{
			if (!edit->isEnabled() && !required)
				return true;
			const QString text = edit->text();
			double value = 0.0;
			const bool empty = text.trimmed().isEmpty();
			if ((required && empty) || (!empty && (!toFinite(text, &value) || (hasMin && value < minValue))))
			{
				if (inPopup)
					openDisplayPopup();
				else
					edit->setFocus();
				return false;
			}
			return true;
		};

		const bool manualRange = _choiceLegendRange->currentIndex() == 2;
		//插值阶数=QSpinBox 值域自保证（1..1000000），无需文本校验
		if (!numberFieldOk(_editDeformCoef, false, 0, true, false)			//变形系数：required 有效数
			|| !numberFieldOk(_editSizeValue, true, 0, false, true)		//统一尺寸：min 0
			|| !numberFieldOk(_editScaleBy, true, 0, false, true))			//缩放倍率：min 0
		{
			*error = rangeError;
			return false;
		}
		if (manualRange && (!numberFieldOk(_legendMax->lineEdit(), false, 0, true, false)
			|| !numberFieldOk(_legendMin->lineEdit(), false, 0, true, false)))
		{
			//手动范围：最大/最小值 required 有效数
			*error = rangeError;
			return false;
		}

		//③尺寸数值、缩放倍率必须大于 0
		auto positiveEnabledOk = [&](QLineEdit* edit) -> bool
		{
			if (!edit->isEnabled())
				return true;
			double value = 0.0;
			return toFinite(edit->text(), &value) && value > 0;
		};
		if (!positiveEnabledOk(_editSizeValue) || !positiveEnabledOk(_editScaleBy))
		{
			openDisplayPopup();	//尺寸系无效在弹窗内：先开弹窗
			*error = _langEn ? QString("Symbol size and scale factor must be greater than 0.")
				: QString::fromUtf8("尺寸数值和缩放倍率必须大于0。");
			return false;
		}

		//④手动范围：最大值>最小值；等比分布最小值>0
		if (manualRange)
		{
			double maxValue = 0.0, minValue = 0.0;
			toFinite(_legendMax->lineEdit()->text(), &maxValue);
			toFinite(_legendMin->lineEdit()->text(), &minValue);
			const bool geometric = _choiceSpacing->currentIndex() == 1;
			if (maxValue <= minValue || (geometric && minValue <= 0))
			{
				*error = _langEn ? QString("Maximum must exceed minimum; minimum must be positive for geometric spacing.")
					: QString::fromUtf8("最大值必须大于最小值；等比分布的最小值必须大于0。");
				return false;
			}
		}
		return true;
	}

	void mPostVectorPlotPanel::slot_applyClicked()
	{
		QString error;
		if (!validateForApply(&error))
		{
			setStatus(error);
			return;
		}

		//上下文：当前激活后处理视口（源视口直作用，随云图视口存续）
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentCloudContext(&postRend, &guidata))
		{
			setStatus(_langEn ? QString("No active result viewport; import a result first.")
				: QString::fromUtf8("当前无后处理视口，请先导入结果。"));
			return;
		}

		//面板选中工况/帧与视口不一致：先切视口（保证"所画即所选"；切帧链含矢量跟随插桩）
		syncViewToPanelSelection(postRend, guidata);

		//收集参数 → guidata（渲染重建最小集；通道索引 0..6 = X/Y/Z/X+Y/Y+Z/Z+X/X+Y+Z）
		guidata->_vectorPlotVarName = _comboVariable->currentText();
		bool checked[7] = { false };
		checked[0] = _componentChecks[0]->isChecked();
		checked[1] = _componentChecks[1]->isChecked();
		checked[2] = _componentChecks[2]->isChecked();
		checked[3] = _resultantChecks[0]->isChecked();
		checked[4] = _resultantChecks[1]->isChecked();
		checked[5] = _resultantChecks[2]->isChecked();
		checked[6] = _checkTotal->isChecked();
		guidata->_vectorPlotLabels = detail::buildVectorPlotLabels(checked, _componentColors);
		guidata->_vectorPlotScale = detail::vectorPlotScale(_choiceSizeMode->currentIndex(),
			_editSizeValue->text().trimmed().toDouble(), _editScaleBy->text().trimmed().toDouble());
		guidata->_vectorPlotArrowType = detail::vectorPlotArrowType(_choiceColorMode->currentIndex());
		guidata->_vectorPlotShowText = _checkShowValues->isChecked();
		//B5 图例值域档位：图例范围档位+手动值缓存+等差等比（applyVectorPlot 解析下发）
		guidata->_vectorPlotLegendRange = _choiceLegendRange->currentText();
		if (guidata->_vectorPlotLegendRange == QString::fromUtf8("手动"))
		{
			guidata->_vectorPlotLegendMax = _manualMax.trimmed().toDouble();
			guidata->_vectorPlotLegendMin = _manualMin.trimmed().toDouble();
		}
		guidata->_vectorPlotEquivariance = (_choiceSpacing->currentIndex() == 0);   //0=等差 1=等比

		endPick();
		if (guidata->_vectorPlotVarName.isEmpty() || guidata->_vectorPlotLabels.isEmpty())
		{
			//校验①已保证至少一通道，此处为防御边界（无结果库/变量表为空）
			setStatus(_langEn ? QString("No vector variable data available for this result.")
				: QString::fromUtf8("当前结果无可用矢量变量数据。"));
			return;
		}
		if (applyVectorPlot(postRend, guidata))
		{
			_requested = true;
			_btnClear->setEnabled(true);
			//边界行回显本次解析值域（非手动档只读显示实际范围）
			_resolvedRange = qMakePair(guidata->_vectorPlotResolvedMin, guidata->_vectorPlotResolvedMax);
			refreshLegendRange();
			setStatus(_langEn ? QString("Vector plot applied: case %1, frame %2.")
				.arg(guidata->_cloudCase).arg(guidata->_cloudFrame)
				: QString::fromUtf8("矢量图已渲染：%1 第%2帧。").arg(guidata->_cloudCase).arg(guidata->_cloudFrame));
		}
		else
		{
			//读场失败明细已由 applyVectorPlot 输出到输出窗口
			setStatus(_langEn ? QString("Vector plot failed; see output window for details.")
				: QString::fromUtf8("矢量图渲染失败，详见输出窗口。"));
		}
	}

	void mPostVectorPlotPanel::slot_clearClicked()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (currentCloudContext(&postRend, &guidata))
			removeVectorPlot(postRend, guidata);
		_requested = false;
		_btnClear->setEnabled(false);
		_resolvedRange = { 0.0, 0.0 };   //边界行随清除回占位（引擎侧色条已自动回云图域）
		refreshLegendRange();
		setStatus(_langEn ? QString("Vector plot cleared.")
			: QString::fromUtf8("已清除矢量图渲染。"));
	}

	void mPostVectorPlotPanel::setStatus(const QString& text)
	{
		_statusLabel->setText(text);
		_statusLabel->setVisible(!text.isEmpty());
	}

	void mPostVectorPlotPanel::clearStatus()
	{
		setStatus(QString());
	}

	void mPostVectorPlotPanel::regLabel(PanelLabel* label, const char* cn, const char* en)
	{
		_labelTexts.append({ label, cn, en });
		label->setText(_langEn ? QString::fromUtf8(en) : QString::fromUtf8(cn));
	}

	void mPostVectorPlotPanel::slot_changeLanguage(int language)
	{
		_langEn = (language == 1);
		//范围控制器先刷新（范围文本随语言重建，scopeTextChanged 联动拾取条默认文案），
		//随后重设面板文本
		if (_btnObjectScope != nullptr)
			_btnObjectScope->refreshLanguage(language);
		applyLanguageTexts();
	}

	void mPostVectorPlotPanel::applyLanguageTexts()
	{
		//折叠组标题
		_variableFold->setTitle(_langEn ? QString("Vector Variables") : QString::fromUtf8("向量变量"));
		_objectFold->setTitle(_langEn ? QString("Object & Coordinate System") : QString::fromUtf8("对象及坐标系"));
		_plotFold->setTitle(_langEn ? QString("Plot Settings") : QString::fromUtf8("绘图设置"));
		_legendFold->setTitle(_langEn ? QString("Legend") : QString::fromUtf8("图例"));

		for (const LabelText& entry : _labelTexts)
			entry.label->setText(_langEn ? QString::fromUtf8(entry.en) : QString::fromUtf8(entry.cn));

		//操作条
		_btnClear->setText(_langEn ? QString("Clear Rendering") : QString::fromUtf8("清除渲染"));
		_btnApply->setText(_langEn ? QString("Apply Rendering") : QString::fromUtf8("应用渲染"));

		//禁用占位与复选文案（云图对齐：跟踪坐标启用、边中点值默认勾禁用，无待接入 tooltip）
		_comboLayer->setToolTip(_langEn ? QString("Layered vector data not available yet.") : QString::fromUtf8("当前未接入分层向量数据"));
		_checkCornerData->setText(_langEn ? QString("Use Corner Data") : QString::fromUtf8("使用角点数据"));
		_checkCornerData->setToolTip(_langEn ? QString("Corner vector data not available yet.") : QString::fromUtf8("当前未接入角点向量数据"));
		_checkTrackingCoord->setText(_langEn ? QString("Use Tracking Coordinates") : QString::fromUtf8("使用跟踪坐标"));
		_checkMidsideValues->setText(_langEn ? QString("Midside Values") : QString::fromUtf8("边中点值"));
		_btnCoordAdd->setToolTip(_langEn ? QString("Quick create coordinate system") : QString::fromUtf8("快速新建坐标系"));
		//拾取条（mPost 栈）内建按钮 tooltip 为栈统一中文文案，不随语言切换（云图同口径）
		_checkShowValues->setText(_langEn ? QString("Show Values") : QString::fromUtf8("显示数值"));
		_checkLabelPrefix->setText(_langEn ? QString("Label Prefix") : QString::fromUtf8("标签前缀"));

		//下拉选项重填（blockSignals 保持选择；选项由 initOptions 已设默认）
		auto refillCombo = [](QComboBox* combo, const QStringList& items)
		{
			const int index = combo->currentIndex();
			combo->blockSignals(true);
			combo->clear();
			combo->addItems(items);
			if (index >= 0 && index < combo->count())
				combo->setCurrentIndex(index);
			combo->blockSignals(false);
		};
		refillCombo(_comboVariable, _langEn
			? QStringList{ "Displacement", "Velocity", "Rotational Velocity", "Acceleration", "Force" }
			: QStringList{ QString::fromUtf8("位移"), QString::fromUtf8("速度"), QString::fromUtf8("转动速度"),
				QString::fromUtf8("加速度"), QString::fromUtf8("力") });
		refillCombo(_objectTypeCombo, _langEn
			? QStringList{ "Element", "Part" }
			: QStringList{ QString::fromUtf8("单元"), QString::fromUtf8("部件") });
		//坐标系为数据驱动项（全局坐标系+guidata 自定义名，云图口径不翻译），跳过
		refillCombo(_comboLegendMode, _langEn
			? QStringList{ "All Parts", "Visible Parts" }
			: QStringList{ QString::fromUtf8("所有部件"), QString::fromUtf8("显示部件") });
		refillCombo(_comboFontFamily, _langEn
			? QStringList{ "Microsoft YaHei", "SimSun", "Arial" }
			: QStringList{ QString::fromUtf8("微软雅黑"), QString::fromUtf8("宋体"), QString("Arial") });
		refillCombo(_comboHeadForm, _langEn
			? QStringList{ "Arrow", "Line" }
			: QStringList{ QString::fromUtf8("箭头"), QString::fromUtf8("线条") });
		//字号为数字项不翻译，跳过

		//mChoiceGroup 选项重填（按索引保持选择）
		auto refillChoice = [](MxFunctions::mChoiceGroup* group, const QStringList& items)
		{
			const int index = group->currentIndex();
			group->setOptions(items);
			if (index >= 0 && index < group->count())
				group->setCurrentIndex(index);
		};
		refillChoice(_choiceLegendRange, _langEn
			? QStringList{ "Frame", "Global", "Manual" }
			: QStringList{ QString::fromUtf8("逐帧"), QString::fromUtf8("全程"), QString::fromUtf8("手动") });
		//setOptions 重建选项后说明 tooltip 需随语言复设（云图同款三条）
		_choiceLegendRange->setOptionTooltips(_langEn
			? QStringList{ "Range of the current frame", "Range over the entire case", "Set maximum and minimum manually" }
			: QStringList{ QString::fromUtf8("当前帧的数值范围"), QString::fromUtf8("当前工况整个分析过程的数值范围"),
				QString::fromUtf8("手动设置最大值和最小值") });
		refillChoice(_choiceSmoothMode, _langEn
			? QStringList{ "Smooth", "Discrete" }
			: QStringList{ QString::fromUtf8("平滑"), QString::fromUtf8("离散") });
		refillChoice(_choiceSpacing, _langEn
			? QStringList{ "Arithmetic", "Geometric" }
			: QStringList{ QString::fromUtf8("等差"), QString::fromUtf8("等比") });
		refillChoice(_choiceColorMode, _langEn
			? QStringList{ "Value", "Custom" }
			: QStringList{ QString::fromUtf8("数值"), QString::fromUtf8("自定义") });
		refillChoice(_choiceSizeMode, _langEn
			? QStringList{ "Auto", "Uniform", "Scale" }
			: QStringList{ QString::fromUtf8("自动"), QString::fromUtf8("统一"), QString::fromUtf8("缩放") });
		refillChoice(_choiceSymbolPos, _langEn
			? QStringList{ "Head", "Middle", "Tail" }
			: QStringList{ QString::fromUtf8("头部"), QString::fromUtf8("中点"), QString::fromUtf8("尾部") });

		//渲染图例三钮与图例边界行提示（复位/眼睛随态文案由 mLegendBoundRow 内部管理）
		_btnLegendHide->setText(_legendHidden ? (_langEn ? QString("Show") : QString::fromUtf8("显示"))
			: (_langEn ? QString("Hide") : QString::fromUtf8("隐藏")));
		_btnLegendHide->setToolTip(_legendHidden
			? (_langEn ? QString("Show rendered legend") : QString::fromUtf8("显示渲染图例"))
			: (_langEn ? QString("Hide rendered legend") : QString::fromUtf8("隐藏渲染图例")));
		_btnLegendReset->setText(_langEn ? QString("Reset") : QString::fromUtf8("复位"));
		_btnLegendCustom->setText(_langEn ? QString("Customize") : QString::fromUtf8("自定义"));
		//信息标签三钮（隐藏钮文案随显隐态，复位/自定义固定文案；云图同款）
		_btnLabelReset->setText(_langEn ? QString("Reset") : QString::fromUtf8("复位"));
		_btnLabelCustom->setText(_langEn ? QString("Customize") : QString::fromUtf8("自定义"));
		setInfoLabelButtonsState(_infoLabelVisible);
		_legendMax->setTooltips(_langEn ? QString("Refresh maximum") : QString::fromUtf8("刷新最大值"),
			_langEn ? QString("Maximum: hidden, click to show") : QString::fromUtf8("最大值：当前隐藏，点击显示"),
			_langEn ? QString("Maximum: shown, click to hide") : QString::fromUtf8("最大值：当前显示，点击隐藏"));
		_legendMin->setTooltips(_langEn ? QString("Refresh minimum") : QString::fromUtf8("刷新最小值"),
			_langEn ? QString("Minimum: hidden, click to show") : QString::fromUtf8("最小值：当前隐藏，点击显示"),
			_langEn ? QString("Minimum: shown, click to hide") : QString::fromUtf8("最小值：当前显示，点击隐藏"));

		//显示设置入口/弹窗头/色块提示
		QLabel* openText = _btnDisplayOpen->findChild<QLabel*>(QString::fromUtf8("vectorDisplayOpenText"));
		if (openText)
			openText->setText(_langEn ? QString("Settings") : QString::fromUtf8("设置"));
		QLabel* popupTitle = _displayPopup ? _displayPopup->findChild<QLabel*>(QString::fromUtf8("vectorDisplayTitle")) : nullptr;
		if (popupTitle)
			popupTitle->setText(_langEn ? QString("Display Settings") : QString::fromUtf8("显示设置"));
		_btnFontColor->setToolTip(_langEn ? QString("Value text color") : QString::fromUtf8("数值文字颜色"));

		//齿轮与帧导航提示（保持启停态）
		_btnOpenDeformation->setToolTip(_langEn ? QString("Open deformation settings") : QString::fromUtf8("设置模型变形"));
		refreshFrameButtons();

		_objectPickController->setDefultTip(_btnObjectScope ? _btnObjectScope->scopeText()
			: QString::fromUtf8("所有部件"));	//语言切换后空态默认文案随新语言范围标签（原 refreshPickText 职责）
		refreshLegendRange();
	}

	void mPostVectorPlotPanel::hideEvent(QHideEvent* event)
	{
		//面板隐藏=拾取控件收口+收回弹窗（宿主 setMainWidget 切换面板时触发；
		//原型 MutationObserver parameterPanel hidden → deactivate + closeDisplay）。
		//拾取条弹窗为 Qt::Popup 随宿主隐藏自动关闭（云图同款双保险口径）
		if (_objectPickController != nullptr)
			_objectPickController->setPickWidgetVisible(false);
		closeDisplayPopup();
		mPanelBase::hideEvent(event);
	}

	void mPostVectorPlotPanel::showEvent(QShowEvent* event)
	{
		mPanelBase::showEvent(event);
		//隐藏期 setPickWidgetVisible(false) 的对称恢复（云图同款；已拾取数据保留在
		//控件数据模型，条体显示随数据自动恢复）
		if (_objectPickController != nullptr)
			_objectPickController->setPickWidgetVisible(true);
	}
}
