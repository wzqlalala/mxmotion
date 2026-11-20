#include "mContactManager.h"
#include "mContactBase.h"

#include "mGlobalSignals.h"


namespace MechContactData
{
	mContactManager::mContactManager()
	{
		_maxID = 0;
	}

	mContactManager::~mContactManager()
	{
		for (auto it : _propertyData)
		{
			mContactBase * base = it.second;
			if (base != nullptr)
			{
				delete base;
				base = nullptr;
			}
		}

		_propertyData.clear();
	}

	mContactBase * mContactManager::getContactData(int id)
	{
		auto it = _propertyData.find(id);
		if (it != _propertyData.end())
		{
			return _propertyData[id];
		}

		return nullptr;
	}

	mContactBase* mContactManager::getContactData(const QString& name)
	{
		for (auto it : _propertyData)
		{
			mContactBase* base = it.second;
			if (base != nullptr)
			{
				if (base->getContactName() == name)
				{
					return base;
				}
			}
		}
		return nullptr;
	}

	QString mContactManager::getContactNameByID(int id)
	{
		if (_propertyData.find(id) != _propertyData.cend())
			return _propertyData[id]->getContactName();

		return QString();
	}

	int mContactManager::getContactIdByName(QString name)
	{
		for (auto& it : _propertyData)
		{
			if (it.second && it.second->getContactName() == name)
			{
				return it.first;
			}
		}
		return -1;
	}

	void mContactManager::AddContactData(mContactBase * base)
	{
		_propertyData.insert({ base->getContactId(), base });
		//if (sendSignal)
		//{
		//	MxFunctions::mGlobalSignals::getInstance()->updateModelData(MxFunctions::Physical_Contact);
		//}
	}

	bool mContactManager::ExistContactName(const QString& name)
	{
		for (auto it : _propertyData)
		{
			if (it.second != nullptr && it.second->getContactName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mContactManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _propertyData)
		{
			if (it.second != nullptr && it.second->getContactName() == oldName)
			{
				it.second->setContactName(newName);
				break;
			}
		}
	}

	int mContactManager::copyContactData(const int & id)
	{
		mContactBase* mat = getContactData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getContactName() + "-copy";
		while (ExistContactName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mContactBase*copyContact = mat->copyContact(createID(),copyName);
		AddContactData(copyContact);
		return copyContact->getContactId();
	}

	void mContactManager::deleteAllContact()
	{
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_propertyData.clear();
	}

	void mContactManager::deleteContact(const int& id)
	{
		if (_propertyData.find(id) != _propertyData.cend())
		{
			if (_propertyData[id] != nullptr)
			{
				delete _propertyData[id];
				_propertyData[id] = nullptr;
			}
			_propertyData.erase(id);
		}
	}

	std::vector<int> mContactManager::getAllContactID()
	{
		std::vector<int> keys;
		for (auto pair : _propertyData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mContactManager::getAllContactNames()
	{
		QStringList names;
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getContactName());
		}
		return names;
	}

	QStringList mContactManager::getAllContactNamesByTypes(MxFunctions::ContactType type)
	{
		QStringList names;
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getContactType() == type)
			{
				names.append(it.second->getContactName());
			}
		}
		return names;
	}

	std::vector<int> mContactManager::getAllContactIdsByTypes(MxFunctions::ContactType type)
	{
		std::vector<int> ids;
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getContactType() == type)
			{
				ids.push_back(it.second->getContactId());
			}
		}
		return ids;
	}

	int mContactManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

}