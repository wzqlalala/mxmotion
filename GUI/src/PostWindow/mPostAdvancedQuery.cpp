#include "mPostAdvancedQuery.h"

#include <QFormLayout>
#include <algorithm>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"

//MxFunctions
#include "mGlobalSignals.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"
#include "mPostResultDataStruct.h"
#include "mPostModelData.h"
#include "mMxdbThread.h"
#include "mIMxdbFile1.h"
// 须在 mIMxdbFile1.h 之后：mIONativeFile.h 的裸 include "mIOEnum.h" 靠 include 栈
// （mIMxdbFile1.h 所在的 PostIOFile 目录）命中 MIOFile 版枚举，先于本行会被 Common 版遮蔽
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"
#include "mPostInfoHub.h"
#include "mPost3DResult.h"
#include "mAppConfiguration.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/view/REBUILD_PostModelRender.h"
#include "mxr/view/REBUILD_PostOneFrameRender.h"
#include "mPostValueProvider.h"

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

namespace
{
	// 读数据失败的历史行为是静默（查询表格无数据无从排查），统一落输出窗口；
	// 文案口径与云图/动画 native 分支一致
	void reportQueryReadFailure(int frame, const QString& varstr, const QString& compostr,
		const QString& postPathName)
	{
		const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
		mGlobalSignals::getInstance()->outputMessageSig(2,
			isChinese ? QStringLiteral("高级查询读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
							.arg(frame).arg(varstr).arg(compostr).arg(postPathName)
					  : QStringLiteral("Advanced query failed to read result component: frame %1, variable %2, component %3 (%4); see the debug log for details")
							.arg(frame).arg(varstr).arg(compostr).arg(postPathName),
			true);
	}
}

namespace PostWindow
{
	mPostAdvancedQueryPanel::mPostAdvancedQueryPanel(QWidget* parent) : mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);
		this->setStyleSheet(
			"mPostAdvancedQueryPanel { background-color: white; }"

			"QComboBox QAbstractItemView {"
			"    background-color: white;"     /* 白底   */
			"    color: black;"                /* 黑字   */
			"    selection-background-color: #3399ff;" /* 悬停高亮，可自定 */
			"    selection-color: black;"      /* 悬停时文字保持黑色 */
			"}");

		FoldSection* defineFold = new FoldSection("定义", this);
		_comboApply = new QComboBox();
		_comboType = new QComboBox();
		_comboValType = new QComboBox();
		_lineEditValue = new QLineEdit();
		_checkThreshold = new QCheckBox("警告阈值");
		_spinThreshold = new QSpinBox();
		_comboRange = new QComboBox();
		_btnQuery = new QPushButton("查询");

		//应用于
		defineFold->addRow(new QLabel("应用于"), _comboApply, _comboType);
		QStringList list = {"所有", "显示"};
		_comboApply->addItems(list);

		//值
		defineFold->addRow(new QLabel("值"), _comboValType, _lineEditValue);
		QStringList type = { ">=", "<=" };
		_comboValType->addItems(type);
		_comboValType->setFixedWidth(100);
		QDoubleValidator* validator = new QDoubleValidator(this);
		validator->setRange(-1000000000.0, 1000000000.0);
		validator->setDecimals(10);
		_lineEditValue->setValidator(validator);

		//
		QLabel* label = new QLabel("%");
		defineFold->addRow(new QLabel(""), _checkThreshold, _spinThreshold, label);
		_checkThreshold->setFixedWidth(80);
		label->setFixedWidth(15);
		_spinThreshold->setRange(1, 99);
		_spinThreshold->setValue(85);
		_checkThreshold->setChecked(true);

		//范围
		defineFold->addRow(new QLabel("范围"), _comboRange);
		QStringList range = {"当前帧", "当前工况"};
		_comboRange->addItems(range);

		//查询
		defineFold->addRow(_btnQuery);
		connect(_btnQuery, SIGNAL(clicked()), this, SLOT(slot_query()));
		
		defineFold->refreshLayout();
		m_foldWidgets.append(defineFold);
		_mainLayout->addWidget(defineFold);
		_mainLayout->addStretch();

