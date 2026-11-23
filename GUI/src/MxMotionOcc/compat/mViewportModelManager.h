#pragma once
#include "mViewportModel.h"
#include <QPointer>
namespace MechPreData {
class mViewportModelManager : public QObject {
    Q_OBJECT
public:
    static mViewportModelManager* getInstance() { static mViewportModelManager value;return &value; }
    mViewportModel* getCurrentViewportModel() const { return current_; }
    mViewportModel* getViewportModel(int id) const { return current_ && current_->getModelData()->getModelDataID()==id ? current_.data() : nullptr; }
    void setCurrentViewportModel(mViewportModel* model) { if(current_==model)return;current_=model;emit currentViewportChanged(model ? model->getModelData()->getModelDataID() : -1); }
signals:
    void currentViewportChanged(int viewportID);
private: QPointer<mViewportModel> current_;
};
}
#define getCVM (MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel())
