#include "mGlobalParam.h"

#include "mGenericParamData.h"
#include "mSolveControlData.h"
#include "mGlobalOutputControlData.h"
namespace MechGlobalParamData
{
	mGlobalParam::mGlobalParam()
	{
		_maxGenericParamID = 0;
		_maxPeakFilterID = 0;
		_genericAssignData = nullptr;
		_genericMatrixData = nullptr;
		_modalContriData = nullptr;
		_mNodeContriData = nullptr;
		_mPanelContriData = nullptr;
		_mTPContriData = nullptr;
		_mERPData = nullptr;
		_genericPartData = nullptr;
		_ascouplingSearchData = nullptr;
		_solveControlData = nullptr;
		_globalOutputControlData = nullptr;
	}

	mGlobalParam::~mGlobalParam()
	{
		if (_genericAssignData)
		{
			delete _genericAssignData;
			_genericAssignData = nullptr;
		}
		if (_genericMatrixData)
		{
			delete _genericMatrixData;
			_genericMatrixData = nullptr;
		}
		if (_modalContriData)
		{
			delete _modalContriData;
			_modalContriData = nullptr;
		}		
		if (_mNodeContriData)
		{
			delete _mNodeContriData;
			_mNodeContriData = nullptr;
		}
		if (_mPanelContriData)
		{
			delete _mPanelContriData;
			_mPanelContriData = nullptr;
		}
		if (_mTPContriData)
		{
			delete _mTPContriData;
			_mTPContriData = nullptr;
		}
		if (_mERPData)
		{
			delete _mERPData;
			_mERPData = nullptr;
		}
		for (int i = 0; i < _peakFilterData.size(); i++)
		{
			auto data = _peakFilterData[i];
			if (data)
			{
				delete data;
			}
		}
		_peakFilterData.clear();
		if (_genericPartData)
		{
			delete _genericPartData;
			_genericPartData = nullptr;
		}
		if (_ascouplingSearchData)
		{
			delete _ascouplingSearchData;
			_ascouplingSearchData = nullptr;
		}
		if (_solveControlData)
		{
			delete _solveControlData;
			_solveControlData = nullptr;
		}
		if (_globalOutputControlData)
		{
			delete _globalOutputControlData;
			_globalOutputControlData = nullptr;
		}
		for (int i = 0; i < _genericParaData.size(); i++)
		{
			auto data = _genericParaData[i];
			if (data)
			{
				delete data;
			}
		}
		_genericParaData.clear();
	}


	mGenericParamData* mGlobalParam::getGenericParamData(int id)
	{
		auto it = _genericParaData.find(id);
		if (it != _genericParaData.end())
		{
			return _genericParaData[id];
		}

		return nullptr;
	}

	mGenericParamData* mGlobalParam::getGenericParamData(const QString& name) const
	{
		for (auto it : _genericParaData)
		{
			mGenericParamData* base = it.second;
			if (base != nullptr)
			{
				if (base->getName() == name)
				{
					return base;
				}
			}
		}
		return nullptr;
	}

	QString mGlobalParam::getGenericParamNameByID(int id) const
	{
		if (_genericParaData.find(id) != _genericParaData.cend())
			return _genericParaData.at(id)->getName();

		return QString();
	}

	int mGlobalParam::getGenericParamIDByName(const QString& name) const
	{
		for (const auto& pair : _genericParaData) {
			if (pair.second && pair.second->getName() == name) 
				return pair.first;
		}
		return -1;
	}

	void mGlobalParam::addGenericParamData(mGenericParamData* base)
	{
		_genericParaData.insert({ base->getId(), base });
	}

