#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mFieldManager.h"
#include "mAnalysisField.h"



namespace MechDiscreteFieldData
{
	/*
	* 分析场二进制写出。
	* 注意：mFieldManager 的流算子在 mDiscreteFieldDataOutStream.h 中按
	* FieldType::Discrete 过滤写出离散场；分析场由本文件的 mAnalysisField
	* 流算子逐字段写出（枚举以 qint32 强转写出），谁使用分析场数据谁负责
	* 按本顺序逐个 <<。
	*/
	inline QDataStream& operator<<(QDataStream& out, mAnalysisField& obj)
	{
		out << obj._id;
		out << obj._name;

		out << (qint32)obj._locate;
		out << (qint32)obj._componentCount;
		out << (qint32)obj._method;
		out << (qint32)obj._dataType;

		out << obj._textFilePath;
		out << (qint32)obj._textStartRow;
		out << (qint32)obj._textEndRow;
		out << (qint32)obj._textStartColumn;

		//数据行
		out << (qint32)obj._rows.size();
		for (const auto& row : obj._rows)
		{
			out << row.id;
			out << row.elementId;
			out << row.faceId;
			out << row.x;
			out << row.y;
			out << row.z;
			out << row.values;
		}

		out << obj._defaultValue;

		//映射空间（坐标系/目标集合不存于本类，由 mMapRelationManager 序列化）
		out << (qint32)obj._scaleMode;
		out << obj._uniformScale;
		out << obj._scaleX;
		out << obj._scaleY;
		out << obj._scaleZ;

		//映射控制
		out << (qint32)obj._toleranceType;
		out << obj._positiveTolerance;
		out << obj._negativeTolerance;
		out << obj._neighborhoodTolerance;
		out << obj._interpolationTolerance;
		out << (qint32)obj._mappingAlgorithm;

		return out;
	}
}
