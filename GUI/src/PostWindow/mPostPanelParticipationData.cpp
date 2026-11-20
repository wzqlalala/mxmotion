#include "mPostPanelParticipationData.h"

#include <string>
#include <iostream>
#include <QSet>
#include <qmath.h>

using namespace std;
namespace PostWindow {
    mPostFrequencyPanelData::mPostFrequencyPanelData()
    {

    }

    mPostFrequencyPanelData::~mPostFrequencyPanelData()
    {

    }

    int mPostFrequencyPanelData::getGridIDBySubcaseIDAndResultType(int caseID, QString resType)
    {
        if (subcaseId_ == caseID && participationFactors_.contains(resType))
        {
            return gridID_;
        }
        return 0;
    }

    bool mPostFrequencyPanelData::getComponentBySubcaseIDAndResultType(int caseID, QString resType, int id, QString &compo)
    {
        if (subcaseId_ == caseID && participationFactors_.contains(resType)&& gridID_ == id)
        {
            compo = component_;
            return true;
        }
        return false;
    }

    bool mPostFrequencyPanelData::getFrequencyBySubcaseIDAndResultType(int caseID, QString resType, int gridid, QString component,double &fre)
    {
        if (subcaseId_ == caseID && participationFactors_.contains(resType) && gridID_ == gridid && component_ == component)
        {
            fre = frequence_;
            return true;
        }
        return false;
    }

    bool mPostFrequencyPanelData::getTotalResponseBySubcaseIDAndResultType(int caseID, QString resType, int gridid, QString component, double &totalRes)
    {
        if (subcaseId_ == caseID && participationFactors_.contains(resType) && gridID_ == gridid && component_ == component)
        {
            totalRes = totalResponse_;
            return true;
        }
        return false;
    }

    QSet<QString> mPostFrequencyPanelData::getPanelIndexBySubcaseIDAndResultType(int caseID, QString resType)
    {
        QSet<QString> allPanel;
        if (subcaseId_ == caseID && participationFactors_.contains(resType))
        {
            QList<QString> panels = panelResponse_.keys();
            for (QString panel : panels)
            {
                allPanel.insert(panel);
            }
        }
        return allPanel;
    }

    double mPostFrequencyPanelData::getTotalResponseByFrequence(int caseID, QString resType, int gridid, QString component, double fre)
    {
        if (subcaseId_ == caseID && participationFactors_.contains(resType)&& frequence_ == fre && gridID_ == gridid && component_ == component)
        {
            return totalResponse_;
        }
        return -1;
    }

    double mPostFrequencyPanelData::getNewResponseAfterExcludePanel(int caseID, QString resType, int grid, QString com, double fre, QVector<QString> excludeIndex, double ratio)
    {
        if (subcaseId_ == caseID && participationFactors_.contains(resType) && frequence_ == fre && gridID_ == grid && component_ == com)
        {
            double radians = qDegreesToRadians(totalAngle_);
            double totalReal = totalResponse_ * qCos(radians);
            double totalImag = totalResponse_ * qSin(radians);

            double totalPanellReal = 0;
            double totalPanellImag = 0;
            double panelReal = 0;
            double panelImag = 0;
            for (QString index : excludeIndex)
            {
                panelReal = panelResponse_.value(index).first;
                panelImag = panelResponse_.value(index).second;
                totalPanellReal += panelReal * ratio/100;
                totalPanellImag += panelImag * ratio/100;
            }
            double totalRealExc = totalReal - totalPanellReal;
            double totalImagExc = totalImag - totalPanellImag;

            double totalExc = sqrt(totalRealExc*totalRealExc + totalImagExc* totalImagExc);
            return totalExc;
        }
        return -1;
    }

    void mPostFrequencyPanelData::getPanelPercentByFrequence(int caseID, QString resType, double frequence, int gridid, QString component, QList<QPair<QString, double>> &panelPercents)
    {
        if (subcaseId_ == caseID && participationFactors_.contains(resType) && frequence_ == frequence && gridID_ == gridid && component_ == component)
        {
            for (auto it = panelResponse_.constBegin(); it != panelResponse_.constEnd(); ++it)
            {
                //面板名称
                QString panel = it.key();

                //计算面板对应百分比
                //实部、虚部
                double panelReal = it.value().first;
                double panelImag = it.value().second;
                //幅值
                double magnitude = sqrt(panelReal * panelReal + panelImag * panelImag);
                //角度
                double angle = acos(panelReal / magnitude) * 180 / M_PI;
                if (panelImag < 0)
                {
                    angle = 360 - angle;
                }
                double Angle = angle - totalAngle_;
                //百分比
                double angleRadains = Angle * (M_PI / 180);
                double cosin = cos(angleRadains);
                double percent = magnitude * cosin / totalResponse_ * 100;

                panelPercents.append(qMakePair(panel,percent));
            }
        }
    }

