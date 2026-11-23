#pragma once
#include "mPickWidgetData.h"
#include <functional>
#include <memory>

namespace MechMainWindow {
using PersistencePickMap=std::unordered_map<MxFunctions::PickDataType,std::vector<RenderPickResultData>>;
class IPickPersistenceStrategy {
public:
    virtual ~IPickPersistenceStrategy()=default;
    virtual void save(const PersistencePickMap& data)=0;
    virtual void saveSingleSet(const QString&) {}
    virtual QString strategyName() const { return QStringLiteral("IPickPersistenceStrategy"); }
};
class DataClassPersistenceStrategy : public IPickPersistenceStrategy {
public:
    using DataWriter=std::function<void(const PersistencePickMap&)>;
    using SetResolver=std::function<PersistencePickMap(const QString&)>;
    explicit DataClassPersistenceStrategy(DataWriter writer,SetResolver resolver={})
        : m_writer(std::move(writer)),m_resolver(std::move(resolver)) {}
    void save(const PersistencePickMap& data) override { if(m_writer) m_writer(data); }
    void saveSingleSet(const QString& name) override { if(m_writer && m_resolver) m_writer(m_resolver(name)); }
    QString strategyName() const override { return QStringLiteral("DataClassPersistenceStrategy"); }
private:
    DataWriter m_writer;
    SetResolver m_resolver;
};
} // namespace MechMainWindow
