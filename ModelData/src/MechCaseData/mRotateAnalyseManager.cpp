#include "mRotateAnalyseManager.h"
#include "mRoatateAnalyseBase.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mRotateAnalyseManager::mRotateAnalyseManager()
	{
		_maxID = 0;
	}

	mRotateAnalyseManager::~mRotateAnalyseManager()
	{
		_rotateAnalyseData.clear();
	}

	std::shared_ptr<mRoatateAnalyseBase> mRotateAnalyseManager::getRotateAnalyseData(int id)
	{
		auto it = _rotateAnalyseData.find(id);
		if (it != _rotateAnalyseData.end())
		{
			return _rotateAnalyseData[id];
		}

		return nullptr;
	}

	std::shared_ptr<mRoatateAnalyseBase> mRotateAnalyseManager::getRotateAnalyseData(const QString& name)
	{
		for (auto it : _rotateAnalyseData)
		{
			if (it.second != nullptr && it.second->getRotateAnalyseName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mRotateAnalyseManager::getRotateAnalyseNameByID(int id)
	{
		if (_rotateAnalyseData.find(id) != _rotateAnalyseData.cend())
			return _rotateAnalyseData[id]->getRotateAnalyseName();

		return QString();
	}

	QString mRotateAnalyseManager::getRotateAnalyseTypeNameByID(int id)
	{
		RotateAnalysyType type;
		if (_rotateAnalyseData.find(id) != _rotateAnalyseData.cend())
			type = _rotateAnalyseData[id]->getRotateAnalyseType();

		QString typeName;

		switch (type)
		{
		case MxFunctions::ROTATIONAL_ACC:
			typeName = "旋转加速度";
			break;
		case MxFunctions::ROTATIONAL_SPEED:
			typeName = "旋转速度";
			break;
		case MxFunctions::ROTATIONAL_FORCE:
			typeName = "旋转力";
			break;
		default:
			typeName = QString();
			break;
		}
		return typeName;
	}

	int mRotateAnalyseManager::getIDByRotateAnalyseName(QString name)
	{
		for (auto& it : _rotateAnalyseData)
		{
			if (it.second && it.second->getRotateAnalyseName() == name)
			{
				return it.first;
			}
		}
		return -1;
	}

	void mRotateAnalyseManager::AddRotateAnalyseData(std::shared_ptr<mRoatateAnalyseBase> base)
	{
		_rotateAnalyseData.insert({ base->getRotateAnalyseId(), base });
	}

	bool mRotateAnalyseManager::ExistRotateAnalyseName(const QString& name)
	{
		for (auto it : _rotateAnalyseData)
		{
			if (it.second != nullptr && it.second->getRotateAnalyseName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mRotateAnalyseManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _rotateAnalyseData)
		{
			if (it.second != nullptr && it.second->getRotateAnalyseName() == oldName)
			{
				it.second->setRotateAnalyseName(newName);
				break;
			}
		}
	}

	int mRotateAnalyseManager::copyRotateAnalyseData(const int& id)
	{
		std::shared_ptr<mRoatateAnalyseBase> rotateAnalyse = getRotateAnalyseData(id);
		if (rotateAnalyse == nullptr)
			return 0;

		QString copyName = rotateAnalyse->getRotateAnalyseName() + "-copy";
		while (ExistRotateAnalyseName(copyName))
		{
			copyName = copyName + "-copy";
		}

		std::shared_ptr<mRoatateAnalyseBase> copyRotateAnalyse = rotateAnalyse->copyRotateAnalyse(createID(), copyName);
		AddRotateAnalyseData(copyRotateAnalyse);
		return copyRotateAnalyse->getRotateAnalyseId();
	}

	void mRotateAnalyseManager::deleteAllRotateAnalyse()
	{
		_rotateAnalyseData.clear();
	}

	void mRotateAnalyseManager::deleteRotateAnalyse(const int& id)
	{
		if (_rotateAnalyseData.find(id) != _rotateAnalyseData.cend())
		{
			_rotateAnalyseData.erase(id);
		}
	}

	std::vector<int> mRotateAnalyseManager::getAllRotateAnalyseID()
	{
		std::vector<int> keys;
		for (auto pair : _rotateAnalyseData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mRotateAnalyseManager::getAllRotateAnalyseNames()
	{
		QStringList names;
		for (auto it : _rotateAnalyseData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getRotateAnalyseName());
		}
		return names;
	}

	QStringList mRotateAnalyseManager::getAllRotateAnalyseNamesByType(MxFunctions::RotateAnalysyType type)
	{
		QStringList names;
		for (auto it : _rotateAnalyseData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getRotateAnalyseType() == type)
			{
				names.append(it.second->getRotateAnalyseName());
			}
		}
		return names;
	}

	int mRotateAnalyseManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}
}
