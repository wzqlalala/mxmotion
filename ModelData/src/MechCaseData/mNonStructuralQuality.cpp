#include "mNonStructuralQuality.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechCaseData
{
	

	mNonStructuralQuality::mNonStructuralQuality(int id, const QString& name)
		:  _nsmName(name), _id(id)
	{
		
		if (name.isEmpty())
		{
			_nsmName = QString("%1_%2").arg(getNonStructuralQualityStr()).arg(_id);
		}
	}

	mNonStructuralQuality::mNonStructuralQuality(int id, MxFunctions::Non type, const QString& name)
		: _nsmName(name), _id(id),_typeIndex(type)
	{
		if (name.isEmpty())
		{
			_nsmName = QString("%1_%2").arg(getNonStructuralQualityStr()).arg(_id);
		}
	}

	mNonStructuralQuality::~mNonStructuralQuality()
	{
	}

	void mNonStructuralQuality::setNsmId(int id)
	{
		_id = id;
	}

	int mNonStructuralQuality::getNonStructuralQualityId()
	{
		return _id;
	}

	void mNonStructuralQuality::setNsmName(const QString& name)
	{
		_nsmName = name;
	}

	QString mNonStructuralQuality::getNonStructuralQualityName()
	{
		return _nsmName;
	}

	QString mNonStructuralQuality::getNonStructuralQualityStr()
	{
		return QString("非结构质量");
	}


	mNonStructuralQuality* mNonStructuralQuality::copyNonStructuralQuality(const int& id, const QString& name)
	{
		mNonStructuralQuality* caseData = new mNonStructuralQuality(id,  name);
		//caseData->_angle = _angle;
		
		caseData->_typeIndex = _typeIndex;
		caseData->_propIndex = _propIndex;
		caseData->_unionValue = _unionValue;

		caseData->_crossPropertyData = _crossPropertyData;
		caseData->_elementData = _elementData;
		caseData->_elementSetData = _elementSetData;

		return caseData;
	}

	void mNonStructuralQuality::setUnionValue(const QString& value)
	{
		_unionValue = value;
	}

	QString mNonStructuralQuality::getUnionValue() const
	{
		return _unionValue;
	}

	void mNonStructuralQuality::setCrossPropertyData(const std::map<QString, QString>& data)
	{
		_crossPropertyData = data;
	}

	std::map<QString, QString>& mNonStructuralQuality::getCrossPropertyData()
	{
		return _crossPropertyData;
	}

	const std::map<QString, QString>& mNonStructuralQuality::getCrossPropertyData() const
	{
		return _crossPropertyData;
	}

	void mNonStructuralQuality::setElementData(const std::map<QString, QString>& data)
	{
		_elementData = data;
	}

	std::map<QString, QString>& mNonStructuralQuality::getElementData()
	{
		return _elementData;
	}

	const std::map<QString, QString>& mNonStructuralQuality::getElementData() const
	{
		return _elementData;
	}

	void mNonStructuralQuality::setElementSetData(const std::map<QString, QString>& data)
	{
		_elementSetData = data;
	}

	std::map<QString, QString>& mNonStructuralQuality::getElementSetData()
	{
		return _elementSetData;
	}

	const std::map<QString, QString>& mNonStructuralQuality::getElementSetData() const
	{
		return _elementSetData;
	}

	MxFunctions::Non mNonStructuralQuality::getTypeIndex()
	{
		return _typeIndex;
	}

	void mNonStructuralQuality::setTypeIndex(MxFunctions::Non typeIndex)
	{
		_typeIndex = typeIndex;
	}

	MxFunctions::PhysicalFieldType mNonStructuralQuality::getPropIndex()
	{
		return _propIndex;
	}

	void mNonStructuralQuality::setPropIndex(MxFunctions::PhysicalFieldType propIndex)
	{
		_propIndex = propIndex;
	}

	MxFunctions::BrowserTreeStatus mNonStructuralQuality::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		bool isFailture = false;

		if (_propIndex == Physical_Property)
		{
			if (_crossPropertyData.empty())
				isFailture = true;
		}
		else if (_propIndex == Physical_Element)
		{
			if (_elementData.empty())
				isFailture = true;
		}
		else if (_propIndex == Physical_Set)
		{
			if (_elementSetData.empty())
				isFailture = true;
		}

		if (isFailture)
		{
			info.append({tip,"非结构质量为空"});
			return Tree_Failure;
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}


}
