#include "mPostDeformation.h"
#include "mCommonFunctions.h"   // unify-addobject-arch：色表值域段经 ColorLegend 台账下发

#include <QHBoxLayout>
#include <QColor>
#include <QFileInfo>
#include <cfloat>
#include <cmath>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"

//MxFunctions
#include "mGlobalSignals.h"
#include "mOtherFunction.h"
#include "mAppConfiguration.h"
#include "mChoiceGroup.h"
#include "mComboActionRow.h"

//Post
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostInfoHub.h"
#include "mPost3DResult.h"
#include "mPostValueProvider.h"
#include "mFbdColorPicker.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/REBUILD_PostOneFrameRender.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"

//PostIOFile（按变形变量读分量场）
#include "mIMxdbFile1.h"
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"

using namespace MxFunctions;
using namespace PostData;
using namespace MechPreData;

namespace PostWindow
{
	void applyPostDeformation(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata, bool updateRange)
	{
		if (postRend == nullptr || guidata == nullptr) return;

		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		auto* dataProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
		if (dataProxy == nullptr) return;

		auto query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(dataProxy->getRendQueryDataProxy());
		auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(dataProxy->getModelRenderDataProxy());

		//guidata 未写入前的界面默认值：仅显示当前构型 + 系数 1（对齐 2.0 默认）。
		//此处不能默认"同时显示"——云图重绘链（slot_showCloud）在模型加载/切帧时
		//即调用本函数，默认叠加会把初始构型在未选择时就显示出来
		QString shapeType = guidata->_shapeType;
		if (shapeType.isEmpty())
			shapeType = QStringLiteral("仅显示当前构型");
		QVector3D factor = guidata->_deformFactor;
		if (factor.isNull())
			factor = QVector3D(1, 1, 1);

		//初始构型显示档（渲染端按档切换无变形线形几何）：0=不显示 1=线框 2=边线 3=特征线。
		//档位以 _initialDisplayMode 为准（面板写入）；空值（旧链路仅写 _shapeType）时兜底：
		//同时显示→线框，其余→不显示
		int initialMode = 0;
		const QString& initialModeStr = guidata->_initialDisplayMode;
		if (initialModeStr == QString("线框"))
			initialMode = 1;
		else if (initialModeStr == QString("边线"))
			initialMode = 2;
		else if (initialModeStr == QString("特征线"))
			initialMode = 3;
		else if (initialModeStr.isEmpty() && shapeType == QString("同时显示"))
			initialMode = 1;

		//初始构型颜色：0=部件（渲染端查部件色表）1=网格（默认黑）2=自定义（用户色）
		int initialColorMode = 0;
		if (guidata->_initialColorMode == QString("网格"))
			initialColorMode = 1;
		else if (guidata->_initialColorMode == QString("自定义"))
			initialColorMode = 2;
		const QColor& initialColor = guidata->_initialCustomColor;

		if (shapeType == QString("仅显示初始构型"))
		{
			if (query_proxy != nullptr)
				query_proxy->setDeformationScale(QVector3D(0, 0, 0));
			postRender->setDeformationScale(QVector3D(0, 0, 0));
			postRender->setInitialShapeMode(0);
			//零系数下重算包围盒（对应 2.0 末尾无条件的 updateThreadAABB），供范围档"部件"取值
			if (model_proxy != nullptr)
				model_proxy->updateAABB();
		}
		else
		{
			//缩放策略三支分派（HyperView 口径，V1.1 §6"缩放策略解析"落地）：
			// 变形系数——k 直用（无单位倍率）；
			// 模型单位——k×位移（模型单位下的真实变形，HyperView "Model
			//   units"）——公式与变形系数完全一致，刻意同路非遗漏，仅语义框架
			//   不同（倍率 vs 模型单位长度）；
			// 模型比例——最大位移渲染为 k% 模型尺寸：k_eff = k × L/(100×dmax)
			// （L=当前模型 AABB 对角线，dmax=当前帧位移最大模；dmax/L 退化时
			// 保持 k——零位移本身不产生形变）。归一乘在等比例/分方向系数向量上
			// （保持方向比例）；只影响渲染值，不写回 guidata 系数（面板显示值不变）。
			if (guidata->_deformScaleStrategy == QString("模型比例"))
			{
				//位移场经新架构数据视图取（零拷贝，与流线渲染
				//REBUILD_PostStreamLineRender 同款口径；旧
				//postDB->getDisplacementDataByFrameID 路径不再用）
				double dmax = 0.0;
				if (model_proxy != nullptr)
				{
					if (IPostDataView* dataView = model_proxy->postDataView())
					{
						const std::vector<glm::vec3> dis =
							dataView->displacementsByFrame(postRender->currentFrame());   // S5：getCurrentAnimationFrame→currentFrame
						for (const auto& dvec : dis)
							dmax = qMax(dmax, double(glm::length(dvec)));
					}
				}
				mxr::AABB aabb = postRender->getCurrentModelData();   // S5 门面直接替换：getOneFrameRender→getModelRender→getModelAABB 下钻链消亡，getCurrentModelData 同源承载（绑定实例模型 AABB）
				const double modelSize = (aabb.maxEdge - aabb.minEdge).length();
				if (dmax > 0.0 && modelSize > 0.0)
					factor *= float(modelSize / (100.0 * dmax));
			}
			else if (guidata->_deformScaleStrategy == QString("模型单位"))
			{
				//模型单位（HyperView "Model units"）：k 直用即真实变形（见上
				//分派注释），无换算分支——显式空支以示三档齐备
			}
			//else：变形系数/未知值——k 直用

			if (query_proxy != nullptr)
				query_proxy->setDeformationScale(factor);
			postRender->setDeformationScale(QVector3D(factor));
			if (model_proxy != nullptr)
				model_proxy->updateAABB();

			postRender->setInitialShapeMode(initialMode);
		}
		//线档不透明（用户裁定）：a 分量渲染端不参与，仅取 rgb
		postRender->setInitialShapeColor(initialColorMode,
			QVector4D(initialColor.redF(), initialColor.greenF(), initialColor.blueF(), 0.0));

		//变形后重算图例最值（自 mPost3DResultPanel::updateDeformRangeValues 迁入）
		if (updateRange && query_proxy != nullptr)
		{
			float maxval = 0;
			float minval = 0;
			bool hasRange = false;
			if (guidata->_rangeIndex == 0)
			{
				maxval = query_proxy->getOriginalMaxData();
				minval = query_proxy->getOriginalMinData();
				hasRange = true;
			}
			else if (guidata->_rangeIndex == 1 && model_proxy != nullptr)
			{
				auto cur_min_max = model_proxy->getPartMinMax();
				minval = cur_min_max.first;
				maxval = cur_min_max.second;
				hasRange = true;
			}

		if (hasRange)
		{
			// 值域非法（倒挂或非有限）＝数据未就绪而非数值错误：结果未装载的
			// 哨兵态（max=-FLT_MAX、min=FLT_MAX）、空值读入落库的 ±DBL_MAX→±inf、
			// 半就绪部件聚合（min=FLT_MAX>max=真值）都算在内——静默跳过图例更新，
			// 不弹"最大值不能小于最小值"打扰
			const bool rangeInvalid = !(maxval >= minval)
				|| !std::isfinite(maxval) || !std::isfinite(minval);
			if (!rangeInvalid)
			{
				{   // 现状首笔窗写（原行含 S5 备注则随段注保留）
				    // 色表值域收编（C4 双效点·窗写段，与本函数伴生 setMinMaxData 成对）
				    mxr::ColorLegendParams _lg;
				    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				    _lg.targetFrame = -1;
				    _lg.valueMin = minval;
				    _lg.valueMax = maxval;
				    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
				}
				query_proxy->setCurrentMaxData(maxval);
				query_proxy->setCurrentMinData(minval);
				{
				    // 色表值域收编（C4 双效点·实例写段）：旧 setMinMaxData 原体（含窗写+刷新链）
				    mxr::ColorLegendParams _lg;
				    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
				    _lg.valueMin = minval;
				    _lg.valueMax = maxval;
				    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
				}

				guidata->_cloudMaxValue = maxval;
				guidata->_cloudMinValue = minval;
			}
		}
		}

		postRend->updatePaint();
	}