    void mPostFrequencyPanelData::getPanelMagnitudeByFrequence(int caseID, QString resType, double frequence, int gridid, QString com, QList<QPair<QString, double>> &panelMagnitude)
    {
        if (subcaseId_ == caseID && participationFactors_.contains(resType) && frequence_ == frequence && gridID_ == gridid && component_ == com)
        {
            for (auto it = panelResponse_.constBegin(); it != panelResponse_.constEnd(); ++it)
            {
                //面板名称
                QString panel = it.key();

                //计算面板对应幅值
                //实部、虚部
                double panelReal = it.value().first;
                double panelImag = it.value().second;
                //幅值
                double magnitude = sqrt(panelReal * panelReal + panelImag * panelImag);

                panelMagnitude.append(qMakePair(panel, magnitude));
            }
        }
    }

    mPostPanelParticipationData::mPostPanelParticipationData()
    {

    }

    mPostPanelParticipationData::~mPostPanelParticipationData()
    {
        
    }

    QVector<int> mPostPanelParticipationData::getAllSubcaseID()
    {
        QVector<int> ids;
        for (auto frequenceData : allFrequencyPanelData_)
        {
            ids.append(frequenceData->subcaseId_);
        }
        return ids;
        
    }

    QStringList mPostPanelParticipationData::getResultTypeBySubcaseID(int caseId)
    {
        QStringList result;
        for (auto Data : allFrequencyPanelData_)
        {
            if (Data->subcaseId_ == caseId)
            {
                result.append(Data->participationFactors_);
            }
        }
        return result;
    }

    QStringList mPostPanelParticipationData::removeDuplicates(const QStringList& list)
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

    QVector<int> mPostPanelParticipationData::removeDuplicates(const QVector<int>& vector)
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

    QVector<int> mPostPanelParticipationData::getGridIDBySubcaseIDAndResultType(int caseID, QString resType)
    {
        QVector<int> ids;
        for (auto frequenceData : allFrequencyPanelData_)
        {
            if (frequenceData->subcaseId_ == caseID && frequenceData->participationFactors_.contains(resType))
            {
                ids.push_back(frequenceData->gridID_);
            }
        }
        return ids;
    }

    QStringList mPostPanelParticipationData::getComponentBySubcaseIDAndResultType(int caseID, QString resType, int gridid)
    {
        QStringList str;
        for (auto frequenceData : allFrequencyPanelData_)
        {
            QString component;
            bool success = frequenceData->getComponentBySubcaseIDAndResultType(caseID, resType, gridid, component);
            if (success)
            {
                str.append(component);
            }
        }
        return str;
    }

    QVector<double> mPostPanelParticipationData::getAllFrequencyBySubcaseIDAndResultType(int caseID, QString resType, int gridid, QString component)
    {
        QVector<double> vecFrequence;
        for (auto frequenceData : allFrequencyPanelData_)
        {
            double frequence = 0.0;
            bool success = frequenceData->getFrequencyBySubcaseIDAndResultType(caseID, resType, gridid, component,frequence);
            if (success)
            {
                vecFrequence.push_back(frequence);
            }
        }
        allFrequence_ = vecFrequence;
        return vecFrequence;
    }

    QVector<double> mPostPanelParticipationData::getAllTotalResponseBySubcaseIDAndResultType(int caseID, QString resType, int gridid, QString component)
    {
        QVector<double> vecTotalResponse;
        for (auto frequenceData : allFrequencyPanelData_)
        {
            double totalResponse = 0.0;
            bool success = frequenceData->getTotalResponseBySubcaseIDAndResultType(caseID, resType, gridid, component, totalResponse);
            if (success)
            {
                vecTotalResponse.push_back(totalResponse);
            }
        }
        return vecTotalResponse;
    }

    QSet<QString> mPostPanelParticipationData::getPanelIndexBySubcaseIDAndResultType(int caseID, QString resType)
    {
        QSet<QString> panellIndex;
        for (auto frequenceData : allFrequencyPanelData_)
        {
            panellIndex.unite(frequenceData->getPanelIndexBySubcaseIDAndResultType(caseID, resType));
        }
        return panellIndex;
    }

