#include "mPostTransferPathAnalysisData.h"

#include <string>
#include <iostream>
#include <QVector>
#include <QSet>
#include <qmath.h>

using namespace std;
namespace PostWindow {
    mPostTransferFunctionData::mPostTransferFunctionData()
    {

    }

    mPostTransferFunctionData::~mPostTransferFunctionData()
    {

    }

    mPostResponseData::mPostResponseData()
    {

    }

    mPostResponseData::~mPostResponseData()
    {

    }

    mPostForceData::mPostForceData()
    {

    }

    mPostForceData::~mPostForceData()
    {

    }

    mPostTransferPathAnalysisData::mPostTransferPathAnalysisData()
    {

    }

    mPostTransferPathAnalysisData::~mPostTransferPathAnalysisData()
    {
        
    }

    QStringList mPostTransferPathAnalysisData::removeDuplicates(const QStringList& list)
    {
        QSet<QString> seen;
        QStringList result;

        for (const QString& item : list) {
            if (!seen.contains(item)) {
                seen.insert(item);
                result.append(item);
            }
        }
        return result;
    }

    QVector<int> mPostTransferPathAnalysisData::removeDuplicates(const QVector<int>& vector)
    {
        QSet<int> seen;
        QVector<int> result;

        for (const int& item : vector) {
            if (!seen.contains(item)) {
                seen.insert(item);
                result.push_back(item);
            }
        }
        return result;
    }

    QVector<int> mPostTransferPathAnalysisData::getAllSubcaseID()
    {
        QVector<int> ids;
        for (auto responseData : _allResponseData)
        {
            ids.push_back(responseData->_subcaseID);
        }
        return ids;
    }

    QStringList mPostTransferPathAnalysisData::getResultTypeBySubcaseID(int caseId)
    {
        QStringList resultList;
        for (auto responseData : _allResponseData)
        {
            if (responseData->_subcaseID == caseId)
            {
                resultList.append(responseData->_type);
            }
        }
        return resultList;
    }

    QVector<int> mPostTransferPathAnalysisData::getResponseIDBySubcaseIDAndResultType(int caseID, QString resType)
    {
        QVector<int> ids;
        for (auto responseData : _allResponseData)
        {
            if (responseData->_subcaseID == caseID && responseData->_type == resType)
            {
                for (int id : responseData->_nodeId)
                {
                    ids.push_back(id);
                }
            }
        }
        return ids;
    }

    QVector<double> mPostTransferPathAnalysisData::getAllFrequency(int caseID, QString resType, int responseID)
    {
        QVector<double> fres;
        for (auto responseData : _allResponseData)
        {
            if (responseData->_subcaseID == caseID && responseData->_type == resType && responseData->_nodeId.contains(responseID))
            {
                if (responseData->_resultType == 1)
                {
                    fres.push_back(responseData->_frequency[0]);
                }
                else if (responseData->_resultType == 2)
                {
                    fres = responseData->_frequency;
                    break;
                }
            }
        }
        _allFrequence = fres;
        return fres;
    }

    QVector<QPair<double, double>> mPostTransferPathAnalysisData::getAllSolverResponseData(int caseID, QString resType, int responseID, int componentindex)
    {
        QVector<QPair<double, double>> allSolverResponseData;
        for (auto responseData : _allResponseData)
        {
            QPair<double, double> pairval;
            if (responseData->_subcaseID == caseID && responseData->_type == resType && responseData->_nodeId.contains(responseID))
            {
                //按频率，有多个节点
                if (responseData->_resultType == 1)
                {
                    int index = responseData->_nodeId.indexOf(responseID);
                    pairval = responseData->_allComponentValue[index].value(componentindex);
                    allSolverResponseData.push_back(pairval);
                }
                //按节点，多个频率
                else if (responseData->_resultType == 2)
                {
                    for (auto comval : responseData->_allComponentValue)
                    {
                        allSolverResponseData.push_back(comval.value(componentindex));
                    }
                    break;
                }
            }
        }
        return allSolverResponseData;
    }

    QVector<QPair<double, double>> mPostTransferPathAnalysisData::getAllSolverResponseDataByRange(int caseID, QString resType, int responseID, int componentindex, double min, double max)
    {
        QVector<QPair<double, double>> allData = getAllSolverResponseData(caseID, resType, responseID, componentindex);
        QVector<double> rangeFreq = getFrequenceByRange(min, max);
        if (rangeFreq.isEmpty()) return {};

        int startIdx = -1, endIdx = -1;
        for (int i = 0; i < _allFrequence.size(); ++i)
        {
            if (qFuzzyCompare(_allFrequence[i], rangeFreq.first())) { startIdx = i; break; }
        }
        for (int i = _allFrequence.size() - 1; i >= 0; --i)
        {
            if (qFuzzyCompare(_allFrequence[i], rangeFreq.last())) { endIdx = i; break; }
        }
        if (startIdx == -1 || endIdx == -1 || startIdx > endIdx) return {};

        QVector<QPair<double, double>> rangeData;
        for (int i = startIdx; i <= endIdx; ++i)
        {
            rangeData.push_back(allData[i]);
        }
        return rangeData;
    }