	namespace
	{
		//懒加载读取线程登记在云图面板的私有哈希（key=规范化结果文件路径），变形面板无法直接访问，
		//经 mPostInfoHub 拿云图面板按路径查（与云图显示链路 slot_showCloud 同口径）。
		//不能 qApp->findChildren 找 reader：reader 父链止于顶层主窗口，不在 QApplication 的
		//QObject 子树内，永远找不到（经最小程序实测验证）
		MIOFile::mIONativeFile* findNativeReader(const QString& postFile)
		{
			return mPostInfoHub::getInstance()->cloudResultPanel()->nativeReaderFor(postFile);
		}

		MIOFile::mIOVtkFile* findVtkReader(const QString& postFile)
		{
			return mPostInfoHub::getInstance()->cloudResultPanel()->vtkReaderFor(postFile);
		}

		//原生读取格式（OP2/ANSYS RST·RTH/LS-DYNA d3plot/H3D/ODB）均为懒加载链路
		bool isNativePostFile(const QString& postFile)
		{
			const QFileInfo fileInfo(postFile);
			const QString suffix = fileInfo.suffix().toLower();
			return suffix == QStringLiteral("op2") ||
				suffix == QStringLiteral("rst") ||
				suffix == QStringLiteral("rth") ||
				suffix == QStringLiteral("odb") ||
				suffix == QStringLiteral("h3d") ||
				fileInfo.fileName().compare(QStringLiteral("d3plot"), Qt::CaseInsensitive) == 0;
		}