    QVector<double> mPostPanelParticipationData::getFrequenceByRange(double min, double max)
    {
        QVector<double> frequence;
        for (double fre : allFrequence_)
        {
            if ((fre - min >= 0) && (fre - max <= 0) )
            {
                frequence.push_back(fre);
            }
        }
        return frequence;
    }

    QVector<double> mPostPanelParticipationData::getTotalResponseByFrequence(int caseID, QString resType, int gridid, QString component, QVector<double> fres)
    {
        QVector<double> vecResponse;
        vecResponse.resize(fres.size());
        vecResponse.fill(-1);
        for (auto frequenceData : allFrequencyPanelData_)
        {
            for (int i = 0; i < fres.size(); i++)
            {
                double res = frequenceData->getTotalResponseByFrequence(caseID, resType, gridid, component, fres[i]);
                if (res != -1)
                {
                    vecResponse[i] = res;
                    break;
                }
            }
        }
        return vecResponse;
    }

    double mPostPanelParticipationData::getTotalResponseByFrequence(int caseID, QString resType, int gridid, QString component, double fre)
    {
        double res;
        for (auto frequenceData : allFrequencyPanelData_)
        {
            res = frequenceData->getTotalResponseByFrequence(caseID, resType, gridid, component, fre);
            if (res != -1)
            {
                return res;
            }
        }
        return -1;
    }

    QVector<double> mPostPanelParticipationData::getNewResponseAfterExcludePanel(int caseID, QString resType, int grid, QString com, QVector<double> fres, QVector<QString> excludeIndex, double ratio)
    {
        QVector<double> vecResponse;
        vecResponse.resize(fres.size());
        vecResponse.fill(-1);
        for (auto frequenceData : allFrequencyPanelData_)
        {
            for (int i = 0; i < fres.size(); i++)
            {
                double res = frequenceData->getNewResponseAfterExcludePanel(caseID, resType, grid, com, fres[i], excludeIndex, ratio);
                if (res != -1)
                {
                    vecResponse[i] = res;
                    break;
                }
            }
        }
        return vecResponse;
    }

    void mPostPanelParticipationData::getPanelPercentByFrequence(int caseID, QString resType, double frequence, int grid, QString com,QList<QPair<QString, double>> &panelPercents)
    {
        for (auto frequenceData : allFrequencyPanelData_)
        {
            frequenceData->getPanelPercentByFrequence(caseID, resType, frequence, grid, com, panelPercents);
        }
    }

    void mPostPanelParticipationData::getPanelMagnitudeByFrequence(int caseID, QString resType, double frequence, int gridid, QString com, QList<QPair<QString, double>> &panelMagnitude)
    {
        for (auto frequenceData : allFrequencyPanelData_)
        {
            frequenceData->getPanelMagnitudeByFrequence(caseID, resType, frequence, gridid, com, panelMagnitude);
        }
    }

    QList<QString> mPostPanelParticipationData::getAllPanelName(int caseID, QString resType)
    {
        QList<QString> panels;
        for (auto iniData : allFrequencyPanelData_)
        {
            if (iniData->subcaseId_ == caseID &&  iniData->participationFactors_ == resType)
            {
                for (int i = 0; i < iniData->panelResponse_.size(); i++)
                {
                    panels = iniData->panelResponse_.keys();
                }
            }
        }
        return panels;
    }

    void mPostPanelParticipationData::getAllFreAndMagnitudeByPanel(int caseID, QString resType, QString panel, QVector<double> &fres, int gridid, QString com, QVector<double> &magnitudes)
    {
        for (auto frequenceData : allFrequencyPanelData_)
        {
            if (caseID == frequenceData->subcaseId_ && resType == frequenceData->participationFactors_ && gridid == frequenceData->gridID_ && com == frequenceData->component_)
            {
                if (frequenceData->panelResponse_.keys().contains(panel))
                {
                    fres.push_back(frequenceData->frequence_);
                    QPair<double,double> response = frequenceData->panelResponse_.value(panel);
                    double magnitude = sqrt(pow(response.first, 2) + pow(response.second, 2));
                    magnitudes.push_back(magnitude);
                }
            }
        }
    }

    //double mPostPanelParticipationData::getInitialPanelFrequency(int caseID, int panel)
    //{
    //    for (auto iniData : allIniPanelData_)
    //    {
    //        if (iniData->subcaseID == caseID && iniData->initialPanelFrequnecy.contains(panel))
    //        {
    //            return iniData->initialPanelFrequnecy.value(panel);
    //        }
    //    }
    //    return -1.0;
    //}

    initialPanelData::initialPanelData()
    {

    }

    initialPanelData::~initialPanelData()
    {

    }

}