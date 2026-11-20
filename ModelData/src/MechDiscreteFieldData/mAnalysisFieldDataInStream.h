#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mFieldManager.h"
#include "mAnalysisField.h"


namespace MechDiscreteFieldData
{
	/*
	* 分析场二进制读入：按 mAnalysisFieldDataOutStream.h 的字段顺序逐个 in >>，
	* 枚举以 qint32 读出后强转回枚举类型。
	*/
	inline QDataStream& operator>>(QDataStream& in, mAnalysisField& obj)
	{
		in >> obj._id;
		in >> obj._name;

		qint32 locate;
		in >> locate;
		obj._locate = static_cast<MxFunctions::PickDataType>(locate);

		in >> obj._componentCount;

		qint32 method;
		in >> method;
		obj._method = static_cast<mAnalysisField::DefineMethod>(method);

		qint32 dataType;
		in >> dataType;
		obj._dataType = static_cast<mAnalysisField::DataType>(dataType);

		in >> obj._textFilePath;
		in >> obj._textStartRow;
		in >> obj._textEndRow;
		in >> obj._textStartColumn;

		//数据行
		qint32 rowCount;
		in >> rowCount;
		obj._rows.clear();
		obj._rows.reserve(rowCount);
		for (qint32 i = 0; i < rowCount; ++i)
		{
			AnalysisFieldRow row;
			in >> row.id;
			in >> row.elementId;
			in >> row.faceId;
			in >> row.x;
			in >> row.y;
			in >> row.z;
			in >> row.values;
			obj._rows.append(row);
		}

		in >> obj._defaultValue;

		//映射空间
		qint32 scaleMode;
		in >> scaleMode;
		obj._scaleMode = static_cast<mAnalysisField::ScaleMode>(scaleMode);
		in >> obj._uniformScale;
		in >> obj._scaleX;
		in >> obj._scaleY;
		in >> obj._scaleZ;

		//映射控制
		qint32 toleranceType;
		in >> toleranceType;
		obj._toleranceType = static_cast<mAnalysisField::ToleranceType>(toleranceType);
		in >> obj._positiveTolerance;
		in >> obj._negativeTolerance;
		in >> obj._neighborhoodTolerance;
		in >> obj._interpolationTolerance;

		qint32 mappingAlgorithm;
		in >> mappingAlgorithm;
		obj._mappingAlgorithm = static_cast<mAnalysisField::MappingAlgorithm>(mappingAlgorithm);

		return in;
	}
}
