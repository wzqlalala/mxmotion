#include "mSpring.h"
#include "mNodeValidor.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mSpring::mSpring(int id, PropertyType type, const QString& name) :mPropertyBase(id, type, name)
	{
		_k = "x";

		_nonlinear = "线性";

		_value = "";

		_K = "";

		_GE = "";

		_S = "";

		switch (type)
		{
		case Elas1Property:
			_springCategory = "单自由度弹簧";
			break;
		case Spring1Property:
			_springCategory = "六自由度接地弹簧";
			break;
		case Spring2Property:
			_springCategory = "六自由度弹簧";
			break;
		case SpringAProperty:
			_springCategory = "三自由度弹簧";
			break;
		default:
			_springCategory = "六自由度接地弹簧";
			break;
		}
	}

	mSpring::~mSpring()
	{

	}
	mSpring* MechPropertyData::mSpring::copyProperty(const int& id, const QString& name)
	{
		mSpring* caseData = new mSpring(id, getPropertyType(), name);
		caseData->_k = _k;
		caseData->_nonlinear = _nonlinear;
		caseData->_value = _value;
		caseData->_luCurve = _luCurve;
		caseData->_K = _K;
		caseData->_GE = _GE;
		caseData->_S = _S;
		caseData->_springCategory = _springCategory;
		return caseData;
	}

	MxFunctions::BrowserTreeStatus mSpring::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		PropertyType type = getPropertyType();
		if (type == Elas1Property)
		{
			checkRequiredPositive(_K, "弹性属性", info, status);
			checkOptionalPositive(_GE, "阻尼系数", info, status);
			checkOptionalPositive(_S, "应力系数", info, status);
		}
		else
		{
			if (_nonlinear == "线性")
			{
				checkRequiredPositive(_value, "常刚度", info, status);
			}
			else
			{
				if (_luCurve.isEmpty())
				{
					info.append({ "力-位移曲线", "缺少数据" });
					if (status == Tree_Yes)
						status = Tree_Failure;
				}
			}
		}
		return status;
	}
}
