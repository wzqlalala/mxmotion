#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mGlobalParam.h"
#include "mGenericParamData.h"
#include "mGenericAssignData.h"
#include "mGenericMatrixData.h"
#include "mModalContriData.h"
#include "mNodeContriData.h"
#include "mPanelContriData.h"
#include "mTPContriData.h"
#include "mERPData.h"
#include "mPeakFilterData.h"
#include "mSolveControlData.h"
#include "mGlobalOutputControlData.h"
#include "mASCouplingSearchData.h"

namespace MechGlobalParamData
{
    inline QDataStream& operator>>(QDataStream& in, mGenericParamData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGenericParamDataV3(in, obj);
        }
        else
        {
            InStreammGenericParamDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammGenericParamDataV3(QDataStream& in, mGenericParamData& obj)
    {
        in >> obj._id >> obj._name >> obj._method >> obj._upFrequency
            >> obj._order >> obj._startID;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mGenericAssignData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGenericAssignDataV3(in, obj);
        }
        else
        {
            InStreammGenericAssignDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammGenericAssignDataV3(QDataStream& in, mGenericAssignData& obj)
    {
        in >> obj._filePath;
        // 读取矩阵配置
        in >> obj._stiffnessMatrixEnabled;
        in >> obj._stiffnessMatrixNames;

        in >> obj._massMatrixEnabled;
        in >> obj._massMatrixNames;

        in >> obj._viscousDampingMatrixEnabled;
        in >> obj._viscousDampingMatrixNames;

        in >> obj._materialMatrixEnabled;
        in >> obj._materialMatrixNames;


        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mGenericMatrixData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGenericMatrixDataV3(in, obj);
        }
        else
        {
            InStreammGenericMatrixDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammGenericMatrixDataV3(QDataStream& in, mGenericMatrixData& obj)
    {
        in >> obj._matrixName;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mModalContriData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammModalContriDataV3(in, obj);
        }
        else
        {
            InStreammModalContriDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammModalContriDataV3(QDataStream& in, mModalContriData& obj)
    {
        in >> obj._outPutType >> obj._SortType >> obj._ResultType >>
            obj._fltScore >> obj._OutOrd >> obj._checkBoxPkScr >>
            obj._fltScoreChecked >> obj._OutOrdChecked;
        in >> obj._id >> obj._name;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mNodeContriData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammNodeContriDataV3(in, obj);
        }
        else
        {
            InStreammNodeContriDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammNodeContriDataV3(QDataStream& in, mNodeContriData& obj)
    {
        in >> obj._rpTruncType >> obj._rpTruncValue >>
            obj._checkBoxCavityProj >> obj._checkBoxPkScr;
        in >> obj._id >> obj._name;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mPanelContriData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPanelContriDataV3(in, obj);
        }
        else
        {
            InStreammPanelContriDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammPanelContriDataV3(QDataStream& in, mPanelContriData& obj)
    {
        in >> obj._outPutType >> obj._checkBoxPkScr;
        in >> obj._id >> obj._name;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mTPContriData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammTPContriDataV3(in, obj);
        }
        else
        {
            InStreammTPContriDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammTPContriDataV3(QDataStream& in, mTPContriData& obj)
    {
        in >> obj._respType;
        in >> obj._id;
        in >> obj._name;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mERPData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammERPDataV3(in, obj);
        }
        else
        {
            InStreammERPDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammERPDataV3(QDataStream& in, mERPData& obj)
    {
        in >> obj._erprho >> obj._erpc >> obj._rhocp >> obj._erprlf >> obj._erprefdb >> obj._id >> obj._name;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mPeakFilterData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammPeakFilterDataV3(in, obj);
        }
        else
        {
            InStreammPeakFilterDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammPeakFilterDataV3(QDataStream& in, mPeakFilterData& obj)
    {
        in >> obj._id >> obj._name >> obj._respType >> obj._identType
            >> obj._extractCount >> obj._minSpacing >> obj._maxSpacing
            >> obj._startFreq >> obj._endFreq >> obj._applyToGlobal;
        // 表格数据
        qint32 tableSize;
        in >> tableSize;
        obj._tableData.resize(tableSize);
        for (int i = 0; i < tableSize; ++i)
        {
            qint32 colSize;
            in >> colSize;
            obj._tableData[i].resize(colSize);
            for (int j = 0; j < colSize; ++j)
            {
                in >> obj._tableData[i][j];
            }
        }
        return in;
    }




    inline QDataStream& operator>>(QDataStream& in, mASCouplingSearchData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammASCouplingSearchDataV3(in, obj);
        }
        else
        {
            InStreammASCouplingSearchDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammASCouplingSearchDataV3(QDataStream& in, mASCouplingSearchData& obj)
    {
        in >> obj._autosearch >> obj._mainSurface >> obj._angelChecked >> obj._searchAngel
            >> obj._outChecked >> obj._outerScale >> obj._innerChecked >> obj._innerScale
            >> obj._horizontalChecked >> obj._horizontalScale >> obj._boundingBoxChecked >> obj.boundingBoxScale >> obj._filePath;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mGenericPartData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGenericPartDataV3(in, obj);
        }
        else
        {
            InStreammGenericPartDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammGenericPartDataV3(QDataStream& in, mGenericPartData& obj)
    {
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mSolveControlData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammSolveControlDataV3(in, obj);
        }
        else
        {
            InStreammSolveControlDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammSolveControlDataV3(QDataStream& in, mSolveControlData& obj)
    {
        in >> obj._cpuCoreCount;
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mGlobalOutputControlData& obj)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGlobalOutputControlDataV3(in, obj);
        }
        else
        {
            InStreammGlobalOutputControlDataV3(in, obj);
        }
        return in;
}
    inline QDataStream& InStreammGlobalOutputControlDataV3(QDataStream& in, mGlobalOutputControlData& obj)
    {
        in >> obj._created;
        qint32 size = 0;
        in >> size;
        obj._outputSelections.clear();
        for (qint32 i = 0; i < size; ++i)
        {
            QString key;
            QStringList value;
            in >> key;
            in >> value;
            obj._outputSelections[key] = value;
        }
        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mGlobalParam& mgr)
    {
        //版本号高先判断，兼容所有低版本与高版本
        if (FILE_VERSION >= 0x3000)
        {
            InStreammGlobalParamV3(in, mgr);
        }
        else
        {
            InStreammGlobalParamV3(in, mgr);
        }
        return in;
}
    inline QDataStream& InStreammGlobalParamV3(QDataStream& in, mGlobalParam& mgr)
    {
        in >> mgr._maxGenericParamID;
        qint32 genericParaSize;
        in >> genericParaSize;
        for (qint32 i = 0; i < genericParaSize; ++i)
        {
            int key;
            in >> key;
            // 动态创建新对象并读入数据
            mGenericParamData* genericParaObj = new mGenericParamData(key, "");
            in >> *genericParaObj;
            mgr._genericParaData[key] = genericParaObj;
        }

        bool hasGenericAssign;
        in >> hasGenericAssign;
        if (hasGenericAssign)
        {
            mgr._genericAssignData = new mGenericAssignData();
            in >> *(mgr._genericAssignData);
        }

        bool hasGenericMatrix;
        in >> hasGenericMatrix;
        if (hasGenericMatrix)
        {
            mgr._genericMatrixData = new mGenericMatrixData();
            in >> *(mgr._genericMatrixData);
        }

        bool hasGenericPart;
        in >> hasGenericPart;
        if (hasGenericPart)
        {
            mgr._genericPartData = new mGenericPartData();
            in >> *(mgr._genericPartData);
        }

        bool hasmodalContri;
        in >> hasmodalContri;
        if (hasmodalContri)
        {
            mgr._modalContriData = new mModalContriData();
            in >> *(mgr._modalContriData);
        }

        bool hasNodeContri;
        in >> hasNodeContri;
        if (hasNodeContri)
        {
            mgr._mNodeContriData = new mNodeContriData();
            in >> *(mgr._mNodeContriData);
        }

        bool hasPanelContri;
        in >> hasPanelContri;
        if (hasPanelContri)
        {
            mgr._mPanelContriData = new mPanelContriData();
            in >> *(mgr._mPanelContriData);
        }

        bool hasTPContri;
        in >> hasTPContri;
        if (hasTPContri)
        {
            mgr._mTPContriData = new mTPContriData();
            in >> *(mgr._mTPContriData);
        }

        bool hasERP;
        in >> hasERP;
        if (hasERP)
        {
            mgr._mERPData = new mERPData();
            in >> *(mgr._mERPData);
        }

        in >> mgr._maxPeakFilterID;
        qint32 peakFilterSize;
        in >> peakFilterSize;
        for (qint32 i = 0; i < peakFilterSize; ++i)
        {
            int key;
            in >> key;
            mPeakFilterData* peakFilterObj = new mPeakFilterData(key, "");
            in >> *peakFilterObj;
            mgr._peakFilterData[key] = peakFilterObj;
        }

        bool hasASCouplingSearch;
        in >> hasASCouplingSearch;
        if (hasASCouplingSearch)
        {
            mgr._ascouplingSearchData = new mASCouplingSearchData();
            in >> *(mgr._ascouplingSearchData);
        }

        bool hasSolveControl;
        in >> hasSolveControl;
        if (hasSolveControl)
        {
            mgr._solveControlData = new mSolveControlData();
            in >> *(mgr._solveControlData);
        }

        bool hasGlobalOutputControl;
        in >> hasGlobalOutputControl;
        if (hasGlobalOutputControl)
        {
            mgr._globalOutputControlData = new mGlobalOutputControlData();
            in >> *(mgr._globalOutputControlData);
        }

        return in;
    }

}