		//懒加载链路 readVarDatas 会重发布帧结果数据/云图缓存（覆盖云图当前分量显示态），
		//读变形分量前后各调一次保持云图显示态不变；顺带在云图变量为位移时刷新位移哈希基线
		void restoreLazyCloudField(mPostDataBase* postDB,
			const MechPreData::mPostGUIData* guidata, int frame)
		{
			const QString cloudVar = guidata->_cloudVarName;
			if (cloudVar.isEmpty() || cloudVar == QString("SolidColor") || guidata->_cloudCompo.isEmpty())
				return;

			const QString postFile = postDB->getFile();
			if (isNativePostFile(postFile))
			{
				MIOFile::mIONativeFile* reader = findNativeReader(postFile);
				if (reader != nullptr)
					reader->readVarDatas(frame, cloudVar, guidata->_cloudCompo, guidata->_cloudComplex);
			}
			else if (MIOFile::mIOVtkFile::isVtkFile(postFile))
			{
				MIOFile::mIOVtkFile* reader = findVtkReader(postFile);
				if (reader != nullptr)
					reader->readVarDatas(frame, cloudVar, guidata->_cloudCompo, guidata->_cloudComplex);
			}
		}

		//读单分量标量场（键为连续节点id，与位移哈希同域）：
		//统一取值 provider（阶段2 三批）：native/vtk=发布后回取快照、老格式 mxdb=出参重载，
		//三链路 value1 均为连续节点id 键（同域成立）；新格式 mxdb 维持显式拒绝——其出参以
		//val.entityId（真实id）入键（mMxdbThread::readNewFormatScalarValues），与位移哈希
		//（连续id）域不一致，待键域归一专项后放开
		bool readNodeComponentField(mPostDataBase* postDB, const QString& postName,
			const QString& varName, const QString& compoName, int frame, QHash<int, double>& outField)
		{
			Q_UNUSED(postName)
			IPostValueProvider* provider = valueProviderFor(postDB);
			if (provider == nullptr)
				return false;
			const bool isMxdbNew = provider->sourceKind() == IPostValueProvider::SourceKind::MxdbNew;
			ValueRequest req;
			req.frame = frame;
			req.var = varName;
			req.compo = compoName;
			req.complexMode = QStringLiteral("Real");   //层传空：变形用矢量变量通常无层，有层按第一层理解
			Compones compone;
			const bool ok = !isMxdbNew && provider->fetch(req, compone);
			delete provider;
			if (!ok)
				return false;
			outField = compone.value1;
			return !outField.isEmpty();
		}

	//旧格式 mxdb（非懒加载且 getDataFlag()==false）：readVarDatas 出参键域与位移哈希
	//可能不一致（真实id vs 连续id），暂不支持按变量变形
	bool isLegacyMxdbPost(mPostDataBase* postDB)
	{
		const QString postFile = postDB->getFile();
		return !isNativePostFile(postFile) && !MIOFile::mIOVtkFile::isVtkFile(postFile) && !postDB->getDataFlag();
	}
}

//按变量读前三分量组装节点矢量场（键为连续节点id，与位移哈希同域）。
//变形变量切换与矢量图渲染共用；懒加载链路顺序：先恢复云图分量（刷新位移基线/
//显示态）→读三分量→再恢复云图分量（三次分量读取会覆盖结果数据）；
//结果哈希的写入由调用方在返回后执行（变形写位移哈希，矢量图写矢量哈希）
QHash<int, QVector3D> readNodeVectorField(mPostDataBase* postDB, const QString& postName,
	const QString& varName, int frame, const MechPreData::mPostGUIData* guidata)
{
	QStringList compos = postDB->getVariableCompos(varName);
	if (compos.size() < 3)
		return {};

	const bool isLazy = isNativePostFile(postDB->getFile()) || MIOFile::mIOVtkFile::isVtkFile(postDB->getFile());
	if (isLazy)
		restoreLazyCloudField(postDB, guidata, frame);

	compos = compos.mid(0, 3);
	QHash<int, double> comps[3];
	for (int i = 0; i < 3; ++i)
	{
		if (!readNodeComponentField(postDB, postName, varName, compos.at(i), frame, comps[i]))
			return {};
	}
	QHash<int, QVector3D> field = detail::assembleNodeVectorField(comps[0], comps[1], comps[2]);

	if (isLazy)
		restoreLazyCloudField(postDB, guidata, frame);
	return field;
}

