#pragma once
#include <QObject>
#include <QString>
#include <array>
namespace CommonData {
// Original index order; all imported OCC coordinates remain millimetres.
inline const QString UnitSystems[8][7] = {
    {"m-kg-s","m","kg","s","N","Pa","m/s²"},
    {"cm-g-s","cm","g","s","dyn","dyn/cm²","cm/s²"},
    {"mm-kg-s","mm","kg","s","mN","kPa","mm/s²"},
    {"mm-t-s","mm","t","s","N","MPa","mm/s²"},
    {"mm-g-s","mm","g","s","µN","Pa","mm/s²"},
    {"µm-kg-s","µm","kg","s","µN","MPa","µm/s²"},
    {"ft-slug-s","ft","slug","s","lbf","lbf/ft²","ft/s²"},
    {"in-lbm-s","in","lbm","s","lbf","psi","in/s²"}
};
class mUnitSystemManager : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    const std::array<int,4>& getCurUnitsIndex() const { return current_; }
    const std::array<int,4>& getHisUnitsIndex() const { return previous_; }
    void setUnitIndex(int field,int index) {
        if(field<0 || field>3 || index<0 || (field==0 && index>7) || current_[field]==index)return;
        previous_=current_;current_[field]=index;emit sg_unitSystemChanged(index);
    }
signals: void sg_unitSystemChanged(int);
private: std::array<int,4> current_{{2,0,0,0}},previous_{{2,0,0,0}};
};
}
