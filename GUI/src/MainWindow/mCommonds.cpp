#include "mCommonds.h"
#include "mCommondSystem.h"

namespace MechMainWindow
{
	mCommondBase::mCommondBase(CommondType commond):
		_commond(commond)
	{

	}

	mCommondBase::~mCommondBase()
	{

	}

	std::unordered_map <mCommondBase::CommondType, std::function<void()>>& mCommondBase::globalCallbacks()
	{
		static std::unordered_map <mCommondBase::CommondType, std::function<void()>> _callBackFuncs; 
		return _callBackFuncs;
	}

	void mCommondBase::regCallBackFunc(const mCommondBase::CommondType& commond, std::function<void()>func)
	{

		if (globalCallbacks().find(commond) != globalCallbacks().end())
		{
			return;
		}
		globalCallbacks()[commond] = std::move(func);
	}

	void mCommondBase::callBackFunc(const mCommondBase::CommondType& commond)
	{
		if (globalCallbacks().find(commond) == globalCallbacks().end())
		{
			return;
		}
		globalCallbacks()[commond]();
	}

	/********************************************************材料**********************************************************/

	mCommondMaterial::mCommondMaterial(CommondType commond):
		mCommondBase(commond)
	{
	}

	mCommondMaterial::~mCommondMaterial()
	{

	}

	void mCommondMaterial::storeSnap(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
	{
		_matData = std::move(mat);
	}

	std::shared_ptr<MechMaterialData::mMaterialBase> mCommondMaterial::getSnap()
	{
		return _matData;
	}

	void mCommondMaterial::switchState()
	{
		callBackFunc(_commond);
		switch (_commond)
		{
		case CommondType::MAT_CREATEUNDO:
			_commond = CommondType::MAT_CREATEREDO;
			//emit mCommondSystem::getInstance()->sg_materialCreateUndo();
			break;
		case CommondType::MAT_CREATEREDO:
			_commond = CommondType::MAT_CREATEUNDO;
			//emit mCommondSystem::getInstance()->sg_materialCreateRedo();
			break;
		case CommondType::MAT_DELUNDO:
			_commond = CommondType::MAT_DELREDO;
			break;
		case CommondType::MAT_DELREDO:
			_commond = CommondType::MAT_DELUNDO;
			break;
		default:
			break;
		}
	}

	mCommondMaterialProp::mCommondMaterialProp(CommondType commond) :
		mCommondBase(commond)
	{
	}

	mCommondMaterialProp::~mCommondMaterialProp()
	{

	}

	void mCommondMaterialProp::storeProp(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& prop)
	{
		_prop = prop;
	}

	QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> mCommondMaterialProp::getProp()
	{
		return _prop;
	}

	void mCommondMaterialProp::storeMatID(const int& matid)
	{
		_matID = matid;
	}

	int mCommondMaterialProp::getMatID()
	{
		return _matID;
	}

	void mCommondMaterialProp::storeMatType(const MxFunctions::MaterialType& mattype)
	{
		_mattype = mattype;
	}

	MxFunctions::MaterialType mCommondMaterialProp::getMatType()
	{
		return _mattype;
	}

	void mCommondMaterialProp::switchState()
	{
		callBackFunc(_commond);
		switch (_commond)
		{
		case CommondType::MAT_PROPADDUNDO:
			_commond = CommondType::MAT_PROPADDREDO;
			break;
		case CommondType::MAT_PROPADDREDO:
			_commond = CommondType::MAT_PROPADDUNDO;
			break;
		case CommondType::MAT_PROPDELUNDO:
			_commond = CommondType::MAT_PROPDELREDO;
			break;
		case CommondType::MAT_PROPDELREDO:
			_commond = CommondType::MAT_PROPDELUNDO;
			break;
		case CommondType::MAT_PROPMODUNDO:
			_commond = CommondType::MAT_PROPMODREDO;
			break;
		case CommondType::MAT_PROPMODREDO:
			_commond = CommondType::MAT_PROPMODUNDO;
			break;
		default:
			break;
		}
	}

	mCommondLoad::mCommondLoad(CommondType commond) :
		mCommondBase(commond)
	{

	}

	mCommondLoad::~mCommondLoad()
	{

	}

	void mCommondLoad::switchState()
	{
		callBackFunc(_commond);
		switch (_commond)
		{
		case CommondType::LOAD_CREATEUNDO:
			_commond = CommondType::LOAD_CREATEREDO;
			break;
		case CommondType::LOAD_CREATEREDO:
			_commond = CommondType::LOAD_CREATEUNDO;
			break;
		case CommondType::LOAD_DELUNDO:
			_commond = CommondType::LOAD_DELREDO;
			break;
		case CommondType::LOAD_DELREDO:
			_commond = CommondType::LOAD_DELUNDO;
			break;
		default:
			break;
		}
	}

	mCommondBoundary::mCommondBoundary(CommondType commond) :
		mCommondBase(commond)
	{

	}

	mCommondBoundary::~mCommondBoundary()
	{

	}

	void mCommondBoundary::switchState()
	{
		callBackFunc(_commond);
		switch (_commond)
		{
		case CommondType::BOUNDARY_CREATEUNDO:
			_commond = CommondType::BOUNDARY_CREATEREDO;
			break;
		case CommondType::BOUNDARY_CREATEREDO:
			_commond = CommondType::BOUNDARY_CREATEUNDO;
			break;
		case CommondType::BOUNDARY_DELUNDO:
			_commond = CommondType::BOUNDARY_DELREDO;
			break;
		case CommondType::BOUNDARY_DELREDO:
			_commond = CommondType::BOUNDARY_DELUNDO;
			break;
		default:
			break;
		}
	}


	mCommondConnector::mCommondConnector(CommondType commond) :
		mCommondBase(commond)
	{

	}

	mCommondConnector::~mCommondConnector()
	{

	}

	void mCommondConnector::switchState()
	{
		callBackFunc(_commond);
		switch (_commond)
		{
		case CommondType::CONNECTOR_CREATEUNDO:
			_commond = CommondType::CONNECTOR_CREATEREDO;
			break;
		case CommondType::CONNECTOR_CREATEREDO:
			_commond = CommondType::CONNECTOR_CREATEUNDO;
			break;
		case CommondType::CONNECTOR_DELUNDO:
			_commond = CommondType::CONNECTOR_DELREDO;
			break;
		case CommondType::CONNECTOR_DELREDO:
			_commond = CommondType::CONNECTOR_DELUNDO;
			break;
		default:
			break;
		}
	}
}