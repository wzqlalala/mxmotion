#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QString>

namespace MAppConfiguration {
// Original application configuration names and Settings.ini location. Paths
// are real filesystem locations used by the original working-directory and
// external-flex-export workflows.
class mAppConfiguration {
public:
    static mAppConfiguration* getInstance() { static mAppConfiguration value; return &value; }
    QString _workingDir;
    QString _recentDir;
    int _language = 0;
private:
    mAppConfiguration() {
        QSettings settings(QCoreApplication::applicationDirPath()+QStringLiteral("/../Setting/Settings.ini"),QSettings::IniFormat);
        const QString fallback=QDir(QCoreApplication::applicationDirPath()+QStringLiteral("/../WorkingDir")).absolutePath();
        _workingDir=settings.value(QStringLiteral("WorkingDir/WorkingDir"),fallback).toString();
        if(_workingDir.isEmpty())_workingDir=fallback;
        _recentDir=settings.value(QStringLiteral("RecentDir/RecentDir"),_workingDir).toString();
        _language=settings.value(QStringLiteral("Language/Language"),0).toInt();
    }
};
}
