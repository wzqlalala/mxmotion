#include "mPostResultPath.h"
#include "PosPickWidget1.h"	//PosPickWidget1已析出GUIFoundation

#include <QFormLayout>
#include <QGraphicsDropShadowEffect>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>
#include <QDateTime>
#include <QElapsedTimer>
#include <set>
#include <algorithm>
#include <functional>

#include "mCommonFunctions.h"
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
#include "mPostModelData.h"
#include "mPostGUIData.h"
#include "PickCommonFunction.h"
#include "ZsTempRender.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"

//窗口
#include "mModelData.h"
#include "mPostPlotFloatWindow.h"
#include "mPlotView.h"
#include "mPostDataTable.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;
using namespace PlotView;

namespace PostWindow
{
	// 构造函数：初始化界面并建立信号槽连接
	mPostResultPath::mPostResultPath(QWidget* parent) : mPanelBase(parent)
	{
		setupUI();
		setupConnections();
	}

	// 设置界面：创建结果路径折叠面板，内部放置起始点、终止点拾取面板和按钮
	void mPostResultPath::setupUI()
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);

		// 创建“结果路径”折叠分区
		_numandpos = new FoldSection("结果路径", this);
		_pointContainer = new QWidget();
		_pointContainer2 = new QWidget();

		// 起始点面板横向布局
		QHBoxLayout* layout_point = new QHBoxLayout(_pointContainer);
		layout_point->setContentsMargins(0, 0, 0, 0);
		layout_point->setSpacing(10);

		// 初始化起始点拾取面板，并隐藏默认的“添加”按钮
		_startPointPanel.initWidget("起始点");
		_startPointPanel.setState(false);
		_startPointPanel.hideAddBtn(true);
		layout_point->addWidget(_startPointPanel.propwid);

		// 终止点面板横向布局
		QHBoxLayout* layout_point2 = new QHBoxLayout(_pointContainer2);
		layout_point2->setContentsMargins(0, 0, 0, 0);
		layout_point2->setSpacing(10);

		// 初始化终止点拾取面板，并隐藏默认的“添加”按钮
		_endPointPanel.initWidget("终止点");
		_endPointPanel.setState(false);
		_endPointPanel.hideAddBtn(true);
		layout_point2->addWidget(_endPointPanel.propwid);

		// 路径结果按钮：用于求解两点间拓扑最短路径
		_btnPathResult = new QPushButton("路径结果");
		// 曲线生成按钮：沿当前选中路径的节点序列取样生成曲线（FR-09）
		_btnCurveGeneration = new QPushButton("曲线生成");

		// 汇总行：最短路径条数统计（未计算/共 N 条/超上限提示，02 §3.1）
		_summaryLabel = new QLabel("最短路径（尚未计算）");

		// 最短路径列表：序号|节点数|路径长度，单选、不可编辑；无条目时禁用（FR-03）
		_pathList = new QTableWidget(0, 3);
		_pathList->setHorizontalHeaderLabels(QStringList() << QString("序号") << QString("节点数") << QString("路径长度"));
		_pathList->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_pathList->setSelectionBehavior(QAbstractItemView::SelectRows);
		_pathList->setSelectionMode(QAbstractItemView::SingleSelection);
		_pathList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
		_pathList->verticalHeader()->setVisible(false);
		_pathList->horizontalHeader()->setStretchLastSection(true);
		_pathList->setMinimumHeight(100);
		_pathList->setMaximumHeight(220);
		_pathList->setEnabled(false);

		// 将控件加入折叠分区（汇总行与列表插在“路径结果”与“曲线生成”之间，02 §3.1）
		_numandpos->addRow(_pointContainer);
		_numandpos->addRow(_pointContainer2);
		_numandpos->addRow(_btnPathResult);
		_numandpos->addRow(_summaryLabel);
		_numandpos->addRow(_pathList);
		_numandpos->addRow(_btnCurveGeneration);

		m_foldWidgets.append(_numandpos);
		_mainLayout->addWidget(_numandpos);
		_mainLayout->addStretch();
	}

	// 建立信号槽连接：按钮点击事件与对应处理函数绑定
	void mPostResultPath::setupConnections()
	{
		connect(_startPointPanel.btnok, &QPushButton::clicked, this, &mPostResultPath::slot_pickStartPoint);
		connect(_startPointPanel.btnclear, &QPushButton::clicked, this, &mPostResultPath::slot_clearStartPoint);
		connect(_endPointPanel.btnok, &QPushButton::clicked, this, &mPostResultPath::slot_pickEndPoint);
		connect(_endPointPanel.btnclear, &QPushButton::clicked, this, &mPostResultPath::slot_clearEndPoint);
		connect(_btnPathResult, &QPushButton::clicked, this, &mPostResultPath::slot_generatePath);
		connect(_btnCurveGeneration, &QPushButton::clicked, this, &mPostResultPath::slot_generateNodeCurve);
		// 列表条目点击 → 切换当前高亮路径（FR-05）；cellClicked 保证重复点击同一行也响应
		connect(_pathList, &QTableWidget::cellClicked, this, &mPostResultPath::slot_pathItemSelected);
	}

	// 新建路径结果面板：清理旧状态并获取当前后处理视口及数据库
	void mPostResultPath::newPathResult(int caseid)
	{
		// 清除临时线条和高亮
		clearResult();
		// 清除起始点和终止点的拾取状态
		clearPointPick(true);
		clearPointPick(false);

		if (!getCVM)
			return;

		// 获取当前后处理渲染窗口
		_rWidget = getCVM->getPostRenderWidget();
		if (_rWidget == nullptr)
			return;

		// 根据 caseid 获取对应的后处理数据库
		_db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);

		// FR-08/R7：帧与变形系数变化 → 路径显示刷新（不重算）；挂接方式仿测量面板
		//（mPostMeasureTool.cpp:1331-1350，deformation 用 QueuedConnection 防渲染内联嵌套）
		std::shared_ptr<mxr::REBUILD_PostRender> postRender = _rWidget->getPostRender();
		if (postRender != nullptr)
		{
			if (_frameConn)
				disconnect(_frameConn);
			_frameConn = connect(postRender.get(), &mxr::REBUILD_PostRender::frameChanged,
				this, &mPostResultPath::slot_postFrameChanged);

			if (_deformConn)
				disconnect(_deformConn);
			_deformConn = connect(postRender.get(), &mxr::REBUILD_PostRender::deformationChanged,
				this, &mPostResultPath::slot_postDeformChanged, Qt::QueuedConnection);
		}
	}

	// 面板隐藏时：清理临时线条、高亮及拾取状态
	void mPostResultPath::hideEvent(QHideEvent* event)
	{
		clearResult();
		clearPointPick(true);
		clearPointPick(false);
		mPanelBase::hideEvent(event);
	}

	// 面板显示时：补挂帧/变形信号连接（QY-06：hide 后经 dock 布局恢复显示不经
	// newPathResult 入口，FR-08 帧跟随须在该路径下生效）；逐连接幂等，已挂不重复
	void mPostResultPath::showEvent(QShowEvent* event)
	{
		mPanelBase::showEvent(event);

		if (_rWidget == nullptr)
			return;

		std::shared_ptr<mxr::REBUILD_PostRender> postRender = _rWidget->getPostRender();
		if (postRender == nullptr)
			return;

		if (!_frameConn)
		{
			_frameConn = connect(postRender.get(), &mxr::REBUILD_PostRender::frameChanged,
				this, &mPostResultPath::slot_postFrameChanged);
		}
		if (!_deformConn)
		{
			_deformConn = connect(postRender.get(), &mxr::REBUILD_PostRender::deformationChanged,
				this, &mPostResultPath::slot_postDeformChanged, Qt::QueuedConnection);
		}
	}

	// 槽函数：点击“起始点”拾取按钮
	void mPostResultPath::slot_pickStartPoint()
	{
		startPointPick(true);
	}

	// 槽函数：点击“终止点”拾取按钮
	void mPostResultPath::slot_pickEndPoint()
	{
		startPointPick(false);
	}

	// 槽函数：点击“起始点”清除按钮
	void mPostResultPath::slot_clearStartPoint()
	{
		clearPointPick(true);
	}

	// 槽函数：点击“终止点”清除按钮
	void mPostResultPath::slot_clearEndPoint()
	{
		clearPointPick(false);
	}

	// 槽函数：点击“路径结果”按钮：校验 → 置“计算中”态 → 延迟到事件循环完成一次重绘后同步求解
	void mPostResultPath::slot_generatePath()
	{
		if (_startNodeID < 0 || _endNodeID < 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("请先选择起始点和终止点"), true);
			return;
		}
		if (_startNodeID == _endNodeID)
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("起始点与终止点不能相同"), true);
			return;
		}
		if (_rWidget == nullptr || _db == nullptr)
			return;

		// 先置“计算中”UI 态（同一调用栈内改 UI 后再启 timer，确保“计算中…”先于冻结窗口出现），
		// singleShot(0) 让事件循环完成一次重绘后再进入同步求解（03 §2.6-A，界面忙而非假死）
		setComputingState(true);
		_solvePending = true;   // 求解已排队，期间显隐/帧信号的重检请求直接跳过（防重入）
		QTimer::singleShot(0, this, [this]() { solveAndShowPath(); });
	}

	// 设置拾取按钮的视觉状态：selecting 为 true 时显示淡蓝色发光效果
	void mPostResultPath::setPickButtonSelecting(QWidget* btn, bool selecting)
	{
		if (btn == nullptr)
			return;

		QGraphicsDropShadowEffect* glow = qobject_cast<QGraphicsDropShadowEffect*>(btn->graphicsEffect());
		if (glow == nullptr)
		{
			glow = new QGraphicsDropShadowEffect(btn);
			btn->setGraphicsEffect(glow);
		}

		if (selecting)
		{
			glow->setColor(QRgb(0x5677fc));     // 淡蓝色
			glow->setBlurRadius(5);             // 羽化半径
			glow->setOffset(0, 0);
		}
		else
		{
			glow->setColor(QRgb(0xdcdcdc));     // 亮灰色，默认
			glow->setBlurRadius(0);
			glow->setOffset(0, 0);
		}
	}

	// 开始节点拾取：isStart 为 true 表示拾取起始点，false 表示拾取终止点
	void mPostResultPath::startPointPick(bool isStart)
	{
		if (_rWidget == nullptr)
			return;

		// 避免同时进入两种拾取状态：如果当前开始拾取起始点，则先结束终止点拾取状态
		if (isStart && _endPointSelecting)
			clearPointPick(false);
		if (!isStart && _startPointSelecting)
			clearPointPick(true);

		// 根据 isStart 获取对应的状态引用
		bool& selecting = isStart ? _startPointSelecting : _endPointSelecting;
		QMetaObject::Connection& conn = isStart ? _startPointPickConn : _endPointPickConn;
		PosPickWidget1& panel = isStart ? _startPointPanel : _endPointPanel;

		// 切换拾取状态：当前未拾取则进入，当前已拾取则退出
		selecting = !selecting;
		if (selecting)
		{
			// 清空当前点记录，表示准备重新拾取
			if (isStart)
			{
				_startNodeID = -1;
				_startNodePos = QVector3D();
			}
			else
			{
				_endNodeID = -1;
				_endNodePos = QVector3D();
			}
			panel.setState(false);

			// 进入新的拾取前，先清空 MeshNode 选择容器，避免残留上次拾取结果
			auto meshData = _rWidget->getSelectionContainer()->getMeshSelectData();
			meshData->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshNode, {});
			MechPreData::UpdatePostWidgetHighlight(_rWidget);

			// 设置拾取过滤器为节点拾取，并监听选择容器就绪信号
			_rWidget->setPickFilter(mxr::PickFilter::PickMeshNode);
			conn = connect(_rWidget->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer,
				this, [this, isStart]() { finishPointPick(isStart); });
			setPickButtonSelecting(panel.btnok, true);
		}
		else
		{
			// 退出拾取状态：断开信号连接并恢复按钮样式
			if (conn)
				disconnect(conn);
			setPickButtonSelecting(panel.btnok, false);
		}
	}

	// 节点拾取完成：从选择容器中提取最新拾取到的节点，并记录到对应成员
	void mPostResultPath::finishPointPick(bool isStart)
	{
		if (_rWidget == nullptr)
			return;

		PosPickWidget1& panel = isStart ? _startPointPanel : _endPointPanel;
		bool& selecting = isStart ? _startPointSelecting : _endPointSelecting;
		QMetaObject::Connection& conn = isStart ? _startPointPickConn : _endPointPickConn;

		// 从选择容器中获取 MeshNode 类型的拾取结果
		auto nodes = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
		if (nodes.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("未拾取到节点"), true);
			return;
		}

		int nodeID = nodes[0].data_id.node_id;

		// 保留当前节点高亮，只清除其他可能的拾取类型（线、面、单元、部件），避免显示混乱
		auto meshData = _rWidget->getSelectionContainer()->getMeshSelectData();
		meshData->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshLine, {});
		meshData->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshFace, {});
		meshData->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshElement, {});
		meshData->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshPart, {});

		if (isStart)
		{
			// 记录起始节点 ID 及其变形后坐标
			_startNodeID = nodeID;
			_startNodePos = getDeformedNodePosition(nodeID);
		}
		else
		{
			// 记录终止节点 ID 及其变形后坐标
			_endNodeID = nodeID;
			_endNodePos = getDeformedNodePosition(nodeID);
		}

		// 更新面板显示：显示已选节点 ID
		panel.labelpicked->setText(QString("已选择节点 %1").arg(nodeID));
		panel.setState(true);

		// 结束本次拾取状态：断开信号连接并恢复按钮样式
		if (conn)
			disconnect(conn);
		selecting = false;
		setPickButtonSelecting(panel.btnok, false);
	}

	// 清除指定点的拾取状态：isStart 为 true 则清除起始点，false 则清除终止点
	void mPostResultPath::clearPointPick(bool isStart)
	{
		PosPickWidget1& panel = isStart ? _startPointPanel : _endPointPanel;
		bool& selecting = isStart ? _startPointSelecting : _endPointSelecting;
		QMetaObject::Connection& conn = isStart ? _startPointPickConn : _endPointPickConn;

		// 断开拾取信号连接
		if (conn)
			disconnect(conn);

		selecting = false;
		setPickButtonSelecting(panel.btnok, false);

		if (isStart)
		{
			_startNodeID = -1;
			_startNodePos = QVector3D();
		}
		else
		{
			_endNodeID = -1;
			_endNodePos = QVector3D();
		}
		panel.setState(false);
	}

	// 获取节点在当前动画帧下的变形后坐标：原始坐标 + 位移缩放
	QVector3D mPostResultPath::getDeformedNodePosition(int nodeID) const
	{
		if (_db == nullptr || _rWidget == nullptr)
			return QVector3D();

		// 获取当前后处理视口的 GUI 数据（包含当前帧和变形系数）
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		if (guidata == nullptr)
			return QVector3D();

		// 获取当前帧对应的模型数据
		mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
		if (modeldata == nullptr)
			return QVector3D();

		// 获取当前帧的位移数据，并读取变形缩放系数
		const QHash<int, QVector3D> dis = _db->getDisplacementDataByFrameID(guidata->_cloudFrame);
		QVector3D deformationScale = guidata->_deformFactor;

		// 将节点全局 ID 转换为模型内部连续索引
		int cId = modeldata->_nodedata.nodeidmap.value(nodeID, -1);
		if (cId < 0)
			return QVector3D();

		// 原始坐标 + 变形系数 * 位移 = 当前帧变形后坐标
		QVector3D coord = modeldata->getVector(cId);
		return coord + deformationScale * dis.value(cId);
	}

	// 计算线段与网格相交的单元，并返回这些单元包含的节点 ID 集合
	std::vector<int> mPostResultPath::findIntersectionNodeIDs(int startNodeID, int endNodeID) const
	{
		std::vector<int> result;
		if (_db == nullptr || _rWidget == nullptr)
			return result;

		// 获取后处理渲染器
		std::shared_ptr<mxr::REBUILD_PostRender> postRender = _rWidget->getPostRender();
		if (postRender == nullptr)
			return result;

		// 获取当前视口 GUI 数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		if (guidata == nullptr)
			return result;

		// 获取当前帧模型数据
		mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
		if (modeldata == nullptr)
			return result;

		// 获取查询代理和模型渲染数据代理，用于 ZsTempRender 的相交计算
		// S5 新门面：frameProxy(currentFrame()) 取承载托管代理（F8 契约：换帧/重装载后由本函数每次现取；
		// 未装载帧返回 nullptr，先于子调用判空防解引用）
		mxr::PostFrameDataProxy* frameData = postRender->frameProxy(postRender->currentFrame());
		if (frameData == nullptr)
			return result;
		mxr::IPostRendQueryDataProxy* queryProxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(
			frameData->getRendQueryDataProxy());
		mxr::IPostModelRenderDataProxy* modelProxy = frameData->getModelRenderDataProxy();
		if (queryProxy == nullptr || modelProxy == nullptr)
			return result;

		// 使用 ZsTempRender 提供的线段-三角形求交工具，得到相交单元节点 ID 集合
		ZsTempRender tempRender;
		std::set<int> ids = tempRender.getIntersectedNodeIDs(
			startNodeID, endNodeID, guidata->_cloudFrame,
			modeldata, _db, queryProxy, modelProxy);

		result.assign(ids.begin(), ids.end());
		return result;
	}

	// 计算线段与单元面的交点坐标，返回沿起点到终点方向排序的点序列
	std::vector<QVector3D> mPostResultPath::findIntersectionPoints(int startNodeID, int endNodeID) const
	{
		std::vector<QVector3D> result;
		if (_db == nullptr || _rWidget == nullptr)
			return result;

		// 获取后处理渲染器
		std::shared_ptr<mxr::REBUILD_PostRender> postRender = _rWidget->getPostRender();
		if (postRender == nullptr)
			return result;

		// 获取当前视口 GUI 数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		if (guidata == nullptr)
			return result;

		// 获取当前帧模型数据
		mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
		if (modeldata == nullptr)
			return result;

		// 获取查询代理和模型渲染数据代理，用于 ZsTempRender 的相交计算
		// S5 新门面：frameProxy(currentFrame()) 取承载托管代理（F8 契约：换帧/重装载后由本函数每次现取；
		// 未装载帧返回 nullptr，先于子调用判空防解引用）
		mxr::PostFrameDataProxy* frameData = postRender->frameProxy(postRender->currentFrame());
		if (frameData == nullptr)
			return result;
		mxr::IPostRendQueryDataProxy* queryProxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(
			frameData->getRendQueryDataProxy());
		mxr::IPostModelRenderDataProxy* modelProxy = frameData->getModelRenderDataProxy();
		if (queryProxy == nullptr || modelProxy == nullptr)
			return result;

		// 使用 ZsTempRender 提供的线段-三角形求交工具，得到线段与单元面的交点坐标
		ZsTempRender tempRender;
		result = tempRender.getIntersectedPoints(
			startNodeID, endNodeID, guidata->_cloudFrame,
			modeldata, _db, queryProxy, modelProxy);
		return result;
	}

	// 计算线段与网格实际相交的单元，返回 <part_id, ele_rid> 列表
	std::vector<std::pair<int, int>> mPostResultPath::findIntersectionElements(int startNodeID, int endNodeID) const
	{
		std::vector<std::pair<int, int>> result;
		if (_db == nullptr || _rWidget == nullptr)
			return result;

		// 获取后处理渲染器
		std::shared_ptr<mxr::REBUILD_PostRender> postRender = _rWidget->getPostRender();
		if (postRender == nullptr)
			return result;

		// 获取当前视口 GUI 数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		if (guidata == nullptr)
			return result;

		// 获取当前帧模型数据
		mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
		if (modeldata == nullptr)
			return result;

		// 获取查询代理和模型渲染数据代理
		// S5 新门面：frameProxy(currentFrame()) 取承载托管代理（F8 契约：换帧/重装载后由本函数每次现取；
		// 未装载帧返回 nullptr，先于子调用判空防解引用）
		mxr::PostFrameDataProxy* frameData = postRender->frameProxy(postRender->currentFrame());
		if (frameData == nullptr)
			return result;
		mxr::IPostRendQueryDataProxy* queryProxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(
			frameData->getRendQueryDataProxy());
		mxr::IPostModelRenderDataProxy* modelProxy = frameData->getModelRenderDataProxy();
		if (queryProxy == nullptr || modelProxy == nullptr)
			return result;

		// 调用 ZsTempRender::chooseNode 直接获取被线段击中的单元（part_id, ele_rid）
		ZsTempRender tempRender;
		std::vector<std::pair<int, int>> ele_ids;
		std::vector<QVector3D> point_poses;

		tempRender.chooseNode(startNodeID, endNodeID, guidata->_cloudFrame,
			modeldata, _db, queryProxy, modelProxy, ele_ids, point_poses);

		// 去重：同一单元可能被多个面重复命中
		std::set<std::pair<int, int>> unique_eles;
		for (const auto& pair : ele_ids)
			unique_eles.insert(pair);
		result.assign(unique_eles.begin(), unique_eles.end());
		return result;
	}

	// 计算包含指定节点的所有单元，返回 <part_id, ele_rid> 列表
	std::vector<std::pair<int, int>> mPostResultPath::findElementsByNode(int nodeID) const
	{
		std::vector<std::pair<int, int>> result;
		if (_db == nullptr || _rWidget == nullptr)
			return result;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		if (guidata == nullptr)
			return result;

		mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
		if (modeldata == nullptr)
			return result;

		// cell_node_ids 中存储的是节点连续 ID，而传入的 nodeID 为节点真实 ID，需要先转换
		int node_cId = modeldata->_nodedata.nodeidmap.value(nodeID, -1);
		if (node_cId < 0)
			return result;

		for (int part_id : modeldata->getAllPartsIDs())
		{
			const auto part_data = modeldata->getPartData(part_id);
			if (!part_data || !part_data->getPartVisual())
				continue;

			const auto& pele = part_data->_parteledata;
			const int eleCount = (int)pele.cell_rid.size();
			for (int ele_cid = 0; ele_cid < eleCount; ++ele_cid)
			{
				const int offset = pele.cell_node_offsets[ele_cid];
				const int count = pele.cell_node_counts[ele_cid];
				bool containsNode = false;
				for (int i = 0; i < count; ++i)
				{
					if (pele.cell_node_ids[offset + i] == node_cId)
					{
						containsNode = true;
						break;
					}
				}
				if (!containsNode)
					continue;

				const int ele_rid = pele.cell_rid[ele_cid];
				if (!part_data->getElementVisual(ele_rid))
					continue;

				result.push_back({ part_id, ele_rid });
			}
		}
		return result;
	}

	// “计算中”状态切换：按钮置灰改文本，拾取/列表/曲线生成按钮禁用（02 §3.3，防求解期间竞态）
	void mPostResultPath::setComputingState(bool computing)
	{
		_btnPathResult->setEnabled(!computing);
		_btnPathResult->setText(computing ? QString("计算中…") : QString("路径结果"));
		_startPointPanel.btnok->setEnabled(!computing);
		_startPointPanel.btnclear->setEnabled(!computing);
		_endPointPanel.btnok->setEnabled(!computing);
		_endPointPanel.btnclear->setEnabled(!computing);
		_btnCurveGeneration->setEnabled(!computing);
		_pathList->setEnabled(!computing && !_paths.empty());
	}

	// 遍历可见部件/可见单元提取棱边建图（Q5=A：隐藏部件/单元不入图）
	// 常规/二阶单元走 buildElemoutline（二阶按“角点-中点-角点”拆段，mid-side 节点入图，03 §5.2）；
	// 多面体（MX_POLYHEDRON）从 poly_face_* 面多边形周长提取棱边（03 §5.1）。
	// poly_face_* 由 mxdb 读入线程填装（mMxdbThread.cpp:389-409），其他读入源可能不填装：
	// 空数据守卫（评审 C2）——该单元跳过、无边入图，不影响其余部分连通。
	void mPostResultPath::buildTopologyGraph(PostWindow::PathSolver& solver, mPostModelData* modeldata)
	{
		if (modeldata == nullptr)
			return;
		solver.reserve((std::size_t)modeldata->getElementAmount() * 4);

		for (int part_id : modeldata->getAllPartsIDs())
		{
			auto part = modeldata->getPartData(part_id);
			if (part == nullptr || !part->getPartVisual())
				continue;   // 部件隐藏：整体不入图

			const auto& pele = part->_parteledata;
			const int eleCount = (int)pele.cell_rid.size();
			for (int ele_cid = 0; ele_cid < eleCount; ++ele_cid)
			{
				if (!part->getElementVisual(pele.cell_rid[ele_cid]))
					continue;   // 单元隐藏：不入图

				if (pele.cell_types[ele_cid] == PostCelllType::MX_POLYHEDRON)
				{
					// 空数据守卫（评审 C2）：poly_* 未填装时该单元跳过
					if (pele.poly_cell_indices.empty() || pele.poly_face_counts.empty())
						continue;
					auto cit = std::lower_bound(pele.poly_cell_indices.begin(), pele.poly_cell_indices.end(), ele_cid);
					if (cit == pele.poly_cell_indices.end() || *cit != ele_cid)
						continue;
					const int k = (int)(cit - pele.poly_cell_indices.begin());
					int faceNodeCursor = pele.poly_face_node_offsets[k];
					for (int f = 0; f < pele.poly_face_counts[k]; ++f)
					{
						const int fn = pele.poly_face_node_counts[pele.poly_face_offsets[k] + f];
						for (int i = 0; i < fn; ++i)
							solver.addEdge(pele.poly_face_node_ids[faceNodeCursor + i],
								pele.poly_face_node_ids[faceNodeCursor + (i + 1) % fn]);
						faceNodeCursor += fn;
					}
				}
				else
				{
					// 常规/二阶单元：buildElemoutline 输出“节点连续 ID 对”序列，两两一段
					const int off = pele.cell_node_offsets[ele_cid];
					const int cnt = pele.cell_node_counts[ele_cid];
					QList<int> nodes;
					nodes.reserve(cnt);
					for (int i = 0; i < cnt; ++i)
						nodes.append(pele.cell_node_ids[off + i]);
					const std::vector<int> outline = modeldata->buildElemoutline(nodes, pele.cell_types[ele_cid]);
					for (std::size_t i = 0; i + 1 < outline.size(); i += 2)
						solver.addEdge(outline[i], outline[i + 1]);
				}
			}
		}
	}

	// singleShot 延迟执行：点击时建图、求解、用后即释（03 §2.3-A），并按结果分发
	void mPostResultPath::solveAndShowPath()
	{
		_solvePending = false;   // 已入队求解在此消费，后续信号按新指纹基线重新评估

		// 排队等待期间面板已清理（hideEvent/newPathResult 复位拾取与路径数据）时静默取消
		if (_startNodeID < 0 || _endNodeID < 0)
		{
			setComputingState(false);
			return;
		}

		if (_rWidget == nullptr || _db == nullptr)
		{
			setComputingState(false);
			return;
		}

		std::shared_ptr<mxr::REBUILD_PostRender> postRender = _rWidget->getPostRender();
		if (postRender == nullptr)
		{
			setComputingState(false);
			return;
		}

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		mPostModelData* modeldata = (guidata != nullptr) ? _db->getModelData(guidata->_cloudFrame) : nullptr;
		if (modeldata == nullptr)
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("路径结果计算失败：当前帧模型数据不存在"), true);
			setComputingState(false);
			return;
		}

		// 起终点真实 ID → 连续 ID；查不到即孤立节点（边界 11），按不连通口径处理
		const int startCId = modeldata->_nodedata.nodeidmap.value(_startNodeID, -1);
		const int endCId = modeldata->_nodedata.nodeidmap.value(_endNodeID, -1);

		// 建图 + 求解（耗时统计含建图，NFR-1：超 1s 输出实际耗时）
		QElapsedTimer timer;
		timer.start();
		PostWindow::PathSolveResult result;
		if (startCId >= 0 && endCId >= 0)
		{
			PostWindow::PathSolver solver;
			buildTopologyGraph(solver, modeldata);
			result = solver.solve(modeldata->_nodedata.nodeamount, startCId, endCId,
				[modeldata](int cid, double& x, double& y, double& z)
				{
					const QVector3D p = modeldata->getVector(cid);   // 参考坐标（Q2=A）
					x = p.x();
					y = p.y();
					z = p.z();
				});
			// solver 析构即释放图内存（solve 内部已释放临时边表）
		}
		const qint64 elapsedMs = timer.elapsed();

		// 保留起终点高亮，清除其他拾取类型（原 drawTempLineAndHighlight 逻辑迁移保留）
		keepEndPointHighlights();

		if (result.status != PostWindow::PathSolveResult::Status::Ok)
		{
			// FR-06 不连通（含孤立节点）：消息提示、列表空禁用、3D 无路径线、拾取高亮保留
			clearPathResults();
			_summaryLabel->setText(QString("最短路径（两节点不连通）"));
			// S5 新门面：清空临时标注=G5 TempMark 整面替换为空（缺省空 nodes/lines，空集重置）
			mxr::TempMarkParams tempMark;
			postRender->addObject(tempMark);
			postRender->updateHighLightRender();
			_rWidget->updatePaint();
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("两节点间不存在连通路径"), true);
			setComputingState(false);
			return;
		}

		// 成功：填列表（行号即路径下标），汇总行与消息区按计数封顶情况提示（FR-04/R4 裁决）
		_paths = std::move(result.paths);
		_currentPathIndex = -1;
		fillPathList(modeldata);

		if (result.countCapped)
		{
			// 计数超 1 万封顶：实际条数无法如实给出（R4 裁决文案）
			_summaryLabel->setText(QString("最短路径（等长路径超过 1 万条，仅显示前 100）"));
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("等长最短路径超过 1 万条，仅保留前 100 条"), true);
		}
		else if (result.truncatedToListCap)
		{
			_summaryLabel->setText(QString("最短路径（共 %1 条，仅显示前 100）").arg(result.totalEqualCount));
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("等长最短路径共 %1 条，仅保留前 100 条").arg(result.totalEqualCount), true);
		}
		else
		{
			_summaryLabel->setText(QString("最短路径（共 %1 条）").arg(result.totalEqualCount));
		}

		// NFR-1/边界 10：耗时超过 1s 输出实际耗时提示（Debug 构建慢 5~10 倍属预期）
		if (elapsedMs > 1000)
			mGlobalSignals::getInstance()->outputMessageSig(1, QString("路径结果计算耗时 %1 ms").arg(elapsedMs), true);

		// 记录显隐指纹（R6 惰性重检基准：模型指针 + 部件可见位 + 隐藏单元计数）
		_fingerValid = computeVisibilityFingerprint(_fingerValue);

		// 默认选中第 1 条并高亮（FR-03/FR-07）
		if (!_paths.empty())
		{
			_pathList->blockSignals(true);
			_pathList->selectRow(0);
			_pathList->blockSignals(false);
			showPath(0);
		}

		setComputingState(false);
	}

	// 绘制指定序号路径：temp 折线（节点真实 ID 对，切帧由 proxy 按当前帧重解析）+
	// 路径节点标记（当前帧变形后绝对坐标，切帧需重设）
	void mPostResultPath::showPath(int index)
	{
		if (index < 0 || index >= (int)_paths.size())
			return;
		if (_rWidget == nullptr || _db == nullptr)
			return;

		std::shared_ptr<mxr::REBUILD_PostRender> postRender = _rWidget->getPostRender();
		if (postRender == nullptr)
			return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		mPostModelData* modeldata = (guidata != nullptr) ? _db->getModelData(guidata->_cloudFrame) : nullptr;
		if (modeldata == nullptr)
			return;

		const PostWindow::PathEntry& path = _paths[index];
		_currentPathIndex = index;

		const int n = (int)path.nodeCIds.size();
		std::vector<std::pair<int, int>> lines;
		std::vector<QVector3D> nodePoses;
		lines.reserve(n > 0 ? n - 1 : 0);
		nodePoses.reserve(n);
		int prevRealId = -1;
		for (int k = 0; k < n; ++k)
		{
			const int cId = path.nodeCIds[k];
			if (cId < 0 || cId >= modeldata->_nodedata.nodeamount)
				return;   // 脏数据防御：连续 ID 越界不绘制
			const int realId = modeldata->_nodedata.ids[cId];
			nodePoses.push_back(getDeformedNodePosition(realId));
			if (k > 0)
				lines.push_back({ prevRealId, realId });
			prevRealId = realId;
		}

		// S5 新门面：临时标注=G5 TempMark 单实例整面替换（重复 addObject=整面替换）
		mxr::TempMarkParams tempMark;
		tempMark.nodes = QVector<QVector3D>::fromStdVector(nodePoses);
		tempMark.lines.reserve(int(lines.size()));
		for (const auto& l : lines)
			tempMark.lines.push_back(QPair<int, int>(l.first, l.second));
		postRender->addObject(tempMark);
		postRender->updateHighLightRender();
		_rWidget->updatePaint();
	}

	// FR-08：按当前帧刷新起终点记录坐标与路径显示位置（拓扑不重算、列表不动）
	void mPostResultPath::refreshPathDisplay()
	{
		if (_currentPathIndex < 0 || _currentPathIndex >= (int)_paths.size())
			return;

		if (_startNodeID >= 0)
			_startNodePos = getDeformedNodePosition(_startNodeID);
		if (_endNodeID >= 0)
			_endNodePos = getDeformedNodePosition(_endNodeID);
		showPath(_currentPathIndex);
	}

	// 保留起终点高亮，清除其他节点拾取类型（原 drawTempLineAndHighlight :617-625 迁移；
	// Q6=A：不再向 MeshElement 容器写相交单元高亮）
	void mPostResultPath::keepEndPointHighlights()
	{
		if (_rWidget == nullptr)
			return;

		auto meshData = _rWidget->getSelectionContainer()->getMeshSelectData();
		std::vector<MeshSelectInfoData> keepNodes;
		for (const MeshSelectInfoData& n : meshData->getSelectDataByType(mxr::SelectionInfoType::MeshNode))
		{
			if (n.data_id.node_id == _startNodeID || n.data_id.node_id == _endNodeID)
				keepNodes.push_back(n);
		}
		meshData->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshNode, {});
		meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshNode, keepNodes);
	}

	// 依据 _paths 整体刷新列表控件；modeldata 供连续 ID → 真实 ID 映射（tooltip）
	void mPostResultPath::fillPathList(mPostModelData* modeldata)
	{
		_pathList->blockSignals(true);
		_pathList->setRowCount((int)_paths.size());
		_pathList->setEnabled(!_paths.empty());

		for (int i = 0; i < (int)_paths.size(); ++i)
		{
			const PostWindow::PathEntry& path = _paths[i];

			// tooltip：节点真实 ID 序列，超长截断加省略号（02 §3.1）
			QString tip;
			if (modeldata != nullptr)
			{
				const int maxTipIds = 32;
				const int n = (int)path.nodeCIds.size();
				for (int k = 0; k < n && k < maxTipIds; ++k)
				{
					if (k)
						tip += QStringLiteral("-");
					const int cId = path.nodeCIds[k];
					if (cId >= 0 && cId < modeldata->_nodedata.nodeamount)
						tip += QString::number(modeldata->_nodedata.ids[cId]);
				}
				if (n > maxTipIds)
					tip += QStringLiteral("-…");
			}

			QTableWidgetItem* itemIndex = new QTableWidgetItem(QString::number(i + 1));
			itemIndex->setData(Qt::UserRole, i);
			itemIndex->setToolTip(tip);
			QTableWidgetItem* itemNodes = new QTableWidgetItem(QString::number(path.nodeCIds.size()));
			itemNodes->setToolTip(tip);
			QTableWidgetItem* itemLen = new QTableWidgetItem(QString::number(path.length, 'f', 3));
			itemLen->setToolTip(tip);

			_pathList->setItem(i, 0, itemIndex);
			_pathList->setItem(i, 1, itemNodes);
			_pathList->setItem(i, 2, itemLen);
		}
		_pathList->blockSignals(false);
	}

	// 清空路径结果：_paths/当前序号/列表/汇总行/显隐指纹
	void mPostResultPath::clearPathResults()
	{
		_paths.clear();
		_currentPathIndex = -1;
		_fingerValid = false;
		_summaryLabel->setText(QString("最短路径（尚未计算）"));
		fillPathList(nullptr);
	}

	// 计算显隐指纹：模型指针 + Σ(部件可见位 + 隐藏单元计数)。
	// 复杂度 O(总单元数)（getMaskEleNum 逐 cell_info 扫描，Σ 各 part），仅低频重检时机调用
	bool mPostResultPath::computeVisibilityFingerprint(qint64& fp) const
	{
		fp = 0;
		if (_db == nullptr || _rWidget == nullptr)
			return false;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		if (guidata == nullptr)
			return false;

		mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
		if (modeldata == nullptr)
			return false;

		// 模型数据指针参与指纹：网格重导/切 case 后旧指纹失效
		fp = (qint64)std::hash<const void*>()(modeldata);
		for (int part_id : modeldata->getAllPartsIDs())
		{
			auto part = modeldata->getPartData(part_id);
			if (part == nullptr)
				continue;
			fp = fp * 1000003 + (part->getPartVisual() ? 1 : 0) * 31 + part->getMaskEleNum();
		}
		return true;
	}

	// R6 惰性显隐指纹重检：不一致则按当前显隐自动重算并整体刷新（FR-07/边界 6；
	// 重算后不连通则按 FR-06 清空提示）
	void mPostResultPath::checkVisibilityFingerprint()
	{
		if (!_fingerValid || _paths.empty())
			return;
		// 防重入（BUG-01 修复配套）：已有求解排队（点击或自动重算）时跳过本次重检，
		// 待 solveAndShowPath 完成后按新指纹基线评估，避免重复入队
		if (_solvePending)
			return;

		qint64 fp = 0;
		if (!computeVisibilityFingerprint(fp))
		{
			clearPathResults();
			return;
		}
		if (fp == _fingerValue)
			return;

		// BUG-01（S6 第 1 轮修复，02 §3.3 QY-04 定案）：自动重算与点击「路径结果」同策略——
		// 先置“计算中”态，singleShot(0) 让事件循环完成一次重绘后同步求解（NFR-1：任何情况
		// 不允许无提示假死）；求解完成由 solveAndShowPath 统一恢复使能并刷新列表/3D
		setComputingState(true);
		_solvePending = true;
		QTimer::singleShot(0, this, [this]() { solveAndShowPath(); });
	}

	// 槽函数：点击列表条目 → 切换当前高亮路径（FR-05：只换渲染数据，不重算）
	void mPostResultPath::slot_pathItemSelected(int row, int column)
	{
		Q_UNUSED(column);
		if (row < 0 || row >= (int)_paths.size())
			return;
		showPath(row);

		// R6：列表点击是显隐指纹惰性重检时机之一
		checkVisibilityFingerprint();
	}

	// 槽函数：云图帧变化 → 刷新显示位置 + 显隐指纹惰性重检
	void mPostResultPath::slot_postFrameChanged()
	{
		refreshPathDisplay();

		// R6/评审 C3 写死策略：动画播放中跳过指纹重检（播放中用户不可能同时改显隐，
		// 停止/下次交互必补检）。播放判定：frameChanged 高频连发（与上次间隔 <250ms）
		const qint64 now = QDateTime::currentMSecsSinceEpoch();
		const bool playing = (now - _lastFrameChangeMs) < 250;
		_lastFrameChangeMs = now;
		if (!playing)
			checkVisibilityFingerprint();
	}

	// 槽函数：变形系数变化 → 刷新显示位置 + 显隐指纹惰性重检（低频手动操作）
	void mPostResultPath::slot_postDeformChanged()
	{
		refreshPathDisplay();
		checkVisibilityFingerprint();
	}

	// 清理结果：路径数据/列表清空、帧信号断开、临时线条节点与网格高亮清除
	void mPostResultPath::clearResult()
	{
		// 路径结果与列表清理（不依赖视口，先于 _rWidget 判空执行）
		clearPathResults();

		// R7：断开帧/变形信号连接（与测量面板连接管理同款）
		if (_frameConn)
		{
			disconnect(_frameConn);
			_frameConn = QMetaObject::Connection();
		}
		if (_deformConn)
		{
			disconnect(_deformConn);
			_deformConn = QMetaObject::Connection();
		}

		if (_rWidget == nullptr)
			return;

		std::shared_ptr<mxr::REBUILD_PostRender> postRender = _rWidget->getPostRender();
		if (postRender == nullptr)
			return;

		// S5 新门面：清空临时节点和临时线条数据=G5 TempMark 整面替换为空（空集重置）
		mxr::TempMarkParams tempMark;
		postRender->addObject(tempMark);
		postRender->updateHighLightRender();

		// 清除后处理网格拾取高亮
		MechPreData::ClearPostWidgetMeshPick(_rWidget);
		MechPreData::UpdatePostWidgetHighlight(_rWidget);

		// 刷新视口
		_rWidget->updatePaint();
	}

	// 遍历可见部件/可见单元，建立“路径边(连续 ID 对) → 关联单元”映射（FR-09 type==1 取值用）。
	// 棱边提取口径与 buildTopologyGraph（即 mPostPathSolver 建图）完全一致：
	// 常规/二阶单元走 buildElemoutline 两两一段，多面体走 poly_face_* 面周长（含同样空数据守卫）；
	// 同一边归属多个单元时取遍历序首个，确定性唯一（FR-09 取值规则写死）
	void mPostResultPath::buildEdgeElementMap(mPostModelData* modeldata, QHash<qint64, QPair<int, int>>& edgeToElement)
	{
		edgeToElement.clear();
		if (modeldata == nullptr)
			return;

		for (int part_id : modeldata->getAllPartsIDs())
		{
			auto part = modeldata->getPartData(part_id);
			if (part == nullptr || !part->getPartVisual())
				continue;   // 部件隐藏：不参与

			const auto& pele = part->_parteledata;
			const int eleCount = (int)pele.cell_rid.size();
			for (int ele_cid = 0; ele_cid < eleCount; ++ele_cid)
			{
				if (!part->getElementVisual(pele.cell_rid[ele_cid]))
					continue;   // 单元隐藏：不参与

				if (pele.cell_types[ele_cid] == PostCelllType::MX_POLYHEDRON)
				{
					// 空数据守卫（评审 C2，与 buildTopologyGraph 一致）：poly_* 未填装时跳过
					if (pele.poly_cell_indices.empty() || pele.poly_face_counts.empty())
						continue;
					auto cit = std::lower_bound(pele.poly_cell_indices.begin(), pele.poly_cell_indices.end(), ele_cid);
					if (cit == pele.poly_cell_indices.end() || *cit != ele_cid)
						continue;
					const int k = (int)(cit - pele.poly_cell_indices.begin());
					int faceNodeCursor = pele.poly_face_node_offsets[k];
					for (int f = 0; f < pele.poly_face_counts[k]; ++f)
					{
						const int fn = pele.poly_face_node_counts[pele.poly_face_offsets[k] + f];
						for (int i = 0; i < fn; ++i)
						{
							const int a = pele.poly_face_node_ids[faceNodeCursor + i];
							const int b = pele.poly_face_node_ids[faceNodeCursor + (i + 1) % fn];
							const qint64 key = (qint64)qMin(a, b) << 32 | (quint32)qMax(a, b);
							if (!edgeToElement.contains(key))
								edgeToElement.insert(key, qMakePair(part_id, pele.cell_rid[ele_cid]));
						}
						faceNodeCursor += fn;
					}
				}
				else
				{
					// 常规/二阶单元：buildElemoutline 输出“节点连续 ID 对”序列，两两一段
					const int off = pele.cell_node_offsets[ele_cid];
					const int cnt = pele.cell_node_counts[ele_cid];
					QList<int> nodes;
					nodes.reserve(cnt);
					for (int i = 0; i < cnt; ++i)
						nodes.append(pele.cell_node_ids[off + i]);
					const std::vector<int> outline = modeldata->buildElemoutline(nodes, pele.cell_types[ele_cid]);
					for (std::size_t i = 0; i + 1 < outline.size(); i += 2)
					{
						const int a = outline[i];
						const int b = outline[i + 1];
						const qint64 key = (qint64)qMin(a, b) << 32 | (quint32)qMax(a, b);
						if (!edgeToElement.contains(key))
							edgeToElement.insert(key, qMakePair(part_id, pele.cell_rid[ele_cid]));
					}
				}
			}
		}
	}

	// 点击“曲线生成”按钮：沿当前选中路径的节点序列取样生成曲线（FR-09，2026-11-02 用户修订定稿）
	// X 轴 = 路径点序号 1～N（N=当前路径节点数）；Y 轴 = 节点真实结果值。
	// 旧“起终点直线求交取样”口径（R8）已废弃，不再保留任何回退分支。
	// type==0（结果在节点上）：直接取节点结果值；type==1（结果在单元上）：路径节点取其
	// 与路径上前一节点所成路径边关联单元的结果值（起点取其后第一条路径边），
	// 边-单元关联与 mPostPathSolver 建图数据一致（沿单元棱边相邻，见 buildEdgeElementMap）。
	// 列表切换当前路径不自动重画本曲线（无信号联动），重新点击本按钮才按新当前路径取样。
	void mPostResultPath::slot_generateNodeCurve()
	{
		// FR-09 前置：未计算路径（列表无当前选中条目）时提示后返回，不自动计算、不回退直线求交
		if (_currentPathIndex < 0 || _currentPathIndex >= (int)_paths.size())
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("请先点击路径结果计算最短路径"), true);
			return;
		}
		if (_rWidget == nullptr || _db == nullptr)
			return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(_rWidget);
		if (guidata == nullptr)
			return ;

		const PostWindow::PathEntry& path = _paths[_currentPathIndex];
		const int n = (int)path.nodeCIds.size();
		if (n < 2)
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("数据有误：路径节点数不足，曲线生成结束"), true);
			return;
		}

		QVector<double> xValues;
		QVector<double> yValues;
		//获取结果类型
		int type=_db->getFrameResultType(guidata->_cloudFrame);
		if (type == 1)//在单元上
		{
			//获取当前帧模型数据（边-单元关联需单元连接数据）
			mPostModelData* modeldata = _db->getModelData(guidata->_cloudFrame);
			if (modeldata == nullptr)
			{
				mGlobalSignals::getInstance()->outputMessageSig(2, QString("数据有误：当前帧模型数据不存在，曲线生成结束"), true);
				return;
			}

			// 路径边 → 关联单元映射（与求解器建图同口径，一次性建立避免逐边扫描）
			QHash<qint64, QPair<int, int>> edgeToElement;
			buildEdgeElementMap(modeldata, edgeToElement);

			//获取单元对应的值（key 为单元真实 ID，即 ele_rid，与既有取数口径一致）
			QHash<int, double> elementValue=_db->getResultDataByFrameID(guidata->_cloudFrame);
			int index = 1;
			for (int k = 0; k < n; ++k)
			{
				xValues.push_back(index);
				// 取值边：起点取其后第一条路径边(0,1)；其余节点取其与前一节点所成路径边(k-1,k)
				const int prevIdx = (k == 0) ? 0 : k - 1;
				const int nextIdx = (k == 0) ? 1 : k;
				const int a = path.nodeCIds[prevIdx];
				const int b = path.nodeCIds[nextIdx];
				const qint64 edgeKey = (qint64)qMin(a, b) << 32 | (quint32)qMax(a, b);
				if (!edgeToElement.contains(edgeKey))//路径边在入图单元中不存在
				{
					mGlobalSignals::getInstance()->outputMessageSig(2, QString("数据有误：路径边对应的单元不存在，曲线生成结束"), true);
					return;
				}
				const int eleRid = edgeToElement[edgeKey].second;
				if (elementValue.contains(eleRid))//找到单元ID对应的结果值
				{
					yValues.push_back(elementValue[eleRid]);//取出对应的值
				}
				else
				{
					mGlobalSignals::getInstance()->outputMessageSig(2, QString("数据有误：单元对应的结果值不存在，曲线生成结束"), true);
					return;
				}
				++index;
			}
		}
		else if (type == 0)//在节点上
		{
			// 获取节点结果值（key 为节点连续 ID，与路径节点序列同口径，直接一一对应）
			QHash<int, double> nodeValue = _db->getResultDataByFrameID(guidata->_cloudFrame);
			int index = 1;
			for (int k = 0; k < n; ++k)
			{
				xValues.push_back(index);
				const int cId = path.nodeCIds[k];
				if (!nodeValue.contains(cId))//找到节点对应的结果值
				{
					mGlobalSignals::getInstance()->outputMessageSig(2, QString("数据有误：节点对应的结果值不存在，曲线生成结束"), true);
					return;
				}
				yValues.push_back(nodeValue[cId]);//取出对应的值
				++index;
			}
		}
		else
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("数据有误：获取结果类型失败，曲线生成结束"), true);
			return;
		}
		//数据存在判断
		if (xValues.empty() || yValues.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("无数据，曲线生成结束"), true);
			return;
		}

		// 创建新的曲线视口
		PlotView::mPlotView* plotView = new PlotView::mPlotView();
		plotView->setPlotBackground(QColor(255, 255, 255));
		_nodeCurveViewName = "nodeCurveView";
		if (plotView == nullptr)
			return;

		// 曲线名/标题含当前路径条目序号（FR-09：条目 k、N 点；X 轴标题保持 Node Index）
		const int entryNo = _currentPathIndex + 1;
		QString curveName = QString("Node-Value @ Path %1-%2").arg(entryNo).arg(xValues.size());
		QString xTitle = "Node Index";
		QString yTitle = "Value";
		QString title = QString("Node Value along Path %1-%2").arg(entryNo).arg(xValues.size());

		plotView->clearAll();
		plotView->getQwtPlot()->setAxisTitle(QwtPlot::xBottom, xTitle);
		plotView->getQwtPlot()->setAxisTitle(QwtPlot::yLeft, yTitle);
		plotView->setTitle(title);

		plotView->updateCurveData(curveName, xValues, yValues);
		plotView->setPlotLegendItemVisible(true);
		plotView->rescaleAxes();
		plotView->setEnableMarker(true);

		// 缓存数据供数据表使用
		PlotDataCache cache;
		cache.viewName = _nodeCurveViewName;
		cache.xTitle = xTitle;
		cache.yTitles.append(yTitle);
		cache.xValue = xValues;
		cache.curveDatas.append(qMakePair(curveName, yValues));
		cache.curveNames.append(curveName);
		cachePlotData(_nodeCurveViewName, cache);

		// 在独立浮动窗口中显示
		openPlotInFloatWindow(_nodeCurveViewName, plotView);
	}

	void mPostResultPath::openPlotInFloatWindow(const QString& vname, PlotView::mPlotView* plotView)
	{
		mPostPlotFloatWindow* floatWin = nullptr;
		if (_plotFloatWindows.contains(vname))
		{
			floatWin = _plotFloatWindows[vname];
            if (floatWin == nullptr)
            {
                // 旧窗口已被外部关闭，移除失效条目
                _plotFloatWindows.remove(vname);
                floatWin = new mPostPlotFloatWindow();
                _plotFloatWindows[vname] = floatWin;
            }
			floatWin->setPlotView(plotView);
			floatWin->setWindowTitleText(vname);
		}
		else
		{
			floatWin = new mPostPlotFloatWindow();
			floatWin->setWindowTitleText(vname);
			floatWin->setPlotView(plotView);
			_plotFloatWindows[vname] = floatWin;
		}

		QString keyName = vname + "PostWidow";
		mDockLayoutManager::getInstance()->addDockToBottom(floatWin, vname, keyName);
	}

	void mPostResultPath::cachePlotData(const QString& vname, const PlotDataCache& cache)
	{
		_plotDataCache[vname] = cache;
	}
}
