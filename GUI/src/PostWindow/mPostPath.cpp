#include "mPostPath.h"

#include <QFormLayout>
#include <QHBoxLayout>

#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostModelData.h"
#include "mPostGUIData.h"
#include "mOtherFunction.h"


using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;

namespace PostWindow
{
	mPostPathPanel::mPostPathPanel(QWidget* parent) : mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);

		/********************类型********************/
		FoldSection* typeFold = new FoldSection("类型", this);

		//类型
		_comboPathType = new QComboBox();
		_comboPathType->addItems({ "节点" });
		typeFold->addRow(new QLabel("类型"), _comboPathType);

		typeFold->refreshLayout();
		m_foldWidgets.append(typeFold);
		_mainLayout->addWidget(typeFold);

		/********************路径列表********************/
		FoldSection* listFold = new FoldSection("路径列表", this);

		QGridLayout* layout = new QGridLayout();
		_listPathIds = new QListWidget();
		_listPathIds->setSelectionMode(QAbstractItemView::SingleSelection);

		NodeIdDelegate* delegate = new NodeIdDelegate(this);
		_listPathIds->setItemDelegate(delegate);

		QFrame* line = new QFrame(this);
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken); 
		line->setFixedHeight(10);

		_btnFront = new QToolButton(); _btnFront->setIcon(QIcon(":/mMainWidgets/Icon/post/shangyizheng.png")); _btnFront->setToolTip("添加于前");
		_btnEnd = new QToolButton(); _btnEnd->setIcon(QIcon(":/mMainWidgets/Icon/post/xiayizheng.png")); _btnEnd->setToolTip("添加于后");
		_btnAdd = new QToolButton(); _btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); _btnAdd->setToolTip("添加");
		_btnRemove = new QToolButton(); _btnRemove->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); _btnRemove->setToolTip("移除");
		_btnClear = new QToolButton(); _btnClear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); _btnClear->setToolTip("清空");

		_btnFront->setCheckable(true);
		_btnEnd->setCheckable(true);
		QButtonGroup* group = new QButtonGroup(this);
		group->addButton(_btnFront);
		group->addButton(_btnEnd);
		group->setExclusive(true); 
		_btnEnd->setChecked(true);

		layout->addWidget(_listPathIds, 0, 0, 8, 7);
		layout->addWidget(_btnFront, 0, 7);
		layout->addWidget(_btnEnd, 1, 7);
		layout->addWidget(line, 2, 7);
		layout->addWidget(_btnAdd, 3, 7);
		layout->addWidget(_btnRemove, 4, 7);
		layout->addWidget(_btnClear, 5, 7);

		listFold->addRow(layout);
		m_foldWidgets.append(listFold);
		_mainLayout->addWidget(listFold);
		_mainLayout->addStretch();

		// 连接信号槽
		connect(_btnAdd, &QToolButton::clicked, this, [this]() {
			slot_addItem();
		});
		connect(_btnRemove, &QToolButton::clicked, this, &mPostPathPanel::slot_removeItem);
		connect(_btnClear, &QToolButton::clicked, this, &mPostPathPanel::slot_clearItems);

		// 编辑完成
		connect(_listPathIds, &QListWidget::itemChanged, this, &mPostPathPanel::slot_itemChanged);
		//拾取新增
		//connect(_listPathIds->model(), &QAbstractItemModel::rowsInserted, this, &mPostPathPanel::storagePathData);
	}

	void mPostPathPanel::connectPickSig()
	{
		disconnectPickSig();
        if (!getCVM || !getCVM->getPostRenderWidget()) return;
		_conn = connect(getCVM->getPostRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mPostPathPanel::slot_finishedPick);
	}

	void mPostPathPanel::disconnectPickSig()
	{
        if (!getCVM || !getCVM->getPostRenderWidget()) return;

		if (_conn)
			disconnect(_conn);
	}

	void mPostPathPanel::slot_finishedPick()
	{
		int num = 0;
		std::vector<MeshSelectInfoData> res;
		//获取拾取模式
		PickDataType pickDataType = getCVM->getPostPickDataType(num);
		if (pickDataType != PickDataType::Node) return;

		res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
		if (res.size() == 0) return;

		int id = res[0].data_id.node_id;
		slot_addItem(MxFunctions::mNumberToString(id));
		getCVM->clearPick();
		storagePathData();
	}

	void mPostPathPanel::uneditPathPanel()
	{
		disconnectPickSig();
	}

	void mPostPathPanel::editPathForViewport(int caseid)
	{
		//视口绑定打开（用户定案 2026-11-04）：路径不按次生成实例——数据键=当前激活
		//后处理视口名：同视口重复打开沿用既有节点列表，切换视口后打开自动跟随。
		//条目登记按视口名幂等（路径数据挂在 PathInfo 条目上，无条目则 addPathData
		//静默无效，故首开该视口时登记）
		auto* curPriv = VMManager->getCurrentViewportPrivate();
		if (curPriv == nullptr || curPriv->getViewportType() != MxFunctions::ViewportPost3D)
		{
			return;
		}
		const QString viewName = curPriv->getViewportName();
		if (viewName.isEmpty())
		{
			return;
		}

		initUI();

		_curViewName = viewName;

		_postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
		if (_postDB == nullptr) return;

		auto& mgr = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post);
		if (!mgr.hasItemByViewName(ItemType::PathItem, viewName))
		{
			int pathID = mgr.getItemId(ItemType::PathItem);
			PathInfo info;
			info.caseId = caseid;
			info.pathId = pathID;
			info.itemName = QString("路径_%1").arg(pathID);
			info.viewName = viewName;
			mgr.addItem(ItemType::PathItem, info);
		}
		else
		{
			//该视口已有路径：回填节点列表（首开为空列表，拾取后经 storagePathData 落库）
			updatePathGUI();
		}
	}

	void mPostPathPanel::setCurViewName(const QString& viewName)
	{
		_curViewName = viewName;
	}

	void mPostPathPanel::updatePathGUI()
	{
		connectPickSig();

		bool isEnd;
		QStringList pathList;
		QVector<int> pathIDs;
		getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPathData(_curViewName, isEnd, pathList, pathIDs);

		if (isEnd)
		{
			_btnFront->setChecked(false);
			_btnEnd->setChecked(true);
		}
		else
		{
			_btnFront->setChecked(true);
			_btnEnd->setChecked(false);
		}

		_listPathIds->blockSignals(true);
		_listPathIds->clear();
		for (auto str : pathList)
		{
			QListWidgetItem* item = new QListWidgetItem(str);
			item->setFlags(item->flags() | Qt::ItemIsEditable);
			_listPathIds->addItem(item);
		}
		_listPathIds->blockSignals(false);

		selectLastRow();

		//高亮路径
        highlightPath(pathIDs);

	}

	void mPostPathPanel::initUI()
	{
		_listPathIds->clear();
	}

	void mPostPathPanel::addItem(QString str)
	{
		QListWidgetItem* item = new QListWidgetItem(str);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		_listPathIds->addItem(item);
		_listPathIds->editItem(item);
	}

	void mPostPathPanel::addPickItem(int id)
	{
		QString str = MxFunctions::mNumberToString(id);
		QListWidgetItem* item = new QListWidgetItem(str);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		_listPathIds->addItem(item);
	}

	void mPostPathPanel::insertItemAt(int row, const QString& text)
	{
		QListWidgetItem* item = new QListWidgetItem(text);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		_listPathIds->insertItem(row, item);
		_listPathIds->editItem(item);
	}

	void mPostPathPanel::selectLastRow()
	{
		int count = _listPathIds->count();
		if (count > 0)
		{
			_listPathIds->setCurrentRow(count - 1);
		}
	}

	QVector<int> mPostPathPanel::parseIds(const QString& input)
	{
		QVector<int> result;
		QString text = input.trimmed();

		if (text.contains(':'))
		{
			// 范围格式
			QStringList parts = text.split(':');
			int start = parts[0].toInt();
			int end = parts[1].toInt();
			int step = (parts.size() == 3) ? parts[2].toInt() : 1;

			for (int i = start; i <= end; i += step)
				result.append(i);
		}
		else if (text.contains(','))
		{
			// 逗号分隔
			for (const QString& part : text.split(','))
				result.append(part.trimmed().toInt());
		}
		else
		{
			// 单个数字
			result.append(text.toInt());
		}

		return result;
	}

	void mPostPathPanel::storagePathData()
	{
		//路径绘制
		int count = _listPathIds->count();

		QStringList pathList;
		QVector<int> pathIDs;
		for (int i = 0; i < count; ++i) 
		{
			QListWidgetItem* item = _listPathIds->item(i);
			if (item)
			{
				pathList.append(item->text());
				if (!item->text().isEmpty())
				{
					pathIDs.append(parseIds(item->text()));
				}
			}
		}

		bool isEnd = true;
		if (!_btnEnd->isChecked()) isEnd = false;
		//数据存储
		getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addPathData(_curViewName, isEnd, pathList, pathIDs);

		//高亮
        highlightPath(pathIDs);

        updateItemStatus();
	}

    void mPostPathPanel::updateItemStatus()
    {
        BrowserTreeStatus statu;
        int phyID, caseID;
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPathStatus(_curViewName, phyID, caseID, statu);

        //更新模型树状态
        mMechGlobalSignals::getInstance()->updatePostBrowserTreeStatus(PathChildItem, phyID, caseID, statu);
    }

    void mPostPathPanel::highlightPath(QVector<int> pathIDs, bool isIntersect)
    {
        if (!_postDB) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto postRender = postRend->getPostRender();

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;

        mPostModelData* modelData = _postDB->getModelData(guidata->_cloudFrame);
        if (modelData == nullptr)
            return;

        //高亮路径
        std::vector<QVector3D> poss;
        std::vector<std::pair<int, int>> lines;
        for (int i = 0; i < pathIDs.size() - 1; i++)
        {
            int startPointId = pathIDs[i];
            int endPointId = pathIDs[i + 1];

            int cSId = modelData->_nodedata.nodeidmap.value(startPointId);
            QVector3D sVertex = modelData->getCoordinates(cSId);
            int cEId = modelData->_nodedata.nodeidmap.value(endPointId);
            QVector3D eVertex = modelData->getCoordinates(cEId);
            double totalDistance = sVertex.distanceToPoint(eVertex);

            poss.push_back(sVertex);
            poss.push_back(eVertex);

            std::pair<int, int> linePair = std::make_pair(startPointId, endPointId);
            lines.push_back(linePair);
        }
        // S5 新门面：临时标注=G5 TempMark 单实例整面替换（重复 addObject=整面替换）
        mxr::TempMarkParams tempMark;
        tempMark.nodes = QVector<QVector3D>::fromStdVector(poss);
        tempMark.lines.reserve(int(lines.size()));
        for (const auto& l : lines)
            tempMark.lines.push_back(QPair<int, int>(l.first, l.second));
        postRender->addObject(tempMark);
        postRender->updateHighLightRender();

        //显示ID
        QVector<QVector3D> vertexs;
        const QHash<int, QVector3D> dis = _postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
        QVector3D deformationScale = guidata->_deformFactor;
        for (int id : pathIDs)
        {
            int cid = modelData->_nodedata.nodeidmap.value(id);
            auto nodecoordinate = modelData->getVector(cid);
            QVector3D vertex = nodecoordinate + deformationScale * dis.value(cid);
            vertexs.push_back(vertex);
        }

        QVector<QString> ids;
        auto fontRender = postRend->getFontRender();
        for (int i = 0 ; i < pathIDs.size(); i++)
        {
            ids.append(QString("%1").arg(pathIDs[i]));
        }
        fontRender->appendCommonFont("PathID", vertexs, ids, QVector3D(0, 0, 0));

        postRend->updatePaint();
    }

    void mPostPathPanel::clearhighlight()
    {
        if (!getCVM) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // S5 新门面：清空临时标注=G5 TempMark 整面替换为空（缺省空 nodes/lines，空集重置）
        mxr::TempMarkParams tempMark;
        postRender->addObject(tempMark);
        postRender->updateHighLightRender();

        auto fontRender = postRend->getFontRender();
        fontRender->clearCommonFont("PathID");

        postRend->updatePaint();
    }

	void mPostPathPanel::slot_addItem(QString str)
	{
		int currentRow = _listPathIds->currentRow();
		int count = _listPathIds->count();

		// 列表不为空但没有选中行（理论上不应该发生）
		if (currentRow < 0)
		{
			currentRow = count - 1; // 默认当作最后一行
		}

		if (_btnFront->isChecked())
		{
			// 添加于前：在当前行之前插入
			insertItemAt(currentRow, str);
			_listPathIds->setCurrentRow(currentRow); // 选中新插入的行
		}
		else // 添加于后
		{
			// 添加于后：在当前行之后插入
			insertItemAt(currentRow + 1, str);
			_listPathIds->setCurrentRow(currentRow + 1); // 选中新插入的行
		}
	}

	void mPostPathPanel::slot_removeItem()
	{
		int currentRow = _listPathIds->currentRow();
		if (currentRow >= 0)
		{
			delete _listPathIds->takeItem(currentRow);
			selectLastRow();
		}
		storagePathData();
	}

	void mPostPathPanel::slot_clearItems()
	{
		_listPathIds->clear();
		storagePathData();
	}

	void mPostPathPanel::slot_itemChanged(QListWidgetItem* item)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr) return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		mPostModelData* modeldata = _postDB->getModelData(guidata->_cloudFrame);
		if (modeldata == nullptr)
			return;

		//判断输入id是否合法
		std::set<int> ids = modeldata->getAllNodeIDs();

		QVector<int> inputIds = parseIds(item->text());
		bool isexist = true;
		for (int i = 0; i < inputIds.size(); i++)
		{
			if (ids.count(inputIds[i]) == 0)
				isexist = false;
		}

		if (!isexist)
		{
			mGlobalSignals::getInstance()->outputMessageSig(2, QString("ID不存在，请重新输入!"), true);
			item->setText("");
			_listPathIds->editItem(item);
		}

		storagePathData();
	}

    void mPostPathPanel::hideEvent(QHideEvent* event)
    {
        clearhighlight();
    }

}
