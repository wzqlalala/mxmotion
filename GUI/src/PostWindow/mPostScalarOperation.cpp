#include "mPostScalarOperation.h"
#include "mCommonFunctions.h"   // unify-addobject-arch：色表值域段经 ColorLegend 台账下发

#include <QFormLayout>
#include <QRegularExpression>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"

//MxFunctions
#include "mGlobalSignals.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"

//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;

namespace PostWindow
{
	mPostScalarOperationPanel::mPostScalarOperationPanel(QWidget* parent) : mPanelBase(parent)
	{
		_curPostRend = nullptr;

		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);
		this->setStyleSheet(
			"mPostScalarOperationPanel { background-color: white; }"

			"QComboBox QAbstractItemView {"
			"    background-color: white;"
			"    color: black;"
			"    selection-background-color: #3399ff;"
			"    selection-color: black;"
			"}");

		/********************标量运算********************/
		FoldSection* scalarFold = new FoldSection("标量运算", this);

		QFormLayout* formLayout = new QFormLayout();

		_opCombo = new QComboBox(this);
		_opCombo->addItem("+");
		_opCombo->addItem("-");
		_opCombo->addItem("*");
		_opCombo->addItem("/");
		formLayout->addRow("运算符", _opCombo);

		_valueEdit = new QLineEdit(this);
		_valueEdit->setPlaceholderText("请输入标量数值");
		formLayout->addRow("标量值", _valueEdit);

		scalarFold->addRow(formLayout);

		//生成
		_buttonGenerate = new QPushButton("生成");
		scalarFold->addRow(_buttonGenerate);
		connect(_buttonGenerate, SIGNAL(clicked()), this, SLOT(slot_generateScalar()));