		//connect
		connect(_comboType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_applyTypeChange(const QString&)));
		connect(_comboValType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slot_valTypeChange(const QString&)));
	}

	void mPostAdvancedQueryPanel::updateAdvancedQueryPanel()
	{
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
		if (rebuildPostRend == nullptr) return;

		auto  postRender = rebuildPostRend->getPostRender();
		if (postRender == nullptr) return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
		if (guidata == nullptr) return;

		QString postname = rebuildPostRend->getPostDataName();

		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (modeldb == nullptr)
			return;

		QPointer<mPostModelData> modelData = modeldb->getModelData(guidata->_cloudFrame);
		if (modelData == nullptr)
			return;

		QStringList list{};
		QString varName = guidata->_cloudVarName;

		QList<PostData::PostVariableInfo> varInfos = modelData->_frameInfo._frameVars;
		for (auto it : varInfos)
		{
			if (it._varName == varName)
			{
				if (it._varType == 0 || it._varType == 2)
				{
					_isEle = false;
					list << "部件" << "节点";
				}
				else if (it._varType == 1)
				{
					_isEle = true;
					list << "部件" << "单元";
				}
				break;
			}
		}

		if (_curVar != varName)
		{
			_curVar = varName;
			_comboType->blockSignals(true);
			_comboType->clear();
			_comboType->addItems(list);
			_comboType->blockSignals(false);
			slot_applyTypeChange(_comboType->currentText());
		}
	}

	void mPostAdvancedQueryPanel::mergeHash(QHash<int, double>& hash, QHash<int, double>& hash1, QHash<int, double>& hash2)
	{
		auto it = hash1.begin();
		while (it != hash1.end())
		{
			hash.insert(it.key(), it.value());
			++it;
		}

		it = hash2.begin();
		while (it != hash2.end())
		{
			hash.insert(it.key(), it.value());
			++it;
		}
	}

	void mPostAdvancedQueryPanel::mergeHash(QHash<int, float>& hash, QHash<int, float>& hash1, QHash<int, float>& hash2)
	{
		auto it = hash1.begin();
		while (it != hash1.end())
		{
			hash.insert(it.key(), it.value());
			++it;
		}

		it = hash2.begin();
		while (it != hash2.end())
		{
			hash.insert(it.key(), it.value());
			++it;
		}
	}

	void mPostAdvancedQueryPanel::mergeHash(QHash<int, double>& hash, QHash<int, QHash<int, double>>& hash1, QHash<int, QHash<int, double>>& hash2)
	{
		auto it = hash1.begin();
		while (it != hash1.end())
		{
			double averVal = 0.0;
			QList<double> lis = it.value().values();

			for (auto val : lis)
			{
				averVal += val;
			}

			averVal /= lis.size();

			hash.insert(it.key(), averVal);
			++it;
		}

		it = hash2.begin();
		while (it != hash2.end())
		{
			double averVal = 0.0;
			QList<double> lis = it.value().values();

			for (auto val : lis)
			{
				averVal += val;
			}

			averVal /= lis.size();

			hash.insert(it.key(), averVal);
			++it;
		}
	}

	void mPostAdvancedQueryPanel::mergeHash(QHash<int, float>& hash, QHash<int, QHash<int, float>>& hash1, QHash<int, QHash<int, float>>& hash2)
	{
		auto it = hash1.begin();
		while (it != hash1.end())
		{
			float averVal = 0.0;
			QList<float> lis = it.value().values();

			for (auto val : lis)
			{
				averVal += val;
			}

			averVal /= lis.size();

			hash.insert(it.key(), averVal);
			++it;
		}

		it = hash2.begin();
		while (it != hash2.end())
		{
			float averVal = 0.0;
			QList<float> lis = it.value().values();

			for (auto val : lis)
			{
				averVal += val;
			}

			averVal /= lis.size();

			hash.insert(it.key(), averVal);
			++it;
		}
	}

	void mPostAdvancedQueryPanel::slot_valTypeChange(const QString& text)
	{
		if (text == "Top N" || text == "Bottom N")
		{
			_checkThreshold->setDisabled(true);
			QIntValidator* validator = new QIntValidator();
			_lineEditValue->setValidator(validator);
		}
		else
		{
			_checkThreshold->setDisabled(false);
			QDoubleValidator* validator = new QDoubleValidator();
			validator->setDecimals(10);
			_lineEditValue->setValidator(validator);
		}
	}

	void mPostAdvancedQueryPanel::slot_query()
	{
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
		if (rebuildPostRend == nullptr) return;

		auto  postRender = rebuildPostRend->getPostRender();
		if (postRender == nullptr) return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
		if (guidata == nullptr) return;
		QString postname = rebuildPostRend->getPostDataName();

		//判断输入参数合法性
		int ret = isReasonable();
		if (!ret)
		{
			return;
		}

		mDockLayoutManager::getInstance()->activateDock("Table");

		mTableDataWindow::getInstance()->setUseCurve(false);
		_tableWidget = mTableDataWindow::getInstance()->getTableWidget();
		_tableWidget->clear();
		_tableWidget->setRowCount(0);
		//设置表头
		setTabelHeader();

		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (modeldb == nullptr)
			return;

		QString postPathName = modeldb->getFile();

		//获取帧
		int fi = guidata->_cloudFrame;
		//获取变量
		QString varstr = guidata->_cloudVarName;
		//获取分量
		QString compostr = guidata->_cloudCompo;
		//获取层
		QString layerstr = guidata->_cloudLayerName;

		//获取复数
		QString complex = guidata->_cloudComplex;

		clearFilterAndShowAllPart();

		if (_comboApply->currentText() == "所有")
		{
			if (_comboRange->currentText() == "当前帧")
			{

				Compones compone;
				if (!readQueryCompones(modeldb, postname, postPathName, fi, varstr, compostr, layerstr, complex, compone))
					return;

				QPointer<mPostModelData> modeData = modeldb->getModelData(fi);

				if (_comboType->currentText() == "部件" || _comboType->currentText() == "Part")
				{
					auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
					if (model_proxy == nullptr)
						return;
					QHash<int, std::set<int>> hash{};  //value 部件ID  value 最大值或最小值单元  最大值或最小值节点
					filterPart(hash, modeData, model_proxy, fi);

					generatePartTable(hash, modeData, model_proxy, guidata);
				}
				else
				{
					QHash<int, double> results{};

					filterNodeOrElement(results, compone);

					generateNodeOrElementTable(results, modeData, guidata, compone.frameID);

				}
			}
			else if (_comboRange->currentText() == "当前工况")
			{
				if (_comboType->currentText() == "部件")
				{

					QList<int> frames = modeldb->getFrameIds();
					auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
					if (model_proxy == nullptr)
						return;

					//获取当前帧的数据
					QPointer<mPostModelData> modeData = modeldb->getModelData(fi);
					QHash<int, std::set<int>> hash{};  //value 部件ID  value 最大值或最小值单元  最大值或最小值节点
					filterPart(hash, modeData, model_proxy, fi);

					generatePartTable(hash, modeData, model_proxy, guidata);

					for (auto f : frames)
					{
						if (f != fi)
						{
							MechCommon::mMechGlobalSignals::getInstance()->changedFrame(f);

							modeData = modeldb->getModelData(f);
							auto modelPproxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
							if (modelPproxy == nullptr)
								continue;
							guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
							QHash<int, std::set<int>> hash{};  //value 部件ID  value 最大值或最小值单元  最大值或最小值节点
							filterPart(hash, modeData, modelPproxy, f);

							generatePartTable(hash, modeData, modelPproxy, guidata);
						}
					}
					//恢复云图
					MechCommon::mMechGlobalSignals::getInstance()->changedFrame(fi);
				}
				else
				{
					QHash<int, Compones> hashs{};
					QList<int> fNo = modeldb->getFrameIds();
					for (auto f : fNo)
					{

						Compones compone;
						if (!readQueryCompones(modeldb, postname, postPathName, f, varstr, compostr, layerstr, complex, compone))
							return;
						hashs.insert(f, compone);
					}

					auto hashIt = hashs.begin();
					while (hashIt != hashs.end())
					{
						QHash<int, double> results{};

						filterNodeOrElement(results, hashIt.value());

						QPointer<mPostModelData> modeData = modeldb->getModelData(hashIt.key());

						generateNodeOrElementTable(results, modeData, guidata, hashIt.value().frameID);
						++hashIt;
					}
				}
			}
		}
		else
		{
			if (_comboRange->currentText() == "当前帧")
			{

				Compones compone;
				if (!readQueryCompones(modeldb, postname, postPathName, fi, varstr, compostr, layerstr, complex, compone))
					return;

				QPointer<mPostModelData> modeData = modeldb->getModelData(fi);

				if (_comboType->currentText() == "部件")
				{
					auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
					if (model_proxy == nullptr)
						return;
					QHash<int, std::set<int>> hash{};  //value 部件ID  value 最大值或最小值单元  最大值或最小值节点
					filterPart(hash, modeData, model_proxy, fi, false);

					generatePartTable(hash, modeData, model_proxy, guidata);

				}
				else
				{
					QHash<int, double> results{};

					filterNodeOrElement(results, compone);

					generateNodeOrElementTable(results, modeData, guidata, compone.frameID);

				}

			}
			else if (_comboRange->currentText() == "当前工况")
			{
				if (_comboType->currentText() == "部件")
				{

					QList<int> frames = modeldb->getFrameIds();
					auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
					if (model_proxy == nullptr)
						return;

					//获取当前帧的数据
					QPointer<mPostModelData> modeData = modeldb->getModelData(fi);
					QHash<int, std::set<int>> hash{};  //value 部件ID  value 最大值或最小值单元  最大值或最小值节点
					filterPart(hash, modeData, model_proxy, fi);

					generatePartTable(hash, modeData, model_proxy, guidata);

					for (auto f : frames)
					{
						if (f != fi)
						{
							MechCommon::mMechGlobalSignals::getInstance()->changedFrame(f);
							modeData = modeldb->getModelData(f);
							auto modelPproxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
							if (modelPproxy == nullptr)
								continue;
							guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
							QHash<int, std::set<int>> hash{};  //value 部件ID  value 最大值或最小值单元  最大值或最小值节点
							filterPart(hash, modeData, modelPproxy, f);

							generatePartTable(hash, modeData, modelPproxy, guidata);
						}
					}
					//恢复云图
					MechCommon::mMechGlobalSignals::getInstance()->changedFrame(fi);
				}
				else
				{
					QHash<int, Compones> hashs{};
					QList<int> fNo = modeldb->getFrameIds();
					for (auto f : fNo)
					{
						Compones compone;
						if (!readQueryCompones(modeldb, postname, postPathName, f, varstr, compostr, layerstr, complex, compone))
							return;
						hashs.insert(f, compone);
					}

					auto hashIt = hashs.begin();
					while (hashIt != hashs.end())
					{
						QHash<int, double> results{};

						filterNodeOrElement(results, hashIt.value());

						QPointer<mPostModelData> modeData = modeldb->getModelData(hashIt.key());

						generateNodeOrElementTable(results, modeData, guidata, hashIt.value().frameID);
						++hashIt;
					}
				}
			}
		}
	}

	void mPostAdvancedQueryPanel::slot_sortColumn(int col)
	{
		_tableWidget->setSortingEnabled(false);
		Qt::SortOrder order = _tableWidget->horizontalHeader()->sortIndicatorOrder();
		_tableWidget->setSortingEnabled(true);
		_tableWidget->sortByColumn(col, order);
	}

	void mPostAdvancedQueryPanel::setTabelHeader()
	{
		QStringList list{};
		
		//设置表格
		if (_comboType->currentText().trimmed() == "部件")
		{
			_tableWidget->setColumnCount(6);
			if (_isEle)
			{
				if (_comboValType->currentText() == ">=")
				{
					list << "工况" << "帧" << "部件ID" << "部件名" << "最大单元ID" << "最大值";
				}
				else
				{
					list << "工况" << "帧" << "部件ID" << "部件名" << "最小单元ID" << "最小值";
				}
			}
			else
			{
				if (_comboValType->currentText() == ">=")
				{
					list << "工况" << "帧" << "部件ID" << "部件名" << "最大节点ID" << "最大值";
				}
				else
				{
					list << "工况" << "帧" << "部件ID" << "部件名" << "最小节点ID" << "最小值";
				}
			}
		}
		else if (_comboType->currentText().trimmed() == "单元")
		{
			_tableWidget->setColumnCount(6);
			list << "工况" << "帧" << "单元ID" << "结构" << "部件ID" << "值";
		}
		else
		{
			_tableWidget->setColumnCount(5);
			list << "工况" << "帧" << "节点ID" << "部件ID" << "值";
		}

		_tableWidget->setHorizontalHeaderLabels(list);
		_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::QHeaderView::Stretch);
		connect(_tableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this, &mPostAdvancedQueryPanel::slot_sortColumn);
		_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_tableWidget->verticalHeader()->setVisible(false);
	}

	bool mPostAdvancedQueryPanel::isReasonable()
	{
		if (_lineEditValue->text().trimmed() == "")
		{
			mGlobalSignals::getInstance()->outputMessageSig("请输入值！");
			return false;
		}

		if (_comboValType->currentText() != "Top N" && _comboValType->currentText() != "Bottom N")
		{
			int val = _spinThreshold->value();
			if (val < 1 || val>99)
			{
				mGlobalSignals::getInstance()->outputMessageSig("无效百分比！");
				return false;
			}
		}
		else
		{
			int val = _lineEditValue->text().toInt();
			if (val < 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig("请输入有效值！");
				return false;
			}
		}
		return true;
	}

	void mPostAdvancedQueryPanel::clearFilterAndShowAllPart()
	{
        if (!getCVM) return;
		//清除过滤
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
		if (rebuildPostRend == nullptr) return;

		auto  postRender = rebuildPostRend->getPostRender();
		if (postRender == nullptr) return;

		auto dataProxy = postRender->frameProxy(postRender->currentFrame());
		if (dataProxy == nullptr)
			return;

		// 查询查询数据代理
		mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(dataProxy->getRendQueryDataProxy());
		query_proxy1->setFilterType(mxr::FilterType::NONE);

		// S5 GUI 迁移（引擎缺通道移交项，编译强制）：getOneFrameRender 下钻通道随门面
		// 直接替换消亡——实例级过滤等值清理（deleteSingleContour）暂无 G 通道等价物，
		// 视觉挂起待渲染批次补通道后恢复（05 实施记录“新发现调用点”章登记）

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
		if (guidata == nullptr) return;

		QString postName = rebuildPostRend->getPostDataName();
		//显示
		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (modeldb == nullptr)
			return;

		if (_comboRange->currentText() == QString("当前工况"))
		{
			QHash<int, QPointer<mPostModelData>> modelDatas = modeldb->getAllFrameData();
			for (auto it : modelDatas)
			{
				if (it == nullptr)
					continue;
				it->setAllMeshPartVisual(rebuildPostRend->getViewportName(), true);
			}
		}
		else
		{
			QPointer<mPostModelData> modelData = modeldb->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr)
				return;
			modelData->setAllMeshPartVisual(rebuildPostRend->getViewportName(), true);
		}
		//设置零件显隐状态
		getCVM->setPostAllPartVisual(rebuildPostRend->getViewportName(), true);
		getCVM->updateAllPostView();
	}

	void mPostAdvancedQueryPanel::filterPart(QHash<int, std::set<int>>& hash, QPointer<mPostModelData> modelData, mxr::IPostModelRenderDataProxy* model_proxy, int f, bool flag)
	{
		int warnVal = 100;
		if (_checkThreshold->isCheckable())
		{
			if (_checkThreshold->isChecked())
			{
				warnVal = _spinThreshold->value();
			}
		}
		double val = _lineEditValue->text().toDouble();
		if (_comboValType->currentText() == ">=")
		{
			val = val * (warnVal / 100.0);
			for (auto part : modelData->_partdata)
			{
				if (part == nullptr)
					continue;
				if (!flag && part->getPartVisual() == false)
					continue;

				double maxVal = model_proxy->getPartMinMax(part->getPartID()).second;
				if (qFuzzyCompare(val, maxVal))
				{
					hash.insert(part->getPartID(), model_proxy->getPartMaxID(part->getPartID()));
				}
				else if (maxVal > val)
				{
					hash.insert(part->getPartID(), model_proxy->getPartMaxID(part->getPartID()));
				}
			}
			filterPartModel(hash.keys(), f);
		}
		else if (_comboValType->currentText() == "<=")
		{
			val = val + val * (1 - warnVal / 100.0);
			for (auto part : modelData->_partdata)
			{
				if (part == nullptr)
					continue;
				if (!flag && part->getPartVisual() == false)
					continue;
				double maxVal = model_proxy->getPartMinMax(part->getPartID()).second;
				if (qFuzzyCompare(val, maxVal))
				{
					hash.insert(part->getPartID(), model_proxy->getPartMinID(part->getPartID()));
				}
				else if (maxVal < val)
				{
					hash.insert(part->getPartID(), model_proxy->getPartMinID(part->getPartID()));
				}
			}

			filterPartModel(hash.keys(), f);
		}
		else
		{

		}
	}

	void mPostAdvancedQueryPanel::filterPartModel(QList<int> ids, int f)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		QString postName = postRend->getPostDataName();

		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (modeldb == nullptr)
			return;

		QPointer<mPostModelData> modeData = modeldb->getModelData(f);
		if (modeData == nullptr)
			return;
		std::set<int> hidePartIds{};
		std::set<int> showPartIds{};
		QVector<int> partIds = modeData->getAllPartsIDs();
		for (auto id : partIds)
		{
			if (!ids.contains(id))
			{
				hidePartIds.insert(id);
			}
			else
			{
				showPartIds.insert(id);
			}
		}
		QString viewName = postRend->getViewportName();
		//设置零件显示
		getCVM->setPostSomePartVisual(viewName, showPartIds, true);

		//设置零件隐藏
		getCVM->setPostSomePartVisual(viewName, hidePartIds, false);

		postRend->updatePaint();
	}

	bool mPostAdvancedQueryPanel::readQueryCompones(QPointer<PostData::mPostDataBase> modeldb,
		const QString& postname, const QString& postPathName, int frame, const QString& varstr,
		const QString& compostr, const QString& layerstr, const QString& complex,
		PostData::Compones& compone)
	{
		Q_UNUSED(postname)
		//统一取值 provider（阶段2 试点）：四格式分支收敛为出参式单口（不污染云图态）——
		//native/VTK=发布后回取快照（原口径），mxdb 两系走 reader 池+出参重载；
		//发布协议（isNodeFace=false, isD=true, flag=结果类型, value1=值）与
		// filterNodeOrElement 的 else 分支消费口兼容性不变
		IPostValueProvider* provider = valueProviderFor(modeldb);
		if (provider == nullptr)
		{
			reportQueryReadFailure(frame, varstr, compostr, postPathName);
			return false;
		}
		ValueRequest req;
		req.frame = frame;
		req.var = varstr;
		req.compo = compostr;
		req.layer = layerstr;
		req.complexMode = complex;
		const bool ok = provider->fetch(req, compone);
		delete provider;
		if (!ok)
		{
			reportQueryReadFailure(frame, varstr, compostr, postPathName);
			return false;
		}
		return true;
	}

	void mPostAdvancedQueryPanel::filterNodeOrElement(QHash<int, double>& results, Compones& compone)	{
		int warnVal = 100;
		if (_checkThreshold->isCheckable())
		{
			if (_checkThreshold->isChecked())
			{
				warnVal = _spinThreshold->value();
			}
		}

		if (_comboValType->currentText() == ">=")
		{
			double val = _lineEditValue->text().toDouble();
			val = val * (warnVal / 100.0);
			//筛大于等于的值
			QHash<int, double> idValmapD{};
			QHash<int, float> idValmapF{};
			if (compone.isNodeFace)
			{
				if (compone.isD)
				{
					mergeHash(idValmapD, compone.dRValue, compone.dQValue);
					auto it = idValmapD.begin();
					while (it != idValmapD.end())
					{
						double v = it.value();
						if (qFuzzyCompare(val, v))
						{
							results.insert(it.key(), v);
						}
						else
						{
							if (v > val)
							{
								results.insert(it.key(), v);
							}
						}

						++it;
					}
				}
				else
				{
					mergeHash(idValmapF, compone.fRValue, compone.fQValue);
					auto it = idValmapF.begin();
					while (it != idValmapF.end())
					{
						double v = it.value();
						if (qFuzzyCompare(val, v))
						{
							results.insert(it.key(), v);
						}
						else
						{
							if (v > val)
							{
								results.insert(it.key(), v);
							}
						}

						++it;
					}
				}
			}
			else
			{
				if (compone.isD)
				{
					mergeHash(idValmapD, compone.value1, compone.value2);
					auto it = idValmapD.begin();
					while (it != idValmapD.end())
					{
						double v = it.value();
						if (qFuzzyCompare(val, v))
						{
							results.insert(it.key(), v);
						}
						else
						{
							if (v > val)
							{
								results.insert(it.key(), v);
							}
						}

						++it;
					}
				}
				else
				{
					mergeHash(idValmapF, compone.value3, compone.value4);
					auto it = idValmapF.begin();
					while (it != idValmapF.end())
					{
						double v = it.value();
						if (qFuzzyCompare(val, v))
						{
							results.insert(it.key(), v);
						}
						else
						{
							if (v > val)
							{
								results.insert(it.key(), v);
							}
						}

						++it;
					}
				}
			}
			filterModel(val, 0);
		}
		else if (_comboValType->currentText() == "<=")
		{
			double val = _lineEditValue->text().toDouble();
			val = val + val * (1 - warnVal / 100.0);
			//筛小于等于的值
			QHash<int, double> idValmapD{};
			QHash<int, float> idValmapF{};
			if (compone.isNodeFace)
			{
				if (compone.isD)
				{
					mergeHash(idValmapD, compone.dRValue, compone.dQValue);
					auto it = idValmapD.begin();
					while (it != idValmapD.end())
					{
						double v = it.value();
						if (qFuzzyCompare(val, v))
						{
							results.insert(it.key(), v);
						}
						else
						{
							if (v < val)
							{
								results.insert(it.key(), v);
							}
						}

						++it;
					}
				}
				else
				{
					mergeHash(idValmapF, compone.fRValue, compone.fQValue);
					auto it = idValmapF.begin();
					while (it != idValmapF.end())
					{
						double v = it.value();
						if (qFuzzyCompare(val, v))
						{
							results.insert(it.key(), v);
						}
						else
						{
							if (v < val)
							{
								results.insert(it.key(), v);
							}
						}

						++it;
					}
				}
			}
			else
			{
				if (compone.isD)
				{
					mergeHash(idValmapD, compone.value1, compone.value2);
					auto it = idValmapD.begin();
					while (it != idValmapD.end())
					{
						double v = it.value();
						if (qFuzzyCompare(val, v))
						{
							results.insert(it.key(), v);
						}
						else
						{
							if (v < val)
							{
								results.insert(it.key(), v);
							}
						}

						++it;
					}
				}
				else
				{
					mergeHash(idValmapF, compone.value3, compone.value4);
					auto it = idValmapF.begin();
					while (it != idValmapF.end())
					{
						double v = it.value();
						if (qFuzzyCompare(val, v))
						{
							results.insert(it.key(), v);
						}
						else
						{
							if (v < val)
							{
								results.insert(it.key(), v);
							}
						}

						++it;
					}
				}
			}
			filterModel(val, 1);
		}
		else if (_comboValType->currentText() == "Top N")
		{
			int num = _lineEditValue->text().toInt();

			//值最大的num节点
			QHash<int, double> idValmapD{};
			QHash<int, float> idValmapF{};
			double val;
			if (compone.isNodeFace)
			{
				if (compone.isD)
				{
					mergeHash(idValmapD, compone.dRValue, compone.dQValue);
					//排序
					std::multimap<double, int> temp{};
					auto it = idValmapD.begin();
					while (it != idValmapD.end())
					{
						temp.insert({ it.value(), it.key() });
						++it;
					}

					std::set<double> tempKeys{};
					auto mapIt = temp.begin();
					while (mapIt != temp.end())
					{
						tempKeys.insert(mapIt->first);
						++mapIt;
					}

					int n = 0;
					auto keyIt = tempKeys.rbegin();
					while (keyIt != tempKeys.rend())
					{
						if (n == num)
							break;
						val = *keyIt;
						auto range = temp.equal_range(val);
						for (auto it = range.first; it != range.second; ++it)
						{
							results.insert(it->second, it->first);
						}
						++n;
						++keyIt;
					}
				}
				else
				{
					mergeHash(idValmapF, compone.fRValue, compone.fQValue);
					//排序
					std::multimap<double, int> temp{};
					auto it = idValmapF.begin();
					while (it != idValmapF.end())
					{
						temp.insert({ it.value(), it.key() });
						++it;
					}

					std::set<double> tempKeys{};
					auto mapIt = temp.begin();
					while (mapIt != temp.end())
					{
						tempKeys.insert(mapIt->first);
						++mapIt;
					}

					int n = 0;
					auto keyIt = tempKeys.rbegin();
					while (keyIt != tempKeys.rend())
					{
						if (n == num)
							break;
						val = *keyIt;
						auto range = temp.equal_range(val);
						for (auto it = range.first; it != range.second; ++it)
						{
							results.insert(it->second, it->first);
						}
						++n;
						++keyIt;
					}
				}
			}
			else
			{
				if (compone.isD)
				{
					mergeHash(idValmapD, compone.value1, compone.value2);
					//排序
					std::multimap<double, int> temp{};
					auto it = idValmapD.begin();
					while (it != idValmapD.end())
					{
						temp.insert({ it.value(), it.key() });
						++it;
					}

					std::set<double> tempKeys{};
					auto mapIt = temp.begin();
					while (mapIt != temp.end())
					{
						tempKeys.insert(mapIt->first);
						++mapIt;
					}

					int n = 0;
					auto keyIt = tempKeys.rbegin();
					while (keyIt != tempKeys.rend())
					{
						if (n == num)
							break;
						val = *keyIt;
						auto range = temp.equal_range(val);
						for (auto it = range.first; it != range.second; ++it)
						{
							results.insert(it->second, it->first);
						}
						++n;
						++keyIt;
					}
				}
				else
				{
					mergeHash(idValmapF, compone.value3, compone.value4);
					//排序
					std::multimap<double, int> temp{};
					auto it = idValmapF.begin();
					while (it != idValmapF.end())
					{
						temp.insert({ it.value(), it.key() });
						++it;
					}

					std::set<double> tempKeys{};
					auto mapIt = temp.begin();
					while (mapIt != temp.end())
					{
						tempKeys.insert(mapIt->first);
						++mapIt;
					}

					int n = 0;
					auto keyIt = tempKeys.rbegin();
					while (keyIt != tempKeys.rend())
					{
						if (n == num)
							break;
						val = *keyIt;
						auto range = temp.equal_range(val);
						for (auto it = range.first; it != range.second; ++it)
						{
							results.insert(it->second, it->first);
						}
						++n;
						++keyIt;
					}
				}
			}
			filterModel(val, 0);
		}
		else
		{
			int num = _lineEditValue->text().toInt();

			//值最大的num节点
			QHash<int, double> idValmapD{};
			QHash<int, float> idValmapF{};
			double val;
			if (compone.isD)
			{
				mergeHash(idValmapD, compone.value1, compone.value2);
				//排序
				std::multimap<double, int> temp{};
				auto it = idValmapD.begin();
				while (it != idValmapD.end())
				{
					temp.insert({ it.value(), it.key() });
					++it;
				}

				std::set<double> tempKeys{};
				auto mapIt = temp.begin();
				while (mapIt != temp.end())
				{
					tempKeys.insert(mapIt->first);
					++mapIt;
				}

				int n = 0;
				auto keyIt = tempKeys.begin();
				while (keyIt != tempKeys.end())
				{
					if (n == num)
						break;
					val = *keyIt;
					auto range = temp.equal_range(val);
					for (auto it = range.first; it != range.second; ++it)
					{
						results.insert(it->second, it->first);
					}
					++n;
					++keyIt;
				}
			}
			else
			{
				mergeHash(idValmapF, compone.value3, compone.value4);
				//排序
				std::multimap<double, int> temp{};
				auto it = idValmapF.begin();
				while (it != idValmapF.end())
				{
					temp.insert({ it.value(), it.key() });
					++it;
				}

				std::set<double> tempKeys{};
				auto mapIt = temp.begin();
				while (mapIt != temp.end())
				{
					tempKeys.insert(mapIt->first);
					++mapIt;
				}

				int n = 0;
				auto keyIt = tempKeys.begin();
				while (keyIt != tempKeys.end())
				{
					if (n == num)
						break;
					val = *keyIt;
					auto range = temp.equal_range(val);
					for (auto it = range.first; it != range.second; ++it)
					{
						results.insert(it->second, it->first);
					}
					++n;
					++keyIt;
				}
			}
			filterModel(val, 1);
		}
	}

	void mPostAdvancedQueryPanel::generatePartTable(QHash<int, std::set<int>>& hash, QPointer<mPostModelData> modelData, mxr::IPostModelRenderDataProxy* model_proxy, mPostGUIData* guidata)
	{
		_tableWidget->blockSignals(true);
		int flag = 0;
		if (_comboValType->currentText() == ">=")
		{
			flag = 0;
		}
		else
		{
			flag = 1;
		}
		double val = _lineEditValue->text().toDouble();
		double mVal;

		auto hashIt = hash.begin();
		while (hashIt != hash.end())
		{
			if (flag == 0)
			{
				mVal = model_proxy->getPartMinMax(hashIt.key()).second;
			}
			else
			{
				mVal = model_proxy->getPartMinMax(hashIt.key()).first;
			}

			for (auto id : hashIt.value())
			{
				QStringList itemList{};
				itemList << guidata->_cloudCase;

				itemList << MxFunctions::mNumberToString(guidata->_cloudFrame);

				itemList << MxFunctions::mNumberToString(hashIt.key());

				QString partName = modelData->getPartNameByPartId(hashIt.key());
				itemList << partName;

				itemList << MxFunctions::mNumberToString(id);

				QColor color(Qt::black);
				if (flag == 0)
				{
					if (val > mVal)
					{
						color = QColor(Qt::red);
					}

				}
				else
				{
					if (val < mVal)
					{
						color = QColor(Qt::red);
					}
				}
				itemList << MxFunctions::mNumberToString(mVal, 'e');

				addRow(_tableWidget, itemList, color);
			}
			++hashIt;
		}
		_tableWidget->blockSignals(false);
	}

	void mPostAdvancedQueryPanel::generateNodeOrElementTable(QHash<int, double>& results, QPointer<mPostModelData> modelData, mPostGUIData* guidata, int f, bool flag)
	{
		_tableWidget->blockSignals(true);

		int flags = -1;
		if (_comboValType->currentText() == ">=")
		{
			flags = 0;
		}
		else if (_comboValType->currentText() == "<=")
		{
			flags = 1;
		}
		double val = _lineEditValue->text().toDouble();
		QHash<int, std::set<int>> nodeIdPartIdmap{};

		//去重
		if (_comboType->currentText() == "节点")
		{
			//统计结点所在部件情况（去重）
			auto it = results.begin();
			while (it != results.end())
			{
				std::set<int> temp{};
				for (auto part : modelData->_partdata)
				{
					if (part == nullptr)
						continue;
					int count = part->getNodeIdSet()->count(it.key());
					if (count > 0)
					{
						temp.insert(part->getPartID());
					}

				}
				nodeIdPartIdmap.insert(it.key(), temp);
				++it;
			}

			//判断是否去除隐藏的节点
			if (flag)
			{
				//不去除隐藏的节点
				auto nodeIt = nodeIdPartIdmap.begin();
				while (nodeIt != nodeIdPartIdmap.end())
				{
					QStringList itemList{};
					itemList << guidata->_cloudCase;

					itemList << MxFunctions::mNumberToString(f);
					int id = modelData->_nodedata.ids[nodeIt.key()];
					itemList << MxFunctions::mNumberToString(id);

					QString str = "";
					int k = 1;
					for (auto id : nodeIt.value())
					{
						int val = id;
						if (k == nodeIt.value().size())
						{
							str += MxFunctions::mNumberToString(val);
						}
						else
						{
							str += MxFunctions::mNumberToString(val) + " ";
						}
						++k;
					}
					itemList << str;

					QColor color(Qt::black);
					if (flags == 0)
					{
						if (val > results.value(nodeIt.key()))
						{
							color = QColor(Qt::red);
						}

					}
					else if (flags == 1)
					{
						if (val < results.value(nodeIt.key()))
						{
							color = QColor(Qt::red);
						}
					}

					itemList << MxFunctions::mNumberToString(results.value(nodeIt.key()), 'e');

					addRow(_tableWidget, itemList, color);
					++nodeIt;
				}
			}
			else
			{
				//去除隐藏的节点
				std::set<int> ids = modelData->getAllNodeIDs();
				auto nodeIt = ids.begin();
				while (nodeIt != ids.end())
				{
					QStringList itemList{};
					itemList << guidata->_cloudCase;

					itemList << MxFunctions::mNumberToString(f);
					int id = modelData->_nodedata.ids[*nodeIt];
					itemList << MxFunctions::mNumberToString(id);

					QString str = "";
					int k = 1;
					for (auto id : nodeIdPartIdmap.value(*nodeIt))
					{
						int val = id;
						if (k == nodeIdPartIdmap.value(*nodeIt).size())
						{
							str += MxFunctions::mNumberToString(val);
						}
						else
						{
							str += MxFunctions::mNumberToString(val) + " ";
						}
						++k;
					}
					itemList << str;

					QColor color(Qt::black);
					if (flags == 0)
					{
						if (val > results.value(*nodeIt))
						{
							color = QColor(Qt::red);
						}

					}
					else if (flags == 1)
					{
						if (val < results.value(*nodeIt))
						{
							color = QColor(Qt::red);
						}
					}
					itemList << MxFunctions::mNumberToString(results.value(*nodeIt), 'e');

					addRow(_tableWidget, itemList, color);
					++nodeIt;
				}
			}
		}
		else
		{
			if (_comboType->currentText() == "节点")
			{
				//统计结点所在部件情况
				auto it = results.begin();
				while (it != results.end())
				{
					std::set<int> temp{};
					for (auto part : modelData->_partdata)
					{
						if (part == nullptr)
							continue;
						int count = part->getNodeIdSet()->count(it.key());
						if (count > 0)
						{
							temp.insert(part->getPartID());
						}
					}
					nodeIdPartIdmap.insert(it.key(), temp);
					++it;
				}
			}
			else
			{
				//统计结点所在部件情况
				auto it = results.begin();
				while (it != results.end())
				{
					std::set<int> temp{};
					for (auto part : modelData->_partdata)
					{
						if (part == nullptr)
							continue;
						if (part->_parteledata.cell_rid_to_cid.find(it.key()) != part->_parteledata.cell_rid_to_cid.cend())
						{
							temp.insert(part->getPartID());
							break;
						}
					}
					nodeIdPartIdmap.insert(it.key(), temp);
					++it;
				}
			}
			int row = nodeIdPartIdmap.size();

			if (_comboType->currentText() == "节点")
			{
				//判断是否去除隐藏的节点
				if (flag)
				{
					//不去隐藏的节点
					auto nodeIt = nodeIdPartIdmap.begin();
					while (nodeIt != nodeIdPartIdmap.end())
					{
						for (auto id : nodeIt.value())
						{
							QStringList itemList{};
							itemList << guidata->_cloudCase;

							itemList << MxFunctions::mNumberToString(f);

							int rid = modelData->_nodedata.ids[nodeIt.key()];
							itemList << MxFunctions::mNumberToString(rid);

							itemList << MxFunctions::mNumberToString(id);

							QColor color(Qt::black);
							if (flags == 0)
							{
								if (val > results.value(nodeIt.key()))
								{
									color = QColor(Qt::red);
								}
							}
							else if (flags == 1)
							{
								if (val < results.value(nodeIt.key()))
								{
									color = QColor(Qt::red);
								}
							}
							itemList << MxFunctions::mNumberToString(results.value(nodeIt.key()), 'e');

							addRow(_tableWidget, itemList, color);
						}
						++nodeIt;
					}
				}
				else
				{
					//去隐藏的节点
					std::set<int> ids = modelData->getAllNodeIDs();
					auto nodeIt = ids.begin();
					while (nodeIt != ids.end())
					{
						for (auto id : nodeIdPartIdmap.value(*nodeIt))
						{
							QStringList itemList{};
							itemList << guidata->_cloudCase;

							itemList << MxFunctions::mNumberToString(f);
							int rid = modelData->_nodedata.ids[*nodeIt];
							itemList << MxFunctions::mNumberToString(rid);

							itemList << MxFunctions::mNumberToString(id);

							QColor color(Qt::black);
							if (flags == 0)
							{
								if (val > results.value(*nodeIt))
								{
									color = QColor(Qt::red);
								}

							}
							else if (flags == 1)
							{
								if (val < results.value(*nodeIt))
								{
									color = QColor(Qt::red);
								}
							}
							itemList << MxFunctions::mNumberToString(results.value(*nodeIt), 'e');

							addRow(_tableWidget, itemList, color);
						}
						++nodeIt;
					}
				}
			}
			else
			{
				//判断是否去除隐藏的单元
				if (flag)
				{
					//不去隐藏的单元
					auto eleIt = nodeIdPartIdmap.begin();
					while (eleIt != nodeIdPartIdmap.end())
					{
						QStringList itemList{};
						itemList << guidata->_cloudCase;

						itemList << MxFunctions::mNumberToString(f);

						itemList << MxFunctions::mNumberToString(eleIt.key());

						int typeInt = modelData->getEleType(*eleIt.value().begin(), eleIt.key());
						itemList << MxFunctions::mNumberToString(typeInt);

						itemList << MxFunctions::mNumberToString(*eleIt.value().begin());

						QColor color(Qt::black);
						if (flags == 0)
						{
							if (val > results.value(eleIt.key()))
							{
								color = QColor(Qt::red);
							}

						}
						else if (flags == 1)
						{
							if (val < results.value(eleIt.key()))
							{
								color = QColor(Qt::red);
							}
						}

						itemList << MxFunctions::mNumberToString(results.value(eleIt.key()), 'e');

						addRow(_tableWidget, itemList, color);
						++eleIt;
					}
				}
				else
				{
					//去隐藏的单元
					QList<int> ids = modelData->getAllMeshIDs();
					auto eleIt = ids.begin();
					while (eleIt != ids.end())
					{
						QStringList itemList{};
						itemList << guidata->_cloudCase;

						itemList << MxFunctions::mNumberToString(f);

						itemList << MxFunctions::mNumberToString(*eleIt);

						int typeInt = modelData->getEleType(*(nodeIdPartIdmap.value(*eleIt).begin()), *eleIt);
						itemList << MxFunctions::mNumberToString(typeInt);

						itemList << MxFunctions::mNumberToString(*(nodeIdPartIdmap.value(*eleIt).begin()));

						QColor color(Qt::black);
						if (flags == 0)
						{
							if (val > results.value(*eleIt))
							{
								color = QColor(Qt::red);
							}
						}
						else if (flags == 1)
						{
							if (val < results.value(*eleIt))
							{
								color = QColor(Qt::red);
							}
						}

						itemList << MxFunctions::mNumberToString(results.value(*eleIt), 'e');

						addRow(_tableWidget, itemList, color);
						++eleIt;
					}
				}
			}
		}
		_tableWidget->blockSignals(false);

	}

	void mPostAdvancedQueryPanel::filterModel(double value, int type)
	{
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
		if (rebuildPostRend == nullptr) return;

		auto  postRender = rebuildPostRend->getPostRender();
		if (postRender == nullptr) return;

		// S5 GUI 迁移（引擎缺通道移交项，编译强制）：实例级过滤等值渲染（createSingle
		// Contour）暂无 G 通道等价物——视觉挂起待渲染批次补通道后恢复；数据侧过滤
		// 类型经 frameProxy 查询代理保留（05 实施记录登记）

		auto dataProxy = postRender->frameProxy(postRender->currentFrame());
		if (dataProxy == nullptr)
			return;

		// 查询查询数据代理
		mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(dataProxy->getRendQueryDataProxy());
		if (type == 0)
		{
			query_proxy1->setFilterType(mxr::FilterType::ABOVE);
		}
		else
		{
			query_proxy1->setFilterType(mxr::FilterType::BELOW);
		}
		rebuildPostRend->updatePaint();
	}

	void mPostAdvancedQueryPanel::addRow(QTableWidget* table, const QStringList& rowData, QColor color)
	{
		int row = table->rowCount();
		table->insertRow(row);
		for (int col = 0; col < rowData.size(); ++col) 
		{
			QTableWidgetItem* item = new QTableWidgetItem(rowData[col]);
			if (col == rowData.size() -1)
			{
				item->setForeground(QBrush(color));
			}
			table->setItem(row, col, item);
		}
	}

	void mPostAdvancedQueryPanel::slot_applyTypeChange(const QString& text)
	{
		QStringList list{};
		if (text == "部件")
		{
			list << ">=" << "<=";
		}
		else if (text == "单元")
		{
			list << ">=" << "<=" << "Top N" << "Bottom N";
		}
		else
		{
			list << ">=" << "<=" << "Top N" << "Bottom N";
		}
		_comboValType->blockSignals(true);
		_comboValType->clear();
		_comboValType->addItems(list);
		_comboValType->blockSignals(false);
	}

	void mPostAdvancedQueryPanel::hideEvent(QHideEvent* event)
	{
		clearFilterAndShowAllPart();
	}
}
