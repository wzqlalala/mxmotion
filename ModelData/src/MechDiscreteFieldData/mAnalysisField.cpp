#include "mAnalysisField.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechDiscreteFieldData
{
	mAnalysisField::mAnalysisField(int id, const QString& name)
		: mFieldBase(id, name)
	{
	}

	mAnalysisField::~mAnalysisField()
	{
	}

	mAnalysisField* mAnalysisField::copyAnalysisField()
	{
		mAnalysisField* copyField = new mAnalysisField(_id, _name);
		copyField->_locate = _locate;
		copyField->_componentCount = _componentCount;
		copyField->_method = _method;
		copyField->_dataType = _dataType;
		copyField->_textFilePath = _textFilePath;
		copyField->_textStartRow = _textStartRow;
		copyField->_textEndRow = _textEndRow;
		copyField->_textStartColumn = _textStartColumn;
		copyField->_rows = _rows;
		copyField->_defaultValue = _defaultValue;
		copyField->_scaleMode = _scaleMode;
		copyField->_uniformScale = _uniformScale;
		copyField->_scaleX = _scaleX;
		copyField->_scaleY = _scaleY;
		copyField->_scaleZ = _scaleZ;
		copyField->_toleranceType = _toleranceType;
		copyField->_positiveTolerance = _positiveTolerance;
		copyField->_negativeTolerance = _negativeTolerance;
		copyField->_neighborhoodTolerance = _neighborhoodTolerance;
		copyField->_interpolationTolerance = _interpolationTolerance;
		copyField->_mappingAlgorithm = _mappingAlgorithm;
		return copyField;
	}

	void mAnalysisField::setLocate(MxFunctions::PickDataType locate)
	{
		_locate = locate;
	}

	MxFunctions::PickDataType mAnalysisField::getLocate()
	{
		return _locate;
	}

	void mAnalysisField::setAnalysisFieldName(const QString& name)
	{
		_name = name;
	}

	QString mAnalysisField::getAnalysisFieldName() const
	{
		return _name;
	}

	void mAnalysisField::setComponentCount(int count)
	{
		_componentCount = count;
	}

	int mAnalysisField::getComponentCount() const
	{
		return _componentCount;
	}

	void mAnalysisField::setDefineMethod(DefineMethod method)
	{
		_method = method;
	}

	mAnalysisField::DefineMethod mAnalysisField::getDefineMethod() const
	{
		return _method;
	}

	void mAnalysisField::setDataType(DataType type)
	{
		_dataType = type;
	}

	mAnalysisField::DataType mAnalysisField::getDataType() const
	{
		return _dataType;
	}

	void mAnalysisField::setTextFilePath(const QString& path)
	{
		_textFilePath = path;
	}

	QString mAnalysisField::getTextFilePath() const
	{
		return _textFilePath;
	}

	void mAnalysisField::setTextStartRow(int row)
	{
		_textStartRow = row;
	}

	int mAnalysisField::getTextStartRow() const
	{
		return _textStartRow;
	}

	void mAnalysisField::setTextEndRow(int row)
	{
		_textEndRow = row;
	}

	int mAnalysisField::getTextEndRow() const
	{
		return _textEndRow;
	}

	void mAnalysisField::setTextStartColumn(int column)
	{
		_textStartColumn = column;
	}

	int mAnalysisField::getTextStartColumn() const
	{
		return _textStartColumn;
	}

	void mAnalysisField::setRows(const QList<AnalysisFieldRow>& rows)
	{
		_rows = rows;
	}

	void mAnalysisField::setRows(QList<AnalysisFieldRow>&& rows)
	{
		_rows = std::move(rows);
	}

	QList<AnalysisFieldRow> mAnalysisField::getRows() const
	{
		return _rows;
	}

	void mAnalysisField::normalizeRowValues()
	{
		for (auto& row : _rows)
		{
			while (row.values.size() < _componentCount)
				row.values.append(QString());
			while (row.values.size() > _componentCount)
				row.values.removeLast();
		}
	}

	void mAnalysisField::setDefaultValue(const QString& value)
	{
		_defaultValue = value;
	}

	QString mAnalysisField::getDefaultValue() const
	{
		return _defaultValue;
	}

	void mAnalysisField::setScaleMode(ScaleMode mode)
	{
		_scaleMode = mode;
	}

	mAnalysisField::ScaleMode mAnalysisField::getScaleMode() const
	{
		return _scaleMode;
	}

	void mAnalysisField::setUniformScale(double scale)
	{
		_uniformScale = scale;
	}

	double mAnalysisField::getUniformScale() const
	{
		return _uniformScale;
	}

	void mAnalysisField::setScaleX(double scale)
	{
		_scaleX = scale;
	}

	double mAnalysisField::getScaleX() const
	{
		return _scaleX;
	}

	void mAnalysisField::setScaleY(double scale)
	{
		_scaleY = scale;
	}

	double mAnalysisField::getScaleY() const
	{
		return _scaleY;
	}

	void mAnalysisField::setScaleZ(double scale)
	{
		_scaleZ = scale;
	}

	double mAnalysisField::getScaleZ() const
	{
		return _scaleZ;
	}

	void mAnalysisField::setToleranceType(ToleranceType type)
	{
		_toleranceType = type;
	}

	mAnalysisField::ToleranceType mAnalysisField::getToleranceType() const
	{
		return _toleranceType;
	}

	void mAnalysisField::setPositiveTolerance(double tolerance)
	{
		_positiveTolerance = tolerance;
	}

	double mAnalysisField::getPositiveTolerance() const
	{
		return _positiveTolerance;
	}

	void mAnalysisField::setNegativeTolerance(double tolerance)
	{
		_negativeTolerance = tolerance;
	}

	double mAnalysisField::getNegativeTolerance() const
	{
		return _negativeTolerance;
	}

	void mAnalysisField::setNeighborhoodTolerance(double tolerance)
	{
		_neighborhoodTolerance = tolerance;
	}

	double mAnalysisField::getNeighborhoodTolerance() const
	{
		return _neighborhoodTolerance;
	}

	void mAnalysisField::setInterpolationTolerance(double tolerance)
	{
		_interpolationTolerance = tolerance;
	}

	double mAnalysisField::getInterpolationTolerance() const
	{
		return _interpolationTolerance;
	}

	void mAnalysisField::setMappingAlgorithm(MappingAlgorithm algorithm)
	{
		_mappingAlgorithm = algorithm;
	}

	mAnalysisField::MappingAlgorithm mAnalysisField::getMappingAlgorithm() const
	{
		return _mappingAlgorithm;
	}

	MxFunctions::BrowserTreeStatus mAnalysisField::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		bool isFailture = false;

		if (_name.isEmpty())
		{
			isFailture = true;
		}

		//点云模式：行数据为空且默认场值为空视为不完整
		if (_dataType == DataType::PointCloud)
		{
			if (_rows.isEmpty() && _defaultValue.isEmpty())
				isFailture = true;
		}
		//离散模式：行数据为空视为不完整
		else if (_dataType == DataType::ObjectDiscrete)
		{
			if (_rows.isEmpty())
				isFailture = true;
		}

		if (isFailture)
		{
			info.append({ tip,"分析场数据不完整" });
			return Tree_Failure;
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}
}