    QStringList mPostTransferPathAnalysisData::getAllNodePathName(QString resType, int responseID)
    {
        QStringList allpath;
        for (auto transferdata : _allTransferFunctionData)
        {
            if (transferdata->_type == resType && transferdata->_nodeId.contains(responseID))
            {
                allpath.append(transferdata->_transferPath);
            }
        }
        return allpath;
    }

    QHash<QString, QVector<QPair<double, double>>> mPostTransferPathAnalysisData::getAllFrequencyTranseferData(QStringList paths, int responseID, int componentindex)
    {
        QHash<QString, QVector<QPair<double, double>>> allTransferData;
        for (QString pathname : paths)
        {
            QVector<QPair<double, double>> componentData;
            for (auto transferData : _allTransferFunctionData)
            {
                if (transferData->_resultType == 1)
                {
                    if (transferData->_transferPath == pathname && transferData->_nodeId.contains(responseID))
                    {
                        QPair<double, double> pairval;
                        int index = transferData->_nodeId.indexOf(responseID);
                        pairval = transferData->_allComponentValue[index].value(componentindex);
                        componentData.push_back(pairval);
                    }
                }
                else if (transferData->_resultType == 2)
                {
                    if (transferData->_transferPath == pathname && transferData->_nodeId.contains(responseID))
                    {
                        for (auto comval : transferData->_allComponentValue)
                        {
                            componentData.push_back(comval.value(componentindex));
                        }
                        break;
                    }
                }
            }
            allTransferData[pathname] = componentData;
        }
        return allTransferData;
    }

    QHash<QString, QVector<QPair<double, double>>> mPostTransferPathAnalysisData::getAllFrequencyTranseferDataByRange(QStringList paths, int responseID, int componentindex, double min, double max)
    {
        QHash<QString, QVector<QPair<double, double>>> allData = getAllFrequencyTranseferData(paths, responseID, componentindex);
        QVector<double> rangeFreq = getFrequenceByRange(min, max);
        if (rangeFreq.isEmpty()) return {};

        int startIdx = -1, endIdx = -1;
        for (int i = 0; i < _allFrequence.size(); ++i)
        {
            if (qFuzzyCompare(_allFrequence[i], rangeFreq.first())) { startIdx = i; break; }
        }
        for (int i = _allFrequence.size() - 1; i >= 0; --i)
        {
            if (qFuzzyCompare(_allFrequence[i], rangeFreq.last())) { endIdx = i; break; }
        }
        if (startIdx == -1 || endIdx == -1 || startIdx > endIdx) return {};

        QHash<QString, QVector<QPair<double, double>>> rangeData;
        for (auto path : paths)
        {
            QVector<QPair<double, double>> pathData = allData.value(path);
            QVector<QPair<double, double>> rangePathData;
            for (int i = startIdx; i <= endIdx; ++i)
            {
                rangePathData.push_back(pathData[i]);
            }
            rangeData[path] = rangePathData;
        }
        return rangeData;
    }

    QHash<QString, QVector<QPair<double, double>>> mPostTransferPathAnalysisData::getAllFrequencyForceData(QStringList paths)
    {
        QHash<QString, QVector<QPair<double, double>>> allForceData;
        for (QString pathname : paths)
        {
            QVector<QPair<double, double>> componentData;
            int elementID;
            int componentindex;
            //截取路径的单元ID和自由度
            QStringList pathparts = pathname.split("<>");
            if (pathparts.size() < 2) return{};
            QString elestr = pathparts[1];
            if (elestr.isEmpty())
                return {};
            QStringList eleparts = elestr.split(":");
            if (eleparts.size() == 2)
            {
                elementID = eleparts[0].toInt();
                componentindex = getForceIndex(eleparts[1]);
            }

            for (auto forceData : _allForceData)
            {
                //单元力
                if (forceData->_forceType == 1)
                {
                    if (forceData->_resultType == 1)
                    {
                        if (forceData->_elementId.contains(elementID))
                        {
                            QPair<double, double> pairval;
                            int index = forceData->_elementId.indexOf(elementID);
                            pairval = forceData->_allComponentValue[index].value(componentindex);
                            componentData.push_back(pairval);
                        }
                    }
                    else if (forceData->_resultType == 2)
                    {
                        if (forceData->_elementId.contains(elementID))
                        {
                            for (auto comval : forceData->_allComponentValue)
                            {
                                componentData.push_back(comval.value(componentindex));
                            }
                            break;
                        }
                    }
                }
                //节点力
                else if (forceData->_forceType == 2)
                {
                    for (int i = 0; i < forceData->_nodeElementID.size(); i++)
                    {
                        if (forceData->_nodeElementID[i].second == elementID)
                        {
                            QPair<double, double> pairval;
                            int index = i;
                            pairval = forceData->_allComponentValue[index].value(componentindex);
                            componentData.push_back(pairval);
                        }
                    }
                }
            }
            allForceData[pathname] = componentData;
        }
        return allForceData;
    }

