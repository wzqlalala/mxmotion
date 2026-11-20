#include "mPredefinedFieldInitTemp.h"

namespace MechCaseData
{
    mPredefinedFieldInitTemp::mPredefinedFieldInitTemp(int id, MxFunctions::PredefinedFieldType type, const QString& name)
        :mPredefinedFieldBase(id, type, name)
    {
        _defineType = 0; // 默认统一赋值
    }

    mPredefinedFieldInitTemp::~mPredefinedFieldInitTemp()
    {

    }

    std::shared_ptr<mPredefinedFieldBase> mPredefinedFieldInitTemp::copyPredefinedField(const int& id, const QString& name)
    {
        std::shared_ptr<mPredefinedFieldInitTemp> field = std::make_shared<mPredefinedFieldInitTemp>(id, getPredefinedFieldType(), name);
        field->_temp = _temp;
        field->_defineType = _defineType;
        field->_nodeTempMap = _nodeTempMap;
        return field;
    }

    MxFunctions::BrowserTreeStatus mPredefinedFieldInitTemp::getNodeStatu(QVector<QPair<QString, QString>>& info)
    {
        QString tip = "警告";

        if (_defineType == 0) // 统一赋值
        {
            if (_temp.isEmpty())
            {
                info.append({ tip, "温度大小未设置" });
            }
        }

        if (_defineType == 1) // 独立赋值
        {
            if (_nodeTempMap.empty())
            {
                info.append({ tip, "未添加任何节点温度数据" });
            }
        }
        return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
    }
}