	bool applyDeformVariableField(mxr::REBUILD_PostRend* postRend, MechPreData::mPostGUIData* guidata)
	{
		if (postRend == nullptr || guidata == nullptr) return false;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return false;
		if (postRender->frameProxy(postRender->currentFrame()) == nullptr) return false;   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8）

		QString postName = postRend->getPostDataName();
		QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (postDB == nullptr) return false;

		const int fi = guidata->_cloudFrame;
		const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;

		if (detail::isDefaultDeformVariable(guidata))
		{
			//默认（文件位移）变量：恢复缓存的原始位移；缓存缺失（如 OP2 导入后未显示过位移云图）
			//则按默认变量名现读三分量重建（懒加载链路读位移变量本身会顺带刷新位移哈希）
			if (guidata->_origDisplacementFrame == fi && !guidata->_origDisplacement.isEmpty())
			{
				postDB->setDisplacementDataByFrameID(fi, guidata->_origDisplacement);
				return true;
			}
			if (guidata->_deformVariable.isEmpty() || guidata->_deformVarNames.isEmpty())
				return true;	//从未选择过变量：数据库现状即文件位移，无需处理

			QHash<int, QVector3D> field = readNodeVectorField(postDB, postName, guidata->_deformVariable, fi, guidata);
			if (!field.isEmpty())
				postDB->setDisplacementDataByFrameID(fi, field);
			return true;
		}

		if (isLegacyMxdbPost(postDB))
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, isChinese
				? QString("旧格式结果文件暂不支持按变形变量切换，已保持原变形。")
				: QString("Legacy result files do not support switching deformation variable yet. Deformation kept unchanged."),
				true);
			return false;
		}

		QHash<int, QVector3D> field = readNodeVectorField(postDB, postName, guidata->_deformVariable, fi, guidata);
		if (field.isEmpty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, isChinese
				? QString("变量%1的变形分量数据读取失败，已保持原变形。").arg(guidata->_deformVariable)
				: QString("Failed to read deformation components of variable %1. Deformation kept unchanged.").arg(guidata->_deformVariable),
				true);
			return false;
		}

		//先恢复云图分量基线（readNodeVectorField 内已做）再缓存原始位移，最后写入新场
		if (guidata->_origDisplacementFrame != fi)
		{
			guidata->_origDisplacement = postDB->getDisplacementDataByFrameID(fi);
			guidata->_origDisplacementFrame = fi;
		}
		postDB->setDisplacementDataByFrameID(fi, field);
		return true;
	}

	namespace detail
	{
		//前三分量是否为空间矢量分量（mxdb 的 X/Y/Z、OP2 的 T1/T2/T3）
		bool isSpatialVectorCompos(const QStringList& compos)
		{
			if (compos.size() < 3)
				return false;
			const QStringList front = compos.mid(0, 3);
			return front == QStringList{ "X", "Y", "Z" } || front == QStringList{ "T1", "T2", "T3" };
		}

		QStringList filterDeformVariables(const QStringList& varNames,
			const QHash<QString, QStringList>& composByName, const QHash<QString, int>& varTypeByName)
		{
			QStringList outs;
			for (const QString& name : varNames)
			{
				if (!composByName.contains(name) || !varTypeByName.contains(name))
					continue;
				if (isSpatialVectorCompos(composByName.value(name)) && varTypeByName.value(name) == 0)
					outs.append(name);
			}
			return outs;
		}

		bool isDefaultDeformVariable(const MechPreData::mPostGUIData* guidata)
		{
			if (guidata == nullptr) return true;
			if (guidata->_deformVariable.isEmpty() || guidata->_deformVarNames.isEmpty())
				return true;
			return guidata->_deformVariable == guidata->_deformVarNames.first();
		}

		QHash<int, QVector3D> assembleNodeVectorField(const QHash<int, double>& compoX,
			const QHash<int, double>& compoY, const QHash<int, double>& compoZ)
		{
			QHash<int, QVector3D> field;
			QList<int> ids = compoX.keys();
			ids += compoY.keys();
			ids += compoZ.keys();
			for (int id : ids)
			{
				if (field.contains(id))
					continue;
				field.insert(id, QVector3D(compoX.value(id), compoY.value(id), compoZ.value(id)));
			}
			return field;
		}
	}

	//26×26 自定义颜色色块样式（applySwatchColor 口径：底色+1px 边框+2px 圆角）
	QString kInitialSwatchQss(const QColor& color)
	{
		return QString("QPushButton{background:%1;border:1px solid #DCE2E9;border-radius:2px;}")
			.arg(color.name());
	}

	mPostDeformationPanel::mPostDeformationPanel(QWidget* parent) : mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);

		/********************变形控制********************/
		FoldSection* deformFold = new FoldSection("变形控制", this);

		_comboDeformVariable = new QComboBox();
		deformFold->addRow("变形变量", _comboDeformVariable);

		_comboScaleStrategy = new QComboBox();
		_comboScaleStrategy->addItems({ "变形系数", "模型比例", "模型单位" });	//后两档渲染端暂未对接（TODO）
		deformFold->addRow("缩放策略", _comboScaleStrategy);

		//系数定义——公共控件 mChoiceGroup（共框圆点单选，原型 mirror-planes）
		_scaleModeChoice = new MxFunctions::mChoiceGroup({ "等比例", "分方向" }, this);
		_scaleModeChoice->setObjectName("deformScaleModeChoice");	//ui-shot 状态驱动：setCurrentIndex(1)=分方向
		deformFold->addRow("系数定义", _scaleModeChoice);

		//变形系数（等比例态）：整行输入；分方向态替换为 X/Y/Z 方向系数三行
		//（原型 vote-scale-rows 行级布局，非一行三输入框）
		_editUniformScale = new QLineEdit();
		_editUniformScale->setText(MxFunctions::mNumberToString(1, 'f', 6));
		_editUniformScale->setValidator(new QDoubleValidator(this));
		deformFold->addRow("变形系数", _editUniformScale);

		_editScaleX = new QLineEdit(); _editScaleX->setText("1");
		_editScaleX->setValidator(new QDoubleValidator(this));
		deformFold->addRow("X方向系数", _editScaleX);
		_editScaleY = new QLineEdit(); _editScaleY->setText("1");
		_editScaleY->setValidator(new QDoubleValidator(this));
		deformFold->addRow("Y方向系数", _editScaleY);
		_editScaleZ = new QLineEdit(); _editScaleZ->setText("1");
		_editScaleZ->setValidator(new QDoubleValidator(this));
		deformFold->addRow("Z方向系数", _editScaleZ);

		//坐标系——公共控件 mComboActionRow（下拉伸展＋H×H"＋"动作钮，原型
		//coordinate-composite）；"＋"待接入置灰（截面力面板同款）
		_coordRow = new MxFunctions::mComboActionRow(this);
		_comboCoordSystem = _coordRow->combo();
		_comboCoordSystem->addItem("全局坐标系");
		_btnCoordQuickCreate = _coordRow->actionButton();
		_btnCoordQuickCreate->setToolTip(QStringLiteral("快捷创建坐标系（待接入）"));
		_btnCoordQuickCreate->setEnabled(false);
		deformFold->addRow("坐标系", _coordRow);

		deformFold->refreshLayout();
		m_foldWidgets.append(deformFold);
		_mainLayout->addWidget(deformFold);

		/********************初始构型显示********************/
		FoldSection* initialFold = new FoldSection("初始构型显示", this);

		_comboInitialDisplay = new QComboBox();
		//V1.3 原型 4 项（无"仅显示初始构型"）；线框/边线/特征线经 setInitialShapeMode
		//切换无变形线形几何（面线+线/边界边/特征边）
		_comboInitialDisplay->addItems({ "不显示", "线框", "边线", "特征线" });
		initialFold->addRow("初始构型", _comboInitialDisplay);

		_comboInitialColorMode = new QComboBox();
		_comboInitialColorMode->setObjectName("deformInitialColorMode");	//ui-shot 状态驱动：setCurrentIndex(2)=自定义
		_comboInitialColorMode->addItems({ "部件", "网格", "自定义" });	//颜色来源渲染端暂未对接（TODO）
		_btnInitialColor = new QPushButton();
		_btnInitialColor->setFixedSize(QSize(26, 26));
		_btnInitialColor->setCursor(Qt::PointingHandCursor);
		_btnInitialColor->setToolTip("自定义初始构型颜色");
		_btnInitialColor->setStyleSheet(kInitialSwatchQss(QColor("#1890ff")));
		_btnInitialColor->hide();
		QWidget* colorRow = new QWidget();
		QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
		colorLayout->setContentsMargins(0, 0, 0, 0);
		colorLayout->setSpacing(6);
		colorLayout->addWidget(_comboInitialColorMode);
		colorLayout->addWidget(_btnInitialColor);
		initialFold->addRow("颜色", colorRow);

		initialFold->refreshLayout();
		m_foldWidgets.append(initialFold);
		_mainLayout->addWidget(initialFold);
		_mainLayout->addStretch();

		//connect
		connect(_comboDeformVariable, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseDeformVariable(const QString&)));
		connect(_editUniformScale, &QLineEdit::returnPressed, this, &mPostDeformationPanel::slot_applyDeformation);
		connect(_editScaleX, &QLineEdit::returnPressed, this, &mPostDeformationPanel::slot_applyDeformation);
		connect(_editScaleY, &QLineEdit::returnPressed, this, &mPostDeformationPanel::slot_applyDeformation);
		connect(_editScaleZ, &QLineEdit::returnPressed, this, &mPostDeformationPanel::slot_applyDeformation);
		connect(_scaleModeChoice, &MxFunctions::mChoiceGroup::currentIndexChanged, this, &mPostDeformationPanel::slot_chooseScaleMode);
		connect(_comboScaleStrategy, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_saveDeformOptions()));
		connect(_comboCoordSystem, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_saveDeformOptions()));
		connect(_comboInitialDisplay, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseInitialDisplay(const QString&)));
		connect(_comboInitialColorMode, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_chooseInitialColorMode(const QString&)));
		connect(_btnInitialColor, &QPushButton::clicked, this, &mPostDeformationPanel::slot_pickInitialColor);

		//初始行显隐（等比例：变形系数行；分方向：X/Y/Z 方向系数三行）
		updateScaleModePages();

		this->setMinimumWidth(300);
	}

	void mPostDeformationPanel::newDeformationPanel(int caseid)
	{
		Q_UNUSED(caseid);

		//变形控制作用于当前激活云图视口
		mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr) return;

		_curViewName = postRend->getViewportName();
		loadDeformationSettings();
	}

	void mPostDeformationPanel::setCurViewName(const QString& viewName)
	{
		_curViewName = viewName;
		loadDeformationSettings();
	}

	bool mPostDeformationPanel::currentContext(mxr::REBUILD_PostRend** postRendOut, MechPreData::mPostGUIData** guidataOut)
	{
		*postRendOut = nullptr;
		*guidataOut = nullptr;

		mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return false;
		if (postRend->getViewportName() != _curViewName)
			return false;

		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return false;

		if (postRender->frameProxy(postRender->currentFrame()) == nullptr) return false;   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8）

		MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return false;

		*postRendOut = postRend;
		*guidataOut = guidata;
		return true;
	}

	void mPostDeformationPanel::syncToGUIData()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentContext(&postRend, &guidata))
			return;

		guidata->_deformVariable = _comboDeformVariable->currentText();
		guidata->_deformScaleStrategy = _comboScaleStrategy->currentText();
		guidata->_deformCoordSystem = _comboCoordSystem->currentText();
		guidata->_initialDisplayMode = _comboInitialDisplay->currentText();
		guidata->_initialColorMode = _comboInitialColorMode->currentText();

		//显示构型映射（对齐 2.0 三态）：不显示→仅当前构型；线框/边线/特征线→
		//同时显示（叠加初始构型，具体线形由 setInitialShapeMode 按档切换）；
		//仅显示初始构型→变形系数置 0 只看未变形构型
		if (guidata->_initialDisplayMode == QString("不显示"))
			guidata->_shapeType = QString("仅显示当前构型");
		else if (guidata->_initialDisplayMode == QString("仅显示初始构型"))
			guidata->_shapeType = QString("仅显示初始构型");
		else
			guidata->_shapeType = QString("同时显示");

		QVector3D factor = guidata->_deformFactor;
		if (factor.isNull())
			factor = QVector3D(1, 1, 1);
		//系数定义：0=等比例 1=分方向（mChoiceGroup；deformType 沿用 guidata 字符串协议）
		QString deformtype = _scaleModeChoice->currentIndex() == 1 ? QString("不一致") : QString("一致");
		guidata->_deformType = deformtype;

		bool hasInvalid = false;
		//无效输入恢复"上次有效值"（设计稿验收：不覆盖上次有效状态；guidata 现值即上次有效值）
		auto validateValue = [&](QLineEdit* edit, double lastValid) -> double {
			bool ok = false;
			double val = edit->text().trimmed().toDouble(&ok);
			if (!ok)
			{
				edit->setText(MxFunctions::mNumberToString(lastValid, 'f', 6));
				hasInvalid = true;
				return lastValid;
			}
			return val;
		};

		if (deformtype == QString("一致"))
		{
			//等比例与分方向各自保留数值：单值存 _deformUniformScale，XYZ 存 _deformFactor
			double val = validateValue(_editUniformScale, guidata->_deformUniformScale);
			guidata->_deformUniformScale = val;
			factor = QVector3D(val, val, val);
		}
		else
		{
			factor.setX(validateValue(_editScaleX, factor.x()));
			factor.setY(validateValue(_editScaleY, factor.y()));
			factor.setZ(validateValue(_editScaleZ, factor.z()));
		}

		if (hasInvalid)
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, QString("变形系数输入无效，已恢复上次有效值。"), true);
			}
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, QString("Invalid deformation coefficient, restored the last valid value."), true);
			}
		}

		guidata->_deformFactor = factor;
	}

	void mPostDeformationPanel::slot_chooseDeformVariable(const QString& text)
	{
		Q_UNUSED(text);

		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentContext(&postRend, &guidata))
			return;

		QString postName = postRend->getPostDataName();
		QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (postDB == nullptr) return;

		const QString prevVariable = _lastDeformVariable;
		syncToGUIData();
		if (!applyDeformVariableField(postRend, guidata))
		{
			//读场失败：界面与 guidata 回滚到上一生效变量（失败提示已由 applyDeformVariableField 输出）
			guidata->_deformVariable = prevVariable;
			_comboDeformVariable->blockSignals(true);
			if (!prevVariable.isEmpty())
				_comboDeformVariable->setCurrentText(prevVariable);
			_comboDeformVariable->blockSignals(false);
			return;
		}
		_lastDeformVariable = _comboDeformVariable->currentText();

		/*
		* 云图联动（同用户在云图面板切变量的效果）：切换变形变量后云图一并切到该变量重绘。
		* 经 hub 云图面板 reloadCloudFromGUIData 走既有全量重绘链（updateResultInfoGUI 回填+
		* slot_showCloud：重读云图数据→建帧代理[slot_showCloud 内已插桩 applyDeformVariableField，
		* 幂等]→着色/图例/部件显隐），不手搓代理重建（首版最小路径漏着色模式重应用致云图消失）。
		* 分量默认 Mag（无则首分量），层/复数沿用 guidata 现值（空间矢量变量无层，复数缺省实部）。
		*/
		guidata->_cloudVarName = guidata->_deformVariable;
		QStringList cnames = postDB->getVariableCompos(guidata->_cloudVarName);
		guidata->_cloudCompos = cnames;
		guidata->_cloudCompo = cnames.contains(QString("Mag"))
			? QString("Mag")
			: (cnames.isEmpty() ? QString() : cnames.first());
		mPostInfoHub::getInstance()->cloudResultPanel()->reloadCloudFromGUIData(_curViewName);

		//动画帧代理的位移在帧创建时已物化，已生成的动画不随变量切换刷新（重新生成动画即用新场，TODO）
	}

	void mPostDeformationPanel::slot_applyDeformation()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentContext(&postRend, &guidata))
			return;

		syncToGUIData();
		applyPostDeformation(postRend, guidata);
	}

	void mPostDeformationPanel::slot_chooseScaleMode()
	{
		updateScaleModePages();
		//切换即时保存模式与当前输入（不触发渲染；另一模式的数值保留待切回恢复）
		syncToGUIData();
	}

	void mPostDeformationPanel::slot_saveDeformOptions()
	{
		//缩放策略/坐标系切换：仅保存选项（渲染端对应能力为待接入，不触发重绘）
		syncToGUIData();
	}

	void mPostDeformationPanel::slot_chooseInitialDisplay(const QString& text)
	{
		Q_UNUSED(text);

		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentContext(&postRend, &guidata))
			return;

		syncToGUIData();
		applyPostDeformation(postRend, guidata);
	}

	void mPostDeformationPanel::slot_chooseInitialColorMode(const QString& text)
	{
		Q_UNUSED(text);

		updateInitialColorVisibility();

		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentContext(&postRend, &guidata))
			return;

		guidata->_initialColorMode = _comboInitialColorMode->currentText();
		//颜色来源切换即时生效（不重算图例最值）
		applyPostDeformation(postRend, guidata, false);
	}

	void mPostDeformationPanel::slot_pickInitialColor()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentContext(&postRend, &guidata))
			return;

		//共享取色弹窗（标签/剖切面板同款）：懒创建，锚定色块下方弹出，越界收回屏内；
		//取色过程 colorChanged 实时回调 applyInitialColor 即时生效
		if (!_colorPicker)
		{
			_colorPicker = new FbdColorPicker(this);
			connect(_colorPicker, &FbdColorPicker::colorChanged, this, [this](const QColor& color) {
				applyInitialColor(color);
			});
		}
		_colorPicker->setCurrentColor(guidata->_initialCustomColor);
		const QPoint global = _btnInitialColor->mapToGlobal(QPoint(0, _btnInitialColor->height() + 2));
		_colorPicker->popupAt(global);
	}

	void mPostDeformationPanel::applyInitialColor(const QColor& color)
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		if (!currentContext(&postRend, &guidata))
			return;

		guidata->_initialCustomColor = color;
		_btnInitialColor->setStyleSheet(kInitialSwatchQss(color));
		//自定义色即时生效（不重算图例最值）
		applyPostDeformation(postRend, guidata, false);
	}

	void mPostDeformationPanel::loadDeformationSettings()
	{
		mxr::REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		_comboDeformVariable->blockSignals(true);
		_comboScaleStrategy->blockSignals(true);
		_comboCoordSystem->blockSignals(true);
		_comboInitialDisplay->blockSignals(true);
		_comboInitialColorMode->blockSignals(true);

		//变形变量选项：当前帧节点矢量变量（分量数≥3）；guidata 未同步（面板先于云图打开）时按数据库现查兜底
		QStringList deformVars = guidata->_deformVarNames;
		if (deformVars.isEmpty())
		{
			QString postName = postRend->getPostDataName();
			mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
			if (postDB != nullptr)
			{
				QStringList frameVars = postDB->getVariableByFrameID(guidata->_cloudFrame);
				QHash<QString, QStringList> composMap;
				QHash<QString, int> varTypes;
				for (const QString& name : frameVars)
				{
					composMap.insert(name, postDB->getVariableCompos(name));
					varTypes.insert(name, postDB->getVariableType(name));
				}
				deformVars = detail::filterDeformVariables(frameVars, composMap, varTypes);
			}
		}
		_comboDeformVariable->clear();
		_comboDeformVariable->addItems(deformVars);
		if (!guidata->_deformVariable.isEmpty() && deformVars.contains(guidata->_deformVariable))
			_comboDeformVariable->setCurrentText(guidata->_deformVariable);
		else if (!deformVars.isEmpty())
		{
			//默认=列表首项（结果文件的位移变量）；无效旧选项回退首候选（原型口径）
			guidata->_deformVariable = deformVars.first();
			_comboDeformVariable->setCurrentIndex(0);
		}
		else
		{
			//无可变形变量：回退文件位移（isDefaultDeformVariable 口径），不保留占位选项
			guidata->_deformVariable.clear();
		}
		_lastDeformVariable = _comboDeformVariable->currentText();

		if (!guidata->_deformScaleStrategy.isEmpty())
			_comboScaleStrategy->setCurrentText(guidata->_deformScaleStrategy);

		//坐标系：全局坐标系 + 云图坐标系列表
		_comboCoordSystem->clear();
		_comboCoordSystem->addItem("全局坐标系");
		for (const QString& name : guidata->_cloudCoordinateNames)
		{
			if (name != QString("全局坐标系"))
				_comboCoordSystem->addItem(name);
		}
		if (!guidata->_deformCoordSystem.isEmpty())
			_comboCoordSystem->setCurrentText(guidata->_deformCoordSystem);

		//系数定义回填（setCurrentIndex 同值不发信号，blockSignals 双保险防
		//中途 currentIndexChanged 触发 slot_chooseScaleMode 写编辑缓冲）
		const bool uniform = (guidata->_deformType != QString("不一致"));
		_scaleModeChoice->blockSignals(true);
		_scaleModeChoice->setCurrentIndex(uniform ? 0 : 1);
		_scaleModeChoice->blockSignals(false);

		QVector3D factor = guidata->_deformFactor;
		if (factor.isNull())
			factor = QVector3D(1, 1, 1);
		//等比例与分方向分别回填各自保存值（单值 _deformUniformScale，XYZ _deformFactor）
		_editUniformScale->setText(MxFunctions::mNumberToString(guidata->_deformUniformScale, 'f', 6));
		_editScaleX->setText(MxFunctions::mNumberToString(factor.x(), 'f', 6));
		_editScaleY->setText(MxFunctions::mNumberToString(factor.y(), 'f', 6));
		_editScaleZ->setText(MxFunctions::mNumberToString(factor.z(), 'f', 6));

		if (!guidata->_initialDisplayMode.isEmpty())
			_comboInitialDisplay->setCurrentText(guidata->_initialDisplayMode);
		if (!guidata->_initialColorMode.isEmpty())
			_comboInitialColorMode->setCurrentText(guidata->_initialColorMode);
		_btnInitialColor->setStyleSheet(kInitialSwatchQss(guidata->_initialCustomColor));

		_comboDeformVariable->blockSignals(false);
		_comboScaleStrategy->blockSignals(false);
		_comboCoordSystem->blockSignals(false);
		_comboInitialDisplay->blockSignals(false);
		_comboInitialColorMode->blockSignals(false);

		updateScaleModePages();
		updateInitialColorVisibility();
	}

	void mPostDeformationPanel::updateScaleModePages()
	{
		const bool uniform = _scaleModeChoice->currentIndex() == 0;
		//行级显隐（setRowVisible 整行摘除/插回并同步折叠高度帽—— setVisible
		//不收行距且高度帽不刷新，见 FoldSection::setRowVisible 注释）；
		//非当前模式字段禁用并隐藏（原型口径：切换只改可见性与启用，不清空另一模式数值）
		FoldSection* deformFold = nullptr;
		for (auto fold : m_foldWidgets)
		{
			if (fold && fold->getTitle() == QString("变形控制"))
			{
				deformFold = fold;
				break;
			}
		}
		if (deformFold == nullptr)
			return;

		deformFold->setRowVisible(_editUniformScale, uniform);
		deformFold->setRowVisible(_editScaleX, !uniform);
		deformFold->setRowVisible(_editScaleY, !uniform);
		deformFold->setRowVisible(_editScaleZ, !uniform);
		_editUniformScale->setDisabled(!uniform);
		_editScaleX->setDisabled(uniform);
		_editScaleY->setDisabled(uniform);
		_editScaleZ->setDisabled(uniform);
	}

	void mPostDeformationPanel::updateInitialColorVisibility()
	{
		_btnInitialColor->setVisible(_comboInitialColorMode->currentText() == QString("自定义"));
	}
}