	bool mGlobalParam::existGenericParamName(const QString& name) const
	{
		for (auto it : _genericParaData)
		{
			if (it.second != nullptr && it.second->getName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mGlobalParam::resetGenericParamName(const QString& oldName, const QString& newName)
	{
		for (auto it : _genericParaData)
		{
			if (it.second != nullptr && it.second->getName() == oldName)
			{
				it.second->setName(newName);
				break;
			}
		}
	}

	int mGlobalParam::copyGenericParamData(const int& id)
	{
		mGenericParamData* mat = getGenericParamData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getName() + "-copy";
		while (existGenericParamName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mGenericParamData* copy = mat->copyData(createGenericParamID(), copyName);
		addGenericParamData(copy);
		return copy->getId();
	}

	void mGlobalParam::deleteAllGenericParamData()
	{
		for (auto it : _genericParaData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_genericParaData.clear();
	}

	void mGlobalParam::deleteGenericParamData(int id)
	{
		if (_genericParaData.find(id) != _genericParaData.cend())
		{
			if (_genericParaData[id] != nullptr)
			{
				delete _genericParaData[id];
				_genericParaData[id] = nullptr;
			}
			_genericParaData.erase(id);
		}
	}

	std::vector<int> mGlobalParam::getAllGenericParamIDs() const
	{
		std::vector<int> keys;
		for (auto pair : _genericParaData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mGlobalParam::getAllGenericParamNames() const
	{
		QStringList names;
		for (auto it : _genericParaData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getName());
		}
		return names;
	}

	int mGlobalParam::createGenericParamID()
	{
		_maxGenericParamID += 1;
		return _maxGenericParamID;
	}
	void mGlobalParam::deleteGenericAssignData()
	{
		if (_genericAssignData)
		{
			delete _genericAssignData;
			_genericAssignData = nullptr;
		}
	}
	mGenericAssignData* mGlobalParam::createGenericAssignData()
	{
		if (!_genericAssignData)
		{
			_genericAssignData = new mGenericAssignData;
		}
		return _genericAssignData;
	}
	mGenericAssignData* mGlobalParam::getGenericAssignData()
	{
		return _genericAssignData;
	}
	void mGlobalParam::deleteGenericMatrixData()
	{
		if (_genericMatrixData)
		{
			delete _genericMatrixData;
			_genericMatrixData = nullptr;
		}
	}
	mGenericMatrixData* mGlobalParam::createGenericMatrixData()
	{
		if (!_genericMatrixData)
		{
			_genericMatrixData = new mGenericMatrixData;
		}
		return _genericMatrixData;
	}
	mGenericMatrixData* mGlobalParam::getGenericMatrixData()
	{
		return _genericMatrixData;
	}


	/*****************************************贡献量*************************************************/
	//模态贡献量
	void mGlobalParam::deleteModalContriData()
	{
		if (_modalContriData)
		{
			delete _modalContriData;
			_modalContriData = nullptr;
		}
	}
	mModalContriData* mGlobalParam::createModalContriData()
	{
		if (!_modalContriData)
		{
			_modalContriData = new mModalContriData(0, "模态贡献量");
		}
		return _modalContriData;
	}
	mModalContriData* mGlobalParam::getModalContriData()
	{
		return _modalContriData;
	}
	//节点贡献量
	void mGlobalParam::deleteNodeContriData()
	{
		if (_mNodeContriData)
		{
			delete _mNodeContriData;
			_mNodeContriData = nullptr;
		}
	}
	mNodeContriData* mGlobalParam::createNodeContriData()
	{
		if (!_mNodeContriData)
		{
			_mNodeContriData = new mNodeContriData(0, "节点贡献量");
		}
		return _mNodeContriData;
	}
	mNodeContriData* mGlobalParam::getNodeContriData()
	{
		return _mNodeContriData;
	}
	//传递路径贡献量
	void mGlobalParam::deleteTPContriData()
	{
		if (_mTPContriData)
		{
			delete _mTPContriData;
			_mTPContriData = nullptr;
		}
	}
	mTPContriData* mGlobalParam::createTPContriData()
	{
		if (!_mTPContriData)
		{
			_mTPContriData = new mTPContriData(0, "传递路径贡献量");
		}
		return _mTPContriData;
	}
	mTPContriData* mGlobalParam::getTPContriData()
	{
		return _mTPContriData;
	}

	//面板贡献量
	void mGlobalParam::deletePanelContriData()
	{
		if (_mPanelContriData)
		{
			delete _mPanelContriData;
			_mPanelContriData = nullptr;
		}
	}
	mPanelContriData* mGlobalParam::createPanelContriData()
	{
		if (!_mPanelContriData)
		{
			_mPanelContriData = new mPanelContriData(0, "面板贡献量");
		}
		return _mPanelContriData;
	}
	mPanelContriData* mGlobalParam::getPanelContriData()
	{
		return _mPanelContriData;
	}

	//等效辐射功率
	void mGlobalParam::deleteERPData()
	{
		if (_mERPData)
		{
			delete _mERPData;
			_mERPData = nullptr;
		}
	}
	mERPData* mGlobalParam::createERPData()
	{
		if (!_mERPData)
		{
			_mERPData = new mERPData(0, "等效辐射功率");
		}
		return _mERPData;
	}
	mERPData* mGlobalParam::getERPData()
	{
		return _mERPData;
	}

	/*****************************************峰值筛选*************************************************/
	mPeakFilterData* mGlobalParam::getPeakFilterData(int id)
	{
		auto it = _peakFilterData.find(id);
		if (it != _peakFilterData.end())
		{
			return _peakFilterData[id];
		}
		return nullptr;
	}

	mPeakFilterData* mGlobalParam::getPeakFilterData(const QString& name) const
	{
		for (auto it : _peakFilterData)
		{
			mPeakFilterData* base = it.second;
			if (base != nullptr)
			{
				if (base->getName() == name)
				{
					return base;
				}
			}
		}
		return nullptr;
	}

	QString mGlobalParam::getPeakFilterNameByID(int id) const
	{
		if (_peakFilterData.find(id) != _peakFilterData.cend())
			return _peakFilterData.at(id)->getName();
		return QString();
	}

	int mGlobalParam::getPeakFilterIDByName(const QString& name) const
	{
		for (const auto& pair : _peakFilterData) {
			if (pair.second && pair.second->getName() == name)
				return pair.first;
		}
		return -1;
	}

	void mGlobalParam::addPeakFilterData(mPeakFilterData* base)
	{
		_peakFilterData.insert({ base->getId(), base });
	}

	bool mGlobalParam::existPeakFilterName(const QString& name) const
	{
		for (auto it : _peakFilterData)
		{
			if (it.second != nullptr && it.second->getName() == name)
			{
				return true;
			}
		}
		return false;
	}

	void mGlobalParam::resetPeakFilterName(const QString& oldName, const QString& newName)
	{
		for (auto it : _peakFilterData)
		{
			if (it.second != nullptr && it.second->getName() == oldName)
			{
				it.second->setName(newName);
				break;
			}
		}
	}

	int mGlobalParam::copyPeakFilterData(const int& id)
	{
		mPeakFilterData* mat = getPeakFilterData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getName() + "-copy";
		while (existPeakFilterName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mPeakFilterData* copy = mat->copyData(createPeakFilterID(), copyName);
		addPeakFilterData(copy);
		return copy->getId();
	}

	void mGlobalParam::deleteAllPeakFilterData()
	{
		for (auto it : _peakFilterData)
		{
			if (it.second == nullptr)
				continue;
			delete it.second;
			it.second = nullptr;
		}
		_peakFilterData.clear();
	}

	void mGlobalParam::deletePeakFilterData(int id)
	{
		if (_peakFilterData.find(id) != _peakFilterData.cend())
		{
			if (_peakFilterData[id] != nullptr)
			{
				delete _peakFilterData[id];
				_peakFilterData[id] = nullptr;
			}
			_peakFilterData.erase(id);
		}
	}

	std::vector<int> mGlobalParam::getAllPeakFilterIDs() const
	{
		std::vector<int> keys;
		for (auto pair : _peakFilterData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mGlobalParam::getAllPeakFilterNames() const
	{
		QStringList names;
		for (auto it : _peakFilterData)
		{
			if (it.second == nullptr)
				continue;
			names.append(it.second->getName());
		}
		return names;
	}

	int mGlobalParam::createPeakFilterID()
	{
		_maxPeakFilterID += 1;
		return _maxPeakFilterID;
	}

	void mGlobalParam::deleteGenericPartData()
	{
		if (_genericPartData)
		{
			delete _genericPartData;
			_genericPartData = nullptr;
		}
	}
	mGenericPartData* mGlobalParam::createGenericPartData()
	{
		if (!_genericPartData)
		{
			_genericPartData = new mGenericPartData;
		}
		return _genericPartData;
	}
	mGenericPartData* mGlobalParam::getGenericPartData()
	{
		return _genericPartData;
	}
	void mGlobalParam::deleteASCouplingSearchData()
	{
		if (_ascouplingSearchData)
		{
			delete _ascouplingSearchData;
			_ascouplingSearchData = nullptr;
		}
	}
	mASCouplingSearchData* mGlobalParam::createASCouplingSearchData()
	{
		if (!_ascouplingSearchData)
		{
			_ascouplingSearchData = new mASCouplingSearchData;
		}
		return _ascouplingSearchData;
	}
	mASCouplingSearchData* mGlobalParam::getASCouplingSearchData()
	{
		return _ascouplingSearchData;
	}

	void mGlobalParam::deleteSolveControlData()
	{
		if (_solveControlData)
		{
			delete _solveControlData;
			_solveControlData = nullptr;
		}
	}
	mSolveControlData* mGlobalParam::createSolveControlData()
	{
		if (!_solveControlData)
		{
			_solveControlData = new mSolveControlData;
		}
		return _solveControlData;
	}
	mSolveControlData* mGlobalParam::getSolveControlData()
	{
		return _solveControlData;
	}

	void mGlobalParam::deleteGlobalOutputControlData()
	{
		if (_globalOutputControlData)
		{
			delete _globalOutputControlData;
			_globalOutputControlData = nullptr;
		}
	}
	mGlobalOutputControlData* mGlobalParam::createGlobalOutputControlData()
	{
		if (!_globalOutputControlData)
		{
			_globalOutputControlData = new mGlobalOutputControlData;
		}
		return _globalOutputControlData;
	}
	mGlobalOutputControlData* mGlobalParam::getGlobalOutputControlData()
	{
		return _globalOutputControlData;
	}
}