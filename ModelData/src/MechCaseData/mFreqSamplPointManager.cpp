#include "mFreqSamplPointManager.h"
#include "mFreqSamplPointBase.h"
#include "mGlobalSignals.h"



namespace MechCaseData
{
	mFreqSamplPointManager::mFreqSamplPointManager()
	{
		_maxID = 0;
	}

	mFreqSamplPointManager::~mFreqSamplPointManager()
	{
		deleteAllFreqSamplPoint();
	}

	mFreqSamplPointBase* mFreqSamplPointManager::getFreqSamplPointData(int id)
	{
		auto it = _freqSamplPointData.find(id);
		if (it != _freqSamplPointData.end())
		{
			return _freqSamplPointData[id];
		}
		return nullptr;
	}

	mFreqSamplPointBase* mFreqSamplPointManager::getFreqSamplPointData(const QString& name)
	{
		for (auto it : _freqSamplPointData)
		{
			if (it.second != nullptr && it.second->getFreqSamplPointName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mFreqSamplPointManager::getFreqSamplPointNameByID(int id)
	{
		if (_freqSamplPointData.find(id) != _freqSamplPointData.cend())
			return _freqSamplPointData[id]->getFreqSamplPointName();

		return QString();
	}

	int mFreqSamplPointManager::getIDByFreqSamplPointName(QString name)
	{
		for (auto& it : _freqSamplPointData)
		{
			if (it.second && it.second->getFreqSamplPointName() == name)
			{
				return it.first;
			}
		}
		return -1;
	}

	void mFreqSamplPointManager::AddFreqSamplPointData(mFreqSamplPointBase* base)
	{
		_freqSamplPointData.insert({ base->getFreqSamplPointId(), base });
	}

	bool mFreqSamplPointManager::ExistFreqSamplPointName(const QString& name)
	{
		for (auto it : _freqSamplPointData)
		{
			if (it.second != nullptr && it.second->getFreqSamplPointName() == name)
			{
				return true;
			}
		}
		return false;
	}

	void mFreqSamplPointManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _freqSamplPointData)
		{
			if (it.second != nullptr && it.second->getFreqSamplPointName() == oldName)
			{
				it.second->setFreqSamplPointName(newName);
				break;
			}
		}
	}

	int mFreqSamplPointManager::copyFreqSamplPointData(const int& id)
	{
		mFreqSamplPointBase* freqSamplPoint = getFreqSamplPointData(id);
		if (freqSamplPoint == nullptr)
			return 0;

		QString copyName = freqSamplPoint->getFreqSamplPointName() + "-copy";
		while (ExistFreqSamplPointName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mFreqSamplPointBase* copyfreqSamplPoint = freqSamplPoint->copyFreqSamplPoint(createID(), copyName);
		AddFreqSamplPointData(copyfreqSamplPoint);
		return copyfreqSamplPoint->getFreqSamplPointId();
	}

	void mFreqSamplPointManager::deleteAllFreqSamplPoint()
	{
		for (auto it : _freqSamplPointData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_freqSamplPointData.clear();
	}

	void mFreqSamplPointManager::deleteFreqSamplPoint(const int& id)
	{
		if (_freqSamplPointData.find(id) != _freqSamplPointData.cend())
		{
			if (_freqSamplPointData[id] != nullptr)
			{
				delete _freqSamplPointData[id];
			}
			_freqSamplPointData.erase(id);
		}
	}

	std::vector<int> mFreqSamplPointManager::getAllFreqSamplPointID()
	{
		std::vector<int> keys;
		for (auto pair : _freqSamplPointData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mFreqSamplPointManager::getAllFreqSamplPointNames()
	{
		QStringList names;
		for (auto it : _freqSamplPointData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getFreqSamplPointName());
		}
		return names;
	}

	int mFreqSamplPointManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}
}