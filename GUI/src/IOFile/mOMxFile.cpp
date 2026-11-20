#include "mOMxFile.h"

//QT
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QString>

//MxFunction
#include "mGlobalSignals.h"
#include "mElementFunction.h"
#include "mVariableFunction.h"
#include "mWidgetOperation.h"
#include "mAppConfiguration.h"
#include "mModelStruct.h"

//模型修复
#include "mEleSetCtrl.h"
#include "mModelFixManager.h"

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
#include "MeshEntity.h"
#include "MXGeoFace.h"
#include "MXGeoSolid.h"
#include "MXGeoEdge.h"
#include "mMapRelationManager.h"

//MXGeoData
#include "mGeoDataBase.h"
//#include "mGeoDataModelSet.h" //【OCC摘除·头卫生】BasicData 旧导出头仍携 OCC 头；旧栈反查已切 MeshEntity GN 镜像

//材料
#include "mMaterialBase.h"
#include "mMaterialManager.h"

//集合
#include "mSetManager.h"
#include "mNode.h"
#include "mElementSet.h"
#include "mElementSurface.h"
#include "mMixSet.h"
#include "mGeoSet.h"
#include "mFrequencyPoint.h"
#include "mDOFNode.h"
#include "mUnitIoCodec.h"
#include "mPanel.h"
#include "mNodeFace.h"
#include "mSurfaceAnalysis.h"

//非结构质量
#include "mNonStructQualityManager.h"
#include "mNonStructuralQuality.h"

//离散场
#include "mFieldManager.h"
#include "mDiscreteField.h"
#include "mAnalysisField.h"

//截面属性
#include "mSolidHomo.h"
#include "mTorsionBar.h"
#include "mTube.h"
#include "mShearPlate.h"
#include "mPropertyManager.h"
#include "mShellHomo.h"
#include "mConstantBeam.h"
#include "mCrossBeam.h"
#include "mSpring.h"
#include "mViscousdampsection.h"
#include "mCbush.h"
#include "mPlyLaminate.h"
#include "mGasket.h"
#include "mGapuni.h"

//分析工况
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitLinearBuckling.h"
#include "mCaseImplicitModal.h"
#include "mCaseImplicitNonLinearArc.h"
#include "mCaseImplicitNonLinearDynamic.h"
#include "mCaseImplicitNonLinearStatic.h"
#include "mCaseImplicitSteadyHeat.h"
#include "mCaseImplicitStructuralCouplingHeat.h"
#include "mCaseImplicitTransient.h"
#include "mCaseImplicitTransientHeat.h"
#include "mCaseImplicitFrequency.h"
#include "mCaseImplASCoupling.h"
#include "mCaseImplicitResponseSpectrum.h"
#include "mCaseImplicitRandomVibration.h"
#include "mCaseImplicitGeneric.h"

//接触
#include "mContactBase.h"
#include "mContactManager.h"

//周期性边界
#include "mPeriodicBoundary.h"
#include "mPeriodicBoundaryManager.h"

//函数
#include "mFunctionBase.h"
#include "mFunctionManager.h"

//结构优化
#include "mStructOption.h"
#include "mOptimizationDesignData.h"
#include "mOptimizationConstraintData.h"
#include "mOptimizationGoalData.h"

//载荷
#include "mLoadForce.h"
#include "mLoadGravity.h"
#include "mLoadPressure.h"
#include "mLoadHeatFlux.h"
#include "mLoadRadiation.h"
#include "mLoadConvection.h"
#include "mLoadBolt.h"
#include "mLoadTransient.h"
#include "mLoadFreqdomain.h"
#include "mLoadCombined.h"
#include "mLoadSpectrum.h"
#include "mLoadAcousticConcForce.h"
#include "mLoadAcousticFrequencyDomain.h"
#include "mLoadRotationalACC.h"
#include "mLoadRotationalSpeed.h"
#include "mRoatateAnalyseBase.h"
#include "mRotateAnalyseManager.h"
#include "mRotatePhysicalPower.h"
#include "mLoadSurface.h"
#include "mLoadEdge.h"

#include "mModelFixDataBase.h"
//边界条件
#include "mBoundaryFixed.h"
#include "mBoundaryDisplacement.h"
#include "mBoundaryTemp.h"
#include "mBoundaryMove.h"
#include "mBoundaryTempNew.h"
#include "mBoundaryVirtual.h"

//预定义场
#include "mPredefinedFieldInitTemp.h"
#include "mPredefinedFieldTempLoad.h"
#include "mBoundarySuperelement.h"

//连接
#include "mConnectorBase.h"
#include "mConnectorManager.h"
#include "mConnectorRigid.h"
#include "mConnectorFlexible.h"
#include "mConnectorBind.h"
#include "mConnectorMPC.h"
#include "mConnectorDisperse.h"
#include "mConnectorBolt.h"
#include "mConnectorDistributedCoupling.h"
#include "mConnectorBearingUnits.h"
#include "mConnectorRigidbody.h"

//连接副
#include "mDeputyBase.h"
#include "mDeputyManager.h"
#include "mConnectingDeputy.h"
#include "mConnectorFastener.h"

//坐标系
#include "mCoordinateBase.h"
#include "mCoordinateManager.h"

//单元
#include "mElementBase.h"
#include "mDataElement.h"

//PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//截面形状
#include "mSectionShapeBase.h"
#include "mSectionShapeManager.h"
#include "mCustomSectionShape.h"
#include "mMacroEquivalent.h"

//采样频率点
#include "mFreqSamplPointBase.h"
#include "mFreqSamplPointManager.h"

//层合结构
#include "mLaminateBase.h"
#include "mLaminateManager.h"
#include "mSglLayerPlate.h"
#include "mGlobalSglLam.h"
#include "mLamPlate.h"

//全局参数
#include "mGlobalParam.h"
#include "mGenericParamData.h"
#include "mGlobalOutputControlData.h"
#include "mGenericAssignData.h"
#include "mGenericMatrixData.h"
#include "mASCouplingSearchData.h"
#include<QDebug>
#include "mOtherFunction.h"

#include "mIOFileManager.h"

using namespace MxFunctions;
using namespace MechPreData;
using namespace MechSectionShapeData;
using namespace MechModelFixData;
using namespace std;

namespace MechIOFile
{
    mOMxFile::mOMxFile()
    {
        _modelData = nullptr;
        _meshData = nullptr;
    }

    mOMxFile::mOMxFile(QString fname) :_outputFileName(fname)
    {
        _modelData = nullptr;
        _meshData = nullptr;
    }

    void mOMxFile::setCaseFilter(const std::vector<int>& caseIDs)
    {
        _caseFilter.clear();
        for (int id : caseIDs)
        {
            _caseFilter.insert(id);
        }
    }

    mOMxFile::~mOMxFile()
    {
    }

    bool mOMxFile::writeMxFile()
    {
        _file.setFileName(_outputFileName);
        if (!_file.open(QIODevice::Truncate | QIODevice::WriteOnly))//以写的方式打开文件
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "文件写出失败，请检查文件命名格式或文件是否被占用！");
            return false;
        }

        _out.setDevice(&_file);
        _out.setCodec("UTF-8");

        _modelData = getCVM->getModelData();
        _meshData = _modelData->getMeshData();
        _maxElementID = _meshData->getMaxEleId();

        _out << "*implicit_analysis\r\n";

        if (!writeCaseData())
        {
            _file.close();
            return false;
        }
        if (!writemGlobalParam())
        {
            _file.close();
            return false;
        }
        if (!writeStructOptionData())
        {
            _file.close();
            return false;
        }
        if (!writemCoordinate())
        {
            _file.close();
            return false;
        }
        if (!writeNodeData())
        {
            _file.close();
            return false;
        }
        if (!writeElementData())
        {
            _file.close();
            return false;
        }
        if (!writeSpecialElement())
        {
            _file.close();
            return false;
        }
        if (!writeConnectorData())
        {
            _file.close();
            return false;
        }
        if (!writeBearingSection())
        {
            _file.close();
            return false;
        }
        if (!writeConnectorDeputy())
        {
            _file.close();
            return false;
        }
        if (!writeASCouplingSearchData())
        {
            _file.close();
            return false;
        }
        if (!writemGenericAssign())
        {
            _file.close();
            return false;
        }
        if (!writeMaterialData())
        {
            _file.close();
            return false;
        }
        if (!WritePropertyData())
        {
            _file.close();
            return false;
        }
        if (!writemGenericParam())
        {
            _file.close();
            return false;
        }
        if (!writeContactData())
        {
            _file.close();
            return false;
        }
        if (!writePeriodicBoundaryData())
        {
            _file.close();
            return false;
        }
        if (!writeLoadData())
        {
            _file.close();
            return false;
        }

        writeRotateAnalyse();

        if (!writeBoundaryData())
        {
            _file.close();
            return false;
        }
        if (!writeSetData())
        {
            _file.close();
            return false;
        }
        if (!writeTPContriData())
        {
            _file.close();
            return false;
        }
        if (!writeNonSturcturalQualityData())
        {
            _file.close();
            return false;
        }

        if (!writeDiscreteField())
        {
            _file.close();
            return false;
        }

        if (!writeAnalysisField())
        {
            _file.close();
            return false;
        }

        if (!writeFreqSamplPointData())
        {
            _file.close();
            return false;
        }

        if (!writePeakFilterData())
        {
            _file.close();
            return false;
        }

        if (!writeLaminateData())
        {
            _file.close();
            return false;
        }

        if (!writeFunction())
        {
            _file.close();
            return false;
        }

        if (!writeModelChangeData())
        {
            _file.close();
            return false;
        }

        if (!writePredefinedFieldData())
        {
            _file.close();
            return false;
        }

        if (!writeGlobalOutputControl())
        {
            _file.close();
            return false;
        }

        _out << "*end*" << "\r\n";
        _file.close();
        //成功写文件
        return true;
    }

    bool mOMxFile::writeModelChangeData()
    {
        using namespace MechModelFixData;

        vector<int> ids = _modelData->getAllPhyID(Physical_ModelFix);
        if (ids.empty())
        {
            return true;
        }

        mModelFixManager& manager = _modelData->getManager<mModelFixManager>(Physical_ModelFix);
        for (int id : ids)
        {
            std::shared_ptr<mModelFixDataBase> base = manager.getModelFixData(id);
            std::shared_ptr<mEleSetCtrl> eleSetCtrl = std::dynamic_pointer_cast<mEleSetCtrl>(base);
            if (!eleSetCtrl)
                continue;

            int setID = _modelData->getMapRelationManager().getMapRelation<int>(
                { Physical_ModelFix, id }, Physical_Set);

            QString name = eleSetCtrl->getModelFixName();
            QString type = "element";
            QString method = eleSetCtrl->isActiveOrRemove() ? "active" : "remove";
            QString setName = writeGeoSet(setID, Element);

            _out << QString("*modelchange, type = %1, name = \"%2\", method = %3\r\n")
                .arg(type, name, method);
            _out << QString("\"%1\"").arg(setName);
            _out << QString("\r\n");
        }

        return true;
    }

    bool mOMxFile::writePredefinedFieldData()
    {
        vector<int> ids = _modelData->getAllPhyID(Physical_PredefinedField);
        if (ids.size() == 0)
        {
            return true;
        }

        for (int id : ids)
        {
            if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
            {
                _file.close();
                return false;
            }

            std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldBase = _modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).getPredefinedFieldData(id);
            if (!fieldBase)
                continue;

            if (fieldBase->getPredefinedFieldType() == PREDEFINED_FIELD_INIT_TEMP)
            {
                writeInitTempData(fieldBase, id);
            }
            else if (fieldBase->getPredefinedFieldType() == PREDEFINED_FIELD_TEMP_LOAD)
            {
                writeTempLoadData(fieldBase, id);
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("预定义场写文件完成");
        return true;
    }

    void mOMxFile::writeInitTempData(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldBase, int id)
    {
        std::shared_ptr<MechCaseData::mPredefinedFieldInitTemp> initTemp = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldInitTemp>(fieldBase);
        if (!initTemp)
            return;

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_PredefinedField,initTemp->getPredefinedFieldId() }, Physical_Set);
        QString setName = "";
        if (initTemp->_defineType == 0)
        {
            setName = writeGeoSet(setID, Node);
        }

        QString fieldName = initTemp->getPredefinedFieldName();

        if (initTemp->_defineType == 0)
        {
            // ========== 统一赋值格式 ==========
            _out << QString("*ibc, type = temp, set = \"%1\", name = \"%2\"\r\n").arg(setName).arg(fieldName);
            //温度仿射换算：内部值 → 当前界面单位制
            bool tempOk = false; double tempVal = initTemp->_temp.toDouble(&tempOk);
            if (tempOk)
            {
                _out << mUnitIoCodec::encodeTemp(tempVal) << "\r\n";
            }
            else
            {
                _out << initTemp->_temp << "\r\n";
            }
        }
        else
        {
            // ========== 离散（独立）赋值格式 ==========
            _out << QString("*ibc, type = temp, name = \"%1\"\r\n").arg(fieldName);

            for (const auto& entry : initTemp->_nodeTempMap)
            {
                int nodeId = entry.first.result_id;
                QVariant tempVal;
                if (!entry.second.isEmpty())
                {
                    tempVal = entry.second.first();
                }

                _out << QString("%1, ").arg(nodeId);
                //温度仿射换算：内部值 → 当前界面单位制（非数值原样写出）
                bool tempOk = false; double tempNum = tempVal.toDouble(&tempOk);
                if (tempOk && !tempVal.toString().simplified().isEmpty())
                {
                    _out << mUnitIoCodec::encodeTemp(tempNum);
                }
                else
                {
                    writeQVariantValue(tempVal);
                }
                _out << "\r\n";
            }
        }
    }

    void mOMxFile::writeTempLoadData(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldBase, int id)
    {
        std::shared_ptr<MechCaseData::mPredefinedFieldTempLoad> tempLoad =
            std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldTempLoad>(fieldBase);
        if (!tempLoad)
            return;

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_PredefinedField,tempLoad->getPredefinedFieldId() }, Physical_Set);
        QString setName = "";
        if (tempLoad->_defineType == 0)
        {
            setName = writeGeoSet(setID, Node);
        }

        QString fieldName = tempLoad->getPredefinedFieldName();

        switch (tempLoad->_defineType)
        {
        case 0: // ========== 模式0：统一赋值 ==========
        {
            setName = _modelData->getNameByPhyID(Physical_Set, setID);
            _out << QString("*temp, set = \"%1\", name = \"%2\"").arg(setName).arg(fieldName);
            writeMapIntData(Physical_PredefinedField, id, Physical_Function);
            _out << "\r\n";
            //温度仿射换算：内部值 → 当前界面单位制
            bool tempOk = false; double tempVal = tempLoad->_temp.toDouble(&tempOk);
            if (tempOk)
            {
                _out << mUnitIoCodec::encodeTemp(tempVal);
            }
            else
            {
                _out << tempLoad->_temp;
            }
            _out << "\r\n";
            break;
        }
        case 1: // ========== 模式1：独立（离散）赋值 ==========
        {
            _out << QString("*temp, name = \"%1\"").arg(fieldName);
            writeMapIntData(Physical_PredefinedField, id, Physical_Function);
            _out << "\r\n";
            for (const auto& entry : tempLoad->_nodeTempMap)
            {
                int nodeId = entry.first.result_id;
                QVariant tempVal;
                if (!entry.second.isEmpty())
                {
                    tempVal = entry.second.first();
                }

                // 修改：移除空值过滤，固定输出节点ID+逗号站位，空值正常保留
                _out << QString("%1, ").arg(nodeId);
                //温度仿射换算：内部值 → 当前界面单位制（非数值原样写出）
                bool tempOk = false; double tempNum = tempVal.toDouble(&tempOk);
                if (tempOk && !tempVal.toString().simplified().isEmpty())
                {
                    _out << mUnitIoCodec::encodeTemp(tempNum);
                }
                else
                {
                    writeQVariantValue(tempVal);
                }
                _out << "\r\n";
            }
            break;
        }
        case 2: // ========== 模式2：文件定义 ==========
        {
            _out << QString("*temp, name = \"%1\"").arg(fieldName);
            writeMapIntData(Physical_PredefinedField, id, Physical_Function);
            _out << QString(", file=\"%1\"").arg(tempLoad->_filePath);

            if (tempLoad->_interpolateType == 0)
            {
                _out << QString(", type=single-step");
            }
            else
            {
                _out << QString(", type=multi-step");
            }

            if (tempLoad->_midNodeInterpolate)
            {
                _out << QString(", midside=on");
            }

            _out << "\r\n";
            break;
        }
        }
    }

    bool mOMxFile::writeNodeData()
    {
        mGlobalSignals::getInstance()->outputMessageSig(0, QString("节点写文件开始"), true, true);
        _out << "*node" << "\r\n";

        map<int, int> nodeId;
        for (auto it = _meshData->_nodedata.nodeidmap.begin(); it != _meshData->_nodedata.nodeidmap.end(); ++it)
        {
            nodeId[it.key()] = it.value();
        }
        writeNodeData(nodeId);


        mGlobalSignals::getInstance()->outputMessageSig(0, QString("节点写文件完成"), true, true);
        return true;

    }

    void mOMxFile::writeNodeData(std::map<int, int> NodeId)
    {
        double* xyz = _meshData->_nodedata.xyz;
        std::map<int, std::pair<int, int>> nodeCoordinateData = _modelData->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getNodeCoordinateData();
        for (auto it = NodeId.begin(); it != NodeId.end(); ++it)
        {
            int nodeKey = it->first;
            int nodeID = it->second;
            double x = xyz[3 * nodeID];
            double y = xyz[3 * nodeID + 1];
            double z = xyz[3 * nodeID + 2];

            //参考坐标系-分析坐标系
            QString refCoord;
            QString caseCoord;
            //柱坐标标记：柱坐标下y列为角度(度，文件格式固定语义)，长度换算时须跳过
            bool cylindricalRef = false;
            if (nodeCoordinateData.find(nodeKey) != nodeCoordinateData.cend())
            {
                if (nodeCoordinateData[nodeKey].first > 0)
                {
                    refCoord = _modelData->getNameByPhyID(Physical_Coordinate, nodeCoordinateData[nodeKey].first);
                }
                if (nodeCoordinateData[nodeKey].second > 0)
                {
                    caseCoord = _modelData->getNameByPhyID(Physical_Coordinate, nodeCoordinateData[nodeKey].second);
                }

                if (!refCoord.isEmpty())
                {
                    MechCoordinateData::mCoordinateBase* coorBase = _modelData->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(refCoord);
                    // 缓存轴数据
                    bool success; QString msg; QVector3D origin;
                    const QVector<QVector3D> xyzAxis = coorBase->getXYZAxis(success, msg, origin);

                    QVector3D zAxisPoint(origin.x() + xyzAxis[2].x(), origin.y() + xyzAxis[2].y(), origin.z() + xyzAxis[2].z());
                    QVector3D refPoint(origin.x() + xyzAxis[0].x(), origin.y() + xyzAxis[0].y(), origin.z() + xyzAxis[0].z());

                    if (coorBase->getCoordinateType() == Cylindrical)
                    {
                        cylindricalRef = true;
                        double x1 = x;
                        double y1 = y;
                        double z1 = z;

                        QVector3D e_z = (zAxisPoint - origin).normalized();
                        QVector3D v_ref = refPoint - origin;
                        QVector3D e_x = (v_ref - e_z * QVector3D::dotProduct(v_ref, e_z)).normalized();
                        QVector3D e_y = QVector3D::crossProduct(e_z, e_x);

                        // 将全局坐标转换为柱坐标 (r, theta, h)
                        QVector3D P(x1 - origin.x(), y1 - origin.y(), z1 - origin.z());
                        double h = QVector3D::dotProduct(P, e_z);
                        QVector3D P_perp = P - e_z * h;
                        double r = P_perp.length();

                        double theta = 0.0;
                        if (r > 1e-12)
                        {
                            theta = std::atan2(QVector3D::dotProduct(P_perp, e_y), QVector3D::dotProduct(P_perp, e_x));
                        }
                        theta = theta * 180.0 / 3.14159265358979323846;

                        x = r;
                        y = theta;
                        z = h;
                    }
                    else
                    {
                        QVector3D Edge11 = xyzAxis[2];
                        QVector3D Edge12 = xyzAxis[0];
                        QVector3D Edge13;
                        QVector3D Edge14;

                        Edge13 = QVector3D::crossProduct(QVector3D(Edge11[0], Edge11[1], Edge11[2]), QVector3D(Edge12[0], Edge12[1], Edge12[2]));
                        Edge14 = QVector3D::crossProduct(QVector3D(Edge13[0], Edge13[1], Edge13[2]), QVector3D(Edge11[0], Edge11[1], Edge11[2]));

                        Edge11.normalize();
                        Edge12.normalize();
                        Edge13.normalize();
                        Edge14.normalize();

                        double x1 = x;
                        double y1 = y;
                        double z1 = z;
                        x1 = Edge14[0] * (x - origin.x()) + Edge14[1] * (y - origin.y()) + Edge14[2] * (z - origin.z());
                        y1 = Edge13[0] * (x - origin.x()) + Edge13[1] * (y - origin.y()) + Edge13[2] * (z - origin.z());
                        z1 = Edge11[0] * (x - origin.x()) + Edge11[1] * (y - origin.y()) + Edge11[2] * (z - origin.z());

                        x = x1;
                        y = y1;
                        z = z1;
                    }
                }
            }

            //单位制换算：内部mm-t-N-s → 当前界面单位制（长度）；
            //柱坐标y列(θ,度)为文件格式固定语义不换算
            x = mUnitIoCodec::encode(MxFunctions::Unit::Length, x);
            if (!cylindricalRef)
            {
                y = mUnitIoCodec::encode(MxFunctions::Unit::Length, y);
            }
            z = mUnitIoCodec::encode(MxFunctions::Unit::Length, z);

            _out << QString("%1").arg(nodeKey, 8) << ", " << QString("%1").arg(x, 15, 'f', 8) << ", " << QString("%1").arg(y, 15, 'f', 8) << ", " << QString("%1").arg(z, 15, 'f', 8);
            if (!refCoord.isEmpty())
            {
                _out << QString(", \"%1\"").arg(refCoord);
            }
            else if (!caseCoord.isEmpty())
            {
                _out << QString(", ");
            }

            if (!caseCoord.isEmpty())
            {
                _out << QString(", \"%1\"").arg(caseCoord);
            }
            _out << "\r\n";
        }
    }

    bool mOMxFile::writeElementData()
    {
        mGlobalSignals::getInstance()->outputMessageSig(0, "单元写文件开始...", true, true);
        if (_partNames.isEmpty()) _partNames = _meshData->getAllPartNames();
        if (_partNames.size() == 0)
        {
            return true;
        }

        //获取单元数据
        MechElementData::mDataElement* elements = _modelData->getElementData();
        MechMapRelation::mMapRelationManager& mapManager = _modelData->getMapRelationManager();

        for (QString partName : _partNames)
        {
            if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
            {
                _file.close();
                return false;
            }
            int partID = _meshData->getPartIdByPartName(partName);
            int propertyID = mapManager.getMapRelation<int>({ Physical_Part,partID }, Physical_Property);
            QString propertyName = _modelData->getNameByPhyID(Physical_Property, propertyID);
            QHash<ElementType, vector<int>> group;//单元类型-----单元ID列表
            //三维
            vector<int> meshIDs = _meshData->getMeshID(partName);
            for (int meshID : meshIDs)
            {
                MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(meshID);
                if (elementBase == nullptr || elementBase->elementType == MxFunctions::NoneElement)
                {
                    continue;
                }
                group[elementBase->elementType].push_back(meshID);
            }
            QHashIterator<ElementType, vector<int>> iter3(group);
            while (iter3.hasNext())
            {
                iter3.next();
                _out << QString("*element, type = %1, part = \"%2\"").arg(mElementFunction::getElementString(iter3.key()).toLower()).arg(partName);
                if (propertyName.isEmpty())
                {
                    _out << QString("\r\n");
                }
                else
                {
                    _out << QString(", section = \"%1\"\r\n").arg(propertyName);
                }

                vector<int> eleIDs = iter3.value();
                for (int eleID : eleIDs)
                {
                    vector<int> indexs = _meshData->getNodeIndexByID(partID, eleID);
                    _out << QString("%1, ").arg(eleID, 10);
                    for (int i = 0; i < indexs.size(); ++i)
                    {
                        if (i == indexs.size() - 1)
                        {
                            _out << QString("%1\r\n").arg(indexs.at(i), 10);
                        }

                        else
                        {
                            _out << QString("%1, ").arg(indexs.at(i), 10);
                        }
                    }
                }
            }

            //二维
            group.clear();
            vector<int>geoFaceIDs = _meshData->getSurfaceMeshID(partName);
            for (int meshID : geoFaceIDs)
            {
                MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(meshID);
                if (elementBase == nullptr || elementBase->elementType == MxFunctions::NoneElement)
                {
                    continue;
                }
                group[elementBase->elementType].push_back(meshID);
            }
            QHashIterator<ElementType, vector<int>> iter2(group);
            while (iter2.hasNext())
            {
                iter2.next();
                {
                    _out << QString("*element, type = %1, part = \"%2\"").arg(mElementFunction::getElementString(iter2.key()).toLower()).arg(partName);
                    if (propertyName.isEmpty())
                    {
                        _out << QString("\r\n");
                    }
                    else
                    {
                        _out << QString(", section = \"%1\"\r\n").arg(propertyName);
                    }

                    vector<int> eleIDs = iter2.value();
                    switch (iter2.key())
                    {
                    case CPDSG:
                    case T3:
                    case TRIA3:
                    case CTRIA3:
                    case CQUAD4:
                    case Q4:
                    case MITC4:
                    {
                        QString temp_para;
                        for (int eleID : eleIDs)
                        {
                            vector<int> indexs = _meshData->getNodeIndexByID(partID, eleID);
                            _out << QString("%1, ").arg(eleID, 10);
                            for (int i = 0; i < indexs.size(); ++i)
                            {
                                if (i == indexs.size() - 1)
                                {
                                    _out << QString("%1").arg(indexs.at(i), 10);
                                }

                                else
                                {
                                    _out << QString("%1, ").arg(indexs.at(i), 10);
                                }
                            }

                            MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(eleID);
                            MechElementData::mShellElement* element1 = dynamic_cast<MechElementData::mShellElement*>(elementBase);
                            if (element1)
                            {
                                QStringList paraList;
                                if (element1->_paraType == "材料坐标系")
                                {
                                    QString coorName = _modelData->getNameByPhyID(Physical_Coordinate, element1->_coordID);
                                    if (!coorName.isEmpty()) paraList.append(QString("\"%1\"").arg(coorName));
                                }
                                else if (element1->_paraType == "材料转角值")
                                {
                                    paraList.append(element1->_angle);
                                }
                                else
                                {
                                    paraList.append("");
                                }
                                paraList.append(element1->_bias);
                                temp_para = mOtherFunction::getParameter(paraList);
                                if (!temp_para.isEmpty()) _out << QString(", %1").arg(temp_para);
                            }
                            _out << "\r\n";
                        }
                        break;
                    }
                    default:
                        for (int eleID : eleIDs)
                        {
                            vector<int> indexs = _meshData->getNodeIndexByID(partID, eleID);
                            _out << QString("%1, ").arg(eleID, 10);
                            for (int i = 0; i < indexs.size(); ++i)
                            {
                                if (i == indexs.size() - 1)
                                {
                                    _out << QString("%1\r\n").arg(indexs.at(i), 10);
                                }

                                else
                                {
                                    _out << QString("%1, ").arg(indexs.at(i), 10);
                                }
                            }
                        }
                        break;
                    }
                }
            }

            //一维
            group.clear();
            vector<int> geoLineIDs = _meshData->get1DLineID(partName);
            for (int geoLineID : geoLineIDs)
            {
                MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(geoLineID);
                if (elementBase == nullptr || elementBase->elementType == MxFunctions::NoneElement)
                {
                    continue;
                }
                group[elementBase->elementType].push_back(geoLineID);
            }
            QHashIterator<ElementType, vector<int>> iter1(group);
            while (iter1.hasNext())
            {
                iter1.next();
                _out << QString("*element, type = %1, part = \"%2\"").arg(mElementFunction::getElementString(iter1.key()).toLower()).arg(partName);
                if (propertyName.isEmpty())
                {
                    _out << QString("\r\n");
                }
                else
                {
                    _out << QString(", section = \"%1\"\r\n").arg(propertyName);
                }

                vector<int> eleIDs = iter1.value();
                switch (iter1.key())
                {
                case MxFunctions::D3B2:
                {
                    for (int eleID : eleIDs)
                    {
                        vector<int> indexs = _meshData->getNodeIndexByID(partID, eleID);
                        _out << QString("%1, ").arg(eleID, 10);
                        for (int i = 0; i < indexs.size(); ++i)
                        {
                            if (i == indexs.size() - 1)
                            {
                                _out << QString("%1").arg(indexs.at(i), 10);
                                MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(eleID);
                                MechElementData::mBeamElement* element1 = dynamic_cast<MechElementData::mBeamElement*>(elementBase);
                                if (element1 == nullptr)
                                {
                                    _out << QString("\r\n");
                                    continue;
                                }

                                if (!element1->_node.isEmpty())
                                {
                                    _out << QString(", %1").arg(element1->_node, 10);
                                }
                                else if (!element1->_beamValue.isEmpty())
                                {
                                    for (auto para : element1->_beamValue.split(","))
                                    {
                                        _out << QString(", %1").arg(para.toDouble(), 15, 'f', 8);
                                    }
                                }
                                else
                                {
                                    _out << QString("\r\n");
                                    continue;
                                }

                                if (!element1->_biasType.isEmpty())
                                {
                                    _out << QString(", %1, %2, %3, %4, %5, %6, %7").arg(element1->_biasType).arg(element1->_X1)
                                        .arg(element1->_Y1).arg(element1->_Z1).arg(element1->_X2).arg(element1->_Y2).arg(element1->_Z2);
                                }

                                if (element1->_pinsa != 0 || element1->_pinsb != 0)
                                {
                                    if (element1->_pinsa != 0)
                                    {
                                        _out << QString(", %1").arg(element1->_pinsa);
                                    }
                                    else
                                    {
                                        _out << QString(", 0");
                                    }

                                    if (element1->_pinsb != 0)
                                    {
                                        _out << QString(", %1").arg(element1->_pinsb);
                                    }
                                    else
                                    {
                                        _out << QString(", 0");
                                    }
                                }

                                _out << QString("\r\n");
                            }
                            else
                            {
                                _out << QString("%1, ").arg(indexs.at(i), 10);
                            }
                        }
                    }
                    break;
                }
                case ELAS1:
                case VISCOUSDAMP:
                case DASHPOT2:
                {
                    for (int eleID : eleIDs)
                    {
                        vector<int> indexs = _meshData->getNodeIndexByID(partID, eleID);
                        _out << QString("%1, ").arg(eleID, 10);
                        for (int i = 0; i < indexs.size(); ++i)
                        {
                            if (i == indexs.size() - 1)
                            {
                                _out << QString("%1").arg(indexs.at(i), 10);
                                MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(eleID);
                                MechElementData::mElas1Element* element1 = dynamic_cast<MechElementData::mElas1Element*>(elementBase);
                                if (element1 == nullptr)
                                {
                                    _out << QString("\r\n");
                                    continue;
                                }

                                _out << QString(", %1").arg(element1->_dof, 10);
                                _out << QString("\r\n");
                            }
                            else
                            {
                                _out << QString("%1, ").arg(indexs.at(i), 10);
                            }
                        }
                    }
                    break;
                }
                case CBUSH:
                case CBUSH1:
                {
                    for (int eleID : eleIDs)
                    {
                        vector<int> indexs = _meshData->getNodeIndexByID(partID, eleID);
                        _out << QString("%1, ").arg(eleID, 10);
                        for (int i = 0; i < indexs.size(); ++i)
                        {
                            if (i == indexs.size() - 1)
                            {
                                _out << QString("%1").arg(indexs.at(i), 10);
                                MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(eleID);
                                MechElementData::mCbushElement* element1 = dynamic_cast<MechElementData::mCbushElement*>(elementBase);
                                if (element1 == nullptr)
                                {
                                    _out << QString("\r\n");
                                    continue;
                                }

                                QString coorName = _modelData->getNameByPhyID(Physical_Coordinate, element1->_cid);
                                if (!coorName.isEmpty()) _out << QString(", \"%1\"").arg(coorName);
                                else _out << QString(", ");

                                _out << QString(", %1").arg(element1->_s);

                                coorName = _modelData->getNameByPhyID(Physical_Coordinate, element1->_ocid);
                                if (!coorName.isEmpty()) _out << QString(", \"%1\"").arg(coorName);
                                else _out << QString(", ");

                                _out << QString(", ");

                                QStringList paraList;
                                paraList.push_back(element1->_x1);
                                paraList.push_back(element1->_x2);
                                paraList.push_back(element1->_x3);
                                QString temp_para = mOtherFunction::getParameter(paraList);
                                _out << temp_para;
                                _out << QString("\r\n");
                            }
                            else
                            {
                                _out << QString("%1, ").arg(indexs.at(i), 10);
                            }
                        }
                    }
                    break;
                }
                default:
                    for (int eleID : eleIDs)
                    {
                        vector<int> indexs = _meshData->getNodeIndexByID(partID, eleID);
                        _out << QString("%1, ").arg(eleID, 10);
                        for (int i = 0; i < indexs.size(); ++i)
                        {
                            if (i == indexs.size() - 1)
                            {
                                _out << QString("%1\r\n").arg(indexs.at(i), 10);

                            }
                            else
                            {
                                _out << QString("%1, ").arg(indexs.at(i), 10);
                            }
                        }
                    }
                    break;
                }
            }

            //0维
            group.clear();
            meshIDs = _meshData->getMassIDs(partName);
            for (int meshID : meshIDs)
            {
                MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(meshID);
                if (elementBase == nullptr || elementBase->elementType == NoneElement)
                {
                    continue;
                }
                group[elementBase->elementType].push_back(meshID);
            }
            QHashIterator<ElementType, vector<int>> iter0(group);
            while (iter0.hasNext())
            {
                iter0.next();
                _out << QString("*element, type = %1, part = \"%2\"").arg(mElementFunction::getElementString(iter0.key()).toLower()).arg(partName);
                if (propertyName.isEmpty())
                {
                    _out << QString("\r\n");
                }
                else
                {
                    _out << QString(", section = \"%1\"\r\n").arg(propertyName);
                }
                vector<int> eleIDs = iter0.value();
                for (int eleID : eleIDs)
                {
                    MechElementData::mElementBase* elementBase = elements->getElementDataByMeshID(eleID);
                    vector<int> indexs = _meshData->getNodeIndexByID(partID, eleID);
                    _out << QString("%1, %2, %3\r\n").arg(eleID, 10).arg(indexs[0], 10).arg(elementBase->getParameter());
                }
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("单元写文件完成");
        return true;
    }

    bool mOMxFile::writeSpecialElement()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Deputy);
        vector<int> ids1 = _modelData->getAllPhyID(Physical_Connector);
        if (ids.size() == 0 && ids1.size() == 0)
        {
            return true;
        }

        MechConnectorData::mDeputyManager& manager = _modelData->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);
        for (int id : ids)
        {
            shared_ptr<MechConnectorData::mDeputyBase> deputyBase = manager.getDeputyData(id);
            if (!deputyBase || deputyBase->isSuppressed())
                continue;

            if (deputyBase->getDeputyType() == Deputy_Connector)
            {
                std::shared_ptr<MechConnectorData::mConnectingDeputy> deputyData = dynamic_pointer_cast<MechConnectorData::mConnectingDeputy>(deputyBase);
                if (!deputyData) continue;

                int size = deputyData->refNodeIds_.size();
                if (size == 0) continue;

                _out << QString("*element, type = CONN3D2, part = \"%1\", section = \"%2\"\r\n").arg(deputyBase->getDeputyName()).arg(deputyBase->getDeputyName());
                for (int i = 0; i < size; i++)
                {
                    _out << QString("%1, %2, %3\r\n").arg(++_maxElementID, 10).arg(deputyData->refNodeIds_[i], 10).arg(deputyData->movNodeIds_[i], 10);
                }
            }
            else if (deputyBase->getDeputyType() == Deputy_Fastener)
            {
                std::shared_ptr<MechConnectorData::mConnectorFastener> deputyData = dynamic_pointer_cast<MechConnectorData::mConnectorFastener>(deputyBase);
                if (!deputyData && deputyData->defineType_ != 0) continue;

                if (!_meshData->NodeIsHaveByID(deputyData->nodeId_)) continue;

                _out << QString("*element, type = CONN3D2, part = \"%1\", section = \"%2\"\r\n").arg(deputyBase->getDeputyName()).arg(deputyBase->getDeputyName());
                _out << QString("%1, %2, %3\r\n").arg(++_maxElementID, 10).arg(deputyData->nodeId_, 10).arg(deputyData->tempNodeId_, 10);
            }

        }
        for (auto& id : ids1)
        {
            shared_ptr<MechConnectorData::mConnectorBase> connector = _modelData->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(id);
            if (connector->getConnectorType() == Connector_BearingUnits)
            {
                writeConnectorBearingUnits(connector);
            }
        }
        return true;
    }

    // 抽取的单个工况写入函数
    void mOMxFile::writeSingleCase(MechCaseData::mCaseBase* caseBase)
    {
        switch (caseBase->getCaseType())
        {
        case ImplicitLinearStatic:
            writeImplicitLinearStatic(caseBase);
            break;
        case ImplicitNonLinearStatic:
            writeImplicitNonLinearStatic(caseBase);
            break;
        case ImplicitNonLinearDynamic:
            writeImplicitNonLinearDynamic(caseBase);
            break;
        case ImplicitPostBuckling:
            writeImplicitPostBuckling(caseBase);
            break;
        case ImplicitSteadyHeat:
            writeImplicitSteadyHeat(caseBase);
            break;
        case ImplicitTransientHeat:
            writeImplicitTransientHeat(caseBase);
            break;
        case ImplicitStructuralCouplingHeat:
            writeImplicitStructuralCouplingHeat(caseBase);
            break;
        case ImplicitModal:
            writeImplicitModal(caseBase);
            break;
        case ImplicitLinearBuckling:
            writeImplicitLinearBuckling(caseBase);
            break;
        case ImplicitFrequency:
            writeImplicitFrequency(caseBase);
            break;
        case ASCoupling:
            writeImplASCoupling(caseBase);
            break;
        case ImplicitResponseSpectrum:
            writeImplicitResponseSpectrum(caseBase);
            break;
        case ImplicitRandomVibration:
            writeImplicitRandomVibration(caseBase);
            break;
        case ImplicitTransient:
            writeImplicitTransient(caseBase);
            break;
        case ImplicitGeneric:
            writeImplicitGeneric(caseBase);
            break;
        default:
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("分析工况 %1 不支持写出").arg(caseBase->getCaseName()));
            break;
        }
    }

    bool mOMxFile::writeCaseData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Case);
        if (ids.size() == 0)
        {
            return true;
        }

        // 优先写入的工况
        vector<MechCaseData::mCaseBase*> priorityCases;
        // 普通工况
        vector<MechCaseData::mCaseBase*> normalCases;

        for (auto& id : ids)
        {
            //工况过滤：设置了过滤列表时只写列表内的工况（任务只算所选工况）
            if (!_caseFilter.empty() && _caseFilter.find(id) == _caseFilter.end())
                continue;

            MechCaseData::mCaseBase* caseBase = _modelData->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(id);
            if (!caseBase)
                continue;

            CaseType type = caseBase->getCaseType();

            // 判断是否为优先类型
            if (type == ImplicitLinearStatic ||
                type == ImplicitSteadyHeat ||
                type == ImplicitTransientHeat ||
                type == ImplicitStructuralCouplingHeat)
                priorityCases.push_back(caseBase);
            else
                normalCases.push_back(caseBase);
        }

        // 先写优先类型
        for (auto* caseBase : priorityCases)
        {
            if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
            {
                _file.close();
                return false;
            }
            writeSingleCase(caseBase);
        }

        // 再写普通类型
        for (auto* caseBase : normalCases)
        {
            if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
            {
                _file.close();
                return false;
            }
            writeSingleCase(caseBase);
        }

        mGlobalSignals::getInstance()->outputMessageSig("分析工况写文件完成");
        return true;
    }

    void mOMxFile::writeCasePredefinedField(MechCaseData::mCaseBase* caseBase)
    {
        set<int> pfIDs = _modelData->getMapRelationManager().getMapRelation<set<int>>({ Physical_Case, caseBase->getCaseId() }, Physical_PredefinedField);
        vector<int> ic, temp;
        _modelData->getManager<MechCaseData::mPredefinedFieldManager>(Physical_PredefinedField).differentPredefinedFieldFlag(pfIDs, ic, temp);

        if (ic.size() > 0)
        {
            _out << "*ic\r\n";
            for (auto it : ic)
            {
                _out << QString("\"%1\"\r\n").arg(_modelData->getNameByPhyID(Physical_PredefinedField, it));
            }
        }

        if (temp.size() > 0)
        {
            _out << "*temp_load, type = load\r\n";
            for (auto it : temp)
            {
                _out << QString("\"%1\"\r\n").arg(_modelData->getNameByPhyID(Physical_PredefinedField, it));
            }
        }
    }

    void mOMxFile::writeCaseLoadBCData(MechCaseData::mCaseBase* caseBase)
    {
        set<int> loadIDs = _modelData->getMapRelationManager().getMapRelation<set<int>>({ Physical_Case, caseBase->getCaseId() }, Physical_LoadBoundary);
        vector<int> mechanicalLoad, dynamicLoad;
        _modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).differentiateLoadFlag(loadIDs, mechanicalLoad, dynamicLoad, true);
        mechanicalLoad.reserve(mechanicalLoad.size() + dynamicLoad.size());
        CaseType caseType = caseBase->getCaseType();
        if (caseType == ImplicitLinearStatic || caseType == ImplicitNonLinearStatic || caseType == ImplicitNonLinearDynamic || caseType == ImplicitPostBuckling)
        {
            int temp_caseID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, caseBase->getCaseId() }, Physical_Case);
            if (temp_caseID != -1)
            {
                _out << "*temp_load, type = subcase\r\n";
                _out << QString("\"%1\"\r\n").arg(_modelData->getNameByPhyID(Physical_Case, temp_caseID));
            }
        }

        if (mechanicalLoad.size() > 0)
        {
            _out << "*load\r\n";
            for (int i = 0; i < mechanicalLoad.size(); i++)
            {
                int loadID = mechanicalLoad[i];
                auto loadBase = _modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadID);
                if (!loadBase)
                    continue;
                if (loadBase->getLoadBoundaryType() == LOAD_IM_BOLT)
                {
                    int connectID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary, loadID }, Physical_Connector);
                    auto connectorBase = _modelData->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(connectID);
                    if (connectorBase && connectorBase->isSuppressed())
                        continue;
                }
                _out << QString("\"%1\", %2\r\n").arg(_modelData->getNameByPhyID(Physical_LoadBoundary, loadID))
                    .arg(_modelData->getMapRelationManager().getScaleByCase({ Physical_LoadBoundary, loadID }, caseBase->getCaseId()));
            }
        }

        if (dynamicLoad.size() > 0)
        {
            _out << "*dload\r\n";
            for (int i = 0; i < dynamicLoad.size(); i++)
            {
                int loadID = dynamicLoad[i];
                _out << QString("\"%1\"\r\n").arg(_modelData->getNameByPhyID(Physical_LoadBoundary, loadID));
            }
        }

        set<int> bcIDs = _modelData->getMapRelationManager().getMapRelation<set<int>>({ Physical_Case, caseBase->getCaseId() }, Physical_LoadBoundary);
        vector<int> spcBC, suport1BC;
        _modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).differentiateBoundaryFlag(bcIDs, spcBC, suport1BC, true);
        if (spcBC.size() > 0)
        {
            _out << "*spc\r\n";
            if (caseBase->getCaseType() == ImplicitModal || caseBase->getCaseType() == ImplicitResponseSpectrum || caseBase->getCaseType() == ImplicitRandomVibration ||
                caseBase->getCaseType() == ImplicitFrequency || caseBase->getCaseType() == ImplicitTransient || caseBase->getCaseType() == ImplicitNonLinearDynamic
                || caseBase->getCaseType() == ASCoupling || caseBase->getCaseType() == ImplicitGeneric) //动态分析+超单元分不需要写缩放因子
            {
                for (auto it : spcBC)
                {
                    _out << QString("\"%1\"\r\n").arg(_modelData->getNameByPhyID(Physical_LoadBoundary, it));
                }
            }
            else
            {
                for (auto it : spcBC)
                {
                    _out << QString("\"%1\", %2\r\n").arg(_modelData->getNameByPhyID(Physical_LoadBoundary, it))
                        .arg(_modelData->getMapRelationManager().getScaleByCase({ Physical_LoadBoundary, it }, caseBase->getCaseId()));
                }
            }
        }
        if (suport1BC.size() > 0)
        {
            _out << "*suport1\r\n";
            for (auto it : suport1BC)
            {
                _out << QString("\"%1\"\r\n").arg(_modelData->getNameByPhyID(Physical_LoadBoundary, it));
            }
        }
    }
    void mOMxFile::writeCaseContactinterference(MechCaseData::mCaseBase* caseBase)
    {
        set<int> contactIDs = _modelData->getMapRelationManager().getMapRelation<set<int>>({ Physical_Case, caseBase->getCaseId() }, Physical_Contact);
        // 过滤掉被抑制的过盈接触
        QVector<int> validIDs;
        for (auto it : contactIDs)
        {
            MechContactData::mContactBase* contact = _modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(it);
            if (!contact || contact->isSuppressed())
                continue;
            validIDs.append(it);
        }
        if (validIDs.isEmpty()) return;
        _out << "*contactinterference\r\n";
        for (auto it : validIDs)
        {
            QString contactName = _modelData->getNameByPhyID(Physical_Contact, it);
            if (!contactName.isEmpty()) _out << QString("\"%1\"\r\n").arg(contactName);
        }
    }
    void mOMxFile::writeCaseOutPut(MechCaseData::mCaseBase* caseBase)
    {
        QHash<QString, QStringList>& outPut = caseBase->getOutputs();
        if (outPut.isEmpty())
            return;

        // 第一步：先收集所有有效的输出关键字（避免空块）
        QStringList validOutputs;
        QHash<QString, QHash<QString, QStringList>> selectedSet = caseBase->getSelectedSet();
        for (auto it = outPut.constBegin(); it != outPut.constEnd(); ++it)
        {
            QString key = it.key();
            for (const QString& str : it.value()) // 使用const引用避免拷贝
            {
                QString outkeyword = mVariableFunction::getVariableByOutPutPair({ key, str });
                QStringList sets;
                if (selectedSet.contains(key) && selectedSet.value(key).contains(str)) {
                    sets = selectedSet.value(key).value(str);
                }
                if (!outkeyword.isEmpty() && !sets.isEmpty()) {
                    QString resultStr = outkeyword;
                    for (auto setname : sets) {
                        resultStr += ",\"" + setname + "\"";
                    }
                    validOutputs.append(resultStr);
                }
                else if (!outkeyword.isEmpty())
                {
                    validOutputs.append(outkeyword);
                }
            }
        }

        // 没有任何有效关键字，直接返回，不输出任何内容
        if (validOutputs.isEmpty())
            return;

        // 第二步：统一写入，格式严格控制
        QString outputType = caseBase->getOutputType();
        QStringList outputTypeList = { "op2","h5" };
        _out << "*output";
        /*
        if (caseBase->getCaseType() == ImplicitRandomVibration)
        {
            _out << ", type = abs\r\n";
        }*/
        if (outputTypeList.contains(outputType)) {
            _out << ",type=" << outputType << "\r\n";
        }
        else
        {
            _out << "\r\n";
        }
        for (int i = 0; i < validOutputs.size(); ++i)
        {
            _out << validOutputs[i];
            _out << "\r\n";
        }
    }

    void mOMxFile::writeCasePchOutPut(MechCaseData::mCaseBase* caseBase)
    {
        QHash<QString, QStringList>& outPut = caseBase->getPchOutputs();
        if (outPut.isEmpty())
            return;

        // 第一步：先收集所有有效的输出关键字（避免空块）
        QStringList validOutputs;
        QHash<QString, QHash<QString, QStringList>> selectedSet = caseBase->getPchSelectedSet();
        for (auto it = outPut.constBegin(); it != outPut.constEnd(); ++it)
        {
            QString key = it.key();
            for (const QString& str : it.value()) // 使用const引用避免拷贝
            {
                QString outkeyword = mVariableFunction::getVariableByOutPutPair({ key, str });
                QStringList sets;
                if (selectedSet.contains(key) && selectedSet.value(key).contains(str)) {
                    sets = selectedSet.value(key).value(str);
                }
                if (!outkeyword.isEmpty() && !sets.isEmpty()) {
                    QString resultStr = outkeyword;
                    for (auto setname : sets) {
                        resultStr += ",\"" + setname + "\"";
                    }
                    validOutputs.append(resultStr);
                }
                else if (!outkeyword.isEmpty())
                {
                    validOutputs.append(outkeyword);
                }
            }
        }

        // 没有任何有效关键字，直接返回，不输出任何内容
        if (validOutputs.isEmpty())
            return;

        // 第二步：统一写入，格式严格控制
        _out << "*output";
        QList<CaseType> pchTypes = { ImplicitRandomVibration, ImplicitFrequency, ImplicitTransient, ASCoupling };
        if (pchTypes.contains(caseBase->getCaseType()))
        {
            _out << ",type=pch\r\n";
        }
        else
        {
            _out << "\r\n";
        }
        for (int i = 0; i < validOutputs.size(); ++i)
        {
            _out << validOutputs[i];
            _out << "\r\n";
        }
    }

    bool mOMxFile::writeGlobalOutputControl()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        auto data = _modelData->getGlobalParam()->getGlobalOutputControlData();
        if (!data)
        {
            return true;
        }

        QHash<QString, QStringList> outPut = data->getOutputSelections();
        if (outPut.isEmpty())
        {
            return true;
        }

        // 收集所有有效的输出关键字（避免空块）
        QStringList validOutputs;
        for (auto it = outPut.constBegin(); it != outPut.constEnd(); ++it)
        {
            for (const QString& str : it.value())
            {
                QString outkeyword = mVariableFunction::getVariableByOutPutPair({ it.key(), str });
                if (!outkeyword.isEmpty())
                {
                    validOutputs.append(outkeyword);
                }
            }
        }

        if (validOutputs.isEmpty())
        {
            return true;
        }

        _out << "*globaloutput\r\n";
        for (int i = 0; i < validOutputs.size(); ++i)
        {
            _out << validOutputs[i];
            _out << "\r\n";
        }

        return true;
    }

    void mOMxFile::writeHistoryOutput(MechCaseData::mCaseBase* caseBase)
    {
        // 1. 检查是否启用历史输出
        if (!caseBase->getHistoryOutputEnabled())
        {
            return;
        }

        // 2. 中文名称 -> 求解器关键字映射表（严格对应）
        static const QHash<QString, QString> keywordMap = {
            {"位移", "disp"},
            {"速度", "vel"},
            {"加速度", "acc"},
            {"应变", "strain"},
            {"单元应变", "estrain"},
            {"应力", "stress"},
            {"单元应力", "estress"},
            {"温度", "temperature"},
            {"反作用力", "rf"}
        };

        // 3. 获取输出集合
        int setID = _modelData->getMapRelationManager().getMapRelation<int>(
            { Physical_Case, caseBase->getCaseId() },
            Physical_Set,
            MapRelation_HistorySet
        );
        QString setName = _modelData->getNameByPhyID(Physical_Set, setID);

        // 4. 数据类型转换（界面值 -> 求解器关键字值）
        QString dataType;
        QString uiDataType = caseBase->getHstDataType();
        if (uiDataType == "实数")
            dataType = "real";
        else if (uiDataType == "复数")
            dataType = "imag";
        else if (uiDataType == "幅值")
            dataType = "mag";
        else if (uiDataType == "相位")
            dataType = "phase";
        else if (uiDataType == "全部")
            dataType = "both";
        else // "无"或其他
            dataType = "";

        // 5. 构建第一行关键字（保留逗号后的空格，与示例完全一致）
        QString firstLine = QString("*history, filetype = %1, type = %2")
            .arg(caseBase->getHstFileType())
            .arg(dataType);

        firstLine += QString(", set = \"%1\"").arg(setName);

        // 6. 收集所有输出项
        QStringList outputLines;

        // 6.1 基本结果、应变、应力等常规输出（中文转英文）
        QHash<QString, QStringList> historyOutputs = caseBase->getHistoryOutputs();
        for (auto it = historyOutputs.constBegin(); it != historyOutputs.constEnd(); ++it)
        {
            const QStringList& items = it.value();
            for (const QString& item : items)
            {
                if (keywordMap.contains(item))
                {
                    outputLines.append(keywordMap[item]);
                }
            }
        }

        // 6.2 动刚度结果输出（保留逗号后的空格）
        QStringList kdOutputs = caseBase->getHistoryKdOutputs();
        QString dynamicStiff = caseBase->getHstDynamicStiff();

        for (const QString& kdItem : kdOutputs)
        {
            if (kdItem == "平均法")
            {
                outputLines.append("Kd, average");
            }
            else if (kdItem == "指数法")
            {
                outputLines.append("Kd, index");
            }
            else if (kdItem == "面积法")
            {
                if (!dynamicStiff.isEmpty() && dynamicStiff != "0")
                {
                    outputLines.append(QString("Kd, area, %1").arg(dynamicStiff));
                }
                else
                {
                    outputLines.append("Kd, area");
                }
            }
        }

        // 7. 只有当有输出项时才写入文件（严格控制换行）
        if (!outputLines.isEmpty())
        {
            _out << firstLine << "\r\n";
            for (int i = 0; i < outputLines.size(); ++i)
            {
                _out << outputLines[i];
                _out << "\r\n";
            }
        }
    }
    void mOMxFile::writeNsmData(MechCaseData::mCaseBase* caseBase)
    {
        QList<int> nsmIds = _modelData->getMapRelationManager().getMapRelation<QList<int>>(
            { Physical_Case, caseBase->getCaseId() },
            Physical_NonStructuralQuality);
        if (nsmIds.size() > 0)
            _out << "*nsm\r\n";
        for (auto id : nsmIds)
        {
            QString str = _modelData->getNameByPhyID(Physical_NonStructuralQuality, id);
            if (!str.isEmpty())
                _out << QString("\"%1\"").arg(str) << "\r\n";
        }
    }

    void mOMxFile::writeModelFixData(MechCaseData::mCaseBase* caseBase)
    {
        QList<int> modelFixIds = _modelData->getMapRelationManager().getMapRelation<QList<int>>(
            { Physical_Case, caseBase->getCaseId() },
            Physical_ModelFix);
        if (modelFixIds.size() > 0)
            _out << "*modelchange\r\n";
        for (auto id : modelFixIds)
        {
            QString str = _modelData->getNameByPhyID(Physical_ModelFix, id);
            if (!str.isEmpty())
                _out << QString("\"%1\"").arg(str) << "\r\n";
        }
    }

    void mOMxFile::writeOmega(MechCaseData::mCaseBase* caseBase)
    {
        // 写出与该复模态工况映射的旋转速度/旋转加速度名称列表
        std::set<int> rotateIDs = _modelData->getMapRelationManager().getMapRelation<std::set<int>>(
            { Physical_Case, caseBase->getCaseId() }, Physical_RotateAnalyse);
        if (!rotateIDs.empty())
        {
            _out << "*omega\r\n";
            for (int rotateID : rotateIDs)
            {
                QString str = _modelData->getNameByPhyID(Physical_RotateAnalyse, rotateID);
                if (!str.isEmpty())
                    _out << QString("\"%1\"").arg(str) << "\r\n";
            }
        }
    }

    void mOMxFile::writeImplicitLinearStatic(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitLinearStatic* analysis = dynamic_cast<MechCaseData::mCaseImplicitLinearStatic*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = linearstatic";
        if (analysis->_inertiaRelief == "关闭")
        {
            _out << ", inrel = 0";
        }
        else if (analysis->_inertiaRelief == "手动开启")
        {
            _out << ", inrel = -1";
        }
        else if (analysis->_inertiaRelief == "自动开启")
        {
            _out << ", inrel = -2";
        }

        if (analysis->_equationSolver == "直接法求解器DSV")
        {
            _out << ", solver = dsv";
        }
        else if (analysis->_equationSolver == "迭代法求解器Pariters")
        {
            _out << ", solver = pariters";
        }

        if (analysis->_autospc == "开启")
        {
            _out << ", autospc = on";
        }
        else if (analysis->_autospc == "关闭")
        {
            _out << ", autospc = off";
        }

        _out << "\r\n";

        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }
    void mOMxFile::writeImplicitNonLinearStatic(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitNonLinearStatic* analysis = dynamic_cast<MechCaseData::mCaseImplicitNonLinearStatic*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = nonlinear";

        if (analysis->_nonLinearity == "开")
        {
            _out << ", nlgeo = on";
        }
        else if (analysis->_nonLinearity == "关")
        {
            _out << ", nlgeo = off";
        }

        if (analysis->_timeStepType == "固定")
        {
            _out << ", direct = on";
        }
        else if (analysis->_timeStepType == "自动")
        {
            _out << ", direct = off";
        }

        if (analysis->_cntnlsub == "开启")
        {
            _out << ", cntnlsub = on";
        }
        else if (analysis->_cntnlsub == "关闭")
        {
            _out << ", cntnlsub = off";
        }

        if (analysis->_inertiaRelief == "关闭")
        {
            _out << ", inrel = 0";
        }
        else if (analysis->_inertiaRelief == "手动开启")
        {
            _out << ", inrel = -1";
        }
        else if (analysis->_inertiaRelief == "自动开启")
        {
            _out << ", inrel = -2";
        }

        if (analysis->_Sparsematrix == "关闭")
        {
            _out << ", sparsematrix = symm";
        }
        else if (analysis->_Sparsematrix == "开启")
        {
            _out << ", sparsematrix = unsymm";
        }
        _out << "\r\n";

        if (analysis->_visco)
        {
            _out << QString("*visco, cetol = %1\r\n").arg(analysis->_cetol);
        }
        else
        {
            _out << "*static\r\n";
        }
        _out << QString("%1, %2").arg(analysis->_initialStepValue).arg(analysis->_totolTimeValue);
        if (analysis->_timeStepType == "自动")
        {
            _out << QString(", %1, %2").arg(analysis->_minTimeStep).arg(analysis->_maxTimeStep);
        }
        else
        {
            _out << QString(", , ");
        }
        _out << "\r\n";

        writeNsmData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseContactinterference(caseBase);
        writeModelFixData(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }
    void mOMxFile::writeImplicitNonLinearDynamic(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitNonLinearDynamic* analysis = dynamic_cast<MechCaseData::mCaseImplicitNonLinearDynamic*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = nonlinear";

        if (analysis->_nonLinearity == "开")
        {
            _out << ", nlgeo = on";
        }
        else if (analysis->_nonLinearity == "关")
        {
            _out << ", nlgeo = off";
        }

        if (analysis->_timeStepType == "固定")
        {
            _out << ", direct = on";
        }
        else if (analysis->_timeStepType == "自动")
        {
            _out << ", direct = off";
        }

        if (analysis->_cntnlsub == "开启")
        {
            _out << ", cntnlsub = on";
        }
        else if (analysis->_cntnlsub == "关闭")
        {
            _out << ", cntnlsub = off";
        }

        if (analysis->_inertiaRelief == "关闭")
        {
            _out << ", inrel = 0";
        }
        else if (analysis->_inertiaRelief == "手动开启")
        {
            _out << ", inrel = -1";
        }
        else if (analysis->_inertiaRelief == "自动开启")
        {
            _out << ", inrel = -2";
        }

        _out << "\r\n";

        _out << "*dynamic";
        if (analysis->_quasiStaticAnasys == "开")
        {
            _out << ", quasistatic = on";
        }
        else if (analysis->_quasiStaticAnasys == "关")
        {
            _out << ", quasistatic = off";
        }
        _out << "\r\n";
        _out << QString("%1, %2").arg(analysis->_initialStepValue).arg(analysis->_totolTimeValue);
        if (analysis->_timeStepType == "自动")
        {
            _out << QString(", %1, %2").arg(analysis->_minTimeStep).arg(analysis->_maxTimeStep);
        }
        else
        {
            _out << QString(", , ");
        }
        _out << "\r\n";


        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseContactinterference(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }
    void mOMxFile::writeImplicitPostBuckling(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitNonLinearArc* analysis = dynamic_cast<MechCaseData::mCaseImplicitNonLinearArc*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = nonlinear";

        if (analysis->_nonLinearity == "开")
        {
            _out << ", nlgeo = on";
        }
        else if (analysis->_nonLinearity == "关")
        {
            _out << ", nlgeo = off";
        }

        if (analysis->_timeStepType == "固定")
        {
            _out << ", direct = on";
        }
        else if (analysis->_timeStepType == "自动")
        {
            _out << ", direct = off";
        }

        if (analysis->_cntnlsub == "开启")
        {
            _out << ", cntnlsub = on";
        }
        else if (analysis->_cntnlsub == "关闭")
        {
            _out << ", cntnlsub = off";
        }

        if (analysis->_inertiaRelief == "关闭")
        {
            _out << ", inrel = 0";
        }
        else if (analysis->_inertiaRelief == "手动开启")
        {
            _out << ", inrel = -1";
        }
        else if (analysis->_inertiaRelief == "自动开启")
        {
            _out << ", inrel = -2";
        }

        _out << "\r\n";

        _out << "*arc\r\n";
        _out << QString("%1, %2").arg(analysis->_initialStepValue).arg(analysis->_totolTimeValue);
        if (analysis->_timeStepType == "自动")
        {
            _out << QString(", %1, %2").arg(analysis->_minTimeStep).arg(analysis->_maxTimeStep);
        }
        else
        {
            _out << QString(", , ");
        }
        _out << QString(", %1").arg(analysis->_lMax);
        _out << QString(", , , ");
        _out << QString(", %1").arg(analysis->_nMax);
        _out << "\r\n";

        if (analysis->_initBug)
        {
            QString typeStr = (analysis->_initBugType == MxFunctions::InitBugType::Static) ? "static" : "buckling";
            _out << QString("*imperfection, type = %1").arg(typeStr);
            if (!analysis->_initBugFileName.isEmpty())
            {
                _out << QString(", file = \"%1\"").arg(analysis->_initBugFileName) << "\r\n";
            }
            if (!analysis->_scaleValue.isEmpty())
            {
                _out << QString("%1").arg(analysis->_scaleValue);
            }
            _out << "\r\n";
        }

        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseContactinterference(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }
    void mOMxFile::writeImplicitSteadyHeat(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitSteadyHeat* analysis = dynamic_cast<MechCaseData::mCaseImplicitSteadyHeat*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = heattransfer";

        if (analysis->_timeStepType == "固定")
        {
            _out << ", direct = on";
        }
        else if (analysis->_timeStepType == "自动")
        {
            _out << ", direct = off";
        }

        if (analysis->_cntnlsub == "开启")
        {
            _out << ", cntnlsub = on";
        }
        else if (analysis->_cntnlsub == "关闭")
        {
            _out << ", cntnlsub = off";
        }

        _out << "\r\n";

        _out << "*steady\r\n";
        _out << QString("%1, %2").arg(analysis->_initialStepValue).arg(analysis->_totolTimeValue);
        if (analysis->_timeStepType == "自动")
        {
            _out << QString(", %1, %2").arg(analysis->_minTimeStep).arg(analysis->_maxTimeStep);
        }
        else
        {
            _out << QString(", , ");
        }
        _out << "\r\n";


        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }
    void mOMxFile::writeImplicitTransientHeat(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitTransientHeat* analysis = dynamic_cast<MechCaseData::mCaseImplicitTransientHeat*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = heattransfer";

        if (analysis->_timeStepType == "固定")
        {
            _out << ", direct = on";
        }
        else if (analysis->_timeStepType == "自动")
        {
            _out << ", direct = off";
        }

        if (analysis->_cntnlsub == "开启")
        {
            _out << ", cntnlsub = on";
        }
        else if (analysis->_cntnlsub == "关闭")
        {
            _out << ", cntnlsub = off";
        }

        _out << "\r\n";

        _out << "*transient\r\n";
        _out << QString("%1, %2").arg(analysis->_initialStepValue).arg(analysis->_totolTimeValue);
        if (analysis->_timeStepType == "自动")
        {
            _out << QString(", %1, %2").arg(analysis->_minTimeStep).arg(analysis->_maxTimeStep);
        }
        else
        {
            _out << QString(", , ");
        }
        _out << "\r\n";


        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }

    void mOMxFile::writeImplicitStructuralCouplingHeat(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitStructuralCouplingHeat* analysis = dynamic_cast<MechCaseData::mCaseImplicitStructuralCouplingHeat*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = heattransfer";

        if (analysis->_timeStepType == "固定")
        {
            _out << ", direct = on";
        }
        else if (analysis->_timeStepType == "自动")
        {
            _out << ", direct = off";
        }

        if (analysis->_cntnlsub == "开启")
        {
            _out << ", cntnlsub = on";
        }
        else if (analysis->_cntnlsub == "关闭")
        {
            _out << ", cntnlsub = off";
        }

        _out << "\r\n";

        _out << "*coupledtempdisp\r\n";
        _out << QString("%1, %2").arg(analysis->_initialStepValue).arg(analysis->_totolTimeValue);
        if (analysis->_timeStepType == "自动")
        {
            _out << QString(", %1, %2").arg(analysis->_minTimeStep).arg(analysis->_maxTimeStep);
        }
        else
        {
            _out << QString(", , ");
        }
        _out << "\r\n";


        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }

    void mOMxFile::writeImplicitModal(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitModal* analysis = dynamic_cast<MechCaseData::mCaseImplicitModal*>(caseBase);
        if (!analysis)
        {
            return;
        }

        auto writeBlocklanczos = [=]() {
            _out << "*eigrl";
            if (analysis->_normalization == "广义质量")
            {
                _out << ", normalization = mass";
            }
            else if (analysis->_normalization == "最大位移")
            {
                _out << ", normalization = displacement";
            }
            _out << "\r\n";

            _out << QString("%1, %2, %3, %4\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
                .arg(analysis->_order).arg(analysis->_tolerance);
            };


        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis";
        if (analysis->_modalType == "复模态分析")
        {
            if (analysis->_rotationalChecked)
            {
                _out << ", type = rotor_modes";

            }
            else
            {
                _out << ", type = complexmodes1";
            }

            if (analysis->_complexMethod == "精确法")
            {
                _out << ", method = exact";
            }
            else if (analysis->_complexMethod == "估计法")
            {
                _out << ", method = approximate";
            }
            _out << "\r\n";

            if (analysis->_rotationalChecked)
            {
                QString standCoor;
                if (analysis->_standCoor == "静止坐标系")
                {
                    standCoor = "stationary";
                }
                else if (analysis->_standCoor == "旋转坐标系")
                {
                    standCoor = "rotating";
                }
                _out << QString("*coriolis, system = %1").arg(standCoor);
                _out << "\r\n";
                if (analysis->_picChecked)
                {
                    _out << "*plcamp\r\n";
                    _out << analysis->_picValue << "\r\n";
                }

                writeOmega(caseBase);
            }
            else
            {
                writeBlocklanczos();
            }


            _out << "*eigc\r\n";
            _out << QString("%1\r\n").arg(analysis->_rootNum);

            if (!analysis->_dampingG.isEmpty())
            {
                _out << "*damp\r\n";
                _out << QString("%1\r\n").arg(analysis->_dampingG);
            }
        }
        else
        {
            _out << ", type = modes";
            if (analysis->_nlgeo)
            {
                _out << ", nlgeo = on";
            }

            if (analysis->_modalSolver == "自动多重子结构")
            {
                _out << ", solver = amls";
                _out << "\r\n";

                _out << "*eigra\r\n";
                _out << QString("%1, %2, %3, %4, %5\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
                    .arg(analysis->_order).arg(analysis->_tolerance).arg(analysis->_ampffact);
            }
            else if (analysis->_modalSolver == "兰索斯")
            {
                _out << ", solver = blocklanczos";
                _out << "\r\n";

                writeBlocklanczos();
            }
        }


        int temp_caseID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Case);
        QString temp_caseName = _modelData->getNameByPhyID(Physical_Case, temp_caseID);
        if (!temp_caseName.isEmpty())
        {
            _out << "*preload\r\n";
            _out << QString("\"%1\"\r\n").arg(temp_caseName);
        }

        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }
    void mOMxFile::writeImplicitLinearBuckling(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitLinearBuckling* analysis = dynamic_cast<MechCaseData::mCaseImplicitLinearBuckling*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = buckling";

        int temp_caseID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Case);
        QString temp_caseName = _modelData->getNameByPhyID(Physical_Case, temp_caseID);
        _out << QString(", statsub = \"%1\"").arg(temp_caseName);
        _out << "\r\n";

        _out << "*eigrl\r\n";
        _out << QString("%1, %2, %3, %4\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
            .arg(analysis->_order).arg(analysis->_tolerance);


        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }




    void mOMxFile::writeImplASCoupling(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplASCoupling* analysis = dynamic_cast<MechCaseData::mCaseImplASCoupling*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis";
        //if (analysis->_method == "模态法")
        //{
        _out << ", type = frequencyresponse_modal1";
        if (analysis->_isRemnantOpen)
        {
            _out << ", resvec = on";
        }
        _out << "\r\n";

        if (analysis->_modalSolver == "自动多重子结构")
        {
            _out << "*eigra, field = structure\r\n";
            _out << QString("%1, %2, %3, %4, %5\r\n").arg(analysis->_structMinFrequency).arg(analysis->_structMaxFrequency)
                .arg(analysis->_structOrder).arg(analysis->_structTolerance).arg(analysis->_structAmpffact);

            _out << "*eigra, field = fluid\r\n";
            _out << QString("%1, %2, %3, %4, %5\r\n").arg(analysis->_acousticsMinFrequency).arg(analysis->_acousticsMaxFrequency)
                .arg(analysis->_acousticsOrder).arg(analysis->_acousticsTolerance).arg(analysis->_acousticsAmpffact);
        }
        else if (analysis->_modalSolver == "兰索斯")
        {

            _out << "*eigrl, field = structure";
            if (analysis->_structNormalization == "广义质量")
            {
                _out << ", normalization = mass";
            }
            else if (analysis->_structNormalization == "最大位移")
            {
                _out << ", normalization = displacement";
            }
            _out << "\r\n";

            _out << QString("%1, %2, %3, %4\r\n").arg(analysis->_structMinFrequency).arg(analysis->_structMaxFrequency)
                .arg(analysis->_structOrder).arg(analysis->_structTolerance);


            _out << "*eigrl, field = fluid";
            if (analysis->_acousticsNormalization == "广义质量")
            {
                _out << ", normalization = mass";
            }
            else if (analysis->_acousticsNormalization == "最大位移")
            {
                _out << ", normalization = displacement";
            }
            _out << "\r\n";

            _out << QString("%1, %2, %3, %4\r\n").arg(analysis->_acousticsMinFrequency).arg(analysis->_acousticsMaxFrequency)
                .arg(analysis->_acousticsOrder).arg(analysis->_acousticsTolerance);
        }


        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Set, MapRelation_ElforceSet);
        QString setName = _modelData->getNameByPhyID(Physical_Set, setID);
        if (!setName.isEmpty())
        {
            _out << QString("*elforce, set = \"%1\"\r\n").arg(setName);
        }

        setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Set, MapRelation_GpforceSet);
        setName = _modelData->getNameByPhyID(Physical_Set, setID);
        if (!setName.isEmpty())
        {
            _out << QString("*gpforce, set = \"%1\"\r\n").arg(setName);
        }
        /*  }
          else
          {
              _out << ", type = frequencyresponse_direct";
              _out << "\r\n";
          }*/

        _out << "*damp, field = structure\r\n";
        _out << QString("%1, %2, %3, ").arg(analysis->_structDampingG).arg(analysis->_structDampingAlpha1)
            .arg(analysis->_structDampingAlpha2);
        int curveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Function);
        QString curveName = _modelData->getNameByPhyID(Physical_Function, curveID);
        if (!curveName.isEmpty())
        {
            _out << QString("\"%1\"").arg(curveName);
        }
        _out << "\r\n";
        _out << "*damp, field = fluid\r\n";
        _out << QString("%1, %2, %3, ").arg(analysis->_acousticsDampingG).arg(analysis->_acousticsDampingAlpha1)
            .arg(analysis->_acousticsDampingAlpha2);
        int acousticsCurveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Function);
        QString acousticsCurveName = _modelData->getNameByPhyID(Physical_Function, acousticsCurveID);
        if (!acousticsCurveName.isEmpty())
        {
            _out << QString("\"%1\"").arg(acousticsCurveName);
        }
        _out << "\r\n";

        int temp_caseID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Case);
        QString temp_caseName = _modelData->getNameByPhyID(Physical_Case, temp_caseID);
        if (!temp_caseName.isEmpty())
        {
            _out << "*preload\r\n";
            _out << QString("\"%1\"\r\n").arg(temp_caseName);
        }
        //写采样点频率
        if (analysis->checked)
        {
            _out << "*freq\r\n";
            //获取映射
            int id = caseBase->getCaseId();
            QList<int> freqIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, id }, Physical_FreqSamplPoint);

            for (auto id : freqIDs)
            {
                //根据id获取采样频率点数据
                MechCaseData::mFreqSamplPointBase* data = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getFreqSamplPointData(id);
                //获勾选情况进行拆分
                QList<bool> checked = data->_checked;
                if (!checked.contains(true)) return;

                if (checked[0])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_1");
                    _out << "\r\n";
                }
                if (checked[1])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_2");
                    _out << "\r\n";
                }
                if (checked[2])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_3");
                    _out << "\r\n";
                }
                if (checked[3])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_5");
                    _out << "\r\n";
                }
                if (checked[4])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_4");
                    _out << "\r\n";
                }
            }
        }

        // 写模态贡献量：自己有写自己的，自己没有写模型树全局的
        mModalContriData* caseModalContri = analysis->getModalContriData();
        if (caseModalContri)
        {
            writeModalContriData(caseModalContri);
        }
        else
        {
            mModalContriData* globalModalContri = _modelData->getGlobalParam()->getModalContriData();
            if (globalModalContri)
            {
                writeModalContriData(globalModalContri);
            }
        }

        // 写节点贡献量：自己有写自己的，自己没有写模型树全局的
        mNodeContriData* caseNodeContri = analysis->getNodeContriData();
        if (caseNodeContri)
        {
            writeNodeContriData(caseNodeContri);
        }
        else
        {
            mNodeContriData* globalNodeContri = _modelData->getGlobalParam()->getNodeContriData();
            if (globalNodeContri)
            {
                writeNodeContriData(globalNodeContri);
            }
        }

        // 写面板贡献量：自己有写自己的，自己没有写模型树全局的
        mPanelContriData* casePanelContri = analysis->getPanelContriData();
        if (casePanelContri)
        {
            writePanelContriData(casePanelContri);
        }
        else
        {
            mPanelContriData* globalPanelContri = _modelData->getGlobalParam()->getPanelContriData();
            if (globalPanelContri)
            {
                writePanelContriData(globalPanelContri);
            }
        }

        writeERPData();
        writeCasePeakFilterData(caseBase);

        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        writeCasePchOutPut(caseBase);
        writeHistoryOutput(caseBase);
        _out << "\r\n";
    }


    void mOMxFile::writeImplicitFrequency(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitFrequency* analysis = dynamic_cast<MechCaseData::mCaseImplicitFrequency*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis";
        if (analysis->_method == "模态法")
        {
            _out << ", type = frequencyresponse_modal";
            if (analysis->_isRemnantOpen)
            {
                _out << ", resvec = on";
            }
            _out << "\r\n";

            if (analysis->_modalSolver == "自动多重子结构")
            {
                _out << "*eigra\r\n";
                _out << QString("%1, %2, %3, %4, %5\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
                    .arg(analysis->_order).arg(analysis->_tolerance).arg(analysis->_ampffact);
            }
            else if (analysis->_modalSolver == "兰索斯")
            {
                _out << "*eigrl";
                if (analysis->_normalization == "广义质量")
                {
                    _out << ", normalization = mass";
                }
                else if (analysis->_normalization == "最大位移")
                {
                    _out << ", normalization = displacement";
                }
                _out << "\r\n";

                _out << QString("%1, %2, %3, %4\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
                    .arg(analysis->_order).arg(analysis->_tolerance);
            }


            int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Set, MapRelation_ElforceSet);
            QString setName = _modelData->getNameByPhyID(Physical_Set, setID);
            if (!setName.isEmpty())
            {
                _out << QString("*elforce, set = \"%1\"\r\n").arg(setName);
            }

            setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Set, MapRelation_GpforceSet);
            setName = _modelData->getNameByPhyID(Physical_Set, setID);
            if (!setName.isEmpty())
            {
                _out << QString("*gpforce, set = \"%1\"\r\n").arg(setName);
            }
        }
        else
        {
            _out << ", type = frequencyresponse_direct";
            _out << "\r\n";
        }

        _out << "*damp\r\n";
        _out << QString("%1, %2, %3, ").arg(analysis->_dampingG).arg(analysis->_dampingAlpha1)
            .arg(analysis->_dampingAlpha2);
        int curveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Function);
        QString curveName = _modelData->getNameByPhyID(Physical_Function, curveID);
        if (!curveName.isEmpty())
        {
            _out << QString("\"%1\"").arg(curveName);
        }
        _out << "\r\n";

        int temp_caseID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Case);
        QString temp_caseName = _modelData->getNameByPhyID(Physical_Case, temp_caseID);
        if (!temp_caseName.isEmpty())
        {
            _out << "*preload\r\n";
            _out << QString("\"%1\"\r\n").arg(temp_caseName);
        }

        //写采样点频率
        if (analysis->checked)
        {
            _out << "*freq\r\n";
            //获取映射
            int id = caseBase->getCaseId();
            QList<int> freqIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, id }, Physical_FreqSamplPoint);

            for (auto id : freqIDs)
            {
                if (id == -1) continue;
                //根据id获取采样频率点数据
                MechCaseData::mFreqSamplPointBase* data = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getFreqSamplPointData(id);
                if (!data)continue;
                //获勾选情况进行拆分
                QList<bool> checked = data->_checked;
                if (!checked.contains(true)) return;

                if (checked[0])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_1");
                    _out << "\r\n";
                }
                if (checked[1])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_2");
                    _out << "\r\n";
                }
                if (checked[2])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_3");
                    _out << "\r\n";
                }
                if (checked[3])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_5");
                    _out << "\r\n";
                }
                if (checked[4])
                {
                    _out << QString("\"%1\"").arg(data->getFreqSamplPointName() + "_4");
                    _out << "\r\n";
                }
            }
        }

        //写模态贡献量
        mModalContriData* caseModalContri = analysis->getModalContriData();
        if (caseModalContri)
        {
            writeModalContriData(caseModalContri);
        }
        else
        {
            mModalContriData* globalModalContri = _modelData->getGlobalParam()->getModalContriData();
            if (globalModalContri)
            {
                writeModalContriData(globalModalContri);
            }
        }

        writeERPData();
        writeCasePeakFilterData(caseBase);

        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        writeCasePchOutPut(caseBase);
        writeHistoryOutput(caseBase);
        _out << "\r\n";
    }

    void mOMxFile::writeModalContriData(MechGlobalParamData::mModalContriData* data)
    {
        if (!data) return;

        //获取输出集合名称
        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_ModalContri, data->getId() }, Physical_Set, MapRelation_OutSet);
        QString setName = _modelData->getNameByPhyID(Physical_Set, setID);

        //获取频率点集合名称
        int freqSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_ModalContri, data->getId() }, Physical_Set, MapRelation_FrequencySet);
        QString freqSetName = _modelData->getNameByPhyID(Physical_Set, freqSetID);

        //写第一行：*mcf 关键字
        _out << "*mcf";
        // type 为必选
        _out << QString(", type = %1").arg(data->_outPutType);
        // set 为可选：开启峰值筛选时可不设置，未开启时为必选（完整性检查保证），可与 peakout 共存
        if (!setName.isEmpty())
        {
            _out << QString(", set = \"%1\"").arg(setName);
        }
        // frequency 为可选，为空则不写
        if (!freqSetName.isEmpty())
        {
            _out << QString(", frequency = \"%1\"").arg(freqSetName);
        }
        // sort 为可选，为空则不写
        if (!data->_SortType.isEmpty())
        {
            _out << QString(", sort = %1").arg(data->_SortType);
        }
        if (data->_checkBoxPkScr)
        {
            _out << ", peakout = on";
        }
        _out << "\r\n";

        //写第二行：结果类型、过滤分数、输出阶数（均为可选，空值保留逗号占位）
        QString fltScore = data->_fltScoreChecked ? data->_fltScore : "";
        QString outOrd = data->_OutOrdChecked ? data->_OutOrd : "";
        _out << QString("%1, %2, %3\r\n").arg(data->_ResultType).arg(fltScore).arg(outOrd);
    }

    void mOMxFile::writeNodeContriData(MechGlobalParamData::mNodeContriData* data)
    {
        if (!data) return;

        // 获取结构节点集合名称
        int gridSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_NodeContri, data->getId() }, Physical_Set, MapRelation_GridSet);
        QString gridSetName = _modelData->getNameByPhyID(Physical_Set, gridSetID);

        // 获取声腔节点集合名称
        int gridfSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_NodeContri, data->getId() }, Physical_Set, MapRelation_GridfSet);
        QString gridfSetName = _modelData->getNameByPhyID(Physical_Set, gridfSetID);

        // 获取频率点集合名称 (solution)
        int freqSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_NodeContri, data->getId() }, Physical_Set, MapRelation_FrequencySet);
        QString freqSetName = _modelData->getNameByPhyID(Physical_Set, freqSetID);

        // 写第一行：*pfgrid
        _out << "*pfgrid";
        // grids 为可选，为空则不写
        if (!gridSetName.isEmpty())
        {
            _out << QString(", grids = \"%1\"").arg(gridSetName);
        }
        // gridf 为可选，为空则不写
        if (!gridfSetName.isEmpty())
        {
            _out << QString(", gridf = \"%1\"").arg(gridfSetName);
        }
        // solution 为可选，为空则不写
        if (!freqSetName.isEmpty())
        {
            _out << QString(", solution = \"%1\"").arg(freqSetName);
        }

        // 响应截断值
        if (data->_rpTruncType == "帕斯卡" && !data->_rpTruncValue.isEmpty())
        {
            _out << QString(", rpcutoff = %1").arg(data->_rpTruncValue);
        }
        else if (data->_rpTruncType == "分贝计权" && !data->_rpTruncValue.isEmpty())
        {
            _out << QString(", rpdbacut = %1").arg(data->_rpTruncValue);
        }

        if (data->_checkBoxCavityProj)
        {
            _out << ", contour = on";
        }

        if (data->_checkBoxPkScr)
        {
            _out << ", peakout = on";
        }
        _out << "\r\n";

        // 写第二行：响应点集合（可与 peakout 共存）：设置了才写该行，未设置时整行不写；
        // 未开启峰值筛选时响应点集合为必选，异常空值仍写空占位行
        int respSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_NodeContri, data->getId() }, Physical_Set, MapRelation_ResponseGridSet);
        QString respSetName = _modelData->getNameByPhyID(Physical_Set, respSetID);
        if (!respSetName.isEmpty())
        {
            _out << QString("\"%1\"\r\n").arg(respSetName);
        }
        else if (!data->_checkBoxPkScr)
        {
            _out << "\"\"\r\n";
        }
    }

    void mOMxFile::writePanelContriData(MechGlobalParamData::mPanelContriData* data)
    {
        if (!data) return;

        // 获取频率点集合名称
        int freqSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_PanelContri, data->getId() }, Physical_Set, MapRelation_FrequencySet);
        QString freqSetName = _modelData->getNameByPhyID(Physical_Set, freqSetID);

        // 写第一行：*pfpanel
        _out << "*pfpanel";
        // frequency 为可选，为空则不写
        if (!freqSetName.isEmpty())
        {
            _out << QString(", frequency = \"%1\"").arg(freqSetName);
        }
        // 输出类型：仅"幅值"需要写type，"实部"/"虚部"不写type（可选）
        if (data->_outPutType == "幅值")
        {
            _out << ", type = phase";
        }
        if (data->_checkBoxPkScr)
        {
            _out << ", peakout = on";
        }
        _out << "\r\n";

        // 写第二行：响应点集合（可与 peakout 共存）：设置了才写该行，未设置时整行不写
        int respSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_PanelContri, data->getId() }, Physical_Set, MapRelation_ResponseGridSet);
        QString respSetName = _modelData->getNameByPhyID(Physical_Set, respSetID);
        if (!respSetName.isEmpty())
        {
            _out << QString("\"%1\"\r\n").arg(respSetName);
        }
    }

    bool mOMxFile::writeTPContriData()
    {
        mTPContriData* tpData = _modelData->getGlobalParam()->getTPContriData();
        if (!tpData)
        {
            return true;
        }

        mGlobalSignals::getInstance()->outputMessageSig(0, QString("传递路径贡献量写文件开始"), true, true);

        int id = tpData->getId();

        // 第一行：*path, type = xxx, name = "xxx"  (type可选)
        _out << "*path";
        if (!tpData->_respType.isEmpty())
        {
            _out << QString(", type = %1").arg(tpData->_respType);
        }
        _out << QString(", name = \"TPA_1\"\r\n"); //tpData->getName()

        // 获取各集合名称
        int conptSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_TPContri, id }, Physical_Set, MapRelation_ConptSet);
        QString conptSetName = _modelData->getNameByPhyID(Physical_Set, conptSetID);

        int ridSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_TPContri, id }, Physical_Set, MapRelation_ResponseGridSet);
        QString ridSetName = _modelData->getNameByPhyID(Physical_Set, ridSetID);

        int conelSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_TPContri, id }, Physical_Set, MapRelation_ConelSet);
        QString conelSetName = _modelData->getNameByPhyID(Physical_Set, conelSetID);

        int conrelSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_TPContri, id }, Physical_Set, MapRelation_ConrelSet);
        QString conrelSetName = _modelData->getNameByPhyID(Physical_Set, conrelSetID);

        int convolSetID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_TPContri, id }, Physical_Set, MapRelation_ConvolSet);
        QString convolSetName = _modelData->getNameByPhyID(Physical_Set, convolSetID);

        // 第二行：前3个必选（空则写""），后2个可选
        // 规则：
        // - conrel 和 convol 都空 → 只写3个
        // - 只有 convol 空 → 写4个
        // - 只有 conrel 空 → 写5个，conrel位置留占位逗号
        // - 都不空 → 写5个
        if (conrelSetName.isEmpty() && convolSetName.isEmpty())
        {
            _out << QString("\"%1\",\"%2\",\"%3\"\r\n")
                .arg(conptSetName).arg(ridSetName).arg(conelSetName);
        }
        else if (!conrelSetName.isEmpty() && convolSetName.isEmpty())
        {
            _out << QString("\"%1\",\"%2\",\"%3\",\"%4\"\r\n")
                .arg(conptSetName).arg(ridSetName).arg(conelSetName).arg(conrelSetName);
        }
        else if (conrelSetName.isEmpty() && !convolSetName.isEmpty())
        {
            _out << QString("\"%1\",\"%2\",\"%3\", ,\"%4\"\r\n")
                .arg(conptSetName).arg(ridSetName).arg(conelSetName).arg(convolSetName);
        }
        else
        {
            _out << QString("\"%1\",\"%2\",\"%3\",\"%4\",\"%5\"\r\n")
                .arg(conptSetName).arg(ridSetName).arg(conelSetName).arg(conrelSetName).arg(convolSetName);
        }

        mGlobalSignals::getInstance()->outputMessageSig(0, QString("传递路径贡献量写文件完成"), true, true);
        return true;
    }


    void mOMxFile::writeERPData()
    {
        mERPData* erpData = _modelData->getGlobalParam()->getERPData();
        if (!erpData)
        {
            return;
        }

        _out << "*erp\r\n";
        _out << QString("%1, %2, %3, %4, %5\r\n")
            .arg(erpData->_erprho)
            .arg(erpData->_erpc)
            .arg(erpData->_rhocp)
            .arg(erpData->_erprlf)
            .arg(erpData->_erprefdb);
    }

    void mOMxFile::writeImplicitResponseSpectrum(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitResponseSpectrum* analysis = dynamic_cast<MechCaseData::mCaseImplicitResponseSpectrum*>(caseBase);
        if (!analysis)
        {
            return;
        }
        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = responsespectrum\r\n";

        if (analysis->_modalSolver == "自动多重子结构")
        {
            _out << "*eigra\r\n";
            _out << QString("%1, %2, %3, %4, %5\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
                .arg(analysis->_order).arg(analysis->_tolerance).arg(analysis->_ampffact);
        }
        else if (analysis->_modalSolver == "兰索斯")
        {
            _out << "*eigrl";
            if (analysis->_normalization == "广义质量")
            {
                _out << ", normalization = mass";
            }
            else if (analysis->_normalization == "最大位移")
            {
                _out << ", normalization = displacement";
            }
            _out << "\r\n";
            _out << QString("%1, %2, %3, %4\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
                .arg(analysis->_order).arg(analysis->_tolerance);
        }

        //写阻尼参数
        _out << "*damp\r\n";
        _out << QString("%1, %2, %3,").arg(analysis->_dampingG).arg(analysis->_dampingAlpha1)
            .arg(analysis->_dampingAlpha2);
        int curveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Function);
        QString curveName = _modelData->getNameByPhyID(Physical_Function, curveID);
        if (!curveName.isEmpty())
        {
            _out << QString("\"%1\"").arg(curveName);
        }
        _out << "\r\n";

        //写响应谱分析参数
        _out << "*rspec, ";
        if (analysis->_dirction == "ALG")
        {
            _out << "dcomb = ALG";
        }
        else if (analysis->_dirction == "SRSS")
        {
            _out << "dcomb = SRSS";
        }
        else
        {
            _out << "dcomb = ALG";
        }
        _out << ", ";
        if (analysis->_modal == "ABS")
        {
            _out << "mcomb = ABS";
        }
        else if (analysis->_modal == "SRSS")
        {
            _out << "mcomb = SRSS";
        }
        else if (analysis->_modal == "CQC")
        {
            _out << "mcomb = CQC";
        }
        else if (analysis->_modal == "NRL")
        {
            _out << "mcomb = NRL";
        }
        else if (analysis->_modal == "DSUM")
        {
            _out << "mcomb = DSUM";
        }
        else if (analysis->_modal == "ROSE")
        {
            _out << "mcomb = ROSE";
        }
        else {
            _out << "mcomb = ABS";
        }
        _out << ", ";
        _out << "close = ";
        _out << QString("%1").arg(analysis->_modalFrequencyClose);
        _out << " \r\n";

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> pulist = mapRelation.getMapRelation<QList<int>>(
            { Physical_Case, analysis->getCaseId() },
            Physical_LoadBoundary
        );
        // 确保索引不越界
        int maxRows = qMin(pulist.size(), analysis->_tableData.size());
        for (int i = 0; i < maxRows; i++)
        {

            auto& value = analysis->_tableData[i];
            QString text = getCVM->getModelData()
                ->getNameByPhyID(Physical_LoadBoundary, pulist[i]);
            _out << QString("\"%1\", %2, %3, %4, %5 \r\n")
                .arg(text)
                .arg(value._scales)
                .arg(value._loadx)
                .arg(value._loady)
                .arg(value._loadz);
        }
        int temp_caseID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Case);
        QString temp_caseName = _modelData->getNameByPhyID(Physical_Case, temp_caseID);
        if (!temp_caseName.isEmpty())
        {
            _out << "*preload\r\n";
            _out << QString("\"%1\"\r\n").arg(temp_caseName);
        }

        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        _out << "\r\n";
    }

    void mOMxFile::writeImplicitRandomVibration(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitRandomVibration* analysis = dynamic_cast<MechCaseData::mCaseImplicitRandomVibration*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis, type = random, method = 0";
        _out << "\r\n";

        QList<int> caseID1 = _modelData->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, analysis->getCaseId() }, Physical_Case, MapRelation_TableCase1);
        QList<int> caseID2 = _modelData->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, analysis->getCaseId() }, Physical_Case, MapRelation_TableCase2);
        QList<int> functionID = _modelData->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, analysis->getCaseId() }, Physical_Case, MapRelation_TableFunction);

        QSet<int> caseAllID = caseID1.toSet() + caseID2.toSet();
        caseAllID.remove(-1);
        if (caseAllID.size() > 0)
        {
            _out << "*loadsteps\r\n";
            for (auto it : caseAllID)
            {
                QString caseName = _modelData->getNameByPhyID(Physical_Case, it);
                _out << QString("\"%1\"\r\n").arg(caseName);
            }

            _out << "*randps\r\n";
            for (int i = 0; i < caseID1.size(); i++)
            {
                QString caseName1 = _modelData->getNameByPhyID(Physical_Case, caseID1[i]);
                QString caseName2 = _modelData->getNameByPhyID(Physical_Case, caseID2[i]);
                QString funcName = _modelData->getNameByPhyID(Physical_Function, functionID[i]);
                _out << QString("\"%1\", \"%2\", %3, %4, \"%5\"\r\n").arg(caseName1).arg(caseName2).arg(analysis->_tableXYData[i]._x).arg(analysis->_tableXYData[i]._y).arg(funcName);
            }
        }

        if (analysis->m_outputData.size() > 0)
        {
            _out << "*caseoutput\r\n";

            for (int i = 0; i < analysis->m_outputData.size(); i++)
            {
                MechCaseData::RandomCase_OutPutType& data = analysis->m_outputData[i];
                _out << QString("\"%1\", %2, %3, %4\r\n")
                    .arg(data._outputType)
                    .arg(data._randomType)
                    .arg(data._nodeNum)
                    .arg(data._dofNum);
            }
        }

        _out << "*randt\r\n";
        _out << QString("%1, %2, %3\r\n").arg(analysis->_time0).arg(analysis->_timeMax).arg(analysis->_nTime);


        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        writeCasePchOutPut(caseBase);
        _out << "\r\n";
    }

    void mOMxFile::writeImplicitTransient(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitTransient* analysis = dynamic_cast<MechCaseData::mCaseImplicitTransient*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis";
        if (analysis->_method == "模态法")
        {
            _out << ", type = lineartransient_modal";
            if (analysis->_isRemnantOpen)
            {
                _out << ", resvec = on";
            }
            _out << "\r\n";

            if (analysis->_modalSolver == "自动多重子结构")
            {
                _out << "*eigra\r\n";
                _out << QString("%1, %2, %3, %4, %5\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
                    .arg(analysis->_order).arg(analysis->_tolerance).arg(analysis->_ampffact);
            }
            else if (analysis->_modalSolver == "兰索斯")
            {
                _out << "*eigrl";
                if (analysis->_normalization == "广义质量")
                {
                    _out << ", normalization = mass";
                }
                else if (analysis->_normalization == "最大位移")
                {
                    _out << ", normalization = displacement";
                }
                _out << "\r\n";

                _out << QString("%1, %2, %3, %4\r\n").arg(analysis->_minFrequency).arg(analysis->_maxFrequency)
                    .arg(analysis->_order).arg(analysis->_tolerance);
            }
        }
        else
        {
            _out << ", type = lineartransient_direct";
            if (analysis->_solveMethod == "瞬态newmark法")
            {
                _out << ", method = newmark";
            }
            else if (analysis->_solveMethod == "瞬态hht法")
            {
                _out << ", method = hht";
            }
            else if (analysis->_solveMethod == "瞬态wilson法")
            {
                _out << ", method = wilson";
            }
            _out << "\r\n";
        }

        _out << "*timestep\r\n";
        _out << QString("%1, %2\r\n").arg(analysis->_timestepCount).arg(analysis->_timestepVal);


        _out << "*damp\r\n";
        _out << QString("%1, %2, %3, %4").arg(analysis->_dampingG).arg(analysis->_dampingOmg3)
            .arg(analysis->_dampingAlpha1).arg(analysis->_dampingAlpha2);

        if (analysis->_method == "模态法")
        {
            int curveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Function);
            QString curveName = _modelData->getNameByPhyID(Physical_Function, curveID);
            _out << ", " << "\"" << curveName << "\"";
        }
        _out << "\r\n";

        int temp_caseID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_Case);
        QString temp_caseName = _modelData->getNameByPhyID(Physical_Case, temp_caseID);
        if (!temp_caseName.isEmpty())
        {
            _out << "*preload\r\n";
            _out << QString("\"%1\"\r\n").arg(temp_caseName);
        }

        writeNsmData(caseBase);
        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        writeCaseOutPut(caseBase);
        writeCasePchOutPut(caseBase);
        writeHistoryOutput(caseBase);
        _out << "\r\n";
    }
    void mOMxFile::writeImplicitGeneric(MechCaseData::mCaseBase* caseBase)
    {
        MechCaseData::mCaseImplicitGeneric* analysis = dynamic_cast<MechCaseData::mCaseImplicitGeneric*>(caseBase);
        if (!analysis)
        {
            return;
        }

        _out << QString("*subcase, name = \"%1\"\r\n").arg(analysis->getCaseName());
        _out << "*analysis";
        _out << ", type = generic";
        _out << "\r\n";

        int paramID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Case, analysis->getCaseId() }, Physical_GenericParam);
        QString paramName = _modelData->getNameByPhyID(Physical_GenericParam, paramID);
        if (paramID > 0 && !paramName.isEmpty())
        {
            _out << "*cms\r\n";
            _out << QString("\"%1\"\r\n").arg(paramName);
        }

        writeModelFixData(caseBase);
        writeCasePredefinedField(caseBase);
        writeCaseLoadBCData(caseBase);
        _out << "\r\n";

        QList<int> partIDs = _modelData->getMapRelationManager().getMapRelation<QList<int>>({ Physical_GenericPart, _modelData->getModelDataID() }, Physical_Part);
        if (_modelData->getGlobalParam()->getGenericPartData() && !partIDs.isEmpty())
        {
            for (auto partID : partIDs)
            {
                _partNames.push_back(_modelData->getNameByPhyID(Physical_Part, partID));
            }
        }

    }
    bool mOMxFile::writeStructOptionData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        if (!_modelData->getStructOptionBase())
            return true;

        _out << "*optimization\r\n";
        writeStructOptionTopopara();
        writeStructOptionDesign();
        writeStructOptionObjective();
        writeStructOptionConstraint();


        mGlobalSignals::getInstance()->outputMessageSig("结构优化写文件完成");
        return true;
    }

    bool mOMxFile::writeMaterialToMatFile(QString filePath, std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        if (!mat)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "材料为空，无法写出");
            return false;
        }

        _file.setFileName(filePath);
        if (!_file.open(QIODevice::Truncate | QIODevice::WriteOnly))//以写的方式打开文件
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "文件打开失败");
            return false;
        }

        _out.setDevice(&_file);
        _out.setCodec("UTF-8");

        const auto matType = mat->getMaterialType();
        switch (matType)
        {
        case OtherMaterial_LIBRARY:
            writeLibraryMat(mat);
            break;
        case LinearElastic_Isotropic:
            writeIsolinearelastic(mat);
            break;
        case LinearElastic_Orthotropic:
            writeOrthotropy(mat);
            break;
        case LinearElastic_Anisotropic:
            writeAnisotropy(mat);
            break;
        case Plastic_Normal:
            writeElastoplasticMat(mat);
            break;
        case Plastic_Cyclic:
            writeChabocheMat(mat);
            break;
        case Plastic_Damage:
            writeDeformationplasticityMat(mat);
            break;
        case Plastic_Johnson:
            writeJohnsonCookMat(mat);
            break;
        case Damage_Shear:
            writeShearDamageMat(mat);
            break;
        case HyperElastic_NeoHookean:
        case HyperElastic_Ogden:
        case HyperElastic_ArrudaBoyce:
        case HyperElastic_MooneyRivlin:
        case HyperElastic_Yeoh:
        case HyperElastic_Polynomial:
            writeHyperelasticMat(mat);
            break;
        case Viscous_Viscoplastic:
            writeViscoPlasticityMat(mat);
            break;
        case Viscous_Viscoelastic:
            writeViscoelasticMat(mat);
            break;
        case AcousticMaterial_Base:
            writeAeroacousticsMat(mat);
            break;
        case Thermal_Base:
            writeHeatMat(mat);
            break;
        case OtherMaterial_Shim:
            writeShimMat(mat);
            break;
        case OtherMaterial_Foam:
            writeHyperfoamMat(mat);
            break;
        case OtherMaterial_Break:
            writeBreakMat(mat);
            break;
        default:
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("材料 %1 不支持写出").arg(mat->getMaterialName()));
            _file.close();
            return false;
        }
        }
        _out << "*end";
        _out.flush();
        _file.close();

        mGlobalSignals::getInstance()->outputMessageSig("材料写文件完成");
        return true;
    }

    bool mOMxFile::writeMaterialData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Material);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            shared_ptr<MechMaterialData::mMaterialBase> mat = _modelData->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(id);
            if (!mat)
                continue;
            auto matType = mat->getMaterialType();
            switch (matType)
            {
            case OtherMaterial_LIBRARY:
                writeLibraryMat(mat);
                break;
            case LinearElastic_Isotropic:
                writeIsolinearelastic(mat);
                break;
            case LinearElastic_Orthotropic:
                writeOrthotropy(mat);
                break;
            case LinearElastic_Anisotropic:
                writeAnisotropy(mat);
                break;
            case Plastic_Normal:
                writeElastoplasticMat(mat);
                break;
            case Plastic_Cyclic:
                writeChabocheMat(mat);
                break;
            case Plastic_Damage:
                writeDeformationplasticityMat(mat);
                break;
            case Plastic_Johnson:
                writeJohnsonCookMat(mat);
                break;
            case HyperElastic_NeoHookean:
            case HyperElastic_Ogden:
            case HyperElastic_ArrudaBoyce:
            case HyperElastic_MooneyRivlin:
            case HyperElastic_Yeoh:
                writeHyperelasticMat(mat);
                break;
            case HyperElastic_Polynomial:
                writeHyperelasticMat(mat);
                break;
            case Viscous_Viscoplastic:
                writeViscoPlasticityMat(mat);
                break;
            case Viscous_Viscoelastic:
                writeViscoelasticMat(mat);
                break;
            case AcousticMaterial_Base:
                writeAeroacousticsMat(mat);
                break;
            case Thermal_Base:
                writeHeatMat(mat);
                break;
                /*case MAT_IsotropicThermalMaterial:
                    writeHeatMat(mat);
                    break;*/
            case OtherMaterial_Shim:
                writeShimMat(mat);
                break;
            case OtherMaterial_Foam:
                writeHyperfoamMat(mat);
                break;
            case OtherMaterial_Break:
                writeBreakMat(mat);
                break;
            case Damage_Shear:
                writeShearDamageMat(mat);
                break;
            default:
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("材料 %1 不支持写出").arg(mat->getMaterialName()));
                break;
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("材料写文件完成");
        return true;
    }
    void mOMxFile::writeLibraryMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        _out << QString("*material, type = isolinearelastic, name = \"%1\"").arg(mat->getMaterialName());
        _out << "\r\n";
        const auto& matProp = mat->getAllLibProps();
        QHash<QString, QPair<QVector<Unit>, QVariant>> prop = matProp.value(MAT_LibStructural);
        if (prop.find("弹性模量") != prop.cend() && prop.value("弹性模量").second.isValid())
        {
            _out << prop.value("弹性模量").second.toDouble();
        }
        else
        {
            _out << "";
        }

        _out << ", ";

        if (prop.find("泊松比") != prop.cend() && prop.value("泊松比").second.isValid())
        {
            _out << prop.value("泊松比").second.toDouble();
        }
        else
        {
            _out << "";
        }

        _out << "\r\n";

        prop = matProp.value(MAT_LibCommon);
        if (prop.find("质量密度") != prop.cend() && prop.value("质量密度").second.isValid())
        {
            _out << "*density\r\n";
            _out << prop.value("质量密度").second.toDouble();
            _out << "\r\n";
        }
    }
    void mOMxFile::writeMaterialPropValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop, MxFunctions::MaterialPropType proType)
    {
        if (prop.value(proType).isValid())
        {
            //单位制换算：内部mm-t-N-s → 当前界面单位制（按材料属性量纲）
            _out << mUnitIoCodec::encode(mUnitIoCodec::unitOfMaterialProp(proType), prop.value(proType).toDouble());
        }
        else
        {
            _out << "";
        }
    }
    void mOMxFile::writeMaterialPropCurve(const QHash<MxFunctions::MaterialPropType, QVariant>& prop, MxFunctions::MaterialPropType proType)
    {
        if (prop.value(proType).isValid())
        {
            const MechMaterialData::Curves& curveData = MechMaterialData::mMaterialBase::transVatint2Curve(prop.value(proType));
            const QVector<QVector<double>>& curve = curveData.curves;
            int column = curve.size();
            //勾选曲线但未填数据(或读回产生的空曲线)时curve为空，curve[0]越界会导致写出中途崩溃产生0字节文件
            if (column <= 0 || curve[0].isEmpty())
            {
                _out << "\r\n";
                _out << "\r\n";
                return;
            }
            int row = curve[0].size();
            //单位制换算：曲线y列按材料属性量纲（x列为温度/频率，文件格式语义，不换算）
            const MxFunctions::Unit propUnit = mUnitIoCodec::unitOfMaterialProp(proType);

            if (curveData.correlation == MechMaterialData::CurveType::NoCorrelation)
            {
                _out << "\r\n";
                for (int i = 0; i < row; i++)
                {
                    _out << QString("%1").arg(curve[0][i]);
                    for (int j = 1; j < column; j++)
                    {
                        _out << QString(", %1").arg(curve[j][i]);
                    }
                    _out << "\r\n";
                }
            }
            else
            {

                if (curveData.correlation == MechMaterialData::CurveType::TemperatureCorrelation)
                {
                    _out << ", dependency = temp\r\n";
                }
                else if (curveData.correlation == MechMaterialData::CurveType::FrequencyDependence)
                {
                    _out << ", dependency = freq\r\n";
                }

                for (int i = 0; i < row; i++)
                {
                    _out << QString("%1").arg(mUnitIoCodec::encode(propUnit, curve[1][i]));
                    for (int j = 2; j < column; j++)
                    {
                        _out << QString(", %1").arg(mUnitIoCodec::encode(propUnit, curve[j][i]));
                    }
                    _out << QString(", %1").arg(curve[0][i]);
                    _out << "\r\n";
                }
            }
        }
        else
        {
            _out << "\r\n";
            _out << "\r\n";
        }
    }
    void mOMxFile::writeMapIntData(MxFunctions::PhysicalFieldType phyType, int phyID, MxFunctions::PhysicalFieldType type)
    {
        int id = _modelData->getMapRelationManager().getMapRelation<int>({ phyType, phyID }, type);
        if (id == -1)
            return;

        QString name = _modelData->getNameByPhyID(type, id);
        if (!name.isEmpty())
        {
            switch (type)
            {
            case MxFunctions::Physical_Material:
                _out << QString(", mat = \"%1\"").arg(name);
                break;
            case MxFunctions::Physical_Coordinate:
                _out << QString(", csys = \"%1\"").arg(name);
                break;
            case MxFunctions::Physical_Function:
                _out << QString(", function = \"%1\"").arg(name);
                break;
            case MxFunctions::Physical_OptimationDesign:
                _out << QString(", target = \"%1\"").arg(name);
                break;
            case MxFunctions::Physical_Case:
                _out << QString(", targetstep = \"%1\"").arg(name);
                break;
                //case MxFunctions::Physical_PropertyShape:
                //	break;
            default:
                break;
            }
        }
    }
    void mOMxFile::writeMapIntPropertyData(MxFunctions::PhysicalFieldType phyType, int phyID, MxFunctions::PhysicalFieldType type)
    {
        int id = _modelData->getMapRelationManager().getMapRelation<int>({ phyType, phyID }, type);
        if (id == -1)
            return;

        QString name = _modelData->getNameByPhyID(type, id);
        if (!name.isEmpty())
        {
            switch (type)
            {
            case MxFunctions::Physical_Material:
                _out << QString(", mat = \"%1\"").arg(name);
                break;
            case MxFunctions::Physical_Coordinate:
                _out << QString(", sys = \"%1\"").arg(name);
                break;
            default:
                break;
            }
        }
    }

    void mOMxFile::writeMapIntDataConv(MxFunctions::PhysicalFieldType phyType, int phyID, MxFunctions::PhysicalFieldType type, MxFunctions::MapRelationType relationType)
    {
        int id = _modelData->getMapRelationManager().getMapRelation<int>({ phyType, phyID }, type, relationType);
        if (id == -1)
            return;

        QString name = _modelData->getNameByPhyID(type, id);
        if (!name.isEmpty())
        {
            switch (type)
            {
            case MxFunctions::Physical_Function:
                _out << QString(", convfunction = \"%1\"").arg(name);
                break;
            default:
                break;
            }
        }
    }

    void mOMxFile::writeMapIntDataField(MxFunctions::PhysicalFieldType phyType, int phyID, MxFunctions::PhysicalFieldType type, MxFunctions::MapRelationType relationType)
    {
        int id = _modelData->getMapRelationManager().getMapRelation<int>({ phyType, phyID }, type, relationType);
        if (id == -1)
            return;

        QString name = _modelData->getNameByPhyID(type, id);
        if (!name.isEmpty())
        {
            switch (type)
            {
            case MxFunctions::Physical_Field:
                if (relationType == MxFunctions::MapRelation_convAnalysisField)
                    _out << QString(", convfield = \"%1\"").arg(name);
                else
                    _out << QString(", field = \"%1\"").arg(name);
                break;
            default:
                break;
            }
        }
    }
    void mOMxFile::writeQVariantValue(QVariant& data, MechCommon::QVariantType type)
    {
        if (!data.isValid())
        {
            _out << "";
        }

        switch (type)
        {
        case MechCommon::QVariantType::QVariant_Int:
            _out << data.toInt();
            break;
        case MechCommon::QVariantType::QVariant_Double:
            _out << QString("%1").arg(data.toDouble(), 0, 'g', 15);
            break;
        default:
            _out << "";
            break;
        }
    }
    void mOMxFile::getNodesByPickType(MxFunctions::PickDataType type, std::vector<RenderPickResultData> datas, std::set<int>& nodeIDs)
    {
        set<int> geo_nodes;
        switch (type)
        {
        case MxFunctions::PickDataType::GeoPoint:
        {
            std::for_each(datas.begin(), datas.end(), [&](RenderPickResultData data) {geo_nodes.insert(data.result_id); });
            geo_nodes = _modelData->getMeshData()->getNodeByGeoPoint(geo_nodes);
            nodeIDs.insert(geo_nodes.begin(), geo_nodes.end());
            break;
        }
        case MxFunctions::PickDataType::GeoEdge:
        {
            std::for_each(datas.begin(), datas.end(), [&](RenderPickResultData data) {geo_nodes.insert(data.result_id); });
            geo_nodes = _modelData->getMeshData()->getNodeByGeoEdge(geo_nodes);
            nodeIDs.insert(geo_nodes.begin(), geo_nodes.end());
            break;
        }
        case MxFunctions::PickDataType::GeoFace:
        {
            std::for_each(datas.begin(), datas.end(), [&](RenderPickResultData data) {geo_nodes.insert(data.result_id); });
            geo_nodes = _modelData->getMeshData()->getNodeByGeoFace(geo_nodes);
            nodeIDs.insert(geo_nodes.begin(), geo_nodes.end());
            break;
        }
        case MxFunctions::PickDataType::GeoSolid:
        {
            std::for_each(datas.begin(), datas.end(), [&](RenderPickResultData data) {geo_nodes.insert(data.result_id); });
            _modelData->getMeshData()->getNodeByGeoSolid(geo_nodes, nodeIDs);
            break;
        }
        case MxFunctions::PickDataType::Node:
        {
            std::for_each(datas.begin(), datas.end(), [&](RenderPickResultData data) {nodeIDs.insert(data.result_id); });
            break;
        }
        case MxFunctions::PickDataType::Part:
        {
            geo_nodes = _modelData->getMeshData()->getNodeIdsByPartId(datas);
            nodeIDs.insert(geo_nodes.begin(), geo_nodes.end());
            break;
        }
        default:
            break;
        }
    }
    void mOMxFile::writeConnectorSlaveNodeDof(set<int> dof)
    {
        _out << "*slavedofs" << "\r\n";
        QStringList dofList;
        for_each(dof.begin(), dof.end(), [&](int value) {
            dofList.append(MxFunctions::mNumberToString(value));
            });

        _out << dofList.join(", ") << "\r\n";
    }
    void mOMxFile::writeSectionShapeType(int phyID, const QString& xb, bool isPbeami)
    {
        MechSectionShapeData::mSectionShapeBase* shapeBase = _modelData->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(phyID);
        if (shapeBase == nullptr)
            return;

        if (isPbeami)
        {
            _out << "*pbeamsectioni" << "\r\n";
            _out << QString("%1, ").arg(xb);
        }
        else
        {
            if (shapeBase->getSectionShapeType() == Sec_None)  _out << "*beamsection";
            else  _out << "*pbeamsection";

            switch (shapeBase->getSectionShapeType())
            {
            case Sec_H:
                _out << ", type = h";
                break;
            case Sec_I:
                _out << ", type = i";
                break;
            case Sec_L:
                _out << ", type = l";
                break;
            case Sec_T:
                _out << ", type = t";
                break;
            case Sec_Box:
                _out << ", type = box";
                break;
            case Sec_Rec:
                _out << ", type = rectangular";
                break;
            case Sec_Cir:
                _out << ", type = circular";
                break;
            case Sec_Ann:
                _out << ", type = ring";
                break;
            case Sec_Channel:
                _out << ", type = chan";
                break;
            case Sec_Channel1:
                _out << ", type = chan1";
                break;
            case Sec_Channel2:
                _out << ", type = chan2";
                break;
            case Sec_I1:
                _out << ", type = i1";
                break;
            case Sec_T1:
                _out << ", type = t1";
                break;
            case Sec_T2:
                _out << ", type = t2";
                break;
            case Sec_Box1:
                _out << ", type = box1";
                break;
            case Sec_Cross:
                _out << ", type = cross";
                break;
            case Sec_Hat:
                _out << ", type = hat";
                break;
            case Sec_Z:
                _out << ", type = z";
                break;
            default:
                break;
            }

            _out << "\r\n";
        }

        if (shapeBase->getSectionShapeType() == Sec_None)
        {
            auto custom = dynamic_cast<mCustomSectionShape*>(shapeBase);
            QList<QString> para = custom->getCustomPara();
            for (int i = 0; i < 15; i++)
            {
                _out << QString("%1, ").arg(para[i]);
            }
            _out << QString("%1\r\n").arg(para[15]);

            int index = 0;
            for (int i = 16; i < para.size(); i++)
            {
                if (!para[i].isEmpty()) index = i + 1;
            }
            QString str_para = para.mid(16, index).join(", ");
            if (!str_para.isEmpty())
            {
                _out << str_para << "\r\n";
            }
        }
        else
        {
            QList<double> para = shapeBase->getParaNoType();
            double value = 0.0;
            for (int i = 0; i < para.size(); i++)
            {
                value = para[i];
                if ((shapeBase->getSectionShapeType() == Sec_I1 && i == 0)
                    || (shapeBase->getSectionShapeType() == Sec_H && i == 1)
                    || (shapeBase->getSectionShapeType() == Sec_Cross && i == 0)) {
                    value *= 2;
                }
                if (i == para.size() - 1)  _out << QString("%1").arg(value);
                else _out << QString("%1, ").arg(value);
            }

            if (!shapeBase->getNsm().isEmpty())
                _out << QString(", %1").arg(shapeBase->getNsm());

            _out << "\r\n";
        }

    }
    void mOMxFile::writeMultiRowData(MxFunctions::PhysicalFieldType type, QList<int> ids)
    {
        int num = 0;
        int size = 0;
        for (int i = 0; i < ids.size(); i++)
        {
            size++;
            QString name = _modelData->getNameByPhyID(type, ids[i]);
            if (name.isEmpty())
            {
                if (size == ids.size()) _out << "\r\n";
                continue;
            }

            num++;
            _out << QString("\"%1\"").arg(name);
            if (num % 16 == 0 || size == ids.size())
            {
                _out << "\r\n";
            }
            else
            {
                _out << ",";
            }
        }

    }
    void mOMxFile::writeCoorPointData(QString vec)
    {
        QStringList list = vec.split(",");
        vector<QString> str_list;
        str_list.resize(3);
        for (int i = 0; i < list.size(); i++)
        {
            str_list[i] = list[i];
        }
        _out << str_list[0] << ", " << str_list[1] << ", " << str_list[2];
    }
    void mOMxFile::writeIsolinearelastic(shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        _out << QString("*material, type = isolinearelastic, name = \"%1\"").arg(mat->getMaterialName());
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Isotropy);
        if (prop.contains(MatProp_Curve_Y_B))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_Y_B);
        }
        else
        {
            _out << "\r\n";

            writeMaterialPropValue(prop, MatProp_YoungsModulus);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_Botson);
            _out << "\r\n";

        }

        writeDensity(mat, matProp); //写出密度
        writeMaterialDamping(mat, matProp); //材料阻尼
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效

    }
    void mOMxFile::writeOrthotropy(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Orthotropy);

        _out << "*material";
        int d = prop.value(MatProp_Dimensionality).toInt();
        if (prop.value(MatProp_Dimensionality) == 0)  //二维
        {
            _out << ", type = ortholinearelastic2d";
        }
        else if (prop.value(MatProp_Dimensionality) == 1)//三维-工程常数
        {
            _out << ", type = ortholinearelastic3d, constants = on";
        }
        else if (prop.value(MatProp_Dimensionality) == 2)//三维正交
        {
            _out << ", type = ortholinearelastic3d, constants = off";
        }
        _out << QString(", name = \"%1\"").arg(mat->getMaterialName());
        if (prop.contains(MatProp_Curve_Orthotropy))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_Orthotropy);
        }
        else
        {
            _out << "\r\n";

            if (prop.value(MatProp_Dimensionality) == 0)  //二维
            {
                writeMaterialPropValue(prop, MatProp_Ex);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_Ey);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_NUxy);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_Gxy);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_Gxz);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_Gyz);
                _out << "\r\n";
            }
            else//三维
            {
                if (prop.contains(MatProp_Ex))
                {
                    writeMaterialPropValue(prop, MatProp_Ex);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Ey);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Ez);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_NUyz);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_NUxz);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_NUxy);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Gyz);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Gxz);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Gxy);
                }
                else if (prop.contains(MatProp_D1111))
                {
                    writeMaterialPropValue(prop, MatProp_D1111);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_D1122);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_D2222);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_D1133);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_D2233);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_D3333);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_D1212);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_D1313);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_D2323);
                }

                _out << "\r\n";
            }
        }

        writeDensity(mat, matProp); //写出密度
        writeMaterialDamping(mat, matProp); //材料阻尼
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeHashIndamage(mat, matProp); //Hashin失效
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
    }
    void mOMxFile::writeAnisotropy(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Anisotropy);

        _out << "*material";
        if (prop.value(MatProp_Dimensionality) == 0)  //二维
        {
            _out << ", type = anisotropiclinearelastic2d";
        }
        else//三维
        {
            _out << ", type = anisotropiclinearelastic3d";
        }
        _out << QString(", name = \"%1\"").arg(mat->getMaterialName());
        if (prop.contains(MatProp_Curve_Anisotropy_Gn))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_Anisotropy_Gn);
        }
        else
        {
            _out << "\r\n";

            if (prop.value(MatProp_Dimensionality) == 0)  //二维
            {
                writeMaterialPropValue(prop, MatProp_G11);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_G12);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_G13);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_G22);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_G23);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_G33);
                _out << "\r\n";
            }
            else//三维
            {
                writeMaterialPropValue(prop, MatProp_G11);
                for (MaterialPropType propType = MatProp_G12; propType <= MatProp_G66;)
                {
                    _out << ", ";
                    writeMaterialPropValue(prop, propType);
                    propType = MaterialPropType(propType + 1);
                }
                _out << "\r\n";
            }
        }

        writeDensity(mat, matProp); //写出密度
        writeMaterialDamping(mat, matProp); //材料阻尼
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效
    }
    void mOMxFile::writeHeatMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        _out << QString("*material, type = heat, name = \"%1\"\r\n").arg(mat->getMaterialName());

        _out << "*conductivity";

        if (matProp.contains(MAT_HeatConductivityCoefficient_UnCheck))
        {
            QHash<MxFunctions::MaterialPropType, QVariant> prop1 = matProp.value(MAT_HeatConductivityCoefficient_UnCheck);
            int type = prop1.value(MatProp_HeatConductivityCoefficient).toInt();
            int size = 0;
            switch (type)
            {
            case 0:
                _out << ", type = iso";
                size = 1;
                break;
            case 1:
                _out << ", type = ortho";
                size = 3;
                break;
            case 2:
                _out << ", type = aniso";
                size = 6;
                break;
            }

            if (prop1.contains(MatProp_Curve_HeatConductivityCoefficient))
            {
                writeMaterialPropCurve(prop1, MatProp_Curve_HeatConductivityCoefficient);
            }
            else
            {
                _out << "\r\n";
                writeMaterialPropValue(prop1, MatProp_HeatConductivityCoefficient_A1);
                for (int i = 1; i < size; i++)
                {
                    _out << ", ";
                    writeMaterialPropValue(prop1, MaterialPropType(MatProp_HeatConductivityCoefficient_A1 + i));
                }
                _out << "\r\n";
            }
        }

        if (matProp.contains(MAT_Heat))
        {
            QHash<MxFunctions::MaterialPropType, QVariant> prop2 = matProp.value(MAT_Heat);
            if (prop2.contains(MatProp_HeatCapacityDensity) || prop2.contains(MatProp_Curve_HeatCapacityDensity))
            {
                _out << "*specificheat";
                if (prop2.contains(MatProp_Curve_HeatCapacityDensity))
                {
                    writeMaterialPropCurve(prop2, MatProp_Curve_HeatCapacityDensity);
                }
                else
                {
                    _out << "\r\n";
                    writeMaterialPropValue(prop2, MatProp_HeatCapacityDensity);
                    _out << "\r\n";
                }
            }
        }

        if (matProp.contains(MAT_HeatGenerationRate))
        {
            _out << "*heatgeneration";
            QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_HeatGenerationRate);
            _out << "\r\n";
            writeMaterialPropValue(prop, MatProp_HeatGenerationRate);
            _out << "\r\n";
        }

        writeDensity(mat, matProp); //写出密度
    }
    void mOMxFile::writeChabocheMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        _out << QString("*material, type = chaboche, name = \"%1\"\r\n").arg(mat->getMaterialName());

        writeFlexMat(matProp);

        writePlasticityWithOrderMat(matProp);

        writeCyclichardening(matProp);

        writeDensity(mat, matProp); //写出密度
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效
    }
    
    void mOMxFile::writeJohnsonCookMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        _out << QString("*material, type = JohnsonCookPlastic, name = \"%1\"\r\n").arg(mat->getMaterialName());

        writeFlexMat(matProp);

        writePlasticityJohnsonMat(matProp);

        writeRateDependentMat(matProp);

        writeShearDamage(mat, matProp);

        writeDamageEvolution(mat, matProp);

        writeDensity(mat, matProp); //密度

        writeHeat(mat, matProp); //热传导、比热容

    }
    void mOMxFile::writeBreakMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Break);
        _out << QString("*material, type = phasematerial, name = \"%1\"").arg(mat->getMaterialName());

        writeBreakParamMat(matProp);
        writeBreakPhaseFieldMat(matProp);
    }

    void mOMxFile::writeBreakParamMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_BreakParam))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_BreakParam);

        _out << "\r\n";
        writeMaterialPropValue(prop, MatProp_YoungsModulus);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_Botson);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_Strech_Limit_Strenth);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_Break_Power);
        _out << "\r\n";
    }

    void mOMxFile::writeBreakPhaseFieldMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_PhaseFieldParam))
            return;
        QHash<MxFunctions::MaterialPropType, QVariant> prop
            = matProp.value(MAT_PhaseFieldParam);

        _out << "*phaseparameter";
        _out << "\r\n";
        int phaseFieldModel = prop.value(MatProp_PhiseFild_Model).toInt();
        int cohesionModel = prop.value(MatProp_Cohension_Model).toInt();
        _out << QString("%1, %2,").arg(phaseFieldModel).arg(cohesionModel);
        writeMaterialPropValue(prop, MatProp_Specifile_W);
        _out << "\r\n";
    }

    void mOMxFile::writeAeroacousticsMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Acoustic);
        _out << QString("*material, type = acoustic_mat, name = \"%1\"\r\n").arg(mat->getMaterialName());

        writeSoundVelocity(matProp);

        //写出密度
        if (matProp.contains(MAT_Density_UnCheck))
        {
            _out << "*density";
            QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Density_UnCheck);
            if (prop.contains(MatProp_Curve_Density))
            {
                writeMaterialPropCurve(prop, MatProp_Curve_Density);
            }
            else
            {
                _out << "\r\n";
                writeMaterialPropValue(prop, MatProp_Density);
                _out << "\r\n";
            }
        }

        writeBulkModulus(mat, matProp);

        writeAcousticMaterialDamping(mat, matProp);
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效

    }

    void mOMxFile::writeDeformationplasticityMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Deformation);
        _out << QString("*material, type = deformationplasticity, name = \"%1\"\r\n").arg(mat->getMaterialName());

        writeMaterialPropValue(prop, MatProp_YoungsModulus);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_Botson);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_YieldStress);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_StressExponent);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_YieldOffset);
        _out << "\r\n";


        writeDensity(mat, matProp); //写出密度
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效

    }


    void mOMxFile::writeViscoPlasticityMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Viscoplasticity);
        _out << QString("*material, type = viscoplastic, name = \"%1\"\r\n").arg(mat->getMaterialName());

        writeFlexMat(matProp);

        writeCreep(mat, matProp);//蠕变行为

        writeDensity(mat, matProp); //写出密度
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效

    }

    void mOMxFile::writeViscoelasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Viscoelasticity);
        _out << QString("*material, type = viscoelastic, name = \"%1\"\r\n").arg(mat->getMaterialName());
        _out << "*elastic";
        if (prop.value(MatProp_ModulusTimeScale).toInt() == 0)
        {
            _out << ", moduli = long_term";
        }
        else
        {
            _out << ", moduli = instantaneous";
        }
        _out << "\r\n";
        writeMaterialPropValue(prop, MatProp_YoungsModulus);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_Botson);
        _out << "\r\n";

        _out << "*viscoelastic";
        _out << ", time=prony";

        writeMaterialPropCurve(prop, MatProp_Curve_TimeDomain);

        int trsType = prop.value(MatProp_TRSType).toInt();
        if (trsType == 1)
        {
            _out << "*trs, definition = wlf\r\n";
            writeMaterialPropValue(prop, MatProp_Theta0);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_C1);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_C2);
            _out << "\r\n";
        }
        else if (trsType == 2)
        {
            _out << "*trs, definition = arrhenius\r\n";
            writeMaterialPropValue(prop, MatProp_Theta0);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_E0);
            _out << "\r\n";
        }

        writeDensity(mat, matProp); //写出密度
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效

    }

    void mOMxFile::writeElastoplasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        //QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(mat->getMaterialType());
        _out << QString("*material, type = elastoplastic, name = \"%1\"").arg(mat->getMaterialName());
        _out << "\r\n";

        writeFlexMat(matProp);
        writePlasticityComboMat(matProp);
        writeCyclicHardeningWithTable(matProp);

        //writeCyclichardening(matProp); //循环硬化

        writeDensity(mat, matProp); //写出密度
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效
        writeCreep(mat, matProp); //蠕变
    }

    void mOMxFile::writeShearDamageMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        _out << QString("*material, type = sheardamage, name = \"%1\"").arg(mat->getMaterialName());
        _out << "\r\n";
        writeShearDamage(mat, matProp);
        writeDamageEvolution(mat, matProp);
        
    }

    void mOMxFile::writeHyperelasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop;

        if (mat->getMaterialType() == HyperElastic_NeoHookean)
        {
            prop = matProp[MAT_Neohookean2];
            _out << QString("*material, type = neohookean2, name = \"%1\"").arg(mat->getMaterialName());
            _out << "\r\n";
            int index = prop[MatProp_ParaDataType].toInt();
            if (index == 0)
            {
                writeMaterialPropValue(prop, MatProp_C10);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_D1);
                _out << "\r\n";
            }
            else
            {
                if (prop.contains(MatProp_Curve_Testdatainput_Press))
                {
                    _out << "*testdatainput, type = uniaxial";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_Press);

                }
                if (prop.contains(MatProp_Curve_Testdatainput_V))
                {
                    _out << "*testdatainput, type = volumetric";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_V);

                }
            }

        }
        if (mat->getMaterialType() == HyperElastic_MooneyRivlin)
        {
            prop = matProp[MAT_MooneyRivlin];
            _out << QString("*material, type = mooneyrivlin, name = \"%1\"").arg(mat->getMaterialName());
            _out << "\r\n";
            int index = prop[MatProp_ParaDataType].toInt();
            if (index == 0)
            {

                writeMaterialPropValue(prop, MatProp_C10);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_C01);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_D1);
                _out << "\r\n";
            }
            else
            {
                /*if (prop.contains(MatProp_TestDataType))
                {
                    int index1 = prop[MatProp_TestDataType].toInt();
                    if (index1 == 0)
                    {
                        _out << "*testdatainput, type = uniaxial";
                    }
                    else
                    {
                        _out << "*testdatainput, type = volumetric";
                    }
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput);
                }*/

                if (prop.contains(MatProp_Curve_Testdatainput_Press))
                {
                    _out << "*testdatainput, type = uniaxial";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_Press);
                }

                if (prop.contains(MatProp_Curve_Testdatainput_V))
                {
                    _out << "*testdatainput, type = volumetric";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_V);
                }

            }

        }
        if (mat->getMaterialType() == HyperElastic_ArrudaBoyce)
        {
            prop = matProp[MAT_Arrudaboyce];
            _out << QString("*material, type = arrudaboyce, name = \"%1\"").arg(mat->getMaterialName());
            _out << "\r\n";
            int index = prop[MatProp_ParaDataType].toInt();
            if (index == 0)
            {
                writeMaterialPropValue(prop, MatProp_MU);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_LambdaM);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_D);
                _out << "\r\n";
            }
            else
            {
                /*if (prop.contains(MatProp_TestDataType))
                {
                    int index1 = prop[MatProp_TestDataType].toInt();
                    if (index1 == 0)
                    {
                        _out << "*testdatainput, type = uniaxial";
                    }
                    else
                    {
                        _out << "*testdatainput, type = volumetric";
                    }
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput);
                }*/

                if (prop.contains(MatProp_Curve_Testdatainput_Press))
                {
                    _out << "*testdatainput, type = uniaxial";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_Press);
                }
                if (prop.contains(MatProp_Curve_Testdatainput_V))
                {
                    _out << "*testdatainput, type = volumetric";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_V);
                }

            }


        }
        //新增Yeoh
        if (mat->getMaterialType() == HyperElastic_Yeoh)
        {
            prop = matProp[MAT_Yeoh];
            _out << QString("*material, type = yeoh, name = \"%1\"").arg(mat->getMaterialName());
            _out << "\r\n";
            int index = prop[MatProp_ParaDataType].toInt();
            if (index == 0)
            {
                writeMaterialPropValue(prop, MatProp_C10);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_C20);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_C30);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_D1);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_D2);
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_D3);
                _out << "\r\n";


            }
            else
            {
                /*if (prop.contains(MatProp_TestDataType))
                {
                    int index1 = prop[MatProp_TestDataType].toInt();
                    if (index1 == 0)
                    {
                        _out << "*testdatainput, type = uniaxial";
                    }
                    else
                    {
                        _out << "*testdatainput, type = volumetric";
                    }
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput);
                }*/
                if (prop.contains(MatProp_Curve_Testdatainput_Press))
                {
                    _out << "*testdatainput, type = uniaxial";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_Press);
                }
                if (prop.contains(MatProp_Curve_Testdatainput_V))
                {
                    _out << "*testdatainput, type = volumetric";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_V);
                }

            }


        }
        if (mat->getMaterialType() == HyperElastic_Ogden)
        {
            prop = matProp[MAT_Ogden];
            int index = prop[MatProp_ParaDataType].toInt();
            int step = prop[MatProp_Order].toInt();
            _out << QString("*material, type = ogden, name = \"%1\", ").arg(mat->getMaterialName());
            if (index == 0)
            {

                if (prop.contains(MatProp_Order))
                {

                    if (step == 1)
                    {
                        _out << "N = 1";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_Mu1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                    }if (step == 2)
                    {
                        _out << "N = 2";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_Mu1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                    }
                    if (step == 3)
                    {
                        _out << "N = 3";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_Mu1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D3);
                    }
                    if (step == 4)
                    {
                        _out << "N = 4";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_Mu1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D4);
                    }
                    if (step == 5)
                    {
                        _out << "N = 5";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_Mu1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu5);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha5);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D5);
                    }
                    if (step == 6)
                    {
                        _out << "N = 6";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_Mu1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu5);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha5);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Mu6);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_Alpha6);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D5);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D6);
                    }
                    _out << "\r\n";

                }
            }
            else
            {
                /*if (prop.contains(MatProp_TestDataType))
                {
                    _out << "N = " << step;
                    _out << "\r\n";
                    int index1 = prop[MatProp_TestDataType].toInt();
                    if (index1 == 0)
                    {
                        _out << "*testdatainput, type = uniaxial";
                    }
                    else
                    {
                        _out << "*testdatainput, type = volumetric";
                    }
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput);
                }*/

                _out << "N = " << step;
                _out << "\r\n";
                if (prop.contains(MatProp_Curve_Testdatainput_Press))
                {
                    _out << "*testdatainput, type = uniaxial";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_Press);
                }

                if (prop.contains(MatProp_Curve_Testdatainput_V))
                {
                    _out << "*testdatainput, type = volumetric";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_V);
                }


            }
        }

        if (mat->getMaterialType() == HyperElastic_Polynomial)
        {
            prop = matProp[MAT_Polynomial];
            int index = prop[MatProp_ParaDataType].toInt();
            int step = prop[MatProp_Order].toInt();
            _out << QString("*material, type = polynomial, name = \"%1\", ").arg(mat->getMaterialName());
            if (index == 0)
            {
                if (prop.contains(MatProp_Order))
                {
                    if (step == 1)
                    {
                        _out << "N = 1";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_C10);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C01);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                    }
                    if (step == 2)
                    {
                        _out << "N = 2";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_C10);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C01);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C20);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C11);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C02);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                    }
                    if (step == 3)
                    {
                        _out << "N = 3";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_C10);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C01);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C20);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C11);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C02);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C30);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C21);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C12);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C03);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D3);
                    }
                    if (step == 4)
                    {
                        _out << "N = 4";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_C10);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C01);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C20);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C11);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C02);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C30);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C21);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C12);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C03);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C40);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C31);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C22);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C13);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C04);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D4);
                    }
                    if (step == 5)
                    {
                        _out << "N = 5";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_C10);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C01);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C20);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C11);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C02);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C30);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C21);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C12);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C03);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C40);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C31);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C22);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C13);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C04);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C50);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C41);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C32);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C23);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C14);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C05);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D5);
                    }
                    if (step == 6)
                    {
                        _out << "N = 6";
                        _out << "\r\n";
                        writeMaterialPropValue(prop, MatProp_C10);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C01);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C20);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C11);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C02);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C30);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C21);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C12);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C03);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C40);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C31);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C22);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C13);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C04);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C50);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C41);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C32);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C23);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C14);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C05);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C60);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C51);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C42);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C33);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C24);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C15);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_C06);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D1);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D2);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D3);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D4);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D5);
                        _out << ", ";
                        writeMaterialPropValue(prop, MatProp_D6);
                    }
                    _out << "\r\n";
                }
            }
            else
            {
                /*if (prop.contains(MatProp_TestDataType))
                {
                    _out << "N = " << step;
                    _out << "\r\n";
                    int index1 = prop[MatProp_TestDataType].toInt();
                    if (index1 == 0)
                    {
                        _out << "*testdatainput, type = uniaxial";
                    }
                    else
                    {
                        _out << "*testdatainput, type = volumetric";
                    }
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput);
                }*/

                _out << "N = " << step;
                _out << "\r\n";
                if (prop.contains(MatProp_Curve_Testdatainput_Press))
                {
                    _out << "*testdatainput, type = uniaxial";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_Press);
                }
                if (prop.contains(MatProp_Curve_Testdatainput_V))
                {
                    _out << "*testdatainput, type = volumetric";
                    writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput_V);
                }
            }
        }

        writeDensity(mat, matProp); //写出密度
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效
    }

    void mOMxFile::writeShimMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        _out << QString("*material, type = GasketBehavior, name = \"%1\"").arg(mat->getMaterialName()) << "\r\n";
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp
            = mat->getAllProps();

        writeFilmBehaviorMat(matProp);
        writeThicknessBehaviorMat(matProp);
        writeCutBehaviorMat(matProp);

        writeDensity(mat, matProp); //写出密度
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效
    }

    void mOMxFile::writeHyperfoamMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp = mat->getAllProps();
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp[MAT_Hyperhorm];
        int index = prop[MatProp_HyperfoamType].toInt();
        int step = prop[MatProp_Order].toInt();
        _out << QString("*material, type = hyperfoam, name = \"%1\", ").arg(mat->getMaterialName());
        if (index == 0)
        {
            if (prop.contains(MatProp_Order))
            {

                if (step == 1)
                {
                    _out << "N = 1";
                    _out << "\r\n";
                    writeMaterialPropValue(prop, MatProp_Mu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu1);
                }if (step == 2)
                {
                    _out << "N = 2";
                    _out << "\r\n";
                    writeMaterialPropValue(prop, MatProp_Mu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu2);
                }
                if (step == 3)
                {
                    _out << "N = 3";
                    _out << "\r\n";
                    writeMaterialPropValue(prop, MatProp_Mu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu3);
                }
                if (step == 4)
                {
                    _out << "N = 4";
                    _out << "\r\n";
                    writeMaterialPropValue(prop, MatProp_Mu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu4);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha4);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu4);
                }
                if (step == 5)
                {
                    _out << "N = 5";
                    _out << "\r\n";
                    writeMaterialPropValue(prop, MatProp_Mu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu4);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha4);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu5);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha5);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu4);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu5);
                }
                if (step == 6)
                {
                    _out << "N = 6";
                    _out << "\r\n";
                    writeMaterialPropValue(prop, MatProp_Mu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu4);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha4);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu5);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha5);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Mu6);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Alpha6);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu1);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu2);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu3);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu4);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu5);
                    _out << ", ";
                    writeMaterialPropValue(prop, MatProp_Nu6);
                }
                _out << "\r\n";

            }
        }
        else
        {
            if (prop.contains(MatProp_TestDataType))
            {
                _out << "N = " << step;
                _out << "\r\n";
                int index1 = prop[MatProp_TestDataType].toInt();
                if (index1 == 0)
                {
                    _out << "*testdatainput, type = uniaxial";
                }
                else
                {
                    _out << "*testdatainput, type = volumetric";
                }
                writeMaterialPropCurve(prop, MatProp_Curve_Testdatainput);
            }
        }

        writeDensity(mat, matProp); //写出密度
        writeExpansion(mat, matProp); //热膨胀
        writeHeat(mat, matProp); //热导率、比热容
        writeFailureStress(mat, matProp); //失效应力
        writeFailureEffect(mat, matProp); //失效应变
        writeEnergydamage(mat, matProp); //Hashin渐进损伤
        writeHashIndamage(mat, matProp); //Hashin失效
    }

    void mOMxFile::writeFlexMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_Flex))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Flex);
        _out << "*elastic";
        if (prop.contains(MatProp_Curve_Y_B))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_Y_B);
        }
        else
        {
            _out << "\r\n";
            writeMaterialPropValue(prop, MatProp_YoungsModulus);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_Botson);
            _out << "\r\n";
        }
    }

    void mOMxFile::writePlasticityWithOrderMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_PlasticityWithOrder))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_PlasticityWithOrder);
        int num = prop.value(MatProp_Order).toInt();
        _out << QString("*plastic, number = %1\r\n").arg(num);
        writeMaterialPropValue(prop, MatProp_InitialYieldStressAtZeroPlasticity);
        for (int i = 0; i < num * 2; i++)
        {
            _out << ", ";
            writeMaterialPropValue(prop, MaterialPropType(MatProp_KinematicHardeningC1 + i));
        }
        _out << "\r\n";
    }

    void mOMxFile::writePlasticityJohnsonMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_PlasticityJohnson))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_PlasticityJohnson);
        _out << "*plastic\r\n";
        writeMaterialPropValue(prop, MatProp_A);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_B);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_N);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_M);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_MeltingTemp);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_TransitionTemp);
        _out << "\r\n";
    }

    void mOMxFile::writeRateDependentMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_RateDependent))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_RateDependent);
        int idx = prop.value(MatProp_RateType).toInt();
        if (idx == 0) {
            //JC塑性
            _out << QString("*ratedependent, type=JohnsonCook")<<"\r\n";
            writeMaterialPropValue(prop, MatProp_C);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_RefStrainRate);
            _out << "\r\n";
        }
       
    }

    void mOMxFile::writePlasticityComboMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_Plasticity))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Plasticity);
        _out << "*plastic";
        if (prop.contains(MatProp_Curve_PlasticityProperties_type))
        {
            int type = prop.value(MatProp_Curve_PlasticityProperties_type).toInt();
            if (type == 0)
            {
                _out << ", hardening = isotropic";
            }
            else if (type == 1)
            {
                _out << ", hardening = kinematic";
            }
            else if (type == 2)
            {
                _out << ", hardening = combined";
            }
        }
        writeMaterialPropCurve(prop, MatProp_Curve_PlasticityProperties);
    }

    void mOMxFile::writeThicknessBehaviorMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_ThicknessBehavior))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_ThicknessBehavior);
        if (prop.contains(MatProp_Curve_ShimLoading))
        {
            _out << QString("*GasketThicknessBehavior, type = %1")
                .arg(prop.value(MatProp_ShimThicknessType) == 0 ? "DAMAGE" : "ELASTIC_PLASTI");
            writeMaterialPropCurve(prop, MatProp_Curve_ShimLoading);
        }

        if (prop.contains(MatProp_Curve_ShimUnloading))
        {
            _out << QString("*GasketThicknessBehavior, direction=UNLOADING, type = %1")
                .arg(prop.value(MatProp_ShimThicknessType) == 0 ? "DAMAGE" : "ELASTIC_PLASTI");
            writeMaterialPropCurve(prop, MatProp_Curve_ShimUnloading);
        }
    }

    void mOMxFile::writeFilmBehaviorMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_FilmBehavior))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_FilmBehavior);
        _out << "*GasketElasticity, component = membrane\r\n";
        writeMaterialPropValue(prop, MatProp_YoungsModulus);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_Botson);
        _out << "\r\n";
    }

    void mOMxFile::writeCutBehaviorMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_CutBehavior))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_CutBehavior);
        _out << "*GasketElasticity, component = stresstiffness\r\n";
        writeMaterialPropValue(prop, MatProp_ShearStiffness);
        _out << "\r\n";
    }

    void mOMxFile::writeCyclicHardeningWithTable(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_CyclicHardeningWithTable))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_CyclicHardeningWithTable);
        if (prop.value(MatProp_Curve_CyclicHardeningWithTable).isValid())
        {
            _out << "*cyclichardening";
            writeMaterialPropCurve(prop, MatProp_Curve_CyclicHardeningWithTable);
        }
    }

    void mOMxFile::writeDensity(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_Density))
            return;

        _out << "*density";
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Density);
        if (prop.contains(MatProp_Curve_Density))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_Density);
        }
        else
        {
            _out << "\r\n";
            writeMaterialPropValue(prop, MatProp_Density);
            _out << "\r\n";
        }
    }

    void mOMxFile::writeHashIndamage(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_FailureHashin))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_FailureHashin);
        QString a = MxFunctions::mNumberToString(prop.value(MatProp_Alpha).toDouble(), 'f', 1);
        _out << QString("*hashindamage, alpha = %1\r\n").arg(a);
        if (prop.isEmpty())
        {
            return;
        }
        else
        {
            _out << MxFunctions::mNumberToString(prop.value(MatProp_Xt).toDouble(), 'f', 1) << ", "
                << MxFunctions::mNumberToString(prop.value(MatProp_Xc).toDouble(), 'f', 1) << ", "
                << MxFunctions::mNumberToString(prop.value(MatProp_Yt).toDouble(), 'f', 1) << ", "
                << MxFunctions::mNumberToString(prop.value(MatProp_Yc).toDouble(), 'f', 1) << ", "
                << MxFunctions::mNumberToString(prop.value(MatProp_Sl).toDouble(), 'f', 1) << ", "
                << MxFunctions::mNumberToString(prop.value(MatProp_St).toDouble(), 'f', 1) << "\r\n";
        }
    }

    void mOMxFile::writeEnergydamage(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_EnergyDamage))
            return;
        _out << "*energydamage\r\n";
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_EnergyDamage);
        if (prop.isEmpty())
        {
            return;
        }
        else
        {
            //不带小数点
           /* writeMaterialPropValue(prop, MatProp_Gft);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_Gfc);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_Gmt);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_Gmc);
            _out << "\r\n";*/
            //带小数点
            _out << MxFunctions::mNumberToString(prop.value(MatProp_Gft).toDouble(), 'f', 1) << ", "
                << MxFunctions::mNumberToString(prop.value(MatProp_Gfc).toDouble(), 'f', 1) << ", "
                << MxFunctions::mNumberToString(prop.value(MatProp_Gmt).toDouble(), 'f', 1) << ", "
                << MxFunctions::mNumberToString(prop.value(MatProp_Gmc).toDouble(), 'f', 1) << "\r\n";
        }

    }

    void  mOMxFile::writeShearDamage(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_ShearDamage))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_ShearDamage);
        //'f',1定点1位小数会造成导出/读回不一致(如0.25→0.2)，用默认全精度
        QString ks = MxFunctions::mNumberToString(prop.value(MatProp_KS).toDouble());
        _out << QString("*sheardamage, ks = %1").arg(ks);
        if (prop.contains(MatProp_Curve_Fs_Ssr_Sr))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_Fs_Ssr_Sr);
        }
        else
        {
            _out << "\r\n";
        }
    }

    void mOMxFile::writeDamageEvolution(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_DamageEvolution);
        if (!matProp.contains(MAT_DamageEvolution) || !prop.contains(MatProp_EvolutionType))
        {
            QHash<MxFunctions::MaterialPropType, QVariant> shearProp = matProp.value(MAT_ShearDamage);
            if (shearProp.contains(MatProp_EvolutionType))
            {
                prop = shearProp;
            }
        }
        if (!prop.contains(MatProp_EvolutionType))
            return;

        int type = prop.value(MatProp_EvolutionType).toInt();
        int mode = prop.value(MatProp_EvolutionMode).toInt();
        if (type == 0 && mode == 0) {
            _out << QString("*damageevolution, type = Displacement\r\n");
            writeMaterialPropValue(prop, MatProp_Fd);
            _out << "\r\n";
        }

    }

    void mOMxFile::writeSoundVelocity(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_SoundVelocity))
            return;

        _out << "*speed";
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_SoundVelocity);
        _out << "\r\n";
        writeMaterialPropValue(prop, MatProp_SoundVelocity);
        _out << "\r\n";

    }

    void mOMxFile::writeBulkModulus(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_BulkModulus))
            return;

        _out << "*bulkmodulus";
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_BulkModulus);
        _out << "\r\n";
        writeMaterialPropValue(prop, MatProp_BulkModulus);
        _out << "\r\n";

    }

    void mOMxFile::writeAcousticMaterialDamping(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_AcousticMaterialDamping))
            return;

        _out << "*fluiddamp";
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_AcousticMaterialDamping);
        _out << "\r\n";
        writeMaterialPropValue(prop, MatProp_AcousticMaterialDamping);
        _out << "\r\n";

    }

    void mOMxFile::writeMaterialDamping(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_MaterialDamping))
            return;

        _out << "*materialdamping";
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_MaterialDamping);
        _out << "\r\n";
        writeMaterialPropValue(prop, MatProp_MaterialDamping);
        _out << "\r\n";

    }

    void mOMxFile::writeExpansion(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_CoefficientOfThermalExpansion))
            return;

        _out << "*expansion";
        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_CoefficientOfThermalExpansion);
        int type = prop.value(MatProp_CoefficientOfThermalExpansion).toInt();
        int size = 0;
        switch (type)
        {
        case 0:
            _out << ", type = iso";
            size = 1;
            break;
            /*case 1:
                _out << ", type = ortho2d";
                size = 2;
                break;*/
        case 1:
            _out << ", type = ortho";
            size = 3;
            break;
            /*case 3:
                _out << ", type = aniso2d";
                size = 2;
                break;*/
        case 2:
            _out << ", type = aniso";
            size = 6;
            break;
        }

        if (prop.contains(MatProp_ReferenceTemperature))
        {
            _out << ", reftemp = ";
            writeMaterialPropValue(prop, MatProp_ReferenceTemperature);
        }
        else
        {
            _out << ", reftemp = 0.0";
        }

        if (prop.contains(MatProp_Curve_CoefficientOfThermalExpansion))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_CoefficientOfThermalExpansion);
        }
        else
        {
            _out << "\r\n";
            writeMaterialPropValue(prop, MatProp_CoefficientOfThermalExpansion_A1);
            for (int i = 1; i < size; i++)
            {
                _out << ", ";
                writeMaterialPropValue(prop, MaterialPropType(MatProp_CoefficientOfThermalExpansion_A1 + i));
            }
            if (type == 3)
            {
                _out << ", ";
                writeMaterialPropValue(prop, MatProp_CoefficientOfThermalExpansion_A12);
            }
            _out << "\r\n";
        }
    }
    void mOMxFile::writeHeat(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_HeatConductivityCoefficient) && !matProp.contains(MAT_Heat)
            && !matProp.contains(MAT_HeatGenerationRate)
            )
            return;

        bool outHead = false;
        if (matProp.contains(MAT_HeatConductivityCoefficient))
        {
            if (!outHead)
            {
                _out << "*heat\r\n";
                outHead = true;
            }

            _out << "*conductivity";
            QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_HeatConductivityCoefficient);
            int type = prop.value(MatProp_HeatConductivityCoefficient).toInt();
            int size = 0;
            switch (type)
            {
            case 0:
                _out << ", type = iso";
                size = 1;
                break;
            case 1:
                _out << ", type = ortho";
                size = 3;
                break;
            case 2:
                _out << ", type = aniso";
                size = 6;
                break;
            }

            if (prop.contains(MatProp_Curve_HeatConductivityCoefficient))
            {
                writeMaterialPropCurve(prop, MatProp_Curve_HeatConductivityCoefficient);
            }
            else
            {
                _out << "\r\n";
                writeMaterialPropValue(prop, MatProp_HeatConductivityCoefficient_A1);
                for (int i = 1; i < size; i++)
                {
                    _out << ", ";
                    writeMaterialPropValue(prop, MaterialPropType(MatProp_HeatConductivityCoefficient_A1 + i));
                }
                _out << "\r\n";
            }
        }

        if (matProp.contains(MAT_Heat))
        {
            if (!outHead)
            {
                _out << "*heat\r\n";
                outHead = true;
            }
            _out << "*specificheat";
            QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_Heat);
            if (prop.contains(MatProp_Curve_HeatCapacityDensity))
            {
                writeMaterialPropCurve(prop, MatProp_Curve_HeatCapacityDensity);
            }
            else
            {
                _out << "\r\n";
                writeMaterialPropValue(prop, MatProp_HeatCapacityDensity);
                _out << "\r\n";
            }
        }

        if (matProp.contains(MAT_HeatGenerationRate))
        {
            if (!outHead)
            {
                _out << "*heat\r\n";
            }
            _out << "*heatgeneration";
            QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_HeatGenerationRate);
            _out << "\r\n";
            writeMaterialPropValue(prop, MatProp_HeatProductionRate);
            _out << "\r\n";
        }
    }

    void mOMxFile::writeFailureEffect(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_YieldStrain))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_YieldStrain);
        _out << "*failstrain";

        int type = prop.value(MatProp_FailStrainType).toInt();
        switch (type)
        {
        case 0:
            _out << ", type = iso";
            break;
        case 1:
            _out << ", type = ortho2d";
            break;
        }

        if (prop.contains(MatProp_Curve_FailStrain))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_FailStrain);
        }
        else
        {
            if (type == 0)
            {
                _out << "\r\n";
                writeMaterialPropValue(prop, MatProp_St);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Sc);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Ss);
                _out << "\r\n";
            }
            else
            {
                _out << "\r\n";
                writeMaterialPropValue(prop, MatProp_Xt);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Xc);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Yt);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Yc);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_S);
                _out << "\r\n";
            }

        }
    }

    void mOMxFile::writeFailureStress(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_YieldStress))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_YieldStress);
        _out << "*failstress";

        int type = prop.value(MatProp_FailStressType).toInt();
        switch (type)
        {
        case 0:
            _out << ", type = iso";
            break;
        case 1:
            _out << ", type = ortho2d";
            break;
        }

        if (prop.contains(MatProp_Curve_FailStress))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_FailStress);
        }
        else
        {
            if (type == 0)
            {
                _out << "\r\n";
                writeMaterialPropValue(prop, MatProp_St);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Sc);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Ss);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_F12);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Biax);
                _out << "\r\n";
            }
            else
            {
                _out << "\r\n";
                writeMaterialPropValue(prop, MatProp_Xt);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Xc);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Yt);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Yc);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_S);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_F12);
                _out << ",";
                writeMaterialPropValue(prop, MatProp_Biax);
                _out << "\r\n";
            }
        }
    }

    void mOMxFile::writeCyclichardening(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_CyclicHardening))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_CyclicHardening);
        _out << "*cyclichardening\r\n";
        writeMaterialPropValue(prop, MatProp_YieldStress);
        _out << ", ";
        writeMaterialPropValue(prop, MatProp_HardeningRateControl);
        _out << "\r\n";
    }
    void mOMxFile::writeCreep(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp)
    {
        if (!matProp.contains(MAT_CreepBehavior) && !matProp.contains(MAT_CreepBehavior_Check))
            return;

        QHash<MxFunctions::MaterialPropType, QVariant> prop = matProp.value(MAT_CreepBehavior);
        if (mat->getMaterialType() == Plastic_Normal)
            prop = matProp.value(MAT_CreepBehavior_Check);
        _out << "*creep";

        int hardeningLaw = prop.value(MatProp_HardeningType).toInt();
        if (hardeningLaw == 2)
        {
            //Anand：15个模型参数，按顺序对应关键字S1/Q/R/A/Xi/m/A0/s^/n/a/S2/S3/A1/A2/A3/A4
            _out << ", law = anand\r\n";
            writeMaterialPropValue(prop, MatProp_Anand_DeformationResistance); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_ActivationEnergy); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_PowerLawMultiplier1); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_StressMultiplier); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_StressExponent); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_HardeningConstant); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_ResistancePowerLawMultiplier); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_ResistancePowerExponent); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_RateExponent); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_TempCoefficientS2); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_TempCoefficientS3); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_TempCoefficientA1); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_TempCoefficientA2); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_CreepCoefficientA3); _out << ", ";
            writeMaterialPropValue(prop, MatProp_Anand_CreepCoefficientA4);
            _out << "\r\n";
            return;
        }

        if (hardeningLaw == 0)
        {
            _out << ", law = time";
        }
        else
        {
            _out << ", law = strain";
        }

        if (prop.contains(MatProp_Curve_CreepBehavior))
        {
            writeMaterialPropCurve(prop, MatProp_Curve_CreepBehavior);
        }
        else
        {
            _out << "\r\n";
            writeMaterialPropValue(prop, MatProp_NortonCoefficient);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_StressExponent);
            _out << ", ";
            writeMaterialPropValue(prop, MatProp_TimeIndex);
            _out << "\r\n";
        }

    }
    bool mOMxFile::WritePropertyData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Property);
        if (ids.size() == 0)
        {
            return true;
        }


        for (auto& id : ids)
        {
            MechPropertyData::mPropertyBase* proBase = _modelData->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(id);
            if (!proBase)
                continue;

            switch (proBase->getPropertyType())
            {
            case SolidProperty:
                writeSolidProperty(proBase);
                break;
            case RodProperty:
                writeTorsionBarProperty(proBase);
                break;
            case ShearPlateProperty:
                writeShearPlateProperty(proBase);
                break;
            case TubeProperty:
                writeTubeProperty(proBase);
                break;
            case ShellProperty:
                writeShellProperty(proBase);
                break;
            case ResultantProperty:
                writeResultantProperty(proBase);
                break;
            case CrossProperty:
                writeCrossProperty(proBase);
                break;
            case Elas1Property:
                writeElas1Property(proBase);
                break;
            case Spring1Property:
            case Spring2Property:
            case SpringAProperty:
                writeSpringProperty(proBase);
                break;
            case ViscousDampProperty:
                writeViscousdampsection(proBase);
                break;
            case CbushProperty:
                writeBushsection(proBase);
                break;
            case PlyLaminateProperty:
                writePlyLaminate(proBase);
                break;
            case PlaneStrainProperty:
                writePlaneStrainProperty(proBase);
                break;
            case PlaneStressProperty:
                writePlaneStressProperty(proBase);
                break;
            case GasketProperty:
                writeGasketProperty(proBase);
                break;
            case GapuniProperty:
                writeGapunisection(proBase);
                break;
            case MacroEquivalentSection:
                writeMacroEquivalentsection(proBase);
                break;
            default:
                break;
            }

        }
        mGlobalSignals::getInstance()->outputMessageSig("截面属性写文件完成");
        return true;
    }
    void mOMxFile::writeSolidProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mSolidHomo* property = dynamic_cast<MechPropertyData::mSolidHomo*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
            _out << "\r\n";
            _out << "*solidsection" << "\r\n";
        }
    }
    void mOMxFile::writeTorsionBarProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mTorsionBar* property = dynamic_cast<MechPropertyData::mTorsionBar*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
            _out << "\r\n";
            _out << "*rodsection" << "\r\n";
            _out << QString("%1, %2, %3\r\n").arg(property->_J).arg(property->_A).arg(property->_nsm);
        }
    }
    void mOMxFile::writeShearPlateProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mShearPlate* property = dynamic_cast<MechPropertyData::mShearPlate*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
            _out << "\r\n";

            _out << "*shearplatesection" << "\r\n";
            _out << QString("%1, %2, %3\r\n").arg(property->_thickness).arg(property->_valF1).arg(property->_valF2);
        }
    }
    void mOMxFile::writeTubeProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mTube* property = dynamic_cast<MechPropertyData::mTube*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
            _out << "\r\n";
            _out << "*tubesection" << "\r\n";
            _out << QString("%1, %2, %3, %4\r\n").arg(property->_od).arg(property->_t).arg(property->_nsm).arg(property->_od2);
        }
    }
    void mOMxFile::writeShellProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mShellHomo* property = dynamic_cast<MechPropertyData::mShellHomo*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
            _out << "\r\n";
            _out << "*shellsection" << "\r\n";
            _out << QString("%1, %2\r\n").arg(property->_thickness).arg(property->_nsm);

            if (property->_referenceplane == "无")
            {
                return;
            }
            else
            {
                _out << "*referenceplane";
                if (property->_referenceplane == "偏置距离")
                {
                    _out << ", type = zoffs\r\n";
                    _out << property->_z0 << "\r\n";
                }
                else if (property->_referenceplane == "底部")
                {
                    _out << ", type = bottom\r\n";
                }
                else if (property->_referenceplane == "顶部")
                {
                    _out << ", type = top\r\n";
                }
            }

        }
    }
    void mOMxFile::writeResultantProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mConstantBeam* property = dynamic_cast<MechPropertyData::mConstantBeam*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
            _out << "\r\n";

            int shapeID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Property, property->getPropertyId() }, Physical_PropertyShape);
            writeSectionShapeType(shapeID);
        }
    }
    void mOMxFile::writeCrossProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mCrossBeam* property = dynamic_cast<MechPropertyData::mCrossBeam*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
            _out << "\r\n";

            QList<int> shapeIDs = _modelData->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Property, property->getPropertyId() }, Physical_PropertyShape);
            if (shapeIDs.size() == 0)
                return;

            writeSectionShapeType(shapeIDs[0], property->_paraList[0]);
            for (int i = 1; i < shapeIDs.size(); i++)
            {
                writeSectionShapeType(shapeIDs[i], property->_paraList[i], true);
            }
        }
    }
    void mOMxFile::writeElas1Property(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mSpring* property = dynamic_cast<MechPropertyData::mSpring*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            _out << "\r\n";

            _out << QString("*springsection, type = elas1\r\n");
            _out << QString("%1, %2, %3\r\n").arg(property->_K).arg(property->_GE).arg(property->_S);
        }

    }
    void mOMxFile::writeSpringProperty(MechPropertyData::mPropertyBase* proBase)
    {
        auto func_spring = [this](const QString& nonlinear, const QString& k,
            const QString& value, QList<QPair<double, double>>& _luCurve, bool isSpring2 = false)
            {
                if (nonlinear == "线性")
                {
                    _out << "\r\n";

                    if (!k.isEmpty())
                    {
                        _out << k;
                        if (isSpring2)
                        {
                            _out << ", " << k;
                        }
                        _out << "\r\n";
                    }

                    _out << value << "\r\n";
                }
                else
                {
                    _out << ", NONLINEAR = on";
                    _out << "\r\n";

                    if (!k.isEmpty())
                    {
                        _out << k;
                        if (isSpring2)
                        {
                            _out << ", " << k;
                        }
                        _out << "\r\n";
                    }

                    for (auto& it : _luCurve)
                    {
                        _out << QString("%1, %2\r\n").arg(it.first).arg(it.second);
                    }
                }
            };


        MechPropertyData::mSpring* property = dynamic_cast<MechPropertyData::mSpring*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());

            QString k = "";
            if (property->_k == "x") k = "1";
            else if (property->_k == "y") k = "2";
            else if (property->_k == "z") k = "3";
            else if (property->_k == "rx") k = "4";
            else if (property->_k == "ry") k = "5";
            else if (property->_k == "rz") k = "6";

            if (property->getPropertyType() == Spring1Property)
            {
                _out << "\r\n";
                _out << QString("*springsection, type = SPRING1");
                func_spring(property->_nonlinear, k, property->_value, property->_luCurve);
            }
            else if (property->getPropertyType() == Spring2Property)
            {
                writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
                _out << "\r\n";
                _out << QString("*springsection, type = SPRING2");
                func_spring(property->_nonlinear, k, property->_value, property->_luCurve, true);
            }
            else if (property->getPropertyType() == SpringAProperty)
            {
                k = "";
                _out << "\r\n";
                _out << QString("*springsection, type = SPRINGA");
                func_spring(property->_nonlinear, k, property->_value, property->_luCurve);
            }
        }
    }
    void mOMxFile::writeViscousdampsection(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mViscousdampsection* property = dynamic_cast<MechPropertyData::mViscousdampsection*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            _out << "\r\n";

            _out << QString("*viscousdampsection\r\n");
            _out << property->_GE << "\r\n";
        }
    }
    void mOMxFile::writeBushsection(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mCbush* property = dynamic_cast<MechPropertyData::mCbush*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            _out << "\r\n";

            _out << QString("*bushsection\r\n");
            vector<QString> hor = { "k" , "km", "b" , "ge" , "ang" , "m" };
            for (int i = 0; i < property->_isCheck.size(); i++)
            {
                if (property->_isCheck[i])
                {
                    _out << hor[i] << "\r\n";
                    _out << property->_value[i][0] << ", " << property->_value[i][1] << ", " << property->_value[i][2] << ", " << property->_value[i][3] << ", " <<
                        property->_value[i][4] << ", " << property->_value[i][5] << "\r\n";
                }
            }
        }
    }

    void mOMxFile::writePlaneStrainProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mShellHomo* property = dynamic_cast<MechPropertyData::mShellHomo*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            _out << "\r\n";

            _out << QString("*planestrainsection\r\n");
            _out << property->_thickness << "\r\n";
        }
    }

    void mOMxFile::writePlaneStressProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mShellHomo* property = dynamic_cast<MechPropertyData::mShellHomo*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            _out << "\r\n";

            _out << QString("*planestresssection\r\n");
            _out << property->_thickness << "\r\n";
        }
    }

    void mOMxFile::writeGasketProperty(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mGasket* property = dynamic_cast<MechPropertyData::mGasket*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Material);
            writeMapIntPropertyData(Physical_Property, property->getPropertyId(), Physical_Coordinate);
            _out << "\r\n";

            _out << QString("*gasketsection\r\n");
            if (property->_thicknessDef == "自动计算")
            {
                _out << QString(" ,%1\r\n").arg(property->_initialGap);
            }
            else
            {
                _out << QString("%1 ,%2\r\n").arg(property->_initialThickness).arg(property->_initialGap);
            }
        }
    }

    void mOMxFile::writeGapunisection(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mGapuni* property = dynamic_cast<MechPropertyData::mGapuni*>(proBase);
        if (property)
        {
            _out << QString("*section, type = structural, name = \"%1\"").arg(property->getPropertyName());
            _out << "\r\n";

            _out << QString("*gapunisection, type = gapuni\r\n");
            _out << property->_gapDistance;
            _out << ", ";

            writeCoorPointData(property->_direction);

            _out << ", ";
            _out << property->_area;
            _out << "\r\n";
        }
    }

    void mOMxFile::writeMacroEquivalentsection(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mMacroEquivalent* property = dynamic_cast<MechPropertyData::mMacroEquivalent*>(proBase);
        if (property)
        {
            _out << QString("*section, type = multiscale, name = \"%1\"").arg(property->getPropertyName());
            _out << "\r\n";

            QFileInfo fi(_file);
            // 拿到不含文件名的目录路径
            QString dirPath = fi.path();

            qDebug() << dirPath; // 输出："D:/data/model"
            _out << QString("*fiberreinforce, rve = \"%1/%2.mf\"").arg(dirPath).arg(property->_frbName);

            _out << "\r\n";

            MechIOFile::mIOFileManager::getInstance()->slot_exportMfFile(dirPath, property->_frbName);
        }
    }

    QString mOMxFile::getPlyFinalSysName(MechPropertyData::mPlyLaminate* plyData)
    {
        if (!plyData) return "";

        // 1. 获取铺层关联的坐标系ID
        int sysId = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Property, plyData->getPropertyId() }, Physical_Coordinate);
        if (sysId == -1) return "";

        // 2. 获取坐标系实体
        auto coorMgr = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate);
        MechCoordinateData::mCoordinateBase* coor = coorMgr.getCoordinateData(sysId);
        if (!coor) return "";
        QString originSysName = coor->getCoordinateName();

        // 3. 判断该坐标系是否需要拼接铺层名
        auto refMap = getCVM->getModelData()->getMapRelationManager().getCoordinateMap(sysId);
        auto& propMgr = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property);
        auto plyNames = propMgr.getAllPropertyNamesByTypes(PlyLaminateProperty);

        std::set<int> plyIdSet;
        for (const auto& name : plyNames)
        {
            plyIdSet.insert(propMgr.getIDByPropertyName(name));
        }

        std::vector<int> plyRefIds;
        bool hasOtherRef = false;
        for (const auto& pair : refMap)
        {
            if (pair.first == MxFunctions::Physical_Property)
            {
                for (int id : pair.second)
                {
                    if (plyIdSet.count(id))
                        plyRefIds.push_back(id);
                }
            }
            else if (!pair.second.empty())
            {
                hasOtherRef = true;
            }
        }

        // 判定是否为副本坐标系
        bool isCopySys = hasOtherRef || (plyRefIds.size() > 1);
        if (isCopySys)
        {
            return QString("%1_%2").arg(plyData->getPropertyName()).arg(originSysName);
        }
        return originSysName;
    }

    void mOMxFile::writePlyLaminate(MechPropertyData::mPropertyBase* proBase)
    {
        MechPropertyData::mPlyLaminate* property = dynamic_cast<MechPropertyData::mPlyLaminate*>(proBase);
        if (property)
        {
            if (property->_type == 0)
            {
                writeShell(property);
            }
            else if (property->_type == 1)
            {
                writeSolidShell(property);
            }
            else if (property->_type == 2)
            {
                writeSolid(property);
            }
        }
    }
    void mOMxFile::writeShell(MechPropertyData::mPlyLaminate* prodata)
    {
        if (!prodata) return;

        //新增：获取坐标系信息（用于输出 sys = "xxx"）
        int sysId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
            { Physical_Property, prodata->getPropertyId() }, Physical_Coordinate);
        QString sysName;
        bool needWriteSys = (sysId != -1);

        if (needWriteSys)
        {
            MechCoordinateData::mCoordinateBase* sys = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(sysId);
            if (sys)
            {
                // 判断是否为“额外复制的坐标系”：需要拼接 铺层名_坐标系名
                auto refMap = getCVM->getModelData()->getMapRelationManager().getCoordinateMap(sysId);
                auto& propMgr = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property);
                auto propertyNames = propMgr.getAllPropertyNamesByTypes(PlyLaminateProperty);
                std::set<int> plyIdSet;
                for (auto& name : propertyNames)
                    plyIdSet.insert(propMgr.getIDByPropertyName(name));

                std::vector<int> plyRefIds;
                bool hasOtherRef = false;
                for (auto& p : refMap)
                {
                    if (p.first == MxFunctions::Physical_Property)
                    {
                        for (int id : p.second)
                            if (plyIdSet.count(id)) plyRefIds.push_back(id);
                    }
                    else if (!p.second.empty())
                        hasOtherRef = true;
                }

                bool isCopySys = hasOtherRef || (plyRefIds.size() > 1);
                if (isCopySys)
                    sysName = QString("%1_%2").arg(prodata->getPropertyName()).arg(sys->getCoordinateName());
                else
                    sysName = sys->getCoordinateName();
            }
        }

        // 获取厚度离散场名称
        QString thickFieldName;
        int thickFieldID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
            { Physical_Property, prodata->getPropertyId() }, Physical_Field, MapRelation_ThickField);
        if (thickFieldID >= 0)
        {
            auto& dfMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
            auto* dfData = dfMgr.getDiscreteFieldData(thickFieldID);
            if (dfData && dfData->getSpaceType() == MxFunctions::ThicknessSpace)
                thickFieldName = dfMgr.getFieldNameByID(thickFieldID);
        }

        //1.关键字（直接定义 或者 层合板定义）
        if (prodata->_plyDefType == 0)
        {
            //在这里加 sys
            _out << QString("*section, type = layered, name = \"%1\"").arg(prodata->getPropertyName());
            if (needWriteSys && !sysName.isEmpty())
                _out << QString(", sys = \"%1\"").arg(sysName);
            _out << "\r\n";

            _out << QString("*ply, type = define");
            if (!thickFieldName.isEmpty())
                _out << QString(", thickfield = \"%1\"").arg(thickFieldName);
            _out << "\r\n";

            QList<int> matIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Property,  prodata->getPropertyId() }, Physical_Material);
            int count = prodata->thicknessList.size();
            if (count <= 0 || matIDs.size() <= 0)
                return;

            for (int i = 0; i < count; ++i)
            {
                QString matName = "";
                if (i < matIDs.size() && matIDs[i] >= 0)
                {
                    matName = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMatNameByID(matIDs[i]);
                }
                _out << QString("\"%1\", %2, %3, %4").arg(matName).arg(prodata->angleList[i]).arg(prodata->thicknessList[i]).arg(prodata->intePointList[i]) << "\r\n";
            }
        }
        else if (prodata->_plyDefType == 1)
        {
            //在这里加 sys
            _out << QString("*section, type = layered, name = \"%1\"").arg(prodata->getPropertyName());
            if (needWriteSys && !sysName.isEmpty())
                _out << QString(", sys = \"%1\"").arg(sysName);
            _out << "\r\n";

            int id = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Property, prodata->getPropertyId() }, Physical_Laminate);
            QString name = getCVM->getModelData()->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getLaminateNameByID(id);
            _out << QString("*ply, type = laminate, laminate = \"%1\"").arg(name);
            if (!thickFieldName.isEmpty())
                _out << QString(", thickfield = \"%1\"").arg(thickFieldName);
            _out << "\r\n";
        }

        //2.材料方向（需放到坐标系写文件处）
        //3.属性
        if (prodata->_shellProperty != "无")
        {
            _out << QString("*laminationoption");
            _out << "\r\n";

            QString str = "";
            if (prodata->_shellProperty == "对称铺层")
                str = "sym";
            else if (prodata->_shellProperty == "弯曲行为")
                str = "bend";
            else if (prodata->_shellProperty == "膜行为")
                str = "mem";
            else if (prodata->_shellProperty == "等效单层")
                str = "smear";
            else if (prodata->_shellProperty == "对称弯曲")
                str = "sybend";
            else if (prodata->_shellProperty == "对称膜")
                str = "symem";
            else if (prodata->_shellProperty == "对称等效单层")
                str = "sysmear";

            _out << QString("%1").arg(str);
            _out << "\r\n";
        }

        //4.参考面位置
        if (prodata->_referenceSurfacePos != "无")
        {
            if (prodata->_referenceSurfacePos == "偏置距离")
            {
                _out << QString("*referenceplane, type = zoffs");
                _out << "\r\n";
                _out << prodata->_referenceSurfaceDis;
            }
            else if (prodata->_referenceSurfacePos == "偏置率")
            {
                _out << QString("*referenceplane, type = ratio");
                _out << "\r\n";
                _out << prodata->_referenceSurfaceDis;
            }
            else if (prodata->_referenceSurfacePos == "顶部")
            {
                _out << QString("*referenceplane, type = top");
            }
            else if (prodata->_referenceSurfacePos == "底部")
            {
                _out << QString("*referenceplane, type = bottom");
            }
            _out << "\r\n";
        }

        //5.失效
        if (prodata->_failure != "无")
        {
            _out << QString("*failtheory");
            _out << "\r\n";

            QString str = "";
            if (prodata->_failure == "最大应力")
                str = "max_stress";
            else if (prodata->_failure == "最大应变")
                str = "max_strain";
            else if (prodata->_failure == "Tsai-Hill")
                str = "tsai_hill";
            else if (prodata->_failure == "Tsai-Wu")
                str = "tsai_wu";
            else if (prodata->_failure == "Azzi-Tsai-Hill")
                str = "azzi_tasi_hill";
            else if (prodata->_failure == "Hoff")
                str = "hoff";
            else if (prodata->_failure == "Puck")
                str = "puck";
            else if (prodata->_failure == "Hashin")
                str = "hfail";

            _out << QString("%1").arg(str);
            _out << "\r\n";
        }

        //6.厚度（已合并到*ply行输出）
    }
    void mOMxFile::writeSolidShell(MechPropertyData::mPlyLaminate* prodata)
    {
        if (!prodata) return;

        //新增：坐标系 sys 逻辑
        int sysId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
            { Physical_Property, prodata->getPropertyId() }, Physical_Coordinate);
        QString sysName;
        bool needWriteSys = (sysId != -1);
        if (needWriteSys)
        {
            MechCoordinateData::mCoordinateBase* sys = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(sysId);
            if (sys)
            {
                auto refMap = getCVM->getModelData()->getMapRelationManager().getCoordinateMap(sysId);
                auto& propMgr = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property);
                auto propertyNames = propMgr.getAllPropertyNamesByTypes(PlyLaminateProperty);
                std::set<int> plyIdSet;
                for (auto& name : propertyNames)
                    plyIdSet.insert(propMgr.getIDByPropertyName(name));

                std::vector<int> plyRefIds;
                bool hasOtherRef = false;
                for (auto& p : refMap)
                {
                    if (p.first == MxFunctions::Physical_Property)
                    {
                        for (int id : p.second)
                            if (plyIdSet.count(id)) plyRefIds.push_back(id);
                    }
                    else if (!p.second.empty())
                        hasOtherRef = true;
                }

                bool isCopySys = hasOtherRef || (plyRefIds.size() > 1);
                if (isCopySys)
                    sysName = QString("%1_%2").arg(prodata->getPropertyName()).arg(sys->getCoordinateName());
                else
                    sysName = sys->getCoordinateName();
            }
        }

        // 获取角度离散场名称
        QString rotFieldName;
        int rotFieldID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
            { Physical_Property, prodata->getPropertyId() }, Physical_Field, MapRelation_RotationField);
        if (rotFieldID >= 0)
        {
            auto& dfMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
            auto* dfData = dfMgr.getDiscreteFieldData(rotFieldID);
            if (dfData && dfData->getSpaceType() == MxFunctions::AngelSpace)
                rotFieldName = dfMgr.getFieldNameByID(rotFieldID);
        }

        // 获取厚度离散场名称
        QString thickFieldName;
        int thickFieldID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
            { Physical_Property, prodata->getPropertyId() }, Physical_Field, MapRelation_ThickField);
        if (thickFieldID >= 0)
        {
            auto& dfMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
            auto* dfData = dfMgr.getDiscreteFieldData(thickFieldID);
            if (dfData && dfData->getSpaceType() == MxFunctions::ThicknessSpace)
                thickFieldName = dfMgr.getFieldNameByID(thickFieldID);
        }

        _out << QString("*section, type = layered, element = continuum-shell, name = \"%1\"").arg(prodata->getPropertyName());
        if (needWriteSys && !sysName.isEmpty())
            _out << QString(", sys = \"%1\"").arg(sysName);
        if (!rotFieldName.isEmpty())
            _out << QString(", rotfield = \"%1\"").arg(rotFieldName);
        _out << "\r\n";

        if (prodata->_plyDefType == 0)
        {
            _out << QString("*ply, type = define");
            _out << "\r\n";

            QList<int> matIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Property,  prodata->getPropertyId() }, Physical_Material);
            int count = prodata->thicknessList.size();
            if (count <= 0 || matIDs.size() <= 0)
                return;

            for (int i = 0; i < count; ++i)
            {
                QString matName = "";
                if (i < matIDs.size() && matIDs[i] >= 0)
                {
                    matName = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMatNameByID(matIDs[i]);
                }
                _out << QString("\"%1\", %2, %3, %4").arg(matName).arg(prodata->angleList[i]).arg(prodata->thicknessList[i]).arg(prodata->intePointList[i]) << "\r\n";
            }
        }
        else if (prodata->_plyDefType == 1)
        {
            int id = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Property, prodata->getPropertyId() }, Physical_Laminate);
            QString name = getCVM->getModelData()->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getLaminateNameByID(id);
            _out << QString("*ply, type = laminate, laminate = \"%1\"").arg(name);
            _out << "\r\n";
        }

        if (prodata->_solidShellProperty != "无")
        {
            _out << QString("*laminationoption");
            _out << "\r\n";
            _out << "sym";
            _out << "\r\n";
        }
    }
    void mOMxFile::writeSolid(MechPropertyData::mPlyLaminate* prodata)
    {
        if (!prodata) return;

        //新增：坐标系 sys 逻辑
        int sysId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
            { Physical_Property, prodata->getPropertyId() }, Physical_Coordinate);
        QString sysName;
        bool needWriteSys = (sysId != -1);
        if (needWriteSys)
        {
            MechCoordinateData::mCoordinateBase* sys = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(sysId);
            if (sys)
            {
                auto refMap = getCVM->getModelData()->getMapRelationManager().getCoordinateMap(sysId);
                auto& propMgr = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property);
                auto propertyNames = propMgr.getAllPropertyNamesByTypes(PlyLaminateProperty);
                std::set<int> plyIdSet;
                for (auto& name : propertyNames)
                    plyIdSet.insert(propMgr.getIDByPropertyName(name));

                std::vector<int> plyRefIds;
                bool hasOtherRef = false;
                for (auto& p : refMap)
                {
                    if (p.first == MxFunctions::Physical_Property)
                    {
                        for (int id : p.second)
                            if (plyIdSet.count(id)) plyRefIds.push_back(id);
                    }
                    else if (!p.second.empty())
                        hasOtherRef = true;
                }

                bool isCopySys = hasOtherRef || (plyRefIds.size() > 1);
                if (isCopySys)
                    sysName = QString("%1_%2").arg(prodata->getPropertyName()).arg(sys->getCoordinateName());
                else
                    sysName = sys->getCoordinateName();
            }
        }

        // 获取角度离散场名称
        QString rotFieldName;
        int rotFieldID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
            { Physical_Property, prodata->getPropertyId() }, Physical_Field, MapRelation_RotationField);
        if (rotFieldID >= 0)
        {
            auto& dfMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
            auto* dfData = dfMgr.getDiscreteFieldData(rotFieldID);
            if (dfData && dfData->getSpaceType() == MxFunctions::AngelSpace)
                rotFieldName = dfMgr.getFieldNameByID(rotFieldID);
        }

        // 获取厚度离散场名称
        QString thickFieldName;
        int thickFieldID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
            { Physical_Property, prodata->getPropertyId() }, Physical_Field, MapRelation_ThickField);
        if (thickFieldID >= 0)
        {
            auto& dfMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
            auto* dfData = dfMgr.getDiscreteFieldData(thickFieldID);
            if (dfData && dfData->getSpaceType() == MxFunctions::ThicknessSpace)
                thickFieldName = dfMgr.getFieldNameByID(thickFieldID);
        }

        _out << QString("*section, type = layered, element = solid, name = \"%1\", stackdirection = %2").arg(prodata->getPropertyName()).arg(prodata->eleMentDir);
        if (needWriteSys && !sysName.isEmpty())
            _out << QString(", sys = \"%1\"").arg(sysName);
        if (!rotFieldName.isEmpty())
            _out << QString(", rotfield = \"%1\"").arg(rotFieldName);
        _out << "\r\n";

        if (prodata->_plyDefType == 0)
        {
            _out << QString("*ply, type = define");
            _out << "\r\n";

            QList<int> matIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Property,  prodata->getPropertyId() }, Physical_Material);
            int count = prodata->thicknessList.size();
            if (count <= 0 || matIDs.size() <= 0)
                return;

            for (int i = 0; i < count; ++i)
            {
                QString matName = "";
                if (i < matIDs.size() && matIDs[i] >= 0)
                {
                    matName = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMatNameByID(matIDs[i]);
                }
                _out << QString("\"%1\", %2, %3, %4").arg(matName).arg(prodata->angleList[i]).arg(prodata->thicknessList[i]).arg(prodata->intePointList[i]) << "\r\n";
            }
        }
        else if (prodata->_plyDefType == 1)
        {
            int id = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Property, prodata->getPropertyId() }, Physical_Laminate);
            QString name = getCVM->getModelData()->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getLaminateNameByID(id);
            _out << QString("*ply, type = laminate, laminate = \"%1\"").arg(name);
            _out << "\r\n";
        }

        if (prodata->_solidProperty != "无")
        {
            _out << QString("*laminationoption");
            _out << "\r\n";
            _out << "sym";
            _out << "\r\n";
        }
    }
    bool mOMxFile::writeSetData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetID(true);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            MechSetData::mSetBase* setBase = _modelData->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(id);
            if (!setBase)
                continue;

            switch (setBase->getSetType())
            {
            case Node:
                writeNodeSetData(setBase);
                break;
            case Element:
                writeElementSetData(setBase);
                break;
            case ElementSurface:
                writeElementSurfaceData(setBase);
                break;
            case ElementEdge:
                writeElementEdgeData(setBase);
                break;
            case FrequencyPoint:
                writeFrequencyPointData(setBase);
                break;
            case DOFNode:
                writeDofNodeData(setBase);
                break;
            case Panel:
                writePanelData(setBase);
                break;
            case NodeSurface:
                writeNodeSurfaceData(setBase);
                break;
            case SurfaceAnalysis:
                writeSurfaceAnalysisData(setBase);
                break;
            default:
                break;
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("集合写文件完成");
        return true;
    }

    bool mOMxFile::writeDiscreteField()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getManager<MechDiscreteFieldData::mFieldManager>
            (Physical_Field).getAllFieldID(MxFunctions::FieldType::Discrete);
        if (ids.size() == 0) return true;

        for (auto& id : ids)
        {
            MechDiscreteFieldData::mDiscreteField* discreteData = _modelData
                ->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field)
                .getDiscreteFieldData(id);
            if (!discreteData) continue;

            auto locate = discreteData->getLocate();
            QString locationStr = "";
            if (locate == MxFunctions::PickDataType::Node)
                locationStr = "node";
            else if (locate == MxFunctions::PickDataType::Element)
                locationStr = "element";

            auto spaceType = discreteData->getSpaceType();

            if (locate == MxFunctions::PickDataType::Node
                && spaceType != MxFunctions::DiscreteSpace::ThicknessSpace)
                continue;

            if (spaceType == MxFunctions::DiscreteSpace::AngelSpace)
            {
                _out << QString("*distribution, name = \"%1\", type = angle,"
                    "location = %2").arg(discreteData->getDiscreteSpaceName()).arg(locationStr);
                _out << "\r\n";

                // 先写默认值，序号为0
                _out << "0, " << discreteData->getDefaultAngel() << "\r\n";

                std::map<int, QString> angelData;
                if (locationStr == "element")
                    angelData = discreteData->getElementAngelMap();

                int index = 1;
                for (const auto& it : angelData)
                {
                    _out << it.first << ", " << it.second << "\r\n";
                    ++index;
                }
            }
            else if (spaceType == MxFunctions::DiscreteSpace::ThicknessSpace)
            {
                _out << QString("*distribution, name = \"%1\", type = thick,"
                    "location = %2").arg(discreteData->getDiscreteSpaceName()).arg(locationStr);
                _out << "\r\n";
                std::map<int, QString> thickData;
                if (locationStr == "element")
                {
                    // 先写默认值，序号为0
                    _out << "0, " << discreteData->getDefaultThickness() << "\r\n";
                    thickData = discreteData->getElementThicknessMap();
                }
                else
                {
                    thickData = discreteData->getNodeThicknessMap();
                }
                int index = 1;
                for (const auto& it : thickData)
                {
                    _out << it.first << ", " << it.second << "\r\n";
                    ++index;
                }
            }
            else if (spaceType == MxFunctions::DiscreteSpace::CoorSpace)
            {
                auto coorType = discreteData->getCoorType();
                QString type = "";
                if (coorType == MxFunctions::CoordinateType::Cartesian)
                    type = "rectangular";
                else if (coorType == MxFunctions::CoordinateType::Cylindrical)
                    type = "cylindrical";
                else if (coorType == MxFunctions::CoordinateType::Spherical)
                    type = "spherical";

                _out << QString("*distribution, name = \"%1\", type = %2,"
                    "location = %3").arg(discreteData->getDiscreteSpaceName())
                    .arg(type).arg(locationStr);
                _out << "\r\n";

                // 先写默认值（三个方向参考点），序号为0
                auto p1 = discreteData->getDirectPoint1();
                auto p2 = discreteData->getDirectPoint2();
                auto p3 = discreteData->getDirectPoint3();
                _out << QString("0, %1, %2, %3, %4, %5, %6, %7, %8, %9")
                    .arg(p1.x()).arg(p1.y()).arg(p1.z())
                    .arg(p2.x()).arg(p2.y()).arg(p2.z())
                    .arg(p3.x()).arg(p3.y()).arg(p3.z());
                _out << "\r\n";

                std::map<int, QStringList> coorData;
                if (locationStr == "element")
                    coorData = discreteData->getElementCoorMap();

                int index = 1;
                for (const auto& it : coorData)
                {
                    _out << it.first << ", ";
                    int i = 0;
                    for (auto str : it.second)
                    {
                        _out << str;
                        if (i != it.second.size() - 1)
                            _out << ", ";
                        i++;
                    }
                    _out << "\r\n";
                    ++index;
                }
            }
        }

        return true;
    }

    bool mOMxFile::writeAnalysisField()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getManager<MechDiscreteFieldData::mFieldManager>
            (Physical_Field).getAllFieldID(MxFunctions::FieldType::Analysis);
        if (ids.size() == 0) return true;

        for (auto& id : ids)
        {
            MechDiscreteFieldData::mAnalysisField* fieldData = _modelData
                ->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field)
                .getAnalysisFieldData(id);
            if (!fieldData) continue;

            auto locate = fieldData->getLocate();
            QString locationStr = "";
            if (locate == MxFunctions::PickDataType::Node)
                locationStr = "node";
            else if (locate == MxFunctions::PickDataType::Element)
                locationStr = "element";
            else if (locate == MxFunctions::PickDataType::ElementFace)
                locationStr = "elementface";

            QString methodStr = fieldData->getDefineMethod() == MechDiscreteFieldData::mAnalysisField::DefineMethod::Inline
                ? "inline" : "file";
            QString typeStr = fieldData->getDataType() == MechDiscreteFieldData::mAnalysisField::DataType::PointCloud
                ? "pointcloud" : "discrete";

            _out << QString("*analysisfield, name = \"%1\", location = %2, components = %3,"
                " method = %4, type = %5")
                .arg(fieldData->getAnalysisFieldName())
                .arg(locationStr)
                .arg(fieldData->getComponentCount())
                .arg(methodStr).arg(typeStr);

            //坐标系/目标集合走映射关系（面板 syncUiToField 中建立），-1 表示全局坐标系/整个模型
            auto& mapRel = _modelData->getMapRelationManager();
            int coorID = mapRel.getMapRelation<int>({ Physical_Field, id }, Physical_Coordinate);
            if (coorID > 0)
            {
                QString coorName = _modelData->getNameByPhyID(Physical_Coordinate, coorID);
                if (!coorName.isEmpty())
                    _out << QString(", csys = \"%1\"").arg(coorName);
            }
            int setID = mapRel.getMapRelation<int>({ Physical_Field, id }, Physical_Set);
            if (setID > 0)
            {
                QString setName = _modelData->getNameByPhyID(Physical_Set, setID);
                if (!setName.isEmpty())
                    _out << QString(", target = \"%1\"").arg(setName);
            }

            //文本文档定义参数
            if (fieldData->getDefineMethod() == MechDiscreteFieldData::mAnalysisField::DefineMethod::TextFile)
            {
                _out << QString(", file = \"%1\", startrow = %2, endrow = %3, startcol = %4")
                    .arg(fieldData->getTextFilePath())
                    .arg(fieldData->getTextStartRow())
                    .arg(fieldData->getTextEndRow())
                    .arg(fieldData->getTextStartColumn());
            }

            //映射空间定义（仅点云）
            if (fieldData->getDataType() == MechDiscreteFieldData::mAnalysisField::DataType::PointCloud)
            {
                QString scaleStr = "none";
                if (fieldData->getScaleMode() == MechDiscreteFieldData::mAnalysisField::ScaleMode::Uniform)
                    scaleStr = "uniform";
                else if (fieldData->getScaleMode() == MechDiscreteFieldData::mAnalysisField::ScaleMode::NonUniform)
                    scaleStr = "nonuniform";
                _out << QString(", scalemode = %1").arg(scaleStr);
                if (fieldData->getScaleMode() == MechDiscreteFieldData::mAnalysisField::ScaleMode::Uniform)
                    _out << QString(", scale = %1").arg(fieldData->getUniformScale());
                else if (fieldData->getScaleMode() == MechDiscreteFieldData::mAnalysisField::ScaleMode::NonUniform)
                    _out << QString(", scalex = %1, scaley = %2, scalez = %3")
                    .arg(fieldData->getScaleX()).arg(fieldData->getScaleY()).arg(fieldData->getScaleZ());
                if (!fieldData->getDefaultValue().isEmpty())
                    _out << QString(", default = \"%1\"").arg(fieldData->getDefaultValue());

                //映射控制（仅点云）
                QString tolTypeStr = fieldData->getToleranceType() == MechDiscreteFieldData::mAnalysisField::ToleranceType::Relative
                    ? "relative" : "absolute";
                QString algStr = fieldData->getMappingAlgorithm() == MechDiscreteFieldData::mAnalysisField::MappingAlgorithm::Surface
                    ? "surface" : "volumetric";
                _out << QString(", tolerancetype = %1, ptol = %2, ntol = %3, nbrtol = %4, itol = %5, algorithm = %6")
                    .arg(tolTypeStr)
                    .arg(fieldData->getPositiveTolerance())
                    .arg(fieldData->getNegativeTolerance())
                    .arg(fieldData->getNeighborhoodTolerance())
                    .arg(fieldData->getInterpolationTolerance())
                    .arg(algStr);
            }
            _out << "\r\n";

            //数据行：点云写 x,y,z,分量...；节点/单元写 id,分量...；单元面写 elementid,faceid,分量...
            const int compCount = fieldData->getComponentCount();
            for (const auto& row : fieldData->getRows())
            {
                if (fieldData->getDataType() == MechDiscreteFieldData::mAnalysisField::DataType::PointCloud)
                    _out << row.x << ", " << row.y << ", " << row.z;
                else if (locate == MxFunctions::PickDataType::ElementFace)
                    _out << row.elementId << ", " << row.faceId;
                else
                    _out << row.id;
                for (int i = 0; i < compCount; ++i)
                    _out << ", " << (i < row.values.size() ? row.values.at(i) : QString());
                _out << "\r\n";
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("分析场写文件完成");
        return true;
    }

    bool mOMxFile::writeNonSturcturalQualityData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getManager<MechCaseData::mNonStructQualityManager>
            (Physical_NonStructuralQuality).getAllNonSturcturalQualityID();
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            MechCaseData::mNonStructuralQuality* mNonData = _modelData->getManager<MechCaseData::mNonStructQualityManager>
                (Physical_NonStructuralQuality)
                .getNonStructuralQualityData(id);
            if (!mNonData) continue;

            auto typeIndex = mNonData->getTypeIndex();
            auto propIndex = mNonData->getPropIndex();

            QString prefix;
            if (typeIndex == Non::Union)
                prefix = "*nsm";
            else if (typeIndex == Non::MultUnion)
                prefix = "*nsm1";
            else if (typeIndex == Non::ALL)
                prefix = "*nsml";
            else if (typeIndex == Non::MultAll)
                prefix = "*nsml1";

            QString typeStr;
            if (propIndex == Physical_Property)
                typeStr = "property";
            else if (propIndex == Physical_Element)
                typeStr = "element";
            else if (propIndex == Physical_Set)
                typeStr = "set";

            _out << QString("%1, type = %2, name = \"%3\"").arg(prefix).arg(typeStr).arg(mNonData->getNonStructuralQualityName());
            _out << "\r\n";

            if (typeIndex == Non::MultUnion || typeIndex == Non::MultAll)
            {
                QString unionValue = mNonData->getUnionValue();
                _out << unionValue;
                _out << "\r\n";
            }

            std::map<QString, QString> dataMap;
            if (propIndex == Physical_Property)
                dataMap = mNonData->getCrossPropertyData();
            else if (propIndex == Physical_Element)
                dataMap = mNonData->getElementData();
            else if (propIndex == Physical_Set)
                dataMap = mNonData->getElementSetData();

            QStringList elementLists;
            if (propIndex == Physical_Set)
            {
                elementLists = getCVM->getModelData()
                    ->getManager<MechSetData::mSetManager>(Physical_Set)
                    .getAllSetNamesByTypes(Element);
            }

            bool isMult = (typeIndex == Non::MultUnion || typeIndex == Non::MultAll);
            int itemsPerLine = isMult ? 16 : 4;

            std::vector<std::pair<QString, QString>> filteredData;
            for (const auto& pair : dataMap)
            {
                if (propIndex == Physical_Set && !elementLists.contains(pair.first))
                    continue;
                if (propIndex == Physical_Property)
                {
                    MechPropertyData::mPropertyBase* proBase = getCVM->getModelData()
                        ->getManager<MechPropertyData::mPropertyManager>(Physical_Property)
                        .getPropertyData(pair.first);
                    if (!proBase)
                        continue;
                }
                filteredData.push_back(pair);
            }

            int num = 0;
            int total = filteredData.size();
            for (const auto& pair : filteredData)
            {
                num++;
                if (isMult)
                {
                    if (propIndex == Physical_Element)
                        _out << QString("%1").arg(pair.first);
                    else
                        _out << QString("\"%1\"").arg(pair.first);
                }
                else
                {
                    if (propIndex == Physical_Element)
                        _out << QString("%1, %2").arg(pair.first).arg(pair.second);
                    else
                        _out << QString("\"%1\", %2").arg(pair.first).arg(pair.second);
                }

                if (num % itemsPerLine == 0 || num == total)
                    _out << "\r\n";
                else
                    _out << ", ";
            }
        }

        return true;
    }

    QString mOMxFile::writeGeoSet(int setID, MxFunctions::SetType type)
    {
        MechSetData::mSetBase* setBase = _modelData->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
        if (!setBase)
            return QString();

        QString setName = getRenameSet();

        vector<RenderPickResultData> data;
        switch (setBase->getSetType())
        {
        case GeoPoint:
            writeGeoPointData(setBase, type, setName);
            break;
        case GeoEdge:
            writeGeoEdgeData(setBase, type, setName);
            break;
        case GeoFace:
            writeGeoFaceData(setBase, type, setName);
            break;
        case GeoSolid:
            writeGeoSolidData(setBase, type, setName);
            break;
        case Mix:
            writeMixSetData(setBase, type, setName);
            break;
        case Part:
            writeGeoPartData(setBase, type, setName);
            break;
        default:
            if (setBase->getInternal())
                setBase->setSetName(setName);
            else
                return setBase->getSetName();
        }

        return setName;
    }
    void mOMxFile::writeGeoPointData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName)
    {
        MechSetData::mGeoSet* setData = dynamic_cast<MechSetData::mGeoSet*>(setBase);
        if ((type != Node && type != NodeSurface) || setData == nullptr)
            return;

        _out << QString("*set, type = %1, name = \"%2\"")
            .arg(type == NodeSurface ? QStringLiteral("nodesurface") : QStringLiteral("node"), newName);
        if (setData->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";

        vector<int> ids = setData->getGeoSetData();
        set<int> data = _modelData->getMeshData()->getNodeByGeoPoint(set<int>{ids.begin(), ids.end()});
        writeSetData(data);
    }
    void mOMxFile::writeGeoEdgeData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName)
    {
        MechSetData::mGeoSet* setData = dynamic_cast<MechSetData::mGeoSet*>(setBase);
        if (setData == nullptr)
            return;

        vector<int> ids = setData->getGeoSetData();
        if (type == Node)
        {
            _out << QString("*set, type = node, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            set<int> data = _modelData->getMeshData()->getNodeByGeoEdge(set<int>{ids.begin(), ids.end()});
            writeSetData(data);
        }
        else if (type == NodeSurface)
        {
            set<int> data = _modelData->getMeshData()->getNodeByGeoEdge(set<int>{ids.begin(), ids.end()});
            writeNodeSurfaceData(std::vector<RenderPickResultData>(data.begin(), data.end()), newName, setData->getInternal());
        }
        else if (type == Element)
        {
            _out << QString("*set, type = element, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            set<int> data = _modelData->getMeshData()->getElementByGeoEdge(set<int>{ids.begin(), ids.end()});
            writeSetData(data);
        }
        else if (type == ElementEdge)
        {
            _out << QString("*set, type = elementedge, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            vector<RenderPickResultData> edges;
            for (int geoEdgeId : ids)
            {
                vector<RenderPickResultData> data = _modelData->getMeshData()->getMeshEdgeByGeoEdge(geoEdgeId);
                edges.insert(edges.end(), data.begin(), data.end());
            }
            writeEdgeSetData(edges);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("几何集合 %1 暂不支持该类型转化!").arg(setBase->getSetName()));
        }
    }
    void mOMxFile::writeGeoFaceData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName)
    {
        MechSetData::mGeoSet* setData = dynamic_cast<MechSetData::mGeoSet*>(setBase);
        if (setData == nullptr)
            return;

        vector<int> ids = setData->getGeoSetData();
        if (type == Node)
        {
            _out << QString("*set, type = node, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            set<int> data = _modelData->getMeshData()->getNodeByGeoFace(set<int>{ids.begin(), ids.end()});
            writeSetData(data);
        }
        else if (type == NodeSurface)
        {
            set<int> data = _modelData->getMeshData()->getNodeByGeoFace(set<int>{ids.begin(), ids.end()});
            writeNodeSurfaceData(std::vector<RenderPickResultData>(data.begin(), data.end()), newName, setData->getInternal());
        }
        else if (type == Element)
        {
            _out << QString("*set, type = element, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            set<int> data = _modelData->getMeshData()->getElementByGeoFace(set<int>{ids.begin(), ids.end()});
            writeSetData(data);
        }
        else if (type == ElementSurface)
        {
            _out << QString("*set, type = elementsurface, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            vector<RenderPickResultData> twoEle;
            vector<RenderPickResultData> threeEle;
            _modelData->getMeshData()->getElementFaceByGeoFace(ids, twoEle, threeEle);
            if (threeEle.size() > 0)
            {
                writeSurfaceSetData(threeEle);
            }
            else if (twoEle.size() > 0)
            {
                writeSurfaceSetData(twoEle);
            }
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("几何集合 %1 暂不支持该类型转化!").arg(setBase->getSetName()));
        }
    }
    void mOMxFile::writeGeoSolidData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName)
    {
        MechSetData::mGeoSet* setData = dynamic_cast<MechSetData::mGeoSet*>(setBase);
        if (setData == nullptr)
            return;

        vector<int> ids = setData->getGeoSetData();
        if (type == Node)
        {
            _out << QString("*set, type = node, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            vector<RenderPickResultData> data = _modelData->getMeshData()->getNodeByGeoSolid(ids);
            writeSetData(data);
        }
        else if (type == NodeSurface)
        {
            vector<RenderPickResultData> data = _modelData->getMeshData()->getNodeByGeoSolid(ids);
            writeNodeSurfaceData(data, newName, setData->getInternal());
        }
        else if (type == Element)
        {
            _out << QString("*set, type = element, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            vector<RenderPickResultData>  data = _modelData->getMeshData()->getElementByGeoSolid(ids);
            writeSetData(data);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("几何集合 %1 暂不支持该类型转化!").arg(setBase->getSetName()));
        }

        return;
    }

    auto transformSetInt = [](const vector<RenderPickResultData>& datas) {
        set<int> ids;
        for (int i = 0; i < datas.size(); i++)
            ids.insert(datas[i].result_id);
        return ids;
        };

    auto transformVecInt = [](const vector<RenderPickResultData>& datas) {
        vector<int> ids;
        for (int i = 0; i < datas.size(); i++)
            ids.push_back(datas[i].result_id);
        return ids;
        };

    auto transformPartResult = [](const vector<int>& datas) {
        vector<RenderPickResultData> ids;
        for (int i = 0; i < datas.size(); i++)
        {
            RenderPickResultData data;
            data.part_id = datas[i];
            ids.push_back(data);
        }
        return ids;
        };

    auto transform = [](const vector<RenderPickResultData>& datas, set<int>& ids) {
        for (int i = 0; i < datas.size(); i++)
            ids.insert(datas[i].result_id);
        };

    void mOMxFile::writeMixSetData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName)
    {
        MechSetData::mMixSet* setData = dynamic_cast<MechSetData::mMixSet*>(setBase);
        if (setData == nullptr)
            return;

        const unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& ids = setData->getMixSetData();
        if (type == NodeSurface)
        {
            set<int> nodes;
            for (auto& it : ids)
            {
                switch (it.first)
                {
                case MxFunctions::PickDataType::Node:
                {
                    transform(it.second, nodes);
                    break;
                }
                case MxFunctions::PickDataType::GeoPoint:
                {
                    set<int> data = _modelData->getMeshData()->getNodeByGeoPoint(transformSetInt(it.second));
                    nodes.insert(data.begin(), data.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoEdge:
                {
                    set<int> data = _modelData->getMeshData()->getNodeByGeoEdge(transformSetInt(it.second));
                    nodes.insert(data.begin(), data.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoFace:
                {
                    set<int> data = _modelData->getMeshData()->getNodeByGeoFace(transformSetInt(it.second));
                    nodes.insert(data.begin(), data.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoSolid:
                {
                    vector<RenderPickResultData> data = _modelData->getMeshData()->getNodeByGeoSolid(transformVecInt(it.second));
                    transform(data, nodes);
                    break;
                }
                case MxFunctions::PickDataType::Part:
                {
                    nodes = _modelData->getMeshData()->getNodeIdsByPartId(it.second);
                    break;
                }
                default:
                    break;
                }
            }
            writeNodeSurfaceData(std::vector<RenderPickResultData>(nodes.begin(), nodes.end()), newName, setData->getInternal());
            return;
        }
        if (type == Node)
        {
            _out << QString("*set, type = node, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            set<int> nodes;
            for (auto& it : ids)
            {
                switch (it.first)
                {
                case MxFunctions::PickDataType::Node:
                {
                    transform(it.second, nodes);
                    break;
                }
                case MxFunctions::PickDataType::GeoPoint:
                {
                    set<int> data = _modelData->getMeshData()->getNodeByGeoPoint(transformSetInt(it.second));
                    nodes.insert(data.begin(), data.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoEdge:
                {
                    set<int> data = _modelData->getMeshData()->getNodeByGeoEdge(transformSetInt(it.second));
                    nodes.insert(data.begin(), data.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoFace:
                {
                    set<int> data = _modelData->getMeshData()->getNodeByGeoFace(transformSetInt(it.second));
                    nodes.insert(data.begin(), data.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoSolid:
                {
                    vector<RenderPickResultData> data = _modelData->getMeshData()->getNodeByGeoSolid(transformVecInt(it.second));
                    transform(data, nodes);
                    break;
                }
                case MxFunctions::PickDataType::Part:
                {
                    nodes = _modelData->getMeshData()->getNodeIdsByPartId(it.second);
                    break;
                }
                default:
                    break;
                }
            }
            writeSetData(nodes);
        }
        else if (type == Element)
        {
            _out << QString("*set, type = element, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            set<int> elements;
            for (auto& it : ids)
            {
                switch (it.first)
                {
                case MxFunctions::PickDataType::Element:
                {
                    transform(it.second, elements);
                    break;
                }
                case MxFunctions::PickDataType::GeoEdge:
                {
                    set<int> data = _modelData->getMeshData()->getElementByGeoEdge(transformSetInt(it.second));
                    elements.insert(data.begin(), data.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoFace:
                {
                    set<int> data = _modelData->getMeshData()->getElementByGeoFace(transformSetInt(it.second));
                    elements.insert(data.begin(), data.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoSolid:
                {
                    vector<RenderPickResultData> data = _modelData->getMeshData()->getElementByGeoSolid(transformVecInt(it.second));
                    transform(data, elements);
                    break;
                }
                case MxFunctions::PickDataType::Part:
                {
                    vector<RenderPickResultData> data = _modelData->getMeshData()->getMeshIDsInPart(it.second);
                    transform(data, elements);
                    break;
                }
                default:
                    break;
                }
            }
            writeSetData(elements);
        }
        else if (type == ElementSurface)
        {
            _out << QString("*set, type = elementsurface, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            vector<RenderPickResultData> surfaces;
            for (auto& it : ids)
            {
                switch (it.first)
                {
                case MxFunctions::PickDataType::ElementFace:
                {
                    surfaces.insert(surfaces.end(), it.second.begin(), it.second.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoFace:
                {
                    vector<RenderPickResultData> twoEle;
                    vector<RenderPickResultData> threeEle;
                    _modelData->getMeshData()->getElementFaceByGeoFace(transformVecInt(it.second), twoEle, threeEle);
                    surfaces.insert(surfaces.end(), twoEle.begin(), twoEle.end());
                    surfaces.insert(surfaces.end(), threeEle.begin(), threeEle.end());
                    break;
                }
                default:
                    break;
                }
            }
            writeSurfaceSetData(surfaces);
        }
        else if (type == ElementEdge)
        {
            _out << QString("*set, type = elementedge, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            vector<RenderPickResultData> edges;
            for (auto& it : ids)
            {
                switch (it.first)
                {
                case MxFunctions::PickDataType::ElementEdge:
                {
                    edges.insert(edges.end(), it.second.begin(), it.second.end());
                    break;
                }
                case MxFunctions::PickDataType::GeoEdge:
                {
                    vector<RenderPickResultData> data;
                    for (auto& geoEdge : it.second)
                    {
                        data = _modelData->getMeshData()->getMeshEdgeByGeoEdge(geoEdge.result_id);
                        edges.insert(edges.end(), data.begin(), data.end());
                    }
                    break;
                }
                case MxFunctions::PickDataType::GeoFace:
                {
                    // 几何面 → 几何边 → 单元边
                    //【产品集成】旧栈 GeoDataModelSet 反查（getPartSetByGeoFaceID/getEdgeTagsOnFace）
                    // 退役，改 MeshEntity GN 面边镜像查询（对齐引擎范式总账）
                    auto* meshEntity = _modelData->getMeshData()->getMeshEntity();
                    if (meshEntity == nullptr)
                        break;
                    set<int> geoEdgeIds;
                    for (const auto& geoFace : it.second)
                    {
                        MXGeoFace* face = meshEntity->getGeoFaceById(geoFace.result_id);
                        if (face == nullptr)
                            continue;
                        for (MXGeoEdge* edge : face->getEdgesOfGeoFaces())
                        {
                            if (edge != nullptr)
                                geoEdgeIds.insert(edge->getGeoTag());
                        }
                    }
                    for (const int geoEdgeId : geoEdgeIds)
                    {
                        vector<RenderPickResultData> data = _modelData->getMeshData()->getMeshEdgeByGeoEdge(geoEdgeId);
                        edges.insert(edges.end(), data.begin(), data.end());
                    }
                    break;
                }
                case MxFunctions::PickDataType::GeoSolid:
                {
                    // 几何实体 → 几何面 → 几何边 → 单元边（【产品集成】同上切 GN 镜像）
                    auto* meshEntity = _modelData->getMeshData()->getMeshEntity();
                    if (meshEntity == nullptr)
                        break;
                    set<int> geoEdgeIds;
                    for (const auto& geoSolid : it.second)
                    {
                        MXGeoSolid* solid = meshEntity->getGeoSolidById(geoSolid.result_id);
                        if (solid == nullptr)
                            continue;
                        for (MXGeoFace* face : solid->getface())
                        {
                            if (face == nullptr)
                                continue;
                            for (MXGeoEdge* edge : face->getEdgesOfGeoFaces())
                            {
                                if (edge != nullptr)
                                    geoEdgeIds.insert(edge->getGeoTag());
                            }
                        }
                    }
                    for (const int geoEdgeId : geoEdgeIds)
                    {
                        vector<RenderPickResultData> data = _modelData->getMeshData()->getMeshEdgeByGeoEdge(geoEdgeId);
                        edges.insert(edges.end(), data.begin(), data.end());
                    }
                    break;
                }
                default:
                    break;
                }
            }
            writeEdgeSetData(edges);
            }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("几何集合 %1 暂不支持该类型转化!").arg(setBase->getSetName()));
        }
    }
    void mOMxFile::writeGeoPartData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName)
    {
        MechSetData::mGeoSet* setData = dynamic_cast<MechSetData::mGeoSet*>(setBase);
        if (setData == nullptr)
            return;

        vector<int> ids = setData->getGeoSetData();
        if (type == Node)
        {
            _out << QString("*set, type = node, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            set<int> nodes = _modelData->getMeshData()->getNodeIdsByPartId(transformPartResult(ids));
            writeSetData(nodes);
        }
        else if (type == NodeSurface)
        {
            set<int> nodes = _modelData->getMeshData()->getNodeIdsByPartId(transformPartResult(ids));
            writeNodeSurfaceData(std::vector<RenderPickResultData>(nodes.begin(), nodes.end()), newName, setData->getInternal());
        }
        else if (type == Element)
        {
            _out << QString("*set, type = element, name = \"%1\"").arg(newName);
            if (setData->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            vector<RenderPickResultData> data = _modelData->getMeshData()->getMeshIDsInPart(transformPartResult(ids));
            writeSetData(data);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("几何集合 %1 暂不支持该类型转化!").arg(setBase->getSetName()));
        }
    }

    void mOMxFile::writeStructOptionTopopara()
    {
        MechStructOptionData::mStructOption* _structOptionBase = getCVM->getModelData()->getStructOptionBase();
        auto& opt = _structOptionBase->_mStructOptionData;

        _out << QString("*topoparam\r\n");

        if (opt._isMaxiter)
        {
            _out << QString("maxiter, %1\r\n").arg(opt._maxiter);
        }

        if (opt._isObjectivetolrance)
        {
            _out << QString("objectivetolrance, %1\r\n").arg(opt._objectivetolrance);
        }

        if (opt._isDiscretetarget)
        {
            _out << QString("discretetarget, %1\r\n").arg(opt._discretetarget);
        }

        if (opt._isTopomovelimit)
        {
            _out << QString("topomovelimit, %1\r\n").arg(opt._topomovelimit);
        }

        if (opt._isRmin)
        {
            _out << QString("mindim, %1\r\n").arg(opt._rmin);
        }

        if (opt._isPenal)
        {
            _out << QString("penal, %1\r\n").arg(opt._penal);
        }

        if (opt._isInitdensity)
        {
            _out << QString("initdensity, %1\r\n").arg(opt._initdensity);
        }

        if (opt._isMinDesity)
        {
            _out << QString("minDensity, %1\r\n").arg(opt._minDesity);
        }

        if (opt._isChecker)
        {
            _out << QString("checker, %1\r\n").arg(opt._checker);
        }

        if (opt._isRebuild)
        {
            _out << QString("Rebuild, %1\r\n").arg(opt._rebuild);
        }

        if (opt._isMatintertype)
        {
            _out << "*matintertype" << "\r\n";;
            _out << opt._matintertype << "\r\n";
        }

        if (opt._isRestart)
        {
            QFileInfo srcInfo(opt._resStartPath);
            if (srcInfo.exists())
            {
                QFileInfo outInfo(_outputFileName);
                QString destPath = outInfo.absolutePath() + "/" + srcInfo.fileName();
                QFile::copy(opt._resStartPath, destPath);
                _out << QString("*restart, iteration = %1, filepath = %2\r\n").arg(opt._iteration).arg(srcInfo.fileName());
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("文件 %1  不存在！").arg(opt._resStartPath));
            }
        }
    }

    void mOMxFile::writeStructOptionDesign()
    {
        MechStructOptionData::mStructOption* structOptionBase = getCVM->getModelData()->getStructOptionBase();

        for (auto& it : structOptionBase->_designData)
        {
            MechStructOptionData::mOptimizationDesignData* design = it.second;
            if (design == nullptr)
                continue;

            int propertyID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_OptimationDesign,design->getId() }, Physical_Property);
            QString propertyName = _modelData->getNameByPhyID(Physical_Property, propertyID);
            _out << QString("*designvar, name = \"%1\"").arg(design->getName());

            bool isLiftingStatus = false;
            bool isSupportStatus = false;
            if (design->_designDomainType == "实体")
            {
                isLiftingStatus = true;
                isSupportStatus = true;
                _out << QString(", type = solidsection, secname = \"%1\"\r\n").arg(propertyName);
            }
            else if (design->_designDomainType == "壳")
            {
                _out << QString(", type = shellsection");
                if (design->_optimizationMethod == "厚度")
                {
                    isLiftingStatus = true;
                    _out << QString(", method = thick, secname = \"%1\"\r\n").arg(propertyName);
                    _out << "*basicthick\r\n";
                    _out << design->_baseThickness << "\r\n";
                }
                else if (design->_optimizationMethod == "加筋")
                {
                    _out << QString(", method = rib, secname = \"%1\"\r\n").arg(propertyName);
                    if (design->_ribHeightStatus || design->_ribWidthStatus)
                    {
                        _out << QString("*ribsize\r\n");

                        if (design->_ribHeightStatus) _out << QString("%1").arg(design->_ribHeight);
                        else _out << " ";

                        if (design->_ribWidthStatus) _out << QString(", %1").arg(design->_ribWidth);
                        else _out << ", ";

                        _out << "\r\n";
                    }
                }
                else if (design->_optimizationMethod == "形貌")
                {
                    _out << QString(", method = topography, secname = \"%1\"\r\n").arg(propertyName);
                    _out << QString("*ribsize\r\n");

                    _out << QString("%1").arg(design->_morphUpLimit);
                    _out << QString(", %1").arg(design->_morphDownLimit);
                    _out << "\r\n";
                    continue;
                }
            }

            int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_OptimationDesign,design->getId() }, Physical_Set);
            QString setName = _modelData->getNameByPhyID(Physical_Set, setID);
            if (setID != -1 && !setName.isEmpty())
            {
                _out << QString("*frozen, set = \"%1\"\r\n").arg(setName);
            }

            if (design->_minMemberStatus || design->_maxMemberStatus || design->_minGapStatus)
            {
                _out << QString("*membersize\r\n");

                if (design->_minMemberStatus) _out << QString("%1").arg(design->_minMemberSize);
                else _out << " ";

                if (design->_maxMemberStatus) _out << QString(", %1").arg(design->_maxMemberSize);
                else _out << ", ";

                if (design->_minGapStatus) _out << QString(", %1").arg(design->_minGapSize);
                else _out << ", ";

                _out << "\r\n";
            }

            if (design->_stressConstraintStatus)
            {
                _out << QString("*stressconstraint\r\n");
                _out << QString("%1\r\n").arg(design->_stressConstraintValue);
            }

            if (design->_patternStatus)
            {
                _out << "*patterngrouping";
                if (design->_patternType == "双平面")
                {
                    _out << ", type = planesym2\r\n";
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_origin.x()).arg(design->_origin.y()).arg(design->_origin.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint1.x()).arg(design->_dirPoint1.y()).arg(design->_dirPoint1.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint2.x()).arg(design->_dirPoint2.y()).arg(design->_dirPoint2.z());
                }
                else  if (design->_patternType == "三平面")
                {
                    _out << ", type = planesym3\r\n";
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_origin.x()).arg(design->_origin.y()).arg(design->_origin.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint1.x()).arg(design->_dirPoint1.y()).arg(design->_dirPoint1.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint2.x()).arg(design->_dirPoint2.y()).arg(design->_dirPoint2.z());
                }
                else if (design->_patternType == "周向")
                {
                    _out << ", type = cyclic\r\n";
                    _out << design->_patternNum << "\r\n";
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_origin.x()).arg(design->_origin.y()).arg(design->_origin.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint1.x()).arg(design->_dirPoint1.y()).arg(design->_dirPoint1.z());
                }
                else  if (design->_patternType == "周向+平面")
                {
                    _out << ", type = cyclicplane\r\n";
                    _out << design->_patternNum << "\r\n";
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_origin.x()).arg(design->_origin.y()).arg(design->_origin.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint1.x()).arg(design->_dirPoint1.y()).arg(design->_dirPoint1.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint2.x()).arg(design->_dirPoint2.y()).arg(design->_dirPoint2.z());
                }
                else
                {
                    _out << ", type = planesym1\r\n";
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_origin.x()).arg(design->_origin.y()).arg(design->_origin.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint1.x()).arg(design->_dirPoint1.y()).arg(design->_dirPoint1.z());
                }
            }

            if (design->_liftingStatus && isLiftingStatus)
            {
                _out << QString("*draw");
                if (design->_liftingType == "单向")
                {
                    _out << ", type = single";
                }
                else if (design->_liftingType == "双向")
                {
                    _out << ", type = split";
                }

                if (design->_nohole == "开启") _out << ", nohole = on";
                _out << "\r\n";

                _out << QString("0, %1, %2, %3\r\n").arg(design->_startPoint.x()).arg(design->_startPoint.y()).arg(design->_startPoint.z());
                _out << QString("0, %1, %2, %3\r\n").arg(design->_dirPoint.x()).arg(design->_dirPoint.y()).arg(design->_dirPoint.z());
            }

            if (design->_extrusionStatus)
            {
                auto writePath = [this](QVector<int>& path) {

                    int num = 0;
                    for (int i = 0; i < path.size(); i++)
                    {
                        num++;
                        _out << QString("%1").arg(path[i], 8);
                        if (num % 16 == 0 || num == path.size())
                        {
                            _out << "\r\n";
                        }
                        else
                        {
                            _out << ",";
                        }
                    }
                    };

                _out << QString("*extrusion");
                if (design->_extrusionType == "平行")
                {
                    _out << ", type = notwist\r\n";

                    _out << "*path1\r\n";
                    writePath(design->_path1);
                }
                else if (design->_extrusionType == "扭转")
                {
                    _out << ", type = twist\r\n";

                    _out << "*path1\r\n";
                    writePath(design->_path1);

                    _out << "*path2\r\n";
                    writePath(design->_path2);
                }
            }

            if (design->_supportStatus && isSupportStatus)
            {
                _out << QString("*overhang, type = constr\r\n");
                _out << design->_overhangAngle << "\r\n";

                _out << QString("0, %1, %2, %3\r\n").arg(design->_supportStartPoint.x()).arg(design->_supportStartPoint.y()).arg(design->_supportStartPoint.z());
                _out << QString("0, %1, %2, %3\r\n").arg(design->_supportDirPoint.x()).arg(design->_supportDirPoint.y()).arg(design->_supportDirPoint.z());
            }

            if (design->_patternRepStatus)
            {
                _out << QString("*patternrepetition");
                if (design->_domainType == "主设计域")
                {
                    _out << ", type = master\r\n";
                }
                else if (design->_domainType == "从设计域")
                {
                    _out << ", type = slave";
                    writeMapIntData(Physical_OptimationDesign, design->getId(), Physical_OptimationDesign);
                    _out << "\r\n";
                }

                _out << "*coord";
                if (design->_coordType == "全局坐标系")
                {
                    _out << ", type = globalsystem\r\n";
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_repBasePoint.x()).arg(design->_repBasePoint.y()).arg(design->_repBasePoint.z());
                }
                else if (design->_coordType == "指定点")
                {
                    _out << ", type = point\r\n";
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_repBasePoint.x()).arg(design->_repBasePoint.y()).arg(design->_repBasePoint.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_repDirPoint1.x()).arg(design->_repDirPoint1.y()).arg(design->_repDirPoint1.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_repDirPoint2.x()).arg(design->_repDirPoint2.y()).arg(design->_repDirPoint2.z());
                    _out << QString("0, %1, %2, %3\r\n").arg(design->_repDirPoint3.x()).arg(design->_repDirPoint3.y()).arg(design->_repDirPoint3.z());
                }

                _out << "*scale\r\n";
                _out << QString("%1, %2, %3\r\n").arg(design->_scaleX).arg(design->_scaleY).arg(design->_scaleZ);
            }
        }
    }

    void mOMxFile::writeStructOptionConstraint()
    {
        MechStructOptionData::mStructOption* structOptionBase = getCVM->getModelData()->getStructOptionBase();
        for (auto it : structOptionBase->_constraintData)
        {
            MechStructOptionData::mOptimizationConstraintData* constraint = it.second;
            if (constraint == nullptr) continue;

            int propertyID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_OptimationConstraint,constraint->getId() }, Physical_Property);
            QString propertyName = _modelData->getNameByPhyID(Physical_Property, propertyID);

            QString type;
            if (constraint->_constraintType == "质量" && propertyName.isEmpty()) type = "totalmass";
            else if (constraint->_constraintType == "质量" && !propertyName.isEmpty()) type = "mass";
            else if (constraint->_constraintType == "质量分数" && propertyName.isEmpty()) type = "totalmassfrac";
            else if (constraint->_constraintType == "质量分数" && !propertyName.isEmpty()) type = "massfrac";
            else if (constraint->_constraintType == "体积" && propertyName.isEmpty()) type = "totalvolume";
            else if (constraint->_constraintType == "体积" && !propertyName.isEmpty()) type = "volume";
            else if (constraint->_constraintType == "体积分数" && propertyName.isEmpty()) type = "totalvolfrac";
            else if (constraint->_constraintType == "体积分数" && !propertyName.isEmpty()) type = "volfrac";
            else if (constraint->_constraintType == "柔度") type = "compliance";
            else if (constraint->_constraintType == "位移") type = "displacement";
            else if (constraint->_constraintType == "频率") type = "frequency";
            else if (constraint->_constraintType == "应力") type = "stress";
            else if (constraint->_constraintType == "综合柔度") type = "weightcompliance";
            else if (constraint->_constraintType == "综合频率") type = "weightfrequency";
            else if (constraint->_constraintType == "柔度指数") type = "complianceindex";
            else if (constraint->_constraintType == "热柔度") type = "thermalcompliance";
            else if (constraint->_constraintType == "节点温度") type = "temperature";
            else if (constraint->_constraintType == "PSD位移") type = "psddisplacement";
            else if (constraint->_constraintType == "频响位移") type = "freqdisplacement";

            QString target = QString("%1_%2").arg(constraint->getName()).arg(type);
            _out << QString("*dconstrain, name = \"%1\", target = \"%2\"\r\n").arg(constraint->getName()).arg(target);

            QString lower = "off"; QString upper = "off";
            QString lowerValue = "0.0"; QString upperValue = "0.0";
            if (constraint->_lowerBoundStatus)
            {
                lower = "on";
                lowerValue = constraint->_lowerBoundValue;
            }

            if (constraint->_upperBoundStatus)
            {
                upper = "on";
                upperValue = constraint->_upperBoundValue;
            }

            _out << QString("*bound, lower = %1, upper = %2\r\n").arg(lower).arg(upper);
            _out << QString("%1, %2\r\n").arg(lowerValue).arg(upperValue);

            //响应类型约束：*response子块与优化目标格式保持一致
            bool isResponseType = constraint->_constraintType == "柔度" || constraint->_constraintType == "位移"
                || constraint->_constraintType == "频率" || constraint->_constraintType == "应力"
                || constraint->_constraintType == "综合柔度" || constraint->_constraintType == "综合频率"
                || constraint->_constraintType == "柔度指数" || constraint->_constraintType == "热柔度"
                || constraint->_constraintType == "节点温度" || constraint->_constraintType == "PSD位移"
                || constraint->_constraintType == "频响位移";
            if (isResponseType)
            {
                writeStructOptionConstraintResponse(constraint, target);
            }
            else
            {
                _out << QString("*response, type = %1, name = \"%2\"").arg(type).arg(target);
                if (!propertyName.isEmpty()) _out << QString(", targetsec = \"%1\"").arg(propertyName);
                _out << "\r\n";
            }
        }
    }

    //优化约束-响应类型子块，格式与 writeStructOptionObjective 的*response子块保持一致
    void mOMxFile::writeStructOptionConstraintResponse(MechStructOptionData::mOptimizationConstraintData* constraint, const QString& target)
    {
        if (constraint == nullptr) return;
        int constraintID = constraint->getId();

        if (constraint->_constraintType == "柔度")
        {
            _out << QString("*response, type = compliance, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";
        }
        else if (constraint->_constraintType == "应力")
        {
            _out << QString("*response, type = stress, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";
        }
        else if (constraint->_constraintType == "热柔度")
        {
            _out << QString("*response, type = thermalcompliance, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";
        }
        else if (constraint->_constraintType == "节点温度")
        {
            _out << QString("*response, type = temperature, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";

            set<int> nodes;
            getNodesByPickType(constraint->_node.first, { constraint->_node.second }, nodes);
            if (nodes.empty()) _out << " ";
            else  _out << *nodes.begin();
            _out << "\r\n";
        }
        else if (constraint->_constraintType == "位移")
        {
            _out << QString("*response, type = displacement, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";

            set<int> nodes;
            getNodesByPickType(constraint->_node.first, { constraint->_node.second }, nodes);
            if (nodes.empty()) _out << " ";
            else  _out << *nodes.begin();
            QStringList list = { "0","0","0" ,"0" ,"0" ,"0" };
            if (constraint->_nodeDofs == "disp")
            {
                list[0] = "1";
                list[1] = "1";
                list[2] = "1";
            }
            else if (constraint->_nodeDofs == "rot")
            {
                list[3] = "1";
                list[4] = "1";
                list[5] = "1";
            }
            else if (constraint->_nodeDofs.toInt() > 0)
            {
                list[constraint->_nodeDofs.toInt() - 1] = "1";
            }
            _out << ", ";
            _out << list.join(", ") << "\r\n";
        }
        else if (constraint->_constraintType == "频率")
        {
            _out << QString("*response, type = frequency, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";

            _out << constraint->_frequencyOrder << "\r\n";
        }
        else if (constraint->_constraintType == "综合柔度")
        {
            _out << QString("*response, type = weightcompliance, name = \"%1\"").arg(target);
            _out << "\r\n";

            for (auto& it : constraint->_caseToweight)
            {
                QString caseName = getCVM->getModelData()->getNameByPhyID(Physical_Case, it.first);
                if (caseName.isEmpty()) continue;

                double val = it.second;
                QString valStr = MxFunctions::mNumberToString(val, 'g', 15);
                if (!valStr.contains('.')) valStr += ".0";
                _out << QString("\"%1\", %2\r\n").arg(caseName).arg(valStr);
            }
        }
        else if (constraint->_constraintType == "综合频率")
        {
            _out << QString("*response, type = weightfrequency, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";

            for (auto it : constraint->_order)
            {
                _out << QString("%1, %2\r\n").arg(it.first).arg(it.second);
            }
        }
        else if (constraint->_constraintType == "柔度指数")
        {
            _out << QString("*response, type = complianceindex, name = \"%1\"").arg(target);
            _out << "\r\n";
            if (constraint->_isCompliance)
            {
                _out << "*linearstep\r\n";
                for (auto& it : constraint->_caseToweight)
                {
                    QString caseName = getCVM->getModelData()->getNameByPhyID(Physical_Case, it.first);
                    if (caseName.isEmpty()) continue;

                    _out << QString("\"%1\", %2\r\n").arg(caseName).arg(it.second);
                }
            }

            if (constraint->_isFrequency)
            {
                _out << "*modestep";
                int id = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_OptimationConstraint, constraintID }, Physical_Case);
                if (id != -1)
                {
                    QString caseName = _modelData->getNameByPhyID(Physical_Case, id);
                    _out << QString(", name = \"%1\"").arg(caseName);
                }
                _out << "\r\n";

                for (auto it : constraint->_order)
                {
                    _out << QString("%1, %2\r\n").arg(it.first).arg(it.second);
                }
            }
        }
        else if (constraint->_constraintType == "PSD位移")
        {
            _out << QString("*response, type = psddisplacement, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";

            _out << "*node\r\n";
            set<int> nodes;
            getNodesByPickType(constraint->_node.first, { constraint->_node.second }, nodes);
            if (nodes.empty()) _out << " ";
            else  _out << *nodes.begin();
            QStringList list = { "0","0","0" ,"0" ,"0" ,"0" };
            if (constraint->_nodeDofs == "disp")
            {
                list[0] = "1";
                list[1] = "1";
                list[2] = "1";
            }
            else if (constraint->_nodeDofs == "rot")
            {
                list[3] = "1";
                list[4] = "1";
                list[5] = "1";
            }
            else if (constraint->_nodeDofs.toInt() > 0)
            {
                list[constraint->_nodeDofs.toInt() - 1] = "1";
            }
            _out << ",";
            _out << list.join(",") << "\r\n";

            _out << "*freqpoint\r\n";
            if (constraint->_frequencyTargetType == "指定频率点")
            {
                _out << constraint->_specifiedFrequency << "\r\n";
            }
            else
            {
                QString freqTarget;
                if (constraint->_frequencyTargetType == "最大响应点") freqTarget = "MAX";
                else if (constraint->_frequencyTargetType == "最小响应点") freqTarget = "MIN";
                else if (constraint->_frequencyTargetType == "平均") freqTarget = "AVG";
                else if (constraint->_frequencyTargetType == "求和") freqTarget = "SUM";
                else if (constraint->_frequencyTargetType == "RMS") freqTarget = "RMS";
                else freqTarget = constraint->_frequencyTargetType;
                _out << freqTarget << "\r\n";
            }
        }
        else if (constraint->_constraintType == "频响位移")
        {
            _out << QString("*response, type = freqdisplacement, name = \"%1\"").arg(target);
            writeMapIntData(Physical_OptimationConstraint, constraintID, Physical_Case);
            _out << "\r\n";

            _out << "*node\r\n";
            set<int> nodes;
            getNodesByPickType(constraint->_node.first, { constraint->_node.second }, nodes);
            if (nodes.empty()) _out << " ";
            else  _out << *nodes.begin();
            QStringList list = { "0","0","0" ,"0" ,"0" ,"0" };
            if (constraint->_nodeDofs == "disp")
            {
                list[0] = "1";
                list[1] = "1";
                list[2] = "1";
            }
            else if (constraint->_nodeDofs == "rot")
            {
                list[3] = "1";
                list[4] = "1";
                list[5] = "1";
            }
            else if (constraint->_nodeDofs.toInt() > 0)
            {
                list[constraint->_nodeDofs.toInt() - 1] = "1";
            }
            _out << ",";
            _out << list.join(",");

            QString valueType;
            if (constraint->_frfValueType == "幅值") valueType = "M";
            else if (constraint->_frfValueType == "实部") valueType = "R";
            else if (constraint->_frfValueType == "虚部") valueType = "I";
            else valueType = constraint->_frfValueType;
            _out << "," << valueType << "\r\n";

            _out << "*freqpoint\r\n";
            if (constraint->_frequencyTargetType == "指定频率点")
            {
                _out << constraint->_specifiedFrequency << "\r\n";
            }
            else
            {
                QString freqTarget;
                if (constraint->_frequencyTargetType == "最大响应点") freqTarget = "MAX";
                else if (constraint->_frequencyTargetType == "最小响应点") freqTarget = "MIN";
                else if (constraint->_frequencyTargetType == "平均") freqTarget = "AVG";
                else if (constraint->_frequencyTargetType == "求和") freqTarget = "SUM";
                else if (constraint->_frequencyTargetType == "RMS") freqTarget = "RMS";
                else freqTarget = constraint->_frequencyTargetType;
                _out << freqTarget << "\r\n";
            }
        }
    }

    void mOMxFile::writeStructOptionObjective()
    {
        MechStructOptionData::mStructOption* structOptionBase = getCVM->getModelData()->getStructOptionBase();
        MechStructOptionData::mOptimizationGoalData* goal = structOptionBase->_goalData;
        if (goal == nullptr) return;
        int GoalID = _modelData->getModelDataID();

        _out << "*objective";
        if (goal->_objectiveType == "最小化")
        {
            _out << ", type = min";
        }
        else if (goal->_objectiveType == "最大化")
        {
            _out << ", type = max";
        }

        _out << ", name = \"objName\", target = \"objResp\"\r\n";
        _out << "*response";

        if (goal->_responseType == "柔度")
        {
            _out << ", type = compliance, name = \"objResp\"";
            writeMapIntData(Physical_OptimationGoal, GoalID, Physical_Case);
            _out << "\r\n";
        }
        else if (goal->_responseType == "应力")
        {
            _out << ", type = stress, name = \"objResp\"";
            writeMapIntData(Physical_OptimationGoal, GoalID, Physical_Case);
            _out << "\r\n";
        }
        else if (goal->_responseType == "热柔度")
        {
            _out << ", type = thermalcompliance, name = \"objResp\"";
            writeMapIntData(Physical_OptimationGoal, GoalID, Physical_Case);
            _out << "\r\n";
        }
        else if (goal->_responseType == "节点温度")
        {
            _out << ", type = temperature, name = \"objResp\"";
            writeMapIntData(Physical_OptimationGoal, GoalID, Physical_Case);
            _out << "\r\n";

            set<int> nodes;
            getNodesByPickType(goal->_node.first, { goal->_node.second }, nodes);
            if (nodes.empty()) _out << " ";
            else  _out << *nodes.begin();
            _out << "\r\n";
        }
        else if (goal->_responseType == "位移")
        {
            _out << ", type = displacement, name = \"objResp\"";
            writeMapIntData(Physical_OptimationGoal, GoalID, Physical_Case);
            _out << "\r\n";

            set<int> nodes;
            getNodesByPickType(goal->_node.first, { goal->_node.second }, nodes);
            if (nodes.empty()) _out << " ";
            else  _out << *nodes.begin();
            QStringList list = { "0","0","0" ,"0" ,"0" ,"0" };
            if (goal->_nodeDofs == "disp")
            {
                list[0] = "1";
                list[1] = "1";
                list[2] = "1";
            }
            else if (goal->_nodeDofs == "rot")
            {
                list[3] = "1";
                list[4] = "1";
                list[5] = "1";
            }
            else if (goal->_nodeDofs.toInt() > 0)
            {
                list[goal->_nodeDofs.toInt() - 1] = "1";
            }
            _out << ", ";
            _out << list.join(", ") << "\r\n";
        }
        else if (goal->_responseType == "频率")
        {
            _out << ", type = frequency, name = \"objResp\"";
            writeMapIntData(Physical_OptimationGoal, GoalID, Physical_Case);
            _out << "\r\n";

            _out << goal->_frequencyOrder << "\r\n";
        }
        else if (goal->_responseType == "综合柔度")
        {
            _out << ", type = weightcompliance, name = \"objResp\"";
            _out << "\r\n";

            for (auto& it : goal->_caseToweight)
            {
                QString caseName = getCVM->getModelData()->getNameByPhyID(Physical_Case, it.first);
                if (caseName.isEmpty()) continue;

                double val = it.second;
                QString valStr = MxFunctions::mNumberToString(val, 'g', 15);
                if (!valStr.contains('.')) valStr += ".0";
                _out << QString("\"%1\", %2\r\n").arg(caseName).arg(valStr);
            }
        }
        else if (goal->_responseType == "综合频率")
        {
            _out << ", type = weightfrequency, name = \"objResp\"";
            writeMapIntData(Physical_OptimationGoal, GoalID, Physical_Case);
            _out << "\r\n";

            for (auto it : goal->_order)
            {
                _out << QString("%1, %2\r\n").arg(it.first).arg(it.second);
            }
        }
        else if (goal->_responseType == "柔度指数")
        {
            _out << ", type = complianceindex, name = \"objResp\"";
            _out << "\r\n";
            if (goal->_isCompliance)
            {
                _out << "*linearstep\r\n";
                for (auto& it : goal->_caseToweight)
                {
                    QString caseName = getCVM->getModelData()->getNameByPhyID(Physical_Case, it.first);
                    if (caseName.isEmpty()) continue;

                    _out << QString("\"%1\", %2\r\n").arg(caseName).arg(it.second);
                }
            }

            if (goal->_isFrequency)
            {
                _out << "*modestep";
                int id = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_OptimationGoal, GoalID }, Physical_Case);
                if (id != -1)
                {
                    QString caseName = _modelData->getNameByPhyID(Physical_Case, id);
                    _out << QString(", name = \"%1\"").arg(caseName);
                }
                _out << "\r\n";

                for (auto it : goal->_order)
                {
                    _out << QString("%1, %2\r\n").arg(it.first).arg(it.second);
                }
            }

        }
        // PSD位移 关键字格式（1.1.1 随机振动工况相关响应）
        else if (goal->_responseType == "PSD位移")
        {
            _out << ", type = psddisplacement, name = \"objResp\"";

            int caseId = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_OptimationGoal, GoalID }, Physical_Case);
            if (caseId != -1)
            {
                QString caseName = _modelData->getNameByPhyID(Physical_Case, caseId);
                if (!caseName.isEmpty())
                    _out << QString(", targetstep = \"%1\"").arg(caseName);
            }
            _out << "\r\n";

            _out << "*node\r\n";
            set<int> nodes;
            getNodesByPickType(goal->_node.first, { goal->_node.second }, nodes);
            if (nodes.empty()) _out << " ";
            else  _out << *nodes.begin();
            QStringList list = { "0","0","0" ,"0" ,"0" ,"0" };
            if (goal->_nodeDofs == "disp")
            {
                list[0] = "1";
                list[1] = "1";
                list[2] = "1";
            }
            else if (goal->_nodeDofs == "rot")
            {
                list[3] = "1";
                list[4] = "1";
                list[5] = "1";
            }
            else if (goal->_nodeDofs.toInt() > 0)
            {
                // 单个自由度数字：对应位为1，其余为0（如 "1"→1,0,0,0,0,0；"2"→0,1,0,0,0,0）
                list[goal->_nodeDofs.toInt() - 1] = "1";
            }
            _out << ",";
            _out << list.join(",") << "\r\n";

            _out << "*freqpoint\r\n";
            if (goal->_frequencyTargetType == "指定频率点")
            {
                _out << goal->_specifiedFrequency << "\r\n";
            }
            else
            {
                QString freqTarget;
                if (goal->_frequencyTargetType == "最大响应点") freqTarget = "MAX";
                else if (goal->_frequencyTargetType == "最小响应点") freqTarget = "MIN";
                else if (goal->_frequencyTargetType == "平均") freqTarget = "AVG";
                else if (goal->_frequencyTargetType == "求和") freqTarget = "SUM";
                else if (goal->_frequencyTargetType == "RMS") freqTarget = "RMS";
                else freqTarget = goal->_frequencyTargetType;
                _out << freqTarget << "\r\n";
            }
        }
        else if (goal->_responseType == "频响位移")
        {
            _out << ", type = freqdisplacement, name = \"objResp\"";

            int caseId = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_OptimationGoal, GoalID }, Physical_Case);
            if (caseId != -1)
            {
                QString caseName = _modelData->getNameByPhyID(Physical_Case, caseId);
                if (!caseName.isEmpty())
                    _out << QString(", targetstep = \"%1\"").arg(caseName);
            }
            _out << "\r\n";

            _out << "*node\r\n";
            set<int> nodes;
            getNodesByPickType(goal->_node.first, { goal->_node.second }, nodes);
            if (nodes.empty()) _out << " ";
            else  _out << *nodes.begin();
            QStringList list = { "0","0","0" ,"0" ,"0" ,"0" };
            if (goal->_nodeDofs == "disp")
            {
                list[0] = "1";
                list[1] = "1";
                list[2] = "1";
            }
            else if (goal->_nodeDofs == "rot")
            {
                list[3] = "1";
                list[4] = "1";
                list[5] = "1";
            }
            else if (goal->_nodeDofs.toInt() > 0)
            {
                // 单个自由度数字：对应位为1，其余为0（如 "1"→1,0,0,0,0,0；"2"→0,1,0,0,0,0）
                list[goal->_nodeDofs.toInt() - 1] = "1";
            }
            _out << ",";
            _out << list.join(",");

            QString valueType;
            if (goal->_frfValueType == "幅值") valueType = "M";
            else if (goal->_frfValueType == "实部") valueType = "R";
            else if (goal->_frfValueType == "虚部") valueType = "I";
            else valueType = goal->_frfValueType;
            _out << "," << valueType << "\r\n";

            _out << "*freqpoint\r\n";
            if (goal->_frequencyTargetType == "指定频率点")
            {
                _out << goal->_specifiedFrequency << "\r\n";
            }
            else
            {
                QString freqTarget;
                if (goal->_frequencyTargetType == "最大响应点") freqTarget = "MAX";
                else if (goal->_frequencyTargetType == "最小响应点") freqTarget = "MIN";
                else if (goal->_frequencyTargetType == "平均") freqTarget = "AVG";
                else if (goal->_frequencyTargetType == "求和") freqTarget = "SUM";
                else if (goal->_frequencyTargetType == "RMS") freqTarget = "RMS";
                else freqTarget = goal->_frequencyTargetType;
                _out << freqTarget << "\r\n";
            }
        }
    }


    void mOMxFile::writeNodeSetData(MechSetData::mSetBase* setBase)
    {
        _out << QString("*set, type = node, name = \"%1\"").arg(setBase->getSetName());
        if (setBase->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";

        MechSetData::mNode* set = dynamic_cast<MechSetData::mNode*>(setBase);
        if (set)
        {
            const vector<RenderPickResultData>& data = set->getNodeData();
            writeSetData(data);
        }
    }
    void mOMxFile::writeNodeSurfaceData(MechSetData::mSetBase* setBase)
    {
        _out << QString("*set, type = nodesurface, name = \"%1\"").arg(setBase->getSetName());
        if (setBase->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";

        MechSetData::mNodeFace* set = dynamic_cast<MechSetData::mNodeFace*>(setBase);
        if (set)
        {
            const vector<RenderPickResultData>& data = set->getNodeData();
            writeSetData(data);
        }
    }
    void mOMxFile::writeNodeSurfaceData(const std::vector<RenderPickResultData>& data, const QString& name, bool internal)
    {
        _out << QString("*set, type = nodesurface, name = \"%1\"").arg(name);
        if (internal)
        {
            _out << ", internal";
        }
        _out << "\r\n";
        writeSetData(data);
    }
    void mOMxFile::writeSurfaceAnalysisData(MechSetData::mSetBase* setBase)
    {
        auto* set = dynamic_cast<MechSetData::mSurfaceAnalysis*>(setBase);
        if (!set)
            return;

        // 根据子类型确定关键字 type 字符串
        QString typeStr;
        switch (set->surfaceSubType())
        {
        case MechSetData::RevolutionSurface:
            typeStr = "revolutionsurface";
            break;
        case MechSetData::CylinderSurface:
            typeStr = "cylindersurface";
            break;
        case MechSetData::SegmentSurface:
            typeStr = "segmentsurface";
            break;
        default:
            return;
        }

        _out << QString("*set, type = %1, name = \"%2\"").arg(typeStr, set->getSetName());
        if (set->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";

        if (set->surfaceSubType() == MechSetData::RevolutionSurface)
        {
            // 旋转轴任意点A、B
            _out << QString("%1, %2, %3, %4, %5, %6")
                .arg(MxFunctions::mNumberToString(set->xa()))
                .arg(MxFunctions::mNumberToString(set->ya()))
                .arg(MxFunctions::mNumberToString(set->za()))
                .arg(MxFunctions::mNumberToString(set->xb()))
                .arg(MxFunctions::mNumberToString(set->yb()))
                .arg(MxFunctions::mNumberToString(set->zb()));
            _out << "\r\n";

            // 起点 rz局部坐标
            _out << QString("START, %1, %2")
                .arg(MxFunctions::mNumberToString(set->r1()))
                .arg(MxFunctions::mNumberToString(set->z1()));
            _out << "\r\n";

            // 终点 rz局部坐标
            _out << QString("LINE, %1, %2")
                .arg(MxFunctions::mNumberToString(set->r2()))
                .arg(MxFunctions::mNumberToString(set->z2()));
            _out << "\r\n";
        }
        else if (set->surfaceSubType() == MechSetData::CylinderSurface)
        {
            // 原点 + x正轴 + z负轴
            _out << QString("%1, %2, %3, %4, %5, %6, %7, %8, %9")
                .arg(MxFunctions::mNumberToString(set->ox()))
                .arg(MxFunctions::mNumberToString(set->oy()))
                .arg(MxFunctions::mNumberToString(set->oz()))
                .arg(MxFunctions::mNumberToString(set->xx()))
                .arg(MxFunctions::mNumberToString(set->xy()))
                .arg(MxFunctions::mNumberToString(set->xz()))
                .arg(MxFunctions::mNumberToString(set->zx()))
                .arg(MxFunctions::mNumberToString(set->zy()))
                .arg(MxFunctions::mNumberToString(set->zz()));
            _out << "\r\n";

            // 起点 xy局部坐标
            _out << QString("START, %1, %2")
                .arg(MxFunctions::mNumberToString(set->sx()))
                .arg(MxFunctions::mNumberToString(set->sy()));
            _out << "\r\n";

            // 终点 xy局部坐标
            _out << QString("LINE, %1, %2, %3")
                .arg(MxFunctions::mNumberToString(set->ex()))
                .arg(MxFunctions::mNumberToString(set->ey()));
            _out << "\r\n";
        }
        else if (set->surfaceSubType() == MechSetData::SegmentSurface)
        {
            // 原点 + x正轴 + z负轴
            _out << QString("%1, %2, %3, %4, %5, %6, %7, %8, %9")
                .arg(MxFunctions::mNumberToString(set->ox()))
                .arg(MxFunctions::mNumberToString(set->oy()))
                .arg(MxFunctions::mNumberToString(set->oz()))
                .arg(MxFunctions::mNumberToString(set->xx()))
                .arg(MxFunctions::mNumberToString(set->xy()))
                .arg(MxFunctions::mNumberToString(set->xz()))
                .arg(MxFunctions::mNumberToString(set->zx()))
                .arg(MxFunctions::mNumberToString(set->zy()))
                .arg(MxFunctions::mNumberToString(set->zz()));
            _out << "\r\n";

            // 起点 xy局部坐标
            _out << QString("START, %1, %2")
                .arg(MxFunctions::mNumberToString(set->sx()))
                .arg(MxFunctions::mNumberToString(set->sy()));
            _out << "\r\n";

            // 线段轮廓：每行 LINE 或 CIRCL
            const auto& contours = set->contours();
            for (const auto& row : contours)
            {
                if (row.rowType == "line")
                {
                    _out << QString("LINE, %1, %2")
                        .arg(MxFunctions::mNumberToString(row.x1))
                        .arg(MxFunctions::mNumberToString(row.y1));
                }
                else if (row.rowType == "circl")
                {
                    _out << QString("CIRCL, %1, %2, %3, %4")
                        .arg(MxFunctions::mNumberToString(row.x1))
                        .arg(MxFunctions::mNumberToString(row.y1))
                        .arg(MxFunctions::mNumberToString(row.x2))
                        .arg(MxFunctions::mNumberToString(row.y2));
                }
                _out << "\r\n";
            }
        }
    }
    void mOMxFile::writeElementSetData(MechSetData::mSetBase* setBase)
    {
        _out << QString("*set, type = element, name = \"%1\"").arg(setBase->getSetName());
        if (setBase->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";
        MechSetData::mElementSet* set = dynamic_cast<MechSetData::mElementSet*>(setBase);
        if (set)
        {
            const vector<RenderPickResultData>& data = set->getElementSetData();
            writeSetData(data);
        }
    }
    void mOMxFile::writeElementSurfaceData(MechSetData::mSetBase* setBase)
    {
        _out << QString("*set, type = elementsurface, name = \"%1\"").arg(setBase->getSetName());
        if (setBase->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";

        MechSetData::mElementSurface* set = dynamic_cast<MechSetData::mElementSurface*>(setBase);
        if (set)
        {
            const vector<RenderPickResultData>& data = set->getElementSurfaceData();
            writeSurfaceSetData(data);
        }
    }
    void mOMxFile::writeElementEdgeData(MechSetData::mSetBase* setBase)
    {
        _out << QString("*set, type = elementedge, name = \"%1\"").arg(setBase->getSetName());
        if (setBase->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";

        MechSetData::mElementSurface* set = dynamic_cast<MechSetData::mElementSurface*>(setBase);
        if (set)
        {
            const vector<RenderPickResultData>& data = set->getElementSurfaceData();
            writeEdgeSetData(data);
        }
    }

    void mOMxFile::writeFrequencyPointData(MechSetData::mSetBase* setBase)
    {
        _out << QString("*set, type = frequency, name = \"%1\"").arg(setBase->getSetName());
        if (setBase->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";
        MechSetData::mFrequencyPoint* set = dynamic_cast<MechSetData::mFrequencyPoint*>(setBase);

        int num = 0;
        int iOrderSize = set->orderCount();
        for (int i = 0; i < iOrderSize; i++)
        {
            num++;
            _out << MxFunctions::mNumberToString(set->orderSecondValueAt(i));
            if (num % 16 == 0 || num == iOrderSize)
            {
                _out << "\r\n";
            }
            else
            {
                _out << ",";
            }

        }
    }

    void mOMxFile::writeDofNodeData(MechSetData::mSetBase* setBase)
    {
        _out << QString("*set, type = nodec, name = \"%1\"").arg(setBase->getSetName());
        if (setBase->getInternal())
        {
            _out << ", internal";
        }
        _out << "\r\n";
        MechSetData::mDOFNode* set = dynamic_cast<MechSetData::mDOFNode*>(setBase);
        int idof = set->dof();
        QString dofStr = "";
        switch (idof)
        {
        case static_cast<int>(Dof::T1):
            dofStr = "T1";
            break;
        case static_cast<int>(Dof::T2):
            dofStr = "T2";
            break;
        case static_cast<int>(Dof::T3):
            dofStr = "T3";
            break;
        case static_cast<int>(Dof::R1):
            dofStr = "R1";
            break;
        case static_cast<int>(Dof::R2):
            dofStr = "R2";
            break;
        case static_cast<int>(Dof::R3):
            dofStr = "R3";
            break;
        default:
            break;
        }

        const auto& data = set->getNodeData();
        for (auto node : data)
        {
            _out << MxFunctions::mNumberToString(node.result_id) + ", " + dofStr;
            _out << "\r\n";
        }


    }

    void mOMxFile::writePanelData(MechSetData::mSetBase* setBase)
    {
        MechSetData::mPanel* set = dynamic_cast<MechSetData::mPanel*>(setBase);
        int panelId = set->getSetId();
        MechMapRelation::mMapRelationManager& mapManager = _modelData->getMapRelationManager();
        QList<int> iElementIds = mapManager.getMapRelation<QList<int>>({ Physical_Set, panelId }, Physical_Set);

        _out << "\r\n";
        if (set->getPanelCollectionType() == 0)
        {
            _out << QString("*set, type = erppnl, name = \"%1\"").arg(setBase->getSetName());
            if (setBase->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            auto ele = set->elenames();
            QStringList lineParts;
            for (auto elementName : ele)
            {
                int id = _modelData->getPhyIDByName(Physical_Set, elementName);
                if (iElementIds.contains(id))
                {
                    lineParts.append(QString("\"%1\"").arg(elementName));
                }
            }
            if (!lineParts.isEmpty())
            {
                for (int i = 0; i < lineParts.size(); ++i)
                {
                    _out << lineParts[i];
                    if ((i + 1) % 8 == 0)
                    {
                        _out << "\r\n";
                        continue;
                    }

                    if (i != lineParts.size() - 1)
                    {
                        _out << ",";
                    }
                }
                if (lineParts.size() % 8 != 0)
                {
                    _out << "\r\n";
                }
            }
        }
        else
        {

            _out << QString("*set, type = panel, name = \"%1\"").arg(setBase->getSetName());
            if (setBase->getInternal())
            {
                _out << ", internal";
            }
            _out << "\r\n";

            //会获取单元集和节点集
            auto nodeAndEle = set->nodeNames();
            QStringList lineParts;

            for (auto name : nodeAndEle)
            {
                int id = _modelData->getPhyIDByName(Physical_Set, name);
                if (iElementIds.contains(id))
                {
                    lineParts.append(QString("\"%1\"").arg(name));
                }
            }
            if (!lineParts.isEmpty())
            {
                for (int i = 0; i < lineParts.size(); ++i)
                {
                    _out << lineParts[i] << ",";
                    if ((i + 1) % 8 == 0)
                    {
                        _out << "\r\n";
                    }
                }
                if (lineParts.size() % 8 != 0)
                {
                    _out << "\r\n";
                }
            }
        }

    }

    QString mOMxFile::getRenameSet()
    {
        QString name;
        do
        {
            name = QString("contain_%1").arg(_index++);

        } while (_modelData->getManager<MechSetData::mSetManager>(Physical_Set).ExistSetName(name));

        return name;
    }
    void mOMxFile::writeSurfaceSetData(const std::vector<RenderPickResultData>& datas)
    {
        if (datas.size() == 0)
            return;

        int dim = _modelData->getMeshData()->getDim(datas[0]);
        if (dim == 3)
        {
            for (auto& data : datas)
            {
                _out << QString("%1, B%2\r\n").arg(data.result_id).arg(data.result_id2);
            }
        }
        else if (dim == 2)
        {
            for (auto& data : datas)
            {
                _out << QString("%1, BPOS\r\n").arg(data.result_id);  //未完善，现二维网格表面全为正面
            }
        }

    }
    void mOMxFile::writeEdgeSetData(const std::vector<RenderPickResultData>& datas)
    {
        if (datas.size() == 0)
            return;

        int elementId; int Bn;
        for (auto& data : datas)
        {
            if (_meshData->getElementAndBnByEdgeNode(data, elementId, Bn))
            {
                _out << QString("%1, B%2\r\n").arg(elementId).arg(Bn);
            }
        }
    }
    void mOMxFile::writeSetData(const std::vector<RenderPickResultData>& data)
    {
        int num = 0;
        for (int i = 0; i < data.size(); i++)
        {
            num++;
            _out << QString("%1").arg(data[i].result_id, 8);
            if (num % 16 == 0 || num == data.size())
            {
                _out << "\r\n";
            }
            else
            {
                _out << ",";
            }
        }
    }
    void mOMxFile::writeSetData(const std::set<int>& data)
    {
        int num = 0;
        for (auto& it : data)
        {
            num++;
            _out << QString("%1").arg(it, 8);
            if (num % 16 == 0 || num == data.size())
            {
                _out << "\r\n";
            }
            else
            {
                _out << ",";
            }
        }
    }

    bool mOMxFile::writeContactData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Contact);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            MechContactData::mContactBase* contact = _modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(id);
            if (!contact || contact->isSuppressed())
                continue;

            switch (contact->getContactType())
            {
            case NodeToSurfaceContact:
                writeNodeToSurfaceContact(contact);
                break;
            case SurfaceToSurfaceContact:
                writeSurfaceToSurfaceContact(contact);
                break;
            case TwoDimensionalContact:
                writeTwoDimensionalContact(contact);
                break;
            case CommonContact:
                writeCommonContact(contact);
                break;
            case SelfContact:
                writeSelfContact(contact);
                break;
            case LineToSurfaceContact:
                writeLineToSurfaceContact(contact);
                break;
            default:
                break;
            }
        }

        //后写过盈接触
        for (auto& id : ids)
        {
            MechContactData::mContactBase* contact = _modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(id);
            if (!contact || contact->isSuppressed())
                continue;

            if (contact->getContactType() != InterferenceContact)
                continue;

            if (!writeInterferenceContactData(contact))
                return false;
        }

        mGlobalSignals::getInstance()->outputMessageSig("接触写文件完成");
        return true;
    }

    bool mOMxFile::writePeriodicBoundaryData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_PeriodicBoundary);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            MechContactData::mPeriodicBoundary* periodicData = _modelData->getManager<MechContactData::mPeriodicBoundaryManager>(Physical_PeriodicBoundary).getPeriodicBoundaryData(id);
            if (!periodicData || periodicData->isSuppressed())
                continue;

            int slaveID = _modelData->getMapRelationManager().getMapRelation<int>(
                { Physical_PeriodicBoundary, periodicData->getPeriodicBoundaryId() },
                Physical_Set, MapRelation_Slave);
            int masterID = _modelData->getMapRelationManager().getMapRelation<int>(
                { Physical_PeriodicBoundary, periodicData->getPeriodicBoundaryId() },
                Physical_Set, MapRelation_Master);
            QString slaveName = writeGeoSet(slaveID, ElementSurface);
            QString masterName = writeGeoSet(masterID, ElementSurface);

            QString typeStr = (periodicData->getPeriodicBoundaryType() == PeriodicBoundary_Rotation)
                ? "rotation" : "translation";
            QString adjustStr = periodicData->isAdjust() ? "on" : "off";

            // 头部行：始终输出adjust字段
            _out << QString("*periodic, type = %1, name = \"%2\",adjust = %3\r\n")
                .arg(typeStr)
                .arg(periodicData->getPeriodicBoundaryName())
                .arg(adjustStr);

            QString value = periodicData->value;
            bool isManual = !periodicData->isAutoCalc();

            if (isManual)
            {
                // 手动指定模式：输出10个参数，末尾追加tol
                _out << QString("\"%1\", \"%2\", %3, %4, %5, %6, %7, %8, %9, %10\r\n")
                    .arg(slaveName)
                    .arg(masterName)
                    .arg(periodicData->startPoint.x(), 0, 'g', 15)
                    .arg(periodicData->startPoint.y(), 0, 'g', 15)
                    .arg(periodicData->startPoint.z(), 0, 'g', 15)
                    .arg(periodicData->endPoint.x(), 0, 'g', 15)
                    .arg(periodicData->endPoint.y(), 0, 'g', 15)
                    .arg(periodicData->endPoint.z(), 0, 'g', 15)
                    .arg(value)
                    .arg(periodicData->getManualValue(), 0, 'g', 15);
            }
            else
            {
                // 自动计算模式：输出9个参数，无tol
                _out << QString("\"%1\", \"%2\", %3, %4, %5, %6, %7, %8, %9\r\n")
                    .arg(slaveName)
                    .arg(masterName)
                    .arg(periodicData->startPoint.x(), 0, 'g', 15)
                    .arg(periodicData->startPoint.y(), 0, 'g', 15)
                    .arg(periodicData->startPoint.z(), 0, 'g', 15)
                    .arg(periodicData->endPoint.x(), 0, 'g', 15)
                    .arg(periodicData->endPoint.y(), 0, 'g', 15)
                    .arg(periodicData->endPoint.z(), 0, 'g', 15)
                    .arg(value);
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("周期性边界写文件完成");
        return true;
    }

    void mOMxFile::writeNodeToSurfaceContact(MechContactData::mContactBase* contact)
    {
        int slaveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Slave);
        int masterID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Master);
        //从面区域为节点表面时输出节点面集合（*set, type = nodesurface），否则按单元面集合输出
        QString slaveName = writeGeoSet(slaveID, contact->slave_region == 1 ? MxFunctions::NodeSurface : ElementSurface);
        QString masterName = writeGeoSet(masterID, ElementSurface);
        _interferenceContact[contact->getContactId()] = { slaveName , masterName };

        _out << QString("*contact, type = nodetosurface, name = \"%1\"").arg(contact->getContactName());
        _out << (contact->smallSlip ? ", smallsliding = on" : ", smallsliding = off");
        _out << "\r\n";
        _out << QString("\"%1\", \"%2\"\r\n").arg(slaveName).arg(masterName);

        writeBehavior(contact);
        writeFriction(contact);
        writeGapconductance(contact);
        writeAdjust(contact);
    }
    void mOMxFile::writeLineToSurfaceContact(MechContactData::mContactBase* contact)
    {
        int slaveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Slave);
        int masterID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Master);
        QString slaveName = writeGeoSet(slaveID, contact->slave_region == 1 ? MxFunctions::NodeSurface : ElementSurface);
        QString masterName = writeGeoSet(masterID, ElementSurface);
        _interferenceContact[contact->getContactId()] = { slaveName , masterName };

        _out << QString("*contact, type = nodetosurface, name = \"%1\"").arg(contact->getContactName());
        _out << (contact->smallSlip ? ", smallsliding = on" : ", smallsliding = off");
        _out << ", line = on\r\n";
        _out << QString("\"%1\", \"%2\"\r\n").arg(slaveName).arg(masterName);

        writeBehavior(contact);
        writeFriction(contact);
        writeGapconductance(contact);
        writeAdjust(contact);
    }
    void mOMxFile::writeSurfaceToSurfaceContact(MechContactData::mContactBase* contact)
    {
        int slaveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Slave);
        int masterID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Master);
        const auto targetSetType = ElementSurface;
        QString slaveName = writeGeoSet(slaveID, targetSetType);
        QString masterName = writeGeoSet(masterID, targetSetType);
        _interferenceContact[contact->getContactId()] = { slaveName , masterName };

        _out << QString("*contact, type = surfacetosurface, name = \"%1\"").arg(contact->getContactName());
        if (contact->smallSlip)
            _out << ", smallsliding = on";
        else
            _out << ", smallsliding = off";

        _out << "\r\n";
        _out << QString("\"%1\", \"%2\"\r\n").arg(slaveName).arg(masterName);

        writeBehavior(contact);
        writeFriction(contact);
        writeGapconductance(contact);

        if (!contact->smallSlip)
        {
            writeClearance(contact);
        }
        writeAdjust(contact);
        if (contact->evaluation)
        {
            _out << "*evaluation\r\n";
            contact->eva_MP ? _out << "1," : _out << "0,";
            contact->eva_SD ? _out << "1," : _out << "0,";
            contact->eva_CV ? _out << "1," : _out << "0,";
            contact->eva_PC ? _out << "1," : _out << "0,";
            contact->eva_R ? _out << "1," : _out << "0,";
            _out << "\r\n";
        }

    }
    void mOMxFile::writeTwoDimensionalContact(MechContactData::mContactBase* contact)
    {
        int slaveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Slave);
        int masterID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Master);
        QString slaveName = writeGeoSet(slaveID, ElementEdge);
        QString masterName = writeGeoSet(masterID, ElementEdge);
        _interferenceContact[contact->getContactId()] = { slaveName , masterName };

        _out << QString("*contact, type = surfacetosurface, name = \"%1\"").arg(contact->getContactName());
        _out << (contact->smallSlip ? ", smallsliding = on" : ", smallsliding = off");
        _out << ", line = on\r\n";
        _out << QString("\"%1\", \"%2\"\r\n").arg(slaveName).arg(masterName);

        writeBehavior(contact);
        writeFriction(contact);
        if (!contact->smallSlip)
        {
            writeClearance(contact);
        }
        writeAdjust(contact);
    }
    void mOMxFile::writeCommonContact(MechContactData::mContactBase* contact)
    {
        _out << QString("*contact, type = surfacetosurface, name = \"%1\", autosearch = on").arg(contact->getContactName());
        if (contact->smallSlip)
            _out << ", smallsliding = on";
        else
            _out << ", smallsliding = off";

        _out << "\r\n";

        writeBehavior(contact);
        writeFriction(contact);
        writeGapconductance(contact);
        writeClearance(contact);
        writeAdjust(contact);
    }
    void mOMxFile::writeSelfContact(MechContactData::mContactBase* contact)
    {
        int masterID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Set, MapRelation_Master);
        QString masterName = writeGeoSet(masterID, ElementSurface);

        _out << QString("*contact, type = surfacetosurface, name = \"%1\"").arg(contact->getContactName());
        _out << ", selfcontact = on"; //自接触
        _out << "\r\n";

        _out << QString("\"%1\"\r\n").arg(masterName);

        writeBehavior(contact);
        writeFriction(contact);
        writeGapconductance(contact);
        writeClearance(contact);
        writeAdjust(contact);
    }
    bool mOMxFile::writeInterferenceContactData(MechContactData::mContactBase* contact)
    {
        const int contactID = _modelData->getMapRelationManager().getMapRelation<int>(
            { Physical_Contact,contact->getContactId() }, Physical_Contact);
        auto* pairContact = _modelData->getManager<MechContactData::mContactManager>(Physical_Contact).getContactData(contactID);
        const auto fail = [&](const QString& reason) {
            mGlobalSignals::getInstance()->outputMessageSig(1,
                QString("过盈接触 %1 导出失败：%2").arg(contact->getContactName(), reason));
            return false;
            };
        if (!pairContact)
            return fail(QStringLiteral("接触对不存在，请重新选择"));
        if (pairContact->isSuppressed())
            return fail(QStringLiteral("接触对 %1 已被抑制").arg(pairContact->getContactName()));

        const auto pairType = pairContact->getContactType();
        if (pairType != SurfaceToSurfaceContact && pairType != NodeToSurfaceContact
            && pairType != TwoDimensionalContact)
        {
            return fail(QStringLiteral("接触对 %1 的类型不支持").arg(pairContact->getContactName()));
        }

        const auto sets = _interferenceContact.constFind(contactID);
        if (sets == _interferenceContact.cend() || sets->first.isEmpty() || sets->second.isEmpty())
            return fail(QStringLiteral("接触对 %1 的主从集合无效").arg(pairContact->getContactName()));

        writeInterferenceContact(contact);
        return true;
    }
    void mOMxFile::writeInterferenceContact(MechContactData::mContactBase* contact)
    {
        _out << QString("*contactinterference, name = \"%1\"").arg(contact->getContactName());
        if (contact->clr_fit_typ == 0)
        {
            _out << ", shrink  = on";
        }
        else
        {
            writeMapIntData(Physical_Contact, contact->getContactId(), Physical_Function);
        }
        _out << "\r\n";

        int contactID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Contact,contact->getContactId() }, Physical_Contact);
        const auto& contactSets = _interferenceContact[contactID];
        _out << QString("\"%1\", \"%2\"").arg(contactSets.first, contactSets.second);

        if (contact->clr_fit_typ == 1)
        {
            _out << ", " << contact->size;
        }
        else
        {
            _out << ", 0";
        }
        _out << "\r\n";
    }
    void mOMxFile::writeBehavior(MechContactData::mContactBase* contact)
    {
        _out << "*behavior";
        if (contact->overclosure == "线性接触")
        {
            _out << ", pressurecurve = linear";
            _out << "\r\n";

            if (!contact->penalty_factor.isEmpty())
            {
                _out << contact->penalty_factor;
                _out << "\r\n";
            }
        }
        else
        {
            if (contact->overclosure == "硬接触")
            {
                _out << ", pressurecurve = hard";
            }
            else if (contact->overclosure == "法向不分离接触")
            {
                _out << ", pressurecurve = normaltie";
            }
            else if (contact->overclosure == "完全绑定接触")
            {
                _out << ", pressurecurve = tie";
            }

            if (contact->constraint == "罚函数法")
            {
                _out << ", type = penalty";
                if (!contact->scale_factor.isEmpty())
                {
                    _out << QString(", scalefactor = %1").arg(contact->scale_factor);
                }
            }
            else if (contact->constraint == "增广-摄动拉格朗日乘子法")
            {
                _out << ", type = augment";
            }
            else if (contact->constraint == "多点约束法")
            {
                _out << ", type = mpc";
            }

            _out << "\r\n";
        }
    }
    void mOMxFile::writeFriction(MechContactData::mContactBase* contact)
    {
        if (!contact->tangential_define)
            return;

        _out << "*friction";
        if (contact->tangential_friction == "通用接触摩擦")
        {
            _out << ", type = general";
            _out << "\r\n";
            _out << QString("%1\r\n").arg(contact->tangential_static_coef);
        }
        else if (contact->tangential_friction == "无摩擦")
        {
            _out << ", type = frictionless";
            _out << "\r\n";
        }
        else if (contact->tangential_friction == "粗糙")
        {
            _out << ", type = rough";
            _out << "\r\n";
        }
    }
    void mOMxFile::writeGapconductance(MechContactData::mContactBase* contact)
    {
        if (!contact->heat_define)
            return;

        _out << "*gapconductance\r\n";
        for (auto value : contact->heat_value)
        {
            _out << QString("%1, %2\r\n").arg(value.first).arg(value.second);
        }
    }
    void mOMxFile::writeAdjust(MechContactData::mContactBase* contact)
    {
        if (!contact->geom_define)
            return;

        _out << "*adjust\r\n";
        _out << QString("%1\r\n").arg(contact->geom_value);
    }
    void mOMxFile::writeClearance(MechContactData::mContactBase* contact)
    {
        if (!contact->gap_define)
            return;

        _out << "*clearance\r\n";
        _out << QString("%1\r\n").arg(contact->gap_overclosure);
    }
    bool  mOMxFile::writeLoadData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_LoadBoundary);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            shared_ptr<MechCaseData::mLoadBoundaryBase> load = _modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(id);
            if (!load || load->isSuppressed())
                continue;

            switch (load->getLoadBoundaryType())
            {
            case LOAD_IM_FORCE:
            case LOAD_IM_TORQUE:
                writeNodalload(load);
                break;
            case LOAD_IM_PRESSURE:
                writePressure(load);
                break;
            case LOAD_IM_GRAVITY:
                readBodyload(load);
                break;
            case LOAD_IM_HEATFLUX:
                writeHeatFlux(load);
                break;
            case LOAD_IM_RADIATION:
                writeRadiation(load);
                break;
            case LOAD_IM_CONVECTION:
                writeConvection(load);
                break;
            case LOAD_IM_TRANSIENT:
                writeTransient(load);
                break;
            case LOAD_IM_FREQDOMAIN:
                writeFreqdomain(load);
                break;
            case LOAD_IM_COMBINED:
                writeCombined(load);
                break;
            case LOAD_IM_ACOCONCFORCE:
                writeAcousticConcForce(load);
                break;
            case LOAD_IM_ACOFREQDOMAIN:
                writeAcousticFrequencyDomain(load);
                break;
            case LOAD_IM_SPECTRUM:
                writeSpectrum(load);
                break;
            case LOAD_IM_BOLT:
                writeBoltload(load);
                break;
            case LOAD_ROTATE_PHYSICAL_POWER:
                writeRotatePower(load);
                break;
            case LOAD_IM_SURFACE:
                writeSurface(load);
                break;
            case LOAD_IM_EDGE:
                writeEdgeLoad(load);
                break;
            default:
                break;
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("载荷写文件完成");
        return true;
    }
    void mOMxFile::writeNodalload(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadForce> load = dynamic_pointer_cast<MechCaseData::mLoadForce>(loadBase);
        if (!load)
        {
            return;
        }

        bool isTorque = (load->getLoadBoundaryType() == LOAD_IM_TORQUE);
        // 分析场方式：写 field 关键字，无作用对象/大小/数据行
        bool isFieldMode = load->isAnalysisFieldMode_;

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Set);
        QString setName = "";

        // 集中力：独立赋值(way_==2)/分析场不需要集合，其余模式（各向分量/方向与大小/文件）生成集合
        // 力矩：独立赋值(way_==1)不需要集合，其余模式（各向分量/文件）生成集合
        if (isTorque)
        {
            if (load->way_ != 1 && !isFieldMode)
                setName = writeGeoSet(setID, Node);
        }
        else
        {
            if (load->way_ != 2 && !isFieldMode)
                setName = writeGeoSet(setID, Node);
        }

        // 写*nodalload行头
        _out << "*nodalload";
        if (isTorque)
            _out << ", type = moment";
        else
            _out << ", type = force";

        _out << QString(", name = \"%1\"").arg(load->getLoadBoundaryName());

        // 文件模式不写函数和坐标系映射（与老版一致）；分析场模式写函数/坐标系/分析场映射
        if (load->path_.isEmpty())
        {
            writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
            writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Coordinate);
        }
        if (isFieldMode)
        {
            writeMapIntDataField(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Field, MapRelation_AnalysisField);
        }

        // 模式索引说明：
        // 集中力：0=各向分量, 1=方向与大小, 2=独立赋值, 3=文件
        // 力矩：  0=各向分量, 1=独立赋值, 2=文件
        if (isFieldMode) // 分析场：无数据行
        {
            _out << "\r\n";
        }
        else if (!load->path_.isEmpty()) // 文件定义（两种载荷通用判断：路径非空即为文件模式）
        {
            QFileInfo info(load->path_);
            _out << QString(", file = \"%1\"").arg(info.fileName());
            _out << "\r\n";

            // 拷贝文件到输出目录
            QFileInfo outInfo(_outputFileName);
            QString outPath = outInfo.absolutePath() + "/" + info.fileName();
            if (!QFile::exists(outPath))
            {
                bool ok = QFile::copy(load->path_, outPath);
                if (!ok)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2,
                        QString("%1 %2 中文件 %3 不存在或拷贝失败")
                        .arg(isTorque ? "力矩" : "集中力")
                        .arg(load->getLoadBoundaryName())
                        .arg(load->path_));
                }
            }
        }
        else if ((isTorque && load->way_ == 1) || (!isTorque && load->way_ == 2)) // 独立（离散）赋值
        {
            _out << "\r\n";
            // 遍历统一数据结构，每个非空方向写一行
            for (auto nodeIt = load->nodeForceMap_.begin(); nodeIt != load->nodeForceMap_.end(); ++nodeIt)
            {
                int nodeId = nodeIt->first.result_id;
                const QVector<QVariant>& values = nodeIt->second;

                for (int i = 0; i < values.size(); ++i)
                {
                    const QVariant& value = values[i];
                    // 0值是有效值，仅空值不写入
                    if (value.isNull() || !value.isValid())
                        continue;

                    int freedom = i + 1; // 1=X, 2=Y, 3=Z
                    // 力矩离散型数据：4=X, 5=Y, 6=Z
                    if (isTorque)
                        freedom += 3;

                    _out << QString("%1, %2, %3\r\n").arg(nodeId).arg(freedom)
                        .arg(mUnitIoCodec::encode(isTorque ? MxFunctions::Unit::Torque : MxFunctions::Unit::Force, value.toDouble()));
                }
            }
        }
        else // 统一赋值（集中力包含各向分量+方向与大小；力矩仅包含各向分量）
        {
            _out << QString(", set = \"%1\"").arg(setName);
            _out << "\r\n";

            // 统一写入三个分量
            // 各向分量模式：直接存储用户输入的分量值
            // 方向与大小模式：storeLoadValue 中已自动计算并写入 force_x/y/z
            // 两种模式输出格式完全一致，下游无需区分
            //单位制换算：内部 → 当前界面单位制（力/力矩），空值原样保留
            const MxFunctions::Unit loadUnit = isTorque ? MxFunctions::Unit::Torque : MxFunctions::Unit::Force;
            auto encodeVar = [](const QVariant& v, const MxFunctions::Unit& u)->QVariant {
                if (v.isValid() && !v.toString().simplified().isEmpty())
                    return QVariant(mUnitIoCodec::encode(u, v.toDouble()));
                return v;
            };
            QVariant fx = encodeVar(load->force_x, loadUnit);
            QVariant fy = encodeVar(load->force_y, loadUnit);
            QVariant fz = encodeVar(load->force_z, loadUnit);
            writeQVariantValue(fx);
            _out << ", ";
            writeQVariantValue(fy);
            _out << ", ";
            writeQVariantValue(fz);
            _out << "\r\n";
        }
    }

    void mOMxFile::writePressure(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadPressure> load = dynamic_pointer_cast<MechCaseData::mLoadPressure>(loadBase);
        if (!load)
        {
            return;
        }

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,load->getLoadBoundaryId() }, Physical_Set);
        QString setName = "";
        // 分析场（仅常规压力）：无作用对象，不写 set
        bool isFieldMode = (load->pressureType_ == 0 && load->isAnalysisFieldMode_);
        if ((load->pressureType_ == 0 && load->way_ == 0 && !isFieldMode) || load->pressureType_ == 1)
        {
            setName = writeGeoSet(setID, ElementSurface);
        }

        if (load->pressureType_ == 1)
        {
            // 净水压力
            _out << QString("*surfaceload, type = hydrostatic, name = \"%1\"").arg(load->getLoadBoundaryName());
            _out << QString(", set = \"%1\"").arg(setName);
            writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
            _out << "\r\n";

            QVector3D dir = load->depthDirection_;
            if (dir.isNull())
                dir = QVector3D(0, 0, 1);

            //单位制换算：内部 → 当前界面单位制（参考压力/深度）
            auto encodeVar = [](const QVariant& v, const MxFunctions::Unit& u)->QVariant {
                if (v.isValid() && !v.toString().simplified().isEmpty())
                    return QVariant(mUnitIoCodec::encode(u, v.toDouble()));
                return v;
            };
            QVariant refP = encodeVar(load->refPressure_, MxFunctions::Unit::Stress);
            QVariant zD = encodeVar(load->zeroDepth_, MxFunctions::Unit::Length);
            QVariant rD = encodeVar(load->refDepth_, MxFunctions::Unit::Length);
            writeQVariantValue(refP);
            _out << ", ";
            writeQVariantValue(zD);
            _out << ", ";
            writeQVariantValue(rD);
            _out << QString(", %1, %2, %3\r\n")
                .arg(dir.x())
                .arg(dir.y())
                .arg(dir.z());
            return;
        }

        _out << QString("*surfaceload, type = pressure, name = \"%1\"").arg(load->getLoadBoundaryName());

        if (isFieldMode)
        {
            // 分析场：写函数与分析场映射，无 set、无数据行
            writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
            writeMapIntDataField(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Field, MapRelation_AnalysisField);
            _out << "\r\n";
        }
        else if (load->way_ == 0)
        {
            // 统一赋值：带set参数
            _out << QString(", set = \"%1\"").arg(setName);

            writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
            _out << "\r\n";

            //仅写压强值（单位制换算：内部 → 当前界面单位制）
            auto encodeVar = [](const QVariant& v, const MxFunctions::Unit& u)->QVariant {
                if (v.isValid() && !v.toString().simplified().isEmpty())
                    return QVariant(mUnitIoCodec::encode(u, v.toDouble()));
                return v;
            };
            QVariant pa = encodeVar(load->pa_, MxFunctions::Unit::Stress);
            writeQVariantValue(pa);
            _out << "\r\n";
        }
        else if (load->way_ == 1)
        {
            // 独立赋值：不带set参数
            writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
            _out << "\r\n";

            // 遍历单元面Map，面编号前加B
            for (const auto& entry : load->cellFaceMap_)
            {
                int cellId = entry.first.result_id;
                int faceId = entry.first.result_id2;
                QVariant pressure;
                if (!entry.second.isEmpty() && entry.second[0].isValid() && !entry.second[0].isNull())
                    pressure = entry.second[0];

                _out << QString("%1, B%2, ").arg(cellId).arg(faceId);
                //单位制换算：内部 → 当前界面单位制（压强）
                if (pressure.isValid() && !pressure.toString().simplified().isEmpty())
                {
                    pressure = QVariant(mUnitIoCodec::encode(MxFunctions::Unit::Stress, pressure.toDouble()));
                }
                writeQVariantValue(pressure);
                _out << "\r\n";
            }
        }
        else if (load->way_ == 2)
        {
            // 文件赋值
            QFileInfo info(load->path_);
            _out << QString(", file = \"%1\"").arg(info.fileName());
            writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
            _out << "\r\n";

            // 拷贝文件到输出目录
            QFileInfo outInfo(_outputFileName);
            QString outPath = outInfo.absolutePath() + "/" + info.fileName();
            if (!QFile::exists(outPath))
            {
                bool ok = QFile::copy(load->path_, outPath);
                if (!ok)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2,
                        QString("压强 %1 中文件 %2 不存在或拷贝失败")
                        .arg(load->getLoadBoundaryName())
                        .arg(load->path_));
                }
            }
        }
    }
    void mOMxFile::readBodyload(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadGravity> load = dynamic_pointer_cast<MechCaseData::mLoadGravity>(loadBase);
        if (!load)
        {
            return;
        }

        QString setName = "";
        if (load->isCornerAddChecked)
        {
            int setID = _modelData->getMapRelationManager().getMapRelation<int>(
                { Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Set);

            setName = writeGeoSet(setID, Element);
        }

        if (!load->isCornerAddChecked)
            _out << QString("*bodyload, type = gravity, name = \"%1\"").arg(load->getLoadBoundaryName());
        else
        {
            _out << QString("*bodyload, type = gravity, name = \"%1\", set = \"%2\"")
                .arg(load->getLoadBoundaryName()).arg(setName);
        }
        writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
        _out << "\r\n";

        // 统一出口：V1/V2/V3 已在保存时由各模式换算完成
        writeQVariantValue(load->V1);
        _out << ", ";
        writeQVariantValue(load->V2);
        _out << ", ";
        writeQVariantValue(load->V3);
        _out << "\r\n";
    }
    void mOMxFile::writeHeatFlux(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadHeatFlux> load = dynamic_pointer_cast<MechCaseData::mLoadHeatFlux>(loadBase);
        if (!load)
        {
            return;
        }

        int setID = _modelData->getMapRelationManager().getMapRelation<int>(
            { Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Set);

        QString setName = "";
        if (load->defineType_ == 0)
        {
            if (load->way_ == 0)
                setName = writeGeoSet(setID, Node);
            else if (load->way_ == 1)
                setName = writeGeoSet(setID, ElementSurface);
            else
                setName = writeGeoSet(setID, Element);
        }
        //写行头
        if (load->way_ == 0) //点热流
        {
            _out << "*nodalflux";
        }
        else if (load->way_ == 1) //面热流
        {
            _out << "*surfaceload, type = flux";
        }
        else if (load->way_ == 2) //体热流
        {
            _out << "*bodyload, type = internalheat";
        }

        _out << QString(", name = \"%1\"").arg(load->getLoadBoundaryName());
        writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
        writeMapIntDataField(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Field, MapRelation_AnalysisField);

        //统一赋值带set参数，独立赋值不带
        if (load->defineType_ == 0)
        {
            _out << QString(", set = \"%1\"").arg(setName);
        }

        _out << "\r\n";

        if (load->defineType_ == 0)
        {
            // 统一赋值：写单个数值
            writeQVariantValue(load->pa_);
            _out << "\r\n";
        }
        else if (load->defineType_ == 2)
        {
            // 分析场：无数据行
        }
        else
        {
            // 独立赋值：遍历cellFaceData_写多行
            for (const auto& entry : load->cellFaceData_)
            {
                int mainId = entry.first.result_id;
                int subId = entry.first.result_id2;
                QVariant val;
                if (!entry.second.empty())
                    val = entry.second.front();

                if (load->way_ == 0) // 点热流格式：NID, VAL
                {
                    _out << QString("%1, ").arg(mainId);
                    writeQVariantValue(val);
                    _out << "\r\n";
                }
                else if (load->way_ == 1) // 面热流格式：EID, Bi, VAL（自动加B）
                {
                    _out << QString("%1, B%2, ").arg(mainId).arg(subId);
                    writeQVariantValue(val);
                    _out << "\r\n";
                }
                else if (load->way_ == 2) // 体热流格式：EID, VAL
                {
                    _out << QString("%1, ").arg(mainId);
                    writeQVariantValue(val);
                    _out << "\r\n";
                }
            }
        }
    }
    void mOMxFile::writeRadiation(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadRadiation> load = dynamic_pointer_cast<MechCaseData::mLoadRadiation>(loadBase);
        if (!load)
        {
            return;
        }

        int setID = _modelData->getMapRelationManager().getMapRelation<int>(
            { Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Set);
        QString setName = "";
        if (load->defineType_ == 0)
        {
            setName = writeGeoSet(setID, ElementSurface);
        }

        _out << QString("*surfaceload, type = radiation");
        _out << QString(", name = \"%1\"").arg(load->getLoadBoundaryName());

        // 统一赋值带set，独立赋值不带set
        if (load->defineType_ == 0)
        {
            _out << QString(", set = \"%1\"").arg(setName);
        }

        writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
        writeMapIntDataField(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Field, MapRelation_AnalysisField);
        _out << "\r\n";

        if (load->defineType_ == 0)
        {
            // 统一赋值：原有逻辑完全保留
            writeQVariantValue(load->V2);
            _out << ", ";
            writeQVariantValue(load->V1);
            _out << "\r\n";
        }
        else if (load->defineType_ == 2)
        {
            // 分析场：无数据行
        }
        else
        {
            // 独立赋值：输出 EID, Bi, e, t 格式，面编号自动加B
            for (const auto& entry : load->cellFaceData_)
            {
                int eid = entry.first.result_id;
                int fid = entry.first.result_id2;
                const QVector<QVariant>& values = entry.second;

                QVariant eVal = values.size() > 1 ? values[1] : QVariant();
                QVariant tVal = values.size() > 0 ? values[0] : QVariant();

                // 移除双参数有效性判断，强制输出所有条目
                _out << QString("%1, B%2, ").arg(eid).arg(fid);
                writeQVariantValue(eVal);
                _out << ", ";
                writeQVariantValue(tVal);
                _out << "\r\n";
            }

        }
    }
    void mOMxFile::writeConvection(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadConvection> load = dynamic_pointer_cast<MechCaseData::mLoadConvection>(loadBase);
        if (!load)
        {
            return;
        }

        int setID = _modelData->getMapRelationManager().getMapRelation<int>(
            { Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Set);
        QString setName = "";
        if (load->defineType_ == 0 || load->defineType_ == 2 || load->defineType_ == 3)
        {
            setName = writeGeoSet(setID, ElementSurface);
        }
        _out << QString("*surfaceload, type = conv");
        _out << QString(", name = \"%1\"").arg(load->getLoadBoundaryName());

        if (load->defineType_ == 0 || load->defineType_ == 2 || load->defineType_ == 3)
        {
            _out << QString(", set = \"%1\"").arg(setName);
        }

        if (load->defineType_ == 2)
        {
            // 文件定义
            QFileInfo info(load->path_);
            _out << QString(", file = \"%1\"").arg(info.fileName());
        }

        writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);
        writeMapIntDataConv(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function, MapRelationType::MapRelation_convFunction);
        writeMapIntDataField(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Field, MapRelation_AnalysisField);
        writeMapIntDataField(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Field, MapRelation_convAnalysisField);

        _out << "\r\n";

        if (load->defineType_ == 0)
        {
            writeQVariantValue(load->V2);
            _out << ", ";
            writeQVariantValue(load->V1);
            _out << "\r\n";
        }
        else if (load->defineType_ == 1)
        {
            // 独立赋值：输出 EID, Bi, h, t 格式，面编号自动加B
            for (const auto& entry : load->cellFaceData_)
            {
                int eid = entry.first.result_id;
                int fid = entry.first.result_id2;
                const QVector<QVariant>& values = entry.second;

                QVariant tVal = values.size() > 0 ? values[0] : QVariant();
                QVariant hVal = values.size() > 1 ? values[1] : QVariant();

                _out << QString("%1, B%2, ")
                    .arg(eid)
                    .arg(fid);
                writeQVariantValue(hVal); // 对流换热系数h
                _out << ", ";
                writeQVariantValue(tVal); // 环境温度t
                _out << "\r\n";
            }
        }
        else if (load->defineType_ == 3)
        {
            // 分析场：无数据行
        }
        else
        {
            writeQVariantValue(load->V1);
            _out << "\r\n";

            // 文件定义：拷贝文件到输出目录
            QFileInfo info(load->path_);
            QFileInfo outInfo(_outputFileName);
            QString outPath = outInfo.absolutePath() + "/" + info.fileName();
            if (!QFile::exists(outPath))
            {
                bool ok = QFile::copy(load->path_, outPath);
                if (!ok)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2,
                        QString("对流换热 %1 中文件 %2 不存在或拷贝失败")
                        .arg(load->getLoadBoundaryName())
                        .arg(load->path_));
                }
            }
        }
    }
    void mOMxFile::writeTransient(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadTransient> load = dynamic_pointer_cast<MechCaseData::mLoadTransient>(loadBase);
        if (!load)
        {
            return;
        }

        _out << QString("*tload, type = %1").arg(load->getSelectLoadType());
        _out << QString(", name = \"%1\"").arg(load->getLoadBoundaryName());
        _out << QString(", delay = ");
        writeQVariantValue(load->pa_);
        _out << "\r\n";
        int ID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,load->getLoadBoundaryId() }, Physical_LoadBoundary);
        QString loadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(ID);
        _out << "\"" << loadName << "\"";
        _out << "\r\n";
    }
    void mOMxFile::writeFreqdomain(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadFreqdomain> load = dynamic_pointer_cast<MechCaseData::mLoadFreqdomain>(loadBase);
        //写*rload, type
        if (!load)
        {
            return;
        }

        if (load->way2_ == 0)
        {
            _out << QString("*rload, type = %1").arg(load->getSelectLoadType());
        }
        else if (load->way2_ == 1)
        {
            _out << QString("*rload1, type = %1").arg(load->getSelectLoadType());
        }
        //写name delay xita
        _out << QString(", name = \"%1\"").arg(load->getLoadBoundaryName());
        _out << QString(", delay = ");
        writeQVariantValue(load->delay_);
        _out << QString(", xita = ");
        writeQVariantValue(load->phase_);
        _out << "\r\n";
        //写调用载荷名称（被引用载荷抑制时保留空字符串占位）
        int ID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,load->getLoadBoundaryId() }, Physical_LoadBoundary);
        auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(ID);
        QString loadName;
        if (refLoad && refLoad->isSuppressed())
        {
            loadName = QString();
        }
        else
        {
            loadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(ID);
        }
        _out << "\"" << loadName << "\"";
        _out << QString(",");
        //写第一个函数
        int id1 = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Function, MapRelationType::MapRelation_AmplitudeFunction);
        QString name1 = _modelData->getNameByPhyID(Physical_Function, id1);
        _out << "\"" << name1 << "\"";
        _out << QString(",");
        //写第二个函数
        int id2 = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Function, MapRelationType::MapRelation_PhaseFunction);
        if (id2 == -1)
        {
            _out << "\r\n";
        }
        else
        {
            QString name2 = _modelData->getNameByPhyID(Physical_Function, id2);
            _out << "\"" << name2 << "\"";
            _out << "\r\n";
        }
    }
    void mOMxFile::writeCombined(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadCombined> load = dynamic_pointer_cast<MechCaseData::mLoadCombined>(loadBase);
        //写*rload, type
        if (!load)
        {
            return;
        }

        _out << QString("*dload, name = \"%1\"").arg(load->getLoadBoundaryName());
        _out << QString(", S = ");
        writeQVariantValue(load->ktotal_);
        _out << "\r\n";
        //获取表格所有载荷名，和所有比例系数
        QList<int> loadIDs = _modelData->getMapRelationManager().getMapRelation<QList<int>>({ Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_LoadBoundary);
        QStringList loadNameList;
        QList<double> radioList;
        for (int i = 0; i < loadIDs.size(); ++i)
        {
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadIDs[i]);
            if (refLoad && refLoad->isSuppressed())
            {
                // 被引用载荷抑制时保留空行占位，保持表格行数与系数对齐
                loadNameList.push_back(QString());
                radioList.push_back(load->ratioList_[i]);
            }
            else
            {
                QString name = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(loadIDs[i]);
                loadNameList.push_back(name);
                radioList.push_back(load->ratioList_[i]);
            }
        }
        //获取到后，循环写入
        for (int i = 0; i < loadNameList.size(); ++i)
        {
            _out << "\"" << loadNameList[i] << "\"" << ",";
            _out << radioList[i];
            _out << "\r\n";
        }

    }
    void mOMxFile::writeAcousticConcForce(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadAcousticConcForce> load = dynamic_pointer_cast<MechCaseData::mLoadAcousticConcForce>(loadBase);
        //写*rload, type
        if (!load)
        {
            return;
        }

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,load->getLoadBoundaryId() }, Physical_Set);
        QString setName = writeGeoSet(setID, Node);

        _out << QString("*sload,name = \"%1\"").arg(load->getLoadBoundaryName());
        _out << QString(", set = \"%1\"").arg(setName);
        _out << "\r\n";
        writeQVariantValue(load->value_);
        _out << "\r\n";
    }
    void mOMxFile::writeAcousticFrequencyDomain(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadAcousticFrequencyDomain> load = dynamic_pointer_cast<MechCaseData::mLoadAcousticFrequencyDomain>(loadBase);
        //写*rload, type
        if (!load)
        {
            return;
        }

        _out << QString("*acsrce,type = load,name = \"%1\"").arg(load->getLoadBoundaryName());
        _out << QString(", delay = ");
        writeQVariantValue(load->delay_);
        _out << QString(", xita = ");
        writeQVariantValue(load->xita_);
        _out << QString(", rho = ");
        writeQVariantValue(load->rho_);
        _out << QString(", b = ");
        writeQVariantValue(load->B_);
        _out << "\r\n";
        //写调用载荷名称（被引用载荷抑制时保留空字符串占位）
        int ID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,load->getLoadBoundaryId() }, Physical_LoadBoundary);
        auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(ID);
        QString loadName;
        if (refLoad && refLoad->isSuppressed())
        {
            loadName = QString();
        }
        else
        {
            loadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(ID);
        }
        _out << "\"" << loadName << "\"";
        _out << QString(",");
        //写第一个函数
        int id = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Function);
        QString name = _modelData->getNameByPhyID(Physical_Function, id);
        _out << "\"" << name << "\"";
        _out << "\r\n";
    }
    void mOMxFile::writeSpectrum(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadSpectrum> load = dynamic_pointer_cast<MechCaseData::mLoadSpectrum>(loadBase);
        //写*rload, type
        if (!load)
        {
            return;
        }
        _out << QString("*ds");
        if (load->type_ == "位移D")
            _out << QString(", type = D, ");
        else if (load->type_ == "速度V")
            _out << QString(", type = V, ");
        else
            _out << QString(", type = A, ");
        _out << QString("name = \"%1\"").arg(load->getLoadBoundaryName()) << "\r\n";
        //获取表格所有普曲线和阻尼值
        QList<QString> list = load->dampValueList_;
        QList<QString> spectrumCurveList;
        QList<int> funcIDs = _modelData->getMapRelationManager().getMapRelation<QList<int>>({ Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Function);
        for (int i = 0; i < funcIDs.size(); ++i)
        {
            QString name = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>((Physical_Function)).getFunctionNameByID(funcIDs[i]);
            spectrumCurveList.push_back(name);
        }
        if (spectrumCurveList.size() != load->dampValueList_.size())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("读取到的谱载荷数据存在错误！"));
            if (spectrumCurveList.size() > load->dampValueList_.size())
            {
                list = load->dampValueList_;
            }
            else
            {
                list = spectrumCurveList;
            }
        }
        //获取到后，循环写入
        for (int i = 0; i < list.size(); ++i)
        {
            _out << "\"" << spectrumCurveList[i] << "\"";
            _out << QString(", ");
            _out << load->dampValueList_[i];

            _out << "\r\n";
        }

    }

    void mOMxFile::writeBoltload(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadBolt> load = dynamic_pointer_cast<MechCaseData::mLoadBolt>(loadBase);
        if (!load)
        {
            return;
        }

        int nid = -1;
        int loadID = load->getLoadBoundaryId();
        int connectID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary ,loadID }, Physical_Connector);
        std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase = _modelData->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(connectID);
        if (connectorBase && connectorBase->isSuppressed())
        {
            return;
        }
        shared_ptr<MechConnectorData::mConnectorBolt> connect = dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(connectorBase);
        if (connect)
        {
            nid = connect->node_;
        }

        if (load->way_ == 0)
        {
            _out << QString("*nodalload, name = \"%1\", boltload = on").arg(load->getLoadBoundaryName());
            writeMapIntData(Physical_LoadBoundary, loadID, Physical_Function);
            _out << "\r\n";

            _out << QString("%1, 1, ").arg(nid);
            writeQVariantValue(load->value_);
            _out << "\r\n";
        }
        else if (load->way_ == 1)
        {
            _out << QString("*spc, name = \"%1\", boltload = on").arg(load->getLoadBoundaryName());
            writeMapIntData(Physical_LoadBoundary, loadID, Physical_Function);
            _out << "\r\n";

            _out << QString("%1, 1, 1, ").arg(nid);
            writeQVariantValue(load->value_);
            _out << "\r\n";
        }
        else if (load->way_ == 2)
        {
            _out << QString("*spc, name = \"%1\", boltload = on").arg(load->getLoadBoundaryName());
            _out << QString(", fixed = on");
            _out << "\r\n";

            _out << QString("%1, 1, 1, 0").arg(nid);
            _out << "\r\n";
        }
    }


    void mOMxFile::writeRoACC(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadRotationalACC> load
            = dynamic_pointer_cast<MechCaseData::mLoadRotationalACC>(loadBase);
        if (!load)
        {
            return;
        }

        QString typeName = "constant";
        if (load->_accCurveType == 1)
            typeName = "time";

        _out << QString("*domega, type = %1").arg(typeName);
        _out << QString(", name = \"%1\"").arg(load->getRotateAnalyseName());
        QString part = load->_rotationalPart;
        if (_modelData)
        {
            std::set<int> setIDs = _modelData->getMapRelationManager().getMapRelation<std::set<int>>(
                { Physical_RotateAnalyse, load->getRotateAnalyseId() }, Physical_Set);
            if (!setIDs.empty())
            {
                int setID = *setIDs.begin();
                part = _modelData->getNameByPhyID(Physical_Set, setID);
            }
        }
        if (!part.isEmpty())
            _out << QString(", part=\"%1\"").arg(part);
        _out << "\r\n";

        // 按行写出数据：将标量旋转加速度转换为 omega 向量后输出。
        // 时间曲线第一列为时间，第二列为加速度；其余类型第一列为加速度。
        // 每行格式：数据列... , omegaX, omegaY, omegaZ, startX, startY, startZ
        const QList<QStringList>& tableData = (load->_accCurveType == 1)
            ? load->_timeCurveTableData
            : load->_constantTableData;
        QVector3D vStart = load->_startPoint1;
        QVector3D vDire = load->_directPoint2 - load->_startPoint1;
        if (!vDire.isNull())
            vDire.normalize();

        for (const QStringList& row : tableData)
        {
            if (row.isEmpty())
                continue;
            // 计算当前行的 omega 分量（方向向量乘以加速度值）。
            double accValue = 0.0;
            int accCol = (load->_accCurveType == 1) ? 1 : 0;
            if (row.size() > accCol)
            {
                QString accStr = row[accCol].trimmed();
                bool ok = false;
                accValue = accStr.toDouble(&ok);
                if (!ok)
                    accValue = 0.0;
            }

            // 仅时间曲线保留时间列
            if (load->_accCurveType == 1)
            {
                if (row.size() > 0)
                    _out << row[0].trimmed();
                _out << ", ";
            }

            QVector3D omega = vDire * accValue;
            _out << QString("%1, %2, %3").arg(omega.x(), 0, 'g', 16).arg(omega.y(), 0, 'g', 16).arg(omega.z(), 0, 'g', 16);
            _out << QString(", %1, %2, %3").arg(vStart.x(), 0, 'g', 16).arg(vStart.y(), 0, 'g', 16).arg(vStart.z(), 0, 'g', 16);
            _out << "\r\n";
        }
    }

    //旋转速度
    void mOMxFile::writeRoSpeed(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> loadBase)
    {
        shared_ptr<MechCaseData::mLoadRotationalSpeed> load
            = dynamic_pointer_cast<MechCaseData::mLoadRotationalSpeed>(loadBase);
        if (!load)
        {
            return;
        }

        QString typeName = "constant";
        if (load->_speedCurveType == 1)
            typeName = "table";
        else if (load->_speedCurveType == 2)
            typeName = "time";

        _out << QString("*omega, type = %1").arg(typeName);
        _out << QString(", name = \"%1\"").arg(load->getRotateAnalyseName());
        QString part = load->_rotationalPart;
        if (_modelData)
        {
            std::set<int> setIDs = _modelData->getMapRelationManager().getMapRelation<std::set<int>>(
                { Physical_RotateAnalyse, load->getRotateAnalyseId() }, Physical_Set);
            if (!setIDs.empty())
            {
                int setID = *setIDs.begin();
                part = _modelData->getNameByPhyID(Physical_Set, setID);
            }
        }
        if (!part.isEmpty())
            _out << QString(", part=\"%1\"").arg(part);
        _out << "\r\n";

        // 按行写出数据：将标量旋转速度转换为 omega 向量后输出。
        // 时间曲线第一列为时间，第二列为速度；其余类型第一列为速度。
        // 每行格式：数据列... , omegaX, omegaY, omegaZ, startX, startY, startZ
        const QList<QStringList>* tableData = &load->_constantTableData;
        if (load->_speedCurveType == 1)
            tableData = &load->_curveTableData;
        else if (load->_speedCurveType == 2)
            tableData = &load->_timeCurveTableData;
        QVector3D vStart = load->_startPoint1;
        QVector3D vDire = load->_directPoint2 - load->_startPoint1;
        if (!vDire.isNull())
            vDire.normalize();

        for (const QStringList& row : *tableData)
        {
            if (row.isEmpty())
                continue;
            // 计算当前行的 omega 分量（方向向量乘以速度值）。
            double speedValue = 0.0;
            int speedCol = (load->_speedCurveType == 2) ? 1 : 0;
            if (row.size() > speedCol)
            {
                QString speedStr = row[speedCol].trimmed();
                bool ok = false;
                speedValue = speedStr.toDouble(&ok);
                if (!ok)
                    speedValue = 0.0;
            }

            // 仅时间曲线保留时间列
            if (load->_speedCurveType == 2)
            {
                if (row.size() > 0)
                    _out << row[0].trimmed();
                _out << ", ";
            }

            QVector3D omega = vDire * speedValue;
            _out << QString("%1, %2, %3").arg(omega.x(), 0, 'g', 16).arg(omega.y(), 0, 'g', 16).arg(omega.z(), 0, 'g', 16);
            _out << QString(", %1, %2, %3").arg(vStart.x(), 0, 'g', 16).arg(vStart.y(), 0, 'g', 16).arg(vStart.z(), 0, 'g', 16);
            _out << "\r\n";
        }
    }


    void mOMxFile::writeRotateAnalyse()
    {
        if (!_modelData)
            return;

        MechCaseData::mRotateAnalyseManager& manager =
            _modelData->getManager<MechCaseData::mRotateAnalyseManager>(Physical_RotateAnalyse);

        std::vector<int> ids = manager.getAllRotateAnalyseID();
        for (int id : ids)
        {
            std::shared_ptr<MechCaseData::mRoatateAnalyseBase> load = manager.getRotateAnalyseData(id);
            if (!load)
                continue;

            // 已被任意复模态转子动力学工况映射的旋转分析在 writeImplicitModal 中按工况输出，
            // 此处跳过，避免重复。
            std::set<int> caseIDs = _modelData->getMapRelationManager().getMapRelation<std::set<int>>(
                { Physical_RotateAnalyse, id }, Physical_Case);
            bool mappedToRotorCase = false;
            for (int caseID : caseIDs)
            {
                MechCaseData::mCaseBase* caseBase = _modelData->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(caseID);
                if (!caseBase || caseBase->getCaseType() != ImplicitModal)
                    continue;
                MechCaseData::mCaseImplicitModal* modalData = dynamic_cast<MechCaseData::mCaseImplicitModal*>(caseBase);
                if (modalData && modalData->_modalType == "复模态分析" && modalData->_rotationalChecked)
                {
                    mappedToRotorCase = true;
                    break;
                }
            }
            if (mappedToRotorCase)
                continue;

            switch (load->getRotateAnalyseType())
            {
            case ROTATIONAL_ACC:
                writeRoACC(load);
                break;
            case ROTATIONAL_SPEED:
                writeRoSpeed(load);
                break;
            case ROTATIONAL_FORCE:
                // TODO: 旋转力输出
                break;
            default:
                break;
            }
        }
    }

    void mOMxFile::writeRotatePower(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        shared_ptr<MechCaseData::mRotatePhysicalPower> load
            = dynamic_pointer_cast<MechCaseData::mRotatePhysicalPower>(loadBase);
        if (!load)
        {
            return;
        }

        // 根据类型映射关键字名称
        QString typeName;
        if (load->_rotateType == 0)
        {
            typeName = "centrifugalload";
        }
        else if (load->_rotateType == 1)
        {
            typeName = "rotaryaccelerationload";
        }

        int setID = _modelData->getMapRelationManager().getMapRelation<int>(
            { Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Set);

        // 统一赋值输出 set，独立赋值不输出 set
        QString setName;
        //if (load->_defineType == 0)
        {
            setName = writeGeoSet(setID, Element);
        }

        _out << QString("*bodyload, type = %1, name = \"%2\"").arg(typeName).arg(load->getLoadBoundaryName());
        if (!setName.isEmpty())
        {
            _out << QString(", set = \"%1\"").arg(setName);
        }
        writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);

        _out << "\r\n";

        if (load->_defineType == 0)
        {
            // 统一赋值：Val, X1, Y1, Z1, X2, Y2, Z2
            // 离心力类型时，内部按 rad/s 存储转速，输出时直接平方
            QVariant outValue = load->_uniformValue;
            if (load->_rotateType == 0 && outValue.isValid() && !outValue.isNull())
            {
                double speedRads = outValue.toDouble();
                outValue = QVariant::fromValue<double>(speedRads * speedRads);
            }
            writeQVariantValue(outValue);
            _out << QString(", %1, %2, %3, %4, %5, %6\r\n")
                .arg(load->_startPoint1.x())
                .arg(load->_startPoint1.y())
                .arg(load->_startPoint1.z())
                .arg(load->_directPoint2.x())
                .arg(load->_directPoint2.y())
                .arg(load->_directPoint2.z());
        }
        else
        {
            // 独立赋值：每行 EID, Val, X1, Y1, Z1, X2, Y2, Z2
            // _elementCoorMap 内部存储为 [Val(QVariant::double), 起始点(QVector3D), 方向点(QVector3D)]
            for (const auto& entry : load->_elementCoorMap)
            {
                int elementId = entry.first.result_id;
                const QVector<QVariant>& values = entry.second;

                QString val;
                if (values.size() > 0 && values[0].isValid() && !values[0].isNull())
                    val = QString::number(values[0].toDouble());

                QVector3D startPoint = QVector3D(0, 0, 0);
                if (values.size() > 1 && values[1].isValid() && !values[1].isNull())
                    startPoint = values[1].value<QVector3D>();

                QVector3D directPoint = QVector3D(0, 0, 1);
                if (values.size() > 2 && values[2].isValid() && !values[2].isNull())
                    directPoint = values[2].value<QVector3D>();

                _out << QString("%1, %2, %3, %4, %5, %6, %7, %8\r\n")
                    .arg(elementId)
                    .arg(val)
                    .arg(startPoint.x())
                    .arg(startPoint.y())
                    .arg(startPoint.z())
                    .arg(directPoint.x())
                    .arg(directPoint.y())
                    .arg(directPoint.z());
            }
        }
    }

    void mOMxFile::writeSurface(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadSurface>(loadBase);
        if (!load)
            return;

        // 1. 获取集合名称（仅统一赋值有效）
        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Set);

        QString setName;
        if (load->way_ == 0) // 统一赋值
        {
            setName = writeGeoSet(setID, ElementSurface);
        }

        // 2. 写行头
        _out << "*surfaceload, type = pressure";
        _out << QString(", name = \"%1\"").arg(load->getLoadBoundaryName());
        writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function);

        // 统一赋值追加set参数
        if (load->way_ == 0)
        {
            _out << QString(", set = \"%1\"").arg(setName);
        }
        QString dirStr = (load->dirMode_ == 0) ? "shear" : "trvec";
        _out << QString(", direction = %1").arg(dirStr);

        _out << "\r\n";

        // 3. 写数值内容
        if (load->way_ == 0)
        {
            // ========== 统一赋值：单行 Val,V1,V2,V3 ==========
            QString valStr;
            if (load->loadValue_.isValid() && !load->loadValue_.isNull())
            {
                valStr = QString::number(load->loadValue_.toDouble());
            }

            QString v1Str, v2Str, v3Str;
            QVariant dirVar;
            const auto& state = load->dirPickState_;
            if (state.mode == 1) dirVar = QVariant::fromValue(QVector3D(1, 0, 0));
            else if (state.mode == 2) dirVar = QVariant::fromValue(QVector3D(0, 1, 0));
            else if (state.mode == 3) dirVar = QVariant::fromValue(QVector3D(0, 0, 1));
            else if (state.mode == 6) dirVar = QVariant::fromValue(state.directInputDir);
            else if ((state.mode == 4 || state.mode == 5) && state.startPoint.isValid() && state.endPoint.isValid())
            {
                QVector3D start = state.startPoint.value<QVector3D>();
                QVector3D end = state.endPoint.value<QVector3D>();
                dirVar = QVariant::fromValue(end - start);
            }

            if (dirVar.isValid() && !dirVar.isNull())
            {
                QVector3D dir = dirVar.value<QVector3D>();
                if (!dir.isNull())
                {
                    if (state.reversed) dir = -dir;
                    dir.normalize();
                    v1Str = QString::number(dir.x());
                    v2Str = QString::number(dir.y());
                    v3Str = QString::number(dir.z());
                }
            }

            // 空值保留逗号占位
            _out << QString("%1,%2,%3,%4").arg(valStr).arg(v1Str).arg(v2Str).arg(v3Str);
            _out << "\r\n";
        }
        else
        {
            // ========== 独立赋值：逐行输出 ==========
            for (const auto& entry : load->surfaceForceMap_)
            {
                int elemId = entry.first.result_id;
                int faceId = entry.first.result_id2;
                const QVector<QVariant>& values = entry.second;

                auto variantToString = [](const QVariant& v) -> QString {
                    if (!v.isValid() || v.isNull()) return "";
                    return QString::number(v.toDouble());
                    };

                QString valStr = values.size() > 0 ? variantToString(values[0]) : "";
                QString v1Str = values.size() > 1 ? variantToString(values[1]) : "";
                QString v2Str = values.size() > 2 ? variantToString(values[2]) : "";
                QString v3Str = values.size() > 3 ? variantToString(values[3]) : "";

                // 格式：EID, B<面号>, Val,V1,V2,V3
                _out << QString("%1, B%2, %3,%4,%5,%6")
                    .arg(elemId)
                    .arg(faceId)
                    .arg(valStr)
                    .arg(v1Str)
                    .arg(v2Str)
                    .arg(v3Str);
                _out << "\r\n";
            }
        }
    }

    void mOMxFile::writeEdgeLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase)
    {
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadEdge>(loadBase);
        if (!load)
            return;

        int setID = _modelData->getMapRelationManager().getMapRelation<int>(
            { Physical_LoadBoundary, load->getLoadBoundaryId() }, Physical_Set);
        QString setName = writeGeoSet(setID, ElementEdge);

        QString direct = load->getDirectionModeEng();

        // ========== 修正：头部字段按格式顺序拼接，修复重复name、direction赋值错误 ==========
        _out << QString("*edgeload, type = line, name = \"%1\"").arg(load->getLoadBoundaryName());
        _out << QString(", direction = \"%1\"").arg(direct);
        _out << QString(", set = \"%1\"").arg(setName);     // 集合必选，空值也写出
        writeMapIntData(Physical_LoadBoundary, load->getLoadBoundaryId(), Physical_Function); //函数可选为空不写
        _out << "\r\n";

        //数值行写入
        if (direct == "edld") // 通用模式：载荷值 + 方向矢量三分量
        {
            QString valStr = load->size_;
            // 方向矢量无效时默认补0
            double v1 = 0.0, v2 = 0.0, v3 = 0.0;
            if (load->dir_.isValid() && !load->dir_.isNull())
            {
                QVector3D dir = load->dir_.value<QVector3D>();
                v1 = dir.x();
                v2 = dir.y();
                v3 = dir.z();
            }
            _out << QString("%1, %2, %3, %4\r\n").arg(valStr).arg(v1).arg(v2).arg(v3);
        }
        else // 非通用模式：仅写入载荷值
        {
            _out << QString("%1\r\n").arg(load->size_);
        }
        _out << "\r\n";
    }

    bool mOMxFile::writeBoundaryData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_LoadBoundary);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            shared_ptr<MechCaseData::mLoadBoundaryBase> boundary = _modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(id);
            if (!boundary || boundary->isSuppressed())
                continue;

            switch (boundary->getLoadBoundaryType())
            {
            case BOUNDARY_IM_FIXED:
            case BOUNDARY_IM_DISPLACEMENT:
            case BOUNDARY_IM_SPEED:
            case BOUNDARY_IM_ACC:
            case BOUNDARY_IM_TEMP:
                writeSpc(boundary);
                break;
            case BOUNDARY_IM_VIRTUAL:
                writeSuport(boundary);
                break;
            case BOUNDARY_IM_SUPERELEMENT:
                writeSuperelement(boundary);
                break;
            case BOUNDARY_MOVE:
                writeBoundaryMove(boundary);
                break;
            case BOUNDARY_TEMP_NEW:
                writeBoundaryTempNew(boundary);
                break;
            default:
                break;
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("边界条件写文件完成");
        return true;
    }
    void mOMxFile::writeSpc(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase)
    {
        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,boundaryBase->getLoadBoundaryId() }, Physical_Set);
        // 温度边界分析场方式无作用对象，不写集合
        bool isTempFieldMode = false;
        if (boundaryBase->getLoadBoundaryType() == BOUNDARY_IM_TEMP)
        {
            auto tempBc = dynamic_pointer_cast<MechCaseData::mBoundaryTemp>(boundaryBase);
            isTempFieldMode = tempBc && tempBc->isAnalysisFieldMode_;
        }
        QString setName = isTempFieldMode ? QString() : writeGeoSet(setID, Node);

        _out << QString("*spc");

        if (boundaryBase->getLoadBoundaryType() == BOUNDARY_IM_SPEED)
        {
            _out << QString(", type = velocity");
        }
        else if (boundaryBase->getLoadBoundaryType() == BOUNDARY_IM_ACC)
        {
            _out << QString(", type = acceleration");
        }
        _out << QString(", name = \"%1\"").arg(boundaryBase->getLoadBoundaryName());
        if (boundaryBase->getLoadBoundaryType() == BOUNDARY_IM_FIXED)
        {
            _out << QString(", set = \"%1\"").arg(setName);
            writeMapIntData(Physical_LoadBoundary, boundaryBase->getLoadBoundaryId(), Physical_Function);
            _out << "\r\n";
            shared_ptr<MechCaseData::mBoundaryFixed> bc = dynamic_pointer_cast<MechCaseData::mBoundaryFixed>(boundaryBase);
            QHash<int, QString> hash_type = {
                {0, "encastre"},
                {1, "pinned"},
                {2, "xsymm"},
                {3, "ysymm"},
                {4, "zsymm"},
                {5, "xasymm"},
                {6, "yasymm"},
                {7, "zasymm"}
            };
            _out << QString("%1\r\n").arg(hash_type[bc->_fixedType]);
        }
        else if (boundaryBase->getLoadBoundaryType() == BOUNDARY_IM_DISPLACEMENT ||
            boundaryBase->getLoadBoundaryType() == BOUNDARY_IM_SPEED || boundaryBase->getLoadBoundaryType() == BOUNDARY_IM_ACC)
        {
            shared_ptr<MechCaseData::mBoundaryDisplacement> bc = dynamic_pointer_cast<MechCaseData::mBoundaryDisplacement>(boundaryBase);
            if (bc->_definitionMode == 1)
            {
                writeMapIntData(Physical_LoadBoundary, boundaryBase->getLoadBoundaryId(), Physical_Function);
                _out << "\r\n";
                for (const auto& entry : bc->_independentTable)
                {
                    int nodeId = entry.first.result_id;
                    const QVector<QVariant>& values = entry.second;
                    for (int dof = 1; dof <= 6 && dof <= values.size(); ++dof)
                    {
                        const QVariant& val = values[dof - 1];
                        if (!val.isValid() || val.isNull())
                            continue;
                        _out << QString("%1, %2, %3, %4\r\n")
                            .arg(nodeId)
                            .arg(dof)
                            .arg(dof)
                            .arg(val.toDouble());
                    }
                }
            }
            else
            {
                _out << QString(", set = \"%1\"").arg(setName);
                writeMapIntData(Physical_LoadBoundary, boundaryBase->getLoadBoundaryId(), Physical_Function);
                _out << "\r\n";
                if (bc->_U1.isValid())
                {
                    _out << QString("1, %1\r\n").arg(bc->_U1.toDouble());
                }
                if (bc->_U2.isValid())
                {
                    _out << QString("2, %1\r\n").arg(bc->_U2.toDouble());
                }
                if (bc->_U3.isValid())
                {
                    _out << QString("3, %1\r\n").arg(bc->_U3.toDouble());
                }
                if (bc->_UR1.isValid())
                {
                    _out << QString("4, %1\r\n").arg(bc->_UR1.toDouble());
                }
                if (bc->_UR2.isValid())
                {
                    _out << QString("5, %1\r\n").arg(bc->_UR2.toDouble());
                }
                if (bc->_UR3.isValid())
                {
                    _out << QString("6, %1\r\n").arg(bc->_UR3.toDouble());
                }
            }
        }
        else if (boundaryBase->getLoadBoundaryType() == BOUNDARY_IM_TEMP)
        {
            shared_ptr<MechCaseData::mBoundaryTemp> bc = dynamic_pointer_cast<MechCaseData::mBoundaryTemp>(boundaryBase);
            if (bc->isAnalysisFieldMode_)
            {
                // 分析场：写函数与分析场映射，无 set、无数据行
                writeMapIntData(Physical_LoadBoundary, boundaryBase->getLoadBoundaryId(), Physical_Function);
                writeMapIntDataField(Physical_LoadBoundary, boundaryBase->getLoadBoundaryId(), Physical_Field, MapRelation_AnalysisField);
                _out << "\r\n";
            }
            else
            {
                _out << QString(", set = \"%1\"").arg(setName);
                writeMapIntData(Physical_LoadBoundary, boundaryBase->getLoadBoundaryId(), Physical_Function);
                _out << "\r\n";
                _out << QString("7, %1\r\n").arg(bc->_temp);
            }
        }
    }

    void mOMxFile::writeBoundaryMove(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase)
    {
        shared_ptr<MechCaseData::mBoundaryMove> bc = dynamic_pointer_cast<MechCaseData::mBoundaryMove>(boundaryBase);
        if (!bc)
        {
            return;
        }

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,boundaryBase->getLoadBoundaryId() }, Physical_Set);
        QString setName = writeGeoSet(setID, Node);
        auto pickIt = bc->_pickData.find(MxFunctions::PickDataType::Node);
        if (setName.isEmpty() && pickIt != bc->_pickData.end() && !pickIt->second.empty())
        {
            setName = getRenameSet();
            _out << QString("*set, type = node, name = \"%1\"\r\n").arg(setName);
            writeSetData(pickIt->second);
        }

        _out << QString("*spc, type = submodeldisp, name = \"%1\"").arg(boundaryBase->getLoadBoundaryName());
        if (!setName.isEmpty())
            _out << QString(", set = \"%1\"").arg(setName);
        _out << "\r\n";

        // 提取文件绝对路径（与 writeNodalload 保持一致）
        QFileInfo info(bc->_mainModelFile);
        _out << QString("*mastermodel, type = mxdb, file = \"%1\", step = \"%2\"")
            .arg(info.absoluteFilePath()).arg(bc->_timeStep);
        if (bc->_useMainSet && !bc->_mainSet.isEmpty())
            _out << QString(", set = \"%1\"").arg(bc->_mainSet);
        _out << "\r\n";
    }

    void mOMxFile::writeBoundaryTempNew(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase)
    {
        shared_ptr<MechCaseData::mBoundaryTempNew> bc = dynamic_pointer_cast<MechCaseData::mBoundaryTempNew>(boundaryBase);
        if (!bc)
        {
            return;
        }

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,boundaryBase->getLoadBoundaryId() }, Physical_Set);
        QString setName = writeGeoSet(setID, Node);
        auto pickIt = bc->_pickData.find(MxFunctions::PickDataType::Node);
        if (setName.isEmpty() && pickIt != bc->_pickData.end() && !pickIt->second.empty())
        {
            setName = getRenameSet();
            _out << QString("*set, type = node, name = \"%1\"\r\n").arg(setName);
            writeSetData(pickIt->second);
        }

        _out << QString("*spc, type = submodeltemp, name = \"%1\"").arg(boundaryBase->getLoadBoundaryName());
        if (!setName.isEmpty())
            _out << QString(", set = \"%1\"").arg(setName);
        _out << "\r\n";

        // 提取文件名并拷贝到输出目录（与 writeNodalload 保持一致）
        QFileInfo info(bc->_mainModelFile);
        _out << QString("*mastermodel, type = mxdb, file = \"%1\", step = \"%2\"")
            .arg(info.absoluteFilePath()).arg(bc->_timeStep);
        if (bc->_useMainSet && !bc->_mainSet.isEmpty())
            _out << QString(", set = \"%1\"").arg(bc->_mainSet);
        _out << "\r\n";
    }

    void mOMxFile::writeSuport(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase)
    {
        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,boundaryBase->getLoadBoundaryId() }, Physical_Set);
        QString setName = writeGeoSet(setID, Node);

        _out << QString("*suport1, name = \"%1\"").arg(boundaryBase->getLoadBoundaryName());
        _out << QString(", set = \"%1\"").arg(setName);
        _out << "\r\n";

        shared_ptr<MechCaseData::mBoundaryVirtual> bc = dynamic_pointer_cast<MechCaseData::mBoundaryVirtual>(boundaryBase);
        if (bc == nullptr) return;
        if (bc->_U1)
        {
            _out << "1\r\n";
        }
        if (bc->_U2)
        {
            _out << "2\r\n";
        }
        if (bc->_U3)
        {
            _out << "3\r\n";
        }
        if (bc->_UR1)
        {
            _out << "4\r\n";
        }
        if (bc->_UR2)
        {
            _out << "5\r\n";
        }
        if (bc->_UR3)
        {
            _out << "6\r\n";
        }
    }

    void mOMxFile::writeSuperelement(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase)
    {
        shared_ptr<MechCaseData::mBoundarySuperelement> bc = dynamic_pointer_cast<MechCaseData::mBoundarySuperelement>(boundaryBase);

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_LoadBoundary,bc->getLoadBoundaryId() }, Physical_Set);
        QString setName = writeGeoSet(setID, Node);

        _out << QString("*constraint, type = %1, name = \"%2\"").arg(bc->_ConstraintType).arg(bc->getLoadBoundaryName());
        _out << QString(", set = \"%1\"").arg(setName);
        _out << "\r\n";

        if (bc == nullptr) return;
        if (bc->_DOF[0])
        {
            _out << "1\r\n";
        }
        if (bc->_DOF[1])
        {
            _out << "2\r\n";
        }
        if (bc->_DOF[2])
        {
            _out << "3\r\n";
        }
        if (bc->_DOF[3])
        {
            _out << "4\r\n";
        }
        if (bc->_DOF[4])
        {
            _out << "5\r\n";
        }
        if (bc->_DOF[5])
        {
            _out << "6\r\n";
        }
    }

    bool mOMxFile::writeConnectorData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Connector);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            shared_ptr<MechConnectorData::mConnectorBase> connector = _modelData->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(id);
            if (!connector || connector->isSuppressed())
                continue;

            switch (connector->getConnectorType())
            {
            case Connector_Rigid:
                writeConnectorRigid(connector);
                break;
            case Connector_Flexible:
                writeConnectorFlexible(connector);
                break;
            case Connector_Bind:
                writeConnectorBind(connector);
                break;
            case Connector_MPC:
                writeConnectorMPC(connector);
                break;
            case Connector_Disperse:
                writeConnectorDisperse(connector);
                break;
            case Connector_Rigidbody:
                writeConnectorRigidbody(connector);
                break;
            case Connector_Bolt:
                writePretension(connector);
                break;
            case Connector_DistributedCoupling:
                writeConnectorDistributedCoupling(connector);
                break;
            default:
                break;
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("连接写文件完成");
        return true;
    }
    bool mOMxFile::writeBearingSection()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Connector);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            shared_ptr<MechConnectorData::mConnectorBase> base = _modelData->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(id);
            shared_ptr<MechConnectorData::mConnectorBearingUnits> connect = dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(base);
            if (!base || base->isSuppressed())
                continue;

            switch (base->getConnectorType())
            {
            case Connector_BearingUnits:
                _out << QString("*section, type = structural, name = \"%1\"\r\n").arg(base->getConnectorName());

                if (connect->rotateFace_ == 0)
                {
                    _out << QString("*bearingsection, rotatingplane = xy\r\n");
                }
                else if (connect->rotateFace_ == 1)
                {
                    _out << QString("*bearingsection, rotatingplane = xz\r\n");
                }
                else if (connect->rotateFace_ == 2)
                {
                    _out << QString("*bearingsection, rotatingplane = yz\r\n");
                }
                if (connect->defMode_ == 0)
                {
                    _out << QString("0,%1,%2,%3,%4,%5,%6,%7,%8\r\n")
                        .arg(connect->bearingPara_[0])
                        .arg(connect->bearingPara_[1])
                        .arg(connect->bearingPara_[2])
                        .arg(connect->bearingPara_[3])
                        .arg(connect->bearingPara_[4])
                        .arg(connect->bearingPara_[5])
                        .arg(connect->bearingPara_[6])
                        .arg(connect->bearingPara_[7]);
                }
                else
                {
                    for (int i = 0; i < connect->curveRotate_.size(); ++i)
                    {
                        _out << QString("%1,%2,%3,%4,%5,%6,%7,%8,%9\r\n")
                            .arg(connect->curveRotate_[i][0])
                            .arg(connect->curveRotate_[i][1])
                            .arg(connect->curveRotate_[i][2])
                            .arg(connect->curveRotate_[i][3])
                            .arg(connect->curveRotate_[i][4])
                            .arg(connect->curveRotate_[i][5])
                            .arg(connect->curveRotate_[i][6])
                            .arg(connect->curveRotate_[i][7])
                            .arg(connect->curveRotate_[i][8]);
                    }
                }
                break;
            default:
                break;
            }
        }
        return true;

    }
    void mOMxFile::writeConnectorRigid(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorRigid> connect = dynamic_pointer_cast<MechConnectorData::mConnectorRigid>(connectorBase);
        if (!connect)
            return;

        _out << QString("*connector_part, name = \"%1\"\r\n").arg(connectorBase->getConnectorName());
        _out << QString("*connector, type = rigid, n = %1\r\n").arg(connectorBase->getConnectorId());

        _out << "*masternodes" << "\r\n";
        set<int> mainnode;
        getNodesByPickType(connect->mainnodes_.first, { connect->mainnodes_.second }, mainnode);
        if (mainnode.size() > 0)
        {
            _out << QString("%1").arg(*(mainnode.begin()));
        }
        _out << "\r\n";

        _out << "*slavenodes" << "\r\n";
        set<int> slavenodes;
        for (auto& it : connect->subnodes_)
        {
            getNodesByPickType(it.first, it.second, slavenodes);
        }
        if (slavenodes.size() > 0)
        {
            writeSetData(slavenodes);
        }
        else
        {
            _out << "\r\n";
        }

        writeConnectorSlaveNodeDof(connect->degreeoffreedom_main);
    }
    void mOMxFile::writeConnectorFlexible(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorFlexible> connect = dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(connectorBase);
        if (!connect)
            return;

        _out << QString("*connector_part, name = \"%1\"\r\n").arg(connectorBase->getConnectorName());
        _out << QString("*connector, type = flexible, n = %1\r\n").arg(connectorBase->getConnectorId());

        _out << "*masternodes" << "\r\n";
        set<int> mainnodes;
        for (auto& it : connect->mainnodes_)
        {
            if (it.first == PickDataType::Node)
            {
                vector<RenderPickResultData>& nodes = connect->mainnodes_[PickDataType::Node];
                for (int i = 0; i < nodes.size(); i++)
                {
                    double w = connect->singleweight_;
                    std::set<int> dof = connect->degreeoffreedom_main;
                    if (connect->isTableMode_ && i < (int)connect->w_.size())
                    {
                        w = connect->w_[i];
                    }
                    if (connect->isTableMode_ && i < (int)connect->dof_.size())
                    {
                        dof = connect->dof_[i];
                    }
                    _out << QString("%1, %2").arg(nodes[i].result_id).arg(w);
                    for (int d : dof)
                    {
                        _out << QString(", %1").arg(d);
                    }
                    _out << "\r\n";
                }
            }
            else
            {
                getNodesByPickType(it.first, it.second, mainnodes);
            }
        }
        if (mainnodes.size() > 0)
        {
            for (int node : mainnodes)
            {
                _out << QString("%1, %2").arg(node).arg(connect->singleweight_);
                for (int dof : connect->degreeoffreedom_main)
                {
                    _out << QString(", %1").arg(dof);
                }
                _out << "\r\n";
            }
        }



        _out << "*slavenodes" << "\r\n";
        set<int> slavenodes;
        getNodesByPickType(connect->subnodes_.first, { connect->subnodes_.second }, slavenodes);
        if (slavenodes.size() > 0)
        {
            _out << QString("%1").arg(*(slavenodes.begin()));
        }
        _out << "\r\n";

        writeConnectorSlaveNodeDof(connect->degreeoffreedom_sub);
    }
    void mOMxFile::writeConnectorBind(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorBind> connect = dynamic_pointer_cast<MechConnectorData::mConnectorBind>(connectorBase);
        if (!connect)
            return;

        int slaveID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Connector,connect->getConnectorId() }, Physical_Set, MapRelation_Slave);
        int masterID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Connector,connect->getConnectorId() }, Physical_Set, MapRelation_Master);
        QString slaveName = writeGeoSet(slaveID, ElementSurface);
        QString masterName = writeGeoSet(masterID, ElementSurface);

        _out << QString("*connector_part, name = \"%1\"\r\n").arg(connectorBase->getConnectorName());
        _out << QString("*connector");
        if (connect->type_ == 0)
        {
            _out << QString(", type = n2stie").arg(connectorBase->getConnectorId());
        }
        else
        {
            _out << QString(", type = s2stie").arg(connectorBase->getConnectorId());
        }
        _out << QString(", n = %1").arg(connectorBase->getConnectorId());

        if (connect->adjust_)
        {
            _out << QString(", adjust = on");
        }
        else
        {
            _out << QString(", adjust = off");
        }
        _out << "\r\n";

        _out << QString("\"%1\", \"%2\"\r\n").arg(slaveName).arg(masterName);

        _out << QString("*tol, type = %1\r\n").arg(connect->allowrancetype_);
        if (connect->allowrancetype_ == 0)
        {
            writeQVariantValue(connect->allowrance_);
            _out << "\r\n";
        }
    }
    void mOMxFile::writeConnectorMPC(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorMPC> connect = dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(connectorBase);
        if (!connect)
            return;

        _out << QString("*connector_part, name = \"%1\"\r\n").arg(connectorBase->getConnectorName());
        _out << QString("*connector, type = mpc, n = %1\r\n").arg(connectorBase->getConnectorId());

        if (connect->coefficient_.isValid())
        {
            _out << QString("%1\r\n").arg(connect->coefficient_.toDouble());
        }

        set<int> slavenodes;
        getNodesByPickType(connect->subnodes_.first, { connect->subnodes_.second }, slavenodes);
        if (slavenodes.size() > 0)
        {
            _out << QString("%1, %2, %3\r\n").arg(*(slavenodes.begin())).arg(connect->subDof_ + 1).arg(connect->subWeights_);
        }

        set<int> mainnodes;
        for (auto& it : connect->mainnodes_)
        {
            if (it.first == PickDataType::Node)
            {
                vector<RenderPickResultData>& nodes = connect->mainnodes_[PickDataType::Node];
                for (int i = 0; i < nodes.size(); i++)
                {
                    for (int j = 0; j < connect->mainweights_[i].size(); j++)
                    {
                        if (connect->mainweights_[i][j].isValid())
                        {
                            _out << QString("%1, %2, %3\r\n").arg(nodes[i].result_id).arg(j + 1).arg(connect->mainweights_[i][j].toDouble());
                        }
                    }
                }
            }
            else
            {
                getNodesByPickType(it.first, it.second, mainnodes);
            }
        }
        if (mainnodes.size() > 0)
        {
            for (int node : mainnodes)
            {
                for (auto it = connect->defaultweights_.begin(); it != connect->defaultweights_.end(); ++it)
                {
                    _out << QString("%1, %2, %3\r\n").arg(node).arg(it.key() + 1).arg(it.value());
                }
            }
        }
    }
    void mOMxFile::writeConnectorDisperse(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorDisperse> connect = dynamic_pointer_cast<MechConnectorData::mConnectorDisperse>(connectorBase);
        if (connect)
            return;

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Connector ,connect->getConnectorId() }, Physical_Set, MapRelation_Slave);
        QString slaveName = writeGeoSet(setID, Node);

        _out << "*rigidbody";//写入关键字
        //获取节点集合
        if (!slaveName.isEmpty())
        {
            //获取集合
            _out << QString(", refnode = \"%1\"").arg(slaveName);
        }
        else
        {
            _out << QString(", refnode = \"%1\"").arg("");
        }

        //获取单元集合
        setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Connector ,connect->getConnectorId() }, Physical_Set, MapRelation_Master);
        if (setID != -1)
        {
            //获取集合
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setID);
            _out << QString(", set = \"%1\"\r\n").arg(setName);
        }
        else
        {
            _out << QString(", set = \"%1\"\r\n").arg("");
        }
    }
    void mOMxFile::writeConnectorRigidbody(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorRigidbody> connect = dynamic_pointer_cast<MechConnectorData::mConnectorRigidbody>(connectorBase);
        if (!connect)
            return;

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Connector ,connect->getConnectorId() }, Physical_Set, MapRelation_Slave);
        QString slaveName = writeGeoSet(setID, Node);

        _out << "*rigidbody";//写入关键字
        //获取节点集合
        if (!slaveName.isEmpty())
        {
            //获取集合
            _out << QString(", refnode = \"%1\"").arg(slaveName);
        }
        else
        {
            _out << QString(", refnode = \"%1\"").arg("");
        }

        //获取单元集合
        setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Connector ,connect->getConnectorId() }, Physical_Set, MapRelation_Master);
        if (setID != -1)
        {
            //获取集合
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setID);
            _out << QString(", analyticalsurface = \"%1\"\r\n").arg(setName);

        }
        else
        {
            _out << QString(", analyticalsurface = \"%1\"\r\n").arg("");
        }
    }
    void mOMxFile::writeConnectorDistributedCoupling(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connect = dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(connectorBase);
        if (!connect)
            return;

        _out << QString("*connector_part, name = \"%1\"\r\n").arg(connectorBase->getConnectorName());
        _out << QString("*connector, type = distributing, n = %1\r\n").arg(connectorBase->getConnectorId());

        _out << "*masternodes" << "\r\n";
        set<int> mainnodes;
        for (auto& it : connect->mainnodes_)
        {
            if (it.first == PickDataType::Node)
            {
                vector<RenderPickResultData>& nodes = connect->mainnodes_[PickDataType::Node];
                if (connect->w_.size() >= nodes.size())
                {
                    for (int i = 0; i < nodes.size(); i++)
                    {
                        _out << QString("%1, %2\r\n").arg(nodes[i].result_id).arg(connect->w_[i]);
                    }
                }
                else
                {
                    for (int i = 0; i < nodes.size(); i++)
                    {
                        _out << QString("%1, %2\r\n").arg(nodes[i].result_id).arg(connect->singleweight_);
                    }
                }
            }
            else
            {
                getNodesByPickType(it.first, it.second, mainnodes);
            }
        }
        if (mainnodes.size() > 0)
        {
            for (int node : mainnodes)
            {
                _out << QString("%1, %2\r\n").arg(node).arg(connect->singleweight_);
            }
        }

        _out << "*slavenodes" << "\r\n";
        set<int> slavenodes;
        getNodesByPickType(connect->subnodes_.first, { connect->subnodes_.second }, slavenodes);
        if (slavenodes.size() > 0)
        {
            _out << QString("%1").arg(*(slavenodes.begin()));
        }
        _out << "\r\n";

        writeConnectorSlaveNodeDof(connect->degreeoffreedom_main);
    }

    void mOMxFile::writeConnectorBearingUnits(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorBearingUnits> connect = dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(connectorBase);
        if (!connect)
            return;

        _out << QString("*Element, type = bearing, part = \"%1\", section = \"%2\"\r\n").arg(connectorBase->getConnectorName()).arg(connectorBase->getConnectorName());
        int connectorID = connect->getConnectorId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int coor1 = mapRelation.getMapRelation<int>(
            { Physical_Connector, connectorID },
            Physical_Coordinate
        );
        int coor2 = mapRelation.getMapRelation<int>(
            { Physical_Connector, connectorID },
            Physical_Coordinate, MapRelation_Coord2
        );
        QString name1;
        QString name2;
        if (coor1 != -1)
        {
            name1 = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateNameByID(coor1);
        }
        else {
            name1 = "";
        }
        if (coor2 != -1)
        {
            name2 = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateNameByID(coor2);
        }
        else {
            name2 = "";
        }
        auto formatName = [](const QString& name) -> QString {
            return name.isEmpty() ? name : QString("\"%1\"").arg(name);
            };
        QString fName1 = formatName(name1);
        QString fName2 = formatName(name2);
        if (connect->type_ == 1)
        {
            _out << QString("%1, %2, %3, %4, %5\r\n")
                .arg(++_maxElementID, 10)
                .arg(connect->node1_.second.result_id)
                .arg(connect->node2_.second.result_id)
                .arg(fName1)
                .arg(fName2);
        }
        else {
            _out << QString("%1, %2, 0, %3, %4\r\n")
                .arg(++_maxElementID, 10)
                .arg(connect->node1_.second.result_id)
                .arg(fName1)
                .arg(fName2);
        }

    }

    void mOMxFile::writePretension(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase)
    {
        shared_ptr<MechConnectorData::mConnectorBolt> connect = dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(connectorBase);
        if (!connect)
            return;

        int setID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Connector ,connect->getConnectorId() }, Physical_Set);

        if (connect->type_ == 0)
        {
            QString setName = writeGeoSet(setID, Element);
            _out << QString("*pretension, node = %1").arg(connect->node_);
            _out << QString(", element = \"%1\"").arg(setName);
        }
        else
        {
            QString setName = writeGeoSet(setID, ElementSurface);
            _out << QString("*pretension, node = %1").arg(connect->node_);
            _out << QString(", set = \"%1\"").arg(setName);
        }

        _out << "\r\n";

        if (connect->isOpen_)
        {
            writeQVariantValue(connect->x_);
            _out << ", ";
            writeQVariantValue(connect->y_);
            _out << ", ";
            writeQVariantValue(connect->z_);
            _out << "\r\n";
        }
        else
        {
            _out << "\r\n";
        }
    }
    bool mOMxFile::writemCoordinate()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateID(false);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            MechCoordinateData::mCoordinateBase* coor = _modelData->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(id);
            if (!coor)
                continue;

            switch (coor->getCoordinateType())
            {
            case Cartesian:
            case Cylindrical:
            case Spherical:
                writeCoordinateBase(coor);
                break;
            case DiscreteCoordinate:
                writeDiscreteCoordinate(coor);
                break;
            default:
                break;
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("坐标系写文件完成");
        return true;
    }
    // 写原始坐标系
    void mOMxFile::writeRawCoordinate(MechCoordinateData::mCoordinateBase* coor, const QString& axisLine)
    {
        _out << "*coordinatesystem";
        switch (coor->getCoordinateType())
        {
        case Cartesian:
            _out << ", type = rectangular";
            break;
        case Cylindrical:
            _out << ", type = cylindrical";
            break;
        case Spherical:
            _out << ", type = spherical";
            break;
        default:
            break;
        }
        _out << QString(", name = \"%1\"").arg(coor->getCoordinateName()) << "\r\n";
        _out << axisLine << "\r\n";
    }
    // 写带铺层属性的坐标系
    void mOMxFile::writePlyCoordinate(MechCoordinateData::mCoordinateBase* coor, MechPropertyData::mPlyLaminate* plyLam, const QString& axisLine, bool isCopy)
    {
        if (!plyLam)
            return;

        _out << "*coordinatesystem";
        switch (coor->getCoordinateType())
        {
        case Cartesian:
            _out << ", type = rectangular";
            break;
        case Cylindrical:
            _out << ", type = cylindrical";
            break;
        case Spherical:
            _out << ", type = spherical";
            break;
        default:
            break;
        }

        // 副本坐标系 = 铺层名称 + 原坐标系名称
        QString finalName;
        if (isCopy)
        {
            finalName = QString("%1_%2").arg(plyLam->getPropertyName()).arg(coor->getCoordinateName());
        }
        else
        {
            finalName = coor->getCoordinateName();
        }

        _out << QString(", name = \"%1\"").arg(finalName);
        _out << QString(", normaldirection = %1").arg(plyLam->_normaldirection);

        if (plyLam->_rotAngleType == 1)
        {
            _out << "\r\n" << axisLine << ", " << plyLam->_rotAngle << "\r\n";
        }
        else if (plyLam->_rotAngleType == 2)
        {
            // 获取角离散场名称
            QString angleFieldName = "";
            int angleFieldID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Property, plyLam->getPropertyId() }, Physical_Field, MapRelation_RotationField);
            if (angleFieldID >= 0)
            {
                auto& dfMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
                auto* dfData = dfMgr.getDiscreteFieldData(angleFieldID);
                if (dfData && dfData->getSpaceType() == MxFunctions::AngelSpace)
                    angleFieldName = dfMgr.getFieldNameByID(angleFieldID);
            }
            _out << QString(", rotationfield = \"%1\"").arg(angleFieldName) << "\r\n";
            _out << axisLine << "\r\n";
        }
        else
        {
            _out << "\r\n" << axisLine << "\r\n";
        }
    }
    void mOMxFile::writeCoordinateBase(MechCoordinateData::mCoordinateBase* coor)
    {
        const int coorId = coor->getCoordinateId();

        // 获取引用关系 & 铺层ID
        const auto refMap = getCVM->getModelData()->getMapRelationManager().getCoordinateMap(coorId);

        auto& propMgr = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property);
        //获取所有铺层名称
        const auto propertyNames = propMgr.getAllPropertyNamesByTypes(PlyLaminateProperty);

        //通过铺层名称获取铺层id
        std::set<int> plyIdSet;
        for (const auto& name : propertyNames)
        {
            plyIdSet.insert(propMgr.getIDByPropertyName(name));
        }

        std::vector<int> plyRefIds;
        bool hasOtherReference = false;
        for (const auto& pair : refMap)
        {
            const auto fieldType = pair.first;
            const auto& idSet = pair.second;

            if (fieldType == MxFunctions::Physical_Property)
            {
                for (int id : idSet)
                {
                    if (plyIdSet.count(id))
                        plyRefIds.push_back(id);
                    else
                        hasOtherReference = true;
                }
            }
            else if (!idSet.empty())
            {
                hasOtherReference = true;
            }
        }

        // 缓存轴数据
        bool success; QString msg; QVector3D origin;
        const QVector<QVector3D> xyzAxis = coor->getXYZAxis(success, msg, origin);
        //单位制换算：坐标系三点(内部mm) → 当前界面单位制（长度）
        const QVector3D pOrigin(
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.x()),
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.y()),
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.z()));
        const QVector3D pZ(
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.x() + xyzAxis[2].x()),
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.y() + xyzAxis[2].y()),
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.z() + xyzAxis[2].z()));
        const QVector3D pX(
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.x() + xyzAxis[0].x()),
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.y() + xyzAxis[0].y()),
            mUnitIoCodec::encode(MxFunctions::Unit::Length, origin.z() + xyzAxis[0].z()));
        const QString axisLine = QString("%1, %2, %3").arg(pOrigin.x()).arg(pOrigin.y()).arg(pOrigin.z())
            + QString(", %1, %2, %3").arg(pZ.x()).arg(pZ.y()).arg(pZ.z())
            + QString(", %1, %2, %3").arg(pX.x()).arg(pX.y()).arg(pX.z());

        // 分支输出
        if (hasOtherReference)
        {
            writeRawCoordinate(coor, axisLine);

            for (int plyId : plyRefIds)
            {
                auto plyLam = dynamic_cast<MechPropertyData::mPlyLaminate*>(propMgr.getPropertyData(plyId));
                writePlyCoordinate(coor, plyLam, axisLine, true); // 副本 → 改名
            }
        }
        else
        {
            if (plyRefIds.size() == 1)
            {
                auto plyLam = dynamic_cast<MechPropertyData::mPlyLaminate*>(propMgr.getPropertyData(plyRefIds[0]));
                writePlyCoordinate(coor, plyLam, axisLine, false); // 不是副本 → 不改名
            }
            else
            {
                writeRawCoordinate(coor, axisLine);

                for (int plyId : plyRefIds)
                {
                    auto plyLam = dynamic_cast<MechPropertyData::mPlyLaminate*>(propMgr.getPropertyData(plyId));
                    writePlyCoordinate(coor, plyLam, axisLine, true); // 副本 → 改名
                }
            }
        }
    }

    void mOMxFile::writeDiscreteCoordinate(MechCoordinateData::mCoordinateBase* coor)
    {
        const int coorId = coor->getCoordinateId();

        // 获取离散场名称
        QString distributionName;
        int discreteFieldID = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Coordinate, coorId }, Physical_Field);
        if (discreteFieldID >= 0)
        {
            auto& manager = _modelData->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
            distributionName = manager.getFieldNameByID(discreteFieldID);
        }

        // 获取引用关系 & 铺层ID
        const auto refMap = getCVM->getModelData()->getMapRelationManager().getCoordinateMap(coorId);
        auto& propMgr = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property);
        const auto propertyNames = propMgr.getAllPropertyNamesByTypes(PlyLaminateProperty);
        std::set<int> plyIdSet;
        for (const auto& name : propertyNames)
        {
            plyIdSet.insert(propMgr.getIDByPropertyName(name));
        }

        std::vector<int> plyRefIds;
        bool hasOtherReference = false;
        for (const auto& pair : refMap)
        {
            const auto fieldType = pair.first;
            const auto& idSet = pair.second;

            if (fieldType == MxFunctions::Physical_Property)
            {
                for (int id : idSet)
                {
                    if (plyIdSet.count(id))
                        plyRefIds.push_back(id);
                    else
                        hasOtherReference = true;
                }
            }
            else if (!idSet.empty())
            {
                hasOtherReference = true;
            }
        }

        // 辅助：写离散坐标系基本内容
        auto writeDiscreteBase = [&](const QString& name)
            {
                _out << "*coordinatesystem, type = distribution";
                _out << QString(", name = \"%1\"").arg(name);
                _out << QString(", distribution = \"%1\"").arg(distributionName);
            };

        // 辅助：写附加属性（normaldirection / theta / rotationfield）
        auto writeAppendix = [&](int normaldirection, int rotAngleType, const QString& rotAngle, const QString& rotFieldName)
            {
                _out << QString(", normaldirection = %1").arg(normaldirection);
                if (rotAngleType == 1)
                {
                    _out << QString(", theta = %1").arg(rotAngle);
                }
                else if (rotAngleType == 2)
                {
                    _out << QString(", rotationfield = \"%1\"").arg(rotFieldName);
                }
            };

        // 辅助：通过映射获取 rotationfield 离散场名称
        auto getRotationFieldName = [&](MxFunctions::PhysicalFieldType physicalType, int id) -> QString {
            int fieldID = _modelData->getMapRelationManager().getMapRelation<int>({ physicalType, id }, Physical_Field, MapRelation_RotationField);

            if (fieldID < 0) return "";
            auto& manager = _modelData->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
            return manager.getFieldNameByID(fieldID);
            };

        // 分支输出
        if (hasOtherReference)
        {
            // 有其他引用：写原始（无附加属性）
            writeDiscreteBase(coor->getCoordinateName());
            _out << "\r\n";

            // 为每个铺层写副本（带附加属性，改名）
            for (int plyId : plyRefIds)
            {
                auto plyLam = dynamic_cast<MechPropertyData::mPlyLaminate*>(propMgr.getPropertyData(plyId));
                if (!plyLam) continue;
                QString copyName = QString("%1_%2").arg(plyLam->getPropertyName()).arg(coor->getCoordinateName());
                writeDiscreteBase(copyName);
                writeAppendix(plyLam->_normaldirection, plyLam->_rotAngleType, plyLam->_rotAngle, getRotationFieldName(MxFunctions::Physical_Property, plyLam->getPropertyId()));
                _out << "\r\n";
            }
        }
        else
        {
            if (plyRefIds.size() == 1)
            {
                // 只有一个铺层引用：基本内容 + 铺层附加属性，不改名
                auto plyLam = dynamic_cast<MechPropertyData::mPlyLaminate*>(propMgr.getPropertyData(plyRefIds[0]));
                if (plyLam)
                {
                    writeDiscreteBase(coor->getCoordinateName());
                    writeAppendix(plyLam->_normaldirection, plyLam->_rotAngleType, plyLam->_rotAngle, getRotationFieldName(MxFunctions::Physical_Property, plyLam->getPropertyId()));
                    _out << "\r\n";
                }
            }
            else
            {
                // 0个或≥2个铺层：先写原始（带自身附加属性）
                writeDiscreteBase(coor->getCoordinateName());
                _out << "\r\n";

                // 再为每个铺层写副本（带附加属性，改名）
                for (int plyId : plyRefIds)
                {
                    auto plyLam = dynamic_cast<MechPropertyData::mPlyLaminate*>(propMgr.getPropertyData(plyId));
                    if (!plyLam) continue;
                    QString copyName = QString("%1_%2").arg(plyLam->getPropertyName()).arg(coor->getCoordinateName());
                    writeDiscreteBase(copyName);
                    writeAppendix(plyLam->_normaldirection, plyLam->_rotAngleType, plyLam->_rotAngle, getRotationFieldName(MxFunctions::Physical_Property, plyLam->getPropertyId()));
                    _out << "\r\n";
                }
            }
        }
    }

    bool mOMxFile::writeFreqSamplPointData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getAllFreqSamplPointID();

        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            MechCaseData::mFreqSamplPointBase* freq = _modelData->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getFreqSamplPointData(id);
            if (!freq)
                continue;

            //获取勾选情况
            QList<bool> checked = freq->_checked;
            //如果没有勾选任何选项，直接跳过
            if (!checked.contains(true))
                continue;

            /*
            *freqpoint, type = freq, name = "FREQi_30_1"
            F1, F2, F3,...
            */
            if (checked[0])
            {
                _out << QString("*freqpoint, type = freq,  name = \"%1\"").arg(freq->getFreqSamplPointName() + "_1");
                _out << "\r\n";

                QVector<double> point = freq->_freqSamplPoint;

                // 循环输出 double 数值，用逗号分隔
                for (int i = 0; i < point.size(); ++i)
                {
                    _out << point[i];

                    // 不是最后一项才加逗号
                    if (i != point.size() - 1)
                    {
                        _out << ", ";
                    }
                }
                _out << "\r\n";
            }

            /*
            *freqpoint, type = freq1, name = "freqi_3_1"
            F1, DF, NDF
            */
            if (checked[1])
            {
                _out << QString("*freqpoint, type = freq1, name = \"%1\"").arg(freq->getFreqSamplPointName() + "_2");
                _out << "\r\n";
                MechCaseData::FreqSamplPoint1 point1 = freq->_freqSamplPoint1;
                _out << point1.startFreq << ", ";
                _out << point1.stepFreq << ", ";
                _out << point1.intervalNum;
                _out << "\r\n";
            }

            /*
            *freqpoint, type = freq2, name = "freqi_3_1"
            F1, F2, NF
            */
            if (checked[2])
            {
                _out << QString("*freqpoint, type = freq2, name = \"%1\"").arg(freq->getFreqSamplPointName() + "_3");
                _out << "\r\n";
                MechCaseData::FreqSamplPoint2 point2 = freq->_freqSamplPoint2;
                _out << point2.startFreq << ", ";
                _out << point2.stepFreq << ", ";
                _out << point2.totalPoints;
                _out << "\r\n";
            }

            /*
            *freqpoint, type = freq5, name = "freqi_3_1"
            F1, F2, NEF, bias, scale
            */
            if (checked[3])
            {
                _out << QString("*freqpoint, type = freq5, name =\"%1\"").arg(freq->getFreqSamplPointName() + "_5");
                _out << "\r\n";
                MechCaseData::FreqSamplPoint3 point3 = freq->_freqSamplPoint3;
                _out << point3.startFreq << ", ";
                _out << point3.endFreq << ", ";
                _out << point3.refineNum << ", ";
                _out << point3.expandValue << ", ";
                _out << point3.diffType;
                _out << "\r\n";
            }

            /*
            *freqpoint, type = freq4, name = "freqi_3_1"
            F1, DF, FSPD, NDF
            */
            if (checked[4])
            {
                _out << QString("*freqpoint, type = freq4, name = \"%1\"").arg(freq->getFreqSamplPointName() + "_4");
                _out << "\r\n";
                MechCaseData::FreqSamplPoint4 point4 = freq->_freqSamplPoint4;
                _out << point4.lowerFreq << ", ";
                _out << point4.stepFreq << ", ";
                _out << point4.shiftValue << ", ";
                _out << point4.sampleNum;
                _out << "\r\n";
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("采样频率点写文件完成");
        return true;
    }
    bool mOMxFile::writeLaminateData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getAllLaminateID();

        if (ids.size() == 0)
        {
            return true;
        }

        for (auto id : ids)
        {
            MechPropertyData::mLaminateBase* laminateBase = _modelData->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getLaminateData(id);

            if (!laminateBase)
                continue;

            auto type = laminateBase->getLaminateType();

            switch (type)
            {
                /**layer, name = “Ply_1”, shape = “SetName”, distribution = “disName”
                  “material - 1”, theta, thick*/
            case MxFunctions::SglLayerPlate:
            {
                MechPropertyData::mSglLayerPlate* sglLayer = dynamic_cast<MechPropertyData::mSglLayerPlate*>(laminateBase);

                //获取集合映射
                int setid = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Laminate, sglLayer->getLaminateId() }, Physical_Set);
                //获取材料映射
                int matId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Laminate, sglLayer->getLaminateId() }, Physical_Material);
                if (!sglLayer || sglLayer->_thick.isEmpty() || setid == -1 || matId == -1) continue; //指针不存在或厚度未填写或未选择集合或未设置材料都直接跳过

                _out << QString("*layer, name = \"%1\"").arg(sglLayer->getLaminateName());
                //写集合
                QString setname;
                if (setid != -1)
                {
                    setname = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setid);
                }
                // 获取离散转角分布场映射
                int discreteFieldID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>(
                    { Physical_Laminate, sglLayer->getLaminateId() }, Physical_Field);

                _out << QString(", shape = \"%1\"").arg(setname);
                if (discreteFieldID >= 0)
                {
                    auto& manager = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
                    QString distributionName = manager.getFieldNameByID(discreteFieldID);
                    _out << QString(", distribution = \"%1\"").arg(distributionName);
                }
                _out << "\r\n";
                //写材料
                QString matname;
                if (matId != -1)
                {
                    matname = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMatNameByID(matId);
                }
                _out << QString("\"%1\"").arg(matname);
                _out << QString(", %1, %2\r\n").arg(sglLayer->_theta).arg(sglLayer->_thick);
            }
            break;

            /**layer, type = global, name = “GlobalPly_1”
              “material - 1”, theta, thick*/
            case MxFunctions::GlobalSglLam:
            {
                MechPropertyData::mGlobalSglLam* gloSgl = dynamic_cast<MechPropertyData::mGlobalSglLam*>(laminateBase);
                //获取材料映射
                int matId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Laminate, gloSgl->getLaminateId() }, Physical_Material);
                if (!gloSgl || gloSgl->_thick.isEmpty() || matId == -1) continue;//指针不存在或者厚度为空或者材料未选择直接跳过

                _out << QString("*layer, type = global, name = \"%1\"").arg(gloSgl->getLaminateName());
                _out << "\r\n";
                //写材料
                QString matname;
                if (matId != -1)
                {
                    matname = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMatNameByID(matId);
                }
                _out << QString("\"%1\"").arg(matname);
                _out << QString(", %1, %2\r\n").arg(gloSgl->_theta).arg(gloSgl->_thick);
            }
            break;

            /*   *stack, name = “laminate1”
              //“Ply - 1”, ”Ply_2”, ..., “Ply_8”
               “Ply_9”
            */
            case MxFunctions::LamPlate:
            {
                MechPropertyData::mLamPlate* lam = dynamic_cast<MechPropertyData::mLamPlate*>(laminateBase);
                // 获取层合结构映射
                QList<int> subLamIds = getCVM->getModelData()
                    ->getMapRelationManager()
                    .getMapRelation<QList<int>>({ Physical_Laminate, lam->getLaminateId() }, Physical_Laminate);

                if (!lam || subLamIds.size() == 0) continue; //未选择集合直接跳过

                _out << QString("*stack, name = \"%1\"").arg(lam->getLaminateName());
                _out << "\r\n";

                // 按8个一行格式化输出
                const int ITEMS_PER_LINE = 8;
                for (int i = 0; i < subLamIds.size(); i += ITEMS_PER_LINE)
                {
                    // 计算当前行的结束位置（不超过列表总长度）
                    int lineEnd = qMin(i + ITEMS_PER_LINE, subLamIds.size());

                    // 输出当前行的所有元素
                    for (int j = i; j < lineEnd; ++j)
                    {
                        int subLamId = subLamIds[j];
                        QString subLamName = getCVM->getModelData()->getManager<MechPropertyData::mLaminateManager>(Physical_Laminate).getLaminateNameByID(subLamId);

                        // 输出带引号的名称
                        _out << "\"" << subLamName << "\"";

                        // 只有不是当前行最后一个元素时才加逗号
                        if (j != lineEnd - 1)
                        {
                            _out << ", ";
                        }
                    }
                    // 每行结束换行
                    _out << "\r\n";
                }
            }
            break;
            default:
                break;
            }

        }
    }
    bool mOMxFile::writemGlobalParam()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        auto matrix = _modelData->getGlobalParam()->getGenericMatrixData();
        mTPContriData* tpData = _modelData->getGlobalParam()->getTPContriData();

        auto assign = _modelData->getGlobalParam()->getGenericAssignData();
        bool hasMatrixParam = false;
        QString stiffnessNames;
        QString massNames;
        QString dampingNames;
        QString materialNames;
        if (assign)
        {
            // 预处理：获取所有非空矩阵
            stiffnessNames = assign->_stiffnessMatrixNames.trimmed();
            massNames = assign->_massMatrixNames.trimmed();
            dampingNames = assign->_viscousDampingMatrixNames.trimmed();
            materialNames = assign->_materialMatrixNames.trimmed();

            // 只有当至少有一个矩阵非空时，才输出*param块
            hasMatrixParam = !stiffnessNames.isEmpty() ||
                !massNames.isEmpty() ||
                !dampingNames.isEmpty() ||
                !materialNames.isEmpty();
        }



        if (!matrix && !tpData && !hasMatrixParam) return true;

        _out << "*param\r\n";
        if (matrix)
        {
            _out << QString("DMIGNAME = %1\r\n").arg(matrix->_matrixName);
        }
        if (tpData)
        {
            _out << QString("tpa = \"%1\"\r\n").arg("TPA_1");
        }

        if (hasMatrixParam)
        {
            // 有哪个矩阵就输出哪个，空矩阵不输出
            if (!stiffnessNames.isEmpty())
            {
                _out << QString("K2GG = %1\r\n").arg(stiffnessNames);
            }

            if (!massNames.isEmpty())
            {
                _out << QString("M2GG = %1\r\n").arg(massNames);
            }

            if (!dampingNames.isEmpty())
            {
                _out << QString("B2GG = %1\r\n").arg(dampingNames);
            }

            if (!materialNames.isEmpty())
            {
                _out << QString("K42GG = %1\r\n").arg(materialNames);
            }
        }

        return true;
    }
    bool mOMxFile::writePeakFilterData()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getGlobalParam()->getAllPeakFilterIDs();
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto id : ids)
        {
            auto pf = _modelData->getGlobalParam()->getPeakFilterData(id);
            if (!pf) continue;

            _out << QString("*peakout, type = %1, scale = %2, name=\"%3\"\r\n")
                .arg(pf->_respType).arg(pf->_identType).arg(pf->getName());
            _out << QString("%1, %2, %3, %4, %5\r\n")
                .arg(pf->_extractCount).arg(pf->_minSpacing).arg(pf->_maxSpacing)
                .arg(pf->_startFreq).arg(pf->_endFreq);

            for (int r = 0; r < pf->_tableData.size(); ++r)
            {
                if (pf->_tableData[r].size() >= 3)
                {
                    _out << QString("%1, %2, %3\r\n")
                        .arg(pf->_tableData[r][0])
                        .arg(pf->_tableData[r][1])
                        .arg(pf->_tableData[r][2]);
                }
            }
            _out << "\r\n";
        }

        return true;
    }
    void mOMxFile::writeCasePeakFilterData(MechCaseData::mCaseBase* caseBase)
    {
        if (!caseBase) return;

        QList<int> pfIds = _modelData->getMapRelationManager()
            .getMapRelation<QList<int>>({ Physical_Case, caseBase->getCaseId() }, Physical_PeakFilter);

        if (pfIds.isEmpty()) return;

        _out << "*peakout\r\n";
        for (int pfId : pfIds)
        {
            QString name = _modelData->getGlobalParam()->getPeakFilterNameByID(pfId);
            if (!name.isEmpty())
            {
                _out << QString("\"%1\"\r\n").arg(name);
            }
        }
    }
    bool mOMxFile::writemGenericParam()
    {
        vector<int> ids = _modelData->getGlobalParam()->getAllGenericParamIDs();
        for (auto id : ids)
        {
            auto param = _modelData->getGlobalParam()->getGenericParamData(id);
            if (!param) continue;

            _out << QString("*CMSMethod, method = %1, name = \"%2\"\r\n").arg(param->_method).arg(param->getName());
            if (!(param->_method == "GUYAN" && param->_upFrequency.isEmpty() && param->_order.isEmpty() && param->_startID.isEmpty()))
            {
                _out << QString("%1, %2, %3\r\n").arg(param->_upFrequency).arg(param->_order).arg(param->_startID);
            }

            _out << "\r\n";
        }
        return true;
    }

    bool mOMxFile::writeASCouplingSearchData()
    {
        auto searchData = _modelData->getGlobalParam()->getASCouplingSearchData();
        if (!searchData) return true;

        QString autosearche = searchData->getAutosearch() ? "on" : "off";
        QString type = searchData->getMainSurface();
        _out << QString("*fsi_pair, autosearch = %1, type = %2").arg(autosearche).arg(type);
        _out << "\r\n";

        if (searchData->getAngelChecked())
        {
            _out << QString("*angle");
            _out << "\r\n";
            _out << searchData->getSearchAngel();
            _out << "\r\n";
        }
        if (searchData->getOutChecked())
        {
            _out << QString("*normal");
            _out << "\r\n";
            _out << searchData->getOuterScale();
            _out << "\r\n";
        }
        if (searchData->getInnerChecked())
        {
            _out << QString("*intol");
            _out << "\r\n";
            _out << searchData->getInnerScale();
            _out << "\r\n";
        }
        if (searchData->getHorizontalChecked())
        {
            _out << QString("*skneps");
            _out << "\r\n";
            _out << searchData->getHorizontalScale();
            _out << "\r\n";
        }
        if (searchData->getBoundingBoxChecked())
        {
            _out << QString("*aabb");
            _out << "\r\n";
            _out << searchData->getBoundingBoxScale();
            _out << "\r\n";
        }

        return true;
    }
    bool mOMxFile::writemGenericAssign()
    {
        auto assign = _modelData->getGlobalParam()->getGenericAssignData();
        if (!assign) return true;

        // 预处理：获取所有非空矩阵
        QString stiffnessNames = assign->_stiffnessMatrixNames.trimmed();
        QString massNames = assign->_massMatrixNames.trimmed();
        QString dampingNames = assign->_viscousDampingMatrixNames.trimmed();
        QString materialNames = assign->_materialMatrixNames.trimmed();

        // 只有当至少有一个矩阵非空时，才输出*param块
        bool hasMatrixParam = !stiffnessNames.isEmpty() ||
            !massNames.isEmpty() ||
            !dampingNames.isEmpty() ||
            !materialNames.isEmpty();

        if (hasMatrixParam)
        {
            _out << "*param\r\n";

            // 有哪个矩阵就输出哪个，空矩阵不输出
            if (!stiffnessNames.isEmpty())
            {
                _out << QString("K2GG = %1\r\n").arg(stiffnessNames);
            }

            if (!massNames.isEmpty())
            {
                _out << QString("M2GG = %1\r\n").arg(massNames);
            }

            if (!dampingNames.isEmpty())
            {
                _out << QString("B2GG = %1\r\n").arg(dampingNames);
            }

            if (!materialNames.isEmpty())
            {
                _out << QString("K42GG = %1\r\n").arg(materialNames);
            }
        }

        // 输出超单元文件路径
        _out << "*assign\r\n";
        for (const auto& path : assign->_filePath)
        {
            _out << QString("\"%1\"\r\n").arg(path);
        }

        return true;
    }
    bool mOMxFile::writeFunction()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Function);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            MechFunctionData::mFunctionBase* func = _modelData->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionData(id);
            if (!func)
                continue;

            switch (func->getFunctionType())
            {
            case 2:
                writeFunctionInterpCol(func);
                break;
            case 5:
                writeFunction1(func);
                break;
            case 6:
                writeFunction2(func);
                break;
            case 7:
                writeFunction3(func);
                break;
            case 8:
                writeFunction4(func);
                break;
            case 9:
                writeFunctionPeriodic(func);
                break;
            case 10:
                writeFunctionModulated(func);
                break;
            case 11:
                writeFunctionDecay(func);
                break;
            default:
                break;
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("接触写文件完成");
        return true;
    }
    void mOMxFile::writeFunction1(MechFunctionData::mFunctionBase* func)
    {
        _out << "*function";
        _out << ", type = function1";
        if (func->table().xAxisType == 0)
        {
            _out << ", axisx = linear";
        }
        else if (func->table().xAxisType == 1)
        {
            _out << ", axisx = log";
        }

        if (func->table().yAxisType == 0)
        {
            _out << ", axisy = linear";
        }
        else if (func->table().yAxisType == 1)
        {
            _out << ", axisy = log";
        }

        _out << QString(", extend = %1").arg(func->table().extrapMode);
        _out << QString(", name = \"%1\"").arg(func->getFunctionName());
        _out << "\r\n";

        for (int i = 0; i < func->table().x.size(); i++)
        {
            _out << QString("%1, %2\r\n").arg(func->table().x[i]).arg(func->table().y[i]);
        }
        _out << "\r\n";
    }
    void mOMxFile::writeFunction2(MechFunctionData::mFunctionBase* func)
    {
        _out << "*function";
        _out << ", type = function2";

        _out << QString(", extend = %1").arg(func->table().extrapMode);
        _out << QString(", name = \"%1\"").arg(func->getFunctionName());
        _out << QString(", x1 = %1").arg(func->table().x1);
        _out << "\r\n";

        for (int i = 0; i < func->table().x.size(); i++)
        {
            _out << QString("%1, %2\r\n").arg(func->table().x[i]).arg(func->table().y[i]);
        }
        _out << "\r\n";
    }
    void mOMxFile::writeFunction3(MechFunctionData::mFunctionBase* func)
    {
        _out << "*function";
        _out << ", type = function3";

        _out << QString(", extend = %1").arg(func->table().extrapMode);
        _out << QString(", name = \"%1\"").arg(func->getFunctionName());
        _out << QString(", x1 = %1").arg(func->table().x1);
        _out << QString(", x2 = %1").arg(func->table().x2);
        _out << "\r\n";

        for (int i = 0; i < func->table().x.size(); i++)
        {
            _out << QString("%1, %2\r\n").arg(func->table().x[i]).arg(func->table().y[i]);
        }
        _out << "\r\n";
    }
    void mOMxFile::writeFunction4(MechFunctionData::mFunctionBase* func)
    {
        _out << "*function";
        _out << ", type = function4";

        _out << QString(", name = \"%1\"").arg(func->getFunctionName());
        _out << "\r\n";

        _out << QString("%1, %2, %3, %4\r\n").arg(func->table().x1).arg(func->table().x2).arg(func->table().x3).arg(func->table().x4);


        int num = 0;
        for (int i = 0; i < func->table().x.size(); i++)
        {
            num++;
            _out << QString("%1").arg(func->table().x[i]);
            if (num % 16 == 0 || num == func->table().x.size())
            {
                _out << "\r\n";
            }
            else
            {
                _out << ",";
            }
        }

        _out << "\r\n";
    }
    void mOMxFile::writeFunctionInterpCol(MechFunctionData::mFunctionBase* func)
    {
        int mode = func->interp().interpMode;
        // 0-临近点 1-线性 2-分段三次 3-三次样条 4-对数插值 5-光滑 6-阶跃
        _out << "*function";

        if (mode == 4)
        {
            _out << ", type = log";
        }
        else if (mode == 1)
        {
            _out << ", type = linear";
        }
        else if (mode == 5)
        {
            _out << ", type = smooth";
        }
        else if (mode == 6)
        {
            _out << ", type = step";
        }
        else
        {
            // 其余插值方式（临近点/分段三次/三次样条）暂不支持导出
            return;
        }

        if (!(mode == 5 || mode == 6))
        {
            if (func->interp().extrapMode == 0)
            {
                _out << QString(", extend = const");
            }
            else
            {
                _out << QString(", extend = linear");
            }
        }

        _out << QString(", name = \"%1\"").arg(func->getFunctionName());
        _out << "\r\n";

        if (mode == 5 || mode == 6)
        {
            int n = (int)func->interp().x.size();
            if (n <= 0)
            {
                _out << "\r\n";
                return;
            }
            _out << QString("%1, %2\r\n").arg(func->interp().x.front()).arg(func->interp().y.front());
            if (n > 1)
            {
                _out << QString("%1, %2\r\n").arg(func->interp().x.back()).arg(func->interp().y.back());
            }
            _out << "\r\n";
            return;
        }

        for (int i = 0; i < func->interp().x.size(); i++)
        {
            _out << QString("%1, %2\r\n").arg(func->interp().x[i]).arg(func->interp().y[i]);
        }
        _out << "\r\n";
    }

    void mOMxFile::writeFunctionPeriodic(MechFunctionData::mFunctionBase* func)
    {
        const MechFunctionData::PeriodicCol& p = func->periodic();

        _out << QString("*function, type = periodic, name = \"%1\"\r\n").arg(func->getFunctionName());
        _out << QString("%1, %2, %3, %4\r\n")
            .arg(p.order)
            .arg(p.freq)
            .arg(p.t0)
            .arg(p.A0);

        int n = (int)p.A.size();
        for (int i = 0; i < n; ++i)
        {
            double bv = (i < (int)p.B.size()) ? p.B[i] : 0.0;
            _out << QString("%1, %2\r\n").arg(p.A[i]).arg(bv);
        }
        _out << "\r\n";
    }

    void mOMxFile::writeFunctionModulated(MechFunctionData::mFunctionBase* func)
    {
        // *function, type = modulated, name = "xxx"
        // A0, A, t0, f1, f2
        const MechFunctionData::ModulatedCol& m = func->modulated();

        _out << QString("*function, type = modulated, name = \"%1\"\r\n").arg(func->getFunctionName());
        _out << QString("%1, %2, %3, %4, %5\r\n")
            .arg(m.A0).arg(m.A).arg(m.t0).arg(m.f1).arg(m.f2);
        _out << "\r\n";
    }

    void mOMxFile::writeFunctionDecay(MechFunctionData::mFunctionBase* func)
    {
        // *function, type = decay, name = "xxx"
        // A0, Am, t0, td
        const MechFunctionData::ExpDecayCol& d = func->expDecay();

        _out << QString("*function, type = decay, name = \"%1\"\r\n").arg(func->getFunctionName());
        _out << QString("%1, %2, %3, %4\r\n")
            .arg(d.A0).arg(d.Am).arg(d.t0).arg(d.td);
        _out << "\r\n";
    }
    bool mOMxFile::writeConnectorDeputy()
    {
        if (!MAppConfiguration::mAppConfiguration::getInstance()->threadFlag)
        {
            _file.close();
            return false;
        }

        vector<int> ids = _modelData->getAllPhyID(Physical_Deputy);
        if (ids.size() == 0)
        {
            return true;
        }

        for (auto& id : ids)
        {
            shared_ptr<MechConnectorData::mDeputyBase> base = _modelData->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyData(id);
            if (!base || base->isSuppressed())
                continue;

            switch (base->getDeputyType())
            {
            case Deputy_Connector:
                writeDeputy(base);
                break;
            case Deputy_Fastener:
                writeFastener(base);
                break;
            default:
                break;
            }
        }

        mGlobalSignals::getInstance()->outputMessageSig("连接副写文件完成");
        return true;
    }
    void mOMxFile::writeDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> base)
    {
        shared_ptr<MechConnectorData::mConnectingDeputy> deputy = dynamic_pointer_cast<MechConnectorData::mConnectingDeputy>(base);
        if (!deputy)
            return;

        _out << QString("*section, type = structural, name = \"%1\"").arg(base->getDeputyName());
        writeMapIntPropertyData(Physical_Deputy, base->getDeputyId(), Physical_Coordinate);
        _out << "\r\n";

        auto writeType2 = [&]() {
            if (deputy->subdatatype_ == "旋转")
            {
                _out << ", revolute";
            }
            else if (deputy->subdatatype_ == "对齐")
            {
                _out << ", align";
            }
            else if (deputy->subdatatype_ == "万向节")
            {
                _out << ", universal";
            }
            };

        _out << "*connectorsection\r\n";
        if (deputy->datatype_ == "球铰")
        {
            _out << "join";
            writeType2();
        }
        else if (deputy->datatype_ == "滑动")
        {
            _out << "slot";
            writeType2();
        }
        else if (deputy->datatype_ == "铰接")
        {
            _out << "hinge";
        }
        else if (deputy->datatype_ == "焊接")
        {
            _out << "weld";
        }
        else if (deputy->datatype_ == "圆柱")
        {
            _out << "cylindrical";
        }
        else if (deputy->datatype_ == "平移")
        {
            _out << "translator";
        }
        _out << "\r\n";
    }
    void mOMxFile::writeFastener(std::shared_ptr<MechConnectorData::mDeputyBase> base)
    {
        shared_ptr<MechConnectorData::mConnectorFastener> deputy = dynamic_pointer_cast<MechConnectorData::mConnectorFastener>(base);
        if (!deputy)
            return;

        if (deputy->defineType_ == 0)
        {
            if (!_meshData->NodeIsHaveByID(deputy->nodeId_)) return;

            _out << QString("*section, type = structural, name = \"%1\"\r\n").arg(deputy->getDeputyName());
            _out << "*connectorsection\r\n";
            _out << "weld\r\n";
            _out << QString("*connector, type = fastener, n = %1, part = \"%2\"\r\n").arg(deputy->getDeputyId()).arg(deputy->getDeputyName());
        }
        else if (deputy->defineType_ == 1)
        {
            int id = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_Deputy, deputy->getDeputyId() }, Physical_Deputy);
            std::shared_ptr<MechConnectorData::mDeputyBase> refBase = _modelData->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyData(id);
            QString name;
            if (refBase && refBase->isSuppressed())
            {
                name = QString(); // 被引用 deputy 抑制时保留空 part 名称占位
            }
            else
            {
                name = _modelData->getNameByPhyID(Physical_Deputy, id);
            }
            _out << QString("*connector, type = fastener, n = %1, part = \"%2\"\r\n").arg(deputy->getDeputyId()).arg(name);
        }

        _out << deputy->influenceRadius_ << "\r\n";

        QList<int> partIDs = _modelData->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Deputy, deputy->getDeputyId() }, Physical_Part);
        writeMultiRowData(Physical_Part, partIDs);

        _out << "\r\n";
    }
}


