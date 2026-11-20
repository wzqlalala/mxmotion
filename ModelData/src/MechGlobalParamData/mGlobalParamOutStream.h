#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>
#include <vector>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mGlobalParam.h"
#include "mGenericParamData.h"
#include "mGenericAssignData.h"
#include "mGenericMatrixData.h"
#include "mModalContriData.h"
#include "mPanelContriData.h"
#include "mTPContriData.h"
#include "mERPData.h"
#include "mPeakFilterData.h"
#include "mSolveControlData.h"
#include "mGlobalOutputControlData.h"

#include "mASCouplingSearchData.h"

namespace MechGlobalParamData
{
    inline QDataStream& operator<<(QDataStream& out, mGenericParamData& obj)
    {
        out << obj._id << obj._name << obj._method << obj._upFrequency
            << obj._order << obj._startID;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mGenericAssignData& obj)
    {
        out << obj._filePath;
        // 写入矩阵配置
        out << obj._stiffnessMatrixEnabled;
        out << obj._stiffnessMatrixNames;

        out << obj._massMatrixEnabled;
        out << obj._massMatrixNames;

        out << obj._viscousDampingMatrixEnabled;
        out << obj._viscousDampingMatrixNames;

        out << obj._materialMatrixEnabled;
        out << obj._materialMatrixNames;

        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mGenericMatrixData& obj)
    {
        out << obj._matrixName;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mModalContriData& obj)
    {
        out << obj._outPutType << obj._SortType << obj._ResultType <<
            obj._fltScore << obj._OutOrd << obj._checkBoxPkScr <<
            obj._fltScoreChecked << obj._OutOrdChecked;
        out << obj._id << obj._name;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mNodeContriData& obj)
    {
        out << obj._rpTruncType << obj._rpTruncValue <<
            obj._checkBoxCavityProj << obj._checkBoxPkScr;
        out << obj._id << obj._name;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mPanelContriData& obj)
    {
        out << obj._outPutType << obj._checkBoxPkScr;
        out << obj._id << obj._name;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mTPContriData& obj)
    {
        out << obj._respType;
        out << obj._id;
        out << obj._name;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mERPData& obj)
    {
        out << obj._erprho << obj._erpc << obj._rhocp << obj._erprlf << obj._erprefdb << obj._id << obj._name;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mPeakFilterData& obj)
    {
        out << obj._id << obj._name << obj._respType << obj._identType
            << obj._extractCount << obj._minSpacing << obj._maxSpacing
            << obj._startFreq << obj._endFreq << obj._applyToGlobal;
        // 表格数据
        out << static_cast<qint32>(obj._tableData.size());
        for (int i = 0; i < obj._tableData.size(); ++i)
        {
            out << static_cast<qint32>(obj._tableData[i].size());
            for (int j = 0; j < obj._tableData[i].size(); ++j)
            {
                out << obj._tableData[i][j];
            }
        }
        return out;
    }



    inline QDataStream& operator<<(QDataStream& out, mASCouplingSearchData& obj)
    {
        out << obj._autosearch << obj._mainSurface << obj._angelChecked << obj._searchAngel
            << obj._outChecked << obj._outerScale << obj._innerChecked << obj._innerScale
            << obj._horizontalChecked << obj._horizontalScale
            << obj._boundingBoxChecked << obj.boundingBoxScale << obj._filePath;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mGenericPartData& obj)
    {
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mSolveControlData& obj)
    {
        out << obj._cpuCoreCount;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mGlobalOutputControlData& obj)
    {
        out << obj._created;
        out << static_cast<qint32>(obj._outputSelections.size());
        for (auto it = obj._outputSelections.begin(); it != obj._outputSelections.end(); ++it)
        {
            out << it.key() << it.value();
        }
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mGlobalParam& mgr)
    {

        out << mgr._maxGenericParamID;
        out << static_cast<qint32>(mgr._genericParaData.size());
        for (auto it = mgr._genericParaData.begin(); it != mgr._genericParaData.end(); ++it)
        {
            out << it->first;           // 存 ID 键
            out << *(it->second);       // 存对象实例
        }

        if (mgr._genericAssignData)
        {
            out << true;                // 标记位：数据存在
            out << *(mgr._genericAssignData);    // 存对象实例
        }
        else
        {
            out << false;               // 标记位：数据为空
        }

        if (mgr._genericMatrixData)
        {
            out << true;                // 标记位：数据存在
            out << *(mgr._genericMatrixData);    // 存对象实例
        }
        else
        {
            out << false;               // 标记位：数据为空
        }

        if (mgr._genericPartData)
        {
            out << true;
            out << *(mgr._genericPartData);
        }
        else
        {
            out << false;
        }

        if (mgr._modalContriData)
        {
            out << true;
            out << *(mgr._modalContriData);
        }
        else
        {
            out << false;
        }

        if (mgr._mNodeContriData)
        {
            out << true;
            out << *(mgr._mNodeContriData);
        }
        else
        {
            out << false;
        }

        if (mgr._mPanelContriData)
        {
            out << true;
            out << *(mgr._mPanelContriData);
        }
        else
        {
            out << false;
        }

        if (mgr._mTPContriData)
        {
            out << true;
            out << *(mgr._mTPContriData);
        }
        else
        {
            out << false;
        }

        if (mgr._mERPData)
        {
            out << true;
            out << *(mgr._mERPData);
        }
        else
        {
            out << false;
        }

        out << mgr._maxPeakFilterID;
        out << static_cast<qint32>(mgr._peakFilterData.size());
        for (auto it = mgr._peakFilterData.begin(); it != mgr._peakFilterData.end(); ++it)
        {
            out << it->first;
            out << *(it->second);
        }
        if (mgr._ascouplingSearchData)
        {
            out << true;
            out << *(mgr._ascouplingSearchData);
        }
        else
        {
            out << false;
        }

        if (mgr._solveControlData)
        {
            out << true;
            out << *(mgr._solveControlData);
        }
        else
        {
            out << false;
        }

        if (mgr._globalOutputControlData)
        {
            out << true;
            out << *(mgr._globalOutputControlData);
        }
        else
        {
            out << false;
        }

        return out;
    }
}