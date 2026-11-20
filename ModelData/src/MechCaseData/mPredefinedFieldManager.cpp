#include "mPredefinedFieldManager.h"
#include "mPredefinedFieldBase.h"

namespace MechCaseData
{
	mPredefinedFieldManager::mPredefinedFieldManager()
	{
		_maxID = 0;
	}

	mPredefinedFieldManager::~mPredefinedFieldManager()
	{
		_predefinedFieldData.clear();
	}

	std::shared_ptr<mPredefinedFieldBase> mPredefinedFieldManager::getPredefinedFieldData(int id)
	{
		auto it = _predefinedFieldData.find(id);
		if (it != _predefinedFieldData.end())
		{
			return _predefinedFieldData[id];
		}

		return nullptr;
	}

	std::shared_ptr<mPredefinedFieldBase> mPredefinedFieldManager::getPredefinedFieldData(const QString& name)
	{
		for (auto it : _predefinedFieldData)
		{
			if (it.second != nullptr && it.second->getPredefinedFieldName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mPredefinedFieldManager::getPredefinedFieldNameByID(int id)
	{
		if (_predefinedFieldData.find(id) != _predefinedFieldData.cend())
			return _predefinedFieldData[id]->getPredefinedFieldName();

		return QString();
	}

	QString mPredefinedFieldManager::getPredefinedFieldTypeNameByID(int id)
	{
		PredefinedFieldType type;
		if (_predefinedFieldData.find(id) != _predefinedFieldData.cend())
			type = _predefinedFieldData[id]->getPredefinedFieldType();

		QString typeName;

		switch (type)
		{
		case MxFunctions::PREDEFINED_FIELD_INIT_TEMP:
			typeName = "初始温度";
			break;
		case MxFunctions::PREDEFINED_FIELD_TEMP_LOAD:
			typeName = "温度载荷";
			break;
		default:
			typeName = QString();
			break;
		}
		return typeName;
	}

	int mPredefinedFieldManager::getIDByPredefinedFieldName(QString name)
	{
		for (auto& it : _predefinedFieldData)
		{
			if (it.second && it.second->getPredefinedFieldName() == name)
			{
				return it.first;
			}
		}
		return -1;
	}

	void mPredefinedFieldManager::AddPredefinedFieldData(std::shared_ptr<mPredefinedFieldBase> base)
	{
		_predefinedFieldData.insert({ base->getPredefinedFieldId(), base });
	}

	bool mPredefinedFieldManager::ExistPredefinedFieldName(const QString& name)
	{
		for (auto it : _predefinedFieldData)
		{
			if (it.second != nullptr && it.second->getPredefinedFieldName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mPredefinedFieldManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _predefinedFieldData)
		{
			if (it.second != nullptr && it.second->getPredefinedFieldName() == oldName)
			{
				it.second->setPredefinedFieldName(newName);
				break;
			}
		}
	}

	int mPredefinedFieldManager::copyPredefinedFieldData(const int& id)
	{
		std::shared_ptr<mPredefinedFieldBase> predefinedField = getPredefinedFieldData(id);
		if (predefinedField == nullptr)
			return 0;

		QString copyName = predefinedField->getPredefinedFieldName() + "-copy";
		while (ExistPredefinedFieldName(copyName))
		{
			copyName = copyName + "-copy";
		}

		std::shared_ptr<mPredefinedFieldBase> copyPredefinedField = predefinedField->copyPredefinedField(createID(), copyName);
		AddPredefinedFieldData(copyPredefinedField);
		return copyPredefinedField->getPredefinedFieldId();
	}

	void mPredefinedFieldManager::deleteAllPredefinedField()
	{
		_predefinedFieldData.clear();
	}

	void mPredefinedFieldManager::deletePredefinedField(const int& id)
	{
		if (_predefinedFieldData.find(id) != _predefinedFieldData.cend())
		{
			_predefinedFieldData.erase(id);
		}
	}

	std::vector<int> mPredefinedFieldManager::getAllPredefinedFieldID()
	{
		std::vector<int> keys;
		for (auto pair : _predefinedFieldData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mPredefinedFieldManager::getAllPredefinedFieldNames()
	{
		QStringList names;
		for (auto it : _predefinedFieldData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getPredefinedFieldName());
		}
		return names;
	}

	QStringList mPredefinedFieldManager::getAllPredefinedFieldNamesByType(MxFunctions::LoadBoundaryType type)
	{
		QStringList names;
		for (auto it : _predefinedFieldData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getPredefinedFieldType() == type)
			{
				names.append(it.second->getPredefinedFieldName());
			}
		}
		return names;
	}

	int mPredefinedFieldManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

	void mPredefinedFieldManager::differentPredefinedFieldFlag(const std::set<int>& pds, std::vector<int>& ic, std::vector<int>& temp)
	{
		for (int ID : pds)
		{
			if (_predefinedFieldData.find(ID) == _predefinedFieldData.cend())
				continue;

			switch (_predefinedFieldData[ID]->getPredefinedFieldType())
			{
			case MxFunctions::PREDEFINED_FIELD_INIT_TEMP:
				ic.push_back(ID);
				break;
			case MxFunctions::PREDEFINED_FIELD_TEMP_LOAD:
				temp.push_back(ID);
			default:
				break;
			}
		}
	}
}
