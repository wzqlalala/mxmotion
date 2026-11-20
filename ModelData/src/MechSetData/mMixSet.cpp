#include "mMixSet.h"

using namespace MxFunctions;
namespace MechSetData
{
	mMixSet::mMixSet(int id, MxFunctions::SetType type,const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mMixSet::~mMixSet()
	{
	}

	mSetBase* mMixSet::copySet(const int& id, const QString& name)
	{
		mMixSet* set = new mMixSet(id,getSetType(), name);
		set->_ids = _ids;
		return set;
	}

	std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mMixSet::getMixSetData()
	{
		return _ids;
	}

	void mMixSet::setMixSetData(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data)
	{
		_ids = data;
	}

	MxFunctions::BrowserTreeStatus mMixSet::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		return Tree_Yes;
	}
	void mMixSet::replaceMixSetData(MxFunctions::PickDataType pickDataType, std::vector<RenderPickResultData> data, std::vector<int> ids)
	{
		_ids.clear();
		std::vector<RenderPickResultData> res;
		if (pickDataType >= PickDataType::GeoPoint && pickDataType <= PickDataType::GeoSolid || pickDataType == PickDataType::Part)
		{
			for (auto id : ids)
			{
				res.push_back(RenderPickResultData(id));
			}
		}
		else
		{
			res = data;
		}

		_ids[pickDataType] = res;
	}
	void mMixSet::getMixSetData(MxFunctions::PickDataType& pickDataType, std::vector<RenderPickResultData>& data, std::vector<int>& ids)
	{
		pickDataType = PickDataType::None;
		for (auto iter : _ids)
		{
			if (!iter.second.empty())
			{
				if ((iter.first >= PickDataType::GeoPoint && iter.first <= PickDataType::GeoSolid) || iter.first == PickDataType::Part)
				{
					ids.reserve(iter.second.size());
					for (auto resultData : iter.second)
					{
						ids.push_back(resultData.result_id);
					}
				}
				else
				{
					data = iter.second;
				}
				pickDataType = iter.first;
			}
			break;
		}
	}
}
