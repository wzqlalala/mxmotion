#include "mDiscreteField.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechDiscreteFieldData
{
	mDiscreteField::mDiscreteField(int id, const QString& name)
		: mFieldBase(id, name)
		, _directPoint2(1, 0, 0), _directPoint3(0, 1, 0)
	{
	}

	mDiscreteField::~mDiscreteField()
	{
	}

	mDiscreteField* mDiscreteField::copyDiscreteField()
	{
		mDiscreteField* copyField = new mDiscreteField(_id, _name);
		copyField->_locate = _locate;
		copyField->_spaceType = _spaceType;
		copyField->_defaultAngel = _defaultAngel;
		copyField->_elementAngelMap = _elementAngelMap;
		copyField->_defaultThickness = _defaultThickness;
		copyField->_elementThicknessMap = _elementThicknessMap;
		copyField->_nodeThicknessMap = _nodeThicknessMap;
		copyField->_coorType = _coorType;
		copyField->_directPoint1 = _directPoint1;
		copyField->_directPoint2 = _directPoint2;
		copyField->_directPoint3 = _directPoint3;
		copyField->_elementCoorMap = _elementCoorMap;
		return copyField;
	}

	void mDiscreteField::setDiscreteSpaceName(const QString& name)
	{
		_name = name;
	}

	QString mDiscreteField::getDiscreteSpaceName() const
	{
		return _name;
	}

	void mDiscreteField::setLocate(MxFunctions::PickDataType locate)
	{
		_locate = locate;
	}

	MxFunctions::PickDataType mDiscreteField::getLocate()
	{
		return _locate;
	}

	void mDiscreteField::setSpaceType(MxFunctions::DiscreteSpace type)
	{
		_spaceType = type;
	}

	MxFunctions::DiscreteSpace mDiscreteField::getSpaceType() const
	{
		return _spaceType;
	}

	void mDiscreteField::setDefaultAngel(const QString& angel)
	{
		_defaultAngel = angel;
	}

	QString mDiscreteField::getDefaultAngel() const
	{
		return _defaultAngel;
	}

	void mDiscreteField::setElementAngelMap(const std::map<int, QString>& map)
	{
		_elementAngelMap = map;
	}

	std::map<int, QString> mDiscreteField::getElementAngelMap() const
	{
		return _elementAngelMap;
	}

	void mDiscreteField::setDefaultThickness(const QString& thickness)
	{
		_defaultThickness = thickness;
	}

	QString mDiscreteField::getDefaultThickness() const
	{
		return _defaultThickness;
	}

	void mDiscreteField::setElementThicknessMap(const std::map<int, QString>& map)
	{
		_elementThicknessMap = map;
	}

	std::map<int, QString> mDiscreteField::getElementThicknessMap() const
	{
		return _elementThicknessMap;
	}

	void mDiscreteField::setNodeThicknessMap(const std::map<int, QString>& map)
	{
		_nodeThicknessMap = map;
	}

	std::map<int, QString> mDiscreteField::getNodeThicknessMap() const
	{
		return _nodeThicknessMap;
	}

	void mDiscreteField::setCoorType(MxFunctions::CoordinateType type)
	{
		_coorType = type;
	}

	MxFunctions::CoordinateType mDiscreteField::getCoorType() const
	{
		return _coorType;
	}

	void mDiscreteField::setDirectPoint1(const QVector3D& point)
	{
		_directPoint1 = point;
	}

	QVector3D mDiscreteField::getDirectPoint1() const
	{
		return _directPoint1;
	}

	void mDiscreteField::setDirectPoint2(const QVector3D& point)
	{
		_directPoint2 = point;
	}

	QVector3D mDiscreteField::getDirectPoint2() const
	{
		return _directPoint2;
	}

	void mDiscreteField::setDirectPoint3(const QVector3D& point)
	{
		_directPoint3 = point;
	}

	QVector3D mDiscreteField::getDirectPoint3() const
	{
		return _directPoint3;
	}

	void mDiscreteField::setElementCoorMap(const std::map<int, QStringList>& map)
	{
		_elementCoorMap = map;
	}

	std::map<int, QStringList> mDiscreteField::getElementCoorMap() const
	{
		return _elementCoorMap;
	}

	MxFunctions::BrowserTreeStatus mDiscreteField::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		bool isFailture = false;

		if (_name.isEmpty())
		{
			isFailture = true;
		}

		if (_spaceType == MxFunctions::DiscreteSpace::AngelSpace)
		{
			if (_defaultAngel.isEmpty() && _elementAngelMap.empty())
				isFailture = true;
			else if (_locate == MxFunctions::PickDataType::Node)
				isFailture = true;
			else if (_locate == MxFunctions::PickDataType::Element && _elementAngelMap.empty())
				isFailture = true;
			else if (_locate == MxFunctions::PickDataType::Element && _defaultAngel.isEmpty())
				isFailture = true;
		}
		else if (_spaceType == MxFunctions::DiscreteSpace::ThicknessSpace)
		{
			if (_locate == MxFunctions::PickDataType::Node && _nodeThicknessMap.empty())
				isFailture = true;
			else if (_locate == MxFunctions::PickDataType::Element && _elementThicknessMap.empty())
				isFailture = true;
			else if (_locate == MxFunctions::PickDataType::Element && _defaultThickness.isEmpty())
				isFailture = true;
		}
		else if (_spaceType == MxFunctions::DiscreteSpace::CoorSpace)
		{
			if (_locate == MxFunctions::PickDataType::Node)
				isFailture = true;
			else if (_locate == MxFunctions::PickDataType::Element && _elementCoorMap.empty())
				isFailture = true;
		}

		if (isFailture)
		{
			info.append({ tip,"离散场数据不完整" });
			return Tree_Failure;
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}
}
