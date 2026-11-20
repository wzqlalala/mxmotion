#pragma execution_character_set("utf-8")

// =============================================================================
// mCurveLayerManager —— 曲线图层管理（实现），接口语义见 mCurveLayerManager.h
// 设计依据 openspec add-template-curve D5（文档外轻量注册表 + 复用 kVisible）
// =============================================================================

#include "mCurveLayerManager.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"

//QT
#include <QUndoStack>
#include <QSet>

namespace PostCurve
{
	namespace
	{
		//存活成员裁剪：仅保留文档中仍存在且类型相符的 id（曲线/基准线被删即出列）
		QList<int> liveIds(const QList<int>& ids, CurveNodeType type)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			QList<int> out;
			for (int id : ids)
			{
				if (doc->node(id) && doc->nodeType(id) == type)
					out << id;
			}
			return out;
		}

		//存活页面裁剪（页面被删即出列）
		QVector<int> livePages(const QVector<int>& ids)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			QVector<int> out;
			for (int id : ids)
			{
				if (doc->node(id) && doc->nodeType(id) == CurveNodeType::Page)
					out << id;
			}
			return out;
		}

		//基准线数据全等（D10 判重字段口径）：除 visible/layer 外全字段比对
		bool baselineDataEqual(const CurveBaselineData& a, const CurveBaselineData& b)
		{
			return a.label == b.label && a.orientation == b.orientation
				&& a.value == b.value && a.color == b.color
				&& a.lineStyle == b.lineStyle && a.lineWidth == b.lineWidth
				&& a.labelFont == b.labelFont && a.labelPos == b.labelPos
				&& a.link == b.link;
		}
	}

	mCurveLayerManager* mCurveLayerManager::_instance = nullptr;

	mCurveLayerManager* mCurveLayerManager::getInstance()
	{
		if (!_instance)
			_instance = new mCurveLayerManager;
		return _instance;
	}

	mCurveLayerManager::mCurveLayerManager(QObject* parent)
		: QObject(parent)
	{
	}

	int mCurveLayerManager::registerLayer(const QString& name, const QList<int>& curveIds,
		const QList<int>& baselineIds, const QVector<int>& pageIds)
	{
		Rec rec;
		rec.id = _nextLayerId++;
		rec.name = name;
		rec.curveIds = curveIds;
		rec.baselineIds = baselineIds;
		rec.pageIds = pageIds;
		_layers.append(rec);
		return rec.id;
	}

	int mCurveLayerManager::mergeDatumLayer(const QList<int>& newBaselineIds,
		const QVector<int>& pageIds)
	{
		const QVector<int> pageVec = livePages(pageIds);
		if (pageVec.isEmpty())
			return 0;
		CurveDocument* doc = mCurveDocController::getInstance()->document();

		//整理范围（v3 收窄）：同组（同起始页）上一次应用注册的普通曲线层——
		//层序即应用序，取登记序最新的一个；更早普通层的基准线不迁出
		Rec* prevNormal = nullptr;
		for (Rec& rec : _layers)
		{
			if (rec.datumNo > 0)
				continue;
			const QVector<int> pages = livePages(rec.pageIds);
			if (!pages.isEmpty() && pages.first() == pageVec.first())
				prevNormal = &rec;
		}

		//待归置基准线：上一次普通层落在本次页面上的存活成员（迁出按落页过滤，
		//页面外成员不动）+ 本次新建
		QList<int> pending;
		if (prevNormal)
		{
			QSet<int> onPage;
			for (int pid : pageVec)
			{
				for (int chartId : doc->childNodeIds(pid))
				{
					if (doc->nodeType(chartId) != CurveNodeType::Chart)
						continue;
					for (int bid : doc->childNodeIds(chartId))
					{
						if (doc->nodeType(bid) == CurveNodeType::Baseline)
							onPage.insert(bid);
					}
				}
			}
			QList<int> stay;
			for (int bid : prevNormal->baselineIds)
			{
				if (onPage.contains(bid) && doc->node(bid)
					&& doc->nodeType(bid) == CurveNodeType::Baseline)
					pending << bid;
				else
					stay << bid;
			}
			if (stay.size() != prevNormal->baselineIds.size())
				prevNormal->baselineIds = stay;
		}
		for (int bid : newBaselineIds)
		{
			if (!pending.contains(bid))
				pending << bid;
		}

		//逐条按数据等价类归置（v3）：全局（跨组）找已有基准线层中含数据全等
		//成员者——命中则并入（页集合并集；起始页不同=跨组，置 ownGroup 升独立
		//页面节点，显示号不变）；未命中则本组新建（同组可多个基准线层）
		int lastTouched = 0;
		for (int bid : pending)
		{
			if (!doc->node(bid) || doc->nodeType(bid) != CurveNodeType::Baseline)
				continue;
			const CurveBaselineData spec = doc->node(bid)->baseline;
			Rec* target = nullptr;
			for (Rec& rec : _layers)
			{
				if (rec.datumNo <= 0)
					continue;
				for (int mid : rec.baselineIds)
				{
					const CurveNode* n = doc->node(mid);
					if (n && doc->nodeType(mid) == CurveNodeType::Baseline
						&& baselineDataEqual(n->baseline, spec))
					{
						target = &rec;
						break;
					}
				}
				if (target)
					break;
			}
			if (!target)
			{
				Rec rec;
				rec.id = _nextLayerId++;          //内部 id 单一序列（防查找碰撞）
				rec.datumNo = ++_nextDatumNo;     //显示号独立分序（D10）
				rec.name = QStringLiteral("Datum Lines and Reference Curves");
				rec.baselineIds << bid;
				rec.pageIds = pageVec;
				_layers.append(rec);
				lastTouched = rec.id;
				continue;
			}
			if (!target->baselineIds.contains(bid))
				target->baselineIds << bid;
			for (int pid : pageVec)
			{
				if (!target->pageIds.contains(pid))
					target->pageIds.append(pid);
			}
			const QVector<int> targetPages = livePages(target->pageIds);
			if (targetPages.isEmpty() || targetPages.first() != pageVec.first())
				target->ownGroup = true;   //跨组合并：树上升独立页面分组节点
			lastTouched = target->id;
		}
		return lastTouched;
	}

	int mCurveLayerManager::findEquivalentBaseline(const QVector<int>& pageIds,
		const CurveBaselineData& spec) const
	{
		const QVector<int> pageVec = livePages(pageIds);
		if (pageVec.isEmpty())
			return 0;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		for (int pid : pageVec)
		{
			for (int chartId : doc->childNodeIds(pid))
			{
				if (doc->nodeType(chartId) != CurveNodeType::Chart)
					continue;
				for (int bid : doc->childNodeIds(chartId))
				{
					if (doc->nodeType(bid) != CurveNodeType::Baseline)
						continue;
					const CurveNode* n = doc->node(bid);
					if (n && baselineDataEqual(n->baseline, spec))
						return bid;
				}
			}
		}
		return 0;
	}

	QList<mCurveLayerManager::LayerInfo> mCurveLayerManager::layers() const
	{
		QList<LayerInfo> out;
		for (const Rec& rec : _layers)
		{
			LayerInfo info;
			info.id = rec.id;
			info.name = rec.name;
			info.curveIds = liveIds(rec.curveIds, CurveNodeType::Curve);
			info.baselineIds = liveIds(rec.baselineIds, CurveNodeType::Baseline);
			info.pageIds = livePages(rec.pageIds);
			info.datumNo = rec.datumNo;
		info.ownGroup = rec.ownGroup;
			if (info.curveIds.isEmpty() && info.baselineIds.isEmpty())
				continue;   //成员全空=层已随文档删除消亡（基准线层只看基准线成员）
			out << info;
		}
		return out;
	}

	int mCurveLayerManager::layerOfCurve(int curveId) const
	{
		//层序倒查无必要：曲线至多归属一层（每次应用各自登记），正查即回
		for (const Rec& rec : _layers)
		{
			if (liveIds(rec.curveIds, CurveNodeType::Curve).contains(curveId))
				return rec.id;
		}
		return -1;
	}

	QList<mCurveLayerManager::GroupInfo> mCurveLayerManager::validGroups() const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();

		//页 id -> 位置序（1 基，按根下 Page 顺序；label 显示与视图页序同源）
		QHash<int, int> pageNo;
		const QList<int> rootPages = doc->childNodeIds(doc->rootNodeId());
		for (int i = 0; i < rootPages.size(); ++i)
			pageNo.insert(rootPages.at(i), i + 1);

		QList<GroupInfo> out;
		for (const Rec& rec : _layers)
		{
			if (rec.datumNo > 0)
				continue;   //基准线层不作叠加/替换目标组（D10）
			const QVector<int> pages = livePages(rec.pageIds);
			if (pages.isEmpty())
				continue;   //起始页被删=组失效（"选择起始页面"不出现）
			GroupInfo g;
			g.layerId = rec.id;
			g.startPageId = pages.first();
			g.label = QStringLiteral("%1 (Page %2: %3)").arg(rec.name)
				.arg(pageNo.value(g.startPageId, 0))
				.arg(doc->nodeName(g.startPageId));
			out << g;
		}
		return out;
	}

	bool mCurveLayerManager::renameLayer(int layerId, const QString& name)
	{
		for (Rec& rec : _layers)
		{
			if (rec.id == layerId)
			{
				rec.name = name;
				return true;
			}
		}
		return false;
	}

	bool mCurveLayerManager::removeLayer(int layerId, QString* err)
	{
		mCurveDocController* controller = mCurveDocController::getInstance();
		for (int i = 0; i < _layers.size(); ++i)
		{
			if (_layers.at(i).id != layerId)
				continue;
			const QList<int> victims = liveIds(_layers.at(i).curveIds, CurveNodeType::Curve)
				+ liveIds(_layers.at(i).baselineIds, CurveNodeType::Baseline);
			if (!victims.isEmpty())
			{
				//单个撤销宏：跨页成员一次 Ctrl+Z 整体撤销（D5）
				controller->undoStack()->beginMacro(QStringLiteral("删除图层"));
				for (int id : victims)
					controller->removeNode(id);
				controller->undoStack()->endMacro();
			}
			_layers.removeAt(i);   //删除即出列：撤销恢复节点但层不回收（v1，重应用重建）
			return true;
		}
		if (err)
			*err = QStringLiteral("图层不存在或已失效");
		return false;
	}

	void mCurveLayerManager::setCurvesVisible(int layerId, bool visible)
	{
		mCurveDocController* controller = mCurveDocController::getInstance();
		for (const Rec& rec : _layers)
		{
			if (rec.id != layerId)
				continue;
			for (int id : liveIds(rec.curveIds, CurveNodeType::Curve))
				controller->setNodeProperty(id, CurveProp::kVisible, visible);
			//基准线同写 kVisible（D10：层显隐=曲线+基准线，与删除口径对齐）
			for (int id : liveIds(rec.baselineIds, CurveNodeType::Baseline))
				controller->setNodeProperty(id, CurveProp::kVisible, visible);
			return;
		}
	}

	void mCurveLayerManager::reset()
	{
		_layers.clear();
		_nextLayerId = 1;   //会话重置：层 id 与基准线层显示号一并从 1 重新起计（D10）
		_nextDatumNo = 0;
	}
}
