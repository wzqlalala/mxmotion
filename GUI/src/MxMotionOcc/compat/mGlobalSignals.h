#pragma once
#include <QObject>
#include <QString>
namespace MxFunctions {
class mGlobalSignals : public QObject {
    Q_OBJECT
public:
    static mGlobalSignals* getInstance() { static mGlobalSignals value; return &value; }
signals:
    void updateTranslator(int);
    void outputMessageSig(const QString& text, bool error=false);
    void outputMessageSig(int level,const QString& text,bool popup=false);
    void currentViewportChanged();
};
}