    QHash<QString, QVector<QPair<double, double>>> mPostTransferPathAnalysisData::getAllFrequencyForceData(QStringList paths,int subcaseid)
    {
        QHash<QString, QVector<QPair<double, double>>> allForceData;
        for (QString pathname : paths)
        {
            QVector<QPair<double, double>> componentData;
            int elementID;
            int componentindex;
            //截取路径的单元ID和自由度
            QStringList pathparts = pathname.split("<>");
            if (pathparts.size() < 2) return{};
            QString elestr = pathparts[1];
            if (elestr.isEmpty())
                return {};
            QStringList eleparts = elestr.split(":");
            if (eleparts.size() == 2)
            {
                elementID = eleparts[0].toInt();
                componentindex = getForceIndex(eleparts[1]);
            }

            for (auto forceData : _allForceData)
            {
                //单元力
                if (forceData->_forceType == 1)
                {
                    if (forceData->_resultType == 1)
                    {
                        if (forceData->_elementId.contains(elementID))
                        {
                            QPair<double, double> pairval;
                            int index = forceData->_elementId.indexOf(elementID);
                            pairval = forceData->_allComponentValue[index].value(componentindex);
                            componentData.push_back(pairval);
                        }
                    }
                    else if (forceData->_resultType == 2)
                    {
                        if (forceData->_elementId.contains(elementID))
                        {
                            for (auto comval : forceData->_allComponentValue)
                            {
                                componentData.push_back(comval.value(componentindex));
                            }
                            break;
                        }
                    }
                }
                //节点力
                else if (forceData->_forceType == 2)
                {
                    int a = forceData->_nodeElementID.size();
                    for (int i = 0; i < forceData->_nodeElementID.size(); i++)
                    {
                        int b = forceData->_nodeElementID[i].second;
                        if (forceData->_nodeElementID[i].second == elementID && forceData->_subcaseID == subcaseid)
                        {
                            QPair<double, double> pairval;
                            int index = i;
                            pairval = forceData->_allComponentValue[index].value(componentindex);
                            componentData.push_back(pairval);
                        }
                    }
                }
            }
            allForceData[pathname] = componentData;
        }
        return allForceData;
    }


    QHash<QString, QVector<QPair<double, double>>> mPostTransferPathAnalysisData::getAllFrequencyForceDataByRange(QStringList paths, double min, double max)
    {
        QHash<QString, QVector<QPair<double, double>>> allData = getAllFrequencyForceData(paths);
        QVector<double> rangeFreq = getFrequenceByRange(min, max);
        if (rangeFreq.isEmpty()) return {};

        int startIdx = -1, endIdx = -1;
        for (int i = 0; i < _allFrequence.size(); ++i)
        {
            if (qFuzzyCompare(_allFrequence[i], rangeFreq.first())) { startIdx = i; break; }
        }
        for (int i = _allFrequence.size() - 1; i >= 0; --i)
        {
            if (qFuzzyCompare(_allFrequence[i], rangeFreq.last())) { endIdx = i; break; }
        }
        if (startIdx == -1 || endIdx == -1 || startIdx > endIdx) return {};

        QHash<QString, QVector<QPair<double, double>>> rangeData;
        for (auto path : paths)
        {
            QVector<QPair<double, double>> pathData = allData.value(path);
            QVector<QPair<double, double>> rangePathData;
            for (int i = startIdx; i <= endIdx; ++i)
            {
                rangePathData.push_back(pathData[i]);
            }
            rangeData[path] = rangePathData;
        }
        return rangeData;
    }

    int mPostTransferPathAnalysisData::getForceIndex(QString str)
    {
        if (str == "+X")
        {
            return 0;
        }
        else if (str == "+Y")
        {
            return 1;
        }
        else if (str == "+Z")
        {
            return 2;
        }
        else if (str == "+RX")
        {
            return 3;
        }
        else if (str == "+RY")
        {
            return 4;
        }
        else if (str == "+RZ")
        {
            return 5;
        }
    }

    int mPostTransferPathAnalysisData::getTransferFileType()
    {
        if (_allTransferFunctionData.size() > 0)
        {
            return _allTransferFunctionData[0]->_outputType;
        }
        return -1;
    }

    int mPostTransferPathAnalysisData::getForceFileType()
    {
        if (_allForceData.size() > 0)
        {
            return _allForceData[0]->_outputType;
        }
        return -1;
    }

    QVector<double> mPostTransferPathAnalysisData::getFrequenceByRange(double min, double max)
    {
        QVector<double> frequence;
        for (double fre : _allFrequence)
        {
            if ((fre - min >= 0) && (fre - max <= 0))
            {
                frequence.push_back(fre);
            }
        }
        return frequence;
    }

    QVector<double> mPostTransferPathAnalysisData::getTotalResponseByFrequence(int caseID, QString resType, int gridid, QString component, QVector<double> fres)
    {
        QVector<double> vecResponse;
        //for (auto frequenceData : _allResponseData)
        //{
        //    for (double fre : fres)
        //    {
        //        double res = frequenceData->getTotalResponseByFrequence(caseID, resType, gridid, component, fre);
        //        if (res != -1)
        //        {
        //            vecResponse.push_back(res);
        //        }
        //    }
        //}
        return vecResponse;
    }

}