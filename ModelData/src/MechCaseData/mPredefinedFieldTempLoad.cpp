#include "mPredefinedFieldTempLoad.h"

namespace MechCaseData
{
	mPredefinedFieldTempLoad::mPredefinedFieldTempLoad(int id, MxFunctions::PredefinedFieldType type, const QString& name)
		:mPredefinedFieldBase(id, type, name)
	{
		_defineType = 0;
		_interpolateType = 0;
		_midNodeInterpolate = false;
	}

	mPredefinedFieldTempLoad::~mPredefinedFieldTempLoad()
	{

	}

	std::shared_ptr<mPredefinedFieldBase> mPredefinedFieldTempLoad::copyPredefinedField(const int& id, const QString& name)
	{
		std::shared_ptr<mPredefinedFieldTempLoad> field = std::make_shared<mPredefinedFieldTempLoad>(id, getPredefinedFieldType(), name);
		field->_temp = _temp;
		field->_defineType = _defineType;
		field->_nodeTempMap = _nodeTempMap;
		field->_filePath = _filePath;
		field->_interpolateType = _interpolateType;
		field->_midNodeInterpolate = _midNodeInterpolate;
		return field;
	}

	MxFunctions::BrowserTreeStatus mPredefinedFieldTempLoad::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";

		if (_defineType == 0) // 统一赋值
		{
			if (_temp.isEmpty())
			{
				info.append({ tip, "温度大小未设置" });
			}
		}
		else if (_defineType == 1) // 独立赋值
		{
			if (_nodeTempMap.empty())
			{
				info.append({ tip, "未添加任何节点温度数据" });
			}
		}
		else // 文件定义
		{
			if (_filePath.isEmpty())
			{
				info.append({ tip, "未选择温度文件" });
			}
		}

		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}
