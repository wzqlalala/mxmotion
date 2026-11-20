#include "mPanel.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechSetData
{
	mPanel::mPanel(int id, MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mPanel::~mPanel()
	{
	}

	mPanel* mPanel::copySet(const int& id, const QString& name)
	{
		mPanel* set = new mPanel(id, getSetType(), name);
		set->_ids = _ids;
		set->_isOrder = _isOrder;
		set->_elenames = _elenames;
		set->_nodeNames = _nodeNames;
		return set;
	}

	std::vector<RenderPickResultData> mPanel::getNodeData()
	{
		return _ids;
	}

	void mPanel::setNodeData(std::vector<RenderPickResultData> data)
	{
		_ids = data;
	}

	bool mPanel::getOrder()
	{
		return _isOrder;
	}

	void mPanel::setOrder(bool order)
	{
		_isOrder = order;
	}

	int mPanel::getPanelCollectionType() const
	{
		return _panelCollectionType;
	}

	void mPanel::setPanelCollectionType(int type)
	{
		// 0: 等效辐射功率面板集合（仅单元集）
		// 1: 单元/节点集合（可包含单元集和节点集）
		_panelCollectionType = (type == 1) ? 1 : 0;
	}

	bool mPanel::isCollectNode() const
	{
		return _panelCollectionType == 1;
	}

	QList<QString> mPanel::elenames()
	{
		return _elenames;
	}

	void mPanel::setElenames(QList<QString> elenames)
	{
		_elenames = elenames;
	}

	void mPanel::appendEle(QString eleName)
	{
		_elenames.append(eleName);
	}

	void mPanel::deleteEle(QString eleName)
	{
		_elenames.removeAll(eleName);
	}

	void mPanel::clearEles()
	{
		_elenames.clear();
	}

	MxFunctions::BrowserTreeStatus mPanel::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (_panelCollectionType == 1)
		{
			if (_nodeNames.empty())
			{
				info.append({ tip,"集合数据为空" });
				return Tree_Failure;
			}
		}
		else
		{
			if (_elenames.empty())
			{
				info.append({ tip,"集合数据为空" });
				return Tree_Failure;
			}
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}
	
	QList<QString> mPanel::nodeNames() const
	{
		return _nodeNames;
	}
	
	void mPanel::setNodeNames(const QList<QString>& nodeNames)
	{
		_nodeNames = nodeNames;
	}
	
	void mPanel::appendNodeName(const QString& nodeName)
	{
		_nodeNames.append(nodeName);
	}
	
	void mPanel::removeNodeName(const QString& nodeName)
	{
		_nodeNames.removeAll(nodeName);
	}
	
	void mPanel::clearNodeNames()
	{
		_nodeNames.clear();
	}
	
	}
