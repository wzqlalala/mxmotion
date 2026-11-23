#include "LegacyMotionExtensionCodec.h"
#include "mModelData.h"
#include <QJsonArray>
#include <stdexcept>
using namespace MechMotionData;
using namespace MxFunctions;
namespace MotionOcc {
namespace {
const QStringList coSimNames{"Mechatronics","Simulink","FMU","Custom"};
const QStringList exportNames{"PortList","Signal"};
const QStringList hydraulicNames{"ValveControl","PumpControl","Custom"};
const QStringList algorithmNames{"PPO","SAC","DDPG","TD3","A3C"};
const QStringList environmentNames{"Motion","Trajectory","Force","Hybrid"};
int namedIndex(const QJsonValue& value,const QStringList& names) {
    if(value.isUndefined())return 0;
    const int i=names.indexOf(value.toString());
    if(i<0)throw std::runtime_error(QString("未知配置类型：%1").arg(value.toVariant().toString()).toStdString());return i;
}
int objectId(const QJsonObject& row) {
    const int id=row.value("id").toInt(-1);if(id<0)throw std::runtime_error("Original GUI extension requires a nonnegative ID");return id;
}
QVector<QPair<QString,QString>> readPortPairs(const QJsonValue& value) {
    QVector<QPair<QString,QString>> out;for(const auto& item:value.toArray()) {
        const auto p=item.toObject();out.append({p.value("portName").toString(),p.value("motionPort").toString()});
    } return out;
}
QJsonArray writePortPairs(const QVector<QPair<QString,QString>>& ports) {
    QJsonArray out;for(const auto& port:ports)out.append(QJsonObject{{"portName",port.first},{"motionPort",port.second}});return out;
}
QVector<mMotionElectroExport::PortConfig> readExportPorts(const QJsonValue& value) {
    QVector<mMotionElectroExport::PortConfig> out;for(const auto& item:value.toArray()) {
        const auto p=item.toObject();out.append({p.value("portName").toString(),p.value("order").toInt(),p.value("scale").toDouble(1.),p.value("offset").toDouble(),p.value("unit").toString()});
    } return out;
}
QJsonArray writeExportPorts(const QVector<mMotionElectroExport::PortConfig>& ports) {
    QJsonArray out;for(const auto& p:ports)out.append(QJsonObject{{"portName",p.portName},{"order",p.order},{"scale",p.scale},{"offset",p.offset},{"unit",p.unit}});return out;
}
template<class T> QVector<T> readValues(const QJsonValue& value) {
    QVector<T> out;for(const auto& item:value.toArray())if constexpr(std::is_same_v<T,QString>)out.append(item.toString());else out.append(item.toInt());return out;
}
template<class T> QJsonArray writeValues(const QVector<T>& value) { QJsonArray out;for(const auto& item:value)out.append(item);return out; }
}
void loadLegacyMotionExtensions(const QJsonObject& data,MechModelData::mModelData& model) {
    for(const auto& item:data.value("electroImports").toArray()) {
        const auto row=item.toObject();auto p=std::make_unique<mMotionElectroImport>(objectId(row),row.value("name").toString());
        p->setCoSimType(CoSimType(namedIndex(row.value("coSimType"),coSimNames)));
        p->setPlatform(row.value("platform").toString());p->setModelFile(row.value("modelFile").toString());p->setHeaderFile(row.value("headerFile").toString());
        p->setInputPorts(readPortPairs(row.value("inputPorts")));p->setOutputPorts(readPortPairs(row.value("outputPorts")));
        model.addPhysicalField(p.release());
    }
    for(const auto& item:data.value("electroExports").toArray()) {
        const auto row=item.toObject();auto p=std::make_unique<mMotionElectroExport>(objectId(row),row.value("name").toString());
        p->setExportType(ExportType(namedIndex(row.value("exportType"),exportNames)));p->setPortListFile(row.value("portListFile").toString());
        p->setInputPorts(readExportPorts(row.value("inputPorts")));p->setOutputPorts(readExportPorts(row.value("outputPorts")));model.addPhysicalField(p.release());
    }
    for(const auto& item:data.value("hydraulics").toArray()) {
        const auto row=item.toObject();auto p=std::make_unique<mMotionHydraulic>(objectId(row),row.value("name").toString());
        p->setHydraulicType(HydraulicType(namedIndex(row.value("hydraulicType"),hydraulicNames)));p->setModelFile(row.value("modelFile").toString());
        p->setFluidDensity(row.value("fluidDensity").toDouble(p->getFluidDensity()));p->setFluidViscosity(row.value("fluidViscosity").toDouble(p->getFluidViscosity()));
        p->setBulkModulus(row.value("bulkModulus").toDouble(p->getBulkModulus()));
        QVector<mMotionHydraulic::PortInfo> ports;for(const auto& v:row.value("ports").toArray()) { const auto r=v.toObject();ports.append({r.value("portName").toString(),r.value("diameter").toDouble(10),r.value("length").toDouble(100)}); }
        QVector<mMotionHydraulic::PipeInfo> pipes;for(const auto& v:row.value("pipes").toArray()) { const auto r=v.toObject();pipes.append({r.value("pipeName").toString(),r.value("diameter").toDouble(10),r.value("length").toDouble(100)}); }
        p->setPorts(ports);p->setPipes(pipes);model.addPhysicalField(p.release());
    }
    for(const auto& item:data.value("rlTasks").toArray()) {
        const auto row=item.toObject();auto p=std::make_unique<mMotionRL>(objectId(row),row.value("name").toString());
        p->setAlgorithm(RLAlgorithm(namedIndex(row.value("algorithm"),algorithmNames)));p->setEnvType(RLEnvType(namedIndex(row.value("environmentType"),environmentNames)));
        p->setStateSpace(readValues<QString>(row.value("stateSpace")));p->setActionSpace(readValues<QString>(row.value("actionSpace")));p->setHiddenLayers(readValues<int>(row.value("hiddenLayers")));
        p->setLearningRate(row.value("learningRate").toDouble(p->getLearningRate()));p->setDiscountFactor(row.value("discountFactor").toDouble(p->getDiscountFactor()));
        p->setMaxEpisodes(row.value("maxEpisodes").toInt(p->getMaxEpisodes()));p->setMaxSteps(row.value("maxSteps").toInt(p->getMaxSteps()));
        p->setDatasetPath(row.value("datasetPath").toString());p->setModelSavePath(row.value("modelSavePath").toString());model.addPhysicalField(p.release());
    }
}
void saveLegacyMotionExtensions(QJsonObject& data,MechModelData::mModelData& model) {
    QJsonArray imports,exports,hydraulics,tasks;
    auto& importManager=model.getManager<mMotionElectroImportManager>(Physical_MotionElectroImport);
    for(int id:importManager.getAllIDs()) { const auto* p=importManager.getData(id);imports.append(QJsonObject{
        {"id",id},{"name",p->getName()},{"coSimType",coSimNames.value(int(p->getCoSimType()))},{"platform",p->getPlatform()},
        {"modelFile",p->getModelFile()},{"headerFile",p->getHeaderFile()},{"inputPorts",writePortPairs(p->getInputPorts())},{"outputPorts",writePortPairs(p->getOutputPorts())}}); }
    auto& exportManager=model.getManager<mMotionElectroExportManager>(Physical_MotionElectroExport);
    for(int id:exportManager.getAllIDs()) { const auto* p=exportManager.getData(id);exports.append(QJsonObject{
        {"id",id},{"name",p->getName()},{"exportType",exportNames.value(int(p->getExportType()))},{"portListFile",p->getPortListFile()},
        {"inputPorts",writeExportPorts(p->getInputPorts())},{"outputPorts",writeExportPorts(p->getOutputPorts())}}); }
    auto& hydraulicManager=model.getManager<mMotionHydraulicManager>(Physical_MotionHydraulic);
    for(int id:hydraulicManager.getAllIDs()) { const auto* p=hydraulicManager.getData(id);QJsonArray ports,pipes;
        for(const auto& r:p->getPorts())ports.append(QJsonObject{{"portName",r.portName},{"diameter",r.diameter},{"length",r.length}});
        for(const auto& r:p->getPipes())pipes.append(QJsonObject{{"pipeName",r.pipeName},{"diameter",r.diameter},{"length",r.length}});
        hydraulics.append(QJsonObject{{"id",id},{"name",p->getName()},{"hydraulicType",hydraulicNames.value(int(p->getHydraulicType()))},
            {"modelFile",p->getModelFile()},{"fluidDensity",p->getFluidDensity()},{"fluidViscosity",p->getFluidViscosity()},{"bulkModulus",p->getBulkModulus()},
            {"ports",ports},{"pipes",pipes}}); }
    auto& rlManager=model.getManager<mMotionRLManager>(Physical_MotionRL);
    for(int id:rlManager.getAllIDs()) { const auto* p=rlManager.getData(id);tasks.append(QJsonObject{
        {"id",id},{"name",p->getName()},{"algorithm",algorithmNames.value(int(p->getAlgorithm()))},{"environmentType",environmentNames.value(int(p->getEnvType()))},
        {"stateSpace",writeValues(p->getStateSpace())},{"actionSpace",writeValues(p->getActionSpace())},{"hiddenLayers",writeValues(p->getHiddenLayers())},
        {"learningRate",p->getLearningRate()},{"discountFactor",p->getDiscountFactor()},{"maxEpisodes",p->getMaxEpisodes()},{"maxSteps",p->getMaxSteps()},
        {"datasetPath",p->getDatasetPath()},{"modelSavePath",p->getModelSavePath()}}); }
    data.insert("electroImports",imports);data.insert("electroExports",exports);data.insert("hydraulics",hydraulics);data.insert("rlTasks",tasks);
}
}
