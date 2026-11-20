#include "mFieldManager.h"
#include "mFieldBase.h"
#include "mDiscreteField.h"
#include "mAnalysisField.h"
#include <vector>

namespace MechDiscreteFieldData
{
	mFieldManager::mFieldManager()
	{
		_maxID = 0;
	}

	mFieldManager::~mFieldManager()
	{
		for (auto it : _fieldDataMap)
		{
			mFieldBase* base = it.second;
			if (base != nullptr)
			{
				delete base;
				base = nullptr;
			}
		}

		_fieldDataMap.clear();
	}

	mFieldBase* mFieldManager::getFieldData(int id)
	{
		auto it = _fieldDataMap.find(id);
		if (it != _fieldDataMap.end())
		{
			return it->second;
		}

		return nullptr;
	}

	mDiscreteField* mFieldManager::getDiscreteFieldData(int id)
	{
		return dynamic_cast<mDiscreteField*>(getFieldData(id));
	}

	mDiscreteField* mFieldManager::getDiscreteFieldData(const QString& name)
	{
		for (auto it : _fieldDataMap)
		{
			mDiscreteField* field = dynamic_cast<mDiscreteField*>(it.second);
			if (field != nullptr && field->getDiscreteSpaceName() == name)
			{
				return field;
			}
		}
		return nullptr;
	}

	mAnalysisField* mFieldManager::getAnalysisFieldData(int id)
	{
		return dynamic_cast<mAnalysisField*>(getFieldData(id));
	}

	mAnalysisField* mFieldManager::getAnalysisFieldData(const QString& name)
	{
		for (auto it : _fieldDataMap)
		{
			mAnalysisField* field = dynamic_cast<mAnalysisField*>(it.second);
			if (field != nullptr && field->getAnalysisFieldName() == name)
			{
				return field;
			}
		}
		return nullptr;
	}

	QString mFieldManager::getFieldNameByID(int id)
	{
		auto it = _fieldDataMap.find(id);
		if (it != _fieldDataMap.cend() && it->second != nullptr)
			return it->second->getFieldName();

		return QString();
	}

	int mFieldManager::getIDByFieldName(const QString& name, const MxFunctions::FieldType* fieldType)
	{
		for (auto& it : _fieldDataMap)
		{
			if (it.second == nullptr || it.second->getFieldName() != name)
				continue;

			if (fieldType == nullptr || it.second->getFieldType() == *fieldType)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mFieldManager::addFieldData(mFieldBase* base)
	{
		_fieldDataMap.insert({ base->getId(), base });
	}

	bool mFieldManager::existFieldName(const QString& name, MxFunctions::FieldType fieldType)
	{
		for (auto it : _fieldDataMap)
		{
			if (it.second != nullptr && it.second->getFieldType() == fieldType
				&& it.second->getFieldName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mFieldManager::resetName(const QString& oldName, const QString& newName, MxFunctions::FieldType fieldType)
	{
		for (auto it : _fieldDataMap)
		{
			if (it.second != nullptr && it.second->getFieldType() == fieldType
				&& it.second->getFieldName() == oldName)
			{
				it.second->setFieldName(newName);
				break;
			}
		}
	}

	int mFieldManager::copyFieldData(const int& id)
	{
		mFieldBase* field = getFieldData(id);
		if (field == nullptr)
			return 0;

		MxFunctions::FieldType fieldType = field->getFieldType();

		QString copyName = field->getFieldName() + "-copy";
		while (existFieldName(copyName, fieldType))
		{
			copyName = copyName + "-copy";
		}

		mFieldBase* copyField = field->copyField();
		copyField->setId(createID());
		copyField->setFieldName(copyName);
		addFieldData(copyField);
		return copyField->getId();
	}

	void mFieldManager::deleteAllField(MxFunctions::FieldType fieldType)
	{
		for (auto it = _fieldDataMap.begin(); it != _fieldDataMap.end();)
		{
			if (it->second != nullptr && it->second->getFieldType() == fieldType)
			{
				delete it->second;
				it = _fieldDataMap.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void mFieldManager::deleteField(const int& id)
	{
		auto it = _fieldDataMap.find(id);
		if (it != _fieldDataMap.end())
		{
			if (it->second != nullptr)
			{
				delete it->second;
				it->second = nullptr;
			}
			_fieldDataMap.erase(it);
		}
	}

	std::vector<int> mFieldManager::getAllFieldID(MxFunctions::FieldType fieldType)
	{
		std::vector<int> keys;
		for (auto pair : _fieldDataMap)
		{
			if (pair.second != nullptr && pair.second->getFieldType() == fieldType)
				keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mFieldManager::getAllFieldNames(MxFunctions::FieldType fieldType)
	{
		QStringList names;
		for (auto it : _fieldDataMap)
		{
			if (it.second == nullptr || it.second->getFieldType() != fieldType)
				continue;

			names.append(it.second->getFieldName());
		}
		return names;
	}

	int mFieldManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}
}
