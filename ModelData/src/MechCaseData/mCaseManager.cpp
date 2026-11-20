#include "mCaseManager.h"
#include "mCaseBase.h"
#include "mLoadBoundaryBase.h"



namespace MechCaseData
{

	int mCaseManager::_maxID = 0;
	mCaseManager::mCaseManager()
	{
		
	}

	mCaseManager::~mCaseManager()
	{
		for (auto it : _caseData)
		{
			mCaseBase * base = it.second;
			if (base != nullptr)
			{
				delete base;
				base = nullptr;
			}
		}

		_caseData.clear();
	}

	mCaseBase * mCaseManager::getCaseData(int id)
	{
		auto it = _caseData.find(id);
		if (it != _caseData.end())
		{
			return _caseData[id];
		}

		return nullptr;
	}

	mCaseBase* mCaseManager::getCaseData(const QString& name)
	{
		for (auto it : _caseData)
		{
			if (it.second != nullptr && it.second->getCaseName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mCaseManager::getCaseNameByID(int id)
	{
		if (_caseData.find(id) != _caseData.cend())
			return _caseData[id]->getCaseName();

		return QString();
	}

	QString mCaseManager::getCaseTypeNameByID(int id)
	{
		CaseType type ;
		if (_caseData.find(id) != _caseData.cend())
			type = _caseData[id]->getCaseType();

		QString typeName;

		switch (type)
		{
		case MxFunctions::ImplicitLinearStatic:
			typeName = "线性静力学分析";
			break;
		case MxFunctions::ImplicitNonLinearStatic:
			typeName = "非线性静力学分析";
			break;
		case MxFunctions::ImplicitModal:
			typeName = "模态分析";
			break;
		case MxFunctions::ImplicitResponseSpectrum:
			typeName = "响应谱分析";
			break;
		case MxFunctions::ImplicitRandomVibration:
			typeName = "随机振动分析";
			break;
		case MxFunctions::ImplicitFrequency:
			typeName = "频响分析";
			break;
		case MxFunctions::ASCoupling:
			typeName = "声固耦合分析";
			break;
		case MxFunctions::ImplicitTransient:
			typeName = "瞬态分析";
			break;
		case MxFunctions::ImplicitNonLinearDynamic:
			typeName = "非线性动力学";
			break;
		case MxFunctions::ImplicitLinearBuckling:
			typeName = "线性屈曲分析";
			break;
		case MxFunctions::ImplicitPostBuckling:
			typeName = "后屈曲分析";
			break;
		case MxFunctions::ImplicitSteadyHeat:
			typeName = "稳态传热分析";
			break;
		case MxFunctions::ImplicitTransientHeat:
			typeName = "瞬态传热分析";
			break;
		case MxFunctions::ImplicitStructuralCouplingHeat:
			typeName = "结构耦合传热分析";
			break;
		case MxFunctions::ImplicitGeneric:
			typeName = "超单元分析";
			break;
		default:
			typeName = QString();
			break;
		}

		return typeName;
	}

	int mCaseManager::getIDByCaseName(QString name)
	{
		for (auto& it : _caseData)
		{
			if (it.second && it.second->getCaseName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mCaseManager::AddCaseData(mCaseBase * base)
	{
		_caseData.insert({ base->getCaseId(), base });
		//if (sendSignal)
		//{
		//	MxFunctions::mGlobalSignals::getInstance()->updateModelData(MxFunctions::Physical_Case);
		//}
	}

	bool mCaseManager::ExistCaseName(const QString& name)
	{
		for (auto it : _caseData)
		{
			if (it.second != nullptr && it.second->getCaseName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mCaseManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _caseData)
		{
			if (it.second != nullptr && it.second->getCaseName() == oldName)
			{
				it.second->setCaseName(newName);
				break;
			}
		}
	}

	int mCaseManager::copyCaseData(const int & id)
	{
		mCaseBase* mat = getCaseData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getCaseName() + "-copy";
		while (ExistCaseName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mCaseBase*copyCase = mat->copyCase(createID(), copyName);
		AddCaseData(copyCase);
		return copyCase->getCaseId();
	}

	void mCaseManager::deleteAllCase()
	{
		for (auto it : _caseData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_caseData.clear();
	}

	void mCaseManager::deleteCase(const int& id)
	{
		if (_caseData.find(id) != _caseData.cend())
		{
			if (_caseData[id] != nullptr)
			{
				delete _caseData[id];
				_caseData[id] = nullptr;
			}
			_caseData.erase(id);
		}
	}

	std::vector<int> mCaseManager::getAllCaseID()
	{
		std::vector<int> keys;
		for (auto pair : _caseData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mCaseManager::getAllCaseNames()
	{
		QStringList names;
		for (auto it : _caseData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getCaseName());
		}
		return names;
	}

	QStringList mCaseManager::getAllCaseNamesByTypes(MxFunctions::CaseType type)
	{
		QStringList names;
		for (auto it : _caseData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getCaseType() == type)
			{
				names.append(it.second->getCaseName());
			}
		}
		return names;
	}
	int mCaseManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

}