		scalarFold->refreshLayout();
		m_foldWidgets.append(scalarFold);
		_mainLayout->addWidget(scalarFold);
		_mainLayout->addStretch();

		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createScalarOperationViewport, this, &mPostScalarOperationPanel::slot_createScalarViewport);
	}

	void mPostScalarOperationPanel::newScalarPanel(int caseid)
	{
		QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
		if (postDB == nullptr) return;

		//创建标量运算节点
		MechCommon::mMechGlobalSignals::getInstance()->createScalarOperationSig();
	}

	void mPostScalarOperationPanel::slot_createScalarViewport(QString viewport)
	{
		//创建视口
		mViewTreeFunction::newViewport(MxFunctions::ViewportPost3D, viewport);
		MechPreData::mViewportPrivate* viewportData = VMManager->getCurrentViewportPrivate();
		if (viewportData->getViewportType() == MxFunctions::ViewportPost3D)
		{
			_curPostRend = dynamic_cast<mxr::REBUILD_PostRend*>(viewportData->getRendWidget());
		}

		_curViewName = viewport;
		if (_curPostRend)
		{
			_curPostRend->FitView();
			_curPostRend->updatePaint();
		}
	}

	QString mPostScalarOperationPanel::getCloudViewName(const QString& viewName)
	{
		//源云图定位：优先管理器关系（ScalarOperationInfo.viewName → resultId → CloudItem.viewName）
		//——显示名接管后视口名不再携带 "_Scalar<N>" 后缀，名字解析只作兜底
		//（派生视口创建信号先于 addItem 登记的窗口期，彼时视口名仍带后缀）
		const QString byInfo = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post)
			.getCloudViewNameByView(PostData::ItemType::ScalarOperationItem, viewName);
		if (!byInfo.isEmpty())
			return byInfo;
		QRegularExpression re("^(.*)(_Scalar\\d+)$");
		QRegularExpressionMatch match = re.match(viewName);
		if (match.hasMatch()) {
			return match.captured(1);
		}
		return viewName;
	}

	void mPostScalarOperationPanel::setCurViewName(const QString& viewName)
	{
		_curViewName = viewName;

		// 回显标量运算自身持久化参数
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		_opCombo->setCurrentIndex(guidata->_scalarOperationOp);
		_valueEdit->setText(MxFunctions::mNumberToString(guidata->_scalarOperationValue));
	}

	void mPostScalarOperationPanel::setShowMode()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr) return;

		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr) return;
		switch (viewportPrivate->_showViewCheckedStatus)
		{
		case 0:postRender->setShowFuntion(ShowFuntion::WireEdge);
			break;
		case 1:postRender->setShowFuntion(ShowFuntion::WireFrame);
			break;
		case 2:postRender->setShowFuntion(ShowFuntion::ElementFace);
			break;
		case 3:postRender->setShowFuntion(ShowFuntion::SmoothShaded);
			break;
		}
		postRend->updatePaint();
	}

	void mPostScalarOperationPanel::slot_generateScalar()
	{
		// 1. 校验当前视口
		if (_curViewName.isEmpty()) return;
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
		if (rebuildPostRend == nullptr) return;

		const auto& postRender = rebuildPostRend->getPostRender();
		if (postRender == nullptr) return;

		// 2. 获取 mPostDataBase
		QString postName = rebuildPostRend->getPostDataName();
		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (modeldb == nullptr) return;

		// 3. 获取父云图视口的 GUI 数据（云图参数保存在父视口，而非标量运算视口）
		QString cloudView = getCloudViewName(_curViewName);
		REBUILD_PostRend* cloudPostRend = getCVM->getPostRenderByViewName(cloudView);
		if (cloudPostRend == nullptr) return;
		mPostGUIData* cloudData = getCVM->getGUIDataByPostRender(cloudPostRend);
		if (cloudData == nullptr) return;

		int frame = cloudData->_cloudFrame;
		if (frame < 0) return;

		// 4. 读取运算符和标量值
		int op = _opCombo->currentIndex();                  // 0:+ 1:- 2:* 3:/
		double scalar = _valueEdit->text().toDouble();
		if (op == 3 && qFuzzyIsNull(scalar))
		{
			// 除数不能为0
			return;
		}

		// 5. 读取当前帧原始结果数据（避免多次生成叠加）
		QHash<int, double> frameResult = modeldb->getOriginalResultDataByFrameID(frame);
		if (frameResult.isEmpty())
		{
			// 若原始数据未备份，则将当前结果数据作为原始数据备份，并以其作为计算基础
			modeldb->backupOriginalResultDataByFrameID(frame);
			frameResult = modeldb->getOriginalResultDataByFrameID(frame);
		}
		if (frameResult.isEmpty()) return;

		// 6. 逐值计算
		QHash<int, double> newResult;
		for (auto it = frameResult.constBegin(); it != frameResult.constEnd(); ++it)
		{
			double v = it.value();
			switch (op)
			{
			case 0: v += scalar; break;
			case 1: v -= scalar; break;
			case 2: v *= scalar; break;
			case 3: v /= scalar; break;
			}
			newResult[it.key()] = v;
		}

		// 7. 写回 mPostDataBase，并记录结果类型
		int resultType = modeldb->getFrameResultType(frame);
		if (resultType < 0) resultType = cloudData->_resultType;
		modeldb->setScalarOperationData(newResult, op, scalar, frame, resultType);

		// 8. 持久化到当前标量运算视口的 mPostGUIData
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
		if (guidata != nullptr)
		{
			guidata->_scalarOperationOp = op;
			guidata->_scalarOperationValue = scalar;
			guidata->_scalarOperationFrame = frame;
			guidata->_hasScalarOperation = true;
		}

		// 9. 装载/绑定（S5 门面直接替换，B1b(M1) 口径延续）：旧手造代理三连调
		//（createFrameDataProxy→setCurFrameDataProxy→setRendCurrentFrameData）随移除集
		// 消亡——标量运算结果已于步骤 7 写回 mPostDataBase，门面装载工厂从库现造代理
		// 即含新值；attachModel（幂等换绑）+requestFrames（渐进装载）+seekFrame（未就绪
		// 保持上一帧，frameReady 后由云图刷新链重入呈现）
		if (!postRender->attachModel(modeldb.data(), rebuildPostRend->getViewportName()))
			return;
		postRender->requestFrames(frame, frame);
		if (!postRender->seekFrame(frame))
			return;
		mxr::PostFrameDataProxy* frameData = postRender->frameProxy(postRender->currentFrame());   // F8：绑定点代理现取（换帧/重装载后重取）
		if (frameData == nullptr) return;

		QPointer<PostData::mPostModelData> modelData = modeldb->getModelData(frame);
		if (modelData == nullptr) return;

		mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frameData->getRendQueryDataProxy());
		if (query_proxy != nullptr)
		{
			for (auto it : modeldb->getAllRigidWallDatas())
			{
				QString name = it->getRigidWallName();
				QVector3D endPos = modelData->_frameInfo.getRigidWallEndPos(name);
				it->updatePos(endPos);
			}
		}
		// S5 门面直接替换：绑定已由上方 seekFrame 内部消化（displayCurrentFrame 同构路径），
		// 旧 setRendCurrentFrameData 注入步删除

		// 10. 刷新渲染数据
		mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameData->getModelRenderDataProxy());
		if (model_proxy != nullptr) { model_proxy->updateValue(); }

		postRender->updateAllModelOperate(UpdateVariableValue);
		postRender->updateAllModelOperate(UpdateMinMax);
		rebuildPostRend->update();

		// 11. 根据变量类型刷新节点/单元颜色
		QList<PostVariableInfo> varInfos = modelData->_frameInfo._frameVars;
		for (const auto& it : varInfos)
		{
			if (it._varName == cloudData->_cloudVarName)
			{
				if (it._varType == 0)
					postRender->updateAllModelOperate(ShowNodeColor);
				else if (it._varType == 1)
					postRender->updateAllModelOperate(ShowElementColor);
				break;
			}
		}

		// 12. 更新颜色表、Min/Max、Foot 文字
		postRender->getColorTableWindow()->setVisiable(true);

		//标量运算视图继承源云图视口的图例最大最小值显隐状态
		if (cloudPostRend != rebuildPostRend && cloudPostRend->getPostRender() != nullptr)
		{
			auto* srcCT = cloudPostRend->getPostRender()->getColorTableWindow();
			auto* dstCT = postRender->getColorTableWindow();
			dstCT->showOrHidleMaxMinFoot({ 1,  srcCT->isMaxFootVisible() });
			dstCT->showOrHidleMaxMinFoot({ -1, srcCT->isMinFootVisible() });
		}
		std::set<int> hideParts = getCVM->getModelPartVisualData(rebuildPostRend->getViewportName());
		auto id_all = query_proxy->getAllPartID();
		if (auto* stateEditor = query_proxy->partStateEditor())
		{
			stateEditor->setVisibleParts(id_all);
			stateEditor->setInvisibleParts(hideParts);
		}
		getCVM->updateAllPostView();
		for (auto id : hideParts)
		{
			modelData->setMeshPartVisual(rebuildPostRend->getViewportName(), id, false);
		}

		float minval = FLT_MAX;
		float maxval = -FLT_MAX;
		int rangeIndex = cloudData->_rangeIndex;
		if (rangeIndex == 0)
		{
			maxval = query_proxy->getOriginalMaxData();
			minval = query_proxy->getOriginalMinData();
			query_proxy->setCurrentMaxData(maxval);
			query_proxy->setCurrentMinData(minval);
			{   // 现状首笔窗写（原行含 S5 备注则随段注保留）
			    // 色表值域收编（C4 双效点·窗写段，与本函数伴生 setMinMaxData 成对）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = -1;
			    _lg.valueMin = minval;
			    _lg.valueMax = maxval;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
		}
		else if (rangeIndex == 1)
		{
			auto cur_min_max = model_proxy->getPartMinMax();
			minval = cur_min_max.first;
			maxval = cur_min_max.second;
			query_proxy->setCurrentMinData(minval);
			query_proxy->setCurrentMaxData(maxval);
			{
			    // 色表值域收编（C4 双效点·默认值域段+窗写段一口两掩码，顺序按现状）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_DefaultRange
			        | mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = -1;
			    _lg.valueMin = minval;
			    _lg.valueMax = maxval;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			{   // 现状首笔窗写（原行含 S5 备注则随段注保留）
			    // 色表值域收编（C4 双效点·窗写段，与本函数伴生 setMinMaxData 成对）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = -1;
			    _lg.valueMin = minval;
			    _lg.valueMax = maxval;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
		}

		{
		    // 色表值域收编（C4 双效点·实例写段）：旧 setMinMaxData 原体（含窗写+刷新链）
		    mxr::ColorLegendParams _lg;
		    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
		    _lg.valueMin = minval;
		    _lg.valueMax = maxval;
		    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
		}
		postRender->updateAllModelOperate(ColorAllPartOperate);

		// 13. 设置标题和 Foot
		postRender->getColorTableWindow()->SetTitle(0, "MxSim");
		if (cloudData->_cloudLayerName.isEmpty())
		{
			postRender->getColorTableWindow()->SetTitle(1, "%s-%s",
				cloudData->_cloudVarName.toStdString().c_str(),
				cloudData->_cloudCompo.toStdString().c_str());
		}
		else
		{
			postRender->getColorTableWindow()->SetTitle(1, "%s-%s:%s",
				cloudData->_cloudVarName.toStdString().c_str(),
				cloudData->_cloudCompo.toStdString().c_str(),
				cloudData->_cloudLayerName.toStdString().c_str());
		}

		if (query_proxy != nullptr && model_proxy != nullptr)
		{
			mxr::PostNodeOrElement nodeOrElement = query_proxy->getNodeOrElement();
			std::pair<int, float> maxid = model_proxy->getOneTotalMaxID();
			std::pair<int, float> minid = model_proxy->getOneTotalMinID();
			if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
			{
				postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy->getCurrentMaxData());
				postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
				postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy->getCurrentMinData());
				postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
			}
			else
			{
				postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy->getCurrentMaxData());
				postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
				postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy->getCurrentMinData());
				postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
			}
		}

		// 14. 刷新显示
		setShowMode();
		rebuildPostRend->FitView();
		rebuildPostRend->updatePaint();
	}
}
