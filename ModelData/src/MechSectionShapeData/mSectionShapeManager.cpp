#include "mSectionShapeManager.h"
#include <QDateTime>
#include <QDebug>
#include <QList>

namespace MechSectionShapeData
{
  /*  mSectionShapeManager* mSectionShapeManager::_inst = nullptr;*/

    //mSectionShapeManager* mSectionShapeManager::getInstance()
    //{
    //    if (!_inst) _inst = new mSectionShapeManager;
    //    return _inst;
    //}

    mSectionShapeManager::mSectionShapeManager()
    {
        _maxID = 0;
    }
    mSectionShapeManager::~mSectionShapeManager()
    {
        deleteAllSectionShape();
    }
    /* 数据接口 */
    mSectionShapeBase* mSectionShapeManager::getSectionShapeData(int id)
    {
        auto it = _sectionShapeData.find(id);
        if (it != _sectionShapeData.end())
        {
            return _sectionShapeData[id];
        }

        return nullptr;
    }
    
    mSectionShapeBase* mSectionShapeManager::getSectionShapeData(const QString& name)
    {
        for (auto it : _sectionShapeData)
        {
            mSectionShapeBase* base = it.second;
            if (base != nullptr)
            {
                if (base->getSectionShapeName() == name)
                {
                    return base;
                }
            }
        }
        return nullptr;
    }

    QString mSectionShapeManager::getSectionShapeNameByID(int id)
    {
        if (_sectionShapeData.find(id) != _sectionShapeData.cend())
            return _sectionShapeData[id]->getSectionShapeName();

        return QString();
    }

	int mSectionShapeManager::getIDByPropertyShapeName(QString name)
	{
		for (auto& it : _sectionShapeData)
		{
			if (it.second && it.second->getSectionShapeName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}
   
    void mSectionShapeManager::AddSectionShapeData(mSectionShapeBase* sec)
    {
        if (!sec) return;
        _sectionShapeData[sec->getSectionShapeId()] = std::move(sec);
    }
    int mSectionShapeManager::createID()
    {
        _maxID += 1;
        return _maxID;
    }

	bool mSectionShapeManager::ExistSectionShapeName(const QString& name)
	{
		for (auto it : _sectionShapeData)
		{
			if (it.second != nullptr && it.second->getSectionShapeName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mSectionShapeManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _sectionShapeData)
		{
			if (it.second != nullptr && it.second->getSectionShapeName() == oldName)
			{
				it.second->setSectionShapeName(newName);
				break;
			}
		}
	}

	int mSectionShapeManager::copySectionShapeData(const int& id)
	{
		mSectionShapeBase* mat = getSectionShapeData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getSectionShapeName() + "-copy";
		while (ExistSectionShapeName(copyName))
		{
			copyName = copyName + "-copy";
		}
		int _id = createID();
		mSectionShapeBase* copyProperty = mat->copySectionShape(_id, copyName);//10/22还没实现

		copyProperty = mat->clone();
		copyProperty->setSectionShapeId(_id);
		copyProperty->setSectionShapeName(copyName);

		//getSectionShapeData(id);
		AddSectionShapeData(copyProperty);
		return copyProperty->getSectionShapeId();
	}

	void mSectionShapeManager::deleteAllSectionShape()
	{
		for (auto it : _sectionShapeData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_sectionShapeData.clear();
	}

	void mSectionShapeManager::deleteSectionShape(const int& id)
	{
		if (_sectionShapeData.find(id) != _sectionShapeData.cend())
		{
			if (_sectionShapeData[id] != nullptr)
			{
				delete _sectionShapeData[id];
				_sectionShapeData[id] = nullptr;
			}
			_sectionShapeData.erase(id);
		}
	}

	std::vector<int> mSectionShapeManager::getAllSectionShapeID()
	{
		std::vector<int> keys;
		for (auto pair : _sectionShapeData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mSectionShapeManager::getAllSectionShapeNames()
	{
		QStringList names;
		for (auto it : _sectionShapeData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getSectionShapeName());
		}
		return names;
	}

	QStringList mSectionShapeManager::getAllSectionShapeNamesByTypes(MxFunctions::SecType type)
	{
		QStringList names;
		for (auto it : _sectionShapeData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getSectionShapeType() == type)
			{
				names.append(it.second->getSectionShapeName());
			}
		}
		return names;
	}

    int mSectionShapeManager::getMaxID()
    {
        return _maxID;
    }
    int mSectionShapeManager::getCurrentID()
    {
        return _currentID;
    }
    void mSectionShapeManager::setCurrentID(int currentID)
    {
        _currentID = currentID;
    }
} // namespace MechSectionShapeData